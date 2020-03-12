CFLAGS := -c -O2
CC := avr-gcc
LD := avr-ld
OBJCOPY := avr-objcopy

OBJS := \
	uart_routines.o\
	timer.o\
	start.o

$(info $(OBJS))

atmega8a.bin: atmega8a.o
	$(OBJCOPY) --only-section .text -O binary $^ $@

atmega8a.o: $(OBJS)
	$(LD) -T link.lds -Map $(@:.o=.map) -verbose=10 -nostdlib -nostartfiles $^ -o $@

%.o: %.S
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -fv start.o atmega8a.o
