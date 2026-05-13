#ifndef _NF_SOLO_ENC_H_
#define _NF_SOLO_ENC_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_enc.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
11/17/2009 0.1   tikim          Created.

................................................................................

DESCRIPTION:

  This module contains process encoded frame of SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// #include <gst/nf/gstnfbuddybuffer.h>
#include <gobj.h>
#include <gobjmedia.h>
#include "nf_solo_common.h"

/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

#define IOCTL_ENC_DEV			"/dev/solo6110_enc"

#define IOCTL_ENC_MAGIC			'e'

#define IOCTL_CAP_SET_SCALE		_IOW(IOCTL_ENC_MAGIC, 0, CAP_SCALE)
#define IOCTL_CAP_SET_INTERVAL	_IOW(IOCTL_ENC_MAGIC, 1, CAP_INTERVAL)
#define IOCTL_MP4E_SET_GOPSIZE	_IOW(IOCTL_ENC_MAGIC, 2, MP4E_GOPSIZE)
#define IOCTL_MP4E_SET_QP		_IOW(IOCTL_ENC_MAGIC, 3, MP4E_QP)
#define IOCTL_CAP_GET_SCALE		_IOWR(IOCTL_ENC_MAGIC, 4, CAP_SCALE)

#define IOCTL_CAP_GET_INTERVAL	_IOWR(IOCTL_ENC_MAGIC, 5, CAP_INTERVAL)
#define IOCTL_MP4E_GET_GOPSIZE	_IOWR(IOCTL_ENC_MAGIC, 6, MP4E_GOPSIZE)
#define IOCTL_MP4E_GET_QP		_IOWR(IOCTL_ENC_MAGIC, 7, MP4E_QP)

#define IOCTL_CAP_OSD			_IOW(IOCTL_ENC_MAGIC, 8, CAP_OSD)

#define IOCTL_SET_JPEG_MODE		_IO(IOCTL_ENC_MAGIC, 9)

#define IOCTL_GOP_RESET			_IOW(IOCTL_ENC_MAGIC, 10, unsigned int)
#define IOCTL_SET_BITRATE		_IOW(IOCTL_ENC_MAGIC, 11, MP4E_BITRATE)

#define IOCTL_CLEAR_MPEG_QUEUE	_IO(IOCTL_ENC_MAGIC, 12)
#define IOCTL_CLEAR_JPEG_QUEUE	_IO(IOCTL_ENC_MAGIC, 13)

/* Add Command for Control frame. */
#define IOCTL_MP4E_SET_INFO		_IOW(IOCTL_ENC_MAGIC, 14, RECORD_INFO)
#define IOCTL_MP4E_GET_INFO		_IOW(IOCTL_ENC_MAGIC, 15, RECORD_INFO)
#define IOCTL_MP4E_CTRL_FRAME   _IOW(IOCTL_ENC_MAGIC, 16, guchar)
#define IOCTL_SET_TIMESYNC		_IO(IOCTL_ENC_MAGIC, 17)

#define IOCTL_ENC_MAXNR			( 14 + 4 )	/* Original + Custom */

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef struct _CAP_SCALE
{
	unsigned int channel;
	unsigned int value;
} CAP_SCALE;

typedef struct _MP4E_GOPSIZE
{
	unsigned int channel;
	unsigned int value;
} MP4E_GOPSIZE;

typedef struct _MP4E_QP
{
	unsigned int channel;
	unsigned int value;
} MP4E_QP;

typedef struct _CAP_INTERVAL
{
	unsigned int channel;
	unsigned int value;
} CAP_INTERVAL;

typedef struct _MP4E_BITRATE
{
	unsigned int channel;
	int value;
} MP4E_BITRATE;

typedef struct _solo_enc_data
{
	CAP_SCALE 	 scale;
	MP4E_GOPSIZE gop;
	MP4E_QP 	 qp;
	CAP_INTERVAL intvl;
	MP4E_BITRATE bitrate;
} SOLO_ENC_DATA;

typedef struct _CAP_OSD
{
	unsigned int channel;
	unsigned int on;
	unsigned long data;
} CAP_OSD;

typedef struct _solo_enc_header
{
	guchar 		header_size;	/* ICODEC header size */
	guchar 		channel;		/* CH */
	guchar 		vop_type;		/* I/P/Ctrl frame */
	guchar 		scale;			/* Resolution */
	guint 		interval;		/* Capture interval */
	guint		sec;			/* When encording start sec. */
	guint		usec;			/* When encording start usec. */
//	gboolean 	is_motion;		/* Record type */
} SOLO_ENC_HEADER;

