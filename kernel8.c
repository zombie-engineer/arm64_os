#include <config.h>
#include <clock_manager.h>
#include <irq.h>
#include <uart/uart.h>
#include <gpio.h>
#include <gpio_set.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <arch/armv8/armv8.h>
#include <avr_update.h>
#include <spinlock.h>
#include <i2c.h>
#include <font.h>
#include <vcanvas.h>
#include <rand.h>
#include <timer.h>
#include <delays.h>
#include <tags.h>
#include <mmu.h>
#include <common.h>
#include <stringlib.h>
#include <console.h>
#include <sched.h>
#include <intr_ctl.h>
#include <exception.h>
#include <cmdrunner.h>
#include <max7219.h>
#include <drivers/atmega8a.h>
#include <drivers/display/nokia5110.h>
#include <drivers/display/nokia5110_console.h>
#include <debug.h>
#include <unhandled_exception.h>
#include <board/bcm2835/bcm2835.h>
#include <board/bcm2835/bcm2835_irq.h>
#include <board/bcm2835/bcm2835_arm_timer.h>
#include <board/bcm2835/bcm2835_systimer.h>
#include <arch/armv8/armv8_generic_timer.h>
#include <init_task.h>
#include <memory/dma_area.h>

#include <cpu.h>
#include <list.h>
#include <self_test.h>
#include <pwm.h>
#include <drivers/servo/sg90.h>
#include <drivers/usb/usbd.h>

/*
 * This is a garbage can for all possible test code
 * injected straight into main function for fast
 * development and experiments.
 * Leave your litter in that file.
 */
#include <kernel_tests.h>

  // hexdump_addr(0x100);
  // 0x0000000000001122
  // PARange  2: 40 bits, 1TB 0x2
  // ASIDBits 2: 16 bits
  // BigEnd   1: Mixed-endian support
  // SNSMem   1: Secure versus Non-secure Memory
  // TGran16  0: 16KB granule not supported
  // TGran64  0: 64KB granule is supported
  // TGran4   0: 4KB granule is supported

#ifdef CHECK_PRINTF
void check_printf()
{
  printf("016d 10: %016d\n", 10);
  printf("-%4.3d\n", 1);
  printf("-%4.3x\n", 1);
  printf("0xf: %08x\n", 0xf);
  printf("0x7f: %8x\n", 0x7f);
  printf("0x3ff: %x\n", 0x3ff);
  printf("0x1fff: %x\n", 0x1fff);
  printf("0xef0f0: %x\n", 0xef0f0);
  printf("0xefffff: %x\n", 0xefffff);
  printf("016d -10: %016d\n", -10);

  printf("sizoef(long) = %ld\n", sizeof(long));
  printf("printf_test:\nlld: %lld,\nllu: %llu,\nllx: %llx,\nld: %ld,\nlu: %lu,\nlx: %lx,\nd: %d, u: %u, x: %x\n",
    0xffffffffffffffffll,
    0xffffffff66666666llu,
    0xffffffff55555555ll,

    0xffffffffl,
    0xffff3333lu,
    0xffff2222,

    (unsigned int)0xffffffff,
    (unsigned int)0xffffffff,
    (unsigned int)0xffffffff);

  printf("lld 0x7fffffffffffffff = %lld\n", 0x7fffffffffffffff);
  printf("lld 0xfffffffffffffffe = %lld\n", 0xfffffffffffffffe);
  printf("ld  0x7fffffffffffffff = %ld\n", 0x7fffffffffffffff);
  printf("ld  0xfffffffffffffffe = %ld\n", 0xfffffffffffffffe);
  printf("d   0x7fffffffffffffff = %d\n", 0x7fffffffffffffff);
  printf("d   0xfffffffffffffffe = %d\n", 0xfffffffffffffffe);

  printf("lld 0x8000000000000000 = %lld\n", 0x8000000000000000ll);
  printf("ld  0x80000000         = %ld\n" , 0x80000000l);
  printf("ld  0x80000000         = %d\n", 0x80000000);
}
#endif

extern uint64_t __aarch64_hcr_el2_init_value;

void print_cpu_info()
{
  int i;
  for (i = 0; i < 4; ++i)
    printf("Core %d: mpidr_el1: %016llx\n", i, armv8_get_mpidr_el1(i));
}

