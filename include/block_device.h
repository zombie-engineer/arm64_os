#pragma once
#include <types.h>

struct block_device;

struct block_device_ops {
  int (*read)(struct block_device *, char *buf, uint32_t bufsz, uint64_t start_sector, uint32_t num_sectors);
  int (*write)(struct block_device *, char *buf, uint32_t bufsz, uint64_t start_sector, uint32_t num_sectors);
};

struct block_device {
  struct block_device_ops ops;
  int sector_size;
  void *priv;
};
