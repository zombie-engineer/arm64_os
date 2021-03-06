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

#define TOTAL_SLOTS (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)

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

//static void __iomem *g_regs;

extern int vchiq_arm_log_level;

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id);

// static int
// create_pagelist(char *buf, size_t count, unsigned short type,
 //               struct task_struct *task, PAGELIST_T ** ppagelist);

//static void
//free_pagelist(PAGELIST_T *pagelist, int actual);
//
//
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

// structure used to provide the information needed to open a server or a client
typedef struct {
  struct vchi_version version;
  int32_t service_id;
  VCHI_CONNECTION_T *connection;
  uint32_t rx_fifo_size;
  uint32_t tx_fifo_size;
  VCHI_CALLBACK_T callback;
  void *callback_param;
  /* client intends to receive bulk transfers of
  odd lengths or into unaligned buffers */
  int32_t want_unaligned_bulk_rx;
  /* client intends to transmit bulk transfers of
  odd lengths or out of unaligned buffers */
  int32_t want_unaligned_bulk_tx;
  /* client wants to check CRCs on (bulk) xfers.
  Only needs to be set at 1 end - will do both directions. */
  int32_t want_crc;
} SERVICE_CREATION_T;

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

void *vchiq_mmal_check_reply_msg(struct vchiq_header_struct *h, int msg_type, int mmal_msg_type)
{
  struct mmal_msg *rmsg;
  if (VCHIQ_MSG_TYPE(h->msgid) != msg_type) {
    MMAL_ERR("vchiq message from remote is not of expected type %s", msg_type_str(msg_type));
    return NULL;
  }
  rmsg = (struct mmal_msg *)h->data;
  if (rmsg->h.type != mmal_msg_type) {
    MMAL_ERR("vchiq mmal message from remote is not of expected type %s",
      mmal_msg_type_names[mmal_msg_type]);
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
    printf("Expected msg type CONNECT from remote\r\n");
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

struct mmal_service {
  int remoteport;
  int localport;
  struct vchiq_state_struct *s;
  int component_handle;
};

struct mmal_component {
  struct mmal_service *ms;
  int handle;
  char name[16];
  bool enabled;
  int inputs;
  int outputs;
  int clocks;
  struct vchiq_mmal_port control;
  struct vchiq_mmal_port input[MAX_PORT_COUNT];
  struct vchiq_mmal_port output[MAX_PORT_COUNT];
};

void vchiq_handmade_prep_msg(struct vchiq_state_struct *s, int msgid, int srcport, int dstport, void *payload, int payload_sz)
{
  struct vchiq_header_struct *h;

  h = vchiq_prep_next_header_tx(s, payload_sz);

  h->msgid = VCHIQ_MAKE_MSG(msgid, srcport, dstport);
  memcpy(h->data, payload, payload_sz);
}

int vchiq_handmade_open_mmal(struct vchiq_state_struct *s, struct mmal_service *ms)
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
    printf("Expected msg type VCHIQ_MSG_OPENACK from remote\r\n");
    return ERR_GENERIC;
  }
  msg_id = h->msgid;
  openack = (struct vchiq_openack_payload *)(h->data);
  ms->localport = VCHIQ_MSG_DSTPORT(msg_id);
  ms->remoteport = VCHIQ_MSG_SRCPORT(msg_id);
  printf("OPENACK: msgid: %08x, localport: %d, remoteport: %d, version: %d\r\n",
    msg_id,
    ms->localport,
    ms->remoteport,
    openack->version);
  return ERR_OK;
}

void vchiq_mmal_fill_header(struct mmal_service *ms, int mmal_msg_type, struct mmal_msg *msg)
{
  msg->h.magic = MMAL_MAGIC;
  msg->h.context = 0x5a5a5a5a;
  msg->h.control_service = 0x6a6a6a6a;
  msg->h.status = 0;
  msg->h.padding = 0;
  msg->h.type = mmal_msg_type;
}

