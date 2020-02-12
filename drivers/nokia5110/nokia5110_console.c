#include <drivers/display/nokia5110_console.h>
#include <drivers/display/nokia5110.h>
#include <error.h>
#include <debug.h>

static nokia5110_console_control_t nokia5110_console_control = {
  .initialized = 0,
  .cursor_x = 0,
  .cursor_y = 0
};

#define CONSOLE_CTL() nokia5110_console_control_t *ctl = &nokia5110_console_control

int nokia5110_term_new_line()
{
  return ERR_OK;
}

int nokia5110_term_carriage_return()
{
  return ERR_OK;
}

int nokia5110_term_draw_char(char c)
{
  CONSOLE_CTL();
  ctl->cursor_x = ctl->cursor_y = -1; 
  nokia5110_draw_char(ctl->cursor_x, ctl->cursor_y, c);
  // ctl->cursor_x++;
  return ERR_OK;
}

int nokia5110_putc(char c)
{
  switch(c) {
    case '\n': return nokia5110_term_new_line();
    case '\r': return nokia5110_term_carriage_return();
    default  : return nokia5110_term_draw_char(c);
  }
}

int nokia5110_puts(const char *c)
{
  int ret;
  while(*c) {
    ret = nokia5110_putc(*c++);
    if (ret)
      return ret;
  }
  return ERR_OK;
}

static console_dev_t nokia5110_console_dev = {
  .puts = nokia5110_puts,
  .putc = nokia5110_putc
};

int nokia5110_console_init()
{
  nokia5110_console_control.cursor_x = nokia5110_console_control.cursor_y = 0;
  nokia5110_console_control.initialized = 1;
  return ERR_OK;
}

console_dev_t *nokia5110_get_console_device()
{
  if (!nokia5110_console_control.initialized)
    return 0;
  return &nokia5110_console_dev;
}

