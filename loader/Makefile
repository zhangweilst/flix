
CFLAGS += -ffreestanding -nostdlib -mcmodel=large -fshort-wchar \
	-fno-PIE -fno-pie -no-pie -I../include

objects = startup.o main.o call_wrapper.o

startup.img: startup.exec
	strip -R .rel.dyn -R .reginfo -R .note -R .comment -R .drectve \
		-R .note.gnu.gold-version --strip-unneeded -K start \
		-o startup.img startup.exec

startup.exec: $(objects)
	$(CC) $(CFLAGS) -Wl,--build-id=none -Wl,-N -Wl,-r \
		-o startup.exec $(objects)

startup.o: startup.s
	$(AS) -o startup.o startup.s

main.o: ../include/efi/efi.h

call_wrapper.o: call_wrapper.s
	$(AS) -o call_wrapper.o call_wrapper.s

.PHONY: clean
clean:
	find . -name "*.o" | xargs $(RM)
	find . -name "*.img" | xargs $(RM)
	find . -name "*.exec" | xargs $(RM)