void print_current_ex_level()
{
  unsigned long el;
  asm volatile("mrs %0, CurrentEL; lsr %0, %0, #2" : "=r"(el));
  printf("Executing at EL%d\n", el);
  printf("HCR_EL2: %016lx (stored at %016lx)\n",
    __aarch64_hcr_el2_init_value,
    &__aarch64_hcr_el2_init_value);
}

void print_mbox_props()
{
  int val, val2, i;
  unsigned clock_rate;
  char buf[6];
  val = mbox_get_firmware_rev();
  printf("firmware rev:    %08x\n", val);
  val = mbox_get_board_model();
  printf("board model:     %08x\n", val);
  val = mbox_get_board_rev();
  printf("board rev:       %08x\n", val);
  val = mbox_get_board_serial();
  printf("board serial:    %08x\n", val);
  if (mbox_get_mac_addr(&buf[0], &buf[7]))
    printf("failed to get MAC addr\n");
  else
    printf("mac address:     %02x:%02x:%02x:%02x:%02x:%02x\n",
      (int)(buf[0]),
      (int)(buf[1]),
      (int)(buf[2]),
      (int)(buf[3]),
      (int)(buf[4]),
      (int)(buf[5]),
      (int)(buf[6]));

  if (mbox_get_arm_memory(&val, &val2))
    printf("failed to get arm memory\n");
  else {
    printf("arm memory base: %08x\n", val);
    printf("arm memory size: %08x\n", val2);
  }

  if (mbox_get_vc_memory(&val, &val2))
    printf("failed to get arm memory\n");
  else {
    printf("vc memory base:  %08x\n", val);
    printf("vc memory size:  %08x\n", val2);
  }
  for (i = 0; i < 8; ++i) {
    if (mbox_get_clock_rate(i, &clock_rate))
      printf("failed to get clock rate for clock_id %d\n", i);
    else
      printf("clock %d rate: %08x (%d KHz)\n", i, clock_rate, clock_rate / 1000);
  }

#define GET_DEVICE_POWER_STATE(x)\
  if (mbox_get_power_state(MBOX_DEVICE_ID_ ## x, (uint32_t*)&val, (uint32_t*)&val2))\
    puts("failed to get power state for device " #x "\r\n");\
  else\
    printf("power_state: " #x ": on:%d,exists:%d\r\n", val, val2);
  GET_DEVICE_POWER_STATE(SD);
  GET_DEVICE_POWER_STATE(UART0);
  GET_DEVICE_POWER_STATE(UART1);
  GET_DEVICE_POWER_STATE(USB);
  GET_DEVICE_POWER_STATE(I2C0);
  GET_DEVICE_POWER_STATE(I2C1 );
  GET_DEVICE_POWER_STATE(I2C2);
  GET_DEVICE_POWER_STATE(SPI);
  GET_DEVICE_POWER_STATE(CCP2TX);
}

void print_cache_stats_for_type(int cache_level, int cache_type)
{
  char cache_type_s = 'U';
  if (cache_type == CACHE_TYPE_I)
    cache_type_s = 'i';
  if (cache_type == CACHE_TYPE_D)
    cache_type_s = 'd';

  printf("[Memory model][L%d %c-cache] sets:%4d ways:%4d linesize:%4d\n",
    cache_level + 1,
    cache_type_s,
    mem_cache_get_num_sets(cache_level, cache_type),
    mem_cache_get_num_ways(cache_level, cache_type),
    mem_cache_get_line_size(cache_level, cache_type)
  );
}

void print_cache_stats()
{
  int cl, ct;
  printf("CPU mem features: max phys addr bits: %d, asid bits: %d, "
         "4k: %d, 16k: %d, 64k: %d, "
         "st2_4k: %d, st2_16k: %d, st2_64k: %d, "
         "\n",
    mem_model_max_pa_bits(),
    mem_model_num_asid_bits(),
    mem_model_4k_granule_support(),
    mem_model_16k_granule_support(),
    mem_model_64k_granule_support(),
    mem_model_st2_4k_granule_support(),
    mem_model_st2_16k_granule_support(),
    mem_model_st2_64k_granule_support()
  );
  for (ct = 0; ct <= CACHE_TYPE_MAX; ct++) {
    for (cl = 0; cl <= CACHE_LEVEL_MAX; cl++)
      print_cache_stats_for_type(cl, ct);
  }
}

void print_arm_features()
{
  unsigned long el;
  asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(el));
  printf("id_aa64mmfr0_el1 is: %08x\n", el);
}

static int current_pin1;
static int current_pin2;
static int cont;

typedef struct print_cpuctx_ctx {
  int nr;
} print_cpuctx_ctx_t;

//static int print_reg_cb(const char *reg_str, size_t reg_str_sz, void *cb_priv)
//{
//  print_cpuctx_ctx_t *print_ctx = (print_cpuctx_ctx_t *)cb_priv;
//  if (print_ctx->nr) {
//    if (print_ctx->nr % 4 == 0) {
//      uart_putc('\n');
//      uart_putc('\r');
//    } else {
//      uart_putc(',');
//      uart_putc(' ');
//    }
//  }
//  uart_puts(reg_str);
//  print_ctx->nr++;
//  return 0;
//}

// extern void *__current_cpuctx;
void gpio_handle_irq()
{
  uint64_t elr;
  asm volatile ("mrs   %0, elr_el1": "=r"(elr));
  // print_cpuctx_ctx_t print_ctx = { 0 };
  printf("basic: %08x, gpu1: %08x, gpu2: %08x, elr: %016llx\n",
      read_reg(0x3f00b200),
      read_reg(0x3f00b204),
      read_reg(0x3f00b208), elr);

  //printf("__current_cpuctx at %016llx pointing at %016llx\n" , &__current_cpuctx, __current_cpuctx);

  //cpuctx_print_regs(__current_cpuctx, print_reg_cb, &print_ctx);

  if (gpio_pin_status_triggered(current_pin1)) {
    gpio_pin_status_clear(current_pin1);
    if (gpio_is_set(current_pin1))
      putc('+');
    else
      putc('-');
  }
  if (gpio_pin_status_triggered(current_pin2)) {
    gpio_pin_status_clear(current_pin2);
    if (gpio_is_set(current_pin2))
      putc('+');
    else
      putc('-');
  }
  putc('\n');
}

static int sda_pin = 0;
static int scl_pin = 0;

void gpio_handle_i2c_irq()
{
  int gpio_val = read_reg(0x3f200034) & ((1<<sda_pin)|(1<<scl_pin));
  int gpio_trig = read_reg(0x3f200040) & ((1<<sda_pin)|(1<<scl_pin));
  int sda = (gpio_val & (1<< sda_pin)) ? 1: 0;
  int scl = (gpio_val & (1<< scl_pin)) ? 1: 0;
  int sda_trig = (gpio_trig & (1<< sda_pin)) ? 1: 0;
  int scl_trig = (gpio_trig & (1<< scl_pin)) ? 1: 0;
  write_reg(0x3f200040, (sda_trig << sda_pin)|(scl_trig << scl_pin));

  printf("%08x,%08x,%08x,%08x:", gpio_val, gpio_trig,
      read_reg(0x3f20004c),
      read_reg(0x3f200058)
  );
  if (sda_trig)
    putc(sda ? 'D' : 'd');

  if (scl_trig)
    putc(scl ? 'C' : 'c');
  putc('\n');
  putc('\r');
}

DECL_GPIO_SET_KEY(gpio_i2c_test_gpiokey, "I2CTESTGPIO_KE0");
void gpio_i2c_test(int scl, int sda, int poll)
{
  int x = 0;
  volatile uint32_t *i2c_base = (uint32_t*)0x3f214000;
//  uint32_t *dr  = i2c_base + 0;
//  uint32_t *rsr = i2c_base + 1;
  volatile uint32_t *slv = i2c_base + 2;
//  uint32_t *cr  = i2c_base + 3;
  int poll_counter = 0;

  int pins[2] = { scl, sda };
  gpio_set_handle_t gpio_set_handle;
  gpio_set_handle = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins), gpio_i2c_test_gpiokey);

  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
    puts("Failed to request gpio pins for SDA,SCL pins.\n");
    while(1);
  }

  *slv = 0xcc;
  printf("gpio_i2c_test: scl:%d, sda:%d, poll:%d\n",
      scl, sda, poll);
  scl_pin = scl;
  sda_pin = sda;

  gpio_set_function(scl_pin, GPIO_FUNC_IN);
  gpio_set_function(sda_pin, GPIO_FUNC_IN);
  if (poll) {
    while(1) {
      int scl_set = gpio_is_set(scl_pin);
      int sda_set = gpio_is_set(sda_pin);
      putc(scl_set ? 'C' : '-');
      putc(sda_set ? 'D' : '-');
      nokia5110_draw_dot(x, scl_set ? 8 : 4);
      nokia5110_draw_dot(x, sda_set ? 40: 44);
      poll_counter++;
      if (!(poll_counter % 32))
        puts("\r\n");
      if (++x > NOKIA5110_PIXEL_SIZE_X) {
        x = 0;
        nokia5110_blank_screen();
      }
    }
  }

  gpio_set_detect_rising_edge(sda_pin);
  gpio_set_detect_rising_edge(scl_pin);

  gpio_set_detect_falling_edge(sda_pin);
  gpio_set_detect_falling_edge(scl_pin);

  intr_ctl_enable_gpio_irq();

  irq_set(0, ARM_IRQ2_GPIO_1, gpio_handle_i2c_irq);
  enable_irq();

  do {
//    intr_ctl_dump_regs("waiting...\r\n");
    wait_msec(1000);
    blink_led(2, 200);
  } while(1);
}

