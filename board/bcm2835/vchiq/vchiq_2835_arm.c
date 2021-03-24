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

#define TOTAL_SLOTS (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)
#define MAX_FRAGMENTS (VCHIQ_NUM_CURRENT_BULKS * 2)

#define BELL0	0x00
#define BELL2	0x08

#define VCHIQ_ARM_ADDRESS(x) ((void *)((char *)x + RAM_BASE_BUS_UNCACHED))
#define VCHIQ_MMAL_MAX_COMPONENTS 64
typedef unsigned int VCHI_SERVICE_HANDLE_T;
extern void port_to_mmal_msg(struct vchiq_mmal_port *port, struct mmal_port *p);

typedef uint32_t irqreturn_t;
reg32_t g_regs;
int mmal_log_level = LOG_LEVEL_DEBUG3;

typedef struct vchiq_2835_state_struct {
   int inited;
   VCHIQ_ARM_STATE_T arm_state;
} VCHIQ_2835_ARM_STATE_T;

extern int vchiq_arm_log_level;

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id);

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

void do_stuff()
{
  struct vchiq_mmal_instance *instance;
  vchiq_mmal_init(&instance);
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

static inline void *vchiq_check_reply_msg(struct vchiq_header_struct *h, int msg_type)
{
  if (VCHIQ_MSG_TYPE(h->msgid) != msg_type) {
    MMAL_ERR("expected type %s, received: %s", msg_type_str(msg_type), msg_type_str(h->msgid));
    return NULL;
  }
  return (struct mmal_msg *)h->data;
}

static inline void *vchiq_mmal_check_reply_msg(struct vchiq_header_struct *h, int msg_type, int mmal_msg_type)
{
  struct mmal_msg *rmsg;

  rmsg = vchiq_check_reply_msg(h, msg_type);
  if (!rmsg)
    return NULL;

  if (rmsg->h.type != mmal_msg_type) {
    MMAL_ERR("mmal msg expected type %s, received: %s",
      mmal_msg_type_names[mmal_msg_type],
      mmal_msg_type_names[rmsg->h.type]);
    return NULL;
  }
  return &rmsg->u;
}

static inline int vchiq_calc_stride(int size)
{
  size += sizeof(struct vchiq_header_struct);
  return (size + sizeof(struct vchiq_header_struct) - 1) & ~(sizeof(struct vchiq_header_struct) - 1);
}

struct vchiq_header_struct *vchiq_prep_next_header_tx(struct vchiq_state_struct *s, int msg_size)
{
  int tx_pos, slot_queue_index, slot_index;
  struct vchiq_header_struct *h;
  int slot_space;

  /* Recall last position for tx */
  tx_pos = s->local_tx_pos;

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
  if (slot_space < msg_size) {
    h = (struct vchiq_header_struct *)s->tx_data + (tx_pos & VCHIQ_SLOT_MASK);
    h->msgid = VCHIQ_MSGID_PADDING;
    h->size = slot_space - sizeof(*h);
    s->local_tx_pos += slot_space;
    tx_pos += slot_space;
  }

  slot_queue_index = ((int)((unsigned int)(tx_pos) / VCHIQ_SLOT_SIZE));
  slot_index = s->local->slot_queue[slot_queue_index & VCHIQ_SLOT_QUEUE_MASK];

  s->tx_data = (char*)&s->slot_data[slot_index];
  s->local_tx_pos += vchiq_calc_stride(msg_size);
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
  if (h->msgid == VCHIQ_MSGID_PADDING) {
    state->rx_pos += vchiq_calc_stride(h->size);
    goto next_header;
  }
  state->rx_pos = state->remote->tx_pos;
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

void vchiq_event_wait(struct remote_event_struct *event)
{
  if (!event->fired) {
    event->armed = 1;
    dsb();
    while (!event->fired);
    event->armed = 0;
    wmb();
  }
  event->fired = 0;
  rmb();
}

int vchiq_handmade_connect(struct vchiq_state_struct *s)
{
  struct vchiq_shared_state_struct *lstate, *rstate;
  struct vchiq_header_struct *header;
  int msg_size;

  // rx_pos = s->rx_pos;
  lstate = s->local;
  rstate = s->remote;

  header = vchiq_get_next_header_rx(s);
  if (VCHIQ_MSG_TYPE(header->msgid) != VCHIQ_MSG_CONNECT) {
    MMAL_ERR("Expected msg type CONNECT from remote");
    return ERR_GENERIC;
  }

  /* Send CONNECT MESSAGE */
  msg_size = 0;
  header = vchiq_prep_next_header_tx(s, msg_size);

  header->msgid = VCHIQ_MAKE_MSG(VCHIQ_MSG_CONNECT, 0, 0);
  header->size = msg_size;

  s->conn_state = VCHIQ_CONNSTATE_CONNECTING;
  wmb();
  /* Make the new tx_pos visible to the peer. */
  lstate->tx_pos = s->local_tx_pos;
  wmb();

  vchiq_event_signal(&rstate->trigger);
  lstate->recycle.armed = 1;

  /* Wait CONNECTED RESPONSE */
  if (!lstate->trigger.fired) {
    lstate->trigger.armed = 1;
    dsb();
    while (!lstate->trigger.fired);
    lstate->trigger.armed = 0;
    wmb();
  }
  lstate->trigger.fired = 0;
  rmb();

  s->conn_state = VCHIQ_CONNSTATE_CONNECTED;
  return ERR_OK;
}

#define VC_MMAL_VER 15
#define VC_MMAL_MIN_VER 10

void vchiq_handmade_prep_msg(struct vchiq_state_struct *s, int msgid, int srcport, int dstport, void *payload, int payload_sz)
{
  struct vchiq_header_struct *h;

  h = vchiq_prep_next_header_tx(s, payload_sz);

  h->msgid = VCHIQ_MAKE_MSG(msgid, srcport, dstport);
  memcpy(h->data, payload, payload_sz);
}

#define VC_SM_VER  1
#define VC_SM_MIN_VER 0

struct vchiq_service_common *vchiq_handmade_open_sm_cma_service(struct vchiq_state_struct *s)
{
  /* Service open payload */
  int msg_id;
  struct vchiq_open_payload open_payload;
  struct vchiq_openack_payload *openack;
  struct vchiq_header_struct *h;
  struct vchiq_service_common *ms;
  ms = kzalloc(sizeof(*ms), GFP_KERNEL);

  open_payload.fourcc = MAKE_FOURCC("SMEM");
  open_payload.client_id = 0;
  open_payload.version = VC_SM_VER;
  open_payload.version_min = VC_SM_MIN_VER;

  vchiq_handmade_prep_msg(s, VCHIQ_MSG_OPEN, 0, 0, &open_payload, sizeof(open_payload));
  vchiq_event_signal(&s->remote->trigger);
  vchiq_event_wait(&s->local->trigger);

  h = vchiq_get_next_header_rx(s);
  if (VCHIQ_MSG_TYPE(h->msgid) != VCHIQ_MSG_OPENACK) {
    MMAL_ERR("Expected msg type VCHIQ_MSG_OPENACK from remote");
    return ERR_PTR(ERR_GENERIC);
  }
  msg_id = h->msgid;
  openack = (struct vchiq_openack_payload *)(h->data);
  ms->localport = VCHIQ_MSG_DSTPORT(msg_id);
  ms->remoteport = VCHIQ_MSG_SRCPORT(msg_id);
  ms->s = s;
  MMAL_INFO("OPENACK: msgid: %08x, localport: %d, remoteport: %d, version: %d",
    msg_id,
    ms->localport,
    ms->remoteport,
    openack->version);
  return ms;
}

int vchiq_handmade_open_mmal(struct vchiq_state_struct *s, struct vchiq_service_common *ms)
{
  /* Service open payload */
  int msg_id;
  struct vchiq_open_payload open_payload;
  struct vchiq_openack_payload *openack;
  struct vchiq_header_struct *h;

  /* Open "mmal" service */
  open_payload.fourcc = MAKE_FOURCC("mmal");
  open_payload.client_id = 0;
  open_payload.version = VC_MMAL_VER;
  open_payload.version_min = VC_MMAL_MIN_VER;

  vchiq_handmade_prep_msg(s, VCHIQ_MSG_OPEN, 0, 0, &open_payload, sizeof(open_payload));
  vchiq_event_signal(&s->remote->trigger);
  vchiq_event_wait(&s->local->trigger);

  h = vchiq_get_next_header_rx(s);
  if (VCHIQ_MSG_TYPE(h->msgid) != VCHIQ_MSG_OPENACK) {
    MMAL_ERR("Expected msg type VCHIQ_MSG_OPENACK from remote");
    return ERR_GENERIC;
  }
  msg_id = h->msgid;
  openack = (struct vchiq_openack_payload *)(h->data);
  ms->localport = VCHIQ_MSG_DSTPORT(msg_id);
  ms->remoteport = VCHIQ_MSG_SRCPORT(msg_id);
  MMAL_INFO("OPENACK: msgid: %08x, localport: %d, remoteport: %d, version: %d",
    msg_id,
    ms->localport,
    ms->remoteport,
    openack->version);
  return ERR_OK;
}

void vchiq_mmal_fill_header(struct vchiq_service_common *ms, int mmal_msg_type, struct mmal_msg *msg)
{
  msg->h.magic = MMAL_MAGIC;
  msg->h.context = 0x5a5a5a5a;
  msg->h.control_service = 0x6a6a6a6a;
  msg->h.status = 0;
  msg->h.padding = 0;
  msg->h.type = mmal_msg_type;
}

#define VCHIQ_MMAL_MSG_DECL(__ms, __msg_type, __mmal_msg_type, __msg_u, __msg_u_reply) \
  struct mmal_msg msg; \
  struct vchiq_header_struct *_h; \
  struct vchiq_service_common *_ms = __ms; \
  const int msg_type = VCHIQ_MSG_ ## __msg_type; \
  const int mmal_msg_type = MMAL_MSG_TYPE_ ## __mmal_msg_type; \
  struct mmal_msg_ ## __msg_u *m = &msg.u. __msg_u; \
  struct mmal_msg_ ## __msg_u_reply *r; \
  memset(&msg, 0, sizeof(msg));

#define VCHIQ_MMAL_MSG_COMMUNICATE_ASYNC() \
  vchiq_mmal_fill_header(_ms, mmal_msg_type, &msg); \
  vchiq_handmade_prep_msg(_ms->s, msg_type, _ms->localport, _ms->remoteport, &msg, \
    sizeof(struct mmal_msg_header) + sizeof(*m)); \
  vchiq_event_signal(&_ms->s->remote->trigger) \

#define VCHIQ_MMAL_MSG_COMMUNICATE_SYNC() \
  VCHIQ_MMAL_MSG_COMMUNICATE_ASYNC(); \
  vchiq_event_wait(&_ms->s->local->trigger); \
  _h = vchiq_get_next_header_rx(_ms->s); \
  r = vchiq_mmal_check_reply_msg(_h, msg_type, mmal_msg_type); \
  if (!r) { \
    MMAL_ERR("invalid reply"); \
    return ERR_GENERIC; \
  } \
  if (r->status != MMAL_MSG_STATUS_SUCCESS) { \
    MMAL_ERR("status not success: %d", r->status); \
    return ERR_GENERIC; \
  }

int vchiq_mmal_handmade_component_disable(struct vchiq_mmal_component *c)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, COMPONENT_DISABLE, component_disable,
    component_disable_reply);

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
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, COMPONENT_DESTROY, component_destroy,
    component_destroy_reply);

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
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_INFO_GET, port_info_get, port_info_get_reply);

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

