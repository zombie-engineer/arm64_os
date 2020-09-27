#include <drivers/display/tft_lcd.h>
#include <common.h>
#include <delays.h>
#include <gpio.h>
#include <gpio_set.h>
#include <stringlib.h>
#include <font.h>
#include <spinlock.h>
#include <cpu.h>
#include <debug.h>
#include <sched.h>

//DECL_GPIO_SET_KEY(tft_lcd_gpio_set_key, "TFT_LCD___GPIO0");

typedef struct tft_lcd_dev {
  spi_dev_t *spi;
  uint32_t dc;
  uint32_t rst;
  gpio_set_handle_t gpioset;
} tft_lcd_dev_t;

typedef struct tft_lcd_canvas_control {
  uint8_t *canvas;
  int cursor_x;
  int cursor_y;
  const font_desc_t *font;
} tft_lcd_canvas_control_t;

// static const font_desc_t *tft_lcd_font;
// static tft_lcd_dev_t tft_lcd_device;
// static tft_lcd_dev_t *tft_lcd_dev = 0;
// static struct spinlock tft_lcd_lock;

#define ILI9341_CMD_SOFT_RESET   0x01
#define ILI9341_CMD_READ_ID      0x04
#define ILI9341_CMD_SLEEP_OUT    0x11
#define ILI9341_CMD_DISPLAY_OFF  0x28
#define ILI9341_CMD_DISPLAY_ON   0x29
#define ILI9341_CMD_SET_CURSOR_X 0x2a
#define ILI9341_CMD_SET_CURSOR_Y 0x2b
#define ILI9341_CMD_WRITE_PIXELS 0x2c
#define ILI9341_CMD_WRITE_MEMORY_CONTINUE 0x3c
#define ILI9341_CMD_POWER_CTL_A  0xcb
#define ILI9341_CMD_POWER_CTL_B  0xcf
#define ILI9341_CMD_TIMING_CTL_A 0xe8
#define ILI9341_CMD_TIMING_CTL_B 0xea
#define ILI9341_CMD_POWER_ON_SEQ 0xed
#define ILI9341_CMD_PUMP_RATIO   0xf7
#define ILI9341_CMD_POWER_CTL_1  0xc0
#define ILI9341_CMD_POWER_CTL_2  0xc1
#define ILI9341_CMD_VCOM_CTL_1   0xc5
#define ILI9341_CMD_VCOM_CTL_2   0xc7
#define ILI9341_CMD_FRAME_RATE_CTL 0xb1
#define ILI9341_CMD_BLANK_PORCH  0xb5
#define ILI9341_CMD_DISPL_FUNC   0xb6

// ILI9341 displays are able to update at any rate between 61Hz to up to 119Hz. Default at power on is 70Hz.
#define ILI9341_FRAMERATE_61_HZ 0x1F
#define ILI9341_FRAMERATE_63_HZ 0x1E
#define ILI9341_FRAMERATE_65_HZ 0x1D
#define ILI9341_FRAMERATE_68_HZ 0x1C
#define ILI9341_FRAMERATE_70_HZ 0x1B
#define ILI9341_FRAMERATE_73_HZ 0x1A
#define ILI9341_FRAMERATE_76_HZ 0x19
#define ILI9341_FRAMERATE_79_HZ 0x18
#define ILI9341_FRAMERATE_83_HZ 0x17
#define ILI9341_FRAMERATE_86_HZ 0x16
#define ILI9341_FRAMERATE_90_HZ 0x15
#define ILI9341_FRAMERATE_95_HZ 0x14
#define ILI9341_FRAMERATE_100_HZ 0x13
#define ILI9341_FRAMERATE_106_HZ 0x12
#define ILI9341_FRAMERATE_112_HZ 0x11
#define ILI9341_FRAMERATE_119_HZ 0x10
//
// Visually estimating NES Super Mario Bros 3 "match mushroom, flower, star" arcade game, 119Hz gives visually
// most pleasing result, so default to using that. You can also try other settings above. 119 Hz should give
// lowest latency, perhaps 61 Hz might give least amount of tearing, although this can be quite subjective.
#define ILI9341_UPDATE_FRAMERATE ILI9341_FRAMERATE_119_HZ

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

