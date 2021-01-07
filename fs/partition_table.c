#include <partition_table.h>
#include <error.h>
#include <bug.h>

int mbr_read(struct block_device *b, struct mbr *mbr)
{
  BUG(sizeof(*mbr) != 512, "Wrong MBR size at compile time");
  b->ops.read(b, (char*)mbr, sizeof(*mbr), 0, 1);
  return 0;
}
