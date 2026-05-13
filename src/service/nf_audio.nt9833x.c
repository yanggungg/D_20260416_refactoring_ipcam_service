#define _GNU_SOURCE
#include <sched.h>
#include <sys/ioctl.h>

#include <gobj.h>
#include <gobjmedia.h>

#include "nf_common.h"

#include <libicmem.h>

#if defined(ENABLE_AI_ALARM_AUDIO)
#include "itx_ai_def.h"
#endif

#include <novatek/hd_debug.h>
#include <novatek/vendor/vendor_common.h>

#include "nf_codec_header.h"
#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"
#include "nf_audio_put_frame.h"
#include "nf_audio_put_sst.h"
#include "nf_audio_convert.h"
#include "nf_audio_pb_local.h"
#include "nf_audio_cntl_stream.h"

#include "nf_util_device.h"
#include "nf_api_ipcam.h"

#include "nf_hw.h"

/**
	Extern Function Definition!!
**/
extern gboolean nf_debug_category_add(const gchar *cate, const gchar *log_str, guint *log_arr, guint log_arr_max_idx);
extern int nf_hw_get_audio_nr(void);
extern void nf_ipcam_send_stream(gint ch, gchar *data, guint len);

/**
	Gloval Function
**/
static void nf_audio_class_init (NfAudioClass * klass);
static void nf_audio_instance_init (GTypeInstance* instance, gpointer g_class);
static void nf_audio_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void nf_audio_dispose (GObject * object);
static void nf_audio_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void nf_audio_finalize (GObject * object);

#if defined(CHIP_NVT_NA51055)		// Camera
#if defined(ENABLE_GST_VERSION_UP)
static GstBuffer *nf_audio_gst_buffer(gint size, gboolean verbose);
#else
static GObject *nf_audio_gst_buffer(gint size, gboolean verbose);
#endif
#endif
static void nf_audio_threadRec(NfAudio *self);
static void nf_audio_threadRd(NfAudio *self);
static void nf_audio_threadPb(NfAudio *self);
static void nf_audio_threadPb_out(NfAudio *self);
static void nf_audio_threadVolume(NfAudio *self);
#if defined(ENABLE_AUDIO_HANDOFF_VLC)
	static void nf_audio_threadNetVlc(NfAudio *self);
#endif
#if defined(ENABLE_AI_ALARM_AUDIO)
static void nf_audio_threadAiAlarm(NfAudio *self);
#endif
static void _nf_audio_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void nf_audio_dvr_status_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#if defined(ENABLE_AI_ALARM_AUDIO)
static void _nf_audio_ai_event_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif
static void nf_audio_set_input_table(gint *table);
static void nf_audio_volume_set(void);
static void nf_audio_dump_icodec_header( const char *str, ICODEC_HEADER *pheader);
static int nf_audio_pb_cb_web_mic_atype(char *stream_buf, int hi_aud_stm_cnt, int stream_size, gboolean is_buffer_clear);
static int nf_audio_pb_cb_web_mic_btype(char *stream_buf, int hi_aud_stm_cnt, int stream_size, gboolean is_buffer_clear);

/**
    Gloval Variable
 **/
static GObjectClass *parent_class = NULL;
static NfAudio *_nf_audio = NULL;

extern int qc_audio_manual_enable;
extern int flag_qc_end;
gint qcPacket[AUDIO_NR_16]={0,};

static const char *_DEBUG_NF_AUDIO_str[32] =
{
	"REC_AUDIO_IDX_START",
	"REC_AUDIO_IDX_THREAD_RD",
	"REC_AUDIO_IDX_THREAD_REC",
	"REC_AUDIO_IDX_THREAD_PB",
	"REC_AUDIO_IDX_THREAD_NET_VLC",
	"REC_AUDIO_IDX_HANDOFF",
	"REC_AUDIO_IDX_HANDOFF_VLC",
	"REC_AUDIO_IDX_SST",

	"REC_AUDIO_IDX_NR"
};

static gint _DEBUG_NF_AUDIO_log[32] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

static void cpu_core_set(int core_id)
{
       int s=0;
       cpu_set_t cpuset;
       pthread_t thread;

       thread = pthread_self();

       // http://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html
       CPU_ZERO(&cpuset);
       CPU_SET(core_id, &cpuset);

       s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
       if (s != 0)
            printf("pthread_setaffinity_np \n");
       /* Check the actual affinity mask assigned to the thread */

       s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
       if (s != 0)
            printf("pthread_getaffinity_np \n");	
}


/**
	 Extern Variable
 **/


/**
	 Extern Function
 **/
#if defined(ENABLE_GST_VERSION_UP)
extern GMutex *g_mutex_new(void);
#endif
extern gboolean nf_sysman_qcmode_is_enable(void);
extern gboolean nf_sysman_hotkey_is_nfs(void);
extern guint nf_live_get_audio_output_type(void);
extern gint nf_live_get_cnt_audio_input(void);
extern guint nf_network_get_webra_audio_status(void);
#if defined(ENABLE_AI_ALARM_AUDIO)
	extern gint nf_rec_aud_ai_send_frame(NF_AUD_DATA_AI_ALARM *ai_aud);
	extern void nf_rec_aud_ai_evt_check(NF_AUD_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt);
#endif

/**
	Static Function Definition
**/
static gboolean _nf_audio_live_put_frame(gint curr_ch , NF_AUDIO_RD *self, gpointer data);


GType
nf_audio_get_type (void)
{
	static GType nf_audio_type = 0;

	if (G_UNLIKELY (nf_audio_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfAudioClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_audio_class_init,
			NULL,
			NULL,
			sizeof (NfAudio),
			0,
			(GInstanceInitFunc) nf_audio_instance_init,
			NULL
		};

		nf_audio_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfAudio", &object_info, 0);
	}

	return nf_audio_type;
}

static void
nf_audio_class_init (NfAudioClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_audio_set_property;
	gobject_class->get_property = nf_audio_get_property;

	gobject_class->dispose = nf_audio_dispose;
	gobject_class->finalize = nf_audio_finalize;

}

static void
nf_audio_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfAudio *self = NF_AUDIO(instance);

	self->init_done = 0;
	#if defined(ENABLE_AI_ALARM_AUDIO)
		self->init_done_ai_alarm = 0;
	#endif

	// event context & loop
	self->context = g_main_context_new ();
	self->loop = g_main_loop_new (self->context, FALSE);

	// queue
	self->stAudRec.queue = g_async_queue_new();
	self->stAudRd.queue = self->stAudRec.queue;
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		self->stAudNetVlc.queue = g_async_queue_new();
		self->stAudRd.queue_net_vlc = self->stAudNetVlc.queue;
	#endif
	self->stAudPb.queue_pb = g_async_queue_new();

	#if defined(ENABLE_AI_ALARM_AUDIO)
		self->thread_run_ai_alarm=TRUE;
		self->thread_ai_alarm=g_thread_create((GThreadFunc)nf_audio_threadAiAlarm,
										self, FALSE, NULL);
	#endif
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_audio_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_audio_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_audio_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_audio_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

gboolean nf_audio_init(void)
{
	NF_AUDIO_RD *pstAudRd = NULL;
	NF_AUDIO_REC *pstAudRec = NULL;
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		NF_AUDIO_NET_VLC *pstAudNetVlc = NULL;
	#endif
	gulong cb_handle=0;
	gint ch=0;
	gint num_audio=0;
	AUDIO_RECORD *info_rec;
	AUDIO_PLAYBACK *info_pb;

	g_return_val_if_fail (_nf_audio == NULL, FALSE);

	_nf_audio = g_object_new ( NF_TYPE_NF_AUDIO , NULL);

	nf_debug_category_add("nfaudio", (const char *)_DEBUG_NF_AUDIO_str, (guint *)_DEBUG_NF_AUDIO_log, DEBUG_NF_AUDIO_IDX_NR);

	pstAudRd = nf_audio_getRd();
	pstAudRec = nf_audio_getRec();
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		pstAudNetVlc = nf_audio_getNetVlc();
	#endif

	g_assert ( NULL != pstAudRd );
	g_assert ( NULL != pstAudRec );
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		g_assert ( NULL != pstAudNetVlc );
	#endif

	_nf_audio->audio_nr=nf_hw_get_audio_nr();
	pstAudRd->stInfo=(NF_AUDIO_INFO *)g_malloc0(sizeof(NF_AUDIO_INFO) * _nf_audio->audio_nr);
	memset(pstAudRd->stInfo, 0x00, (sizeof(NF_AUDIO_INFO) * _nf_audio->audio_nr));

	info_rec=&_nf_audio->info_rec;
	info_pb=&_nf_audio->info_pb;
	
	info_rec->audiocap_path=(HD_PATH_ID *)g_malloc0(sizeof(HD_PATH_ID) * _nf_audio->audio_nr);
	memset(info_rec->audiocap_path, 0x00, (sizeof(HD_PATH_ID) * _nf_audio->audio_nr));
	info_rec->audioenc_path=(HD_PATH_ID *)g_malloc0(sizeof(HD_PATH_ID) * _nf_audio->audio_nr);
	memset(info_rec->audioenc_path, 0x00, (sizeof(HD_PATH_ID) * _nf_audio->audio_nr));

	nf_audio_set_input_table(_nf_audio->table_input);

	_nf_audio->live_ch=NF_AUDIO_DAC_PLAYBACK;	// Init!!

	if(nf_sysman_qcmode_is_enable())
		_nf_audio->aud_output_type = AUD_OUTPUT_RCA;	//qcmode init mode RCA
	else
		_nf_audio->aud_output_type = (gint)nf_live_get_audio_output_type();
	g_message( "[%s] Audio type%d init #Start#", __FUNCTION__, _nf_audio->aud_output_type);

	nf_audio_init_lib(info_rec, info_pb, HD_AUDIO_CODEC_PCM, HD_AUDIO_CODEC_PCM, 
						(guint)_nf_audio->aud_output_type, _nf_audio->audio_nr);

	if(_nf_audio->aud_output_type == AUD_OUTPUT_HDMI) {
		nf_audio_set_pb_status(AUD_PB_OUT_MODE_DVR, FALSE);
		nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, TRUE);
	}
	else {
		nf_audio_set_pb_status(AUD_PB_OUT_MODE_DVR, TRUE);
		nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, FALSE);
	}

	/* Init mutex lock. */
	pstAudRec->object.lock = g_mutex_new();
	g_assert( NULL != pstAudRec->object.lock );
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		pstAudNetVlc->object.lock = g_mutex_new();
		g_assert( NULL != pstAudNetVlc->object.lock );
	#endif

	/* Set CH & stream id */
	for ( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
	{
		pstAudRec->stParam[ch].u8Chn = 0xFF;
		pstAudRec->stParam[ch].s32StmId = -1;
	}
	pstAudRec->handoffFunc = NULL;

	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		/* Set CH & stream id */
		for ( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
		{
			pstAudNetVlc->stParam[ch].u8Chn = 0xFF;
			pstAudNetVlc->stParam[ch].s32StmId = -1;
		}
		pstAudNetVlc->handoffFunc = NULL;
	#endif

	pstAudRd->isEnable = FALSE;
	pstAudRec->isEnable = FALSE;

	// Thread Enable
	_nf_audio->thread_run_rd = TRUE;
	_nf_audio->thread_run_rec = TRUE;
	_nf_audio->thread_run_pb = TRUE;

	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		_nf_audio->thread_run_net_vlc = TRUE;
	#endif
	_nf_audio->thread_run_net_pb = TRUE;
	_nf_audio->thread_run_pb_out = TRUE;
	// Not Use in DVR / NVR
	_nf_audio->thread_run_volume = FALSE;

	// For Debug
	_nf_audio->dbg_pb_out = FALSE;
	_nf_audio->dbg_pb = FALSE;

	// Thread Create
	_nf_audio->thread_rec = g_thread_create((GThreadFunc)nf_audio_threadRec,
										_nf_audio, FALSE, NULL);
	_nf_audio->thread_rd = g_thread_create((GThreadFunc)nf_audio_threadRd,
										_nf_audio, FALSE, NULL);
	_nf_audio->thread_pb = g_thread_create((GThreadFunc)nf_audio_threadPb,
										_nf_audio, FALSE, NULL);
	_nf_audio->thread_pb = g_thread_create((GThreadFunc)nf_audio_threadPb_out,
										_nf_audio, FALSE, NULL);
	_nf_audio->thread_volume = g_thread_create((GThreadFunc)nf_audio_threadVolume,
										_nf_audio, FALSE, NULL);
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		_nf_audio->thread_net_vlc = g_thread_create((GThreadFunc)nf_audio_threadNetVlc,
											_nf_audio, FALSE, NULL);
	#endif

	// Set Volume
	nf_audio_volume_set();

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _nf_audio_sysdb_reload_cb_func, NULL);
	g_message("%s _nf_audio_sysdb_reload_cb_func connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle = nf_notify_connect_cb("dvr_status", nf_audio_dvr_status_cb_func, (gpointer)_nf_audio);
	g_message("%s nf_audio_dvr_status_cb_func connect_cb[%ld]",__FUNCTION__, cb_handle);
	g_assert(cb_handle >0);

	#if defined(ENABLE_AI_ALARM_AUDIO)
		cb_handle= nf_notify_connect_cb( "ai_event", _nf_audio_ai_event_notify_cb_func , (gpointer)NULL);
		g_message("%s _nf_audio_ai_event connect_cb[%ld]",__FUNCTION__, cb_handle);
		g_assert(cb_handle >0);
	#endif

	_nf_audio->stAudRd.send_type=NF_AUDIO_SEND_IPCAM;
	_nf_audio->live_ch=_nf_audio->table_input[0];

	#if 0
		// For HW Test
		nf_audio_set_pb_status(AUD_PB_OUT_MODE_LIVE, TRUE);
		_nf_audio->live_ch=0;
	#endif

	g_message("[%s] Audio init #END#", __FUNCTION__);

	return TRUE;
}

gboolean nf_audio_start( NF_REC_AUDIO_PARAM *pstParam )
{
	NF_AUDIO_QDATA *qdata = NULL;
	NF_AUDIO_PARAM *param = NULL;
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		NF_AUDIO_QDATA *qdata_net = NULL;
		NF_AUDIO_PARAM *param_net = NULL;
	#endif
	gint i;

	g_return_val_if_fail ( NULL != pstParam, FALSE );

	//g_message("[%s] START(From record manager)", __FUNCTION__);

	/* Create audio info */
	qdata = nf_audio_CreateQdata( sizeof(NF_AUDIO_PARAM) * NUM_ACTIVE_CH );
	g_assert( NULL != qdata );

	param = (NF_AUDIO_PARAM *) ( qdata->pData );
	g_assert( NULL != param );

	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		qdata_net = nf_audio_CreateQdata( sizeof(NF_AUDIO_PARAM) * NUM_ACTIVE_CH );
		g_assert( NULL != qdata_net );

		param_net = (NF_AUDIO_PARAM *) ( qdata_net->pData );
		g_assert( NULL != param_net );
	#endif

	/* Check audio param */
	for ( i = 0; i < NUM_ACTIVE_CH; i++)
	{
		#if 0
			** Message: [nf_audio_start][398] CH[255] Reason[0] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[255] Reason[0] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[255] Reason[0] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[00] Reason[1] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[01] Reason[1] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[02] Reason[1] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[03] Reason[1] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[255] Reason[0] Pre_Time[0] Pre_Close[0] 
			** Message: [nf_audio_start][398] CH[255] Reason[0] Pre_Time[0] Pre_Close[0] 
		#endif
		NF_AUD_DBG(DBG_MSG_AUD_CFG, "[%s][%d] CH[%d -> %02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", __FUNCTION__, __LINE__, i,
												pstParam->ch_arr[i],
												pstParam->rec_reason[i],
												pstParam->pre_rec_time[i],
												pstParam->pre_rec_close[i] );

		if (pstParam->send_type == NF_AUDIO_SEND_IPCAM) {
			if ( ( pstParam->ch_arr[i] >= NUM_ACTIVE_CH) &&
				 ( pstParam->ch_arr[i] != 0xFF ) )
			{
				g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
				return FALSE;
			}
			else
			{
				param[i].u8Chn = pstParam->ch_arr[i];
				#if defined(ENABLE_AUDIO_HANDOFF_VLC)
					param_net[i].u8Chn = pstParam->ch_arr[i];
				#endif
			}
		} else {
			if ( ( pstParam->ch_arr[i] >= _nf_audio->audio_nr) &&
				( pstParam->ch_arr[i] != 0xFF ) )
			{
				g_warning("[%s][%d] CH[%d] -> Audio mapping[%d] Invaild parameter.", 
								__FUNCTION__, __LINE__, i, pstParam->ch_arr[i]);
				return FALSE;
			}
			else
			{
				param[i].u8Chn = pstParam->ch_arr[i];
				#if defined(ENABLE_AUDIO_HANDOFF_VLC)
					param_net[i].u8Chn = pstParam->ch_arr[i];
				#endif
			}			
		}

		if ( pstParam->pre_rec_time[i] > MAX_PRE_REC_TIME )
		{
			g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
			return FALSE;
		}
		else
		{
			param[i].u8PreRecTime = pstParam->pre_rec_time[i];
			#if defined(ENABLE_AUDIO_HANDOFF_VLC)
				param_net[i].u8PreRecTime = pstParam->pre_rec_time[i];
			#endif
		}

		if ( pstParam->rec_reason[i] > NF_RECORD_REASON_PRE )
		{
			g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
			return FALSE;
		}
		else
		{
			param[i].u8Reason = pstParam->rec_reason[i];
			#if defined(ENABLE_AUDIO_HANDOFF_VLC)
				param_net[i].u8Reason = pstParam->rec_reason[i];
			#endif
		}

		param[i].u8PreRecClose = pstParam->pre_rec_close[i];
		#if defined(ENABLE_AUDIO_HANDOFF_VLC)
			param_net[i].u8PreRecClose = pstParam->pre_rec_close[i];
		#endif

		#if defined(ENABLE_AUDIO_HANDOFF_VLC)
			if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_START] & i) {
				g_message("[%s][Param_VLC] CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] "
							"CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", 
							_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_START],
							param[i].u8Chn, param[i].u8Reason, param[i].u8PreRecTime, param[i].u8PreRecClose,
							param_net[i].u8Chn, param_net[i].u8Reason, param_net[i].u8PreRecTime, param_net[i].u8PreRecClose);
			}
		#else
			if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_START] & i) {
				g_message("[%s][Param]     CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", 
							_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_START],
							param[i].u8Chn, param[i].u8Reason, param[i].u8PreRecTime, param[i].u8PreRecClose );
			}
		#endif
	}

	/* Audio enable */
	nf_audio_setRdEnable( TRUE );

	qdata->s32Cmd = AUD_CMD_CFG;
	/* Send config info to audio queue */
	if ( nf_audio_sendQdata( qdata, nf_audio_getRecQueue() ) == FALSE )
	{
		g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
		return FALSE;
	}

	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		qdata_net->s32Cmd = AUD_CMD_NET_VLC_CFG;

		/* Send config info to audio queue */
		if ( nf_audio_sendQdata( qdata_net, nf_audio_getNetVlcQueue() ) == FALSE )
		{
			g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
			return FALSE;
		}
	#endif

	return TRUE;
}

