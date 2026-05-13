#ifndef __NF_AUDIO_H__
#define __NF_AUDIO_H__

#include <libsst.h>

// Define
#define NF_AUDIO_CONVERT_LPCM16_TO_URAW

/* type macro */
#define NF_TYPE_NF_AUDIO					(nf_audio_get_type ())

#define NF_IS_HI_AUD(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NF_TYPE_NF_AUDIO))
#define NF_IS_HI_AUD_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_NF_AUDIO))

#define NF_AUDIO_GET_CLASS(obj)				(G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_NF_AUDIO, NfAudioClass))
#define NF_AUDIO(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj), NF_TYPE_NF_AUDIO, NfAudio))
#define NF_AUDIO_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_NF_AUDIO, NfAudioClass))

#define NF_AUDIO_CAST(obj)					((NfAudio*)(obj))
#define NF_AUDIO_CLASS_CAST(klass)			((NfAudioClass*)(klass))

typedef struct _NfAudio         NfAudio;
typedef struct _NfAudioClass    NfAudioClass;

#define NF_AUD_TIME_UNIT				(1020000)       // 1020000usec
#if defined(CHIP_NVT_NA51039)
#define NF_AUD_PINPERFRM_ITX			8160
#define NF_AUD_PINPERFRM				816			// CPU Chip's frame
#define NF_AUD_PINPERFRM_CNT			(20)
#elif defined(CHIP_NVT_NT9833x)
#define NF_AUD_PINPERFRM_ITX			8320//8000
#define NF_AUD_PINPERFRM				832//640			// CPU Chip's frame
#define NF_AUD_PINPERFRM_CNT			(20)//(25)
#else
#define NF_AUD_PINPERFRM_ITX			8160
#define NF_AUD_PINPERFRM				2048		// CPU Chip's frame
#define NF_AUD_PINPERFRM_CNT			(8)			// 1.02sec/(1/8000)sec*480
#endif
#define NF_AUD_SIZE_SEND_DATA_FRM		(NF_AUD_PINPERFRM / sizeof(gushort))		// for uraw data
#define NF_AUD_SIZE_SEND_DATA_TOTAL		(NF_AUD_SIZE_SEND_DATA_FRM * NF_AUD_PINPERFRM_CNT)	// Total Send Data

#define NF_AUD_NET_VLC_DEVISION			(2)
#define NF_AUD_SIZE_VLC					(NF_AUD_SIZE_SEND_DATA_FRM / NF_AUD_NET_VLC_DEVISION)

// for network mic
#define NF_AUD_PINPERFRM_CNT_WEB		1
#define NF_AUDIO_DATA_SIZE_WEB_MIC		(800)		// uraw data
#define NF_AUDIO_DATA_SIZE_WEB_MIC_PCM	(NF_AUDIO_DATA_SIZE_WEB_MIC * sizeof(gushort))
#define NF_AUD_DATA_SEND_SIZE_WEB		(NF_AUDIO_DATA_SIZE_WEB_MIC * NF_AUD_PINPERFRM_CNT_WEB)

#define NF_AUDIO_NET_PB_MAX_QUEUE		100

// for issm module
#define NF_AUD_PINPERFRM_CNT_NET		(1)
#define NF_AUDIO_DATA_SEND_SIZE			(NF_AUDIO_DATA_SIZE_WEB_MIC * NF_AUD_PINPERFRM_CNT_NET)

#define NF_AUDIO_DAC_PLAYBACK			(0xff)


#if defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
	#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))
#endif

#define DBG_MSG_NONE					(0)
#define DBG_MSG_AUD						(1)

#define NF_AUD_DBG(level, fmt...)   \
	do{\
		if(DBG_MSG_AUD == level)\
		{\
			g_message(fmt);\
		}\
	} while(0);

#define DBG_MSG_AUD_SST					DBG_MSG_NONE
#define DBG_MSG_AUD_REC					DBG_MSG_NONE
#define DBG_MSG_AUD_CFG					DBG_MSG_NONE


typedef void ( *NF_AUDIO_HANDOFF_FUNC ) ( gpointer data );

typedef enum _NF_AUD_OUTPUT_E
{
    AUD_OUTPUT_RCA		= 0,
    AUD_OUTPUT_HDMI		= 1,
    AUD_OUTPUT_AUTO		= 2,

    AUD_OUTPUT_UNKNOWN	= 3,
}_NF_AUD_OUTPUT_E;

#if 0
typedef enum _NF_AUDIO_SEND_TYPE_E
{
	NF_AUDIO_SEND_DVR               = 0,
	NF_AUDIO_SEND_IPCAM             = 1
} NF_AUDIO_SEND_TYPE;

