#pragma once
#include <error.h>
#include <kernel_panic.h>
#include <cpu.h>
#include <spinlock.h>

#define min(a, b) (a < b ? a : b)

#define max(a, b) (a < b ? b : a)

#ifndef TEST_STRING
#define printf _printf
#define puts _puts
#define putc _putc
#endif

const char * _printf(const char *fmt, ...);

void _puts(const char *str);

void _putc(char ch);

void hexdump_addr(unsigned int *addr);

#define print_reg32(regname) printf(#regname " %08x\n",  *regname)

#define print_reg32_at(regname) printf(#regname " %08x\n",  *(reg32_t)regname)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define DECL_ASSIGN_PTR(type, name, value) type *name = (type *)(value)

#define DECL_FUNC_CHECK_INIT(fn, cond, ...) \
int fn(__VA_ARGS__) { \
  if (!cond) return ERR_NOT_INIT;

#define RET_IF_ERR(fn, ...) if ((err = fn(__VA_ARGS__))) return err;

#define __ptr_off(ptr, offset) ((const char *)(ptr)) + offset

#define address_of(type, member) (unsigned long long)(&((type *)0)->member)

#define container_of(ptr, type, member) (type *)(__ptr_off(ptr, - address_of(type, member)))

static inline int should_lock() 
{
  return spinlocks_enabled && is_irq_enabled();
}
