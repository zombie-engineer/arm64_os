#include "string.h"


int strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, unsigned int n)
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

int strnlen(const char* ptr, int n)
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

char * strncpy(char *dst, const char *src, int n)
{
  char *res = dst;
  while(*src && n--)
    *dst++ = *src++;
  return res;
}

void * memset(void *dst, char value, unsigned int n)
{
  char *ptr = (char *)dst;
  while (n--) {
    *ptr++ = value;
  }
  return dst;
}
