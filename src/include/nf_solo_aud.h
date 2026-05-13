#ifndef _NF_SOLO_AUD_H_
#define _NF_SOLO_AUD_H_

/*******************************************************************************
*  (c) COPYRIGHT 2009 ITX Security                                             *
*                                                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  tikim                                                                  	   *
*  ITX Security                                                                *
*                                                                              *
********************************************************************************

MODULE NAME: nf_solo_aud.h

REVISION HISTORY:

Date       Ver   Name           Description
__________ ____  ______________ ________________________________________________
12/11/2009 1.0   tikim          Created.
07/05/2010 2.0	 tikim			For SNF model.

................................................................................

DESCRIPTION:

  This module contains audio part for SOLO6110.

................................................................................
*/

/** ********************************************************************* **
 ** includes
 ** ********************************************************************* **/

#include "nf_solo_common.h"
#include "nf_codec_header.h"
#include "nf_rec_audio.h"

/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/

/* Audio I/O info */
#define AUDIO_ENC_NUM		( 4 )
#define AUDIO_DEC_NUM		( 1 )
#define AUDIO_CH_NUM		( AUDIO_ENC_NUM + AUDIO_DEC_NUM )				// 4 + 1 = 5CH

/* Audio unit info */
#define AUDIO_SAMPLE_RATE	( 8000 )										// 8KHz
#define AUDIO_SAMPLE_TIME	( 1000 / AUDIO_SAMPLE_RATE )					// 1000ms(1sec) / 8KHz = 0.125ms

#define AUDIO_CODE_SIZE		( 3 )											// 3Bit
#define AUDIO_CODE_NUM		( 128 )											// 128code

#if DEBUG_AUD_DATA_SIZE_
#define AUDIO_G723_TIME		( AUDIO_SAMPLE_TIME * AUDIO_CODE_NUM )			// 16msec
#define AUDIO_G723_SIZE		( ( AUDIO_CODE_SIZE * AUDIO_CODE_NUM ) / 8 )	// 384bit / 8 = 48bytes
#else
#define AUDIO_G723_TIME		( 16 )											// 16msec
#define AUDIO_G723_SIZE		( 48 )											// 384bit / 8 = 48bytes
#endif /* DEBUG_AUD_DATA_SIZE_ */

#define AUDIO_ENC_DATA_SIZE	( AUDIO_G723_SIZE * AUDIO_ENC_NUM )				// 48 * 4 = 192bytes
#define AUDIO_DEC_DATA_SIZE	( AUDIO_G723_SIZE * AUDIO_DEC_NUM )				// 48 * 1 = 48bytes
#define AUDIO_DATA_SIZE		( AUDIO_G723_SIZE * AUDIO_CH_NUM )				// 192 + 48 = 240bytes

#define AUDIO_PAGE_SIZE		( AUDIO_DATA_SIZE + 64 )						// 960 + 64 = 1KB
#define AUDIO_PAGE_NUM		( 32 )

#define AUDIO_ENC_MEM_SIZE	( AUDIO_PAGE_SIZE * AUDIO_PAGE_NUM )			// 32KB
#define AUDIO_DEC_MEM_SIZE	( AUDIO_PAGE_SIZE * AUDIO_PAGE_NUM )			// 32KB

/* Audio timestamp size */
#define AUDIO_SEC_SIZE		( 4 )											// 4bytes
#define AUDIO_USEC_SIZE		( 4 )											// 4bytes
#define AUDIO_TIME_SIZE		( AUDIO_SEC_SIZE + AUDIO_USEC_SIZE )			// 8bytes

/* Audio block info */
#if DEBUG_AUD_OUTPUT_DEC_
#define AUDIO_BLOCK_TIME	( 16 )											// 16ms
#else
#define AUDIO_BLOCK_TIME	( 1024 )										// 1024ms
#endif /* DEBUG_AUD_OUTPUT_DEC_ */

#define AUDIO_BLOCK_NUM		( AUDIO_BLOCK_TIME / AUDIO_G723_TIME )			// 64 or 1
#define AUDIO_BLOCK_SIZE	( AUDIO_G723_SIZE *  AUDIO_BLOCK_NUM )			// 3072bytes or 48bytes

/* Audio buffer info */
#if DEBUG_AUD_DATA_SIZE_

