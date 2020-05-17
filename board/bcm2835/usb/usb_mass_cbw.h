#pragma once
#include <types.h>
#include <compiler.h>
#include <bits_api.h>

/*
 * Universal Serial Bus Mass Storage Class Bulk-Only Transport
 *
 * Command Block Wrapper (CBW)
 */
struct cbw {
#define CBW_SIGNATURE 0x43425355
  uint32_t cbw_signature;

  uint32_t cbw_tag;         // +4
  uint32_t cbw_data_length; // +8
  uint8_t  cbw_flags;       // +12
  uint8_t  cbw_lun  : 4;    // +13
  uint8_t  reserved1 : 4;
  uint8_t  cbw_length : 5;  // +14
  uint8_t  reserved2  : 3;
  char     cbw_block[0];    // +15
} PACKED;

static inline void cbw_set_flags(struct cbw *c, int dir) 
{
  BIT_SET_V_8(c->cbw_flags, 7, dir);
}

static inline int cbw_get_direction(struct cbw *c) 
{
  return BIT_IS_SET(c->cbw_flags, 7) ? 1 : 0;
}

/*
 * Command Status Wrapper (CSW)
 */
struct csw {
#define CSW_SIGNATURE 0x53425355
  uint32_t csw_signature;
  uint32_t csw_tag;
  uint32_t csw_data_residue;

#define CSW_STATUS_GOOD      0
#define CSW_STATUS_FAILED    1
#define CSW_STATUS_PHASE_ERR 2
  uint8_t  csw_status;
} PACKED;

int cbw_transfer(struct usb_hcd_pipe *d, int dir, void *buf, int bufsz);
