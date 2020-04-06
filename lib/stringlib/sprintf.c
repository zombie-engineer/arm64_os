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

enum {
  FMT_SIZE_NO      = 0x00,
  FMT_SIZE_LONG    = 0x01,
  FMT_SIZE_LLONG   = 0x02,
  FMT_SIZE_INTMAX  = 0x04,
  FMT_SIZE_SIZE_T  = 0x08,
  FMT_SIZE_T       = 0x10,
  FMT_SIZE_INT64   = 0x20,
  FMT_SIZE_HALF    = 0x40,
  FMT_SIZE_HHALF   = 0x80,
  FMT_SIZE_PTRDIFF = 0x100
};

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

#define DST_FREE_BYTES() (c->dst_end - c->dst)

#define DST_APPEND_C(ch)\
  do {\
    if (c->dst < c->dst_end)\
      *c->dst++ = ch;\
    c->dst_virt++;\
  } while(0)

#define DST_APPEND_S(s)\
  do {\
    const char *__s = s;\
    char ch = *__s++;\
    while(ch) {\
      DST_APPEND_C(ch);\
      ch = *__s++;\
    }\
  } while(0)

#define DST_MEMCPY(src, len) \
  {\
    int op_len = min(len, DST_FREE_BYTES());\
    memcpy(c->dst, src, op_len);\
    c->dst += op_len;\
    c->dst_virt += op_len;\
  }

#define DST_MEMSET(val, len) \
  {\
    int op_len = min(len, DST_FREE_BYTES());\
    memset(c->dst, val, op_len);\
    c->dst += op_len;\
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

#define fmt_fetch_special() special = *(++c->fmt)
  
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
      case 'l': c->size = FMT_SIZE_LONG; break;\
      case 'h': c->size = FMT_SIZE_HALF; break;\
      case 'j': c->size = FMT_SIZE_INTMAX; break;\
      case 'z': c->size = FMT_SIZE_SIZE_T; break;\
      case 't': c->size = FMT_SIZE_PTRDIFF; break;\
      case 'L': c->size = FMT_SIZE_INT64; break;\
      default:  c->size = FMT_SIZE_NO; c->fmt--; break;\
    }\
    fmt_fetch_special();\
    if (c->size == FMT_SIZE_LONG && special == 'l') {\
      c->size = FMT_SIZE_LLONG;\
      fmt_fetch_special();\
    }\
    else if (c->size == FMT_SIZE_HALF && special == 'h') {\
      c->size = FMT_SIZE_HHALF;\
      fmt_fetch_special();\
    }\
    else if (special == 'p' || special == 's') \
      c->size = FMT_SIZE_LLONG

#define arg_fetch(args)\
    switch(c->size) {\
      case FMT_SIZE_SIZE_T:   c->arg = __builtin_va_arg(*args, size_t)   ; break;\
      case FMT_SIZE_LONG:   c->arg = __builtin_va_arg(*args, long)     ; break;\
      case FMT_SIZE_LLONG:  c->arg = __builtin_va_arg(*args, long long); break;\
      default:       c->arg = __builtin_va_arg(*args, int)      ; break;\
    }

static inline void print_number_8(struct printf_ctx *c)
{
//  unsigned long long divider = 1;
//  int space_padding, padding;
//
//  if (size == FMT_SIZE_LLONG && (long long)arg == MAX_NEG_LLD) {
//    arg = LLU(MAX_NEG_LLD_STR);
//    return;
//  } else if (size == FMT_SIZE_LLONG && (long long)arg < 0ll) {
//    is_neg_sign = 1;
//    arg = ((long long)arg) * -1ll;
//  } else if (size == FMT_SIZE_LONG && (long)arg < 0l) {
//    is_neg_sign = 1;
//    arg = ((long)arg) * -1l;
//  } else if (size == FMT_SIZE_NO && (int)arg < 0) {
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
//  DST_MEMSET(' ', space_padding);
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
//  DST_MEMSET('0', padding);
//
//  while(divider) {
//    APPEND(to_char_8_10(DIV_LLD(arg, divider), base));
//    arg = arg % divider;
//    divider /= base;
//  }
}

