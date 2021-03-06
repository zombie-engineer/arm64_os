/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Broadcom BM2835 V4L2 driver
 *
 * Copyright © 2013 Raspberry Pi (Trading) Ltd.
 *
 * Authors: Vincent Sanders @ Collabora
 *          Dave Stevenson @ Broadcom
 *		(now dave.stevenson@raspberrypi.org)
 *          Simon Mellor @ Broadcom
 *          Luke Diamand @ Broadcom
 */

#pragma once

/* MMAL_PORT_TYPE_T */
enum mmal_port_type {
	MMAL_PORT_TYPE_UNKNOWN = 0,	/* Unknown port type */
	MMAL_PORT_TYPE_CONTROL,		/* Control port */
	MMAL_PORT_TYPE_INPUT,		/* Input port */
	MMAL_PORT_TYPE_OUTPUT,		/* Output port */
	MMAL_PORT_TYPE_CLOCK,		/* Clock port */
};

/* The port is pass-through and doesn't need buffer headers allocated */
#define MMAL_PORT_CAPABILITY_PASSTHROUGH                       0x01
/*
 *The port wants to allocate the buffer payloads.
 * This signals a preference that payload allocation should be done
 * on this port for efficiency reasons.
 */
#define MMAL_PORT_CAPABILITY_ALLOCATION                        0x02
/*
 * The port supports format change events.
 * This applies to input ports and is used to let the client know
 * whether the port supports being reconfigured via a format
 * change event (i.e. without having to disable the port).
 */
#define MMAL_PORT_CAPABILITY_SUPPORTS_EVENT_FORMAT_CHANGE      0x04

/*
 * mmal port structure (MMAL_PORT_T)
 *
 * most elements are informational only, the pointer values for
 * interogation messages are generally provided as additional
 * structures within the message. When used to set values only the
 * buffer_num, buffer_size and userdata parameters are writable.
 */
struct mmal_port {
	uint32_t priv;	/* Private member used by the framework */
	uint32_t name;	/* Port name. Used for debugging purposes (RO) */

	uint32_t type;	/* Type of the port (RO) enum mmal_port_type */
	uint16_t index;	/* Index of the port in its type list (RO) */
	uint16_t index_all;	/* Index of the port in the list of all ports (RO) */

	uint32_t is_enabled;	/* Indicates whether the port is enabled or not (RO) */
	uint32_t format;	/* Format of the elementary stream */

	uint32_t buffer_num_min;	/* Minimum number of buffers the port
				 *   requires (RO).  This is set by the
				 *   component.
				 */

	uint32_t buffer_size_min;	/* Minimum size of buffers the port
				 * requires (RO).  This is set by the
				 * component.
				 */

	uint32_t buffer_alignment_min;/* Minimum alignment requirement for
				  * the buffers (RO).  A value of
				  * zero means no special alignment
				  * requirements.  This is set by the
				  * component.
				  */

	uint32_t buffer_num_recommended;	/* Number of buffers the port
					 * recommends for optimal
					 * performance (RO).  A value of
					 * zero means no special
					 * recommendation.  This is set
					 * by the component.
					 */

	uint32_t buffer_size_recommended;	/* Size of buffers the port
					 * recommends for optimal
					 * performance (RO).  A value of
					 * zero means no special
					 * recommendation.  This is set
					 * by the component.
					 */

	uint32_t buffer_num;	/* Actual number of buffers the port will use.
			 * This is set by the client.
			 */

	uint32_t buffer_size; /* Actual maximum size of the buffers that
			  * will be sent to the port. This is set by
			  * the client.
			  */

	uint32_t component;	/* Component this port belongs to (Read Only) */

	uint32_t userdata;	/* Field reserved for use by the client */

	uint32_t capabilities;	/* Flags describing the capabilities of a
				 * port (RO).  Bitwise combination of \ref
				 * portcapabilities "Port capabilities"
				 * values.
				 */
};
