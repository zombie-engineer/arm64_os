#include "binblock.h"
#include <checksum.h>
#include <stringlib.h>
#include <error.h>

int binblock_send(const void *data, size_t data_sz, const char *binblock_id, sender_fn send)
{
  int ret;
  binblock_header_t header = { 0 };
  header.checksum = checksum_basic(data, data_sz, 0);
  header.len = data_sz;
  memcpy(header.magic, MAGIC_BINBLOCK, sizeof(MAGIC_BINBLOCK));

  ret = send(&header, sizeof(header));
  ret = send(data, data_sz);
  return ret;
}

int fill_exception_block(void *buf, size_t bufsz, exception_info_t *e)
{
  int ret;
  binblock_exception_t ex;
  memset(&ex, 0, sizeof(ex));
  ex.type = e->type;
  ex.esr = e->esr;
  ex.spsr = e->spsr;
  ex.far = e->far;
  ret = ERR_OK;
  return ret;
}
