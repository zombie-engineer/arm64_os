#include <checksum.h>

uint32_t checksum_basic(const char *buf, int size, uint32_t initval)
{
  int i;
  uint32_t result = initval;
  for (i = 0; i < size; ++i)
    result += buf[i];
  return result;
}