#define SPI_CS   ((volatile uint32_t *)0x3f204000)
#define SPI_FIFO ((volatile uint32_t *)0x3f204004)
#define SPI_CLK  ((volatile uint32_t *)0x3f204008)
#define SPI_DLEN ((volatile uint32_t *)0x3f20400c)
#define SPI_CS_CLEAR_TX (1 << 4)
#define SPI_CS_CLEAR_RX (1 << 5)
#define SPI_CS_TA       (1 << 7)
#define SPI_CS_DONE     (1 << 16)
#define SPI_CS_RXD      (1 << 17)
#define SPI_CS_TXD      (1 << 18)
#define SPI_CS_RXR      (1 << 19)
#define SPI_CS_RXF      (1 << 20)

#define SPI_DC_SET() write_reg(0x3f20001c, 1 << gpio_pin_dc)
#define SPI_DC_CLEAR() write_reg(0x3f200028, 1 << gpio_pin_dc)

#define SEND_CMD(__cmd) do { \
  if(*SPI_CS & SPI_CS_RXD) \
    *SPI_CS = SPI_CS_TA | SPI_CS_CLEAR_RX;\
  SPI_DC_CLEAR();\
  while(!(*SPI_CS & SPI_CS_TXD)) /*printf("-%08x\r\n", *SPI_CS)*/;\
  *SPI_FIFO = __cmd;\
  while(!(*SPI_CS & SPI_CS_DONE)) /*printf("+%08x\r\n", *SPI_CS)*/;\
  SPI_DC_SET();\
} while(0)

#define SEND_CMD_DATA(__cmd, __data, __datalen) do { \
  char *__ptr = (char *)(__data);\
  char *__end = __ptr + __datalen;\
  uint32_t r;\
  SEND_CMD(__cmd);\
  while(__ptr < __end) {\
    char __d = *(__ptr++);\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_RXD) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __d);\
  }\
  while(1) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_DONE)\
      break;\
    /*printf("dr:%08x\r\n", r);*/\
    if (r & SPI_CS_RXR) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
  }\
} while(0)

#define SEND_CMD_CHAR_REP(__cmd, __r, __g, __b, __reps) do { \
  uint32_t r;\
  int __rep = 0;\
  SEND_CMD(__cmd);\
  while(__rep++ < __reps) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_RXD) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __r);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __g);\
    while(1) {\
      r = read_reg(SPI_CS);\
      if (r & SPI_CS_TXD)\
        break;\
      /*printf("rr:%08x\r\n", r);*/\
    }\
    write_reg(SPI_FIFO, __b);\
  }\
  while(1) {\
    r = read_reg(SPI_CS);\
    if (r & SPI_CS_DONE)\
      break;\
    /*printf("dr:%08x\r\n", r);*/\
    if (r & SPI_CS_RXR) \
      write_reg(SPI_CS, SPI_CS_TA | SPI_CS_CLEAR_RX);\
  }\
} while(0)

#define RECV_CMD_DATA(__cmd, __data, __datalen) do { \
  char *__ptr = (char *)(__data);\
  char *__end = __ptr + __datalen;\
  uint32_t r;\
  SEND_CMD(__cmd);\
  write_reg(SPI_CS, SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX);\
  write_reg(SPI_CS, SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX | SPI_CS_TA);\
  while(__ptr < __end) {\
    uint8_t c;\
    r = read_reg(SPI_CS);\
    /*printf("reg:%08x\r\n", r);*/\
    if (read_reg(SPI_CS) & SPI_CS_TXD) {\
      write_reg(SPI_FIFO, 0);\
      while (read_reg(SPI_CS) & SPI_CS_DONE);\
      while (read_reg(SPI_CS) & SPI_CS_RXD) {\
        c = read_reg(SPI_FIFO);\
        /*printf("fifo:%08x\r\n", c);*/\
        *(__ptr++) = c;\
      }\
    }\
  }\
} while(0)

