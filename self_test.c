#include <common.h>
#include <stringlib.h>

#define FAIL() while(1);
void test_sprintf()
{
  char buf[1024];
  int n;
  puts("Running test_sprintf.\n");
#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#define TEST_SPRINTF(exp, exp_n, fmt, ...)\
  n = sprintf(buf, fmt, ## __VA_ARGS__);\
  if (n != exp_n) {\
    puts(__FILE__ ":" STRINGIFY(__LINE__) " Unexpected return value for '" #fmt "'. expected " #exp_n " \n");\
    FAIL();\
  }\
  if (strcmp(buf, exp)) {\
    puts(__FILE__ ":" STRINGIFY(__LINE__) " Unexpected string value for '" #fmt "'. expected " exp " \n");\
    FAIL();\
  }\
  puts(buf);\
  putc('\n');


  TEST_SPRINTF("0", 1, "%d", 0);
  TEST_SPRINTF("1", 1, "%d", 1);
  TEST_SPRINTF("12345\n", 6, "%d\n", 12345);
  TEST_SPRINTF("12345\n", 6, "%u\n", 12345);
  TEST_SPRINTF("6623131232\n", 11, "%lu\n", 6623131232);
  TEST_SPRINTF("6623131232\n", 11, "%llu\n", 6623131232);
  TEST_SPRINTF("100000023131232-hello\n", 22, "%lu-%s\n", 100000023131232, "hello");
  TEST_SPRINTF("0xffffffff12345678", 18, "%p", 0xffffffff12345678);
#undef TEST_SPRINTF
  puts("Running test_sprintf complete.\n");
}

void test_perf_sprintf()
{
  puts("Running test_perf_sprintf.\n");
  puts("Running test_perf_sprintf complete.\n");
}

void self_test()
{
  puts("Running self test.\n");
  test_sprintf();
  test_perf_sprintf();
  puts("Self test complete.\n");
}
