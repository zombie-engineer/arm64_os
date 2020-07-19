#pragma once

typedef struct ringbuf {
  /* linear array start */
  char *buf;
  /* linear array end */
  char *buf_end;

  /* writes feed write_ptr, write_ptr extends until 
   * it reaches read_ptr 
   */
  char *write_ptr;

  /* reads eat up read_ptr until reach write_ptr */
  char *read_ptr;

  /* read awrite_ptr of write */
  int read_is_ahead;
} ringbuf_t;

void ringbuf_init(ringbuf_t *r, char *buf, int sz);

#define ringbuf_sz_inv(r) ((r->buf_end - r->start) + (r->end - r->buf))
#define ringbuf_sz_fwd(r) (r->end - r->start)
#define ringbuf_size(r) (r->start > r->end ? ringbuf_sz_inv(r) : ringbuf_sz_fwd(r))

int ringbuf_read(ringbuf_t *r, char *dst, int sz);

int ringbuf_write(ringbuf_t *r, const char *src, int sz);
