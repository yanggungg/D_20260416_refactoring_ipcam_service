#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

#include <glib.h>
#include <math.h>

#include "nf_common.h"
#include "nf_event.h"
#include "nf_watchdog.h"
#include "jbshell.h"

#include "nf_notify.h"
#include "nf_util_device.h"
#include "nf_util_netif.h"
#include "nf_dspcomm_app.h"
#include "unp.h"
#include "nf_api_eventlog.h"
#include "nf_api_live.h"

#include "tw2880_api.h"

#include "nf_timer.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "nf_network.h"
#include "nf_sysman.h"
#include "nf_va_object_detector.h"

#include "nf_api_dlva.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "event"

#if !defined(_OTM_MODEL) && !defined(_SNF_MODEL)

#if defined(_NF_1648)||defined(_NF_0824)
	#define ENABLE_DSP_MOTION
#endif

#endif /* _OTM_MODEL */
#define ENABLE_NETIF_MON_NOTIFY
#define ENABLE_DISK_SMART_MON		// 2010-05-03 ???? 6:53:20 test
#define ENABLE_NET_LOGIN_FAIL_MON	// 2012-01-31 ???? 11:42:27 choissi
#define ENABLE_HW_BURST_TEST		// 2014-02-10 ???? 3:02:47 choissi

//#define DEBUG_EVENT_POE_ERROR
//#define DEBUG_EVENT_MOTION
//#define DEBUG_EVENT_SENSOR 
//#define DEBUG_EVENT_KEYPAD

//#define TEST_EVENT_CB
#ifdef ENABLE_DISK_SMART_MON
	#include "nf_api_disk.h"
	static 	NF_DISK_INFO  _Disk_info;
	
#endif 

#ifdef ENABLE_NET_LOGIN_FAIL_MON	// 2012-01-31 ???? 11:42:27 choissi
	
	#include <unistd.h>
	static gboolean	_event_net_login_fail_check_timer_cb_func(gpointer data);
	extern void nf_action_logon_fail_check(gchar *user_id, gboolean is_fail, gboolean is_dvr);	
	
#endif 
	
#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	extern guint _nf_action_num_alarm;
	extern guint _nf_action_num_nvr_alarm;
#endif

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

#if defined(_HDI_0412)
	NF_EVENT_VIDEO_STD _nf_event_std_type[NF_EVENT_VIDEO_IN_TYPE_MAX_NR] =
	{
		{HDI_VIDEO_IN_TYPE_1920x1080p60 , HDI_VIDEO_IN_TYPE_VAL_1920x1080p60 },     // uncertain value
		{HDI_VIDEO_IN_TYPE_1920x1080i60 , HDI_VIDEO_IN_TYPE_VAL_1920x1080i60 },
		{HDI_VIDEO_IN_TYPE_1680x1050p60 , HDI_VIDEO_IN_TYPE_VAL_1680x1050p60 },     // uncertain value
		{HDI_VIDEO_IN_TYPE_1280x720p60  , HDI_VIDEO_IN_TYPE_VAL_1280x720p60  },
		{HDI_VIDEO_IN_TYPE_1280x1024p60 , HDI_VIDEO_IN_TYPE_VAL_1280x1024p60 },     // uncertain value
		{HDI_VIDEO_IN_TYPE_1024x768p60  , HDI_VIDEO_IN_TYPE_VAL_1024x768p60  },     // uncertain value
		{HDI_VIDEO_IN_TYPE_800x600p60   , HDI_VIDEO_IN_TYPE_VAL_800x600p60   },     // uncertain value
		{HDI_VIDEO_IN_TYPE_720x576i50   , HDI_VIDEO_IN_TYPE_VAL_720x576i50   },     // uncertain value
		{HDI_VIDEO_IN_TYPE_720x480i60   , HDI_VIDEO_IN_TYPE_VAL_720x480i60   },     // uncertain value
		{HDI_VIDEO_IN_TYPE_1920x1080p30 , HDI_VIDEO_IN_TYPE_VAL_1920x1080p30 },
		{HDI_VIDEO_IN_TYPE_1280x720p30  , HDI_VIDEO_IN_TYPE_VAL_1280x720p30  },
		{HDI_VIDEO_IN_TYPE_1920x1080p25 , HDI_VIDEO_IN_TYPE_VAL_1920x1080p25 },
		{HDI_VIDEO_IN_TYPE_1280x720p50  , HDI_VIDEO_IN_TYPE_VAL_1280x720p50  },
		{HDI_VIDEO_IN_TYPE_1280x720p25  , HDI_VIDEO_IN_TYPE_VAL_1280x720p25  }
	};
#endif


/**
	Function Definition
 **/
static void nf_event_class_init (NfEventClass * klass);
static void nf_event_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_event_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_event_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_event_dispose (GObject * object);
static void nf_event_finalize (GObject * object);

static void _event_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static gboolean _event_timer_cb_func( gpointer data);

#if defined(_SNF_MODEL)
	static gboolean _event_motion_notify_refire_cb_func(void);
	static gboolean _event_regiter_motion_calback_timer_cb_func(gpointer data);
	static void _event_motion_cb_func(NFIPCamMotionRaw* mraw, gpointer user_data);
	static void _event_motion_rdata_cb_func(NFIPCamMotionRaw* mraw);
	static void _event_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
#endif

#if defined(ENABLE_SENSOR_IPCAM)
	static gboolean _event_regiter_ipcam_alarm_calback_timer_cb_func(gpointer data);
#endif

#if defined(ENABLE_SENSOR_IPCAM)
	static gboolean _event_sensor_in_ipcam(u_int sensor_in);
#endif

#if defined(ENABLE_FAN_FAIL_CHECK)
	static gboolean _event_fan_cb_func( gpointer data);
	#if defined(ENABLE_SW_FAN_CTRL)
		static void _nf_event_sw_fan_ctrl(NF_UTIL_FAN_INFO *fan_info);
	#endif
#endif
#if defined(ENABLE_POE_CHECK)
	static gboolean _event_poe_cb_func(gpointer data);
	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	   || defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
	static void nf_event_check_poe_power_over(guint tot_mW, guint tot_mW_hub); 
	#else
	static void nf_event_check_poe_power_over(guint tot_mW);
	#endif
#endif

static gboolean _event_regiter_xload_check_timer_cb_func(gpointer data);
static gboolean _event_sw_fan_ctrl_cb_func(gpointer data);

#if defined(USE_DEV_GENNUM)
	static gboolean gio_std_type_in (GIOChannel *gio, GIOCondition condition, gpointer data);
#endif
#if defined(_HDI_0412)
	static void _nf_event_invalid_video_check(void);
#endif
	

#if defined(ENABLE_DISK_SMART_MON)
	#if 1 // FIXME
		#define DISK_SMART_MON_INTERVAL		(60*60*12)	// 12 hour
		#define DISK_SMART_INIT_SEC			(60*5)		// 5 min after booting
		#define DISK_SMART_MAX_DISK_CNT		5
	#else // TEST
		#define DISK_SMART_MON_INTERVAL		(60)	// 12 hour
		#define DISK_SMART_INIT_SEC			(60*2)		// 5 min after booting
		#define DISK_SMART_MAX_DISK_CNT		16
#endif 

static guint _disk_mon_smart_interval = DISK_SMART_MON_INTERVAL;
static guint _disk_mon_sysdb_change = 0;

static gboolean
_event_disk_mon_timer_cb_func(gpointer data);

#endif 

static GObjectClass *parent_class = NULL;
static NfEvent *_nf_event = NULL;

static void
event_thread_func (NfEvent * test) ;

static void
event_watchdog_thread_func (NfEvent *self) ;

static gboolean 
nf_event_watchdog_kick( NF_EVENT_WATCHDOG_MEMBER_E module );

static gboolean 
nf_event_watchdog_ctrl( NF_EVENT_WATCHDOG_MEMBER_E module, guint interval_sec, gboolean enable);

static void
event_netmon_thread_func (NfEvent *self);

static void _sensor_init(void);
static void _motion_init(void);
#if defined(ENABLE_ARI_PANIC)
static void _sensor_init_ari_panic(void);
#endif
static void _nf_event_load_event_sensor_data(void);
static void _nf_event_load_event_motion_data(void);
static void _nf_event_load_event_system_sys_data(void);
static void _nf_event_load_system_magement_data(void);
static void _nf_event_load_event_system_net_data(void);
static void _nf_event_load_user_data(void);
static void _event_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

extern void nf_datetime_localtime(time_t *ttime, gboolean is_dst, struct tm *stTime);

static char *_eth_dev = HOST_ETH_DEV;

GType
nf_event_get_type (void)
{
	static GType nf_event_type = 0;

	if (G_UNLIKELY (nf_event_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfEventClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_event_class_init,
			NULL,
			NULL,
			sizeof (NfEvent),
			0,
			(GInstanceInitFunc) nf_event_instance_init,
			NULL
		};

		nf_event_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfEvent", &object_info, 0);
	}
	
	return nf_event_type;
}

static void
nf_event_class_init (NfEventClass * klass)
{	
	GObjectClass *gobject_class;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_event_set_property;
	gobject_class->get_property = nf_event_get_property;
			
	gobject_class->dispose = nf_event_dispose;
	gobject_class->finalize = nf_event_finalize;

}

static void
nf_event_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfEvent *self = NF_EVENT (instance);
	
	self->init_done = 0;

	// event context & loop
	self->context = g_main_context_new ();	
	self->loop = g_main_loop_new (self->context, FALSE);
				
	// queue ????
	self->queue = g_async_queue_new();

	self->net_event = 0;	
	self->thread_run = 1;

	memset( self->sensor_data, 0x00, sizeof(self->sensor_data) );

	self->thread = g_thread_create(	(GThreadFunc)event_thread_func, 
									self, FALSE, NULL);

	self->netmon_thread = g_thread_create(	(GThreadFunc)event_netmon_thread_func, 
									self, FALSE, NULL);
			
	self->thread_run_watchdog=TRUE;
	self->thread_watchdog = g_thread_create(    (GThreadFunc)event_watchdog_thread_func,
									self, FALSE, NULL);			
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_event_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_event_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_event_set_property (GObject * object, guint prop_id,
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
nf_event_get_property (GObject * object, guint prop_id,
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


guint
nf_event_src_add( int fd, GSourceFunc cb_func, gpointer data)
{
	GIOChannel	*gio_chan = NULL;
	GSource		*source;
	guint		ret = 0;
		
	g_return_val_if_fail (_nf_event != NULL, 0);
	g_return_val_if_fail (fd >0, 0);
	g_return_val_if_fail (cb_func != NULL, 0);
		
	gio_chan = g_io_channel_unix_new ( fd );

	//g_io_add_watch?? ????Ʈ context?? add?ϱ? ?????? ?̷??? ?????? ???? ?Ѵ?.
	//ret = g_io_add_watch (gio_chan, G_IO_IN | G_IO_HUP, cb_func, data);		
	
	source = g_io_create_watch (gio_chan,  G_IO_IN | G_IO_HUP | G_IO_ERR);
	
	//g_source_set_priority (source, G_PRIORITY_HIGH);	
	g_source_set_callback (source, (GSourceFunc)cb_func, data, NULL);	

	ret = g_source_attach (source, _nf_event->context );
	
	g_source_unref (source);
  				
	return ret;		
}				

guint
nf_event_timer_add( guint interval, GSourceFunc cb_func, gpointer data)
{
	GSource			*source;
	guint			ret = 0;

	g_return_val_if_fail (_nf_event != NULL, 0);
	g_return_val_if_fail (cb_func != NULL, 0);
						
	source = g_timeout_source_new (interval);
	
	// init event_timeout_cb
	g_source_set_callback (source, (GSourceFunc)cb_func, data, NULL);
	g_source_set_priority (source, G_PRIORITY_HIGH);

	ret = g_source_attach (source, _nf_event->context );

	g_source_unref(source);

	return ret;
}

gboolean
nf_event_timer_remove (guint tag)
{
  GSource *source;
  
  g_return_val_if_fail (_nf_event != NULL, 0);
  g_return_val_if_fail (tag > 0, FALSE);

  source = g_main_context_find_source_by_id (_nf_event->context, tag);
  
  if (source)
		g_source_destroy (source);

  return source != NULL;  
}

#define nf_event_src_remove 	nf_event_timer_remove

/******************************************************************************/

static gint g_dack_ctl_val = 0;
static void event_netmon_sysdb_change_func(NF_NOTIFY_INFO *p_info, gpointer p_data)
{
	if (p_info->d.params[0] == NF_SYSDB_CATE_NET)
		g_dack_ctl_val = (gint)nf_sysdb_get_uint("net.proto.dack");
}

static void
event_netmon_thread_func (NfEvent *self)
{	
	gint before_link_status=0, link_status=0, ret=0;
	gboolean  is_load_module=FALSE, net_event;
	gboolean dack_ctl_enable=FALSE;
	gulong cb_handle;
	gint old_link_status = 0;

#ifdef ENABLE_NETIF_MON_NOTIFY
	NF_NETIF_GET_STAT net_stat;
	NF_NETIF_GET_STAT delta_stat;
#endif 

	g_message("%s start", __FUNCTION__);
	
	dack_ctl_enable = nf_sysdb_get_bool("net.proto.dack_enable");
	g_dack_ctl_val = (gint)nf_sysdb_get_uint("net.proto.dack");
	cb_handle = nf_notify_connect_cb( "sysdb_change", event_netmon_sysdb_change_func, (gpointer)NULL);
	if (cb_handle == 0)
	{
		printf("[%s][%d] nf_notify_connect_cb return failure.\n", __FUNCTION__, __LINE__);
		return;
	}
	
#ifdef ENABLE_NETIF_MON_NOTIFY
	memset( &net_stat, 0x00, sizeof(net_stat));	
#endif

	nf_netif_get_link_status(_eth_dev, &old_link_status );

	while(self->thread_run)
	{
		nf_netif_get_link_status(_eth_dev, &link_status );
		
		if(before_link_status != link_status)
		{
			net_event=TRUE;

			g_warning("%s %s LINK status [%d]", __FUNCTION__, _eth_dev, link_status);
			g_warning("%s %s LINK status [%d]", __FUNCTION__, _eth_dev, link_status);

			before_link_status = link_status;

			if( link_status == NF_NETIF_LINK_STATUS_UP )
			{						
				nf_eventlog_put_param(NULL, LT_NETWORK_EVENT, 0, LP2_NETWORK_WAN_PORT_STATUS_ON, NULL);
			}
			else
			{
				nf_eventlog_put_param(NULL, LT_NETWORK_EVENT, 0, LP2_NETWORK_WAN_PORT_STATUS_OFF, NULL);
			}
			
#ifdef ENABLE_HW_BURST_TEST
			// For CBC-A2
			if (dack_ctl_enable)
			{
				if (g_dack_ctl_val == 10)
				{
					if(link_status)
						nf_sysman_set_delack(10);
					else
						nf_sysman_set_delack(4);
				}
				else
					nf_sysman_set_delack(g_dack_ctl_val);
			}
			else
			{
				if(link_status)
					nf_sysman_set_delack(10);
				else
					nf_sysman_set_delack(4);
			}
#endif
			
		}

		if(old_link_status != link_status)
		{
			old_link_status = link_status;

			if( link_status == NF_NETIF_LINK_STATUS_UP ){
				nf_network_dhcp_renew();
			}
		}

#ifdef ENABLE_NETIF_MON_NOTIFY
		// choissi 2010-04-21 ???? 4:33:12  ethernet monitor							
		// dev_name, prev, next, delta
		memset( &delta_stat, 0x00, sizeof(delta_stat));
		
		if( nf_netif_get_delta(_eth_dev, &net_stat, &net_stat, &delta_stat) )
		{
				
#ifdef	DEBUG_NETSVR_NET_STAT
			g_message("%s rx[%lld][%lld][%lld] tx[%lld][%lld][%lld] %d.%06d",  
				__FUNCTION__, 
				delta_stat.rx_byte, delta_stat.rx_packet, delta_stat.rx_error,
				delta_stat.tx_byte, delta_stat.tx_packet, delta_stat.tx_error,
				delta_stat.tval.tv_sec, delta_stat.tval.tv_usec );
#endif			
			nf_notify_fire_params("net_rxtx", 0 /* gServer_info.net_status */, 
										(unsigned int)(delta_stat.rx_byte/1024),
										(unsigned int)(delta_stat.tx_byte/1024),
										(guint)link_status);
		}
#endif 
		
		nf_netif_renew_dhcp();
		
		sleep(1);
	}
	g_message("%s end", __FUNCTION__);
}


static void
event_thread_func (NfEvent * self)
{	
	g_message("%s start", __FUNCTION__);	
	self->init_done = 1;
		
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
		int s=0;
		cpu_set_t cpuset;
		        
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



		// http://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html
		CPU_ZERO(&cpuset);
		CPU_SET(0, &cpuset);

		s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		if (s != 0)
			g_warning("pthread_setaffinity_np");
		/* Check the actual affinity mask assigned to the thread */

		s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		if (s != 0)
			g_warning("pthread_getaffinity_np");
    }
    	
	g_main_loop_run (self->loop); 		
	g_message("%s end", __FUNCTION__);
}

static const gchar *_nf_event_watchdog_member_str_arr[] ={

	"EVENT_TIMER",
	"MOTION",
	"DISK_MON",
	"DISK_TEMP",
	"DISK_MBID",
	"NET_LOGIN",
	"FAN",
	"ALARM",
	"POE"
};

static void
event_watchdog_thread_func (NfEvent *self)
{
	gint i=0;

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

    while(self->thread_run_watchdog)
    {
		if(!self->init_done_main)
		{
			g_usleep(1000);
			continue;
		}

		NF_OBJECT_LOCK(_nf_event);
		for (i=0;i<NF_EVENT_WATCHDOG_MEMBER_NR;i++)
		{
			#if !defined(ENABLE_DISK_SMART_MON)
				if(i == NF_EVT_WATCHDOG_MEMBER_DISK_MON)
					continue;
				if(i == NF_EVT_WATCHDOG_MEMBER_DISK_TEMP)
					continue;
			#endif
			#if !defined(ENABLE_DISK_MBID_CHECK)
				if(i == NF_EVT_WATCHDOG_MEMBER_DISK_MBID)
					continue;
			#endif
			#if !defined(ENABLE_NET_LOGIN_FAIL_MON)
				if(i == NF_EVT_WATCHDOG_MEMBER_NET_LOGIN)
					continue;
			#endif
			#if !defined(ENABLE_FAN_FAIL_CHECK)
				if(i == NF_EVT_WATCHDOG_MEMBER_FAN)
					continue;
			#endif
			#if !defined(ENABLE_POE_CHECK)
				if(i == NF_EVT_WATCHDOG_MEMBER_POE)
					continue;
			#endif
			
			if(self->wdt.is_start[i] == FALSE)
				continue;

			if(self->wdt.is_timer_killed[i])
			{
				g_message("%s [%s] Timer Killed", __FUNCTION__, _nf_event_watchdog_member_str_arr[i]);
				continue;
			}

			if( self->wdt.enable_cp[i] )
			{
				if( self->wdt.max_cp[i] < self->wdt.cur_cp[i] )
				{
					g_warning("%s Timer Not Running! [%2d][%s] max[%d] cur[%d] last[%ld]!!",
								__FUNCTION__, i,
								_nf_event_watchdog_member_str_arr[i],
								self->wdt.max_cp[i], self->wdt.cur_cp[i], self->wdt.last_kick_tv[i].tv_sec);
				}
			}
			#if 1
				g_message("%s line%d [%2d][%s] max %d cur %d last[%ld]", 
							__FUNCTION__, __LINE__, i, _nf_event_watchdog_member_str_arr[i], 
							self->wdt.max_cp[i], self->wdt.cur_cp[i], self->wdt.last_kick_tv[i].tv_sec);
			#endif
			++self->wdt.cur_cp[i];
		}
		NF_OBJECT_UNLOCK(_nf_event);

		sleep(5);
	}
}

static gboolean 
nf_event_watchdog_kick( NF_EVENT_WATCHDOG_MEMBER_E module )
{
	#if 0
		g_message("%s line%d kick -> %s", __FUNCTION__, __LINE__, _nf_event_watchdog_member_str_arr[module]);
	#endif
	NF_OBJECT_LOCK(_nf_event);
	_nf_event->wdt.cur_cp[module] = 0;
	g_get_current_time( &_nf_event->wdt.last_kick_tv[module] );
	NF_OBJECT_UNLOCK(_nf_event);
}

static gboolean 
nf_event_watchdog_ctrl( NF_EVENT_WATCHDOG_MEMBER_E module, guint interval_sec, gboolean enable)
{

	g_return_val_if_fail (_nf_event != NULL, FALSE);
	g_return_val_if_fail ( module < NF_EVENT_WATCHDOG_MEMBER_NR , FALSE);

	NF_OBJECT_LOCK(_nf_event);

	_nf_event->wdt.enable_cp[module] = (guint)enable;
	_nf_event->wdt.max_cp[module] = interval_sec;
	_nf_event->wdt.cur_cp[module] = 0;
	g_get_current_time( &_nf_event->wdt.last_kick_tv[module] );

	NF_OBJECT_UNLOCK(_nf_event);

	g_message("%s module %s max %d", 
				__FUNCTION__, _nf_event_watchdog_member_str_arr[module], _nf_event->wdt.max_cp[module]);

	return TRUE;
}

#ifdef TEST_EVENT_CB
static gboolean
event_timeout_cb (gpointer data)
{
	static gint cnt = 0;
	g_message("%s %d", __FUNCTION__, ++cnt);		
	return TRUE;
}

static void
testvec_motion_cb( NF_NOTIFY_INFO *pinfo, gpointer data )
{	
	g_return_val_if_fail(pinfo != NULL, 0);

	g_message("%s cb_data[%s] type[%d] [%d][%d][%d][%d] ", __FUNCTION__,  
					data,
					pinfo->type,
					pinfo->d.params[0],
					pinfo->d.params[1],
					pinfo->d.params[2],
					pinfo->d.params[3] );	
}

static gboolean
event_testvec_cb (gpointer data)
{
	static gint cnt = 0;
	static guint cb_handle = 0;
	
	g_message("%s %d", __FUNCTION__, ++cnt);
	
	if(cnt == 1)
		cb_handle= nf_notify_connect_cb( "motion", testvec_motion_cb, "motion");
	
	nf_notify_fire_params("motion", (1<<cnt%4),0,0,0);
		
	return TRUE;
}
#endif

/******************************************************************************/
#ifdef DEBUG_EVENT_KEYPAD
static gboolean gio_keypad_in (GIOChannel *gio, GIOCondition condition, gpointer data)
{
	GIOError ret;
	gchar buff[32];
	
	gsize buff_ret; 
	
	g_print("keypad device   ");
	if (condition & G_IO_HUP)
	{
		g_warning("%s Read end of pipe died![%d]", __FUNCTION__, ret);		
	}	

	ret = g_io_channel_read(gio, buff, 1, &buff_ret);		
	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s error[%d]", __FUNCTION__, ret);
	} 

	g_message("Read %u bytes pressed[%d]  [0x%02x]", buff_ret, buff[0]&0x80 ? 1:0, buff[0]&0x3f );
			
	return TRUE;
}
#endif

