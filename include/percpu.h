#pragma once
#include <compiler.h>
#include <types.h>
#include <config.h>

/*
 * Structure that stores information per processing unit.
 * For best cache performance each per-cpu context should be 
 * aligned to cache line width. This way writing to one will
 * not invalidate others.
 */
struct percpu_context {
  /*
   * MPIDR register value, learned at startup.
   */
  uint64_t mpidr;
  /*
   * Execution context of a currently scheduled task on this
   * processing unit.
   */
  void *__current_cpuctx;
  void (*cpu_startup_fn)(struct percpu_context *);
  uint64_t *el1_stack;
  uint64_t padding[4];
} PACKED;

extern struct percpu_context percpu_context[NUM_CORES];

struct percpu_context *get_percpu_context(int cpu_num);
