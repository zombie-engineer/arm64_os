#include <cmdrunner.h>
#include <spi.h>
#include "argument.h"
#include <stdlib.h>
#include <common.h>
#include <string.h>
#include <max7219.h>
#include <delays.h>


static int command_max7219_print_help()
{
  puts("max7219 init SPI_TYPE - max7219 driver should be initialized by this function first\n");
  puts("                      - SPI_TYPE - type of spi bus to use:\n");
  puts("                        - spi0\n");
  puts("                        - spi1\n");
  puts("                        - spi-emulated\n");
  puts("max7219 set VALUE     - raw 16bit value that will be shifted to the chip's ADDRESS and DATA registers\n");
  puts("max7219 lineloop LINE INTERVAL REPS - run loop on line LINE with specified interval and number of REPS\n");
  return CMD_ERR_NO_ERROR;
}

static int command_max7219_info()
{
  int st;
  st = max7219_print_info();
  if (st)
    return CMD_ERR_EXECUTION_ERR;
  return CMD_ERR_NO_ERROR;
}

static int command_max7219_init(const string_tokens_t *args)
{
  int i;
  int err;
  spi_dev_t *spidev;
  ASSERT_NUMARGS_EQ(1);

  spidev = spi_get_dev(spi_type_from_string(args->ts[0].s, args->ts[0].len));
  if (!spidev) {
    printf("unknown spi interface: %s\n", args->ts[0].s);
    return CMD_ERR_INVALID_ARGS;
  }

  err = max7219_set_spi_dev(spidev);
  if (err) {
    printf("max7219_set_spi_dev failed with error: %d\n", err);
    return CMD_ERR_EXECUTION_ERR;
  }

  max7219_set_test_mode_on();
  wait_msec(100);
  max7219_set_test_mode_off();
  wait_msec(100);

  for (i = 0; i < 8; ++i)
    max7219_row_off(i);

  max7219_set_shutdown_mode_off();
  wait_msec(100);
  max7219_set_scan_limit(MAX7219_SCAN_LIMIT_FULL);
  wait_msec(100);
  max7219_row_on(0);
  wait_msec(10000);
  max7219_row_off(0);
  return CMD_ERR_NO_ERROR;
}


static int command_max7219_set(const string_tokens_t *args)
{
  uint16_t value;
  char *endptr;
  ASSERT_NUMARGS_EQ(1);
  GET_NUMERIC_PARAM(value, uint16_t, 0, "max7219 shiftreg bits");
  max7219_set_raw(value);

  return CMD_ERR_NO_ERROR;
}



static int command_max7219_lineloop(const string_tokens_t *args)
{
  int line, interval, repeats, i, b;
  char *endptr;
  ASSERT_NUMARGS_EQ(3);
  GET_NUMERIC_PARAM(line    , int, 0, "max7219 lineloop line");
  GET_NUMERIC_PARAM(interval, int, 1, "max7219 lineloop interval");
  GET_NUMERIC_PARAM(repeats , int, 2, "max7219 lineloop repeats");

  if (line > 7) {
    puts("line argument out of range. Value should be in range 0-7\n");
    return CMD_ERR_INVALID_ARGS;
  }

  for (i = 0; i < repeats; ++i) {
    for (b = 0; b < 8; ++b) {
      max7219_row_on(b);
      wait_msec(interval);
      max7219_row_off(b);
      wait_msec(interval);
    }
  }

  return CMD_ERR_NO_ERROR;
}

int command_max7219(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_max7219_print_help();
  if (string_token_eq(subcmd_token, "info"))
    return command_max7219_info();
  if (string_token_eq(subcmd_token, "init"))
    return command_max7219_init(&subargs);
  if (string_token_eq(subcmd_token, "set"))
    return command_max7219_set(&subargs);
  if (string_token_eq(subcmd_token, "lineloop"))
    return command_max7219_lineloop(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(max7219, "access to max7219 driver");
