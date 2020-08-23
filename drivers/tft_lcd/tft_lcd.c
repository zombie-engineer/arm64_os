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
static tft_lcd_dev_t *tft_lcd_dev = 0;
// static struct spinlock tft_lcd_lock;

#define CHECKED_FUNC(fn, ...) DECL_FUNC_CHECK_INIT(fn, tft_lcd_dev, __VA_ARGS__)

int tft_lcd_send_data(const void *data, size_t sz)
{
  int ret;
  gpio_set_on(tft_lcd_dev->dc);
  ret = tft_lcd_dev->spi->xmit(tft_lcd_dev->spi, data, 0, sz);
  return ret;
}

int tft_lcd_send_data_dma(const void *data, size_t sz)
{
  int ret;
  gpio_set_on(tft_lcd_dev->dc);
  ret = tft_lcd_dev->spi->xmit_dma(tft_lcd_dev->spi, data, 0, sz);
  return ret;
}

#define SEND_DATA_DMA(data, len)



#define NOKIA5110_INSTRUCTION_SET_NORMAL 0
#define NOKIA5110_INSTRUCTION_SET_EX     1
//static CHECKED_FUNC(tft_lcd_set_instructions, int instruction_set)
//  int err;
//  if (instruction_set == NOKIA5110_INSTRUCTION_SET_NORMAL) {
//    SEND_CMD(0xc0);
//  }
//  else {
//    SEND_CMD(0x21);
//  }
//  return ERR_OK;
//}
#define SCLK 11
#define RST  25
#define DC   22
#define MOSI 10
#define MISO 9

#define MS 3

#define RST_HI gpio_set_on(RST)
#define RST_LO gpio_set_off(RST)

#define DC_HI gpio_set_on(DC)
#define DC_LO gpio_set_off(DC)

#define CLK_HI gpio_set_on(SCLK);
#define CLK_LO gpio_set_on(SCLK);

#define RDMISO_TACT(__dest, __step)\
    CLK_HI; wait_msec(MS);__dest |= ((gpio_is_set(MISO) ? 1:0) << __step); CLK_LO; wait_msec(MS);

#define RDMISO8(__dest)\
  RDMISO_TACT(__dest, 0);\
  RDMISO_TACT(__dest, 1);\
  RDMISO_TACT(__dest, 2);\
  RDMISO_TACT(__dest, 3);\
  RDMISO_TACT(__dest, 4);\
  RDMISO_TACT(__dest, 5);\
  RDMISO_TACT(__dest, 6);\
  RDMISO_TACT(__dest, 7);

#define WRMOSI_TACT(__src, __step)\
  do {\
    if (__src & (1 << (7 - __step)))\
      gpio_set_on(MOSI);\
    else\
      gpio_set_off(MOSI);\
    CLK_HI; \
    wait_msec(MS);\
    CLK_LO; \
    wait_msec(MS);\
  } while(0)

#define WRMOSI8(__src)\
  WRMOSI_TACT(__src, 0);\
  WRMOSI_TACT(__src, 1);\
  WRMOSI_TACT(__src, 2);\
  WRMOSI_TACT(__src, 3);\
  WRMOSI_TACT(__src, 4);\
  WRMOSI_TACT(__src, 5);\
  WRMOSI_TACT(__src, 6);\
  WRMOSI_TACT(__src, 7);

#define DISPLAY_SET_CURSOR_X 0x2A
#define DISPLAY_SET_CURSOR_Y 0x2B
#define DISPLAY_WRITE_PIXELS 0x2C

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
#define GPIO_TFT_DATA_CONTROL 22  /*!< Version 1, Pin P1-22, PiTFT 2.8 resistive Data/Control pin */

// Appears in ILI9341 Data Sheet v1.11 (2011/06/10), but not in older v1.02 (2010/12/06). This has a subtle effect on colors/saturation.
// Valid values are 0x20 and 0x30. Spec says 0x20 is default at boot, but doesn't seem so, more like 0x00 is default, giving supersaturated colors. I like 0x30 best.
// Value 0x30 doesn't seem to be available on ILI9340.
#define ILI9341_PUMP_CONTROL_2XVCI 0x20
#define ILI9341_PUMP_CONTROL_3XVCI 0x30

#ifdef ILI9341
#define ILI9341_PUMP_CONTROL ILI9341_PUMP_CONTROL_3XVCI
#else
#define ILI9341_PUMP_CONTROL ILI9341_PUMP_CONTROL_2XVCI
#endif


#define DISPLAY_NATIVE_WIDTH 240
#define DISPLAY_NATIVE_HEIGHT 320

