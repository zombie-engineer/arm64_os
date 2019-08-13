#pragma once
#include "reg_access.h"
#include "gpio.h"

#define ARM_TIMER_BASE                     0x3f00b400
// ARM_TIMER_BASE is proved to be exactly 0x3f00b400
#define ARM_TIMER_LOAD_REG                 *(volatile int*)(ARM_TIMER_BASE)
#define ARM_TIMER_VALUE_REG                *(volatile int*)(ARM_TIMER_BASE + 4)
#define ARM_TIMER_CONTROL_REG              *(volatile int*)(ARM_TIMER_BASE + 8)
#define ARM_TIMER_IRQ_CLEAR_ACK_REG        *(volatile int*)(ARM_TIMER_BASE + 12)
#define ARM_TIMER_RAW_IRQ_REG              *(volatile int*)(ARM_TIMER_BASE + 16)
#define ARM_TIMER_MASKED_IRQ_REG           *(volatile int*)(ARM_TIMER_BASE + 20)
#define ARM_TIMER_RELOAD_REG               *(volatile int*)(ARM_TIMER_BASE + 24)
#define ARM_TIMER_PRE_DIVIDED_REG          *(volatile int*)(ARM_TIMER_BASE + 28)
#define ARM_TIMER_FREE_RUNNING_COUNTER_REG *(volatile int*)(ARM_TIMER_BASE + 32)

#define ARM_TMR_CTRL_R_WIDTH_BP  1
#define ARM_TMR_CTRL_R_IRQ_EN_BP 5
#define ARM_TMR_CTRL_R_TMR_EN_BP 7

typedef enum {
  Clkdiv1      = 0b00,
  Clkdiv16     = 0b01,
  Clkdiv256    = 0b10,
  Clkdiv_undef = 0b11,
} TIMER_PRESCALE;

typedef struct {
  unsigned long cs;
  unsigned long clo;
  unsigned long chi;
  unsigned long c0;
  unsigned long c1;
  unsigned long c2;
  unsigned long c3;
} __attribute__ ((packed)) timer_ctrl_t;


typedef struct {
  unsigned unused           : 1;
  unsigned couner_32_bit    : 1;
  unsigned prescale         : 2;
  unsigned unused4          : 1;
  unsigned timer_irq_enable : 1;
  unsigned unused6          : 1;
  unsigned timer_enable     : 1;
  unsigned timer_halt       : 1;
  unsigned free_runnung_en  : 1;
  unsigned unused10         : 6;
  unsigned unused16         : 7;
  char     unused24;
} __attribute__((packed)) arm_timer_control_reg_t;

typedef struct {
  unsigned load;
  unsigned value;
  union {
    unsigned raw;
    arm_timer_control_reg_t t;
  } control;
  unsigned irq_clr_ack;
  unsigned raw_irq;
  unsigned masked_irq;
  unsigned reload;
  unsigned pre_divider;
  unsigned free_counter;
} __attribute__((packed)) arm_timer_regs_t;

#define DECL_ARM_TIMER(var) DECL_PERIPH_BASE(arm_timer_regs_t, var, (MMIO_BASE + 0xb400))

void arm_timer_dump_regs(const char* tag);

void arm_timer_set(unsigned period_in_us, void(*irq_timer_handler)(void));