static inline void tft_set_region_coords(int gpio_pin_dc, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
#define LO8(__v) (__v & 0xff)
#define HI8(__v) ((__v >> 8) & 0xff)
  char data_x[4] = { HI8(x0), LO8(x0), HI8(x1), LO8(x1) };
  char data_y[4] = { HI8(y0), LO8(y0), HI8(y1), LO8(y1) };
  SEND_CMD_DATA(ILI9341_CMD_SET_CURSOR_X, data_x, sizeof(data_x));
  SEND_CMD_DATA(ILI9341_CMD_SET_CURSOR_Y, data_y, sizeof(data_y));
#undef LO8
#undef HI8
}

#include <drivers/usb/usbd.h>
#include <drivers/usb/hcd.h>
#include <drivers/usb/usb_mass_storage.h>

struct usb_find_device_fn_arg {
  hcd_mass_t *mass_device;
};

int usb_find_device_fn(struct usb_hcd_device *d, void *priv)
{
  struct usb_find_device_fn_arg *arg = priv;
  int device_class, interface_class;
  hcd_mass_t *m = NULL;
  int i;

  arg->mass_device = NULL;
  device_class = hcd_device_get_class(d);
  printf("device_class: %d\r\n", device_class);
  if (device_class == USB_HCD_DEVICE_CLASS_MASS) {
    m = usb_hcd_device_to_mass(d);
  } else if (device_class == USB_HCD_DEVICE_CLASS_UNDEFINED) {
    for (i = 0; i < hcd_device_get_num_interfaces(d); ++i) {
      printf("interface: %d\r\n", i);
      interface_class = usb_hcd_get_interface_class(d, i);
      if (interface_class == USB_INTERFACE_CLASS_MASSSTORAGE) {
        m = usb_hcd_device_to_mass(d);
        break;
      }
    }
  }
  if (m) {
    printf("state: %d\r\n", hcd_device_get_state(d));
    if (hcd_device_get_state(d) == USB_DEVICE_STATE_CONFIGURED) {
      arg->mass_device = m;
      return USB_ITER_STOP;
    }
  }
  return USB_ITER_CONTINUE;
}

char tft_lcd_canvas[DISPLAY_WIDTH * DISPLAY_HEIGHT * 3];

