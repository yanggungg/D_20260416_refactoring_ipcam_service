#ifndef _NF_SOLO_JPEG_H_
#define _NF_SOLO_JPEG_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_jpeg.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
12/29/2009 1.0   tikim          Created.
07/19/2010 2.0   tikim          For SNF.

................................................................................

DESCRIPTION:

  This module contains dual stream(jpeg) for SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"
#include "nf_codec_header.h"
#include "nf_util_jpeg.h"

/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

#define IOCTL_JPEG_DEV			"/dev/solo6110_jpeg"

#define SOI_OFFSET				( 0 )
#define SOI_LEN					( 2 )

#define DQT_OFFSET				( SOI_OFFSET + SOI_LEN )
#define DQT_LEN					( 138 )

#define DHT_OFFSET				( DQT_OFFSET + DQT_LEN )
#define DHT_LEN					( 420 )

#define SOF_OFFSET				( DHT_OFFSET + DHT_LEN )
#define SOF_LEN					( 19 )

#define SOS_OFFSET				( SOF_OFFSET + SOF_LEN )
#define SOS_LEN					( 14 )

#define JPEG_HEADER_SIZE		( SOS_OFFSET + SOS_LEN )

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef enum _solo_jpeg_status_e
{
	SOLO_JPEG_STATUS_READY = 0,
	SOLO_JPEG_STATUS_READ,
	SOLO_JPEG_STATUS_PARSING,
	SOLO_JPEG_STATUS_CHECK,
	SOLO_JPEG_STATUS_SEND,
	SOLO_JPEG_STATUS_SUCCESS,
	SOLO_JPEG_STATUS_FINISH
} SOLO_JPEG_STATUS_E;

typedef struct _solo_jpeg_info_t
{
	gint				ch;					/* Channel */
	guint				qp;					/* QP */
	guint				width;				/* Horizontal Picture Size */
	guint				height;				/* Vertical Picture Size */
	guint				sec;				/* Encoded time */
	guint				usec;				/* Encoded usec */
} SOLO_JPEG_INFO_T;

typedef struct _solo_jpeg_data_t
{
	ICODEC_HEADER		header;
	gchar				*data;
	SOLO_JPEG_INFO_T	info;
} SOLO_JPEG_DATA_T;

typedef struct _solo_jpeg_t
{
	NfObject	  		object;
	GThread				*thread;				/* Thread that for read jpeg. */
	gint 				thread_run;				/* Thread run is 1. */
	SOLO_JPEG_STATUS_E	thread_status;			/* Process status */
	gint 				init_done;				/* Init is done */
	gint 				fd[SOLO_JPEG_DEV_NUM_];	/* Jpeg descripter */
	gboolean 			disable;				/* Jpeg enable/disable. */
	gchar 				*header;				/* Jpeg header */
	gchar 				*buffer;				/* Buffer that for read jpeg from SOLO */
	gchar	 			*frame;					/* Pointer that indicate jpeg data. */
	SOLO_JPEG_INFO_T	*info;					/* Jpeg info */
	SOLO_JPEG_DATA_T	*output;				/* For webRA */
	guint  				channel_mask;			/* mask status of each CH */
	guint  				max_frame_size;			/* 1MB */
	gint	  			frame_size;				/* Jpeg size */
	GAsyncQueue	  		*queue;					/* Not used */
} SOLO_JPEG_T;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_JPEG_T **solo_jpeg_get_address(void);
SOLO_JPEG_T *solo_jpeg_get_struct(void);

gboolean solo_jpeg_create(SOLO_JPEG_T **p);
gboolean solo_jpeg_open(SOLO_JPEG_T *t, gchar *path);
gboolean solo_jpeg_init(SOLO_JPEG_T **p);

gboolean solo_jpeg_read(SOLO_JPEG_T *t);
gboolean solo_jpeg_parsing_header(SOLO_JPEG_T *t);
gint solo_jpeg_chk_disable(SOLO_JPEG_T *t);
gint solo_jpeg_ch_mask(SOLO_JPEG_T *t);
gboolean solo_jpeg_set_header(SOLO_JPEG_T *t);
gboolean solo_jpeg_send_webra(SOLO_JPEG_T *t);
gboolean solo_jpeg_request(gint ch);

static void solo_jpeg_thread_func(SOLO_JPEG_T *self);


#endif /* _NF_SOLO_JPEG_H_ */
