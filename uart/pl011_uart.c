#include <uart/pl011_uart.h>
#include <gpio.h>
#include <mbox/mbox.h>


#define UART0_BASE  (MMIO_BASE + 0x00201000)
#define UART0_DR    ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR    ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD  ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD  ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH  ((volatile unsigned int*)(UART0_BASE + 0x2c))
#define UART0_CR    ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_IMSC  ((volatile unsigned int*)(UART0_BASE + 0x38))
#define UART0_ICR   ((volatile unsigned int*)(UART0_BASE + 0x44))


/* UART0 enable */
#define UART0_CR_UARTEN (1)

/* UART0 Transmit enable */
#define UART0_CR_TXE (1 << 8)

/* UART0 Recieve enable */
#define UART0_CR_RXE (1 << 9)

/* UART0 Work length */
#define UART0_LCRH_WLEN(x) (x<<5)

/* Recieve FIFO empty */
#define UART0_FR_RXFE (1<<4)

/* Transmit FIFO full*/
#define UART0_FR_TXFF (1<<5)


void pl011_uart_init(int baudrate, int unused)
{
  *UART0_CR = 0; // turn off UART0

  /* set up clock for consistent divisor values */
  mbox[0] = 9 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_SET_CLOCK_RATE;
  mbox[3] = 12;
  mbox[4] = 8;
  mbox[5] = 2;       // UART clock
  mbox[6] = 4000000; // 4Mhz
  mbox[7] = 0;       // clear turbo
  mbox[8] = MBOX_TAG_LAST;
  mbox_call(MBOX_CH_PROP);

  /* map UART0 to GPIO pins */
  gpio_set_function(14, GPIO_FUNC_ALT_0);
  gpio_set_function(15, GPIO_FUNC_ALT_0);

  gpio_set_pullupdown(14, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(15, GPIO_PULLUPDOWN_NO_PULLUPDOWN);

  *UART0_ICR = 0x7ff;      // clear interrupts
  *UART0_IBRD = 2;         // 115200 baud
  *UART0_FBRD = 0xb;       // 
  // Word length is 8 bits
  *UART0_LCRH =UART0_LCRH_WLEN(3); 
  // enable Tx, Rx, FIFO
  *UART0_CR = UART0_CR_UARTEN | UART0_CR_TXE | UART0_CR_RXE;
  // 0000 0010 <> 1011 - 0x0002 0xb
}

void pl011_uart_send(unsigned int c)
{
  do { asm volatile("nop"); } while(*UART0_FR & UART0_FR_TXFF); 
  *UART0_DR = c;
}

char pl011_uart_getc()
{
  do { asm volatile("nop"); } while(*UART0_FR & UART0_FR_RXFE); 
  return (char)(*UART0_DR);
}
