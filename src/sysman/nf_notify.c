#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"

#include "nf_notify.h"
#include "nf_timer.h"
#include "nf_codec_header.h"
#include "nf_api_eventlog.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "notify"

//#define TEST_NOTIFY_API
//#define DEBUG_RAND_ALARM
//#define DEBUG_NOTIFY_DIRECT_FIRE
#define DEBUG_NOTIFY_MOTION_LOG_SKIP
#define DEBUG_NOTIFY_ALARM_LOG_SKIP

#define DEBUG_NOTIFY_JBSHELL
#define DEBUG_NOTIFY_LOG

#define ENABLE_PND_PROP

#ifdef DEBUG_NOTIFY_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_NOTIFY_IDX_E
{
	DEBUG_NOTIFY_IDX_THREAD 	= 0,
	DEBUG_NOTIFY_IDX_GET	 	= 1,
	DEBUG_NOTIFY_IDX_SET 		= 2,
	DEBUG_NOTIFY_IDX_CALLBACK	= 3,
	
	DEBUG_NOTIFY_IDX_LOGPUT		= 4,
	DEBUG_NOTIFY_IDX_LOGPUT_SIMPLE 	= 5,
	DEBUG_NOTIFY_IDX_LOGPUT_REC		= 6,
	DEBUG_NOTIFY_IDX_LOGPUT_REC_SIMPLE 	= 7,
	
	DEBUG_NOTIFY_IDX_FIRE 	= 8,	
	DEBUG_NOTIFY_IDX_NR	= 9
}DEBUG_NOTIFY_IDX_E;

static const char *_DEBUG_NOTIFY_str[32] =
{
	"NOTIFY_IDX_THREAD",
	"NOTIFY_IDX_GET",
	"NOTIFY_IDX_SET",
	"NOTIFY_IDX_CALLBACK",
	
	"NOTIFY_IDX_LOGPUT",
	"NOTIFY_IDX_LOGPUT_SIMPLE",
	"NOTIFY_IDX_LOGPUT_REC",
	"NOTIFY_IDX_LOGPUT_REC_SIMPLE",	
	
	"NOTIFY_IDX_FIRE",
	"NOTIFY_IDX_NR"
};

static gint _DEBUG_NOTIFY_log[32] = 
{
	0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,0,
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
	PROP_SENSOR,
	PROP_MOTION,
	PROP_USER_ALARM,
	PROP_VLOSS,	
	PROP_ALARM,
	PROP_LOG,
	
	PROP_ANALOG_REC,
	PROP_IPCAM_REC,

	PROP_ENC_STATUS,
	PROP_NET_STATUS,	
	PROP_NET_RXTX,
	
	PROP_FS_STATUS,
	PROP_DISK_FULL,
	PROP_DISK_OVERWR,	
	PROP_DISK_USAGE,
	PROP_DISK_SMART,
	
	PROP_SYSDB_CAM_TITLE,
	PROP_SYSDB_COVERT,
	PROP_SYSDB_PTZ,
	PROP_SYSDB_TIME_FORMAT,
	PROP_SYSDB_TIME_ZONE,

	PROP_GUI_SETUP,
	PROP_SYSDB_CHANGE,
	PROP_TIME_CHANGE,
	
	PROP_DVR_STATUS,
	PROP_DVR_WDTIMER,

#ifdef ENABLE_PND_PROP
	PROP_PND_EVENT,
	PROP_PND_PROGRESS,
	PROP_PND_OSD,
#endif

	PROP_PND_HUB_STATUS,
	PROP_IPCAM_SEARCH_LIST,

#ifdef ENABLE_IPCAM_PROP
	PROP_IPCAM_SENSOR,		
	PROP_IPCAM_MOTION,		
	PROP_IPCAM_USER_ALARM,	
	PROP_IPCAM_VLOSS,		
	PROP_IPCAM_ALARM,	
	PROP_IPCAM_ENC_STATUS,
#endif

	PROP_DISK_WRITE_FAIL,
	PROP_DISK_EXHAUST,
	PROP_DISK_NODISK,
	PROP_DISK_SMART_REQCHK,
	
	PROP_SYS_FAN,	
	PROP_SYS_TEMPERATURE,	
	PROP_SYS_POE_STATUS,
	PROP_SYS_POE_STATUS_HUB,
	
	PROP_DVR_LOGIN_FAIL,
	PROP_NET_LOGIN_FAIL,	
	
	PROP_NET_WAN_STATUS,
	PROP_NET_DDNS_STATUS,
	PROP_MOTION_RAW_DATA,
				
	PROP_SYS_BOOTING,	
	PROP_BUZZER,
	
	PROP_LOGIN_USER,
	PROP_NET_IP_CHANGED,
	
	PROP_SYS_POE_PORT,
	PROP_SYSDB_TMP_CHANGE,

	PROP_DVA_OBJECT,
	PROP_DVA_EVENT,
	PROP_DVA_COUNTER,
	
	PROP_VCA_EVENT,
	PROP_VCA_TRACKINFO,
	PROP_VCA_META_DATA,
	PROP_VCA_COUNTER,

	PROP_AI_EVENT,
	PROP_AI_TRACKINFO,
	PROP_AI_COUNTER,
	PROP_AI_FR_EVENT,
	PROP_AI_FR_DATA,
	PROP_AI_LPR_EVENT,
	PROP_AI_LPR_DATA,

	PROP_AI_GENERIC_EVENT,
	PROP_AI_KEEP_ALIVE,
	
	PROP_AIBOX_DB_CHANGE,

	PROP_SYSDB_IPCAM_CHANGE,

	PROP_UEVENT_S1_DUAL,

	PROP_NET_S1_FW_UP,

	PROP_USB_SERIAL,
	PROP_SYS_POS_STATUS,

	PROP_MANUAL_EVENT,

	PROP_SET_IP_CONFLICT,
	PROP_CAM_IP_CONFLICT,

	PROP_POS_TEXT_EVENT,
	PROP_DISK_RAID,

	LAST_PROP
	/* FILL ME */
};

typedef struct _NOTIFY_INFO
{
	guint			idx;
	gchar			name[64];
	guint			size;
	NF_NOTIFY_TYPE	type;
	GParamSpec 		*param;	
	gpointer		data;
}NOTIFY_INFO;
                         