#if DEBUG_AUD_OUTPUT_DEC_
#define AUDIO_BUFFER_SIZE	AUDIO_ENC_DATA_SIZE								// 48 * 4 = 192bytes
#else
#define AUDIO_BUFFER_SIZE	( AUDIO_BLOCK_SIZE * AUDIO_ENC_NUM )			// 3072 * 4 = 12288 = 12KB
#endif /* DEBUG_AUD_OUTPUT_DEC_ */

#else /* !DEBUG_AUD_DATA_SIZE_ */

#if DEBUG_AUD_OUTPUT_DEC_
#define AUDIO_BUFFER_SIZE	( 192 )											// 48 * 4 = 192bytes
#else
#define AUDIO_BUFFER_SIZE	( 12288 )										// 3072 * 4 = 12288 = 12KB
#endif /* DEBUG_AUD_OUTPUT_DEC_ */

#endif /* DEBUG_AUD_DATA_SIZE_ */

#define AUDIO_ENCODING_ULAW		(1)
#define AUDIO_ENCODING_ALAW		(2)
#define AUDIO_ENCODING_LINEAR	(3)

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)			/* Number of A-law segments. */
#define	SEG_SHIFT	(4)			/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */

/* I/O control */
#define IOCTL_AUD_DEV				"/dev/solo6110_g723_"

#define IOCTL_G723_MAGIC			'g'

#define IOCTL_G723_CLEAR_WR_BUF		_IO(IOCTL_G723_MAGIC, 0)
#define IOCTL_G723_CLEAR_RD_BUF		_IO(IOCTL_G723_MAGIC, 1)

#define IOCTL_G723_MAXNR			(2)

/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/

/* Audio handoff */
typedef void ( *SOLO_AUD_HANDOFF_FUNC ) ( gpointer data );

typedef enum _solo_aud_input_ch_e
{
	AUD_INPUT_CH01,
	AUD_INPUT_CH02,
	AUD_INPUT_CH03,
	AUD_INPUT_CH04,
	AUD_INPUT_CH05,
	AUD_INPUT_CH06,
	AUD_INPUT_CH07,
	AUD_INPUT_CH08,
	AUD_INPUT_CH09,
	AUD_INPUT_CH10,
	AUD_INPUT_CH11,
	AUD_INPUT_CH12,
	AUD_INPUT_CH13,
	AUD_INPUT_CH14,
	AUD_INPUT_CH15,
	AUD_INPUT_CH16,
 	AUD_INPUT_CHNR,
	AUD_INPUT_OFF 	= 0xFF
} SOLO_AUD_INPUT_CH_E;

typedef enum _solo_aud_cmd_e
{
	SOLO_CMD_AUD_ENCODER,
	SOLO_CMD_AUD_DECODER,
	SOLO_CMD_AUD_CFG
} SOLO_AUD_CMD_E;

typedef struct _solo_aud_info_t
{
	gchar			*data;		/* G723 page.(16CH + Time) */
	gint			size;		/* Data size */
	gint			index;		/* Index of device */
	GTimeVal		stime;		/* Start time of encoded page */
	GTimeVal		etime;		/* End time of encoded page */
	SOLO_AUD_CMD_E	cmd;		/* Queue command */
} SOLO_AUD_INFO_T;

typedef struct _solo_aud_param_t
{
	guchar		ch				[NUM_ANALOG_CHANNEL];
	guchar		reason			[NUM_ANALOG_CHANNEL];
	guchar		pre_rec_time	[NUM_ANALOG_CHANNEL];		/* pre_rec_time(sec) [0:Normal] */
	guchar		pre_rec_close	[NUM_ANALOG_CHANNEL];		/* pre_rec [0:None/Discard 1:Flush] */
	gint		stream_id		[NUM_ANALOG_CHANNEL];
	GTimeVal	timestamp		[NUM_ANALOG_CHANNEL];
} SOLO_AUD_PARAM_T;

typedef struct _solo_aud_frame_t
{
	ICODEC_HEADER *header[SOLO_AUD_UNIT_NUM_];
	gpointer buffer[SOLO_AUD_UNIT_NUM_];
} SOLO_AUD_FRAME_T;

