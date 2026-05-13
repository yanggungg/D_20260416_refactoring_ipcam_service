#ifndef __NF_HI_AUD_3536_H__
#define __NF_HI_AUD_3536_H__

#include "nf_common.h"
#include "nf_HI_common.h"
#include "nf_object.h"

#define NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW     // pakkhman

#define HI_DBG_AUD(level, fmt...)   \
	do{\
		if(DBG_MSG_AUD == level)\
		{\
			g_message(fmt);\
		}\
	} while(0);

#define NF_HI_AUDIO_SST_START_CH_NO			(64)

#define DBG_MSG_AUD_CFG						DBG_MSG_NONE
#define DBG_MSG_AUD_REC						DBG_MSG_NONE

#define HI_AUD_TIME_UNIT					(1020000)       // 1020000usec
#define HI_AUD_STM_CNT						(17)            // 1.02sec/(1/8000)sec*480
#if 0
	#define HI_AUDIO_PTNUMPERFRM				480
#else
	#define HI_AUDIO_PTNUMPERFRM				320
#endif

#define HI_AUD_TOTAL_SIZE					(HI_AUDIO_PTNUMPERFRM * HI_AUD_STM_CNT)         // 480 * HI_AUD_STM_CNT

// for network mic
#define HI_AUD_STM_CNT_NET					(1)
#if 0
	#define HI_AUDIO_SIZE_STREAM				(960)
#else
	#define HI_AUDIO_SIZE_STREAM				(640)
#endif
#define HI_AUDIO_DATA_WEB_SIZE				(800)
#define HI_AUDIO_DATA_SEND_SIZE				(HI_AUDIO_DATA_WEB_SIZE * HI_AUD_STM_CNT_NET)
#define HI_AUDIO_DATA_CONVERT_SIZE			((HI_AUDIO_DATA_WEB_SIZE * HI_AUD_STM_CNT_NET) * 2)
#define HI_AUDIO_NET_PB_MAX_QUEUE			50

#define SIO_MAX_NUM							6

/* type macro */
#define NF_TYPE_HI_AUD						(nf_HI_aud_get_type ())

#define NF_IS_HI_AUD(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NF_TYPE_HI_AUD))
#define NF_IS_HI_AUD_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_HI_AUD))

#define NF_HI_AUD_GET_CLASS(obj)				(G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_HI_AUD, NfHiaudClass))
#define NF_HI_AUD(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj), NF_TYPE_HI_AUD, NfHiaud))
#define NF_HI_AUD_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_HI_AUD, NfHiaudClass))

#define NF_HI_AUD_CAST(obj)					((NfHiaud*)(obj))
#define NF_HI_AUD_CLASS_CAST(klass)			((NfHiaudClass*)(klass))

typedef struct _NfHiaud			NfHiaud;
typedef struct _NfHiaudClass	NfHiaudClass;


typedef char*					HI_PCHAR;
typedef void*					HI_Ptr;       /* data pointer */


/* Audio handoff */
typedef void ( *NF_HI_AUD_HANDOFF_FUNC ) ( gpointer data );

/* Definition Struct */
typedef struct _NF_HI_AUD_PB_T
{
	NfObject		object;
	AI_CHN			liveChn;

	GAsyncQueue     *queue_net_pb;
//	GThread         *thread_net_pb;     /* Thread that for Read a frame. */
//	gint            thread_run_net_pb;      /* Thread run is 1. */

	GAsyncQueue     *queue_output_pb;
	GThread         *thread_output_pb;
//	gint            thread_run_output_pb;

} NF_HI_AUD_PB;

typedef struct _NF_HI_AUD_NET_PB_T
{
	gchar       data[HI_AUDIO_DATA_CONVERT_SIZE];
	gint        cnt;
} NF_HI_AUD_NET_PB;

typedef struct _NF_HI_AUD_OUTPUT_PB_T
{
	void        *data;
	gint        len;
} NF_HI_AUD_OUTPUT_PB;

#if 0
typedef struct _NF_HI_AUD_T
{
	NfObject object;
	AI_CHN   liveChn;

	GAsyncQueue     *queue_net_pb;
	GThread         *thread_net_pb;     /* Thread that for Read a frame. */
	gint            thread_run_net_pb;      /* Thread run is 1. */

	GAsyncQueue     *queue_output_pb;
	GThread         *thread_output_pb;
	gint            thread_run_output_pb;

} NF_HI_AUD_T;
#endif

typedef struct _NF_HI_AUD_QDATA_T
{
	gint  s32Cmd;
	gint  s32Chn;
	gint  s32Len;
	guint64 u64Start;
	guint64 u64End;
	HI_VOID *pData;
} NF_HI_AUD_QDATA;

typedef struct _NF_HI_AUD_INFO_T
{
	NF_HI_AUD_QDATA *pstQdata;
	NF_HI_AUD_QDATA *pstQdata_net;
	HI_S32 s32StmCnt;
} NF_HI_AUD_INFO;

