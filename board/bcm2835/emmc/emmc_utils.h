#pragma once
#include <reg_access.h>
#include "emmc_log.h"
#include "delays.h"
#include "emmc_regs.h"
#include "emmc_regs_bits.h"

const char *emmc_reg_address_to_name(reg32_t reg);

static inline void OPTIMIZED emmc_write_reg(reg32_t reg, uint32_t val)
{
#ifdef CONFIG_EMMC_DEBUG2
  EMMC_DBG2("reg write: reg: %08x(%s), value:%08x", reg, emmc_reg_address_to_name(reg), val);
#endif
  write_reg(reg, val);
}

static inline uint32_t OPTIMIZED emmc_read_reg(reg32_t reg)
{
  uint32_t val;
  val = read_reg(reg);
#ifdef CONFIG_EMMC_DEBUG2
  EMMC_DBG2("reg read: reg: %08x(%s), value:%08x", reg, emmc_reg_address_to_name(reg), val);
#endif
  return val;
}

/*
 * mask == 0:          Wait until reg value equals any of bits in value
 * mask == 0xffffffff: Wait until reg value equals value
 * mask == any other : Wait until reg value equals (mask & value)
 */
static inline int OPTIMIZED emmc_wait_reg_value(
  reg32_t regaddr,
  uint32_t mask,
  uint32_t value,
  uint64_t timeout_usec,
  bool blocking, uint32_t *val)
{
  const uint64_t wait_step_usec = 100;
  uint64_t wait_left_usec = timeout_usec;
  uint32_t regval;

  BUG(blocking == false, "Nonblocking timed wait not supported");

  while(wait_left_usec >= wait_step_usec) {
    regval = emmc_read_reg(regaddr);
    if (mask) {
      if ((regval & mask) == value)
        goto wait_good;
    } else {
      if (regval & value)
        goto wait_good;
    }
    wait_usec(wait_step_usec);
    wait_left_usec -= wait_step_usec;
  }
  return ERR_TIMEOUT;

wait_good:
  if (val)
    *val = regval;
  return ERR_OK;
}

static inline int emmc_interrupt_wait_done_or_err(uint64_t timeout_usec, bool waitcmd, bool waitdat, bool blocking, uint32_t *intval)
{
  uint32_t intmask;
  intmask = EMMC_INTERRUPT_MASK_ERR;
  if (waitcmd)
    intmask |= EMMC_INTERRUPT_MASK_CMD_DONE;
  if (waitdat)
    intmask |= EMMC_INTERRUPT_MASK_DATA_DONE;
  return emmc_wait_reg_value(EMMC_INTERRUPT, 0, intmask, timeout_usec, blocking, intval);
}

static inline int emmc_wait_cmd_dat_ready(void)
{
  int i;
  uint32_t reg;

  for (i = 0; i < 1000; ++i) {
    reg = emmc_read_reg(EMMC_STATUS);
    if (!(EMMC_STATUS_GET_CMD_INHIBIT(reg) || EMMC_STATUS_GET_DAT_INHIBIT(reg)))
      break;
    wait_usec(6);
  }
  if (i == 1000) {
    printf("emmc_wait_cmd_dat_ready: timeout\r\n");
    return -1;
  }
  return 0;
}

static inline int emmc_wait_cmd_inhibit()
{
  int i;
  uint32_t status;
  for (i = 0; i < 1000; ++i) {
    status = emmc_read_reg(EMMC_STATUS);
    if (!EMMC_STATUS_GET_CMD_INHIBIT(status))
      break;
    wait_usec(1000);
  }
  if (i == 1000)
    return -1;
  return 0;
}

static inline int emmc_wait_dat_inhibit()
{
  int i;
  uint32_t status;
  for (i = 0; i < 1000; ++i) {
    status = emmc_read_reg(EMMC_STATUS);
    if (!EMMC_STATUS_GET_DAT_INHIBIT(status))
      break;
    wait_usec(1000);
  }
  if (i == 1000)
    return -1;
  return 0;
}


#define EMMC_CLOCK_HZ_SETUP 400000
#define EMMC_CLOCK_HZ_NORMAL 25000000

int emmc_set_clock(int target_hz);
