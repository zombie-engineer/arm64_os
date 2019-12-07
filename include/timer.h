#pragma once
#include <memory.h>
#include <types.h>
#include <reg_access.h>
#include <gpio.h>

void bcm2835_arm_timer_init();

void bcm2835_arm_timer_dump_regs(const char* tag);

typedef void*(*timer_callback)(void*);

void timer_irq_callback();

void bcm2835_arm_timer_set(uint32_t usec, timer_callback cb, void *cb_arg);

