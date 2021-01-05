#pragma once

#define EMMC_BLKSIZECNT_GET_BLKSIZE(v)           BF_EXTRACT(v, 0 , 10)
#define EMMC_BLKSIZECNT_GET_BLKCNT(v)            BF_EXTRACT(v, 16, 16)
#define EMMC_BLKSIZECNT_CLR_SET_BLKSIZE(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 10)
#define EMMC_BLKSIZECNT_CLR_SET_BLKCNT(v, set)           BF_CLEAR_AND_SET(v, set, 16, 16)
#define EMMC_BLKSIZECNT_CLR_BLKSIZE(v)           BF_CLEAR(v, 0 , 10)
#define EMMC_BLKSIZECNT_CLR_BLKCNT(v)            BF_CLEAR(v, 16, 16)
#define EMMC_BLKSIZECNT_MASK_BLKSIZE             BF_MASK_AT_32(0, 10)
#define EMMC_BLKSIZECNT_MASK_BLKCNT              BF_MASK_AT_32(16, 16)
#define EMMC_BLKSIZECNT_SHIFT_BLKSIZE            0
#define EMMC_BLKSIZECNT_SHIFT_BLKCNT             16


static inline int emmc_blksizecnt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,BLKSIZE:%x,BLKCNT:%x",
    v,
    (int)EMMC_BLKSIZECNT_GET_BLKSIZE(v),
    (int)EMMC_BLKSIZECNT_GET_BLKCNT(v));
}
#define EMMC_ARG1_GET_ARGUMENT(v)                BF_EXTRACT(v, 0 , 32)
#define EMMC_ARG1_CLR_SET_ARGUMENT(v, set)               BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_ARG1_CLR_ARGUMENT(v)                BF_CLEAR(v, 0 , 32)
#define EMMC_ARG1_MASK_ARGUMENT                  BF_MASK_AT_32(0, 32)
#define EMMC_ARG1_SHIFT_ARGUMENT                 0


static inline int emmc_arg1_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,ARGUMENT:%x",
    v,
    (int)EMMC_ARG1_GET_ARGUMENT(v));
}
#define EMMC_CMDTM_GET_TM_BLKCNT_EN(v)           BF_EXTRACT(v, 1 , 1 )
#define EMMC_CMDTM_GET_TM_AUTOCMD_EN(v)          BF_EXTRACT(v, 2 , 2 )
#define EMMC_CMDTM_GET_TM_DAT_DIR(v)             BF_EXTRACT(v, 4 , 1 )
#define EMMC_CMDTM_GET_TM_MULTI_BLOCK(v)         BF_EXTRACT(v, 5 , 1 )
#define EMMC_CMDTM_GET_CMD_RSPNS_TYPE(v)         BF_EXTRACT(v, 16, 2 )
#define EMMC_CMDTM_GET_CMD_CRCCHK_EN(v)          BF_EXTRACT(v, 19, 1 )
#define EMMC_CMDTM_GET_CMD_IXCHK_EN(v)           BF_EXTRACT(v, 20, 1 )
#define EMMC_CMDTM_GET_CMD_ISDATA(v)             BF_EXTRACT(v, 21, 1 )
#define EMMC_CMDTM_GET_CMD_TYPE(v)               BF_EXTRACT(v, 22, 2 )
#define EMMC_CMDTM_GET_CMD_INDEX(v)              BF_EXTRACT(v, 24, 6 )
#define EMMC_CMDTM_CLR_SET_TM_BLKCNT_EN(v, set)          BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_CMDTM_CLR_SET_TM_AUTOCMD_EN(v, set)         BF_CLEAR_AND_SET(v, set, 2 , 2 )
#define EMMC_CMDTM_CLR_SET_TM_DAT_DIR(v, set)            BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_CMDTM_CLR_SET_TM_MULTI_BLOCK(v, set)        BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_CMDTM_CLR_SET_CMD_RSPNS_TYPE(v, set)        BF_CLEAR_AND_SET(v, set, 16, 2 )
#define EMMC_CMDTM_CLR_SET_CMD_CRCCHK_EN(v, set)         BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_CMDTM_CLR_SET_CMD_IXCHK_EN(v, set)          BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_CMDTM_CLR_SET_CMD_ISDATA(v, set)            BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_CMDTM_CLR_SET_CMD_TYPE(v, set)              BF_CLEAR_AND_SET(v, set, 22, 2 )
#define EMMC_CMDTM_CLR_SET_CMD_INDEX(v, set)             BF_CLEAR_AND_SET(v, set, 24, 6 )
#define EMMC_CMDTM_CLR_TM_BLKCNT_EN(v)           BF_CLEAR(v, 1 , 1 )
#define EMMC_CMDTM_CLR_TM_AUTOCMD_EN(v)          BF_CLEAR(v, 2 , 2 )
#define EMMC_CMDTM_CLR_TM_DAT_DIR(v)             BF_CLEAR(v, 4 , 1 )
#define EMMC_CMDTM_CLR_TM_MULTI_BLOCK(v)         BF_CLEAR(v, 5 , 1 )
#define EMMC_CMDTM_CLR_CMD_RSPNS_TYPE(v)         BF_CLEAR(v, 16, 2 )
#define EMMC_CMDTM_CLR_CMD_CRCCHK_EN(v)          BF_CLEAR(v, 19, 1 )
#define EMMC_CMDTM_CLR_CMD_IXCHK_EN(v)           BF_CLEAR(v, 20, 1 )
#define EMMC_CMDTM_CLR_CMD_ISDATA(v)             BF_CLEAR(v, 21, 1 )
#define EMMC_CMDTM_CLR_CMD_TYPE(v)               BF_CLEAR(v, 22, 2 )
#define EMMC_CMDTM_CLR_CMD_INDEX(v)              BF_CLEAR(v, 24, 6 )
#define EMMC_CMDTM_MASK_TM_BLKCNT_EN             BF_MASK_AT_32(1, 1)
#define EMMC_CMDTM_MASK_TM_AUTOCMD_EN            BF_MASK_AT_32(2, 2)
#define EMMC_CMDTM_MASK_TM_DAT_DIR               BF_MASK_AT_32(4, 1)
#define EMMC_CMDTM_MASK_TM_MULTI_BLOCK           BF_MASK_AT_32(5, 1)
#define EMMC_CMDTM_MASK_CMD_RSPNS_TYPE           BF_MASK_AT_32(16, 2)
#define EMMC_CMDTM_MASK_CMD_CRCCHK_EN            BF_MASK_AT_32(19, 1)
#define EMMC_CMDTM_MASK_CMD_IXCHK_EN             BF_MASK_AT_32(20, 1)
#define EMMC_CMDTM_MASK_CMD_ISDATA               BF_MASK_AT_32(21, 1)
#define EMMC_CMDTM_MASK_CMD_TYPE                 BF_MASK_AT_32(22, 2)
#define EMMC_CMDTM_MASK_CMD_INDEX                BF_MASK_AT_32(24, 6)
#define EMMC_CMDTM_SHIFT_TM_BLKCNT_EN            1
#define EMMC_CMDTM_SHIFT_TM_AUTOCMD_EN           2
#define EMMC_CMDTM_SHIFT_TM_DAT_DIR              4
#define EMMC_CMDTM_SHIFT_TM_MULTI_BLOCK          5
#define EMMC_CMDTM_SHIFT_CMD_RSPNS_TYPE          16
#define EMMC_CMDTM_SHIFT_CMD_CRCCHK_EN           19
#define EMMC_CMDTM_SHIFT_CMD_IXCHK_EN            20
#define EMMC_CMDTM_SHIFT_CMD_ISDATA              21
#define EMMC_CMDTM_SHIFT_CMD_TYPE                22
#define EMMC_CMDTM_SHIFT_CMD_INDEX               24


static inline int emmc_cmdtm_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,TM_BLKCNT_EN:%x,TM_AUTOCMD_EN:%x,TM_DAT_DIR:%x,TM_MULTI_BLOCK:%x,CMD_RSPNS_TYPE:%x,CMD_CRCCHK_EN:%x,CMD_IXCHK_EN:%x,CMD_ISDATA:%x,CMD_TYPE:%x,CMD_INDEX:%x",
    v,
    (int)EMMC_CMDTM_GET_TM_BLKCNT_EN(v),
    (int)EMMC_CMDTM_GET_TM_AUTOCMD_EN(v),
    (int)EMMC_CMDTM_GET_TM_DAT_DIR(v),
    (int)EMMC_CMDTM_GET_TM_MULTI_BLOCK(v),
    (int)EMMC_CMDTM_GET_CMD_RSPNS_TYPE(v),
    (int)EMMC_CMDTM_GET_CMD_CRCCHK_EN(v),
    (int)EMMC_CMDTM_GET_CMD_IXCHK_EN(v),
    (int)EMMC_CMDTM_GET_CMD_ISDATA(v),
    (int)EMMC_CMDTM_GET_CMD_TYPE(v),
    (int)EMMC_CMDTM_GET_CMD_INDEX(v));
}
#define EMMC_RESP0_GET_RESPONSE(v)               BF_EXTRACT(v, 0 , 32)
#define EMMC_RESP0_CLR_SET_RESPONSE(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_RESP0_CLR_RESPONSE(v)               BF_CLEAR(v, 0 , 32)
#define EMMC_RESP0_MASK_RESPONSE                 BF_MASK_AT_32(0, 32)
#define EMMC_RESP0_SHIFT_RESPONSE                0


static inline int emmc_resp0_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RESPONSE:%x",
    v,
    (int)EMMC_RESP0_GET_RESPONSE(v));
}
#define EMMC_RESP1_GET_RESPONSE(v)               BF_EXTRACT(v, 0 , 32)
#define EMMC_RESP1_CLR_SET_RESPONSE(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_RESP1_CLR_RESPONSE(v)               BF_CLEAR(v, 0 , 32)
#define EMMC_RESP1_MASK_RESPONSE                 BF_MASK_AT_32(0, 32)
#define EMMC_RESP1_SHIFT_RESPONSE                0


static inline int emmc_resp1_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RESPONSE:%x",
    v,
    (int)EMMC_RESP1_GET_RESPONSE(v));
}
#define EMMC_RESP2_GET_RESPONSE(v)               BF_EXTRACT(v, 0 , 32)
#define EMMC_RESP2_CLR_SET_RESPONSE(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_RESP2_CLR_RESPONSE(v)               BF_CLEAR(v, 0 , 32)
#define EMMC_RESP2_MASK_RESPONSE                 BF_MASK_AT_32(0, 32)
#define EMMC_RESP2_SHIFT_RESPONSE                0


static inline int emmc_resp2_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RESPONSE:%x",
    v,
    (int)EMMC_RESP2_GET_RESPONSE(v));
}
#define EMMC_RESP3_GET_RESPONSE(v)               BF_EXTRACT(v, 0 , 32)
#define EMMC_RESP3_CLR_SET_RESPONSE(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_RESP3_CLR_RESPONSE(v)               BF_CLEAR(v, 0 , 32)
#define EMMC_RESP3_MASK_RESPONSE                 BF_MASK_AT_32(0, 32)
#define EMMC_RESP3_SHIFT_RESPONSE                0


static inline int emmc_resp3_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RESPONSE:%x",
    v,
    (int)EMMC_RESP3_GET_RESPONSE(v));
}
#define EMMC_DATA_GET_DATA(v)                    BF_EXTRACT(v, 0 , 32)
#define EMMC_DATA_CLR_SET_DATA(v, set)                   BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_DATA_CLR_DATA(v)                    BF_CLEAR(v, 0 , 32)
#define EMMC_DATA_MASK_DATA                      BF_MASK_AT_32(0, 32)
#define EMMC_DATA_SHIFT_DATA                     0