int vchiq_mmal_handmade_component_disable(struct mmal_service *ms, struct mmal_component *c)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_component_disable_reply *r;

  BUG(!c->enabled, "trying to disable mmal component, which is already disabled");

  memset(&msg, 0, sizeof(msg));

  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_COMPONENT_DISABLE, &msg);
  msg.u.component_disable.component_handle = c->handle;

  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.component_disable));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);
  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_COMPONENT_DISABLE);
  if (!r)
    return ERR_GENERIC;
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("component disable status not success: %d\r\n", r->status);
    return ERR_GENERIC;
  }
  c->enabled = false;
  printf("vchiq_mmal_handmade_component_disable, name:%s, handle:%d, status:%d\r\n",
    c->name, c->handle, r->status);
  return ERR_OK;
}

int vchiq_mmal_handmade_component_destroy(struct mmal_service *ms, struct mmal_component *c)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_component_destroy_reply *r;

  BUG(c->enabled, "trying to destroy mmal component, which is not disabled first");
  memset(&msg, 0, sizeof(msg));

  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_COMPONENT_DESTROY, &msg);
  msg.u.component_destroy.component_handle = c->handle;

  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.component_destroy));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);
  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_COMPONENT_DESTROY);
  if (!r)
    return ERR_GENERIC;
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("component destroy status not success: %d\r\n", r->status);
    return ERR_GENERIC;
  }
  printf("vchiq_mmal_handmade_component_destroy, name:%s, handle:%d, status:%d\r\n",
    c->name, c->handle, r->status);
  kfree(c);
  return ERR_OK;
}

int vchiq_mmal_port_info_get(struct mmal_service *ms, struct mmal_component *c, struct vchiq_mmal_port *p)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_port_info_get_reply *r;

  msg.u.port_info_get.component_handle = c->handle;
  msg.u.port_info_get.index = p->index;
  msg.u.port_info_get.port_type = p->type;

  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_PORT_INFO_GET, &msg);
  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_info_get));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);
  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_PORT_INFO_GET);
  if (!r) {
    MMAL_ERR("invalid reply");
    return ERR_GENERIC;
  }

  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    MMAL_ERR("status not success: %d", r->status);
    return ERR_GENERIC;
  }

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
  return ERR_OK;
}


struct mmal_component *vchiq_mmal_component_create(struct mmal_service *ms, const char *name, int component_idx)
{
  struct mmal_component *c;
  struct vchiq_header_struct *header;
  struct mmal_msg_component_create_reply *reply;
  struct mmal_msg msg;
  int i;

  c = kzalloc(sizeof(*c), GFP_KERNEL);
  if (IS_ERR(c)) {
    printf("failed to allocate memory for mmal_component\n");
    return c;
  }

  memset(&msg, 0, sizeof(msg));

  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_COMPONENT_CREATE, &msg);
  msg.u.component_create.client_component = component_idx;
  strncpy(msg.u.component_create.name, name, sizeof(msg.u.component_create.name));

  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.component_create));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);

  header = vchiq_get_next_header_rx(ms->s);
  reply = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_COMPONENT_CREATE);
  if (!reply) {
    printf("invalid reply for create component\r\n");
    return ERR_PTR(ERR_GENERIC);
  }
  if (reply->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("component create status not success: %d\r\n", reply->status);
    return ERR_PTR(ERR_GENERIC);
  }

  c->handle = reply->component_handle;
  c->ms = ms;
  c->enabled = true;
  c->inputs = reply->input_num;
  c->outputs = reply->output_num;
  c->clocks = reply->clock_num;
  strncpy(c->name, name, sizeof(c->name));
  printf("vchiq component created name:%s: status:%d, handle: %d, input: %d, output: %d, clock: %d\r\n",
    c->name, reply->status, c->handle, c->inputs, c->outputs, c->clocks);

  c->control.type = MMAL_PORT_TYPE_CONTROL;
  c->control.index = 0;
  vchiq_mmal_port_info_get(ms, c, &c->control);

  for (i = 0; i < c->inputs; ++i) {
    c->input[i].type = MMAL_PORT_TYPE_INPUT;
    c->input[i].index = i;
    vchiq_mmal_port_info_get(ms, c, &c->input[i]);
  }

  for (i = 0; i < c->outputs; ++i) {
    c->output[i].type = MMAL_PORT_TYPE_OUTPUT;
    c->output[i].index = i;
    vchiq_mmal_port_info_get(ms, c, &c->output[i]);
  }

  return c;
}

