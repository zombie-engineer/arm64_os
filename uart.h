#pragma once
#define UART_CONSOLE_NAME "uartcon"

void uart_init();

void uart1_send(unsigned int c);

void uart_send(unsigned int c);

char uart1_getc();

char uart_getc();

void uart_puts(const char *s);

void uart_putc(char c);

void uart_hex(unsigned int d);
