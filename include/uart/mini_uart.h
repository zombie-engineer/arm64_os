#pragma once

void mini_uart_init(int target_baudrate, int system_clock);

void mini_uart_send(unsigned int c);

char mini_uart_getc();
