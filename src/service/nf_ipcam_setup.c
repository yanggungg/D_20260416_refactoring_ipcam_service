/**
 * @file nf_ipcam_setup.c
 * @brief IP카메라 최초접속 루틴 및 설정 Api.
 * @author Jae-young Kim
 * @date 2011-01-12
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_IPCAM_SETUP_C__
#define __NF_IPCAM_SETUP_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <glib.h>
// #include <gst/gst.h>
// #include <nmf.h>
// #include <nmf_mrtp_pipe.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_api_openmode.h>

#include "nf_ipcam_driver_s1.h"
#include "nf_ipcam_driver_axis.h"

#include <nfdal.h>
#include "ivca_def.h"

#include "nf_onvif_analytics.h"

#define PRINT_CONNECTION_HTTP_API (0)

#define SCLE(a,min,max) (roundi(((double)(a)/100.0)*(double)((max)-(min)))+(min))
#define IPCAM_MANAGER_CAM_COUNT 8

typedef struct __JOB_DEFINITION_
{
	NF_IPCAM_SETUP_TYPE_E type;	/**< Job 정의 enum. */
	char* t_str;	/**< Job 설명. */
	int t_size;		/**< 관련 파라메터의 size_t. */
	int t_timeout;	/**< 각 job별 timeout. */
} NF_IPCAM_SETUP_T;

typedef struct __JOB_STATUS_
{
	NF_IPCAM_SETUP_STATUS_E type;
	char *t_str;
} NF_IPCAM_JOB_STATUS_T;

static NF_IPCAM_SETUP_T _jobs[NF_IPCAM_TYPE_MAX] = {
	{ NF_IPCAM_TYPE_INIT,            "INITIALIZE",          0    ,20 },
	{ NF_IPCAM_TYPE_CUSTOM0,         "CUSTOM FUNC0",        0    ,10 },
	{ NF_IPCAM_TYPE_CUSTOM1,         "CUSTOM FUNC1",        0    ,10 },
	{ NF_IPCAM_TYPE_CUSTOM2,         "CUSTOM FUNC2",        0    ,10 },
	{ NF_IPCAM_TYPE_CUSTOM3,         "CUSTOM FUNC3",        0    ,10 },
	{ NF_IPCAM_TYPE_REBOOT_SOFT,     "REBOOT",              0    ,60 },
	{ NF_IPCAM_TYPE_FACTORY_DEFAULT, "FACTORY DEFAULT",     0    ,60 },
	{ NF_IPCAM_TYPE_SET_VCODEC,      "SET VIDEO CODEC",     84   ,30 },
	{ NF_IPCAM_TYPE_SET_ACODEC,      "SET AUDIO CODEC",     84   ,10 },
	{ NF_IPCAM_TYPE_SET_IMAGE,       "SET IMAGE",           120  ,10 },
	{ NF_IPCAM_TYPE_SET_ALARM,       "SET ALARM",           84   ,20 },
	{ NF_IPCAM_TYPE_SET_MOTION,      "SET MOTION",          1568 ,40 },
	{ NF_IPCAM_TYPE_SET_PMASK,       "SET PRIVACY MASK",    208  ,10 },
	{ NF_IPCAM_TYPE_GET_LENSCAP,     "GET LENS CAPABILITY", 84   ,10 },
	{ NF_IPCAM_TYPE_SET_ORIGIN,      "SET CALIBRATION",     0    ,10 },
	{ NF_IPCAM_TYPE_SET_AF_MODE,     "SET AUTO FOCUS MODE", 4    ,10 },
	{ NF_IPCAM_TYPE_SET_AR_MODE,     "SET AUTO IRIS MODE",  4    ,10 },
	{ NF_IPCAM_TYPE_SET_PAN_TILT,    "SET PAN/TILT",        20   ,10 },
	{ NF_IPCAM_TYPE_SET_ZOOM,        "SET ZOOM",            20   ,10 },
	{ NF_IPCAM_TYPE_SET_FOCUS,       "SET FOCUS",           20   ,10 },
	{ NF_IPCAM_TYPE_SET_IRIS,        "SET IRIS",            4    ,10 },
	{ NF_IPCAM_TYPE_SET_ONESHOT,     "SET ONEPUSH FOCUS",   0    ,10 },
	{ NF_IPCAM_TYPE_GET_PAN,         "GET PAN",             4    ,10 },
	{ NF_IPCAM_TYPE_GET_TILT,        "GET TILT",            4    ,10 },
	{ NF_IPCAM_TYPE_GET_ZOOM,        "GET ZOOM",            4    ,10 },
	{ NF_IPCAM_TYPE_GET_FOCUS,       "GET FOCUS",           4    ,10 },
	{ NF_IPCAM_TYPE_GET_IRIS,        "GET IRIS",            4    ,10 },
	{ NF_IPCAM_TYPE_SET_STOP,        "SET STOP",            4    ,10 },
	{ NF_IPCAM_TYPE_PRESET_SET,      "SET PRESET",          4    ,10 },
	{ NF_IPCAM_TYPE_PRESET_CLEAR,    "CLEAR PRESET",        4    ,10 },
	{ NF_IPCAM_TYPE_PRESET_GO,       "GO PRESET",           4    ,10 },
	{ NF_IPCAM_TYPE_POLL_EVENT,      "POLL EVENT",          0    ,10 },
	{ NF_IPCAM_TYPE_SET_IMAGE_ONVIF, "SET IMAGE ONVIF",     132  ,10 },
	{ NF_IPCAM_TYPE_SET_EXP_ONVIF,   "SET EXP ONVIF",       132  ,10 },
	{ NF_IPCAM_TYPE_SET_FOCUS_ONVIF, "SET FOCUS ONVIF",     16   ,10 },
	{ NF_IPCAM_TYPE_SET_ENABLE_VA,   "SET ENABLE VA",       0    ,20 },
	{ NF_IPCAM_TYPE_SET_RESET_VA,    "SET RESET VA",        0    ,20 },
	{ NF_IPCAM_TYPE_SET_VA_CONFIG,   "SET VA CONFIG",       0    ,20 },
	{ NF_IPCAM_TYPE_SET_VA_OPTION,   "SET VA OPTION",       0    ,20 },
	{ NF_IPCAM_TYPE_SET_MIRROR,	     "SET MIRROR",          84   ,30 },
	{ NF_IPCAM_TYPE_SET_ADJUST_D2N,  "SET ADJUST_D2N", 	    120  ,10 },
	{ NF_IPCAM_TYPE_SET_ADJUST_N2D,  "SET ADJUST_N2D",      120  ,10 },
	{ NF_IPCAM_TYPE_SET_FOCUS_COMP,  "SET FOCUS_COMP",      28   ,10 },
	{ NF_IPCAM_TYPE_SET_DC_IRIS_CAL, "SET DC Iris CAL",     0    ,20 },
	{ NF_IPCAM_TYPE_SET_ZOOM_STOP,   "SET ZOOM STOP",       4    ,10 },
	{ NF_IPCAM_TYPE_SET_ROI,         "SET ROI",             224  ,40 },
	{ NF_IPCAM_TYPE_SET_SMART_MOTION,"SET SMART MOTION",    1568 ,40 },
	{ NF_IPCAM_TYPE_SET_CORRIDOR_MODE,"SET CORRIDOR MODE",	4    ,10 },
	{ NF_IPCAM_TYPE_SET_MOUNT,       "SET MOUNT",           4    ,10 },
	{ NF_IPCAM_TYPE_SET_DEWARP,      "SET DEWARP",          4    ,10 },
	{ NF_IPCAM_TYPE_GET_EPTZ_LAYOUT, "GET EPTZ LAYOUT",     9999 ,10 }, // 9999 is null value
};

static NF_IPCAM_JOB_STATUS_T _job_status[NF_IPCAM_STATUS_MAX] = {
	{ NF_IPCAM_STATUS_NORMAL,      "NORMAL"         },
	{ NF_IPCAM_STATUS_BEGIN,       "BEGIN"          },
	{ NF_IPCAM_STATUS_PENDING,     "PENDING"        },
	{ NF_IPCAM_STATUS_FAILED_REQ,  "REQUEST FAILED" },
	{ NF_IPCAM_STATUS_TIMEOUT,     "TIMEOUT"        },
	{ NF_IPCAM_STATUS_END_SUCCESS, "END SUCCESS"    }
};

/** @var _ipcam_setup_sending
 *  @brief 각 채널별, job 유형별 command 전송 여부.
 *  중복 전송을 막기 위해 사용함.
 */
static int _ipcam_setup_sending[AVAILABLE_MAX_CH][NF_IPCAM_TYPE_MAX];
/** @var _wait_cnt
 *  @brief 비디오 코덱 설정 등 response가 오래 걸리는 명령들에 대해
 *  개별적으로 timeout 추가하기 위해 사용.
 */
static int _wait_cnt[AVAILABLE_MAX_CH];

/** @var _init_devices_mask
 *  @brief Login을 마치고 카메라 초기설정이 필요한 카메라 bitmask.
 */
/** @var _discovered_devices_mask
 *  @brief 최초 검색 완료된 카메라 bitmask.
 */
static unsigned int _init_devices_mask = 0;
static unsigned int _discovered_devices_mask = 0;

//static int product_portnum = 0;
//static GAsyncQueue *queue = NULL;

/** @var callbacks
 *  @brief 카메라 설정시 callback함수 등록 및 관리 변수.
 */
static NFIPCamCallbacks callbacks[AVAILABLE_MAX_CH][NF_IPCAM_TYPE_MAX];

static pthread_t cam_setup_th;
static pthread_t cam_prepare_th;
static void nf_ipcam_setup_th_func(void *arg);
static void cam_setup_func(int start_ch, int end_ch);
static void nf_ipcam_prepare_th_func(void);
static void cam_prepare_func(void);
static void cam_setup_setcb(int, NF_IPCAM_SETUP_TYPE_E, NFIPCamSetupCallback, gpointer);

static void init_ipcam_manager(void);

static int modeldb_search(int port);
static int modeldb_search_onvif(int port);
static void sysdb_load(int port);
static void sysdb_load_onvif(int port);
static void sysdb_load_preset(int port);

static void cam_prepare(int port);
static void cam_prepare2(int port);
static void initial_setup_init(int port);
static void initial_setup_c0(int port);
static void initial_setup_c1(int port);
static void initial_setup_c2(int port);
static void initial_setup_c3(int port);
static void initial_setup_vcodec(int port);
static void initial_setup_va_option(int port);
static void initial_setup_va_config(int port);
static void initial_setup_va_enable(int port);
static void initial_setup_va_reset(int port);
static void initial_setup_acodec(int port);
static void initial_setup_image(int port);
static void initial_setup_privacy_mask(int port);
static void initial_setup_roi_area(int port);
static void initial_setup_motion_area(int port);
static void initial_setup_done(int port);
static void initial_setup_callback(NFIPCamProgress*, gpointer);

static void cam_prepare_onvif(int port);
static void onvif_initial_setup_c0(int port);
static void onvif_initial_setup_c1(int port);
static void onvif_initial_setup_c2(int port);
static void onvif_initial_setup_c3(int port);
static void onvif_initial_setup_vcodec(int port);
static void onvif_initial_setup_acodec(int port);
static void onvif_initial_setup_image(int port);
//static void onvif_initial_setup_exposure(int port);
static void onvif_initial_setup_motion_area(int port);
static void onvif_initial_setup_done(int port);
static void onvif_initial_setup_callback(NFIPCamProgress*, gpointer);

//static int get_pid_by_name(char*);

static void cam_setup_done(int ch, NF_IPCAM_SETUP_TYPE_E type);
static void cam_setup_failed_req(int ch, NF_IPCAM_SETUP_TYPE_E type);
static void call_setup_reserved(void* data, int channel, NF_IPCAM_SETUP_TYPE_E type);

extern gboolean nf_notify_fire_params(const gchar*, guint, guint, guint, guint);
extern int cam_set_focus_comp(focus_comp_info* info, int cam_id);

extern NF_IPCAM_FPS_E _get_max_fps_mask(int db_fps, unsigned int support);
static void _build_fps_table(int ch, int max_fps, int stream_no);
extern nf_onvif_set_relayout(void *data, int ch);
extern void calculate_day_night(void);
extern void calculate_dnn(void);

extern void _convert_pmask_area_by_corridor_view(int ch, NFIPCamPrivacyMask* pmask);
extern void _convert_roi_area_by_corridor_view(int ch, NFIPCamSetupROIArea* roi);

/**
 * @brief 전 채널의 콜백 및 연결 소켓을 초기화한다.
 * @see nf_ipcam_stop
 */
extern void cam_setup_restart(void)
{
	int i=0;
	int j=0;
	//IPCAM_DBG(MAJOR, "start\n");

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		for (j=NF_IPCAM_TYPE_INIT; j<NF_IPCAM_TYPE_MAX; j++)
		{
			int temp_sock = callbacks[i][j].sock;
			memset(&callbacks[i][j], 0x00, sizeof(NFIPCamCallbacks));
			if (temp_sock > 0)
			{
				close(temp_sock);
			}
			callbacks[i][j].sock = (-1);
		}
	}
	//IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 카메라 셋업 루틴을 init한다.
 * @param channel_num 모델의 최대 채널수.
 */
extern void ipcam_manager(int channel_num)
{
	//product_portnum = channel_num;
	init_ipcam_manager();
}

/**
 * @brief 카메라 discover메세지 수신 완료시 호출.
 * @param port 채널 번호.
 */
extern void discovered_device_handler_reg(int port)
{
	IPCAM_DBG(MAJOR, "discovered devices %08x(%08x)\n", (1<<port), _discovered_devices_mask);
	_discovered_devices_mask |= (1<<port);
	//IPCAM_DBG(MAJOR, "discovered devices registered(%08x)\n", _discovered_devices_mask);
}

/**
 * @brief 카메라 factory clear 시나리오 완료시 호출.
 * @param port 채널 번호.
 */
extern void device_initializer_reg(int port)
{
	IPCAM_DBG(MAJOR, "device initializer %08x(%08x)\n", (1<<port), _init_devices_mask);
	_init_devices_mask |= (1<<port);
	//IPCAM_DBG(MAJOR, "device initializer registered(%08x)\n", _init_devices_mask);
}

/**
 * @brief Discover 완료된 카메라 처리.
 * @param port 채널 번호.
 * @return 1 - 성공, 0 - 실패.
 *
 * Model db및 카메라의 모델 정보 비교, sysdb 불러오기 등을 수행한다.
 */
extern int discovered_device_handler(int port)
{
	int result;
	mtable *runtime = get_runtime();
	//queue = get_queue();

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_MODEL);
	result = modeldb_search(port);
	if (result == IPX_SEARCH_CONNECTION_FAILED)
	{
		IPCAM_DBG(WARN, "device couldn't be reached(CH(%d))\n", port);
		nf_eventlog_put_ipcam_msg("Connection Fail(ITX)", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_CONNECTION_FAIL, __LINE__, __FILE__);
		return (0);
	}
	if (result == IPX_SEARCH_LOGIN_FAIL)
	{
		IPCAM_DBG(WARN, "login failed(CH(%d))\n", port);
		nf_eventlog_put_ipcam_msg("Login Fail(ITX)", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
		return (0);
	}
	if (result == IPX_SEARCH_FOUND_NOT_SUPPORTED)
	{
		IPCAM_DBG(WARN, "modeldb has no matches(CH(%d))\n", port);
		nf_eventlog_put_ipcam_msg("Not Supported(ITX)", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_MODEL_UNSUPPORT, __LINE__, __FILE__);
		return (0);
	}

	// nvs video stream is started by nvs_video_handler() of nf_ipcam_discovery.c
	if (runtime[port].sys.type == SYSTEM_DEVICE_NVS) {
		nvs_close_stream(port);
		IPCAM_DBG(MINOR, "device type of CH(%d) is [NVS]\n", port);
		return (1);
	}

	sysdb_load(port);
	runtime[port].conn_type = 0;
	cam_prepare(port);

	return (1);
}

/**
 * @brief Nvs 채널 처리.
 * @param port 채널 번호.
 */
extern void nvs_device_handler(int port)
{
	sysdb_load(port);
	cam_prepare(port);
}

/**
 * @brief ONVIF discovery 완료된 카메라 처리.
 * @param port 채널 번호.
 * @return 1 - 성공, 0 - 실패.
 */
extern int onvif_device_handler(int port)
{
	int result;
	mtable *runtime = NULL;
	runtime = get_runtime();
	//queue = get_queue();

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_MODEL);
	result = modeldb_search_onvif(port);
	if (result == IPX_SEARCH_CONNECTION_FAILED)
	{
		IPCAM_DBG(WARN, "device couldn't be reached(CH(%d))\n", port);
		nf_eventlog_put_ipcam_msg("Connection Fail(ONVIF)", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_CONNECTION_FAIL, __LINE__, __FILE__);
		return (0);
	}
	if (result == IPX_SEARCH_LOGIN_FAIL)
	{
		IPCAM_DBG(WARN, "login failed(CH(%d))\n", port);
		nf_eventlog_put_ipcam_msg("Login Fail(ONVIF)", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
		return (0);
	}
	if (result == IPX_SEARCH_FOUND_NOT_SUPPORTED)
	{
		/* do nothing */
	}

	runtime[port].conn_type = 1;

	nf_pnd_queue_push(port, IPCAM_EVENT_ONVIF_PRIME_READY, __LINE__, __FILE__);
	return (1);
}

// using by _onvif_set_motion_info
struct _sensitivity 
{
	int min;
	int max;
};

struct cam_info 
{
	char name[64];
	int max_rect;
	struct _sensitivity sens;
};

// ONVIF Set Motion Info
// call by onvif_supported_device_handler
static int _onvif_set_motion_info(mtable *runtime, int ch, int type)
{

	int rtn, columns, rows, i;
	int device_count;
	int model_num = -1;
	int sense=0, sense_min=0, sense_max=0, max_ret=0;
	char key[64];
	motion_t motion_info;
	struct _AnalyticsModules analyticsModule;
	

	// Onvif Analytics Support Model DB
	struct cam_info onvif_analytics_device [] = {
		// NAME, 		MAX_RECT, 	SENSITIVITY.MIN, 	SENSITIVITY.MAX
		{"AMZ-2210",	1, 			{1,					5}},
	};

	IPCAM_DBG(MINOR,"[ONVIF] Set Moton Info Start\n");

	memset(key,              0x00, sizeof(key));
	memset(&motion_info,     0x00, sizeof(motion_info));
	memset(&analyticsModule, 0x00, sizeof(analyticsModule));

	// Currnet Device Count
	device_count = sizeof(onvif_analytics_device)/sizeof(struct cam_info);


	// Search Device Number from Device Name
	for(i = 0; i< device_count; i++)
	{
		if(strcmp(runtime[ch].sys.model, onvif_analytics_device[i].name)==0)
		{
			break;
		}
	}
	
	// Find Done
	if(i < device_count)
		model_num = i;
	else
		model_num = -1;		

	// Debug Print Device & Device DB Info
	{
		IPCAM_DBG(MINOR, "[ONVIF] Set Motion Model(%s) | Vendor(%s) | Device Array Size(%d) | Device Count(%d) | Model Num(%d)\n",
		runtime[ch].sys.model,
		runtime[ch].sys.vendor,
		sizeof(onvif_analytics_device),
		sizeof(struct cam_info),
		device_count,
		model_num);
	}

	if(type == OMT_REGION) {
		rtn = nf_onvif_va_set_analytics_modules_enable(ch);
		rtn = nf_onvif_va_get_analytics_modules(&analyticsModule, ch);
		if(rtn != IPCAM_SETUP_RTN_DONE)	{
			IPCAM_DBG(MINOR,"Onvif Va Get Analytics Not Suupport or Error = %d\n", rtn);
			goto exit_end;
		}


	} else if(type == OMT_CELL) {
		rtn = nf_onvif_va_get_analytics_modules(&analyticsModule, ch);
		if(rtn != IPCAM_SETUP_RTN_DONE)	{
			IPCAM_DBG(MINOR,"Onvif Va Get Analytics Not Suupport or Error = %d\n", rtn);
			goto exit_end;
		}

	} else if(type == OMT_MOTION) {
		strcpy(analyticsModule.Columns, "22");
		strcpy(analyticsModule.Rows, "15");
		sense_min 	= 0;
		sense_max 	= 100;
		sense		= 80;
		max_ret 	= 1;
		rtn = IPCAM_SETUP_RTN_DONE;
	}


	// if Support Onvif Analytics Module
	columns 	= atoi(analyticsModule.Columns);
	rows		= atoi(analyticsModule.Rows);

	IPCAM_DBG(MINOR,"columns=%d, rows=%d\n", columns, rows);

	// Sucesses Get column & rows
	if(columns != 0 && rows != 0) {

		// Get Sensitivity from sysdb
		if(get_dn_now(ch) == 1) {
			snprintf(key, 64, "alarm.motion.M%d.sense_d", ch);
		} else{
			snprintf(key, 64, "alarm.motion.M%d.sense_n", ch);
		}

		sense = nf_sysdb_get_uint(key);

		if(model_num != -1) {
			// old sensitivity value error
			if(sense < onvif_analytics_device[model_num].sens.min && sense > onvif_analytics_device[model_num].sens.max ) {	
				int value = onvif_analytics_device[model_num].sens.max - onvif_analytics_device[model_num].sens.min;
				value = value/2;
				sense = onvif_analytics_device[model_num].sens.min + value;
			} else {
				sense = 2;
			}

			sense_min = onvif_analytics_device[model_num].sens.min;
			sense_max = onvif_analytics_device[model_num].sens.max;
			max_ret = onvif_analytics_device[model_num].max_rect;
		} else {
			sense = 45;
			sense_min = 1;
			sense_max = 100;
			max_ret = 1;
		}

		// Set Motion Info
		motion_info.block_width = columns;
		motion_info.block_height= rows;
		motion_info.num_block	= columns * rows;
		motion_info.min_block 	= 1; 		// Event Send Minimum Block Number
		motion_info.max_rect 	= max_ret;  	// Max Drowing Retangle
		motion_info.sensitivity.min 	= sense_min;	// Sensitivity Range Mininum
		motion_info.sensitivity.max 	= sense_max;	// Sensitivity Range Maximum
		motion_info.sensitivity.value	= sense;	// Sensitivity Set Value
		motion_info.method = MAM_RECTANGLE;


		if(motion_info.method != MAM_NONE)
		{
			if(type == OMT_REGION)
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = &nf_onvif_va_set_motion;
			else if(type == OMT_CELL)	
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = &nf_onvif_va_set_motion_cells;
			else if(type == OMT_MOTION)
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = &nf_onvif_va_set_motion_window;
			else
				goto exit_end;

			runtime[ch].func |= NF_IPCAM_FUNC_MOTION;
		}

		memcpy(&runtime[ch].motion, &motion_info, sizeof(motion_t));
	}
exit_end:
	IPCAM_DBG(MINOR,"[ONVIF] Set Moton Info End\n");
	return rtn;

}

// ONVIF Relay Output Support Set
static int _onvif_set_relayout(mtable *runtime, int ch)
{

	IPCAM_DBG(MINOR,"[ONVIF] %s\n",__func__);
	runtime[ch].funcs[NF_IPCAM_TYPE_SET_ALARM] = &nf_onvif_set_relayout;
	runtime[ch].func |= NF_IPCAM_FUNC_ALARM_OUT | NF_IPCAM_FUNC_ALARM_IN ;
	nf_ipcam_set_relay(ch, 0, NULL, NULL, NULL);

}

/**
 * @brief ONVIF 카메라의 초기화 처리.
 * @param ch 채널 번호.
 * @return 1 - 성공, 0 - 실패.
 *
 * 각 option및 현재 값들을 조회하고 sysdb와 비교 및 동기화 처리를 수행한다.
 * S1 이기종 프로토콜도 1회씩 조회해본다.
 */
extern int onvif_supported_device_handler(int ch)
{
	int rtn = 0;
	mtable *runtime = get_runtime();
	//queue = get_queue();

	onvif_service_t service;
	memset(&service , 0x00, sizeof(service));
	nf_onvif_get_capabilities(ch, &service);

	if(strcmp(runtime[ch].sys.vendor, "AXIS") == 0)
	{
		rtn = nf_axis_get_image(ch);
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			rtn = nf_onvif_get_image_t(ch);
			rtn = nf_onvif_get_image_t_value(ch);
		}
	}
	else
	{
	    rtn = nf_onvif_get_image_t(ch);
		rtn = nf_onvif_get_image_t_value(ch);
	}
	
	if(nf_ipcam_is_vendor_orion() || nf_ipcam_is_vendor_g4s())
	{
		if(strcasecmp(runtime[ch].sys.vendor, "HDPRO") == 0)
		{
			nf_hdpro_enable_mirror(ch);
			nf_hdpro_enable_onepush(ch);
		}
	}

#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
	if((strcmp(runtime[ch].sys.vendor, "HD-IP") == 0) ||
	   (strcmp(runtime[ch].sys.vendor, "IPCamera") == 0))
	{
		// Sunell Fisheye Exception
		if((strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/13") == 0) ||
		   (strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/23") == 0) || 
		   (strcmp(runtime[ch].sys.stdver, "CBP360-IP") == 0))
		{
			NF_IPCAM_MOUNT_TYPES_E mount;
			NF_IPCAM_DEWARP_MODES_E dewarp;

			runtime[ch].cam_type = NF_IPCAM_CAM_TYPE_FISHEYE;
			runtime[ch].fisheye.fisheye_supported = 0;
			runtime[ch].fisheye.fisheye_supported = NF_IPCAM_FISHEYE_MOUNT  |
													NF_IPCAM_FISHEYE_DEWARP |
													NF_IPCAM_FISHEYE_EPTZ;
			nf_sunell_enable_mount(ch);
			nf_sunell_get_mount(ch, &mount);
			runtime[ch].fisheye.mount.value = mount;

			nf_sunell_enable_dewarp(ch);
			nf_sunell_get_dewarp(ch, &dewarp);
			runtime[ch].fisheye.dewarp.value = dewarp;

			nf_sunell_enable_ePTZ(ch);
		}
	}
#endif

	/*if(rtn != 0)
	{
		nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
		return (0);
	}*/

	/*if(rtn != 0)
	{
		nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
		return (0);
	}*/

#if 0
	/* S1 integration : for techwin, idis, huviron cam */
	//if (nf_ipcam_is_vendor_s1() || nf_sysman_get_fwver_vendor() == 83)
	{
		motion_t motion_info;
		rtn = s1_get_event_cap(&motion_info, ch);
		if(rtn == IPCAM_SETUP_RTN_DONE)
		{
			//IPCAM_DBG(MINOR, "s1 motion : method(%d) area(%d) width(%d) height(%d) sensmin(%d) sensmax(%d)\n",
			//		motion_info.method, motion_info.max_rect, motion_info.block_width, motion_info.block_height,
			//		motion_info.sensitivity.min, motion_info.sensitivity.max
			//		);

			mtable* runtime = get_runtime();
			memcpy(&runtime[ch].motion, &motion_info, sizeof(motion_t));
			if(motion_info.method != MAM_NONE)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = &s1_set_motion_area;
				runtime[ch].func |= NF_IPCAM_FUNC_MOTION;
			}

			int cap;
			rtn = s1_get_mirror_cap(&cap, ch);
			//IPCAM_DBG(MINOR, "s1 mirror : support(%08x)\n", cap);
			if(rtn == IPCAM_SETUP_RTN_DONE && cap != 0)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MIRROR] = &s1_set_mirror_val;	//FIXME separate vcodec & mirror
				runtime[ch].video.mirror.support = cap;
				runtime[ch].video.supported |= VIDEO_SETUP_MIRROR;
				runtime[ch].video.onthefly |= VIDEO_SETUP_MIRROR;
				runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_ROTATION;
			}

			rtn = s1_get_onepush_cap(&cap, ch);
			//IPCAM_DBG(MINOR, "s1 onepush : support(%08x)\n", cap);
			if(rtn == IPCAM_SETUP_RTN_DONE && cap != 0)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = &s1_set_onepush;
				runtime[ch].image.supported |= NF_IPCAM_IMAGE_ONEPUSH;
				runtime[ch].image.onthefly |= NF_IPCAM_IMAGE_ONEPUSH;
				runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;
				runtime[ch].ptz.supported |= PTZ_SETUP_ONEPUSH;
			}

			//rtn = nf_onvif_create_event(ch);
		}
	}
#endif

	// Check Support Onvif Service
	if(runtime[ch].onvif.vac_type != 0)
		_onvif_set_motion_info(runtime, ch, runtime[ch].onvif.vac_type);


	// Realy Output Set
	if(service.device.IO.supported == 1 && 0 < service.device.IO.RelayOutputs)
	{
		IPCAM_DBG(MINOR,"[ONVIF] | ONVIF Device IO Supported, supported(%d), RelayOutputs(%d), InputConnectors(%d)\n",
				service.device.IO.supported, service.device.IO.RelayOutputs, service.device.IO.InputConnectors);
		_onvif_set_relayout(runtime, ch);
	}
	else
	{
		IPCAM_DBG(MINOR,"[ONVIF] | ONVIF Device IO Not Supported, supported(%d), RelayOutputs(%d), InputConnectors(%d)\n",
				service.device.IO.supported, service.device.IO.RelayOutputs, service.device.IO.InputConnectors);
	}

	// compare sysdb with image_t_onvif, synchronize
	sysdb_load_onvif(ch);

	cam_prepare_onvif(ch);

	return (1);
}

