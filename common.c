#include "common.h"
#include "sprintf.h"
#include "console.h"

#define PRINTF_BUF_SIZE 512
char printfbuf[PRINTF_BUF_SIZE];

const char * printf(const char* fmt, ...)
{
  const char *res = fmt;
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsprintf(printfbuf, fmt, args);
  console_puts(printfbuf);
  return res;
}