static NOTIFY_INFO _g_notify_info[] =
{
	{PROP_0					,"null"				,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_SENSOR			,"sensor"			,0	,NF_NOTIFY_PARAM, NULL , NULL},	// 
	{PROP_MOTION			,"motion"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_USER_ALARM		,"user_alarm"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_VLOSS				,"vloss"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_ALARM				,"alarm"			,0	,NF_NOTIFY_PARAM, NULL , NULL},		
	{PROP_LOG				,"log"				,0	,NF_NOTIFY_POINTER, NULL , NULL},	

	{PROP_ANALOG_REC		,"analog_rec"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},	
	{PROP_IPCAM_REC			,"ipcam_rec"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},	

	{PROP_ENC_STATUS		,"enc_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_STATUS		,"net_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_RXTX			,"net_rxtx"			,0	,NF_NOTIFY_PARAM, NULL , NULL},				
	
	{PROP_FS_STATUS			,"fs_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_FULL			,"disk_full"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_OVERWR		,"disk_overwr"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_DISK_USAGE		,"disk_usage"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_SMART		,"disk_smart"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_SYSDB_CAM_TITLE	,"sysdb_cam_title"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_COVERT		,"sysdb_covert"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_PTZ			,"sysdb_ptz"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_TIME_FORMAT	,"sysdb_tformat"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_TIME_ZONE	,"sysdb_tzone"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_GUI_SETUP			,"gui_setup"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_SYSDB_CHANGE		,"sysdb_change"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_TIME_CHANGE		,"time_change"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	

	{PROP_DVR_STATUS		,"dvr_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_DVR_WDTIMER		,"dvr_wdtimer"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

#ifdef ENABLE_PND_PROP
	{PROP_PND_EVENT			,"pnd_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_PND_PROGRESS		,"pnd_progress"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_PND_OSD			,"pnd_osd"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
#endif

	{PROP_PND_HUB_STATUS	,"pnd_hub_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_SEARCH_LIST ,"ipcam_slist"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
#ifdef ENABLE_IPCAM_PROP
	{PROP_IPCAM_SENSOR		,"ipcam_sensor"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_MOTION		,"ipcam_motion"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_USER_ALARM	,"ipcam_user_alarm"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_VLOSS		,"ipcam_vloss"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_ALARM		,"ipcam_alarm"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_IPCAM_ENC_STATUS	,"ipcam_enc_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
#endif

	{PROP_DISK_WRITE_FAIL	,"disk_write_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_EXHAUST		,"disk_exhaust"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_NODISK		,"disk_nodisk"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_SMART_REQCHK	,"disk_smart_reqchk",0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_SYS_FAN			,"sys_fan"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_TEMPERATURE	,"sys_temperature"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_STATUS	,"sys_poe_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_STATUS_HUB,"sys_poe_status_hub",0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_DVR_LOGIN_FAIL	,"dvr_login_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_LOGIN_FAIL	,"net_login_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_NET_WAN_STATUS	,"net_wan_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_DDNS_STATUS	,"net_ddns_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_MOTION_RAW_DATA	,"mraw_data"		,0	,NF_NOTIFY_POINTER, NULL , NULL},	

	{PROP_SYS_BOOTING		,"sys_booting"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_BUZZER			,"buzzer"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_LOGIN_USER        ,"login_user"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},
	{PROP_NET_IP_CHANGED    ,"net_ip_changed"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_PORT      ,"sys_poe_port"	    ,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_SYSDB_TMP_CHANGE	,"sysdb_tmp_change"	 ,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_DVA_OBJECT		,"dva_object"		,0	,NF_NOTIFY_POINTER, NULL , NULL},
	{PROP_DVA_EVENT			,"dva_event"		,0	,NF_NOTIFY_POINTER, NULL , NULL},
	{PROP_DVA_COUNTER		,"dva_counter"		,0	,NF_NOTIFY_POINTER, NULL , NULL},
	
	{PROP_VCA_EVENT			,"vca_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_VCA_TRACKINFO		,"vca_trackinfo"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_VCA_META_DATA		,"vca_mata_data"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_VCA_COUNTER		,"vca_counter"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_AI_EVENT			,"ai_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_TRACKINFO		,"ai_trackinfo"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_COUNTER		,"ai_counter"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_AI_FR_EVENT			,"ai_fr_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_FR_DATA		,"ai_fr_data"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_LPR_EVENT			,"ai_lpr_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_LPR_DATA		,"ai_lpr_data"	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_AI_GENERIC_EVENT			,"ai_generic_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_AI_KEEP_ALIVE				,"ai_keep_alive"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

    {PROP_AIBOX_DB_CHANGE      ,"aibox_db_change"     ,0  ,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_SYSDB_IPCAM_CHANGE,"sysdb_ipcam_change",0	,NF_NOTIFY_PARAM, NULL , NULL},

	// 2013-06-07 오후 2:43:03 choissi
	{PROP_UEVENT_S1_DUAL	,"uevent_s1_dual"	 	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_NET_S1_FW_UP		,"net_s1_fw_up"	 	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_USB_SERIAL		,"pos_dev"	 	,0	,NF_NOTIFY_POINTER, NULL , NULL},
	{PROP_SYS_POS_STATUS	,"sys_pos_changed" 	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_MANUAL_EVENT		,"manual_event" 	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_SET_IP_CONFLICT   ,"set_ip_conflict"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_CAM_IP_CONFLICT   ,"cam_ip_conflict"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_POS_TEXT_EVENT	,"pos_text_event"	 	,0	,NF_NOTIFY_POINTER, NULL , NULL},	
	
	{PROP_DISK_RAID			,"disk_raid"	 	,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{LAST_PROP				,"null"				,0	,NF_NOTIFY_PARAM, NULL , NULL}
};

/*
		

1) motion,alarm,sensor,vloss 
   NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용    
      d.params[0]      아날로그 채널별 비트 마스크 
      d.params[1]   ipcamera 채널별 비트 마스크 
             
2) analog_rec, ipcam_rec 
   NF_NOTIFY_CHMAP 
      c.chmap[0~15]   'T'   timer rec 
            'P'   panic 
            'A' alarm 
            'M' motion 
            'p' pre_recoding
            ' '   none 
                                           
3) enc_status   - 녹화중인 채널의 갯수 
   NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
      d.params[0]      현재 analog fps 합계 
      d.params[1]      현재 ipcamera fps 합계 
      d.params[2]      max analog fps 
      d.params[3]      max ipcamera fps 

4) net_status   - 네트워크 클라이언트 상태 
   NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
      d.params[0]      접속중인 클라이언트 
      d.params[1]      live mode 클라이언트 
      d.params[2]      playback mode 클라이언트 

5) net_rxtx   - 네트워크 트래픽 모니터 
   NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
      d.params[0]      netsvr status 100~0 
                  트래픽이 원할한지를 표현함 
                  (프레임 레이트 컨트롤 변수로 확인해야겠음) 
      d.params[1]      netsvr rx    bytes/sec
      d.params[2]      netsvr tx    bytes/sec
      d.params[3]      link status  0:offline 1:online

6) disk_usage   - 디스크 사용량 
   NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
      d.params[0]      사용량 (MB) 
      d.params[1]      총용량 (MB) 

7)  sysdb_cam_title - 카메라 타이틀 sysdb에서 항목이 변경됨
	sysdb_covert	- covert 변경
	sysdb_ptz		- ptz 설정 변경
	sysdb_tformat	- time format 변경
	sysdb_tzone		- time zone 변경
	NF_NOTIFY_PARAM type이나 의미 없음. 단순 시그널 용

8) gui_setup - gui 가 셋업 모드 들어 가고 나갈 때
	NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
	  d.params[0]      1: 셋업모드 진입 0:셋업모드 나감

9) sysdb_change	- sysdb가 변경되어서 모듈들이 sysdb를 다시 읽어 와야 할 때,
	NF_NOTIFY_PARAM type의 NF_NOTIFY_INFO를 사용 
	  d.params[0]      enum _NF_SYSDB_CATE_E  {	
						NF_SYSDB_CATE_SYS,		// 0
						NF_SYSDB_CATE_NET,		// 1
						NF_SYSDB_CATE_AUDIO,	// 2
						NF_SYSDB_CATE_DISK,		// 3
						NF_SYSDB_CATE_CAM,		// 4
						NF_SYSDB_CATE_USR,		// 5
						NF_SYSDB_CATE_ALARM,	// 6
						NF_SYSDB_CATE_ACT,		// 7
						NF_SYSDB_CATE_DISP,		// 8 
						NF_SYSDB_CATE_REC		// 9
					}   

10) fs_status - filesystem start/stop 
	d.params[0]	 start:1, stop:0

11) disk_overwr - disk overwrite 발생 
	time_change	- datetime, timezone 변경시 
	
	NF_NOTIFY_PARAM type이나 의미 없음. 단순 시그널 용

12) dvr_status - dvr에서 GUI 상태

typedef enum _NF_DVR_STATUS_E	// nf_sysman.h
{
  NF_DVR_STATUS_INIT = 0,
  
  NF_DVR_STATUS_LIVE = 1,
  NF_DVR_STATUS_ZOOM = 2,				// ZOOM
  
  NF_DVR_STATUS_PLAYBACK = 3,
  NF_DVR_STATUS_RUN_PLAYBACK = 4,		// PLAY
  NF_DVR_STATUS_ARCHIVE = 5,
  NF_DVR_STATUS_RUN_ARCHIVE = 6,
  
  NF_DVR_STATUS_SETUP = 7,      		
  
  NF_DVR_STATUS_SHUTDOWN = 8,  

  NF_DVR_STATUS_LIVE_RUN_ARCHIVE = 9,	// choissi 2009-11-24 오후 2:13:54
  NF_DVR_STATUS_LIVE_AUTO_SYNC = 10,	// choissi 2009-11-24 오후 2:13:54
  
  NF_DVR_STATUS_NR

} NF_DVR_STATUS_E;

13) pnd_event
	d.params[0]  PND_TYPE_XXX
	d.params[1]  channel number integer

14) pnd_progress
	d.params[0]  connection progress
	d.params[1]  channel number integer


	disk_write_fail
		d.param[0] 부팅 후 Disk Write 실패 count
		d.param[1] 3분 누적 카운트
		d.param[2] 10분 누적 카운트
		d.param[3] 30분 누적 카운트

	disk_exhaust
		d.param[0]  1:디스크 거의 가득참 ( DB설정 값에 따라 동작: 기본값 90% )
	
	disk_nodisk
		d.param[0]	1:no_disk 상태임

	disk_smart_reqchk
		d.param[0]	1:SMART PRE ALARM
					실제 SMART Fail 상태는 아니고 DVR에서 각 필드를 읽어서 경고 해줌
		d.param[1]	disk_id 0 base이고 사용자에게 보여줄때는 +1해야함 

	sys_fan
		d.param[0]	fan fail mask
					0x01 cpu, 0x02 sys1, 0x04, sys2
		d.param[1]	cpu fan rpm
		d.param[2]	sys fan 1 rpm
		d.param[3]	sys fan 2 rpm
		
	sys_temperature
		d.param[0]	temperature fail mask      fan이 망가지거나, 통풍구가 막혀서 시스템 온도가 65 넘었을 때
					0x01 CPU, 0x02 SYSTEM
		d.param[1]	cpu temperature				현재 CPU 온도
		d.param[2]	system temperature			현재 System 온도
		
	sys_poe_status
		d.param[0]	0:init(정상)
					1:port fail
					2:totmW tail
					3:port and totmW fail					 
		d.param[1]	poe_warn_mask			12W이상 사용중인 포트번호
		d.param[2]	poe_power_cut_mask		15W이상이라 강제 전원 차단한 포트 마스크 
		d.param[3]  총 POE 전원 사용량 (mW)

	dvr_login_fail
		d.param[0]	fail_cnt 부팅후 누적
		d.param[1]	1분 간 누적 횟수 

	net_login_fail
		d.param[0]	fail_cnt 부팅후 누적
		d.param[1]	1분 간 누적 횟수 		
		d.param[2]	web fail_cnt
		d.param[3]	rtsp fail_cnt

	net_wan_status
		d.param[0]  0:초기화 상태, 1:정상, 음수일 때는 에러 코드
					nf_notify_get_update_time 을 사용하면 마지막 상태 업데이트 시간 얻을 수 있음
		Changed
		d.param[0]	0 : Success	1 : Fail

	net_ddns_status
		d.param[0]  0:초기화 상태, 1:정상, 음수일 때는 에러 코드
					nf_notify_get_update_time 을 사용하면 마지막 상태 업데이트 시간 얻을 수 있음
		
		Changed
		d.param[0]	0 : Success	1 : Fail
		d.param[1] 	Reason!!
		
	net_ip_changed
		d.param[0]  현재ip  IPV4만 유효, IPV6는 추후 고려
		d.param[1]  이전ip

	sys_poe_port
		d.param[0]	port
		d.param[1]	status
		d.param[2]	voltage (V)	
		d.param[3]	power (mW)

15) buzzer
	buzzer
		d.param[0]	0 : Buzzer Off


16 sysdb_tmp_chnage
		d.param[0]	EVENT_ID
		d.param[1]	param1
		d.param[2]	param2
		d.param[3]	param3


9) sysdb_ipcam_change	- 
	  d.params[0]   enum __NF_IPCAM_CATE_E_ {
						NF_IPCAM_CATE_STREAM	// 0
						NF_IPCAM_CATE_IMAGE,	// 1
						NF_IPCAM_CATE_PTZ,		// 2
						NF_IPCAM_CATE_MOTION,	// 3
						NF_IPCAM_CATE_PMASK,	// 4
						NF_IPCAM_CATE_VCA,		// 5
					}
	  d.params[1]   CH_MASK
	  
	disk_raid	
		d.params[0]
			0 : init or ok
			1: degrade
			2: rebuilding
			3: broken

		d.params[1]
			if ( d.params[0] == 2)	rebuild rate
			else don't care value


17) net_s1_fw_up
   에스원 원격 펌웨어 업그레이드
   서버에서 새로운 펌웨어 발견시 발생

18) pos_dev

    USB Serial 의 접속/비접속시 해당 정보를 알려줌.
    
    NOTIFY POINTER 에 해당 장치 정보 포함.
	
19) sys_pos_changed

    SYSTEM->POS/ATM 설정 메뉴에 의해 해당 채널의 동작여부 전달. 

20) set_ip_conflict

    본체 / 세트 기준.

    d.params[0]
                = 1   => 충돌 발생
                = 0   => 충돌 발생 없음.            

21) cam_ip_conflict

    카메라 기준. ( NVR 의 IPCAM )

    d.params[0] 
                = 1   => 충돌 발생
                = 0   => 충돌 발생 없음.

	d.params[1]
	            = ch  => 충돌이 발생되거나 해제된 채널 ( 0 base ) 

22) pos_text_event

	POS 데이터에서 설정된 TEXT 발견시 발생.

	NOTIFY POINTER 에서 해당 원문을 실어서 보냄. (라인기준)
	
	
23) disk_raid // SWIPXVETHR-356

p0 에 상태값
	0: init or ok
	1: degrade
	2: rebuilding
	3: broken
			
p1 
	2: rebuilding 일때 p1 리빌딩 레이트	
*/

static NF_NOTIFY_INFO _g_notify_arg[LAST_PROP];

static void nf_notify_class_init (NfNotifyClass * klass);
static void nf_notify_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_notify_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_notify_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_notify_dispose (GObject * object);
static void nf_notify_finalize (GObject * object);


/* static data */
static GQuark	quark_data_value = 0;
static GQuark	quark_cb_list = 0;
static GObjectClass *parent_class = NULL;
static NfNotify	*_nf_notify = NULL;

static void
notify_thread_func (NfNotify * test) ;

GType
nf_notify_get_type (void)
{
	static GType nf_notify_type = 0;

	if (G_UNLIKELY (nf_notify_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfNotifyClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_notify_class_init,
			NULL,
			NULL,
			sizeof (NfNotify),
			0,
			(GInstanceInitFunc) nf_notify_instance_init,
			NULL
		};

		nf_notify_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfNotify", &object_info, 0);
	}
	
	return nf_notify_type;
}

static void
nf_notify_class_init (NfNotifyClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_notify_set_property;
	gobject_class->get_property = nf_notify_get_property;
			
	gobject_class->dispose = nf_notify_dispose;
	gobject_class->finalize = nf_notify_finalize;

	quark_data_value = g_quark_from_static_string ("data-value");
	quark_cb_list = g_quark_from_static_string ("cb-list");
	
	for(i = PROP_SENSOR; i<LAST_PROP; i++)
	{				
		GParamSpec	*pspec;		
		gpointer	pdata = NULL;
		GList		*plist;
		
		//g_print(" %s\n",_g_notify_info[i].name);
		
		pspec = g_param_spec_pointer( _g_notify_info[i].name ,
										_g_notify_info[i].name, NULL, 
										G_PARAM_READWRITE );
		
		pdata = g_malloc0( sizeof (NF_NOTIFY_INFO) );		
		plist = g_list_alloc();
		
		g_assert( pspec );
		g_assert( pdata );
		g_assert( plist );

		_g_notify_info[i].param	= pspec;
		_g_notify_info[i].data	= pdata;
		
		// 실제로 데이터가 담길 곳;		
		g_param_spec_set_qdata (pspec, quark_data_value, (gpointer) pdata );
		
		// 관리 리스트
		g_param_spec_set_qdata (pspec, quark_cb_list, (gpointer) plist );

		g_object_class_install_property ( gobject_class,
											_g_notify_info[i].idx,
											_g_notify_info[i].param );
	}
	
}

static void
nf_notify_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfNotify *self = NF_NOTIFY (instance);
				
	self->init_done = 0;
	
	// queue 생성
	self->queue = g_async_queue_new();
 		 
	// notification signal emit용 thread 생성
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)notify_thread_func, 
									self, FALSE, NULL);
																		
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_notify_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_notify_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_notify_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
 	NfObject *self;
	self = NF_OBJECT (object);
	
	NF_NOTIFY_INFO *pinfo, *ptmp, *pdup;
	
/*
	윗단에서 프로퍼티가 해당 오브젝트에 있으면,
	prop_id가 얻어 져야 이 함수가 실행됨.
	
*/

#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_SET] )
		g_message("%s prop_id[%d] in",__FUNCTION__, prop_id);
#endif

	if( PROP_0 >= prop_id || LAST_PROP <= prop_id )
	{		
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);				
		return;
	}
	
	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);	
	
	pinfo = (NF_NOTIFY_INFO *)g_value_get_pointer(value);			
	pdup = nf_notify_dup( pinfo );	
	//g_message("%s prop_id[%d] [0x%p]",__FUNCTION__, prop_id, pdup);
#if 1 // choissi VCA workaround 2013-07-17 오후 11:20:02
	g_return_if_fail( pdup != NULL);		
#else	
	g_assert( pdup != NULL );	
#endif	

	g_param_spec_set_qdata (pspec, quark_data_value, pdup);
	_g_notify_info[prop_id].data = pdup;
	
	nf_notify_free( ptmp ); // 이전꺼 삭제
				
#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_SET] )
		g_message("%s prop_id[%d] out",__FUNCTION__, prop_id);
#endif
	
	return ;	
}

static void
nf_notify_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	NfObject *self;
	self = NF_OBJECT (object);

/*
	tmp_value = g_value_get_pointer(value)
	self->value = tmp_value,
	ref_count;
*/
#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_GET] )
		g_message("%s prop_id[%d] in",__FUNCTION__, prop_id);
#endif

	if( PROP_0 >= prop_id || LAST_PROP <= prop_id )
	{		
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);				
		return;
	}			
	
	g_value_set_pointer( value, nf_notify_dup( _g_notify_info[prop_id].data ) );	

#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_GET] )
		g_message("%s prop_id[%d] out",__FUNCTION__, prop_id);
#endif
}

