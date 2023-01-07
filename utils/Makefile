
CFLAGS += -I../include

elf2efi: ../include/pe32.h elf2efi.c
	$(CC) $(CFLAGS) -o elf2efi elf2efi.c

.PHONY: clean
clean:
	$(RM) elf2efi
