#pragma once
#include <board_map.h>

#define EMMC_ARG2           (reg32_t)(EMMC_BASE + 0x0000)
#define EMMC_BLKSIZECNT     (reg32_t)(EMMC_BASE + 0x0004)
#define EMMC_ARG1           (reg32_t)(EMMC_BASE + 0x0008)
#define EMMC_CMDTM          (reg32_t)(EMMC_BASE + 0x000c)
#define EMMC_RESP0          (reg32_t)(EMMC_BASE + 0x0010)
#define EMMC_RESP1          (reg32_t)(EMMC_BASE + 0x0014)
#define EMMC_RESP2          (reg32_t)(EMMC_BASE + 0x0018)
#define EMMC_RESP3          (reg32_t)(EMMC_BASE + 0x001c)
#define EMMC_DATA           (reg32_t)(EMMC_BASE + 0x0020)
#define EMMC_STATUS         (reg32_t)(EMMC_BASE + 0x0024)
#define EMMC_CONTROL0       (reg32_t)(EMMC_BASE + 0x0028)
#define EMMC_CONTROL1       (reg32_t)(EMMC_BASE + 0x002c)
#define EMMC_INTERRUPT      (reg32_t)(EMMC_BASE + 0x0030)
#define EMMC_IRPT_MASK      (reg32_t)(EMMC_BASE + 0x0034)
#define EMMC_IRPT_EN        (reg32_t)(EMMC_BASE + 0x0038)
#define EMMC_CONTROL2       (reg32_t)(EMMC_BASE + 0x003c)
#define EMMC_CAPABILITIES_0 (reg32_t)(EMMC_BASE + 0x0040)
#define EMMC_CAPABILITIES_1 (reg32_t)(EMMC_BASE + 0x0044)
#define EMMC_FORCE_IRPT     (reg32_t)(EMMC_BASE + 0x0050)
#define EMMC_BOOT_TIMEOUT   (reg32_t)(EMMC_BASE + 0x0070)
#define EMMC_DBG_SEL        (reg32_t)(EMMC_BASE + 0x0074)
#define EMMC_EXRDFIFO_CFG   (reg32_t)(EMMC_BASE + 0x0080)
#define EMMC_EXRDFIFO_EN    (reg32_t)(EMMC_BASE + 0x0084)
#define EMMC_TUNE_STEP      (reg32_t)(EMMC_BASE + 0x0088)
#define EMMC_TUNE_STEPS_STD (reg32_t)(EMMC_BASE + 0x008c)
#define EMMC_TUNE_STEPS_DDR (reg32_t)(EMMC_BASE + 0x0090)
#define EMMC_SPI_INT_SPT    (reg32_t)(EMMC_BASE + 0x00f0)
#define EMMC_SLOTISR_VER    (reg32_t)(EMMC_BASE + 0x00fc)

#define EMMC_CONTROL1_SRST_HC 24

#define EMMC_RESPONCE_TYPE_NONE         0b00
#define EMMC_RESPONCE_TYPE_136_BITS     0b01
#define EMMC_RESPONCE_TYPE_48_BITS      0b10
#define EMMC_RESPONCE_TYPE_48_BITS_BUSY 0b11
