#include <glib.h>

#include "nf_common.h"
#include "nf_watchdog.h"
#include "nf_notify.h"
#include "nf_debug.h"
#include "nf_util_device.h"
#include "nf_dspcomm_app.h"

#define ENABLE_WATCHDOG_HW_ON
#define ENABLE_WATCHDOG_RESET_DUMP				//	2013-07-04 ���� 4:29:30 choissi
#define ENABLE_WATCHDOG_RECORD_AUDIT
#define ENABLE_WATCHDOG_RECORD_AUDIT_FOR_NVR	// 2013-07-04 ���� 4:08:51 choissi

#define ENABLE_WATCHDOG_SST_WRITE_FAIL_AUDIT
#define MAX_WATCHDOG_SST_WRITE_FAIL_CNT_CHK		120
#define MAX_WATCHDOG_SST_WRITE_FAIL_CNT_TOT		1800

//#define DEBUG_WATCHDOG_FORCE_REBOOT     /*XXX, +HW on/off test only*/
#define DEBUG_WATCHDOG_SW_FPGA_RESET    /*XXX, +HW on/off test only*/
#define ENABLE_WATCHDOG_SST_RECOVERY_TIMEOUT
#define ENABLE_WATCHDOG_REBOOT_REASON // 2013-06-27 ���� 4:11:35 choissi

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "watchdog"

#define DEBUG_WATCHDOG_LOG
#define DEBUG_WATCHDOG_JBSHELL

#ifdef DEBUG_WATCHDOG_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_WATCHDOG_IDX_E
{
	DEBUG_WATCHDOG_IDX_INIT 		= 0,
	DEBUG_WATCHDOG_IDX_THREAD 		= 1,
	DEBUG_WATCHDOG_IDX_CTRL 		= 2,
	DEBUG_WATCHDOG_IDX_KICK 		= 3,

	DEBUG_WATCHDOG_IDX_NR			= 4

}DEBUG_WATCHDOG_IDX_E;

static const char *_DEBUG_WATCHDOG_str[32] =
{
	"WATCHDOG_IDX_INIT",
	"WATCHDOG_IDX_THREAD",
	"WATCHDOG_IDX_CTRL",
	"WATCHDOG_IDX_KICK",

	"WATCHDOG_IDX_NR"
};

static guint _DEBUG_WATCHDOG_log[32] =
{
	1,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};



static const gchar *_member_str_arr[] = {
	"RECORD",
	"RECORD_AUDIO",
	"SST",
	"DSP0",

	"DSP1",
	"GUI",
	"MAIN_TIMER",
	"NETSVR",

	"EMAIL_SEND",
	"ACTION",
	"EVENT",
	"PTZ",

	"KEYCTRL",
	"NOTIFY",
	"ENCODER0",
	"ENCODER1",  /*FIXME*/

	"ENCODERZ",
	"M3_VPSS",
	"M3_CODEC",

	"NR"
};

#define MEMBER_STR_ARR_SIZE  (sizeof( _member_str_arr ) / sizeof(char *))

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

static void nf_watchdog_class_init (NfWatchdogClass * klass);
static void nf_watchdog_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_watchdog_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_watchdog_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_watchdog_dispose (GObject * object);
static void nf_watchdog_finalize (GObject * object);

static void _watchdog_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static gboolean _watchdog_timer_cb_func( gpointer data);

static GObjectClass *parent_class = NULL;
static NfWatchdog	*_nf_watchdog = NULL;

static void
watchdog_thread_func (NfWatchdog * test) ;

extern guint nf_timer_add( guint interval, GSourceFunc cb_func, gpointer data);

GType
nf_watchdog_get_type (void)
{
	static GType nf_watchdog_type = 0;

	if (G_UNLIKELY (nf_watchdog_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfWatchdogClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_watchdog_class_init,
			NULL,
			NULL,
			sizeof (NfWatchdog),
			0,
			(GInstanceInitFunc) nf_watchdog_instance_init,
			NULL
		};

		nf_watchdog_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfWatchdog", &object_info, 0);
	}

	return nf_watchdog_type;
}