struct mmal_component *vchiq_mmal_create_camera_info(struct mmal_service *ms)
{
  return vchiq_mmal_component_create(ms, "camera_info", 0);
}

int vchiq_mmal_port_info_set(struct mmal_service *ms, struct mmal_component *c, struct vchiq_mmal_port *p)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_port_info_set *ps;
  struct mmal_msg_port_info_set_reply *r;

  ps = &msg.u.port_info_set;

  ps->component_handle = c->handle;
  ps->port_type = p->type;
  ps->port_index = p->index;

  port_to_mmal_msg(p, &ps->port);
  ps->format.type = p->format.type;
  ps->format.encoding = p->format.encoding;
  ps->format.encoding_variant = p->format.encoding_variant;
  ps->format.bitrate = p->format.bitrate;
  ps->format.flags = p->format.flags;

  memcpy(&ps->es, &p->es, sizeof(union mmal_es_specific_format));

  ps->format.extradata_size = p->format.extradata_size;
  memcpy(&ps->extradata, p->format.extradata, p->format.extradata_size);

  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_PORT_INFO_SET, &msg);
  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_info_set));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);

  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_PORT_INFO_SET);
  if (!r) {
    MMAL_ERR("invalid reply");
    return ERR_GENERIC;
  }
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    MMAL_ERR("status not success: %d", r->status);
    return ERR_GENERIC;
  }
  return ERR_OK;
}

int vchiq_mmal_port_parameter_set(struct mmal_component *c, struct vchiq_mmal_port *p,
  uint32_t parameter_id, void *value, int value_size)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_port_parameter_set_reply *r;

  /* GET PARAMETER CAMERA INFO */
  msg.u.port_parameter_set.component_handle = c->handle;
  msg.u.port_parameter_set.port_handle = p->handle;
  msg.u.port_parameter_set.id = parameter_id;
  msg.u.port_parameter_set.size = 2 * sizeof(uint32_t) + value_size;
  memcpy(&msg.u.port_parameter_set.value, value, value_size);

  /* send MMAL message synchronous */
  vchiq_mmal_fill_header(c->ms, MMAL_MSG_TYPE_PORT_PARAMETER_SET, &msg);
  vchiq_handmade_prep_msg(c->ms->s, VCHIQ_MSG_DATA,
    c->ms->localport,
    c->ms->remoteport,
    &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_parameter_set));
  vchiq_event_signal(&c->ms->s->remote->trigger);
  vchiq_event_wait(&c->ms->s->local->trigger);

  header = vchiq_get_next_header_rx(c->ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_PORT_PARAMETER_SET);
  if (!r) {
    printf("port_paramter_set error: invalid reply\r\n");
    return ERR_GENERIC;
  }
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("failed to set vchiq mmal port parameter '%d', status: %d\r\n",
      parameter_id, r->status);
    return ERR_GENERIC;
  }
  return ERR_OK;
}

int vchiq_mmal_port_set_format(struct mmal_service *ms, struct mmal_component *c,
  struct vchiq_mmal_port *port)
{
  int err;
  err = vchiq_mmal_port_info_set(ms, c, port);
  CHECK_ERR("failed to set port info");

  err = vchiq_mmal_port_info_get(ms, c, port);
  CHECK_ERR("failed to get port info");

out_err:
  return err;
}

int vchiq_mmal_port_parameter_get(struct mmal_service *ms, struct mmal_component *c,
  struct vchiq_mmal_port *port, int parameter_id, void *value, int *value_size)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_port_parameter_get_reply *r;

  msg.u.port_parameter_get.component_handle = c->handle;
  msg.u.port_parameter_get.port_handle = port->handle;
  msg.u.port_parameter_get.id = parameter_id;
  msg.u.port_parameter_get.size = 2 * sizeof(uint32_t) + *value_size;

  /* send MMAL message synchronous */
  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_PORT_PARAMETER_GET, &msg);
  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_parameter_get));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);
  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_PORT_PARAMETER_GET);
  if (!r) {
    printf("vchiq_mmal_port_parameter_get: invalid reply\r\n");
    return -1;
  }
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("vchiq_mmal_port_parameter_get: status not success: %d\r\n", r->status);
    return ERR_GENERIC;
  }
  memcpy(value, r->value, min(r->size, *value_size));
  *value_size = r->size;
  return ERR_OK;
}

