#include <cpu.h>
#include <stringlib.h>

typedef struct cpuctx_serialize_cb_arg {
  char *buf;
  int bufsz;
  int n;
  int numregs;
} cpuctx_serialize_cb_arg_t;

static int cpuctx_serialize_cb(const cpu_reg_t *r_it, int r_idx, void *cb_priv)
{
  cpuctx_serialize_cb_arg_t *a;
  reg_info_t *r_info;

  a = (cpuctx_serialize_cb_arg_t *)cb_priv;
  r_info = (reg_info_t *)(a->buf + a->n);
  a->n += sizeof(*r_info);

  if (a->n < a->bufsz) {
    memset(r_info->name, 0, sizeof(r_info->name));
    strncpy(r_info->name, r_it->name, sizeof(r_info->name));
    r_info->value = r_it->value;
  }
  a->numregs++;
  return 0;
}

int cpuctx_serialize(const void *ctx, bin_regs_hdr_t *h, char *buf, int bufsz)
{
  cpuctx_serialize_cb_arg_t a = {
    .buf = buf,
    .bufsz = bufsz,
    .n = 0,
    .numregs = 0
  };

  cpuctx_enum_registers(ctx, cpuctx_serialize_cb, &a);
  h->numregs = a.numregs;
  h->len = h->numregs * sizeof(reg_info_t);
  return a.n;
}

typedef struct cpuctx_print_reg_arg {
  cpuctx_print_regs_cb print;
  void *print_priv;
} cpuctx_print_reg_arg_t; 

static int cpuctx_print_reg(const cpu_reg_t *r, int reg_idx, void *cb_priv)
{
  char reg_str[64];
  char regname[4];
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
