#include <partition_device.h>
#include <common.h>
#include <stringlib.h>

int partition_device_read(struct block_device *bdev,
 char *buf,
 uint32_t bufsz,
 uint64_t start_sector,
 uint32_t num_sectors)
{
  struct partition_device *pdev;

  pdev = container_of(bdev, struct partition_device, bdev);

  return pdev->disk_dev->ops.read(pdev->disk_dev,
    buf, bufsz,
    pdev->start_sector + start_sector,
    num_sectors);
}

int partition_device_write(struct block_device *bdev,
 char *buf,
 uint32_t bufsz,
 uint64_t start_sector,
 uint32_t num_sectors)
{
  struct partition_device *pdev;

  pdev = container_of(bdev, struct partition_device, bdev);

  return pdev->disk_dev->ops.write(pdev->disk_dev,
    buf, bufsz,
    pdev->start_sector + start_sector,
    num_sectors);
}

void partition_device_init(struct partition_device *pdev,
 struct block_device *disk_dev,
 uint64_t start_sector,
 uint64_t end_sector)
{
  memset(pdev, 0, sizeof(*pdev));
  pdev->bdev.ops.read = partition_device_read;
  pdev->bdev.ops.write = partition_device_write;
  pdev->disk_dev = disk_dev;
  pdev->start_sector = start_sector;
  pdev->end_sector = end_sector;
}
