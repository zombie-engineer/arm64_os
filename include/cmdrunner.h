#pragma once

#define CMDRUNNER_ERR_MAXCOMMANDSREACHED -1

#define CMD_ERR_NO_ERROR          0
#define CMD_ERR_PARSE_ARG_ERROR   1
#define CMD_ERR_UNKNOWN_SUBCMD    2
#define CMD_ERR_INVALID_ARGS      3
#define CMD_ERR_EXECUTION_ERR     4
#define CMD_ERR_NOT_IMPLEMENTED 255

#define COMMAND_MAX_COUNT 256
#define CMDLINE_BUF_SIZE 1024
#define MAX_HISTORY_LINES  8


typedef struct string_token {
  const char *s;
  int len;
} string_token_t;

typedef struct string_tokens {
  string_token_t *ts;
  int len;
} string_tokens_t;

#define STRING_TOKEN_ARGN(arg, n) (arg->ts[n])

int string_tokens_from_string(const char *string_start, const char *string_end, int maxlen, string_tokens_t *out);

int string_token_eq(const string_token_t *t, const char *str);

#define STRING_TOKENS_LOOP(t, var) \
  string_token_t *var = t->ts; \
  for(; var < &t->ts[t->len]; ++var)

/* generic signature for every command */
typedef int (*cmd_func)(const string_tokens_t *);


/* structure of a command */
typedef struct command {
  const char *name;
  const char *description;
  cmd_func func;
} command_t;


/* init command runner */
void cmdrunner_init(void);

/* add command to command runner */
int cmdrunner_add_cmd(
  const char *name, 
  const char *description, 
  cmd_func);


/* helper macro to add command. Addition of all commands 
 * is built around this macro.
 */
#define CMDRUNNER_ADD_CMD(cmd) \
  cmdrunner_add_cmd(get_command_name_ ## cmd (), get_command_description_ ## cmd (), get_command_func_ ## cmd ());


#define CMDRUNNER_DECL_CMD(name) \
  int command_ ## name (const string_tokens_t *); \
  const char * get_command_name_ ## name ();\
  const char * get_command_description_ ## name ();\
  cmd_func     get_command_func_ ## name ();


#define CMDRUNNER_IMPL_CMD(name, description) \
  const char * get_command_name_ ## name () { return #name; } \
  const char * get_command_description_ ## name () { return description; } \
  cmd_func     get_command_func_ ## name () { return command_ ## name; }


/* callback function type that recieves a command as input */
typedef int (*iter_cmd_cb)(command_t* cmd);


/* iterate all registered command and execute a callback function
 * for each of them. Iteration stops when all commands are 
 * iterated or when callback function returns non-zero.
 */
void cmdrunner_iterate_commands(iter_cmd_cb cb);


/* main interactive loop where user inputs commands */
void cmdrunner_run_interactive_loop(void);

int cmdrunner_process(int argc, char **argv);
