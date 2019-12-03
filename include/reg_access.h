#pragma once

#define DECL_PERIPH_BASE(type, var, addr) volatile type * var = (volatile type *)(addr)

typedef volatile unsigned int* reg32_t;

#define write_reg(reg_addr, value) *(reg32_t)(reg_addr) = (int)(value)
#define read_reg(reg_addr) (*(reg32_t)(reg_addr))
