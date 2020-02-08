#include <cpu.h>
#include <stringlib.h>
#include "binblock.h"
#include <common.h>


typedef struct cpuctx_binblock_fill_regs_cb_arg {
  binblock_cpuctx_t *binblock;
  int n;
} cpuctx_binblock_fill_regs_cb_arg_t;

static int cpuctx_binblock_fill_regs_cb(const cpu_reg_t *cpu_r, int r_idx, void *cb_priv)
{
  cpuctx_binblock_fill_regs_cb_arg_t *a;
  a = (cpuctx_binblock_fill_regs_cb_arg_t *)cb_priv;
  binblock_cpu_reg_t *bin_r = &a->binblock->regs[a->n];

  if (a->n >= ARRAY_SIZE(a->binblock->regs))
    return -1;

  memset(bin_r->name, 0, REG_NAME_MAX_LEN);
  strncpy(bin_r->name, cpu_r->name,  REG_NAME_MAX_LEN - 1);
  bin_r->value = cpu_r->value;
  a->n++;
  return 0;
}

int cpuctx_binblock_fill_regs(const void *ctx, binblock_cpuctx_t *binblock)
{
  cpuctx_binblock_fill_regs_cb_arg_t a = {
    .binblock = binblock,
    .n = 0,
  };

  cpuctx_enum_registers(ctx, cpuctx_binblock_fill_regs_cb, &a);
  return ERR_OK;
}
typedef struct cpuctx_print_reg_arg {
  cpuctx_print_regs_cb print;
  void *print_priv;
} cpuctx_print_reg_arg_t; 

static int cpuctx_print_reg(const cpu_reg_t *r, int reg_idx, void *cb_priv)
{
  char reg_str[64];
  char regname[5];
  int n;
  cpuctx_print_reg_arg_t *a = (cpuctx_print_reg_arg_t *)cb_priv;

  const char *ptr = r->name;
  char *regptr = regname + sizeof(regname) - 1; // points one char past regname array
  // zero-terminate last char in regname array later

  while(*ptr) 
    ptr++;

  // by now ptr and regptr point at both at last char of zero-termed strings.

  while(regptr >= regname && ptr >= r->name)
    *regptr-- = *ptr--;

  while(regptr >= regname)
    *regptr-- = ' ';

  n = snprintf(reg_str, sizeof(reg_str), "%s: %016llx", regname, r->value);
  return a->print(reg_str, n, a->print_priv);
}

int cpuctx_print_regs(void *ctx, cpuctx_print_regs_cb cb, void *cb_priv)
{
  cpuctx_print_reg_arg_t a = {
    .print = cb,
    .print_priv = cb_priv
  };

  return cpuctx_enum_registers(ctx, cpuctx_print_reg, &a);
}
