#pragma once

typedef struct ringbuf {
  char *buf;
  char *buf_end;
  char *start;
  char *end;
} ringbuf_t;

char pipe_buf[2048];
ringbuf_t pipe;

void ringbuf_init(ringbuf_t *r, char *buf, int sz);

#define ringbuf_sz_inv(r) ((r->buf_end - r->start) + (r->end - r->buf))
#define ringbuf_sz_fwd(r) (r->end - r->start)
#define ringbuf_size(r) (r->start > r->end ? ringbuf_sz_inv(r) : ringbuf_sz_fwd(r))

int ringbuf_read(ringbuf_t *r, char *dst, int sz);

int ringbuf_write(ringbuf_t *r, const char *src, int sz);
