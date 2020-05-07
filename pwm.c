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
#include <memory/static_slot.h>
#include <stringlib.h>

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
#define PWM_CTL_CLRF2 13
#define PWM_CTL_USEF2 13
#define PWM_CTL_POLA2 12
#define PWM_CTL_SBIT2 11
#define PWM_CTL_RPTL2 10
#define PWM_CTL_MODE2 9
#define PWM_CTL_PWEN2 8
#define PWM_CTL_MSEN1 7
#define PWM_CTL_CLRF1 6
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

#define PWM_CTL_WRITE(x)\
  do {\
    write_reg(PWM_CTL, x);\
    wait_msec(20);\
    sta = read_reg(PWM_STA);\
  } while (BIT_IS_SET(sta, PWM_STA_BERR))

#define PWM_RELEASE_GPIO_HANDLE(pwm) \
  do {\
    int __err;\
    __err = gpio_set_release(pwm->gpio_handle, pwm->gpio_key);\
    if (__err != ERR_OK) {\
      printf("failed to release gpioset:%d:%s:%d\r\n",\
          pwm->gpio_handle, pwm->gpio_key, __err);\
      kernel_panic("pwm_create_failpath_panic");\
    }\
  } while(0)

static int pwm_bcm2835_initialized = 0;
static gpio_set_handle_t gpio_set_handle_pwm;
static int gpio_pin_pwm0;
static int gpio_pin_pwm1;

struct pwm_bcm2835 {
  struct pwm pwm;
  int gpio_pin;
  const char *gpio_key;
  gpio_set_handle_t gpio_handle;
  int channel;
  struct list_head STATIC_SLOT_OBJ_FIELD(pwm_bcm2835);
};


DECL_GPIO_SET_KEY(pwm_key, "PWM_KEYS_______");
DECL_GPIO_SET_KEY(bcm2835_gpio_key_pwm_chan_0, "BCM2835_PWM__0_");
DECL_GPIO_SET_KEY(bcm2835_gpio_key_pwm_chan_1, "BCM2835_PWM__1_");

DECL_STATIC_SLOT(struct pwm_bcm2835, pwm_bcm2835, 2)