gboolean nf_audio_ctrNetVlc( NF_AUDIO_PARAM *pstOld,  NF_AUDIO_PARAM *pstNew )
{
	gint i, ret;

	g_return_val_if_fail (pstOld != NULL, FALSE);
	g_return_val_if_fail (pstNew != NULL, FALSE);

	for( i = 0; i < NUM_ACTIVE_CH; i++ )
	{
		#if 0
			g_message("[%s] Old CH[%02d] Reason[%d] PreTime[%d] PreClose[%d] STM[%d]", __FUNCTION__,
													pstOld[i].u8Chn,
													pstOld[i].u8Reason,
													pstOld[i].u8PreRecTime,
													pstOld[i].u8PreRecClose,
													pstOld[i].s32StmId );

			g_message("[%s] New CH[%02d] Reason[%d] PreTime[%d] PreClose[%d]", __FUNCTION__,
													pstOld[i].u8Chn,
													pstOld[i].u8Reason,
													pstOld[i].u8PreRecTime,
													pstOld[i].u8PreRecClose );
		#endif

		pstOld[i].u8Chn         = pstNew[i].u8Chn;
		pstOld[i].u8Reason      = pstNew[i].u8Reason;
		pstOld[i].u8PreRecTime  = pstNew[i].u8PreRecTime;
		pstOld[i].u8PreRecClose = pstNew[i].u8PreRecClose;

	}

	return TRUE;
}

static GStaticMutex _mutex_cb_sysdb = G_STATIC_MUTEX_INIT;
static void
_nf_audio_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint type=0;
	guint is_hdmi=0;

	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_AUDIO) {
		is_hdmi=nf_live_get_audio_output_type();

		#if 0
			g_message("%s line%d type[%d] is_hdmi[%d]", __FUNCTION__, __LINE__,
						_nf_audio->aud_output_type, is_hdmi);
		#endif

		if(_nf_audio->aud_output_type != (gint)is_hdmi) {
			if(is_hdmi == AUD_OUTPUT_RCA) {
				g_static_mutex_lock(&_mutex_cb_sysdb);

				g_message("%s Audio Output HDMI -> RCA", __FUNCTION__);

				nf_audio_nvt_set_pb_out_mode(&_nf_audio->info_pb, FALSE);

				_nf_audio->aud_output_type=AUD_OUTPUT_RCA;

				g_static_mutex_unlock (&_mutex_cb_sysdb);
			}
			else if(is_hdmi == AUD_OUTPUT_HDMI) {
				g_static_mutex_lock(&_mutex_cb_sysdb);

				g_message("%s Audio Output RCA -> HDMI", __FUNCTION__);

				nf_audio_nvt_set_pb_out_mode(&_nf_audio->info_pb, TRUE);

				_nf_audio->aud_output_type=AUD_OUTPUT_HDMI;

				g_static_mutex_unlock (&_mutex_cb_sysdb);
			}
			else {
				g_warning("%s Unknown Audio Output Type!!", __FUNCTION__);
				return ;
			}
		}
	}
}

static void nf_audio_dvr_status_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NfAudio *self=NULL;
	
	self=(NfAudio *)data;

	g_return_if_fail(pinfo != NULL);

	#if 0
		g_message("%s line%d %d %d %d %d", __FUNCTION__, __LINE__, 
					pinfo->d.params[0], pinfo->d.params[1], pinfo->d.params[2], pinfo->d.params[3]);
	#endif

	self->dvr_status=pinfo->d.params[0];
}

#if defined(ENABLE_AI_ALARM_AUDIO)
static void _nf_audio_ai_event_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int *p=NULL;
	ai_rule_event_t *pevt;

	g_return_if_fail(pinfo != NULL);

	NF_OBJECT_LOCK(_nf_audio);

	p = pinfo->p.ptr;
	pevt=(p + 2);

	extern int get_ai_enable(int chan);     // nf_action.c
	if(!get_ai_enable(p[0])) {
		NF_OBJECT_UNLOCK(_nf_audio);
		return;
	}

	#define VCA_MAX_ELEMS MAX(IVCA_MAX_ZONES, IVCA_MAX_CNTRS)
	g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

	_nf_audio->aud_data_ai.ai_evt_cnt=p[1];
	nf_rec_aud_ai_evt_check(&_nf_audio->aud_data_ai, pevt);

	// g_message("%s line%d ch%d [0x%08x]", __FUNCTION__, __LINE__, pevt->ch,
	// 			_nf_audio->aud_data_ai.mask_ai_evt[pevt->ch]);

	NF_OBJECT_UNLOCK(_nf_audio);
}
#endif

gboolean nf_audio_set_live_audio_ch(int ch)
{
	g_return_val_if_fail(_nf_audio != NULL, 0);
	g_return_val_if_fail((ch < (guint)_nf_audio->audio_nr) || (ch == 0xFF), 0);

	if(ch == NF_AUDIO_DAC_PLAYBACK) {
		#if defined(USE_DEV_DECODER)
			nf_dev_audio_set_dac(NF_AUDIO_DAC_PLAYBACK);
		#endif
	}
	else {
		if(_nf_audio->aud_output_type == AUD_OUTPUT_HDMI) {
			#if defined(USE_DEV_DECODER)
				nf_dev_audio_set_dac(NF_AUDIO_DAC_PLAYBACK);
			#endif
		}
		else {
			#if defined(USE_DEV_DECODER)
				nf_dev_audio_set_dac(ch);
			#endif
		}
	}

	if(!_nf_audio->aud_status_pb.running_live) {
		_nf_audio->live_ch=ch;
	}

	return TRUE;
}

gboolean nf_audio_isRdEnable(void)
{
	return _nf_audio->stAudRd.isEnable;
}

void nf_audio_setRdEnable(gboolean bValue)
{
	_nf_audio->stAudRd.isEnable = bValue;
}

NF_AUDIO_RD *nf_audio_getRd(void)
{
    return &_nf_audio->stAudRd;
}

NF_AUDIO_REC *nf_audio_getRec(void)
{
	return &_nf_audio->stAudRec;
}

GAsyncQueue *nf_audio_getRecQueue(void)
{
	return _nf_audio->stAudRec.queue;
}

NF_AUDIO_PARAM *nf_audio_getRecParam(void)
{
	return _nf_audio->stAudRec.stParam;
}

void nf_audio_setRecHandoff(guint u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc)
{
	g_assert ( NULL != handoffFunc );

	if(_nf_audio->stAudRec.handoffFunc != handoffFunc)
		_nf_audio->stAudRec.handoffFunc = handoffFunc;

	_nf_audio->stAudRec.u32HandoffChnMask = u32ChnMask;
}

#if defined(ENABLE_AUDIO_HANDOFF_VLC)
NF_AUDIO_NET_VLC *nf_audio_getNetVlc(void)
{
	return &_nf_audio->stAudNetVlc;
}

GAsyncQueue *nf_audio_getNetVlcQueue(void)
{
	return _nf_audio->stAudNetVlc.queue;
}

NF_AUDIO_PARAM *nf_audio_getNetVlcParam(void)
{
	return _nf_audio->stAudNetVlc.stParam;
}

void nf_audio_setNetVlcHandoff(guint u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc)
{
	g_assert(NULL != handoffFunc);

	_nf_audio->stAudNetVlc.u32HandoffChnMask = u32ChnMask;
	_nf_audio->stAudNetVlc.handoffFunc = handoffFunc;
}
#endif

NF_AUDIO_QDATA *nf_audio_CreateQdata( guint s32Len )
{
	NF_AUDIO_QDATA *qdata = NULL;

	qdata = (NF_AUDIO_QDATA *) g_malloc0( sizeof(NF_AUDIO_QDATA) );

	g_assert( NULL != qdata );

	qdata->pData = (void *) g_malloc( s32Len );

	g_assert( NULL != qdata->pData );

	return qdata;
}

NF_AUDIO_QDATA *nf_audio_CreateQdata_gst_buffer(GObject *buffer)
{
	NF_AUDIO_QDATA *qdata = NULL;

	g_return_val_if_fail( buffer != NULL, NULL);

	qdata = g_malloc0( sizeof(NF_AUDIO_QDATA) );
	g_return_val_if_fail( qdata != NULL, NULL);

	void *tmp_gst_ret = NULL;
	tmp_gst_ret = g_object_ref( buffer );
	if(tmp_gst_ret == NULL)
		fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);

	qdata->s32Cmd = AUD_CMD_REC_GST_BUFFER;
	qdata->pData = (void *)buffer;
	qdata->s32Len  = 0;

	return qdata;
}

static gboolean
_nf_audio_freeQdata_gst_buffer(NF_AUDIO_QDATA *pstQdata)
{
	g_assert ( NULL != pstQdata );

	g_object_unref( pstQdata->pData );

	g_free( pstQdata );

	return TRUE;
}

gboolean nf_audio_freeQdata( NF_AUDIO_QDATA *pstQdata )
{
	g_assert ( NULL != pstQdata );

	g_free( pstQdata->pData );
	g_free( pstQdata );

	return TRUE;
}

gboolean nf_audio_sendQdata( NF_AUDIO_QDATA *pstQdata, GAsyncQueue *pQue)
{
	g_assert ( NULL != pstQdata );
	g_assert ( NULL != pQue );

	g_async_queue_push( pQue, pstQdata );

	return TRUE;
}

#if defined(ENABLE_GST_VERSION_UP)
#if defined(CHIP_NVT_NA51055)		// Camera
static GstBuffer *nf_audio_gst_buffer(gint size, gboolean verbose)
#elif defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
GObject *nf_audio_gst_buffer(gint size, gboolean verbose)
#endif
{
	GstBuffer *p=NULL;
	GstMemory *mem=NULL;
	GstMapInfo info;
	ICODEC_HEADER *h=NULL;

	g_return_val_if_fail(size < ( 1 << 20 ), NULL);

	/* make empty buffer */
	p = gst_buffer_new ();
	g_assert(p != NULL);

	/* make memory holding size bytes */
	mem = gst_allocator_alloc(NULL, size, NULL);

	/* add the buffer */
	gst_buffer_append_memory(p, mem);

	gst_buffer_map (p, &info, GST_MAP_WRITE);

	h=(ICODEC_HEADER *)info.data;
	h->gst_buffer=p;
	h->reserved=0;

	gst_buffer_unmap (p, &info);

	return p;
}
#else
#if defined(CHIP_NVT_NA51055)		// Camera
static GobjBuddyBuffer *nf_audio_gst_buffer(gint size, gboolean verbose)
#elif defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
GObject *nf_audio_gst_buffer(gint size, gboolean verbose)
#endif
{
	GobjBuddyBuffer *p = NULL;
	ICODEC_HEADER *h = NULL;
	
	g_return_val_if_fail( size < ( 1 << 20 ), NULL );

	#if 0       // For Gst Null Frame Test!!
		if(_is_null)
		{
			g_message("%s line%d Force NULL!!!", __FUNCTION__, __LINE__);
			return NULL;
		}
	#endif

	p = (GobjBuddyBuffer *) gobj_buddy_buffer_new_malloc( size );

	#if 0
		if( p == NULL ) return p;
	#else
		g_assert( p != NULL );
	#endif

	h = (ICODEC_HEADER *)( gobj_buddy_buffer_buf_get_addr( G_OBJECT(p) ) );

	#if 0
		if( h == NULL ) return h;
	#else
		g_assert( h != NULL );
	#endif

	h->gst_buffer   = p;
	h->reserved     = 0;

	if ( verbose == TRUE )
	{
		guint addr;
		addr = ICMEM_getBufferPhysicalAddress( h );
	}

	#if defined(CHIP_NVT_NA51055)		// Camera
		return p;
	#elif defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
		return (GObject *)p;
	#endif
}
#endif