#define GPIO_TFT_RESET_PIN 25

//void ili9341_init()
//{
//  // If a Reset pin is defined, toggle it briefly high->low->high to enable the device.
//  // Some devices do not have a reset pin, in which case compile with GPIO_TFT_RESET_PIN left undefined.
//  printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
//  gpio_set_function(GPIO_TFT_RESET_PIN, GPIO_FUNC_OUT);
//  gpio_set_on(GPIO_TFT_RESET_PIN);
//  wait_msec(120);
//  gpio_set_off(GPIO_TFT_RESET_PIN);
//  wait_msec(120);
//  gpio_set_on(GPIO_TFT_RESET_PIN);
//  wait_msec(120);
//
//  // Do the initialization with a very low SPI bus speed,
//  // so that it will succeed even if the bus speed chosen by the user is too high.
//  spi0_set_clock(34);
//
//  printf("1\n");
//  SPI_TRANSFER0(0x01 /*Software Reset*/);
//  wait_msec(5);
//  printf("2\n");
//
//  char out;
//  gpio_set_on(GPIO_TFT_DATA_CONTROL);
//  spi0_xmit_byte(0, 0x04, &out);
//  gpio_set_off(GPIO_TFT_DATA_CONTROL);
//  printf("out:%02x\n", out);
//  spi0_xmit_byte(0, 0x00, &out);
//  printf("out:%02x\n", out);
//  spi0_xmit_byte(0, 0x00, &out);
//  printf("out:%02x\n", out);
//  spi0_xmit_byte(0, 0x00, &out);
//  printf("out:%02x\n", out);
//  spi0_xmit_byte(0, 0x00, &out);
//  printf("out:%02x\n", out);
//  spi0_xmit_byte(0, 0x00, &out);
//  printf("out:%02x\n", out);
//
//  gpio_set_off(GPIO_TFT_DATA_CONTROL);
//  spi0_xmit_byte(0, 0x28, &out);
//  gpio_set_on(GPIO_TFT_DATA_CONTROL);
//
//  SPI_TRANSFER0(0x28 /*Display OFF*/);
//  // The following appear in ILI9341 Data Sheet v1.11 (2011/06/10), but not in older v1.02 (2010/12/06).
//
//  printf("Display is off\n");
//  uint8_t data_power_control_a[] = {
//    0x39/*Reserved*/,
//    0x2C/*Reserved*/,
//    0x00/*Reserved*/,
//    0x34/*REG_VD=1.6V*/,
//    0x02/*VBC=5.6V*/
//  };
//
//  SPI_TRANSFER(0xCB/*Power Control A*/, data_power_control_a);
//  // These are the same as power on.
//
//  uint8_t data_power_control_b[] = {
//      0x00/*Always Zero*/,
//      0xC1/*Power Control=0,DRV_ena=0,PCEQ=1*/,
//      0x30/*DC_ena=1*/
//  };
//
//  SPI_TRANSFER(0xCF/*Power Control B*/, data_power_control_b);
//  // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
//
//  uint8_t data_timing_control_a[] = {
//    0x85, 0x00, 0x78
//  };
//  SPI_TRANSFER(0xE8/*Driver Timing Control A*/, data_timing_control_a);
//  // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
//
//
//  uint8_t data_timing_control_b[] = {
//    0x00, 0x00
//  };
//  SPI_TRANSFER(0xEA/*Driver Timing Control B*/, data_timing_control_b);
//  // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
//
//  uint8_t data_power_on_seq_control[] = {
//    0x64, 0x03, 0x12, 0x81
//  };
//  SPI_TRANSFER(0xED/*Power On Sequence Control*/, data_power_on_seq_control);
//  // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
//
//  // The following appear also in old ILI9341 Data Sheet v1.02 (2010/12/06).
//  uint8_t data_power_control_1[] = {
//    0x23/*VRH=4.60V*/
//  };
//  SPI_TRANSFER(0xC0/*Power Control 1*/, data_power_control_1);
//  // Set the GVDD level, which is a reference level for the VCOM level and the grayscale voltage level.
//
//  uint8_t data_power_control_2[] = {
//    0x10/*AVCC=VCIx2,VGH=VCIx7,VGL=-VCIx4*/
//  };
//  SPI_TRANSFER(0xC1/*Power Control 2*/, data_power_control_2); // Sets the factor used in the step-up circuits. To reduce power consumption, set a smaller factor.
//
//  uint8_t data_vcom_control_1[] = {
//    0x3e/*VCOMH=4.250V*/, 0x28/*VCOML=-1.500V*/
//  };
//  SPI_TRANSFER(0xC5/*VCOM Control 1*/, data_vcom_control_1); // Adjusting VCOM 1 and 2 can control display brightness
//
//  uint8_t data_vcom_control_2[] = {
//    0x86/*VCOMH=VMH-58,VCOML=VML-58*/
//  };
//  SPI_TRANSFER(0xC7/*VCOM Control 2*/, data_vcom_control_2);
//
//#define MADCTL_BGR_PIXEL_ORDER (1<<3)
//#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
//#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
//#define MADCTL_ROW_ADDRESS_ORDER_SWAP (1<<7)
//#define MADCTL_ROTATE_180_DEGREES (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_ADDRESS_ORDER_SWAP)
//
//  uint8_t madctl[1] = { 0 };
////#ifndef DISPLAY_SWAP_BGR
////  madctl |= MADCTL_BGR_PIXEL_ORDER;
////#endif
////#if defined(DISPLAY_FLIP_ORIENTATION_IN_HARDWARE)
////    madctl |= MADCTL_ROW_COLUMN_EXCHANGE;
////#endif
////#ifdef DISPLAY_ROTATE_180_DEGREES
////    madctl ^= MADCTL_ROTATE_180_DEGREES;
////#endif
//  SPI_TRANSFER(0x36/*MADCTL: Memory Access Control*/, madctl);
//
//  SPI_TRANSFER0(0x20/*Display Inversion OFF*/);
//  uint8_t data_pixel_format[] = { 0x55/*DPI=16bits/pixel,DBI=16bits/pixel*/ };
//  SPI_TRANSFER(0x3A/*COLMOD: Pixel Format Set*/, data_pixel_format);
//
//  // According to spec sheet, display frame rate in 4-wire SPI "internal clock mode" is computed
//  // with the following formula:
//  // frameRate = 615000 / [ (pow(2,DIVA) * (320 + VFP + VBP) * RTNA ]
//  // where
//  // - DIVA is clock division ratio, 0 <= DIVA <= 3; so pow(2,DIVA) is either 1, 2, 4 or 8.
//  // - RTNA specifies the number of clocks assigned to each horizontal scanline, and must follow 16 <= RTNA <= 31.
//  // - VFP is vertical front porch, number of idle sleep scanlines before refreshing a new frame, 2 <= VFP <= 127.
//  // - VBP is vertical back porch, number of idle sleep scanlines after refreshing a new frame, 2 <= VBP <= 127.
//
//  // Max refresh rate then is with DIVA=0, VFP=2, VBP=2 and RTNA=16:
//  // maxFrameRate = 615000 / (1 * (320 + 2 + 2) * 16) = 118.63 Hz
//
//  // To get 60fps, set DIVA=0, RTNA=31, VFP=2 and VBP=2:
//  // minFrameRate = 615000 / (8 * (320 + 2 + 2) * 31) = 61.23 Hz
//
//  // It seems that in internal clock mode, horizontal front and back porch settings (HFP, BFP) are ignored(?)
//
//  uint8_t data_frame_rate_control[] = {
//    0x00/*DIVA=fosc*/, ILI9341_UPDATE_FRAMERATE/*RTNA(Frame Rate)*/
//  };
//  SPI_TRANSFER(0xB1/*Frame Rate Control (In Normal Mode/Full Colors)*/, data_frame_rate_control);
//
//  uint8_t data_display_function_control[] = {
//    0x08/*PTG=Interval Scan,PT=V63/V0/VCOML/VCOMH*/,
//    0x82/*REV=1(Normally white),ISC(Scan Cycle)=5 frames*/,
//    0x27/*LCD Driver Lines=320*/
//  };
//  SPI_TRANSFER(0xB6/*Display Function Control*/, data_display_function_control);
//
//  uint8_t data_display_enable_3g[] = {
//    0x02/*False*/
//  };
//  SPI_TRANSFER(0xF2/*Enable 3G*/, data_display_enable_3g); // This one is present only in ILI9341 Data Sheet v1.11 (2011/06/10)
//
//  uint8_t data_gamma_curve[] = {
//    0x01/*Gamma curve 1 (G2.2)*/
//  };
//  SPI_TRANSFER(0x26/*Gamma Set*/, data_gamma_curve);
//
//  uint8_t data_gamma_correction_p[] = {
//    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
//  };
//  SPI_TRANSFER(0xE0/*Positive Gamma Correction*/, data_gamma_correction_p);
//
//  uint8_t data_gamma_correction_n[] = {
//    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
//  };
//  SPI_TRANSFER(0xE1/*Negative Gamma Correction*/, data_gamma_correction_n);
//  SPI_TRANSFER0(0x11/*Sleep Out*/);
//  wait_msec(120);
//  SPI_TRANSFER0(/*Display ON*/0x29);
//
//  ClearScreen();
//  // And speed up to the desired operation speed finally after init is done.
//  wait_msec(10); // Delay a bit before restoring CLK, or otherwise this has been observed to cause the display not init if done back to back after the clear operation above.
//#define SPI_BUS_CLOCK_DIVISOR 40
//  spi0_set_clock(SPI_BUS_CLOCK_DIVISOR);
//}

