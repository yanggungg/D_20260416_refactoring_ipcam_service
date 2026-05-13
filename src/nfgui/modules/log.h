/*
 * log.h
 * 	- log control module
 *	- dependencies :
 *	
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 2, 2011
 *
 */

#ifndef __LOG_H
#define __LOG_H

#include "nf_logevtdef.h"
#include "nf_api_eventlog.h"
#include "nf_api_pos_eventlog.h"
#include "nf_api_dva_eventlog.h"

////////////////////////////////////////////////////////////
//
// public data type 
//

#define	SZ_PARAM	264
#define LEN_USERID	31
#define LEN_SMART	31

#define _L(cat, param)	(cat << 16 | param)
#define _LCAT(type) (type >> 16)
#define _LSUB(type) (type & 0x0000FFFF)

typedef enum _LF_RESET_E {
	LF_ALL		= 0,
	LF_NOT_ALL	= 1
} LF_RESET_E;

typedef enum _LF_CAT_E {	// CAT: CATEGORY
	LF_CAT_ALL		= 0,
	LF_CAT_ALARM	= 1,
	LF_CAT_MOTION	= 2,
	LF_CAT_VLOSS	= 3,
	LF_CAT_RECORD	= 4,
	LF_CAT_SYSTEM	= 5,
	LF_CAT_STORAGE	= 6,
	LF_CAT_NETWORK	= 7,
	LF_CAT_IPCAM	= 8,
	LF_CAT_SETUP	= 9,
	LF_CAT_TAMPER	= 10,
	LF_CAT_VCA		= 11,
	LF_CAT_POS		= 12,
	LF_CAT_DEBUG	= 13,
	LF_CAT_DVA		= 14,

	LF_CAT_CNT		= 15
} LF_CAT_E;

typedef enum _LF_ORDER_E {
	LF_LATEST = NF_LOG_PARAM_DIR_BACKWARD,
	LF_OLDEST = NF_LOG_PARAM_DIR_FORWARD,
} LF_ORDER_E;

typedef struct _LPR_KLASS_T{
	int			klass;
	char		dummy[SZ_PARAM - 4];
} LPR_KLASS_T;

typedef struct _LPR_CHANNEL_T{
	int			klass;
	int			channel;
	char		dummy[SZ_PARAM - 4];
} LPR_CHANNEL_T;

typedef struct _LPR_SYSTEM_T {
	int			klass;
	char		userid[32];
	char		dummy[SZ_PARAM - 36];
} LPR_SYSTEM_T;

typedef struct _LPR_REBOOT_T {
	int			klass;
	char		dummy[SZ_PARAM - 4];
} LPR_REBOOT_T;

typedef struct _LPR_FWUP_T {
	int			klass;
	char		userid[32];
	char		dummy[SZ_PARAM - 36];
} LPR_FWUP_T;

typedef struct _LPR_SYSLOG_T {
	int			klass;
	char		userid[32];
	char		dummy[SZ_PARAM - 36];
} LPR_SYSLOG_T;

typedef struct _LPR_LOGONOFF_T {
	int			klass;
	char		userid[32];
	char		ipaddr[32];
	char		dummy[SZ_PARAM - 68];
} LPR_LOGONOFF_T;

typedef struct _LPR_SETUP_T {
	int			klass;
	char		userid[32];
	char		ipaddr[32];
	char		dummy[SZ_PARAM - 68];
} LPR_SETUP_T;

typedef struct _LPR_SMART_T {
	int			klass;
	char		diskid[32];
	char		dummy[SZ_PARAM - 36];
} LPR_SMART_T;

typedef struct _LPR_DISK_T {
	int			klass;
	int			location;
	char		dummy[SZ_PARAM - 8];
} LPR_DISK_T;

typedef struct _LPR_RECORD_T {
	int			klass;
	int			channel;
	char		dummy[SZ_PARAM - 8];
} LPR_RECORD_T;

