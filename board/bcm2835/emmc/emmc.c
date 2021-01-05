#include <emmc.h>
#include <common.h>
#include <reg_access.h>
#include <bits_api.h>
#include <stringlib.h>
#include "emmc_regs.h"
#include "emmc_cmd.h"
#include "emmc_utils.h"
#include "emmc_initialize.h"
#include "emmc_regs_bits.h"
#include <mbox/mbox.h>
#include <mbox/mbox_props.h>
#include <gpio.h>

int emmc_log_level = LOG_LEVEL_DEBUG2;
bool emmc_mode_blocking = true;
uint32_t emmc_device_id[4];
uint32_t emmc_rca;

#define GPIO_PIN_CARD_DETECT 47

static inline void emmc_init_gpio(void)
{
  int gpio_num;

  /*
   *  GPIO_CD
   */
  gpio_set_function(GPIO_PIN_CARD_DETECT, GPIO_FUNC_IN);

  /*
   * GPIO_CD Pull-up
   */
  gpio_set_pullupdown(GPIO_PIN_CARD_DETECT, GPIO_PULLUPDOWN_EN_PULLUP);
  gpio_set_detect_high(GPIO_PIN_CARD_DETECT);

  for (gpio_num = 48; gpio_num < 48 + 6; gpio_num++) {
    gpio_set_function(gpio_num, GPIO_FUNC_ALT_3);
    gpio_set_pullupdown(gpio_num, GPIO_PULLUPDOWN_EN_PULLUP);
  }

  /*
   * Doesn't seem to work on my setup
   */
#if 0
  while(1) {
    wait_msec(500);
    printf("GPIO_PIN_LEVEL_1: %08x, PED:%08x\r\n", *(uint32_t*)0x3f200038, *(uint32_t*)0x3f200044);
    *(uint32_t*)0x3f200044 = 0x8000;
    if (gpio_is_set(GPIO_PIN_CARD_DETECT))
      printf("emmc: card detected\r\n");
    else
      printf("emmc: no card detected\r\n");
  }
#endif
}

static inline void emmc_debug_registers(void)
{
  printf("EMMC_ARG2          : %08x\r\n", read_reg(EMMC_ARG2          ));
  printf("EMMC_BLKSIZECNT    : %08x\r\n", read_reg(EMMC_BLKSIZECNT    ));
  printf("EMMC_ARG1          : %08x\r\n", read_reg(EMMC_ARG1          ));
  printf("EMMC_CMDTM         : %08x\r\n", read_reg(EMMC_CMDTM         ));
  printf("EMMC_RESP0         : %08x\r\n", read_reg(EMMC_RESP0         ));
  printf("EMMC_RESP1         : %08x\r\n", read_reg(EMMC_RESP1         ));
  printf("EMMC_RESP2         : %08x\r\n", read_reg(EMMC_RESP2         ));
  printf("EMMC_RESP3         : %08x\r\n", read_reg(EMMC_RESP3         ));
  printf("EMMC_DATA          : %08x\r\n", read_reg(EMMC_DATA          ));
  printf("EMMC_STATUS        : %08x\r\n", read_reg(EMMC_STATUS        ));
  printf("EMMC_CONTROL0      : %08x\r\n", read_reg(EMMC_CONTROL0      ));
  printf("EMMC_CONTROL1      : %08x\r\n", read_reg(EMMC_CONTROL1      ));
  printf("EMMC_INTERRUPT     : %08x\r\n", read_reg(EMMC_INTERRUPT     ));
  printf("EMMC_IRPT_MASK     : %08x\r\n", read_reg(EMMC_IRPT_MASK     ));
  printf("EMMC_IRPT_EN       : %08x\r\n", read_reg(EMMC_IRPT_EN       ));
  printf("EMMC_CONTROL2      : %08x\r\n", read_reg(EMMC_CONTROL2      ));
  printf("EMMC_CAPABILITIES_0: %08x\r\n", read_reg(EMMC_CAPABILITIES_0));
  printf("EMMC_CAPABILITIES_1: %08x\r\n", read_reg(EMMC_CAPABILITIES_1));
  printf("EMMC_FORCE_IRPT    : %08x\r\n", read_reg(EMMC_FORCE_IRPT    ));
  printf("EMMC_BOOT_TIMEOUT  : %08x\r\n", read_reg(EMMC_BOOT_TIMEOUT  ));
  printf("EMMC_DBG_SEL       : %08x\r\n", read_reg(EMMC_DBG_SEL       ));
  printf("EMMC_EXRDFIFO_CFG  : %08x\r\n", read_reg(EMMC_EXRDFIFO_CFG  ));
  printf("EMMC_EXRDFIFO_EN   : %08x\r\n", read_reg(EMMC_EXRDFIFO_EN   ));
  printf("EMMC_TUNE_STEP     : %08x\r\n", read_reg(EMMC_TUNE_STEP     ));
  printf("EMMC_TUNE_STEPS_STD: %08x\r\n", read_reg(EMMC_TUNE_STEPS_STD));
  printf("EMMC_TUNE_STEPS_DDR: %08x\r\n", read_reg(EMMC_TUNE_STEPS_DDR));
  printf("EMMC_SPI_INT_SPT   : %08x\r\n", read_reg(EMMC_SPI_INT_SPT   ));
  printf("EMMC_SLOTISR_VER   : %08x\r\n", read_reg(EMMC_SLOTISR_VER   ));
}

