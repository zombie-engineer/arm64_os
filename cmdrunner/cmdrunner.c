#include <cmdrunner.h>
#include <uart/uart.h>
#include <stringlib.h>
#include <common.h>
#include <console_char.h>
#include <vcanvas.h>
#include <exception.h>
#include "cmdrunner_internal.h"
#include <error.h>
#include <debug.h>

CMDRUNNER_DECL_CMD(clock);
CMDRUNNER_DECL_CMD(f5161ah);
CMDRUNNER_DECL_CMD(gpio);
CMDRUNNER_DECL_CMD(help);
CMDRUNNER_DECL_CMD(ls);
CMDRUNNER_DECL_CMD(max7219);
CMDRUNNER_DECL_CMD(mbox);
CMDRUNNER_DECL_CMD(memdump);
#ifdef CONFIG_NOKIA_5110
CMDRUNNER_DECL_CMD(nokia5110);
#endif
CMDRUNNER_DECL_CMD(pwm);
CMDRUNNER_DECL_CMD(rw);
CMDRUNNER_DECL_CMD(shiftreg);
CMDRUNNER_DECL_CMD(sleep);
CMDRUNNER_DECL_CMD(spi);
CMDRUNNER_DECL_CMD(ww);
CMDRUNNER_DECL_CMD(ps);

static command_t commands[COMMAND_MAX_COUNT];
static unsigned int num_commands;


typedef struct  {
  char s[CMDLINE_BUF_SIZE];
  int len;
} history_element_t;

typedef struct {
  history_element_t h[MAX_HISTORY_LINES];
  int current;
  int last;
} history_t;

static history_t hist;

static char inputbuf[CMDLINE_BUF_SIZE];
static char *inputbuf_end;
static char *inputbuf_carret;

static int cmdrunner_history_prev()
{
  int tmp_current;
  tmp_current = hist.current;
  if (hist.current == 0) {
    if (hist.h[MAX_HISTORY_LINES - 2].s[0])
      hist.current = MAX_HISTORY_LINES - 2;
  } else {
    if (hist.h[hist.current - 1].s[0])
      hist.current--;
  }
  return hist.current != tmp_current;
}

static int cmdrunner_history_next()
{
  int tmp_current;
  tmp_current = hist.current;

  if (hist.current + 1 != MAX_HISTORY_LINES) {
    if (hist.h[hist.current + 1].s[0])
      hist.current++;
  }
  else
    hist.current = 0;

  return hist.current != tmp_current;
}


static void cmdrunner_backspace(void)
{
  if (inputbuf_carret == inputbuf)
    return;
  *inputbuf_carret-- = 0;
  putc(CONSOLE_CHAR_BACKSPACE);
  putc(' ');
  putc(CONSOLE_CHAR_BACKSPACE);
}

static void cmdrunner_clear_line(void)
{
  while (inputbuf_carret > inputbuf)
    cmdrunner_backspace();
}

static void cmdrunner_history_fetch()
{
  const char *p;
  cmdrunner_clear_line();
  memcpy(inputbuf, hist.h[hist.current].s, hist.h[hist.current].len);
  inputbuf_carret = inputbuf + hist.h[hist.current].len;
  *inputbuf_carret = 0;
  for (p = inputbuf; *p; p++) putc(*p);
}


static void cmdrunner_history_init()
{
  memset(&hist, 0, sizeof(hist));
}

static void cmdrunner_history_put()
{
  if (strncmp(hist.h[hist.last].s, inputbuf, inputbuf_carret - inputbuf) == 0)
    return;

  if (++hist.last == MAX_HISTORY_LINES)
    hist.last = 0;
  memcpy(hist.h[hist.last].s, inputbuf, inputbuf_carret - inputbuf);
  hist.h[hist.last].len = inputbuf_carret - inputbuf;
  hist.current = hist.last + 1;
  if (hist.current == MAX_HISTORY_LINES)
    hist.current = 0;
}


static const char *cmdrunner_err_to_str(int err)
{
#define CASE(a) case a: return #a
  switch(err) {
    CASE(CMD_ERR_NO_ERROR);
    CASE(CMD_ERR_PARSE_ARG_ERROR);
    CASE(CMD_ERR_NOT_IMPLEMENTED);
    CASE(CMD_ERR_UNKNOWN_SUBCMD);
    CASE(CMD_ERR_INVALID_ARGS);
    CASE(CMD_ERR_EXECUTION_ERR);
    default: return "CMD_ERR_UNDEFINED";
  }
#undef CASE
}

static command_t *cmdrunner_get_command(string_token_t *cmd_token)
{
  int i;
  command_t *cmd;

  /* find COMMAND in the list of registered commands */
  for (i = 0; i < num_commands; ++i) {
    cmd = &commands[i];
    if (string_token_eq(cmd_token, cmd->name))
      return cmd;
  }
  return NULL;
}

