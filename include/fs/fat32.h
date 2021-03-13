#pragma once
#include <block_device.h>
#include <compiler.h>
#include <mem_access.h>
#include <common.h>

#define FAT32_ENTRY_SIZE 4
#define FAT32_ENTRY_SIZE_LOG 2
#define FAT32_ENTRY_END_OF_CHAIN     0x0fffffff
#define FAT32_ENTRY_END_OF_CHAIN_ALT 0x0ffffff8
#define FAT32_ENTRY_FREE             0x00000000
#define FAT32_ENTRY_ID               0x0ffffff8

#define FAT_ENTRY_DELETED_MARK 0xe5
#define FAT32_DATA_FIRST_CLUSTER 2

#define FAT_FILE_ATTR_READONLY     (1<<0)
#define FAT_FILE_ATTR_HIDDEN       (1<<1)
#define FAT_FILE_ATTR_SYSTEM       (1<<2)
#define FAT_FILE_ATTR_VOLUME_LABEL (1<<3)
#define FAT_FILE_ATTR_SUBDIRECTORY (1<<4)
#define FAT_FILE_ATTR_ARCHIVE      (1<<5)
#define FAT_FILE_ATTR_DEVICE       (1<<6)
#define FAT_FILE_ATTR_RESERVED     (1<<7)

#define FAT_FILE_ATTR_VFAT_LFN 0xf

struct vfat_lfn_entry {
  char seq_num;
  char name_part_1[10];

  /* should be 0x0f */
  char file_attributes;

  /* should be 0x00 */
  char file_type;
  char checksum;
  char name_part_2[12];

  /* should be 0x00 */
  uint16_t first_cluster;

  char name_part_3[4];
} PACKED;

STRICT_SIZE(struct vfat_lfn_entry, 32);

struct fat_dentry {
  char filename_short[8];
  char extension[3];
  char file_attributes;
  /*  */
  char dos_attrs;
  char undelete_byte;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t cluster_addr_high;
  uint16_t last_modify_time;
  uint16_t last_modify_date;
  uint16_t cluster_addr_low;
  uint32_t file_size;
} PACKED;

static inline uint32_t fat32_dentry_get_cluster(struct fat_dentry *d)
{
  return (get_unaligned_16_le(&d->cluster_addr_high) << 16) |
    get_unaligned_16_le(&d->cluster_addr_low);
}

static inline uint32_t fat32_dentry_get_file_size(struct fat_dentry *d)
{
  return (get_unaligned_32_le(&d->file_size));
}

static inline bool fat32_dentry_is_valid(struct fat_dentry *d)
{
  return d->filename_short[0] &&
    fat32_dentry_get_file_size(d) &&
    fat32_dentry_get_cluster(d);
}

static inline bool fat32_dentry_is_deleted(struct fat_dentry *d)
{
  return ((char*)d)[0] == FAT_ENTRY_DELETED_MARK;
}

static inline bool fat32_dentry_is_vfat_lfn(struct fat_dentry *d)
{
  return d->file_attributes == FAT_FILE_ATTR_VFAT_LFN;
}

static inline bool fat32_dentry_is_end_mark(struct fat_dentry *d)
{
  char *ptr = (char *)d;
  char *end = ptr + sizeof(*d);
  while(ptr < end) {
    if (*ptr++)
      return false;
  }
  return true;
}

static inline struct fat_dentry *fat32_dentry_next(
  struct fat_dentry *d,
  struct fat_dentry *d_end,
  struct vfat_lfn_entry **first_lfn)
{
  while(d != d_end) {
    d++;
    if (fat32_dentry_is_vfat_lfn(d)) {
      if (!*first_lfn)
        *first_lfn = (struct vfat_lfn_entry *)d;
      continue;
    }

    if (fat32_dentry_is_deleted(d)) {
      *first_lfn = 0;
      continue;
    }

    if (fat32_dentry_is_valid(d))
      return d;

    if (fat32_dentry_is_end_mark(d))
      return 0;

    /* ?? invalid record */
    puts("fat32_dentry_next: found invalid dentry record:" __endline);
    hexdump_memory_ex("--", 32, d, sizeof(*d));
    *first_lfn = 0;
  }
  return d;
}

STRICT_SIZE(struct fat_dentry, 32);

struct fat32_fs_info {
  /* "RRaA" */
  char signature[4];
  char reserved[480];
  /* "rrAa" */
  char signature2[4];
  uint32_t last_known_free_clusters;
  uint32_t last_allocated_cluster;
  char reserved2[12];
  /* 00 00 55 AA */
  char signature3[4];
} PACKED;

STRICT_SIZE(struct fat32_fs_info, 512);

