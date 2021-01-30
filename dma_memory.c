#include <memory/dma_memory.h>
#include <memory/chunk.h>
#include <refs.h>
#include <common.h>
#include <math.h>
#include <list.h>
#include <config.h>

extern char __dma_memory_start;
extern char __dma_memory_end;

#define dma_memory_start (void *)&__dma_memory_start
#define dma_memory_end   (void *)&__dma_memory_end

#define DMA_SIZE(__n) (ARRAY_SIZE(chunks_ ## __n) << __n)
#define DMA_MEMORY_SIZE_4096 (DMA_MEMORY_SIZE - DMA_SIZE(6) - DMA_SIZE(7) - DMA_SIZE(9))

// SECTION(".bss")

#define DMA_CHUNK_GROUP(__chunk_sz_log, __num_chunks, __alignment)\
  __CHUNK_GROUP(__chunk_sz_log, __num_chunks, __alignment, ".dma_memory")

DMA_CHUNK_GROUP(6 , 64, 64);
DMA_CHUNK_GROUP(7 , 64, 64);
DMA_CHUNK_GROUP(9 , 64, 1024);
DMA_CHUNK_GROUP(12, 64, 4096);
DMA_CHUNK_GROUP(19, 64, 4096);

static struct chunk_area dma_chunk_areas[] = {
  __CHUNK(6),
  __CHUNK(7),
  __CHUNK(9),
  __CHUNK(12),
  __CHUNK(19)
};

static int logsize_to_area_idx[] = {
  /* starting from 5 log2(32) = 5 */
  0, /* 32 -> 64 */
  0, /* 64 -> 64 */
  1, /* 128 -> 128 */
  2, /* 256 -> 512 */
  2, /* 512 -> 512 */
  3, /* 1024 -> 4096 */
  3, /* 2048 -> 4096 */
  3, /* 4096 -> 4096 */
  4, /* 8192 -> 524288 */
  4, /* 16384 -> 524288 */
  4, /* 32768 -> 524288 */
  4, /* 65636 -> 524288 */
  4, /* 131072 -> 524288 */
  4, /* 262144 -> 524288 */
};

static inline int chunk_area_get_szlog(struct chunk_area *a)
{
  return a->chunk_sz_log;
}

static inline struct chunk *chunk_get_by_addr(void *addr, struct chunk_area **chunk_area)
{
  int i;
  struct chunk_area *d;
  struct chunk *entry;
  uint64_t base_addr = (uint64_t)dma_memory_start;
  uint64_t tmp;
  BUG(addr < dma_memory_start, "attempt to free area before dma range");
  BUG(addr >= dma_memory_end, "attempt to free area after dma range");

  /*
   * First detect the right area
   */
  for (i = 0; i < ARRAY_SIZE(dma_chunk_areas); ++i) {
    d = &dma_chunk_areas[i];
    base_addr += d->num_chunks << chunk_area_get_szlog(d);
    if ((uint64_t)addr <= base_addr)
      break;
  }

  /*
   * Now get exact chunk by address
   */
  base_addr = (uint64_t)d->chunks_table[0].addr;
  tmp = (uint64_t)addr - base_addr;
  i = tmp >> chunk_area_get_szlog(d);
  entry = &d->chunks_table[i];
  *chunk_area = d;
  return entry;
}

static inline struct chunk_area *chunk_area_get_by_sz(int sz)
{
  int logsz = get_biggest_log2(sz);

  if ((logsz - 5) > ARRAY_SIZE(logsize_to_area_idx)) {
    printf("chunk_area_get_by_sz: size too big, size:%d,  biggest_log: %d, array_len: %d"__endline, sz, logsz, ARRAY_SIZE(logsize_to_area_idx));
    return NULL;
  }

  return &dma_chunk_areas[logsize_to_area_idx[logsz - 5 - 1]];
}

uint64_t dma_memory_get_start_addr(void)
{
  return (uint64_t)dma_memory_start;
}

uint64_t dma_memory_get_end_addr(void)
{
  return (uint64_t)dma_memory_end;
}

void *dma_alloc(int sz)
{
  struct chunk *c;
  struct chunk_area *a;
  a = chunk_area_get_by_sz(sz);
  BUG(!a, "dma_alloc: size too big");
  c = list_first_entry(&a->free_list, struct chunk, list);
  list_del(&c->list);
  list_add_tail(&c->list, &a->busy_list);
  return c->addr;
}

void dma_free(void *addr)
{
  struct chunk *c;
  struct chunk_area *a;
  c = chunk_get_by_addr(addr, &a);
  BUG(!c, "Failed to find chunk by given address");
  list_del(&c->list);
  list_add(&c->list, &a->free_list);
}

void dma_memory_init(void)
{
  struct chunk_area *ca, *ca_end;
  struct chunk *c, *c_end;
  char *chunk_mem;

  ca = dma_chunk_areas;
  ca_end = ca + ARRAY_SIZE(dma_chunk_areas);

  for (; ca < ca_end; ca++) {
    INIT_LIST_HEAD(&ca->free_list);
    INIT_LIST_HEAD(&ca->busy_list);

    c = ca->chunks_table;
    c_end = c + ca->num_chunks;
    chunk_mem = ca->chunks_mem;
    for (; c < c_end; ++c) {
      INIT_LIST_HEAD(&c->list);
      list_add_tail(&c->list, &ca->free_list);
      c->addr = chunk_mem;
      chunk_mem += (1 << ca->chunk_sz_log);
    }
  }
}
