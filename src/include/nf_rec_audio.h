#ifndef __NF_REC_AUDIO_AUDIO_H__
#define __NF_REC_AUDIO_AUDIO_H__

#include "nf_object.h"
#include "nf_common.h"
#include "nf_codec_header.h"

//#define ENABLE_DEBUG_IMMEDIATELY_AUDIO
typedef enum _NF_AUDIO_DEF_E
{
	NF_AUDIO_DEF_SPEED 		= 8160,		// 1.02
	#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0824P4E) || \
		defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
		NF_AUDIO_DEF_CHANNEL	= 1,
	#else
		NF_AUDIO_DEF_CHANNEL	= 4,
	#endif
	NF_AUDIO_DEF_STEREO		= 2,
	NF_AUDIO_DEF_MONO 		= 1,
	NF_AUDIO_DEF_RX_BIT 	= 8,
	NF_AUDIO_DEF_TX_BIT 	= 16,
} NF_AUDIO_DEF_E;

typedef enum _NF_AUDIO_QITEM_TYPE_E
{
	NF_AUDIO_QITEM_TYPE_FRAME,
	NF_AUDIO_QITEM_TYPE_CMD,
	NF_AUDIO_QITEM_TYPE_GST_BUFFER,
	
}NF_AUDIO_QITEM_TYPE_E;

typedef struct _NF_AUDIO_QITEM_T
{
	gint		type;
	GTimeVal	req_timeval;
	GTimeVal	ret_timeval;
	gchar		*buff;
	gint		buff_len;
} NF_AUDIO_QITEM;

typedef enum _NF_AUDIO_INPUT_CH_E
{
	NF_AUDIO_INPUT_CH01,
	NF_AUDIO_INPUT_CH02,
	NF_AUDIO_INPUT_CH03,
	NF_AUDIO_INPUT_CH04,
	NF_AUDIO_INPUT_CH05,
	NF_AUDIO_INPUT_CH06,
	NF_AUDIO_INPUT_CH07,
	NF_AUDIO_INPUT_CH08,
	NF_AUDIO_INPUT_CH09,
	NF_AUDIO_INPUT_CH10,
	NF_AUDIO_INPUT_CH11,
	NF_AUDIO_INPUT_CH12,
	NF_AUDIO_INPUT_CH13,
	NF_AUDIO_INPUT_CH14,
	NF_AUDIO_INPUT_CH15,
	NF_AUDIO_INPUT_CH16,
	NF_AUDIO_INPUT_CHNR,
	NF_AUDIO_INPUT_OFF = 0xff
} NF_AUDIO_INPUT_CH_E;

typedef struct _NF_REC_AUDIO_PARAM_T
{
	guchar	ch_arr       [64];
	guchar	rec_reason   [64];
	guchar	pre_rec_time [64];	// pre_rec_time(sec) 0:normal
	guchar  pre_rec_close[64];	// pre_rec 0:none,discard 1:flush
	gboolean        send_type;
} NF_REC_AUDIO_PARAM;

typedef enum _NF_AUDIO_SEND_TYPE_E
{
	   NF_AUDIO_SEND_DVR               = 0,
	   NF_AUDIO_SEND_IPCAM             = 1
} NF_AUDIO_SEND_TYPE;

// audio hand off
typedef void (*NF_REC_AUDIO_HANDOFF_FUNC) ( gpointer data ); 
#if defined(ENABLE_REC_NET_IMMEDIATELY)
	typedef void (*NF_REC_AUDIO_HANDOFF_FUNC_IMMEDIATELY) ( gpointer data ); 
#endif

#define NF_AUDIO_SST_START_CH_NO	(32)

	#define MAX_AUDIO_FRAME_SIZE		(64*1024*2)
#if defined(ENABLE_REC_LIVE_AUDIO_HI)
	#define NF_REC_AUDIO_DAC_PLAYBACK		(0xff)
#endif

/* type macro */
#define NF_TYPE_REC_AUDIO				(nf_rec_audio_get_type())

#define NF_IS_REC_AUDIO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_REC_AUDIO))
#define NF_IS_REC_AUDIO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_REC_AUDIO))

#define NF_REC_AUDIO_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_REC_AUDIO, NfRecAudioClass))
#define NF_REC_AUDIO(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_REC_AUDIO, NfRecAudio))
#define NF_REC_AUDIO_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_REC_AUDIO, NfRecAudioClass))

#define NF_REC_AUDIO_CAST(obj)			((NfRecAudio*)(obj))
#define NF_REC_AUDIO_CLASS_CAST(klass)	((NfRecAudioClass*)(klass))

typedef struct _NfRecAudio 			NfRecAudio;
typedef struct _NfRecAudioClass 	NfRecAudioClass;

#if defined(ENABLE_REC_LIVE_AUDIO)
#define NF_REC_AUDIO_DAC_PLAYBACK		(0xff)
#define NF_REC_AUDIO_DAC_NET_AUDIO		(0xfe)
#define NF_REC_AUDIO_DEF_SPEED_DEVIDE	20
#define NF_REC_AUDIO_LIMIT_PB_DELAY		1000

