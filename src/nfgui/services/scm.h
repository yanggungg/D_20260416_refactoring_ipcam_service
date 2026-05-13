/*
 * scm.h
 * 	- scenario service manager
 *	- dependency :
 *		ix_comm
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */

#ifndef __SCM_H
#define __SCM_H

/*
 * The SCM exist only for a VSM
 *
 */


/*
 * services of the SCM
 *
 *
 * 	1) Device Monitoring
 * 		- sends a notifying message whenever device info is changed.
 * 			INFY_MEDIA_STATUS_CHANGED
 * 		- supplies interface for reading media
 * 			media : wrapping the device info NF_ARCH_DEV_INFO
 * 		- you can use the id what you can get via scm_new_media_list()
 * 		- the ctx scm_new_xxx() is like a new / free functions when you use alloc / free
 * 			so, you must free the pointer that you've got it via the scm_new_xxx() functions.
 *
 *	3) System Time
 *		- changes system time
 *		- applies NTP
 *
 *	4) Firmware Upgrade
 *		- upgrades firmware
 *
 *	5)
 *
 */


#include "iux_msg.h"
#include "cmm.h"
#include "vfs.h"
#include "mda.h"
#include "log.h"
#include "acp.h"
#include "qry.h"
#include "nf_api_ipcam.h"
#include "nf_api_openmode.h"
#include "nf_notify.h"
#include "nf_api_archive.h"
#include "rmt.h"
#include "var.h"
#include "nf_api_disk.h"
#include "amd.h"
#include "nf_util_netif.h"
#include "nf_api_play.h"
#include "nf_util_device.h"
#include "nf_api_raid.h"
#include "nfdal.h"
#include "nf_util_time.h"


////////////////////////////////////////////////////////////////
//
// public definition
//

#define QRCODE_PATH_SEQURINET       "/tmp/sequrinet"
#define QRCODE_EXT_SEQURINET        "png"
#define QRCODE_URL_SIZE             512

////////////////////////////////////////////////////////////////
//
// public data type
//



typedef enum _TLINE_MODE_E {
	TL_LIVE		= 0,
	TL_PLAY,
} TLINE_MODE_E;

typedef enum _REBOOT_REASON_E {
	RR_NA				= 0,
	RR_UNC_ERROR,
	RR_FWUP,
	RR_NODISK,
	RR_DISKERROR,
	RR_DBCHANGE,
	RR_SMART,
	RR_CAM_CHANGE,
	RR_RCVREXPIRED,
	RR_CAM_UPGRADE,
	RR_SIGNAL_CHANGE,
	RR_HD_SPOT_CHANGE,
	RR_INV_BOARD,
	RR_INV_PASSWD,
	RR_WATCHDOG,
	RR_NORMAL_BOOT,
	RR_INSTALLMODE_CHANGE,
	RR_REBOOT_MENU,
	RR_RAID,
	RR_SPOT_CHANGE,
} REBOOT_REASON_E;

typedef enum _DISK_USE_E {
	NOTIN_USE		= 0,
	IN_USE			= 1,
} DISK_USE_E;

typedef enum _ARCH_FORMAT_E {
	ARCH_AVI		= 0,
	ARCH_RAW		= 1,
    ARCH_RAW_EN     = 2,
} ARCH_FORMAT_E;

enum {
	SMART_WRN	= -1,
	SMART_ERR	= -2
};

typedef enum _DISK_VALID_E {
	DISK_INVALID = 0,
	DISK_VALID	= 1
} DISK_VALID_E;

// Disk
typedef struct _DISK_CAP_T {
	DISK_VALID_E	valid;
	guint			id;
	gchar			model[32];
	gchar			serial[32];
	int				exist;
	gboolean		sys_disk;
	guchar			state;
	gchar			reserved[3];
	guint64			size;
	DISK_USE_E		use;
} DISK_CAP_T;

typedef struct _DISK_REC_T {
	DISK_VALID_E	valid;
	time_t			rec_start;
	time_t			rec_end;
} DISK_REC_T;

typedef struct _DISK_SMART_T {
	DISK_VALID_E	valid;

	GTimeVal	update_time;
	guint 		disk_status;

	NF_SMART_ATTR	raw_read_error_rate;
	NF_SMART_ATTR	spin_up_time;
	NF_SMART_ATTR	reallocated_sector_ct;
	NF_SMART_ATTR	seek_error_rate;

	NF_SMART_ATTR	reallocation_event_ct;
	NF_SMART_ATTR	current_pending_sector;
	NF_SMART_ATTR	offline_uncorrectable;

	guint	start_stop_cnt;
	guint	power_on_hours;
	guint	spin_retry_cnt;
	guint	power_cycle_cnt;
	guint	temperature_celsius;					//degree
	guint	temperature_status;
} DISK_SMART_T;

typedef struct _DISK_CAPINFO_T {
	int		 		tdisk_count;
	DISK_CAP_T 		disk_unit[16];
	guint64			tsize;
} DISK_CAPINFO_T;

typedef struct _DISK_RECINFO_T {
	time_t			trec_start;
	time_t			trec_end;
	DISK_REC_T 		disk_unit[16];
} DISK_RECINFO_T;

typedef struct _DISK_SMARTINFO_T {
	DISK_SMART_T	disk_unit[16];
} DISK_SMARTINFO_T;

typedef enum {
	RAID_ALL_MODE = (1 << 0),
	RAID_1_MODE = (1 << 1),
	RAID_5_MODE = (1 << 5),
	RAID_CONF_MODE = (1 << 6)
}RAID_MODE_E;

typedef enum {
	RAID1_CONF_ADD 		= 11,
	RAID1_CONF_DELETE 	= 12,
	RAID5_CONF_ADD 		= 51,
	RAID5_CONF_DELETE 	= 52
}RAID_CONF_MODE_E;

typedef struct _SATA_INFO_T {
	gchar model_name[MAX_DISK_IN_RAID][41];
	guint capacity[MAX_DISK_IN_RAID];
}SATA_INFO_T;

enum {
	RAID_CONF_FAIL = -1,
	RAID_CONF_SUCCESS = 0,
	RAID_CONFIGURING,
	RAID_CONF_FINISH,
};

typedef struct _DISK_RAID_T {
	gint		raid_id;
	guchar		raid_mode;
	guchar 		member_count;
	gchar		model[MAX_DISK_IN_RAID][41];
	guint		capacity;
	gint		unused[MAX_DISK_IN_RAID];
	guchar		status;
	gint 		rebuild;
	gint		sata_index[MAX_DISK_IN_RAID];
	gint		sata_capacity[MAX_DISK_IN_RAID];
}DISK_RAID_T;

typedef struct _DISK_RAIDINFO_T {
	guchar			mode;
	gint			raid_cnt;
	DISK_RAID_T 	rinfo[MAX_RAID_IN_CONTROLLER];
}DISK_RAIDINFO_T;

typedef struct _DISK_DB_T {
	guchar added_disks;
	guchar removed_disks;

	DISK_CAPINFO_T		cap[2];
	DISK_RECINFO_T		rec[2];
	DISK_SMARTINFO_T 	smart[2];
	DISK_RAIDINFO_T     raid;
} DISK_DB_T;

