#pragma once
#include <compiler.h>
#include <types.h>

typedef struct cpuctx cpuctx_t;

typedef struct reg_info {
  char name[8];
  uint64_t value;
} packed reg_info_t;

typedef struct bin_regs_hdr {
  char magic[8];
  int crc;
  int len;
  int numregs;
} packed bin_regs_hdr_t;

int cpuctx_serialize(const void *ctx, bin_regs_hdr_t *h, char *buf, int bufsize);

typedef struct cpuctx_init_opts {
  /* address to buffer, used as a cpu context
   * generic for architectures
   */
  void *cpuctx;

  /* size of the above buffer
   */
  int cpuctx_sz;

  /* start of stack will be set to this value
   */
  uint64_t sp;

  /* program counter will be set to this value
   */
  uint64_t pc;

  /* arguments that will be passed to function
   */
  struct {
    int argc;
    char **argv;
  } args;

} cpuctx_init_opts_t;

int cpuctx_init(cpuctx_init_opts_t *o);


/* This is a generic way to pull out any register set from any cpu. */
typedef struct cpu_reg {
  char name[16];
  uint64_t value;
} cpu_reg_t;

typedef int (*cpuctx_enum_regs_cb)(const cpu_reg_t *, int, void *);

int cpuctx_enum_registers(const void *ctx, cpuctx_enum_regs_cb cb, void *cb_priv);

typedef int (*cpuctx_print_regs_cb)(const char *reg_str, size_t reg_str_sz, void *cb_priv);
int cpuctx_print_regs(void *ctx, cpuctx_print_regs_cb cb, void *cb_priv);

void enable_irq(void);
void disable_irq(void);
int is_irq_enabled(void);

#define WAIT_FOR_EVENT asm volatile("wfe" ::: "memory")

