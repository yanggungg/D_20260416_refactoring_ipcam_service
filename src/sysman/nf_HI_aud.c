#include <libsst.h>
#include <gst/gst.h>
#include <gst/nf/gstnfbuddybuffer.h>

#include "nf_codec_header.h"
#include "nf_HI_aud.h"
#include "nf_HI_aud_api.h"

#include "nf_util_device.h"
#include "nf_api_live.h"
#include "nf_rec_audio.h"

#include "nf_cntl_audio.h"
#include "nf_util_device.h"

#include "HA.AUDIO.AAC.encode.h"

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

#define DBG_MSG_AUD_REC     DBG_MSG_NONE
//#define DBG_MSG_AUD_REC   DBG_MSG_AUD
#define DBG_MSG_AUD_CFG     DBG_MSG_NONE
//#define DBG_MSG_AUD_CFG   DBG_MSG_AUD

// #define HI_ACODEC_TYPE_HDMI		1
/**
	Gloval Function
**/
static void nf_HI_aud_threadRec(NfHiaud *self);
static void nf_HI_aud_threadRd(NfHiaud *self);
#if defined(ENABLE_REC_NET_IMMEDIATELY)
	static void nf_HI_aud_threadNet(NfHiaud *self);
#endif
static void nf_HI_aud_threadNet_pb(NfHiaud *self);
static void nf_HI_aud_thread_Output_pb(NfHiaud *self);

static void nf_HI_aud_class_init (NfHiaudClass * klass);
static void nf_HI_aud_instance_init (GTypeInstance* instance, gpointer g_class);
static void nf_HI_aud_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void nf_HI_aud_dispose (GObject * object);
static void nf_HI_aud_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void nf_HI_aud_finalize (GObject * object);

static gboolean nf_HI_aud_init_lib(guint aud_output_type);
static void nf_HI_aud_codec_cfg(void);
static void nf_HI_aud_get_attr(void);
static void nf_HI_aud_get_attr_hdmi(void);

static void _nf_HI_aud_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
NF_HI_AUD_QDATA *nf_HI_aud_CreateQdata( HI_U32 s32Len );
static gpointer _nf_HI_aud_CreateQdata_gst_buffer(GstBuffer *buffer);
HI_BOOL nf_HI_aud_freeQdata( NF_HI_AUD_QDATA *pstQdata );
static HI_BOOL _nf_HI_aud_freeQdata_gst_buffer(NF_HI_AUD_QDATA *pstQdata);

static gboolean
_nf_HI_aud_live_put_frame(gint curr_ch , NF_HI_AUD_RD *self, gpointer data);

static GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose);
static void
_nf_HI_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader);

/**
	Extern Function
**/

/**
	Gloval Variable
**/
static GObjectClass *parent_class = NULL;
static NfHiaud *_nf_HI_aud = NULL;

static gint _tbl_aud_input[16] =
{
	#if defined(USE_DEV_DECODER_IDEN1100)
		#if (NUM_AUDIO == 16)
			HI_AUD_INPUT_LV0,   HI_AUD_INPUT_LV2,   HI_AUD_INPUT_LV4,   HI_AUD_INPUT_LV6,   HI_AUD_INPUT_LV8,
			HI_AUD_INPUT_LV10,  HI_AUD_INPUT_LV12,  HI_AUD_INPUT_LV14,  HI_AUD_INPUT_LV1,   HI_AUD_INPUT_LV3,
			HI_AUD_INPUT_LV5,   HI_AUD_INPUT_LV7,   HI_AUD_INPUT_LV9,   HI_AUD_INPUT_LV11,  HI_AUD_INPUT_LV13,
			HI_AUD_INPUT_LV15
		#elif (NUM_AUDIO == 8)
			HI_AUD_INPUT_LV0,   HI_AUD_INPUT_LV2,   HI_AUD_INPUT_LV4,   HI_AUD_INPUT_LV6,   HI_AUD_INPUT_LV1,
			HI_AUD_INPUT_LV3,   HI_AUD_INPUT_LV5,   HI_AUD_INPUT_LV7,   HI_AUD_INPUT_LV8,   HI_AUD_INPUT_LV10,
			HI_AUD_INPUT_LV14,  HI_AUD_INPUT_LV12,  HI_AUD_INPUT_LV9,   HI_AUD_INPUT_LV11,  HI_AUD_INPUT_LV13,
			HI_AUD_INPUT_LV15
		#elif (NUM_AUDIO == 4)
			HI_AUD_INPUT_LV0,   HI_AUD_INPUT_LV2,   HI_AUD_INPUT_LV1,   HI_AUD_INPUT_LV3,   HI_AUD_INPUT_LV4,
			HI_AUD_INPUT_LV6,   HI_AUD_INPUT_LV5,   HI_AUD_INPUT_LV7,   HI_AUD_INPUT_LV8,   HI_AUD_INPUT_LV10,
			HI_AUD_INPUT_LV14,  HI_AUD_INPUT_LV12,  HI_AUD_INPUT_LV9,   HI_AUD_INPUT_LV11,  HI_AUD_INPUT_LV13,
			HI_AUD_INPUT_LV15
		#endif
	#else
		HI_AUD_INPUT_LV0,   HI_AUD_INPUT_LV1,   HI_AUD_INPUT_LV2,   HI_AUD_INPUT_LV3,   HI_AUD_INPUT_LV4,
		HI_AUD_INPUT_LV5,   HI_AUD_INPUT_LV6,   HI_AUD_INPUT_LV7,   HI_AUD_INPUT_LV8,   HI_AUD_INPUT_LV9,
		HI_AUD_INPUT_LV10,  HI_AUD_INPUT_LV11,  HI_AUD_INPUT_LV12,  HI_AUD_INPUT_LV13,  HI_AUD_INPUT_LV14,
		HI_AUD_INPUT_LV15
	#endif
};


GType
nf_HI_aud_get_type (void)
{
	static GType nf_HI_aud_type = 0;

	if (G_UNLIKELY (nf_HI_aud_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfHiaudClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_HI_aud_class_init,
			NULL,
			NULL,
			sizeof (NfHiaud),
			0,
			(GInstanceInitFunc) nf_HI_aud_instance_init,
			NULL
		};

		nf_HI_aud_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfHiaud", &object_info, 0);
	}

	return nf_HI_aud_type;
}

static void
nf_HI_aud_class_init (NfHiaudClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_HI_aud_set_property;
	gobject_class->get_property = nf_HI_aud_get_property;

	gobject_class->dispose = nf_HI_aud_dispose;
	gobject_class->finalize = nf_HI_aud_finalize;

}

static void
nf_HI_aud_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfHiaud *self = NF_HI_AUD(instance);

	self->init_done = 0;

	// event context & loop
	self->context = g_main_context_new ();
	self->loop = g_main_loop_new (self->context, FALSE);

	// queue
	self->stAudRec.queue = g_async_queue_new();
	self->stAudRd.queue = self->stAudRec.queue;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		self->stAudNet.queue = g_async_queue_new();
		self->stAudRd.queue_net = self->stAudNet.queue;
	#endif
	self->stAudPb.queue_net_pb = g_async_queue_new();
	self->stAudPb.queue_output_pb = g_async_queue_new();

#if 0
	self->net_event = 0;
	self->thread_run = 1;

	memset( self->sensor_data, 0x00, sizeof(self->sensor_data) );

	self->thread = g_thread_create( (GThreadFunc)event_thread_func,
									self, FALSE, NULL);

	self->netmon_thread = g_thread_create(  (GThreadFunc)event_netmon_thread_func,
									self, FALSE, NULL);
