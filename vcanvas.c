#include <vcanvas.h>
#include <common.h>
#include <uart/uart.h>
#include <mbox/mbox_props.h>
#include <mbox/mbox.h>
#include <console_char.h>
#include "homer.h"
#include <exception.h>
#include <rectangle.h>
#include <stringlib.h>

#define MAX_VIEWPORTS 16

unsigned int fb_width, fb_height, fb_pitch, fb_pixelsize;
unsigned char *framebuf;
static int vcanvas_initialized = 0;
static int vcanvas_fg_color = 0;
static int vcanvas_bg_color = 0;
static int vcanvas_tabwidth = 1;

static unsigned int num_viewports = 0;
static viewport_t viewports[MAX_VIEWPORTS];


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
} packed psf_t;

extern volatile unsigned char _binary_font_psf_start;

int vcanvas_is_initialized()
{
  return vcanvas_initialized;
}

void vcanvas_init(int width, int height)
{
  mbox_buffer[0] = 35 * 4;
  mbox_buffer[1] = MBOX_REQUEST;

  mbox_buffer[2] = MBOX_TAG_SET_PHYS_WIDTH_HEIGHT;
  mbox_buffer[3] = 8;
  mbox_buffer[4] = 8;
  mbox_buffer[5] = width;
  mbox_buffer[6] = height;

  mbox_buffer[7] = MBOX_TAG_SET_VIRT_WIDTH_HEIGHT;
  mbox_buffer[8] = 8;
  mbox_buffer[9] = 8;
  mbox_buffer[10] = width;
  mbox_buffer[11] = height;

  mbox_buffer[12] = MBOX_TAG_SET_VIRT_OFFSET;
  mbox_buffer[13] = 8;
  mbox_buffer[14] = 8;
  mbox_buffer[15] = 0;
  mbox_buffer[16] = 0;

  mbox_buffer[17] = MBOX_TAG_SET_DEPTH;
  mbox_buffer[18] = 4;
  mbox_buffer[19] = 4;
  mbox_buffer[20] = 32;

  mbox_buffer[21] = MBOX_TAG_SET_PIXEL_ORDER;
  mbox_buffer[22] = 4;
  mbox_buffer[23] = 4;
  mbox_buffer[24] = 1;

  mbox_buffer[25] = MBOX_TAG_ALLOCATE_BUFFER;
  mbox_buffer[26] = 8;
  mbox_buffer[27] = 8;
  mbox_buffer[28] = 4096;
  mbox_buffer[29] = 0;

  mbox_buffer[30] = MBOX_TAG_GET_PITCH;
  mbox_buffer[31] = 4;
  mbox_buffer[32] = 4;
  mbox_buffer[33] = 0;

  mbox_buffer[34] = MBOX_TAG_LAST;
  if (!mbox_prop_call(MBOX_CH_PROP) && mbox_buffer[20] == 32 && mbox_buffer[28] != 0)
  {
    mbox_buffer[28] &= 0x3fffffff;
    fb_width = mbox_buffer[5];
    fb_height = mbox_buffer[6];
    fb_pitch = mbox_buffer[33];
    framebuf = (void*)((unsigned long)mbox_buffer[28]);
    fb_pixelsize = 4;
  }
  else
  {
    uart_puts("Unable to set screen resolution to 1024x768x32\n");
  }

  vcanvas_tabwidth = 4;
  vcanvas_initialized = 1;
  num_viewports = 0;
}

int vcanvas_get_width_height(uint32_t *width, uint32_t *height)
{
  return mbox_get_virt_wh(width, height);
}

//static void fill_background(psf_t *font, char *framebuf_off, int pitch)
//{
//  int x, y;
//  unsigned int *pixel_addr;
//  for (y = 0; y < font->height; ++y) {
//    for (x = 0; x < font->width; ++x) {
//      pixel_addr = (unsigned int*)(framebuf_off + y * pitch + x * 4);
//      *pixel_addr = vcanvas_bg_color;
//    }
//  }
//}

static unsigned int * fb_get_pixel_addr(int x, int y)
{
  return (unsigned int*)(framebuf + y * fb_pitch + x * fb_pixelsize);
}