typedef enum _HI_AUDIO_DEF_E
{
	NF_AUDIO_NET_INS_DEF_SPEED      = NF_AUD_SIZE_SEND_DATA_FRM,
} HI_AUDIO_DEF_E;
#endif

typedef enum NF_AUDIO_CMD_E
{
	AUD_CMD_REC					= 0,
	AUD_CMD_CFG					= 1,

	AUD_CMD_NET_VLC_HANDOFF		= 5,
	AUD_CMD_NET_VLC_CFG			= 6,
	AUD_CMD_REC_GST_BUFFER		= 7

} NF_AUDIO_CMD;

typedef enum NF_AUDIO_INPUT_LV_E
{
	AUD_INPUT_LV0		= 0 ,
	AUD_INPUT_LV1		= 1 ,
	AUD_INPUT_LV2		= 2 ,
	AUD_INPUT_LV3		= 3 ,
	AUD_INPUT_LV4		= 4 ,
	AUD_INPUT_LV5		= 5 ,
	AUD_INPUT_LV6		= 6 ,
	AUD_INPUT_LV7		= 7 ,
	AUD_INPUT_LV8		= 8 ,
	AUD_INPUT_LV9		= 9 ,
	AUD_INPUT_LV10		= 10 ,
	AUD_INPUT_LV11		= 11 ,
	AUD_INPUT_LV12		= 12 ,
	AUD_INPUT_LV13		= 13 ,
	AUD_INPUT_LV14		= 14 ,
	AUD_INPUT_LV15		= 15 
} NF_AUDIO_INPUT_LV;

typedef enum _DEBUG_NF_AUDIO_IDX_E
{
	DEBUG_NF_AUDIO_IDX_START			= 0,
	DEBUG_NF_AUDIO_IDX_THREAD_RD		= 2,
	DEBUG_NF_AUDIO_IDX_THREAD_REC		= 3,
	DEBUG_NF_AUDIO_IDX_THREAD_PB		= 4,
	DEBUG_NF_AUDIO_IDX_THREAD_NET_VLC	= 5,
	DEBUG_NF_AUDIO_IDX_HANDOFF			= 6,
	DEBUG_NF_AUDIO_IDX_HANDOFF_VLC		= 7,
	DEBUG_NF_AUDIO_IDX_SST				= 8,

	DEBUG_NF_AUDIO_IDX_NR
} DEBUG_NF_AUDIO_IDX_E;

#if 0
typedef struct _AUDIO_QDATA_S
{
	guint	s32Cmd;
	guint	s32Chn;
	guint	s32Len;
	gulong	u64Start;
	gulong	u64End;
	void	*pData;
} NF_AUDIO_QDATA_S;
#endif

typedef struct _NF_AUDIO_PB_T
{
	NfObject        object;
	gint			liveChn;

	GAsyncQueue     *queue_pb;
} NF_AUDIO_PB;

typedef struct _NF_AUDIO_PB_OUT_T
{
	gboolean		is_dvr;
	gboolean		is_beep;
	gboolean		is_mic_web;
} NF_AUDIO_PB_OUT;

typedef struct _NF_AUDIO_QDATA_PB_T
{
	#if 0
		gchar			data_web_mic[NF_AUDIO_DATA_CONVERT_SIZE_WEB];
	#endif
	gchar			data[NF_AUD_PINPERFRM];
	gint			cnt;

	gint			outmode;
} NF_AUDIO_QDATA_PB;

typedef struct _NF_AUD_PB_STATUS
{
	#if 0
		gint is_pb_web;
		gint is_pb_beep;
	#endif
	gint running_qc;
	gint running_dvr;
	gint running_web_mic;
	gint running_beep;
	gint running_live;
	gint running_live_hdmi;
} NF_AUD_PB_STATUS;

typedef enum _NF_HI_AUD_PB_OUT_MODE_E
{
	AUD_PB_OUT_MODE_DVR    		= 1,
	AUD_PB_OUT_MODE_WEB    		= 2,
	AUD_PB_OUT_MODE_BEEP   		= 3,
	AUD_PB_OUT_MODE_LIVE		= 4,
	AUD_PB_OUT_MODE_QC			= 5,
	AUD_PB_OUT_MODE_LIVE_HDMI	= 6,

	AUD_PB_OUT_OFF         		= 0,

	AUD_PB_OUT_DVR         		= 1,
	AUD_PB_OUT_WEB_ON      		= 2,
	AUD_PB_OUT_BEEP_ON     		= 3,
	AUD_PB_OUT_LIVE_ON			= 4,
	AUD_PB_OUT_QC_ON			= 5,
	AUD_PB_OUT_LIVE_HDMI_ON		= 6,

} NF_HI_AUD_PB_OUT_MODE;

