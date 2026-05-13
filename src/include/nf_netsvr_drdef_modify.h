#ifndef __NF_NET_DRDEF_H__
#define __NF_NET_DRDEF_H__

#include "nf_common.h"

enum enumDronProtocol {
	//--- request code
	DR_LOGIN   = 32, // == VPR_USER,
	DR_GET_SYSINFO,
	DR_START_LIVE,
	DR_STOP_LIVE,
	DR_START_PLAY,
	DR_STOP_PLAY,
	DR_GET_RECINFO,
	DR_GET_LOG,
	DR_SET_EVENTMASK,
	DR_START_BACKUP,
	DR_STOP_BACKUP, 
	DR_PTZ, // = 43
	DR_GET_CURRENTTIME,
	DR_SET_CURRENTTIME,
	DR_AUTHENTICATION,
	DR_GET_SETUP,
	DR_SET_SETUP, 
	DR_RELEASE_SETUP,
	DR_GET_ACCOUNTINFO,
	DR_DISCONNECT_CLIENT,
	DR_GET_FUNCTABLE,
	DR_SYSTEM_FUNCTION,
	DR_AUDIO_MUTE,
	DR_GET_CAMERA_TITLE,			//55
	DR_GET_COVERT_STATUS,			//56
	DR_GET_PTZ_STATUS, 
	DR_GET_ALARM_STATUS,
	DR_ALARM_CONTROL,
	DR_GET_HDD_SIZE, 
	DR_GET_TIMEZONE = 61,
	DR_GET_DATETIME_FORMAT,
	DR_GET_CAMERA_NOVIDEO_STATUS,
	DR_TIMELINE = 64,
	
	DR_DISCONNECT = 101,
	DR_SEND_EVENT,
	DR_KEEPALIVE,
	DR_LOCALSETUP_STARTED,
	DR_CHANGE_NETWORKAUDIO,			// 105
	DR_CHANGE_CAMERATITLE,
	DR_CHANGE_COVERT,
	DR_CHANGE_PTZ,	
	DR_CHANGE_ALARM,	
	DR_CHNAGE_TIMEZONE,				//110	
	DR_CHNAGE_NOVIDEO,
	DR_CHNAGE_DATETIMEFORMAT,
	DR_GET_FWVERSION     = 115,
	
	DR_BACKDOOR = 255,
		
	//--- channel mask
	DRCHANNEL_101		= 0x00001,
	DRCHANNEL_102		= 0x00002,
	DRCHANNEL_103		= 0x00004,
	DRCHANNEL_104		= 0x00008,
	DRCHANNEL_105		= 0x00010,
	DRCHANNEL_106		= 0x00020,
	DRCHANNEL_107		= 0x00040,
	DRCHANNEL_108		= 0x00080,
	DRCHANNEL_109		= 0x00100,
	DRCHANNEL_110		= 0x00200,
	DRCHANNEL_111		= 0x00400,
	DRCHANNEL_112		= 0x00800,
	DRCHANNEL_113		= 0x01000,
	DRCHANNEL_114		= 0x02000,
	DRCHANNEL_115		= 0x04000,
	DRCHANNEL_116		= 0x08000,

	DRCHANNEL_4A		= 0x0000F,
	DRCHANNEL_4B		= 0x000F0,
	DRCHANNEL_4C		= 0x00F00,
	DRCHANNEL_4D		= 0x0F000,

	DRCHANNEL_9A		= 0x001FF,
	DRCHANNEL_9B		= 0x0FF80,

	DRCHANNEL_16		= 0x0FFFF,
	DRCHANNEL_ALL		= 0x0FFFF,
	
	//--- play directions
	DRPLAY_FORWARD	= 0,
	DRPLAY_BACKWARD	= 1,

	//--- video formats
	DRVIDEO_NTSC	= 0,
	DRVIDEO_PAL		= 1,

	//--- PTZ actions
	DRPTZ_MOVEX		= 1,
	DRPTZ_MOVEY		= 2,
	DRPTZ_ZOOM		= 3,
	DRPTZ_FOCUS		= 4,

 	DRPTZ_ACTION_STOP	= 0,//action_control
 	DRPTZ_ACTION_START	= 1,//action_control

