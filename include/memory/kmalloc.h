#pragma once
#include <types.h>

extern char __kernel_memory_start;
extern char __kernel_memory_end;

void *kmalloc(size_t size);
void kfree(void *addr);
void kmalloc_init(void);