static inline int vchiq_mmal_get_camera_info(struct mmal_service *ms, struct mmal_component *c, struct mmal_parameter_camera_info_t *cam_info)
{
  int err;
  int param_size;

  param_size = sizeof(*cam_info);
  err = vchiq_mmal_port_parameter_get(ms, c, &c->control,
    MMAL_PARAMETER_CAMERA_INFO, cam_info, &param_size);
  return err;
}

void vchiq_mmal_cam_info_print(struct mmal_parameter_camera_info_t *cam_info)
{
  int i;
  struct mmal_parameter_camera_info_camera_t *c;

  for (i = 0; i < cam_info->num_cameras; ++i) {
    c = &cam_info->cameras[i];
    printf("cam %d: name:%s, W x H: %dx%x\r\n", i, c->camera_name, c->max_width, c->max_height);
  }
}

int mmal_set_camera_parameters(struct mmal_component *c, struct mmal_parameter_camera_info_camera_t *cam_info)
{
  struct mmal_parameter_camera_config config = {
    .max_stills_w = cam_info->max_width,
    .max_stills_h = cam_info->max_height,
    .stills_yuv422 = 1,
    .one_shot_stills = 1,
    .max_preview_video_w = min(1920, cam_info->max_width),
    .max_preview_video_h = min(1920, cam_info->max_height),
    .num_preview_video_frames = 3,
    .stills_capture_circular_buffer_height = 0,
    .fast_preview_resume = 0,
    .use_stc_timestamp = 0
  };

  vchiq_mmal_port_parameter_set(c, &c->control,
    MMAL_PARAMETER_CAMERA_CONFIG, &config, sizeof(config));
  return ERR_OK;
}

int _vchiq_mmal_submit_buffer(struct mmal_service *ms,
  struct vchiq_mmal_port *port, struct mmal_buffer *buffer)
{
  return ERR_OK;
}

int vchiq_mmal_port_action_port(struct mmal_service *ms, struct mmal_component *c, struct vchiq_mmal_port *p, int action)
{
  struct mmal_msg msg;
  struct vchiq_header_struct *header;
  struct mmal_msg_port_action_reply *r;

  msg.u.port_action_port.component_handle = c->handle;
  msg.u.port_action_port.port_handle = p->handle;
  msg.u.port_action_port.action = action;
  port_to_mmal_msg(p, &msg.u.port_action_port.port);

  /* send MMAL message synchronous */
  vchiq_mmal_fill_header(ms, MMAL_MSG_TYPE_PORT_ACTION, &msg);
  vchiq_handmade_prep_msg(ms->s, VCHIQ_MSG_DATA, ms->localport, ms->remoteport, &msg,
    sizeof(struct mmal_msg_header) + sizeof(msg.u.port_action_port));
  vchiq_event_signal(&ms->s->remote->trigger);
  vchiq_event_wait(&ms->s->local->trigger);
  header = vchiq_get_next_header_rx(ms->s);
  r = vchiq_mmal_check_reply_msg(header, VCHIQ_MSG_DATA, MMAL_MSG_TYPE_PORT_ACTION);
  if (!r) {
    printf("vchiq_mmal_port_action_port: invalid reply\r\n");
    return -1;
  }
  if (r->status != MMAL_MSG_STATUS_SUCCESS) {
    printf("vchiq_mmal_port_action_port: status not success: %d\r\n", r->status);
    return ERR_GENERIC;
  }
  return ERR_OK;
}

int vchiq_mmal_port_enable(struct mmal_service *ms, struct mmal_component *c, struct vchiq_mmal_port *p)
{
  int err;

  if (p->enabled) {
    printf("vchiq_mmal_port_enable: skipping port already enabled\r\n");
    return ERR_OK;
  }
  err = vchiq_mmal_port_action_port(ms, c, p, MMAL_MSG_PORT_ACTION_TYPE_ENABLE);
  if (err) {
    printf("vchiq_mmal_port_enable: port action type enable failed, err: %d\r\n", err);
    return err;
  }
  p->enabled = 1;
  return vchiq_mmal_port_info_get(ms, c, p);
}