typedef struct _NF_HI_AUD_RD
{
	NfObject    object;
//	GThread     *thread;        /* Thread that for Read a frame. */
//	gint        thread_run;     /* Thread run is 1. */
	gint        thread_status;  /* Not used */
	gint        init_done;
	GAsyncQueue *queue;         /* From enc to rec. */
	GAsyncQueue *queue_net; /* From enc to net immediately. */
	HI_BOOL     isEnable;
	HI_S32      s32DecoderFd;
	HI_S32      s32MaxFd;
	HI_S32      s32AudFd[NUM_AUDIO];
	NF_HI_AUD_INFO stInfo[NUM_AUDIO];
	gint        live_ch0;
	#if defined(ENABLE_AUD_STEREO_OUT)
		gint        live_ch1;
	#endif
	gboolean	send_type;
} NF_HI_AUD_RD;

typedef struct nf_HI_AUD_PARAM
{
	HI_U8  u8Chn;
	HI_U8  u8Reason;
	HI_U8  u8PreRecTime;    // pre_rec_time(sec) 0:normal
	HI_U8  u8PreRecClose;   // pre_rec 0:none,discard 1:flush
	HI_S32 s32StmId;
	HI_U64 u64Timestamp;
	GTimeVal timestamp;         // For Gst Buffer Put
} NF_HI_AUD_PARAM;

typedef struct _NF_HI_AUD_REC
{
	NfObject    object;
//	GThread     *thread;        /* Thread that for Read a frame. */
//	gint        thread_run;     /* Thread run is 1. */
//	gint        thread_status;  /* Not used */
	gint        init_done;
	GAsyncQueue *queue;         /* From enc to rec. */
	HI_BOOL     isEnable;
	NF_HI_AUD_PARAM       stParam[NUM_ACTIVE_CH];
	HI_U32                  u32HandoffChnMask;
	NF_HI_AUD_HANDOFF_FUNC  handoffFunc;
} NF_HI_AUD_REC;


/* Definition Enum */
typedef enum NF_HI_AUD_INPUT_LV_E
{
	HI_AUD_INPUT_LV0        = 0 ,
	HI_AUD_INPUT_LV1        = 1 ,
	HI_AUD_INPUT_LV2        = 2 ,
	HI_AUD_INPUT_LV3        = 3 ,
	HI_AUD_INPUT_LV4        = 4 ,
	HI_AUD_INPUT_LV5        = 5 ,
	HI_AUD_INPUT_LV6        = 6 ,
	HI_AUD_INPUT_LV7        = 7 ,
	HI_AUD_INPUT_LV8        = 8 ,
	HI_AUD_INPUT_LV9        = 9 ,
	HI_AUD_INPUT_LV10       = 10 ,
	HI_AUD_INPUT_LV11       = 11 ,
	HI_AUD_INPUT_LV12       = 12 ,
	HI_AUD_INPUT_LV13       = 13 ,
	HI_AUD_INPUT_LV14       = 14 ,
	HI_AUD_INPUT_LV15       = 15
} NF_HI_AUD_INPUT_LV;

typedef enum _NF_HI_AUD_OUTPUT_TYPE_E
{
	AUD_OUTPUT_RCA          = 0,
	AUD_OUTPUT_HDMI         = 1,
	AUD_OUTPUT_AUDO         = 2,

	AUD_OUTPUT_UNKNOWN      = 3,
}_NF_HI_AUD_OUTPUT_TYPE_E;

typedef enum _NF_HI_AUD_CMD_E
{
	HI_CMD_AUD_REC,
	HI_CMD_AUD_REC_GST_BUFFER,
	HI_CMD_AUD_NET,
	HI_CMD_AUD_CFG

} NF_HI_AUD_CMD;


// Hisilicon Header File
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


/**
 * NfEvent:
 *
 * NfDVR notify class
 */
struct _NfHiaud {
	NfObject            object;

	GMainContext        *context;
	GMainLoop           *loop;

	gint				init_done;

	GThread             *thread_rec;
	GThread             *thread_rd;
	GThread             *thread_net;
	GThread             *thread_net_pb;
	GThread             *thread_output_pb;

	gint				thread_run_rec;
	gint				thread_run_rd;
	gint				thread_run_net;
	gint				thread_run_net_pb;
	gint				thread_run_output_pb;

	GAsyncQueue     	*queue_net_pb;

//	NF_HI_AUD_T			stAud_t;
	NF_HI_AUD_PB		stAudPb;
	NF_HI_AUD_RD		stAudRd;
	NF_HI_AUD_REC		stAudRec;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		NF_HI_AUD_REC		stAudNet;
	#endif

	gint				aud_output_type;
	gint				fdAcodec;

	volatile guint		vin_mask;

}__attribute__((packed));

struct _NfHiaudClass {
	NfObjectClass   parent_class;

	/* signals */

