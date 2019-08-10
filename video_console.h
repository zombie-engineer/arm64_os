#pragma once
#include "console.h"

#define VIDEO_CONSOLE_NAME "videocon"

int video_console_init();
void video_console_puts(const char *str);
void video_console_putc(char chr);
