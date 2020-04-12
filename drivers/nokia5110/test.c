#include <drivers/display/nokia5110.h>
#include <drivers/display/nokia5110_console.h>
#include <stringlib.h>
#include "nokia5110_internal.h"
#include <common.h>
#include <console.h>
#include <delays.h>
#include <bins.h>

static char frame_1[NOKIA5110_CANVAS_SIZE + 4];
static char frame_2[NOKIA5110_CANVAS_SIZE + 4];
static char frame_3[NOKIA5110_CANVAS_SIZE + 4];
static char frame_4[NOKIA5110_CANVAS_SIZE + 4];
static char frame_5[NOKIA5110_CANVAS_SIZE + 4];
static char frame_6[NOKIA5110_CANVAS_SIZE + 4];
static char frame_7[NOKIA5110_CANVAS_SIZE + 4];
static char frame_8[NOKIA5110_CANVAS_SIZE + 4];

static int nokia5110_run_test_loop_1(int iterations, int wait_interval)
{
  int i, err;
  char *ptr;
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  memset(frame_1, 0xff, 4);
  memset(frame_2, 0xff, 4);
  memset(frame_3, 0xff, 4);
  memset(frame_4, 0xff, 4);

  ptr = frame_1 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b10000000;
    *ptr++ = 0b01000000;
    *ptr++ = 0b00100000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00001000;
    *ptr++ = 0b00000100;
    *ptr++ = 0b00000010;
    *ptr++ = 0b00000001;
  }

  ptr = frame_2 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00010000;
  }

  ptr = frame_3 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00000001;
    *ptr++ = 0b00000010;
    *ptr++ = 0b00000100;
    *ptr++ = 0b00001000;
    *ptr++ = 0b00010000;
    *ptr++ = 0b00100000;
    *ptr++ = 0b01000000;
    *ptr++ = 0b10000000;
  }

  ptr = frame_4 + 4;
  for (i = 0; i < 504 / 8; ++i) {
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b11111111;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
    *ptr++ = 0b00000000;
  }

  for (i = 0; i < iterations; ++i) {
    nokia5110_send_data_dma(frame_1, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_2, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_3, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_4, 504);
    wait_msec(wait_interval);
  }

  return 0;
}

static int nokia5110_run_test_loop_2(int iterations, int wait_interval)
{
  int i;
  int err;
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  memset(frame_1, 0b10000000, 508);
  memset(frame_2, 0b01000000, 508);
  memset(frame_3, 0b00100000, 508);
  memset(frame_4, 0b00010000, 508);
  memset(frame_5, 0b00001000, 508);
  memset(frame_6, 0b00000100, 508);
  memset(frame_7, 0b00000010, 508);
  memset(frame_8, 0b00000001, 508);

  for (i = 0; i < iterations; ++i) {
    nokia5110_send_data_dma(frame_1, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_2, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_3, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_4, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_5, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_6, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_7, 504);
    wait_msec(wait_interval);
    nokia5110_send_data_dma(frame_8, 504);
    wait_msec(wait_interval);
  }

  return ERR_OK;
}

static int nokia5110_run_test_loop_3(int iterations, int wait_interval)
{
  int err, i, numframes;
  const char *video_0;
  uint64_t bufsize;
  wait_msec(5000);

  video_0 = bins_get_start_nokia5110_animation();
  bufsize = bins_get_size_nokia5110_animation();
  numframes = bufsize / 508;
  printf("showing video_0 from address: %08x, num frames: %d\n", video_0, numframes);
  RET_IF_ERR(nokia5110_set_cursor, 0, 0);
  for (i = 0; i < numframes; ++i) {
    RET_IF_ERR(nokia5110_set_cursor, 0, 0);
    nokia5110_send_data_dma(video_0 + (504 + 4) * i, 504);
    wait_msec(20);
  }
  return ERR_OK;
}

void nokia5110_test()
{
  nokia5110_run_test_loop_1(5, 200);
  nokia5110_run_test_loop_3(30, 10);
  nokia5110_run_test_loop_1(8, 100);
  nokia5110_run_test_loop_1(8, 50);
  nokia5110_run_test_loop_1(8, 30);
  nokia5110_run_test_loop_1(8, 15);
  nokia5110_run_test_loop_2(10, 50);
  nokia5110_run_test_loop_2(10, 40);
  nokia5110_run_test_loop_2(10, 30);
  nokia5110_run_test_loop_2(20, 20);
  nokia5110_run_test_loop_2(30, 10);
}

void nokia5110_test_text()
{
  console_dev_t *d = nokia5110_get_console_device();
  if (!d)
    kernel_panic("Failed to get nokia5110 console device.\n");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
  d->puts("This is a test.");
}
