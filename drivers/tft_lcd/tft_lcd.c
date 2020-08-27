#include <drivers/display/tft_lcd.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>
#include <gpio_set.h>
#include <stringlib.h>
#include <font.h>
#include <spinlock.h>
#include <cpu.h>
#include <debug.h>

//DECL_GPIO_SET_KEY(tft_lcd_gpio_set_key, "TFT_LCD___GPIO0");

typedef struct tft_lcd_dev {
  spi_dev_t *spi;
  uint32_t dc;
  uint32_t rst;
  gpio_set_handle_t gpioset;
} tft_lcd_dev_t;

typedef struct tft_lcd_canvas_control {
  uint8_t *canvas;
  int cursor_x;
  int cursor_y;
  const font_desc_t *font;
} tft_lcd_canvas_control_t;

// static const font_desc_t *tft_lcd_font;
// static tft_lcd_dev_t tft_lcd_device;
// static tft_lcd_dev_t *tft_lcd_dev = 0;
// static struct spinlock tft_lcd_lock;

#define ILI9341_CMD_SOFT_RESET   0x01
#define ILI9341_CMD_READ_ID      0x04
#define ILI9341_CMD_SLEEP_OUT    0x11
#define ILI9341_CMD_DISPLAY_OFF  0x28
#define ILI9341_CMD_DISPLAY_ON   0x29
#define ILI9341_CMD_SET_CURSOR_X 0x2a
#define ILI9341_CMD_SET_CURSOR_Y 0x2b
#define ILI9341_CMD_WRITE_PIXELS 0x2c
#define ILI9341_CMD_POWER_CTL_A  0xcb
#define ILI9341_CMD_POWER_CTL_B  0xcf
#define ILI9341_CMD_TIMING_CTL_A 0xe8
#define ILI9341_CMD_TIMING_CTL_B 0xea
#define ILI9341_CMD_POWER_ON_SEQ 0xed
#define ILI9341_CMD_PUMP_RATIO   0xf7
#define ILI9341_CMD_POWER_CTL_1  0xc0
#define ILI9341_CMD_POWER_CTL_2  0xc1
#define ILI9341_CMD_VCOM_CTL_1   0xc5
#define ILI9341_CMD_VCOM_CTL_2   0xc7
#define ILI9341_CMD_FRAME_RATE_CTL 0xb1
#define ILI9341_CMD_BLANK_PORCH  0xb5
#define ILI9341_CMD_DISPL_FUNC   0xb6

// ILI9341 displays are able to update at any rate between 61Hz to up to 119Hz. Default at power on is 70Hz.
#define ILI9341_FRAMERATE_61_HZ 0x1F
#define ILI9341_FRAMERATE_63_HZ 0x1E
#define ILI9341_FRAMERATE_65_HZ 0x1D
#define ILI9341_FRAMERATE_68_HZ 0x1C
#define ILI9341_FRAMERATE_70_HZ 0x1B
#define ILI9341_FRAMERATE_73_HZ 0x1A
#define ILI9341_FRAMERATE_76_HZ 0x19
#define ILI9341_FRAMERATE_79_HZ 0x18
#define ILI9341_FRAMERATE_83_HZ 0x17
#define ILI9341_FRAMERATE_86_HZ 0x16
#define ILI9341_FRAMERATE_90_HZ 0x15
#define ILI9341_FRAMERATE_95_HZ 0x14
#define ILI9341_FRAMERATE_100_HZ 0x13
#define ILI9341_FRAMERATE_106_HZ 0x12
#define ILI9341_FRAMERATE_112_HZ 0x11
#define ILI9341_FRAMERATE_119_HZ 0x10
//
// Visually estimating NES Super Mario Bros 3 "match mushroom, flower, star" arcade game, 119Hz gives visually
// most pleasing result, so default to using that. You can also try other settings above. 119 Hz should give
// lowest latency, perhaps 61 Hz might give least amount of tearing, although this can be quite subjective.
#define ILI9341_UPDATE_FRAMERATE ILI9341_FRAMERATE_119_HZ

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

#define SPI_CS   ((volatile uint32_t *)0x3f204000)
#define SPI_FIFO ((volatile uint32_t *)0x3f204004)
#define SPI_CLK  ((volatile uint32_t *)0x3f204008)
#define SPI_DLEN ((volatile uint32_t *)0x3f20400c)
#define SPI_CS_CLEAR_TX (1 << 4)
#define SPI_CS_CLEAR_RX (1 << 5)
#define SPI_CS_TA       (1 << 7)
#define SPI_CS_DONE     (1 << 16)
#define SPI_CS_RXD      (1 << 17)
#define SPI_CS_TXD      (1 << 18)
#define SPI_CS_RXR      (1 << 19)
#define SPI_CS_RXF      (1 << 20)

