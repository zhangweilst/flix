
export CC := gcc
export CFLAGS := -O2
SUBDIRS := utils loader kernel
SUBCLEAN := $(addsuffix .clean,$(SUBDIRS))

loader.img: subdirs utils/elf2efi loader/startup.img
	./utils/elf2efi loader/startup.img loader.img

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install:
	sudo modprobe nbd
	sleep 0.2s
	sudo qemu-nbd -d /dev/nbd0
	sudo qemu-nbd -c /dev/nbd0 /data1/develop/qemu/flix_x86_64.img
	sudo mount /dev/nbd0p1 /mnt/flix
	sudo cp -f loader.img /mnt/flix/EFI/BOOT/BOOTX64.EFI
	sudo cp -f kernel/kernel.img /mnt/flix/EFI/BOOT/kernel.img
	sudo cp -f loader.img /boot/efi/EFI/flix/flix.efi
	sudo cp -f kernel/kernel.img /boot/efi/EFI/flix/kernel.img
	sudo umount /mnt/flix
	sudo qemu-nbd -d /dev/nbd0

run:
	sleep 0.2s
	qemu-system-x86_64 -m 4096 -smp cpus=1 -bios /usr/share/ovmf/OVMF.fd \
		-daemonize -enable-kvm /data1/develop/qemu/flix_x86_64.img

clean: $(SUBCLEAN)
	$(RM) loader.img

$(SUBCLEAN): %.clean:
	$(MAKE) -C $* clean

.PHONY: clean $(SUBCLEAN) subdirs $(SUBDIRS)
