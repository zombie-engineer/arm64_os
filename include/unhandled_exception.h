#pragma once
#include <exception.h>

typedef void (*fn_print_summary)(exception_info_t *);
typedef void (*fn_print_cpu_ctx)(exception_info_t *);
typedef void (*fn_dump_cpu_ctx)(exception_info_t *);

typedef struct unhandled_exception_reporter {
  char name[16];
  int enabled;
  fn_print_summary print_summary;
  fn_print_cpu_ctx print_cpu_ctx;
  fn_dump_cpu_ctx dump_cpu_ctx;
} unhandled_exception_reporter_t;

void init_unhandled_exception_reporters();

/*
 * Report unhandled exception with all enabled
 * reporters
 */
void report_unhandled_exception(exception_info_t *e);

