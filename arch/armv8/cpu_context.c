#include <arch/armv8/cpu_context.h>
#include <common.h>
#include <stringlib.h>

#define CHECKED_ADVANCE(p, n, end) \
  { p += n; if (p > end) kernel_panic("aarch64_cpu_ctx_dump_to_buf: buf overflow\n"); }

/* dump cpu context to buffer */
void aarch64_cpu_ctx_dump_to_buf(char *buf, int bufsize, aarch64_cpu_ctx_t *ctx)
{
  int i;
  const int num_regs_in_line = 2;
  char *p = buf;
  char *end = buf + bufsize;
  for (i = 0; i < sizeof(ctx->u.regs) / sizeof(ctx->u.regs[0]); ++i) {
    CHECKED_ADVANCE(p, sprintf(p, "x%d: %016llx", i, ctx->u.regs[i]), end);
    if (i % num_regs_in_line == 0) {
      CHECKED_ADVANCE(p, sprintf(p, "\n"), end);
    }
    else {
      CHECKED_ADVANCE(p, sprintf(p, "  "), end);
    }
  }
}

void cpu_dump_ctx(char *buf, int bufsize, void *ctx)
{
  aarch64_cpu_ctx_dump_to_buf(buf, bufsize, (aarch64_cpu_ctx_t *)ctx);
}
