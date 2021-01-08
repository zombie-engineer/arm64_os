#include <fs/fat32.h>
#include <common.h>
#include <stringlib.h>

struct fat32_boot_sector fat32_boot_sector ALIGNED(4);

static char fatbuf[512] ALIGNED(4) = { 0 };

static uint32_t fat32_get_next_cluster_num(struct fat32_fs *f, uint32_t cluster_idx)
{
  int err;
  uint32_t fat_start_sector;
  uint32_t entries_per_sector;
  uint64_t entry_sector;
  uint32_t offset_in_sector;
  uint32_t *fat_sector;

  fat_start_sector = fat32_get_num_reserved_sectors(f);
  entries_per_sector = fat32_get_logical_sector_size(f) >> FAT32_ENTRY_SIZE_LOG;
  entry_sector = fat_start_sector + cluster_idx / entries_per_sector;
  if (entry_sector > fat32_get_sectors_per_fat(f)) {
    printf("fat32_get_next_cluster_num: cluster index %d too big\r\n", cluster_idx);
    return FAT32_ENTRY_END_OF_CHAIN;
  }
  offset_in_sector = cluster_idx % entries_per_sector;

  err = f->bdev->ops.read(f->bdev, fatbuf, sizeof(fatbuf), entry_sector, 1);
  if (err < 0) {
    printf("fat32_get_next_cluster_num: error reading first sector\r\n");
    return err;
  }

  fat_sector = (uint32_t *)fatbuf;

  return fat_sector[offset_in_sector];
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

static inline char *fat32_fill_filename_lfn(struct vfat_lfn_entry *le, struct fat_dentry *dentry, char *buf, int bufsz)
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

static inline int fat32_dentry_fill_name_short(struct fat_dentry *d, char *buf, int bufsz)
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

void fat32_dentry_get_filename(struct fat_dentry *d, struct vfat_lfn_entry *le, char *buf, uint32_t bufsz)
{
  if (le)
    fat32_fill_filename_lfn(le, d, buf, bufsz);
  else
    fat32_dentry_fill_name_short(d, buf, bufsz);
}

void fat32_dentry_print(struct fat_dentry *d, struct vfat_lfn_entry *le)
{
  uint32_t cluster;
  char namebuf[128];

  fat32_dentry_get_filename(d, le, namebuf, sizeof(namebuf));

  cluster = fat32_dentry_get_cluster(d);
  printf("%s size: %d, data cluster: %d, attr: %02x\r\n", namebuf, d->file_size, cluster, d->file_attributes);
}

/*
 * Iterate symbols of filepath until end of string of slash-delimeter '/'
 * returns number of symbols from the given position to end of subpath,
 * subpath is terminated with '/', zero-terminator or end pointer, whichever
 * is met first.
 */
int filepath_get_next_element(const char **path)
{
  int len;
  const char *p = *path;

  while(*p && *p != '/')
    p++;

  len = p - *path;
  *path = p;
  return len;
}

//static inline void fat32_iter_cluster_chain(struct fat32_fs *f, uint32_t cluster_idx, int(*cb)(uint32_t, void *), void *cb_arg)
//{
//  uint32_t cluster  = cluster_idx;
//  do {
//    printf("fat32_ls: reading dir cluster: %d\r\n", cluster);
//    if (cb(cluster, cb_arg))
//      break;
//    cluster = fat32_get_next_cluster_num(f, cluster);
//  } while(cluster != FAT32_ENTRY_END_OF_CHAIN && cluster != FAT32_ENTRY_END_OF_CHAIN_ALT);
//}

/*
 * cluster_it - cluster iterator.
 * Holds logic of finding the next cluster by lookup up the appropriate entry in
 * the file allocation table.
 *
 * Usage:
 * for(cluster_it_init(ci); cluster_it_valid(ci); cluster_it_next(ci)) {
 *   use(ci->cluster_idx)
 * }
 */
struct cluster_it {
  struct fat32_fs *f;
  uint32_t cluster_idx;
  bool valid;
};

static inline void cluster_it_init(struct cluster_it *ci, struct fat32_fs *f, uint32_t start_cluster)
{
  ci->f = f;
  ci->cluster_idx = start_cluster;
  ci->valid = true;
}

static inline bool cluster_it_valid(struct cluster_it *ci)
{
  return ci->valid;
}

static inline void cluster_it_next(struct cluster_it *ci)
{
  ci->cluster_idx = fat32_get_next_cluster_num(ci->f, ci->cluster_idx);
  if (ci->cluster_idx == FAT32_ENTRY_END_OF_CHAIN ||
      ci->cluster_idx == FAT32_ENTRY_END_OF_CHAIN_ALT)
    ci->valid = false;
}

/*
 * cl_sector_iter - cluster sector iterator.
 * Encapsulates logic of iterating over sectors of
 * a single cluster.
 *
 * Usage:
 * for(cluster_it_init(ci); cluster_it_valid(ci); cluster_it_next(ci)) {
 *   use(ci->cluster_idx)
 * }
 */
struct cl_sector_iter {
  struct fat32_fs *f;
  uint32_t cluster_idx;
  uint64_t sector_idx;
  uint64_t end_sector_idx;
};

static inline void cl_sector_iter_init(struct cl_sector_iter *csi, struct fat32_fs *f, uint32_t cluster_idx)
{
  /* sector from where actual data clusters start */
  uint64_t first_data_sector_idx;

  /* index of a sector from start of first data cluster */
  uint64_t data_sector_idx;

  first_data_sector_idx = fat32_get_data_start_sector(f);
  data_sector_idx = (cluster_idx - FAT32_DATA_FIRST_CLUSTER) * fat32_get_sectors_per_cluster(f);

  csi->f = f;
  csi->cluster_idx = cluster_idx;
  csi->sector_idx = first_data_sector_idx + data_sector_idx;
  csi->end_sector_idx = csi->sector_idx + fat32_get_sectors_per_cluster(f);
}

static inline bool cl_sector_iter_valid(struct cl_sector_iter *csi)
{
  return csi->sector_idx < csi->end_sector_idx;
}

static inline void cl_sector_iter_next(struct cl_sector_iter *csi)
{
  csi->sector_idx++;
}

typedef enum {
  FAT_DIR_ITER_CONTINUE = 0,
  FAT_DIR_ITER_STOP = 1
} fat_dir_iter_status_t;

static int fat32_iterate_directory(
  struct fat32_fs *f,
  uint32_t dir_start_cluster,
  fat_dir_iter_status_t(*cb)(struct fat_dentry *, struct vfat_lfn_entry *, void *),
  void *cb_arg)
{
  int err = 0;
  struct cluster_it ci;
  struct cl_sector_iter csi;
  char buf[512] ALIGNED(4);
  struct fat_dentry *d, *d_end;
  struct vfat_lfn_entry *first_le;
  uint32_t sector_size;

  d = NULL;
  sector_size = fat32_get_logical_sector_size(f);

  cluster_it_init(&ci, f, dir_start_cluster);
  for(; cluster_it_valid(&ci); cluster_it_next(&ci)) {
    cl_sector_iter_init(&csi, f, ci.cluster_idx);
    for(; cl_sector_iter_valid(&csi); cl_sector_iter_next(&csi)) {
      printf("fat32_iterate_directory: reading dir sector: sector: %lld\r\n", csi.sector_idx);
      err = f->bdev->ops.read(f->bdev, buf, sizeof(buf), csi.sector_idx, 1);
      if (err < 0) {
        printf("fat32_ls: failed to read root dir cluster, err = %d\r\n", err);
        goto stop_iter;
      }

      d = (struct fat_dentry *)buf;
      d_end = (struct fat_dentry *)(buf + sector_size);

      /* print first directory */
      first_le = fat32_dentry_is_vfat_lfn(d) ? (struct vfat_lfn_entry *)d : 0;
      if (!first_le && fat32_dentry_is_valid(d)) {
        if (cb(d, first_le, cb_arg) == FAT_DIR_ITER_STOP)
          goto stop_iter;
      }

      while(1) {
        /* next directory */
        d = fat32_dentry_next(d, d_end, &first_le);

        /* no more entries */
        if (!d)
          goto stop_iter;

        if (IS_ERR(d)) {
          printf("fat32_ls: failed to get next dentry, err = %d\r\n",(uint32_t)PTR_ERR(d));
          err = PTR_ERR(d);
          goto stop_iter;
        }

        /*
         * iterated to the end of sector
         * proceed to next cluster
         */
        if (d == d_end)
          break;

        if (cb(d, first_le, cb_arg) == FAT_DIR_ITER_STOP)
          goto stop_iter;

        first_le = 0;
      }
    }
    if (!d) {
      /* EOF reached */
      goto stop_iter;
    }
  }
stop_iter:
  return err;
}

struct fat32_compare_filename_arg {
  struct fat32_fs *f;
  const char *filename;
  int err;
  struct fat_dentry dentry;
};

fat_dir_iter_status_t fat32_compare_filename_cb(struct fat_dentry *d, struct vfat_lfn_entry *le, void *cb_arg)
{
  char namebuf[128];
  struct fat32_compare_filename_arg *arg = cb_arg;

  fat32_dentry_get_filename(d, le, namebuf, sizeof(namebuf));
  if (strcmp(namebuf, arg->filename) == 0) {
    arg->err = ERR_OK;
    return FAT_DIR_ITER_STOP;
  }

  return FAT_DIR_ITER_CONTINUE;
}

int fat32_lookup(struct fat32_fs *f, const char *filepath, struct fat_dentry *out_dentry)
{
  int err;
  int n;
  const char *p;
  uint32_t dir_start_cluster_idx;
  char namebuf[128];
  struct fat32_compare_filename_arg arg;
  volatile int xx = 1;
  while(xx);

  arg.f = f;

  p = filepath;
  if (!*p || *p != '/') {
    printf("fat32_lookup: invalid argument: filepath: %s\r\n", filepath);
    return -1;
  }
  p++;
  dir_start_cluster_idx = fat32_get_root_dir_cluster(f);

  while(1) {
    n = filepath_get_next_element(&p);
    if (!n)
      break;
    strncpy(namebuf, p - n, n);
    namebuf[n] = 0;
    printf("fat32_lookup: %s\r\n", namebuf);
    if (*p == '/')
      p++;

    arg.err = ERR_NOT_FOUND;
    arg.filename = namebuf;

    err = fat32_iterate_directory(f, dir_start_cluster_idx, fat32_compare_filename_cb, &arg);
    if (err) {
      printf("fat32_lookup: failed to iterate directory entries\r\n");
      return err;
    }
    if (arg.err != ERR_OK) {
      if (arg.err != ERR_NOT_FOUND)
        printf("fat32_lookup: unexpected error during directory iteration: %d\r\n", arg.err);
      return err;
    }
    dir_start_cluster_idx = fat32_dentry_get_cluster(&arg.dentry);
  }
  return 0;
}


int fat32_ls(struct fat32_fs *f, const char *dirpath)
{
  char buf[512];
  int err;
  uint32_t cluster;
  uint32_t sectors_per_cluster;
  struct fat_dentry *d, *d_end;
  struct vfat_lfn_entry *first_le;
  uint64_t data_start_sector;
  uint64_t sector;
  uint32_t sector_size;

  d = NULL;
  data_start_sector = fat32_get_data_start_sector(f);
  sector_size = fat32_get_logical_sector_size(f);

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
      first_le = fat32_dentry_is_vfat_lfn(d) ? (struct vfat_lfn_entry *)d : 0;
      if (!first_le && fat32_dentry_is_valid(d))
        fat32_dentry_print(d, first_le);

      while(1) {
        /* next directory */
        d = fat32_dentry_next(d, d_end, &first_le);
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

        fat32_dentry_print(d, first_le);
        first_le = 0;
      }
    }
    if (!d) {
      /* EOF reached */
      break;
    }
    cluster = fat32_get_next_cluster_num(f, cluster);
  } while(cluster != FAT32_ENTRY_END_OF_CHAIN && cluster != FAT32_ENTRY_END_OF_CHAIN_ALT);
end:
  return 0;
}

int fat32_dump_file_cluster_chain(struct fat32_fs *f, const char *filename)
{
  return 0;
}
