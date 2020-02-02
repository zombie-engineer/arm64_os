#include <font.h>
#include <compiler.h>
#include <error.h>
#include <common.h>
#include <stringlib.h>
#include "font_generated.h"


#define BITMAP(_name) font_bitmap_raw_ ## _name
#define METRICS(_name) font_glyph_metrics_ ## _name
#define DECL_FONT(_name, _bits_per_pixel, _glyph_pixels_x, _glyph_pixels_y, _glyphs_per_x, _glyphs_per_y) \
{                                       \
  .name = #_name,                       \
  .bitmap = BITMAP(_name),              \
  .bitmap_sz = sizeof(BITMAP(_name)),   \
  .glyph_metrics = METRICS(_name),      \
  .bits_per_pixel = _bits_per_pixel,    \
  .glyph_pixels_x = _glyph_pixels_x,    \
  .glyph_pixels_y = _glyph_pixels_y,    \
  .glyphs_per_x   = _glyphs_per_x,      \
  .glyphs_per_y   = _glyphs_per_y,      \
  .glyph_stride   = _glyphs_per_x * _glyph_pixels_x / _bits_per_pixel\
}

static font_desc_t font_descriptors[] = {
  DECL_FONT(myfont, 8, 8, 8, 16, 6)
};

int font_init_lib()
{
  return 0;
}

int font_get_font(const char *name, const font_desc_t **f)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(font_descriptors); ++i) {
    if (!strcmp(name, font_descriptors[i].name)) {
      *f = &font_descriptors[i];
      return 0;
    }
  }
  return -1;
}

static const uint8_t *font_get_glyph(const font_desc_t *f, char c)
{   
  /*       c2      glyph_cols = 5
   *    c1 /       glyph_rows = 3
   * c0 / /c3
   * / / / /
   * 0 1 2 3 4 <- row 0
   * 5 6 7 8 9 <- row 1
   * a b c d e <- row 2 
   *
   * 'd' is r2;c4 , r =  13(0xd) / glyph_cols = 2; c = 14 % glyph_cols = 3
   */

  int x, y;
  const uint8_t *base;

  if (c < 0x20) 
    c = '.'; 
  c -= 0x20;

  y = c / f->glyphs_per_x;
  x = c % f->glyphs_per_x;
  base = (const uint8_t*)f->bitmap;
  return base + y * f->glyph_stride + x * FONT_BYTES_PER_GLYPH_X(f);
}

static int font_glyph_bit_is_set(const font_desc_t *f, const uint8_t *glyph_addr, int x, int y)
{
  int byte_off = y * f->glyph_stride + x * (f->glyph_pixels_x * (f->bits_per_pixel >> 3));
  int bit_off  = x % 8;
  return (glyph_addr[byte_off] & (1 << bit_off)) ? 1 : 0;
}

static void font_canvas_put_pixel(font_canvas_desc_t *c, int pixel, int x, int y)
{
  int byte_off, bit_off;
  char val;

  if (c->size_x <= x || c->size_y <= y)  
    return;

  byte_off = x / c->pixels_per_byte + y * c->stride;
  bit_off = x % c->pixels_per_byte;
  val = *((unsigned char *)c->addr + byte_off);
  val |= (pixel & 1) << bit_off;
  *((unsigned char *)c->addr + byte_off) = val;
}

//int font_draw_char(const font_desc_t *f, font_canvas_desc_t *c, font_draw_char_param_t *p)
//{
//  // glyph coordinates
//  const uint8_t *glyph;
//  int gx, gy;
//  int gpixel;
//
//  glyph = font_get_glyph(f, p->c);
//
//  for (gy = 0; gy < f->glyph_pixels_x; gy++) {
//    for (gx = 0; gx < f->glyph_pixels_y; ++gx) {
//      gpixel = font_glyph_bit_is_set(f, glyph, gx, gy);
//      font_canvas_put_pixel(c, gpixel, p->x + gx, p->y + gy);
//    }
//  }
//  return 0;
//}