static gboolean
_nf_audio_live_put_frame(gint curr_ch , NF_AUDIO_RD *self, gpointer data)
{
	return TRUE;
}

gboolean nf_audio_registerHandoff(guint u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc)
{
	if (nf_sysman_qcmode_is_enable())
		return TRUE;

	g_return_val_if_fail(_nf_audio != NULL, 0);
	g_return_val_if_fail((_nf_audio->stAudRec.init_done == TRUE), FALSE);

	g_assert(NULL != handoffFunc);

	g_message("[%s] chnMask[0x%x] handoffFx[%p]", __FUNCTION__, u32ChnMask, handoffFunc);

	#if 0       // For Dealock!!
		NF_OBJECT_LOCK( nf_audio_getRec() );
	#endif

	nf_audio_setRecHandoff(u32ChnMask, handoffFunc);

	#if 0       // For Dealock!!
		NF_OBJECT_UNLOCK( nf_audio_getRec() );
	#endif

	return TRUE;
}

#if defined(ENABLE_AUDIO_HANDOFF_VLC)
gboolean nf_audio_registerHandoff_vlc(guint u32ChnMask, NF_AUDIO_HANDOFF_FUNC handoffFunc)
{
	if (nf_sysman_qcmode_is_enable())
		return TRUE;

	g_return_val_if_fail(_nf_audio != NULL, 0);
	g_return_val_if_fail((_nf_audio->stAudNetVlc.init_done == TRUE), FALSE);

	g_assert ( NULL != handoffFunc );

	g_message("[%s] chnMask[0x%x] handoffFx[%p]", __FUNCTION__, u32ChnMask, handoffFunc);

	NF_OBJECT_LOCK(nf_audio_getNetVlc());

	nf_audio_setNetVlcHandoff(u32ChnMask, handoffFunc);

	NF_OBJECT_UNLOCK(nf_audio_getNetVlc());

	return TRUE;
}
#endif

static gboolean
_nf_rec_audio_handoff_gst_buffer( NF_AUDIO_REC *stAudRec, guint ch, gpointer frame )
{
	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	gint				clen, ret, stream_id;		
	GobjBuddyBuffer 	*gst_buf = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr( gst_buf );
	
	//if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_HANDOFF] & 1<<ch)
		//dump_icodec_header("handoff", pheader);
		
	NF_OBJECT_LOCK(_nf_audio);
	if(stAudRec->handoffFunc)
		stAudRec->handoffFunc(gst_buf);
	NF_OBJECT_UNLOCK(_nf_audio);

	return 1;
}

gboolean nf_audio_handoff(NF_AUDIO_REC *stAudRec, NF_AUDIO_QDATA *pstQdata)
{
	#if defined(ENABLE_GST_VERSION_UP)
		GstBuffer *gstBuf=NULL;
		GstMapInfo info;
	#else
		GobjBuddyBuffer *gstBuf=NULL;
	#endif
	ICODEC_HEADER *ih=NULL;
	gint gstSize=0;

	g_assert(NULL != stAudRec);
	g_assert(NULL != pstQdata);

	g_assert(NULL != pstQdata->pData);
	g_assert(_nf_audio->audio_nr > pstQdata->s32Chn);
	g_assert(NF_AUD_SIZE_SEND_DATA_TOTAL == pstQdata->s32Len);
	/* Align address of buffer */
	gstSize = (gint)ALIGN(gint, (sizeof(ICODEC_HEADER) + pstQdata->s32Len) , 32);

	/* Alloc CMEM */
	#if defined(ENABLE_GST_VERSION_UP)
		gstBuf = nf_audio_gst_buffer(gstSize, FALSE);
	#else
		gstBuf = (GobjBuddyBuffer *)nf_audio_gst_buffer(gstSize, FALSE);
	#endif
	if(gstBuf == NULL) {
		printf("%s alloc fail!!\n", __FUNCTION__);
		return FALSE;
	}

	if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_HANDOFF] & (1 << pstQdata->s32Chn)) {
		g_message("[%s] Create GST[%p]", __FUNCTION__, gstBuf);
	}

	//error: lvalue required as left operand of assignment

	#if defined(ENABLE_GST_VERSION_UP)
		gst_buffer_map(gstBuf, &info, GST_MAP_WRITE);
		memcpy(info.data + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len);
		gst_buffer_unmap(gstBuf, &info);
	#else
		memcpy((gobj_buddy_buffer_buf_get_addr(G_OBJECT(gstBuf)) + sizeof(ICODEC_HEADER)), pstQdata->pData, (size_t)pstQdata->s32Len);
	#endif

	/* Create ICODEC header */
	#if defined(ENABLE_GST_VERSION_UP)
		ih=(ICODEC_HEADER *)info.data;
	#else
		ih=(ICODEC_HEADER *) gobj_buddy_buffer_buf_get_addr(G_OBJECT(gstBuf));
		g_assert(NULL != ih);
	#endif

	ih->chan = (guchar)pstQdata->s32Chn;
	ih->codec = NF_CODEC_TYPE_URAW;
	ih->flags = 0;
	ih->version = NF_CODEC_VERSION_1;
	ih->frame_size = (guint)pstQdata->s32Len;
	ih->frame_type = NF_FRAME_TYPE_AUDIO;
	ih->resolution = 0;
	ih->frame_rate = NF_FPS_CR01;
	#if 0	// hisilicon
		ih->timestamp   = (guint)(pstQdata->u64Start/ 1000000);
		ih->timestampl  = (guchar)((pstQdata->u64Start % 1000000) / 5000);
	#else	// novatek
		ih->timestamp   = (guint)(pstQdata->u64Start);
		ih->timestampl  = (guchar)(pstQdata->u64Startl / 1000 / 5);
	#endif

	NF_OBJECT_LOCK(stAudRec);

	if(stAudRec->handoffFunc) {
		if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_HANDOFF] & (1 << pstQdata->s32Chn))
			nf_audio_dump_icodec_header(_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_HANDOFF], ih);

		stAudRec->handoffFunc(gstBuf);
	}

	NF_OBJECT_UNLOCK(stAudRec);

	g_object_unref(gstBuf);

	return TRUE;
}

#if defined(ENABLE_AUDIO_HANDOFF_VLC)
gboolean nf_audio_handoff_vlc(NF_AUDIO_NET_VLC *stAudNetVlc, NF_AUDIO_QDATA *pstQdata)
{
	#if defined(ENABLE_GST_VERSION_UP)
		GstBuffer *gstBuf = NULL;
		GstMapInfo info;
	#else
		GobjBuddyBuffer *gstBuf=NULL;
	#endif
	ICODEC_HEADER *ih = NULL;
	gint gstSize = 0;

	g_assert(NULL != stAudNetVlc);
	g_assert(NULL != pstQdata);

	g_assert(NULL != pstQdata->pData);
	g_assert(_nf_audio->audio_nr > pstQdata->s32Chn);
	g_assert(NF_AUD_SIZE_VLC == pstQdata->s32Len);

	/* Align address of buffer */
	gstSize=ALIGN(gint, (sizeof(ICODEC_HEADER) + pstQdata->s32Len) , 32);

	/* Alloc CMEM */
	gstBuf = (GobjBuddyBuffer *)nf_audio_gst_buffer(gstSize, FALSE);
	if(gstBuf == NULL)
		return FALSE;

	if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_HANDOFF_VLC] & (1 << pstQdata->s32Chn)) {
		g_message("[%s] Create GST[%p]", __FUNCTION__, gstBuf);
	}

	if(gstBuf == NULL) {
		printf("%s alloc fail!!\n", __FUNCTION__);
		return FALSE;
	}

	//error: lvalue required as left operand of assignment
	//GST_BUFFER_SIZE( gstBuf ) = (guint)gstSize;

//		memcpy( GST_BUFFER_DATA(gstBuf) + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len );
	#if defined(ENABLE_GST_VERSION_UP)
		gst_buffer_map (gstBuf, &info, GST_MAP_WRITE);
		memcpy( info.data + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len );
		gst_buffer_unmap (gstBuf, &info);
	#else
		memcpy((gobj_buddy_buffer_buf_get_addr(G_OBJECT(gstBuf)) + sizeof(ICODEC_HEADER)), pstQdata->pData, (size_t)pstQdata->s32Len);
	#endif

	/* Create ICODEC header */
	#if defined(ENABLE_GST_VERSION_UP)
		ih = (ICODEC_HEADER *)info.data;
	#else
		ih=(ICODEC_HEADER *) gobj_buddy_buffer_buf_get_addr(G_OBJECT(gstBuf));
		g_assert(NULL != ih);
	#endif
	#if 1
		ih->chan = (guchar)pstQdata->s32Chn;
	#else
		ih->chan = ch;
	#endif
	ih->codec = NF_CODEC_TYPE_URAW;
	ih->flags = 0;
	ih->version = NF_CODEC_VERSION_1;


	///////////////////////////////////////
	ih->frame_size = (guint)pstQdata->s32Len;
	ih->frame_type = NF_FRAME_TYPE_AUDIO;
	ih->resolution = 0;
	ih->frame_rate = NF_FPS_CR01;
	#if 0		// hisilicon
		ih->timestamp   = (guint)(pstQdata->u64Start/ 1000000);
		ih->timestampl  = (guchar)((pstQdata->u64Start % 1000000) / 5000);
	#else		// novatek
		ih->timestamp   = (guint)(pstQdata->u64Start);
		ih->timestampl  = (guchar)(pstQdata->u64Startl / 1000 / 5);
	#endif

	NF_OBJECT_LOCK(stAudNetVlc);

	if(stAudNetVlc->handoffFunc) {
		if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_HANDOFF_VLC] & (1 << pstQdata->s32Chn))
			nf_audio_dump_icodec_header(_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_HANDOFF_VLC], ih);

		stAudNetVlc->handoffFunc(gstBuf);
	}

	NF_OBJECT_UNLOCK(stAudNetVlc);

	g_object_unref(gstBuf);

	return TRUE;
}
#endif

#if 0
gboolean nf_audio_ctrlSST( NF_AUDIO_PARAM *pstOld,  NF_AUDIO_PARAM *pstNew )
{
	g_return_val_if_fail (pstOld != NULL, FALSE);
	g_return_val_if_fail (pstNew != NULL, FALSE);

	return TRUE;
}
#endif

//ksi_test
#ifndef NUM_AUDIO_MAX
#define NUM_AUDIO_MAX 32
#endif

