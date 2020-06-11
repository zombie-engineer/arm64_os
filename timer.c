#include <timer.h>
#include <error.h>
#include <config.h>
#include <common.h>

#if defined CONFIG_SYSTEM_TIMER_BCM2835_ARM_TIMER
#include <board/bcm2835/bcm2835_arm_timer.h>
#elif defined CONFIG_SYSTEM_TIMER_BCM2835_SYSTEM_TIMER
#include <board/bcm2835/bcm2835_systimer.h>
#else
#error System timer not selected
#endif

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
  return ERR_OK;
}

struct timer *get_timer(int timer_id)
{
  int i;
  for (i = 0; i < num_timers; ++i) {
    if (timers[i].id == timer_id)
      return timers[i].t;
  }
  return NULL;
}

typedef struct systimer {
  int (*set_periodic)(uint32_t usec, timer_callback_t cb, void *cb_arg);
  int (*set_oneshot)(uint32_t usec, timer_callback_t cb, void *cb_arg);
} systimer_t;

static systimer_t systimer;

int systimer_init()
{
  int status;
  status = ERR_FATAL;
#if defined CONFIG_SYSTEM_TIMER_BCM2835_ARM_TIMER
  status = bcm2835_arm_timer_init();
  if (status == ERR_OK) {
    systimer.set_periodic = bcm2835_arm_timer_set_periodic;
    systimer.set_oneshot = bcm2835_arm_timer_set_oneshot;
  }
#elif defined CONFIG_SYSTEM_TIMER_BCM2835_SYSTEM_TIMER
  status = bcm2835_systimer_init();
  if (status == ERR_OK) {
    systimer.set_periodic = bcm2835_systimer_set_periodic;
    systimer.set_oneshot = bcm2835_systimer_set_oneshot;
  }
#endif
  return status;
}

int systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  return systimer.set_periodic(usec, cb, cb_arg);
}

int systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg)
{
  return systimer.set_oneshot(usec, cb, cb_arg);
}


