#include "binblock.h"
#include <checksum.h>
#include <stringlib.h>
#include <error.h>
#include <cpu.h>

static int bin_to_ascii_hex(char *dst, size_t dst_sz, const char *src, size_t src_sz)
{
  char current_byte, half_byte;
  char *dst_end = dst + dst_sz;
  const char *s = src;
  const char *src_end = s + src_sz;
  while(s != src_end && dst + 2 <= dst_end) {
    current_byte = *s++;
    half_byte = (current_byte >> 4) & 0xf;
    half_byte += (half_byte < 10) ? '0' : 'a' - 10;
    *dst++ = half_byte;
    half_byte = current_byte & 0xf;
    half_byte += (half_byte < 10) ? '0' : 'a' - 10;
    *dst++ = half_byte;
  }
  return (s - src) * 2;
}

static int binblock_send_ascii_bin(const void *bytes, size_t bytes_count, sender_fn send)
{
  char buf[128];
  int bytes_sent = 0;
  int sz;
  while(bytes_sent < bytes_count) {
    sz = bin_to_ascii_hex(buf, sizeof(buf),
        ((const char *)bytes) + bytes_sent,
        bytes_count - bytes_sent);

    send(buf, sz);
    bytes_sent += sz / 2;
  }

  return bytes_sent;
}

int binblock_send(const void *data, size_t data_sz, const char *binblock_id, sender_fn send)
{
  int checksum = checksum_basic(data, data_sz, 0);
  if (send(MAGIC_BINBLOCK, 8) != 8)
    return -1;
  if (send(binblock_id, 8) != 8)
    return -1;
  if (binblock_send_ascii_bin(&checksum, sizeof(checksum), send) != sizeof(checksum))
    return -1;
  if (binblock_send_ascii_bin(&data_sz, sizeof(data_sz), send) != sizeof(data_sz))
    return -1;
  if (binblock_send_ascii_bin(data, data_sz, send) != data_sz)
    return -1;
  return 0;
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