static void
nf_watchdog_class_init (NfWatchdogClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_watchdog_set_property;
	gobject_class->get_property = nf_watchdog_get_property;

	gobject_class->dispose = nf_watchdog_dispose;
	gobject_class->finalize = nf_watchdog_finalize;

}

static void
nf_watchdog_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfWatchdog *self = NF_WATCHDOG (instance);

	self->init_done = 0;

	// queue ����
	self->queue = g_async_queue_new();

	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)watchdog_thread_func,
									self, FALSE, NULL);

	self->is_running = 0;

	memset(self->enable_cp, 0x00,sizeof(self->enable_cp));
	memset(self->max_cp, 0x00,sizeof(self->max_cp));
	memset(self->cur_cp, 0x00,sizeof(self->cur_cp));
	memset(self->last_kick_tv, 0x00, sizeof(GTimeVal)* NF_WATCHDOG_MEMBER_NR);

}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_watchdog_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_watchdog_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_watchdog_set_property (GObject * object, guint prop_id,
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
nf_watchdog_get_property (GObject * object, guint prop_id,
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


/******************************************************************************/

static const char *_member_idx_to_str( NF_WATCHDOG_MEMBER_E module )
{
	g_return_val_if_fail ( module < MEMBER_STR_ARR_SIZE, "NULL" );

	return _member_str_arr[module];
}


#ifdef ENABLE_WATCHDOG_RECORD_AUDIT
extern gboolean nf_record_watchdog_is_ok();
#endif

#include "nf_api_eventlog.h"
static void
watchdog_thread_func (NfWatchdog * self)
{
	int  i,j, nand_log_flush_cnt = 0;
	gchar buf[128] = { 0, };

#ifdef ENABLE_WATCHDOG_RECORD_AUDIT
	guint	record_fail_count = 0;
#endif

	g_message("%s start", __FUNCTION__);
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;

        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_max(policy);
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }

#ifdef ENABLE_WATCHDOG_HW_ON
	nf_dev_watchdog_chg_margin(	120 );
#endif
	self->init_done = 1;

	while(1)
	{
		GTimeVal   cur_tv;

		g_get_current_time( &cur_tv);

		nand_log_flush_cnt ++;

		if ( nand_log_flush_cnt == 60 )
		{
			nf_event_nand_log_flush();
			nand_log_flush_cnt = 0;
		}

#ifdef DEBUG_WATCHDOG_LOG
		if(_DEBUG_WATCHDOG_log[DEBUG_WATCHDOG_IDX_THREAD])
			g_message("%s is_running[%d] alive sec[%ld]",
							__FUNCTION__, self->is_running, cur_tv.tv_sec);
#endif

#ifdef ENABLE_WATCHDOG_RECORD_AUDIT
		if( !nf_record_watchdog_is_ok() )
			++record_fail_count;
		else
			record_fail_count = 0;


	#ifdef ENABLE_WATCHDOG_RECORD_AUDIT_FOR_NVR

		if( record_fail_count == 60  )
		{
			g_warning("%s RECORD_AUDIT WARNING!!",__FUNCTION__);
			g_warning("%s RECORD_AUDIT WARNING!!",__FUNCTION__);

			snprintf(buf, 128,  "watchdog_thread_func RECORD_AUDIT WARNING!!");
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);

		}else if( record_fail_count > 14400 ) {//ksi_test 3600 -> 14400
			g_warning("%s RECORD_AUDIT RESET!!",__FUNCTION__);
			g_warning("%s RECORD_AUDIT RESET!!",__FUNCTION__);

			snprintf(buf, 128,  "watchdog_thread_func RECORD_AUDIT RESET!!");
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);

			nf_dev_buzzer_on();
			sleep(5);
			nf_dev_buzzer_off();
			goto watchdog_reset;
		}

		// for 1min buzzer alarm
		if( record_fail_count > 60 && (record_fail_count % 60) == 0 )
		{
			g_warning("%s RECORD_AUDIT WARNING!!",__FUNCTION__);
			g_warning("%s RECORD_AUDIT WARNING!!",__FUNCTION__);
/*
			nf_dev_buzzer_on();
			sleep(1);
			nf_dev_buzzer_off();
*/
		}

	#else
		if( record_fail_count > 70 )	// 30 -> 70 FOR EMI TEST
		{
			g_warning("%s RECORD_AUDIT RESET!!",__FUNCTION__);
			g_warning("%s RECORD_AUDIT RESET!!",__FUNCTION__);

			snprintf(buf, 128,  "watchdog_thread_func RECORD_AUDIT RESET!!");
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);

			nf_dev_buzzer_on();
			sleep(5);
			nf_dev_buzzer_off();
			goto watchdog_reset;
		}
	#endif	// ENABLE_WATCHDOG_RECORD_AUDIT_FOR_NVR

