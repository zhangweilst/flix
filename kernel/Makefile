
CFLAGS += -ffreestanding -nostdlib -fPIE -fno-asynchronous-unwind-tables
objects = boot/boot.o boot/main.o

kernel.img: kernel.lds $(objects)
	$(CC) $(CFLAGS) -Wl,--build-id=none -Wl,-N -Wl,--no-dynamic-linker \
		-Wl,-pie -Wl,-melf_x86_64 -Wl,-znoexecstack \
		-Wl,--no-ld-generated-unwind-info \
		-T kernel.lds \
		-o kernel.img $(objects)

boot/boot.o: boot/boot.s
	$(AS) -o boot/boot.o boot/boot.s

boot/main.o: boot/main.c

.PHONY: clean
clean:
	find . -name "*.o" | xargs $(RM)
	find . -name "*.img" | xargs $(RM)
