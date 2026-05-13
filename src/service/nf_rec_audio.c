#include <glib.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// #include <gst/nf/gstnfbuddybuffer.h>
#include <gobj.h>
#include <gobjmedia.h>
#include <pthread.h>

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"

#include "nf_record.h"
#include "nf_rec_audio.h"

#include "libsst.h"

#include "nf_codec_header.h"
#include "nf_dspcomm_app.h"

#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "recaudio"

#define DEBUG_REC_AUDIO_JBSHELL
#define DEBUG_REC_AUDIO_CMD

#define ENABLE_REC_AUDIO_DSP_FD

#define DEBUG_REC_AUDIO_LOG		// log_system

#ifdef DEBUG_REC_AUDIO_JBSHELL
	#include "jbshell.h"
#endif

#if defined(ENABLE_REC_AUDIO_DSP_FD)
	#include "nf_util_device.h"
#endif

#if defined(ENABLE_REC_LIVE_AUDIO)
	#include "nf_sysman.h"
	#include "nf_network.h"
#endif

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
	#include "itx_ai_def.h"
#endif

//#define TRACE_ANFREC_AUDIO_
#ifdef	TRACE_ANFREC_AUDIO_
#define __T(fmt, args...)	g_print("[%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)
#define __M(fmt, args...)	g_message(fmt"\n", ##args)

#else	/**/
#define __T(fmt, afgs...)
#define __M(fmt, args...)
#endif	/**/
#define __E(fmt, args...)    g_error("[ANFAUD] "fmt"\n", ##args)

typedef enum _DEBUG_REC_AUDIO_IDX_E
{
	DEBUG_REC_AUDIO_IDX_DUMP 		= 0,
	DEBUG_REC_AUDIO_IDX_SKIP_SST 	= 1,
	DEBUG_REC_AUDIO_IDX_GET 		= 2,
	DEBUG_REC_AUDIO_IDX_GET_HEX		= 3,
	
	DEBUG_REC_AUDIO_IDX_PUT 		= 4,
	DEBUG_REC_AUDIO_IDX_PUT_HEX		= 5,
	DEBUG_REC_AUDIO_IDX_PUT_SST 	= 6,
	DEBUG_REC_AUDIO_IDX_PUT_DIFF	= 7,

	DEBUG_REC_AUDIO_IDX_HANDOFF 	= 8,	
	DEBUG_REC_AUDIO_IDX_CMD 		= 9,	
	DEBUG_REC_AUDIO_IDX_PUTAPI 		= 10,	
	DEBUG_REC_AUDIO_IDX_PUTAPI_HEX 	= 11,	

	DEBUG_REC_AUDIO_IDX_MICOUT	 	= 12,	
	
	DEBUG_REC_AUDIO_IDX_NR						
}DEBUG_REC_AUDIO_IDX_E;

static const char *_DEBUG_REC_AUDIO_str[32] =
{
	"REC_AUDIO_IDX_DUMP",
	"REC_AUDIO_IDX_SKIP_SST",
	"REC_AUDIO_IDX_GET",
	"REC_AUDIO_IDX_GET_HEX",
	
	"REC_AUDIO_IDX_PUT",
	"REC_AUDIO_IDX_PUT_HEX",
	"REC_AUDIO_IDX_PUT_SST",
	"REC_AUDIO_IDX_PUT_DIFF",
	
	"REC_AUDIO_IDX_HANDOFF",	
	"REC_AUDIO_IDX_CMD",
	"REC_AUDIO_IDX_PUTAPI",	
	"REC_AUDIO_IDX_PUTAPI_HEX",

	"REC_AUDIO_IDX_MICOUT",
	
	"REC_AUDIO_IDX_NR"
};

static gint _DEBUG_REC_AUDIO_log[32] = 
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
};


/* Object signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_0,		
	LAST_PROP	
	/* FILL ME */
};

static gint _tbl_aud_input[16] =
{
	NF_AUDIO_INPUT_CH01,   NF_AUDIO_INPUT_CH02,   NF_AUDIO_INPUT_CH03,   NF_AUDIO_INPUT_CH04,   NF_AUDIO_INPUT_CH05,
	NF_AUDIO_INPUT_CH06,   NF_AUDIO_INPUT_CH07,   NF_AUDIO_INPUT_CH08,   NF_AUDIO_INPUT_CH09,   NF_AUDIO_INPUT_CH10,
	NF_AUDIO_INPUT_CH11,   NF_AUDIO_INPUT_CH12,   NF_AUDIO_INPUT_CH13,   NF_AUDIO_INPUT_CH14,   NF_AUDIO_INPUT_CH15,
	NF_AUDIO_INPUT_CH16
};


static void nf_rec_audio_class_init (NfRecAudioClass * klass);
static void nf_rec_audio_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_rec_audio_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_rec_audio_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_rec_audio_dispose (GObject * object);
static void nf_rec_audio_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfRecAudio	*_nf_rec_audio = NULL;

static void rec_audio_thread_func (NfRecAudio *arg);
#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
	static void rec_audio_alarm_thread_func (NfRecAudio *self);
#endif
static void rec_audio_thread_put_func (NfRecAudio *arg);
#if defined(ENABLE_REC_NET_IMMEDIATELY)
	static void rec_audio_thread_net_immediately_put_func (NfRecAudio * self);
#endif
static gpointer  _nf_audio_qitem_new(int size);
static void _nf_audio_qitem_free(NF_AUDIO_QITEM *pqitem);

static void dump_icodec_header( const char *str, ICODEC_HEADER *pheader);
static void dump_rec_audio_cmd( const char *str, NF_REC_AUDIO_PARAM	*param);
static gboolean	_nf_rec_audio_sst_put_frame( guint ch, gpointer frame );
static gboolean _nf_rec_aud_sysdb_chg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#if 0
static void nf_rec_audio_load_data_alarm_audio(void);
#endif
#if defined(ENABLE_REC_LIVE_AUDIO)
	static gboolean	_nf_rec_audio_live_put_frame(gpointer frame, guint size);
	static gboolean _nf_rec_covert_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
	static void nf_rec_audio_live_data_load(void);
#endif

//#define NF_REC_ENABLE_DSP_RESET
#if defined(NF_REC_ENABLE_DSP_RESET)
	#define NF_REC_DSP_RESET_TIME		60
	static gboolean nf_rec_audio_reset_dsp(gpointer data);
#endif

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
extern gint nf_rec_aud_ai_send_frame(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud);
extern void nf_rec_aud_ai_evt_check(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt);
#endif

GType
nf_rec_audio_get_type (void)
{
	static GType nf_rec_audio_type = 0;

	if (G_UNLIKELY (nf_rec_audio_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfRecAudioClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_rec_audio_class_init,
			NULL,
			NULL,
			sizeof (NfRecAudio),
			0,
			(GInstanceInitFunc) nf_rec_audio_instance_init,
			NULL
		};

		nf_rec_audio_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfRecAudio", &object_info, 0);
	}
	
	return nf_rec_audio_type;
}

static void
nf_rec_audio_class_init (NfRecAudioClass * klass)
{	
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_rec_audio_set_property;
	gobject_class->get_property = nf_rec_audio_get_property;
			
	gobject_class->dispose = nf_rec_audio_dispose;
	gobject_class->finalize = nf_rec_audio_finalize;

}