/*
 * gpio_num_dout - digital output
 * poll          - poll for gpio values instead of interrupts
 */
void gpio_irq_test(int pin1, int pin2, int poll)
{
  printf("gpio_irq_test: pin1:%d, pin2:%d, poll:%d\n",
      pin1, pin2, poll);
  current_pin1 = pin1;
  current_pin2 = pin2;
  cont = 1;

  gpio_set_function(pin1, GPIO_FUNC_IN);
  gpio_set_function(pin2, GPIO_FUNC_IN);
  if (poll) {
    while(1) {
      if (gpio_is_set(pin1))
        putc('1');
      else
        putc('-');
      if (gpio_is_set(pin2))
        putc('2');
      else
        putc('-');
    }
  }

  gpio_set_detect_rising_edge(pin1);
  gpio_set_detect_rising_edge(pin2);

  gpio_set_detect_falling_edge(pin1);
  gpio_set_detect_falling_edge(pin2);

  irq_set(0, ARM_IRQ2_GPIO_1, gpio_handle_irq);
  intr_ctl_enable_gpio_irq();
  enable_irq();

  do {
   // intr_ctl_dump_regs("waiting...\r\n");
    wait_msec(1000);
    // asm volatile ("svc 0x1001\n");
    blink_led(2, 200);
  } while(1);
}