#define SPI_BYTESPERPIXEL 3
#define SHARED_MEMORY_SIZE (320 * 240 * SPI_BYTESPERPIXEL * 3)

char spimem[SHARED_MEMORY_SIZE] ALIGNED(4096);

 //int tft_lcd_init()
 //{
 //  gpio_set_function(GPIO_TFT_DATA_CONTROL, GPIO_FUNC_OUT); // Data/Control pin to output (0x01)
 //  spi0_init_cool();
 //  ili9341_init();
 ////  int err;
 ////  // if (!spidev)
 ////  //  return ERR_INVAL_ARG;
 ////  s = spidev;
 ////
 ////  int pins[2] = { rst_pin, dc_pin };
 ////
 ////  gpio_set_handle_t gpio_set_handle;
 ////  gpio_set_handle = gpio_set_request_n_pins(pins, ARRAY_SIZE(pins), tft_lcd_gpio_set_key);
 ////
 ////  if (gpio_set_handle == GPIO_SET_INVALID_HANDLE) {
 ////    puts("Failed to request gpio pins for RST,DC pins.\n");
 ////    return ERR_BUSY;
 ////  }
 ////
 ////  tft_lcd_device.spi = spidev;
 ////  tft_lcd_device.dc = dc_pin;
 ////  tft_lcd_device.rst = rst_pin;
 ////  tft_lcd_device.gpioset = gpio_set_handle;
 ////  tft_lcd_dev = &tft_lcd_device;
 ////
 ////  gpio_set_function(RST, GPIO_FUNC_OUT);
 ////  gpio_set_function(DC, GPIO_FUNC_OUT);
 ////
 ////  // gpio_set_function(MOSI, GPIO_FUNC_OUT);
 ////  // gpio_set_function(SCLK, GPIO_FUNC_OUT);
 ////  // gpio_set_function(MISO, GPIO_FUNC_IN);
 ////
 ////  RST_HI;
 ////  wait_msec(1000);
 ////  RST_LO;
 ////  wait_msec(5);
 ////  RST_HI;
 ////  tft_send_cmd(0x01, 0);
 ////  wait_msec(1000);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x04, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  s->xmit_byte(s, 0x09, 0);
 ////  while(1);
 ////  tft_send_cmd(0x04, 8);
 ////  tft_send_cmd(0xc0, 1);
 ////  tft_send_data(0x25, 1);
 ////
 ////  tft_send_cmd(0xc1, 1);
 ////  tft_send_data(0x11, 1);
 ////
 ////  tft_send_cmd(0xc5, 1);
 ////  tft_send_data(0x2b, 1);
 ////  tft_send_data(0x2b, 1);
 ////
 ////  tft_send_cmd(0xc7, 1);
 ////  tft_send_data(0x86, 1);
 ////
 ////  tft_send_cmd(0x3a, 1);
 ////  tft_send_data(0x05, 1);
 ////  while(1);
 ////
 ////  gpio_set_off(tft_lcd_device.rst);
 ////
 ////  RET_IF_ERR(tft_lcd_reset);
 ////
 //////  SEND_CMD(NOKIA5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_SET));
 //////  SEND_CMD(NOKIA5110_SET_VOP(0x39));
 //////  SEND_CMD(NOKIA5110_TEMP_CONTROL(0));
 //////  SEND_CMD(NOKIA5110_BIAS_SYSTEM(4));
 //////  SEND_CMD(NOKIA5110_FUNCTION_SET(PD_BIT_CLEAR, V_BIT_CLEAR, H_BIT_CLEAR));
 //////  SEND_CMD(NOKIA5110_DISPLAY_CONTROL_NORMAL);
 ////
 ////  tft_lcd_font = 0;
 ////  cond_spinlock_init(&tft_lcd_lock);
 ////
 //  puts("TFT_LCD: init is complete\n");
 //  return ERR_OK;
 //}

