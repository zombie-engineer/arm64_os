#pragma once
#include <error.h>

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

