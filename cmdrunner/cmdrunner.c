#include <cmdrunner.h>
#include <uart/uart.h>
#include <string.h>
#include <common.h>
#include <console_char.h>

CMDRUNNER_DECL_CMD(ls);
CMDRUNNER_DECL_CMD(memdump);
CMDRUNNER_DECL_CMD(help);

static command_t commands[COMMAND_MAX_COUNT];
static unsigned int num_commands;

static char inputbuf[CMDLINE_BUF_SIZE];
static char *inputbuf_end;
static char *inputbuf_carret;

#define MAX_CMD_STRCMP_LEN 16


static void cmdrunner_on_newline(void)
{
  unsigned int i, maxarglen, cmplen, res;
  maxarglen = inputbuf_end - inputbuf;
  for (i = 0; i < num_commands; ++i) {
    command_t *cmd = &commands[i];
    cmplen = min(maxarglen, MAX_CMD_STRCMP_LEN);
    if (strncmp(cmd->name, inputbuf, cmplen) == 0) {
      res = cmd->func(inputbuf_carret, inputbuf_end);
      if (res) {
        printf("Command completed with error code: %d\n", res);
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
