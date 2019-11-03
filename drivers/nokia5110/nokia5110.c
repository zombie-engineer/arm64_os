#include <drivers/display/nokia5110.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>


#define DISPLAY_MODE_MAX 3

typedef struct nokia5110_dev {
  spi_dev_t *spi;
  uint32_t dc;
  uint32_t rst;
} nokia5110_dev_t;


static nokia5110_dev_t nokia5110_device;


static nokia5110_dev_t *nokia5110_dev = 0;

#define CHECKED_FUNC(fn, ...) DECL_FUNC_CHECK_INIT(fn, nokia5110_dev, __VA_ARGS__)

// sets DC pin to DATA, tells display that this byte should be written to display RAM
#define SET_DATA() RET_IF_ERR(gpio_set_on , nokia5110_dev->dc)
// sets DC pint to CMD, tells display that this byte is a command
#define SET_CMD()  RET_IF_ERR(gpio_set_off, nokia5110_dev->dc)

#define SPI_SEND(data, len)  RET_IF_ERR(nokia5110_dev->spi->xmit, data, len)

#define SPI_SEND_BYTE(data)  RET_IF_ERR(nokia5110_dev->spi->xmit_byte, data)

#define SEND_CMD(cmd)        SET_CMD(); SPI_SEND_BYTE((cmd))

#define SEND_DATA(data, len) SET_DATA(); SPI_SEND(&data, len)


#define NOKIA5110_INSTRUCTION_SET_NORMAL 0
#define NOKIA5110_INSTRUCTION_SET_EX     1
static CHECKED_FUNC(nokia5110_set_instructions, int instruction_set)
  int err;
  if (instruction_set == NOKIA5110_INSTRUCTION_SET_NORMAL) {
    SEND_CMD(0xc0);
  }
  else {
    SEND_CMD(0x21);
  }
  return ERR_OK;
}

int nokia5110_init(spi_dev_t *spidev, uint32_t rst_pin, uint32_t dc_pin, int function_flags, int display_mode)
{ 
  int err;
  if (!spidev)
    return ERR_INVAL_ARG;

  RET_IF_ERR(gpio_set_function, dc_pin, GPIO_FUNC_OUT);
  RET_IF_ERR(gpio_set_function, rst_pin, GPIO_FUNC_OUT);

  nokia5110_device.spi = spidev;
  nokia5110_device.dc = dc_pin;
  nokia5110_device.rst = rst_pin;
  nokia5110_dev = &nokia5110_device;

  RET_IF_ERR(gpio_set_on, nokia5110_device.rst);
  RET_IF_ERR(gpio_set_off, nokia5110_device.dc);
  RET_IF_ERR(nokia5110_reset);


  SEND_CMD(0x21);
  return ERR_OK;
  SEND_CMD(0xb9);
  SEND_CMD(0x04);
  SEND_CMD(0x14);
  SEND_CMD(0x20);
  SEND_CMD(0x0c);
  // RET_IF_ERR(nokia5110_set_function, function_flags);
  // RET_IF_ERR(nokia5110_set_display_mode, display_mode);
  
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
  wait_msec(1000);
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
  SEND_DATA(dot_pixel, 1);
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
