#ifndef _NF_SOLO_REC_H_
#define _NF_SOLO_REC_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_rec.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
11/17/2009 0.1   tikim          Created.

................................................................................

DESCRIPTION:

  This module contains record manager for SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

typedef enum _rec_res_e
{
	REC_RES_NONE	= 0,
	REC_RES_2CIF	= 1,
	REC_RES_CIF		= 2,
	REC_RES_D1		= 9,
} rec_res_e;

typedef enum _rec_intvl_frame_ntsc_e
{
	REC_INTVL_FRAME_NTSC_FPS30 = 0,
	REC_INTVL_FRAME_NTSC_FPS15 = 1,
	REC_INTVL_FRAME_NTSC_FPS07 = 3,
	REC_INTVL_FRAME_NTSC_FPS03 = 9,
	REC_INTVL_FRAME_NTSC_FPS02 = 14,
	REC_INTVL_FRAME_NTSC_FPS01 = 29
} rec_intvl_frame_ntsc_e;

typedef enum _rec_intvl_field_ntsc_e
{
	REC_INTVL_FIELD_NTSC_FPS30 = 1,
	REC_INTVL_FIELD_NTSC_FPS15 = 2,
	REC_INTVL_FIELD_NTSC_FPS07 = 4,
	REC_INTVL_FIELD_NTSC_FPS03 = 10,
	REC_INTVL_FIELD_NTSC_FPS02 = 15,
	REC_INTVL_FIELD_NTSC_FPS01 = 30
} rec_intvl_field_ntsc_e;

typedef enum _rec_intvl_frame_pal_e
{
	REC_INTVL_FRAME_PAL_FPS25 = 0,
	REC_INTVL_FRAME_PAL_FPS13 = 1,
	REC_INTVL_FRAME_PAL_FPS07 = 3,
	REC_INTVL_FRAME_PAL_FPS03 = 7,
	REC_INTVL_FRAME_PAL_FPS02 = 11,
	REC_INTVL_FRAME_PAL_FPS01 = 24 
} rec_intvl_frame_pal_e;

typedef enum _rec_intvl_field_pal_e
{
	REC_INTVL_FIELD_PAL_FPS25 = 1,
	REC_INTVL_FIELD_PAL_FPS13 = 2,
	REC_INTVL_FIELD_PAL_FPS07 = 4,
	REC_INTVL_FIELD_PAL_FPS03 = 8,
	REC_INTVL_FIELD_PAL_FPS02 = 12,
	REC_INTVL_FIELD_PAL_FPS01 = 25 
} rec_intvl_field_pal_e;

typedef enum _rec_gop_e
{
	REC_GOP_00	= 0,
	REC_GOP_01	= 1,
	REC_GOP_02	= 2,
	REC_GOP_03	= 3,
	REC_GOP_04	= 4,
	REC_GOP_07	= 7,
	REC_GOP_08	= 8,
	REC_GOP_13	= 13,
	REC_GOP_15	= 15,
	REC_GOP_16	= 16,
	REC_GOP_25	= 25,
	REC_GOP_30	= 30
} rec_gop_e;

typedef enum _rec_qp_e
{
	REC_QP_SUPER 	= 5,
	REC_QP_HIGHEST 	= 8,
	REC_QP_HIGH		= 11,
	REC_QP_STANDARD	= 14,
	REC_QP_LOW		= 17
} rec_qp_e;

typedef enum _rec_cmd_e
{
  SOLO_CMD_REC_TABLE_CHANGE  	= 0,
  SOLO_CMD_REC_VIDEO_ENCODER 	= 1,
  SOLO_CMD_REC_CTRL_FRAME	 	= 2,
  SOLO_CMD_REC_NET_TABLE_CHANGE	= 3
} rec_cmd_e;

typedef enum _rec_ctrl_e
{
	RP00 = 0,
	RP01 = 1,
	RP10 = 2,
	RP11 = 3
} rec_ctrl_e;

typedef enum _rec_end_flag_e
{
	END_FLAG_NONE           = 0,
    END_FLAG_FLUSH_PREREC   = 1,
    END_FLAG_DISCARD_PREREC = 2
} rec_end_flag_e;

