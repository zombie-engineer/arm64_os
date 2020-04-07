#include <delays.h>
#include <gpio.h>
#include <arch/armv8/armv8.h>
#include <common.h>


#define SYSTMR_LO ((volatile unsigned int*)(PERIPHERAL_BASE_PHY + 0x00003004))
#define SYSTMR_HI ((volatile unsigned int*)(PERIPHERAL_BASE_PHY + 0x00003008))


void wait_cycles(uint32_t n)
{
  if (n) 
    while(n--);
}

void wait_usec(uint32_t n)
{
  register uint64_t counter, counter_target;
  register uint32_t counter_freq, wait_counts;

  counter = read_cpu_counter_64();
  counter_freq = get_cpu_counter_64_freq();
  wait_counts = (counter_freq / 1000000) * n;

  counter_target = counter + wait_counts;
  while(read_cpu_counter_64() < counter_target);
}

void wait_msec(uint32_t n)
{
  // get Hz (counts per second) 19200000
  register uint64_t counter, counter_target;
  register uint32_t counter_freq, wait_counts;

  counter = read_cpu_counter_64();
  counter_freq = get_cpu_counter_64_freq();
  wait_counts = (counter_freq / 1000) * n;

  counter_target = counter + wait_counts;
  while(read_cpu_counter_64() < counter_target);
}
