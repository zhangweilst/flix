/**
 * pe object file definitions
 */

#ifndef _PE32_H_
#define _PE32_H_

#define PE32_REL_BASED_ABSOLUTE		0
#define PE32_REL_BASED_HIGH		1
#define PE32_REL_BASED_LOW		2
#define PE32_REL_BASED_HIGHLOW		3
#define PE32_REL_BASED_HIGHADJ		4
#define PE32_REL_BASED_MIPS_JMPADDR	5
#define PE32_REL_BASED_ARM_MOV32A 	5
#define PE32_REL_BASED_RISCV_HI20	5
#define PE32_REL_BASED_SECTION		6
#define PE32_REL_BASED_REL		7
#define PE32_REL_BASED_ARM_MOV32T	7
#define PE32_REL_BASED_RISCV_LOW12I	7
#define PE32_REL_BASED_RISCV_LOW12S	8
#define PE32_REL_BASED_IA64_IMM64	9
#define PE32_REL_BASED_DIR64		10
#define PE32_REL_BASED_HIGH3ADJ		11

#define PE32_MSDOS_STUB \
{ \
	0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, \
	0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, \
	0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, \
	0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, \
	0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68, \
	0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, \
	0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f, \
	0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e, \
	0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20, \
	0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a, \
	0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  \
}

#define PE32_PE32_MAGIC			0x10b
#define PE32_PE64_MAGIC			0x20b
#define PE32_NUM_DATA_DIRECTORIES	16

#define PE32_RELOCS_STRIPPED		0x0001
#define PE32_EXECUTABLE_IMAGE		0x0002
#define PE32_LINE_NUMS_STRIPPED		0x0004
#define PE32_LOCAL_SYMS_STRIPPED	0x0008
#define PE32_AGGRESSIVE_WS_TRIM		0x0010
#define PE32_LARGE_ADDRESS_AWARE	0x0020
#define PE32_16BIT_MACHINE		0x0040
#define PE32_BYTES_REVERSED_LO		0x0080
#define PE32_32BIT_MACHINE		0x0100
#define PE32_DEBUG_STRIPPED		0x0200
#define PE32_REMOVABLE_RUN_FROM_SWAP	0x0400
#define PE32_SYSTEM			0x1000
#define PE32_DLL			0x2000
#define PE32_UP_SYSTEM_ONLY		0x4000
#define PE32_BYTES_REVERSED_HI		0x8000

#define PE32_SUBSYSTEM_EFI_APPLICATION	10

#define PE32_SCN_CNT_CODE		0x00000020
#define PE32_SCN_CNT_INITIALIZED_DATA	0x00000040
#define PE32_SCN_MEM_DISCARDABLE	0x02000000
#define PE32_SCN_MEM_EXECUTE		0x20000000
#define PE32_SCN_MEM_READ		0x40000000
#define PE32_SCN_MEM_WRITE		0x80000000

#define PE32_SCN_ALIGN_1BYTES		0x00100000
#define PE32_SCN_ALIGN_2BYTES		0x00200000
#define PE32_SCN_ALIGN_4BYTES		0x00300000
#define PE32_SCN_ALIGN_8BYTES		0x00400000
#define PE32_SCN_ALIGN_16BYTES		0x00500000
#define PE32_SCN_ALIGN_32BYTES		0x00600000
#define PE32_SCN_ALIGN_64BYTES		0x00700000

struct pe32_coff_header
{
	uint16_t machine;
	uint16_t num_sections;
	uint32_t time;
	uint32_t symtab_offset;
	uint32_t num_symbols;
	uint16_t optional_header_size;
	uint16_t characteristics;
};

struct pe32_data_directory
{
	uint32_t rva;
	uint32_t size;
};

struct pe32_optional_header
{
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t code_size;
	uint32_t data_size;
	uint32_t bss_size;
	uint32_t entry_addr;
	uint32_t code_base;

	uint32_t data_base;
	uint32_t image_base;

	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_os_version;
	uint16_t minor_os_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t header_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;

	uint32_t stack_reserve_size;
	uint32_t stack_commit_size;
	uint32_t heap_reserve_size;
	uint32_t heap_commit_size;

	uint32_t loader_flags;
	uint32_t num_data_directories;

	struct pe32_data_directory export_table;
	struct pe32_data_directory import_table;
	struct pe32_data_directory resource_table;
	struct pe32_data_directory exception_table;
	struct pe32_data_directory certificate_table;
	struct pe32_data_directory base_relocation_table;
	struct pe32_data_directory debug;
	struct pe32_data_directory architecture;
	struct pe32_data_directory global_ptr;
	struct pe32_data_directory tls_table;
	struct pe32_data_directory load_config_table;
	struct pe32_data_directory bound_import;
	struct pe32_data_directory iat;
	struct pe32_data_directory delay_import_descriptor;
	struct pe32_data_directory com_runtime_header;
	struct pe32_data_directory reserved_entry;
};

struct pe64_optional_header
{
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t code_size;
	uint32_t data_size;
	uint32_t bss_size;
	uint32_t entry_addr;
	uint32_t code_base;

	uint64_t image_base;

	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_os_version;
	uint16_t minor_os_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t reserved;
	uint32_t image_size;
	uint32_t header_size;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;

	uint64_t stack_reserve_size;
	uint64_t stack_commit_size;
	uint64_t heap_reserve_size;
	uint64_t heap_commit_size;

	uint32_t loader_flags;
	uint32_t num_data_directories;

	struct pe32_data_directory export_table;
	struct pe32_data_directory import_table;
	struct pe32_data_directory resource_table;
	struct pe32_data_directory exception_table;
	struct pe32_data_directory certificate_table;
	struct pe32_data_directory base_relocation_table;
	struct pe32_data_directory debug;
	struct pe32_data_directory architecture;
	struct pe32_data_directory global_ptr;
	struct pe32_data_directory tls_table;
	struct pe32_data_directory load_config_table;
	struct pe32_data_directory bound_import;
	struct pe32_data_directory iat;
	struct pe32_data_directory delay_import_descriptor;
	struct pe32_data_directory com_runtime_header;
	struct pe32_data_directory reserved_entry;
};

struct pe32_section_table
{
	char name[8];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t raw_data_size;
	uint32_t raw_data_offset;
	uint32_t relocations_offset;
	uint32_t line_numbers_offset;
	uint16_t num_relocations;
	uint16_t num_line_numbers;
	uint32_t characteristics;
};

struct pe32_fixup_block
{
	uint32_t page_rva;
	uint32_t block_size;
	uint16_t entries[0];
};

#endif