typedef struct _solo_enc
{
	NfObject  		object;
	GThread			*thread;						/* Thread that for Read a frame. */
	gint 			thread_run;						/* Thread run is 1. */
	gint 			thread_status;					/* Not used */	
	gint 			init_done;						/* Init is done */
	gint 			fd[SOLO_ENC_NUM_];				/* Encoder descripter */
	gchar 			*device[SOLO_ENC_NUM_];			/* Device path */
	gchar 			*buffer[SOLO_ENC_NUM_];			/* Address of control frame */
	// GstNfBuddyBuffer *gst_buf[SOLO_ENC_NUM_];		/* Address of CMEM for H.264 frame */
	GobjBuddyBuffer *gst_buf[SOLO_ENC_NUM_];		/* Address of CMEM for H.264 frame */
	SOLO_ENC_HEADER *header_info[SOLO_ENC_NUM_];	/* Header summary info. */
	gint  			frame_size[SOLO_ENC_NUM_];		/* Frame size */
	gboolean 		disable;						/* Encoder enable/disable. */
	gboolean		clear_iframe;					/* Do clear I-Frame of all CH. */
	guint  			wait_iframe;					/* I-Frame wait status of each CH. */
	guint  			channel_mask;					/* mask status of each CH */
	guint  			max_frame_size;					/* 1MB */
	GAsyncQueue	  	*queue;							/* SOLO enc -> SOLO rec */
} SOLO_ENC;

typedef struct _solo_enc_qdata
{
	gchar 			*frame;			/* SOLO header + frame */
	SOLO_ENC_HEADER *header_info;	/* SOLO_ENC.header_info */
	gint 			frame_size;		/* SOLO_ENC.frame_size */
	gint			reserved;
} SOLO_ENC_QDATA;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_ENC **solo_enc_get_address(void);
SOLO_ENC *solo_enc_get_struct(void);

gboolean solo_enc_create(SOLO_ENC **p);
gboolean solo_enc_create_header(SOLO_ENC_HEADER **p);
gboolean solo_enc_init(SOLO_ENC **p);
gboolean solo_enc_open(SOLO_ENC *t, gchar *path);
gint	 solo_enc_read(SOLO_ENC *t);
gboolean solo_enc_enable(void);
gboolean solo_enc_disable(SOLO_ENC *t);
gboolean solo_enc_disable_config(gint *fd);
gboolean solo_enc_clear_queue(gint *fd);
gboolean solo_enc_clear_iframe(SOLO_ENC *t);

gboolean solo_enc_ctrl_frame(guchar ch);
gboolean solo_enc_set_timesync(void);
gint	 solo_enc_get_iframe(guchar ch);
gboolean solo_enc_set_iframe(guchar ch, gchar onoff);
gboolean solo_enc_set_ch_mask(guchar ch, gchar onoff);

gboolean solo_enc_parsing_header(SOLO_ENC *t);
gboolean solo_enc_chk_iframe(SOLO_ENC *t);
gboolean solo_enc_wait_iframe(SOLO_ENC *t);
gboolean solo_enc_ch_mask(SOLO_ENC *t);
gint	 solo_enc_chk_disable(SOLO_ENC *t);
gint 	 solo_enc_get_motion(const SOLO_ENC_HEADER *t);		/* Not used */
gint 	 solo_enc_motion(SOLO_ENC *t);

gboolean solo_enc_send_cmd(SOLO_ENC *t);

static void solo_enc_thread_func(SOLO_ENC *self);

#if DEBUG_ENC_INFO_FRAME_ 

typedef struct _solo_enc_debug_frame
{
	gboolean	init_done;
	guchar 		vop_type;
	guint		sec;
	guint		usec;
	guint		frames;
} SOLO_ENC_DEBUG_FRAME;

void solo_enc_debug_frame(SOLO_ENC *t);

#endif /* DEBUG_ENC_INFO_FRAME_ */

#if DEBUG_ENC_INFO_TASK_

int64_t gettime(void);
void solo_enc_debug_task(void);

#define TIME_CHECK_START \
	{ \
		int64_t start, end, delta; \
		start = gettime(); \

#define TIME_CHECK_END \
		end = gettime(); \
		delta = end - start; \
		g_message("Time:%lld \n", delta); \
	}

#endif /* DEBUG_ENC_INFO_TASK_ */

#if DEBUG_ENC_INFO_QUEUE_
extern guint enc_queue_count;
#endif /* DEBUG_ENC_INFO_QUEUE_ */

#endif /* _NF_SOLO_ENC_H_ */

