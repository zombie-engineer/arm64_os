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

static inline OPTIMIZED atomic_t atomic_cmp_and_swap(atomic_t *a, uint64_t expected_val, uint64_t new_val)
{
  atomic_t ret;
  asm volatile(
      "mov x3, x0\n"
      "1:\n"
      "ldxr x0, [x3]\n"
      "cbz x0, 2f\n"
      "stxr w0, xzr, [x3]\n"
      "cbnz w0, 1b\n"
      "mov x0, #1\n"
      "2:\n"
  :"=r"(ret):: "memory","cc","x3");
  return ret;
}
