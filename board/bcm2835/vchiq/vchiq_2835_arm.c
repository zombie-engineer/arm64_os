/**
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_connected.h"
#include "vchiq_killable.h"
#include <memory/dma_memory.h>
#include <memory.h>
#include <mbox/mbox_props.h>
#include <reg_access.h>
#include <cpu.h>
#include <error.h>
#include <stringlib.h>
#include <log.h>
#include <spinlock.h>
#include "vchiq_connection.h"
#include "mmal-msg.h"
#include "mmal-msg-port.h"
#include "mmal-parameters.h"
#include <memory/kmalloc.h>
#include "mmal-log.h"
#include "vchiq_mmal_err_handle.h"
#include "mmal-encodings.h"
#include "videodev2.h"
#include <delays.h>
#include "vc_sm_defs.h"
#include <intr_ctl.h>
#include <sched.h>
#include <irq.h>

#define SLOT_INFO_FROM_INDEX(state, index) (state->slot_info + (index))
#define SLOT_DATA_FROM_INDEX(state, index) (state->slot_data + (index))
#define SLOT_INDEX_FROM_DATA(state, data) \
	(((unsigned int)((char *)data - (char *)state->slot_data)) / \
	VCHIQ_SLOT_SIZE)
#define SLOT_INDEX_FROM_INFO(state, info) \
	((unsigned int)(info - state->slot_info))
#define SLOT_QUEUE_INDEX_FROM_POS(pos) \
	((int)((unsigned int)(pos) / VCHIQ_SLOT_SIZE))

#define BULK_INDEX(x) (x & (VCHIQ_NUM_SERVICE_BULKS - 1))

#define SRVTRACE_LEVEL(srv) \
	(((srv) && (srv)->trace) ? VCHIQ_LOG_TRACE : vchiq_core_msg_log_level)
#define SRVTRACE_ENABLED(srv, lev) \
	(((srv) && (srv)->trace) || (vchiq_core_msg_log_level >= (lev)))

#ifndef min
	#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

#define TOTAL_SLOTS (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)
#define MAX_FRAGMENTS (VCHIQ_NUM_CURRENT_BULKS * 2)

// #define BELL0	0x00
// #define BELL2	0x08

#define VCHIQ_ARM_ADDRESS(x) ((void *)((char *)x + RAM_BASE_BUS_UNCACHED))
#define VCHIQ_MMAL_MAX_COMPONENTS 64
typedef unsigned int VCHI_SERVICE_HANDLE_T;
extern void port_to_mmal_msg(struct vchiq_mmal_port *port, struct mmal_port *p);

static struct vchiq_state_struct vchiq_state ALIGNED(64);

static uint64_t vchiq_events ALIGNED(64) = 0xfffffffffffffff4;

typedef uint32_t irqreturn_t;
reg32_t g_regs;
int mmal_log_level = LOG_LEVEL_DEBUG3;

typedef struct vchiq_2835_state_struct {
   int inited;
   VCHIQ_ARM_STATE_T arm_state;
} VCHIQ_2835_ARM_STATE_T;

extern int vchiq_arm_log_level;

// static irqreturn_t
//vchiq_doorbell_irq(int irq, void *dev_id);

#define MAX_PORT_COUNT 4

struct vchiq_mmal_instance {
  struct vchiq_instance_struct *vchi_instance;
  VCHI_SERVICE_HANDLE_T handle;

  /* ensure serialised access to service */
  struct mutex vchiq_mutex;

  struct vchiq_mmal_component component[VCHIQ_MMAL_MAX_COMPONENTS];
};

struct vchi_version {
  uint32_t version;
  uint32_t version_min;
};

#define VCHI_VERSION(v_) { v_, v_ }
#define VCHI_VERSION_EX(v_, m_) { v_, m_ }

enum {
  COMP_CAMERA = 0,
  COMP_PREVIEW,
  COMP_IMAGE_ENCODE,
  COMP_VIDEO_ENCODE,
  COMP_COUNT
};

enum {
  CAM_PORT_PREVIEW = 0,
  CAM_PORT_VIDEO,
  CAM_PORT_CAPTURE,
  CAM_PORT_COUNT
};
#define MAX_SUPPORTED_ENCODINGS 20


extern int vchiq_mmal_init(struct vchiq_mmal_instance **out_instance);

typedef int (*mmal_io_fn)(struct vchiq_mmal_component *, struct vchiq_mmal_port *, struct mmal_buffer_header *);

struct mmal_io_work {
  struct list_head list;
  struct mmal_buffer_header *b;
  struct vchiq_mmal_component *c;
  struct vchiq_mmal_port *p;
  mmal_io_fn fn;
};

static struct list_head mmal_io_work_list;
static atomic_t mmal_io_work_waitflag;
static struct spinlock mmal_io_work_list_lock;

int mmal_io_work_push(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, struct mmal_buffer_header *b, mmal_io_fn fn)
{
  int irqflags;
  struct mmal_io_work *w;
  w = kzalloc(sizeof(*w), GFP_KERNEL);
  if (IS_ERR(w))
    return PTR_ERR(w);
  w->c = c;
  w->p = p;
  w->b = b;
  w->fn = fn;
  // MMAL_INFO("pushing work: %p", w);
  wmb();
  spinlock_lock_disable_irq(&mmal_io_work_list_lock, irqflags);
  list_add_tail(&w->list, &mmal_io_work_list);
  wakeup_waitflag(&mmal_io_work_waitflag);
  spinlock_unlock_restore_irq(&mmal_io_work_list_lock, irqflags);
  return ERR_OK;
}

struct mmal_io_work *mmal_io_work_pop(void)
{
  int irqflags;
  struct mmal_io_work *w = NULL;
  spinlock_lock_disable_irq(&mmal_io_work_list_lock, irqflags);
  if (list_empty(&mmal_io_work_list))
    goto out_unlock;

  w = list_first_entry(&mmal_io_work_list, typeof(*w), list);
  list_del_init(&w->list);
  wmb();

out_unlock:
  spinlock_unlock_restore_irq(&mmal_io_work_list_lock, irqflags);
  return w;
}

static int vchiq_io_thread(void)
{
  struct mmal_io_work *w;
  while(1) {
    wait_on_waitflag(&mmal_io_work_waitflag);
    // MMAL_INFO("io_thread: woke up");
    w = mmal_io_work_pop();
    if (w) {
      // MMAL_INFO("io_thread: have new work: %p", w);
      w->fn(w->c, w->p, w->b);
    }
  }
  return ERR_OK;
}

struct vchiq_open_payload {
  int fourcc;
  int client_id;
  short version;
  short version_min;
} PACKED;

struct vchiq_openack_payload {
    short version;
} PACKED;

static const char *msg_type_str(unsigned int msg_type)
{
	switch (msg_type) {
	case VCHIQ_MSG_PADDING:       return "PADDING";
	case VCHIQ_MSG_CONNECT:       return "CONNECT";
	case VCHIQ_MSG_OPEN:          return "OPEN";
	case VCHIQ_MSG_OPENACK:       return "OPENACK";
	case VCHIQ_MSG_CLOSE:         return "CLOSE";
	case VCHIQ_MSG_DATA:          return "DATA";
	case VCHIQ_MSG_BULK_RX:       return "BULK_RX";
	case VCHIQ_MSG_BULK_TX:       return "BULK_TX";
	case VCHIQ_MSG_BULK_RX_DONE:  return "BULK_RX_DONE";
	case VCHIQ_MSG_BULK_TX_DONE:  return "BULK_TX_DONE";
	case VCHIQ_MSG_PAUSE:         return "PAUSE";
	case VCHIQ_MSG_RESUME:        return "RESUME";
	case VCHIQ_MSG_REMOTE_USE:    return "REMOTE_USE";
	case VCHIQ_MSG_REMOTE_RELEASE:      return "REMOTE_RELEASE";
	case VCHIQ_MSG_REMOTE_USE_ACTIVE:   return "REMOTE_USE_ACTIVE";
	}
	return "???";
}

static const char *const mmal_msg_type_names[] = {
	"UNKNOWN",
	"QUIT",
	"SERVICE_CLOSED",
	"GET_VERSION",
	"COMPONENT_CREATE",
	"COMPONENT_DESTROY",
	"COMPONENT_ENABLE",
	"COMPONENT_DISABLE",
	"PORT_INFO_GET",
	"PORT_INFO_SET",
	"PORT_ACTION",
	"BUFFER_FROM_HOST",
	"BUFFER_TO_HOST",
	"GET_STATS",
	"PORT_PARAMETER_SET",
	"PORT_PARAMETER_GET",
	"EVENT_TO_HOST",
	"GET_CORE_STATS_FOR_PORT",
	"OPAQUE_ALLOCATOR",
	"CONSUME_MEM",
	"LMK",
	"OPAQUE_ALLOCATOR_DESC",
	"DRM_GET_LHS32",
	"DRM_GET_TIME",
	"BUFFER_FROM_HOST_ZEROLEN",
	"PORT_FLUSH",
	"HOST_LOG",
};

static inline void *mmal_check_reply_msg(struct mmal_msg *rmsg, int msg_type)
{
  if (rmsg->h.type != msg_type) {
    MMAL_ERR("mmal msg expected type %s, received: %s",
      mmal_msg_type_names[msg_type],
      mmal_msg_type_names[rmsg->h.type]);
    return NULL;
  }
  return &rmsg->u;
}

static int vchiq_calc_stride(int size)
{
  size += sizeof(struct vchiq_header_struct);
  return (size + sizeof(struct vchiq_header_struct) - 1) & ~(sizeof(struct vchiq_header_struct) - 1);
}

struct vchiq_header_struct *vchiq_prep_next_header_tx(struct vchiq_state_struct *s, int msg_size)
{
  int tx_pos, slot_queue_index, slot_index;
  struct vchiq_header_struct *h;
  int slot_space;
  int stride;

  /* Recall last position for tx */
  tx_pos = s->local_tx_pos;
  stride = vchiq_calc_stride(msg_size);