#endif

}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_HI_aud_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_HI_aud_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_HI_aud_set_property (GObject * object, guint prop_id,
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
nf_HI_aud_get_property (GObject * object, guint prop_id,
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

gboolean nf_HI_aud_init(void)
{
	NF_HI_AUD_RD *pstAudRd = NULL;
	NF_HI_AUD_REC *pstAudRec = NULL;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		NF_HI_AUD_REC_S *pstAudNet = NULL;
	#endif
	gulong cb_handle=0;
	gint ch=0;

	g_return_val_if_fail (_nf_HI_aud == NULL, FALSE);

	_nf_HI_aud = g_object_new ( NF_TYPE_HI_AUD , NULL);

	pstAudRd = nf_HI_aud_getRd();
	pstAudRec = nf_HI_aud_getRec();
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		pstAudNet = nf_HI_aud_getNet();
	#endif

	g_assert ( NULL != pstAudRd );
	g_assert ( NULL != pstAudRec );
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		g_assert ( NULL != pstAudNet );
	#endif

	_nf_HI_aud->aud_output_type = nf_live_get_audio_output_type();

	g_message( "[%s] Audio type%d init #Start#", __FUNCTION__, _nf_HI_aud->aud_output_type);
#if 0
	_nf_HI_aud->fdAcodec=nf_dev_open_acodec();
	// Volume Contorl
	nf_HI_aud_cntl_acodec_set_adcl_vol(24);		// HWT Request 6dB
	nf_HI_aud_cntl_acodec_set_dacl_vol(6);		// HWT Request 0dB

	nf_HI_aud_init_lib(_nf_HI_aud->aud_output_type);

	if ( nf_HI_aud_getFd( pstAudRd->s32AudFd, &pstAudRd->s32MaxFd ) == HI_FAILURE )
	{
		g_error("[%s][%d] fail.", __FUNCTION__, __LINE__);
	}

	cb_handle= nf_notify_connect_cb( "vloss", _nf_HI_aud_vloss_cb_func , (gpointer)_nf_HI_aud);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	#if defined(ENABLE_REC_LIVE_AUDIO_HI)
		pstAudRd->live_ch0 =NF_REC_AUDIO_DAC_PLAYBACK;
		#if defined(ENABLE_AUD_STEREO_OUT)
			pstAudRd->live_ch1 =NF_REC_AUDIO_DAC_PLAYBACK;
		#endif
	#endif

	/* Set CH & stream id */
	for ( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
	{
		pstAudRec->stParam[ch].u8Chn = 0xFF;
		pstAudRec->stParam[ch].s32StmId = -1;
	}
	 pstAudRec->handoffFunc = NULL;

	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		/* Set CH & stream id */
		for ( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
		{
			pstAudNet->stParam[ch].u8Chn = 0xFF;
			pstAudNet->stParam[ch].s32StmId = -1;
		}
		pstAudNet->handoffFunc = NULL;
	#endif

	pstAudRd->isEnable = FALSE;
	pstAudRec->isEnable = FALSE;

	// Thread Enable
	_nf_HI_aud->thread_run_rd = TRUE;
	_nf_HI_aud->thread_run_rec = TRUE;
	_nf_HI_aud->thread_run_net = TRUE;
	_nf_HI_aud->thread_run_net_pb = TRUE;
	_nf_HI_aud->thread_run_output_pb = TRUE;

	// Thread Create
	_nf_HI_aud->thread_rec = g_thread_create((GThreadFunc)nf_HI_aud_threadRec,
										_nf_HI_aud, FALSE, NULL);
	_nf_HI_aud->thread_rd = g_thread_create((GThreadFunc)nf_HI_aud_threadRd,
										_nf_HI_aud, FALSE, NULL);
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		_nf_HI_aud->thread_net = g_thread_create((GThreadFunc)nf_HI_aud_threadNet,
											_nf_HI_aud, FALSE, NULL);
	#endif
	_nf_HI_aud->thread_net_pb = g_thread_create((GThreadFunc)nf_HI_aud_threadNet_pb,
										_nf_HI_aud, FALSE, NULL);

	_nf_HI_aud->thread_output_pb = g_thread_create ( (GThreadFunc)nf_HI_aud_thread_Output_pb, 
										_nf_HI_aud, FALSE, NULL );
#else
	#if defined(USE_DEV_HID_RAW)
		nf_HI_aud_codec_cfg();
	#endif
	#if 0
		nf_HI_aud_init_lib(_nf_HI_aud->aud_output_type);
	#endif
#endif

	HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] Audio init #END#", __FUNCTION__);

	return HI_SUCCESS;
}

static gboolean nf_HI_aud_init_lib(guint aud_output_type)
{
	HI_S32 Ret;
	HI_HANDLE hAenc;
	HI_HANDLE hAi;
	HI_UNF_AENC_ATTR_S stAencAttr;
	HI_UNF_AI_ATTR_S stAiAttr;
	#if 0
		HI_CHAR InputCmd[32];
		static pthread_t g_AencAiThd;
	#endif

	HI_SYS_Init();
	Ret = HI_UNF_AI_Init();
	if (HI_SUCCESS != Ret)
	{
		g_warning("call Ai Init failed.\n");
		goto SYS_DEINIT;
	}
	Ret = HI_UNF_AI_GetDefaultAttr(HI_UNF_AI_I2S0,&stAiAttr);

	if(HI_SUCCESS != Ret)
	{
		printf("Get Default Ai Attr Failed \n");
		goto AI_DEINIT;
	}

	Ret = HI_UNF_AI_Create(HI_UNF_AI_I2S0, &stAiAttr, &hAi);
	if(HI_SUCCESS != Ret)
	{
		printf("Ai Create Failed \n");
		goto AI_DEINIT;
	}

	Ret = HI_UNF_AENC_Init();
	if (Ret != HI_SUCCESS)
	{
		g_warning("call HI_UNF_AENC_Init failed.\n");
		goto AI_DESTROY;
	}

	HI_UNF_AENC_RegisterEncoder("libHA.AUDIO.AAC.encode.so");
	AAC_ENC_CONFIG stPrivateConfig;
	HA_AAC_GetDefaultConfig(&stPrivateConfig);
	HA_AAC_GetEncDefaultOpenParam(&(stAencAttr.sOpenParam), &stPrivateConfig);

	Ret = HI_UNF_AENC_Create(&stAencAttr, &hAenc);
	if (Ret != HI_SUCCESS)
	{
		g_warning("call HI_UNF_AENC_Create failed.\n");
		goto AENC_DEINIT;
	}

	Ret = HI_UNF_AENC_AttachInput(hAenc, hAi);
	if(HI_SUCCESS != Ret)
	{
		printf("Ai Enable Failed \n");
		goto AENC_DESTROY;
	}

	Ret = HI_UNF_AI_SetEnable(hAi, HI_TRUE);
	if(HI_SUCCESS != Ret)
	{
		printf("Ai Enable Failed \n");
		goto AENC_DETACH;
	}

AENC_DETACH:
	HI_UNF_AENC_DetachInput(hAenc);
AENC_DESTROY:
	HI_UNF_AENC_Destroy(hAenc);
AENC_DEINIT:
	HI_UNF_AENC_DeInit();
AI_DESTROY:
	HI_UNF_AI_Destroy(hAi);
AI_DEINIT:
	HI_UNF_AI_DeInit();
SYS_DEINIT:
	HI_SYS_DeInit();

	return HI_SUCCESS;
}

#if defined(USE_DEV_HID_RAW)
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

static void nf_HI_aud_codec_cfg(void)
{
	gchar *str="hidraw";

	FILE *fp;
	DIR *dir;
	struct dirent *entry;
	struct stat fstat;

	gint pid=0;
	gchar cmdLine[256]={0, }, tempPath[256]={0, };

	dir = opendir("/dev");

g_message("%s line%d", __FUNCTION__, __LINE__);
	while ((entry = readdir(dir)) != NULL) {
		snprintf(tempPath, sizeof(tempPath), "/dev/%s", entry->d_name);

	  if (S_ISDIR(fstat.st_mode))
			 continue;

		if(strstr(tempPath, str) != NULL)
		{
			int fd=0;
			gint desc_size=0;
			guint bustype=0;
			gshort vendor=0, product=0;
			gchar plocation[256]={0, };
			gchar buf_feature[256]={0, };
			gchar buf_report[256]={0, };

			fd = Open(tempPath, O_RDWR, 0);
//			fd = Open(tempPath, O_RDWR|O_NONBLOCK, 0);
			if(fd < 0){
				g_warning("%s line%d %s open error... fd[%d]", __FUNCTION__, __LINE__, tempPath, fd);
			}else{
				gchar name[128]={0,};

				g_message("%s line%d %s open success!!!    fd[%d]", __FUNCTION__, __LINE__, tempPath, fd);
				nf_dev_open_hid_raw(0, fd);

				nf_dev_hid_get_report_discriptor_size(0, &desc_size);
				nf_dev_hid_get_report_discriptor(0, desc_size);				
				nf_dev_hid_get_raw_info(0, &bustype, &vendor, &product);
				nf_dev_hid_get_physical_location(0, plocation);
				nf_dev_hid_get_raw_name(0, name);
			#if 1
				buf_feature[0]=0x22;
				buf_feature[1]=0xff;
				buf_feature[2]=0xff;
				buf_feature[3]=0xff;
				nf_dev_hid_set_feature(0, buf_feature);
				buf_feature[0]=0x22;
				nf_dev_hid_get_feature(0, buf_feature);
//				nf_dev_hid_get_report(0, buf_report);
			#endif
#if 0
				g_message("%s line%d", __FUNCTION__, __LINE__);
				nf_dev_hid_send_report(0, 0);
				g_message("%s line%d", __FUNCTION__, __LINE__);
				nf_dev_hid_get_report(0, buf_report);
				g_message("%s line%d", __FUNCTION__, __LINE__);
#endif
			}
		}
	}
}
#endif

static void nf_HI_aud_get_attr(void)
{
}

static void nf_HI_aud_get_attr_hdmi(void)
{
}

HI_BOOL nf_HI_aud_start(GValue *data)
{
	NF_HI_AUD_QDATA *qdata = NULL;
	NF_HI_AUD_PARAM *param = NULL;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		NF_HI_AUD_QDATA *qdata_net = NULL;
		NF_HI_AUD_PARAM *param_net = NULL;
	#endif
	NF_HI_AUD_RD *pstAudRd = NULL;
	NF_REC_AUDIO_PARAM *pstParam = NULL;

	HI_S32 i;
	gint num_audio=0;

	pstParam=(NF_REC_AUDIO_PARAM *)data;
	g_return_val_if_fail ( NULL != pstParam, HI_FALSE );

	HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] START(From record manager)", __FUNCTION__);

	/* Create audio info */
	qdata = nf_HI_aud_CreateQdata( sizeof(NF_HI_AUD_PARAM) * NUM_ACTIVE_CH );
	g_assert( NULL != qdata );

	param = (NF_HI_AUD_PARAM *) ( qdata->pData );
	g_assert( NULL != param );

	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		qdata_net = nf_HI_aud_CreateQdata( sizeof(NF_HI_AUD_PARAM) * NUM_ACTIVE_CH );
		g_assert( NULL != qdata_net );

		param_net = (NF_HI_AUD_PARAM *) ( qdata_net->pData );
		g_assert( NULL != param_net );
	#endif

	/* Check audio param */
	pstAudRd = nf_HI_aud_getRd();
	pstAudRd->send_type = pstParam->send_type;

	if(pstAudRd->send_type == NF_AUDIO_SEND_DVR)
		num_audio=NUM_AUDIO;
	else
		num_audio=NUM_ACTIVE_CH;

	for ( i = 0; i < NUM_ACTIVE_CH; i++)
	{
		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s][%d] CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", __FUNCTION__, __LINE__,
												pstParam->ch_arr[i],
												pstParam->rec_reason[i],
												pstParam->pre_rec_time[i],
												pstParam->pre_rec_close[i] );

		if ( ( pstParam->ch_arr[i] >= num_audio) &&
			 ( pstParam->ch_arr[i] != 0xFF ) )
		{
			g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
			return HI_FALSE;
		}
		else
		{
			param[i].u8Chn = pstParam->ch_arr[i];
			#if defined(ENABLE_REC_NET_IMMEDIATELY)
				param_net[i].u8Chn = pstParam->ch_arr[i];
			#endif
		}

		if ( pstParam->pre_rec_time[i] > MAX_PRE_REC_TIME )
		{
			g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
			return HI_FALSE;
		}
		else
		{
			param[i].u8PreRecTime = pstParam->pre_rec_time[i];
			#if defined(ENABLE_REC_NET_IMMEDIATELY)
				param_net[i].u8PreRecTime = pstParam->pre_rec_time[i];
			#endif
		}

		if ( pstParam->rec_reason[i] > NF_RECORD_REASON_PRE )
		{
			g_warning("[%s][%d] Invaild parameter.", __FUNCTION__, __LINE__);
			return HI_FALSE;
		}
		else
		{
			param[i].u8Reason = pstParam->rec_reason[i];
			#if defined(ENABLE_REC_NET_IMMEDIATELY)
				param_net[i].u8Reason = pstParam->rec_reason[i];
			#endif
		}

		param[i].u8PreRecClose = pstParam->pre_rec_close[i];
		#if defined(ENABLE_REC_NET_IMMEDIATELY)
			param_net[i].u8PreRecClose = pstParam->pre_rec_close[i];
		#endif

		#if defined(ENABLE_REC_NET_IMMEDIATELY)
			HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s][%d] CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] "
										"CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", __FUNCTION__, __LINE__,
									param[i].u8Chn, param[i].u8Reason, param[i].u8PreRecTime, param[i].u8PreRecClose,
									param_net[i].u8Chn, param_net[i].u8Reason, param_net[i].u8PreRecTime, param_net[i].u8PreRecClose );
		#else
			HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s][%d] CH[%02d] Reason[%d] Pre_Time[%d] Pre_Close[%d] ", __FUNCTION__, __LINE__,
											param[i].u8Chn,
											param[i].u8Reason,
											param[i].u8PreRecTime,
											param[i].u8PreRecClose );
		#endif
	}

	/* Audio enable */
	nf_HI_aud_setRdEnable( HI_TRUE );

	qdata->s32Cmd = HI_CMD_AUD_CFG;
	/* Send config info to audio queue */
	if ( nf_HI_aud_sendQdata( qdata, nf_HI_aud_getRecQueue() ) == HI_FALSE )
	{
		g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
		return HI_FALSE;
	}

	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		qdata_net->s32Cmd = HI_CMD_AUD_CFG;

		/* Send config info to audio queue */
		if ( nf_HI_aud_sendQdata( qdata_net, nf_HI_aud_getNetQueue() ) == HI_FALSE )
		{
			g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
			return HI_FALSE;
		}
	#endif

	return HI_TRUE;
}