/**
 * @brief ITX카메라 초기 접속 루틴을 실행한다.
 * @param port
 */
extern void device_initializer(int port)
{
	cam_prepare2(port);
}

/**
 * @brief 해당 채널의 setup response의 수신 timeout을 증가시킨다.
 * @param port 채널 번호.
 * @param msec 증가시키고자 하는 timeout 시간.
 */
extern void nf_ipcam_waiting_settime(int port, int msec)
{
	int add_cnt;

	IPCAM_DBG(MAJOR, "port(%d) msec(%d)\n", port, msec);
	add_cnt = (msec/60);
	if (msec%60)
	{
		add_cnt++;
	}

	IPCAM_DBG(MAJOR, "count_now(%d) add(%d)\n", _wait_cnt[port], add_cnt);
	_wait_cnt[port] += add_cnt;
}

/**
 * @brief 01 문자열로 되어 있는 모션 설정을 좌표로 변환한다.
 * @param[in] width 모션 영역 Width.
 * @param[in] height 모션 영역 Height.
 * @param[in] area 모션 영역 설정 문자열.
 * @param[out] lt 왼쪽 위 좌표.
 * @param[out] rb 오른쪽 아래 좌표.
 */
extern void _get_rect_points
( int width, int height, char* area, NF_IPCAM_POINT *lt, NF_IPCAM_POINT *rb)
{
	int i,j;
	int left = (-1);
	int right = (-1);
	int top = (-1);
	int bottom = (-1);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (*(area+(i*width)+j) == '1')
			{
				if (left < 0) { left = j; }
				else if (left > j) { left = j; }

				if (right < 0) { right = j; }
				else if (right < j) { right = j; }

				if (top < 0) { top = i; }
				else if (top > i) { top = i; }

				if (bottom < 0) { bottom = i; }
				else if (bottom < i) { bottom = i; }
			}
		}
	}

	g_return_if_fail(left>=0);
	g_return_if_fail(right>=0);
	g_return_if_fail(top>=0);
	g_return_if_fail(bottom>=0);

	lt->x = left;
	lt->y = top;
	rb->x = right;
	rb->y = bottom;

	//IPCAM_DBG(MINOR, "left(%d) right(%d) top(%d) bottom(%d)\n", left, right, top, bottom);
}

/**
 * @brief 해당 job의 설명을 조회한다.
 * @param type Job 유형 enum.
 * @return Job 설명.
 */
extern char* ipcam_get_type_str(int type)
{
	return (_jobs[type].t_str);
}

/**
 * @brief 카메라에 각종 command를 전송하며 callback 관리등을 실시한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 * @param cb_fxn 설정 완료시 호출할 콜백 함수.
 * @param user_data 콜백 함수에 같이 넘길 user data.
 * @param info 설정과 관련된 정보가 위치한 포인터. Job 유형에 따라 다르다.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_PENDING - 기존 job 진행중이라 next Queue에 들어감, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int nf_ipcam_setup_request(int ch, NF_IPCAM_SETUP_TYPE_E type, NFIPCamSetupCallback cb_fxn, gpointer user_data, void* info)
{
	int rtn = 0;
	mtable *runtime = NULL;

	int (*func0)(int);
	int (*func4)(int, int);
	int (*func28)(focus_comp_info*, int);
	int (*func44)(cam_info*, int);
	int (*func116)(image_info*, int);
	int (*func132)(image_info_onvif*, int);
	int (*func208)(NFIPCamPrivacyMask*, int);
	int (*func224)(NFIPCamSetupROIArea*, int);
	int (*func1568)(NFIPCamSetupMotionArea*, int);
	int (*func3600)(ivca_rule_t*, int);
	int (*funcPTZ)(int,  ptz_info_onvif *);

	IPCAM_DBG(MAJOR, "%s CH(%d)\n", _jobs[type].t_str, ch);

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

#if 0
	if (type == NF_IPCAM_TYPE_SET_FOCUS)
	{
		if (callbacks[ch][NF_IPCAM_TYPE_SET_ZOOM].picked)
		{
			printf("[%s] WARN: focus control prevented while ZOOM\n", __FUNCTION__);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if (callbacks[ch][NF_IPCAM_TYPE_SET_ONESHOT].picked)
		{
			printf("[%s] WARN: focus control prevented while ONEPUSH\n", __FUNCTION__);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if (callbacks[ch][NF_IPCAM_TYPE_SET_ORIGIN].picked)
		{
			printf("[%s] WARN: focus control prevented while ORIGIN\n", __FUNCTION__);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
#endif

#if 0
	if (type == NF_IPCAM_TYPE_SET_PAN_TILT &&
				(runtime[ch].ptz.supported & (PTZ_SETUP_PAN|PTZ_SETUP_TILT)))
	{
		runtime[ch].ptz.moving |= (PTZ_SETUP_PAN|PTZ_SETUP_TILT);
	}
	else if (type == NF_IPCAM_TYPE_SET_ZOOM &&
				(runtime[ch].ptz.supported & PTZ_SETUP_ZOOM))
	{
		runtime[ch].ptz.moving |= PTZ_SETUP_ZOOM;
	}
	else if (type == NF_IPCAM_TYPE_SET_FOCUS &&
				(runtime[ch].ptz.supported & PTZ_SETUP_FOCUS))
	{
		runtime[ch].ptz.moving |= PTZ_SETUP_FOCUS;
	}
	else if (type == NF_IPCAM_TYPE_SET_IRIS &&
				(runtime[ch].ptz.supported & PTZ_SETUP_IRIS))
	{
		runtime[ch].ptz.moving |= PTZ_SETUP_IRIS;
	}
#else
	if (type == NF_IPCAM_TYPE_SET_PAN_TILT)
	{
		if ((_ipcam_setup_sending[ch][type]==1) || (callbacks[ch][type].picked==1))
		{
			IPCAM_DBG(WARN, "PAN/TILT CH(%d) new job prevented(m %d s %d p %d)\n",
					ch,
					runtime[ch].ptz.moving,
					_ipcam_setup_sending[ch][type],
					callbacks[ch][type].picked
					);
			return IPCAM_SETUP_RTN_DONE;
		}
		IPCAM_DBG(MINOR, "%s PAN/TILT\n", _jobs[type].t_str);
		runtime[ch].ptz.moving |= (PTZ_SETUP_PAN|PTZ_SETUP_TILT);
		if(nf_ipcam_is_onvif_support(ch) == 1)
		{	// onvif�� pt, z ���о���
			runtime[ch].ptz.moving |= PTZ_SETUP_ZOOM;
		}
	}
	else if (type == NF_IPCAM_TYPE_SET_ZOOM)
	{
		if ((_ipcam_setup_sending[ch][type]==1) || (callbacks[ch][type].picked==1))
		{
			IPCAM_DBG(WARN, "ZOOM CH(%d) new job prevented(m %d s %d p %d)\n",
					ch,
					runtime[ch].ptz.moving,
					_ipcam_setup_sending[ch][type],
					callbacks[ch][type].picked
					);
			return IPCAM_SETUP_RTN_DONE;
		}
		IPCAM_DBG(MINOR, "%s ZOOM\n", _jobs[type].t_str);
		runtime[ch].ptz.moving |= PTZ_SETUP_ZOOM;
	}
	else if (type == NF_IPCAM_TYPE_SET_FOCUS || type == NF_IPCAM_TYPE_SET_FOCUS_ONVIF)
	{
		if ((_ipcam_setup_sending[ch][type]==1) || (callbacks[ch][type].picked==1))
		{
			IPCAM_DBG(WARN, "FOCUS CH(%d) new job prevented(m %d s %d p %d)\n",
					ch,
					runtime[ch].ptz.moving,
					_ipcam_setup_sending[ch][type],
					callbacks[ch][type].picked
					);
			return IPCAM_SETUP_RTN_DONE;
		}
		IPCAM_DBG(MINOR, "%s FOCUS\n", _jobs[type].t_str);

		if(type == NF_IPCAM_TYPE_SET_FOCUS_ONVIF)
		{
			focus_move_onvif *tmp = (focus_move_onvif *)info;
			if(tmp->mode == NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
			{
				runtime[ch].ptz.moving |= PTZ_SETUP_FOCUS;
			}
		}
		else
		{
			runtime[ch].ptz.moving |= PTZ_SETUP_FOCUS;
		}
	}
	else if (type == NF_IPCAM_TYPE_SET_IRIS)
	{
		if ((_ipcam_setup_sending[ch][type]==1) || (callbacks[ch][type].picked==1))
		{
			IPCAM_DBG(WARN, "IRIS CH(%d) new job prevented(m %d s %d p %d)\n",
					ch,
					runtime[ch].ptz.moving,
					_ipcam_setup_sending[ch][type],
					callbacks[ch][type].picked
					);
			return IPCAM_SETUP_RTN_DONE;
		}
		IPCAM_DBG(MINOR, "%s IRIS\n", _jobs[type].t_str);
		runtime[ch].ptz.moving |= PTZ_SETUP_IRIS;
	}
#endif
	//else if (type == NF_IPCAM_TYPE_SET_STOP && nf_ipcam_is_onvif_support(ch) != 1)
	else if (type == NF_IPCAM_TYPE_SET_STOP && nf_ipcam_is_onvif_support(ch) != 1)
	{
		if (_ipcam_setup_sending[ch][type]==1)
		{
			IPCAM_DBG(WARN, "STOP CH(%d) new job prevented(s %d)\n",
					ch, _ipcam_setup_sending[ch][type]);
			return IPCAM_SETUP_RTN_DONE;
		}

		if(runtime[ch].sys.use_ssl)
		{
			while(1)
			{
				pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[type]);
				int state = runtime[ch].sys.ssl_state[type];
				pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[type]);

				if (state != IPCAM_SSL_WAITING)
					break;

				usleep(500 * 1000);
			}
		}

		IPCAM_DBG(MINOR, "%s STOP\n", _jobs[type].t_str);
		if (runtime[ch].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT))
		{
			IPCAM_DBG(MINOR, "%s STOP PAN/TILT\n", _jobs[type].t_str);
			if (runtime[ch].funcs[type] != NULL)
			{
				func4 = runtime[ch].funcs[type];
				rtn = func4( PTZ_SETUP_PAN, ch);
			}
			else
			{
				nf_ipcam_setup_waiting(ch, type, (-1));
				rtn = IPCAM_SETUP_RTN_DONE;
			}

			//runtime[ch].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT);
		}
		if (runtime[ch].ptz.moving & PTZ_SETUP_ZOOM)
		{
			IPCAM_DBG(MINOR, "%s STOP ZOOM\n", _jobs[type].t_str);
			if (runtime[ch].funcs[type] != NULL)
			{
				func4 = runtime[ch].funcs[type];
				rtn = func4( PTZ_SETUP_ZOOM, ch);
			}
			else
			{
				nf_ipcam_setup_waiting(ch, type, (-1));
				rtn = IPCAM_SETUP_RTN_DONE;
			}
			//runtime[ch].ptz.moving &= ~PTZ_SETUP_ZOOM;
		}
		if (runtime[ch].ptz.moving & PTZ_SETUP_FOCUS)
		{
			IPCAM_DBG(MINOR, "%s STOP FOCUS\n", _jobs[type].t_str);
			if (runtime[ch].funcs[type] != NULL)
			{
				func4 = runtime[ch].funcs[type];
				rtn = func4( PTZ_SETUP_FOCUS, ch);
			}
			else
			{
				nf_ipcam_setup_waiting(ch, type, (-1));
				rtn = IPCAM_SETUP_RTN_DONE;
			}
			//runtime[ch].ptz.moving &= ~PTZ_SETUP_FOCUS;
		}
		if (runtime[ch].ptz.moving & PTZ_SETUP_IRIS)
		{
			IPCAM_DBG(MINOR, "%s STOP IRIS\n", _jobs[type].t_str);
			if (runtime[ch].funcs[type] != NULL)
			{
				func4 = runtime[ch].funcs[type];
				rtn = func4( PTZ_SETUP_IRIS, ch);
			}
			else
			{
				nf_ipcam_setup_waiting(ch, type, (-1));
				rtn = IPCAM_SETUP_RTN_DONE;
			}
			runtime[ch].ptz.moving &= ~PTZ_SETUP_IRIS;
		}
		return IPCAM_SETUP_RTN_DONE;
	}
	else if (type == NF_IPCAM_TYPE_SET_ZOOM_STOP && nf_ipcam_is_onvif_support(ch) != 1)
	{
		if (_ipcam_setup_sending[ch][type]==1)
		{
			IPCAM_DBG(WARN, "STOP CH(%d) new job prevented(s %d)\n",
					ch, _ipcam_setup_sending[ch][type]);
			return IPCAM_SETUP_RTN_DONE;
		}

		if(runtime[ch].sys.use_ssl)
		{
			while(1)
			{
				pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[type]);
				int state = runtime[ch].sys.ssl_state[type];
				pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[type]);

				if (state != IPCAM_SSL_WAITING)
					break;

				usleep(500 * 1000);
			}
		}

		IPCAM_DBG(MINOR, "%s STOP\n", _jobs[type].t_str);
		if (runtime[ch].ptz.moving & PTZ_SETUP_ZOOM)
		{
			IPCAM_DBG(MINOR, "%s STOP ZOOM\n", _jobs[type].t_str);
			if (runtime[ch].funcs[NF_IPCAM_TYPE_SET_STOP] != NULL)
			{
				func4 = runtime[ch].funcs[NF_IPCAM_TYPE_SET_STOP];
				rtn = func4( PTZ_SETUP_ZOOM, ch);
			}
			else
			{
				nf_ipcam_setup_waiting(ch, type, (-1));
				rtn = IPCAM_SETUP_RTN_DONE;
			}
			//runtime[ch].ptz.moving &= ~PTZ_SETUP_ZOOM;
		}
		return IPCAM_SETUP_RTN_DONE;
	}
	else
	{
		cam_setup_setcb(ch, type, cb_fxn, user_data);
		if ((callbacks[ch][type].picked == 1) || (_ipcam_setup_sending[ch][type] == 1))
		{
			IPCAM_DBG(WARN, "Command doing(%s), job enqueued\n", _jobs[type].t_str);
			callbacks[ch][type].reserved = 1;
			// FIXME. Callback function overwriting
			//callbacks[ch][type].data_next = &info;
			memcpy((void*)&callbacks[ch][type].data_next, info, _jobs[type].t_size);

			return IPCAM_SETUP_RTN_PENDING;
		}
	}

	if(nf_ipcam_is_onvif_support(ch) == 1 &&
	  (type == NF_IPCAM_TYPE_SET_IMAGE_ONVIF
	|| type == NF_IPCAM_TYPE_SET_EXP_ONVIF
	|| type == NF_IPCAM_TYPE_SET_VCODEC
	|| type == NF_IPCAM_TYPE_SET_PAN_TILT
	|| type == NF_IPCAM_TYPE_SET_STOP
	|| type == NF_IPCAM_TYPE_SET_ORIGIN
	|| type == NF_IPCAM_TYPE_PRESET_SET
	|| type == NF_IPCAM_TYPE_PRESET_GO
	|| type == NF_IPCAM_TYPE_PRESET_CLEAR
	|| type == NF_IPCAM_TYPE_SET_FOCUS_ONVIF
	|| type == NF_IPCAM_TYPE_SET_ZOOM_STOP
	|| type == NF_IPCAM_TYPE_SET_MOUNT
	|| type == NF_IPCAM_TYPE_SET_DEWARP)
	  )
	{
		switch(type)
		{
			case NF_IPCAM_TYPE_SET_IMAGE_ONVIF:
				{
					if (runtime[ch].funcs[type] != NULL)
					{
						func132= runtime[ch].funcs[type];
						rtn = func132((image_info_onvif*)info, ch);
					}
					else
					{
				rtn = nf_onvif_set_image(ch, *((image_info_onvif *)info));
					}
				}
				break;

			case NF_IPCAM_TYPE_SET_EXP_ONVIF:
				rtn = nf_onvif_set_image(ch, *((image_info_onvif *)info));
				break;

			case NF_IPCAM_TYPE_SET_VCODEC:
				if (strcmp(runtime[ch].sys.vendor, "H264") == 0)
				{
					rtn = xiongmai_stream_set(ch,info);
					break;
				}
				rtn = nf_onvif_set_stream(ch, (cam_info*)info);
				break;

			case NF_IPCAM_TYPE_SET_PAN_TILT:
				{
					if(runtime[ch].funcs[type] != NULL)
					{
						funcPTZ = runtime[ch].funcs[type];
						rtn = funcPTZ(ch, (ptz_info_onvif *)info);
					}
					else
					{
                        rtn = nf_onvif_ptz_move(ch, *((ptz_info_onvif *)info));
					}
				}
				break;

			case NF_IPCAM_TYPE_SET_STOP:
				// FIXME cheating
#if 0
				if (strcmp(runtime[ch].sys.vendor, "Grundig") == 0)
				{
					if(runtime[ch].ptz.moving & PTZ_SETUP_FOCUS)
					{
						rtn = grundig_ipptz_set_ptz_stop(PTZ_SETUP_FOCUS, ch);
						runtime[ch].ptz.moving &= ~PTZ_SETUP_FOCUS;
					}
					if(runtime[ch].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT ) )
					{
						rtn = grundig_ipptz_set_ptz_stop(PTZ_SETUP_PAN, ch);
						runtime[ch].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT);
					}
					/*if(runtime[ch].ptz.moving & PTZ_SETUP_ZOOM)
					{
						rtn = grundig_ipptz_set_ptz_stop(PTZ_SETUP_ZOOM, ch);
						runtime[ch].ptz.moving &= ~PTZ_SETUP_ZOOM;
					}*/
				}
				//else