  /*
   * If message can not passed in one chunk within current slot,
   * we have to add padding header to this last free slot space and
   * allocate start of header already in the next slot
   *
   * |----------SLOT 1----------------|----------SLOT 2----------------|
   * |XXXXXXXXXXXXXXXXXXXXXXXXX0000000|00000000000000000000000000000000|
   * |                         ^                                       |
   * |                           \start of header                      |
   * |                         |============| <- message overlaps      |
   * |                                           slot boundary         |
   * |                         |pppppp|============| <- messsage moved |
   * |                        /                         to next slot   |
   * |         padding message added                                   |
   */
  slot_space = VCHIQ_SLOT_SIZE - (tx_pos & VCHIQ_SLOT_MASK);
  if (slot_space < stride) {
    slot_queue_index = ((int)((unsigned int)(tx_pos) / VCHIQ_SLOT_SIZE));
    slot_index = s->local->slot_queue[slot_queue_index & VCHIQ_SLOT_QUEUE_MASK];

    s->tx_data = (char*)&s->slot_data[slot_index];

    h = (struct vchiq_header_struct *)(s->tx_data + (tx_pos & VCHIQ_SLOT_MASK));
    h->msgid = VCHIQ_MSGID_PADDING;
    h->size = slot_space - sizeof(*h);
    s->local_tx_pos += slot_space;
    tx_pos += slot_space;
  }

  slot_queue_index = ((int)((unsigned int)(tx_pos) / VCHIQ_SLOT_SIZE));
  slot_index = s->local->slot_queue[slot_queue_index & VCHIQ_SLOT_QUEUE_MASK];

  s->tx_data = (char*)&s->slot_data[slot_index];
  s->local_tx_pos += stride;
  s->local->tx_pos = s->local_tx_pos;

  h = (struct vchiq_header_struct *)(s->tx_data + (tx_pos & VCHIQ_SLOT_MASK));
  h->size = msg_size;
  return h;
}

struct vchiq_header_struct *vchiq_get_next_header_rx(struct vchiq_state_struct *state)
{
  struct vchiq_header_struct *h;
  int rx_pos, slot_queue_index, slot_index;

next_header:
  slot_queue_index = ((int)((unsigned int)(state->rx_pos) / VCHIQ_SLOT_SIZE));
  slot_index = state->remote->slot_queue[slot_queue_index & VCHIQ_SLOT_QUEUE_MASK];
  state->rx_data = (char *)(state->slot_data + slot_index);
  rx_pos = state->rx_pos;
  h = (struct vchiq_header_struct *)&state->rx_data[rx_pos & VCHIQ_SLOT_MASK];
  state->rx_pos += vchiq_calc_stride(h->size);
  if (h->msgid == VCHIQ_MSGID_PADDING)
    goto next_header;

  return h;
}

void vchiq_event_signal(struct remote_event_struct *event)
{
  wmb();
  event->fired = 1;
  dsb();
  if (event->armed)
    vchiq_ring_bell();
}

void vchiq_event_wait(atomic_t *waitflag, struct remote_event_struct *event)
{
  if (!event->fired) {
    event->armed = 1;
    dsb();
    wait_on_waitflag(waitflag);
//    if (!event->fired) {
//      while(1) {
//      printf("---");
//      wait_msec(300);
//      }
//    }
    // BUG(!event->fired, "wait finished but event not fired");
    event->armed = 0;
    wmb();
  }
  event->fired = 0;
  rmb();
}

static inline void vchiq_event_check(atomic_t *waitflag, struct remote_event_struct *e)
{
  dsb();
  if (e->armed && e->fired)
    wakeup_waitflag(waitflag);
}

int vchiq_handmade_connect(struct vchiq_state_struct *s)
{
  struct vchiq_header_struct *header;
  int msg_size;
  int err;

  /* Send CONNECT MESSAGE */
  msg_size = 0;
  header = vchiq_prep_next_header_tx(s, msg_size);

  header->msgid = VCHIQ_MAKE_MSG(VCHIQ_MSG_CONNECT, 0, 0);
  header->size = msg_size;

  wmb();
  /* Make the new tx_pos visible to the peer. */
  s->local->tx_pos = s->local_tx_pos;
  wmb();

  vchiq_event_signal(&s->remote->trigger);

  if (s->conn_state != VCHIQ_CONNSTATE_CONNECTED) {
    err = ERR_GENERIC;
    goto out_err;
  }

  return ERR_OK;

out_err:
  return err;
}

#define VC_MMAL_VER 15
#define VC_MMAL_MIN_VER 10

void vchiq_handmade_prep_msg(struct vchiq_state_struct *s, int msgid, int srcport, int dstport, void *payload, int payload_sz)
{
  struct vchiq_header_struct *h;
  int old_tx_pos = s->local->tx_pos;

  h = vchiq_prep_next_header_tx(s, payload_sz);
  MMAL_INFO("msg sent: %p, tx_pos: %d, size: %d", h, old_tx_pos, vchiq_calc_stride(h->size));

  h->msgid = VCHIQ_MAKE_MSG(msgid, srcport, dstport);
  memcpy(h->data, payload, payload_sz);
}

#define VC_SM_VER  1
#define VC_SM_MIN_VER 0

static struct vchiq_service_common *vchiq_service_map[10];

static struct vchiq_service_common *vchiq_alloc_service(void)
{
  struct vchiq_service_common *service;

  int i;
  for (i = 0; i < ARRAY_SIZE(vchiq_service_map); ++i) {
    if (vchiq_service_map[i])
      continue;
    service = kzalloc(sizeof(*service), GFP_KERNEL);
    if (!IS_ERR(service)) {
      /* ser localport to non zero value or vchiq will assign something */
      service->localport = i + 1;
      wmb();
      vchiq_service_map[i] = service;
    }
    return service;
  }
  MMAL_ERR("No free service port");
  return ERR_PTR(ERR_NO_RESOURCE);
}

static struct vchiq_service_common *vchiq_service_find_by_localport(int localport)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(vchiq_service_map); ++i) {
    if (vchiq_service_map[i] && vchiq_service_map[i]->localport == localport)
      return vchiq_service_map[i];
  }
  return NULL;
}

static int vchiq_open_service(struct vchiq_state_struct *state, struct vchiq_service_common *service, uint32_t fourcc, short version, short version_min)
{
  /* Service open payload */
  struct vchiq_open_payload open_payload;

  /* Open "mmal" service */
  open_payload.fourcc = fourcc;
  open_payload.client_id = 0;
  open_payload.version = version;
  open_payload.version_min = version_min;

  service->opened = false;
  service->fourcc = fourcc;

  vchiq_handmade_prep_msg(state, VCHIQ_MSG_OPEN, service->localport, 0, &open_payload, sizeof(open_payload));
  vchiq_event_signal(&state->remote->trigger);
  wait_on_waitflag(&state->state_waitflag);
  BUG(!service->opened, "Service still not opened");
  MMAL_INFO("opened service %08x, localport: %d, remoteport: %d",
    service->fourcc,
    service->localport,
    service->remoteport);
  return ERR_OK;
}

static struct vchiq_service_common *vchiq_alloc_open_service(struct vchiq_state_struct *state,
  uint32_t fourcc, short version, short version_min, vchiq_data_callback_t data_callback)
{
  struct vchiq_service_common *service;
  int err;

  service = vchiq_alloc_service();
  if (IS_ERR(service))
    return service;

  err = vchiq_open_service(state, service, fourcc, version, version_min);
  if (err != ERR_OK) {
    MMAL_ERR("failed to open service");
    kfree(service);
    return ERR_PTR(err);
  }
  service->s = state;
  service->data_callback = data_callback;
  return service;
}

struct mems_msg_context {
  bool active;
  uint32_t trans_id;
  atomic_t completion_waitflag;
  void *data;
  int data_size;
};

static uint32_t mems_transaction_counter = 0;
#define MEMS_CONTEXT_POOL_SIZE 4
static struct mems_msg_context mems_msg_context_pool[MEMS_CONTEXT_POOL_SIZE] = { 0 };

struct mems_msg_context *mems_msg_context_alloc(void)
{
  int i;
  struct mems_msg_context *ctx;
  for (i = 0; i < ARRAY_SIZE(mems_msg_context_pool); ++i) {
    ctx = &mems_msg_context_pool[i];
    if (!ctx->active) {
      ctx->active = true;
      ctx->data = NULL;
      ctx->trans_id = mems_transaction_counter++;
      waitflag_init(&ctx->completion_waitflag);
      return ctx;
    }
  }
  return ERR_PTR(ERR_NO_RESOURCE);
}

struct mems_msg_context *mems_msg_context_from_trans_id(uint32_t trans_id)
{
  int err;
  int i;
  struct mems_msg_context *ctx;
  for (i = 0; i < ARRAY_SIZE(mems_msg_context_pool); ++i) {
    ctx = &mems_msg_context_pool[i];
    if (ctx->trans_id == trans_id) {
      if (!ctx->active) {
        err = ERR_GENERIC;
        MMAL_ERR("mems data callback for inactive message");
        return ERR_PTR(err);
      }
      return ctx;
    }
  }
  err = ERR_NOT_FOUND;
  MMAL_ERR("mems data callback for non-existent context");
  return ERR_PTR(err);
}

void mems_msg_context_free(struct mems_msg_context *ctx)
{
  if (ctx > &mems_msg_context_pool[MEMS_CONTEXT_POOL_SIZE] || ctx < &mems_msg_context_pool[0])
    kernel_panic("mems context free error");
  ctx->active = 0;
}

int mems_service_data_callback(struct vchiq_service_common *s, struct vchiq_header_struct *h)
{
  struct vc_sm_result_t *r;
  // MMAL_INFO("mems_service_data_callback");
  struct mems_msg_context *ctx;
  BUG(h->size < 4, "mems message less than 4");
  r = (struct vc_sm_result_t *)&h->data[0];
  ctx = mems_msg_context_from_trans_id(r->trans_id);
  BUG(IS_ERR(ctx), "mem transaction ctx problem");
  ctx->data = h->data;
  ctx->data_size = h->size;
  wakeup_waitflag(&ctx->completion_waitflag);
  return ERR_OK;
}

struct mmal_msg_context {
  union {
    struct {
      int msg_type;
      atomic_t completion_waitflag;
      struct mmal_msg *rmsg;
      int rmsg_size;
    } sync;
    struct {
      struct vchiq_mmal_port *port;
    } bulk;
  } u;
};

#define MMAL_MSG_CONTEXT_INIT_SYNC(__msg_type) \
{ \
  .u.sync = { \
    .msg_type = MMAL_MSG_TYPE_ ## __msg_type,\
    .completion_waitflag = WAITFLAG_INIT,\
    .rmsg = NULL \
  }\
}



static uint32_t mmal_msg_context_to_handle(struct mmal_msg_context *ctx)
{
  return (uint32_t)(uint64_t)ctx;
}

