
objects = font_uni2_bold_18_10.o font_uni2_bold_32_16.o font_lat2_bold_32_16.o

all: $(objects)

font_uni2_bold_18_10.o: font_uni2_bold_18_10.psf
	objcopy -O elf64-x86-64 -B i386 -I binary font_uni2_bold_18_10.psf font_uni2_bold_18_10.o

font_uni2_bold_32_16.o: font_uni2_bold_32_16.psf
	objcopy -O elf64-x86-64 -B i386 -I binary font_uni2_bold_32_16.psf font_uni2_bold_32_16.o

font_lat2_bold_32_16.o: font_lat2_bold_32_16.psf
	objcopy -O elf64-x86-64 -B i386 -I binary font_lat2_bold_32_16.psf font_lat2_bold_32_16.o

clean:
	$(RM) *.o
