#include <cmdrunner.h>
#include "argument.h"
#include <common.h>
#include <delays.h>
#include <stringlib.h>
#include <types.h>

static int sleep_print_help()
{
  puts("sleep MILLISECONDS - sleep given number of milliseconds\n");

  return CMD_ERR_NO_ERROR;
}

int command_sleep(const string_tokens_t *args)
{
  int milliseconds;;
  char *endptr;

  ASSERT_NUMARGS_EQ(1);

  if (string_token_eq(&STRING_TOKEN_ARGN(args, 0), "help"))
    return sleep_print_help();

  GET_NUMERIC_PARAM(milliseconds, int, 0, "milliseconds");
  wait_msec(milliseconds);
  return CMD_ERR_NO_ERROR;
}


CMDRUNNER_IMPL_CMD(sleep, "sleeps given number of seconds");
