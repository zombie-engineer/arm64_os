#include <cmdrunner.h>

int command_memdump(const char *args_start, const char *args_end)
{
  return CMD_ERR_NOT_IMPLEMENTED;
}

CMDRUNNER_IMPL_CMD(memdump, "dumps memory area");