typedef struct _NF_REC_AUDIO_DATA_T {
	guint		live;
	guint		live_ch;
	gboolean	tx;
	gboolean	rx;
	gboolean	keybuzzer;
	guint		in_ch[NUM_AUDIO];
} NF_REC_AUDIO_DATA;
#endif

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
#define NF_REC_AUDIO_STR_256		256
#define NF_REC_AUDIO_AI_MAX_EVT_CNT	16
typedef struct _NF_REC_AUDIO_DATA_AI_ALARM_T {

	guint		ai_evt_cnt;
	gboolean	is_playing[NUM_ACTIVE_CH];
	guint		mask_ai_evt[NF_REC_AUDIO_AI_MAX_EVT_CNT][NUM_ACTIVE_CH];
	gint		ai_evt_rule[NF_REC_AUDIO_AI_MAX_EVT_CNT][NUM_ACTIVE_CH];
	guint		mask_ai_evt_test;
	guint		ai_evt_aud_test;
	gchar		ai_evt_aud_test_filename[NF_REC_AUDIO_STR_256];
	guint		ai_aud_size_remain[NUM_ACTIVE_CH];
	guint		ai_aud_size_send;

	FILE		*fp[NUM_ACTIVE_CH];

} NF_REC_AUDIO_DATA_AI_ALARM;
#endif

#if defined(ENABLE_REC_NET_IMMEDIATELY)
	#define NF_REC_AUDIO_DEF_NET_SPEED_DEVIDE	(NF_AUDIO_DEF_SPEED / NF_AUDIO_NET_INS_DEF_SPEED)
#endif

/**
 * NfRecAudio:
 *
 * NfDVR notify class
 */
struct _NfRecAudio {
	NfObject 	 	object;
	
	/*< public >*/
	gint			init_done;
	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		gint			init_done_alarm;
	#endif
	
	GAsyncQueue		*queue;			
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		GAsyncQueue		*queue_net;		// to send network audio stream immediately
	#endif

	GThread			*thread;
	gint			thread_run;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		gint			thread_run_net;
	#endif
	gint			thread_status;
	
	GThread			*thread_put;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		GThread			*thread_put_net;
	#endif
	gint			thread_put_run;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		gint			thread_put_net_run;
	#endif
	gint			thread_put_status;

	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		GThread			*thread_alarm;
		gint			thread_run_alarm;
	#endif

	gint			audio_fd;
	gint			audio_fd_write;
	gint			stream_id[64];
	GTimeVal		timestamp[64];
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		GTimeVal		timestamp_imme[64];
		#if defined(ENABLE_DEBUG_IMMEDIATELY_AUDIO)
			GTimeVal		timestamp_imme_prev[64];
	
			gint			audio_imme_tc[NUM_AUDIO];
			guint64			audio_handoff_cnt[NUM_AUDIO];
			guint64			audio_handoff_cnt_total[NUM_AUDIO];
			guint			audio_start_handoff;
			guchar			audio_start_handoffl;
		#endif
	#endif
		
	NF_REC_AUDIO_PARAM	rec_param;
	
	NF_REC_AUDIO_HANDOFF_FUNC	handoff_func;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		NF_REC_AUDIO_HANDOFF_FUNC_IMMEDIATELY	handoff_func_immediately;
	#endif
	guint						handoff_ch_mask;
	#if defined(ENABLE_REC_LIVE_AUDIO)
		NF_REC_AUDIO_DATA			rec_aud_data;
		guint						covert_mask;
	#endif

	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		NF_REC_AUDIO_DATA_AI_ALARM	rec_aud_data_ai;
	#endif
};

struct _NfRecAudioClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

gboolean 
nf_rec_audio_init(int wait);

gboolean
nf_rec_audio_start(NF_REC_AUDIO_PARAM *param);

gboolean
nf_rec_audio_change(NF_REC_AUDIO_PARAM *param);

gboolean
nf_rec_audio_register_handoff(guint ch_mask, NF_REC_AUDIO_HANDOFF_FUNC handoff_func );

#if defined(ENABLE_REC_NET_IMMEDIATELY)
	gboolean
	nf_rec_audio_register_handoff_immediately(guint ch_mask, NF_REC_AUDIO_HANDOFF_FUNC_IMMEDIATELY handoff_func );
#endif

gboolean
nf_rec_audio_stop();

gboolean 
nf_rec_audio_put_frame(gint ch_num, gpointer frame );
#if defined(ENABLE_REC_LIVE_AUDIO)
	void nf_rec_set_live_audio_ch(guint ch);
#endif

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
	void nf_rec_audio_alarm_test(guint mask_alarm_ch, char *str);
	void nf_rec_aud_send_to_ipcam(gint ch, gchar *data, guint len);
#endif

#endif