//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	
static void
notify_thread_func (NfNotify * self)
{
	gpointer	que_poped_data = NULL;
	gint		qsize = 0, qsize_mark = 0;
	gint		prev_time_sec = 0, curr_time_sec = 0;
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
	while( _nf_notify == NULL ) g_usleep(10*1000);

	self->init_done = 1;	

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_NOTIFY, NF_WATCHDOG_TIME_SEC*2, NF_WATCHDOG_ENABLE);
#endif
					
	while(self->thread_run)
	{				

		que_poped_data = g_async_queue_pop( self->queue);		
		if( que_poped_data == NULL)		// timeout
		{	
/*				
		100ms나 200ms 마다 깨어나서, 
		통지에 필요한 것을 수집할 필요가 있음.
		
		넷 클라이언트, 디스크 사용량, 디스크 가득참		
*/
			//g_message("notify_thread_func pop_timeout");
						
			//void g_object_notify(GObject *object, const gchar *property_name);
			//check callback duration
		}
		else // get_data
		{
			NF_NOTIFY_QITEM	*pqitem = ( NF_NOTIFY_QITEM *)que_poped_data;
			const gchar 			*property_name = NULL;
												
			property_name = g_param_spec_get_nick(pqitem->pspec);

#ifdef DEBUG_NOTIFY_LOG
			if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_THREAD] )
				g_message("%s process %s 0x%p", __FUNCTION__, 
							property_name, pqitem->pitem );
#endif
			g_object_set(_nf_notify, property_name, pqitem->pitem , NULL);
			
			curr_time_sec = pqitem->pitem->timestamp.tv_sec;
			
			nf_notify_free ( pqitem->pitem ); // NF_NOTIFY_INFO
			g_free ( pqitem ); //NF_NOTIFY_QITEM
		}				

#ifdef ENABLE_WATCHDOG
		if(prev_time_sec != curr_time_sec)
		{
			nf_watchdog_kick( NF_WATCHDOG_MEMBER_NOTIFY );
			prev_time_sec = curr_time_sec;
		}
#endif		

		qsize = g_async_queue_length( self->queue);

#ifdef DEBUG_NOTIFY_LOG
			if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_THREAD] )
				g_message("%s notify qsize[%d]",__FUNCTION__, qsize);
#endif
		
		if(qsize > NF_NOTIFY_QSIZE_FULL)
		{
			if(qsize_mark != NF_NOTIFY_QSIZE_FULL){				
				g_warning("%s notify QFULL qsize[%d]",__FUNCTION__, qsize);
				qsize_mark = NF_NOTIFY_QSIZE_FULL;
			}
		}
		
		if( qsize_mark == NF_NOTIFY_QSIZE_FULL)
		{
			if(qsize < NF_NOTIFY_QSIZE_THRESHOLD)
			{
				g_warning("%s notify Qfull qsize[%d]",__FUNCTION__, qsize);
				qsize_mark = qsize;
			}			
		}else{
			qsize_mark = qsize;
		}
		
	}
	g_message("%s end", __FUNCTION__);
}

/**
	@brief				notify item malloc
	@return				gpointer 생성된 notify item
*/
NF_NOTIFY_INFO *
nf_notify_new()
{	
	// g_malloc0에 , timestamp 까지 생성
	
	NF_NOTIFY_INFO	*pinfo;	
	pinfo = g_malloc0(sizeof( NF_NOTIFY_INFO ));
		
	g_return_val_if_fail(pinfo != NULL, 0);

	if( pinfo )
	{
		pinfo->type = NF_NOTIFY_PARAM;
		//g_get_current_time( &pinfo->timestamp );
		gettimeofday((struct timeval *)&pinfo->timestamp, NULL);
	}
	
	return pinfo;
}

/**
	@brief				notify item malloc with size
	@return				gpointer 생성된 notify item
*/
NF_NOTIFY_INFO *
nf_notify_new_size( guint size)
{
	// g_malloc0 에다가 , timestamp를 찍어서 반환
	// sub message malloc;
	
	NF_NOTIFY_INFO	*pinfo;		
//printf("SKSHIN-1] [%d]\n", size);		
	g_return_val_if_fail (size > 0, NULL);
	
	pinfo = nf_notify_new();
	if( pinfo )
	{
		gpointer tmp;
						
//printf("SKSHIN-2] [%d]\n", size);		
		tmp = g_malloc(size);
		if(tmp)
		{
			pinfo->type = NF_NOTIFY_POINTER;	
			pinfo->p.len = size;
			pinfo->p.ptr = tmp;
			
			// For Test choissi 2013-07-17 오후 11:10:00 
			// ksi_test
			// pinfo->p.reserved[0] = tmp;
			// pinfo->p.reserved[1] = size;
		}	
		else
		{			
			g_free(pinfo);
			pinfo = NULL;
		}
	}
		
	return pinfo;
}

