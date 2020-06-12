#include <kernel_panic.h>
#include <common.h>

void report_kernel_panic(exception_info_t *e, const char *panic_message)
{
  printf("KERNEL_PANIC: %s" __endline, panic_message);
}
