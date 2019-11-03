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


void main()
{
  vcanvas_init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  vcanvas_set_fg_color(0x00ffffaa);
  vcanvas_set_bg_color(0x00000010);
  // shiftreg setup is for 8x8 led matrix 
  uart_init(115200, BCM2825_SYSTEM_CLOCK);
  init_consoles();
  printf("hello, %lld, %llu, %llx, %ld, %lu, %lx, %d, %u, %x\n",
    0xffffffffffffffffll,
    0xffffffffffffffffllu,
    0xffffffffffffffffll,
    0xffffffffl,
    0xfffffffflu,
    0xffffffffl);
  printf("%lld\n", 0x7fffffffffffffff);
  printf("%lld\n", 0xfffffffffffffffe);
  printf("%lld\n", 0x8000000000000000LL);
  printf("%lld\n", 0x8000000000000000LL + 1);
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
