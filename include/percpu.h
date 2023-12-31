#pragma once
#include <compiler.h>
#include <types.h>
#include <config.h>

/*
 * __percpu_func - is a hint notice to indicate this function runs in percpu
 * context, in other words the function will access/modify data, that is
 * related to exactly to the id of currently running CPU/CORE.
 */
#define __percpu_func

#define PCPU_ALIGN ALIGNED(64)
#define PCPU_DATA_TYPE(name) struct pcpu_ ## name
#define PCPU_DATA_NAME(name) pcpu_ ## name

#define DECL_PCPU_DATA(type, name) \
  PCPU_DATA_TYPE(name) { type __data PCPU_ALIGN; };\
  PCPU_DATA_TYPE(name) PCPU_DATA_NAME(name)[NUM_CORES] PCPU_ALIGN

#define get_pcpu_data_n(cpu_num, name) \
  (&PCPU_DATA_NAME(name)[cpu_num].__data)

#define get_pcpu_data(name) get_pcpu_data_n(get_cpu_num(), name)

/*
 * Structure that stores information per processing unit.
 * For best cache performance each per-cpu context should be
 * aligned to cache line width. This way writing to one will
 * not invalidate others.
 */
struct percpu_data {
  uint64_t stack_el0;
  uint64_t stack_el1;
  void *jmp_addr;
  /*
   * MPIDR register value, learned at startup.
   */
  uint64_t mpidr;
  /*
   * Execution context of a currently scheduled task on this
   * processing unit.
   */
  void *context_addr;
  uint64_t padding[3];
} PACKED;

/*
 * Global variable pointing to cpuctx of a current task
 */
extern struct percpu_data __percpu_data[NUM_CORES];

// extern struct percpu_context percpu_context[NUM_CORES];

// struct percpu_context *get_percpu_context(int cpu_num);