static inline int emmc_data_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,DATA:%x",
    v,
    (int)EMMC_DATA_GET_DATA(v));
}
#define EMMC_STATUS_GET_CMD_INHIBIT(v)           BF_EXTRACT(v, 0 , 1 )
#define EMMC_STATUS_GET_DAT_INHIBIT(v)           BF_EXTRACT(v, 1 , 1 )
#define EMMC_STATUS_GET_DAT_ACTIVE(v)            BF_EXTRACT(v, 2 , 1 )
#define EMMC_STATUS_GET_WRITE_TRANSFER(v)        BF_EXTRACT(v, 8 , 1 )
#define EMMC_STATUS_GET_READ_TRANSFER(v)         BF_EXTRACT(v, 9 , 1 )
#define EMMC_STATUS_GET_DAT_LEVEL0(v)            BF_EXTRACT(v, 20, 4 )
#define EMMC_STATUS_GET_CMD_LEVEL(v)             BF_EXTRACT(v, 24, 1 )
#define EMMC_STATUS_GET_DAT_LEVEL1(v)            BF_EXTRACT(v, 25, 4 )
#define EMMC_STATUS_CLR_SET_CMD_INHIBIT(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_STATUS_CLR_SET_DAT_INHIBIT(v, set)          BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_STATUS_CLR_SET_DAT_ACTIVE(v, set)           BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_STATUS_CLR_SET_WRITE_TRANSFER(v, set)       BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define EMMC_STATUS_CLR_SET_READ_TRANSFER(v, set)        BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define EMMC_STATUS_CLR_SET_DAT_LEVEL0(v, set)           BF_CLEAR_AND_SET(v, set, 20, 4 )
#define EMMC_STATUS_CLR_SET_CMD_LEVEL(v, set)            BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_STATUS_CLR_SET_DAT_LEVEL1(v, set)           BF_CLEAR_AND_SET(v, set, 25, 4 )
#define EMMC_STATUS_CLR_CMD_INHIBIT(v)           BF_CLEAR(v, 0 , 1 )
#define EMMC_STATUS_CLR_DAT_INHIBIT(v)           BF_CLEAR(v, 1 , 1 )
#define EMMC_STATUS_CLR_DAT_ACTIVE(v)            BF_CLEAR(v, 2 , 1 )
#define EMMC_STATUS_CLR_WRITE_TRANSFER(v)        BF_CLEAR(v, 8 , 1 )
#define EMMC_STATUS_CLR_READ_TRANSFER(v)         BF_CLEAR(v, 9 , 1 )
#define EMMC_STATUS_CLR_DAT_LEVEL0(v)            BF_CLEAR(v, 20, 4 )
#define EMMC_STATUS_CLR_CMD_LEVEL(v)             BF_CLEAR(v, 24, 1 )
#define EMMC_STATUS_CLR_DAT_LEVEL1(v)            BF_CLEAR(v, 25, 4 )
#define EMMC_STATUS_MASK_CMD_INHIBIT             BF_MASK_AT_32(0, 1)
#define EMMC_STATUS_MASK_DAT_INHIBIT             BF_MASK_AT_32(1, 1)
#define EMMC_STATUS_MASK_DAT_ACTIVE              BF_MASK_AT_32(2, 1)
#define EMMC_STATUS_MASK_WRITE_TRANSFER          BF_MASK_AT_32(8, 1)
#define EMMC_STATUS_MASK_READ_TRANSFER           BF_MASK_AT_32(9, 1)
#define EMMC_STATUS_MASK_DAT_LEVEL0              BF_MASK_AT_32(20, 4)
#define EMMC_STATUS_MASK_CMD_LEVEL               BF_MASK_AT_32(24, 1)
#define EMMC_STATUS_MASK_DAT_LEVEL1              BF_MASK_AT_32(25, 4)
#define EMMC_STATUS_SHIFT_CMD_INHIBIT            0
#define EMMC_STATUS_SHIFT_DAT_INHIBIT            1
#define EMMC_STATUS_SHIFT_DAT_ACTIVE             2
#define EMMC_STATUS_SHIFT_WRITE_TRANSFER         8
#define EMMC_STATUS_SHIFT_READ_TRANSFER          9
#define EMMC_STATUS_SHIFT_DAT_LEVEL0             20
#define EMMC_STATUS_SHIFT_CMD_LEVEL              24
#define EMMC_STATUS_SHIFT_DAT_LEVEL1             25


static inline int emmc_status_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CMD_INHIBIT:%x,DAT_INHIBIT:%x,DAT_ACTIVE:%x,WRITE_TRANSFER:%x,READ_TRANSFER:%x,DAT_LEVEL0:%x,CMD_LEVEL:%x,DAT_LEVEL1:%x",
    v,
    (int)EMMC_STATUS_GET_CMD_INHIBIT(v),
    (int)EMMC_STATUS_GET_DAT_INHIBIT(v),
    (int)EMMC_STATUS_GET_DAT_ACTIVE(v),
    (int)EMMC_STATUS_GET_WRITE_TRANSFER(v),
    (int)EMMC_STATUS_GET_READ_TRANSFER(v),
    (int)EMMC_STATUS_GET_DAT_LEVEL0(v),
    (int)EMMC_STATUS_GET_CMD_LEVEL(v),
    (int)EMMC_STATUS_GET_DAT_LEVEL1(v));
}
#define EMMC_CONTROL0_GET_HCTL_DWIDTH(v)         BF_EXTRACT(v, 1 , 1 )
#define EMMC_CONTROL0_GET_HCTL_HS_EN(v)          BF_EXTRACT(v, 2 , 1 )
#define EMMC_CONTROL0_GET_HCTL_8BIT(v)           BF_EXTRACT(v, 5 , 1 )
#define EMMC_CONTROL0_GET_GAP_STOP(v)            BF_EXTRACT(v, 16, 1 )
#define EMMC_CONTROL0_GET_GAP_RESTART(v)         BF_EXTRACT(v, 17, 1 )
#define EMMC_CONTROL0_GET_READWAIT_EN(v)         BF_EXTRACT(v, 18, 1 )
#define EMMC_CONTROL0_GET_GAP_IEN(v)             BF_EXTRACT(v, 19, 1 )
#define EMMC_CONTROL0_GET_SPI_MODE(v)            BF_EXTRACT(v, 20, 1 )
#define EMMC_CONTROL0_GET_BOOT_EN(v)             BF_EXTRACT(v, 21, 1 )
#define EMMC_CONTROL0_GET_ALT_BOOT_EN(v)         BF_EXTRACT(v, 22, 1 )
#define EMMC_CONTROL0_CLR_SET_HCTL_DWIDTH(v, set)        BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_CONTROL0_CLR_SET_HCTL_HS_EN(v, set)         BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_CONTROL0_CLR_SET_HCTL_8BIT(v, set)          BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_CONTROL0_CLR_SET_GAP_STOP(v, set)           BF_CLEAR_AND_SET(v, set, 16, 1 )
#define EMMC_CONTROL0_CLR_SET_GAP_RESTART(v, set)        BF_CLEAR_AND_SET(v, set, 17, 1 )
#define EMMC_CONTROL0_CLR_SET_READWAIT_EN(v, set)        BF_CLEAR_AND_SET(v, set, 18, 1 )
#define EMMC_CONTROL0_CLR_SET_GAP_IEN(v, set)            BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_CONTROL0_CLR_SET_SPI_MODE(v, set)           BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_CONTROL0_CLR_SET_BOOT_EN(v, set)            BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_CONTROL0_CLR_SET_ALT_BOOT_EN(v, set)        BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_CONTROL0_CLR_HCTL_DWIDTH(v)         BF_CLEAR(v, 1 , 1 )
#define EMMC_CONTROL0_CLR_HCTL_HS_EN(v)          BF_CLEAR(v, 2 , 1 )
#define EMMC_CONTROL0_CLR_HCTL_8BIT(v)           BF_CLEAR(v, 5 , 1 )
#define EMMC_CONTROL0_CLR_GAP_STOP(v)            BF_CLEAR(v, 16, 1 )
#define EMMC_CONTROL0_CLR_GAP_RESTART(v)         BF_CLEAR(v, 17, 1 )
#define EMMC_CONTROL0_CLR_READWAIT_EN(v)         BF_CLEAR(v, 18, 1 )
#define EMMC_CONTROL0_CLR_GAP_IEN(v)             BF_CLEAR(v, 19, 1 )
#define EMMC_CONTROL0_CLR_SPI_MODE(v)            BF_CLEAR(v, 20, 1 )
#define EMMC_CONTROL0_CLR_BOOT_EN(v)             BF_CLEAR(v, 21, 1 )
#define EMMC_CONTROL0_CLR_ALT_BOOT_EN(v)         BF_CLEAR(v, 22, 1 )
#define EMMC_CONTROL0_MASK_HCTL_DWIDTH           BF_MASK_AT_32(1, 1)
#define EMMC_CONTROL0_MASK_HCTL_HS_EN            BF_MASK_AT_32(2, 1)
#define EMMC_CONTROL0_MASK_HCTL_8BIT             BF_MASK_AT_32(5, 1)
#define EMMC_CONTROL0_MASK_GAP_STOP              BF_MASK_AT_32(16, 1)
#define EMMC_CONTROL0_MASK_GAP_RESTART           BF_MASK_AT_32(17, 1)
#define EMMC_CONTROL0_MASK_READWAIT_EN           BF_MASK_AT_32(18, 1)
#define EMMC_CONTROL0_MASK_GAP_IEN               BF_MASK_AT_32(19, 1)
#define EMMC_CONTROL0_MASK_SPI_MODE              BF_MASK_AT_32(20, 1)
#define EMMC_CONTROL0_MASK_BOOT_EN               BF_MASK_AT_32(21, 1)
#define EMMC_CONTROL0_MASK_ALT_BOOT_EN           BF_MASK_AT_32(22, 1)
#define EMMC_CONTROL0_SHIFT_HCTL_DWIDTH          1
#define EMMC_CONTROL0_SHIFT_HCTL_HS_EN           2
#define EMMC_CONTROL0_SHIFT_HCTL_8BIT            5
#define EMMC_CONTROL0_SHIFT_GAP_STOP             16
#define EMMC_CONTROL0_SHIFT_GAP_RESTART          17
#define EMMC_CONTROL0_SHIFT_READWAIT_EN          18
#define EMMC_CONTROL0_SHIFT_GAP_IEN              19
#define EMMC_CONTROL0_SHIFT_SPI_MODE             20
#define EMMC_CONTROL0_SHIFT_BOOT_EN              21
#define EMMC_CONTROL0_SHIFT_ALT_BOOT_EN          22

