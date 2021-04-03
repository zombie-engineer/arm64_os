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

#include "vchiq_core.h"
#include "vchiq_killable.h"
#include <atomic.h>
#include <reg_access.h>
#include <sched.h>

#define VCHIQ_SLOT_HANDLER_STACK 8192

#define HANDLE_STATE_SHIFT 12

int vchiq_core_msg_log_level;
int vchiq_arm_log_level;
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

VCHIQ_SLOT_ZERO_T *vslotz = NULL;
VCHIQ_SLOT_T *vslot = NULL;
extern VCHIQ_STATE_T vchiq_state;

struct vchiq_open_payload {
	int fourcc;
	int client_id;
	short version;
	short version_min;
};

struct vchiq_openack_payload {
	short version;
};

enum
{
	QMFLAGS_IS_BLOCKING     = (1 << 0),
	QMFLAGS_NO_MUTEX_LOCK   = (1 << 1),
	QMFLAGS_NO_MUTEX_UNLOCK = (1 << 2)
};

/* we require this for consistency between endpoints */
vchiq_static_assert(sizeof(VCHIQ_HEADER_T) == 8);
vchiq_static_assert(IS_POW2(sizeof(VCHIQ_HEADER_T)));
vchiq_static_assert(IS_POW2(VCHIQ_NUM_CURRENT_BULKS));
vchiq_static_assert(IS_POW2(VCHIQ_NUM_SERVICE_BULKS));
vchiq_static_assert(IS_POW2(VCHIQ_MAX_SERVICES));
vchiq_static_assert(VCHIQ_VERSION >= VCHIQ_VERSION_MIN);

/* Run time control of log level, based on KERN_XXX level. */
int vchiq_core_log_level = VCHIQ_LOG_DEFAULT;
int vchiq_core_msg_log_level = VCHIQ_LOG_DEFAULT;
int vchiq_sync_log_level = VCHIQ_LOG_DEFAULT;

static DECL_SPINLOCK(service_spinlock);
DECL_SPINLOCK(bulk_waiter_spinlock);
DECL_SPINLOCK(quota_spinlock);

VCHIQ_STATE_T *vchiq_states[VCHIQ_MAX_STATES];
static unsigned int handle_seq;
static VCHIQ_SERVICE_T vservice;

static const char *const srvstate_names[] = {
	"FREE",
	"HIDDEN",
	"LISTENING",
	"OPENING",
	"OPEN",
	"OPENSYNC",
	"CLOSESENT",
	"CLOSERECVD",
	"CLOSEWAIT",
	"CLOSED"
};

static const char *const reason_names[] = {
	"SERVICE_OPENED",
	"SERVICE_CLOSED",
	"MESSAGE_AVAILABLE",
	"BULK_TRANSMIT_DONE",
	"BULK_RECEIVE_DONE",
	"BULK_TRANSMIT_ABORTED",
	"BULK_RECEIVE_ABORTED"
};

static const char *const conn_state_names[] = {
	"DISCONNECTED",
	"CONNECTING",
	"CONNECTED",
	"PAUSING",
	"PAUSE_SENT",
	"PAUSED",
	"RESUMING",
	"PAUSE_TIMEOUT",
	"RESUME_TIMEOUT"
};


static void
release_message_sync(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header);

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

static inline void
vchiq_set_service_state(VCHIQ_SERVICE_T *service, int newstate)
{
	vchiq_log_info(vchiq_core_log_level, "%d: srv:%d %s->%s",
		service->state->id, service->localport,
		srvstate_names[service->srvstate],
		srvstate_names[newstate]);
	service->srvstate = newstate;
}

VCHIQ_SERVICE_T *
find_service_by_handle(VCHIQ_SERVICE_HANDLE_T handle)
{
//  int flags;
	VCHIQ_SERVICE_T *service;

//	spinlock_lock_disable_irq(&service_spinlock, flags);
	service = handle_to_service(handle);
	if (service && (service->srvstate != VCHIQ_SRVSTATE_FREE) &&
		(service->handle == handle)) {
		BUG(service->ref_count == 0, "ref_count = 0");
		service->ref_count++;
	} else
		service = NULL;
//	spinlock_unlock_restore_irq(&service_spinlock, flags);

	if (!service)
		vchiq_log_info(vchiq_core_log_level,
			"Invalid service handle 0x%x", handle);

	return service;
}

VCHIQ_SERVICE_T *
find_service_by_port(VCHIQ_STATE_T *state, int localport)
{
  int flags;
	VCHIQ_SERVICE_T *service = NULL;
	if ((unsigned int)localport <= VCHIQ_PORT_MAX) {
		spinlock_lock_disable_irq(&service_spinlock, flags);
		service = state->services[localport];
		if (service && (service->srvstate != VCHIQ_SRVSTATE_FREE)) {
			BUG(service->ref_count == 0, "ref_count == 0 (b)");
			service->ref_count++;
		} else
			service = NULL;
		spinlock_unlock_restore_irq(&service_spinlock, flags);
	}

	if (!service)
		vchiq_log_info(vchiq_core_log_level,
			"Invalid port %d", localport);

	return service;
}

VCHIQ_SERVICE_T *
find_service_for_instance(VCHIQ_INSTANCE_T instance,
	VCHIQ_SERVICE_HANDLE_T handle) {
//  int flags;
	VCHIQ_SERVICE_T *service;

//	spinlock_lock_disable_irq(&service_spinlock, flags);
	service = handle_to_service(handle);
	if (service && (service->srvstate != VCHIQ_SRVSTATE_FREE) &&
		(service->handle == handle) &&
		(service->instance == instance)) {
		BUG(service->ref_count == 0, "ref_count == 0 (c)");
		service->ref_count++;
	} else
		service = NULL;
//	spinlock_unlock_restore_irq(&service_spinlock, flags);

	if (!service)
		vchiq_log_info(vchiq_core_log_level,
			"Invalid service handle 0x%x", handle);

	return service;
}

VCHIQ_SERVICE_T *
find_closed_service_for_instance(VCHIQ_INSTANCE_T instance,
	VCHIQ_SERVICE_HANDLE_T handle) {
//  int flags;
	VCHIQ_SERVICE_T *service;

//	spinlock_lock_disable_irq(&service_spinlock, flags);
	service = handle_to_service(handle);
	if (service &&
		((service->srvstate == VCHIQ_SRVSTATE_FREE) ||
		 (service->srvstate == VCHIQ_SRVSTATE_CLOSED)) &&
		(service->handle == handle) &&
		(service->instance == instance)) {
		BUG(service->ref_count == 0, "ref_count == 0 (d)");
		service->ref_count++;
	} else
		service = NULL;
//	spinlock_unlock_restore_irq(&service_spinlock, flags);

	if (!service)
		vchiq_log_info(vchiq_core_log_level,
			"Invalid service handle 0x%x", handle);

	return service;
}

VCHIQ_SERVICE_T *
next_service_by_instance(VCHIQ_STATE_T *state, VCHIQ_INSTANCE_T instance,
	int *pidx)
{
//  int flags;
	VCHIQ_SERVICE_T *service = NULL;
	int idx = *pidx;

//	spinlock_lock_disable_irq(&service_spinlock, flags);
	while (idx < state->unused_service) {
		VCHIQ_SERVICE_T *srv = state->services[idx++];
		if (srv && (srv->srvstate != VCHIQ_SRVSTATE_FREE) &&
			(srv->instance == instance)) {
			service = srv;
			BUG(service->ref_count == 0, "ref_count == 0 (e)");
			service->ref_count++;
			break;
		}
	}
//	spinlock_unlock_restore_irq(&service_spinlock, flags);

	*pidx = idx;

	return service;
}

void
lock_service(VCHIQ_SERVICE_T *service)
{
//  int flags;
//	spinlock_lock_disable_irq(&service_spinlock, flags);
	BUG(!service || (service->ref_count == 0), "ref_count == 0 (f)");
	if (service)
		service->ref_count++;
//	spinlock_unlock_restore_irq(&service_spinlock, flags);
}

void
unlock_service(VCHIQ_SERVICE_T *service)
{
	VCHIQ_STATE_T *state = service->state;
//  int flags;
//	spinlock_lock_disable_irq(&service_spinlock, flags);
	BUG(!service || (service->ref_count == 0), "ref_count == 0 (j)");
	if (service && service->ref_count) {
		service->ref_count--;
		if (!service->ref_count) {
			BUG(service->srvstate != VCHIQ_SRVSTATE_FREE, "srvstate wrong");
			state->services[service->localport] = NULL;
		} else
			service = NULL;
	}
//	spinlock_unlock_restore_irq(&service_spinlock, flags);

	if (service && service->userdata_term)
		service->userdata_term(service->base.userdata);

	// kfree(service);
}

int
vchiq_get_client_id(VCHIQ_SERVICE_HANDLE_T handle)
{
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	int id;

	id = service ? service->client_id : 0;
	if (service)
		unlock_service(service);

	return id;
}

void *
vchiq_get_service_userdata(VCHIQ_SERVICE_HANDLE_T handle)
{
	VCHIQ_SERVICE_T *service = handle_to_service(handle);

	return service ? service->base.userdata : NULL;
}

