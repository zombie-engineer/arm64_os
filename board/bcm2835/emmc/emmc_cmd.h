#pragma once
#include <types.h>
#include <stringlib.h>

/* GO_IDLE */
#define EMMC_CMD0                 0x00000000
/* SEND_ALL_CID */
#define EMMC_CMD2                 0x00000002
/* SEND_RELATIVE_ADDRESS */
#define EMMC_CMD3                 0x00000003
/* CHECK SDIO */
#define EMMC_CMD5                 0x00000005
/* SEND_IF_COND */
#define EMMC_CMD8                 0x00000008

/* SEND_IF_COND */
#define EMMC_CMD8                 0x00000008
/* APP_CMD */
#define EMMC_CMD55                0x00000037

#define ACMD_BIT 31
#define ACMD(__idx) ((1<<ACMD_BIT) | __idx)
#define EMMC_CMD_IS_ACMD(__cmd) (__cmd & (1<<ACMD_BIT) ? 1 : 0)
#define EMMC_ACMD_RAW_IDX(__cmd) (__cmd & ~(1<<ACMD_BIT))

#define EMMC_ACMD41               0x80000029

typedef enum emmc_cmd_status {
  EMMC_CMD_OK,
  EMMC_CMD_ERR,
  EMMC_CMD_TIMEOUT
} emmc_cmd_status_t;

struct emmc_cmd {
  uint32_t cmd_idx;
  uint32_t arg;
  uint32_t num_blocks;
  uint32_t rca;
  uint32_t resp0;
  uint32_t resp1;
  uint32_t resp2;
  uint32_t resp3;
  emmc_cmd_status_t status;
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
