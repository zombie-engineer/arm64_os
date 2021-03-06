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

#ifndef MMAL_MSG_FORMAT_H
#define MMAL_MSG_FORMAT_H

#include "mmal-msg-common.h"

/* MMAL_ES_FORMAT_T */

struct mmal_audio_format {
	uint32_t channels;		/* Number of audio channels */
	uint32_t sample_rate;	/* Sample rate */

	uint32_t bits_per_sample;	/* Bits per sample */
	uint32_t block_align;	/* Size of a block of data */
};

struct mmal_video_format {
	uint32_t width;		/* Width of frame in pixels */
	uint32_t height;		/* Height of frame in rows of pixels */
	struct mmal_rect crop;	/* Visible region of the frame */
	struct mmal_rational frame_rate;	/* Frame rate */
	struct mmal_rational par;		/* Pixel aspect ratio */

	/*
	 * FourCC specifying the color space of the video stream. See the
	 * MmalColorSpace "pre-defined color spaces" for some examples.
	 */
	uint32_t color_space;
};

struct mmal_subpicture_format {
	uint32_t x_offset;
	uint32_t y_offset;
};

union mmal_es_specific_format {
	struct mmal_audio_format audio;
	struct mmal_video_format video;
	struct mmal_subpicture_format subpicture;
};

/* Definition of an elementary stream format (MMAL_ES_FORMAT_T) */
struct mmal_es_format_local {
	uint32_t type;	/* enum mmal_es_type */

	uint32_t encoding;	/* FourCC specifying encoding of the elementary
			 * stream.
			 */
	uint32_t encoding_variant;	/* FourCC specifying the specific
				 * encoding variant of the elementary
				 * stream.
				 */

	union mmal_es_specific_format *es;	/* Type specific
						 * information for the
						 * elementary stream
						 */

	uint32_t bitrate;	/* Bitrate in bits per second */
	uint32_t flags;	/* Flags describing properties of the elementary
			 * stream.
			 */

	uint32_t extradata_size;	/* Size of the codec specific data */
	uint8_t *extradata;		/* Codec specific data */
};

/* Remote definition of an elementary stream format (MMAL_ES_FORMAT_T) */
struct mmal_es_format {
	uint32_t type;	/* enum mmal_es_type */

	uint32_t encoding;	/* FourCC specifying encoding of the elementary
			 * stream.
			 */
	uint32_t encoding_variant;	/* FourCC specifying the specific
				 * encoding variant of the elementary
				 * stream.
				 */

	uint32_t es;	/* Type specific
		 * information for the
		 * elementary stream
		 */

	uint32_t bitrate;	/* Bitrate in bits per second */
	uint32_t flags;	/* Flags describing properties of the elementary
			 * stream.
			 */

	uint32_t extradata_size;	/* Size of the codec specific data */
	uint32_t extradata;		/* Codec specific data */
};

#endif /* MMAL_MSG_FORMAT_H */
