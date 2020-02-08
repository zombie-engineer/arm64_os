#include <config.h>
#include <uart/uart.h>
#include <gpio.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <arch/armv8/armv8.h>
#include <spinlock.h>
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
#include <drivers/display/nokia5110.h>
#include <debug.h>
#include <unhandled_exception.h>

#include <cpu.h>
#include <list.h>

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

extern char __arm_initial_reg_value_address_hcr_el2;

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
    *(uint64_t*)(&__arm_initial_reg_value_address_hcr_el2), 
    &__arm_initial_reg_value_address_hcr_el2
  );
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

void* my_timer_callback_periodic(void* arg)
{
  puts("periodic tick\n");
  return 0;
}

void wait_gpio()
{
  intr_ctl_dump_regs("before set\n");
  intr_ctl_enable_gpio_irq(20);
  enable_irq();
  // gpio_set_function(20, GPIO_FUNC_IN);
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

  intr_ctl_enable_gpio_irq(gpio_num_dout);
  gpio_set_detect_rising_edge(gpio_num_dout);
  gpio_set_detect_falling_edge(gpio_num_dout);
  enable_irq();
  
  intr_ctl_dump_regs("after set\n");
  while(1) {
    wait_cycles(0x300000);
  }
}

#ifndef CONFIG_QEMU
void nokia5110_test()
{
  nokia5110_run_test_loop_1(5, 200);
  nokia5110_draw_text("Good evening, sir!", 0, 0);
  nokia5110_run_test_loop_3(30, 10);
  nokia5110_run_test_loop_1(8, 100);
  nokia5110_run_test_loop_1(8, 50);
  nokia5110_run_test_loop_1(8, 30);
  nokia5110_run_test_loop_1(8, 15);
  nokia5110_run_test_loop_2(10, 50);
  nokia5110_run_test_loop_2(10, 40);
  nokia5110_run_test_loop_2(10, 30);
  nokia5110_run_test_loop_2(20, 20);
  nokia5110_run_test_loop_2(30, 10);
}

void init_nokia5110_display(int report_exceptions, int run_test)
{
  spi_dev_t *spidev;
  const font_desc_t *font;
  spi0_init();
  spidev = spi_get_dev(SPI_TYPE_SPI0);

  font_get_font("myfont", &font);

  nokia5110_init(spidev, 23, 18, 1, 1);
  nokia5110_set_font(font);

  if (report_exceptions) {
    if (enable_unhandled_exception_reporter(REPORTER_ID_NOKIA5110))
      uart_puts("Failed to init exception reporter for nokia5110.\n");
  }

  if (run_test)
    nokia5110_test();
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

void main()
{
  debug_init();
  init_unhandled_exception_reporters();

  font_init_lib();

  vcanvas_init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  vcanvas_set_fg_color(0x00ffffaa);
  vcanvas_set_bg_color(0x00000010);

  init_uart(1);

#ifndef CONFIG_QEMU
  init_nokia5110_display(1, 0);
#endif

  init_consoles();

  print_mbox_props();
  set_irq_cb(intr_ctl_handle_irq);
  nokia5110_draw_text("Start MMU", 0, 0);
  mmu_init();
  nokia5110_draw_text("MMU OK!", 0, 0);
  spinlocks_enabled = 1;
  // uint64_t mair;
  // asm volatile ("mrs %0, mair_el1\n" : "=r"(mair));
  // printf("mair: %016llx\n", mair);
  pl011_uart_print_regs();
  printf("__shared_mem_start: %016llx\n", *(uint64_t *)__shared_mem_start);
  // asm volatile("AT S1E0R, %0"::"r"(&pl011_rx_buf_lock));
  // asm volatile("at s1e1r, %0"::"r"(__shared_mem_start));
  // __shared_mem_start = 0x80000;
  // asm volatile("ldxr w1, [%0]"::"r"(__shared_mem_start));
  // asm volatile("at s1e1r, %0"::"r"(__shared_mem_start));
  // puts("1\n");
  // asm volatile("svc #0");
  // puts("2\n");
  // asm volatile("svc #1");
  // puts("3\n");
  // asm volatile("svc #2");
  // puts("4\n");
  print_cpu_info();
  systimer_init();
  print_current_ex_level();

  // enable_irq();
  // while(1);
  add_unhandled_exception_hook(report_unhandled_exception);
  //kernel_panic("hello");
  // *(int *)0xfffffffffff = 0;
  scheduler_init();
  while(1);

  print_mmu_features();
  print_cache_stats();

  // disable_l1_caches();

  rand_init();

  vcanvas_showpicture();
  // vibration_sensor_test(19, 0 /* no poll, use interrupts */);
  // generate exception here
  // el = *(unsigned long*)0xffffffff;
  tags_print_cmdline();

  // gpio_set_function(21, GPIO_FUNC_OUT);
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

  wait_gpio();
}
