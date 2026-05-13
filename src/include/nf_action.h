#ifndef __NF_ACTION_H__
#define __NF_ACTION_H__

#include <glib.h>
#include <jansson.h>
#include "nf_object.h"
#include "nf_common.h"
#ifdef	SUPPORT_VCA_CAMERA
#include "nf_meta_data.h"
#endif	/* SUPPORT_VCA_CAMERA */
#include "nf_api_dva_eventlog.h"
#include "itx_ai_def.h"

#define ENABLE_MOUSE_UNTILKEY_STOP
#define NF_ACTION_FORCE_BUZZER_TRANS		// Force Transparent

typedef struct _NF_ACTION_AI_GENERIC_EVT
{
	char caption[45];   			/**< Object class */
	char title[64];						/**< Reserved */
} NF_ACTION_AI_GENERIC_EVT;

typedef enum _NF_ACTION_MODE_E
{
#if 0
	NF_ACTION_MODE_LATCHED		= 0,
	NF_ACTION_MODE_TRANSPARENT	= 1
#else
	NF_ACTION_MODE_TRANSPARENT	= 0
#endif
} NF_ACTION_MODE_E;

#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
typedef enum _NF_ACTION_EVENT_E
{
	NF_ACTION_EVENT_ALARM		= 0,
	NF_ACTION_EVENT_MOTION		= 1,
	NF_ACTION_EVENT_VLOSS		= 2,
	NF_ACTION_EVENT_DISK		= 3,
	NF_ACTION_EVENT_RECORD		= 4,
	NF_ACTION_EVENT_SYSTEM		= 5 ,
	NF_ACTION_EVENT_NET			= 6,
	NF_ACTION_EVENT_NR
}NF_ACTION_EVENT;
#endif

typedef enum _NF_ACTION_TYPE_E
{
	NF_ACTION_TYPE_NOPEN		= 0,
	NF_ACTION_TYPE_NCLOSE		= 1,
	NF_ACTION_TYPE_NONE = 2
} NF_ACTION_TYPE_E;

typedef enum _NF_ACTION_RELAY_CHAR_E{
    NF_ACTION_RELAY_CHAR_EVENT		= '2',  /* By event (schedule) */
    NF_ACTION_RELAY_CHAR_ON			= '1',  /* on */
    NF_ACTION_RELAY_CHAR_OFF		= '0',  /* off */
} NF_ACTION_RELAY_CHAR_E;


typedef enum _NF_ACTION_HDD_EVENT_E
{
	NF_ACTION_HDD_EVENT_OVER			= 0,
	NF_ACTION_HDD_EVENT_SMART			= 1,
	NF_ACTION_HDD_EVENT_NODISK			= 2,
	NF_ACTION_HDD_EVENT_WRFAIL			= 3,
	NF_ACTION_HDD_EVENT_EXHU			= 4,
	NF_ACTION_HDD_EVENT_FULL			= 5,
	NF_ACTION_HDD_EVENT_SMART_REQCHK	= 6,

	NF_ACTION_HDD_EVENT_NR
} NF_ACTION_HDD_EVENT_E;

typedef enum _NF_ACTION_REC_EVENT_E
{
	NF_ACTION_REC_EVENT_PANIC	= 0,

	NF_ACTION_REC_EVENT_NR
} NF_ACTION_REC_EVENT_E;

typedef enum _NF_ACTION_SYSTEM_EVENT_E
{
	NF_ACTION_SYSTEM_EVENT_BOOTING		= 0,
	NF_ACTION_SYSTEM_EVENT_LOG_ON_FAIL	= 1,
	NF_ACTION_SYSTEM_EVENT_FAN_FAIL		= 2,
	NF_ACTION_SYSTEM_EVENT_TEMPER_FAIL	= 3,
	#if defined(ENABLE_POE_CHECK)
		NF_ACTION_SYSTEM_EVENT_POE			= 4,
	#endif
	NF_ACTION_SYSTEM_EVENT_NR
} NF_ACTION_SYSTEM_EVENT_E;

typedef enum _NF_ACTION_NET_EVENT_E
{
	NF_ACTION_NET_EVENT_TROUBLE			= 0,
	NF_ACTION_NET_EVENT_LOGON_FAIL		= 1,
	NF_ACTION_NET_EVENT_DDNS_FAIL		= 2,
	NF_ACTION_NET_EVENT_IP_CONFLICT		= 3,
	NF_ACTION_NET_EVENT_TROUBLE_AI_BOX	= 4,

	NF_ACTION_NET_EVENT_NR

} NF_ACTION_NET_EVENT_E;

#if defined(SUPPORT_VCA_CAMERA)
/*	ivca_def.h
	Event types. */
typedef enum _NF_ACTION_VCA_TYPE_E
{
	NF_ACTION_VCA_DIR_POS         = 0,  /**< Crossed positive direction. */
	NF_ACTION_VCA_DIR_NEG         = 1,  /**< Crossed negative direction. */
	NF_ACTION_VCA_ENTER           = 2,  /**< Entered. */
	NF_ACTION_VCA_EXIT            = 3,  /**< Exited. */
	NF_ACTION_VCA_STOPPED         = 4,  /**< Stopped. */
	NF_ACTION_VCA_ABANDONED       = 5,  /**< Abandoned. */
	NF_ACTION_VCA_REMOVED         = 6,  /**< Removed. */
	NF_ACTION_VCA_LOITERED        = 7,  /**< Loitered. */
	NF_ACTION_VCA_FALL            = 8,  /**< Fall. */
	NF_ACTION_VCA_COUNTER         = 9,  /**< Counter value exceeded. */
	NF_ACTION_VCA_TAMPER          = 10,  /**< Camera tamper detected. */
	NF_ACTION_VCA_COLOR           = 11,  /**< Color filter. */
	NF_ACTION_VCA_SIZE            = 12,  /**< Size filter. */
	NF_ACTION_VCA_CLASS           = 13,  /**< Class filter. */
	NF_ACTION_VCA_SPEED           = 14,  /**< Speed filter. */

	NF_ACTION_VCA_NR
	/* FILL ME */
} NF_ACTION_VCA_TYPE;
#endif