int
vchiq_get_service_fourcc(VCHIQ_SERVICE_HANDLE_T handle)
{
	VCHIQ_SERVICE_T *service = handle_to_service(handle);

	return service ? service->base.fourcc : 0;
}

static void
mark_service_closing_internal(VCHIQ_SERVICE_T *service, int sh_thread)
{
	VCHIQ_STATE_T *state = service->state;
	VCHIQ_SERVICE_QUOTA_T *service_quota;

	service->closing = 1;

	/* Synchronise with other threads. */
	mutex_lock(&state->recycle_mutex);
	mutex_unlock(&state->recycle_mutex);
	if (!sh_thread || (state->conn_state != VCHIQ_CONNSTATE_PAUSE_SENT)) {
		/* If we're pausing then the slot_mutex is held until resume
		 * by the slot handler.  Therefore don't try to acquire this
		 * mutex if we're the slot handler and in the pause sent state.
		 * We don't need to in this case anyway. */
		mutex_lock(&state->slot_mutex);
		mutex_unlock(&state->slot_mutex);
	}

	/* Unblock any sending thread. */
	service_quota = &state->service_quotas[service->localport];
	semaphore_up(&service_quota->quota_event);
}

static void
mark_service_closing(VCHIQ_SERVICE_T *service)
{
	mark_service_closing_internal(service, 0);
}

static inline VCHIQ_STATUS_T
make_service_callback(VCHIQ_SERVICE_T *service, VCHIQ_REASON_T reason,
	VCHIQ_HEADER_T *header, void *bulk_userdata)
{
	VCHIQ_STATUS_T status;
	vchiq_log_trace(vchiq_core_log_level, "%d: callback:%d (%s, %x, %x)",
		service->state->id, service->localport, reason_names[reason],
		(unsigned int)(uint64_t)header, (unsigned int)(uint64_t)bulk_userdata);
	status = service->base.callback(reason, header, service->handle,
		bulk_userdata);
	if (status == VCHIQ_ERROR) {
		vchiq_log_warning(vchiq_core_log_level,
			"%d: ignoring ERROR from callback to service %x",
			service->state->id, service->handle);
		status = VCHIQ_SUCCESS;
	}
	return status;
}

inline void
vchiq_set_conn_state(VCHIQ_STATE_T *state, VCHIQ_CONNSTATE_T newstate)
{
	VCHIQ_CONNSTATE_T oldstate = state->conn_state;
	vchiq_log_info(vchiq_core_log_level, "%d: %s->%s", state->id, conn_state_names[oldstate], conn_state_names[newstate]);
	state->conn_state = newstate;
}

static inline void
remote_event_destroy(REMOTE_EVENT_T *event)
{
	(void)event;
}

static inline int
remote_event_wait(REMOTE_EVENT_T *event)
{
	if (!event->fired) {
		event->armed = 1;
		dsb();
		if (!event->fired)
			semaphore_down((struct semaphore *)(uint64_t)event->event);
		event->armed = 0;
		wmb();
	}

	event->fired = 0;
	return 1;
}

//static inline void
//remote_event_poll(REMOTE_EVENT_T *event)
//{
//	if (event->fired && event->armed)
//		remote_event_signal_local(event);
//}
//
//void
//remote_event_pollall(VCHIQ_STATE_T *state)
//{
//	remote_event_poll(&state->local->sync_trigger);
//	remote_event_poll(&state->local->sync_release);
//	remote_event_poll(&state->local->trigger);
//	remote_event_poll(&state->local->recycle);
//}

/* Round up message sizes so that any space at the end of a slot is always big
** enough for a header. This relies on header size being a power of two, which
** has been verified earlier by a static assertion. */

static inline unsigned int
calc_stride(unsigned int size)
{
	/* Allow room for the header */
	size += sizeof(VCHIQ_HEADER_T);

	/* Round up */
	return (size + sizeof(VCHIQ_HEADER_T) - 1) & ~(sizeof(VCHIQ_HEADER_T)
		- 1);
}

/* Called by the slot handler thread */
static VCHIQ_SERVICE_T *
get_listening_service(VCHIQ_STATE_T *state, int fourcc)
{
	int i;

	BUG(fourcc == VCHIQ_FOURCC_INVALID, "fourcc == VCHIQ_FOURCC_INVALID");

	for (i = 0; i < state->unused_service; i++) {
		VCHIQ_SERVICE_T *service = state->services[i];
		if (service &&
			(service->public_fourcc == fourcc) &&
			((service->srvstate == VCHIQ_SRVSTATE_LISTENING) ||
			((service->srvstate == VCHIQ_SRVSTATE_OPEN) &&
			(service->remoteport == VCHIQ_PORT_FREE)))) {
			lock_service(service);
			return service;
		}
	}

	return NULL;
}

/* Called by the slot handler thread */
static VCHIQ_SERVICE_T *
get_connected_service(VCHIQ_STATE_T *state, unsigned int port)
{
	int i;
	for (i = 0; i < state->unused_service; i++) {
		VCHIQ_SERVICE_T *service = state->services[i];
		if (service && (service->srvstate == VCHIQ_SRVSTATE_OPEN)
			&& (service->remoteport == port)) {
			lock_service(service);
			return service;
		}
	}
	return NULL;
}

/* Called from queue_message, by the slot handler and application threads,
** with slot_mutex held */
static VCHIQ_HEADER_T *
reserve_space(VCHIQ_STATE_T *state, int space, int is_blocking)
{
	VCHIQ_SHARED_STATE_T *local = state->local;
	int tx_pos = state->local_tx_pos;
	int slot_space = VCHIQ_SLOT_SIZE - (tx_pos & VCHIQ_SLOT_MASK);

	if (space > slot_space) {
		VCHIQ_HEADER_T *header;
		/* Fill the remaining space with padding */
		BUG(state->tx_data == NULL, "state->tx_data == NULL");
		header = (VCHIQ_HEADER_T *)
			(state->tx_data + (tx_pos & VCHIQ_SLOT_MASK));
		header->msgid = VCHIQ_MSGID_PADDING;
		header->size = slot_space - sizeof(VCHIQ_HEADER_T);

		tx_pos += slot_space;
	}

	/* If necessary, get the next slot. */
	if ((tx_pos & VCHIQ_SLOT_MASK) == 0) {
		int slot_index;

		/* If there is no free slot... */

//		if (down_trylock(&state->slot_available_event) != 0) {
			/* ...wait for one. */

//			VCHIQ_STATS_INC(state, slot_stalls);
//
//			/* But first, flush through the last slot. */
//			state->local_tx_pos = tx_pos;
//			local->tx_pos = tx_pos;
//			remote_event_signal(&state->remote->trigger);
//
//			if (!is_blocking ||
//				(down_interruptible(
//				&state->slot_available_event) != 0))
//				return NULL; /* No space available */
//		}

		BUG(tx_pos == (state->slot_queue_available * VCHIQ_SLOT_SIZE), "tx_pos == (state->slot_queue_available * VCHIQ_SLOT_SIZE)");

		slot_index = local->slot_queue[
			SLOT_QUEUE_INDEX_FROM_POS(tx_pos) &
			VCHIQ_SLOT_QUEUE_MASK];
		state->tx_data =
			(char *)SLOT_DATA_FROM_INDEX(state, slot_index);
	}

	state->local_tx_pos = tx_pos + space;

	return (VCHIQ_HEADER_T *)(state->tx_data + (tx_pos & VCHIQ_SLOT_MASK));
}