static inline int emmc_control0_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_CONTROL0_GET_HCTL_DWIDTH(v)) {
    n += snprintf(buf + n, bufsz - n, "%sHCTL_DWIDTH", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_HCTL_HS_EN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sHCTL_HS_EN", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_HCTL_8BIT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sHCTL_8BIT", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_GAP_STOP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sGAP_STOP", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_GAP_RESTART(v)) {
    n += snprintf(buf + n, bufsz - n, "%sGAP_RESTART", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_READWAIT_EN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sREADWAIT_EN", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_GAP_IEN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sGAP_IEN", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_SPI_MODE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sSPI_MODE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_BOOT_EN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBOOT_EN", first ? "" : ",");
    first = 0;
  }
  if (EMMC_CONTROL0_GET_ALT_BOOT_EN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sALT_BOOT_EN", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_control0_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,HCTL_DWIDTH:%x,HCTL_HS_EN:%x,HCTL_8BIT:%x,GAP_STOP:%x,GAP_RESTART:%x,READWAIT_EN:%x,GAP_IEN:%x,SPI_MODE:%x,BOOT_EN:%x,ALT_BOOT_EN:%x",
    v,
    (int)EMMC_CONTROL0_GET_HCTL_DWIDTH(v),
    (int)EMMC_CONTROL0_GET_HCTL_HS_EN(v),
    (int)EMMC_CONTROL0_GET_HCTL_8BIT(v),
    (int)EMMC_CONTROL0_GET_GAP_STOP(v),
    (int)EMMC_CONTROL0_GET_GAP_RESTART(v),
    (int)EMMC_CONTROL0_GET_READWAIT_EN(v),
    (int)EMMC_CONTROL0_GET_GAP_IEN(v),
    (int)EMMC_CONTROL0_GET_SPI_MODE(v),
    (int)EMMC_CONTROL0_GET_BOOT_EN(v),
    (int)EMMC_CONTROL0_GET_ALT_BOOT_EN(v));
}
#define EMMC_CONTROL1_GET_CLK_INTLEN(v)          BF_EXTRACT(v, 0 , 1 )
#define EMMC_CONTROL1_GET_CLK_STABLE(v)          BF_EXTRACT(v, 1 , 1 )
#define EMMC_CONTROL1_GET_CLK_EN(v)              BF_EXTRACT(v, 2 , 1 )
#define EMMC_CONTROL1_GET_CLK_GENSEL(v)          BF_EXTRACT(v, 5 , 1 )
#define EMMC_CONTROL1_GET_CLK_FREQ_MS2(v)        BF_EXTRACT(v, 6 , 2 )
#define EMMC_CONTROL1_GET_CLK_FREQ8(v)           BF_EXTRACT(v, 8 , 6 )
#define EMMC_CONTROL1_GET_DATA_TOUNIT(v)         BF_EXTRACT(v, 16, 4 )
#define EMMC_CONTROL1_GET_SRST_HC(v)             BF_EXTRACT(v, 24, 1 )
#define EMMC_CONTROL1_GET_SRST_CMD(v)            BF_EXTRACT(v, 25, 1 )
#define EMMC_CONTROL1_GET_SRST_DATA(v)           BF_EXTRACT(v, 26, 1 )
#define EMMC_CONTROL1_CLR_SET_CLK_INTLEN(v, set)         BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_CONTROL1_CLR_SET_CLK_STABLE(v, set)         BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_CONTROL1_CLR_SET_CLK_EN(v, set)             BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_CONTROL1_CLR_SET_CLK_GENSEL(v, set)         BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_CONTROL1_CLR_SET_CLK_FREQ_MS2(v, set)       BF_CLEAR_AND_SET(v, set, 6 , 2 )
#define EMMC_CONTROL1_CLR_SET_CLK_FREQ8(v, set)          BF_CLEAR_AND_SET(v, set, 8 , 6 )
#define EMMC_CONTROL1_CLR_SET_DATA_TOUNIT(v, set)        BF_CLEAR_AND_SET(v, set, 16, 4 )
#define EMMC_CONTROL1_CLR_SET_SRST_HC(v, set)            BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_CONTROL1_CLR_SET_SRST_CMD(v, set)           BF_CLEAR_AND_SET(v, set, 25, 1 )
#define EMMC_CONTROL1_CLR_SET_SRST_DATA(v, set)          BF_CLEAR_AND_SET(v, set, 26, 1 )
#define EMMC_CONTROL1_CLR_CLK_INTLEN(v)          BF_CLEAR(v, 0 , 1 )
#define EMMC_CONTROL1_CLR_CLK_STABLE(v)          BF_CLEAR(v, 1 , 1 )
#define EMMC_CONTROL1_CLR_CLK_EN(v)              BF_CLEAR(v, 2 , 1 )
#define EMMC_CONTROL1_CLR_CLK_GENSEL(v)          BF_CLEAR(v, 5 , 1 )
#define EMMC_CONTROL1_CLR_CLK_FREQ_MS2(v)        BF_CLEAR(v, 6 , 2 )
#define EMMC_CONTROL1_CLR_CLK_FREQ8(v)           BF_CLEAR(v, 8 , 6 )
#define EMMC_CONTROL1_CLR_DATA_TOUNIT(v)         BF_CLEAR(v, 16, 4 )
#define EMMC_CONTROL1_CLR_SRST_HC(v)             BF_CLEAR(v, 24, 1 )
#define EMMC_CONTROL1_CLR_SRST_CMD(v)            BF_CLEAR(v, 25, 1 )
#define EMMC_CONTROL1_CLR_SRST_DATA(v)           BF_CLEAR(v, 26, 1 )
#define EMMC_CONTROL1_MASK_CLK_INTLEN            BF_MASK_AT_32(0, 1)
#define EMMC_CONTROL1_MASK_CLK_STABLE            BF_MASK_AT_32(1, 1)
#define EMMC_CONTROL1_MASK_CLK_EN                BF_MASK_AT_32(2, 1)
#define EMMC_CONTROL1_MASK_CLK_GENSEL            BF_MASK_AT_32(5, 1)
#define EMMC_CONTROL1_MASK_CLK_FREQ_MS2          BF_MASK_AT_32(6, 2)
#define EMMC_CONTROL1_MASK_CLK_FREQ8             BF_MASK_AT_32(8, 6)
#define EMMC_CONTROL1_MASK_DATA_TOUNIT           BF_MASK_AT_32(16, 4)
#define EMMC_CONTROL1_MASK_SRST_HC               BF_MASK_AT_32(24, 1)
#define EMMC_CONTROL1_MASK_SRST_CMD              BF_MASK_AT_32(25, 1)
#define EMMC_CONTROL1_MASK_SRST_DATA             BF_MASK_AT_32(26, 1)
#define EMMC_CONTROL1_SHIFT_CLK_INTLEN           0
#define EMMC_CONTROL1_SHIFT_CLK_STABLE           1
#define EMMC_CONTROL1_SHIFT_CLK_EN               2
#define EMMC_CONTROL1_SHIFT_CLK_GENSEL           5
#define EMMC_CONTROL1_SHIFT_CLK_FREQ_MS2         6
#define EMMC_CONTROL1_SHIFT_CLK_FREQ8            8
#define EMMC_CONTROL1_SHIFT_DATA_TOUNIT          16
#define EMMC_CONTROL1_SHIFT_SRST_HC              24
#define EMMC_CONTROL1_SHIFT_SRST_CMD             25
#define EMMC_CONTROL1_SHIFT_SRST_DATA            26


static inline int emmc_control1_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CLK_INTLEN:%x,CLK_STABLE:%x,CLK_EN:%x,CLK_GENSEL:%x,CLK_FREQ_MS2:%x,CLK_FREQ8:%x,DATA_TOUNIT:%x,SRST_HC:%x,SRST_CMD:%x,SRST_DATA:%x",
    v,
    (int)EMMC_CONTROL1_GET_CLK_INTLEN(v),
    (int)EMMC_CONTROL1_GET_CLK_STABLE(v),
    (int)EMMC_CONTROL1_GET_CLK_EN(v),
    (int)EMMC_CONTROL1_GET_CLK_GENSEL(v),
    (int)EMMC_CONTROL1_GET_CLK_FREQ_MS2(v),
    (int)EMMC_CONTROL1_GET_CLK_FREQ8(v),
    (int)EMMC_CONTROL1_GET_DATA_TOUNIT(v),
    (int)EMMC_CONTROL1_GET_SRST_HC(v),
    (int)EMMC_CONTROL1_GET_SRST_CMD(v),
    (int)EMMC_CONTROL1_GET_SRST_DATA(v));
}
#define EMMC_INTERRUPT_GET_CMD_DONE(v)           BF_EXTRACT(v, 0 , 1 )
#define EMMC_INTERRUPT_GET_DATA_DONE(v)          BF_EXTRACT(v, 1 , 1 )
#define EMMC_INTERRUPT_GET_BLOCK_GAP(v)          BF_EXTRACT(v, 2 , 1 )
#define EMMC_INTERRUPT_GET_WRITE_RDY(v)          BF_EXTRACT(v, 4 , 1 )
#define EMMC_INTERRUPT_GET_READ_RDY(v)           BF_EXTRACT(v, 5 , 1 )
#define EMMC_INTERRUPT_GET_CARD(v)               BF_EXTRACT(v, 8 , 1 )
#define EMMC_INTERRUPT_GET_RETUNE(v)             BF_EXTRACT(v, 12, 1 )
#define EMMC_INTERRUPT_GET_BOOTACK(v)            BF_EXTRACT(v, 13, 1 )
#define EMMC_INTERRUPT_GET_ENDBOOT(v)            BF_EXTRACT(v, 14, 1 )
#define EMMC_INTERRUPT_GET_ERR(v)                BF_EXTRACT(v, 15, 1 )
#define EMMC_INTERRUPT_GET_CTO_ERR(v)            BF_EXTRACT(v, 16, 1 )
#define EMMC_INTERRUPT_GET_CCRC_ERR(v)           BF_EXTRACT(v, 17, 1 )
#define EMMC_INTERRUPT_GET_CEND_ERR(v)           BF_EXTRACT(v, 18, 1 )
#define EMMC_INTERRUPT_GET_CBAD_ERR(v)           BF_EXTRACT(v, 19, 1 )
#define EMMC_INTERRUPT_GET_DTO_ERR(v)            BF_EXTRACT(v, 20, 1 )
#define EMMC_INTERRUPT_GET_DCRC_ERR(v)           BF_EXTRACT(v, 21, 1 )
#define EMMC_INTERRUPT_GET_DEND_ERR(v)           BF_EXTRACT(v, 22, 1 )
#define EMMC_INTERRUPT_GET_ACMD_ERR(v)           BF_EXTRACT(v, 24, 1 )
#define EMMC_INTERRUPT_CLR_SET_CMD_DONE(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_INTERRUPT_CLR_SET_DATA_DONE(v, set)         BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_INTERRUPT_CLR_SET_BLOCK_GAP(v, set)         BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_INTERRUPT_CLR_SET_WRITE_RDY(v, set)         BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_INTERRUPT_CLR_SET_READ_RDY(v, set)          BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_INTERRUPT_CLR_SET_CARD(v, set)              BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define EMMC_INTERRUPT_CLR_SET_RETUNE(v, set)            BF_CLEAR_AND_SET(v, set, 12, 1 )
#define EMMC_INTERRUPT_CLR_SET_BOOTACK(v, set)           BF_CLEAR_AND_SET(v, set, 13, 1 )
#define EMMC_INTERRUPT_CLR_SET_ENDBOOT(v, set)           BF_CLEAR_AND_SET(v, set, 14, 1 )
#define EMMC_INTERRUPT_CLR_SET_ERR(v, set)               BF_CLEAR_AND_SET(v, set, 15, 1 )
#define EMMC_INTERRUPT_CLR_SET_CTO_ERR(v, set)           BF_CLEAR_AND_SET(v, set, 16, 1 )
#define EMMC_INTERRUPT_CLR_SET_CCRC_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 17, 1 )
#define EMMC_INTERRUPT_CLR_SET_CEND_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 18, 1 )
#define EMMC_INTERRUPT_CLR_SET_CBAD_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_INTERRUPT_CLR_SET_DTO_ERR(v, set)           BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_INTERRUPT_CLR_SET_DCRC_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_INTERRUPT_CLR_SET_DEND_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_INTERRUPT_CLR_SET_ACMD_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_INTERRUPT_CLR_CMD_DONE(v)           BF_CLEAR(v, 0 , 1 )
#define EMMC_INTERRUPT_CLR_DATA_DONE(v)          BF_CLEAR(v, 1 , 1 )
#define EMMC_INTERRUPT_CLR_BLOCK_GAP(v)          BF_CLEAR(v, 2 , 1 )
#define EMMC_INTERRUPT_CLR_WRITE_RDY(v)          BF_CLEAR(v, 4 , 1 )
#define EMMC_INTERRUPT_CLR_READ_RDY(v)           BF_CLEAR(v, 5 , 1 )
#define EMMC_INTERRUPT_CLR_CARD(v)               BF_CLEAR(v, 8 , 1 )
#define EMMC_INTERRUPT_CLR_RETUNE(v)             BF_CLEAR(v, 12, 1 )
#define EMMC_INTERRUPT_CLR_BOOTACK(v)            BF_CLEAR(v, 13, 1 )
#define EMMC_INTERRUPT_CLR_ENDBOOT(v)            BF_CLEAR(v, 14, 1 )
#define EMMC_INTERRUPT_CLR_ERR(v)                BF_CLEAR(v, 15, 1 )
#define EMMC_INTERRUPT_CLR_CTO_ERR(v)            BF_CLEAR(v, 16, 1 )
#define EMMC_INTERRUPT_CLR_CCRC_ERR(v)           BF_CLEAR(v, 17, 1 )
#define EMMC_INTERRUPT_CLR_CEND_ERR(v)           BF_CLEAR(v, 18, 1 )
#define EMMC_INTERRUPT_CLR_CBAD_ERR(v)           BF_CLEAR(v, 19, 1 )
#define EMMC_INTERRUPT_CLR_DTO_ERR(v)            BF_CLEAR(v, 20, 1 )
#define EMMC_INTERRUPT_CLR_DCRC_ERR(v)           BF_CLEAR(v, 21, 1 )
#define EMMC_INTERRUPT_CLR_DEND_ERR(v)           BF_CLEAR(v, 22, 1 )
#define EMMC_INTERRUPT_CLR_ACMD_ERR(v)           BF_CLEAR(v, 24, 1 )
#define EMMC_INTERRUPT_MASK_CMD_DONE             BF_MASK_AT_32(0, 1)
#define EMMC_INTERRUPT_MASK_DATA_DONE            BF_MASK_AT_32(1, 1)
#define EMMC_INTERRUPT_MASK_BLOCK_GAP            BF_MASK_AT_32(2, 1)
#define EMMC_INTERRUPT_MASK_WRITE_RDY            BF_MASK_AT_32(4, 1)
#define EMMC_INTERRUPT_MASK_READ_RDY             BF_MASK_AT_32(5, 1)
#define EMMC_INTERRUPT_MASK_CARD                 BF_MASK_AT_32(8, 1)
#define EMMC_INTERRUPT_MASK_RETUNE               BF_MASK_AT_32(12, 1)
#define EMMC_INTERRUPT_MASK_BOOTACK              BF_MASK_AT_32(13, 1)
#define EMMC_INTERRUPT_MASK_ENDBOOT              BF_MASK_AT_32(14, 1)
#define EMMC_INTERRUPT_MASK_ERR                  BF_MASK_AT_32(15, 1)
#define EMMC_INTERRUPT_MASK_CTO_ERR              BF_MASK_AT_32(16, 1)
#define EMMC_INTERRUPT_MASK_CCRC_ERR             BF_MASK_AT_32(17, 1)
#define EMMC_INTERRUPT_MASK_CEND_ERR             BF_MASK_AT_32(18, 1)
#define EMMC_INTERRUPT_MASK_CBAD_ERR             BF_MASK_AT_32(19, 1)
#define EMMC_INTERRUPT_MASK_DTO_ERR              BF_MASK_AT_32(20, 1)
#define EMMC_INTERRUPT_MASK_DCRC_ERR             BF_MASK_AT_32(21, 1)
#define EMMC_INTERRUPT_MASK_DEND_ERR             BF_MASK_AT_32(22, 1)
#define EMMC_INTERRUPT_MASK_ACMD_ERR             BF_MASK_AT_32(24, 1)
#define EMMC_INTERRUPT_SHIFT_CMD_DONE            0
#define EMMC_INTERRUPT_SHIFT_DATA_DONE           1
#define EMMC_INTERRUPT_SHIFT_BLOCK_GAP           2
#define EMMC_INTERRUPT_SHIFT_WRITE_RDY           4
#define EMMC_INTERRUPT_SHIFT_READ_RDY            5
#define EMMC_INTERRUPT_SHIFT_CARD                8
#define EMMC_INTERRUPT_SHIFT_RETUNE              12
#define EMMC_INTERRUPT_SHIFT_BOOTACK             13
#define EMMC_INTERRUPT_SHIFT_ENDBOOT             14
#define EMMC_INTERRUPT_SHIFT_ERR                 15
#define EMMC_INTERRUPT_SHIFT_CTO_ERR             16
#define EMMC_INTERRUPT_SHIFT_CCRC_ERR            17
#define EMMC_INTERRUPT_SHIFT_CEND_ERR            18
#define EMMC_INTERRUPT_SHIFT_CBAD_ERR            19
#define EMMC_INTERRUPT_SHIFT_DTO_ERR             20
#define EMMC_INTERRUPT_SHIFT_DCRC_ERR            21
#define EMMC_INTERRUPT_SHIFT_DEND_ERR            22
#define EMMC_INTERRUPT_SHIFT_ACMD_ERR            24

