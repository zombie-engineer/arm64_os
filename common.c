#include <common.h>
#include <stringlib.h>
#include <console.h>

#define PRINTF_BUF_SIZE 1024

char printfbuf[PRINTF_BUF_SIZE];

void _putc(char ch)
{
  console_putc(ch);
}

void _puts(const char *str)
{
  console_puts(str);
}

const char * _printf(const char* fmt, ...)
{
  const char *res = fmt;
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  vsnprintf(printfbuf, sizeof(printfbuf), fmt, &args);
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

static inline void __hexdump_line(const void *addr, int sz)
{
  int i;
  const unsigned char *ptr = addr;
  printf("%08llx:", (unsigned long long)addr);
  for (i = 0; i < sz; ++i)
    printf(" %02x", *ptr++);
  putc('\r');
  putc('\n');
}

void hexdump_memory_ex(const char *tag, int line_width, const void *addr, size_t sz)
{
  int last_sz;
  const unsigned char *ptr = addr;
  while(sz) {
    last_sz = min(line_width, sz);
    puts(tag);
    puts(" ");
    __hexdump_line(ptr, last_sz);
    ptr += last_sz;
    sz -= last_sz;
  }
}

void hexdump_memory(const void *addr, size_t sz)
{
  const int line_width = 16;
  int last_sz;
  const unsigned char *ptr = addr;
  while(sz) {
    last_sz = min(line_width, sz);
    __hexdump_line(ptr, last_sz);
    ptr += last_sz;
    sz -= last_sz;
  }
}

int prefix_padding_to_string(const char *prefix, char padchar, int depth, int repeat, char *buf, int buf_sz)
{
  int buf_sz_saved = buf_sz;
  char c;

  while(buf_sz > 0) {
    c = *prefix;
    if (!c)
      break;

    *buf++ = c;
    buf_sz--;
    prefix++;
  }
  depth *= repeat;
  while(buf_sz > 0 && depth) {
    *buf++ = padchar;
    depth--;
  }
  *buf = 0;
  return buf_sz_saved - buf_sz;
}