static void
nf_rec_audio_instance_init (GTypeInstance* instance, gpointer g_class)
{
	int i=0;
	NfRecAudio *self = NF_REC_AUDIO (instance);


	self->init_done = 0;
	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		self->init_done_alarm = 0;
	#endif

	#ifdef ENABLE_REC_AUDIO_DSP_FD
		#if 0
			self->audio_fd = Open( "/dev/dsp", O_RDONLY, 0);
			g_message("%s LIVEAUDIO fd[%d]", __FUNCTION__, self->audio_fd);
		#else		// pakkhman
			self->audio_fd=nf_dev_open_dsp_read();
			self->audio_fd_write=nf_dev_open_dsp_write();
			g_message("%s LIVEAUDIO fd_read[%d] fd_write[%d]", __FUNCTION__, self->audio_fd, self->audio_fd_write);
		#endif

			nf_dev_audio_input_ch_cntl(NF_AUDIO_DEF_CHANNEL, self->audio_fd);

	#endif

//	g_assert( self->audio_fd != -1 );
				
	// queue ����
	self->queue = g_async_queue_new();
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		self->queue_net = g_async_queue_new();
	#endif
 	
 	memset( &self->rec_param, 0x00, sizeof(self->rec_param));
	memset( self->rec_param.ch_arr, NF_AUDIO_INPUT_OFF, sizeof(self->rec_param.ch_arr));
	
	for(i=0; i< NUM_ANALOG_CHANNEL; i++)
		self->stream_id[i] = -1;

 	memset( self->timestamp, 0x00, sizeof(self->timestamp));
	
	self->handoff_func = NULL;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		self->handoff_func_immediately = NULL;
	#endif
	self->handoff_ch_mask = 0;

	#ifdef ENABLE_REC_AUDIO_DSP_FD
		// "/dev/dsp" read	
		self->thread_run = 1;
		self->thread = g_thread_create(	(GThreadFunc)rec_audio_thread_func, 
										self, FALSE, NULL);
	#endif
										
	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		self->thread_run_alarm = 1;
		self->thread_alarm = g_thread_create(	(GThreadFunc)rec_audio_alarm_thread_func, 
										self, FALSE, NULL);
	#endif

	// audio_frame put	
	self->thread_put_run = 1;
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		self->thread_put_net_run = 1;
	#endif

	self->thread_put = g_thread_create(	(GThreadFunc)rec_audio_thread_put_func, 
									self, FALSE, NULL);
	#if defined(ENABLE_REC_NET_IMMEDIATELY)
		self->thread_put_net = g_thread_create(	(GThreadFunc)rec_audio_thread_net_immediately_put_func, 
										self, FALSE, NULL);
	#endif
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_rec_audio_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_rec_audio_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_rec_audio_set_property (GObject * object, guint prop_id,
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
nf_rec_audio_get_property (GObject * object, guint prop_id,
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

#if 0  // pakkhman
#define MAX_AUDIO_FRAME_SIZE	(64*1024)
#endif

static gpointer 
_nf_audio_qitem_new(int size)
{
	NF_AUDIO_QITEM	*qitem = NULL;
		
	g_return_val_if_fail ( size > 0 && size < MAX_AUDIO_FRAME_SIZE, NULL);
	
	qitem = g_malloc0( sizeof(NF_AUDIO_QITEM) );
	g_return_val_if_fail( qitem != NULL, NULL);
		
	qitem->buff = g_malloc( (gsize)size );
	if(qitem->buff == NULL)
	{
		g_free(qitem);
		g_return_val_if_fail( 0, NULL);
	}	
	qitem->buff_len = size;
		
	return qitem;		
}

static gpointer 
_nf_audio_qitem_new_gst_buffer(GstBuffer *buffer)
{
	NF_AUDIO_QITEM	*qitem = NULL;

	g_return_val_if_fail( buffer != NULL, NULL);
	
	qitem = g_malloc0( sizeof(NF_AUDIO_QITEM) );
	g_return_val_if_fail( qitem != NULL, NULL);
	
	void *tmp_gst_ret = NULL;
	tmp_gst_ret = gst_buffer_ref( buffer );
	if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
	
	qitem->type = NF_AUDIO_QITEM_TYPE_GST_BUFFER;
	qitem->buff = (char *)buffer;
	qitem->buff_len = 0;
		
	return qitem;
}

static void
_nf_audio_qitem_free(NF_AUDIO_QITEM *pqitem)
{
	g_return_if_fail ( pqitem );
	
	if(pqitem->type == NF_AUDIO_QITEM_TYPE_GST_BUFFER) {
		gst_buffer_unref( (GstBuffer *) pqitem->buff );
	}else{		
		if(pqitem->buff)
			g_free(	pqitem->buff );
	}	
	g_free(	pqitem );
	
	return;
}
	
static void 
dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", str , 
					pheader->chan, 
					pheader->flags, 
					pheader->frame_type, 
					pheader->frame_rate, 
					pheader->resolution, 						
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size	 );	
}
	
static void 
dump_rec_audio_cmd( const char *str, NF_REC_AUDIO_PARAM	*param)
{
	gint i=0;
	
	g_print("\n");
	for(i=0; i<	NUM_ANALOG_CHANNEL; i++)
	{
		g_message("%s ch[0x%02x] reason[%d] pre[%d]pre_flush[%d]", str, 
						param->ch_arr[i], param->rec_reason[i], 
						param->pre_rec_time[i], param->pre_rec_close[i] );
	}
	
}

static void 
_nf_rec_audio_dummy_handoff( gpointer data)
{
	GstBuffer *gst_buf = (GstBuffer *)data;
	g_return_if_fail( data != NULL);

	dump_icodec_header("dummy_handoff", (ICODEC_HEADER*)GST_BUFFER_DATA(gst_buf));

}

static gboolean
_nf_rec_audio_handoff( guint ch, gpointer frame )
{
	gint				clen, ret, stream_id;	
	GstNfBuddyBuffer 		*gst_buf;
	ICODEC_HEADER		*pheader;

	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );	

	gst_buf = (GstNfBuddyBuffer *)gst_nf_buddy_buffer_new_and_alloc (clen, NULL);

#ifdef ENABLE_GST_BUFFER_FAILED_SKIP 
	if(gst_buf == NULL)
	{
		g_warning("%s GST_BUFFER_FAILED ch[%d] size[%d]", __FUNCTION__,ch, clen);
		return 0;
	}
#else
	g_assert(gst_buf != NULL);	
#endif
	
	GST_BUFFER_SIZE(gst_buf) = (guint)clen;	
	memcpy( GST_BUFFER_DATA(gst_buf), frame, (sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED) );		

	pheader = (ICODEC_HEADER *)GST_BUFFER_DATA(gst_buf);	
	pheader->gst_buffer = gst_buf;
	pheader->chan = (guchar)ch;
	gst_buf->frame = pheader;

	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_HANDOFF] & 1<<ch)
		dump_icodec_header("handoff", pheader);

	NF_OBJECT_LOCK(_nf_rec_audio);
	if(_nf_rec_audio->handoff_func)
		_nf_rec_audio->handoff_func(gst_buf);
	NF_OBJECT_UNLOCK(_nf_rec_audio);

	gst_buffer_unref((GstBuffer *)gst_buf);

	return 1;
}

#if defined(ENABLE_REC_NET_IMMEDIATELY)
#define ENABLE_REC_NET_IMMEDIATELY_TIME_SYNC_PATCH
static gboolean
_nf_rec_audio_handoff_immediately( guint ch, gpointer frame )
{
	gint                clen, ret, stream_id;
	GstNfBuddyBuffer        *gst_buf;
	ICODEC_HEADER       *pheader= (ICODEC_HEADER *)frame;

	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	#if 0
		clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );
	#else
		clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_NET_INS_DEF_SPEED , 64 );
	#endif

	gst_buf = (GstNfBuddyBuffer *)gst_nf_buddy_buffer_new_and_alloc (clen, NULL);

	#ifdef ENABLE_GST_BUFFER_FAILED_SKIP 
		if(gst_buf == NULL)
		{    
			g_warning("%s GST_BUFFER_FAILED ch[%d] size[%d]", __FUNCTION__,ch, clen);
			return 0;
		}    
	#else
		g_assert(gst_buf != NULL);
	#endif

	GST_BUFFER_SIZE(gst_buf) = (guint)clen; 
	memcpy( GST_BUFFER_DATA(gst_buf), frame, (sizeof(ICODEC_HEADER)+(NF_AUDIO_NET_INS_DEF_SPEED)) );

	pheader = (ICODEC_HEADER *)GST_BUFFER_DATA(gst_buf);
	pheader->gst_buffer = gst_buf;
	pheader->chan = (guchar)ch;
	gst_buf->frame = pheader;

	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_HANDOFF] & 1<<ch)
		dump_icodec_header("handoff", pheader);

	#if 0
		g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] ", __FUNCTION__ , 
						pheader->chan, 
						pheader->flags, 
						pheader->frame_type, 
						pheader->frame_rate, 
						pheader->resolution,
						pheader->timestamp,
						pheader->timestampl,
						pheader->frame_size	 );
	#endif

	NF_OBJECT_LOCK(_nf_rec_audio);

	if(_nf_rec_audio->handoff_func_immediately)
		_nf_rec_audio->handoff_func_immediately(gst_buf);
	
	NF_OBJECT_UNLOCK(_nf_rec_audio);

	gst_buffer_unref((GstBuffer *)gst_buf);
			
	return 1;
}
#endif

