#include <rectangle.h>
#include <ringbuf.h>
#include <stringlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_VALUE_EQ(v1, v2, desc) \
  if ((v1) != (v2)) { printf("assertion! values not equal for '%s': %lld != %lld\n", desc, (long long)v1, (long long)v2); exit(1); }

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
  char buf1[256];
  char buf2[256];
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  _vsprintf(buf1, fmt, args);
  __builtin_va_start(args, fmt);
  vsprintf(buf2, fmt, args);
  if (strcmp(buf1, buf2)) {
    printf("assertion! strings not equal: expected: '%s', got '%s'\n", buf1, buf2);
    exit(1);
  }
  printf("success: '%s'\n", buf1);
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

int main() 
{
  run_cases_rectangle();
  run_cases_strtol();
  run_cases_sprintf();
}
