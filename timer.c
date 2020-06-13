#include <timer.h>
#include <error.h>
#include <config.h>
#include <common.h>

#define MAX_TIMERS 8

struct timer_entry {
  struct timer *t;
  int id;
};

static struct timer_entry timers[MAX_TIMERS];

static int num_timers = 0;

int timer_register(struct timer *t, int timer_id)
{
  int i;
  for (i = 0; i < num_timers; ++i) {
    if (timers[i].id == timer_id)
      return ERR_BUSY;
  }
  if (num_timers == MAX_TIMERS)
    return ERR_NO_RESOURCE;

  timers[num_timers].t = t;
  timers[num_timers].id = timer_id;
  num_timers++;
  dcache_clean_and_invalidate_rng((uint64_t)timers, (uint64_t)((char *)timers + sizeof(timers)));
  return ERR_OK;
}

struct timer *timer_get(int timer_id)
{
  int i;
  for (i = 0; i < num_timers; ++i) {
    if (timers[i].id == timer_id)
      return timers[i].t;
  }
  return NULL;
}

void list_timers()
{
  int i;
  for (i = 0; i < num_timers; ++i) {
    struct timer_entry *t = &timers[i];
    printf("timer id=%d, name=%s, addr=%p" __endline, t->id, t->t->name, t->t);
  }
}