static void nf_HI_aud_threadRec(NfHiaud *self)
{
	NF_HI_AUD_REC *ptr_rec=NULL;
	gpointer pQueData=NULL;
	HI_S32 i, ret, cnt_err=0;

	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while ( NULL == nf_HI_aud_getRec() )
		g_usleep(10*1000);

	ptr_rec=&self->stAudRec;

	ptr_rec->init_done = 1;

	/* Start to get streams of each channel. */

	while ( self->thread_run_rec )
	{
		NF_HI_AUD_QDATA *pstQdata = NULL;

		pQueData = g_async_queue_pop( ptr_rec->queue );
		g_assert( NULL != pQueData );

		pstQdata = (NF_HI_AUD_QDATA *) pQueData;
		g_assert( NULL != pstQdata );

		switch ( pstQdata->s32Cmd )
		{
			case HI_CMD_AUD_REC:
			{
				HI_DBG_AUD(DBG_MSG_AUD_REC, "[%s] CMD[REC] CH[%d] LEN[%d] S_PTS[%llu] E_PTS[%llu] Diff[%llu]", __FUNCTION__,
															pstQdata->s32Chn,
															pstQdata->s32Len,
															pstQdata->u64Start,
															pstQdata->u64End,
															pstQdata->u64End - pstQdata->u64Start );
				/* Handoff */
				if( ( ptr_rec->handoffFunc != NULL ) &&
					( ptr_rec->u32HandoffChnMask & (HI_U32)( 1 << pstQdata->s32Chn ) ) )
				{
					if ( nf_HI_aud_handoff( ptr_rec, pstQdata ) == HI_FALSE )
					{
						#if 0
							g_warning("[%s][%d] hi_aud get gst buffer fail.", __FUNCTION__, __LINE__);
							goto exitRec;
						#else
							goto aud_rec_skip;
						#endif
					}
				}

				/* Put frame sst */
				for( i = 0; i < NUM_ACTIVE_CH; i++ )
				{
					if( ( ptr_rec->stParam[i].u8Chn == pstQdata->s32Chn ) &&
						( ptr_rec->stParam[i].u8PreRecTime > 0 ||
						ptr_rec->stParam[i].u8Reason != NF_RECORD_REASON_NOTHING ) )
					{
						if ( nf_HI_aud_putSST( i, pstQdata, &(ptr_rec->stParam[i]) ) == HI_FALSE )
						{
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
						g_warning("[%s][%d] hi_aud get gst buffer fail.", __FUNCTION__, __LINE__);
				break;
			}
			break;

			case HI_CMD_AUD_REC_GST_BUFFER:
			{
				GstBuffer *buffer = (GstBuffer *)pstQdata->pData;
				ICODEC_HEADER *pheader_single = (ICODEC_HEADER *)GST_BUFFER_DATA(buffer);
				guint chan = pheader_single->chan;

				#ifdef DEBUG_REC_AUDIO_LOG
					if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_HEX] )
						nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
				#endif

				if( ( ptr_rec->handoffFunc != NULL ) &&
					( ptr_rec->u32HandoffChnMask & (HI_U32)( 1 << pstQdata->s32Chn ) ) )
					nf_HI_aud_handoff_gst_buffer(ptr_rec, pstQdata);

				if( ptr_rec->stParam[chan].u8Chn != 0xff
						&& ( ptr_rec->stParam[chan].u8PreRecTime >0
							|| ptr_rec->stParam[chan].u8Reason != NF_RECORD_REASON_NOTHING ) )
					_nf_HI_aud_putSST_put_gst_buffer( chan, buffer, &(ptr_rec->stParam[chan]));
			}
			break;

			case HI_CMD_AUD_CFG:
			{
				NF_HI_AUD_PARAM *paramNew = (NF_HI_AUD_PARAM *) (pstQdata->pData);
				g_assert( NULL != paramNew );

				HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] CMD[CFG]", __FUNCTION__);

				if ( nf_HI_aud_ctrlSST( ptr_rec->stParam, paramNew ) == HI_FALSE )
				{
					g_warning("[%s][%d] Audio configuration failed.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
			break;

			default:
				g_warning("[%s][%d] Invaild cmd.", __FUNCTION__, __LINE__);
				goto exitRec;

		} /* switch */

		if(pstQdata->s32Cmd == HI_CMD_AUD_REC_GST_BUFFER)
		{
			if(_nf_HI_aud_freeQdata_gst_buffer( pstQdata ) == HI_FALSE)
			{
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitRec;
			}
		}
		else
		{
			if ( nf_HI_aud_freeQdata( pstQdata ) == HI_FALSE )
			{
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitRec;
			}
		}

		cnt_err++;

	} /* while */

exitRec:
	self->thread_run_rec = FALSE;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

//#define NF_HI_AUDIO_STREAM_WRITE_TO_FILE
#if defined(NF_HI_AUDIO_STREAM_WRITE_TO_FILE)
	gchar *str_aud[] =
	{
		"/audio_test_ch0.raw",
		"/audio_test_ch1.raw",
		"/audio_test_ch2.raw",
		"/audio_test_ch3.raw",
		"/audio_test_ch4.raw",
		"/audio_test_ch5.raw",
		"/audio_test_ch6.raw",
		"/audio_test_ch7.raw",
		"/audio_test_ch8.raw",
		"/audio_test_ch9.raw",
		"/audio_test_ch10.raw",
		"/audio_test_ch11.raw",
		"/audio_test_ch12.raw",
		"/audio_test_ch13.raw",
		"/audio_test_ch14.raw",
		"/audio_test_ch15.raw"
	};
#endif
static void nf_HI_aud_threadRd(NfHiaud *self)
{
	HI_PCHAR p[NUM_AUDIO] = { NULL, };
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		gchar *p_net[NUM_AUDIO] = { NULL, };
	#endif
	HI_S32 i, j, ch=0;
	HI_S32 ret;
	fd_set read_fds;
	GTimeVal timeout;
	#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
		gchar *stream=NULL;
	#endif
	#if defined(ENABLE_REC_LIVE_AUDIO_HI)
#if 0
		AUDIO_STREAM_S stStream_lv0;
		AUDIO_STREAM_S stStream_lv1;
#endif
		guchar *stream_live0=NULL;
		guchar *stream_live1=NULL;
	#endif
#if 0
	AUDIO_STREAM_S stStream;
#endif

	gint policy;
	struct sched_param sched;
	pthread_t thread;

	NF_HI_AUD_RD *prd=&self->stAudRd;

	#if defined(NF_HI_AUDIO_STREAM_WRITE_TO_FILE)
		FILE *fp_aud[NUM_AUDIO];

		for(i=0; i<NUM_AUDIO; i++)
		{
			if((fp_aud[i] = fopen(str_aud[i], "w")) == NULL)
			{
				g_warning("%s File Open Error!! name -> %s", __FUNCTION__, str_aud[i]);
				return HI_FALSE;
			}
			else
				g_message("%s Line[%d] File Open Success!! name -> %s", __FUNCTION__, __LINE__, str_aud[i]);
		}
	#endif

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while ( NULL == nf_HI_aud_getRd() )
		g_usleep(10*1000);

	prd->init_done = 1;

	#if defined(ENABLE_RESET_AUDIO_AI_HI)
		g_message("Configured HI Audio AI Reset Time %dsec", HI_AUD_RESET_TIME_AI);
	#endif

	/* Start to get streams of each channel. */
	while ( self->thread_run_rd )
	{
		g_usleep(10*1000);
	} /* while */

exitRd:
	#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
		g_free( stream );
	#endif
	self->thread_run_rd = 0;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;

}

#if defined(ENABLE_REC_NET_IMMEDIATELY)
static void nf_HI_aud_threadNet(NfHiaud *self)
{
	gpointer pQueData = NULL;
	HI_S32 i, ret;

	NF_HI_AUD_REC *pnet=NULL;
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	/* Create audio info */

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	pnet=&self->stAudNet;
	#if defined(NF_HI_AUDIO_NET_STREAM_WRITE_TO_FILE)
		FILE *fp_ch0=NULL;

		if((fp_ch0 = fopen(FILENAME_AUDIO_NET_STREAM_CH0, "w")) == NULL)
		{
			g_warning("%s File Open Error!!", __FUNCTION__);
			return ;
		}
		else
			g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
	#endif

	while ( NULL == nf_HI_aud_getNet() )
		g_usleep(10*1000);

	pnet->init_done = 1;

	/* Start to get streams of each channel. */
	while ( self->thread_run_net )
	{
		g_usleep(10*1000);
	}

    return;
}
#endif

static void nf_HI_aud_threadNet_pb(NfHiaud *self)
{
	while(self->thread_run_net_pb)
	{
		g_usleep(10*1000);
	}

	g_message("%s End!!", __FUNCTION__);
}

static void nf_HI_aud_thread_Output_pb(NfHiaud *self)
{
	while(self->thread_run_output_pb)
	{
		g_usleep(10*1000);
	}

	g_message("%s End!!", __FUNCTION__);
}

#define NF_HI_AUDIO_PATCH_AOUT_LIVE_SYNC
static gboolean
_nf_HI_aud_live_put_frame(gint curr_ch , NF_HI_AUD_RD *self, gpointer data)
{
#if 0
	AUDIO_STREAM_S *stream=NULL;
#endif
	static guint prev_ch=0;
	#if defined(NF_HI_AUDIO_PATCH_AOUT_LIVE_SYNC)
		GTimeVal curr_timeval;
		static gulong reset_time_ao=0;
	#endif
	gint live_ch0=0, live_ch1=0;
	static gboolean is_ao_reset=FALSE;
#if 0
	stream=(AUDIO_STREAM_S *)data;
	g_return_val_if_fail( stream != NULL, FALSE);
#endif
	g_return_val_if_fail( ((curr_ch < NUM_AUDIO)), FALSE);

	live_ch0=self->live_ch0;
	#if defined(ENABLE_AUD_STEREO_OUT)
		live_ch1=self->live_ch1;
	#endif

	#if 0
		g_message("%s line%d live_ch0 %d live_ch1 %d curr_ch %d  len %d", 
				__FUNCTION__, __LINE__, live_ch0, live_ch1, curr_ch, stream->u32Len);
	#endif

	if(nf_network_get_webra_audio_status())
		return FALSE;

	if(live_ch0 == NF_REC_AUDIO_DAC_PLAYBACK)
	{
		prev_ch=NF_REC_AUDIO_DAC_PLAYBACK;
		return FALSE;
	}
	else
		is_ao_reset=FALSE;      // For playback

	if(prev_ch == NF_REC_AUDIO_DAC_PLAYBACK)
	{
		g_message("%s line%d AO Reset Playback -> Live Change", __FUNCTION__, __LINE__);
	}

	prev_ch=live_ch0;

	return TRUE;
}

// For DVR PB
// IPCAM -> DVR Path
int playback_audio_cb(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type)
{
	#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
		gshort  pcm_val;
	#else
		guchar pcm_val;
	#endif
	guchar  u_val;
	guchar *u8AudioStream;
	short s16AudioStream[HI_AUDIO_PTNUMPERFRM];
	static guchar u8AudioRemainStream[HI_AUDIO_PTNUMPERFRM];
	static int remain_size = 0;
	static gint pre_stream_size = 0;
	int i,j;
	int SendAllSize = 0;
	char* temp_ptr;
	NF_HI_AUD_OUTPUT_PB *QData = NULL;
	short *queue_data  = NULL;

	if(_nf_HI_aud->stAudRd.send_type == NF_AUDIO_SEND_IPCAM)
	{
		if(nf_network_get_webra_audio_status())
			return stream_size;
	}

	j = 0;
	if( pre_stream_size != stream_size )
	{
		remain_size = 0;
		pre_stream_size = stream_size;
	}

	u8AudioStream   = (guchar  *)malloc(stream_size + remain_size);

	if( remain_size )
		memcpy( u8AudioStream, u8AudioRemainStream, remain_size );
	memcpy( u8AudioStream + remain_size, stream_buf, stream_size );

	remain_size += stream_size;
	while(remain_size > 0)
	{
		if( remain_size < HI_AUDIO_PTNUMPERFRM )
		{
			memcpy( u8AudioRemainStream, u8AudioStream+SendAllSize, remain_size );
			break;
		}

		temp_ptr = u8AudioStream + (j*HI_AUDIO_PTNUMPERFRM);

		for(i=0; i <HI_AUDIO_PTNUMPERFRM; i++)
		{
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
			pcm_val = nf_HI_muraw_to_lpcm16(u_val);
			#else
				pcm_val=u_val;
			#endif
			s16AudioStream[i] = pcm_val;
		}

		remain_size-=HI_AUDIO_PTNUMPERFRM;
		SendAllSize+=HI_AUDIO_PTNUMPERFRM;

		QData       = (NF_HI_AUD_OUTPUT_PB *)malloc(sizeof(NF_HI_AUD_OUTPUT_PB));
		queue_data  = (short *)malloc(HI_AUDIO_SIZE_STREAM);
		memcpy(queue_data, s16AudioStream, HI_AUDIO_SIZE_STREAM );
		QData->data = (void *)queue_data;
		QData->len  = HI_AUDIO_SIZE_STREAM;
		g_async_queue_push( _nf_HI_aud->stAudPb.queue_output_pb, QData);

		j++;
	}
	free(u8AudioStream);

	return stream_size;
}

// For Network PB
int playback_audio_cb_net(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, int hi_aud_stm_cnt)
{
	NF_HI_AUD_NET_PB *data_net_pb;
	gint qlen=0;
	#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
		short   pcm_val;
	#else
		guchar pcm_val;
	#endif
	guchar  u_val;
	gshort  *AudioStream=NULL;
	gint i=0, j=0;
	gchar *temp_ptr=NULL;
	NF_HI_AUD_NET_PB *que_poped_data = NULL;

	data_net_pb = (NF_HI_AUD_NET_PB *)g_malloc0( sizeof(NF_HI_AUD_NET_PB) );

	AudioStream = (gshort *) g_malloc0(HI_AUDIO_DATA_CONVERT_SIZE);
	if(AudioStream == NULL)
		g_message("%s alloc fail!!", __FUNCTION__);

	// Converting
	for(j=0; j<hi_aud_stm_cnt; j++)
	{
		gint index=0;

		temp_ptr = stream_buf + (j * HI_AUDIO_DATA_WEB_SIZE);

		for(i=0; i <HI_AUDIO_DATA_WEB_SIZE; i++)
		{
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
				pcm_val = nf_HI_muraw_to_lpcm16(u_val);
			#else
				pcm_val = u_val;
			#endif

			index=(i + (j * HI_AUDIO_DATA_WEB_SIZE));
			*(AudioStream + index) = pcm_val;
		}
	}

	memcpy(data_net_pb->data, AudioStream, HI_AUDIO_DATA_CONVERT_SIZE);
	data_net_pb->cnt=HI_AUDIO_DATA_CONVERT_SIZE;

	free(AudioStream);

	qlen=g_async_queue_length(_nf_HI_aud->stAudPb.queue_net_pb);
	if(qlen > HI_AUDIO_NET_PB_MAX_QUEUE)
	{
		g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
		g_free(data_net_pb);
		return FALSE;
	}

	g_async_queue_push( _nf_HI_aud->stAudPb.queue_net_pb, data_net_pb);

	return stream_size;
}


HI_BOOL nf_HI_aud_putSST( HI_S32 s32Chn, NF_HI_AUD_QDATA *pstQdata, NF_HI_AUD_PARAM* pstParam )
{
	GstNfBuddyBuffer *gstBuf = NULL;
	ICODEC_HEADER *ih = NULL;

	HI_S32 gstSize;
	HI_U64 preTimestamp, diffTimestamp;
	HI_BOOL bTime;

	g_assert( NULL != pstQdata );
	g_assert( NULL != pstParam );

	preTimestamp = pstParam->u64Timestamp;

	HI_DBG_AUD(DBG_MSG_AUD_REC, "[%s] CH[%02d] Pre[%llu] Now[%llu]", __FUNCTION__, s32Chn, pstParam->u64Timestamp, pstQdata->u64Start);

	/* Update timestamp */
	if ( preTimestamp == 0 )
	{
		preTimestamp = pstQdata->u64Start;
	}
	else
	{
		preTimestamp += HI_AUD_TIME_UNIT;
	}

	if ( pstQdata->u64Start >= preTimestamp )
	{
		diffTimestamp = pstQdata->u64Start - preTimestamp;
		bTime = HI_TRUE;
	}
	else
	{
		diffTimestamp = preTimestamp - pstQdata->u64Start;
		bTime = HI_FALSE;
	}

	HI_DBG_AUD(DBG_MSG_AUD_REC, "[%s] Diff[%llu] Flag[%d]", __FUNCTION__, diffTimestamp, bTime);

	if ( diffTimestamp > 200000 )
	{
		HI_DBG_AUD(DBG_MSG_AUD, "[%s] Frm[%llu] Cur[%llu] Diff[%llu] [%d]", __FUNCTION__,
											pstQdata->u64Start,
											pstParam->u64Timestamp,
											diffTimestamp,
											bTime );

		pstParam->u64Timestamp = 0;

		if ( bTime == HI_FALSE )
		{
			/* Skip */
			g_warning( "[%s] Audio frame skip.", __FUNCTION__ );

			return HI_TRUE;
		}
	}

	/* Align address of buffer */
	gstSize = ALIGN( HI_S32, ( sizeof( ICODEC_HEADER ) + pstQdata->s32Len ) , 32 );

	/* Alloc stream buffer */
	gstBuf = nf_HI_gst_buffer( gstSize, FALSE );
	if(gstBuf == NULL)
		return HI_FALSE;

	GST_BUFFER_SIZE( gstBuf ) = (guint)gstSize;

	/* Copy stream buffer. */
	memcpy( GST_BUFFER_DATA( gstBuf ) + sizeof( ICODEC_HEADER ),
			pstQdata->pData,
			(size_t)pstQdata->s32Len );

	/* Set codec header. */
	ih = ( ICODEC_HEADER * )( GST_BUFFER_DATA( gstBuf ) );
	g_assert( NULL != ih );

	ih->chan = (guchar)s32Chn;
	ih->flags = pstParam->u8Reason;

	ih->codec = NF_CODEC_TYPE_URAW;
	ih->version = NF_CODEC_VERSION_1;
	ih->frame_size = (guint)pstQdata->s32Len;
	ih->frame_type = NF_FRAME_TYPE_AUDIO;
	ih->resolution = 0;
	ih->frame_rate = NF_FPS_CR01;

	/* Set timestamp */
	if( pstParam->u64Timestamp == 0 )
	{
		pstParam->u64Timestamp = pstQdata->u64Start;
	}
	else
	{
		pstParam->u64Timestamp += HI_AUD_TIME_UNIT;
	}

	ih->timestamp = (guint) (pstParam->u64Timestamp / 1000000);
	ih->timestampl = (guchar) ((pstParam->u64Timestamp % 1000000 ) / 5000 );

	//dump_icodec_header("nf_HI_aud_putSST", ih);

	if ( pstParam->s32StmId >= 0 )
	{
		if( sst_record_put_frame( pstParam->s32StmId, (struct icodec_header_t *)ih ) )
		{
			g_warning("[%s] SST error!", __FUNCTION__);

			gst_buffer_unref( (GstBuffer *)gstBuf );
		}
	}
	else
	{
		gst_buffer_unref( (GstBuffer *)gstBuf );
	}

	return HI_TRUE;
}

HI_BOOL nf_HI_aud_ctrlSST( NF_HI_AUD_PARAM *pstOld,  NF_HI_AUD_PARAM *pstNew )
{
	HI_S32 i, ret;

	g_return_val_if_fail (pstOld != NULL, HI_FALSE);
	g_return_val_if_fail (pstNew != NULL, HI_FALSE);

	for( i = 0; i < NUM_ACTIVE_CH; i++ )
	{
		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] Old CH[%02d] Reason[%d] PreTime[%d] PreClose[%d] STM[%d]", __FUNCTION__,
												pstOld[i].u8Chn,
												pstOld[i].u8Reason,
												pstOld[i].u8PreRecTime,
												pstOld[i].u8PreRecClose,
												pstOld[i].s32StmId );

		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] New CH[%02d] Reason[%d] PreTime[%d] PreClose[%d]", __FUNCTION__,
												pstOld[i].u8Chn,
												pstOld[i].u8Reason,
												pstOld[i].u8PreRecTime,
												pstOld[i].u8PreRecClose );

		/* Open -> Close */
		if( ( pstOld[i].s32StmId >= 0 ) &&
			( ( pstNew[i].u8Chn == 0xFF ) ||
			  ( pstNew[i].u8Reason == NF_RECORD_REASON_NOTHING ) ||
			  ( pstOld[i].u8Reason != pstNew[i].u8Reason ) ||
			  ( pstOld[i].u8PreRecTime != pstNew[i].u8PreRecTime ) ) )
		{
			HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] sst_record_close CH[%2d] SID[%2d] pre_flush[%d]", __FUNCTION__,
												i + NF_HI_AUDIO_SST_START_CH_NO,
												pstOld[i].s32StmId,
												pstNew[i].u8PreRecTime );

			ret = sst_record_close( pstOld[i].s32StmId,
									(HI_U8)( pstNew[i].u8PreRecClose ) );

			g_assert( 0 == ret );

			/* Set param */
			pstOld[i].s32StmId = -1;
			pstOld[i].u64Timestamp = 0;
		}

		/* Close -> Open */
		if( ( pstNew[i].u8Chn != 0xFF ) &&
			( pstOld[i].s32StmId < 0 ) &&
			( ( pstNew[i].u8PreRecTime > 0 ) ||
			  ( pstNew[i].u8Reason != NF_RECORD_REASON_NOTHING ) ) )
		{
			HI_S32 stmID = -1;

			/* If open the stream, fail */
			g_assert( pstOld[i].s32StmId < 0 );

			stmID = sst_record_open( (guchar)(i + NF_HI_AUDIO_SST_START_CH_NO),    // ch
									 pstNew[i].u8PreRecTime,                    // pre_rec_time
									 pstNew[i].u8Reason,                        // rec_reason
									 NF_CODEC_TYPE_URAW,                        // codec
									 0,                                         // resolution
									 NF_FPS_CR01,                               // frame rate
									 0 );                                       // quality

			HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] sst_record_open  CH[%2d] SID[%2d] Reason[%d] PreTime[%d]", __FUNCTION__,
																i + NF_HI_AUDIO_SST_START_CH_NO,
																stmID,
																pstNew[i].u8Reason,
																pstNew[i].u8PreRecTime );

			if( stmID < 0 )
			{
				g_message( "[%s] SST_record_open result[%d](%s) ###", __FUNCTION__,
													stmID,
													sst_get_error_string( stmID ) );
			}

			/* Fail open the SST */
			g_assert( stmID >= 0 || stmID == -SST_ERR_DISKFULL );

			pstOld[i].s32StmId = stmID;
		}

		pstOld[i].u8Chn         = pstNew[i].u8Chn;
		pstOld[i].u8Reason      = pstNew[i].u8Reason;
		pstOld[i].u8PreRecTime  = pstNew[i].u8PreRecTime;
		pstOld[i].u8PreRecClose = pstNew[i].u8PreRecClose;

	} /* for */

	return HI_TRUE;
}

