#pragma once
#include "reg_access.h"
#include "gpio.h"

#define INTERRUPT_CONTROLLER_BASE (PERIPHERAL_BASE_PHY + 0xb200)

typedef struct {
  unsigned timer                 : 1;
  unsigned mailbox               : 1;
  unsigned doorbell0             : 1;
  unsigned doorbell1             : 1;
  unsigned gpu0_halted           : 1;
  unsigned gpu1_halted           : 1;
  unsigned illegal_access_type_1 : 1;
  unsigned illegal_access_type_0 : 1;
  char reserved8;
  char reserved16;
  char reserved24;
} __attribute__((packed)) arm_irq_enable_t;

typedef struct {
  unsigned irq_basic_pending;
  unsigned irq_pending_1;
  unsigned irq_pending_2;
  unsigned fiq_control;
  unsigned enable_irqs_1;
  unsigned enable_irqs_2;
  union {
    unsigned u32;
    arm_irq_enable_t t;
  } enable_basic_irqs;
  unsigned disable_irqs_1;
  unsigned disable_irqs_2;
  unsigned disable_base_irqs;
} __attribute__((packed)) irq_controller_t;


void interrupt_ctrl_dump_regs(const char* tag);

void interrupt_ctrl_enable_timer_irq(void);

void interrupt_ctrl_enable_gpio_irq(int gpio_num);

void interrupt_ctrl_enable_sys_timer_irq(void);
