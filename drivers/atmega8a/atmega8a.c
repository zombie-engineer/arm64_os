#include <drivers/atmega8a.h>
#include <atmcmd.h>
#include <gpio.h>
#include <gpio_set.h>
#include <spi.h>
#include <error.h>
#include <common.h>
#include <delays.h>
#include <stringlib.h>
#include <drivers/display/nokia5110.h>
#include <pwm.h>

#define atlog(fmt, ...) printf(fmt "\r\n", ## __VA_ARGS__)

#define ATMEGA8A_PAGES_PER_FLASH 128
#define ATMEGA8A_EEPROM_PAGES 128
#define ATMEGA8A_BYTES_PER_WORD 2

#define ATMEGA8A_FLASH_WORDS_PER_PAGE 32
#define ATMEGA8A_FLASH_BYTES_PER_PAGE (ATMEGA8A_FLASH_WORDS_PER_PAGE * ATMEGA8A_BYTES_PER_WORD)
#define ATMEGA8A_FLASH_BYTES_TOTAL (ATMEGA8A_FLASH_BYTES_PER_PAGE * ATMEGA8A_PAGES_PER_FLASH)

typedef struct atmega8a_dev {
  spi_dev_t *spi;
  int gpio_pin_reset;
  gpio_set_handle_t gpio_set_handle;
  int debug_level;
} atmega8a_dev_t;

static atmega8a_dev_t atmega8a_dev = {
  .spi = 0,
  .gpio_pin_reset = -1,
  .gpio_set_handle = GPIO_SET_INVALID_HANDLE,
  .debug_level = 0
};

DECL_GPIO_SET_KEY(atmega8a_reset_key, "ATMEGA8_GPIO_RS");

#define CMD_PROGRAMMIG_ENABLE    0x000053ac

#define CMD_READ_FUSE_BITS_LOW   0x00000050

#define CMD_READ_FUSE_BITS_HIGH 0x00000858

#define EXTRACT_BYTE(res, x) ((char)((res >> (x * 8)) & 0xff))

static inline uint32_t get_read_signature_bytes_cmd(int byte_idx)
{
  return 0x00000030 | (byte_idx << 16);
}

static inline int atmega8a_is_init()
{
  return atmega8a_dev.spi != NULL;
}

#define _FLASH_WORD_ADDR(word_addr)  (((word_addr)&0xf00)|(((word_addr)&0xff)<<16))
#define _EEPROM_BYTE_ADDR(byte_addr) (((byte_addr)&0x100)|(((byte_addr)&0xff)<<16))

/* CMD_FLASH_READ_BYTE 
 * Instruction format:
 * [0010 H000][0000 aaaa][bbbb bbbb][oooo oooo]
 * H - high or low byte of a word 
 * aaaa bbbb bbbb - word address 0 - 4095
 *
 * Flash memory is 128 Pages of 32 Words each. Word size = 2 bytes.
 * Thus 128 * 32 = 4096 Words = 8912 Bytes.
 * 4K Word address space is covered by 12 bits aaaa bbbb bbbb
 * Excat byte in a word (low or high) is then selected by bit H
 */

#define _FLASH_WORD_BYTE(H) ((H & 1) << 3)

#define CMD_FLASH_READ_BYTE(word_addr, H) (0x20|_FLASH_WORD_BYTE(H)|_FLASH_WORD_ADDR(word_addr))

/* CMD_FLASH_LOAD_BYTE
 * Instruction format:
 * [0100 H000][0000 xxxx][xxxb bbbb][iiii iiii]
 * H - high or low byte of a word 
 * b bbbb - word address in page 0-31
 */
#define CMD_FLASH_LOAD_BYTE(word_addr, H, byte) (0x40|_FLASH_WORD_BYTE(H)|(word_addr&0b11111)<<16|((byte&0xff)<<24))

/* CMD_FLASH_WRITE_PAGE
 * Instruction format:
 * [0100 1100][0000 aaaa][bbbx xxxx][xxxx xxxx]
 * aaaa bbb - page address 0-127
 */
#define CMD_FLASH_WRITE_PAGE(page_number) (0x4c\
    |((page_number&0b1111000)<<8)\
    |((page_number&0b0000111)<<16))


/* CMD_READ_EEPROM
 * Instruction format:
 * [1010 0000][00xx xxxa][bbbb bbbb][oooo oooo]
 * a bbbb bbbb - byte address 0 - 511
 *
 * EEPROM memory is 128 Pages of 4 Bytes each.
 */
#define CMD_READ_EEPROM_BYTE(byte_addr) (0xa0 | _EEPROM_BYTE_ADDR(byte_addr))

