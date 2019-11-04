#include <stringlib.h>
#include <limits.h>
#include <common.h>
#include <compiler.h>

#define MAX_NEG_LLD         0x8000000000000000
#define MAX_NEG_LLD_STR     "-9223372036854775808"
#define MAX_DIV_LLU         10000000000000000000ull
#define MAX_DIV_LLD         1000000000000000000ll

#define is_digit(ch) ((ch) >= '0' && (ch) <= '9')

#define LLD(x) ((long long)(x))
#define LLU(x) ((unsigned long long)(x))
#define DIV_LLD(a, b) (LLD(a) / LLD(b))
#define DIV_LLU(a, b) (LLU(a) / LLU(b))

#define SIZE_no  0
#define SIZE_l   1
#define SIZE_hh  2
#define SIZE_h   3
#define SIZE_ll  4
#define SIZE_j   5
#define SIZE_z   6
#define SIZE_t   7
#define SIZE_L   8

char to_char_16(int i, int uppercase) {
  if (i >= 10)
    return i + (1 - uppercase) * ('a' - 'A') + 'A' - 10;
  return i + '0';
}

char to_char_8_10(int i, int base) {
  if (i >= base)
    return '#';
  return i + '0';
}

unsigned int optimized _vsprintf(char *dst, const char *fmt, __builtin_va_list args)
{
  unsigned long long arg, divider;
  char type;
  int wordlen;
  int padding;
  int space_padding;
  int maxbits;
  int leading_zeroes;
  int nibble;
  int rightmost_nibble;
  int is_neg_sign;
  int base;
  int flags_left_align;
  int flags_plus;
  int flags_space;
  int flags_zero;
  int width;
  int precision;
  int size;
  const char *s_src;

  char *orig;
  orig = dst;

  while(*fmt) {
    if (*fmt != '%') {
      *dst++ = *fmt++;
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      *dst++ = *fmt++;
      continue;
    }

    // Handle flags
    flags_zero = 0;
    if (*fmt == '0') {
      flags_zero = 1;
      fmt++;
    }

    flags_left_align = 0;
    if (*fmt == '-') {
      flags_left_align = 1;
      fmt++;
    }

    flags_plus = 0;
    if (*fmt == '+') {
      flags_plus = 1;
      fmt++;
    }

    flags_space = 0;
    if (*fmt == ' ') {
      flags_space = 1;
      fmt++;
    }

    // Handle width
    width = -1;
    if (*fmt == '*') {
      // width in next argument
      width = __builtin_va_arg(args, int);
      fmt++;
    } else if (is_digit(*fmt)) {
      // width in fmt as decimal
      width = 0;
      while(is_digit(*fmt))
        width = width * 10 + (*fmt++ - '0');
    }

    // Handle precision
    precision = -1;
    if (*fmt == '.') {
      fmt++;
      if (*fmt == '*') {
        precision = __builtin_va_arg(args, int);
        fmt++;
      } else { 
        precision = 0;
        if (!is_digit(*fmt))
          return -1;
        while(is_digit(*fmt))
          precision = precision * 10 + (*fmt++ - '0');
      }
    }
    
    // Handle size
    switch(*fmt) {
      case 'l': size = SIZE_l; break;
      case 'h': size = SIZE_h; break;
      case 'j': size = SIZE_j; break;
      case 'z': size = SIZE_z; break;
      case 't': size = SIZE_t; break;
      case 'L': size = SIZE_L; break;
      default:  size = SIZE_no; fmt--; break;
    }
    fmt++;
    if (size == SIZE_l && *fmt == 'l') {
      size = SIZE_ll;
      fmt++;
    }
    else if (size == SIZE_h && *fmt == 'h') {
      size = SIZE_hh;
      fmt++;
    }
    else if (*fmt == 'p' || *fmt == 's') {
      size = SIZE_ll;
    }

    // Fetch argument based on it's size
    switch(size) {
      case SIZE_z:   arg = __builtin_va_arg(args, size_t)   ; break;
      case SIZE_l:   arg = __builtin_va_arg(args, long)     ; break;
      case SIZE_ll:  arg = __builtin_va_arg(args, long long); break;
      default:       arg = __builtin_va_arg(args, int)      ; break;
    }

    // Handle type
    type = *fmt++;
    switch(type) {
      case 'o': base = 8;  goto _sprintf_number;
      case 'd': 
      case 'i': 
      case 'u': base = 10; goto _sprintf_number;
      case 'x':
      case 'X': base = 16; goto _sprintf_number;
      case 'p': goto _sprintf_ptr;
      case 'a':
      case 'A': goto _sprintf_float_hex;
      case 'e':
      case 'E': goto _sprintf_float_exp;
      case 'f':
      case 'F':
      case 'g':
      case 'G': goto _sprintf_float;
      case 'c': goto _sprintf_char;
      case 's':
      case 'S': goto _sprintf_string;
      default: goto _sprintf_loop_end;
    }
_sprintf_char:
    if (_isprint((char)arg))
      *dst++ = (char)arg;
    else
      *dst++ = '.';
    goto _sprintf_loop_end;

_sprintf_ptr:
    base = 16;
    *dst++ = '0'; *dst++ = 'x';
    width = 16;
    type = 'x';
_sprintf_number:
    is_neg_sign = 0;
    padding = 0;
    if (type == 'x' || type == 'X') {
      if (size == SIZE_ll) {
        leading_zeroes = __builtin_clzll(arg);
        maxbits = sizeof(long long) * 8;
      }
      else if (size == SIZE_l) {
        leading_zeroes = __builtin_clzl((long)arg);
        maxbits = sizeof(long) * 8;
      }
      else if (size == SIZE_no) {
        leading_zeroes = __builtin_clz((int)arg);
        maxbits = sizeof(int) * 8;
      }
      else {
        maxbits = 0;
        leading_zeroes = 0;
        *dst++ = '<';
        *dst++ = '>';
      }

      // padding
      rightmost_nibble = (maxbits - leading_zeroes - 1) / 4;
      padding = max(max(width, precision) - (rightmost_nibble + 1), 0);

      space_padding = padding;
      if (precision != -1) {
        precision = max(precision - (rightmost_nibble + 1), 0);
        space_padding = padding - precision;
      }
      else if (flags_zero)
        space_padding = 0;

      memset(dst, ' ', space_padding);
      dst += space_padding;
      padding -= space_padding;

      memset(dst, '0', padding);
      dst += padding;

      for (;rightmost_nibble >= 0; rightmost_nibble--) {
        nibble = (int)((LLU(arg) >> (rightmost_nibble * 4)) & 0xf);
        *dst++ = to_char_16(nibble, /* uppercase */ type < 'a');
      }

      goto _sprintf_loop_end;
    }

    divider = 1;
    if (type == 'd') {
      if (size == SIZE_ll && (long long)arg == MAX_NEG_LLD) {
        arg = LLU(MAX_NEG_LLD_STR);
        goto _sprintf_string;
      }

      else if (size == SIZE_ll && (long long)arg < 0ll) {
        is_neg_sign = 1;
        arg = ((long long)arg) * -1ll;
      }

      else if (size == SIZE_l && (long)arg < 0l) {
        is_neg_sign = 1;
        arg = ((long)arg) * -1l;
      }
      else if (size == SIZE_no && (int)arg < 0) {
        is_neg_sign = 1;
        arg = (unsigned int)(arg * -1);
      }

      wordlen = 1;
      while(divider != MAX_DIV_LLD && DIV_LLD(arg, divider * 10)) {
        divider *= 10;
        wordlen++;
      }

      if (is_neg_sign || flags_plus || flags_space)
        wordlen++;

      padding = max(width - wordlen, 0);
      space_padding = max(padding - max(max(precision, 0) - wordlen, 0), 0);
      if (flags_zero && precision == -1)
        space_padding = 0;

      memset(dst, ' ', space_padding);
      dst += space_padding;
      padding -= space_padding;

      if (is_neg_sign)
        *dst++ = '-';
      else if (flags_plus)
        *dst++ = '+';
      else if (flags_space)
        *dst++ = ' ';

      memset(dst, '0', padding);
      dst += padding;

      while(divider) {
        *dst++ = to_char_8_10(DIV_LLD(arg, divider), base);
        arg = arg % divider;
        divider /= base;
      }
    }
    else if (type == 'u') {
      if (sizeof(long) == sizeof(int) && size == SIZE_l)
        arg &= 0xffffffff;
      else if (size == SIZE_no)
        arg &= 0xffffffff;

      wordlen = 1;
      while(divider != MAX_DIV_LLU && DIV_LLU(arg, divider * 10)) {
        divider *= 10;
        wordlen++;
      }

      if (flags_plus || flags_space)
        wordlen++;

      padding = max(width - wordlen, 0);
      space_padding = max(padding - max(max(precision, 0) - wordlen, 0), 0);
      if (flags_zero && precision == -1)
        space_padding = 0;

      memset(dst, ' ', space_padding);
      dst += space_padding;
      padding -= space_padding;

      if (is_neg_sign)
        *dst++ = '-';
      else if (flags_plus)
        *dst++ = '+';
      else if (flags_space)
        *dst++ = ' ';

      memset(dst, '0', padding);
      dst += padding;

      while(divider) {
        *dst++ = to_char_8_10(DIV_LLU(arg, divider), base);
        arg = arg % divider;
        divider /= base;
      }
    }
    goto _sprintf_loop_end;
_sprintf_float:
    goto _sprintf_loop_end;
_sprintf_float_hex:
    goto _sprintf_loop_end;
_sprintf_float_exp:
    goto _sprintf_loop_end;
_sprintf_string:
    s_src = (const char *)arg;
    while(*s_src) {
      *dst++ = *s_src++;
    }
    goto _sprintf_loop_end;
_sprintf_loop_end:;
  }
  *dst = 0;
  return dst - orig;
}

unsigned int _sprintf(char *dst, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return _vsprintf(dst, fmt, args);
}