/* Called by the recycle thread. */
static void
process_free_queue(VCHIQ_STATE_T *state)
{
	VCHIQ_SHARED_STATE_T *local = state->local;
	BITSET_T service_found[BITSET_SIZE(VCHIQ_MAX_SERVICES)];
	int slot_queue_available;
//  int flags;

	/* Find slots which have been freed by the other side, and return them
	** to the available queue. */
	slot_queue_available = state->slot_queue_available;

	/* Use a memory barrier to ensure that any state that may have been
	** modified by another thread is not masked by stale prefetched
	** values. */
	mb();

	while (slot_queue_available != local->slot_queue_recycle) {
		unsigned int pos;
		int slot_index = local->slot_queue[slot_queue_available++ &
			VCHIQ_SLOT_QUEUE_MASK];
		char *data = (char *)SLOT_DATA_FROM_INDEX(state, slot_index);
		int data_found = 0;

		rmb();

		vchiq_log_trace(vchiq_core_log_level, "%d: pfq %d=%x %x %x",
			state->id, slot_index, (unsigned int)(uint64_t)data,
			local->slot_queue_recycle, slot_queue_available);

		/* Initialise the bitmask for services which have used this
		** slot */
		BITSET_ZERO(service_found);

		pos = 0;

		while (pos < VCHIQ_SLOT_SIZE) {
			VCHIQ_HEADER_T *header =
				(VCHIQ_HEADER_T *)(data + pos);
			int msgid = header->msgid;
			if (VCHIQ_MSG_TYPE(msgid) == VCHIQ_MSG_DATA) {
				int port = VCHIQ_MSG_SRCPORT(msgid);
				VCHIQ_SERVICE_QUOTA_T *service_quota =
					&state->service_quotas[port];
				int count;
//				spinlock_lock_disable_irq(&quota_spinlock, flags);
				count = service_quota->message_use_count;
				if (count > 0)
					service_quota->message_use_count =
						count - 1;
//				spinlock_unlock_restore_irq(&quota_spinlock, flags);

				if (count == service_quota->message_quota)
					/* Signal the service that it
					** has dropped below its quota
					*/
					semaphore_up(&service_quota->quota_event);
				else if (count == 0) {
					vchiq_log_error(vchiq_core_log_level,
						"service %d "
						"message_use_count=%d "
						"(header %x, msgid %x, "
						"header->msgid %x, "
						"header->size %x)",
						port,
						service_quota->
							message_use_count,
						(unsigned int)(uint64_t)header, msgid,
						header->msgid,
						header->size);
					BUG(1, "invalid message use count\n");
				}
				if (!BITSET_IS_SET(service_found, port)) {
					/* Set the found bit for this service */
					BITSET_SET(service_found, port);

//					spinlock_lock_disable_irq(&quota_spinlock, flags);
					count = service_quota->slot_use_count;
					if (count > 0)
						service_quota->slot_use_count =
							count - 1;
//					spinlock_unlock_restore_irq(&quota_spinlock, flags);

					if (count > 0) {
						/* Signal the service in case
						** it has dropped below its
						** quota */
						semaphore_up(&service_quota->quota_event);
						vchiq_log_trace(
							vchiq_core_log_level,
							"%d: pfq:%d %x@%x - "
							"slot_use->%d",
							state->id, port,
							header->size,
							(unsigned int)(uint64_t)header,
							count - 1);
					} else {
						vchiq_log_error(
							vchiq_core_log_level,
								"service %d "
								"slot_use_count"
								"=%d (header %x"
								", msgid %x, "
								"header->msgid"
								" %x, header->"
								"size %x)",
							port, count,
							(unsigned int)(uint64_t)header,
							msgid,
							header->msgid,
							header->size);
						BUG(1, "bad slot use count\n");
					}
				}

				data_found = 1;
			}

			pos += calc_stride(header->size);
			if (pos > VCHIQ_SLOT_SIZE) {
				vchiq_log_error(vchiq_core_log_level,
					"pfq - pos %x: header %x, msgid %x, "
					"header->msgid %x, header->size %x",
					pos, (unsigned int)(uint64_t)header, msgid,
					header->msgid, header->size);
				BUG(1, "invalid slot position\n");
			}
		}

		if (data_found) {
			int count;
//			spinlock_lock_disable_irq(&quota_spinlock, flags);
			count = state->data_use_count;
			if (count > 0)
				state->data_use_count =
					count - 1;
//			spinlock_unlock_restore_irq(&quota_spinlock, flags);
			if (count == state->data_quota)
				semaphore_up(&state->data_quota_event);
		}

		mb();

		state->slot_queue_available = slot_queue_available;
		semaphore_up(&state->slot_available_event);
	}
}

/* Called by the slot handler and application threads */
static VCHIQ_STATUS_T
queue_message(VCHIQ_STATE_T *state, VCHIQ_SERVICE_T *service,
	int msgid, const VCHIQ_ELEMENT_T *elements,
	int count, int size, int flags)
{
	VCHIQ_SHARED_STATE_T *local;
	VCHIQ_SERVICE_QUOTA_T *service_quota = NULL;
	VCHIQ_HEADER_T *header;
	int type = VCHIQ_MSG_TYPE(msgid);
  int irqflags;

	unsigned int stride;

	local = state->local;

	stride = calc_stride(size);

	BUG(!(stride <= VCHIQ_SLOT_SIZE), "!(stride <= VCHIQ_SLOT_SIZE)");

//	if (!(flags & QMFLAGS_NO_MUTEX_LOCK) &&
	//	(mutex_lock_interruptible(&state->slot_mutex) != 0))
		//return VCHIQ_RETRY;

	if (type == VCHIQ_MSG_DATA) {
		int tx_end_index;

		BUG(!service, "!service");
//		BUG((flags & (QMFLAGS_NO_MUTEX_LOCK |
	//			 QMFLAGS_NO_MUTEX_UNLOCK)) != 0, "flags & (QMFLAGS_NO_MUTEX_LOCK");

		if (service->closing) {
			/* The service has been closed */
	//		mutex_unlock(&state->slot_mutex);
			return VCHIQ_ERROR;
		}

		service_quota = &state->service_quotas[service->localport];

//		spinlock_lock_disable_irq(&quota_spinlock, irqflags);

		/* Ensure this service doesn't use more than its quota of
		** messages or slots */
		tx_end_index = SLOT_QUEUE_INDEX_FROM_POS(
			state->local_tx_pos + stride - 1);

		/* Ensure data messages don't use more than their quota of
		** slots */
		while ((tx_end_index != state->previous_data_index) &&
			(state->data_use_count == state->data_quota)) {
			VCHIQ_STATS_INC(state, data_stalls);
		//	spinlock_unlock_restore_irq(&quota_spinlock, irqflags);
			//mutex_unlock(&state->slot_mutex);

//			if (down_interruptible(&state->data_quota_event)
	//			!= 0)
		//		return VCHIQ_RETRY;

//			mutex_lock(&state->slot_mutex);
	//		spinlock_lock_disable_irq(&quota_spinlock, irqflags);
			tx_end_index = SLOT_QUEUE_INDEX_FROM_POS(
				state->local_tx_pos + stride - 1);
			if ((tx_end_index == state->previous_data_index) ||
				(state->data_use_count < state->data_quota)) {
				/* Pass the signal on to other waiters */
	//			semaphore_up(&state->data_quota_event);
				break;
			}
		}

		while ((service_quota->message_use_count ==
				service_quota->message_quota) ||
			((tx_end_index != service_quota->previous_tx_index) &&
			(service_quota->slot_use_count ==
				service_quota->slot_quota))) {
	//		spinlock_unlock_restore_irq(&quota_spinlock, irqflags);
			vchiq_log_trace(vchiq_core_log_level,
				"%d: qm:%d %s,%x - quota stall "
				"(msg %d, slot %d)",
				state->id, service->localport,
				msg_type_str(type), size,
				service_quota->message_use_count,
				service_quota->slot_use_count);
			VCHIQ_SERVICE_STATS_INC(service, quota_stalls);
	//		mutex_unlock(&state->slot_mutex);
		//	if (down_interruptible(&service_quota->quota_event)
			//	!= 0)
				//return VCHIQ_RETRY;
			if (service->closing)
				return VCHIQ_ERROR;
//			if (mutex_lock_interruptible(&state->slot_mutex) != 0)
	//			return VCHIQ_RETRY;
			if (service->srvstate != VCHIQ_SRVSTATE_OPEN) {
				/* The service has been closed */
		//		mutex_unlock(&state->slot_mutex);
				return VCHIQ_ERROR;
			}
	//		spinlock_unlock_restore_irq(&quota_spinlock, irqflags);
			tx_end_index = SLOT_QUEUE_INDEX_FROM_POS(
				state->local_tx_pos + stride - 1);
		}

//		spinlock_unlock_restore_irq(&quota_spinlock, irqflags);
	}

	header = reserve_space(state, stride, flags & QMFLAGS_IS_BLOCKING);

	if (!header) {
		if (service)
			VCHIQ_SERVICE_STATS_INC(service, slot_stalls);
		/* In the event of a failure, return the mutex to the
		   state it was in */
		if (!(flags & QMFLAGS_NO_MUTEX_LOCK))
			mutex_unlock(&state->slot_mutex);
		return VCHIQ_RETRY;
	}

	if (type == VCHIQ_MSG_DATA) {
		int i, pos;
		int tx_end_index;
		int slot_use_count;

		vchiq_log_info(vchiq_core_log_level,
			"%d: qm %s@%x,%x (%d->%d)",
			state->id,
			msg_type_str(VCHIQ_MSG_TYPE(msgid)),
			(unsigned int)(uint64_t)header, size,
			VCHIQ_MSG_SRCPORT(msgid),
			VCHIQ_MSG_DSTPORT(msgid));

		BUG(!service, "!service");
		BUG((flags & (QMFLAGS_NO_MUTEX_LOCK |
				 QMFLAGS_NO_MUTEX_UNLOCK)) != 0, "flags & (QMFLAGS_NO_MUTEX_LOCK");

		for (i = 0, pos = 0; i < (unsigned int)count;
			pos += elements[i++].size)
			if (elements[i].size) {
//				if (vchiq_copy_from_user
//					(header->data + pos, elements[i].data,
//					(size_t) elements[i].size) !=
//					VCHIQ_SUCCESS) {
//					mutex_unlock(&state->slot_mutex);
//					VCHIQ_SERVICE_STATS_INC(service,
//						error_count);
//					return VCHIQ_ERROR;
//				}
			}

		if (SRVTRACE_ENABLED(service,
				VCHIQ_LOG_INFO))
			vchiq_log_dump_mem("Sent", 0,
				header->data,
				min(16, pos));

		spinlock_lock_disable_irq(&quota_spinlock, irqflags);
		service_quota->message_use_count++;

		tx_end_index =
			SLOT_QUEUE_INDEX_FROM_POS(state->local_tx_pos - 1);

		/* If this transmission can't fit in the last slot used by any
		** service, the data_use_count must be increased. */
		if (tx_end_index != state->previous_data_index) {
			state->previous_data_index = tx_end_index;
			state->data_use_count++;
		}

		/* If this isn't the same slot last used by this service,
		** the service's slot_use_count must be increased. */
		if (tx_end_index != service_quota->previous_tx_index) {
			service_quota->previous_tx_index = tx_end_index;
			slot_use_count = ++service_quota->slot_use_count;
		} else {
			slot_use_count = 0;
		}

		spinlock_unlock_restore_irq(&quota_spinlock, irqflags);

		if (slot_use_count)
			vchiq_log_trace(vchiq_core_log_level,
				"%d: qm:%d %s,%x - slot_use->%d (hdr %p)",
				state->id, service->localport,
				msg_type_str(VCHIQ_MSG_TYPE(msgid)), size,
				slot_use_count, header);

		VCHIQ_SERVICE_STATS_INC(service, ctrl_tx_count);
		VCHIQ_SERVICE_STATS_ADD(service, ctrl_tx_bytes, size);
	} else {
		vchiq_log_info(vchiq_core_log_level,
			"%d: qm %s@%x,%x (%d->%d)", state->id,
			msg_type_str(VCHIQ_MSG_TYPE(msgid)),
			(unsigned int)(uint64_t)header, size,
			VCHIQ_MSG_SRCPORT(msgid),
			VCHIQ_MSG_DSTPORT(msgid));
		if (size != 0) {
			BUG(!((count == 1) && (size == elements[0].size)), "--");
			memcpy(header->data, elements[0].data,
				elements[0].size);
		}
		VCHIQ_STATS_INC(state, ctrl_tx_count);
	}

	header->msgid = msgid;
	header->size = size;

	{
		int svc_fourcc;

		svc_fourcc = service
			? service->base.fourcc
			: VCHIQ_MAKE_FOURCC('?', '?', '?', '?');

		vchiq_log_info(SRVTRACE_LEVEL(service),
			"Sent Msg %s(%u) to %c%c%c%c s:%u d:%d len:%d",
			msg_type_str(VCHIQ_MSG_TYPE(msgid)),
			VCHIQ_MSG_TYPE(msgid),
			VCHIQ_FOURCC_AS_4CHARS(svc_fourcc),
			VCHIQ_MSG_SRCPORT(msgid),
			VCHIQ_MSG_DSTPORT(msgid),
			size);
	}

	/* Make sure the new header is visible to the peer. */
	wmb();

	/* Make the new tx_pos visible to the peer. */
	local->tx_pos = state->local_tx_pos;
	wmb();

	if (service && (type == VCHIQ_MSG_CLOSE))
		vchiq_set_service_state(service, VCHIQ_SRVSTATE_CLOSESENT);

	if (!(flags & QMFLAGS_NO_MUTEX_UNLOCK))
		mutex_unlock(&state->slot_mutex);

	remote_event_signal(&state->remote->trigger);

	return VCHIQ_SUCCESS;
}