typedef enum _REBOOT_INFO_E {
	REB_NONE			= 0,
	REB_CAM_ERROR		= 1,
	REB_DISK_ERROR		= 2,
	REB_REC_ERROR		= 3,
	REB_RECOVERY_ERROR	= 4,
	REB_NORMAL_BOOT		= 99
} REBOOT_INFO_E;

typedef struct _REBOOT_INFO_CONT {
	char content[256];
} REBOOT_INFO_CONT;

typedef enum _BRN_CODE_E {
	BRN_SUCCESS							= NF_ARCH_ERR_NONE,
	BRN_CODE_INV_COMMAND				= -NF_ARCH_ERR_INVMODE,
	BRN_CODE_INV_DEV					= -NF_ARCH_ERR_INVDEV,
	BRN_CODE_INV_PARAM					= -NF_ARCH_ERR_INVPARAM,
	BRN_CODE_INV_MEDIA					= -NF_ARCH_ERR_INVMEDIA,
	BRN_CODE_FULL_MEDIA					= -NF_ARCH_ERR_MEDIA_FULL,
	BRN_CODE_NOTERASABLE_MEDIA			= -NF_ARCH_ERR_MEDIA_NOTERASABLE,
	BRN_CODE_FAIL						= -NF_ARCH_ERR_FAIL,
	BRN_CODE_FAIL_WRITING				= -NF_ARCH_ERR_FAIL_DEVERR,
	BRN_CODE_NEXT_MEDIA					= -NF_ARCH_ERR_REQMORE,
	BRN_CODE_CANCELED					= -NF_ARCH_ERR_CANCELED,
	BRN_CODE_FTP_CONN					= -NF_ARCH_ERR_FTP_CONN,
	BRN_CODE_FTP_AUTH					= -NF_ARCH_ERR_FTP_AUTH,
	BRN_CODE_FTP_FAIL					= -NF_ARCH_ERR_FTP_FAIL,
	BRN_CODE_FAIL_MULTISESSION			= -NF_ARCH_ERR_MULTISESSION_ERROR,
	BRN_CODE_NOTSUPP_MULTISESSION		= -NF_ARCH_ERR_DISABLE_MULTISESSION,
	BRN_CODE_UNKNOWN_BUG				= -NF_ARCH_ERR_DATA_OR_BUG,
} BRN_CODE_E;

typedef enum _RSV_CODE_E {
	RSV_SUCCESS							= NF_ARCH_ERR_NONE,
	RSV_CODE_INV_COMMAND				= -NF_ARCH_ERR_INVMODE,
	RSV_CODE_INV_DEV					= -NF_ARCH_ERR_INVDEV,
	RSV_CODE_INV_PARAM					= -NF_ARCH_ERR_INVPARAM,
	RSV_CODE_INV_MEDIA					= -NF_ARCH_ERR_INVMEDIA,
	RSV_CODE_FAIL						= -NF_ARCH_ERR_FAIL,
	RSV_CODE_EMPTY_LIST					= -NF_ARCH_ERR_LISTEMPTY,
	RSV_CODE_FULL_LIST					= -NF_ARCH_ERR_LISTFULL,
	RSV_CODE_FAIL_LOCK					= -NF_ARCH_ERR_DATALOCKED,
} RSV_CODE_E;

typedef enum _FTP_CODE_E {
	FTP_SUCCESS							= NF_ARCH_ERR_NONE,
	FTP_CODE_CONN						= -NF_ARCH_ERR_FTP_CONN,
	FTP_CODE_AUTH						= -NF_ARCH_ERR_FTP_AUTH,
	FTP_CODE_FAIL						= -NF_ARCH_ERR_FTP_FAIL,
} FTP_CODE_E;

typedef enum _DISK_CONF_E {
	DC_ERROR			= -1,
	DC_NOCHANGE			= 0,
	DC_NODISK			= 1,
	DC_CONFLICTED		= 2,
	DC_ADDED			= 3,
	DC_REMOVED			= 4,
	DC_ADDED_N_REMOVED	= 5,
	DC_NEED_FORMAT_ALL	= 6,
	DC_FORMAT_N_RCVR	= 7,
	DC_ENFORCE_FORMAT	= 8,
	DC_SYSTEM_REMOVED	= 9,
} DISK_CONF_E;

typedef enum _CONFIRM_E {
	CONFIRM_NONE		= 0,
	CONFIRM_OK			= 1,
} CONFIRM_E;

typedef enum _DISK_CONFIRM_E {
	DISK_NONE			= 0,
	DISK_OK				= 1,
	DISK_SYNC			= 2,
	DISK_NOSYNC			= 3,
	DISK_FORMAT			= 4,
	DISK_NOFORMAT		= 5,
	DISK_REBOOT			= 6,
	DISK_DELRAID		= 7,
} DISK_CONFIRM_E;

#if defined(__ITXGUI64)
typedef unsigned long int LOGCTX;
typedef unsigned long int TLOGCTX;
typedef unsigned long int DLOGCTX;
#else
typedef unsigned int LOGCTX;
typedef unsigned int TLOGCTX;
typedef unsigned int DLOGCTX;
#endif

typedef struct _RESOL_INFO_T {
	guint64 cap;
	guint64 cur;
} RESOL_INFO_T;

typedef struct _FPS_INFO_T {
	guint cap;
	guint cur;
} FPS_INFO_T;

typedef struct _CAM_PROFILE_T {
	bool 				connected;
	RESOL_INFO_T		resol;
	FPS_INFO_T			fps;
	NFIPCamModelInfo 	model;
	NFIPCamConfInfo		conf;
} CAM_PROFILE_T;

typedef struct _CAM_ENCODER_T {
	guint64             cap[3];
	guint64             cur[3];
    NFIPCamEncoderCap   capinfo;
} CAM_ENCODER_T;

typedef struct _AUDIO_INFO_T {
	bool 	audio_on;
	bool	audio_has;
	int		codec;
} AUDIO_INFO_T;

typedef enum _ERROR_CODE_E {
	ER_NONE				= 0,
	ER_COMMON			= -1,
	ER_VFS_STOPPING		= -2,
	ER_VFS_STARTING		= -3,
	ER_DATA_DELETING	= -4,
	ER_FORMATTING		= -5,
	ER_RTL_SETTING		= -6,
	ER_VERIFYING		= -7,
	ER_DB_LOADING		= -8,
	ER_DB_APPLYING		= -9,
	ER_DB_FACTORYING	= -10,
	ER_UNC_CHECK		= -11,
	ER_SMART			= -12,
	ER_RECOVERY			= -13,
	ER_ERASE_CH			= -14,
} ERROR_CODE_E;

typedef struct _FTP_INFO_T {
	char	server[255 + 1];
	int		port;
	char	user[63 + 1];
	char	passwd[63 + 1];
} FTP_INFO_T;