static struct mmal_msg_context *mmal_msg_context_from_handle(uint32_t handle)
{
  return (struct mmal_msg_context *)(uint64_t)handle;
}

static uint32_t vchiq_service_to_handle(struct vchiq_service_common *s)
{
  return (uint32_t)(uint64_t)s;
}

static struct vchiq_service_common *mmal_msg_service_from_handle(uint32_t handle)
{
  return (struct vchiq_service_common *)(uint64_t)handle;
}

static inline void mmal_buffer_header_make_flags_string(struct mmal_buffer_header *h, char *buf, int bufsz)
{
  int n = 0;

#define CHECK_FLAG(__name) \
  if (h->flags & MMAL_BUFFER_HEADER_FLAG_ ## __name) { \
    if (n != 0 && (bufsz - n >= 2)) { \
      buf[n++] = ','; \
      buf[n++] = ' '; \
    } \
    strncpy(buf + n, #__name, min(sizeof(#__name), bufsz - n));\
  }

  CHECK_FLAG(EOS);
  CHECK_FLAG(FRAME_START);
  CHECK_FLAG(FRAME_END);
  CHECK_FLAG(KEYFRAME);
  CHECK_FLAG(DISCONTINUITY);
  CHECK_FLAG(CONFIG);
  CHECK_FLAG(ENCRYPTED);
  CHECK_FLAG(CODECSIDEINFO);
  CHECK_FLAG(SNAPSHOT);
  CHECK_FLAG(CORRUPTED);
  CHECK_FLAG(TRANSMISSION_FAILED);
#undef CHECK_FLAG
}

static inline void mmal_buffer_print_meta(struct mmal_buffer_header *h)
{
  char flagsbuf[256];
  mmal_buffer_header_make_flags_string(h, flagsbuf, sizeof(flagsbuf));
  MMAL_INFO("buffer_header: %p,hdl:%08x,addr:%08x,sz:%d/%d,%s", h,
    h->data, h->user_data, h->alloc_size, h->length, flagsbuf);
}

static int mmal_port_buffer_send_all(struct vchiq_mmal_port *p);
static int mmal_port_buffer_send_one(struct vchiq_mmal_port *p, struct mmal_buffer *h);

static int mmal_camera_capture_frames(struct vchiq_mmal_component *cam, struct vchiq_mmal_port *capture_port);


extern void tft_lcd_print_data(char *data, int size);

static inline struct mmal_buffer *mmal_port_get_buffer_from_header(struct vchiq_mmal_port *p, struct mmal_buffer_header *h)
{
  struct mmal_buffer *b;
  uint32_t buffer_data;

  list_for_each_entry(b, &p->buffers, list) {
    if (p->zero_copy)
      buffer_data = (uint32_t)(uint64_t)b->vcsm_handle;
    else
      buffer_data = ((uint32_t)(uint64_t)b->buffer) | 0xc0000000;
    if (buffer_data == h->data)
      return b;
  }

  MMAL_ERR("buffer not found for data: %08x", h->data);
  return NULL;
}

static int mmal_port_buffer_io_work(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, struct mmal_buffer_header *h)
{
  int err;
  struct mmal_buffer *b;

  // MMAL_INFO("io_work");
  /*
   * Find the buffer in a list of buffers bound to port
   */
  b = mmal_port_get_buffer_from_header(p, h);
  BUG(!b, "failed to find buffer");

  /*
   * Buffer payload
   */
  if (h->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END) {
    // MMAL_INFO("Received non-EOS, pushing to display");
    tft_lcd_print_data(b->buffer, h->length);
  }

  err = mmal_port_buffer_send_one(p, b);
  CHECK_ERR("Failed to submit buffer");

  if (h->flags & MMAL_BUFFER_HEADER_FLAG_EOS) {
    // MMAL_INFO("EOS received, sending CAPTURE command");
    err = mmal_camera_capture_frames(c, p);
    CHECK_ERR("Failed to initiate frame capture");
  }

  return ERR_OK;
out_err:
  return err;
}

int mmal_buffer_to_host_cb(struct mmal_msg *rmsg)
{
  int err;
  struct vchiq_mmal_port *p;

  struct mmal_msg_buffer_from_host *r;

  r = (struct mmal_msg_buffer_from_host *)&rmsg->u;
  p = (struct vchiq_mmal_port *)(uint64_t)r->drvbuf.client_context;
  mmal_buffer_print_meta(&r->buffer_header);

  // err = mmal_port_buffer_send_all(p);
  // CHECK_ERR("Failed to submit buffer");
  err = mmal_io_work_push(p->component, p, &r->buffer_header, mmal_port_buffer_io_work);
  CHECK_ERR("Failed to schedule mmal io work");
  return ERR_OK;
out_err:
  return err;
}

static int mmal_service_data_callback(struct vchiq_service_common *s, struct vchiq_header_struct *h)
{
  struct mmal_msg *rmsg;
  struct mmal_msg_context *msg_ctx;

  // MMAL_INFO("mmal_service_data_callback");
  rmsg = (struct mmal_msg *)h->data;

  if (rmsg->h.type == MMAL_MSG_TYPE_BUFFER_TO_HOST) {
    return mmal_buffer_to_host_cb(rmsg);
  }

  msg_ctx = mmal_msg_context_from_handle(rmsg->h.context);

  if (msg_ctx->u.sync.msg_type != rmsg->h.type) {
    MMAL_ERR("mmal msg expected type %s, received: %s",
      mmal_msg_type_names[rmsg->h.type],
      mmal_msg_type_names[msg_ctx->u.sync.msg_type]);
    return ERR_INVAL_ARG;
  }
  msg_ctx->u.sync.rmsg = rmsg;
  msg_ctx->u.sync.rmsg_size = h->size;
  wakeup_waitflag(&msg_ctx->u.sync.completion_waitflag);

  return ERR_OK;
}


static inline struct vchiq_service_common *vchiq_open_mmal_service(struct vchiq_state_struct *state)
{
  return vchiq_alloc_open_service(state, MAKE_FOURCC("mmal"), VC_MMAL_VER, VC_MMAL_MIN_VER,
    mmal_service_data_callback);
}

static inline struct vchiq_service_common *vchiq_open_smem_service(struct vchiq_state_struct *state)
{
  return vchiq_alloc_open_service(state, MAKE_FOURCC("SMEM"), VC_SM_VER, VC_SM_MIN_VER,
    mems_service_data_callback);
}

void vchiq_mmal_fill_header(struct vchiq_service_common *service, int mmal_msg_type, struct mmal_msg *msg, struct mmal_msg_context *ctx)
{
  msg->h.magic = MMAL_MAGIC;
  msg->h.context = mmal_msg_context_to_handle(ctx);
  msg->h.control_service = vchiq_service_to_handle(service);
  msg->h.status = 0;
  msg->h.padding = 0;
  msg->h.type = mmal_msg_type;
}

#define VCHIQ_MMAL_MSG_DECL_ASYNC(__ms, __msg_type, __msg_u) \
  struct mmal_msg_context ctx = MMAL_MSG_CONTEXT_INIT_SYNC(__msg_type); \
  struct mmal_msg msg = { 0 }; \
  struct vchiq_service_common *_ms = __ms; \
  struct mmal_msg_ ## __msg_u *m = &msg.u. __msg_u

#define VCHIQ_MMAL_MSG_DECL(__ms, __mmal_msg_type, __msg_u, __msg_u_reply) \
  VCHIQ_MMAL_MSG_DECL_ASYNC(__ms, __mmal_msg_type, __msg_u); \
  struct mmal_msg_ ## __msg_u_reply *r;

#define VCHIQ_MMAL_MSG_COMMUNICATE_ASYNC() \
  waitflag_init(&ctx.u.sync.completion_waitflag); \
  vchiq_mmal_fill_header(_ms, ctx.u.sync.msg_type, &msg, &ctx); \
  vchiq_handmade_prep_msg(_ms->s, VCHIQ_MSG_DATA, _ms->localport, _ms->remoteport, &msg, \
    sizeof(struct mmal_msg_header) + sizeof(*m)); \
  vchiq_event_signal(&_ms->s->remote->trigger);

#define VCHIQ_MMAL_MSG_COMMUNICATE_SYNC() \
  VCHIQ_MMAL_MSG_COMMUNICATE_ASYNC(); \
  wait_on_waitflag(&ctx.u.sync.completion_waitflag); \
  r = mmal_check_reply_msg(ctx.u.sync.rmsg, ctx.u.sync.msg_type); \
  if (!r) { \
    MMAL_ERR("invalid reply");\
    return ERR_GENERIC; \
  } \
  if (r->status != MMAL_MSG_STATUS_SUCCESS) { \
    MMAL_ERR("status not success: %d", r->status); \
    return ERR_GENERIC; \
  }


int vchiq_mmal_handmade_component_disable(struct vchiq_mmal_component *c)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, COMPONENT_DISABLE, component_disable, component_disable_reply);

  BUG(!c->enabled, "trying to disable mmal component, which is already disabled");
  m->component_handle = c->handle;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  c->enabled = false;
  MMAL_INFO("vchiq_mmal_handmade_component_disable, name:%s, handle:%d",
    c->name, c->handle);
  return ERR_OK;
}

int vchiq_mmal_handmade_component_destroy(struct vchiq_service_common *ms, struct vchiq_mmal_component *c)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, COMPONENT_DESTROY, component_destroy, component_destroy_reply);

  BUG(c->enabled, "trying to destroy mmal component, which is not disabled first");
  m->component_handle = c->handle;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  MMAL_INFO("vchiq_mmal_handmade_component_destroy, name:%s, handle:%d",
    c->name, c->handle);
  kfree(c);
  return ERR_OK;
}

int vchiq_mmal_port_info_get(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_INFO_GET, port_info_get, port_info_get_reply);

  m->component_handle = c->handle;
  m->index = p->index;
  m->port_type = p->type;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  if (r->port.is_enabled)
    p->enabled = 1;
  else
    p->enabled = 0;

  p->handle = r->port_handle;
  p->type = r->port_type;
  p->index = r->port_index;

  p->minimum_buffer.num = r->port.buffer_num_min;
  p->minimum_buffer.size = r->port.buffer_size_min;
  p->minimum_buffer.alignment = r->port.buffer_alignment_min;

  p->recommended_buffer.alignment = r->port.buffer_alignment_min;
  p->recommended_buffer.num = r->port.buffer_num_recommended;

  p->current_buffer.num = r->port.buffer_num;
  p->current_buffer.size = r->port.buffer_size;

  /* stream format */
  p->format.type = r->format.type;
  p->format.encoding = r->format.encoding;
  p->format.encoding_variant = r->format.encoding_variant;
  p->format.bitrate = r->format.bitrate;
  p->format.flags = r->format.flags;

  /* elementary stream format */
  memcpy(&p->es, &r->es, sizeof(union mmal_es_specific_format));
  p->format.es = &p->es;

  p->format.extradata_size = r->format.extradata_size;
  memcpy(p->format.extradata, r->extradata, p->format.extradata_size);
  p->component = c;
  return ERR_OK;
}

