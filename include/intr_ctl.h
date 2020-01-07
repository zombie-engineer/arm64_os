#pragma once
#include "reg_access.h"
#include "gpio.h"

uint32_t intr_ctl_read_pending_gpu_1();

void intr_ctl_dump_regs(const char* tag);

void intr_ctl_enable_timer_irq(void);

void intr_ctl_enable_gpio_irq(int gpio_num);

void intr_ctl_enable_systimer_1(void);

void intr_ctl_enable_systimer_3(void);

void intr_ctl_disable_systimer_1(void);

void intr_ctl_disable_systimer_3(void);