//#define NF_AUDIO_STREAM_WRITE_TO_FILE
static void nf_audio_threadRd(NfAudio *self)
{
	gchar *p[NUM_AUDIO_MAX]={NULL, };
	#if defined(ENABLE_AUDIO_HANDOFF_VLC)
		gchar *p_net[NUM_AUDIO_MAX]={NULL, };
	#endif
	gint *size_remain=NULL;
	gint audio_nr=0, ch_aud=0, *table_input=NULL;
	gint ret;
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		gchar *stream=NULL;
	#endif
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	long long msec_curr=0, msec_fix=0, msec_diff=0;

	// For Live Test
	NF_AUDIO_QDATA_PB *qdata_pb=NULL;
	gint qlen=0;
	struct timeval tv;

	// For Auto QC Test
	char qc_file[64] = {0,};

	NF_AUDIO_RD *prd=&self->stAudRd;
	// Novatek
	CHAR **pcm_data;
	CHAR **bitstream_data;

	HD_AUDIOENC_POLL_LIST *poll_list;
	HD_AUDIOENC_RECV_LIST *recv_list;
	AUDIO_RECORD  *p_rec_info=(AUDIO_RECORD *)&self->info_rec;

	HD_COMMON_MEM_VB_BLK *pcm_blk;
	HD_COMMON_MEM_VB_BLK *bitstream_blk;

	UINTPTR *pcm_addr;
	UINTPTR *bitstream_addr;

	audio_nr=_nf_audio->audio_nr;

	pcm_data=(CHAR **)g_malloc0(sizeof(CHAR *) * audio_nr);
	bitstream_data=(CHAR **)g_malloc0(sizeof(CHAR *) * audio_nr);
	for(ch_aud=0; ch_aud<audio_nr; ch_aud++) {
		*(pcm_data + ch_aud)=(CHAR *)g_malloc0(sizeof(CHAR));
		*(bitstream_data + ch_aud)=(CHAR *)g_malloc0(sizeof(CHAR));
	}

	poll_list=(HD_AUDIOENC_POLL_LIST *)g_malloc0(sizeof(HD_AUDIOENC_POLL_LIST) * audio_nr);
	memset(poll_list, 0x00, (sizeof(HD_AUDIOENC_POLL_LIST) * audio_nr));
	recv_list=(HD_AUDIOENC_RECV_LIST *)g_malloc0(sizeof(HD_AUDIOENC_RECV_LIST) * audio_nr);
	memset(recv_list, 0x00, (sizeof(HD_AUDIOENC_RECV_LIST) * audio_nr));

	pcm_blk=(HD_COMMON_MEM_VB_BLK *)g_malloc0(sizeof(HD_COMMON_MEM_VB_BLK) * audio_nr);
	memset(pcm_blk, 0x00, (sizeof(HD_COMMON_MEM_VB_BLK) * audio_nr));
	bitstream_blk=(HD_COMMON_MEM_VB_BLK *)g_malloc0(sizeof(HD_COMMON_MEM_VB_BLK) * audio_nr);
	memset(bitstream_blk, 0x00, (sizeof(HD_COMMON_MEM_VB_BLK) * audio_nr));

	pcm_addr=(UINTPTR *)g_malloc0(sizeof(UINTPTR) * audio_nr);
	memset(pcm_addr, 0x00, (sizeof(UINTPTR) * audio_nr));
	bitstream_addr=(UINTPTR *)g_malloc0(sizeof(UINTPTR) * audio_nr);
	memset(bitstream_addr, 0x00, (sizeof(UINTPTR) * audio_nr));

	for (ch_aud=0; ch_aud<audio_nr; ch_aud++) {
		/* prepare buffer for receiving data */
		pcm_blk[ch_aud] = hd_common_mem_get_block(HD_COMMON_MEM_USER_BLK, BITSTREAM_LEN, 0);
		if (HD_COMMON_MEM_VB_INVALID_BLK == pcm_blk[ch_aud]) {
			printf("hd_common_mem_get_block fail\r\n");
			exit(1);
		}
		pcm_addr[ch_aud] = hd_common_mem_blk2pa(pcm_blk[ch_aud]);
		if (pcm_addr[ch_aud] == 0) {
			printf("hd_common_mem_blk2pa fail, pcm_blk[%d] = %#lx\r\n", ch_aud, pcm_blk[ch_aud]);
			hd_common_mem_release_block(pcm_blk[ch_aud]);
			exit(1);
		}
		pcm_data[ch_aud] = (char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pcm_addr[ch_aud], BITSTREAM_LEN);
		if (pcm_data[ch_aud] == 0) {
			g_assert(0);
		}
		memset(pcm_data[ch_aud], 0, BITSTREAM_LEN);

		bitstream_blk[ch_aud] = hd_common_mem_get_block(HD_COMMON_MEM_USER_BLK, BITSTREAM_LEN, 0);
		if (HD_COMMON_MEM_VB_INVALID_BLK == bitstream_blk[ch_aud]) {
			printf("hd_common_mem_get_block fail\r\n");
			g_assert(0);
		}
		bitstream_addr[ch_aud] = hd_common_mem_blk2pa(bitstream_blk[ch_aud]);
		if (bitstream_addr[ch_aud] == 0) {
			printf("hd_common_mem_blk2pa fail, bitstream_blk[%d] = %#lx\r\n", ch_aud, bitstream_blk[ch_aud]);
			hd_common_mem_release_block(bitstream_blk[ch_aud]);
			g_assert(0);
		}
		bitstream_data[ch_aud] = (char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, bitstream_addr[ch_aud], BITSTREAM_LEN);
		if (bitstream_data[ch_aud] == 0) {
			g_assert(0);
		}
		memset(bitstream_data[ch_aud], 0, BITSTREAM_LEN);
	}

	#if defined(NF_AUDIO_STREAM_WRITE_TO_FILE)
		FILE *fp_aud[NUM_AUDIO_MAX];
		char out_f[32]={0, };

		for(ch_aud=0; ch_aud<audio_nr; ch_aud++) {
			sprintf(out_f, "/ch%d_audio.raw", ch_aud);
			if((fp_aud[ch_aud] = fopen(out_f, "w")) == NULL) {
				g_warning("[AUDIO][RD][T] File Open Error.. %s", out_f);
				return ;
			}
			else {
				g_message("[AUDIO][RD][T] File Open.. %s", out_f);
			}
		}
	#endif

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	cpu_core_set(3);

	// for audio input table
	table_input=self->table_input;

	prd->init_done = 1;

	memset(poll_list, 0, sizeof(poll_list));
	for (ch_aud=0; ch_aud<audio_nr; ch_aud++) {
		poll_list[ch_aud].path_id = p_rec_info->audioenc_path[ch_aud];
	}

	#if 0
		printf("[AUDIO][T][RD] Configure Done..\n");
	#endif
	/* Start to get streams of each channel. */
	while ( self->thread_run_rd )
	{
		gint i=0;

		/* call poll to check the stream availability */
		ret = hd_audioenc_poll_list(poll_list, audio_nr, 1000);
		if (ret == HD_ERR_TIMEDOUT) {
			printf("Poll timeout!!\n");
			continue;
		}

		/* fill the structure and receive it from audioenc */
		memset(recv_list, 0, sizeof(recv_list));

		for(ch_aud=0; ch_aud<audio_nr; ch_aud++) {
			if (poll_list[ch_aud].revent.event != TRUE) {
				#if 0
					g_message("%s line%d ch_aud[%d]", __FUNCTION__, __LINE__, ch_aud);
				#endif
				continue;
			}
			if (poll_list[ch_aud].revent.bs_size > BITSTREAM_LEN) {
				printf("buffer size is not enough! %lu, %d\n",
					   poll_list[ch_aud].revent.bs_size, BITSTREAM_LEN);
				continue;
			}
			recv_list[ch_aud].path_id = p_rec_info->audioenc_path[ch_aud];
			recv_list[ch_aud].user_bs.p_user_buf = bitstream_data[ch_aud];
			recv_list[ch_aud].user_bs.user_buf_size = BITSTREAM_LEN;
		}

		if ((ret = hd_audioenc_recv_list(recv_list, audio_nr)) < 0) {
			printf("Error return value %d\n", ret);
		}
		else {
			guchar *ptr=NULL;
			guint size=0;
			UINT32 timestamp = hd_gettime_ms();
			gettimeofday(&tv, NULL);

			for(ch_aud=0; ch_aud<audio_nr; ch_aud++) {

				if (!recv_list[ch_aud].path_id) {
					#if 0
						g_message("%s line%d ch_aud[%d]", __FUNCTION__, __LINE__, ch_aud);
					#endif
					continue; 
				}
				if (recv_list[ch_aud].retval < 0) {
					printf("get bitstreame error! ret = %d ch[%d]\n", ret, ch_aud);
				} else if (recv_list[ch_aud].retval >= 0) {

					ptr=recv_list[ch_aud].user_bs.p_user_buf;
					size=recv_list[ch_aud].user_bs.size;		// size is 640Byte

					#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
						stream= (gchar *) g_malloc0(size);
						nf_audio_convert(ptr, stream, (size / sizeof(gshort)));
					#endif

					if(nf_sysman_qcmode_is_enable())
					{
						if(qc_audio_manual_enable)
						{
							nf_audio_set_pb_status(AUD_PB_OUT_MODE_LIVE, TRUE);
						}
						else
						{
							nf_audio_set_pb_status(AUD_PB_OUT_MODE_LIVE, FALSE);
						}
					}

					if(self->aud_status_pb.running_live) {
						nf_audio_set_pb_status(AUD_PB_OUT_MODE_LIVE, TRUE);

						if(table_input[ch_aud] == _nf_audio->live_ch) {
							qdata_pb=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));
						
							memcpy(qdata_pb->data, ptr, size);
							qdata_pb->cnt=size;
							qdata_pb->outmode=AUD_PB_OUT_MODE_LIVE;

							qlen=g_async_queue_length(self->stAudPb.queue_pb);
							if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
							{
								g_warning("%s queue full.. len[%d] live fail!!",__FUNCTION__, qlen);
								g_free(qdata_pb);
							}
							else {
								g_async_queue_push(self->stAudPb.queue_pb, qdata_pb);
							}
						}
					}
					else {

						if(self->aud_status_pb.running_live_hdmi) {

							if(table_input[ch_aud] == _nf_audio->live_ch) {

								qdata_pb=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));
								
								memcpy(qdata_pb->data, ptr, size);
								qdata_pb->cnt=size;
								qdata_pb->outmode=AUD_PB_OUT_MODE_LIVE_HDMI;

								qlen=g_async_queue_length(self->stAudPb.queue_pb);
								if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
								{
									g_warning("%s queue full.. len[%d] live fail!!",__FUNCTION__, qlen);
									g_free(qdata_pb);
								}
								else {
									g_async_queue_push(self->stAudPb.queue_pb, qdata_pb);
								}
							}
						}
					}

					#if 0	// 16bit pcm raw data write
						#if defined(NF_AUDIO_STREAM_WRITE_TO_FILE)
							if(nf_sysman_hotkey_is_nfs()) {
								if (fp_aud[ch_aud]) {
									fwrite(ptr, 1, size, fp_aud[ch_aud]); 
									fflush(fp_aud[ch_aud]);
								}
							}
						#endif
					#endif

					if(prd->stInfo[ch_aud].s32StmCnt == 0) {
						g_assert(NULL == prd->stInfo[ch_aud].pstQdata);
						prd->stInfo[ch_aud].pstQdata = nf_audio_CreateQdata(NF_AUD_SIZE_SEND_DATA_TOTAL);
						g_assert(NULL != prd->stInfo[ch_aud].pstQdata);

						p[ch_aud] = prd->stInfo[ch_aud].pstQdata->pData;
						prd->stInfo[ch_aud].pstQdata->s32Chn=table_input[ch_aud];
						prd->stInfo[ch_aud].pstQdata->u64Start = tv.tv_sec;
						prd->stInfo[ch_aud].pstQdata->u64Startl = tv.tv_usec;

						if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_THREAD_RD] & table_input[ch_aud]) {
							#if defined(ENABLE_ARCH_A64)
								g_message("[%s] Init CH[%d] QData[%p] PTS[%lu]",
											_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_RD],
											prd->stInfo[ch_aud].pstQdata->s32Chn, prd->stInfo[ch_aud].pstQdata,
											prd->stInfo[ch_aud].pstQdata->u64Start );
							#else
								g_message("[%s] Init CH[%d] QData[%p] PTS[%llu]",
											_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_RD],
											prd->stInfo[ch_aud].pstQdata->s32Chn, prd->stInfo[ch_aud].pstQdata,
											prd->stInfo[ch_aud].pstQdata->u64Start );
							#endif
						}
					}

					#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
						memcpy(p[ch_aud], stream, (size / sizeof(gshort)));

						#if defined(ENABLE_AUDIO_HANDOFF_VLC)
							gint cnt_vlc=0;

							for(cnt_vlc=0; cnt_vlc<NF_AUD_NET_VLC_DEVISION; cnt_vlc++) {

								g_assert (NULL == prd->stInfo[ch_aud].pstQdata_net_vlc);

								prd->stInfo[ch_aud].pstQdata_net_vlc=nf_audio_CreateQdata(NF_AUD_SIZE_VLC);

								g_assert(NULL != prd->stInfo[ch_aud].pstQdata_net_vlc);
								p_net[ch_aud]=(gchar *)prd->stInfo[ch_aud].pstQdata_net_vlc->pData;
								prd->stInfo[ch_aud].pstQdata_net_vlc->s32Chn=table_input[ch_aud];
								prd->stInfo[ch_aud].pstQdata_net_vlc->u64Start=tv.tv_sec;

								memcpy((gchar *)p_net[ch_aud], stream+(NF_AUD_SIZE_VLC * cnt_vlc), NF_AUD_SIZE_VLC);
								prd->stInfo[ch_aud].pstQdata_net_vlc->s32Len = NF_AUD_SIZE_VLC;
								prd->stInfo[ch_aud].pstQdata_net_vlc->u64Startl = tv.tv_usec;
								prd->stInfo[ch_aud].pstQdata_net_vlc->s32Cmd = AUD_CMD_NET_VLC_HANDOFF;

								#if 1
									if(nf_audio_sendQdata(prd->stInfo[ch_aud].pstQdata_net_vlc, prd->queue_net_vlc) == FALSE)
									{
										g_warning("[%s][%d] net send fail.", __FUNCTION__, __LINE__);
										if(nf_audio_freeQdata(prd->stInfo[ch_aud].pstQdata_net_vlc) == FALSE) {
											g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
										}
										goto exitRd;
									}
								#else
									nf_audio_freeQdata(prd->stInfo[ch_aud].pstQdata_net_vlc);
								#endif
								p_net[ch_aud] = NULL;
								prd->stInfo[ch_aud].pstQdata_net_vlc = NULL;
							}

							#if 0
							if (ch_aud == 0) {
								g_message("[AUDIO_RD][VLC] size[%d] vlc_cnt[%d] msec_curr[%lld] "
										"msec_fix[%lld] msec_diff[%lld] [%lld].[%lld]",  NF_AUD_SIZE_VLC, NF_AUD_NET_VLC_DEVISION, 
											msec_curr, msec_fix, msec_diff, (msec_fix / 1000),
											((msec_fix % 1000) / 5));
								}
							#endif
						#endif

						#if defined(NF_AUDIO_STREAM_WRITE_TO_FILE)
							// uraw data write
							if(nf_sysman_hotkey_is_nfs()) {
								int ch_tmp=table_input[ch_aud];

								if (fp_aud[ch_tmp]) {
									fwrite(p[ch_aud], 1, (size/sizeof(gshort)), fp_aud[ch_tmp]);
									fflush(fp_aud[ch_tmp]);
								}
							}
						#endif

						p[ch_aud] += (size / sizeof(gshort));
						prd->stInfo[ch_aud].pstQdata->s32Len += (gint)((guint)size / sizeof(gshort));

						#if 0
							g_message("NF_AUD_SIZE_SEND_DATA_TOTAL[%d] [%d] [%d]", NF_AUD_SIZE_SEND_DATA_TOTAL,
									prd->stInfo[ch_aud].pstQdata->s32Len, size/sizeof(gshort));
						#endif
					#else
						/*
						   To Do..
						*/
					#endif

					prd->stInfo[ch_aud].s32StmCnt++;

					if (NF_AUD_PINPERFRM_CNT <= prd->stInfo[ch_aud].s32StmCnt)
					{
						g_assert(NF_AUD_SIZE_SEND_DATA_TOTAL == prd->stInfo[ch_aud].pstQdata->s32Len);
						g_assert(table_input[ch_aud] == prd->stInfo[ch_aud].pstQdata->s32Chn);

						prd->stInfo[ch_aud].pstQdata->u64End = tv.tv_sec;
						prd->stInfo[ch_aud].pstQdata->u64Endl = tv.tv_usec;
						prd->stInfo[ch_aud].pstQdata->s32Cmd = AUD_CMD_REC;

						if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_THREAD_RD] & table_input[ch_aud]) {
							#if defined(ENABLE_ARCH_A64)
								g_message("[%s] Send CH[%d] QData[%p] CNT[%02d] PTS[%lu] LEN[%d]",
											_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_RD],
											prd->stInfo[ch_aud].pstQdata->s32Chn, prd->stInfo[ch_aud].pstQdata,
											prd->stInfo[ch_aud].s32StmCnt, prd->stInfo[ch_aud].pstQdata->u64End,
											prd->stInfo[ch_aud].pstQdata->s32Len );
							#else
								g_message("[%s] Send CH[%d] QData[%p] CNT[%02d] PTS[%llu] LEN[%d]",
											_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_RD],
											prd->stInfo[ch_aud].pstQdata->s32Chn, prd->stInfo[ch_aud].pstQdata,
											prd->stInfo[ch_aud].s32StmCnt, prd->stInfo[ch_aud].pstQdata->u64End,
											prd->stInfo[ch_aud].pstQdata->s32Len );
							#endif
						}
						if(prd->send_type == NF_AUDIO_SEND_DVR) {
							#if 1
								if(nf_audio_sendQdata(prd->stInfo[ch_aud].pstQdata, prd->queue) == FALSE) {
									g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
									goto exitRd;
								}
							#else
								nf_audio_freeQdata(prd->stInfo[ch_aud].pstQdata);
							#endif
						} else {
							if(nf_dev_mic_get_onoff()) {
								gint ch=0;
								guint aout_mask = nf_dev_mic_get_output_mask();
								self->vin_mask = ~(nf_notify_get_param0("vloss"));
								#if 0
									g_message("%s line%d aout_mask 0x%08x vin_mask 0x%08x", 
													__FUNCTION__, __LINE__, aout_mask, self->vin_mask);
								#endif
								for(ch=0; ch<NUM_ACTIVE_CH; ++ch)
								{
									#if defined(ENABLE_AI_ALARM_AUDIO)
										gboolean is_ai_aud_playing=self->aud_data_ai.is_playing[ch];

										if(is_ai_aud_playing) {
											#if 0
												g_message("%s line%d CH[%d] AI Event Audio Playing!! Mic On!!", 
															__FUNCTION__, __LINE__, ch);
											#endif
											aout_mask &= (guint)~(1 << ch);
										}
									#endif

									if(aout_mask & (1 << ch) && self->vin_mask & (1 << ch)) {
										nf_ipcam_send_stream(ch, prd->stInfo[ch_aud].pstQdata->pData, NF_AUD_SIZE_SEND_DATA_TOTAL);
									}
									else {
										#if defined(ENABLE_AI_ALARM_AUDIO)
											//g_message("%s line%d ch%d", __FUNCTION__, __LINE__, ch);
											if(!is_ai_aud_playing) {
												nf_ipcam_send_stream( ch, NULL, 0); // aout end 
											}
										#else
											nf_ipcam_send_stream( ch, NULL, 0); // aout end
										#endif
									}
								}
							}else{
								gint ch=0;

								for(ch=0; ch<NUM_ACTIVE_CH; ++ch) {
									#if defined(ENABLE_AI_ALARM_AUDIO)
										gboolean is_ai_aud_playing=self->aud_data_ai.is_playing[ch_aud];

										if(is_ai_aud_playing) {
											#if 0
												g_message("%s line%d CH[%d] AI Event Audio Playing!! Mic Off!!", 
															__FUNCTION__, __LINE__, ch);
											#else
												;
											#endif
										}
										else {
											nf_ipcam_send_stream( ch, NULL, 0);// aout end
										}
									#else
										nf_ipcam_send_stream( ch, NULL, 0);// aout end
									#endif
								}
							}
							nf_audio_freeQdata(prd->stInfo[ch_aud].pstQdata);
						}
						p[ch_aud] = NULL;
						prd->stInfo[ch_aud].pstQdata = NULL;
						prd->stInfo[ch_aud].s32StmCnt = 0;
					}

					#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
						g_free(stream);
					#endif

					Release_point:
					;
				}	// end if
			}	// end for
			
		}	// end if
	} /* while */