#endif

				{
					if(runtime[ch].ptz.moving & PTZ_SETUP_FOCUS)
					{
						rtn = nf_onvif_focus_stop(ch);
						runtime[ch].ptz.moving &= ~PTZ_SETUP_FOCUS;
					}
					if(runtime[ch].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT | PTZ_SETUP_ZOOM) )
					{
						if(runtime[ch].funcs[type] != NULL)
						{
							funcPTZ = runtime[ch].funcs[type];
							rtn = funcPTZ(ch, (ptz_info_onvif *)info);
						}
						else
						{
                            rtn = nf_onvif_ptz_stop(ch);
						}
						runtime[ch].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT | PTZ_SETUP_ZOOM);
					}
				}

				return IPCAM_SETUP_RTN_DONE;

			case NF_IPCAM_TYPE_SET_ZOOM_STOP:
				if(runtime[ch].ptz.moving & PTZ_SETUP_ZOOM)
				{
					rtn = nf_onvif_ptz_stop(ch);
					runtime[ch].ptz.moving &= ~(PTZ_SETUP_ZOOM);
				}
				return IPCAM_SETUP_RTN_DONE;


			case NF_IPCAM_TYPE_SET_ORIGIN:
				rtn = nf_onvif_ptz_goto_home(ch);
				break;

			case NF_IPCAM_TYPE_PRESET_SET:
				rtn = nf_onvif_ptz_set_preset(ch, *((int*) info));
				break;

			case NF_IPCAM_TYPE_PRESET_GO:
				rtn = nf_onvif_ptz_goto_preset(ch, *((int*) info));
				break;

			case NF_IPCAM_TYPE_PRESET_CLEAR:
				rtn = nf_onvif_ptz_remove_preset(ch, *((int*) info));
				break;

			case NF_IPCAM_TYPE_SET_FOCUS_ONVIF:
				rtn = nf_onvif_focus_move(ch, *((focus_move_onvif *)info));
				break;
			case NF_IPCAM_TYPE_SET_MIRROR:
                {
                    if (runtime[ch].funcs[type] != NULL)
                    {
                        func44= runtime[ch].funcs[type];
                        rtn = func44((cam_info*)info, ch);
                    }
                }
                break;

            case NF_IPCAM_TYPE_SET_MOUNT:
                {
                    if(runtime[ch].funcs[type] != NULL)
                    {
                        func4 = runtime[ch].funcs[type];
                        rtn = func4(ch, *((NF_IPCAM_MOUNT_TYPES_E *)info));
                    }
                }
                break;

            case NF_IPCAM_TYPE_SET_DEWARP:
                {
                    if(runtime[ch].funcs[type] != NULL)
                    {
                        func4 = runtime[ch].funcs[type];
                        rtn = func4(ch, *((NF_IPCAM_DEWARP_MODES_E *)info));
                    }
                }
                break;

            default:
                break;

		}
		nf_ipcam_setup_waiting(ch, type, (-1));
	}
	else
	{
		switch (_jobs[type].t_size)
		{
			case 0:
				if (runtime[ch].funcs[type] != NULL)
				{
					func0 = runtime[ch].funcs[type];
					rtn = func0(ch);
				}
				else
				{
					if ((type == NF_IPCAM_TYPE_FACTORY_DEFAULT) && (runtime[ch].state & MGMT_STATE_READY))
					{
						extern int _cam_factory_default(int);
						runtime[ch].sys.http_port = runtime[ch].admin_http;
						func0 = &_cam_factory_default;
						rtn = func0(ch);
						nf_ipcam_setup_waiting(ch, type, (-1));
					}
					else
					{
						nf_ipcam_setup_waiting(ch, type, (-1));
						rtn = IPCAM_SETUP_RTN_DONE;
					}
				}
				break;
			case 4:
				if (runtime[ch].funcs[type] != NULL)
				{
					func4 = runtime[ch].funcs[type];
					rtn = func4( *((int*) info), ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 20:
				if(runtime[ch].funcs[type] != NULL)
				{
					int (*func20)(ptz_info*, int);
					func20 = runtime[ch].funcs[type];
					rtn = func20((ptz_info*)info, ch);
				}
				break;
			case 28:
				if (runtime[ch].funcs[type] != NULL)
				{
					func28 = runtime[ch].funcs[type];
					rtn = func28((focus_comp_info*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 84:
				if (runtime[ch].funcs[type] != NULL)
				{
					func44 = runtime[ch].funcs[type];
					rtn = func44((cam_info*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 120:
				if (runtime[ch].funcs[type] != NULL)
				{
					func116= runtime[ch].funcs[type];
					rtn = func116((image_info*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 208:
				if (runtime[ch].funcs[type] != NULL)
				{
					func208 = runtime[ch].funcs[type];
					rtn = func208((NFIPCamPrivacyMask*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 224:
				if (runtime[ch].funcs[type] != NULL)
				{
					func224 = runtime[ch].funcs[type];
					rtn = func224((NFIPCamSetupROIArea*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 1568:
				if (runtime[ch].funcs[type] != NULL)
				{
					func1568 = runtime[ch].funcs[type];
					rtn = func1568((NFIPCamSetupMotionArea*)info, ch);
					if(rtn == 0) rtn = IPCAM_SETUP_RTN_DONE;
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 24*1024:
				if (runtime[ch].funcs[type] != NULL)
				{
					func3600 = runtime[ch].funcs[type];
					rtn = func3600((ivca_rule_t*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			default:
				g_warning("[%s] job requested with wrong param(%d)\n", __FUNCTION__, _jobs[type].t_size);
				rtn = IPCAM_SETUP_RTN_FAILED;
				break;
		}
	}

	switch (rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			cam_setup_failed_req(ch, type);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			break;
		default:
			g_warning("[%s] Return code wrong(%d)\n", __FUNCTION__, rtn);
			break;
	}

	return rtn;
}

/**
 * @brief 해당 채널의 ip주소 문자열을 조회한다.
 * @param[in] id 채널 번호.
 * @param[out] dst_buf 대상 문자열.
 */
extern void nf_ipcam_get_ipstr(int id, char* dst_buf)
{
	unsigned int h;
	mtable* runtime = NULL;

	runtime = get_runtime();
	while (runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}

	h = runtime[id].sys.ipaddr;
	sprintf(dst_buf, "%d.%d.%d.%d", (h&0xff), ((h&0xff00)>>8), ((h&0xff0000)>>16), ((h&0xff000000)>>24));
}

/**
 * @brief 해당 채널의 user ID를 조회한다.
 * @param[in] id 채널 번호.
 * @param[out] dst_buf 대상 문자열.
 */
extern void nf_ipcam_get_username(int id, char* dst_buf)
{
	mtable* runtime = NULL;

	runtime = get_runtime();
	while(runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}
	snprintf(dst_buf, 64, "%s", runtime[id].username);
}

/**
 * @brief 해당 채널의 user password를 조회한다.
 * @param[in] id 채널 번호.
 * @param[out] dst_buf 대상 문자열.
 */
extern void nf_ipcam_get_password(int id, char* dst_buf)
{
	mtable* runtime = NULL;

	runtime = get_runtime();
	while(runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}
	snprintf(dst_buf, 64, "%s", runtime[id].password);
}

/**
 * @brief 해당 채널의 http port번호를 조회한다. direct configure ui 표시목적.
 * @param ch 채널 번호.
 * @returns 대상 http port 번호.
 */
extern unsigned short nf_ipcam_get_dconf_port(int ch)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	// Xiongmai cam exception
	if(strcmp(runtime[ch].sys.vendor, "H264") == 0)
	{
		return (80);
	}

	return (runtime[ch].sys.http_port);
}

/**
 * @brief 해당 채널의 http port번호를 조회한다.
 * @param ch 채널 번호.
 * @returns 대상 http port 번호.
 */
extern unsigned short nf_ipcam_get_http_port(int ch)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	return (runtime[ch].sys.http_port);
}

/**
 * @brief 해당 채널의 1st stream rtsp port번호를 조회한다.
 * @param ch 채널 번호.
 * @returns 대상 rtsp port 번호.
 */
extern unsigned short nf_ipcam_get_rtsp_port(int ch)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	return (runtime[ch].sys.rtsp_port[0]);
}

/**
 * @brief 해당 채널의 ip주소를 조회한다.
 * @param ch 채널 번호.
 * @return 대상 ip주소.
 */
extern unsigned int nf_ipcam_get_ipaddr(int ch)
{
	mtable *runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	return (runtime[ch].sys.ipaddr);
}

extern unsigned int nf_ipcam_is_ssl(int ch)
{
	mtable *runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	return (runtime[ch].sys.use_ssl);
}

/**
 * @brief 현재 Command 전송 여부를 조회한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 * @return 현재 command전송 여부.
 */
extern unsigned int nf_ipcam_is_setup_sending(int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	return _ipcam_setup_sending[ch][type];
}

/**
 * @brief Command 전송 직전에 호출함.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
extern void nf_ipcam_setup_sending(int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	_ipcam_setup_sending[ch][type] = 1;
}

/**
 * @brief Command 전송 완료시 호출함.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
extern void nf_ipcam_setup_send_done(int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	_ipcam_setup_sending[ch][type] = 0;
}

/**
 * @brief Command 전송 성공시 response처리를 쓰레드에서 처리하기 위해 호출.
 * @param ch 채널 번호.
 * @param type Job 유형.
 * @param sock_r 전문 전송 및 수신에 사용할 socket.
 */
extern void nf_ipcam_setup_waiting(int ch, NF_IPCAM_SETUP_TYPE_E type, int sock_r)
{
	if(callbacks[ch][type].sock > 0)
	{
		printf("%s : overwrite to callbacks CH(%d) old_fd(%d) new_fd(%d) type(%s)\n", __func__, 
				ch, callbacks[ch][type].sock, sock_r, _jobs[type].t_str);
	}
	callbacks[ch][type].run_cnt = 0;
	callbacks[ch][type].sock = sock_r;
	callbacks[ch][type].picked = 1;
	_ipcam_setup_sending[ch][type] = 0;
}

/**
 * @brief 한 채널의 콜백 및 연결 소켓을 초기화한다.
 * @param ch 채널 번호.
 *
 * @sa cam_setup_restart
 */
extern void nf_ipcam_setup_clear_cb(int ch)
{
	int i = 0;

	for (i=0; i<NF_IPCAM_TYPE_MAX; i++)
	{
		if (callbacks[ch][i].sock > 0)
		{
			mtable *runtime = get_runtime();
			if (runtime[ch].sys.use_ssl)
			{
				_ipcam_ssl_close(ch, i);
			}
			close(callbacks[ch][i].sock);
			callbacks[ch][i].sock = (-1);
		}
	}
	memset((void*)&callbacks[ch][0], 0x00, sizeof(NFIPCamCallbacks)*NF_IPCAM_TYPE_MAX);
	//memset((void*)&ipcam_waiting[ch][0], 0x00, sizeof(int)*NF_IPCAM_TYPE_MAX);
}

/**
 * @brief 특정 Job이 현재 실행 대기상태인지 조회한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 * @return 현재 Job의 Queue 대기여부.
 *
 * Rate control에서만 사용.
 */
extern int nf_ipcam_get_setup_queuelen(gint ch, NF_IPCAM_SETUP_TYPE_E type)
{
	return callbacks[ch][type].reserved;
}

/**
 * @brief 콜백 함수 관련 설정을 초기 등록한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 * @param cb_fxn 설정 완료시 호출할 콜백 함수.
 * @param user_data 콜백 함수에 같이 넘길 user data.
 */
static void cam_setup_setcb
(	int ch,
	NF_IPCAM_SETUP_TYPE_E type,
	NFIPCamSetupCallback cb_fxn,
	gpointer user_data	)
{
	//IPCAM_DBG(MAJOR, "start CH(%d) %s\n", ch, _jobs[type].t_str);

	if ((callbacks[ch][type].picked == 1) || (_ipcam_setup_sending[ch][type] == 1))
	{
		if (callbacks[ch][type].picked)
		{
			IPCAM_DBG(WARN, "callbacks[CH(%d)][%s].picked - %d\n", ch, _jobs[type].t_str, callbacks[ch][type].picked);
		}
		if (_ipcam_setup_sending[ch][type])
		{
			IPCAM_DBG(WARN, "_ipcam_setup_sending[CH(%d)][%s] - %d\n", ch, _jobs[type].t_str, _ipcam_setup_sending[ch][type]);
		}
		IPCAM_DBG(WARN, "previous setup command is pending\n");
		if (cb_fxn != NULL)
		{
			callbacks[ch][type].cb_next = cb_fxn;
		}
		callbacks[ch][type].user_data_next = user_data;
		callbacks[ch][type].reserved = 1;
		return;
	}

	callbacks[ch][type].cb_func = cb_fxn;
	callbacks[ch][type].user_data = user_data;

	callbacks[ch][type].progress.type = type;
	callbacks[ch][type].progress.ch = ch;
	callbacks[ch][type].progress.current = 0;
	callbacks[ch][type].progress.total = 0;
	callbacks[ch][type].progress.status = NF_IPCAM_STATUS_BEGIN;
}

/**
 * @brief Job 처리가 완료(Response 수신 및 callback함수 호출 완료)되었을 때 마무리 작업.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
static void cam_setup_done(int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	mtable *runtime = get_runtime();
	if (runtime[ch].sys.use_ssl)
	{
		_ipcam_ssl_close(ch, type);
	}
	if (callbacks[ch][type].sock > 0)
	{
		close(callbacks[ch][type].sock);
	}
	callbacks[ch][type].sock = (-1);
	callbacks[ch][type].picked = 0;
	callbacks[ch][type].run_cnt = 0;

	if (callbacks[ch][type].progress.status == NF_IPCAM_STATUS_BEGIN)
		callbacks[ch][type].progress.status = NF_IPCAM_STATUS_END_SUCCESS;
	if (callbacks[ch][type].progress.status == NF_IPCAM_STATUS_PENDING)
		callbacks[ch][type].progress.status = NF_IPCAM_STATUS_END_SUCCESS;
}

/**
 * @brief Job 처리가 실패하였을 때 마무리 작업.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
static void cam_setup_failed_req(int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	mtable *runtime = get_runtime();
	if (runtime[ch].sys.use_ssl)
	{
		_ipcam_ssl_close(ch, type);
	}
	if (callbacks[ch][type].sock > 0)
	{
		close(callbacks[ch][type].sock);
	}
	callbacks[ch][type].sock = (-1);
	callbacks[ch][type].picked = 0;
	callbacks[ch][type].run_cnt = 0;

	if (callbacks[ch][type].progress.status == NF_IPCAM_STATUS_BEGIN)
		callbacks[ch][type].progress.status = NF_IPCAM_STATUS_FAILED_REQ;

	if (callbacks[ch][type].progress.status == NF_IPCAM_STATUS_PENDING)
		callbacks[ch][type].progress.status = NF_IPCAM_STATUS_FAILED_REQ;
}

/**
 * @brief 기존 Queue에 등록되어있던 Job을 실행한다.
 * @param info 설정과 관련된 정보가 위치한 포인터. Job 유형에 따라 다르다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 *
 * 처리 부분은 nf_ipcam_setup_request와 동일함.
 */
static void call_setup_reserved(void* info, int ch, NF_IPCAM_SETUP_TYPE_E type)
{
	int rtn = 0;
	mtable *runtime = NULL;

	int (*func0)(int);
	int (*func4)(int, int);
	int (*func28)(focus_comp_info*, int);
	int (*func44)(cam_info*, int);
	int (*func116)(image_info*, int);
	int (*func208)(NFIPCamPrivacyMask*, int);
	int (*func224)(NFIPCamSetupROIArea*, int);
	int (*func1568)(NFIPCamSetupMotionArea*, int);

	//IPCAM_DBG(MAJOR, "%s\n", _jobs[type].t_str);

	runtime = get_runtime();
	g_return_if_fail(runtime != NULL);

	// model_code�� onvif support check
	if(nf_ipcam_is_onvif_support(ch) == 1 &&
	  (type == NF_IPCAM_TYPE_SET_IMAGE_ONVIF
	|| type == NF_IPCAM_TYPE_SET_EXP_ONVIF
	|| type == NF_IPCAM_TYPE_SET_VCODEC
	|| type == NF_IPCAM_TYPE_SET_PAN_TILT
	|| type == NF_IPCAM_TYPE_SET_STOP
	|| type == NF_IPCAM_TYPE_SET_ORIGIN
	|| type == NF_IPCAM_TYPE_PRESET_SET
	|| type == NF_IPCAM_TYPE_PRESET_GO
	|| type == NF_IPCAM_TYPE_PRESET_CLEAR))
	{
		switch(type)
		{
			case NF_IPCAM_TYPE_SET_IMAGE_ONVIF:
				rtn = nf_onvif_set_image(ch, *((image_info_onvif *)info));
				break;

			case NF_IPCAM_TYPE_SET_EXP_ONVIF:
				rtn = nf_onvif_set_image(ch, *((image_info_onvif *)info));
				break;

			case NF_IPCAM_TYPE_SET_VCODEC:
				//rtn = nf_onvif_set_vcodec(ch);
				if (strcmp(runtime[ch].sys.vendor, "H264") == 0)
				{
					rtn = xiongmai_stream_set(ch,info);
					break;
				}
				rtn = nf_onvif_set_stream(ch, (cam_info*)info);
				break;

			case NF_IPCAM_TYPE_SET_PAN_TILT:
				rtn = nf_onvif_ptz_move(ch, *((ptz_info_onvif *)info));
				break;

			case NF_IPCAM_TYPE_SET_STOP:
				// TODO : ptz stop
				if(runtime[ch].ptz.moving & PTZ_SETUP_FOCUS)
				{
					rtn = nf_onvif_focus_stop(ch);
					runtime[ch].ptz.moving &= ~PTZ_SETUP_FOCUS;
				}
				if(runtime[ch].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT | PTZ_SETUP_ZOOM) )
				{
					rtn = nf_onvif_ptz_stop(ch);
					runtime[ch].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT | PTZ_SETUP_ZOOM);
				}

				rtn = IPCAM_SETUP_RTN_DONE;
				break;

			case NF_IPCAM_TYPE_SET_ORIGIN:
				rtn = nf_onvif_ptz_goto_home(ch);
				break;

			case NF_IPCAM_TYPE_PRESET_SET:
				rtn = nf_onvif_ptz_set_preset(ch, *((int*) info));
				break;

			case NF_IPCAM_TYPE_PRESET_GO:
				rtn = nf_onvif_ptz_goto_preset(ch, *((int*) info));
				break;

			case NF_IPCAM_TYPE_PRESET_CLEAR:
				rtn = nf_onvif_ptz_remove_preset(ch, *((int*) info));
				break;
		}
		nf_ipcam_setup_waiting(ch, type, (-1));
	}
	else
	{
		switch (_jobs[type].t_size)
		{
			case 0:
				if (runtime[ch].funcs[type] != NULL)
				{
					func0 = runtime[ch].funcs[type];
					rtn = func0(ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			case 4:
				if (runtime[ch].funcs[type] != NULL)
				{
					func4 = runtime[ch].funcs[type];
					rtn = func4( *((int*) info), ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			case 20:
				if(runtime[ch].funcs[type] != NULL)
				{
					int (*func20)(ptz_info*, int);
					func20 = runtime[ch].funcs[type];
					rtn = func20((ptz_info*)info, ch);
				}
				break;

			case 28:
				if (runtime[ch].funcs[type] != NULL)
				{
					func28 = runtime[ch].funcs[type];
					rtn = func28((focus_comp_info*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;

			case 84:
				if (runtime[ch].funcs[type] != NULL)
				{
					func44 = runtime[ch].funcs[type];
					rtn = func44((cam_info*)info, ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			case 120:
				if (runtime[ch].funcs[type] != NULL)
				{
					func116 = runtime[ch].funcs[type];
					rtn = func116((image_info*)info, ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			case 208:
				if (runtime[ch].funcs[type] != NULL)
				{
					func208 = runtime[ch].funcs[type];
					rtn = func208((NFIPCamPrivacyMask*)info, ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			case 224:
				if (runtime[ch].funcs[type] != NULL)
				{
					func224 = runtime[ch].funcs[type];
					rtn = func224((NFIPCamSetupROIArea*)info, ch);
				}
				else
				{
					nf_ipcam_setup_waiting(ch, type, (-1));
					rtn = IPCAM_SETUP_RTN_DONE;
				}
				break;
			case 1568:
				if (runtime[ch].funcs[type] != NULL)
				{
					func1568 = runtime[ch].funcs[type];
					rtn = func1568((NFIPCamSetupMotionArea*)info, ch);
				}
				else
				{
					rtn = IPCAM_SETUP_RTN_DONE;
					nf_ipcam_setup_waiting(ch, type, (-1));
				}
				break;
			default:
				g_warning("[%s] job requested with wrong param(%d)\n", __FUNCTION__, _jobs[type].t_size);
				rtn = IPCAM_SETUP_RTN_FAILED;
				break;
		}
	}

	switch (rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			cam_setup_failed_req(ch, type);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			break;
		default:
			g_warning("[%s] Return code wrong(%d)\n", __FUNCTION__, rtn);
			break;
	}
}

/**
 * @brief 카메라 접속 루틴 쓰레드 1.
 */
static void nf_ipcam_prepare_th_func(void)
{
	cam_prepare_func();
}

/**
 * @brief 카메라 접속 루틴 쓰레드 2.
 *
 * Discover 및 init된 카메라를 찾아 접속 루틴을 수행한다.
 */
static void cam_prepare_func(void)
{
	int i;

	//IPCAM_DBG(MAJOR, "start\n");
	while(1)
	{
		usleep(100*1000);
		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (_discovered_devices_mask & (1<<i))
			{
				//IPCAM_DBG(MAJOR, "discovered devices goes %08x(%08x)\n", (1<<i), _discovered_devices_mask);
				_discovered_devices_mask &= (~(1<<i));
				discovered_device_handler(i);
				//IPCAM_DBG(MAJOR, "discovered devices handled(%08x)\n", _discovered_devices_mask);
			}

			if (_init_devices_mask & (1<<i))
			{
				//IPCAM_DBG(MAJOR, "device initializer goes %08x(%08x)\n", (1<<i), _init_devices_mask);
				_init_devices_mask &= (~(1<<i));
				device_initializer(i);
				//IPCAM_DBG(MAJOR, "device initializer handled(%08x)\n", _init_devices_mask);
			}
		}
	}
}

/**
 * @brief 모든 카메라의 Job들의 실행이 끝났는지 확인한다.(ipcam_stop하기 위해)
 * @return 1 - Job 실행완료.
 *
 * @deprecated nf_ipcam_stop 참조.
 */
extern int is_setup_th_ready_to_stop(void)
{
	int i=0,j=0;

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		for (j=0; j<NF_IPCAM_TYPE_MAX; j++)
		{
			if (_ipcam_setup_sending[i][j] != 0)
			{
				return 0;
			}

			if (callbacks[i][j].picked != 0)
			{
				return 0;
			}
		}
	}

	return 1;
}

/**
 * @brief Job실행을 강제 종료한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
extern void nf_ipcam_setup_clear(int ch, unsigned int type)
{
#if 0
	while(1)
	{
		if (_ipcam_setup_sending[ch][type] == 1)
		{
			usleep(100*1000);
			continue;
		}
		break;
	}
#endif

	callbacks[ch][type].picked = 0;
	if (callbacks[ch][type].sock > 0)
	{
		mtable *runtime = get_runtime();
		if (runtime[ch].sys.use_ssl)
		{
			_ipcam_ssl_close(ch, type);
		}
		close(callbacks[ch][type].sock);
		callbacks[ch][type].sock = (-1);
	}
	_ipcam_setup_sending[ch][type] = 0;
}

/**
 * @brief 카메라 Job관리 쓰레드 1.
 */
static void nf_ipcam_setup_th_func(void *arg)
{
    int start_ch;
    int end_ch;
    start_ch = (int)arg;
    end_ch = start_ch + IPCAM_MANAGER_CAM_COUNT;
    if(end_ch > AVAILABLE_MAX_CH)
        end_ch = AVAILABLE_MAX_CH;
	cam_setup_func(start_ch, end_ch);
}

/**
 * @brief 카메라 Job관리 쓰레드 2.
 *
 * Response 수신, timeout처리 및 callback함수 호출.
 */
static void cam_setup_func(int start_ch, int end_ch)
{
	int i = 0;
	int j = 0;
	int status = 0;
	int len = 0;
	struct timespec timeNow;
	char buf[2048];
	mtable *runtime = NULL;
	int (*func_poll_event)(int);
	static unsigned int csf_call_cnt = 0;
	char *errcode = NULL;
	NFIPCamProgress *prog = NULL;
	struct timeval curr_tv, prev_tv;

    if(start_ch < 0 || start_ch >= end_ch){
        printf("[%s:%d] err start_ch[%d] end_ch[%d]\n", __func__, __LINE__, start_ch, end_ch);
        return;
    }

	gettimeofday(&prev_tv, NULL);
	runtime = get_runtime();
	g_assert(runtime != NULL);
	while(1)
	{
		/* Day and Night time calculation */
		{
			if(gettimeofday(&curr_tv, NULL) != 0){  /* error routine */     }

			/* calls every sec, calculate_day_night() */
			if(curr_tv.tv_sec - prev_tv.tv_sec >= 1)
			{
				calculate_dnn(); // Day & Night Schedule
				calculate_day_night(); // Motion
			}

			if(memcpy(&prev_tv, &curr_tv, sizeof(struct timeval)) == NULL) { /* error routine */ }
		}

		csf_call_cnt++;

		if ((csf_call_cnt % 50) == 0) { IPCAM_DBG(MAJOR, "SETUP THREAD RUNNING\n"); }

		usleep(20*1000);
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			for (i=start_ch; i<end_ch; i++)
			{
				for (j=0; j<NF_IPCAM_TYPE_MAX; j++)
				{
					_ipcam_setup_sending[i][j] = 0;
					callbacks[i][j].picked = 0;
				}
			}
			continue;
		}
		if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
		{
			for (i=start_ch; i<end_ch; i++)
			{
				for (j=0; j<NF_IPCAM_TYPE_MAX; j++)
				{
					_ipcam_setup_sending[i][j] = 0;
					callbacks[i][j].picked = 0;
				}
			}
		}
		for (i = start_ch; i < end_ch; i++)
		{
			if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
			{
				break;
			}
			if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
			{
				break;
			}
			if (_wait_cnt[i] > 0)
			{
				_wait_cnt[i]--;
				continue;
			}

			for (j = 0; j < NF_IPCAM_TYPE_MAX; j++)
			{
				if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
				{
					break;
				}
				if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
				{
					break;
				}
				if (callbacks[i][j].picked == 0)
				{
					continue;
				}

#if 0
				if (callbacks[i][j].run_cnt > 300)
				{
					IPCAM_DBG(MINOR, "CH(%d) type[%d] timeout(picked(%d) sock(%d) run_cnt(%d)\n",
							i, j,
							callbacks[i][j].picked,
							callbacks[i][j].sock,
							callbacks[i][j].run_cnt);
					if (callbacks[i][j].sock > 0)
					{
						close(callbacks[i][j].sock);
					}
					callbacks[i][j].sock = (-1);
					if (callbacks[i][j].reserved == 0)
					{
						callbacks[i][j].picked = 0;
					}
					callbacks[i][j].run_cnt = 0;
					continue;
				}
#endif

				memset(buf, 0x00, sizeof(buf));
				if (callbacks[i][j].sock <= 0)
				{
					//IPCAM_DBG(MINOR, "CH(%d) type[%s] no socket\n", i, _jobs[j].t_str);
					cam_setup_done(i, j);
					continue;
				}

				if (runtime[i].sys.use_ssl)
				{
					char recv_msg[2048];
					int str_len = 0;
					pthread_mutex_lock(&runtime[i].sys.ssl_mtx[j]);
					if (runtime[i].sys.ssl_state[j] != IPCAM_SSL_WAITING)
					{
						pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);
						callbacks[i][j].run_cnt++;
						continue;
					}
					if ((len=SSL_read(runtime[i].sys.ssl[j], buf, sizeof(buf))) <= 0)
					{
						callbacks[i][j].run_cnt++;
						pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);
						continue;
					}
					str_len += len;
					strncpy(recv_msg, buf, len);

					pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);
#if PRINT_CONNECTION_HTTP_API
					printf("===========================================================\n");
					IPCAM_DBG(MINOR, "CH(%d) TYPE(%s)\n", i, _jobs[j].t_str);
					printf("%s\n", buf);
					printf("===========================================================\n");
#endif

					errcode = strstr(buf, "401 Unauthorized");
					if (errcode != NULL)
					{
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL auth fail\n", i, _jobs[j].t_str);
						errcode = strstr(buf, "WWW-Authenticate: Digest");
						if (errcode == NULL)
						{
							IPCAM_DBG(WARN, "CH(%d) type(%d) SSL request failed\n", i, j);
							cam_setup_failed_req(i, j);
							continue;
						}

						_ipcam_ssl_close(i, j);

						if (callbacks[i][j].sock > 0)
						{
							close(callbacks[i][j].sock);
							callbacks[i][j].sock = (-1);
						}
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL digest start\n", i, _jobs[j].t_str);
						itx_digest_setup_again(i, j, buf);
						continue;
					}
					if (strstr(buf, "200 OK") == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) type[%s] SSL no code 200 OK\n", i, _jobs[j].t_str);
						cam_setup_failed_req(i, j);
						continue;
					}

					if (strstr(buf, "Content-Length") != NULL)
					{
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL received content found\n", i, _jobs[j].t_str);
						if (runtime[i].recv_handler != NULL)
						{
#if PRINT_CONNECTION_HTTP_API
							IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL recv_handler content goes(%s)\n", i, _jobs[j].t_str, buf);
#endif
							int (*recv_handler)(int, int, char*);

							recv_handler = runtime[i].recv_handler;
							recv_handler(i, j, buf);
						}
					}
					else if (strstr(buf, "chunked") != NULL)
					{
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL chunks found\n", i, _jobs[j].t_str);
						//memset(buf, 0x00, sizeof(buf));
						pthread_mutex_lock(&runtime[i].sys.ssl_mtx[j]);
						//if (runtime[i].sys.ssl_state[j] != IPCAM_SSL_WAITING)
						/*
						while(1)
						{
							memset(buf, 0x00, sizeof(buf));
							if((len = SSL_read(runtime[i].sys.ssl[j], buf, 2048)) <= 0)
							{
								break;
							}
							else
							{
								if(str_len > 2048){
									break;
								}
								strncpy(recv_msg[str_len], buf, len);
								str_len += len;
							}
						}
						*/
						pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);
						if(str_len > 2048){
								cam_setup_failed_req(i, j);
								continue;
						}
					

						memset(buf, 0x00, sizeof(buf));
						pthread_mutex_lock(&runtime[i].sys.ssl_mtx[j]);
						if (runtime[i].sys.ssl_state[j] != IPCAM_SSL_WAITING)
						{
							pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);
							continue;
						}
						pthread_mutex_unlock(&runtime[i].sys.ssl_mtx[j]);

						if (runtime[i].recv_handler != NULL)
						{
#if PRINT_CONNECTION_HTTP_API
							IPCAM_DBG(MINOR, "CH(%d) type[%s] SSL recv_handler chunk goes(%s)\n", i, _jobs[j].t_str, recv_msg);
#endif
							int (*recv_handler)(int, int, char*);

							recv_handler = runtime[i].recv_handler;
							recv_handler(i, j, recv_msg);
						}
					}
				}
				else
				{
					const char f_ok[] = "200 OK";
					const char f_auth_fail[] = "401 Unauthorized";
					mtable *runtime = get_runtime();

					if (recv(callbacks[i][j].sock, buf, sizeof(buf), MSG_DONTWAIT|MSG_PEEK) <= 0)
					{
						callbacks[i][j].run_cnt++;
						continue;
					}
					recv(callbacks[i][j].sock, buf, sizeof(buf), MSG_DONTWAIT);

#if PRINT_CONNECTION_HTTP_API
					printf("===========================================================\n");
					IPCAM_DBG(MINOR, "CH(%d) TYPE(%s)\n", i, _jobs[j].t_str);
					printf("%s\n", buf);
					printf("===========================================================\n");
#endif
					errcode = strstr(buf, f_auth_fail);
					if (errcode != NULL)
					{
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] auth fail\n", i, _jobs[j].t_str);
						errcode = strstr(buf, "WWW-Authenticate: Digest");
						if (errcode == NULL)
						{
							IPCAM_DBG(WARN, "ch(%d) type(%d) request failed\n", i, j);
							cam_setup_failed_req(i, j);
							continue;
						}

						close(callbacks[i][j].sock);
						callbacks[i][j].sock = (-1);
						if (callbacks[i][j].reserved == 0)
						{
							callbacks[i][j].picked = 0;
						}
						callbacks[i][j].run_cnt = 0;
						//IPCAM_DBG(MINOR, "CH(%d) type[%s] digest start\n", i, _jobs[j].t_str);
						itx_digest_setup_again(i, j, buf);
						continue;
					}
					else
					{
						errcode = strstr(buf, f_ok);
						if (errcode == NULL)
						{
							IPCAM_DBG(WARN, "CH(%d) type[%s] no code 200 OK\n", i, _jobs[j].t_str);
							cam_setup_failed_req(i, j);
							continue;
						}
					}

					if (runtime[i].recv_handler != NULL)
					{
#if PRINT_CONNECTION_HTTP_API
						IPCAM_DBG(MINOR, "CH(%d) type[%s] recv_handler goes(%s)\n", i, _jobs[j].t_str, buf);
#endif
						int (*recv_handler)(int, int, char*);

						recv_handler = runtime[i].recv_handler;
						recv_handler(i, j, buf);
					}
				}

				cam_setup_done(i, j);
				continue;
			}
		}
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			continue;
		}
		if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
		{
			continue;
		}

		//usleep(20*1000);
		usleep(10*1000);
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			continue;
		}
		if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
		{
			continue;
		}
		for (i = start_ch; i < end_ch; i++)
		{
			if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
			{
				break;
			}
			if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
			{
				break;
			}
			for (j = 0; j < NF_IPCAM_TYPE_MAX; j++)
			{
				if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
				{
					break;
				}
				if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
				{
					break;
				}
				status = callbacks[i][j].progress.status;

				switch(status)
				{
					case NF_IPCAM_STATUS_NORMAL:
						break;
					case NF_IPCAM_STATUS_BEGIN:
					{
						clock_gettime(CLOCK_REALTIME, &timeNow);
						prog = &callbacks[i][j].progress;
						prog->timeBegin = timeNow.tv_sec;
						prog->timeElapsed = 0;
						prog->timeout = _jobs[j].t_timeout;
						//IPCAM_DBG(MINOR, "New job enqueued: CH(%d) type[%s] timeout[%d]\n",
						//		i, _jobs[j].t_str, _jobs[j].t_timeout);

						if (callbacks[i][j].cb_func != NULL)
							callbacks[i][j].cb_func(prog, callbacks[i][j].user_data);

						prog->status = NF_IPCAM_STATUS_PENDING;
						break;
					}
					case NF_IPCAM_STATUS_PENDING:
					{
						clock_gettime(CLOCK_REALTIME, &timeNow);
						prog = &callbacks[i][j].progress;
						prog->timeElapsed = timeNow.tv_sec - prog->timeBegin;
						if (callbacks[i][j].cb_func != NULL)
							callbacks[i][j].cb_func(prog, callbacks[i][j].user_data);

						if (prog->timeElapsed >= prog->timeout)
						{
							prog->status = NF_IPCAM_STATUS_TIMEOUT;
							nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
						}

						break;
					}
					case NF_IPCAM_STATUS_FAILED_REQ:
					case NF_IPCAM_STATUS_TIMEOUT:
					case NF_IPCAM_STATUS_END_SUCCESS:
					{
						//IPCAM_DBG(MINOR, "Job end: CH(%d) type[%s] timeout[%d] waitcnt[%d] "
						//		"reserved[%d] pick[%d] runcnt[%d] [%s] \n",
						//		i, _jobs[j].t_str, _jobs[j].t_timeout, _wait_cnt[i],
						//		callbacks[i][j].reserved, callbacks[i][j].picked, callbacks[i][j].run_cnt,
						//		_job_status[status].t_str);

#if 1
						if (status == NF_IPCAM_STATUS_TIMEOUT)
						{
							if (runtime[i].sys.use_ssl)
							{
								_ipcam_ssl_close(i, j);
							}
							if (callbacks[i][j].sock > 0)
							{
								close(callbacks[i][j].sock);
							}
							callbacks[i][j].sock = (-1);
							callbacks[i][j].picked = 0;
							callbacks[i][j].run_cnt = 0;
						}
#endif

						clock_gettime(CLOCK_REALTIME, &timeNow);
						prog = &callbacks[i][j].progress;
						prog->timeElapsed = timeNow.tv_sec - prog->timeBegin;
						if (callbacks[i][j].cb_func != NULL)
							callbacks[i][j].cb_func(prog, callbacks[i][j].user_data);

						if (callbacks[i][j].reserved == 1)
						{
							callbacks[i][j].picked = 1;
							callbacks[i][j].reserved = 0;

							callbacks[i][j].cb_func = callbacks[i][j].cb_next;
							callbacks[i][j].cb_next = NULL;
							callbacks[i][j].user_data = callbacks[i][j].user_data_next;
							callbacks[i][j].user_data_next = NULL;

							callbacks[i][j].progress.type = j;
							callbacks[i][j].progress.ch = i;
							callbacks[i][j].progress.stream = 0;
							callbacks[i][j].progress.current = 0;
							callbacks[i][j].progress.total = 0;
							callbacks[i][j].progress.status = NF_IPCAM_STATUS_BEGIN;


							call_setup_reserved(&callbacks[i][j].data_next, i, j);
						}
						else
						{
							if(callbacks[i][j].sock > 0)
							{
								printf("%s : CH(%d) Wrong memset callbacks CH(%d) fd(%d) type(%s) job(%s)\n", 
										__func__, i,i, callbacks[i][j].sock, _jobs[j].t_str, _job_status[status].t_str);
							}

							memset(&callbacks[i][j], 0x00, sizeof(NFIPCamCallbacks));
						}
						break;
					}
					default:
						break;
				}
			}
		}

		/* Polling methods call */
		//usleep(20*1000);
		usleep(10*1000);
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			continue;
		}
		if (nf_get_running_mode() == 1 && nf_openmode_get_state() == OPENMODE_STATE_INIT)
		{
			continue;
		}
		if (csf_call_cnt % 4 == 0)
		{
			for (i = start_ch; i < end_ch; i++)
			{
				if (runtime[i].state & MGMT_STATE_CONFIGURED == 0) { continue; }
				if (callbacks[i][NF_IPCAM_TYPE_POLL_EVENT].progress.status == NF_IPCAM_STATUS_BEGIN || callbacks[i][NF_IPCAM_TYPE_POLL_EVENT].progress.status == NF_IPCAM_STATUS_PENDING) { continue; }
				if (runtime[i].funcs[NF_IPCAM_TYPE_POLL_EVENT] != NULL)
				{
					func_poll_event = runtime[i].funcs[NF_IPCAM_TYPE_POLL_EVENT];
					func_poll_event(i);
				}
			}
		}
	}
}


/**
 * @brief 전역변수 초기화 및 카메라 설정 관련 쓰레드를 생성한다.
 */
static void init_ipcam_manager(void)
{
    int i;
	GAsyncQueue *queue;
	if (nf_get_running_mode() == 0)
	{
		queue = get_queue();
		while(queue == NULL)
		{
			usleep(500*1000);
			queue = get_queue();
		}
	}

	init_remember_buf();
	memset(_wait_cnt, 0x00, sizeof(int)*AVAILABLE_MAX_CH);
	memset(_ipcam_setup_sending, 0x00, sizeof(int)*AVAILABLE_MAX_CH*NF_IPCAM_TYPE_MAX);
	memset(callbacks, 0x00, sizeof(NFIPCamCallbacks) * AVAILABLE_MAX_CH * NF_IPCAM_TYPE_MAX);

    for(i = 0; i < AVAILABLE_MAX_CH; i+=IPCAM_MANAGER_CAM_COUNT){
        pthread_t th;
        pthread_create(&th, NULL, (void*)&nf_ipcam_setup_th_func, i);
    }
	pthread_create(&cam_prepare_th, NULL, (void*)&nf_ipcam_prepare_th_func, NULL);
	//pthread_create(&cam_prepare_th, NULL, (void*)&cam_prepare_func, NULL);
}

/**
 * @brief Model db에서 해당 ITX카메라 모델을 검색한다.
 * @param port 채널 번호.
 * @return __IPX_IPCAM_RESULTS_ 참조.
 */
static int modeldb_search(int port)
{
	return (build_management_table(port));
}

/**
 * @brief Model db에서 해당 ONVIF카메라 모델을 검색한다.
 * @param port 채널 번호.
 * @return __IPX_IPCAM_RESULTS_ 참조.
 */
static int modeldb_search_onvif(int port)
{
	return (build_management_table_onvif(port));
}

/**
 * @brief Sysdb 녹화설정의 해상도 옵션을 카메라 설정에서 쓰이는 해상도 enum값으로 변경한다.
 * @param a Sysdb 녹화설정 char.
 * @return 카메라 해상도 enum값.
 *
 * @see NF_IPCAM_RES_E
 */
static uint64_t _change_szdb_to_camgr(char a)
{
	uint64_t rtn = 0;
	switch(a)
	{
		case 'A': { rtn = 0; break; }
		case 'B': { rtn = NF_IPCAM_RES_352x240; break; }
		//case 'C':
		//case 'D':
		case 'E': { rtn = NF_IPCAM_RES_704x480; break; }
		case 'F': { rtn = NF_IPCAM_RES_352x288; break; }
		//case 'G':
		//case 'H':
		case 'I': { rtn = NF_IPCAM_RES_704x576; break; }
		case 'J': { rtn = NF_IPCAM_RES_640x480; break; }
		case 'K': { rtn = NF_IPCAM_RES_720x480; break; }
		case 'L': { rtn = NF_IPCAM_RES_720x576; break; }
		case 'M': { rtn = NF_IPCAM_RES_800x600; break; }
		case 'N': { rtn = NF_IPCAM_RES_1024x768; break; }
		case 'O': { rtn = NF_IPCAM_RES_1280x1024; break; }
		case 'P': { rtn = NF_IPCAM_RES_1600x1200; break; }
		case 'Q': { rtn = NF_IPCAM_RES_1280x720; break; }
		case 'R': { rtn = NF_IPCAM_RES_1920x1080; break; }
		case 'S': { rtn = NF_IPCAM_RES_640x352; break; }
		case 'T': { rtn = NF_IPCAM_RES_640x360; break; }
		//case 'U': { rtn = NF_IPCAM_RES_640x480; break; }
		//case 'V': { rtn = NF_IPCAM_RES_640x480; break; }
		//case 'W': { rtn = NF_IPCAM_RES_640x480; break; }
		case 'X': { rtn = NF_IPCAM_RES_640x400; break; }
		case 'Y': { rtn = NF_IPCAM_RES_800x450; break; }
		case 'Z': { rtn = NF_IPCAM_RES_1440x900; break; }
		case 'c': { rtn = NF_IPCAM_RES_320x180; break; }
		case 'd': { rtn = NF_IPCAM_RES_2304x1296; break; }
		case 'e': { rtn = NF_IPCAM_RES_2048x1536; break; }
		case 'f': { rtn = NF_IPCAM_RES_2560x1440; break; }
		case 'g': { rtn = NF_IPCAM_RES_2688x1520; break; }
		case 'h': { rtn = NF_IPCAM_RES_2560x1600; break; }
		case 'i': { rtn = NF_IPCAM_RES_2560x1920; break; }
		case 'j': { rtn = NF_IPCAM_RES_2592x1920; break; }
		case 'k': { rtn = NF_IPCAM_RES_2592x1944; break; }
		case 'l': { rtn = NF_IPCAM_RES_2992x1680; break; }
		case 'm': { rtn = NF_IPCAM_RES_2880x1800; break; }
		case 'n': { rtn = NF_IPCAM_RES_3200x1800; break; }
		case 'o': { rtn = NF_IPCAM_RES_2880x2160; break; }
		case 'p': { rtn = NF_IPCAM_RES_3072x2048; break; }
		case 'q': { rtn = NF_IPCAM_RES_3200x2400; break; }
		case 'r': { rtn = NF_IPCAM_RES_3840x2160; break; }
		case 's': { rtn = NF_IPCAM_RES_2592x1520; break; }
		case '1': { rtn = NF_IPCAM_RES_3000x3000; break; }
		case '2': { rtn = NF_IPCAM_RES_2048x2048; break; }
		case '3': { rtn = NF_IPCAM_RES_1280x1280; break; }
		case '4': { rtn = NF_IPCAM_RES_640x640; break; }
		case '5': { rtn = NF_IPCAM_RES_320x320; break; }
	}

	return rtn;
}

/**
 * @brief 카메라 설정에서 쓰이는 해상도 enum값을 Sysdb 녹화설정의 해상도 문자로 변경한다.
 * @param a 카메라 해상도 enum값.
 * @return Sysdb 녹화설정 char.
 */
static char _change_camgr_to_sysdb(uint64_t a)
{
	char rtn = 0;
	if(a == NF_IPCAM_RES_352x240){rtn = 'B';}
	if(a == NF_IPCAM_RES_704x480){rtn = 'E';}
	if(a == NF_IPCAM_RES_352x288){rtn = 'F';}
	if(a == NF_IPCAM_RES_704x576){rtn = 'I';}
	if(a == NF_IPCAM_RES_640x480){rtn = 'J';}
	if(a == NF_IPCAM_RES_720x480){rtn = 'K';}
	if(a == NF_IPCAM_RES_720x576){rtn = 'L';}
	if(a == NF_IPCAM_RES_800x600){rtn = 'M';}
	if(a == NF_IPCAM_RES_1024x768){rtn = 'N';}
	if(a == NF_IPCAM_RES_1280x1024){rtn = 'O';}
	if(a == NF_IPCAM_RES_1600x1200){rtn = 'P';}
	if(a == NF_IPCAM_RES_1280x720){rtn = 'Q';}
	if(a == NF_IPCAM_RES_1920x1080){rtn = 'R';}
	if(a == NF_IPCAM_RES_640x352){rtn = 'S';}
	if(a == NF_IPCAM_RES_640x360){rtn = 'T';}
	if(a == NF_IPCAM_RES_640x400){rtn = 'X';}
	if(a == NF_IPCAM_RES_800x450){rtn = 'Y';}
	if(a == NF_IPCAM_RES_1440x900){rtn = 'Z';}
	if(a == NF_IPCAM_RES_320x180){rtn = 'c';}
	if(a == NF_IPCAM_RES_2304x1296){rtn = 'd';}
	if(a == NF_IPCAM_RES_2048x1536){rtn = 'e';}
	if(a == NF_IPCAM_RES_2560x1440){rtn = 'f';}
	if(a == NF_IPCAM_RES_2688x1520){rtn = 'g';}
	if(a == NF_IPCAM_RES_2560x1600){rtn = 'h';}
	if(a == NF_IPCAM_RES_2560x1920){rtn = 'i';}
	if(a == NF_IPCAM_RES_2592x1920){rtn = 'j';}
	if(a == NF_IPCAM_RES_2592x1944){rtn = 'k';}
	if(a == NF_IPCAM_RES_2992x1680){rtn = 'l';}
	if(a == NF_IPCAM_RES_2880x1800){rtn = 'm';}
	if(a == NF_IPCAM_RES_3200x1800){rtn = 'n';}
	if(a == NF_IPCAM_RES_2880x2160){rtn = 'o';}
	if(a == NF_IPCAM_RES_3072x2048){rtn = 'p';}
	if(a == NF_IPCAM_RES_3200x2400){rtn = 'q';}
	if(a == NF_IPCAM_RES_3840x2160){rtn = 'r';}
	if(a == NF_IPCAM_RES_2592x1520){rtn = 's';}
	if(a == NF_IPCAM_RES_3000x3000){rtn = '1';}
	if(a == NF_IPCAM_RES_2048x2048){rtn = '2';}
	if(a == NF_IPCAM_RES_1280x1280){rtn = '3';}
	if(a == NF_IPCAM_RES_640x640){rtn = '4';}
	if(a == NF_IPCAM_RES_320x320){rtn = '5';}

	return rtn;
}

static void _change_cambitctrl_to_sysdb(gchar *bitctrl,  unsigned int a)
{
	char *rtn = 0;

	if(bitctrl == NULL) { return; }

	switch(a)
	{
		case NF_IPCAM_BITRATE_CONTROL_CBR : rtn = "CBR"; break;
		case NF_IPCAM_BITRATE_CONTROL_VBR : rtn = "VBR"; break;
		case NF_IPCAM_BITRATE_CONTROL_MBR : rtn = "MBR"; break;
		case NF_IPCAM_BITRATE_CONTROL_VBR_PLUS : rtn = "IDNR"; break;
		default : rtn = ""; break;
	}
	strcpy(bitctrl, rtn);
}

static unsigned int _change_bitctrldb_to_camgr(char a)
{
	unsigned int rtn = 0;
	switch(a)
	{
		case 'C' : rtn = NF_IPCAM_BITRATE_CONTROL_CBR; break;
		case 'V' : rtn = NF_IPCAM_BITRATE_CONTROL_VBR; break;
		case 'M' : rtn = NF_IPCAM_BITRATE_CONTROL_MBR; break;
		case 'I' : rtn = NF_IPCAM_BITRATE_CONTROL_VBR_PLUS; break;
		default: rtn = 0; break;
	}

	return rtn;
}

static unsigned int _change_vcodec_db_to_camgr(char *a)
{
	unsigned int rtn = 0;

	if(a == NULL){ return 0; }

	if (strcmp(a, "H.264") == 0)
	{
		rtn = NF_IPCAM_VCODEC_H264;
	}
	else if (strcmp(a, "H.265") == 0)
	{
		rtn = NF_IPCAM_VCODEC_H265;
	}
	else { rtn = 0; }

	return rtn;
}

static void _change_cam_vcodec_to_sysdb(gchar *vcodec, unsigned int a)
{
	char *rtn = 0;

	if(vcodec == NULL){ return 0; }

	switch(a)
	{
		case NF_IPCAM_VCODEC_H264 : rtn = "H.264"; break;
		case NF_IPCAM_VCODEC_H265 : rtn = "H.265"; break;
		default: rtn = ""; break;
	}

	strcpy(vcodec, rtn);
}

/**
 * @brief Sysdb에 카메라 스트림 설정을 저장한다.(nfdal.c에서 복사)
 * @param data 스트림 설정 struct.
 * @param channel 채널 번호.
 * @return 0 - 성공, etc - 실패.
 */
static guint _prvSetStreamData(StreamData data, guint channel)
{
	GValue set_value = {0,};
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for (i = 0; i < 2; i++)
    {
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.size[i]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.size", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.control[i]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.bitctrl", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.vcodec[i]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.vcodec", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.max_fps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.fps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 2;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.max_bps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.max_bps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 3;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.min_bps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.min_bps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 4;
    	}
    	g_value_unset(&set_value);
    }

    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    return 0;
}

/**
 * @brief 최초 접속시 ITX카메라의 runtime정보와 sysdb를 동기화한다.
 * @param port 채널 번호.
 *
 * 카메라의 모델명을 sysdb와 비교하여 신규 모델일시 runtime->sysdb 동기화, 동일 모델일시 sysdb->runtime 동기화 실시.
 */
static void sysdb_load(int port)
{
	unsigned int bri, con, col, tin, sha, rot, etc, ntp, img_ntp;
	int notsupport = 0;
	gchar *model_nm;
	char key[64];
	mtable *runtime = get_runtime();

	/* sysdb init scenario changed!! */

	snprintf(key, 64, "cam.C%d.model_nm", port);
	model_nm = nf_sysdb_get_str(key);

	if(strcmp(model_nm, runtime[port].sys.stdver) == 0)
	{
		//IPCAM_DBG(MINOR,"sysdb load (ch:%d) (model:%s)\n", port, model_nm);

		if(1)
		{
			snprintf(key, 64, "cam.C%d.bright", port);
			bri = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.contrast", port);
			con = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.tint", port);
			tin = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.color", port);
			col = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.sharpness", port);
			sha = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.rotate", port);
			rot = nf_sysdb_get_uint(key);
			snprintf(key, 64, "sys.info.sig_type");
			ntp = nf_sysdb_get_bool(key);
			snprintf(key, 64, "cam.C%d.antiflicker", port);
			img_ntp = nf_sysdb_get_uint(key);


			runtime[port].image.brightness.value = bri;
			runtime[port].image.contrast.value = con;
			runtime[port].image.tint.value = tin;
			runtime[port].image.color.value = col;

			runtime[port].image.sharpness.value = sha;

			if (rot & runtime[port].video.mirror.support)
			{
				runtime[port].video.mirror.value = rot;
			}

			if(img_ntp & runtime[port].image.anti_flicker.support)
			{
				runtime[port].image.anti_flicker.value = img_ntp;
			}
			else
			{
				notsupport = 1;
			}

			if (ntp == 1)
			{
				runtime[port].video.anti_flicker.value = (1<<ntp);
			}

			snprintf(key, 64, "cam.C%d.focus_limit", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.focus_limit.support)
			{
				runtime[port].image.focus_limit.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.stabilizer", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.stabilizer.support)
			{
				runtime[port].image.stabilizer.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.ir_correction", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.ir_correction.support)
			{
				runtime[port].image.ir_correction.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.exposure_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.exposure.support)
			{
				runtime[port].image.exposure.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			// SWIPXXP-691
			snprintf(key, 64, "cam.C%d.max_shutter", port);
            etc = nf_sysdb_get_uint(key);
            if (etc & runtime[port].image.max_shutter.support)
            {
                runtime[port].image.max_shutter.value = etc;
            }
			else
			{
				notsupport = 1;
			}

            snprintf(key, 64, "cam.C%d.base_shutter", port);
            etc = nf_sysdb_get_uint(key);
            if (etc & runtime[port].image.base_shutter.support)
            {
                runtime[port].image.base_shutter.value = etc;
            }
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.agc_gain", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.agc.value = etc;

			snprintf(key, 64, "cam.C%d.gain", port);
			etc = nf_sysdb_get_int(key);
			if(etc != 0)
			{
				runtime[port].image.agc.value = etc;
			}

			snprintf(key, 64, "cam.C%d.shutter_speed", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.eshutter_speed.value = etc;

			snprintf(key, 64, "cam.C%d.etime", port);
			etc = nf_sysdb_get_int(key);
			if(etc != 0)
			{
				runtime[port].image.eshutter_speed.value = etc;
			}

			int dnn_ntod, dnn_dton;
			snprintf(key, 64, "cam.C%d.dnn_ntod", port);
			etc = nf_sysdb_get_uint(key);
			{
				dnn_ntod = etc;
			}

			snprintf(key, 64, "cam.C%d.dnn_dton", port);
			etc = nf_sysdb_get_uint(key);
			{
				dnn_dton = etc;
			}

			if(!((dnn_ntod <= 0 && dnn_dton <= 0) || (dnn_ntod - dnn_dton < 3) || dnn_ntod > 10 || dnn_dton > 7))
			{
				runtime[port].image.dnn_sense_dton.value = dnn_dton;
				runtime[port].image.dnn_sense_ntod.value = dnn_ntod;
			}

			snprintf(key, 64, "cam.C%d.slow_shutter", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.slow_shutter.support)
			{
				runtime[port].image.slow_shutter.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.max_agc", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.max_agc.support)
			{
				runtime[port].image.max_agc.value = etc;
			}
			else
			{
				notsupport = 1;
			}

		#if 0
			snprintf(key, 64, "cam.C%d.iris_control", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.iris.support)
			{
				runtime[port].image.iris.value = etc;
			}
			/*else
			{
				notsupport = 1;
			}*/
		#else
			snprintf(key, 64, "cam.C%d.iris_control", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.iris.value = etc;
		#endif

			snprintf(key, 64, "cam.C%d.blc_control", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.blc.support)
			{
				runtime[port].image.blc.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.day_night_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.day_night.support)
			{
				runtime[port].image.day_night.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.day_night_duration", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.tg_time.support)
			{
				runtime[port].image.tg_time.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.wb_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.wb.support)
			{
				runtime[port].image.wb.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.mwb_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.mwb.support)
			{
				runtime[port].image.mwb.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.wdr_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.wd.support)
			{
				runtime[port].image.wd.value = etc;
			}
			else
			{
				//notsupport = 1;
			}


			snprintf(key, 64, "cam.C%d.adaptive_ir", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.adaptive_ir.support)
			{
				runtime[port].image.adaptive_ir.value = etc;
			}
			else
			{
				//notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.defog", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.defog.support)
			{
				runtime[port].image.defog.value = etc;
			}
			else
			{
				//notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.hlc", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.hlc.support)
			{
				runtime[port].image.hlc.value = etc;
			}
			else
			{
				//notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.focus_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.focus_mode.support)
			{
				runtime[port].image.focus_mode.value = etc;
			}
			else
			{
				//notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.dnr_control", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.dnr_ctr.support)
			{
				runtime[port].image.dnr_ctr.value = etc;
			}
			else
			{
				notsupport = 1;
			}

			snprintf(key, 64, "cam.C%d.focus_dnn_comp", port);
			etc = nf_sysdb_get_bool(key);
			if(runtime[port].focus.supported & NF_IPCAM_FOCUS_DNN_COMP)
			{
				if(etc == 0 || etc == 1)
					runtime[port].focus.dnn_comp.value = etc;
			}

			snprintf(key, 64, "cam.C%d.focus_tem_comp", port);
			etc = nf_sysdb_get_bool(key);
			if(runtime[port].focus.supported & NF_IPCAM_FOCUS_TEM_COMP)
			{
				if(etc == 0 || etc == 1)
					runtime[port].focus.tem_comp.value = etc;
			}

			snprintf(key, 64, "cam.C%d.dnn_start_hour", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.dnn_schedule.start.hour = etc;

			snprintf(key, 64, "cam.C%d.dnn_start_min", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.dnn_schedule.start.min = etc;

			snprintf(key, 64, "cam.C%d.dnn_end_hour", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.dnn_schedule.end.hour = etc;

			snprintf(key, 64, "cam.C%d.dnn_end_min", port);
			etc = nf_sysdb_get_uint(key);
			runtime[port].image.dnn_schedule.end.min = etc;

			//colorvu
			snprintf(key, 64, "cam.C%d.illumination_level_ctl", port);
			etc = nf_sysdb_get_uint(key);
			if(etc & runtime[port].image.colorvu_ctrl.support)
			{
				runtime[port].image.colorvu_ctrl.value = etc;
				snprintf(key, 64, "cam.C%d.illumination_level", port);
				etc = nf_sysdb_get_uint(key);
				runtime[port].image.colorvu_level.value = etc;
			}



			if(notsupport)
			{
				/*
				 * same model nm, but different model db ( camera fw upgrade .. )
				 * -> update sysdb value
				 */

				//IPCAM_DBG(MINOR, "sysdb update (ch:%d) not support!\n", __FUNCTION__, port );

				// reuse sysdb function in nfdal.c
				IPCamSetupData data;
				guint channel = port;
				memset(&data, 0x00, sizeof(IPCamSetupData));

				data.bright = bri;
				data.contrast = con;
				data.color = col;
				data.tint = tin;
				data.sharpness = sha;

				data.focus_limit = runtime[port].image.focus_limit.value;
				data.stabilizer = runtime[port].image.stabilizer.value;
				data.ir_correction = runtime[port].image.ir_correction.value;

				data.exposure_mode = runtime[port].image.exposure.value;
				data.gain = runtime[port].image.agc.value;
				data.etime = runtime[port].image.eshutter_speed.value;
				data.dnn_sense_ntod = runtime[port].image.dnn_sense_ntod.value;
				data.dnn_sense_dton = runtime[port].image.dnn_sense_dton.value;
				data.slow_shutter = runtime[port].image.slow_shutter.value;
				data.max_agc = runtime[port].image.max_agc.value;
				data.iris_control = runtime[port].image.iris.value;
				data.blc_control = runtime[port].image.blc.value;
				data.day_night_mode = runtime[port].image.day_night.value;
				data.day_night_duration = runtime[port].image.tg_time.value;
				data.wb_mode = runtime[port].image.wb.value;
				data.mwb_mode = runtime[port].image.mwb.value;
				data.wdr_mode = runtime[port].image.wd.value;
				data.adaptive_ir = runtime[port].image.adaptive_ir.value;
				data.defog = runtime[port].image.defog.value;
				data.hlc = runtime[port].image.hlc.value;
				data.focus_mode = runtime[port].image.focus_mode.value;

				data.rotate = runtime[port].video.mirror.value;

				data.antiflicker = runtime[port].image.anti_flicker.value;
				data.max_shutter = runtime[port].image.max_shutter.value;
				data.base_shutter = runtime[port].image.base_shutter.value;

				data.dnr = runtime[port].image.dnr_ctr.value;

				data.dnn_start_hour = runtime[port].image.dnn_schedule.start.hour;
				data.dnn_start_min = runtime[port].image.dnn_schedule.start.min;
				data.dnn_end_hour = runtime[port].image.dnn_schedule.end.hour;
				data.dnn_end_min = runtime[port].image.dnn_schedule.end.min;

				DAL_set_IPCamSetup_data(data, channel);
			}
		}


		if(1)
		{	/* stream setup load */
			uint64_t sz_1st;
			unsigned int fps_1st;
			unsigned int maxbps_1st;
			unsigned int minbps_1st;
			unsigned int bitctrl_1st;
			unsigned int vcodec_1st;
			uint64_t sz_2nd;
			unsigned int fps_2nd;
			unsigned int maxbps_2nd;
			unsigned int minbps_2nd;
			unsigned int bitctrl_2nd;
			unsigned int vcodec_2nd;
			unsigned int sub_val;
			gchar *val;
			StreamData info;

			snprintf(key, 64, "cam.C%d.stream.S0.size", port);
			val = nf_sysdb_get_str_nocopy(key);
			if(val != NULL)
				sz_1st = _change_szdb_to_camgr(*val);
			else
				sz_1st = _change_szdb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S0.fps", port);
			fps_1st = nf_sysdb_get_uint(key);
			runtime[port].encoder.cur_maxfps[0] = fps_1st;

			snprintf(key, 64, "cam.C%d.stream.S0.max_bps", port);
			maxbps_1st = nf_sysdb_get_uint(key);
			if (maxbps_1st > runtime[port].encoder.max_bitrate[0] || maxbps_1st < runtime[port].encoder.min_bitrate[0])
			{
				maxbps_1st = runtime[port].encoder.max_bitrate[0];
			}

			snprintf(key, 64, "cam.C%d.stream.S0.min_bps", port);
			minbps_1st = nf_sysdb_get_uint(key);
			if (minbps_1st > runtime[port].encoder.max_bitrate[0] || minbps_1st < runtime[port].encoder.min_bitrate[0])
			{
				minbps_1st = runtime[port].encoder.min_bitrate[0];
			}

			snprintf(key, 64, "cam.C%d.stream.S0.bitctrl", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				bitctrl_1st = _change_bitctrldb_to_camgr(*val);
			else
				bitctrl_1st = _change_bitctrldb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S0.vcodec", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				vcodec_1st = _change_vcodec_db_to_camgr(val);
			else
				vcodec_1st = _change_vcodec_db_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S1.size", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				sz_2nd = _change_szdb_to_camgr(*val);
			else
				sz_2nd = _change_szdb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S1.fps", port);
			fps_2nd = nf_sysdb_get_uint(key);
			runtime[port].encoder.cur_maxfps[1] = fps_2nd;

			snprintf(key, 64, "cam.C%d.stream.S1.max_bps", port);
			maxbps_2nd = nf_sysdb_get_uint(key);
			if ((maxbps_2nd > runtime[port].encoder.max_bitrate[1] || maxbps_2nd < runtime[port].encoder.min_bitrate[1]) || (maxbps_2nd == 0))
			{
				maxbps_2nd = runtime[port].encoder.max_bitrate[1];
			}

			snprintf(key, 64, "cam.C%d.stream.S1.min_bps", port);
			minbps_2nd = nf_sysdb_get_uint(key);
			if ((minbps_2nd < runtime[port].encoder.min_bitrate[1] || minbps_2nd > runtime[port].encoder.max_bitrate[1]) || (minbps_2nd == 0))
			{
				minbps_2nd = runtime[port].encoder.min_bitrate[1];
			}

			snprintf(key, 64, "cam.C%d.stream.S1.bitctrl", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				bitctrl_2nd = _change_bitctrldb_to_camgr(*val);
			else
				bitctrl_2nd = _change_bitctrldb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S1.vcodec", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				vcodec_2nd = _change_vcodec_db_to_camgr(val);
			else
				vcodec_2nd = _change_vcodec_db_to_camgr(0);

			IPCAM_DBG(MINOR, "CH(%d) line(%d) resol(%08x,%08x) fps(%d.%d) bmax(%d,%d) bmin(%d,%d) bitctrl(%08x,%08x)\n",
					port, __LINE__,
					sz_1st, sz_2nd, fps_1st, fps_2nd, maxbps_1st, maxbps_2nd, minbps_1st, minbps_2nd, bitctrl_1st, bitctrl_2nd);
#if 0
			if ((runtime[port].func & NF_IPCAM_FUNC_VA) != 0)
			{
				int active = 0;
				sprintf(key, "cam.vca.cfg.R%u.act", port);
				active = nf_sysdb_get_bool(key);
				if (active)
				{
					runtime[port].encoder.res_support[0] &= ~NF_IPCAM_RES_1920x1080;
					runtime[port].video.resolution.supported &= ~NF_IPCAM_RES_1920x1080;
					if (runtime[port].video.resolution.resolution[0] == NF_IPCAM_RES_1920x1080)
					{
						runtime[port].video.resolution.resolution[0] = NF_IPCAM_RES_1280x720;
						runtime[port].video.resolution.supported |= NF_IPCAM_RES_1280x720;
					}
					/* SWDMVA-419 */
					if (sz_1st == NF_IPCAM_RES_1920x1080)
					{
						IPCAM_DBG(MINOR, "CH(%d) line(%d) DMVA cam forced lower br\n", port, __LINE__);
						maxbps_1st = 3000;
						minbps_1st = 1500;
						maxbps_2nd = 1000;
						minbps_2nd = 600;
					}
				}
			}
#endif
			if (runtime[port].video.resolution.resolution[0] != sz_1st)
			{
				if (runtime[port].encoder.res_support[0] & sz_1st)
				{
					IPCAM_DBG(MINOR, "CH(%d) line(%d) resol old(%08x) new(%08x)\n",
							port, __LINE__,
							runtime[port].video.resolution.resolution[0], sz_1st);
					runtime[port].video.resolution.supported &= ~(runtime[port].video.resolution.resolution[0]);
					runtime[port].video.resolution.resolution[0] = sz_1st;
					runtime[port].video.resolution.supported |= sz_1st;
				}
			}
			if (runtime[port].video.resolution.resolution[1] != sz_2nd)
			{
				if (runtime[port].encoder.res_support[1] & sz_2nd)
				{
	IPCAM_DBG(MINOR, "CH(%d) line(%d) resol old(%08x) new(%08x)\n",
			port, __LINE__,
			runtime[port].video.resolution.resolution[1], sz_2nd);
					runtime[port].video.resolution.supported &= ~(runtime[port].video.resolution.resolution[1]);
					runtime[port].video.resolution.resolution[1] = sz_2nd;
					runtime[port].video.resolution.supported |= sz_2nd;
				}
			}

			if (runtime[port].video.bitctrl[0] != bitctrl_1st)
			{
				if (runtime[port].encoder.bitctrl[0] & bitctrl_1st)
				{
					runtime[port].video.bitctrl[0] = bitctrl_1st;
				}
			}
			if (runtime[port].video.bitctrl[1] != bitctrl_2nd)
			{
				if (runtime[port].encoder.bitctrl[1] & bitctrl_2nd)
				{
					runtime[port].video.bitctrl[1] = bitctrl_2nd;
				}
			}


			if (runtime[port].video.vcodec[0] != vcodec_1st)
			{
				if (runtime[port].encoder.vcodec[0] & vcodec_1st)
				{
					runtime[port].video.vcodec[0] = vcodec_1st;
				}
			}
			if (runtime[port].video.vcodec[1] != vcodec_2nd)
			{
				if (runtime[port].encoder.vcodec[1] & vcodec_2nd)
				{
					runtime[port].video.vcodec[1] = vcodec_2nd;
				}
			}

			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER]    = maxbps_1st;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]      = minbps_1st;
			sub_val = (maxbps_1st-minbps_1st)/4;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_1st-sub_val;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH]     = maxbps_1st-(sub_val*2);
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD] = maxbps_1st-(sub_val*3);
	IPCAM_DBG(MINOR, "CH(%d) line(%d) (%d.%d.%d.%d.%d)\n", port, __LINE__,
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]);

			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER]    = maxbps_2nd;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]      = minbps_2nd;
			sub_val = (maxbps_2nd-minbps_2nd)/4;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_2nd-sub_val;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH]     = maxbps_2nd-(sub_val*2);
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD] = maxbps_2nd-(sub_val*3);
	IPCAM_DBG(MINOR, "CH(%d) line(%d) (%d.%d.%d.%d.%d)\n", port, __LINE__,
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]);

			_build_fps_table(port, fps_1st, 0);
			_build_fps_table(port, fps_2nd, 1);

			DAL_get_stream_data(&info, port);
			info.size[0][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[0]);
			info.size[1][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[1]);
			info.max_bps[0] = maxbps_1st;
			info.max_bps[1] = maxbps_2nd;
			info.min_bps[0] = minbps_1st;
			info.min_bps[1] = minbps_2nd;
			_change_cambitctrl_to_sysdb(info.control[0], runtime[port].video.bitctrl[0]);
			_change_cambitctrl_to_sysdb(info.control[1], runtime[port].video.bitctrl[1]);
			_change_cam_vcodec_to_sysdb(info.vcodec[0], runtime[port].video.vcodec[0]);
			_change_cam_vcodec_to_sysdb(info.vcodec[1], runtime[port].video.vcodec[1]);

			_prvSetStreamData(info, port);
			nf_notify_fire_params("sysdb_ipcam_change", NF_IPCAM_CATE_STREAM, (0xffffffff & (1 << port)), 0, 0);
		}
	}
	else
	{
		//IPCAM_DBG(MINOR, "sysdb overwrite (ch:%d) (model:%s->%s)\n", port, runtime[port].sys.stdver, model_nm);
		snprintf(key, 64, "cam.C%d.model_nm", port);
		nf_sysdb_set_str(key, runtime[port].sys.stdver);

		if(1)
		{	/* set image */
			IPCamSetupData data;
			guint channel = port;
			memset(&data, 0x00, sizeof(IPCamSetupData));

			/* convert runtime value(0~30) to sysdb value(0~100) */

			data.bright = runtime[port].image.brightness.value;
			data.contrast = runtime[port].image.contrast.value;
			data.color = runtime[port].image.color.value;
			data.tint = runtime[port].image.tint.value;
			data.sharpness = runtime[port].image.sharpness.value;
			
			data.focus_limit = runtime[port].image.focus_limit.value;
			data.stabilizer = runtime[port].image.stabilizer.value;
			data.ir_correction = runtime[port].image.ir_correction.value;

			data.exposure_mode = runtime[port].image.exposure.value;
			data.gain = runtime[port].image.agc.value;
			data.etime = runtime[port].image.eshutter_speed.value;
			data.dnn_sense_ntod = runtime[port].image.dnn_sense_ntod.value;
			data.dnn_sense_dton = runtime[port].image.dnn_sense_dton.value;
			data.slow_shutter = runtime[port].image.slow_shutter.value;
			data.max_agc = runtime[port].image.max_agc.value;
			data.iris_control = runtime[port].image.iris.value;
			data.blc_control = runtime[port].image.blc.value;
			data.day_night_mode = runtime[port].image.day_night.value;
			data.day_night_duration = runtime[port].image.tg_time.value;
			data.wb_mode = runtime[port].image.wb.value;
			data.mwb_mode = runtime[port].image.mwb.value;
			data.wdr_mode = runtime[port].image.wd.value;
			data.adaptive_ir = runtime[port].image.adaptive_ir.value;
			data.defog = runtime[port].image.defog.value;
			data.hlc = runtime[port].image.hlc.value;
			data.focus_mode = runtime[port].image.focus_mode.value;

			data.rotate = runtime[port].video.mirror.value;

			data.antiflicker = runtime[port].image.anti_flicker.value;
			data.max_shutter = runtime[port].image.max_shutter.value;
			data.base_shutter = runtime[port].image.base_shutter.value;
			data.dnr = runtime[port].image.dnr_ctr.value;

			data.dnn_start_hour = runtime[port].image.dnn_schedule.start.hour;
			data.dnn_start_min = runtime[port].image.dnn_schedule.start.min;
			data.dnn_end_hour = runtime[port].image.dnn_schedule.end.hour;
			data.dnn_end_min = runtime[port].image.dnn_schedule.end.min;

			DAL_set_IPCamSetup_data(data, channel);

			//colorvu
			if(runtime[port].image.supported & NF_IPCAM_IMAGE_COLORVU)
			{
				GValue _value = {0,};
				g_value_init(&_value, G_TYPE_UINT);
				g_value_set_uint(&_value, runtime[port].image.colorvu_ctrl.value);
				nf_sysdb_set_key1("cam.C%d.illumination_level_ctl", port, &_value, NULL);
				g_value_unset(&_value);

				g_value_init(&_value, G_TYPE_UINT);
				g_value_set_uint(&_value, runtime[port].image.colorvu_level.value);
				nf_sysdb_set_key1("cam.C%d.illumination_level", port, &_value, NULL);
				g_value_unset(&_value);
			}

			/* follow this : ntsc/pal sysdb */
			snprintf(key, 64, "sys.info.sig_type");
			ntp = nf_sysdb_get_bool(key);

			if (ntp == 1)
			{
				runtime[port].video.anti_flicker.value = (1<<ntp);
			}

			GValue set_value = {0,};
			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, runtime[port].focus.tem_comp.value);
			nf_sysdb_set_key1("cam.C%d.focus_tem_comp", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, runtime[port].focus.dnn_comp.value);
			nf_sysdb_set_key1("cam.C%d.focus_dnn_comp", port, &set_value, NULL);
			g_value_unset(&set_value);

		}

		if(1)
		{	/* set motion */
			OnvifMotionIPCam motiondata;
			memset(&motiondata, 0x00, sizeof(OnvifMotionIPCam));
			motiondata.col = runtime[port].motion.block_width;
			motiondata.row = runtime[port].motion.block_height;
			motiondata.method = runtime[port].motion.method;
			motiondata.sense_min = runtime[port].motion.sensitivity.min;
			motiondata.sense_max = runtime[port].motion.sensitivity.max;

			DAL_set_onvif_ipcam_data(motiondata, port);

			GValue set_value = {0,};
			guchar buf[1401];

			memset(buf, '1', 1400);
			buf[1400] = '\0';

			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			nf_sysdb_set_key1("alarm.motion.M%d.area", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, 0);
			nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.col1", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, 0);
			nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.row1", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.block_width - 1);
			nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.col2", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.block_height - 1);
			nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.row2", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.sensitivity.value);
			nf_sysdb_set_key1("alarm.motion.M%d.sense_d", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.sensitivity.value);
			nf_sysdb_set_key1("alarm.motion.M%d.sense_n", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.sensitivity.value);
			nf_sysdb_set_key1("alarm.motion.M%d.sense", port, &set_value, NULL);
			g_value_unset(&set_value);

			//DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);
		}

		if(1)
		{	/* set smart motion */
			if(runtime[port].motion.smart_motion_support)
			{
				GValue set_value = {0,};

				//smart motion enable
				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, (gint)runtime[port].motion.smart_motion_enable);
				nf_sysdb_set_key1("alarm.motion.M%d.smart_motion", port, &set_value, NULL);
				g_value_unset(&set_value);

				//ai alarm event
				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, (gint)runtime[port].motion.ai_alarm_event);
				nf_sysdb_set_key1("alarm.motion.M%d.use_ai_alarmevt", port, &set_value, NULL);
				g_value_unset(&set_value);
			}
		}

		if(1)
		{	/* set roi */
			nf_sysdb_lock(NF_SYSDB_CATE_CAM);
			printf("\e[31m >>>>>>>>>>>>>>>>> new camera init roi <<<<<<<<<<<<<<<< \e[0m\n");
			GValue set_value = {0,};
			int i = 0;

			g_value_init(&set_value, G_TYPE_INT);
			g_value_set_int(&set_value, (gint)0);
			nf_sysdb_set_key1("cam.ROI.C%d.mode", port, &set_value, NULL);
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_INT);
			g_value_set_int(&set_value, (gint)0);
			nf_sysdb_set_key1("cam.ROI.C%d.quality", port, &set_value, NULL);
			g_value_unset(&set_value);

			for(i = 0; i < runtime[port].roi_area.max_rect; i++)
			{
				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)0);
				nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.level", port, i, &set_value, NULL);
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)0);
				nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.tx", port, i, &set_value, NULL);
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)0);
				nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.ty", port, i, &set_value, NULL);
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)0);
				nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.bx", port, i, &set_value, NULL);
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)0);
				nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.by", port, i, &set_value, NULL);
				g_value_unset(&set_value);
			}
			nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
		}

		if(1)
		{	/* set stream setup */
			StreamData info;
			unsigned int sub_val = 0;

			DAL_get_stream_data(&info, port);
			info.size[0][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[0]);
			info.size[1][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[1]);
			info.max_fps[0] = runtime[port].encoder.cur_maxfps[0];
			info.max_fps[1] = runtime[port].encoder.cur_maxfps[1];

			_build_fps_table(port, info.max_fps[0], 0);
			_build_fps_table(port, info.max_fps[1], 1);
			
			//ITX 4M resol defualt camera DB switching
			if(strncmp(runtime[port].sys.swver, "48100", 5) == 0	//3516D Chipset
					&& strstr(runtime[port].sys.stdver, "-4007") != NULL)
			{
				int sdkver[4] = {0, };
				int is_pal = nf_sysdb_get_bool("sys.info.sig_type");
				sscanf(runtime[port].sys.sdkver, "%d.%d.%d.%d", &sdkver[0], &sdkver[1], &sdkver[2], &sdkver[3]);
				
				if(sdkver[1] == ITX_CAM_SDK_TYPE_HS && sdkver[3] > 8)
					info.max_fps[0] = 25;
				else
					info.max_fps[0] = 20;

				printf("[%s](CH:%d) 4M 3516D -> switching max fps\n", __FUNCTION__, port);
			}

			info.max_bps[1] = runtime[port].encoder.max_bitrate[1];
			info.min_bps[0] = runtime[port].encoder.min_bitrate[0];
			info.min_bps[1] = runtime[port].encoder.min_bitrate[1];
			_change_cambitctrl_to_sysdb(info.control[0], runtime[port].video.bitctrl[0]);
			_change_cambitctrl_to_sysdb(info.control[1], runtime[port].video.bitctrl[1]);
			_change_cam_vcodec_to_sysdb(info.vcodec[0], runtime[port].video.vcodec[0]);
			_change_cam_vcodec_to_sysdb(info.vcodec[1], runtime[port].video.vcodec[1]);

			info.max_bps[0] = runtime[port].encoder.max_bitrate[0] >= 4000 ? 4000:runtime[port].encoder.max_bitrate[0];
			if(nf_ipcam_is_vendor_videcon())
			{
				info.max_bps[1] = runtime[port].encoder.max_bitrate[1] >= 1000 ? 1000:runtime[port].encoder.max_bitrate[1];
			}

			if(nf_ipcam_is_vendor_zicom()) // SWPFOURCE-1180
			{
				info.max_bps[0] = runtime[port].video.bitrate[0].value;
				info.max_bps[1] = runtime[port].encoder.min_bitrate[1];
			}
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER] = info.max_bps[0];
			sub_val = (info.max_bps[0]-info.min_bps[0])/4;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW] = info.min_bps[0];
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST]  = info.max_bps[0]-sub_val;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH]     = info.max_bps[0]-(sub_val*2);
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD] = info.max_bps[0]-(sub_val*3);

			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW] = info.min_bps[1];
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER] = info.max_bps[1];
			sub_val = (info.max_bps[1]-info.min_bps[1])/4;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST]  = info.max_bps[1]-sub_val;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH]     = info.max_bps[1]-(sub_val*2);
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD] = info.max_bps[1]-(sub_val*3);

			_prvSetStreamData(info, port);
			nf_notify_fire_params("sysdb_ipcam_change", NF_IPCAM_CATE_STREAM, (0xffffffff & (1 << port)), 0, 0);
		}
	}

	snprintf(key, 64, "cam.C%d.iris", port);
	etc = nf_sysdb_get_int(key);

	runtime[port].ptz.iris.value = etc;

	snprintf(key, 64, "alarm.sensor.S%d.op_type", port);
	etc = nf_sysdb_get_bool(key);
	runtime[port].alarm.alarm_in_type.value = 1<<etc;

	sysdb_load_preset(port);

	g_free(model_nm);
}

