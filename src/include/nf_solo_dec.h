#ifndef _NF_SOLO_DEC_H_
#define _NF_SOLO_DEC_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_dec.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
11/17/2009 0.1   tikim          Created.

................................................................................

DESCRIPTION:

  This module contains decoder for SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"
#include "nf_solo_capture.h"
#if defined(_OTM_MODEL)
#include "nf_solo_disp.h"
#elif defined(_SNF_MODEL)
#include "nf_tw2880_disp.h"
#endif
/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

#define IOCTL_DEC_DEV0			"/dev/solo6110_dec0"
#define IOCTL_DEC_DEV1			"/dev/solo6110_dec1"
#define IOCTL_DEC_DEV2			"/dev/solo6110_dec2"
#define IOCTL_DEC_DEV3			"/dev/solo6110_dec3"

#define IOCTL_VD_MAGIC		'i'

#define	IOCTL_VD_DEINTERLACE_ON    	_IO(IOCTL_VD_MAGIC, 0)
#define	IOCTL_VD_DEINTERLACE_OFF    _IO(IOCTL_VD_MAGIC, 1)
#define	IOCTL_VD_DEINTERLACE_TEST   _IOW(IOCTL_VD_MAGIC, 2, unsigned int)

#define IOCTL_VD_MAXNR		3

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef enum _dec_cmd_e
{
	SOLO_CMD_DEC_START = 0,
	SOLO_CMD_DEC_EOS = 1,
	SOLO_CMD_DEC_END
} DEC_CMD_E;


typedef struct _solo_dec_header_t
{
	guchar 	 header_size;
	guchar 	 vop_type;
	guchar 	 scale;
	guchar 	 channel;
	guint 	 interval;
	guint	 sec;
	guint	 usec;
//	gboolean is_motion;
} SOLO_DEC_HEADER_T;

typedef struct _solo_dec_t
{
	GThread				*thread;
	gint				thread_run;
	gint				thread_status;
	gint 				init_done;
	gint 				fd[4];
	gchar 				*device;			/* Not used. */
	gchar 				*buffer;			/* Aligned buffer */
	SOLO_DEC_HEADER_T 	*header_info;
	gchar 				*frame;				/* Indicate encoded data  pointer */
	guint				wait_i_frame;
	guint				frame_size;			/* Except ICODEC header */
	guint				max_frame_size;
#if defined(_OTM_MODEL)	
	SOLO_DEC_DISP_T		*disp;
#endif	
	GAsyncQueue			*queue;
} SOLO_DEC_T;

typedef struct _solo_dec_frame_t
{
#if DEBUG_DEC_GST_BUFFER_
//	GstNfBuddyBuffer *data;
  void *data;
#else
	gchar *data;
#endif /* DEBUG_DEC_GST_BUFFER_ */
	guint size;
	gint  width;
	gint  height;
} SOLO_DEC_FRAME_T;

typedef struct _solo_dec_qdata
{
	guchar			 dec_cmd;
	SOLO_DEC_FRAME_T *frame_info;
	guchar display_ch_cnt;
	guchar display_start_ch;
	guchar			 reserved[2];
} SOLO_DEC_QDATA;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_DEC_T **solo_dec_get_address(void);
SOLO_DEC_T *solo_dec_get_struct(void);

gboolean solo_dec_create(SOLO_DEC_T **p);
gboolean solo_dec_create_header(SOLO_DEC_HEADER_T **p);
gboolean solo_dec_open(SOLO_DEC_T *t, gchar *path, gint index);
gboolean solo_dec_init(SOLO_DEC_T **p);

gboolean solo_dec_set_ch(SOLO_DEC_T  *self, SOLO_DEC_QDATA *dec_qdata_t, gboolean end_cmd);
gboolean solo_dec_get_data(SOLO_DEC_T *dest, const SOLO_DEC_FRAME_T *src);
gboolean solo_dec_set_header(SOLO_DEC_T *t,  const SOLO_DEC_FRAME_T *src);
gboolean solo_dec_get_header(SOLO_DEC_T *t, const SOLO_DEC_FRAME_T *src);
gint	 solo_dec_ch_mask(SOLO_DEC_T *t);
gint 	 solo_dec_wait_iframe(SOLO_DEC_T *t);

gboolean solo_dec_create_qdata(SOLO_DEC_QDATA **p);
gboolean solo_dec_free_qdata(SOLO_DEC_QDATA **p, SOLO_DEC_T *t);
gboolean solo_dec_set_qdata(SOLO_DEC_QDATA *t, const gchar *p,  const gint n, gint w, gint h, gint display_ch_cnt, gint display_start_ch, gint eos_ch );

//gboolean solo_dec_send_cmd(const GstBuffer *buf, gint display_ch_cnt, gint display_start_ch, gpointer data);
gboolean solo_dec_send_cmd(const GstBuffer * buf, gint display_ch_cnt, gint display_start_ch , gint eos_ch, gpointer data); 
gboolean solo_dec_send_cmd_end(void);
gboolean solo_dec_recv_cmd(SOLO_DEC_QDATA **p, const SOLO_DEC_T *t);

static void solo_dec_thread_func(SOLO_DEC_T *self);

#endif /* _NF_SOLO_DEC_H_ */

