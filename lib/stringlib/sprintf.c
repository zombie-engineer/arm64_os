#include <stringlib.h>
#include <limits.h>
#include <common.h>
#include <compiler.h>

#define MAX_NEG_LLD         0x8000000000000000
#define MAX_NEG_LLD_STR     "-9223372036854775808"
#define MAX_DIV_LLU         10000000000000000000ull
#define MAX_DIV_LLD         1000000000000000000ll
#define DST_LEN_UNLIMITED 0xffffffff

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


struct printf_ctx {
  const char *fmt;
  char *dst;
  char *dst_virt;
  char *dst_end;
  int flag_plus;
  int flag_space;
  int flag_zero;
  int flag_left_align;

  int width;
  int precision;
  int size;
  char type;

  unsigned long long arg;
};

static inline void fmt_appendc(struct printf_ctx *c, char ch)
{
  if (c->dst < c->dst_end)
    *c->dst++ = ch;
  c->dst_virt++;
}

static inline void fmt_appends(struct printf_ctx *c, const char *str)
{
  char ch = *str++;
  while(ch) {
    fmt_appendc(c, ch);
    ch = *str++;
  }
}


static inline char to_char_16(int i, int uppercase) {
  if (i >= 10)
    return i + (1 - uppercase) * ('a' - 'A') + 'A' - 10;
  return i + '0';
}

static inline char to_char_8_10(int i, int base) {
  if (i >= base)
    return '#';
  return i + '0';
}

#define APPEND(c) { if (c->dst < end) *(dst++) = c; counter++; }

#ifdef TEST_STRING
#include <string.h>
#endif

#define MEMSET(val, len) \
  {\
    int memset_len = min(len, c->dst_end - c->dst);\
    memset(c->dst, val, memset_len);\
    c->dst += memset_len;\
    c->dst_virt += len;\
  }

extern const char *__mem_find_any_byte_of_2(const char *start_addr, const char *end_addr, uint64_t bytearray);

const char *__mem_find_any_byte_of_2(const char *start_addr, const char *end_addr, uint64_t bytearray)
{
  const char *p = start_addr;
  char a = bytearray & 0xff;
  char b = (bytearray >> 8) & 0xff;
  char t;
  while(p < end_addr) {
    t = *p;
    if (t == a || t == b)
      break;
    ++p;
  }
  return p;
}

static inline const char *__find_special_or_null(const char *pos, const char *end)
{
  while(1) {
    pos = __mem_find_any_byte_of_2(pos, end, ('%' << 8) | ('\0' << 0));
    if (*pos && *(pos+1) != '%')
      break;
    if (!*pos)
      break;
    pos += 1;
  }
  return pos;
}

#define fmt_fetch_special() special = *(c->fmt++)
  
#define fmt_handle_flag(fname, fchar) \
    c->flag_ ## fname = 0;\
    if (special == '0') {\
      c->flag_ ## fname = 1;\
      fmt_fetch_special();\
    }

#define fmt_handle_star_or_digit(digit_var, args)\
    digit_var = -1;\
    if (special == '*') {\
      /* digit in next argument */\
      digit_var = __builtin_va_arg(args, int);\
      fmt_fetch_special();\
    } else if (is_digit(special)) {\
      /* digit in fmt as decimal */\
      digit_var = 0;\
      do {\
        digit_var = digit_var * 10 + (special - '0');\
      fmt_fetch_special();\
      } while(is_digit(special));\
    }

#define fmt_handle_size() \
    switch(special) {\
      case 'c': c->fmt--;\
      case 'l': c->size = SIZE_l; break;\
      case 'h': c->size = SIZE_h; break;\
      case 'j': c->size = SIZE_j; break;\
      case 'z': c->size = SIZE_z; break;\
      case 't': c->size = SIZE_t; break;\
      case 'L': c->size = SIZE_L; break;\
      default:  c->size = SIZE_no; c->fmt--; break;\
    }\
    fmt_fetch_special();\
    if (c->size == SIZE_l && special == 'l') {\
      c->size = SIZE_ll;\
      fmt_fetch_special();\
    }\
    else if (c->size == SIZE_h && special == 'h') {\
      c->size = SIZE_hh;\
      fmt_fetch_special();\
    }\
    else if (special == 'p' || special == 's') \
      c->size = SIZE_ll

#define arg_fetch(args)\
    switch(c->size) {\
      case SIZE_z:   c->arg = __builtin_va_arg(args, size_t)   ; break;\
      case SIZE_l:   c->arg = __builtin_va_arg(args, long)     ; break;\
      case SIZE_ll:  c->arg = __builtin_va_arg(args, long long); break;\
      default:       c->arg = __builtin_va_arg(args, int)      ; break;\
    }

