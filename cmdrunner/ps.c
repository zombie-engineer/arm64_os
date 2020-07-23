#include <cmdrunner.h>
#include "argument.h"
#include <stringlib.h>
#include <sched.h>
#include <common.h>

static int command_ps_print_help()
{
  puts("ps - list all running processes\n");
 //  puts("\tCOMMANDS:\n");
 //  puts("\t\tset-clock-rate CLOCK_ID RATE - sets clock rate of a clock CLOCK_ID\n");
 //  puts("\t\tget-clock CLOCK_ID           - prints clock rate of a clock CLOCK_ID\n");
 //  puts("\t\tget-clocks                   - prints all clocks\n");
  return CMD_ERR_NO_ERROR;
}

static inline int cmdrunner_ps_print_one(char *buf, int bufsz, int cpu_n, struct task *t)
{
  return snprintf(buf, bufsz, "cpu:%d, task: %s\n", cpu_n, t->name);
}

static int cmdrunner_ps()
{
  char buf[256];
  int cpu_n;
  int n = 0;
  struct scheduler *s;
  struct task *t;
  for (cpu_n = 0; cpu_n < NUM_CORES; ++cpu_n) {
    s = get_scheduler_n(cpu_n);
    list_for_each_entry(t, &s->running, schedlist)
      n +=  cmdrunner_ps_print_one(buf + n, sizeof(buf) - n, cpu_n, t);

    list_for_each_entry(t, &s->timer_waiting, schedlist)
      n +=  cmdrunner_ps_print_one(buf + n, sizeof(buf) - n, cpu_n, t);

    list_for_each_entry(t, &s->flag_waiting, schedlist)
      n +=  cmdrunner_ps_print_one(buf + n, sizeof(buf) - n, cpu_n, t);
  }
  printf("%s", buf);
  return CMD_ERR_NO_ERROR;
}

int command_ps(const string_tokens_t *args)
{
  DECL_ARGS_CTX();
  // ASSERT_NUMARGS_GE(1);

  if (string_token_eq(subcmd_token, "help"))
    return command_ps_print_help();
  return cmdrunner_ps();
}

CMDRUNNER_IMPL_CMD(ps, "implementation of most of message box API");
