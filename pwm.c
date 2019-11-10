#include <pwm.h>
#include <reg_access.h>
#include <types.h>
#include <memory.h>
#include <clock_manager.h>
#include <common.h>
#include <delays.h>

#define PWM_BASE   (unsigned long)(PERIPHERAL_BASE_PHY + 0x0020c000)


typedef struct {
  char PWEN1 : 1; // 0
  char MODE1 : 1; // 1
  char RPTL1 : 1; // 2
  char SBIT1 : 1; // 3
  char POLA1 : 1; // 4
  char USEF1 : 1; // 5
  char CLRF1 : 1; // 6
  char MSEN1 : 1; // 7
  char PWEN2 : 1; // 8
  char MODE2 : 1; // 9
  char RPTL2 : 1; // 10
  char SBIT2 : 1; // 11
  char POLA2 : 1; // 12
  char USEF2 : 1; // 13
  char CLRF2 : 1; // 14
  char MSEN2 : 1; // 15
  uint32_t RESRV : 16;// 31:16
} pwm_ctl_t;

typedef struct {
  char FULL1 : 1; // 0  Fifo full
  char EMPT1 : 1; // 1  Fifo empty
  char WERR1 : 1; // 2  Fifo write err
  char RERR1 : 1; // 3  Fifo read  err
  char GAPO1 : 1; // 4  Channel 1 gap occured
  char GAPO2 : 1; // 5  Channel 2 gap occured
  char GAPO3 : 1; // 6  Channel 3 gap occured
  char GAPO4 : 1; // 7  Channel 4 gap occured
  char BERR  : 1; // 8  Bus error
  char STA1  : 1; // 9  Channel 1 state
  char STA2  : 1; // 10 Channel 2 state
  char STA3  : 1; // 11 Channel 3 state
  char STA4  : 1; // 12 Channel 4 state
  uint32_t RESRV : 19;// 31:13
} pwm_sta_t;


typedef struct {
  union {
    pwm_ctl_t fld;
    uint32_t  val;
  } CTL;

  union {
    pwm_sta_t fld;
    uint32_t  val;
  } STA;

  uint32_t DMAC;
  uint32_t UND1;
  uint32_t RNG1;
  uint32_t DAT1;
  uint32_t FIF1;
  uint32_t UND2;
  uint32_t RNG2;
  uint32_t DAT2;
} pwm_t;

#define PWM_CONTROL ((volatile pwm_t*)PWM_BASE)

int pwm_enable(int channel, int ms_mode)
{
  int st;
  uint32_t divi, divf;

  /* freq = 19.2 MHz = 19 200 000 */
  divi = 192; 
  /* freq = 19.2 MHz / 192 = 100 KHz = 19200000 / 192 = 100000 */
  divf = 0;

  st = cm_set_clock(CM_CLK_ID_PWM, CM_SETCLK_SRC_OSC, CM_SETCLK_MASH_OFF, divi, divf);
  if (st) {
    printf("pwm_enable error: %d (%s)\n", st, set_clock_err_to_str(st));
    return -1;
  }
  
  PWM_CONTROL->CTL.val = 0;
  while(PWM_CONTROL->STA.val != 2 && PWM_CONTROL->STA.val != 0x202) {
    printf("PWM disabled STA: %08x\n", PWM_CONTROL->STA.val);
  }
  PWM_CONTROL->CTL.fld.MSEN1 = ms_mode;
  wait_msec(10);
  PWM_CONTROL->CTL.fld.PWEN1 = 1;
  return 0;
}

int pwm_set(int channel, int range, int data)
{
  /* freq = 10 000 Hz / range Hz */
  if (channel == 0) {
    PWM_CONTROL->RNG1 = range;
    PWM_CONTROL->DAT1 = data;
  }
  else if (channel == 1) {
    PWM_CONTROL->RNG2 = range;
    PWM_CONTROL->DAT2 = data;
  }
  return 0;
}