static inline void print_number_8(struct printf_ctx *c)
{
//  unsigned long long divider = 1;
//  int space_padding, padding;
//
//  if (size == SIZE_ll && (long long)arg == MAX_NEG_LLD) {
//    arg = LLU(MAX_NEG_LLD_STR);
//    return;
//  } else if (size == SIZE_ll && (long long)arg < 0ll) {
//    is_neg_sign = 1;
//    arg = ((long long)arg) * -1ll;
//  } else if (size == SIZE_l && (long)arg < 0l) {
//    is_neg_sign = 1;
//    arg = ((long)arg) * -1l;
//  } else if (size == SIZE_no && (int)arg < 0) {
//    is_neg_sign = 1;
//    arg = (unsigned int)(arg * -1);
//  }
//
//  wordlen = 1;
//  while(divider != MAX_DIV_LLD && DIV_LLD(arg, divider * 10)) {
//    divider *= 10;
//    wordlen++;
//  }
//
//  if (is_neg_sign || flags_plus || flags_space)
//    wordlen++;
//
//  padding = max(width - wordlen, 0);
//  space_padding = max(padding - max(max(precision, 0) - wordlen, 0), 0);
//  if (flags_zero && precision == -1)
//    space_padding = 0;
//
//  MEMSET(' ', space_padding);
//  padding -= space_padding;
//
//  if (is_neg_sign) {
//    APPEND('-');
//  }
//  else if (flags_plus) {
//    APPEND('+');
//  }
//  else if (flags_space) {
//    APPEND(' ');
//  }
//
//  MEMSET('0', padding);
//
//  while(divider) {
//    APPEND(to_char_8_10(DIV_LLD(arg, divider), base));
//    arg = arg % divider;
//    divider /= base;
//  }
}

static unsigned long long pot[] = {
  1,
  10,
  100,
  1000,
  10000,

  100000,
  1000000,
  10000000,
  100000000,
  1000000000,


  10000000000,
  100000000000,
  1000000000000,
  10000000000000,
  100000000000000,

  1000000000000000,
  10000000000000000,
  100000000000000000,
  1000000000000000000,
  10000000000000000000ull
};

static OPTIMIZED int closest_round_base10_number(unsigned long long val)
{
  int x;

  if (val >= pot[5]) {
    if (val >= pot[8])
      x = (val >= pot[9]) ? 9 : 8;
    else {
      if (val >= pot[6])
        x = (val >= pot[7]) ? 7 : 6;
      else
        x = 5;
    }
  }
  else {
    if (val >= pot[3])
      x = (val >= pot[4]) ? 4 : 3;
    else {
      if (val >= pot[1])
        x = (val >= pot[2]) ? 2 : 1;
      else
        x = 0;
    }
  }
  return x;
}

static OPTIMIZED int get_digit(unsigned long long num, unsigned long long powered_base, unsigned long long *out_round)
{
  int i;
  unsigned long long cmp = powered_base;
  for (i = 1; i < 10; ++i) {
    if (num < cmp) {
      *out_round = cmp - powered_base;
      return (i - 1);
    }
    cmp += powered_base;
  }
  *out_round = 0;
  return 0;
}

struct print_num_spec {
  int space_padding;
  int padding;
  int wordlen;
  int is_neg_sign;
};

static NOINLINE void __print_fn_number_10_u(struct printf_ctx *c)
{
  unsigned long long divider = 1;
  unsigned long long subtract_round;
  unsigned long long tmp_arg;
  struct print_num_spec s = { 0 };
  int tmp_num;
  char tmp;

  if (sizeof(long) == sizeof(int) && c->size == SIZE_l)
    c->arg &= 0xffffffff;
  else if (c->size == SIZE_no)
    c->arg &= 0xffffffff;

  s.wordlen = 1;
  while(divider != MAX_DIV_LLD && DIV_LLD(c->arg, divider * 10)) {
    divider *= 10;
    s.wordlen++;
  }

  s.wordlen = closest_round_base10_number(c->arg);
  s.wordlen += (c->flag_plus || c->flag_space) & 1;

  s.padding = max(c->width - s.wordlen, 0);
  s.space_padding = max(s.padding - max(max(c->precision, 0) - s.wordlen, 0), 0);

  if (c->flag_zero && c->precision == -1)
    s.space_padding = 0;

  MEMSET(' ', s.space_padding);
  s.padding -= s.space_padding;

  if (s.is_neg_sign)
    fmt_appendc(c, '-');
  else if (c->flag_plus)
    fmt_appendc(c, '+');
  else if (c->flag_space)
    fmt_appendc(c, ' ');

  MEMSET('0', s.padding);

  tmp_arg = c->arg;
  for (;s.wordlen >= 0; s.wordlen--) {
    tmp_num = get_digit(tmp_arg, pot[s.wordlen], &subtract_round);
    tmp = to_char_8_10(tmp_num, 10);
    fmt_appendc(c, tmp);
    tmp_arg -= subtract_round;
  }
}

