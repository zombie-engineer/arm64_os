#include <fs/fat32.h>
#include <common.h>
#include <stringlib.h>

struct fat32_boot_sector fat32_boot_sector ALIGNED(4);

int fat32_open(struct block_device *bdev, struct fat32_fs *f)
{
  char oem_buf[9];
  int err;
  uint32_t root_cluster_num;
  uint32_t cluster_size;
  err = bdev->ops.read(bdev, (char*)&fat32_boot_sector, 512, 0, 1);
  if (err < 0) {
    printf("fat32_open: error reading first sector\r\n");
    return err;
  }

  f->boot_sector = &fat32_boot_sector;
  f->bdev = bdev;
  memcpy(oem_buf, fat32_boot_sector.oem_name, sizeof(fat32_boot_sector.oem_name));
  oem_buf[8] = 0;
  root_cluster_num = fat32_get_root_cluster_num(f);
  cluster_size = fat32_get_bytes_per_cluster(f);
  printf("fat32_open: success. OEM: \"%s\", root_cluster:%d, cluster_sz:%d, fat_sectors:%d\r\n",
    oem_buf,
    root_cluster_num,
    cluster_size, 
    fat32_get_sectors_per_fat(f));
 

  return 0;
}
