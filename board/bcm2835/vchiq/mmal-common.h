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
 *
 * MMAL structures
 *
 */
#ifndef MMAL_COMMON_H
#define MMAL_COMMON_H
#include <list.h>

#define BIT_ULL(nr)                (1ULL << (nr))
#define MMAL_FOURCC(a, b, c, d) ((a) | (b << 8) | (c << 16) | (d << 24))
#define MMAL_MAGIC MMAL_FOURCC('m', 'm', 'a', 'l')

/** Special value signalling that time is not known */
#define MMAL_TIME_UNKNOWN BIT_ULL(63)

struct mmal_msg_context;

/* mapping between v4l and mmal video modes */
struct mmal_fmt {
	char  *name;
	uint32_t   fourcc;          /* v4l2 format id */
	int   flags;           /* v4l2 flags field */
	uint32_t   mmal;
	int   depth;
	uint32_t   mmal_component;  /* MMAL component index to be used to encode */
	uint32_t   ybbp;            /* depth of first Y plane for planar formats */
	bool  remove_padding;  /* Does the GPU have to remove padding,
				* or can we do hide padding via bytesperline.
				*/
};

/* buffer for one video frame */
struct mmal_buffer {
	/* v4l buffer data -- must be first */
	//struct vb2_v4l2_buffer	vb;

	/* list of buffers available */
	struct list_head	list;

	void *buffer; /* buffer pointer */
	unsigned long buffer_size; /* size of allocated buffer */

	struct mmal_msg_context *msg_context;

	struct dma_buf *dma_buf;/* Exported dmabuf fd from videobuf2 */
	void *vcsm_handle;	/* VCSM handle having imported the dmabuf */
	uint32_t vc_handle;		/* VC handle to that dmabuf */

	uint32_t cmd;		/* MMAL command. 0=data. */
	unsigned long length;
	uint32_t mmal_flags;
	int64_t dts;
	int64_t pts;
};

/* */
struct mmal_colourfx {
	int32_t enable;
	uint32_t u;
	uint32_t v;
};
#endif