int mmal_port_create(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, enum mmal_port_type type, int index)
{
  int err;

  /* Type and index are needed for port info get */
  p->type = type;
  p->index = index;

  err = vchiq_mmal_port_info_get(c, p);
  CHECK_ERR("Failed to get port info");

  INIT_LIST_HEAD(&p->buffers);
  p->component = c;
  return ERR_OK;
out_err:
  return err;
}

int mmal_component_destroy(struct vchiq_service_common *mmal_service, struct vchiq_mmal_component *c)
{
  return ERR_OK;
}

int mmal_component_create(struct vchiq_service_common *mmal_service, const char *name, int component_idx, struct vchiq_mmal_component *c)
{
  VCHIQ_MMAL_MSG_DECL(mmal_service, COMPONENT_CREATE, component_create, component_create_reply);

  m->client_component = component_idx;
  strncpy(m->name, name, sizeof(m->name));

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();

  c->handle = r->component_handle;
  c->ms = mmal_service;
  c->enabled = true;
  c->inputs = r->input_num;
  c->outputs = r->output_num;
  c->clocks = r->clock_num;
  return ERR_OK;
}

struct vchiq_mmal_component *component_create(struct vchiq_service_common *mmal_service, const char *name)
{
  int err, i;
  struct vchiq_mmal_component *c;
  c = kzalloc(sizeof(*c), GFP_KERNEL);
  if (IS_ERR(c)) {
    MMAL_ERR("failed to allocate memory for mmal_component");
    return c;
  }
  err = mmal_component_create(mmal_service, name, 0, c);
  if (err != ERR_OK)
    goto err_component_free;

  strncpy(c->name, name, sizeof(c->name));

  MMAL_INFO("vchiq component created name:%s: handle: %d, input: %d, output: %d, clock: %d",
    c->name, c->handle, c->inputs, c->outputs, c->clocks);

  err = mmal_port_create(c, &c->control, MMAL_PORT_TYPE_CONTROL, 0);
  if (err != ERR_OK)
    goto err_component_destroy;

  for (i = 0; i < c->inputs; ++i) {
    err = mmal_port_create(c, &c->input[i], MMAL_PORT_TYPE_INPUT, i);
    if (err != ERR_OK)
      goto err_component_destroy;
  }

  for (i = 0; i < c->outputs; ++i) {
    err = mmal_port_create(c, &c->output[i], MMAL_PORT_TYPE_OUTPUT, i);
    if (err != ERR_OK)
      goto err_component_destroy;
  }

  return c;
err_component_destroy:
  mmal_component_destroy(mmal_service, c);

err_component_free:
  kfree(c);
  return ERR_PTR(err);
}

static inline struct vchiq_mmal_component *vchiq_mmal_create_camera_info(struct vchiq_service_common *mmal_service)
{
  return component_create(mmal_service, "camera_info");
}

int vchiq_mmal_port_info_set(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_INFO_SET, port_info_set, port_info_set_reply);

  m->component_handle = c->handle;
  m->port_type = p->type;
  m->port_index = p->index;

  port_to_mmal_msg(p, &m->port);
  m->format.type = p->format.type;
  m->format.encoding = p->format.encoding;
  m->format.encoding_variant = p->format.encoding_variant;
  m->format.bitrate = p->format.bitrate;
  m->format.flags = p->format.flags;

  memcpy(&m->es, &p->es, sizeof(union mmal_es_specific_format));

  m->format.extradata_size = p->format.extradata_size;
  memcpy(&m->extradata, p->format.extradata, p->format.extradata_size);
  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_mmal_port_parameter_set(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p,
  uint32_t parameter_id, void *value, int value_size)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_PARAMETER_SET, port_parameter_set, port_parameter_set_reply);

  /* GET PARAMETER CAMERA INFO */
  m->component_handle = c->handle;
  m->port_handle = p->handle;
  m->id = parameter_id;
  m->size = 2 * sizeof(uint32_t) + value_size;
  memcpy(&m->value, value, value_size);

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  if (parameter_id == MMAL_PARAMETER_ZERO_COPY)
    p->zero_copy = 1;
  return ERR_OK;
}

int vchiq_mmal_port_set_format(struct vchiq_mmal_component *c, struct vchiq_mmal_port *port)
{
  int err;

  err = vchiq_mmal_port_info_set(c, port);
  CHECK_ERR("failed to set port info");

  err = vchiq_mmal_port_info_get(c, port);
  CHECK_ERR("failed to get port info");

out_err:
  return err;
}

int vchiq_mmal_port_parameter_get(struct vchiq_mmal_component *c,
  struct vchiq_mmal_port *port, int parameter_id, void *value, uint32_t *value_size)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_PARAMETER_GET, port_parameter_get, port_parameter_get_reply);

  m->component_handle = c->handle;
  m->port_handle = port->handle;
  m->id = parameter_id;
  m->size = 2 * sizeof(uint32_t) + *value_size;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();

  memcpy(value, r->value, min(r->size, *value_size));
  *value_size = r->size;
  return ERR_OK;
}

static inline int vchiq_mmal_get_camera_info(struct vchiq_service_common *ms, struct vchiq_mmal_component *c, struct mmal_parameter_camera_info_t *cam_info)
{
  int err;
  uint32_t param_size;

  param_size = sizeof(*cam_info);
  err = vchiq_mmal_port_parameter_get(c, &c->control,
    MMAL_PARAMETER_CAMERA_INFO, cam_info, &param_size);
  return err;
}

void vchiq_mmal_cam_info_print(struct mmal_parameter_camera_info_t *cam_info)
{
  int i;
  struct mmal_parameter_camera_info_camera_t *c;

  for (i = 0; i < cam_info->num_cameras; ++i) {
    c = &cam_info->cameras[i];
    MMAL_INFO("cam %d: name:%s, W x H: %dx%x\r\n", i, c->camera_name, c->max_width, c->max_height);
  }
}

int mmal_set_camera_parameters(struct vchiq_mmal_component *c, struct mmal_parameter_camera_info_camera_t *cam_info)
{
  uint32_t config_size;
  struct mmal_parameter_camera_config config = {
    .max_stills_w = cam_info->max_width,
    .max_stills_h = cam_info->max_height,
    .stills_yuv422 = 1,
    .one_shot_stills = 1,
    .max_preview_video_w = max(1920, cam_info->max_width),
    .max_preview_video_h = max(1088, cam_info->max_height),
    .num_preview_video_frames = 3,
    .stills_capture_circular_buffer_height = 0,
    .fast_preview_resume = 0,
    .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC
  }, new_config;

  config_size = sizeof(new_config);

  vchiq_mmal_port_parameter_set(c, &c->control,
    MMAL_PARAMETER_CAMERA_CONFIG, &config, sizeof(config));
  vchiq_mmal_port_parameter_get(c, &c->control,
    MMAL_PARAMETER_CAMERA_CONFIG, &new_config, &config_size);
  return ERR_OK;
}

int vchiq_mmal_port_action_port(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, int action)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_ACTION, port_action_port, port_action_reply);

  m->component_handle = c->handle;
  m->port_handle = p->handle;
  m->action = action;
  port_to_mmal_msg(p, &m->port);

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_mmal_port_action_handle(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, int action, int dst_component_handle, int dst_port_handle)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, PORT_ACTION, port_action_handle, port_action_reply);

  m->component_handle = c->handle;
  m->port_handle = p->handle;
  m->action = action;
  m->connect_component_handle = dst_component_handle;
  m->connect_port_handle = dst_port_handle;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_mmal_buffer_from_host(struct vchiq_mmal_port *p, struct mmal_buffer *b)
{
  VCHIQ_MMAL_MSG_DECL_ASYNC(p->component->ms, BUFFER_FROM_HOST, buffer_from_host);

  memset(m, 0xbc, sizeof(*m));

  m->drvbuf.magic = MMAL_MAGIC;
  m->drvbuf.component_handle = p->component->handle;
  m->drvbuf.port_handle = p->handle;
  m->drvbuf.client_context = (uint32_t)(uint64_t)p;

  m->is_zero_copy = p->zero_copy;
  m->buffer_header.next = 0;
  m->buffer_header.priv = 0;
  m->buffer_header.cmd = 0;
  m->buffer_header.user_data =(uint32_t)(uint64_t)b;
  if (p->zero_copy)
    m->buffer_header.data = (uint32_t)(uint64_t)b->vcsm_handle;
  else
    m->buffer_header.data = ((uint32_t)(uint64_t)b->buffer) | 0xc0000000;
  m->buffer_header.alloc_size = b->buffer_size;

  if (p->type == MMAL_PORT_TYPE_OUTPUT) {
    m->buffer_header.length = 0;
    m->buffer_header.offset = 0;
    m->buffer_header.flags = 0;
    m->buffer_header.pts = MMAL_TIME_UNKNOWN;
    m->buffer_header.dts = MMAL_TIME_UNKNOWN;
  } else {
    m->buffer_header.length = b->length;
    m->buffer_header.offset = 0;
    m->buffer_header.flags = b->mmal_flags;
    m->buffer_header.pts = b->pts;
    m->buffer_header.dts = b->dts;
  }
  memset(&m->buffer_header_type_specific, 0,sizeof( m->buffer_header_type_specific));
  m->payload_in_message = 0;

  VCHIQ_MMAL_MSG_COMMUNICATE_ASYNC();
  return ERR_OK;
}

int vchiq_mmal_component_enable(struct vchiq_mmal_component *c)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, COMPONENT_ENABLE, component_enable, component_enable_reply);

  m->component_handle = c->handle;
  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_mmal_port_enable(struct vchiq_mmal_port *p)
{
  int err;

  if (p->enabled) {
    MMAL_INFO("skipping port already enabled");
    return ERR_OK;
  }
  err = vchiq_mmal_port_action_port(p->component, p, MMAL_MSG_PORT_ACTION_TYPE_ENABLE);
  CHECK_ERR("port action type enable failed");
  p->enabled = 1;
out_err:
  return vchiq_mmal_port_info_get(p->component, p);
}