#define POT_0 1
#define POT_1 10
#define POT_2 100
#define POT_3 1000
#define POT_4 10000
#define POT_5 100000
#define POT_6 1000000
#define POT_7 10000000
#define POT_8 100000000
#define POT_9 1000000000

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

int OPTIMIZED closest_round_base10_div(uint64_t val)
{
  int wordlen = 0;
  uint64_t divider = 1;
  uint64_t next_divider;
  for (;;) {
    if (divider == MAX_DIV_LLD)
      break;
    next_divider = divider * 10;
    if (!DIV_LLD(val, next_divider))
      break;
    divider = next_divider;
    wordlen++;
  }
  return wordlen;
}

int OPTIMIZED closest_round_base10_number(uint64_t val)
{
  const int max_idx = ARRAY_SIZE(pot); /* 20 */
  int base = 0;
  int delta = max_idx / 2;
  int pivot;

  do {
    pivot = base + delta;
    delta /= 2;
    if (val >= pot[pivot])
      base = pivot;
  } while(base != pivot || delta);

  if (pivot == 0)
    return 0;

  if (val >= pot[pivot]) {
    if (pivot <= ARRAY_SIZE(pot) - 2 && val >= pot[pivot + 1])
        return pivot + 1;
    return pivot;
  }

  return pivot - 1;
}

// 0: 
// d = 10->5, b = 0, p = 10, >= false
// d = 5 ->2, b = 0, p = 5 , >= false
// d = 2 ->1, b = 0, p = 2 , >= false
// d = 1 ->0, b = 0, p = 1 , >= false
// d = 0    , b = 1, p = 1 , >= false, p[0] = 1 -> 0

// 10: 
// d = 10->5, b = 0, p = 10, >= false
// d = 5 ->2, b = 0, p = 5 , >= false
// d = 2 ->1, b = 0, p = 2 , >= false
// d = 1 ->0, b = 0, p = 1 , >= true , p[1] = 10
// d = 0    , b = 1, p = 1,  >= true , p[1] = 10 -> 1


// 101: 
// d = 10->5, b = 0, p = 10, >= false
// d = 5 ->2, b = 0, p = 5 , >= false
// d = 2 ->1, b = 0, p = 2 , >= true , p[2] = 100
// d = 1 ->0, b = 2, p = 3 , >= false, p[3] = 1000
// d = 0,     b = 3, p = 3 , >= false, p[3] = 1000 -> 3

// 101: 
// d = 10->5, b = 0, p = 10, >= false
// d = 5 ->2, b = 0, p = 5 , >= false
// d = 2 ->1, b = 0, p = 2 , >= true , p[2] = 100
// d = 1 ->0, b = 2, p = 3 , >= false, p[3] = 1000
// d = 0
// 0 or 1 - 100% definately 0

