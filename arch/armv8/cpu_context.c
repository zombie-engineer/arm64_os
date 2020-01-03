#include <arch/armv8/cpu_context.h>
#include <common.h>
#include <stringlib.h>

#define do_sprintf(fmt, ...) \
  n += snprintf(buf + n, bufsz - n, fmt, ##__VA_ARGS__)
   
typedef struct aarch64_cpu_iter {
  const char *name;
  int namelen;
  uint64_t value;
  int reg_idx;
}aarch64_cpu_iter_t;

typedef int (*aarch64_cpu_iter_cb)(const aarch64_cpu_iter_t *, void *);

static char *aarch64_cpu_reg_names[] = {
  "x0" , "x1" , "x2" , "x3" , "x4",
  "x5" , "x6" , "x7" , "x8" , "x9",
  "x10", "x11", "x12", "x13", "x14",
  "x15", "x16", "x17", "x18", "x19",
  "x20", "x21", "x22", "x23", "x24",
  "x25", "x26", "x27", "x28", "x29",
  "lr", "sp" , "pc"
};

static void aarch64_cpu_iter_regs(aarch64_cpu_ctx_t *c, aarch64_cpu_iter_cb cb, void *cb_arg)
{
  int i;
  aarch64_cpu_iter_t it = { 0 };

  for (i = 0; i < ARRAY_SIZE(c->u.regs); ++i) {
    it.value = c->u.regs[i];
    it.reg_idx = i;
    it.name = aarch64_cpu_reg_names[i];
    if (cb(&it, cb_arg))
      break;
  }
}

typedef struct aarch64_print_cpu_arg {
  char *buf;
  int bufsz;
  int n;
} aarch64_print_cpu_arg_t;

static int aarch64_print_cpu_cb(const aarch64_cpu_iter_t *r, void *cb_arg)
{
  char last_char;
  aarch64_print_cpu_arg_t *a;
  const int regs_per_line = 2;

  a = (aarch64_print_cpu_arg_t *)cb_arg;
  last_char = (r->reg_idx % regs_per_line == 0) ? '\n' : ' ';
  a->n += snprintf(a->buf + a->n, a->bufsz - a->n, "%s: %016llx%c", r->name, r->value, last_char);
  return 0;
}

/* dump cpu context to buffer */
int aarch64_print_cpu_ctx(aarch64_cpu_ctx_t *c, char *buf, int bufsz)
{
  aarch64_print_cpu_arg_t a = {
    .buf = buf,
    .bufsz = bufsz,
    .n = 0
  };

  aarch64_cpu_iter_regs(c, aarch64_print_cpu_cb, &a);
  return a.n;
}

typedef struct aarch64_ser_regs_arg {
  char *buf;
  int bufsz;
  int n;
  int numregs;
} aarch64_ser_regs_arg_t;

static int aarch64_ser_regs_cb(const aarch64_cpu_iter_t *it, void *cb_arg)
{
  aarch64_ser_regs_arg_t *a;
  reg_info_t *r;

  a = (aarch64_ser_regs_arg_t *)cb_arg;
  r = (reg_info_t *)(a->buf + a->n);
  a->n += sizeof(*r);

  if (a->n < a->bufsz) {
    memset(r->name, 0, sizeof(r->name));
    strncpy(r->name, it->name, sizeof(r->name));
    r->value = it->value;
  }
  a->numregs++;
  return 0;
}

int aarch64_cpu_serialize_regs(aarch64_cpu_ctx_t *c, bin_regs_hdr_t *h, char *buf, int bufsz)
{
  aarch64_ser_regs_arg_t a = {
    .buf = buf,
    .bufsz = bufsz,
    .n = 0,
    .numregs = 0
  };

  aarch64_cpu_iter_regs(c, aarch64_ser_regs_cb, &a);
  h->numregs = a.numregs;
  h->len = h->numregs * sizeof(reg_info_t);
  return a.n;
}

int cpu_dump_ctx(void *ctx, char *buf, int bufsz)
{
  return aarch64_print_cpu_ctx((aarch64_cpu_ctx_t *)ctx, buf, bufsz);
}


int cpu_serialize_regs(void *ctx, bin_regs_hdr_t *h, char *buf, int bufsz)
{
  return aarch64_cpu_serialize_regs((aarch64_cpu_ctx_t *)ctx, h, buf, bufsz);
}