/*
	sensor
*/
#if defined ( _ANF_1648 )
static gboolean gio_pannel_in (GIOChannel *gio, GIOCondition condition, gpointer data)
{
	GIOError ret;
	guint buff[32]={0,};
	gsize buff_ret; 
	guint val=0;

	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	ret = g_io_channel_read(gio, buff, 8, &buff_ret);	// driver (u32)
	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s g_io_channel_read ret[%d] fd[%d]", __FUNCTION__, ret, g_io_channel_unix_get_fd(gio));
		return TRUE;
	} 
	
	
	val = buff[0] | buff[1];
	nf_notify_fire_params((unsigned char *)data, buff[0]&0xfffff, 0, 0, 0);
#if 0
	printf("====================> val [%08x] buff[0] ==> [%x] buff[1] ==> [%x]\n", val, buff[0], buff[1]);
//	nf_notify_fire_params((unsigned char *)data, &val, 0, 0, 0);
#endif
	return TRUE;
}
#else
static gboolean gio_pannel_in (GIOChannel *gio, GIOCondition condition, gpointer data)
{
	guint i = 0;
	//guint alarm = 0;
	GIOError ret;
	guint buff[32];
	gsize buff_ret; 
	guint ari_panic;

	#if defined(ENABLE_SENSOR_IPCAM)
		guint alarm_curr_ipcam=0;
	#endif

	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	ret = g_io_channel_read(gio, (gchar *)buff, 8, (gsize *)&buff_ret);	// driver (u32)
	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s g_io_channel_read ret[%d] fd[%d]", __FUNCTION__, ret, g_io_channel_unix_get_fd(gio));
		return TRUE;
	} 

	int ain_dvr=0;
	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)
	{
	    #if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
	  	ain_dvr = 16;
	    #elif defined(_IPX_0824M4) || defined(_IPX_0824M4E) || defined(_IPX_0824P4E)
		ain_dvr = 8;
	    #elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
	  	ain_dvr = 16;	    
	    #else
		ain_dvr = 4;
	    #endif
	}
	else 
	{
	    ain_dvr = 2;
		
	    #if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
	  	; /*    */ 
	    #endif
	}
	ari_panic=((buff[0] >> 16) & 0x3);

#if defined(ENABLE_SENSOR_IPCAM)
	alarm_curr_ipcam = nf_notify_get_param1("sensor");

	// captainnn todo
	_nf_event->physical_alarm_mask = buff[0];
	_nf_event->physical_alarm_mask = (_nf_event->physical_alarm_mask << NUM_ALARM_IPCAM) | alarm_curr_ipcam;
	/*
	alarm = (buff[0] << NUM_ALARM_IPCAM);
	for(i = 0; i < ain_dvr ;i++)
	{
		if ( alarm & (1 << (i+16)) )
			_nf_event->physical_alarm_mask |= (1 << (i+16));
		if ( (_nf_event->physical_alarm_mask & (1 << (i+16))) && !(alarm & (1 << (i+16))) )
			_nf_event->physical_alarm_mask -= pow(2, (i+16));
	}
	*/
#endif
	//printf("\e[33m [%s] alarm_curr_dvr = 0x%x alarm_curr_ipcam = 0x%x \e[0m\n", __FUNCTION__, alarm, alarm_curr_ipcam);
	printf("\e[33m [%s] alarm_curr_dvr = 0x%x alarm_curr_ipcam = 0x%x \e[0m\n", __FUNCTION__, (buff[0] & 0xffffffff), alarm_curr_ipcam);
	printf("\e[33m [%s] ari_panic = 0x%x \e[0m\n", __FUNCTION__, ari_panic);

	nf_notify_fire_params((gchar *)data, (buff[0] & 0xffffffff), alarm_curr_ipcam, 0, ari_panic);

	return TRUE;
}

#if defined(ENABLE_SENSOR_IPCAM)
static gboolean _event_sensor_in_ipcam_cb_func(guint mask, gpointer user_data)
{
	guint i = 0;
	guint alarm_curr_dvr=0, alarm_next=0;
	alarm_curr_dvr = nf_notify_get_param0("sensor");

	//captainnn todo
	_nf_event->physical_alarm_mask =(alarm_curr_dvr << NUM_ALARM_IPCAM) | mask;
	/*
	for ( i = 0; i < NUM_ACTIVE_CH; i++ )
	{
		if ( mask & (1 << i) )
			_nf_event->physical_alarm_mask |= (1 << i);
		if ( (_nf_event->physical_alarm_mask & (1 << i)) && !(mask & (1 << i)) )
			_nf_event->physical_alarm_mask -= pow(2, i);
	}
	*/
	//alarm_next = (mask | (alarm_curr & 0xffff0000));

	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)		// A Type
	{
	   #if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		nf_notify_fire_params("sensor", alarm_curr_dvr, mask, 0, 0);		// virutal alarm phyical alarm check param1
	   #elif defined(_IPX_0824M4) || defined(_IPX_0824P4E) || defined(_IPX_0824M4E) 
		nf_notify_fire_params("sensor", alarm_curr_dvr, mask, 0, 0);		// virutal alarm phyical alarm check param1
	   #elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		nf_notify_fire_params("sensor", alarm_curr_dvr, mask, 0, 0);		// virutal alarm phyical alarm check param1
	   #else
		nf_notify_fire_params("sensor", alarm_curr_dvr, mask, 0, 0);		// virutal alarm phyical alarm check param1
	   #endif
	}
	else 	// B Type
	{
		nf_notify_fire_params("sensor", alarm_curr_dvr, mask, 0, 0);		// virutal alarm phyical alarm check param1
		
	   #if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		; /*    */ 	
	   #endif
	}
	   printf("\e[33m [%s] alarm_curr_ipcam = 0x%x \e[0m\n", __FUNCTION__,mask);
	return TRUE;
}
#endif

#endif

/*
	video loss
*/
static gboolean gio_vloss_in (GIOChannel *gio, GIOCondition condition, gpointer data)							
{
	GIOError ret;
	guint buff=0, curr_vloss=0, novid_mask=0;
	gsize buff_ret=0;

	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	//tw2864 ?????? ?ݵ??? 1?? ?о? ?־??? ??
	ret = g_io_channel_read(gio, (gchar *)&buff, 1, (gsize *)&buff_ret);

	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s g_io_channel_read ret[%d] fd[%d]", __FUNCTION__, ret, g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	curr_vloss = (buff & _nf_event->vloss_mask);
#ifdef ENABLE_HNF_IPCAM
	{
		guint ipcam_vloss = nf_notify_get_param1("vloss");
		nf_notify_fire_params((unsigned char *)data, 
					curr_vloss|(ipcam_vloss<<BASE_IPCAM_CHANNEL),
					ipcam_vloss, 0, 0);
	}
#else

	_nf_event->novid_mask |= (~(curr_vloss) & NUM_ACTIVE_CH_MASK);
	novid_mask=~(_nf_event->novid_mask);

	#if defined(_HDI_0412)
		_nf_event_invalid_video_check();

		_nf_event->invalid_vid_mask &= (~(curr_vloss & NUM_ACTIVE_CH_MASK));
		#if 0
			g_message("%s Vloss[0x%08x]Invalid[0x%08x] Novid[0x%08x]", 
							__FUNCTION__, curr_vloss, _nf_event->invalid_vid_mask, novid_mask);
		#endif

		curr_vloss |= _nf_event->invalid_vid_mask;
		nf_notify_fire_params((gchar *)data, curr_vloss, novid_mask, _nf_event->invalid_vid_mask, 0);
	#else
		nf_notify_fire_params((gchar *)data, curr_vloss, novid_mask, 0, 0);
	#endif
#endif

	return TRUE;
}

#if defined(_HDI_0412)
static void _nf_event_invalid_video_check(void)
{
	gint ch=0, vid_type=0;
	guint type_db=0;
	guchar type_cam=0;
	gchar tmp_key[256]={0, };

	NF_OBJECT_LOCK( _nf_event );
	_nf_event->invalid_vid_mask=0;
	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		/** get to sysdb **/
		sprintf(tmp_key, "cam.C%d.type", ch);
		type_db=nf_sysdb_get_uint(tmp_key);
		
		/** get to driver **/
		type_cam=0;
		nf_dev_gennum_get_video_stardard(ch, &type_cam);

		for(vid_type=0; vid_type<HDI_VIDEO_IN_TYPE_NR; vid_type++)
		{
			if(_nf_event_std_type[vid_type].in_type == type_db)
			{
				#if 0	
					g_message("%s ch[%d] in_type_db[%d] type_db[0x%02x] type_cam[0x%02x]", __FUNCTION__, ch, 
										_nf_event_std_type[vid_type].in_type, _nf_event_std_type[vid_type].val, type_cam);
				#endif
				if(_nf_event_std_type[vid_type].val != type_cam)
					_nf_event->invalid_vid_mask |= (1<<ch);
			}
		}
	}
	NF_OBJECT_UNLOCK( _nf_event );
}
#endif

#if defined(USE_DEV_GENNUM)
/*
	Vided Standard Type Check In Gennum Receiver Driver
*/
static gboolean gio_std_type_in (GIOChannel *gio, GIOCondition condition, gpointer data)
{
	GIOError ret=0;
	guint buff[2]={0, }, mask=0;
	static guchar buff_std[NUM_ACTIVE_CH]={0, };
	guchar buff_tmp[NUM_ACTIVE_CH]={0, };
	gsize buff_ret=0;
	gint i=0, ch=0;
	static guint std_type=0;

	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	ret = g_io_channel_read(gio, (gchar *)&buff, 1, (gsize *)&buff_ret);

	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s g_io_channel_read ret[%d] fd[%d]", __FUNCTION__, ret, g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

#if 0
	g_message("%s called... 0x%08x 0x%08x", __FUNCTION__, buff[0], buff[1]);
#endif

	nf_notify_fire_params((gchar *)data, (buff[0] & NUM_ACTIVE_CH_MASK), buff[1], 0, 0);

	return TRUE;
}
#endif

#ifdef ENABLE_DSP_MOTION
static gboolean gio_dsp_motion_in (GIOChannel *gio, GIOCondition condition, gpointer data)
{
	gchar buff[MAX_CMD_BUFF];	
	static guint motion_flag = 0;		
	gint ret;
	gint dspid = (gint)data;
	guint tmp_flag = motion_flag;
	
	DPEVT_MD_RES *pmd_res = &buff[sizeof(DPACKET)];
	
	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , 
					g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	memset(buff, 0x00, 0x100);
	
	ret = nf_dspcomm_recv_cmd(dspid, NF_DSPCOMM_CH_MDEVT, buff, 0);
#ifdef DEBUG_EVENT_MOTION	
	g_message("%s dspid[%d] nf_dspcomm_recv_cmd ret[%d] ch[%02d][%d]", __FUNCTION__, 
			dspid, ret, pmd_res->ch, pmd_res->detected);	
#endif
	if( pmd_res->detected == 1)
	{
		tmp_flag |= (1 << pmd_res->ch);
	}else{
		tmp_flag &= ~(1 << pmd_res->ch);
	}

	if( tmp_flag != motion_flag )
	{							
		motion_flag = tmp_flag;
		nf_notify_fire_params("motion", motion_flag,0,0,0);
	}
	
	return TRUE;
}
#endif

