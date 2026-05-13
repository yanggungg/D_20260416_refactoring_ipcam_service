// #include <glib.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// //#include <gst/nf/gstnfbuddybuffer2.h>
// #include <gst/nf/gstnfbuddybuffer.h>
// #include <gst/nf/gstnflistbuffer.h>
#define _GNU_SOURCE
#include <sched.h>
#include <gobj.h>
#include <gobjmedia.h>

#include <pthread.h>
#include <sys/time.h>

#include "libsst.h"
#include "libicmem.h"

#include "unp.h"

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"

#include "nf_record.h"
#include "nf_audio_common.h"//ksi_test

/* Novatek Header */
#if defined(CHIP_NVT)
#include <novatek/hdal.h>
#include <novatek/hd_debug.h>
#include "nf_audio_novatek.h"
#endif
#include "nf_audio.h"

// #include "nf_rec_audio.h" ksi_test

#include "nf_notify.h"
#include "nf_timer.h"
#include "nf_codec_header.h"
#include "nf_logevtdef.h"

#include "nf_api_disk.h"
#include "nf_api_live.h"

#include "nf_api_ipcam.h"
#include "nf_issm_ctl.h"
#include "nf_issm_ctl_funcs.h"

#include "nf_util_device.h"
#include "nf_util_time.h"

//#include <nmf_mrtp_pipe.h>
#include <gobjmrtppipe.h>


//captainnn
#include "ivca_def.h"
#include "nf_meta_data.h"

#include "nf_pos.h"
#include "nf_va_object_detector.h"

// #if defined(ENABLE_AUD_HI_CHIP)
// #include "nf_HI_aud.h"
// #endif

// #if defined(_IPX_0412M4) || defined(_IPX_0824M4)|| defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
// #include "nf_cntl_audio.h"
// #endif

#ifdef USE_RTSP4VLC
#include "nf_nvs_common.h"
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "record"

#define ENABLE_GST_BUFFER_FAILED_SKIP
#define ENABLE_PUT_FRAME_MAX_QUEUE		
#define REC_AUDIO_ENABLE
#define PROFILE_DATA

#define ENABLE_IPCAM_2ND_RECODING	// FIXME
#define ENABLE_PUT_FRAME_ASSERT
#define PUT_FRAME_ASSERT_CNT		3600
#define NETWORK_REQ_DEBOUNCE_DELAY  5    //sec

#define ENABLE_POE_RESET	// 2013-02-15 ���� 3:27:00
#define ENABLE_USER_VA_EVENT	// 2013-06-07 ���� 8:04:22

#define ENABLE_DISK_FULL_WORKAROUND // 2013-06-26 ���� 2:08:30
#define ENABLE_SYSDB_CHANGE_START_TIME	// 2014-06-05 ���� 1:35:34 BPM-515 #77 (16ch audio rec issue)

#if defined(_ATM_1624) || defined(_OTM_1648) || defined(_OTM_0824)    /*20090804, TODO */
//#define ENABLE_ALARM_CH_MAP
#endif  /**/

//#define QUICK_REFRESH_TBL
//#define QUICK_RECORD_CMD

//#define DEBUG_PRE_RECORD_OFF
//#define DEBUG_POST_RECORD_OFF
//#define DEBUG_POST_REC_TIMER
//#define DEBUG_RECORD_DISABLE_TBL_TIMER
//#define DEBUG_RECORD_PRE_REC_THREE_TIMES

#define DEBUG_RECORD_LOG
//#define DEBUG_RECORD_DUMP
#define DEBUG_RECORD_JBSHELL

#ifdef DEBUG_RECORD_JBSHELL
	#include "jbshell.h"
#endif

//#define TRACE_ANFREC_
#ifdef	TRACE_ANFREC_
#define __T(fmt, args...)	g_print("[ANFREC]. . . . . . . . . . . . ."fmt"\n", ##args)

#else	/**/
#define __T(fmt, afgs...)
#endif	/**/
#define __E(fmt, args...)    g_error("[ANFREC]. . . . . . . . . . . . ."fmt"\n", ##args)
#define __A(c, fmt, args...) \
                if(!(c))    \
				{           \
					g_print("\n[ANFREC] **ERROR** func:%s, line:%d. "fmt,\
                            __FUNCTION__, __LINE__, ##args);          \
					g_assert(0);    \
				}

typedef enum _DEBUG_RECORD_IDX_E
{
	DEBUG_RECORD_IDX_QPOP_SIZE 		= 0,
	DEBUG_RECORD_IDX_FRAME_CPY		= 1,
	DEBUG_RECORD_IDX_QSIZE			= 2,
	DEBUG_RECORD_IDX_QFREE			= 3,
	
	DEBUG_RECORD_IDX_FREE_FRAME		= 4,
	DEBUG_RECORD_IDX_QPUSH			= 5,
	DEBUG_RECORD_IDX_DSPQUE			= 6,	
	DEBUG_RECORD_IDX_REF_TBL		= 7,
	
	DEBUG_RECORD_IDX_SKIP_SST		= 8,
	DEBUG_RECORD_IDX_REC_MAN		= 9,
	DEBUG_RECORD_IDX_REC_MAN_REASON	= 10,
	DEBUG_RECORD_IDX_REC_MAN_LOG	= 11,
	
	DEBUG_RECORD_IDX_REC_MAN_TBL	= 12,
	DEBUG_RECORD_IDX_DUMP			= 13,	
	DEBUG_RECORD_IDX_DUMP_CH_MASK	= 14,		
	DEBUG_RECORD_IDX_LOG_CH_MASK	= 15,

	DEBUG_RECORD_IDX_NO_REC_FRAME	= 16,
	DEBUG_RECORD_IDX_REC_FRAME		= 17,
	DEBUG_RECORD_IDX_SKIP_REC_FRAME	= 18,
	DEBUG_RECORD_IDX_DSP2ARM_SYNC	= 19,
	
	DEBUG_RECORD_IDX_NR				= 20
}DEBUG_RECORD_IDX_E;

static const char *_DEBUG_RECORD_str[32] =
{
	"RECORD_IDX_QPOP_SIZE",       
	"RECORD_IDX_FRAME_CPY",       
	"RECORD_IDX_QSIZE",           
	"RECORD_IDX_QFREE",         
	  
	"RECORD_IDX_FREE_FRAME",      
	"RECORD_IDX_QPUSH",           
	"RECORD_IDX_DSPQUE",          
	"RECORD_IDX_REF_TBL",       
	  
	"RECORD_IDX_SKIP_SST",        
	"RECORD_IDX_REC_MAN",
	"RECORD_IDX_REC_MAN_REASON",
	"RECORD_IDX_REC_MAN_LOG",
	
	"RECORD_IDX_REC_MAN_TBL",
	"RECORD_IDX_DUMP",            	
	"RECORD_IDX_DUMP_CH_MASK",  	  
	"RECORD_IDX_LOG_CH_MASK",
	
	"RECORD_IDX_NO_REC_FRAME",	
	"RECORD_IDX_REC_FRAME",
	"RECORD_IDX_SKIP_REC_FRAME",
	"RECORD_IDX_DSP2ARM_SYNC",
	
	"RECORD_IDX_NR"               
};

static guint _DEBUG_RECORD_log[32] = 
{
/*  0 1 2 3  4 5 6 7  8 9 0 1  2 3 4 5 */
#if 0
	0,0,0,0, 0,0,0,0, 0,0xffff,0,0, 0,0,0,0,
	0xffff,0xffff,0xffff,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
#else
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
#endif		

};

#define TIME_CHECK_START \
  { \
    struct timespec start; \
    struct timespec end;   \
    float delta;           \
    clock_gettime(CLOCK_REALTIME, &start); 

#define TIME_CHECK_END \
    clock_gettime(CLOCK_REALTIME, &end); \
    delta = (end.tv_sec - start.tv_sec)*1000.0 + (end.tv_nsec - start.tv_nsec)/1.0e6; \
    g_messagef("%f\n", delta); \
  }
  
 
/* Object signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_SENSOR,
	PROP_MOTION,
	PROP_VLOSS,	

#ifdef ENABLE_USER_VA_EVENT
	PROP_VA_EVENT,
	PROP_USER_EVENT,	
#endif

	PROP_POS_EVENT,
	PROP_MANUAL_EVENT,

	PROP_DVA_EVENT,

	LAST_PROP	
	/* FILL ME */
};

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	extern guint _nf_action_num_alarm;
#endif
//#ifdef SUPPORT_VCA_CAMERA
//captainnn
extern NF_META *nf_meta;
//#endif

static void nf_record_class_init (NfRecordClass * klass);
static void nf_record_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_record_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_record_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_record_dispose (GObject * object);
static void nf_record_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfRecord	*_nf_record = NULL;

static void record_thread_func (NfRecord *arg);
static void record_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_notify_sensor_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void disk_full_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_vca_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_ai_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_ai_fr_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_ai_lpr_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void record_ai_generic_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static void _sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static gint _get_pre_rec_time();
static gint _get_post_rec_time();

static gint _get_rec_mode();
static gint _get_rec_auto_config();
static gint _get_panic_time();


static void _load_panic_timer(gint sec);
static gint _dec_panic_timer();

int test_frame_1, test_frame_2, test_frame_3, test_frame_4;
int t_video_frame, t_meta_event_frame, t_audio_frame;
static pthread_t _frame_check_log_th;
static void _frame_check_log(void);
static void _frame_check_log(void)
{
	//printf("[khkh] (%s) thread Start \n", __func__);
	int interval = 2;
	test_frame_1 = 0; // mrtp->record
	test_frame_2 = 0; // record -> sst, 1st
	test_frame_3 = 0; // record -> sst, 2nd
	test_frame_4 = 0; // relaese

	while(1)
	{
		//printf("[khkh] [Current Frame Check] MRTP->Record:(%d), Record->SST:(%d),(%d+%d), Free:(%d) \n", 
		//		test_frame_1, test_frame_2+test_frame_3, test_frame_2, test_frame_3, test_frame_4);
		test_frame_1 = 0;
		test_frame_2 = 0;
		test_frame_3 = 0;
		test_frame_4 = 0;
		/*
		printf("[khkh] [Create Frame] Video:(%d), Audio:(%d), Meta/Event:(%d) \n", 
				t_video_frame, t_audio_frame, t_meta_event_frame);
		t_video_frame = 0;
		t_meta_event_frame = 0;
		t_audio_frame = 0;
		*/
		sleep(interval);
	}
	//printf("[khkh] (%s) thread error, check please \n", __func__);
}

static inline void _set_network_live(gint ch, gint val);
static gint _timer_simple_record(gint mode);
static void _dump_rec_table();
#ifdef ENABLE_ALARM_CH_MAP
static gint _get_alarm_ch_map(gint ch, gint date, gint hour);
#endif
static gboolean record_queue_process_ipcam_1st( gpointer frame, guint chan, gboolean is_2nd);
static gboolean record_queue_process_ipcam_2nd( gpointer frame, guint chan, gboolean is_2nd);



static NF_REC_AUDIO_PARAM _audio_param;
static guint _get_cam_audio_in(gint ch);

static gint _get_rec_priority_mode();
static gint _get_rec_sched();

static void
_record_update_profile( ICODEC_HEADER *pheader, gint stream_idx );

// private ---------------------------------------
typedef struct __record_set_unit
{
	int		enable;

	int		size;
	int		fps;
	int		quality;
	int		audio;
} _record_set_unit;

typedef struct __record_setting_t
{
	int		rec_mode;	// AUTO/MANUAL
	int		auto_config;	// in Auto(0 ~ 7)
	int		priority_mode;	// in Auto->Continuous(low q / high q)
	int		sched_mode;		// in Maunal(Daily/Weekly)

	_record_set_unit	auto_motion[NUM_CHANNEL];
	_record_set_unit	auto_alarm[NUM_CHANNEL];
	_record_set_unit	auto_motion_alarm[NUM_CHANNEL];

	_record_set_unit	auto_its_motion_normal[NUM_CHANNEL];
	_record_set_unit	auto_its_motion_motion[NUM_CHANNEL];

	_record_set_unit	auto_its_alarm_normal[NUM_CHANNEL];
	_record_set_unit	auto_its_alarm_alarm[NUM_CHANNEL];

	_record_set_unit	auto_its_ma_normal[NUM_CHANNEL];
	_record_set_unit	auto_its_ma_event[NUM_CHANNEL];

	_record_set_unit	manual_continuous[NUM_CHANNEL][8][24];	// SUN~SAT + DAILY
	_record_set_unit	manual_motion[NUM_CHANNEL][8][24];
	_record_set_unit	manual_alarm[NUM_CHANNEL][8][24];
} _record_setting_t;

#define F_OVERRATE_RATE (1.1)

static void _build_record_sysdb_table(_record_setting_t *table, int mode);
static void _calc_bps_mode_auto_high_qual(gint priority, NFIPCamBpsTable *bps_table, gdouble *result);
static void _calc_bps_mode_auto(_record_set_unit *record_table, NFIPCamBpsTable *bps_table, gdouble *result);
static void _calc_bps_mode_manual_daily(_record_set_unit record_table[NUM_ACTIVE_CH][8][24], NFIPCamBpsTable *bps_table, gdouble *result);
static void _calc_bps_mode_manual_weekly(_record_set_unit record_table[NUM_ACTIVE_CH][8][24], NFIPCamBpsTable *bps_table, gdouble *result);
static guint _get_bps(int ch, _record_set_unit unit, NFIPCamBpsTable *bps_table);
static guint _get_audio_bps(int ch, NFIPCamBpsTable *bps_table);
static gint _convert_resol_to_ipcam(gint val);
static gint _convert_fps_to_ipcam(gint val);
// -----------------------------------------------

GType
nf_record_get_type (void)
{
	static GType nf_record_type = 0;

	if (G_UNLIKELY (nf_record_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfRecordClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_record_class_init,
			NULL,
			NULL,
			sizeof (NfRecord),
			0,
			(GInstanceInitFunc) nf_record_instance_init,
			NULL
		};

		nf_record_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfRecord", &object_info, 0);
	}
	
	return nf_record_type;
}

static void
nf_record_class_init (NfRecordClass * klass)
{	
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_record_set_property;
	gobject_class->get_property = nf_record_get_property;
			
	gobject_class->dispose = nf_record_dispose;
	gobject_class->finalize = nf_record_finalize;

}

static void 
record_table_init( NF_RECORD_MAN *tbl )
{
	int i;	
	memset( tbl, 0x00, (sizeof(NF_RECORD_MAN)*NUM_TOTAL_CHANNEL) );
	
	for(i=0; i< NUM_TOTAL_CHANNEL; i++)
	{
		//tbl[i].stream_id = -1; 
	}

#if 0
	for(i=0; i< NUM_TOTAL_CHANNEL; i++)
	{		
		tbl[i].resolution = NF_RES_NTSC_CIF;
		tbl[i].fps  	= NF_FPS_CR32;
		tbl[i].quality	= NF_QUALITY_HIGHEST;
		tbl[i].audio	= 0;
		tbl[i].record_reason = 0;
	}
#endif	
}


static void 
record_sst_init( NF_RECORD_SST *tbl )
{
	int i;	
	memset( tbl, 0x00, (sizeof(NF_RECORD_SST)*NUM_TOTAL_CHANNEL) );
	
	for(i=0; i< NUM_TOTAL_CHANNEL; i++)
	{
		tbl[i].stream_id = -1; 
	}	
}


static void
nf_record_instance_init (GTypeInstance* instance, gpointer g_class)
{
	int i=0;
	NfRecord *self = NF_RECORD (instance);

	self->init_done = 0;
				
	// queue ����
	self->queue = g_async_queue_new();
#if defined(_ANF_1648)||defined(_ATM_1624)||defined(_ATM_0424)
	self->queue_v4l2    = g_async_queue_new();      /*_ANF_1648*/
#endif
	self->record_off = 1;	
	self->intensive_mode = NF_RECORD_INTENSIVE_NONE;
	self->sched_mode = NF_RECORD_SCHEDULE_DAILY;		
	self->manual_rec = 0;	
	self->pre_rec_time = 0;
	self->post_rec_time = 0;
	self->record_man_start = 0;

	self->panic_time = 0;	
	self->record_auto_mode = 0;
	self->panic_rec_timer = 0;

#ifdef ENABLE_CALENDAR_OPTIMIZE
	self->log_stream_id = -1;
#endif

	memset( &self->stat, 0x00, sizeof(NF_RECORD_STAT));
	
	for(i=0; i<NF_DSPCOMM_NUM_CH; ++i)
	{
		self->stat.gop_arr[i] = g_malloc0( sizeof(ICODEC_HEADER)*NF_DSPCOMM_GOP_CNT );
		
		g_assert(self->stat.gop_arr[i] != NULL );
	}
		
	memset( self->ch, 0x00, sizeof(NF_RECORD_CH)*NUM_TOTAL_CHANNEL );

	record_table_init( self->man );	// NF_RECORD_MAN man;
	
	record_sst_init( self->sst[0] );	// 1st
	record_sst_init( self->sst[1] );	// 2nd
		
	//_timer_simple_record(0);

	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)record_thread_func, 
									self, FALSE, NULL);
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_record_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_record_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_record_set_property (GObject * object, guint prop_id,
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
nf_record_get_property (GObject * object, guint prop_id,
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


static void 
dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d] [%x]", str , 
					pheader->chan, 
					pheader->flags, 
					pheader->frame_type, 
					pheader->frame_rate, 
					pheader->resolution, 						
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size,
					pheader->gst_buffer	 );	
}

//#define DEBUG_RECORD_DUMP_REDMEMBER
#define DEBUG_RECORD_DUMP_HEADER

static void	dump_frame_data( ICODEC_HEADER		*pheader )
{
	static guint frame_cnt[32] = {0,};
	static guint wait_i_frame[32] = {0,};
	
	gchar	fname[256];
	FILE	*fp = NULL;
	gchar	*data = (((char *)pheader)+sizeof(ICODEC_HEADER));
	gint	channel = pheader->chan;
		
#if defined( DEBUG_RECORD_DUMP_REDMEMBER )
	if (frame_cnt[channel] < _DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP])
	{					
		int 	size = 	pheader->frame_size;
		int		frame_type = pheader->frame_type;
				
		snprintf( fname, sizeof(fname), "/dump_ch%2d.out", channel);	
				
		fp = fopen( fname, frame_cnt[channel] == 0 ? "w" : "a");
				
		fwrite( &frame_type, sizeof(int), 1, fp);		
		fwrite( &size, sizeof(int), 1, fp);		
		fwrite( data, pheader->frame_size, 1, fp);					
	
		fclose(fp);

		dump_icodec_header( "fdump", pheader);
		frame_cnt[channel]++;
		
	}	

#elif defined(DEBUG_RECORD_DUMP_HEADER)

	if (frame_cnt[channel] < _DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP])
	{					

		if( pheader->frame_type== 0x01 )
			wait_i_frame[channel] = 1;
		
		if( wait_i_frame[channel] == 0 )
			return;
						
		snprintf( fname, sizeof(fname), "/dump_ch%2d.out", channel);	
		fp = fopen( fname, frame_cnt[channel] == 0 ? "w" : "a");
		
		fwrite( pheader, pheader->frame_size+sizeof(ICODEC_HEADER), 1, fp);
		//fwrite( data, pheader->frame_size, 1, fp);
		
		fclose(fp);

		dump_icodec_header( "fdump", pheader);		
		frame_cnt[channel]++;
	}
	
#else	
	if (frame_cnt[channel] < _DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP])
	{			
		sprintf(fname, "/dump_ch%02d_%08d%c.out", channel, frame_cnt[channel]++,
				(pheader->frame_type== 0x01) ? 'i':'p'	);

		fp = fopen(fname, "w");
		fwrite( data, pheader->frame_size, 1, fp);
		
		fclose(fp);
				
		dump_icodec_header( "fdump", pheader);
		frame_cnt[channel]++;
	}
#endif
	else{
		_DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP] = 0;
		memset(frame_cnt, 0x00, sizeof(frame_cnt));
		memset(wait_i_frame, 0x00, sizeof(wait_i_frame));
	}

}


static gboolean 
_is_record_ch_reopen( guint ch, ICODEC_HEADER *pheader, ICODEC_HEADER *prev_pheader)
{
	
	// ������ ���� ��������� ���ؼ� ���ڵ� ���°� �ٲ���� Ȯ���ؾ� �Ѵ�.			
	if(
		   (pheader->codec		== prev_pheader->codec		)
		&& (pheader->version	== prev_pheader->version	)
		&& (pheader->frame_rate	== prev_pheader->frame_rate	)
		&& (pheader->resolution	== prev_pheader->resolution	)
		&& (pheader->flags		== prev_pheader->flags		)  )
		return 0;
	else
		return 1;
		
}

static void 
record_sst_ch_reset(NF_RECORD_SST *psst)
{
	psst->stream_id = -1;	
	psst->need_to_close = 0;
	
	psst->is_put_iframe = 0;
	//psst->skip_pframe_cnt = 0;
				
	memset( &psst->last_iframe_header, 0x00, sizeof(ICODEC_HEADER));
	//memset( &psst->last_header, 0x00, sizeof(ICODEC_HEADER));
	
	return;	
}

void test_delay_calculater(char *mod_name, struct timeval test_start_tv, struct timeval test_end_tv, int diff)
{
	guint test_sec, test_usec;
	
	if( test_start_tv.tv_usec > test_end_tv.tv_usec )
	{
		test_sec = (test_end_tv.tv_sec - 1) - test_start_tv.tv_sec ;
		test_usec = (1000000 + test_end_tv.tv_usec) - test_start_tv.tv_usec;
	}
	else
	{
		test_sec = test_end_tv.tv_sec - test_start_tv.tv_sec;
		test_usec = test_end_tv.tv_usec - test_start_tv.tv_usec;
	}

	if( test_sec > diff )
	{
		printf("[test_delay] (%s) (%d:%u)\n", mod_name, test_sec, test_usec);
	}
}

static gboolean
record_queue_process_ipcam_1st( gpointer frame, guint chan, gboolean is_2nd )
{	
	
	GobjBuddyBuffer	*buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = gobj_buddy_buffer_buf_get_addr(buffer);
	
	NF_RECORD_MAN		*pman = &_nf_record->man[chan];	
	NF_RECORD_SST		*psst = &_nf_record->sst[0][chan];	
		
	gint				ret = 0;
	guint				phy_chan = chan + (BASE_IPCAM_2ND_CHANNEL*is_2nd);
	gboolean			is_icodec_change = 0;
	
#if 1
	// pheader->flags = pheader->flags >>2;	
	// FIXME!!
	if( pman->resolution == NF_RES_640x360
		|| pman->resolution == NF_RES_640x352
		|| pman->resolution == NF_RES_640x480
		|| pman->resolution == NF_RES_640x400
		|| pman->resolution == NF_RES_NTSC_CIF
		|| pman->resolution == NF_RES_PAL_CIF
		|| pman->resolution == NF_RES_320x180
		|| pman->resolution == NF_RES_NTSC_4CIF
		|| pman->resolution == NF_RES_NTSC_4CIFP
		|| (pman->record_reason == NF_RECORD_REASON_PRE 
		|| pman->resolution == NF_RES_1280x720
		|| pman->resolution == NF_RES_1024x768
		&& _nf_record->pre_rec_time>MAX_PRE_REC_TIME_HD_LIMIT ) ){
		if(!is_2nd)	// 1st Stream
			goto out;		
	}else{
		if(is_2nd)	// 2nd Stream
			goto out;
	}
#endif 

	if( psst->is_put_iframe == 0 && pheader->frame_type == NF_FRAME_TYPE_P ){
		++psst->skip_pframe_cnt;
		goto out;
	}

	is_icodec_change = _is_record_ch_reopen(chan, pheader, &psst->last_iframe_header);
				
	if( psst->stream_id >=0 
		&& (psst->need_to_close || is_icodec_change || pheader->frame_type == NF_FRAME_TYPE_END) ){
					
		gboolean is_flush = 0;
		guint pre_close_sec = 0;
				
		if( (pman->record_reason > 1 && pman->record_reason < 6) && pman->pre_record ) {
			is_flush = 1;			
			pre_close_sec = _nf_record->pre_rec_time;
		}

		if( pheader->frame_type != NF_FRAME_TYPE_P ) {	// 2014-02-14 ���� 2:56:48 frame loss(gop toggle condition)
						
			//g_message("sst_record_close ch[%2d] sid[%2d] pre_flush[%d][%d]  [%d]", 
			//			chan, psst->stream_id , is_flush, pre_close_sec, psst->put_frame_count ); 
	
			ret = sst_record_close( psst->stream_id, pre_close_sec);
			g_assert( ret == 0);


#ifdef REC_AUDIO_ENABLE
			//_audio_param.ch_arr[chan] = 0xff;
			_audio_param.rec_reason[chan] = 0;
			_audio_param.pre_rec_time[chan] = 0;
			_audio_param.pre_rec_close[chan] = pre_close_sec;
			// NVR & DVR
			#if defined(_NVR_MODEL)
				_audio_param.send_type = NF_AUDIO_SEND_IPCAM;
			#else
				_audio_param.send_type = NF_AUDIO_SEND_DVR;
			#endif

			nf_audio_start(&_audio_param);
#endif		
			record_sst_ch_reset( psst );
		}else{
			if( pheader->resolution == psst->last_iframe_header.resolution )
				is_icodec_change = 0;
		}
			
			
	}

	if(pheader->frame_type == NF_FRAME_TYPE_END )
		goto out;
	
	if( pheader->frame_type == NF_FRAME_TYPE_P 
			&& ( is_icodec_change || pman->fps == NF_FPS_CR01 || psst->skip_pframe_cnt ) ) {
		++psst->skip_pframe_cnt;
		++psst->put_frame_count;
		goto out;
	}

	
	if( pheader->frame_type == NF_FRAME_TYPE_I && pman->fps == NF_FPS_CR01 )
	{
		static unsigned int ts_check[64] = {0,};
		
		if( ts_check[phy_chan] == pheader->timestamp )
		{	
			//g_message("skip ch[%d]ts[%d]", phy_chan, pheader->timestamp);
			++psst->put_frame_count;
			goto out;
		}else{
			ts_check[phy_chan] = pheader->timestamp;
		}
	}
	
	if( psst->last_header.timestamp > pheader->timestamp ||
			(psst->last_header.timestamp == pheader->timestamp 
				&& psst->last_header.timestampl > pheader->timestampl ) )
	{		
		//dump_icodec_header("ERR TS skip frame", pheader);
		goto out;
	}
	
	if( psst->stream_id == -1 && pman->record_reason != 0 && nf_record_is_rec_off() != TRUE )
	{
		gint stream_id = -1;			
		gint pre_rec_sec = (pman->record_reason == NF_RECORD_REASON_PRE) ? _nf_record->pre_rec_time:0;

		stream_id = sst_record_open( (guint8)chan,
										(guint8)pre_rec_sec,
										(guint8)pman->record_reason, 
										(guint8)pheader->codec,
										(guint8)pheader->resolution,
										(guint8)pheader->frame_rate,
										//(guint8)pman->fps,
										(guint8)pman->quality);
		
		/*
		g_message("sst_record_open  ch[%2d] sid[%2d] reason[%d] pre[%d]codec[%d]res[%x]fps[%2d]q[%d]",
						chan, stream_id ,
						pman->record_reason,
						pre_rec_sec, // pre_max
						pheader->codec,
						pheader->resolution,
						pheader->frame_rate,
						pman->quality );
						*/

		if( stream_id <0)
		{
			dump_icodec_header("ERR header frame", pheader);
			//g_message("sst_record_open result[%d](%s)", stream_id, sst_get_error_string(stream_id)); 	
			
			g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� g_assert

		}else{
			psst->stream_id = stream_id;			
		}

#ifdef REC_AUDIO_ENABLE
		// g_message("XXXXXXXXXXXXXXXXXX vch:%d/audio:%d/ach:%d.",
		//   channel, pman->audio, pman->audio_ch);   //20090724, cultfactory 

		#ifdef ENABLE_CAM_AUDIO_REMAP
			if( pman->audio == 1 &&  pman->audio_ch != 0 )
				_audio_param.ch_arr[chan] = pman->audio_ch - 1;
			else
				_audio_param.ch_arr[chan] = 0xff;
		#else 
			if( pman->audio == 1 )
				_audio_param.ch_arr[chan] = (pman->audio_ch-1);
			else
				_audio_param.ch_arr[chan] = 0xff;
		#endif 
		_audio_param.rec_reason[chan] = pman->record_reason;
		_audio_param.pre_rec_time[chan] = pre_rec_sec;
		_audio_param.pre_rec_close[chan] = 0;
		// NVR & DVR
		#if defined(_NVR_MODEL)
			_audio_param.send_type = NF_AUDIO_SEND_IPCAM;
		#else
			_audio_param.send_type = NF_AUDIO_SEND_DVR;
		#endif

		nf_audio_start(&_audio_param);
#endif

	}
	
	if( psst->stream_id >=0 && pman->record_reason != 0 && 
			(pheader->frame_type == NF_FRAME_TYPE_P 
				|| pheader->frame_type == NF_FRAME_TYPE_I) )
	{
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = g_object_ref(frame);
		if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);

		struct timeval test_start_tv, test_end_tv;

		gettimeofday(&test_start_tv, NULL);	
		test_frame_2++;
		ret = sst_record_put_frame ( psst->stream_id, pheader );
		gettimeofday(&test_end_tv, NULL);
		
		test_delay_calculater("1st_record_put", test_start_tv, test_end_tv, 1);
	
		if(ret){

#ifdef ENABLE_PUT_FRAME_ASSERT
       	    g_assert( ret == -SST_ERR_DISKFULL || ret == -SST_ERR_INVPARAM );
#endif       	    
			if( ret == -SST_ERR_INVPARAM )
			{

				if(psst->err_frame_ts != pheader->timestamp){
					++psst->err_frame_count;

					//g_message("sst_record_put_frame result[%d](%s)", ret, sst_get_error_string(ret));
					//dump_icodec_header("ERR frame", pheader);
					//g_warning("%s SST_ERR_INVPARAM ch[%d] count[%d]",__FUNCTION__, 
					//				phy_chan, psst->err_frame_count);

				}
				psst->err_frame_ts = pheader->timestamp;

#ifdef ENABLE_PUT_FRAME_ASSERT
				g_assert( psst->err_frame_count < PUT_FRAME_ASSERT_CNT);	// 1 min
#endif
			}					
			g_object_unref(frame);
			
		}else{
			
			++psst->put_frame_count;
			memcpy( &psst->last_header, pheader, sizeof(ICODEC_HEADER));			
			if( pheader->frame_type == NF_FRAME_TYPE_I ){
				memcpy( &psst->last_iframe_header, pheader, sizeof(ICODEC_HEADER));
				psst->is_put_iframe = 1;
				psst->skip_pframe_cnt = 0;
			}
			
		}
	}else {

#ifdef DEBUG_RECORD_LOG
		if( _DEBUG_RECORD_log[DEBUG_RECORD_IDX_NO_REC_FRAME] & (1<<pheader->chan) )
			dump_icodec_header("noREC", pheader);
#endif

	}
							