/* Called by the slot handler and application threads */
static VCHIQ_STATUS_T
queue_message_sync(VCHIQ_STATE_T *state, VCHIQ_SERVICE_T *service,
	int msgid, const VCHIQ_ELEMENT_T *elements,
	int count, int size, int is_blocking)
{
	VCHIQ_SHARED_STATE_T *local;
	VCHIQ_HEADER_T *header;

	local = state->local;

	if ((VCHIQ_MSG_TYPE(msgid) != VCHIQ_MSG_RESUME) &&
		(mutex_lock_interruptible(&state->sync_mutex) != 0))
		return VCHIQ_RETRY;

	remote_event_wait(&local->sync_release);

	rmb();

	header = (VCHIQ_HEADER_T *)SLOT_DATA_FROM_INDEX(state, local->slot_sync);

	{
		int oldmsgid = header->msgid;
		if (oldmsgid != VCHIQ_MSGID_PADDING)
			vchiq_log_error(vchiq_core_log_level,
				"%d: qms - msgid %x, not PADDING",
				state->id, oldmsgid);
	}

	if (service) {
		int i, pos;

		for (i = 0, pos = 0; i < (unsigned int)count; pos += elements[i++].size)
			if (elements[i].size) { }
  }

	header->size = size;
	header->msgid = msgid;

	/* Make sure the new header is visible to the peer. */
	wmb();

	remote_event_signal(&state->remote->sync_trigger);

	if (VCHIQ_MSG_TYPE(msgid) != VCHIQ_MSG_PAUSE)
		mutex_unlock(&state->sync_mutex);

	return VCHIQ_SUCCESS;
}

static inline void
claim_slot(VCHIQ_SLOT_INFO_T *slot)
{
	slot->use_count++;
}

static void
release_slot(VCHIQ_STATE_T *state, VCHIQ_SLOT_INFO_T *slot_info,
	VCHIQ_HEADER_T *header, VCHIQ_SERVICE_T *service)
{
	int release_count;

	mutex_lock(&state->recycle_mutex);

	if (header) {
		int msgid = header->msgid;
		if (((msgid & VCHIQ_MSGID_CLAIMED) == 0) ||
			(service && service->closing)) {
			mutex_unlock(&state->recycle_mutex);
			return;
		}

		/* Rewrite the message header to prevent a double
		** release */
		header->msgid = msgid & ~VCHIQ_MSGID_CLAIMED;
	}

	release_count = slot_info->release_count;
	slot_info->release_count = ++release_count;

	if (release_count == slot_info->use_count) {
		int slot_queue_recycle;
		/* Add to the freed queue */

		/* A read barrier is necessary here to prevent speculative
		** fetches of remote->slot_queue_recycle from overtaking the
		** mutex. */
		rmb();

		slot_queue_recycle = state->remote->slot_queue_recycle;
		state->remote->slot_queue[slot_queue_recycle &
			VCHIQ_SLOT_QUEUE_MASK] =
			SLOT_INDEX_FROM_INFO(state, slot_info);
		state->remote->slot_queue_recycle = slot_queue_recycle + 1;
		vchiq_log_info(vchiq_core_log_level,
			"%d: release_slot %d - recycle->%x",
			state->id, SLOT_INDEX_FROM_INFO(state, slot_info),
			state->remote->slot_queue_recycle);

		/* A write barrier is necessary, but remote_event_signal
		** contains one. */
		remote_event_signal(&state->remote->recycle);
	}

	mutex_unlock(&state->recycle_mutex);
}

/* Called by the slot handler - don't hold the bulk mutex */
static VCHIQ_STATUS_T
notify_bulks(VCHIQ_SERVICE_T *service, VCHIQ_BULK_QUEUE_T *queue,
	int retry_poll)
{
  int irqflags;
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;

	vchiq_log_trace(vchiq_core_log_level,
		"%d: nb:%d %cx - p=%x rn=%x r=%x",
		service->state->id, service->localport,
		(queue == &service->bulk_tx) ? 't' : 'r',
		queue->process, queue->remote_notify, queue->remove);

	if (service->state->is_master) {
		while (queue->remote_notify != queue->process) {
			VCHIQ_BULK_T *bulk =
				&queue->bulks[BULK_INDEX(queue->remote_notify)];
			int msgtype = (bulk->dir == VCHIQ_BULK_TRANSMIT) ?
				VCHIQ_MSG_BULK_RX_DONE : VCHIQ_MSG_BULK_TX_DONE;
			int msgid = VCHIQ_MAKE_MSG(msgtype, service->localport,
				service->remoteport);
			VCHIQ_ELEMENT_T element = { &bulk->actual, 4 };
			/* Only reply to non-dummy bulk requests */
			if (bulk->remote_data) {
				status = queue_message(service->state, NULL,
					msgid, &element, 1, 4, 0);
				if (status != VCHIQ_SUCCESS)
					break;
			}
			queue->remote_notify++;
		}
	} else {
		queue->remote_notify = queue->process;
	}

	if (status == VCHIQ_SUCCESS) {
		while (queue->remove != queue->remote_notify) {
			VCHIQ_BULK_T *bulk =
				&queue->bulks[BULK_INDEX(queue->remove)];

			/* Only generate callbacks for non-dummy bulk
			** requests, and non-terminated services */
			if (bulk->data && service->instance) {
				if (bulk->actual != VCHIQ_BULK_ACTUAL_ABORTED) {
					if (bulk->dir == VCHIQ_BULK_TRANSMIT) {
						VCHIQ_SERVICE_STATS_INC(service,
							bulk_tx_count);
						VCHIQ_SERVICE_STATS_ADD(service,
							bulk_tx_bytes,
							bulk->actual);
					} else {
						VCHIQ_SERVICE_STATS_INC(service,
							bulk_rx_count);
						VCHIQ_SERVICE_STATS_ADD(service,
							bulk_rx_bytes,
							bulk->actual);
					}
				} else {
					VCHIQ_SERVICE_STATS_INC(service,
						bulk_aborted_count);
				}
				if (bulk->mode == VCHIQ_BULK_MODE_BLOCKING) {
					struct bulk_waiter *waiter;
					spinlock_lock_disable_irq(&bulk_waiter_spinlock, irqflags);
					waiter = bulk->userdata;
					if (waiter) {
						waiter->actual = bulk->actual;
						semaphore_up(&waiter->event);
					}
					spinlock_unlock_restore_irq(&bulk_waiter_spinlock, irqflags);
				} else if (bulk->mode ==
					VCHIQ_BULK_MODE_CALLBACK) {
					VCHIQ_REASON_T reason = (bulk->dir ==
						VCHIQ_BULK_TRANSMIT) ?
						((bulk->actual ==
						VCHIQ_BULK_ACTUAL_ABORTED) ?
						VCHIQ_BULK_TRANSMIT_ABORTED :
						VCHIQ_BULK_TRANSMIT_DONE) :
						((bulk->actual ==
						VCHIQ_BULK_ACTUAL_ABORTED) ?
						VCHIQ_BULK_RECEIVE_ABORTED :
						VCHIQ_BULK_RECEIVE_DONE);
					status = make_service_callback(service,
						reason,	NULL, bulk->userdata);
					if (status == VCHIQ_RETRY)
						break;
				}
			}

			queue->remove++;
			semaphore_up(&service->bulk_remove_event);
		}
		if (!retry_poll)
			status = VCHIQ_SUCCESS;
	}

	return status;
}

