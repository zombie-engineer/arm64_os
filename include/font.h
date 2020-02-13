#pragma once

typedef struct font_desc font_desc_t;

typedef struct glyph_metrics {
  /* x, y coordinates of a glyph on a bitmap plate */
  int pos_x;
  int pos_y;
  /* x, y sizes of bounding box of a glyph */
  int bound_x;
  int bound_y;
  /* x, y offsets from the current cursor point
   * until the start of the leftmost/bottommost boundary
   * of a glyph
   */
  int bearing_x;
  int bearing_y;
  /* offsets of a cursor after the glyph has been drawn */
  int advance_x;
  int advance_y;
} font_glyph_metrics_t;

typedef struct font_desc {
  /* Font name */
  char name[32];
  /* Address to buffer that holds bitmap plate */
  void *bitmap;
  /* Size of a bitmap buffer */
  int bitmap_sz;

  /* Address of glyph_metrics array */
  font_glyph_metrics_t *glyph_metrics;
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

static inline const font_glyph_metrics_t *font_get_glyph_metrics(const font_desc_t *f, char c)
{
  int idx;
  idx = c - 0x20;
  if (idx < 0)
    idx = '.' - 0x20;

  return &f->glyph_metrics[idx];
}

static inline int glyph_metrics_get_offset(const font_desc_t *f, const font_glyph_metrics_t *gm)
{
  int glyph_y = gm->pos_y - gm->bound_y + 1;
  return glyph_y * f->glyph_stride + (gm->pos_x >> 3);
}

static inline int glyph_metrics_get_cursor_step_x(const font_glyph_metrics_t *gm)
{
  return gm->bearing_x + gm->bound_x + gm->advance_x;
}


// int font_draw_char(const font_desc_t *f, font_canvas_desc_t *c, font_draw_char_param_t *p);
