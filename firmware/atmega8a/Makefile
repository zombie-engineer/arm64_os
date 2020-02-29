atmega8a.bin: _atmega8a.o
	avr-objcopy --only-section .text -O binary $^ $@

_atmega8a.o: atmega8a.S
	avr-gcc $^ -o $@

