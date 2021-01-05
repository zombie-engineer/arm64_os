/*
 * SD card initialization procedure.
 * Documentation:
 * 1. SD Card Initialization Part 1-2-3 http://www.rjhcoding.com/avrc-sd-interface-1.php
 */

#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include "emmc_cmd.h"
#include "emmc_utils.h"

extern uint32_t *emmc_device_id;
extern uint32_t emmc_rca;

static inline int emmc_cmd0(void)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD0, 0);
  cmd_ret = emmc_cmd(&c, 0);

  if (cmd_ret != EMMC_CMD_OK)
    return -1;
  return 0;
}

/* SEND_ALL_CID */
static inline int emmc_cmd2(uint32_t *device_id)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD2, 0);
  cmd_ret = emmc_cmd(&c, 0);

  if (cmd_ret != EMMC_CMD_OK)
    return -1;

  device_id[0] = c.resp0;
  device_id[1] = c.resp1;
  device_id[2] = c.resp2;
  device_id[3] = c.resp3;

  return 0;
}

/* SEND_RELATIVE_ADDR */
static inline int emmc_cmd3(uint32_t *out_rca)
{
  uint32_t rca;
  bool crc_error;
  bool illegal_cmd;
  bool error;
  bool status;
  bool ready;

  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD3, 0);
  cmd_ret = emmc_cmd(&c, 0);

  if (cmd_ret != EMMC_CMD_OK)
    return -1;

  crc_error = BF_EXTRACT(c.resp0, 15, 1);
  illegal_cmd = BF_EXTRACT(c.resp0, 14, 1);
  error = BF_EXTRACT(c.resp0, 13, 1);
  status = BF_EXTRACT(c.resp0, 9, 1);
  ready = BF_EXTRACT(c.resp0, 8, 1);
  rca = BF_EXTRACT(c.resp0, 0, 16);

  EMMC_LOG("emmc_cmd3 result: err: %d, crc_err: %d, illegal_cmd: %d, status: %d, ready: %d, rca: %04x",
    error, crc_error, illegal_cmd, status, ready, rca);

  if (error)
    return -1;

  *out_rca = rca;

  return 0;
}

static inline emmc_cmd_status_t emmc_cmd5(void)
{
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD5, 0);
  return emmc_cmd(&c, 1000000);
}

static inline int emmc_cmd8(void)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD8, 0x1aa);
  cmd_ret = emmc_cmd(&c, 0);

  if (cmd_ret != EMMC_CMD_OK)
    return -1;

  if (c.resp0 != 0x1aa)
    return -1;

  return 0;
}

static inline int emmc_acmd41(uint32_t arg, uint32_t *resp)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_ACMD41, arg);
  cmd_ret = emmc_cmd(&c, 0);

  if (cmd_ret != EMMC_CMD_OK)
    return -1;

  *resp = c.resp0;
  return 0;
}

int emmc_reset(void)
{
  int i;
  emmc_cmd_status_t cmd_status;
  uint32_t status;
  uint32_t powered_on = 0, exists = 0;
  uint32_t control1;
  uint32_t intmsk;
  uint32_t acmd41_resp;

  emmc_write_reg(EMMC_CONTROL0, 0);

  mbox_get_power_state(MBOX_DEVICE_ID_SD, &powered_on, &exists);
  EMMC_LOG("emmc_reset: SD powered on: %d, exists: %d", powered_on, exists);

  /* Disable and re-set clock */
  /*
   * 1. Reset circuit and disable clock
   */
  control1 = emmc_read_reg(EMMC_CONTROL1);
  EMMC_CONTROL1_CLR_SET_SRST_HC(control1, 1);
  EMMC_CONTROL1_CLR_CLK_INTLEN(control1);
  EMMC_CONTROL1_CLR_CLK_EN(control1);
  emmc_write_reg(EMMC_CONTROL1, control1);

  for(i = 0; i < 1000; ++i) {
    wait_usec(6);
    control1 = emmc_read_reg(EMMC_CONTROL1);
    if (!EMMC_CONTROL1_GET_SRST_HC(control1))
      break;
  }

  if (i == 1000) {
    EMMC_ERR("SRST_HC timeout");
    return -1;
  }

  EMMC_LOG("emmc_reset: after SRST_HC: control1: %08x", control1);
  emmc_write_reg(EMMC_CONTROL2, 0);
  /*
   * Set low clock
   */
  if (emmc_set_clock(EMMC_CLOCK_HZ_SETUP)) {
    EMMC_ERR("failed to set clock to %d Hz", EMMC_CLOCK_HZ_SETUP);
    return -1;
  }

  status = emmc_read_reg(EMMC_STATUS);
  printf("STATUS: %08x\r\n", status);

  printf("Enabling SD clock\r\n");
  wait_usec(2000);
  control1 = emmc_read_reg(EMMC_CONTROL1);
  EMMC_CONTROL1_CLR_SET_CLK_EN(control1, 1);
  emmc_write_reg(EMMC_CONTROL1, control1);
  wait_usec(2000);

  emmc_write_reg(EMMC_IRPT_EN, 0);
  intmsk = 0xffffffff;
  emmc_write_reg(EMMC_INTERRUPT, intmsk);
  // EMMC_INTERRUPT_CLR_CARD(intmsk);
  intmsk = 0xfffffeff;
  emmc_write_reg(EMMC_IRPT_MASK, intmsk);
  wait_usec(2000);

  if (emmc_cmd0() != EMMC_CMD_OK) {
    EMMC_ERR("emmc_reset: failed at CMD0 step");
    return -1;
  }

  if (emmc_cmd8() != EMMC_CMD_OK) {
    EMMC_ERR("emmc_reset: failed at CMD8 step");
    return -1;
  }

  cmd_status = emmc_cmd5();
  if (cmd_status == EMMC_CMD_OK) {
    EMMC_CRIT("emmc_reset: detected SDIO card. Not supported");
    return -1;
  } else if (cmd_status == EMMC_CMD_TIMEOUT) {
    EMMC_LOG("emmc_reset: detected SD card");
    if (emmc_reset_cmd()) {
      printf("emmc_reset: failed to reset cmd");
      return -1;
    }
  } else {
    EMMC_ERR("emmc_reset: failed at CMD5 step");
    return -1;
  }

  if (emmc_acmd41(0, &acmd41_resp)) {
    EMMC_ERR("emmc_reset: failed at ACMD41");
    return -1;
  }

  while(BIT_IS_CLEAR(acmd41_resp, 31)) {
    emmc_acmd41(0x00ff8000 | (1<<28) | (1<<30), &acmd41_resp);
    wait_usec(500000);
  }

  /*
   * Set normal clock
   */
  if (emmc_set_clock(EMMC_CLOCK_HZ_NORMAL)) {
    EMMC_ERR("failed to set clock to %d Hz", EMMC_CLOCK_HZ_NORMAL);
    return -1;
  }

  /* Get device id */
  if (emmc_cmd2(emmc_device_id)) {
    EMMC_ERR("emmc_reset: failed at CMD2 (SEND_ALL_CID) step");
    return -1;
  }

  EMMC_LOG("emmc_reset: device_id: %08x.%08x.%08x.%08x",
    emmc_device_id[0],
    emmc_device_id[1],
    emmc_device_id[2],
    emmc_device_id[3]);

  if (emmc_cmd3(&emmc_rca)) {
    EMMC_ERR("emmc_reset: failed at CMD3 (SEND_RELATIVE_ADDR) step");
    return -1;
  }

  EMMC_LOG("emmc_reset: RCA: %08x", emmc_rca);

  EMMC_LOG("emmc_reset: completed successfully");
  return 0;
}


