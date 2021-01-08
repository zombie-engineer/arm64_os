#pragma once
#include <block_device.h>
#include <compiler.h>
#include <mem_access.h>

struct fat_dir_entry {
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

STRICT_SIZE(struct fat_dir_entry, 32);

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
  uint16_t max_root_entries;
  uint16_t total_sectors;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat;
} PACKED;

STRICT_SIZE(struct fat_dos20_bpb, 13);

/* FAT32 Extended BIOS parameter block (EBPB) */
struct fat32_ebpb {
  struct fat_dos20_bpb bpb;
  char dos331_bpb[24 - sizeof(struct fat_dos20_bpb)];
  uint32_t sectors_per_fat;
  char drive_desc[2];
  char version[2];
  uint32_t root_cluster_number;
  uint16_t fs_info_sector;
  uint16_t fat_cpy_sector;
  char res[12];
  char misc_bpb[3];
  char volume_id[4];
  char volume_label[11];
  char fs_type_name[12];
} PACKED;

STRICT_SIZE(struct fat32_ebpb, 82);

struct fat32_boot_sector {
  char jmp[3];
  char oem_name[8];
  struct fat32_ebpb ebpb;
  char bootcode[415];
  char drive_number;
  /* 55 aa */
  char signature[2];
} PACKED;

struct fat32_fs {
  struct block_device *bdev;
  struct fat32_boot_sector *boot_sector;
};

static inline uint32_t fat32_get_root_cluster_num(struct fat32_fs *f)
{
  return get_unaligned_32_le(&f->boot_sector->ebpb.root_cluster_number);
}

static inline uint32_t fat32_get_bytes_per_sector(struct fat32_fs *f)
{
  return get_unaligned_16_le(&f->boot_sector->ebpb.bpb.bytes_per_sector);
}

static inline uint32_t fat32_get_sectors_per_cluster(struct fat32_fs *f)
{
  return f->boot_sector->ebpb.bpb.sectors_per_cluster;
}

static inline uint32_t fat32_get_bytes_per_cluster(struct fat32_fs *f)
{
  return fat32_get_bytes_per_sector(f) * fat32_get_sectors_per_cluster(f);
}

static inline uint32_t fat32_get_sectors_per_fat(struct fat32_fs *f)
{
  return get_unaligned_32_le(&f->boot_sector->ebpb.bpb.sectors_per_fat);
}

static inline uint32_t fat32_get_num_root_entries(struct fat32_fs *f)
{
  return get_unaligned_32_le(&f->boot_sector->ebpb.bpb.sectors_per_fat);
}



int fat32_open(struct block_device *bdev, struct fat32_fs *f);

void fat32_summary(struct fat32_fs *f);


