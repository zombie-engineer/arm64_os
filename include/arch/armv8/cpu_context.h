#pragma once
#include <types.h>
#include <cpu.h>

typedef struct aarch64_cpuctx {
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
      /* x29 is a frame pointer */
      uint64_t x29;
      /* x30 is a link register */
      uint64_t x30;

      uint64_t sp;   // +31 * 8
      uint64_t pc;   // +32 * 8
      uint64_t cpsr; // + 33 * 8
    } n;
    uint64_t regs[34];
  } u;
} aarch64_cpuctx_t;

/* dump cpu context to buffer */
int aarch64_print_cpu_ctx(aarch64_cpuctx_t *ctx, char *buf, int bufsize);

void aarch64_cpu_ctx_save_ctx(aarch64_cpuctx_t *to);

void aarch64_cpu_ctx_load_ctx(aarch64_cpuctx_t *from);