#if defined(ENABLE_REC_LIVE_AUDIO)
static gboolean	
_nf_rec_audio_live_put_frame(gpointer frame , guint size)
{
	guchar *data=NULL;
	gint delay=0;
	guint aud_data=0, curr_ch=0, index=0;
	NF_REC_AUDIO_DATA *rec_aud_data=&_nf_rec_audio->rec_aud_data;
	static guint prev_ch=0;
	gboolean is_wabra=FALSE;

	curr_ch=rec_aud_data->live_ch;

	g_return_val_if_fail( frame != NULL, FALSE);
	g_return_val_if_fail( ((curr_ch > NUM_AUDIO)) || (curr_ch != 0xff), FALSE);

	if(nf_network_get_webra_audio_status())
		return FALSE;

	if(curr_ch == NF_REC_AUDIO_DAC_PLAYBACK)
	{
		prev_ch=NF_REC_AUDIO_DAC_PLAYBACK;
		return FALSE;
	}

	#if defined(_HDI_0412)
	if(rec_aud_data->in_ch[curr_ch])		// SDI Cam
		curr_ch += NUM_ACTIVE_CH;
	else									// Rear
		curr_ch=curr_ch;
	#else
		curr_ch=curr_ch;
	#endif

	nf_dev_audio_dsp_get_odelay(FALSE, &delay);
	
	if((curr_ch != prev_ch) || (delay >= NF_REC_AUDIO_LIMIT_PB_DELAY))
	{
		nf_dev_audio_dsp_reset(FALSE);
		
		if(delay >= NF_REC_AUDIO_LIMIT_PB_DELAY)
			g_message("%s DSP Reset For PlayBack Delay --> %d", __FUNCTION__, delay);
	}

#if 0
	g_message("%s called.. live_ch[%d] size[%d] pb_delay[%dByte]", __FUNCTION__, curr_ch, size, delay);
#endif

#if 0
	data=(guchar *)frame;
	for(aud_data=0; aud_data<size; aud_data+=NF_AUDIO_DEF_CHANNEL)
		write(_nf_rec_audio->audio_fd_write, data+(aud_data+curr_ch), 1);
#else
	data = g_malloc0( size / NF_AUDIO_DEF_CHANNEL);

	for(aud_data=0, index=0; aud_data<size; aud_data+=NF_AUDIO_DEF_CHANNEL, index++)
		data[index]=*(guchar *)(frame+(aud_data+curr_ch));

	#if 0
		for(aud_data=0; aud_data<(size/NF_AUDIO_DEF_CHANNEL); aud_data++)
			write(_nf_rec_audio->audio_fd_write, data, 1);
	#else
		write(_nf_rec_audio->audio_fd_write, data, size/NF_AUDIO_DEF_CHANNEL);
	#endif

	g_free(data);
#endif

	prev_ch=curr_ch;

	return TRUE;
}
#endif

static gboolean
_nf_rec_audio_sst_put_frame( guint ch, gpointer frame )
{
	gint				clen, ret, stream_id;	
	GstNfBuddyBuffer 		*gst_buf;
	ICODEC_HEADER		*pheader = (ICODEC_HEADER *)frame;

	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );	


	{
		GTimeVal ftval = { (glong)pheader->timestamp, pheader->timestampl*5*1000};
		GTimeVal ctval = _nf_rec_audio->timestamp[ch];
							
		guint64	ftmp, ctmp, diff;
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
		
		if( diff > 1100000000/5 ) // 1.1��
		{
			_nf_rec_audio->timestamp[ch].tv_sec = 0;
			_nf_rec_audio->timestamp[ch].tv_usec = 0;
			
			g_warning("%s ftval[%ld.%06ld] ctval[%ld.%06ld] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);

			// ���� �ð����� ����ȭ �� �� �̱� ������ 
			// �̹� �������� ������ �Ѵ�.
			if( c == 'C' )
				return 0; 
		}
					
#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_DIFF] & (1 << ch) )
			g_message("%s ftval[%ld.%06ld] ctval[%ld.%06ld] diff(%4lld)ms %c", __FUNCTION__,
						ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
						(diff>>20), c);
#endif

	}
	

	gst_buf = (GstNfBuddyBuffer *)gst_nf_buddy_buffer_new_and_alloc (clen, NULL);

#ifdef ENABLE_GST_BUFFER_FAILED_SKIP 
	if(gst_buf == NULL)
	{
		g_warning("%s GST_BUFFER_FAILED ch[%d] size[%d]", __FUNCTION__, ch, clen);

		// ������ ���� �ð�ó���� ����
		if( _nf_rec_audio->timestamp[ch].tv_sec != 0 )
		{
			g_time_val_add(&_nf_rec_audio->timestamp[ch], 1020000);
		}else{
			_nf_rec_audio->timestamp[ch].tv_sec = (glong)pheader->timestamp;
			_nf_rec_audio->timestamp[ch].tv_usec = pheader->timestampl*5*1000;				
		}
		return 0;
	}
#else
	g_assert(gst_buf != NULL);	
#endif
	
	GST_BUFFER_SIZE(gst_buf) = (guint)clen;	
	memcpy( GST_BUFFER_DATA(gst_buf), frame, (sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED) );		

	pheader = (ICODEC_HEADER *)GST_BUFFER_DATA(gst_buf);		
	pheader->gst_buffer = gst_buf;
	pheader->chan = (guchar)ch;
	pheader->flags = _nf_rec_audio->rec_param.rec_reason[ch];
	pheader->frame_type = NF_FRAME_TYPE_AUDIO;
	
	gst_buf->frame = pheader;
	
	if( _nf_rec_audio->timestamp[ch].tv_sec != 0 )
	{
		g_time_val_add(&_nf_rec_audio->timestamp[ch], 1020000);

		pheader->timestamp 	= (guint)_nf_rec_audio->timestamp[ch].tv_sec;
		pheader->timestampl = (guchar)(_nf_rec_audio->timestamp[ch].tv_usec/1000/5);

	}else{
		_nf_rec_audio->timestamp[ch].tv_sec = (glong)pheader->timestamp;
		_nf_rec_audio->timestamp[ch].tv_usec = pheader->timestampl*5*1000;
	}

#ifdef DEBUG_REC_AUDIO_LOG
	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_SST] & (1 << ch) )
		dump_icodec_header("sst_put", pheader);
#endif

	if( _nf_rec_audio->stream_id[ch] >= 0)
	{	
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = gst_buffer_ref((GstBuffer *)gst_buf);
		if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
		ret = sst_record_put_frame( _nf_rec_audio->stream_id[ch], GST_BUFFER_DATA(gst_buf) );
		if(ret){
			dump_icodec_header("ERR frame", pheader);
			g_message("sst_record_put_frame result[%d](%s)\n", ret, sst_get_error_string(ret));
	        gst_buffer_unref((GstBuffer *)gst_buf);
		}	
	}
	
	gst_buffer_unref((GstBuffer *)gst_buf);
	
	return 1;
}

