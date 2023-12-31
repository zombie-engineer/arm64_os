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
#include <memory/dma_memory.h>
#include <dma.h>
#include <dma_regs.h>

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
#define ILI9341_CMD_MEM_ACCESS_CONTROL 0x36
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

#ifdef DISPLAY_MODE_PORTRAIT
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320
#else
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#endif

#define SPI_CS   ((volatile uint32_t *)0x3f204000)
#define SPI_FIFO ((volatile uint32_t *)0x3f204004)
#define SPI_CLK  ((volatile uint32_t *)0x3f204008)
#define SPI_DLEN ((volatile uint32_t *)0x3f20400c)
#define SPI_CS_CLEAR_TX (1 << 4)
#define SPI_CS_CLEAR_RX (1 << 5)
#define SPI_CS_CLEAR    (3 << 4)
#define SPI_CS_TA       (1 << 7)
#define SPI_CS_DMAEN    (1 << 8)
#define SPI_CS_ADCS     (1 << 11)
#define SPI_CS_DONE     (1 << 16)
#define SPI_CS_RXD      (1 << 17)
#define SPI_CS_TXD      (1 << 18)
#define SPI_CS_RXR      (1 << 19)
#define SPI_CS_RXF      (1 << 20)

#define SPI_DC_SET() write_reg(0x3f20001c, 1 << gpio_pin_dc)
#define SPI_DC_CLEAR() write_reg(0x3f200028, 1 << gpio_pin_dc)

struct tft_control {
  int gpio_pin_mosi;
  int gpio_pin_miso;
  int gpio_pin_sclk;
  int gpio_pin_blk;
  int gpio_pin_reset;
  int gpio_pin_dc;
};

static struct tft_control g_tft_control;
static struct tft_control *tftc = NULL;

static void tft_cmd(char cmd)
{
  int gpio_pin_dc = tftc->gpio_pin_dc;

  if(*SPI_CS & SPI_CS_RXD)
    *SPI_CS = SPI_CS_TA | SPI_CS_CLEAR_RX;
 // gpio_set_off(tftc->gpio_pin_dc);
  SPI_DC_CLEAR();
  while(!(*SPI_CS & SPI_CS_TXD));
  *SPI_FIFO = cmd;
  while(!(*SPI_CS & SPI_CS_DONE));
  // gpio_set_on(tftc->gpio_pin_dc);
  SPI_DC_SET();
}

#define SEND_CMD(__cmd) tft_cmd(__cmd)

#if 0
#define SEND_CMD(__cmd) do { \
  if(*SPI_CS & SPI_CS_RXD) \
    *SPI_CS = SPI_CS_TA | SPI_CS_CLEAR_RX;\
  SPI_DC_CLEAR();\
  while(!(*SPI_CS & SPI_CS_TXD)) /*printf("-%08x\r\n", *SPI_CS)*/;\
  *SPI_FIFO = __cmd;\
  while(!(*SPI_CS & SPI_CS_DONE)) /*printf("+%08x\r\n", *SPI_CS)*/;\
  SPI_DC_SET();\
} while(0)

#endif

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

#define BYTES_PER_PIXEL 3
#define BYTES_PER_FRAME (DISPLAY_WIDTH * DISPLAY_HEIGHT * BYTES_PER_PIXEL)
#define NUM_FRAMES 20
static char tft_lcd_canvas[BYTES_PER_FRAME * NUM_FRAMES];

int OPTIMIZED display_read_frame(
  hcd_mass_t *m,
  char *dst,
  int offset,
  int read_size)
{
  int err;
  err = usb_mass_read(m, dst, offset, read_size);
  if (err != ERR_OK) {
    printf("tft_lcd: failed to read mass storage device\r\n");
  }
  return err;
}

