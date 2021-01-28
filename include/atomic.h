#pragma once
#include <types.h>
#include <compiler.h>

typedef uint64_t atomic_t;

static inline atomic_t ldxr(atomic_t *a)
{
  atomic_t result;
  asm volatile("ldxr %0, [%1]": "=r"(result) : "r"(a));
  return result;
}

static inline OPTIMIZED bool stxr(atomic_t *a, atomic_t val)
{
  bool result;
  asm volatile("stxr %w0, %1, [%2]" : "=r"(result) : "r"(val) , "r"(a));
  return result;
}

static inline int atomic_read(atomic_t *a)
{
  return *a;
}

static inline int atomic_cmpxchg(atomic_t *a, int cmp, int val)
{
  uint64_t res, oldval = 0;
  asm volatile(
      "ldxr  %w1, %2\n"
      "mov   %w0, #0\n"
      "cmp   %w1, %w3\n"
      "b.ne  1f\n"
      "stxr  %w0, %w4, %2\n"
      "1:\n"
      : "=&r" (res), "=&r" (oldval), "+Q"(*(uint64_t *)a)
      : "Ir" (oldval), "r"(val)
      );
  return oldval;
}

atomic_t atomic_cmp_and_swap(atomic_t *a, uint64_t expected_val, uint64_t new_val);

atomic_t atomic_inc(atomic_t *a);

atomic_t atomic_dec(atomic_t *a);

static inline void atomic_set(atomic_t *a, uint64_t val)
{
  *a = val;
}