/* Called by the slot handler thread */
static void
poll_services(VCHIQ_STATE_T *state)
{
}

/* Called from the slot handler thread */
static void
pause_bulks(VCHIQ_STATE_T *state)
{
//	if (unlikely(atomic_inc_return(&pause_bulks_count) != 1)) {
    BUG(1, "---");
	//	atomic_set(&pause_bulks_count, 1);
//		return;
//	}

	/* Block bulk transfers from all services */
	mutex_lock(&state->bulk_transfer_mutex);
}

static int
parse_open(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header)
{
	VCHIQ_SERVICE_T *service = NULL;
	int msgid, size;
	/* int type; */
	unsigned int remoteport;
  unsigned int localport;

	msgid = header->msgid;
	size = header->size;
	/* type = VCHIQ_MSG_TYPE(msgid); */
	localport = VCHIQ_MSG_DSTPORT(msgid);
  if (localport)
    kernel_panic("localport");
	remoteport = VCHIQ_MSG_SRCPORT(msgid);
	if (size >= sizeof(struct vchiq_open_payload)) {
		const struct vchiq_open_payload *payload = (struct vchiq_open_payload *)header->data;
		unsigned int fourcc;

		fourcc = payload->fourcc;
		service = get_listening_service(state, fourcc);

		if (service) {
			/* A matching service exists */
			short version = payload->version;
			short version_min = payload->version_min;
			if ((service->version < version_min) ||
				(version < service->version_min)) {
				/* Version mismatch */
				vchiq_loud_error_header();
				vchiq_loud_error("%d: service %d (%c%c%c%c) "
					"version mismatch - local (%d, min %d)"
					" vs. remote (%d, min %d)",
					state->id, service->localport,
					VCHIQ_FOURCC_AS_4CHARS(fourcc),
					service->version, service->version_min,
					version, version_min);
				vchiq_loud_error_footer();
				unlock_service(service);
				service = NULL;
				goto fail_open;
			}
			service->peer_version = version;

			if (service->srvstate == VCHIQ_SRVSTATE_LISTENING) {
				struct vchiq_openack_payload ack_payload = {
					service->version
				};
				VCHIQ_ELEMENT_T body = {
					&ack_payload,
					sizeof(ack_payload)
				};

				if (state->version_common <
				    VCHIQ_VERSION_SYNCHRONOUS_MODE)
					service->sync = 0;

				/* Acknowledge the OPEN */
				if (service->sync &&
				    (state->version_common >=
				     VCHIQ_VERSION_SYNCHRONOUS_MODE)) {
					if (queue_message_sync(state, NULL,
						VCHIQ_MAKE_MSG( VCHIQ_MSG_OPENACK, service->localport, remoteport), &body, 1, sizeof(ack_payload), 0) == VCHIQ_RETRY)
						goto bail_not_ready;
				} else {
					if (queue_message(state, NULL, VCHIQ_MAKE_MSG( VCHIQ_MSG_OPENACK, service->localport, remoteport), &body, 1, sizeof(ack_payload), 0) == VCHIQ_RETRY)
						goto bail_not_ready;
				}

				/* The service is now open */
				vchiq_set_service_state(service, service->sync ? VCHIQ_SRVSTATE_OPENSYNC : VCHIQ_SRVSTATE_OPEN);
			}

			service->remoteport = remoteport;
			service->client_id = ((int *)header->data)[1];
			if (make_service_callback(service, VCHIQ_SERVICE_OPENED,
				NULL, NULL) == VCHIQ_RETRY) {
				/* Bail out if not ready */
				service->remoteport = VCHIQ_PORT_FREE;
				goto bail_not_ready;
			}

			/* Success - the message has been dealt with */
			unlock_service(service);
			return 1;
		}
	}

fail_open:
	/* No available service, or an invalid request - send a CLOSE */
	if (queue_message(state, NULL,
		VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, 0, VCHIQ_MSG_SRCPORT(msgid)),
		NULL, 0, 0, 0) == VCHIQ_RETRY)
		goto bail_not_ready;

	return 1;

bail_not_ready:
	if (service)
		unlock_service(service);

	return 0;
}



/* Called by the sync thread */

static void
init_bulk_queue(VCHIQ_BULK_QUEUE_T *queue)
{
	queue->local_insert = 0;
	queue->remote_insert = 0;
	queue->process = 0;
	queue->remote_notify = 0;
	queue->remove = 0;
}


inline const char *
get_conn_state_name(VCHIQ_CONNSTATE_T conn_state)
{
	return conn_state_names[conn_state];
}


VCHIQ_SLOT_ZERO_T *
vchiq_init_slots(void *mem_base, int mem_size)
{
	int mem_align = (VCHIQ_SLOT_SIZE - (int)(uint64_t)mem_base) & VCHIQ_SLOT_MASK;
	VCHIQ_SLOT_ZERO_T *slot_zero = (VCHIQ_SLOT_ZERO_T *)((char *)mem_base + mem_align);
	int num_slots = (mem_size - mem_align)/VCHIQ_SLOT_SIZE;
	int first_data_slot = VCHIQ_SLOT_ZERO_SLOTS;

	vslotz = mem_base;
	vslot = mem_base;

	/* Ensure there is enough memory to run an absolutely minimum system */
	num_slots -= first_data_slot;

	if (num_slots < 4) {
		vchiq_log_error(vchiq_core_log_level,
			"vchiq_init_slots - insufficient memory %x bytes",
			mem_size);
		return NULL;
	}

	memset(slot_zero, 0, sizeof(VCHIQ_SLOT_ZERO_T));

	slot_zero->magic = VCHIQ_MAGIC;
	slot_zero->version = VCHIQ_VERSION;
	slot_zero->version_min = VCHIQ_VERSION_MIN;
	slot_zero->slot_zero_size = sizeof(VCHIQ_SLOT_ZERO_T);
	slot_zero->slot_size = VCHIQ_SLOT_SIZE;
	slot_zero->max_slots = VCHIQ_MAX_SLOTS;
	slot_zero->max_slots_per_side = VCHIQ_MAX_SLOTS_PER_SIDE;

	slot_zero->master.slot_sync = first_data_slot;
	slot_zero->master.slot_first = first_data_slot + 1;
	slot_zero->master.slot_last = first_data_slot + (num_slots/2) - 1;
	slot_zero->slave.slot_sync = first_data_slot + (num_slots/2);
	slot_zero->slave.slot_first = first_data_slot + (num_slots/2) + 1;
	slot_zero->slave.slot_last = first_data_slot + num_slots - 1;

	return slot_zero;
}

