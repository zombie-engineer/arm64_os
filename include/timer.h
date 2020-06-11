#pragma once
#include <memory.h>
#include <types.h>
#include <reg_access.h>
#include <gpio.h>
#include <time.h>

// Generic callback triggered at timer event
typedef void(*timer_callback_t)(void*);

int systimer_init();

int systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg);

int systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg);

void timer_irq_callback();

struct timer {
  int flags;
  int (*set_oneshot)(uint32_t usec, timer_callback_t cb, void *cb_arg);
  int (*set_periodic)(uint32_t usec, timer_callback_t cb, void *cb_arg);
};

int timer_register(struct timer *t, int timer_id);
struct timer *get_timer(int timer_id);