out:	
	return 1;
	
}

/********************************
R: record flag, P: prerecord flag
E: END_FRAME, S: START_FRAME
---------------------------------
P: Prerecroding
PD: Prerecord discard
PF: Prerecord flush

========================
new  | cur  RP RP RP RP
RP   |      00 01 10 11
-----|------------------
00   |      X  E  E  E
     |         PD
-----|------------------
01   |      S  ES ES ES
     |      P  PD P  P
-----|------------------
10   |      S  ES ES ES
     |         PD
-----|------------------
11   |      S  ES ES ES
     |         PF
========================
*********************************/

static gboolean
record_queue_process_ipcam_2nd( gpointer frame, guint chan, gboolean is_2nd )
{	
	
	GobjBuddyBuffer	*buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = gobj_buddy_buffer_buf_get_addr(buffer);
	
	NF_RECORD_MAN		*pman = &_nf_record->man[chan];	
	NF_RECORD_SST		*psst = &_nf_record->sst[1][chan];	
		
	gint				ret = 0;
	guint				phy_chan = chan + (BASE_IPCAM_2ND_CHANNEL*is_2nd);
	gboolean			is_icodec_change = 0;

	if( !is_2nd ) goto out;
					
	//pheader->flags = pheader->flags >>2;		
	if( psst->is_put_iframe == 0 && pheader->frame_type == NF_FRAME_TYPE_P ){
		++psst->skip_pframe_cnt;
		goto out;
	}

	if( pheader->frame_type == NF_FRAME_TYPE_I 
		&& _is_record_ch_reopen(chan, pheader, &psst->last_iframe_header) ){
		is_icodec_change = 1;
	}
	
	if( psst->stream_id >=0 
		&& (psst->need_to_close || is_icodec_change || pheader->frame_type == NF_FRAME_TYPE_END) ){
	
		gboolean is_flush = 0;
		guint pre_close_sec = 0;
				
		if( (pman->record_reason > 1 && pman->record_reason < 6) && pman->pre_record ) {
			is_flush = 1;			
			pre_close_sec = _nf_record->pre_rec_time;
		}
						
		//g_message("sst_record_close ch[%2d] sid[%2d] pre_flush[%d][%d]  [%d]", 
		//			phy_chan, psst->stream_id , is_flush, pre_close_sec, psst->put_frame_count ); 
															
		ret = sst_record_close( psst->stream_id, pre_close_sec);
		g_assert( ret == 0);
		
		record_sst_ch_reset( psst );		
	}

	if(pheader->frame_type == NF_FRAME_TYPE_END )
		goto out;

	if( pheader->frame_type == NF_FRAME_TYPE_P 
			&& ( is_icodec_change || pman->fps == NF_FPS_CR01 || psst->skip_pframe_cnt ) ) {
		++psst->skip_pframe_cnt;
		++psst->put_frame_count;
		goto out;
	}

	if( pheader->frame_type == NF_FRAME_TYPE_I && pman->fps == NF_FPS_CR01 )
	{
		static unsigned int ts_check[64] = {0,};
		
		if( ts_check[phy_chan] == pheader->timestamp )
		{	
			//g_message("skip ch[%d]ts[%d]", phy_chan, pheader->timestamp);
			++psst->put_frame_count;
			goto out;
		}else{
			ts_check[phy_chan] = pheader->timestamp;
		}
	}
	
	if( psst->stream_id == -1 && pman->record_reason != 0 && nf_record_is_rec_off() != TRUE )
	{
		gint stream_id = -1;			
		gint pre_rec_sec = pman->record_reason == NF_RECORD_REASON_PRE  ? _nf_record->pre_rec_time:0 ;

		stream_id = sst_record_open( (guint8)phy_chan,
										(guint8)pre_rec_sec,
										(guint8)pman->record_reason, 
										(guint8)pheader->codec,
										(guint8)pheader->resolution,
										(guint8)pheader->frame_rate,
										//(guint8)pman->fps,
										(guint8)pman->quality);
		
		/*
		g_message("sst_record_open  ch[%2d] sid[%2d] reason[%d] pre[%d]codec[%d]res[%x]fps[%2d]q[%d]",
						phy_chan, stream_id ,
						pman->record_reason,
						pre_rec_sec, // pre_max
						pheader->codec,
						pheader->resolution,
						pheader->frame_rate,
						pman->quality );
						*/

		if( stream_id <0)
		{
			dump_icodec_header("ERR header frame", pheader);
			//g_message("sst_record_open result[%d](%s)", stream_id, sst_get_error_string(stream_id)); 	
			
			g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� g_assert
		}else{
			psst->stream_id = stream_id;
		}				

	}
	
	if( psst->stream_id >=0 && pman->record_reason != 0 && 
			(pheader->frame_type == NF_FRAME_TYPE_P 
				|| pheader->frame_type == NF_FRAME_TYPE_I) )
	{
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = g_object_ref(frame);
		if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);

		struct timeval test_start_tv, test_end_tv;

		gettimeofday(&test_start_tv, NULL);	
		test_frame_3++;
		ret = sst_record_put_frame ( psst->stream_id, pheader );
		gettimeofday(&test_end_tv, NULL);
		
		test_delay_calculater("2nd_record_put", test_start_tv, test_end_tv, 1);	

		if(ret){

#ifdef ENABLE_PUT_FRAME_ASSERT
       	    g_assert( ret == -SST_ERR_DISKFULL || ret == -SST_ERR_INVPARAM );
#endif
			if( ret == -SST_ERR_INVPARAM )
			{
				if(psst->err_frame_ts != pheader->timestamp){
					++psst->err_frame_count;

					//g_message("sst_record_put_frame result[%d](%s)", ret, sst_get_error_string(ret));
					//dump_icodec_header("ERR frame", pheader);				
					//g_warning("%s SST_ERR_INVPARAM ch[%d] count[%d]",__FUNCTION__, phy_chan, psst->err_frame_count);
					
				}
				psst->err_frame_ts = pheader->timestamp;
#ifdef ENABLE_PUT_FRAME_ASSERT
				g_assert( psst->err_frame_count < PUT_FRAME_ASSERT_CNT);	// 1 min				
#endif
			}					
			g_object_unref(frame);
			
		}else{
			
			++psst->put_frame_count;
			memcpy( &psst->last_header, pheader, sizeof(ICODEC_HEADER));				
			if( pheader->frame_type == NF_FRAME_TYPE_I ){
				memcpy( &psst->last_iframe_header, pheader, sizeof(ICODEC_HEADER));
				psst->is_put_iframe = 1;
				psst->skip_pframe_cnt = 0;
			}
			
		}
	}else {

#ifdef DEBUG_RECORD_LOG
		if( _DEBUG_RECORD_log[DEBUG_RECORD_IDX_NO_REC_FRAME] & (1<<pheader->chan) )
			dump_icodec_header("noREC", pheader);
#endif

	}
						
out:	
	return 1;
	
}

static gint _refresh_tbl_flag  = 0;

static gboolean
nf_record_refresh_tbl(gint mode)
{			
	GobjBuddyBuffer		*frame;
	ICODEC_HEADER	*pheader;
	gint clen;
		
	g_return_val_if_fail(_nf_record != NULL, FALSE);
	// manual, alarm, motion, timer ����

	g_return_val_if_fail(_nf_record->record_man_start != 0, FALSE);
		
	clen = 1024; // dummy size

	frame = gobj_buddy_buffer_new_malloc( clen );
	g_return_val_if_fail (frame != NULL, FALSE);		

	pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);
	
	memset(pheader, 0xff, sizeof(ICODEC_HEADER));
	
	// control frame marker
	pheader->reserved  = 0x01234567;
	pheader->timestamp = 0x89ABCDEF;
	pheader->frame_size = 0;
	pheader->gst_buffer = NULL;
	
	pheader->flags = (guchar)mode;
		
	//g_object_ref(frame);
	g_async_queue_push( _nf_record->queue, frame );	


#ifdef DEBUG_RECORD_LOG
	if( _DEBUG_RECORD_log[ DEBUG_RECORD_IDX_REF_TBL ] )
		g_message("%s mode[0x%08x]", __FUNCTION__, mode);
#endif

	return 1;
}

guint manual_event_status = 0;

gboolean nf_record_alarm_by_manual_event(gint ch, gboolean event_on)
{
	g_return_val_if_fail ( ch< NUM_ACTIVE_CH, FALSE );	
	
	if(event_on)
	{
		manual_event_status |= (1 << ch);
	}
	else
	{	
		manual_event_status &= ~(1 << ch);
	}
	
	nf_notify_fire_params("manual_event", manual_event_status, 0, 0, 0);

	return TRUE;
}

#define NF_RECORD_REF_PANIC_RECORD_START		0
#define NF_RECORD_REF_PANIC_RECORD_STOP			1
#define NF_RECORD_REF_RECORD_START				2
#define NF_RECORD_REF_RECORD_STOP				3
#define NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC		4
#define NF_RECORD_REF_RECORD_REFRESH_TBL_TIMER	5
#define NF_RECORD_REF_SYSDB_CHANGED				6


static int panic_pending (void)
{
	static struct timeval otv = { .tv_sec=0, .tv_usec=0 };
	struct timeval tv, tvr;
	int rv = 0;

	gettimeofday (&tv, NULL);

#if 0
	if (tv.tv_sec != otv.tv_sec)
		rv = 1;
#else
	timersub (&tv, &otv, &tvr);	

	if (tvr.tv_sec >= 1 || tvr.tv_sec <= -1)
	{
		rv = 1;
		otv = tv;
	}
#endif
	

	return rv;
} 


static void _panic_set( gboolean is_set )
{
	if(is_set)
		proxy_system("/bin/touch /NFDVR/data/panic_rec",1,3);
	else 
		proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);
				
	proxy_system("/bin/sync",1,3);	
}

static gboolean _panic_is_set()
{
	struct stat buf;	

#if 0	
	g_message("%s stat(/NFDVR/data/panic_rec) [%d]", __FUNCTION__, stat( "/NFDVR/data/panic_rec", &buf) );
	g_message("%s [%d][%s]", __FUNCTION__, errno, strerror(errno) );
#endif 
		
	return ( stat( "/NFDVR/data/panic_rec", &buf ) == 0) ? 1:0;	
}


/**
	@brief				�д� ���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_start( GError **error )
{
	g_message( "%s called", __FUNCTION__ );

	g_return_val_if_fail (_nf_record != NULL, FALSE);
	
	if (!panic_pending())
		return 0;

	if(  _nf_record->manual_rec == 1)
		g_warning("%s already panic(manual) record start!!", __FUNCTION__);

	NF_OBJECT_LOCK(_nf_record);

	_nf_record->manual_rec = 1;
	_panic_set( _nf_record->manual_rec );
		
	if( _nf_record->panic_time )
		_load_panic_timer(_nf_record->panic_time);

	NF_OBJECT_UNLOCK(_nf_record);
		
	nf_record_refresh_tbl(NF_RECORD_REF_PANIC_RECORD_START);
	
	return 1;	
}

/**
	@brief				�д� ���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_stop(  GError **error )
{
	g_message( "%s called", __FUNCTION__ );

	g_return_val_if_fail (_nf_record != NULL, FALSE);
	
	if (!panic_pending())
		return 0;

	if(  _nf_record->manual_rec == 0)
		g_warning("%s already panic(manual) stop!!", __FUNCTION__);

	NF_OBJECT_LOCK(_nf_record);

	_nf_record->manual_rec = 0;
	_panic_set( _nf_record->manual_rec );
	
	_nf_record->panic_rec_timer = 0;

	NF_OBJECT_UNLOCK(_nf_record);
	
	nf_record_refresh_tbl(NF_RECORD_REF_PANIC_RECORD_STOP);
	
	return 1;	
} 


/**
	@brief				�д� ���ڵ� toggle
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_is_set( )
{
	g_return_val_if_fail (_nf_record != NULL, FALSE);
	return _nf_record->manual_rec;
}



/**
	@brief				�д� ���ڵ� toggle
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_panic_record_toggle(  GError **error )
{
	//g_message( "%s called", __FUNCTION__ );
	g_return_val_if_fail (_nf_record != NULL, FALSE);

	//if (!panic_pending()) return 0;

	if( _nf_record->manual_rec == 0 ){
		g_message("%s PANIC START",__FUNCTION__);		
        return nf_panic_record_start(NULL);
	}else{		
		g_message("%s PANIC STOP",__FUNCTION__);
		return nf_panic_record_stop(NULL);
	}
		
	return 1;	
} 

void cb_mrtp_pipe_put_record(GobjListBuffer *buffer, 
                                gpointer user_data)
{
	ICODEC_HEADER	*pheader;
    
#if 1
	if(GOBJ_IS_LIST_BUFFER(buffer))
    {
		GobjListBuffer *list_buffer = buffer;
		GList *walk;
		walk = list_buffer->buffer_list;

		while(walk) 
		{
			if( !GOBJ_IS_BUDDY_BUFFER(walk->data) )
			{
		        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
				walk = g_list_next(walk);
				continue;
			}			
			
			GobjBuddyBuffer *child_buf = walk->data;
            pheader = gobj_buddy_buffer_buf_get_addr(child_buf);
            
			void *tmp_gst_ret = NULL;
			tmp_gst_ret = g_object_ref(child_buf);
			if(tmp_gst_ret == NULL)
				fprintf(stderr, "- minto - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);

			if( pheader->frame_type == NF_FRAME_TYPE_AUDIO )            
			{
#ifdef REC_AUDIO_ENABLE
				#if defined(CHIP_NVT)
					if(nf_audio_put_frame(pheader->chan, child_buf) == FALSE) {
						//g_warning("%s: put frame fail\n", __FUNCTION__);
						//dump_icodec_header("put_fail", pheader);
						g_object_unref(child_buf);
					}
				#else
					if(nf_rec_audio_put_frame(pheader->chan, child_buf) == FALSE)
					{
						//g_warning("%s: put frame fail\n", __FUNCTION__);
						//dump_icodec_header("put_fail", pheader);
						g_object_unref(child_buf);
					}
				#endif
#else 
					g_object_unref(child_buf);
#endif         	
			}else{ // AUDIO

			//captainnn
		if( pheader->frame_type == 7 ){
			g_object_unref(buffer);

		}
		else if( pheader->frame_type == 8){
			g_object_unref(buffer);
		}
		else{				
	            if(nf_record_put_frame(pheader->chan, child_buf) == FALSE)
	            {
					if (0x7fffffffff < child_buf) {
	                	g_warning("%s: child_buf %p put frame fail\n", __FUNCTION__, child_buf);
	                //dump_icodec_header("put_fail", pheader);
					} else {
	                	g_object_unref(child_buf);
					}
	            }
		}

			}
			walk = g_list_next(walk);
        }
    }
    else
    {
		if( !GOBJ_IS_BUDDY_BUFFER(buffer) )
		{
	        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
			return;
		}
		
        pheader = gobj_buddy_buffer_buf_get_addr(buffer);

		GobjBuddyBuffer *tmpBuffer = NULL;
       	tmpBuffer = g_object_ref(buffer);

		test_frame_1++; // for frame test 

		if(tmpBuffer == NULL) {
			fprintf(stderr, "- ERROR - g_object_ref ret is NULL\n");
		}
		if( tmpBuffer != NULL && pheader->frame_type == NF_FRAME_TYPE_AUDIO )
		{
#ifdef REC_AUDIO_ENABLE
			#if defined(CHIP_NVT)
				if(nf_audio_put_frame(pheader->chan, buffer) == FALSE) {
					//g_warning("%s: put frame fail\n", __FUNCTION__);
					//dump_icodec_header("put_fail", pheader);
					g_object_unref(buffer);
				}
			#else
				if(nf_rec_audio_put_frame(pheader->chan, buffer) == FALSE)
				{
					//g_warning("%s: put frame fail\n", __FUNCTION__);
					//dump_icodec_header("put_fail", pheader);
					g_object_unref(buffer);
				}
			#endif
#else 
		g_object_unref(buffer);
#endif         	
			
        }else{	// AUDIO
        //captainnn
        // meta data frame type 7 from nmrtp todo define
		if( pheader->frame_type == 7 ){
#ifdef SUPPORT_VCA_CAMERA
			//t_meta_event_frame++;
			nf_meta_data_push(buffer);
#endif
		} 
		else if( pheader->frame_type == 8)	// event data
		{
#ifdef SUPPORT_VCA_CAMERA
			nf_event_data_func(buffer);
			g_object_unref(buffer);
#endif
		}
		else {
			//t_video_frame++;
			if(nf_record_put_frame(pheader->chan, buffer) == FALSE)
			{
				if (0x7fffffffff < buffer) {
					g_warning("%s():%d buffer %p put frame fail\n", __FUNCTION__, __LINE__, buffer);
				//dump_icodec_header("put_fail", pheader);
				} else {
					g_object_unref(buffer);
				}
			}
		}
	}
    }
#endif
}

static guint _record_start_time = 0;

static void _record_set_start_time(guint time)
{
	printf("[%s] SETTING [%d]", __FUNCTION__, time);
	
	_record_start_time = time;
}

guint nf_record_get_start_time(void)
{
	return _record_start_time;
}

static guint _record_end_time = 0;

static void _record_set_end_time(guint time)
{
	printf("[%s] SETTING [%d]", __FUNCTION__, time);
	
	_record_end_time = time;
}

guint nf_record_get_end_time(void)
{
	return _record_end_time;
}

/**
	@brief				���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/ 
gboolean
nf_record_start( GError **error )
{
	NF_DISK_INFO disk_info;
	gboolean ret = 0;
	
	g_message( "%s called", __FUNCTION__ );
	g_return_val_if_fail (_nf_record != NULL, FALSE);	
		
	if(nf_sysman_qcmode_is_enable()) 
	{
		g_message("%s skipped..... QC MODE" , __FUNCTION__);
		return FALSE;
	}

	if( _nf_record->record_man_start == 0 )
	{
        gpointer h_mrtp_pipe;
#if 1	// ���⼭ �ϸ� �ȵ� --> fs_start_complete ���� ȣ�� �ؾ� ��.	
		// --> gui�� �ؾ���.. 	
		nf_sysman_time_check();
        sleep(3);
#endif	// FIXME !!

        h_mrtp_pipe = nf_live_get_mrtp_pipe_handle();
        nmf_mrtp_pipe_set_src_handoff((GobjMrtpPipe *)h_mrtp_pipe, 
                                        cb_mrtp_pipe_put_record, 
                                        NULL);

		_nf_record->record_man_start = 1;
		g_message("%s record_man_start!! [%d]",__FUNCTION__, 
					_nf_record->record_man_start);		
#ifdef ENABLE_VLOSS_RECOVERY
        nf_live_recover_vloss(0x3398ffff);  // "onoff test" sync failed recovery
#endif
        
		if( _panic_is_set() ) {
			_nf_record->manual_rec = 1;
			g_message("%s record_man_start!! PANIC ON!!",__FUNCTION__ );
		}		
	}
	
	if( _nf_record->record_off == 0 )
		g_warning("%s already record start!!", __FUNCTION__);

	_nf_record->record_off = 0;
	
	if( nf_disk_get_info( &disk_info, NULL ) == FALSE  || nf_filesystem_is_online() == 0 )
	{				
		g_warning("%s NO_DISK!!", __FUNCTION__);
		g_warning("%s NO_DISK!!", __FUNCTION__);		
		
		_nf_record->record_off = 1;
				
	}
	else if (nf_sysman_qcmode_is_enable()) 
	{
		g_warning("%s QC MODE!!", __FUNCTION__);
		_nf_record->record_off = 1;
	}
	else
	{		
		guint is_overwrite = nf_sysdb_get_uint("disk.write_mode");	
		
		if( is_overwrite == 0 &&  nf_disk_is_full( NULL ) )
		{				
			g_warning("%s DISK_FULL!!", __FUNCTION__);
			g_warning("%s DISK_FULL!!", __FUNCTION__);
			
			_nf_record->record_off = 1;
		}			
	}
			
	nf_record_refresh_tbl(NF_RECORD_REF_RECORD_START);

//	unit_test_main();

	GTimeVal curr_timeval;
	gettimeofday(&curr_timeval, NULL);

	_record_set_start_time(curr_timeval.tv_sec);

	return TRUE;	
	
}

/**
	@brief				���ڵ� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/ 
gboolean
nf_record_stop( GError **error )
{
	gint wait_cnt = 0;
	
	g_message( "%s called", __FUNCTION__ );
	g_return_val_if_fail (_nf_record != NULL, FALSE);
	
	if(nf_sysman_qcmode_is_enable()) 
	{
		g_message("%s skipped..... QC MODE" , __FUNCTION__);
		return FALSE;
	}

	if(  _nf_record->record_off == 1)
		g_warning("%s already record stop!!", __FUNCTION__);
			
	_nf_record->record_off = 1;
	
	nf_record_refresh_tbl(NF_RECORD_REF_RECORD_STOP);

	//captainnn
#ifdef SUPPORT_VCA_CAMERA
	nf_record_meta_close();
#endif

	wait_cnt = 0;
	while(1)	
	{
		int stream_count = 0, stream_count_2nd = 0;		
		int ch;
		int meta_stream_count =0;
		int audio_stream_count = 0;

// #if defined(ENABLE_AUD_HI_CHIP)
// 		NF_HI_AUD_REC_S *pstAudRec = NULL;
#if defined(REC_AUDIO_ENABLE)
		NF_AUDIO_REC *pstAudRec = NULL;
#endif		
		
		++wait_cnt;
				
		for(ch=0;ch<NUM_TOTAL_CHANNEL;ch++) {
			if( _nf_record->sst[0][ch].stream_id >= 0)
				++stream_count;

			if( _nf_record->sst[1][ch].stream_id >= 0)
				++stream_count_2nd;
#ifdef SUPPORT_VCA_CAMERA
			if( nf_meta->sst[ch].stream_id >= 0)
				++meta_stream_count;
#endif
		}	

// #if defined(ENABLE_AUD_HI_CHIP)
#ifdef REC_AUDIO_ENABLE
		// pstAudRec = nf_HI_aud_getRec();ksi_test
		pstAudRec = nf_audio_getRec();

		for (ch=0; ch < NUM_ACTIVE_CH; ch++)
		{
			if( pstAudRec->stParam[ch].s32StmId >= 0 )
				++audio_stream_count;
		}		
#endif
// #endif

#ifdef SUPPORT_VCA_CAMERA
		g_message("%s wait_cnt[%d] stream_count[%d][%d][%d]",__FUNCTION__ ,wait_cnt , 
			stream_count, stream_count_2nd, meta_stream_count);
#else
		g_message("%s wait_cnt[%d] stream_count[%d][%d]",__FUNCTION__ ,wait_cnt , 
			stream_count, stream_count_2nd);
#endif

		if( stream_count == 0 && stream_count_2nd == 0 && meta_stream_count ==0 && audio_stream_count == 0 )
			break;
							
		if( wait_cnt > 10 )
		{
			int audio_reset = 0;
			
			g_warning("%s wait_cnt failed", __FUNCTION__);
	
			proxy_system("cat /proc/ifs_recording_status", 1, 3);	
			
			//_dump_rec_table();

			// FIXME for ipcam recive module workaround
			for(ch=0;ch<NUM_TOTAL_CHANNEL;ch++) {				
			
				if( _nf_record->sst[0][ch].stream_id >= 0) {
					
					NF_RECORD_SST		*psst = &_nf_record->sst[0][ch];
						
					//g_message("sst_record_close ch[%2d] sid[%2d]", ch, psst->stream_id); 
					sst_record_close( psst->stream_id, 0);
					record_sst_ch_reset( psst );
				}
				
				if( _nf_record->sst[1][ch].stream_id >= 0) {
					
					NF_RECORD_SST		*psst = &_nf_record->sst[1][ch];
					
					//g_message("sst_record_close ch[%2d] sid[%2d]", ch, psst->stream_id); 
					sst_record_close( psst->stream_id, 0);
					
					record_sst_ch_reset( psst );
				}
				//captainnn
#ifdef SUPPORT_VCA_CAMERA
				if( nf_meta->sst[ch].stream_id >= 0) {
					
					NF_RECORD_SST		*psst = &nf_meta->sst[ch];
					
					//g_message("sst_record_close ch[%2d] sid[%2d]", ch, psst->stream_id); 
					nf_record_meta_close();
				}
#endif
			}
//ksi_test
// #if defined(ENABLE_AUD_HI_CHIP)
// #ifdef REC_AUDIO_ENABLE
#if defined(REC_AUDIO_ENABLE)
			for(ch=0; ch<NUM_ACTIVE_CH; ch++)
			{
				if( pstAudRec->stParam[ch].s32StmId >= 0 )
				{
					g_message("force audio close ch[%d]", ch);
					
					//_audio_param.ch_arr[chan] = 0xff;
					_audio_param.rec_reason[ch] = 0;
					_audio_param.pre_rec_time[ch] = 0;
					_audio_param.pre_rec_close[ch] = 0;
					// NVR & DVR
					#if defined(_NVR_MODEL)
						_audio_param.send_type = NF_AUDIO_SEND_IPCAM;
					#else
						_audio_param.send_type = NF_AUDIO_SEND_DVR;
					#endif
			
					audio_reset = 1;
				}
			}

			if(audio_reset)
			{
				nf_audio_start(&_audio_param);
			}
#endif			
// #endif
			break;
		}
			
		g_usleep(1000*1000);
		
	}

	GTimeVal curr_timeval;
	gettimeofday(&curr_timeval, NULL);

	_record_set_end_time(curr_timeval.tv_sec);
	
	return TRUE;	
}

/**
	@brief				ä���� ���ڵ� ���� ��ȸ
	@param[in]	ch_num	���� ä�� 
	@param[out] ch_info	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_get_channel_info(gint ch_num, NF_RECORD_CH *ch_info )
{
	g_return_val_if_fail (_nf_record != NULL, FALSE);	
	g_return_val_if_fail (ch_num >= 0 && ch_num < NUM_TOTAL_CHANNEL , FALSE);	
	g_return_val_if_fail (ch_info != NULL , FALSE);	
	
	memcpy( ch_info, &_nf_record->ch[ch_num], sizeof(NF_RECORD_CH));
		
	return 1;				
}


#ifdef ENABLE_DISK_FULL_WORKAROUND
static int _g_disk_full_workaround = 0;
#endif


#ifdef DEBUG_RECORD_JBSHELL
static int _g_rec_err_sim_onoff = 0;

static char rec_err_sim_help[] = "rec_err_sim [onoff]";
static int rec_err_sim(int argc, char **argv)
{				
	if(argc < 2){
		printf("%s\n",rec_err_sim_help);
		return -1;
	}
	_g_rec_err_sim_onoff = strtol(argv[1],NULL,0);			
	return 0;
}
__commandlist(rec_err_sim,"rec_err_sim",rec_err_sim_help, rec_err_sim_help);

#endif 

/**
	@brief				ä���� ���ڵ� watchdog
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_watchdog_is_ok()
{
	guint 		i = 0, ret = 1;	
	static	 	guint prev_count[NUM_TOTAL_CHANNEL] = {0, };	
	static	 	guint err_count[NUM_TOTAL_CHANNEL] = {0, };	
	static		guint log_flag[NUM_TOTAL_CHANNEL] = {0, };	
	static	 	guint record_queue_count[NUM_TOTAL_CHANNEL] = {0, };
			
	g_return_val_if_fail (_nf_record != NULL, 1);
	g_return_val_if_fail ( _g_rec_err_sim_onoff == 0, 0);		
	
#ifdef ENABLE_DISK_FULL_WORKAROUND
	if( _g_disk_full_workaround ) {
		++_g_disk_full_workaround;
		g_warning("%s _g_disk_full_workaround[%d]",__FUNCTION__, _g_disk_full_workaround);
		g_assert( _g_disk_full_workaround < 10 );
		return 0;
	}
#endif

	for(i=0; i<NUM_TOTAL_CHANNEL; ++i)
	{
		if(	_nf_record->man[i].record_reason 
			&& prev_count[i] == _nf_record->sst[0][i].put_frame_count && record_queue_count[i] != _nf_record->sst[0][i].push_frame_count) {	

			ret = 0;
			++err_count[i];
			if(	err_count[i] > 30 )
			{
				if( nf_sysdb_get_bool("cam.install.mode") == 0)	{ // is_cctv_mode
					g_warning("%s nf_ipcam_poe_reboot ch[%d]/[%d]", __FUNCTION__, i,  prev_count[i]);
					nf_ipcam_poe_reboot(i, NULL, NULL, NULL);
				}else{
					g_warning("%s skip! openmode ch[%d]/[%d]", __FUNCTION__, i,  prev_count[i]);
				}
												
				if( log_flag[i] == 0 ){
					char buf[128];
					snprintf(buf, 128,  "ipcam recv err ch[%d]/[%d]", i, prev_count[i]);
					nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);
					log_flag[i] = 1;
					g_warning("%s nf_eventlog_put_param [%s]", __FUNCTION__, buf);
				}
				err_count[i]=0;
			}
		}else{
			err_count[i]=0;
		}
		prev_count[i] = _nf_record->sst[0][i].put_frame_count;
		record_queue_count[i] = _nf_record->sst[0][i].push_frame_count;
	}		
	return ret;				
}



#if !(DEBUG_REC_OUTPUT_)
/**
	@brief				
	@param[in]	ch_num	���� ä��
	@param[in]	frame	���� ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_put_frame(gint ch_num, gpointer frame )
{
	g_return_val_if_fail (_nf_record != NULL, FALSE);	
	g_return_val_if_fail (_nf_record->record_man_start != 0, FALSE);
	g_return_val_if_fail (frame != 0, FALSE);
	g_return_val_if_fail (ch_num >= 0 && ch_num < 64, FALSE);	


#ifdef ENABLE_PUT_FRAME_MAX_QUEUE
	while( g_async_queue_length( _nf_record->queue ) > (480*4) ) g_usleep(10*1000);
#endif

	g_async_queue_push( _nf_record->queue, frame );

	if( ch_num < BASE_IPCAM_2ND_CHANNEL)
		++_nf_record->sst[0][ch_num].push_frame_count;
/* // 2013-06-26 ���� 2:31:22 choissi disable
	else
		++_nf_record->sst[1][ch_num-BASE_IPCAM_2ND_CHANNEL].push_frame_count;
*/
	return 1;
}
#endif /* DEBUG_REC_OUTPUT_ */
/**
	@brief				rec_audio handoff ���
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean
nf_record_register_handoff(long long ch_mask, NF_RECORD_HANDOFF_FUNC handoff_func, long long mobile_mask)
{


	gint i,ch;
	g_return_val_if_fail (_nf_record != NULL, 0);	
	
	g_message("%s ch_mask[0x%016llx] handoff_fnx[%p] mobile_mask[0x%016llx]", __FUNCTION__, 
					ch_mask, handoff_func, mobile_mask);
							
	NF_OBJECT_LOCK(_nf_record);
	
	_nf_record->handoff_func = handoff_func;
	_nf_record->handoff_ch_mask = ch_mask;

	for(ch=0;ch<NUM_ANALOG_CHANNEL;ch++)
	{
		long long tmp_mask =  (1<<ch) | (1<<(ch+32));
		
		if( ch_mask & tmp_mask ) {
			_set_network_live(ch, (mobile_mask & tmp_mask ) ? 3 : 1 );	// 1: just live, 3: mobile live	
		} else {
			_set_network_live(ch, 0);
		}
			
	}	
		
	NF_OBJECT_UNLOCK(_nf_record);

	return 1;
}


//_nf_record->dspcomm_thread[NUM_DSP];
void nf_record_get_stream_stat( gint type,  NF_RECORD_STREAM_STAT *state ) // 0:curr 1:diff
{	
	gint i;
	
	static NF_DSPCOMM_STATE	tot_state[NF_DSPCOMM_NUM_CH];	
	static int init = 0;
	
	g_return_if_fail( _nf_record != NULL );
	
	if(init == 0 )
	{
		memset( tot_state, 0x00, sizeof(tot_state));
		init = 1;
	}
			
	if(type == 0)
	{
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->i_cnt[i] = _nf_record->stat.tot_state[i].i_cnt; 			
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->i_kbyte[i] = _nf_record->stat.tot_state[i].i_kbyte; 
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->p_cnt[i] = _nf_record->stat.tot_state[i].p_cnt;
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->p_kbyte[i] = _nf_record->stat.tot_state[i].p_kbyte; 			

	}else{

		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->i_cnt[i] =  _nf_record->stat.tot_state[i].i_cnt - tot_state[i].i_cnt;
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->i_kbyte[i] = _nf_record->stat.tot_state[i].i_kbyte - tot_state[i].i_kbyte;	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->p_cnt[i] = _nf_record->stat.tot_state[i].p_cnt - tot_state[i].p_cnt;
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			state->p_kbyte[i] = _nf_record->stat.tot_state[i].p_kbyte - tot_state[i].p_kbyte;				
	}

	for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
	{
		tot_state[i].i_cnt = _nf_record->stat.tot_state[i].i_cnt;
		tot_state[i].p_cnt = _nf_record->stat.tot_state[i].p_cnt;
		tot_state[i].i_kbyte = _nf_record->stat.tot_state[i].i_kbyte;
		tot_state[i].p_kbyte = _nf_record->stat.tot_state[i].p_kbyte;
	}
			
}

//_nf_record->dspcomm_thread[NUM_DSP];
void nf_record_send_stream_stat_to_s1() // 0:curr 1:diff
{	
	gint i;
	static NF_DSPCOMM_STATE	tot_state[NF_DSPCOMM_NUM_CH];	
	static int init = 0;			
	unsigned int frame_cnt[64] = {0,};
	
	g_return_if_fail( _nf_record != NULL );
	
	if(init == 0 )
	{
		memset( tot_state, 0x00, sizeof(tot_state));
		init = 1;
	}
				
	for(i=0; i<NF_DSPCOMM_NUM_CH; i++) {
		frame_cnt[i] =  _nf_record->stat.tot_state[i].p_cnt - tot_state[i].p_cnt;
	}
	
	for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
	{
//		tot_state[i].i_cnt = _nf_record->stat.tot_state[i].i_cnt;
		tot_state[i].p_cnt = _nf_record->stat.tot_state[i].p_cnt;
//		tot_state[i].i_kbyte = _nf_record->stat.tot_state[i].i_kbyte;
//		tot_state[i].p_kbyte = _nf_record->stat.tot_state[i].p_kbyte;
	}	
	nf_ipcam_put_recorded_fcnt( frame_cnt );	
}


#ifdef _ATM_0412
#define ATM_MAX_REC_RSC_NTSC    (120+3)
#define ATM_MAX_REC_RSC_PAL     (100+3)

#elif defined(_SNF_1648)
#define ATM_MAX_REC_RSC_NTSC    (1920)
#define ATM_MAX_REC_RSC_PAL     (1600)

#define ATM_MAX_NET_STREAM_RSC_NTSC    (480)
#define ATM_MAX_NET_STREAM_RSC_PAL     (400)

#elif defined(_SNF_0824) || defined(_SNF_0424)
#define ATM_MAX_REC_RSC_NTSC    (960)
#define ATM_MAX_REC_RSC_PAL     (800)

#define ATM_MAX_NET_STREAM_RSC_NTSC    (240)
#define ATM_MAX_NET_STREAM_RSC_PAL     (200)

#else   /**/
#define ATM_MAX_REC_RSC_NTSC    (480)
#define ATM_MAX_REC_RSC_PAL     (400)

