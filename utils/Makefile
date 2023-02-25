CC := gcc

obj-y += elf2efi
obj-y += get_psf_array

elf2efi: pe32.h elf2efi.c
	$(CC) $(CFLAGS) -o elf2efi elf2efi.c

get_psf_array: get_psf_array.c
	$(CC) $(CFLAGS) -o get_psf_array get_psf_array.c
