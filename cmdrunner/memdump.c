#include <cmdrunner.h>
#include <common.h>

int command_memdump(const char *args_start, const char *args_end)
{
  int i;
  const char *ptr = (const char *)0x0;
  for (i = 0; i < 8; ++i)
    printf(" %02x", ptr[i]);
  putc(' ');
  for (i = 0; i < 8; ++i)
    printf(" %02x", ptr[i]);
  putc('\r');
  putc('\n');

  return CMD_ERR_NOT_IMPLEMENTED;
}

CMDRUNNER_IMPL_CMD(memdump, "dumps memory area");