exitRd:
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		g_free( stream );
	#endif
	/* release buffer */
	for (ch_aud=0; ch_aud<audio_nr; ch_aud++) {
		ret = hd_common_mem_munmap(pcm_data[ch_aud], BITSTREAM_LEN);
		if (ret != HD_OK) {
			printf("hd_common_mem_munmap fail\n");
			goto exitRd_end;
		}
		ret = hd_common_mem_release_block(pcm_blk[ch_aud]);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail\n");
			goto exitRd_end;
		}
		ret = hd_common_mem_munmap(bitstream_data[ch_aud], BITSTREAM_LEN);
		if (ret != HD_OK) {
			printf("hd_common_mem_munmap fail\n");
			goto exitRd_end;
		}
		ret = hd_common_mem_release_block(bitstream_blk[ch_aud]);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail\n");
			goto exitRd_end;
		}
	}

exitRd_end:
	self->thread_run_rd=0;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

//#define NF_AUDIO_DUMP_PB_DATA
static void nf_audio_threadPb(NfAudio *self)
{
	NF_AUDIO_REC *ptr_rec=NULL;
	NF_AUDIO_PB *ptr_pb=NULL;
	gpointer pQueData=NULL;
	guint cnt=0;
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	#if defined(NF_AUDIO_DUMP_PB_DATA)
		static FILE *fp_pb=NULL;
		gboolean is_open_error=FALSE;
	#endif
	guint dvr_status=0;
	AUDIO_PLAYBACK *p_pb_info=(AUDIO_PLAYBACK *)&self->info_pb;

	INT ret, length, elapse_time, au_buf_time;
	CHAR *bitstream_data;
	HD_AUDIODEC_SEND_LIST send_list[1];
	UINT start_time, data_time;

	UINT64 pool = HD_COMMON_MEM_USER_BLK;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 addr;

	/* prepare buffer for sending data */
	blk = hd_common_mem_get_block(pool, BITSTREAM_LEN, 0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		exit(1);
	}
	addr = hd_common_mem_blk2pa(blk);
	if (addr == 0) {
		printf("hd_common_mem_blk2pa fail, blk = %#lx\r\n", blk);
		hd_common_mem_release_block(blk);
		exit(1);
	}
	bitstream_data = (char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, addr, BITSTREAM_LEN);
	if (bitstream_data == 0) {
		printf("hd_common_mem_mmap fail, blk = %#lx\r\n", blk);
		hd_common_mem_release_block(blk);
		exit(1);
	}

	start_time = nf_audio_get_current_time();
	data_time = 0;

	ptr_rec=&self->stAudRec;
	ptr_rec->init_done = 1;

	ptr_pb=&self->stAudPb;

	#if defined(NF_AUDIO_DUMP_PB_DATA)
		if(fp_pb == NULL) {
			if((fp_pb = fopen("/audio_pb.raw", "w")) == NULL) {
				g_warning("%s File Open Error!!", __FUNCTION__);
				is_open_error=TRUE;
			}
			else
				g_message("%s Line[%d] PB File Open Success!!", __FUNCTION__, __LINE__);
		}
	#endif

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

       cpu_core_set(3);

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while (1) {
		gboolean pb_is_dvr=0, pb_is_web_mic=0, pb_is_beep=0, pb_is_live=0, pb_is_live_hdmi=0;
		NF_AUDIO_QDATA_PB *qdata=(NF_AUDIO_QDATA_PB *)g_async_queue_pop(ptr_pb->queue_pb);
		gint outmode=0;

		dvr_status=self->dvr_status;

		#if 0
			if(qdata) {
				g_free(qdata);
			}
			continue;
		#endif

		outmode=qdata->outmode;
		pb_is_dvr=self->aud_status_pb.running_dvr;
		pb_is_web_mic=self->aud_status_pb.running_web_mic;
		pb_is_beep=self->aud_status_pb.running_beep;
		pb_is_live=self->aud_status_pb.running_live;
		pb_is_live_hdmi=self->aud_status_pb.running_live_hdmi;

		if(self->dbg_pb) {
			if((cnt % 50) == 0) {
				g_message("Playback Running pb_is_dvr[%d] pb_is_web_mic[%d] pb_is_beep[%d] pb_is_live[%d] pb_is_live_hdmi[%d], outmode[%d]", 
							pb_is_dvr, pb_is_web_mic, pb_is_beep, pb_is_live, pb_is_live_hdmi, outmode);
			}
		}
		
		if(pb_is_live) {

			if(self->dbg_pb) {
				if((cnt % 50) == 0) {
					g_message("Playback Playing Live");
				}
			}

			if((outmode == AUD_PB_OUT_MODE_DVR) || (outmode == AUD_PB_OUT_MODE_WEB) 
				|| (outmode == AUD_PB_OUT_MODE_BEEP) || (outmode == AUD_PB_OUT_MODE_LIVE_HDMI)) {

				if(self->dbg_pb) {
					if((cnt % 50) == 0) {
						g_message("Playback Playing Live -> Skip Web / Dvr / Beep / HDMI Live");
					}
				}

				if(qdata) {
					g_free(qdata);
				}
				continue;
			}
		}
		else if(pb_is_live_hdmi) {

			if(self->dbg_pb) {
				if((cnt % 50) == 0) {
					g_message("Playback Playing HDMI Live");
				}
			}

			if((outmode == AUD_PB_OUT_MODE_DVR) || (outmode == AUD_PB_OUT_MODE_WEB) 
				|| (outmode == AUD_PB_OUT_MODE_BEEP)) {
				if(self->dbg_pb) {
					if((cnt % 50) == 0) {
						g_message("Playback Playing HDMI Live -> Skip Web / Dvr / Beep");
					}
				}

				if(qdata) {
					g_free(qdata);
				}
				continue;
			}
		}
		else {
			if(pb_is_beep) {
				if(self->dbg_pb) {
					if((cnt % 50) == 0) {
						g_message("Playback Playing Beep");
					}
				}

				if((outmode == AUD_PB_OUT_MODE_DVR) || (outmode == AUD_PB_OUT_MODE_WEB)) {
					if(self->dbg_pb) {
						if((cnt % 50) == 0) {
							g_message("Playback Playing Live -> Skip Web / Dvr");
						}
					}

					if(qdata) {
						g_free(qdata);
					}
					continue;
				}
			}
			else {
				if(pb_is_web_mic) {
					if(self->dbg_pb) {
						if((cnt % 50) == 0) {
							g_message("Playback Playing Web Mic");
						}
					}

					if(outmode == AUD_PB_OUT_MODE_DVR) {
						if(self->dbg_pb) {
							if((cnt % 50) == 0) {
								g_message("Playback Playing Live -> Skip Dvr");
							}
						}

						if(qdata) {
							g_free(qdata);
						}
						continue;
					}
				}
				else {
					if(self->dbg_pb) {
						if((cnt % 50) == 0) {
							g_message("Playback Playing DVR or NVR");
						}
					}
				}
			}
		}

		length=qdata->cnt;
#if 0
retry:
		#define BUFFER_TIME_MS      500

		elapse_time = TIME_DIFF(nf_audio_get_current_time(), start_time);
		au_buf_time = data_time - elapse_time;
		if (au_buf_time > BUFFER_TIME_MS) {
			usleep(10000);
			g_message("%s line%d playback debug!! retry!!", __FUNCTION__, __LINE__);
			goto retry;
		}
#endif
		memcpy((char *)bitstream_data, qdata->data, length);

		#if defined(NF_AUDIO_DUMP_PB_DATA)
			if( nf_sysman_hotkey_is_nfs() )
			{
				if(!is_open_error) {
					fwrite(qdata->data, 1, length, fp_pb);
				}
			}
		#endif

		if(qdata) {
			g_free(qdata);
		}

		data_time += (length * 1000 / 8000 / 2); // for sample rate 8K, 16bits, mono

		/* fill the structure and send it to audiodec */
		memset(send_list, 0, sizeof(send_list));  /* clear all mutli bs */
		send_list[0].path_id = p_pb_info->audiodec_path;
		send_list[0].user_bs.p_user_buf = bitstream_data;
		send_list[0].user_bs.user_buf_size = length;
		if ((ret = hd_audiodec_send_list(send_list, 1, 500)) < 0) {
			printf("<send bitstream fail(%d)!>\n", ret);
		}
	}

	/* release buffer */
	ret = hd_common_mem_munmap(bitstream_data, BITSTREAM_LEN);
	if (ret != HD_OK) {
		printf("hd_common_mem_munmap fail\n");
		exit(1);
	}

rel_blk:
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		printf("hd_common_mem_release_block fail\n");
		exit(1);
	}

exitPlayback:
	self->thread_run_rec = 0;
    g_thread_exit(0);
    g_error("[%s] END", __FUNCTION__);

	return ;
}

//#define NF_AUDIO_REC_STREAM_WRITE_TO_FILE
#if defined(NF_AUDIO_REC_STREAM_WRITE_TO_FILE)
	#define FILENAME_AUDIO_REC_STREAM_CH0       "/audio_rec_ch0.raw"
