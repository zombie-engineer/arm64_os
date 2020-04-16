#include <spi.h>
#include <gpio.h>
#include <list.h>
#include <gpio_set.h>
#include <types.h>
#include <common.h>
#include <delays.h>
#include <error.h>
#include <stringlib.h>

DECL_GPIO_SET_KEY(spie_gpio_set_key_0, "SPI_EMU_GPIO_K0");
DECL_GPIO_SET_KEY(spie_gpio_set_key_1, "SPI_EMU_GPIO_K1");
DECL_GPIO_SET_KEY(spie_gpio_set_key_2, "SPI_EMU_GPIO_K2");
DECL_GPIO_SET_KEY(spie_gpio_set_key_3, "SPI_EMU_GPIO_K3");
DECL_GPIO_SET_KEY(spie_gpio_set_key_4, "SPI_EMU_GPIO_K4");
DECL_GPIO_SET_KEY(spie_gpio_set_key_5, "SPI_EMU_GPIO_K5");

static int init_counter = 0;

static const char *spi_emulated_get_gpioset_key(void)
{
  switch(init_counter++) {
    case 0: return spie_gpio_set_key_0;
    case 1: return spie_gpio_set_key_1;
    case 2: return spie_gpio_set_key_2;
    case 3: return spie_gpio_set_key_3;
    case 4: return spie_gpio_set_key_4;
    case 5: return spie_gpio_set_key_5;
    default: return NULL;
  }
}

typedef struct spi_emulated_dev {
  spi_dev_t spidev;
  int sclk_gpio_pin;
  int mosi_gpio_pin;
  int miso_gpio_pin;
  int ce0_gpio_pin;
  int ce1_gpio_pin;
  int clk_half_period;
  gpio_set_handle_t gpio_set_handle;
  const char *gpio_set_key;
  struct list_head spi_emulated_list;
  int mode;
} spi_emulated_dev_t;

static struct spi_emulated_dev __spi_emulated_array[4];

LIST_HEAD(spi_emulated_free);
LIST_HEAD(spi_emulated_active);

spi_emulated_dev_t *__spi_emulated_alloc()
{
  struct spi_emulated_dev *dev;
  dev = list_next_entry_or_null(&spi_emulated_free,
    struct spi_emulated_dev, spi_emulated_list);

  if (dev)
    list_move(&dev->spi_emulated_list, &spi_emulated_active);

  return dev;
}

void __spi_emulated_release(struct spi_emulated_dev *dev)
{
  list_move(&dev->spi_emulated_list, &spi_emulated_free);  
}

static int spi_emulated_verbose_output = 0;
static int spi_emulated_initialized = 0;

#define ASSERT_INITIALIZED(fname) \
  if (!spi_emulated_initialized) { \
    puts(fname " error: spi not initialized\n"); \
    return SPI_ERR_NOT_INITIALIZED; \
  }


#define DECL_ASSERTED_FN(fn, ...) \
static int fn(__VA_ARGS__) \
{ \
  ASSERT_INITIALIZED(#fn);

static inline int spi_emulated_ce0_set(struct spi_emulated_dev *d)
{
  if (d->ce0_gpio_pin != -1)
    gpio_set_on(d->ce0_gpio_pin);
  return ERR_OK;
}

static inline int spi_emulated_ce0_clear(struct spi_emulated_dev *d)
{
  if (d->ce0_gpio_pin != -1)
    gpio_set_off(d->ce0_gpio_pin);
  return ERR_OK;
}

#define PULSE_SCLK(d) \
  gpio_set_on(d->sclk_gpio_pin);              \
  wait_usec(d->clk_half_period);              \
  if (d->miso_gpio_pin != -1)                 \
    *bit_out = gpio_is_set(d->miso_gpio_pin); \
  gpio_set_off(d->sclk_gpio_pin);             \
  wait_usec(d->clk_half_period);

static int spi_emulated_xmit_bit(spi_dev_t *spidev, 
    uint8_t bit_in, uint8_t *bit_out)
{
  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);

  if (spi_emulated_verbose_output)
    putc('-');

  if (d->mosi_gpio_pin != -1) {
    if (bit_in) {
      gpio_set_on(d->mosi_gpio_pin);
      if (spi_emulated_verbose_output)
        putc('1');
    } else {
      gpio_set_off(d->mosi_gpio_pin);
      if (spi_emulated_verbose_output)
        putc('0');
    }
  }
  PULSE_SCLK(d);
  return ERR_OK;
}

static inline OPTIMIZED void spi_emulated_slave_xmit_bit(spi_dev_t *spidev, 
    uint8_t bit_in, uint8_t *bit_out)
{
  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);

  if (bit_out && d->mosi_gpio_pin != -1)
    *bit_out = gpio_is_set(d->mosi_gpio_pin) ? 1 : 0;
  if (d->miso_gpio_pin != -1) {
    if (bit_in)
      gpio_set_on(d->mosi_gpio_pin);
    else
      gpio_set_off(d->mosi_gpio_pin);
  }
}


