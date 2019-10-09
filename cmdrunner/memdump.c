#include <cmdrunner.h>
#include "argument.h"
#include <common.h>
#include <stdlib.h>
#include <types.h>

#define DUMPSIZE_BYTES       1
#define DUMPSIZE_HALFWORDS   2
#define DUMPSIZE_WORDS       4
#define DUMPSIZE_DOUBLEWORDS 8

static void memdump_dump_bytes(const char *ptr, size_t sz) 
{
  int i;
  for (i = 0; i < min(sz, 8); i++)
    printf("%02x ", *(ptr++));
  if (i == sz)
    return;

  putc(' ');
  for (i = 0; i < min(sz, 8); i++)
    printf("%02x ", *(ptr++));
}

static void memdump_dump_halfwords(const char *ptr, size_t sz) 
{

}

static void memdump_dump_words(const char *ptr, size_t sz) 
{

}

static void memdump_dump_doublewords(const char *ptr, size_t sz) 
{

}

static int memdump_print_help()
{
  puts("memdump [-h] [-BHWDQ] OFFSET:SIZE\n");
  puts("\tOFFSET - start memory address to dump\n");
  puts("\tSIZE   - number of bytes to dump\n");
  puts("\t-h - prints this help\n");
  puts("\t-B - dump single bytes (default)\n");
  puts("\t-H - dump half-words\n");
  puts("\t-W - dump words\n");
  puts("\t-D - dump double-words\n");
  return CMD_ERR_NO_ERROR;
}

int command_memdump_get_elementsize(char c) 
{
  switch (c) {
    case 'H': return DUMPSIZE_HALFWORDS;
    case 'W': return DUMPSIZE_WORDS;
    case 'D': return DUMPSIZE_DOUBLEWORDS;
    case 'B': 
    default : return DUMPSIZE_BYTES;
  }
}

int command_memdump(const string_tokens_t *args)
{
  int i, linesize;;
  // aptr - arguments pointer
  // ptr  - memdump pointer
  const char *ptr;
  char *endptr;
  size_t bytescount;
  char nameless_args, element_size;

  element_size = DUMPSIZE_BYTES;
  bytescount = 16;
  int offset_arg = 1;

  ASSERT_NUMARGS_GE(1);
  if (string_token_eq(&STRING_TOKEN_ARGN(args, 1), "help"))
    return memdump_print_help();

  if (STRING_TOKEN_ARGN(args, offset_arg).len > 1 && STRING_TOKEN_ARGN(args, offset_arg).s[0] == '-') {
    element_size = command_memdump_get_elementsize(STRING_TOKEN_ARGN(args, offset_arg).s[1]);
    offset_arg++;
  }

  /* should be enough arguments to contain offset and size */
  ASSERT_NUMARGS_GE(offset_arg + 2)
  GET_NUMERIC_PARAM(ptr, char *, offset_arg, "Memory address");
  GET_NUMERIC_PARAM(bytescount, size_t, offset_arg + 1, "Num elements to dump");

  while(bytescount) {
    int todump = min(16, bytescount);
    bytescount -= todump;
    switch (element_size) {
      case DUMPSIZE_BYTES:       memdump_dump_bytes      (ptr, todump); break;
      case DUMPSIZE_HALFWORDS:   memdump_dump_halfwords  (ptr, todump); break;
      case DUMPSIZE_WORDS:       memdump_dump_words      (ptr, todump); break;
      case DUMPSIZE_DOUBLEWORDS: memdump_dump_doublewords(ptr, todump); break;
      default: return CMD_ERR_PARSE_ARG_ERROR;
    }
    ptr += todump;
    putc('\r');
    putc('\n');
  }

  return CMD_ERR_NO_ERROR;
}

static int command_ww_print_help()
{
  puts("ww OFFSET VALUE\n");
  puts("\tOFFSET - 4 byte aligned memory destination address\n");
  puts("\tVALUE - 4 byte int value to write\n");
  return CMD_ERR_NO_ERROR;
}

static int command_rw_print_help()
{
  puts("rw OFFSET\n");
  puts("\tOFFSET - 4 byte aligned memory source address\n");
  return CMD_ERR_NO_ERROR;
}

int command_ww(const string_tokens_t *args)
{
  char *endptr;
  unsigned long long address;
  int value;

  puts("ww: ");
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(&args->ts[1], "help"))
    return command_ww_print_help();

  ASSERT_NUMARGS_EQ(3);

  GET_NUMERIC_PARAM(address, unsigned long long, 1, "address to write");
  GET_NUMERIC_PARAM(value  , int               , 2, "value to write");
  *(int*)address = value;
  return CMD_ERR_NO_ERROR;
}

int command_rw(const string_tokens_t *args)
{
  char *endptr;
  unsigned long long address;

  puts("rw: ");
  ASSERT_NUMARGS_GE(1);

  if (string_token_eq(&args->ts[1], "help"))
    return command_ww_print_help();

  GET_NUMERIC_PARAM(address, unsigned long long, 1, "address to read");
  printf("%08x (%d)\n", *(int*)address, *(int*)address);
  return CMD_ERR_NO_ERROR;
}

CMDRUNNER_IMPL_CMD(memdump, "dumps memory area");

CMDRUNNER_IMPL_CMD(ww, "write 4 bytes word to a 4 bytes aligned memory");
CMDRUNNER_IMPL_CMD(rw, "read a 4 bytes word from a 4 bytes aligned memory");
