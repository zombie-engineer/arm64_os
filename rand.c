#include "rand.h"
#include "gpio.h"

#define RNG_CTRL     ((volatile unsigned int*)(MMIO_BASE + 0x00104000))
#define RNG_STATUS   ((volatile unsigned int*)(MMIO_BASE + 0x00104004))
#define RNG_DATA     ((volatile unsigned int*)(MMIO_BASE + 0x00104008))
#define RNG_INT_MASK ((volatile unsigned int*)(MMIO_BASE + 0x00104010))

void rand_init()
{
  *RNG_STATUS = 0x40000;
  *RNG_INT_MASK |= 1;

  *RNG_CTRL |= 1;

  while(!((*RNG_STATUS)>>24)) asm volatile ("nop");
}

unsigned int rand(unsigned int min, unsigned int max)
{
  return *RNG_DATA % (max - min) + min;
}
