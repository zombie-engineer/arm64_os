#include <config.h>
#include <uart/uart.h>
#include <gpio.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <arch/armv8/armv8.h>
#include <vcanvas.h>
#include <rand.h>
#include <timer.h>
#include <delays.h>
#include <tags.h>
#include <mmu.h>
#include <common.h>
#include <sprintf.h>
#include <console.h>
#include <interrupts.h>
#include <exception.h>
#include <timer.h>
#include <cmdrunner.h>
#include <max7219.h>

#define DISPLAY_WIDTH 1824
#define DISPLAY_HEIGHT 984

void print_current_ex_level()
{
  unsigned long el;
  asm volatile("mrs %0, CurrentEL; lsr %0, %0, #2" : "=r"(el));
  printf("Executing at EL%d\n", el);
}

void print_mbox_props()
{
  int val, val2, i;
  unsigned clock_rate;
  char buf[6];
  val = mbox_get_firmware_rev();
  printf("firmware rev:    %08x\n", val);
  gpio_set_function(18, GPIO_FUNC_ALT_5);
  val = mbox_get_board_model();
  printf("board model:     %08x\n", val);
  val = mbox_get_board_rev();
  printf("board rev:       %08x\n", val);
  val = mbox_get_board_serial();
  printf("board serial:    %08x\n", val);
  if (mbox_get_mac_addr(&buf[0], &buf[7]))
    printf("failed to get MAC addr\n");
  else
    printf("mac address:     %x:%x:%x:%x:%x:%x\n", 
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
      printf("clock %d rate: %08x\n", i, clock_rate);
  }
}

int lit = 0;

void c_irq_handler(void)
{
  gpio_set_on(21);
  // if (lit) lit = 0; else lit = 1;
//  set_activity_led(lit);
// clear_timer_irq();
}

void wait_timer()
{
  gpio_set_function(21, GPIO_FUNC_OUT);
  enable_irq();
  interrupt_ctrl_enable_timer_irq();
  if (is_irq_enabled())
    printf("irq enabled\n");
  arm_timer_set(500000, c_irq_handler);
  while(1)
  {
    interrupt_ctrl_dump_regs("after set\n");
    print_reg32_at(GPIO_REG_GPEDS0);
    wait_cycles(0x1800000);
    print_reg32_at(GPIO_REG_GPEDS0);
    print_reg32(ARM_TIMER_VALUE_REG);
    print_reg32(ARM_TIMER_RAW_IRQ_REG);
    print_reg32(ARM_TIMER_MASKED_IRQ_REG);
    printf(">>\n");
    gpio_set_on(21);
    wait_cycles(0x1800000);
    print_reg32(ARM_TIMER_VALUE_REG);
    print_reg32(ARM_TIMER_RAW_IRQ_REG);
    print_reg32(ARM_TIMER_MASKED_IRQ_REG);
    printf(">>\n");
    gpio_set_off(21);
    if (ARM_TIMER_RAW_IRQ_REG)
      ARM_TIMER_IRQ_CLEAR_ACK_REG = 1;
  }
}

