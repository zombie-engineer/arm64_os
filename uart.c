#include "uart.h"
#include "gpio.h"
#include "mbox.h"

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

#define UART0_BASE  (MMIO_BASE + 0x00201000)
#define UART0_DR    ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD  ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD  ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH  ((volatile unsigned int*)(UART0_BASE + 0x2c))
#define UART0_CR    ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_IMSC  ((volatile unsigned int*)(UART0_BASE + 0x38))
#define UART0_ICR   ((volatile unsigned int*)(UART0_BASE + 0x44))


void uart_init_simple()
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

/* UART0 enable */
#define UART0_CR_UARTEN (1)

/* UART0 Transmit enable */
#define UART0_CR_TXE (1 << 8)

/* UART0 Recieve enable */
#define UART0_CR_RXE (1 << 9)

/* UART0 Work length */
#define UART0_LCRH_WLEN(x) (x<<5)
void uart_init()
{
  register unsigned int r;
  *UART0_CR = 0; // turn off UART0

  /* set up clock for consistent divisor values */
  mbox[0] = 9 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_SETCLKRATE;
  mbox[3] = 12;
  mbox[4] = 8;
  mbox[5] = 2;       // UART clock
  mbox[6] = 4000000; // 4Mhz
  mbox[7] = 0;       // clear turbo
  mbox[8] = MBOX_TAG_LAST;
  mbox_call(MBOX_CH_PROP);

  /* map UART0 to GPIO pins */
  r = *GPFSEL1;
  r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
  r |= (4 << 12) | (4 << 15);    // alt0
  *GPFSEL1 = r;
  *GPPUD = 0;
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *GPPUDCLK0 = (1<<14|1<<15);
  r = 150;
  while(r--) { asm volatile("nop"); } 
  *GPPUDCLK0 = 0;

  *UART0_ICR = 0x7ff;      // clear interrupts
  *UART0_IBRD = 2;         // 115200 baud
  *UART0_FBRD = 0xb;       // 
  // Word length is 8 bits
  *UART0_LCRH =UART0_LCRH_WLEN(3); 
  // enable Tx, Rx, FIFO
  *UART0_CR = UART0_CR_UARTEN | UART0_CR_TXE | UART0_CR_RXE;
  // 0000 0010 <> 1011 - 0x0002 0xb
}

void uart1_send(unsigned int c)
{
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x20)); 
  *AUX_MU_IO = c;
}

/* Recieve FIFO empty */
#define UART0_FR_RXFE (1<<4)

/* Transmit FIFO full*/
#define UART0_FR_TXFF (1<<5)

void uart_send(unsigned int c)
{
  do { asm volatile("nop"); } while(*UART0_FR & UART0_FR_TXFF); 
  *UART0_DR = c;
}

char uart1_getc()
{
  char r;
  do { asm volatile("nop"); } while(!(*AUX_MU_LSR & 0x1)); 
  r = (char)*AUX_MU_IO;
  return r == '\r' ? '\n' : r;
}

char uart_getc()
{
  char r;
  do { asm volatile("nop"); } while(*UART0_FR & UART0_FR_RXFE); 
  r = (char)(*UART0_DR);
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
