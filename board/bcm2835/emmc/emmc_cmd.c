#include <bits_api.h>
#include <stringlib.h>
#include "emmc_cmd.h"
#include "emmc_utils.h"
#include "emmc_regs_bits.h"
#include "emmc_regs.h"

extern bool emmc_mode_blocking;

#define CMDTM_GEN(__idx, __resp_type, __crc_enable)\
  ((__idx << EMMC_CMDTM_SHIFT_CMD_INDEX) |\
  (EMMC_RESPONCE_TYPE_ ##  __resp_type << EMMC_CMDTM_SHIFT_CMD_RSPNS_TYPE) |\
  (__crc_enable << EMMC_CMDTM_SHIFT_CMD_CRCCHK_EN))

static uint32_t sd_commands[] = {
  CMDTM_GEN(0,  NONE,    0),
  CMDTM_GEN(1,  NONE,    0),
  CMDTM_GEN(2,  136_BITS,1),
  CMDTM_GEN(3,  48_BITS, 1),
  CMDTM_GEN(4,  NONE,    0),
  CMDTM_GEN(5,  136_BITS,0),
  CMDTM_GEN(6,  NONE,    0),
  CMDTM_GEN(7,  NONE,    0),
  CMDTM_GEN(8,  48_BITS, 1),
  CMDTM_GEN(9,  NONE,    0),
  CMDTM_GEN(10, NONE,    0),
  CMDTM_GEN(11, NONE,    0),
  CMDTM_GEN(12, NONE,    0),
  CMDTM_GEN(13, NONE,    0),
  CMDTM_GEN(14, NONE,    0),
  CMDTM_GEN(15, NONE,    0),
  CMDTM_GEN(16, NONE,    0),
  CMDTM_GEN(17, NONE,    0),
  CMDTM_GEN(18, NONE,    0),
  CMDTM_GEN(19, NONE,    0),
  CMDTM_GEN(20, NONE,    0),
  CMDTM_GEN(21, NONE,    0),
  CMDTM_GEN(22, NONE,    0),
  CMDTM_GEN(23, NONE,    0),
  CMDTM_GEN(24, NONE,    0),
  CMDTM_GEN(25, NONE,    0),
  CMDTM_GEN(26, NONE,    0),
  CMDTM_GEN(27, NONE,    0),
  CMDTM_GEN(28, NONE,    0),
  CMDTM_GEN(29, NONE,    0),
  CMDTM_GEN(30, NONE,    0),
  CMDTM_GEN(31, NONE,    0),
  CMDTM_GEN(32, NONE,    0),
  CMDTM_GEN(33, NONE,    0),
  CMDTM_GEN(34, NONE,    0),
  CMDTM_GEN(35, NONE,    0),
  CMDTM_GEN(36, NONE,    0),
  CMDTM_GEN(37, NONE,    0),
  CMDTM_GEN(38, NONE,    0),
  CMDTM_GEN(39, NONE,    0),
  CMDTM_GEN(40, NONE,    0),
  CMDTM_GEN(41, NONE,    0),
  CMDTM_GEN(42, NONE,    0),
  CMDTM_GEN(43, NONE,    0),
  CMDTM_GEN(44, NONE,    0),
  CMDTM_GEN(45, NONE,    0),
  CMDTM_GEN(46, NONE,    0),
  CMDTM_GEN(47, NONE,    0),
  CMDTM_GEN(48, NONE,    0),
  CMDTM_GEN(49, NONE,    0),
  CMDTM_GEN(50, NONE,    0),
  CMDTM_GEN(51, NONE,    0),
  CMDTM_GEN(52, NONE,    0),
  CMDTM_GEN(53, NONE,    0),
  CMDTM_GEN(54, NONE,    0),
  CMDTM_GEN(55, 48_BITS, 1),
  CMDTM_GEN(56, NONE,    0),
  CMDTM_GEN(57, NONE,    0),
  CMDTM_GEN(58, NONE,    0),
  CMDTM_GEN(59, NONE,    0)
};

static uint32_t sd_acommands[] = {
  CMDTM_GEN(0, NONE,    0),
  CMDTM_GEN(1, NONE,    0),
  CMDTM_GEN(2, NONE,    0),
  CMDTM_GEN(3, NONE,    0),
  CMDTM_GEN(4, NONE,    0),
  CMDTM_GEN(5, NONE,    0),
  CMDTM_GEN(6, NONE,    0),
  CMDTM_GEN(7, NONE,    0),
  CMDTM_GEN(8, NONE,    0),
  CMDTM_GEN(9, NONE,    0),
  CMDTM_GEN(10, NONE,    0),
  CMDTM_GEN(11, NONE,    0),
  CMDTM_GEN(12, NONE,    0),
  CMDTM_GEN(13, NONE,    0),
  CMDTM_GEN(14, NONE,    0),
  CMDTM_GEN(15, NONE,    0),
  CMDTM_GEN(16, NONE,    0),
  CMDTM_GEN(17, NONE,    0),
  CMDTM_GEN(18, NONE,    0),
  CMDTM_GEN(19, NONE,    0),
  CMDTM_GEN(20, NONE,    0),
  CMDTM_GEN(21, NONE,    0),
  CMDTM_GEN(22, NONE,    0),
  CMDTM_GEN(23, NONE,    0),
  CMDTM_GEN(24, NONE,    0),
  CMDTM_GEN(25, NONE,    0),
  CMDTM_GEN(26, NONE,    0),
  CMDTM_GEN(27, NONE,    0),
  CMDTM_GEN(28, NONE,    0),
  CMDTM_GEN(29, NONE,    0),
  CMDTM_GEN(30, NONE,    0),
  CMDTM_GEN(31, NONE,    0),
  CMDTM_GEN(32, NONE,    0),
  CMDTM_GEN(33, NONE,    0),
  CMDTM_GEN(34, NONE,    0),
  CMDTM_GEN(35, NONE,    0),
  CMDTM_GEN(36, NONE,    0),
  CMDTM_GEN(37, NONE,    0),
  CMDTM_GEN(38, NONE,    0),
  CMDTM_GEN(39, NONE,    0),
  CMDTM_GEN(40, NONE,    0),
  CMDTM_GEN(41, 48_BITS, 0),
  CMDTM_GEN(42, NONE,    0),
  CMDTM_GEN(43, NONE,    0),
  CMDTM_GEN(44, NONE,    0),
  CMDTM_GEN(45, NONE,    0),
  CMDTM_GEN(46, NONE,    0),
  CMDTM_GEN(47, NONE,    0),
  CMDTM_GEN(48, NONE,    0),
  CMDTM_GEN(49, NONE,    0)
};

#define EMMC_BLOCK_SIZE 1024

static inline emmc_cmd_status_t emmc_do_issue_cmd(struct emmc_cmd *c, uint32_t cmdreg, uint64_t timeout_usec)
{
  int err;
  uint32_t blksizecnt;
  uint32_t intval, intval_cmp;
  int response_type;
  char intbuf[256];

  EMMC_LOG("emmc_do_issue_cmd: cmd_idx:%08x, arg:%08x, blocks:%d",
    c->cmd_idx, c->arg, c->num_blocks);

  if (emmc_wait_cmd_inhibit())
    return -1;
  if (emmc_wait_dat_inhibit())
    return -1;

  blksizecnt = 0;
  EMMC_BLKSIZECNT_CLR_SET_BLKSIZE(blksizecnt, EMMC_BLOCK_SIZE);
  EMMC_BLKSIZECNT_CLR_SET_BLKCNT(blksizecnt, c->num_blocks);
  emmc_write_reg(EMMC_BLKSIZECNT, blksizecnt);
  emmc_write_reg(EMMC_ARG1, c->arg);
  emmc_write_reg(EMMC_CMDTM, cmdreg);

  err = emmc_interrupt_wait_done_or_err(timeout_usec, emmc_mode_blocking, &intval);

  /* Clear interrupts before proceeding */
  emmc_write_reg(EMMC_INTERRUPT, intval);

  if (err)
    return EMMC_CMD_TIMEOUT;

  if (EMMC_INTERRUPT_GET_ERR(intval)) {
    if (EMMC_INTERRUPT_GET_CTO_ERR(intval)) {
      intval_cmp = 0;
      EMMC_INTERRUPT_CLR_SET_CTO_ERR(intval_cmp, 1);
      EMMC_INTERRUPT_CLR_SET_ERR(intval_cmp, 1);
      if (intval_cmp == intval)
        return EMMC_CMD_TIMEOUT;
    }
    emmc_interrupt_bitmask_to_string(intbuf, sizeof(intbuf), intval);
    EMMC_ERR("emmc_do_issue_cmd: error in INTERRUPT register: %08x, %s", intval, intbuf);
    return EMMC_CMD_ERR;
  }

  emmc_write_reg(EMMC_INTERRUPT, 0xffff0001);
  response_type = EMMC_CMDTM_GET_CMD_RSPNS_TYPE(cmdreg);

  switch(response_type) {
    case EMMC_RESPONCE_TYPE_NONE:
      break;
    case EMMC_RESPONCE_TYPE_136_BITS:
      c->resp0 = emmc_read_reg(EMMC_RESP0);
      c->resp1 = emmc_read_reg(EMMC_RESP1);
      c->resp2 = emmc_read_reg(EMMC_RESP2);
      c->resp3 = emmc_read_reg(EMMC_RESP3);
      break;
    case EMMC_RESPONCE_TYPE_48_BITS:
      c->resp0 = emmc_read_reg(EMMC_RESP0);
    break;
    case EMMC_RESPONCE_TYPE_48_BITS_BUSY: break;
  }
  EMMC_LOG("emmc_do_issue_cmd result: %d, resp: [%08x][%08x][%08x][%08x]", c->status, c->resp0, c->resp1, c->resp2, c->resp3);
  return EMMC_CMD_OK;
}

emmc_cmd_status_t emmc_cmd(struct emmc_cmd *c, uint64_t timeout_usec)
{
  uint32_t intval;
  char intbuf[256];
  emmc_cmd_status_t tmp_status;
  struct emmc_cmd tmp_cmd;

  intval = emmc_read_reg(EMMC_INTERRUPT);
  if (intval) {
    emmc_interrupt_bitmask_to_string(intbuf, sizeof(intbuf), intval);
    EMMC_WARN("interrupts detected: %08x, %s", intval, intbuf);
    emmc_write_reg(EMMC_INTERRUPT, intval);
  }

  if (EMMC_CMD_IS_ACMD(c->cmd_idx)) {
    emmc_cmd_init(&tmp_cmd, EMMC_CMD55 /* APP_CMD */, c->rca << 16);
    tmp_status = emmc_do_issue_cmd(&tmp_cmd, sd_commands[EMMC_CMD55], timeout_usec);
    if (tmp_status != EMMC_CMD_OK)
      return tmp_status;
    return emmc_do_issue_cmd(c, sd_acommands[c->cmd_idx], timeout_usec);
  }

  return emmc_do_issue_cmd(c, sd_commands[c->cmd_idx], timeout_usec);
}

int emmc_reset_cmd()
{
  uint32_t control1;

  control1 = emmc_read_reg(EMMC_CONTROL1);
  EMMC_CONTROL1_CLR_SET_SRST_CMD(control1, 1);
  emmc_write_reg(EMMC_CONTROL1, control1);

  if (emmc_wait_reg_value(EMMC_CONTROL1, EMMC_CONTROL1_MASK_SRST_CMD, 0, 1000000, emmc_mode_blocking, NULL)) {
    EMMC_ERR("emmc_reset_cmd: timeout");
    return -1;
  }

  return 0;
}

