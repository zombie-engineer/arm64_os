#include <ringbuf.h>
#include <common.h>
#include <stringlib.h>

#ifdef TEST_STRING
#include <string.h>
#endif

void ringbuf_init(ringbuf_t *r, char *buf, int sz) 
{
  r->buf = r->start = r->end = buf;
  r->buf_end = buf + sz;
}

#define ringbuf_sz_inv(r) ((r->buf_end - r->start) + (r->end - r->buf))
#define ringbuf_sz_fwd(r) (r->end - r->start)
#define ringbuf_size(r) (r->start > r->end ? ringbuf_sz_inv(r) : ringbuf_sz_fwd(r))

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
  if (r->end < r->start) {
    io_sz = min(r->buf_end - r->start, sz);
    memcpy(dst, r->start, io_sz);
    progress = io_sz;
    r->start += progress;

    io_sz = min(r->end - r->buf, sz - progress);
    if (io_sz) {
      memcpy(dst + progress, r->buf, io_sz); 
      r->start = r->buf + io_sz;
      progress += io_sz;
    }
  } else {
    io_sz = min(r->end - r->start, sz);
    memcpy(dst + progress, r->buf, io_sz); 
    progress += io_sz;
    r->start += io_sz;
  }
  return progress;
}

int ringbuf_write(ringbuf_t *r, const char *src, int sz)
{
  int io_sz;
  int progress = 0;
  if (r->end < r->start) {
    io_sz = min(r->start - r->end, sz);
    memcpy(r->end, src, io_sz);
    progress = io_sz;
    r->end += progress;
  } else {
    io_sz = min(r->buf_end - r->end, sz);
    memcpy(r->end, src, io_sz);
    progress += io_sz;
    io_sz = min(r->start - r->buf, sz - progress);
    if (io_sz) {
      memcpy(r->buf, src + progress, io_sz);
      r->end = r->buf + io_sz;
      progress += io_sz;
    }
  }
  return progress;
}
