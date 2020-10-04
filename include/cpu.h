#pragma once
#include <compiler.h>
#include <types.h>

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

typedef struct binblock_cpuctx binblock_cpuctx_t;
int cpuctx_binblock_fill_regs(const void *ctx, binblock_cpuctx_t *h);

uint64_t cpuctx_get_sp(const void *ctx);

uint64_t cpuctx_get_fp(const void *ctx);

#define irq_mask_all()\
  asm volatile("msr daifset, #0xf")

#define disable_irq_save_flags(__flags)\
  asm volatile(\
      "mrs %0, daif\n"\
      "msr daifset, #2\n"\
      : "=r"(__flags)\
      :\
      : "memory")

#define restore_irq_flags(__flags)\
  asm volatile("msr daif, %0\n"\
      :\
      : "r"(__flags)\
      : "memory")

#define get_daif(__daif)\
  asm volatile("mrs %0, daif\n" : "=r"(__daif))

void enable_irq(void);
void disable_irq(void);
int is_irq_enabled(void);

/*
 * dcache_clean_and_invalidate_rng - cleans and invalidates
 * all data cache lines that form whole virtual address range
 * given by the arguments.
 * vaddr_start - virtual address of first byte in range
 * vaddr_end   - virtual address of end of range
 */
void dcache_clean_and_invalidate_rng(uint64_t vaddr_start, uint64_t vaddr_end);

void dcache_invalidate_rng(uint64_t vaddr_start, uint64_t vaddr_end);

#define dcache_flush(addr, size)\
  dcache_clean_and_invalidate_rng((uint64_t)(addr), (uint64_t)(addr) + (size))

#define dcache_invalidate(addr, size)\
  dcache_invalidate_rng((uint64_t)(addr), (uint64_t)(addr) + (size))

/*
 * dcache_line_width - read cpu regs and returns data cache
 * line width
 */
uint64_t dcache_line_width();

#define WAIT_FOR_EVENT asm volatile("wfe" ::: "memory")

/*
 * Returns system clock frequency in Hz
 */
uint64_t get_cpu_counter_64_freq(void);

/*
 *  reads cpu-specific generic 64 bit counter
 */
uint64_t read_cpu_counter_64(void);

void print_cpu_flags();

int get_cpu_num();