/* CMD_WRITE_EEPROM
 * Instruction format:
 * [1100 0000][00xx xxxa][bbbb bbbb][iiii iiii]
 * a bbbb bbbb - byte address 0 - 511
 *
 * EEPROM memory is 128 Pages of 4 Bytes each.
 */
#define CMD_WRITE_EEPROM_BYTE(byte_addr, byte) (0xc0 | _EEPROM_BYTE_ADDR(byte_addr))
int atmega8a_cmd(spi_dev_t *spidev, const char *cmd, char *res)
{
  int ret;

  ret = spidev->xmit(spidev, cmd, res, 4);
  if (ret != ERR_OK) {
    atlog("spidev->xmit failed: %d", ret);
    return ret;
  }

  if (atmega8a_dev.debug_level > 2) {
    atlog("cmd:%02x:%02x:%02x:%02x -> res:%02x:%02x:%02x:%02x",
        cmd[0], cmd[1], cmd[2], cmd[3],
        res[0], res[1], res[2], res[3]
    );
  }
  return ERR_OK;
}

static inline int atmega8a_read_signature(spi_dev_t *spidev, uint32_t *signature)
{
  int i;
  uint32_t tmp;

  char res[4];
  char cmd[4];

  tmp = 0;
  for (i = 0; i < 3; ++i) {
    cmd[0] = 0x30;
    cmd[1] = 0x00;
    cmd[2] = i & 0x3;
    cmd[3] = 0x00;
    if (atmega8a_cmd(spidev, cmd, res) != ERR_OK)
      return ERR_FATAL;
    tmp = (tmp << 8) | res[3];
  }
  *signature = tmp;
  return ERR_OK;
}

int atmega8a_init(spi_dev_t *spidev, int gpio_pin_reset)
{
  int ret;
  char res[4];
  char cmd[4];
  gpio_set_handle_t gpio_set_handle;

  uint32_t signature;

  gpio_set_handle = gpio_set_request_1_pins(gpio_pin_reset, atmega8a_reset_key);
  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE)
    return ERR_BUSY;

  gpio_set_function(gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_off(gpio_pin_reset);

  /* Programming enable */
  gpio_set_on(gpio_pin_reset);
  wait_msec(10);
  gpio_set_off(gpio_pin_reset);
  wait_msec(10);
  gpio_set_on(gpio_pin_reset);
  wait_msec(10);
  gpio_set_off(gpio_pin_reset);
  wait_msec(10);

  cmd[0] = 0xac;
  cmd[1] = 0x53;
  cmd[2] = 0x00;
  cmd[3] = 0x00;
  if ((ret = atmega8a_cmd(spidev, cmd, res)) != ERR_OK)
    return ERR_FATAL;

  if (res[2] != 0x53) {
    atlog("CMD_PROGRAMMING_ENABLE failed: 0x53 not returned by chip. Should write retry code. Exiting..");
    return ERR_FATAL;
  }

  if (atmega8a_read_signature(spidev, &signature) != ERR_OK)
      return ERR_FATAL;

  atlog("Signature bytes: %08x", signature);

  atmega8a_dev.spi = spidev;
  atmega8a_dev.gpio_pin_reset = gpio_pin_reset;
  atmega8a_dev.gpio_set_handle = gpio_set_handle;
  return ERR_OK;
}

int atmega8a_deinit()
{
  int err;
  err = spi_deallocate_emulated(atmega8a_dev.spi);
  if (err != ERR_OK) {
    printf("atmega8a_deinit: failed to stop spi:%d\r\n", err);
    return err;
  }

  gpio_set_off(atmega8a_dev.gpio_pin_reset);
  gpio_set_function(atmega8a_dev.gpio_pin_reset, GPIO_FUNC_OUT);
  err = gpio_set_release(atmega8a_dev.gpio_set_handle, atmega8a_reset_key);
  if (err != ERR_OK) {
    printf("atmega8a_deinit: failed to release gpioset:%d:%s:%d\r\n", 
        atmega8a_dev.gpio_set_handle,
        atmega8a_reset_key, err);
    return err;
  }
  
  printf("atmega8a_deinit:%d .\r\n", err);
  return err;
}

int atmega8a_reset()
{
  atlog("atmega8a_reset");
  if (!atmega8a_is_init()) {
    atlog("atmega8a_reset: device is not initialized.");
    return ERR_NOT_INIT;
  }
  gpio_set_on(atmega8a_dev.gpio_pin_reset);
  wait_msec(100);
  gpio_set_off(atmega8a_dev.gpio_pin_reset);
  wait_msec(100);
  gpio_set_on(atmega8a_dev.gpio_pin_reset);
  atlog("atmega8a_reset: completed");
  return ERR_OK;
}

