
#include <stdint.h>
#include <console.h>
#include <ioop.h>
#include <framebuffer.h>

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

	for (int i = 0; i < 3600; ++i) {
		con_puts("line: ");
		con_puts(i2a(i + 1));
		con_puts("\n");
	}

	cpu_relax();
}