#define ATM_MAX_NET_STREAM_RSC_NTSC    (120)
#define ATM_MAX_NET_STREAM_RSC_PAL     (100)
#endif  /**/

static gint 
TestSizeRecRate(const gint *size, const gfloat *recrate)
{
    int i;
    float sum = 0;
    
	for (i = 0; i < NUM_ACTIVE_CH; i++)
		sum += (size[i]*recrate[i]);

	return ( DISPLAY_IS_PAL? (gint)(ATM_MAX_REC_RSC_PAL-sum): (gint)(ATM_MAX_REC_RSC_NTSC-sum) );
}

static gfloat
_convert_fps( gint fps )
{
    gfloat val;

    if( DISPLAY_IS_PAL ) {
        switch(fps)
        {
            case NF_FPS_CR32:   val = 25.0f; break;
            case NF_FPS_CR16:   val = 12.5f; break;
            case NF_FPS_CR08:   val = 6.25f; break;
            case NF_FPS_CR04:   val = 3.13f; break;
            case NF_FPS_CR02:   val = 1.57f; break;
            case NF_FPS_CR01:   val = 0.79f; break;
            case NF_FPS_CR00:   val = 0;  break;
            default:
                __A(0, "wrong fps.");
                val = 0;  
                break;
        }
    }
    else {  /*NTSC*/
        switch(fps)
        {
            case NF_FPS_CR32:   val = 30.0f; break;
            case NF_FPS_CR16:   val = 15.0;  break;
            case NF_FPS_CR08:   val = 7.5f;  break;
            case NF_FPS_CR04:   val = 3.75f; break;
            case NF_FPS_CR02:   val = 1.88;  break;
            case NF_FPS_CR01:   val = 0.94;  break;
            case NF_FPS_CR00:   val = 0;  break;
            default:
                __A(0, "wrong fps.");
                val = 0;  
                break;
        }
    }
    
    return val;
}

static gint _convert_res( gint res )
{
    gint val;
    
    switch(res)
    {
        case NF_RES_NTSC_CIF:
        case NF_RES_PAL_CIF:
            val = 1;
            break;
        case NF_RES_NTSC_2CIF:
        case NF_RES_PAL_2CIF:
            val = 2;
            break;
        case NF_RES_NTSC_4CIF:
        case NF_RES_NTSC_4CIFP:
        case NF_RES_PAL_4CIF:
        case NF_RES_PAL_4CIFP:
            val = 4;
            break;
        default:
            __A(0,"wrong res.");
            val = 1;
            break;
    }
    
    return val;
}

/**
	@brief				    check validity of recording cfg.
	@param[in]	record_cfg	recording cfg to check validity.
	@return	    gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_record_check_validity( NF_RECORD_CFG *record_cfg )
{
    gint ch, remained;
    gfloat fps[NUM_ACTIVE_CH];
	gint res[NUM_ACTIVE_CH];
    
    for(ch = 0; ch<NUM_ACTIVE_CH; ch++)
    {
        fps[ch] = _convert_fps( record_cfg->fps[ch] );
        res[ch] = _convert_res( record_cfg->res[ch] );
    }
    
    remained = TestSizeRecRate(res, fps);
    if( remained < 0 )
        return FALSE;
        
    return TRUE;
    
}

/**
	@brief				record �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_record_init(int wait)
{
	gboolean ret = TRUE;			
	gulong	cb_handle;
	GobjBuddyBuffer	*gst_buf = NULL;
	
	g_return_val_if_fail (_nf_record == NULL, FALSE);	

#if 1		// choissinf 2008-12-30 ���� 8:29:19 
	// preventing gst_nf_buddy_buffer init race bug
	gst_buf = gobj_buddy_buffer_new_malloc (1024);
	g_assert(gst_buf != NULL);
	// gst_buf->cmemq = gst_buf->free_cb = 0;		
	g_object_unref(gst_buf);
#endif
	
	_nf_record = g_object_new ( NF_TYPE_RECORD , NULL);
			
	nf_debug_category_add( "record", _DEBUG_RECORD_str, _DEBUG_RECORD_log, DEBUG_RECORD_IDX_NR);

	cb_handle= nf_notify_connect_cb( "sysdb_change", _sysdb_reload_cb_func , (gpointer)NULL );
	g_message("%s sysdb_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
		
	cb_handle= nf_notify_connect_cb( "sensor", record_notify_sensor_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "motion", record_notify_cb_func , (gpointer)PROP_MOTION );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "vloss", record_notify_cb_func , (gpointer)PROP_VLOSS);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#ifdef ENABLE_USER_VA_EVENT
	cb_handle= nf_notify_connect_cb( "uevent_s1_dual", record_notify_cb_func , (gpointer)PROP_USER_EVENT);
	g_message("%s sys_s1_dual connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	cb_handle= nf_notify_connect_cb( "pos_text_event", record_notify_cb_func , (gpointer)PROP_POS_EVENT);
	g_message("%s pos_event connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "manual_event", record_notify_cb_func , (gpointer)PROP_MANUAL_EVENT );
	g_message("%s manual_event connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "dva_event", record_notify_cb_func , (gpointer)PROP_DVA_EVENT);
	g_message("%s dva_event connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_full", disk_full_notify_cb_func , (gpointer)0);
	g_message("%s disk_full connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "vca_event", record_vca_notify_cb_func , NULL);

	cb_handle= nf_notify_connect_cb( "ai_event", record_ai_notify_cb_func , NULL);
	cb_handle= nf_notify_connect_cb( "ai_fr_event", record_ai_fr_notify_cb_func , NULL);
	cb_handle= nf_notify_connect_cb( "ai_lpr_event", record_ai_lpr_notify_cb_func , NULL);
	cb_handle= nf_notify_connect_cb( "ai_generic_event", record_ai_generic_notify_cb_func , NULL);

	// log test
    pthread_create(&_frame_check_log_th, NULL, (void*)&_frame_check_log, NULL);

	_nf_record->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );
	_nf_record->is_enable_stream_control = nf_sysdb_get_bool("rec.netstream.enable_stream_control");

	{
		gchar *sysdb_netstream_fps, *sysdb_netstream_quality;
		sysdb_netstream_fps = nf_sysdb_get_str_nocopy("rec.netstream.fps");
		sysdb_netstream_quality = nf_sysdb_get_str_nocopy("rec.netstream.quality");

		if (sysdb_netstream_fps)
			memcpy(_nf_record->netstream_fps, sysdb_netstream_fps, sizeof(_nf_record->netstream_fps));
		if (sysdb_netstream_quality)
			memcpy(_nf_record->netstream_quality, sysdb_netstream_quality, sizeof(_nf_record->netstream_quality));
	}
	
	memset( &_audio_param, 0x00, sizeof(_audio_param));
	memset( _audio_param.ch_arr, NF_AUDIO_INPUT_OFF, sizeof(_audio_param.ch_arr));	

	//nf_dspcomm_sync_time();	// !!! MUST CALL  dsp command recive start command

	if( wait )
	{
		while( _nf_record->init_done != 1)
			g_usleep(10*1000);
	}
			
	return ret;
}

/******************************************************************************/

static guint _get_cam_audio_in(gint ch);

static const char *_get_sysdb_linked_cam_ch( gint ch)
{	
	char buff[256];	
	snprintf(buff, sizeof(buff), "act.sensor.S%d.lcamera", ch);
	return nf_sysdb_get_str_nocopy(buff);		
}

static void _get_sysdb_linked_cam_all_data()
{
	int i;
	char *lcam = NULL;
	
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		if( lcam = _get_sysdb_linked_cam_ch(i) )
			memcpy( _nf_record->linked_cam[i], lcam, 32);
		else
			memset( _nf_record->linked_cam[i], '0', 32);
	}
}


static guint _is_check_linked_cam_event( gint ch, guint64 mask )
{
	int i;
			
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (i = 0; i < _nf_action_num_alarm; ++i)
	#else
	for(i=0; i<NUM_ALARM; ++i)
	#endif
	{
		if( _nf_record->linked_cam[i][ch] == '1' && (mask & 1ULL<<i) )
			return 1;
	}
	return 0;
}

static const char *_get_sysdb_linked_pos_ch( gint ch)
{	
	char buff[256];	
	snprintf(buff, sizeof(buff), "act.pos.P%d.lcamera", ch);
	return nf_sysdb_get_str_nocopy(buff);		
}

static void _get_sysdb_linked_pos_all_data()
{
	int i;
	char *lpos = NULL;
	
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		if( lpos = _get_sysdb_linked_pos_ch(i) )
			memcpy( _nf_record->linked_pos[i], lpos, 32);
		else
			memset( _nf_record->linked_pos[i], '0', 32);
        printf("[%s:%d] ch[%d] lpos[%.32s]\n", __func__, __LINE__, i, lpos);
	}
}

static guint _is_check_linked_pos_event( gint ch, guint mask )
{
	int i;
			
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		if( _nf_record->linked_pos[i][ch] == '1' && (mask & 1<<i) )
			return 1;
	}
	return 0;
}
/*
static char *_get_sysdb_linked_dva_ch( gint ch)
{	
	char buff[256];	
	snprintf(buff, sizeof(buff), "act.pos.P%d.lcamera", ch);
	return nf_sysdb_get_str_nocopy(buff);		
}

static void _get_sysdb_linked_dva_all_data()
{
	int i;
	char *ldva = NULL;
	
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		if( ldva = _get_sysdb_linked_dva_ch(i) )
			memcpy( _nf_record->linked_dva[i], ldva, 16);
		else
			memset( _nf_record->linked_dva[i], '0', 16);
	}
}

static guint _is_check_linked_dva_event( gint ch, guint mask )
{
	int i;
			
	for(i=0; i<NUM_ACTIVE_CH; ++i)
	{
		if( _nf_record->linked_dva[i][ch] == '1' && (mask & 1<<i) )
			return 1;
	}
	return 0;
}
*/
static void
_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	int ch, ret;
	g_return_if_fail(pinfo != NULL);
	
	g_message("%s param[%d]",__FUNCTION__, pinfo->d.params[0]);
	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_REC){
		nf_record_refresh_tbl(NF_RECORD_REF_SYSDB_CHANGED);
		
		// 2013-01-10 ���� 5:58:06 choissi 
		_nf_record->is_enable_stream_control = nf_sysdb_get_bool("rec.netstream.enable_stream_control");

		{
			gchar *sysdb_netstream_fps, *sysdb_netstream_quality;
			sysdb_netstream_fps = nf_sysdb_get_str_nocopy("rec.netstream.fps");
			sysdb_netstream_quality = nf_sysdb_get_str_nocopy("rec.netstream.quality");

			if (sysdb_netstream_fps)
				memcpy(_nf_record->netstream_fps, sysdb_netstream_fps, sizeof(_nf_record->netstream_fps));
			if (sysdb_netstream_quality)
				memcpy(_nf_record->netstream_quality, sysdb_netstream_quality, sizeof(_nf_record->netstream_quality));
		}
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM){
#ifdef ENABLE_CAM_AUDIO_REMAP
		for(ch=0;ch<NUM_ACTIVE_CH;++ch)
		{	
			if ( _nf_record->man[ch].audio_ch != _get_cam_audio_in(ch) ){
				nf_record_refresh_tbl(NF_RECORD_REF_SYSDB_CHANGED);
				break;
			}
		}
#endif		
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_ACT) {
		char *lcam;
		char *lpos;
//		char *ldva;		
		int i;
		
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		for (i = 0; i < _nf_action_num_alarm; ++i)
		#else
		for(i=0; i<NUM_ALARM; ++i)
		#endif
		{
			lcam = _get_sysdb_linked_cam_ch(i);
			if( lcam == NULL) continue;				

			if( memcmp( _nf_record->linked_cam[i], lcam, 32) != 0 )
			{				
				nf_record_refresh_tbl(NF_RECORD_REF_SYSDB_CHANGED);
				return;
			}
		}

		for(i=0; i<NUM_ACTIVE_CH; ++i)
		{
			lpos = _get_sysdb_linked_pos_ch(i);
			if( lpos == NULL) continue;				
			if( memcmp( _nf_record->linked_pos[i], lpos, 32) != 0 )
			{				
				nf_record_refresh_tbl(NF_RECORD_REF_SYSDB_CHANGED);
				return;
			}
		}
/*
		for(i=0; i<NUM_ACTIVE_CH; ++i)
		{
			ldva = _get_sysdb_linked_dva_ch(i);
			if( ldva == NULL) continue;				
			if( memcmp( _nf_record->linked_dva[i], ldva, 16) != 0 )
			{				
				nf_record_refresh_tbl(NF_RECORD_REF_SYSDB_CHANGED);
				return;
			}
		}		
*/		
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS){	// DST is_off
		_nf_record->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );
	}	
}

static void
disk_full_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{	

	
	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,					
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
			
	if( pinfo->d.params[0] )
	{
		// FIXME workaround DISK_FULL BUG!!
		guint is_overwrite = nf_sysdb_get_uint("disk.write_mode");
				
		g_warning("%s DISK_FULL is_overwrite[%d]",__FUNCTION__, is_overwrite);
		g_warning("%s DISK_FULL is_overwrite[%d]",__FUNCTION__, is_overwrite);
		nf_record_stop( NULL );	
		g_warning("%s DISK_FULL",__FUNCTION__);		
		g_warning("%s DISK_FULL",__FUNCTION__);

#ifdef ENABLE_DISK_FULL_WORKAROUND						
		if( is_overwrite == 1 )
		{
			g_warning("%s DISK_FULL FIXME is_overwrite[%d] workaround assert ",__FUNCTION__, is_overwrite);
			_g_disk_full_workaround = 1;
			g_warning("%s DISK_FULL FIXME is_overwrite[%d] workaround assert ",__FUNCTION__, is_overwrite);
		}
#endif
		
	}
		
}

static void
record_notify_sensor_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint64 old_val = 0ULL;
			
	guint prop_id = (gint)data;			
	guint64 new_val, m_new, m_old;
	guint64 *curr_val, *rise_val;
	
	gint i;
	gint max_i;

	g_return_if_fail(pinfo != NULL);
	g_assert( prop_id == PROP_SENSOR );

#ifdef DEBUG_NOTIFY_CB
	g_message("%s prop_id[%d] [%ld.%06ld] type[%d] nvr_alarm[%x] ipcam_alarm[%x]", __FUNCTION__,
					prop_id,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0], pinfo->type, pinfo->d.params[1] );
#endif

    new_val = (pinfo->d.params[1] & 0xffffffff) | ((unsigned long long)pinfo->d.params[0] << NUM_ALARM_IPCAM);
#ifdef DEBUG_NOTIFY_CB
    printf("[%s:%d] new_val[%016llx]\n", __func__, __LINE__, new_val);
#endif

	if( old_val == new_val )
	{
#ifdef DEBUG_NOTIFY_CB
		g_message("%s  same value, skip old[%08x] new[%08x]", 
					__FUNCTION__, old_val , new_val);
#endif
		return;
	}

	NF_OBJECT_LOCK( _nf_record );
	
    curr_val = &_nf_record->notify_data.cb_curr_alarm;
    rise_val = &_nf_record->notify_data.cb_rise_alarm;		

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
    max_i = _nf_action_num_alarm;
#else
    max_i = NUM_ALARM;
#endif

#ifdef DEBUG_NOTIFY_CB
    printf("[%s:%d] max_i[%d]\n", __func__, __LINE__, max_i);
#endif

	for (i = 0; i < max_i; ++i) {
		m_new = (new_val >> i) & 1ULL;
		m_old = (old_val >> i) & 1ULL;
				
		if( m_new != m_old )
		{
#ifdef DEBUG_NOTIFY_CB
		g_message("%s ch[%d] [%d]-->[%d]",  __FUNCTION__, i,  m_old, m_new);
#endif
			if(m_new)
			{
				*rise_val |= (1ULL << i);
				*curr_val |= (1ULL << i);
			}else{
				*curr_val &= ~(1ULL << i);
			}
			++_nf_record->notify_data.cb_change_flag;
		}
	}

	if( prop_id == PROP_POS_EVENT )
	{
		curr_val = 0ULL;
		old_val = 0ULL;
	}
	else if( prop_id == PROP_DVA_EVENT )
	{
		curr_val = 0ULL;
		old_val = 0ULL;
	}	
	else
	{	
		old_val = new_val;
	}

	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s curr[0x%016llx] rise[0x%016llx]",  __FUNCTION__, *curr_val, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif

}

static void
record_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
			
	guint prop_id = (gint)data;			
	guint new_val, m_new, m_old;
	guint *curr_val, *rise_val;
	
	gint i;
	gint max_i;
	
	g_return_if_fail(pinfo != NULL);
	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_NOTIFY_CB
	g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					prop_id,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	if(prop_id == PROP_POS_EVENT ){
		NF_POS_TEXT_EVENT *pos_event;

		pos_event = pinfo->p.ptr;
		
		new_val = 1 << pos_event->ch;
	}
	else if(prop_id == PROP_DVA_EVENT ){
		DVA_MSG *msg;
//		struct objects *objs;

		msg = pinfo->p.ptr;
		
		new_val = 1 << msg->ch;
	}	
	else
	{
		new_val	= pinfo->d.params[0];
#ifdef ENABLE_USER_VA_EVENT
		if(prop_id == PROP_USER_EVENT ){		
			new_val	|= pinfo->d.params[1];
			new_val	|= pinfo->d.params[2];
			new_val	|= pinfo->d.params[3];
		}
#endif
	}

	if( old_val[prop_id] == new_val )
	{
#ifdef DEBUG_NOTIFY_CB
		g_message("%s  same value, skip old[%08x] new[%08x]", 
					__FUNCTION__, old_val[prop_id] , new_val);
#endif
		return;
	}

	NF_OBJECT_LOCK( _nf_record );
	
	if(prop_id == PROP_MOTION ){
		curr_val = &_nf_record->notify_data.cb_curr_motion;
		rise_val = &_nf_record->notify_data.cb_rise_motion;		
		max_i = NUM_ACTIVE_CH;
	}else if(prop_id == PROP_SENSOR){
		curr_val = &_nf_record->notify_data.cb_curr_alarm;
		rise_val = &_nf_record->notify_data.cb_rise_alarm;		
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
			max_i = _nf_action_num_alarm;
		#else
			max_i = NUM_ALARM;
		#endif
	}else if(prop_id == PROP_VLOSS ){
		curr_val = &_nf_record->notify_data.cb_curr_vloss;
		rise_val = &_nf_record->notify_data.cb_rise_vloss;
		max_i = NUM_ACTIVE_CH;
#ifdef ENABLE_USER_VA_EVENT
	}else if(prop_id == PROP_VA_EVENT ){
		curr_val = &_nf_record->notify_data.cb_curr_va_event;
		rise_val = &_nf_record->notify_data.cb_rise_va_event;
		max_i = NUM_ACTIVE_CH;
	}else if(prop_id == PROP_USER_EVENT ){
		curr_val = &_nf_record->notify_data.cb_curr_user_event;
		rise_val = &_nf_record->notify_data.cb_rise_user_event;
		max_i = NUM_ACTIVE_CH;
#endif
	}else if(prop_id == PROP_POS_EVENT ){
		curr_val = &_nf_record->notify_data.cb_curr_pos_event;
		rise_val = &_nf_record->notify_data.cb_rise_pos_event;
		max_i = NUM_ACTIVE_CH;
	}else if(prop_id == PROP_MANUAL_EVENT ){
		curr_val = &_nf_record->notify_data.cb_curr_manual_event;
		rise_val = &_nf_record->notify_data.cb_rise_manual_event;
		max_i = NUM_ACTIVE_CH;
	}else if(prop_id == PROP_DVA_EVENT ){
		curr_val = &_nf_record->notify_data.cb_curr_dva_event;
		rise_val = &_nf_record->notify_data.cb_rise_dva_event;
		max_i = NUM_ACTIVE_CH;			
	}else{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		NF_OBJECT_UNLOCK( _nf_record );
		return;
	}
				
	for (i = 0; i < max_i; ++i) {
		m_new = (new_val >> i) & 1;
		m_old = (old_val[prop_id] >> i) & 1;
				
		if( m_new != m_old )
		{
#ifdef DEBUG_NOTIFY_CB
		g_message("%s ch[%d] [%d]-->[%d]",  __FUNCTION__, i,  m_old, m_new);
#endif
			if(m_new)
			{
				*rise_val |= (1 << i);
				*curr_val |= (1 << i);
			}else{
				*curr_val &= ~(1 << i);
			}
			++_nf_record->notify_data.cb_change_flag;
		}
	}

	if( prop_id == PROP_POS_EVENT )
	{
		curr_val = 0;
		old_val[prop_id] = 0;
	}
	else if( prop_id == PROP_DVA_EVENT )
	{
		curr_val = 0;
		old_val[prop_id] = 0;
	}	
	else
	{	
		old_val[prop_id] = new_val;
	}

	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s curr[0x%08x] rise[0x%08x]",  __FUNCTION__, *curr_val, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif

}

#define	VCA_MAX_ELEMS	MAX(IVCA_MAX_ZONES, IVCA_MAX_CNTRS)

static void
record_vca_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	guint *rise_val;
	
	gint i;

	g_return_if_fail(pinfo != NULL);

#ifdef DEBUG_NOTIFY_CB
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	ivca_rule_event_t *pevt;
	int* p;

	p = pinfo->p.ptr;
	pevt = p+2;

	rise_val = &_nf_record->notify_data.cb_rise_vca_event;

	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if( p[1] < 1 )
		return;

	NF_OBJECT_LOCK( _nf_record );

	for(i=0; i<p[1];i++){
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

		if( pevt->type ){
			*rise_val |= 1 << pevt->ch;
			++_nf_record->notify_data.cb_change_flag;
		}

		pevt++;
	}		
		
	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s rise[0x%08x]",  __FUNCTION__, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif
}

static void
record_ai_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	guint *rise_val;
	
	gint i;

	g_return_if_fail(pinfo != NULL);

#ifdef DEBUG_NOTIFY_CB
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	ai_rule_event_t *pevt;
	int* p;

	p = pinfo->p.ptr;
	pevt = p+2;

	rise_val = &_nf_record->notify_data.cb_rise_vca_event;

	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if( p[1] < 1 )
		return;

	NF_OBJECT_LOCK( _nf_record );

	for(i=0; i<p[1];i++){
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

		if( pevt->type ){
			*rise_val |= 1 << pevt->ch;
			++_nf_record->notify_data.cb_change_flag;
		}

		pevt++;
	}		
		
	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s rise[0x%08x]",  __FUNCTION__, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif
}

static void
record_ai_fr_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	guint *rise_val;
	
	gint i;

	g_return_if_fail(pinfo != NULL);

#ifdef DEBUG_NOTIFY_CB
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	ai_fr_event_t *pevt;
	int* p;

	p = pinfo->p.ptr;
	pevt = p+2;

	rise_val = &_nf_record->notify_data.cb_rise_vca_event;

	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if( p[1] < 1 )
		return;

	NF_OBJECT_LOCK( _nf_record );

	for(i=0; i<p[1];i++){
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

		if( pevt->type ){
			*rise_val |= 1 << pevt->ch;
			++_nf_record->notify_data.cb_change_flag;
		}

		pevt++;
	}		
		
	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s rise[0x%08x]",  __FUNCTION__, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif
}

static void
record_ai_lpr_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	guint *rise_val;
	
	gint i;

	g_return_if_fail(pinfo != NULL);

#ifdef DEBUG_NOTIFY_CB
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	ai_lpr_event_t *pevt;
	int* p;

	p = pinfo->p.ptr;
	pevt = p+2;

	rise_val = &_nf_record->notify_data.cb_rise_vca_event;

	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if( p[1] < 1 )
		return;

	NF_OBJECT_LOCK( _nf_record );

	for(i=0; i<p[1];i++){
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);
		g_return_if_fail(pevt->rule_id >= 0 && pevt->rule_id< VCA_MAX_ELEMS);

		if( pevt->type ){
			*rise_val |= 1 << pevt->ch;
			++_nf_record->notify_data.cb_change_flag;
		}

		pevt++;
	}		
		
	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s rise[0x%08x]",  __FUNCTION__, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif
}

static void
record_ai_generic_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	guint *rise_val;
	
	gint i;

	g_return_if_fail(pinfo != NULL);

#ifdef DEBUG_NOTIFY_CB
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif

	ai_generic_event_t *pevt;
	int* p;

	p = pinfo->p.ptr;
	pevt = p+2;

	rise_val = &_nf_record->notify_data.cb_rise_vca_event;

	//captainnn
	if ( p[0] >= NUM_ACTIVE_CH)
		return;		/* Skip events generated by smart search. */

	if( p[1] < 1 )
		return;

	NF_OBJECT_LOCK( _nf_record );

	for(i=0; i<p[1];i++){
		g_return_if_fail(pevt);
		g_return_if_fail(pevt->ch < NUM_ACTIVE_CH);

		*rise_val |= 1 << pevt->ch;
		++_nf_record->notify_data.cb_change_flag;
		
		pevt++;
	}		
		
	NF_OBJECT_UNLOCK( _nf_record );
#ifdef DEBUG_NOTIFY_CB
	g_message("%s rise[0x%08x]",  __FUNCTION__, *rise_val);
#endif

#ifdef	QUICK_REFRESH_TBL
	if(_nf_record->notify_data.cb_change_flag)
		nf_record_refresh_tbl(NF_RECORD_REF_RECORD_NOTIFY_CB_FUNC);	
#endif
}


static gboolean
record_refresh_tbl_timer(gpointer data)
{	
	//g_message("ggggg=================================================");	
	nf_record_refresh_tbl(NF_RECORD_REF_RECORD_REFRESH_TBL_TIMER);
	return TRUE;
}


#ifdef ENABLE_SYSDB_CHANGE_START_TIME

static gboolean
record_sysdb_changed_restart_timer(gpointer data)
{	
	g_message("%s nf_record_start called!!");	
	nf_record_start(NULL);
	return FALSE;
}

#endif


#ifdef ENABLE_VLOSS_RECOVERY
static gboolean
record_recover_tbl_timer(gpointer data)
{	
	static unsigned int cnt = 0;		
#if 0
	nf_live_recover_vloss( 1 << (cnt++ % NUM_ACTIVE_CH) );  // "onoff test" sync failed recovery
#else
	nf_live_recover_vloss( 0x3398ffff );  // "onoff test" sync failed recovery
#endif	
	return TRUE;
}
#endif

static void 
_free_icodec_data(ICODEC_HEADER *ih) 
{
	GobjBuddyBuffer *buf;
	int length;

	length = ih->frame_size + sizeof(ICODEC_HEADER);
	buf = gobj_buddy_buffer_new();
	gobj_buddy_buffer_buf_set_addr(buf, (guint8 *)ih);
	gobj_buddy_buffer_buf_set_size(buf, length);
	g_object_unref(buf);
}




