#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <common.h>
#include <string.h>
#include <shiftreg.h>


static shiftreg_t *sr = 0;

static int command_shiftreg_print_help()
{
  puts("shiftreg init CLCK_WAIT SER_PIN SRCLK_PIN RCLK_PIN\n");
  puts("              CLCK_WAIT - clock pulse interval\n");
  puts("shiftreg pushbit BIT    - push BIT 0 or 1\n");
  puts("shiftreg pushbyte BYTE  - push BYTE value MSR first\n");
  puts("shiftreg latch          - latch values from shift registers to output registers\n");
  return CMD_ERR_NO_ERROR;
}


static int command_shiftreg_pushbit(const string_tokens_t *args)
{
  int st;
  char bit_value;
  char *endptr;
  ASSERT_NUMARGS_EQ(1);
  GET_NUMERIC_PARAM(bit_value, char, 0, "bit value");
  st = shiftreg_push_bit(sr, bit_value);
  if (st) {
    printf("shiftreg_push_bit failed with error %d\n", st);
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_shiftreg_pushbyte(const string_tokens_t *args)
{
  int st;
  char byte_value;
  char *endptr;
  ASSERT_NUMARGS_EQ(1);
  GET_NUMERIC_PARAM(byte_value, char, 0, "byte value");
  st = shiftreg_push_byte(sr, byte_value);
  if (st) {
    printf("shiftreg_push_byte failed with error %d\n", st);
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_shiftreg_latch(const string_tokens_t *args)
{
  int st;
  ASSERT_NUMARGS_EQ(0);
  st = shiftreg_pulse_rclk(sr);
  if (st) {
    printf("shiftreg_pulse_rclk failed with error %d\n", st);
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


static int command_shiftreg_init(const string_tokens_t *args)
{
  int clk_wait, ser, srclk, rclk;
  char *endptr;
  ASSERT_NUMARGS_EQ(4);
  GET_NUMERIC_PARAM(clk_wait, int, 0, "clk_wait");
  GET_NUMERIC_PARAM(ser     , int, 1, "ser");
  GET_NUMERIC_PARAM(srclk   , int, 2, "srclk");
  GET_NUMERIC_PARAM(rclk    , int, 3, "rclk");
  sr = shiftreg_init(clk_wait, ser, srclk, rclk, SHIFTREG_INIT_PIN_DISABLED, SHIFTREG_INIT_PIN_DISABLED);
  if (!sr) {
    printf("shiftreg_init failed\n");
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}


int command_shiftreg(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_shiftreg_print_help();
  if (string_token_eq(subcmd_token, "init"))
    return command_shiftreg_init(&subargs);
  if (string_token_eq(subcmd_token, "pushbit"))
    return command_shiftreg_pushbit(&subargs);
  if (string_token_eq(subcmd_token, "pushbyte"))
    return command_shiftreg_pushbyte(&subargs);
  if (string_token_eq(subcmd_token, "latch"))
    return command_shiftreg_latch(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(shiftreg, "access to shift register via GPIO pins");
