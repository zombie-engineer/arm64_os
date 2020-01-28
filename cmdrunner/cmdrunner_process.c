#include <cmdrunner.h>
#include "cmdrunner_internal.h"
#include <uart/uart.h>
#include <types.h>
#include <spinlock.h>
#include <ringbuf.h>
#include <compiler.h>
#include <barriers.h>
#include <cpu.h>

static ringbuf_t char_pipe;
static aligned(64) uint64_t char_pipe_lock;

static char cmdrunner_getch()
{
  int n;
  char c;

  while(1) {
    spinlock_lock(&char_pipe_lock);
    n = ringbuf_read(&char_pipe, &c, 1);
    spinlock_unlock(&char_pipe_lock);
    if (n)
      break;

    SYNC_BARRIER;
    WAIT_FOR_EVENT;
  }
  return c;
}

static void cmdrunner_rx_cb(void *priv, char c)
{
  spinlock_lock(&char_pipe_lock);
  ringbuf_write(&char_pipe, &c, 1);
  spinlock_unlock(&char_pipe_lock);
}

int cmdrunner_process(void)
{
  char c;
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);
  uart_subscribe_to_rx_event(cmdrunner_rx_cb, 0);

  while(1) {
    c = cmdrunner_getch();
    pl011_uart_putchar(c);
  }

  return 0;
}
