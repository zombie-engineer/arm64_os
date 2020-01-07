#pragma once
#include "reg_access.h"
#include "gpio.h"

uint32_t interrupt_ctrl_read_pending_gpu_1();

void interrupt_ctrl_dump_regs(const char* tag);

void interrupt_ctrl_enable_timer_irq(void);

void interrupt_ctrl_enable_gpio_irq(int gpio_num);

void interrupt_ctrl_enable_systimer_1(void);

void interrupt_ctrl_enable_systimer_3(void);

void interrupt_ctrl_disable_systimer_1(void);

void interrupt_ctrl_disable_systimer_3(void);
