#include <avr_update.h>
#include <common.h>
#include <error.h>
#include <stringlib.h>
#include <spi.h>
#include <drivers/atmega8a.h>
#include <bins.h>

#define ATMEGA_FIRMWARE_HEADER "ATMGBIN8"
#define ATMEGA_FIRMWARE_HEADER_END "ATMGEND0"

static int init_atmega8a()
{
  const int gpio_pin_cs0   = 5;
  const int gpio_pin_mosi  = 6;
  const int gpio_pin_miso  = 13;
  const int gpio_pin_sclk  = 19;
  const int gpio_pin_reset = 26;

  int ret;
  char fuse_high;
  char fuse_low;
  char lock_bits;
  int flash_size;
  int eeprom_size;
  spi_dev_t *spidev;
  char lock_bits_desc[128];
  spidev = spi_allocate_emulated("spi_avr_isp", 
      gpio_pin_sclk, gpio_pin_mosi, gpio_pin_miso, gpio_pin_cs0, -1);
  if (IS_ERR(spidev)) {
    printf("Failed to initialize emulated spi. Error code: %d\n", 
       (int)PTR_ERR(spidev));
    return ERR_GENERIC;
  }

  ret = atmega8a_init(spidev, gpio_pin_reset);

  if (ret != ERR_OK) {
    printf("Failed to init atmega8a. Error_code: %d\n", ret);
    return ERR_GENERIC;
  }
 
  if (atmega8a_read_fuse_bits(&fuse_low, &fuse_high) != ERR_OK) {
    printf("Failed to read program memory\n");
    return ERR_GENERIC;
  }

  if (atmega8a_read_lock_bits(&lock_bits) != ERR_OK) {
    printf("Failed to read program memory\n");
    return ERR_GENERIC;
  }

  atmega8a_lock_bits_describe(lock_bits_desc, sizeof(lock_bits_desc), lock_bits);

  flash_size = atmega8a_get_flash_size();
  eeprom_size = atmega8a_get_eeprom_size();
  printf("atmega8 downloader initialization:\r\n");
  printf("  flash size: %d bytes\r\n", flash_size);
  printf("  eeprom size: %d bytes\r\n", eeprom_size);
  printf("  lock bits: %x: %s\r\n", lock_bits, lock_bits_desc);
  printf("  fuse bits: low:%02x high:%02x\r\n", fuse_low, fuse_high);
  return ERR_OK;
}

int avr_program(void)
{
  int err;
  char fuse_bits_low = 0xe0 | (ATMEGA8A_FUSE_CPU_FREQ_8MHZ & ATMEGA8A_FUSE_CPU_FREQ_MASK);
  printf("avr_program: programming atmega8a: \r\n");
  printf("writing low fuse bits to %x\r\n", fuse_bits_low);
  err = atmega8a_write_fuse_bits_low(fuse_bits_low);
  if (err != ERR_OK)
    printf("avr_program: failed to write low fuse bits: %d\n", err);
  else
    puts("avr_program: device has been programmed.\r\n");
  return err;
}

int avr_dump_firmware(void)
{
  int err;
  char membuf[8192];
  int num_read;
  num_read = min(atmega8a_get_flash_size(), sizeof(membuf));
  printf("atmega8a_read_firmware: reading %d bytes of atmega8a flash memory\r\n.", num_read);
  err = atmega8a_read_flash_memory(membuf, num_read, 0);
  if (err != ERR_OK) {
    printf("atmega8a_read_firmware: flash read failed: %d\n", err);
    return err;
  }
  hexdump_memory(membuf, num_read);
  puts("atmega8a_read_firmware: Success.\r\n");
  return err;
}

static int atmega8a_download(const void *bin, int bin_size)
{
  int err;
  char membuf[8192];
  const char *binary = bin;
  int binary_size = bin_size;
  memset(membuf, 0xee, 8192);
  if (bin_size > 8192) {
    printf("atmega8a_download: firmware too big: %d bytes.\r\n", bin_size);
    return ERR_GENERIC;
  }

  if (bin_size % 64) {
    binary = membuf;
    binary_size = ((bin_size / 64) + 1) * 64;
    printf("atmega8a_download: firmware size %d not page-aligned (64 bytes), " 
          "padding up to %d bytes\r\n", 
        bin_size, binary_size);
    memcpy(membuf, bin, bin_size);
  }

  puts("atmega8a_download: starting flash erase..\r\n");
  err = atmega8a_chip_erase();
  if (err != ERR_OK) {
    printf("atmega8a_download: Failed to erase flash\r\n");
    return err;
  }
  puts("atmega8a_download: flash erased.\r\n");
  printf("atmega8a_download: writing firmware to flash (%d bytes at %p)..\r\n", 
      binary_size, binary);

  err = atmega8a_write_flash(binary, binary_size, 0);
  if (err != ERR_OK) {
    printf("atmega8a_download: write to flash failed: %d\n", err);
    return err;
  }
  printf("atmega8a_download: write to flash completed.\r\n");

  memset(membuf, 0x00, sizeof(membuf));

  printf("atmega8a_download: reading flash to validate.\r\n");

  err = atmega8a_read_flash_memory(membuf, bin_size, 0);
  if (err != ERR_OK) {
    printf("atmega8a_download: read from flash failed.\n");
    return err;
  }

  if (memcmp(membuf, bin, bin_size)) {
    puts("atmega8a_download: on device flash differs. validation failed.\n");
    return ERR_GENERIC;
  }

  printf("atmega8a_download: validation completed.\r\n");
  return ERR_OK;
}

int avr_update(void)
{
  int err;
  int check_crc = 0;
  const char *bin = bins_get_start_atmega();
  int binsz       = bins_get_size_atmega();
  uint32_t new_checksum, old_checksum;

  init_atmega8a();

  if (check_crc) {
    printf("avr_update: Checking update blob, start:%p, end:%p, size:%d\n",
      bin, bin + binsz, binsz);
  
    if (strncmp(bin + 0x28, ATMEGA_FIRMWARE_HEADER, 8)) {
      puts("avr_update: no header in uploadable blob\n");
      return ERR_INVAL_ARG;
    }
  
    if (strncmp(bin + 0x28 + 8 + 4, ATMEGA_FIRMWARE_HEADER_END, 8)) {
      puts("avr_update: no header end in uploadable blob\n");
      return ERR_INVAL_ARG;
    }
  
    new_checksum = *(uint32_t*)(bin + 0x28 + 8);
    printf("avr_update: new firmware checksum: %08x\n",
        new_checksum);
  
    err = atmega8a_read_flash_memory(&old_checksum, 4, 0x28 + 8);
    if (err != ERR_OK) {
      printf("avr_update: flash read failed: %d\n", err);
      return err;
    }
    printf("avr_update: firmware checksum on device: %08x\n", old_checksum);
  
    if (new_checksum == old_checksum) {
      puts("avr_update: old checksum matches new. Skipping update.\n");
      atmega8a_reset();
      return ERR_OK;
    }
  }

  avr_program();

  err = atmega8a_download(bin, binsz);
  if (err != ERR_OK)
    printf("avr_update: failed to download firmware to device.\n");
  else {
    puts("avr_update: firmware on device has been updated.\n");
  }
  atmega8a_reset();
  return err;
}
