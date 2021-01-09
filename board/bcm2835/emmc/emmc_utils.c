#include <stringlib.h>
#include <bits_api.h>
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include "emmc_utils.h"

extern bool emmc_mode_blocking;

static inline uint32_t emmc_get_clock_div(uint32_t target_clock)
{
  uint32_t base_clock;

  if (mbox_get_clock_rate(MBOX_CLOCK_ID_EMMC, &base_clock))
    return -1;

  if (base_clock == 200000000) {
    if (target_clock == EMMC_CLOCK_HZ_SETUP) {
      return 64;
    }
    if (target_clock == EMMC_CLOCK_HZ_NORMAL) {
      return 4;
    }
  }

  return 0;
}

static inline int emmc_wait_clock_stabilized(uint64_t timeout_usec, bool blocking)
{
  return emmc_wait_reg_value(
    EMMC_CONTROL1,
    EMMC_CONTROL1_MASK_CLK_STABLE,
    EMMC_CONTROL1_MASK_CLK_STABLE,
    timeout_usec,
    blocking, NULL);
}

int emmc_set_clock(int target_hz)
{
  int err;
  uint32_t control1;
  uint32_t div;

  div = emmc_get_clock_div(target_hz);

  if (div == 0) {
    EMMC_ERR("emmc_set_clock: failed to deduce clock divisor");
    return ERR_GENERIC;
  }

  if (emmc_wait_cmd_dat_ready())
    return ERR_GENERIC;

  EMMC_LOG("emmc_set_clock: status: %08x", emmc_read_reg(EMMC_STATUS));

  control1 = emmc_read_reg(EMMC_CONTROL1);
  EMMC_CONTROL1_CLR_SET_DATA_TOUNIT(control1, 0xb);
  EMMC_CONTROL1_CLR_SET_CLK_INTLEN(control1, 1);
  control1 |= div;
  EMMC_LOG("emmc_set_clock: control1: %08x", control1);
  emmc_write_reg(EMMC_CONTROL1, control1);
  wait_usec(6);

  err = emmc_wait_clock_stabilized(1000, emmc_mode_blocking);
  if (err) {
    EMMC_LOG("emmc_set_clock: failed to stabilize clock, err: %d");
    return err;
  }

  return ERR_OK;
}

const char *emmc_reg_address_to_name(reg32_t reg)
{
  if (reg == EMMC_ARG2) return "EMMC_ARG2";
  if (reg == EMMC_BLKSIZECNT) return "EMMC_BLKSIZECNT";
  if (reg == EMMC_ARG1) return "EMMC_ARG1";
  if (reg == EMMC_CMDTM) return "EMMC_CMDTM";
  if (reg == EMMC_RESP0) return "EMMC_RESP0";
  if (reg == EMMC_RESP1) return "EMMC_RESP1";
  if (reg == EMMC_RESP2) return "EMMC_RESP2";
  if (reg == EMMC_RESP3) return "EMMC_RESP3";
  if (reg == EMMC_DATA) return "EMMC_DATA";
  if (reg == EMMC_STATUS) return "EMMC_STATUS";
  if (reg == EMMC_CONTROL0) return "EMMC_CONTROL0";
  if (reg == EMMC_CONTROL1) return "EMMC_CONTROL1";
  if (reg == EMMC_INTERRUPT) return "EMMC_INTERRUPT";
  if (reg == EMMC_IRPT_MASK) return "EMMC_IRPT_MASK";
  if (reg == EMMC_IRPT_EN) return "EMMC_IRPT_EN";
  if (reg == EMMC_CONTROL2) return "EMMC_CONTROL2";
  if (reg == EMMC_CAPABILITIES_0) return "EMMC_CAPABILITIES_0";
  if (reg == EMMC_CAPABILITIES_1) return "EMMC_CAPABILITIES_1";
  if (reg == EMMC_FORCE_IRPT) return "EMMC_FORCE_IRPT";
  if (reg == EMMC_BOOT_TIMEOUT) return "EMMC_BOOT_TIMEOUT";
  if (reg == EMMC_DBG_SEL) return "EMMC_DBG_SEL";
  if (reg == EMMC_EXRDFIFO_CFG) return "EMMC_EXRDFIFO_CFG";
  if (reg == EMMC_EXRDFIFO_EN) return "EMMC_EXRDFIFO_EN";
  if (reg == EMMC_TUNE_STEP) return "EMMC_TUNE_STEP";
  if (reg == EMMC_TUNE_STEPS_STD) return "EMMC_TUNE_STEPS_STD";
  if (reg == EMMC_TUNE_STEPS_DDR) return "EMMC_TUNE_STEPS_DDR";
  if (reg == EMMC_SPI_INT_SPT) return "EMMC_SPI_INT_SPT";
  if (reg == EMMC_SLOTISR_VER) return "EMMC_SLOTISR_VER";
  return "REG_UNKNOWN";
}