struct fat_dos20_bpb {
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t num_reserved_sectors;
  uint8_t num_fats;
  /* 0 for FAT32 */
  uint16_t max_root_entries;
  /* 0 for FAT32 */
  uint16_t total_sectors;
  uint8_t media_descriptor;
  /* 0 for FAT32 */
  uint16_t sectors_per_fat;
} PACKED;

STRICT_SIZE(struct fat_dos20_bpb, 13);

struct fat_dos331_bpb {
  struct fat_dos20_bpb bpb_dos20;
  uint16_t sectors_per_track;
  uint16_t heads_per_disk;
  uint32_t num_hidden_sectors;
  uint32_t total_logical_sectors;
} PACKED;

STRICT_SIZE(struct fat_dos331_bpb, 25);

/* FAT32 Extended BIOS parameter block (EBPB) */
struct fat32_ebpb {
  struct fat_dos331_bpb bpb_dos331;
  uint32_t sectors_per_fat;
  char drive_desc[2];
  char version[2];
  uint32_t root_dir_cluster;
  uint16_t fs_info_sector;
  uint16_t fat_cpy_sector;
  char res[12];
  char misc_exfat;
  char misc_unused1;
  char misc_unused2;
  char volume_id[4];
  char volume_label[11];
  char fs_type_name[8];
} PACKED;

STRICT_SIZE(struct fat32_ebpb, 79);

struct fat32_boot_sector {
  char jmp[3];
  char oem_name[8];
  struct fat32_ebpb ebpb;
  char bootcode[419];
  char drive_number;
  /* 55 aa */
  char signature[2];
} PACKED;

STRICT_SIZE(struct fat32_boot_sector, 512);

struct fat32_fs {
  struct block_device *bdev;
  struct fat32_boot_sector *boot_sector;
};

static inline uint16_t fat32_get_num_reserved_sectors(struct fat32_fs *f)
{
  return get_unaligned_16_le(&f->boot_sector->ebpb.bpb_dos331.bpb_dos20.num_reserved_sectors);
}

static inline uint32_t fat32_get_root_dir_cluster(struct fat32_fs *f)
{
  return get_unaligned_32_le(&f->boot_sector->ebpb.root_dir_cluster);
}

static inline uint16_t fat32_get_num_root_entries(struct fat32_fs *f)
{
  return get_unaligned_16_le(&f->boot_sector->ebpb.bpb_dos331.bpb_dos20.max_root_entries);
}

static inline uint32_t fat32_get_logical_sector_size(struct fat32_fs *f)
{
  return get_unaligned_16_le(&f->boot_sector->ebpb.bpb_dos331.bpb_dos20.bytes_per_sector);
}

static inline uint32_t fat32_get_sectors_per_cluster(struct fat32_fs *f)
{
  return f->boot_sector->ebpb.bpb_dos331.bpb_dos20.sectors_per_cluster;
}

static inline uint32_t fat32_get_bytes_per_cluster(struct fat32_fs *f)
{
  return fat32_get_logical_sector_size(f) * fat32_get_sectors_per_cluster(f);
}

static inline uint16_t fat32_get_sectors_per_fat(struct fat32_fs *f)
{
  return get_unaligned_16_le(&f->boot_sector->ebpb.sectors_per_fat);
}

static inline uint8_t fat32_get_num_fats(struct fat32_fs *f)
{
  return f->boot_sector->ebpb.bpb_dos331.bpb_dos20.num_fats;
}

static inline uint32_t fat32_get_num_fat_sectors(struct fat32_fs *f)
{
  return fat32_get_num_fats(f) * fat32_get_sectors_per_fat(f);
}

static inline uint32_t fat32_get_num_root_dir_sectors(struct fat32_fs *f)
{
  return (fat32_get_num_root_entries(f) * sizeof(struct fat_dentry)) / fat32_get_logical_sector_size(f);
}

static inline uint64_t fat32_get_data_start_sector(struct fat32_fs *f)
{
  return fat32_get_num_reserved_sectors(f) + fat32_get_num_fat_sectors(f);
}

static inline uint32_t fat32_get_root_dir_sector(struct fat32_fs *f)
{
  return fat32_get_data_start_sector(f);
}

int fat32_open(struct block_device *bdev, struct fat32_fs *f);

void fat32_summary(struct fat32_fs *f);

int fat32_ls(struct fat32_fs *f, const char *dirpath);

int fat32_dump_file_cluster_chain(struct fat32_fs *f, const char *filename);

int fat32_lookup(struct fat32_fs *f, const char *filepath, struct fat_dentry *out_dentry);

int fat32_iterate_file_sectors(
  struct fat32_fs *f,
  struct fat_dentry *d,
  int (*cb)(uint32_t, uint64_t, uint32_t, void *),
  void *cb_arg);