/** IMSI AI BOX **/
typedef enum _NF_ACTION_AI_TYPE_E
{
	NF_ACTION_AI_DIR_POS         = 0,  /**< Crossed positive direction. */
	NF_ACTION_AI_DIR_NEG         = 1,  /**< Crossed negative direction. */
	NF_ACTION_AI_ENTER           = 2,  /**< Entered. */
	NF_ACTION_AI_EXIT            = 3,  /**< Exited. */
	NF_ACTION_AI_STOPPED         = 4,  /**< Stopped. */
	NF_ACTION_AI_ABANDONED       = 5,  /**< Abandoned. */
	NF_ACTION_AI_REMOVED         = 6,  /**< Removed. */
	NF_ACTION_AI_LOITERED        = 7,  /**< Loitered. */
	NF_ACTION_AI_FALL            = 8,  /**< Fall. */
	NF_ACTION_AI_COUNTER         = 9,  /**< Counter value exceeded. */
	NF_ACTION_AI_TAMPER          = 10,  /**< Camera tamper detected. */
	NF_ACTION_AI_COLOR           = 11,  /**< Color filter. */
	NF_ACTION_AI_SIZE            = 12,  /**< Size filter. */
	NF_ACTION_AI_CLASS           = 13,  /**< Class filter. */
	NF_ACTION_AI_SPEED           = 14,  /**< Speed filter. */
	NF_ACTION_AI_INTRUSION           = 15,  /**< Intrusion */
	NF_ACTION_AI_FR           = 16,  /**< Face Reco */
	NF_ACTION_AI_LPR         = 17,  /**< LPR */
	NF_ACTION_AI_GENERIC           = 18,  /**< Generic */

	NF_ACTION_AI_NR
	/* FILL ME */
} NF_ACTION_AI_TYPE;

/** DVA Event **/
typedef enum _NF_ACTION_DVA_DESC_TYPE_E
{
	NF_ACTION_DVA_DESC_HUMAN	= 0,
	NF_ACTION_DVA_DESC_VEHICLE	= 1,
	NF_ACTION_DVA_DESC_ANIMAL	= 2,
	NF_ACTION_DVA_DESC_ILLEGALPARKING = 3,
	NF_ACTION_DVA_DESC_NR,
}NF_ACTION_DVA_DESC_TYPE;

typedef enum _NF_ACTION_DVA_IDZ_TYPE_E
{
	/*human*/
	NF_ACTION_DVA_IDZ_HUMAN         = 0,
	/*vehicle*/	
	NF_ACTION_DVA_IDZ_BICYCLE       = 1,
	NF_ACTION_DVA_IDZ_MOTORBIKE		= 2,
	NF_ACTION_DVA_IDZ_BUS			= 3,
	NF_ACTION_DVA_IDZ_CAR		    = 4,
	/*animal*/
	NF_ACTION_DVA_IDZ_BIRD     		= 5,
	NF_ACTION_DVA_IDZ_CAT     		= 6,
	NF_ACTION_DVA_IDZ_DOG    		= 7,
	NF_ACTION_DVA_IDZ_COW     		= 8,
	NF_ACTION_DVA_IDZ_HORSE     	= 9,
	
	NF_ACTION_DVA_IDZ_NR
	/* FILL ME */
} NF_ACTION_DVA_IDZ_TYPE;

typedef enum _NF_ACTION_DVA_IPZ_TYPE_E
{
	NF_ACTION_DVA_IPZ_BICYCLE     	= 0,
	NF_ACTION_DVA_IPZ_MOTORBIKE	    = 1,
	NF_ACTION_DVA_IPZ_BUS			= 2,
	NF_ACTION_DVA_IPZ_CAR		    = 3,
	NF_ACTION_DVA_IPZ_NR
	/* FILL ME */
} NF_ACTION_DVA_IPZ_TYPE;

#if defined(ENABLE_ACTION_PTZ_PRESET)
typedef enum _NF_ACTION_PTZ_PRESET_E
{
	NF_ACTION_PTZ_PRESET_ALARM			= 0,
	NF_ACTION_PTZ_PRESET_MOTION			= 1,
	NF_ACTION_PTZ_PRESET_VLOSS			= 2,
	NF_ACTION_PTZ_PRESET_POS			= 3,
	NF_ACTION_PTZ_PRESET_DVA			= 4,
	NF_ACTION_PTZ_PRESET_VCA			= 5,
	NF_ACTION_PTZ_PRESET_AI				= 6, /** IMSI AI BOX **/

	NF_ACTION_PTZ_PRESET_NR

} NF_ACTION_PTZ_PRESET_E;
#endif

#if defined(ENABLE_EMAIL_DUAL_SERVER)
typedef enum _NF_ACTION_MAIL_SERVER_E
{
	NF_ACTION_MAIL_SERVER_FIRST		= 0,
	NF_ACTION_MAIL_SERVER_SECNOD	= 1

} NF_ACTION_MAIL_SERVER;
#endif

#define NF_ACTION_UNTIL_KEY				9999
#define NF_ACTION_SNAPSHOT_OFF			0xFF

/* type macro */
#define NF_TYPE_ACTION					(nf_action_get_type ())

#define NF_IS_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_ACTION))
#define NF_IS_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_ACTION))

#define NF_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_ACTION, NfActionClass))
#define NF_ACTION(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_ACTION, NfAction))
#define NF_ACTION_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_ACTION, NfActionClass))

#define NF_ACTION_CAST(obj)				((NfAction*)(obj))
#define NF_ACTION_CLASS_CAST(klass)		((NfActionClass*)(klass))

