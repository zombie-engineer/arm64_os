#pragma once
#include <ringbuf.h>
#include <spinlock.h>
#include <sched.h>
#include <atomic.h>

struct pipe {
  struct spinlock lock;
  atomic_t waitflag;
  struct ringbuf ringbuf;
};

void pipe_init(struct pipe *p, char *buf, int bufsize);

void pipe_push(struct pipe *p, char c);

char pipe_pop(struct pipe *p);