CHECKED_FUNC(tft_lcd_reset)
  int err;
  RET_IF_ERR(gpio_set_off, tft_lcd_dev->rst);
  wait_usec(200);
  RET_IF_ERR(gpio_set_on, tft_lcd_dev->rst);
  return ERR_OK;
}

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

#define SPI_CS ((volatile uint32_t *)0x3f204000)
#define SPI_FIFO ((volatile uint32_t *)0x3f204004)
#define SPI_CLK ((volatile uint32_t *)0x3f204008)
#define SPI_DLEN ((volatile uint32_t *)0x3f20400c)
#define SPI_CS_TA (1 << 7)
#define SPI_CS_DONE (1 << 16)
#define SPI_CS_RXD (1 << 17)
#define SPI_CS_TXD (1 << 18)
#define SPI_CS_RXR (1 << 19)
#define SPI_CS_RXF (1 << 20)
#define SPI_CS_CLEAR_RX (1 << 5)
#define SPI_CS_CLEAR_TX (1 << 4)

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

static inline void tft_set_region_coords(int gpio_pin_dc, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
#define LO8(__v) (__v & 0xff)
#define HI8(__v) ((__v >> 8) & 0xff)
  char data_x[4] = { HI8(x0), LO8(x0), HI8(x1), LO8(x1) };
  char data_y[4] = { HI8(y0), LO8(y0), HI8(y1), LO8(y1) };
  SEND_CMD_DATA(0x2a, data_x, sizeof(data_x));
  SEND_CMD_DATA(0x2b, data_y, sizeof(data_y));
#undef LO8
#undef HI8
}