typedef enum _CAP_RES_E {
	CAP_RES_NTSC_CIF 	= 0,
	CAP_RES_NTSC_2CIF 	= 1,
	CAP_RES_NTSC_4CIF 	= 2,
	CAP_RES_NTSC_4CIFP 	= 3,
	CAP_RES_PAL_CIF 	= 4,
	CAP_RES_PAL_2CIF 	= 5,
	CAP_RES_PAL_4CIF 	= 6,
	CAP_RES_PAL_4CIFP 	= 7,
	CAP_RES_640x480 	= 8,
	CAP_RES_720x480 	= 9,
	CAP_RES_720x576 	= 10,
	CAP_RES_800x600 	= 11,
	CAP_RES_1024x768 	= 12,
	CAP_RES_1280x1024 	= 13,
	CAP_RES_1600x1200 	= 14,
	CAP_RES_1280x720 	= 15,
	CAP_RES_1920x1080 	= 16,
	CAP_RES_640x352 	= 17,
	CAP_RES_640x360 	= 18,
	CAP_RES_640x360I 	= 19,
	CAP_RES_1280x720I 	= 20,
	CAP_RES_1920x1080I 	= 21,
	CAP_RES_640x400 	= 22,
	CAP_RES_800x450 	= 23,
	CAP_RES_1440x900 	= 24,
	CAP_RES_960x480 	= 25,
	CAP_RES_960x576 	= 26,
	CAP_RES_320x180 	= 27,
	CAP_RES_2304x1296 	= 28,
	CAP_RES_2048x1536 	= 29,
	CAP_RES_2560x1440 	= 30,
	CAP_RES_2688x1520 	= 31,
	CAP_RES_2560x1600 	= 32,
	CAP_RES_2560x1920 	= 33,
	CAP_RES_2592x1920 	= 34,
	CAP_RES_2592x1944 	= 35,
	CAP_RES_2992x1680 	= 36,
	CAP_RES_2880x1800 	= 37,
	CAP_RES_3200x1800 	= 38,
	CAP_RES_2880x2160 	= 39,
	CAP_RES_3072x2048 	= 40,
	CAP_RES_3200x2400 	= 41,
	CAP_RES_3840x2160 	= 42,
	CAP_RES_2592x1520 	= 43,
	CAP_RES_3000x3000 	= 44,	
	CAP_RES_2048x2048 	= 45,	
	CAP_RES_1280x1280 	= 46,	
	CAP_RES_640x640 	= 47,	
	CAP_RES_320x320 	= 48,	
	CAP_RES_UNKNOWN,
	CAP_RES_MAX
} CAP_RES_E;

typedef enum _CAP_FPS_E {
	CAP_FPS_30 			= 0,
	CAP_FPS_25 			= 1,
	CAP_FPS_15 			= 2,
	CAP_FPS_12 			= 3,
	CAP_FPS_07 			= 4,
	CAP_FPS_06 			= 5,
	CAP_FPS_04 			= 6,
	CAP_FPS_03 			= 7,
	CAP_FPS_02 			= 8,
	CAP_FPS_01 			= 9,
	CAP_FPS_00 			= 10,

	CAP_FPS_UNKNOWN,
	CAP_FPS_MAX
} CAP_FPS_E;

typedef struct _CAPTURE_IMAGE_T {
	gint 	ch;
	time_t 	time;
	int 	width;
	int 	height;
	int 	size;
	void 	*buffer;
} CAPTURE_IMAGE_T;

typedef enum _CONNENT_CAM_E {
	CONNENT_CAM_SD			= 0,
	CONNENT_CAM_HDSDI		= 1,
} CONNENT_CAM_E;

typedef struct _MOTION_PROF_T {
	gint ch;
	gint sensitivity_min;
	gint sensitivity_max;
	gint block_width;
	gint block_height;
	gint min_block;
	gint num_blocks;
	gint area_method;
	gint rect_num;
} MOTION_PROF_T;

typedef enum _STRG_TYPE_E {
	INTERNAL	= 0,
	EXTERNAL	= 1
} STRG_TYPE_E;

typedef struct _DDNS_INFO_T {
	gchar server[STRING_SIZE_32];
	gchar hostname[STRING_SIZE_64];
	gchar id[STRING_SIZE_64];
	gchar pwd[STRING_SIZE_64];
} DDNS_INFO_T;

typedef enum _FUJIKODNS_RET_MSG {
	FUJIKODNS_RES_OK              = 1,
	FUJIKODNS_RES_BADAUTH_ERROR   = 23,
	FUJIKODNS_RES_MAX,
}FUJIKODNS_RET_MSG;

typedef enum _NET_FW_STATE{
	NET_FW_READY         = 0,
	NET_FW_NOTSUPPORT    = 1,
	NET_FW_REQ_UPDATE    = 2,
	NET_FW_REJECT        = 3,
	NET_FW_APPLY         = 4,
	NET_FW_UPDATE        = 5,
	NET_FW_ERROR         = 6,
	NET_FW_FINISH        = 7
}NET_FW_STATE;

typedef struct _NET_FW_UPDATE{
	guint type;
	guint rate;
	gpointer *data;
}NET_UPDATE_STATE;

typedef void(*get_update_state)(NET_UPDATE_STATE *data);

typedef struct _NET_UPDATE_DATA{
	gchar url[1024];
	gchar filename[256];
	get_update_state cb_func;
	gpointer            cb_data;
}NET_UPDATE_DATA;


typedef enum _WEBMSG_TO_VW{
	WEBMSG_PASS_INIT        = 0,
} WEBMSG_TO_VW;

typedef struct _MAIL_SERVER_T {
    char            server_name[64];
    int             port;
    int             security;
    char            user[64];
    char            passwd[32];
} MAIL_SERVER_T;

typedef struct _MAIL_CONTENT_T {
    char            from[64];
    char            to[64];
    char            subject[256];
    char            contents[4096];
} MAIL_CONTENT_T;

typedef struct _SMS_SERVER_T {
	char	server[128];
	char	user[128];
	char	password[128];
	char	appid[128];
} SMS_SERVER_T;

typedef struct _SMS_RECEIVER_T {
	char	number[128];
} SMS_RECEIVER_T;

typedef struct  _DETECT_VERINFO_T
{
	gint need;
	gchar model[128];
	gchar new_fwver[128];
	gchar importance[128];
	gchar url[1024];
	gchar infor_general[1024];
	gchar infor_fix[1024];
	gchar infor_func[1024];
	gchar reference_link[1024];
} DETECT_VERINFO_T ;


typedef enum _DIAG_TYPE_E {
	DIAG_NONE			= 0,
	DIAG_IPCAM_NET		= 1,
	DIAG_IPCAM_POWER	= 2,
	DIAG_DISK			= 3,
	DIAG_SERVICE_PORT	= 4,
} DIAG_TYPE_E;

typedef enum _DIAG_RES_E {
	DIAG_RES_CHECK		= -2,
	DIAG_RES_ERROR		= -1,
	DIAG_RES_NA			= 0,
	DIAG_RES_SUCCESS		= 1,
} DIAG_RES_E;

typedef struct _DIAG_RES_T {
	DIAG_TYPE_E type;
	DIAG_RES_E ipcam_net[16];
	DIAG_RES_E ipcam_power[16];
	DIAG_RES_E internal_disk[5];
	DIAG_RES_E external_disk[5];
	DIAG_RES_E web_port;
	DIAG_RES_E rtsp_port;
} DIAG_RES_T;

typedef struct _CHKCABLE_RES_T {
    guint   device;
    guint   cable_num;
    int     port;
    int     length;
    int     result;
} CHKCABLE_RES_T;

typedef struct _APPQC_RES_T {
    int alarm;
    int audio;
    int network;
    int rs485;
    int fan;
    int temper;
    int poe;
    int rs232;
    GTimeVal rtc;
} APPQC_RES_T;

typedef struct _SEARCH_KEY_T {
    gint oper[MAX_STR_OPER_CNT];
    gchar keyword[MAX_STR_NUM_CNT][64];
} SEARCH_KEY_T;

