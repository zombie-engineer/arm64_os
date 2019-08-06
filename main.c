#include "uart.h"
void main()
{
  uart_init();

  uart_puts("hello\n");

  while(1) {
    char c = uart_getc();
    uart_send(c);
  }
}
