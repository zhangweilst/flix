
CFLAGS += -ffreestanding -nostdlib -fPIE -fno-asynchronous-unwind-tables \
	-I../include
SUBDIRS := fonts
SUBCLEAN := $(addsuffix .clean,$(SUBDIRS))

objects := console.o framebuffer.o

all: $(objects) subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

console.o: ../include/framebuffer.h

framebuffer.o:

clean: $(SUBCLEAN)
	$(RM) *.o

$(SUBCLEAN): %.clean:
	$(MAKE) -C $* clean

.PHONY: clean $(SUBCLEAN) subdirs $(SUBDIRS)