void __attribute__((optimize("O0"))) payload(int gpio_pin_dc)
{
  int i;
  char buff[9];
  tft_set_region_coords(gpio_pin_dc, 0, 0, 2, 1);
  for (i = 0; i < 1000; ++i) {
    putc('c');
    buff[0] = 0xff; buff[1] = 0x00; buff[2] = 0x00;
    buff[3] = 0x00; buff[4] = 0xff; buff[5] = 0x00;
    buff[6] = 0x00; buff[7] = 0x00; buff[8] = 0xff;
    SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, buff, 9);
    wait_msec(700);
    buff[0] = 0x00; buff[1] = 0xff; buff[2] = 0x00;
    buff[3] = 0x00; buff[4] = 0x00; buff[5] = 0xff;
    buff[6] = 0xff; buff[7] = 0x00; buff[8] = 0x00;
    SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, buff, 9);
    wait_msec(700);
    buff[0] = 0x00; buff[1] = 0x00; buff[2] = 0xff;
    buff[3] = 0x00; buff[4] = 0xff; buff[5] = 0x00;
    buff[6] = 0x00; buff[7] = 0x00; buff[8] = 0xff;
    SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, buff, 9);
    wait_msec(700);
  }
}
void OPTIMIZED display_payload(int gpio_pin_dc)
{
  int i, y;
  int err;
  struct usb_find_device_fn_arg arg = { 0 };
  char buf[256];
  const char *ptr;
  hcd_mass_t *m = NULL;

  for (i = 0; i < 10; ++i) {
    usb_iter_devices(usb_find_device_fn, &arg);
    if (arg.mass_device)
      break;
    printf("mass storage device not found yet. continue\r\n");
    wait_msec(10 * 1000);
  }
  if (!arg.mass_device) {
    printf("tft_lcd: failed to detect usb mass storage device\r\n");
    goto err;
  }
  m = arg.mass_device;

  printf("mass storage device found\r\n");
  memset(buf, 0, sizeof(buf));
  dcache_flush(m, sizeof(*m));
  dcache_flush(m->d, sizeof(*(m->d)));

  tft_set_region_coords(gpio_pin_dc, 0, 0, 239, DISPLAY_HEIGHT - 1);
  for (i = 0; i < 450; ++i) {
    //printf("reading i:%d, offset:%d\r\n", i, i * 512);
    err = usb_mass_read(arg.mass_device, i * 512, tft_lcd_canvas, 512);
    if (err != ERR_OK) {
      printf("tft_lcd: failed to read mass storage device\r\n");
      goto err;
    }
    SEND_CMD_DATA(ILI9341_CMD_WRITE_MEMORY_CONTINUE, tft_lcd_canvas, 512);
  }
  puts("end\r\n");
  while(1);
  ptr = tft_lcd_canvas;
  for (y = 0; y < DISPLAY_HEIGHT; ++y) {
    tft_set_region_coords(gpio_pin_dc, 0, y, DISPLAY_WIDTH - 1, y + 1);
    SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, ptr, DISPLAY_WIDTH * 3);
    ptr += DISPLAY_WIDTH * 3;
  }

err:
  while(1) {
    asm volatile("wfe");
  }
}

void OPTIMIZED clear_screen(int gpio_pin_dc)
{
  char data_rgb[3] = { 0, 0, 0xff };
  int x, y;
  for (y = 0; y < DISPLAY_HEIGHT; ++y) {
    for (x = 0; x < DISPLAY_WIDTH; ++x) {
      tft_set_region_coords(gpio_pin_dc, x, y, x + 1, y + 1);
      SEND_CMD_DATA(0x2c, data_rgb, 3);
    }
  }
}

/*
 * tft_fill_rect
 * gpio_pin_dc - used in SPI macro to send command
 * x0, x1 - range of fill from [x0, x1), x1 not included
 * x1, x2 - range of fill from [y0, y1), y1 not included
 * r,g,b  - obviously color components of fill
 *
 * Speed: optimized version is using 3 64-bit values to compress rgb inside of them
 * this way 3 stores of 64bit words substitute 8 * 3 = 24 char stores, which are slower
 * anyway even in 1:1 compare.
 * Counts are incremented 19200000 per second = 19.2MHz
 * Rates are:
 * - buffer fill with chars                 : 269449  (0.014  sec) (14 millisec)
 * - buffer fill with 64-bitwords           : 16817   (0.0008 sec) (0.800 millisec)
 * - buffer transfer via SPI CLK=34 no DLEN : 5414663 (0.2820 sec) (280 millisec)
 * - buffer transfer via SPI CLK=34         : 4813039 (0.2506 sec) (250 millisec)
 * - buffer transfer via SPI CLK=32         : 4529939 (0.2359 sec) (239 millisec)
 * - buffer transfer via SPI CLK=24         : 3397452 (0.1769 sec) (176 millisec)
 * - buffer transfer via SPI CLK=16         : 2501551 (0.1302 sec) (130 millisec)
 * - buffer transfer via SPI CLK=8          : 2194014 (0.1142 sec) (114 millisec) < best
 * - buffer transfer via SPI CLK=4          : 2448441 (0.1275 sec) (127 millisec)
 * DLEN = 2 gives a considerable optimization
 */
