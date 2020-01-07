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
  write_reg(UART0_CR, 0);
}

static void pl011_uart_gpio_enable(int tx_pin, int rx_pin)
{
  /* map UART0 to GPIO pins */
  gpio_set_function(tx_pin, GPIO_FUNC_ALT_0);
  gpio_set_function(rx_pin, GPIO_FUNC_ALT_0);

  gpio_set_pullupdown(rx_pin, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
}


static void pl011_calc_divisor(int baudrate, uint64_t clock_hz, uint32_t *idiv, uint32_t *fdiv)
{
  /* Calculate integral and fractional parts of divisor, using the formula
   * from the bcm2835 manual.
   * DIVISOR = CLOCK_HZ / (16 * BAUDRATE)
   * where DIVISOR is represented by idiv.fdiv floating point number,
   * where idiv is intergral part of DIVISOR and fdiv is fractional
   * part of DIVISOR
   * ex: 2.11 = 4 000 000 Hz / (16 * 115200), idiv = 2, fdiv = 11
   */

  uint64_t div;
  uint32_t i, f;
  /* Defaults for a 4Mhz clock */
  // *idiv = 2;
  // *fdiv = 11;

  /* Defaults for a 48Mhz clock */
  // *idiv = 26;
  // *fdiv = 0;

  div = clock_hz * 10 / (16 * baudrate);
  i = div / 10;
  f = div % 10;
  if (i > 0xffff) 
    return;

  if (f > 0b11111)
    f = 0b11111;

  *idiv = i;
  *fdiv = f;
}

void pl011_uart_init(int baudrate, int unused)
{
  uint32_t hz, idiv, fdiv;
  pl011_uart_disable();
  while(read_reg(UART0_FR) & (UART0_FR_BUSY | UART0_FR_TXFF));
  pl011_uart_gpio_enable(14 /*tx pin*/, 15 /*rx pin*/);

  /* set up clock for consistent divisor values */
  mbox_get_clock_rate(MBOX_CLOCK_ID_UART, &hz);

  /* mask all interrupts */
  write_reg(UART0_ICR, 0x7ff);

  /* IBRD - interger part of divisor
   * FBRD - floating-point part of divisor
   * divisor = IBRD.FBRD
   */
  pl011_calc_divisor(baudrate, hz, &idiv, &fdiv);

  write_reg(UART0_IBRD, idiv);
  write_reg(UART0_FBRD, fdiv);

  // Word length is 8 bits
  write_reg(UART0_LCRH, UART0_LCRH_WLEN_8BITS); 
  // enable Tx, Rx, FIFO
  write_reg(UART0_CR, UART0_CR_UARTEN | UART0_CR_TXE | UART0_CR_RXE);
  // 0000 0010 <> 1011 - 0x0002 0xb
}

void pl011_uart_send(unsigned int c)
{
  while(read_reg(UART0_FR) & UART0_FR_TXFF); 
  write_reg(UART0_DR, c);
}

void pl011_uart_send_buf(const char *buf, int n)
{
  int i;
  for (i = 0; i < n; ++i) {
    while(read_reg(UART0_FR) & UART0_FR_TXFF);
    write_reg(UART0_DR, buf[i]);
  }
}

char pl011_uart_getc()
{
  while(read_reg(UART0_FR) & UART0_FR_RXFE); 
  return (char)read_reg(UART0_DR);
}