static void cmdrunner_on_newline(void)
{
  int err;
  command_t *cmd;

  string_token_t stokens[8];
  string_tokens_t tokens, args;

  cmdrunner_history_put();
  putc(CONSOLE_CHAR_LINEFEED);
  tokens.ts = &stokens[0];

  if (tokenize_string(inputbuf, inputbuf_carret, ARRAY_SIZE(stokens), &tokens)) {
    printf("Failed to parse command line to string tokens\n");
    return;
  }

  cmd = cmdrunner_get_command(&tokens.ts[0]);
  if (!cmd) {
    printf("Unknown command: %s\n", inputbuf);
    return;
  }

  args.ts = &tokens.ts[1];
  args.len = tokens.len - 1;

  err = cmd->func(&args);
  if (err)
    printf("Command completed with error code: %d (%s)\n", err, cmdrunner_err_to_str(err));
}

static void cmdrunner_history_scroll_up()
{
  if (cmdrunner_history_prev())
    cmdrunner_history_fetch();
}

static void cmdrunner_history_scroll_down()
{
  if (cmdrunner_history_next())
    cmdrunner_history_fetch();
}

static void cmdrunner_on_arrow_up(void)
{
  int x, y;
  x = y = 0;
  cmdrunner_history_scroll_up();
  vcanvas_puts(&x, &y, "UP");
}

static void cmdrunner_on_arrow_down(void)
{
  int x, y;
  x = y = 0;
  cmdrunner_history_scroll_down();
  vcanvas_puts(&x, &y, "DOWN");
}

static void cmdrunner_flush_inputbuf(void)
{
  inputbuf_carret = inputbuf;
  memset(inputbuf, 0, CMDLINE_BUF_SIZE);
}

void cmdrunner_init(void)
{
  inputbuf_end = inputbuf + CMDLINE_BUF_SIZE;
  cmdrunner_flush_inputbuf();
  cmdrunner_history_init();
  num_commands = 0;

  CMDRUNNER_ADD_CMD(clock);
  CMDRUNNER_ADD_CMD(f5161ah);
  CMDRUNNER_ADD_CMD(gpio);
  CMDRUNNER_ADD_CMD(help);
  CMDRUNNER_ADD_CMD(ls);
  CMDRUNNER_ADD_CMD(max7219);
  CMDRUNNER_ADD_CMD(mbox);
  CMDRUNNER_ADD_CMD(memdump);
#ifdef CONFIG_NOKIA_5110
  CMDRUNNER_ADD_CMD(nokia5110);
#endif
  CMDRUNNER_ADD_CMD(rw);
  CMDRUNNER_ADD_CMD(pwm);
  CMDRUNNER_ADD_CMD(shiftreg);
  CMDRUNNER_ADD_CMD(sleep);
  CMDRUNNER_ADD_CMD(spi);
  CMDRUNNER_ADD_CMD(ww);
  CMDRUNNER_ADD_CMD(ps);
  printf("cmdrunner_init completed"__endline);

  cmdrunner_state_t s;
  cmdrunner_state_init(&s);
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

int cmdrunner_handle_char(cmdrunner_state_t *s, char c)
{
  if (s->escbuflen > 0) {
    if (s->escbuflen == 1) {
      if (c == CONSOLE_CHAR_LEFT_BRACKET) {
        // vcanvas_puts(&x, &y, "[");
        s->escbuflen++;
      }
      else {
        //cmdrunner_clear_line();
        kernel_panic("cmdrunner_run_interactive_loop: Logic error.");
        return ERR_FATAL;
      }
    } else {
      switch (c) {
        case CONSOLE_CHAR_UP_ARROW:    cmdrunner_on_arrow_up()   ; break;
        case CONSOLE_CHAR_DOWN_ARROW:  cmdrunner_on_arrow_down() ; break;
        case CONSOLE_CHAR_LEFT_ARROW:  /*vcanvas_puts(&x, &y, "LEFT") ; */break;
        case CONSOLE_CHAR_RIGHT_ARROW: /*vcanvas_puts(&x, &y, "RIGHT"); */break;
        default: /*vcanvas_puts(&x, &y, "0");*/ break;
      }
      s->escbuflen = 0;
    }
    return ERR_OK;
  }

  if (c == CONSOLE_CHAR_ESC) {
    // vcanvas_puts(&x, &y, "ESC");
    s->escbuflen = 1;
    return ERR_OK;
  }

  if (c == CONSOLE_CHAR_LINEFEED || c == 0x0d) {
    cmdrunner_on_newline();
    cmdrunner_flush_inputbuf();
    puts("\n\r >");
  } else if (c == CONSOLE_CHAR_BACKSPACE || c == CONSOLE_CHAR_ASCII_BACKSPACE) {
    cmdrunner_backspace();
  } else {
    if (inputbuf_carret < inputbuf_end)
      *inputbuf_carret++ = c;
    putc(c);
  }
  return ERR_OK;
}

void cmdrunner_state_init(cmdrunner_state_t *s)
{
  s->escbuflen = 0;
  memset(s->escbuf, 0, sizeof(s->escbuf));
}

void cmdrunner_run_interactive_loop(void)
{
  char ch;
  cmdrunner_state_t s;
  cmdrunner_state_init(&s);

  puts("\n >");

  while(1) {
    ch = uart_getc();
    cmdrunner_handle_char(&s, ch);
    // sprintf(buf, "%02x ", ch);
    // vcanvas_puts(&x, &y, buf);
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

int string_token_eq(const string_token_t *t, const char *str)
{
  return strncmp(t->s, str, t->len) == 0;
}

