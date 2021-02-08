#pragma once
#include <types.h>

extern char __kernel_memory_start;
extern char __kernel_memory_end;

#define GFP_KERNEL (1<<0)
#define GFP_ZERO   (1<<1)

void *kmalloc(size_t size, int flags);

void kfree(void *addr);

void kmalloc_init(void);

static inline void *kzalloc(size_t size, int flags)
{
  return kmalloc(size, flags | GFP_ZERO);
}
