#include <cmdrunner.h>
#include "argument.h"
#include <stringlib.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <common.h>

static int command_mbox_print_help()
{
  puts("mbox COMMAND VALUES\n");
  puts("\tCOMMANDS:\n");
  puts("\t\tset-clock-rate CLOCK_ID RATE - sets clock rate of a clock CLOCK_ID\n");
  puts("\t\tget-clock CLOCK_ID           - prints clock rate of a clock CLOCK_ID\n");
  puts("\t\tget-clocks                   - prints all clocks\n");
  return CMD_ERR_NO_ERROR;
}

typedef struct {
  char name[16];
  uint32_t   id;
} clock_info_t;

static clock_info_t clock_names[10];

static clock_info_t* command_mbox_get_clock_infos(int *num)
{
  memset(&clock_names, 0, sizeof(clock_names));

#define SET_CLKNAME(NAME) \
  strcpy(clock_names[MBOX_CLOCK_ID_ ## NAME - 1].name, #NAME);\
  clock_names[MBOX_CLOCK_ID_ ## NAME - 1].id = MBOX_CLOCK_ID_ ## NAME;

  SET_CLKNAME(EMMC);
  SET_CLKNAME(UART);
  SET_CLKNAME(ARM);
  SET_CLKNAME(CORE);
  SET_CLKNAME(V3D);
  SET_CLKNAME(H264);
  SET_CLKNAME(ISP);
  SET_CLKNAME(SDRAM);
  SET_CLKNAME(PIXEL);
  SET_CLKNAME(PWM);
  SET_CLKNAME(EMMC2);

  *num = ARRAY_SIZE(clock_names);
  return clock_names;
}

#define RUN_CHK_STATUS(fn, args) \
  {\
    status = fn args;\
    if (status) {\
      printf(#fn " failed with error: %d\n", status);\
      return CMD_ERR_EXECUTION_ERR;\
    }\
  }

static int command_mbox_get_clock_worker(const clock_info_t *i)
{
  uint32_t enabled, exists, clock_rate,
           min_clock_rate, max_clock_rate,
           status;

  RUN_CHK_STATUS(mbox_get_clock_rate , (i->id, &clock_rate));
  RUN_CHK_STATUS(mbox_get_min_clock_rate , (i->id, &min_clock_rate));
  RUN_CHK_STATUS(mbox_get_max_clock_rate , (i->id, &max_clock_rate));
  RUN_CHK_STATUS(mbox_get_clock_state, (i->id, &enabled, &exists));
  printf("\t%s: %s%s, %dHz, min: %dHz, max: %dHz\n",
    i->name,
    enabled ? " on" : "off",
    exists  ? "   "   : " ,x",
    clock_rate,
    min_clock_rate,
    max_clock_rate);

  return CMD_ERR_NO_ERROR;
}

static int command_mbox_get_clock(const string_tokens_t *args)
{
  int num, i;
  ASSERT_NUMARGS_EQ(1);
  DECL_ASSIGN_PTR(clock_info_t, names, command_mbox_get_clock_infos(&num));
  for (i = 0; i < num; ++i) {
    if (string_token_eq(&args->ts[0], names[i].name))
      return command_mbox_get_clock_worker(&names[i]);
  }
  return CMD_ERR_INVALID_ARGS;
}

static int command_mbox_get_clocks(const string_tokens_t *args)
{
  puts("mbox get-clocks: \r\n");
  ASSERT_NUMARGS_EQ(0);

  int num, i, status;
  DECL_ASSIGN_PTR(clock_info_t, names, command_mbox_get_clock_infos(&num));

  for (i = 0; i < num; ++i) {
    RUN_CHK_STATUS(command_mbox_get_clock_worker, (&names[i]));
  }

  return CMD_ERR_NO_ERROR;
}

static int command_mbox_set_clock_rate(const string_tokens_t *args)
{
  char *endptr;
  int num, i, status;
  uint32_t rate;

  puts("mbox set-clock-rate: \r\n");
  ASSERT_NUMARGS_EQ(2);
  GET_NUMERIC_PARAM(rate, uint32_t, 1, "clock rate\n");

  DECL_ASSIGN_PTR(clock_info_t, names, command_mbox_get_clock_infos(&num));

  for (i = 0; i < num; ++i) {
    if (string_token_eq(&args->ts[0], names[i].name)) {
      printf("setting clock rate for clock:%s, id: %d, hz: %d\n", names[i].name, names[i].id, rate);
      RUN_CHK_STATUS(mbox_set_clock_rate, (names[i].id, &rate, 0));
      return CMD_ERR_NO_ERROR;
    }
  }
  return CMD_ERR_INVALID_ARGS;
}

int command_mbox(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_mbox_print_help();
  if (string_token_eq(subcmd_token, "get-clock"))
    return command_mbox_get_clock(&subargs);
  if (string_token_eq(subcmd_token, "get-clocks"))
    return command_mbox_get_clocks(&subargs);
  if (string_token_eq(subcmd_token, "set-clock-rate"))
    return command_mbox_set_clock_rate(&subargs);

  return CMD_ERR_UNKNOWN_SUBCMD;
}

CMDRUNNER_IMPL_CMD(mbox, "implementation of most of message box API");
