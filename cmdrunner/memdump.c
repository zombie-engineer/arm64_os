#include <cmdrunner.h>
#include <common.h>
#include <stdlib.h>

#define DUMPSIZE_BYTES       0
#define DUMPSIZE_HALFWORDS   1
#define DUMPSIZE_WORDS       2
#define DUMPSIZE_DOUBLEWORDS 3
#define DUMPSIZE_QUADWORDS   4

static void memdump_print_help()
{
  puts("memdump [-h] [-BHWDQ] OFFSET:SIZE\n");
  puts("\tOFFSET - start memory address to dump\n");
  puts("\tSIZE   - number of bytes to dump\n");
  puts("\t-h - prints this help\n");
  puts("\t-B - dump single bytes (default)\n");
  puts("\t-H - dump half-words\n");
  puts("\t-W - dump words\n");
  puts("\t-D - dump double-words\n");
  puts("\t-Q - dump quad-words\n");
}

int command_memdump(const char *args_start, const char *args_end)
{
  int i;
  // aptr - arguments pointer
  // ptr - memdump pointer
  const char *aptr, *ptr;
  int print_help;
  char dump_size = 0;
  print_help = 0;

  // init and skip spaces
  aptr = args_start;
  for(; aptr < args_end && isspace(aptr); aptr++);

  for(; aptr < args_end; ) {
    if (strncmp(aptr, "-h", min(2, args_end - aptr)) == 0) {
      print_help = 1;
      break;
    }
    if (strncmp(aptr, "-B", min(2, args_end - aptr)) == 0) {

    }
    aptr++;
  }

  if (print_help) {
    memdump_print_help();
    return CMD_ERR_NO_ERROR;
  }

  ptr = (const char *)0x0;
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
