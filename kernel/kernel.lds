OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(startup)

SECTIONS
{
	. = 0;
	. = ALIGN(1 << 12);
	.text : {
		_text = .;
		KEEP(*(.text))
		KEEP(*(.text.*))
		_etext = .;
	}
	.data : {
		_data = .;
		KEEP(*(.data))
		KEEP(*(.data.*))
		_edata = .;
	}
	. = ALIGN(1 << 6);
	.bss : {
		_bss = .;
		KEEP(*(.bss))
		KEEP(*(.bss.*))
		_ebss = .;
	}
	. = ALIGN(1 << 12);
	_end = .;

	.comment 0 : { *(.comment) }
	.symtab 0 : { *(.symtab) }
	.strtab 0 : { *(.strtab) }
	.shstrtab 0 : { *(.shstrtab) }

	/DISCARD/ : {
		*(.dynamic) *(.dynsym) *(.dynstr) *(.dynbss)
		*(.hash) *(.gnu.hash)
		*(.note.*)
	}
}
