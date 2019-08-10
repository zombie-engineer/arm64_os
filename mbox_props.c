#include "mbox_props.h"
#include "mbox.h"

int mbox_get_firmware_rev()
{
  mbox[0] = 7 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_FIRMWARE_REV;
  mbox[3] = 4;
  mbox[4] = 4;
  mbox[5] = 0;
  mbox[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox[5];
  return -1;
}

int mbox_get_mac_addr(char *mac_start, char *mac_end)
{
  int i;
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_MAC_ADDR;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
  {
    for (i = 0; i < 6; ++i)
    {
      if (mac_start + i >= mac_end)
        break;
      mac_start[i] = ((char*)(&mbox[5]))[i];
    }
    return 0;
  }
  return -1;
}

int mbox_get_board_model()
{
  mbox[0] = 7 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_BOARD_MODEL;
  mbox[3] = 4;
  mbox[4] = 4;
  mbox[5] = 0;
  mbox[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox[5];
  return -1;
}

int mbox_get_board_rev()
{
  mbox[0] = 7 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_BOARD_REV;
  mbox[3] = 4;
  mbox[4] = 4;
  mbox[5] = 0;
  mbox[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox[5];
  return -1;
}

unsigned long mbox_get_board_serial()
{
  unsigned long res;

  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_BOARD_SERIAL;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;
  res = mbox[5];
  res <<= 32;
  res |= mbox[6];
  return res;
}

int mbox_get_arm_memory(int *base_addr, int *byte_size)
{
  if (base_addr == 0 || byte_size == 0)
    return -1;

  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_ARM_MEMORY;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
  {
    *base_addr = mbox[5];
    *byte_size = mbox[6];
    return 0;
  }
  return -1;
}

int mbox_get_vc_memory(int *base_addr, int *byte_size)
{
  if (base_addr == 0 || byte_size == 0)
    return -1;

  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = MBOX_TAG_GET_ARM_MEMORY;
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
  {
    *base_addr = mbox[5];
    *byte_size = mbox[6];
    return 0;
  }
  return -1;
}
