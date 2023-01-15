/**
 * elf2efi.c
 *
 * Convert elf object file to an efi bootable image
 * 
 * To fool around with the processor, we want a directly bootable efi image
 * without a bootloader. The problem is efi uses a non-standard PE32+ format.
 * A cross compiler can do this, but currently don't feel like that for just
 * several hundred lines of code.
 *
 * The conversion logic comes from grub-mkimage, currently we only focus
 * on x86_64.
 *
 * Other ways to do this:
 * 
 * gnu-efi: https://wiki.osdev.org/GNU-EFI
 * llvm: https://dvdhrm.github.io/2019/01/31/goodbye-gnuefi/
 * gcc cross compiler: https://wiki.osdev.org/UEFI_App_Bare_Bones
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <elf.h>
#include "pe32.h"

#define ALIGN_UP(addr, align) \
	(((addr) + (typeof (addr)) (align) - 1) & ~((typeof (addr)) (align) - 1))

#define EFI_PAGE_SHIFT		12
#define EFI_PAGE_SIZE		(1 << EFI_PAGE_SHIFT)
#define PE32_SECTION_ALIGNMENT	EFI_PAGE_SIZE
#define PE32_FILE_ALIGNMENT	PE32_SECTION_ALIGNMENT
#define PE32_MSDOS_STUB_SIZE	0x80
#define PE32_SIGNATURE_SIZE	4
#define PE32_FIXUP_ENTRY(type, offset)	(((type) << 12) | (offset))
#define PE32_MACHINE_X86_64	0x8664

#define EFI64_HEADER_SIZE \
 	ALIGN_UP (PE32_MSDOS_STUB_SIZE		\
		  + PE32_SIGNATURE_SIZE		\
		  + sizeof (struct pe32_coff_header) \
		  + sizeof (struct pe64_optional_header) \
		  + 4 * sizeof (struct pe32_section_table), \
		  PE32_FILE_ALIGNMENT)
#define VADDR_OFFSET EFI64_HEADER_SIZE

#define STABLE_EMBEDDING_TIMESTAMP	1420070400

#define PE_OHDR(o32, o64, field) (*(		\
{						\
  __typeof__((o64)->field) tmp_;		\
  __typeof__((o64)->field) *ret_ = &tmp_;	\
  if (o32)					\
    ret_ = (void *)(&((o32)->field));		\
  else if (o64)					\
    ret_ = (void *)(&((o64)->field));		\
  ret_;						\
}))

struct section_metadata
{
	Elf64_Half num_sections;
	Elf64_Shdr *sections;
	Elf64_Addr *addrs;
	Elf64_Addr *vaddrs;
	Elf64_Half section_entsize;
	Elf64_Shdr *symtab;
	const char *strtab;
};

struct efi_image_layout
{
	size_t exec_size;
	size_t kernel_size;
	size_t bss_size;
	size_t sbat_size;
	uint64_t start_address;
	void *reloc_section;
	size_t reloc_size;
	size_t align;
	uint32_t tramp_off;
	uint32_t got_off;
	uint32_t got_size;
	uint32_t bss_start;
	uint32_t end;
};

struct fixup_block_list
{
	struct fixup_block_list *next;
	int state;
	struct pe32_fixup_block b;
};

struct translate_context
{
	struct fixup_block_list *lst, *lst0;
	Elf64_Addr current_address;
};

static int is_text_section(Elf64_Shdr *s)
{
	return (s->sh_flags & (SHF_EXECINSTR | SHF_ALLOC)) ==
		(SHF_EXECINSTR | SHF_ALLOC);
}

static int is_data_section(Elf64_Shdr *s)
{
	return ((s->sh_flags & (SHF_EXECINSTR | SHF_ALLOC)) ==
		SHF_ALLOC) && s->sh_type != SHT_NOBITS;
}

static int is_bss_section(Elf64_Shdr *s)
{
	return ((s->sh_flags & (SHF_EXECINSTR | SHF_ALLOC)) ==
		SHF_ALLOC) && s->sh_type == SHT_NOBITS;
}

static int is_kept_section(Elf64_Shdr *s)
{
	return is_text_section(s) || is_data_section(s) ||
		is_bss_section(s);
}

static int is_kept_reloc_section (Elf64_Shdr *s, struct section_metadata *smd)
{
	int i;
	const char *name = smd->strtab + s->sh_name;

	if (!strncmp(name, ".rela.", 6))
		name += 5;
	else if (!strncmp(name, ".rel.", 5))
		name += 4;
	else
		return 1;

	for (i = 0, s = smd->sections; i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		const char *sname = smd->strtab + s->sh_name;
		if (strcmp (sname, name))
			continue;
		
		return is_kept_section(s);
	}

	return 0;
}

static Elf64_Addr *
get_target_address(Elf64_Ehdr *e, Elf64_Shdr *s, Elf64_Addr offset)
{
	return (Elf64_Addr *) ((char *) e + s->sh_offset + offset);
}

static Elf64_Addr get_symbol_address(Elf64_Ehdr *e, Elf64_Shdr *s, Elf64_Word i)
{
	Elf64_Sym *sym =
		(Elf64_Sym *) ((char *) e + s->sh_offset + i * s->sh_entsize);

	return sym->st_value;
}

static Elf64_Addr
put_section(Elf64_Shdr *s, int i, Elf64_Addr current_address,
	struct section_metadata *smd)
{
	Elf64_Xword align = s->sh_addralign;
	const char *name = smd->strtab + s->sh_name;

	if (align) {
		current_address = ALIGN_UP(current_address +
			VADDR_OFFSET, align) - VADDR_OFFSET;
	}
	fprintf(stdout, "Locating the section %s at 0x%llx\n", name,
		(unsigned long long) current_address);

	smd->addrs[i] = current_address;
	return current_address + s->sh_size;
}

static void
locate_sections(Elf64_Ehdr *e, const char *elf_path,
	struct section_metadata *smd, struct efi_image_layout *layout)
{
	int i;
	Elf64_Shdr *s;

	layout->align = 1;
	layout->kernel_size = 0;

	/* .text */
	for (i = 0, s = smd->sections; i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		if (is_text_section(s)) {
			layout->kernel_size = put_section(s, i,
				layout->kernel_size, smd);
		}
	}
	layout->kernel_size = ALIGN_UP(layout->kernel_size + VADDR_OFFSET,
		PE32_SECTION_ALIGNMENT) - VADDR_OFFSET;
	layout->exec_size = layout->kernel_size;

	/* .data */
	for (i = 0, s = smd->sections; i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		if (is_data_section(s)) {
			layout->kernel_size = put_section(s, i,
				layout->kernel_size, smd);
		}
	}
	layout->bss_start = layout->kernel_size;
	layout->end = layout->kernel_size;

	/* .bss */
	for (i = 0, s = smd->sections; i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		if (is_bss_section(s)) {
			layout->end = put_section(s, i,
				layout->end, smd);
		}
		smd->vaddrs[i] = smd->addrs[i] + VADDR_OFFSET;
	}
	layout->end = ALIGN_UP(layout->end + VADDR_OFFSET,
		PE32_SECTION_ALIGNMENT) - VADDR_OFFSET;
	layout->kernel_size = layout->end;
}