static int atmega8a_flash_page_fill(const uint8_t *src)
{
  char res[4];
  char cmd[4];

  /* page-local word address 0-31 */
  uint16_t wordaddr;
  atlog("page fill started.");
  for (wordaddr = 0; wordaddr < 32; ++wordaddr) {
    cmd[0] = 0x40;
    cmd[1] = 0x00;
    cmd[2] = wordaddr & 0x1f;
    cmd[3] = *(src++);

    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;

    cmd[0] = 0x48;
    cmd[1] = 0x00;
    cmd[2] = wordaddr & 0x1f;
    cmd[3] = *(src++);
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;
  }
  atlog("page fill completed.");
  return ERR_OK;
}

static int atmega8a_flash_page_write(int page, int check_value, int check_addr) 
{
  char res[4];
  char cmd[4];
  atlog("page write started.");

  if (page >= ATMEGA8A_PAGES_PER_FLASH) {
    atlog("atmega8a_flash_page_write: page number too big: %d", page);
    return ERR_INVAL_ARG;
  }

  /* Check this before doing the actual write */
  if ((check_value != 0xff) && (check_addr > ATMEGA8A_FLASH_BYTES_TOTAL)) {
    atlog("Failed to check value after page write. check_addr out of range.");
    return ERR_FATAL;
  }

  cmd[0] = 0x4c;
  cmd[1] = (page >> 3) & 0xf;
  cmd[2] = (page & 7) << 5;
  cmd[3] = 0x00;

  if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
    return ERR_FATAL;

  if (check_value != 0xff) {
    int i;
    int high = check_addr & 1;

    check_addr += page * ATMEGA8A_FLASH_BYTES_PER_PAGE;
    check_addr /= ATMEGA8A_BYTES_PER_WORD;

    cmd[0] = 0x20 | (high << 3);
    cmd[1] = (check_addr >> 8) & 0xf;
    cmd[2] = check_addr & 0xff;
    cmd[3] = 0x00;

    for (i = 0; i < 64; ++i) {
      if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
        return ERR_FATAL;

      if (res[3] == check_value) 
        goto success;

      wait_msec(500);
    }
    atlog("page write check failed.");
    return ERR_FATAL;
  }

success:
  atlog("page write completed.");
  return ERR_OK;
}

static inline int mem_find_first_not(const uint8_t *buf, int sz, char fill_value)
{
  int i;
  for (i = 0; i < sz; ++i)
    if (buf[i] != fill_value)
      return i;
  return -1;
}

static inline int atmega8a_write_flash_check(int start_page, int end_page, int buf_sz)
{
  if (start_page >= ATMEGA8A_PAGES_PER_FLASH) {
    atlog("atmega8a_write_flash: from_page argument too big: %d", start_page);
    return ERR_INVAL_ARG;
  }

  if (end_page >= ATMEGA8A_PAGES_PER_FLASH) {
    atlog("atmega8a_write_flash: last page index outside of flash memory space: %d", end_page);
    return ERR_INVAL_ARG;
  }

  if (buf_sz % ATMEGA8A_FLASH_BYTES_PER_PAGE) {
    atlog("atmega8a_write_flash: source buf size (%d) not aligned to page size %d.",
      buf_sz, ATMEGA8A_FLASH_BYTES_PER_PAGE);
    return ERR_INVAL_ARG;
  }
  return ERR_OK;
}

int atmega8a_write_flash(const void *buf, int sz, int from_page)
{
  int ret;
  int page = from_page;
  int check_addr;
  char check_value;
  int num_pages = sz / ATMEGA8A_FLASH_BYTES_PER_PAGE;
  int end_page = from_page + num_pages;
  const uint8_t *src = buf;

  atlog("Writing to pages: %d to %d", page, end_page - 1);
  ret = atmega8a_write_flash_check(from_page, end_page, sz);
  if (ret != ERR_OK)
    return ret;

  while(page < end_page) {
    atlog("Writing to atmega8a: source:%08llx, page: %d", src, page);
    check_addr = mem_find_first_not(src, ATMEGA8A_FLASH_BYTES_PER_PAGE, 0xff);
    if (check_addr != -1) {
      check_value = src[check_addr];
      if (atmega8a_flash_page_fill(src) != ERR_OK) {
        atlog("atmega8a_write_flash: failed to fill flash page");
        return ERR_FATAL;
      }
      if (atmega8a_flash_page_write(page, check_value, check_addr) != ERR_OK) {
        atlog("atmega8a_write_flash: failed to write flash page");
        return ERR_FATAL;
      }
    } else
      atlog("Skipping page with 0xff");

    page++;
    src += ATMEGA8A_FLASH_BYTES_PER_PAGE;
  }

  atlog("Pages %d to %d successfully written to flash memory.",
    from_page, 
    end_page - 1);

  return ERR_OK;
}

