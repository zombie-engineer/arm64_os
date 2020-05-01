#pragma once
#include <error.h>
#include <kernel_panic.h>
#include <cpu.h>
#include <spinlock.h>
#include <defs.h>

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

void hexdump_memory(const void *addr, size_t sz);

#define __endline "\r\n"

#define print_reg32(regname) printf(#regname " %08x" __endline,  *regname)

#define print_reg32_at(regname) printf(#regname " %08x" __endline,  *(reg32_t)regname)

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define __msg_codeline(__msg) "("__FILE__ ":" STRINGIFY(__LINE__) "): "__msg __endline

#define __puts_codeline(__msg) puts(__msg_codeline(__msg))
#define __printf_codeline(__fmt, ...) printf(__msg_codeline(__fmt), ## __VA_ARGS__)

#define logf(__prefix, __fmt, ...)\
  printf(__prefix "%s:" __fmt __endline, __func__, ## __VA_ARGS__)


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

#define spinlock_cond_lock(lock) \
  do {                           \
  if (should_lock())             \
    spinlock_lock(lock);         \
  } while(0)

#define spinlock_cond_unlock(lock) \
  do {                             \
    if (should_lock())             \
      spinlock_unlock(lock);       \
  } while(0)


