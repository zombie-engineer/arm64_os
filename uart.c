#include "uart.h"
#include "gpio.h"

#define AUX_BASE        (MMIO_BASE + 0x00215000)

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(AUX_BASE + 0x04))
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_BASE + 0x40))
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_BASE + 0x44))
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_BASE + 0x48))
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_BASE + 0x4C))
#define AUX_MU_MCR      ((volatile unsigned int*)(AUX_BASE + 0x50))
#define AUX_MU_LSR      ((volatile unsigned int*)(AUX_BASE + 0x54))
#define AUX_MU_MSR      ((volatile unsigned int*)(AUX_BASE + 0x58))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(AUX_BASE + 0x5C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(AUX_BASE + 0x60))
#define AUX_MU_STAT     ((volatile unsigned int*)(AUX_BASE + 0x64))
#define AUX_MU_BAUD     ((volatile unsigned int*)(AUX_BASE + 0x68))

void uart_init()
{
  register unsigned int r;
  *AUX_ENABLE |= 1;
  *AUX_MU_CNTL = 0;
  *AUX_MU_LCR  = 3;
  *AUX_MU_MCR  = 0;
  *AUX_MU_IER  = 0;
  *AUX_MU_IIR  = 0xc6;
  *AUX_MU_BAUD = 270;

  r = *GPFSEL1;
  // pins gpio14, gpio15
  r &= ~(7<<12|7<<15);
  r |= (2<<12|2<<15);
  *GPFSEL1 = r;
  *GPPUD = 0;
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *GPPUDCLK0 = (1<<14|1<<15);
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *GPPUDCLK0 = 0;
  *AUX_MU_CNTL = 3; // enable Tx,Rx
}

void uart_send(unsigned int c)
{
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x20)); 
  *AUX_MU_IO = c;
}

char uart_getc()
{
  char r;
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x1)); 
  r = (char)*AUX_MU_IO;
  return r == '\r' ? '\n' : r;
}

void uart_puts(char *s)
{
  while(*s)
  {
    if (*s == '\n')
      uart_send('\r');
    uart_send(*s++);
  }
}

void uart_hex(unsigned int d)
{
  unsigned int n;
  int c;
  for(c = 28; c >= 0; c -=4) {
    n = (d>>c) & 0xf;
    n += n > 9 ? 0x37 : 0x30;
    uart_send(n);
  }
}
