#pragma once

#define min(a, b) (a < b ? a : b)

const char * printf(const char *fmt, ...);

void puts(const char *str);

void putc(char ch);

void hexdump_addr(unsigned int *addr);

#define print_reg32(regname) printf(#regname " %08x\n",  regname)

#define print_reg32_at(regname) printf(#regname " %08x\n",  *(reg32_t)regname)