int mmal_component_create(struct vchiq_service_common *ms, const char *name, int component_idx, struct vchiq_mmal_component **out_component)
{
  struct vchiq_mmal_component *c;
  int i;
  VCHIQ_MMAL_MSG_DECL(ms, DATA, COMPONENT_CREATE, component_create, component_create_reply);

  c = kzalloc(sizeof(*c), GFP_KERNEL);
  if (IS_ERR(c)) {
    MMAL_ERR("failed to allocate memory for mmal_component");
    return PTR_ERR(c);
  }

  m->client_component = component_idx;
  strncpy(m->name, name, sizeof(m->name));

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();

  c->handle = r->component_handle;
  c->ms = ms;
  c->enabled = true;
  c->inputs = r->input_num;
  c->outputs = r->output_num;
  c->clocks = r->clock_num;
  strncpy(c->name, name, sizeof(c->name));

  MMAL_INFO("vchiq component created name:%s: status:%d, handle: %d, input: %d, output: %d, clock: %d",
    c->name, r->status, c->handle, c->inputs, c->outputs, c->clocks);

  mmal_port_create(c, &c->control, MMAL_PORT_TYPE_CONTROL, 0);

  for (i = 0; i < c->inputs; ++i)
    mmal_port_create(c, &c->input[i], MMAL_PORT_TYPE_INPUT, i);

  for (i = 0; i < c->outputs; ++i)
    mmal_port_create(c, &c->output[i], MMAL_PORT_TYPE_OUTPUT, i);

  *out_component = c;
  return ERR_OK;
}