static int _show_backtrace()
{
	void *frame_addrs[16];
	char **frame_strings;
	size_t backtrace_size;
	int i;

	backtrace_size = backtrace(frame_addrs, 16);
	frame_strings = backtrace_symbols(frame_addrs, backtrace_size);
	for (i = 0; i < backtrace_size; ++i) {
		printf("%d: [0x%x] %s\n", i, frame_addrs[i], frame_strings[i]);
	}
	free(frame_strings);
	return 0;
}

/**
	@brief				notify item dup
	@return				gpointer 생성된 notify item
*/
NF_NOTIFY_INFO *
nf_notify_dup( NF_NOTIFY_INFO *src )
{	
	// g_malloc0에 , timestamp 까지 생성	
	NF_NOTIFY_INFO	*dest;
			
	g_return_val_if_fail (src != NULL, NULL);

	if( src->type == NF_NOTIFY_POINTER)
	{
		if( src->p.ptr == NULL || src->p.len == 0)
			g_return_val_if_reached(NULL);
	}
		
	dest = nf_notify_new();
	if(dest)
	{
		memcpy( dest, src, sizeof(NF_NOTIFY_INFO) );
				
		if( src->type == NF_NOTIFY_POINTER)
		{
			gpointer tmp;			
			// choissi workaround VAC 2013-07-17 오후 11:13:25			
			if (src->p.len > 1024*1024) {				

				char buf[128];
				snprintf(buf, 128,  "VAC workaround size[%d]", src->p.len);
				nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, buf);
				
				nf_debug_hexdump( src, sizeof( NF_NOTIFY_INFO ));
				nf_debug_hexdump( src->p.ptr, 280); // maybe NF_LOG_DATA 
				nf_notify_free(dest);
				
				return NULL;
			}				
/*			
if (src->p.len > 1373000000) {
	_show_backtrace();
	nf_debug_hexdump( src, sizeof( NF_NOTIFY_INFO ));
	nf_debug_hexdump( src->p.ptr, 256);
	my_command("notify_dump");
}
printf("SKSHIN-3] [%d]\n", src->p.len);	
*/
			tmp = g_malloc( src->p.len);
			if(tmp)
			{				
				dest->p.ptr = tmp;
				memcpy( dest->p.ptr, src->p.ptr, src->p.len);				
			}else{			
				g_free( dest );
				dest = NULL;
			}
			
			// FOR Test  choissi 2013-07-17 오후 11:09:49
			// ksi_test
			// dest->p.reserved[0] = dest->p.ptr;
			// dest->p.reserved[1] = dest->p.len;
		}
	}		
	return dest;
}

/**
	@brief				notify item free
	@return				void
*/
void
nf_notify_free( NF_NOTIFY_INFO *src  )
{	
	// free할때 variable size data 이면 알맹이도 free		
	g_return_if_fail (src != NULL);
	
	if( src->type == NF_NOTIFY_POINTER )
	{
		g_return_if_fail ( src->p.ptr != NULL);
		g_free(src->p.ptr);
	}

#if 0		// 일딴 보류
	#define FREE_MAGIC_KEY	0x12343398
	src->type = FREE_MAGIC_KEY;	
#endif
	g_free(src);
	
	//g_message ("%s 0x%p",  __FUNCTION__, src);	
	return;
}

#ifdef TEST_NOTIFY_API
static void
dummy_notify_api( NF_NOTIFY_INFO *pinfo, gpointer data )
{	
	g_return_val_if_fail(pinfo != NULL, 0);

	g_message("%s cb_data[%s] type[%d] [%d][%d][%d][%d] ", __FUNCTION__,  
					(char *)data,
					pinfo->type,
					pinfo->d.params[0],
					pinfo->d.params[1],
					pinfo->d.params[2],
					pinfo->d.params[3] );	
}
#endif

static void
nf_notify_test()
{
	
#ifdef TEST_NOTIFY_API
{
	gulong 		cb_handle = 0;
	gboolean	cb_remove_ret = 0;
	
	NF_NOTIFY_INFO	*pinfo;
	
	cb_handle= nf_notify_connect_cb( "sensor", dummy_notify_api, "sensor");
	g_message("%s connected cb_handle[%u]",__FUNCTION__, cb_handle);

	cb_handle= nf_notify_connect_cb( "sensor", dummy_notify_api, "sensor");
	g_message("%s connected cb_handle[%u]",__FUNCTION__, cb_handle);
			
	pinfo = nf_notify_new();
		
	pinfo->d.params[0] = 3;
	pinfo->d.params[1] = 3;
	pinfo->d.params[2] = 9;
	pinfo->d.params[3] = 8;
			
	g_message("%s object set",__FUNCTION__);
	g_object_set(_nf_notify, "sensor", pinfo, NULL);
		
	cb_remove_ret = nf_notify_remove_cb ("sensor", cb_handle);
	g_message("%s remove cb_handle ret[%d]",__FUNCTION__, cb_remove_ret);	
	
	pinfo->d.params[3] = 9;
		
	g_message("%s object set",__FUNCTION__);
	g_object_set(_nf_notify, "sensor", pinfo, NULL);
		
#if 1
	pinfo->d.params[0] = 9999;
	nf_notify_fire( "sensor", pinfo);
	
	nf_notify_fire_params( "sensor", 111,2,3,444);
	nf_notify_fire_params( "sensor", 222,2,3,444);
	
	pinfo->d.params[0] = 8888;
	nf_notify_fire( "sensor", pinfo);	
#endif

	cb_handle= nf_notify_connect_cb( "ipcam_rec", dummy_notify_api, "ipcam_rec");
	g_message("%s connected cb_handle[%u]",__FUNCTION__, cb_handle);

	nf_notify_fire_params( "ipcam_rec", 111,2,3,444);
	nf_notify_fire_params( "ipcam_rec", 111,2,3,555);
	
	nf_notify_free( pinfo);	
}
#endif	
	
}
            
static gint _convet_param2( char mode )
{
	gint ret = 0;			
	switch  (mode)
	{
		case 0x0:
		case ' ':
		case 'p':
		case 'U':
		case 'T': ret = LP2_RECORD_STARTED_TIMER;  break;
		case 'P': ret = LP2_RECORD_STARTED_PANIC;  break;
		case 'A': ret = LP2_RECORD_STARTED_SENSOR; break;		
		case 'M': ret = LP2_RECORD_STARTED_MOTION; break;
		default:
			g_return_val_if_fail( ret, LP2_RECORD_STARTED_TIMER);
	}								
	return ret;
}
static void
_notify_cb_log_put_record( NF_NOTIFY_INFO *pinfo, gpointer data )
{		
	gint prop_id = (gint)data;
	gint i, log_type, param2;
	gchar *new_val, *old_val;

#if 0
	g_return_val_if_fail(pinfo != NULL, 0);	
#else
	if(pinfo == NULL)
	{
		g_warning("%s pinfo is NULL!!", __FUNCTION__);
		return ;
	}
#endif

	g_assert( prop_id>0 && prop_id < LAST_PROP);	
	g_assert( pinfo->type == NF_NOTIFY_CHMAP);

#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT_REC] 
		|| _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT_REC_SIMPLE] )
		g_message("%s prop_id[%d][%s] [%ld.%06ld] type[%d] [%-32.32s] ", __FUNCTION__,  
					prop_id, _g_notify_info[prop_id].name,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type,
					pinfo->c.chmap );
#endif

	new_val = pinfo->c.chmap;
	old_val = _g_notify_arg[prop_id].c.chmap;
		
	if( !memcmp(old_val, new_val, 16) )
	{
#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT_REC] )
		g_message("%s  same value!!", __FUNCTION__);
#endif
		return;
	}
			
	//for(i=0; i<NUM_ANALOG_CHANNEL; i++)
	for(i=0; i<NUM_ACTIVE_CH; i++)
	{			
		if( old_val[i] != new_val[i] )
		{

#ifdef DEBUG_NOTIFY_LOG
			if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT_REC] )
				g_message("%s prop_id[%d][%s] ch[%2d] [%c]-->[%c] ", __FUNCTION__,		
						prop_id, _g_notify_info[prop_id].name, i, 
						(old_val[i] == 0) ? ' ' : old_val[i] , new_val[i] );
#endif		
			if( !(old_val[i] == NF_RECORD_REASON_CHAR_PRE 
					||  old_val[i] == NF_RECORD_REASON_CHAR_NOTHING
					||  old_val[i] == 0x00 )  )
			{
				log_type = LT_RECORD_STOPPED;
				param2 = _convet_param2( old_val[i]);

				nf_eventlog_put_param( &pinfo->timestamp, log_type, 
										i /* ch_num */, 
										param2 /* record_reason */,
										NULL);

#ifdef DEBUG_NOTIFY_MOTION_LOG_SKIP
				if(param2 == LP2_RECORD_STARTED_MOTION)
					nf_eventlog_put_param( &pinfo->timestamp, LT_MOTION_DETECTION, 
										i /* ch_num */, 
										0 /* on/off */,
										NULL);

#endif				
#ifdef DEBUG_NOTIFY_ALARM_LOG_SKIP		
				if(param2 == LP2_RECORD_STARTED_SENSOR)
					nf_eventlog_put_param( &pinfo->timestamp, LT_SENSOR_INPUT, 
										i /* ch_num */, 
										0 /* on/off */,
										NULL);
#endif

			}

		}
	}

	//for(i=0; i<NUM_ANALOG_CHANNEL; i++)
	for(i=0; i<NUM_ACTIVE_CH; i++)
	{			
		if( old_val[i] != new_val[i] )
		{

			if( !(new_val[i] == NF_RECORD_REASON_CHAR_PRE 
				|| new_val[i] == NF_RECORD_REASON_CHAR_NOTHING)  )
			{

				log_type = LT_RECORD_STARTED;
				param2 = _convet_param2( new_val[i] );
			
				nf_eventlog_put_param( &pinfo->timestamp, log_type, 
										i /* ch_num */, 
										param2 /* record_reason */,
										NULL);

#ifdef DEBUG_NOTIFY_MOTION_LOG_SKIP
				if(param2 == LP2_RECORD_STARTED_MOTION)
					nf_eventlog_put_param( &pinfo->timestamp, LT_MOTION_DETECTION, 
										i /* ch_num */, 
										1 /* on/off */,
										NULL);

#endif				
#ifdef DEBUG_NOTIFY_ALARM_LOG_SKIP		
				if(param2 == LP2_RECORD_STARTED_SENSOR)
					nf_eventlog_put_param( &pinfo->timestamp, LT_SENSOR_INPUT, 
										i /* ch_num */, 
										1 /* on/off */,
										NULL);
#endif		
										
			}
		}
	}	
	memcpy(&_g_notify_arg[prop_id],pinfo,sizeof(NF_NOTIFY_INFO));
}