static inline int emmc_interrupt_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_INTERRUPT_GET_CMD_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCMD_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_DATA_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDATA_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_BLOCK_GAP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBLOCK_GAP", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_WRITE_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sWRITE_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_READ_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sREAD_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_CARD(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCARD", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_RETUNE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRETUNE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_BOOTACK(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBOOTACK", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_ENDBOOT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sENDBOOT", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_CTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_CCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_CEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_CBAD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCBAD_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_DTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_DCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_DEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_INTERRUPT_GET_ACMD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sACMD_ERR", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_interrupt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CMD_DONE:%x,DATA_DONE:%x,BLOCK_GAP:%x,WRITE_RDY:%x,READ_RDY:%x,CARD:%x,RETUNE:%x,BOOTACK:%x,ENDBOOT:%x,ERR:%x,CTO_ERR:%x,CCRC_ERR:%x,CEND_ERR:%x,CBAD_ERR:%x,DTO_ERR:%x,DCRC_ERR:%x,DEND_ERR:%x,ACMD_ERR:%x",
    v,
    (int)EMMC_INTERRUPT_GET_CMD_DONE(v),
    (int)EMMC_INTERRUPT_GET_DATA_DONE(v),
    (int)EMMC_INTERRUPT_GET_BLOCK_GAP(v),
    (int)EMMC_INTERRUPT_GET_WRITE_RDY(v),
    (int)EMMC_INTERRUPT_GET_READ_RDY(v),
    (int)EMMC_INTERRUPT_GET_CARD(v),
    (int)EMMC_INTERRUPT_GET_RETUNE(v),
    (int)EMMC_INTERRUPT_GET_BOOTACK(v),
    (int)EMMC_INTERRUPT_GET_ENDBOOT(v),
    (int)EMMC_INTERRUPT_GET_ERR(v),
    (int)EMMC_INTERRUPT_GET_CTO_ERR(v),
    (int)EMMC_INTERRUPT_GET_CCRC_ERR(v),
    (int)EMMC_INTERRUPT_GET_CEND_ERR(v),
    (int)EMMC_INTERRUPT_GET_CBAD_ERR(v),
    (int)EMMC_INTERRUPT_GET_DTO_ERR(v),
    (int)EMMC_INTERRUPT_GET_DCRC_ERR(v),
    (int)EMMC_INTERRUPT_GET_DEND_ERR(v),
    (int)EMMC_INTERRUPT_GET_ACMD_ERR(v));
}
#define EMMC_IRPT_MASK_GET_CMD_DONE(v)           BF_EXTRACT(v, 0 , 1 )
#define EMMC_IRPT_MASK_GET_DATA_DONE(v)          BF_EXTRACT(v, 1 , 1 )
#define EMMC_IRPT_MASK_GET_BLOCK_GAP(v)          BF_EXTRACT(v, 2 , 1 )
#define EMMC_IRPT_MASK_GET_WRITE_RDY(v)          BF_EXTRACT(v, 4 , 1 )
#define EMMC_IRPT_MASK_GET_READ_RDY(v)           BF_EXTRACT(v, 5 , 1 )
#define EMMC_IRPT_MASK_GET_CARD(v)               BF_EXTRACT(v, 8 , 1 )
#define EMMC_IRPT_MASK_GET_RETUNE(v)             BF_EXTRACT(v, 12, 1 )
#define EMMC_IRPT_MASK_GET_BOOTACK(v)            BF_EXTRACT(v, 13, 1 )
#define EMMC_IRPT_MASK_GET_ENDBOOT(v)            BF_EXTRACT(v, 14, 1 )
#define EMMC_IRPT_MASK_GET_ERR(v)                BF_EXTRACT(v, 15, 1 )
#define EMMC_IRPT_MASK_GET_CTO_ERR(v)            BF_EXTRACT(v, 16, 1 )
#define EMMC_IRPT_MASK_GET_CCRC_ERR(v)           BF_EXTRACT(v, 17, 1 )
#define EMMC_IRPT_MASK_GET_CEND_ERR(v)           BF_EXTRACT(v, 18, 1 )
#define EMMC_IRPT_MASK_GET_CBAD_ERR(v)           BF_EXTRACT(v, 19, 1 )
#define EMMC_IRPT_MASK_GET_DTO_ERR(v)            BF_EXTRACT(v, 20, 1 )
#define EMMC_IRPT_MASK_GET_DCRC_ERR(v)           BF_EXTRACT(v, 21, 1 )
#define EMMC_IRPT_MASK_GET_DEND_ERR(v)           BF_EXTRACT(v, 22, 1 )
#define EMMC_IRPT_MASK_GET_ACMD_ERR(v)           BF_EXTRACT(v, 24, 1 )
#define EMMC_IRPT_MASK_CLR_SET_CMD_DONE(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_DATA_DONE(v, set)         BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_BLOCK_GAP(v, set)         BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_WRITE_RDY(v, set)         BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_READ_RDY(v, set)          BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_CARD(v, set)              BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define EMMC_IRPT_MASK_CLR_SET_RETUNE(v, set)            BF_CLEAR_AND_SET(v, set, 12, 1 )
#define EMMC_IRPT_MASK_CLR_SET_BOOTACK(v, set)           BF_CLEAR_AND_SET(v, set, 13, 1 )
#define EMMC_IRPT_MASK_CLR_SET_ENDBOOT(v, set)           BF_CLEAR_AND_SET(v, set, 14, 1 )
#define EMMC_IRPT_MASK_CLR_SET_ERR(v, set)               BF_CLEAR_AND_SET(v, set, 15, 1 )
#define EMMC_IRPT_MASK_CLR_SET_CTO_ERR(v, set)           BF_CLEAR_AND_SET(v, set, 16, 1 )
#define EMMC_IRPT_MASK_CLR_SET_CCRC_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 17, 1 )
#define EMMC_IRPT_MASK_CLR_SET_CEND_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 18, 1 )
#define EMMC_IRPT_MASK_CLR_SET_CBAD_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_IRPT_MASK_CLR_SET_DTO_ERR(v, set)           BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_IRPT_MASK_CLR_SET_DCRC_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_IRPT_MASK_CLR_SET_DEND_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_IRPT_MASK_CLR_SET_ACMD_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_IRPT_MASK_CLR_CMD_DONE(v)           BF_CLEAR(v, 0 , 1 )
#define EMMC_IRPT_MASK_CLR_DATA_DONE(v)          BF_CLEAR(v, 1 , 1 )
#define EMMC_IRPT_MASK_CLR_BLOCK_GAP(v)          BF_CLEAR(v, 2 , 1 )
#define EMMC_IRPT_MASK_CLR_WRITE_RDY(v)          BF_CLEAR(v, 4 , 1 )
#define EMMC_IRPT_MASK_CLR_READ_RDY(v)           BF_CLEAR(v, 5 , 1 )
#define EMMC_IRPT_MASK_CLR_CARD(v)               BF_CLEAR(v, 8 , 1 )
#define EMMC_IRPT_MASK_CLR_RETUNE(v)             BF_CLEAR(v, 12, 1 )
#define EMMC_IRPT_MASK_CLR_BOOTACK(v)            BF_CLEAR(v, 13, 1 )
#define EMMC_IRPT_MASK_CLR_ENDBOOT(v)            BF_CLEAR(v, 14, 1 )
#define EMMC_IRPT_MASK_CLR_ERR(v)                BF_CLEAR(v, 15, 1 )
#define EMMC_IRPT_MASK_CLR_CTO_ERR(v)            BF_CLEAR(v, 16, 1 )
#define EMMC_IRPT_MASK_CLR_CCRC_ERR(v)           BF_CLEAR(v, 17, 1 )
#define EMMC_IRPT_MASK_CLR_CEND_ERR(v)           BF_CLEAR(v, 18, 1 )
#define EMMC_IRPT_MASK_CLR_CBAD_ERR(v)           BF_CLEAR(v, 19, 1 )
#define EMMC_IRPT_MASK_CLR_DTO_ERR(v)            BF_CLEAR(v, 20, 1 )
#define EMMC_IRPT_MASK_CLR_DCRC_ERR(v)           BF_CLEAR(v, 21, 1 )
#define EMMC_IRPT_MASK_CLR_DEND_ERR(v)           BF_CLEAR(v, 22, 1 )
#define EMMC_IRPT_MASK_CLR_ACMD_ERR(v)           BF_CLEAR(v, 24, 1 )
#define EMMC_IRPT_MASK_MASK_CMD_DONE             BF_MASK_AT_32(0, 1)
#define EMMC_IRPT_MASK_MASK_DATA_DONE            BF_MASK_AT_32(1, 1)
#define EMMC_IRPT_MASK_MASK_BLOCK_GAP            BF_MASK_AT_32(2, 1)
#define EMMC_IRPT_MASK_MASK_WRITE_RDY            BF_MASK_AT_32(4, 1)
#define EMMC_IRPT_MASK_MASK_READ_RDY             BF_MASK_AT_32(5, 1)
#define EMMC_IRPT_MASK_MASK_CARD                 BF_MASK_AT_32(8, 1)
#define EMMC_IRPT_MASK_MASK_RETUNE               BF_MASK_AT_32(12, 1)
#define EMMC_IRPT_MASK_MASK_BOOTACK              BF_MASK_AT_32(13, 1)
#define EMMC_IRPT_MASK_MASK_ENDBOOT              BF_MASK_AT_32(14, 1)
#define EMMC_IRPT_MASK_MASK_ERR                  BF_MASK_AT_32(15, 1)
#define EMMC_IRPT_MASK_MASK_CTO_ERR              BF_MASK_AT_32(16, 1)
#define EMMC_IRPT_MASK_MASK_CCRC_ERR             BF_MASK_AT_32(17, 1)
#define EMMC_IRPT_MASK_MASK_CEND_ERR             BF_MASK_AT_32(18, 1)
#define EMMC_IRPT_MASK_MASK_CBAD_ERR             BF_MASK_AT_32(19, 1)
#define EMMC_IRPT_MASK_MASK_DTO_ERR              BF_MASK_AT_32(20, 1)
#define EMMC_IRPT_MASK_MASK_DCRC_ERR             BF_MASK_AT_32(21, 1)
#define EMMC_IRPT_MASK_MASK_DEND_ERR             BF_MASK_AT_32(22, 1)
#define EMMC_IRPT_MASK_MASK_ACMD_ERR             BF_MASK_AT_32(24, 1)
#define EMMC_IRPT_MASK_SHIFT_CMD_DONE            0
#define EMMC_IRPT_MASK_SHIFT_DATA_DONE           1
#define EMMC_IRPT_MASK_SHIFT_BLOCK_GAP           2
#define EMMC_IRPT_MASK_SHIFT_WRITE_RDY           4
#define EMMC_IRPT_MASK_SHIFT_READ_RDY            5
#define EMMC_IRPT_MASK_SHIFT_CARD                8
#define EMMC_IRPT_MASK_SHIFT_RETUNE              12
#define EMMC_IRPT_MASK_SHIFT_BOOTACK             13
#define EMMC_IRPT_MASK_SHIFT_ENDBOOT             14
#define EMMC_IRPT_MASK_SHIFT_ERR                 15
#define EMMC_IRPT_MASK_SHIFT_CTO_ERR             16
#define EMMC_IRPT_MASK_SHIFT_CCRC_ERR            17
#define EMMC_IRPT_MASK_SHIFT_CEND_ERR            18
#define EMMC_IRPT_MASK_SHIFT_CBAD_ERR            19
#define EMMC_IRPT_MASK_SHIFT_DTO_ERR             20
#define EMMC_IRPT_MASK_SHIFT_DCRC_ERR            21
#define EMMC_IRPT_MASK_SHIFT_DEND_ERR            22
#define EMMC_IRPT_MASK_SHIFT_ACMD_ERR            24

