#include <sys/ioctl.h>
#include <libsst.h>
#include <gst/nf/gstnfbuddybuffer.h>
#include <gst/nf/gstnflistbuffer.h>

#include "hi37XX/hi_type.h"		// filesys/usr/include/
#include "hi37XX/hi_unf_avplay.h"
#include "hi37XX/hi_unf_sound.h"

#include "nf_codec_header.h"
#include "nf_rec_audio_hi3798c.h"

#include "HA.AUDIO.MP3.decode.h"
#include "HA.AUDIO.MP2.decode.h"
#include "HA.AUDIO.AAC.decode.h"
#include "HA.AUDIO.DRA.decode.h" #include "HA.AUDIO.PCM.decode.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "HA.AUDIO.AMRNB.codec.h"
#include "HA.AUDIO.AMRWB.codec.h"
#include "HA.AUDIO.TRUEHDPASSTHROUGH.decode.h"
#include "HA.AUDIO.DTSHD.decode.h"
#include "HA.AUDIO.AC3PASSTHROUGH.decode.h"
#include "HA.AUDIO.DTSPASSTHROUGH.decode.h"

/**
	Gloval Function
**/
static void nf_hi_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void nf_hi_aud_threadRec(NfHiaud *self);
static void nf_hi_aud_threadRd(NfHiaud *self);
static void nf_hi_aud_threadOut(NfHiaud *self);
#if defined(NF_REC_AUDIO_HISILICON_TEST)
static void nf_hi_aud_thread_test(NfHiaud *self);
#endif
static gboolean nf_hi_aud_hdmi_write(HI_U32 u32Snd, HI_U32 u32SndNum, HI_HANDLE hAvplay, guchar *data, guint size);

static void nf_hi_aud_class_init (NfHiaudClass * klass);
static void nf_hi_aud_instance_init (GTypeInstance* instance, gpointer g_class);
static void nf_hi_aud_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void nf_hi_aud_dispose (GObject * object);
static void nf_hi_aud_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void nf_hi_aud_finalize (GObject * object);

static gboolean nf_hi_aud_init_lib(NfHiaud *hi_aud);

static void nf_hi_aud_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static gboolean nf_hi_aud_put_frame_out(gpointer frame, gint size);
static gboolean nf_hi_aud_put_frame_rec(gint ch_num, gpointer frame, gint size);
static NF_HI_AUD_QDATA *nf_hi_aud_CreateQdata(HI_U32 s32Len);
static gpointer nf_hi_aud_CreateQdata_gst_buffer(GstBuffer *buffer);
static gboolean nf_hi_aud_freeQdata( NF_HI_AUD_QDATA *pstQdata );
static gboolean nf_hi_aud_freeQdata_gst_buffer(NF_HI_AUD_QDATA *pstQdata);
static gboolean nf_hi_aud_sendQdata( NF_HI_AUD_QDATA *pstQdata, GAsyncQueue *pQue);
static void nf_hi_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader);

/**
	Extern Function
**/
extern gboolean nf_sysman_hotkey_is_nfs(void);
extern GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose);
extern HI_S32 HIADP_AVPlay_RegADecLib(void);
extern HI_S32 HIADP_Disp_Init(HI_UNF_ENC_FMT_E enFormat);
extern HI_S32 HIADP_AVPlay_SetAdecAttr(HI_HANDLE hAvplay, HI_U32 enADecType, HI_HA_DECODEMODE_E enMode, HI_S32 isCoreOnly);
extern guint nf_live_get_audio_output_type(void);

/**
	Gloval Variable
**/
static GObjectClass *parent_class = NULL;
static NfHiaud *_nf_hi_aud = NULL;
static GStaticMutex _nf_hi_aud_mutex = G_STATIC_MUTEX_INIT;

static gint _tbl_aud_input[16] =
{
	HI_AUD_INPUT_LV0,   HI_AUD_INPUT_LV1,   HI_AUD_INPUT_LV2,   HI_AUD_INPUT_LV3,   HI_AUD_INPUT_LV4,
	HI_AUD_INPUT_LV5,   HI_AUD_INPUT_LV6,   HI_AUD_INPUT_LV7,   HI_AUD_INPUT_LV8,   HI_AUD_INPUT_LV9,
	HI_AUD_INPUT_LV10,  HI_AUD_INPUT_LV11,  HI_AUD_INPUT_LV12,  HI_AUD_INPUT_LV13,  HI_AUD_INPUT_LV14,
	HI_AUD_INPUT_LV15
};