int vchiq_mmal_create_tunnel(struct vchiq_mmal_component *c, struct vchiq_mmal_port *src, struct vchiq_mmal_component *dst_c, struct vchiq_mmal_port *dst)
{
  int err;
  /* copy src port format to dst */
  dst->format.encoding = src->format.encoding;
  dst->es.video.width = src->es.video.width;
  dst->es.video.height = src->es.video.height;
  dst->es.video.crop.x = src->es.video.crop.x;
  dst->es.video.crop.y = src->es.video.crop.y;
  dst->es.video.crop.width = src->es.video.crop.width;
  dst->es.video.crop.height = src->es.video.crop.height;
  dst->es.video.frame_rate.num = src->es.video.frame_rate.num;
  dst->es.video.frame_rate.den = src->es.video.frame_rate.den;
  err = vchiq_mmal_port_set_format(dst_c, dst);
  CHECK_ERR("Failed to set format");
  err = vchiq_mmal_port_action_handle(c, src, MMAL_MSG_PORT_ACTION_TYPE_CONNECT, dst_c->handle, dst->handle);
  CHECK_ERR("Failed to connect ports");
out_err:
  return err;
}


/* Command blocks come from a pool */
#define SM_MAX_NUM_CMD_RSP_BLKS 32

/* The number of supported connections */
#define SM_MAX_NUM_CONNECTIONS 3

struct sm_cmd_rsp_blk {
  struct list_head head;  /* To create lists */
  /* To be signaled when the response is there */
  struct completion cmplt;
  uint32_t id;
  uint16_t length;

  uint8_t msg[VC_SM_MAX_MSG_LEN];
  uint32_t wait:1;
  uint32_t sent:1;
  uint32_t alloc:1;
};

struct sm_instance {
  uint32_t num_connections;
  unsigned int service_handle[SM_MAX_NUM_CONNECTIONS];
  struct task_struct *io_thread;
  struct completion io_cmplt;

//   vpu_event_cb vpu_event;

  /* Mutex over the following lists */
  struct mutex lock;
  uint32_t trans_id;
  struct list_head cmd_list;
  struct list_head rsp_list;
  struct list_head dead_list;
  struct sm_cmd_rsp_blk free_blk[SM_MAX_NUM_CMD_RSP_BLKS];
  /* Mutex over the free_list */
  struct mutex free_lock;
  struct list_head free_list;
  struct semaphore free_sema;
};

static int vc_trans_id = 0;
static struct
sm_cmd_rsp_blk *vc_vchi_cmd_create(enum vc_sm_msg_type id, void *msg,
           uint32_t size, int wait)
{
  struct sm_cmd_rsp_blk *blk;
  struct vc_sm_msg_hdr_t *hdr;

  blk = kmalloc(sizeof(*blk), GFP_KERNEL);
  if (!blk)
    return NULL;

  blk->alloc = 1;

  blk->sent = 0;
  blk->wait = wait;
  blk->length = sizeof(*hdr) + size;

  hdr = (struct vc_sm_msg_hdr_t *)blk->msg;
  hdr->type = id;
  /*
   * Retain the top bit for identifying asynchronous events, or VPU cmds.
   */
  hdr->trans_id = vc_trans_id;
  *(uint8_t*)&blk->id = vc_trans_id;
  vc_trans_id++;

  if (size)
    memcpy(hdr->body, msg, size);

  return blk;
}

#define VC_SM_RESOURCE_NAME_DEFAULT       "sm-host-resource"

static int vc_sm_cma_vchi_send_msg(struct vchiq_service_common *mems_service,
  enum vc_sm_msg_type msg_id, void *msg,
  uint32_t msg_size, void *result, uint32_t result_size,
  uint32_t *cur_trans_id, uint8_t wait_reply)
{
  int err;
  char buf[256];
  struct vc_sm_msg_hdr_t *hdr = (struct vc_sm_msg_hdr_t *)buf;
  struct mems_msg_context *ctx;
  ctx = mems_msg_context_alloc();
  if (IS_ERR(ctx)) {
    err = PTR_ERR(ctx);
    MMAL_ERR("Failed to allocate mems message context");
    return err;
  }
  // struct sm_cmd_rsp_blk *cmd;
  hdr->type = msg_id;
  hdr->trans_id = vc_trans_id++;

  if (msg_size)
    memcpy(hdr->body, msg, msg_size);

  // cmd = vc_vchi_cmd_create(msg_id, msg, msg_size, 1);
  // cmd->sent = 1;

  vchiq_handmade_prep_msg(
    mems_service->s,
    VCHIQ_MSG_DATA,
    mems_service->localport,
    mems_service->remoteport,
    hdr,
    msg_size + sizeof(*hdr));

  vchiq_event_signal(&mems_service->s->remote->trigger);
  wait_on_waitflag(&ctx->completion_waitflag);

  memcpy(result, ctx->data, ctx->data_size);
  mems_msg_context_free(ctx);
  return ERR_OK;
}

int vc_sm_cma_vchi_import(struct vchiq_service_common *mems_service, struct vc_sm_import *msg,
  struct vc_sm_import_result *result, uint32_t *cur_trans_id)
{
  return vc_sm_cma_vchi_send_msg(mems_service, VC_SM_MSG_TYPE_IMPORT, msg,
   sizeof(*msg), result, sizeof(*result), cur_trans_id, 1);
}

int vc_sm_cma_import_dmabuf(struct vchiq_service_common *mems_service, struct mmal_buffer *b, void **vcsm_handle)
{
  int err;
  struct vc_sm_import import;
  struct vc_sm_import_result result;
  uint32_t cur_trans_id = 0;
  import.type = VC_SM_ALLOC_NON_CACHED;
  import.allocator = 0x66aa;
  import.addr = (uint32_t)(uint64_t)b->buffer | 0xc0000000;
  import.size = b->buffer_size;
  import.kernel_id = 0x6677;

  memcpy(import.name, VC_SM_RESOURCE_NAME_DEFAULT, sizeof(VC_SM_RESOURCE_NAME_DEFAULT));
  err = vc_sm_cma_vchi_import(mems_service, &import, &result, &cur_trans_id);
  CHECK_ERR("Failed to import buffer to vc");
  MMAL_INFO("imported_dmabuf: addr:%08x, size: %d, trans_id: %08x, res.trans_id: %08x, res.handle: %08x",
    import.addr, import.size, cur_trans_id, result.trans_id, result.res_handle);

  *vcsm_handle = (void*)(uint64_t)result.res_handle;

  return ERR_OK;

out_err:
  return err;
}

int mmal_alloc_port_buffers(struct vchiq_service_common *mems_service, struct vchiq_mmal_port *p)
{
  int err;
  int i;
  struct mmal_buffer *buf;
  for (i = 0; i < p->minimum_buffer.num * 2; ++i) {
    buf = kzalloc(sizeof(*buf), GFP_KERNEL);
    buf->buffer_size = p->minimum_buffer.size;
    buf->buffer = dma_alloc(buf->buffer_size);;
    MMAL_INFO("min_num: %d, min_sz:%d, min_al:%d, port_enabled:%s, buf:%08x",
      p->minimum_buffer.num,
      p->minimum_buffer.size,
      p->minimum_buffer.alignment,
      p->enabled ? "yes" : "no",
      buf->buffer);

    if (p->zero_copy) {
      err = vc_sm_cma_import_dmabuf(mems_service, buf, &buf->vcsm_handle);
      CHECK_ERR("failed to import dmabuf");
    }
    list_add_tail(&buf->list, &p->buffers);
  }
  return ERR_OK;

out_err:
  return err;
}

int mmal_camera_enable(struct vchiq_mmal_component *cam)
{
  int err;
  uint32_t camera_num;
  camera_num = 0;
  err = vchiq_mmal_port_parameter_set(cam, &cam->control, MMAL_PARAMETER_CAMERA_NUM, &camera_num, sizeof(camera_num));
  CHECK_ERR("Failed to set camera_num parameter");
  err = vchiq_mmal_component_enable(cam);
  wait_msec(300);
  return ERR_OK;
out_err:
  return err;
}

int vchiq_mmal_get_cam_info(struct vchiq_service_common *ms, struct mmal_parameter_camera_info_t *cam_info)
{
  int err;
  struct vchiq_mmal_component *camera_info;

  camera_info = vchiq_mmal_create_camera_info(ms);
  err = vchiq_mmal_get_camera_info(ms, camera_info, cam_info);
  CHECK_ERR("Failed to get camera info");

  vchiq_mmal_cam_info_print(cam_info);
  err = vchiq_mmal_handmade_component_disable(camera_info);
  CHECK_ERR("Failed to disable 'camera info' component");

  err = vchiq_mmal_handmade_component_destroy(ms, camera_info);
  CHECK_ERR("Failed to destroy 'camera info' component");
  return ERR_OK;

out_err:
  return err;
}

void vchiq_mmal_local_format_fill(struct mmal_es_format_local *f, int encoding, int encoding_variant, int width, int height)
{
  f->encoding = encoding;
  f->encoding_variant = encoding_variant;
  f->es->video.width = width;
  f->es->video.height = height;
  f->es->video.crop.x = 0;
  f->es->video.crop.y = 0;
  f->es->video.crop.width = width;
  f->es->video.crop.height = height;
  f->es->video.frame_rate.num = 1;
  f->es->video.frame_rate.den = 0;
}

