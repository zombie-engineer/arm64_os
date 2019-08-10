#include "sprintf.h"

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args)
{
  long int arg;
  int len, sign, i;
  char *p;
  char *orig;
  char tmpstr[19];
  orig = dst;

  while(*fmt) {
    // argument access
    if (*fmt == '%') {
      fmt++;
      // literal %
      if (*fmt == '%') 
        goto put;
      len = 0;
      // size modifier
      while(*fmt >= '0' && *fmt <= '9') {
        len *= 10;
        len += *fmt - '0';
        fmt++;
      }
      // skip long modifier
      if (*fmt == 'l')
        fmt++;
      // character
      if (*fmt == 'c') {
        arg = __builtin_va_arg(args, int);
        *dst++ = (char)arg;
        continue;
      } 
      // if decimal number
      else if (*fmt == 'd') {
        arg = __builtin_va_arg(args, int);
        // check input
        sign = 0;
        if ((int)arg < 0) {
          arg *= -1;
          sign++;
        }
        if (arg > 99999999999999999L) 
          arg = 99999999999999999L;
        // convert to string
        i = 18;
        tmpstr[i] = 0;
        do {
          tmpstr[--i] = '0' + (arg % 10);
          arg /= 10;
        } while (arg != 0 && i > 0);

        if (sign)
          tmpstr[--i] = '-';

        if (len > 0 && len < 18) {
          while(i > 18 - len)
            tmpstr[--i] = ' ';
        }
        p = &tmpstr[i];
        goto copystring;
      } 
      // if hex number
      else if (*fmt == 'x') {
        arg = __builtin_va_arg(args, long int);
        // convert to string
        i = 16;
        tmpstr[i] = 0;
        do {
          char n = arg & 0xf;
          // 0-9 >= '0' - '9', 10-15 => 'A' - 'F'
          tmpstr[--i] = n + (n <= 9 ? '0' : 0x37);
          arg >>= 4;
        } while (arg != 0 && i > 0);
        // padding, only leading zeroes
        if (len > 0 && len <= 16) {
          while (i > 16 - len)
            tmpstr[--i] = '0';
        }
        p = &tmpstr[i];
        goto copystring;
      } 
      //string
      else if (*fmt == 's') {
        p = __builtin_va_arg(args, char*);
copystring:
        if (p == (void*)0) 
          p = "(null)";
        while(*p)
          *dst++ = *p++;
      } 
    } 
    else 
put:
      *dst++ = *fmt;
    fmt++;
  }
  *dst = 0;
  // number of bytes written
  return dst - orig;
}

unsigned int sprintf(char *dst, char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return vsprintf(dst, fmt, args);
}


