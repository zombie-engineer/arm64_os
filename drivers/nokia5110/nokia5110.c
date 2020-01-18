#include <drivers/display/nokia5110.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>
#include <stringlib.h>

#define NOKIA_5110_FUNCTION_SET(PD, V, H) (0b00100000 | (PD << 2) | (V << 1) | (H << 0))
#define PD_BIT_SET 1
#define PD_BIT_CLEAR 0

#define V_BIT_SET 1
#define V_BIT_CLEAR 0

#define H_BIT_SET 1
#define H_BIT_CLEAR 0

#define NOKIA_5110_DISPLAY_CONTROL(D, E) (0b00001000 | (D << 2) | (E << 0))

#define NOKIA_5110_DISPLAY_CONTROL_BLANK      NOKIA_5110_DISPLAY_CONTROL(0, 0)
#define NOKIA_5110_DISPLAY_CONTROL_NORMAL     NOKIA_5110_DISPLAY_CONTROL(1, 0)
#define NOKIA_5110_DISPLAY_CONTROL_ALL_SEG_ON NOKIA_5110_DISPLAY_CONTROL(0, 1)
#define NOKIA_5110_DISPLAY_CONTROL_INVERTED   NOKIA_5110_DISPLAY_CONTROL(1, 1)

// Temperature control 0 - 3 0 is more contrast, 3 - is less contrast
#define NOKIA_5110_TEMP_CONTROL(value) (0b00000100 | (value & 3))
#define NOKIA_5110_BIAS_SYSTEM(value)  (0b00010000 | (value & 7))
#define NOKIA_5110_SET_VOP(value)      (0b10000000 | (value & 0x7f))

#define DISPLAY_MODE_MAX 3

extern const char _binary_to_raspi_nokia5110_start;
extern const char _binary_to_raspi_nokia5110_end;

typedef struct nokia5110_dev {
  spi_dev_t *spi;
  uint32_t dc;
  uint32_t rst;
} nokia5110_dev_t;


static nokia5110_dev_t nokia5110_device;


static nokia5110_dev_t *nokia5110_dev = 0;

static char frame_1[504 + 4];
static char frame_2[504 + 4];
static char frame_3[504 + 4];
static char frame_4[504 + 4];
static char frame_5[504 + 4];
static char frame_6[504 + 4];
static char frame_7[504 + 4];
static char frame_8[504 + 4];
#define CHECKED_FUNC(fn, ...) DECL_FUNC_CHECK_INIT(fn, nokia5110_dev, __VA_ARGS__)

// sets DC pin to DATA, tells display that this byte should be written to display RAM
#define SET_DATA() RET_IF_ERR(gpio_set_on , nokia5110_dev->dc)
// sets DC pint to CMD, tells display that this byte is a command
#define SET_CMD()  RET_IF_ERR(gpio_set_off, nokia5110_dev->dc)

#define SPI_SEND(data, len)  RET_IF_ERR(nokia5110_dev->spi->xmit, data, len)

#define SPI_SEND_DMA(data, len)  RET_IF_ERR(nokia5110_dev->spi->xmit_dma, data, 0, len)

#define SPI_SEND_BYTE(data)  RET_IF_ERR(nokia5110_dev->spi->xmit_byte, data)

#define SEND_CMD(cmd)        SET_CMD(); SPI_SEND_BYTE((cmd));

#define SEND_DATA(data, len) SET_DATA(); SPI_SEND(data, len)

#define SEND_DATA_DMA(data, len) SET_DATA(); SPI_SEND_DMA(data, len)


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

int nokia5110_run_test_loop_1(int iterations, int wait_interval)
{
  int i, err;
  char *ptr;
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  memset(frame_1, 0xff, 4);
  memset(frame_2, 0xff, 4);
  memset(frame_3, 0xff, 4);
  memset(frame_4, 0xff, 4);

  ptr = frame_1 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b10000000;
    *ptr++ = 0b01000000;
    *ptr++ = 0b00100000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00001000;
    *ptr++ = 0b00000100;
    *ptr++ = 0b00000010;
    *ptr++ = 0b00000001;
  }

  ptr = frame_2 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
  }

  ptr = frame_3 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00000001;
    *ptr++ = 0b00000010;
    *ptr++ = 0b00000100;
    *ptr++ = 0b00001000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00100000;
    *ptr++ = 0b01000000;
    *ptr++ = 0b10000000;
  }

  ptr = frame_4 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b11111111;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
  }

  for (i = 0; i < iterations; ++i) {
    SEND_DATA_DMA(frame_1, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_2, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_3, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_4, 504);
    wait_msec(wait_interval);
  }

  return 0;
}

int nokia5110_run_test_loop_2(int iterations, int wait_interval)
{
  int i;
  int err;
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  memset(frame_1, 0b10000000, 508);
  memset(frame_2, 0b01000000, 508);
  memset(frame_3, 0b00100000, 508);
  memset(frame_4, 0b00010000, 508);
  memset(frame_5, 0b00001000, 508);
  memset(frame_6, 0b00000100, 508);
  memset(frame_7, 0b00000010, 508);
  memset(frame_8, 0b00000001, 508);

  for (i = 0; i < iterations; ++i) {
    SEND_DATA_DMA(frame_1, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_2, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_3, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_4, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_5, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_6, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_7, 504);
    wait_msec(wait_interval);
    SEND_DATA_DMA(frame_8, 504);
    wait_msec(wait_interval);
  }

  return ERR_OK;
}

int nokia5110_run_test_loop_3(int iterations, int wait_interval)
{
  int err, i, numframes;
  const char *video_0;
  uint64_t bufsize;
  wait_msec(5000);

  video_0 = &_binary_to_raspi_nokia5110_start;
  bufsize = (uint64_t)(&_binary_to_raspi_nokia5110_end) - (uint64_t)&_binary_to_raspi_nokia5110_start;
  numframes = bufsize / 508;
  printf("showing video_0 from address: %08x, num frames: %d\n", &_binary_to_raspi_nokia5110_start, numframes);
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  for (i = 0; i < numframes; ++i) {
    RET_IF_ERR(nokia5110_set_cursor, 0, 0);
    SEND_DATA_DMA(video_0 + (504 + 4) * i, 504);
    wait_msec(20);
  }
  return ERR_OK;
}

int nokia5110_init(spi_dev_t *spidev, uint32_t rst_pin, uint32_t dc_pin, int function_flags, int display_mode)
{ 
  int err;
  if (!spidev)
    return ERR_INVAL_ARG;

  RET_IF_ERR(gpio_set_function, rst_pin, GPIO_FUNC_OUT);
  RET_IF_ERR(gpio_set_function, dc_pin, GPIO_FUNC_OUT);

  nokia5110_device.spi = spidev;
  nokia5110_device.dc = dc_pin;
  nokia5110_device.rst = rst_pin;
  nokia5110_dev = &nokia5110_device;

  RET_IF_ERR(gpio_set_on, nokia5110_device.rst);
  RET_IF_ERR(gpio_set_off, nokia5110_device.dc);
  RET_IF_ERR(nokia5110_reset);

  SEND_CMD(NOKIA_5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_SET));
  SEND_CMD(NOKIA_5110_SET_VOP(0x39));
  SEND_CMD(NOKIA_5110_TEMP_CONTROL(0));
  SEND_CMD(NOKIA_5110_BIAS_SYSTEM(4));
  SEND_CMD(NOKIA_5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_CLEAR));
  SEND_CMD(NOKIA_5110_DISPLAY_CONTROL_NORMAL);

  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  puts("nokia5110 init is complete\n");
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
  puts("R\r\n");
  wait_usec(1000);
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
