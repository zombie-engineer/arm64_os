#include <uart/pl011_uart.h>
#include <gpio.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <types.h>


#define UART0_BASE  (PERIPHERAL_BASE_PHY + 0x00201000)
#define UART0_DR    ((reg32_t)(UART0_BASE + 0x00))
#define UART0_FR    ((reg32_t)(UART0_BASE + 0x18))
#define UART0_IBRD  ((reg32_t)(UART0_BASE + 0x24))
#define UART0_FBRD  ((reg32_t)(UART0_BASE + 0x28))
#define UART0_LCRH  ((reg32_t)(UART0_BASE + 0x2c))
#define UART0_CR    ((reg32_t)(UART0_BASE + 0x30))
#define UART0_IMSC  ((reg32_t)(UART0_BASE + 0x38))
#define UART0_ICR   ((reg32_t)(UART0_BASE + 0x44))


/* UART0 enable */
#define UART0_CR_UARTEN (1)

/* UART0 Transmit enable */
#define UART0_CR_TXE (1 << 8)

/* UART0 Recieve enable */
#define UART0_CR_RXE (1 << 9)

/* UART0 Work length */
#define UART0_LCRH_WLEN_5BITS (0b00 << 5)
#define UART0_LCRH_WLEN_6BITS (0b01 << 5)
#define UART0_LCRH_WLEN_7BITS (0b10 << 5)
#define UART0_LCRH_WLEN_8BITS (0b11 << 5)

/* UART is transmitting */
#define UART0_FR_BUSY (1<<3)

/* Recieve FIFO empty */
#define UART0_FR_RXFE (1<<4)

/* Transmit FIFO full*/
#define UART0_FR_TXFF (1<<5)


static void pl011_uart_disable()
{
  *UART0_CR = 0;
}

static void pl011_uart_gpio_enable(int tx_pin, int rx_pin)
{
  /* map UART0 to GPIO pins */
  gpio_set_function(tx_pin, GPIO_FUNC_ALT_0);
  gpio_set_function(rx_pin, GPIO_FUNC_ALT_0);

  // gpio_set_pullupdown(14, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(rx_pin, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
}

void pl011_uart_init(int baudrate, int unused)
{
  uint32_t hz;
  pl011_uart_disable();
  while(*UART0_FR & (UART0_FR_BUSY | UART0_FR_TXFF));
  pl011_uart_gpio_enable(14 /*tx pin*/, 15 /*rx pin*/);

  /* set up clock for consistent divisor values */
  hz = 4000000;
  mbox_set_clock_rate(MBOX_CLOCK_ID_UART, &hz, 0);

  *UART0_ICR = 0x7ff;      // clear interrupts
  *UART0_IBRD = 2;         // 115200 baud
  *UART0_FBRD = 0xb;       // 
  // Word length is 8 bits
  *UART0_LCRH = UART0_LCRH_WLEN_8BITS; 
  // enable Tx, Rx, FIFO
  *UART0_CR = UART0_CR_UARTEN | UART0_CR_TXE | UART0_CR_RXE;
  // 0000 0010 <> 1011 - 0x0002 0xb
}

void pl011_uart_send(unsigned int c)
{
  while(*UART0_FR & UART0_FR_TXFF); 
  *UART0_DR = c;
}

char pl011_uart_getc()
{
  while(*UART0_FR & UART0_FR_RXFE); 
  return (char)(*UART0_DR);
}
