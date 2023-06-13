#include <unhandled_exception.h>
#include <common.h>
#include "exception_reporter.h"

static unhandled_exception_reporter_t unhandled_exception_reporters[] = {
  {
    .name = FATAL_EXCEPTION_REPORTER_UART,
    .reporter_id = REPORTER_ID_UART_PL011,
    .enabled = 0,
    .print_summary = exception_print_summary_uart,
    .print_cpu_ctx = exception_print_cpu_ctx_uart,
    .dump_cpu_ctx = exception_dump_cpu_ctx_uart,
  },
  {
    .name = FATAL_EXCEPTION_REPORTER_VCANVAS,
    .reporter_id = REPORTER_ID_VCANVAS,
    .enabled = 0,
    .print_summary = exception_print_summary_vcanvas,
    .print_cpu_ctx = exception_print_cpu_ctx_vcanvas,
    .dump_cpu_ctx = 0
  },
#ifdef CONFIG_NOKIA_5110
  {
    .name = FATAL_EXCEPTION_REPORTER_NOKIA5110,
    .reporter_id = REPORTER_ID_NOKIA5110,
    .enabled = 0,
    .print_summary = exception_print_summary_nokia5110,
    .print_cpu_ctx = exception_print_cpu_ctx_nokia5110,
    .dump_cpu_ctx = 0
  }
#endif
};

void init_unhandled_exception_reporters()
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    r->enabled = 1;
  }
}

void report_unhandled_exception(exception_info_t *e)
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    if (r->enabled) {
      if (r->print_summary)
        r->print_summary(e);
      if (r->print_cpu_ctx)
        r->print_cpu_ctx(e);
      if (r->dump_cpu_ctx)
        r->dump_cpu_ctx(e);
    }
  }
}

int enable_unhandled_exception_reporter(int reporter_id)
{
  int i;
  unhandled_exception_reporter_t *r;
  for (i = 0; i < ARRAY_SIZE(unhandled_exception_reporters); ++i) {
    r = &unhandled_exception_reporters[i];
    if (r->reporter_id == reporter_id) {
      r->enabled = 1;
      return ERR_OK;
    }
  }
  return ERR_NOT_FOUND;
}
