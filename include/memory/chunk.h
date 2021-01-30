#pragma once
#include <list.h>

struct chunk {
  void *addr;
  struct list_head list;
};

struct chunk_area {
  struct chunk *chunks_table;
  char *chunks_mem;
  int chunk_sz_log;
  int num_chunks;
  struct list_head free_list;
  struct list_head busy_list;
};

#define __CHUNK_GROUP(__chunk_sz_log, __num_chunks, __alignment, __section_name)\
  char chunk_mem_ ## __chunk_sz_log  [(1 <<__chunk_sz_log) * __num_chunks] SECTION(__section_name);\
  static struct chunk chunk_hdr_ ## __chunk_sz_log  [1 << __chunk_sz_log] ALIGNED(__alignment)

#define __CHUNK(__chunk_sz_log) {\
  .chunks_table = chunk_hdr_ ## __chunk_sz_log,\
  .chunks_mem = chunk_mem_ ## __chunk_sz_log,\
  .chunk_sz_log = __chunk_sz_log,\
  .num_chunks = ARRAY_SIZE(chunk_hdr_ ## __chunk_sz_log),\
}