static Elf64_Addr
relocate_symbols(Elf64_Ehdr *e, struct section_metadata *smd,
		 Elf64_Addr bss_start, Elf64_Addr end)
{
	Elf64_Word symtab_size, sym_size, num_syms;
	Elf64_Off symtab_offset;
	Elf64_Addr start_address = (Elf64_Addr) -1;
	Elf64_Sym *sym;
	Elf64_Word i;

	Elf64_Shdr *symstr_section;
	const char *symstr;

	symstr_section = (Elf64_Shdr *) ((char *) smd->sections +
		smd->symtab->sh_link * smd->section_entsize);
	symstr = (char *) e + symstr_section->sh_offset;

	symtab_size = smd->symtab->sh_size;
	sym_size = smd->symtab->sh_entsize;
	symtab_offset = smd->symtab->sh_offset;
	num_syms = symtab_size / sym_size;

	for (i = 0, sym = (Elf64_Sym *) ((char *) e + symtab_offset);
	     i < num_syms;
	     i++, sym = (Elf64_Sym *) ((char *) sym + sym_size)) {
		Elf64_Section cur_index;
		const char *name;

		cur_index = sym->st_shndx;
		name = symstr + sym->st_name;

		if (cur_index == SHN_ABS) {
			continue;
		} else if (cur_index == SHN_UNDEF) {
			if (sym->st_name && strcmp(name, "__bss_start") == 0)
				sym->st_value = bss_start;
			else if (sym->st_name && strcmp(name, "_end") == 0)
				sym->st_value = end;
			else if (sym->st_name) {
				fprintf(stderr, "Undefined symbol: %s\n", name);
				exit(1);
			}
			else
				continue;
		} else if (cur_index >= smd->num_sections) {
			fprintf(stderr, "Section %d does not exist\n", cur_index);
		} else 
			sym->st_value += smd->vaddrs[cur_index];

		fprintf(stdout, "Locating %s at 0x%llx (0x%llx)\n", name,
			(unsigned long long) sym->st_value,
			(unsigned long long) smd->vaddrs[cur_index]);
		
		if (start_address == (Elf64_Addr) -1) {
			if (strcmp(name, "_start") == 0 || strcmp(name, "start") == 0)
				start_address = sym->st_value;
		}
	}

	return start_address;
}

