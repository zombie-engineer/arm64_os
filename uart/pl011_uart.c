#include <uart/pl011_uart.h>
#include <spinlock.h>
#include <gpio.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <types.h>
#include <intr_ctl.h>
#include <vcanvas.h>
#include <common.h>
#include <stringlib.h>
#include <ringbuf.h>
#include <compiler.h>
#include <arch/armv8/armv8.h>


#define UART0_BASE  (PERIPHERAL_BASE_PHY + 0x00201000)
#define UART0_DR    ((reg32_t)(UART0_BASE + 0x00))
#define UART0_FR    ((reg32_t)(UART0_BASE + 0x18))
#define UART0_IBRD  ((reg32_t)(UART0_BASE + 0x24))
#define UART0_FBRD  ((reg32_t)(UART0_BASE + 0x28))
#define UART0_LCRH  ((reg32_t)(UART0_BASE + 0x2c))
#define UART0_CR    ((reg32_t)(UART0_BASE + 0x30))
// Interrupt mask set clear
#define UART0_IMSC  ((reg32_t)(UART0_BASE + 0x38))
// Raw interrupt status
#define UART0_RIS   ((reg32_t)(UART0_BASE + 0x3c))
// Masked interrupt status
#define UART0_MIS   ((reg32_t)(UART0_BASE + 0x40))
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


static char pl011_rx_buf[2048];
static int pl011_rx_data_sz = 0;

ringbuf_t uart_pipe;
aligned(64) uint64_t uart_pipe_lock;

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

void pl011_uart_init(int baudrate, int _not_used)
{
  uint32_t hz, idiv, fdiv;
  pl011_uart_disable();
  while(read_reg(UART0_FR) & (UART0_FR_BUSY | UART0_FR_TXFF));
  pl011_uart_gpio_enable(14 /*tx pin*/, 15 /*rx pin*/);

  /* set up clock for consistent divisor values */
  mbox_get_clock_rate(MBOX_CLOCK_ID_UART, &hz);

  /* Clear all interrupts */
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

#define UART0_INT_BIT_CTS (1<<1)
#define UART0_INT_BIT_RX  (1<<4)
#define UART0_INT_BIT_TX  (1<<5)
#define UART0_INT_BIT_RT  (1<<6)
#define UART0_INT_BIT_FE  (1<<7)
#define UART0_INT_BIT_PE  (1<<8)
#define UART0_INT_BIT_BE  (1<<9)
#define UART0_INT_BIT_OE  (1<<10)

static int cx = 60;
static int cy = 0;

static inline void pl011_rx_buf_init()
{
  pl011_rx_data_sz = 0;
}

static inline void pl011_rx_buf_putchar(char c)
{
  if (pl011_rx_data_sz == sizeof(pl011_rx_buf))
    pl011_rx_data_sz--;

  pl011_rx_buf[pl011_rx_data_sz++] = c;
  asm volatile("dsb sy" :::"memory");
}

static inline char pl011_rx_buf_getchar()
{
  char c;
  while(1) {
    // disable_irq();
    asm volatile("dsb sy" ::: "memory");
    if (pl011_rx_data_sz) {
      c = pl011_rx_buf[--pl011_rx_data_sz];
      // enable_irq();
      break;
    }
    // enable_irq();
  }

  return c;
}

void pl011_uart_handle_interrupt()
{
  int c;
  int ris_value = read_reg(UART0_RIS); 
  cx = 60;
  cy = 2;
  if (ris_value & UART0_INT_BIT_RX) {
    cx = 65;
    // vcanvas_puts(&cx, &cy, "RX");
    c = read_reg(UART0_DR);
    pl011_rx_buf_putchar(c);

    /* echo uart back for debug */
    pl011_uart_send(c);
    //vcanvas_putc(&cx, &cy, c);
    ris_value &= ~UART0_INT_BIT_RX;
  }
  if (ris_value & UART0_INT_BIT_TX) {
    cx = 64;
    // vcanvas_puts(&cx, &cy, "TX");
    ris_value &= ~UART0_INT_BIT_TX;
  }
  if (ris_value & UART0_INT_BIT_RT) {
    // vcanvas_puts(&cx, &cy, "RT");
    ris_value &= ~UART0_INT_BIT_RT;
  }
  if (ris_value & UART0_INT_BIT_FE) {
    // vcanvas_puts(&cx, &cy, "FE");
    ris_value &= ~UART0_INT_BIT_FE;
  }
  if (ris_value & UART0_INT_BIT_PE) {
    // vcanvas_puts(&cx, &cy, "PE");
    ris_value &= ~UART0_INT_BIT_PE;
  }
  if (ris_value & UART0_INT_BIT_BE) {
    // vcanvas_puts(&cx, &cy, "BE");
    ris_value &= ~UART0_INT_BIT_BE;
  }
  if (ris_value & UART0_INT_BIT_OE) {
    // vcanvas_puts(&cx, &cy, "OE");
    ris_value &= ~UART0_INT_BIT_OE;
  }
  write_reg(UART0_ICR, 0x7ff);
}

void pl011_uart_print_regs()
{
  printf("pl011_uart registers: ris: %08x, mis: %08x, imsc: %08x\n",
      read_reg(UART0_RIS),
      read_reg(UART0_MIS),
      read_reg(UART0_IMSC)
      );
}

static void pl011_uart_reprogram_cr_prep()
{
  pl011_uart_disable();
  while(read_reg(UART0_FR) & (UART0_FR_BUSY | UART0_FR_TXFF));
  write_reg(UART0_LCRH, read_reg(UART0_LCRH) & ~(1<<4));
}

void pl011_uart_set_interrupt_mode()
{
  pl011_rx_buf_init();

  intr_ctl_set_cb(INTR_CTL_IRQ_TYPE_GPU, INTR_CTL_IRQ_GPU_UART0, 
      pl011_uart_handle_interrupt);

  pl011_uart_reprogram_cr_prep();

  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_UART0);

  pl011_uart_gpio_enable(14 /*tx pin*/, 15 /*rx pin*/);
  /* Clear pending interrupts */
  write_reg(UART0_ICR, 0x7ff);
  /* Unmask all interrupts */
  write_reg(UART0_IMSC, 0x7ff);
  write_reg(UART0_CR, UART0_CR_UARTEN | UART0_CR_TXE | UART0_CR_RXE);
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

// static void pl011_copy_to_pipe(const char *buf, int sz)
// {
//  int to_copy;
//  int copied = 0;
//  while(sz != copied) {
//    spinlock_lock(&pipe_lock);
//    pipe_free = sizeof(pipe_buf) - pipe_num_bytes;
//    to_copy = min(pipe_free, sz - copied);
//
//    memcpy(pipe_buf + pipe_sz, buf + copied, to_copy);
//    pipe_sz += to_copy;
//    copied = to_copy;
//
//    spinlock_unlock(&pipe_lock);
//  }
// }

static void pl011_io_thread_work()
{
  char c;
  int num;
  c = pl011_rx_buf_getchar();
  while(1) {
    spinlock_lock(&uart_pipe_lock);
    num = ringbuf_write(&uart_pipe, &c, 1);
    spinlock_unlock(&uart_pipe_lock);
    if (num)
      break;
  }
}

static char uart_pipe_buf[2048];

int pl011_io_thread(void)
{
  int ret = 0;
  spinlock_init(&uart_pipe_lock);
  ringbuf_init(&uart_pipe, uart_pipe_buf, sizeof(uart_pipe_buf));

  pl011_uart_set_interrupt_mode();
  while(1) {
    asm volatile("wfe");
    pl011_io_thread_work();
  }
  return ret;
}