static void
_notify_cb_log_put( NF_NOTIFY_INFO *pinfo, gpointer data )
{		
	gint prop_id = (gint)data;
	gint log_type = 0, i, param2;
	guint new_val, old_val;
	guint m_new, m_old;

#if 0
	g_return_val_if_fail(pinfo != NULL, 0);	
#else
	if(pinfo == NULL)
	{
		g_warning("%s pinfo is NULL!!", __FUNCTION__);
		return ;
	}
#endif

	g_assert( prop_id>0 && prop_id < LAST_PROP);

#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT]
		|| _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT_SIMPLE] )		
			g_message("%s prop_id[%d][%s] [%ld.%06ld] type[%d] [%08x][%x][%x][%x] ", __FUNCTION__,  
					prop_id, _g_notify_info[prop_id].name,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type,
					pinfo->d.params[0],
					pinfo->d.params[1],
					pinfo->d.params[2],
					pinfo->d.params[3] );	
#endif

	new_val	= pinfo->d.params[0];
	old_val = _g_notify_arg[prop_id].d.params[0];	
	if( old_val == new_val )
	{
#ifdef DEBUG_NOTIFY_LOG
		if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT] )
			g_message("%s  same value!! skip old[%08x] new[%08x]", __FUNCTION__,  old_val, new_val);						
#endif
		return;
	}
	
	if( prop_id == PROP_MOTION )
		log_type = LT_MOTION_DETECTION;
	else if( prop_id == PROP_VLOSS )
		log_type = LT_VIDEO_LOSS;
	else if( prop_id == PROP_SENSOR )
		log_type = LT_SENSOR_INPUT;
	else
		g_assert(0);		
		

#ifdef ENABLE_HNF_IPCAM
	for (i = 0; i < NUM_TOTAL_CHANNEL; i++) {
		if( i == NUM_ACTIVE_CH && i != BASE_IPCAM_CHANNEL ) {
			i = BASE_IPCAM_CHANNEL;
			continue;
		}		
#else
	for (i = 0; i < NUM_ACTIVE_CH; ++i) {
#endif	

		m_new = new_val & (1<<i);
		m_old = old_val & (1<<i);
		
		if( m_new != m_old )
		{
			if(log_type == LT_VIDEO_LOSS && m_new == 0 ) 
				log_type = LT_VIDEO_IN;
			
			param2 = (m_new) ? 1:0;
											
#ifdef DEBUG_NOTIFY_LOG
			if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_LOGPUT] )
				g_message("%s prop_id[%d][%s] ch[%2d] [%d]-->[%d] ", __FUNCTION__,
						prop_id, _g_notify_info[prop_id].name, i,  m_old, m_new);
#endif								


#ifdef DEBUG_NOTIFY_MOTION_LOG_SKIP			
			if( log_type == LT_MOTION_DETECTION )
				continue;
#endif				
#ifdef DEBUG_NOTIFY_ALARM_LOG_SKIP			
			if( log_type == LT_SENSOR_INPUT )
				continue;
#endif				
				
			nf_eventlog_put_param( &pinfo->timestamp, log_type, 
									i /* ch_num */, 
									param2 /* onoff */, 
									NULL);
		}
	}
	
	memcpy(&_g_notify_arg[prop_id],pinfo,sizeof(NF_NOTIFY_INFO));
}

static gboolean 
_is_sysdb_change_tformat( guint *tform, guint *dform)
{	
	static guint timeform = 0x33981234;
	static guint dateform = 0x33981234;
	guint cur_timeform, cur_dateform;
			
	cur_timeform = nf_sysdb_get_uint("sys.date.timeform");	
	cur_dateform = nf_sysdb_get_uint("sys.date.dateform");
	
	if( cur_timeform != timeform || cur_dateform != dateform ) {
		*tform = timeform = cur_timeform;
		*dform = dateform = cur_dateform;		
		return 1;		
	}else {		
		return 0;
	}
}

static gboolean 
_is_sysdb_change_tzone( guint *tzone, guint *is_dst)
{	
	static guint timezone = 0x33981234;		
	static gboolean dst = 0x33981234;	
	
	guint		cur_timezone;
	gboolean  	cur_dst;

	cur_dst = nf_sysdb_get_bool("sys.date.daylight");	
	cur_timezone = nf_sysdb_get_uint("sys.date.tz_index");		
	
	
	if( cur_timezone != timezone || cur_dst != dst ) {
		*tzone = timezone = cur_timezone;			
		*is_dst = dst = cur_dst;
		return 1;		
	}else {		
		return 0;
	}
}

static gboolean 
_is_sysdb_change_covert( guint *co )
{	
#if defined(_SNF_0824) || defined(_SNF_0424) || defined(_SNF_1648) 
	static guint usrcovert[8] = { 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000 };
	static guint camcovert[4] = { 0x10000, 0x10000, 0x10000, 0x10000 }; // bitval for covert of each channel

	guint cur_usrcovert = 0;
	guint cur_camcovert[4] = {0,};
    guint this_usr_covert = 0;
    
    gboolean usr_changed = FALSE, cam_changed = FALSE;

    gchar covstr[33] = {0, };
    gchar ret = 0;
    gchar login_user[17] = {0,};

    gchar uname[33] = {0,},
	grpname[33] = {0,};

    NF_NOTIFY_INFO *pnotify;    
	guint i, ch;
    guint usrcnt;
    int grpidx = 0;

    gboolean usr_found = FALSE;

    pnotify = nf_notify_get("login_user");

    if( !pnotify )   { 
        g_warning("%s No notify [%s]", __func__, "login_user"); 
    } else { 
        snprintf( login_user, sizeof(login_user), "%s", pnotify->c.chmap);
        nf_notify_free(pnotify); 
    } 

    usrcnt = nf_sysdb_get_uint("usr.UCNT");

    // check user covert setting and logon user
	for( i=0 ; i < usrcnt ; i++ ) {
        char buf[128];		

        snprintf(buf, sizeof(buf), "usr.U%d.covert", i);		
        snprintf(covstr, sizeof(covstr), "%s", nf_sysdb_get_str_nocopy(buf));

        cur_usrcovert = 0;

        for( ch=0 ; ch < NUM_ACTIVE_CH ; ch++ ) {
            if( covstr[ch] == '1' ) {
                cur_usrcovert |= (1 << ch);
            }
        }	

        // check if the covert setting is changed
        if( cur_usrcovert != usrcovert[i] ) {
            usrcovert[i] = cur_usrcovert;				
            usr_changed = TRUE;
        }

        if( strlen(login_user) > 0 ) {
            snprintf(buf, sizeof(buf), "usr.U%d.name", i);		
            snprintf(uname, sizeof(uname), "%s", nf_sysdb_get_str_nocopy(buf));

            if( strcmp(login_user, uname) == 0 ) {
                // found user logged in
                usr_found = TRUE;
                this_usr_covert = cur_usrcovert;
                snprintf(buf, sizeof(buf), "usr.U%d.grpname", i);		
                snprintf(grpname, sizeof(grpname), "%s", nf_sysdb_get_str_nocopy(buf));
            }
        }
	}

        // check camera covert and group's covert setting
	for( ch=0; ch < NUM_ACTIVE_CH; ch++)
	{
		char buf[128];		

		snprintf(buf, sizeof(buf), "cam.C%d.cv_admin", ch);
		if(nf_sysdb_get_bool(buf))
		{
			cur_camcovert[0] |= (1 << ch);
		}

		snprintf(buf, sizeof(buf), "cam.C%d.cv_manager", ch);
		if(nf_sysdb_get_bool(buf))
		{
			cur_camcovert[1] |= (1 << ch);
		}

		snprintf(buf, sizeof(buf), "cam.C%d.cv_user", ch);
		if(nf_sysdb_get_bool(buf))
		{
			cur_camcovert[2] |= (1 << ch);
		}

		snprintf(buf, sizeof(buf), "cam.C%d.cv_logoff", ch);
		if(nf_sysdb_get_bool(buf))
		{
			cur_camcovert[3] |= (1 << ch);
		}
	}

	for( i=0 ; i < 4 ; i++ ) {
	    if( camcovert[i] != cur_camcovert[i] ) {
	        camcovert[i] = cur_camcovert[i];
	        cam_changed = TRUE;
	    }
	}
	
	g_message("%s grp[%s] usr_c[0x%x]", __func__, grpname, this_usr_covert);
	
	if( usr_changed || cam_changed ) {
	    if( strcmp(grpname, "ADMIN") == 0 ) {
	        grpidx = 0;
	    } else if( strcmp(grpname, "MANAGER") == 0 ) {
	        grpidx = 1;
	    } else if( strcmp(grpname, "USER") == 0 ) {
	        grpidx = 2;
	    } else {
	        grpidx = 3;
	    }
	
	    this_usr_covert |= cur_camcovert[grpidx];
	    ret = 1;
	    
	} else {
	    ret = 0;
	    grpidx = 3;
	}
	
	*co = this_usr_covert | cur_camcovert[grpidx];
	
	g_message("%s ret[%d] cam_c[0x%x] grpidx[%d] fin_c[0x%x]", __func__, ret, cur_camcovert[grpidx], grpidx, *co);
	
	return ret;
	
#else
	static guint covert = 0x33981234;	
	guint cur_covert = 0;
	int i;
								
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{
		char buf[128];		
		snprintf(buf, sizeof(buf), "cam.C%d.covert", i);		
		if(nf_sysdb_get_bool(buf))
		{
			cur_covert |= (1 << i);
		}	
	}
#ifdef ENABLE_HNF_IPCAM
	for(i=0;i<NUM_IPCAM_CHANNEL;i++)
	{
		char buf[128];		
		snprintf(buf, sizeof(buf), "ipcam.C%d.covert", i);		
		if(nf_sysdb_get_bool(buf))
		{
			cur_covert |= (1 << (i+ BASE_IPCAM_CHANNEL));
		}	
	}
#endif
							
	if( cur_covert != covert ) {
		*co = covert = cur_covert;				
		return 1;		
	}else {		
		return 0;
	}
#endif

}


