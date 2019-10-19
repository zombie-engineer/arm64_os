#include <cmdrunner.h>
#include "argument.h"
#include <stdlib.h>
#include <pwm.h>
#include <common.h>
#include <string.h>

static int command_pwm_print_help()
{
  puts("pwm COMMAND VALUES\n");
  puts("\tCOMMANDS:\n");
  puts("pwm enable CHANNEL MS_MODE - enables PWM with initial range/data\n");
  puts("\t\t                       - MS_MODE - 0 for PWM, 1 for M/S mode\n");
  puts("pwm set CHANNEL RANGE DATA    - sets PWM range/data\n");
  puts("\t\t                  CHANNEL - PWM0 or PWM1\n");
  puts("\t\t                  RANGE   - number of cycles in a single PWM loop\n");
  puts("\t\t                  DATA    - binary sequence, sent within the range in a loop\n");
  puts("\t\tinfo   - prints info about PWM registers\n");
  return CMD_ERR_NO_ERROR;
}

static int command_pwm_info(const string_tokens_t *args)
{
 //  DECL_ASSIGN_PTR(volatile pwm_t, p, PWM);
 //  puts("pwm info:\n");
 //  printf("CTL : %08x\n", p->CTL);
 //  printf("STA : %08x\n", p->STA);
 //  printf("DMAC: %08x\n", p->DMAC);
 //  printf("UND1: %08x\n", p->UND1);
 //  printf("RNG1: %08x\n", p->RNG1);
 //  printf("DAT1: %08x\n", p->DAT1);
 //  printf("FIF1: %08x\n", p->FIF1);
 //  printf("UND2: %08x\n", p->UND2);
 //  printf("RNG2: %08x\n", p->RNG2);
 //  printf("DAT2: %08x\n", p->DAT2);
  return CMD_ERR_NO_ERROR;
}

static int command_pwm_enable(const string_tokens_t *args)
{
  int channel, ms_mode, st;
  char *endptr;
  ASSERT_NUMARGS_EQ(2);
  GET_NUMERIC_PARAM(channel, int, 0, "PWM channel");
  GET_NUMERIC_PARAM(ms_mode, int, 1, "MS/PWM mode");

  st = pwm_enable(channel, ms_mode);
  if (st) {
    printf("command_pwd_enable error: pwm_enable completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}

static int command_pwm_set(const string_tokens_t *args)
{
  int channel, range, data, st;
  char *endptr;
  ASSERT_NUMARGS_EQ(3);
  GET_NUMERIC_PARAM(channel, int, 0, "PWM channel");
  GET_NUMERIC_PARAM(range  , int, 1, "PWM range");
  GET_NUMERIC_PARAM(data   , int, 2, "PWM data");

  st = pwm_set(channel, range, data);
  if (st) {
    printf("command_pwd_set error: pwm_enable completed with error %d\n");
    return CMD_ERR_EXECUTION_ERR;
  }
  return CMD_ERR_NO_ERROR;
}

int command_pwm(const string_tokens_t *args)
{
  string_tokens_t subargs;
  string_token_t *subcmd_token;
  puts("pwm: \n");

  ASSERT_NUMARGS_GE(2);

  subcmd_token = &args->ts[1];
  subargs.ts  = subcmd_token + 1;
  subargs.len = args->len - 2;

  if (string_token_eq(subcmd_token, "help"))
    return command_pwm_print_help();
  if (string_token_eq(subcmd_token, "enable"))
    return command_pwm_enable(&subargs);
  if (string_token_eq(subcmd_token, "set"))
    return command_pwm_set(&subargs);
  if (string_token_eq(subcmd_token, "info"))
    return command_pwm_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(pwm, "access to PWM controller");
