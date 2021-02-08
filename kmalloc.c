#include <memory/kmalloc.h>
#include <memory/page.h>
#include <compiler.h>
#include <error.h>
#include <math.h>
#include <common.h>
#include <bitmap.h>
#include <stringlib.h>

char kernel_memory[16 * 1024 * 1024] SECTION(".kernel_memory");

typedef void *(*kmalloc_fn)(size_t);

struct kmalloc_descriptor {
  uint64_t *bitmap;
  int num_entries;
  int chunk_size_log;
  uint64_t offset;
};

static void *kmalloc_common(struct kmalloc_descriptor *d)
{
  int idx;
  int end_idx = d->num_entries;

  idx = bitmap_set_next_free(d->bitmap, end_idx);
  if (idx == end_idx)
    return ERR_PTR(ERR_NO_RESOURCE);

  return (char *)(kernel_memory) + d->offset + (idx << d->chunk_size_log);
}

/*
 * called with 100% checked kmalloc_descriptor, so we do not
 * do additional checks here.
 */
static void kfree_common(struct kmalloc_descriptor *d, void *addr)
{
  int idx;

  idx = ((char *)addr - (kernel_memory + d->offset)) << d->chunk_size_log;

  BUG(!bitmap_bit_is_set(d->bitmap, idx), "kfree: double free");
  bitmap_clear(d->bitmap, idx);
}

#define DECL_MALLOC_DESC(__chunk_size_log, __num_entries) \
  uint64_t kmalloc_bitmap ## __chunk_size_log[__num_entries >> BITS_PER_WORD_LOG]; \
  static struct kmalloc_descriptor kmalloc_descriptor ## __chunk_size_log = { \
    .bitmap = kmalloc_bitmap ## __chunk_size_log, \
    .num_entries = __num_entries, \
    .chunk_size_log = __chunk_size_log, \
  };

DECL_MALLOC_DESC(3 , 4096);
DECL_MALLOC_DESC(4 , 2048);
DECL_MALLOC_DESC(5 , 1024);
DECL_MALLOC_DESC(6 , 512);
DECL_MALLOC_DESC(7 , 256);
DECL_MALLOC_DESC(8 , 256);
DECL_MALLOC_DESC(9 , 256);
DECL_MALLOC_DESC(10, 256);
DECL_MALLOC_DESC(11, 256);
DECL_MALLOC_DESC(12, 256);
DECL_MALLOC_DESC(13, 64);
DECL_MALLOC_DESC(19, 32);

struct kmalloc_descriptor *kmalloc_all_descriptors[] = {
  &kmalloc_descriptor3,
  &kmalloc_descriptor4,
  &kmalloc_descriptor5,
  &kmalloc_descriptor6,
  &kmalloc_descriptor7,
  &kmalloc_descriptor8,
  &kmalloc_descriptor9,
  &kmalloc_descriptor10,
  &kmalloc_descriptor11,
  &kmalloc_descriptor12,
  &kmalloc_descriptor13,
  &kmalloc_descriptor19,
};

struct kmalloc_descriptor *kmalloc_descriptors[] = {
  &kmalloc_descriptor3,
  &kmalloc_descriptor3,
  &kmalloc_descriptor3,
  &kmalloc_descriptor3,
  &kmalloc_descriptor4,
  &kmalloc_descriptor5,
  &kmalloc_descriptor6,
  &kmalloc_descriptor7,
  &kmalloc_descriptor8,
  &kmalloc_descriptor9,
  &kmalloc_descriptor10,
  &kmalloc_descriptor11,
  &kmalloc_descriptor12,
  &kmalloc_descriptor13,
  &kmalloc_descriptor19,
  &kmalloc_descriptor19,
  &kmalloc_descriptor19,
  &kmalloc_descriptor19,
  &kmalloc_descriptor19,
  &kmalloc_descriptor19,
};

void *kmalloc(size_t size, int flags)
{
  int logsz = get_biggest_log2(size);
  void *result;

  if (logsz >= ARRAY_SIZE(kmalloc_descriptors))
    return ERR_PTR(ERR_NO_RESOURCE);
  result = kmalloc_common(kmalloc_descriptors[logsz]);
  if (!IS_ERR(result)) {
    if (flags & GFP_ZERO)
      memset(result, 0, size);
  }
  return result;
}

static inline bool addr_in_range(char *addr, char *start, char *end)
{
  return addr >= start && addr < end;
}

void kfree(void *addr)
{
  int i;
  struct kmalloc_descriptor *d;

  BUG(!addr_in_range(addr, kernel_memory,
    kernel_memory + sizeof(kernel_memory)),
    "kfree: invalid address");

  for (i = 1; i < ARRAY_SIZE(kmalloc_all_descriptors); ++i) {
    d = kmalloc_all_descriptors[i];
    if (kernel_memory + d->offset > (char *)addr)
      break;
  }

  BUG(i == ARRAY_SIZE(kmalloc_all_descriptors),
    "kfree: address beyond descriptors");
  kfree_common(d - 1, addr);
}

void kmalloc_init(void)
{
  int i;
  struct kmalloc_descriptor *d, *prev_d = NULL;
  for (i = 0; i < ARRAY_SIZE(kmalloc_all_descriptors); ++i) {
    d = kmalloc_all_descriptors[i];
    if (!prev_d)
      d->offset = 0;
    else
      d->offset = prev_d->offset + (prev_d->num_entries << prev_d->chunk_size_log);

    prev_d = d;
  }
}