static void
relocate_addrs(Elf64_Ehdr *e, struct section_metadata *smd,
		 char *pe_target, Elf64_Addr tramp_off, Elf64_Addr got_off)
{
	Elf64_Half i;
	Elf64_Shdr *s;

	for (i = 0, s = smd->sections;
	     i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		if (s->sh_type != SHT_REL && s->sh_type != SHT_RELA)
			continue;
		
		Elf64_Rela *r;
		Elf64_Word rtab_size, r_size, num_rs;
		Elf64_Off rtab_offset;
		Elf64_Word target_section_index;
		Elf64_Addr target_section_addr;
		Elf64_Shdr *target_section;
		Elf64_Word j;

		if (!is_kept_section(s) && !is_kept_reloc_section(s, smd)) {
			fprintf(stdout, "Not translating relocations for omitted section %s\n",
				smd->strtab + s->sh_name);
			continue;
		}

		target_section_index = s->sh_info;
		target_section_addr = smd->addrs[target_section_index];
		target_section = (Elf64_Shdr *) ((char *) smd->sections
			+ (target_section_index * smd->section_entsize));
		fprintf(stdout, "Dealing with the relocation section %s for %s\n",
			smd->strtab + s->sh_name, smd->strtab + target_section->sh_name);
		
		rtab_size = s->sh_size;
		r_size = s->sh_entsize;
		rtab_offset = s->sh_offset;
		num_rs = rtab_size / r_size;

		for (j = 0, r = (Elf64_Rela *) ((char *)e + rtab_offset);
		     j < num_rs;
		     j++, r = (Elf64_Rela *) ((char *) r + r_size)) {
			Elf64_Addr info;
			Elf64_Addr offset;
			Elf64_Addr sym_addr;
			Elf64_Addr *target;
			Elf64_Addr addend;

			offset = r->r_offset;
			target = get_target_address(e, target_section, offset);
			info = r->r_info;
			sym_addr = get_symbol_address(e, smd->symtab, ELF64_R_SYM(info));
			addend = (s->sh_type == SHT_RELA) ? r->r_addend : 0;

			switch (ELF64_R_TYPE(info)) {
			case R_X86_64_NONE:
				break;
			
			case R_X86_64_64:
				*target += addend + sym_addr;
				fprintf(stdout, "Relocating an R_X86_64_64 entry to 0x%llx"
					" at the offset 0x%llx\n",
					(unsigned long long) *target,
					(unsigned long long) offset);
				break;

			case R_X86_64_PC32:
			case R_X86_64_PLT32: {
		    		uint32_t *t32 = (uint32_t *) target;
		    		*t32 = *t32 + addend + sym_addr - target_section_addr -
				       offset - VADDR_OFFSET;
		    		fprintf(stdout, "Relocating an R_X86_64_PC32 entry to 0x%x at the offset 0x%llx\n",
					*t32, (unsigned long long) offset);
		    		break;
		  	}

			case R_X86_64_PC64: {
		    		*target = *target + addend + sym_addr
					  - target_section_addr - offset
					  - VADDR_OFFSET;
		    		fprintf(stdout, "Relocating an R_X86_64_PC64 entry to 0x%llx"
					" at the offset 0x%llx\n",
					(unsigned long long) *target,
					(unsigned long long) offset);
		    		break;
			}

			case R_X86_64_32:
			case R_X86_64_32S: {
		    		uint32_t *t32 = (uint32_t *) target;
		    		*t32 = *t32 + addend + sym_addr;
		    		fprintf(stdout, "Relocating an R_X86_64_32(S) entry to 0x%x at the offset 0x%llx\n",
					*t32, (unsigned long long) offset);
		    		break;
		  	}

			default:
		  		fprintf(stderr, "Relocation 0x%x is not implemented yet\n",
					(unsigned int) ELF64_R_TYPE(info));
		  		break;
			}
		}
	}
}

