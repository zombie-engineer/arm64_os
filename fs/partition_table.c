#include <partition_table.h>
#include <mem_access.h>
#include <common.h>
#include <error.h>
#include <bug.h>

int mbr_read(struct block_device *b, struct mbr *mbr)
{
  // _Static_assert(sizeof(struct mbr) == 512, "Wrong MBR size at compile time");
  b->ops.read(b, (char*)mbr, sizeof(*mbr), 0, 1);
  return 0;
}

void mbr_print_summary(struct mbr *mbr)
{
  int i;
  struct mbr_partition_entry *e;
  uint32_t lba, num_sectors;

  for (i = 0; i < ARRAY_SIZE(mbr->entries); ++i) {
    e = &mbr->entries[i];
    lba = mbr_part_get_lba(mbr, i);
    num_sectors = mbr_part_get_num_sectors(mbr, i);
    printf("part%d: %02x, type:%02x, start:%08x, end:%08x\r\n", i + 1,
      e->status_byte,
      e->part_type,
      lba,
      lba + num_sectors);
  }
}