HI_BOOL nf_HI_aud_ctrNet( NF_HI_AUD_PARAM *pstOld,  NF_HI_AUD_PARAM *pstNew )
{
	HI_S32 i, ret;

	g_return_val_if_fail (pstOld != NULL, HI_FALSE);
	g_return_val_if_fail (pstNew != NULL, HI_FALSE);

	for( i = 0; i < NUM_ACTIVE_CH; i++ )
	{
		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] Old CH[%02d] Reason[%d] PreTime[%d] PreClose[%d] STM[%d]", __FUNCTION__,
												pstOld[i].u8Chn,
												pstOld[i].u8Reason,
												pstOld[i].u8PreRecTime,
												pstOld[i].u8PreRecClose,
												pstOld[i].s32StmId );

		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] New CH[%02d] Reason[%d] PreTime[%d] PreClose[%d]", __FUNCTION__,
												pstOld[i].u8Chn,
												pstOld[i].u8Reason,
												pstOld[i].u8PreRecTime,
												pstOld[i].u8PreRecClose );

		pstOld[i].u8Chn         = pstNew[i].u8Chn;
		pstOld[i].u8Reason      = pstNew[i].u8Reason;
		pstOld[i].u8PreRecTime  = pstNew[i].u8PreRecTime;
		pstOld[i].u8PreRecClose = pstNew[i].u8PreRecClose;

	}

	return HI_TRUE;
}