static void
translate_reloc_start(struct translate_context *ctx)
{
	memset(ctx, 0, sizeof (*ctx));
	ctx->lst = ctx->lst0 = malloc(sizeof (*ctx->lst) + 2 * 0x1000);
	if (!ctx->lst) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(ctx->lst, 0, sizeof (*ctx->lst) + 2 * 0x1000);
	ctx->current_address = 0;
}

static Elf64_Addr
add_fixup_entry(struct fixup_block_list **cblock, uint16_t type,
		Elf64_Addr addr, int flush, Elf64_Addr current_address)
{
	struct pe32_fixup_block *b;

	b = &((*cblock)->b);
	if ((*cblock)->state) {
		if (flush || addr < b->page_rva || b->page_rva + 0x1000 <= addr) {
	  		uint32_t size;
			if (flush) {
				Elf64_Addr next_address;
				unsigned padding_size;
        			size_t cur_index;

				next_address = current_address + b->block_size;
				padding_size = ((ALIGN_UP(next_address, PE32_SECTION_ALIGNMENT) -
						 next_address) >> 1);
              			cur_index = ((b->block_size - sizeof (*b)) >> 1);
              			fprintf(stdout, "Adding %d padding fixup entries\n", padding_size);
	      			while (padding_size--) {
					b->entries[cur_index++] = 0;
					b->block_size += 2;
				}
	    		} else while (b->block_size & (8 - 1)) {
        			size_t cur_index;
				fprintf(stdout, "Adding a padding fixup entry\n");
        			cur_index = ((b->block_size - sizeof (*b)) >> 1);
        			b->entries[cur_index] = 0;
        			b->block_size += 2;
        		}

			fprintf(stdout, "Writing %d bytes of a fixup block starting at 0x%x\n",
                        	b->block_size, b->page_rva);
        		size = b->block_size;
	  		current_address += size;
	  		b->page_rva = b->page_rva;
	  		b->block_size = b->block_size;
	  		(*cblock)->next = malloc(sizeof (**cblock) + 2 * 0x1000);
			if (!(*cblock)->next) {
				fprintf(stderr, "Out of memory\n");
				exit(1);
			}
	  		memset((*cblock)->next, 0, sizeof (**cblock) + 2 * 0x1000);
	  		*cblock = (*cblock)->next;
		}
    	}

	b = &((*cblock)->b);

	if (!flush) {
		uint16_t entry;
		size_t cur_index;

		if (!(*cblock)->state) {
			(*cblock)->state = 1;
			b->page_rva = (addr & ~(0x1000 - 1));
			b->block_size = sizeof (*b);
		}

		if (b->block_size >= sizeof (*b) + 2 * 0x1000) {
			fprintf(stderr, "Too many fixup entries\n");
		}

		cur_index = ((b->block_size - sizeof (*b)) >> 1);
		entry = PE32_FIXUP_ENTRY(type, addr - b->page_rva);
		b->entries[cur_index] = entry;
		b->block_size += 2;
    	}

  return current_address;
}

static void
translate_relocation(struct translate_context *ctx, Elf64_Addr addr, Elf64_Addr info)
{
	if (ELF64_R_TYPE(info) == R_X86_64_32 ||
	    ELF64_R_TYPE(info) == R_X86_64_32S) {
		fprintf(stderr, "Can\'t add fixup entry for R_X86_64_32(S)\n");
		exit(1);
	} else if (ELF64_R_TYPE(info) == R_X86_64_64) {
		fprintf(stdout, "adding a relocation entry for 0x%llx\n",
			(unsigned long long) addr);
		ctx->current_address =
			add_fixup_entry(&ctx->lst,
					PE32_REL_BASED_DIR64,
					addr,
					0,
					ctx->current_address);
	}
}