#endif
static void nf_audio_threadRec(NfAudio *self)
{
	NF_AUDIO_REC *ptr_rec=NULL;
	gpointer pQueData = NULL;
	gint ch=0, ret=0, cnt_err=0;

	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	cpu_core_set(3);

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while(NULL == nf_audio_getRec())
		g_usleep(10*1000);

	ptr_rec=&self->stAudRec;
	#if defined(NF_AUDIO_REC_STREAM_WRITE_TO_FILE)
		FILE *fp_ch0=NULL;

		if((fp_ch0=fopen(FILENAME_AUDIO_REC_STREAM_CH0, "w")) == NULL) {
			g_warning("%s File Open Error!!", __FUNCTION__);
			return ;
		}
		else
			g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
	#endif

	ptr_rec->init_done=1;

	/* Start to get streams of each channel. */

	while(self->thread_run_rec)
	{
		NF_AUDIO_QDATA *pstQdata=NULL;

		pQueData = g_async_queue_pop(ptr_rec->queue);
		g_assert(NULL != pQueData);

		pstQdata=(NF_AUDIO_QDATA *)pQueData;
		g_assert(NULL != pstQdata);

		switch(pstQdata->s32Cmd)
		{
			case AUD_CMD_REC:
			{
				if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_THREAD_REC] & pstQdata->s32Len) {
					#if defined(ENABLE_ARCH_A64)
						g_message("[%s] CMD[REC] CH[%d] LEN[%d] S_PTS[%lu] E_PTS[%lu] Diff[%lu]",
									_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_REC],
									pstQdata->s32Chn, pstQdata->s32Len, pstQdata->u64Start,
									pstQdata->u64End, pstQdata->u64End - pstQdata->u64Start);
					#else
						g_message("[%s] CMD[REC] CH[%d] LEN[%d] S_PTS[%llu] E_PTS[%llu] Diff[%llu]",
									_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_REC],
									pstQdata->s32Chn, pstQdata->s32Len, pstQdata->u64Start,
									pstQdata->u64End, pstQdata->u64End - pstQdata->u64Start);
					#endif
				}
				#if defined(NF_AUDIO_REC_STREAM_WRITE_TO_FILE)
					if(pstQdata->s32Chn == 0) {
						if(nf_sysman_hotkey_is_nfs())
							fwrite(pstQdata->pData, 1, pstQdata->s32Len, fp_ch0);
					}
				#endif
				/* Handoff */
				if((ptr_rec->handoffFunc != NULL) &&
					(ptr_rec->u32HandoffChnMask & (guint)( 1 << pstQdata->s32Chn)))
				{
					if(nf_audio_handoff(ptr_rec, pstQdata) == FALSE) {
						#if 0
							g_warning("[%s][%d] hi_aud get gst buffer fail.", __FUNCTION__, __LINE__);
							goto exitRec;
						#else
							goto aud_rec_skip;
						#endif
					}
				}

				/* Put frame sst */
				for(ch=0; ch<NUM_ACTIVE_CH; ch++)
				{
					#if 0		// For Debug
						if(ch == 0) {
							printf("%s line%d ch%d  %d==%d / (%d > 0) / %d != %d\n", 
									__FUNCTION__, __LINE__, ch, ptr_rec->stParam[ch].u8Chn,
									pstQdata->s32Chn, ptr_rec->stParam[ch].u8PreRecTime,
									ptr_rec->stParam[ch].u8Reason, NF_RECORD_REASON_NOTHING);
						}
					#endif

					if((ptr_rec->stParam[ch].u8Chn == pstQdata->s32Chn) &&
						(ptr_rec->stParam[ch].u8PreRecTime > 0 ||
						ptr_rec->stParam[ch].u8Reason != NF_RECORD_REASON_NOTHING))
					{
						if(nf_audio_SSTput(ch, pstQdata, &(ptr_rec->stParam[ch])) == FALSE) {
							#if 0
								g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
								goto exitRec;
							#else
								goto aud_rec_skip;
							#endif
						}
					}
				}
				break;
				aud_rec_skip:
					if((cnt_err % 20) == 0)
						g_warning("[%s][%d] audio get gst buffer fail.", __FUNCTION__, __LINE__);
				break;
			}
			break;

			case AUD_CMD_REC_GST_BUFFER:
			{
				GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)pstQdata->pData;
				ICODEC_HEADER *pheader_single = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(buffer);
				guint chan = pheader_single->chan;

				#ifdef DEBUG_REC_AUDIO_LOG
					if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_HEX] )
						nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
				#endif

				if( ( ptr_rec->handoffFunc != NULL ) &&
					( ptr_rec->u32HandoffChnMask & (guint)( 1 << pstQdata->s32Chn ) ) )
					_nf_rec_audio_handoff_gst_buffer(ptr_rec, chan, buffer);

				if( ptr_rec->stParam[chan].u8Chn != 0xff
						&& ( ptr_rec->stParam[chan].u8PreRecTime >0
							|| ptr_rec->stParam[chan].u8Reason != NF_RECORD_REASON_NOTHING ) )
					nf_audio_put_frame_gst_buffer_sst( chan, buffer, &(ptr_rec->stParam[chan]));
			}
			break;

			case AUD_CMD_CFG:
			{
				NF_AUDIO_PARAM *paramNew=(NF_AUDIO_PARAM *)(pstQdata->pData);
				g_assert(NULL != paramNew);

				#if 0
					g_message("[%s] CMD[CFG]", __FUNCTION__);
				#endif

				if(nf_audio_SSTctrl(ptr_rec->stParam, paramNew) == FALSE) {
					g_warning("[%s][%d] Audio configuration failed.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
			break;

			default:
				g_warning("[%s][%d] Invaild cmd.", __FUNCTION__, __LINE__);
				goto exitRec;

		} /* switch */

		if (pstQdata->s32Cmd == AUD_CMD_REC_GST_BUFFER) {
			if (_nf_audio_freeQdata_gst_buffer( pstQdata ) == FALSE) {
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitRec;
			}
		} else {
			if(nf_audio_freeQdata(pstQdata) == FALSE) {
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitRec;
			}
		}

		cnt_err++;

	} /* while */

exitRec:
	self->thread_run_rec = 0;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

#if defined(ENABLE_AUDIO_HANDOFF_VLC)
//#define NF_AUDIO_DUMP_NET_VLC_DATA
#if defined(NF_AUDIO_DUMP_NET_VLC_DATA)
	#define FILENAME_AUDIO_NET_STREAM_CH0       "/audio_net_ch0.raw"
#endif
/*
	rtsp://ADMIN:1234@192.168.101.16/live/second <Camera>
	rtsp://192.168.101.16/live/main  <Camera>
	rtsp://192.168.101.16:5554/live/main0 <DVR>
*/
void nf_audio_threadNetVlc(NfAudio *self)
{
	gpointer pQueData = NULL;
	gint i, ret, cnt_err=0;

	NF_AUDIO_NET_VLC *pnet=NULL;
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	cpu_core_set(3);

	/* Create audio info */

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	pnet=&self->stAudNetVlc;
	#if defined(NF_AUDIO_DUMP_NET_VLC_DATA)
		FILE *fp_ch0=NULL;

		if((fp_ch0 = fopen(FILENAME_AUDIO_NET_STREAM_CH0, "w")) == NULL)
		{
			g_warning("%s File Open Error!!", __FUNCTION__);
			return ;
		}
		else
			g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
	#endif

	while(NULL == nf_audio_getNetVlc())
		g_usleep(10*1000);

	pnet->init_done = 1;

	/* Start to get streams of each channel. */
	while(self->thread_run_net_vlc)
	{
		NF_AUDIO_QDATA *pstQdata=NULL;

		pQueData = g_async_queue_pop(pnet->queue);
		g_assert( NULL != pQueData );

		pstQdata = (NF_AUDIO_QDATA *)pQueData;
		g_assert(NULL != pstQdata);

		switch(pstQdata->s32Cmd)
		{
			case AUD_CMD_NET_VLC_HANDOFF:
			{
				if(_DEBUG_NF_AUDIO_log[DEBUG_NF_AUDIO_IDX_THREAD_NET_VLC] & (1 << pstQdata->s32Chn)) {
					#if defined(ENABLE_ARCH_A64)
						g_message("[%s] CMD[REC] CH[%d] LEN[%d] S_PTS[%lu] E_PTS[%lu] Diff[%lu]", 
									_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_NET_VLC],
									pstQdata->s32Chn, pstQdata->s32Len, pstQdata->u64Start,
									pstQdata->u64End, pstQdata->u64End - pstQdata->u64Start );
					#else
						g_message("[%s] CMD[REC] CH[%d] LEN[%d] S_PTS[%llu] E_PTS[%llu] Diff[%llu]", 
									_DEBUG_NF_AUDIO_str[DEBUG_NF_AUDIO_IDX_THREAD_NET_VLC],
									pstQdata->s32Chn, pstQdata->s32Len, pstQdata->u64Start,
									pstQdata->u64End, pstQdata->u64End - pstQdata->u64Start );
					#endif
				}

				#if defined(NF_AUDIO_DUMP_NET_VLC_DATA)
					if(pstQdata->s32Chn == 0)
					{
						if(nf_sysman_hotkey_is_nfs())
							fwrite(pstQdata->pData, 1, pstQdata->s32Len, fp_ch0);
					}
				#endif
				/* Handoff */
				if((self->stAudRec.handoffFunc != NULL) &&
					(self->stAudRec.u32HandoffChnMask & (guint)(1 << pstQdata->s32Chn)))
				{
					if (nf_audio_handoff_vlc(pnet, pstQdata) == FALSE)
					{
						#if 0
							g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
							goto exitRec;
						#else
							goto aud_skip_net;
						#endif
					}
				}

				break;
				aud_skip_net:
					if((cnt_err % 20) == 0)
						g_warning("[%s][%d] net send skip!! get gst buffer fail!!", __FUNCTION__, __LINE__);
					break;
			}
			break;
			case AUD_CMD_NET_VLC_CFG:
			{
				NF_AUDIO_PARAM *paramNew = (NF_AUDIO_PARAM *) (pstQdata->pData);
				g_assert( NULL != paramNew );

				//g_message("[%s] CMD[NET CFG]", __FUNCTION__);

				if ( nf_audio_ctrNetVlc( pnet->stParam, paramNew ) == FALSE )
				{
					g_warning("[%s][%d] Audio net configuration failed.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
			break;
			default:
				g_warning("[%s][%d] Invaild cmd.", __FUNCTION__, __LINE__);
				goto exitRec;

		} /* switch */

		if ( nf_audio_freeQdata( pstQdata ) == FALSE )
		{
			g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
			goto exitRec;
		}

		cnt_err++;
	} /* while */

exitRec:
	self->thread_run_net_vlc = 0;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}
#endif

#if defined(ENABLE_AI_ALARM_AUDIO)
static void nf_audio_threadAiAlarm(NfAudio *self)
{  
	guint size=0;
   
	{
		int policy;
		struct sched_param sched;
		pthread_t thread;
 
		policy = SCHED_FIFO;
		thread = pthread_self();

		#if 0 //PRI_ADJUST
			sched.sched_priority = sched_get_priority_max(policy)-2;
		#else
			sched.sched_priority = sched_get_priority_max(policy)-1;
		#endif

		cpu_core_set(3);
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
		g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
		g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
		g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
	}
   
	size=(8000 / AUDIO_DIV_UNIT);
 
	// wait init complete
	while( _nf_audio == NULL ) g_usleep(10*1000);

	self->init_done_ai_alarm = 1;

	self->aud_data_ai.ai_aud_size_send=size;
	while(self->thread_run_ai_alarm) {
   
		NF_OBJECT_LOCK(_nf_audio);
   
		nf_rec_aud_ai_send_frame(&self->aud_data_ai);
   
		NF_OBJECT_UNLOCK(_nf_audio);
		g_usleep(100000);
	}

	g_message("%s end", __FUNCTION__);
	g_thread_exit(0);
}

#endif

static void nf_audio_threadPb_out(NfAudio *self)
{
	gint cnt=0;
	gboolean is_web_mic=FALSE;
	gboolean dbg_pb_out=FALSE;
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	int aud_num = 0;
	int ch = 0;

	aud_num = nf_hw_get_audio_nr();

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	cpu_core_set(3);

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while(self->thread_run_pb_out) {
		
		dbg_pb_out=self->dbg_pb_out;

		#if 0
			if(_nf_audio->aud_status_pb.running_live) {
				if((cnt % 50) == 0) {
					g_message("Audio Live Test Mode!!");
				}
			}
			else {
				if(nf_network_get_webra_audio_status()) {
					nf_audio_set_pb_status(AUD_PB_OUT_MODE_WEB, TRUE);
					g_message("%s running Web Mic");
				}
				else {
					nf_audio_set_pb_status(AUD_PB_OUT_MODE_WEB, FALSE);
				}
			}
		#else

			if(self->dvr_status == 4) {		// Playback Mode -> include/nf_sysman.hi -> NF_DVR_STATUS_RUN_PLAYBACK

				if(nf_network_get_webra_audio_status()) {
					is_web_mic=TRUE;
					if(dbg_pb_out) {
						if((cnt % 50) == 0) {
							g_message("[%s][%d] Audio PB Web Mic Off!! WEB[0 -> X]", __FUNCTION__, __LINE__);
						}
					}
				}
				else {
					if(is_web_mic) {
						is_web_mic=FALSE;

						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio PB Web Mic Off!! WEB[0 -> X]", __FUNCTION__, __LINE__);
							}
						}
					}
					else {
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio PB Web Mic Off!! WEB[X -> X]", __FUNCTION__, __LINE__);
							}
						}
					}
				}

				nf_audio_set_pb_status(AUD_PB_OUT_MODE_WEB, FALSE);
				if(_nf_audio->aud_output_type == AUD_OUTPUT_HDMI) {
					nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, FALSE);
				}

				nf_audio_set_pb_status(AUD_PB_OUT_MODE_DVR, TRUE);

				if(dbg_pb_out) {
					if((cnt % 50) == 0) {
						g_message("[%s][%d] Audio DVR PB Mode!!", __FUNCTION__, __LINE__);
					}
				}
			}
			else {
				nf_audio_set_pb_status(AUD_PB_OUT_MODE_DVR, FALSE);
				if(dbg_pb_out) {
					if((cnt % 50) == 0) {
						g_message("[%s][%d] web_audio_status[%d]", __FUNCTION__, __LINE__, nf_network_get_webra_audio_status());
					}
				}

				if(nf_network_get_webra_audio_status()) {
					is_web_mic=TRUE;

					if(_nf_audio->aud_output_type == AUD_OUTPUT_HDMI) {
						nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, FALSE);
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio DVR Web Mic Mode HDMI LIVE Off!!", __FUNCTION__, __LINE__);
							}
						}
					}
					else {
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio DVR Web Mic Mode!!", __FUNCTION__, __LINE__);
							}
						}
					}
					nf_audio_set_pb_status(AUD_PB_OUT_MODE_WEB, TRUE);

				}
				else {
					if(is_web_mic == TRUE) {
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Web Mic Off!!", __FUNCTION__, __LINE__);
							}
						}
						nf_audio_set_pb_status(AUD_PB_OUT_MODE_WEB, FALSE);
						nf_audio_pb_cb_web_mic(NULL, NULL, 0, 0, 0, TRUE);
						is_web_mic=FALSE;
					}

					if(_nf_audio->aud_output_type == AUD_OUTPUT_HDMI) {
						nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, TRUE);
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio DVR HDMI Live Mode!!", __FUNCTION__, __LINE__);
							}
						}
					}
					else {
						nf_audio_set_pb_status(AUD_PB_OUT_LIVE_HDMI_ON, FALSE);
						if(dbg_pb_out) {
							if((cnt % 50) == 0) {
								g_message("[%s][%d] Audio DVR Live Mode!!", __FUNCTION__, __LINE__);
							}
						}
					}
				}
			}
		#endif

		if(dbg_pb_out) {
			if((cnt % 50) == 0) {
				g_message("[%s][%d] dvr[%d] web_mic[%d] beep[%d] live[%d] hdmi_live[%d] qc[%d]", 
							__FUNCTION__, __LINE__, self->aud_status_pb.running_dvr, self->aud_status_pb.running_web_mic,
							self->aud_status_pb.running_beep, self->aud_status_pb.running_live, 
							self->aud_status_pb.running_live_hdmi, self->aud_status_pb.running_qc);
			}
		}

		cnt++;
		g_usleep(10*1000);
	}
}

static void nf_audio_threadVolume(NfAudio *self)
{
	guint volume_in=0, volume_out=0;
	guint volume_in_prev=0, volume_out_prev=0;;
	gboolean volume_in_mute=0, volume_out_mute=0;


	/*
	   Not Use in DVR / NVR
	*/
	while(self->thread_run_volume) {

		// Spaser Volue Set in ipcam
		if(self->volume_out_mute) {
			if(volume_out_mute == FALSE) {
				g_message("%s line%d Set Volume (Speaker) Set Mute!!", __FUNCTION__, __LINE__);
				volume_out=0;
				nf_audio_nvt_set_volume_output(&self->info_pb, volume_out);

				volume_out_mute=TRUE;
			}
		}
		else {

			volume_out=self->volume_out;

			if(volume_out_mute == TRUE) {
				g_message("%s line%d Set Volume (Speaker) Set Mute On -> Mute Off!! Volume[%d]", 
							__FUNCTION__, __LINE__, self->volume_out);

				nf_audio_nvt_set_volume_output(&self->info_pb, volume_out);
				volume_out_prev=volume_out;
			
				volume_out_mute=FALSE;
			}
			else {
				if(volume_out_prev != self->volume_out) {
					g_message("%s line%d Set Volume (Speaker) Set!! Volume[%d]", __FUNCTION__, __LINE__, self->volume_out);
					
					nf_audio_nvt_set_volume_output(&self->info_pb, volume_out);
					volume_out_prev=volume_out;
				}
			}
		}

		// Mic Volume Set in ipcam
		if(self->volume_in_mute) {
			if(volume_in_mute == FALSE) {
				g_message("%s line%d Set Volume (Mic) Set Mute!!", __FUNCTION__, __LINE__);
				volume_in=0;
	
				nf_audio_nvt_set_volume_intput(&self->info_rec, volume_in);

				volume_in_mute=TRUE;
			}
		}
		else {

			volume_in=self->volume_in;

			if(volume_in_mute == TRUE) {
				g_message("%s line%d Set Volume (Mic) Set Mute On -> Mute Off!! Volume[%d]", __FUNCTION__, __LINE__, self->volume_in);

				nf_audio_nvt_set_volume_intput(&self->info_rec, volume_in);
				volume_in_prev=volume_in;
			
				volume_in_mute=FALSE;
			}
			else {
				if(volume_in_prev != self->volume_in) {
					g_message("%s line%d Set Volume (Mic) Set!! Volume[%d]", __FUNCTION__, __LINE__, self->volume_in);
					
					nf_audio_nvt_set_volume_intput(&self->info_rec, volume_in);
					volume_in_prev=volume_in;
				}
			}
		}

		g_usleep(10*1000);
	}
}

void nf_audio_set_pb_status(int pb_mode, gboolean is_running)
{
	if(pb_mode == AUD_PB_OUT_MODE_DVR) {
		_nf_audio->aud_status_pb.running_dvr=is_running;
	}
	else if(pb_mode == AUD_PB_OUT_MODE_WEB) {
		_nf_audio->aud_status_pb.running_web_mic=is_running;
	}
	else if(pb_mode == AUD_PB_OUT_MODE_BEEP) {
		_nf_audio->aud_status_pb.running_beep=is_running;
	}
	else if(pb_mode == AUD_PB_OUT_MODE_LIVE) {
		_nf_audio->aud_status_pb.running_live=is_running;
	}
	else if(pb_mode == AUD_PB_OUT_MODE_LIVE_HDMI) {
		_nf_audio->aud_status_pb.running_live_hdmi=is_running;
	}
	else {
		g_warning("[func : %s] unknown pb_status.. check please!!", __FUNCTION__);
	}
}

// For DVR PB
//#define AUDIO_DUMP_PB_LOCAL
// stream_size is 8000.
static GStaticMutex mutex_pb_local = G_STATIC_MUTEX_INIT;
int nf_audio_pb_cb_local(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type)
{
	NF_AUDIO_PB *pstAudPb;
	int ret=0, aud_status=1;
	gboolean is_buffer_clear=FALSE;

	g_return_val_if_fail(_nf_audio != NULL, 0);
	g_return_val_if_fail((_nf_audio->stAudRec.init_done ==TRUE), FALSE);

	g_static_mutex_lock(&mutex_pb_local);

	pstAudPb=&_nf_audio->stAudPb;

	ret=nf_audio_pb_local_ipcam(h_stream_buf, stream_buf, stream_size, codec_type, pstAudPb, is_buffer_clear);

	g_static_mutex_unlock(&mutex_pb_local);

    return stream_size;
}

