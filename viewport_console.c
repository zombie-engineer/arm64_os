#include <viewport_console.h>
#include <vcanvas.h>

viewport_t *viewport;

static int viewport_fg_color;
static int viewport_bg_color;
static int viewport_x;
static int viewport_y;

int viewport_console_init()
{
  viewport_fg_color = 0x00ffffaa;
  viewport_bg_color = 0x00111111;
  viewport_x = 0;
  viewport_y = 0;

  viewport = vcanvas_make_viewport(20, 20, 400, 400);
  viewport_fill(viewport, 0x00101010);
  viewport_draw_text(viewport, 0, 0, viewport_fg_color, viewport_bg_color, "hello", 5);
  return 0;
}

void viewport_console_puts(const char *str)
{
  viewport_draw_text(viewport, viewport_x, viewport_y, viewport_fg_color, viewport_bg_color, str, 5);
  viewport_y += 16;
}

void viewport_console_putc(char c)
{
  viewport_draw_char(viewport, viewport_x, viewport_y, viewport_fg_color, viewport_bg_color, c);
}