static void
_notify_cb_sysdb_chnage( NF_NOTIFY_INFO *pinfo, gpointer data )
{		
	guint param1 = 0, param2 = 0;
#if 0	
	g_return_val_if_fail(pinfo != NULL, 0);	
#else
	if(pinfo == NULL)
	{
		g_warning("%s pinfo is NULL!!", __FUNCTION__);
		return ;
	}
#endif

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS){	
				
		if( _is_sysdb_change_tformat( &param1 /* tform */ , &param2 /* dform */) )
			nf_notify_fire_params("sysdb_tformat", param1, param2, 0, 0);

		if( _is_sysdb_change_tzone( &param1 /* tz_index */, &param2 /* is_dst */) )
			nf_notify_fire_params("sysdb_tzone", param1, param2, 0, 0);
#ifdef ENABLE_HNF_IPCAM										
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM 
				|| pinfo->d.params[0] == NF_SYSDB_CATE_IPCAM ){
#else
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM ){
#endif
		nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);
		
		if( _is_sysdb_change_covert( &param1 ) )
			nf_notify_fire_params("sysdb_covert", param1, 0, 0, 0);
			
		nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);		
	
	}
	
}
	
static void 
_nf_notify_cb_init()
{
	guint param1 = 0, param2 = 0;
	gulong cb_handle = 0;
	
	memset(_g_notify_arg, 0x00, sizeof(_g_notify_arg));

	cb_handle= nf_notify_connect_cb( _g_notify_info[PROP_SENSOR].name, _notify_cb_log_put, (gpointer)PROP_SENSOR);
	g_message("%s connected log_put[%s] cb_handle[%ld]",__FUNCTION__, _g_notify_info[PROP_SENSOR].name, cb_handle);	
	g_assert( cb_handle >0);

	cb_handle= nf_notify_connect_cb( _g_notify_info[PROP_MOTION].name, _notify_cb_log_put, (gpointer)PROP_MOTION);
	g_message("%s connected log_put[%s] cb_handle[%ld]",__FUNCTION__, _g_notify_info[PROP_MOTION].name, cb_handle);	
	g_assert( cb_handle >0);

	cb_handle= nf_notify_connect_cb( _g_notify_info[PROP_VLOSS].name, _notify_cb_log_put, (gpointer)PROP_VLOSS);
	g_message("%s connected log_put[%s] cb_handle[%ld]",__FUNCTION__, _g_notify_info[PROP_VLOSS].name, cb_handle);	
	g_assert( cb_handle >0);

	cb_handle= nf_notify_connect_cb( _g_notify_info[PROP_ANALOG_REC].name, _notify_cb_log_put_record, (gpointer)PROP_ANALOG_REC);
	g_message("%s connected log_put[%s] cb_handle[%ld]",__FUNCTION__, _g_notify_info[PROP_ANALOG_REC].name, cb_handle);	
	g_assert( cb_handle >0);	

	cb_handle= nf_notify_connect_cb( _g_notify_info[PROP_SYSDB_CHANGE].name, _notify_cb_sysdb_chnage, (gpointer)PROP_SYSDB_CHANGE);
	g_message("%s connected log_put[%s] cb_handle[%ld]",__FUNCTION__, _g_notify_info[PROP_SYSDB_CHANGE].name, cb_handle);	
	g_assert( cb_handle >0);	


	
	_is_sysdb_change_tformat( &param1 /* tform */ , &param2 /* dform */);
	nf_notify_fire_params("sysdb_tformat", param1, param2, 0, 0);
	
	_is_sysdb_change_tzone( &param1 /* tz_index */, &param2 /* is_dst */);
	nf_notify_fire_params("sysdb_tzone", param1, param2, 0, 0);
	
	_is_sysdb_change_covert( &param1 );	
	nf_notify_fire_params("sysdb_covert", param1, 0, 0, 0);
	
	return;
}

#ifdef DEBUG_RAND_ALARM

//static GRand* _g_prand;

static gboolean
_notify_timer_cb_rand_alaram (gpointer data)
{
	static gint cnt = 0;
	guint ch_mask = 0;	
	gint i;
	gchar analog_rec[16+1];
		
	g_message("%s %d", __FUNCTION__, ++cnt);
	
	memset( analog_rec, ' ', sizeof(analog_rec));	
	analog_rec[ sizeof(analog_rec) - 1 ] = 0;

	ch_mask = g_random_int();
	
	for(i=0;i<NUM_ANALOG_CHANNEL;i++)
	{
		if( ch_mask & (1<<i) )
			analog_rec[i] = 'A';
	}	
	g_message("%s 0x%08x [%16.16s]", __FUNCTION__, ch_mask, analog_rec);
	
	nf_notify_fire_chmap("analog_rec", analog_rec);
	
	return TRUE;
}


static gboolean
_notify_timer_cb_rand_alarm_single (gpointer data)
{
	static gint cnt = 0;
	gint i;	
	static gchar analog_rec[16+1];
		
	g_message("%s %d", __FUNCTION__, ++cnt);
	if(cnt == 1)	
	{
		memset( analog_rec, 'T', sizeof(analog_rec));	
		analog_rec[ sizeof(analog_rec) - 1 ] = 0;
	}

	i = g_random_int() % NUM_ANALOG_CHANNEL;
	
	if( analog_rec[i] =='A' )
		analog_rec[i] = 'T';
	else
		analog_rec[i] = 'A';
		
	g_message("%s [%16.16s]", __FUNCTION__, analog_rec);
	
	nf_notify_fire_chmap("analog_rec", analog_rec);	
	return TRUE;
}

#endif


/*
	GMainLoop에 timer source를 사용해서 타이머 구현
	
	nf_host main thread가 동작하면서 NF_Context,NF_Loop 등이 초기화 되는데
	이 context와 loop에서 timer 구현된다.		
*/


/**
	@brief				notify 초기화
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_notify_init(int wait)
{
	gboolean ret = TRUE;			
	g_return_val_if_fail (_nf_notify == NULL, FALSE);		
	
	_nf_notify = g_object_new ( NF_TYPE_NOTIFY , NULL);	
		
	nf_debug_category_add( "notify", _DEBUG_NOTIFY_str, _DEBUG_NOTIFY_log, DEBUG_NOTIFY_IDX_NR);
#if defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
	nf_notify_fire_chmap("analog_rec","                                ");
#else
	nf_notify_fire_chmap("analog_rec","                ");
#endif
	// not used ???
	//nf_notify_fire_chmap("ipcam_rec","                ");
		
	nf_notify_fire_chmap("login_user", "");

	_nf_notify_cb_init();

#ifdef DEBUG_RAND_ALARM
	//_g_prand =  g_rand_new_with_seed(3398);		
	//nf_timer_add( 3*1000, _notify_timer_cb_rand_alaram, NULL);	
	nf_timer_add( 3*1000, _notify_timer_cb_rand_alarm_single, NULL);	
#endif
 
#ifdef TEST_NOTIFY_API
	nf_notify_test();	
#endif	

	if( wait )
	{
		while( _nf_notify->init_done != 1)
			g_usleep(10*1000);
	}
	
	return ret;
}


static GParamSpec*			_last_pspec = NULL;
static NF_NOTIFY_CALLBACK*	_last_cb_data = NULL;
		
static void
wrap_notify_cb (GObject    *object, GParamSpec *pspec, gpointer data)
{		
	NF_NOTIFY_CALLBACK	*pcb = (NF_NOTIFY_CALLBACK *)data;
	NF_NOTIFY_INFO		*ptmp;

	g_return_if_fail (pspec != NULL);
	g_return_if_fail (data != NULL);

#ifdef DEBUG_NOTIFY_LOG
	if( _DEBUG_NOTIFY_log[DEBUG_NOTIFY_IDX_CALLBACK] )
		g_message("%s [%s][%s][%p][%p]",__FUNCTION__,
			NF_OBJECT_NAME(object), pspec->name, data, pcb->cb_func );
#endif
		
	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);

	_last_pspec = pspec;
	_last_cb_data = pcb;

#if 1	// no dup version
	pcb->cb_func( ptmp, pcb->cb_data );		
/*	
	static int cnt = 0;	
	if( strcmp(pspec->name,"motion") == 0 && ++cnt > 1000 )
		while(1);
*/		
#else						
	pinfo = nf_notify_dup( ptmp);	
	pcb->cb_func( pinfo, pcb->cb_data );
	nf_notify_free( pinfo);
#endif
	
}


static GStaticMutex cb_mutex = G_STATIC_MUTEX_INIT;

/**
	@brief				notify callback function connect
	@return	gulong		the handler id
*/
gulong 
nf_notify_connect_cb(const gchar *property_name, NF_NOTIFY_CB_FUNC cb_func, gpointer data)
{

	NF_NOTIFY_CALLBACK	*pcb;
	GParamSpec			*pspec = NULL;	
	GList				*plist = NULL;
	gchar 				tmp[256];
	
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);	
	g_return_val_if_fail (cb_func != NULL, 0);			
		
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), property_name);

	g_return_val_if_fail ( pspec != NULL, 0);

	pcb = g_malloc0( sizeof(NF_NOTIFY_CALLBACK) );

	g_return_val_if_fail ( pcb != NULL, 0);

	pcb->cb_func = cb_func;
	pcb->cb_data = data;
	pcb->pspec = pspec;
			
	snprintf( tmp, sizeof(tmp), "notify::%s", g_param_spec_get_name(pspec) );

	pcb->cb_handle = g_signal_connect (_nf_notify, tmp, 
										G_CALLBACK (wrap_notify_cb), pcb); 
    g_static_mutex_lock (&cb_mutex);

    plist = g_param_spec_get_qdata (pspec, quark_cb_list);        
	plist = g_list_append (plist, (gpointer)pcb );
	g_param_spec_set_qdata (pspec, quark_cb_list, (gpointer)plist  );

	g_static_mutex_unlock (&cb_mutex);
							
	return pcb->cb_handle;
}

