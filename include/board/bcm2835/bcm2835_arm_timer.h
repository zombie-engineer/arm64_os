#pragma once
#include <timer.h>

int bcm2835_arm_timer_init();

void bcm2835_arm_timer_dump_regs(const char* tag);

int bcm2835_arm_timer_set_periodic(uint32_t usec, timer_callback_t cb, void *cb_arg);

int bcm2835_arm_timer_set_oneshot(uint32_t usec, timer_callback_t cb, void *cb_arg);