	/*< public >*/

	/*< private >*/

}__attribute__((packed));

//typedef NF_REC_AUDIO_PARAM NF_HI_REC_AUDIO_PARAM;

gboolean nf_HI_aud_init(void);
//HI_BOOL nf_HI_aud_start( NF_REC_AUDIO_PARAM *pstParam );
HI_BOOL nf_HI_aud_start(GValue *data);
//HI_BOOL nf_HI_aud_start( NF_HI_REC_AUDIO_PARAM *pstParam );

int playback_audio_cb(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type);
int playback_audio_cb_net(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, int hi_aud_stm_cnt);
HI_BOOL nf_HI_aud_getFd(HI_S32 ps32AudFd[], HI_S32 *ps32MaxFd);
NF_HI_AUD_RD *nf_HI_aud_getRd(void);
HI_BOOL nf_HI_aud_isRdEnable(void);
void nf_HI_aud_setRdEnable(HI_BOOL bValue);
NF_HI_AUD_REC *nf_HI_aud_getRec(void);
GAsyncQueue *nf_HI_aud_getRecQueue(void);
NF_HI_AUD_PARAM *nf_HI_aud_getRecParam(void);
#if defined(ENABLE_REC_NET_IMMEDIATELY)
NF_HI_AUD_REC *nf_HI_aud_getNet(void);
GAsyncQueue *nf_HI_aud_getNetQueue(void);
NF_HI_AUD_PARAM_S *nf_HI_aud_getNetParam(void);
void nf_HI_aud_setNetHandoff( HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc );
#endif
void nf_HI_aud_setRecHandoff( HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc );
HI_BOOL nf_HI_aud_registerHandoff(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc);
HI_BOOL nf_HI_aud_registerHandoff_immediately(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc);
HI_BOOL nf_HI_aud_sendQdata( NF_HI_AUD_QDATA *pstQdata, GAsyncQueue *pQue);
HI_BOOL nf_HI_aud_putSST( HI_S32 s32Chn, NF_HI_AUD_QDATA *pstQdata, NF_HI_AUD_PARAM* pstParam );
HI_BOOL nf_HI_aud_ctrlSST( NF_HI_AUD_PARAM *pstOld,  NF_HI_AUD_PARAM *pstNew );
HI_BOOL nf_HI_aud_ctrNet( NF_HI_AUD_PARAM *pstOld,  NF_HI_AUD_PARAM *pstNew );
HI_BOOL nf_HI_aud_handoff( NF_HI_AUD_REC *stAudRec, NF_HI_AUD_QDATA *pstQdata );
HI_BOOL nf_HI_aud_handoff_gst_buffer( NF_HI_AUD_REC *stAudRec, NF_HI_AUD_QDATA *pstQdata );
static gboolean _nf_HI_aud_putSST_put_gst_buffer( guint ch, gpointer frame, NF_HI_AUD_PARAM *pstParam);
gboolean nf_HI_aud_put_frame(gint ch_num, gpointer frame );

static void _nf_HI_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader);
static void nf_HI_aud_nf_HI_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader);

#if defined(ENABLE_AUD_STEREO_OUT)
	void nf_HI_aud_set_live_audio_ch_stereo(guint ch0, guint ch1);
#endif
void nf_HI_aud_set_live_audio_ch(guint ch);

#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
	guchar nf_HI_lpcm16_to_muraw(short  pcm_val);
	short nf_HI_search_seg(short val, short *table, short size);
	short nf_HI_muraw_to_lpcm16(guchar u_val);
	void nf_HI_aud_convert(gchar *stream_src, gchar *stream_dest, guint len);
#endif

gboolean nf_HI_aud_cntl_acodec_set_input_volume(gint volume);
gboolean nf_HI_aud_cntl_acodec_get_input_volume(gint *volume);
gboolean nf_HI_aud_cntl_acodec_set_output_volume(gint volume);
gboolean nf_HI_aud_cntl_acodec_get_output_volume(gint *volume);
gboolean nf_HI_aud_cntl_acodec_set_gain_micl(guint volume);	
gboolean nf_HI_aud_cntl_acodec_set_gain_micr(guint volume);
gboolean nf_HI_aud_cntl_acodec_set_dacl_vol(guint volume);
gboolean nf_HI_aud_cntl_acodec_set_dacr_vol(guint volume);
gboolean nf_HI_aud_cntl_acodec_set_adcl_vol(guint volume);
gboolean nf_HI_aud_cntl_acodec_set_adcr_vol(guint volume);
gboolean nf_HI_aud_cntl_acodec_set_micl_mute(gboolean is_mute);
gboolean nf_HI_aud_cntl_acodec_set_micr_mute(gboolean is_mute);
gboolean nf_HI_aud_cntl_acodec_set_dacl_mute(gboolean is_mute);
gboolean nf_HI_aud_cntl_acodec_set_dacr_mute(gboolean is_mute);

#endif