static void
finish_reloc_translation(struct translate_context *ctx,
			 struct efi_image_layout *layout)
{
	ctx->current_address = add_fixup_entry(&ctx->lst, 0, 0, 1,
		ctx->current_address);
	{
		uint8_t *ptr;
		layout->reloc_section = ptr = malloc(ctx->current_address);
		if (!ptr) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
    		for (ctx->lst = ctx->lst0; ctx->lst; ctx->lst = ctx->lst->next)
			if (ctx->lst->state) {
				memcpy(ptr, &ctx->lst->b, ctx->lst->b.block_size);
				ptr += ctx->lst->b.block_size;
			}
    		assert((ctx->current_address + (uint8_t *) layout->reloc_section) == ptr);
	}

	for (ctx->lst = ctx->lst0; ctx->lst; ) {
		struct fixup_block_list *next;
		next = ctx->lst->next;
		free (ctx->lst);
		ctx->lst = next;
	}

	layout->reloc_size = ctx->current_address;
}

static void
make_reloc_section(Elf64_Ehdr *e, struct efi_image_layout *layout,
		   struct section_metadata *smd)
{
	unsigned i;
	Elf64_Shdr *s;
	struct translate_context ctx;

	translate_reloc_start (&ctx);

	for (i = 0, s = smd->sections; i < smd->num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd->section_entsize)) {
		if ((s->sh_type != SHT_REL) && (s->sh_type != SHT_RELA))
			continue;
		Elf64_Rel *r;
		Elf64_Word rtab_size, r_size, num_rs;
		Elf64_Off rtab_offset;
		Elf64_Addr section_address;
		Elf64_Word j;

		if (!is_kept_reloc_section(s, smd)) {
			fprintf(stdout, "Not translating the skipped relocation section %s\n",
				smd->strtab + s->sh_name);
			continue;
		}

		fprintf(stdout, "Translating the relocation section %s\n",
			smd->strtab + s->sh_name);

		rtab_size = s->sh_size;
		r_size = s->sh_entsize;
		rtab_offset = s->sh_offset;
		num_rs = rtab_size / r_size;

		section_address = smd->vaddrs[s->sh_info];

		for (j = 0, r = (Elf64_Rel *) ((char *) e + rtab_offset);
		     j < num_rs;
		     j++, r = (Elf64_Rel *) ((char *) r + r_size)) {
			Elf64_Addr info;
			Elf64_Addr offset;
			Elf64_Addr addr;

			offset = r->r_offset;
	    		info = r->r_info;

			addr = section_address + offset;

			translate_relocation(&ctx, addr, info);
		}
	}

	finish_reloc_translation (&ctx, layout);
}

static struct pe32_section_table *
init_pe_section(struct pe32_section_table *section,
		const char * const name,
		uint32_t *vma, uint32_t vsz, uint32_t valign,
		uint32_t *rda, uint32_t rsz,
		uint32_t characteristics)
{
	size_t len = strlen(name);

	if (len > sizeof (section->name)) {
		fprintf(stderr, "Section name %s length is bigger than %lu\n",
			name, (unsigned long) sizeof (section->name));
		exit(1);
	}

	memcpy(section->name, name, len);

	section->virtual_address = *vma;
	section->virtual_size = vsz;
	(*vma) = ALIGN_UP (*vma + vsz, valign);

	section->raw_data_offset = *rda;
	section->raw_data_size = rsz;
	(*rda) = ALIGN_UP (*rda + rsz, PE32_FILE_ALIGNMENT);

	section->characteristics = characteristics;

	return section + 1;
}