int vchiq_mmal_camera_capture(struct mmal_service *ms, struct mmal_component *cam)
{
  uint32_t frame_count;

  frame_count = 1;
  return vchiq_mmal_port_parameter_set(cam, &cam->output[CAM_PORT_CAPTURE],
    MMAL_PARAMETER_CAPTURE, &frame_count, sizeof(frame_count));
}

int vchiq_handmade_run_camera(struct mmal_service *ms)
{
  int err;
  int param_size;
  int enable;
  /* mmal create component */
  struct mmal_component *camera_info, *image_encode, *video_encode;
  struct mmal_component *ril_camera;
  struct mmal_component *preview_camera;
  struct vchiq_mmal_port *encoder_port;
  struct mmal_parameter_camera_info_t cam_info = {0};
  struct mmal_es_format_local *format;
  uint32_t supported_encodings[MAX_SUPPORTED_ENCODINGS];

  camera_info = vchiq_mmal_create_camera_info(ms);
  err = vchiq_mmal_get_camera_info(ms, camera_info, &cam_info);
  CHECK_ERR("Failed to get camera info");

  vchiq_mmal_cam_info_print(&cam_info);
  err = vchiq_mmal_handmade_component_disable(ms, camera_info);
  CHECK_ERR("Failed to disable 'camera info' component");

  err = vchiq_mmal_handmade_component_destroy(ms, camera_info);
  CHECK_ERR("Failed to destroy 'camera info' component");

  ril_camera = vchiq_mmal_component_create(ms, "ril.camera", 0);
  CHECK_ERR_PTR(ril_camera, "Failed to create component 'ril.camera'");

  err =  mmal_set_camera_parameters(ril_camera, &cam_info.cameras[0]);
  CHECK_ERR("Failed to set parameters to component 'ril.camera'");

  format = &ril_camera->output[CAM_PORT_PREVIEW].format;
  format->encoding = MMAL_ENCODING_OPAQUE;
  format->encoding_variant = MMAL_ENCODING_I420;
  format->es->video.width = 1024;
  format->es->video.height = 768;
  format->es->video.crop.x = 0;
  format->es->video.crop.y = 0;
  format->es->video.crop.width = 1024;
  format->es->video.crop.height = 768;
  format->es->video.frame_rate.num = 0;
  format->es->video.frame_rate.den = 1;

  format = &ril_camera->output[CAM_PORT_VIDEO].format;
  format->encoding = MMAL_ENCODING_OPAQUE;
  format->encoding_variant = MMAL_ENCODING_I420;
  format->es->video.width = 1024;
  format->es->video.height = 768;
  format->es->video.crop.x = 0;
  format->es->video.crop.y = 0;
  format->es->video.crop.width = 1024;
  format->es->video.crop.height = 768;
  format->es->video.frame_rate.num = 0;
  format->es->video.frame_rate.den = 1;

  format = &ril_camera->output[CAM_PORT_CAPTURE].format;
  format->encoding = MMAL_ENCODING_OPAQUE;
  format->es->video.width = 2592;
  format->es->video.height = 1944;
  format->es->video.crop.x = 0;
  format->es->video.crop.y = 0;
  format->es->video.crop.width = 2592;
  format->es->video.crop.height = 1944;
  format->es->video.frame_rate.num = 0;
  format->es->video.frame_rate.den = 1;

  preview_camera = vchiq_mmal_component_create(ms, "ril.video_render", 0);
  CHECK_ERR_PTR(preview_camera, "Failed to create component 'ril.video_render'");

  image_encode = vchiq_mmal_component_create(ms, "ril.image_encode", 0);
  CHECK_ERR_PTR(preview_camera, "Failed to create component 'ril.image_encode'");

  video_encode = vchiq_mmal_component_create(ms, "ril.video_encode", 0);
  CHECK_ERR_PTR(preview_camera, "Failed to create component 'ril.video_encode'");

  encoder_port = &video_encode->output[0];
  encoder_port->format.encoding = MMAL_ENCODING_H264;
  vchiq_mmal_port_set_format(ms, video_encode, encoder_port);

  enable = 1;
  err = vchiq_mmal_port_parameter_set(video_encode, &video_encode->control,
    MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, &enable, sizeof(enable));
  CHECK_ERR("Failed to set MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT to 1");

  err = vchiq_mmal_port_parameter_set(video_encode, &video_encode->control,
    MMAL_PARAMETER_MINIMISE_FRAGMENTATION, &enable, sizeof(enable));
  CHECK_ERR("Failed to set MMAL_PARAMETER_MINIMISE_FRAGMENTATION to 1");

//  err = vchiq_mmal_port_parameter_set(video_encode, &video_encode->control,
//    MMAL_PARAMETER_VIDEO_ENCODE_HEADERS_WITH_FRAME, &enable, sizeof(enable));
//  CHECK_ERR("Failed to set MMAL_PARAMETER_VIDEO_ENCODE_HEADERS_WITH_FRAME to 1");

  param_size = sizeof(supported_encodings);
  err = vchiq_mmal_port_parameter_get(ms, ril_camera, &ril_camera->output[CAM_PORT_CAPTURE],
    MMAL_PARAMETER_SUPPORTED_ENCODINGS, supported_encodings, &param_size);
  CHECK_ERR("Failed to get port parameters for component 'ril.camera'");


  err = vchiq_mmal_port_enable(ms, ril_camera, &ril_camera->output[CAM_PORT_CAPTURE]);
  CHECK_ERR("Failed to enable port 'CAM_PORT_CAPTURE' for component 'ril.camera'");

  err = vchiq_mmal_camera_capture(ms, ril_camera);
  CHECK_ERR("Failed to set camera capture port for 'ril.camera'");

  err = _vchiq_mmal_submit_buffer(ms, &ril_camera->output[CAM_PORT_CAPTURE], NULL);
  CHECK_ERR("Failed to get mmal buffer for 'ril.camera' capture");
out_err:
  return err;
}