/**
 * @brief 최초 접속시 ONVIF카메라의 runtime정보와 sysdb를 동기화한다.
 * @param port 채널 번호.
 *
 * 카메라의 모델명을 sysdb와 비교하여 신규 모델일시 runtime->sysdb 동기화, 동일 모델일시 sysdb->runtime 동기화 실시.
 */
static void sysdb_load_onvif(int port)
{
	unsigned int value, rot, etc, ntp;
	char key[64];
	gchar *model_nm;
	mtable *runtime = get_runtime();

	snprintf(key, 64, "cam.C%d.model_nm", port);
	model_nm = nf_sysdb_get_str(key);

	if(strcmp(model_nm, runtime[port].sys.stdver) == 0)
	{
		//IPCAM_DBG(MINOR, "sysdb load (ch:%d) (model:%s->%s)\n", port, runtime[port].sys.stdver, model_nm);
		{
			uint64_t sz_1st;
			unsigned int fps_1st;
			unsigned int maxbps_1st;
			unsigned int minbps_1st;
			unsigned int bitctrl_1st;
			unsigned int vcodec_1st;
			uint64_t sz_2nd;
			unsigned int fps_2nd;
			unsigned int maxbps_2nd;
			unsigned int minbps_2nd;
			unsigned int bitctrl_2nd;
			unsigned int vcodec_2nd;
			unsigned int sub_val;
			gchar *val;

			snprintf(key, 64, "cam.C%d.stream.S0.size", port);
			val = nf_sysdb_get_str_nocopy(key);
			if(val != NULL)
				sz_1st = _change_szdb_to_camgr(*val);
			else
				sz_1st = _change_szdb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S0.fps", port);
			fps_1st = nf_sysdb_get_uint(key);
			if( fps_1st > runtime[port].encoder.max_framerate[0] && runtime[port].encoder.max_framerate[0] > 0 )
			{
				fps_1st = runtime[port].encoder.max_framerate[0];			
			}
			runtime[port].encoder.cur_maxfps[0] = fps_1st;

			snprintf(key, 64, "cam.C%d.stream.S0.max_bps", port);
			maxbps_1st = nf_sysdb_get_uint(key);
			if ((maxbps_1st > runtime[port].encoder.max_bitrate[0] && runtime[port].encoder.max_bitrate[0] > 0) || maxbps_1st < runtime[port].encoder.min_bitrate[0])
			{
				maxbps_1st = runtime[port].encoder.max_bitrate[0];
			}

			snprintf(key, 64, "cam.C%d.stream.S0.min_bps", port);
			minbps_1st = nf_sysdb_get_uint(key);
			if ((minbps_1st > runtime[port].encoder.max_bitrate[0] && runtime[port].encoder.max_bitrate[0] > 0)  || minbps_1st < runtime[port].encoder.min_bitrate[0])
			{
				minbps_1st = runtime[port].encoder.min_bitrate[0];
			}

			if(runtime[port].encoder.max_bitrate[0] == 0 && runtime[port].encoder.min_bitrate[0] == 0)
			{
				maxbps_1st = 0;
				minbps_1st = 0;
			}

			snprintf(key, 64, "cam.C%d.stream.S0.bitctrl", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				bitctrl_1st = _change_bitctrldb_to_camgr(*val);
			else
				bitctrl_1st = _change_bitctrldb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S0.vcodec", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				vcodec_1st = _change_vcodec_db_to_camgr(val);
			else
				vcodec_1st = _change_vcodec_db_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S1.size", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				sz_2nd = _change_szdb_to_camgr(*val);
			else
				sz_2nd = _change_szdb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S1.fps", port);
			fps_2nd = nf_sysdb_get_uint(key);
			if( fps_2nd  > runtime[port].encoder.max_framerate[1] && runtime[port].encoder.max_framerate[1] > 0 )
			{
				fps_2nd = runtime[port].encoder.max_framerate[1];			
			}
			runtime[port].encoder.cur_maxfps[1] = fps_2nd;

			snprintf(key, 64, "cam.C%d.stream.S1.max_bps", port);
			maxbps_2nd = nf_sysdb_get_uint(key);
			if (((maxbps_2nd > runtime[port].encoder.max_bitrate[1] && runtime[port].encoder.max_bitrate[1] > 0)|| maxbps_2nd < runtime[port].encoder.min_bitrate[1]) || (maxbps_2nd == 0))
			{
				maxbps_2nd = runtime[port].encoder.max_bitrate[1];
			}

			snprintf(key, 64, "cam.C%d.stream.S1.min_bps", port);
			minbps_2nd = nf_sysdb_get_uint(key);
			if ((minbps_2nd < runtime[port].encoder.min_bitrate[1] || (minbps_2nd > runtime[port].encoder.max_bitrate[1] && runtime[port].encoder.max_bitrate[1] > 0)) || (minbps_2nd == 0))
			{
				minbps_2nd = runtime[port].encoder.min_bitrate[1];
			}

			if(runtime[port].encoder.max_bitrate[1] == 0 && runtime[port].encoder.min_bitrate[1] == 0)
			{
				maxbps_2nd = 0;
				minbps_2nd = 0;
			}

			snprintf(key, 64, "cam.C%d.stream.S1.bitctrl", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				bitctrl_2nd = _change_bitctrldb_to_camgr(*val);
			else
				bitctrl_2nd = _change_bitctrldb_to_camgr(0);
			bitctrl_2nd = _change_bitctrldb_to_camgr(*val);

			snprintf(key, 64, "cam.C%d.stream.S1.vcodec", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				vcodec_2nd = _change_vcodec_db_to_camgr(val);
			else
				vcodec_2nd = _change_vcodec_db_to_camgr(0);


			IPCAM_DBG(MINOR, "CH(%d) line(%d) resol(%08x,%08x) fps(%d.%d) bmax(%d,%d) bmin(%d,%d)\n",
			port, __LINE__,
			sz_1st, sz_2nd, fps_1st, fps_2nd, maxbps_1st, maxbps_2nd, minbps_1st, minbps_2nd);

			if (runtime[port].video.resolution.resolution[0] != sz_1st)
			{
				if (runtime[port].encoder.res_support[0] & sz_1st)
				{
					IPCAM_DBG(MINOR, "CH(%d) line(%d) resol old(%08x) new(%08x)\n",
							port, __LINE__,
							runtime[port].video.resolution.resolution[0], sz_1st);
					runtime[port].video.resolution.supported &= ~(runtime[port].video.resolution.resolution[0]);
					runtime[port].video.resolution.resolution[0] = sz_1st;
					runtime[port].video.resolution.supported |= sz_1st;
				}
			}
			if (runtime[port].video.resolution.resolution[1] != sz_2nd)
			{
				if (runtime[port].encoder.res_support[1] & sz_2nd)
				{
					IPCAM_DBG(MINOR, "CH(%d) line(%d) resol old(%08x) new(%08x)\n",
					port, __LINE__,
					runtime[port].video.resolution.resolution[1], sz_2nd);
					runtime[port].video.resolution.supported &= ~(runtime[port].video.resolution.resolution[1]);
					runtime[port].video.resolution.resolution[1] = sz_2nd;
					runtime[port].video.resolution.supported |= sz_2nd;
				}
			}

			if (runtime[port].video.bitctrl[0] != bitctrl_1st)
			{
				if (runtime[port].encoder.bitctrl[0] & bitctrl_1st)
				{
					runtime[port].video.bitctrl[0] = bitctrl_1st;
				}
			}
			if (runtime[port].video.bitctrl[1] != bitctrl_2nd)
			{
				if (runtime[port].encoder.bitctrl[1] & bitctrl_2nd)
				{
					runtime[port].video.bitctrl[1] = bitctrl_2nd;
				}
			}

			if (runtime[port].video.vcodec[0] != vcodec_1st)
			{
				if (runtime[port].encoder.vcodec[0] & vcodec_1st)
				{
					runtime[port].video.vcodec[0] = vcodec_1st;
				}
			}
			if (runtime[port].video.vcodec[1] != vcodec_2nd)
			{
				if (runtime[port].encoder.vcodec[1] & vcodec_2nd)
				{
					runtime[port].video.vcodec[1] = vcodec_2nd;
				}
			}

			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER]    = maxbps_1st;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]      = minbps_1st;
			sub_val = (maxbps_1st-minbps_1st)/4;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_1st-sub_val;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH]     = maxbps_1st-(sub_val*2);
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD] = maxbps_1st-(sub_val*3);
			IPCAM_DBG(MINOR, "CH(%d) line(%d) (%d.%d.%d.%d.%d)\n", port, __LINE__,
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD],
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]);

			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER]    = maxbps_2nd;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]      = minbps_2nd;
			sub_val = (maxbps_2nd-minbps_2nd)/4;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_2nd-sub_val;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH]     = maxbps_2nd-(sub_val*2);
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD] = maxbps_2nd-(sub_val*3);
			IPCAM_DBG(MINOR, "CH(%d) line(%d) (%d.%d.%d.%d.%d)\n", port, __LINE__,
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD],
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]);

			_build_fps_table(port, fps_1st, 0);
			_build_fps_table(port, fps_2nd, 1);


		}

		snprintf(key, 64, "cam.C%d.bright", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
		{
			runtime[port].image_onvif.brightness.value = value;
		}

		snprintf(key, 64, "cam.C%d.color", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
		{
			runtime[port].image_onvif.color.value = value;
		}

		snprintf(key, 64, "cam.C%d.contrast", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
		{
			runtime[port].image_onvif.contrast.value = value;
		}

		snprintf(key, 64, "cam.C%d.sharpness", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
		{
			runtime[port].image_onvif.sharpness.value = value;
		}

		snprintf(key, 64, "cam.C%d.rotate", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_ROTATION)
		{
			runtime[port].video.mirror.value = value;
		}

		snprintf(key, 64, "cam.C%d.focus_mode", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE)
		&& (runtime[port].image_onvif.focus.mode.support & value))
		{
			runtime[port].image_onvif.focus.mode.value = value;
		}

		snprintf(key, 64, "cam.C%d.focus_default_speed", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
		{
			runtime[port].image_onvif.focus.defaultspeed.value = value;
		}

		snprintf(key, 64, "cam.C%d.focus_near_limit", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
		{
			runtime[port].image_onvif.focus.nearlimit.value = value;
		}

		snprintf(key, 64, "cam.C%d.focus_far_limit", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
		{
			runtime[port].image_onvif.focus.farlimit.value = value;
		}
		//todo distance&speed

		snprintf(key, 64, "cam.C%d.wb_mode", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_MODE)
		&& (runtime[port].image_onvif.wb.mode.support & value))
		{
			runtime[port].image_onvif.wb.mode.value = value;
		}

		snprintf(key, 64, "cam.C%d.wb_crgain", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
		{
			runtime[port].image_onvif.wb.crgain.value = value;
		}

		snprintf(key, 64, "cam.C%d.wb_cbgain", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
		{
			runtime[port].image_onvif.wb.cbgain.value = value;
		}

		snprintf(key, 64, "cam.C%d.focus_limit", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT)
		{
			runtime[port].image_onvif.focus_limit.value = value;
		}

		snprintf(key, 64, "cam.C%d.stabilizer", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_IMAGE_ONVIF_STABILIZER)
		{
			runtime[port].image_onvif.stabilizer.value = value;
		}

		snprintf(key, 64, "cam.C%d.ir_correction", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION)
		{
			runtime[port].image_onvif.ir_correction.value = value;
		}

		snprintf(key, 64, "cam.C%d.wdr_mode", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE)
		&& (runtime[port].image_onvif.wdrmode.support & value))
		{
			runtime[port].image_onvif.wdrmode.value = value;
		}

		snprintf(key, 64, "cam.C%d.wdr_level", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
		{
			runtime[port].image_onvif.wdrlevel.value = value;
		}

		snprintf(key, 64, "cam.C%d.adaptive_ir", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR)
		{
			runtime[port].image_onvif.adaptive_ir.value = value;
		}

		snprintf(key, 64, "cam.C%d.defog", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DEFOG)
		{
			runtime[port].image_onvif.defog.value = value;
		}

		snprintf(key, 64, "cam.C%d.hlc", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_HLC)
		{
			runtime[port].image_onvif.hlc.value = value;
		}

		snprintf(key, 64, "cam.C%d.day_night_mode", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
		&& (runtime[port].image_onvif.ircut.support & value))
		{
			runtime[port].image_onvif.ircut.value = value;
		}

		snprintf(key, 64, "cam.C%d.exposure_mode", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE)
		&& (runtime[port].image_onvif.exposure.mode.support & value))
		{
			runtime[port].image_onvif.exposure.mode.value = value;
		}

		snprintf(key, 64, "cam.C%d.exposure_priority", port);
		value = nf_sysdb_get_uint(key);
		if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
		&& (runtime[port].image_onvif.exposure.priority.support & value))
		{
			runtime[port].image_onvif.exposure.priority.value = value;
		}

		snprintf(key, 64, "cam.C%d.min_etime", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
		{
			runtime[port].image_onvif.exposure.minetime.value = value;
		}

		snprintf(key, 64, "cam.C%d.max_etime", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
		{
			runtime[port].image_onvif.exposure.maxetime.value = value;
		}

		snprintf(key, 64, "cam.C%d.etime", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
		{
			runtime[port].image_onvif.exposure.etime.value = value;
		}

		snprintf(key, 64, "cam.C%d.dnn_ntod", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
		{
			runtime[port].image_onvif.dnn_sense_ntod.value = value;
		}

		snprintf(key, 64, "cam.C%d.dnn_dton", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON)
		{
			runtime[port].image_onvif.dnn_sense_dton.value = value;
		}

		snprintf(key, 64, "cam.C%d.min_gain", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
		{
			runtime[port].image_onvif.exposure.mingain.value = value;
		}

		snprintf(key, 64, "cam.C%d.max_gain", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
		{
			runtime[port].image_onvif.exposure.maxgain.value = value;
		}

		snprintf(key, 64, "cam.C%d.gain", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
		{
			runtime[port].image_onvif.exposure.gain.value = value;
		}

		snprintf(key, 64, "cam.C%d.min_iris", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
		{
			runtime[port].image_onvif.exposure.miniris.value = value;
		}

		snprintf(key, 64, "cam.C%d.max_iris", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
		{
			runtime[port].image_onvif.exposure.maxiris.value = value;
		}

		snprintf(key, 64, "cam.C%d.iris", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
		{
			runtime[port].image_onvif.exposure.iris.value = value;
		}

		snprintf(key, 64, "cam.C%d.iris_control", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS)
		{
			runtime[port].image_onvif.exposure.iris_mode.value = value;
		}

		snprintf(key, 64, "cam.C%d.blc_control", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE)
		{
			runtime[port].image_onvif.blcmode.value = value;
		}

		snprintf(key, 64, "cam.C%d.blc_level", port);
		value = nf_sysdb_get_int(key);
		if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
		{
			runtime[port].image_onvif.blclevel.value = value;
		}

#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		snprintf(key, 64, "cam.fisheye.f%d.mount_type", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].fisheye.fisheye_supported & NF_IPCAM_FISHEYE_MOUNT)
		{
			runtime[port].fisheye.mount.value = value;
		}

		snprintf(key, 64, "cam.fisheye.f%d.dewarp_mode", port);
		value = nf_sysdb_get_uint(key);
		if(runtime[port].fisheye.fisheye_supported & NF_IPCAM_FISHEYE_DEWARP)
		{
			runtime[port].fisheye.dewarp.value = value;
		}
#endif
	}
	else
	{
		//IPCAM_DBG(MINOR, "sysdb overwrite (ch:%d) (model:%s->%s)\n", port, runtime[port].sys.model, model_nm);
		snprintf(key, 64, "cam.C%d.model_nm", port);
		nf_sysdb_set_str(key, runtime[port].sys.stdver);

		// sysdb update
		if(runtime[port].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE))
		{

			// reuse sysdb function in nfdal.c
			IPCamSetupData data;
			guint channel = port;
			memset(&data, 0x00, sizeof(IPCamSetupData));

			data.bright = runtime[port].image_onvif.brightness.value;
			data.contrast = runtime[port].image_onvif.contrast.value;
			data.color = runtime[port].image_onvif.color.value;
			data.sharpness = runtime[port].image_onvif.sharpness.value;

			data.focus_mode = runtime[port].image_onvif.focus.mode.value;
			data.focus_default_speed = runtime[port].image_onvif.focus.defaultspeed.value;
			data.focus_near_limit = runtime[port].image_onvif.focus.nearlimit.value;
			data.focus_far_limit = runtime[port].image_onvif.focus.farlimit.value;

			data.wb_mode = runtime[port].image_onvif.wb.mode.value;
			data.wb_crgain = runtime[port].image_onvif.wb.crgain.value;
			data.wb_cbgain = runtime[port].image_onvif.wb.cbgain.value;
			data.wdr_mode = runtime[port].image_onvif.wdrmode.value;
			data.wdr_level = runtime[port].image_onvif.wdrlevel.value;
			data.day_night_mode = runtime[port].image_onvif.ircut.value;

			data.exposure_mode = runtime[port].image_onvif.exposure.mode.value;
			data.exposure_priority = runtime[port].image_onvif.exposure.priority.value;
			data.min_etime = runtime[port].image_onvif.exposure.minetime.value;
			data.max_etime = runtime[port].image_onvif.exposure.maxetime.value;
			data.etime = runtime[port].image_onvif.exposure.etime.value;
			data.min_gain = runtime[port].image_onvif.exposure.mingain.value;
			data.max_gain = runtime[port].image_onvif.exposure.maxgain.value;
			data.gain = runtime[port].image_onvif.exposure.gain.value;
			data.min_iris = runtime[port].image_onvif.exposure.miniris.value;
			data.max_iris = runtime[port].image_onvif.exposure.maxiris.value;
			data.iris = runtime[port].image_onvif.exposure.iris.value;
			data.iris_control = runtime[port].image_onvif.exposure.iris_mode.value;
			data.rotate = runtime[port].video.mirror.value;

			data.blc_control = runtime[port].image_onvif.blcmode.value;
			data.blc_level = runtime[port].image_onvif.blclevel.value;

			DAL_set_IPCamSetup_data(data, channel);
		}

		if(runtime[port].motion.method != MAM_NONE)
		{
			nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

			GValue set_value = {0,};

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.sensitivity.value);
			if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_d", port, &set_value, NULL))
			{
				g_value_unset(&set_value);
			}
			if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_n", port, &set_value, NULL))
			{
				g_value_unset(&set_value);
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.block_width - 1);
			if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.col2", port, &set_value, NULL))
			{
				g_value_unset(&set_value);
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, runtime[port].motion.block_height - 1);
			if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.R0.row2", port, &set_value, NULL))
			{
				g_value_unset(&set_value);
			}
			g_value_unset(&set_value);

			nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
			//DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);
		}

		OnvifMotionIPCam motiondata;
		memset(&motiondata, 0x00, sizeof(OnvifMotionIPCam));
		motiondata.col = runtime[port].motion.block_width;
		motiondata.row = runtime[port].motion.block_height;
		motiondata.method = runtime[port].motion.method;
		motiondata.sense_min = runtime[port].motion.sensitivity.min;
		motiondata.sense_max = runtime[port].motion.sensitivity.max;

		DAL_set_onvif_ipcam_data(motiondata, port);

		GValue set_value = {0,};
		guchar buf[1401];

		memset(buf, '1', 1400);
		buf[1400] = '\0';

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("alarm.motion.M%d.area", port, &set_value, NULL))
		{
			g_value_unset(&set_value);
		}
		g_value_unset(&set_value);

		if(1)
		{	/* set stream setup */
			int maxbps_1st, minbps_1st, maxbps_2nd, minbps_2nd, sub_val, fps_1st, fps_2nd;
			int bitctrl_1st, bitctrl_2nd;
			int vcodec_1st, vcodec_2nd;
			maxbps_1st = (runtime[port].encoder.max_bitrate[0] >= 4000) ? 4000:runtime[port].encoder.max_bitrate[0];
			maxbps_2nd = runtime[port].encoder.max_bitrate[1];
			minbps_1st = runtime[port].encoder.min_bitrate[0];
			minbps_2nd = runtime[port].encoder.min_bitrate[1];

			if (nf_ipcam_is_vendor_videcon())
			{
				minbps_2nd = (runtime[port].encoder.min_bitrate[1] >= 1000) ? 1000:runtime[port].encoder.min_bitrate[1];
			}
			else
			{
				minbps_2nd = (runtime[port].encoder.min_bitrate[1] > 800) ? runtime[port].encoder.min_bitrate[1]:800;
			}

			fps_1st = runtime[port].encoder.cur_maxfps[0];
			fps_2nd = runtime[port].encoder.cur_maxfps[1];
			bitctrl_1st = runtime[port].video.bitctrl[0];
			bitctrl_2nd = runtime[port].video.bitctrl[1];
			vcodec_1st = runtime[port].video.vcodec[0];
			vcodec_2nd = runtime[port].video.vcodec[1];

			//IPCAM_DBG(MINOR, "CH(%d) line(%d) bmax(%d,%d) bmin(%d,%d)\n",
			//		port, __LINE__,
			//		maxbps_1st, maxbps_2nd, minbps_1st, minbps_2nd);

			runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER]    = maxbps_1st;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]      = minbps_1st;
			sub_val = (maxbps_1st-minbps_1st)/4;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_1st-sub_val;
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_HIGH]     = maxbps_1st-(sub_val*2);
			runtime[port].video.quality[0][NF_IPCAM_QUALITY_STANDARD] = maxbps_1st-(sub_val*3);

			runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER]    = maxbps_2nd;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]      = minbps_2nd;
			sub_val = (maxbps_2nd-minbps_2nd)/4;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGHEST]  = maxbps_2nd-sub_val;
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_HIGH]     = maxbps_2nd-(sub_val*2);
			runtime[port].video.quality[1][NF_IPCAM_QUALITY_STANDARD] = maxbps_2nd-(sub_val*3);

			_build_fps_table(port, fps_1st, 0);
			_build_fps_table(port, fps_2nd, 1);

			StreamData info;

			DAL_get_stream_data(&info, port);
			info.size[0][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[0]);
			info.size[1][0] = _change_camgr_to_sysdb(runtime[port].video.resolution.resolution[1]);
			info.max_fps[0] = fps_1st;
			info.max_fps[1] = fps_2nd;
			info.max_bps[0] = maxbps_1st;
			info.max_bps[1] = maxbps_2nd;
			info.min_bps[0] = minbps_1st;
			info.min_bps[1] = minbps_2nd;
			_change_cambitctrl_to_sysdb(info.control[0], bitctrl_1st);
			_change_cambitctrl_to_sysdb(info.control[1], bitctrl_2nd);
			_change_cam_vcodec_to_sysdb(info.vcodec[0], vcodec_1st);
			_change_cam_vcodec_to_sysdb(info.vcodec[1], vcodec_2nd);

			_prvSetStreamData(info, port);
			nf_notify_fire_params("sysdb_ipcam_change", NF_IPCAM_CATE_STREAM, (0xffffffff & (1 << port)), 0, 0);
		}

#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, runtime[port].fisheye.mount.value);
		nf_sysdb_set_key1("cam.fisheye.f%d.mount_type", port, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, runtime[port].fisheye.dewarp.value);
		nf_sysdb_set_key1("cam.fisheye.f%d.dewarp_mode", port, &set_value, NULL);
		g_value_unset(&set_value);
#endif
	}

	snprintf(key, 64, "cam.C%d.rotate", port);
	rot = nf_sysdb_get_uint(key);
	snprintf(key, 64, "sys.info.sig_type", port);
	ntp = nf_sysdb_get_bool(key);

	if (rot & runtime[port].video.mirror.support)
	{
		runtime[port].image_onvif.mirror.value = rot;
		runtime[port].video.mirror.value = rot;
	}
	//if (ntp == 1)
	{
		runtime[port].video.anti_flicker.value = (1<<ntp);
	}

	snprintf(key, 64, "alarm.sensor.S%d.op_type", port);
	etc = nf_sysdb_get_bool(key);
	runtime[port].alarm.alarm_in_type.value = 1<<etc;

	g_free(model_nm);

	//sysdb_load_preset(port);
}

/**
 * @brief 최초 접속시 sysdb의 PTZ preset정보를 불러온다.
 * @param port
 */
static void sysdb_load_preset(int port)
{
	unsigned int value, i;
	char key[64];
	gchar *temp;
	mtable *runtime = get_runtime();

	snprintf(key, 64, "cam.ptz.P%d.preset.PCNT", port);
	value = nf_sysdb_get_uint(key);
	runtime[port].preset.preset_cnt = value;

	for(i = 0; i < runtime[port].preset.preset_cnt; i++)
	{
		snprintf(key, 64, "cam.ptz.P%d.preset.P%d.number", port, i);
		value = nf_sysdb_get_uint(key);
		runtime[port].preset.preset_number[i] = value;

		snprintf(key, 64, "cam.ptz.P%d.preset.P%d.name", port, i);
		temp = nf_sysdb_get_str(key);
		snprintf(runtime[port].preset.preset_name[i], 64, "%s", temp);
		g_free(temp);
		/*
		snprintf(key, 64, "cam.ptz.P%d.preset.P%d.token", port, i);
		temp = nf_sysdb_get_str(key);
		snprintf(runtime[port].preset.preset_token[i], 64, "%s", temp);
		g_free(temp);
		*/
	}
}

/**
 * @brief Open모드에서 ITX카메라 연결을 준비한다.
 * @param port 채널 번호.
 *
 * 모델 db로 처리할 수 없는 것들(현재 렌즈 설정 등)을 조회 및 처리한다.
 */
extern void nf_openmode_cam_prepare(int port)
{
	int rtn = 0;
	int result = 0;
	mtable *runtime = get_runtime();
	int (*func0)(int*,int);
	int val = 0;

	runtime[port].state = MGMT_STATE_LINKED|MGMT_STATE_READY|OPENMODE_STATE_CONFIGURING;
	//sysdb_load(port); //(SWIPXXP-689)

	/* Auto focus capability */
	//IPCAM_DBG(MAJOR, "Try to get the auto focus capability(CH(%d))\n", port);
	{
		NFIPCamAFCapa info;

		memset(&info, 0x00, sizeof(NFIPCamAFCapa));
		rtn = nf_ipcam_get_af_capability(port, &info, NULL);
		if (rtn == IPCAM_SETUP_RTN_DONE)
		{
			if (info.mfz == 0)
			{
				//IPCAM_DBG(MINOR, "No AF command supported(CH(%d))\n", port);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
			}
			else if (info.mfz == 1)
			{
				//IPCAM_DBG(MINOR, "AF command enable(CH(%d))\n", port);
				runtime[port].image.supported |= (NF_IPCAM_IMAGE_CALIBRATION | NF_IPCAM_IMAGE_ZOOM | NF_IPCAM_IMAGE_FOCUS | NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].ptz.zoom.min = info.zoom_min;
				runtime[port].ptz.zoom.max = info.zoom_max;
				runtime[port].ptz.focus.min = info.focus_min;
				runtime[port].ptz.focus.max = info.focus_max;

                //SWIPXMFOUR-2612   Focus Compensation
                {
                    runtime[port].focus.dnn_comp.value = 1;

                    runtime[port].focus.supported |= NF_IPCAM_FOCUS_DNN_COMP;
                    runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS_COMP] = cam_set_focus_comp;
                    focus_comp_info fcinfo;
                    memset(&fcinfo, 0x00, sizeof(fcinfo));
                    if(cam_get_focus_comp(&fcinfo, port) == IPCAM_SETUP_RTN_DONE)
                    {
                        runtime[port].focus.maskarea[0].value = fcinfo.maskarea[0];
                        runtime[port].focus.areatx[0].value = fcinfo.areatx[0];
                        runtime[port].focus.areaty[0].value = fcinfo.areaty[0];
                        runtime[port].focus.areabx[0].value = fcinfo.areabx[0];
                        runtime[port].focus.areaby[0].value = fcinfo.areaby[0];
                    }
                }
			}

			if (info.iris_mode == 1)
			{
				runtime[port].image.supported |= NF_IPCAM_IMAGE_PIRIS;
				runtime[port].ptz.iris.min = info.iris_min;
				runtime[port].ptz.iris.max = info.iris_max;

				if(runtime[port].sys.model_code == NF_IPCAM_MODEL_TI_368)
				{
					if(strncmp(runtime[port].sys.stdver, "IN", 2) != 0)
					{
						if(strcmp(runtime[port].sys.sdkver, "3.0.0.4") >= 0)
						{
							runtime[port].image.iris.support = 
								(NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL | NF_IPCAM_IMAGE_PIRIS_BQ | NF_IPCAM_IMAGE_PIRIS_DF);
						}
						else
						{
							runtime[port].image.iris.support =
								NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL ;
						}

						if(strstr(runtime[port].sys.stdver,"-0303P") != NULL)
						{
							runtime[port].image.iris.support = NF_IPCAM_IMAGE_DCIRIS_OFF | NF_IPCAM_IMAGE_DCIRIS_ON;
							runtime[port].image.iris.value = NF_IPCAM_IMAGE_DCIRIS_OFF;
						}

						{
							char key[64];
							unsigned int etc;
							snprintf(key, 64, "cam.C%d.iris_control", port);
							etc = nf_sysdb_get_uint(key);
							if ((etc & runtime[port].image.iris.support) != 0)
							{
								runtime[port].image.iris.value = etc;
							}
							else
							{
								runtime[port].image.iris.value = NF_IPCAM_IMAGE_PIRIS_AUTO;
							}
						}
					}
				}
				else
				{
					runtime[port].image.iris.support =
						NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL |
						NF_IPCAM_IMAGE_PIRIS_BQ | NF_IPCAM_IMAGE_PIRIS_DF |
						NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING | NF_IPCAM_IMAGE_PIRIS_DF_TRACKING;
					{
						char key[64];
						unsigned int etc;
						snprintf(key, 64, "cam.C%d.iris_control", port);
						etc = nf_sysdb_get_uint(key);
						if ((etc & runtime[port].image.iris.support) != 0)
						{
							runtime[port].image.iris.value = etc;
						}
						else
						{
							runtime[port].image.iris.value = NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING;
						}
					}
				}

				
				if(runtime[port].sys.model_code == NF_IPCAM_MODEL_AMB_A2 ||
					runtime[port].sys.model_code == NF_IPCAM_MODEL_AMB_D1)
				{
					runtime[port].image.exposure.support =
						NF_IPCAM_IMAGE_EXP_MODE_MANUAL |
						NF_IPCAM_IMAGE_EXP_MODE_AUTO;
				}
					
				if ((runtime[port].image.exposure.support & runtime[port].image.exposure.value) == 0)
				{
					runtime[port].image.exposure.value = NF_IPCAM_IMAGE_EXP_MODE_AUTO;
				}
			}
			else
			{
				if(strcmp(runtime[port].sys.stdver, "NMB-2003PR") == 0 ||
				strcmp(runtime[port].sys.stdver, "NMX-2003P") == 0 ||
				strcmp(runtime[port].sys.stdver, "NMD-2003P") == 0 ||
				strcmp(runtime[port].sys.stdver, "NMDi-2003PR") == 0 
				)
				{
					runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_DCIRIS);
					runtime[port].image.iris.support = 0;
					runtime[port].image.iris.value = 0;
				}
			}

			{
				char key[64];
				unsigned int onoff;

				// auto focus
				if(runtime[port].ptz.supported & PTZ_SETUP_FOCUS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_focus", port);
					onoff = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_focus(port, onoff);
				}

				// auto iris
				if(runtime[port].ptz.supported & PTZ_SETUP_IRIS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_iris", port);
					onoff = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_iris(port, onoff);
				}
			}

			//runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
			//runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
		}
	}
	sysdb_load(port); // SWIPXXP-689
	/* PTZ initial value retrieve */

	//IPCAM_DBG(MAJOR, "Try to get the PTZ value(CH(%d))\n", port);
	{
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] != NULL)
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get zoom failed(CH(%d))\n", __FUNCTION__, port);
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].ptz.zoom.value = 0;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "ZOOM retrieved value(CH(%d) - %d)\n", port, val);
			runtime[port].ptz.zoom.value = val;
		}
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] != NULL)
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get focus failed(CH(%d))\n", __FUNCTION__, port);
				runtime[port].ptz.focus.value = 0;
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "FOCUS retrieved value(CH(%d) - %d)\n", port, val);
			runtime[port].ptz.focus.value = val;
		}
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] != NULL
		&& (runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS))
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get iris failed(CH:%d)\n", __FUNCTION__, port);
				runtime[port].ptz.iris.value = 0;
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "IRIS retrieved value(CH(%d) - %d)\n", port, val);
			runtime[port].ptz.iris.value = val;
		}
	}
	{
		// exclude image support and include ptz support
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_CALIBRATION)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_CALIBRATION;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_CALIBRATION;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_ONEPUSH)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_ONEPUSH;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_ONEPUSH;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_FOCUS)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_FOCUS_NONPTZ;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_FOCUS;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_IRIS_NONPTZ;
			runtime[port].image.supported &= ~NF_IPCAM_IMAGE_PIRIS;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_ZOOM)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_ZOOM_NONPTZ;
			runtime[port].image.supported &= ~NF_IPCAM_IMAGE_ZOOM;
		}
	}

	if (runtime[port].func & NF_IPCAM_FUNC_ALARM_IN)
	{
		//IPCAM_DBG(MINOR, "ALARM SENSOR value(CH(%d))\n", runtime[port].alarm.alarm_in_type.value);
		nf_ipcam_set_alarm_in(port, 1, runtime[port].alarm.alarm_in_type.value, &initial_setup_callback, NULL, NULL);
		return;
	}

	initial_setup_init(port);
}

