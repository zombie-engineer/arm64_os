/*
 * SD card initialization procedure.
 * Documentation:
 * 1. SD Card Initialization Part 1-2-3 http://www.rjhcoding.com/avrc-sd-interface-1.php
 */

#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <bits_api.h>
#include <emmc.h>
#include "emmc_cmd.h"
#include "emmc_utils.h"

extern uint32_t *emmc_device_id;
extern uint32_t emmc_rca;

#define EMMC_CHECK_ERR(__fmt, ...)\
  do {\
    if (err != ERR_OK) {\
      EMMC_ERR("%s err %d, " __fmt, __func__,  err, #__VA_ARGS__);\
      return err;\
    }\
  } while(0)

static inline int emmc_cmd0(void)
{
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD0, 0);
  return emmc_cmd_status_to_err(emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC));
}

/* SEND_ALL_CID */
static inline int emmc_cmd2(uint32_t *device_id)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD2, 0);
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);

  if (cmd_ret != EMMC_CMD_OK)
    return emmc_cmd_status_to_err(cmd_ret);

  device_id[0] = c.resp0;
  device_id[1] = c.resp1;
  device_id[2] = c.resp2;
  device_id[3] = c.resp3;

  return ERR_OK;
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
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);

  if (cmd_ret != EMMC_CMD_OK)
    return emmc_cmd_status_to_err(cmd_ret);

  crc_error = BF_EXTRACT(c.resp0, 15, 1);
  illegal_cmd = BF_EXTRACT(c.resp0, 14, 1);
  error = BF_EXTRACT(c.resp0, 13, 1);
  status = BF_EXTRACT(c.resp0, 9, 1);
  ready = BF_EXTRACT(c.resp0, 8, 1);
  rca = BF_EXTRACT(c.resp0, 16, 16);

  EMMC_LOG("emmc_cmd3 result: err: %d, crc_err: %d, illegal_cmd: %d, status: %d, ready: %d, rca: %04x",
    error, crc_error, illegal_cmd, status, ready, rca);

  if (error)
    return ERR_GENERIC;

  *out_rca = rca;

  return ERR_OK;
}

static inline int emmc_cmd5(void)
{
  struct emmc_cmd c;
  emmc_cmd_status_t cmd_ret;

  emmc_cmd_init(&c, EMMC_CMD5, 0);
  cmd_ret = emmc_cmd(&c, MSEC_TO_USEC(2));
  return emmc_cmd_status_to_err(cmd_ret);
}

static inline int emmc_cmd8(void)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD8, EMMC_CMD8_ARG);
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);

  if (cmd_ret != EMMC_CMD_OK)
    return emmc_cmd_status_to_err(cmd_ret);

  if (c.resp0 != EMMC_CMD8_VALID_RESP)
    return ERR_GENERIC;

  return ERR_OK;
}

static inline int emmc_acmd6(uint32_t rca, uint32_t bus_width_bit)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_ACMD6, bus_width_bit);
  c.rca = rca;
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);
  return emmc_cmd_status_to_err(cmd_ret);
}

static inline int emmc_acmd41(uint32_t arg, uint32_t *resp)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_ACMD41, arg);
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);

  if (cmd_ret != EMMC_CMD_OK)
    return emmc_cmd_status_to_err(cmd_ret);

  *resp = c.resp0;
  return ERR_OK;
}

/* SEND_SCR */
static inline int emmc_acmd51(uint32_t rca, char *scrbuf, int scrbuf_len)
{
  struct emmc_cmd c;

  if (scrbuf_len < 8) {
    EMMC_CRIT("SRC buffer less than 8 bytes");
    return -1;
  }

  if ((uint64_t)scrbuf & 3) {
    EMMC_CRIT("SRC buffer not aligned to 4 bytes");
    return -1;
  }

  emmc_cmd_init(&c, EMMC_ACMD51, 0);

  c.block_size = 8;
  c.num_blocks = 1;
  c.rca = rca;
  c.databuf = scrbuf;

  return emmc_cmd_status_to_err(emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC * 4));
}

static inline void emmc_set_block_size(uint32_t block_size)
{
  uint32_t regval;
  regval = emmc_read_reg(EMMC_BLKSIZECNT);
  regval &= 0xffff;
  regval |= block_size;
  emmc_write_reg(EMMC_BLKSIZECNT, regval);
}

static inline int emmc_reset_handle_scr(uint32_t rca, uint32_t *scr)
{
  int err;
  char *s = (char *)scr;
  uint32_t control0;
  uint32_t scr_le32 = (s[0]<<24)| (s[1]<<16) | (s[2]<<8) | s[3];
  int sd_spec = BF_EXTRACT(scr_le32, (56-32), 4);
  int sd_spec3 = BF_EXTRACT(scr_le32, (47-32), 1);
  int sd_spec4 = BF_EXTRACT(scr_le32, (42-32), 1);
  int sd_specx = BF_EXTRACT(scr_le32, (38-32), 4);
  int bus_width = BF_EXTRACT(scr_le32, (48-32), 4);

  EMMC_LOG("SCR: sd_spec:%d, sd_spec3:%d, sd_spec4:%d, sd_specx:%d",
    sd_spec, sd_spec3, sd_spec4, sd_specx);
  if (bus_width & 4) {
    err = emmc_acmd6(rca, EMMC_BUS_WIDTH_4BITS);
    EMMC_CHECK_ERR("failed to set bus to 4 bits");

    control0 = emmc_read_reg(EMMC_CONTROL0);
    EMMC_CONTROL0_CLR_SET_HCTL_DWIDTH(control0, 1);
    emmc_write_reg(EMMC_CONTROL0, control0);
  }
  emmc_write_reg(EMMC_INTERRUPT, 0xffffffff);
  return ERR_OK;
}

