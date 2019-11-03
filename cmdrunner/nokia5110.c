#include <cmdrunner.h>
#include <spi.h>
#include "argument.h"
#include <stdlib.h>
#include <common.h>
#include <string.h>
#include <drivers/display/nokia5110.h>
#include <delays.h>


static int command_nokia5110_print_help()
{
  puts("nokia5110 init SPI_TYPE RST_PIN DC_PIN FUNC_FLAGS DISPLAY_MODE - init display\n");
  puts("               SPI_TYPE     - spi0, spi1, spi2, emulated\n");
  puts("               RST_PIN      - GPIO pin number for RST\n");
  puts("               DC_PIN       - GPIO pin number for D/C (data/command)\n");
  puts("               FUNC_FLAGS   - vertical/horiz addressing, instruction set\n");
  puts("               DISPLAY_MODE - 0-BLANK, 1-NORMAL, 2-ALL_SEG_ON, 3-INVERSE \n");
  puts("nokia5110 dot X Y           - draw dot\n");
  puts("nokia5110 line X0 Y0 X1 Y1  - draw line\n");
  return CMD_ERR_NO_ERROR;
}

static int command_nokia5110_info()
{
  nokia5110_print_info();
  return CMD_ERR_NO_ERROR;
}

static int command_nokia5110_init(const string_tokens_t *args)
{
  char *endptr;
  int err;
  spi_dev_t *spidev;

  uint32_t rst_pin, dc_pin; 
  int function_flags, display_mode;
  ASSERT_NUMARGS_EQ(5);
  GET_NUMERIC_PARAM(rst_pin       , uint32_t, 1, "rst pin");
  GET_NUMERIC_PARAM(dc_pin        , uint32_t, 2, "dc pin");
  GET_NUMERIC_PARAM(function_flags, int     , 3, "function flags");
  GET_NUMERIC_PARAM(display_mode  , int     , 4, "display mode");

  spidev = spi_get_dev(spi_type_from_string(args->ts[0].s, args->ts[0].len));
  if (!spidev) {
    printf("unknown spi interface: %s\n", args->ts[0].s);
    return CMD_ERR_INVALID_ARGS;
  }

  err = nokia5110_init(spidev, rst_pin, dc_pin, function_flags, display_mode);
  if (err) {
    printf("nokia5110_init failed with err: %d\n", err);
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_nokia5110_dot(const string_tokens_t *args)
{
  int x, y, err;
  char *endptr;
  ASSERT_NUMARGS_EQ(2);
  GET_NUMERIC_PARAM(x, int, 0, "x coord");
  GET_NUMERIC_PARAM(y, int, 1, "y coord");
  err = nokia5110_draw_dot(x, y);
  if (err) {
    printf("nokia5110_draw_dot failed with err: %d\n", err);
    return CMD_ERR_EXECUTION_ERR;
  }

  return CMD_ERR_NO_ERROR;
}


static int command_nokia5110_line(const string_tokens_t *args)
{
  int err, x0, y0, x1, y1;
  char *endptr;
  ASSERT_NUMARGS_EQ(4);
  GET_NUMERIC_PARAM(x0, int, 0, "x0 coord");
  GET_NUMERIC_PARAM(y0, int, 1, "y0 coord");
  GET_NUMERIC_PARAM(x1, int, 2, "x1 coord");
  GET_NUMERIC_PARAM(y1, int, 3, "y1 coord");
  err = nokia5110_draw_line(x0, y0, x1, y1);
  if (err) {
    printf("nokia5110_draw_dot failed with err: %d\n", err);
    return CMD_ERR_EXECUTION_ERR;
  }

  return CMD_ERR_NO_ERROR;
}


int command_nokia5110(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_nokia5110_print_help();
  if (string_token_eq(subcmd_token, "info"))
    return command_nokia5110_info();
  if (string_token_eq(subcmd_token, "init"))
    return command_nokia5110_init(&subargs);
  if (string_token_eq(subcmd_token, "dot"))
    return command_nokia5110_dot(&subargs);
  if (string_token_eq(subcmd_token, "line"))
    return command_nokia5110_line(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(nokia5110, "access to nokia5110 driver");
