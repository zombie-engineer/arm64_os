#pragma once

#include <compiler.h>
#include <types.h>

typedef int(*irq_cb_t)();

void set_irq_cb(irq_cb_t cb);

void set_fiq_cb(irq_cb_t cb);

void generate_exception();

void __handle_interrupt();

typedef struct exception_info {
  uint64_t esr; 
  uint64_t spsr; 
  uint64_t elr; 
  uint64_t far;
  uint64_t type;
  void *cpu_ctx;
  uint64_t *stack;
} packed exception_info_t;