GType
nf_hi_aud_get_type (void)
{
	static GType nf_hi_aud_type = 0;

	if (G_UNLIKELY (nf_hi_aud_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfHiaudClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_hi_aud_class_init,
			NULL,
			NULL,
			sizeof (NfHiaud),
			0,
			(GInstanceInitFunc) nf_hi_aud_instance_init,
			NULL
		};

		nf_hi_aud_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfHiaud", &object_info, 0);
	}

	return nf_hi_aud_type;
}

static void
nf_hi_aud_class_init (NfHiaudClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_hi_aud_set_property;
	gobject_class->get_property = nf_hi_aud_get_property;

	gobject_class->dispose = nf_hi_aud_dispose;
	gobject_class->finalize = nf_hi_aud_finalize;

}

static void
nf_hi_aud_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfHiaud *self = NF_HI_AUD(instance);

	self->init_done = 0;
	self->queue = g_async_queue_new();
	self->queue_out = g_async_queue_new();

	// event context & loop
	self->context = g_main_context_new ();
	self->loop = g_main_loop_new (self->context, FALSE);
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_hi_aud_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_hi_aud_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_hi_aud_set_property (GObject * object, guint prop_id,
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
nf_hi_aud_get_property (GObject * object, guint prop_id,
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

gboolean nf_hi_aud_init(void)
{
	gulong cb_handle=0;
	gint ch=0;

	g_return_val_if_fail (_nf_hi_aud == NULL, FALSE);

	_nf_hi_aud = g_object_new ( NF_TYPE_HI_AUD , NULL);

	g_message("%s start!!", __FUNCTION__);

	if(!nf_hi_aud_init_lib(_nf_hi_aud)) {
		g_warning("%s fail!!", __FUNCTION__);
	}
	else {
		g_message("%s success!!", __FUNCTION__);
	}

	_nf_hi_aud->aud_output=nf_live_get_audio_output_type();

	cb_handle= nf_notify_connect_cb( "vloss", nf_hi_aud_vloss_cb_func , (gpointer)_nf_hi_aud);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", nf_hi_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	// Thread Create
	_nf_hi_aud->thread_rec=g_thread_create((GThreadFunc)nf_hi_aud_threadRec,
										_nf_hi_aud, FALSE, NULL);
	_nf_hi_aud->thread_rd=g_thread_create((GThreadFunc)nf_hi_aud_threadRd,
										_nf_hi_aud, FALSE, NULL);
	_nf_hi_aud->thread_out=g_thread_create((GThreadFunc)nf_hi_aud_threadOut,
										_nf_hi_aud, FALSE, NULL);

	// Thread Enable
	_nf_hi_aud->thread_run_rd=FALSE;
	_nf_hi_aud->thread_run_rec=FALSE;
	_nf_hi_aud->thread_run_out=TRUE;

	_nf_hi_aud->init_done=TRUE;

	return HI_SUCCESS;
}

static gboolean nf_hi_aud_init_lib(NfHiaud *hi_aud)
{
	HI_S32 Ret=0;
	HI_U32 u32SndNum=0;
	HI_HANDLE hTrack=0;
	HI_U32 u32Snd=0;
	HI_UNF_AUDIOTRACK_ATTR_S  stTrackAttr;
	HI_UNF_SND_ATTR_S stAttr;

	HI_U32 AdecType=0;
	HI_UNF_AVPLAY_ATTR_S AvplayAttr;
	HI_UNF_SYNC_ATTR_S AvSyncAttr;
	HI_UNF_AVPLAY_STOP_OPT_S Stop;
	HI_CHAR InputCmd[32];
	HI_U32 u32InVal = 0;
	HI_HANDLE hAvplay=0;
	HI_HA_DECODEMODE_E enAudioDecMode = HD_DEC_MODE_RAWPCM;
	HI_S32 s32DtsDtsCoreOnly = 0;
	HI_UNF_ENC_FMT_E g_enDefaultFmt = HI_UNF_ENC_FMT_720P_50;
	int hdmi_toggle = 0;
	HI_UNF_SND_E enSnd = HI_UNF_SND_0;
	#if defined(NF_REC_AUDIO_HISILICON_TEST)
		FILE *fp=NULL;
		char filename[32]={0, };
	#endif

	u32SndNum=1;
	#if defined(NF_REC_AUDIO_HISILICON_TEST)
		#if 0
			strcpy(filename, "/play/1.mp3");
		#else
			strcpy(filename, "/test/1.aiff");
		#endif
		hi_aud->aud_param.fp=fopen(filename, "rb");
		fp=hi_aud->aud_param.fp;
		if (!fp) {
			printf("open file %s error!\n", filename);
			return FALSE;
		}
		else {
			g_message("%s fileopen[%s]", __FUNCTION__, filename);
		}
	#endif

	#if 1
		AdecType=HA_AUDIO_ID_PCM;		// singned 16bit 8000 pcm
	#else
		AdecType=HA_AUDIO_ID_MP3;
	#endif

	#if 1		// lib/nmf/lib/libipx_dispmux.so
		HI_SYS_Init();
	#endif
	Ret = HI_UNF_AVPLAY_Init();
	if (Ret != HI_SUCCESS)
	{
		printf("call HI_UNF_AVPLAY_Init failed.\n");
		goto SYS_DEINIT;
	}

	Ret = HI_UNF_SND_Init();
	if (Ret != HI_SUCCESS)
	{
		printf("call HI_UNF_SND_Init failed.\n");
		goto AVPLAY_DEINIT;
	}

	HI_UNF_SND_GetDefaultOpenAttr(HI_UNF_SND_0, &stAttr);

	/*
		Must Change For PCM !! common/hi_adp_mpi.c -> HIADP_AVPlay_SetAdecAttr() -> 
		stWavFormat.nSamplesPerSec = 48000 -> 8000;
		stWavFormat.wBitsPerSample = 16;
	*/
	Ret = HI_UNF_SND_Open(HI_UNF_SND_0, &stAttr);
	if (Ret != HI_SUCCESS)
	{
		printf("call HI_UNF_SND_Open failed.\n");
		goto SND_CLOSE;
	}

	Ret = HIADP_AVPlay_RegADecLib();
	if (Ret != HI_SUCCESS)
	{
		printf("call HI_UNF_AVPLAY_RegisterAcodecLib failed.\n");
		goto SND_CLOSE;
	}

	Ret = HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
	Ret |= HI_UNF_AVPLAY_Create(&AvplayAttr, &hAvplay);
	if (Ret != HI_SUCCESS)
	{
		printf("call HI_UNF_AVPLAY_Create failed.\n");
		goto AVPLAY_DESTROY;
	}
	Ret = HI_UNF_AVPLAY_GetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &AvSyncAttr);
	AvSyncAttr.enSyncRef = HI_UNF_SYNC_REF_NONE;
	Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &AvSyncAttr);
	if (HI_SUCCESS != Ret)
	{
		printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
		goto AVPLAY_DESTROY;
	}


	Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
	if (Ret != HI_SUCCESS) {
	  printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
	  goto ACHN_CLOSE;
	}

	Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);

	Ret = HI_UNF_SND_CreateTrack(u32Snd,&stTrackAttr,&hTrack);
	if (Ret != HI_SUCCESS) {
		printf("call HI_UNF_SND_CreateTrack failed.\n");
		goto TRACK_DESTROY;
	}

	Ret = HIADP_Disp_Init(g_enDefaultFmt);
	if (Ret != HI_SUCCESS) {
		printf("call DispInit failed.\n");
		goto SND_DETACH;
	}

	Ret = HI_UNF_SND_Attach(hTrack, hAvplay);
	if (Ret != HI_SUCCESS) {
		printf("call HI_UNF_SND_Attach failed.\n");
		goto SND_DETACH;
	}
	Ret = HIADP_AVPlay_SetAdecAttr(hAvplay, AdecType, enAudioDecMode, s32DtsDtsCoreOnly);
	if (Ret != HI_SUCCESS) {
		printf("call HIADP_AVPlay_SetAdecAttr failed.\n");
		goto SND_DETACH;
	}
	Ret = HI_UNF_AVPLAY_Start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
	if (Ret != HI_SUCCESS) {
		printf("call HI_UNF_AVPLAY_Start failed.\n");
	}

	// Set To HDMI Output
	enSnd = HI_UNF_SND_0;
	hdmi_toggle++;
	if(hdmi_toggle >= HI_UNF_SND_HDMI_MODE_BUTT)
	{
		hdmi_toggle = HI_UNF_SND_HDMI_MODE_LPCM;
	}

	HI_UNF_SND_SetHdmiMode(enSnd, HI_UNF_SND_OUTPUTPORT_HDMI0, hdmi_toggle);
	#if 0
		printf("hdmi mode %d!\n", hdmi_toggle);
	#endif

	hi_aud->aud_param.u32Snd=0;
	hi_aud->aud_param.u32SndNum=u32SndNum;
	hi_aud->aud_param.hAvplay=hAvplay;
	#if defined(NF_REC_AUDIO_HISILICON_TEST)
		hi_aud->aud_param.thread_run_test=TRUE;
		hi_aud->aud_param.thread=g_thread_create((GThreadFunc)nf_hi_aud_thread_test, hi_aud, FALSE, NULL);
	#endif

	return TRUE;

