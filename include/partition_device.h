#pragma once
#include <block_device.h>

struct partition_device {
  /* embedded block device structure */
  struct block_device bdev;

  /*
   * block device that on top of which
   * this partition leaves.
   */
  struct block_device *disk_dev;

  /*
   * start sector on underlying disk from
   * where this partition starts.
   */
  uint64_t start_sector;

  /*
   * next after last sector on underlying disk from
   * where this partition starts.
   */
  uint64_t end_sector;
};

void partition_device_init(struct partition_device *pdev,
 struct block_device *disk_dev,
 uint64_t start_sector,
 uint64_t end_sector);

