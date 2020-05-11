#pragma once
#include <types.h>

#define __GET_BYTE(__addr, __i) (((char *)__addr)[__i])
static inline uint16_t get_unaligned_16_le(void *addr)
{
  return (uint16_t)((__GET_BYTE(addr, 1) << 8) | (__GET_BYTE(addr, 0)));
}