static inline int emmc_irpt_mask_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_IRPT_MASK_GET_CMD_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCMD_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_DATA_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDATA_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_BLOCK_GAP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBLOCK_GAP", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_WRITE_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sWRITE_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_READ_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sREAD_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_CARD(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCARD", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_RETUNE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRETUNE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_BOOTACK(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBOOTACK", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_ENDBOOT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sENDBOOT", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_CTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_CCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_CEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_CBAD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCBAD_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_DTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_DCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_DEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_MASK_GET_ACMD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sACMD_ERR", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_irpt_mask_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CMD_DONE:%x,DATA_DONE:%x,BLOCK_GAP:%x,WRITE_RDY:%x,READ_RDY:%x,CARD:%x,RETUNE:%x,BOOTACK:%x,ENDBOOT:%x,ERR:%x,CTO_ERR:%x,CCRC_ERR:%x,CEND_ERR:%x,CBAD_ERR:%x,DTO_ERR:%x,DCRC_ERR:%x,DEND_ERR:%x,ACMD_ERR:%x",
    v,
    (int)EMMC_IRPT_MASK_GET_CMD_DONE(v),
    (int)EMMC_IRPT_MASK_GET_DATA_DONE(v),
    (int)EMMC_IRPT_MASK_GET_BLOCK_GAP(v),
    (int)EMMC_IRPT_MASK_GET_WRITE_RDY(v),
    (int)EMMC_IRPT_MASK_GET_READ_RDY(v),
    (int)EMMC_IRPT_MASK_GET_CARD(v),
    (int)EMMC_IRPT_MASK_GET_RETUNE(v),
    (int)EMMC_IRPT_MASK_GET_BOOTACK(v),
    (int)EMMC_IRPT_MASK_GET_ENDBOOT(v),
    (int)EMMC_IRPT_MASK_GET_ERR(v),
    (int)EMMC_IRPT_MASK_GET_CTO_ERR(v),
    (int)EMMC_IRPT_MASK_GET_CCRC_ERR(v),
    (int)EMMC_IRPT_MASK_GET_CEND_ERR(v),
    (int)EMMC_IRPT_MASK_GET_CBAD_ERR(v),
    (int)EMMC_IRPT_MASK_GET_DTO_ERR(v),
    (int)EMMC_IRPT_MASK_GET_DCRC_ERR(v),
    (int)EMMC_IRPT_MASK_GET_DEND_ERR(v),
    (int)EMMC_IRPT_MASK_GET_ACMD_ERR(v));
}
#define EMMC_IRPT_EN_GET_CMD_DONE(v)             BF_EXTRACT(v, 0 , 1 )
#define EMMC_IRPT_EN_GET_DATA_DONE(v)            BF_EXTRACT(v, 1 , 1 )
#define EMMC_IRPT_EN_GET_BLOCK_GAP(v)            BF_EXTRACT(v, 2 , 1 )
#define EMMC_IRPT_EN_GET_WRITE_RDY(v)            BF_EXTRACT(v, 4 , 1 )
#define EMMC_IRPT_EN_GET_READ_RDY(v)             BF_EXTRACT(v, 5 , 1 )
#define EMMC_IRPT_EN_GET_CARD(v)                 BF_EXTRACT(v, 8 , 1 )
#define EMMC_IRPT_EN_GET_RETUNE(v)               BF_EXTRACT(v, 12, 1 )
#define EMMC_IRPT_EN_GET_BOOTACK(v)              BF_EXTRACT(v, 13, 1 )
#define EMMC_IRPT_EN_GET_ENDBOOT(v)              BF_EXTRACT(v, 14, 1 )
#define EMMC_IRPT_EN_GET_ERR(v)                  BF_EXTRACT(v, 15, 1 )
#define EMMC_IRPT_EN_GET_CTO_ERR(v)              BF_EXTRACT(v, 16, 1 )
#define EMMC_IRPT_EN_GET_CCRC_ERR(v)             BF_EXTRACT(v, 17, 1 )
#define EMMC_IRPT_EN_GET_CEND_ERR(v)             BF_EXTRACT(v, 18, 1 )
#define EMMC_IRPT_EN_GET_CBAD_ERR(v)             BF_EXTRACT(v, 19, 1 )
#define EMMC_IRPT_EN_GET_DTO_ERR(v)              BF_EXTRACT(v, 20, 1 )
#define EMMC_IRPT_EN_GET_DCRC_ERR(v)             BF_EXTRACT(v, 21, 1 )
#define EMMC_IRPT_EN_GET_DEND_ERR(v)             BF_EXTRACT(v, 22, 1 )
#define EMMC_IRPT_EN_GET_ACMD_ERR(v)             BF_EXTRACT(v, 24, 1 )
#define EMMC_IRPT_EN_CLR_SET_CMD_DONE(v, set)            BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_IRPT_EN_CLR_SET_DATA_DONE(v, set)           BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_IRPT_EN_CLR_SET_BLOCK_GAP(v, set)           BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_IRPT_EN_CLR_SET_WRITE_RDY(v, set)           BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_IRPT_EN_CLR_SET_READ_RDY(v, set)            BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_IRPT_EN_CLR_SET_CARD(v, set)                BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define EMMC_IRPT_EN_CLR_SET_RETUNE(v, set)              BF_CLEAR_AND_SET(v, set, 12, 1 )
#define EMMC_IRPT_EN_CLR_SET_BOOTACK(v, set)             BF_CLEAR_AND_SET(v, set, 13, 1 )
#define EMMC_IRPT_EN_CLR_SET_ENDBOOT(v, set)             BF_CLEAR_AND_SET(v, set, 14, 1 )
#define EMMC_IRPT_EN_CLR_SET_ERR(v, set)                 BF_CLEAR_AND_SET(v, set, 15, 1 )
#define EMMC_IRPT_EN_CLR_SET_CTO_ERR(v, set)             BF_CLEAR_AND_SET(v, set, 16, 1 )
#define EMMC_IRPT_EN_CLR_SET_CCRC_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 17, 1 )
#define EMMC_IRPT_EN_CLR_SET_CEND_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 18, 1 )
#define EMMC_IRPT_EN_CLR_SET_CBAD_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_IRPT_EN_CLR_SET_DTO_ERR(v, set)             BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_IRPT_EN_CLR_SET_DCRC_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_IRPT_EN_CLR_SET_DEND_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_IRPT_EN_CLR_SET_ACMD_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_IRPT_EN_CLR_CMD_DONE(v)             BF_CLEAR(v, 0 , 1 )
#define EMMC_IRPT_EN_CLR_DATA_DONE(v)            BF_CLEAR(v, 1 , 1 )
#define EMMC_IRPT_EN_CLR_BLOCK_GAP(v)            BF_CLEAR(v, 2 , 1 )
#define EMMC_IRPT_EN_CLR_WRITE_RDY(v)            BF_CLEAR(v, 4 , 1 )
#define EMMC_IRPT_EN_CLR_READ_RDY(v)             BF_CLEAR(v, 5 , 1 )
#define EMMC_IRPT_EN_CLR_CARD(v)                 BF_CLEAR(v, 8 , 1 )
#define EMMC_IRPT_EN_CLR_RETUNE(v)               BF_CLEAR(v, 12, 1 )
#define EMMC_IRPT_EN_CLR_BOOTACK(v)              BF_CLEAR(v, 13, 1 )
#define EMMC_IRPT_EN_CLR_ENDBOOT(v)              BF_CLEAR(v, 14, 1 )
#define EMMC_IRPT_EN_CLR_ERR(v)                  BF_CLEAR(v, 15, 1 )
#define EMMC_IRPT_EN_CLR_CTO_ERR(v)              BF_CLEAR(v, 16, 1 )
#define EMMC_IRPT_EN_CLR_CCRC_ERR(v)             BF_CLEAR(v, 17, 1 )
#define EMMC_IRPT_EN_CLR_CEND_ERR(v)             BF_CLEAR(v, 18, 1 )
#define EMMC_IRPT_EN_CLR_CBAD_ERR(v)             BF_CLEAR(v, 19, 1 )
#define EMMC_IRPT_EN_CLR_DTO_ERR(v)              BF_CLEAR(v, 20, 1 )
#define EMMC_IRPT_EN_CLR_DCRC_ERR(v)             BF_CLEAR(v, 21, 1 )
#define EMMC_IRPT_EN_CLR_DEND_ERR(v)             BF_CLEAR(v, 22, 1 )
#define EMMC_IRPT_EN_CLR_ACMD_ERR(v)             BF_CLEAR(v, 24, 1 )
#define EMMC_IRPT_EN_MASK_CMD_DONE               BF_MASK_AT_32(0, 1)
#define EMMC_IRPT_EN_MASK_DATA_DONE              BF_MASK_AT_32(1, 1)
#define EMMC_IRPT_EN_MASK_BLOCK_GAP              BF_MASK_AT_32(2, 1)
#define EMMC_IRPT_EN_MASK_WRITE_RDY              BF_MASK_AT_32(4, 1)
#define EMMC_IRPT_EN_MASK_READ_RDY               BF_MASK_AT_32(5, 1)
#define EMMC_IRPT_EN_MASK_CARD                   BF_MASK_AT_32(8, 1)
#define EMMC_IRPT_EN_MASK_RETUNE                 BF_MASK_AT_32(12, 1)
#define EMMC_IRPT_EN_MASK_BOOTACK                BF_MASK_AT_32(13, 1)
#define EMMC_IRPT_EN_MASK_ENDBOOT                BF_MASK_AT_32(14, 1)
#define EMMC_IRPT_EN_MASK_ERR                    BF_MASK_AT_32(15, 1)
#define EMMC_IRPT_EN_MASK_CTO_ERR                BF_MASK_AT_32(16, 1)
#define EMMC_IRPT_EN_MASK_CCRC_ERR               BF_MASK_AT_32(17, 1)
#define EMMC_IRPT_EN_MASK_CEND_ERR               BF_MASK_AT_32(18, 1)
#define EMMC_IRPT_EN_MASK_CBAD_ERR               BF_MASK_AT_32(19, 1)
#define EMMC_IRPT_EN_MASK_DTO_ERR                BF_MASK_AT_32(20, 1)
#define EMMC_IRPT_EN_MASK_DCRC_ERR               BF_MASK_AT_32(21, 1)
#define EMMC_IRPT_EN_MASK_DEND_ERR               BF_MASK_AT_32(22, 1)
#define EMMC_IRPT_EN_MASK_ACMD_ERR               BF_MASK_AT_32(24, 1)
#define EMMC_IRPT_EN_SHIFT_CMD_DONE              0
#define EMMC_IRPT_EN_SHIFT_DATA_DONE             1
#define EMMC_IRPT_EN_SHIFT_BLOCK_GAP             2
#define EMMC_IRPT_EN_SHIFT_WRITE_RDY             4
#define EMMC_IRPT_EN_SHIFT_READ_RDY              5
#define EMMC_IRPT_EN_SHIFT_CARD                  8
#define EMMC_IRPT_EN_SHIFT_RETUNE                12
#define EMMC_IRPT_EN_SHIFT_BOOTACK               13
#define EMMC_IRPT_EN_SHIFT_ENDBOOT               14
#define EMMC_IRPT_EN_SHIFT_ERR                   15
#define EMMC_IRPT_EN_SHIFT_CTO_ERR               16
#define EMMC_IRPT_EN_SHIFT_CCRC_ERR              17
#define EMMC_IRPT_EN_SHIFT_CEND_ERR              18
#define EMMC_IRPT_EN_SHIFT_CBAD_ERR              19
#define EMMC_IRPT_EN_SHIFT_DTO_ERR               20
#define EMMC_IRPT_EN_SHIFT_DCRC_ERR              21
#define EMMC_IRPT_EN_SHIFT_DEND_ERR              22
#define EMMC_IRPT_EN_SHIFT_ACMD_ERR              24