	//--- error codes
	DRE_SOCKET		= 0x0000000F,		// socket error
	DRE_TIMEOUT,						// network connection/read/write timeout
	DRE_CONNFAIL,						// connection failure
	DRE_CONNFAIL_DATA,					// connection for data socket failed
	DRE_READ,							// general socket read error
	DRE_WRITE,							// general socket write error
	DRE_USERID,							// invalid user ID
	DRE_PASSWD,							// incorrect password
	DRE_NOTIMPL,						// service not implemented.
	DRE_CLIENT_FULL,					// full connection 	 //#20041011
	DRE_AUTHORITY,						// have no authority //#20041011
 	DRE_NEED_DISCONNECT_OTHER,   // (admin only)admin needs to select one of connected client and disconnect it
 	DRE_BACKUP_BY_ADMIN,    // (admin only)admin needs to decide to disconnect connected admin with backup
	DRE_DISCONNECT_FAIL,
 	DRE_LOCAL_SETUP,     // local admin is in setup mode
 	DRE_NOT_ENOUGH_MEM,
 	DRE_EXIST_ADMIN,
 	DRE_REMOTE_SETUP
};

//DVRIN_PROTOCOL_PTZ
typedef enum _DR_PTZ_CMD_E { 

	DR_PTZ_CMD_STOP,
	DR_PTZ_CMD_PAN_LEFT,
	DR_PTZ_CMD_PAN_RIGHT,
	DR_PTZ_CMD_TILT_UP,
	DR_PTZ_CMD_TILT_DOWN,
	DR_PTZ_CMD_ZOOM_WIDE,
	DR_PTZ_CMD_ZOOM_TELE,
		
	DR_PTZ_CMD_FOCUS_FAR,
	DR_PTZ_CMD_FOCUS_NEAR,

	DR_PTZ_CMD_SET_PRESET,			
	DR_PTZ_CMD_GOTO_PRESET,		
	DR_PTZ_CMD_SWING		
	
} DR_PTZ_CMD_E;

typedef enum _DR_INFORM_E {
	DR_INFORM_KEEPALIVE,
	DR_INFORM_LOCALSETUP_STARTED,
	DR_INFORM_CHANGE_NETWORKAUDIO,
	DR_INFORM_CHANGE_CAMERA_TITLE,
	DR_INFORM_CHANGE_COVERT,
	DR_INFORM_CHANGE_PTZ, 
	DR_INFORM_CHANGE_ALARM,
	DR_INFORM_CHNAGE_TIMEZONE,
	DR_INFORM_CHNAGE_NOVIDEO,
	DR_INFORM_CHNAGE_DATETIMEFORMAT,
	DR_INFORM_DISCONNECT,
	DR_INFORM_NR
} DR_INFORM_E;


//--- request parameter blocks
typedef struct _DRP_CONNECT_T {
	unsigned int		key_code;
	unsigned short		product_ver;
	unsigned short		protocol_ver;
} DRP_CONNECT;

typedef struct _DRREQ_LOGIN_T {
	char				userName[32];
	char				passwd[32];
} DRREQ_LOGIN;

typedef struct _DRREQ_START_LIVE_T {
	unsigned int		channel_mask;
	unsigned short		audio_channel_id;
	unsigned char		audio_mute;
	unsigned char		reserved;
} DRREQ_START_LIVE;

typedef struct _DRREQ_START_PLAY_T {	
	unsigned int		start_time;
	unsigned int		end_time;
	unsigned char		start_time_sub;
	unsigned char		end_time_sub;
	unsigned char		direction;
	unsigned char		speed;
	unsigned int		channel_mask;
	unsigned short		audio_channel_id;
	unsigned char		audio_mute;
	unsigned char		reserved;
} DRREQ_START_PLAY;

typedef struct _DRREQ_GET_REC_INFO_T {	
	unsigned int		date;
	unsigned char		channel;
	unsigned char		reserved1;
	unsigned short		reserved2;

} DRREQ_GET_REC_INFO;

typedef struct _DRREQ_GET_LOG_T {
	unsigned int		start_time;
	unsigned int		end_time;
	unsigned char		start_time_sub;
	unsigned char		end_time_sub;
	unsigned char		direction;
	unsigned char		mode;
	unsigned int		log_id;
	unsigned int		log_id_sub;
	unsigned int		max_count;
	unsigned int		type_mask;	
	unsigned char		param1mask[352];
	unsigned int		channel_mask;
} DRREQ_GET_LOG;

typedef struct _DRREQ_START_BACKUP_T {	
	unsigned int		start_time;
	unsigned int		end_time;
	unsigned char		start_time_sub;
	unsigned char		end_time_sub;
	unsigned char		reserved;
	unsigned char		include_audio;
	unsigned int		channel_mask;
} DRREQ_START_BACKUP;

typedef struct _DRREQ_SET_EVENT_MASK_T {
	unsigned short		filter;
	unsigned short		reserved;
} DRREQ_SET_EVENT_MASK;

