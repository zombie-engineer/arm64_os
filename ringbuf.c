#include <ringbuf.h>
#include <common.h>
#include <stringlib.h>

#ifdef TEST_STRING
#include <string.h>
#endif

void ringbuf_init(ringbuf_t *r, char *buf, int sz) 
{
  r->buf = r->write_ptr = r->read_ptr = buf;
  r->buf_end = buf + sz;
  r->read_is_ahead = 0;
}

#define IS_HEAD_FIRST(r) (r->write_ptr >= r->read_ptr)

#define RINGBUF_SZ_INV(r) ((r->buf_end - r->write_ptr) + (r->read_ptr - r->buf))
#define RINGBUF_SZ_FWD(r) (r->write_ptr - r->read_ptr)
#define RINGBUF_SIZE(r) (IS_HEAD_FIRST(r) ? RINGBUF_SZ_INV(r) : RINGBUF_SZ_FWD(r))

int ringbuf_read(ringbuf_t *r, char *dst, int sz) 
{
  /*
  | --------*-------|   
  | ++++++*-*-------|   
  | ++++++*-------*-|   
  | ++++++++++++++*-|   
  | ----*+++++++++*-|   
  */
  int io_sz;
  int progress = 0;
  if (r->read_is_ahead) {
    io_sz = min(r->buf_end - r->read_ptr, sz);
    memcpy(dst, r->read_ptr, io_sz);
    progress = io_sz;
    r->read_ptr += progress;

    io_sz = min(r->write_ptr - r->buf, sz - progress);
    if (io_sz) {
      r->read_is_ahead = 0;
      memcpy(dst + progress, r->buf, io_sz); 
      r->read_ptr = r->buf + io_sz;
      progress += io_sz;
    }
  } else {
    io_sz = min(r->write_ptr - r->read_ptr, sz);
    memcpy(dst, r->read_ptr, io_sz); 
    progress += io_sz;
    r->read_ptr += io_sz;
  }
  return progress;
}

int ringbuf_write(ringbuf_t *r, const char *src, int sz)
{
  int io_sz;
  int progress = 0;

  if (r->read_is_ahead) {
    io_sz = min(r->read_ptr - r->write_ptr, sz);
    memcpy(r->write_ptr, src, io_sz);
    progress = io_sz;
  } else {
    io_sz = min(r->buf_end - r->write_ptr, sz);
    memcpy(r->write_ptr, src, io_sz);
    r->write_ptr += io_sz;
    progress = io_sz;
    io_sz = min(r->read_ptr - r->buf, sz - progress);
    if (io_sz) {
      memcpy(r->buf, src + progress, io_sz);
      r->write_ptr = r->buf + io_sz;
      progress += io_sz;
      r->read_is_ahead = 1;
    } else if (r->write_ptr == r->buf_end) {
      // r->write_ptr = r->buf;
      // r->read_is_ahead = 1;
    }
  }
  return progress;
}
