#include <drivers/display/nokia5110.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>
#include <gpio_set.h>
#include <stringlib.h>
#include <font.h>
#include <spinlock.h>
#include <cpu.h>
#include <debug.h>
#include "nokia5110_internal.h"

#define NOKIA5110_FUNCTION_SET(PD, V, H) (0b00100000 | (PD << 2) | (V << 1) | (H << 0))
#define PD_BIT_SET 1
#define PD_BIT_CLEAR 0

#define V_BIT_SET 1
#define V_BIT_CLEAR 0

#define H_BIT_SET 1
#define H_BIT_CLEAR 0

#define NOKIA5110_DISPLAY_CONTROL(D, E) (0b00001000 | (D << 2) | (E << 0))

#define NOKIA5110_DISPLAY_CONTROL_BLANK      NOKIA5110_DISPLAY_CONTROL(0, 0)
#define NOKIA5110_DISPLAY_CONTROL_NORMAL     NOKIA5110_DISPLAY_CONTROL(1, 0)
#define NOKIA5110_DISPLAY_CONTROL_ALL_SEG_ON NOKIA5110_DISPLAY_CONTROL(0, 1)
#define NOKIA5110_DISPLAY_CONTROL_INVERTED   NOKIA5110_DISPLAY_CONTROL(1, 1)

// Temperature control 0 - 3 0 is more contrast, 3 - is less contrast
#define NOKIA5110_TEMP_CONTROL(value) (0b00000100 | (value & 3))
#define NOKIA5110_BIAS_SYSTEM(value)  (0b00010000 | (value & 7))
#define NOKIA5110_SET_VOP(value)      (0b10000000 | (value & 0x7f))

#define DISPLAY_MODE_MAX 3

#define NOKIA5110_CANVAS_OFFSET(x, y) (x + NOKIA5110_WIDTH * (y >> NOKIA5110_PIXELS_PER_BYTE_LOG))

#define NOKIA5110_CANVAS_SET_XY(c, x, y, v) \
  *(uint8_t*)(c->canvas + NOKIA5110_CANVAS_OFFSET(x, y)) = v

#define NOKIA5110_CANVAS_SET(c, v) \
  NOKIA5110_CANVAS_SET_XY(c, c->cursor_x, c->cursor_y, v)

DECL_GPIO_SET_KEY(nokia5110_gpio_set_key, "NOKIA5110_GPIO0");

typedef struct nokia5110_dev {
  spi_dev_t *spi;
  uint32_t dc;
  uint32_t rst;
  gpio_set_handle_t gpioset;
} nokia5110_dev_t;

typedef struct nokia5110_canvas_control {
  uint8_t *canvas;
  int cursor_x;
  int cursor_y;
  const font_desc_t *font;
} nokia5110_canvas_control_t;

static const font_desc_t *nokia5110_font;
static nokia5110_dev_t nokia5110_device;
static nokia5110_dev_t *nokia5110_dev = 0;
static struct spinlock nokia5110_lock;

static uint8_t nokia5110_canvas[NOKIA5110_CANVAS_SIZE];

#define CHECKED_FUNC(fn, ...) DECL_FUNC_CHECK_INIT(fn, nokia5110_dev, __VA_ARGS__)

// sets DC pin to DATA, tells display that this byte should be written to display RAM
#define SET_DATA() RET_IF_ERR(gpio_set_on , nokia5110_dev->dc)
// sets DC pint to CMD, tells display that this byte is a command
#define SET_CMD()  RET_IF_ERR(gpio_set_off, nokia5110_dev->dc)

#define SPI_SEND(data, len)  RET_IF_ERR(nokia5110_dev->spi->xmit, nokia5110_dev->spi, data, 0, len)

#define SPI_SEND_DMA(data, len)  RET_IF_ERR(nokia5110_dev->spi->xmit_dma, nokia5110_dev->spi, data, 0, len)

#define SPI_SEND_BYTE(data)  RET_IF_ERR(nokia5110_dev->spi->xmit_byte, nokia5110_dev->spi, data, 0)

