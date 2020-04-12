BINOBJS =\
	bins/nokia5110_animation.bin.o\
 	bins/atmega.bin.o\
 	bins/font.bin.o

.SECONDARY: $(BINOBJS)

%.bin.o: %.bin
	$(OBJCOPY) \
		-B aarch64 \
		-I binary \
		-O elf64-littleaarch64 \
		--section-alignment=128 \
		--rename-section .data=.data.$(notdir $<) $< $@

.PHONY: firmware/atmega8a/atmega8a.bin
firmware/atmega8a/atmega8a.bin:
	make -C firmware/atmega8a

bins/atmega.bin: firmware/atmega8a/atmega8a.bin
	cp -fv $? $@

OBJS_BINS = bins/bins.o