struct vchiq_mmal_component *vchiq_mmal_create_camera_info(struct vchiq_service_common *ms)
{
  int err;
  struct vchiq_mmal_component *c;
  err = mmal_component_create(ms, "camera_info", 0, &c);
  if (err != ERR_OK)
    return ERR_PTR(err);
  return c;
}

int vchiq_mmal_port_info_set(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_INFO_SET, port_info_set, port_info_set_reply);

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
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_PARAMETER_SET, port_parameter_set, port_parameter_set_reply);

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
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_PARAMETER_GET, port_parameter_get, port_parameter_get_reply);

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
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_ACTION, port_action_port, port_action_reply);

  m->component_handle = c->handle;
  m->port_handle = p->handle;
  m->action = action;
  port_to_mmal_msg(p, &m->port);

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_mmal_port_action_handle(struct vchiq_mmal_component *c, struct vchiq_mmal_port *p, int action, int dst_component_handle, int dst_port_handle)
{
  VCHIQ_MMAL_MSG_DECL(c->ms, DATA, PORT_ACTION, port_action_handle, port_action_reply);

  m->component_handle = c->handle;
  m->port_handle = p->handle;
  m->action = action;
  m->connect_component_handle = dst_component_handle;
  m->connect_port_handle = dst_port_handle;

  VCHIQ_MMAL_MSG_COMMUNICATE_SYNC();
  return ERR_OK;
}