#if defined(_ANF_1648) || defined(_ATM_1624) || defined(_OTM_MODEL)
static gboolean gio_motion_in (GIOChannel *gio, GIOCondition condition, gpointer data)		//hosik_motion
{
	GIOError ret;
	guint buff;
	gsize buff_ret;
	static guint motion_flag = 0;

	if (condition & G_IO_HUP)
	{
		g_warning("%s HUP cond[%d] fd[%d]",__FUNCTION__ , condition , g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	ret = g_io_channel_read(gio, &buff, 1, &buff_ret);
#ifdef DEBUG_EVENT_MOTION	
	g_message("[gio_motion_in] buff_ret=%d,buff=0x%x",buff_ret,buff);						//hosik_rem
#endif
	if (ret != G_IO_ERROR_NONE)
	{
		g_warning("%s g_io_channel_read ret[%d] fd[%d]", __FUNCTION__, ret, g_io_channel_unix_get_fd(gio));
		return TRUE;
	}

	if(  motion_flag != (buff&0x0000ffff) )
	{
		motion_flag = (buff&0x0000ffff);
		nf_notify_fire_params((unsigned char *)data, motion_flag ,0,0,0);			//?????? ???? ?????? ????; ???? ???? d?? 16 ch ?? ???? ??
	}
	
	return TRUE;
}
#endif

// IPX Motion CallBack Function
#if defined(_SNF_0824)
static gboolean gio_motion_in(void)
{
	
	return TRUE;
}

#endif

#if defined(_SNF_MODEL_XXX)
static gboolean tw2880_motion_timer(gpointer data)
{
	tw2880_motion_data_type motion_param;
	guint i, buff=0;
	static guint motion_flag = 0;

	if ( itx_tw2880_get_motion_detection(&motion_param) == -1)
		g_message("Error [ itx_tw2880_get_motion_detection fail.]");

//	g_message("<%s, %d> motion_buff : %08X", __FUNCTION__, __LINE__, motion_param.dection_flag);

	if(  motion_flag != motion_param.dection_flag)
	{
		motion_flag = motion_param.dection_flag;
		motion_flag &= 0x0000ffff;
		nf_notify_fire_params((unsigned char *)data, motion_flag ,0,0,0);			//?????? ???? ?????? ????; ???? ???? d?? 16 ch ?? ???? ??
	}
    return TRUE;
}
#endif

#if defined(ENABLE_ARI_PANIC)
/* ari, panic bit on */
static void _sensor_init_ari_panic(void)
{
	gint sensor_ch=0;
	
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (sensor_ch = 16; sensor_ch < ARI_PANIC_LOCATION + NUM_ALARM_ARI_PANIC; ++sensor_ch)
	#else
	for (sensor_ch=ARI_PANIC_LOCATION ; sensor_ch < ARI_PANIC_LOCATION+NUM_ALARM_ARI_PANIC; ++sensor_ch)
	#endif
		nf_dev_sensor_ch_onoff(sensor_ch, NF_SENSOR_ON);
}
#endif

static
void _sensor_init(void)
{
	gint sensor_ch=0, on_off=0;
	guint sensor_bit=0;
	#if defined(ENABLE_SENSOR_IPCAM)
		guint sensor_bit_dvr=0, sensor_bit_ipcam=0;
		GError **error;
	#endif

	g_message("%s called", __FUNCTION__);

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (sensor_ch = 0; sensor_ch < _nf_action_num_alarm; ++sensor_ch) 
	#else
	for (sensor_ch = 0; sensor_ch < NUM_ALARM; ++sensor_ch) 
	#endif
	{
		if(_nf_event->sensor_data[sensor_ch].op_type == NF_SENSOR_TYPE_NC){		// N/C
			#if defined(ENABLE_SENSOR_IPCAM)
				if(sensor_ch < NUM_ALARM_IPCAM)
					sensor_bit_ipcam |= (1 << sensor_ch);
				else
					sensor_bit_dvr |= (1 << (sensor_ch - NUM_ALARM_IPCAM));
			#else
				sensor_bit |= (1<<sensor_ch);
			#endif
		}
	}

	#if defined(ENABLE_SENSOR_IPCAM)
		nf_dev_sensor_set_type(sensor_bit_dvr);
		/**
		  To Do
		  --> ipcam set sensor type
		**/
		for(sensor_ch=0; sensor_ch<NUM_ALARM_IPCAM; sensor_ch++)
		{
			if(sensor_bit_ipcam & (1<<sensor_ch))
				nf_ipcam_set_alarm_in(sensor_ch, 1, NF_IPCAM_ALARM_TYPE_NC, NULL, NULL, error);
			else
				nf_ipcam_set_alarm_in(sensor_ch, 1, NF_IPCAM_ALARM_TYPE_NO, NULL, NULL, error);
		}
	#else
		nf_dev_sensor_set_type(sensor_bit);
	#endif

	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (sensor_ch = 0; sensor_ch < _nf_action_num_alarm; ++sensor_ch)
	#else
	for (sensor_ch = 0; sensor_ch < NUM_ALARM; ++sensor_ch)
	#endif
	{

		on_off = 1;
		
		#if defined(ENABLE_SENSOR_IPCAM)
			if(sensor_ch < NUM_ALARM_IPCAM)
			{
				/**
				  To Do
				  --> ipcam set sensor off
				**/
				;
			}
			else
			{
				gint ch=0;

				ch=(sensor_ch - NUM_ALARM_IPCAM);
				nf_dev_sensor_ch_onoff(ch, on_off);
			}
		#else
			nf_dev_sensor_ch_onoff(sensor_ch, on_off);
		#endif
	}
}

static void
_nf_event_load_event_sensor_data(void)
{
	gint sensor_ch=0;
	char tmp_key[256]={0, };

	g_message("%s called", __FUNCTION__);
	
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	for (sensor_ch = 0; sensor_ch < _nf_action_num_alarm; ++sensor_ch)
	#else
	for (sensor_ch = 0; sensor_ch < NUM_ALARM; ++sensor_ch)
	#endif
	{

		sprintf(tmp_key, "alarm.sensor.S%d.op_type", sensor_ch);
		_nf_event->sensor_data[sensor_ch].op_type = nf_sysdb_get_bool(tmp_key);

		#ifdef DEBUG_EVENT_SENSOR
			g_message("%s alarm.sensor.S%d.op_type [%d] " , __FUNCTION__, 
						sensor_ch , _nf_event->sensor_data[sensor_ch].op_type);		
		#endif
	}
}

static void
_nf_event_load_event_system_sys_data(void)
{
	gchar tmp_key[256]={0, };

	g_message("%s called", __FUNCTION__);

	sprintf(tmp_key, "act.sys.sys.logon_fail.cnt");
	_nf_event->system_ddata.logOn_fail_cnt = nf_sysdb_get_int(tmp_key);

	#if defined(ENABLE_POE_CHECK)	
		sprintf(tmp_key, "act.sys.sys.poe_fail.threshold");
		_nf_event->system_ddata.poe_fail_threshold = nf_sysdb_get_int(tmp_key);
	#endif
}

static void
_nf_event_load_system_magement_data(void)
{
	#if defined(ENABLE_POE_CHECK)	
		gchar tmp_key[256]={0, };
	#endif

	g_message("%s called", __FUNCTION__);

	#if defined(ENABLE_POE_CHECK)	
		sprintf(tmp_key, "sys.info.poe_limit");
		_nf_event->system_ddata.poe_limit = nf_sysdb_get_uint(tmp_key);
		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
		 || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
			sprintf(tmp_key, "sys.info.poe_hub_limit");
			_nf_event->system_ddata.poe_limit_hub = nf_sysdb_get_uint(tmp_key);
		#endif
	#endif
}

static void
_nf_event_load_event_system_net_data(void)
{
	gchar tmp_key[256]={0, };

	g_message("%s called", __FUNCTION__);

	sprintf(tmp_key, "act.sys.net.rfail.cnt");
	_nf_event->net_ddata.logOn_fail_cnt = nf_sysdb_get_int(tmp_key);
}

static 
void _nf_event_load_user_data(void)
{
	gchar tmp_key[256]={0, };
	gint user_cnt=0;

	g_message("%s called", __FUNCTION__);

	memset(&_nf_event->user_data, 0x0, sizeof(USER_DATA)*NF_EVENT_USER_MAX);

	for(user_cnt=0; user_cnt<NF_EVENT_USER_MAX; user_cnt++)
	{
		gchar *s=NULL;
		USER_DATA *udata=&_nf_event->user_data[user_cnt];

		sprintf(tmp_key, "usr.U%d.name", user_cnt);
		s=nf_sysdb_get_str_nocopy(tmp_key);

		strncpy(udata->name, s, NF_EVENT_USER_NAME_MAX_LEN);
		udata->logOn_fail_cnt_dvr=0;
		udata->logOn_fail_cnt_net=0;
	}
}

static
void _motion_init(void)
{
	memset(_nf_event->motion_status, 0x0, sizeof(MOTION_STATUS) * NUM_ACTIVE_CH);
}

static void
_nf_event_load_event_motion_data(void)
{
	gint ch=0;
	char tmp_key[256]={0, };
	char *s=NULL;

	g_message("%s called", __FUNCTION__);
	
	memset(_nf_event->motion_data, 0x0, sizeof(MOTION_DATA_EVENT) * (NUM_ACTIVE_CH+1));

	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		sprintf(tmp_key, "alarm.motion.M%d.act", ch);
		_nf_event->motion_data[ch].motion_act = nf_sysdb_get_bool(tmp_key);
		
		sprintf(tmp_key, "alarm.motion.M%d.detect", ch);
		_nf_event->motion_data[ch].detect = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "alarm.motion.M%d.sense_d", ch);
		_nf_event->motion_data[ch].sense_d = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "alarm.motion.M%d.sense_n", ch);
		_nf_event->motion_data[ch].sense_n = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "alarm.motion.M%d.mini_d", ch);
		_nf_event->motion_data[ch].mini_d = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "alarm.motion.M%d.mini_n", ch);
		_nf_event->motion_data[ch].mini_n = nf_sysdb_get_uint(tmp_key);

		sprintf(tmp_key, "alarm.motion.M%d.area", ch);
		s=nf_sysdb_get_str_nocopy(tmp_key);
		memcpy(_nf_event->motion_data[ch].area, s, NF_MOTION_CELL_MIN_NUM);

		sprintf(tmp_key, "alarm.motion.M%d.time_start", ch);
		_nf_event->motion_data[ch].time_start = nf_sysdb_get_uint(tmp_key);
		sprintf(tmp_key, "alarm.motion.M%d.time_end", ch);
		_nf_event->motion_data[ch].time_end = nf_sysdb_get_uint(tmp_key);
	}

}


/**
	@brief				event ?ʱ?ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_event_init(int wait)
{
	gboolean ret=TRUE;
	guint tmp=0, cb_handle=0;
	
	gint fd, dspid;
	
	g_return_val_if_fail (_nf_event == NULL, FALSE);	
	
	_eth_dev = nf_netif_get_eth_str();

	_nf_event = g_object_new ( NF_TYPE_EVENT , NULL);
	
//	nf_event_timer_add( 1000, event_timeout_cb, _nf_event);
//	nf_event_timer_add( 2000, event_testvec_cb, _nf_event);

	_motion_init();
	_nf_event_load_event_sensor_data();
	_nf_event_load_event_system_sys_data();
	_nf_event_load_event_system_net_data();
	_nf_event_load_user_data();
	_nf_event_load_system_magement_data();

	
#ifndef _HDI_0412
	_nf_event_load_event_motion_data();		// use ipx
#endif  /**/

	fd = nf_dev_open_sensor();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_sensor Failed!! ret[%d]", __FUNCTION__, fd);
    	//g_assert(0);
    }else{
    	g_message("%s  nf_dev_open_sensor fd[%d]", __FUNCTION__,  fd);
		nf_dev_sensor_all_enable(1);

		_sensor_init();
		#if defined(ENABLE_ARI_PANIC)
		_sensor_init_ari_panic();
		#endif

   		cb_handle = nf_event_src_add(fd , (GSourceFunc)gio_pannel_in , "sensor");
		g_message("%s nf_event_src_add[sensor] cb_handle[%d]", 
					__FUNCTION__, cb_handle);
		//nf_notify_fire_params("sensor", nf_dev_sensor_get(),0,0,0);
	}
		
	cb_handle= nf_notify_connect_cb( "sensor", _event_cb_func , NULL );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#ifdef DEBUG_TEST_SENSOR
	nf_dev_sensor_ch_onoff(0,1);
	nf_dev_sensor_ch_onoff(1,1);
	nf_dev_sensor_ch_onoff(2,1);
	nf_dev_sensor_ch_onoff(3,1);
#endif

//	nf_dev_open_relay();
//	nf_dev_relay_set(0,1);
#ifdef DEBUG_TEST_RELAY 
    // move to nf_action.c
	nf_dev_open_relay();
	nf_dev_relay_set(0,1);
	nf_dev_relay_set(3,1);
	nf_dev_relay_set(5,1);
	nf_dev_relay_set(7,1);
#endif

#ifdef ENABLE_HNF_IPCAM
	{
		int i;
		guint ipcam_max_cnt = nf_sysdb_get_uint ("ipcam.CCNT");
		guint active_ch_cnt = NUM_ACTIVE_CH - ipcam_max_cnt;

		_nf_event->vloss_mask = 0;

		for(i=0;i<active_ch_cnt;++i)
		{
			_nf_event->vloss_mask |= (1<<i);
		}
		g_message("%s vloss_mask [0x%08x]", __FUNCTION__, _nf_event->vloss_mask);
	}
#else
	_nf_event->vloss_mask = NUM_ACTIVE_CH_MASK;
#endif

	_nf_event->virtual_alarm_mask = 0;		// virtual alarm init yys
	_nf_event->physical_alarm_mask = 0;

#if defined(_SNF_MODEL)
	cb_handle= nf_notify_connect_cb( "vloss", _event_vloss_cb_func, NULL);
	g_message("%s vloss connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	nf_event_timer_add( 2000, _event_regiter_motion_calback_timer_cb_func, NULL);
#elif defined(_HDI_0412)
	#if defined(USE_DEV_GENNUM)
		fd = nf_dev_open_gennum();
		if(fd < 0)
		{
			g_warning("%s nf_dev_open_gennum Failed!! ret[%d]", __FUNCTION__, fd);
			//g_assert(0);
		}else{
			guint curr_vloss=0, novid_mask=0;

			g_message("%s  nf_dev_open_gennum fd[%d]", __FUNCTION__,  fd);
			nf_dev_gennum_rx_set_vloss(_nf_event->vloss_mask);		// vloss on/off status

			nf_dev_gennum_get_vloss(&curr_vloss);
			g_message("%s curr_vloss[0x%08x]", __FUNCTION__, curr_vloss);

			_nf_event->novid_mask |= ~(curr_vloss);
			 novid_mask=(~(_nf_event->novid_mask) & NUM_ACTIVE_CH_MASK);

			_nf_event_invalid_video_check();

			_nf_event->invalid_vid_mask &= (~(curr_vloss & NUM_ACTIVE_CH_MASK));
			#if 0
				g_message("%s Vloss[0x%08x]Invalid[0x%08x] Novid[0x%08x]", 
								__FUNCTION__, curr_vloss, _nf_event->invalid_vid_mask, novid_mask);
			#endif

			curr_vloss |= _nf_event->invalid_vid_mask;
			nf_notify_fire_params("vloss", curr_vloss, novid_mask, _nf_event->invalid_vid_mask, 0);

			cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_vloss_in , "vloss");
			g_message("%s nf_event_src_add[vloss] cb_handle[%d]", 
						__FUNCTION__, cb_handle);
		}

		fd = nf_dev_open_gennum_minor1();
		if(fd < 0)
		{
			g_warning("%s nf_dev_open_gennum_minor1 Failed!! ret[%d]", __FUNCTION__, fd);
			//g_assert(0);
		}else{

			g_message("%s  nf_dev_open_gennum_minor1 fd[%d]", __FUNCTION__,  fd);

			cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_std_type_in , "std_type");
			g_message("%s nf_event_src_add[std_type] cb_handle[%d]", 
						__FUNCTION__, cb_handle);
		}
	#endif
#else
#if defined(USE_DEV_TW2864)
	fd = nf_dev_open_tw2864();
	if(fd < 0)
	{
		g_warning("%s nf_dev_open_tw2864 Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
	}else{

		guint curr_vloss = 0;    
		guint ipcam_vloss = nf_notify_get_param1("vloss");

		g_message("%s  nf_dev_open_tw2864 fd[%d]", __FUNCTION__,  fd);

		nf_dev_tw2864_init(1);

		curr_vloss = nf_dev_tw2864_get_vloss_status()&_nf_event->vloss_mask;

		nf_notify_fire_params("vloss",  (curr_vloss)|(ipcam_vloss<<BASE_IPCAM_CHANNEL),
								ipcam_vloss, 0, 0);

		cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_vloss_in , "vloss");
		g_message("%s nf_event_src_add[vloss] cb_handle[%d]", 
					__FUNCTION__, cb_handle);

		nf_dev_tw2864_set_dac(NF_TW2864_DAC_PLAYBACK);

	#ifdef DEBUG_TEST_TW2864
		nf_dev_tw2864_set_picture(0,50,50,50,50);
		nf_dev_tw2864_set_ntsc_pal(0);
		nf_dev_tw2864_get_signal_type();
	#endif
#endif
#endif

#ifdef DEBUG_EVENT_KEYPAD
	fd = nf_dev_open_keypad();
	nf_dev_keypad_dev_enable();
	
	g_message("%s  nf_device_open_keypad fd[%d]", __FUNCTION__,  fd);
	
	cb_handle = nf_event_src_add(fd , gio_keypad_in ,"keypad");
	g_message("%s nf_event_src_add keypad cb_handle[%d]", __FUNCTION__, cb_handle);
#endif

#ifdef ENABLE_DSP_MOTION
	for(dspid=0; dspid<NUM_ACTIVE_DSP; ++dspid)
	{
		fd = nf_dspcomm_open_chan(dspid, NF_DSPCOMM_CH_MDEVT);
	    if(fd<0)
	    {
	    	 g_warning("%s nf_dspcomm_open_chan Failed!! ret[%d]", __FUNCTION__, fd);
	    	 //g_assert(0);
	    }else{

			g_message("%s  nf_dspcomm_open_chan fd[%d]", __FUNCTION__,  fd);

		   	cb_handle = nf_event_src_add(fd, gio_dsp_motion_in , (gpointer)dspid);
			g_message("%s nf_event_src_add[dsp_motion](%d) cb_handle[%d]", 
						__FUNCTION__, dspid, cb_handle);
		}
	}
#elif defined(_ANF_1648) || defined(_ATM_1624)
	fd = nf_dev_open_fs1648();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_fs1648 Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
    }else{

    	g_message("%s  nf_dev_open_fs1648 fd[%d]", __FUNCTION__,  fd);

    	cb_handle = nf_event_src_add(fd, gio_motion_in , "motion");
    	g_message("%s nf_event_src_add[motion] cb_handle[%d]",
    				__FUNCTION__, cb_handle);
    }