//#define AUDIO_TEST_BEEP
// Warning Stream.. stream_buf is 16bit pcm format..
int nf_audio_pb_cb_beep(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type)
{
	NF_AUDIO_QDATA_PB *data_pb_beep;
	int ret=0, qlen=0;
	char *pu8AudioStream = (char *) g_malloc0(NF_AUD_PINPERFRM);

	#if defined(AUDIO_TEST_BEEP)
		FILE *fp=NULL;
		char filename[32]={0, };

		strncpy(filename, "/out.raw", sizeof(filename));
		if((fp=fopen(filename, "r")) == NULL) {
			g_warning("%s File Open Error!!! File Name [%s]",
						__FUNCTION__, filename);
			goto playback_audio_cb_w_error;
		}
		else {
			g_message("%s line%d File Open Success", __FUNCTION__, __LINE__);
		}
		stream_size=NF_AUD_PINPERFRM;
	#endif

	if(stream_size != NF_AUD_PINPERFRM) {
		g_warning("%s stream_size not match!!", __FUNCTION__);
		ret=FALSE;
	}
	else {

		#if defined(AUDIO_TEST_BEEP)
			glong fsize=0;

			nf_audio_set_pb_status(AUD_PB_OUT_MODE_BEEP, TRUE);
			fseek(fp, 0, SEEK_END);
			fsize=ftell(fp);
			fseek(fp, 0, SEEK_SET);

			while(fsize > 0) {
				data_pb_beep=(NF_AUDIO_QDATA_PB *)g_malloc0( sizeof(NF_AUDIO_QDATA_PB));

				if(fread(pu8AudioStream, stream_size, 1, fp) != 1) {
					g_warning("%s File Read Error!!! ", __FUNCTION__);
					goto playback_audio_cb_w_error;
				}

				memcpy(data_pb_beep->data, (guchar *)pu8AudioStream, stream_size);
				data_pb_beep->cnt=stream_size;
				data_pb_beep->outmode=AUD_PB_OUT_MODE_BEEP;

				qlen=g_async_queue_length(_nf_audio->stAudPb.queue_pb);
				if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
				{
					g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
					goto playback_audio_cb_w_error;
				}

				g_async_queue_push(_nf_audio->stAudPb.queue_pb, data_pb_beep);

				fsize-=stream_size;
			}

			nf_audio_set_pb_status(AUD_PB_OUT_MODE_BEEP, FALSE);
		#else
			data_pb_beep=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));

			memcpy(data_pb_beep->data, stream_buf, NF_AUD_PINPERFRM);
			data_pb_beep->cnt=NF_AUD_PINPERFRM;

			qlen=g_async_queue_length(_nf_audio->stAudPb.queue_pb);
			if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
			{
				g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
				goto playback_audio_cb_w_error;
			}

			g_async_queue_push(_nf_audio->stAudPb.queue_pb, data_pb_beep);
		#endif

		ret=TRUE;
	}

	free(pu8AudioStream);
#if defined(AUDIO_TEST_BEEP)
		fclose(fp);
#endif

	return ret;

playback_audio_cb_w_error:
	free(pu8AudioStream);
	g_free(data_pb_beep);
	ret=FALSE;
	#if defined(AUDIO_TEST_BEEP)
		fclose(fp);
	#endif

	return ret;
}

// For Network Web Mic PB
//#define NF_AUDIO_DUMP_WEB_MIC_DATA
//#define NF_AUDIO_DEBUG_WEB_MIC
static GStaticMutex mutex_web_mic = G_STATIC_MUTEX_INIT;
int nf_audio_pb_cb_web_mic(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
							int hi_aud_stm_cnt, gboolean is_buffer_clear)
{
	gint ret=0;

	g_return_val_if_fail(_nf_audio != NULL, 0);
	g_return_val_if_fail((_nf_audio->stAudRec.init_done ==TRUE), FALSE);

	g_static_mutex_lock(&mutex_web_mic);

	if(NF_AUD_PINPERFRM < NF_AUDIO_DATA_SIZE_WEB_MIC_PCM) {
		ret=nf_audio_pb_cb_web_mic_atype(stream_buf, hi_aud_stm_cnt, stream_size, is_buffer_clear);
	}
	else {
		ret=nf_audio_pb_cb_web_mic_btype(stream_buf, hi_aud_stm_cnt, stream_size, is_buffer_clear);
	}

	g_static_mutex_unlock(&mutex_web_mic);

	return ret;
}

static int nf_audio_pb_cb_web_mic_atype(char *stream_buf, int hi_aud_stm_cnt, int stream_size, gboolean is_buffer_clear)
{
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		short   pcm_val;
	#else
		guchar pcm_val;
	#endif
	guchar  u_val;
	gshort  *AudioStream=NULL;
	gint i=0, j=0;
	gchar *temp_ptr=NULL;
	#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
		static FILE *fp_mic_u=NULL, *fp_mic_p0=NULL, *fp_mic_p1=NULL;
		gboolean is_open_error_u=FALSE, is_open_error_p0=FALSE, is_open_error_p1=FALSE;
	#endif
	gint cnt_send=0, len_data=0;
	static gint remain=0;
	static gchar data_remain[NF_AUD_PINPERFRM]={0, };
   	gchar data_send[NF_AUDIO_DATA_SIZE_WEB_MIC_PCM + NF_AUD_PINPERFRM]={0, };

	#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)

		if(fp_mic_u == NULL) {
			if((fp_mic_u = fopen("/audio_web_mic_uraw.raw", "w")) == NULL) {
				g_warning("%s File Open Error!!", __FUNCTION__);
				is_open_error_u=TRUE;
			}
			else
				g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}

		if(fp_mic_p0 == NULL) {
			if((fp_mic_p0 = fopen("/audio_web_mic_pcm0.raw", "w")) == NULL) {
				g_warning("%s File Open Error!!", __FUNCTION__);
				is_open_error_p0=TRUE;
			}
			else
				g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}

		if(fp_mic_p1 == NULL) {
			if((fp_mic_p1 = fopen("/audio_web_mic_pcm1.raw", "w")) == NULL) {
				g_warning("%s File Open Error!!", __FUNCTION__);
				is_open_error_p1=TRUE;
			}
			else
				g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}


		if(nf_sysman_hotkey_is_nfs()) {
			if(!is_open_error_u) {
				fwrite(stream_buf, 1, NF_AUDIO_DATA_SIZE_WEB_MIC, fp_mic_u);
				#if 0
					g_message("Web Mic Data Write!!! Uraw");
				#endif
			}
		}
	#endif

	if(is_buffer_clear) {

		g_message("[%s] Web Mic Buffer Clear!!", __FUNCTION__);

		remain=0;
		memset(data_send, 0x0, sizeof(data_send));
		memset(data_remain, 0x0, sizeof(data_remain));

		return stream_size;
	}

	AudioStream=(gshort *) g_malloc0(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM);
	if(AudioStream == NULL)
		g_message("%s alloc fail!!", __FUNCTION__);

	// Converting
	for(j=0; j<hi_aud_stm_cnt; j++) {
		gint index=0;

		temp_ptr = stream_buf + (j * NF_AUDIO_DATA_SIZE_WEB_MIC);

		for(i=0; i <NF_AUDIO_DATA_SIZE_WEB_MIC; i++) {
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
				pcm_val = nf_audio_cvt_muraw_to_lpcm16(u_val);
			#else
				pcm_val = u_val;
			#endif

			index=(i + (j * NF_AUDIO_DATA_SIZE_WEB_MIC));
			*(AudioStream + index) = pcm_val;
		}
	}

	#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
		if(nf_sysman_hotkey_is_nfs()) {
			if(!is_open_error_p0) {
				fwrite(AudioStream, 1, NF_AUDIO_DATA_SIZE_WEB_MIC_PCM, fp_mic_p0);
				#if 0
					g_message("Web Mic Data Write!!! PCM0");
				#endif
			}
		}
	#endif

	memcpy(data_send, data_remain, (size_t)remain);
	memcpy(data_send + remain, AudioStream, (size_t)NF_AUDIO_DATA_SIZE_WEB_MIC_PCM);

	free(AudioStream);

	remain += NF_AUDIO_DATA_SIZE_WEB_MIC_PCM;
	cnt_send=0;
	while(remain >= NF_AUD_PINPERFRM) {

		NF_AUDIO_QDATA_PB *data_pb_web_mic;
		gint cnt_retry=0, qlen=0;

		data_pb_web_mic=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));
		#if defined(NF_AUDIO_DEBUG_WEB_MIC)
			g_message("%s line%d Send Queue", __FUNCTION__, __LINE__);
		#endif

		memcpy(data_pb_web_mic->data, (data_send + (cnt_send * NF_AUD_PINPERFRM)), NF_AUD_PINPERFRM);

		#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
			if(nf_sysman_hotkey_is_nfs()) {
				if(!is_open_error_p1) {
					fwrite(data_pb_web_mic->data, 1, NF_AUD_PINPERFRM, fp_mic_p1);
					#if 0
						g_message("Web Mic Data Write!!! PCM1");
					#endif
				}
			}
		#endif

		data_pb_web_mic->cnt=NF_AUD_PINPERFRM;
		data_pb_web_mic->outmode=AUD_PB_OUT_MODE_WEB;

nf_audio_retry_pb_web_mic_atype:
		qlen=g_async_queue_length(_nf_audio->stAudPb.queue_pb);
		if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE) {
#if 1
			int i;
			NF_AUDIO_QDATA_PB *qdata_tmp = NULL;
			g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
			g_free(data_pb_web_mic);
			// free(AudioStream);
			for (i =0; i < qlen; i++) {
				qdata_tmp = g_async_queue_pop(_nf_audio->stAudPb.queue_pb);
				g_free(qdata_tmp);
			}

			remain = 0;
			return FALSE;
#else
			cnt_retry++;
			g_usleep(100000);
			goto nf_audio_retry_pb_web_mic_atype;
#endif
		}

		if(cnt_retry != 0) {
			g_warning("%s queue full.. retry_count[%d]", __FUNCTION__, cnt_retry);
		}

		g_async_queue_push(_nf_audio->stAudPb.queue_pb, data_pb_web_mic);

		remain -= NF_AUD_PINPERFRM;
		cnt_send++;
	}

	memcpy(data_remain, data_send+(cnt_send * NF_AUD_PINPERFRM), (size_t)remain);

	return stream_size;
}

