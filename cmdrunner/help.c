#include <cmdrunner.h>
#include <common.h>

static int command_printer(command_t *cmd)
{
  printf("%s: %s\n", cmd->name, cmd->description);
  return 0;
}

int command_help(const string_tokens_t *args)
{
  puts("Simple text command interpreter. Available commands: \n");
  cmdrunner_iterate_commands(command_printer);
  return CMD_ERR_NO_ERROR;
}

CMDRUNNER_IMPL_CMD(help, "prints this help");