void vchiq_handmade(struct vchiq_state_struct *s, struct vchiq_slot_zero_struct *z)
{
  int err;
  struct mmal_service ms;

  ms.s = s;
  s->rx_pos = 0;

  s->conn_state = VCHIQ_CONNSTATE_DISCONNECTED;

  err = vchiq_handmade_connect(s);
  if (err) {
    printf("vchiq_handmade: failed at connection step, err: %d\r\n", err);
    goto err;
  }
  err =  vchiq_handmade_open_mmal(s, &ms);
  if (err) {
    printf("vchiq_handmade failed at open mmal step, err: %d\r\n", err);
    goto err;
  }
  err = vchiq_handmade_run_camera(&ms);
  if (err) {
    printf("vchiq_handmade: failed at run camera step, err: %d\r\n", err);
    goto err;
  }

err:
  while(1) asm volatile("wfe");
}

int vchiq_platform_init(VCHIQ_STATE_T *state)
{
	VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
	void *slot_mem;
	uint32_t slot_phys;
	uint32_t channelbase;
	int slot_mem_size, frag_mem_size;
	int err;

	/* Allocate space for the channels in coherent memory */
	slot_mem_size = 80 /* TOTAL_SLOTS */ * VCHIQ_SLOT_SIZE;
	frag_mem_size = 0;

	slot_mem = dma_alloc(slot_mem_size + frag_mem_size);
  slot_phys = (uint32_t)(uint64_t)slot_mem;
	if (!slot_mem) {
		printf("could not allocate DMA memory\n");
		return ERR_MEMALLOC;
	}

	vchiq_slot_zero = vchiq_init_slots(slot_mem, slot_mem_size);
	if (!vchiq_slot_zero)
		return ERR_INVAL_ARG;

	if (vchiq_init_state(state, vchiq_slot_zero, 0) != VCHIQ_SUCCESS)
		return ERR_INVAL_ARG;

	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX] =
		(int)slot_phys + slot_mem_size;
	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX] =
		MAX_FRAGMENTS;

	/* Send the base address of the slots to VideoCore */
	channelbase = 0xc0000000 | slot_phys;

  err = mbox_init_vchiq(&channelbase);
	if (err || channelbase) {
	  LOG(0, ERR, "vchiq", "failed to set channelbase");
		return err ? : ERR_GENERIC;
	}

//	if (vchiq_init_state(state, vchiq_slot_zero, 0) != VCHIQ_SUCCESS)
//		return ERR_INVAL_ARG;

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