static int spi_emulated_xmit_dma(spi_dev_t *spidev, const void *data_out, void *data_in, uint32_t len)
{
  return ERR_NOT_IMPLEMENTED;
}

static int spi_emulated_slave_xmit_dma(spi_dev_t *spidev, const void *data_out, void *data_in, uint32_t len)
{
  return ERR_NOT_IMPLEMENTED;
}

static int spi_emulated_xmit_byte(spi_dev_t *spidev, char byte_in, char *byte_out)
{
  int i, err;
  char out;

  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);

  if (spi_emulated_verbose_output)
    printf("spi_emulated_xmit_byte: 0x%02x\n", byte_in);

  spi_emulated_ce0_clear(d);

  out = 0;
  for (i = 0; i < 8; ++i) {
    uint8_t c;
    err = spi_emulated_xmit_bit(&d->spidev, (byte_in >> (7 - i)) & 1, &c);
    if (err)
      return err;
    out = (out << 1) | c;
  }

  spi_emulated_ce0_set(d);
  if (byte_out)
    *byte_out = out;

  if (spi_emulated_verbose_output)
    printf("-MISO:%02x\r\n", out);

  return ERR_OK;
}

static void spi_emulated_slave_wait_ce0(spi_emulated_dev_t *spidev)
{
  while(!gpio_is_set(spidev->ce0_gpio_pin));
}

//static void __attribute__((optimize("O3"))) spi_emulated_slave_wait_sclk_rise(spi_emulated_dev_t *spidev)
//{
//  uint32_t lev0, lev1;
//  while(1) {
//    lev0 = *(uint32_t *)GPIO_REG_GPLEV0;
//    lev1 = *(uint32_t *)GPIO_REG_GPLEV1;
//    printf("%08x%08x\r\n", lev0, lev1);
//  }
 // printf("spi_emulated_slave_wait_sclk_rise:sclk at gpio_pin:%d\r\n", 
   //   spidev->sclk_gpio_pin);
//  while(gpio_is_set(spidev->sclk_gpio_pin)) {
//  }
//  while(!gpio_is_set(spidev->sclk_gpio_pin)) {
//  }
//}

struct spi_slave_stats spi_slave_stats = { 0 };

static int __attribute__((optimize("O3"))) spi_emulated_slave_xmit_byte(spi_dev_t *spidev, char byte_in, char *byte_out)
{
  // char from_mosi;
  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);
