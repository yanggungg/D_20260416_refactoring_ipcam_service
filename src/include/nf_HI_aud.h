#ifndef __NF_HI_AUD_H__
#define __NF_HI_AUD_H__

#include <libsst.h>
#include "nf_HI_common.h"
#include "nf_rec_audio.h"
//#include "hi3531/hi_comm_hdmi.h"
#include "hi35XX/hi_comm_hdmi.h"

#define NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW		// pakkhman

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4	/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K				/* MEDIA_G726_16K, MEDIA_G726_24K ... */

#define HI_AUD_TIME_UNIT	(1020000)		// 1020000usec
#define HI_AUD_STM_CNT			(17)			// 1.02sec/(1/8000)sec*480
#define HI_AUDIO_PTNUMPERFRM	480

#define HI_AUD_TOTAL_SIZE		(HI_AUDIO_PTNUMPERFRM * HI_AUD_STM_CNT)			// 480 * HI_AUD_STM_CNT

#define HI_AUD_HEADER_SIZE	(4)				// 4byte
#define HI_AUD_NET_DEVISION		(10)			// For VLC

// for network mic
#define HI_AUD_STM_CNT_NET				(1)
#define HI_AUDIO_SIZE_STREAM			(960)
#define HI_AUDIO_DATA_WEB_SIZE			(800)
#define HI_AUDIO_DATA_SEND_SIZE			(HI_AUDIO_DATA_WEB_SIZE * HI_AUD_STM_CNT_NET)
#define HI_AUDIO_DATA_CONVERT_SIZE		((HI_AUDIO_DATA_WEB_SIZE * HI_AUD_STM_CNT_NET) * 2)
#define HI_AUDIO_NET_PB_MAX_QUEUE		50

#define HI_AUDIO_LIVE_STREAM_SIZE (1024)
#define HI_AUDIO_PLAY_STREAM_SIZE (8160)
/**************************************************************************************
	Convert
**************************************************************************************/
#define BIAS				(0x84)			/* Bias for linear code. */
#define CLIP				8159

#define SIO_MAX_NUM          6

typedef char*                   HI_PCHAR;
typedef void*                   HI_Ptr;       /* data pointer */

typedef enum _HI_AUDIO_DEF_E
{
	NF_AUDIO_NET_INS_DEF_SPEED      = HI_AUDIO_PTNUMPERFRM,
} HI_AUDIO_DEF_E;

/* Audio handoff */
typedef void ( *NF_HI_AUD_HANDOFF_FUNC ) ( gpointer data );

typedef struct tagSAMPLE_AI_S
{
	HI_BOOL bStart;
	HI_S32  AiDev;
	HI_S32  AiChn;
	HI_S32  AencChn;
	HI_S32  AoDev;
	HI_S32  AoChn;
	HI_BOOL bSendAenc;
	HI_BOOL bSendAo;
	pthread_t stAiPid;
} SAMPLE_AI_S;

typedef enum nf_HI_AUD_CMD_E
{
	HI_CMD_AUD_REC,
	HI_CMD_AUD_REC_GST_BUFFER,
	HI_CMD_AUD_NET,
	HI_CMD_AUD_CFG

} NF_HI_AUD_CMD_E;

typedef enum NF_HI_AUD_INPUT_LV_E
{
	HI_AUD_INPUT_LV0		= 0 ,
	HI_AUD_INPUT_LV1		= 1 ,
	HI_AUD_INPUT_LV2		= 2 ,
	HI_AUD_INPUT_LV3		= 3 ,
	HI_AUD_INPUT_LV4		= 4 ,
	HI_AUD_INPUT_LV5		= 5 ,
	HI_AUD_INPUT_LV6		= 6 ,
	HI_AUD_INPUT_LV7		= 7 ,
	HI_AUD_INPUT_LV8		= 8 ,
	HI_AUD_INPUT_LV9		= 9 ,
	HI_AUD_INPUT_LV10		= 10 ,
	HI_AUD_INPUT_LV11		= 11 ,
	HI_AUD_INPUT_LV12		= 12 ,
	HI_AUD_INPUT_LV13		= 13 ,
	HI_AUD_INPUT_LV14		= 14 ,
	HI_AUD_INPUT_LV15		= 15 
} NF_HI_AUD_INPUT_LV;

typedef struct nf_HI_AUD_QDATA_S
{
	HI_S32	s32Cmd;
	HI_S32	s32Chn;
	HI_S32	s32Len;
	HI_U64	u64Start;
	HI_U64	u64End;
	HI_VOID	*pData;
} NF_HI_AUD_QDATA_S;

typedef struct nf_HI_AUD_INFO_S
{
	NF_HI_AUD_QDATA_S *pstQdata;
	NF_HI_AUD_QDATA_S *pstQdata_net;
	HI_S32 s32StmCnt;
} NF_HI_AUD_INFO_S;

typedef struct _NF_HI_AUD_NET_PB_S
{
	gchar 		data[HI_AUDIO_DATA_CONVERT_SIZE];
	gint		cnt;
} NF_HI_AUD_NET_PB;

typedef struct _NF_HI_AUD_OUTPUT_PB_S
{
	void 		*data;
	gint        len;
} NF_HI_AUD_OUTPUT_PB;

typedef struct nf_HI_AUD_S
{
	NfObject object;
	AI_CHN	 liveChn;

	GAsyncQueue		*queue_net_pb;
	GThread			*thread_net_pb;		/* Thread that for Read a frame. */
	gint 			thread_run_net_pb;		/* Thread run is 1. */

	GAsyncQueue		*queue_output_pb;
	GThread			*thread_output_pb;		
	gint 			thread_run_output_pb;

} NF_HI_AUD_S;

