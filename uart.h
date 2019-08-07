#pragma once

void uart_init();

void uart1_send(unsigned int c);

void uart_send(unsigned int c);

char uart1_getc();

char uart_getc();

void uart_puts(char *s);

void uart_hex(unsigned int d);