#define NF_ACTION_HOUR_A_DAY			24
#define NF_ACTION_RELAY_SCHED_DAY_NUM	7		// day(mon ~ sun)
#define NF_ACTION_RELAY_SCHED_DATA		((NF_ACTION_RELAY_SCHED_DAY_NUM * NF_ACTION_HOUR_A_DAY)+1)

#define NF_ACTION_EMAIL_MAX_ADDRESS		8
#define NF_ACTION_EMAIL_MAX_LENGTH		64

#define NF_JPEC_REQUEST_TIME			1500
#define NF_JPEC_JPEG_CHECK_TIME			3000

#define NF_ACTION_LANG_ID_MAX			3

#define NF_ACTION_VENDOR_VIDECON		"VIDECON"

#define NF_ACTION_POS_TEXT_DATA_MAX_NUM		8
#define NF_ACTION_POS_TEXT_MAX_LENGTH		160 // 4byte + 8byte + 128byte + 20byte(reserved)

typedef struct _NfAction 		NfAction;
typedef struct _NfActionClass 	NfActionClass;

typedef struct _RELAY_STATE_T {
	gint			onoff;
	gint			state;
	glong			dwell_in_sec;
	glong			dwell_out_sec;
	guint			keyin_check;
	gboolean		is_manual_test;

} RELAY_STATE;

typedef struct _RELAY_DATA_T {
	gboolean		op_type;

	gboolean		relay_type;
	gint 			dwell_time;
} RELAY_DATA;

typedef struct _RELAYL_SCHED_DATA_T {
	gchar			sched_mode;
	gchar			sched_data[NUM_RELAY][NF_ACTION_RELAY_SCHED_DATA];
	gchar			*sched_ptr;
} RELAY_SCHED_DATA;

typedef struct _BUZZER_STATE_T {
	gboolean		onoff;
	gint			state;

	glong           dwell_in_sec;
	glong           dwell_out_sec;
	guint			keyin_check;
	gboolean		is_manual_test;
#if defined(NF_ACTION_FORCE_BUZZER_TRANS)
	guint			event[NF_ACTION_EVENT_NR];
#endif
} BUZZER_STATE;

typedef struct _BUZZER_DATA_T {
	gboolean		buzzer_act;

	guint			dwell_type;
	gint			dwell_time;
} BUZZER_DATA;

typedef struct _EMAIL_DATA_T {
	gboolean		email_act;
	guint			frequency;

	gchar           address[NF_ACTION_EMAIL_MAX_ADDRESS][NF_ACTION_EMAIL_MAX_LENGTH];
	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		guint			email_serv[NF_ACTION_EMAIL_MAX_ADDRESS];
	#endif
	gboolean		snapshot_onoff;
	gint			snapshot_ch;

	#if 0
	guint			sched_from;
	guint			sched_to;
	guint			sched_wday[7];
	#else
		guint			sched_wday[10];
		guint			sched_from_h[10];
		guint			sched_from_m[10];
		guint			sched_from_s[10];
		guint			sched_to_h[10];
		guint			sched_to_m[10];
		guint			sched_to_s[10];
	#endif
	gboolean		vlink_onoff;

	guint           al_switch;
	guint           al_switch_port;
} EMAIL_DATA;

typedef struct _FTP_DATA_T {
	gboolean		ftp_act;
	guint			frequency;				// MIN

	gboolean		snapshot_onoff;
	gint			snapshot_ch;

	gboolean		video_onoff;
	gint			video_ch;

	guint			trans_mode;				// act.ftp_noti.trans_mode	// UINT
	char			dir_path[256];			// act.ftp_noti.dir_path
	guint			dir_mode;				// act.ftp_noti.dir_mode
	char			fname_prefix[256];		// act.ftp_noti.fname_prefix
	guint			fname_mode;				// act.ftp_noti.fname_mode
	guint			webra_link;				// act.ftp_noti.weblin
} FTP_DATA;

#if defined(ENABLE_ACTION_MOBILE)
typedef struct _MOBILE_DATA_T {
	gboolean		mobile_act;
	guint			frequency;				// MIN

	gboolean		snapshot_onoff;
	gint			snapshot_ch;
} MOBILE_DATA;
#endif


#if defined(ENABLE_ACTION_PUSH)
typedef struct _PUSH_DATA_T {
	gboolean		push_act;
	guint			frequency;				// MIN

	gboolean		snapshot_onoff;
	gint			snapshot_ch;

	guint           al_switch;
	guint           al_switch_port;
} PUSH_DATA;
#endif

typedef struct _EMAIL_STATE_T {

	glong			send_time;
	glong			start_time;
	glong			div_frequency;
	gint			div_count;

	gint            email_state;
	gboolean        is_event;

} EMAIL_STATE;

#if defined(ENABLE_ACTION_MOBILE)
typedef struct _MOBILE_STATE_T {

	glong			send_time;
	glong			start_time;
	glong			div_frequency;
	gint			div_count;

} MOBILE_STATE;
#endif

#if defined(ENABLE_ACTION_PUSH)
typedef struct _PUSH_STATE_T {

	glong			send_time;
	glong			start_time;
	glong			div_frequency;
	gint			div_count;

} PUSH_STATE;
#endif

typedef struct _ALARM_SENSOR_DATA_T {
	guint64			mask_arout;
	guint			mask_lcamera;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} ALARM_SENSOR_DATA;

typedef struct _MOTION_DATA_T {
	gint			ignore_interval;
	guint64			mask_arout;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} MOTION_DATA;

typedef struct _MOTION_STATE_T {
	gint			is_motion;
	gint			is_lasting;
	glong			ignore_sec;
} MOTION_STATE;

typedef struct _VLOSS_DATA_T {
	guint64			mask_arout;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif
} VLOSS_DATA;

