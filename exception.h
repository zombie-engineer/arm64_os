#pragma once

static inline void generate_exception()
{
  *(volatile long int*)(0xffffffffffffffff) = 1;
}
