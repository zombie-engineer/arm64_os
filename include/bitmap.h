#pragma once
#include <bitword.h>

/*
 * clear bit in bitmap
 */
static inline void bitmap_clear(void *bitmap, int bit_idx)
{
  uint64_t *ptr = ((uint64_t*)bitmap) + (bit_idx >> BITS_PER_WORD_LOG);
  bitword_clear_bit(ptr, bit_idx % BITS_PER_WORD);
}

/*
 * bitmap_set_next_free - looks for first bit that is not set, sets it
 * and returns it's index. If no bits were clear, returns end_idx
 */
int bitmap_set_next_free(void *bitmap, int end_idx)
{
  uint64_t *current_word = bitmap;
  uint64_t *end_word = current_word + (end_idx >> BITS_PER_WORD_LOG);
  int bit;

  do {
    bit = bitword_set_next_free(current_word, BITS_PER_WORD);
    if (bit <= (BITS_PER_WORD - 1))
      goto out_result;
    current_word++;
  } while(current_word != end_word);

  if (end_idx % BITS_PER_WORD) {
    bit = bitword_set_next_free(current_word, end_idx % BITS_PER_WORD);
    if (bit <= (end_idx % BITS_PER_WORD))
      goto out_result;
  }

  return end_idx;

out_result:
  return ((current_word - (uint64_t *)bitmap) << BITS_PER_WORD_LOG) + bit;
}

bool bitmap_bit_is_set(void *bitmap, int idx)
{
  uint64_t *word = ((uint64_t*)bitmap) + (idx >> BITS_PER_WORD_LOG);
  return bitword_bit_is_set(word, idx % BITS_PER_WORD);
}

