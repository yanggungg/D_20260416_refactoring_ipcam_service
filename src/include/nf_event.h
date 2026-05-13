#ifndef __NF_EVENT_H__
#define __NF_EVENT_H__

#include "nf_object.h"
#include "nf_api_ipcam.h"

#define NF_EVENT_WATCHDOG_TIME_SEC            10

typedef enum _NF_EVENT_WATCHDOG_MEMBER_E {
	NF_EVT_WATCHDOG_MEMBER_EVENT_TIMER  = 0,
	NF_EVT_WATCHDOG_MEMBER_MOTION       = 1,
	NF_EVT_WATCHDOG_MEMBER_DISK_MON     = 2,
	NF_EVT_WATCHDOG_MEMBER_DISK_TEMP    = 3,
	NF_EVT_WATCHDOG_MEMBER_DISK_MBID    = 4,
	NF_EVT_WATCHDOG_MEMBER_NET_LOGIN    = 5,
	NF_EVT_WATCHDOG_MEMBER_FAN          = 6,
	NF_EVT_WATCHDOG_MEMBER_ALARM        = 7,
	NF_EVT_WATCHDOG_MEMBER_POE			= 8,

	NF_EVENT_WATCHDOG_MEMBER_NR

} NF_EVENT_WATCHDOG_MEMBER_E;

typedef enum _NF_EVENT_TYPE_E
{
	NF_EVENT_TYPE_NOPEN = 0,
	NF_EVENT_TYPE_NCLOSE = 1
} NF_EVENT_TYPE_E;

#define NF_SENSOR_ON						1
#define NF_SENSOR_OFF						0

#define NF_SENSOR_TYPE_NC					NF_EVENT_TYPE_NCLOSE
#define NF_SENSOR_TYPE_NO					NF_EVENT_TYPE_NOPEN

#if defined(ENABLE_FAN_FAIL_CHECK)
	#define NF_FAN_THREADHOLD_CPU			100 //95 ->100
	
	#if defined(_HDI_0412)
		#define NF_FAN_THREADHOLD_SYS			50
	#else
		#define NF_FAN_THREADHOLD_SYS			50		// 60->50
	#endif

	#if defined(ENABLE_SW_FAN_CTRL)
		#if defined(_IPX_0412VE3) || defined(_IPX_0824VE3) || defined(_IPX_1648VE3) \
		 || defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) \
		 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
			#define NF_EVENT_FAN_SW_CNTL_RPM_MIN		2000	
			#define NF_EVENT_FAN_SW_CNTL_RPM_MAX		5000
			#define NF_EVENT_FAN_FAIL_RPM_MIN			1500	
			#define NF_EVENT_FAN_FAIL_RPM_MAX			6000	
		#else
			#define NF_EVENT_FAN_SW_CNTL_RPM_MIN		1800
			#define NF_EVENT_FAN_SW_CNTL_RPM_MAX		2250
			#define NF_EVENT_FAN_FAIL_RPM_MIN			1400	
			#define NF_EVENT_FAN_FAIL_RPM_MAX			2700	
		#endif
		
		#define NF_EVENT_FAN_SW_CNTL_RPM			50
		#define NF_EVENT_FAN_SW_CNTL_TEMPER_MIDDLE	50
	#else
		#define NF_EVENT_FAN_FAIL_RPM_MIN			300	
		#define NF_EVENT_FAN_FAIL_RPM_MAX			6000
	#endif
#endif

#define NF_MOTION_CELL_MAX_NUM				(513)
#define NF_MOTION_CELL_MIN_NUM				(396)

#if defined(ENABLE_POE_CHECK)
	#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_0824P4E) || defined(_IPX_32P5)
		#define NF_EVENT_POE_WA_PER_PORT		((float)30000)	//AT
	#else
		#define NF_EVENT_POE_WA_PER_PORT		((float)15000)	//AF
	#endif
	#define NF_EVENT_POE_POWER_OVER_PORT_ENABLE_TIME			30
	#define NF_EVENT_POE_DEBOUNCE_CHIP_ERROR_CNT			5
#endif

/* type macro */
#define NF_TYPE_EVENT						(nf_event_get_type ())

#define NF_IS_EVENT(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_EVENT))
#define NF_IS_EVENT_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_EVENT))

#define NF_EVENT_GET_CLASS(obj)				(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_EVENT, NfEventClass))
#define NF_EVENT(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_EVENT, NfEvent))
#define NF_EVENT_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_EVENT, NfEventClass))

#define NF_EVENT_CAST(obj)					((NfEvent*)(obj))
#define NF_EVENT_CLASS_CAST(klass)			((NfEventClass*)(klass))

#define NF_EVENT_MOTION_SENSITIVITY_MAX		10

#define NF_EVENT_USER_UNKNOWN				(1)
#if defined(TAKENAKA)
#define NF_EVENT_USER_MAX					(15)
#else
#define NF_EVENT_USER_MAX					(8)
#endif
#define NF_EVENT_USER_NAME_MAX_LEN		(64)

typedef struct _NfEvent 		NfEvent;
typedef struct _NfEventClass 	NfEventClass;

#if defined(_HDI_0412)
typedef struct _NF_EVENT_VIDEO_STD_T
{
	guint	in_type;
	guchar	val;
}__attribute__ ((packed)) NF_EVENT_VIDEO_STD;

	#define NF_EVENT_VIDEO_IN_TYPE_MAX_NR		14

#endif

typedef struct _SENSOR_DATA_T {
#if 0
	gboolean		sensor_act;
	gchar			*desc;
#endif
	gboolean		op_type;
} __attribute__((packed))SENSOR_DATA;

#if 0
	struct _NFIPCamMotionRaw
	{
		gint ch;
		gint stream_num;
		gint width;
		gint height;
		guint timestamp;
		guint timestampl;
		guchar mraw[256];
	};
