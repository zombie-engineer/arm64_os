#pragma once

/* Init PL011 UART */
void pl011_uart_init(int baudrate, int unused);

void pl011_uart_send(unsigned int c);

void pl011_uart_send_buf(const char *c, int n);

void pl011_set_interrupt_mode();

char pl011_uart_getc();

