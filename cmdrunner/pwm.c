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
  puts("\t\tenable - enables PWM module\n");
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
  pwm_enable(0);
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
  if (string_token_eq(subcmd_token, "info"))
    return command_pwm_info(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(pwm, "access to PWM controller");