int vchiq_bulk_rx(struct vchiq_mmal_component *c, uint32_t bufaddr, uint32_t bufsize)
{
  /* Service open payload */
  uint32_t payload[2];
  struct vchiq_header_struct *h;

  payload[0] = bufaddr;
  payload[1] = bufsize;

  vchiq_handmade_prep_msg(c->ms->s, VCHIQ_MSG_BULK_RX, c->ms->localport, c->ms->remoteport, payload, sizeof(payload));
  vchiq_event_signal(&c->ms->s->remote->trigger);
  vchiq_event_wait(&c->ms->s->local->trigger);

  h = vchiq_get_next_header_rx(c->ms->s);
  if (VCHIQ_MSG_TYPE(h->msgid) != VCHIQ_MSG_BULK_RX_DONE) {
    MMAL_ERR("Expected msg type VCHIQ_MSG_BULK_RX_DONE from remote");
    return ERR_GENERIC;
  }
  return ERR_OK;
}

static inline void mmal_buffer_header_print_flags(struct mmal_buffer_header *h)
{
  char buf[256];
  int n = 0;

#define CHECK_FLAG(__name) \
  if (h->flags & MMAL_BUFFER_HEADER_FLAG_ ## __name) { \
    if (n != 0 && (sizeof(buf) - n >= 2)) { \
      buf[n++] = ','; \
      buf[n++] = ' '; \
    } \
    strncpy(buf + n, #__name, min(sizeof(#__name), sizeof(buf) - n));\
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
  MMAL_INFO("buffer_header: %s", buf);
}

int vchiq_mmal_buffer_to_host(struct vchiq_mmal_component *c)
{
  struct vchiq_header_struct *h;
  struct mmal_msg_buffer_from_host *r;

  vchiq_event_wait(&c->ms->s->local->trigger);
  h = vchiq_get_next_header_rx(c->ms->s);
  r = vchiq_mmal_check_reply_msg(h, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_BUFFER_TO_HOST);
  if (!r) {
    MMAL_ERR("invalid reply");
    return ERR_GENERIC;
  }
  mmal_buffer_header_print_flags(&r->buffer_header);
  return ERR_OK;
}

int vchiq_mmal_buffer_from_host(struct vchiq_mmal_port *p, struct mmal_buffer *b)
{
  int err;
  VCHIQ_MMAL_MSG_DECL(p->component->ms, DATA, BUFFER_FROM_HOST, buffer_from_host, buffer_from_host);

  memset(m, 0xbc, sizeof(*m));

  m->drvbuf.magic = MMAL_MAGIC;
  m->drvbuf.component_handle = p->component->handle;
  m->drvbuf.port_handle = p->handle;
  m->drvbuf.client_context = 0x7e7e7e7e;

  m->is_zero_copy = p->zero_copy;
  m->buffer_header.next = 0;
  m->buffer_header.priv = 0xffaaffaa;
  m->buffer_header.cmd = 0;
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
  wait_msec(100);
  if (!_ms->s->local->trigger.fired)
    return ERR_OK;
  return ERR_OK;
  vchiq_event_wait(&_ms->s->local->trigger);
  _h = vchiq_get_next_header_rx(_ms->s);
  r = vchiq_mmal_check_reply_msg(_h, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_BUFFER_TO_HOST);
  if (!r) {
    MMAL_ERR("invalid reply");
    return ERR_GENERIC;
  }
  if (r->buffer_header.length == 0 || r->payload_in_message) {
    kernel_panic("if (r->length == 0 || r->payload_in_message)");
  }
  if (!r->is_zero_copy)
    err = vchiq_bulk_rx(p->component, (uint32_t)(uint64_t)b->buffer | 0xc0000000, b->buffer_size);
  return err;
}

int vchiq_mmal_component_enable(struct vchiq_mmal_component *c)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_component_enable_reply *r;

  msg.u.component_enable.component_handle = c->handle;

  /* send MMAL message synchronous */
  vchiq_mmal_fill_header(c->ms, MMAL_MSG_TYPE_COMPONENT_ENABLE, &msg);
  vchiq_handmade_prep_msg(c->ms->s, VCHIQ_MSG_DATA, c->ms->localport, c->ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_action_port));
  vchiq_event_signal(&c->ms->s->remote->trigger);
  vchiq_event_wait(&c->ms->s->local->trigger);
  header = vchiq_get_next_header_rx(c->ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_COMPONENT_ENABLE);
  if (!r) {
    MMAL_ERR("invalid reply");
    return -1;
  }
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    MMAL_ERR("status not success: %d reply", r->status);
    return ERR_GENERIC;
  }
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

static int vc_sm_cma_vchi_send_msg(struct vchiq_service_common *ms,
  enum vc_sm_msg_type msg_id, void *msg,
  uint32_t msg_size, void *result, uint32_t result_size,
  uint32_t *cur_trans_id, uint8_t wait_reply)
{
  struct vchiq_header_struct *h;
  char buf[256];
  struct vc_sm_msg_hdr_t *hdr = (struct vc_sm_msg_hdr_t *)buf;
  // struct sm_cmd_rsp_blk *cmd;
  hdr->type = msg_id;
  hdr->trans_id = vc_trans_id++;

  if (msg_size)
    memcpy(hdr->body, msg, msg_size);

  // cmd = vc_vchi_cmd_create(msg_id, msg, msg_size, 1);
  // cmd->sent = 1;

  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, hdr, msg_size + sizeof(*hdr));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);

  h = vchiq_get_next_header_rx(ms->s);
  if (VCHIQ_MSG_TYPE(h->msgid) != VCHIQ_MSG_DATA) {
    MMAL_ERR("Expected msg type VCHIQ_MSG_OPENACK from remote");
    return ERR_GENERIC;
  }
  memcpy(result, h->data, result_size);
  return ERR_OK;
}

