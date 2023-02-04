
#include <stdint.h>
#include <console.h>
#include <rop.h>
#include <framebuffer.h>
#include <acpi.h>

extern void con_init();
extern void framebuffer_init();
extern void spin(uint64_t t);
extern void cpu_relax();

char *i2a(uint64_t a)
{
	static char str[64];
	int i, j, r;

	i = 0;
	do {
		r = a % 10;
		a = a / 10;
		str[i++] = r + '0';
	} while (a > 0);

	str[i] = '\0';

	for (j = 0, i--; j < i; j++, i--) {
		char tmp = str[i];
		str[i] = str[j];
		str[j] = tmp;
	}
	return str;
}

char *i2h(uint64_t a)
{
	static char str[64];
	int i, j, r;

	i = 0;
	do {
		r = a % 16;
		a = a / 16;
		str[i++] = r > 9 ? r - 10 + 'A' : r + '0';
	} while (a > 0);

	str[i] = '\0';

	for (j = 0, i--; j < i; j++, i--) {
		char tmp = str[i];
		str[i] = str[j];
		str[j] = tmp;
	}
	return str;
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t lslot = (uint32_t) slot;
	uint32_t lfunc = (uint32_t) func;
	uint16_t tmp = 0;

	address = ((lbus << 16) | (slot << 11) | (lfunc << 8) |
		(offset & 0xFC) | ((uint32_t) 0x80000000));
	outl(0xCF8, address);
	tmp = (uint16_t) ((inl(0xCFC) >> ((offset & 2) << 3)) & 0xFFFF);
	return tmp;
}

uint16_t pci_get_vendor(uint8_t bus, uint8_t slot)
{
	return pci_config_read_word(bus, slot, 0, 0);
}

void pci_enumerate()
{
	uint16_t bus, device;
	uint16_t vendor;

	for (bus = 0; bus < 256; ++bus) {
		for (device = 0; device < 32; ++device) {
			vendor = pci_get_vendor(bus, device);
			if (vendor != 0xFFFF) {
				con_puts("bus: 0x");
				con_puts(i2h(bus));
				con_puts(", device: 0x");
				con_puts(i2h(device));
				con_puts(", vendor: 0x");
				con_puts(i2h(vendor));
				con_puts("\n");
			}
		}
	}
}

