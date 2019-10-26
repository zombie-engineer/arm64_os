#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <common.h>
#include <string.h>
#include <led_display/f5161ah.h>
#include <delays.h>

static int command_f5161ah_print_help()
{
  puts("f5161ah init shiftreg CLK_WAIT SER SRCLK RCLK\n"
       "     - init driver for 7 segment display f5161ah, driven by shiftreg\n"
       "     - which will be initialized with pins SER, SRCLK and RLCK\n"
       );
  puts("f5161ah show CHAR\n"
       "     - displays printable CHAR on 7 segment led display.\n");

  puts("f5161ah countdown REPS WAIT\n"
       "     - display countdown on 7 segment led display number of REPS.\n");
  return CMD_ERR_NO_ERROR;
}


static int command_f5161ah_init_with_shiftreg(const string_tokens_t *args)
{
  int clk_wait, ser, srclk, rclk, st;
  shiftreg_t *sr;
  char *endptr;

  ASSERT_NUMARGS_EQ(4);
  GET_NUMERIC_PARAM(clk_wait, int, 0, "shiftreg CLK_WAIT gpio pin");
  GET_NUMERIC_PARAM(ser     , int, 1, "shiftreg SER gpio pin");
  GET_NUMERIC_PARAM(srclk   , int, 2, "shiftreg SRCLK gpio pin");
  GET_NUMERIC_PARAM(rclk    , int, 3, "shiftreg RCLK gpio pin");

  sr = shiftreg_init(clk_wait, ser, srclk, rclk, SHIFTREG_INIT_PIN_DISABLED, SHIFTREG_INIT_PIN_DISABLED);
  if (!sr) {
    printf("shiftreg_init failed\n");
    return CMD_ERR_EXECUTION_ERR;
  }

  if ((st = f5161ah_init(sr))) {
    printf("f5161ah_init failed with error: %d\n", st);
    return CMD_ERR_EXECUTION_ERR;
  }

  return CMD_ERR_NO_ERROR;
}


static int command_f5161ah_init(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "shiftreg"))
    return command_f5161ah_init_with_shiftreg(&subargs);

  return CMD_ERR_INVALID_ARGS;
}


static int command_f5161ah_show(const string_tokens_t *args)
{
  uint8_t value, dot, args_valid;
  int st;
  ASSERT_NUMARGS_GE(1);

  args_valid = 0;

  if (args->ts[0].len == 1) {
    if (args->ts[0].s[0] >= '0' && args->ts[0].s[0] <= '9') {
      value = args->ts[0].s[0] - '0';

      if (args->len > 1) {
        if (args->ts[1].len == 1 && args->ts[1].s[0] == '.')
          dot = 1;
        else
          dot = 0;
      }
      args_valid = 1;
    }
    else if (args->ts[0].s[0] == '.') {
      value = CHAR_NULL;
      dot = 1;
      args_valid = 1;
    }
  }
  if (args_valid) {
    if ((st = f5161ah_display_char(value, dot))) {
      printf("f5161ah_display_char failed with error: %d\n", st);
      return CMD_ERR_EXECUTION_ERR;
    }
    return CMD_ERR_NO_ERROR;
  }
  return CMD_ERR_INVALID_ARGS;
}

static int command_f5161ah_countdown(const string_tokens_t *args) {
  int st, i, j, reps, msec;
  char *endptr;

  ASSERT_NUMARGS_EQ(2);
  GET_NUMERIC_PARAM(reps, int, 0, "REPS");
  GET_NUMERIC_PARAM(msec, int, 1, "WAIT");
  for (i = 0; i < reps; ++i) {
    for (j = 0; j < 10; ++j) {
      if ((st = f5161ah_display_char(9 - j, 0))) {
        printf("f5161ah_display_char failed with error: %d\n", st);
        return CMD_ERR_EXECUTION_ERR;
      }
      if (msec)
        msec -= 500;

      wait_msec(msec);
    }
  }
  
  return CMD_ERR_NO_ERROR;
}

int command_f5161ah(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_f5161ah_print_help();
  if (string_token_eq(subcmd_token, "init"))
    return command_f5161ah_init(&subargs);
  if (string_token_eq(subcmd_token, "show"))
    return command_f5161ah_show(&subargs);
  if (string_token_eq(subcmd_token, "countdown"))
    return command_f5161ah_countdown(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(f5161ah, "access to f5161ah driver");
