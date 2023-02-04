
obj-y += font_uni2_bold_18_10.o
obj-y += font_uni2_bold_32_16.o
obj-y += font_lat2_bold_32_16.o

$(src)/font_uni2_bold_18_10.o: $(src)/font_uni2_bold_18_10.psf
	cd $(src) && \
	$(OBJCOPY) -O elf64-x86-64 -B i386 -I binary \
	font_uni2_bold_18_10.psf font_uni2_bold_18_10.o

$(src)/font_uni2_bold_32_16.o: $(src)/font_uni2_bold_32_16.psf
	cd $(src) && \
	$(OBJCOPY) -O elf64-x86-64 -B i386 -I binary \
	font_uni2_bold_32_16.psf font_uni2_bold_32_16.o

$(src)/font_lat2_bold_32_16.o: $(src)/font_lat2_bold_32_16.psf
	cd $(src) && \
	$(OBJCOPY) -O elf64-x86-64 -B i386 -I binary \
	font_lat2_bold_32_16.psf font_lat2_bold_32_16.o
