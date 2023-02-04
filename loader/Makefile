
ccflags-y += -ffreestanding -nostdlib -mcmodel=large -fshort-wchar \
	-fno-PIE -fno-pie -no-pie

objects := $(src)/startup.o $(src)/main.o $(src)/call_wrapper.o

obj-y += startup.img

$(src)/startup.img: $(src)/startup.exec
	strip -R .rel.dyn -R .reginfo -R .note -R .comment -R .drectve \
		-R .note.gnu.gold-version --strip-unneeded -K start \
		-o $(src)/startup.img $(src)/startup.exec

$(src)/startup.exec: $(objects)
	$(CC) $(cflags) -Wl,--build-id=none -Wl,-N -Wl,-r \
		-o $(src)/startup.exec $(objects)
