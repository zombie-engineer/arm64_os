#include <cmdrunner.h>
#include "cmdrunner_internal.h"
#include <uart/uart.h>
#include <types.h>
#include <spinlock.h>
#include <pipe.h>
#include <compiler.h>
#include <barriers.h>
#include <cpu.h>
#include <debug.h>
#include <drivers/display/nokia5110_console.h>
#include <console.h>

static char ringbuf_buf[8];
static struct pipe char_pipe;

static char cmdrunner_getch()
{
  char c;
  c = pipe_pop(&char_pipe);
  return c;
}

static void cmdrunner_rx_cb(void *priv, char c)
{
  pipe_push(&char_pipe, c);
}

extern int pl011_uart_putchar(uint8_t c);

int cmdrunner_process(void)
{
  char c;
  int i = 0;
  // console_dev_t *console;
  // console = nokia5110_get_console_device();
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);
  pipe_init(&char_pipe, ringbuf_buf, sizeof(ringbuf_buf));

  while(!uart_is_initialized());
  uart_subscribe_to_rx_event(cmdrunner_rx_cb, 0);

  while(1) {
    i++;
    if (i == 8) {
     // pipe_debug(&char_pipe);
      i = 0;
    }
    c = cmdrunner_getch();
    cmdrunner_handle_char(&s, c);
    // pl011_uart_putchar(c);
    // console->putc(c);
  }

  return 0;
}