#endif	// ENABLE_WATCHDOG_RECORD_AUDIT


#ifdef ENABLE_WATCHDOG_SST_RECOVERY_TIMEOUT
	{
		static volatile int is_sst_failed = 0;

		if( self->enable_cp[NF_WATCHDOG_MEMBER_SST] )
		{
			if( self->cur_cp[NF_WATCHDOG_MEMBER_SST] == (self->max_cp[NF_WATCHDOG_MEMBER_SST] - 60) )
			{
				is_sst_failed = 1;
				nf_notify_fire_params("disk_smart", 1,0xff,0,0);
			}
		}else{
			if ( is_sst_failed) {
				is_sst_failed = 0;
				nf_notify_fire_params("disk_smart", 0,0,0,0);
			}
		}
	}
#endif

/*
	guint 			enable_cp[NF_WATCHDOG_MEMBER_NR];
	guint 			max_cp[NF_WATCHDOG_MEMBER_NR];
	guint 			cur_cp[NF_WATCHDOG_MEMBER_NR];
	GTimeVal		last_kick_tv[NF_WATCHDOG_MEMBER_NR];
*/
		if( self->is_running )
		{
			NF_OBJECT_LOCK(_nf_watchdog);
			for	(i=0;i<NF_WATCHDOG_MEMBER_NR;i++)
			{
				if( self->enable_cp[i] )
				{
					if( self->max_cp[i] < self->cur_cp[i] )
					{
						g_warning("%s RESET!!",__FUNCTION__);
						g_warning("%s RESET!!",__FUNCTION__);

						g_warning("%s RESET [%2d][%s] max[%d] cur[%d] last[%ld]!!",
									__FUNCTION__, i, _member_idx_to_str(i),
									self->max_cp[i],
									self->cur_cp[i],
									self->last_kick_tv[i].tv_sec );

						g_warning("%s RESET!!",__FUNCTION__);
						g_warning("%s RESET!!",__FUNCTION__);

						NF_OBJECT_UNLOCK(_nf_watchdog);

#ifdef ENABLE_WATCHDOG_HW_ON
						nf_dev_keypad_led_on(i);

						nf_dev_buzzer_on();

						snprintf(buf, 128,  "Watchdog RESET [%2d][%s] max[%d] cur[%d] last[%ld]!!",
								i, _member_idx_to_str(i),
								self->max_cp[i],
								self->cur_cp[i],
								self->last_kick_tv[i].tv_sec);
						nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);

						#ifdef ENABLE_WATCHDOG_RESET_DUMP
							for	(j=0;j<NF_WATCHDOG_MEMBER_NR;j++){
								if( self->enable_cp[j] ) {
									snprintf(buf, 128,  "Watchdog DUMP [%2d][%s] max[%d] cur[%d] last[%ld]!!",
										j, _member_idx_to_str(j),
										self->max_cp[j],
										self->cur_cp[j],
										self->last_kick_tv[j].tv_sec);
									nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);
								}
							}
						#endif

						sleep(2);
						nf_dev_buzzer_off();
						for(j=0;i<j; ++j)
						{
							g_usleep(1000*330);
							nf_dev_buzzer_on();
							g_usleep(1000*330);
							nf_dev_buzzer_off();
						}

						goto watchdog_reset;

#else
						nf_dev_buzzer_on();
						sleep(1);
						nf_dev_buzzer_off();
#endif
						NF_OBJECT_LOCK(_nf_watchdog);
					}
					++self->cur_cp[i];
				}
			}
			NF_OBJECT_UNLOCK(_nf_watchdog);
		}

