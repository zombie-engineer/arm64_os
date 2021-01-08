#include <fs/fs.h>
#include <fs/fat32.h>
#include <block_device.h>
#include <partition_table.h>
#include <partition_device.h>
#include <common.h>
#include <emmc.h>

int emmc_block_device_read(struct block_device *bdev,
 char *buf,
 uint32_t bufsz,
 uint64_t start_sector,
 uint32_t num_sectors)
{
  uint32_t i;
  uint64_t current_sector;
  int err;
  char *ptr;
  char *bufend;

  ptr = buf;
  bufend = buf + bufsz;
  for (i = 0; i < num_sectors; ++i) {
    if (ptr >= bufend) {
      printf("emmc_block_device_read: buffer too small size: %d, needed: %d\r\n", current_sector, bufsz);
      return -1;
    }
    current_sector = i + start_sector;
    err = emmc_read(current_sector, 1, ptr, bufend - ptr);
    if (err) {
      printf("emmc_block_device_read: failed to read sector %d\r\n", current_sector);
      return -1;
    }
    ptr += bdev->sector_size;
  }
  return num_sectors;
}

int emmc_block_device_write(struct block_device *bdev,
 char *buf,
 int bufsz,
 uint64_t start_sector,
 uint32_t num_sectors)
{
  return -1;
}

static struct block_device emmc_block_device = {
  .ops = {
    .read = emmc_block_device_read,
  },
  .sector_size = 512
};

static inline struct mbr_partition_entry *mbr_get_first_valid_partition(struct mbr *mbr)
{
  int i;
  struct mbr_partition_entry *e;
  for (i = 0; i < ARRAY_SIZE(mbr->entries); ++i) {
    e = &mbr->entries[i];
    if (e->part_type)
      return e;
  }
  return NULL;
}

void fs_probe_early(void)
{
  int err;
  char buf[1024];
  struct block_device *bdev = &emmc_block_device;
  struct mbr mbr ALIGNED(4);
  struct mbr_partition_entry *pe;
  struct partition_device pdev;
  struct fat32_fs fat32fs;
  uint64_t start_sector;
  struct fat_dentry d;

  bdev->ops.read(bdev, buf, sizeof(buf), 0, 1);
  hexdump_memory_ex("emmc", 32, buf, 64);
  bdev->ops.read(bdev, buf, sizeof(buf), 0x2000, 1);
  hexdump_memory_ex("emmc", 32, buf, 64);

  err = mbr_read(bdev, &mbr);
  if (err) {
    printf("fs_probe_early: failed to read MBR from block_device %p\r\n", bdev);
    return;
  }
  mbr_print_summary(&mbr);
  pe = mbr_get_first_valid_partition(&mbr);
  if (!pe) {
    printf("fs_probe_early: no valid partitions in MBR table on block device %p\r\n", bdev);
    return;
  }

  start_sector = mbr_partition_entry_get_lba(pe);
  partition_device_init(&pdev, bdev,
    start_sector,
    start_sector + mbr_partition_entry_get_num_sectors(pe));
  pdev.bdev.ops.read(&pdev.bdev, buf, sizeof(buf), 0, 1);
  hexdump_memory_ex("emmc", 32, buf, 64);
  err = fat32_open(&pdev.bdev, &fat32fs);
  if (err)
    return;
  fat32_lookup(&fat32fs, "/u-boot.bin", &d);
  fat32_ls(&fat32fs, "/");
  fat32_dump_file_cluster_chain(&fat32fs, "/");
}

