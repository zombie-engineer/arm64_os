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

#define PIN_MODE_OUT(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_MODE_IN(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_ON(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

#define PIN_OFF(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

void __attribute__((optimize("O2"))) blink() 
{
  char *blinker = 0x100;
  if (*blinker == 1) {
    *blinker = 0;
    PIN_ON(B, 0);
  } else {
    *blinker = 1;
    PIN_OFF(B, 0);
  }
//  delay_cycles_16(1000);
//  delay_cycles_16(1000);
}

void main() 
{
  *(char*)0x100 = 0;
  PIN_MODE_OUT(B, 0);
  while(1) {
    countdown_32(ARG_SPLIT_32(200000));
    blink();
  }
}