static inline int emmc_irpt_en_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_IRPT_EN_GET_CMD_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCMD_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_DATA_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDATA_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_BLOCK_GAP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBLOCK_GAP", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_WRITE_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sWRITE_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_READ_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sREAD_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_CARD(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCARD", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_RETUNE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRETUNE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_BOOTACK(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBOOTACK", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_ENDBOOT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sENDBOOT", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_CTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_CCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_CEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_CBAD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCBAD_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_DTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_DCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_DEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_IRPT_EN_GET_ACMD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sACMD_ERR", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_irpt_en_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CMD_DONE:%x,DATA_DONE:%x,BLOCK_GAP:%x,WRITE_RDY:%x,READ_RDY:%x,CARD:%x,RETUNE:%x,BOOTACK:%x,ENDBOOT:%x,ERR:%x,CTO_ERR:%x,CCRC_ERR:%x,CEND_ERR:%x,CBAD_ERR:%x,DTO_ERR:%x,DCRC_ERR:%x,DEND_ERR:%x,ACMD_ERR:%x",
    v,
    (int)EMMC_IRPT_EN_GET_CMD_DONE(v),
    (int)EMMC_IRPT_EN_GET_DATA_DONE(v),
    (int)EMMC_IRPT_EN_GET_BLOCK_GAP(v),
    (int)EMMC_IRPT_EN_GET_WRITE_RDY(v),
    (int)EMMC_IRPT_EN_GET_READ_RDY(v),
    (int)EMMC_IRPT_EN_GET_CARD(v),
    (int)EMMC_IRPT_EN_GET_RETUNE(v),
    (int)EMMC_IRPT_EN_GET_BOOTACK(v),
    (int)EMMC_IRPT_EN_GET_ENDBOOT(v),
    (int)EMMC_IRPT_EN_GET_ERR(v),
    (int)EMMC_IRPT_EN_GET_CTO_ERR(v),
    (int)EMMC_IRPT_EN_GET_CCRC_ERR(v),
    (int)EMMC_IRPT_EN_GET_CEND_ERR(v),
    (int)EMMC_IRPT_EN_GET_CBAD_ERR(v),
    (int)EMMC_IRPT_EN_GET_DTO_ERR(v),
    (int)EMMC_IRPT_EN_GET_DCRC_ERR(v),
    (int)EMMC_IRPT_EN_GET_DEND_ERR(v),
    (int)EMMC_IRPT_EN_GET_ACMD_ERR(v));
}
#define EMMC_FORCE_IRPT_GET_CMD_DONE(v)          BF_EXTRACT(v, 0 , 1 )
#define EMMC_FORCE_IRPT_GET_DATA_DONE(v)         BF_EXTRACT(v, 1 , 1 )
#define EMMC_FORCE_IRPT_GET_BLOCK_GAP(v)         BF_EXTRACT(v, 2 , 1 )
#define EMMC_FORCE_IRPT_GET_WRITE_RDY(v)         BF_EXTRACT(v, 4 , 1 )
#define EMMC_FORCE_IRPT_GET_READ_RDY(v)          BF_EXTRACT(v, 5 , 1 )
#define EMMC_FORCE_IRPT_GET_CARD(v)              BF_EXTRACT(v, 8 , 1 )
#define EMMC_FORCE_IRPT_GET_RETUNE(v)            BF_EXTRACT(v, 12, 1 )
#define EMMC_FORCE_IRPT_GET_BOOTACK(v)           BF_EXTRACT(v, 13, 1 )
#define EMMC_FORCE_IRPT_GET_ENDBOOT(v)           BF_EXTRACT(v, 14, 1 )
#define EMMC_FORCE_IRPT_GET_ERR(v)               BF_EXTRACT(v, 15, 1 )
#define EMMC_FORCE_IRPT_GET_CTO_ERR(v)           BF_EXTRACT(v, 16, 1 )
#define EMMC_FORCE_IRPT_GET_CCRC_ERR(v)          BF_EXTRACT(v, 17, 1 )
#define EMMC_FORCE_IRPT_GET_CEND_ERR(v)          BF_EXTRACT(v, 18, 1 )
#define EMMC_FORCE_IRPT_GET_CBAD_ERR(v)          BF_EXTRACT(v, 19, 1 )
#define EMMC_FORCE_IRPT_GET_DTO_ERR(v)           BF_EXTRACT(v, 20, 1 )
#define EMMC_FORCE_IRPT_GET_DCRC_ERR(v)          BF_EXTRACT(v, 21, 1 )
#define EMMC_FORCE_IRPT_GET_DEND_ERR(v)          BF_EXTRACT(v, 22, 1 )
#define EMMC_FORCE_IRPT_GET_ACMD_ERR(v)          BF_EXTRACT(v, 24, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CMD_DONE(v, set)         BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_DATA_DONE(v, set)        BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_BLOCK_GAP(v, set)        BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_WRITE_RDY(v, set)        BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_READ_RDY(v, set)         BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CARD(v, set)             BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define EMMC_FORCE_IRPT_CLR_SET_RETUNE(v, set)           BF_CLEAR_AND_SET(v, set, 12, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_BOOTACK(v, set)          BF_CLEAR_AND_SET(v, set, 13, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_ENDBOOT(v, set)          BF_CLEAR_AND_SET(v, set, 14, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_ERR(v, set)              BF_CLEAR_AND_SET(v, set, 15, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CTO_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 16, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CCRC_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 17, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CEND_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 18, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_CBAD_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 19, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_DTO_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 20, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_DCRC_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 21, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_DEND_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_FORCE_IRPT_CLR_SET_ACMD_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 24, 1 )
#define EMMC_FORCE_IRPT_CLR_CMD_DONE(v)          BF_CLEAR(v, 0 , 1 )
#define EMMC_FORCE_IRPT_CLR_DATA_DONE(v)         BF_CLEAR(v, 1 , 1 )
#define EMMC_FORCE_IRPT_CLR_BLOCK_GAP(v)         BF_CLEAR(v, 2 , 1 )
#define EMMC_FORCE_IRPT_CLR_WRITE_RDY(v)         BF_CLEAR(v, 4 , 1 )
#define EMMC_FORCE_IRPT_CLR_READ_RDY(v)          BF_CLEAR(v, 5 , 1 )
#define EMMC_FORCE_IRPT_CLR_CARD(v)              BF_CLEAR(v, 8 , 1 )
#define EMMC_FORCE_IRPT_CLR_RETUNE(v)            BF_CLEAR(v, 12, 1 )
#define EMMC_FORCE_IRPT_CLR_BOOTACK(v)           BF_CLEAR(v, 13, 1 )
#define EMMC_FORCE_IRPT_CLR_ENDBOOT(v)           BF_CLEAR(v, 14, 1 )
#define EMMC_FORCE_IRPT_CLR_ERR(v)               BF_CLEAR(v, 15, 1 )
#define EMMC_FORCE_IRPT_CLR_CTO_ERR(v)           BF_CLEAR(v, 16, 1 )
#define EMMC_FORCE_IRPT_CLR_CCRC_ERR(v)          BF_CLEAR(v, 17, 1 )
#define EMMC_FORCE_IRPT_CLR_CEND_ERR(v)          BF_CLEAR(v, 18, 1 )
#define EMMC_FORCE_IRPT_CLR_CBAD_ERR(v)          BF_CLEAR(v, 19, 1 )
#define EMMC_FORCE_IRPT_CLR_DTO_ERR(v)           BF_CLEAR(v, 20, 1 )
#define EMMC_FORCE_IRPT_CLR_DCRC_ERR(v)          BF_CLEAR(v, 21, 1 )
#define EMMC_FORCE_IRPT_CLR_DEND_ERR(v)          BF_CLEAR(v, 22, 1 )
#define EMMC_FORCE_IRPT_CLR_ACMD_ERR(v)          BF_CLEAR(v, 24, 1 )
#define EMMC_FORCE_IRPT_MASK_CMD_DONE            BF_MASK_AT_32(0, 1)
#define EMMC_FORCE_IRPT_MASK_DATA_DONE           BF_MASK_AT_32(1, 1)
#define EMMC_FORCE_IRPT_MASK_BLOCK_GAP           BF_MASK_AT_32(2, 1)
#define EMMC_FORCE_IRPT_MASK_WRITE_RDY           BF_MASK_AT_32(4, 1)
#define EMMC_FORCE_IRPT_MASK_READ_RDY            BF_MASK_AT_32(5, 1)
#define EMMC_FORCE_IRPT_MASK_CARD                BF_MASK_AT_32(8, 1)
#define EMMC_FORCE_IRPT_MASK_RETUNE              BF_MASK_AT_32(12, 1)
#define EMMC_FORCE_IRPT_MASK_BOOTACK             BF_MASK_AT_32(13, 1)
#define EMMC_FORCE_IRPT_MASK_ENDBOOT             BF_MASK_AT_32(14, 1)
#define EMMC_FORCE_IRPT_MASK_ERR                 BF_MASK_AT_32(15, 1)
#define EMMC_FORCE_IRPT_MASK_CTO_ERR             BF_MASK_AT_32(16, 1)
#define EMMC_FORCE_IRPT_MASK_CCRC_ERR            BF_MASK_AT_32(17, 1)
#define EMMC_FORCE_IRPT_MASK_CEND_ERR            BF_MASK_AT_32(18, 1)
#define EMMC_FORCE_IRPT_MASK_CBAD_ERR            BF_MASK_AT_32(19, 1)
#define EMMC_FORCE_IRPT_MASK_DTO_ERR             BF_MASK_AT_32(20, 1)
#define EMMC_FORCE_IRPT_MASK_DCRC_ERR            BF_MASK_AT_32(21, 1)
#define EMMC_FORCE_IRPT_MASK_DEND_ERR            BF_MASK_AT_32(22, 1)
#define EMMC_FORCE_IRPT_MASK_ACMD_ERR            BF_MASK_AT_32(24, 1)
#define EMMC_FORCE_IRPT_SHIFT_CMD_DONE           0
#define EMMC_FORCE_IRPT_SHIFT_DATA_DONE          1
#define EMMC_FORCE_IRPT_SHIFT_BLOCK_GAP          2
#define EMMC_FORCE_IRPT_SHIFT_WRITE_RDY          4
#define EMMC_FORCE_IRPT_SHIFT_READ_RDY           5
#define EMMC_FORCE_IRPT_SHIFT_CARD               8
#define EMMC_FORCE_IRPT_SHIFT_RETUNE             12
#define EMMC_FORCE_IRPT_SHIFT_BOOTACK            13
#define EMMC_FORCE_IRPT_SHIFT_ENDBOOT            14
#define EMMC_FORCE_IRPT_SHIFT_ERR                15
#define EMMC_FORCE_IRPT_SHIFT_CTO_ERR            16
#define EMMC_FORCE_IRPT_SHIFT_CCRC_ERR           17
#define EMMC_FORCE_IRPT_SHIFT_CEND_ERR           18
#define EMMC_FORCE_IRPT_SHIFT_CBAD_ERR           19
#define EMMC_FORCE_IRPT_SHIFT_DTO_ERR            20
#define EMMC_FORCE_IRPT_SHIFT_DCRC_ERR           21
#define EMMC_FORCE_IRPT_SHIFT_DEND_ERR           22
#define EMMC_FORCE_IRPT_SHIFT_ACMD_ERR           24