const unsigned char * m_pStart;
unsigned short m_nLength;
int m_nCurrentBit;
unsigned int ReadBit()
{
    //g_assert(m_nCurrentBit <= m_nLength * 8);
    //if(m_nCurrentBit <= m_nLength * 8) {
    if(m_nCurrentBit > m_nLength * 8) {
		printf("%s:%d: m_nCurrentBit : %d,  m_nLength: %d * 8 : %d\n", __FUNCTION__, __LINE__,
			m_nCurrentBit, m_nLength, m_nLength*8);
    }
    int nIndex = m_nCurrentBit / 8;
    int nOffset = m_nCurrentBit % 8 + 1;

    m_nCurrentBit ++;
    return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
}

unsigned int ReadBits(int n)
{
    int r = 0;
    int i;
    for (i = 0; i < n; i++)
    {
        r |= ( ReadBit() << ( n - i - 1 ) );
    }
    return r;
}

unsigned int ReadExponentialGolombCode()
{
    int r = 0;
    int i = 0;

    while( (ReadBit() == 0) && (i < 32) )
    {
        i++;
    }

    r = ReadBits(i);
    r += (1 << i) - 1;
    return r;
}

unsigned int ReadSE() 
{
    int r = ReadExponentialGolombCode();
    if (r & 0x01)
    {
        r = (r+1)/2;
    }
    else
    {
        r = -(r/2);
    }
    return r;
}

void Parse(const unsigned char * pStart, unsigned short nLen, int *p_w, int *p_h)
{
    m_pStart = pStart;
    m_nLength = nLen;
    m_nCurrentBit = 0;

    int frame_crop_left_offset=0;
    int frame_crop_right_offset=0;
    int frame_crop_top_offset=0;
    int frame_crop_bottom_offset=0;

    int profile_idc = ReadBits(8);          
    int constraint_set0_flag = ReadBit();   
    int constraint_set1_flag = ReadBit();   
    int constraint_set2_flag = ReadBit();   
    int constraint_set3_flag = ReadBit();   
    int constraint_set4_flag = ReadBit();   
    int constraint_set5_flag = ReadBit();   
    int reserved_zero_2bits  = ReadBits(2); 
    int level_idc = ReadBits(8);            
    int seq_parameter_set_id = ReadExponentialGolombCode();


    if( profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118 )
    {
        int chroma_format_idc = ReadExponentialGolombCode();

        if( chroma_format_idc == 3 )
        {
            int residual_colour_transform_flag = ReadBit();         
        }
        int bit_depth_luma_minus8 = ReadExponentialGolombCode();        
        int bit_depth_chroma_minus8 = ReadExponentialGolombCode();      
        int qpprime_y_zero_transform_bypass_flag = ReadBit();       
        int seq_scaling_matrix_present_flag = ReadBit();        

        if (seq_scaling_matrix_present_flag) 
        {
            int i=0;
            for ( i = 0; i < 8; i++) 
            {
                int seq_scaling_list_present_flag = ReadBit();
                if (seq_scaling_list_present_flag) 
                {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j=0;
                    for ( j = 0; j < sizeOfScalingList; j++) 
                    {
                        if (nextScale != 0) 
                        {
                            int delta_scale = ReadSE();
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = ReadExponentialGolombCode();
    int pic_order_cnt_type = ReadExponentialGolombCode();
    if( pic_order_cnt_type == 0 )
    {
        int log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
    }
    else if( pic_order_cnt_type == 1 )
    {
        int delta_pic_order_always_zero_flag = ReadBit();
        int offset_for_non_ref_pic = ReadSE();
        int offset_for_top_to_bottom_field = ReadSE();
        int num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
        int i;
        for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            ReadSE();
            //sps->offset_for_ref_frame[ i ] = ReadSE();
        }
    }
    int max_num_ref_frames = ReadExponentialGolombCode();
    int gaps_in_frame_num_value_allowed_flag = ReadBit();
    int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
    int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();

	printf("%s:%d: 	pic_width_in_mbs_minus1: %d, pic_height_in_map_units_minus1: %d\n", __FUNCTION__, __LINE__, 
		pic_width_in_mbs_minus1, pic_height_in_map_units_minus1);

	*p_w = pic_width_in_mbs_minus1;
	*p_h = pic_height_in_map_units_minus1;

    int frame_mbs_only_flag = ReadBit();
    if( !frame_mbs_only_flag )
    {
        int mb_adaptive_frame_field_flag = ReadBit();
    }
    int direct_8x8_inference_flag = ReadBit();
    int frame_cropping_flag = ReadBit();
    if( frame_cropping_flag )
    {
        frame_crop_left_offset = ReadExponentialGolombCode();
        frame_crop_right_offset = ReadExponentialGolombCode();
        frame_crop_top_offset = ReadExponentialGolombCode();
        frame_crop_bottom_offset = ReadExponentialGolombCode();
    }
    int vui_parameters_present_flag = ReadBit();
 //   pStart++;

    int Width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
    int Height = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);

    printf("\nWxH = %dx%d\n",Width,Height); 
	
 

	if(vui_parameters_present_flag) {
		g_message("KJH - vui_parameters_present_flag : TRUE");
		
		int ar_present = ReadBit();   // aspect_ratio_info_present_flag
		if (ar_present) {
			int ar_idc = ReadBits(8);     // aspect_ratio_idc
			if (ar_idc == 255) {
				ReadBits(16);               // sar.num
				ReadBits(16);               // sar.den
			}
		}

		int overscan = ReadBit();     // overscan_info_present_flag
		if (overscan)
			ReadBit();                  // overscan_appropriate_flag

		int vid_sig_type = ReadBit(); // video_signal_type_present_flag
		if (vid_sig_type) {
			int video_format;
			int video_full_range_flag;
			int colour_description_present_flag;

			video_format = ReadBits(3);
			video_full_range_flag = ReadBit();
			colour_description_present_flag = ReadBit();

			g_message("KJH - vid_sig_type:TRUE => video_format:%d, video_full_range_flag:%d, colour_description_present_flag:%d"
				,video_format, video_full_range_flag, colour_description_present_flag);

			if (colour_description_present_flag) {

			g_message("KJH - %d", __LINE__);
				
//				sps.primaries = ReadBits(8);
//				sps.trc = ReadBits(8);
//				sps.colorspace = ReadBits(8);
				ReadBits(8);
				ReadBits(8);
				ReadBits(8);
			}
		}

		int chroma_loc_info_present_flag = ReadBit();
		if (chroma_loc_info_present_flag) {

			g_message("KJH - %d", __LINE__);
			
			ReadExponentialGolombCode();
			ReadExponentialGolombCode();
			/* uint32_t chroma_sample_loc_type_top_field = */ //br.ue();
			/* uint32_t chroma_sample_loc_type_bottom_field = */ //br.ue();
		}

		int num_units_in_tick;
		int time_scale;
		int fixed_frame_rate_flag;
		
		int timing_info_present_flag = ReadBit();
		if (timing_info_present_flag) {
			num_units_in_tick = ReadBits(32);
			time_scale = ReadBits(32);
			fixed_frame_rate_flag = ReadBit();

			g_message("KJH - timing_info_present_flag : TRUE => num_units_in_tick:%d, time_scale:%d, fixed_frame_rate_flag:%d"
				,num_units_in_tick, time_scale, fixed_frame_rate_flag);
		}
		else
		{
			g_message("KJH - timing_info_present_flag : FALSE");
		}
	}
	else{
		g_message("KJH - vui_parameters_present_flag : FALSE");
	}
}

typedef struct _RAW_TIMESTAMP{
	guchar 		timestampl;			/* 5 msec sub tick for uTimeStamp */
	guint 		timestamp;			/* Second tick since 1970 GMT */
} RAW_TIMESTAMP;

typedef struct _RAW_LIST{
	int num;
	int tot_fps[10];
	RAW_TIMESTAMP start[10];
	RAW_TIMESTAMP end[10];
	char raw_name[10][256];
} RECORD_RAW_LIST;

static gint _calc_fps( gint fps )
{
    gint val;

    if( DISPLAY_IS_PAL ) {
        switch(fps)
        {
            case NF_FPS_CR32:   val = 25; break;
            case NF_FPS_CR16:   val = 12; break;
            case NF_FPS_CR08:   val = 6; break;
            case NF_FPS_CR04:   val = 3; break;
            case NF_FPS_CR02:   val = 2; break;
            case NF_FPS_CR01:   val = 1; break;
            case NF_FPS_CR00:   val = 0;  break;
            default:
                __A(0, "wrong fps.");
                val = 0;  
                break;
        }
    }
    else {  /*NTSC*/
        switch(fps)
        {
            case NF_FPS_CR32:   val = 30; break;
            case NF_FPS_CR16:   val = 15;  break;
            case NF_FPS_CR08:   val = 7;  break;
            case NF_FPS_CR04:   val = 3; break;
            case NF_FPS_CR02:   val = 2;  break;
            case NF_FPS_CR01:   val = 1;  break;
            case NF_FPS_CR00:   val = 0;  break;
            default:
                __A(0, "wrong fps.");
                val = 0;  
                break;
        }
    }
    
    return val;
}

static gint _export_raw_data(int ch, GTimeVal start_time, GTimeVal end_time, char *raw_name, RECORD_RAW_LIST *raw_list)
{
	GTimeVal 	search_time;
	int sst_id;
	int ret;

	FILE *fp = NULL;

	NF_FPS_E v_fps = NF_FPS_CR00;
	NF_RESOLUTION_E v_size = NF_RES_NTSC_NONE;
	int i = -1;
	char tmp_name[256];

	g_message("%s => CH:%d, Start:%u.%u, End:%u.%u", __FUNCTION__, ch,
		start_time.tv_sec, start_time.tv_usec, end_time.tv_sec, end_time.tv_usec);

	memset( raw_list, 0x0, sizeof(RECORD_RAW_LIST));	

	search_time = start_time;
/*
	time_t ltime;
	struct tm st_buff;
	struct tm *st = &st_buff;

	ltime = start_time.tv_sec;
	localtime_r(&ltime, st);
					
	g_message("%s start_time [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
			st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
			st->tm_hour, st->tm_min, st->tm_sec);			


	ltime = search_time.tv_sec;
	localtime_r(&ltime, st);

	g_message("%s search_time [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
			st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
			st->tm_hour, st->tm_min, st->tm_sec);


	ltime = end_time.tv_sec;
	localtime_r(&ltime, st);

	g_message("%s end_time [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
			st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
			st->tm_hour, st->tm_min, st->tm_sec);
*/			
	sst_id = sst_play_open_ex( 0, ch, 
							1, 
							0, 
							0,
							0,							
//							1,
							GTIMEVAL_TO_GUINT64(start_time),											
							GTIMEVAL_TO_GUINT64(end_time),											
							GTIMEVAL_TO_GUINT64(search_time ) , 
							PB_TYPE_REMOTE, 0, 0);

	if( sst_id == -SST_ERR_NODATA){				
		g_message("%s - SST_ERR_NODATA", __FUNCTION__);
		return 0;				
	}else if( sst_id < 0) {
		g_message("%s - SST_OPEN_ERROR", __FUNCTION__);
		return 0;
	}

	while(1)
	{
		ICODEC_HEADER	frame;
		
		ret = sst_play_check_frame( sst_id, &frame);
		if(ret == 0){ 				
//			g_message("%s - %d", __FUNCTION__, __LINE__);
			break;
		}else if(ret == -SST_ERR_ENDDATA){
			g_message("%s - first check frame => END DATA", __FUNCTION__);
//			break;
			goto error_send_ctrl;
		}else if(ret == -SST_ERR_EMPTY){
//			g_message("%s - %d", __FUNCTION__, __LINE__);		
			g_usleep(10*1000);
			continue;
		}else{
			g_message("%s - first check frame => ERROR", __FUNCTION__);			
			goto error_send_ctrl;
		}
	}

	g_usleep(100000);
	
//	snprintf(tmp_name, sizeof(tmp_name), "%s_%d", raw_name, i);

//	fp = fopen( tmp_name, "wb");
	
	while(1)
	{
		ICODEC_HEADER *ih_frame;
		char *buf;		
		
		ret = sst_play_get_frame(sst_id, &ih_frame, 5000);  //wait 5second		
		if(ret<0){
			g_message("%s - get frame => DONE", __FUNCTION__);
			goto error_send_ctrl;
		}
		
		buf = ((char *)ih_frame)+sizeof(ICODEC_HEADER);

//		g_message("%s - %d - type:%d, size:%d ,buf:%x", __FUNCTION__, __LINE__, ih_frame->frame_type, ih_frame->frame_size, buf);

		if(ih_frame->frame_size != 0)
		{
			if( ih_frame->resolution != v_size || ih_frame->frame_rate != v_fps )
			{
				if(fp){
					fclose(fp);
					fp = NULL;

					g_message("%s FP close", __FUNCTION__);
				}

				g_message("%s - RES:%d, FPS:%d", __FUNCTION__, ih_frame->resolution, ih_frame->frame_rate);

				v_size = ih_frame->resolution;
				v_fps = ih_frame->frame_rate;

				i++;

				snprintf(tmp_name, sizeof(tmp_name), "%s_%d", raw_name, i);
				fp = fopen( tmp_name, "wb");

				g_message("%s FP Open : %s", __FUNCTION__, tmp_name);

				(raw_list->num)++;
				
				raw_list->start[i].timestamp  = ih_frame->timestamp;
				raw_list->start[i].timestampl = ih_frame->timestampl;
				
				sprintf(raw_list->raw_name[i], "%s", tmp_name);
			}
			
/*
{
	static int h264_w[32] = { 0, };
	static int h264_h[32] = { 0 , };
	int t_w, t_h;
	int time_scale;
	int num_units_in_tick;

	if( ih_frame->frame_type == NF_FRAME_TYPE_I) {
		Parse(&buf[5], ih_frame->frame_size, &t_w, &t_h);



#if 0
		if (h264_w[data->chan] != t_w || h264_h[data->chan] != t_h)
			{
				printf("[%s] resolution changed!!!!!!!!! ch[%02d]    pre[%d:%d]   cur[%d:%d]\n", __FUNCTION__, (int)(data->chan),
					h264_w[data->chan],
					h264_h[data->chan], t_w, t_h);

				h264_w[data->chan] = t_w;
				h264_h[data->chan] = t_h;
			}
#endif			
	}
	
}
*/
			raw_list->end[i].timestamp  = ih_frame->timestamp;
			raw_list->end[i].timestampl = ih_frame->timestampl;
			(raw_list->tot_fps[i])++;

//			g_message("%s RAW EXPORT END => SEC:%u, USEC:%u", __FUNCTION__, ih_frame->timestamp, ih_frame->timestampl);
			
			fwrite( buf, ih_frame->frame_size, 1, fp);
		}

		_free_icodec_data(ih_frame); 
		ih_frame = NULL;

		while(1) // get new header
		{
			ICODEC_HEADER	frame;
			
			ret = sst_play_check_frame(sst_id, &frame);
			if(ret == 0){
	
			 	break;
			}else if(ret == -SST_ERR_ENDDATA){
//				g_message("%s - %d", __FUNCTION__, __LINE__);											
				break;
			}else if(ret == -SST_ERR_EMPTY){
//				g_message("%s - %d", __FUNCTION__, __LINE__);			
				g_usleep(10*1000);
				continue;
			}else{
				g_message("%s - second check frame => ERROR", __FUNCTION__);
				goto error_send_ctrl;
			}
		}
	}

error_send_ctrl:

	ret = sst_play_close(sst_id);

	if(fp)
	{
		fclose(fp);
		g_message("%s FP close", __FUNCTION__);		
	}

	return 0;
}

int _remove_raw_list(RECORD_RAW_LIST *raw_list)
{
	int i;
	
	if( raw_list->num > 0 )
	{
		for( i=0; i < raw_list->num; i++ )
		{
			remove( raw_list->raw_name[i] );
		}
	}

	return TRUE;
}

static int _convert_raw_to_mp4(char *fps, char *raw_name, char *mp4_name)
{
	char cmd[512];

	if(access(raw_name, F_OK) != 0)
		return -1;
/*
	snprintf(cmd, sizeof(cmd), 
		"gst-launch-0.10 --gst-debug=0,mp4mux:5,GST_PLUGIN_LOADING:0,GST_REGISTRY:0,GST_CAPS:0 filesrc location=%s blocksize=100000 ! video/x-h264 ! h264parse output-format=sample access-unit=true ! mp4mux ! filesink location=%s",
		raw_name, mp4_name);
*/	
	snprintf(cmd, sizeof(cmd), 
		"/NFDVR/ffmpeg -loglevel panic -y -r %s -i %s -vcodec copy %s", fps, raw_name, mp4_name);
	
	proxy_system(cmd, 1, 10);	

	return 0;
}

static double _get_average_fps(int tot_fps, RAW_TIMESTAMP start, RAW_TIMESTAMP end)
{
	guchar timestampl;			/* 5 msec sub tick for uTimeStamp */
	guint  timestamp;

	double ret;
	double res;

	g_message("%s START => SEC:%u, USEC:%u", __FUNCTION__, start.timestamp, start.timestampl);			
	g_message("%s END => SEC:%u, USEC:%u", __FUNCTION__, end.timestamp, end.timestampl);
	g_message("%s TOT_FPS => %d", __FUNCTION__, tot_fps);
	
	if( start.timestampl > end.timestampl )
	{
		timestamp = (end.timestamp - 1) - start.timestamp;
		timestampl = (end.timestampl + 200) - start.timestampl;
	}
	else
	{
		timestamp = end.timestamp - start.timestamp;
		timestampl = end.timestampl - start.timestampl;
	}

	g_message("%s duration => %u.%u", __FUNCTION__, timestamp, (guint)timestampl * 5);

	res = (double)timestamp + (double)(timestampl * 5.0) / 1000.0;

	ret = ((double)tot_fps) / res;

	return ret;
}

int _make_mp4_by_time(int ch, GTimeVal start_time, GTimeVal end_time, char *mp4_name, NF_RECORD_MP4_LIST *mp4_list)
{
	char raw_name[256];
	int ret;
	int i;
	char file_name[256];
	char tmp_name[256];

	RECORD_RAW_LIST raw_list;

	double tran_fps;
	char fps[256];

	g_message("%s => want mp4_name : %s", __FUNCTION__, mp4_name);

	memset(mp4_list, 0x0, sizeof(NF_RECORD_MP4_LIST));
	snprintf(raw_name, sizeof(raw_name), "/tmp/raw_data");

	ret = _export_raw_data(ch, start_time, end_time, raw_name, &raw_list);

	if(ret < 0)
		return -1;

	if( raw_list.num > 0 )
	{
		char *p;
		
		snprintf(file_name, sizeof(file_name), "%s", mp4_name);
		
		p = strstr(file_name, ".mp4");

		if(p)
			*p = 0;

		for(i=0; i < raw_list.num; i++)
		{
			g_message("%s - IDX:%d, raw_name:%s", __FUNCTION__, i, raw_list.raw_name[i]);
		}
		
		if( raw_list.num == 1)
		{
			snprintf(tmp_name, sizeof(tmp_name), "%s.mp4", file_name);			

			tran_fps = _get_average_fps(raw_list.tot_fps[0], raw_list.start[0], raw_list.end[0]);
			snprintf(fps, sizeof(fps), "%.1f", tran_fps);
			
			g_message("%s  => FPS : %s", __FUNCTION__, fps);

			_convert_raw_to_mp4( fps ,raw_list.raw_name[0], tmp_name);

			(mp4_list->num)++;
			sprintf(mp4_list->mp4_name[0], "%s", tmp_name);		
		}
		else
		{	
			for(i=0; i < raw_list.num; i++)
			{
				snprintf(tmp_name, sizeof(tmp_name), "%s_%d.mp4", file_name, i);
				
				tran_fps = _get_average_fps(raw_list.tot_fps[i], raw_list.start[i], raw_list.end[i]);
				snprintf(fps, sizeof(fps), "%.1f", tran_fps);

				g_message("%s  => IDX:%d , FPS : %s", __FUNCTION__, i, fps);


				(mp4_list->num)++;
				sprintf(mp4_list->mp4_name[i], "%s", tmp_name);
			}
		}

		_remove_raw_list(&raw_list);
	}

	return 0;	
}

int _remove_mp4_list(NF_RECORD_MP4_LIST *mp4_list)
{
	int i;
	
	if( mp4_list->num > 0 )
	{
		for( i=0; i < mp4_list->num; i++ )
		{
			remove( mp4_list->mp4_name[i] );
		}
	}

	return TRUE;
}

static char reccord_get_sst_frame_help[] = "reccord_get_sst_frame [CH]";
static int reccord_get_sst_frame(int argc, char **argv)
{	
	GTimeVal  		start_time;		// play start time
	GTimeVal 		search_time;	// play search time
	GTimeVal 		end_time;		// play end time
	NF_RECORD_MP4_LIST mp4_list;
	
	int ch;
	char filename[256];
	int ret;

	if(argc < 2){
		printf("%s\n", reccord_get_sst_frame_help);
		return -1;
	}

	ch = atoi(argv[1]);

	g_message("%s - %d", __FUNCTION__, __LINE__);

//	ch += 16;			

	snprintf(filename, sizeof(filename),"/tmp/test_%d.mp4", ch);

	gettimeofday(&end_time, NULL);	

	start_time.tv_sec = end_time.tv_sec - 10;
	start_time.tv_usec = end_time.tv_usec;

	ret = _make_mp4_by_time(ch, start_time, end_time, filename, &mp4_list);
	if( ret < 0 )
	{
		g_message("%s - %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_message("%s - %d", __FUNCTION__, __LINE__);
	}

	return 0;
}
__commandlist(reccord_get_sst_frame, "reccord_get_sst_frame", reccord_get_sst_frame_help, reccord_get_sst_frame_help);

#ifdef ENABLE_CALENDAR_OPTIMIZE

#define LOG_RECORD_CH_NUM 				127
#define LOG_RECORD_FRAME_SIZE   		64
#define LOG_RECORD_BIG_FRAME_SIZE   	( (1024*16) - sizeof(ICODEC_HEADER) )

static inline gint _is_no_write();	

typedef struct _PROC_FILE_LIST_T {
	char	filename[64];
	int		mode;
} PROC_FILE_LIST;

// _start_vlaue_element_handler FIXME!!
PROC_FILE_LIST _proc_file_list[] = {
	{"/proc/ifs"                   ,0 },
	{"/proc/ifs_bio_wq"            ,0 },
	{"/proc/ifs_io_state"          ,0 },
	{"/proc/ifs_error_report"      ,0 },

//	{"/proc/ifs_mblg_list_ex"      ,0 },	
//	{"/proc/ifs_rec_pb_status"     ,0 },
	{"/proc/ifs_recording_status"  ,0 },
	{"/proc/cmem"                  ,0 },
	
	{"/proc/meminfo"               ,0 },
	{"/proc/diskstats"             ,0 },
	{"/proc/interrupts"            ,0 },
	{"/proc/vmstat"                ,0 },
	
	{"/proc/net/arp"               ,0 },	
	{"/proc/net/netstat"           ,0 },	
	{"/proc/net/sockstat"          ,0 },
	{"/proc/net/tcp"               ,0 },
		
	{"/proc/net/udp"               ,0 },
	{"/tmp/webra-info/notify_dump.txt"  ,0 },
	{"/tmp/webra-info/cam_stat.txt"  ,0 }
		
};

const int _proc_file_cnt = sizeof(_proc_file_list)/sizeof(PROC_FILE_LIST);

static int _proc_dump(const char *filename, char *buff)
{
	gchar *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;
  	
	if (!g_file_get_contents (filename, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return -1;
	}
	
	if(contents)
	{
		snprintf(buff, LOG_RECORD_BIG_FRAME_SIZE-1, 
						"proc_dump[%s]\n%*.*s", filename, length, length, contents);
						
		//g_print("%s", buff);
		g_free(contents);		
	}	
	return 0;
}

static gpointer
_record_log_malloc_frame( gint size, guint time_ts){

	static guint put_frame_cnt = 0;
	
	GobjBuddyBuffer	*gst_buf = NULL;
	ICODEC_HEADER 		*icodec_h = NULL;
			
	gst_buf = gobj_buddy_buffer_new_malloc( sizeof(ICODEC_HEADER) + size );

	g_return_val_if_fail( gst_buf != NULL, NULL);	
		
	icodec_h = (ICODEC_HEADER*)gobj_buddy_buffer_buf_get_addr(gst_buf);

	memset( gobj_buddy_buffer_buf_get_addr(gst_buf), 0x00, sizeof(ICODEC_HEADER) + size );
	
	_proc_dump (  _proc_file_list[ put_frame_cnt % _proc_file_cnt ].filename,
					gobj_buddy_buffer_buf_get_addr(gst_buf)+sizeof(ICODEC_HEADER) );				
	++put_frame_cnt;
	
	icodec_h->chan = LOG_RECORD_CH_NUM;
	icodec_h->codec = NF_CODEC_TYPE_H264MP;
	icodec_h->flags = 1;
	icodec_h->version = 1;
	icodec_h->frame_size = size;
	icodec_h->frame_type = NF_FRAME_TYPE_I;
	icodec_h->resolution = NF_RES_NTSC_CIF;
	icodec_h->frame_rate = NF_FPS_CR01;
	icodec_h->gst_buffer = gst_buf;
				
	icodec_h->timestamp  = time_ts;
    icodec_h->timestampl = 0;
    
	return gst_buf;		
}

static guint _record_log_flush_check_ts = 0;
static guint _record_log_last_time_ts  = 0;

static void 
_record_log_stream_process() {
	
	int i, ret, is_record = 0;
	int stream_id;	
	guint curr_ts_sec = 0;
		

	if( !_is_no_write() ) { 
		
		for(i=0; i<NUM_TOTAL_CHANNEL; ++i)
		{
			if(	_nf_record->man[i].record_reason > NF_RECORD_REASON_NOTHING
				&& _nf_record->man[i].record_reason < NF_RECORD_REASON_PRE
				&& _nf_record->sst[0][i].is_put_iframe ) {
				is_record = 1;
				break;
			}
		}
	}

	if( is_record ) {		
		for(i=0; i<NUM_TOTAL_CHANNEL; ++i)
		{
			if(	_nf_record->man[i].record_reason > NF_RECORD_REASON_NOTHING
				&& _nf_record->man[i].record_reason < NF_RECORD_REASON_PRE 
				&& _nf_record->sst[0][i].is_put_iframe 
				&& _nf_record->sst[0][i].last_iframe_header.timestamp > _record_log_last_time_ts ) {			
				curr_ts_sec = _nf_record->sst[0][i].last_iframe_header.timestamp;
			}
		}
		
		if( curr_ts_sec == 0 ) {
			//g_message("%s cal_ts skip[%d]", __FUNCTION__, _record_log_last_time_ts);
			return;
		} else {
			_record_log_last_time_ts = curr_ts_sec;
		}
	}
				
	if( is_record ) {		

		gint is_first_open_frame = 0;
		
		if( _nf_record->log_stream_id == -1 && nf_record_is_rec_off() != TRUE )
		{
			stream_id = sst_record_open( (guint8)LOG_RECORD_CH_NUM,
										(guint8)0,
										(guint8)NF_RECORD_REASON_TIMER,
										(guint8)NF_CODEC_TYPE_H264MP,
										(guint8)NF_RES_NTSC_CIF,
										(guint8)NF_FPS_CR01,
										(guint8)0);

			//g_message("sst_record_open  ch[%2d] sid[%2d]",LOG_RECORD_CH_NUM, stream_id);			
			g_assert( stream_id >= 0 || stream_id == -SST_ERR_DISKFULL );	// sid ���� ���и� g_assert
			_nf_record->log_stream_id = stream_id;
		}
				
		if( _nf_record->log_stream_id > 0)
		{			
			GobjBuddyBuffer	*gst_buf = NULL; 
			ICODEC_HEADER 		*pheader = NULL;
			gst_buf = _record_log_malloc_frame( LOG_RECORD_BIG_FRAME_SIZE, _record_log_last_time_ts); 			
			if( gst_buf ) {	
				guint curr_ts = 0;
				pheader = (ICODEC_HEADER*)gobj_buddy_buffer_buf_get_addr(gst_buf);
				
				ret = sst_record_put_frame( _nf_record->log_stream_id, pheader );
				if(ret){
					//g_message("sst_record_put_frame result[%d](%s)\n", ret, sst_get_error_string(ret));
					g_assert( ret == -SST_ERR_DISKFULL || ret == -SST_ERR_INVPARAM );
					g_object_unref(gst_buf);															
			    }else{
			    				    				    	
			    	// for FAST FLUSH
			    	if( _record_log_flush_check_ts != _nf_record->current_date )
			    	{
						//g_message("sst_record_close ch[%2d] sid[%2d] FAST_FLUSH", LOG_RECORD_CH_NUM, _nf_record->log_stream_id); 
						ret = sst_record_close( _nf_record->log_stream_id, 0);
						g_assert( ret == 0);
						_nf_record->log_stream_id = -1;

						_record_log_flush_check_ts = _nf_record->current_date;
			    	}			    	
			    }			    			    			    
			}else{				
				g_warning("%s cmem_fail", __FUNCTION__);								
			}
		}
		
	}else{
		if( _nf_record->log_stream_id != -1 )
		{
			//g_message("sst_record_close ch[%2d] sid[%2d]", LOG_RECORD_CH_NUM, _nf_record->log_stream_id); 
			ret = sst_record_close( _nf_record->log_stream_id, 0);
			g_assert( ret == 0);
			
			_nf_record->log_stream_id = -1;
		}
	}		
}

static char rec_caldump_help[] = "rec_caldump [time] [count] [key]";
static int
rec_caldump(int argc, char **argv)
{
	
	GTimeVal start_time, end_time, search_time;
	ICODEC_HEADER *ih_frame = NULL;
	
	guint sid = -1;	
	guint time = 0x0;
	guint count = 100;
	gchar *str = NULL;
	
	gint ret;	
			
	if ( argc < 2 ) {
		printf("%s\n", rec_caldump_help);
		return -1;
	}

	time = atoi(argv[1]);
	
	if( argc > 2)
		count = atoi(argv[2]);

	if( argc > 3)
		str = argv[3];
		
	start_time.tv_sec = time;
	start_time.tv_usec = 0;
	
	search_time.tv_sec = time;
	search_time.tv_usec = 0;

	end_time.tv_sec  = 0;
	end_time.tv_usec = 0;
			
	sid  = sst_play_open_ex( 0, LOG_RECORD_CH_NUM, 
				1, //param->rate, 
				0, //param->direction, 
				0, //param->rate_ctrl,	// 0
				0, //param->hide,		// 0
				GTIMEVAL_TO_GUINT64(start_time),
				GTIMEVAL_TO_GUINT64(end_time),
				GTIMEVAL_TO_GUINT64(search_time),
				PB_TYPE_REMOTE, 0, 0);

	if( sid == -SST_ERR_NODATA){				
		g_message("%s sst_play_open NODATA",__FUNCTION__);
		return 0;
	
	}else if( sid < 0) {
		g_warning("%s sst_play_open ret[%d]",__FUNCTION__, sid);
		return 0;
	}

	while(--count)
	{
		char *buf;
		char tmp[64], time_str[64];
		
		ih_frame = NULL;
		ret = sst_play_get_frame( sid, &ih_frame, 5000);  //wait 5second		
		if(ret<0)
		{
			g_warning("%s sst_play_get_frame ret[%d]", __FUNCTION__, ret);
			goto error_send_ctrl;
		}
									
		buf = ((char *)ih_frame)+sizeof(ICODEC_HEADER);	
			
		if( str && ih_frame->frame_type == NF_FRAME_TYPE_I){						
			strncpy(tmp, buf, sizeof(tmp)-1 );
			if( strstr(tmp, str) == NULL )
			{
				_free_icodec_data(ih_frame);
				g_usleep(10*1000);
				continue;
			}
		}	
						
		ctime_r(&ih_frame->timestamp, time_str);
	
		ipx_printf("\n========== [%d][%6d] [%s][%d]\n", 
					ih_frame->timestamp,
					ih_frame->frame_size,					
					time_str, ih_frame->timestamp - time);

		if( ih_frame->frame_type == NF_FRAME_TYPE_I) 			
			ipx_printf_large("%s", buf);
						
		_free_icodec_data(ih_frame);
		
		g_usleep(10*1000);		

	}
			
error_send_ctrl:		
	if(sid >= 0) {
		g_message("%s sst_play_close sid[%d]",__FUNCTION__, sid);
		sst_play_close(sid);		
	}
	
	ipx_printf("\n--> rec_caldump %d %d end\n", time, count);
				
	return 0;
}
__commandlist(rec_caldump, "rec_caldump", "rec caldata dump", rec_caldump_help);

#endif // ENABLE_CALENDAR_OPTIMIZE


struct timeval record_frame_end_tv = { 0, 0};

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

//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	
static void
record_thread_func (NfRecord * self)
{
	
	gpointer		que_poped_data = NULL;	
	GobjBuddyBuffer		*frame;
	ICODEC_HEADER	*pheader;
	gint 			i;

	gint			frame_cnt = 0;
	guint			prev_timestamp = 0;
	guint			curr_timestamp = 0;

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

		cpu_core_set(3);
    }

	self->init_done = 1;
		
	// wait init complete
	while( _nf_record == NULL ) g_usleep(10*1000);

	// wait for first nf_record_start() calling	
	while( self->record_man_start == 0)
		g_usleep(10*1000);

{		
	_get_sysdb_linked_cam_all_data();
	_get_sysdb_linked_pos_all_data();
//	_get_sysdb_linked_dva_all_data();
	
	guint vloss = nf_notify_get_param0("vloss");
    guint64 alarm = (nf_notify_get_param1("sensor") & 0xffffffff) | ((unsigned long long)nf_notify_get_param0("sensor") << NUM_ALARM_IPCAM);
	guint motion = nf_notify_get_param0("motion");

	g_message("%s INIT vloss[0x%08x] alarm[0x%016llx] motion[0x%08x]", __FUNCTION__, vloss, alarm, motion);
	
	// FIXME HNF ( add ipcam status )
	for(i=0;i<NUM_ACTIVE_CH;++i)
	{
		guint ch_mask = (1<<i);
				
		_nf_record->man[i].vloss = (vloss & ch_mask) ? 1:0;
		_nf_record->man[i].alarm = _is_check_linked_cam_event( i, alarm );
		_nf_record->man[i].motion =	(motion & ch_mask) ? 1:0;
	}		
}

#ifdef ENABLE_VLOSS_RECOVERY
	nf_timer_add( 60*1000, record_recover_tbl_timer, NULL);
#endif
		
#ifndef	DEBUG_RECORD_DISABLE_TBL_TIMER
	nf_timer_add( 1000, record_refresh_tbl_timer, NULL);
#else
	g_warning("%s DEBUG_RECORD_DISABLE_TBL_TIMER", __FUNCTION__);
	g_warning("%s DEBUG_RECORD_DISABLE_TBL_TIMER", __FUNCTION__);
	g_warning("%s DEBUG_RECORD_DISABLE_TBL_TIMER", __FUNCTION__);
#endif	 		

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_RECORD, NF_WATCHDOG_TIME_SEC*12, NF_WATCHDOG_ENABLE);
#endif
		
	while(self->thread_run)
	{	
				
		que_poped_data = g_async_queue_pop( self->queue);
		
		if( que_poped_data == NULL)	// get_data
			continue;

		if( !GOBJ_IS_BUDDY_BUFFER(que_poped_data) )
		{
	        printf("[%s][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__);
			continue;
		}

		frame = (GobjBuddyBuffer *)que_poped_data;
		pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);
		
		struct timeval record_frame_curr_tv;	

		gettimeofday(&record_frame_curr_tv, NULL);
		
		test_delay_calculater("record_thread_get", record_frame_end_tv, record_frame_curr_tv, 2);

		record_frame_end_tv = record_frame_curr_tv;
		
		// control frame
		if(	pheader->chan == 0xff
				&& pheader->reserved == 0x01234567
				&& pheader->timestamp == 0x89ABCDEF
				&& pheader->frame_size == 0 )
		{

			NF_RECORD_NOTIFY_DATA	notify_data;
						
			gint date, hour, sched_mode;
			time_t tick;
			struct tm cur_cal_time;		

			tick = time(NULL);
			nf_datetime_localtime(&tick, _nf_record->is_dst, &cur_cal_time);

			gettimeofday(&_nf_record->current_time, NULL);

			sched_mode = _get_rec_sched();
/*					
			date = ( sched_mode == NF_RECORD_SCHEDULE_DAILY ) ? 
							NF_RECORD_SCHEDULE_ANYDAY : cur_cal_time.tm_wday;
*/
			if( sched_mode == NF_RECORD_SCHEDULE_DAILY )
				date = NF_RECORD_SCHEDULE_ANYDAY;
			else{
				if( scm_is_holiday(cur_cal_time.tm_year	+ 1900, cur_cal_time.tm_mon + 1, cur_cal_time.tm_mday) > 0 )
					date = NF_RECORD_SCHEDULE_HOLIDAY;
				else
					date = cur_cal_time.tm_wday;
			}
//			printf("[DEBUG] Y[%d] M[%d] D[%d] H[%d] W[%d] DATE[%d]\n", cur_cal_time.tm_year, cur_cal_time.tm_mon, cur_cal_time.tm_mday, cur_cal_time.tm_hour, cur_cal_time.tm_wday, date);
								
			hour = cur_cal_time.tm_hour;
			
			_nf_record->sched_mode = sched_mode;
			
										// 20111013
			_nf_record->current_date = (cur_cal_time.tm_year+1900)*1000000 
										+ (cur_cal_time.tm_mon+1)*10000 
										+ cur_cal_time.tm_mday* 100
										+ cur_cal_time.tm_hour;
										
			_nf_record->current_week = date;
			_nf_record->current_hour = hour;

			if( pheader->flags != NF_RECORD_REF_RECORD_REFRESH_TBL_TIMER )
			{
				
				_nf_record->pre_rec_time = _get_pre_rec_time();
				if(_nf_record->pre_rec_time)
					_nf_record->pre_rec_time += 1;	// IPCAM GOP problem
					
				_nf_record->post_rec_time = _get_post_rec_time();				
				if(_nf_record->post_rec_time)
					_nf_record->post_rec_time += 1;	// IPCAM GOP problem
	
				_nf_record->record_mode = _get_rec_mode();
				_nf_record->record_auto_mode = _get_rec_auto_config();
				_nf_record->panic_time = _get_panic_time();
			}


			if( pheader->flags == NF_RECORD_REF_SYSDB_CHANGED )
			{
				_get_sysdb_linked_cam_all_data();		
				_get_sysdb_linked_pos_all_data();
//				_get_sysdb_linked_dva_all_data();
			}
			
			{	// clear error_cnt;
				static gint prev_hour = 0;
				
				if( prev_hour != _nf_record->current_hour )
				{
					prev_hour = _nf_record->current_hour;
					for (i = 0; i < NUM_TOTAL_CHANNEL; ++i) {
						_nf_record->sst[0][i].err_frame_count = 0;
						_nf_record->sst[1][i].err_frame_count = 0;
					}
				}
			}

			{	// time change 				
				if( pheader->flags == NF_RECORD_REF_RECORD_STOP )
				{
					for (i = 0; i < NUM_TOTAL_CHANNEL; ++i) {
						memset( &_nf_record->sst[0][i].last_header, 0x00, sizeof(ICODEC_HEADER));
						memset( &_nf_record->sst[1][i].last_header, 0x00, sizeof(ICODEC_HEADER));
					}				
					_nf_record->panic_rec_timer = 0;					
					curr_timestamp = prev_timestamp = 0;					
				}								
#ifdef	ENABLE_CALENDAR_OPTIMIZE
				if( pheader->flags == NF_RECORD_REF_RECORD_STOP ){
					_record_log_flush_check_ts = 0;
					_record_log_last_time_ts = 0;
				}
#endif
			}
						
			NF_OBJECT_LOCK( _nf_record );
			{					
				memcpy(&notify_data, &_nf_record->notify_data, sizeof(notify_data));
					
				_nf_record->notify_data.cb_rise_alarm = 0;
				_nf_record->notify_data.cb_rise_vloss = 0;
				_nf_record->notify_data.cb_rise_motion = 0;
				
				_nf_record->notify_data.cb_rise_user_event = 0;
				_nf_record->notify_data.cb_rise_va_event = 0;

				_nf_record->notify_data.cb_rise_pos_event = 0;
				_nf_record->notify_data.cb_rise_manual_event = 0;				

				_nf_record->notify_data.cb_rise_vca_event = 0;

				_nf_record->notify_data.cb_rise_dva_event = 0;
								
				_nf_record->notify_data.cb_change_flag = 0;			
	
				_refresh_tbl_flag = 0;
			}
			NF_OBJECT_UNLOCK( _nf_record );

			if( notify_data.cb_change_flag  || pheader->flags == NF_RECORD_REF_SYSDB_CHANGED )
			{	
				for (i = 0; i < NUM_ACTIVE_CH; ++i) 
				{
					guint ch_mask = (1<<i);
					guint alarm_ch_mask = ch_mask;
					
					_nf_record->man[i].vloss = (notify_data.cb_curr_vloss & ch_mask) ? 1:0;
					_nf_record->man[i].alarm = _is_check_linked_cam_event( i, (notify_data.cb_rise_alarm | notify_data.cb_curr_alarm));
					_nf_record->man[i].motion = ((notify_data.cb_rise_motion | notify_data.cb_curr_motion) & ch_mask) ? 1:0;
#ifdef ENABLE_USER_VA_EVENT
					_nf_record->man[i].alarm |= (notify_data.cb_rise_user_event & ch_mask) ? 1:0;
					_nf_record->man[i].alarm |= (notify_data.cb_curr_user_event & ch_mask) ? 1:0;
#endif	
					_nf_record->man[i].alarm |= _is_check_linked_pos_event( i, notify_data.cb_rise_pos_event);

					_nf_record->man[i].alarm |= ((notify_data.cb_rise_manual_event | notify_data.cb_curr_manual_event) & ch_mask) ? 1:0;

//					_nf_record->man[i].alarm |= _is_check_linked_dva_event( i, notify_data.cb_rise_dva_event);
					_nf_record->man[i].alarm |= (notify_data.cb_rise_dva_event & ch_mask) ? 1:0;

					_nf_record->man[i].alarm |= (notify_data.cb_rise_vca_event & ch_mask) ? 1:0;					
				}
			}

			NF_OBJECT_LOCK(_nf_record);
			if( _nf_record->panic_time 
				&& _nf_record->manual_rec 
				&& _nf_record->record_off == 0
				&& _nf_record->panic_rec_timer == 0  ) {
				_load_panic_timer( _nf_record->panic_time );
			}
			
			// panic auto stop !!
			if( _nf_record->panic_time && _nf_record->panic_rec_timer && _dec_panic_timer() ) {
				_nf_record->manual_rec = 0;
				_nf_record->panic_rec_timer = 0;
				_panic_set(_nf_record->manual_rec);
			}			
			NF_OBJECT_UNLOCK(_nf_record);

			if( pheader->flags == NF_RECORD_REF_SYSDB_CHANGED
				&& _nf_record->record_off == 0)
			{
				g_message("%s SYSDB_CHANGED gop toggle ========", __FUNCTION__);
				
				_nf_record->record_off = 1;
#ifdef ENABLE_SYSDB_CHANGE_START_TIME
				nf_timer_add( 3000, record_sysdb_changed_restart_timer, NULL);	
#else
				_timer_simple_record(0);


				_nf_record->record_off = 0;
#endif

				
				g_message("%s SYSDB_CHANGED gop toggle ========", __FUNCTION__);
			}
									
			_timer_simple_record(0);
			
			if( notify_data.cb_change_flag || pheader->flags == NF_RECORD_REF_SYSDB_CHANGED )
			{
				for (i = 0; i < NUM_ACTIVE_CH; ++i) 
				{
					guint ch_mask = (1<<i);
					guint alarm_ch_mask = ch_mask;															
					_nf_record->man[i].alarm = _is_check_linked_cam_event( i, notify_data.cb_curr_alarm );						
					_nf_record->man[i].motion = (notify_data.cb_curr_motion & ch_mask) ? 1:0;
#ifdef ENABLE_USER_VA_EVENT
					_nf_record->man[i].alarm |= (notify_data.cb_curr_user_event & ch_mask) ? 1:0;
#endif
					_nf_record->man[i].alarm |= (notify_data.cb_curr_manual_event & ch_mask) ? 1:0;	
				}
			}	

#ifdef ENABLE_CALENDAR_OPTIMIZE
			//g_message("%s RecMan CTRL FRAME[%d] [%d][%d]========", __FUNCTION__, pheader->flags, _record_log_flush_check_ts, _record_log_last_time_ts );	
			_record_log_stream_process();
#endif
			g_object_unref(frame);

#ifdef ENABLE_WATCHDOG
			nf_watchdog_kick( NF_WATCHDOG_MEMBER_RECORD );
#endif						

#ifdef SUPPORT_VCA_CAMERA
			//captainnn
			memcpy(nf_meta->man,_nf_record->man, sizeof(_nf_record->man));
			nf_meta->is_dst = _nf_record->is_dst;
#endif
			continue;			
		}

		curr_timestamp = pheader->timestamp;
		
		// live handoff		
		if( (pheader->frame_type == NF_FRAME_TYPE_P 
					|| pheader->frame_type == NF_FRAME_TYPE_I )
					&& _nf_record->handoff_func
					&& _nf_record->handoff_ch_mask & ((long long)1 << pheader->chan) )	{

			guint phy_chan = pheader->chan;
			guint vir_chan = phy_chan % BASE_IPCAM_2ND_CHANNEL;				

			//pheader->chan = vir_chan;
			pheader->flags = _nf_record->man[vir_chan].record_reason;
			_nf_record->handoff_func(frame);
						
		}
		
#ifdef USE_RTSP4VLC
		if( pheader->frame_type == NF_FRAME_TYPE_P || pheader->frame_type == NF_FRAME_TYPE_I )
			nf_nvs_get_frame_func( frame );
#endif
		
		if( _DEBUG_RECORD_log[ DEBUG_RECORD_IDX_SKIP_SST] )
		{
			g_object_unref(frame);			
		}else{

            if(pheader->chan< BASE_AUDIO_CHANNEL) {

				guint phy_chan = pheader->chan;
				guint vir_chan = phy_chan % BASE_IPCAM_2ND_CHANNEL;				
				gboolean is_2nd = (phy_chan >= BASE_IPCAM_2ND_CHANNEL) ? 1 : 0;

			 						
#ifdef DEBUG_RECORD_LOG
				if( _DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_FRAME] & (1<<pheader->chan) )
					dump_icodec_header("REC", pheader);
#endif				
#ifdef DEBUG_RECORD_LOG
				if( _DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP] 
					&& pheader->frame_size >0 
					&& (_DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP_CH_MASK] & (1<<pheader->chan) ) )
				{		
					dump_frame_data( pheader); 
				}	
#endif	 			
				_record_update_profile( pheader, 0); // MAIN

				if( pheader->frame_type == NF_FRAME_TYPE_END)
					dump_icodec_header( "END", pheader); 

				//pheader->chan = vir_chan;
				pheader->flags = _nf_record->man[vir_chan].record_reason;							
				record_queue_process_ipcam_1st(frame, vir_chan, is_2nd );
				record_queue_process_ipcam_2nd(frame, vir_chan, is_2nd );

            }
            
			g_object_unref(frame);			
		}						

		++frame_cnt;
		
#ifdef ENABLE_WATCHDOG
		if( curr_timestamp - prev_timestamp > 0  )
		{
			nf_watchdog_kick( NF_WATCHDOG_MEMBER_RECORD );
			prev_timestamp = curr_timestamp;

#ifdef ENABLE_CALENDAR_OPTIMIZE
			_record_log_stream_process();
#endif

		}
#endif

		if( !(frame_cnt & 0x3f ) )
			sleep(0);	// yield schedule			
						
	}
	g_message("%s end", __FUNCTION__);
	g_thread_exit(0);
}


static void
_record_update_profile( ICODEC_HEADER *pheader, gint stream_idx )
{
	gint ch = 0;
	
	g_return_if_fail( _nf_record != NULL );
	
	g_return_if_fail ( pheader );
	
	ch = pheader->chan;
	g_return_if_fail ( ch < NF_DSPCOMM_NUM_CH );
			
	gulong frame_kbyte = pheader->frame_size >> 10;

	if( pheader->frame_type == NF_FRAME_TYPE_I 
		|| 	pheader->frame_type == NF_FRAME_TYPE_P )
	{
		_nf_record->stat.tot_state[ ch ].tv_sec = pheader->timestamp;

		++_nf_record->stat.gop_idx[ch]; 
		_nf_record->stat.gop_idx[ch] %= NF_DSPCOMM_GOP_CNT;

		memcpy ( &(_nf_record->stat.gop_arr[ch][_nf_record->stat.gop_idx[ch]]),
					 pheader, sizeof(ICODEC_HEADER));
		
	}else{
		return;	
	}

	if( pheader->frame_type == NF_FRAME_TYPE_I )
	{						
		++_nf_record->stat.tot_state[ ch ].i_cnt;
		_nf_record->stat.tot_state[ ch ].i_kbyte += frame_kbyte;			
		
	}
	else if( pheader->frame_type == NF_FRAME_TYPE_P )
	{
		++_nf_record->stat.tot_state[ ch ].p_cnt;
		_nf_record->stat.tot_state[ ch ].p_kbyte += frame_kbyte;
		
	}		
}

//_nf_record->dspcomm_thread[NUM_DSP];
static void 
_record_dump_profile( gint type ) // 0:curr 1:diff
{	
	gint i;
	
	static NF_DSPCOMM_STATE	tot_state[NF_DSPCOMM_NUM_CH];	
	static int init = 0;
	
	g_return_if_fail( _nf_record != NULL );
	
	if(init == 0 )
	{
		memset( tot_state, 0x00, sizeof(tot_state));
		init = 1;
	}
			
	if(type == 0)
	{
		printf("i_cnt, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].i_cnt);
		printf("\ni_kbyte, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].i_kbyte);
	
		printf("\np_cnt, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].p_cnt);
		printf("\np_kbyte, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].p_kbyte);
			
	}else{

		printf("i_cnt, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].i_cnt - tot_state[i].i_cnt);
		printf("\ni_kbyte, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].i_kbyte - tot_state[i].i_kbyte);
	
		printf("\np_cnt, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].p_cnt - tot_state[i].p_cnt);
		printf("\np_kbyte, ");	
		for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
			printf("%ld,", _nf_record->stat.tot_state[i].p_kbyte - tot_state[i].p_kbyte);
	
	}

	for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
	{
		tot_state[i].i_cnt = _nf_record->stat.tot_state[i].i_cnt;
		tot_state[i].p_cnt = _nf_record->stat.tot_state[i].p_cnt;
		tot_state[i].i_kbyte = _nf_record->stat.tot_state[i].i_kbyte;
		tot_state[i].p_kbyte = _nf_record->stat.tot_state[i].p_kbyte;
	}
		
	printf("\n");	
			
}

static int 
_record_dump_gop( gint ch, gint print )
{	
	gint i;
	gint i_cnt = 0;
	guint tot_size = 0;

	ICODEC_HEADER *pheader;
				
	g_return_val_if_fail( _nf_record != NULL ,0);
	g_return_val_if_fail( ch < NF_DSPCOMM_NUM_CH ,0);
	
	if( print )
		printf("GOP ch[%2d]=========\n", ch);
	
	for(i=0; i<NF_DSPCOMM_GOP_CNT; i++)
	{		
		pheader = &_nf_record->stat.gop_arr[ch][i];				
		if( print )
			printf("ch[%2d] [0x%02x] [0x%02x][0x%02x][0x%02x] [%d][%03d] [%6d]\n",
				pheader->chan, 
				pheader->flags, 
				pheader->frame_type, 
				pheader->frame_rate, 
				pheader->resolution, 				
				pheader->timestamp,
				pheader->timestampl,
				pheader->frame_size	);
				
		tot_size += pheader->frame_size;

		if( pheader->frame_type == NF_FRAME_TYPE_I)
			++i_cnt;
	}								

	printf("GOP ch[%2d]========= tot_size[%8d] avg[%7d] est[%8d][%d] \n", ch, 
				tot_size, tot_size/NF_DSPCOMM_GOP_CNT, 
				tot_size/NF_DSPCOMM_GOP_CNT*480,i_cnt);
	
	return tot_size;
			
}

#ifdef DEBUG_RECORD_JBSHELL

static char rec_manual_event_help[] = "rec_manual_event [ch] [on:off]";
static int rec_manual_event(int argc, char **argv)
{			
	gint ch = 0;
	gint onoff = 0;

	if(argc < 3){
		printf("%s\n",rec_manual_event_help);
		return -1;
	}

	ch = atoi(argv[1]);	
	onoff = atoi(argv[2]);

	nf_record_alarm_by_manual_event(ch, onoff);
	
	return 0;
}
__commandlist(rec_manual_event,"rec_manual_event",rec_manual_event_help, rec_manual_event_help);

static char rec_skip_sst_help[] = "rec_skip_sst [onoff]";
static int rec_skip_sst(int argc, char **argv)
{			
	gint onoff = 0;

	if(argc < 2){
		printf("%s\n",rec_skip_sst_help);
		return -1;
	}
	onoff = strtol(argv[1],NULL,0);
	
	_DEBUG_RECORD_log[ DEBUG_RECORD_IDX_SKIP_SST]= onoff;
	
	return 0;
}
__commandlist(rec_skip_sst,"rec_skip_sst",rec_skip_sst_help, rec_skip_sst_help);



static char rec_onoff_help[] = "rec_onoff [onoff] [reset]";
static int rec_onoff(int argc, char **argv)
{	
	gint onoff = 0, reset = 0;
	gint i;
		
	if(argc < 2){
		printf("%s\n",rec_onoff_help);
		return -1;
	}
	
	onoff = strtol(argv[1],NULL,0);
		
	if(argc > 2) reset = strtol(argv[2],NULL,0);

	if(onoff)
		nf_record_start(NULL);
	else
		nf_record_stop(NULL);

	g_return_val_if_fail( _nf_record != NULL, -1 );
	
	return 0;
}
__commandlist(rec_onoff,"rec_onoff",rec_onoff_help, rec_onoff_help);



static char rec_panic_help[] = "rec_panic [onoff]";
static int rec_panic(int argc, char **argv)
{			
	gint onoff = 0;

	if(argc < 2){
		printf("%s\n",rec_panic_help);
		return -1;
	}
	onoff = strtol(argv[1],NULL,0);

	if(onoff)
		nf_panic_record_start(NULL);
	else
		nf_panic_record_stop(NULL);
		
	return 0;
}
__commandlist(rec_panic,"rec_panic",rec_panic_help, rec_panic_help);



static char rec_status_help[] = "rec_stat [type]";
static int rec_status(int argc, char **argv)
{			
	int type = 0;

	if(argc > 1) 
		type = strtol(argv[1],NULL,0);
	
	_record_dump_profile( type );
			
	return 0;
}
__commandlist(rec_status,"rec_stat",rec_status_help, rec_status_help);



static char rec_gop_help[] = "rec_gop [ch_mask] [print]";
static int rec_gop(int argc, char **argv)
{	
	guint ch_mask = 0;
	guint gop_header_print = 1;
	gint i;
	guint tot_size = 0;
						
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",rec_gop_help);
		return -1;
	}
	
	ch_mask = strtoul(argv[1],NULL,0);
	if(argc > 2) 
		gop_header_print = strtol(argv[2],NULL,0);
				
	for(i=0; i<NF_DSPCOMM_NUM_CH; i++)
	{
		if( ch_mask & (1<<i) )
		{
			tot_size = _record_dump_gop(i, gop_header_print);
		}
	}
		
	return 0;
}
__commandlist(rec_gop,"rec_gop", rec_gop_help, rec_gop_help);


static char rec_tbl_help[] = "rec_tbl";
static int rec_tbl(int argc, char **argv)
{	

	g_return_val_if_fail( _nf_record != NULL, -1 );

	_dump_rec_table();
	
	return 0;
}
__commandlist(rec_tbl,"rec_tbl", rec_tbl_help, rec_tbl_help);



static char rec_manlog_help[] = "rec_manlog [on/off]";
static int rec_manlog(int argc, char **argv)
{	

	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",rec_manlog_help);
		return -1;
	}
		
	_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN] = strtol(argv[1],NULL,0);
	
	return 0;
}
__commandlist(rec_manlog,"rec_manlog", rec_manlog_help, rec_manlog_help);



static char rec_chlog_help[] = "rec_chlog [ch_mask]";
static int rec_chlog(int argc, char **argv)
{	

	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",rec_chlog_help);
		return -1;
	}
		
	_DEBUG_RECORD_log[DEBUG_RECORD_IDX_LOG_CH_MASK] = strtoul(argv[1],NULL,0);
	
	return 0;
}
__commandlist(rec_chlog,"rec_chlog", rec_chlog_help, rec_chlog_help);


static char rec_log_help[] = "rec_log [idx] [val]";
static int rec_log(int argc, char **argv)
{	
	gint idx, val,i;
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 3){
		printf("%s\n",rec_log_help);		
		nf_debug_dump("record");
		return -1;
	}
		
	idx = strtol(argv[1],NULL,0);
	val = strtol(argv[2],NULL,0);
	
	g_return_val_if_fail( idx <= DEBUG_RECORD_IDX_NR, -1 );
	
	if( idx == DEBUG_RECORD_IDX_NR )
	{
		for(i=0;i<DEBUG_RECORD_IDX_NR;i++)		
			_DEBUG_RECORD_log[i] = val;
	}else{
		_DEBUG_RECORD_log[idx] = val;
	}
	
	return 0;
}
__commandlist(rec_log,"rec_log", rec_log_help, rec_log_help);


static char rec_file_dump_help[] = "rec_file_dump [ch_mask] [cnt]";
static int rec_file_dump(int argc, char **argv)
{	
	guint ch_mask = 0;
	guint frame_cnt = 0;
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 3){
		printf("%s\n",rec_file_dump_help);
		return -1;
	}
	ch_mask = strtoul(argv[1],NULL,0);
	frame_cnt = strtoul(argv[2],NULL,0);
	
	_DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP] = frame_cnt;
	_DEBUG_RECORD_log[DEBUG_RECORD_IDX_DUMP_CH_MASK] = ch_mask;		// ch 0	
	
	return 0;
}
__commandlist(rec_file_dump,"rec_file_dump", rec_file_dump_help, rec_file_dump_help);


static char rec_net_help[] = "rec_net [ch_mask]";
static int rec_net(int argc, char **argv)
{	
	guint ch_mask;
	gint ch;
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",rec_net_help);
		return -1;
	}
		
	ch_mask = strtoul(argv[1],NULL,0);
	
	for(ch=0;ch<NUM_ACTIVE_CH;++ch)
	{
		if( ch_mask & 1<<ch)
			_set_network_live(ch,1);			
		else
			_set_network_live(ch,0);			
	}	
	return 0;
}
__commandlist(rec_net,"rec_net", rec_net_help, rec_net_help);