volatile int xxx = 1;
int emmc_init(void)
{
  while(xxx);
  // emmc_init_gpio();
  if (emmc_reset()) {
    EMMC_ERR("emmc_init failed");
    return -1;
  }

  EMMC_LOG("emmc_init successfull");
  // emmc_debug_registers();
  return 0;
}


void emmc_report(void)
{
  uint32_t ver;
  int vendor, sdver;
  uint32_t clock_rate;

  ver = emmc_read_reg(EMMC_SLOTISR_VER);
  mbox_get_clock_rate(MBOX_CLOCK_ID_EMMC, &clock_rate);
  vendor = EMMC_SLOTISR_VER_GET_VENDOR(ver);
  sdver = EMMC_SLOTISR_VER_GET_SDVERSION(ver);

  EMMC_LOG("version %08x, VENDOR: %04x, SD: %04x, clock: %d", ver, vendor, sdver, clock_rate);
}

//int emmc_read(int blocknum, int numblocks, char *buf, int bufsz)
//{
//  int block;
//  int i;
//  int num_fetches;
//
//  uint32_t *ptr = (uint32_t*)buf;
//  uint32_t *ptr_end = ptr + (bufsz >> 2);
//
//  /*
//   * num_fetches - number of times we have to read from DATA register
//   * to receive a complete block of data.
//   */
//  num_fetches = EMMC_BLOCK_SIZE / sizeof(uint32_t);
//
//  if (sd_cmd(EMMC_CMD_SET_BLOCKCNT, EMMC_BLOCK_SIZE)) {
//    printf("emmc_read: failed to send cmd EMMC_CMD_SET_BLOCKCNT\r\n");
//    return -1;
//  }
//
//  emmc_debug_registers();
//  for (block = 0; block < numblocks; ++block) {
//    printf("emmc_read: Reading block %d\r\n", blocknum + block);
//    if (sd_cmd(EMMC_CMD_READ_SINGLE, blocknum + block)) {
//      printf("emmc_read: failed to send cmd EMMC_CMD_READ_SINGLE\r\n");
//      return -1;
//    }
//
//    for (i = 0; i < num_fetches; ++i) {
//      *(ptr++) = emmc_read_reg(EMMC_DATA);
//      // printf("fetch: %08x\r\n", *(ptr - 1));
//      if (ptr > ptr_end)
//        break;
//    }
//  }
//
//  return (char*)ptr - buf;
//}
