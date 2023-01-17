
CFLAGS += -ffreestanding -nostdlib -fPIE -fno-asynchronous-unwind-tables \
	-I../include
objects := boot/boot.o boot/main.o

all: $(objects)

boot/boot.o: boot/boot.s
	$(AS) -o boot/boot.o boot/boot.s

boot/main.o: boot/main.c

.PHONY: clean
clean:
	find . -name "*.o" | xargs $(RM)
	find . -name "*.img" | xargs $(RM)