/**
 * @brief Open모드에서 ONVIF카메라 연결을 준비한다.
 * @param port 채널 번호.
 *
 * ONVIF capability들을 조회하며, 이기종 연동 프로토콜(S1, huviron)도 처리한다.
 */
extern void nf_openmode_cam_prepare_onvif(int ch)
{
	int rtn;
	mtable* runtime = get_runtime();

	onvif_service_t service;
	memset(&service, 0x00, sizeof(onvif_service_t));
	runtime[ch].state = MGMT_STATE_LINKED|MGMT_STATE_READY|OPENMODE_STATE_CONFIGURING;

	// Get Onvif Support Service
	rtn = nf_onvif_get_capabilities(ch, &service);

	// set time test
	nf_onvif_set_time_sync(ch);
	
	if(strcmp(runtime[ch].sys.vendor, "AXIS") == 0)
	{
		rtn = nf_axis_get_image(ch);
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
            rtn = nf_onvif_get_image_t(ch);
            rtn = nf_onvif_get_image_t_value(ch);
		}
	}
	else
	{
		rtn = nf_onvif_get_image_t(ch);
		rtn = nf_onvif_get_image_t_value(ch);
	}

	if(nf_ipcam_is_vendor_orion() || nf_ipcam_is_vendor_g4s())
	{
		if(strcasecmp(runtime[ch].sys.vendor, "HDPRO") == 0)
		{
			nf_hdpro_enable_mirror(ch);
			nf_hdpro_enable_onepush(ch);
		}
	}

