
export CC := x86_64-elf-gcc
export KBUILD_CFLAGS := -O2 -Iinclude/
export OBJCOPY := objcopy

MAKEFLAGS += --no-print-directory

include scripts/Kbuild.include

PHONY := all
all: loader.img kernel.img

build-dir	:= .

KCFLAGS := $(KBUILD_CFLAGS) -ffreestanding -nostdlib -fPIE
KCFLAGS += -fno-asynchronous-unwind-tables

KLDFLAGS := -Wl,--build-id=none -Wl,-N -Wl,--no-dynamic-linker
KLDFLAGS += -Wl,-pie -Wl,-melf_x86_64 -Wl,-znoexecstack
KLDFLAGS += -Wl,--no-ld-generated-unwind-info

loader.img: $(build-dir)
	./utils/elf2efi loader/startup.img loader.img >/dev/null

kernel.img: $(build-dir) kernel/kernel.lds
	$(CC) $(KCFLAGS) $(KLDFLAGS) -T kernel/kernel.lds -o kernel.img \
	-Wl,-whole-archive built-in.a -Wl,-no-whole-archive -lgcc

PHONY += $(build-dir)
$(build-dir):
	$(MAKE) $(build)=$@ need-builtin=1

PHONY += install
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

PHONY += run
run:
	sleep 0.2s
	qemu-system-x86_64 -m 4096 -smp cpus=1 -bios /usr/share/ovmf/OVMF.fd \
		-daemonize -enable-kvm /data1/develop/qemu/flix_x86_64.img

PHONY += clean
clean:
	find . -name "built-in.a" | xargs $(RM)
	find . -name "*.o" | xargs $(RM)
	find . -name "*.cmd" | xargs $(RM)
	find . -name "*.o.d" | xargs $(RM)
	$(RM) kernel.img loader.img
	$(RM) loader/startup.exec loader/startup.img
	$(RM) utils/elf2efi utils/get_psf_array

.PHONY: $(PHONY)
