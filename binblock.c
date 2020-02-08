#include "binblock.h"
#include <checksum.h>
#include <stringlib.h>
#include <error.h>
#include <cpu.h>

int binblock_send(const void *data, size_t data_sz, const char *binblock_id, sender_fn send)
{
  int ret;
  binblock_header_t header = { 0 };
  header.checksum = checksum_basic(data, data_sz, 0);
  header.len = data_sz;
  memcpy(&header.magic[0], MAGIC_BINBLOCK, sizeof(header.magic));
  memcpy(&header.binblock_id[0], binblock_id, sizeof(header.binblock_id));

  ret = send((const char *)&header, sizeof(header));
  ret = send(data, data_sz);
  return ret;
}

int binblock_fill_exception(void *buf, size_t bufsz, exception_info_t *e)
{
  binblock_exception_t *ex = (binblock_exception_t *)buf;
  if (bufsz < sizeof(*ex))
    return ERR_INVAL_ARG;

  memset(ex, 0, sizeof(*ex));
  ex->type = e->type;
  ex->esr  = e->esr;
  ex->spsr = e->spsr;
  ex->far  = e->far;
  cpuctx_binblock_fill_regs(e->cpu_ctx, &ex->cpu_ctx);
  return sizeof(*ex);
}
