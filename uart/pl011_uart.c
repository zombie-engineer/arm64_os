#include <uart/pl011_uart.h>
#include <spinlock.h>
#include <gpio.h>
#include <gpio_set.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <types.h>
#include <intr_ctl.h>
#include <board/bcm2835/bcm2835_irq.h>
#include <irq.h>
#include <vcanvas.h>
#include <common.h>
#include <stringlib.h>
#include <ringbuf.h>
#include <compiler.h>
#include <arch/armv8/armv8.h>
#include <cpu.h>
#include <debug.h>
#include <error.h>
#include <barriers.h>


#define PL011_UART_BASE  (PERIPHERAL_BASE_PHY + 0x00201000)
#define PL011_UARTDR    ((reg32_t)(PL011_UART_BASE + 0x00))
#define PL011_UARTFR    ((reg32_t)(PL011_UART_BASE + 0x18))
#define PL011_UARTIBRD  ((reg32_t)(PL011_UART_BASE + 0x24))
#define PL011_UARTFBRD  ((reg32_t)(PL011_UART_BASE + 0x28))
#define PL011_UARTLCR_H  ((reg32_t)(PL011_UART_BASE + 0x2c))
#define PL011_UARTCR    ((reg32_t)(PL011_UART_BASE + 0x30))
// Interrupt mask set clear
#define PL011_UARTIMSC  ((reg32_t)(PL011_UART_BASE + 0x38))
// Raw interrupt status
#define PL011_UARTRIS   ((reg32_t)(PL011_UART_BASE + 0x3c))
// Masked interrupt status
#define PL011_UARTMIS   ((reg32_t)(PL011_UART_BASE + 0x40))
#define PL011_UARTICR   ((reg32_t)(PL011_UART_BASE + 0x44))


/* UART0 enable */
#define PL011_UARTCR_UARTEN (1)

/* UART0 Transmit enable */
#define PL011_UARTCR_TXE (1 << 8)

/* UART0 Recieve enable */
#define PL011_UARTCR_RXE (1 << 9)

/* UART0 Work length */
#define PL011_UARTLCR_H_WLEN_5BITS (0b00 << 5)
#define PL011_UARTLCR_H_WLEN_6BITS (0b01 << 5)
#define PL011_UARTLCR_H_WLEN_7BITS (0b10 << 5)
#define PL011_UARTLCR_H_WLEN_8BITS (0b11 << 5)

/* UART is transmitting */
#define PL011_UARTFR_BUSY (1<<3)

/* Recieve FIFO empty */
#define PL011_UARTFR_RXFE (1<<4)

/* Transmit FIFO full*/
#define PL011_UARTFR_TXFF (1<<5)

DECL_GPIO_SET_KEY(pl011_gpio_set_key, "PL011_GPIO_SET_");

static char pl011_rx_buf[2048];
static int pl011_rx_data_sz = 0;
static int pl011_uart_initialized = 0;

static void pl011_uart_disable()
{
  write_reg(PL011_UARTCR, 0);
}


static int pl011_uart_gpio_init()
{
  const int tx_pin = 14;
  const int rx_pin = 15;
  gpio_set_handle_t gpio_set_handle;
  gpio_set_handle = gpio_set_request_2_pins(tx_pin, rx_pin, pl011_gpio_set_key);
  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
    printf("Failed to request gpio pins 14, 15 for pl011_uart.\n");
    return ERR_BUSY;
  }

  /* map UART0 to GPIO pins */
  gpio_set_function(tx_pin, GPIO_FUNC_ALT_0);
  gpio_set_function(rx_pin, GPIO_FUNC_ALT_0);

  gpio_set_pullupdown(rx_pin, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  return ERR_OK;
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
  while(read_reg(PL011_UARTFR) & (PL011_UARTFR_BUSY | PL011_UARTFR_TXFF));
  pl011_uart_gpio_init();

  /* set up clock for consistent divisor values */
  mbox_get_clock_rate(MBOX_CLOCK_ID_UART, &hz);
  if (hz != 48000000)
    debug_event_1();

  /* Clear all interrupts */
  write_reg(PL011_UARTICR, 0x7ff);

  /* IBRD - interger part of divisor
   * FBRD - floating-point part of divisor
   * divisor = IBRD.FBRD
   */
  pl011_calc_divisor(baudrate, hz, &idiv, &fdiv);

  write_reg(PL011_UARTIBRD, idiv);
  write_reg(PL011_UARTFBRD, fdiv);

  // Word length is 8 bits
  write_reg(PL011_UARTLCR_H, PL011_UARTLCR_H_WLEN_8BITS);
  // enable Tx, Rx, FIFO
  write_reg(PL011_UARTCR, PL011_UARTCR_UARTEN | PL011_UARTCR_TXE | PL011_UARTCR_RXE);
  // 0000 0010 <> 1011 - 0x0002 0xb
}

