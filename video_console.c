#include <video_console.h>
#include <lfb.h>

typedef struct console_screen {
  int x;
  int y;
  int width;
  int height;
} console_screen_t;

static console_screen_t sc;

int video_console_init()
{
  if (!lfb_is_initialized())
    return -1;

  sc.x = 0;
  sc.y = 0;
  lfb_get_width_height(&sc.width, &sc.height);
  return 0;
}


void video_console_puts(const char *str)
{
  lfb_puts(&sc.x, &sc.y, str);
}

void video_console_putc(char chr)
{
  lfb_putc(&sc.x, &sc.y, chr);
}
