#include <memory/dma_area.h>
#include <refs.h>
#include <common.h>
#include <list.h>
#include <config.h>

extern char __dma_area_start;
extern char __dma_area_end;

#define dma_area_start (void *)&__dma_area_start
#define dma_area_end   (void *)&__dma_area_end

struct chunk {
  void *addr;
  struct list_head list;
};

struct chunk_area {
  struct chunk *chunks_table;
  int szlog;
  int num_chunks;
  struct list_head free_list;
  struct list_head busy_list;
};

#define DMA_SIZE(__n) (ARRAY_SIZE(chunks_ ## __n) << __n)
#define DMA_AREA_SIZE_4096 (DMA_AREA_SIZE - DMA_SIZE(6) - DMA_SIZE(7) - DMA_SIZE(9))

static struct chunk chunks_6  [128] ALIGNED(64);
static struct chunk chunks_7  [128] ALIGNED(64);
static struct chunk chunks_9  [64]  ALIGNED(64);
static struct chunk chunks_12 [DMA_AREA_SIZE_4096 >> 12]  ALIGNED(64);

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
};

#define __CHUNK(__szlog) {\
  .chunks_table = chunks_ ## __szlog,\
  .szlog = __szlog,\
  .num_chunks = ARRAY_SIZE(chunks_ ## __szlog),\
}

static struct chunk_area chunk_areas[] = {
    __CHUNK(6),
    __CHUNK(7),
    __CHUNK(9),
    __CHUNK(12)
};

static inline int get_biggest_log2(int num)
{
  int res = 0;
  int tmp = num;
  while(tmp) {
    res++;
    tmp >>= 1;
  }
  if (tmp << res < num)
    res++;
  return res;
}

static inline int chunk_area_get_szlog(struct chunk_area *a)
{
  return a->szlog;
}

static inline struct chunk *chunk_get_by_addr(void *addr, struct chunk_area **chunk_area)
{
  int i;
  struct chunk_area *d;
  struct chunk *entry;
  uint64_t base_addr = (uint64_t)dma_area_start;
  uint64_t tmp;
  BUG(addr < dma_area_start, "attempt to free area before dma range");
  BUG(addr >= dma_area_end, "attempt to free area after dma range");

  /*
   * First detect the right area
   */
  for (i = 0; i < ARRAY_SIZE(chunk_areas); ++i) {
    d = &chunk_areas[i];
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

  if (logsz > ARRAY_SIZE(logsize_to_area_idx)) {
    printf("chunk_area_get_by_sz: size too big"__endline);
    return NULL;
  }

  return &chunk_areas[logsize_to_area_idx[logsz]];
}

uint64_t dma_area_get_start_addr(void)
{
  return (uint64_t)dma_area_start;
}

uint64_t dma_area_get_end_addr(void)
{
  return (uint64_t)dma_area_end;
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

void dma_area_init(void)
{
  int i;
  struct chunk_area *d;
  for (i = 0; i < ARRAY_SIZE(chunk_areas); ++i) {
    d = &chunk_areas[i];
    INIT_LIST_HEAD(&d->free_list);
    INIT_LIST_HEAD(&d->busy_list);
  }
}