typedef struct _LPR_SYSEVT_T {
	int			klass;
	int			channel;
	char		text[SZ_PARAM - 8];
} LPR_SYSEVT_T;

typedef struct _LPR_DEBUG_T {
	int			klass;
	char		text[SZ_PARAM - 4];
} LPR_DEBUG_T;

typedef struct _LPR_NETWORK_T {
	int			klass;
	int			channel;
	char		text[SZ_PARAM - 8];
} LPR_NETWORK_T;

typedef struct _LPR_IPCAM_T {
	int			klass;
	int			channel;
	char		dummy[SZ_PARAM - 8];
} LPR_IPCAM_T;

typedef struct _LPR_TAMPER_T {
	int			klass;
	int			channel;
	char		dummy[SZ_PARAM - 8];
} LPR_TAMPER_T;

typedef struct _LPR_VCA_T {
	int			klass;
	int			channel;
	int			type;
	char		binary[SZ_PARAM - 12];
} LPR_VCA_T;

typedef struct _LPR_DVA_T {
	int			klass;
	int			channel;
	int			type;
	char		binary[SZ_PARAM - 12];
} LPR_DVA_T;

typedef struct _LPR_POS_T {
	int			klass;
	int			channel;
	char		text[SZ_PARAM-8];
} LPR_POS_T;

typedef struct _LPR_ATM_T {
	int			klass;
	int			channel;
	char		text[SZ_PARAM-8];
} LPR_ATM_T;

typedef struct _LOG_DATA_T {
	GTimeVal 	tvTime;
	guint64		log_id;
	gint 		type;
	union {
		LPR_SYSTEM_T	cat_system;
		LPR_REBOOT_T	cat_reboot;
		LPR_FWUP_T		cat_fwup;
		LPR_SYSLOG_T	cat_syslog;
		LPR_LOGONOFF_T	cat_logon;
		LPR_LOGONOFF_T	cat_logoff;
		LPR_LOGONOFF_T	cat_rlogon;
		LPR_LOGONOFF_T	cat_rlogoff;
		LPR_SETUP_T		cat_syssetup;
		LPR_SETUP_T		cat_recsetup;
		LPR_CHANNEL_T	cat_sensor;
		LPR_CHANNEL_T	cat_motion;
		LPR_CHANNEL_T	cat_videoin;
		LPR_CHANNEL_T	cat_videoloss;
		LPR_CHANNEL_T	cat_tamper;
		LPR_SMART_T		cat_smart;
		LPR_DISK_T		cat_disk;
		LPR_DISK_T		cat_diskfull;
		LPR_DISK_T		cat_diskow;
		LPR_RECORD_T	cat_recstart;
		LPR_RECORD_T	cat_recstop;
		LPR_SYSEVT_T	cat_sysevt;
		LPR_DEBUG_T		cat_debug;
		LPR_POS_T		cat_pos;
		LPR_NETWORK_T	cat_network;
		LPR_IPCAM_T		cat_ipcam;
        LPR_VCA_T		cat_vca;
        LPR_DVA_T		cat_dva;
	} p;

} LOG_DATA_T;

typedef NF_LOG_DATA		SYSREC_DATA_T;

typedef struct _LOGX_T {
	NF_LOG_PARAM	param;
	int				count;
	guint64			page_top_logid;
	guint64			page_bottom_logid;
	LOG_DATA_T		*data;
	SYSREC_DATA_T	*sysrec;
} LOGX_T;

typedef struct _TLOG_DATA_T {
	GTimeVal 	tvTime;
	guint64		log_id;
	gint 		type;
	union {
		LPR_POS_T		cat_pos;
	} p;
} TLOG_DATA_T;

typedef NF_POS_LOG_DATA TLOG_SYSREC_DATA_T;

