#include "atmega8.h"

typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

extern void delay_cycles_16(uint16_t num_cycles);
extern void countdown_32(uint16_t count_hi, uint16_t count_low);
#define LOW16(v) (v & 0xffff)
#define HIGH16(v) ((v >> 16) & 0xffff)
#define ARG_SPLIT_32(v) HIGH16(v), LOW16(v)

#define wait_usec(v) countdown_32()

#define PIN_MODE_OUT(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_MODE_IN(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_ON(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

#define PIN_OFF(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

#define PIN_PULL_UP(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

#define PIN_PULL_OFF(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

void __attribute__((optimize("O2"))) blink() 
{
  char *blinker = (char *)0x100;
  if (*blinker == 1) {
    *blinker = 0;
    PIN_ON(B, 0);
  } else {
    *blinker = 1;
    PIN_OFF(B, 0);
  }
}

#define MSEC_PER_SEC 1000
#define FCPU 1000000
#define COUNT_PER_SEC (FCPU / 5)
#define COUNT_PER_MSEC (COUNT_PER_SEC / MSEC_PER_SEC)

// 1 second  = 200 000
// 1 milli   = 200 000 / 1000 = 200
// 100 milli = (200 000 / 1000) * 100 = 200 * 100 = 20000
#define MSEC_TO_COUNT(v) (v * COUNT_PER_MSEC)
#define SEC_TO_COUNT(v) (v * COUNT_PER_SEC)

void wait_1_sec()
{
  countdown_32(ARG_SPLIT_32(SEC_TO_COUNT(1)));
}

void wait_3_sec()
{
  countdown_32(ARG_SPLIT_32(SEC_TO_COUNT(3)));
}

void wait_100_msec()
{
  countdown_32(ARG_SPLIT_32(MSEC_TO_COUNT(100)));
}

void wait_1_msec()
{
  countdown_32(ARG_SPLIT_32(MSEC_TO_COUNT(1)));
}

int __attribute__((noinline)) get_int(int x)
{
  return x+2;
}

extern int twi_get_status(void);
extern void twi_master_init(void);
extern void twi_master_deinit(void);
extern void twi_master_start(void);

void main() 
{
  int i;
  char status;
  *(char*)0x100 = 0;
  PIN_MODE_OUT(B, 0);
  PIN_MODE_OUT(C, 5);
  PIN_MODE_OUT(D, 7);
  PIN_OFF(C, 5);
  PIN_ON(D, 7);
  for (i = 0; i < 20; ++i) {
    PIN_ON(B, 0);
    wait_100_msec();
    PIN_OFF(B, 0);
    wait_100_msec();
  }
  while(1) {
    PIN_ON(B, 0);
    twi_master_init();
    wait_3_sec();
    twi_master_start();
    status = twi_get_status();
    if (status != 8) {
      PIN_OFF(D, 7);
      while(1);
    }
    wait_3_sec();
    PIN_OFF(B, 0);
    twi_master_deinit();
    wait_3_sec();
  }
}

