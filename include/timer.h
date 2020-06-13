#pragma once
#include <memory.h>
#include <types.h>
#include <reg_access.h>
#include <gpio.h>
#include <time.h>

#define TIMER_ID_SYSTIMER 1
#define TIMER_ID_ARM_TIMER 2
#define TIMER_ID_ARM_GENERIC_TIMER 3

// Generic callback triggered at timer event
typedef void(*timer_callback_t)(void*);

int systimer_init();

int systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg);

int systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg);

void timer_irq_callback();

#define TIMER_NAME_LEN 8

struct timer {
  char *name;
  int flags;
  int (*interrupt_enable)(void);
  int (*set_oneshot)(uint32_t usec, timer_callback_t cb, void *cb_arg);
  int (*set_periodic)(uint32_t usec, timer_callback_t cb, void *cb_arg);
};

int timer_register(struct timer *t, int id);
struct timer *timer_get(int timer_id);
void list_timers();