typedef struct _TLOGX_T {
	NF_POS_LOG_PARAM	param;
	int				    count;
	guint64			    page_top_logid;
	guint64			    page_bottom_logid;
	TLOG_DATA_T	        *data;
	TLOG_SYSREC_DATA_T	*sysrec;
} TLOGX_T;


typedef struct _DLOGX_T {
	NF_DVA_LOG_PARAM	param;
	int				    count;
	guint64			    page_top_logid;
	guint64			    page_bottom_logid;
	LOG_DATA_T	        *data;
	SYSREC_DATA_T		*sysrec;
} DLOGX_T;

typedef enum _PUTLOG_TYPE_E {
	OPEN_LIVE		 	= _L(LT_LOCAL_LOG_ON, LP2_LOCAL_LOG_ON_LIVE_DISPLAY),
	OPEN_PLAYBACK 		= _L(LT_LOCAL_LOG_ON, LP2_LOCAL_LOG_ON_PLAY_BACK),
	OPEN_ARCHIVING		= _L(LT_LOCAL_LOG_ON, LP2_LOCAL_LOG_ON_ARCHIVING),
	OPEN_SYS_SETUP		= _L(LT_LOCAL_LOG_ON, LP2_LOCAL_LOG_ON_SYSTEM_SETUP),
	OPEN_REC_SETUP		= _L(LT_LOCAL_LOG_ON, LP2_LOCAL_LOG_ON_RECORD_SETUP),

	CLOSE_LIVE		 	= _L(LT_LOCAL_LOG_OFF, LP2_LOCAL_LOG_OFF_LIVE_DISPLAY),
	CLOSE_PLAYBACK 		= _L(LT_LOCAL_LOG_OFF, LP2_LOCAL_LOG_OFF_PLAY_BACK),
	CLOSE_ARCHIVING		= _L(LT_LOCAL_LOG_OFF, LP2_LOCAL_LOG_OFF_ARCHIVING),
	CLOSE_SYS_SETUP		= _L(LT_LOCAL_LOG_OFF, LP2_LOCAL_LOG_OFF_SYSTEM_SETUP),
	CLOSE_REC_SETUP		= _L(LT_LOCAL_LOG_OFF, LP2_LOCAL_LOG_OFF_RECORD_SETUP),

	CHANGE_CAM_TITLE	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_TITLE),
	CHANGE_CAM_COLOR	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_COLOR),
	CHANGE_CAM_IMAGE	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAM_IMAGE),
	CHANGE_CAM_PTZ		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_PTZ),				// not used
	CHANGE_CAM_OSD		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_OSD),				// not used
	CHANGE_CAM_CVRT		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_COVERT),
	CHANGE_CAM_MOTION	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_MOTION_SENSOR),
	CHANGE_CAM_VCA		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_VCA),
	CHANGE_AUD_MAP		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CAMERA_AUDIO_MAPPING),	// not used

	CHANGE_EVT_ALARMOUT	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ALARM_OUT),
	CHANGE_EVT_NOTI		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_NOTIFICATION),
	CHANGE_EVT_SENSOR	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EVENT_SENSOR),
	CHANGE_EVT_MOTION	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EVENT_MOTION),
	CHANGE_EVT_VLOSS	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EVENT_VLOSS),
	CHANGE_EVT_SYS		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_SYSTEM),
	CHANGE_EVT_VCA		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EVENT_VCA),
//	CHANGE_EVT_POSATM	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_TXTIN),

	CHANGE_ACT_RELAY	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_RELAY),				// not used
	CHANGE_ACT_EMAIL	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_EMAIL),				// not used
	CHANGE_ACT_REMOTE	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_REMOTE),				// not used
	CHANGE_ACT_VPOP		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_VPOP),				// not used
	CHANGE_ACT_BUZZ		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_ACT_BUZZ),				// not used

	CHANGE_DISP_TEMP	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DISP_TEMP),				// not used
	CHANGE_DISP_MAINSEQ	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DIS_SEQ),
	CHANGE_DISP_SPOTSEQ	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DISP_SPOT1_SEQ),			// not used
	CHANGE_DISP_MONITOR	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DISPLAY_MONITOR),
	CHANGE_DISP_OSD		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DISPLAY_OSD),
	CHANGE_DISP_LAYOUT	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_LIVELAYOUT),