// big:
// d = 10, b = 0 , p = 10, >= true
// d = 5 , b = 10, p = 15, >= true
// d = 2 , b = 15, p = 17, >= true
// d = 1 , b = 15, p = 18, >= true
// d = 1 , b = 15, p = 18, >= true
// d = 0 , b = 18, p = 18, >= true
// 18 or 19 ?
static OPTIMIZED int get_digit(unsigned long long num, unsigned long long powered_base, unsigned long long *out_round)
{
  int i;
  unsigned long long cmp = powered_base;
  for (i = 1; i < 11; ++i) {
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
  unsigned long long subtract_round;
  unsigned long long tmp_arg;
  struct print_num_spec s = { 0 };
  int tmp_num;
  char tmp;

  if (sizeof(long) == sizeof(int) && c->size == FMT_SIZE_LONG)
    c->arg &= 0xffffffff;
  else if (c->size == FMT_SIZE_NO)
    c->arg &= 0xffffffff;

  s.wordlen = 1;

#ifdef NODIV
  s.wordlen = closest_round_base10_nodiv(c->arg);
#else
  s.wordlen = closest_round_base10_div(c->arg);
#endif

  s.wordlen += c->flag_plus | c->flag_space;

  s.padding = max(c->width - s.wordlen, 0);
  s.space_padding = max(s.padding - max(max(c->precision, 0) - s.wordlen, 0), 0);

  if (c->flag_zero && c->precision == -1)
    s.space_padding = 0;

  DST_MEMSET(' ', s.space_padding);
  s.padding -= s.space_padding;

  if (s.is_neg_sign)
    DST_APPEND_C('-');
  else if (c->flag_plus)
    DST_APPEND_C('+');
  else if (c->flag_space)
    DST_APPEND_C(' ');

  DST_MEMSET('0', s.padding);

  tmp_arg = c->arg;
  for (;s.wordlen >= 0; s.wordlen--) {
    tmp_num = get_digit(tmp_arg, pot[s.wordlen], &subtract_round);
    tmp = to_char_8_10(tmp_num, 10);
    DST_APPEND_C(tmp);
    tmp_arg -= subtract_round;
  }
}

static NOINLINE void __print_fn_number_10_d(struct printf_ctx *c)
{
  unsigned long long subtract_round;
  unsigned long long tmp_arg;
  unsigned long long divider = 1;
  int tmp_num;
  char tmp;
  struct print_num_spec s = { 0 };
  char ch;
  int max_bit_pos = 31;
  uint64_t type_mask;
  // TODO: Support more types 
  if (c->size & (FMT_SIZE_LLONG |
#if (__LP64__ == 1)
        FMT_SIZE_LONG |
#endif
        FMT_SIZE_INT64)) {
     max_bit_pos = 63;
  }

  /* zero-out top part of arg for smaller types */
  // type_mask = (1<<(max_bit_pos + 1)) - 1; does not work for 64
  switch(max_bit_pos) {
    case  7: type_mask = 0xff;               break;
    case 15: type_mask = 0xffff;             break;
    case 31: type_mask = 0xffffffff;         break;
    default: type_mask = 0xffffffffffffffff; break;
  }

  c->arg &= type_mask;

  if (c->arg & (1ll << max_bit_pos)) {
    c->arg = (~c->arg + 1) & type_mask;
    s.is_neg_sign = 1;
  }

  s.wordlen = 1;
  while(divider != MAX_DIV_LLD && DIV_LLD(c->arg, divider * 10)) {
    divider *= 10;
    s.wordlen++;
  }

  s.wordlen += s.is_neg_sign | c->flag_plus | c->flag_space;

  s.padding = max(c->width - s.wordlen, 0);
  s.space_padding = max(s.padding - max(max(c->precision, 0) - s.wordlen, 0), 0);
  if (c->flag_zero && c->precision == -1)
    s.space_padding = 0;

  DST_MEMSET(' ', s.space_padding);
  s.padding -= s.space_padding;

  if (s.is_neg_sign) {
    DST_APPEND_C('-');
    s.wordlen--;
  }
  else if (c->flag_plus) {
    DST_APPEND_C('+');
    s.wordlen--;
  }
  else if (c->flag_space) {
    DST_APPEND_C(' ');
    s.wordlen--;
  }

  DST_MEMSET('0', s.padding);

  tmp_arg = c->arg;
  for (;s.wordlen > 0; s.wordlen--) {
    tmp_num = get_digit(tmp_arg, pot[s.wordlen - 1], &subtract_round);
    tmp = to_char_8_10(tmp_num, 10);
    DST_APPEND_C(tmp);
    tmp_arg -= subtract_round;
  }
//  while(divider) {
//    ch = to_char_8_10(DIV_LLD(c->arg, divider), 10);
//    DST_APPEND_C(ch);
//    c->arg = c->arg % divider;
//    divider /= 10;
//  }
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
  case FMT_SIZE_LLONG: leading_zeroes = __builtin_clzll(c->arg)     ; maxbits = sizeof(long long) * 8; break;
  case FMT_SIZE_LONG : leading_zeroes = __builtin_clzl((long)c->arg); maxbits = sizeof(long)      * 8; break;
  case FMT_SIZE_NO   : leading_zeroes = __builtin_clz((int)c->arg)  ; maxbits = sizeof(int)       * 8; break;
  default: DST_APPEND_S("<>")                                                               ; break;
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

  DST_MEMSET(' ', space_padding);
  padding -= space_padding;

  DST_MEMSET('0', padding);

  for (;rightmost_nibble >= 0; rightmost_nibble--) {
    nibble = (int)((LLU(c->arg) >> (rightmost_nibble * 4)) & 0xf);
    tmp = to_char_16(nibble, /* uppercase */ c->type < 'a');
    DST_APPEND_C(tmp);
  }
}

static inline char *__printf_strlcpy(char *dst, const char *src, int n)
{
  const char *src_end = src + n;
  while(*src && src != src_end)
    *dst++ = *src++;
  return dst;
}

static void NOINLINE __print_fn_string(struct printf_ctx *c)
{
  int n;
  const char *src = (const char *)c->arg;
  c->dst = __printf_strlcpy(c->dst, src, c->dst_end - c->dst);

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

  fmt_pos = __find_special_or_null(c->fmt, c->fmt + 512);

  DST_MEMCPY(c->fmt, fmt_pos - c->fmt);
  c->fmt = fmt_pos;
  
  special = *c->fmt;
  if (special == '%')
    special = *(++(c->fmt));

  return special;
}

static inline int fmt_parse_token(struct printf_ctx *c, __builtin_va_list *args)
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

  fmt_handle_star_or_digit(c->width, *args);
  fmt_handle_star_or_digit(c->precision, *args);
  fmt_handle_size();

  // arg_fetch(args);
    switch(c->size) {
      case FMT_SIZE_SIZE_T: c->arg = __builtin_va_arg(*args, size_t)   ; break;
      case FMT_SIZE_LONG:   c->arg = __builtin_va_arg(*args, long)     ; break;
      case FMT_SIZE_LLONG:  c->arg = __builtin_va_arg(*args, long long); break;
      default:              c->arg = __builtin_va_arg(*args, int)      ; break;
    }
  c->type = special;
  c->fmt++;
  return 0;
}