static gboolean
_nf_rec_audio_handoff_gst_buffer( guint ch, gpointer frame )
{
	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	gint				clen, ret, stream_id;		
	GstNfBuddyBuffer 	*gst_buf = (GstNfBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = (ICODEC_HEADER *)GST_BUFFER_DATA( gst_buf );
	
	
	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_HANDOFF] & 1<<ch)
		dump_icodec_header("handoff", pheader);

	NF_OBJECT_LOCK(_nf_rec_audio);
	if(_nf_rec_audio->handoff_func)
		_nf_rec_audio->handoff_func(gst_buf);
	NF_OBJECT_UNLOCK(_nf_rec_audio);


	return 1;
}


static gboolean
_nf_rec_audio_sst_put_gst_buffer( guint ch, gpointer frame )
{
	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	gint				clen, ret, stream_id;	
	GstNfBuddyBuffer 	*gst_buf = (GstNfBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = (ICODEC_HEADER *)GST_BUFFER_DATA( gst_buf );

	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );	


	{
		GTimeVal ftval = { (glong)pheader->timestamp, pheader->timestampl*5*1000};
		GTimeVal ctval = _nf_rec_audio->timestamp[ch];
							
		guint64	ftmp, ctmp, diff;
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
		
		if( diff > 1100000000/5 ) // 1.1��
		{
			_nf_rec_audio->timestamp[ch].tv_sec = 0;
			_nf_rec_audio->timestamp[ch].tv_usec = 0;
			
			g_warning("%s ftval[%ld.%06ld] ctval[%ld.%06ld] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);

			// ���� �ð����� ����ȭ �� �� �̱� ������ 
			// �̹� �������� ������ �Ѵ�.
			if( c == 'C' )
				return 0; 

		}
					
#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_DIFF] & (1 << ch) )
			g_message("%s ftval[%ld.%06ld] ctval[%ld.%06ld] diff(%4lld)ms %c", __FUNCTION__,
						ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
						(diff>>20), c);
#endif

	}
			
	if( _nf_rec_audio->timestamp[ch].tv_sec != 0 )
	{
		g_time_val_add(&_nf_rec_audio->timestamp[ch], 1020000);

		pheader->timestamp 	= (guint)_nf_rec_audio->timestamp[ch].tv_sec;
		pheader->timestampl = (guchar)(_nf_rec_audio->timestamp[ch].tv_usec/1000/5);

	}else{
		_nf_rec_audio->timestamp[ch].tv_sec = (glong)pheader->timestamp;
		_nf_rec_audio->timestamp[ch].tv_usec = (guchar)(pheader->timestampl*5*1000);
	}

	// header FIXME!!
	pheader->flags = _nf_rec_audio->rec_param.rec_reason[ch];
	pheader->frame_rate = NF_FPS_CR01;


	if( _nf_rec_audio->stream_id[ch] < 0)
	{
		NF_REC_AUDIO_PARAM	*p_new = &_nf_rec_audio->rec_param;
		
		if( p_new->ch_arr[ch] != 0xff
				&& ( p_new->pre_rec_time[ch] > 0 
					||  p_new->rec_reason[ch] != NF_RECORD_REASON_NOTHING ) && nf_record_is_rec_off() != TRUE )
		{
			
			gint stream_id = -1;
			/* �̹� ���� �ִ� stream_id�� ������ g_assert(); */
																
			stream_id = sst_record_open( ch + NF_AUDIO_SST_START_CH_NO,	// ch
										p_new->pre_rec_time[ch],			// pre_rec_time
										p_new->rec_reason[ch],			// rec_reason
										NF_CODEC_TYPE_URAW,				// codec
										0,								// resolution
										NF_FPS_CR01,					// frame rate
										0);								// quality
													
			g_message("sst_record_open  ch[%2d] sid[%2d] reason[%d] pre[%d]codec[%d]res[%d]fps[%2d]q[%d]", 
						ch + NF_AUDIO_SST_START_CH_NO, stream_id ,
						p_new->rec_reason[ch],
						p_new->pre_rec_time[ch],
						NF_CODEC_TYPE_URAW,
						0,
						NF_FPS_CR01,
						0 );
																		
			if( stream_id <0)
			{
				g_message("sst_record_open result[%d](%s)", stream_id, sst_get_error_string(stream_id)); 	
			}
			g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� g_assert
			_nf_rec_audio->stream_id[ch] = stream_id;	
		}
	}


#ifdef DEBUG_REC_AUDIO_LOG
	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_SST] & (1 << ch) )
		dump_icodec_header("sst_put", pheader);
#endif

	if( _nf_rec_audio->stream_id[ch] >= 0)
	{							
		{
			void *tmp_gst_ret = NULL;
			tmp_gst_ret = gst_buffer_ref((GstBuffer *)gst_buf);
			if(tmp_gst_ret == NULL)
				fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
		}
		ret = sst_record_put_frame( _nf_rec_audio->stream_id[ch], GST_BUFFER_DATA(gst_buf) );
		if(ret){
			dump_icodec_header("ERR frame", pheader);
			g_message("sst_record_put_frame result[%d](%s)\n", ret, sst_get_error_string(ret));
	        gst_buffer_unref(gst_buf);
		}	
	}
			
	return 1;
}



//  rec_audio_thread_func (async_queue, GINT_TO_POINTER (id)); 
//	
static void
rec_audio_thread_put_func (NfRecAudio * self) // put/handoff thread
{
	gpointer	que_poped_data = NULL;
	
	gchar 		buffs[NF_AUDIO_DEF_CHANNEL][ sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED ];	
	ICODEC_HEADER		*pheader[4];
	gchar				*frame_data[4], *que_data;	
	gint		i, max_i, ret;	
	#if defined(NF_REC_NET_AUDIO_DATA_DUMP)
		static FILE *fp=NULL;
	#endif

	g_message("%s start", __FUNCTION__);

    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
        policy = SCHED_FIFO;
        thread = pthread_self();

#if PRI_ADJUST
		sched.sched_priority = sched_get_priority_max(policy)-2;
#else		
		sched.sched_priority = sched_get_priority_max(policy)-1;
#endif

		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }
    						
	// wait init complete
	while( _nf_rec_audio == NULL ) g_usleep(10*1000);

	#if defined(NF_REC_AUDIO_DATA_DUMP)
		fp=fopen(NF_REC_AUDIO_DATA_DUMP_FILE_NAME, "w");
		if(fp == NULL)
			g_warning("%s Line[%d] File Open Fail!! filename[%s]", __FUNCTION__, __LINE__, NF_REC_AUDIO_DATA_DUMP_FILE_NAME);
		else
			g_message("%s Line[%d] %s Open!!", __FUNCTION__, __LINE__, NF_REC_AUDIO_DATA_DUMP_FILE_NAME);
	#endif

#ifndef ENABLE_REC_AUDIO_DSP_FD	
	self->init_done = 1;
	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		self->init_done_alarm = 1;
	#endif
#endif	

#ifdef ENABLE_WATCHDOGxx
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_RECORD_AUDIO, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
#endif
	
	while(self->thread_put_run)
	{				
		NF_AUDIO_QITEM	*pqitem = NULL;
				
		que_poped_data = g_async_queue_pop( self->queue);
		if( que_poped_data == NULL)	
			continue;

#ifdef ENABLE_WATCHDOG
		nf_watchdog_kick( NF_WATCHDOG_MEMBER_RECORD_AUDIO );
#endif		

		pqitem = ( NF_AUDIO_QITEM *)que_poped_data;
			
		if( pqitem->type == NF_AUDIO_QITEM_TYPE_FRAME )
		{				

			// noting
			g_warning("%s no item handler!!", __FUNCTION__);
						
		}else if( pqitem->type == NF_AUDIO_QITEM_TYPE_GST_BUFFER ){

			GstBuffer *buffer = (GstBuffer *)pqitem->buff;
			ICODEC_HEADER *pheader_single = (ICODEC_HEADER *)GST_BUFFER_DATA(buffer);
			guint chan = pheader_single->chan;
			
#ifdef DEBUG_REC_AUDIO_LOG
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_HEX] )
				nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
