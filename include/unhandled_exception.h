#pragma once
#include <exception.h>

#define REPORTER_ID_UART_PL011 0
#define REPORTER_ID_VCANVAS    1
#define REPORTER_ID_NOKIA5110  2

typedef void (*fn_print_summary)(exception_info_t *);
typedef void (*fn_print_cpu_ctx)(exception_info_t *);
typedef void (*fn_dump_cpu_ctx)(exception_info_t *);

typedef struct unhandled_exception_reporter {
  char name[16];
  int reporter_id;
  int enabled;
  fn_print_summary print_summary;
  fn_print_cpu_ctx print_cpu_ctx;
  fn_dump_cpu_ctx dump_cpu_ctx;
} unhandled_exception_reporter_t;

void init_unhandled_exception_reporters();

/*
 * Enable reporting of unhandle exception via
 * specific reporter.
 */
int enable_unhandled_exception_reporter(int reporter_id);

/*
 * Report unhandled exception with all enabled
 * reporters
 */
void report_unhandled_exception(exception_info_t *e);

