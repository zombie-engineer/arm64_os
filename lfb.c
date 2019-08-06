#include "lfb.h"

#include "uart.h"
#include "mbox.h"
#include "homer.h"

unsigned int width, height, pitch;
unsigned char *lfb;


void lfb_init()
{
  mbox[0] = 35 * 4;
  mbox[1] = MBOX_REQUEST;

  mbox[2] = 0x48003;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024;
  mbox[6] = 768;

  mbox[7] = 0x48004;
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024;
  mbox[11] = 768;

  mbox[12] = 0x48009;
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0;
  mbox[16] = 0;

  mbox[17] = 0x48005;
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32;

  mbox[21] = 0x48006;
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1;

  mbox[25] = 0x40001;
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096;
  mbox[29] = 0;

  mbox[30] = 0x40008;
  mbox[31] = 4;
  mbox[32] = 4;
  mbox[33] = 0;

  mbox[34] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0)
  {
    mbox[28] &= 0x3fffffff;
    width = mbox[5];
    height = mbox[6];
    pitch = mbox[33];
    lfb = (void*)((unsigned long)mbox[28]);
  }
  else
  {
    uart_puts("Unable to set screen resolution to 1024x768x32\n");
  }
}

void lfb_showpicture()
{
  int x,y;
  unsigned char *ptr = lfb;
  char *data = homer_data, pixel[4];
  ptr += (height - homer_height) / 2 * pitch + (width - homer_width) * 2;
  for (y = 0; y < homer_height; ++y)
  {
    for (x = 0; x < homer_width; ++x)
    {
      HEADER_PIXEL(data, pixel);
      *((unsigned int*)ptr) = *((unsigned int*)&pixel);
      ptr += 4;
    }
    ptr += pitch - homer_width * 4;
  }
}
