#pragma once
#include <block_device.h>

struct fat32_fs {
  struct block_device *bdev;
};

int fat32_open(struct block_device *bdev, struct fat32_fs *f);

void fat32_summary(struct fat32_fs *f);