#elif defined(_HDI_0412)

#elif defined(_OTM_MODEL)
	fd = nf_dev_open_solo_vin();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_solo_vin Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
    }else{

    	g_message("%s  nf_dev_open_solo_vin fd[%d]", __FUNCTION__,  fd);

    	cb_handle = nf_event_src_add(fd, gio_motion_in , "motion");
    	g_message("%s nf_event_src_add[motion] cb_handle[%d]",
    				__FUNCTION__, cb_handle);
    }
#elif defined(_SNF_MODEL_XXX)
	g_timeout_add(200, tw2880_motion_timer, "motion");
#endif

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _event_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_EVENT_TIMER, NF_EVENT_WATCHDOG_TIME_SEC, TRUE);	// 10ms
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_MOTION, NF_EVENT_WATCHDOG_TIME_SEC * 2, TRUE);	// 100ms
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_ALARM, NF_EVENT_WATCHDOG_TIME_SEC * 2, TRUE);		// 100ms
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_POE, NF_EVENT_WATCHDOG_TIME_SEC * 2, TRUE);		// 10ms
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_DISK_MON, NF_EVENT_WATCHDOG_TIME_SEC, TRUE);		// 1sec
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_DISK_TEMP, NF_EVENT_WATCHDOG_TIME_SEC, TRUE);		// 1sec
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_DISK_MBID, NF_EVENT_WATCHDOG_TIME_SEC, TRUE);		// 1sec
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_NET_LOGIN, NF_EVENT_WATCHDOG_TIME_SEC, TRUE);		// 1sec
	nf_event_watchdog_ctrl(NF_EVT_WATCHDOG_MEMBER_FAN, (NF_EVENT_WATCHDOG_TIME_SEC * 2), TRUE);		// 10sec

	//register timer callback function
	nf_event_timer_add( 1000, _event_timer_cb_func, NULL);
	
	#if defined(ENABLE_SENSOR_IPCAM)
		nf_event_timer_add( 2000, _event_regiter_ipcam_alarm_calback_timer_cb_func, NULL);
	#endif

	// 20110422	
	#if defined(ENABLE_FAN_FAIL_CHECK)
//		nf_event_timer_add( (1000 * 10), _event_fan_cb_func, NULL);
		nf_event_timer_add( (1000 * 3), _event_fan_cb_func, NULL);
	#endif

	#if defined(ENABLE_POE_CHECK)
	//ksi_test
		nf_event_timer_add( (5000), _event_poe_cb_func, NULL); // for normal priority thread
	#endif
	
	// 20111101
	nf_event_timer_add( 2000, _event_regiter_xload_check_timer_cb_func, "Xfbdev");
#ifdef ENABLE_DISK_SMART_MON

	memset( &_Disk_info, 0x00, sizeof(NF_DISK_INFO) );
	
	if(!nf_disk_get_info(&_Disk_info, NULL)) {
		g_warning("%s  nf_disk_get_info failed!!", __FUNCTION__ );
	}

	// disk_smart monitoring 
//db value was changed from 0 to 1 and from 23 to 24.(0~23 -> 1~24)
//	_disk_mon_smart_interval = (nf_sysdb_get_uint("disk.smart_chk") + 1) * 3600 ;
	_disk_mon_smart_interval = (nf_sysdb_get_uint("disk.smart_chk")) * 3600 ;


	g_message("%s disk_mon_smart_interval[%d]",__FUNCTION__, _disk_mon_smart_interval );

	nf_event_timer_add( 1000, _event_disk_mon_timer_cb_func, NULL);

#endif 

#ifdef ENABLE_NET_LOGIN_FAIL_MON
	nf_event_timer_add( 1000, _event_net_login_fail_check_timer_cb_func, NULL);
#endif

	#if defined(_SNF_MODEL)
		nf_event_timer_add( 1000, _event_motion_notify_refire_cb_func, NULL);
	#endif

	nf_dva_event_init();
	nf_dva_object_detector_init();

	_nf_event->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );

	if( wait )
	{
		while( _nf_event->init_done != 1)
			g_usleep(10*1000);
	}
	
	#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_EVENT, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
	#endif
	
	_nf_event->init_done_main = TRUE;	
	return 1;
}

static void
_event_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_event != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_ALARM)
	{
		_nf_event->sysdb_reload = 1;
#ifdef ENABLE_DISK_SMART_MON
		_disk_mon_smart_interval = (nf_sysdb_get_uint("alarm.strg.smart_chk") + 1) * 3600 ;		
		++_disk_mon_sysdb_change;
#endif 	
	}
	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_USR)
		_nf_event_load_user_data();
	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_ACT)
	{
		_nf_event_load_event_system_sys_data();	
		_nf_event_load_event_system_net_data();
	}

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS)
	{
		_nf_event_load_system_magement_data();	
	}

	_nf_event->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );
}

static gboolean
_event_timer_cb_func(gpointer data)
{
	guint sensor_val_curr=0;
	#if defined(ENABLE_SENSOR_IPCAM)
		guint sensor_val_dvr=0, sensor_val_ipcam=0;
		guint sensor_val_old=0;
	#endif

	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_EVENT_TIMER] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_EVENT_TIMER] = TRUE;

	if(_nf_event->sysdb_reload)
	{
		#if 0		//  Moved to nf_event_reload_alarm_in func 
			_nf_event_load_event_sensor_data();
			_sensor_init();
		#endif	
			#ifndef _HDI_0412
				_nf_event_load_event_motion_data();		// use ipx
			#endif  /**/
			_motion_init();	
		_nf_event->sysdb_reload = 0;
	}

#ifdef ENABLE_WATCHDOG
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_EVENT );
	//g_message("%s nf_watchdog_kick!!!! EVENT", __FUNCTION__);	
#endif

	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_EVENT_TIMER);

	return TRUE;
}

void nf_event_reload_alarm_in(void)
{
	_nf_event_load_event_sensor_data();
	_sensor_init();
}

#if defined(ENABLE_SENSOR_IPCAM)
static gboolean
_event_regiter_ipcam_alarm_calback_timer_cb_func(gpointer data)
{
	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_ALARM] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_ALARM] = TRUE;

	if(nf_ipcam_set_alarm_callback((NFIPCamAlarmCallback *)&_event_sensor_in_ipcam_cb_func, NULL, NULL))
	{
		g_message("%s IPCAM Senosr CallBack CallBack Function Register Done!!!!", __FUNCTION__);
	
		nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_ALARM);
		_nf_event->wdt.is_timer_killed[NF_EVT_WATCHDOG_MEMBER_ALARM]=TRUE;
	
		return FALSE;		// Timer Reomve!!
	}

	g_message("%s line%d", __FUNCTION__, __LINE__);
	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_ALARM);

	return TRUE;
}
#endif

#if defined(_SNF_MODEL)
static gboolean
_event_motion_notify_refire_cb_func(void)
{
	guint mask_vloss=0, mask_motion=0, mask_tmp=0;
	gint ch=0;

	mask_vloss = nf_notify_get_param0("vloss");
	mask_motion = nf_notify_get_param0("motion");
	mask_tmp=mask_motion;
	
	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		if((mask_vloss >> ch) & 0x1)
		{
			if(((mask_motion >> ch) & 0x1))
				mask_motion &= ~(1 << ch);
		}
	}
	
	if(mask_tmp != mask_motion)
	{
		g_message("%s Line[%d] mask_motion[0x%08x] mask_tmp[0x%08x]", __FUNCTION__, __LINE__, mask_motion, mask_tmp);
		nf_notify_fire_params("motion", mask_motion, 0, 0, 0);
	}
	
	return TRUE;
}

static gboolean
_event_regiter_motion_calback_timer_cb_func(gpointer data)
{
	static gint cnt=0;

	if(nf_ipcam_set_motion_callback((NFIPCamMotionCallback *)&_event_motion_cb_func, NULL, NULL))
	{
		g_message("%s Motion CallBack CallBack Function Register Done!!!!", __FUNCTION__);
		return FALSE;		// Timer Reomve!!
	}

	if(cnt >=30)
	{
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_STRING, "MRTP PIPE CRATE FAIL!!");
		g_warning("%s MRTP PIPE CREATE FAIL!!!", __FUNCTION__);
		g_assert(0);

		return FALSE;
	}
	else
		cnt++;

	return TRUE;
}

/*****************************************************************/
/**
		@brief                      When IPCAM Vloss Status, To Remove Motion Flag
									Current This Function is not used!!
		@param[in]
		@return     void
*/
static void
_event_vloss_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NFIPCamMotionRaw mraw;
	gint ch=0;
	guint mask_vloss_curr=0;
	#if defined(ENABLE_SENSOR_IPCAM)
		guint alarm_curr_dvr=0,alarm_curr_ipcam=0, alarm_next=0;
	#endif

	mask_vloss_curr = (pinfo->d.params[0] & NUM_ACTIVE_CH_MASK);

	#if defined(ENABLE_SENSOR_IPCAM)
		alarm_curr_dvr = nf_notify_get_param0("sensor");
		alarm_curr_ipcam = nf_notify_get_param1("sensor");
		alarm_next = (~mask_vloss_curr) & (alarm_curr_ipcam & NUM_ACTIVE_CH_MASK);
		//alarm_next = ((~mask_vloss_curr & 0xffff) & (alarm_curr & 0x0000ffff));

	if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A){
		#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
			nf_notify_fire_params("sensor", alarm_curr_dvr, alarm_next, 0, 0);
		#elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
			nf_notify_fire_params("sensor", alarm_curr_dvr, alarm_next, 0, 0);
		#else
			nf_notify_fire_params("sensor", alarm_curr_dvr, alarm_next, 0, 0);
		#endif		
	} else {
		nf_notify_fire_params("sensor", alarm_curr_dvr, alarm_next, 0, 0);
			
		#if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
			; /*      */ 
		#endif
	}

	#endif

	#if 0	// This is Bug!!! 20120316
		mraw.ch=NUM_ACTIVE_CH+1;
	#else
		mraw.ch=NUM_ACTIVE_CH;					// To Remove Motion Flag
	#endif
	mraw.stream_num=(gint)mask_vloss_curr;		// Current Vloss CH
	
//	g_message("%s called stream_num[0x%08x]", __FUNCTION__, mask_vloss_curr);
	#if 0
		nf_notify_fire_pointer("mraw_data", &mraw, sizeof(NFIPCamMotionRaw));
	#endif
}

static 
void _event_motion_rdata_cb_func(NFIPCamMotionRaw *mraw_data)
{
	NFIPCamMotionRaw	mraw;
	MOTION_RAW_DATA		*mrdata;
	MOTION_DATA_EVENT	*mdata;
	MOTION_STATUS		*mstatus;
	gint sel_num=0, width=0, height=0, curr_ch=0, stream_num=0;
	guint time_start=0, time_end=0, curr_hour=0, timestamp=0;
	guint mblock=0, sensitivity=0, min_blocks=0;
	guchar *raw_data=NULL;
	guint interval=0;
	static guint mflag=0, mflag_old=0;
	guint mask_vloss=0;
	time_t		tick;
	struct tm	curr_tm;

	// To Remove Motion Flag
	mask_vloss = nf_notify_get_param0("vloss");
	mflag &= ~mask_vloss;

	#if 0
		memcpy(&mraw, pinfo->p.ptr, sizeof(NFIPCamMotionRaw));
	#else
		memcpy(&mraw, mraw_data, sizeof(NFIPCamMotionRaw));
	#endif

	width=mraw.width;
	height=mraw.height;
	curr_ch=mraw.ch;
	timestamp=mraw.timestamp;
	stream_num=mraw.stream_num;
	raw_data=mraw.mraw;

	mrdata=&_nf_event->motion_raw_data[curr_ch];
	mdata=&_nf_event->motion_data[curr_ch];
	mstatus=&_nf_event->motion_status[curr_ch];
	mblock=0;
	
	#if 0		// for test
		memset(mdata->area, '4', sizeof(mdata->area));
	#endif

	#if 0
		g_print("MOTION[ch(%d) stream(%d) width(%d) height(%d) time(%u.%u)]\n",
					mraw.ch, mraw.stream_num, mraw.width, mraw.height,
					mraw.timestamp, mraw.timestampl);
	#endif
	
	// Get Curr Hour
	tick = time(NULL);
	nf_datetime_localtime(&tick, _nf_event->is_dst, &curr_tm);
	curr_hour = (guint)curr_tm.tm_hour;
	time_start=mdata->time_start;
	time_end=mdata->time_end;
	interval=0;		// force 0!! Reason : ignore interval control action manager!!

	if(time_start < time_end)
	{
		if( (curr_hour>=time_start) && (curr_hour<time_end) )	// day time
		{
			sensitivity=mdata->sense_d;
			min_blocks=mdata->mini_d;
		}
		else		// night time
		{
			sensitivity=mdata->sense_n;
			min_blocks=mdata->mini_n;
		}
	}
	else
	{
		if( (curr_hour>=time_start) || ((curr_hour)<time_end) )	// day time
		{
			sensitivity=mdata->sense_d;
			min_blocks=mdata->mini_d;
		}
		else		// night time
		{
			sensitivity=mdata->sense_n;
			min_blocks=mdata->mini_n;
		}
	}
	
	if(curr_ch == NUM_ACTIVE_CH)		/** For Vloss --> To Remove Motion Flag **/
	{
		gint i=0;

		for(i=0; i<NUM_ACTIVE_CH; i++)
		{
			if(	(stream_num >> i) & 0x1 ) // if volss, remove motion flag
			{
				_nf_event->motion_status[i].is_on=NF_EVENT_MOTION_NOT_ACTIVE;
				mflag &= ~(1<<i);
			}
		}
		#if defined(EVENT_DEBUG_MOTION)
			g_message("%s LINE[%d] Motion Vloss~~~~~~~~~ [0x%08x] mflag_old[0x%08x] mflag[0x%08x] status[%d]", 
								__FUNCTION__, __LINE__, stream_num, mflag_old, mflag, mstatus->is_on);
		#endif
	}
	else
	{
		if(mdata->motion_act)
		{
			#if defined(EVENT_DEBUG_MOTION)
				g_message("%s LINE[%d] curr_ch[%d] status[%d] mflag_old[0x%08x] mflag[0x%08x]", 
								__FUNCTION__, __LINE__, curr_ch, mstatus->is_on, mflag_old, mflag);
			#endif

			if((width == 0) && (height == 0))		// Not Motion Event!!
			{
				mflag &= ~(1<<curr_ch);
				mstatus->is_on=NF_EVENT_MOTION_NOT_ACTIVE;
			}
			else if((width == 0) && (height != 0))	// Motion Event!!
			{
				mflag |= (1<<curr_ch);
				if(mstatus->is_on == NF_EVENT_MOTION_NOT_ACTIVE) 
				{
					mstatus->is_on=NF_EVENT_MOTION_RISE;
					mstatus->motion_active_sec=timestamp+interval;
				}
			}
			else if((width != 0) && (height != 0))
			{
				gint q=width*height;
				gint  level = (gint)(NF_EVENT_MOTION_SENSITIVITY_MAX - sensitivity); 
				gchar *praw = (gchar *)raw_data;
				gchar *psel = mdata->area;

				for(sel_num=0; sel_num<q; sel_num++)
				{
					if(*praw == -1)//NPT Motion exception case 
					{
						*praw = 10;
					}
					if( *praw  > level )
					{
						if( *psel  == '1')
							mblock++;

						if(mblock >= min_blocks)
							break;

					}
					++praw;
					++psel;
					#if 0
						if((sel_num != 0) && ((sel_num % 12) == 0))
							g_print("\n");
						g_print("%2d ", raw_data[sel_num]);
					#endif
				}
				#if 0
					g_print("\n\n");
				#endif
				
				if(mblock >= min_blocks)
				{
					mflag |= (1<<curr_ch);
					if(mstatus->is_on == NF_EVENT_MOTION_NOT_ACTIVE) 
					{
						mstatus->is_on=NF_EVENT_MOTION_RISE;
						mstatus->motion_active_sec=timestamp+interval;
						#if defined(EVENT_DEBUG_MOTION)
							g_message("%s LINE[%d] Motion Rise!! curr_ch[%d] mblock[%d] min_blocks[%d] "
										"timestamp[%d] timestamp_out[%d] interval[%d]\n",
											__FUNCTION__, __LINE__, curr_ch, mblock, min_blocks, 
											timestamp, mstatus->motion_active_sec, interval);
						#endif
					}
				}
				else
				{
					mflag &= ~(1<<curr_ch);
					mstatus->is_on=NF_EVENT_MOTION_NOT_ACTIVE;
				}
			}
			else		// Undefined!! Always Motion Off
			{
				mflag &= ~(1<<curr_ch);
				mstatus->is_on=NF_EVENT_MOTION_NOT_ACTIVE;
			}
		}
		else
		{
			mflag &= ~(1<<curr_ch);
			mstatus->is_on=NF_EVENT_MOTION_NOT_ACTIVE;
			#if defined(EVENT_DEBUG_MOTION)
				g_message("%s LINE[%d] Motion OFF!! ch[%d]\n", __FUNCTION__, __LINE__, curr_ch);
			#endif
		}
	}

	if(!mdata->motion_act)
	{
		mflag &= ~(1<<curr_ch);
		mstatus->is_on=NF_EVENT_MOTION_NOT_ACTIVE;
	}

	if((mstatus->is_on == NF_EVENT_MOTION_ACTIVE) || 
			(mstatus->is_on == NF_EVENT_MOTION_NOT_ACTIVE))
	{
		if(mflag_old == mflag)
		{
			#if defined(EVENT_DEBUG_MOTION)
				g_message("%s LINE[%d] Same Flag!! status[%d] mflag[0x%08x] mflag_old[0x%08x]\n",
								__FUNCTION__, __LINE__, mstatus->is_on, mflag, mflag_old);
			#endif
			return;
		}
	}

	if(mstatus->is_on == NF_EVENT_MOTION_RISE)
	{
		if(timestamp >= mstatus->motion_active_sec)
		{
			#if defined(EVENT_DEBUG_MOTION)
				g_message("%s LINE[%d] Motion Notify Fire!! ch[%d] mflag[0x%08x] mflag_old[0x%08x] timestamp[%d] timestamp_out[%d]\n", 
								__FUNCTION__, __LINE__, curr_ch, mflag, mflag_old, timestamp, mstatus->motion_active_sec);
			#endif
			nf_notify_fire_params("motion", mflag, 0, 0, 0);
			mstatus->is_on=NF_EVENT_MOTION_ACTIVE;
		}
	}
	else if(mstatus->is_on == NF_EVENT_MOTION_NOT_ACTIVE)		// off
	{
		#if defined(EVENT_DEBUG_MOTION)
			g_message("%s LINE[%d] Motion Notify Off!!! ch[%d], mflag[0x%08x]", __FUNCTION__, __LINE__, curr_ch, mflag);
		#endif
		nf_notify_fire_params("motion", mflag, 0, 0, 0);
	}

	mflag_old=mflag;
}