#define PRINT_PWM_BIT(regname, regval, bit)\
  if (regval & (1<< PWM_ ## regname ## _ ## bit))\
    puts(#bit "-");

#define PWM_PRINT_STA(regval, bit) PRINT_PWM_BIT(STA, regval, bit)
#define PWM_PRINT_CTL(regval, bit) PRINT_PWM_BIT(CTL, regval, bit)

static inline void pwm_print_sta(const char *prefix, uint32_t sta, int newline)
{
  puts(prefix);
  PWM_PRINT_STA(sta, FULL1);
  PWM_PRINT_STA(sta, EMPT1);
  PWM_PRINT_STA(sta, WERR1);
  PWM_PRINT_STA(sta, RERR1);
  PWM_PRINT_STA(sta, GAPO1);
  PWM_PRINT_STA(sta, GAPO2);
  PWM_PRINT_STA(sta, GAPO3);
  PWM_PRINT_STA(sta, GAPO4);
  PWM_PRINT_STA(sta, BERR);
  PWM_PRINT_STA(sta, STA1);
  PWM_PRINT_STA(sta, STA2);
  PWM_PRINT_STA(sta, STA3);
  if (newline)
    puts("\r\n");
}

static inline void pwm_print_ctl(const char *prefix, uint32_t ctl, int newline)
{
  puts(prefix);
  PWM_PRINT_CTL(ctl, PWEN1);
  PWM_PRINT_CTL(ctl, MODE1);
  PWM_PRINT_CTL(ctl, RPTL1);
  PWM_PRINT_CTL(ctl, SBIT1);
  PWM_PRINT_CTL(ctl, POLA1);
  PWM_PRINT_CTL(ctl, USEF1);
  PWM_PRINT_CTL(ctl, CLRF1);
  PWM_PRINT_CTL(ctl, MSEN1);
  PWM_PRINT_CTL(ctl, PWEN2);
  PWM_PRINT_CTL(ctl, MODE2);
  PWM_PRINT_CTL(ctl, RPTL2);
  PWM_PRINT_CTL(ctl, SBIT2);
  PWM_PRINT_CTL(ctl, POLA2);
  PWM_PRINT_CTL(ctl, USEF2);
  PWM_PRINT_CTL(ctl, CLRF2);
  PWM_PRINT_CTL(ctl, MSEN2);
  if (newline)
    puts("\r\n");
}

static inline void pwm_print_ctl_sta(const char *prefix, uint32_t ctl, uint32_t sta)
{
  puts(prefix);
  pwm_print_ctl(":ctl:", ctl, 0);
  pwm_print_sta(",sta:", sta, 1);
}

int pwm_enable(int ch, int ms_mode)
{
  uint32_t sta, ctl;
  int choff = ch << 3;
  printf("pwm_enable:ch:%d:msmode:%d\r\n", ch, ms_mode);

  if (ch != 0 && ch != 1)
    return ERR_INVAL_ARG;

  ctl = read_reg(PWM_CTL);
  BIT_CLEAR_U32(ctl, PWM_CTL_PWEN1 + choff);
  PWM_CTL_WRITE(ctl);

  while(1) {
    sta = read_reg(PWM_STA);
    pwm_print_sta("pwm ctl off:sta:", sta, 1);

    if (sta & (BT(PWM_STA_EMPT1)|BT(PWM_STA_STA1+ch)))
      break;
    printf("pwm_enable:STA:%08x\n", sta); 
    sta = read_reg(PWM_STA);
  }

  if (ms_mode)
    BIT_SET_U32(ctl, PWM_CTL_MSEN1 + choff);
  else
    BIT_CLEAR_U32(ctl, PWM_CTL_MSEN1 + choff);
  BIT_SET_U32(ctl, PWM_CTL_PWEN1 + choff);
  PWM_CTL_WRITE(ctl);
  puts("pwm_enable:completed\r\n");
  return ERR_OK;
}

int pwm_set(int ch, int range, int data)
{
  /* freq = 10 000 Hz / range Hz */
  write_reg(PWM_RNG1 + (ch << 2), range);
  write_reg(PWM_DAT1 + (ch << 2), data);
  return 0;
}

int pwm_bcm2835_disable(struct pwm *pwm)
{
  uint32_t sta;
  uint32_t ctl;
  struct pwm_bcm2835 *p = container_of(pwm, struct pwm_bcm2835, pwm);
  int choff = p->channel << 3;
  ctl = read_reg(PWM_CTL);
  BIT_CLEAR_U32(ctl, PWM_CTL_PWEN1 + choff);
  PWM_CTL_WRITE(ctl);

  gpio_set_off(p->gpio_pin);
  gpio_set_function(p->gpio_pin, GPIO_FUNC_OUT);
  PWM_RELEASE_GPIO_HANDLE(p);
  pwm_bcm2835_release(p);
  return ERR_OK;
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

static int pwm_bcm2835_set_clk_freq(struct pwm *pwm, int hz)
{
  return bcm2835_set_pwm_clk_freq(hz);
}

static int pwm_bcm2835_set(struct pwm *pwm, uint32_t range, uint32_t data)
{
  struct pwm_bcm2835 *p = container_of(pwm, struct pwm_bcm2835, pwm);
  write_reg(PWM_RNG1 + (p->channel << 2), range);
  write_reg(PWM_DAT1 + (p->channel << 2), data);
  return ERR_OK;
}

static int pwm_bcm2835_set_range(struct pwm *pwm, uint32_t range)
{
  struct pwm_bcm2835 *p = container_of(pwm, struct pwm_bcm2835, pwm);
  write_reg(PWM_RNG1 + (p->channel << 2), range);
  return ERR_OK;
}

static int pwm_bcm2835_set_data(struct pwm *pwm, uint32_t data)
{
  struct pwm_bcm2835 *p = container_of(pwm, struct pwm_bcm2835, pwm);
  // printf("pwm_bcm2835_set_data:%p,ch:%d,data:%d\r\n", p, p->channel, data);
  write_reg(PWM_DAT1 + (p->channel << 2), data);
  return ERR_OK;
}

struct pwm *pwm_bcm2835_create(int gpio_pin, int ms_mode)
{
  int err;
  int channel;
  const char *gpio_key;
  int gpio_func;
  struct pwm_bcm2835 *p;
  if (!pwm_bcm2835_initialized) {
    printf("pwm_bcm2835_create:not initialized\r\n");
    return ERR_PTR(ERR_NOT_INIT);
  }
 
  p = pwm_bcm2835_alloc();
  if (!p)
    return ERR_PTR(ERR_BUSY);

  if (gpio_pin == 12 || gpio_pin == 18) {
    channel = 0;
    gpio_key = bcm2835_gpio_key_pwm_chan_0;
  }
  else if (gpio_pin == 13 || gpio_pin == 19) {
    channel = 1;
    gpio_key = bcm2835_gpio_key_pwm_chan_1;
  }
  else {
    printf("pwm_create:invalid gpio pin in argument:%d\r\n", gpio_pin);
    err = ERR_INVAL_ARG;
    goto err;
  }

  if (gpio_pin - channel == 12)
    gpio_func = GPIO_FUNC_ALT_0;
  else
    gpio_func = GPIO_FUNC_ALT_5;

  p->gpio_handle = gpio_set_request_1_pins(gpio_pin, gpio_key);
  if (p->gpio_handle == GPIO_SET_INVALID_HANDLE) {
    printf("pwm_bcm2835_prepare_channel_x:failed to request gpio pin:%d, key:%s\r\n", 
        gpio_pin, gpio_key);
    err = ERR_BUSY;
    goto err;
  }

  p->gpio_key = gpio_key;
  p->gpio_pin = gpio_pin;

  gpio_set_function(gpio_pin, gpio_func);
  err = pwm_enable(channel, ms_mode);
  if (err != ERR_OK)
    goto err_enable;

  p->channel = channel;
  p->pwm.set_clk_freq = pwm_bcm2835_set_clk_freq;
  p->pwm.set = pwm_bcm2835_set;
  p->pwm.set_range = pwm_bcm2835_set_range;
  p->pwm.set_data = pwm_bcm2835_set_data;
  p->pwm.release = pwm_bcm2835_disable;

  printf("pwm_bcm2835_create:complted:%p", p);
  return &p->pwm;

err_enable:
  gpio_set_function(gpio_pin, GPIO_FUNC_OUT);
  gpio_set_off(gpio_pin);
// err_handle:
  PWM_RELEASE_GPIO_HANDLE(p);
err:
  pwm_bcm2835_release(p);
  return ERR_PTR(err);
}

int bcm2835_set_pwm_clk_freq(int hz)
{
  int err;
  const uint32_t pwm_clock_hz = 19200000;
  /* freq = 19.2 MHz = 19 200 000 */
  uint32_t divi = pwm_clock_hz / hz; 
  /* freq = 19.2 MHz / 192 = 100 KHz = 19200000 / 192 = 100000 */
  uint32_t divf = (((float)pwm_clock_hz / hz) - divi) * 10000;

  printf("bcm2835_set_pwm_clk_freq:arg:%dHz, divi:%d, divf:%d\r\n",
      hz, divi, divf);

  err = cm_set_clock(CM_CLK_ID_PWM, CM_SETCLK_SRC_OSC, CM_SETCLK_MASH_OFF, divi, divf);
  if (err != ERR_OK) {
    printf("bcm2835_set_pwm_clk_freq:error:%d (%s)\n", err, set_clock_err_to_str(err));
    return err;
  }
  return ERR_OK;
}

void pwm_bcm2835_init()
{
  STATIC_SLOT_INIT_FREE(pwm_bcm2835);
  pwm_bcm2835_initialized = 1;
}