//	CHANGE_DISP_POSATM	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_DISP_XX),

	CHANGE_AUDIO		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_AUDIO),
	CHANGE_AUD_BUZZ		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_AUDIO_BUZZER),

	CHANGE_USER_ID		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_USER_ID),
	CHANGE_USER_GRP		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_USER_GRP),
	CHANGE_USR_MAN		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_USER_MANAGEMENT),		// not used
	CHANGE_USER_ADD     = _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_USER_ADD),
	CHANGE_USER_DEL     = _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_USER_DEL),

	CHANGE_NET_PROTO	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_NET_PROTO),				// not used
	CHANGE_NET_EMAIL	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_NET_EMAIL),
	CHANGE_NET_IP		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_NETWORK_IP),
	CHANGE_NET_DDNS		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_NETWORK_DDNS),

	CHANGE_SYS_INFO		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_SYSTEM_INFO),			// not used
	CHANGE_SYS_TIME		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_SYSTEM_DATE),
	CHANGE_CTRL_DEV		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_CONTROL_DEVICE),
	CHANGE_SYS_MAN		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_SYSTEM_MANAGEMENT),		// not used
//	CHANGE_SYS_POSATM	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_SYS_TXTIN),

	CHANGE_STRG_OP		= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_STORAGE_OP),
	CHANGE_AUTO_LOGOUT	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_AUTO_LOGOUT),
	CHANGE_PRIVACY_MASK	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_PRIVACY_MASK),
	CHANGE_EMAILSSL_ON	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EMAIL_SSL_ON),
	CHANGE_EMAILSSL_OFF	= _L(LT_SYSTEM_SETUP_CHANGED, LP2_SYSTEM_SETUP_CHANGED_EMAIL_SSL_OFF),

	CHANGE_REC_MODE		= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_MODE),
	CHANGE_CONT_PARAM	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_CONT_PARAM),
	CHANGE_CONT_SCHED	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_CONT_SCHED),
	CHANGE_MOTION_PARAM	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_MOTION_PARAM),
	CHANGE_MOTION_SCHED	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_MOTION_SCHED),
	CHANGE_ALARM_PARAM	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_ALARM_PARAM),
	CHANGE_ALARM_SCHED	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_ALARM_SCHED),
	CHANGE_PANIC_PARAM	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_PANIC_PARAM),
	CHANGE_DUAL_PARAM	= _L(LT_RECORD_SETUP_CHANGED, LP2_RECORD_SETUP_CHANGED_DUAL_PARAM),

	SYSTEM_STARTED		= _L(LT_SYSTEM_STARTED, 0), 
	SYSTEM_SHUTDOWN		= _L(LT_SYSTEM_SHUTDOWN, 0), 
	ABNORMAL_SHUTDOWN	= _L(LT_ABNORMAL_SHUTDOWN_DETECTED, LP2_RBOOT_UNKNOWN), 
	ABNORMAL_CAMERR		= _L(LT_ABNORMAL_SHUTDOWN_DETECTED, LP2_RBOOT_POORCOMM), 
	ABNORMAL_DISKERR	= _L(LT_ABNORMAL_SHUTDOWN_DETECTED, LP2_RBOOT_DISKERR), 
	ABNORMAL_RECERR		= _L(LT_ABNORMAL_SHUTDOWN_DETECTED, LP2_RBOOT_RECORDERR), 
	ABNORMAL_RCVERR		= _L(LT_ABNORMAL_SHUTDOWN_DETECTED, LP2_RBOOT_RECOVERYERR), 
	SYSTEM_RECOVERED	= _L(LT_SYSTEM_RECOVERED, 0), 
	TIME_CHANGED		= _L(LT_SYSTEM_TIME_CHANGED, 0),
	FW_UPGRADE			= _L(LT_SYSTEM_FW_UPGRADE, LP2_FW_UPGRADE_NORMAL),
	FW_UPGRADE_START	= _L(LT_SYSTEM_FW_UPGRADE, LP2_FW_UPGRADE_START),
	FW_UPGRADE_SUCC		= _L(LT_SYSTEM_FW_UPGRADE, LP2_FW_UPGRADE_SUCCESS),
	FW_UPGRADE_FAIL		= _L(LT_SYSTEM_FW_UPGRADE, LP2_FW_UPGRADE_FAIL),
	FORMAT				= _L(LT_SYSTEM_FORMAT, 0),

	ERASE				= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_PRIVACY_FRAME),
	ERASED_RANGE		= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_ERASED_RANGE),

	EMAIL_SENT			= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_EMAIL_SENT),
	ARCHIVE_START		= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_ARCHIVE_START),
	ARCHIVE_END			= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_ARCHIVE_END),
	FACTORY_DEFAULT		= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_FACTORY_DEFAULT),
	DB_SAVE				= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_SYSTEM_DATA_SAVE),
	DB_LOAD				= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_SYSTEM_DATA_LOAD),
	DATA_RESV			= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_DATA_RESERVED),
	REMOVE_DATA_RESV	= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_RESERVED_DATA_REMOVED),
	FAIL_AUTOTIMESYNC	= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_AUTO_TIME_SYNC_FAIL),
	SUCC_AUTOTIMESYNC	= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_AUTO_TIME_SYNC_OK),
	LOCAL_LOGON_FAIL    = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_LOCAL_LOGIN_FAIL),
	REMOTE_MNG_START	= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_REMOTE_MNG_START),
	REMOTE_MNG_STOP		= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_REMOTE_MNG_STOP),
	SYSTEM_DIAGNOSIS	= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_SYSTEM_DIAGNOSIS),
	REMOTE_MNG_KEY		= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_REMOTE_MNG_KEY),
	CABLE_TEST          = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_CABLE_TEST),
	CABLE_TEST_WARN     = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_CABLE_TEST_WARN),
	AUTH_CODE_ISSUED    = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_AUTH_CODE_ISSUED),
	POE_WATT            = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_POE_WATT),
	POE_WATT_TOTAL      = _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_POE_WATT_TOTAL),
	ARM					= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_ARM),
	DISARM				= _L(LT_SYSTEM_EVENT, LP2_SYSTEM_EVENT_DISARM),



	BOOTING_FAIL		= _L(LT_SYSTEM_DEBUG, LP2_SYSTEM_DEBUG_BOOTING_FAIL),
	HDD_REMOVED			= _L(LT_SYSTEM_DEBUG, LP2_SYSTEM_DEBUG_HDD_REMOVED_BY_USER),
	SYSDB_IMPORT		= _L(LT_SYSTEM_DEBUG, LP2_SYSTEM_DEBUG_SYSBD_IMPORT),							// not used
	SYSDB_EXPORT		= _L(LT_SYSTEM_DEBUG, LP2_SYSTEM_DEBUG_SYSBD_EXPORT),							// not used

	CHANGE_TO_RTL		= _L(LT_DISK_EVENT, LP2_DISK_EVENT_SET_RTL),
	CHANGE_TO_OW		= _L(LT_DISK_EVENT, LP2_DISK_EVENT_SET_OVERWRITE),
	CHANGE_TO_ONCE		= _L(LT_DISK_EVENT, LP2_DISK_EVENT_SET_WRITEONCE),

	STORAGE_PLUG        = _L(LT_SYSTEM_SYSLOG, LP2_SYS_STORAGE_PLUG),
	STORAGE_UNPLUG      = _L(LT_SYSTEM_SYSLOG, LP2_SYS_STORAGE_UNPLUG),	
	SYSTEM_RESTART      = _L(LT_SYSTEM_SYSLOG, LP2_SYS_SYSTEM_RESTART),
	
} PUTLOG_TYPE_E;



