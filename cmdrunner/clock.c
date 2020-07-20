#include "argument.h"
#include <cmdrunner.h>
#include <clock_manager.h>
#include <common.h>
#include <stringlib.h>

static int command_clock_print_help()
{
  puts("clock COMMAND VALUES\n");
  puts("\tCOMMANDS:\n");
  puts("\t\tset CLOCK_ID CLOCK_SRC DIV - enables clock\n");
  puts("\t\t                 CLOCK_ID  - gp0, gp1, gp2, pwm\n");
  puts("\t\t                 CLOCK_SRC - gnd, osc, tstdbg0, tstdbg1, plla, pllc, plld, hdmi\n");
  puts("\t\t                 DIV       - divisor\n");
  puts("\t\tinfo [CLOCK_ID]            - prints info about PWM registers\n");
  return CMD_ERR_NO_ERROR;
}

static int command_clock_info(const string_tokens_t *args)
{
  puts("clock info:\n");
  ASSERT_NUMARGS_EQ(1);

  return CMD_ERR_NO_ERROR;
}

static int command_clock_set(const string_tokens_t *args)
{
  int clock_id, st;
  uint32_t clock_src, mash, divi, divf;
  char *endptr;

  ASSERT_NUMARGS_GE(3);

  if      (string_token_eq(&args->ts[0], "gp0"))
    clock_id = CM_CLK_ID_GP_0;
  else if (string_token_eq(&args->ts[0], "gp1"))
    clock_id = CM_CLK_ID_GP_1;
  else if (string_token_eq(&args->ts[0], "gp2"))
    clock_id = CM_CLK_ID_GP_2;
  else if (string_token_eq(&args->ts[0], "pwm"))
    clock_id = CM_CLK_ID_PWM;
  else {
    puts("error: invalid clock id argument\n");
    return CMD_ERR_INVALID_ARGS;
  }

  if      (string_token_eq(&args->ts[1], "gnd"))
    clock_src = CM_SETCLK_SRC_GND;
  else if (string_token_eq(&args->ts[1], "osc"))
    clock_src = CM_SETCLK_SRC_OSC;
  else if (string_token_eq(&args->ts[1], "tstdbg0"))
    clock_src = CM_SETCLK_SRC_TSTDBG0;
  else if (string_token_eq(&args->ts[1], "tstdbg1"))
    clock_src = CM_SETCLK_SRC_TSTDBG1;
  else if (string_token_eq(&args->ts[1], "plla"))
    clock_src = CM_SETCLK_SRC_PLLA;
  else if (string_token_eq(&args->ts[1], "pllc"))
    clock_src = CM_SETCLK_SRC_PLLC;
  else if (string_token_eq(&args->ts[1], "plld"))
    clock_src = CM_SETCLK_SRC_PLLD;
  else if (string_token_eq(&args->ts[1], "hdmi"))
    clock_src = CM_SETCLK_SRC_HDMI;
  else {
    puts("error: invalid clock source argument\n");
    return CMD_ERR_INVALID_ARGS;
  }

  GET_NUMERIC_PARAM(divi, uint32_t, 2, "divi");

  divf = 0;
  if (args->len >= 4) {
    GET_NUMERIC_PARAM(divf, uint32_t, 3, "divf");
  }
  mash = 0;

  printf("setting clock id = %d, source = %d, divi = %d, divf = %d\n", clock_id, clock_src, divi, divf);
  if ((st = cm_set_clock(clock_id, clock_src, mash, divi, divf))) {
    if (st == CM_SETCLK_ERR_INV)
      return CMD_ERR_INVALID_ARGS;
    if (st == CM_SETCLK_ERR_BUSY) {
      puts("error: clock busy\n");
      return CMD_ERR_EXECUTION_ERR;
    }
  }
  return CMD_ERR_NO_ERROR;
}

int command_clock(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_clock_print_help();
  if (string_token_eq(subcmd_token, "set"))
    return command_clock_set(&subargs);
  if (string_token_eq(subcmd_token, "info"))
    return command_clock_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(clock, "general purpose and pwm clock controller control");