#endif

typedef enum _NF_EVENT_MOTION_EVENT_E
{
	NF_EVENT_MOTION_NOT_ACTIVE	= 0,
	NF_EVENT_MOTION_RISE		= 1,
	NF_EVENT_MOTION_ACTIVE		= 2

} NF_EVENT_MOTION_EVENT_E;

typedef struct _MOTION_DATA_EVENT_T {
	gboolean		motion_act;
	guint			detect;
	guint			sense_d;
	guint			sense_n;
	guint			mini_d;
	guint			mini_n;
	gchar			area[NF_MOTION_CELL_MIN_NUM];
	guint			time_start;
	guint			time_end;
} __attribute__((packed))MOTION_DATA_EVENT;

typedef struct _MOTION_STATUS_T {
	guint			motion_active_sec;
	guint			is_on;
} __attribute__((packed))MOTION_STATUS;

typedef struct _MOTION_RAW_DATA_T {
	NFIPCamMotionRaw	mraw;
} __attribute__((packed))MOTION_RAW_DATA;

typedef struct _SYSTEM_DDATA_T {
	gint				logOn_fail_cnt;
	#if defined(ENABLE_POE_CHECK)
		gint				poe_fail_threshold;
		guint				poe_limit;
		#if defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
			|| defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
			|| defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
			guint				poe_limit_hub;
		#endif
	#endif
} __attribute__((packed))SYSTEM_DDATA;

typedef struct _NET_DDATA_T {
	gint				logOn_fail_cnt;
} __attribute__((packed))NET_DDATA;

typedef struct _USER_DATA_T {
	gchar				name[NF_EVENT_USER_NAME_MAX_LEN];
	gint				logOn_fail_cnt_dvr;
	gint				logOn_fail_cnt_net;
} __attribute__((packed))USER_DATA;

#if defined(ENABLE_POE_CHECK)
typedef struct _POE_DISABLE_INFO_T
{
	gboolean			port[NUM_ACTIVE_CH];
	glong				time_enable[NUM_ACTIVE_CH];

} POE_DISABLE_INFO;
#endif

typedef struct _NF_EVENT_WDT_DATA_T {

	guint           enable_cp[NF_EVENT_WATCHDOG_MEMBER_NR];
	guint           max_cp[NF_EVENT_WATCHDOG_MEMBER_NR];
	guint           cur_cp[NF_EVENT_WATCHDOG_MEMBER_NR];
	guint           is_start[NF_EVENT_WATCHDOG_MEMBER_NR];
	guint           is_timer_killed[NF_EVENT_WATCHDOG_MEMBER_NR];
	GTimeVal        last_kick_tv[NF_EVENT_WATCHDOG_MEMBER_NR];

} __attribute__((packed)) NF_EVENT_WDT_DATA;

/**
 * NfEvent:
 *
 * NfDVR notify class
 */
struct _NfEvent {
	NfObject			object;

	/*< public >*/
	gint				init_done;
	gboolean            init_done_main;

	GAsyncQueue			*queue;

	GThread				*thread;
	GThread				*netmon_thread;
	GThread             *thread_watchdog;
	
	gint				thread_run;
	gint				thread_run_watchdog;
	gint				thread_status;

	GMainContext		*context;
	GMainLoop			*loop;
									
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		SENSOR_DATA			sensor_data[32];
	#else
		SENSOR_DATA			sensor_data[NUM_ALARM];
	#endif
	MOTION_DATA_EVENT	motion_data[NUM_ACTIVE_CH+1];		// +1 means vloss fake data
	MOTION_RAW_DATA		motion_raw_data[NUM_ACTIVE_CH+1];
	MOTION_STATUS		motion_status[NUM_ACTIVE_CH+1];
	SYSTEM_DDATA		system_ddata;
	NET_DDATA			net_ddata;
	USER_DATA			user_data[NF_EVENT_USER_MAX+NF_EVENT_USER_UNKNOWN];	

	gboolean			sysdb_reload;
	gboolean			net_event;
	guint				vloss_mask;
	guint				novid_mask;
	guint64				virtual_alarm_mask;
	guint64				physical_alarm_mask;
	#if defined(_HDI_0412)
		guint				invalid_vid_mask;
	#endif

	gboolean			is_dst;
	#if defined(ENABLE_POE_CHECK)
		gboolean			is_enable_poe_mon;
	#endif
	NF_EVENT_WDT_DATA   wdt;
	/*< public >*/ /* with LOCK */

	/*< private >*/	
}__attribute__((packed));

struct _NfEventClass {
	NfObjectClass	parent_class;

	/* signals */

	/*< public >*/

	/*< private >*/

}__attribute__((packed));

gboolean 
nf_event_init_qc(int wait);

gboolean 
nf_event_init(int wait);

guint
nf_event_timer_add( guint interval, GSourceFunc cb_func, gpointer data);

guint 
nf_event_src_add( int fd, GSourceFunc cb_func, gpointer data);

gboolean
nf_event_timer_remove (guint tag);

void
nf_event_reload_alarm_in(void);

void
nf_event_check_ps(char *ps_name, gint *pid);

int nf_event_net_login_fail_check_cb(char *userid, int result, int reason);

void 
nf_event_logon_fail_check(gchar *user_id, gboolean is_fail, gboolean is_dvr);

void _event_set_virtual_alarm_mask(guint64 mask);
guint64 _event_get_virtual_alarm_mask();
guint64 _event_get_physical_alarm_mask();

#if defined(ENABLE_POE_CHECK)
	void nf_event_enable_poe_mon_debug(gboolean is_enable);
#endif

#define nf_event_src_remove		nf_event_timer_remove

#endif

