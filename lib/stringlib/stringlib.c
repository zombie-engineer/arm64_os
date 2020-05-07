#include <stringlib.h>
#include <common.h>

int _isprint(char ch)
{
  return (ch >= 0x20 && ch < 0x7f) || ch == '\n' || ch == ' ';
}

int _isdigit(char ch)
{
  return ch >= '0' && ch <= '9';
}

int isspace(char c) {
  //return c == ' ' || c == '\t' || c == '\f' || c == '\n' || c == '\r' || c == '\v' ;
  return c == ' ' || (c >= 0x9 && c <= 0xd);
}


int _strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int _memcmp(const void *a, const void *b, size_t n)
{
  const char *p1 = a;
  const char *p2 = b;
  while(n--) {
    if (*p1 != *p2)
      return *p1 - *p2;
  }
  return 0;
}

int _strncmp(const char *s1, const char *s2, size_t n)
{
  while (n && *s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  if (!n)
    return 0;
  return *s1 - *s2;
}

int _strlen(const char* ptr)
{
  int res;
  res = 0;
  while(*ptr++)
    res++;
  return res;
}

int _strnlen(const char* ptr, size_t n)
{
  int res;
  res = 0;
  while(*ptr++ && n--)
    res++;
  return res;
}

char * _strcpy(char *dst, const char *src)
{
  char *res = dst;
  while(*src)
    *dst++ = *src++;
  *dst = 0;
  return res;
}

char * _strncpy(char *dst, const char *src, size_t n)
{
  char *res = dst;
  char tmp;
  while(n) {
    tmp = *src++; 
    *dst++ = tmp;
    n--;
    if (!tmp)
      break;
  }
  while(n--)
    *dst++ = 0;
  return res;
}

void * _memset(void *dst, char value, size_t n)
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
#ifndef TEST_STRING
  if (!IS_8_ALIGNED(dst) || !IS_8_ALIGNED(src) || n % 8)
    kernel_panic("memcpy not aligned");
#endif

  for (i = 0; i < n / 8; ++i)
    CPY_LONG_EL(dst, src, i);

  return dst;
}

void * _memcpy(void *dst, const void *src, size_t n)
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
      for (i = 0; i < imax / 4; ++i)
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

void wtomb(char *buf, size_t buf_sz, char *src, int src_sz)
{
  const char *sptr = src;
  const char *send = src + src_sz;

  char *dptr = buf;
  char *dend = buf + buf_sz;
  char c;

  if (dptr >= dend)
    return;

  c = *sptr++;
  while (1) {
    *sptr++;
    *dptr++ = c;
    if (dptr >= dend) {
      *(dptr - 1) = 0;
      break;
    }
    c = *sptr++;
    if (!c) {
      *dptr = 0;
      break;
    }
    if (send - sptr < 1) {
      *dptr++ = c;
      *dptr = 0;
      break;
    }
  } 
}
