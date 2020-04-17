#include <pwm.h>
#include <reg_access.h>
#include <types.h>
#include <memory.h>
#include <clock_manager.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>
#include <gpio_set.h>
#include <bits_api.h>

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

#define PWM_STA_FULL1 0
#define PWM_STA_EMPT1 1
#define PWM_STA_WERR1 2
#define PWM_STA_RERR1 3
#define PWM_STA_GAPO1 4
#define PWM_STA_GAPO2 5
#define PWM_STA_GAPO3 6
#define PWM_STA_GAPO4 7
#define PWM_STA_BERR  8
#define PWM_STA_STA1  9
#define PWM_STA_STA2  10
#define PWM_STA_STA3  11
#define PWM_STA_STA4  12

static gpio_set_handle_t gpio_set_handle_pwm;
static int gpio_pin_pwm0;
static int gpio_pin_pwm1;

DECL_GPIO_SET_KEY(pwm_key, "PWM_KEYS_______");

int pwm_enable(int ch, int ms_mode)
{
  int st;
  uint32_t divi, divf;
  uint32_t sta, ctl;
  int choff = ch << 3;
  printf("pwm_enalbe:ch:%d:msmode:%d\r\n", ch, ms_mode);

  if (ch != 0 && ch != 1)
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
  BIT_CLEAR_U32(ctl, PWM_CTL_PWEN1 + choff);

#define PWM_CTL_WRITE(x)\
  do {\
    write_reg(PWM_CTL, x);\
    wait_usec(20);\
    sta = read_reg(PWM_STA);\
  } while (BIT_IS_SET(sta, PWM_STA_BERR))

  PWM_CTL_WRITE(ctl);

#define CHECK_STA(r, bit)\
  if (r & (1<< PWM_STA_ ## bit))\
    puts(#bit "-");
  while(1) {
    sta = read_reg(PWM_STA);
    CHECK_STA(sta, FULL1);
    CHECK_STA(sta, EMPT1);
    CHECK_STA(sta, WERR1);
    CHECK_STA(sta, RERR1);
    CHECK_STA(sta, GAPO1);
    CHECK_STA(sta, GAPO2);
    CHECK_STA(sta, GAPO3);
    CHECK_STA(sta, GAPO4);
    CHECK_STA(sta, BERR);
    CHECK_STA(sta, STA1);
    CHECK_STA(sta, STA2);
    CHECK_STA(sta, STA3);
    CHECK_STA(sta, STA4);

    if (sta & (BT(PWM_STA_EMPT1)|BT(PWM_STA_STA1+ch)))
      break;
    printf("pwm_enable:STA:%08x\n", sta); 
    sta = read_reg(PWM_STA);
  }

  if (ms_mode)
    BIT_SET_U32(ctl, PWM_CTL_MSEN1 + choff);
  else
    BIT_CLEAR_U32(ctl, PWM_CTL_MSEN1 + choff);

  PWM_CTL_WRITE(ctl);
  BIT_SET_U32(ctl, PWM_CTL_PWEN1 + choff);
  PWM_CTL_WRITE(ctl);
  puts("pwm_enable_completed\r\n");
  return 0;
}

int pwm_set(int ch, int range, int data)
{
  /* freq = 10 000 Hz / range Hz */
  write_reg(PWM_RNG1 + (ch << 2), range);
  write_reg(PWM_DAT1 + (ch << 2), data);
  return 0;
}

int pwm_prepare(int gpio_pwm0, int gpio_pwm1)
{
  puts("pwm_prepare start\r\n");
  int pins[] = { gpio_pwm0, gpio_pwm1 };
  int alt_func_pwm_0 = -1;
  int alt_func_pwm_1 = -1;
  
  switch (gpio_pwm0) {
    case 12:
      alt_func_pwm_0 = GPIO_FUNC_ALT_0;
      break;
    case 18:
      alt_func_pwm_0 = GPIO_FUNC_ALT_5;
    case -1:
      break;
  }

  switch (gpio_pwm1) {
    case 13:
      alt_func_pwm_1 = GPIO_FUNC_ALT_0;
      break;
    case 19:
      alt_func_pwm_1 = GPIO_FUNC_ALT_5;
    case -1:
      break;
  }

  if (alt_func_pwm_0 == -1 && alt_func_pwm_1 == -1) {
    printf("Invalid combination of provided gpio pins:pwm0:%d,pwm1:%d\r\n",
        gpio_pwm0, gpio_pwm1);
    return ERR_INVAL_ARG;
  }

  gpio_set_handle_pwm = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins), pwm_key);

  if (gpio_set_handle_pwm == GPIO_SET_INVALID_HANDLE) {
    printf("Failed to request gpio pins %d,%d for pwm0,pwm1\r\n", 
        gpio_pin_pwm0, gpio_pin_pwm1);
    return ERR_BUSY;
  }
    
  gpio_pin_pwm0 = gpio_pwm0;
  gpio_pin_pwm1 = gpio_pwm1;

  if (alt_func_pwm_0 != -1) {
    gpio_set_function(gpio_pin_pwm0, alt_func_pwm_0);
    pwm_enable(0, 1);
  }
  if (alt_func_pwm_1 != -1) {
    gpio_set_function(gpio_pin_pwm1, alt_func_pwm_1);
    pwm_enable(1, 1);
  }
  puts("pwm_prepare completed\r\n");
  return ERR_OK;
}

