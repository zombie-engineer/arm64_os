#include "uart.h"
#include "mbox.h"
#include "armv8.h"
#include "lfb.h"
#include "rand.h"
#include "timer.h"
#include "delays.h"
#include "tags.h"
#include "mmu.h"
#include "common.h"
#include "sprintf.h"
#include "console.h"
#include "mbox_props.h"
#include "gpio.h"
#include "interrupts.h"
#include "exception.h"
#include "timer.h"

#define DISPLAY_WIDTH 1824
#define DISPLAY_HEIGHT 984

void print_current_ex_level()
{
  unsigned long el;
  asm volatile("mrs %0, CurrentEL" : "=r"(el));
  printf("current_el: 0x%016x\n", el);
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

void set_timer_irq_address(void(*irq_addr)(void))
{
  asm volatile("msr daifset, #2\nldr x1, _vectors_irq\nldr x2, [x1]\nstr x0, [x1]\nmov x0, x2\nret\n");
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



void main()
{
  lfb_init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  uart_init();
  init_consoles();
  print_current_ex_level();
  print_mbox_props();

  unsigned long el;

  rand_init();
  // mmu_init();

  lfb_showpicture();
  // generate exception here
  // el = *(unsigned long*)0xffffffff;
  tags_print_cmdline();

  // hexdump_addr(0x100);
  asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(el));
  printf("id_aa64mmfr0_el1 is: %08x\n", el);
  // 0x0000000000001122
  // PARange  2: 40 bits, 1TB 0x2
  // ASIDBits 2: 16 bits
  // BigEnd   1: Mixed-endian support
  // SNSMem   1: Secure versus Non-secure Memory
  // TGran16  0: 16KB granule not supported
  // TGran64  0: 64KB granule is supported
  // TGran4   0: 4KB granule is supported
  // wait_timer();
  wait_gpio();

  
  unsigned long ttbr0, ttbr1, ttbcr;
  asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
  // 0x936dd22509d4006a
  // 
  asm volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
  // asm volatile("mrs %0, tcr_el1" : "=r"(ttbcr));
  printf("ttbr1_el1: %016x\n", ttbr1);

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
