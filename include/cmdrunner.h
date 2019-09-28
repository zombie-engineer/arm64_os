#pragma once
#include <commands.h>

#define CMDRUNNER_ERR_NAMETOOLONG        -1
#define CMDRUNNER_ERR_MAXCOMMANDSREACHED -2

int cmdrunner_add_cmd(const char* name, unsigned int namelen, cmd_func);

void cmdrunner_init(void);

void cmdrunner_run_interactive_loop(void);