NF_HI_AUD_RD *nf_HI_aud_getRd(void)
{
	return &_nf_HI_aud->stAudRd;
}

HI_BOOL nf_HI_aud_isRdEnable(void)
{
	return _nf_HI_aud->stAudRd.isEnable;
}

void nf_HI_aud_setRdEnable(HI_BOOL bValue)
{
	_nf_HI_aud->stAudRd.isEnable = bValue;
}

NF_HI_AUD_REC *nf_HI_aud_getRec(void)
{
	return &_nf_HI_aud->stAudRec;
}

GAsyncQueue *nf_HI_aud_getRecQueue(void)
{
	return _nf_HI_aud->stAudRec.queue;
}

NF_HI_AUD_PARAM *nf_HI_aud_getRecParam(void)
{
	return _nf_HI_aud->stAudRec.stParam;
}

#if defined(ENABLE_REC_NET_IMMEDIATELY)
NF_HI_AUD_REC *nf_HI_aud_getNet(void)
{
	return &_nf_HI_aud->stAudNet;
}

GAsyncQueue *nf_HI_aud_getNetQueue(void)
{
	return _nf_HI_aud->stAudNet.queue;
}

NF_HI_AUD_PARAM_S *nf_HI_aud_getNetParam(void)
{
	return _nf_HI_aud->stAudNet.stParam;
}