#define STATE_WAIT_CE0 0
#define STATE_EXPECT_SCLK_DROP 1
#define STATE_EXPECT_SCLK_RIZE 2

  uint32_t register gpio_level;
  uint32_t register mask_ce0 = 1<<d->ce0_gpio_pin ;
  uint32_t register mask_sclk = 1<<d->sclk_gpio_pin;
  reg32_t  register gpio_lev_reg = (reg32_t)GPIO_REG_GPLEV0;
  uint32_t register state = STATE_WAIT_CE0;
  uint32_t register mosi_shift = d->mosi_gpio_pin;
  uint8_t  register byte = 0;
  int      register i = 0;

  while(1) {
    gpio_level = read_reg(gpio_lev_reg);
    if (gpio_level & mask_ce0) {
      if (i) {
        byte = 0;
        i = 0;
      }
      continue;
    }

    switch(state) {
      case STATE_WAIT_CE0:
        if (!(gpio_level & mask_sclk))
          state = STATE_EXPECT_SCLK_RIZE;
        else
          state = STATE_EXPECT_SCLK_DROP;
        break;
      case STATE_EXPECT_SCLK_RIZE:
        if (gpio_level & mask_ce0) {
          // state = STATE_CE0_BREAK;
          *byte_out = 0xaa;
          break;
        }
        if (gpio_level & mask_sclk) {
          byte |= ((gpio_level >> mosi_shift) & 1) << (7-i);
          i++;
          state = STATE_EXPECT_SCLK_DROP;
        }
        break;
      case STATE_EXPECT_SCLK_DROP:
        if (gpio_level & mask_ce0) {
          *byte_out = 0xaa; //state = STATE_CE0_BREAK;
          break;
        }
        if (!(gpio_level & mask_sclk))
          state = STATE_EXPECT_SCLK_RIZE;
        break;
    }
    if (i == 8)
      break;
  }
  *byte_out = byte;
  // t1 = read_cpu_counter_64();

 //  from_mosi = 0;
 //  for (i = 0; i < 8; ++i) {
 //    spi_emulated_slave_wait_sclk_rise(d);
 //    // t_wait_end = read_cpu_counter_64();
 //    // spi_slave_stats.sclk_up_delta = t_wait_end - spi_slave_stats.last_sclk_up;
 //    // spi_slave_stats.last_sclk_up = t_wait_end;
 //    from_mosi |= (gpio_is_set(d->mosi_gpio_pin) << (7-i));
 //  }
 //  // t2 = read_cpu_counter_64();
 //  // spi_slave_stats.cycles_byte_xfer = t2 - t1;
 //  *byte_out = from_mosi;
  return ERR_OK;
}

static int spi_emulated_slave_xmit(spi_dev_t *spidev, const char* bytes_in, char *bytes_out, uint32_t len)
{
  int i, j;
  char from_mosi;
  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);

  for (j = 0; j < len; ++j) {
    spi_emulated_slave_wait_ce0(d);
    for (i = 0; i < 8; ++i) {
      uint8_t mosi_bit;
      spi_emulated_slave_xmit_bit(&d->spidev, (bytes_in[j] >> (7 - i)) & 1, &mosi_bit);
      from_mosi = (from_mosi << 1) | mosi_bit;
    }
    if (bytes_out)
      *bytes_out++ = from_mosi;
  }
  return ERR_OK;
}


static int spi_emulated_xmit(spi_dev_t *spidev, const char* bytes_in, char *bytes_out, uint32_t len)
{
  int i, j, st;
  char out;
  struct spi_emulated_dev *d = container_of(spidev, 
      struct spi_emulated_dev, spidev);

  spi_emulated_ce0_clear(d);

  for (j = 0; j < len; ++j) {
    if (spi_emulated_verbose_output)
      printf("spi_emulated_xmit:%02x:", bytes_in[j]);

    for (i = 0; i < 8; ++i) {
      uint8_t c;
      st = spi_emulated_xmit_bit(&d->spidev, (bytes_in[j] >> (7 - i)) & 1, &c);
      if (st) {
        printf("spidev->push_bit error: %d\n", st);
        return st;
      }

      out = (out << 1) | c;
    }
    if (bytes_out)
      *bytes_out++ = out;

    if (spi_emulated_verbose_output)
      printf("-MISO:%02x\r\n", out);
    if (spi_emulated_verbose_output)
      putc(':');
  }
  spi_emulated_ce0_set(d);
  return ERR_OK;
}

