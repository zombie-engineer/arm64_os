#include <config.h>
#include <irq.h>
#include <uart/uart.h>
#include <gpio.h>
#include <gpio_set.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <arch/armv8/armv8.h>
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
#include <timer.h>
#include <cmdrunner.h>
#include <max7219.h>
#include <drivers/atmega8a.h>
#include <drivers/display/nokia5110.h>
#include <drivers/display/nokia5110_console.h>
#include <debug.h>
#include <unhandled_exception.h>
#include <board/bcm2835/bcm2835.h>
#include <board/bcm2835/bcm2835_irq.h>

#include <cpu.h>
#include <list.h>
#include <self_test.h>

#define DISPLAY_WIDTH 1824
#define DISPLAY_HEIGHT 984

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
}

void wait_gpio()
{
  intr_ctl_enable_gpio_irq();
  enable_irq();
  gpio_set_function(20, GPIO_FUNC_IN);
  // gpio_set_detect_high(20);
  // GPLEN0 |= 1 << 2;
  gpio_set_detect_falling_edge(2);
  
  intr_ctl_dump_regs("after set\n");
  while(1) {
    // f: 1111 b: 1011
    wait_cycles(0x300000);
    // print_reg32(INT_CTRL_IRQ_PENDING_2);
    print_reg32_at(GPIO_REG_GPLEV0);
    print_reg32_at(GPIO_REG_GPEDS0);
  }
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

void print_mmu_stats()
{
  printf("ttbr0_el1: 0x%016lx ttbr1_el1: 0x%016lx, tcr_el1: 0x%016lx\n", 
    arm_get_ttbr0_el1(), 
    arm_get_ttbr1_el1(), 
    arm_get_tcr_el1());
}

void print_mmu_features()
{
  char memattrs[8];
  uint64_t mair_value = armv8_get_mair_el1();
  memattrs[0] = mair_value & 0xff;
  memattrs[1] = (mair_value >> 8) & 0xff;
  printf("mmu: mem_attr_0: %02x, mem_attr_1: %02x\n", memattrs[0], memattrs[1]);
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

static int print_reg_cb(const char *reg_str, size_t reg_str_sz, void *cb_priv)
{
  print_cpuctx_ctx_t *print_ctx = (print_cpuctx_ctx_t *)cb_priv;
  if (print_ctx->nr) {
    if (print_ctx->nr % 4 == 0) {
      uart_putc('\n');
      uart_putc('\r');
    } else {
      uart_putc(',');
      uart_putc(' ');
    }
  }
  uart_puts(reg_str);
  print_ctx->nr++;
  return 0;
}

extern void *__current_cpuctx;
void gpio_handle_irq()
{
  uint64_t elr;
  asm volatile ("mrs   %0, elr_el1": "=r"(elr));
  print_cpuctx_ctx_t print_ctx = { 0 };
  printf("basic: %08x, gpu1: %08x, gpu2: %08x, elr: %016llx\n",
      read_reg(0x3f00b200), 
      read_reg(0x3f00b204), 
      read_reg(0x3f00b208), elr);

  printf("__current_cpuctx at %016llx pointing at %016llx\n" , &__current_cpuctx, __current_cpuctx);

  cpuctx_print_regs(__current_cpuctx, print_reg_cb, &print_ctx);

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
  uint32_t *i2c_base = 0x3f214000;
  uint32_t *dr  = i2c_base + 0;
  uint32_t *rsr = i2c_base + 1;
  uint32_t *slv = i2c_base + 2;
  uint32_t *cr  = i2c_base + 3;
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
      nokia5110_draw_dot(x, scl_set ? 8 : 7);
      nokia5110_draw_dot(x, sda_set ? 14 : 13);
      poll_counter++;
      if (!(poll_counter % 32))
        puts("\r\n");
      if (++x > NOKIA5110_PIXEL_SIZE_X)
        x = 0;
    }
  }

  gpio_set_detect_rising_edge(sda_pin);
  gpio_set_detect_rising_edge(scl_pin);

  gpio_set_detect_falling_edge(sda_pin);
  gpio_set_detect_falling_edge(scl_pin);

  intr_ctl_enable_gpio_irq();

  irq_set(ARM_IRQ2_GPIO_1, gpio_handle_i2c_irq);
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

  irq_set(ARM_IRQ2_GPIO_1, gpio_handle_irq);
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
      -1
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
extern uint64_t __shared_mem_start;

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
  max7219_set_spi_dev(spi_allocate_emulated("spi_test", sclk, mosi, -1, cs, -1));

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
  const int gpio_pin_cs0  = 20;
  const int gpio_pin_sclk = 16;
  const int gpio_pin_miso = -1;
  spi_dev_t *spidev;

  spidev = spi_allocate_emulated("spi_max7219", gpio_pin_sclk, gpio_pin_mosi, gpio_pin_miso, -1, -1);
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

void init_atmega8a()
{
  const int gpio_pin_mosi  = 6;
  const int gpio_pin_miso  = 13;
  const int gpio_pin_sclk  = 19;
  const int gpio_pin_reset = 26;

  int ret;
  char fuse_high;
  char fuse_low;
  char lock_bits;
  int flash_size;
  int eeprom_size;
  spi_dev_t *spidev;
  char lock_bits_desc[128];
  spidev = spi_allocate_emulated("spi_avr_isp", 
      gpio_pin_sclk, gpio_pin_mosi, gpio_pin_miso, -1, -1);
  if (IS_ERR(spidev)) {
    printf("Failed to initialize emulated spi. Error code: %d\n", 
       (int)PTR_ERR(spidev));
    return;
  }

  ret = atmega8a_init(spidev, gpio_pin_reset);

  if (ret != ERR_OK) {
    printf("Failed to init atmega8a. Error_code: %d\n", ret);
    return;
  }

  if (atmega8a_read_fuse_bits(&fuse_low, &fuse_high) != ERR_OK)
    printf("Failed to read program memory\n");

  if (atmega8a_read_lock_bits(&lock_bits) != ERR_OK)
    printf("Failed to read program memory\n");

  atmega8a_lock_bits_describe(lock_bits_desc, sizeof(lock_bits_desc), lock_bits);

  flash_size = atmega8a_get_flash_size();
  eeprom_size = atmega8a_get_eeprom_size();
  printf("atmega8 downloader initialization:\r\n");
  printf("  flash size: %d bytes\r\n", flash_size);
  printf("  eeprom size: %d bytes\r\n", eeprom_size);
  printf("  lock bits: %x: %s\r\n", lock_bits, lock_bits_desc);
  printf("  fuse bits: low:%02x high:%02x\r\n", fuse_low, fuse_high);
}

void atmega8a_program()
{
  char fuse_bits_low = 0xe0 | (ATMEGA8A_FUSE_CPU_FREQ_1MHZ & ATMEGA8A_FUSE_CPU_FREQ_MASK);
  printf("programming atmega8a: \r\n");
  printf("writing low fuse bits to %x\r\n", fuse_bits_low);
  atmega8a_write_fuse_bits_low(fuse_bits_low);
  printf("programming atmega8a complete\r\n");
}

void atmega8a_read_firmware()
{
  char membuf[8192];
  int num_read;
  num_read = min(atmega8a_get_flash_size(), sizeof(membuf));
  printf("atmega8a_read_firmware: reading %d bytes of atmega8a flash memory\r\n.", num_read);
  atmega8a_read_flash_memory(membuf, num_read, 0);
  hexdump_memory(membuf, num_read);
  puts("atmega8a_read_firmware: Success.\r\n");
}

void atmega8a_download(const void *bin, int bin_size)
{
  char membuf[8192];
  const char *binary = bin;
  int binary_size = bin_size;
  if (bin_size > 8192) {
    printf("Not writing binary of size %d to flash memory. Too big.\r\n",
        bin_size);
    return;
  }

  if (bin_size % 64) {
    binary = membuf;
    binary_size = ((bin_size / 64) + 1) * 64;
    printf("Binary size %d not page-aligned (64 bytes), padding up to %d bytes\r\n", 
        bin_size, binary_size);
    memcpy(membuf, bin, bin_size);
  }

  printf("Erasing chip flash memory..\r\n");
  if (atmega8a_chip_erase() != ERR_OK) {
    printf("Failed to erase chip\r\n");
    while(1);
  }
  printf("Chip flash erased.\r\n");
  hexdump_memory(binary, binary_size);
  printf("Writing %d bytes of binary to flash memory..\r\n");
  if (atmega8a_write_flash(binary, binary_size, 0) != ERR_OK) {
    printf("Failed to read program memory\n");
    while(1);
  }
  printf("Write complete.\r\n");

  memset(membuf, 0x00, 64);

  printf("Reading.\r\n");
  if (atmega8a_read_flash_memory(membuf, 64, 0 * 64) != ERR_OK)
    printf("Failed to read program memory\n");
  printf("Read complete.\r\n");
  hexdump_memory(membuf, 64);
  while(1);
}

extern const char _binary_firmware_atmega8a_atmega8a_bin_start;
extern const char _binary_firmware_atmega8a_atmega8a_bin_end;

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

void main()
{
  const char *atmega8a_bin = &_binary_firmware_atmega8a_atmega8a_bin_start;
  int atmega8a_bin_size = &_binary_firmware_atmega8a_atmega8a_bin_end 
      - &_binary_firmware_atmega8a_atmega8a_bin_start;

  debug_init();
  gpio_set_init();
  spi_emulated_init();
  init_unhandled_exception_reporters();

  font_init_lib();

  vcanvas_init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  vcanvas_set_fg_color(0x00ffffaa);
  vcanvas_set_bg_color(0x00000010);
  init_uart(1);
  init_consoles();
  self_test();
  irq_init(0 /*loglevel*/);
  add_unhandled_exception_hook(report_unhandled_exception);
  init_atmega8a();
#ifndef CONFIG_QEMU
  init_nokia5110_display(1, 0);
  nokia5110_draw_text("Display ready", 0, 0);
#endif
  gpio_i2c_test(8, 25, 1 /* yes poll */);
  while(1);
  // gpio_irq_test(16, 21, 0 /* no poll, use interrupts */);
  // while(1);

  // wait_gpio();
  // i2c_init();
  // i2c_bitbang();
//  if (bsc_slave_init(BSC_SLAVE_MODE_I2C, 0x66)) {
//    
//    puts("Failed to init bsc_slave in I2C mode\n");
//  } else
//    bsc_slave_debug();
//  while(1);
  // atmega8a_program();
  // atmega8a_read_firmware();
  atmega8a_download(atmega8a_bin, atmega8a_bin_size);
  while(1);
  
  print_mbox_props();
  mmu_init();
  // nokia5110_draw_text("MMU OK!", 0, 0);
  spinlocks_enabled = 1;
  
  print_cpu_info();
  print_current_ex_level();
  systimer_init();

  gpio_irq_test(16, 21, 0 /* no poll, use interrupts */);
  while(1);
  scheduler_init();
  while(1);

  print_mmu_features();
  print_cache_stats();

  // disable_l1_caches();

  rand_init();

  vcanvas_showpicture();
  // generate exception here
  // el = *(unsigned long*)0xffffffff;
  tags_print_cmdline();

  print_mmu_stats();
  // run_task();

  // hexdump_addr(0x100);
  // 0x0000000000001122
  // PARange  2: 40 bits, 1TB 0x2
  // ASIDBits 2: 16 bits
  // BigEnd   1: Mixed-endian support
  // SNSMem   1: Secure versus Non-secure Memory
  // TGran16  0: 16KB granule not supported
  // TGran64  0: 64KB granule is supported
  // TGran4   0: 4KB granule is supported

  // vcanvas_fill_rect(10, 10, 100, 10, 0x00ffffff);
  // vcanvas_fill_rect(10, 20, 100, 10, 0x00ff0000);
  cmdrunner_init();
  cmdrunner_run_interactive_loop();
}
