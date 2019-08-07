#include "uart.h"
#include "mbox.h"
#include "lfb.h"
#include "rand.h"
#include "delays.h"

void main()
{
  uart_init();
  lfb_init();

  rand_init();


  lfb_showpicture();
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
  wait_cycles(0x7ffffff);
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec(200000);
  uart_puts("wait complete.\n");

  uart_puts("waiting 200000 cycles\n");
  wait_msec_st(4000000);
  uart_puts("wait complete.\n");
  while(1) {
    char c = uart_getc();
    uart_send(c);
  }
}
