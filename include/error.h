#pragma once
#include <types.h>

#define ERR_OK               0
#define ERR_INVAL_ARG       -1
#define ERR_NOT_INIT        -2
#define ERR_NOT_IMPLEMENTED -3
#define ERR_FATAL           -4
#define ERR_MEMALLOC        -5
#define ERR_NO_RESOURCE     -6
#define ERR_NOT_FOUND       -7
#define ERR_BUSY            -8
#define ERR_GENERIC         -9
#define ERR_ALIGN          -10
#define ERR_TIMEOUT        -11
#define ERR_RETRY          -12
#define MAX_ERRNO          128

static inline void *ERR_PTR(int64_t err) 
{
  return (void *)err;
}

static inline int IS_ERR(const void *ptr)
{
  return (uint64_t)ptr >= (uint64_t)(-MAX_ERRNO);
}

static inline int64_t PTR_ERR(const void *ptr)
{
  return (int64_t)ptr;
}
