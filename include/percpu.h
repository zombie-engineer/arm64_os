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
struct percpu_data {
  uint64_t stack_el0;
  uint64_t stack_el1;
  uint64_t jmp_addr;
  /*
   * MPIDR register value, learned at startup.
   */
  uint64_t mpidr;
  /*
   * Execution context of a currently scheduled task on this
   * processing unit.
   */
  uint64_t context_addr;
  uint64_t padding[3];
} PACKED;

/*
 * Global variable pointing to cpuctx of a current task
 */
extern struct percpu_data __percpu_data[NUM_CORES];

// extern struct percpu_context percpu_context[NUM_CORES];

// struct percpu_context *get_percpu_context(int cpu_num);