#ifndef CONFIG_QEMU

// gpio_set_off(gpio_pin_dc);
// gpio_set_on(gpio_pin_dc);
// spidev->xmit_byte(spidev, cmd, NULL);
// *(volatile uint32_t *)0x3f204000 = 0;
// printf("%08x\r\n", r);

void init_nokia5110_display(int report_exceptions, int run_test)
{
  spi_dev_t *spidev;
  const font_desc_t *font;
  const int gpio_pin_mosi  = 12;
  const int gpio_pin_sclk  = 7;
  const int gpio_pin_dc    = 16;
  const int gpio_pin_ce    = 20;
  const int gpio_pin_reset = 21;

  spidev = spi_allocate_emulated(
      "spi_nokia5110",
      gpio_pin_sclk,
      gpio_pin_mosi,
      -1,
      gpio_pin_ce,
      -1,
      SPI_EMU_MODE_MASTER
  );

  font_get_font("myfont", &font);

  nokia5110_init(spidev, gpio_pin_reset, gpio_pin_dc, 1, 1);
  nokia5110_set_font(font);
  if (nokia5110_console_init())
    uart_puts("Failed to init nokia5110 console device.\n");

  if (report_exceptions) {
    if (enable_unhandled_exception_reporter(REPORTER_ID_NOKIA5110))
      uart_puts("Failed to init exception reporter for nokia5110.\n");
  }

  if (run_test) {
    nokia5110_test_text();
    nokia5110_test();
  }
}
#endif

extern void pl011_uart_print_regs();

void init_uart(int report_exceptions)
{
  uart_init(115200, BCM2835_SYSTEM_CLOCK);

  if (report_exceptions) {
    if (enable_unhandled_exception_reporter(REPORTER_ID_UART_PL011))
      uart_puts("Failed to init exception reporter for uart_pl011.");
  }
}

