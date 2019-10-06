#include <cmdrunner.h>
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
}

int command_memdump(const string_tokens_t *args)
{
  int i, linesize;;
  // aptr - arguments pointer
  // ptr - memdump pointer
  const char *ptr, *endptr;
  size_t bytescount;
  char nameless_args, element_size;

  element_size = DUMPSIZE_BYTES;
  bytescount = 16;
  nameless_args = 0;

  // init and skip spaces
  //
  STRING_TOKENS_LOOP(args, t) {
    if (string_token_eq(t, "help")) {
      memdump_print_help();
      return CMD_ERR_NO_ERROR;
    }
    if (string_token_eq(t, "-B")) {
      element_size = DUMPSIZE_BYTES;
      continue;
    }
    if (string_token_eq(t, "-H")) {
      element_size = DUMPSIZE_HALFWORDS;
      continue;
    }
    if (string_token_eq(t, "-W")) {
      element_size = DUMPSIZE_WORDS;
      continue;
    }
    if (string_token_eq(t, "-D")) {
      element_size = DUMPSIZE_DOUBLEWORDS;
      continue;
    }
    if (nameless_args == 0) {
      ptr = (char *)strtoll(t->s, &endptr, 0);
      if (endptr > t->s) {
        nameless_args++;
        continue;
      }
      return CMD_ERR_PARSE_ARG_ERROR;
    }
    if (nameless_args == 1) {
      bytescount = (size_t)strtoll(t->s, &endptr, 0);
      if (endptr > t->s) {
        nameless_args++;
        break;
      }
      return CMD_ERR_PARSE_ARG_ERROR;
    }
  }

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

CMDRUNNER_IMPL_CMD(memdump, "dumps memory area");