#define SEND_CMD(cmd)        SET_CMD(); SPI_SEND_BYTE((cmd));

#define SEND_DATA(data, len) SET_DATA(); SPI_SEND(data, len)

int nokia5110_send_data(const void *data, size_t sz) 
{
  int ret;
  gpio_set_on(nokia5110_dev->dc);
  ret = nokia5110_dev->spi->xmit(nokia5110_dev->spi, data, 0, sz);
  return ret;
}

int nokia5110_send_data_dma(const void *data, size_t sz)
{
  int ret;
  gpio_set_on(nokia5110_dev->dc);
  ret = nokia5110_dev->spi->xmit_dma(nokia5110_dev->spi, data, 0, sz);
  return ret;
}

#define SEND_DATA_DMA(data, len) 



#define NOKIA5110_INSTRUCTION_SET_NORMAL 0
#define NOKIA5110_INSTRUCTION_SET_EX     1
//static CHECKED_FUNC(nokia5110_set_instructions, int instruction_set)
//  int err;
//  if (instruction_set == NOKIA5110_INSTRUCTION_SET_NORMAL) {
//    SEND_CMD(0xc0);
//  }
//  else {
//    SEND_CMD(0x21);
//  }
//  return ERR_OK;
//}

int nokia5110_init(spi_dev_t *spidev, uint32_t rst_pin, uint32_t dc_pin, int function_flags, int display_mode)
{ 
  int err;
  if (!spidev)
    return ERR_INVAL_ARG;

  int pins[2] = { rst_pin, dc_pin };

  gpio_set_handle_t gpio_set_handle;
  gpio_set_handle = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins), nokia5110_gpio_set_key);

  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
    puts("Failed to request gpio pins for RST,DC pins.\n");
    return ERR_BUSY;
  }

  RET_IF_ERR(gpio_set_function, rst_pin, GPIO_FUNC_OUT);
  RET_IF_ERR(gpio_set_function, dc_pin, GPIO_FUNC_OUT);

  nokia5110_device.spi = spidev;
  nokia5110_device.dc = dc_pin;
  nokia5110_device.rst = rst_pin;
  nokia5110_device.gpioset = gpio_set_handle;
  nokia5110_dev = &nokia5110_device;

  RET_IF_ERR(gpio_set_on, nokia5110_device.rst);
  RET_IF_ERR(gpio_set_off, nokia5110_device.dc);
  RET_IF_ERR(nokia5110_reset);

  SEND_CMD(NOKIA5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_SET));
  SEND_CMD(NOKIA5110_SET_VOP(0x39));
  SEND_CMD(NOKIA5110_TEMP_CONTROL(0));
  SEND_CMD(NOKIA5110_BIAS_SYSTEM(4));
  SEND_CMD(NOKIA5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_CLEAR));
  SEND_CMD(NOKIA5110_DISPLAY_CONTROL_NORMAL);

  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  nokia5110_font = 0;
  cond_spinlock_init(&nokia5110_lock);

  puts("NOKIA5110: init is complete\n");
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_function, int function_flags)
  int err;
  SEND_CMD(0b00100000 | function_flags);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_display_mode, int display_mode)
  int err;
  char bit_D;
  char bit_E;
  if (display_mode > DISPLAY_MODE_MAX)
    return ERR_INVAL_ARG;

  bit_D = (display_mode >> 1) & 1;
  bit_E = display_mode & 1;

  SEND_CMD(0b00001000 | (bit_D << 2) | bit_E);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_temperature_coeff, int coeff)
  int err;
  if (coeff > NOKIA5110_TEMP_COEFF_MAX)
    return ERR_INVAL_ARG;
  SEND_CMD(0b00000100 | coeff);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_bias, int bias)
  int err;
  if (bias > NOKIA5110_BIAS_MAX)
    return ERR_INVAL_ARG;
  SEND_CMD(bias);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_reset)
  int err;
  RET_IF_ERR(gpio_set_off, nokia5110_dev->rst);
  wait_usec(200);
  RET_IF_ERR(gpio_set_on, nokia5110_dev->rst);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_y_addr, int y)
  int err;
  if (y > NOKIA5110_ROW_MAX)
    return ERR_INVAL_ARG;
  SEND_CMD(0b01000000 | y);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_set_cursor, int x, int y)
  int err;
  if (y > NOKIA5110_ROW_MAX)
    return ERR_INVAL_ARG;
  if (x > NOKIA5110_COLUMN_MAX)
    return ERR_INVAL_ARG;

  SEND_CMD(0b01000000 | y);
  SEND_CMD(0b10000000 | x);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_draw_dot, int x, int y)
  int err;
  char dot_pixel;
  RET_IF_ERR(nokia5110_set_cursor, x, y / 8);
  dot_pixel = 1 << (y % 8);
  SEND_DATA(&dot_pixel, 1);
  return ERR_OK;
}