static int nf_audio_pb_cb_web_mic_btype(char *stream_buf, int hi_aud_stm_cnt, int stream_size, gboolean is_buffer_clear)
{
	NF_AUDIO_QDATA_PB *data_pb_web_mic;
	gint qlen=0;
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		short   pcm_val;
	#else
		guchar pcm_val;
	#endif  
	guchar  u_val;
	gshort  *AudioStream=NULL;
	gint i=0, j=0;
	gchar *temp_ptr=NULL;
	#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
		static FILE *fp_mic=NULL;
		gboolean is_open_error=FALSE;
		gboolean is_uraw=FALSE;
	#endif
	// For Gather Data
	static int remain=0, size_total=0;
	gint size_copied=0;
	static gchar data_remain[NF_AUDIO_DATA_SIZE_WEB_MIC_PCM]={0, }, data_send[NF_AUD_PINPERFRM]={0, };
	gchar *ptr=NULL;
	gint tmp=0;
	static gboolean is_remain_short=FALSE;
	static int remain_size_short=0;

	#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)

		if(fp_mic == NULL) {
			if((fp_mic = fopen("/audio_web_mic.raw", "w")) == NULL) {
				g_warning("%s File Open Error!!", __FUNCTION__);
				is_open_error=TRUE;
			}
			else
				g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}

		if(is_uraw) {
			if( nf_sysman_hotkey_is_nfs() )
			{
				if(!is_open_error) {
					fwrite(stream_buf, 1, NF_AUDIO_DATA_SIZE_WEB_MIC, fp_mic);
					g_message("Web Mic Data Write!!! Uraw");
				}
			}
		}
	#endif

	if(is_buffer_clear) {

		g_message("[%s] Web Mic Buffer Clear!!", __FUNCTION__);

		size_copied=0; size_total=0; remain=0;
		memset(data_send, 0x0, sizeof(data_send));
		memset(data_remain, 0x0, sizeof(data_remain));

		return stream_size;
	}


	AudioStream=(gshort *) g_malloc0(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM);
	if(AudioStream == NULL)
		g_message("%s alloc fail!!", __FUNCTION__);

	// Converting
	for(j=0; j<hi_aud_stm_cnt; j++)
	{
		gint index=0;

		temp_ptr = stream_buf + (j * NF_AUDIO_DATA_SIZE_WEB_MIC);

		for(i=0; i <NF_AUDIO_DATA_SIZE_WEB_MIC; i++)
		{
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
				pcm_val = nf_audio_cvt_muraw_to_lpcm16(u_val);
			#else
				pcm_val = u_val;
			#endif

			index=(i + (j * NF_AUDIO_DATA_SIZE_WEB_MIC));
			*(AudioStream + index) = pcm_val;
		}
	}

	#if 0
		#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
			if( nf_sysman_hotkey_is_nfs() )
			{
				if(!is_open_error) {
					fwrite(AudioStream, 1, NF_AUDIO_DATA_SIZE_WEB_MIC_PCM, fp_mic);
					g_message("Web Mic Data Write!!! PCM");
				}
			}
		#endif
	#endif


	#if 1       // Novatek.. for pin_per_size 2048
		// Gather Data.. 
		ptr=(gchar *)AudioStream;       // size is 1600 byte
		if(size_total == 0) {

			if(remain == 0){
				size_copied=NF_AUDIO_DATA_SIZE_WEB_MIC_PCM;
				#if defined(NF_AUDIO_DEBUG_WEB_MIC)
					g_message("%s line%d Write 1 !! remain is 0.. copysize[%d]", __FUNCTION__, __LINE__, size_copied);
				#endif
				#if defined(NF_AUDIO_DEBUG_WEB_MIC)
					if(size_copied > NF_AUD_PINPERFRM)
						g_warning("%s line%d Warning!!! memory exceed!!", __FUNCTION__, __LINE__);
				#endif
				memcpy(data_send, ptr, (size_t)size_copied);

				remain=0;
				is_remain_short=TRUE;
				remain_size_short=(NF_AUD_PINPERFRM - size_copied);
			}
			else {
				tmp=(NF_AUD_PINPERFRM - remain);
					#if 0
						{
							int size=0;
							size=remain+tmp;
							
							if(size >NF_AUDIO_DATA_SIZE_WEB_MIC_PCM) {
								printf("%s line%d Size Deubg!! Exceed!!!!!!! size[%d] remain[%d] tmp[%d]\n",
											__FUNCTION__, __LINE__, size, remain, tmp);
								if(remain
							}
							else if(size == NF_AUDIO_DATA_SIZE_WEB_MIC_PCM) {
								printf("%s line%d Size Deubg!! Same!!!!!!! size[%d] remain[%d] tmp[%d]\n",
											__FUNCTION__, __LINE__, size, remain, tmp);
							}
							else {
								printf("%s line%d Size Deubg!! Exceed!!!!!!!  XXXX size[%d] remain[%d] tmp[%d]\n",
											__FUNCTION__, __LINE__, size, remain, tmp);
							}
						}
					#endif

				if(tmp <= NF_AUDIO_DATA_SIZE_WEB_MIC_PCM) {
					#if defined(NF_AUDIO_DEBUG_WEB_MIC)
						g_message("%s line%d Write 2 !! size_total[%d] remain[%d] tmp[%d]",
									__FUNCTION__, __LINE__, size_total, remain, tmp);
					#endif

					// copy remain data
					#if defined(NF_AUDIO_DEBUG_WEB_MIC)
						if(remain > NF_AUD_PINPERFRM)
							g_warning("%s line%d Warning!!! memory exceed!! remain %d", __FUNCTION__, __LINE__, remain);
					#endif
					memcpy(data_send, (gchar *)data_remain, remain);

					size_copied=remain;

					// copy web mic data
					#if defined(NF_AUDIO_DEBUG_WEB_MIC)
						if(tmp > (NF_AUD_PINPERFRM - remain)) {
							g_warning("%s line%d Warning!!! tmp[%d] memory exceed!! %d ",
										__FUNCTION__, __LINE__, tmp, (NF_AUD_PINPERFRM - remain));
						}
					#endif
					memcpy(data_send + remain, ptr, (size_t)tmp);
					size_copied+=tmp;

					#if 1
						#if defined(NF_AUDIO_DEBUG_WEB_MIC)
							if((remain + tmp) < NF_AUD_PINPERFRM) {
								g_message("%s line%d Write 7 Shortage Data!! %d ", __FUNCTION__, __LINE__, remain + tmp);
							}
						#endif
						if(tmp == NF_AUDIO_DATA_SIZE_WEB_MIC_PCM) {
							remain=(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM - tmp);
							#if defined(NF_AUDIO_DEBUG_WEB_MIC)
								g_message("%s line%d Write 5 !! remain is 0.. size_copied[%d] size_total[%d] remain[%d] tmp[%d]",
											__FUNCTION__, __LINE__, size_copied, size_total, remain, tmp);
							#endif
						}
						else {
							// renew remain size and copy
							remain=(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM - tmp);

							#if defined(NF_AUDIO_DEBUG_WEB_MIC)
								if(remain > NF_AUD_PINPERFRM) {
									g_warning("%s line%d Warning!!! memory exceed!! remain[%d]", __FUNCTION__, __LINE__, remain);
								}
							#endif
							memcpy(data_remain, ptr + tmp, remain);
						}
					#else
						// renew remain size and copy
						remain=(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM - tmp);
						g_message("%s line%d Write 5 !! remain is 0.. size_copied[%d] size_total[%d] remain[%d] tmp[%d]", 
									__FUNCTION__, __LINE__, size_copied, size_total, remain, tmp);
						
						#if defined(NF_AUDIO_DEBUG_WEB_MIC)
							if(remain > NF_AUD_PINPERFRM) {
								g_warning("%s line%d Warning!!! memory exceed!! remain[%d]", __FUNCTION__, __LINE__, remain);
							}
						#endif
						memcpy(data_remain, ptr + tmp, remain);
					#endif
				}
				else {
					#if defined(NF_AUDIO_DEBUG_WEB_MIC)
						g_message("%s line%d Write 3 !! Shortage Data remain[%d] tmp[%d]", __FUNCTION__, __LINE__, remain, tmp);
						if(remain > NF_AUD_PINPERFRM) {
							g_warning("%s line%d Warning!!! memory exceed!! %d", __FUNCTION__, __LINE__, remain);
						}
					#endif
					memcpy(data_send, (gchar *)data_remain, remain);

					size_copied=remain;

					#if defined(NF_AUDIO_DEBUG_WEB_MIC)
						if(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM > (NF_AUD_PINPERFRM - remain)) {
							g_warning("%s line%d Warning!!! memory exceed!! %d",
										__FUNCTION__, __LINE__, (NF_AUD_PINPERFRM - remain));
						}
					#endif
					memcpy(data_send + remain, ptr, NF_AUDIO_DATA_SIZE_WEB_MIC_PCM);
					size_copied+=NF_AUDIO_DATA_SIZE_WEB_MIC_PCM;

					remain=0;
					is_remain_short=TRUE;
					remain_size_short=(NF_AUD_PINPERFRM - size_copied);
				}
			}
		}
		else {

			if(is_remain_short) {
				gint offset=0;

				#if defined(NF_AUDIO_DEBUG_WEB_MIC)
					g_message("%s line%d Write 4 !! size_total[%d] remain[%d] tmp[%d] remain_size_short[%d]",
							__FUNCTION__, __LINE__, size_total, remain, tmp, remain_size_short);
				#endif
				offset=(NF_AUD_PINPERFRM - remain_size_short);
				tmp=remain_size_short;

				#if defined(NF_AUDIO_DEBUG_WEB_MIC)
					if(tmp > (NF_AUD_PINPERFRM - offset)) {
						g_warning("%s line%d Warning!!! memory exceed!! tmp %d -> %d",
									__FUNCTION__, __LINE__, tmp, (NF_AUD_PINPERFRM - offset));
					}
				#endif
				memcpy(data_send + offset, ptr, (size_t)tmp);
				size_copied=tmp;

				remain=(NF_AUDIO_DATA_SIZE_WEB_MIC_PCM - tmp);
				#if defined(NF_AUDIO_DEBUG_WEB_MIC)
					if(remain > NF_AUD_PINPERFRM) {
						g_warning("%s line%d Warning!!! memory exceed!!", __FUNCTION__, __LINE__);
					}
				#endif

				memcpy(data_remain, ptr + tmp, remain);
				is_remain_short=FALSE;
				remain_size_short=0;
			}
			else {
				g_message("%s line%d audio web mic data is something worng!!! size_total[%d]", __FUNCTION__, __LINE__, size_total);
			}
		}

		size_total+=size_copied;

		#if defined(NF_AUDIO_DEBUG_WEB_MIC)
			g_message("%s line%d size_total[%d]", __FUNCTION__, __LINE__, size_total);
		#endif
		if(size_total == NF_AUD_PINPERFRM) {
			gint cnt_retry=0;

			data_pb_web_mic=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));
			#if defined(NF_AUDIO_DEBUG_WEB_MIC)
				g_message("%s line%d Send Queue", __FUNCTION__, __LINE__);
			#endif

			memcpy(data_pb_web_mic->data, data_send, NF_AUD_PINPERFRM);
			#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
				if(!is_uraw) {
					if( nf_sysman_hotkey_is_nfs() )
					{
						if(!is_open_error) {
							fwrite(data_pb_web_mic->data, 1, NF_AUD_PINPERFRM, fp_mic);
							g_message("Web Mic Data Write!!! PCM");
						}
					}
				}
			#endif
			data_pb_web_mic->cnt=NF_AUD_PINPERFRM;
			data_pb_web_mic->outmode=AUD_PB_OUT_MODE_WEB;

nf_audio_retry_pb_web_mic:
			qlen=g_async_queue_length(_nf_audio->stAudPb.queue_pb);
			if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
			{
#if 1
			int i;
			NF_AUDIO_QDATA_PB *qdata_tmp = NULL;
			g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
			g_free(data_pb_web_mic);
			// free(AudioStream);
			for (i =0; i < qlen; i++) {
				qdata_tmp = g_async_queue_pop(_nf_audio->stAudPb.queue_pb);
				free(qdata_tmp);
			}

			remain = 0;
			return FALSE;
#else
			cnt_retry++;
			g_usleep(100000);
			goto nf_audio_retry_pb_web_mic_atype;
#endif
			}

			if(cnt_retry != 0)
				g_warning("%s queue full.. retry_count[%d]", __FUNCTION__, cnt_retry);

			g_async_queue_push(_nf_audio->stAudPb.queue_pb, data_pb_web_mic);

			size_total=0;
		}

		free(AudioStream);

	#else
		memcpy(data_pb_web_mic->data_web, AudioStream, NF_AUDIO_DATA_SIZE_WEB_MIC_PCM);
		#if defined(NF_AUDIO_DUMP_WEB_MIC_DATA)
			if(!is_uraw) {
				if( nf_sysman_hotkey_is_nfs() )
				{
					if(!is_open_error) {
						fwrite(data_pb_web_mic->data, 1, NF_AUD_PINPERFRM, fp_mic);
						g_message("Web Mic Data Write!!! PCM");
					}
				}
			}
		#endif
		data_pb_web_mic->cnt=NF_AUDIO_DATA_SIZE_WEB_MIC_PCM;
		data_pb_web_mic->outmode=AUD_PB_OUT_MODE_WEB;

		free(AudioStream);

		qlen=g_async_queue_length(_nf_audio->stAudPb.queue_pb);
		if(qlen > NF_AUDIO_NET_PB_MAX_QUEUE)
		{
			g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
			g_free(data_pb_web_mic);
			return FALSE;
		}

		g_async_queue_push(_nf_audio->stAudPb.queue_pb, data_pb_web_mic);
	#endif

	return stream_size;
}

static void nf_audio_set_input_table(gint *table)
{
	table[0]=AUD_INPUT_LV0;
	table[1]=AUD_INPUT_LV1;
	table[2]=AUD_INPUT_LV2;
	table[3]=AUD_INPUT_LV3;
	table[4]=AUD_INPUT_LV4;
	table[5]=AUD_INPUT_LV5;
	table[6]=AUD_INPUT_LV6;
	table[7]=AUD_INPUT_LV7;
	table[8]=AUD_INPUT_LV8;
	table[9]=AUD_INPUT_LV9;
	table[10]=AUD_INPUT_LV10;
	table[11]=AUD_INPUT_LV11;
	table[12]=AUD_INPUT_LV12;
	table[13]=AUD_INPUT_LV13;
	table[14]=AUD_INPUT_LV14;
	table[15]=AUD_INPUT_LV15;
}

static void nf_audio_volume_set(void)
{
	char tmp_key[256]={0, };
	guint volume=0, is_mute=0;
	gboolean is_enable=0;

	sprintf(tmp_key, "audio.enable");       // speaker
	is_enable=nf_sysdb_get_bool(tmp_key);
	if(is_enable) {
		is_mute=FALSE;
	}
	else {
		is_mute=TRUE;
	}
		
	sprintf(tmp_key, "audio.out_volume");       // speaker
	volume = nf_sysdb_get_uint(tmp_key);

	nf_audio_ctrl_vol_dac(is_mute, volume);

	sprintf(tmp_key, "audio.in_volume");        // mic
	volume = nf_sysdb_get_uint(tmp_key);

	nf_audio_ctrl_vol_adc(is_mute, volume);
}

gboolean nf_audio_ctrl_vol_adc(gint is_mute, guint volume)
{
	g_return_val_if_fail (_nf_audio != NULL, FALSE);

	_nf_audio->volume_out_mute=is_mute;
	_nf_audio->volume_out=volume;

	g_message("%s line%d is_mute[%d] volume[%d]", __FUNCTION__, __LINE__, _nf_audio->volume_out_mute, _nf_audio->volume_out);	

	return TRUE;
}

gboolean nf_audio_ctrl_vol_dac(gint is_mute, guint volume)
{
	g_return_val_if_fail (_nf_audio != NULL, FALSE);

	_nf_audio->volume_in_mute=is_mute;
	_nf_audio->volume_in=volume;

	g_message("%s line%d is_mute[%d] volume[%d]", __FUNCTION__, __LINE__, _nf_audio->volume_in_mute, _nf_audio->volume_in);	

	return TRUE;
}

#if defined(ENABLE_AI_ALARM_AUDIO)
void nf_audio_ai_alarm_test(guint mask_alarm_ch, char *str)
{
	gchar *s=NULL;

	g_message("%s line%d ch_mask[0x%08x] str[%s]\n", __FUNCTION__, __LINE__, mask_alarm_ch, str);
	_nf_audio->aud_data_ai.mask_ai_evt_test |= mask_alarm_ch;
	s=_nf_audio->aud_data_ai.ai_evt_aud_test_filename;
	sprintf(s, "/opt/%s", str);

	_nf_audio->aud_data_ai.ai_evt_aud_test=TRUE;
}  
#endif

/**
 *  used to get current system time in ms.
 */
unsigned int nf_audio_get_current_time(void)
{
	struct  timeval now_timeval;
	UINT ts_in_ms;
	gettimeofday(&now_timeval, NULL);
	ts_in_ms = 1000 * ((unsigned long long)now_timeval.tv_sec) +
			   ((unsigned long long)now_timeval.tv_usec / 1000);
	return ts_in_ms;
}

static void nf_audio_dump_icodec_header(const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", str ,
					pheader->chan,
					pheader->flags,
					pheader->frame_type,
					pheader->frame_rate,
					pheader->resolution,
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size);
}

void
nf_audio_dump_icodec_header_extern(int idx, ICODEC_HEADER *pheader, int ch)
{
	if(_DEBUG_NF_AUDIO_log[idx] & (1 << ch)) {
		nf_audio_dump_icodec_header(_DEBUG_NF_AUDIO_str[idx], pheader);
	}
}

#include "jbshell.h"
static char nf_audio_pb_help[] = "nf_aud_pb mode [pbmode] [is_running]";
int jbshell_nf_audio_pb(int argc, char **argv)
{
	if(argc < 4) {
		printf("Invalid arguments\n%s\n", nf_audio_pb_help);
		return -1;
	}

	if(strcmp(argv[1], "mode") == 0) {
		gint pb_mode=0;
		gboolean is_running=0;

		if ( argc < 4 ) {
			printf("Invalid arguments\n%s\n", nf_audio_pb_help);
			return -1;
		}

		pb_mode = (gint)strtoul(argv[2], NULL, 0);
		is_running = (gboolean)strtoul(argv[3], NULL, 0);

		g_message("%s line%d pb_mode[%d] is_running[%d]", __FUNCTION__, __LINE__, pb_mode, is_running);

		nf_audio_set_pb_status(pb_mode, is_running);
	}
	else {
		printf("Invalid arguments\n%s\n", nf_audio_pb_help);
		return -1;
	}


	return 0;
}
__commandlist(jbshell_nf_audio_pb,"nf_aud_pb", nf_audio_pb_help, nf_audio_pb_help);

static char nf_audio_dbg_help[] = "nf_aud_dbg [mode] [is_enable]";
int jbshell_nf_audio_dbg(int argc, char **argv)
{   
	gint mode=0;
	gboolean is_enable=0;

	if(argc < 3) {
		printf("Invalid arguments\n%s\n", nf_audio_pb_help);
		return -1;
	}
	
	mode = (gint)strtoul(argv[1], NULL, 0);
	is_enable = (gboolean)strtoul(argv[2], NULL, 0);

	if(mode == 0) {

		g_message("%s line%d mode[%d] is_enable[%d]", __FUNCTION__, __LINE__, mode, is_enable);
		_nf_audio->dbg_pb=is_enable;
	}
	else if(mode == 1) {

		g_message("%s line%d mode[%d] is_enable[%d]", __FUNCTION__, __LINE__, mode, is_enable);
		_nf_audio->dbg_pb_out=is_enable;
	}
	else {
		printf("Invalid arguments\n%s\n", nf_audio_dbg_help);
		return -1;
	}

	
	return 0;
}
__commandlist(jbshell_nf_audio_dbg,"nf_aud_dbg", nf_audio_dbg_help, nf_audio_dbg_help);

