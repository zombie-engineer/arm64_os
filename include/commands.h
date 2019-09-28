#pragma once

#define CMD_ERR_NOT_IMPLEMENTED 255

typedef int (*cmd_func)(const char*, const char*);