int atmega8a_read_flash_memory(void *buf, int sz, int byte_addr)
{
  char res[4];
  char cmd[4] = { 0x20, 0x00, 0x00, 0x00 };
  char *ptr = (char *)buf; 
  char *end = ptr + sz;
  uint16_t wordaddr = byte_addr / 2;
  int high = byte_addr & 1;

  while(ptr < end) {
    cmd[0] = 0x20 | (high << 3);
    cmd[1] = (wordaddr >> 8) & 0xf;
    cmd[2] = wordaddr & 0xff;
    cmd[3] = 0x00;
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;

    *(ptr++) = res[3];

    wordaddr += high;
    high = high ^ 1;
  }

  return ERR_OK;
}

int atmega8a_write_eeprom(const void *buf, int sz, int byte_addr)
{
  char res[4];
  char cmd[4] = { 0xc0, 0x00, 0x00, 0x00 };

  char *src = (char *)buf; 
  int i;
  for (i = 0; i < sz; ++i) {
    cmd[1] = (byte_addr >> 8) & 1;
    cmd[2] = byte_addr & 0xff;
    cmd[3] = *src;
    src++;
    byte_addr++;
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;
  }
  return ERR_OK;
}

int atmega8a_read_eeprom_memory(void *buf, int sz, int addr)
{
  char res[4];
  char cmd[4] = { 0xa0, 0x00, 0x00, 0x00 };
  char *ptr = (char *)buf; 
  size_t i;
  for (i = 0; i < sz; ++i) {
    cmd[1] = ((addr + i) >> 8) & 1;
    cmd[2] = (addr + i) & 0xff;
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;
    *ptr++ = res[3];
  }
  return ERR_OK;
}

int atmega8a_write_fuse_bits_low(char fuse_low)
{
  char res[4];
  char cmd_low[4]  = {0xac, 0xa0, 0x00, 0x00}; // CMD_WRITE_FUSE_BITS_LOW
  cmd_low[3] = fuse_low;

  if (atmega8a_cmd(atmega8a_dev.spi, cmd_low, res) != ERR_OK)
    return ERR_FATAL;

  return ERR_OK;
}

int atmega8a_read_fuse_bits(char *out_fuse_low, char *out_fuse_high)
{
  char res[4];
  char cmd_low[4]  = {0x50, 0x00, 0x00, 0x00}; // CMD_READ_FUSE_BITS_LOW
  char cmd_high[4] = {0x58, 0x80, 0x00, 0x00}; // CMD_READ_FUSE_BITS_HIGH

  if (atmega8a_cmd(atmega8a_dev.spi, cmd_low, res) != ERR_OK)
    return ERR_FATAL;

  *out_fuse_low = res[3];

  if (atmega8a_cmd(atmega8a_dev.spi, cmd_high, res) != ERR_OK)
    return ERR_FATAL;

  *out_fuse_high = res[3];
  return ERR_OK;
}

int atmega8a_get_flash_size()
{
  return ATMEGA8A_FLASH_BYTES_TOTAL;
}

int atmega8a_get_eeprom_size()
{
  return 512;
}

int atmega8a_chip_erase()
{
  char cmd[4];
  char res[4];

  cmd[0] = 0xac;
  cmd[1] = 0x80;
  cmd[2] = 0x00;
  cmd[3] = 0x00;

  if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
    return ERR_FATAL;

  return ERR_OK;
}

int atmega8a_read_lock_bits(char *out_lock_bits)
{
  char cmd[4] = { 0x58, 0x00, 0x00, 0x00 };
  char res[4];

  if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
    return ERR_FATAL;

  *out_lock_bits = res[3];

  return ERR_OK;
}


int atmega8a_write_lock_bits(char lock_bits)
{
  char cmd[4] = { 0x5c, 0x00, 0x00, 0x00 };
  char res[4];

  if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
    return ERR_FATAL;

  return ERR_OK;
}

#define BLB12 "BLB12"
#define BLB11 "BLB11"
#define BLB02 "BLB02"
#define BLB01 "BLB01"
#define LB2 "LB02"
#define LB1 "LB01"