typedef struct _DISK_DATA_T {
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif
} DISK_DATA;

typedef struct _DISK_DDATA_T {
	guint			exhaust_threshold;
	gint			nodisk_cnt;
} DISK_DDATA;

typedef struct _REC_DATA_T {
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} REC_DATA;

typedef struct _SYSTEM_DATA_T {
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} SYSTEM_DATA;

typedef struct _NET_DATA_T {
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} NET_DATA;

typedef struct _TEXTIN_DATA_T {
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} TEXTIN_DATA;

#if defined(ENABLE_EVENT_TAMPER)
typedef struct _TAMPER_DATA_T {
	gint			ignore_interval;
	guint64			mask_arout;
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
} TAMPER_DATA;

typedef struct _TAMPER_STATE_T {
	gint			is_tamper;
	gint			is_lasting;
	glong			ignore_sec;
} TAMPER_STATE;
#endif

typedef enum _NF_ACTION_POS_DATA_RESET_E
{
	NF_ACTION_POS_DATA_RESET_EMAIL		= 1<<0,
	NF_ACTION_POS_DATA_RESET_FTP		= 1<<1,
	NF_ACTION_POS_DATA_RESET_PUSH		= 1<<2,
} NF_ACTION_POS_DATA_RESET_E;

typedef struct _POS_DATA_T {
	guint64			mask_arout;
	guint			mask_lcamera;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif
} POS_DATA;

typedef struct _POS_TEXT_DATA_T {
	gint data_position;
	guint64	timestamp[NF_ACTION_POS_TEXT_DATA_MAX_NUM];
	char data[NF_ACTION_POS_TEXT_DATA_MAX_NUM][128+1];
} POS_TEXT_DATA;

typedef struct _DVA_DATA_T {
	gint			ignore_interval;
	guint64			mask_arout;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} DVA_DATA;

#ifdef	SUPPORT_VCA_CAMERA

#define	VCA_MAX_ELEMS	MAX(IVCA_MAX_ZONES, IVCA_MAX_CNTRS)
#if	VCA_MAX_ELEMS > 32
#error "VCA_MAX_ELEMS should not be larger than 32!"
#endif

typedef struct _VCA_DATA_T {
	guint64			mask_arout;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} VCA_DATA;

typedef struct _VCA_STATE_T {
	glong			dwell_out_sec;
} VCA_STATE;
#endif	/* SUPPORT_VCA_CAMERA */

/** IMSI AI BOX **/
typedef struct _AI_DATA_T {
	guint64			mask_arout;
	#if defined(ENABLE_ACTION_PTZ_PRESET)
		gboolean		preset_act;
		unsigned char			preset_num[NUM_ACTIVE_CH];
	#endif
	gboolean		buzzer_act;
	gboolean		email_act;
	gboolean		ftp_act;
	#if defined(ENABLE_ACTION_MOBILE)
		gboolean		mobile_act;
	#endif
	#if defined(ENABLE_ACTION_PUSH)
		gboolean		push_act;
	#endif

} AI_DATA;

typedef struct _AI_STATE_T {
	glong			dwell_out_sec;
} AI_STATE;


#define NF_ACTION_EMAIL_MAX_INTERVAL	60

// 1분 마다의 이벤트를 저장
// 6 * 64 Event
// Table col 0~59 	 : 1minute ~ frequency minute
// Table col 60~62   : empty
// Table col 63      : tatal event sum
// Table row0:alarm  	row1:motion
//       row2:vloss  	row3:hdd
//       row4:booting 	row5:setup change

typedef struct _EMAIL_SNAPSHOT_DATA_T {
	gint			ch;
	guint			event_cnt;

	gboolean		is_request;
	gboolean		is_complete;

	GTimeVal		detect_time;
	GTimeVal		capture_time;

	guchar			*data;
	guint			data_size;
} EMAIL_SNAPSHOT_DATA;

typedef enum _NF_ACTION_EMAIL_STATE_E
{
	NF_ACTION_EMAIL_STATE_FIRST     = 0,
	NF_ACTION_EMAIL_STATE_SECOND    = 1,

} NF_ACTION_EMAIL_STATE;

typedef struct _FTP_VIDEO_DATA_T {
	gint			ch;
	guint			event_cnt;

	gboolean		is_request;
	gboolean		is_complete;

	GTimeVal		start_time;
	GTimeVal		end_time;
} FTP_VIDEO_DATA;

typedef enum _NF_ACTION_PROP_E
{
	PROP_SENSOR,
	#if defined(ENABLE_SENSOR_IPCAM)
		PROP_SENSOR_IPCAM,
	#endif
	PROP_MOTION,
	PROP_VLOSS,

	PROP_DISK_OVERWR,
	PROP_DISK_SMART,
	PROP_DISK_NODISK,
	PROP_DISK_WRFAIL,
	PROP_DISK_EXHAUST,
	PROP_DISK_FULL,
	PROP_DISK_SMART_REQCHK,

	PROP_REC_PANIC,
	PROP_REC_ALARM,
	PROP_REC_MOTION,

	PROP_SYS_BOOTING,
	PROP_SYS_LOGON_FAIL,
	PROP_SYS_FAN,
	PROP_SYS_TEMPERATURE,
	#if defined(ENABLE_POE_CHECK)
		PROP_SYS_POE,
		PROP_SYS_POE_HUB,
	#endif

	PROP_NET_TROUBLE,
	PROP_NET_LOGIN_FAIL,
	PROP_NET_DDNS_FAIL,
	PROP_NET_SET_IP_CONFLICT,
	PROP_NET_CAM_IP_CONFLICT,
	PROP_NET_TROUBLE_AI,
	PROP_TEXT_IN,
	#if defined(ENABLE_EVENT_TAMPER)
		PROP_TAMPER,
	#endif

	PROP_SETUP_CHG,

#ifdef	SUPPORT_VCA_CAMERA
	PROP_VCA,
#endif	/* SUPPORT_VCA_CAMERA */
	PROP_POS,
	PROP_DVA, /** DVA Event **/
	PROP_AI, /** IMSI AI BOX **/
	PROP_AI_FR,
	PROP_AI_LPR,
	PROP_AI_GENERIC,
	LAST_PROP,
	/* FILL ME */
} NF_ACTION_PROP;

