#pragma once

/* Init PL011 UART */
void pl011_uart_init(int baudrate, int unused);

void pl011_uart_send(unsigned int c);

char pl011_uart_getc();