void nf_HI_aud_setNetHandoff( HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc )
{
	g_assert ( NULL != handoffFunc );

	_nf_HI_aud->stAudNet.u32HandoffChnMask = u32ChnMask;
	_nf_HI_aud->stAudNet.handoffFunc = handoffFunc;
}
#endif

void nf_HI_aud_setRecHandoff( HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc )
{
	g_assert ( NULL != handoffFunc );

	_nf_HI_aud->stAudRec.u32HandoffChnMask = u32ChnMask;
	_nf_HI_aud->stAudRec.handoffFunc = handoffFunc;
}

/**
	Register Handofffunc
**/
HI_BOOL nf_HI_aud_registerHandoff(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc)
{
	if (nf_sysman_qcmode_is_enable()) {
		return HI_TRUE;
	}

	g_return_val_if_fail((_nf_HI_aud->stAudRec.init_done ==TRUE), FALSE);

	g_assert ( NULL != handoffFunc );

	HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] chnMask[0x%x] handoffFx[%p]", __FUNCTION__, u32ChnMask, handoffFunc);
	#if 0       // For Dealock!!
		NF_OBJECT_LOCK( nf_HI_aud_getRec() );
	#endif

	nf_HI_aud_setRecHandoff( u32ChnMask, handoffFunc );

	#if 0       // For Dealock!!
		NF_OBJECT_UNLOCK( nf_HI_aud_getRec() );
	#endif

	return HI_TRUE;
}