#ifdef ENABLE_WATCHDOG_HW_ON
		nf_dev_watchdog_alive();
#endif
		sleep(1);

		// 2012-11-04 ���� 12:58:17 choissi  for s1 hw test
		if( nf_sysman_get_fwver_vendor() == 30)
			nf_record_send_stream_stat_to_s1();

	}

watchdog_reset:
	g_warning("%s end", __FUNCTION__);

#ifdef DEBUG_WATCHDOG_SW_FPGA_RESET
	sleep(5);
    g_warning("%s DEBUG_WATCHDOG_SW_FPGA_RESET", __FUNCTION__);
	sleep(5);
    g_warning("%s DEBUG_WATCHDOG_SW_FPGA_RESET", __FUNCTION__);
	sleep(5);
    g_warning("%s DEBUG_WATCHDOG_SW_FPGA_RESET", __FUNCTION__);
    nf_dev_board_reset();
#endif

}


static gboolean
_watchdog_timer(gpointer data)
{
	static guint cnt = 0;

	nf_notify_fire_params("dvr_wdtimer", ++cnt,0,0,0);
	return TRUE;
}

static void
_dvr_wdtimer_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	nf_watchdog_kick(NF_WATCHDOG_MEMBER_MAIN_TIMER);
}

#ifdef ENABLE_WATCHDOG_SST_WRITE_FAIL_AUDIT

static void
_dvr_disk_write_fail_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{

	static guint tot_fail_cnt = 0;
	static guint chk_fail_cnt = 0;
	static guint last_tv_sec = 0;
	char buf[128];

	g_message("%s param[%d][%d]", __FUNCTION__, pinfo->d.params[0], pinfo->d.params[1]);

	if( last_tv_sec > pinfo->timestamp.tv_sec )
		last_tv_sec = 0;

	++tot_fail_cnt;
	g_assert ( tot_fail_cnt < MAX_WATCHDOG_SST_WRITE_FAIL_CNT_TOT );

	++chk_fail_cnt;
	g_assert ( chk_fail_cnt < MAX_WATCHDOG_SST_WRITE_FAIL_CNT_CHK );

	if( pinfo->timestamp.tv_sec - last_tv_sec > 86400 ) {
		if(chk_fail_cnt) {
			// for nand log
			snprintf(buf, 128,  "%s chk_fail_cnt tot[%d] chk[%d]", __FUNCTION__, tot_fail_cnt, chk_fail_cnt);
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);

			g_warning("%s chk_fail_cnt tot[%d] chk[%d]", __FUNCTION__, tot_fail_cnt, chk_fail_cnt);
			chk_fail_cnt = 0;
		}
	}

	last_tv_sec = pinfo->timestamp.tv_sec;
	g_warning("%s last_tv[%d] tot[%d]chk[%d]",__FUNCTION__, last_tv_sec, tot_fail_cnt, chk_fail_cnt);

}

#endif