void kernel_entry()
{
	framebuffer_init();
	con_init();

	con_set_color(0x000000, 0x3de1ad);
	con_clear_screen();

	con_puts("framebuffer base: 0x");
	con_puts(i2h((uint64_t) _frameb_base));
	con_puts("\n");

	uint64_t mtrr_cap = rdmsr(0xFE);
	con_puts("mtrr_cap: 0x");
	con_puts(i2h(mtrr_cap));
	con_puts("\n");

	uint64_t mtrr_def_type = rdmsr(0x2FF);
	con_puts("mtrr_def_type: 0x");
	con_puts(i2h(mtrr_def_type));
	con_puts("\n");

	uint64_t vmtrr_num = mtrr_cap & 0xFF;
	for (int i = 0; i < vmtrr_num; ++i) {
		uint64_t mtrr_phy_base = rdmsr(0x200 + 2 * i);
		uint64_t mtrr_phy_mask = rdmsr(0x200 + 2 * i + 1);
		con_puts("mtrr_phy_base ");
		con_puts(i2a(i));
		con_puts(": 0x");
		con_puts(i2h(mtrr_phy_base));
		con_puts("\n");

		con_puts("mtrr_phy_mask ");
		con_puts(i2a(i));
		con_puts(": 0x");
		con_puts(i2h(mtrr_phy_mask));
		con_puts("\n");
	}

	uint64_t cr0 = rdcr0();
	uint64_t cr4 = rdcr4();
	uint64_t cr3 = rdcr3();
	uint64_t efer = rdmsr(MSR_EFER);
	uint64_t pat = rdmsr(MSR_PAT);
	uint64_t pg = (cr0 >> 31) & 1;
	uint64_t pae = (cr4 >> 5) & 1;
	uint64_t lme = (efer >> 8) & 1;
	uint64_t la57 = (cr4 >> 12) & 1;
	uint64_t cd = (cr0 >> 30) & 1;
	uint64_t nw = (cr0 >> 29) & 1;
	uint64_t pcide = (cr4 >> 17) & 1;
	if (pg) {
		con_puts("Paging enabled: true\n");
	} else {
		con_puts("Paging enabled: false\n");
	}
	if (pg && !pae) {
		con_puts("32-bit paging: true\n");
	}
	if (pg && pae && !lme) {
		con_puts("pae paging: true\n");
	}
	if (pg && pae && lme && !la57) {
		con_puts("4-level paging: true\n");
	}
	if (pg && pae && lme && la57) {
		con_puts("5-level paging: true\n");
	}

	if (cd) {
		con_puts("Cache disabled: true\n");
	} else {
		con_puts("Cache disabled: false\n");
	}

	if (nw) {
		con_puts("Not Write-through: true\n");
	} else {
		con_puts("Not Write-through: false\n");
	}

	if (pcide) {
		con_puts("PCID enable: true\n");
	} else {
		con_puts("PCID enable: false\n");
	}

	for (int i = 0; i < 8; ++i) {
		con_puts("PAT");
		con_puts(i2a(i));
		con_puts(": 0x");
		con_puts(i2h(pat & 0x7));
		con_puts("\n");
		pat = pat >> 8;
	}

	/* MAXPHYADDRESS */
	cpuid_r cpuid_return = cpuid(0x80000008, 0);
	uint8_t maxphyaddress = cpuid_return.eax & 0xFF;
	uint8_t maxviraddress = (cpuid_return.eax >> 8) & 0xFF;
	con_puts("MAXPHYADDRESS: ");
	con_puts(i2a(maxphyaddress));
	con_puts("\n");
	con_puts("MAXVIRADDRESS: ");
	con_puts(i2a(maxviraddress));
	con_puts("\n");

	uint64_t fbbase = (uint64_t) _frameb_base;
	con_puts("CR3 PCD: ");
	con_puts(i2a((cr3 >> 4) & 1));
	con_puts("\n");
	con_puts("CR3 PWT: ");
	con_puts(i2a((cr3 >> 3) & 1));
	con_puts("\n");

	uint64_t pml4t = cr3 & (~0xFFF) & 0x0000FFFFFFFFFFFF;
	uint64_t pml4e = *((uint64_t *) pml4t + ((fbbase >> 39) & 0x1FF));
	if (!(pml4e & 0x1)) {
		con_puts("PML4E not present\n");
		goto resume;
	}
	con_puts("PML4E PCD: ");
	con_puts(i2a((pml4e >> 4) & 1));
	con_puts("\n");
	con_puts("PML4E PWT: ");
	con_puts(i2a((pml4e >> 3) & 1));
	con_puts("\n");

	uint64_t pdpt = pml4e & (~0xFFF) & 0x0000FFFFFFFFFFFF;
	uint64_t pdpte = *((uint64_t *) pdpt + ((fbbase >> 30) & 0x1FF));
	if (!(pdpte & 0x1)) {
		con_puts("PDPTE not present\n");
		goto resume;
	}
	con_puts("PDPTE PCD: ");
	con_puts(i2a((pdpte >> 4) & 1));
	con_puts("\n");
	con_puts("PDPTE PWT: ");
	con_puts(i2a((pdpte >> 3) & 1));
	con_puts("\n");
	if ((pdpte >> 7) & 0x1) {
		con_puts("PDPTE points to 1GB page\n");
		con_puts("PDPTE PAT: ");
		con_puts(i2a((pdpte >> 12) & 1));
		con_puts("\n");
		goto resume;
	}

	uint64_t pdt = pdpte & (~0xFFF) & 0x0000FFFFFFFFFFFF;
	uint64_t pde = *((uint64_t *) pdt + ((fbbase >> 21) & 0x1FF));
	if (!(pde & 0x1)) {
		con_puts("PDE not present\n");
		goto resume;
	}
	con_puts("PDE PCD: ");
	con_puts(i2a((pde >> 4) & 1));
	con_puts("\n");
	con_puts("PDE PWT: ");
	con_puts(i2a((pde >> 3) & 1));
	con_puts("\n");
	if ((pde >> 7) & 0x1) {
		con_puts("PDE points to 2MB page\n");
		con_puts("PDE PAT: ");
		con_puts(i2a((pde >> 12) & 1));
		con_puts("\n");
		goto resume;
	}

	uint64_t pt = pde & (~0xFFF) & 0x0000FFFFFFFFFFFF;
	uint64_t pte = *((uint64_t *) pt + ((fbbase >> 12) & 0x1FF));
	if (!(pte & 0x1)) {
		con_puts("PTE not present\n");
		goto resume;
	}
	con_puts("PTE PCD: ");
	con_puts(i2a((pte >> 4) & 1));
	con_puts("\n");
	con_puts("PTE PWT: ");
	con_puts(i2a((pte >> 3) & 1));
	con_puts("\n");
	con_puts("PTE PAT: ");
	con_puts(i2a((pte >> 7) & 1));
	con_puts("\n");

resume:
	cr4 = pre_mtrr_change();
	wrmsr(MSR_MTRR_DEF_TYPE, 0xC06);
	post_mtrr_change(cr4);
	mtrr_def_type = rdmsr(MSR_MTRR_DEF_TYPE);
	con_puts("mtrr_def_type: 0x");
	con_puts(i2h(mtrr_def_type));
	con_puts("\n");

	/* acpi related */
	if (get_acpi_version() == 2) {
		acpi_table_xsdt *xsdt = (acpi_table_xsdt *) (get_acpi_table_rsdp()->xsdt);
		uint32_t length = xsdt->header.length;
		uint32_t num_entries = (length - sizeof(acpi_table_header)) / 8;
		for (int i = 0; i < num_entries; ++i) {
			acpi_table_header *header = (acpi_table_header *) (xsdt->entry[i]);
			for (int j = 0; j < 4; ++j) {
				con_putchar(header->signature[j]);
			}
			con_putchar('\n');
		}
	}

	spin(50);
	for (int i = 0; i < 1024; ++i) {
		con_puts("line: ");
		con_puts(i2a(i + 1));
		con_puts("\n");
	}
	cpu_relax();
}