typedef struct nf_HI_AUD_RD_S
{
	NfObject	object;
	GThread		*thread;		/* Thread that for Read a frame. */
	gint 		thread_run;		/* Thread run is 1. */
	gint 		thread_status;	/* Not used */
	gint		init_done;
	GAsyncQueue	*queue;			/* From enc to rec. */
GAsyncQueue	*queue_net;	/* From enc to net immediately. */
	HI_BOOL		isEnable;
	HI_S32		s32DecoderFd;
	HI_S32		s32MaxFd;
	HI_S32		s32AudFd[NUM_AUDIO];
	NF_HI_AUD_INFO_S stInfo[NUM_AUDIO];
	gint		live_ch0;
	#if defined(ENABLE_AUD_STEREO_OUT)
		gint		live_ch1;
	#endif
	gboolean        send_type;
} NF_HI_AUD_RD_S;

typedef struct nf_HI_AUD_PARAM_S
{
	HI_U8  u8Chn;
	HI_U8  u8Reason;
	HI_U8  u8PreRecTime;	// pre_rec_time(sec) 0:normal
	HI_U8  u8PreRecClose;	// pre_rec 0:none,discard 1:flush
	HI_S32 s32StmId;
	HI_U64 u64Timestamp;
	GTimeVal timestamp;         // For Gst Buffer Put
} NF_HI_AUD_PARAM_S;

typedef struct nf_HI_AUD_REC_S
{
	NfObject	object;
	GThread		*thread;		/* Thread that for Read a frame. */
	gint 		thread_run;		/* Thread run is 1. */
	gint 		thread_status;	/* Not used */
	gint		init_done;
	GAsyncQueue	*queue;			/* From enc to rec. */
	HI_BOOL		isEnable;
	NF_HI_AUD_PARAM_S 		stParam[NUM_ACTIVE_CH];
	HI_U32					u32HandoffChnMask;
	NF_HI_AUD_HANDOFF_FUNC	handoffFunc;
} NF_HI_AUD_REC_S;

typedef enum _NF_HI_AUD_OUTPUT_TYPE_E
{
	AUD_OUTPUT_RCA			= 0,
	AUD_OUTPUT_HDMI			= 1,
	AUD_OUTPUT_AUDO			= 2,

	AUD_OUTPUT_UNKNOWN		= 3,
}_NF_HI_AUD_OUTPUT_TYPE_E;

void nf_HI_aud_setLiveChn( AI_CHN AiChn );
AI_CHN nf_HI_aud_getLiveChn( void );
void nf_HI_aud_get_attr(AIO_ATTR_S *stAioAttr, AUDIO_DEV *AiDev, AUDIO_DEV *AoDev,
							AO_CHN *AoChn, ADEC_CHN *AdChn, PAYLOAD_TYPE_E *enPayloadType, guint audio_output_type);
gboolean nf_HI_aud_init(void);
HI_BOOL nf_HI_AUD_initAenc(HI_S32 s32AencChnCnt, PAYLOAD_TYPE_E enType, AUDIO_DEV AiDev, HI_BOOL bUserGetMode);
HI_S32 nf_HI_aud_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn);
HI_S32 nf_HI_aud_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn);
HI_S32 nf_HI_aud_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn);
HI_BOOL nf_HI_aud_start( NF_REC_AUDIO_PARAM *pstParam );
HI_BOOL nf_HI_aud_startAdecAo(void);
HI_BOOL nf_HI_aud_stopAdecAo(ADEC_CHN AdChn);
HI_S32 nf_HI_aud_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, HI_BOOL bAnrEn, HI_BOOL bResampleEn);
HI_BOOL nf_HI_aud_startAiAo(AUDIO_DEV AoDev, AO_CHN AoChn, AUDIO_DEV AiDev, AI_CHN AiChn);
HI_BOOL nf_HI_aud_stopAiAo(AUDIO_DEV AoDev, AO_CHN AoChn, AUDIO_DEV AiDev, AI_CHN AiChn, HI_BOOL bResampleEn);
HI_BOOL nf_HI_aud_registerHandoff(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc);
HI_BOOL nf_HI_aud_ctrNet( NF_HI_AUD_PARAM_S *pstOld,  NF_HI_AUD_PARAM_S *pstNew );
#if defined(ENABLE_REC_NET_IMMEDIATELY)
	HI_BOOL nf_HI_aud_registerHandoff_immediately(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc);
#endif
HI_BOOL nf_HI_aud_ai_start_stop(gboolean is_start);
#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
	guchar nf_HI_lpcm16_to_muraw(short  pcm_val);
	short nf_HI_search_seg(short val, short *table, short size);
	short nf_HI_muraw_to_lpcm16(guchar u_val);
	void nf_HI_aud_convert(gchar *stream_src, gchar *stream_dest, guint len);
#endif
#if defined(ENABLE_AUD_STEREO_OUT)
	void nf_HI_aud_set_live_audio_ch_stereo(guint ch0, guint ch1);
#endif
void nf_HI_aud_set_live_audio_ch(guint ch);
void _nf_hi_aud_reset(gboolean is_reset_ao, gboolean is_start);

int playback_audio_cb(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type);
int playback_audio_cb_net(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, int hi_aud_stm_cnt);

#endif /* __NF_HI_AUD_H__ */

