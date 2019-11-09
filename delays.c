#include <delays.h>
#include <gpio.h>
#include <types.h>
#include <arch/armv8/armv8.h>
#include <common.h>


#define SYSTMR_LO ((volatile unsigned int*)(MMIO_BASE + 0x00003004))
#define SYSTMR_HI ((volatile unsigned int*)(MMIO_BASE + 0x00003008))


void wait_cycles(unsigned int n)
{
  if (n) 
    while(n--);
}

void wait_usec(unsigned int n)
{
  register uint64_t now, counter, counter_target;
  register uint32_t counter_freq, counter_freq_milli_sec, wait_counts;

  counter = get_system_timer();
  counter_freq = get_system_timer_freq();
  wait_counts = (counter_freq / 1000000) * n;

  counter_target = counter + wait_counts;
  while(get_system_timer() < counter_target);
}

void wait_msec(unsigned int n)
{
  // get Hz (counts per second) 19200000
  register uint64_t now, counter, counter_target;
  register uint32_t counter_freq, counter_freq_milli_sec, wait_counts;

  counter = get_system_timer();
  counter_freq = get_system_timer_freq();
  wait_counts = (counter_freq / 1000) * n;

  counter_target = counter + wait_counts;
  while(get_system_timer() < counter_target);
}


//unsigned long get_system_timer()
//{
//  unsigned int h=-1, l;
//  // we must read MMIO area as two separate 32 bit reads
//  h=*SYSTMR_HI;
//  l=*SYSTMR_LO;
//  // we have to repeat it if high word changed during read
//  if(h!=*SYSTMR_HI) {
//    h=*SYSTMR_HI;
//    l=*SYSTMR_LO;
//  }
//  // compose long int value
//  return ((unsigned long) h << 32) | l;
//}

void wait_msec_st(unsigned int n)
{
  unsigned long t = get_system_timer();
  // we must check if it's non-zero, because qemu does not emulate
  // system timer, and returning constant zero would mean infinite loop
  if (t) 
    while (get_system_timer() < t + n);
}

void set_timer(unsigned int ms)
{

}
