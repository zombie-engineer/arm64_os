#pragma once
#include <uart/uart.h>

/* Init PL011 UART */
void pl011_uart_init(int baudrate, int _not_used);

void pl011_uart_send(unsigned int c);

int pl011_uart_send_buf(const void *buf, size_t n);

void pl011_uart_set_interrupt_mode();

char pl011_uart_getc();

int pl011_io_thread(void);

int pl011_uart_subscribe_to_rx_event(uart_rx_event_cb cb, void *cb_arg);
