#include <drivers/atmega8a.h>
#include <gpio.h>
#include <gpio_set.h>
#include <spi.h>
#include <error.h>
#include <common.h>
#include <delays.h>

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
} atmega8a_dev_t;

static atmega8a_dev_t atmega8a_dev = {
  .spi = 0,
  .gpio_pin_reset = -1,
  .gpio_set_handle = GPIO_SET_INVALID_HANDLE
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

  ret = spidev->xmit(cmd, res, 4);
  if (ret != ERR_OK) {
    atlog("spidev->xmit failed: %d", ret);
    return ret;
  }

  atlog("cmd:%02x:%02x:%02x:%02x -> res:%02x:%02x:%02x:%02x",
      cmd[0], cmd[1], cmd[2], cmd[3],
      res[0], res[1], res[2], res[3]
  );
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
    // cmd = get_read_signature_bytes_cmd(i);
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

    // cmd = CMD_FLASH_LOAD_BYTE(wordaddr, 0, b);
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;

    cmd[0] = 0x48;
    cmd[1] = 0x00;
    cmd[2] = wordaddr & 0x1f;
    cmd[3] = *(src++);
    // cmd = CMD_FLASH_LOAD_BYTE(wordaddr, 1, b);
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

  // cmd = CMD_FLASH_WRITE_PAGE(page);
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

    // cmd = CMD_FLASH_READ_BYTE(check_addr, high);

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
    // cmd = CMD_FLASH_READ_BYTE(wordaddr, high);
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
    // cmd = CMD_WRITE_EEPROM_BYTE(byte_addr, src);
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
    // cmd = CMD_READ_EEPROM_BYTE(addr + i);
    cmd[1] = ((addr + i) >> 8) & 1;
    cmd[2] = (addr + i) & 0xff;
    if (atmega8a_cmd(atmega8a_dev.spi, cmd, res) != ERR_OK)
      return ERR_FATAL;
    *ptr++ = res[3];
  }
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
  return 8192;
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
