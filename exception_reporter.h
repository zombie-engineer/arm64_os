#pragma once
#include <exception.h>

#define FATAL_EXCEPTION_REPORTER_UART      "uart"
#define FATAL_EXCEPTION_REPORTER_VCANVAS   "vcanvas"
#define FATAL_EXCEPTION_REPORTER_NOKIA5110 "nokia5110"

void exception_print_summary_uart(exception_info_t *e);
void exception_print_cpu_ctx_uart(exception_info_t *e);
void exception_dump_cpu_ctx_uart(exception_info_t *e);
void exception_print_summary_vcanvas(exception_info_t *e);
void exception_print_cpu_ctx_vcanvas(exception_info_t *e);
void exception_print_summary_nokia5110(exception_info_t *e);
void exception_print_cpu_ctx_nokia5110(exception_info_t *e);
