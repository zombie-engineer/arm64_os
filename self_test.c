#include <common.h>
#include <stringlib.h>

#define FAIL() while(1);

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define __msg_codeline(__msg) "("__FILE__ ":" STRINGIFY(__LINE__) "): "__msg "\n"

#define __puts_codeline(__msg) puts(__msg_codeline(__msg))
#define __printf_codeline(__fmt, ...) printf(__msg_codeline(__fmt), ## __VA_ARGS__)

void test_sprintf()
{
  char buf[1024];
  int n;
  puts("Running test_sprintf.\n");

#define TEST_SPRINTF(exp, exp_n, fmt, ...)\
  n = sprintf(buf, fmt, ## __VA_ARGS__);\
  if (n != exp_n) {\
    __puts_codeline(" Unexpected return value for '" #fmt "'. expected " #exp_n);\
    FAIL();\
  }\
  if (strcmp(buf, exp)) {\
    __puts_codeline(" Unexpected string value for '" #fmt "'. expected " exp);\
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
  char buf[1024];
  int n;
  uint64_t ts1, ts2;
  printf("Running test_perf_sprintf. generic counter freq: %d Hz\n", 
      get_cpu_counter_64_freq());
#define TEST_SPRINTF_PERF(exp, exp_n, fmt, ...)\
  ts1 = read_cpu_counter_64();\
  n = sprintf(buf, fmt, ## __VA_ARGS__);\
  ts2 = read_cpu_counter_64();\
  if (n != exp_n) {\
    __puts_codeline(" Unexpected return value for '" #fmt "'. expected " #exp_n);\
    FAIL();\
  }\
  if (strcmp(buf, exp)) {\
    __puts_codeline(" Unexpected string value for '" #fmt "'. expected " exp);\
    FAIL();\
  }\
  __printf_codeline("snprintf(\"%s\") took %llu counter ticks.)", fmt, ts2- ts1);\
  puts(buf);\
  putc('\n');
  TEST_SPRINTF_PERF("0", 1, "%d", 0);
  TEST_SPRINTF_PERF("1", 1, "%d", 1);
  TEST_SPRINTF_PERF("12", 2, "%d", 12);
  TEST_SPRINTF_PERF("123", 3, "%d", 123);
  TEST_SPRINTF_PERF("1234", 4, "%d", 1234);
  TEST_SPRINTF_PERF("12345", 5, "%d", 12345);
  TEST_SPRINTF_PERF("12345", 5, "%x", 0x12345);
  TEST_SPRINTF_PERF("1234512345", 10, "%x%x", 0x12345, 0x12345);
  TEST_SPRINTF_PERF("12345123451234512345", 20, "%x%x%x%x", 0x12345, 0x12345, 0x12345, 0x12345);

  puts("Running test_perf_sprintf\n");
}

void self_test()
{
  puts("Running self test.\n");
  test_sprintf();
  test_perf_sprintf();
  printf("Self test complete.\n");
}
