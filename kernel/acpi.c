#include <acpi.h>

extern uint64_t __acpi_version;
extern uint64_t __acpi_table_rsdp;

uint64_t get_acpi_version()
{
	return __acpi_version;
}

acpi_table_rsdp *
get_acpi_table_rsdp()
{
	return (acpi_table_rsdp *) __acpi_table_rsdp;
}
