#include "lfb.h"

#include "uart.h"
#include "mbox.h"
#include "homer.h"

unsigned int width, height, pitch;
unsigned char *lfb;

typedef struct {
  unsigned int magic;
  unsigned int version;
  unsigned int headersize;
  unsigned int flags;
  unsigned int numglyph;
  unsigned int bytesperglyph;
  unsigned int height;
  unsigned int width;
  unsigned char glyphs;
} __attribute__ ((packed)) psf_t;

extern volatile unsigned char _binary_font_psf_start;


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

void lfb_print(int x, int y, char *s)
{
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  while(*s) {
    // get offset to the glyph. Need to adjust this to support unicode..
    unsigned char *glyph = (unsigned char*)&_binary_font_psf_start + font->headersize
      + (*((unsigned char*)s) < font->numglyph ? *s : 0) * font->bytesperglyph;
    // calculate offset on screen
    int offs = (y * font->height * pitch) + (x * (font->width + 1) * 4);
    
    int i, j, line, mask, bytesperline = (font->width + 7) / 8;
    // handle carriage return
    if (*s == '\r') {
      x = 0;
    } else
    // new line
    if (*s == '\n') {
      x = 0; y++;
    } else {
      // display character
      for (j = 0; j < font->height; ++j) {
        line = offs;
        mask = 1 << (font->width - 1);
        for (i = 0; i < font->width; ++i) {
          *((unsigned int*)(lfb + line)) = ((int)*glyph) & mask ? 0xffffff : 0;
          mask >>= 1;
          line += 4;
        }
        // adjust to next line
        glyph += bytesperline;
        offs += pitch;
      }
      x++;
    }
    // next character
    s++;
  }
}

void lfb_print_long_hex(int x, int y, unsigned long val)
{
  char* dump_place = (char*)0x90000;
  char* ptr = dump_place;
  unsigned long n;
  int c;
  for(c = 60; c >= 0; c -=4) {
    n = (val >> c) & 0xf;
    n += n > 9 ? 0x37 : 0x30;
    *ptr++ = n;
  }
  *ptr = 0;
  lfb_print(x, y, dump_place);
}