int /*optimized*/ __vsnprintf(char *dst, size_t dst_len, const char *fmt, __builtin_va_list *args)
{
  struct printf_ctx ctx = { 0 };
  struct printf_ctx *c = &ctx;
  c->fmt = fmt;
  c->dst = dst;

  /*
   * dst_len is harder than you think. If dst_len is 0 - we don't write
   * anything, just calculate resulting string length.
   * if dst_len is greater than 0, 1 byte from this length is for
   * zero-termination. if dst_len == 1, the first byte in dst
   * will be written 0. So the actual string length decays to dst_len - 1.
   */
  c->dst_end = c->dst + dst_len;
  c->dst_virt = c->dst;

  while(*c->fmt && !fmt_parse_token(c, args))
    fmt_print_type(c);

  /*
   * Two possible outcomes are possible. 
   * - c->dst has reached end.
   * - c->dst haven't readed end.
   * In first - we still need to write zero-termination somewhere.
   * So we overwrite the end of the resulting string.
   * In second - we safely put zero-termination after the text,
   * because there is still space to do that.
   */
  if (dst_len) {
    if (c->dst == c->dst_end)
      *(c->dst - 1) = 0;
    else
      *c->dst = 0; 
  }

  return c->dst_virt - dst;
}

int /*optimized*/ _vsnprintf(char *dst, size_t dst_len, const char *fmt, __builtin_va_list args)
{
  return __vsnprintf(dst, dst_len, fmt, &args);
}

int /*optimized*/ _vsprintf(char *dst, const char *fmt, __builtin_va_list args)
{
  return __vsnprintf(dst, 4096, fmt, &args);
}

int _sprintf(char *dst, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return __vsnprintf(dst, 4096, fmt, &args);
}

int _snprintf(char *dst, size_t dst_size, const char *fmt, ...)
{
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return __vsnprintf(dst, dst_size, fmt, &args);
}