#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E) 
	if((strcmp(runtime[ch].sys.vendor, "HD-IP") == 0) ||
	   (strcmp(runtime[ch].sys.vendor, "IPCamera") == 0))
	{
		// Sunell Fisheye Exception
		if((strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/13") == 0) ||
		   (strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/23") == 0) || 
		   (strcmp(runtime[ch].sys.stdver, "CBP360-IP") == 0))
		{
			NF_IPCAM_MOUNT_TYPES_E mount;
			NF_IPCAM_DEWARP_MODES_E dewarp;

			runtime[ch].cam_type = NF_IPCAM_CAM_TYPE_FISHEYE;
			runtime[ch].fisheye.fisheye_supported = 0;
			runtime[ch].fisheye.fisheye_supported = NF_IPCAM_FISHEYE_MOUNT  |
													NF_IPCAM_FISHEYE_DEWARP |
													NF_IPCAM_FISHEYE_EPTZ;
			nf_sunell_enable_mount(ch);
			nf_sunell_get_mount(ch, &mount);
			runtime[ch].fisheye.mount.value = mount;

			nf_sunell_enable_dewarp(ch);
			nf_sunell_get_dewarp(ch, &dewarp);
			runtime[ch].fisheye.dewarp.value = dewarp;

			nf_sunell_enable_ePTZ(ch);
		}
	}
#endif

#if 0
	// s1 integration
	//if (nf_ipcam_is_vendor_s1() || nf_sysman_get_fwver_vendor() == 83)
	{
		motion_t motion_info;
		rtn = s1_get_event_cap(&motion_info, ch);
		if(rtn == IPCAM_SETUP_RTN_DONE)
		{
			IPCAM_DBG(MINOR, "s1 motion : method(%d) area(%d) width(%d) height(%d) sensmin(%d) sensmax(%d)\n",
					motion_info.method, motion_info.max_rect, motion_info.block_width, motion_info.block_height,
					motion_info.sensitivity.min, motion_info.sensitivity.max
			);

			mtable* runtime = get_runtime();
			memcpy(&runtime[ch].motion, &motion_info, sizeof(motion_t));
			if(motion_info.method != MAM_NONE)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOTION] = &s1_set_motion_area;
				runtime[ch].func |= NF_IPCAM_FUNC_MOTION;
			}

			int cap;
			rtn = s1_get_mirror_cap(&cap, ch);
			//IPCAM_DBG(MINOR, "s1 mirror : support(%08x)\n", cap);
			if(rtn == IPCAM_SETUP_RTN_DONE && cap != 0)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_MIRROR] = &s1_set_mirror_val;	//FIXME separate vcodec & mirror
				runtime[ch].video.mirror.support = cap;
				runtime[ch].video.supported |= VIDEO_SETUP_MIRROR;
				runtime[ch].video.onthefly |= VIDEO_SETUP_MIRROR;
				runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_ROTATION;
			}

			rtn = s1_get_onepush_cap(&cap, ch);
			//IPCAM_DBG(MINOR, "s1 onepush : support(%08x)\n", cap);
			if(rtn == IPCAM_SETUP_RTN_DONE && cap != 0)
			{
				runtime[ch].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = &s1_set_onepush;
				runtime[ch].image.supported |= NF_IPCAM_IMAGE_ONEPUSH;
				runtime[ch].image.onthefly |= NF_IPCAM_IMAGE_ONEPUSH;
				runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;
				runtime[ch].ptz.supported |= PTZ_SETUP_ONEPUSH;
			}

			//rtn = nf_onvif_create_event(ch);
		}
	}
#endif

	// Check Support Onvif Service
	if(runtime[ch].onvif.vac_type != 0)
		_onvif_set_motion_info(runtime, ch, runtime[ch].onvif.vac_type);

	// Realy Output Set
	if(service.device.IO.supported == 1 && 0 < service.device.IO.RelayOutputs)
	{
		IPCAM_DBG(MINOR,"[ONVIF] | ONVIF Device IO Supported, supported(%d), RelayOutputs(%d), InputConnectors(%d)\n",
				service.device.IO.supported, service.device.IO.RelayOutputs, service.device.IO.InputConnectors);
		_onvif_set_relayout(runtime, ch);
	}
	else
	{
		IPCAM_DBG(MINOR,"[ONVIF] | ONVIF Device IO Not Supported, supported(%d), RelayOutputs(%d), InputConnectors(%d)\n",
				service.device.IO.supported, service.device.IO.RelayOutputs, service.device.IO.InputConnectors);
	}


	sysdb_load_onvif(ch);

	rtn = nf_onvif_get_status(ch);
	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(MINOR, "CH(%d) disable focus move\n", ch);
		runtime[ch].image_onvif.supported_image &= ~(NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION | NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE | NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED | NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME );

		runtime[ch].image_onvif.focus.mode.support &= ~(
				NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE | NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE | NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS );

		runtime[ch].image_onvif.move.mode.support &= ~(
				NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE | NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE | NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS );

		runtime[ch].image_onvif.move.abposition.value = 0;
	}

	if(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ))
	{
		rtn = nf_ipcam_prepare_onvif_ptz(ch);
	}

	if (runtime[ch].funcs[NF_IPCAM_TYPE_INIT] == NULL)
	{
		onvif_initial_setup_c0(ch);
		return;
	}

	/* Initialization request */
	IPCAM_DBG(MINOR, "Initialize camera(CH(%d))\n", ch);
	{
		rtn = nf_ipcam_func_init(ch, &onvif_initial_setup_callback, NULL, NULL);

		if (rtn == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			g_warning("[%s] Initialization failed(CH:%d)\n", __FUNCTION__, ch);
			return;

		}
	}
}

/**
 * @brief Discover 된 카메라의 '공장초기화' 모드를 해제시킨다.(S1 시나리오)
 * @param port 채널 번호.
 *
 * SSL 사용중이며 비밀번호가 1234일 때 공장초기화 상태로 간주한다.
 */
static void cam_prepare(int port)
{
	mtable *runtime = get_runtime();
	int type = 0;

	/* SSL factory mode off */
	IPCAM_DBG(MAJOR, "Try to disable factory mode(CH(%d))\n", port);
	/* S1 시나리오 제거 (SWIPXXP-790)
	while (1)
	{
		if (runtime[port].sys.use_ssl == 0)
		{
			IPCAM_DBG(MINOR, "SSL unused(CH(%d))\n", port);
			//evt = ipx_ipcam_new_event(IPCAM_EVENT_DELAY_READY);
			type = IPCAM_EVENT_DELAY_READY;
			break;
		}
#if 1
		if (strcmp(runtime[port].password, "1234") != 0)
		{
			IPCAM_DBG(MINOR, "Non-default state(CH(%d))\n", port);
			//evt = ipx_ipcam_new_event(IPCAM_EVENT_DELAY_READY);
			type = IPCAM_EVENT_DELAY_READY;
			break;
		}
#endif

		if (cam_factory_mode(port) == IPCAM_SETUP_RTN_DONE)
		{
			//IPCAM_DBG(MINOR, "Factory mode off(CH(%d))\n", port);
#if AUTO_SSL_OFF
			runtime[port].sys.use_ssl = 0;
			runtime[port].sys.ssl_g = NULL;
			runtime[port].sys.ctx_g = NULL;
#endif
			nf_pnd_queue_delay(port, IPCAM_EVENT_DELAY_READY, 10, __LINE__, __FILE__);
			return;
			//break;
		}
		else
		{
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
				type = IPCAM_EVENT_CONFIG_FAIL;
			}
			else
			{
				type = IPCAM_EVENT_CONFIG_FAIL;
				g_warning("[%s] Initialization failed(CH(%d))\n", __FUNCTION__, port);
			}
			break;
		}
	}
	*/
	type = IPCAM_EVENT_DELAY_READY;
	nf_pnd_queue_push(port, type, __LINE__, __FILE__);
}

/**
 * @brief CCTV모드에서 ITX카메라 연결을 준비한다.
 * @param port 채널 번호.
 *
 * 모델 db로 처리할 수 없는 것들(현재 렌즈 설정 등)을 조회 및 처리한다.
 */
