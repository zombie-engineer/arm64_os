#pragma once

#define min(a, b) (a < b ? a : b)

#define max(a, b) (a < b ? b : a)

const char * printf(const char *fmt, ...);

void puts(const char *str);

void putc(char ch);

void hexdump_addr(unsigned int *addr);

int isspace(int c);

#define print_reg32(regname) printf(#regname " %08x\n",  regname)

#define print_reg32_at(regname) printf(#regname " %08x\n",  *(reg32_t)regname)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define DECL_ASSIGN_PTR(type, name, value) type *name = (type *)(value)

