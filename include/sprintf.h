#pragma once

unsigned int vsprintf(char *dst, const char *fmt, __builtin_va_list args);

unsigned int sprintf(char *dst, const char *fmt, ...);
