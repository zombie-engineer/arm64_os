#include "atmega8.h"
extern void delay_cycles(short num_cycles);

#define PIN_MODE_OUT(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_MODE_IN(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(DDR ## port), "i"(pin))

#define PIN_ON(port, pin) \
  asm volatile("sbi %0, %1\n"::"i"(PORT ## port), "i"(pin))

#define PIN_OFF(port, pin) \
  asm volatile("cbi %0, %1\n"::"i"(PORT ## port), "i"(pin))


void blink() 
{
  PIN_ON(B, 0);
  delay_cycles(1000);
  PIN_OFF(B, 0);
  delay_cycles(1000);
}

void main() 
{
  PIN_MODE_OUT(B, 0);
  while(1) {
    delay_cycles(65000);
    blink();
  }
}