static NOINLINE void __print_fn_number_10_d(struct printf_ctx *c)
{
  unsigned long long divider = 1;
  struct print_num_spec s = { 0 };
  char ch;

  if (c->size == SIZE_ll && (long long)c->arg == MAX_NEG_LLD) {
    c->arg = LLU(MAX_NEG_LLD_STR);
    return;
  } else if (c->size == SIZE_ll && (long long)c->arg < 0ll) {
    s.is_neg_sign = 1;
    c->arg = ((long long)c->arg) * -1ll;
  } else if (c->size == SIZE_l && (long)c->arg < 0l) {
    s.is_neg_sign = 1;
    c->arg = ((long)c->arg) * -1l;
  } else if (c->size == SIZE_no && (int)c->arg < 0) {
    s.is_neg_sign = 1;
    c->arg = (unsigned int)(c->arg * -1);
  }

  s.wordlen = 1;
  while(divider != MAX_DIV_LLD && DIV_LLD(c->arg, divider * 10)) {
    divider *= 10;
    s.wordlen++;
  }

  s.wordlen += (s.is_neg_sign | c->flag_plus | c->flag_space) & 1;

  s.padding = max(c->width - s.wordlen, 0);
  s.space_padding = max(s.padding - max(max(c->precision, 0) - s.wordlen, 0), 0);
  if (c->flag_zero && c->precision == -1)
    s.space_padding = 0;

  MEMSET(' ', s.space_padding);
  s.padding -= s.space_padding;

  if (s.is_neg_sign)
    fmt_appendc(c, '-');
  else if (c->flag_plus)
    fmt_appendc(c, '+');
  else if (c->flag_space)
    fmt_appendc(c, ' ');

  MEMSET('0', s.padding);

  while(divider) {
    ch = to_char_8_10(DIV_LLD(c->arg, divider), 10);
    fmt_appendc(c, ch);
    c->arg = c->arg % divider;
    divider /= 10;
  }
}

static void NOINLINE  __print_fn_number_16(struct printf_ctx *c)
{
  int nibble;
  int leading_zeroes = 0;
  int maxbits = 0; 
  int rightmost_nibble;
  int space_padding, padding;
  char tmp;

  switch(c->size) {
  case SIZE_ll: leading_zeroes = __builtin_clzll(c->arg)     ; maxbits = sizeof(long long) * 8; break;
  case SIZE_l : leading_zeroes = __builtin_clzl((long)c->arg); maxbits = sizeof(long)      * 8; break;
  case SIZE_no: leading_zeroes = __builtin_clz((int)c->arg)  ; maxbits = sizeof(int)       * 8; break;
  default: fmt_appends(c, "<>")                                                               ; break;
  }

  // padding
  rightmost_nibble = (maxbits - leading_zeroes - 1) / 4;
  padding = max(max(c->width, c->precision) - (rightmost_nibble + 1), 0);
  space_padding = padding;
  if (c->precision != -1) {
    c->precision = max(c->precision - (rightmost_nibble + 1), 0);
    space_padding = padding - c->precision;
  }
  else if (c->flag_zero)
    space_padding = 0;

  MEMSET(' ', space_padding);
  padding -= space_padding;

  MEMSET('0', padding);

  for (;rightmost_nibble >= 0; rightmost_nibble--) {
    nibble = (int)((LLU(c->arg) >> (rightmost_nibble * 4)) & 0xf);
    tmp = to_char_16(nibble, /* uppercase */ c->type < 'a');
    fmt_appendc(c, tmp);
  }
}

static inline char *__printf_strlcpy(char *dst, const char **src, int n)
{
  while(**src && n) {
    *(dst++) = *((*src)++);
    n++;
  }
  return dst;
}

static void NOINLINE __print_fn_string(struct printf_ctx *c)
{
  int n;
  const char *src = (const char *)c->arg;
  c->dst = __printf_strlcpy(c->dst, &src, c->dst_end - c->dst);

  while(*src) {
    c->dst_virt++;
    src++;
  }
}

