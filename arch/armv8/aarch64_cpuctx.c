#include <arch/armv8/cpu_context.h>
#include <compiler.h>
#include <common.h>
#include <stringlib.h>
#include "armv8_asm.h"

static const char *aarch64_cpu_reg_names[] = {
  "x0" , "x1" , "x2" , "x3" , "x4",
  "x5" , "x6" , "x7" , "x8" , "x9",
  "x10", "x11", "x12", "x13", "x14",
  "x15", "x16", "x17", "x18", "x19",
  "x20", "x21", "x22", "x23", "x24",
  "x25", "x26", "x27", "x28", "x29",
  "lr", "sp" , "pc"
};

int cpuctx_enum_registers(const void *ctx, cpuctx_enum_regs_cb cb, void *cb_priv)
{
  int i;
  cpu_reg_t reg = { 0 };
  const aarch64_cpuctx_t *c = (const aarch64_cpuctx_t*)ctx;

  for (i = 0; i < ARRAY_SIZE(c->u.regs); ++i) {
    memset(reg.name, 0, sizeof(reg.name));
    strncpy(reg.name, aarch64_cpu_reg_names[i], sizeof(reg.name) - 1);
    reg.value = c->u.regs[i];
    if (cb(&reg, i, cb_priv))
      break;
  }
  return ERR_OK;
}

int cpuctx_init(cpuctx_init_opts_t *o)
{
  memset(o->cpuctx, 0, o->cpuctx_sz);
  ((aarch64_cpuctx_t *)o->cpuctx)->u.n.pc = o->pc;
  ((aarch64_cpuctx_t *)o->cpuctx)->u.n.sp = o->sp;
  return ERR_OK;
}