static void 
_event_motion_cb_func(NFIPCamMotionRaw* mraw, gpointer user_data)
{
	gint ch_md=0;

	NFIPCamMotionRaw mraw_data;

	memcpy(&mraw_data, mraw, sizeof(NFIPCamMotionRaw));

	_event_motion_rdata_cb_func(&mraw_data);

	ch_md=nf_ipcam_get_mraw_ch();

	if(ch_md == mraw->ch)
		nf_notify_fire_pointer("mraw_data", &mraw_data, sizeof(NFIPCamMotionRaw));
}
#endif

#ifdef ENABLE_DISK_SMART_MON

static gboolean
_event_disk_mon_timer_cb_func(gpointer data)
{
	static gint _disk_mon_cnt = DISK_SMART_INIT_SEC;	
	int i, j, ret, fail_cnt = 0;
	NF_SMART_DISK_INFO info;
	guint8 buf[128] = { 0, };
	
	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_DISK_MON] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_DISK_MON] = TRUE;
	
	// sysdb reload	
	if( _disk_mon_sysdb_change )
	{
		_disk_mon_cnt = (gint)_disk_mon_smart_interval;
		_disk_mon_sysdb_change = 0;	
	}

	//g_message("%s _disk_mon_cnt[%d] fs_is_online[%d]", __FUNCTION__, _disk_mon_cnt, nf_filesystem_is_online() );		
	
	// for preventing watchdog reset   #choissi 2011-12-28 ???? 11:37:33 
	if( nf_filesystem_is_online() == 0 )
	{
		nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_DISK_MON);
		return TRUE;		
	}

	//Internal disk
	if( --_disk_mon_cnt < 0 )
	{
		for(i=0;i<DISK_SMART_MAX_DISK_CNT;++i)
		{
			if ( _Disk_info.disk_size[0][i] || (_Disk_info.disk_state[0][i] & NF_DISK_INFO_FLAG_MIRROR))
			{
				memset(&info, 0, sizeof(NF_SMART_DISK_INFO));
				info.update_time.tv_sec = 120;	//120s

				ret = nf_smart_get_info(0, i, &info, NULL);
				g_message("%s SMART Internal disk[%d] ret[%d] status[%d]", __FUNCTION__, i, ret, info.disk_status );

				if ( ret == TRUE && info.disk_status == 0 )
				{
					sprintf((gchar *)buf, "(%d:%s:%lldGB):%d:%lld:%lld:%d:%d:%lld:%d:%d:%d",
							i, _Disk_info.model_num[0][i], _Disk_info.disk_size[0][i]/1024/1024,
							info.raw_read_error_rate.value,
							info.spin_up_time.raw,
							info.reallocated_sector_ct.raw,
							info.seek_error_rate.value,
							info.reallocation_event_ct.value,
							info.current_pending_sector.raw,
							info.offline_uncorrectable.value,
							info.start_stop_cnt,
							info.power_on_hours);
					nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
				}
				else
					fail_cnt++;
			}
		}

		//External disk
		for(i=0;i<DISK_SMART_MAX_DISK_CNT;++i)
		{
			if ( _Disk_info.disk_size[1][i] )
			{
				guint8 buf[128] = { 0, };
				memset(&info, 0, sizeof(NF_SMART_DISK_INFO));
				info.update_time.tv_sec = 120;	//120s

				ret = nf_smart_get_info(1, i, &info, NULL);

				if ( ret == TRUE && info.disk_status == 0 )
				{
					sprintf((gchar *)buf, "(%d:%s:%lldGB):%d:%lld:%lld:%d:%d:%lld:%d:%d:%d",
							i+16, _Disk_info.model_num[1][i], _Disk_info.disk_size[1][i]/1024/1024,
							info.raw_read_error_rate.value,
							info.spin_up_time.raw,
							info.reallocated_sector_ct.raw,
							info.seek_error_rate.value,
							info.reallocation_event_ct.value,
							info.current_pending_sector.raw,
							info.offline_uncorrectable.value,
							info.start_stop_cnt,
							info.power_on_hours);
					nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
				}
				else
					fail_cnt++;
			}
		}
		
		if( fail_cnt == 0 ) {
			g_message("%s SMART Check OK!!", __FUNCTION__);
		}

	
//		_disk_mon_cnt = DISK_SMART_MON_INTERVAL;
		if( _disk_mon_smart_interval >= 3600 )
			_disk_mon_cnt = (gint)_disk_mon_smart_interval;
		else
			_disk_mon_cnt = DISK_SMART_MON_INTERVAL;

	}else if( _disk_mon_cnt == (DISK_SMART_MON_INTERVAL-5) ) {
#if 0		
		if( nf_notify_get_param0("disk_smart") ) {
			nf_notify_fire_params( "disk_smart", 0,0,0,0 );
		}			
		//g_message("%s SMART Check Sleep!! [%d]", __FUNCTION__, _disk_mon_cnt);
#endif						
	}
			
	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_DISK_MON);

	return TRUE;
}

#endif		// ENABLE_DISK_SMART_MON

void nf_event_logon_fail_check(gchar *user_id, gboolean is_fail, gboolean is_dvr)
{
	gint user_cnt=0;
	SYSTEM_DDATA    *sddata=&_nf_event->system_ddata;
	NET_DDATA       *nddata=&_nf_event->net_ddata;
	USER_DATA       *udata=NULL;
 
#if 0
	g_message("%s called.. user[%s] is_fail[%d] is_dvr[%d]", __FUNCTION__, user_id, is_fail, is_dvr);
#endif

	for(user_cnt=0; user_cnt<NF_EVENT_USER_MAX; user_cnt++)
	{
		udata=&_nf_event->user_data[user_cnt];

		if(strncmp(udata->name, user_id, NF_EVENT_USER_NAME_MAX_LEN) == 0)
		{
			if(is_fail)
				goto nf_event_logoon_fail;
			else
				goto nf_event_logoon_success;
		}
	}

	udata=&_nf_event->user_data[user_cnt];     // Unknown User
	goto nf_event_logoon_fail;

nf_event_logoon_success:
	if(is_dvr)
	{
		udata->logOn_fail_cnt_dvr=0;
		nf_notify_fire_params("dvr_login_fail", 0x0, (guint)user_cnt, (guint)udata->logOn_fail_cnt_dvr, 0);
	}
	else
	{
		udata->logOn_fail_cnt_net=0;
		nf_notify_fire_params("net_login_fail", 0x0, (guint)user_cnt, (guint)udata->logOn_fail_cnt_net, 0);
	}

	return;

nf_event_logoon_fail:
	if(is_dvr)
	{
		udata->logOn_fail_cnt_dvr++;
		if(udata->logOn_fail_cnt_dvr >= sddata->logOn_fail_cnt)
		{
			nf_notify_fire_params("dvr_login_fail", 0x1, (guint)user_cnt, (guint)udata->logOn_fail_cnt_dvr, 0);
			udata->logOn_fail_cnt_dvr=0;
		}
	}
	else
	{
		udata->logOn_fail_cnt_net++;
		if(udata->logOn_fail_cnt_net >= nddata->logOn_fail_cnt)
		{
			nf_notify_fire_params("net_login_fail", 0x1, (guint)user_cnt, (guint)udata->logOn_fail_cnt_net, 0);
			udata->logOn_fail_cnt_net=0;
		}
	}

	return;
}

#if defined(ENABLE_FAN_FAIL_CHECK)
gboolean _is_fan_fail=0;			// for jbshell
gboolean _is_temper_fail=0;			// for jbshell

static gboolean
_event_fan_cb_func(gpointer data)
{
	static guint cnt=0;
	NF_UTIL_FAN_INFO fan_info;
	u_short fan_rpm[3] = {0, };
	u_int fan_fail_mask = 0;		// fan fail mask
	u_int fan_temp_fail_mask = 0;		// temperature fail mask
	gboolean is_temper_fail = FALSE, is_fan_fail = FALSE;
	guchar temper_cpu = 0, temper_sys = 0, thresh_cpu = 0, thresh_sys = 0;
	static glong cpu_fan_log = 0, sys_fan1_log = 0, sys_fan2_log = 0, cpu_temper_log = 0, sys_temper_log = 0, fail_noti_cnt = 0, temp_fail_noti_cnt = 0;
	GTimeVal tv;

	printf("[%s] start \n", __FUNCTION__);

	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_FAN] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_FAN] = TRUE;

	gettimeofday((struct timeval *)&tv, NULL);

	memset(&fan_info, 0x0, sizeof(NF_UTIL_FAN_INFO));

	if (nf_dev_board_pp_fan_get_info(&fan_info))
	{
		gint temper_cpu_real = 0;
#if 0
		g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
		g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
		g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
		g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
		g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
		g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
		g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
		g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
		g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);
#endif

		fan_rpm[0]=fan_info.speed_cpu_fan;
		fan_rpm[1]=fan_info.speed_sys_fan1;
		fan_rpm[2]=fan_info.speed_sys_fan2;
		temper_cpu=fan_info.temper_cpu;
		temper_sys=fan_info.temper_sys;
		thresh_cpu=fan_info.thresh_cpu;
		thresh_sys=fan_info.thresh_sys;

		#if 0
		temper_cpu_real = (gint)(((float)temper_cpu/(float)6.66666) + (float)temper_cpu);

			g_message("%s temper_cpu[%d]-->[%d] temper_sys[%d]", 
						__FUNCTION__, temper_cpu, temper_cpu_real, temper_sys);
		temper_cpu=(gchar)temper_cpu_real;
		#else
			temper_cpu_real=temper_cpu;
			// when 1U, cpu -50 is system.
			if (temper_cpu <= 50) {
				temper_sys = 0;
			} else {
				temper_sys=(temper_cpu - 50);
			}
			if((cnt % 50) == 0) {
				g_message("%s temper_cpu[%d]-->[%d] temper_sys[%d]",
								__FUNCTION__, temper_cpu, temper_cpu_real, temper_sys);
			}
		#endif	
	}
	else
	{
		nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_FAN);
		_nf_event->wdt.is_timer_killed[NF_EVT_WATCHDOG_MEMBER_FAN]=TRUE;

		return FALSE;
	}

	#if defined(ENABLE_SW_FAN_CTRL)
		_nf_event_sw_fan_ctrl(&fan_info);
	#endif

	/** Fan Fail Check **/
	#if defined(ENABLE_CPUFAN)
		if((fan_rpm[0] < NF_EVENT_FAN_FAIL_RPM_MIN) || (fan_rpm[0] > NF_EVENT_FAN_FAIL_RPM_MAX))
		{
			is_fan_fail = TRUE;
			fan_fail_mask |= (1 << 0);
		}
	#endif

	#if defined(ENABLE_SYSFAN1)
		if ((fan_rpm[1] < NF_EVENT_FAN_FAIL_RPM_MIN) || (fan_rpm[1] > NF_EVENT_FAN_FAIL_RPM_MAX))
		{
			is_fan_fail = TRUE;
			fan_fail_mask |= (1 << 1);
		}
	#endif
	
	#if defined(ENABLE_SYSFAN2)
		if((fan_rpm[2] < NF_EVENT_FAN_FAIL_RPM_MIN) || (fan_rpm[2] > NF_EVENT_FAN_FAIL_RPM_MAX))
		{
			is_fan_fail = TRUE;
			fan_fail_mask |= (1 << 2);
		}
		#endif
	
		/** Fan Temperature Fail Check **/
	if(temper_cpu >= NF_FAN_THREADHOLD_CPU)
	{
			is_temper_fail = TRUE;
			fan_temp_fail_mask |= (1 << 0);
	}

	if(temper_sys >= NF_FAN_THREADHOLD_SYS)
	{
			is_temper_fail = TRUE;
			fan_temp_fail_mask |= (1 << 1);
	}

#if 0
	g_message("rpm [%d][%d][%d]", (guint)fan_rpm[0], (guint)fan_rpm[1], (guint)fan_rpm[2]);	
#endif


	if (is_fan_fail || _is_fan_fail)
	{
		if (++fail_noti_cnt == 3) {

			nf_notify_fire_params("sys_fan", fan_fail_mask, (guint)fan_rpm[0], (guint)fan_rpm[1], (guint)fan_rpm[2]);
		
		#if defined(ENABLE_CPUFAN)
			if (cpu_fan_log <= 0)
			{
				#if 0
					g_message("%s Cpu Fan Event Log Put.. CNT[%ld]", __FUNCTION__, cpu_fan_log);
				#endif
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_CPU_FAN_FAIL, NULL);
				cpu_fan_log = 20 * 60 * 24;
			}
			else
				cpu_fan_log--;
		#endif

		#if defined(ENABLE_SYSFAN1)
			if (sys_fan1_log <= 0)
			{	
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_SYS_FAN_FAIL, NULL);
				sys_fan1_log = 20 * 60 * 24;
			}
			else
				sys_fan1_log--;
		#endif
		
		#if defined(ENABLE_SYSFAN2)
			if (sys_fan2_log <= 0)
			{	
				nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_SYS_FAN_FAIL, NULL);
				sys_fan2_log = 20 * 60 * 24;
			}
			else
				sys_fan2_log--;
		#endif
		
		fail_noti_cnt = 0;
		}
	}
	else
	{
		nf_notify_fire_params("sys_fan", fan_fail_mask, (guint)fan_rpm[0], (guint)fan_rpm[1], (guint)fan_rpm[2]);

		#if defined(ENABLE_CPUFAN)
			cpu_fan_log=0;
		#endif
		#if defined(ENABLE_SYSFAN1)
			sys_fan1_log=0;
		#endif
		#if defined(ENABLE_SYSFAN2)
			sys_fan2_log=0;
		#endif
		
		fail_noti_cnt = 0;
	}

	#if 0
	// For JBShell Test!!
	if(_is_temper_fail)
		fan_temp_fail_mask=(1<<0) | (1<<1);
	else
		fan_temp_fail_mask=0x0;
	#endif

	if (is_temper_fail || _is_temper_fail)
	{
		if (++temp_fail_noti_cnt == 3) {
			nf_notify_fire_params("sys_temperature", fan_temp_fail_mask, (guint)temper_cpu, (guint)temper_sys, 0);

			g_message("%s Temperature Fail!! mask[0x%08x]", __FUNCTION__, fan_temp_fail_mask);
			if (fan_temp_fail_mask & 0x1)
			{
				if (cpu_temper_log <= 0)
				{
					g_message("%s Log Put CPU Temperature Fail!!", __FUNCTION__);
					nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_CPU_TEMP_FAIL, NULL);
					cpu_temper_log = 20 * 60 * 24;
				}
				else
					cpu_temper_log--;
			}

			if ((fan_temp_fail_mask >> 1) & 0x1)
			{
				if (sys_temper_log <= 0)
				{
					g_message("%s Log Put System Temperature Fail!!", __FUNCTION__);
					nf_eventlog_put_param(&tv, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_SYS_TEMP_FAIL, NULL);
					sys_temper_log = 20 * 60 * 24;
				}
				else
					sys_temper_log--;
			}
			temp_fail_noti_cnt = 0;
		}
	}
	else
	{
		nf_notify_fire_params("sys_temperature", fan_temp_fail_mask, (guint)temper_cpu, (guint)temper_sys, 0);

		cpu_temper_log = 0;
		sys_temper_log = 0;
		temp_fail_noti_cnt = 0;
	}

	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_FAN);

	return TRUE;
}

#if defined(ENABLE_SW_FAN_CTRL)
/**
	0xff -> fan_rpm_sys1 2520 fan_rpm_sys2 2400                              
	0xe0 -> fan_rpm_sys1 2400 fan_rpm_sys2 2280                              
	0xc0 -> fan_rpm_sys1 2340 fan_rpm_sys2 2280                              
	0xb0 -> fan_rpm_sys1 2280 fan_rpm_sys2 2220                              
	0xa0 -> fan_rpm_sys1 2220 fan_rpm_sys2 2220                              
	0x90 -> fan_rpm_sys1 2220 fan_rpm_sys2 2100                              
	0x80 -> fan_rpm_sys1 2160 fan_rpm_sys2 2100                              
	0x70 -> fan_rpm_sys1 2100 fan_rpm_sys2 2040                              
	0x60 -> fan_rpm_sys1 1980 fan_rpm_sys2 1920                              
	0x50 -> fan_rpm_sys1 1860 fan_rpm_sys2 1800                              
	0x40 -> fan_rpm_sys1 1740 fan_rpm_sys2 1620                              
	0x30 -> fan_rpm_sys1 1440 fan_rpm_sys2 1320                              
	0x20 -> fan_rpm_sys1 1140 fan_rpm_sys2 1080                              
	0x10 -> fan_rpm_sys1 1380 fan_rpm_sys2 1380                              
	0x0 -> fan_rpm_sys1 0 fan_rpm_sys2 0 -> fan_rpm_sys1 840 fan_rpm_sys2 780

	range : 0x50 ~ 0xb0 (from hw)

	init state : 0x80 --> In Uboot
**/
static guchar _nf_event_sw_fan_tbl[11] =
{
	0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x70, 0x60, 0x50
};