typedef struct _DRREQ_PTZ_T {
	unsigned char		channel;
	unsigned char		action;
	unsigned char		action_param1;
	unsigned char		action_param2;
} DRREQ_PTZ;

typedef struct _DRREQ_SETSETUP_T {
	char				passwd[32];
	unsigned int		dbidx;
} DRREQ_SETUP;

typedef struct _DRREQ_TIMELINE_T {
	unsigned int	time_beg;
	unsigned int	resolution;
	unsigned short	count;
	unsigned char	max_channel;
	unsigned char	split_channel;
	unsigned int	channel_mask;
} DRREQ_TIMELINE;

typedef struct _DRRPY_TIMELINE_ELEM_T
{
	// bit[3:0]: Record Reason and Type
	// bit[7:4]: Resolution
	unsigned char	RTRes;
	unsigned char	fps;
} DRRPY_TIMELINE_ELEM;		


//-- reply data blocks
typedef struct _DRRPY_GET_SYS_INFO_T {
	char				model_name[64];
	unsigned int		hw_version;
	unsigned char		video_format;		// NTSC=0, PAL=1
	unsigned char		channel_count;
	unsigned int		channel_mask;
	unsigned short		product_ver;
	unsigned short		protocol_ver;
	unsigned char		audio_count;
	unsigned char		audio_mask;
	
} DRRPY_GET_SYS_INFO;

#define drtimerisset(tvp)         ((tvp)->sec || (tvp)->usec)
#define drtimercmp(tvp, uvp, cmp) \
        ((tvp)->sec cmp (uvp)->sec || \
         (tvp)->sec == (uvp)->sec && (tvp)->subsec cmp (uvp)->subsec)
#define drtimerclear(tvp)         (tvp)->sec = (tvp)->subsec = 0

//--- event log
enum {
	DREC_NOTASSIGNED	= 0xFF,		// channel id is not assigned.
	
	// event log type
	DRET_SYSTEM			= 0x8000,	
	DRET_HDD			= 0x4000,
	DRET_LOGON			= 0x2000,
	DRET_ALARM			= 0x1000,
	DRET_MOTION			= 0x0800,
	DRET_CAM			= 0x0400,
	//unsigned char		end_time_sub;
	DRET_REC			= 0x0200,
	DRET_NET			= 0x0100,
	DRET_ERROR			= 0x0080,

	// event log parameter
	DREP_S_SYSON		= 0x8000,
	DREP_S_SYSOFF		= 0x4000,
	
	DREP_H_FORMAT		= 0x8000,
	DREP_H_BADSECTOR	= 0x4000,
	
	DREP_L_ADMIN		= 0x8000,
	DREP_L_USER			= 0x4000,
	DREP_L_NET			= 0x2000,
	DREP_L_LOCAL		= 0x1000,
	DREP_L_LOGON		= 0x0800,
	DREP_L_LOGOFF		= 0x0400,
	
	DREP_C_VIN			= 0x8000,	// video in
	DREP_C_VLOSS		= 0x4000,	// video loss
	
	DREP_R_MANUAL		= 0x8000,
	DREP_R_ALARM		= 0x4000,
	DREP_R_SCHEDULE		= 0x2000,
	DREP_R_MOTION		= 0x1000,
	DREP_R_NET			= 0x0800,
	DREP_R_PREREC		= 0x0400,
	
	DREP_N_CONNECTED	= 0x8000,
	DREP_N_DISCONNECTED	= 0x4000,
	
	DREP_E_BOOT			= 0x8000,
	DREP_E_ENC			= 0x4000,
	DREP_E_DEC			= 0x2000,
	DREP_E_HDDIO		= 0x1000,
	DREP_E_DISPLAY		= 0x0800,
};


typedef struct _DR_EVENT_LOG {
	unsigned int		sec;
	unsigned char		subsec;
	unsigned char		param3;
	unsigned short		eventlog_type;
	unsigned int		channel_id;
	unsigned short		param1;
	unsigned short		param2;
	unsigned int		seqNum;	//applied after 20040630
} DR_EVENT_LOG;

//#20041014
typedef struct _DR_PACKET_ACCOUNT_INFO_T
{
	char				user_id[32];
	unsigned int		ip;
	unsigned int		start_connection_time;	//current time since 1970.1.1 (second)
	unsigned char		authority;			//0: operator, 1: manager, 2: admin
	unsigned char		state;				//0: None, 1:live , 2: play, 3:backup
	unsigned short		account_id;			//session #
} DR_PACKET_ACCOUNT_INFO;