#define SPI_DC_SET() write_reg(0x3f20001c, 1 << gpio_pin_dc)
#define SPI_DC_CLEAR() write_reg(0x3f200028, 1 << gpio_pin_dc)

#define SEND_CMD(__cmd) do { \
  if(*SPI_CS & SPI_CS_RXD) \
    *SPI_CS = SPI_CS_TA | SPI_CS_CLEAR_RX;\
  SPI_DC_CLEAR();\
  while(!(*SPI_CS & SPI_CS_TXD)) /*printf("-%08x\r\n", *SPI_CS)*/;\
  *SPI_FIFO = __cmd;\
  while(!(*SPI_CS & SPI_CS_DONE)) /*printf("+%08x\r\n", *SPI_CS)*/;\
  SPI_DC_SET();\
} while(0)

#define SEND_CMD_DATA(__cmd, __data, __datalen) do { \
  char *__ptr = (char *)(__data);\
  char *__end = __ptr + __datalen;\
  uint32_t r;\
  SEND_CMD(__cmd);\
  while(__ptr < __end) {\
    char __d = *(__ptr++);\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_RXD) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __d);\
  }\
  while(1) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_DONE)\
      break;\
    /*printf("dr:%08x\r\n", r);*/\
    if (r & SPI_CS_RXR) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
  }\
} while(0)

#define SEND_CMD_CHAR_REP(__cmd, __r, __g, __b, __reps) do { \
  uint32_t r;\
  int __rep = 0;\
  SEND_CMD(__cmd);\
  while(__rep++ < __reps) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_RXD) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __r);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __g);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __b);\
  }\
  while(1) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_DONE)\
      break;\
    /*printf("dr:%08x\r\n", r);*/\
    if (r & SPI_CS_RXR) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
  }\
} while(0)

#define RECV_CMD_DATA(__cmd, __data, __datalen) do { \
  char *__ptr = (char *)(__data);\
  char *__end = __ptr + __datalen;\
  uint32_t r;\
  SEND_CMD(__cmd);\
  write_reg(SPI_CS, SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX);\
  write_reg(SPI_CS, SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX | SPI_CS_TA);\
  while(__ptr < __end) {\
    uint8_t c;\
    r = read_reg(SPI_CS);\
    /*printf("reg:%08x\r\n", r);*/\
    if (read_reg(SPI_CS) & SPI_CS_TXD) {\
      write_reg(SPI_FIFO, 0);\
      while (read_reg(SPI_CS) & SPI_CS_DONE);\
      while (read_reg(SPI_CS) & SPI_CS_RXD) {\
        c = read_reg(SPI_FIFO);\
        /*printf("fifo:%08x\r\n", c);*/\
        *(__ptr++) = c;\
      }\
    }\
  }\
} while(0)

static inline void tft_set_region_coords(int gpio_pin_dc, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
#define LO8(__v) (__v & 0xff)
#define HI8(__v) ((__v >> 8) & 0xff)
  char data_x[4] = { HI8(x0), LO8(x0), HI8(x1), LO8(x1) };
  char data_y[4] = { HI8(y0), LO8(y0), HI8(y1), LO8(y1) };
  SEND_CMD_DATA(ILI9341_CMD_SET_CURSOR_X, data_x, sizeof(data_x));
  SEND_CMD_DATA(ILI9341_CMD_SET_CURSOR_Y, data_y, sizeof(data_y));
#undef LO8
#undef HI8
}

void OPTIMIZED clear_screen(int gpio_pin_dc)
{
  char data_rgb[3] = { 0, 0, 0xff };
  int x, y;
  for (y = 0; y < DISPLAY_HEIGHT; ++y) {
    for (x = 0; x < DISPLAY_WIDTH; ++x) {
      tft_set_region_coords(gpio_pin_dc, x, y, x + 1, y + 1);
      SEND_CMD_DATA(0x2c, data_rgb, 3);
    }
  }
}

void OPTIMIZED tft_fill_rect(int gpio_pin_dc, int x0, int y0, int x1, int y1, char r, char g, char b)
{
   char data_rgb[DISPLAY_WIDTH * 3];
   int y, x;

  for (x = 0; x < (x1 - x0); ++x) {
    data_rgb[x * 3] = r;
    data_rgb[x * 3 + 1] = g;
    data_rgb[x * 3 + 2] = b;
  }

 // tft_set_region_coords(gpio_pin_dc, x0, y0, x1, y1);
//  SEND_CMD_CHAR_REP(ILI9341_CMD_WRITE_PIXELS, r, g, b, DISPLAY_WIDTH * DISPLAY_HEIGHT * 3);
  for (y = y0; y < y1; ++y) {
    tft_set_region_coords(gpio_pin_dc, x0, y, x1, y + 1);
    SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, data_rgb, (x1 - x0) * 3);
  }
}