static char sensor_help[] = "sensor [ch_mask]";
static int sensor(int argc, char **argv)
{	
	guint mask = 0;
	gchar buff[256];
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",sensor_help);
		return -1;
	}
	mask = strtoul(argv[1],NULL,0);	
	
	snprintf( buff, sizeof(buff), "notify_fire_params sensor 0x%08x", mask);
	my_command(buff);
		
	return 0;
}
__commandlist(sensor,"sensor", sensor_help, sensor_help);


static char motion_help[] = "motion [ch_mask] [cnt:1] [silent:0(usleep)]";
static int motion(int argc, char **argv)
{	
	guint mask = 0;
	guint repeat_cnt = 1, i;
	guint silent = 0;
	
	gchar buff[256];
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",motion_help);
		return -1;
	}
	mask = strtoul(argv[1],NULL,0);	

	if(argc > 2) 
		repeat_cnt = strtol(argv[2],NULL,0); 

	if(argc > 3) 
		silent = strtol(argv[3],NULL,0); 

	if( silent ){
		for(i=0; i<	repeat_cnt; ++i) {
			nf_notify_fire_params( "motion", mask + i, 0,0,0);
			g_usleep(silent);
		}						
	}else{
		for(i=0; i<	repeat_cnt; ++i)
		{
			snprintf( buff, sizeof(buff), "notify_fire_params motion 0x%08x", mask + i);
			my_command(buff);
		}
	}

	return 0;
}
__commandlist(motion,"motion", motion_help, motion_help);


static char vloss_help[] = "vloss [ch_mask]";
static int vloss(int argc, char **argv)
{	
	guint mask = 0;
	gchar buff[256];
	
	g_return_val_if_fail( _nf_record != NULL, -1 );

	if(argc < 2){
		printf("%s\n",vloss_help);
		return -1;
	}
	mask = strtoul(argv[1],NULL,0);	
	
	snprintf( buff, sizeof(buff), "notify_fire_params vloss 0x%08x", mask);
	my_command(buff);
		
	return 0;
}
__commandlist(vloss,"vloss", vloss_help, vloss_help);


#endif	//DEBUG_RECORD_JBSHELL


// sysdb
/******************************************************************************/

