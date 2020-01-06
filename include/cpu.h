#pragma once
#include <compiler.h>

typedef struct cpuctx cpuctx_t;

int cpuctx_dump(void *ctx, char *buf, int bufsize);

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

int cpuctx_serialize(void *ctx, bin_regs_hdr_t *h, char *buf, int bufsize);

typedef struct cpuctx_init_opts {
  /* address to buffer, used as a cpu context
   * generic for architectures
   */
  void *cpuctx;

  /* size of the above buffer
   */
  int cpuctx_sz;

  /* start of stack will be set to this value
   */
  uint64_t sp;

  /* program counter will be set to this value
   */
  uint64_t pc;

  /* arguments that will be passed to function
   */
  struct {
    int argc;
    char **argv;
  } args;

} cpuctx_init_opts_t;

int cpuctx_init(cpuctx_init_opts_t *o);
