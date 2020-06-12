#pragma once
#include <exception.h>

void kernel_panic(const char *message);

void report_kernel_panic(exception_info_t *e, const char *panic_message);
