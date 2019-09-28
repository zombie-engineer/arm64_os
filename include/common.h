#pragma once

const char * printf(const char* fmt, ...);

void hexdump_addr(unsigned int *addr);

#define print_reg32(regname) printf(#regname " %08x\n",  regname)

#define print_reg32_at(regname) printf(#regname " %08x\n",  *(reg32_t)regname)