typedef enum _NF_ACTION_EMAIL_LANG_ID_E {
	LANG_ID_ALARM					= 0,
	LANG_ID_MOTION,
	LANG_ID_VLOSS,

	LANG_ID_REC_PANIC,
	LANG_ID_REC_ALARM,
	LANG_ID_REC_MOTION,

	LANG_ID_TEXT_IN,
	#if defined(ENABLE_EVENT_TAMPER)
		LANG_ID_TAMPER,
	#endif
	#if defined(SUPPORT_VCA_CAMERA)
		LANG_ID_VCA,
	#endif
	LANG_ID_SETUP_CHANGE,
	LANG_ID_CH,
	LANG_ID_TITLE,
	LANG_ID_TIME,
	LANG_ID_SYSTEM_ID,
	LANG_ID_COUNT_OCCUR,
	LANG_ID_OCCUR,
	LANG_ID_POS,
	LANG_ID_DVA_IDZ,
	LAND_ID_DVA_IPZ,
	LAND_ID_DVA_LPR,
	LANG_ID_DVA_OBJ_CNT,
	LANG_ID_AI, /** IMSI AI BOX **/
	LANG_ID_AI_FR,			/** IMSI AI FR LPR **/		
	LANG_ID_AI_LPR,
	LANG_ID_AI_GENERIC,
	LANG_ID_NR

} NF_ACTION_EMAIL_LANG_ID_E;

// TODO event log id 와 action 모듈 내에서 사용하는 lod ig 연동 작업 필요
typedef enum _NF_ACTION_PUSH_ID_E {
	LANG_PUSH_ID_ALARM					= 0,
	LANG_PUSH_ID_MOTION,
	LANG_PUSH_ID_VLOSS,

	LANG_PUSH_ID_REC_PANIC,
	LANG_PUSH_ID_REC_ALARM,
	LANG_PUSH_ID_REC_MOTION,

	LANG_PUSH_ID_TEXT_IN,
	#if defined(ENABLE_EVENT_TAMPER)
		LANG_PUSH_ID_TAMPER,
	#endif
	LANG_PUSH_ID_POS,
	LANG_PUSH_ID_DVA_IDZ,
	LANG_PUSH_ID_DVA_IPZ,
	LANG_PUSH_ID_DVA_OBJ_CNT,
	#if defined(SUPPORT_VCA_CAMERA)
		LANG_PUSH_ID_VCA,
	#endif
	LANG_PUSH_ID_AI, /** IMSI AI BOX **/
	LANG_PUSH_ID_AI_FR,		/** IMSI AI FR LPR **/
	LANG_PUSH_ID_AI_LPR,
	LANG_PUSH_ID_AI_GENERIC,
	LANG_PUSH_ID_NR

} NF_ACTION_PUSH_ID_E;

#if 0
	LANG_ID_HDD_EVENT_OVER		= 3,
	LANG_ID_HDD_EVENT_FULL		= 4,
	LANG_ID_HDD_EVENT_EXHA		= 5,
	LANG_ID_HDD_EVENT_SMART		= 6,
	LANG_ID_HDD_EVENT_NODISK	= 7,

	LANG_ID_BOOTING				= 9,
	LANG_ID_LOGON_FAIL			= 10,

	LANG_ID_NET_ETH_TROUBLE		= 11,
	LANG_ID_NET_REMOTE_FAIL		= 12,
	LANG_ID_NET_DDNS_FAIL		= 13,
#endif

typedef enum _NF_ACTION_EMAIL_EVENT_TYPE0_E
{
	EMAIL_EVENT_TYPE0_DISARM			= 0,
	EMAIL_EVENT_TYPE0_ARM,
	EMAIL_EVENT_TYPE0_NR
} NF_ACTION_EMAIL_EVENT_TYPE0;


typedef enum _NF_ACTION_EMAIL_EVENT_TYPE1_E
{
	EMAIL_EVENT_TYPE1_SENSOR		= 0,
	EMAIL_EVENT_TYPE1_MOTION,
	EMAIL_EVENT_TYPE1_VLOSS,

	EMAIL_EVENT_TYPE1_REC_PANIC,
	EMAIL_EVENT_TYPE1_REC_ALARM,
	EMAIL_EVENT_TYPE1_REC_MOTION,
	EMAIL_EVENT_TYPE1_TEXT_IN,
	#if defined(ENABLE_EVENT_TAMPER)
		EMAIL_EVENT_TYPE1_TAMPER,
	#endif
	EMAIL_EVENT_TYPE1_POS,
	EMAIL_EVENT_TYPE1_DVA_IDZ,
	EMAIL_EVENT_TYPE1_DVA_IPZ,
	EMAIL_EVENT_TYPE1_DVA_OBJ_CNT,

	EMAIL_EVENT_TYPE1_NR
} NF_ACTION_EMAIL_EVENT_TYPE1;

typedef enum _NF_ACTION_EMAIL_EVENT_TYPE2_E
{
	EMAIL_EVENT_TYPE2_HDD			= 0,
	EMAIL_EVENT_TYPE2_SYSTEM		= 1,
	EMAIL_EVENT_TYPE2_NET			= 2,
	EMAIL_EVENT_TYPE2_SETUP_CHG		= 3,

	EMAIL_EVENT_TYPE2_NR
	/* FILL ME */
} NF_ACTION_EMAIL_EVENT_TYPE2;

