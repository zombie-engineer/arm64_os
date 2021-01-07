#pragma once
#include <block_device.h>
#include <types.h>

struct mbr_partition_entry {
  char status_byte;
  char chs_first[3];
  char part_type;
  char chs_last[3];
  uint32_t lba;
  uint32_t num_sectors;
};

struct mbr {
  char bootstap_code[446];
  struct mbr_partition_entry entries[4];
  char boot_signature[2];
};

int mbr_read(struct block_device *b, struct mbr *mbr);