int vc_sm_cma_vchi_import(struct vchiq_service_common *ms, struct vc_sm_import *msg, struct vc_sm_import_result *result, uint32_t *cur_trans_id)
{
  return vc_sm_cma_vchi_send_msg(ms, VC_SM_MSG_TYPE_IMPORT, msg, sizeof(*msg), result, sizeof(*result), cur_trans_id, 1);
}

int vc_sm_cma_import_dmabuf(struct vchiq_service_common *ms, struct mmal_buffer *b, void **vcsm_handle)
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
  err = vc_sm_cma_vchi_import(ms, &import, &result, &cur_trans_id);
  CHECK_ERR("Failed to import buffer to vc");
  MMAL_INFO("imported_dmabuf: addr:%08x, size: %d, trans_id: %08x, res.trans_id: %08x, res.handle: %08x",
    import.addr, import.size, cur_trans_id, result.trans_id, result.res_handle);

  *vcsm_handle = (void*)(uint64_t)result.res_handle;

  return ERR_OK;

out_err:
  return err;
}

int mmal_alloc_port_buffers(struct vchiq_service_common *mem_service, struct vchiq_mmal_port *p)
{
  int err;
  int i;
  struct mmal_buffer *buf;
  for (i = 0; i < p->minimum_buffer.num; ++i) {
    buf = kzalloc(sizeof(*buf), GFP_KERNEL);
    buf->buffer_size = p->minimum_buffer.size;
    buf->buffer = dma_alloc(buf->buffer_size);;
    MMAL_INFO("mmal_alloc_port_buffers: min_num: %d, min_sz:%d, min_al:%d, port_enabled:%s, buf:%08x",
      p->minimum_buffer.num,
      p->minimum_buffer.size,
      p->minimum_buffer.alignment,
      p->enabled ? "yes" : "no",
      buf->buffer);

    if (p->zero_copy) {
      err = vc_sm_cma_import_dmabuf(mem_service, buf, &buf->vcsm_handle);
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

static int mmal_camera_capture_frames(struct vchiq_mmal_component *cam, struct vchiq_mmal_port *capture_port, uint32_t num_frames)
{
  uint32_t frame_count = num_frames;
  return vchiq_mmal_port_parameter_set(cam, capture_port, MMAL_PARAMETER_CAPTURE, &frame_count, sizeof(frame_count));
}

static int mmal_port_buffer_send(struct vchiq_mmal_port *p)
{
  int err;
  struct mmal_buffer *b;

  if (list_empty(&p->buffers)) {
    MMAL_ERR("port buffer list is empty");
    return ERR_NO_RESOURCE;
  }

  b = list_first_entry(&p->buffers, typeof(*b), list);
  err = vchiq_mmal_buffer_from_host(p, b);
  if (err) {
    list_add_tail(&b->list, &p->buffers);
    MMAL_ERR("failed to submit port buffer to VC");
    return err;
  }

  return ERR_OK;
}

static int mmal_port_buffer_receive(struct vchiq_mmal_port *p)
{
  return vchiq_mmal_buffer_to_host(p->component);
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

int vchiq_camera_run(struct vchiq_service_common *ms, struct vchiq_service_common *mem_service, int frame_width, int frame_height)
{
  int err;
  struct mmal_parameter_camera_info_t cam_info = {0};
  struct vchiq_mmal_component *cam;
  struct vchiq_mmal_port *still_port, *video_port, *preview_port;
  uint32_t supported_encodings[MAX_SUPPORTED_ENCODINGS];
  int num_encodings;

  err = vchiq_mmal_get_cam_info(ms, &cam_info);
  CHECK_ERR("Failed to get num cameras");

  /* mmal_init start */
  err = mmal_component_create(ms, "ril.camera", 0, &cam);
  CHECK_ERR("Failed to create component 'ril.camera'");

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

  err = mmal_alloc_port_buffers(mem_service, still_port);
  CHECK_ERR("Failed to prepare buffer for still port");

  while(1) {
    err = mmal_port_buffer_send(still_port);
    CHECK_ERR("Failed to send buffer to port");
    mmal_camera_capture_frames(cam, still_port, 20);
    CHECK_ERR("Failed to initiate frame capture");
    err = mmal_port_buffer_receive(still_port);
    CHECK_ERR("Failed to receive buffer from VC");
  }
  return ERR_OK;

out_err:
  return err;
}

struct vchiq_service_common *vchiq_mmal_service_run(struct vchiq_state_struct *s)
{
  int err;
  struct vchiq_service_common *ms;
  ms = kzalloc(sizeof(*ms), GFP_KERNEL);

  ms->s = s;
  s->rx_pos = 0;

  s->conn_state = VCHIQ_CONNSTATE_DISCONNECTED;

  err = vchiq_handmade_connect(s);
  CHECK_ERR("failed at connection step");
  err =  vchiq_handmade_open_mmal(s, ms);
  CHECK_ERR("failed at open mmal step");

  return ms;

out_err:
  return ERR_PTR(err);
}

void vchiq_handmade(struct vchiq_state_struct *s, struct vchiq_slot_zero_struct *z)
{
  int err;
  struct vchiq_service_common *ms, *mem_service;

  ms = vchiq_mmal_service_run(s);
  CHECK_ERR_PTR(ms, "failed to run mmal service");

  mem_service =  vchiq_handmade_open_sm_cma_service(s);
  CHECK_ERR_PTR(mem_service, "failed at open mems service");

  err = vchiq_camera_run(ms, mem_service, 160, 120);
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

int vchiq_platform_init(VCHIQ_STATE_T *state)
{
  VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
  void *slot_mem;
  uint32_t slot_phys;
  uint32_t channelbase;
  int slot_mem_size, frag_mem_size;
  int err;
  uint32_t fragment_size;

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

  if (vchiq_init_state(state, vchiq_slot_zero, 0) != VCHIQ_SUCCESS)
    return ERR_INVAL_ARG;

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

  vchiq_handmade(state, vchiq_slot_zero);

  return ERR_OK;
}

VCHIQ_STATUS_T
vchiq_platform_init_state(VCHIQ_STATE_T *state)
{
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
   // state->platform_state = kzalloc(sizeof(VCHIQ_2835_ARM_STATE_T), GFP_KERNEL);
   ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 1;
   // status = vchiq_arm_init_state(state, &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state);
//   if(status != VCHIQ_SUCCESS)
//   {
//      ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 0;
//   }
   return status;
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

	dsb();         /* data barrier operation */

	if (event->armed)
    vchiq_ring_bell();
//		write_reg(g_regs + BELL2, 0); /* trigger vc interrupt */
}

VCHIQ_STATUS_T
vchiq_prepare_bulk_data(VCHIQ_BULK_T *bulk, VCHI_MEM_HANDLE_T memhandle,
	void *offset, int size, int dir)
{
	// PAGELIST_T *pagelist;
	// int ret;

	// WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);

//	ret = create_pagelist((char __user *)offset, size,
//			(dir == VCHIQ_BULK_RECEIVE)
//			? PAGELIST_READ
//			: PAGELIST_WRITE,
//			current,
//			&pagelist);
//	if (ret != 0)
		return VCHIQ_ERROR;

	bulk->handle = memhandle;
	// bulk->data = VCHIQ_ARM_ADDRESS(pagelist);

	/* Store the pagelist address in remote_data, which isn't used by the
	   slave. */
	// bulk->remote_data = pagelist;

	return VCHIQ_SUCCESS;
}

void
vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
//	if (bulk && bulk->remote_data && bulk->actual)
//		free_pagelist((PAGELIST_T *)bulk->remote_data, bulk->actual);
}

void
vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
	/*
	 * This should only be called on the master (VideoCore) side, but
	 * provide an implementation to avoid the need for ifdefery.
	 */
	BUG(1, "vchiq_transfer_bulk");
}

VCHIQ_STATUS_T
vchiq_platform_suspend(VCHIQ_STATE_T *state)
{
   return VCHIQ_ERROR;
}

VCHIQ_STATUS_T
vchiq_platform_resume(VCHIQ_STATE_T *state)
{
   return VCHIQ_SUCCESS;
}

void
vchiq_platform_paused(VCHIQ_STATE_T *state)
{
}

void
vchiq_platform_resumed(VCHIQ_STATE_T *state)
{
}

int
vchiq_platform_videocore_wanted(VCHIQ_STATE_T* state)
{
   return 1; // autosuspend not supported - videocore always wanted
}

int
vchiq_platform_use_suspend_timer(void)
{
   return ERR_OK;
}
void
vchiq_dump_platform_use_state(VCHIQ_STATE_T *state)
{
	vchiq_log_info(vchiq_arm_log_level, "Suspend timer not in use");
}
void
vchiq_platform_handle_timeout(VCHIQ_STATE_T *state)
{
	(void)state;
}
/*
 * Local functions
 */

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id)
{
#define IRQ_NONE 0
#define IRQ_HANDLED 1
#define IRQ_WAKE_THREAD 2
	VCHIQ_STATE_T *state = dev_id;
	irqreturn_t ret = IRQ_NONE;
	unsigned int status;

	/* Read (and clear) the doorbell */
	status = read_reg(g_regs + BELL0);

	if (status & 0x4) {  /* Was the doorbell rung? */
		remote_event_pollall(state);
		ret = IRQ_HANDLED;
	}

	return ret;
}

/* There is a potential problem with partial cache lines (pages?)
** at the ends of the block when reading. If the CPU accessed anything in
** the same line (page?) then it may have pulled old data into the cache,
** obscuring the new data underneath. We can solve this by transferring the
** partial cache lines separately, and allowing the ARM to copy into the
** cached area.

** N.B. This implementation plays slightly fast and loose with the Linux
** driver programming rules, e.g. its use of dmac_map_area instead of
** dma_map_single, but it isn't a multi-platform driver and it benefits
** from increased speed as a result.
*/

#ifdef __circle__
#undef PAGE_SIZE
#define PAGE_SIZE	4096

struct page {};
#define vmalloc_to_page(p)	((struct page *) ((uintptr_t) (p) & ~(PAGE_SIZE - 1)))
#define page_address(pg)	((void *) (pg))
#endif

//static int
//create_pagelist(char __user *buf, size_t count, unsigned short type,
//	struct task_struct *task, PAGELIST_T ** ppagelist)
//{
//	PAGELIST_T *pagelist;
//	struct page **pages;
//	unsigned int *addrs;
//	unsigned int num_pages, offset, i;
//	char *addr, *base_addr, *next_addr;
//	int run, addridx, actual_pages;
//        unsigned int *need_release;
//
//	offset = (unsigned int)(uintptr_t)buf & (PAGE_SIZE - 1);
//	num_pages = (count + offset + PAGE_SIZE - 1) / PAGE_SIZE;
//
//	*ppagelist = NULL;
//
//	/* Allocate enough storage to hold the page pointers and the page
//	** list
//	*/
//	pagelist = kmalloc(sizeof(PAGELIST_T) +
//                           (num_pages * sizeof(unsigned int)) +
//                           sizeof(unsigned int) +
//                           (num_pages * sizeof(pages[0])),
//                           GFP_KERNEL);
//
//	vchiq_log_trace(vchiq_arm_log_level,
//		"create_pagelist - %x", (unsigned int)(uintptr_t)pagelist);
//	if (!pagelist)
//		return -ENOMEM;
//
//	addrs = pagelist->addrs;
//        need_release = (unsigned int *)(addrs + num_pages);
//	pages = (struct page **)(addrs + num_pages + 1);
//
//#ifndef __circle__
//	if (is_vmalloc_addr(buf)) {
//		int dir = (type == PAGELIST_WRITE) ?
//			DMA_TO_DEVICE : DMA_FROM_DEVICE;
//#endif
//		unsigned int length = count;
//		unsigned int off = offset;
//
//		for (actual_pages = 0; actual_pages < num_pages;
//		     actual_pages++) {
//			struct page *pg = vmalloc_to_page(buf + (actual_pages *
//								 PAGE_SIZE));
//			size_t bytes = PAGE_SIZE - off;
//
//			if (bytes > length)
//				bytes = length;
//			pages[actual_pages] = pg;
//#ifndef __circle__
//			dmac_map_area(page_address(pg) + off, bytes, dir);
//#else
//			CleanAndInvalidateDataCacheRange ((uintptr_t) pg + off, bytes);
//#endif
//			length -= bytes;
//			off = 0;
//		}
//		*need_release = 0; /* do not try and release vmalloc pages */
//#ifndef __circle__
//	} else {
//		down_read(&task->mm->mmap_sem);
//		actual_pages = get_user_pages(
//				          (unsigned int)buf & ~(PAGE_SIZE - 1),
//					  num_pages,
//					  (type == PAGELIST_READ) ? FOLL_WRITE : 0,
//					  pages,
//					  NULL /*vmas */);
//		up_read(&task->mm->mmap_sem);
//
//		if (actual_pages != num_pages) {
//			vchiq_log_info(vchiq_arm_log_level,
//				       "create_pagelist - only %d/%d pages locked",
//				       actual_pages,
//				       num_pages);
//
//			/* This is probably due to the process being killed */
//			while (actual_pages > 0)
//			{
//				actual_pages--;
//				put_page(pages[actual_pages]);
//			}
//			kfree(pagelist);
//			if (actual_pages == 0)
//				actual_pages = -ENOMEM;
//			return actual_pages;
//		}
//		*need_release = 1; /* release user pages */
//	}
//#endif
//
//	pagelist->length = count;
//	pagelist->type = type;
//	pagelist->offset = offset;
//
//	/* Group the pages into runs of contiguous pages */
//
//#if RASPPI <= 3
//	base_addr = VCHIQ_ARM_ADDRESS(page_address(pages[0]));
//	next_addr = base_addr + PAGE_SIZE;
//	addridx = 0;
//	run = 0;
//
//	for (i = 1; i < num_pages; i++) {
//		addr = VCHIQ_ARM_ADDRESS(page_address(pages[i]));
//		if ((addr == next_addr) && (run < (PAGE_SIZE - 1))) {
//			next_addr += PAGE_SIZE;
//			run++;
//		} else {
//			addrs[addridx] = (unsigned int)(uintptr_t)base_addr + run;
//			addridx++;
//			base_addr = addr;
//			next_addr = addr + PAGE_SIZE;
//			run = 0;
//		}
//	}
//
//	addrs[addridx] = (unsigned int)(uintptr_t)base_addr + run;
//	addridx++;
//#else
//	base_addr = page_address(pages[0]);
//	next_addr = base_addr + PAGE_SIZE;
//	addridx = 0;
//	run = 0;
//
//	for (i = 1; i < num_pages; i++) {
//		addr = page_address(pages[i]);
//		if ((addr == next_addr) && (run < 255)) {
//			next_addr += PAGE_SIZE;
//			run++;
//		} else {
//			BUG_ON (((uintptr_t)base_addr & 0xFFF) != 0);
//			addrs[addridx] = (unsigned int)((uintptr_t)base_addr >> 4) + run;
//			addridx++;
//			base_addr = addr;
//			next_addr = addr + PAGE_SIZE;
//			run = 0;
//		}
//	}
//
//	BUG_ON (((uintptr_t)base_addr & 0xFFF) != 0);
//	addrs[addridx] = (unsigned int)((uintptr_t)base_addr >> 4) + run;
//	addridx++;
//#endif
//
//#ifndef __circle__
//	/* Partial cache lines (fragments) require special measures */
//	if ((type == PAGELIST_READ) &&
//		((pagelist->offset & (g_cache_line_size - 1)) ||
//		((pagelist->offset + pagelist->length) &
//		(g_cache_line_size - 1)))) {
//		char *fragments;
//
//		if (down_interruptible(&g_free_fragments_sema) != 0) {
//			kfree(pagelist);
//			return -EINTR;
//		}
//
//		WARN_ON(g_free_fragments == NULL);
//
//		down(&g_free_fragments_mutex);
//		fragments = g_free_fragments;
//		WARN_ON(fragments == NULL);
//		g_free_fragments = *(char **) g_free_fragments;
//		up(&g_free_fragments_mutex);
//		pagelist->type = PAGELIST_READ_WITH_FRAGMENTS +
//			(fragments - g_fragments_base) / g_fragments_size;
//	}
//
//	dmac_flush_range(pagelist, addrs + num_pages);
//#else
//	CleanAndInvalidateDataCacheRange ((uintptr_t) pagelist,
//					  (uintptr_t) (addrs + num_pages) - (uintptr_t) pagelist);
//#endif
//
//	*ppagelist = pagelist;
//
//	return ERR_OK;
//}
//
//static void
//free_pagelist(PAGELIST_T *pagelist, int actual)
//{
//#ifndef __circle__
//        unsigned long *need_release;
//	struct page **pages;
//	unsigned int num_pages, i;
//#endif
//
//	vchiq_log_trace(vchiq_arm_log_level,
//		"free_pagelist - %x, %d", (unsigned int)(uintptr_t)pagelist, actual);
//
//#ifndef __circle__
//	num_pages =
//		(pagelist->length + pagelist->offset + PAGE_SIZE - 1) /
//		PAGE_SIZE;
//
//        need_release = (unsigned long *)(pagelist->addrs + num_pages);
//	pages = (struct page **)(pagelist->addrs + num_pages + 1);
//
//	/* Deal with any partial cache lines (fragments) */
//	if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS) {
//		char *fragments = g_fragments_base +
//			(pagelist->type - PAGELIST_READ_WITH_FRAGMENTS) *
//			g_fragments_size;
//		int head_bytes, tail_bytes;
//		head_bytes = (g_cache_line_size - pagelist->offset) &
//			(g_cache_line_size - 1);
//		tail_bytes = (pagelist->offset + actual) &
//			(g_cache_line_size - 1);
//
//		if ((actual >= 0) && (head_bytes != 0)) {
//			if (head_bytes > actual)
//				head_bytes = actual;
//
//			memcpy((char *)kmap(pages[0]) +
//				pagelist->offset,
//				fragments,
//				head_bytes);
//			kunmap(pages[0]);
//		}
//		if ((actual >= 0) && (head_bytes < actual) &&
//			(tail_bytes != 0)) {
//			memcpy((char *)kmap(pages[num_pages - 1]) +
//				((pagelist->offset + actual) &
//				(PAGE_SIZE - 1) & ~(g_cache_line_size - 1)),
//				fragments + g_cache_line_size,
//				tail_bytes);
//			kunmap(pages[num_pages - 1]);
//		}
//
//		down(&g_free_fragments_mutex);
//		*(char **)fragments = g_free_fragments;
//		g_free_fragments = fragments;
//		up(&g_free_fragments_mutex);
//		up(&g_free_fragments_sema);
//	}
//
//	if (*need_release) {
//		unsigned int length = pagelist->length;
//		unsigned int offset = pagelist->offset;
//
//		for (i = 0; i < num_pages; i++) {
//			struct page *pg = pages[i];
//
//			if (pagelist->type != PAGELIST_WRITE) {
//				unsigned int bytes = PAGE_SIZE - offset;
//
//				if (bytes > length)
//					bytes = length;
//				dmac_unmap_area(page_address(pg) + offset,
//						bytes, DMA_FROM_DEVICE);
//				length -= bytes;
//				offset = 0;
//				set_page_dirty(pg);
//			}
//			put_page(pg);
//		}
//	}
//#endif
//
//	kfree(pagelist);
//}
