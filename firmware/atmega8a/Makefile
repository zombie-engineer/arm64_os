CFLAGS := -c -mmcu=atmega8 -OS
CC := avr-gcc
LD := avr-ld
OBJCOPY := avr-objcopy
OBJDUMP := avr-objdump

dump_listing = $(OBJDUMP) -d $(1) > $(1:.o=.text)
# dump_text = echo $(1) $(1:.o=.text)

OBJS := \
	uart_routines.o\
	timer.o\
	start.o\
	ext_int.o\
	adc.o\
	twi.o\
	spi.o\
	delay.o\
	main.o

$(info $(OBJS))

get_checksum = cat $(1:.bin=.map) | grep checksum | awk '{print $$1}'

atmega8a.bin: atmega8a.o
	$(OBJCOPY) --only-section .text -O binary $^ $@
#	python patch_checksum.py $@ $$($(call get_checksum, $@))

#atmega8a.o: $(OBJS)
#	$(LD) -T link.lds -Map $(@:.o=.map) -verbose=10 -nostdlib -nostartfiles $^ -o $@
#	$(call dump_listing,$@)

atmega8a.o: main.c twi.S delay.S spi.S
	avr-gcc -O2 -mmcu=atmega8a $? -o $@
	# $(LD) -Map $(@:.o=.map) -verbose=10 -nostdlib -nostartfiles $^ -o $@
	$(call dump_listing,$@)

%.o: %.S
	$(CC) $(CFLAGS) $^ -o $@
	$(call dump_listing,$@)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
	$(call dump_listing,$@)

.PHONY: clean at emu
clean:
	rm -fv atmega8a.o $(OBJS) $(OBJS:.o=.text)

at:
	avr-gdb atmega8a.o -x rungdb.gdb

emu:
	simulavr -D -g -d atmega8 atmega8a.bin