#if defined(ENABLE_REC_NET_IMMEDIATELY)
HI_BOOL nf_HI_aud_registerHandoff_immediately(HI_U32 u32ChnMask, NF_HI_AUD_HANDOFF_FUNC handoffFunc)
{
	if (nf_sysman_qcmode_is_enable()) {
		return HI_TRUE;
	}

	g_return_val_if_fail((_nf_HI_aud->stAudNet.init_done == TRUE), FALSE);

	g_assert ( NULL != handoffFunc );

	HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] chnMask[0x%x] handoffFx[%p]", __FUNCTION__, u32ChnMask, handoffFunc);

	#if 0       // For Dealock!!
		NF_OBJECT_LOCK( nf_HI_aud_getNet() );
	#endif

	nf_HI_aud_setNetHandoff( u32ChnMask, handoffFunc );

	#if 0       // For Dealock!!
		NF_OBJECT_UNLOCK( nf_HI_aud_getNet() );
	#endif

	return HI_TRUE;
}
#endif

static void
_nf_HI_aud_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NfHiaud *ptr=NULL;

	g_return_if_fail(pinfo != NULL);

	ptr=(NfHiaud *)data;
	ptr->vin_mask = ~(pinfo->d.params[0]);
}

NF_HI_AUD_QDATA *nf_HI_aud_CreateQdata( HI_U32 s32Len )
{
	NF_HI_AUD_QDATA *qdata = NULL;

	qdata = (NF_HI_AUD_QDATA *) g_malloc0( sizeof(NF_HI_AUD_QDATA) );

	g_assert( NULL != qdata );

	qdata->pData = (HI_VOID *) g_malloc( s32Len );

	g_assert( NULL != qdata->pData );

	return qdata;
}

static gpointer
_nf_HI_aud_CreateQdata_gst_buffer(GstBuffer *buffer)
{
	NF_HI_AUD_QDATA *qdata=NULL;

	g_return_val_if_fail( buffer != NULL, NULL);

	qdata = g_malloc0( sizeof(NF_HI_AUD_QDATA) );
	g_return_val_if_fail( qdata != NULL, NULL);

	void *tmp_gst_ret = NULL;
	tmp_gst_ret = gst_buffer_ref( buffer );
	if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);

	qdata->s32Cmd = HI_CMD_AUD_REC_GST_BUFFER;
	qdata->pData = (HI_VOID *)buffer;
	qdata->s32Len  = 0;

	return qdata;
}

HI_BOOL nf_HI_aud_freeQdata( NF_HI_AUD_QDATA *pstQdata )
{
	g_assert ( NULL != pstQdata );

	g_free( pstQdata->pData );
	g_free( pstQdata );

	return HI_TRUE;
}

static HI_BOOL
_nf_HI_aud_freeQdata_gst_buffer(NF_HI_AUD_QDATA *pstQdata)
{
	g_assert ( NULL != pstQdata );

	gst_buffer_unref( pstQdata->pData );

	g_free( pstQdata );

	return HI_TRUE;
}

HI_BOOL nf_HI_aud_sendQdata( NF_HI_AUD_QDATA *pstQdata, GAsyncQueue *pQue)
{
	g_assert ( NULL != pstQdata );
	g_assert ( NULL != pQue );

	g_async_queue_push( pQue, pstQdata );

	return HI_TRUE;
}

HI_BOOL nf_HI_aud_handoff( NF_HI_AUD_REC *stAudRec, NF_HI_AUD_QDATA *pstQdata )
{
	GstNfBuddyBuffer *gstBuf = NULL;
	ICODEC_HEADER *ih = NULL;
	HI_S32 gstSize = 0;

	g_assert( NULL != stAudRec );
	g_assert( NULL != pstQdata );

	g_assert( NULL != pstQdata->pData );
	g_assert( NUM_AUDIO > pstQdata->s32Chn );
	g_assert( HI_AUD_TOTAL_SIZE == pstQdata->s32Len );

	/* Align address of buffer */
	gstSize = (HI_S32)ALIGN( gint, ( sizeof( ICODEC_HEADER ) + pstQdata->s32Len ) , 32 );

	/* Alloc CMEM */
	gstBuf = nf_HI_gst_buffer( gstSize, FALSE );

	HI_DBG_AUD(DBG_MSG_AUD_REC, "[%s] Create GST[%p] BUF[%p]", __FUNCTION__, gstBuf, GST_BUFFER_DATA( gstBuf ));

	g_assert( NULL != gstBuf );

	GST_BUFFER_SIZE( gstBuf ) = gstSize;

	memcpy( GST_BUFFER_DATA(gstBuf) + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len );

	/* Create ICODEC header */
	ih = ( ICODEC_HEADER * ) GST_BUFFER_DATA( gstBuf );
	g_assert( NULL != ih );

	ih->chan = pstQdata->s32Chn;
	ih->codec = NF_CODEC_TYPE_URAW;
	ih->flags = 0;
	ih->version = NF_CODEC_VERSION_1;
	ih->frame_size = pstQdata->s32Len;
	ih->frame_type = NF_FRAME_TYPE_AUDIO;
	ih->resolution = 0;
	ih->frame_rate = NF_FPS_CR01;
	ih->timestamp   = (guint) ( pstQdata->u64Start/ 1000000 );
	ih->timestampl  = (guchar) ( ( pstQdata->u64Start % 1000000 ) / 5000 );

	NF_OBJECT_LOCK( _nf_HI_aud );

	if(stAudRec->handoffFunc)
	{
		_nf_HI_aud_dump_icodec_header( "nf_HI_aud_handoff", ih );

		stAudRec->handoffFunc( gstBuf );
	}

	NF_OBJECT_UNLOCK( _nf_HI_aud );

	gst_buffer_unref( gstBuf );

	return HI_TRUE;
}

HI_BOOL nf_HI_aud_handoff_gst_buffer( NF_HI_AUD_REC *stAudRec, NF_HI_AUD_QDATA *pstQdata )
{
	GstNfBuddyBuffer    *gst_buf = (GstNfBuddyBuffer *)pstQdata->pData;
	ICODEC_HEADER       *pheader = (ICODEC_HEADER *)GST_BUFFER_DATA( gst_buf );


	NF_OBJECT_LOCK( _nf_HI_aud );

	#if 0
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_HANDOFF] & 1<<ch)
			_nf_HI_aud_dump_icodec_header("handoff", pheader);
	#endif

	if(stAudRec->handoffFunc)
		stAudRec->handoffFunc(gst_buf);

	NF_OBJECT_UNLOCK( _nf_HI_aud );

	return TRUE;
}