typedef enum _NF_ACTION_EMAIL_EVENT_TYPE3_E
{
	#if defined(SUPPORT_VCA_CAMERA)
		EMAIL_EVENT_TYPE3_VCA			= 0,
	#endif

	EMAIL_EVENT_TYPE3_NR
	/* FILL ME */
} NF_ACTION_EMAIL_EVENT_TYPE3;
/** DVA Event **/
typedef enum _NF_ACTION_EMAIL_EVENT_TYPE4_E
{
	EMAIL_EVENT_TYPE4_DVA_IDZ			= 0,
	EMAIL_EVENT_TYPE4_DVA_IPZ			= 1,
	/*DVA OBJ CNT*/
	EMAIL_EVENT_TYPE4_DVA_OBJ_CNT		= 2,
	EMAIL_EVENT_TYPE4_NR
	/* FILL ME */
} NF_ACTION_EMAIL_EVENT_TYPE4;
/** DVA Event **/

/** IMSI AI BOX **/
typedef enum _NF_ACTION_EMAIL_EVENT_TYPE5_E
{
	EMAIL_EVENT_TYPE5_AI			= 0,
	EMAIL_EVENT_TYPE5_NR
} NF_ACTION_EMAIL_EVENT_TYPE5;

#if defined(ENABLE_SENSOR_IPCAM)
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		#define EMAIL_EVENT_TYPE1_INDEX_MAX			32
	#else
		#define EMAIL_EVENT_TYPE1_INDEX_MAX			NUM_ALARM
	#endif
#else
	#define EMAIL_EVENT_TYPE1_INDEX_MAX			NUM_ACTIVE_CH
#endif

#define EMAIL_EVENT_TYPE2_INDEX_MAX				7
#define EMAIL_EVENT_TYPE3_INDEX_MAX				15
/** DVA Event **/
#define EMAIL_EVENT_TYPE4_INDEX_MAX				NF_ACTION_DVA_IDZ_NR+NF_ACTION_DVA_IPZ_NR
#define EMAIL_EVENT_TYPE5_INDEX_MAX				NF_ACTION_AI_NR /** IMSI AI BOX **/
#define EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX 30
typedef struct _EMAIL_EVENT_DATA_TYPE0_T {
	gushort			rise[EMAIL_EVENT_TYPE0_NR][64];
	gushort			active[EMAIL_EVENT_TYPE0_NR][64];
} EMAIL_EVENT_DATA_TYPE0;

typedef struct _EMAIL_EVENT_DATA_TYPE1_T {
	gushort			rise[EMAIL_EVENT_TYPE1_INDEX_MAX][64];
	gushort			active[EMAIL_EVENT_TYPE1_INDEX_MAX][64];
} EMAIL_EVENT_DATA_TYPE1;

typedef struct _EMAIL_EVENT_DATA_TYPE2_T {
	gushort			rise[EMAIL_EVENT_TYPE2_NR][EMAIL_EVENT_TYPE2_INDEX_MAX][64];
	gushort			active[EMAIL_EVENT_TYPE2_NR][EMAIL_EVENT_TYPE2_INDEX_MAX][64];
} EMAIL_EVENT_DATA_TYPE2;

typedef struct _EMAIL_EVENT_DATA_TYPE3_T {
	gushort			rise[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE3_NR][EMAIL_EVENT_TYPE3_INDEX_MAX][64];
	gushort			active[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE3_NR][EMAIL_EVENT_TYPE3_INDEX_MAX][64];
} EMAIL_EVENT_DATA_TYPE3;

/** DVA Event **/
typedef struct _EMAIL_EVENT_DATA_TYPE4_T {
	gushort			rise[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR][EMAIL_EVENT_TYPE4_INDEX_MAX][64];
	gushort			active[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR][EMAIL_EVENT_TYPE4_INDEX_MAX][64];
} EMAIL_EVENT_DATA_TYPE4;
/** DVA Event **/
typedef struct _EMAIL_EVENT_DATA_TYPE5_T {
	gushort			rise[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE5_NR][EMAIL_EVENT_TYPE5_INDEX_MAX][64];
	gushort			active[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE5_NR][EMAIL_EVENT_TYPE5_INDEX_MAX][64];
	char			text[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE5_NR][EMAIL_EVENT_TYPE5_INDEX_MAX][128];
	//NF_ACTION_AI_GENERIC_EVT st_generic_evt[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX];
	ai_generic_event_t st_generic_evt[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE5_GENERIC_EVT_INDAX_MAX];
} EMAIL_EVENT_DATA_TYPE5;

typedef struct _EMAIL_EVENT_DATA_T {
	EMAIL_EVENT_DATA_TYPE0		type0;
	EMAIL_EVENT_DATA_TYPE1		type1[EMAIL_EVENT_TYPE1_NR];
	EMAIL_EVENT_DATA_TYPE2		type2;
	EMAIL_EVENT_DATA_TYPE3		type3;
	EMAIL_EVENT_DATA_TYPE4		type4;  /** DVA Event **/
	EMAIL_EVENT_DATA_TYPE5		type5;  /** AI Event **/
} EMAIL_EVENT_DATA;

typedef struct _RECENT_EVENT_DATA_T {
	guint event_type; //1:NF_ACTION_EMAIL_EVENT_TYPE1, 2:NF_ACTION_EMAIL_EVENT_TYPE2
	guint event_num; //EMAIL_EVENT_TYPE1_DVA_IDZ
	guint event_num_sub;
	guint ch;
	guint64 timestamp;
	guint64 timestampl;
} RECENT_EVENT_DATA;

typedef struct _DVA_META_T {
	guint64 timestamp;
	guint64 timestampl;
	union {
		DVA_MSG_IDZ intrusion_detection;
		DVA_MSG_IPZ illegal_parking;
		DVA_MSG_LPR lpr;
		DVA_MSG_COUNTER counter;
	} __attribute__((packed));
} DVA_META;