static void cam_prepare2(int port)
{
	int rtn = 0;
	int result = 0;
	mtable *runtime = get_runtime();
	int (*func0)(int*,int);
	int val = 0;


	if (nf_ipcam_is_vendor_zig())
	{
		IPCAM_DBG(MAJOR, "IPCAM ZIG mode get comp position CH(%d)\n", port);
		result = itx_cam_get_compxy(port, &runtime[port].comp_x, &runtime[port].comp_y);
		if (result == IPCAM_SETUP_RTN_DONE)
		{
			runtime[port].comp_init = 1;
		}
		else
		{
			runtime[port].comp_x = 1;
			runtime[port].comp_y = 3;
			runtime[port].comp_init = 1;
		}
	}

	/* Auto focus capability */
	IPCAM_DBG(MAJOR, "Try to get the auto focus capability(CH(%d))\n", port);
	{
		NFIPCamAFCapa info;

		memset(&info, 0x00, sizeof(NFIPCamAFCapa));
		rtn = nf_ipcam_get_af_capability(port, &info, NULL);
		if (rtn == IPCAM_SETUP_RTN_DONE)
		{
			if (info.mfz == 0)
			{
				//IPCAM_DBG(MINOR, "No AF command supported(CH(%d))\n", port);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
			}
			else if (info.mfz == 1)
			{
				//IPCAM_DBG(MINOR, "AF command enable(CH(%d))\n", port);
				runtime[port].image.supported |= (NF_IPCAM_IMAGE_CALIBRATION | NF_IPCAM_IMAGE_ZOOM | NF_IPCAM_IMAGE_FOCUS | NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].ptz.zoom.min = info.zoom_min;
				runtime[port].ptz.zoom.max = info.zoom_max;
				runtime[port].ptz.focus.min = info.focus_min;
				runtime[port].ptz.focus.max = info.focus_max;
                
                //SWIPXMFOUR-2612   Focus Compensation
                {
                    runtime[port].focus.dnn_comp.value = 1;

                    runtime[port].focus.supported |= NF_IPCAM_FOCUS_DNN_COMP;
                    runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS_COMP] = cam_set_focus_comp;
                    focus_comp_info fcinfo;
                    memset(&fcinfo, 0x00, sizeof(fcinfo));
                    if(cam_get_focus_comp(&fcinfo, port) == IPCAM_SETUP_RTN_DONE)
                    {
                        runtime[port].focus.maskarea[0].value = fcinfo.maskarea[0];
                        runtime[port].focus.areatx[0].value = fcinfo.areatx[0];
                        runtime[port].focus.areaty[0].value = fcinfo.areaty[0];
                        runtime[port].focus.areabx[0].value = fcinfo.areabx[0];
                        runtime[port].focus.areaby[0].value = fcinfo.areaby[0];
                    }
                }
			}

			if (info.iris_mode == 1)
			{
				runtime[port].image.supported |= NF_IPCAM_IMAGE_PIRIS;
				runtime[port].ptz.iris.min = info.iris_min;
				runtime[port].ptz.iris.max = info.iris_max;

				if(runtime[port].sys.model_code == NF_IPCAM_MODEL_TI_368)
				{
					if(strncmp(runtime[port].sys.stdver, "IN", 2) != 0)
					{
						if(strcmp(runtime[port].sys.sdkver, "3.0.0.4") >= 0)
						{
							runtime[port].image.iris.support = 
								(NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL | NF_IPCAM_IMAGE_PIRIS_BQ | NF_IPCAM_IMAGE_PIRIS_DF);
						}
						else
						{
							runtime[port].image.iris.support =
								NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL ;
						}

						if(strstr(runtime[port].sys.stdver,"-0303P") != NULL)
						{
							runtime[port].image.iris.support = NF_IPCAM_IMAGE_DCIRIS_OFF | NF_IPCAM_IMAGE_DCIRIS_ON;
							runtime[port].image.iris.value = NF_IPCAM_IMAGE_DCIRIS_OFF;
						}

						{
							char key[64];
							unsigned int etc;
							snprintf(key, 64, "cam.C%d.iris_control", port);
							etc = nf_sysdb_get_uint(key);
							if ((etc & runtime[port].image.iris.support) != 0)
							{
								runtime[port].image.iris.value = etc;
							}
							else
							{
								runtime[port].image.iris.value = NF_IPCAM_IMAGE_PIRIS_AUTO;
							}
						}
					}
				}
				else
				{
					runtime[port].image.iris.support =
						NF_IPCAM_IMAGE_PIRIS_AUTO | NF_IPCAM_IMAGE_PIRIS_MANUAL |
						NF_IPCAM_IMAGE_PIRIS_BQ | NF_IPCAM_IMAGE_PIRIS_DF |
						NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING | NF_IPCAM_IMAGE_PIRIS_DF_TRACKING;
					{
						char key[64];
						unsigned int etc;
						snprintf(key, 64, "cam.C%d.iris_control", port);
						etc = nf_sysdb_get_uint(key);
						if ((etc & runtime[port].image.iris.support) != 0)
						{
							runtime[port].image.iris.value = etc;
						}
						else
						{
							runtime[port].image.iris.value = NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING;
						}
					}
				}

				if(runtime[port].sys.model_code == NF_IPCAM_MODEL_AMB_A2||
					runtime[port].sys.model_code == NF_IPCAM_MODEL_AMB_D1)
				{
					runtime[port].image.exposure.support =
						NF_IPCAM_IMAGE_EXP_MODE_MANUAL |
						NF_IPCAM_IMAGE_EXP_MODE_AUTO;
				}
				if ((runtime[port].image.exposure.support & runtime[port].image.exposure.value) == 0)
				{
					runtime[port].image.exposure.value = NF_IPCAM_IMAGE_EXP_MODE_AUTO;
				}
			}
			else
			{
				if(strcmp(runtime[port].sys.stdver, "NMB-2003PR") == 0  ||
				strcmp(runtime[port].sys.stdver, "NMX-2003P") == 0 ||
				strcmp(runtime[port].sys.stdver, "NMD-2003P") == 0 ||
				strcmp(runtime[port].sys.stdver, "NMDi-2003PR") == 0 
				)
				{
					runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_DCIRIS);
					runtime[port].image.iris.support = 0;
					runtime[port].image.iris.value = 0;
				}
			}

			{
				char key[64];
				unsigned int onoff;

				// auto focus
				if(runtime[port].ptz.supported & PTZ_SETUP_FOCUS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_focus", port);
					onoff = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_focus(port, onoff);
				}

				// auto iris
				if(runtime[port].ptz.supported & PTZ_SETUP_IRIS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_iris", port);
					onoff = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_iris(port, onoff);
				}
			}

			//runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
			//runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
		}
	}

	/* PTZ initial value retrieve */
	IPCAM_DBG(MAJOR, "Try to get the PTZ value(CH(%d))\n", port);
	{
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] != NULL)
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get zoom failed(CH:%d)\n", __FUNCTION__, port);
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].ptz.zoom.value = 0;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "ZOOM retrieved value(%d)\n", val);
			runtime[port].ptz.zoom.value = val;
		}
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] != NULL)
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get focus failed(CH:%d)\n", __FUNCTION__, port);
				runtime[port].ptz.focus.value = 0;
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "FOCUS retrieved value(%d)\n", val);
			runtime[port].ptz.focus.value = val;
		}
		if (runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] != NULL
		&& (runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS))
		{
			func0 = runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS];
			rtn = func0(&val, port);
			if (rtn == IPCAM_SETUP_RTN_FAILED)
			{
				g_warning("[%s] get iris failed(CH:%d)\n", __FUNCTION__, port);
				runtime[port].ptz.iris.value = 0;
				runtime[port].image.supported &= ~(NF_IPCAM_IMAGE_PIRIS|NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|NF_IPCAM_IMAGE_CALIBRATION|NF_IPCAM_IMAGE_ONEPUSH);
				runtime[port].funcs[NF_IPCAM_TYPE_GET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ZOOM] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_GET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_FOCUS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ORIGIN] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = NULL;

				runtime[port].funcs[NF_IPCAM_TYPE_GET_IRIS] = NULL;
				runtime[port].funcs[NF_IPCAM_TYPE_SET_IRIS] = NULL;
				//return;
			}
			//IPCAM_DBG(MINOR, "IRIS retrieved value(%d)\n", val);
			runtime[port].ptz.iris.value = val;
		}
	}
	{
		// exclude image support and include ptz support
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_CALIBRATION)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_CALIBRATION;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_CALIBRATION;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_ONEPUSH)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_ONEPUSH;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_ONEPUSH;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_FOCUS)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_FOCUS_NONPTZ;
			//runtime[port].image.supported &= ~NF_IPCAM_IMAGE_FOCUS;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_IRIS_NONPTZ;
			runtime[port].image.supported &= ~NF_IPCAM_IMAGE_PIRIS;
		}
		if(runtime[port].image.supported & NF_IPCAM_IMAGE_ZOOM)
		{
			runtime[port].ptz.supported |= PTZ_SETUP_ZOOM_NONPTZ;
			runtime[port].image.supported &= ~NF_IPCAM_IMAGE_ZOOM;
		}
	}

	if (runtime[port].func & NF_IPCAM_FUNC_ALARM_IN)
	{
		//IPCAM_DBG(MINOR, "ALARM SENSOR value(%d)\n", runtime[port].alarm.alarm_in_type.value);
		nf_ipcam_set_alarm_in(port, 1, runtime[port].alarm.alarm_in_type.value, &initial_setup_callback, NULL, NULL);
		return;
	}

	initial_setup_init(port);
}

/**
 * @brief 모델 db에 init으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 *
 * Init function은 현재 A2카메라의 install mode off만 존재한다.
 */
static void initial_setup_init(int port)
{
	mtable *runtime = get_runtime();
	int result = 0;

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_INIT);

	if (runtime[port].funcs[NF_IPCAM_TYPE_INIT] == NULL)
	{
		initial_setup_c0(port);
		return;
	}

	/* Initialization request */
	IPCAM_DBG(MINOR, "Initialize camera(CH(%d))\n", port);
	{
		result = nf_ipcam_func_init(port, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Initialization failed(CH(%d))\n", port);
			}
			return;
		}
	}
}

/**
 * @brief 모델 db에 custom 0으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void initial_setup_c0(int port)
{
	int i;
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM0] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks\n");
		initial_setup_vcodec(port);
		return;
	}

	/* Custom 0 request */
	IPCAM_DBG(MAJOR, "Custom callback 0(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM0, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Custom callback 0 failed(CH(%d))\n", port);
			}
			return;
		}
	}
}

/**
 * @brief 모델 db에 custom 1으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void initial_setup_c1(int port)
{
	int i;
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM1] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		initial_setup_vcodec(port);
		return;
	}

	/* Custom 1 request */
	IPCAM_DBG(MAJOR, "Custom callback 1(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM1, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Custom callback 1 failed(CH(%d))\n", port);
			}
			return;
		}
	}
}

/**
 * @brief 모델 db에 custom 2으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void initial_setup_c2(int port)
{
	int i;
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM2] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		initial_setup_vcodec(port);
		return;
	}

	/* Custom 2 request */
	IPCAM_DBG(MAJOR, "Custom callback 2(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM2, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Custom callback 2 failed(CH(%d))\n", port);
			}
			return;
		}
	}
}

/**
 * @brief 모델 db에 custom 3으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void initial_setup_c3(int port)
{
	int i;
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM3] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		initial_setup_vcodec(port);
		return;
	}

	/* Custom 3 request */
	IPCAM_DBG(MAJOR, "Custom callback 3(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM3, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Custom callback 3 failed(CH(%d))\n", port);
			}
			return;
		}
	}
}

/**
 * @brief 초기 비디오 스트림을 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_vcodec(int port)
{
	int i;
	int result = 0;
	int ntpal;
	mtable *runtime = get_runtime();


	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_VCODEC] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET VIDEO CODEC\n");
		runtime[port].sys.progress = 92;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_va_config(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_VIDEO);
	ntpal = (runtime[port].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

	/* VCodec set request */
	IPCAM_DBG(MAJOR, "Video codec configuration(CH(%d))\n", port);
	{
		NFIPCamSetupVCodec info;
		info.ch = port;
		info.stream_cnt = runtime[port].video.stream_cnt;
		for (i = 0; i < info.stream_cnt; i++)
		{
			info.resolution[i] = runtime[port].video.resolution.resolution[i];
			info.fps[i] = runtime[port].video.fps[ntpal][i].value;
			info.bitrate[i] = runtime[port].video.bitrate[i].value;
			info.bitctrl[i] = runtime[port].video.bitctrl[i];
			info.vcodec[i] = runtime[port].video.vcodec[i];
		}
		info.mirror = runtime[port].video.mirror.value;
		info.ntsc_pal = ntpal;

		if(runtime[port].video.supported & VIDEO_SETUP_CAPTURE_MODE )
		{
			info.capture = runtime[port].video.capture.value;	
		}
		else
		{
			info.capture = 1; // NF_IPCAM_CAPTURE_UNUSED	
		}
		
		result = nf_ipcam_set_vcodec(port, &info, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] Video codec setup failed(CH:%d)\n", __FUNCTION__, port);
			}
			return;
		}
	}

	
	//captainnn
#if 0
	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_VA_CONFIG] != NULL)
	{
		cam_set_va_confg_init(port);
	}
#endif
}

/**
 * @brief 초기 VA Option을 설정한다.
 * @param port 채널 번호.
 *
 * @deprecated va config과 통합됨.
 */
static void initial_setup_va_option(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_VA_OPTION] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET VA OPTION\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_va_config(port);
		return;
	}

	/* ACodec set request */
	if (runtime[port].func & NF_IPCAM_FUNC_VA)
	{
		int ch = port;

		IPCAM_DBG(MINOR, "VA Option configuration(CH(%d))\n", port);

		result = nf_ipcam_set_va_option(port, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] VA Option setup failed(CH:%d)\n", __FUNCTION__, port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "func mask indicate NO VA function supported\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_va_config(port);
		return;
	}
}

/**
 * @brief 초기 VA Config을 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_va_config(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_VA_CONFIG] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET VA CONFIG\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_acodec(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_VCA);
	/* VA set request */
	if (runtime[port].func & NF_IPCAM_FUNC_VA)
	{
		int ch = port;

		IPCAM_DBG(MINOR, "VA Config configuration(CH(%d))\n", port);

		result = nf_ipcam_set_va_config(port, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] VA Config setup failed(CH:%d)\n", __FUNCTION__, port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "func mask indicate NO VA function supported\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_acodec(port);
		return;
	}
}

/**
 * @brief 초기 VA Enable을 설정한다.
 * @param port 채널 번호.
 *
 * @deprecated va config과 통합됨.
 */
static void initial_setup_va_enable(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_ENABLE_VA] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET ENABLE VA\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_acodec(port);
		return;
	}

	/* VA set request */
	if (runtime[port].func & NF_IPCAM_FUNC_VA)
	{
		int ch = port;

		IPCAM_DBG(MINOR, "VA Config configuration(CH(%d))\n", port);

		result = nf_ipcam_set_va_enable(port, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] Enable VA setup failed(CH:%d)\n", __FUNCTION__, port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "func mask indicate NO VA function supported\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_acodec(port);
		return;
	}
}

/**
 * @brief 초기 오디오 코덱을 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_acodec(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_ACODEC] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET AUDIO CODEC\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_image(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_AUDIO);
	/* ACodec set request */
	if ((runtime[port].func & NF_IPCAM_FUNC_AUDIO_RX) ||
		(runtime[port].func & NF_IPCAM_FUNC_AUDIO_TX))
	{
		NFIPCamSetupACodec info;
		int ch = port;

		IPCAM_DBG(MINOR, "Audio codec configuration(CH(%d))\n", port);
		info.ch = port;
		info.audio_tx = runtime[ch].audio.audio_tx;
		info.audio_rx = runtime[ch].audio.audio_rx;
		/* FIXME. acodec type */
		info.audio_codec = (runtime[ch].audio.acodec.value == NF_IPCAM_ACODEC_G711_ULAW) ? 0:1;
		info.mic_volume = runtime[ch].audio.mic_volume.value;
		info.speaker_volume = runtime[ch].audio.speaker_volume.value;

		result = nf_ipcam_set_acodec(port, &info, &initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] Audio codec setup failed(CH:%d)\n", __FUNCTION__, port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "No callback to SET AUDIO CODEC\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_image(port);
		return;
	}
}

/**
 * @brief 초기 카메라 이미지를 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_image(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_IMAGE] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET IMAGE\n");
		runtime[port].sys.progress = 97;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_privacy_mask(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_IMAGE);
	/* Image set request */
	IPCAM_DBG(MAJOR, "Image configuration(CH(%d))\n", port);
	{
		NFIPCamSetupImage info;
		int ch = port;
		memset(&info, 0x00, sizeof(info));

		info.ch = port;
		info.exposure = runtime[ch].image.exposure.value;
		info.agc = runtime[ch].image.agc.value;
		info.eshutter_speed = runtime[ch].image.eshutter_speed.value;
		info.dnn_sense_ntod = runtime[ch].image.dnn_sense_ntod.value;
		info.dnn_sense_dton = runtime[ch].image.dnn_sense_dton.value;
		info.slow_shutter = runtime[ch].image.slow_shutter.value;
		info.max_agc = runtime[ch].image.max_agc.value;
		info.iris = runtime[ch].image.iris.value;
		info.blc = runtime[ch].image.blc.value;
		info.day_night = runtime[ch].image.day_night.value;
		info.det_time = runtime[ch].image.tg_time.value;
		info.white_balance = runtime[ch].image.wb.value;
		info.mwb = runtime[ch].image.mwb.value;
		info.sharpness = runtime[ch].image.sharpness.value;
		info.wdr = runtime[ch].image.wd.value;
		info.focus_mode = runtime[ch].image.focus_mode.value;
		info.dnr_ctr = runtime[ch].image.dnr_ctr.value;
		info.adaptive_ir= runtime[ch].image.adaptive_ir.value;
		info.defog = runtime[ch].image.defog.value;
		info.hlc = runtime[ch].image.hlc.value;

		info.anti_flicker = runtime[ch].image.anti_flicker.value;
		info.max_shutter = runtime[ch].image.max_shutter.value;
		info.base_shutter = runtime[ch].image.base_shutter.value;

		info.brightness = runtime[port].image.brightness.value;
		info.contrast = runtime[port].image.contrast.value;
		info.color = runtime[port].image.color.value;
		info.tint = runtime[port].image.tint.value;

		info.focus_limit = runtime[ch].image.focus_limit.value;
		info.stabilizer = runtime[ch].image.stabilizer.value;
		info.ir_correction = runtime[ch].image.ir_correction.value;

		info.dnn_start_hour = runtime[ch].image.dnn_schedule.start.hour;
		info.dnn_start_min = runtime[ch].image.dnn_schedule.start.min;
		info.dnn_end_hour = runtime[ch].image.dnn_schedule.end.hour;
		info.dnn_end_min = runtime[ch].image.dnn_schedule.end.min;

		if(runtime[port].image.supported & NF_IPCAM_IMAGE_COLORVU){
			info.colorvu_level = runtime[ch].image.colorvu_level.value;
			//info.colorvu_ctrl = runtime[ch].image.colorvu_ctrl.value;
		}

		result = nf_ipcam_set_image(port, &info, &initial_setup_callback, NULL, NULL);

		if(strncmp(runtime[port].sys.stdver,"NPT", 3) != 0)
		{
			if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_NTOD)
			{
				result &= nf_ipcam_set_dnn_adjust_n2d(port, &info, NULL, NULL, NULL);
			}

			if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_DTON)
			{
				result &= nf_ipcam_set_dnn_adjust_d2n(port, &info, NULL, NULL, NULL);
			}
		}

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Image setup failed(CH:%d)\n", port);
			}
			return;
		}

		// iris �߰�
		if((runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS)
		&& (runtime[port].image.iris.value == NF_IPCAM_IMAGE_PIRIS_MANUAL || runtime[port].image.exposure.value == NF_IPCAM_IMAGE_EXP_MODE_MANUAL))
		{
			nf_ipcam_set_iris(port, runtime[port].ptz.iris.value, NULL, NULL, NULL);
		}

		NFIPCamSetupFocusComp focus_comp_info;
		memset(&focus_comp_info, 0x00, sizeof(focus_comp_info));
		// Focus Compensation
		if(runtime[port].focus.supported & NF_IPCAM_FOCUS_DNN_COMP)
		{
			int etc = runtime[port].focus.dnn_comp.value;

			if(etc == 0 || etc == 1)
				focus_comp_info.dnn_comp_mode = etc;
			else
				focus_comp_info.dnn_comp_mode = 1;
		}

		if(runtime[port].focus.supported & NF_IPCAM_FOCUS_TEM_COMP)
		{
			int etc = runtime[port].focus.tem_comp.value;

			if(etc == 0 || etc == 1)
				focus_comp_info.tem_comp_mode = etc;
			else
				focus_comp_info.tem_comp_mode = 1;
		}

		nf_ipcam_set_focus_compensation(port, &focus_comp_info, NULL, NULL, NULL);
	}
}

/**
 * @brief 초기 카메라 사생활 보호를 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_privacy_mask(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_PMASK] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET PRIVACY MASK\n");
		initial_setup_roi_area(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_PMASK);
	/* ACodec set request */
	if (runtime[port].func & NF_IPCAM_FUNC_PRIVACY_MASK)
	{
		int i=0;
		char key[64];
		unsigned int _act, _acnt, _clr;
		NFIPCamPrivacyMask _pmask;

		memset(&_pmask, 0x00, sizeof(NFIPCamPrivacyMask));

		snprintf(key, 64, "cam.privacy.P%d.act", port);
		_act = nf_sysdb_get_bool(key);
		snprintf(key, 64, "cam.privacy.P%d.area.ACNT", port);
		_acnt = nf_sysdb_get_uint(key);
		/* TODO NPT integr */
		if (_act == 0)
		{
			_pmask.ch = port;
			_pmask.rect_cnt = 0;
			_pmask.lt[0].x = (-1);
			_pmask.lt[1].x = (-1);
		}
		//else if( strncmp(runtime[port].sys.model, "NPT", 3) == 0)
		else if( strncmp(runtime[port].sys.stdver, "NPT", 3) == 0)
		{
			_pmask.ch = port;
			_pmask.rect_cnt = 0;
			for(i = 0; i < 10; i++)
			{
				_pmask.lt[i].x = (-1);
			}
		}
		else
		{
			_pmask.ch = port;
			_pmask.rect_cnt = _acnt;

			snprintf(key, 64, "cam.privacy.P%d.color", port);
			_clr = nf_sysdb_get_uint(key);

			for (i=0; i<runtime[port].privacymask.max_rect; i++)
			{
				_pmask.color[i] = _clr;

				snprintf(key, 64, "cam.privacy.P%d.area.A%d.sx", port, i);
				_pmask.lt[i].x = nf_sysdb_get_int(key);
				snprintf(key, 64, "cam.privacy.P%d.area.A%d.sy", port, i);
				_pmask.lt[i].y = nf_sysdb_get_int(key);
				snprintf(key, 64, "cam.privacy.P%d.area.A%d.ex", port, i);
				_pmask.rb[i].x = nf_sysdb_get_int(key);
				snprintf(key, 64, "cam.privacy.P%d.area.A%d.ey", port, i);
				_pmask.rb[i].y = nf_sysdb_get_int(key);
			}

		}
		_convert_pmask_area_by_corridor_view(port, &_pmask);
		result = nf_ipcam_set_privacy_mask(port, &_pmask, &initial_setup_callback, NULL, NULL);
		//if(strncmp(runtime[port].sys.model, "NPT", 3) == 0)
		if(strncmp(runtime[port].sys.stdver, "NPT", 3) == 0)
		{
			runtime[port].func &= ~NF_IPCAM_FUNC_PRIVACY_MASK;
		}

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;
			
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "Privacy mask setup failed(CH:%d)\n", port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "No callback to SET PRIVACY MASK\n");
		initial_setup_roi_area(port);
		return;
	}
}

/**
 * @brief 초기 카메라 ROI area 를 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_roi_area(int port)
{
	int result = 0;
	gint i_level = 0;

	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_ROI] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET ROI Area\n");
		initial_setup_motion_area(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_ROI);

	/* ROI set request */
	if (runtime[port].func & NF_IPCAM_FUNC_ROI)
	{
		int width = 0, height = 0;
		uint64_t current = 0;
		int i=0;
		char key[64];
		gchar *model_nm;

		snprintf(key, 64, "cam.C%d.model_nm", port);
		model_nm = nf_sysdb_get_str_nocopy(key);

		current = runtime[port].video.resolution.resolution[0];

		if(nf_ipcam_change_ipcamres_to_size(current, &width, &height) == IPCAM_SETUP_RTN_DONE)
		{
			NFIPCamSetupROIArea _roi_area;
			memset(&_roi_area, 0x00, sizeof(NFIPCamSetupROIArea));
			
			if(strcmp(model_nm, runtime[port].sys.stdver) != 0)
			{
				//최초
				_roi_area.ch = port;
				_roi_area.width = width;
				_roi_area.height = height;
				_roi_area.roi_area_num = runtime[port].roi_area.max_rect;
				_roi_area.roi_mode = ROI_MANUAL;
				_roi_area.roi_quality = ROI_MID;

				for(i = 0; i < runtime[port].roi_area.max_rect; i++)
				{
					_roi_area.roi_area[i].interest_level = 0;

					_roi_area.roi_area[i].left_top.x = 0;
					_roi_area.roi_area[i].left_top.y = 0;
					_roi_area.roi_area[i].right_bottom.x = 0;
					_roi_area.roi_area[i].right_bottom.y = 0;

					//sysdb init (local UI) 
				}
			}
			else
			{
				_roi_area.ch = port;
				_roi_area.width = width;
				_roi_area.height = height;
				_roi_area.roi_area_num = runtime[port].roi_area.max_rect;

				snprintf(key, 64, "cam.ROI.C%d.mode", port);
				_roi_area.roi_mode = nf_sysdb_get_uint(key);
				snprintf(key, 64, "cam.ROI.C%d.quality", port);
				_roi_area.roi_quality = nf_sysdb_get_int(key);

				for(i = 0; i < runtime[port].roi_area.max_rect; i++)
				{
					snprintf(key, 64, "cam.ROI.C%d.area.A%d.level", port, i);
					_roi_area.roi_area[i].interest_level = nf_sysdb_get_int(key);

					snprintf(key, 64, "cam.ROI.C%d.area.A%d.tx", port, i);
					_roi_area.roi_area[i].left_top.x = nf_sysdb_get_int(key);

					snprintf(key, 64, "cam.ROI.C%d.area.A%d.ty", port, i);
					_roi_area.roi_area[i].left_top.y = nf_sysdb_get_int(key);

					snprintf(key, 64, "cam.ROI.C%d.area.A%d.bx", port, i);
					_roi_area.roi_area[i].right_bottom.x = nf_sysdb_get_int(key);


					snprintf(key, 64, "cam.ROI.C%d.area.A%d.by", port, i);
					_roi_area.roi_area[i].right_bottom.y = nf_sysdb_get_int(key);
				}
			}

			_convert_roi_area_by_corridor_view(port, &_roi_area);
			result = nf_ipcam_set_roi_area(port, &_roi_area, &initial_setup_callback, NULL, NULL);
		}
		else
		{
			result == IPCAM_SETUP_RTN_FAILED;
		}

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt = NULL;

			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				IPCAM_DBG(WARN, "ROI Area setup failed(CH:%d)\n", port);
			}
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "No callback to SET ROI Area\n");
		initial_setup_motion_area(port);
		return;
	}
}

/**
 * @brief 초기 카메라 모션 영역을 설정한다.
 * @param port 채널 번호.
 */
static void initial_setup_motion_area(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	gchar key[64];
	guint sense_d;
	gint i = 0;


	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_MOTION] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET MOTION AREA\n");
		runtime[port].sys.progress = 99;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		initial_setup_done(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_MOTION);
	/* Motion area set request */
	switch(runtime[port].motion.method)
	{
		case MAM_RECTANGLE:
		case MAM_SIM_RECTANGLE:
		{
			NFIPCamSetupMotionArea info;
			gchar *area;

			memset(&info, 0x00, sizeof(NFIPCamSetupMotionArea));
			snprintf(key, 64, "alarm.motion.M%d.area", port);
			area = nf_sysdb_get_str_nocopy(key);
			strncpy(info.area, area, 1400);

			if (get_dn_now(port) == 1)
			{
				snprintf(key, 64, "alarm.motion.M%d.sense_d", port);
			}
			else
			{
				snprintf(key, 64, "alarm.motion.M%d.sense_n", port);
			}
			sense_d = nf_sysdb_get_uint(key);
			snprintf(key, 64, "alarm.motion.M%d.rect.RCNT", port);
			info.area_num = nf_sysdb_get_uint(key);

			if (info.area_num == 0)
			{
				info.ch = port;
				info.block_width = runtime[port].motion.block_width;
				info.block_height = runtime[port].motion.block_height;
				info.method = runtime[port].motion.method;
				info.area_num = 0;
				info.marea[0].sensitivity = sense_d;
				_get_rect_points(
						info.block_width,
						info.block_height,
						area,
						&info.marea[0].FIGURE.RECTANGLE.left_top,
						&info.marea[0].FIGURE.RECTANGLE.right_bottom
				);
			}
			else 
			{
				info.ch = port;
				info.block_width = runtime[port].motion.block_width;
				info.block_height = runtime[port].motion.block_height;
				info.method = runtime[port].motion.method;
				for (i = 0; i < info.area_num; i++)
				{
					info.marea[i].sensitivity = sense_d;
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", port, i);
					info.marea[i].FIGURE.RECTANGLE.left_top.y = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", port, i);
					info.marea[i].FIGURE.RECTANGLE.left_top.x = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", port, i);
					info.marea[i].FIGURE.RECTANGLE.right_bottom.y = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", port, i);
					info.marea[i].FIGURE.RECTANGLE.right_bottom.x = nf_sysdb_get_uint(key);
				}
			}
			result = nf_ipcam_set_motion_area(port, &info, &initial_setup_callback, NULL, NULL);

			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				IPX_PND_EVENT *evt = NULL;
				
				if (nf_get_running_mode())
				{
					runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
					g_warning("[%s] Motion area setup failed(CH:%d)\n", __FUNCTION__, port);
				}
				return;
			}
			break;
		}
		case MAM_POLYGON:
		{
			break;
		}
		case MAM_CELL:
		{
			break;
		}
		case MAM_NONE:
		case MAM_RAW_STREAM:
		default:
			IPCAM_DBG(MINOR, "NO NEED TO SET MOTION AREA\n");
			runtime[port].sys.progress = 99;
			nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
			initial_setup_done(port);
			break;
	}
}

/**
 * @brief 초기 카메라 설정 종료시 호출. 이후 mrtp library를 통해 영상 재생 시작함.
 * @param port 채널 번호.
 */
static void initial_setup_done(int port)
{
	IPX_PND_EVENT *evt = NULL;
	mtable *runtime = get_runtime();

	//queue = get_queue();
	nf_ipcam_set_pnd_osd_status(port, PND_OSD_STREAM);
	if (nf_get_running_mode())
	{
		runtime[port].state = MGMT_STATE_LINKED|MGMT_STATE_READY|MGMT_STATE_CONFIGURED|OPENMODE_STATE_READY;
	}
	else
	{
		//IPCAM_DBG(MAJOR, "Device configurations are completed(CH(%d))\n", port);
		nf_pnd_queue_push(port, IPCAM_EVENT_STREAM_READY, __LINE__, __FILE__);
	}
}

/**
 * @brief 초기 설정 함수 호출시 등록한 콜백함수.
 * @param prog 함수 실행 결과가 포함된 progress 정보 struct.
 * @param ud 콜백 등록시 넘긴 user_data(사용안함).
 *
 * 초기설정 함수 호출결과를 분석하고 다음 초기화 루틴을 실행시킨다.
 */
