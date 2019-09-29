#pragma once

#define CMDRUNNER_ERR_MAXCOMMANDSREACHED -1

#define CMD_ERR_NO_ERROR        0
#define CMD_ERR_NOT_IMPLEMENTED 255

#define COMMAND_MAX_COUNT 256
#define CMDLINE_BUF_SIZE 1024


/* generic signature for every command */
typedef int (*cmd_func)(const char*, const char*);


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
  command_ ## name (const char*, const char*); \
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
