#include "dwc2_channel.h"
#include "dwc2_log.h"
#include <spinlock.h>
#include <types.h>
#include <bits_api.h>
#include <stringlib.h>
#include "dwc2_xfer_control.h"

static struct spinlock dwc2_channels_lock;
static uint8_t channels_bitmap = 0;
static struct dwc2_channel dwc2_channels[6] = { 0 };

struct dwc2_channel *dwc2_channel_create()
{
  uint8_t bitmap;
  int ch;
  int num_channels = 6;
  int irqflags;
  cond_spinlock_lock_disable_irq(&dwc2_channels_lock, irqflags);
  bitmap = channels_bitmap;
  for (ch = 0; ch < num_channels; ++ch) {
    if (!(bitmap & (1<<ch)))
        break;
  }
  if (ch < num_channels) {
    bitmap |= (1<<ch);
    channels_bitmap = bitmap;
  }
  cond_spinlock_unlock_restore_irq(&dwc2_channels_lock, irqflags);
  if (ch == num_channels) {
    DWCERR("Failed to allocate channel");
    return NULL;
  }
  DWCDEBUG2("channel %d allocated: %p", ch, &dwc2_channels[ch]);
  return &dwc2_channels[ch];
}

void dwc2_channel_destroy(struct dwc2_channel *c)
{
  uint8_t bitmap;
  int irqflags;
  cond_spinlock_lock_disable_irq(&dwc2_channels_lock, irqflags);;
  bitmap = channels_bitmap;
  BUG(BIT_IS_CLEAR(bitmap, c->id), "Trying to release channel that's not busy");
  BIT_CLEAR(bitmap, c->id);
  channels_bitmap = bitmap;
  cond_spinlock_unlock_restore_irq(&dwc2_channels_lock, irqflags);
  DWCDEBUG2("channel %d freed %p", c->id, c);
}

struct dwc2_channel *dwc2_channel_get_by_id(int ch_id)
{
  int irqflags;
  uint8_t bitmap;
  struct dwc2_channel *channel = NULL;
  cond_spinlock_lock_disable_irq(&dwc2_channels_lock, irqflags);;
  bitmap = channels_bitmap;
  if (BIT_IS_SET(bitmap, ch_id))
    channel = &dwc2_channels[ch_id];

  cond_spinlock_unlock_restore_irq(&dwc2_channels_lock, irqflags);
  return channel;
}

void dwc2_channel_init(void)
{
  int i;
  cond_spinlock_init(&dwc2_channels_lock);
  memset(dwc2_channels, 0, sizeof(dwc2_channels));
  for (i = 0; i < ARRAY_SIZE(dwc2_channels); ++i)
    dwc2_channels[i].id = i;

}

void dwc2_channel_bind_xfer_control(struct dwc2_channel *c, struct dwc2_xfer_control *ctl)
{
  c->ctl = ctl;
  ctl->c = c;
}

void dwc2_channel_unbind_xfer_control(struct dwc2_channel *c)
{
  BUG(!c->ctl, "unbinging non-existent xfer control from channel");
  c->ctl->c = NULL;
  c->ctl = NULL;
}