VCHIQ_STATUS_T
vchiq_init_state_run_checks(
	VCHIQ_STATE_T *state,
	VCHIQ_SLOT_ZERO_T *slot_zero)
{
	if (slot_zero->magic != VCHIQ_MAGIC) {
		vchiq_loud_error_header();
		vchiq_loud_error("Invalid VCHIQ magic value found.");
		vchiq_loud_error("slot_zero=%x: magic=%x (expected %x)",
			(unsigned int)(uint64_t)slot_zero, slot_zero->magic, VCHIQ_MAGIC);
		vchiq_loud_error_footer();
		return VCHIQ_ERROR;
	}

	if (slot_zero->version < VCHIQ_VERSION_MIN) {
		vchiq_loud_error_header();
		vchiq_loud_error("Incompatible VCHIQ versions found.");
		vchiq_loud_error("slot_zero=%x: VideoCore version=%d "
			"(minimum %d)",
			(unsigned int)(uint64_t)slot_zero, slot_zero->version,
			VCHIQ_VERSION_MIN);
		vchiq_loud_error("Restart with a newer VideoCore image.");
		vchiq_loud_error_footer();
		return VCHIQ_ERROR;
	}

	if (VCHIQ_VERSION < slot_zero->version_min) {
		vchiq_loud_error_header();
		vchiq_loud_error("Incompatible VCHIQ versions found.");
		vchiq_loud_error("slot_zero=%x: version=%d (VideoCore "
			"minimum %d)",
			(unsigned int)(uint64_t)slot_zero, VCHIQ_VERSION,
			slot_zero->version_min);
		vchiq_loud_error("Restart with a newer kernel.");
		vchiq_loud_error_footer();
		return VCHIQ_ERROR;
	}

	if ((slot_zero->slot_zero_size != sizeof(VCHIQ_SLOT_ZERO_T)) ||
		 (slot_zero->slot_size != VCHIQ_SLOT_SIZE) ||
		 (slot_zero->max_slots != VCHIQ_MAX_SLOTS) ||
		 (slot_zero->max_slots_per_side != VCHIQ_MAX_SLOTS_PER_SIDE)) {
		vchiq_loud_error_header();
		if (slot_zero->slot_zero_size != sizeof(VCHIQ_SLOT_ZERO_T))
			vchiq_loud_error("slot_zero=%x: slot_zero_size=%x "
				"(expected %x)",
				(unsigned int)(uint64_t)slot_zero,
				slot_zero->slot_zero_size,
				sizeof(VCHIQ_SLOT_ZERO_T));
		if (slot_zero->slot_size != VCHIQ_SLOT_SIZE)
			vchiq_loud_error("slot_zero=%x: slot_size=%d "
				"(expected %d",
				(unsigned int)(uint64_t)slot_zero, slot_zero->slot_size,
				VCHIQ_SLOT_SIZE);
		if (slot_zero->max_slots != VCHIQ_MAX_SLOTS)
			vchiq_loud_error("slot_zero=%x: max_slots=%d "
				"(expected %d)",
				(unsigned int)(uint64_t)slot_zero, slot_zero->max_slots,
				VCHIQ_MAX_SLOTS);
		if (slot_zero->max_slots_per_side != VCHIQ_MAX_SLOTS_PER_SIDE)
			vchiq_loud_error("slot_zero=%x: max_slots_per_side=%d "
				"(expected %d)",
				(unsigned int)(uint64_t)slot_zero,
				slot_zero->max_slots_per_side,
				VCHIQ_MAX_SLOTS_PER_SIDE);
		vchiq_loud_error_footer();
		return VCHIQ_ERROR;
	}

	if (VCHIQ_VERSION < slot_zero->version)
		slot_zero->version = VCHIQ_VERSION;

  return VCHIQ_SUCCESS;
}

/* Called from application thread when a client or server service is created. */
VCHIQ_SERVICE_T *
vchiq_add_service_internal(VCHIQ_STATE_T *state,
	const VCHIQ_SERVICE_PARAMS_T *params, int srvstate,
	VCHIQ_INSTANCE_T instance, VCHIQ_USERDATA_TERM_T userdata_term)
{
	VCHIQ_SERVICE_T *service;

//	service = kmalloc(sizeof(VCHIQ_SERVICE_T), GFP_KERNEL);
  service = &vservice;
	if (service) {
		service->base.fourcc   = params->fourcc;
		service->base.callback = params->callback;
		service->base.userdata = params->userdata;
		service->handle        = VCHIQ_SERVICE_HANDLE_INVALID;
		service->ref_count     = 1;
		service->srvstate      = VCHIQ_SRVSTATE_FREE;
		service->userdata_term = userdata_term;
		service->localport     = VCHIQ_PORT_FREE;
		service->remoteport    = VCHIQ_PORT_FREE;

		service->public_fourcc = (srvstate == VCHIQ_SRVSTATE_OPENING) ?
			VCHIQ_FOURCC_INVALID : params->fourcc;
		service->client_id     = 0;
		service->auto_close    = 1;
		service->sync          = 0;
		service->closing       = 0;
		service->trace         = 0;
		atomic_set(&service->poll_flags, 0);
		service->version       = params->version;
		service->version_min   = params->version_min;
		service->state         = state;
		service->instance      = instance;
		service->service_use_count = 0;
		init_bulk_queue(&service->bulk_tx);
		init_bulk_queue(&service->bulk_rx);
		semaphore_init(&service->remove_event, 0);
		semaphore_init(&service->bulk_remove_event, 0);
		mutex_init(&service->bulk_mutex);
		memset(&service->stats, 0, sizeof(service->stats));
	} else {
		vchiq_log_error(vchiq_core_log_level,
			"Out of memory");
	}

	if (service) {
		VCHIQ_SERVICE_T **pservice = NULL;
		int i;

		/* Although it is perfectly possible to use service_spinlock
		** to protect the creation of services, it is overkill as it
		** disables interrupts while the array is searched.
		** The only danger is of another thread trying to create a
		** service - service deletion is safe.
		** Therefore it is preferable to use state->mutex which,
		** although slower to claim, doesn't block interrupts while
		** it is held.
		*/

		mutex_lock(&state->mutex);

		/* Prepare to use a previously unused service */
		if (state->unused_service < VCHIQ_MAX_SERVICES)
			pservice = &state->services[state->unused_service];

		if (srvstate == VCHIQ_SRVSTATE_OPENING) {
			for (i = 0; i < state->unused_service; i++) {
				VCHIQ_SERVICE_T *srv = state->services[i];
				if (!srv) {
					pservice = &state->services[i];
					break;
				}
			}
		} else {
			for (i = (state->unused_service - 1); i >= 0; i--) {
				VCHIQ_SERVICE_T *srv = state->services[i];
				if (!srv)
					pservice = &state->services[i];
				else if ((srv->public_fourcc == params->fourcc)
					&& ((srv->instance != instance) ||
					(srv->base.callback !=
					params->callback))) {
					/* There is another server using this
					** fourcc which doesn't match. */
					pservice = NULL;
					break;
				}
			}
		}

		if (pservice) {
			service->localport = (pservice - state->services);
			if (!handle_seq)
				handle_seq = VCHIQ_MAX_STATES *
					 VCHIQ_MAX_SERVICES;
			service->handle = handle_seq |
				(state->id * VCHIQ_MAX_SERVICES) |
				service->localport;
			handle_seq += VCHIQ_MAX_STATES * VCHIQ_MAX_SERVICES;
			*pservice = service;
			if (pservice == &state->services[state->unused_service])
				state->unused_service++;
		}

		mutex_unlock(&state->mutex);

		if (!pservice) {
			//kfree(service);
			service = NULL;
		}
	}

	if (service) {
		VCHIQ_SERVICE_QUOTA_T *service_quota =
			&state->service_quotas[service->localport];
		service_quota->slot_quota = state->default_slot_quota;
		service_quota->message_quota = state->default_message_quota;
		if (service_quota->slot_use_count == 0)
			service_quota->previous_tx_index =
				SLOT_QUEUE_INDEX_FROM_POS(state->local_tx_pos)
				- 1;

		/* Bring this service online */
		vchiq_set_service_state(service, srvstate);

		vchiq_log_info(vchiq_core_msg_log_level,
			"%s Service %c%c%c%c SrcPort:%d",
			(srvstate == VCHIQ_SRVSTATE_OPENING)
			? "Open" : "Add",
			VCHIQ_FOURCC_AS_4CHARS(params->fourcc),
			service->localport);
	}

	/* Don't unlock the service - leave it with a ref_count of 1. */

	return service;
}

VCHIQ_STATUS_T
vchiq_open_service_internal(VCHIQ_SERVICE_T *service, int client_id)
{
	struct vchiq_open_payload payload = {
		service->base.fourcc,
		client_id,
		service->version,
		service->version_min
	};
	VCHIQ_ELEMENT_T body = { &payload, sizeof(payload) };
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;

	service->client_id = client_id;
	// vchiq_use_service_internal(service);
	status = queue_message(service->state, NULL,
		VCHIQ_MAKE_MSG(VCHIQ_MSG_OPEN, service->localport, 0),
		&body, 1, sizeof(payload), QMFLAGS_IS_BLOCKING);
	if (status == VCHIQ_SUCCESS) {
		/* Wait for the ACK/NAK */
		semaphore_down(&service->remove_event);
		if ((service->srvstate != VCHIQ_SRVSTATE_OPEN) && (service->srvstate != VCHIQ_SRVSTATE_OPENSYNC)) {
			if (service->srvstate != VCHIQ_SRVSTATE_CLOSEWAIT)
				vchiq_log_error(vchiq_core_log_level, "%d: osi - srvstate = %s (ref %d)", service->state->id, srvstate_names[service->srvstate], service->ref_count);
			status = VCHIQ_ERROR;
			VCHIQ_SERVICE_STATS_INC(service, error_count);
			// vchiq_release_service_internal(service);
		}
	}
	return status;
}

