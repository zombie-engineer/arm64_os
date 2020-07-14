#pragma once
#include <types.h>

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

atomic_t atomic_cmp_and_swap(atomic_t *a, uint64_t expected_val, uint64_t new_val);

atomic_t atomic_inc(atomic_t *a);

atomic_t atomic_dec(atomic_t *a);