int atmega8a_lock_bits_describe(char *buf, int bufsz, char lock_bits)
{
  const char *descs[] = { LB1, LB2, BLB01, BLB02, BLB11, BLB12 };
  int i, n, is_before;

  n = 0;
  is_before = 0;
  
  for (i = 0; i < ARRAY_SIZE(descs) - 1; ++i) {
    n += snprintf(buf + n, bufsz - n, 
        "%s%s:%c", 
        is_before ? "," : "", 
        descs[i],
        // lock bit is programmed when it is "0"
        // "1" for unprogrammed
        lock_bits & (1<<i) ? 'n' : 'y'
    );
    is_before = 1;
  }

  return n;
}

#define atm_err_check(err, msg)\
  if (err != ERR_OK) {\
        printf(msg ":err:%d\r\n", err);\
        return err;\
  }

#define atm_status_check(status, msg)\
  if (status != ATMCMD_STAT_OK) {\
        printf(msg ":status:%d\r\n", status);\
        return ERR_GENERIC;\
  }

#define atm_send_cmd(spi, cmd,resp) \
  err = spi->xmit_byte(spi, cmd, resp);\
  atm_err_check(err, "atm_send_cmd");

#define atm_get_response(spi, dst) \
  do {\
    _Static_assert(sizeof(dst) == 1, "atm_get_response arg1 should be 1-bytes width");\
    err = spi->xmit_byte(spi, ATMCMD_CMD_GET_RESP, &dst);\
    printf("atm_get_response:%02x\r\n", dst);\
    atm_err_check(err, "atm_get_response");\
  } while(0);

#define atm_recv2(spi, cmd2, dst) \
  _Static_assert(sizeof(dst) == 2, "atm_recv2 arg2 should be 2-bytes width");\
  err = spi->xmit(spi, cmd2, (char *)&dst, sizeof(dst));\
  atm_err_check(err, "atm_recv2");

DECL_GPIO_SET_KEY(pwm0_key, "PWM0_____KEY___");
static gpio_set_handle_t gpio_set_handle_pwm;
static int gpio_pin_pwm0;

int pwm_prepare()
{
  puts("pwm_prepare start\r\n");
  gpio_pin_pwm0 = 18;
  gpio_set_handle_pwm = gpio_set_request_1_pins(gpio_pin_pwm0, pwm0_key);
  if (gpio_set_handle_pwm == GPIO_SET_INVALID_HANDLE) {
    printf("Failed to request gpio pin %d for pwm0\r\n", gpio_pin_pwm0);
    return ERR_BUSY;
  }
  gpio_set_function(gpio_pin_pwm0, GPIO_FUNC_ALT_5);
  pwm_enable(0, 1);
  puts("pwm_prepare completed\r\n");
  return ERR_OK;
}

static inline void pwm_servo(uint16_t value)
{
  int err;
  int range_start = 60;
  int range_end = 250;
  int range = range_end - range_start;
  float norm = (float)(value & 0x3ff) / 0x3ff;
  int pt = range_start + range * norm;
  printf("pwm_servo:%d->%d\r\n", (int)value, pt);
  err = pwm_set(0, 2000, pt);
  if (err != ERR_OK) {
    printf("Failed to enable pwm: %d\r\n", err);
    return err;
  }
  wait_msec(10);
}

int x = 0;

static inline void on_sample(uint16_t value)
{
    uint16_t v;
    int y;
    float y_norm;
    char chan = (value & 0x8000) ? 1: 0;
    v = value & ((1<<10) - 1);
    printf("%d::%04x\r\n", chan, v);
    pwm_servo(v);
    if (!chan)
      return;
    y_norm = (float)v / 0x3ff;
    y = 24 * y_norm;
     
    nokia5110_draw_dot(x, y);
    nokia5110_draw_dot(x, 24);
    if (++x > NOKIA5110_PIXEL_SIZE_X) {
      x = 0;
      nokia5110_blank_screen();
    }
}

int atmega8a_spi_master_test()
{
  spi_dev_t *s = atmega8a_dev.spi;
  int err;
  char atm_status = ATMCMD_STAT_ERR;
  uint16_t value = 0;
  uint16_t cmd_adc = 0x1122;
  err = pwm_prepare();
  if (err) {
    printf("Failed to prepare pwm: %d\r\n", err);
    return err;
  }
 // spi_emulated_set_log_level(1);
  puts("atmega8a_spi_test\r\n");
  spi_emulated_set_clk(s, 16);
  while(atm_status != 0xd5)
    atm_get_response(s, atm_status);
  atm_send_cmd(s, ATMCMD_CMD_RESET, NULL);
  atm_send_cmd(s, ATMCMD_CMD_ADC_START, NULL);
  while(1) {
    atm_recv2(s, &cmd_adc, value);
    on_sample(value);
    wait_msec(10);
  }
  return 0;
}