static inline void spi_gpio_set_master(
    int sclk_pin,
    int mosi_pin,
    int miso_pin,
    int ce0_pin,
    int ce1_pin
    )
{
  gpio_set_function(sclk_pin, GPIO_FUNC_OUT);

  if (mosi_pin != -1)
    gpio_set_function(mosi_pin, GPIO_FUNC_OUT);

  if (miso_pin != -1)
    gpio_set_function(miso_pin, GPIO_FUNC_IN);

  if (ce0_pin != -1) {
    gpio_set_function(ce0_pin, GPIO_FUNC_OUT);
    gpio_set_on(ce0_pin);
  }
  if (ce1_pin != -1) {
    gpio_set_function(ce1_pin, GPIO_FUNC_OUT);
    gpio_set_on(ce1_pin);
  }

  gpio_set_off(sclk_pin);
  gpio_set_off(mosi_pin);
}

static inline void spi_gpio_set_slave(
    int sclk_pin,
    int mosi_pin,
    int miso_pin,
    int ce0_pin,
    int ce1_pin
    )
{
  printf("spi_gpio_set_slave:sclk:%d,mosi:%d,miso:%d,ce0:%d\r\n",
      sclk_pin, mosi_pin,miso_pin,ce0_pin);
  gpio_set_function(sclk_pin, GPIO_FUNC_IN);
  gpio_set_pullupdown(sclk_pin, GPIO_PULLUPDOWN_EN_PULLDOWN);

  if (mosi_pin != -1) {
    gpio_set_function(mosi_pin, GPIO_FUNC_IN);
    gpio_set_pullupdown(mosi_pin, GPIO_PULLUPDOWN_EN_PULLDOWN);
  }

  if (miso_pin != -1)
    gpio_set_function(miso_pin, GPIO_FUNC_OUT);

  if (ce0_pin != -1) {
    gpio_set_function(ce0_pin, GPIO_FUNC_IN);
    gpio_set_pullupdown(ce0_pin, GPIO_PULLUPDOWN_EN_PULLDOWN);
  }
  if (ce1_pin != -1) {
    gpio_set_function(ce1_pin, GPIO_FUNC_IN);
    gpio_set_pullupdown(ce1_pin, GPIO_PULLUPDOWN_EN_PULLDOWN);
  }
}

spi_dev_t *spi_allocate_emulated(
  const char *name,
  int sclk_pin, 
  int mosi_pin, 
  int miso_pin, 
  int ce0_pin,
  int ce1_pin,
  int mode)
{
  int err;
  struct spi_emulated_dev *spidev = NULL;
  int pins[5];
  int numpins = 0;
  const char *gpioset_key = NULL;
  gpio_set_handle_t gpio_set_handle;

  if (mode != SPI_EMU_MODE_MASTER && mode != SPI_EMU_MODE_SLAVE) {
    printf("spi_allocate_emulated:invalid mode:%d\n", mode);
    return ERR_PTR(ERR_INVAL_ARG);
  }

  spidev = __spi_emulated_alloc();

  if (!spidev) {
    printf("spi_allocate_emulated: no avaliable spi emulated slots.\n");
    return ERR_PTR(ERR_NO_RESOURCE);
  }

  if (spi_emulated_verbose_output) {
    printf("spi_emulated_init: sclk: %d, mosi: %d, miso: %d, ce0: %d, ce1: %d\n",
      sclk_pin, mosi_pin, miso_pin, ce0_pin, ce1_pin);
  }

  pins[numpins++] = sclk_pin;
  if (mosi_pin != -1)
    pins[numpins++] = mosi_pin;
  if (miso_pin != -1)
    pins[numpins++] = miso_pin;
  if (ce0_pin != -1)
    pins[numpins++] = ce0_pin;
  if (ce1_pin != -1)
    pins[numpins++] = ce1_pin;

  gpioset_key = spi_emulated_get_gpioset_key();
  if (!gpioset_key) {
    printf("spi_emulated_init: out of gpioset keys.\n");
    err = ERR_NO_RESOURCE;
    goto error;
  }

  gpio_set_handle = gpio_set_request_n_pins(pins, numpins, gpioset_key);

  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
    err = ERR_BUSY;
    goto error;
  }

  if (mode == SPI_EMU_MODE_MASTER) {
    spi_gpio_set_master(sclk_pin, mosi_pin, miso_pin, ce0_pin, ce1_pin);
    spidev->spidev.xmit      = spi_emulated_xmit;
    spidev->spidev.xmit_byte = spi_emulated_xmit_byte;
    spidev->spidev.xmit_dma  = spi_emulated_xmit_dma;
  }
  else {
    spi_gpio_set_slave(sclk_pin, mosi_pin, miso_pin, ce0_pin, ce1_pin);
    spidev->spidev.xmit      = spi_emulated_slave_xmit;
    spidev->spidev.xmit_byte = spi_emulated_slave_xmit_byte;
    spidev->spidev.xmit_dma  = spi_emulated_slave_xmit_dma;
  }

  strncpy(spidev->spidev.name, name, SPIDEV_MAXNAMELEN);
  spidev->spidev.name[SPIDEV_MAXNAMELEN] = 0;

  spidev->sclk_gpio_pin = sclk_pin;
  spidev->mosi_gpio_pin = mosi_pin;
  spidev->miso_gpio_pin = miso_pin;
  spidev->ce0_gpio_pin = ce0_pin;
  spidev->ce1_gpio_pin = ce1_pin;
  spidev->gpio_set_handle = gpio_set_handle;
  spidev->gpio_set_key = gpioset_key;
  spidev->clk_half_period = 1;
  spidev->mode = mode;

  printf("SPIemu: allocated at %p with name: '%s'\n"
         "SPIemu: SCLK:%d,MOSI:%d,MISO:%d,CE0:%d,CE1:%d\n"
         "SPIemu: gpio:%d,key:%s,mode:%d\n", 
      spidev, spidev->spidev.name, sclk_pin, mosi_pin,
      miso_pin, ce0_pin, ce1_pin, gpio_set_handle, gpioset_key,
      mode);

  return &spidev->spidev;

