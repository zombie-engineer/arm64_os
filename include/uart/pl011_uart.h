#pragma once
#include <uart/uart.h>

/* Init PL011 UART */
void pl011_uart_init(int baudrate, int _not_used);

void pl011_uart_send(unsigned int c);

void pl011_uart_send_buf(const char *c, int n);

void pl011_uart_set_interrupt_mode();

char pl011_uart_getc();

int pl011_io_thread(void);