static gint	
_cb_list_compare (  gconstpointer a, gconstpointer b )
{					
	NF_NOTIFY_CALLBACK	*pcb = NULL;
	
	//g_message( "%d %d", pcb->cb_handle, b);		
	if(a == NULL)
		return -1;	
	
	pcb = (NF_NOTIFY_CALLBACK *)a;
		
	return !( pcb->cb_handle == (gulong)b) ;
}

/**
	@brief				notify remove callback function 
	@return	gboolean	TRUE if success
*/
gboolean
nf_notify_remove_cb(const gchar *property_name, gulong handler_id)
{
	GParamSpec			*pspec;	
	GList				*plist = NULL, *pitem = NULL;
	
	g_return_val_if_fail (_nf_notify != NULL, FALSE);
	g_return_val_if_fail (property_name != NULL, FALSE);	
	g_return_val_if_fail (handler_id >0, FALSE);	
		
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
																property_name);

	g_return_val_if_fail ( pspec != NULL, 0);
	
    g_static_mutex_lock (&cb_mutex);    

    plist = g_param_spec_get_qdata (pspec, quark_cb_list);	                                             
	pitem = g_list_find_custom (plist, GINT_TO_POINTER(handler_id), 
											(GCompareFunc)_cb_list_compare );
	if(pitem)
	{
		g_signal_handler_disconnect( _nf_notify, handler_id );
		
		g_free( pitem->data );	// free  _NF_NOTIFY_CALLBACK
		plist = g_list_remove (plist, pitem->data );
		g_param_spec_set_qdata (pspec, quark_cb_list, (gpointer)plist  );		
	}		

	g_static_mutex_unlock (&cb_mutex);
	
	if(pitem)
		return TRUE;
									
	return FALSE;
}


/**
	@brief				notify callback function ?
	@return	gboolean	%TRUE on is connected
*/
gboolean
nf_notify_isconn_cb_byid(const gchar *property_name, gulong handler_id)
{
	return 0;
}

/**
	@brief				notify callback function ?
	@return	gboolean	%TRUE on is connected
*/
gboolean
nf_notify_isconn_cb_byfunc(const gchar *property_name, NF_NOTIFY_CB_FUNC cb)
{
	return 0;
}

/**
	@brief				notify callback function send message
	@return	gboolean	%TRUE on success
*/
gboolean
nf_notify_fire(const gchar *property_name, NF_NOTIFY_INFO *pitem)
{

	GParamSpec			*pspec = NULL;		
	NF_NOTIFY_QITEM		*pqitem = NULL;
		
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);
	g_return_val_if_fail (pitem != NULL, 0);	
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);
	g_return_val_if_fail ( pspec != NULL, 0);

	pqitem = g_malloc(sizeof(NF_NOTIFY_QITEM));			
	if( !pqitem )
		return 0;
	
	pqitem->pspec = pspec;
	pqitem->pitem = nf_notify_dup(pitem);
	
	g_assert( pqitem->pitem != NULL );
			
#ifndef DEBUG_NOTIFY_DIRECT_FIRE
	g_async_queue_push ( _nf_notify->queue, pqitem);
#else
	property_name = g_param_spec_get_nick(pqitem->pspec);
			
	g_object_set(_nf_notify, property_name, pqitem->pitem , NULL);
			
	nf_notify_free ( pqitem->pitem ); // NF_NOTIFY_INFO
	g_free ( pqitem ); //NF_NOTIFY_QITEM	
#endif
			
	//g_param_spec_get_name ()	
	return 1;
}


/**
	@brief				notify callback function send message
	@return	gboolean	%TRUE on success
*/
gboolean
nf_notify_fire_params(const gchar *property_name,
						guint param0, guint param1, guint param2, guint param3)
{

	GParamSpec			*pspec = NULL;		
	NF_NOTIFY_QITEM		*pqitem = NULL;
	
	NF_NOTIFY_INFO 		*pitem = NULL;
			
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);
				
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
											property_name);

	g_return_val_if_fail ( pspec != NULL, 0);
	
	pqitem = g_malloc(sizeof(NF_NOTIFY_QITEM));	
	if( !pqitem )
		goto no_mem;
	
	pitem = nf_notify_new();
	if( !pitem )	
		goto no_mem;

	pitem->type = NF_NOTIFY_PARAM;
	//g_get_current_time(&pitem->timestamp);
	gettimeofday((struct timeval *)&pitem->timestamp, NULL);
		
	pitem->d.params[0] = param0;
	pitem->d.params[1] = param1;
	pitem->d.params[2] = param2;
	pitem->d.params[3] = param3;	
				
	pqitem->pspec = pspec;
	pqitem->pitem = pitem;

#ifndef DEBUG_NOTIFY_DIRECT_FIRE
	g_async_queue_push ( _nf_notify->queue, pqitem);
#else
	property_name = g_param_spec_get_nick(pqitem->pspec);
			
	g_object_set(_nf_notify, property_name, pqitem->pitem , NULL);
			
	nf_notify_free ( pqitem->pitem ); // NF_NOTIFY_INFO
	g_free ( pqitem ); //NF_NOTIFY_QITEM	
#endif

	return 1;
	
no_mem:
	
	if( pqitem ) 
		g_free( pqitem );

	if( pitem )
		nf_notify_free( pitem);
			
	return 0;
}


/**
	@brief				notify callback function send message
	@return	gboolean	%TRUE on success
*/
gboolean
nf_notify_fire_chmap(const gchar *property_name, gchar *chmap)
{

	GParamSpec			*pspec = NULL;		
	NF_NOTIFY_QITEM		*pqitem = NULL;
	
	NF_NOTIFY_INFO 		*pitem = NULL;
			
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);
				
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
											property_name);

	g_return_val_if_fail ( pspec != NULL, 0);
	
	pqitem = g_malloc(sizeof(NF_NOTIFY_QITEM));	
	if( !pqitem )
		goto no_mem;
	
	pitem = nf_notify_new();
	if( !pitem )	
		goto no_mem;

	pitem->type = NF_NOTIFY_CHMAP;
	//g_get_current_time(&pitem->timestamp);
	gettimeofday((struct timeval *)&pitem->timestamp, NULL);

#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	size_t chmap_len = strlen(chmap);
	if (chmap_len < 32) {
        g_warning("chmap buffer too small: %zu bytes, need 32 bytes", chmap_len);
		goto no_mem;
	}	
	memcpy(pitem->c.chmap, chmap, 32);
#else
	memcpy(pitem->c.chmap, chmap, 16);
#endif
									
	pqitem->pspec = pspec;
	pqitem->pitem = pitem;
				
#ifndef DEBUG_NOTIFY_DIRECT_FIRE
	g_async_queue_push ( _nf_notify->queue, pqitem);
#else
	property_name = g_param_spec_get_nick(pqitem->pspec);
			
	g_object_set(_nf_notify, property_name, pqitem->pitem , NULL);
			
	nf_notify_free ( pqitem->pitem ); // NF_NOTIFY_INFO
	g_free ( pqitem ); //NF_NOTIFY_QITEM	
#endif
				
	return 1;
	
no_mem:
	
	if( pqitem ) 
		g_free( pqitem );

	if( pitem )
		nf_notify_free( pitem);
			
	return 0;
}


/**
	@brief				notify callback function send message
	@return	gboolean	%TRUE on success
*/
gboolean
nf_notify_fire_pointer(const gchar *property_name, gpointer data, gint size)
{

	GParamSpec			*pspec = NULL;		
	NF_NOTIFY_QITEM		*pqitem = NULL;
	
	NF_NOTIFY_INFO 		*pitem = NULL;
			
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);
	
	g_return_val_if_fail (data != NULL, 0);
	//captainnn
	//g_return_val_if_fail (size >0 && size <4096, 0);
				
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
											property_name);

	g_return_val_if_fail ( pspec != NULL, 0);
	
	pqitem = g_malloc(sizeof(NF_NOTIFY_QITEM));	
	if( !pqitem )
		goto no_mem;
	
	pitem = nf_notify_new_size( (guint)size);
	if( !pitem )
		goto no_mem;

	pitem->type = NF_NOTIFY_POINTER;	
	//g_get_current_time(&pitem->timestamp);
	gettimeofday((struct timeval *)&pitem->timestamp, NULL);

	// data copy
	memcpy( pitem->p.ptr, data, (guint)size);
	pitem->p.len = (guint)size;

	pqitem->pspec = pspec;
	pqitem->pitem = pitem;
				
#ifndef DEBUG_NOTIFY_DIRECT_FIRE
	g_async_queue_push ( _nf_notify->queue, pqitem);	
#else
	property_name = g_param_spec_get_nick(pqitem->pspec);
			
	g_object_set(_nf_notify, property_name, pqitem->pitem , NULL);
			
	nf_notify_free ( pqitem->pitem ); // NF_NOTIFY_INFO
	g_free ( pqitem ); //NF_NOTIFY_QITEM	
#endif
				
	return 1;
	
no_mem:
	
	if( pqitem ) 
		g_free( pqitem );

	if( pitem )
		nf_notify_free( pitem);
			
	return 0;
}

/**
	@brief				notify notify data query
	@return	gulong		data pointer on success
*/
gpointer
nf_notify_get(const gchar *property_name)
{

	GParamSpec			*pspec;		
	NF_NOTIFY_INFO		*pinfo, *ptmp;
	
	g_return_val_if_fail (_nf_notify != NULL, NULL);
	g_return_val_if_fail (property_name != NULL, NULL);	
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);
	g_return_val_if_fail ( pspec != NULL, NULL);
	
	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);				
	pinfo = nf_notify_dup( ptmp);
	
	//nf_notify_free( pinfo);
	return pinfo;
}

guint
nf_notify_get_param0(const gchar *property_name)
{
	
	GParamSpec			*pspec;		
	NF_NOTIFY_INFO		*ptmp;
	
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);

	g_return_val_if_fail ( pspec != NULL, 0);

	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);

	g_return_val_if_fail ( ptmp != NULL, 0);

	return ptmp->d.params[0];				
}

