#pragma once
#include "console.h"

#define VIEWPORT_CONSOLE_NAME "viewportcon"

int viewport_console_init();
void viewport_console_puts(const char *str);
void viewport_console_putc(char chr);
