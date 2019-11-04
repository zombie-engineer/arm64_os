#include <stringlib.h>

long long int strtoll(const char *str, char **endptr, int basis)
{
  // ptr - for forward walk
  // bptr - for backward walk
  const unsigned char *ptr, *bptr, *eptr;
  unsigned char c;
  char has_negative_sign;
  long long int result;
  unsigned long long order;

  eptr = (const unsigned char *)str;
  ptr = (const unsigned char *)str;
  result = 0;
  has_negative_sign = 0;

  // skip spaces
  SKIP_WHITESPACES(ptr);

  // check sign
  if (*ptr == '+')
    ptr++;
  if (*ptr == '-') {
    has_negative_sign = 1;
    ptr++;
  }

  // detect basis
  if (basis == 0) {
    if (ptr[0] == '0') {
      if (ptr[1] == 'x' || ptr[1] == 'X')
        basis = 16;
      else {
        basis = 2;
      }
    }
    else
      basis = 10;
  }

  // skip prefix
  if (basis == 16) {
    if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
      ptr += 2;
    else {
      goto out;
    }
  }

  switch(basis) {
    case 2:
      order = 0;
      // forward walk
      for(bptr = ptr; *bptr; bptr++) {
        if ((unsigned char)(*bptr - '0') > 1)
          break;
      }
      eptr = bptr;

      // backward walk
      for(bptr--; bptr >= ptr; bptr--) {
        if (*bptr == '1')
          result += 1 << order;
        order++;
      }
      break;
    case 10:
      order = 1;
      // forward walk
      for(bptr = ptr; *bptr; bptr++) {
        if ((unsigned char)(*bptr - '0') > 9)
          break;
      }
      eptr = bptr;

      // backward walk
      for(bptr--; bptr >= ptr; bptr--) {
        result += order * (*bptr - '0');
        order *= basis;
      }
      break;
    case 16:
      order = 1;
      // forward walk
      for(bptr = ptr; *bptr; bptr++) {
        c = *bptr;
        if ((unsigned char)(c -'0') > 9 
            && ((unsigned char)(c - 'a') > 5) 
            && ((unsigned char)(c - 'A') > 5))
          break;
      }
      eptr = bptr;

      // backward walk
      for(bptr--; bptr >= ptr; bptr--) {
        c = *bptr;
        if (c >= 'a')
          c -= ('a' - 'A');
        if (c - '0' <= 9) {
          result += order * (c - '0');
          order *= basis;
        }
        else if (c - 'A' <= 6) {
          result += order * (10 + c - 'A');
          order *= basis;
        }
        else
          break;
      }
      break;
    default:
      break;
  }
out:
  if (endptr)
    *endptr = (char*)eptr;
  if (has_negative_sign)
    result *= -1;
  return result;
}