int emmc_reset(void)
{
  int i, err;
  uint32_t status;
  uint32_t powered_on = 0, exists = 0;
  uint32_t control1;
  uint32_t intmsk;
  uint32_t acmd41_resp;
  uint32_t emmc_status;
  uint32_t emmc_state;
  uint32_t scr[2];

  emmc_write_reg(EMMC_CONTROL0, 0);

  err = mbox_get_power_state(MBOX_DEVICE_ID_SD, &powered_on, &exists);
  if (err)
    EMMC_ERR("emmc_reset: failed to get SD power state");
  EMMC_LOG("emmc_reset: SD powered on: %d, exists: %d", powered_on, exists);
  powered_on = 1;

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
    return ERR_TIMEOUT;
  }

  EMMC_LOG("emmc_reset: after SRST_HC: control1: %08x", control1);
  emmc_write_reg(EMMC_CONTROL2, 0);
  /*
   * Set low clock
   */
  err = emmc_set_clock(EMMC_CLOCK_HZ_SETUP);
  if (err != ERR_OK) {
    EMMC_ERR("failed to set clock to %d Hz", EMMC_CLOCK_HZ_SETUP);
    return err;
  }

  status = emmc_read_reg(EMMC_STATUS);
  EMMC_LOG("STATUS: %08x", status);

  EMMC_LOG("Enabling SD clock");
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

  err = emmc_cmd0();
  EMMC_CHECK_ERR("failed at CMD0 step");

  err = emmc_cmd8();
  EMMC_CHECK_ERR("failed at CMD8 step");

  /* 
   * CMD5 is CHECK_SDIO command, it will timeout for non-SDIO devices
   */
  err = emmc_cmd5();
  if (err == ERR_OK) {
    EMMC_CRIT("emmc_reset: detected SDIO card. Not supported");
    return ERR_NOT_IMPLEMENTED;
  } 
  if (err != ERR_TIMEOUT) {
    EMMC_ERR("emmc_reset: failed at CMD5 step, err: %d", err);
    return err;
  }

  EMMC_LOG("emmc_reset: detected SD card");
  /*
   * After failed command (timeout) we should reset command state machine 
   * to a known state.
   */
  err = emmc_reset_cmd();
  EMMC_CHECK_ERR("failed to reset command after SDIO_CHECK");

  err = emmc_acmd41(0, &acmd41_resp);
  EMMC_CHECK_ERR("failed at ACMD41");

  while(BIT_IS_CLEAR(acmd41_resp, 31)) {
    emmc_acmd41(0x00ff8000 | (1<<28) | (1<<30), &acmd41_resp);
    wait_usec(500000);
  }

  /*
   * Set normal clock
   */
  err = emmc_set_clock(EMMC_CLOCK_HZ_NORMAL);
  EMMC_CHECK_ERR("failed to set clock to %d Hz", EMMC_CLOCK_HZ_NORMAL);

  /* Get device id */
  err = emmc_cmd2(emmc_device_id);
  EMMC_CHECK_ERR("failed at CMD2 (SEND_ALL_CID) step");

  EMMC_LOG("emmc_reset: device_id: %08x.%08x.%08x.%08x",
    emmc_device_id[0],
    emmc_device_id[1],
    emmc_device_id[2],
    emmc_device_id[3]);

  err = emmc_cmd3(&emmc_rca);
  EMMC_CHECK_ERR("failed at CMD3 (SEND_RELATIVE_ADDR) step");
  EMMC_LOG("emmc_reset: RCA: %08x", emmc_rca);

  err = emmc_cmd7(emmc_rca);
  EMMC_CHECK_ERR("failed at CMD7 (SELECT_CARD) step");

  err = emmc_cmd13(emmc_rca, &emmc_status);
  EMMC_CHECK_ERR("failed at CMD13 (SEND_STATUS) step");

  emmc_state = EMMC_CARD_STATUS_GET_CURRENT_STATE(emmc_status);
  EMMC_LOG("emmc_reset: status: %08x, curr state: %d(%s)", emmc_status,
    emmc_state,
    emmc_state_to_string(emmc_state));

  emmc_set_block_size(EMMC_BLOCK_SIZE);

  err = emmc_acmd51(emmc_rca, (char *)scr, sizeof(scr));
  EMMC_CHECK_ERR("failed at ACMD51 (SEND_SCR) step");

  EMMC_LOG("emmc_reset: SCR: %08x.%08x", scr[0], scr[1]);

  err = emmc_reset_handle_scr(emmc_rca, scr);
  EMMC_CHECK_ERR("failed at SCR handling step");

  EMMC_LOG("emmc_reset: completed successfully");

  return ERR_OK;
}


