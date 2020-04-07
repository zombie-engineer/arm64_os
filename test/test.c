#include <rectangle.h>
#include <ringbuf.h>
#include <stringlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ringbuf.h>

#define ASSERT_VALUE_EQ(A, B, desc, ...) \
  if ((A) != (B)) { \
    printf("assertion! values not equal %lld != %lld (" desc ")\n",\
      (long long)(A), (long long)(B), ## __VA_ARGS__);\
    exit(1); \
  }

#define ASSERT_MEM_EQ(ptr_A, ptr_B, N, desc, ...) \
  if (memcmp(ptr_A, ptr_B, N)) { \
    printf("assertion! memory regions not equal (" desc " )\n", ## __VA_ARGS__);\
    printf("ptr_A: %p: ", ptr_A); hexdump(ptr_A, N); putchar('\n');\
    printf("ptr_B: %p: ", ptr_B); hexdump(ptr_B, N); putchar('\n');\
    exit(1); \
  }

void hexdump(const char *buf, int sz)
{
  int i;
  putchar('\'');
  for (i = 0; i + 1 < sz; ++i)
    printf("%02x ", buf[i]);
  printf("%02x\'", buf[i]);
}

void test_rectangle(rect_t *r0, rect_t *r1, intersection_regions_t *r_exp, int retval_exp)
{
  intersection_regions_t r;
  intersection_region_t *rt0, *rt1;
  int retval;
  retval = get_intersection_regions(r0, r1, &r);
  ASSERT_VALUE_EQ(retval, retval_exp, "get_intersection_regions return value");
  if (!retval)
    return;
  int x, y;
  for (y = 0; y < 3; ++y) {
    for (x = 0; x < 3; ++x) {
      rt0 = &r.rgs[y * 3 + x];
      rt1 = &(r_exp->rgs[y * 3 + x]);
      ASSERT_VALUE_EQ(rt0->exists, rt1->exists, "get_intersection_regions exist");
      if (rt0->exists) {
        ASSERT_VALUE_EQ(rt0->r.x.offset, rt1->r.x.offset, "get_intersection_regions r.x.offset");
        ASSERT_VALUE_EQ(rt0->r.y.offset, rt1->r.y.offset, "get_intersection_regions r.y.offset");
        ASSERT_VALUE_EQ(rt0->r.x.size, rt1->r.x.size, "get_intersection_regions r.x.size");
        ASSERT_VALUE_EQ(rt0->r.y.size, rt1->r.y.size, "get_intersection_regions r.x.size");
      }
    }
  }
}

void set_iregion(intersection_region_t *r, int x, int y, unsigned int sx, unsigned int sy, char exists)
{
  r->r.x.offset = x;
  r->r.y.offset = y;
  r->r.x.size   = sx;
  r->r.y.size   = sy;
  r->exists = exists;
}

void fill_rect(rect_t *r, int x, int y, unsigned int sx, unsigned int sy) 
{ 
  r->x.offset = x; 
  r->y.offset = y; 
  r->x.size = sx;
  r->y.size = sy;
}

void run_cases_rectangle()
{
  rect_t r0;
  rect_t r1;

  intersection_regions_t r;
  intersection_regions_t *rs = &r;

  // perfect match
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1), 0, 0, 20, 20, 1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip left
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, -1, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(0, 1), -1,  0,  1, 20,  1);
  set_iregion(RGN(1, 1),  0,  0, 19, 20,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip top 
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, -1, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 0),  0,  -1, 20, 1,  1);
  set_iregion(RGN(1, 1),  0,  0, 20, 19,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip right 
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 19, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1), 19,  0,  1, 20,  1);
  set_iregion(RGN(2, 1), 20,  0, 19, 20,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip bottom
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 19, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1),  0,  19, 20, 1,  1);
  set_iregion(RGN(1, 2),  0,  20, 20, 19,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // out bounds
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, -20, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 20, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 20, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, -20, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);
}

void test_strtoll_single(const char *str, int base, long long int expected)
{
  char *endptr;
  long long int result;
  result = strtoll(str, &endptr, base);
  ASSERT_VALUE_EQ(result, expected, "strtoll");
}
void run_cases_strtol()
{
  test_strtoll_single("-0x1"         , 0, -0x1);
  test_strtoll_single("  -0x1"       , 0, -0x1);
  test_strtoll_single("  0X1"        , 0, 0x1);
  test_strtoll_single("  -0X1"       , 0, -0x1);
  test_strtoll_single("  1"          , 0, 1);
  test_strtoll_single("  -345"       , 0, -345);
  test_strtoll_single("1"            , 10, 1);
  test_strtoll_single(" 1"           , 10, 1);
  test_strtoll_single(" 1LL"         , 10, 1);
  test_strtoll_single(" \t\n0LL"     , 10, 0);
  test_strtoll_single("123"          , 10, 123);
  test_strtoll_single("0x123"        , 16, 0x123);
  test_strtoll_single("0x1"          , 16, 0x1);
  test_strtoll_single("-0x1"         , 16, -0x1);
  test_strtoll_single("0x03579abcdef", 16, 0x03579abcdefLL);
  test_strtoll_single("0xf3579abcdef", 16, 0xf3579abcdefLL);
}