int main(int argc, char *argv[])
{
	char *src_path, *out_path;
	FILE *src_file, *out_file;
	size_t src_size, kernel_size;
	char *elf_img, *kernel_img;
	int i;

	Elf64_Ehdr *e;
	Elf64_Shdr *s;
	Elf64_Off section_offset;
	struct section_metadata smd = { 0, 0, 0, 0, 0, 0, 0 };
	struct efi_image_layout layout;

	char *pe_img, *header;
	struct pe32_section_table *section;
	size_t n_sections = 4;
	size_t scn_size;
	uint32_t vma, raw_data;
	size_t pe_size, header_size;
	struct pe32_coff_header *c;
	static const uint8_t stub[] = PE32_MSDOS_STUB;
	struct pe32_optional_header *o32 = NULL;
	struct pe64_optional_header *o64 = NULL;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s  <input-elf> <output-efi>\n",
			argv[0]);
		return 1;
	}

	src_path = argv[1];
	src_file = fopen(src_path, "rb");
	if (src_file == NULL) {
		fprintf(stderr, "Unable to open file: %s, error number: %d\n",
			src_path, errno);
		return 1;
	}

	off_t _src_size;
	fseeko(src_file, 0, SEEK_END);
	_src_size = ftello(src_file);
	src_size = (size_t) _src_size;
	if (src_size != _src_size) {
		fprintf(stderr, "Input elf file too large: %s\n", src_path);
		return 1;
	}

	elf_img = malloc(src_size);
	if (!elf_img) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	fseeko(src_file, 0, SEEK_SET);
	if (fread(elf_img, 1, src_size, src_file) != src_size) {
		fprintf(stderr, "Cannot read %s, error number: %d,"
			" file size: %lu\n",
			src_path, errno, src_size);
		return 1;
	}
	e = (Elf64_Ehdr *) elf_img;

	section_offset = e->e_shoff;
	smd.section_entsize = e->e_shentsize;
	smd.num_sections = e->e_shnum;

	if (src_size < section_offset +
		smd.section_entsize * smd.num_sections) {
		fprintf(stderr, "Premature end of file: %s\n", src_path);
		return 1;
	}

	smd.sections = (Elf64_Shdr *) (elf_img + section_offset);
	s = (Elf64_Shdr *) ((char *) smd.sections +
		e->e_shstrndx * smd.section_entsize);
	smd.strtab = (char *) e + s->sh_offset;

	smd.addrs = calloc(smd.num_sections, sizeof (*smd.addrs));
	smd.vaddrs = calloc(smd.num_sections, sizeof (*smd.vaddrs));
	if (!smd.addrs || !smd.vaddrs) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	memset(&layout, 0, sizeof (layout));
	layout.start_address = 0;
	locate_sections(e, src_path, &smd, &layout);
	layout.bss_size = 0;

	smd.symtab = NULL;
	for (i = 0, s = smd.sections; i < smd.num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd.section_entsize)) {
		if (s->sh_type == SHT_SYMTAB) {
			smd.symtab = s;
			break;
		}
	}
	if (!smd.symtab) {
		fprintf(stderr, "No symbol table\n");
		return 1;
	}

	layout.kernel_size = ALIGN_UP(layout.kernel_size, PE32_FILE_ALIGNMENT);
	kernel_img = malloc(layout.kernel_size);
	if (!kernel_img) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}
	memset(kernel_img, 0, layout.kernel_size);

	layout.start_address = relocate_symbols(e, &smd, layout.bss_start, layout.end);
	if (layout.start_address == (Elf64_Addr) -1) {
		fprintf(stderr, "Start symbol is not defined\n");
		return 1;
	}

	relocate_addrs(e, &smd, kernel_img, layout.tramp_off, layout.got_off);
	make_reloc_section(e, &layout, &smd);

	for (i = 0, s = smd.sections; i < smd.num_sections;
	     i++, s = (Elf64_Shdr *) ((char *) s + smd.section_entsize)) {
		if (is_kept_section(s)) {
			if (s->sh_type == SHT_NOBITS)
				memset(kernel_img + smd.addrs[i], 0, s->sh_size);
			else
				memcpy(kernel_img + smd.addrs[i], elf_img + s->sh_offset,
				       s->sh_size);
		}
	}

	kernel_size = layout.kernel_size;
	fprintf(stdout, "kernel_img=%p, kernel_size=0x%llx\n",
		kernel_img,
		(unsigned long long) kernel_size);

	/* Prepend the pe header */
	header_size = EFI64_HEADER_SIZE;
	vma = raw_data = header_size;
	pe_size = ALIGN_UP(header_size + kernel_size, PE32_FILE_ALIGNMENT) +
        	  ALIGN_UP(layout.reloc_size, PE32_FILE_ALIGNMENT);
	header = pe_img = calloc(1, pe_size);
	if (!pe_img) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	memcpy(pe_img + raw_data, kernel_img, kernel_size);
	memcpy(header, stub, PE32_MSDOS_STUB_SIZE);
	memcpy(header + PE32_MSDOS_STUB_SIZE, "PE\0\0",
		PE32_SIGNATURE_SIZE);

	c = (struct pe32_coff_header *) (header + PE32_MSDOS_STUB_SIZE
		+ PE32_SIGNATURE_SIZE);
	c->machine = PE32_MACHINE_X86_64;

	c->num_sections = n_sections;
	c->time = STABLE_EMBEDDING_TIMESTAMP;
	c->characteristics = PE32_EXECUTABLE_IMAGE
		| PE32_LINE_NUMS_STRIPPED
		| PE32_LOCAL_SYMS_STRIPPED
		| PE32_DEBUG_STRIPPED;
	
	c->optional_header_size = sizeof (struct pe64_optional_header);
	o64 = (struct pe64_optional_header *)
		(header + PE32_MSDOS_STUB_SIZE + PE32_SIGNATURE_SIZE +
                	sizeof (struct pe32_coff_header));
	o64->magic = PE32_PE64_MAGIC;

	section = (struct pe32_section_table *)(o64 + 1);
	
	PE_OHDR(o32, o64, header_size) = header_size;
	PE_OHDR(o32, o64, entry_addr) = layout.start_address;
	PE_OHDR(o32, o64, image_base) = 0;
	PE_OHDR(o32, o64, image_size) = pe_size;
	PE_OHDR(o32, o64, section_alignment) = PE32_SECTION_ALIGNMENT;
	PE_OHDR(o32, o64, file_alignment) = PE32_FILE_ALIGNMENT;
	PE_OHDR(o32, o64, subsystem) = PE32_SUBSYSTEM_EFI_APPLICATION;

	PE_OHDR(o32, o64, stack_reserve_size) = 0x10000;
	PE_OHDR(o32, o64, stack_commit_size) = 0x10000;
	PE_OHDR(o32, o64, heap_reserve_size) = 0x10000;
	PE_OHDR(o32, o64, heap_commit_size) = 0x10000;

	PE_OHDR(o32, o64, num_data_directories) = PE32_NUM_DATA_DIRECTORIES;

	PE_OHDR(o32, o64, code_base) = vma;
	PE_OHDR(o32, o64, code_size) = layout.exec_size;

	section = init_pe_section(section, ".text",
				  &vma, layout.exec_size,
				  PE32_SECTION_ALIGNMENT,
				  &raw_data, layout.exec_size,
				  PE32_SCN_CNT_CODE |
				  PE32_SCN_MEM_EXECUTE |
				  PE32_SCN_MEM_READ);

	scn_size = ALIGN_UP(layout.kernel_size - layout.exec_size, PE32_FILE_ALIGNMENT);
	PE_OHDR(o32, o64, data_size) = scn_size;

	section = init_pe_section(section, ".data",
				  &vma, scn_size, PE32_SECTION_ALIGNMENT,
				  &raw_data, scn_size,
				  PE32_SCN_CNT_INITIALIZED_DATA |
				  PE32_SCN_MEM_READ |
				  PE32_SCN_MEM_WRITE);

	scn_size = pe_size - layout.reloc_size - raw_data;
	section = init_pe_section(section, "mods",
				  &vma, scn_size, PE32_SECTION_ALIGNMENT,
				  &raw_data, scn_size,
				  PE32_SCN_CNT_INITIALIZED_DATA |
				  PE32_SCN_MEM_READ |
				  PE32_SCN_MEM_WRITE);

	scn_size = layout.reloc_size;
	PE_OHDR(o32, o64, base_relocation_table.rva) = vma;
	PE_OHDR(o32, o64, base_relocation_table.size) = scn_size;

	memcpy (pe_img + raw_data, layout.reloc_section, scn_size);
	init_pe_section (section, ".reloc",
			 &vma, scn_size, PE32_SECTION_ALIGNMENT,
			 &raw_data, scn_size,
			 PE32_SCN_CNT_INITIALIZED_DATA |
			 PE32_SCN_MEM_DISCARDABLE |
			 PE32_SCN_MEM_READ);
	
	/* Write the efi image out */
	out_path = argv[2];
	out_file = fopen(out_path, "wb");
	if (!out_file) {
		fprintf(stderr, "Cannot open %s, error number: %d\n",
			out_path, errno);
		return 1;
	}

	fprintf(stdout, "Writing 0x%llx bytes\n", (unsigned long long) pe_size);
	if (fwrite(pe_img, 1, pe_size, out_file) != pe_size) {
		fprintf(stderr, "Cannot write to %s, error number: %d\n",
			out_path, errno);
		return 1;
	}

	free(elf_img);
	free(kernel_img);
	free(smd.addrs);
	free(smd.vaddrs);
	smd.addrs = NULL;
	smd.vaddrs = NULL;
	free(layout.reloc_section);
	free(pe_img);

	return 0;
}