static struct mmal_fmt formats[] = {
	{
		.name = "4:2:0, planar, YUV",
		.fourcc = V4L2_PIX_FMT_YUV420,
		.flags = 0,
		.mmal = MMAL_ENCODING_I420,
		.depth = 12,
		.mmal_component = COMP_CAMERA,
		.ybbp = 1,
		.remove_padding = 1,
	}, {
		.name = "4:2:2, packed, YUYV",
		.fourcc = V4L2_PIX_FMT_YUYV,
		.flags = 0,
		.mmal = MMAL_ENCODING_YUYV,
		.depth = 16,
		.mmal_component = COMP_CAMERA,
		.ybbp = 2,
		.remove_padding = 0,
	}, {
		.name = "RGB24 (LE)",
		.fourcc = V4L2_PIX_FMT_RGB24,
		.flags = 0,
		.mmal = MMAL_ENCODING_RGB24,
		.depth = 24,
		.mmal_component = COMP_CAMERA,
		.ybbp = 3,
		.remove_padding = 0,
	}, {
		.name = "JPEG",
		.fourcc = V4L2_PIX_FMT_JPEG,
		.flags = V4L2_FMT_FLAG_COMPRESSED,
		.mmal = MMAL_ENCODING_JPEG,
		.depth = 8,
		.mmal_component = COMP_IMAGE_ENCODE,
		.ybbp = 0,
		.remove_padding = 0,
	}, {
		.name = "H264",
		.fourcc = V4L2_PIX_FMT_H264,
		.flags = V4L2_FMT_FLAG_COMPRESSED,
		.mmal = MMAL_ENCODING_H264,
		.depth = 8,
		.mmal_component = COMP_VIDEO_ENCODE,
		.ybbp = 0,
		.remove_padding = 0,
	}, {
		.name = "MJPEG",
		.fourcc = V4L2_PIX_FMT_MJPEG,
		.flags = V4L2_FMT_FLAG_COMPRESSED,
		.mmal = MMAL_ENCODING_MJPEG,
		.depth = 8,
		.mmal_component = COMP_VIDEO_ENCODE,
		.ybbp = 0,
		.remove_padding = 0,
	}, {
		.name = "4:2:2, packed, YVYU",
		.fourcc = V4L2_PIX_FMT_YVYU,
		.flags = 0,
		.mmal = MMAL_ENCODING_YVYU,
		.depth = 16,
		.mmal_component = COMP_CAMERA,
		.ybbp = 2,
		.remove_padding = 0,
	}, {
		.name = "4:2:2, packed, VYUY",
		.fourcc = V4L2_PIX_FMT_VYUY,
		.flags = 0,
		.mmal = MMAL_ENCODING_VYUY,
		.depth = 16,
		.mmal_component = COMP_CAMERA,
		.ybbp = 2,
		.remove_padding = 0,
	}, {
		.name = "4:2:2, packed, UYVY",
		.fourcc = V4L2_PIX_FMT_UYVY,
		.flags = 0,
		.mmal = MMAL_ENCODING_UYVY,
		.depth = 16,
		.mmal_component = COMP_CAMERA,
		.ybbp = 2,
		.remove_padding = 0,
	}, {
		.name = "4:2:0, planar, NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
		.flags = 0,
		.mmal = MMAL_ENCODING_NV12,
		.depth = 12,
		.mmal_component = COMP_CAMERA,
		.ybbp = 1,
		.remove_padding = 1,
	}, {
		.name = "RGB24 (BE)",
		.fourcc = V4L2_PIX_FMT_BGR24,
		.flags = 0,
		.mmal = MMAL_ENCODING_BGR24,
		.depth = 24,
		.mmal_component = COMP_CAMERA,
		.ybbp = 3,
		.remove_padding = 0,
	}, {
		.name = "4:2:0, planar, YVU",
		.fourcc = V4L2_PIX_FMT_YVU420,
		.flags = 0,
		.mmal = MMAL_ENCODING_YV12,
		.depth = 12,
		.mmal_component = COMP_CAMERA,
		.ybbp = 1,
		.remove_padding = 1,
	}, {
		.name = "4:2:0, planar, NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
		.flags = 0,
		.mmal = MMAL_ENCODING_NV21,
		.depth = 12,
		.mmal_component = COMP_CAMERA,
		.ybbp = 1,
		.remove_padding = 1,
	}, {
		.name = "RGB32 (BE)",
		.fourcc = V4L2_PIX_FMT_BGR32,
		.flags = 0,
		.mmal = MMAL_ENCODING_BGRA,
		.depth = 32,
		.mmal_component = COMP_CAMERA,
		.ybbp = 4,
		.remove_padding = 0,
	},
};

static inline struct mmal_fmt *get_format(struct v4l2_format *f)
{
    struct mmal_fmt *fmt;
      unsigned int k;

        for (k = 0; k < ARRAY_SIZE(formats); k++) {
              fmt = &formats[k];
                  if (fmt->fourcc == f->fmt.pix.pixelformat)
                          return fmt;
                    }

          return NULL;
}

int vchiq_mmal_setup_encode_component(
  struct v4l2_format *f,
  struct vchiq_mmal_component *cam,
  struct vchiq_mmal_port *camera_port,
  struct vchiq_mmal_component *encode_component,
  struct vchiq_mmal_port *port)
{
  int err;
  struct mmal_fmt *mfmt = get_format(f);

  camera_port->current_buffer.size = camera_port->recommended_buffer.size;
  camera_port->current_buffer.num = camera_port->recommended_buffer.num;
  err = vchiq_mmal_create_tunnel(cam, camera_port, encode_component, &encode_component->input[0]);
  CHECK_ERR("failed to create tunnel");

  port->es.video.width = f->fmt.pix.width;
  port->es.video.height = f->fmt.pix.height;
  port->es.video.crop.x = 0;
  port->es.video.crop.y = 0;
  port->es.video.crop.width = f->fmt.pix.width;
  port->es.video.crop.height = f->fmt.pix.height;
  port->es.video.frame_rate.num = 0;
  port->es.video.frame_rate.den = 1;
  port->format.encoding = mfmt->mmal;
  port->format.encoding_variant = 0;

  err = vchiq_mmal_port_set_format(encode_component, port);
  CHECK_ERR("Failed to set format for camera port");
  err = vchiq_mmal_component_enable(encode_component);
  CHECK_ERR("Failed to enabled encode component");
  port->current_buffer.num = 1;
  port->current_buffer.size = f->fmt.pix.sizeimage;
  if (port->format.encoding == MMAL_ENCODING_JPEG) {
    MMAL_INFO("JPG - buf size now %d was %d", f->fmt.pix.sizeimage, port->current_buffer.size);
    port->current_buffer.size = (f->fmt.pix.sizeimage < (100 << 10)) ?  (100 << 10) : f->fmt.pix.sizeimage;
  }
  port->current_buffer.alignment = 0;
  return ERR_OK;
out_err:
  return err;
}

int vchiq_mmal_setup_components(struct vchiq_mmal_component *cam, struct vchiq_mmal_component *encode_component, struct v4l2_format *f, struct vchiq_mmal_port **out_camera_port, struct vchiq_mmal_port **out_encode_port)
{
  int err;
  struct vchiq_mmal_port *port = NULL, *camera_port = NULL;
  struct mmal_fmt *mfmt = get_format(f);
  // uint32_t remove_padding;
  switch (mfmt->mmal_component) {
    case COMP_IMAGE_ENCODE:
      port = &encode_component->output[0];
      camera_port = &cam->output[CAM_PORT_CAPTURE];
      break;
    default:
      MMAL_ERR("unsupported mmal component");
      return ERR_INVAL_ARG;
  }
  camera_port->format.encoding = MMAL_ENCODING_OPAQUE;
  camera_port->format.encoding_variant = 0;
  camera_port->es.video.width = f->fmt.pix.width;
  camera_port->es.video.height = f->fmt.pix.height;
  camera_port->es.video.crop.x = 0;
  camera_port->es.video.crop.y = 0;
  camera_port->es.video.crop.width = f->fmt.pix.width;
  camera_port->es.video.crop.height = f->fmt.pix.height;
  camera_port->es.video.frame_rate.num = 0;
  camera_port->es.video.frame_rate.den = 1;
  camera_port->es.video.color_space = MMAL_COLOR_SPACE_JPEG_JFIF;
  err = vchiq_mmal_port_set_format(cam, camera_port);
  CHECK_ERR("Failed to set format for camera port");

  /* skipping setup video component */
  err = vchiq_mmal_setup_encode_component(f, cam, camera_port, encode_component, port);
  CHECK_ERR("Failed to setup encode component");
  *out_camera_port = camera_port;
  *out_encode_port = port;
  return ERR_OK;
out_err:
  return err;
}

//static struct v4l2_format default_v4l2_format = {
//  .fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG,
//  .fmt.pix.width = 1024,
//  .fmt.pix.bytesperline = 0,
//  .fmt.pix.height = 768,
//  .fmt.pix.sizeimage = 1024 * 768,
//};

static inline int mmal_port_set_zero_copy(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p)
{
  uint32_t zero_copy = 1;
  return vchiq_mmal_port_parameter_set(c, p, MMAL_PARAMETER_ZERO_COPY, &zero_copy, sizeof(zero_copy));
}

static inline void mmal_print_supported_encodings(uint32_t *encodings, int num)
{
  int i, ii;
  char buf[5];
  buf[4] = 0;
  char *ptr = (char *)encodings;
  for (i = 0; i < num; ++i) {
    for (ii = 0; ii < 4; ++ii) {
      buf[ii] = ptr[ii];
    }
    ptr += 4;
    MMAL_INFO("supported_encoding: %s", buf);
  }
}

static int mmal_camera_capture_frames(struct vchiq_mmal_component *cam, struct vchiq_mmal_port *capture_port)
{
  uint32_t frame_count = 1;
  return vchiq_mmal_port_parameter_set(cam, capture_port, MMAL_PARAMETER_CAPTURE, &frame_count, sizeof(frame_count));
}

static int mmal_port_buffer_send_one(struct vchiq_mmal_port *p, struct mmal_buffer *b)
{
  int err;
  struct mmal_buffer *pb;

  list_for_each_entry(pb, &p->buffers, list) {
    if (pb == b) {
      err = vchiq_mmal_buffer_from_host(p, pb);
      if (err) {
        MMAL_ERR("failed to submit port buffer to VC");
        return err;
      }
      return ERR_OK;
    }
  }

  MMAL_ERR("Invalid buffer in request: %p: %08x", b, b->buffer);
  return ERR_INVAL_ARG;
}

static int mmal_port_buffer_send_all(struct vchiq_mmal_port *p)
{
  int err;
  struct mmal_buffer *b;

  if (list_empty(&p->buffers)) {
    MMAL_ERR("port buffer list is empty");
    return ERR_NO_RESOURCE;
  }

  list_for_each_entry(b, &p->buffers, list) {
    err = vchiq_mmal_buffer_from_host(p, b);
    if (err) {
      MMAL_ERR("failed to submit port buffer to VC");
      return err;
    }
  }

  return ERR_OK;
}

static int mmal_port_buffer_receive(struct vchiq_mmal_port *p)
{
  while(1) {
    asm volatile ("wfe");
    yield();
  }
  return ERR_OK;
}