typedef struct _LICENSE_KEY_T {
	NF_SYSDB_LICENSE_INFO lic_info[MAX_LICENSE_CNT];
	int key_count;
} LICENSE_KEY_T;

typedef struct _ANALYTICS_ADDITIONAL_EVENT_T {
	GTimeVal timestamp;
	char caption[128];
	char title[128];
	char description[512];
	void *jpeg_buff;
	int jpeg_len;
} ANALYTICS_ADDITIONAL_EVENT_T;

typedef void* (*DUPL_PROC)(void *data);

#ifndef VIEWER_E
#define VIEWER_E
typedef enum _VIEWER_E {
	VIEWER_LOCAL		= 0,
	VIEWER_WEB			= 1
} VIEWER_E;
#endif

typedef enum _GPU_MODE_E {
	GPU_NONE			= 0,
	GPU_FISHEYE			= 1,
	GPU_DLVA			= 2,
} GPU_MODE_E;

enum {
	FWSTEP_128MB_PREPARE_NONE = 0,
    FWSTEP_128MB_PREPARE_REQ_FWUP,
    FWSTEP_128MB_PREPARE_VALIDATE,
    FWSTEP_128MB_PREPARE_DATABACKUP,
    FWSTEP_128MB_PREPARE_COMPLETE,
    FWSTEP_128MB_REBOOT_TO_UBOOTUP,
};

typedef struct _SSL_CERT_INFO_T
{
	char issuer[128];
	char subject[64];
	char from[32];
	char to[32];
} SSL_CERT_INFO_T;

typedef enum _CERT_TYPE_E 
{
	CERT_TYPE_CA = 0,
	CERT_TYPE_CLIENT,
	CERT_TYPE_CLIENT_KEY,
	
	CERT_TYPE_CNT
} CERT_TYPE_E;


////////////////////////////////////////////////////////////////
//
// public interfaces
//


IUXAPI int scm_init();
IUXAPI int scm_cleanup();
IUXAPI /*inline*/ CMMPORT scm_get_cmmport();
IUXAPI int scm_put_message(IMSG msgid, int param, bool dyn_data, void *data);

IUXAPI int scm_update_design(const char *dirname, int vendor, IMSG ret_msg);
IUXAPI int scm_run_factory_default(IMSG ret_msg);

// request system statue
IUXAPI int scm_req_analog_data();
IUXAPI int scm_req_ipcam_data();
IUXAPI int scm_req_vloss_data();
IUXAPI int scm_req_novideo_data();
IUXAPI int scm_req_net_status_data();
IUXAPI int scm_req_net_rxtx_status_data();
IUXAPI int scm_req_disk_full_data();
IUXAPI int scm_req_disk_usage_data();
IUXAPI int scm_req_sensor_event_data();
IUXAPI int scm_req_alarm_event_data();
IUXAPI int scm_req_motion_event_data();
IUXAPI int scm_req_pnd_event_data();
IUXAPI int scm_req_all_pnd_event_data();
IUXAPI int scm_req_wan_status_data();
IUXAPI int scm_req_ddns_status_data();
IUXAPI int scm_req_writefail_event_data();
IUXAPI int scm_req_exhaust_event_data();
IUXAPI int scm_req_nodisk_event_data();
IUXAPI int scm_req_smart_event_data();
IUXAPI int scm_req_sysfan_event_data();
IUXAPI int scm_req_termperature_event_data();
IUXAPI int scm_req_poe_event_data();
IUXAPI int scm_req_dvrloginfail_event_data();
IUXAPI int scm_req_netloginfail_event_data();
IUXAPI int scm_req_audio_ch_event_data();
IUXAPI int scm_req_mic_out_event_data();
IUXAPI int scm_req_disk_ow_event_data();
IUXAPI int scm_req_disk_smart_event_data();
IUXAPI int scm_req_disk_raid_event_data();
IUXAPI int scm_req_pnd_hub_event_data();
IUXAPI int scm_req_ipcam_install_event_data();
IUXAPI int scm_get_analog_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_ipcam_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_vloss_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_novideo_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_net_status_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_disk_full_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_disk_usage_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_vloss_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_sensor_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_motion_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_alarm_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_pnd_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_wan_status_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_ddns_status_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_writefail_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_exhaust_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_nodisk_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_smart_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_smart_reqchk_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_smart_disk_raid_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_sysfan_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_termperature_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_poe_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_dvrloginfail_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_netloginfail_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_audio_ch_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_mic_out_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_disk_ow_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_ai_keep_alive_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_notify_to_system(const gchar *property_name, guint param);
IUXAPI int scm_inform_to_system(const gchar *property_name, guint param);
IUXAPI int scm_notify_to_viewer(WEBMSG_TO_VW msg);
IUXAPI int scm_get_pnd_hub_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_ipcam_install_event_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_get_net_rxtx_status_data(NF_NOTIFY_INFO *buf);
IUXAPI int scm_regi_password_to_hdd();


// System
IUXAPI int scm_bootup_system(IMSG ret_msg);
IUXAPI int scm_reboot_system(REBOOT_REASON_E reason, int after_msec);
IUXAPI int scm_cancel_reboot();
IUXAPI int scm_start_unc_checking();
IUXAPI int scm_stop_unc_checking();
IUXAPI int scm_shutdown_system(IMSG ret_msg);
IUXAPI int scm_restart_service(IMSG ret_msg, RS_SRVSTOP_E rs);
IUXAPI int scm_enter_cam_upgrade_mode(IMSG ret_msg);
IUXAPI int scm_buzzer_on();
IUXAPI int scm_buzzer_off();
IUXAPI int scm_cntl_beep_irda();
IUXAPI gboolean scm_get_video_resolution(gchar buf[32]);
IUXAPI gboolean scm_get_video_output(gchar buf[32]);
IUXAPI int scm_get_hw_ver(char *buf, int len);
IUXAPI int scm_get_poe_port_info(NF_UTIL_POE_PORT_INFO *info);
IUXAPI int scm_enter_direct_mode(IMSG ret_msg, int ch);
IUXAPI int scm_leave_direct_mode(IMSG ret_msg);
IUXAPI int scm_enter_openmode_install();
IUXAPI int scm_leave_openmode_install();
IUXAPI int scm_is_qc_mode();
IUXAPI int scm_is_clon_device();
IUXAPI int scm_reload_disk_info();
IUXAPI int scm_reboot_by_network();
IUXAPI int scm_turn_on_led_all();
IUXAPI int scm_turn_off_led_all();
IUXAPI int scm_turn_on_led_all_qc();
IUXAPI int scm_turn_off_led_all_qc();


IUXAPI int scm_get_gpu_mode_function();
IUXAPI int scm_set_gpu_mode_function(GPU_MODE_E mode);

IUXAPI int scm_dlva_detector_live_start_chmask(guint chmask);
IUXAPI int scm_dlva_detector_setup_open_channel(guint ch);
IUXAPI int scm_dlva_detector_setup_close_channel(guint ch);
IUXAPI int scm_reset_system();
IUXAPI int scm_export_debug_data(IMSG ret_msg, char *usb_path);


