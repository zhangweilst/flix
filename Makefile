
export CC := gcc
export CFLAGS := -O2
KCFLAGS := $(CFLAGS) -ffreestanding -nostdlib -fPIE -fno-asynchronous-unwind-tables \
	-I../include
SUBDIRS := utils loader kernel drivers
SUBCLEAN := $(addsuffix .clean,$(SUBDIRS))

all: subdirs loader.img kernel.img

loader.img: utils/elf2efi loader/startup.img
	./utils/elf2efi loader/startup.img loader.img

kobjects := kernel/boot/boot.o kernel/boot/main.o drivers/console.o \
	drivers/framebuffer.o drivers/fonts/font_uni2_bold_18_10.o \
	drivers/fonts/font_uni2_bold_32_16.o drivers/fonts/font_lat2_bold_32_16.o

kernel.img: kernel/kernel.lds $(kobjects)
	$(CC) $(KCFLAGS) -Wl,--build-id=none -Wl,-N -Wl,--no-dynamic-linker \
		-Wl,-pie -Wl,-melf_x86_64 -Wl,-znoexecstack \
		-Wl,--no-ld-generated-unwind-info \
		-T kernel/kernel.lds \
		-o kernel.img $(kobjects) \

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	sudo modprobe nbd
	sleep 0.2s
	sudo qemu-nbd -d /dev/nbd0
	sleep 0.2s
	sudo qemu-nbd -c /dev/nbd0 /data1/develop/qemu/flix_x86_64.img
	sleep 0.2s
	sudo mount /dev/nbd0p1 /mnt/flix
	sleep 0.2s
	sudo cp -f loader.img /mnt/flix/EFI/BOOT/BOOTX64.EFI
	sudo cp -f kernel.img /mnt/flix/EFI/BOOT/kernel.img
	sudo cp -f loader.img /boot/efi/EFI/flix/flix.efi
	sudo cp -f kernel.img /boot/efi/EFI/flix/kernel.img
	sudo umount /mnt/flix
	sleep 0.2s
	sudo qemu-nbd -d /dev/nbd0

run:
	sleep 0.2s
	qemu-system-x86_64 -m 4096 -smp cpus=1 -bios /usr/share/ovmf/OVMF.fd \
		-daemonize -enable-kvm /data1/develop/qemu/flix_x86_64.img

clean: $(SUBCLEAN)
	$(RM) *.img

$(SUBCLEAN): %.clean:
	$(MAKE) -C $* clean

.PHONY: all clean $(SUBCLEAN) subdirs $(SUBDIRS)