static gint _get_rec_mode()
{		
	return nf_sysdb_get_uint("rec.mode");
}
static gint _get_rec_auto_config()
{
	return nf_sysdb_get_uint("rec.auto_config");
}
static gint _get_rec_priority_mode()
{
	return nf_sysdb_get_uint("rec.priority_mode");
}
static gint _get_rec_sched()
{
	return nf_sysdb_get_uint("rec.sched_mode");
}
static gint _get_pre_rec_time()
{
#ifdef DEBUG_PRE_RECORD_OFF
	return 0; 
#else

#ifdef DEBUG_RECORD_PRE_REC_THREE_TIMES	
	g_warning("wowowowowowwo!!!!!!!!!!!!!! DEBUG_RECORD_PRE_REC_THREE_TIMES	");
	g_warning("wowowowowowwo!!!!!!!!!!!!!! DEBUG_RECORD_PRE_REC_THREE_TIMES	");
	g_warning("wowowowowowwo!!!!!!!!!!!!!! DEBUG_RECORD_PRE_REC_THREE_TIMES	");		
	return (nf_sysdb_get_uint("rec.pre_rec_time")*3);
#else 
	return nf_sysdb_get_uint("rec.pre_rec_time");
#endif
	
#endif	
}
static gint _get_post_rec_time()
{
#ifdef DEBUG_POST_RECORD_OFF
	return 0;
#else
	return nf_sysdb_get_uint("rec.post_rec_time");
#endif	
}
static gint _get_panic_time()
{
	return nf_sysdb_get_uint("rec.panic_time");
}

static void _load_post_timer(gint ch, gint sec)
{
	g_assert( ch>=0 && ch < NUM_CHANNEL);
	g_assert( sec>=0 && sec <= MAX_POST_REC_TIME);
					
	_nf_record->man[ch].post_rec_timer = _nf_record->current_time.tv_sec + sec;

#ifdef 	DEBUG_POST_REC_TIMER
	g_message("%s ch[%d] post_sec[%d] [%ld]->[%ld]", __FUNCTION__, ch, sec,				
				_nf_record->current_time.tv_sec,
				_nf_record->man[ch].post_rec_timer );
#endif				
	
}

static gint _dec_post_timer(gint ch)
{		
	g_assert( ch>=0 && ch < NUM_CHANNEL);

#ifdef 	DEBUG_POST_REC_TIMER	
	g_message("%s ch[%d] timer[%ld] vs curr[%ld]", __FUNCTION__, ch,
				_nf_record->man[ch].post_rec_timer, 
				_nf_record->current_time.tv_sec );
#endif

	if( _nf_record->man[ch].post_rec_timer >= _nf_record->current_time.tv_sec )
		return 0;
	else
		return 1;
}

static void _load_panic_timer( gint sec)
{
	g_assert( sec>=0 && sec <= 60*60);
		
	_nf_record->panic_rec_timer = _nf_record->current_time.tv_sec + sec;	
}

static gint _dec_panic_timer()
{		
	if( _nf_record->panic_rec_timer >= _nf_record->current_time.tv_sec )
		return 0;
	else
		return 1;
}

// onvif_porting
gint _convert_sysdb_fps(gint val)
// onvif_porting
{
	gint ret = NF_FPS_CR32;
	
	switch (val)
	{
		case 'A':	ret = NF_FPS_CR32; break;
		case 'B':	ret = NF_FPS_CR16; break;
		case 'C':	ret = NF_FPS_CR08; break;
		case 'D':	ret = NF_FPS_CR04; break;
		case 'E':	ret = NF_FPS_CR02; break;
		case 'F':	ret = NF_FPS_CR01; break;
		case 'G':	ret = NF_FPS_CR00; break;
		default:	g_message("%s [%c]", __FUNCTION__, val); g_assert(0);
	}						
	return ret;
}

// onvif_porting
gint _convert_sysdb_quality(gint val)
// onvif_porting
{
	gint ret = NF_QUALITY_LOW;
	switch (val)
	{
		case 'A':	ret = NF_QUALITY_SUPER;  	break;
		case 'B':	ret = NF_QUALITY_HIGHEST;	break;
		case 'C':	ret = NF_QUALITY_HIGH;		break;
		case 'D':	ret = NF_QUALITY_STANDARD;	break;
		case 'E':	ret = NF_QUALITY_LOW;		break;
//		default:	g_message("%s [%c]", __FUNCTION__, val); g_assert(0);
	}						
	return ret;
}
//onvif_porting
gint _convert_sysdb_resolution(gint val)
//onvif_porting
{
	gint ret = NF_RES_NTSC_NONE;
	switch (val)
	{
		case 'A':	ret = NF_RES_NTSC_NONE;		break;
		case 'B':	ret = NF_RES_NTSC_CIF;		break;
		case 'C':	ret = NF_RES_NTSC_2CIF;		break;
#if defined(_OTM_MODEL) || defined(_SNF_MODEL)	
		case 'D':	ret = NF_RES_NTSC_4CIFP;	break;
#else
		case 'D':	ret = NF_RES_NTSC_4CIF;		break;
#endif /* _OTM_MODEL */
		case 'E':	ret = NF_RES_NTSC_4CIFP;	break;
		case 'F':	ret = NF_RES_PAL_CIF;		break;
		case 'G':	ret = NF_RES_PAL_2CIF;		break;
		case 'H':	ret = NF_RES_PAL_4CIF;		break;
		case 'I':	ret = NF_RES_PAL_4CIFP;		break;
		case 'J':	ret = NF_RES_640x480;		break;
		case 'K':	ret = NF_RES_720x480;		break;
		case 'L':	ret = NF_RES_720x576;		break;
		case 'M':	ret = NF_RES_800x600;		break;
		case 'N':	ret = NF_RES_1024x768;		break;
		case 'O':	ret = NF_RES_1280x1024;		break;
		case 'P':	ret = NF_RES_1600x1200;		break;
		case 'Q':	ret = NF_RES_1280x720;		break;
		case 'R':	ret = NF_RES_1920x1080;		break;
		case 'S':	ret = NF_RES_640x352;		break;			
		case 'T':	ret = NF_RES_640x360;		break;
//		default:	g_message("%s [%c]", __FUNCTION__, val); g_assert(0);

		case 'U':	ret = NF_RES_640x360I;		break;
		case 'V':	ret = NF_RES_1280x720I;		break;
		case 'W':	ret = NF_RES_1920x1080I;	break;
		case 'X':	ret = NF_RES_640x400;		break;
		case 'Y':	ret = NF_RES_800x450;		break;
		case 'Z':	ret = NF_RES_1440x900;		break;
		case 'c':	ret = NF_RES_320x180;		break;

	// 2015-07-08 ���� 1:26:12 choissi
	// http://222.112.8.34:8080/browse/SWIPXVETHR-441
	
		case 'd': 	ret = NF_RES_2304x1296;		break;	// (3M) 16:9
		case 'e': 	ret = NF_RES_2048x1536;		break; 	// (3M) 4:3	QXGA 
		case 'f': 	ret = NF_RES_2560x1440;		break;	// (3.6M) 16:9 WQHD
		case 'g': 	ret = NF_RES_2688x1520;		break; 	// (4M) 16:9
		case 'h': 	ret = NF_RES_2560x1600;		break; 	// (4.1M) 16:10 WQXGA
		case 'i': 	ret = NF_RES_2560x1920;		break; 	// (5M) 4:3 5M IPCAM ActiveX ���� �ػ�
		case 'j': 	ret = NF_RES_2592x1920;		break; 	// (5M) 4:3 ���� �ػ� / �ڻ� 5M ���� / ���� ���� 5M�� ������ �� �ػ��� ���ɼ��� Ŀ�� �߰�
		case 'k': 	ret = NF_RES_2592x1944;		break;  // (5M) 4:3
		case 'l': 	ret = NF_RES_2992x1680;		break; 	// (5M) 16:9 (bosh)
		case 'm': 	ret = NF_RES_2880x1800;		break; 	// (5.2M) 16:10 
		case 'n': 	ret = NF_RES_3200x1800;		break;	// (5.7M) 16:9 WQXGA+			
		case 'o': 	ret = NF_RES_2880x2160;		break;	// (6M) 4:3
		case 'p': 	ret = NF_RES_3072x2048;		break; 	// (6M) 3:2  (hikvision)
		case 'q': 	ret = NF_RES_3200x2400;		break; 	// (7.7M) 4:3 QUXGA	 
		case 'r': 	ret = NF_RES_3840x2160;		break;	// (8M) 16:9
		case 's': 	ret = NF_RES_2592x1520;		break;	// (3.9M)	

		case '1': 	ret = NF_RES_3000x3000;		break;
		case '2': 	ret = NF_RES_2048x2048;		break; 
		case '3': 	ret = NF_RES_1280x1280;		break;
		case '4': 	ret = NF_RES_640x640;		break;
		case '5': 	ret = NF_RES_320x320;		break;
	}
	return ret;
}

#define SYSDB_CAM_AUDIO_CH			"cam.C%d.audio_ch"

#define SYSDB_REC_TIMER_MODE		"%s.continuous.C%d.mode"
#define SYSDB_REC_TIMER_AUDIO		"%s.continuous.C%d.audio"
#define SYSDB_REC_TIMER_FPS			"%s.continuous.C%d.fps"
#define SYSDB_REC_TIMER_QUALITY		"%s.continuous.C%d.quality"
#define SYSDB_REC_TIMER_SIZE		"%s.continuous.C%d.size"

#define SYSDB_REC_MOTION_MODE		"%s.motion.M%d.mode"
#define SYSDB_REC_MOTION_AUDIO		"%s.motion.M%d.audio"
#define SYSDB_REC_MOTION_FPS		"%s.motion.M%d.fps"
#define SYSDB_REC_MOTION_QUALITY	"%s.motion.M%d.quality"
#define SYSDB_REC_MOTION_SIZE		"%s.motion.M%d.size"

#define SYSDB_REC_ALARM_MODE		"%s.alarm.A%d.mode"    
#define SYSDB_REC_ALARM_AUDIO		"%s.alarm.A%d.audio"    
#define SYSDB_REC_ALARM_FPS			"%s.alarm.A%d.fps"    
#define SYSDB_REC_ALARM_QUALITY		"%s.alarm.A%d.quality"    
#define SYSDB_REC_ALARM_SIZE		"%s.alarm.A%d.size"    
#ifdef	ENABLE_ALARM_CH_MAP
#define SYSDB_REC_ALARM_CH_MAP		"%s.alarm.A%d.alarm_ch"    
#endif

#define SYSDB_REC_MANUAL_MODE		"%s.panic.mode"
#define SYSDB_REC_MANUAL_AUDIO		"%s.panic.audio"
#define SYSDB_REC_MANUAL_FPS		"%s.panic.fps"
#define SYSDB_REC_MANUAL_QUALITY	"%s.panic.quality"
#define SYSDB_REC_MANUAL_SIZE		"%s.panic.size"


#define SYSDB_AUTO_TEMPLATE_AUDIO		  "%s.audio"
#define SYSDB_AUTO_TEMPLATE_FPS			  "%s.fps"
#define SYSDB_AUTO_TEMPLATE_QUALITY		  "%s.quality"
#define SYSDB_AUTO_TEMPLATE_SIZE		  "%s.size"

#define SYSDB_AUTO_MOT				      "rec.auto.motion"
#define SYSDB_AUTO_ALARM			      "rec.auto.alarm"
#define SYSDB_AUTO_MOT_ALARM		      "rec.auto.motion_alarm"

#define SYSDB_AUTO_ITS_MOT_NORMAL   	  "rec.auto.m_intensive.normal"
#define SYSDB_AUTO_ITS_MOT_EVENT    	  "rec.auto.m_intensive.motion"
#define SYSDB_AUTO_ITS_MOT_AUDIO    	  "rec.auto.m_intensive"

#define SYSDB_AUTO_ITS_ALARM_NORMAL       "rec.auto.a_intensive.normal"
#define SYSDB_AUTO_ITS_ALARM_EVENT        "rec.auto.a_intensive.alarm"
#define SYSDB_AUTO_ITS_ALARM_AUDIO        "rec.auto.a_intensive"

#define SYSDB_AUTO_ITS_MOT_ALARM_NORMAL   "rec.auto.ma_intensive.normal"
#define SYSDB_AUTO_ITS_MOT_ALARM_EVENT    "rec.auto.ma_intensive.motion_alarm"
#define SYSDB_AUTO_ITS_MOT_ALARM_AUDIO    "rec.auto.ma_intensive"


/******************************************************************************/

static guint _get_cam_audio_in(gint ch)
{		
	char buff[256];
	
	if(ch<BASE_IPCAM_CHANNEL)
	{
		sprintf(buff, SYSDB_CAM_AUDIO_CH, ch);
		return nf_sysdb_get_uint( buff );
	}
	else{		
		return 0;
	}					
}

static gint _get_sysdb_strmap(const char *str, gint ch, gint date, gint hour)
{
	char buff[256];
	gint idx;
		
	sprintf(buff, str, ch < BASE_IPCAM_CHANNEL ? "rec":"iprec", date);
	idx = (ch%BASE_IPCAM_CHANNEL)*24 + hour;
#if 0
	g_message("%-32.32s ch[%2d] data[%d] hour[%2d] idx[%3d] [%c]", buff, ch, date, hour,
			idx, nf_sysdb_get_strmap(buff, idx) );
#endif
	
	return nf_sysdb_get_strmap(buff, idx);
}

//for manual
static gint _get_sysdb_strmap2(const char *str, gint ch)
{
	char buff[256];
	gint idx;
		
	sprintf(buff, str, ch < BASE_IPCAM_CHANNEL ? "rec":"iprec");
	
	return nf_sysdb_get_strmap(buff, ch%BASE_IPCAM_CHANNEL );
}


//for auto
static gint _get_sysdb_strmap3(const char *str, const char *tmpl,  gint ch)
{			
	char buff[256];
	
	sprintf(buff, tmpl, str);
	
	return nf_sysdb_get_strmap(buff, ch%BASE_IPCAM_CHANNEL );
}


static gint _get_timer_rec_onoff(gint ch, gint date, gint hour)
{		
	return _get_sysdb_strmap( SYSDB_REC_TIMER_MODE, ch, date, hour )-'0';
}
static gint _get_timer_rec_audio(gint ch, gint date, gint hour)
{	
	return _get_sysdb_strmap( SYSDB_REC_TIMER_AUDIO, ch, date, hour )-'0';
}
static gint _get_timer_rec_fps(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_fps( _get_sysdb_strmap( SYSDB_REC_TIMER_FPS, ch, date, hour ) );
}
static gint _get_timer_rec_quality(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_quality( _get_sysdb_strmap( SYSDB_REC_TIMER_QUALITY, ch, date, hour ) );
}
static gint _get_timer_rec_resolution(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_resolution( _get_sysdb_strmap( SYSDB_REC_TIMER_SIZE, ch, date, hour ) );
}

static gint _get_motion_rec_onoff(gint ch, gint date, gint hour)
{		
	return _get_sysdb_strmap( SYSDB_REC_MOTION_MODE, ch, date, hour )-'0';
}
static gint _get_motion_rec_audio(gint ch, gint date, gint hour)
{	
	return _get_sysdb_strmap( SYSDB_REC_MOTION_AUDIO, ch, date, hour )-'0';
}
static gint _get_motion_rec_fps(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_fps( _get_sysdb_strmap( SYSDB_REC_MOTION_FPS, ch, date, hour ) );
}
static gint _get_motion_rec_quality(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_quality( _get_sysdb_strmap( SYSDB_REC_MOTION_QUALITY, ch, date, hour ) );
}
static gint _get_motion_rec_resolution(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_resolution( _get_sysdb_strmap( SYSDB_REC_MOTION_SIZE, ch, date, hour ) );
}

static gint _get_alarm_rec_onoff(gint ch, gint date, gint hour)
{	
	return _get_sysdb_strmap( SYSDB_REC_ALARM_MODE, ch, date, hour )-'0';
}
static gint _get_alarm_rec_audio(gint ch, gint date, gint hour)
{	
	return _get_sysdb_strmap( SYSDB_REC_ALARM_AUDIO, ch, date, hour )-'0';
}
static gint _get_alarm_rec_fps(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_fps( _get_sysdb_strmap( SYSDB_REC_ALARM_FPS, ch, date, hour ) );
}
static gint _get_alarm_rec_quality(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_quality( _get_sysdb_strmap( SYSDB_REC_ALARM_QUALITY, ch, date, hour ) );
}
static gint _get_alarm_rec_resolution(gint ch, gint date, gint hour)
{	
	return _convert_sysdb_resolution( _get_sysdb_strmap( SYSDB_REC_ALARM_SIZE, ch, date, hour ) );
}
#ifdef ENABLE_ALARM_CH_MAP
static gint _get_alarm_ch_map(gint ch, gint date, gint hour)
{	
	return _get_sysdb_strmap( SYSDB_REC_ALARM_CH_MAP, ch, date, hour )-'0';
}
#endif

static gint _get_manual_rec_audio(gint ch)
{
	return _get_sysdb_strmap2(SYSDB_REC_MANUAL_AUDIO, ch)-'0';	
}
static gint _get_manual_rec_fps(gint ch)
{
	return _convert_sysdb_fps(_get_sysdb_strmap2(SYSDB_REC_MANUAL_FPS, ch));
}
static gint _get_manual_rec_quality(gint ch)
{
	return _convert_sysdb_quality(_get_sysdb_strmap2(SYSDB_REC_MANUAL_QUALITY, ch));
}
static gint _get_manual_rec_resolution(gint ch)
{
	return _convert_sysdb_resolution(_get_sysdb_strmap2(SYSDB_REC_MANUAL_SIZE, ch));
}


// for auto
static gint _get_auto_rec_audio(char *str, gint ch)
{
	return _get_sysdb_strmap3(str, SYSDB_AUTO_TEMPLATE_AUDIO, ch)-'0';	
}
static gint _get_auto_rec_fps(char *str, gint ch)
{
	return _convert_sysdb_fps(_get_sysdb_strmap3(str, SYSDB_AUTO_TEMPLATE_FPS,  ch));
}
static gint _get_auto_rec_quality(char *str, gint ch)
{
	return _convert_sysdb_quality(_get_sysdb_strmap3(str, SYSDB_AUTO_TEMPLATE_QUALITY, ch));
}
static gint _get_auto_rec_resolution(char *str, gint ch)
{
	return _convert_sysdb_resolution(_get_sysdb_strmap3(str, SYSDB_AUTO_TEMPLATE_SIZE, ch));
}


/******************************************************************************/

// current record info
static inline gint _is_timer_rec(gint ch)
{		
	return (_nf_record->man[ch].record_reason == NF_RECORD_REASON_TIMER);
}
static inline gint _is_alarm_rec(gint ch)
{
	return (_nf_record->man[ch].record_reason == NF_RECORD_REASON_ALARM);
}

gboolean nf_record_is_alarm_rec(gint ch)
{
	if( ch < 0 || ch >= NUM_TOTAL_CHANNEL )
	{
		g_message("%s => channel error", __FUNCTION__);
		return FALSE;
	}
	
	if( _is_alarm_rec(ch) )
		return TRUE;
	else
		return FALSE;
}

glong nf_record_get_panic_end_time(void)
{
	return _nf_record->panic_rec_timer;
}

static inline gint _is_motion_rec(gint ch)
{
	return (_nf_record->man[ch].record_reason == NF_RECORD_REASON_MOTION);
}
static inline gint _is_manual_rec(gint ch)
{
	return (_nf_record->man[ch].record_reason == NF_RECORD_REASON_MANUAL);
}
static inline gint _is_pre_rec(gint ch)
{
	return (_nf_record->man[ch].pre_record);
}

static inline gint _get_pre_rec_info(gint ch)
{
	return (_nf_record->man[ch].pre_record);
}
static inline void _set_pre_rec_info(gint ch, gint val)
{
	g_assert( ch >= 0 && ch < NUM_TOTAL_CHANNEL);	
#ifdef DEBUG_RECORD_LOG	
	if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN_REASON])
		g_message("%s ch[%2d] [%d]->[%d]", __FUNCTION__, 
				ch, _nf_record->man[ch].pre_record, val);
#endif

	_nf_record->man[ch].pre_record = val;		
}

static inline gint _get_rec_info(gint ch)
{
	return (_nf_record->man[ch].record_reason);
}
static inline void _set_rec_info(gint ch, gint rec_mode)
{
	g_assert( ch >= 0 && ch < NUM_TOTAL_CHANNEL);
	g_assert( rec_mode >=0 && rec_mode <=NF_RECORD_REASON_PRE);

#ifdef DEBUG_RECORD_LOG
	if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN_REASON])	
		g_message("%s ch[%2d] [%d]-->[%d]", __FUNCTION__,
			ch, _nf_record->man[ch].record_reason, rec_mode);	
#endif
	_nf_record->man[ch].record_reason = rec_mode;
	
}

static gint _is_priority_higher(gint mode, gint ch)
{
	switch (mode) {
		case NF_RECORD_REASON_TIMER:
			if (_is_alarm_rec(ch) || _is_motion_rec(ch) || 
					_is_manual_rec(ch) || _is_timer_rec(ch))
				return 0;
			else
				return 1;

		case NF_RECORD_REASON_ALARM:
			if (_is_alarm_rec(ch) || _is_manual_rec(ch)) {
				return 0;
			} else {
				return 1;
			}
		case NF_RECORD_REASON_MOTION:
			if (_is_motion_rec(ch) ||_is_alarm_rec(ch) || _is_manual_rec(ch)) {
				return 0;
			} else {
				return 1;
			}
		case NF_RECORD_REASON_MANUAL:
			if (_is_manual_rec(ch))
				return 0;
			else
				return 1;
		default:
			g_assert(0);
			break;
	}
	return 0;
}

static gint _is_priority_higher2(gint mode, gint ch) // for always high quality
{
	switch (mode) {
		case NF_RECORD_REASON_TIMER:
			if ( _is_manual_rec(ch) || _is_timer_rec(ch) )
				return 0;
			else
				return 1;

		case NF_RECORD_REASON_MANUAL:
			if (_is_manual_rec(ch))
				return 0;
			else
				return 1;

		default:
			g_assert(0);
			break;
	}
	return 0;
}

static inline gint _is_manual_on()
{	
	return (_nf_record->manual_rec);
}

static inline gint _is_alarm_on(gint ch)
{	
	return (_nf_record->man[ch].alarm);
}

static inline gint _is_motion_on(gint ch)
{	
	return (_nf_record->man[ch].motion);
}

static inline gint _is_vloss(gint ch)
{	
	return (_nf_record->man[ch].vloss);
}

static inline gint _is_network_live(gint ch)
{
	return (_nf_record->man[ch].network);
}

static inline void _set_network_live(gint ch, gint val)
{
	++_nf_record->req_network;
	_nf_record->man[ch].network = val;	
}

static inline gint _is_no_write()
{
	return (_nf_record->record_off == 1);	
}

gboolean nf_record_is_rec_off(void)
{
	if( _nf_record->record_off == 1 )
		return TRUE;
	else
		return FALSE;
}

static inline void _set_no_write(gint val)
{
	_nf_record->record_off = val;
}

static gint _send_rec_start_log(gint ch, gint param2)
{

#ifdef DEBUG_RECORD_LOG
	if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN_LOG])
		g_message("%s ch[%2d] param2[%d] ONNNNN", __FUNCTION__, ch, param2);
#endif		
	return 0;
}

static gint _send_rec_stop_log(gint ch, gint param2)
{

#ifdef DEBUG_RECORD_LOG
	if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN_LOG])
		g_message("%s ch[%2d] param2[%d] OFFFFF", __FUNCTION__, ch, param2);
#endif
	return 0;
}
// onvif_porting
gint _conv_fps_real( gint val)
// onvif_porting
{
	
	if( DISPLAY_IS_PAL )
	{
		switch(val)
		{
			case NF_FPS_CR32:	return 25;
			case NF_FPS_CR16:	return 12;
			case NF_FPS_CR08:	return 6;
			case NF_FPS_CR04:	return 3;
			case NF_FPS_CR02:	return 2;
			case NF_FPS_CR01:	return 1;
			case NF_FPS_CR00:	return 0;
			default:	g_message("%s [%d]", __FUNCTION__, val); g_assert(0);
		}
	}else{
		switch(val)
		{
			case NF_FPS_CR32:	return 30;
			case NF_FPS_CR16:	return 15;
			case NF_FPS_CR08:	return 7;
			case NF_FPS_CR04:	return 3;
			case NF_FPS_CR02:	return 2;
			case NF_FPS_CR01:	return 1;
			case NF_FPS_CR00:	return 0;
			default:	g_message("%s [%d]", __FUNCTION__, val); g_assert(0);
		}		
	}
	return 0;
}

static const gchar RECORD_REASON_STR[16]={' ','T','A','M','U','P','p',' ',};
static gint _notify_rec_mode(guint changeflag)
{
	static gchar last_analog_rec[NUM_CHANNEL+1];	
	gchar analog_rec[NUM_CHANNEL+1];
	gint  ch;
	gint  curr_fps = 0, tot_fps = 0;
		
	memset(analog_rec, 0x00, sizeof (analog_rec));
	
	for(ch=0;ch<NUM_ACTIVE_CH; ch++)
	{				
		if( (_nf_record->man[ch].record_reason == NF_RECORD_REASON_PRE) )
			analog_rec[ch] = 'p';
		else
			analog_rec[ch] = RECORD_REASON_STR[ _nf_record->man[ch].record_reason ]; 
					
		if( analog_rec[ch] != ' ' )
		{
			// FIXME choissinf 2008-12-30 ���� 11:31:21
			curr_fps += _conv_fps_real(_nf_record->man[ch].fps) * _nf_record->man[ch].resolution;
		}
	}
	
	if( memcmp( analog_rec, last_analog_rec, sizeof(analog_rec)) != 0)	
	{
		if( DISPLAY_IS_PAL)
			tot_fps = 25 * NUM_ANALOG_CHANNEL;
		else
			tot_fps = 30 * NUM_ANALOG_CHANNEL;
			
		nf_notify_fire_chmap( "analog_rec", analog_rec);
		nf_notify_fire_params( "enc_status", curr_fps,0,tot_fps,0);
		
		memcpy( last_analog_rec, analog_rec, sizeof(analog_rec) );
	}
		
	return 0;	
}


typedef struct _RECORD_INFO_T {
		
	unsigned char channel_id;
	unsigned char flags; //recording flag same as icodec_header flag
	unsigned char quality; //HIGHEST, HIGH, MEDIUM, LOW
	unsigned char fps; //same as icodec_header fps
	unsigned char res;
	unsigned char codec;
	
	//unsigned char reserved[2];	// 2009-02-23 ���� 3:58:15 choissinf
	unsigned char audio;
	unsigned char audio_ch;	

	// 2012-12-16 ���� 8:22:55 for mobile
	unsigned char quality_2nd; //HIGHEST, HIGH, MEDIUM, LOW
	unsigned char fps_2nd; //same as icodec_header fps
	
} RECORD_INFO;

#define DSP_MAX_CHAN	32

typedef struct _DRREQ_RECORD_START_T {
	RECORD_INFO record_info[DSP_MAX_CHAN];
} DRREQ_RECORD_START;


static gint _ipcam_convert_fps (gint fps)
{

	gint ret = NF_IPCAM_FPS_300;
	
	switch ( fps )
	{
		case NF_FPS_CR32: ret = NF_IPCAM_FPS_300; break;
		case NF_FPS_CR16: ret = NF_IPCAM_FPS_150; break;
		case NF_FPS_CR08: ret = NF_IPCAM_FPS_70; break;
		case NF_FPS_CR04: ret = NF_IPCAM_FPS_30; break;
		case NF_FPS_CR02: ret = NF_IPCAM_FPS_20; break;
		default: ret = NF_IPCAM_FPS_300;
	}
	
	return ret;			
}
				
static gint _send_rec_cmd(guint changeflag)
{
	gint ret = 0;
	gint dspid, ch=0, i;	
	
	static guint req_cnt = 0;	
	static DRREQ_RECORD_START req[2];
	
	gint	new_req_idx = req_cnt & 0x1;
	gint	old_req_idx = new_req_idx ? 0:1;
	
	gint	pre_rec_sec = _nf_record->pre_rec_time;
	gint	gop_toggle = 0;	
	guint   change_mask = changeflag;
	
#if 0
		R: record flag, P: prerecord flag
		E: END_FRAME, S: START_FRAME
		P: Prerecroding
		PD: Prerecord discard
		PF: Prerecord flush
		========================
		new  | cur  RP RP RP RP
		RP   |      00 01 10 11
		-----|------------------
		00   |      X  E  E  E
		     |         PD
		-----|------------------
		01   |      S  ES ES ES
		     |      P  PD P  P  
		-----|------------------
		10   |      S  ES ES ES
		     |         PD
		-----|------------------
		11   |      S  ES ES ES
		     |         PF
		========================
#endif
	
	// FIXME	pre_rec_time change ^>^
	
	//g_message("%s   req[%d][%d][%d]",__FUNCTION__, req_cnt, new_req_idx, old_req_idx); 
	//memset(&req[new_req_idx], 0x00, sizeof(DRREQ_RECORD_START));			

	for(ch=0;ch<NUM_ACTIVE_CH; ch++)
	{	
		gint	pre_rec_enable = 0;

#ifdef DEBUG_RECORD_LOG
		if( _DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN] 
				&& changeflag & (1<<ch) )
			g_message("%s ch[%2d] reason[%d]pre[%d]pre_sec[%d]net[%d]", __FUNCTION__,  ch, 
		 			_nf_record->man[ch].record_reason, 
		 			_nf_record->man[ch].pre_record, pre_rec_sec,
		 			_nf_record->man[ch].network );
#endif

		if ( _nf_record->man[ch].pre_record == 0 || pre_rec_sec <= 0 )
			pre_rec_enable = 0;
		else
			pre_rec_enable = 0x2;

		req[new_req_idx].record_info[ch].channel_id = ch;
		req[new_req_idx].record_info[ch].flags = (_nf_record->man[ch].record_reason << 2)
									| pre_rec_enable
									| ((_nf_record->man[ch].network) ? 1:0) ;

		req[new_req_idx].record_info[ch].quality = _nf_record->man[ch].quality;
		req[new_req_idx].record_info[ch].fps = _nf_record->man[ch].fps;
		req[new_req_idx].record_info[ch].res = _nf_record->man[ch].resolution;
		req[new_req_idx].record_info[ch].codec = NF_CODEC_TYPE_H264;
		
		req[new_req_idx].record_info[ch].audio = _nf_record->man[ch].audio;
		req[new_req_idx].record_info[ch].audio_ch = _nf_record->man[ch].audio_ch;

		if( _nf_record->man[ch].network == 3 && 
			_nf_record->is_enable_stream_control ) { // 1: just live, 3: mobile live												
			req[new_req_idx].record_info[ch].quality_2nd = NF_QUALITY_LOW;
			req[new_req_idx].record_info[ch].fps_2nd = 
					( _nf_record->man[ch].fps > NF_FPS_CR16) ? NF_FPS_CR16 : _nf_record->man[ch].fps;
		}else{
			req[new_req_idx].record_info[ch].quality_2nd = _nf_record->man[ch].quality;
			req[new_req_idx].record_info[ch].fps_2nd = _nf_record->man[ch].fps;						
		}

		if (nf_issm_ctl_get_is_2nd_streaming(ch))
		{
			gint converted_fps;
			//gint converted_quality;

			converted_fps = _convert_sysdb_fps(_nf_record->netstream_fps[ch]);
			if (converted_fps < req[new_req_idx].record_info[ch].fps_2nd)
				req[new_req_idx].record_info[ch].fps_2nd = converted_fps;

			//converted_quality = _convert_sysdb_quality(_nf_record->netstream_quality[ch]);
			//if (converted_quality < req[new_req_idx].record_info[ch].quality_2nd)
			//	req[new_req_idx].record_info[ch].quality_2nd = converted_quality;
		}

#if 0	
		if(DISPLAY_IS_PAL)
			req[new_req_idx].record_info[ch].res |= 0x10;
#endif
		
		if( _nf_record->man[ch].vloss )
		{
			req[new_req_idx].record_info[ch].quality = 0;
			req[new_req_idx].record_info[ch].fps = 0;
						
		}else{													
			if( req[new_req_idx].record_info[ch].fps != req[old_req_idx].record_info[ch].fps
				|| req[new_req_idx].record_info[ch].quality != req[old_req_idx].record_info[ch].quality 
				|| req[new_req_idx].record_info[ch].fps_2nd != req[old_req_idx].record_info[ch].fps_2nd
				|| req[new_req_idx].record_info[ch].quality_2nd != req[old_req_idx].record_info[ch].quality_2nd 
				)
				change_mask |= (1<<ch);
		}			
	}

	if( gop_toggle == 0 && memcmp( &req[new_req_idx], &req[old_req_idx], sizeof(DRREQ_RECORD_START) ) == 0 )
	{
#ifdef DEBUG_RECORD_LOG
		if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN])
			g_message("%s same command skip [%d]",__FUNCTION__, req_cnt);
