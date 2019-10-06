#include <cmdrunner.h>
#include <uart/uart.h>
#include <string.h>
#include <string_utils.h>
#include <common.h>
#include <console_char.h>

CMDRUNNER_DECL_CMD(ls);
CMDRUNNER_DECL_CMD(memdump);
CMDRUNNER_DECL_CMD(help);
CMDRUNNER_DECL_CMD(gpio);

static command_t commands[COMMAND_MAX_COUNT];
static unsigned int num_commands;

static char inputbuf[CMDLINE_BUF_SIZE];
static char *inputbuf_end;
static char *inputbuf_carret;

#define MAX_CMD_STRCMP_LEN 16

static const char *cmdrunner_err_to_str(int err) 
{
#define CASE(a) case a: return #a
  switch(err) {
    CASE(CMD_ERR_NO_ERROR);
    CASE(CMD_ERR_PARSE_ARG_ERROR);
    CASE(CMD_ERR_NOT_IMPLEMENTED);
    CASE(CMD_ERR_UNKNOWN_SUBCMD);
    CASE(CMD_ERR_INVALID_ARGS);
    default: return "CMD_ERR_UNDEFINED";
  }
#undef CASE
}

static void cmdrunner_on_newline(void)
{
  unsigned int i, res;

  string_token_t stokens[4];
  string_tokens_t tokens;
  tokens.ts = &stokens;
  if (string_tokens_from_string(inputbuf, inputbuf_carret, ARRAY_SIZE(stokens), &tokens)) {
    printf("Failed to parse command line to string tokens\n");
    return;
  }

  // find COMMAND in the list of registered commands
  for (i = 0; i < num_commands; ++i) {
    command_t *cmd = &commands[i];
    if (string_token_eq(&tokens.ts[0], cmd->name)) {
      res = cmd->func(&tokens);
      if (res) {
        printf("Command completed with error code: %d (%s)\n", res, cmdrunner_err_to_str(res));
      }
      return;
    }
  }
  printf("Unknown command: %s\n", inputbuf);
}

static void cmdrunner_flush_inputbuf(void)
{
  inputbuf_carret = inputbuf;
  memset(inputbuf, 0, CMDLINE_BUF_SIZE);
}

static void cmdrunner_newline(void)
{
  putc('\n');
}

static void cmdrunner_backspace(void)
{
  if (inputbuf_carret == inputbuf)
    return;
  *inputbuf_carret-- = 0;
  putc(CONSOLE_CHAR_BACKSPACE);
}

void cmdrunner_init(void)
{
  inputbuf_end = inputbuf + CMDLINE_BUF_SIZE;
  cmdrunner_flush_inputbuf();
  num_commands = 0;

  CMDRUNNER_ADD_CMD(ls);
  CMDRUNNER_ADD_CMD(memdump);
  CMDRUNNER_ADD_CMD(help);
  CMDRUNNER_ADD_CMD(gpio);
}

int cmdrunner_add_cmd(
  const char *name, 
  const char *description, 
  cmd_func func)
{
  if (num_commands == COMMAND_MAX_COUNT)
    return CMDRUNNER_ERR_MAXCOMMANDSREACHED;

  command_t *newcmd = &commands[num_commands++];
  newcmd->name = name;
  newcmd->description = description;
  newcmd->func = func;
  return 0;
}

void cmdrunner_run_interactive_loop(void)
{
  char ch;
  cmdrunner_newline();
  puts(" >");

  while(1) {
    ch = uart_getc();
    if (ch == '\n') {
      cmdrunner_newline();
      cmdrunner_on_newline();
      cmdrunner_flush_inputbuf();
      puts(" >");
    } else if (ch == CONSOLE_CHAR_BACKSPACE) {
      cmdrunner_backspace();
    } else {
      if (inputbuf_carret < inputbuf_end)
        *inputbuf_carret++ = ch;
      putc(ch);
    }
  }
}

void cmdrunner_iterate_commands(iter_cmd_cb cb)
{
  unsigned int i;
  command_t *cmd;
  for (i = 0; i < num_commands; ++i) {
    cmd = &commands[i];
    if (cb(cmd))
      break;
  }
}

int string_tokens_from_string(const char *string_start, const char *string_end, int maxlen, string_tokens_t *out)
{
  int i;
  const char *ptr;

  ptr = string_start;

  SKIP_WHITESPACES_BOUND(ptr, string_end);
  if (!maxlen)
    return -1;

  out->ts[0].s = ptr;
  out->len = i = 1;

  while(1) {
    SKIP_NONWHITESPACES_BOUND(ptr, string_end);
    out->ts[i - 1].len = ptr - out->ts[i - 1].s;
    if (ptr == string_end)
      break;

    if (i == maxlen)
      break;

    SKIP_WHITESPACES_BOUND(ptr, string_end);
    if (ptr == string_end)
      break;

    out->ts[i++].s = ptr;
    out->len++;
  }
  return 0;
}

int string_token_eq(const string_token_t *t, const char *str)
{
  return strncmp(t->s, str, t->len) == 0;
}
