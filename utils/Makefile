
CFLAGS += -I../include

all: elf2efi get_psf_array

elf2efi: pe32.h elf2efi.c
	$(CC) $(CFLAGS) -o elf2efi elf2efi.c

get_psf_array: get_psf_array.c
	$(CC) $(CFLAGS) -o get_psf_array get_psf_array.c

.PHONY: clean all
clean:
	$(RM) elf2efi get_psf_array