////////////////////////////////////////////////////////////
//
// public interfaces
//



////////////////////////////////////////////////////////////
// General log

LOGX_T *logx_create(int channel);
int logx_destroy(LOGX_T *logx);
int logx_reset_log_filter(LOGX_T *logx, LF_RESET_E rtype);
int logx_set_log_filter_ch(LOGX_T *logx, int ch, int onoff);
int logx_set_log_filter_type(LOGX_T *logx, unsigned int chmask, LF_CAT_E lcat, int onoff);
int logx_get_log(LOGX_T *logx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log);
int logx_get_log_next(LOGX_T *logx, int count, LOG_DATA_T *log);
int logx_has_log_next_next(LOGX_T *logx, int count);
int logx_get_log_prev(LOGX_T *logx, int count, LOG_DATA_T *log);
int logx_has_log_prev_prev(LOGX_T *logx, int count);
int logx_set_log_filter_order(LOGX_T *logx, LF_ORDER_E order);
int logx_put_log(PUTLOG_TYPE_E type, int param1, int param2, char *text);



////////////////////////////////////////////////////////////
// pos log

TLOGX_T *tlogx_create(int channel);
int tlogx_destroy(TLOGX_T *tlogx);
int tlogx_reset_tlog_filter(TLOGX_T *tlogx);
int tlogx_set_tlog_filter_ch(TLOGX_T *tlogx, int ch, int onoff);
int tlogx_set_tlog_filter_text(TLOGX_T *tlogx, unsigned int *key_info, gboolean match_case, gboolean match_whole);
int tlogx_get_tlog(TLOGX_T *tlogx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log);
int tlogx_get_tlog_next(TLOGX_T *tlogx, int count, LOG_DATA_T *log);
int tlogx_has_tlog_next_next(TLOGX_T *tlogx, int count);
int tlogx_get_tlog_prev(TLOGX_T *tlogx, int count, LOG_DATA_T *log);
int tlogx_has_tlog_prev_prev(TLOGX_T *tlogx, int count);
int tlogx_set_tlog_filter_order(TLOGX_T *tlogx, LF_ORDER_E order);