/**
 * NfAction:
 *
 * NfDVR action class
 */
struct _NfAction {
	NfObject			object;

	/*< public >*/
	gint				init_done;

	GAsyncQueue			*queue;
	GThread				*thread;

	gboolean			sysdb_reload;

	gint				thread_run;
	gint				thread_status;

	// for callback statistics
	guint				cb_rise_alarm;
	guint				*cb_alarm_timestamp;
#if defined(ENABLE_SENSOR_IPCAM)
	guint				cb_rise_alarm_ipcam;
	guint				cb_rise_alarm_dvr;
#endif
	guint				cb_rise_vloss;
	guint				cb_vloss_timestamp[NUM_ACTIVE_CH];
	
	guint				cb_rise_motion;
	guint				cb_motion_timestamp[NUM_ACTIVE_CH];
	
	guint				cb_rise_hdd;
	guint				cb_hdd_timestamp;
	
	guint				cb_rise_sysdb;
	
	guint				cb_rise_rec_panic;
	guint				cb_rec_panic_timestamp;
	
	guint				cb_rise_rec_alarm;
	guint				cb_rise_rec_motion;
	
	guint				cb_rise_net;
	guint				cb_net_timestamp;
	
	guint				cb_rise_system;
	guint				cb_system_timestamp;
	#if defined(ENABLE_EVENT_TAMPER)
		guint				cb_rise_tamper;
		guint				cb_tamper_timestamp[NUM_ACTIVE_CH];
	#endif
#ifdef	SUPPORT_VCA_CAMERA
	guint				cb_rise_vca[NUM_ACTIVE_CH];
	guint			 	cb_vca_timestamp[NUM_ACTIVE_CH][NF_ACTION_VCA_NR];
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	guint				cb_rise_ai[NUM_ACTIVE_CH];
	guint			 	cb_ai_timestamp[NUM_ACTIVE_CH][NF_ACTION_AI_NR];
	ai_rule_event_t		*cb_ai_meta_data;
	/** IMSI AI GENERIC EVENT **/
	ai_generic_event_t	*cb_ai_generic_data;
	
	/** AI KEEP ALIVE **/
	guint				cb_ai_keep_alive_state[NUM_ACTIVE_CH];
	
	guint				cb_rise_pos;
	guint				cb_rise_dva_idz;
	guint				cb_rise_dva_ipz;
	/*DVA OBJ CNT*/
	guint				cb_rise_dva_obj_cnt;
	
	/** DVA Event **/
	guint				cb_rise_dva_event[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR];

	guint64				cb_curr_alarm;
#if defined(ENABLE_SENSOR_IPCAM)
	guint				cb_curr_alarm_ipcam;
	guint				cb_curr_alarm_dvr;
#endif
	guint				cb_curr_vloss;
	guint				cb_curr_motion;
	guint				cb_curr_hdd;
	guint				cb_curr_sysdb;
	guint				cb_curr_rec_panic;
	guint				cb_curr_rec_alarm;
	guint				cb_curr_rec_motion;
	guint				cb_curr_net;
	guint				cb_curr_system;
	#if defined(ENABLE_EVENT_TAMPER)
		guint				cb_curr_tamper;
	#endif
#ifdef	SUPPORT_VCA_CAMERA
	guint				cb_curr_vca[NUM_ACTIVE_CH];
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	guint				cb_curr_ai[NUM_ACTIVE_CH];
	/** IMSI AI FR LPR **/
	ai_fr_event_t		*cb_ai_fr_data;
	ai_lpr_event_t		*cb_ai_lpr_data;

	guint				cb_curr_pos;
	guint				cb_curr_dva_idz;
	guint				cb_curr_dva_ipz;
	/*DVA OBJ CNT*/
	guint				cb_curr_dva_obj_cnt;
	/** DVA Event **/
	guint				cb_curr_dva_event[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR];

	DVA_META			cb_meta_dva_idz;
	DVA_META			cb_meta_dva_ipz;
	/*DVA OBJ CNT*/
	DVA_META			cb_meta_dva_obj_cnt;
	
	// for action use
	guint64				rise_alarm;
	guint				*alarm_timestamp;
#if defined(ENABLE_SENSOR_IPCAM)
	guint				rise_alarm_ipcam;
	guint				rise_alarm_dvr;
#endif
	guint				rise_vloss;
	guint				vloss_timestamp[NUM_ACTIVE_CH];
	
	guint				rise_motion;
	guint				motion_timestamp[NUM_ACTIVE_CH];
	
	guint				rise_hdd;
	guint				hdd_timestamp;
	
	guint				rise_sysdb;
	
	guint				rise_rec_panic;
	guint				rec_panic_timestamp;
	
	guint				rise_rec_alarm;
	guint				rise_rec_motion;
	
	guint				rise_net;
	guint				net_timestamp;
	
	guint				rise_system;
	guint				system_timestamp;
	#if defined(ENABLE_EVENT_TAMPER)
		guint				rise_tamper;
		guint				tamper_timestamp[NUM_ACTIVE_CH];
	#endif
#ifdef	SUPPORT_VCA_CAMERA
	guint				rise_vca[NUM_ACTIVE_CH];
	guint			 	vca_timestamp[NUM_ACTIVE_CH][NF_ACTION_VCA_NR];
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	guint				rise_ai[NUM_ACTIVE_CH];
	guint			 	ai_timestamp[NUM_ACTIVE_CH][NF_ACTION_AI_NR];
	ai_rule_event_t		*ai_meta_data;
	/** IMSI AI GENERIC EVENT **/
	ai_generic_event_t	*ai_generic_data;
	
	/** AI KEEP ALIVE **/
	guint				ai_keep_alive_state[NUM_ACTIVE_CH];	
	
