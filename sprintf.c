#include "sprintf.h"

char bytecode[] = {
0x00,0x00,0xa8,0xd2,0x1f,0x00,0x00,0xb9,0x01,0x00,0xb0,0x52,0x01,0x08,0x00,0xb9,0x00,0x04,0x00,0x58,0x00,0xe0,0x1b,0xd5,0x7f,0xe0,0x1c,0xd5,0xe0,0x7f,0x86,0xd2,0x40,0x11,0x1e,0xd5,0x20,0xb6,0x80,0xd2,0x00,0x11,0x1e,0xd5,0x00,0x08,0x80,0xd2,0x20,0xf2,0x19,0xd5,0x20,0x03,0x00,0x58,0x00,0x10,0x1c,0xd5,0x20,0x79,0x80,0xd2,0x00,0x40,0x1e,0xd5,0x60,0x00,0x00,0x10,0x20,0x40,0x1e,0xd5,0xe0,0x03,0x9f,0xd6,0xa6,0x00,0x38,0xd5,0xc6,0x04,0x40,0x92,0xe6,0x00,0x00,0xb4,0xe5,0x03,0x00,0x10,0x5f,0x20,0x03,0xd5,0xa4,0x78,0x66,0xf8,0xc4,0xff,0xff,0xb4,0x00,0x00,0x80,0xd2,0x03,0x00,0x00,0x14,0x44,0x04,0x00,0x18,0x00,0x04,0x00,0x18,0x01,0x00,0x80,0xd2,0x02,0x00,0x80,0xd2,0x03,0x00,0x80,0xd2,0x80,0x00,0x1f,0xd6,0x00,0x00,0x00,0x00,0x00,0xf8,0x24,0x01,0x00,0x00,0x00,0x00,0x30,0x08,0xc5,0x30,0x00,0x00,0x00,0x00
};

unsigned int vsprintf(char *dst, const char *fmt, __builtin_va_list args)
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
        fmt++;
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
      else if (*fmt == 'x' || *fmt == 'X') {
        arg = __builtin_va_arg(args, long int);
        // convert to string
        i = 16;
        tmpstr[i] = 0;
        do {
          char n = arg & 0xf;
          // 0-9 >= '0' - '9', 10-15 => 'A' - 'F'
          n += (n <= 9 ? '0' : 0x37);
          if (n >= 'A' && *fmt == 'x')
            n += 0x20;
          tmpstr[--i] = n;
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

unsigned int sprintf(char *dst, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return vsprintf(dst, fmt, args);
}