#if 0
typedef struct _NF_AUDIO_OUTPUT_PB_T
{
	void        *data;
	gint        len;
} NF_AUDIO_OUTPUT_PB;
#endif

typedef struct _NF_AUDIO_QDATA_T
{
	gint	s32Cmd;
	gint	s32Chn;
	gint	s32Len;
	guint64 u64Start;
	guint64 u64Startl;
	guint64 u64End;
	guint64 u64Endl;
	void	*pData;
} NF_AUDIO_QDATA;

typedef struct _NF_AUDIO_INFO_T
{
	NF_AUDIO_QDATA *pstQdata;
	NF_AUDIO_QDATA *pstQdata_net_vlc;
	guint s32StmCnt;
} NF_AUDIO_INFO;

typedef struct _NF_AUDIO_RD
{
	NfObject    object;
	gint        thread_status;  /* Not used */
	gint        init_done;
	GAsyncQueue *queue;         /* From enc to rec. */
	GAsyncQueue *queue_net_vlc;		/* From enc to net vlc. */
	gboolean     isEnable;
	NF_AUDIO_INFO *stInfo;
	gint        live_ch0;
	#if defined(ENABLE_AUD_STEREO_OUT)
		gint        live_ch1;
	#endif
	gboolean    send_type;
} NF_AUDIO_RD;

typedef struct _NF_AUDIO_PARAM_T
{
	guchar  u8Chn;
	guchar  u8Reason;
	guchar  u8PreRecTime;    // pre_rec_time(sec) 0:normal
	guchar  u8PreRecClose;   // pre_rec 0:none,discard 1:flush
	gint	s32StmId;
	gulong u64Timestamp;
	GTimeVal timestamp;         // For Gst Buffer Put
} NF_AUDIO_PARAM;

typedef struct _NF_AUDIO_REC
{
	NfObject		object;
	gint			init_done;
	GAsyncQueue		*queue;         /* From enc to rec. */
	gboolean		isEnable;
	NF_AUDIO_PARAM	stParam[NUM_ACTIVE_CH];
	unsigned int	u32HandoffChnMask;
	NF_AUDIO_HANDOFF_FUNC	handoffFunc;
} NF_AUDIO_REC;

typedef struct _NF_AUDIO_NET_VLC
{
	NfObject		object;
	gint			init_done;
	GAsyncQueue		*queue;         /* From enc to rec. */
	gboolean		isEnable;
	NF_AUDIO_PARAM	stParam[NUM_ACTIVE_CH];
	unsigned int	u32HandoffChnMask;
	NF_AUDIO_HANDOFF_FUNC	handoffFunc;
} NF_AUDIO_NET_VLC;

#if defined(ENABLE_AI_ALARM_AUDIO)
#define NF_REC_AUDIO_STR_256        256
#define NF_REC_AUDIO_AI_MAX_EVT_CNT 16
typedef struct _NF_AUDIO_DATA_AI_ALARM_T {

	guint       ai_evt_cnt;
	gboolean    is_playing[NUM_ACTIVE_CH];
	guint       mask_ai_evt[NF_REC_AUDIO_AI_MAX_EVT_CNT][NUM_ACTIVE_CH];
	gint        ai_evt_rule[NF_REC_AUDIO_AI_MAX_EVT_CNT][NUM_ACTIVE_CH];
	guint       mask_ai_evt_test;
	guint       ai_evt_aud_test;
	gchar       ai_evt_aud_test_filename[NF_REC_AUDIO_STR_256];
	guint       ai_aud_size_remain[NUM_ACTIVE_CH];
	guint       ai_aud_size_send;

	FILE        *fp[NUM_ACTIVE_CH];

} NF_AUD_DATA_AI_ALARM;
#endif

/**
 * NfHiAud:
 *
 * NfDVR notify class
 */
struct _NfAudio {
	NfObject            object;

	GMainContext        *context;
	GMainLoop           *loop;

	gint                init_done;
	#if defined(ENABLE_AI_ALARM_AUDIO)
		gint				init_done_ai_alarm;
	#endif

	GThread             *thread_rec;
	GThread             *thread_rd;
	GThread             *thread_pb;
	GThread             *thread_net_vlc;
	GThread             *thread_net_pb;
	GThread             *thread_pb_out;
	GThread             *thread_volume;
	#if defined(ENABLE_AI_ALARM_AUDIO)
		GThread             *thread_ai_alarm;
	#endif

	GStaticMutex		mutex_pb;

	gint                thread_run_rec;
	gint                thread_run_rd;
	gint                thread_run_pb;
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		gint                thread_run_net_vlc;
	#endif
	gint                thread_run_net_pb;
	gint                thread_run_pb_out;
	gint                thread_run_volume;
	#if defined(ENABLE_AI_ALARM_AUDIO)
		gint                thread_run_ai_alarm;
	#endif