static inline int emmc_force_irpt_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_FORCE_IRPT_GET_CMD_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCMD_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_DATA_DONE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDATA_DONE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_BLOCK_GAP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBLOCK_GAP", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_WRITE_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sWRITE_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_READ_RDY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sREAD_RDY", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_CARD(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCARD", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_RETUNE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRETUNE", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_BOOTACK(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBOOTACK", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_ENDBOOT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sENDBOOT", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_CTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_CCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_CEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_CBAD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCBAD_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_DTO_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDTO_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_DCRC_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCRC_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_DEND_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDEND_ERR", first ? "" : ",");
    first = 0;
  }
  if (EMMC_FORCE_IRPT_GET_ACMD_ERR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sACMD_ERR", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_force_irpt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CMD_DONE:%x,DATA_DONE:%x,BLOCK_GAP:%x,WRITE_RDY:%x,READ_RDY:%x,CARD:%x,RETUNE:%x,BOOTACK:%x,ENDBOOT:%x,ERR:%x,CTO_ERR:%x,CCRC_ERR:%x,CEND_ERR:%x,CBAD_ERR:%x,DTO_ERR:%x,DCRC_ERR:%x,DEND_ERR:%x,ACMD_ERR:%x",
    v,
    (int)EMMC_FORCE_IRPT_GET_CMD_DONE(v),
    (int)EMMC_FORCE_IRPT_GET_DATA_DONE(v),
    (int)EMMC_FORCE_IRPT_GET_BLOCK_GAP(v),
    (int)EMMC_FORCE_IRPT_GET_WRITE_RDY(v),
    (int)EMMC_FORCE_IRPT_GET_READ_RDY(v),
    (int)EMMC_FORCE_IRPT_GET_CARD(v),
    (int)EMMC_FORCE_IRPT_GET_RETUNE(v),
    (int)EMMC_FORCE_IRPT_GET_BOOTACK(v),
    (int)EMMC_FORCE_IRPT_GET_ENDBOOT(v),
    (int)EMMC_FORCE_IRPT_GET_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_CTO_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_CCRC_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_CEND_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_CBAD_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_DTO_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_DCRC_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_DEND_ERR(v),
    (int)EMMC_FORCE_IRPT_GET_ACMD_ERR(v));
}
#define EMMC_CONTROL2_GET_ACKNOX_ERR(v)          BF_EXTRACT(v, 0 , 1 )
#define EMMC_CONTROL2_GET_ACTO_ERR(v)            BF_EXTRACT(v, 1 , 1 )
#define EMMC_CONTROL2_GET_ACCRC_ERR(v)           BF_EXTRACT(v, 2 , 1 )
#define EMMC_CONTROL2_GET_ACEND_ERR(v)           BF_EXTRACT(v, 3 , 1 )
#define EMMC_CONTROL2_GET_ACBAD_ERR(v)           BF_EXTRACT(v, 4 , 1 )
#define EMMC_CONTROL2_GET_NOTC12_ERR(v)          BF_EXTRACT(v, 7 , 1 )
#define EMMC_CONTROL2_GET_UHSMODE(v)             BF_EXTRACT(v, 16, 3 )
#define EMMC_CONTROL2_GET_TUNEON(v)              BF_EXTRACT(v, 22, 1 )
#define EMMC_CONTROL2_GET_TUNED(v)               BF_EXTRACT(v, 23, 1 )
#define EMMC_CONTROL2_CLR_SET_ACKNOX_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_CONTROL2_CLR_SET_ACTO_ERR(v, set)           BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define EMMC_CONTROL2_CLR_SET_ACCRC_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define EMMC_CONTROL2_CLR_SET_ACEND_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define EMMC_CONTROL2_CLR_SET_ACBAD_ERR(v, set)          BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define EMMC_CONTROL2_CLR_SET_NOTC12_ERR(v, set)         BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define EMMC_CONTROL2_CLR_SET_UHSMODE(v, set)            BF_CLEAR_AND_SET(v, set, 16, 3 )
#define EMMC_CONTROL2_CLR_SET_TUNEON(v, set)             BF_CLEAR_AND_SET(v, set, 22, 1 )
#define EMMC_CONTROL2_CLR_SET_TUNED(v, set)              BF_CLEAR_AND_SET(v, set, 23, 1 )
#define EMMC_CONTROL2_CLR_ACKNOX_ERR(v)          BF_CLEAR(v, 0 , 1 )
#define EMMC_CONTROL2_CLR_ACTO_ERR(v)            BF_CLEAR(v, 1 , 1 )
#define EMMC_CONTROL2_CLR_ACCRC_ERR(v)           BF_CLEAR(v, 2 , 1 )
#define EMMC_CONTROL2_CLR_ACEND_ERR(v)           BF_CLEAR(v, 3 , 1 )
#define EMMC_CONTROL2_CLR_ACBAD_ERR(v)           BF_CLEAR(v, 4 , 1 )
#define EMMC_CONTROL2_CLR_NOTC12_ERR(v)          BF_CLEAR(v, 7 , 1 )
#define EMMC_CONTROL2_CLR_UHSMODE(v)             BF_CLEAR(v, 16, 3 )
#define EMMC_CONTROL2_CLR_TUNEON(v)              BF_CLEAR(v, 22, 1 )
#define EMMC_CONTROL2_CLR_TUNED(v)               BF_CLEAR(v, 23, 1 )
#define EMMC_CONTROL2_MASK_ACKNOX_ERR            BF_MASK_AT_32(0, 1)
#define EMMC_CONTROL2_MASK_ACTO_ERR              BF_MASK_AT_32(1, 1)
#define EMMC_CONTROL2_MASK_ACCRC_ERR             BF_MASK_AT_32(2, 1)
#define EMMC_CONTROL2_MASK_ACEND_ERR             BF_MASK_AT_32(3, 1)
#define EMMC_CONTROL2_MASK_ACBAD_ERR             BF_MASK_AT_32(4, 1)
#define EMMC_CONTROL2_MASK_NOTC12_ERR            BF_MASK_AT_32(7, 1)
#define EMMC_CONTROL2_MASK_UHSMODE               BF_MASK_AT_32(16, 3)
#define EMMC_CONTROL2_MASK_TUNEON                BF_MASK_AT_32(22, 1)
#define EMMC_CONTROL2_MASK_TUNED                 BF_MASK_AT_32(23, 1)
#define EMMC_CONTROL2_SHIFT_ACKNOX_ERR           0
#define EMMC_CONTROL2_SHIFT_ACTO_ERR             1
#define EMMC_CONTROL2_SHIFT_ACCRC_ERR            2
#define EMMC_CONTROL2_SHIFT_ACEND_ERR            3
#define EMMC_CONTROL2_SHIFT_ACBAD_ERR            4
#define EMMC_CONTROL2_SHIFT_NOTC12_ERR           7
#define EMMC_CONTROL2_SHIFT_UHSMODE              16
#define EMMC_CONTROL2_SHIFT_TUNEON               22
#define EMMC_CONTROL2_SHIFT_TUNED                23