#endif

			if( self->handoff_func && self->handoff_ch_mask & (guint)(1<<chan) )
					_nf_rec_audio_handoff_gst_buffer( chan, buffer);
						
			if( self->rec_param.ch_arr[chan] != 0xff 
					&& ( self->rec_param.pre_rec_time[chan] >0 
						|| self->rec_param.rec_reason[chan] != NF_RECORD_REASON_NOTHING ) )
				_nf_rec_audio_sst_put_gst_buffer( chan, buffer );
											
		}else{	// COMMAND
				

			NF_REC_AUDIO_PARAM	*p_new = (NF_REC_AUDIO_PARAM *)pqitem->buff;
			NF_REC_AUDIO_PARAM	*p_cur = &self->rec_param;
			
#ifdef DEBUG_REC_AUDIO_LOG
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_CMD] )
				dump_rec_audio_cmd("audio_rec_tbl", p_new);
#endif		

//			p_new->ch_arr[i], p_new->rec_reason[i], 
//			p_new->pre_rec_time[i], p_new->pre_rec_close[i]	

			for(i=0; i<	NUM_ACTIVE_CH; ++i)
			{
				if( self->stream_id[i]>=0 &&
						(p_new->ch_arr[i] == 0xff
						|| ( p_new->rec_reason[i] == NF_RECORD_REASON_NOTHING )
						|| ( p_cur->rec_reason[i] 	!=  p_new->rec_reason[i] )					
						|| ( p_cur->pre_rec_time[i] !=  p_new->pre_rec_time[i]) )  
					)
				{					
					g_message("sst_record_close ch[%2d] sid[%2d] pre_flush[%d]",  
								i + NF_AUDIO_SST_START_CH_NO, self->stream_id[i] ,
								p_new->pre_rec_close[i]);
															
					ret = sst_record_close( self->stream_id[i], 
											(guint8)p_new->pre_rec_close[i] );
					g_assert( ret == 0);
					self->stream_id[i] = -1;
					
					self->timestamp[i].tv_sec = 0;
					self->timestamp[i].tv_usec = 0;
										
				}

#if 0				
				if( p_new->ch_arr[i] != 0xff 
						&& self->stream_id[i] < 0
						&& ( p_new->pre_rec_time[i] > 0 
							||  p_new->rec_reason[i] != NF_RECORD_REASON_NOTHING ) )
				{

					gint stream_id = -1;					
					/* �̹� ���� �ִ� stream_id�� ������ g_assert(); */
					g_assert( self->stream_id[i] < 0 );
																		
					stream_id = sst_record_open( (guint8)(i + NF_AUDIO_SST_START_CH_NO),	// ch
												(guint8)p_new->pre_rec_time[i],				// pre_rec_time
												(guint8)p_new->rec_reason[i],				// rec_reason
												(guint8)NF_CODEC_TYPE_URAW,					// codec
												(guint8)0,									// resolution
												(guint8)NF_FPS_CR01,						// frame rate
												(guint8)0);									// quality
					
					__T("sst_record_open  ch[%2d] sid[%2d] reason[%d] pre[%d]codec[%d]res[%d]fps[%2d]q[%d]", 
								i + NF_AUDIO_SST_START_CH_NO, stream_id ,
								p_new->rec_reason[i],
								p_new->pre_rec_time[i],
								NF_CODEC_TYPE_URAW,
								0,
								NF_FPS_CR01,
								0 );
																				
					if( stream_id <0)
					{
						g_message("sst_record_open result[%d](%s)", stream_id, sst_get_error_string(stream_id)); 	
					}
					g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� assert
					self->stream_id[i] = stream_id;	
				}
#endif
			
			}									
			memcpy( p_cur, p_new, sizeof(NF_REC_AUDIO_PARAM));			
		
		}	//if(pqitem->type == NF_AUDIO_QITEM_TYPE_FRAME)		
		_nf_audio_qitem_free( que_poped_data );
									
	} // while(self->thread_run)
	g_message("%s end", __FUNCTION__);
}

#include "nf_api_ipcam.h"

#define AUDIO_DIV_UNIT 			(8)
#define AUDIO_RESET_INTERVAL 	(10*60*AUDIO_DIV_UNIT)	// 10��

static volatile guint _vin_mask = 0;

static void _ipcam_send_audio(gint ch, gchar *data, guint len)
{
	static int playing[64] = {0, };
	static int send_cnt[64] = {0,};
		
	gint send_bytes = 0;
	gint ret = 0;
	
	NFIPCamAudioRaw audio;
	NFIPCamAudioRaw audio_cmd;

	g_return_if_fail ( ch < 64);
	
	if( len == 0 ) // close condition
	{		
		
		if( playing[ch] )
		{
			audio_cmd.ch = ch;
			audio_cmd.type = NF_IPCAM_SEND_AUDIO_END;
			audio_cmd.buf = NULL;
			nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
			playing[ch] = 0; 
			send_cnt[ch] = 0;
		}
				
		return;	
	}
				
	// for start cmd
	if( !playing[ch] ){
		audio_cmd.ch = ch;
		audio_cmd.type = NF_IPCAM_SEND_AUDIO_START;
		audio_cmd.buf = NULL;
		ret = nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
		if( ret == 1) {
			playing[ch] = 1; 
			send_cnt[ch] = 0;
		}else{
			send_cnt[ch] = 0;
			return;
		}
	}
		
	audio.buf = gst_buffer_new_and_alloc(len);			
	g_return_if_fail ( audio.buf != NULL);
			
	// data
	audio.ch = ch;
	audio.type = NF_IPCAM_SEND_AUDIO_DATA;	
	memcpy(GST_BUFFER_DATA(audio.buf), data, len);		
	ret = nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL);

	if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_MICOUT] )
		g_message("%s ch[%d] len[%d] ret[%d]", __FUNCTION__, ch, len, ret);
	
	if( ++send_cnt[ch] > AUDIO_RESET_INTERVAL || ret != 1 ) {
		// for end cmd
		audio_cmd.ch = ch;
		audio_cmd.type = NF_IPCAM_SEND_AUDIO_END;
		audio_cmd.buf = NULL;
		nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
		playing[ch] = 0;
		send_cnt[ch] = 0;
	}
		
out:
	if ( audio.buf )
		gst_buffer_unref(audio.buf);
				
	return;
}			

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
void nf_rec_aud_send_to_ipcam(gint ch, gchar *data, guint len)
{
	_ipcam_send_audio(ch, data, len);
}
#endif

static void
vloss_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);	
	
	_vin_mask = ~(pinfo->d.params[0]);
}

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
static void
ai_event_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int *p=NULL;
	ai_rule_event_t *pevt;

	g_return_if_fail(pinfo != NULL);

	NF_OBJECT_LOCK(_nf_rec_audio);

	p = pinfo->p.ptr;
	pevt=(p + 2);

	extern int get_ai_enable(int chan);		// nf_action.c
	if(!get_ai_enable(p[0])) {
		NF_OBJECT_UNLOCK(_nf_rec_audio);
		return;
	}

	#define     VCA_MAX_ELEMS   MAX(IVCA_MAX_ZONES, IVCA_MAX_CNTRS)
	g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

	_nf_rec_audio->rec_aud_data_ai.ai_evt_cnt=p[1];
	nf_rec_aud_ai_evt_check(&_nf_rec_audio->rec_aud_data_ai, pevt);

	#if 0
		g_message("%s line%d ch%d [0x%08x][0x%08x]", __FUNCTION__, __LINE__, pevt->ch, *mask_ai_evt,
					_nf_rec_audio->rec_aud_data_ai.mask_ai_evt[pevt->ch]);
	#endif

	NF_OBJECT_UNLOCK(_nf_rec_audio);
}
#endif

