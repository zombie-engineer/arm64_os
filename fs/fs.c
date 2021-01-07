#include <fs/fs.h>
#include <block_device.h>
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

void fs_probe_early(void)
{
  char buf[1024];
  struct block_device *bdev = &emmc_block_device;

  bdev->ops.read(bdev, buf, sizeof(buf), 0, 1);
  hexdump_memory_ex("emmc", 32, buf, 64);
  bdev->ops.read(bdev, buf, sizeof(buf), 0x2000, 1);
  hexdump_memory_ex("emmc", 32, buf, 64);
}
