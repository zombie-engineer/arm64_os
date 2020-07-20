#pragma once
#include <types.h>
#include <compiler.h>
#include <exception.h>

#define MAGIC_BINBLOCK        "BINBLOCK"
#define BINBLOCK_ID_EXCEPTION "__EXCPTN"
#define BINBLOCK_ID_STACK     "___STACK"

#define REG_NAME_MAX_LEN 8
typedef struct binblock_header {
  char magic[8];
  char binblock_id[8];
  char checksum[8];
  char len[16];
} PACKED binblock_header_t;

typedef struct binblock_cpu_reg {
  char name[REG_NAME_MAX_LEN];
  uint64_t value;
} PACKED binblock_cpu_reg_t;

typedef struct binblock_cpuctx {
  binblock_cpu_reg_t regs[40];
} PACKED binblock_cpuctx_t;

typedef struct binblock_exception {
  uint64_t type;
  uint64_t esr;
  uint64_t spsr;
  uint64_t far;
  binblock_cpuctx_t cpu_ctx;
} PACKED binblock_exception_t;


typedef int (*sender_fn)(const void*, size_t);
int binblock_send(const void *data, size_t data_sz, const char *binblock_id, sender_fn send);

int binblock_fill_exception(void *buf, size_t bufsz, exception_info_t *e);
