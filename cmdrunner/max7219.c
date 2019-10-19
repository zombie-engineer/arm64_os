#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <common.h>
#include <string.h>

static int command_max7219_print_help()
{
  puts("max7219 set VALUE              - raw 16bit value that will be shifted to the chip's ADDRESS and DATA registers\n");
  puts("max7219 lineloop LINE INTERVAL REPS - run loop on line LINE with specified interval and number of REPS\n");
  return CMD_ERR_NO_ERROR;
}

static int command_max7219_set(const string_tokens_t *args)
{
  uint16_t value;
  char *endptr;
  ASSERT_NUMARGS_EQ(1);
  GET_NUMERIC_PARAM(value, uint16_t, 0, "max7219 shiftreg bits");
  shiftreg_push_16bits(value);
  shiftreg_pulse_latch();

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
  for (i = 0; i < repeats; ++i) {
    for (b = 0; b < 8; ++b) {
      shiftreg_push_16bits(0x100 | (1 << b));
      shiftreg_pulse_latch();
      wait_msec(interval);
    }
  }

  return CMD_ERR_NO_ERROR;
}

int command_max7219(const string_tokens_t *args)
{
  string_tokens_t subargs;
  string_token_t *subcmd_token;

  ASSERT_NUMARGS_GE(2);

  subcmd_token = &args->ts[1];
  subargs.ts  = subcmd_token + 1;
  subargs.len = args->len - 2;

  if (string_token_eq(subcmd_token, "help"))
    return command_max7219_print_help();
  if (string_token_eq(subcmd_token, "set"))
    return command_max7219_set(&subargs);
  if (string_token_eq(subcmd_token, "lineloop"))
    return command_max7219_lineloop(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(max7219, "access to max7219 driver");