	GAsyncQueue         *queue_net_pb;

	NF_AUDIO_PB        stAudPb;
	NF_AUDIO_RD        stAudRd;
	NF_AUDIO_REC       stAudRec;
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		NF_AUDIO_NET_VLC	stAudNetVlc;
	#endif

	// Novatec
	#if defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
		AUDIO_RECORD		info_rec;
		AUDIO_PLAYBACK		info_pb;
	#else
		AUDIO_CAPONLY 		caponly;
		AUDIO_PLAYBACK		stream_pb;
	#endif

	NF_AUD_PB_STATUS	aud_status_pb;

	#if defined(ENABLE_AI_ALARM_AUDIO)
		NF_AUD_DATA_AI_ALARM  aud_data_ai;
	#endif

	gint                aud_output_type;
	gint                fdAcodec;

	volatile guint      vin_mask;
	gint                table_input[16];

	guint				volume_in;
	guint				volume_out;
	guint				volume_in_mute;
	guint				volume_out_mute;

	guint				audio_nr;

	gint				live_ch;		// For Live Test
	guint				dvr_status;

	gboolean			dbg_pb_out;
	gboolean			dbg_pb;

	gboolean			enable_ai_alarm;
}__attribute__((packed));

struct _NfAudioClass {
	NfObjectClass   parent_class;

	/* signals */

	/*< public >*/

	/*< private >*/

}__attribute__((packed));

NF_AUDIO_RD *nf_audio_getRd(void);
gboolean nf_audio_isRdEnable(void);
void nf_audio_setRdEnable(gboolean bValue);
NF_AUDIO_REC *nf_audio_getRec(void);
GAsyncQueue *nf_audio_getRecQueue(void);
NF_AUDIO_PARAM *nf_audio_getRecParam(void);
void nf_audio_setRecHandoff(unsigned int u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc);
#if defined(ENABLE_AUDIO_HANDOFF_VLC)
NF_AUDIO_NET_VLC *nf_audio_getNetVlc(void);
GAsyncQueue *nf_audio_getNetVlcQueue(void);
NF_AUDIO_PARAM *nf_audio_getNetVlcParam(void);
#endif
NF_AUDIO_QDATA *nf_audio_CreateQdata_gst_buffer(GObject *buffer);
NF_AUDIO_QDATA *nf_audio_CreateQdata( unsigned int s32Len );
static gboolean _nf_audio_freeQdata_gst_buffer(NF_AUDIO_QDATA *pstQdata);
gboolean nf_audio_freeQdata(NF_AUDIO_QDATA *pstQdata);
gboolean nf_audio_sendQdata(NF_AUDIO_QDATA *pstQdata, GAsyncQueue *pQue);
#if defined(ENABLE_GST_VERSION_UP)
	#if defined(CHIP_NVT_NT9833x)
		GObject *nf_audio_gst_buffer(gint size, gboolean verbose);
	#endif
#else
	#if defined(CHIP_NVT_NT9833x)
		GObject *nf_audio_gst_buffer(gint size, gboolean verbose);
	#endif
#endif
gboolean nf_audio_init(void);
gboolean nf_audio_start(NF_REC_AUDIO_PARAM *pstParam);
gboolean nf_audio_registerHandoff(unsigned int u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc);
gboolean nf_audio_ctrNetVlc(NF_AUDIO_PARAM *pstOld,  NF_AUDIO_PARAM *pstNew);
#if defined(ENABLE_AUDIO_HANDOFF_VLC)
    gboolean nf_audio_registerHandoff_vlc(unsigned int u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc);
#endif
gboolean nf_audio_ai_start_stop(gboolean is_start);
gboolean nf_audio_set_live_audio_ch(int ch);
void nf_audio_reset(gboolean is_hdmi, gboolean is_reset_ao, gboolean is_start);

void nf_audio_set_pb_status(int pb_mode, gboolean is_running);

int nf_audio_pb_cb_local(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type);
int nf_audio_pb_cb_beep(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type);
int nf_audio_pb_cb_web_mic(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						int hi_aud_stm_cnt, gboolean is_buffer_clear);
void nf_audio_rearrange_tbl_input(gint num_audio);
gboolean nf_audio_ctrl_vol_adc(gint is_mute, guint volume);
gboolean nf_audio_ctrl_vol_dac(gint is_mute, guint volume);
#if defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
unsigned int nf_audio_get_current_time(void);
#endif
#if defined(ENABLE_AI_ALARM_AUDIO)
void nf_audio_ai_alarm_test(guint mask_alarm_ch, char *str);
#endif
void nf_audio_dump_icodec_header_extern(int idx, ICODEC_HEADER *pheader, int ch);

#endif /* __NF_AUDIO_H__ */

