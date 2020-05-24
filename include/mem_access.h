#pragma once
#include <types.h>
#include <bits_api.h>

#define __GET_BYTE(__addr, __i) (((char *)__addr)[__i])
#define __SET_BYTE(__addr, __i, __v) ((char *)__addr)[__i] = __v
static inline uint16_t get_unaligned_16_le(void *addr)
{
  return (uint16_t)((__GET_BYTE(addr, 1) << 8) | (__GET_BYTE(addr, 0)));
}

static inline void set_unaligned_16_le(void *a, uint16_t v)
{
  __SET_BYTE(a, 0, BYTE_EXTRACT(v, 1));
  __SET_BYTE(a, 1, BYTE_EXTRACT(v, 0));
}

static inline void set_unaligned_32_le(void *a, uint32_t v)
{
  __SET_BYTE(a, 0, BYTE_EXTRACT(v, 3));
  __SET_BYTE(a, 1, BYTE_EXTRACT(v, 2));
  __SET_BYTE(a, 2, BYTE_EXTRACT(v, 1));
  __SET_BYTE(a, 3, BYTE_EXTRACT(v, 0));
}