void OPTIMIZED tft_fill_rect(int gpio_pin_dc, int x0, int y0, int x1, int y1, char r, char g, char b)
{
  char canvas[DISPLAY_WIDTH * DISPLAY_HEIGHT * 3] ALIGNED(64);
  int y, x;
  int local_width, local_height;
  char *c;
  uint64_t t1, t2;

  local_width = x1 - x0;
  local_height = y1 - y0;
  if (local_width * local_height > 8) {
    uint64_t v1, v2, v3, iters;
    uint64_t *p;
#define BP(v, p) ((uint64_t)v << (p * 8))
    v1 = BP(r, 0) | BP(g, 1) | BP(b, 2) | BP(r, 3) | BP(g, 4) | BP(b, 5) | BP(r, 6) | BP(g, 7);
    v2 = BP(b, 0) | BP(r, 1) | BP(g, 2) | BP(b, 3) | BP(r, 4) | BP(g, 5) | BP(b, 6) | BP(r, 7);
    v3 = BP(g, 0) | BP(b, 1) | BP(r, 2) | BP(g, 3) | BP(b, 4) | BP(r, 5) | BP(g, 6) | BP(b, 7);
#undef BP
    // printf("%016llx:%016llx:%016llx\r\n", v1, v2, v3);
    iters = local_width * local_height / 8;

    p = (uint64_t *)canvas;
    // t1 = read_cpu_counter_64();
    while(iters--) {
      // printf("iters = %d\r\n", iters);
      *p++ = v1;
      *p++ = v2;
      *p++ = v3;
    }
    // t2 = read_cpu_counter_64();
    // printf("iters = %d, lw:%d, lh:%d, count: %lld\r\n", iters, local_width, local_height, t2-t1);
    // count = 16817
  } else {
    c = canvas;
    // t1 = read_cpu_counter_64();
    for (x = 0; x < local_width; ++x) {
      for (y = 0; y < local_height; ++y) {
        c = canvas + (y * local_width + x) * 3;
        *(c++) = r;
        *(c++) = g;
        *(c++) = b;
      }
    }
    // t2 = read_cpu_counter_64();
    // printf("count: %lld\r\n", t2-t1); count = 269449
  }
  tft_set_region_coords(gpio_pin_dc, x0, y0, x1, y1);
  t1 = read_cpu_counter_64();
  SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, canvas, local_width * local_height * 3);
  t2 = read_cpu_counter_64();
  // printf("%lld\r\n", get_cpu_counter_64_freq());
  printf("output: %lld\r\n", t2-t1); // count = 4813039
}

void OPTIMIZED clear_screen2(int gpio_pin_dc)
{
  // while(1) {
  tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, 255);
  tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 255, 0, 0);
  tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 255, 0);
  // }
}

#if 0
static void tft_lcd_init2(void)
{
  printf("read_id: %02x:%02x:%02x\r\n", data[0], data[1], data[2]);

  data[0] = 0x39;
  data[1] = 0x2c;
  data[2] = 0x34;
  data[3] = 0x02;
  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_A, data, 4);
  data[0] = 0x00;
  data[1] = 0xc1;
  data[2] = 0x30;
  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_B, data, 3);
  data[0] = 0x85;
  data[1] = 0x00;
  data[2] = 0x78;
  SEND_CMD_DATA(ILI9341_CMD_TIMING_CTL_A, data, 3);
  data[0] = 0x00;
  data[1] = 0x00;
  SEND_CMD_DATA(ILI9341_CMD_TIMING_CTL_B, data, 2);
  data[0] = 0x64;
  data[1] = 0x03;
  data[2] = 0x12;
  data[3] = 0x81;
  SEND_CMD_DATA(ILI9341_CMD_POWER_ON_SEQ, data, 4);
  data[0] = 0x20;
  SEND_CMD_DATA(ILI9341_CMD_PUMP_RATIO, data, 1);
  data[0] = 0x23;
  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_1, data, 1);
  data[0] = 0x10;
  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_2, data, 1);

  data[0] = 0x3e;
  data[1] = 0x28;
  SEND_CMD_DATA(ILI9341_CMD_VCOM_CTL_1, data, 2);

  data[0] = 0x86;
  SEND_CMD_DATA(ILI9341_CMD_VCOM_CTL_2, data, 1);

  data[0] = 0x10;
  SEND_CMD_DATA(ILI9341_CMD_FRAME_RATE_CTL, data, 1);

  data[0] = 0x02;
  data[1] = 0x02;
  data[2] = 0x0a;
  data[3] = 0x14;
  SEND_CMD_DATA(ILI9341_CMD_BLANK_PORCH, data, 4);

  data[0] = 0x08;
  data[1] = 0x82;
  data[2] = 0x27;
  SEND_CMD_DATA(ILI9341_CMD_DISPL_FUNC, data, 3);

  data[0] = 0x02;
  SEND_CMD_DATA(0xf2, data, 1);
}
#endif