int nokia5110_blank_screen(void)
{
  nokia5110_set_cursor(0, 0);
  memset(nokia5110_canvas, 0, NOKIA5110_CANVAS_SIZE);
  nokia5110_send_data(nokia5110_canvas, NOKIA5110_CANVAS_SIZE);
  return ERR_OK;
}

CHECKED_FUNC(nokia5110_draw_line, int x0, int y0, int x1, int y1)
  int x, err;
  for (x = 0; x < 84; x++) {
    RET_IF_ERR(nokia5110_draw_dot, x0 + x, y0 + x);
  }

  return ERR_OK;
}

CHECKED_FUNC(nokia5110_draw_rect, int x, int y, int sx, int sy)
  return ERR_OK;
}

void nokia5110_print_info()
{
  puts("NOKIA 5110 LCD Display\n");
}

#define draw_char_debug() \
  printf("draw_char: %c: i:%d, x:%d, y:%d, first_pixel_idx:%d\n", c, glyph_idx, gm->pos_x, gm->pos_y, glyph_offset);\
  printf("---------: bearing: %d:%d, bbox: %d:%d, adv: %d:%d\n",\
      gm->bearing_x, gm->bearing_y,\
      gm->bound_x, gm->bound_y,\
      gm->advance_x, gm->advance_y);\
  printf("bitmap: %08llx, glyph: %08llx, off: %08x\n",\
      (long long)ctl->font->bitmap, (long long)glyph, (char *)glyph - (char *)ctl->font->bitmap);\
  printf("ctl: %08llx, orig:%08llx\n", ctl->canvas, nokia5110_canvas)


static void nokia5110_canvas_set_font(nokia5110_canvas_control_t *ctl, const font_desc_t *f)
{
  ctl->font = f;
}

static inline void __nokia5110_canvas_set_cursor(nokia5110_canvas_control_t *ctl, int x, int y)
{
  if (x == -1 && y == -1)
    return;

  ctl->cursor_x = x;
  if (ctl->cursor_x >= NOKIA5110_WIDTH)
    ctl->cursor_x = NOKIA5110_WIDTH - 1;
  ctl->cursor_y = y;

  /* Wrap to start */
  if (ctl->cursor_y >= NOKIA5110_HEIGHT)
    ctl->cursor_y = 0;
}

static inline void nokia5110_canvas_set_newline(nokia5110_canvas_control_t *ctl)
{
  __nokia5110_canvas_set_cursor(ctl, ctl->cursor_x, ctl->cursor_y + (1<<NOKIA5110_PIXELS_PER_BYTE_LOG));
}

static inline void nokia5110_canvas_set_carr_return(nokia5110_canvas_control_t *ctl)
{
  ctl->cursor_x = 0;
}

static inline void nokia5110_canvas_cursor_set(nokia5110_canvas_control_t *ctl, uint8_t value)
{
  NOKIA5110_CANVAS_SET(ctl, value);
  if (ctl->cursor_x++ == NOKIA5110_WIDTH) {
    ctl->cursor_x = 0;
    nokia5110_canvas_set_newline(ctl);
  }
}