guint
nf_notify_get_param1(const gchar *property_name)
{

	GParamSpec			*pspec;		
	NF_NOTIFY_INFO		*ptmp;
	
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);	
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);
	g_return_val_if_fail ( pspec != NULL, 0);

	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);

	g_return_val_if_fail ( ptmp != NULL, 0);

	return ptmp->d.params[1];	
}

guint
nf_notify_get_param_idx(const gchar *property_name, guint idx)
{

	GParamSpec			*pspec;		
	NF_NOTIFY_INFO		*ptmp;
	
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);	
	g_return_val_if_fail (idx<4, 0);	
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);
	g_return_val_if_fail ( pspec != NULL, 0);

	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);

	g_return_val_if_fail ( ptmp != NULL, 0);

	return ptmp->d.params[idx];	
}

gulong
nf_notify_get_update_time(const gchar *property_name)
{

	GParamSpec			*pspec;		
	NF_NOTIFY_INFO		*ptmp;
	
	g_return_val_if_fail (_nf_notify != NULL, 0);
	g_return_val_if_fail (property_name != NULL, 0);	
			
	pspec = g_object_class_find_property( G_OBJECT_GET_CLASS(_nf_notify), 
													property_name);
	g_return_val_if_fail ( pspec != NULL, 0);

	ptmp = g_param_spec_get_qdata (pspec, quark_data_value);

	g_return_val_if_fail ( ptmp != NULL, 0);

	return (gulong)ptmp->timestamp.tv_sec;	
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

#ifdef DEBUG_NOTIFY_JBSHELL

// notify_dump
// notify_fire_params	[name] [param1] [param2]
// notify_fire_chmap	[name] [charmap]

static char notify_dump_help[] = "notify_dump";
static int notify_dump(int argc, char **argv)
{	
	gint i=0;

	g_message("%s last notify_cb is name[%s] cb_func[%p] wowowowowowow",__FUNCTION__, 
				_last_pspec->name, _last_cb_data->cb_func );
	
	for(i=1;i<LAST_PROP;i++)
	{
		NF_NOTIFY_INFO *pinfo = (NF_NOTIFY_INFO *)_g_notify_info[i].data;
		if(pinfo == NULL) continue;
			
		printf("[%02d][%-32.32s] [%ld.%06ld]",  i, _g_notify_info[i].name,
				pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec );
				
		if(pinfo->type == NF_NOTIFY_PARAM)
			printf("t[d] [0x%08x][0x%08x][0x%08x][0x%08x]\n", 
					pinfo->d.params[0], pinfo->d.params[1], 
					pinfo->d.params[2], pinfo->d.params[3] );
		else if(pinfo->type == NF_NOTIFY_CHMAP)
			printf("t[c] [%-16.16s]\n", pinfo->c.chmap);
		else
			printf("t[p] [0x%p](%d)\n", pinfo->p.ptr, pinfo->p.len);
	}						
	return 0;
}
__commandlist(notify_dump,"notify_dump",notify_dump_help, notify_dump_help);

static char notify_dump_file_help[] = "notify_dump_file";
static int notify_dump_file(int argc, char **argv)
{	
	gint i=0;
	char buf[16*1024];
	char *buf_pos = buf;
		
	buf_pos += sprintf(buf_pos, "%s last notify_cb is name[%s] cb_func[%p] wowowowowowow\n",__FUNCTION__, 
				_last_pspec->name, _last_cb_data->cb_func );
	
	for(i=1;i<LAST_PROP;i++)
	{
		NF_NOTIFY_INFO *pinfo = (NF_NOTIFY_INFO *)_g_notify_info[i].data;
		if(pinfo == NULL) continue;
			
		buf_pos += sprintf(buf_pos, "[%02d][%-24.24s] [%10ld.%06ld]",  i, _g_notify_info[i].name,
				pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec );
				
		if(pinfo->type == NF_NOTIFY_PARAM)
			buf_pos += sprintf(buf_pos, "t[d] [0x%08x][0x%08x][0x%08x][0x%08x]\n", 
					pinfo->d.params[0], pinfo->d.params[1], 
					pinfo->d.params[2], pinfo->d.params[3] );
		else if(pinfo->type == NF_NOTIFY_CHMAP)
			buf_pos += sprintf(buf_pos, "t[c] [%-16.16s]\n", pinfo->c.chmap);
		else
			buf_pos += sprintf(buf_pos, "t[p] [0x%p](%d)\n", pinfo->p.ptr, pinfo->p.len);
	}
	
	{	
		FILE *fp;				
		fp = fopen( "/tmp/webra-info/notify_dump.txt", "w") ;
		if(fp){				
			fwrite( buf, strlen(buf), 1, fp);
			fclose(fp);
		}
	}	
	return 0;
}
__commandlist(notify_dump_file,"notify_dump_file",notify_dump_file_help, notify_dump_file_help);

	
static char notify_fire_params_help[] = "notify_fire_params [name] [param1]..[param3] [repeat_cnt]";
static int notify_fire_params(int argc, char **argv)
{	
	gchar *property_name;
	guint params[4] = {0,};
	gint  repeat_cnt = 1, i;
		
	if(argc < 3){
		printf("%s\n",notify_fire_params_help);
		return -1;
	}
		
	property_name = argv[1];	
	params[0] = (guint)strtol(argv[2],NULL,0);
	
	if(argc > 3) params[1] = (guint)strtol(argv[3],NULL,0); 
	if(argc > 4) params[2] = (guint)strtol(argv[4],NULL,0); 
	if(argc > 5) params[3] = (guint)strtol(argv[5],NULL,0); 
	if(argc > 6) repeat_cnt = strtol(argv[6],NULL,0); 
	
	printf("%s property_name[%s] [0x%08x][0x%08x][0x%08x][0x%08x] [%d]\n", __FUNCTION__,
				property_name, params[0], params[1], params[2], params[3], repeat_cnt);
	
	for(i=0;i<repeat_cnt;i++)
		nf_notify_fire_params( property_name, params[0], params[1], params[2], params[3]);
				
	return 0;
}
__commandlist(notify_fire_params,"notify_fire_params",notify_fire_params_help, notify_fire_params_help);


static char notify_fire_chmap_help[] = "notify_fire_chmap [name] [chmap]";
static int notify_fire_chmap(int argc, char **argv)
{	
	gchar *property_name;
	gchar *chmap;	
	gint chmap_len = 0;
	
	if(argc < 3){
		printf("%s\n",notify_fire_chmap_help);
		return -1;
	}
			
	property_name = argv[1];	
	chmap = argv[2];
	chmap_len = (gint)strlen(chmap);

	printf("%s property_name[%s] [%16.16s](%d)\n", __FUNCTION__,
				property_name, chmap, chmap_len);

	nf_notify_fire_chmap( property_name, chmap);
				
	return 0;
}
__commandlist(notify_fire_chmap,"notify_fire_chmap",notify_fire_chmap_help, notify_fire_chmap_help);


static void
test_thread_func(void *param)
{
	char buff[1024];	
	g_message("%s test %x cnt[%x] START!!!", __FUNCTION__, buff, param);
		
	while(1) {
		g_message("%s test %x cnt[%x]", __FUNCTION__, buff, param);
		sleep(1);		
	}

	return;
}

static char notify_test_help[] = "notify_test [cnt]";
static int notify_test(int argc, char **argv)
{	
	static gint tid=0;
	int i, cnt;
	void *tmp;
	
	if(argc < 2){
		printf("%s\n",notify_test_help);
		return -1;
	}
		
	cnt = (guint)strtol(argv[1],NULL,0);			
	for(i=0; i<cnt; ++i)
	{
		++tid;
		tmp = g_thread_create( (GThreadFunc)test_thread_func, (void *)tid, FALSE, NULL);
		printf("%s g_thread_create ret[%x]\n", __FUNCTION__, tmp);
	}	
	
	return 0;
}
__commandlist(notify_test ,"notify_test",notify_test_help, notify_test_help);
	
#endif

/*

# 사용 예) 

1) 각 카메라의 motion 감지 를 알고 싶다. 

      #include "nf_notify.h" 
          
      NF_NOTIFY_INFO *pnotify;    
      pnotify = nf_notify_get( "motion" ); 
    
      if( !pnotify )   { 
         g_error("항목이 없습니다"); 
      }else{ 
         for(int i=0; i<16; i++) 
         { 
            if ( pnotify->d.params[0] & (1<<i) ) 
               print(" %d ch motion detection\n"); 
         }       
         nf_notify_free(pnotify); // 동적할당 받은 것이므로 해제가 필요 
      } 

2) analog 카메라의 record모드를 얻어 오고 싶다. 

      #include "nf_notify.h" 
          
      NF_NOTIFY_INFO *pnotify;    
      pnotify = nf_notify_get( "analog_rec" ); 
    
      if( !pnotify )   { 
         g_error("항목이 없습니다"); 
      }else{ 
         for(int i=0; i<16; i++) 
         {          
            print(" %d ch  rec_mode [%c]\n",  i, pnotify->c.chmap[i]); 
         }       
         nf_notify_free(pnotify); // 동적할당 받은 것이므로 해제가 필요 
      } 


3) network 트래픽 감지 

      #include "nf_notify.h" 
          
      NF_NOTIFY_INFO *pnotify;    
       
      while(1){       
            pnotify = nf_notify_get( "net_rxtx" );             
            if( !pnotify )   { 
               g_error("항목이 없습니다"); 
            }else{ 
                
               int rx = pnotify->d.params[0] ; 
               int tx = pnotify->d.params[1] ; 

               nf_notify_free(pnotify); // 동적할당 받은 것이므로 해제가 필요 
                
               printf("net rx[%d] tx[%d]\n", rx,tx); 
                
            }          
      }    
                
g_signal_connect (test, "notify::dummy", G_CALLBACK (dummy_notify), NULL); 
콜백 등록할 때는 function ptr과 void *를 등록한다.
	-> 그래야 콜백을 구분가능함.

gint dummy;

## 잊지말자 NULL~~;;
g_object_get (test, "dummy", &dummy, NULL);
g_object_set (test, "dummy", dummy + 1, NULL); 
      
*/