static void initial_setup_callback(NFIPCamProgress* prog, gpointer ud)
{
	int i = 0;
	int ch = prog->ch;
	int type = prog->type;
	int status = prog->status;
	mtable *runtime = get_runtime();


	if (status == NF_IPCAM_STATUS_END_SUCCESS)
	{
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			IPCAM_DBG(WARN, "initial setup break\n");
			return;
		}
		if ((nf_get_running_mode() == 1) && ((nf_openmode_get_state() == OPENMODE_STATE_INIT)||(nf_openmode_get_discovery() > 0)))
		{
			IPCAM_DBG(WARN, "initial setup break\n");
			runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
			return;
		}

		switch(type)
		{
			case NF_IPCAM_TYPE_SET_ALARM:
				IPCAM_DBG(MINOR, "SET ALARM callback\n");
				runtime[ch].sys.progress = 91;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_init(ch);
				break;
			case NF_IPCAM_TYPE_INIT:
				IPCAM_DBG(MINOR, "SET INIT callback\n");
				runtime[ch].sys.progress = 91;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_c0(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM0:
				IPCAM_DBG(MINOR, "SET CUSTOM0 callback\n");
				runtime[ch].sys.progress = 92;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_c1(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM1:
				IPCAM_DBG(MINOR, "SET CUSTOM1 callback\n");
				runtime[ch].sys.progress = 93;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_c2(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM2:
				IPCAM_DBG(MINOR, "SET CUSTOM2 callback\n");
				runtime[ch].sys.progress = 94;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_c3(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM3:
				IPCAM_DBG(MINOR, "SET CUSTOM3 callback\n");
				runtime[ch].sys.progress = 95;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_vcodec(ch);
				break;
			case NF_IPCAM_TYPE_SET_VCODEC:
				IPCAM_DBG(MINOR, "SET VIDEO CODEC callback\n");
				runtime[ch].sys.progress = 96;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_va_config(ch);
				break;
			case NF_IPCAM_TYPE_SET_VA_OPTION:
				IPCAM_DBG(MINOR, "SET VA OPTION callback\n");
				runtime[ch].sys.progress = 96;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_va_config(ch);
				break;
			case NF_IPCAM_TYPE_SET_VA_CONFIG:
				IPCAM_DBG(MINOR, "SET VA CONFIG callback\n");
				runtime[ch].sys.progress = 96;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_acodec(ch);
				break;
			case NF_IPCAM_TYPE_SET_ENABLE_VA:
				IPCAM_DBG(MINOR, "SET ENABLE VA callback\n");
				runtime[ch].sys.progress = 96;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_acodec(ch);
				break;
			case NF_IPCAM_TYPE_SET_ACODEC:
				IPCAM_DBG(MINOR, "SET AUDIO CODEC callback\n");
				runtime[ch].sys.progress = 97;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_image(ch);
				break;
			case NF_IPCAM_TYPE_SET_IMAGE:
				IPCAM_DBG(MINOR, "SET IMAGE callback\n");
				runtime[ch].sys.progress = 98;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_privacy_mask(ch);
				break;
			case NF_IPCAM_TYPE_SET_PMASK:
				IPCAM_DBG(MINOR, "SET PMASK callback\n");
				initial_setup_roi_area(ch);
				break;
			case NF_IPCAM_TYPE_SET_ROI:
				IPCAM_DBG(MINOR, "SET ROI callback\n");
				initial_setup_motion_area(ch);
				break;
			case NF_IPCAM_TYPE_SET_MOTION:
				IPCAM_DBG(MINOR, "SET MOTION callback\n");
				runtime[ch].sys.progress = 99;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				initial_setup_done(ch);
				break;
			default:
			{
				IPX_PND_EVENT *evt = NULL;
				if(nf_get_running_mode())
				{
					runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
					IPCAM_DBG(WARN, "Setup failed(CH(%d),TYPE:%d)\n", ch, type);
				}
				break;
			}
		}
	}
	else if (status == NF_IPCAM_STATUS_TIMEOUT || status == NF_IPCAM_STATUS_FAILED_REQ)
	{
		IPX_PND_EVENT *evt = NULL;
		if (nf_get_running_mode())
		{
			//if (nf_openmode_get_live())
			{
				runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
			}
		}
		else
		{
			nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			IPCAM_DBG(WARN, "Setup failed(CH(%d),TYPE:%s)\n", ch, _jobs[type].t_str);

			//nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
		}
		return;
	}
	else
	{
		return;
	}
}

/**
 * @brief CCTV모드에서 ONVIF카메라 연결을 준비한다.
 * @param port 채널 번호.
 *
 * ONVIF capability들을 조회하며, 이기종 연동 프로토콜(S1, huviron)도 처리한다.
 */
static void cam_prepare_onvif(int port)
{
	int i = 0;
	int rtn = 0;
	int result = 0;
	mtable *runtime = get_runtime();
	int (*func0)(int);

	//IPCAM_DBG(MAJOR, "start CH(CH(%d))\n", port);

	rtn = nf_onvif_get_status(port);
	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(MINOR, "CH(%d) disable focus move\n", port);
		runtime[port].image_onvif.supported_image &= ~(NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION | NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE | NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED |
				NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED | NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME );

		runtime[port].image_onvif.focus.mode.support &= ~(
				NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE | NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE | NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS );

		runtime[port].image_onvif.move.mode.support &= ~(
				NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE | NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE | NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS );

		runtime[port].image_onvif.move.abposition.value = 0;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_INIT);
	// time sync
 	nf_onvif_set_time_sync(port);

	//if (runtime[port].sys.model_code == NF_IPCAM_MODEL_ONVIF_L1)
	{
		if (runtime[port].funcs[NF_IPCAM_TYPE_INIT] == NULL)
		{
			onvif_initial_setup_c0(port);
			return;
		}

		/* Initialization request */
		IPCAM_DBG(MINOR, "Initialize camera(CH(%d))\n", port);
		{
			result = nf_ipcam_func_init(port, &onvif_initial_setup_callback, NULL, NULL);

			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				IPX_PND_EVENT *evt;
				if (nf_get_running_mode())
				{
					runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				}
				g_warning("[%s] Initialization failed(CH:%d)\n", __FUNCTION__, port);
				return;

			}
		}
	}
	/*else
	{
		nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
		g_warning("[%s] ONVIF Setup failed(CH:%d)\n", __FUNCTION__, port);
	}*/
}

/**
 * @brief ONVIF 모델 db에 custom 0으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 *
 * 주로 grundig 카메라 SDK 용도로 쓰인다.
 */
static void onvif_initial_setup_c0(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM0] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks\n");
		runtime[port].sys.progress = 91;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_vcodec(port);
		return;
	}

	/* Custom 0 request */
	IPCAM_DBG(MAJOR, "Custom callback 0(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM0, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Custom callback 0 failed(CH:%d)\n", port);
			return;
		}
	}
}

/**
 * @brief ONVIF 모델 db에 custom 1으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_c1(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM1] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		runtime[port].sys.progress = 91;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_vcodec(port);
		return;
	}

	/* Custom 1 request */
	IPCAM_DBG(MAJOR, "Custom callback 1(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM1, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			g_warning("[%s] Custom callback 1 failed(CH:%d)\n", __FUNCTION__, port);
			return;
		}
	}
}

/**
 * @brief ONVIF 모델 db에 custom 2으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_c2(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM2] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		runtime[port].sys.progress = 91;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_vcodec(port);
		return;
	}

	/* Custom 1 request */
	IPCAM_DBG(MAJOR, "Custom callback 2(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM2, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Custom callback 2 failed(CH:%d)\n", port);
			return;
		}
	}
}

/**
 * @brief ONVIF 모델 db에 custom 3으로 등록된 초기화 루틴을 실행한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_c3(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_CUSTOM3] == NULL)
	{
		IPCAM_DBG(MINOR, "No CUSTOM callbacks more\n");
		runtime[port].sys.progress = 91;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_vcodec(port);
		return;
	}

	/* Custom 1 request */
	IPCAM_DBG(MAJOR, "Custom callback 3(CH(%d))\n", port);
	{
		result = nf_ipcam_setup_request(port, NF_IPCAM_TYPE_CUSTOM3, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Custom callback 3 failed(CH:%d)\n", port);
			return;
		}
	}
}

/**
 * @brief 초기 ONVIF 비디오 스트림을 설정한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_vcodec(int port)
{
	int i;
	int result = 0;
	int ntpal;
	int temp_rtn;
	mtable *runtime = get_runtime();

	//IPCAM_DBG(MINOR, "get profile token to stream(CH:%d)\n", port);

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_VIDEO);
	/*temp_rtn = nf_onvif_get_appropriate_profile(port);
	if (temp_rtn != 0)
	{
		IPX_PND_EVENT *evt;
		if (nf_get_running_mode())
		{
			runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
		}
		else
		{
			nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
		}
		return;
	}*/

	if(!(runtime[port].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_MEDIA)))
	{
		IPCAM_DBG(MINOR, "No callback to SET VIDEO CODEC\n");
		runtime[port].sys.progress = 92;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_acodec(port);
		return;
	}

	/* VCodec set request */
	IPCAM_DBG(MAJOR, "Video codec configuration(CH(%d))\n", port);
	{
		int ntpal = (runtime[port].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

		NFIPCamSetupVCodec info;
		memset(&info, 0x00, sizeof(NFIPCamSetupVCodec));
		info.ch = port;
		info.stream_cnt = runtime[port].video.stream_cnt;
		for (i = 0; i < info.stream_cnt; i++)
		{
			info.resolution[i] = runtime[port].video.resolution.resolution[i];
			info.fps[i] = runtime[port].video.fps[ntpal][i].value;
			info.bitrate[i] = runtime[port].video.bitrate[i].value;
			info.bitctrl[i] = runtime[port].video.bitctrl[i];
			info.vcodec[i] = runtime[port].video.vcodec[i];
		}
		info.capture = runtime[port].video.capture.value;
		info.mirror = runtime[port].video.mirror.value;
		info.ntsc_pal = ntpal;

		result = nf_ipcam_set_vcodec_onvif(port, &info, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Video codec setup failed(CH:%d)\n", port);
			return;
		}
		// XXX mirror added
		if(runtime[port].video.mirror.value != 0
		&& (runtime[port].video.mirror.value & runtime[port].video.mirror.support)
		&& (runtime[port].video.supported & VIDEO_SETUP_MIRROR))
		{
			IPCAM_DBG(MINOR, "S1 Mirror configuration(CH(%d))\n", port);
			result = nf_ipcam_set_rotation(port, runtime[port].video.mirror.value, NULL, NULL, NULL);
			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				IPCAM_DBG(MINOR, "S1 Mirror set failed(CH(%d))\n", port);
			}
		}
	}
}

/**
 * @brief 초기 ONVIF 오디오 코덱을 설정한다.
 * @param port 채널 번호.
 *
 * @todo 현재 SDK로만 구현되어 있으며, ONVIF로 구현 필요.
 */
static void onvif_initial_setup_acodec(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_ACODEC] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET AUDIO CODEC\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_image(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_AUDIO);
	/* ACodec set request */
	if ((runtime[port].func & NF_IPCAM_FUNC_AUDIO_RX) ||
		(runtime[port].func & NF_IPCAM_FUNC_AUDIO_TX))
	{
		NFIPCamSetupACodec info;
		int ch = port;

		IPCAM_DBG(MAJOR, "Audio codec configuration(CH(%d))\n", port);
		info.ch = port;
		info.audio_tx = runtime[ch].audio.audio_tx;
		info.audio_rx = runtime[ch].audio.audio_rx;
		/* FIXME. acodec type */
		info.audio_codec = (runtime[ch].audio.acodec.value == NF_IPCAM_ACODEC_G711_ULAW) ? 0:1;
		info.mic_volume = runtime[ch].audio.mic_volume.value;
		info.speaker_volume = runtime[ch].audio.speaker_volume.value;

		result = nf_ipcam_set_acodec(port, &info, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
			}
			else
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Audio codec setup failed(CH(%d))\n", port);
			return;
		}
	}
	else
	{
		IPCAM_DBG(MINOR, "No callback to SET AUDIO CODEC\n");
		runtime[port].sys.progress = 95;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_image(port);
		return;
	}
}

/**
 * @brief 초기 ONVIF 카메라 이미지를 설정한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_image(int port)
{
	int result = 0;
	mtable *runtime = get_runtime();

	if(!(runtime[port].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_IMAGE)))
	{
		IPCAM_DBG(MINOR, "No support to SET IMAGE\n");
		runtime[port].sys.progress = 97;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_motion_area(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_IMAGE);
	/* Image set request */
	IPCAM_DBG(MAJOR, "Image configuration(CH(%d))\n", port);
	{
		NFIPCamSetupImage_onvif info;
		memset(&info, 0x00, sizeof(info));

		int ch = port;

		info.ch = port;
		info.brightness = runtime[ch].image_onvif.brightness.value;
		info.color = runtime[ch].image_onvif.color.value;
		info.contrast = runtime[ch].image_onvif.contrast.value;
		info.sharpness = runtime[ch].image_onvif.sharpness.value;

		info.focus_mode = runtime[ch].image_onvif.focus.mode.value;
		info.default_speed = runtime[ch].image_onvif.focus.defaultspeed.value;
		info.near_limit = runtime[ch].image_onvif.focus.nearlimit.value;
		info.far_limit = runtime[ch].image_onvif.focus.farlimit.value;


		info.white_balance = runtime[ch].image_onvif.wb.mode.value;
		info.cbgain = runtime[ch].image_onvif.wb.cbgain.value;
		info.crgain = runtime[ch].image_onvif.wb.crgain.value;

		NFIPCamSetupExposure_onvif info2;
		memset(&info2, 0x00, sizeof(info2));

		info2.ch = port;
		info2.mode = runtime[ch].image_onvif.exposure.mode.value;
		info2.priority = runtime[ch].image_onvif.exposure.priority.value;
		info2.minetime = runtime[ch].image_onvif.exposure.minetime.value;
		info2.maxetime = runtime[ch].image_onvif.exposure.maxetime.value;
		info2.etime = runtime[ch].image_onvif.exposure.etime.value;
		info2.mingain = runtime[ch].image_onvif.exposure.mingain.value;
		info2.maxgain = runtime[ch].image_onvif.exposure.maxgain.value;
		info2.gain = runtime[ch].image_onvif.exposure.gain.value;
		info2.miniris = runtime[ch].image_onvif.exposure.miniris.value;
		info2.maxiris = runtime[ch].image_onvif.exposure.maxiris.value;
		info2.iris = runtime[ch].image_onvif.exposure.iris.value;
		info2.iris_control = runtime[ch].image_onvif.exposure.iris_mode.value;
		info2.mirror = runtime[ch].video.mirror.value;
		info2.wide_dynamic_mode = runtime[ch].image_onvif.wdrmode.value;
		info2.wide_level = runtime[ch].image_onvif.wdrlevel.value;
		info2.ircut = runtime[ch].image_onvif.ircut.value;

		info2.blc_mode = runtime[ch].image_onvif.blcmode.value;
		info2.blc_level = runtime[ch].image_onvif.blclevel.value;

		result = nf_ipcam_set_image_exp_onvif(port, &info, &info2, &onvif_initial_setup_callback, NULL, NULL);

		if (result == IPCAM_SETUP_RTN_FAILED)
		{
			// exclude support
			runtime[ch].onvif.onvif_service &= ~(__OFM(NF_ONVIF_SERVICE_IMAGE));
			memset(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE], 0x00, 128);
			runtime[ch].image_onvif.supported_image = 0;
			runtime[ch].image_onvif.supported_exposure = 0;
/*
			nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);

			g_warning("[%s] Image setup failed(CH:%d)\n", __FUNCTION__, port);
			return;
*/
		}
	}
}

/**
 * @brief 초기 ONVIF 모션 영역을 설정한다.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_motion_area(int port)
{
	int result = 0;
	int sense_d = 0;
	char key[64];
	int i = 0;
	mtable *runtime = get_runtime();

	if (runtime[port].funcs[NF_IPCAM_TYPE_SET_MOTION] == NULL)
	{
		IPCAM_DBG(MINOR, "No callback to SET MOTION AREA\n");
		runtime[port].sys.progress = 99;
		nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
		onvif_initial_setup_done(port);
		return;
	}

	nf_ipcam_set_pnd_osd_status(port, PND_OSD_CONFIG_MOTION);
	switch(runtime[port].motion.method)
	{
		case MAM_RECTANGLE:
		case MAM_SIM_RECTANGLE:
		{
			NFIPCamSetupMotionArea info;
			gchar *area;

			memset(&info, 0x00, sizeof(NFIPCamSetupMotionArea));
			snprintf(key, 64, "alarm.motion.M%d.area", port);
			area = nf_sysdb_get_str_nocopy(key);
			strncpy(info.area, area, 1400);

			snprintf(key, 64, "alarm.motion.M%d.sense_d", port);
			sense_d = nf_sysdb_get_uint(key);
			snprintf(key, 64, "alarm.motion.M%d.rect.RCNT", port);
			info.area_num = nf_sysdb_get_uint(key);

			if (info.area_num == 0)
			{
				info.ch = port;
				info.block_width = runtime[port].motion.block_width;
				info.block_height = runtime[port].motion.block_height;
				info.method = runtime[port].motion.method;
				info.area_num = 1;
				info.marea[0].sensitivity = sense_d;
				_get_rect_points(
						info.block_width,
						info.block_height,
						area,
						&info.marea[0].FIGURE.RECTANGLE.left_top,
						&info.marea[0].FIGURE.RECTANGLE.right_bottom
				);
			}
			else 
			{
				info.ch = port;
				info.block_width = runtime[port].motion.block_width;
				info.block_height = runtime[port].motion.block_height;
				info.method = runtime[port].motion.method;
				for (i = 0; i < info.area_num; i++)
				{
					info.marea[i].sensitivity = sense_d;
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", port, i);
					info.marea[i].FIGURE.RECTANGLE.left_top.y = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", port, i);
					info.marea[i].FIGURE.RECTANGLE.left_top.x = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", port, i);
					info.marea[i].FIGURE.RECTANGLE.right_bottom.y = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", port, i);
					info.marea[i].FIGURE.RECTANGLE.right_bottom.x = nf_sysdb_get_uint(key);
				}
			}
			result = nf_ipcam_set_motion_area(port, &info, &onvif_initial_setup_callback, NULL, NULL);

			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				IPX_PND_EVENT *evt;
				if (nf_get_running_mode())
				{
					runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				}
				g_warning("[%s] Motion area setup failed(CH:%d)\n", __FUNCTION__, port);
				return;
			}
			break;
		}
		case MAM_POLYGON:
		{
			break;
		}
		case MAM_CELL:
		{
			NFIPCamSetupMotionArea info;
			gchar *cells_flag = NULL;

			snprintf(key, 64, "alarm.motion.M%d.area", port);
			cells_flag = nf_sysdb_get_str_nocopy(key);
			strncpy(info.area, cells_flag, 1400);

			snprintf(key, 64, "alarm.motion.M%d.sense_d", port);
			sense_d = nf_sysdb_get_uint(key);
			if (sense_d > runtime[port].motion.sensitivity.max)
			{
				sense_d = runtime[port].motion.sensitivity.max;
			}
			else if (sense_d < runtime[port].motion.sensitivity.min)
			{
				sense_d = runtime[port].motion.sensitivity.min;
			}

			info.ch = port;
			info.block_width = runtime[port].motion.block_width;
			info.block_height = runtime[port].motion.block_height;
			info.method = runtime[port].motion.method;
			info.area_num = 1;
			info.marea[0].sensitivity = sense_d;
			for (i=0; i<info.block_width*info.block_height; i++)
			{
				info.marea[0].FIGURE.CELL.active_cell[i] = *(cells_flag+i);
			}

			result = nf_ipcam_set_motion_area(port, &info, &onvif_initial_setup_callback, NULL, NULL);

			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				IPX_PND_EVENT *evt;
				if (nf_get_running_mode())
				{
					runtime[port].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				}
				g_warning("[%s] Motion area setup failed(CH:%d)\n", __FUNCTION__, port);
				return;
			}
			
			break;
		}
		case MAM_NONE:
		case MAM_RAW_STREAM:
		default:
			IPCAM_DBG(MINOR, "NO NEED TO SET MOTION AREA\n");
			runtime[port].sys.progress = 99;
			nf_pnd_prog_notify_fire(port, runtime[port].sys.progress, __LINE__, __FILE__);
			onvif_initial_setup_done(port);
			break;
	}
}

/**
 * @brief 초기 ONVIF 카메라 설정 종료시 호출. 이후 mrtp library를 통해 영상 재생 시작함.
 * @param port 채널 번호.
 */
static void onvif_initial_setup_done(int port)
{
	IPX_PND_EVENT *evt = NULL;
	mtable *runtime = get_runtime();

	if (nf_get_running_mode())
	{
		runtime[port].state = MGMT_STATE_LINKED|MGMT_STATE_READY|MGMT_STATE_CONFIGURED|OPENMODE_STATE_READY;
	}
	else
	{
		nf_pnd_queue_push(port, IPCAM_EVENT_STREAM_READY, __LINE__, __FILE__);
	}
	//IPCAM_DBG(MAJOR, "Device configurations are completed(CH(%d))\n", port);
}

/**
 * @brief 초기 ONVIF 설정 함수 호출시 등록한 콜백함수.
 * @param prog 함수 실행 결과가 포함된 progress 정보 struct.
 * @param ud 콜백 등록시 넘긴 user_data(사용안함).
 *
 * 초기설정 함수 호출결과를 분석하고 다음 초기화 루틴을 실행시킨다.
 */
static void onvif_initial_setup_callback(NFIPCamProgress* prog, gpointer ud)
{
	int i = 0;
	int ch = prog->ch;
	int type = prog->type;
	int status = prog->status;
	mtable *runtime = get_runtime();


	if (status == NF_IPCAM_STATUS_END_SUCCESS)
	{
		if (nf_get_running_mode() == 0 && get_running_state() == DISCOVERY_STOPPED)
		{
			return;
		}
		if ((nf_get_running_mode() == 1) && ((nf_openmode_get_state() == OPENMODE_STATE_INIT)||(nf_openmode_get_discovery() > 0)))
		{
			IPCAM_DBG(WARN, "initial setup break\n");
			return;
		}
		switch(type)
		{
			case NF_IPCAM_TYPE_INIT:
				IPCAM_DBG(MINOR, "SET INIT callback\n");
				runtime[ch].sys.progress = 90;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_c0(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM0:
				IPCAM_DBG(MINOR, "SET CUSTOM0 callback\n");
				runtime[ch].sys.progress = 91;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_c1(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM1:
				IPCAM_DBG(MINOR, "SET CUSTOM1 callback\n");
				runtime[ch].sys.progress = 92;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_c2(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM2:
				IPCAM_DBG(MINOR, "SET CUSTOM2 callback\n");
				runtime[ch].sys.progress = 93;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_c3(ch);
				break;
			case NF_IPCAM_TYPE_CUSTOM3:
				IPCAM_DBG(MINOR, "SET CUSTOM3 callback\n");
				runtime[ch].sys.progress = 94;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_vcodec(ch);
				break;
			case NF_IPCAM_TYPE_SET_VCODEC:
				IPCAM_DBG(MINOR, "SET VIDEO CODEC callback\n");
				runtime[ch].sys.progress = 95;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_acodec(ch);
				break;
			case NF_IPCAM_TYPE_SET_ACODEC:
				IPCAM_DBG(MINOR, "SET AUDIO CODEC callback\n");
				runtime[ch].sys.progress = 96;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_image(ch);
				break;
			case NF_IPCAM_TYPE_SET_IMAGE_ONVIF:
				IPCAM_DBG(MINOR, "SET IMAGE callback\n");
				runtime[ch].sys.progress = 97;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_motion_area(ch);
				break;
			case NF_IPCAM_TYPE_SET_MOTION:
				IPCAM_DBG(MINOR, "SET MOTION callback\n");
				runtime[ch].sys.progress = 99;
				nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
				onvif_initial_setup_done(ch);
				break;
			default:
			{
				IPX_PND_EVENT *evt;
				if (nf_get_running_mode())
				{
					runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
				}
				else
				{
					nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				}
				IPCAM_DBG(WARN, "Setup failed(CH(%d),TYPE:%d))\n", ch,type);
				break;
			}
		}
#if 0
		if (type == NF_IPCAM_TYPE_INIT)
		{
			printf("[%s] SET INIT callback\n", __FUNCTION__);
			onvif_initial_setup_c0(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_CUSTOM0)
		{
			printf("[%s] SET CUSTOM 0 callback\n", __FUNCTION__);
			onvif_initial_setup_c1(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_CUSTOM1)
		{
			printf("[%s] SET CUSTOM 1 callback\n", __FUNCTION__);
			onvif_initial_setup_c2(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_CUSTOM2)
		{
			printf("[%s] SET CUSTOM 2 callback\n", __FUNCTION__);
			onvif_initial_setup_c3(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_CUSTOM3)
		{
			printf("[%s] SET CUSTOM 3 callback\n", __FUNCTION__);
			onvif_initial_setup_vcodec(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_SET_VCODEC)
		{
			printf("[%s] SET VIDEO CODEC callback\n", __FUNCTION__);
			runtime[ch].sys.progress = 92;
			nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
			onvif_initial_setup_acodec(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_SET_ACODEC)
		{
			printf("[%s] SET AUDIO CODEC callback\n", __FUNCTION__);
			runtime[ch].sys.progress = 95;
			nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
			onvif_initial_setup_image(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_SET_IMAGE)
		{
			printf("[%s] SET IMAGE callback\n", __FUNCTION__);
			runtime[ch].sys.progress = 97;
			nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
			onvif_initial_setup_motion_area(ch);
			return;
		}
		else if (type == NF_IPCAM_TYPE_SET_MOTION)
		{
			printf("[%s] SET MOTION AREA callback\n", __FUNCTION__);
			runtime[ch].sys.progress = 99;
			nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
			onvif_initial_setup_done(ch);
			return;
		}
		else
		{
			nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			g_warning("[%s] Setup failed(CH:%d)\n", __FUNCTION__, ch);
			return;
			}
		}
#endif
	}
	else if (status == NF_IPCAM_STATUS_TIMEOUT || status == NF_IPCAM_STATUS_FAILED_REQ)
	{
		// image set fail�ϴ��� support�� �����ϰ� continue
		if(type == NF_IPCAM_TYPE_SET_IMAGE_ONVIF)
		{
			IPCAM_DBG(WARN, "SET IMAGE callback(fail)\n");
			onvif_initial_setup_motion_area(ch);
		}
		else
		{
			IPX_PND_EVENT *evt;
			if (nf_get_running_mode())
			{
				//if (nf_openmode_get_live())
				{
					runtime[ch].state = OPENMODE_STATE_CONFIG_FAIL;
				}
			}
			else
			{
				nf_pnd_queue_push(ch, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
			}
			IPCAM_DBG(WARN, "Setup failed(CH(%d))\n", ch);
			return;
		}
	}
	else
	{
		return;
	}
}

/**
 * @brief 스트림 설정의 max fps 및 ntsc/pal값으로 설정가능한 fps bitmask를 조회한다.
 * @param fps Max fps.
 * @param is_ntsc NTSC 여부.
 * @return 설정가능한 fps bitmask.
 */
static int _get_capable_fps_value(int fps, int is_ntsc)
{
	switch(fps)
	{
		case 1:
			return NF_IPCAM_FPS_10;
		case 2:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20;
		case 3:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30;
		case 6:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? 0 : NF_IPCAM_FPS_60);
		case 7:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? NF_IPCAM_FPS_70 : NF_IPCAM_FPS_60);
		case 12:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? NF_IPCAM_FPS_70 : NF_IPCAM_FPS_60 | NF_IPCAM_FPS_120);
		case 15:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? NF_IPCAM_FPS_70 | NF_IPCAM_FPS_150 : NF_IPCAM_FPS_60 | NF_IPCAM_FPS_120);
		case 25:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? NF_IPCAM_FPS_70 | NF_IPCAM_FPS_150 : NF_IPCAM_FPS_60 | NF_IPCAM_FPS_120 | NF_IPCAM_FPS_250);
		case 30:
			return NF_IPCAM_FPS_10 | NF_IPCAM_FPS_20 | NF_IPCAM_FPS_30 |
					(is_ntsc ? NF_IPCAM_FPS_70 | NF_IPCAM_FPS_150 | NF_IPCAM_FPS_300 : NF_IPCAM_FPS_60 | NF_IPCAM_FPS_120 | NF_IPCAM_FPS_250);
		default:
			return 0;
	}
}

/**
 * @brief 비디오 스트림 max fps를 통해 fps support 및 현재값을 설정한다.
 * @param ch 채널 번호.
 * @param max_fps Max fps.
 * @param stream_no 스트림 번호(1st, 2nd).
 */
static void _build_fps_table(int ch, int max_fps, int stream_no)
{
	mtable *runtime = get_runtime();
	if(runtime == NULL) return;
	video_t *vid = &runtime[ch].video;
	int adjust_fps;

	if(max_fps < 2)
	{
		adjust_fps = 1;
	}
	else if(max_fps < 3)
	{
		adjust_fps = 2;
	}
	else if(max_fps < 6)
	{
		adjust_fps = 3;
	}
	else if(max_fps < 7)
	{
		adjust_fps = 6;
	}
	else if(max_fps < 12)
	{
		adjust_fps = 7;
	}
	else if(max_fps < 15)
	{
		adjust_fps = 12;
	}
	else if(max_fps < 25)
	{
		adjust_fps = 15;
	}
	else if(max_fps < 30)
	{
		adjust_fps = 25;
	}
	else
	{
		adjust_fps = 30;
	}
	vid->fps[0][stream_no].support = _get_capable_fps_value(adjust_fps, 1);
	vid->fps[1][stream_no].support = _get_capable_fps_value(adjust_fps, 0);

	/*
	 * if support 'onthefly', change it with record setup.
	 * if not support 'onthefly', change it with stream setup.
	 */
	if(!nf_ipcam_is_config_changable(ch))
	{		
		vid->fps[0][stream_no].value = _get_max_fps_mask(max_fps, vid->fps[0][stream_no].support);
		vid->fps[1][stream_no].value = _get_max_fps_mask(max_fps, vid->fps[1][stream_no].support);;
	}

	if(strcmp(runtime[ch].sys.vendor, "H264") == 0)
	{
		runtime[ch].video.fps[0][0].support &= ~(NF_IPCAM_FPS_30|NF_IPCAM_FPS_20);
		runtime[ch].video.fps[0][1].support &= ~(NF_IPCAM_FPS_30|NF_IPCAM_FPS_20);
		runtime[ch].video.fps[1][0].support &= ~(NF_IPCAM_FPS_30|NF_IPCAM_FPS_20);
		runtime[ch].video.fps[1][1].support &= ~(NF_IPCAM_FPS_30|NF_IPCAM_FPS_20);
	}
}

#endif	// __NF_IPCAM_SETUP_C__
