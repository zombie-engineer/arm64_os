#include <common.h>
#include <kernel_tests.h>
#include <dma.h>
#include <stringlib.h>
#include <delays.h>
#include <reg_access.h>
#include <memory.h>
#include <mmu.h>

#define DMA_CS_0       ((reg32_t)0x3f007000)
#define DMA_CB_ADDR_0  ((reg32_t)0x3f007004)
#define DMA_TI_0       ((reg32_t)0x3f007008)
#define DMA_SRC_0      ((reg32_t)0x3f00700c)
#define DMA_DST_0      ((reg32_t)0x3f007010)
#define DMA_TXFR_LEN_0 ((reg32_t)0x3f007014)
#define DMA_STRIDE_0   ((reg32_t)0x3f007018)
#define DMA_NEXT_CB_0  ((reg32_t)0x3f00701c)
#define DMA_DEBUG_0    ((reg32_t)0x3f007020)
#define DMA_INT        ((reg32_t)0x3f007fe0)
#define DMA_ENA        ((reg32_t)0x3f007ff0)

#define DMA_CS_ACTIVE (1<<0)
#define DMA_CS_END    (1<<1)
#define DMA_CS_RESET  (1<<31)

static inline void test_mmio_dma_flush(
  const char *tag,
  void *addr,
  int sz,
  int mmu_on,
  int is_device_memory)
{
  bool should_flush = mmu_on && !is_device_memory;
  printf("MMU %s. %s cache (\"%s\") at %p(+%d)"__endline,
    mmu_on ? "on" : "off",
    should_flush ? "Flushing" : "Not flushing",
    tag, addr, sz);

  if (should_flush)
    dcache_flush(addr, sz);
}

static inline void test_mmio_dma_invalidate_cache(
  const char *tag,
  void *addr,
  int sz,
  int mmu_on,
  int is_device_memory)
{
  bool should_inval = mmu_on && !is_device_memory;
  printf("MMU %s. %s cache (\"%s\") at %p(+%d)"__endline,
    mmu_on ? "on" : "off",
    should_inval ? "Flushing" : "Not flushing",
    tag, addr, sz);

  if (should_inval)
    dcache_invalidate(addr, sz);
}

static inline void test_mmio_dma_prep_src(char *src, int sz, int mmu_on, int is_device_mem)
{
  int i;
  printf("src:%p" __endline, src);
  for (i = 0; i < sz; ++i)
    src[i] = (i + mmu_on * 0xe0) & 0xff;
  test_mmio_dma_flush("src", src, sz, mmu_on, is_device_mem);
  hexdump_memory_ex("src", 32, src, 8);
}

static inline void test_mmio_dma_prep_dst(char *dst, int sz, int mmu_on, int is_device_mem)
{
  printf("dst:%p" __endline, dst);
  memset(dst, 0x11, sz);
  test_mmio_dma_flush("dst", dst, sz, mmu_on, is_device_mem);
  hexdump_memory_ex("dst", 32, dst, 8);
}

static inline void test_mmio_dump_dma(const char *tag)
{
  printf("DMA[%s]:ENA:%08x,INT:%08x,CS:%08x,TI:%08x,SRC:%08x,DST:%08x,SZ:%08x"__endline,
    tag,
    read_reg(DMA_ENA),
    read_reg(DMA_INT),
    read_reg(DMA_CS_0),
    read_reg(DMA_TI_0),
    read_reg(DMA_SRC_0),
    read_reg(DMA_DST_0),
    read_reg(DMA_TXFR_LEN_0)
  );
}

static inline void test_mmio_dma_prep_cb(
  struct dma_control_block *c,
  char *src,
  char *dst,
  int sz,
  bool mmu_on,
  bool is_device_mem)
{
  memset(c, 0, sizeof(*c));
  c->ti = DMA_TI_DEST_INC|DMA_TI_SRC_INC|DMA_TI_WAIT_RESP;
  c->src_addr = RAM_PHY_TO_BUS_UNCACHED(src);
  c->dst_addr = RAM_PHY_TO_BUS_UNCACHED(dst);
  c->transfer_length = sz;
  test_mmio_dma_flush("cb", c, sizeof(*c), mmu_on, is_device_mem);
}

static inline void test_mmio_dma_init(void)
{
  if ((read_reg(DMA_ENA) & 1) == 0) {
    printf("DMA channel 0 not enabled, enabling" __endline);
    write_reg(DMA_ENA, 1);
    wait_msec(1);
  }

}

void test_mmio_dma(int mmu_on, int dst_is_device_memory)
{
  char src[512] ALIGNED(64);
  char localdst[512] ALIGNED(64);
  char *dst;
  if (dst_is_device_memory)
    dst = (char *)0x13d2000;
  else
    dst = localdst;

  if (mmu_on) {
    mmu_print_va((uint64_t)dst, 1);
  }

  /* 256 byte alignment is required by DMA controller */
  struct dma_control_block cb ALIGNED(256);

  test_mmio_dma_prep_src(src, sizeof(src), mmu_on, 0 /* not device mem */);
  test_mmio_dma_prep_dst(dst, sizeof(src), mmu_on, dst_is_device_memory);
  test_mmio_dma_prep_cb(&cb, src, dst, sizeof(src), mmu_on, 0 /* not device mem */);

  test_mmio_dma_init();

  printf("Resetting" __endline);
  write_reg(DMA_CS_0, DMA_CS_RESET);
  write_reg(DMA_CS_0, DMA_CS_END);

  printf("Setting control block" __endline);
  write_reg(DMA_CB_ADDR_0, RAM_PHY_TO_BUS_UNCACHED(&cb));

  test_mmio_dump_dma("before start");
  puts("Starting DMA transfer" __endline);
  write_reg(DMA_CS_0, DMA_CS_ACTIVE);

  while((read_reg(DMA_CS_0) & 3) != 2) {
    puts("Transfer not completed yet"__endline);
    test_mmio_dump_dma("in flight");
  }

  puts("DMA transfer is complete" __endline);
  write_reg(DMA_CS_0, DMA_CS_END);

  test_mmio_dma_invalidate_cache("dst,post", dst, sizeof(src), mmu_on, dst_is_device_memory);
  hexdump_memory_ex("after_copy dst", 32, dst + 0 , 8);
  hexdump_memory_ex("after_copy dst", 32, dst + 64, 8);
}
