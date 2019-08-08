#include "uart.h"
#include "mbox.h"
#include "lfb.h"
#include "rand.h"
#include "delays.h"

void main()
{
  unsigned long el;
  uart_init();
  lfb_init();

  rand_init();
  asm volatile("mrs %0, CurrentEL" : "=r"(el));
  uart_puts("Current EL is: ");
  uart_hex((el >> 2) & 3);
  uart_puts("\n");

  lfb_showpicture();
  lfb_print(10, 5, "Hello!");
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GETSERIAL;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;

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
