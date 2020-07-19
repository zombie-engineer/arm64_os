#include "pipe.h"

void pipe_push(struct pipe *p, char c)
{
  int irqflags;
  spinlock_lock_disable_irq(&p->lock, irqflags);
  ringbuf_write(&p->ringbuf, &c, 1);
  spinlock_unlock_restore_irq(&p->lock, irqflags);
  wakeup_waitflag(&p->waitflag);
}

char pipe_pop(struct pipe *p)
{
  char c;
  wait_on_waitflag(&p->waitflag);
  BUG(ringbuf_read(&p->ringbuf, &c, 1) != 1, "pl011_rx_pipe_pop should wait until char appears in buf");
  return c;
}

void pipe_init(struct pipe *p, char *buf, int bufsize)
{
  ringbuf_init(&p->ringbuf, buf, bufsize);
  spinlock_init(&p->lock);
  waitflag_init(&p->waitflag);
}