void OPTIMIZED display_payload(void)
{
  int i, frame;
  char *ptr;
  hcd_mass_t *m = NULL;

  m = detect_any_usb_mass_device();
  if (!m) {
    goto err;
  }

  dcache_flush(m, sizeof(*m));
  dcache_flush(m->d, sizeof(*(m->d)));
  printf("mass storage device found\r\n");
  ptr = (char *)0x03d2000;

  printf("dma_buf allocated: %p(%d)\r\n", ptr, 4096);
  memset(tft_lcd_canvas, 0xcc, sizeof(tft_lcd_canvas));

#define READ_SIZE (512 * 512)
  for (frame = 0; frame < NUM_FRAMES; frame++) {
    for (i = 0; i < BYTES_PER_FRAME / READ_SIZE + 1; ++i) {
      display_read_frame(m, ptr, frame * BYTES_PER_FRAME + i * READ_SIZE, READ_SIZE);
      memcpy(tft_lcd_canvas + frame * BYTES_PER_FRAME + i * READ_SIZE, ptr, READ_SIZE);
    }

  }
  // display_read_frame(m, ptr, i * READ_SIZE, 1024);
  while(1) {
    for (frame = 0; frame < NUM_FRAMES; frame++) {
      // tft_set_region_coords(gpio_pin_dc, 0, 0, 239, DISPLAY_HEIGHT - 1);
      SEND_CMD_DATA(
        ILI9341_CMD_WRITE_PIXELS,
        tft_lcd_canvas + frame * BYTES_PER_FRAME,
        BYTES_PER_FRAME);
    }
  }
err:
  while(1) {
    asm volatile("wfe");
  }
}

extern int spi0_xmit_dma(struct spi_dev *d, const void *data_out, void *data_in, uint32_t bytelen);
extern int spi0_init_dma();

#define DMA_SPI_TRANSFER_LEN_OFF 16
static uint32_t dma_set_spi_word(uint32_t *word, uint32_t transfer_len)
{
  const uint32_t max_transfer_len = 65535 & ~3;
  uint16_t spi_transfer_len;
  if (transfer_len > max_transfer_len) {
    spi_transfer_len = (uint16_t)max_transfer_len;
    printf("setting transfer len from %d to %d\n", transfer_len, spi_transfer_len);
  } else {
    spi_transfer_len = (uint16_t)transfer_len;
    printf("setting transfer len to %d\n", spi_transfer_len);
  }

  *word = (spi_transfer_len << DMA_SPI_TRANSFER_LEN_OFF) | SPI_CS_TA;
  return spi_transfer_len;
}

#define SPI_CS_7E         0x7e204000
#define SPI_FIFO_7E       0x7e204004
#define DMA_CS0_7E        0x7e007000
#define DMA_CS1_7E        0x7e007100
#define DMA_CONBLK_AD_7E  0x7e007004

#define PTR_TO_U32(__ptr) ((uint32_t)(uint64_t)__ptr)
#define DMA_ADDR(__ptr) (PTR_TO_U32(__ptr) | 0xc0000000)

#define WAIT_Y true
#define WAIT_N false

#define TI(__src_type, __dst_type, __dreq, __dreq_type, __wait)\
  DMA_TI_ADDR_TYPE_##__src_type,\
  DMA_TI_ADDR_TYPE_##__dst_type,\
  DMA_DREQ_##__dreq,\
  DMA_TI_DREQ_T_##__dreq_type,\
  __wait

/* MEM -> SPI FIFO */
#define TI_TX_PIPE TI(INC_Y, INC_N, SPI_TX, DST, WAIT_Y)
/* SPI FIFO -> MEM */
#define TI_RX_PIPE TI(INC_N, IGNOR, SPI_RX, SRC, WAIT_N)
/* MEM -> MEM */
#define TI_MM_PIPE TI(INC_Y, INC_Y, NONE, NONE, WAIT_Y)



static inline void dma_program_cb(
  struct dma_control_block *cb,
  uint32_t src_addr,
  uint32_t dst_addr,
  uint32_t transfer_len,
  dma_addr_type_t src_addr_type,
  dma_addr_type_t dst_addr_type,
  int dreq,
  dma_ti_dreq_type_t dreq_type,
  bool wait_resp,
  void *cb_next)
{
  cb->ti = dma_make_ti_value(dreq, dreq_type, src_addr_type, dst_addr_type, wait_resp);
  cb->src_addr = src_addr;
  cb->dst_addr = dst_addr;
  cb->transfer_length = transfer_len;
  cb->stride = 0;
  cb->dma_cb_next = cb_next ? DMA_ADDR(cb_next) : 0;
  cb->res0 = 0;
  cb->res1 = 0;
}

static uint32_t spi_stop = SPI_CS_DMAEN;
static uint32_t dma_cs_start = DMA_CS_ACTIVE | DMA_CS_END;

