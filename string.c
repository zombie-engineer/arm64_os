#include "string.h"
#include <exception.h>


int strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  while (n && *s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  return *s1 - *s2;
}

int strlen(const char* ptr)
{
  int res;
  res = 0;
  while(*ptr++)
    res++;
  return res;
}

int strnlen(const char* ptr, size_t n)
{
  int res;
  res = 0;
  while(*ptr++ && n--)
    res++;
  return res;
}

char * strcpy(char *dst, const char *src)
{
  char *res = dst;
  while(*src)
    *dst++ = *src++;
  return res;
}

char * strncpy(char *dst, const char *src, size_t n)
{
  char *res = dst;
  while(*src && n--)
    *dst++ = *src++;
  return res;
}

void * memset(void *dst, char value, size_t n)
{
  char *ptr = (char *)dst;
  while (n--) {
    *ptr++ = value;
  }
  return dst;
}

#define IS_ALIGNED(addr, sz) (!((unsigned long)addr % sz))
#define IS_8_ALIGNED(addr) IS_ALIGNED(addr, 8)
#define IS_4_ALIGNED(addr) IS_ALIGNED(addr, 4)

#define TYPEBYIDX(typ, addr, idx) (((typ*)addr)[i])
#define CPY_ARRAY_EL(type, dst, src, idx) TYPEBYIDX(type, dst, idx) = TYPEBYIDX(type, src, idx)

#define CPY_CHAR_EL(dst, src, idx) CPY_ARRAY_EL(char, dst, src, idx)
#define CPY_INT_EL(dst, src, idx) CPY_ARRAY_EL(int, dst, src, idx)
#define CPY_LONG_EL(dst, src, idx) CPY_ARRAY_EL(long, dst, src, idx)

void * memcpy_8aligned(void *dst, const void *src, size_t n)
{
  int i;
  if (!IS_8_ALIGNED(dst) || !IS_8_ALIGNED(src) || n % 8)
    generate_exception();

  for (i = 0; i < n / 8; ++i)
    CPY_LONG_EL(dst, src, i);

  return dst;
}

void * memcpy(void *dst, const void *src, size_t n)
{
  unsigned int i, imax;
  if (!IS_8_ALIGNED(dst) || !IS_8_ALIGNED(src)) {
    if (!IS_4_ALIGNED(dst) || !IS_4_ALIGNED(src)) {
      // copy 1 byte
      for (i = 0; i < n; i++)
        CPY_CHAR_EL(dst, src, i);
    }
    // copy 4 bytes aligned
    else {
      imax = (n / 4) * 4;
      for (i = 0; i < imax; ++i)
        CPY_INT_EL(dst, src, i);
      for (i = (n / 4) * 4; i < n; ++i)
        CPY_CHAR_EL(dst, src, i);
    }
  }
  else {
    memcpy_8aligned(dst, src, (n / 8) * 8);
    for (i = (n / 8) * 8; i < n; ++i)
      CPY_CHAR_EL(dst, src, i);
  }
  return dst;
}