static inline void nokia5110_canvas_blank_char(nokia5110_canvas_control_t *ctl, const font_glyph_metrics_t *gm)
{
  int i;
  int glyph_width = glyph_metrics_get_cursor_step_x(gm);

  for (i = 0; i < glyph_width; ++i)
    NOKIA5110_CANVAS_SET_XY(ctl, ctl->cursor_x + i, ctl->cursor_y, 0);
}

int nokia5110_canvas_draw_char(nokia5110_canvas_control_t *ctl, char c)
{
  int gx, gy;
  uint8_t tmp;
  const uint8_t *glyph;
  const font_glyph_metrics_t *gm = font_get_glyph_metrics(ctl->font, c);
  int glyph_offset = glyph_metrics_get_offset(ctl->font, gm);
  int glyph_step_x = glyph_metrics_get_cursor_step_x(gm);

  if (ctl->cursor_x + glyph_step_x >= NOKIA5110_WIDTH) {
    nokia5110_canvas_set_carr_return(ctl);
    nokia5110_canvas_set_newline(ctl);
  }

  nokia5110_canvas_blank_char(ctl, gm);

  glyph = (const uint8_t *)ctl->font->bitmap + glyph_offset;

  // draw_char_debug();
  /* Skip space of width 'bearing_x' before actual symbol */
  for (gx = 0; gx < gm->bearing_x; ++gx)
    nokia5110_canvas_cursor_set(ctl, 0);

  for (gx = 0; gx < gm->bound_x; ++gx) {
    tmp = 0;
    for (gy = 0; gy < gm->bound_y; ++gy) {
      char line = glyph[gy * ctl->font->glyph_stride];
      char is_set = (line & (1 << gx)) ? 1 : 0;
      int v = gy + 8 - gm->bound_y - gm->bearing_y;
      tmp |= is_set << v;
    }
    nokia5110_canvas_cursor_set(ctl, tmp);
  }

  /* Advance cursor now */
  return ERR_OK;
}

static int nokia5110_canvas_redraw_locked()
{
  int err;
  err = nokia5110_set_cursor(0, 0);
  if (err) {
    blink_led(6, 200);
  }
  if (!err)
    err = nokia5110_send_data(nokia5110_canvas, NOKIA5110_CANVAS_SIZE);
  return err;
}

static nokia5110_canvas_control_t nokia5110_canvas_control = {
  .canvas = nokia5110_canvas,
  .cursor_x = 0,
  .cursor_y = 0,
  .font = 0
};

int nokia5110_draw_text(const char *text, int x, int y)
{
  int err;
  int irqflags;
  nokia5110_canvas_control_t *ctl;

  ctl = &nokia5110_canvas_control;

  cond_spinlock_lock_disable_irq(&nokia5110_lock, irqflags);

  if (!nokia5110_font) {
    err = ERR_NOT_INIT;
    goto out;
  }

  while(*text)
    nokia5110_canvas_draw_char(ctl, *text++);

  err = nokia5110_canvas_redraw_locked();
out:
  cond_spinlock_unlock_restore_irq(&nokia5110_lock, irqflags);
  return err;
}

int nokia5110_draw_char(char c)
{
  int err;
  err = nokia5110_canvas_draw_char(&nokia5110_canvas_control, c);
  if (!err)
    err = nokia5110_canvas_redraw_locked();
  if (err)
    blink_led(12, 500);
  return err;
}

int nokia5110_canvas_set_cursor(int x, int y)
{
  __nokia5110_canvas_set_cursor(&nokia5110_canvas_control, x, y);
  return ERR_OK;
}

int nokia5110_canvas_get_cursor(int *x, int *y)
{
  *x = nokia5110_canvas_control.cursor_x;
  *y = nokia5110_canvas_control.cursor_y;
  return ERR_OK;
}

void nokia5110_set_font(const font_desc_t *f)
{
  int irqflags;
  cond_spinlock_lock_disable_irq(&nokia5110_lock, irqflags);

  nokia5110_font = f;

  cond_spinlock_unlock_restore_irq(&nokia5110_lock, irqflags);

  nokia5110_canvas_set_font(&nokia5110_canvas_control, f);
}