void OPTIMIZED tft_lcd_init(void)
{
  // char data[512];
  const int gpio_pin_mosi  = 10;
  const int gpio_pin_miso  =  9;
  const int gpio_pin_sclk  = 11;
  const int gpio_pin_bkl   =  8;

  const int gpio_pin_dc    = 25;
  const int gpio_pin_reset = 24;

  gpio_set_function(gpio_pin_mosi, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_miso, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_sclk, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_bkl, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_dc, GPIO_FUNC_OUT);
  gpio_set_on(gpio_pin_dc);
  gpio_set_on(gpio_pin_bkl);
  gpio_set_off(gpio_pin_reset);

  *SPI_CS = SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX;
  *SPI_CLK = 8;
  *SPI_DLEN = 2;

  gpio_set_on(gpio_pin_reset);
  wait_msec(120);
  gpio_set_off(gpio_pin_reset);
  wait_msec(120);
  gpio_set_on(gpio_pin_reset);
  wait_msec(120);

  write_reg(SPI_CS, SPI_CS_TA);

  SEND_CMD(ILI9341_CMD_SOFT_RESET);
  wait_msec(5);
  SEND_CMD(ILI9341_CMD_DISPLAY_OFF);

//  char data[8];
//  data[0] = 0x39;
//  data[1] = 0x2c;
//  data[2] = 0x34;
//  data[3] = 0x02;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_A, data, 4);

  SEND_CMD(ILI9341_CMD_SLEEP_OUT);
  wait_msec(120);
  SEND_CMD(ILI9341_CMD_DISPLAY_ON);
//  while(1) {
//    RECV_CMD_DATA(ILI9341_CMD_READ_ID, data, 2);
//    wait_usec(10);
//  }
  // if (0)
  clear_screen2(gpio_pin_dc);
 // clear_screen(gpio_pin_dc);
//  wait_msec(10 * 1000);
 // payload(gpio_pin_dc);
  display_payload(gpio_pin_dc);
  {
    int x = 0, y = 0;
    int g = 0;
    int x_speed = 2;
    int y_speed = 5;
    while(1) {
      tft_fill_rect(gpio_pin_dc, x, y, x + 10, y + 10, 0, g, 0);
      wait_msec(1000 / 60);
      tft_fill_rect(gpio_pin_dc, x, y, x + 10, y + 10, 0, 0, 255);
      wait_msec(1);
      if (x_speed > 0) {
        if (x > DISPLAY_WIDTH - 10) {
          x_speed *= -1;
          g += 10;
        }
      } else {
        if (x == 0)
          x_speed *= -1;
      }
      if (y_speed > 0) {
        if (y > DISPLAY_HEIGHT - 10)
          y_speed *= -1;
      } else {
        if (y == 0)
          y_speed *= -1;
      }
      x += x_speed;
      y += y_speed;
    }
  }
  wait_usec(100);
}
