#include <pwm.h>
#include <reg_access.h>

#define PWM_BASE (unsigned long)(0x3f020c000)

#define CM_PWMCTL (unsigned long)(0x3f2020a0)
#define CM_PWMDIV (unsigned long)(0x3f2020a4)

#define CM_PWMCTL_PASSWORD 0x5a000000
#define CM_PWMCTL_ENABLE   0x00000010
#define CM_PWMCTL_BUSY     0x00000080

#define CM_PWMCTL_MASH_STAGE_1 (1<<9)

#define CM_PWMCTL_SRC_GND     0
#define CM_PWMCTL_SRC_OSC     1
#define CM_PWMCTL_SRC_TSTDBG0 2
#define CM_PWMCTL_SRC_TSTDBG1 3
#define CM_PWMCTL_SRC_PLLA    4
#define CM_PWMCTL_SRC_PLLC    5
#define CM_PWMCTL_SRC_PLLD    6
#define CM_PWMCTL_SRC_HDMI    7

#define PWM_REG_CTL  (PWM_BASE + 0x00)
#define PWM_REG_STA  (PWM_BASE + 0x04)
#define PWM_REG_DMAC (PWM_BASE + 0x08)
#define PWM_REG_RNG1 (PWM_BASE + 0x10)
#define PWM_REG_DAT1 (PWM_BASE + 0x14)
#define PWM_REG_FIF1 (PWM_BASE + 0x18)
#define PWM_REG_RNG2 (PWM_BASE + 0x20)
#define PWM_REG_DAT2 (PWM_BASE + 0x24)

#define PWM_REG_CTL_EN 1
#define PWM_REG_CTL_MSEN1_MS  (1 << 7)
#define PWM_REG_CTL_MSEN1_PWM (0 << 7)

int pwm_enable(int channel)
{
  int i;
  // pwm off
  *(reg32_t)PWM_REG_CTL = 0;
  // enable flag off
  *(reg32_t)CM_PWMCTL = ((*(reg32_t)CM_PWMCTL) & ~CM_PWMCTL_ENABLE) | CM_PWMCTL_PASSWORD;

  // wait until not busy
  i = 10000;
  while(*(reg32_t)CM_PWMCTL & CM_PWMCTL_BUSY) {
    if (--i == 0)
      return -1;
  }

  // set divider
  *(reg32_t)CM_PWMDIV = (5 << 12) | CM_PWMCTL_PASSWORD;
  *(reg32_t)CM_PWMCTL = CM_PWMCTL_SRC_PLLD | CM_PWMCTL_MASH_STAGE_1 | CM_PWMCTL_ENABLE | CM_PWMCTL_PASSWORD;

  // wait until not busy
  i = 10000;
  while(*(reg32_t)CM_PWMCTL & CM_PWMCTL_BUSY) {
    if (--i == 0)
      return -1;
  }

  *(reg32_t)PWM_REG_RNG1 = 100;
  *(reg32_t)PWM_REG_DAT1 = 60;
  *(reg32_t)PWM_REG_CTL  = PWM_REG_CTL_MSEN1_MS | PWM_REG_CTL_EN;
}
