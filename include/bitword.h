#pragma once
#include <types.h>

#define BITS_PER_WORD_LOG 6
#define BITS_PER_WORD (1<<BITS_PER_WORD_LOG)

static inline int bitword_set_next_free(uint64_t *ptr, int end_bit)
{
  int i;
  char b = *ptr;

  for (i = 0; i < end_bit; ++i) {
    if ((b & (1<<i)) == 0) {
      *ptr = b | (1 << i);
      return i;
    }
  }
  return end_bit;
}

/*
 * bitword_clear_bit - clears bit in the bitword
 */
static inline void bitword_clear_bit(uint64_t *ptr, int bit)
{
  *ptr &= ~(((uint64_t)1) << bit);
}

static inline bool bitword_bit_is_set(uint64_t *ptr, int bit)
{
  return *ptr & (1 << bit) ? true : false;
}