void test_sprintf(const char *fmt, ...)
{
  int status;
  char actual[256];
  char expected[256];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  _vsprintf(actual, fmt, args);
  __builtin_va_start(args, fmt);
  vsprintf(expected, fmt, args);
  if (strcmp(actual, expected)) {
    printf("assertion! strings not equal: expected: '%s', got '%s'\n", expected, actual);
    exit(1);
  }
  printf("success: '%s'\n", actual);
}

void test_snprintf(int n, const char *fmt, ...)
{
  int status;
  int n1, n2;
  char buf1[256];
  char buf2[256];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  n1 = _vsnprintf(buf1, n, fmt, args);
  __builtin_va_start(args, fmt);
  n2 = vsnprintf(buf2, n, fmt, args);
  if (n1 != n2) {
    printf("assertion! vsnprintf ret values dont match: %d != %d\n", n1, n2);
    exit(1);
  }
  if (strcmp(buf1, buf2)) {
    printf("assertion! strings not equal: expected: '%s', got '%s'\n", buf1, buf2);
    exit(1);
  }
  printf("success: '%s'\n", buf1);
}

void run_cases_sprintf()
{
  test_sprintf("%p", 1234);
  test_sprintf("%16p", 1234);
  test_sprintf("%016p", 1234);
  test_sprintf("%d", 0);
  test_sprintf("%d", 1234);
  test_sprintf("%d", -1234);
  test_sprintf("%d", 0x7fffffff);
  test_sprintf("%d", 0x80000000);

  test_sprintf("%x", 1234);
  test_sprintf("%x", -1234);
  test_sprintf("%x", 0x7fffffff);
  test_sprintf("%x", 0x80000000);

  test_sprintf("%lld", 1234);
  test_sprintf("%lld", -1234);
  test_sprintf("%lld", 0x7fffffff);
  test_sprintf("%lld", 0x80000000);

  test_snprintf(0, "%lld", 0x2000);
  test_snprintf(1, "%lld", 0x2000);
  test_snprintf(2, "%lld", 0x2000);
  test_snprintf(3, "%lld", 0x2000);
  test_snprintf(4, "%lld", 0x2000);
  test_snprintf(20, "%lld", 0x2000);

  test_snprintf(0, "%llx", 0x2000);
  test_snprintf(1, "%llx", 0x2000);
  test_snprintf(2, "%llx", 0x2000);
  test_snprintf(3, "%llx", 0x2000);
  test_snprintf(4, "%llx", 0x2000);
  test_snprintf(20, "%llx", 0x2000);

  test_snprintf(0, "%016llx", 0x2000);
  test_snprintf(1, "%016llx", 0x2000);
  test_snprintf(2, "%016llx", 0x2000);
  test_snprintf(3, "%016llx", 0x2000);
  test_snprintf(4, "%016llx", 0x2000);
  test_snprintf(20, "%016llx", 0x2000);
}

#define RNGBUF_WRITE(src, exp_ret) \
  { int ret = ringbuf_write(&r, src, strlen(src)); \
    ASSERT_VALUE_EQ(ret, exp_ret, "ringbuf_write: %s", src); \
  }

#define RNGBUF_READ(count, exp) \
  { char scratch[32] = { 0 }; \
    int ret = ringbuf_read(&r, scratch, count); \
    ASSERT_VALUE_EQ(ret, strlen(exp), "ringbuf_read: %s", exp); \
    ASSERT_MEM_EQ(exp, scratch, strlen(exp), "ringbuf_read: %s ", exp); \
  }

ringbuf_t r;
char buf[16];

void run_cases_ringbuf_loop(const char *pat)
{
  int i;
  int sz = strlen(pat);

  for (i = 0; i < 20; ++i) { 
    RNGBUF_WRITE(pat, sz);  
    RNGBUF_READ(sz, pat);
  }
}

void run_cases_ringbuf()
{
  ringbuf_init(&r, buf, sizeof(buf));
  RNGBUF_WRITE("1111", 4);
  RNGBUF_WRITE("2222", 4);
  RNGBUF_WRITE("3333", 4);
  RNGBUF_WRITE("4444", 4);
  RNGBUF_WRITE("5555", 0);
  RNGBUF_READ(4, "1111");
  RNGBUF_READ(4, "2222");
  RNGBUF_WRITE("666677778888", 8);
  RNGBUF_READ(16, "3333444466667777");

  run_cases_ringbuf_loop("123");
  run_cases_ringbuf_loop("x123");
  run_cases_ringbuf_loop("xz123");
  run_cases_ringbuf_loop("xyz123");
  run_cases_ringbuf_loop("xyz0123");
  run_cases_ringbuf_loop("kxyz0123");
  run_cases_ringbuf_loop("kixyz0123");
  run_cases_ringbuf_loop("kixuyz0123");
  run_cases_ringbuf_loop("kixutyz0123");
  run_cases_ringbuf_loop("kixutyz01234");
  run_cases_ringbuf_loop("kixutyz012345");
  run_cases_ringbuf_loop("kixutyz0123456");
  run_cases_ringbuf_loop("kixutyz01234567");
}

int main() 
{
  run_cases_rectangle();
  run_cases_strtol();
  run_cases_sprintf();
  run_cases_ringbuf();
}
