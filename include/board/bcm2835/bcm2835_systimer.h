#pragma once
#include <types.h>

int bcm2835_systimer_init();

int bcm2835_systimer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg);

int bcm2835_systimer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg);
