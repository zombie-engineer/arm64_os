#include <fs/fat32.h>
#include <common.h>
#include <stringlib.h>

struct fat32_boot_sector fat32_boot_sector ALIGNED(4);

static uint32_t fat32_get_next_cluster(struct fat32_fs *f, uint32_t cluster)
{
  return FAT32_ENTRY_END_OF_CHAIN_ALT;
}

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

  volatile int xx = 1;
  while(xx);
  f->boot_sector = &fat32_boot_sector;
  f->bdev = bdev;
  memcpy(oem_buf, fat32_boot_sector.oem_name, sizeof(fat32_boot_sector.oem_name));
  oem_buf[8] = 0;
  root_cluster_num = fat32_get_root_dir_cluster(f);
  cluster_size = fat32_get_bytes_per_cluster(f);
  printf("fat32_open: success. OEM: \"%s\", root_cluster:%d, cluster_sz:%d, fat_sectors:%d\r\n",
    oem_buf,
    root_cluster_num,
    cluster_size,
    fat32_get_sectors_per_fat(f));
  return 0;
}

#define LFN_UCS2_TO_ASCII(__le, __dst_ptr, __end_ptr, __field, __result_num)\
   __result_num = wtomb(__dst_ptr, __end_ptr - __dst_ptr, __le->__field, sizeof(__le->__field));\
   if (!__result_num)\
     break;\
   __dst_ptr += __result_num;\
   if (__result_num < (sizeof(__le->__field) >> 1))\
     break;

static inline char *vfat_lfn_fill_name(struct vfat_lfn_entry *le, struct fat_dentry *dentry, char *buf, int bufsz)
{
  int num;
  char *ptr = buf;
  char *end = buf + bufsz - 1;

  while((char *)le < (char *)dentry) {
    LFN_UCS2_TO_ASCII(le, ptr, end, name_part_1, num);
    LFN_UCS2_TO_ASCII(le, ptr, end, name_part_2, num);
    LFN_UCS2_TO_ASCII(le, ptr, end, name_part_3, num);
    le++;
  }
  *ptr = 0;
  return ptr;
}

static inline int fat32_dentry_fill_name(struct fat_dentry *d, char *buf, int bufsz)
{
  int i;
  char *ptr, *end;
  ptr = buf;
  end = buf + bufsz;

  for (i = 0; i < sizeof(d->filename_short) && ptr < end; ++i) {
    if (!d->filename_short[i])
      break;
    *ptr++ = d->filename_short[i];
  }

  if (ptr < end && d->extension[0])
    *ptr++ = '.';

  for (i = 0; i < sizeof(d->extension) && ptr < end; ++i) {
    if (d->extension[i] && d->extension[i] != ' ')
      *ptr++ = d->extension[i];
  }

  *ptr = 0;
  return ptr - buf;
}

void fat32_dentry_print(struct fat_dentry *d, struct fat_dentry *lfn)
{
  uint32_t cluster;
  char namebuf[128];

  if (lfn)
    vfat_lfn_fill_name((struct vfat_lfn_entry *)lfn, d, namebuf, sizeof(namebuf));
  else
    fat32_dentry_fill_name(d, namebuf, sizeof(namebuf));

  cluster = fat32_dentry_get_cluster(d);
  printf("%s size: %d, data cluster: %d, attr: %02x\r\n", namebuf, d->file_size, cluster, d->file_attributes);
}

int fat32_ls(struct fat32_fs *f, const char *dirpath)
{
  char buf[512];
  int err;
  uint32_t cluster;
  uint32_t sectors_per_cluster;
  struct fat_dentry *d, *d_end;
  struct fat_dentry *first_lfn;
  uint64_t data_start_sector;
  uint64_t sector;
  uint32_t sector_size;

  d = NULL;
  data_start_sector = fat32_get_data_start_sector(f);
  sector_size = fat32_get_bytes_per_sector(f);

  sectors_per_cluster = fat32_get_sectors_per_cluster(f);

  if (dirpath[0] == '/' && !dirpath[1])
    cluster = fat32_get_root_dir_cluster(f);
  else
    cluster = 2;

  do {
    printf("fat32_ls: reading dir cluster: %d\r\n", cluster, sector);
    sector = data_start_sector + (cluster - FAT32_DATA_FIRST_CLUSTER) * sectors_per_cluster;
    for (; sector < sector + sectors_per_cluster; ++sector) {
      printf("fat32_ls: reading dir sector: sector: %lld\r\n", sector);
      err = f->bdev->ops.read(f->bdev, buf, sizeof(buf), sector, 1);
      if (err < 0) {
        printf("fat32_ls: failed to read root dir cluster, err = %d\r\n", err);
        return err;
      }
      d = (struct fat_dentry *)buf;
      d_end = (struct fat_dentry *)(buf + sector_size);

      /* print first directory */
      first_lfn = fat32_dentry_is_vfat_lfn(d) ? d : 0;
      if (!first_lfn && fat32_dentry_is_valid(d))
        fat32_dentry_print(d, first_lfn);

      while(1) {
        /* next directory */
        d = fat32_dentry_next(d, d_end, &first_lfn);
        /* no more entries */
        if (!d)
          goto end;

        if (IS_ERR(d)) {
          printf("fat32_ls: failed to get next dentry, err = %d\r\n",(uint32_t)PTR_ERR(d));
          return PTR_ERR(d);
        }

        /* iterated to the end of sector */
        if (d == d_end)
          break;

        fat32_dentry_print(d, first_lfn);
        first_lfn = 0;
      }
    }
    if (!d) {
      /* EOF reached */
      break;
    }
    cluster = fat32_get_next_cluster(f, cluster);
  } while(cluster != FAT32_ENTRY_END_OF_CHAIN && cluster != FAT32_ENTRY_END_OF_CHAIN_ALT);
end:
  return 0;
}