// Video Filesystem
IUXAPI int scm_restart_videofs(IMSG ret_msg);
IUXAPI int scm_get_disk_capinfo(STRG_TYPE_E type, DISK_CAPINFO_T *cap_info);
IUXAPI int scm_get_disk_recinfo(STRG_TYPE_E type, DISK_RECINFO_T *rec_info);
IUXAPI int scm_get_disk_smartinfo(STRG_TYPE_E type, DISK_SMARTINFO_T *smart_info);
IUXAPI int scm_format_storage(IMSG ret_msg);
IUXAPI int scm_start_unc_checking(IMSG ret_msg);
IUXAPI int scm_get_prev_unc_error();
IUXAPI int scm_stop_unc_checking();
IUXAPI int scm_get_disk_count();
IUXAPI int scm_erase_ch(BITMASK chmask, time_t start, time_t end, IMSG ret_msg);
IUXAPI int scm_get_disk_info(IMSG ret_msg);
IUXAPI int scm_stop_fs_urgent();




// MEDIA services
IUXAPI MEDIA_INFO_T *scm_new_media_list(int *ret_cnt);
IUXAPI int scm_free_media_list(MEDIA_INFO_T *minfo);
IUXAPI int scm_get_media_count();

IUXAPI int scm_is_mounted_media(MEDIA_ID id);
IUXAPI int scm_get_mounted_path(MEDIA_ID id, char *path, int path_len);
IUXAPI MEDIA_TYPE_E scm_get_media_type(MEDIA_ID id);
IUXAPI int scm_get_device_id(MEDIA_ID id);


// FW list
IUXAPI char **scm_new_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt);
IUXAPI int scm_free_fw_list(char **fw_list);
IUXAPI int scm_get_fw_count(MEDIA_ID id, FWFILE_FILTER_E filter);
IUXAPI int scm_spy_odd_name(char *odd_name);


// dir-file list
IUXAPI char **scm_new_dir_list(MEDIA_ID id, int *ret_cnt);
IUXAPI int scm_free_dir_list(char **dir_list);
IUXAPI char **scm_new_file_list(MEDIA_ID id, char *file_ext, int *ret_cnt);
IUXAPI int scm_free_file_list(char **file_list);


// DB load / save
IUXAPI char **scm_new_db_list(MEDIA_ID id, int *ret_cnt);
IUXAPI char **scm_new_audio_list(MEDIA_ID id, int *ret_cnt);
IUXAPI int scm_import_db(char *full_path, IMSG ret_msg);
IUXAPI int scm_export_db(char *full_path);


// Date / Time
IUXAPI int scm_is_past_time_set();
IUXAPI int scm_set_system_time(GTimeVal *tv);
IUXAPI int scm_restore_system_time();
IUXAPI int scm_apply_timezone(int idx, NF_TIME_UDD *udd);
IUXAPI int scm_is_set_new_time();
IUXAPI int scm_apply_time(IMSG ret_msg);
IUXAPI int scm_change_time(time_t target, time_t command, IMSG ret_msg);
IUXAPI int scm_get_ntp_time(char *server_name, GTimeVal *ret);
IUXAPI int scm_init_timesync();



// FW Upgrade
IUXAPI int scm_upgrade_fw(const char *filename, IMSG ret_msg);
IUXAPI int scm_upgrade_fw_network(const char *update_url, const char *filename, get_update_state update_state, gpointer data);
IUXAPI int scm_upgrade_ipcam_fw(guint ch_mask, const char *filename, IMSG ret_msg);

IUXAPI int scm_prepare_upgrade_validate_check(const char *full_name, IMSG ret_msg);
IUXAPI int scm_prepare_upgrade_validate_check_url(const char *url, IMSG ret_msg);
IUXAPI int scm_prepare_upgrade_data_backup(const gchar *mnt_path, IMSG ret_msg);
IUXAPI int scm_prepare_remote_upgrade_allinone(const char *fw_path, const gchar *backup_path);
IUXAPI int scm_get_fw_upgrade_state(int *state, int *rate);
IUXAPI int scm_get_fw_upgrade_rate();
IUXAPI int scm_get_128MB_fw_upgrate_step();


// NET

IUXAPI int scm_apply_netinfo_by_db(IMSG ret_msg);
IUXAPI int scm_register_rtsp_port(int port, IMSG ret_msg);
IUXAPI int scm_remove_rtsp_port(int port, IMSG ret_msg);
IUXAPI int scm_test_rtsp_port(IMSG ret_msg);
IUXAPI int scm_apply_ipfilter();
IUXAPI int scm_test_sms(SMS_SERVER_T *serverinfo, SMS_RECEIVER_T *receiverinfo);
IUXAPI int scm_onestop_test();


// Alias text will be received by data field of return message(CMM_MESSAGE_T)
IUXAPI int scm_register_web_port(int port, IMSG ret_msg);

IUXAPI int scm_remove_web_port(int port, IMSG ret_msg);
IUXAPI int scm_test_web_port(IMSG ret_msg);

IUXAPI int scm_register_ddns(IMSG ret_msg);
IUXAPI int scm_register_ddns_with_info(DDNS_INFO_T *info, IMSG ret_msg);
IUXAPI int scm_test_ddns(IMSG ret_msg);

IUXAPI int scm_test_email(MAIL_SERVER_T *server, MAIL_CONTENT_T *content);

// IP addr will be received by data field of return message(CMM_MESSAGE_T)
IUXAPI int scm_req_wan_ip(IMSG ret_msg);

IUXAPI REMOTE_USER_T *scm_new_remote_user_list(int *ret_cnt);
IUXAPI int scm_free_remote_user_list(REMOTE_USER_T *list);
IUXAPI SESSION_LIST_T *scm_new_net_session_list(int *ret_cnt);
IUXAPI int scm_free_net_session_list(SESSION_LIST_T *list);
IUXAPI int scm_get_net_session_count();
IUXAPI int scm_get_remote_user_info(char *user_id, REMOTE_USER_T *info);
IUXAPI int scm_is_connected_remote_user(char *user_id);
IUXAPI int scm_disconnect_remote_user(char *user_id);
IUXAPI int scm_disconnect_session(SSID id);
IUXAPI int scm_req_test_ftp(FTP_INFO_T *finfo, IMSG ret_msg);
IUXAPI FTP_CODE_E scm_test_ftp(FTP_INFO_T *finfo);
IUXAPI int scm_restart_rtsp();
IUXAPI int scm_get_sys_netinfo(NF_NETIF_GET_INFO *info);
IUXAPI int scm_get_mac_addr_str(char *buf, int buf_len);
IUXAPI int scm_get_dvr_addr_str(char *addr, int addr_len);
IUXAPI int scm_replace_host_in_addr(char *host_name, char *addr, int addr_len);
IUXAPI int scm_get_dvr_name(char *name, int name_len);
IUXAPI int scm_get_ip_addr_str(char *addr, int addr_len);
IUXAPI int scm_get_compressed_ipv6_addr(char *in_addr, char *out_addr);
IUXAPI int scm_is_wan_connected();
IUXAPI char **scm_new_ssl_dcv_list(MEDIA_ID id, int *ret_cnt);
IUXAPI char **scm_new_ssl_icrt_list(MEDIA_ID id, int *ret_cnt);
IUXAPI char **scm_new_ssl_pem_list(MEDIA_ID id, int *ret_cnt);
IUXAPI char **scm_new_ssl_crt_list(MEDIA_ID id, int *ret_cnt);
IUXAPI char **scm_new_ssl_key_list(MEDIA_ID id, int *ret_cnt);
IUXAPI int scm_free_ssl_list(char **ssl_list);
IUXAPI int scm_check_private_key_pass(char *key_path);
IUXAPI int scm_ssl_icrt_install(char *icrt_path, IMSG ret_msg);
IUXAPI int scm_ssl_install_certificate(char cert_path[][512], char *pri_pass, IMSG ret_msg);
IUXAPI int scm_ssl_self_signed_install(IMSG ret_msg);
IUXAPI int scm_ssl_delete_certificate(IMSG ret_msg);
IUXAPI int scm_ssl_get_cert_info(SSL_CERT_INFO_T *info);
IUXAPI int scm_is_private_key(char *key_path);
IUXAPI int scm_is_public_key(char *key_path);
IUXAPI int scm_ssl_install_dcv_file(char *path);
IUXAPI int scm_ssl_is_exist_certificate();
IUXAPI int scm_8021x_upload_cert_temp(char *fpath, CERT_TYPE_E cert_type);
IUXAPI int scm_8021x_apply_cert();
IUXAPI int scm_8021x_cancel_cert();
IUXAPI int scm_8021x_get_cert_filename(CERT_TYPE_E cert_type, char *fname, int buf_size);
IUXAPI int scm_8021x_get_cert_info(CERT_TYPE_E cert_type, SSL_CERT_INFO_T *info);
IUXAPI int scm_8021x_delete_cert(CERT_TYPE_E cert_type);
IUXAPI int scm_8021x_check_changed();