#endif

	}else{			
				
		if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN_TBL])		
			_dump_rec_table();
			
		{
			NF_RECEVT_SET_ENCODER_CMD_PARAM_T *param = NULL;
			param = (NF_RECEVT_SET_ENCODER_CMD_PARAM_T*) malloc(sizeof(NF_RECEVT_SET_ENCODER_CMD_PARAM_T));
			//NF_IPCAM_FPS_E fps1[NUM_ACTIVE_CH];
			//NF_IPCAM_FPS_E fps2[NUM_ACTIVE_CH];
			//NF_IPCAM_QUALITY_E qual1[NUM_ACTIVE_CH];
			//NF_IPCAM_QUALITY_E qual2[NUM_ACTIVE_CH];

			for (i = 0; i < NUM_ACTIVE_CH; i++)
			{
				param->fps1[i] = _ipcam_convert_fps(req[new_req_idx].record_info[i].fps);
				param->fps2[i] = _ipcam_convert_fps(req[new_req_idx].record_info[i].fps_2nd);
				param->qual1[i] = req[new_req_idx].record_info[i].quality;
				param->qual2[i] = req[new_req_idx].record_info[i].quality_2nd;
			}
			param->change_mask = change_mask;

			nf_ipcam_enqueue_rec_vcodec(param);
			//nf_ipcam_set_rec_vcodec_thread(change_mask, fps1, fps2, qual1, qual2);
			ret = 1;
		}
	}
	++req_cnt;
	
	return ret;
}

static guint _rec_config_manual()
{
	int ch, tmp, changeflag = 0;
		
	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {

		tmp = _get_manual_rec_audio(ch);				
		if ( _nf_record->man[ch].audio != tmp ) {
			_nf_record->man[ch].audio = tmp;
			SETBIT(changeflag, ch);
		}
#ifdef ENABLE_CAM_AUDIO_REMAP
		tmp = _get_cam_audio_in( ch );	
		if ( _nf_record->man[ch].audio_ch != tmp ) {
			_nf_record->man[ch].audio_ch = tmp;
			SETBIT(changeflag, ch);
		}
#else 
		_nf_record->man[ch].audio_ch = ch+1;		
#endif	
							
		tmp = _get_manual_rec_fps(ch);				
		if ( _nf_record->man[ch].fps != tmp ) {
			_nf_record->man[ch].fps = tmp;
			SETBIT(changeflag, ch);
		}

		tmp = _get_manual_rec_quality(ch);				
		if ( _nf_record->man[ch].quality != tmp ) {
			_nf_record->man[ch].quality = tmp;
			SETBIT(changeflag, ch);
		}

		tmp = _get_manual_rec_resolution(ch);
		if ( _nf_record->man[ch].resolution != tmp ) {		
			_nf_record->man[ch].resolution = tmp;
			SETBIT(changeflag, ch);
		}
	}

	return changeflag;
}

static guint _rec_config_alarm(gint date, gint hour)
{
	int ch, tmp, changeflag = 0;

	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
		
		if( _is_alarm_rec(ch) || _is_pre_rec(ch) == 2 /* USE ALARM TBL */) {
							
			tmp = _get_alarm_rec_audio(ch, date, hour);
			if ( _nf_record->man[ch].audio != tmp ) {
				_nf_record->man[ch].audio = tmp;			
				SETBIT(changeflag, ch);
			}
#ifdef ENABLE_CAM_AUDIO_REMAP
			tmp = _get_cam_audio_in( ch );	
			if ( _nf_record->man[ch].audio_ch != tmp ) {
				_nf_record->man[ch].audio_ch = tmp;
				SETBIT(changeflag, ch);
			}
#else 
		_nf_record->man[ch].audio_ch = ch+1;				
#endif				
			tmp = _get_alarm_rec_fps(ch, date, hour);
			if ( _nf_record->man[ch].fps != tmp ) {
				_nf_record->man[ch].fps = tmp;
				SETBIT(changeflag, ch);
			}
	
			tmp = _get_alarm_rec_quality(ch, date, hour);			
			//g_message("%s (ch %d,date %d,hour %d) %d %d",__FUNCTION__, ch, date, hour, _nf_record->man[ch].quality, tmp);
			if ( _nf_record->man[ch].quality != tmp ) {
				_nf_record->man[ch].quality = tmp;
				SETBIT(changeflag, ch);
			}		
	
			tmp = _get_alarm_rec_resolution(ch, date, hour);
			if ( _nf_record->man[ch].resolution != tmp ) {		
				_nf_record->man[ch].resolution = tmp;
				SETBIT(changeflag, ch);
			}
		}
	}

	return changeflag;
}

static guint _rec_config_timer(gint date, gint hour)
{
	int ch, tmp, changeflag = 0;
		
	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {

		if( !_is_timer_rec(ch)  ) continue;
			
		tmp = _get_timer_rec_audio(ch, date, hour);
		if ( _nf_record->man[ch].audio != tmp ) {
			_nf_record->man[ch].audio = tmp;					
			SETBIT(changeflag, ch);
		}

#ifdef ENABLE_CAM_AUDIO_REMAP
		tmp = _get_cam_audio_in( ch );	
		if ( _nf_record->man[ch].audio_ch != tmp ) {
			_nf_record->man[ch].audio_ch = tmp;
			SETBIT(changeflag, ch);
		}
#else 
		_nf_record->man[ch].audio_ch = ch+1;				
#endif			
		tmp = _get_timer_rec_fps(ch, date, hour);
		if ( _nf_record->man[ch].fps != tmp ) {
			_nf_record->man[ch].fps = tmp;
			SETBIT(changeflag, ch);
		}

		tmp = _get_timer_rec_quality(ch, date, hour);			
		if ( _nf_record->man[ch].quality != tmp ) {
			_nf_record->man[ch].quality = tmp;
			SETBIT(changeflag, ch);
		}

		tmp = _get_timer_rec_resolution(ch, date, hour);
		if ( _nf_record->man[ch].resolution != tmp ) {		
			_nf_record->man[ch].resolution = tmp;
			SETBIT(changeflag, ch);
		}
	}	

	return changeflag;
}

static guint _rec_config_motion(gint date, gint hour)
{
	int ch, tmp, changeflag = 0;

	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
		
		if( _is_motion_rec(ch) || _is_pre_rec(ch) == 1 /* USE MOTION TBL */ ) {
		
			tmp = _get_motion_rec_audio(ch, date, hour);
			if ( _nf_record->man[ch].audio != tmp ) {
				_nf_record->man[ch].audio = tmp;					
				SETBIT(changeflag, ch);
			}

#ifdef ENABLE_CAM_AUDIO_REMAP	
			tmp = _get_cam_audio_in( ch );	
			if ( _nf_record->man[ch].audio_ch != tmp ) {
				_nf_record->man[ch].audio_ch = tmp;
				SETBIT(changeflag, ch);
			}
#else 
		_nf_record->man[ch].audio_ch = ch+1;			
#endif				
			tmp = _get_motion_rec_fps(ch, date, hour);
			if ( _nf_record->man[ch].fps != tmp ) {
				_nf_record->man[ch].fps = tmp;
				SETBIT(changeflag, ch);
			}
	
			tmp = _get_motion_rec_quality(ch, date, hour);			
			if ( _nf_record->man[ch].quality != tmp ) {
				_nf_record->man[ch].quality = tmp;
				SETBIT(changeflag, ch);
			}
	
			tmp = _get_motion_rec_resolution(ch, date, hour);
			if ( _nf_record->man[ch].resolution != tmp ) {		
				_nf_record->man[ch].resolution = tmp;
				SETBIT(changeflag, ch);
			}
		}
	}	

	return changeflag;
}

static guint _rec_config_auto_high_qual()
{
	int ch, tmp, changeflag = 0;
		
	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {

		tmp = 1;	// always audio on!!
		if ( _nf_record->man[ch].audio != tmp ) {
			_nf_record->man[ch].audio = tmp;
			SETBIT(changeflag, ch);
		}

#ifdef ENABLE_CAM_AUDIO_REMAP
		tmp = _get_cam_audio_in( ch );
		if ( _nf_record->man[ch].audio_ch != tmp ) {
			_nf_record->man[ch].audio_ch = tmp;
			SETBIT(changeflag, ch);
		}
#else 
		_nf_record->man[ch].audio_ch = ch+1;		
#endif								
		tmp = NF_FPS_CR32;				
		if ( _nf_record->man[ch].fps != tmp ) {
			_nf_record->man[ch].fps = tmp;
			SETBIT(changeflag, ch);
		}

#if 1
		if( nf_sysdb_get_uint("rec.priority_mode") == 0 ) { // 0 : HD ��ȭ�ϼ� �켱, 1 : HD ȭ�� �켱
			tmp = NF_QUALITY_LOW;
		} else {
			tmp = NF_QUALITY_SUPER;
		}
#else
		if( nf_sysman_get_fwver_vendor() == 30 ) {	// S1�̸�			
			tmp = NF_QUALITY_LOW;
		} else {
			tmp = NF_QUALITY_SUPER;
		}
#endif			
		if ( _nf_record->man[ch].quality != tmp ) {
			_nf_record->man[ch].quality = tmp;
			SETBIT(changeflag, ch);
		}

		tmp = NF_RES_2592x1944;  //  FIX ME!!
		if ( _nf_record->man[ch].resolution != tmp ) {		
			_nf_record->man[ch].resolution = tmp;
			SETBIT(changeflag, ch);
		}
	}
	
	return changeflag;
}

static guint _rec_config_auto_event( gint auto_mode )
{
	int ch, tmp, changeflag = 0;
	char *sysdb_str = NULL;
	
	if( auto_mode == NF_RECORD_AUTO_CONFIG_MOT)	sysdb_str = SYSDB_AUTO_MOT;
	else if( auto_mode == NF_RECORD_AUTO_CONFIG_ALARM) sysdb_str = SYSDB_AUTO_ALARM;
	else sysdb_str = SYSDB_AUTO_MOT_ALARM;
		
	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
		
		tmp = _get_auto_rec_audio(sysdb_str, ch);
		if ( _nf_record->man[ch].audio != tmp ) {
			_nf_record->man[ch].audio = tmp;					
			SETBIT(changeflag, ch);
		}
	
#ifdef ENABLE_CAM_AUDIO_REMAP
		tmp = _get_cam_audio_in( ch );	
		if ( _nf_record->man[ch].audio_ch != tmp ) {
			_nf_record->man[ch].audio_ch = tmp;
			SETBIT(changeflag, ch);
		}
#else 
		_nf_record->man[ch].audio_ch = ch+1;		
#endif			
		tmp = _get_auto_rec_fps(sysdb_str, ch);
		if ( _nf_record->man[ch].fps != tmp ) {
			_nf_record->man[ch].fps = tmp;
			SETBIT(changeflag, ch);
		}
	
		tmp = _get_auto_rec_quality(sysdb_str, ch);
		if ( _nf_record->man[ch].quality != tmp ) {
			_nf_record->man[ch].quality = tmp;
			SETBIT(changeflag, ch);
		}
	
		tmp = _get_auto_rec_resolution(sysdb_str, ch);
		if ( _nf_record->man[ch].resolution != tmp ) {		
			_nf_record->man[ch].resolution = tmp;
			SETBIT(changeflag, ch);
		}			
	}
					
	return changeflag;
}

static guint _rec_config_auto_intensive( gint auto_mode )
{
	int ch, tmp, changeflag = 0;
	char *sysdb_normal = NULL;
	char *sysdb_event = NULL;
	char *sysdb_audio = NULL;
	char *sysdb_str;
	
	if(auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT){ 
		sysdb_normal = SYSDB_AUTO_ITS_MOT_NORMAL;
		sysdb_event = SYSDB_AUTO_ITS_MOT_EVENT;
		sysdb_audio = SYSDB_AUTO_ITS_MOT_AUDIO;
	}else if(auto_mode == NF_RECORD_AUTO_CONFIG_ITS_ALARM){
		sysdb_normal = SYSDB_AUTO_ITS_ALARM_NORMAL;
		sysdb_event = SYSDB_AUTO_ITS_ALARM_EVENT;
		sysdb_audio = SYSDB_AUTO_ITS_ALARM_AUDIO;
	}else{ 
		sysdb_normal = SYSDB_AUTO_ITS_MOT_ALARM_NORMAL;
		sysdb_event = SYSDB_AUTO_ITS_MOT_ALARM_EVENT;
		sysdb_audio = SYSDB_AUTO_ITS_MOT_ALARM_AUDIO;
	}
		
	for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
	
		if( (auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT
			   || auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM ) 
			   && _is_motion_rec(ch) )
			sysdb_str = sysdb_event;
		else if( (auto_mode == NF_RECORD_AUTO_CONFIG_ITS_ALARM
			   || auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM ) 
			   && _is_alarm_rec(ch) )
			   sysdb_str = sysdb_event;
		else
			sysdb_str = sysdb_normal;
			
		tmp = _get_auto_rec_audio(sysdb_audio, ch);
		if ( _nf_record->man[ch].audio != tmp ) {
			_nf_record->man[ch].audio = tmp;					
			SETBIT(changeflag, ch);
		}
	
#ifdef ENABLE_CAM_AUDIO_REMAP
		tmp = _get_cam_audio_in( ch );	
		if ( _nf_record->man[ch].audio_ch != tmp ) {
			_nf_record->man[ch].audio_ch = tmp;
			SETBIT(changeflag, ch);
		}
#else 
		_nf_record->man[ch].audio_ch = ch+1;				
#endif			
		tmp = _get_auto_rec_fps(sysdb_str, ch);
		if ( _nf_record->man[ch].fps != tmp ) {
			_nf_record->man[ch].fps = tmp;
			SETBIT(changeflag, ch);
		}
	
		tmp = _get_auto_rec_quality(sysdb_str, ch);
		if ( _nf_record->man[ch].quality != tmp ) {
			_nf_record->man[ch].quality = tmp;
			SETBIT(changeflag, ch);
		}
	
		tmp = _get_auto_rec_resolution(sysdb_str, ch);
		if ( _nf_record->man[ch].resolution != tmp ) {		
			_nf_record->man[ch].resolution = tmp;
			SETBIT(changeflag, ch);
		}			
	}
		
	return changeflag;
}



static gint _is_auto_intensive( gint auto_mode )
{
	return ( auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM );
}

static gint _is_auto_event( gint auto_mode )
{
	return ( auto_mode == NF_RECORD_AUTO_CONFIG_MOT ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_MOT_ALARM );
}

static gint _is_auto_motion( gint auto_mode )
{
	return ( auto_mode == NF_RECORD_AUTO_CONFIG_MOT ||
				auto_mode == NF_RECORD_AUTO_CONFIG_MOT_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM );
}

static gint _is_auto_alarm( gint auto_mode )
{
	return ( auto_mode == NF_RECORD_AUTO_CONFIG_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_MOT_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_ALARM ||
				auto_mode == NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM );
}

// simple / timer record
// if mode == 0, after startting old_rec_tbl will be clear.
static gint _timer_simple_record(gint sysdb_changed)
{
	gint ch, date, hour, rec_mode, sched, auto_mode;
	guint changeflag = 0;
	gint pre_rec_time = _nf_record->pre_rec_time;

	rec_mode = _nf_record->record_mode;
	auto_mode =	_nf_record->record_auto_mode;
	sched = _nf_record->sched_mode;
	date = _nf_record->current_week;
	hour = _nf_record->current_hour;
	static int wait_count = 0;
	static int last_req_network = 0;

	if(_nf_record->req_network){
		last_req_network = _nf_record->req_network;
	}

	if(wait_count > 0){
		wait_count--;
		_nf_record->req_network = 0;
	}else{
		_nf_record->req_network = last_req_network;
	}

#ifdef DEBUG_RECORD_LOG
	if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN])
		g_message("%s rec_mode[%d][%d] sched[%d] date[%d] hour[%2d]", __FUNCTION__, 
			rec_mode, auto_mode, sched, date, hour);
#endif

	if( _nf_record->record_mode == NF_RECORD_REC_MODE_MANUAL ) {
		
		//g_message("pre");	
		/* PRE RECORDING */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			
			int is_alarm_rec_onoff = _get_alarm_rec_onoff(ch, date, hour);
			int is_motion_rec_onoff = _get_motion_rec_onoff(ch, date, hour);
					
			if ( (pre_rec_time) && (!_is_vloss(ch)) && (!_is_no_write()) &&
				 _get_timer_rec_onoff(ch, date, hour) == 0 &&
					(is_motion_rec_onoff || is_alarm_rec_onoff) ) {				
				if (!_is_pre_rec(ch) ) 
				{					
					_set_rec_info(ch, NF_RECORD_REASON_PRE);
					SETBIT(changeflag, ch);				
				}
				
				if( is_alarm_rec_onoff )
					_set_pre_rec_info(ch, 2);		// use ALARM table
				else
					_set_pre_rec_info(ch, 1);		// use MOTION table				
					
			} else {
				if (_is_pre_rec(ch)) {
					_set_pre_rec_info(ch, 0);
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);				
					SETBIT(changeflag, ch);
				}
			}
		} 
	
		//g_message("manual");
		/* manual */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if ( (_is_manual_on()) && 
				(!_is_vloss(ch)) && 
				(!_is_no_write()) ) {
				if (!_is_manual_rec(ch)) {		
					_set_rec_info(ch, NF_RECORD_REASON_MANUAL);
					_send_rec_start_log(ch, LP2_RECORD_STARTED_PANIC);
	
					SETBIT(changeflag, ch);
				}				
			} else {
				if (_is_manual_rec(ch)) {
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
					_send_rec_stop_log(ch, LP2_RECORD_STOPPED_PANIC);
	
					SETBIT(changeflag, ch);
				}								
			}
		}

		//g_message("alarm");	
		/* alarm */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if ( (_get_alarm_rec_onoff(ch, date, hour)) &&
				(_is_alarm_on(ch)) &&			
				(!_is_vloss(ch)) && 
				(!_is_no_write()) ) {
				if (_is_priority_higher(NF_RECORD_REASON_ALARM, ch)) {
					_set_rec_info(ch, NF_RECORD_REASON_ALARM);
					_send_rec_start_log(ch, LP2_RECORD_STARTED_SENSOR);
	
					SETBIT(changeflag, ch);
				}
	
				if (_is_alarm_rec(ch))
					_load_post_timer(ch, _nf_record->post_rec_time );
					
			} else {
				if (_is_alarm_rec(ch) && 
					( _dec_post_timer(ch) || _is_no_write() || _is_vloss(ch)) ) {
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
					_send_rec_stop_log(ch, LP2_RECORD_STOPPED_SENSOR);
	
					SETBIT(changeflag, ch);
				}
			}
		}
			
		//g_message("motion");
		/* motion */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if ( ( _get_motion_rec_onoff(ch, date, hour) ) && 
				(_is_motion_on(ch)) &&
				(!_is_vloss(ch)) && 
				(!_is_no_write()) ) {
				if (_is_priority_higher(NF_RECORD_REASON_MOTION, ch)) {
					_set_rec_info(ch, NF_RECORD_REASON_MOTION);
					_send_rec_start_log(ch, LP2_RECORD_STARTED_MOTION);
	
					SETBIT(changeflag, ch);
				}
				
				if (_is_motion_rec(ch))
					_load_post_timer(ch, _nf_record->post_rec_time );
			} else {
				if (_is_motion_rec(ch) 
					&& ( _dec_post_timer(ch) || _is_no_write() || _is_vloss(ch)) ) {				
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
					_send_rec_stop_log(ch, LP2_RECORD_STOPPED_MOTION);
	
					SETBIT(changeflag, ch);
				}
			}
		}
		
		//g_message("continuous");		
		/* continuous */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if ((_get_timer_rec_onoff(ch, date, hour)) &&
				(!_is_vloss(ch)) && 
				(!_is_no_write()) ) {
				if (_is_priority_higher(NF_RECORD_REASON_TIMER, ch)) 
				{
					// continuous�� pre_rec�� ����.
					_set_pre_rec_info(ch, 0);	
					
					_set_rec_info(ch, NF_RECORD_REASON_TIMER);
					_send_rec_start_log(ch, LP2_RECORD_STARTED_TIMER);	
					SETBIT(changeflag, ch);
				}
			} else {
				if (_is_timer_rec(ch)) {
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
					_send_rec_stop_log(ch, LP2_RECORD_STOPPED_TIMER);
	
					SETBIT(changeflag, ch);
				}
			}
		}
	
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if( _is_pre_rec(ch) 
				&& _get_rec_info(ch) == NF_RECORD_REASON_NOTHING )
			{
				_set_rec_info(ch, NF_RECORD_REASON_PRE);
				SETBIT(changeflag, ch);
			}		
		}
	
		if( _nf_record->req_network ) {	
			_nf_record->req_network = 0;
			changeflag = 0xffffffff;
		}
		
		if( _is_manual_on() ){
			changeflag |= _rec_config_manual();
		}else{					
			changeflag |= _rec_config_alarm( date, hour);
			changeflag |= _rec_config_motion( date, hour); // with prerecord
			changeflag |= _rec_config_timer( date, hour);
		}
	}else { // _nf_record->record_mode == NF_RECORD_REC_MODE_MANUAL  AUTO_MODE


		//g_message("pre");	
		/* PRE RECORDING */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {						
			if ( (pre_rec_time) && (!_is_vloss(ch)) && (!_is_no_write()) ) {
				if (!_is_pre_rec(ch)) 
				{
					_set_pre_rec_info(ch, 1);
					_set_rec_info(ch, NF_RECORD_REASON_PRE);
					SETBIT(changeflag, ch);
				}
			} else {
				if (_is_pre_rec(ch)) {
					_set_pre_rec_info(ch, 0);
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);				
					SETBIT(changeflag, ch);
				}
			}
		}
				
		//g_message("manual");
		/* manual */
		for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
			if ( (_is_manual_on()) && 
				(!_is_vloss(ch)) && 
				(!_is_no_write()) ) {
				if (!_is_manual_rec(ch)) {		
					_set_rec_info(ch, NF_RECORD_REASON_MANUAL);
					_send_rec_start_log(ch, LP2_RECORD_STARTED_PANIC);
	
					SETBIT(changeflag, ch);
				}				
			} else {
				if (_is_manual_rec(ch)) {
					_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
					_send_rec_stop_log(ch, LP2_RECORD_STOPPED_PANIC);
	
					SETBIT(changeflag, ch);
				}								
			}
		}
			
		if( auto_mode == NF_RECORD_AUTO_CONFIG_HIGH_QUAL ) {
			
			//g_message("continuous");		
			/* continuous */
			for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
				if ( (!_is_vloss(ch)) && (!_is_no_write()) ) {
					if (_is_priority_higher2(NF_RECORD_REASON_TIMER, ch)) 
					{
						// continuous�� pre_rec�� ����.
						_set_pre_rec_info(ch, 0);	
						
						_set_rec_info(ch, NF_RECORD_REASON_TIMER);
						_send_rec_start_log(ch, LP2_RECORD_STARTED_TIMER);	
						SETBIT(changeflag, ch);
					}
				} else {
					if (_is_timer_rec(ch)) {
						_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
						_send_rec_stop_log(ch, LP2_RECORD_STOPPED_TIMER);
		
						SETBIT(changeflag, ch);
					}
				}
			}

		}else{ //( nf_record->record_auto_mode == NF_RECORD_AUTO_CONFIG_HIGH_QUAL )
	
			//g_message("alarm");	
			/* alarm */
			for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
				if ( _is_auto_alarm(auto_mode) &&
					(_is_alarm_on(ch)) && 
					(!_is_vloss(ch)) && 
					(!_is_no_write()) ) {
					if (_is_priority_higher(NF_RECORD_REASON_ALARM, ch)) {
						_set_rec_info(ch, NF_RECORD_REASON_ALARM);
						_send_rec_start_log(ch, LP2_RECORD_STARTED_SENSOR);
		
						SETBIT(changeflag, ch);
					}
		
					if (_is_alarm_rec(ch))
						_load_post_timer(ch, _nf_record->post_rec_time );
						
				} else {
					if (_is_alarm_rec(ch) && 
						( _dec_post_timer(ch) || _is_no_write() || _is_vloss(ch)) ) {
						_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
						_send_rec_stop_log(ch, LP2_RECORD_STOPPED_SENSOR);
		
						SETBIT(changeflag, ch);
					}
				}
			}

			//g_message("motion");
			/* motion */
			for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
				if ( _is_auto_motion(auto_mode) && 
					(_is_motion_on(ch)) &&
					(!_is_vloss(ch)) && 
					(!_is_no_write()) ) {
					if (_is_priority_higher(NF_RECORD_REASON_MOTION, ch)) {
						_set_rec_info(ch, NF_RECORD_REASON_MOTION);
						_send_rec_start_log(ch, LP2_RECORD_STARTED_MOTION);
		
						SETBIT(changeflag, ch);
					}
					
					if (_is_motion_rec(ch))
						_load_post_timer(ch, _nf_record->post_rec_time );
				} else {
					if (_is_motion_rec(ch) 
						&& ( _dec_post_timer(ch) || _is_no_write() || _is_vloss(ch)) ) {				
						_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
						_send_rec_stop_log(ch, LP2_RECORD_STOPPED_MOTION);
		
						SETBIT(changeflag, ch);
					}
				}
			}
				
			//g_message("continuous-intensive");
			/* continuous */
			for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
				if ( (!_is_vloss(ch)) && (!_is_no_write()) && _is_auto_intensive( auto_mode ) ) {
					if (_is_priority_higher(NF_RECORD_REASON_TIMER, ch)) 
					{
						// continuous�� pre_rec�� ����.
						_set_pre_rec_info(ch, 0);	
						
						_set_rec_info(ch, NF_RECORD_REASON_TIMER);
						_send_rec_start_log(ch, LP2_RECORD_STARTED_TIMER);	
						SETBIT(changeflag, ch);
					}
				} else {
					if (_is_timer_rec(ch)) {
						_set_rec_info(ch, NF_RECORD_REASON_NOTHING);
						_send_rec_stop_log(ch, LP2_RECORD_STOPPED_TIMER);
		
						SETBIT(changeflag, ch);
					}
				}
			}
			
			for (ch = 0; ch < NUM_ACTIVE_CH; ++ch) {
				if( _is_pre_rec(ch) 
					&& _get_rec_info(ch) == NF_RECORD_REASON_NOTHING )
				{
					_set_rec_info(ch, NF_RECORD_REASON_PRE);
					SETBIT(changeflag, ch);
				}		
			}		
		}
									
		if( _nf_record->req_network ) {	
			_nf_record->req_network = 0;
			changeflag = 0xffffffff;
		}
				
		if( _is_manual_on() ){
			changeflag |= _rec_config_manual();
		}else{
			if( _is_auto_intensive(auto_mode) ) {
				changeflag |= _rec_config_auto_intensive(auto_mode);
			}else if(  _is_auto_event(auto_mode) ) {
				changeflag |= _rec_config_auto_event(auto_mode);
			}else{  // NF_RECORD_AUTO_CONFIG_HIGH_QUAL
				changeflag |= _rec_config_auto_high_qual();
			}			
		}		
	} 
				
	if (changeflag & 0xffffffff) {

#ifdef DEBUG_RECORD_LOG		
		if(_DEBUG_RECORD_log[DEBUG_RECORD_IDX_REC_MAN])
			g_message("%s changeflag[0x%08x]", __FUNCTION__, changeflag);
#endif
		//_send_rec_cmd(changeflag & 0xffffffff);
		last_req_network = 0;
		if(_send_rec_cmd(changeflag & 0xffff)){
			wait_count = NETWORK_REQ_DEBOUNCE_DELAY;
		}
		_notify_rec_mode(changeflag & 0xffff);
										
	}

	return changeflag;
}

static void _dump_rec_table()
{
	gint ch;

	g_message("record_off[%d]manual_rec[%d] pre[%d]post[%d]", 
			_nf_record->record_off, _nf_record->manual_rec,
			_nf_record->pre_rec_time, _nf_record->post_rec_time );
	
	g_message("[vloss alarm md covert] [net pre] [reason]  [audio fps q res]");
	
	for(ch=0;ch<NUM_TOTAL_CHANNEL;ch++) {
		g_message("ch[%2d] [%d][%d][%d][%d]  [%d][%d]  [%d]  [%d][%d][%d][%d] [%d] sid[%d][%d]",
					ch,
					_nf_record->man[ch].vloss,
					_nf_record->man[ch].alarm,
					_nf_record->man[ch].motion,
					_nf_record->man[ch].covert,

					_nf_record->man[ch].network,
					_nf_record->man[ch].pre_record,
					_nf_record->man[ch].record_reason,

					_nf_record->man[ch].audio, /* gb */
					_nf_record->man[ch].fps,
					_nf_record->man[ch].quality,
					_nf_record->man[ch].resolution,
					
					_nf_record->man[ch].post_rec_timer,
					
					_nf_record->sst[0][ch].stream_id,
					_nf_record->sst[1][ch].stream_id);
										
	}	

#if 0
	g_message("last_codec_header ========");
	for(ch=0;ch<NUM_ACTIVE_CH;++ch) {
		dump_icodec_header("frame", &_nf_record->man[ch].last_header);
	}
#endif	
}

