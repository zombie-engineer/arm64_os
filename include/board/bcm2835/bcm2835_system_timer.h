#pragma once
#include <types.h>

int bcm2835_system_timer_init();

int bcm2835_system_timer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg);

int bcm2835_system_timer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg);
