#include <cmdrunner.h>
#include <uart/uart.h>
#include <string.h>
#include <common.h>

#define COMMAND_NAME_MAXLEN 16
#define COMMAND_MAX_COUNT 256
#define CMDLINE_BUF_SIZE 1024

typedef struct command {
  char name[COMMAND_NAME_MAXLEN];
  unsigned int namelen;
  cmd_func func;
} command_t;

static command_t commands[COMMAND_MAX_COUNT];
static unsigned int num_commands;

static char inputbuf[CMDLINE_BUF_SIZE];
static char *inputbuf_end;
static char *inputbuf_carret;

static void cmdrunner_on_newline(void)
{
  unsigned int i, maxarglen, cmplen, res;
  maxarglen = inputbuf_end - inputbuf;
  for (i = 0; i < num_commands; ++i) {
    command_t *cmd = &commands[i];
    cmplen = min(maxarglen, cmd->namelen);
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
  putc('\r');
  putc('\n');
}

static void cmdrunner_backspace(void)
{
  if (inputbuf_carret == inputbuf)
    return;
  *inputbuf_carret-- = 0;
  putc(8);
}

void cmdrunner_init(void)
{
  inputbuf_end = inputbuf + CMDLINE_BUF_SIZE;
  cmdrunner_flush_inputbuf();
  num_commands = 0;
}

int cmdrunner_add_cmd(const char* name, unsigned int namelen, cmd_func func)
{
  if (num_commands == COMMAND_MAX_COUNT)
    return CMDRUNNER_ERR_MAXCOMMANDSREACHED;
  if (namelen > COMMAND_NAME_MAXLEN)
    return CMDRUNNER_ERR_NAMETOOLONG;

  command_t *newcmd = &commands[num_commands++];
  memset(newcmd, 0, sizeof(command_t));
  strncpy(newcmd->name, name, namelen);
  newcmd->namelen = namelen;
  newcmd->func = func;
  return 0;
}

void cmdrunner_run_interactive_loop(void)
{
  char ch;
  while(1) {
    ch = uart_getc();
    if (ch == '\n') {
      cmdrunner_newline();
      cmdrunner_on_newline();
      cmdrunner_flush_inputbuf();
    } else if (ch == 8) {
      cmdrunner_backspace();
    } else {
      if (inputbuf_carret < inputbuf_end)
        *inputbuf_carret++ = ch;
      putc(ch);
    }
  }
}