void spi_work()
{
  int i, old_i;
  int sclk, cs, mosi;

  sclk = 21;
  cs   = 20;
  mosi = 16;
  max7219_set_spi_dev(spi_allocate_emulated("spi_test", sclk, mosi, -1, cs, -1,
    SPI_EMU_MODE_MASTER));

  while(1)
  {
    max7219_set_raw(0xf00);
  }
    // wait_msec(100000);
    max7219_set_raw(0xf01);
    // wait_msec(100000);
  //}
  max7219_set_raw(0xf01);
  // max7219_set_test_mode_on();
  wait_msec(300000);
  max7219_set_raw(0xf00);
  // max7219_set_raw(0xb07);
  max7219_set_scan_limit(7);
  wait_msec(300000);
  max7219_set_shutdown_mode_off();
  wait_msec(30000);

  for (i = 0; i < 8; i++) {
    max7219_set_raw(((i+1) << 8));
    //max7219_set_digit(i, 0x00);
  }

  old_i = 0;
  while(1) {
    for (i = 0; i < 8; ++i) {
      max7219_set_raw(((i+1) << 8) | 0xff);
      max7219_set_raw((old_i+1) << 8);
      old_i = i;
      wait_msec(30000);
    }
  }
}


void max7219_work()
{
  // Set spi device
  int ret;
  const int gpio_pin_mosi = 21;
//  const int gpio_pin_cs0  = 20;
  const int gpio_pin_sclk = 16;
  const int gpio_pin_miso = -1;
  spi_dev_t *spidev;

  spidev = spi_allocate_emulated("spi_max7219", gpio_pin_sclk, gpio_pin_mosi, gpio_pin_miso, -1, -1,
   SPI_EMU_MODE_MASTER);
  if (IS_ERR(spidev)) {
    printf("Failed to initialize emulated spi. Error code: %d\n",
       (int)PTR_ERR(spidev));
    return;
  }
  ret = max7219_set_spi_dev(spidev);
  if (ret != ERR_OK) {
    printf("Failed to initialize max7219 driver. Error code: %d\n", ret);
    return;
  }
  wait_msec(500);
  max7219_set_shutdown_mode_off();
  printf("max7219_set_shutdown_mode_off()\n");
  wait_msec(500);
  max7219_set_scan_limit(MAX7219_SCAN_LIMIT_FULL);
  printf("max7219_set_scan_limit(MAX7219_SCAN_LIMIT_FULL)\n");
  wait_msec(500);
  max7219_set_intensity(0xff);
  printf("max7219_set_intensity(0xff)\n");
  wait_msec(500);
  max7219_set_test_mode_on();
  printf("max7219_set_test_mode_on()\n");
  wait_msec(500);
  while(1);
  while(1) {
    printf("a");
    wait_msec(100);
    max7219_set_raw(0x00f1);
    wait_msec(100);
    max7219_set_raw(0x00f0);
  }
  max7219_set_raw(0x00c1);
  max7219_set_digit(0, 0xff);
  printf("max7219_set_digit(0, 0xff)\n");
  while(1);
  printf("max7219_set_test_mode_on\n");
  max7219_set_test_mode_on();
}

#define PIN_SDA 18
#define PIN_SCL 23

#define PRINT_SDA puts(scl ? "CLK_ON-" : "CLK_OFF-")
#define PRINT_SCL puts(sda ? "T-" : ".-")
#define GET_SDA sda = gpio_is_set(PIN_SDA) ? 0 : 1
#define GET_SCL scl = gpio_is_set(PIN_SCL) ? 0 : 1

void i2c_bitbang_check_line()
{
  int i = 0;
  int sda = 0;
  int scl = 0;
  while(1) {
    sda = 0;
    scl = 0;
    printf("Starting i2c negotiation\r\n");
    while(!(sda && scl)) {
      GET_SDA;
      GET_SCL;
    }
    printf("SDA/SCL up.\r\n");

    // WAIT START
    while(!(!sda && scl)) {
      GET_SDA;
      GET_SCL;
    }
    // puts("Start arrived\n");
    while(scl) GET_SCL;

    char byte = 0;
    for (i = 0; i < 8; ++i) {
      while(!scl) GET_SCL;
      GET_SDA;
      byte |= (sda<<(7-i));
      while(scl) GET_SCL;
    }
    gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_EN_PULLDOWN);
    wait_cycles(500);
    gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
    wait_cycles(500);
    printf("%02x \r\n", byte);
  }

  PRINT_SCL;
  PRINT_SDA;
  GET_SDA;
  GET_SCL;

  // for (i = 0; i < 16; ++i) {
  while(1) {
    while(1) {
      if (gpio_is_set(PIN_SDA)) {
        if (!sda) {
          sda= 1;
          PRINT_SDA;
          break;
        }
      } else {
        if (sda) {
          sda= 0;
          PRINT_SDA;
          break;
        }
      }
      if (gpio_is_set(PIN_SCL)) {
        if (!scl) {
          scl= 1;
          PRINT_SCL;
          break;
        }
      } else {
        if (scl) {
          scl= 0;
          PRINT_SCL;
          break;
        }
      }
    }
 //   wait_cycles(100);
  }
    if (!gpio_is_set(PIN_SDA) || !gpio_is_set(PIN_SCL)) {
      printf("i2c line not pulled-up. Hanging...\r\n");
      while(1);
    }
 // }
}