void tft_lcd_print_data(char *frame_data, int size)
{
  uint32_t transfer_len = size;
  uint64_t tx_headers[64]          ALIGNED(8);
  uint64_t tx_endmarks_check[64]   ALIGNED(8);
  uint64_t rx_endmarks_check[64]   ALIGNED(8);
  uint64_t tx_next_dma[64]         ALIGNED(8);
  uint32_t *hdr;
  const uint32_t max_task_size = (0xffffU - 64) & ~7U;
  int i;
  int num_tasks;

  struct dma_control_block *cbs = dma_alloc(sizeof(*cbs) * 64); //[64] ALIGNED(32);
  struct dma_control_block *cb;
  struct dma_control_block *cb_tx_header;
  struct dma_control_block *cb_tx_payload;
  struct dma_control_block *cb_rx;
  struct dma_control_block *cb_prev_rx;
  struct dma_control_block *cb_spi_stop;
  struct dma_control_block *cb_set_next_tx;
  struct dma_control_block *cb_start_next_tx;
  struct dma_control_block *cb_tx_endmark;
  struct dma_control_block *cb_rx_endmark;

  uint32_t len;
  uint32_t *src;

  cb = cbs;

  SEND_CMD(ILI9341_CMD_WRITE_PIXELS);
 // SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, frame_data, size);

  *(int*)0x3f204000 = SPI_CS_CLEAR|SPI_CS_DMAEN|SPI_CS_ADCS;

  src = (uint32_t *)frame_data;

  *(int*)0x3f007000 = DMA_CS_RESET;
  *(int*)0x3f007100 = DMA_CS_RESET;

  /* fill tx headers first SPI word */
  num_tasks = 0;
  while(transfer_len) {
    len = min(transfer_len, max_task_size);
    hdr = (uint32_t *)&tx_headers[num_tasks++];
    *hdr = (len << DMA_SPI_TRANSFER_LEN_OFF) | SPI_CS_TA;
    transfer_len -= len;
  }
  cb_prev_rx = NULL;

  for (i = 0; i < num_tasks; ++i) {
    len = ((tx_headers[i] >> 16) & 0xffff);
    hdr = (uint32_t*)&tx_headers[i];
    cb_tx_header  = cb++;
    /* if change rx order , change in further below also */
    cb_rx         = cb++;
    cb_tx_payload = cb++;
    cb_tx_endmark = cb++;

    dma_program_cb(cb_tx_header    , DMA_ADDR(hdr), SPI_FIFO_7E,                     4, TI_TX_PIPE, cb_tx_payload);
    dma_program_cb(cb_tx_payload   , DMA_ADDR(src), SPI_FIFO_7E,                   len, TI_TX_PIPE, cb_tx_endmark);
    dma_program_cb(cb_tx_endmark   , DMA_CS0_7E   , DMA_ADDR(&tx_endmarks_check[i]), 8, TI_MM_PIPE, NULL);

    dma_program_cb(cb_rx, SPI_FIFO_7E, 0, len, TI_RX_PIPE, NULL);

    if (cb_prev_rx) {
      cb_set_next_tx    = cb++;
      cb_spi_stop       = cb++;
      cb_start_next_tx  = cb++;
      cb_rx_endmark     = cb++;
      cb_prev_rx->dma_cb_next = DMA_ADDR(cb_set_next_tx);

      dma_program_cb(cb_set_next_tx  , DMA_ADDR(&tx_next_dma[i])  , DMA_CONBLK_AD_7E                 , 4, TI_MM_PIPE, cb_spi_stop);
      dma_program_cb(cb_spi_stop     , DMA_ADDR(&spi_stop)        , SPI_CS_7E                        , 4, TI_MM_PIPE, cb_rx_endmark);
      dma_program_cb(cb_rx_endmark   , DMA_CS1_7E                 , DMA_ADDR(&rx_endmarks_check[i-1]), 8, TI_MM_PIPE, cb_start_next_tx);
      dma_program_cb(cb_start_next_tx, DMA_ADDR(&dma_cs_start)    , DMA_CS0_7E                       , 4, TI_MM_PIPE, cb_rx);

      *(uint32_t*)&tx_next_dma[i] = DMA_ADDR(cb_tx_header);
    }
    src += len >> 2;
    cb_prev_rx = cb_rx;
  }

  dcache_flush(tx_headers, sizeof(tx_headers[0] * num_tasks));
  dcache_flush(tx_next_dma, sizeof(tx_next_dma[0] * num_tasks));
  dcache_flush(cbs, sizeof(cbs[0]) * (cb - cbs));

  write_reg(0x3f007004UL + 0x100 * 0, DMA_ADDR(&cbs[0]));
  write_reg(0x3f007004UL + 0x100 * 1, DMA_ADDR(&cbs[1]));

  *(int*)0x3f007000 |= DMA_CS_ACTIVE;
  *(int*)0x3f007100 |= DMA_CS_ACTIVE;
  dcache_flush(tx_endmarks_check, sizeof(tx_endmarks_check[0] * num_tasks));
  dcache_flush(rx_endmarks_check, sizeof(rx_endmarks_check[0] * num_tasks));
  while((*(int*)0x3f007000) & DMA_CS_END == 0);
  dma_free(cbs);
  /* pointer archithmetic, src will inc by 4 bytes, so we divede len by 4 */

 // spi0_xmit_dma(NULL, data, NULL, size);
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

#define fastfill(__dst, __count, __v1, __v2, __v3) \
  asm volatile(\
    "mov x0, %0\n"\
    "to .req x0\n"\
    "mov x1, #3\n"\
    "mul x1, x1, %1\n"\
    "add x1, x0, x1, lsl 3\n"\
    "to_end .req x1\n"\
    "mov x2, %2\n"\
    "val1 .req x2\n"\
    "mov x3, %3\n"\
    "val2 .req x3\n"\
    "mov x4, %4\n"\
    "val3 .req x4\n"\
    "1:\n"\
    "stp val1, val2, [to], #16\n"\
    "str val3, [to], #8\n"\
    "cmp to, to_end\n"\
    "bne 1b\n"\
    ".unreq to\n"\
    ".unreq to_end\n"\
    ".unreq val1\n"\
    ".unreq val2\n"\
    ".unreq val3\n"\
    ::"r"(__dst), "r"(__count), "r"(__v1), "r"(__v2), "r"(__v3): "x0", "x1", "x2", "x3", "x4")

void OPTIMIZED tft_fill_rect(int gpio_pin_dc, int x0, int y0, int x1, int y1, char r, char g, char b)
{
  char canvas[DISPLAY_WIDTH * DISPLAY_HEIGHT * 3] ALIGNED(64);
  int y, x;
  int local_width, local_height;
  char *c;
  uint64_t v1, v2, v3, iters;
//  uint64_t t1, t2;

  local_width = x1 - x0;
  local_height = y1 - y0;
  if (local_width * local_height > 8) {
 //   volatile uint64_t *p;
#define BP(v, p) ((uint64_t)v << (p * 8))
    v1 = BP(r, 0) | BP(g, 1) | BP(b, 2) | BP(r, 3) | BP(g, 4) | BP(b, 5) | BP(r, 6) | BP(g, 7);
    v2 = BP(b, 0) | BP(r, 1) | BP(g, 2) | BP(b, 3) | BP(r, 4) | BP(g, 5) | BP(b, 6) | BP(r, 7);
    v3 = BP(g, 0) | BP(b, 1) | BP(r, 2) | BP(g, 3) | BP(b, 4) | BP(r, 5) | BP(g, 6) | BP(b, 7);
#undef BP
    iters = local_width * local_height / 8;
    fastfill(canvas, iters, v1, v2, v3);
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
  // hexdump_memory_ex("-", 24, canvas, sizeof(canvas));
  tft_set_region_coords(gpio_pin_dc, x0, y0, x1, y1);
//  t1 = read_cpu_counter_64();
  SEND_CMD_DATA(ILI9341_CMD_WRITE_PIXELS, canvas, local_width * local_height * 3);
//  t2 = read_cpu_counter_64();
  // printf("%lld\r\n", get_cpu_counter_64_freq());
//  printf("output: %lld\r\n", t2-t1); // count = 4813039
}

void OPTIMIZED fill_screen(int gpio_pin_dc)
{
  tft_fill_rect(gpio_pin_dc, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 240, 0, 255);
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

void OPTIMIZED tft_lcd_cube_animation(struct tft_control *t)
{
  int x = 0, y = 0;
  int g = 0;
  int x_speed = 2;
  int y_speed = 5;
  while(1) {
    tft_fill_rect(t->gpio_pin_dc, x, y, x + 10, y + 10, 0, g, 0);
    wait_msec(1000 / 60);
    tft_fill_rect(t->gpio_pin_dc, x, y, x + 10, y + 10, 0, 0, 255);
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

static inline void tftc_gpio_init(struct tft_control *t)
{
  spi0_init_dma();
  t->gpio_pin_mosi  = 10;
  t->gpio_pin_miso  =  9;
  t->gpio_pin_sclk  = 11;
  t->gpio_pin_blk   = 19;

  t->gpio_pin_dc    = 13;
  t->gpio_pin_reset =  6;

  gpio_set_function(t->gpio_pin_mosi, GPIO_FUNC_ALT_0);
  gpio_set_function(t->gpio_pin_miso, GPIO_FUNC_ALT_0);
  gpio_set_function(t->gpio_pin_sclk, GPIO_FUNC_ALT_0);
  gpio_set_function(t->gpio_pin_blk, GPIO_FUNC_OUT);
  gpio_set_function(t->gpio_pin_reset, GPIO_FUNC_OUT);
  gpio_set_function(t->gpio_pin_dc, GPIO_FUNC_OUT);
  gpio_set_on(t->gpio_pin_dc);
  gpio_set_on(t->gpio_pin_blk);
  gpio_set_off(t->gpio_pin_reset);
}

static inline void tftc_transport_init(struct tft_control *t)
{
  tftc_gpio_init(t);
  *SPI_CS = SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX;
  *SPI_CLK = 8;
  *SPI_DLEN = 2;
  tftc = t;
}

void /*OPTIMIZED*/ tft_lcd_init(void)
{
  char data[8];
  // char data[512];
  struct tft_control *t = &g_tft_control;
  tftc_transport_init(t);

  gpio_set_on(t->gpio_pin_reset);
  wait_msec(120);
  gpio_set_off(t->gpio_pin_reset);
  wait_msec(120);
  gpio_set_on(t->gpio_pin_reset);
  wait_msec(120);

  write_reg(SPI_CS, SPI_CS_TA);

  SEND_CMD(ILI9341_CMD_SOFT_RESET);
  wait_msec(5);
  SEND_CMD(ILI9341_CMD_DISPLAY_OFF);

//  data[0] = 0x39;
//  data[1] = 0x2c;
//  data[2] = 0x34;
//  data[3] = 0x02;
//  SEND_CMD_DATA(ILI9341_CMD_POWER_CTL_A, data, 4);
//
/* horizontal refresh direction */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_MH  (1<<2)
/* pixel format default is bgr, with this flag it's rgb */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_BGR (1<<3)
/* horizontal refresh direction */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_ML  (1<<4)
/*
 * swap rows and columns default is PORTRAIT, this flags makes it ALBUM
 */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_MV  (1<<5)
/* column address order */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_MX  (1<<6)
/* row address order */
#define ILI9341_CMD_MEM_ACCESS_CONTROL_MY  (1<<7)
  data[0] = ILI9341_CMD_MEM_ACCESS_CONTROL_BGR
#ifndef DISPLAY_MODE_PORTRAIT
    | ILI9341_CMD_MEM_ACCESS_CONTROL_MV
#endif
  //  | ILI9341_CMD_MEM_ACCESS_CONTROL_ML
  //  | ILI9341_CMD_MEM_ACCESS_CONTROL_MH
  //  | ILI9341_CMD_MEM_ACCESS_CONTROL_MX
  //  | ILI9341_CMD_MEM_ACCESS_CONTROL_MY
  ;
  SEND_CMD_DATA(ILI9341_CMD_MEM_ACCESS_CONTROL, data, 1);

  SEND_CMD(ILI9341_CMD_SLEEP_OUT);
  wait_msec(120);
  SEND_CMD(ILI9341_CMD_DISPLAY_ON);
  wait_msec(120);
//  while(1) {
//    RECV_CMD_DATA(ILI9341_CMD_READ_ID, data, 2);
//    wait_usec(10);
//  }
  fill_screen(t->gpio_pin_dc);
}

void tft_lcd_run(void)
{
  display_payload();
  tft_lcd_cube_animation(tftc);
}