static void _nf_event_sw_fan_ctrl(NF_UTIL_FAN_INFO *fan_info)
{
	static guchar fan_duty_cpu = 0, fan_duty_sys = 0x80, fan_duty_sys_old = 0;
	u_short fan_rpm_cpu = 0, fan_rpm_sys1 = 0, fan_rpm_sys2 = 0;
	gchar temper_cpu = 0, temper_sys = 0, thresh_cpu = 0, thresh_sys = 0;

	fan_rpm_cpu = fan_info->speed_cpu_fan;
	fan_rpm_sys1 = fan_info->speed_sys_fan1;
	fan_rpm_sys2 = fan_info->speed_sys_fan2;
	temper_cpu = fan_info->temper_cpu;
	temper_sys = fan_info->temper_sys;
	thresh_cpu = fan_info->thresh_cpu;
	thresh_sys = fan_info->thresh_sys;

	#if 1
		g_message("%s line %d [fan_rpm_sys1 %d fan_rpm_sys2 %d temper_sys %d fan_duty_sys 0x%x] ", 
					__FUNCTION__, __LINE__, fan_rpm_sys1, fan_rpm_sys2, temper_sys, fan_duty_sys); 
	#endif

	#if defined(ENABLE_CPUFAN)
		nf_dev_board_pp_fan_set_duty(TRUE, fan_duty_cpu);
	#endif

	#if defined(ENABLE_SYSFAN1) || defined(ENABLE_SYSFAN2)
		if (temper_sys > NF_EVENT_FAN_SW_CNTL_TEMPER_MIDDLE)
		{
			if (fan_rpm_sys1 < NF_EVENT_FAN_SW_CNTL_RPM_MAX)
			{
				if (fan_duty_sys <= 0x78)
					fan_duty_sys += 0x8;	
			}
		}
		else if (temper_sys < NF_EVENT_FAN_SW_CNTL_TEMPER_MIDDLE)
		{
			if (fan_rpm_sys1 > NF_EVENT_FAN_SW_CNTL_RPM_MIN)
			{
				if (fan_duty_sys >= 0x48)
					fan_duty_sys -= 0x8;	
			}
		}
		else 
		{
			return ;
		}

		if (fan_duty_sys != fan_duty_sys_old)
		{
			nf_dev_board_pp_fan_set_duty(FALSE, fan_duty_sys);
			fan_duty_sys_old = fan_duty_sys;
		}
	#endif

	return ;
}
#endif

static char nf_event_fan_fail_jbshell_cmd_help[] = "fan_fail temper [1 or 0] : 1->fail 0->normal\n"
													"         fan    [1 or 0] : 1->fail 0->normal\n";
static int nf_event_fan_fail_jbshell_cmd(int argc, char **argv)
{
	gboolean is_fail=0;

	if(argc < 2)
		goto nf_event_fan_fail_help_cmd;

	if(strcmp(argv[1], "temper") == 0)
	{
		is_fail= (gboolean)strtoul(argv[2], NULL, 10);
		if(is_fail)
			_is_temper_fail=TRUE;
		else
			_is_temper_fail=FALSE;
	}
	else if(strcmp(argv[1], "fan") == 0)
	{
		is_fail= (gboolean)strtoul(argv[2], NULL, 10);
		if(is_fail)
			_is_fan_fail=TRUE;
		else
			_is_fan_fail=FALSE;
	}
	else if(strcmp(argv[1], "fan_set") == 0)
	{
		NF_UTIL_FAN_INFO fan_info;
		guchar fan_duty_sys=0;

		fan_duty_sys= (gboolean)strtoul(argv[2], NULL, 16);

		nf_dev_board_pp_fan_set_duty(FALSE, fan_duty_sys);

		memset(&fan_info, 0x0, sizeof(NF_UTIL_FAN_INFO));
		if(nf_dev_board_pp_fan_get_info(&fan_info))
		{
			g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
			g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
			g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
			g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
			g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
			g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
			g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
			g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
			g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);
		}
	}
	#if 0
		else if(strcmp(argv[1], "test") == 0)
		{
			_temper_sys=(gchar)strtoul(argv[2], NULL, 10);

			g_message("%s line%d _temper_sys %d\n", __FUNCTION__, __LINE__, _temper_sys);
		}
	#endif
	else
		goto nf_event_fan_fail_help_cmd;
	
	return 0;

nf_event_fan_fail_help_cmd:
	printf("Invalid arguments\n%s\n", nf_event_fan_fail_jbshell_cmd_help);
	return -1;
}

__commandlist(nf_event_fan_fail_jbshell_cmd, "fan_fail", nf_event_fan_fail_jbshell_cmd_help, nf_event_fan_fail_jbshell_cmd_help);
#endif

#if defined(ENABLE_POE_CHECK)

static NF_UTIL_POE_PORT_INFO _g_port_info;
static int _g_port_info_init = 1;
static int _g_hub_port_info_init = 1;
static int _g_port_info_cur_port = 0;
int hub_discovery[NUM_ACTIVE_CH] = {0,}; // 1: hub port connect, 0: hub port not connect or nvr port connect
int hub_ch_mW[NUM_ACTIVE_CH] = {0,};

#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
 || defined(_IPX_1648M4E)// || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
void _event_set_hub_port_info(int ch, NF_UTIL_POE_INFO info)
#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
|| defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4)
{
	int num_ch = ch + 8;
	int i;

	if (_g_hub_port_info_init) {
		for (i = NUM_ACTIVE_CH_HUB; i < NUM_ACTIVE_CH; i++)
			memset(&_g_port_info.info[i], 0x00, sizeof(NF_UTIL_POE_INFO));

		_g_hub_port_info_init = 0;
	}
	_g_port_info.info[num_ch].is_discovery	= info.is_discovery;
	_g_port_info.info[num_ch].is_active		= info.is_active;
	_g_port_info.info[num_ch].port_class	= info.port_class;
	_g_port_info.info[num_ch].func_status	= info.func_status;
	_g_port_info.info[num_ch].consumption	= info.consumption;
	_g_port_info.info[num_ch].voltage		= info.voltage;
	_g_port_info.info[num_ch].current_mA	= info.current_mA;

	if (!_g_port_info_init) {
		nf_notify_fire_params("sys_poe_port", (guint)num_ch,
					(guint)_g_port_info.info[num_ch].func_status,
					_g_port_info.info[num_ch].voltage,
					(guint)_g_port_info.info[num_ch].consumption);
	}
}
#elif defined(_IPX_1648P4E) || defined(_IPX_1648M4E)// || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)//SWMPFOURE-240
{
	int num_ch = ch + 8;

	if(_g_port_info.info[num_ch].is_discovery != 0) { //nvr port is connected
		hub_discovery[num_ch] = 0;
	} else if(info.is_discovery == 0) { //nvr disconnect, hub disconnect (both disconnect)
		hub_discovery[num_ch] = 0;
	} else { hub_discovery[num_ch] = 1; } //hub is connected

	if(hub_discovery[num_ch]) { //only hub port connect, hub port consumption notify
		//debounce SWMPFOURE-176
		if(hub_ch_mW[num_ch]){
			if(hub_ch_mW[num_ch] < info.consumption) {
				if((hub_ch_mW[num_ch] * 2) < info.consumption) {
					info.consumption = hub_ch_mW[num_ch] + ((info.consumption - hub_ch_mW[num_ch])/20);
				} else  {
					info.consumption = hub_ch_mW[num_ch] + ((info.consumption - hub_ch_mW[num_ch])/10);
				}
			} else if(hub_ch_mW[num_ch] > info.consumption) {
				if(hub_ch_mW[num_ch] > (info.consumption * 2)) {
					info.consumption = hub_ch_mW[num_ch] - ((hub_ch_mW[num_ch] - info.consumption)/20);
				} else {
					info.consumption = hub_ch_mW[num_ch] - ((hub_ch_mW[num_ch] - info.consumption)/10);
				}
			}
		}
		printf("\033[0;36m[%s]notify consumption hub_port[%d] = %d\033[0;39m\n", __FUNCTION__,num_ch,info.consumption);
		nf_notify_fire_params("sys_poe_port", (guint)num_ch,(guint)info.func_status,info.voltage,(guint)info.consumption);
	}
	hub_ch_mW[num_ch] = info.consumption;
}
#endif

#endif

void nf_event_get_zig_info(NF_UTIL_POE_PORT_INFO* info)
{
	memcpy(info, &_g_port_info, sizeof(NF_UTIL_POE_PORT_INFO));
}

static gboolean
_event_poe_cb_func(gpointer data)
{
	guint poe_active_mask=0, poe_warn_mask=0, poe_power_cut_mask=0, poe_error_mask=0;
	gint num_ch=0, thresh=0;
	static gint poe_error_cnt=0, poe_chip_error_cnt=0, poe_first_link[8] = {0, };
	guint tot_mW=0, limit=0, thresh_result=0;
#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
 || defined(_IPX_1648M4E)
	guint tot_mW_hub=0, limit_hub=0, thresh_result_hub=0;
	gint port = (_g_port_info_cur_port++ % NUM_ACTIVE_CH_DVR);
#elif defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	gint port = (_g_port_info_cur_port++ % NUM_ACTIVE_CH_DVR); //Undecided scenario, temporary 16 port
#else
	gint port = (_g_port_info_cur_port++ % NUM_ACTIVE_CH);
#endif
	static glong log_poe_fail=0;
	static gulong poe_power_over_check_cnt=0;
	SYSTEM_DDATA    *sddata=&_nf_event->system_ddata;
	NF_UTIL_POE_CHIP_INFO info;

	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_POE] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_POE] = TRUE;

	memset(&info, 0x0, sizeof(NF_UTIL_POE_CHIP_INFO));
	nf_dev_poe_get_chip_infomation(&info);

	#if 0
		g_message("%s Chip0 -> Id[0x%02x] is_connected[%d]", __FUNCTION__, info.id[0], info.is_connected[0]);
		g_message("%s Chip1 -> Id[0x%02x] is_connected[%d]", __FUNCTION__, info.id[1], info.is_connected[1]);
	#endif

#if defined(_IPX_0824) || defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
	if(!info.is_connected[0] || !info.is_connected[1])
#elif defined(_IPX_1648M4) 
	if(!info.is_connected[0]) 
#else
	if(!info.is_connected[0])
#endif
	{
		printf("\e[33m[%s] POE Chip ID Error ID 0x%x 0x%x\e[0m\n", __FUNCTION__, info.id[0], info.id[1]);

		poe_error_mask=0xffff;
		poe_warn_mask=0xffff;
	   	poe_power_cut_mask=0xffff;
	   	tot_mW=0;

		#if 0		/// Block
			if(poe_chip_error_cnt == (NF_EVENT_POE_DEBOUNCE_CHIP_ERROR_CNT / 2))
			{
				#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4E) || defined(_IPX_0824M4E)	// POE Force Reset
					nf_dev_board_pp_poe_force_reset();
				#endif
			}
		#endif

		if(poe_chip_error_cnt > NF_EVENT_POE_DEBOUNCE_CHIP_ERROR_CNT)
		{
			goto nf_event_poe_notify;
		}
		else
		{
			poe_chip_error_cnt++;
			nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_POE);
			return TRUE;
		}
	}
	else
		poe_chip_error_cnt=0;

	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
	 || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)//Undecided scenario, 32ch temporary no hub
		limit = sddata->poe_limit;
		limit_hub = sddata->poe_limit_hub;
	#else
		limit = sddata->poe_limit;
	#endif
	thresh = sddata->poe_fail_threshold;
	
	if( _g_port_info_init )
	{
		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
		 || defined(_IPX_1648M4E)// || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)//Undecided scenario, 32ch temporary no hub
			if (_g_hub_port_info_init) {
				for (num_ch = NUM_ACTIVE_CH_HUB; num_ch < NUM_ACTIVE_CH; num_ch++)
					memset(&_g_port_info.info[num_ch], 0x00, sizeof(NF_UTIL_POE_INFO));

				_g_hub_port_info_init = 0;
			}
		#endif

		if(!nf_dev_poe_get_info(&_g_port_info)) {
			g_warning("%s nf_dev_poe_get_info fail!!", __FUNCTION__ );
			nf_notify_fire_params("sys_poe_status", 0xffff, 0xffff, 0xffff, 0);
			poe_error_mask=0xffff;

			goto nf_event_poe_fail_log;
		}

		#if defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		for(num_ch=0; num_ch<NUM_ACTIVE_CH/2; num_ch++) //Undecided scenario, temporary 16 port
		#else
		for(num_ch=0; num_ch<NUM_ACTIVE_CH; num_ch++)
		#endif
		{
			g_message("%s CH[%-2d] discovery[%d] act[%d]class[%d]func[%d] [%5d]mw [%02d]V[%03d]mA",
							__FUNCTION__, num_ch, 
							_g_port_info.info[num_ch].is_discovery, 
							_g_port_info.info[num_ch].is_active, 
							_g_port_info.info[num_ch].port_class,
							_g_port_info.info[num_ch].func_status, 
							_g_port_info.info[num_ch].consumption, 
							_g_port_info.info[num_ch].voltage,
							_g_port_info.info[num_ch].current_mA);

			#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			#else
			if (num_ch < 8) 
			#endif
			{
				if (_g_port_info.info[num_ch].consumption != 0) {
					_g_port_info.info[num_ch].consumption = 2500;
				} else {
					poe_first_link[num_ch] = 1;
				}
			}
		}

		_g_port_info_init = 0;
		
	} else {

		guint temp_consumption = 0;
		if (poe_first_link[port] == 0) {
			temp_consumption = _g_port_info.info[port].consumption;
		}

		if(!nf_dev_poe_get_info_single( port, &_g_port_info.info[port]) ) {
			g_warning("%s nf_dev_poe_get_info_single fail!! port[%d]", __FUNCTION__, port);
			nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_POE);
			return 1;
		}

		/*
			sys_poe_port
					d.param[0]	port
					d.param[1]	status
					d.param[2]	voltage (V)	
					d.param[3]	power (mW)
		*/

		if (poe_first_link[port] == 1) {
			if (_g_port_info.info[port].consumption != 0) {
				_g_port_info.info[port].consumption = 2500;
				poe_first_link[port] = 0;
			}
		} else {
			if (_g_port_info.info[port].consumption == 0) {
				poe_first_link[port] = 1;
				if(!nf_dev_poe_get_info_single(port, &_g_port_info.info[port])) {
					g_warning("%s nf_dev_poe_get_info_single fail!! port[%d]", __FUNCTION__, port);
					nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_POE);
					return 1;
				}
				if(_g_port_info.info[port].consumption != 0) {
					printf("\033[0;36mpoe 0_bounce, reset\033[0;39m\n");
					_g_port_info.info[port].consumption = 2500;
					poe_first_link[port] = 0;
				}
			} else {
				printf("\e[33m [%s] temp_consumption = %d _g_port_info.info[%d].consumption = %d\e[0m\n", 
						__FUNCTION__, temp_consumption, port, _g_port_info.info[port].consumption);

				if (temp_consumption < _g_port_info.info[port].consumption) {
					if ((temp_consumption * 2) < _g_port_info.info[port].consumption) {
						_g_port_info.info[port].consumption = temp_consumption + (((_g_port_info.info[port].consumption - temp_consumption) / 20));
					} else {
						_g_port_info.info[port].consumption = temp_consumption + (((_g_port_info.info[port].consumption - temp_consumption) / 10));
					}
				} else if (temp_consumption > _g_port_info.info[port].consumption) {
					if (temp_consumption > (_g_port_info.info[port].consumption * 2)) {
						_g_port_info.info[port].consumption = temp_consumption - (((temp_consumption - _g_port_info.info[port].consumption) / 20));
					} else {
						_g_port_info.info[port].consumption = temp_consumption - (((temp_consumption - _g_port_info.info[port].consumption) / 10));
					}
				}
			}
		}

		if(hub_discovery[port]==0) //nvr port is connect or hub port is not connect = hub_discovery[port] = 0
		{
			printf("\033[0;36m[%s]notify consumption port[%d] = %d\033[0;39m\n", __FUNCTION__,port,_g_port_info.info[port].consumption);

			nf_notify_fire_params("sys_poe_port", (guint)port,
					(guint)_g_port_info.info[port].func_status,
					_g_port_info.info[port].voltage,
					(guint)_g_port_info.info[port].consumption );
		}
	}
	#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	for(num_ch=0; num_ch<NUM_ACTIVE_CH/2; num_ch++) //Undecided scenario, temporary 16 port
	#else
	for(num_ch=0; num_ch<NUM_ACTIVE_CH; num_ch++)
	#endif
	{
		if(_g_port_info.info[num_ch].func_status == 3) {
			poe_power_cut_mask |= (1 << num_ch);
			poe_error_mask |= 0x1;
			printf("[%s] _g_port_info.info[%d].func_status == %d \n",__FUNCTION__,num_ch,_g_port_info.info[num_ch].func_status);
		}
		
		// W per Port is 15
		if(_g_port_info.info[num_ch].consumption > 
					(gint)(NF_EVENT_POE_WA_PER_PORT * ((float)thresh / (float)100)))
		{
			poe_warn_mask |= (1 << num_ch);
			poe_error_mask |= (1 << 3);
			g_warning("%s Poe Port%d Power Consumption Fail!! thresh[%d] consumption [%d] -> [%d]", 
					__FUNCTION__, num_ch, thresh, (gint)(NF_EVENT_POE_WA_PER_PORT * ((float)thresh / (float)100)),
					_g_port_info.info[num_ch].consumption);
		}

		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
		 || defined(_IPX_1648M4E)
			if (num_ch < 8){
				tot_mW += _g_port_info.info[num_ch].consumption;
				#if defined(DEBUG_EVENT_POE_ERROR)
					printf("\033[0;33m %s [%d] port consumption -> [%d]\033[0;39m\n", __FUNCTION__,num_ch,_g_port_info.info[num_ch].consumption);
				#endif	
			}
			else
			{	
				#if defined(DEBUG_EVENT_POE_ERROR)
					printf("\033[0;33m %s [%d] port consumption -> [%d]\033[0;39m\n", __FUNCTION__,num_ch,_g_port_info.info[num_ch].consumption);
				#endif
				
				#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E)
					if(hub_discovery[num_ch])
						tot_mW_hub += hub_ch_mW[num_ch];
					tot_mW += _g_port_info.info[num_ch].consumption;
				#else
					tot_mW_hub += _g_port_info.info[num_ch].consumption;
				#endif
			}
		#else
			tot_mW += (guint)_g_port_info.info[num_ch].consumption;	//Undecided scenario, 32ch routine here
		#endif
	}
	
	if(((float)thresh / (float)100) == (float)0)		// devide by 0 check!!
		g_warning("%s invalid threshold value!! threshold[%d]", __FUNCTION__, thresh);
	else
	{
		// limit is 8CH --> 40W & 16CH --> 72W	
		thresh_result=(guint)((float)limit * ((float)thresh / (float)100));
		if( (tot_mW/1000) >  thresh_result ) {
			#if defined(DEBUG_EVENT_POE_ERROR)
			printf("\033[0;33m %s  [DEBUG_EVENT_POE_ERROR] tot_mW[%d] thresh_result_hub : [%d] \033[0;39m\n", __FUNCTION__,tot_mW, thresh_result_hub);
			#endif
			poe_error_mask |= 0x2;
		}
		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
		 || defined(_IPX_1648M4E)
			thresh_result_hub=(guint)((float)limit_hub * ((float)thresh / (float)100));
			if( (tot_mW_hub/1000) >  thresh_result_hub ) {
				#if defined(DEBUG_EVENT_POE_ERROR)
				printf("\033[0;33m %s  [DEBUG_EVENT_POE_ERROR] tot_mW_hub[%d], thresh_result_hub:[%d] \033[0;39m\n", __FUNCTION__,tot_mW_hub, thresh_result_hub);
				#endif
				poe_error_mask |= 0x2;
			}
		#endif
	}

		g_message("%s poe_error_mask[0x%08x] poe_warn_mask[0x%08x]", 
				__FUNCTION__, poe_error_mask, poe_warn_mask);