void i2c_bitbang_initialize()
{
  gpio_set_function(PIN_SDA, GPIO_FUNC_IN);
  gpio_set_function(PIN_SCL, GPIO_FUNC_IN);

  gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(PIN_SCL, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  i2c_bitbang_check_line();
  printf("Init complete\r\n");
}

void i2c_bitbang_wait_start()
{
  while(gpio_is_set(PIN_SDA));
  printf("start recieved\r\n");
}

void i2c_bitbang_send_ack()
{
}

void i2c_bitbang()
{
  i2c_bitbang_initialize();
  while(1) {
    i2c_bitbang_wait_start();
    i2c_bitbang_send_ack();

    if (gpio_is_set(PIN_SDA)) {
      putc('+');
    }
    if (gpio_is_set(PIN_SCL)) {
      putc('-');
    }

  }
}

void pullup_down_test()
{
  gpio_set_function(18, GPIO_FUNC_IN);
  while(1) {
    puts("PULLUPDOWN:OFF\r\n");
    gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
    int i;
    for (i = 0; i < 16; ++i) {
      wait_msec(500);
      if (gpio_is_set(18))
        puts("on\r\n");
      else
        puts("off\r\n");
    }
    puts("PULLUPDOWN:UP\r\n");
    gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_EN_PULLUP);
    for (i = 0; i < 16; ++i) {
      wait_msec(500);
      if (gpio_is_set(18))
        puts("on\r\n");
      else
        puts("off\r\n");
    }
    puts("PULLUPDOWN:DOWN\r\n");
    gpio_set_pullupdown(PIN_SDA, GPIO_PULLUPDOWN_EN_PULLDOWN);
    for (i = 0; i < 16; ++i) {
      wait_msec(500);
      if (gpio_is_set(18))
        puts("on\r\n");
      else
        puts("off\r\n");
    }
  }
}

void nokia5110_test_draw()
{
  int i;
  int x;
  int y;
  for (i = 0; i < 10; ++i) {
    x = i % NOKIA5110_PIXEL_SIZE_X;
    y = i % NOKIA5110_PIXEL_SIZE_Y;
    nokia5110_blank_screen();
    nokia5110_draw_dot(x, y);
  }
}

int spi_slave_test_bsc()
{
#define BR(x) *(reg32_t)(0x3f214000 + x)
#define CR BR(0xc)
#define DR BR(0x0)

  /* EN,SPI,TXE,RXE */
  CR = 0b00001100000011;
  while(1) {
    wait_msec(500);
    printf("%x\r\n", DR);
  }
  return ERR_OK;
}

static inline void pwm_servo(int ch, int value)
{
  int err;
  int range_start = 60;
  int range_end = 250;
  int range = range_end - range_start;
  float norm = (float)(value & 0x3ff) / 0x3ff;
  int pt = range_start + range * norm;
  // printf("pwm_servo:%d->%d\r\n", (int)value, pt);
  err = pwm_set(ch, 2000, pt);
  if (err != ERR_OK) {
    printf("Failed to enable pwm: %d\r\n", err);
    return;
  }
 // wait_msec(10);
}

struct servo *prep_servo(int ch, int pwm_gpio_pin)
{
  struct pwm *pwm;
  struct servo *servo;
  char idbuf[16];
  snprintf(idbuf, sizeof(idbuf), "serv_%d_%d", ch, pwm_gpio_pin);
  pwm = pwm_bcm2835_create(pwm_gpio_pin, 1);
  if (IS_ERR(pwm)) {
    printf("Failed to prepare pwm for servo with gpio_pin:%d,err:%d\r\n",
        pwm_gpio_pin, (int)PTR_ERR(pwm));
    return ERR_PTR(PTR_ERR(pwm));
  }

  servo = servo_sg90_create(pwm, idbuf);
  if (IS_ERR(servo))
    printf("Failed to create sg90 servo:%d\r\n", (int)PTR_ERR(servo));
  return servo;
}