SND_DETACH:
	HI_UNF_SND_Detach(hTrack, hAvplay);

TRACK_DESTROY:
	HI_UNF_SND_DestroyTrack(hTrack);

ACHN_CLOSE:
	HI_UNF_AVPLAY_ChnClose(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

AVPLAY_DESTROY:
	HI_UNF_AVPLAY_Destroy(hAvplay);

SND_CLOSE:
	HI_UNF_SND_Close(u32Snd);

AVPLAY_DEINIT:
	HI_UNF_AVPLAY_DeInit();

SYS_DEINIT:
	HI_SYS_DeInit();

	#if defined(NF_REC_AUDIO_HISILICON_TEST)
		fclose(fp);
		fp=NULL;
	#endif

	return FALSE;
}

static void
nf_hi_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint type=0;

	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_AUDIO) {

		_nf_hi_aud->aud_output=nf_live_get_audio_output_type();

		return ;
	}
}

HI_BOOL nf_hi_aud_start(GValue *data)
{
	return HI_TRUE;
}

static void nf_hi_aud_threadRec(NfHiaud *self)
{
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while (!self->init_done) {
		g_usleep(1000);
	}

	while(self->thread_run_rec) {

		#if 0
			g_message("%s line%d cmd[%d] size[%d]", __FUNCTION__, __LINE__, qdata->s32Cmd, qdata->s32Len);
		#endif

		#if 0
			switch(qdata->s32Cmd )
			{
				case HI_CMD_AUD_REC:
				{
				}
				break;
				case HI_CMD_AUD_REC_GST_BUFFER:
				{
				}
				break;
				case HI_CMD_AUD_CFG:
				{
				}
				break;
				default:
					g_warning("[%s][%d] Invaild cmd.", __FUNCTION__, __LINE__);
					goto exitRec;
			} /* switch */

			if(qdata->s32Cmd == HI_CMD_AUD_REC_GST_BUFFER) {
				if(!nf_hi_aud_freeQdata_gst_buffer(qdata)) {
					g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
			else if(qdata->s32Cmd == HI_CMD_AUD_REC) {
				if(!nf_hi_aud_freeQdata(qdata)) {
					g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
			else if(qdata->s32Cmd == HI_CMD_AUD_CFG) {
				if(!nf_hi_aud_freeQdata(qdata)) {
					g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
					goto exitRec;
				}
			}
		#else
			goto exitRec;
		#endif

		g_usleep(1000);
	}

exitRec:
	self->thread_run_rec=FALSE;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

static void nf_hi_aud_threadRd(NfHiaud *self)
{
	gint policy;
	struct sched_param sched;
	pthread_t thread;

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while (!self->init_done) {
		g_usleep(1000);
	}

	while(self->thread_run_rd)
	{
		g_usleep(1000);
	}

exitRd:
	self->thread_run_rd=FALSE;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

#if 0
	#define NF_HI_AUDIO_STREAM_DUMP_OUT
#endif
static void nf_hi_aud_threadOut(NfHiaud *self)
{
	gpointer pQueData=NULL;

	gint policy;
	struct sched_param sched;
	pthread_t thread;

	HI_U32 u32Snd=0;
	HI_U32 u32SndNum=0;
	HI_HANDLE hAvplay=0;

	#if defined(NF_HI_AUDIO_STREAM_DUMP_OUT)
		gint cnt_audio=0;
		gchar str_aud[32]={0, };
		FILE *fp_aud=NULL;

		strcpy(str_aud, "audio_test_out_ch0.raw");
		if((fp_aud[cnt_audio] = fopen(str_aud, "w")) == NULL)
		{
			g_warning("%s File Open Error!! name -> %s", __FUNCTION__, str_aud);
			return ;
		}
		else
			g_message("%s Line[%d] File Open Success!! name -> %s", __FUNCTION__, __LINE__, str_aud);
	#endif

	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);

	while (!self->init_done) {
		g_usleep(1000);
	}

	u32Snd=self->aud_param.u32Snd;
	u32SndNum=self->aud_param.u32SndNum;
	hAvplay=self->aud_param.hAvplay;

	while(self->thread_run_out) {

		NF_HI_AUD_QDATA *qdata=NULL;

		pQueData=g_async_queue_pop(self->queue_out);
		g_assert(NULL != pQueData);

		qdata=(NF_HI_AUD_QDATA *)pQueData;
		g_assert(NULL != qdata);

		#if 0
			g_message("%s line%d cmd[%d] size[%d]", __FUNCTION__, __LINE__, qdata->s32Cmd, qdata->s32Len);
		#endif

		switch(qdata->s32Cmd )
		{
			case HI_CMD_AUD_OUT_HDMI:
			{
				int i=0, j=0;
				guint size=0;
				short s16AudioStream[HI_AUDIO_PTNUMPERFRM];
				static guchar u8AudioRemainStream[HI_AUDIO_PTNUMPERFRM];
				static int remain_size=0;
				static int pre_stream_size=0;
				int SendAllSize = 0;
				char *temp_ptr;
				short  pcm_val=0;
				guchar  u_val;
				int stream_size=qdata->s32Len;
				guchar *u8AudioStream;

				#if defined(NF_HI_AUDIO_STREAM_DUMP_OUT)
					// muraw write
					if(nf_sysman_hotkey_is_nfs()) {
						fwrite(qdata->pData, 1, qdata->s32Len, fp_aud);
					}
				#endif

				j=0;
				if(pre_stream_size != stream_size) {
					remain_size=0;
					pre_stream_size=stream_size;
				}

				u8AudioStream=(guchar *)malloc(stream_size + remain_size);

				if(remain_size) {
					memcpy(u8AudioStream, u8AudioRemainStream, remain_size);
				}
				memcpy(u8AudioStream + remain_size, qdata->pData, stream_size);

				remain_size += stream_size;
				while(remain_size > 0)
				{
					if(remain_size < HI_AUDIO_PTNUMPERFRM)
					{
						memcpy(u8AudioRemainStream, (u8AudioStream + SendAllSize), remain_size);
						break;
					}

					temp_ptr=u8AudioStream + (j * HI_AUDIO_PTNUMPERFRM);

					for(i=0; i<HI_AUDIO_PTNUMPERFRM; i++)
					{
						u_val = (guchar)*(temp_ptr + i);
						#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
							pcm_val = nf_hi_muraw_to_lpcm16(u_val);
						#else
							pcm_val=u_val;
						#endif
						s16AudioStream[i] = pcm_val;
					}

					remain_size-=HI_AUDIO_PTNUMPERFRM;
					SendAllSize+=HI_AUDIO_PTNUMPERFRM;

					if(self->aud_output == HI_AUD_OUTPUT_HDMI) {
						nf_hi_aud_hdmi_write(u32Snd, u32SndNum, hAvplay, (guchar *)s16AudioStream, 0x1000);
					}

					j++;
					#if 0       // pcm write
						#if defined(NF_HI_AUDIO_STREAM_DUMP_OUT)
							if(nf_sysman_hotkey_is_nfs()) {
								fwrite((char *)s16AudioStream, 1, 0x1000, fp_aud);
							}
						#endif
					#endif
				}

				free(u8AudioStream);
			}
			break;
			case HI_CMD_AUD_OUT_HDMI_GST_BUFFER:
			{
			}
			break;
			default:
				g_warning("[%s][%d] Invaild cmd.", __FUNCTION__, __LINE__);
				goto exitOut;
		} /* switch */

		if(qdata->s32Cmd == HI_CMD_AUD_OUT_HDMI_GST_BUFFER) {
			if(!nf_hi_aud_freeQdata_gst_buffer(qdata)) {
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitOut;
			}
		}
		else if(qdata->s32Cmd == HI_CMD_AUD_OUT_HDMI) {
			if(!nf_hi_aud_freeQdata(qdata)) {
				g_warning("[%s][%d] fail.", __FUNCTION__, __LINE__);
				goto exitOut;
			}
		}

		g_usleep(1000);
	}

exitOut:
	self->thread_run_out=FALSE;
	g_thread_exit(0);
	g_error("[%s] END", __FUNCTION__);

	return;
}

#if defined(NF_REC_AUDIO_HISILICON_TEST)
static void nf_hi_aud_thread_test(NfHiaud *self)
{
	HI_UNF_STREAM_BUF_S StreamBuf;
	HI_U32 Readlen;
	HI_S32 Ret;
	HI_BOOL bAudBufAvail = HI_FALSE;
	HI_S32 AudEsFileOffest=0;
	HI_U32 u32Snd=0;
	HI_U32 u32SndNum=0;
	HI_HANDLE hAvplay=0;
	FILE *fp=NULL;

	while (!self->init_done) {
		g_usleep(1000);
	}

	u32Snd=self->aud_param.u32Snd;
	u32SndNum=self->aud_param.u32SndNum;
	hAvplay=self->aud_param.hAvplay;
	fp=self->aud_param.fp;

	while(self->aud_param.thread_run_test) {
		Ret = HI_UNF_AVPLAY_GetBuf(hAvplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &StreamBuf, 0);
		if(HI_SUCCESS == Ret) {
			bAudBufAvail = HI_TRUE;
			Readlen = fread(StreamBuf.pu8Data, sizeof(HI_S8), 0x1000, fp);
			if (Readlen > 0) {
				Ret = HI_UNF_AVPLAY_PutBuf(hAvplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, Readlen, 0);
				if (Ret != HI_SUCCESS) {
					printf("call HI_UNF_AVPLAY_PutBuf failed.\n");
				}
			}
			else if (Readlen <= 0) {
				printf("read aud file error!\n");
				rewind(fp);
				if(AudEsFileOffest) {
					fseek(fp, AudEsFileOffest, SEEK_SET);
				}
			}
		}
		else if(Ret != HI_SUCCESS) {
			bAudBufAvail = HI_FALSE;
		}

		/* wait for buffer */
		if(HI_FALSE == bAudBufAvail) {
			usleep(1000 * 10);
		}
	}

	return;
}
#endif

/*
 *	HI_U32 u32Snd, HI_U32 u32SndNum -> Array Index for Output
 *	HiSTBLinuxV100R005C00SPC040/software/HiSTBLinuxV100R005C00SPC040/sample/ao/play/sample_audio_play.c 
 *	-> static HI_VOID AudioTthread(HI_VOID *args)
*/
static gboolean nf_hi_aud_hdmi_write(HI_U32 u32Snd, HI_U32 u32SndNum, HI_HANDLE hAvplay, guchar *data, guint size)
{
	HI_UNF_STREAM_BUF_S StreamBuf;
	HI_U32 Readlen=0;
	HI_S32 Ret=0;
	HI_BOOL bAudBufAvail = HI_FALSE;

	#if 0
		Ret = HI_UNF_AVPLAY_GetBuf(hAvplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &StreamBuf, 0);
	#else
		Ret = HI_UNF_AVPLAY_GetBuf(hAvplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, size, &StreamBuf, 0);
	#endif
	if(HI_SUCCESS == Ret) {
		bAudBufAvail = HI_TRUE;

		memcpy(StreamBuf.pu8Data, data, size);
		Ret = HI_UNF_AVPLAY_PutBuf(hAvplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, size, 0);
		if (Ret != HI_SUCCESS) {
			printf("call HI_UNF_AVPLAY_PutBuf failed.\n");
		}
	}
	else if(Ret != HI_SUCCESS) {
		printf("call HI_UNF_AVPLAY_GetBuf failed.\n");
		bAudBufAvail = HI_FALSE;
	}

	/* wait for buffer */
	if(HI_FALSE == bAudBufAvail) {
		usleep(1000 * 10);
	}

	return TRUE;
}

static void
nf_hi_aud_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NfHiaud *ptr=NULL;

	g_return_if_fail(pinfo != NULL);

	ptr=(NfHiaud *)data;
	ptr->vin_mask = ~(pinfo->d.params[0]);
}

/*
	Write to Output
	Gather For Audio Frame
*/
gboolean nf_hi_aud_send_frame_out(gpointer frame, gint size)
{
	static guchar *stream=NULL;
	static gint stream_size=0;

	if(stream != NULL) {
		guchar *tmp=NULL;

		tmp=(guchar *)g_malloc0(stream_size);
		g_assert(NULL != tmp);
		memcpy((gchar *)tmp, (gchar *)stream, stream_size);

		#if 1
			g_free(stream);
			stream=(guchar *)g_malloc0(stream_size + size);
		#else
			stream=(guchar *)g_realloc(stream, stream_size + size);
		#endif
		g_assert(NULL != stream);

		memset((gchar *)stream, 0x0, stream_size + size);
		memcpy((gchar *)stream, (gchar *)tmp, stream_size);
		memcpy((gchar *)(stream + stream_size), (gchar *)frame, size);

		g_free(tmp);
	}
	else {

		stream=(guchar *)g_malloc0(size);
		g_assert(NULL != stream);

		memcpy((gchar *)stream, (gchar *)frame, size);
	}

	stream_size+=size;

	if(stream_size >= HI_AUDIO_PTNUMPERFRM) {
		nf_hi_aud_put_frame_out(stream, stream_size);

		g_free(stream);
		stream=NULL;
		stream_size=0;
	}

	return TRUE;
}

gboolean nf_hi_aud_send_frame_rec(gint ch_num, gpointer frame, gint size)
{
	static guchar *stream[NUM_AUDIO]={NULL, };
	static gint stream_size[NUM_AUDIO]={0 ,};

	if(stream[ch_num] != NULL) {
		guchar *tmp=NULL;

		tmp=(guchar *)g_malloc0(stream_size[ch_num]);
		g_assert(NULL != tmp);
		memcpy((gchar *)tmp, (gchar *)stream[ch_num], stream_size[ch_num]);

		#if 1
			g_free(stream[ch_num]);
			stream[ch_num]=(guchar *)g_malloc0(stream_size[ch_num] + size);
		#else
			stream[ch_num]=(guchar *)g_realloc(stream[ch_num], stream_size[ch_num] + size);
		#endif
		g_assert(NULL != stream[ch_num]);

		memset((gchar *)stream[ch_num], 0x0, stream_size[ch_num] + size);
		memcpy((gchar *)stream[ch_num], (gchar *)tmp, stream_size[ch_num]);
		memcpy((gchar *)(stream[ch_num] + stream_size[ch_num]), (gchar *)frame, size);

		g_free(tmp);
	}
	else {

		stream[ch_num]=(guchar *)g_malloc0(size);
		g_assert(NULL != stream[ch_num]);

		memcpy((gchar *)stream[ch_num], (gchar *)frame, size);
	}

	stream_size[ch_num]+=size;

	if(stream_size[ch_num] >= HI_AUDIO_PTNUMPERFRM) {
		nf_hi_aud_put_frame_rec(ch_num, stream[ch_num], stream_size[ch_num]);

		g_free(stream[ch_num]);
		stream[ch_num]=NULL;
		stream_size[ch_num]=0;
	}

	return TRUE;
}

static gboolean nf_hi_aud_put_frame_out(gpointer frame, gint size)
{
	NF_HI_AUD_QDATA *pstQdata=NULL;
	GTimeVal tv;

	gettimeofday((struct timeval *)&tv, NULL);

	pstQdata = nf_hi_aud_CreateQdata(size);
	memcpy(pstQdata->pData, frame, size);

	pstQdata->s32Len = size;

	g_return_val_if_fail (pstQdata != 0, FALSE);

	nf_hi_aud_sendQdata(pstQdata, _nf_hi_aud->queue_out);

	return TRUE;
}

static gboolean nf_hi_aud_put_frame_rec(gint ch_num, gpointer frame, gint size)
{
	NF_HI_AUD_QDATA *pstQdata=NULL;
	GTimeVal tv;

	gettimeofday((struct timeval *)&tv, NULL);

	pstQdata = nf_hi_aud_CreateQdata(size);
	memcpy(pstQdata->pData, frame, size);

	pstQdata->s32Chn = ch_num;
	pstQdata->u64Start = tv.tv_usec;
	pstQdata->s32Len = size;
	pstQdata->u64End = tv.tv_usec;
	pstQdata->s32Cmd = HI_CMD_AUD_REC;

	g_return_val_if_fail (pstQdata != 0, FALSE);

	nf_hi_aud_sendQdata(pstQdata, _nf_hi_aud->queue);

	return TRUE;
}

gboolean nf_hi_aud_put_frame_gst(gint ch_num, gpointer frame)
{
	GstBuffer *buffer = (GstBuffer *)frame;
	ICODEC_HEADER *pheader = GST_BUFFER_DATA(buffer);;
	NF_HI_AUD_QDATA *pstQdata=NULL;
	GTimeVal tv;

	g_return_val_if_fail (frame != 0, FALSE);
	g_return_val_if_fail (ch_num >= 0 && ch_num < NUM_ANALOG_CHANNEL, FALSE);

	gettimeofday((struct timeval *)&tv, NULL);

	#ifdef ENABLE_PUT_FRAME_MAX_QUEUE
		while( g_async_queue_length( _nf_HI_aud->stAud.queue ) > (16*5) ) g_usleep(1000);
	#endif

	if ( pheader->frame_type != NF_FRAME_TYPE_AUDIO ) {
		nf_hi_aud_dump_icodec_header("INV_AUDIO", pheader);
		return -1;
	}

	#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI] )
			nf_hi_aud_dump_icodec_header("AUDIO", pheader);

		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI_HEX] )
			nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
	#endif

	#if 0
		pstQdata = nf_hi_aud_CreateQdata( gst_buffer_get_size(buffer) );
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
		pstQdata=nf_hi_aud_CreateQdata_gst_buffer(buffer);
	#endif

	g_return_val_if_fail (pstQdata != 0, FALSE);

	nf_hi_aud_sendQdata(pstQdata, _nf_hi_aud->queue);

	gst_buffer_unref(buffer);

	return TRUE;
}

static NF_HI_AUD_QDATA *nf_hi_aud_CreateQdata(HI_U32 s32Len)
{
	NF_HI_AUD_QDATA *qdata = NULL;

	qdata = (NF_HI_AUD_QDATA *) g_malloc0( sizeof(NF_HI_AUD_QDATA) );

	g_assert( NULL != qdata );

	qdata->pData = (HI_VOID *) g_malloc( s32Len );

	g_assert( NULL != qdata->pData );

	return qdata;
}

static gpointer
nf_hi_aud_CreateQdata_gst_buffer(GstBuffer *buffer)
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

static gboolean 
nf_hi_aud_freeQdata( NF_HI_AUD_QDATA *pstQdata )
{
	g_assert ( NULL != pstQdata );

	g_free( pstQdata->pData );
	g_free( pstQdata );

	return TRUE;
}

static gboolean 
nf_hi_aud_freeQdata_gst_buffer(NF_HI_AUD_QDATA *pstQdata)
{
	g_assert ( NULL != pstQdata );

	gst_buffer_unref( pstQdata->pData );

	g_free( pstQdata );

	return HI_TRUE;
}

static 
gboolean nf_hi_aud_sendQdata( NF_HI_AUD_QDATA *pstQdata, GAsyncQueue *pQue)
{
	g_assert ( NULL != pstQdata );
	g_assert ( NULL != pQue );

	g_async_queue_push( pQue, pstQdata );

	return HI_TRUE;
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

guchar nf_hi_lpcm16_to_muraw(short  pcm_val)    /* 2's complement (16-bit range) */
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
	seg = nf_hi_search_seg(pcm_val, seg_uend, 8);

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

short nf_hi_search_seg(short val, short *table, short size)
{
	short i=0;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

short nf_hi_muraw_to_lpcm16(guchar u_val)
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

void nf_hi_aud_convert(gchar *stream_src, gchar *stream_dest, guint len)
{
	gint i=0, j=0;
	gshort pcm_val=0;

	for(i=0, j=0; i<(gint)len; i++, j+=2)
	{
		memcpy(&pcm_val, stream_src+j, sizeof(gshort));
		*stream_dest=(gchar)nf_hi_lpcm16_to_muraw(pcm_val);

		*stream_dest++;
	}
}
#if 0
/* A-law to u-law conversion */
unsigned char alaw2ulaw(unsigned char aval)
{
	aval &= 0xff;
	return (unsigned char) ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
		(0x7F ^ _a2u[aval ^ 0x55]));
}

/* u-law to A-law conversion */
unsigned char ulaw2alaw(unsigned char uval)
{
	uval &= 0xff;
	return (unsigned char) ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) :
		(unsigned char) (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}

/* 16 bit swapping */
short swap_linear (short pcm_val)
{
	struct lohibyte { unsigned char lb, hb;};
	union { struct lohibyte b;
		 short i;
		  } exchange;
	unsigned char c;

	exchange.i      = pcm_val;
	c               = exchange.b.hb;
	exchange.b.hb   = exchange.b.lb;
	exchange.b.lb   = c;
	return (exchange.i);
}

#endif

#endif


static void
nf_hi_aud_dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
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

