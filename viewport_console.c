#include <viewport_console.h>
#include <vcanvas.h>
#include <common.h>

#define VIEWPORT_TEXTBUF_LEN 4096

typedef struct text_viewport {
  int maxchars_x;
  int maxchars_y;
  char *textbuf;
  char *textbufend;
  char *textbuflast;

  viewport_t *viewport;
  int text_wrapping;
  int fg_color;
  int bg_color;
  int fontsize_x;
  int fontsize_y;
  int charpos_x;
  int charpos_y;
} textviewport_t ;

static textviewport_t tv;

static char viewport_textbuf[VIEWPORT_TEXTBUF_LEN];

int viewport_console_init()
{
  tv.fg_color = 0x00ffffaa;
  tv.bg_color = 0x00111111;
  tv.charpos_x = 0;
  tv.charpos_y = 0;
  tv.textbuf = viewport_textbuf;
  tv.text_wrapping = 1;
  tv.textbufend = tv.textbuf + VIEWPORT_TEXTBUF_LEN;
  tv.textbuflast = tv.textbuf;

  tv.viewport = vcanvas_make_viewport(20, 20, 400, 400);
  vcanvas_get_fontsize(&tv.fontsize_x, &tv.fontsize_y);
  tv.maxchars_x = tv.viewport->size_x / tv.fontsize_x;
  tv.maxchars_y = tv.viewport->size_y / tv.fontsize_y;
  viewport_fill(tv.viewport, 0x00101010);
  viewport_draw_text(tv.viewport, 0, 0, tv.fg_color, tv.bg_color, "hello", 5);
  return 0;
}

static void textviewport_println_single(textviewport_t *tv, const char *text, unsigned int len) 
{
  int pos_x = tv->charpos_x * tv->fontsize_x;
  int pos_y = tv->charpos_y * tv->fontsize_y;
  viewport_draw_text(tv->viewport, pos_x, pos_y, tv->fg_color, tv->bg_color, text, len);
  tv->charpos_x = 0;
  tv->charpos_y++;
}

static void textviewport_println_wrapped(textviewport_t *tv, const char *text, unsigned int len) 
{
  const char *nextline;
  int clippedlen, toprint;
  toprint = len;
  nextline = text;
  while(toprint) {
    clippedlen = min(toprint, tv->maxchars_x - tv->charpos_x);
    toprint -= clippedlen;
    textviewport_println_single(tv, nextline, clippedlen);
    nextline += clippedlen;
  }
}

static void textviewport_println(textviewport_t *tv, const char *text, unsigned int len)
{
  if (!len)
    return;

  if (tv->text_wrapping)
    textviewport_println_wrapped(tv, text, len);
  else
    textviewport_println_single(tv, text, len);
}

void viewport_console_puts(const char *str)
{
  const char *linestart;
  const char *ptr;

  linestart = ptr = str;
  while(*ptr) {
    *(tv.textbuflast++) = *ptr;
    if (*ptr == '\n') {
      textviewport_println(&tv, linestart, ptr - linestart);
      linestart = ptr + 1;
    }
    ptr++;
  }
  textviewport_println(&tv, linestart, ptr - linestart);
}

void viewport_console_putc(char c)
{
  viewport_draw_text(tv.viewport, tv.charpos_x * tv.fontsize_x, tv.charpos_y * tv.fontsize_y, 0x00ff0000, tv.bg_color, "u", 1);
  tv.charpos_x++;
  tv.charpos_y++;
}