static void NOINLINE __print_fn_char(struct printf_ctx *c)
{
  char ch = (char)c->arg;
  if (!_isprint(ch))
    ch = '.';
  if (c->dst < c->dst_end)
    *(c->dst++) = ch;
}

static void NOINLINE __print_fn_ignore(struct printf_ctx *c) {}
static void NOINLINE __print_fn_float_hex(struct printf_ctx *c) { }
static void NOINLINE __print_fn_float_exp(struct printf_ctx *c) { }
static void NOINLINE __print_fn_float(struct printf_ctx *c) { }
static void NOINLINE __print_fn_number_8(struct printf_ctx *c) { }
static void NOINLINE __print_fn_address(struct printf_ctx *c) { }

typedef void (*__print_fn_t)(struct printf_ctx *);
ALIGNED(64) __print_fn_t __printf_fn_map[] = {
#define DECL(ch, fn) [ch - 'a'] = fn
#define IGNR(ch) [ch - 'a'] = __print_fn_ignore

  DECL('a', __print_fn_float_hex),
  IGNR('b'),
  DECL('c', __print_fn_char),
  DECL('d', __print_fn_number_10_d),
  DECL('e', __print_fn_float_exp),
  DECL('f', __print_fn_float),
  DECL('g', __print_fn_float),    
  IGNR('h'),
  DECL('i', __print_fn_number_10_d),
  IGNR('j'),
  IGNR('k'),
  IGNR('l'),
  IGNR('m'),
  IGNR('n'),
  DECL('o', __print_fn_number_8),
  DECL('p', __print_fn_address),
  IGNR('q'),
  IGNR('r'),
  DECL('s', __print_fn_string),
  IGNR('t'),
  DECL('u', __print_fn_number_10_u),
  IGNR('v'),
  IGNR('w'),
  DECL('x', __print_fn_number_16),
  DECL('y', __print_fn_number_16),
  DECL('z', __print_fn_number_16)
};

void fmt_print_type(struct printf_ctx *c)
{
  /* To lower type 'A'(0x41) -> 'a'(0x61), because 0x61=0x41|0x20 
   * but we keep type in original.
   */
  int gentype = c->type | 0x20;
  if (gentype >= 'a' && gentype <= 'x')
    __printf_fn_map[gentype - 'a'](c);
}

/* 
 * Returns NULL if reached zero termination or any format specifier,
 * after % in case it was not %%, ex 'string%p' returns 'p'
 * c->fmt points at position where return value wat taken.
 */
static inline char fmt_skip_to_special(struct printf_ctx *c)
{
  char special = 0;
  const char *fmt_pos;
  fmt_pos = __find_special_or_null(c->fmt, c->fmt + (c->dst_end - c->dst));
  memcpy(c->dst, c->fmt, fmt_pos - c->fmt);
  c->dst += fmt_pos - c->fmt;
  c->fmt = fmt_pos;
  
  special = *c->fmt;
  if (special == '%')
    special = *(++(c->fmt));

  return special;
}

static inline int fmt_parse_token(struct printf_ctx *c, __builtin_va_list args)
{
  char special;
  special = fmt_skip_to_special(c);
  /* zero-termination check */
  if (!special)
    return 1;

  fmt_handle_flag(zero      , '0');
  fmt_handle_flag(left_align, '-');
  fmt_handle_flag(plus      , '+');
  fmt_handle_flag(space     , ' ');

  // TODO implement left-align flag '-'
  // This is to disable compiler warning
  if (c->flag_left_align);

  fmt_handle_star_or_digit(c->width, args);
  fmt_handle_star_or_digit(c->precision, args);
  fmt_handle_size();
  arg_fetch(args);

  fmt_fetch_special();
  c->type = special;
  return 0;
}

int /*optimized*/ _vsnprintf(char *dst, size_t dst_len, const char *fmt, __builtin_va_list args)
{
  struct printf_ctx ctx = { 0 };
  struct printf_ctx *c = &ctx;
  c->fmt = fmt;
  c->dst = dst;
  c->dst_end = c->dst + dst_len;
  c->dst_virt = c->dst;

  while(*c->fmt && (c->dst < c->dst_end)) {
    if (fmt_parse_token(c, args))
      break;
    fmt_print_type(c);
  }

  if (c->dst < c->dst_end)
    *c->dst = 0; 

  return c->dst - dst;
}

int /*optimized*/ _vsprintf(char *dst, const char *fmt, __builtin_va_list args)
{
  return _vsnprintf(dst, 4096, fmt, args);
}


int _sprintf(char *dst, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return _vsprintf(dst, fmt, args);
}

int _snprintf(char *dst, size_t dst_size, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return _vsnprintf(dst, dst_size, fmt, args);
}