void wait_gpio()
{
  interrupt_ctrl_dump_regs("before set\n");
  interrupt_ctrl_enable_gpio_irq(20);
  enable_irq();
  // gpio_set_function(20, GPIO_FUNC_IN);
  // gpio_set_detect_high(20);
  // GPLEN0 |= 1 << 2;
  gpio_set_detect_falling_edge(2);
  
  interrupt_ctrl_dump_regs("after set\n");
  while(1) {
    // f: 1111 b: 1011
    wait_cycles(0x300000);
    print_reg32(INT_CTRL_IRQ_PENDING_2);
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

void print_arm_features()
{
  unsigned long el;
  asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(el));
  printf("id_aa64mmfr0_el1 is: %08x\n", el);
}

void vibration_sensor_test(int gpio_num_dout, int poll)
{
  // gpio_num_dout - digital output
  // poll          - poll for gpio values instead of interrupts
  printf("virbration sensor test\n");
  gpio_set_function(gpio_num_dout, GPIO_FUNC_IN);
  if (poll) {
    while(1) {
      if (gpio_is_set(gpio_num_dout)) {
        printf("-");
      }
    }
  }

  interrupt_ctrl_enable_gpio_irq(gpio_num_dout);
  gpio_set_detect_rising_edge(gpio_num_dout);
  gpio_set_detect_falling_edge(gpio_num_dout);
  enable_irq();
  
  interrupt_ctrl_dump_regs("after set\n");
  while(1) {
    wait_cycles(0x300000);
  }
}

 // void nokia_work()
 // {
 //   int clk, din, dc, ce, rst;
 //   char value;
 //   spi_dev_t *d;
 //   int PD, V, H;
 // 
 //   spi_emulated_init(clk, din, 19, ce, 26);
 //   d = spi_emulated_get_dev();
 // 
 //   gpio_set_function(dc, GPIO_FUNC_OUT);
 //   gpio_set_function(rst, GPIO_FUNC_OUT);
 //   gpio_set_off(rst);
 //   wait_msec(30000);
 //   gpio_set_on(rst);
 //   wait_msec(30000);
 // 
 //   gpio_set_off(dc);
 //   // set PD V H
 //   PD = 0; // not shutdown mode
 //   V = 0;  // horizontal addressing mode
 //   H = 0;  // normal function set
 //   value = 0b00100000 | (2 << PD) | (1 << V) | H;
 //   d->xfer(&value, 1);
 //   wait_msec(30000);
 // 
 //   // set display mode
 //   value = 0b00001000;
 //   d->xfer(&value, 1);
 //   wait_msec(30000);
 //   // set x-addr
 //   value = 0x80;
 //   d->xfer(&value, 1);
 //   wait_msec(30000);
 // 
 //   // set y-addr
 //   value = 0x40;
 //   d->xfer(&value, 1);
 //   wait_msec(30000);
 // }


void shiftreg_work()
{
  int ser, rclk, srclk, srclr;

  srclk = 21;
  rclk  = 20;
  ser   = 16;
  // srclr = 12;

  gpio_set_function(ser  , GPIO_FUNC_OUT); 
  gpio_set_function(rclk , GPIO_FUNC_OUT); 
  gpio_set_function(srclk, GPIO_FUNC_OUT); 
  // gpio_set_function(srclr, GPIO_FUNC_OUT); 

  gpio_set_off (ser); 
  gpio_set_off (rclk); 
  gpio_set_off (srclk); 
  // gpio_set_off (srclr); 

  gpio_set_pullupdown (ser  , GPIO_PULLUPDOWN_EN_PULLDOWN); 
  gpio_set_pullupdown (rclk , GPIO_PULLUPDOWN_EN_PULLDOWN); 
  gpio_set_pullupdown (srclk, GPIO_PULLUPDOWN_EN_PULLDOWN); 
  // gpio_set_pullupdown (srclr, GPIO_PULLUPDOWN_EN_PULLDOWN); 

#define PULSE(time) \
  gpio_set_on(srclk);    \
  wait_msec(time);       \
  gpio_set_off(srclk);   \
  wait_msec(time)

#define PULSE_RCLK(time) \
    gpio_set_off(rclk);  \
    wait_msec(time);     \
    gpio_set_on(rclk);   \

#define PULSE_SRCLR()     \
    gpio_set_off(srclr);  \
    wait_msec(80000);     \
    gpio_set_on(srclr);   \
    wait_msec(80000);     \
    gpio_set_off(srclr);  \
    wait_msec(80000);     \
    gpio_set_on(srclr);   \
 
#define PUSH_BIT(b, time) {\
    if (b) gpio_set_on(ser); else gpio_set_off(ser); \
    wait_msec(time);    \
    PULSE(time); \
  }

  while(1) {
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PULSE_RCLK(10000);

    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PULSE_RCLK(10000);

    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(0, 10000);
    PUSH_BIT(1, 10000);
    PUSH_BIT(1, 10000);
    PULSE_RCLK(10000);
    // PULSE_SRCLR();
  }
}

void spi_work() 
{
  int i, old_i;
  int sclk, cs, mosi;

  // spi_emulated_init(18, 14, 23, 15, 24);

  sclk = 22;
  cs   = 27;
  mosi = 17;

  sclk = 7;
  mosi = 14;
  cs   = 15;

  spi_emulated_init(sclk, mosi, 18, cs, 24);
  spi0_init(SPI_TYPE_SPI0);
  // max7219_set_spi_dev(spi_emulated_get_dev());
  max7219_set_spi_dev(spi0_get_dev());
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

void main()
{
  shiftreg_work(); return;
  vcanvas_init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  vcanvas_set_fg_color(0x00ffffaa);
  vcanvas_set_bg_color(0x00000010);
  // spi_work();
  uart_init(115200, BCM2825_SYSTEM_CLOCK);
  init_consoles();
  // shiftreg setup is for 8x8 led matrix 
  // mmu_init();
  print_current_ex_level();
  print_mbox_props();
  print_arm_features();
  disable_l1_caches();

  rand_init();

  vcanvas_showpicture();
  // vibration_sensor_test(19, 0 /* no poll, use interrupts */);
  // generate exception here
  // el = *(unsigned long*)0xffffffff;
  tags_print_cmdline();

  // gpio_set_function(21, GPIO_FUNC_OUT);
  print_cache_stats();
  print_mmu_stats();

  // hexdump_addr(0x100);
  // 0x0000000000001122
  // PARange  2: 40 bits, 1TB 0x2
  // ASIDBits 2: 16 bits
  // BigEnd   1: Mixed-endian support
  // SNSMem   1: Secure versus Non-secure Memory
  // TGran16  0: 16KB granule not supported
  // TGran64  0: 64KB granule is supported
  // TGran4   0: 4KB granule is supported
  // wait_timer();

  // vcanvas_fill_rect(10, 10, 100, 10, 0x00ffffff);
  // vcanvas_fill_rect(10, 20, 100, 10, 0x00ff0000);
  cmdrunner_init();
  cmdrunner_run_interactive_loop();

  wait_gpio();

  if (mbox_call(MBOX_CH_PROP)) {
    uart_puts("My serial number is: ");
    uart_hex(mbox[6]);
    uart_hex(mbox[5]);
    uart_puts("\n");
  } else {
    uart_puts("Unable to query serial!\n");
  }

  uart_puts("waiting 20000 cycles\n");
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec(2000);
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec_st(40000);
  uart_puts("wait complete.\n");
}