	guint				rise_pos;
	guint				rise_dva_idz;
	guint				rise_dva_ipz;
	/*DVA OBJ CNT*/
	guint				rise_dva_obj_cnt;
	/** DVA Event **/
	guint				rise_dva_event[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR];

	guint64				curr_alarm;
#if defined(ENABLE_SENSOR_IPCAM)
	guint				curr_alarm_ipcam;
	guint				curr_alarm_dvr;
#endif
	guint				curr_vloss;
	guint				curr_motion;
	guint				curr_hdd;
	guint				curr_sysdb;
	guint				curr_rec_panic;
	guint				curr_rec_alarm;
	guint				curr_rec_motion;
	guint				curr_net;
	guint				curr_system;
	#if defined(ENABLE_EVENT_TAMPER)
		guint				curr_tamper;
	#endif
#ifdef	SUPPORT_VCA_CAMERA
	guint				curr_vca[NUM_ACTIVE_CH];
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	guint				curr_ai[NUM_ACTIVE_CH];
	/** IMSI AI FR LPR **/
	ai_fr_event_t		*ai_fr_data;
	ai_lpr_event_t		*ai_lpr_data;

	guint				curr_pos;
	guint				curr_dva_idz;
	guint				curr_dva_ipz;
	/*DVA OBJ CNT*/
	guint				curr_dva_obj_cnt;
	/*DVA OBJ CNT*/
	
	/** DVA Event **/
	guint				curr_dva_event[NUM_ACTIVE_CH][EMAIL_EVENT_TYPE4_NR];

	DVA_META			meta_dva_idz;
	DVA_META			meta_dva_ipz;
	/*DVA OBJ CNT*/
	DVA_META			meta_dva_obj_cnt;

	RELAY_DATA			relay_data[NUM_RELAY];
	RELAY_STATE			relay_state[NUM_RELAY];
	RELAY_SCHED_DATA	relay_sched_data;

	BUZZER_DATA			buzzer_data;
	BUZZER_STATE		buzzer_state;

	EMAIL_DATA			email_data;
	EMAIL_STATE			email_state;
	EMAIL_EVENT_DATA 	*email_send_data;
	EMAIL_SNAPSHOT_DATA	email_snapshot;
	POS_TEXT_DATA		*email_pos_text_data;
	

	MOTION_DATA			motion_data[NUM_ACTIVE_CH];
	MOTION_STATE		motion_state[NUM_ACTIVE_CH];
	#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		ALARM_SENSOR_DATA	sensor_data[32];
	#else
		ALARM_SENSOR_DATA	sensor_data[NUM_ALARM];
	#endif
	VLOSS_DATA			vloss_data[NUM_ACTIVE_CH];
	DISK_DATA			disk_data[NF_ACTION_HDD_EVENT_NR];
	DISK_DDATA			disk_ddata;
	REC_DATA			rec_data[NF_ACTION_REC_EVENT_NR];
	SYSTEM_DATA			system_data[NF_ACTION_SYSTEM_EVENT_NR];
	NET_DATA			net_data[NF_ACTION_NET_EVENT_NR];
	TEXTIN_DATA			textin_data[NUM_ACTIVE_CH];
	POS_DATA			pos_data[NUM_ACTIVE_CH];
	DVA_DATA			dva_data[NUM_ACTIVE_CH];
	guint				pos_reset_flag;

	#if defined(ENABLE_EVENT_TAMPER)
		TAMPER_DATA			tamper_data[NUM_ACTIVE_CH];
		TAMPER_STATE		tamper_state[NUM_ACTIVE_CH];
	#endif
#ifdef	SUPPORT_VCA_CAMERA
	VCA_DATA			vca_data[NUM_ACTIVE_CH];
	VCA_STATE			vca_state[NUM_ACTIVE_CH];
#endif	/* SUPPORT_VCA_CAMERA */

	/** IMSI AI BOX **/
	AI_DATA			ai_data[NUM_ACTIVE_CH];
	AI_STATE		ai_state[NUM_ACTIVE_CH];

//#ifdef ENABLE_ACTION_FTP_SEND
	// 2013-01-13 오후 9:12:45 choissi
	FTP_DATA			ftp_data;
	EMAIL_STATE			ftp_state;
	EMAIL_EVENT_DATA 	*ftp_send_data;
	EMAIL_SNAPSHOT_DATA	ftp_snapshot;
	FTP_VIDEO_DATA		ftp_video;

	POS_TEXT_DATA		*ftp_pos_text_data;
//#endif

	#if defined(ENABLE_ACTION_MOBILE)
		MOBILE_DATA			mobile_data;
		MOBILE_STATE		mobile_state;
		EMAIL_EVENT_DATA 	*mobile_send_data;
	#endif

	#if defined(ENABLE_ACTION_PUSH)
		PUSH_DATA			push_data;
		PUSH_STATE			push_state;
		EMAIL_EVENT_DATA 	*push_send_data;

		POS_TEXT_DATA		*push_pos_text_data;
	#endif

	gboolean			is_dst;
	gboolean			is_booting;
	gint				curr_day;
	gint				curr_hour;

	gint				is_action_ctrl;

	gint                vendor;
	/*< public >*/ /* with LOCK */

	/*< private >*/
};

struct _NfActionClass {
  NfObjectClass	parent_class;

  /* signals */

  /*< public >*/

  /*< private >*/

};

/* function definition */
gboolean  nf_action_init(int wait);
void nf_action_relay_webra_onoff(gint relay_num);
gboolean nf_action_relay_test(gboolean is_test_on, gint relay_num, gboolean type);
void nf_action_buzzer_test(gboolean is_test_on);
#if defined (ENABLE_MOUSE_UNTILKEY_STOP)        /** 091222 by pakkhman **/
	void nf_action_mouse_untilkey_stop(void);
#endif
void nf_action_set_action_ctrl(gboolean is_action_ctrl);
int nf_aciton_get_double_knock_ch(int ch);

#endif