typedef struct _solo_rec_status
{
	rec_ctrl_e	c_RP;		/* Current Rec/Pre-rec flag */
	rec_ctrl_e	n_RP;		/* New Rec/Pre-rec flag */
	gboolean	net;		/* Sending network or not */
	gboolean	pre;		/* Pre-rec or not */
	gboolean	rec;		/* Rec or not */
	gboolean	enc;		/* Enc or not */
	guchar		flags;		/* Save the flags of command */
	guchar		reserved[3];
} SOLO_REC_STATUS;

typedef struct _solo_rec_
{
	GThread				*thread;		/* Thread that for process record. */
	gint 				thread_run;		/* Thread run is 1. */
	gint 				thread_status;	/* Not used */
	gint 				init_done;		/* Init is done */
	DRREQ_RECORD_START 	*info;			/* Current record cfg */
	SOLO_REC_STATUS		*status;		/* Current record status */
	GAsyncQueue	  		*queue;			/* SOLO enc -> SOLO rec <- nf_record */
	guint				reserved;
} SOLO_REC;

typedef struct _solo_rec_qdata
{
	gpointer *data;			/* Data container */
	guchar	 rec_cmd;		/* Command */ 
	guchar	 reserved[3];
} SOLO_REC_QDATA;

typedef struct _solo_rec_start
{
	guint				change_ch;	/* CH change bit mask. */
	DRREQ_RECORD_START	*info;		/* Record config of each CH. */
} SOLO_REC_START;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_REC **solo_rec_get_address(void);
SOLO_REC *solo_rec_get_struct(void);

gboolean solo_rec_create(SOLO_REC **p);
gboolean solo_rec_init(SOLO_REC **p);

gboolean solo_rec_create_qdata(SOLO_REC_QDATA **p);
gboolean solo_rec_free_qdata(SOLO_REC_QDATA **p);
gboolean solo_rec_set_qdata(SOLO_REC_QDATA *dest, const DRREQ_RECORD_START *src, guint ch_mask, gint type);

gboolean solo_rec_send_cmd(guint ch, const DRREQ_RECORD_START *t, gint stream_type);
gboolean solo_rec_recv_cmd(SOLO_REC_QDATA **p, const SOLO_REC *t);

gboolean solo_rec_onoff(const RECORD_INFO *t);
gboolean solo_rec_set_cfg(RECORD_INFO *dest, const SOLO_REC_START *src);
gboolean solo_rec_set_cfg_net(const SOLO_REC_START *t);
gboolean solo_rec_get_cfg(RECORD_INFO *t);

guchar	 solo_rec_solo2nf_vop(guchar vop);
guchar 	 solo_rec_solo2nf_scale(guchar scale);
guchar	 solo_rec_solo2nf_intvl_frame(guint intvl);
guchar	 solo_rec_solo2nf_intvl_field(guint intvl);
guint	 solo_rec_nf2solo_width(guchar res);
guint	 solo_rec_nf2solo_height(guchar res);
guint 	 solo_rec_nf2solo_res(guchar res);
guint 	 solo_rec_nf2solo_fps_frame(guchar fps);
guint 	 solo_rec_nf2solo_fps_field(guchar fps);
guint 	 solo_rec_nf2solo_fps_gop(guchar fps);
guint 	 solo_rec_nf2solo_qp(guchar qp);
guint	 solo_rec_nf2solo_bitrate(guchar qp);

gint	 solo_rec_compare_flags(const RECORD_INFO *old, const RECORD_INFO *new);
gboolean solo_rec_chk_reason(guchar f);
gboolean solo_rec_set_status(SOLO_REC_STATUS *dest, const RECORD_INFO *src);
gboolean solo_rec_chk_net(SOLO_REC_STATUS *dest, const RECORD_INFO *src);
gboolean solo_rec_chk_enc(const SOLO_REC_STATUS *t);
gboolean solo_rec_set_info(RECORD_INFO *dest, const RECORD_INFO *src);

gboolean solo_rec_process_ctrl_frame(const SOLO_REC_STATUS *t1, const RECORD_INFO *t2);

#endif /* _NF_SOLO_REC_H_ */