/*
	@brief				���ڵ� �뷮���
	@param[in]  param	���(current/continuous/motion/alarm ��)
	@param[out]	result	�����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_record_calculate( NF_RECORD_CALC_PARAM_T param, NF_RECORD_CALC_RESULT_T *result )
{
	g_return_val_if_fail(result != NULL, FALSE);
	g_return_val_if_fail (_nf_record != NULL, FALSE);

	if(param.motion_occur_pcnt + param.alarm_occur_pcnt > 100)
	{
		g_warning("[%s] motion + alarm percent must within 100 %%\n", __FUNCTION__);
		return FALSE;
	}

	g_message("[%s] start! mode : %d motion : %d alarm : %d\n", __FUNCTION__, param.mode, param.motion_occur_pcnt, param.alarm_occur_pcnt);

	int i, j, swit, rtn;
	guint used, total;
	gdouble gb_per_day_range = 0.0;
	gdouble gpd_cont[NUM_ACTIVE_CH];
	gdouble gpd_motion[NUM_ACTIVE_CH];
	gdouble gpd_alarm[NUM_ACTIVE_CH];
	memset(gpd_cont, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);
	memset(gpd_motion, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);
	memset(gpd_alarm, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);

	_record_setting_t rec_sysdb;
	NFIPCamBpsTable bps_table[NUM_ACTIVE_CH];
	memset(&rec_sysdb, 0x00, sizeof(_record_setting_t));
	memset(result, 0x00, sizeof(NF_RECORD_CALC_RESULT_T));

	_build_record_sysdb_table(&rec_sysdb, param.mode);
	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		rtn = nf_ipcam_get_bps_table(i, &bps_table[i]);
	}
	if(param.mode == NF_RECORD_CALC_CURRENT)
	{
		if(rec_sysdb.rec_mode == NF_RECORD_REC_MODE_AUTO)
		{
			switch(rec_sysdb.auto_config)
			{
				case NF_RECORD_AUTO_CONFIG_HIGH_QUAL:		swit = 1 << NF_RECORD_CALC_AUTO_CONTINUOUS;			break;
				case NF_RECORD_AUTO_CONFIG_MOT:				swit = 1 << NF_RECORD_CALC_AUTO_MOTION;				break;
				case NF_RECORD_AUTO_CONFIG_ALARM:			swit = 1 << NF_RECORD_CALC_AUTO_ALARM;				break;
				case NF_RECORD_AUTO_CONFIG_MOT_ALARM:		swit = 1 << NF_RECORD_CALC_AUTO_MOTION_ALARM;		break;
				case NF_RECORD_AUTO_CONFIG_ITS_MOT:			swit = 1 << NF_RECORD_CALC_AUTO_ITS_MOTION;			break;
				case NF_RECORD_AUTO_CONFIG_ITS_ALARM:		swit = 1 << NF_RECORD_CALC_AUTO_ITS_ALARM;			break;
				case NF_RECORD_AUTO_CONFIG_ITS_MOT_ALARM:	swit = 1 << NF_RECORD_CALC_AUTO_ITS_MOTION_ALARM;	break;
			}
		}
		else
		{
			swit = (1 << NF_RECORD_CALC_MANUAL_MOTION) | (1 << NF_RECORD_CALC_MANUAL_ALARM) | (1 << NF_RECORD_CALC_MANUAL_CONTINUOUS);
		}
	}
	else
	{
		swit = 1 << param.mode;
	}

	for(j = 0; j < NF_RECORD_CALC_MAX; j++)
	{
		if((1 << j) & swit)
		{
			switch(j)
			{
				case NF_RECORD_CALC_CURRENT:
					break;

				case NF_RECORD_CALC_AUTO_CONTINUOUS:
					_calc_bps_mode_auto_high_qual(rec_sysdb.priority_mode, bps_table, gpd_cont);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_cont[i];
					}
					break;
				case NF_RECORD_CALC_AUTO_MOTION:
					_calc_bps_mode_auto(rec_sysdb.auto_motion, bps_table, gpd_motion);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_motion[i] * (gdouble)param.motion_occur_pcnt / 100.0;
					}
					break;
				case NF_RECORD_CALC_AUTO_ALARM:
					_calc_bps_mode_auto(rec_sysdb.auto_alarm, bps_table, gpd_alarm);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_alarm[i] * (gdouble)param.alarm_occur_pcnt / 100.0;
					}
					break;
				case NF_RECORD_CALC_AUTO_MOTION_ALARM:
					_calc_bps_mode_auto(rec_sysdb.auto_motion_alarm, bps_table, gpd_motion);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_motion[i] * (gdouble)(param.motion_occur_pcnt + param.alarm_occur_pcnt)/ 100.0;
					}
					break;
				case NF_RECORD_CALC_AUTO_ITS_MOTION:
					_calc_bps_mode_auto(rec_sysdb.auto_its_motion_normal, bps_table, gpd_cont);
					_calc_bps_mode_auto(rec_sysdb.auto_its_motion_motion, bps_table, gpd_motion);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_cont[i] * (gdouble)(100 - param.motion_occur_pcnt) / 100.0 + gpd_motion[i] * (gdouble)(param.motion_occur_pcnt)/ 100.0;
					}
					break;
				case NF_RECORD_CALC_AUTO_ITS_ALARM:
					_calc_bps_mode_auto(rec_sysdb.auto_its_alarm_normal, bps_table, gpd_cont);
					_calc_bps_mode_auto(rec_sysdb.auto_its_alarm_alarm, bps_table, gpd_alarm);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_cont[i] * (gdouble)(100 - param.alarm_occur_pcnt) / 100.0 + gpd_alarm[i] * (gdouble)(param.alarm_occur_pcnt)/ 100.0;
					}
					break;
				case NF_RECORD_CALC_AUTO_ITS_MOTION_ALARM:
					_calc_bps_mode_auto(rec_sysdb.auto_its_ma_normal, bps_table, gpd_cont);
					_calc_bps_mode_auto(rec_sysdb.auto_its_ma_event, bps_table, gpd_motion);
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						result->ch_gb_per_day[i] = gpd_cont[i] * (gdouble)(100 - param.motion_occur_pcnt - param.alarm_occur_pcnt) / 100.0
												 + gpd_motion[i] * (gdouble)(param.motion_occur_pcnt + param.alarm_occur_pcnt)/ 100.0;
					}
					break;
				case NF_RECORD_CALC_MANUAL_CONTINUOUS:
					if(rec_sysdb.sched_mode == NF_RECORD_SCHEDULE_DAILY)
					{
						_calc_bps_mode_manual_daily(rec_sysdb.manual_continuous, bps_table, gpd_cont);
					}
					else
					{
						_calc_bps_mode_manual_weekly(rec_sysdb.manual_continuous, bps_table, gpd_cont);
					}
					break;
				case NF_RECORD_CALC_MANUAL_MOTION:
					if(rec_sysdb.sched_mode == NF_RECORD_SCHEDULE_DAILY)
					{
						_calc_bps_mode_manual_daily(rec_sysdb.manual_motion, bps_table, gpd_motion);
					}
					else
					{
						_calc_bps_mode_manual_weekly(rec_sysdb.manual_motion, bps_table, gpd_motion);
					}
					break;
				case NF_RECORD_CALC_MANUAL_ALARM:
					if(rec_sysdb.sched_mode == NF_RECORD_SCHEDULE_DAILY)
					{
						_calc_bps_mode_manual_daily(rec_sysdb.manual_alarm, bps_table, gpd_alarm);
					}
					else
					{
						_calc_bps_mode_manual_weekly(rec_sysdb.manual_alarm, bps_table, gpd_alarm);
					}
					break;
			}
		}
	}
	if(((1 << NF_RECORD_CALC_MANUAL_MOTION) | (1 << NF_RECORD_CALC_MANUAL_ALARM) | (1 << NF_RECORD_CALC_MANUAL_CONTINUOUS)) & swit)
	{
		for(i = 0; i < NUM_ACTIVE_CH; i++)
		{
			result->ch_gb_per_day[i] = gpd_cont[i] * (gdouble)(100 - param.motion_occur_pcnt - param.alarm_occur_pcnt) / 100.0
									 + gpd_motion[i] * (gdouble)(param.motion_occur_pcnt)/ 100.0
									 + gpd_alarm[i] * (gdouble)(param.alarm_occur_pcnt)/ 100.0;
		}
	}
	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		//result->ch_gb_per_day[i] *= F_OVERRATE_RATE;

		// VBR
		if(bps_table[i].bitctrl[0] == NF_IPCAM_BITRATE_CONTROL_VBR)
		{
			gb_per_day_range += result->ch_gb_per_day[i] * 0.3;
		}
		result->gb_per_day += result->ch_gb_per_day[i];
	}

	rtn = nf_disk_get_usage( 0, 0, &used, &total, NULL);

	result->hdd_full_gb = (gdouble)total / 1024.0;
	result->hdd_remain_gb = (gdouble)(total - used) / 1024.0;

	if(result->gb_per_day == 0)
	{
		result->day_full = 0.0;
		result->day_remain = 0.0;
	}
	else
	{
		result->day_full = result->hdd_full_gb / result->gb_per_day;
		result->day_remain = result->hdd_remain_gb / result->gb_per_day;
		if (gb_per_day_range > 0)
		{
			result->day_full_range = result->hdd_full_gb / (result->gb_per_day - gb_per_day_range);
			result->day_full_range *= 0.9; // 10 percent margin
		}

		result->day_full *= 0.9; // 10 percent margin

		g_message("[%s] Hdd : %4.1f GB \n", __FUNCTION__, result->hdd_full_gb);
		g_message("[%s] 1 day : %4.1f GB \n", __FUNCTION__, result->gb_per_day);
		g_message("[%s] 1 day considering vbr : %4.1f GB \n", __FUNCTION__, result->gb_per_day - gb_per_day_range);

	}

	g_message("[%s] end! result : %4.1f day\n", __FUNCTION__, result->day_full);

	return TRUE;
}

static void _build_record_sysdb_table(_record_setting_t *table, int mode)
{
	g_return_if_fail(table != NULL);
	g_return_if_fail(_nf_record != NULL);

	int i, j, k, tmp;
	table->rec_mode = _get_rec_mode();
	table->auto_config = _get_rec_auto_config();
	table->priority_mode = _get_rec_priority_mode();
	table->sched_mode = _get_rec_sched();

	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		//if(mode == NF_RECORD_CALC_AUTO_MOTION || (mode == NF_RECORD_CALC_CURRENT && table->auto_config == NF_RECORD_AUTO_CONFIG_MOT))
		{
			table->auto_motion[i].size = _get_auto_rec_resolution(SYSDB_AUTO_MOT, i);
			table->auto_motion[i].fps = _get_auto_rec_fps(SYSDB_AUTO_MOT, i);
			table->auto_motion[i].quality = _get_auto_rec_quality(SYSDB_AUTO_MOT, i);
			table->auto_motion[i].audio = _get_auto_rec_audio(SYSDB_AUTO_MOT, i);
			table->auto_motion[i].enable = 1;
		}

		table->auto_alarm[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ALARM, i);
		table->auto_alarm[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ALARM, i);
		table->auto_alarm[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ALARM, i);
		table->auto_alarm[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ALARM, i);
		table->auto_alarm[i].enable = 1;

		table->auto_motion_alarm[i].size = _get_auto_rec_resolution(SYSDB_AUTO_MOT_ALARM, i);
		table->auto_motion_alarm[i].fps = _get_auto_rec_fps(SYSDB_AUTO_MOT_ALARM, i);
		table->auto_motion_alarm[i].quality = _get_auto_rec_quality(SYSDB_AUTO_MOT_ALARM, i);
		table->auto_motion_alarm[i].audio = _get_auto_rec_audio(SYSDB_AUTO_MOT_ALARM, i);
		table->auto_motion_alarm[i].enable = 1;

		table->auto_its_motion_normal[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_MOT_NORMAL, i);
		table->auto_its_motion_normal[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_MOT_NORMAL, i);
		table->auto_its_motion_normal[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_MOT_NORMAL, i);
		table->auto_its_motion_normal[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_MOT_AUDIO, i);
		table->auto_its_motion_normal[i].enable = 1;

		table->auto_its_motion_motion[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_MOT_EVENT, i);
		table->auto_its_motion_motion[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_MOT_EVENT, i);
		table->auto_its_motion_motion[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_MOT_EVENT, i);
		table->auto_its_motion_motion[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_MOT_AUDIO, i);
		table->auto_its_motion_motion[i].enable = 1;

		table->auto_its_alarm_normal[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_ALARM_NORMAL, i);
		table->auto_its_alarm_normal[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_ALARM_NORMAL, i);
		table->auto_its_alarm_normal[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_ALARM_NORMAL, i);
		table->auto_its_alarm_normal[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_ALARM_AUDIO, i);
		table->auto_its_alarm_normal[i].enable = 1;

		table->auto_its_alarm_alarm[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_ALARM_EVENT, i);
		table->auto_its_alarm_alarm[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_ALARM_EVENT, i);
		table->auto_its_alarm_alarm[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_ALARM_EVENT, i);
		table->auto_its_alarm_alarm[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_ALARM_AUDIO, i);
		table->auto_its_alarm_alarm[i].enable = 1;

		table->auto_its_ma_normal[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_MOT_ALARM_NORMAL, i);
		table->auto_its_ma_normal[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_MOT_ALARM_NORMAL, i);
		table->auto_its_ma_normal[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_MOT_ALARM_NORMAL, i);
		table->auto_its_ma_normal[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_MOT_ALARM_AUDIO, i);
		table->auto_its_ma_normal[i].enable = 1;

		table->auto_its_ma_event[i].size = _get_auto_rec_resolution(SYSDB_AUTO_ITS_MOT_ALARM_EVENT, i);
		table->auto_its_ma_event[i].fps = _get_auto_rec_fps(SYSDB_AUTO_ITS_MOT_ALARM_EVENT, i);
		table->auto_its_ma_event[i].quality = _get_auto_rec_quality(SYSDB_AUTO_ITS_MOT_ALARM_EVENT, i);
		table->auto_its_ma_event[i].audio = _get_auto_rec_audio(SYSDB_AUTO_ITS_MOT_ALARM_AUDIO, i);
		table->auto_its_ma_event[i].enable = 1;

		for(j = 0; j < 8; j++)	// sun..sat/daily
		{
			for(k = 0; k < 24; k++) // hour
			{
				table->manual_continuous[i][j][k].size = _get_timer_rec_resolution(i, j, k);
				table->manual_continuous[i][j][k].fps = _get_timer_rec_fps(i, j, k);
				table->manual_continuous[i][j][k].quality = _get_timer_rec_quality(i, j, k);
				table->manual_continuous[i][j][k].audio = _get_timer_rec_audio(i, j, k);
				table->manual_continuous[i][j][k].enable = _get_timer_rec_onoff(i, j, k);

				table->manual_motion[i][j][k].size = _get_motion_rec_resolution(i, j, k);
				table->manual_motion[i][j][k].fps = _get_motion_rec_fps(i, j, k);
				table->manual_motion[i][j][k].quality = _get_motion_rec_quality(i, j, k);
				table->manual_motion[i][j][k].audio = _get_motion_rec_audio(i, j, k);
				table->manual_motion[i][j][k].enable = _get_motion_rec_onoff(i, j, k);

				table->manual_alarm[i][j][k].size = _get_alarm_rec_resolution(i, j, k);
				table->manual_alarm[i][j][k].fps = _get_alarm_rec_fps(i, j, k);
				table->manual_alarm[i][j][k].quality = _get_alarm_rec_quality(i, j, k);
				table->manual_alarm[i][j][k].audio = _get_alarm_rec_audio(i, j, k);
				table->manual_alarm[i][j][k].enable = _get_alarm_rec_onoff(i, j, k);
			}
		}
	}
}

static void _calc_bps_mode_auto_high_qual(gint priority, NFIPCamBpsTable *bps_table, gdouble *result)
{
	int i, j, rtn, bps, resol, fps, quality;
	guint capable, current;
	uint64_t cap, cur;
	memset(result, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);

	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		// get runtime res, fps
		if(nf_ipcam_get_resol(i, 0, &cap, &cur, 0) != 1) // IPCAM_SETUP_RTN_DONE
		{
			continue;
		}
		for(j = 0; j < NF_IPCAM_RES_MAX; j++)
		{
			if(shift_res_table[j] == cur) break;
		}
		resol = j;

		if(nf_ipcam_get_fps(i, 0, &capable, &current, 0) != 1)
		{
			continue;
		}
		if((capable & NF_IPCAM_FPS_300) || (capable & NF_IPCAM_FPS_250))
		{
			current = NF_IPCAM_FPS_300;
		}
		else if((capable & NF_IPCAM_FPS_150) || (capable & NF_IPCAM_FPS_120))
		{
			current = NF_IPCAM_FPS_150;
		}
		else if((capable & NF_IPCAM_FPS_70) || (capable & NF_IPCAM_FPS_60))
		{
			current = NF_IPCAM_FPS_70;
		}
		else if(capable & NF_IPCAM_FPS_30)
		{
			current = NF_IPCAM_FPS_30;
		}
		else if(capable & NF_IPCAM_FPS_20)
		{
			current = NF_IPCAM_FPS_20;
		}
		else if(capable & NF_IPCAM_FPS_10)
		{
			current = NF_IPCAM_FPS_10;
		}
		else
		{
			continue;
		}
		for(j = 0; j < NF_IPCAM_FPS_MAX; j++)
		{
			if((1 << j) == current) break;
		}
		fps = j;

		if(priority == 0)
		{
			quality = NF_QUALITY_LOW;
		}
		else
		{
			quality = NF_QUALITY_SUPER;
		}

		bps = bps_table[i].sum_video_bps[resol][fps][quality];
		bps += _get_audio_bps(i, bps_table);

		result[i] = ((gdouble)bps / 8.0) * 60.0 * 60.0 * 24.0 / (1024.0 * 1024.0);
	}

}
// record table[num_ch] return gb per day
static void _calc_bps_mode_auto(_record_set_unit *record_table, NFIPCamBpsTable *bps_table, gdouble *result)
{
	int i, bps;
	memset(result, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);

	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		bps = _get_bps(i, record_table[i], bps_table);
		if(bps > 0 && record_table[i].audio)
		{
			bps += _get_audio_bps(i, bps_table);
		}

		result[i] = ((gdouble)bps / 8.0) * 60.0 * 60.0 * 24.0 / (1024.0 * 1024.0);
	}
}

// record table[num_ch][num_day=8(use 7)][num_hour=24] return gb per day
static void _calc_bps_mode_manual_daily(_record_set_unit record_table[NUM_ACTIVE_CH][8][24], NFIPCamBpsTable *bps_table, gdouble *result)
{
	int i, j, bps = 0;
	memset(result, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);

	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		bps = 0;
		for(j = 0; j < 24; j++)
		{
			if(record_table[i][7][j].enable)
			{
				bps += _get_bps(i, record_table[i][7][j], bps_table);
				if(bps > 0 && record_table[i][7][j].audio)
				{
					bps += _get_audio_bps(i, bps_table);
				}
			}
		}
		result[i] = ((gdouble)bps / 8.0) * 60.0 * 60.0 / (1024.0 * 1024.0);
	}
}
// record table[num_ch][num_day=8(use 0..6)][num_hour=24] return gb per day
static void _calc_bps_mode_manual_weekly(_record_set_unit record_table[NUM_ACTIVE_CH][8][24], NFIPCamBpsTable *bps_table, gdouble *result)
{
	int i, j, k, bps = 0;
	memset(result, 0x00, sizeof(gdouble) * NUM_ACTIVE_CH);

	for(i = 0; i < NUM_ACTIVE_CH; i++)
	{
		bps = 0;
		for(j = 0; j < 7; j++)
		{
			for(k = 0; k < 24; k++)
			{
				if(record_table[i][j][k].enable)
				{
					bps += _get_bps(i, record_table[i][j][k], bps_table);
					if(bps > 0 && record_table[i][j][k].audio)
					{
						bps += _get_audio_bps(i, bps_table);
					}
				}
			}
		}
		result[i] = (((gdouble)bps / 8.0) * 60.0 * 60.0 / 7.0) / (1024.0 * 1024.0);
	}
}
static guint _get_bps(int ch, _record_set_unit unit, NFIPCamBpsTable *bps_table)
{
	int size = _convert_resol_to_ipcam(unit.size);
	int fps = _convert_fps_to_ipcam(unit.fps);

	return bps_table[ch].sum_video_bps[size][fps][unit.quality];
}
static guint _get_audio_bps(int ch, NFIPCamBpsTable *bps_table)
{
	return bps_table[ch].audio_bps;
}

static gint _convert_resol_to_ipcam(gint val)
{
	gint i;
	uint64_t rtn;
	switch (val)
	{
		case NF_RES_NTSC_CIF:	rtn = NF_IPCAM_RES_352x240;		break;
		case NF_RES_NTSC_2CIF:	rtn = NF_IPCAM_RES_704x480;		break;	// XXX
		case NF_RES_NTSC_4CIF:	rtn = NF_IPCAM_RES_704x480;		break;
		case NF_RES_NTSC_4CIFP:	rtn = NF_IPCAM_RES_704x480;		break;
		case NF_RES_PAL_CIF:	rtn = NF_IPCAM_RES_352x288;		break;
		case NF_RES_PAL_2CIF:	rtn = NF_IPCAM_RES_704x576;		break;
		case NF_RES_PAL_4CIF:	rtn = NF_IPCAM_RES_704x576;		break;
		case NF_RES_PAL_4CIFP:	rtn = NF_IPCAM_RES_704x576;		break;
		case NF_RES_640x480:	rtn = NF_IPCAM_RES_640x480;		break;
		case NF_RES_720x480:	rtn = NF_IPCAM_RES_720x480;		break;
		case NF_RES_720x576:	rtn = NF_IPCAM_RES_720x576;		break;
		case NF_RES_800x600:	rtn = NF_IPCAM_RES_800x600;		break;
		case NF_RES_1024x768:	rtn = NF_IPCAM_RES_1024x768;	break;
		case NF_RES_1280x1024:	rtn = NF_IPCAM_RES_1280x1024;	break;
		case NF_RES_1600x1200:	rtn = NF_IPCAM_RES_1600x1200;	break;
		case NF_RES_1280x720:	rtn = NF_IPCAM_RES_1280x720;	break;
		case NF_RES_1920x1080:	rtn = NF_IPCAM_RES_1920x1080;	break;
		case NF_RES_640x352:	rtn = NF_IPCAM_RES_640x352;		break;
		case NF_RES_640x360:	rtn = NF_IPCAM_RES_640x360;		break;
		case NF_RES_640x360I:	rtn = NF_IPCAM_RES_640x360I;	break;
		case NF_RES_1280x720I:	rtn = NF_IPCAM_RES_1280x720I;	break;
		case NF_RES_1920x1080I:	rtn = NF_IPCAM_RES_1920x1080I;	break;
		case NF_RES_640x400:	rtn = NF_IPCAM_RES_640x400;		break;
		case NF_RES_800x450:	rtn = NF_IPCAM_RES_800x450;		break;
		case NF_RES_1440x900:	rtn = NF_IPCAM_RES_1440x900;	break;
		case NF_RES_320x180:	rtn = NF_IPCAM_RES_320x180;		break;
		case NF_RES_2304x1296:	rtn = NF_IPCAM_RES_2304x1296;	break;
		case NF_RES_2048x1536:	rtn = NF_IPCAM_RES_2048x1536;	break;
		case NF_RES_2560x1440:	rtn = NF_IPCAM_RES_2560x1440;	break;
		case NF_RES_2688x1520:	rtn = NF_IPCAM_RES_2688x1520;	break;
		case NF_RES_2560x1600:	rtn = NF_IPCAM_RES_2560x1600;	break;
		case NF_RES_2560x1920:	rtn = NF_IPCAM_RES_2560x1920;	break;
		case NF_RES_2592x1920:	rtn = NF_IPCAM_RES_2592x1920;	break;
		case NF_RES_2592x1944:	rtn = NF_IPCAM_RES_2592x1944;	break;
		case NF_RES_2992x1680:	rtn = NF_IPCAM_RES_2992x1680;	break;
		case NF_RES_2880x1800:	rtn = NF_IPCAM_RES_2880x1800;	break;
		case NF_RES_3200x1800:	rtn = NF_IPCAM_RES_3200x1800;	break;
		case NF_RES_2880x2160:	rtn = NF_IPCAM_RES_2880x2160;	break;
		case NF_RES_3072x2048:	rtn = NF_IPCAM_RES_3072x2048;	break;
		case NF_RES_3200x2400:	rtn = NF_IPCAM_RES_3200x2400;	break;
		case NF_RES_3840x2160:	rtn = NF_IPCAM_RES_3840x2160;	break;
		case NF_RES_2592x1520:	rtn = NF_IPCAM_RES_2592x1520;	break;		
		case NF_RES_NTSC_NONE:	rtn = 0;						break;
		default:				rtn = 0;						break;
	}
	for(i = 0; i < NF_IPCAM_RES_MAX; i++)
	{
		if(shift_res_table[i] == rtn) 
			return i;
	}
	return -1;
}
static gint _convert_fps_to_ipcam(gint val)
{
	gint rtn, i;

	switch (val)
	{
		case NF_FPS_CR32:	rtn = NF_IPCAM_FPS_300;	break;
		case NF_FPS_CR16: 	rtn = NF_IPCAM_FPS_150;	break;
		case NF_FPS_CR08:	rtn = NF_IPCAM_FPS_70;	break;
		case NF_FPS_CR04:	rtn = NF_IPCAM_FPS_30;	break;
		case NF_FPS_CR02:	rtn = NF_IPCAM_FPS_20;	break;
		case NF_FPS_CR01:	rtn = NF_IPCAM_FPS_10;	break;
		case NF_FPS_CR00:	rtn = 0;				break;
		default:			rtn = 0;				break;
	}
	for(i = 0; i < NF_IPCAM_FPS_MAX; i++)
	{
		if((1 << i) == rtn) return i;
	}
	return -1;
}
/*
	sysdb ���� ��ȸ �Լ��� �ʿ�;

	sysdb���� ��� �׸��� ���ڵ� ó���� �ҷ��� �ô��� ������尡 ũ��.
	(���ڵ� ������ ���� �þ)	�׷��� ������ ���� ó����
	
���� ��� ������ ����, motion ���ڵ� ������ ������ �� 3���� �迭�� �ʿ��մϴ�.
daily(weekly), time, channel  = (1+7)*24*16 = 3072 bytes�� �ʿ��մϴ�.
���� (1+7)�� �ش��ϴ� ���ϰ��� �κ��� rec.motion.M?.xx �ε����� �����մϴ�.
(idx range 0 to 6, Sunday being 0,  �̰� daily�� ��쿣 7)

"rec.motion.M0.size" value�� ����� 384 bytes�� ��Ʈ���� [16ä��] * [24�ð�] ������ �����˴ϴ�. (strmap�̶� �ϰ���)
<0��ä�� * 24�ð�><1��ä�� * 24�ð�>..<15��ä�� * 24�ð�>
(���� : C�迭 ���� char rec_size[16][24];)

Size, FPS, Quality�� ��쿣 ipcamera ������ ǥ���ؾ� �ϴ� 
������ ���� ���� ������, ���ĺ�(�빮��)�� ����ؼ� ǥ���մϴ�.

Size : 
	NF_RES_NTSC_NONE	= 0x00, // A
	NF_RES_NTSC_CIF		= 0x01, // B
	NF_RES_NTSC_2CIF	= 0x02, // C
	NF_RES_NTSC_4CIF	= 0x04, // D

FPS:
	NF_FPS_CR32			= 32,	//A
	NF_FPS_CR16			= 16,	//B
	NF_FPS_CR08			= 8,	//C
	NF_FPS_CR04			= 4,	//D
	NF_FPS_CR02			= 2,	//E
	NF_FPS_CR01			= 1,	//F
	NF_FPS_CR00			= 0		//G

Quality:
	NF_QUALITY_HIGHEST	= 3,	//A
	NF_QUALITY_HIGH		= 2,	//B
	NF_QUALITY_STANDARD	= 1,	//C
	NF_QUALITY_LOW		= 0		//D

Audio:
	none				0
	enable				1

Mode(motion,rec.motion.M?.mode):
	none				0
	continuous			1
	motion				2
	continuous/motion	3

Mode(alarm,rec.alarm.A?.mode):
	none				0
	alarm				1


http://nf.intellix.co.kr/phpBB/viewtopic.php?t=306&highlight=record

���ڵ� ��å;

1. AVR �ø���� ���� Record �޴� �������� ����. 
(1) Timer/Motion 
(2) Alarm 
(3) Manual 

2. Default ������ �� ��� Table�� 30fps ��������, �� �ð�  ������ �Ǿ� �ִ� 
���¿��� �Ѵ�. 

3. �� ������ FPS Table�� Schedule Table�� ���´�. 
(1)�� ��쿡�� Timer/Motion/Timer+Motion/None 4�� �߿� �ϳ��� ������ �� �ִ�. 

4. Intensive Check Box�� �־ �̰��� On�� ��Ű�� Motion�̳� Alarm�� �� ä���� 
D1 30fps�� ����ش�. DSP �ϳ��� 2xD1x30fps ���� ��ȭ �� �� �ֱ� ������, �� 
�ѵ����� ������ ü���� ��ȭ rate�� ���� �߸���, �׷��� ���ڸ���, Resolution�� 
����߸����� �Ѵ�. 

Intensive Check Box�� On �� �Ǹ�, �� ��������, �� ������ Motion �Ǵ� Alarm, 
�Ǵ� Motion+Alarm �� �� ���� �� ������ ������ �� �ֵ��� Check Box�� �߰��� 
������ �� �ֵ��� Ȱ��ȭ ��Ų��.

<nf_sysdb>
<sys>
<item key="rec.alarm.A0.audio" type="STRING" val="0000000000000000....000" />
<item key="rec.alarm.A0.fps" type="STRING" val="000000000000000000....0" />
<item key="rec.alarm.A0.mode" type="STRING" val="00000000000000000....00" />
<item key="rec.alarm.A0.quality" type="STRING" val="00000000000000....00000" />
<item key="rec.alarm.A0.size" type="STRING" val="00000000000000000....00" />
<item key="rec.alarm.ACNT" type="UINT" val="8" />
<item key="rec.motion.M0.audio" type="STRING" val="000000000000000....00" />
<item key="rec.motion.M0.fps" type="STRING" val="00000000000000000...." />
<item key="rec.motion.M0.mode" type="STRING" val="0000000000000000....0" />
<item key="rec.motion.M0.quality" type="STRING" val="0000000000000....0000" />
<item key="rec.motion.M0.size" type="STRING" val="0000000000000000....0" />
<item key="rec.motion.MCNT" type="UINT" val="8" />
<item key="rec.mode" type="UINT" val="0" />
<item key="rec.panic.audio" type="STRING" val="0000000000000000" />
<item key="rec.panic.fps" type="STRING" val="0000000000000000" />
<item key="rec.panic.quality" type="STRING" val="0000000000000000" />
<item key="rec.panic.size" type="STRING" val="0000000000000000" />
<item key="rec.post_rec_time" type="UINT" val="5" />
<item key="rec.pre_rec_time" type="UINT" val="5" />
<item key="rec.sched_mode" type="UINT" val="0" />
</sys>
</nf_sysdb> 

*/
