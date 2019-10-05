#include <vcanvas.h>
#include <common.h>
#include <uart/uart.h>
#include <mbox/mbox.h>
#include <console_char.h>
#include "homer.h"
#include "exception.h"
#include <string.h>

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
} __attribute__ ((packed)) psf_t;

extern volatile unsigned char _binary_font_psf_start;

int vcanvas_is_initialized()
{
  return vcanvas_initialized;
}

void vcanvas_init(int width, int height)
{
  mbox[0] = 35 * 4;
  mbox[1] = MBOX_REQUEST;

  mbox[2] = MBOX_TAG_SET_PHYS_WIDTH_HEIGHT;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = width;
  mbox[6] = height;

  mbox[7] = MBOX_TAG_SET_VIRT_WIDTH_HEIGHT;
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = width;
  mbox[11] = height;

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
    fb_width = mbox[5];
    fb_height = mbox[6];
    fb_pitch = mbox[33];
    framebuf = (void*)((unsigned long)mbox[28]);
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

static void fill_background(psf_t *font, char *framebuf_off, int pitch)
{
  int x, y;
  unsigned int *pixel_addr;
  for (y = 0; y < font->height; ++y) {
    for (x = 0; x < font->width; ++x) {
      pixel_addr = (unsigned int*)(framebuf_off + y * pitch + x * 4);
      *pixel_addr = vcanvas_bg_color;
    }
  }
}

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
  int framebuf_off;
  psf_t *font = (psf_t*)&_binary_font_psf_start;
  glyph = font_get_glyph(font, chr);
  // calculate offset on screen
  framebuf_off = (*y * font->height * fb_pitch) + (*x * (font->width + 1) * 4);
  
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
    case CONSOLE_CHAR_DEL:
      if (*x > 0)
        (*x)--;
      framebuf_off = (*y * font->height * fb_pitch) + (*x * (font->width + 1) * 4);
      fill_background(font, ((char*)framebuf) + framebuf_off, fb_pitch);
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
  int width_limit, height_limit;

  if (x == 0 || y == 0 || s == 0)
    generate_exception();

  if (vcanvas_get_width_height(&width_limit, &height_limit))
    generate_exception();

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

int vcanvas_get_width_height(int *width, int *height)
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

typedef struct segment {
  int offset;
  unsigned size;
} segment_t;

static int clip_segment(segment_t *s0, segment_t *s1) {
  /*
   * x------------------x
   * .       .          .
   * .      (op)        .
   * .       .          .
   * .       x-----------------x
   * .       .          .
   * .       =          .
   * .       .          .
   * .       x----------x
   * .       .          .
   */
  if (s0->offset + s0->size > s1->offset 
   || s1->offset + s1->size < s0->offset)
    return -1;

  if (s0->offset > s1->offset) {
    if (s0->offset + s0->size > s1->offset + s1->size)
      s0->size = s1->offset + s1->size - s0->offset;
    return 1;
  }
  if (s0->offset < s1->offset) {
    s0->size -= (s1->offset - s0->offset);
    s0->offset = s1->offset;
    return 1;
  }
  
  return 0;
}

typedef struct rect {
  segment_t x;
  segment_t y;
} rect_t;

/* r0       - rectangle to clip
 * r1       - boundary rectangle 
 * our_rect - rectangle, that is the result of applying 
 *          - r1 boundary to r0 rect
 * return value - '-1', rectangle is clipped away
 *                ' 0', rectangle is not clipped
 *                ' 1', rectangle is partially clipped
 */
// static int clip_rect(
//     rect_t *r0, 
//     rect_t *r1, 
//     rect_t *out_i, 
//     intersection_type *out_t
//     )
// {
//   int partial_x, partial_y;
//   rect_t tmp = *r0;
//   partial_x = clip_segment(&tmp->x, &r1->x);
//   if (partial_x < 0)
//     return -1;
// 
//   partial_y = clip_segment(&tmp->y, &r1->y);
//   if (partial_y < 0)
//     return -1;
// 
//   *out_t = tmp;
//   return (partial_x + partial_y) ? 1 : 0;
// }

typedef struct intersection_region {
  rect_t r;
  char exists;
} intersection_region_t;

typedef struct intersection_regions {
  intersection_region_t rgs[9];
} intersection_regions_t;

#define RGN(x, y) (&(rs->rgs[y * 3 + x]))
#define RCT(x, y) (&(rs->rgs[y * 3 + x].r))

int get_intersection_regions(rect_t *r0, rect_t *r1, intersection_regions_t *rs)
{ 
  /*
   * regions is 3 x 3 matrix of rects with additional fields like
   * is this intersection exists
   */

  /*  
   *  x-------x-------x-------x
   *  |       |       |       |
   *  | [0,0] | [0,1] | [0,2] |
   *  |       |       |       |
   *  x-------x-------x-------x
   *  |       |       |       |
   *  | [1,0] | [1,1] | [1,2] |
   *  |       |       |       |
   *  x-------x-------x-------x
   *  |       |       |       |
   *  | [2,0] | [2,1] | [2,2] |
   *  |       |       |       |
   *  x-------x-------x-------x
   *
   */
  if (r0->x.offset + r0->x.size < r1->x.offset
   || r1->x.offset + r1->x.size < r0->x.offset
   || r0->y.offset + r0->y.size < r1->y.offset
   || r1->y.offset + r1->y.size < r0->y.offset)
    // no intersection
    return 0;

  memset(rs, 0, sizeof(*rs));

  RGN(1, 1)->exists = 1;
  *RCT(1, 1) = *r0;
  RCT(1, 0)->x = RCT(1, 2)->x = r0->x;
  RCT(0, 1)->y = RCT(2, 1)->y = r0->y;

  // Check interection from the left size
  if (r0->x.offset < r1->x.offset) {
  /*  
   *  x---x---x---x
   *  | x |   |   |
   *  x---x---x---x
   *  | x | x |   |
   *  x---x---x---x
   *  | x |   |   |
   *  x---x---x---x
   */
    RCT(1, 0)->x.offset = RCT(2, 0)->x.offset = RCT(0, 0)->x.offset = r1->x.offset;
    RCT(1, 0)->x.size   = RCT(2, 0)->x.size   = RCT(0, 0)->x.size = r1->x.offset - r0->x.offset;
    RGN(1, 0)->exists   = RGN(2, 0)->exists   = RGN(0, 0)->exists = 1;

    RCT(1, 1)->x.offset += RCT(0, 0)->x.size;
    RCT(1, 1)->x.size   -= RCT(0, 0)->x.size;
  }

  // Check interection from the right size
  if (r0->x.offset + r0->x.size > r1->x.offset + r1->x.size) {
  /*  
   *  x---x---x---x
   *  |   |   | x |
   *  x---x---x---x
   *  |   | x | x |
   *  x---x---x---x
   *  |   |   | x |
   *  x---x---x---x
   */
    RCT(1, 2)->x.offset = RCT(2, 2)->x.offset = RCT(0, 2)->x.offset = r0->x.offset;
    RCT(1, 2)->x.size   = RCT(2, 2)->x.size   = RCT(0, 2)->x.size   = r0->x.offset + r0->x.size - r0->x.offset - r0->x.size;
    RGN(1, 2)->exists   = RGN(2, 2)->exists   = RGN(0, 2)->exists = 1;

    RCT(1, 1)->x.size   -= RCT(0, 2)->x.size;
  }

  // Check interection from the top
  if (r0->y.offset < r1->y.offset) {
  /*  
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   |   |   |   | x | x |   |   |   |   | x |   | x |   | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   */
    RCT(0, 1)->y.offset = RCT(0, 2)->y.offset = RCT(0, 0)->y.offset = r1->y.offset;
    RCT(0, 1)->y.size   = RCT(0, 2)->y.size   = RCT(0, 0)->y.size = r1->y.offset - r0->y.offset;

    RGN(0, 1)->exists = 1;

    RCT(1, 1)->y.offset += RCT(0, 0)->y.size;
    RCT(1, 1)->y.size   -= RCT(0, 0)->y.size;
  }
  else {
    RGN(0, 0)->exists = RGN(0, 1)->exists = RGN(0, 2)->exists = 0;
  }


  // Check interection from the bottom
  if (r0->y.offset + r0->y.size > r1->y.offset + r1->y.size) {
  /*  
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   */
    RCT(0, 2)->y.offset = RCT(1, 2)->y.offset = RCT(2, 2)->y.offset = r0->y.offset;
    RCT(0, 2)->y.size   = RCT(1, 2)->y.size   = RCT(2, 2)->y.size   = r0->y.offset + r0->y.size - r1->y.offset - r1->y.size;
    RCT(1, 1)->y.size  -= RCT(1, 2)->y.size;

    RGN(1, 2)->exists   = 1;
  }
  else {
    RGN(0, 2)->exists = RGN(1, 2)->exists = RGN(2, 2)->exists = 0;
  }

  return 1;
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