static inline int emmc_control2_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,ACKNOX_ERR:%x,ACTO_ERR:%x,ACCRC_ERR:%x,ACEND_ERR:%x,ACBAD_ERR:%x,NOTC12_ERR:%x,UHSMODE:%x,TUNEON:%x,TUNED:%x",
    v,
    (int)EMMC_CONTROL2_GET_ACKNOX_ERR(v),
    (int)EMMC_CONTROL2_GET_ACTO_ERR(v),
    (int)EMMC_CONTROL2_GET_ACCRC_ERR(v),
    (int)EMMC_CONTROL2_GET_ACEND_ERR(v),
    (int)EMMC_CONTROL2_GET_ACBAD_ERR(v),
    (int)EMMC_CONTROL2_GET_NOTC12_ERR(v),
    (int)EMMC_CONTROL2_GET_UHSMODE(v),
    (int)EMMC_CONTROL2_GET_TUNEON(v),
    (int)EMMC_CONTROL2_GET_TUNED(v));
}
#define EMMC_BOOT_TIMEOUT_GET_TIMEOUT(v)         BF_EXTRACT(v, 0 , 32)
#define EMMC_BOOT_TIMEOUT_CLR_SET_TIMEOUT(v, set)        BF_CLEAR_AND_SET(v, set, 0 , 32)
#define EMMC_BOOT_TIMEOUT_CLR_TIMEOUT(v)         BF_CLEAR(v, 0 , 32)
#define EMMC_BOOT_TIMEOUT_MASK_TIMEOUT           BF_MASK_AT_32(0, 32)
#define EMMC_BOOT_TIMEOUT_SHIFT_TIMEOUT          0


static inline int emmc_boot_timeout_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,TIMEOUT:%x",
    v,
    (int)EMMC_BOOT_TIMEOUT_GET_TIMEOUT(v));
}
#define EMMC_DBG_SEL_GET_SELECT(v)               BF_EXTRACT(v, 0 , 1 )
#define EMMC_DBG_SEL_CLR_SET_SELECT(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_DBG_SEL_CLR_SELECT(v)               BF_CLEAR(v, 0 , 1 )
#define EMMC_DBG_SEL_MASK_SELECT                 BF_MASK_AT_32(0, 1)
#define EMMC_DBG_SEL_SHIFT_SELECT                0

static inline int emmc_dbg_sel_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_DBG_SEL_GET_SELECT(v)) {
    n += snprintf(buf + n, bufsz - n, "%sSELECT", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_dbg_sel_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,SELECT:%x",
    v,
    (int)EMMC_DBG_SEL_GET_SELECT(v));
}
#define EMMC_EXRDFIFO_CFG_GET_RD_THRSH(v)        BF_EXTRACT(v, 0 , 3 )
#define EMMC_EXRDFIFO_CFG_CLR_SET_RD_THRSH(v, set)       BF_CLEAR_AND_SET(v, set, 0 , 3 )
#define EMMC_EXRDFIFO_CFG_CLR_RD_THRSH(v)        BF_CLEAR(v, 0 , 3 )
#define EMMC_EXRDFIFO_CFG_MASK_RD_THRSH          BF_MASK_AT_32(0, 3)
#define EMMC_EXRDFIFO_CFG_SHIFT_RD_THRSH         0


static inline int emmc_exrdfifo_cfg_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RD_THRSH:%x",
    v,
    (int)EMMC_EXRDFIFO_CFG_GET_RD_THRSH(v));
}
#define EMMC_EXRDFIFO_EN_GET_ENABLE(v)           BF_EXTRACT(v, 0 , 1 )
#define EMMC_EXRDFIFO_EN_CLR_SET_ENABLE(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define EMMC_EXRDFIFO_EN_CLR_ENABLE(v)           BF_CLEAR(v, 0 , 1 )
#define EMMC_EXRDFIFO_EN_MASK_ENABLE             BF_MASK_AT_32(0, 1)
#define EMMC_EXRDFIFO_EN_SHIFT_ENABLE            0

static inline int emmc_exrdfifo_en_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (EMMC_EXRDFIFO_EN_GET_ENABLE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sENABLE", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int emmc_exrdfifo_en_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,ENABLE:%x",
    v,
    (int)EMMC_EXRDFIFO_EN_GET_ENABLE(v));
}
#define EMMC_TUNE_STEP_GET_DELAY(v)              BF_EXTRACT(v, 0 , 3 )
#define EMMC_TUNE_STEP_CLR_SET_DELAY(v, set)             BF_CLEAR_AND_SET(v, set, 0 , 3 )
#define EMMC_TUNE_STEP_CLR_DELAY(v)              BF_CLEAR(v, 0 , 3 )
#define EMMC_TUNE_STEP_MASK_DELAY                BF_MASK_AT_32(0, 3)
#define EMMC_TUNE_STEP_SHIFT_DELAY               0


static inline int emmc_tune_step_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,DELAY:%x",
    v,
    (int)EMMC_TUNE_STEP_GET_DELAY(v));
}
#define EMMC_TUNE_STEPS_STD_GET_STEPS(v)         BF_EXTRACT(v, 0 , 6 )
#define EMMC_TUNE_STEPS_STD_CLR_SET_STEPS(v, set)        BF_CLEAR_AND_SET(v, set, 0 , 6 )
#define EMMC_TUNE_STEPS_STD_CLR_STEPS(v)         BF_CLEAR(v, 0 , 6 )
#define EMMC_TUNE_STEPS_STD_MASK_STEPS           BF_MASK_AT_32(0, 6)
#define EMMC_TUNE_STEPS_STD_SHIFT_STEPS          0


static inline int emmc_tune_steps_std_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,STEPS:%x",
    v,
    (int)EMMC_TUNE_STEPS_STD_GET_STEPS(v));
}
#define EMMC_TUNE_STEPS_DDR_GET_STEPS(v)         BF_EXTRACT(v, 0 , 6 )
#define EMMC_TUNE_STEPS_DDR_CLR_SET_STEPS(v, set)        BF_CLEAR_AND_SET(v, set, 0 , 6 )
#define EMMC_TUNE_STEPS_DDR_CLR_STEPS(v)         BF_CLEAR(v, 0 , 6 )
#define EMMC_TUNE_STEPS_DDR_MASK_STEPS           BF_MASK_AT_32(0, 6)
#define EMMC_TUNE_STEPS_DDR_SHIFT_STEPS          0


static inline int emmc_tune_steps_ddr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,STEPS:%x",
    v,
    (int)EMMC_TUNE_STEPS_DDR_GET_STEPS(v));
}
#define EMMC_SPI_INT_SPT_GET_SELECT(v)           BF_EXTRACT(v, 0 , 8 )
#define EMMC_SPI_INT_SPT_CLR_SET_SELECT(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 8 )
#define EMMC_SPI_INT_SPT_CLR_SELECT(v)           BF_CLEAR(v, 0 , 8 )
#define EMMC_SPI_INT_SPT_MASK_SELECT             BF_MASK_AT_32(0, 8)
#define EMMC_SPI_INT_SPT_SHIFT_SELECT            0


static inline int emmc_spi_int_spt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,SELECT:%x",
    v,
    (int)EMMC_SPI_INT_SPT_GET_SELECT(v));
}
#define EMMC_SLOTISR_VER_GET_SLOT_STATUS(v)      BF_EXTRACT(v, 0 , 8 )
#define EMMC_SLOTISR_VER_GET_SDVERSION(v)        BF_EXTRACT(v, 16, 8 )
#define EMMC_SLOTISR_VER_GET_VENDOR(v)           BF_EXTRACT(v, 24, 8 )
#define EMMC_SLOTISR_VER_CLR_SET_SLOT_STATUS(v, set)     BF_CLEAR_AND_SET(v, set, 0 , 8 )
#define EMMC_SLOTISR_VER_CLR_SET_SDVERSION(v, set)       BF_CLEAR_AND_SET(v, set, 16, 8 )
#define EMMC_SLOTISR_VER_CLR_SET_VENDOR(v, set)          BF_CLEAR_AND_SET(v, set, 24, 8 )
#define EMMC_SLOTISR_VER_CLR_SLOT_STATUS(v)      BF_CLEAR(v, 0 , 8 )
#define EMMC_SLOTISR_VER_CLR_SDVERSION(v)        BF_CLEAR(v, 16, 8 )
#define EMMC_SLOTISR_VER_CLR_VENDOR(v)           BF_CLEAR(v, 24, 8 )
#define EMMC_SLOTISR_VER_MASK_SLOT_STATUS        BF_MASK_AT_32(0, 8)
#define EMMC_SLOTISR_VER_MASK_SDVERSION          BF_MASK_AT_32(16, 8)
#define EMMC_SLOTISR_VER_MASK_VENDOR             BF_MASK_AT_32(24, 8)
#define EMMC_SLOTISR_VER_SHIFT_SLOT_STATUS       0
#define EMMC_SLOTISR_VER_SHIFT_SDVERSION         16
#define EMMC_SLOTISR_VER_SHIFT_VENDOR            24


static inline int emmc_slotisr_ver_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,SLOT_STATUS:%x,SDVERSION:%x,VENDOR:%x",
    v,
    (int)EMMC_SLOTISR_VER_GET_SLOT_STATUS(v),
    (int)EMMC_SLOTISR_VER_GET_SDVERSION(v),
    (int)EMMC_SLOTISR_VER_GET_VENDOR(v));
}