static void vcanvas_draw_glyph(
  psf_t *font, 
  unsigned char *glyph, 
  int x, 
  int y, 
  unsigned int max_size_x, 
  unsigned int max_size_y, 
  int fg_color, 
  int bg_color)
{
  unsigned int _x, _y;
  unsigned limit_x, limit_y;
  int mask;
  unsigned int *pixel_addr;

  limit_x = min(font->width, max_size_x);
  limit_y = min(font->height, max_size_y);

  int bytes_per_line = (font->width + 7) / 8;
  for (_y = 0; _y < limit_y; ++_y) {
    mask = 1 << (font->width - 1);
    for (_x = 0; _x < limit_x; ++_x) {
      pixel_addr = fb_get_pixel_addr(x + _x, y + _y);
      asm volatile ("mov x21, %0\n" : : "r"(pixel_addr));
      asm volatile ("mov x22, %0\n" : : "r"(_x));
      asm volatile ("mov x23, %0\n" : : "r"(_y));
      asm volatile ("mov x24, %0\n" : : "r"(x));
      asm volatile ("mov x25, %0\n" : : "r"(y));
      *pixel_addr = (int)*glyph & mask ? fg_color : bg_color;
      mask >>= 1;
    }
    glyph += bytes_per_line;
  }
}


void vcanvas_set_fg_color(int value)
{
  vcanvas_fg_color = value;
}

void vcanvas_set_bg_color(int value)
{
  vcanvas_bg_color = value;
}

void vcanvas_showpicture()
{
  int x,y;
  unsigned char *ptr = framebuf;
  char *data = homer_data, pixel[4];
  ptr += (fb_height - homer_height) / 2 * fb_pitch + (fb_width - homer_width) * 2;
  for (y = 0; y < homer_height; ++y)
  {
    for (x = 0; x < homer_width; ++x)
    {
      HEADER_PIXEL(data, pixel);
      *((unsigned int*)ptr) = *((unsigned int*)&pixel);
      ptr += 4;
    }
    ptr += fb_pitch - homer_width * 4;
  }
}

unsigned char * font_get_glyph(psf_t *font, char c)
{
  int glyph_idx;
  unsigned char *glyphs;
  
  // get offset to the glyph. Need to adjust this to support unicode..
  glyphs = ((unsigned char*)font) + font->headersize;
  glyph_idx = (c < font->numglyph) ? c : 0;
  return glyphs + glyph_idx * font->bytesperglyph;
}


void vcanvas_putc(int* x, int* y, char chr)
{
  unsigned char *glyph;
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  glyph = font_get_glyph(font, chr);

  switch(chr) {
    case CONSOLE_CHAR_CARRIAGE_RETURN:
      *x = 0;
      break;
    case CONSOLE_CHAR_LINEFEED:
      *x = 0; 
      (*y)++;
      break;
    case CONSOLE_CHAR_HORIZONTAL_TAB:
      *x += vcanvas_tabwidth;
      break;
    case CONSOLE_CHAR_BACKSPACE:
    //case CONSOLE_CHAR_DEL:
      if (*x > 0)
        (*x)--;
      //framebuf_off = (*y * font->height * fb_pitch) + (*x * (font->width + 1) * 4);
      //fill_background(font, ((char*)framebuf) + framebuf_off, fb_pitch);
      break;
    default:
      vcanvas_draw_glyph(font, glyph, *x * font->width, *y * font->height, font->width, font->height, vcanvas_fg_color, vcanvas_bg_color);
      (*x)++;
      break;
  }
}


void vcanvas_puts(int *x, int *y, const char *s)
{
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  uint32_t width_limit, height_limit;

  if (x == 0 || y == 0 || s == 0)
    kernel_panic("vcanvas_puts: some args not provided.");

  if (mbox_get_virt_wh(&width_limit, &height_limit))
    kernel_panic("vcanvas_puts: Failed to get screen resolution.");
  width_limit = 300;
  height_limit = 300;

  width_limit /= (font->width + 1);
  height_limit /= (font->height + 1);

  while(*s) {
    if (*x >= (width_limit - 1)) {
      if (*y >= (height_limit - 1)) 
        break;
      *x = 0;
      (*y)++;
    }

    vcanvas_putc(x, y, *s++);
  }
}

void vcanvas_fill_rect(int x, int y, unsigned int size_x, unsigned int size_y, int rgba)
{
  int _x, _y, xend, yend;

  xend = min(x + size_x, fb_width);
  yend = min(y + size_y, fb_height);

  int *p_pixel;
  for (_x = x; _x < xend; ++_x) {
    for (_y = y; _y < yend; ++_y) {
      p_pixel = (int*)(((char*)framebuf) + _x * fb_pixelsize + _y * fb_pitch);
      *p_pixel = rgba;
    }
  }
}

viewport_t * vcanvas_make_viewport(int x, int y, unsigned int size_x, unsigned int size_y)
{
  if (num_viewports >= MAX_VIEWPORTS)
    return 0;

  viewport_t *v = &viewports[num_viewports++];
  v->pos_x = x;
  v->pos_y = y;
  v->size_x = size_x;
  v->size_y = size_y;
  return v;
}

