#pragma once
#include <types.h>

/* interrupt info */
typedef struct aarch64_cpu_ctx {
  union {
    struct {
      uint64_t x0;
      uint64_t x1;
      uint64_t x2;
      uint64_t x3;
      uint64_t x4;
      uint64_t x5;
      uint64_t x6;
      uint64_t x7;
      uint64_t x8;
      uint64_t x9;
      uint64_t x10;
      uint64_t x11;
      uint64_t x12;
      uint64_t x13;
      uint64_t x14;
      uint64_t x15;
      uint64_t x16;
      uint64_t x17;
      uint64_t x18;
      uint64_t x19;
      uint64_t x20;
      uint64_t x21;
      uint64_t x22;
      uint64_t x23;
      uint64_t x24;
      uint64_t x25;
      uint64_t x26;
      uint64_t x27;
      uint64_t x28;
      /* x29 <=> sp register */
      uint64_t x29;
      /* x30 <=> sp register */
      uint64_t x30;
    } n;
    uint64_t regs[31];
  } u;
} aarch64_cpu_ctx_t;

/* dump cpu context to buffer */
void aarch64_cpu_ctx_dump_to_buf(char *buf, int bufsize, aarch64_cpu_ctx_t *ctx);

void aarch64_cpu_ctx_save_ctx(aarch64_cpu_ctx_t *to);

void aarch64_cpu_ctx_load_ctx(aarch64_cpu_ctx_t *from);