static void
watchdog_m3_mon_thread_func (void *self)
{

	g_message("%s start", __FUNCTION__);
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;

        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_max(policy);
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_M3_VPSS, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_M3_CODEC, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
#endif

	sleep(10);

	while(1)
	{
		//check M3_VPSS
        if(nf_live_check_video_processor() == FALSE)
        {
            g_warning("M3 HANG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            g_warning("M3 HANG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            g_warning("M3 HANG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            break;
        }
		nf_watchdog_kick( NF_WATCHDOG_MEMBER_M3_VPSS );
		//check M3_CODEC
		nf_watchdog_kick( NF_WATCHDOG_MEMBER_M3_CODEC );

        sleep(5);
	}
}

/*
	@brief				watchdog �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_watchdog_init(int wait)
{
	gboolean ret = TRUE;
	guint		tmp = 0;
	GThread		*thread;

	g_return_val_if_fail (_nf_watchdog == NULL, FALSE);

	_nf_watchdog = g_object_new ( NF_TYPE_WATCHDOG , NULL);

	if( wait )
	{
		while( _nf_watchdog->init_done != 1)
			g_usleep(10*1000);
	}

	nf_dev_watchdog_cntl_thread(FALSE);		// Hisilicon Watchdog Stop

	nf_debug_category_add( "watchdog", _DEBUG_WATCHDOG_str, _DEBUG_WATCHDOG_log, DEBUG_WATCHDOG_IDX_NR);

	//nf_dev_watchdog_chg_margin(	3 );

#if defined(_IPX_1648VE3) || defined(_IPX_0824VE3) || defined(_IPX_0412VE3) || \
	defined(_IPX_0824P3) || defined(_IPX_1648P3) || defined(_IPX_0824P3ECO) || defined(_IPX_1648P3ECO) || \
	defined(_IPX_0412P4) || defined(_IPX_0824P4) || defined(_IPX_1648P4) \
       || defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
       || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	//hisilicon nvr not use m3 watchdog
#else
	g_thread_create(	(GThreadFunc)watchdog_m3_mon_thread_func, NULL , FALSE, NULL);
#endif

	return 1;
}


gboolean nf_watchdog_start_run()
{
	gulong	cb_handle;
	g_message( "%s called", __FUNCTION__ );

	g_return_val_if_fail (_nf_watchdog != NULL, FALSE);

	if(  _nf_watchdog->is_running == 1)
	{
		g_warning("%s already running!!", __FUNCTION__);
		return 1;
	}

	_nf_watchdog->is_running = 1;

	nf_timer_add( 1000, _watchdog_timer, NULL);

	cb_handle= nf_notify_connect_cb( "dvr_wdtimer", _dvr_wdtimer_notify_cb_func , (gpointer)0);
	g_message("%s dvr_wdtimer connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#ifdef ENABLE_WATCHDOG_SST_WRITE_FAIL_AUDIT
	cb_handle= nf_notify_connect_cb( "disk_write_fail", _dvr_disk_write_fail_notify_cb_func , (gpointer)0);
	g_message("%s disk_write_fail connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_MAIN_TIMER, 120, NF_WATCHDOG_ENABLE);
#endif

#ifdef ENABLE_WATCHDOG_HW_ON
	#if !defined(_NVR_MODEL) && !defined(_HDI_0412)
		// pakkhman for ipx watchdog
		nf_dev_watchdog_enable();
	#else
		g_message("%s IPX WatchDog Enable!!!!!", __FUNCTION__);
	#endif
#endif

	return 1;
}

gboolean nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_E module, guint interval_sec, gboolean enable)
{

	g_return_val_if_fail (_nf_watchdog != NULL, FALSE);
	g_return_val_if_fail ( module < NF_WATCHDOG_MEMBER_NR , FALSE);
	g_return_val_if_fail ( interval_sec < NF_WATCHDOG_KILL_SECOND , FALSE);

#ifdef DEBUG_WATCHDOG_LOG
	if(_DEBUG_WATCHDOG_log[DEBUG_WATCHDOG_IDX_CTRL])
		g_message("%s module[%2d][%-16.16s] sec[%d] enable[%d]", __FUNCTION__,
				module, _member_idx_to_str(module),
				interval_sec, enable);
#endif

	NF_OBJECT_LOCK(_nf_watchdog);

	_nf_watchdog->enable_cp[module] = (guint)enable;
	_nf_watchdog->max_cp[module] = interval_sec;
	_nf_watchdog->cur_cp[module] = 0;
	g_get_current_time( &_nf_watchdog->last_kick_tv[module] );

	NF_OBJECT_UNLOCK(_nf_watchdog);

	return TRUE;
}

gboolean nf_watchdog_kick( NF_WATCHDOG_MEMBER_E module )
{
	int i=0;
	g_return_val_if_fail (_nf_watchdog != NULL, FALSE);
	g_return_val_if_fail ( module <= NF_WATCHDOG_MEMBER_NR , FALSE);

//	g_message ("@@@ %s(%d) : %2d", __FUNCTION__,__LINE__,module);

#ifdef DEBUG_WATCHDOG_FORCE_REBOOT
    if(module == NF_WATCHDOG_MEMBER_RECORD)
    {
    	g_return_val_if_fail ( module != NF_WATCHDOG_MEMBER_RECORD , FALSE);
    }
#endif

	if( module == NF_WATCHDOG_MEMBER_NR ){

		for(i=0;i<NF_WATCHDOG_MEMBER_NR;++i)
		{
			NF_OBJECT_LOCK(_nf_watchdog);
			if( _nf_watchdog->enable_cp[i] )
			{
				_nf_watchdog->cur_cp[i] = 0;
				g_get_current_time( &_nf_watchdog->last_kick_tv[i] );
			}
			NF_OBJECT_UNLOCK(_nf_watchdog);
		}

		g_message("%s module[%2d][%-16.16s] max[%d] cur[%d] tv_sec[%ld]",
				__FUNCTION__,
				module, _member_idx_to_str(module),
				_nf_watchdog->max_cp[module],
				_nf_watchdog->cur_cp[module],
				_nf_watchdog->last_kick_tv[module].tv_sec);

	}else{
		NF_OBJECT_LOCK(_nf_watchdog);
		_nf_watchdog->cur_cp[module] = 0;
		g_get_current_time( &_nf_watchdog->last_kick_tv[module] );
		NF_OBJECT_UNLOCK(_nf_watchdog);
	}



#ifdef DEBUG_WATCHDOG_LOG
	if(_DEBUG_WATCHDOG_log[DEBUG_WATCHDOG_IDX_KICK])
		g_message("%s module[%2d][%-16.16s] max[%d] cur[%d] tv_sec[%ld]",
				__FUNCTION__,
				module, _member_idx_to_str(module),
				_nf_watchdog->max_cp[module],
				_nf_watchdog->cur_cp[module],
				_nf_watchdog->last_kick_tv[module].tv_sec);
#endif

	return TRUE;
}

gboolean nf_watchdog_dump()
{
	int i=0;

	g_return_val_if_fail (_nf_watchdog != NULL, FALSE);

	g_message("%s is_running [%d]", __FUNCTION__, _nf_watchdog->is_running);
	for	(i=0;i<NF_WATCHDOG_MEMBER_NR;i++)
	{
		g_message("%s [%2d][%-16.16s] en[%d] max[%3d]cur[%3d] tv_sec[%ld]",
						__FUNCTION__, i, _member_idx_to_str(i),
						_nf_watchdog->enable_cp[i],
						_nf_watchdog->max_cp[i],
						_nf_watchdog->cur_cp[i],
						_nf_watchdog->last_kick_tv[i].tv_sec);
	}

	return TRUE;
}


#ifdef DEBUG_WATCHDOG_JBSHELL

static char watchdog_dump_help[] = "watchdog_dump";
static int watchdog_dump(int argc, char **argv)
{

	nf_watchdog_dump();
	return 0;
}
__commandlist(watchdog_dump,"watchdog_dump",watchdog_dump_help, watchdog_dump_help);


static char watchdog_dump_file_help[] = "watchdog_dump_file";
static int watchdog_dump_file(int argc, char **argv)
{
	gint i=0;
	char buf[4*1024];
	char *buf_pos = buf;

	buf_pos += sprintf(buf_pos, "is_running [%d]\n", _nf_watchdog->is_running);
	for	(i=0;i<NF_WATCHDOG_MEMBER_NR;i++)
	{
		buf_pos += sprintf(buf_pos, "[%2d][%-16.16s] en[%d] max[%3d]cur[%3d] tv_sec[%ld]\n",
						i, _member_idx_to_str(i),
						_nf_watchdog->enable_cp[i],
						_nf_watchdog->max_cp[i],
						_nf_watchdog->cur_cp[i],
						_nf_watchdog->last_kick_tv[i].tv_sec);
	}

	{
		FILE *fp = NULL;
		fp = fopen( "/tmp/webra-info/watchdog_dump.txt", "w") ;
		if(fp){
			fwrite( buf, strlen(buf), 1, fp);
			fclose(fp);
		}
	}
	return 0;
}
__commandlist(watchdog_dump_file,"watchdog_dump_file",watchdog_dump_file_help, watchdog_dump_file_help);

#endif


#ifdef ENABLE_WATCHDOG_REBOOT_REASON

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

#endif

