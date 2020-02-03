#pragma once
#include <types.h>
#include <compiler.h>
#include <exception.h>

#define MAGIC_BINBLOCK        "BINBLOCK"
#define BINBLOCK_ID_REGS      "__CPUCTX"
#define BINBLOCK_ID_EXCEPTION "__EXCEPT"

typedef struct binblock_header {
  char magic[8];
  char binblock_id[8];
  uint32_t checksum;
  uint32_t len;
} packed binblock_header_t;

typedef struct binblock_cpuctx {
  struct {
    char name[8];
    uint64_t value;
  } cpu_regs[40];
} packed binblock_cpuctx_t;

typedef struct binblock_exception {
  uint64_t type;
  uint64_t esr; 
  uint64_t spsr; 
  uint64_t far;
  binblock_cpuctx_t cpu_ctx;
} packed binblock_exception_t;


typedef int (*sender_fn)(const char *, size_t);
int binblock_send(const void *data, size_t data_sz, const char *binblock_id, sender_fn send);

int fill_exception_block(void *buf, size_t bufsz, exception_info_t *e);
