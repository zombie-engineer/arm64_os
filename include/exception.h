#pragma once

#include <compiler.h>
#include <types.h>

typedef void(*irq_cb_t)();

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
  uint64_t *stack_base;
} packed exception_info_t;

typedef void (*exception_hook)(exception_info_t *);
int add_unhandled_exception_hook(exception_hook h);

typedef void (*kernel_panic_reporter)(exception_info_t *, const char *);
int add_kernel_panic_reporter(kernel_panic_reporter h);

const char *get_synchr_exception_class_string(int esr);
const char *get_data_abort_string(int esr);
const char *get_exception_type_string(int type);
int gen_exception_string_generic(exception_info_t *e, char *buf, size_t bufsz);
int gen_exception_string_specific(exception_info_t *e, char *buf, size_t bufsz);
