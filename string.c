#include "string.h"

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


 //void * memset(void *dst, int value, size_t num)
 //{
 //  void *res = dst;
 //  int rest = num;
 //  int i;
 //  while (rest > sizeof(long long))
 //  {
 //    rest -= sizeof(long long);
 //    *(long long *)(dst) = 
 //  }
 //  return res;
 //}
