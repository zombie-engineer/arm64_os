#pragma once
#include <types.h>

#define UART_CONSOLE_NAME "uartcon"

typedef void (*uart_rx_event_cb)(void *, char);

void uart_init(int freq, int system_clock);

void uart_set_interrupt_mode();

char uart_getc();

void uart_puts(const char *s);

void uart_putc(char c);

// TODO remove that
void uart_hex(unsigned int d);

int uart_send_buf(const void *buf, size_t n);

int uart_subscribe_to_rx_event(uart_rx_event_cb cb, void *priv);

int uart_is_initialized();