nf_event_poe_notify:

	if(poe_error_mask != 0x0)
		poe_error_cnt++;
	else
		poe_error_cnt = 0;

	if((poe_error_cnt >= 4) || (poe_error_cnt == 0))		// Check During 4sec!! --> Timer is 1sec..
	{
		nf_notify_fire_params("sys_poe_status", poe_error_mask, poe_warn_mask, poe_power_cut_mask, tot_mW);
		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4)
			nf_notify_fire_params("sys_poe_status_hub", poe_error_mask, poe_warn_mask, poe_power_cut_mask, tot_mW_hub);
		#endif

		if(poe_error_cnt >=4)
		{
			g_print("\n===============================================================================================\n");
			g_warning("Poe Event Occur!!");
			#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
			 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
			 || defined(_IPX_1648M4E)// || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
				g_print("poe_error_mask[0x%08x] poe_warn_mask[0x%08x] poe_power_cut_mask[0x%08x]\n", 
				     	poe_error_mask, poe_warn_mask, poe_power_cut_mask);
				g_print("limit[%d] limit_hub[%d] thresh[%d] thresh_result[%d] thresh_result_hub[%d] tot_mW[%d] tot_mW_hub[%d]\n", 
							limit, limit_hub, thresh, thresh_result, thresh_result_hub, (tot_mW/1000), (tot_mW_hub/1000));
			#else
				g_print("poe_error_mask[0x%08x] poe_warn_mask[0x%08x] poe_power_cut_mask[0x%08x]\n", 
								poe_error_mask, poe_warn_mask, poe_power_cut_mask);
				g_print("limit[%d] thresh[%d] thresh_result[%d] tot_mW[%d]\n", 
							limit, thresh, thresh_result, (tot_mW/1000));
			#endif
			g_print("===============================================================================================\n");
		}
	}

nf_event_poe_fail_log:

	if((poe_error_mask == 0x1) || (poe_error_mask == 0x2) 
				|| ((poe_error_mask >> 3) & 0x1) || (poe_error_mask == 0xffff))
	{
		if(log_poe_fail <= 0)
		{
			nf_eventlog_put_param(NULL, LT_SYSTEM_EVENT, 0, LP2_SYSTEM_EVENT_POE_FAIL, NULL);

			g_message("%s Line[%d] Log Put Poe Error!!", __FUNCTION__, __LINE__);

			log_poe_fail=6*60*24;
		}
		else
			log_poe_fail--;
	}
	else
		log_poe_fail=0;

	if(_nf_event->is_enable_poe_mon)
	{
		if( port == 0 )
		{
			#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			for(num_ch=0; num_ch<NUM_ACTIVE_CH/2; num_ch++) //Undecided scenario, temporary 16 port
			#else
			for(num_ch=0; num_ch<NUM_ACTIVE_CH; num_ch++)
			#endif
			{
				char buff[256], sysdb_cam[128];
	
				snprintf(sysdb_cam, sizeof(sysdb_cam), "cam.C%d.title", num_ch);
				snprintf(buff, sizeof(buff), "%d-%dmW", num_ch, _g_port_info.info[num_ch].consumption);

				nf_sysdb_set_str(sysdb_cam, buff);
			}
			nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);
		}
	}

	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_POE);

	return TRUE;
}

#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
static void nf_event_check_poe_power_over(guint tot_mW, guint tot_mW_hub)
#else
static void nf_event_check_poe_power_over(guint tot_mW)
#endif
{
//	printf("[%s] check_check In tot_mW : [%d] , tot_mW_hub : [%d] \n", __FUNCTION__, tot_mW, tot_mW_hub);
	gint is_fail=FALSE, port=0, num_ch=0;
	static POE_DISABLE_INFO info;
	static gboolean is_init=TRUE;
	guint limit=0;
	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
	 || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		guint limit_hub=0;
	#endif
	GTimeVal			curr_timeval;
	SYSTEM_DDATA   		*sddata=&_nf_event->system_ddata;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	if(is_init)
	{
		memset(&info, 0x0, sizeof(POE_DISABLE_INFO));
		is_init=FALSE;
	}

	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		limit = sddata->poe_limit;
		limit_hub = sddata->poe_limit_hub;
	#else
		limit = sddata->poe_limit;
	#endif

	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
	 || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		g_message("%s Line[%d] limit[%d] limit_hub[%d] tot_mW[%d] tot_mW_hub[%d]", 
						__FUNCTION__, __LINE__, limit, limit_hub, tot_mW, tot_mW_hub);
	#else
		g_message("%s Line[%d] limit[%d] tot_mW[%d]", __FUNCTION__, __LINE__, limit, tot_mW);
	#endif

	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
	 || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		num_ch=NUM_ACTIVE_CH_DVR;
	#else
		num_ch=NUM_ACTIVE_CH;
	#endif

	if(tot_mW > limit)
	{
		for(port=(num_ch - 1); port>=0; port--)
		{
			if(info.port[port] == FALSE)
				break;
		}

		if(port >= 0)
		{
			nf_dev_poe_port_onoff(port, FALSE, &is_fail);

			if(!is_fail)
			{
				info.port[port]=TRUE;
				info.time_enable[port]=curr_timeval.tv_sec + NF_EVENT_POE_POWER_OVER_PORT_ENABLE_TIME;
				g_message("%s Line[%d] Dvr Port[%d] Disble Enable_Time[%ld]", __FUNCTION__, __LINE__, port, info.time_enable[port]);
			}
			else
				info.port[port]=FALSE;
		}
	}
	else
	{
		for(port=0; port<num_ch; port++)
		{
			if(info.port[port] == TRUE)
				break;
		}

		if(port < num_ch)
		{
			if(info.time_enable[port] <= curr_timeval.tv_sec)
			{
				g_message("%s Line[%d] Dvr Port[%d] Enable", __FUNCTION__, __LINE__, port);
					nf_dev_poe_port_onoff(port, TRUE, &is_fail);

				if(!is_fail)
				{
					info.port[port]=FALSE;

					if((port + 1) < num_ch)		// For Next Port Enable
						info.time_enable[port+1]=curr_timeval.tv_sec + NF_EVENT_POE_POWER_OVER_PORT_ENABLE_TIME;
				}
				else
					info.port[port]=TRUE;
			}
		}
	}

	#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
	 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
	 || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		if(tot_mW_hub > limit_hub)
		{
			for(port=(NUM_ACTIVE_CH - 1); port>=NUM_ACTIVE_CH_HUB; port--)
			{
				if(info.port[port] == FALSE)
					break;
			}

			if(port >= 0)
			{
				if(!is_fail)
				{
					info.port[port]=TRUE;
					info.time_enable[port]=curr_timeval.tv_sec + NF_EVENT_POE_POWER_OVER_PORT_ENABLE_TIME;
					g_message("%s Line[%d] Hub Port[%d] Disble Enable_Time[%ld]", 
									__FUNCTION__, __LINE__, port, info.time_enable[port]);
				}
				else
					info.port[port]=FALSE;
			}
		}
		else
		{
			for(port=NUM_ACTIVE_CH_HUB; port<NUM_ACTIVE_CH; port++)
			{
				if(info.port[port] == TRUE)
					break;
			}

			if(port < NUM_ACTIVE_CH)
			{
				if(info.time_enable[port] <= curr_timeval.tv_sec)
				{
					g_message("%s Line[%d] Hub Port[%d] Enable", __FUNCTION__, __LINE__, port);
					if(!is_fail)
					{
						info.port[port]=FALSE;

						if((port + 1) < NUM_ACTIVE_CH)		// For Next Port Enable
							info.time_enable[port+1]=curr_timeval.tv_sec + NF_EVENT_POE_POWER_OVER_PORT_ENABLE_TIME;
					}
					else
						info.port[port]=TRUE;
				}
			}
		}
	#endif
}

void nf_event_enable_poe_mon_debug(gboolean is_enable)
{
	NF_OBJECT_LOCK( _nf_event );

	_nf_event->is_enable_poe_mon=is_enable;

	NF_OBJECT_UNLOCK( _nf_event );
}
#endif

static gboolean
_event_regiter_xload_check_timer_cb_func(gpointer data)
{
	static gint cnt=0;
	gint pid=0;
	gchar *process_name=NULL;

	process_name=(gchar *)data;
	nf_event_check_ps(process_name, &pid);

	if(pid <= 0)
		cnt++;	
	else
	{
		g_message("%s %s Register Done!!!!", __FUNCTION__, process_name);
		return FALSE;       // Timer Reomve!!
	}
	
	if(cnt >=1)
	{
		gchar str[128]={0, };
		
		sprintf(str, "%s Load Fail!!", process_name);

		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_STRING, str);
		g_warning("%s %s Load FAIL!!!", __FUNCTION__, process_name);
		// g_assert(0);
	}

	return TRUE;
}

void nf_event_check_ps(gchar *ps_name, gint *pid_num)
{
	printf("chech_check %s \n", __FUNCTION__); 
	FILE *fp;
	DIR *dir;
	struct dirent *entry;
	struct stat fstat;
	gint pid=0;
	gchar cmdLine[256]={0, }, tempPath[256]={0, };

	dir = opendir("/proc");

	while ((entry = readdir(dir)) != NULL) {
		snprintf(tempPath, sizeof(tempPath), "/proc/%s", entry->d_name);

		lstat(tempPath, &fstat);

	  if (!S_ISDIR(fstat.st_mode))
			 continue;

		pid = atoi(entry->d_name);
		if (pid <= 0) 
			continue;

		memset(tempPath, 0x0, sizeof(tempPath));
		sprintf(tempPath, "/proc/%d/cmdline", pid);

		fp = fopen(tempPath, "r");
		if(fp == NULL)
			continue;

		memset(cmdLine, 0, sizeof(cmdLine));
		fgets(cmdLine, 256, fp);
		fclose(fp);
		#if 0
			g_print("[%d] %s\n", pid, cmdLine);
		#endif
		if(strcmp(cmdLine, ps_name) == 0)
			*pid_num=pid;
	}

	closedir(dir);
}

static char nf_event_ps_check_jbshell_cmd_help[] = "ps_check [process_name]\n";
static int nf_event_ps_check_jbshell_cmd(int argc, char **argv)
{
	gint pid=0;

	if(argc < 2)
		goto nf_event_ps_check_help_cmd;
    
	nf_event_check_ps(argv[1], &pid);
	
	if(pid < 0)
		g_message("%s Invaild Porcess Name!!!", __FUNCTION__);
	else
		g_message("%s process_name-->%s pid-->%d", __FUNCTION__, argv[1], pid);

    return 0;

nf_event_ps_check_help_cmd:
    printf("Invalid arguments\n%s\n", nf_event_ps_check_jbshell_cmd_help);

    return -1;
}   

__commandlist(nf_event_ps_check_jbshell_cmd, "ps_check", nf_event_ps_check_jbshell_cmd_help, nf_event_ps_check_jbshell_cmd_help);


#ifdef ENABLE_NET_LOGIN_FAIL_MON

#define FILE_WEB_SERVER_ERROR_LOG	"/tmp/webra/e401.log"
#define ERROR_CLEAR_CNT				15
#define ERROR_LOG_STRING			"http_auth.c"
#define ERROR_USERID				"ADMIN"

/**
 * @brief Function to check the number of failed remote login attempts for WEB2.0
 * 
 * @param userid User ID that attempted to login
 * @param result Result of login attempt
 * 0 : fail, 1 : success
 * @param reason Reason of login failure
 * 0 : TBD, 1 : TBD ...
 * @return int 
 */

int nf_event_net_login_fail_check_cb(char *userid, int result, int reason)
{
	static int fail_cnt[NF_EVENT_USER_MAX] = {0,};
	int user_idx = 0;
	char *strUser = NULL;
	char key[256];
	
	g_message("[%s, %d] called id : %s, result : %d, reason : %d", __FUNCTION__, __LINE__, userid, result, reason);

#if 0	
	for (user_idx = 0; user_idx < NF_EVENT_USER_MAX; user_idx++)
	{
		memset(key, 0x00, sizeof(key));
		g_sprintf(key, "usr.U%d.name", user_idx);
		strUser = nf_sysdb_get_str_nocopy(key);
		if (strcmp(strUser, userid) == 0) break;
	}
	
	if (user_idx == NF_EVENT_USER_MAX)
	{
		//To do
		//check fail count each user
	}
#endif
	if (result)
		nf_event_logon_fail_check(ERROR_USERID, 0, 0);
	else
		nf_event_logon_fail_check(ERROR_USERID, 1, 0);
		
	return 0;
}

static gboolean
_event_net_login_fail_check_timer_cb_func(gpointer data)
{
	static guint error_cnt = 0, ok_cnt = 0;
		
	gchar *contents=NULL;
	gsize  length = 0;
	GError *error = NULL;
	gint	ret = 0;
	
	if(_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_NET_LOGIN] == FALSE)
		_nf_event->wdt.is_start[NF_EVT_WATCHDOG_MEMBER_NET_LOGIN] = TRUE;

	if (!g_file_get_contents ( FILE_WEB_SERVER_ERROR_LOG , &contents, &length, &error))
	{
		g_warning("%s %s\n", __FUNCTION__, error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		
		truncate(FILE_WEB_SERVER_ERROR_LOG,0);

		nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_NET_LOGIN);

		return 1;
	}

	if(	length )
		truncate(FILE_WEB_SERVER_ERROR_LOG,0);

	if(contents)
	{		
		char *is_error_str = NULL;		
		//g_message("%s size[%d]", __FUNCTION__, length);				
		
		if( length ) {
			is_error_str = strstr(contents, ERROR_LOG_STRING);			
		}
						
		if( is_error_str) 
		{
			++error_cnt;
			ok_cnt = 0;
			nf_event_logon_fail_check(ERROR_USERID, 1, 0 ); // FAIL, NETWORK (REMOTE)
		}else{
								
			if( ++ok_cnt == ERROR_CLEAR_CNT){
				nf_event_logon_fail_check(ERROR_USERID, 0, 0 ); // SUCCESS, NETWORK (REMOTE) 
			}
		}
		
		g_free(contents);
	}

	nf_event_watchdog_kick(NF_EVT_WATCHDOG_MEMBER_NET_LOGIN);

	return 1;
}

#endif 

void _event_set_virtual_alarm_mask(guint64 mask)
{
	guint64 v_alarm=0;
	
	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)		// A Type
	{
		#if defined(_IPX_32M4E) || defined(_IPX_32P5)
			v_alarm = 0xffffffffff & mask;
	   #elif defined(_IPX_1648M4) || defined(_IPX_1648M4E)
		v_alarm = 0xffffffff & mask;
	   #elif defined(_IPX_0824M4) || defined(_IPX_0824M4E) || defined(_IPX_0824P4E)
		v_alarm = 0xffffff & mask;
          #elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		v_alarm = 0xffffffff & mask;	   
	   #else
		v_alarm = 0xfffff & mask;
	   #endif
	}
	else{ 				// B Type
		v_alarm = 0x3ffff & mask;
		
   	   #if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		; /*      */ 
	   #endif			
	}		
      
	
	_nf_event->virtual_alarm_mask = v_alarm;
	
	printf("@@@@ virtual alarm set[%llu] @@@@\n", _nf_event->virtual_alarm_mask);
}
guint64 _event_get_virtual_alarm_mask()
{
	return _nf_event->virtual_alarm_mask;
}
guint64 _event_get_physical_alarm_mask()
{
	return _nf_event->physical_alarm_mask;
}

