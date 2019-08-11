#include "uart.h"
#include "mbox.h"
#include "lfb.h"
#include "rand.h"
#include "delays.h"
#include "mmu.h"
#include "common.h"
#include "sprintf.h"
#include "console.h"
#include "mbox_props.h"
#include "gpio.h"

void print_current_ex_level()
{
  unsigned long el;
  asm volatile("mrs %0, CurrentEL" : "=r"(el));
  printf("current_el: 0x%016x\n", el);
}

void print_mbox_props()
{
  int val, val2;
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
}

typedef struct tag {
  unsigned int size;
  unsigned short id;
  unsigned short magic;
  char data[0];
} __attribute__((packed)) tag_t;

void print_cmdline()
{
  volatile unsigned int *tag_base_ptr = (volatile unsigned int *)0x100;
  volatile tag_t *tag = (volatile tag_t *)tag_base_ptr;
  while(1)
  {
    if ((tag->size & 0xff) == 0)
      break;
    if (tag->id == 9)
    {
      printf("0x%08x: tag cmdline=%s\n", *(volatile unsigned int*)(tag->data), (volatile const char*)tag->data);
      break;
    }
    else
    {
      tag = (volatile tag_t*)(((volatile unsigned int*)(tag)) + tag->size);
    }
  }
}

void main()
{
  lfb_init();
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
  print_cmdline();

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
  int i;
  volatile unsigned int *base_ptr = (volatile unsigned int *)0x100;
  for (i = 0; i < 32; ++i)
  {
    printf("%08x: %08x %08x %08x %08x\n", 
        base_ptr + i * 4,
        *(base_ptr + i * 4 + 0),
        *(base_ptr + i * 4 + 1),
        *(base_ptr + i * 4 + 2),
        *(base_ptr + i * 4 + 3)
    );
  }


  wait_msec(200000);
  printf("entering loop...\n");
  gpio_set_function(21, GPIO_FUNC_OUT);
  while(1)
  {
    wait_cycles(0x1800000);
    printf(".");
    gpio_set_on(21);
    wait_cycles(0x1800000);
    gpio_set_off(21);
  }
  
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
