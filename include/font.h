#pragma once

typedef struct font_desc {
  char name[32];
  void *bitmap;
  int bitmap_sz;
  /* Number of bits that describe one pixel, 1-monochrome, 8 - grayscale, etc.. */
  int bits_per_pixel;
  /* Number of pixels per glyph x dimension */
  int glyph_pixels_x;
  /* Number of pixels per glyph y dimension */
  int glyph_pixels_y;
  /* Number of glyphs on bitmap plate per x dimension */
  int glyphs_per_x;
  /* Number of glyphs on bitmap plate per y dimension */
  int glyphs_per_y;

  /* Number of bytes to offset from current glyph pixel 
   * to get to pixel right below current one. y+1
   */
  int glyph_stride;
} font_desc_t;

typedef struct glyph_desc {
  const void *addr;
  const font_desc_t *font;
} glyph_desc_t;

#define FONT_BYTES_PER_GLYPH_X(_f) (_f->glyph_pixels_x * _f->bits_per_pixel >> 3)
#define FONT_GLYPH_STRIDE(_f) (_f->glyphs_per_x * FONT_BYTES_PER_GLYPH_X(_f))

typedef struct font_canvas_desc {
  void *addr;
  int bytes_size;
  int stride;
  int size_x;
  int size_y;
  int pixels_per_byte;
} font_canvas_desc_t;

int font_init_lib();

int font_get_font(const char *name, const font_desc_t **f);

typedef struct font_draw_char_param {
  char c;
  int x;
  int y;
} font_draw_char_param_t;

int font_draw_char(const font_desc_t *f, font_canvas_desc_t *c, font_draw_char_param_t *p);