void OPTIMIZED clear_screen2(int gpio_pin_dc)
{
  int i;
  printf("hello\r\n");
  for (i = 0; i < 1000; ++i)
    tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, (i * 20) % 255);
}

void OPTIMIZED tft_lcd_init(void)
{
  // char data[512];
  const int gpio_pin_mosi  = 10;
  const int gpio_pin_miso  =  9;
  const int gpio_pin_sclk  = 11;
  const int gpio_pin_ce0   =  8;
  const int gpio_pin_dc    = 22;
  const int gpio_pin_reset = 25;

  gpio_set_function(gpio_pin_mosi, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_miso, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_sclk, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_ce0, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_dc, GPIO_FUNC_OUT);
  gpio_set_on(gpio_pin_dc);
  gpio_set_off(gpio_pin_ce0);
  gpio_set_off(gpio_pin_reset);

  *SPI_CS = SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX;
  *SPI_CLK = 34;
  *SPI_DLEN = 2;

  gpio_set_on(gpio_pin_reset);
  wait_msec(120);
  gpio_set_off(gpio_pin_reset);
  wait_msec(120);
  gpio_set_on(gpio_pin_reset);
  wait_msec(120);

  write_reg(SPI_CS, SPI_CS_TA);

  SEND_CMD(ILI9341_CMD_SOFT_RESET);
  wait_msec(5);
  SEND_CMD(ILI9341_CMD_DISPLAY_OFF);

  // printf("read_id: %02x:%02x:%02x\r\n", data[0], data[1], data[2]);

//  data[0] = 0x39;
//  data[1] = 0x2c;
//  data[2] = 0x34;
//  data[3] = 0x02;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_A, data, 4);
//  data[0] = 0x00;
//  data[1] = 0xc1;
//  data[2] = 0x30;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_B, data, 3);
//  data[0] = 0x85;
//  data[1] = 0x00;
//  data[2] = 0x78;
//  SEND_CMD_DATA(ILI9341_CMD_TIMING_CTL_A, data, 3);
//  data[0] = 0x00;
//  data[1] = 0x00;
//  SEND_CMD_DATA(ILI9341_CMD_TIMING_CTL_B, data, 2);
//  data[0] = 0x64;
//  data[1] = 0x03;
//  data[2] = 0x12;
//  data[3] = 0x81;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_ON_SEQ, data, 4);
  // data[0] = 0x20;
  // SEND_CMD_DATA(ILI9341_CMD_PUMP_RATIO, data, 1);
//  data[0] = 0x23;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_1, data, 1);
//  data[0] = 0x10;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_2, data, 1);

  //data[0] = 0x3e;
  // data[1] = 0x28;
  // SEND_CMD_DATA(ILI9341_CMD_VCOM_CTL_1, data, 2);

  //data[0] = 0x86;
  //SEND_CMD_DATA(ILI9341_CMD_VCOM_CTL_2, data, 1);

//  data[0] = 0x10;
//  SEND_CMD_DATA(ILI9341_CMD_FRAME_RATE_CTL, data, 1);
//
//  data[0] = 0x02;
//  data[1] = 0x02;
//  data[2] = 0x0a;
//  data[3] = 0x14;
//  SEND_CMD_DATA(ILI9341_CMD_BLANK_PORCH, data, 4);

//  data[0] = 0x08;
//  data[1] = 0x82;
//  data[2] = 0x27;
//  SEND_CMD_DATA(ILI9341_CMD_DISPL_FUNC, data, 3);

//  data[0] = 0x02;
//  SEND_CMD_DATA(0xf2, data, 1);

  SEND_CMD(ILI9341_CMD_SLEEP_OUT);
  wait_msec(120);
  SEND_CMD(ILI9341_CMD_DISPLAY_ON);
//  while(1) {
//    RECV_CMD_DATA(ILI9341_CMD_READ_ID, data, 2);
//    wait_usec(10);
//  }
  if (0)
    clear_screen2(gpio_pin_dc);
  clear_screen(gpio_pin_dc);
  {
    int x = 0, y = 0;
    int g = 0;
    int x_speed = 2;
    int y_speed = 5;
    while(1) {
      tft_fill_rect(gpio_pin_dc, x, y, x + 10, y + 10, 0, g, 0);
      wait_msec(1000 / 60);
      tft_fill_rect(gpio_pin_dc, x, y, x + 10, y + 10, 0, 0, 255);
      wait_msec(1);
      if (x_speed > 0) {
        if (x > DISPLAY_WIDTH - 10) {
          x_speed *= -1;
          g += 10;
        }
      } else {
        if (x == 0)
          x_speed *= -1;
      }
      if (y_speed > 0) {
        if (y > DISPLAY_HEIGHT - 10)
          y_speed *= -1;
      } else {
        if (y == 0)
          y_speed *= -1;
      }
      x += x_speed;
      y += y_speed;
    }
  }
  wait_usec(100);
}
