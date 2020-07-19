.PHONY: uart/pl011_regs_bits.h
uart/pl011_regs_bits.h: uart/pl011_regs_spec.yaml $(REGTOOL)
	python3 $(REGTOOL) $^ > $@

OBJS_UART = uart/uart.o uart/mini_uart.o uart/pl011_uart.o uart/pl011_routine.o

$(info OBJS_UART $(OBJS_UART))