#define ADC_MIN 0
#define ADC_MAX 0x3ff
#define ANGLE_MIN -90
#define ANGLE_MAX 90
#define ANGLE_RANGE (ANGLE_MAX - ANGLE_MIN)

#define adc_normalize(value) (((float)value) / (ADC_MAX - ADC_MIN))
#define value_to_angle(v) ((ANGLE_RANGE * adc_normalize(v)) + ANGLE_MIN)

int spi_slave_test()
{
  int i;
  struct servo *servos[2];
  const int servos_pins[2] = { 18, 19 };

  const int gpio_pin_cs0   = 5;
  const int gpio_pin_mosi  = 6;
  const int gpio_pin_sclk  = 11;

  spi_dev_t *spidev;
  spidev = spi_allocate_emulated("spi_avr_isp",
      gpio_pin_sclk, gpio_pin_mosi, -1, gpio_pin_cs0, -1,
      SPI_EMU_MODE_SLAVE);
  if (IS_ERR(spidev)) {
    printf("Failed to initialize emulated spi. Error code: %d\n",
        PTR_ERR(spidev));
    return ERR_GENERIC;
  }

  for(i = 0; i < ARRAY_SIZE(servos); ++i) {
    servos[i] = prep_servo(i, servos_pins[i]);
    if (IS_ERR(servos[i]))
      return (int)PTR_ERR(servos[i]);
  }

  puts("starting spi slave while loop\r\n");
  while(1) {
    int ch;
    char x = 0x77;
    char from_spi[50];
    uint16_t value;
    spidev->xmit_byte(spidev, x, from_spi+0);
    spidev->xmit_byte(spidev, x, from_spi+1);
    value = *from_spi + (*(from_spi + 1) << 8);
    ch = (value >> 15) & 1;
    value &= 0x3ff;

    if (ch)
      printf("            %d:%04x\r\n", ch, (int)value);
    else
      printf("%d:%04x\r\n", ch, (int)value);
    servos[ch]->set_angle(servos[ch], value_to_angle(value));
  }
  return ERR_OK;
}

extern char __bss_start;
extern char __bss_end;
extern char __text_start;
extern char __text_end;

static void print_memory_map(void)
{
  uint64_t dma_start = dma_area_get_start_addr();
  uint64_t dma_end = dma_area_get_end_addr();
  uint64_t bss_start = (uint64_t)&__bss_start;
  uint64_t bss_end   = (uint64_t)&__bss_end;
  uint64_t text_start = (uint64_t)&__text_start;
  uint64_t text_end   = (uint64_t)&__text_end;
  printf(".text : [0x%016x:0x%016x]"__endline, text_start, text_end);
  printf(".bss  : [0x%016x:0x%016x]"__endline, bss_start, bss_end);
  printf("dma   : [0x%016x:0x%016x]"__endline, dma_start, dma_end);
}

void UNUSED i2c_eeprom()
{
  int err, n;
  err = i2c_init();
  BUG(err != ERR_OK, "i2c_init failed");
  bool is_read = true;

  if (is_read) {
    char addr = 0x00;
    char readbuf[8];
    n = i2c_write(0x50, &addr, 1);
    BUG(n != 1, "i2c_write did not return 1");
    n = i2c_read(0x50, readbuf, 2);
    BUG(n != 2, "i2c_write did not return 2");
    printf("i2c_read: %02x" __endline, readbuf[0]);
  } else {
    char buf[] = { 0x00, 0x77 };
    n = i2c_write(0x50, buf, sizeof(buf));
    BUG(n != 2, "i2c_write did not return 2");
  }
  while(1);
}

static char samples_dp[8192] ALIGNED(64);
static char samples_dm[8192] ALIGNED(64);

static int sample;

#define USB_DPLUS 19
#define USB_DMINUS 26
static void __attribute__((optimize("O3"))) UNUSED test_pic(int pin)
{
 // char A[8] = {
   // 6, 6, 6, 6, 6, 3, 1, 6
 // };
  float K[8] = {
    1.0, 1.0, 1.0, 1.0, 0.01, 2.0, 1.0, 1.0
  };

#define ON() gpio_set_on(pin)
#define OFF() gpio_set_off(pin)

  gpio_set_function(pin, GPIO_FUNC_OUT);
  gpio_set_off(pin);
  int i;
  int j;
  while(1) {
    for (i = 0; i < 8; ++i) {
      float p = 100 * K[i];
      for (j = 0; j < 1000; ++j) {
        float working_cycle = 0.5;
        ON();
        wait_usec(p * working_cycle);
        OFF();
        wait_usec(p * (1- working_cycle));
      }
    }
  }
}