/*
 * Interrupts according to
 * PrimeCell UART (PL011) Technical Reference Manual
 */

/* change in nUARTCTS modem status line */
#define PL011_UARTCTSINTR (1<<1)

/* receive interrupt - recieve buffer is not empty */
#define PL011_UARTRXINTR  (1<<4)

/* transmit interrupt - transfer buffer is empty */
#define PL011_UARTTXINTR  (1<<5)

/* recieve timeout interrupt */
#define PL011_UARTRTINTR  (1<<6)

/* framing error in a received character interrupt */
#define PL011_UARTFEINTR  (1<<7)

/* parity error in a received character interrupt */
#define PL011_UARTPEINTR  (1<<8)

/* break error interrupt - break in reception */
#define PL011_UARTBEINTR  (1<<9)

/* overrun error interrupt */
#define PL011_UARTOEINTR  (1<<10)

static inline void pl011_rx_buf_init()
{
  pl011_rx_data_sz = 0;
}

static inline void pl011_rx_buf_putchar(char c)
{
  if (pl011_rx_data_sz == sizeof(pl011_rx_buf))
    return;

  pl011_rx_buf[pl011_rx_data_sz] = c;
  pl011_rx_data_sz++;
}

static inline char pl011_rx_buf_getchar()
{
  char c;
  while(1) {
    disable_irq();
    SYNC_BARRIER;
    if (pl011_rx_data_sz) {
      SYNC_BARRIER;
      --pl011_rx_data_sz;
      c = pl011_rx_buf[pl011_rx_data_sz];
      enable_irq();
      break;
    }
    enable_irq();
    WAIT_FOR_EVENT;
  }

  return c;
}

static void pl011_uart_print_int_status()
{
  int n;
  char buf[128];
  int ris; /* raw interrupt status */
  int mis; /* masked interrupt status */
  ris = read_reg(PL011_UARTRIS);
  mis = read_reg(PL011_UARTRIS);
  n = snprintf(buf, sizeof(buf), "MIS: %08x, RIS: %08x\n", mis, ris);
  pl011_uart_send_buf(buf, n);
}

static int pl011_log_level = 2;

static void pl011_uart_debug_interrupt()
{
  if (pl011_log_level > 0)
    debug_event_1();
  if (pl011_log_level > 1)
    pl011_uart_print_int_status();
}

static void pl011_uart_handle_interrupt()
{
  int c;
  int mis; /* masked interrupt status */
  // blink_led(20, 100);

  pl011_uart_debug_interrupt();
  mis = read_reg(PL011_UARTMIS);

  DATA_BARRIER;

  if (mis & PL011_UARTRXINTR) {
    c = read_reg(PL011_UARTDR);
    pl011_rx_buf_putchar(c);
    write_reg(PL011_UARTICR, PL011_UARTRXINTR);
  }
  else if (mis & PL011_UARTTXINTR) {
    pl011_uart_send('T');
    write_reg(PL011_UARTICR, PL011_UARTTXINTR);
  }
  else if (mis & PL011_UARTRTINTR) {
    pl011_uart_send('R');
    write_reg(PL011_UARTICR, PL011_UARTRTINTR);
  }
  else if (mis & PL011_UARTFEINTR) {
    pl011_uart_send('F');
    write_reg(PL011_UARTICR, PL011_UARTFEINTR);
  }
  else if (mis & PL011_UARTPEINTR) {
    pl011_uart_send('P');
    write_reg(PL011_UARTICR, PL011_UARTPEINTR);
  }
  else if (mis & PL011_UARTBEINTR) {
    pl011_uart_send('B');
    write_reg(PL011_UARTICR, PL011_UARTBEINTR);
  }
  else if (mis & PL011_UARTOEINTR) {
    pl011_uart_send('O');
    write_reg(PL011_UARTICR, PL011_UARTOEINTR);
  }
}

void pl011_uart_print_regs()
{
  printf("pl011_uart registers: ris: %08x, mis: %08x, imsc: %08x\n",
      read_reg(PL011_UARTRIS),
      read_reg(PL011_UARTMIS),
      read_reg(PL011_UARTIMSC)
      );
}

