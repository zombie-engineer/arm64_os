#include <arch/armv8/cpu_context.h>
#include <common.h>
#include <stringlib.h>

#define do_sprintf(fmt, ...) \
  n += snprintf(buf + n, bufsize - n, fmt, ##__VA_ARGS__)
   
/* dump cpu context to buffer */
int aarch64_print_cpu_ctx(aarch64_cpu_ctx_t *ctx, char *buf, int bufsize)
{
  int i;
  int n;
  const int num_regs_in_line = 2;
  n = 0;
  for (i = 0; i < ARRAY_SIZE(ctx->u.regs); ++i) {
    do_sprintf("x%d: %016llx", i, ctx->u.regs[i]);
    if (i % num_regs_in_line == 0) {
      do_sprintf("\n");
    }
    else {
      do_sprintf("  ");
    }
  }
  return n;
}

int cpu_dump_ctx(void *ctx, char *buf, int bufsize)
{
  return aarch64_print_cpu_ctx((aarch64_cpu_ctx_t *)ctx, buf, bufsize);
}
