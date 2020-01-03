#pragma once
#include <compiler.h>

typedef struct cpu_ctx cpu_ctx_t;

int cpu_dump_ctx(void *ctx, char *buf, int bufsize);

typedef struct reg_info {
  char name[8];
  uint64_t value;
} packed reg_info_t;

typedef struct bin_regs_hdr {
  char magic[8];
  int crc;
  int len;
  int numregs;
} packed bin_regs_hdr_t;

int cpu_serialize_regs(void *ctx, bin_regs_hdr_t *h, char *buf, int bufsize);

