#include <config.h>
#include <uart/uart.h>
#include <uart/pl011_uart.h>
#include <uart/mini_uart.h>
#include <uart/uart.h>
#include <spinlock.h>
#include <common.h>

#include <error.h>

#if defined UART_PL011 && defined UART_MINI
#error only one implementation of UART is possible.
#endif

#if defined(CONFIG_UART_PL011)
#define uart_send pl011_uart_send
#define uart_recv pl011_uart_getc
#define uart_rx_not_empty pl011_uart_rx_not_empty
#define _uart_init pl011_uart_init
#define _uart_send_buf pl011_uart_send_buf
#define _uart_set_interrupt_mode pl011_uart_set_interrupt_mode
#define _uart_subscribe_to_rx_event pl011_uart_subscribe_to_rx_event
#elif defined(CONFIG_UART_MINI)
#define uart_send mini_uart_send
#define uart_recv mini_uart_getc
#define uart_rx_not_empty mini_uart_rx_not_empty
#define _uart_init mini_uart_init
#define _uart_send_buf mini_uart_send_buf
#define _uart_set_interrupt_mode mini_uart_set_interrupt_mode
#define _uart_subscribe_to_rx_event mini_uart_subscribe_to_rx_event
#endif

static DECL_COND_SPINLOCK(uart_lock);
void uart_init(int baudrate, int system_clock)
{
  _uart_init(baudrate, system_clock);
  cond_spinlock_init(&uart_lock);
}

int uart_putc(char c)
{
  int irqflags;
  cond_spinlock_lock_disable_irq(&uart_lock, irqflags);
  uart_send(c);
  cond_spinlock_unlock_restore_irq(&uart_lock, irqflags);
  return ERR_OK;
}

char uart_getc()
{
  char r;
  int irqflags;
  cond_spinlock_lock_disable_irq(&uart_lock, irqflags);
  r = uart_recv();
  cond_spinlock_unlock_restore_irq(&uart_lock, irqflags);
  return r == '\r' ? '\n' : r;
}

int uart_puts(const char *s)
{
  int irqflags;
  cond_spinlock_lock_disable_irq(&uart_lock, irqflags);

  while(*s)
  {
    if (*s == '\n')
      uart_send('\r');
    uart_send(*s++);
  }

  cond_spinlock_unlock_restore_irq(&uart_lock, irqflags);
  return ERR_OK;
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

int uart_send_buf(const void *buf, size_t n)
{
  return _uart_send_buf(buf, n);
}

void uart_set_interrupt_mode()
{
  _uart_set_interrupt_mode();
}

int uart_subscribe_to_rx_event(uart_rx_event_cb cb, void *priv)
{
  return _uart_subscribe_to_rx_event(cb, priv);
}

#ifdef ENABLE_UART_DOWNLOAD
void check_uart_download(void)
{
  char c;
  if (uart_rx_not_empty()) {
    c = uart_recv();
    if (c == 'd') {
      printf("HELLO\r\n");
      while(1) asm volatile ("wfe");
    }
  }
  printf("Skipping uart download stage" __endline);
}
#endif
