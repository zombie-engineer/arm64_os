#pragma once
#include <compiler.h>
#include <block_device.h>
#include <types.h>
#include <mem_access.h>
#include <bug.h>

struct mbr_partition_entry {
  char status_byte;
  char chs_first[3];
  char part_type;
  char chs_last[3];
  uint32_t lba;
  uint32_t num_sectors;
} PACKED;

STRICT_SIZE(struct mbr_partition_entry, 16); 

static inline uint32_t mbr_partition_entry_get_lba(struct mbr_partition_entry *e)
{
  return get_unaligned_32_le(&e->lba);
}

static inline uint32_t mbr_partition_entry_get_num_sectors(struct mbr_partition_entry *e)
{
  return get_unaligned_32_le(&e->num_sectors);
}


struct mbr {
  char bootstap_code[446];
  struct mbr_partition_entry entries[4];
  char boot_signature[2];
} PACKED;

_Static_assert(sizeof(struct mbr) == 512, "Wrong MBR size at compile time");

int mbr_read(struct block_device *b, struct mbr *mbr);

void mbr_print_summary(struct mbr *mbr);

static inline struct mbr_partition_entry *mbr_get_part_by_idx(struct mbr *m, int partition_idx)
{
  BUG(partition_idx > 3, "partition index too large");
  return &m->entries[partition_idx];
}

static inline uint32_t mbr_part_get_lba(struct mbr *mbr, int partition_idx)
{
  struct mbr_partition_entry *e;
  e = mbr_get_part_by_idx(mbr, partition_idx);
  return mbr_partition_entry_get_lba(e);
}

static inline uint32_t mbr_part_get_num_sectors(struct mbr *mbr, int partition_idx)
{
  struct mbr_partition_entry *e;
  e = mbr_get_part_by_idx(mbr, partition_idx);
  return mbr_partition_entry_get_num_sectors(e);
}