void viewport_fill_rect(viewport_t *v, int x, int y, unsigned int size_x, unsigned int size_y, int color)
{
  int _x, _y;
  unsigned int _size_x, _size_y;

  if (x >= v->size_x || y >= v->size_y)
    return;

  _x = v->pos_x + x;
  _y = v->pos_y + y;

  _size_x = (x + size_x > v->size_x) ? v->size_x - x : size_x;
  _size_y = (y + size_y > v->size_y) ? v->size_y - y : size_y;
  
  vcanvas_fill_rect(_x, _y, _size_x, _size_y, color);
}

void viewport_fill(viewport_t *v, int color)
{
  viewport_fill_rect(v, 0, 0, v->size_x, v->size_y, color);
}

void viewport_draw_char(viewport_t *v, int x, int y, int fg_color, int bg_color, char c)
{
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  unsigned char *glyph;

  // ax, ay - absolute x and y positions
  int ax, ay;
  // size_x, size_y - max sizes of glyphs to draw
  unsigned int size_x, size_y;
  if (x >= v->size_x || y >= v->size_y)
    return;

  size_x = (x + font->width  > v->size_x) ? v->size_x - x : font->width;
  size_y = (y + font->height > v->size_y) ? v->size_y - y : font->height;

  ax = v->pos_x + x;
  ay = v->pos_y + y;
  // printf("ax, ay: %d, %d, size_x, size_y: %d %d\n", ax, ay, size_x, size_y);

  glyph = font_get_glyph(font, c);
  vcanvas_draw_glyph(font, glyph, ax, ay, size_x, size_y, fg_color, bg_color);
}

void viewport_draw_text(viewport_t *v, int x, int y, int fg_color, int bg_color, const char* text, int textlen)
{
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  const char *c;
  unsigned int char_idx;

  // rx, ry - relative to viewport 'v' x and y positions
  int rx, ry;

  if (x >= v->size_x || y >= v->size_y)
    return;

  c = text;
  char_idx = 0;
  while(*c && char_idx < textlen) {
    rx = x + char_idx * font->width;
    ry = y;

    // printf("rx, ry: %d, %d\n", rx, ry);
    if (rx >= v->size_x || ry >= v->size_y)
      return;

    viewport_draw_char(v, rx, ry, fg_color, bg_color, *c);

    char_idx++;
    c++;
  }
}

int vcanvas_get_fontsize(int *size_x, int *size_y)
{
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  if (!font)
    return -1;

  *size_x = font->width;
  *size_y = font->height;
  return 0;
}

void viewport_copy_rect_unsafe(viewport_t *v, int x0, int y0, int size_x, int size_y, int x1, int y1)
{
  int i; 
  char *src, *dst;

  src = (char *)fb_get_pixel_addr(v->pos_x + x0, v->pos_y + y0);
  dst = (char *)fb_get_pixel_addr(v->pos_x + x1, v->pos_y + y1);
  
  for (i = 0; i < size_y; i++) {
    memcpy(dst, src, size_x * fb_pixelsize);
    src += fb_pitch;
    dst += fb_pitch;
  }
}

void viewport_copy_rect(viewport_t *v, int x0, int y0, int size_x, int size_y, int x1, int y1)
{
  rect_t r0, r1;
  unsigned int xi, yi;
  intersection_region_t *ir;
  intersection_regions_t regions;
  intersection_regions_t *rs = &regions;

  r0.x.offset = x0;
  r0.x.size = size_x;
  r0.y.offset = y0;
  r0.y.size = size_y;
  r1.x.offset = 0;
  r1.x.size = v->size_x;
  r1.y.offset = 0;
  r1.y.size = v->size_y;

  if (!get_intersection_regions(&r0, &r1, rs)) {
    // r0 is outside viewport 
    viewport_fill_rect(v, x1, y1, size_x, size_y, vcanvas_bg_color);
    return;
  }
  for (yi = 0; yi < 3; yi ++) {
    for (xi = 0; xi < 3; xi ++) {
      if (xi == 1 && yi == 1)
        continue;
      ir = RGN(xi, yi);
      if (ir->exists && ir->r.x.size && ir->r.y.size)
        viewport_fill_rect(v, ir->r.x.offset - x0 + x1, ir->r.y.offset - y0 + y1, ir->r.x.size, ir->r.y.size, vcanvas_bg_color); 
    }
  }
  ir = RGN(1, 1);
  viewport_copy_rect_unsafe(v, ir->r.x.offset, ir->r.y.offset, ir->r.x.size, ir->r.y.size, ir->r.x.offset - x0 + x1, ir->r.y.offset - y0 + y1);
}