typedef struct _DR_PACKET_FUNC_TABLE_T
{
	unsigned char		get_sys_info;	//authority, 0xff = nobody has authority
	unsigned char		surv;
	unsigned char		play;
	unsigned char		backup;
	unsigned char		get_rec_info;
	unsigned char		get_log;
	unsigned char		set_event_mask;
	unsigned char		ptz;
	unsigned char		get_current_time;
	unsigned char		set_current_time;
	unsigned char		get_setup;
	unsigned char		set_setup;
	unsigned char		get_account_info;
	unsigned char		disconnect_client;
	unsigned char		get_action_auth;
	unsigned char		sys_temfunction;
	unsigned char		audio_mute;
	unsigned char		get_camera_title;
	unsigned char		reserved1;
	unsigned char		reserved2;
} DR_PACKET_FUNC_TABLE;

//주의 DR_PACKET_AUTH와 SES_AUTH는 값이 다르다.. 
enum DR_PACKET_AUTHORITY {
	DR_PACKET_AUTH_NONE = 0,
	DR_PACKET_AUTH_ADMIN,
	DR_PACKET_AUTH_MANAGER,
	DR_PACKET_AUTH_OPERATOR
};

typedef struct _DR_PACKET_GET_HDD_SIZE_T
{
	unsigned int 		total_size;
	unsigned int		used_size;
} DR_PACKET_GET_HDD_SIZE;

typedef struct _DR_PACKET_GET_TIMEZONE_T
{
	unsigned int 		daylight_saving;
	unsigned int 		time_zone;
} DR_PACKET_GET_TIMEZONE; 	

typedef struct _DR_PACKET_GET_DATE_TIME_FORMAT_T
{
	unsigned short 		time_format_idx;
	unsigned short 		date_format_idx;
} DR_PACKET_GET_DATE_TIME_FORMAT;


typedef struct _DR_PACKET_PTZ_ELEM_T
{
	int 				code;
	char 				name[24];
	unsigned int		flag;
} DR_PACKET_PTZ_ELEM;

typedef struct _DR_PACKET_GET_SYSINFO_T
{
	unsigned int		hw_ver;				// not used
	unsigned char		product_ver;			// HTR16(10) HTR8(11) 
	unsigned char		protocal_ver;			// 0
	unsigned char		video_format;			// NTSC(0) PAL(1)
	unsigned char		channel_count;		// get_num_of_active_video()
} DR_PACKET_GET_SYSINFO;

typedef struct _DR_PACKET_GET_FWVERSION_T
{
	unsigned char		version[256];
} DR_PACKET_GET_FWVERSION;


typedef struct _DR_PACKET_GET_CAMTITLE_T
{
	unsigned char title[32][13];	
} DR_PACKET_GET_CAMERATITLE;

/******************************************************************************/
/******************************************************************************/

// for nf_netsvr_reqcmd.c
int _dr_login                     (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);				
int _dr_get_sysinfo               (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_start_live                (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_stop_live                 (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_start_play                (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_stop_play                 (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_recinfo               (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_log                   (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_set_eventmask             (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_start_backup              (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_stop_backup               (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_ptz                       (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_currenttime           (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_set_currenttime           (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_authentication            (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_setup                 (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_set_setup                 (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_release_setup             (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_accountinfo           (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_disconnect_client         (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_functable             (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_system_function           (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_audio_mute                (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_camera_title          (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_covert_status         (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_ptz_status            (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_alarm_status          (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_alarm_control             (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_hdd_size              (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_timezone              (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_datetime_format       (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_get_camera_novideo_status (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_timeline                  (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_disconnect(unsigned short high, unsigned short low);

int _dr_keepalive                 (JOB_INFO *pJobEntry);
int _dr_send_event                (JOB_INFO *pJobEntry);
int _dr_localsetup_started        (JOB_INFO *pJobEntry);
int _dr_change_networkaudio       (JOB_INFO *pJobEntry);
int _dr_change_cameratitle        (JOB_INFO *pJobEntry);
int _dr_change_covert             (JOB_INFO *pJobEntry);
int _dr_change_ptz	              (JOB_INFO *pJobEntry);
int _dr_change_alarm              (JOB_INFO *pJobEntry);
int _dr_chnage_timezone           (JOB_INFO *pJobEntry);
int _dr_chnage_novideo            (JOB_INFO *pJobEntry);
int _dr_chnage_datetimeformat     (JOB_INFO *pJobEntry);

int _process_play(JOB_INFO *pJobEntry);
int _process_backup(JOB_INFO *pJobEntry);

int _dr_get_fwversion             (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);
int _dr_backdoor                  (CLIENT_INFO * info, vpacket_t * req_vp, char *req_buff);

#endif