//#define NF_REC_AUDIO_DATA_DUMP
#if defined(NF_REC_AUDIO_DATA_DUMP)
	#define NF_REC_AUDIO_DATA_DUMP_FILE_NAME     "/dump.raw"
#endif

static void
rec_audio_thread_func (NfRecAudio * self)  // read thread
{		
	#if defined(NF_REC_AUDIO_DATA_DUMP)
		FILE* fp=NULL;
	#endif
	gint 		size, ret, i;
	gint		is_aout[NUM_ACTIVE_CH] = {0,};
	
	
	NF_AUDIO_QITEM *pqitem = NULL;
		
	g_message("%s start", __FUNCTION__);

	#if defined(NF_REC_AUDIO_DATA_DUMP)
		fp=fopen(NF_REC_AUDIO_DATA_DUMP_FILE_NAME, "w");
		if(fp == NULL)
			g_warning("%s Line[%d] File Open Fail!! filename[%s]", __FUNCTION__, __LINE__, NF_REC_AUDIO_DATA_DUMP_FILE_NAME);
		else
			g_message("%s Line[%d] %s Opend!!", __FUNCTION__, __LINE__, NF_REC_AUDIO_DATA_DUMP_FILE_NAME);
	#endif

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

		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }

	#if 0
		g_message("%s default speed[%d] num_ch[%d] bits[%d]", __FUNCTION__, 
					NF_AUDIO_DEF_SPEED,
					NF_AUDIO_DEF_CHANNEL,
					NF_AUDIO_DEF_RX_BIT	);

		size = NF_AUDIO_DEF_SPEED * NF_AUDIO_DEF_CHANNEL
					* NF_AUDIO_DEF_MONO * (NF_AUDIO_DEF_RX_BIT/8);
	#else

		size = 8000/AUDIO_DIV_UNIT;
	#endif

	// wait init complete
	while( _nf_rec_audio == NULL ) g_usleep(10*1000);
	
	self->init_done = 1;

	pqitem = _nf_audio_qitem_new(size);		
	g_assert(pqitem != NULL);
																		
	while(self->thread_run)
	{	
		
		pqitem->type = NF_AUDIO_QITEM_TYPE_FRAME;
		
		//g_get_current_time( &pqitem->req_timeval);	
		gettimeofday( (struct timeval *)&pqitem->req_timeval, NULL );	
		ret = Readn( (int)self->audio_fd, pqitem->buff, (size_t)size);
		if(ret != size)
		{
			g_warning("%s /dev/dsp fd[%d]   Readn ret[%d] size[%d]", __FUNCTION__,
							self->audio_fd, ret, size);
																												
			g_usleep(1000000); //1sec
			continue;
		}

		#if defined(ENABLE_REC_LIVE_AUDIO)
			_nf_rec_audio_live_put_frame(pqitem->buff, size);
		#endif

		#if 0
			#if !defined(ENABLE_AUD_HI_CHIP)
				// Hdmi Audio For HI3798C
				{
					extern gboolean nf_hi_aud_send_frame_out(gpointer frame, gint size);
					nf_hi_aud_send_frame_out(pqitem->buff, size);
				}
			#endif
		#endif

		#if defined(NF_REC_AUDIO_DATA_DUMP)
			if(fp != NULL)
			{
				if(fwrite(pqitem->buff, size, 1, fp) != 1)
				{
					g_warning("%s Line[%d] File Write Error!!", __FUNCTION__, __LINE__);
				}
			}
		#endif
		
		//g_get_current_time( &pqitem->ret_timeval);
		gettimeofday( (struct timeval *)&pqitem->ret_timeval, NULL);

#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_GET] )
			g_message("%s req_sec[%ld.%06ld] ret_sec[%ld.%06ld]  ", __FUNCTION__,
					pqitem->req_timeval.tv_sec, pqitem->req_timeval.tv_usec, 
					pqitem->ret_timeval.tv_sec, pqitem->ret_timeval.tv_usec );

		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_GET_HEX] )
			nf_debug_hexdump( pqitem->buff, 0x10);
#endif


/*
util_dev-Message: nf_dev_mic_set_onoff on[1]
util_dev-Message: nf_dev_mic_set_output_mask mask[00000001]

_ipcam_send_audio( 0, pqitem->buff, size);
*/		
		if (nf_sysman_qcmode_is_enable() == 1) {
			if (nf_sysman_get_qc_live_audio() == 1) {
				write(_nf_rec_audio->audio_fd_write, pqitem->buff, (size_t)size);
			}
		} else {
			#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
				guint mask_ai_evt_test=_nf_rec_audio->rec_aud_data_ai.mask_ai_evt_test;
			#endif

			if( nf_dev_mic_get_onoff() )
			{
				guint aout_mask = nf_dev_mic_get_output_mask();

				for(i=0;i<NUM_ACTIVE_CH; ++i)
				{
					#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
						gboolean is_ai_aud_playing=self->rec_aud_data_ai.is_playing[i];
				
						if(is_ai_aud_playing) {
							#if 0
								g_message("%s line%d CH[%d] AI Event Audio Playing!! Mic On!!", __FUNCTION__, __LINE__, i);
							#endif
							aout_mask &= (guint)~(1 << i);
						}
					#endif
					
					if( aout_mask & (guint)(1 << i) && _vin_mask & (guint)(1 << i) )
						_ipcam_send_audio( i, pqitem->buff, (guint)size);
					else  {
						#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
							if(!is_ai_aud_playing) {
								_ipcam_send_audio( i, NULL, 0);	// aout end 
							}
						#else
							_ipcam_send_audio( i, NULL, 0);	// aout end 
						#endif
					}
				}
			}else{
				for(i=0;i<NUM_ACTIVE_CH; ++i) {
					
					#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
						gboolean is_ai_aud_playing=self->rec_aud_data_ai.is_playing[i];

						if(is_ai_aud_playing) {
							#if 0
								g_message("%s line%d CH[%d] AI Event Audio Playing!! Mic Off!!", __FUNCTION__, __LINE__, i);
							#else
								;
							#endif
						}
						else
							_ipcam_send_audio( i, NULL, 0);// aout end
					#else
						_ipcam_send_audio( i, NULL, 0);// aout end
					#endif
				}
			}
		}
	}
			
	g_message("%s end", __FUNCTION__);
	g_thread_exit(0);
}

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
static void
rec_audio_alarm_thread_func (NfRecAudio *self)  // alarm notify func
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

		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }

	size=(8000 / AUDIO_DIV_UNIT);

	// wait init complete
	while( _nf_rec_audio == NULL ) g_usleep(10*1000);

	self->init_done_alarm = 1;

	self->rec_aud_data_ai.ai_aud_size_send=size;
	while(self->thread_run_alarm) {

		NF_OBJECT_LOCK(_nf_rec_audio);

		nf_rec_aud_ai_send_frame(&self->rec_aud_data_ai);

		NF_OBJECT_UNLOCK(_nf_rec_audio);
		g_usleep(100000);
	}

	g_message("%s end", __FUNCTION__);
	g_thread_exit(0);
}

void nf_rec_audio_alarm_test(guint mask_alarm_ch, char *str)
{
	gchar *s=NULL;

	g_message("%s line%d ch%d str[%s]\n", __FUNCTION__, __LINE__, mask_alarm_ch, str);
	_nf_rec_audio->rec_aud_data_ai.mask_ai_evt_test |= mask_alarm_ch;
	s=_nf_rec_audio->rec_aud_data_ai.ai_evt_aud_test_filename;
	sprintf(s, "/opt/%s", str);

	_nf_rec_audio->rec_aud_data_ai.ai_evt_aud_test=TRUE;
}
#endif

