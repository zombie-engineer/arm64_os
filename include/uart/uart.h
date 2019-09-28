#pragma once

#define UART_CONSOLE_NAME "uartcon"

void uart_init(int freq, int system_clock);

char uart_getc();

void uart_puts(const char *s);

void uart_putc(char c);

// TODO remove that
void uart_hex(unsigned int d);
