#pragma once
#include <types.h>
#include <stringlib.h>
#include <error.h>

/* GO_IDLE */
#define EMMC_CMD0                 0x00000000
/* SEND_ALL_CID */
#define EMMC_CMD2                 0x00000002
/* SEND_RELATIVE_ADDRESS */
#define EMMC_CMD3                 0x00000003
/* CHECK SDIO */
#define EMMC_CMD5                 0x00000005
/* SELECT_CARD */
#define EMMC_CMD7                 0x00000007
/* SEND_IF_COND */
#define EMMC_CMD8                 0x00000008
#define EMMC_CMD8_ARG             0x000001aa
#define EMMC_CMD8_VALID_RESP      EMMC_CMD8_ARG
/* STOP_TRANSMISSION */
#define EMMC_CMD12                0x0000000c
/* SEND_STATUS */
#define EMMC_CMD13                0x0000000d
/* READ_SINGLE_BLOCK */
#define EMMC_CMD17                0x00000011
/* READ_MULTIPLE_BLOCK */
#define EMMC_CMD18                0x00000012
/* WRITE_BLOCK */
#define EMMC_CMD24                0x00000018
/* APP_CMD */
#define EMMC_CMD55                0x00000037

#define ACMD_BIT 31
#define ACMD(__idx) ((1<<ACMD_BIT) | __idx)
#define EMMC_CMD_IS_ACMD(__cmd) (__cmd & (1<<ACMD_BIT) ? 1 : 0)
#define EMMC_ACMD_RAW_IDX(__cmd) (__cmd & ~(1<<ACMD_BIT))

/* SET_BUS_WIDTH */
#define EMMC_BUS_WIDTH_1BIT 0
#define EMMC_BUS_WIDTH_4BITS 2
#define EMMC_ACMD6                0x80000006

#define EMMC_ACMD41               0x80000029
#define EMMC_ACMD51               0x80000033

typedef enum emmc_cmd_status {
  EMMC_CMD_OK,
  EMMC_CMD_ERR,
  EMMC_CMD_TIMEOUT
} emmc_cmd_status_t;

static inline int emmc_cmd_status_to_err(emmc_cmd_status_t status)
{
  int errors[3] = {
    ERR_OK,
    ERR_GENERIC,
    ERR_TIMEOUT
  };
  return errors[status];
}

struct emmc_cmd {
  uint32_t cmd_idx;
  uint32_t arg;
  uint32_t block_size;
  uint32_t num_blocks;
  uint32_t rca;
  uint32_t resp0;
  uint32_t resp1;
  uint32_t resp2;
  uint32_t resp3;
  emmc_cmd_status_t status;
  char *databuf;
};

static inline void emmc_cmd_init(struct emmc_cmd *c, int cmd_idx, int arg)
{
  memset(c, 0, sizeof(*c));
  c->cmd_idx = cmd_idx;
  c->arg = arg;
}

emmc_cmd_status_t emmc_cmd(struct emmc_cmd *c, uint64_t timeout_usec);

/*
 * Reset state after a failed command
 */
int emmc_reset_cmd(void);

/* Select card */
static inline int emmc_cmd7(uint32_t rca)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD7, rca << 16);
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);
  return emmc_cmd_status_to_err(cmd_ret);
}

static inline int emmc_cmd13(uint32_t rca, uint32_t *out_status)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD13, rca << 16);
  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);

  if (cmd_ret != EMMC_CMD_OK)
    return emmc_cmd_status_to_err(cmd_ret);

  *out_status = c.resp0;

  return ERR_OK;
}

/* READ_SINGLE_BLOCK */
static inline int emmc_cmd17(uint32_t block_idx, char *dstbuf)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD17, block_idx);
  c.databuf = dstbuf;
  c.num_blocks = 1;
  c.block_size = 512;

  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);
  return emmc_cmd_status_to_err(cmd_ret);
}

/* WRITE_BLOCK */
static inline int emmc_cmd24(uint32_t block_idx, char *srcbuf)
{
  emmc_cmd_status_t cmd_ret;
  struct emmc_cmd c;

  emmc_cmd_init(&c, EMMC_CMD24, block_idx);
  c.databuf = srcbuf;
  c.num_blocks = 1;
  c.block_size = 512;

  cmd_ret = emmc_cmd(&c, EMMC_WAIT_TIMEOUT_USEC);
  return emmc_cmd_status_to_err(cmd_ret);
}