/* Called from the slot handler */
void
vchiq_free_service_internal(VCHIQ_SERVICE_T *service)
{
	VCHIQ_STATE_T *state = service->state;

	vchiq_log_info(vchiq_core_log_level, "%d: fsi - (%d)",
		state->id, service->localport);

	switch (service->srvstate) {
	case VCHIQ_SRVSTATE_OPENING:
	case VCHIQ_SRVSTATE_CLOSED:
	case VCHIQ_SRVSTATE_HIDDEN:
	case VCHIQ_SRVSTATE_LISTENING:
	case VCHIQ_SRVSTATE_CLOSEWAIT:
		break;
	default:
		vchiq_log_error(vchiq_core_log_level,
			"%d: fsi - (%d) in state %s",
			state->id, service->localport,
			srvstate_names[service->srvstate]);
		return;
	}

	vchiq_set_service_state(service, VCHIQ_SRVSTATE_FREE);

	semaphore_up(&service->remove_event);

	/* Release the initial lock */
	unlock_service(service);
}

VCHIQ_STATUS_T
vchiq_connect_internal(VCHIQ_STATE_T *state, VCHIQ_INSTANCE_T instance)
{
	VCHIQ_SERVICE_T *service;
	int i;

	/* Find all services registered to this client and enable them. */
	i = 0;
	while ((service = next_service_by_instance(state, instance, &i)) != NULL) {
		if (service->srvstate == VCHIQ_SRVSTATE_HIDDEN)
			vchiq_set_service_state(service,
				VCHIQ_SRVSTATE_LISTENING);
		unlock_service(service);
	}

	if (state->conn_state == VCHIQ_CONNSTATE_DISCONNECTED) {
		if (queue_message(state, NULL,
			VCHIQ_MAKE_MSG(VCHIQ_MSG_CONNECT, 0, 0), NULL, 0,
			0, QMFLAGS_IS_BLOCKING) == VCHIQ_RETRY)
			return VCHIQ_RETRY;

		vchiq_set_conn_state(state, VCHIQ_CONNSTATE_CONNECTING);
	}

	if (state->conn_state == VCHIQ_CONNSTATE_CONNECTING) {
		semaphore_down(&state->connect);
		vchiq_set_conn_state(state, VCHIQ_CONNSTATE_CONNECTED);
		semaphore_up(&state->connect);
	}

	return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T
vchiq_shutdown_internal(VCHIQ_STATE_T *state, VCHIQ_INSTANCE_T instance)
{
	VCHIQ_SERVICE_T *service;
	int i;

	/* Find all services registered to this client and enable them. */
	i = 0;
	while ((service = next_service_by_instance(state, instance,
		&i)) !=	NULL) {
		(void)vchiq_remove_service(service->handle);
		unlock_service(service);
	}

	return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T
vchiq_close_service(VCHIQ_SERVICE_HANDLE_T handle)
{
	/* Unregister the service */
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;

	if (!service)
		return VCHIQ_ERROR;

	vchiq_log_info(vchiq_core_log_level,
		"%d: close_service:%d",
		service->state->id, service->localport);

	if ((service->srvstate == VCHIQ_SRVSTATE_FREE) ||
		(service->srvstate == VCHIQ_SRVSTATE_LISTENING) ||
		(service->srvstate == VCHIQ_SRVSTATE_HIDDEN)) {
		unlock_service(service);
		return VCHIQ_ERROR;
	}

	mark_service_closing(service);

//	if (current == service->state->slot_handler_thread) {
//		status = vchiq_close_service_internal(service,
//			0/*!close_recvd*/);
//		BUG_ON(status == VCHIQ_RETRY);
//	} else {
//	/* Mark the service for termination by the slot handler */
//		request_poll(service->state, service, VCHIQ_POLL_TERMINATE);
//	}
//
	while (1) {
		(semaphore_down(&service->remove_event));
		if ((service->srvstate == VCHIQ_SRVSTATE_FREE) ||
			(service->srvstate == VCHIQ_SRVSTATE_LISTENING) ||
			(service->srvstate == VCHIQ_SRVSTATE_OPEN))
			break;

		vchiq_log_warning(vchiq_core_log_level,
			"%d: close_service:%d - waiting in state %s",
			service->state->id, service->localport,
			srvstate_names[service->srvstate]);
	}

	if ((status == VCHIQ_SUCCESS) &&
		(service->srvstate != VCHIQ_SRVSTATE_FREE) &&
		(service->srvstate != VCHIQ_SRVSTATE_LISTENING))
		status = VCHIQ_ERROR;

	unlock_service(service);

	return status;
}

VCHIQ_STATUS_T
vchiq_remove_service(VCHIQ_SERVICE_HANDLE_T handle)
{
	/* Unregister the service */
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	VCHIQ_STATUS_T status = VCHIQ_SUCCESS;

	if (!service)
		return VCHIQ_ERROR;

	vchiq_log_info(vchiq_core_log_level,
		"%d: remove_service:%d",
		service->state->id, service->localport);

	if (service->srvstate == VCHIQ_SRVSTATE_FREE) {
		unlock_service(service);
		return VCHIQ_ERROR;
	}

	mark_service_closing(service);

//	if ((service->srvstate == VCHIQ_SRVSTATE_HIDDEN) ||
//		(current == service->state->slot_handler_thread)) {
//		/* Make it look like a client, because it must be removed and
//		   not left in the LISTENING state. */
//		service->public_fourcc = VCHIQ_FOURCC_INVALID;
//
//		status = vchiq_close_service_internal(service,
//			0/*!close_recvd*/);
//		BUG_ON(status == VCHIQ_RETRY);
//	} else {
//		/* Mark the service for removal by the slot handler */
//		request_poll(service->state, service, VCHIQ_POLL_REMOVE);
//	}
	while (1) {
		semaphore_down(&service->remove_event);

		if ((service->srvstate == VCHIQ_SRVSTATE_FREE) ||
			(service->srvstate == VCHIQ_SRVSTATE_OPEN))
			break;

		vchiq_log_warning(vchiq_core_log_level,
			"%d: remove_service:%d - waiting in state %s",
			service->state->id, service->localport,
			srvstate_names[service->srvstate]);
	}

	if ((status == VCHIQ_SUCCESS) &&
		(service->srvstate != VCHIQ_SRVSTATE_FREE))
		status = VCHIQ_ERROR;

	unlock_service(service);

	return status;
}


VCHIQ_STATUS_T
vchiq_queue_message(VCHIQ_SERVICE_HANDLE_T handle, const VCHIQ_ELEMENT_T *elements, unsigned int count)
{
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	VCHIQ_STATUS_T status = VCHIQ_ERROR;

	unsigned int i;
  int size = 0;

	if (!service ||
		(vchiq_check_service(service) != VCHIQ_SUCCESS))
		goto error_exit;

	for (i = 0; i < (unsigned int)count; i++) {
		if (elements[i].size) {
			if (elements[i].data == NULL) {
				VCHIQ_SERVICE_STATS_INC(service, error_count);
				goto error_exit;
			}
			size += elements[i].size;
		}
	}

	if (size > VCHIQ_MAX_MSG_SIZE) {
		VCHIQ_SERVICE_STATS_INC(service, error_count);
		goto error_exit;
	}

	switch (service->srvstate) {
	case VCHIQ_SRVSTATE_OPEN:
		status = queue_message(service->state, service,
				VCHIQ_MAKE_MSG(VCHIQ_MSG_DATA,
					service->localport,
					service->remoteport),
				elements, count, size, 1);
		break;
	case VCHIQ_SRVSTATE_OPENSYNC:
		status = queue_message_sync(service->state, service,
				VCHIQ_MAKE_MSG(VCHIQ_MSG_DATA,
					service->localport,
					service->remoteport),
				elements, count, size, 1);
		break;
	default:
		status = VCHIQ_ERROR;
		break;
	}

error_exit:
	if (service)
		unlock_service(service);

	return status;
}

void
vchiq_release_message(VCHIQ_SERVICE_HANDLE_T handle, VCHIQ_HEADER_T *header)
{
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	VCHIQ_SHARED_STATE_T *remote;
	VCHIQ_STATE_T *state;
	int slot_index;

	if (!service)
		return;

	state = service->state;
	remote = state->remote;

	slot_index = SLOT_INDEX_FROM_DATA(state, (void *)header);

	if ((slot_index >= remote->slot_first) &&
		(slot_index <= remote->slot_last)) {
		int msgid = header->msgid;
		if (msgid & VCHIQ_MSGID_CLAIMED) {
			VCHIQ_SLOT_INFO_T *slot_info =
				SLOT_INFO_FROM_INDEX(state, slot_index);

			release_slot(state, slot_info, header, service);
		}
	} else if (slot_index == remote->slot_sync)
		release_message_sync(state, header);

	unlock_service(service);
}

static void
release_message_sync(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header)
{
	header->msgid = VCHIQ_MSGID_PADDING;
	wmb();
	remote_event_signal(&state->remote->sync_release);
}

bool vchiq_check_service(VCHIQ_SERVICE_T *service)
{
  return true;
}

VCHIQ_STATUS_T
vchiq_get_peer_version(VCHIQ_SERVICE_HANDLE_T handle, short *peer_version)
{
   VCHIQ_STATUS_T status = VCHIQ_ERROR;
   VCHIQ_SERVICE_T *service = find_service_by_handle(handle);

   if (!service ||
      (vchiq_check_service(service) != VCHIQ_SUCCESS) ||
      !peer_version)
      goto exit;
   *peer_version = service->peer_version;
   status = VCHIQ_SUCCESS;

exit:
   if (service)
      unlock_service(service);
   return status;
}

VCHIQ_STATUS_T
vchiq_get_config(VCHIQ_INSTANCE_T instance,
	int config_size, VCHIQ_CONFIG_T *pconfig)
{
	VCHIQ_CONFIG_T config;

	(void)instance;

	config.max_msg_size           = VCHIQ_MAX_MSG_SIZE;
	config.bulk_threshold         = VCHIQ_MAX_MSG_SIZE;
	config.max_outstanding_bulks  = VCHIQ_NUM_SERVICE_BULKS;
	config.max_services           = VCHIQ_MAX_SERVICES;
	config.version                = VCHIQ_VERSION;
	config.version_min            = VCHIQ_VERSION_MIN;

	if (config_size > sizeof(VCHIQ_CONFIG_T))
		return VCHIQ_ERROR;

	memcpy(pconfig, &config,
		min(config_size, (int)(sizeof(VCHIQ_CONFIG_T))));

	return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T
vchiq_set_service_option(VCHIQ_SERVICE_HANDLE_T handle,
	VCHIQ_SERVICE_OPTION_T option, int value)
{
	VCHIQ_SERVICE_T *service = find_service_by_handle(handle);
	VCHIQ_STATUS_T status = VCHIQ_ERROR;

	if (service) {
		switch (option) {
		case VCHIQ_SERVICE_OPTION_AUTOCLOSE:
			service->auto_close = value;
			status = VCHIQ_SUCCESS;
			break;

		case VCHIQ_SERVICE_OPTION_SLOT_QUOTA: {
			VCHIQ_SERVICE_QUOTA_T *service_quota =
				&service->state->service_quotas[
					service->localport];
			if (value == 0)
				value = service->state->default_slot_quota;
			if ((value >= service_quota->slot_use_count) &&
				 (value < (unsigned short)~0)) {
				service_quota->slot_quota = value;
				if ((value >= service_quota->slot_use_count) &&
					(service_quota->message_quota >=
					 service_quota->message_use_count)) {
					/* Signal the service that it may have
					** dropped below its quota */
					semaphore_up(&service_quota->quota_event);
				}
				status = VCHIQ_SUCCESS;
			}
		} break;

		case VCHIQ_SERVICE_OPTION_MESSAGE_QUOTA: {
			VCHIQ_SERVICE_QUOTA_T *service_quota =
				&service->state->service_quotas[
					service->localport];
			if (value == 0)
				value = service->state->default_message_quota;
			if ((value >= service_quota->message_use_count) &&
				 (value < (unsigned short)~0)) {
				service_quota->message_quota = value;
				if ((value >=
					service_quota->message_use_count) &&
					(service_quota->slot_quota >=
					service_quota->slot_use_count))
					/* Signal the service that it may have
					** dropped below its quota */
					semaphore_up(&service_quota->quota_event);
				status = VCHIQ_SUCCESS;
			}
		} break;

		case VCHIQ_SERVICE_OPTION_SYNCHRONOUS:
			if ((service->srvstate == VCHIQ_SRVSTATE_HIDDEN) ||
				(service->srvstate ==
				VCHIQ_SRVSTATE_LISTENING)) {
				service->sync = value;
				status = VCHIQ_SUCCESS;
			}
			break;

		case VCHIQ_SERVICE_OPTION_TRACE:
			service->trace = value;
			status = VCHIQ_SUCCESS;
			break;

		default:
			break;
		}
		unlock_service(service);
	}

	return status;
}

#ifndef __circle__
void
vchiq_dump_shared_state(void *dump_context, VCHIQ_STATE_T *state,
	VCHIQ_SHARED_STATE_T *shared, const char *label)
{
	static const char *const debug_names[] = {
		"<entries>",
		"SLOT_HANDLER_COUNT",
		"SLOT_HANDLER_LINE",
		"PARSE_LINE",
		"PARSE_HEADER",
		"PARSE_MSGID",
		"AWAIT_COMPLETION_LINE",
		"DEQUEUE_MESSAGE_LINE",
		"SERVICE_CALLBACK_LINE",
		"MSG_QUEUE_FULL_COUNT",
		"COMPLETION_QUEUE_FULL_COUNT"
	};
	int i;

	char buf[80];
	int len;
	len = snprintf(buf, sizeof(buf),
		"  %s: slots %d-%d tx_pos=%x recycle=%x",
		label, shared->slot_first, shared->slot_last,
		shared->tx_pos, shared->slot_queue_recycle);
	// vchiq_dump(dump_context, buf, len + 1);

	len = snprintf(buf, sizeof(buf),
		"    Slots claimed:");
  if (len) {
  }
	// vchiq_dump(dump_context, buf, len + 1);

	for (i = shared->slot_first; i <= shared->slot_last; i++) {
		VCHIQ_SLOT_INFO_T slot_info = *SLOT_INFO_FROM_INDEX(state, i);
		if (slot_info.use_count != slot_info.release_count) {
			len = snprintf(buf, sizeof(buf),
				"      %d: %d/%d", i, slot_info.use_count,
				slot_info.release_count);
			// vchiq_dump(dump_context, buf, len + 1);
		}
	}

	for (i = 1; i < shared->debug[DEBUG_ENTRIES]; i++) {
		len = snprintf(buf, sizeof(buf), "    DEBUG: %s = %d(%x)",
			debug_names[i], shared->debug[i], shared->debug[i]);
		// vchiq_dump(dump_context, buf, len + 1);
	}
}
#endif


void
vchiq_loud_error_header(void)
{
	vchiq_log_error(vchiq_core_log_level,
		"============================================================"
		"================");
	vchiq_log_error(vchiq_core_log_level,
		"============================================================"
		"================");
	vchiq_log_error(vchiq_core_log_level, "=====");
}

void
vchiq_loud_error_footer(void)
{
	vchiq_log_error(vchiq_core_log_level, "=====");
	vchiq_log_error(vchiq_core_log_level,
		"============================================================"
		"================");
	vchiq_log_error(vchiq_core_log_level,
		"============================================================"
		"================");
}


VCHIQ_STATUS_T vchiq_send_remote_use(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_RETRY;
	if (state->conn_state != VCHIQ_CONNSTATE_DISCONNECTED)
		status = queue_message(state, NULL,
			VCHIQ_MAKE_MSG(VCHIQ_MSG_REMOTE_USE, 0, 0),
			NULL, 0, 0, 0);
	return status;
}

VCHIQ_STATUS_T vchiq_send_remote_release(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_RETRY;
	if (state->conn_state != VCHIQ_CONNSTATE_DISCONNECTED)
		status = queue_message(state, NULL,
			VCHIQ_MAKE_MSG(VCHIQ_MSG_REMOTE_RELEASE, 0, 0),
			NULL, 0, 0, 0);
	return status;
}

VCHIQ_STATUS_T vchiq_send_remote_use_active(VCHIQ_STATE_T *state)
{
	VCHIQ_STATUS_T status = VCHIQ_RETRY;
	if (state->conn_state != VCHIQ_CONNSTATE_DISCONNECTED)
		status = queue_message(state, NULL,
			VCHIQ_MAKE_MSG(VCHIQ_MSG_REMOTE_USE_ACTIVE, 0, 0),
			NULL, 0, 0, 0);
	return status;
}

void vchiq_log_dump_mem(const char *label, uint32_t addr, const void *voidMem,
	size_t numBytes)
{
	const uint8_t  *mem = (const uint8_t *)voidMem;
	size_t          offset;
	char            lineBuf[100];
	char           *s;

	while (numBytes > 0) {
		s = lineBuf;

		for (offset = 0; offset < 16; offset++) {
			if (offset < numBytes)
				s += snprintf(s, 4, "%02x ", mem[offset]);
			else
				s += snprintf(s, 4, "   ");
		}

		for (offset = 0; offset < 16; offset++) {
			if (offset < numBytes) {
				uint8_t ch = mem[offset];

				if ((ch < ' ') || (ch > '~'))
					ch = '.';
				*s++ = (char)ch;
			}
		}
		*s++ = '\0';

		if ((label != NULL) && (*label != '\0'))
			vchiq_log_trace(VCHIQ_LOG_TRACE,
				"%s: %08x: %s", label, addr, lineBuf);
		else
			vchiq_log_trace(VCHIQ_LOG_TRACE,
				"%08x: %s", addr, lineBuf);

		addr += 16;
		mem += 16;
		if (numBytes > 16)
			numBytes -= 16;
		else
			numBytes = 0;
	}
}

#define BELL2 ((reg32_t)(0x3f00b848))
void vchiq_ring_bell(void)
{
  write_reg(BELL2, 0);
}
