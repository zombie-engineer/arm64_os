#include <viewport_console.h>
#include <vcanvas.h>
#include <console_char.h>
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
  int linenum;
} textviewport_t;

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

  tv.viewport = vcanvas_make_viewport(20, 20, 768, 512);
  vcanvas_get_fontsize(&tv.fontsize_x, &tv.fontsize_y);
  tv.maxchars_x = tv.viewport->size_x / tv.fontsize_x;
  tv.maxchars_y = tv.viewport->size_y / tv.fontsize_y;
  tv.linenum = 0;
  viewport_fill(tv.viewport, tv.bg_color);
  viewport_draw_text(tv.viewport, 0, 0, tv.fg_color, tv.bg_color, "hello", 5);
  return 0;
}

static const char* textbuf_skiplines(const char *textbuf, int n)
{
  int i;
  const char *ptr;
  ptr = textbuf;
  i = 0;
  while(*ptr && i != n) {
    if (i == n)
      break;
    if (*ptr == '\n')
      ++i;
    ++ptr;
  }
  return ptr;
}

static void textviewport_print_char(textviewport_t *tv, char c)
{
  int pos_x = tv->charpos_x * tv->fontsize_x;
  int pos_y = tv->charpos_y * tv->fontsize_y;
  viewport_draw_char(tv->viewport, pos_x, pos_y, tv->fg_color, tv->bg_color, c);
  tv->charpos_x++;
}

static void textviewport_scrolldown(textviewport_t *tv, unsigned int lines)
{
  unsigned int i, pos_x, pos_y0, pos_y1, size_x, size_y;
  size_x = tv->fontsize_x * tv->maxchars_x;
  size_y = tv->fontsize_y;
  pos_x = 0;
  while(tv->charpos_y && lines) {
    for (i = 0; i < tv->maxchars_y; ++i) {
      pos_y0 = tv->fontsize_y * (i + 1); 
      pos_y1 = pos_y0 - size_y;
      viewport_copy_rect(tv->viewport, pos_x, pos_y0, size_x, size_y, pos_x, pos_y1);
    }
    tv->charpos_y--;
    lines--;
  }
}

static void textviewport_newline(textviewport_t *tv)
{

  tv->charpos_x = 0;
  tv->charpos_y++;
  if (tv->charpos_y == tv->maxchars_y)
    textviewport_scrolldown(tv, 1);
}

static void textviewport_backspace(textviewport_t *tv)
{
  if (tv->charpos_x) {
    tv->charpos_x--;
    viewport_fill_rect(tv->viewport, 
      tv->charpos_x * tv->fontsize_x,
      tv->charpos_y * tv->fontsize_y,
      tv->fontsize_x,
      tv->fontsize_y, tv->bg_color);
  }
}

void viewport_console_puts(const char *str)
{
  const char *ptr;
  ptr = str;
  while(*ptr)
    viewport_console_putc(*ptr++);
}

void viewport_console_putc(char c)
{
  switch (c) {
    case '\n':
      *(tv.textbuflast++) = c;
      *tv.textbuflast = 0;
      textviewport_newline(&tv);
      break;
    case CONSOLE_CHAR_BACKSPACE:
      *(tv.textbuflast--) = 0;
      textviewport_backspace(&tv);
      break;
    default:
      *(tv.textbuflast++) = c;
      *tv.textbuflast = 0;
      if (tv.charpos_x >= tv.maxchars_x)
        textviewport_newline(&tv);
      textviewport_print_char(&tv, c);
      break;
  }
}