/**
	@brief				rec_audio �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_rec_audio_init(int wait)
{
	gboolean ret = TRUE;			
	gulong	cb_handle;
	
	g_return_val_if_fail (_nf_rec_audio == NULL, FALSE);	
	
	g_message("%s called!!", __FUNCTION__);
	
	_nf_rec_audio = g_object_new ( NF_TYPE_REC_AUDIO , NULL);
	
	nf_debug_category_add( "recaudio", _DEBUG_REC_AUDIO_str, _DEBUG_REC_AUDIO_log, DEBUG_REC_AUDIO_IDX_NR);


	_vin_mask = ~(nf_notify_get_param0("vloss"));
		
	cb_handle= nf_notify_connect_cb( "vloss", vloss_notify_cb_func , (gpointer)NULL);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
		cb_handle= nf_notify_connect_cb( "ai_event", ai_event_notify_cb_func , (gpointer)NULL);
		g_message("%s ai_event connect_cb[%ld]",__FUNCTION__, cb_handle );
		g_assert(cb_handle >0);
	#endif

	if( wait )
	{
		while( _nf_rec_audio->init_done != 1)
			g_usleep(10*1000);
		#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
			while( _nf_rec_audio->init_done_alarm != 1)
				g_usleep(10*1000);
		#endif
	}
	
	return ret;
}


/**
	@brief				rec_audio handoff ���
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_rec_audio_register_handoff(guint ch_mask, NF_REC_AUDIO_HANDOFF_FUNC handoff_func )
{

	gint i;
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	
#if 0
	g_return_val_if_fail (ch_mask != 0, 0);	
	g_return_val_if_fail (handoff_func != NULL, 0);	
#endif	
	g_message("%s ch_mask[0x%x] handoff_fnx[%p]", __FUNCTION__, ch_mask, handoff_func);

	//NF_OBJECT_LOCK(_nf_rec_audio);

	if (handoff_func != _nf_rec_audio->handoff_func)
	{	
		_nf_rec_audio->handoff_func = handoff_func;
	}
	_nf_rec_audio->handoff_ch_mask = ch_mask;
	
	//NF_OBJECT_UNLOCK(_nf_rec_audio);

	return 1;
}

#if defined(ENABLE_REC_NET_IMMEDIATELY)
/**
	@brief				rec_audio handoff immediately
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_rec_audio_register_handoff_immediately(guint ch_mask, NF_REC_AUDIO_HANDOFF_FUNC_IMMEDIATELY handoff_func )
{
	gint i;
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	
#if 0
	g_return_val_if_fail (ch_mask != 0, 0);	
	g_return_val_if_fail (handoff_func != NULL, 0);	
#endif	
	g_message("%s ch_mask[0x%x] handoff_immediately_fnx[%p]", __FUNCTION__, ch_mask, handoff_func);

	NF_OBJECT_LOCK(_nf_rec_audio);
	
	_nf_rec_audio->handoff_func_immediately = handoff_func;
	_nf_rec_audio->handoff_ch_mask = ch_mask;
	
	NF_OBJECT_UNLOCK(_nf_rec_audio);

	return 1;
}
#endif

/**
	@brief				rec_audio ���ڵ� �Ķ���� ���� & ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_rec_audio_start(NF_REC_AUDIO_PARAM *param)
{
	gint i;
	NF_AUDIO_QITEM *pqitem = NULL;				

	g_return_val_if_fail (_nf_rec_audio != NULL, 0);
	g_return_val_if_fail (param != NULL, 0);

	for(i=0;i<NUM_ACTIVE_CH;++i)	
	{
/*
		g_return_val_if_fail (param->ch_arr[i] < NF_AUDIO_INPUT_CHNR || 
								param->ch_arr[i] == NF_AUDIO_INPUT_OFF , 0);
																
		g_return_val_if_fail (param->pre_rec_time[i] <= MAX_PRE_REC_TIME, 0);
*/		
		g_return_val_if_fail (param->rec_reason[i] <= NF_RECORD_REASON_PRE, 0);
	}

	pqitem = _nf_audio_qitem_new( sizeof(NF_REC_AUDIO_PARAM) );		
	g_assert(pqitem != NULL);

	pqitem->type = NF_AUDIO_QITEM_TYPE_CMD;

	memcpy(pqitem->buff, param, sizeof(NF_REC_AUDIO_PARAM));
	
	g_async_queue_push( _nf_rec_audio->queue, pqitem );

#if 0	
	NF_OBJECT_LOCK(_nf_rec_audio);
	memcpy( &_nf_rec_audio->rec_param, param, sizeof(NF_REC_AUDIO_PARAM));
	NF_OBJECT_UNLOCK(_nf_rec_audio);
#endif
	return 1;
}