error:
  if (spidev)
    __spi_emulated_release(spidev);
  return ERR_PTR(err);
}

int spi_deallocate_emulated(spi_dev_t *s)
{
  int err;
  struct spi_emulated_dev *d = container_of(s, 
      struct spi_emulated_dev, spidev);

#define PIN(x) d->x ## _gpio_pin
#define SPI_GPIO_OFF(x)\
  if (PIN(x) != -1) {\
    gpio_set_off(PIN(x));\
    gpio_set_function(PIN(x), GPIO_FUNC_OUT);\
  }

  SPI_GPIO_OFF(sclk);
  SPI_GPIO_OFF(mosi);
  SPI_GPIO_OFF(miso);
  SPI_GPIO_OFF(ce0);
  SPI_GPIO_OFF(ce1);

  err = gpio_set_release(d->gpio_set_handle, d->gpio_set_key);
  if (err != ERR_OK) {
    printf("spi_deallocate_emulated: failed to release gpioset:%d:%s:%d\r\n", 
        d->gpio_set_handle,
        d->gpio_set_key, err);
    return err;
  }
  __spi_emulated_release(d);
  puts("spi_deallocate_emulated: success.\r\n");
  return ERR_OK;
}

void spi_emulated_init()
{
  int i;
  init_counter = 0;
  memset(__spi_emulated_array, 0, sizeof(__spi_emulated_array));
  for (i = 0; i < ARRAY_SIZE(__spi_emulated_array); ++i) {
    struct spi_emulated_dev *d = &__spi_emulated_array[i];
    list_add(&d->spi_emulated_list, &spi_emulated_free);
  }
  spi_emulated_initialized = 1;
}

void spi_emulated_print_info()
{
  puts("spi_emulated debug info:\n");
  printf(" verbose_output: %d\n", spi_emulated_verbose_output);
  printf(" initialized   : %d\n", spi_emulated_initialized);
}

void spi_emulated_set_clk(spi_dev_t *s, int val)
{
  struct spi_emulated_dev *d = container_of(s, 
      struct spi_emulated_dev, spidev);
  printf("spi_emulated: setting clk period to %d micro seconds\n\r", val);
  d->clk_half_period = val;
}

void spi_emulated_set_log_level(int log_level)
{
  spi_emulated_verbose_output = log_level;
}
