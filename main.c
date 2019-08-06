#include "uart.h"
#include "mbox.h"

void main()
{
  uart_init();
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

  while(1) {
    char c = uart_getc();
    uart_send(c);
  }
}