////////////////////////////////////////////////////////////
// dva log

DLOGX_T *dlogx_create(int channel);
int dlogx_destroy(DLOGX_T *dlogx);
int dlogx_reset_dlog_filter(DLOGX_T *dlogx);
int dlogx_set_dlog_filter_ch(DLOGX_T *dlogx, int ch, int onoff);
int dlogx_set_dlog_filter_algorithm(DLOGX_T *dlogx, guint algorithm);
int dlogx_set_dlog_filter_event(DLOGX_T *dlogx, guint event_mask);
int dlogx_set_dlog_filter_group_mask(DLOGX_T *dlogx, guint group_mask);
//int dlogx_set_dlog_filter_text(DLOGX_T *dlogx, gchar *key_str);
int dlogx_set_dlog_filter_text(DLOGX_T *dlogx, gchar *key_str, gchar *group_str, gchar *gender_str);
int dlogx_get_dlog(DLOGX_T *dlogx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log);
int dlogx_get_dlog_next(DLOGX_T *dlogx, int count, LOG_DATA_T *log);
int dlogx_has_dlog_next_next(DLOGX_T *dlogx, int count);
int dlogx_get_dlog_prev(DLOGX_T *dlogx, int count, LOG_DATA_T *log);
int dlogx_has_dlog_prev_prev(DLOGX_T *dlogx, int count);
int dlogx_set_dlog_filter_order(DLOGX_T *dlogx, LF_ORDER_E order);

#endif 
