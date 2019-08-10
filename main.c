#include "uart.h"
#include "mbox.h"
#include "lfb.h"
#include "rand.h"
#include "delays.h"
#include "mmu.h"
#include "sprintf.h"

unsigned long get_serial()
{
  unsigned long res;

  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GETSERIAL;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;
  res = mbox[5] << 32 | mbox[6];
  return res;
}

void print_current_ex_level()
{
  unsigned long el;
  asm volatile("mrs %0, CurrentEL" : "=r"(el));
  lfb_print(0, 0, "current_el:");
  lfb_print_long_hex(0, 1, el);
}

void main()
{
  lfb_init();
  print_current_ex_level();

  char buf[256];
  sprintf(buf, "my some: %x \n", 345);
  unsigned long el;
  uart_init();

  rand_init();
  mmu_init();

  lfb_showpicture();
  // generate exception here
  // el = *(unsigned long*)0xffffffff;


  asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(el));
  lfb_print(0, 2, "id_aa64mmfr0_el1 is: ");
  // 0x0000000000001122
  // PARange  2: 40 bits, 1TB 0x2
  // ASIDBits 2: 16 bits
  // BigEnd   1: Mixed-endian support
  // SNSMem   1: Secure versus Non-secure Memory
  // TGran16  0: 16KB granule not supported
  // TGran64  0: 64KB granule is supported
  // TGran4   0: 4KB granule is supported

  lfb_print_long_hex(0, 3, el);
  while(1);
  
  unsigned long ttbr0, ttbr1, ttbcr;
  asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0));
  // 0x936dd22509d4006a
  // 
  asm volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
  // asm volatile("mrs %0, tcr_el1" : "=r"(ttbcr));
  lfb_print(10, 5, "Hello!");
  lfb_print_long_hex(10, 6, ttbr0);
  lfb_print_long_hex(10, 7, ttbr1);
  lfb_print_long_hex(10, 8, ttbcr);

  if (mbox_call(MBOX_CH_PROP)) {
    uart_puts("My serial number is: ");
    uart_hex(mbox[6]);
    uart_hex(mbox[5]);
    uart_puts("\n");
  } else {
    uart_puts("Unable to query serial!\n");
  }

  uart_puts("waiting 20000 cycles\n");
  wait_cycles(0x7ffff);
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec(2000);
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec_st(40000);
  uart_puts("wait complete.\n");
  int i = 0;
  while(1) {
    char c = uart_getc();
    lfb_print(10 + i++, 5, "t");
    uart_send(c);
  }
}