/**
	@brief				rec_audio ���ڵ� �Ķ���� ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_rec_audio_change(NF_REC_AUDIO_PARAM *param)
{
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);
	g_return_val_if_fail (param != NULL, 0);	

	return nf_rec_audio_start(param);
}


/**
	@brief				rec_audio ���ڵ� ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_rec_audio_stop()
{
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	
	
	NF_REC_AUDIO_PARAM param;
	
	memset( &param, 0x00, sizeof(NF_REC_AUDIO_PARAM));
	memset( param.ch_arr, NF_AUDIO_INPUT_OFF, sizeof(param.ch_arr));

	return nf_rec_audio_start( &param);
	
#if 0		
	NF_OBJECT_LOCK(_nf_rec_audio);
	memset( &_nf_rec_audio->rec_param, 0x00, sizeof(NF_REC_AUDIO_PARAM));
	memset( _nf_rec_audio->rec_param.ch_arr, NF_AUDIO_INPUT_OFF, 
										sizeof(_nf_rec_audio->rec_param.ch_arr));
	NF_OBJECT_UNLOCK(_nf_rec_audio);
#endif
				
	return 1;
}


gboolean nf_rec_audio_put_frame(gint ch_num, gpointer frame )
{
	
	GstBuffer *buffer = (GstBuffer *)frame;
	ICODEC_HEADER *pheader = (ICODEC_HEADER *)GST_BUFFER_DATA(buffer);
	NF_AUDIO_QITEM *pqitem = NULL;				
	
	g_return_val_if_fail (_nf_rec_audio != NULL, FALSE);		
	g_return_val_if_fail (frame != 0, FALSE);
	g_return_val_if_fail (ch_num >= 0 && ch_num < NUM_ANALOG_CHANNEL, FALSE);

#ifdef ENABLE_PUT_FRAME_MAX_QUEUE
	while( g_async_queue_length( _nf_rec_audio->queue ) > (16*5) ) g_usleep(10*1000);
#endif
	
	if ( pheader->frame_type != NF_FRAME_TYPE_AUDIO ) {
		dump_icodec_header("INV_AUDIO", pheader);
		return -1;
	}

#ifdef DEBUG_REC_AUDIO_LOG
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI] )
				dump_icodec_header("AUDIO", pheader);
			
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI_HEX] )
				nf_debug_hexdump( GST_BUFFER_DATA(buffer) , 0x100);
#endif

	
	pqitem = _nf_audio_qitem_new_gst_buffer( buffer );	// internal ref +
	g_return_val_if_fail (pqitem != 0, FALSE);
		
	g_async_queue_push( _nf_rec_audio->queue, pqitem );
	
	gst_buffer_unref(buffer);
	
	return 1;
}

static gboolean _nf_rec_aud_sysdb_chg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	gint aud_ch=0;
	#if defined(ENABLE_REC_LIVE_AUDIO)
		NF_REC_AUDIO_DATA *rec_aud_data=&_nf_rec_audio->rec_aud_data;
		gchar tmp_key[256]={0, };
	#endif

	g_return_val_if_fail(pinfo != NULL, FALSE);
	g_return_val_if_fail(_nf_rec_audio != NULL, FALSE);
	
	#if defined(ENABLE_REC_LIVE_AUDIO)
		if(pinfo->d.params[0] == NF_SYSDB_CATE_AUDIO)
		{
			NF_OBJECT_LOCK(_nf_rec_audio);
			for(aud_ch=0; aud_ch<NUM_AUDIO; aud_ch++)
			{
				sprintf(tmp_key, "audio.C%d.in_dev", aud_ch);
				rec_aud_data->in_ch[aud_ch]=nf_sysdb_get_uint(tmp_key);
			}
			NF_OBJECT_UNLOCK(_nf_rec_audio);
		}
	#endif

	return TRUE;
}

#if defined(ENABLE_REC_LIVE_AUDIO)
static void nf_rec_audio_live_data_load(void)
{
	gchar tmp_key[256];
	gint ch_aud=0;
	
	g_message("%s called", __FUNCTION__);

	_nf_rec_audio->rec_aud_data.live = nf_sysdb_get_uint("audio.live");
	_nf_rec_audio->rec_aud_data.live_ch = nf_sysdb_get_uint("audio.ch");
	_nf_rec_audio->rec_aud_data.tx = nf_sysdb_get_bool("audio.tx");
	_nf_rec_audio->rec_aud_data.rx = nf_sysdb_get_bool("audio.rx");

	for(ch_aud=0; ch_aud<NUM_AUDIO; ch_aud++)
	{
		sprintf(tmp_key, "audio.C%d.in_dev", ch_aud);
		_nf_rec_audio->rec_aud_data.in_ch[ch_aud] = nf_sysdb_get_uint(tmp_key);
	}
}

static gboolean _nf_rec_covert_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_val_if_fail(pinfo != NULL, FALSE);
	g_return_val_if_fail(_nf_rec_audio != NULL, FALSE);

	g_message("%s called", __FUNCTION__);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM)
	{
	NF_OBJECT_LOCK(_nf_rec_audio);
		g_message("%s param1[0x%08x]", __FUNCTION__, pinfo->d.params[1]);
		_nf_rec_audio->covert_mask=pinfo->d.params[1];
	NF_OBJECT_UNLOCK(_nf_rec_audio);
	}

	return TRUE;
}

void nf_rec_set_live_audio_ch(guint ch)
{
	NF_OBJECT_LOCK(_nf_rec_audio);
	_nf_rec_audio->rec_aud_data.live_ch=ch;
	NF_OBJECT_UNLOCK(_nf_rec_audio);
}

#endif

#if defined(NF_REC_ENABLE_DSP_RESET)
static gboolean nf_rec_audio_reset_dsp(gpointer data)
{
	g_message("%s line%d", __FUNCTION__, __LINE__);

	nf_dev_audio_dsp_reset(TRUE);
	nf_dev_audio_dsp_reset(TRUE);
	
	return TRUE;
}
#endif

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

#ifdef DEBUG_REC_AUDIO_JBSHELL

// rec_audio_start		[ch] [audio_in_ch]
// rec_audio_stop

static NF_REC_AUDIO_PARAM _audio_param;

static char rec_audio_start_help[] = "rec_audio_start [ch_mask] [audio_in_ch:0~3, 0xff:off] [reason] [pre_rec] [rec_flush 0:none 1:flush]";
static int rec_audio_start(int argc, char **argv)
{	
	static gint init_param = 0;
	gint i;
	gint ch_mask, audio_in_ch;
	gint rec_reason = NF_RECORD_REASON_TIMER, pre_rec_time =0, pre_rec_close=0;
		
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	
	
	if( !init_param )
	{
		memset( &_audio_param, 0x00, sizeof(_audio_param));
		memset( _audio_param.ch_arr, NF_AUDIO_INPUT_OFF, sizeof(_audio_param.ch_arr));
								
		init_param = 1;
	}
					
	if(argc < 3){
		printf("%s\n",rec_audio_start_help);
		return -1;
	}
	
	ch_mask = strtol(argv[1],NULL,0);
	audio_in_ch = strtol(argv[2],NULL,0);
	
	if(argc > 3) rec_reason = strtol(argv[3],NULL,0);
	if(argc > 4) pre_rec_time = strtol(argv[4],NULL,0);
	if(argc > 5) pre_rec_close = strtol(argv[5],NULL,0);

	for(i=0;i<NUM_ACTIVE_CH;++i)
	{
		if( ch_mask & 1 << i)
		{
			_audio_param.ch_arr[i] = (guchar)audio_in_ch;
			_audio_param.rec_reason[i] = (guchar)rec_reason;
			_audio_param.pre_rec_time[i] = (guchar)pre_rec_time;
			_audio_param.pre_rec_close[i] = (guchar)pre_rec_close;
		}
	}			
	nf_rec_audio_start(&_audio_param);
					
	return 0;
}
__commandlist(rec_audio_start,"rec_audio_start",rec_audio_start_help, rec_audio_start_help);

static char rec_audio_stop_help[] = "rec_audio_stop";
static int rec_audio_stop(int argc, char **argv)
{	
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	

	memset( &_audio_param, 0x00, sizeof(_audio_param));
	memset( _audio_param.ch_arr, NF_AUDIO_INPUT_OFF, sizeof(_audio_param.ch_arr));
	
	nf_rec_audio_stop();						
	return 0;
}
__commandlist(rec_audio_stop,"rec_audio_stop",rec_audio_stop_help, rec_audio_stop_help);

static char rec_audio_handoff_help[] = "rec_audio_handoff [ch_mask] [enable]";
static int rec_audio_handoff(int argc, char **argv)
{	
	guint ch_mask, handoff_enable;
	
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	

	if(argc < 3){
		printf("%s\n",rec_audio_handoff_help);		
		return -1;
	}
	ch_mask = (guint)strtol(argv[1],NULL,0);
	handoff_enable = (guint)strtol(argv[2],NULL,0);

	nf_rec_audio_register_handoff( ch_mask, (handoff_enable) ? _nf_rec_audio_dummy_handoff : NULL);

	return 0;
}
__commandlist(rec_audio_handoff,"rec_audio_handoff",rec_audio_handoff_help, rec_audio_handoff_help);


static char rec_audio_tbl_help[] = "rec_audio_tbl";
static int rec_audio_tbl(int argc, char **argv)
{	
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	

	dump_rec_audio_cmd("rec_audio",  &_nf_rec_audio->rec_param);
	return 0;
}
__commandlist(rec_audio_tbl,"rec_audio_tbl",rec_audio_tbl_help, rec_audio_tbl_help);


static char rec_audio_log_help[] = "rec_audio_log [idx] [val]";
static int rec_audio_log(int argc, char **argv)
{	
	gint idx, val,i;
	
	g_return_val_if_fail( _nf_rec_audio != NULL, -1 );

	if(argc < 3){
		printf("%s\n",rec_audio_log_help);
		nf_debug_dump("recaudio");
		return -1;
	}
		
	idx = strtol(argv[1],NULL,0);
	val = strtol(argv[2],NULL,0);
	
	g_return_val_if_fail( idx <= DEBUG_REC_AUDIO_IDX_NR, -1 );
	
	if( idx == DEBUG_REC_AUDIO_IDX_NR )
	{
		for(i=0;i<DEBUG_REC_AUDIO_IDX_NR;i++)		
			_DEBUG_REC_AUDIO_log[i] = val;
	}else{
		_DEBUG_REC_AUDIO_log[idx] = val;
	}
	
	return 0;
}
__commandlist(rec_audio_log,"rec_audio_log", rec_audio_log_help, rec_audio_log_help);

static char rec_audio_test_help[] = "rec_audio_test";
static int rec_audio_test(int argc, char **argv)
{	
	g_return_val_if_fail (_nf_rec_audio != NULL, 0);	

	if(strcmp(argv[1], "reset") == 0)
		nf_dev_audio_dsp_reset(FALSE);
	else if(strcmp(argv[1], "odelay") == 0)		// get odelay
	{
		gint delay=0;

		nf_dev_audio_dsp_get_odelay(FALSE, &delay);	
		g_message("Audio Playback Delay[%d]", delay);
	}

	return 0;
}
__commandlist(rec_audio_test, "rec_audio_test", rec_audio_test_help, rec_audio_test_help);

#endif
