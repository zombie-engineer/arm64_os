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

void hexdump_addr(unsigned int *addr)
{
  int i;
  volatile unsigned int *base_ptr = (volatile unsigned int *)addr;
  for (i = 0; i < 32; ++i)
  {
    printf("%08x: %08x %08x %08x %08x\n", 
        base_ptr + i * 4,
        *(base_ptr + i * 4 + 0),
        *(base_ptr + i * 4 + 1),
        *(base_ptr + i * 4 + 2),
        *(base_ptr + i * 4 + 3)
    );
  }
}


