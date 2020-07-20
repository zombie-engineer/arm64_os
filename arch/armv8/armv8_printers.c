#include <common.h>
#include <bits_api.h>
#include <arch/armv8/cpsr.h>
#include <arch/armv8/cpu_context.h>

void print_cpu_flags()
{
  uint64_t nzcv, daif, current_el, sp_sel;
  // pan, uao, dit, ssbs;

  asm volatile(
    "mrs %0, nzcv\n"
    "mrs %1, daif\n"
    "mrs %2, currentel\n"
    "mrs %3, spsel\n" : "=r"(nzcv), "=r"(daif), "=r"(current_el), "=r"(sp_sel));

  printf("cpsr: nzcv: %x, daif: %x(%s%s%s%s), current_el: %x, sp_sel: %x" __endline,
    nzcv, daif,
    daif & BT(CPSR_F) ? "no-FIQ" : "",
    daif & BT(CPSR_I) ? "no-IRQ" : "",
    daif & BT(CPSR_A) ? "no-SError" : "",
    daif & BT(CPSR_D) ? "no-DBG" : "",
    current_el >> CPSR_EL_OFF, sp_sel);
}

typedef struct print_cpuctx_ctx {
  int nr;
} print_cpuctx_ctx_t;

static int print_reg_cb(const char *reg_str, size_t reg_str_sz, void *cb_priv)
{
  print_cpuctx_ctx_t *print_ctx = (print_cpuctx_ctx_t *)cb_priv;
  if (print_ctx->nr) {
    if (print_ctx->nr % 4 == 0) {
      puts(__endline);
    } else {
      putc(',');
      putc(' ');
    }
  }
  puts(reg_str);
  print_ctx->nr++;
  return 0;
}

void armv8_print_cpu_ctx(armv8_cpuctx_t *ctx)
{
  print_cpuctx_ctx_t print_ctx = { 0 };
  cpuctx_print_regs(ctx, print_reg_cb, &print_ctx);
  puts(__endline);
}