int mmal_port_get_supp_encodings(struct vchiq_mmal_port *p, uint32_t *encodings, int max_encodings, int *num_encodings)
{
  int err;
  uint32_t param_size;

  param_size = max_encodings * sizeof(*encodings);
  err = vchiq_mmal_port_parameter_get(p->component, p, MMAL_PARAMETER_SUPPORTED_ENCODINGS, encodings, &param_size);
  CHECK_ERR("Failed to get supported_encodings");
  *num_encodings = param_size / sizeof(*encodings);
  mmal_print_supported_encodings(encodings, *num_encodings);
  return ERR_OK;
out_err:
  return err;
}

int vchiq_camera_run(struct vchiq_service_common *mmal_service, struct vchiq_service_common *mems_service, int frame_width, int frame_height)
{
  int err;
  struct mmal_parameter_camera_info_t cam_info = {0};
  struct vchiq_mmal_component *cam;
  struct vchiq_mmal_port *still_port, *video_port, *preview_port;
  uint32_t supported_encodings[MAX_SUPPORTED_ENCODINGS];
  int num_encodings;

  err = vchiq_mmal_get_cam_info(mmal_service, &cam_info);
  CHECK_ERR("Failed to get num cameras");

  /* mmal_init start */
  cam = component_create(mmal_service, "ril.camera");
  CHECK_ERR_PTR(cam, "Failed to create component 'ril.camera'");

  err =  mmal_set_camera_parameters(cam, &cam_info.cameras[0]);
  CHECK_ERR("Failed to set parameters to component 'ril.camera'");

  still_port = &cam->output[CAM_PORT_CAPTURE];
  video_port = &cam->output[CAM_PORT_VIDEO];
  preview_port = &cam->output[CAM_PORT_PREVIEW];

  err = mmal_port_get_supp_encodings(still_port, supported_encodings, sizeof(supported_encodings), &num_encodings);
  CHECK_ERR("Failed to retrieve supported encodings from port");

  vchiq_mmal_local_format_fill(&still_port->format, MMAL_ENCODING_RGB24, 0, frame_width, frame_height);
  err = vchiq_mmal_port_set_format(cam, still_port);
  CHECK_ERR("Failed to set format for still capture port");

  vchiq_mmal_local_format_fill(&video_port->format, MMAL_ENCODING_OPAQUE, 0, frame_width, frame_height);
  err = vchiq_mmal_port_set_format(cam, video_port);
  CHECK_ERR("Failed to set format for video capture port");

  vchiq_mmal_local_format_fill(&preview_port->format, MMAL_ENCODING_OPAQUE, 0, frame_width, frame_height);
  err = vchiq_mmal_port_set_format(cam, preview_port);
  CHECK_ERR("Failed to set format for preview capture port");

  err = mmal_port_set_zero_copy(cam, still_port);
  CHECK_ERR("Failed to set zero copy param for camera port");
  err = mmal_camera_enable(cam);
  CHECK_ERR("Failed to enable component camera");

  err = vchiq_mmal_port_enable(still_port);
  CHECK_ERR("Failed to enable video_port");

  err = mmal_alloc_port_buffers(mems_service, still_port);
  CHECK_ERR("Failed to prepare buffer for still port");

  while(1) {
    err = mmal_port_buffer_send_all(still_port);
    CHECK_ERR("Failed to send buffer to port");
    mmal_camera_capture_frames(cam, still_port);
    CHECK_ERR("Failed to initiate frame capture");
    err = mmal_port_buffer_receive(still_port);
    CHECK_ERR("Failed to receive buffer from VC");
  }
  return ERR_OK;

out_err:
  return err;
}

#define BELL0 ((reg32_t)(0x3f00b840))

static void vchiq_check_local_events()
{
  struct vchiq_shared_state_struct *local;
  local = vchiq_state.local;

//  if (local->trigger.fired) {
//    putc('*');
//  }
  vchiq_event_check(&vchiq_state.trigger_waitflag, &local->trigger);
//  if (local->recycle.fired) {
//    putc('+');
//  }

  vchiq_event_check(&vchiq_state.recycle_waitflag, &local->recycle);
  vchiq_event_check(&vchiq_state.sync_trigger_waitflag, &local->sync_trigger);
  vchiq_event_check(&vchiq_state.sync_release_waitflag, &local->sync_release);
}

static void vchiq_irq_handler(void)
{
  uint32_t bell_reg;
  vchiq_events++;
  // printf("vchiq_irq_handler\r\n");

  /*
   * This is a very specific undocumented part of vchiq IRQ
   * handling code, taken as is from linux kernel.
   * BELL0 register is obviously a clear-on-read, once we
   * read the word at this address, the second read shows
   * zero, thus this is a clear-on-read.
   * Mostly in this code path bell_reg will read 0x5 in the
   * next line. And bit 3 (1<<3 == 4) means the bell was rung
   * on the VC side.
   * This thread discusses the documentation for this register:
   * https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=280271&p=1830423&hilit=doorbell#p1830423
   * The thread ends with the understanding that this register is very specific to VCHIQ and is
   * not documented anywhere, so waste your time on your own risk.
   *
   * Anyways, the architecture of event handling is this:
   * VideoCore has a couple of situations when it wants to send signals to us:
   * 1. It needs us to recycle some old messages that waste queue space.
   * 2. It wants to send us some message.
   * All of which is done via special events in the zero slot.
   * For recycling 'recycle_event' is used.
   * For messages 'trigger_event' is used.
   * I don't know how sync_event is used, might be for other kind of messages.
   * To signal one of these events VideoCore sets event->fired = 1, and then writes something to BELL0,
   * which is 0x3f00b840 on Raspberry PI 3B+.
   * Writing to this address triggers IRQ Exception in one of the CPU cores, which one depends on where
   * peripheral interrupts are routed. IRQ will only get triggered if DOORBELL0 iterrupts are enabled
   * in bcm interrupt controller, the one on 0x3f00b200. To enable it set bit 2 in
   * base interrupt enable register 0x3f00b218.
   * DOORBELL 0 has irq number 66 in irq_table. The meaning of 66 is this:
   *    there are 64 GPU interrupts that are in 2 32bit registers pending(0x204,0x208),
   *    enable(0x210,0x214), etc, so they have numbers 0-63.
   *    next number 64 is basic pending register bit 0, 65, is basic bit 1, and the
   *    doorbell 0 is basic bit 2, thus 64+2 = 66.
   */
  bell_reg = read_reg(BELL0);
  if (bell_reg & 4)
    vchiq_check_local_events();
}

static int vchiq_parse_msg_openack(struct vchiq_state_struct *s, int localport, int remoteport)
{
  struct vchiq_service_common *service = NULL;
  // printf("vchiq_parse_msg_openack\r\n");

  service = vchiq_service_find_by_localport(localport);
  BUG(!service, "OPENACK with no service");
  service->opened = true;
  service->remoteport = remoteport;
  wmb();
  wakeup_waitflag(&s->state_waitflag);
  return ERR_OK;
}

static int vchiq_parse_msg_data(struct vchiq_state_struct *s, int localport, int remoteport,
  struct vchiq_header_struct *h)
{
  struct vchiq_service_common *service = NULL;
  // MMAL_DEBUG("data");

  service = vchiq_service_find_by_localport(localport);
  BUG(!service, "MSG_DATA with no service");
  service->data_callback(service, h);
  return ERR_OK;
}

static void vchiq_release_slot(struct vchiq_state_struct *s, int slot_index)
{
  int slot_queue_recycle;

  slot_queue_recycle = s->remote->slot_queue_recycle;
  rmb();
  s->remote->slot_queue[slot_queue_recycle & VCHIQ_SLOT_QUEUE_MASK] = slot_index;
  s->remote->slot_queue_recycle = slot_queue_recycle + 1;
  vchiq_event_signal(&s->remote->recycle);
}

static int vchiq_tx_header_to_slot_idx(struct vchiq_state_struct *s)
{
  return s->remote->slot_queue[(s->rx_pos / VCHIQ_SLOT_SIZE) & VCHIQ_SLOT_QUEUE_MASK];
}

static inline int vchiq_parse_rx_dispatch(struct vchiq_state_struct *s, struct vchiq_header_struct *h)
{
  int err;
  int msg_type, localport, remoteport;
  msg_type = VCHIQ_MSG_TYPE(h->msgid);
  localport = VCHIQ_MSG_DSTPORT(h->msgid);
  remoteport = VCHIQ_MSG_SRCPORT(h->msgid);
  switch(msg_type) {
    case VCHIQ_MSG_CONNECT:
      s->conn_state = VCHIQ_CONNSTATE_CONNECTED;
      wakeup_waitflag(&s->state_waitflag);
      break;
    case VCHIQ_MSG_OPENACK:
      err = vchiq_parse_msg_openack(s, localport, remoteport);
      break;
    case VCHIQ_MSG_DATA:
      err = vchiq_parse_msg_data(s, localport, remoteport, h);
      break;
    default:
      err = ERR_INVAL_ARG;
      break;
  }
  return err;
}

static int vchiq_parse_rx(struct vchiq_state_struct *s)
{
  int err = ERR_OK;
  int rx_slot;
  struct vchiq_header_struct *h;
  int prev_rx_slot = vchiq_tx_header_to_slot_idx(s);

  while(s->rx_pos != s->remote->tx_pos) {
    int old_rx_pos = s->rx_pos;
    h = vchiq_get_next_header_rx(s);
    MMAL_INFO("msg received: %p, rx_pos: %d, size: %d", h, old_rx_pos, vchiq_calc_stride(h->size));
    err = vchiq_parse_rx_dispatch(s, h);
    CHECK_ERR("failed to parse message from remote");

    rx_slot = vchiq_tx_header_to_slot_idx(s);
//    rx_slot = s->rx_pos / VCHIQ_SLOT_SIZE;
//  if ((s->rx_pos & VCHIQ_SLOT_MASK) == 0) {
    if (rx_slot != prev_rx_slot) {
      vchiq_release_slot(s, prev_rx_slot);
      prev_rx_slot = rx_slot;
    }
  }
  return ERR_OK;
out_err:
  return err;
}

/*
 * Called in the context of recycle thread when remote (VC) is
 * done with some particular slot of our messages and signals
 * recylce event to us, so that we can reuse these slots.
 *
 * In original code there is a lot of bookkeeping in this func
 * but it seems it's not needed critically right now.
 */
