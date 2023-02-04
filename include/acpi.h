#ifndef _ACPI_H
#define _ACPI_H

#include <stdint.h>

#pragma pack(1)

#define ACPI_SIG_RSDP		"RSD PTR "
#define ACPI_SIG_RSDT		"RSDT"
#define ACPI_SIG_XSDT		"XSDT"
#define ACPI_SIG_MCFG		"MCFG"

typedef struct _acpi_table_rsdp
{
	char		signature[8];
	uint8_t		checksum;
	char		oemid[6];
	uint8_t		revision;
	uint32_t	rsdt;
	uint32_t	length;
	uint64_t	xsdt;
	uint8_t		e_checksum;
	char		reserved[3];
} acpi_table_rsdp;

typedef struct _acpi_table_header
{
	char		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	char		oemid[6];
	char		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;
} acpi_table_header;

typedef struct _acpi_table_rsdt
{
	acpi_table_header header;
	uint32_t entry[1];
} acpi_table_rsdt;

typedef struct _acpi_table_xsdt
{
	acpi_table_header header;
	uint64_t entry[1];
} acpi_table_xsdt;

typedef struct _acpi_table_mcfg_allocation
{
	uint64_t	base_address;
	uint16_t	segment_group;
	uint8_t		start_bus_number;
	uint8_t		end_bus_number;
	uint32_t	reserved;
} acpi_table_mcfg_allocation;

typedef struct _acpi_table_mcfg
{
	acpi_table_header header;
	acpi_table_mcfg_allocation entry[1];
} acpi_table_mcfg;

extern uint64_t get_acpi_version();
extern acpi_table_rsdp * get_acpi_table_rsdp();

#endif
