#include <pwm.h>
#include <reg_access.h>
#include <types.h>
#include <memory.h>
#include <clock_manager.h>
#include <common.h>
#include <delays.h>

#define PWM_BASE   (uint64_t)(PERIPHERAL_BASE_PHY + 0x0020c000)

#define PWM_CTL    (reg32_t)(PWM_BASE + 0x00)
#define PWM_STA    (reg32_t)(PWM_BASE + 0x04)
#define PWM_DMAC   (reg32_t)(PWM_BASE + 0x08)
#define PWM_RNG1   (reg32_t)(PWM_BASE + 0x10)
#define PWM_DAT1   (reg32_t)(PWM_BASE + 0x14)
#define PWM_FIF1   (reg32_t)(PWM_BASE + 0x18)
#define PWM_RNG2   (reg32_t)(PWM_BASE + 0x20)
#define PWM_DAT2   (reg32_t)(PWM_BASE + 0x24)
#define PWM_FIF2   (reg32_t)(PWM_BASE + 0x28)

#define PWM_CTL_MSEN2 15
#define PWM_CTL_USEF2 13
#define PWM_CTL_POLA2 12
#define PWM_CTL_SBIT2 11
#define PWM_CTL_RPTL2 10
#define PWM_CTL_MODE2 9
#define PWM_CTL_PWEN2 8
#define PWM_CTL_MSEN1 8
#define PWM_CTL_CLRF1 7
#define PWM_CTL_USEF1 5
#define PWM_CTL_POLA1 4
#define PWM_CTL_SBIT1 3
#define PWM_CTL_RPTL1 2
#define PWM_CTL_MODE1 1
#define PWM_CTL_PWEN1 0

//typedef struct {
//  char FULL1 : 1; // 0  Fifo full
//  char EMPT1 : 1; // 1  Fifo empty
//  char WERR1 : 1; // 2  Fifo write err
//  char RERR1 : 1; // 3  Fifo read  err
//  char GAPO1 : 1; // 4  Channel 1 gap occured
//  char GAPO2 : 1; // 5  Channel 2 gap occured
//  char GAPO3 : 1; // 6  Channel 3 gap occured
//  char GAPO4 : 1; // 7  Channel 4 gap occured
//  char BERR  : 1; // 8  Bus error
//  char STA1  : 1; // 9  Channel 1 state
//  char STA2  : 1; // 10 Channel 2 state
//  char STA3  : 1; // 11 Channel 3 state
//  char STA4  : 1; // 12 Channel 4 state
//  uint32_t RESRV : 19;// 31:13
//} pwm_sta_t;

int pwm_enable(int channel, int ms_mode)
{
  int st;
  uint32_t divi, divf;
  uint32_t sta, ctl;

  if (channel != 0 && channel != 1)
    return ERR_INVAL_ARG;

  /* freq = 19.2 MHz = 19 200 000 */
  divi = 192; 
  /* freq = 19.2 MHz / 192 = 100 KHz = 19200000 / 192 = 100000 */
  divf = 0;

  st = cm_set_clock(CM_CLK_ID_PWM, CM_SETCLK_SRC_OSC, CM_SETCLK_MASH_OFF, divi, divf);
  if (st) {
    printf("pwm_enable error: %d (%s)\n", st, set_clock_err_to_str(st));
    return -1;
  }
  
  ctl = read_reg(PWM_CTL);
  ctl &= ~(1<<(PWM_CTL_PWEN1 + (channel<<3)));
  write_reg(PWM_CTL, ctl);

  while(1) {
    sta = read_reg(PWM_STA);
    if (sta == 2 || sta == 0x202)
      break;
    //printf("PWM disabled STA: %08x\n", PWM_CONTROL->STA.val);
  }

  if (ms_mode)
    ctl |= (1<<(PWM_CTL_MSEN1+(channel<<3)));
  else
    ctl &= ~(1<<(PWM_CTL_MSEN1+(channel<<3)));
  write_reg(PWM_CTL, ctl);
  wait_msec(10);
  ctl |= (1<<(PWM_CTL_PWEN1+(channel<<3)));
  return 0;
}

int pwm_set(int channel, int range, int data)
{
  /* freq = 10 000 Hz / range Hz */
  write_reg(PWM_RNG1 + (channel << 2), range);
  write_reg(PWM_DAT1 + (channel << 2), data);
  return 0;
}