static gboolean
_nf_HI_aud_putSST_put_gst_buffer( guint ch, gpointer frame, NF_HI_AUD_PARAM *pstParam)
{
	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	gint                clen, ret, stream_id;
	GstNfBuddyBuffer    *gst_buf = (GstNfBuddyBuffer *)frame;
	ICODEC_HEADER       *pheader = (ICODEC_HEADER *)GST_BUFFER_DATA( gst_buf );

	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );


	{
		GTimeVal ftval = { pheader->timestamp, pheader->timestampl*5*1000};
		GTimeVal ctval = pstParam->timestamp;

		guint64 ftmp, ctmp, diff;
		gchar c;

		if( ctval.tv_sec != 0)
			g_time_val_add(&ctval, 1020000);
		else
			ctval = ftval;

		ftmp = GTIMEVAL_TO_GUINT64(ftval);
		ctmp = GTIMEVAL_TO_GUINT64(ctval);

		if( ftmp >= ctmp){ // ns
			diff = (ftmp - ctmp);
			c='F';
		}else {
			diff = ctmp - ftmp;
			c='C';
		}

		if( diff > 1100000000/5 )
		{
			pstParam->timestamp.tv_sec = 0;
			pstParam->timestamp.tv_usec = 0;

			g_warning("%s ftval[%d.%06d] ctval[%d.%06d] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);

			if( c == 'C' )
				return 0;
		}

		#ifdef DEBUG_REC_AUDIO_LOG
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_DIFF] & (1 << ch) )
				g_message("%s ftval[%d.%06d] ctval[%d.%06d] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);
		#endif
	}

	if( pstParam->timestamp.tv_sec != 0 )
	{
		g_time_val_add(&pstParam->timestamp, 1020000);

		pheader->timestamp  = pstParam->timestamp.tv_sec;
		pheader->timestampl = pstParam->timestamp.tv_usec/1000/5;

	}else{
		pstParam->timestamp.tv_sec = pheader->timestamp;
		pstParam->timestamp.tv_usec = pheader->timestampl*5*1000;
	}

	// header FIXME!!
	pheader->flags = pstParam->u8Reason;
	pheader->frame_rate = NF_FPS_CR01;

	#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_SST] & (1 << ch) )
			_nf_HI_aud_dump_icodec_header("sst_put", pheader);
	#endif

	if( pstParam->s32StmId >= 0)
	{
		{
			void *tmp_gst_ret = NULL;
			tmp_gst_ret = gst_buffer_ref(gst_buf);
			if(tmp_gst_ret == NULL)
				fprintf(stderr, "- minto - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
		}
		ret = sst_record_put_frame( pstParam->s32StmId, GST_BUFFER_DATA(gst_buf) );
		if(ret){
			_nf_HI_aud_dump_icodec_header("ERR frame", pheader);
			g_message("sst_record_put_frame result[%d](%s)\n", ret, sst_get_error_string(ret));
			gst_buffer_unref(gst_buf);
		}
	}

	return TRUE;
}

gboolean nf_HI_aud_put_frame(gint ch_num, gpointer frame )
{
	GstBuffer *buffer = (GstBuffer *)frame;
	ICODEC_HEADER *pheader = GST_BUFFER_DATA(buffer);
	NF_HI_AUD_QDATA *pstQdata=NULL;
	GTimeVal tv;

	g_return_val_if_fail (frame != 0, FALSE);
	g_return_val_if_fail (ch_num >= 0 && ch_num < NUM_ANALOG_CHANNEL, FALSE);

	gettimeofday((struct timeval *)&tv, NULL);

	#ifdef ENABLE_PUT_FRAME_MAX_QUEUE
		while( g_async_queue_length( _nf_HI_aud->stAud.queue ) > (16*5) ) g_usleep(10*1000);
	#endif

	if ( pheader->frame_type != NF_FRAME_TYPE_AUDIO ) {
		_nf_HI_aud_dump_icodec_header("INV_AUDIO", pheader);
		return -1;
	}

	#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI] )
			_nf_HI_aud_dump_icodec_header("AUDIO", pheader);

		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI_HEX] )
			nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
	#endif

	#if 0
		pstQdata = nf_HI_aud_CreateQdata( gst_buffer_get_size(buffer) );
		pstQdata->s32Chn = _tbl_aud_input[ch_num];
		pstQdata->u64Start = tv.tv_usec;
		pstQdata->s32Len = (HI_S32)gst_buffer_get_size(buffer);
		pstQdata->u64End = tv.tv_usec;
		pstQdata->s32Cmd = HI_CMD_AUD_REC;
	#else
		#if 0
			// Time Compensate!!!
			pheader->timestamp  = tv.tv_sec;
			pheader->timestampl = tv.tv_usec/1000/5;
		#endif
		pstQdata=_nf_HI_aud_CreateQdata_gst_buffer(buffer);
	#endif

	g_return_val_if_fail (pstQdata != 0, FALSE);

	nf_HI_aud_sendQdata(pstQdata, nf_HI_aud_getRecQueue());

	gst_buffer_unref(buffer);

	return 1;
}

#if defined(ENABLE_AUD_STEREO_OUT)
void nf_HI_aud_set_live_audio_ch_stereo(guint ch0, guint ch1)
{
	_nf_HI_aud->stAudRd.live_ch0=(gint)ch0;
	_nf_HI_aud->stAudRd.live_ch1=(gint)ch1;
}
#endif

void nf_HI_aud_set_live_audio_ch(guint ch)
{
	#if defined(ENABLE_AUD_STEREO_OUT)
		_nf_HI_aud->stAudRd.live_ch0=(gint)ch;
		_nf_HI_aud->stAudRd.live_ch1=(gint)ch;
	#else
	_nf_HI_aud->stAudRd.live_ch0=(gint)ch;
#endif
}

#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
/**************************************************************************************
	Convert
**************************************************************************************/
#define BIAS			(0x84)          /* Bias for linear code. */
#define CLIP			8159

#define SIGN_BIT		(0x80)          /* Sign bit for a A-law byte. */
#define QUANT_MASK		(0xf)           /* Quantization field mask. */
#define NSEGS			(8)             /* Number of A-law segments. */
#define SEG_SHIFT		(4)             /* Left shift for segment number. */
#define SEG_MASK		(0x70)          /* Segment field mask. */

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};

guchar nf_HI_lpcm16_to_muraw(short  pcm_val)    /* 2's complement (16-bit range) */
{
	short mask=0, seg=0;
	guchar uval=0;

	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		mask = 0x7F;
	} else {
		mask = 0xFF;
	}
		if ( pcm_val > CLIP ) pcm_val = CLIP;       /* clip the magnitude */
	pcm_val += (BIAS >> 2);

	/* Convert the scaled magnitude to segment number. */
	seg = nf_HI_search_seg(pcm_val, seg_uend, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)       /* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else {
		uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}

short nf_HI_search_seg(short val, short *table, short size)
{
	short i=0;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

short nf_HI_muraw_to_lpcm16(guchar u_val)
{
	short t=0;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

void nf_HI_aud_convert(gchar *stream_src, gchar *stream_dest, guint len)
{
	gint i=0, j=0;
	gshort pcm_val=0;

	for(i=0, j=0; i<(gint)len; i++, j+=2)
	{
		memcpy(&pcm_val, stream_src+j, sizeof(gshort));
		*stream_dest=(gchar)nf_HI_lpcm16_to_muraw(pcm_val);

		*stream_dest++;
	}
}
#endif

static GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose)
{
	GstNfBuddyBuffer *p = NULL;
	ICODEC_HEADER *h = NULL;

	g_return_val_if_fail( size < ( 1 << 20 ), NULL );

	#if 0       // For Gst Null Frame Test!!
		if(_is_null)
		{
			g_message("%s line%d Force NULL!!!", __FUNCTION__, __LINE__);
			return NULL;
		}
	#endif

	p = (GstNfBuddyBuffer *) gst_nf_buddy_buffer_new_and_alloc( size, NULL );

	//g_assert( p != NULL );
	if( p == NULL ) return p;

	h = (ICODEC_HEADER *)( GST_BUFFER_DATA( p ) );

	//g_assert( h != NULL );
	if( h == NULL ) return h;

	h->gst_buffer   = p;
	h->reserved     = 0;

	if ( verbose == TRUE )
	{
		guint addr;
		addr = ICMEM_getBufferPhysicalAddress( h );
	}

	p->frame    = h;
	p->cmemq    = 0;
	p->free_cb  = 0;

	return p;
}

static void
_nf_HI_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", str ,
					pheader->chan,
					pheader->flags,
					pheader->frame_type,
					pheader->frame_rate,
					pheader->resolution,
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size  );
}