// LOG
IUXAPI LOGCTX scm_open_log_ctx();
IUXAPI int scm_close_log_ctx(LOGCTX logctx);
IUXAPI int scm_reset_log_filter(LOGCTX logctx, LF_RESET_E rtype);
IUXAPI int scm_set_log_filter_ch(LOGCTX logctx, int ch, int onoff);
IUXAPI int scm_set_log_filter_type(LOGCTX logctx, unsigned int chmask, LF_CAT_E lcat, int onoff);
IUXAPI int scm_set_log_filter_order(LOGCTX logctx, LF_ORDER_E order);
IUXAPI int scm_get_log(LOGCTX logctx, GTimeVal *older, GTimeVal *later, int count, LOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_log_next(LOGCTX logctx, int count, LOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_log_prev(LOGCTX logctx, int count, LOG_DATA_T *log, gboolean *prev);

IUXAPI int scm_put_log(PUTLOG_TYPE_E type, int param1, int param2);
IUXAPI int scm_put_log_t(PUTLOG_TYPE_E type, int param1, int param2, char *text);

IUXAPI TLOGCTX scm_open_tlog_ctx();
IUXAPI int scm_close_tlog_ctx(TLOGCTX tlogctx);
IUXAPI int scm_reset_tlog_filter(TLOGCTX tlogctx);
IUXAPI int scm_set_tlog_filter_ch(TLOGCTX tlogctx, int ch, int onoff);
IUXAPI int scm_set_tlog_filter_text(TLOGCTX tlogctx, SEARCH_KEY_T *key_info, gboolean match_case, gboolean match_whole);
IUXAPI int scm_set_tlog_filter_order(TLOGCTX tlogctx, LF_ORDER_E order);
IUXAPI int scm_get_tlog(TLOGCTX tlogctx, GTimeVal *older, GTimeVal *later, int count, TLOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_tlog_next(TLOGCTX tlogctx, int count, TLOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_tlog_prev(TLOGCTX tlogctx, int count, TLOG_DATA_T *log, gboolean *prev);

IUXAPI DLOGCTX scm_open_dlog_ctx();
IUXAPI int scm_close_dlog_ctx(DLOGCTX dlogctx);
IUXAPI int scm_reset_dlog_filter(DLOGCTX dlogctx);
IUXAPI int scm_set_dlog_filter_ch(DLOGCTX dlogctx, int ch, int onoff);
IUXAPI int scm_set_dlog_filter_algorithm(DLOGCTX dlogctx, guint algorithm);
IUXAPI int scm_set_dlog_filter_event(DLOGCTX dlogctx, guint event_mask);
IUXAPI int scm_set_dlog_filter_text(DLOGCTX dlogctx, char *key_str, gchar *group_str, gchar *gender_str);
IUXAPI int scm_set_dlog_filter_enable(DLOGCTX dlogctx, char *key_str, guint name_search, guint group_search, guint gender_search);
IUXAPI int scm_set_dlog_filter_order(DLOGCTX dlogctx, LF_ORDER_E order);
IUXAPI int scm_get_dlog(DLOGCTX dlogctx, GTimeVal *older, GTimeVal *later, int count, LOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_dlog_next(DLOGCTX dlogctx, int count, LOG_DATA_T *log, gboolean *next);
IUXAPI int scm_get_dlog_prev(DLOGCTX dlogctx, int count, LOG_DATA_T *log, gboolean *prev);


// Timeline
IUXAPI int scm_get_recinfo(int year, int month, BITMASK64 chmask, gchar buf[31]);
IUXAPI int scm_get_timeline(NF_TIMELINE_PARAM *param, gchar **elem);


// Query, Bookmark
IUXAPI QRY_CODE_E scm_start_bookmark(NF_ARCH_PB_AVI_PARAM param);
IUXAPI int scm_pause_bookmark();
IUXAPI int scm_resume_bookmark();
IUXAPI QRY_CODE_E scm_stop_bookmark();
IUXAPI int scm_exit_bookmark();
IUXAPI int scm_request_bookmark_info();
IUXAPI RSV_CODE_E scm_reserve_bookmark_info();
IUXAPI QRY_CODE_E scm_start_avi_query(NF_ARCH_AVI_PARAM *avi_param);
IUXAPI int scm_start_snapshot_query(NF_ARCH_SNAP_PARAM *snap_param, NF_ARCH_SNAP_INFO *snap_info);
IUXAPI int scm_end_query();
IUXAPI RSV_CODE_E scm_reserve_avi();
IUXAPI gboolean scm_is_bookmarking();
IUXAPI RSV_CODE_E scm_reserve_snap_info();
//

// Export
IUXAPI int scm_start_burning(NF_ARCH_TYPE_E type, guint16 arch_id, MEDIA_ID mid, gboolean isErase);
IUXAPI int scm_end_burning();
IUXAPI int scm_cancel_burning();
IUXAPI int scm_get_avi_reserved_data(guint16 arch_id, guint16 count, NF_ARCH_AVI_INFO *data);
IUXAPI int scm_get_snap_reserved_data(guint16 arch_id, guint16 count, NF_ARCH_SNAP_INFO *data);
IUXAPI int scm_get_reserved_data_count(NF_ARCH_TYPE_E type);
IUXAPI int scm_delete_reserved_data(NF_ARCH_TYPE_E type, guint16 arch_id);
IUXAPI int scm_set_arch_info(gchar *tag, gchar *user, gchar *memo);
IUXAPI int scm_set_arch_data_format(ARCH_FORMAT_E format);
IUXAPI int scm_set_arch_data_cipher(ARCH_FORMAT_E format, char *pw, int pw_max_len);

// Archived data reserving
IUXAPI NF_ARCH_AVI_INFO *scm_new_avi_list(int *ret_cnt);
IUXAPI int scm_free_avi_list(NF_ARCH_AVI_INFO *avi);
IUXAPI int scm_get_avi_info(NF_ARCH_AVI_INFO *avi, guint16 avi_id);
IUXAPI int scm_get_avi_list_count();
IUXAPI int scm_delete_avi(guint16 id);
IUXAPI NF_ARCH_SNAP_INFO *scm_new_snap_list(int *ret_cnt);
IUXAPI int scm_free_snap_list(NF_ARCH_SNAP_INFO *snap, int count);
IUXAPI int scm_free_snap_list(NF_ARCH_SNAP_INFO *snap, int count);
IUXAPI int scm_get_snap_info(NF_ARCH_SNAP_INFO *snap, guint16 snap_id);
IUXAPI int scm_get_snap_list_count();
IUXAPI int scm_delete_snap(guint16 id);
IUXAPI NF_DISK_PRESERVE_INFO *scm_new_preserve_list(int *ret_cnt);
IUXAPI int scm_free_preserve_list(NF_DISK_PRESERVE_INFO *preserve);
IUXAPI int scm_get_preserve_list_count();
IUXAPI int scm_delete_preserve(guint16 id);
IUXAPI int scm_get_preserve_reserved_data(guint16 preserve_id, guint16 count, NF_DISK_PRESERVE_INFO *data);


// IP cam
IUXAPI CAM_PROFILE_T *scm_new_cam_profile();
IUXAPI int scm_free_cam_profile(CAM_PROFILE_T *prof);
IUXAPI int scm_get_cam_profile(int ch, CAM_PROFILE_T *prof);
IUXAPI BITMASK scm_get_cam_conn_state();
IUXAPI BITMASK scm_get_cam_signal();
IUXAPI int scm_cam_reset(gint ch);
IUXAPI int scm_get_ipcam_image_supp(int ch, guint *supp);
IUXAPI int scm_set_ipcam_image(NFIPCamSetupColor *info);
IUXAPI int scm_set_ipcam_image_advanced(NFIPCamSetupImage *info);
IUXAPI int scm_set_ipcam_rotation(int ch, int rotate);
IUXAPI int scm_set_ipcam_antiflicker(int ch, int sig);
IUXAPI int scm_req_ipcam_calibration(int ch);
IUXAPI int scm_req_ipcam_onepush(int ch);
IUXAPI int scm_get_ipcam_ptz_supp(int ch);
IUXAPI int scm_get_ipcam_auxiliary_supp(int ch);
IUXAPI int scm_get_ipcam_preset_supp(int ch);
IUXAPI int scm_get_ipcam_zoom_supp(int ch);
IUXAPI int scm_get_ipcam_focus_supp(int ch);
IUXAPI int scm_get_ipcam_iris_supp(int ch);
IUXAPI int scm_get_ipcam_onepush_supp(int ch);
IUXAPI int scm_get_ipcam_calibration_supp(int ch);
IUXAPI int scm_get_ipcam_vca_supp(int ch);
IUXAPI int scm_get_ipcam_dva_supp(int ch);
IUXAPI int scm_get_ipcam_ai_type(int ch);
IUXAPI int scm_get_cam_type_HDI(int ch, gchar type[32]);
IUXAPI CONNENT_CAM_E scm_get_cam_connect(int ch);
IUXAPI int scm_get_motion_size(int ch, int *rows, int *cols);
#if defined(_IPX_MODEL_UX)
IUXAPI int scm_get_mdraw_method(int ch, MOTION_AREA_METHOD_E *method);
IUXAPI int scm_get_ipcam_motion_rect_num(int ch, int *count);
IUXAPI int scm_get_ipcam_motion_profile(int ch, NFIPCamMotionProfile* info);
IUXAPI int scm_get_ipcam_status(int ch, NFIPCamStatusInfo* info);
#endif
IUXAPI int scm_ipcam_is_ptz(int ch);
IUXAPI int scm_get_mraw_data(int ch, guchar data[1024]);
IUXAPI int scm_request_ipcam_factory_default();
IUXAPI int scm_support_direct_config(int ch);
IUXAPI int scm_start_ipcam_install_mode();
IUXAPI int scm_stop_ipcam_install_mode();
IUXAPI int scm_openmode_ipsetup(int index, NFOpenmodeSetupNetwork* info, IMSG ret_msg);
IUXAPI int scm_openmode_change_pw(int index, gchar *pw, IMSG ret_msg);
IUXAPI int scm_openmode_portsetup(int index, NFOpenmodeSetupPorts* info, IMSG ret_msg);
IUXAPI int scm_set_ipcam_poe_onoff(int ch, int onoff);
IUXAPI int scm_support_ipcam_focus_profile(gint ch, NFIPCamFocusCompProfile* profile);
IUXAPI int scm_ipcam_get_max_stream_ratio(gint ch, gint *width_ratio, gint *height_ratio);
IUXAPI int scm_ipcam_get_main_stream_ratio(gint ch, gint *width_ratio, gint *height_ratio);
IUXAPI int scm_get_ipcam_corridor_supp(int ch);
IUXAPI int scm_get_ipcam_corridor_mode(int ch);
IUXAPI int scm_set_ipcam_eosd_noshow_toggle(int ch, int toggle, char *call_func);


// Audio
IUXAPI AUDIO_INFO_T *scm_new_audio_info();
IUXAPI int scm_free_audio_info(AUDIO_INFO_T *audio);
IUXAPI int scm_change_live_audio(int ch);
IUXAPI int scm_get_cur_live_audio_ch();
IUXAPI ONOFF_E scm_get_mic_state();
IUXAPI int scm_turnon_mic();
IUXAPI int scm_turnoff_mic();
IUXAPI int scm_enable_mic_ch(char mic_stat[], int ch_count);
IUXAPI int scm_get_enabled_mic_ch(char buf[], int buf_size);
IUXAPI int scm_get_audio_in_supp(int ch);
IUXAPI int scm_get_mic_out_supp(int ch);

// Ptz control
IUXAPI int scm_init_ptz_param(int ch);
IUXAPI int scm_run_ptz_cmd_left();
IUXAPI int scm_run_ptz_cmd_left_up();
IUXAPI int scm_run_ptz_cmd_up();
IUXAPI int scm_run_ptz_cmd_right_up();
IUXAPI int scm_run_ptz_cmd_right();
IUXAPI int scm_run_ptz_cmd_right_down();
IUXAPI int scm_run_ptz_cmd_down();
IUXAPI int scm_run_ptz_cmd_left_down();
IUXAPI int scm_run_ptz_cmd_zoom_in();
IUXAPI int scm_run_ptz_cmd_zoom_out();
IUXAPI int scm_set_ptz_cmd_zoom_speed(int speed);
IUXAPI int scm_run_ptz_cmd_focus_near();
IUXAPI int scm_run_ptz_cmd_focus_far();
IUXAPI int scm_set_ptz_cmd_focus_speed(int speed);
IUXAPI int scm_run_ptz_cmd_focus_auto();
IUXAPI int scm_run_ptz_cmd_iris_open();
IUXAPI int scm_run_ptz_cmd_iris_close();
IUXAPI int scm_run_ptz_cmd_iris_auto();
IUXAPI int scm_stop_ptz_cmd();
IUXAPI int scm_get_protocol_cnt();
IUXAPI int scm_set_ptz_preset(int param0);
IUXAPI int scm_unset_ptz_preset(int param0);
IUXAPI int scm_run_ptz_cmd_goto(int param0);


// Error count
IUXAPI int scm_get_err_cnt();
IUXAPI int scm_increase_err_cnt();
IUXAPI int scm_reset_err_cnt();


// Snapshot
IUXAPI int scm_req_live_capture(IMSG ret_msg, int ch);
IUXAPI int scm_req_live_capture_without_time(IMSG ret_msg, int ch);
IUXAPI int scm_req_live_capture_without_stream(IMSG ret_msg, int ch);
IUXAPI int scm_req_playback_capture(IMSG ret_msg, int ch);
IUXAPI int scm_req_netkwork_map_capture_auto(IMSG ret_msg);
IUXAPI int scm_req_netkwork_map_capture_manual(IMSG ret_msg);

// Diagnosis
IUXAPI int scm_diagnosis_ipcam_network(BITMASK chmask, IMSG ret_msg);
IUXAPI int scm_diagnosis_ipcam_power(BITMASK chmask, IMSG ret_msg);
IUXAPI int scm_diagnosis_disk(IMSG ret_msg);
IUXAPI int scm_diagnosis_service_port(IMSG ret_msg);

#if 0
/*
 * parameter
 *
 * 	- fmt_email_info is formatted string data
 * 	- first and last character must be '|'.
 *		ex) "|mail.aaa.com|userid|passwd|...|"
 *
 *	- must following the sequence of the input data
 *		1. smtp_server (char pointer)
 *		2. smtp_port (int)
 *		3. smtp_security (int)
 *		4. smtp_user (char pointer)
 *		5. smtp_passwd (char pointer)
 *		6. from (char pointer)
 *		7. to (char pointer)
 *		8. subject (char pointer)
 *		9. contents (char pointer)
 *
 *	- recommend below sprintf codes to write viewers
 *		sprintf(fmt_svr_info, "|%s|%d|%d|%s|%s|", server, port, security, user, passwd);
 *
 */
IUXAPI int scm_test_email(char *fmt_email_info, char delimeter, IMSG ret_msg);
#endif

// ----- AVI PLAYER ----

IUXAPI int scm_open_avi_play_manager(IMSG ret_msg);
IUXAPI int scm_close_avi_play_manager();
IUXAPI int scm_set_play_channel(guint ch);

IUXAPI ACPCTX scm_create_archplay_ctx(int device_id);
IUXAPI int scm_destroy_archplay_ctx(ACPCTX acpctx);
IUXAPI int scm_get_afile_count(ACPCTX acpctx);
IUXAPI int scm_get_afile_name(ACPCTX acpctx, AFILEID id, char *buf, int len);
IUXAPI AFILE_INFO_T *scm_new_afile_list(ACPCTX acpctx, AFILEID start, AFILEID end);
IUXAPI int scm_free_afile_list(AFILE_INFO_T *afile_list);
IUXAPI int scm_get_afile_detail(ACPCTX acpctx, AFILEID id, NF_AVI_PLAYER_INFO_IN_JUNK *detail);
IUXAPI int scm_set_play_afile(ACPCTX acpctx, AFILEID id);
IUXAPI int scm_verify_arch_file(ACPCTX acpctx, AFILEID id, IMSG ret_msg);

IUXAPI gboolean scm_start_panic_record();
IUXAPI gboolean scm_stop_panic_record();
IUXAPI gboolean scm_toggle_panic_record();
IUXAPI gboolean scm_get_panic_record();
IUXAPI gboolean scm_get_record_time(time_t *first, time_t *last);


IUXAPI int scm_check_expired_user(char *user_id);
IUXAPI int scm_send_to_viewer(IMSG msgid, int param, bool dyn_data, void *data);


IUXAPI int scm_req_ipcam_mf_info();
IUXAPI int scm_start_ipcam_scan();
IUXAPI int scm_sync_ipcam_time_info();
IUXAPI int scm_work_ipcam_done();

// storage raid
IUXAPI int scm_get_disk_raidinfo(DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_get_disk_raid1_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_get_disk_raid5_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_get_disk_raid_advanced_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_conf_raid(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_get_disk_easy_raid_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_get_disk_advanced_raid_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_add_raid_disk(STRG_TYPE_E strg_type, guint raid_mode, guchar raid_id, DISK_RAIDINFO_T *disk_rinfo);
IUXAPI int scm_clear_raid_conf();
IUXAPI int scm_get_sata_info(STRG_TYPE_E strg_type, SATA_INFO_T *sata_info);
IUXAPI int scm_create_raid1(STRG_TYPE_E strg_type, IMSG ret_msg);
IUXAPI int scm_create_raid5(STRG_TYPE_E strg_type, IMSG ret_msg);
IUXAPI int scm_delete_raid(IMSG ret_msg);
IUXAPI int scm_get_jm_fw_ver(FIRMWARE_INFO * tFwInfo);

//qr code
IUXAPI int scm_set_qrcode(char *filename);
IUXAPI int scm_set_qrcode_use_URL(char *filename, char *qr_url);


IUXAPI int scm_analyze_video_range_dal(int ch, time_t from, time_t to, VCARule *rule);
IUXAPI int scm_cancel_analyze();

//cable test
IUXAPI int scm_check_cable(int ch, int device, IMSG ret_msg);

IUXAPI REBOOT_INFO_E scm_get_reboot_info(REBOOT_INFO_CONT *cont);
// screen
/*
    |2|3|
    |5|8|
    charr[1]=0;
    charr[2]=1;
    charr[4]=2;
    charr[7]=3;
*/

IUXAPI int scm_screen_live_div_change(char ch_arr[32], NF_DISPLAY_E div);
IUXAPI int scm_screen_live_full_toggle();
IUXAPI int scm_screen_live_osd_toggle();
IUXAPI int scm_screen_live_sequence_toggle();


// LICENSE

IUXAPI int scm_is_supported_va();
IUXAPI int scm_is_supported_dmva();
IUXAPI int scm_is_supported_dlva();
IUXAPI int scm_is_supported_aicam();
IUXAPI int scm_is_supported_aibox();

IUXAPI int scm_license_get_from_server(IMSG ret_msg);
IUXAPI int scm_license_decoding_key(int ch, char *input_code, NF_SYSDB_LICENSE_INFO *lic_info);
IUXAPI int scm_license_get_cam_license(int ch, LicenseData *lic_data);
IUXAPI int scm_license_set_cam_license(int ch, LicenseData *lic_data);
IUXAPI int scm_license_is_activated_dmva(int ch);
IUXAPI int scm_license_is_activated_dlva();
IUXAPI int scm_license_is_activated_aicam(int ch);
IUXAPI guint scm_license_is_activated_aicam_mask();
IUXAPI int scm_license_is_activated_aibox();

IUXAPI int scm_is_holiday(int year, int month, int day);
IUXAPI int scm_is_holiday_timet(time_t timeinfo);
IUXAPI int scm_get_rtl_skip_holiday(int cur_rtl, int year, int month, int day);



// EVENT

IUXAPI int scm_detect_analytics_additional_event(int ch, ANALYTICS_ADDITIONAL_EVENT_T *event);


// IPCAM FW LIST
IUXAPI char **scm_new_ipcam_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt);
IUXAPI char **scm_new_ipcam_fw_list_techwin(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt);
IUXAPI char **scm_new_ipcam_fw_list_idis(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt);

#endif