void OPTIMIZED clear_screen(int gpio_pin_dc)
{
  char data_rgb[3] = { 0, 0xff, 0 };
  int x, y;
  for (y = 0; y < DISPLAY_HEIGHT - 1; ++y) {
    for (x = 0; x < DISPLAY_WIDTH - 1; ++x) {
      tft_set_region_coords(gpio_pin_dc, x, y, x + 1, y + 1);
      SEND_CMD_DATA(0x2c, data_rgb, 3);
    }
  }
}

void OPTIMIZED tft_fill_rect(int gpio_pin_dc, int x0, int y0, int x1, int y1, char r, char g, char b)
{
  char data_rgb[DISPLAY_WIDTH * 3];
  int y, x;

  for (x = 0; x < (x1 - x0); ++x) {
    data_rgb[x * 3] = r;
    data_rgb[x * 3 + 1] = g;
    data_rgb[x * 3 + 2] = b;
  }

  for (y = 0; y < y1; ++y) {
    tft_set_region_coords(gpio_pin_dc, x0, y, x1, y + 1);
    SEND_CMD_DATA(0x2c, data_rgb, (x1 - x0) * 3);
  }
}

void OPTIMIZED clear_screen2(int gpio_pin_dc)
{
  int i;
  printf("hello\r\n");
  while(1) {
    for (i = 0; i < 255; i += 5)
      tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, i, 0, 0);
    for (i = 0; i < 255; i += 5)
      tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 255 - i, 0, 0);
  }
}

void OPTIMIZED tft_lcd_init(void)
{
  const int gpio_pin_mosi  = 10;
  const int gpio_pin_miso  =  9;
  const int gpio_pin_sclk  = 11;
  const int gpio_pin_ce0   = 8;
  const int gpio_pin_dc    = 22;
  const int gpio_pin_reset = 25;

  gpio_set_function(gpio_pin_mosi, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_miso, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_sclk, GPIO_FUNC_ALT_0);
  gpio_set_function(gpio_pin_ce0, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_function(gpio_pin_dc, GPIO_FUNC_OUT);
  gpio_set_on(gpio_pin_dc);
  gpio_set_off(gpio_pin_ce0);
  gpio_set_off(gpio_pin_reset);

  *SPI_CS = SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX;
  *SPI_CLK = 34;
  *SPI_DLEN = 2;

  gpio_set_on(gpio_pin_reset);
  wait_msec(120);
  gpio_set_off(gpio_pin_reset);
  wait_msec(120);
  gpio_set_on(gpio_pin_reset);
  wait_msec(120);

  *SPI_CS = SPI_CS_TA;

  SEND_CMD(0x01/*Software Reset*/);
  wait_msec(5);
  SEND_CMD(0x28/*Display OFF*/);
  SEND_CMD(0x11/*Sleep Out*/);
  wait_msec(120);
  SEND_CMD(/*Display ON*/0x29);
  clear_screen2(gpio_pin_dc);
  clear_screen(gpio_pin_dc);
  while(1);
  wait_usec(100);
}

//#ifndef CONFIG_QEMU
//  tft_lcd_init();
//#endif