typedef struct _solo_aud_t
{
	NfObject	 	 		object;
	GThread					*thread_sst;			/* Thread that for sst put. */
	gint 					thread_sst_run;			/* Thread for sst put run is 1. */
	gint 					thread_sst_status;		/* Not used */	
	GThread					*thread_enc;			/* Thread that for read g723. */
	gint 					thread_enc_run;			/* Thread for read g723 run is 1. */
	gint 					thread_enc_status;		/* Not used */	
	GThread					*thread_dec;			/* Thread that for decode g723. */
	gint 					thread_dec_run;			/* Thread for decode g723 run is 1. */
	gint 					thread_dec_status;		/* Not used */	
	gint 					init_done;				/* Init is done */
	gint 					fd[SOLO_AUD_DEV_NUM_];		/* Audio descripter */
	gboolean 				disable;				/* Audio enable/disable. */
	SOLO_AUD_PARAM_T		*param;					/* Audio param */
	SOLO_AUD_FRAME_T		*frame;					/* Audio encoded data */
	GAsyncQueue	  			*queue_sst;				/* Audio encoder -> SST */
	GAsyncQueue	  			*queue_dec;				/* SST -> Audio decoder */
	SOLO_AUD_HANDOFF_FUNC	handoff_func;
	guint					handoff_ch_mask;
} SOLO_AUD_T;

typedef struct _solo_aud_enc_t
{
	glong	yl;
	gshort	yu;
	gshort	dms;
	gshort	dml;
	gshort	ap;
	gshort	a[2];
	gshort	b[6];
	gshort	pk[2];
	gshort	dq[6];
	gshort	sr[2];
	gchar	*buffer;
	gshort	buffer_size;
	gshort	data_size;
	gchar	td;
} SOLO_AUD_ENC_T;

/** ********************************************************************* **
 ** function protoypes
 ** ********************************************************************* **/

SOLO_AUD_T		**solo_aud_get_address(void);
SOLO_AUD_T		*solo_aud_get_struct(void);
SOLO_AUD_ENC_T	**solo_aud_get_enc_address(void);
SOLO_AUD_ENC_T	*solo_aud_get_enc_struct(void);

gboolean solo_aud_create(SOLO_AUD_T **p);
gboolean solo_aud_create_info(SOLO_AUD_INFO_T **p, gint size, gint nr);
gboolean solo_aud_free_info(SOLO_AUD_INFO_T **p);
gboolean solo_aud_create_enc(SOLO_AUD_ENC_T **p, gint size);
gboolean solo_aud_open(SOLO_AUD_T *t, gchar *path);
gboolean solo_aud_init(SOLO_AUD_T **p);
gboolean solo_aud_enable(void);
gboolean solo_aud_disable(SOLO_AUD_T *t);
gboolean solo_aud_clear_wr_queue(gint *fd);
gboolean solo_aud_clear_rd_queue(gint *fd);

gboolean solo_aud_send_cmd_sst(const GAsyncQueue *q, SOLO_AUD_INFO_T **t);
gboolean solo_aud_send_cmd_dec(const GAsyncQueue *q, SOLO_AUD_INFO_T **t);
gboolean solo_aud_recv_cmd(const GAsyncQueue *q, SOLO_AUD_INFO_T **p);

gboolean solo_aud_set_cfg(SOLO_AUD_INFO_T *dest, const NF_REC_AUDIO_PARAM *src);
gboolean solo_aud_start(NF_REC_AUDIO_PARAM *t);
gboolean solo_aud_set_timestamp( SOLO_AUD_FRAME_T *dest, const SOLO_AUD_INFO_T *src);
gboolean solo_aud_parsing_frame( SOLO_AUD_FRAME_T *dest, const SOLO_AUD_INFO_T *src);
gboolean solo_aud_handoff(SOLO_AUD_T *t, gint index);
gboolean solo_aud_register_handoff(guint ch_mask, SOLO_AUD_HANDOFF_FUNC handoff_func);
gboolean solo_aud_put_sst(gchar ch, gpointer p, SOLO_AUD_PARAM_T *param);
gboolean solo_aud_put_frame(SOLO_AUD_T *t, gint index);
gboolean solo_aud_ctrl_sst(SOLO_AUD_PARAM_T *old,  NF_REC_AUDIO_PARAM *new);

gint	 solo_aud_get_fd(void);
gint	 solo_aud_read_block(gint *fd, SOLO_AUD_INFO_T **t);
gint	 solo_aud_chk_disable(SOLO_AUD_T *t);
gint	 solo_aud_parsing_block(SOLO_AUD_INFO_T **t);
gboolean solo_aud_parsing_page( SOLO_AUD_FRAME_T *dest, const SOLO_AUD_INFO_T *src);

gboolean solo_aud_g723_enc(gchar *dest, const guchar *src, gint size);

#endif /* _NF_SOLO_AUD_H_ */
