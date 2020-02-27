#include <memory.h>
#include <error.h>
#include <common.h>
#include <gpio.h>

#define BSC0_BASE ((uint64_t)PERIPHERAL_BASE_PHY + 0x00205000)
#define BSC1_BASE ((uint64_t)PERIPHERAL_BASE_PHY + 0x00804000)
#define BSC2_BASE ((uint64_t)PERIPHERAL_BASE_PHY + 0x00805000)

/* Control */
#define BSC_MASTER_REG_C(master_id)    BSC ## master_id ## _BASE + 0x00
/* Status */
#define BSC_MASTER_REG_S(master_id)    BSC ## master_id ## _BASE + 0x04
/* Data length */
#define BSC_MASTER_REG_DLEN(master_id) BSC ## master_id ## _BASE + 0x08
/* Slave address */
#define BSC_MASTER_REG_A(master_id)    BSC ## master_id ## _BASE + 0x0c
/* Data FIFO */
#define BSC_MASTER_REG_FIFO(master_id) BSC ## master_id ## _BASE + 0x10
/* Clock divider */
#define BSC_MASTER_REG_DIV(master_id)  BSC ## master_id ## _BASE + 0x14
/* Data delay */
#define BSC_MASTER_REG_DEL(master_id)  BSC ## master_id ## _BASE + 0x18
/* Clock timeout */
#define BSC_MASTER_REG_CLKT(master_id) BSC ## master_id ## _BASE + 0x1c

#define BSC_MASTER_C_READ  (1<<0)
#define BSC_MASTER_C_CLEAR (3<<4)
#define BSC_MASTER_C_ST    (1<<7)
#define BSC_MASTER_C_INTD  (1<<8)
#define BSC_MASTER_C_INTT  (1<<9)
#define BSC_MASTER_C_INTR  (1<<10)
#define BSC_MASTER_C_I2CEN (1<<15)

#define BSC_MASTER_S_TA    (1<<0)
#define BSC_MASTER_S_DONE  (1<<1)
#define BSC_MASTER_S_TXW   (1<<2)
#define BSC_MASTER_S_RXR   (1<<3)
#define BSC_MASTER_S_TXD   (1<<4)
#define BSC_MASTER_S_RXD   (1<<5)
#define BSC_MASTER_S_TXE   (1<<6)
#define BSC_MASTER_S_RXF   (1<<7)
#define BSC_MASTER_S_ERR   (1<<8)
#define BSC_MASTER_S_CLKT  (1<<9)

int i2c_init()
{
  printf("i2c_init\n");
  return ERR_OK;
}