static void __attribute__((optimize("O3"))) UNUSED test_usb()
{
  int dp, dm;
#define get_v() \
  v = *(volatile uint32_t*)0x3f200034

#define get_dpdm() \
    dp = (v >> USB_DPLUS) & 1;\
    dm = (v >> USB_DMINUS) & 1;

  volatile uint64_t v;
  gpio_set_function(USB_DPLUS, GPIO_FUNC_IN);
  gpio_set_function(USB_DMINUS, GPIO_FUNC_IN);
  gpio_set_pullupdown(USB_DPLUS, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
  gpio_set_pullupdown(USB_DMINUS, GPIO_PULLUPDOWN_NO_PULLUPDOWN);
 // char old_c = 'i';

  sample = 0;
  while(1) {
    get_v();
    get_dpdm();
    if (dp)
      break;
  }

  while(sample < sizeof(samples_dp)) {
    get_v();
    get_dpdm();
    samples_dp[sample] = dp;
    samples_dm[sample] = dm;
    sample++;
    wait_usec(500);
  }
  puts("dp:\r\n");
  for (sample = 0; sample < sizeof(samples_dp); ++sample) {
    putc(samples_dp[sample] ? '1' : '0');
    if (sample % 64 == 0)
      puts("\r\n");
  }

  puts("\r\ndm:\r\n");
  for (sample = 0; sample < sizeof(samples_dm); ++sample) {
    putc(samples_dm[sample] ? '1' : '0');
    if (sample % 64 == 0)
      puts("\r\n");
  }
  while(1);
}

void main()
{
  int ret;
  mbox_init();
  debug_init();
  gpio_set_init();
  dma_area_init();
  // spi_emulated_init();
  init_unhandled_exception_reporters();

  font_init_lib();

#ifdef CONFIG_HDMI
  vcanvas_init(CONFIG_DISPLAY_WIDTH, CONFIG_DISPLAY_HEIGHT);
  vcanvas_set_fg_color(0x00ffffaa);
  vcanvas_set_bg_color(0x00000010);
#endif

  init_uart(1);
  init_consoles();
  self_test();
  irq_init(0 /*loglevel*/);
  add_unhandled_exception_hook(report_unhandled_exception);
  add_kernel_panic_reporter(report_kernel_panic);
  // i2c_eeprom();
  // pwm_bcm2835_init();
  // bcm2835_set_pwm_clk_freq(100000);
  // servo_sg90_init();
  print_mbox_props();
  // usbd_init();
  // usbd_print_device_tree();
#ifndef CONFIG_QEMU
  // init_nokia5110_display(1, 0);
  // nokia5110_draw_text("Display ready", 0, 0);
#endif
  // nokia5110_test_draw();
  // init_atmega8a();
  // if (avr_update()) {
  //  puts("halting\n");
  // }
  // atmega8a_drop_spi();
  // spi_slave_test();
  cm_print_clocks();
  // atmega8a_spi_master_test();
  // i2c_test();
  // gpio_irq_test(16, 21, 0 /* no poll, use interrupts */);
  // while(1);

  // i2c_init();
  // i2c_bitbang();
  //  if (bsc_slave_init(BSC_SLAVE_MODE_I2C, 0x66)) {
  //
  //    puts("Failed to init bsc_slave in I2C mode\n");
  //  } else
  //    bsc_slave_debug();
  //  while(1);

  test_mmio_dma(false, false);
  mmu_init();
  test_mmio_dma(true, true);
  // while(1);
  print_memory_map();
  // test_pic(19);
  // nokia5110_draw_text("MMU OK!", 0, 0);
  spinlocks_enabled = 1;

  print_cpu_info();
  print_current_ex_level();
  ret = bcm2835_arm_timer_init();
  BUG(ret != ERR_OK, "Failed to init arm_timer");
  ret = bcm2835_systimer_init();
  BUG(ret != ERR_OK, "Failed to init system timer");
  ret = armv8_generic_timer_init();
  BUG(ret != ERR_OK, "Failed to init armv8 generic timer");

  cmdrunner_init();
  print_cache_stats();
  tags_print_cmdline();
  // disable_l1_caches();

#ifdef CONFIG_HDMI
  vcanvas_showpicture();
#endif

  rand_init();
  // cmdrunner_run_interactive_loop();
  scheduler_init(0/* log_level */, init_func);
  while(1);
}
