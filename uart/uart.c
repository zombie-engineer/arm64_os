#include <config.h>
#include <uart/uart.h>
#include <uart/pl011_uart.h>
#include <uart/mini_uart.h>

#if defined UART_PL011 && defined UART_MINI
#error only one implementation of UART is possible.
#endif

#if defined(UART_PL011)
#define uart_send pl011_uart_send 
#define uart_recv pl011_uart_getc
#define _uart_init pl011_uart_init
#elif defined(UART_MINI)
#define uart_send mini_uart_send 
#define uart_recv mini_uart_getc
#define _uart_init mini_uart_init
#endif

void uart_init(int baudrate, int system_clock)
{
  _uart_init(baudrate, system_clock);
}

void uart_putc(char c)
{
  uart_send(c);
}

char uart_getc()
{
  char r;
  r = uart_recv();
  return r == '\r' ? '\n' : r;
}

void uart_puts(const char *s)
{
  while(*s)
  {
    if (*s == '\n')
      uart_send('\r');
    uart_send(*s++);
  }
}

void uart_hex(unsigned int d)
{
  unsigned int n;
  int c;
  for(c = 28; c >= 0; c -=4) {
    n = (d>>c) & 0xf;
    n += n > 9 ? 0x37 : 0x30;
    uart_send(n);
  }
}