#include <cmdrunner.h>
#include "cmdrunner_internal.h"
#include <uart/uart.h>
#include <types.h>
#include <spinlock.h>
#include <ringbuf.h>
#include <compiler.h>
#include <barriers.h>
#include <cpu.h>
#include <debug.h>
#include <drivers/display/nokia5110_console.h>
#include <console.h>

static char ringbuf_buf[256];
static ringbuf_t char_pipe;
static struct spinlock char_pipe_lock;

static char cmdrunner_getch()
{
  int n;
  char c;
  int irqflags;
  n = 0;
  while(1) {
    spinlock_lock_disable_irq(&char_pipe_lock, irqflags);
    n = ringbuf_read(&char_pipe, &c, 1);
    spinlock_unlock_restore_irq(&char_pipe_lock, irqflags);
    if (n)
      break;

    SYNC_BARRIER;
    WAIT_FOR_EVENT;
  }
  debug_event_1();
  return c;
}

static void cmdrunner_rx_cb(void *priv, char c)
{
  int irqflags;
  spinlock_lock_disable_irq(&char_pipe_lock, irqflags);
  ringbuf_write(&char_pipe, &c, 1);
  spinlock_unlock_restore_irq(&char_pipe_lock, irqflags);
}

extern int pl011_uart_putchar(uint8_t c);

int cmdrunner_process(void)
{
  char c;
  console_dev_t *console; 
  console = nokia5110_get_console_device();
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);
  ringbuf_init(&char_pipe, ringbuf_buf, sizeof(ringbuf_buf));
  spinlock_init(&char_pipe_lock);

  while(!uart_is_initialized());
  uart_subscribe_to_rx_event(cmdrunner_rx_cb, 0);

  while(1) {
    c = cmdrunner_getch();
    pl011_uart_putchar(c);
    console->putc(c);
  }

  return 0;
}