static void vchiq_process_free_queue(struct vchiq_state_struct *s)
{
  int slot_queue_available, pos, slot_index;
  char *slot_data;
  struct vchiq_header_struct *h;

  slot_queue_available = s->slot_queue_available;

  mb();

  while (slot_queue_available != s->local->slot_queue_recycle) {
    slot_index = s->local->slot_queue[slot_queue_available & VCHIQ_SLOT_QUEUE_MASK];
    slot_queue_available++;
    slot_data = (char *)&s->slot_data[slot_index];

    rmb();


    pos = 0;
    while (pos < VCHIQ_SLOT_SIZE) {
      h = (struct vchiq_header_struct *)(slot_data + pos);

      pos += vchiq_calc_stride(h->size);
      BUG(pos > VCHIQ_SLOT_SIZE, "some");
//        BUG(1, "invalid slot position");
    }

    mb();

    s->slot_queue_available = slot_queue_available;
    semaphore_up(&s->slot_available_event);
  }
}


static int vchiq_recycle_thread(void)
{
  struct vchiq_state_struct *s;

  s = &vchiq_state;
  while (1) {
    // puts("recycle_wait\r\n");
    vchiq_event_wait(&s->recycle_waitflag, &s->local->recycle);
    // puts("recycle_wakeup\r\n");
    vchiq_process_free_queue(s);
    yield();
  }
  return ERR_OK;
}

static int vchiq_loop_thread(void)
{
  struct vchiq_state_struct *s;

  s = &vchiq_state;
  while(1) {
    // puts("loop wait\r\n");
    vchiq_event_wait(&s->trigger_waitflag, &s->local->trigger);
    // puts("loop wakeup\r\n");
    BUG(vchiq_parse_rx(s) != ERR_OK,
      "failed to parse incoming vchiq messages");
  }
  return ERR_OK;
}

static int vchiq_start_thread(struct vchiq_state_struct *s)
{
  int err;
  struct task *t;

  INIT_LIST_HEAD(&mmal_io_work_list);
  waitflag_init(&mmal_io_work_waitflag);
  spinlock_init(&mmal_io_work_list_lock);

  t = task_create(vchiq_io_thread, "vchiq_io_thread");
  CHECK_ERR_PTR(t, "Failed to start vchiq_thread");
  sched_queue_runnable_task(get_scheduler(), t);

  irq_set(0, INTR_CTL_IRQ_GPU_DOORBELL_0, vchiq_irq_handler);
  intr_ctl_arm_irq_enable(INTR_CTL_IRQ_ARM_DOORBELL_0);
  t = task_create(vchiq_loop_thread, "vchiq_loop_thread");
  CHECK_ERR_PTR(t, "Failed to start vchiq_thread");
  sched_queue_runnable_task(get_scheduler(), t);
  yield();

  t = task_create(vchiq_recycle_thread, "vchiq_recycle_thread");
  CHECK_ERR_PTR(t, "Failed to start vchiq_thread");
  sched_queue_runnable_task(get_scheduler(), t);


  return ERR_OK;

out_err:
  intr_ctl_arm_irq_disable(INTR_CTL_IRQ_ARM_DOORBELL_0);
  irq_set(0, INTR_CTL_IRQ_ARM_DOORBELL_0, 0);
  return err;
}

void vchiq_handmade(struct vchiq_state_struct *s, struct vchiq_slot_zero_struct *z)
{
  int err;
  struct vchiq_service_common *smem_service, *mmal_service;

  err = vchiq_start_thread(s);
  CHECK_ERR("failed to start vchiq async primitives");
  wait_on_waitflag(&s->state_waitflag);
  err = vchiq_handmade_connect(s);

  mmal_service = vchiq_open_mmal_service(s);
  CHECK_ERR_PTR(mmal_service, "failed to open mmal service");

  smem_service = vchiq_open_smem_service(s);
  CHECK_ERR_PTR(smem_service, "failed at open smem service");

  err = vchiq_camera_run(mmal_service, smem_service, 320, 240);
  CHECK_ERR("failed to run camera");

out_err:
  while(1) asm volatile("wfe");
}

void vchiq_prepare_fragments(char *fragments_base, uint32_t fragment_size)
{
  int i;
//  char **f_curr;
//  char **f_next;
//  for (i = 0; i < (MAX_FRAGMENTS - 1); i++) {
//    f_curr = &fragments_base[(i + 0) * fragment_size];
//    f_next = &fragments_base[(i + 1) * fragment_size];
//    *f_curr = f_next;
//  }
//  *f_next = NULL;
  for (i = 0; i < (MAX_FRAGMENTS - 1); i++) {
    *(char **)&fragments_base[i*fragment_size] = &fragments_base[(i + 1) * fragment_size];
          }
    *(char **)&fragments_base[i * fragment_size] = NULL;

}

static inline void
remote_event_create(REMOTE_EVENT_T *event)
{
  event->armed = 0;
  event->event = (uint32_t)(uint64_t)event;
}

static inline void
remote_event_signal_local(REMOTE_EVENT_T *event)
{
  event->armed = 0;
  // semaphore_up((struct semaphore *)(uint64_t)event->event);
}


static void vchiq_init_state(VCHIQ_STATE_T *state, VCHIQ_SLOT_ZERO_T *slot_zero)
{
  VCHIQ_SHARED_STATE_T *local;
  VCHIQ_SHARED_STATE_T *remote;
  static int id;
  int i;

  /* Check the input configuration */
  local = &slot_zero->slave;
  remote = &slot_zero->master;

  memset(state, 0, sizeof(VCHIQ_STATE_T));

  state->id = id++;
  state->is_master = 0;

  /* initialize shared state pointers */
  state->local = local;
  state->remote = remote;
  state->slot_data = (VCHIQ_SLOT_T *)slot_zero;

  /* initialize events and mutexes */
  semaphore_init(&state->connect, 0);
  mutex_init(&state->mutex);

  waitflag_init(&state->trigger_waitflag);
  waitflag_init(&state->recycle_waitflag);
  waitflag_init(&state->sync_trigger_waitflag);
  waitflag_init(&state->sync_release_waitflag);
  waitflag_init(&state->state_waitflag);

  mutex_init(&state->slot_mutex);
  mutex_init(&state->recycle_mutex);
  mutex_init(&state->sync_mutex);
  mutex_init(&state->bulk_transfer_mutex);

  semaphore_init(&state->slot_available_event, 0);
  semaphore_init(&state->slot_remove_event, 0);
  semaphore_init(&state->data_quota_event, 0);

  state->slot_queue_available = 0;

	for (i = 0; i < VCHIQ_MAX_SERVICES; i++) {
		VCHIQ_SERVICE_QUOTA_T *service_quota =
			&state->service_quotas[i];
		semaphore_init(&service_quota->quota_event, 0);
	}

	for (i = local->slot_first; i <= local->slot_last; i++) {
		local->slot_queue[state->slot_queue_available++] = i;
		// semaphore_up(&state->slot_available_event);
	}

	state->default_slot_quota = state->slot_queue_available/2;
	state->default_message_quota =
		min((unsigned short)(state->default_slot_quota * 256),
		(unsigned short)~0);

	state->previous_data_index = -1;
	state->data_use_count = 0;
	state->data_quota = state->slot_queue_available - 1;

	// local->trigger.event = (unsigned)(uint64_t)&state->trigger_event;
	remote_event_create(&local->trigger);
	local->tx_pos = 0;

	// local->recycle.event = (unsigned)(uint64_t)&state->recycle_event;
	remote_event_create(&local->recycle);
	local->slot_queue_recycle = state->slot_queue_available;

	// local->sync_trigger.event = (unsigned)(uint64_t)&state->sync_trigger_event;
	remote_event_create(&local->sync_trigger);

	// local->sync_release.event = (unsigned)(uint64_t)&state->sync_release_event;
	remote_event_create(&local->sync_release);

	/* At start-of-day, the slot is empty and available */
	((VCHIQ_HEADER_T *)SLOT_DATA_FROM_INDEX(state, local->slot_sync))->msgid
		= VCHIQ_MSGID_PADDING;
	remote_event_signal_local(&local->sync_release);

	local->debug[DEBUG_ENTRIES] = DEBUG_MAX;

	BUG(state->id >= VCHIQ_MAX_STATES, "fff");
	vchiq_states[state->id] = state;

  /* Indicate readiness to the other side */
  local->initialised = 1;
}

int vchiq_platform_init(void)
{
  VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
  void *slot_mem;
  uint32_t slot_phys;
  uint32_t channelbase;
  int slot_mem_size, frag_mem_size;
  int err;
  uint32_t fragment_size;
  struct vchiq_state_struct *s = &vchiq_state;

  fragment_size = 2 * 64; /* g_cache_line_size */
  /* Allocate space for the channels in coherent memory */
  slot_mem_size = 80 /* TOTAL_SLOTS */ * VCHIQ_SLOT_SIZE;
  frag_mem_size = fragment_size * MAX_FRAGMENTS;

  slot_mem = dma_alloc(slot_mem_size + frag_mem_size);
  slot_phys = (uint32_t)(uint64_t)slot_mem;
  if (!slot_mem) {
    printf("failed to allocate DMA memory");
   return ERR_MEMALLOC;
  }

  vchiq_slot_zero = vchiq_init_slots(slot_mem, slot_mem_size);
  if (!vchiq_slot_zero)
    return ERR_INVAL_ARG;

  vchiq_init_state(s, vchiq_slot_zero);

  vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX] = ((uint32_t)slot_phys) + slot_mem_size;
  vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX] = MAX_FRAGMENTS;
  vchiq_prepare_fragments((char *)slot_mem + slot_mem_size, fragment_size);

  wmb();
	/* Send the base address of the slots to VideoCore */
	channelbase = 0xc0000000 | slot_phys;

  err = mbox_init_vchiq(&channelbase);
	if (err || channelbase) {
	  LOG(0, ERR, "vchiq", "failed to set channelbase");
		return err ? : ERR_GENERIC;
	}

  wmb();
  rmb();

  vchiq_log_info(vchiq_arm_log_level, "vchiq_init - done (slots %x, phys %pad)", (unsigned int)(uint64_t)(uint32_t *)vchiq_slot_zero, &slot_phys);

  s->conn_state = VCHIQ_CONNSTATE_DISCONNECTED;

  vchiq_handmade(s, vchiq_slot_zero);

  return ERR_OK;
}

VCHIQ_ARM_STATE_T*
vchiq_platform_get_arm_state(VCHIQ_STATE_T *state)
{
   if(!((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited)
   {
      BUG(true, "platform not initialized");
   }
   return &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state;
}

void
remote_event_signal(REMOTE_EVENT_T *event)
{
  wmb();
  event->fired = 1;
  /* data barrier operation */
  dsb();
  if (event->armed)
    vchiq_ring_bell();
}
