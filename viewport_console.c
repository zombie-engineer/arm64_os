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

  tv.viewport = vcanvas_make_viewport(20, 20, 200, 128);
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


//typedef int (*foreachline_cb)(const char *, const char *, void *);
//
//static void textviewport_println(textviewport_t *tv, const char *text, unsigned int len);
//
//static int textbuf_cb_println(const char *start, const char *end, void *arg)
//{
//  textviewport_println((textviewport_t*)arg, start, end - start);
//  return 0;
//}
//
//static void textbuf_foreach_line(const char *start, const char *end, foreachline_cb cb, void *cb_arg)
//{
//  const char *ptr;
//  const char *linestart;
//  linestart = ptr = start;
//  while(ptr < end && *ptr) {
//    if (*ptr == '\n') {
//      if (cb(linestart, ptr, cb_arg))
//        break;
//      linestart = ptr + 1;
//    }
//    ptr++;
//  }
//  cb(linestart, ptr, cb_arg);
//}
//
//static void textviewport_redraw(textviewport_t *tv)
//{
//  const char *ptr;
//  tv->charpos_x = tv->charpos_y = 0;
//  viewport_fill(tv->viewport, tv->bg_color);
//  ptr = textbuf_skiplines(tv->textbuf, tv->linenum);
//  textbuf_foreach_line(ptr, tv->textbuflast + 1, textbuf_cb_println, tv);
//}
//
//static void textviewport_set_linenum(textviewport_t *tv, int linenum) 
//{
//  tv->linenum = linenum;
//  textviewport_redraw(tv);
//}
//
//
//typedef struct textbuf_cb_getnumlines_arg {
//  textviewport_t *tv;
//  unsigned int numlines;
//} textbuf_cb_getnumlines_arg_t;
//
//static int textbuf_cb_getnumlines(const char *start, const char *end, void *arg)
//{
//  textbuf_cb_getnumlines_arg_t *state = (textbuf_cb_getnumlines_arg_t*)arg;
//  if (end - start < state->tv->maxchars_x) {
//    state->numlines++;
//    return 0;
//  }
//  state->numlines += (end - start + state->tv->maxchars_x - 1) / state->tv->maxchars_x;
//  return 0;
//}
//
//static unsigned int textviewport_get_numlines(textviewport_t *tv, const char *start, const char *end)
//{
//  textbuf_cb_getnumlines_arg_t state;
//  state.tv = tv;
//  state.numlines = 0;
//  textbuf_foreach_line(start, end, textbuf_cb_getnumlines, &state);
//  return state.numlines;
//}
//
//static void textviewport_nextline(textviewport_t *tv)
//{
//  tv->charpos_x = 0;
//  tv->charpos_y++;
//
//  if (tv->charpos_y == tv->maxchars_y) {
//    int numlines = textviewport_get_numlines(tv, tv->textbuf, tv->textbuflast + 1);
//    textviewport_set_linenum(tv, numlines + 1 - tv->maxchars_y);
//  }
//}
//
//static void textviewport_println_single(textviewport_t *tv, const char *text, unsigned int len) 
//{
//  int pos_x = tv->charpos_x * tv->fontsize_x;
//  int pos_y = tv->charpos_y * tv->fontsize_y;
//  viewport_draw_text(tv->viewport, pos_x, pos_y, tv->fg_color, tv->bg_color, text, len);
//  textviewport_nextline(tv);
//}

static void textviewport_print_char(textviewport_t *tv, char c)
{
  //if (c == '\n' || tv->charpos_x >= tv->maxchars_x) {
  //  textviewport_nextline(tv);
  //  return;
  //}
  
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
    default:
      *(tv.textbuflast++) = c;
      *tv.textbuflast = 0;
      if (tv.charpos_x >= tv.maxchars_x)
        textviewport_newline(&tv);
      textviewport_print_char(&tv, c);
      break;
  }
}