void pl011_uart_set_interrupt_mode()
{
  int interrupt_mask;
  pl011_rx_buf_init();

  intr_ctl_set_cb(INTR_CTL_IRQ_TYPE_GPU, INTR_CTL_IRQ_GPU_UART0,
      pl011_uart_handle_interrupt);

  printf("pl011_uart_set_interrupt_mode"__endline);

  intr_ctl_gpu_irq_enable(INTR_CTL_IRQ_GPU_UART0);
  irq_set(0, ARM_IRQ_UART, pl011_uart_handle_interrupt);

  // pl011_uart_gpio_enable(14 /*tx pin*/, 15 /*rx pin*/);

  /* Clear pending interrupts */
  write_reg(PL011_UARTICR, 0x7ff);
  /* Unmask all interrupts */

  interrupt_mask =
    PL011_UARTRXINTR /* |
    PL011_UARTTXINTR  |
    PL011_UARTRTINTR  |
    PL011_UARTFEINTR  |
    PL011_UARTPEINTR  |
    PL011_UARTBEINTR  |
    PL011_UARTOEINTR*/;
  write_reg(PL011_UARTIMSC, interrupt_mask);
  write_reg(PL011_UARTCR, PL011_UARTCR_UARTEN | PL011_UARTCR_TXE | PL011_UARTCR_RXE);
}

extern int pl011_uart_putchar(uint8_t c);

void pl011_uart_send(unsigned int c)
{
  pl011_uart_putchar(c & 0xff);
}

int pl011_uart_send_buf(const void *buf, size_t n)
{
  int i;
  const uint8_t *b = (const uint8_t *)buf;
  for (i = 0; i < n; ++i) {
    while(read_reg(PL011_UARTFR) & PL011_UARTFR_TXFF);
    write_reg(PL011_UARTDR, b[i]);
  }
  return n;
}

char pl011_uart_getc()
{
  while(read_reg(PL011_UARTFR) & PL011_UARTFR_RXFE);
  return (char)read_reg(PL011_UARTDR);
}

typedef struct rx_subscriber {
  uart_rx_event_cb cb;
  void * cb_arg;
} rx_subscriber_t;

static rx_subscriber_t rx_subscribers[8];
static ALIGNED(64) uint64_t rx_subscribers_lock;

static inline void rx_subscribers_init()
{
  spinlock_init(&rx_subscribers_lock);
  memset(rx_subscribers, 0, sizeof(rx_subscribers));
}

static inline int rx_subscriber_slot_is_free(int slot)
{
  return rx_subscribers[slot].cb == 0 ? 1 : 0;
}

static inline void rx_subscriber_slot_occupy(int slot, uart_rx_event_cb cb, void *cb_arg)
{
   rx_subscribers[slot].cb = cb;
   rx_subscribers[slot].cb_arg = cb_arg;
}

static inline int rx_subscriber_is_valid(int slot)
{
  return rx_subscribers[slot].cb ? 1 : 0;
}

int pl011_uart_subscribe_to_rx_event(uart_rx_event_cb cb, void *cb_arg)
{
  int i;
  spinlock_lock(&rx_subscribers_lock);
  for (i = 0; i < ARRAY_SIZE(rx_subscribers); ++i) {
    if (rx_subscriber_slot_is_free(i)) {
      rx_subscriber_slot_occupy(i, cb, cb_arg);
      break;
    }
  }
  spinlock_unlock(&rx_subscribers_lock);
  return ERR_OK;
}

int pl011_io_thread(void)
{
  // char c;
  // int i;
  // rx_subscriber_t *subscriber;

  rx_subscribers_init();
  pl011_uart_initialized = 1;
  disable_irq();
  pl011_uart_set_interrupt_mode();
  uart_puts_blocking("interrupt mode set");
  enable_irq();
  // DATA_BARRIER;
  while(1) {
    asm volatile("wfe");
    // c = pl011_rx_buf_getchar();
    // putc(c);
 //    for (i = 0; i < ARRAY_SIZE(rx_subscribers); ++i) {
 //      if (rx_subscriber_is_valid(i)) {
 //        subscriber = &rx_subscribers[i];
 //        subscriber->cb(subscriber->cb_arg, c);
 //        // debug_event_1();
 //      }
 //    }
  }
  return ERR_OK;
}

int uart_is_initialized()
{
  int ret;
  ret = pl011_uart_initialized;
  DATA_BARRIER;
  return ret;
}

int uart_putc_blocking(char c)
{
  int ret;
  int irqflags;
  disable_irq_save_flags(irqflags);
  ret = pl011_uart_putchar(c);
  restore_irq_flags(irqflags);
  return ret;
}

int uart_puts_blocking(const char *c)
{
  int ret;
  int irqflags;
  disable_irq_save_flags(irqflags);
  while(*c)
    ret = pl011_uart_putchar(*c++);
  restore_irq_flags(irqflags);
  return ret;
}
