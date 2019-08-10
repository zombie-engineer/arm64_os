#include "lfb.h"

#include "uart.h"
#include "mbox.h"
#include "homer.h"

unsigned int width, height, pitch;
unsigned char *lfb;
static int lfb_initialized = 0;

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

int lfb_is_initialized()
{
  return lfb_initialized;
}

void lfb_init()
{
  mbox[0] = 35 * 4;
  mbox[1] = MBOX_REQUEST;

  mbox[2] = MBOX_TAG_SET_PHYS_WIDTH_HEIGHT;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024;
  mbox[6] = 768;

  mbox[7] = MBOX_TAG_SET_VIRT_WIDTH_HEIGHT;
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024;
  mbox[11] = 768;

  mbox[12] = MBOX_TAG_SET_VIRT_OFFSET;
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0;
  mbox[16] = 0;

  mbox[17] = MBOX_TAG_SET_DEPTH;
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32;

  mbox[21] = MBOX_TAG_SET_PIXEL_ORDER;
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1;

  mbox[25] = MBOX_TAG_ALLOCATE_BUFFER;
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096;
  mbox[29] = 0;

  mbox[30] = MBOX_TAG_GET_PITCH;
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
  lfb_initialized = 1;
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

static void print_glyph(unsigned char* glyph_off, psf_t *font, char *framebuf_off, int pitch)
{
  int x, y;
  int mask;
  unsigned int *pixel_addr;
  unsigned int glyph_pixel;
  int bytes_per_line = (font->width + 7) / 8;
  for (y = 0; y < font->height; ++y) {
    mask = 1 << (font->width - 1);
    for (x = 0; x < font->width; ++x) {
      pixel_addr = (unsigned int*)(framebuf_off + y * pitch + x * 4);
      glyph_pixel = (int)*glyph_off & mask ? 0xffffff : 0;
      *pixel_addr = glyph_pixel;
      mask >>= 1;
    }
    glyph_off += bytes_per_line;
  }
}

void lfb_putc(int* x, int* y, char chr)
{
  unsigned char *glyphs, *glyph;
  int glyph_idx;
  int framebuf_off;
  psf_t *font = (psf_t*)&_binary_font_psf_start;

  // get offset to the glyph. Need to adjust this to support unicode..
  glyphs = (unsigned char*)&_binary_font_psf_start + font->headersize;
  glyph_idx = 0;
  if (chr < font->numglyph)
    glyph_idx = chr;
  glyph = glyphs + glyph_idx * font->bytesperglyph;
  // calculate offset on screen
  framebuf_off = (*y * font->height * pitch) + (*x * (font->width + 1) * 4);
  
  // handle carriage return
  if (chr == '\r')
    *x = 0;
  else
  // new line
  if (chr == '\n') {
    *x = 0; 
    (*y)++;
  } else {
    print_glyph(glyph, font, ((char*)lfb) + framebuf_off, pitch);
    (*x)++;
  }
}


void lfb_puts(int *x, int *y, const char *s)
{
  while(*s) {
    lfb_putc(x, y, *s++);
  }
}

int lfb_get_width_height(int *width, int *height)
{
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;

  mbox[2] = MBOX_TAG_GET_VIRT_WIDTH_HEIGHT;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP))
  {
    *width = mbox[5];
    *height = mbox[6];
    return 0;
  }

  uart_puts("Unable to set screen resolution to 1024x768x32\n");
  return -1;
}