static void _event_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint64 alarm_curr=0;
	guint64 v_mask=0;
	guint64 mask_dvr=0;
	guint64 mask_ipcam=0;
	int i = 0;
	guint is_notify = 0;
	
	int ain_dvr=0;
	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)
	{
	   #if defined(_IPX_1648M4) || defined(_IPX_1648M4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
		ain_dvr = 16;
	   #elif defined(_IPX_0824M4) || defined(_IPX_0824M4E) || defined(_IPX_0824P4E)
		ain_dvr = 8; 
	   #elif defined(_IPX_1648P4E)  || defined(_IPX_32P4E)
		ain_dvr = 16; 
	   #else
		ain_dvr = 4;
	   #endif
	}
	else {
		ain_dvr = 2;
		
	   #if defined(_IPX_1648M4) 
		; /*       */ 
	   #endif		
	}

	alarm_curr = _nf_event->physical_alarm_mask;
	v_mask = _nf_event->virtual_alarm_mask;

	for ( i = 0; i < NUM_ALARM; i++ )
	{
		if ( (v_mask & (1LL << i)) && !(alarm_curr & (1LL << i)))
		{
			is_notify = 1;
		}
	}
/*
	for(i = 0; i < ain_dvr ;i++)
			{
		if ( (v_mask & (0x01 << (i + 16))) && !(alarm_curr & (0x01 << (i + 16))))
		{
			is_notify = 1;
			}
	}
	*/

	if (pinfo->d.params[2] == 2 )
		is_notify = 0;

	if ( is_notify )
	{
		for ( i = 0; i < NUM_ALARM_IPCAM; i++ )
		{
			if ( (v_mask & (1LL << i)) || (alarm_curr & (1LL << i)) )
				mask_ipcam |= 1LL << i;
		}
		for(i = 0; i < ain_dvr ;i++)
		{
			if ( (v_mask & (1LL << (i + NUM_ALARM_IPCAM))) || (alarm_curr & (1LL << (i + NUM_ALARM_IPCAM))) )
				mask_dvr |= 1LL << i;
		}
		//printf("\e[31m [%s][%d] renotify[%x]\e[0m\n", __FUNCTION__, __LINE__, mask);

		if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)		// A Type
		{
			#if defined(_IPX_32M4E) || defined(_IPX_32P5)
				nf_notify_fire_params("sensor", (mask_dvr & 0xffff), (mask_ipcam & 0xffffffff), 2, 0);		// virutal alarm phyical alarm check param1
		    #elif defined(_IPX_1648M4) || defined(_IPX_1648M4E)
			nf_notify_fire_params("sensor", (mask_dvr & 0xffffffff), mask_ipcam, 2, 0);		// virutal alarm phyical alarm check param1
		    #elif defined(_IPX_0824M4) || defined(_IPX_0824M4E) 
			nf_notify_fire_params("sensor", (mask_dvr & 0xffffff), mask_ipcam, 2, 0);		// virutal alarm phyical alarm check param1
		    #elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
			nf_notify_fire_params("sensor", (mask_dvr & 0xffffffff), mask_ipcam, 2, 0);		// virutal alarm phyical alarm check param1			
		    #else
			nf_notify_fire_params("sensor", (mask_dvr & 0xfffff), mask_ipcam, 2, 0);		// virutal alarm phyical alarm check param1
		    #endif
		}
		else {				// B Type
		   nf_notify_fire_params("sensor", (mask_dvr & 0x3ffff), mask_ipcam, 2, 0);		// virutal alarm phyical alarm check param1

		   #if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		   	 ; /*        */ 
		   #endif
		}
	}
}

/*
	event_driver_open( self );
	event_connect_gst( self );
	event_connect_ipcamera( self );
*/

/*
typedef enum
{
  G_IO_IN		GLIB_SYSDEF_POLLIN,
  G_IO_OUT		GLIB_SYSDEF_POLLOUT,
  G_IO_PRI		GLIB_SYSDEF_POLLPRI,
  G_IO_ERR		GLIB_SYSDEF_POLLERR,
  G_IO_HUP		GLIB_SYSDEF_POLLHUP,
  G_IO_NVAL		GLIB_SYSDEF_POLLNVAL
} GIOCondition;

-- fd?? io_channe
GIOChannel* g_io_channel_unix_new  (int fd)

g_io_create_watch ()

GSource*    g_io_create_watch               (GIOChannel *channel,
                                             GIOCondition condition);
-- read/write
GIOError    g_io_channel_write              (GIOChannel *channel,
                                             const gchar *buf,
                                             gsize count,
                                             gsize *bytes_written);

GIOError    g_io_channel_read               (GIOChannel *channel,
                                             gchar *buf,
                                             gsize count,
                                             gsize *bytes_read);

typedef enum
{
  G_IO_ERROR_NONE,
  G_IO_ERROR_AGAIN,
  G_IO_ERROR_INVAL,
  G_IO_ERROR_UNKNOWN
} GIOError;

Creates a GSource that's dispatched when condition is met for the given channel.
For example, if condition is G_IO_IN, the source will be dispatched when there's
data available for reading. g_io_add_watch() is a simpler interface to this same 
functionality, for the case where you want to add the source to the default 
main loop at the default priority. 



mknod /dev/sensor	c 242 0
mknod /dev/relay	c 243 0
mknod /dev/keypad	c 244 0
mknod /dev/jog		c 245 0
mknod /dev/shuttle	c 246 0
mknod /dev/remocon	c 247 0

device ????; ??
GIO-channel

kernel driver???? poll interface?? ?????Ǿ? ?ִ?. ioctl?? f?? ?ϴ? ?͵? ??=.

http://www.ezdoum.com/stories.php?story=07/10/15/4799999
*/

gboolean 
nf_event_init_qc(int wait)
{
	gboolean ret=TRUE;
	guint tmp=0, cb_handle=0;
	
	gint fd, dspid;
	
	g_return_val_if_fail (_nf_event == NULL, FALSE);	
	
	_nf_event = g_object_new ( NF_TYPE_EVENT , NULL);
	
//	nf_event_timer_add( 1000, event_timeout_cb, _nf_event);
//	nf_event_timer_add( 2000, event_testvec_cb, _nf_event);

	//_motion_init();
	_nf_event_load_event_sensor_data();
	_nf_event_load_event_system_sys_data();
	_nf_event_load_event_system_net_data();
	_nf_event_load_user_data();
	_nf_event_load_system_magement_data();

	
#ifndef _HDI_0412
	//_nf_event_load_event_motion_data();		// use ipx
#endif  /**/

	fd = nf_dev_open_sensor();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_sensor Failed!! ret[%d]", __FUNCTION__, fd);
    	//g_assert(0);
    }else{
    	g_message("%s  nf_dev_open_sensor fd[%d]", __FUNCTION__,  fd);
		nf_dev_sensor_all_enable(1);

	//	_sensor_init();
		#if defined(ENABLE_ARI_PANIC)
	//	_sensor_init_ari_panic();
		#endif

 	//	cb_handle = nf_event_src_add(fd , (GSourceFunc)gio_pannel_in , "sensor");
		g_message("%s nf_event_src_add[sensor] cb_handle[%d]", 
					__FUNCTION__, cb_handle);
		//nf_notify_fire_params("sensor", nf_dev_sensor_get(),0,0,0);
	}

#ifdef DEBUG_TEST_SENSOR
	nf_dev_sensor_ch_onoff(0,1);
	nf_dev_sensor_ch_onoff(1,1);
	nf_dev_sensor_ch_onoff(2,1);
	nf_dev_sensor_ch_onoff(3,1);
#endif

//	nf_dev_open_relay();
//	nf_dev_relay_set(0,1);
#ifdef DEBUG_TEST_RELAY 
    // move to nf_action.c
	nf_dev_open_relay();
	nf_dev_relay_set(0,1);
	nf_dev_relay_set(3,1);
	nf_dev_relay_set(5,1);
	nf_dev_relay_set(7,1);
#endif

#ifdef ENABLE_HNF_IPCAM
	{
		int i;
		guint ipcam_max_cnt = nf_sysdb_get_uint ("ipcam.CCNT");
		guint active_ch_cnt = NUM_ACTIVE_CH - ipcam_max_cnt;

		_nf_event->vloss_mask = 0;

		for(i=0;i<active_ch_cnt;++i)
		{
			_nf_event->vloss_mask |= (1<<i);
		}
		g_message("%s vloss_mask [0x%08x]", __FUNCTION__, _nf_event->vloss_mask);
	}
#else
	_nf_event->vloss_mask = NUM_ACTIVE_CH_MASK;
#endif

#if defined(_SNF_MODEL)
	//cb_handle= nf_notify_connect_cb( "vloss", _event_vloss_cb_func, NULL);
	//g_message("%s vloss connect_cb[%d]",__FUNCTION__, cb_handle );
	//g_assert(cb_handle >0);

	nf_event_timer_add( 2000, _event_regiter_motion_calback_timer_cb_func, NULL);
#elif defined(_HDI_0412)
	#if defined(USE_DEV_GENNUM)
		fd = nf_dev_open_gennum();
		if(fd < 0)
		{
			g_warning("%s nf_dev_open_gennum Failed!! ret[%d]", __FUNCTION__, fd);
			//g_assert(0);
		}else{
			guint curr_vloss=0, novid_mask=0;

			g_message("%s  nf_dev_open_gennum fd[%d]", __FUNCTION__,  fd);
			nf_dev_gennum_rx_set_vloss(_nf_event->vloss_mask);		// vloss on/off status

			nf_dev_gennum_get_vloss(&curr_vloss);
			g_message("%s curr_vloss[0x%08x]", __FUNCTION__, curr_vloss);

			_nf_event->novid_mask |= ~(curr_vloss);
			 novid_mask=(~(_nf_event->novid_mask) & NUM_ACTIVE_CH_MASK);

			_nf_event_invalid_video_check();

			_nf_event->invalid_vid_mask &= (~(curr_vloss & NUM_ACTIVE_CH_MASK));
			#if 0
				g_message("%s Vloss[0x%08x]Invalid[0x%08x] Novid[0x%08x]", 
								__FUNCTION__, curr_vloss, _nf_event->invalid_vid_mask, novid_mask);
			#endif

			curr_vloss |= _nf_event->invalid_vid_mask;
			nf_notify_fire_params("vloss", curr_vloss, novid_mask, _nf_event->invalid_vid_mask, 0);

			cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_vloss_in , "vloss");
			g_message("%s nf_event_src_add[vloss] cb_handle[%d]", 
						__FUNCTION__, cb_handle);
		}

		fd = nf_dev_open_gennum_minor1();
		if(fd < 0)
		{
			g_warning("%s nf_dev_open_gennum_minor1 Failed!! ret[%d]", __FUNCTION__, fd);
			//g_assert(0);
		}else{

			g_message("%s  nf_dev_open_gennum_minor1 fd[%d]", __FUNCTION__,  fd);

			cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_std_type_in , "std_type");
			g_message("%s nf_event_src_add[std_type] cb_handle[%d]", 
						__FUNCTION__, cb_handle);
		}
	#endif
#else
#if defined(USE_DEV_TW2864)
	fd = nf_dev_open_tw2864();
	if(fd < 0)
	{
		g_warning("%s nf_dev_open_tw2864 Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
	}else{

		guint curr_vloss = 0;    
		guint ipcam_vloss = nf_notify_get_param1("vloss");

		g_message("%s  nf_dev_open_tw2864 fd[%d]", __FUNCTION__,  fd);

		nf_dev_tw2864_init(1);

		curr_vloss = nf_dev_tw2864_get_vloss_status()&_nf_event->vloss_mask;

		nf_notify_fire_params("vloss",  (curr_vloss)|(ipcam_vloss<<BASE_IPCAM_CHANNEL),
								ipcam_vloss, 0, 0);

		cb_handle = nf_event_src_add(fd, (GSourceFunc)gio_vloss_in , "vloss");
		g_message("%s nf_event_src_add[vloss] cb_handle[%d]", 
					__FUNCTION__, cb_handle);

		nf_dev_tw2864_set_dac(NF_TW2864_DAC_PLAYBACK);

	#ifdef DEBUG_TEST_TW2864
		nf_dev_tw2864_set_picture(0,50,50,50,50);
		nf_dev_tw2864_set_ntsc_pal(0);
		nf_dev_tw2864_get_signal_type();
	#endif
#endif
#endif

#ifdef DEBUG_EVENT_KEYPAD
	fd = nf_dev_open_keypad();
	nf_dev_keypad_dev_enable();
	
	g_message("%s  nf_device_open_keypad fd[%d]", __FUNCTION__,  fd);
	
	cb_handle = nf_event_src_add(fd , gio_keypad_in ,"keypad");
	g_message("%s nf_event_src_add keypad cb_handle[%d]", __FUNCTION__, cb_handle);
#endif

#ifdef ENABLE_DSP_MOTION
	for(dspid=0; dspid<NUM_ACTIVE_DSP; ++dspid)
	{
		fd = nf_dspcomm_open_chan(dspid, NF_DSPCOMM_CH_MDEVT);
	    if(fd<0)
	    {
	    	 g_warning("%s nf_dspcomm_open_chan Failed!! ret[%d]", __FUNCTION__, fd);
	    	 //g_assert(0);
	    }else{

			g_message("%s  nf_dspcomm_open_chan fd[%d]", __FUNCTION__,  fd);

	//	   	cb_handle = nf_event_src_add(fd, gio_dsp_motion_in , (gpointer)dspid);
	//		g_message("%s nf_event_src_add[dsp_motion](%d) cb_handle[%d]", 
			//			__FUNCTION__, dspid, cb_handle);
		}
	}
#elif defined(_ANF_1648) || defined(_ATM_1624)
	fd = nf_dev_open_fs1648();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_fs1648 Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
    }else{

    	g_message("%s  nf_dev_open_fs1648 fd[%d]", __FUNCTION__,  fd);

    //	cb_handle = nf_event_src_add(fd, gio_motion_in , "motion");
    //	g_message("%s nf_event_src_add[motion] cb_handle[%d]",
    				__FUNCTION__, cb_handle);
    }

#elif defined(_HDI_0412)

#elif defined(_OTM_MODEL)
	fd = nf_dev_open_solo_vin();
    if(fd < 0)
    {
    	g_warning("%s nf_dev_open_solo_vin Failed!! ret[%d]", __FUNCTION__, fd);
		//g_assert(0);
    }else{

    	g_message("%s  nf_dev_open_solo_vin fd[%d]", __FUNCTION__,  fd);

    //	cb_handle = nf_event_src_add(fd, gio_motion_in , "motion");
    //	g_message("%s nf_event_src_add[motion] cb_handle[%d]",
    				__FUNCTION__, cb_handle);
    }
#elif defined(_SNF_MODEL_XXX)
	//g_timeout_add(200, tw2880_motion_timer, "motion");
#endif

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _event_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
	//register timer callback function
	nf_event_timer_add( 1000, _event_timer_cb_func, NULL);
	
	#if defined(ENABLE_SENSOR_IPCAM)
	//	nf_event_timer_add( 2000, _event_regiter_ipcam_alarm_calback_timer_cb_func, NULL);
	#endif

	// 20110422	
	#if defined(ENABLE_FAN_FAIL_CHECK)
//		nf_event_timer_add( (1000 * 10), _event_fan_cb_func, NULL);
		nf_event_timer_add( (1000 * 3), _event_fan_cb_func, NULL);
	#endif

	#if defined(ENABLE_POE_CHECK)
		//nf_event_timer_add( (1000), _event_poe_cb_func, NULL);
	//	nf_timer_add( (5000), _event_poe_cb_func, NULL); // for normal priority thread
	#endif
	
	// 20111101
	//nf_event_timer_add( 2000, _event_regiter_xload_check_timer_cb_func, "Xfbdev");

#ifdef ENABLE_DISK_SMART_MON

	memset( &_Disk_info, 0x00, sizeof(NF_DISK_INFO) );
	
	if(!nf_disk_get_info(&_Disk_info, NULL)) {
		g_warning("%s  nf_disk_get_info failed!!", __FUNCTION__ );
	}

	// disk_smart monitoring 
//db value was changed from 0 to 1 and from 23 to 24.(0~23 -> 1~24)
//	_disk_mon_smart_interval = (nf_sysdb_get_uint("disk.smart_chk") + 1) * 3600 ;
	_disk_mon_smart_interval = (nf_sysdb_get_uint("disk.smart_chk")) * 3600 ;


	g_message("%s disk_mon_smart_interval[%d]",__FUNCTION__, _disk_mon_smart_interval );

	nf_event_timer_add( 1000, _event_disk_mon_timer_cb_func, NULL);

#endif 

#ifdef ENABLE_NET_LOGIN_FAIL_MON
	nf_event_timer_add( 1000, _event_net_login_fail_check_timer_cb_func, NULL);
#endif

	#if defined(_SNF_MODEL)
		nf_event_timer_add( 1000, _event_motion_notify_refire_cb_func, NULL);
	#endif

	_nf_event->is_dst = nf_sysdb_get_bool( "sys.date.daylight" );

	if( wait )
	{
		while( _nf_event->init_done != 1)
			g_usleep(10*1000);
	}
	
	#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_EVENT, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
	#endif
	
	return 1;
}
