/* set tabspace4 */
/**
 * @file nf_api_ipcam.c
 * @brief IP카메라 관련 외부 API.
 * @author Jae-young Kim
 * @date 03/03/2011
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_API_IPCAM_C__
#define __NF_API_IPCAM_C__

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <dispmux.h>
#include <gobj.h>
#include <gobjmrtppipe.h>
#include <nf_codec_header.h>
#include <nf_api_ipcam.h>
#include <nf_api_openmode.h>
#include <nf_ipcam_defs.h>
#include <nf_ptz.h>
#include <nf_util_device.h>

#include <nfdal.h>
#include <nf_record.h>
#include "ivca_def.h"
#include "nf_ipcam_driver_axis.h"
#include "nf_va_object_detector.h"
#include "nf_api_live.h"
#include "nf_meta_data.h"

#include <nf_api_eventlog.h>
#include <nf_logevtdef.h>

#include "nf_ipcam_utils.h"
#include <curl/curl.h>
#include "nf_ipcam_zmq_utils.h"
#include "nf_api_dlva.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ipcam"

/** @def NUM_IPX_CHANNEL
 *  @brief NVR의 최대 채널수(모델 무관).
 */
#define NUM_IPX_CHANNEL	AVAILABLE_MAX_CH
#define ONVIF_PRESET_MAX (239)

/** @def USE_PND
 *  @brief (CCTV 모드에서)IP카메라 접속 루틴 사용여부.
 *  KMW 프로젝트에서 OPEN모드 미구현시 대용으로 개발함.
 */
#if defined(ENABLE_PROJECT_KMW)||defined(ENABLE_PROJECT_KUMMI)
	#define USE_PND				0
#else
	#define USE_PND				1
#endif

#define AI_META_MAX 7
#define AI_COUNTER_MAX 16

/** @var use_multi_switch
 *  @brief IPX Pro 멀티 스위치 사용여부(S1향 = 1).
 *  IPX Pro 기본향 및 S1향 세트 공용화를 위한 변수.
 */
/** @var is_custom_mode
 *  @brief CUSTOM모드(CONFIGURABLE CCTV MODE) ON/OFF 구분 변수.
 */
/** @var is_open_mode
 *  @brief CCTV / OPEN모드 구분 변수.
 */
static gint use_multi_switch = 0;
static gint is_open_mode = 0;
static gint is_custom_mode = 0;
static gint support_static_ip_onvif_cam = 1;

/** @var _ipcam_mraw_ch
 *  @brief 현재 모션 raw data notify를 보내줘야 할 채널번호.
 */
/** @var _h_mrtp_pipe
 *  @brief GobjMrtpPipe 핸들.
 */
/** @var _port_status
 *  @brief 현재 채널별 연결상태. UI에서 사용.
 */
static gint _ipcam_mraw_ch = 0xff;
static GobjMrtpPipe *_h_mrtp_pipe = NULL;
static NFIPCamPortStatus _port_status[AVAILABLE_MAX_CH];

static void nf_ipcam_header_x_callback(gint ch, guchar *payload, gint len, NMFMrtpPipeHeaderX *rtn_hx, gpointer user_data);
static void nf_ipcam_onvif_metadata_callback(gint ch, guchar* payload, gint len, gpointer user_data);

static void _api_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _api_ipcam_tmp_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _api_sysdb_ipcam_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

static int _find_version_number(const char* version_str, int *out_pcode);
static int _is_valid_fw_name(const char* filename, int* out_pcode, int is_hisilicon);
static int _get_stream_ratio(int iw, int ih,  int *ow, int *oh);

static int _is_updated_ai_rule_engine(int ch);
static int _is_updated_embedded_osd(int ch);
static int _is_updated_dl_option(int ch);

extern void set_running_state(unsigned int val);
extern void nf_openmode_set_state(int state);
extern mtable* get_runtime(void);
//extern profile* get_model_db(void);
//extern void cam_setup_setcb(int, int, NF_IPCAM_SETUP_TYPE_E, NFIPCamSetupCallback, gpointer);
extern int plug_and_detect_init(GobjMrtpPipe *obj, int channel_num);
extern int no_pnd_init(GobjMrtpPipe *obj);
//extern unsigned int *get_vloss_status_ptr(void);
//extern void hub_poe_reboot(int ch);
extern void _get_rect_points(int,int,char*,NF_IPCAM_POINT*,NF_IPCAM_POINT*);
extern void hub_camfwup_request();
extern void nf_ipcam_init_switch_device(void);

extern void init_aibox_conn_state();

extern int itx_cam_get_ai_rule_engine_and_save_db(int ch);
extern int itx_cam_get_dl_option_and_save_db(int ch);
extern int itx_cam_get_embedded_osd_and_save_db(int ch);
extern void itx_cam_set_ai_rule_engine(void *arg);
extern void itx_cam_set_ai_dl_option(int ch);
extern void itx_cam_set_embedded_osd(int ch);
extern int nf_ipcam_get_ai_rule_engine(int ch);
extern int nf_ipcam_get_embedded_osd(int ch);

extern int nf_ipcam_get_max_resol_ratio(int ch);

typedef struct __NFIPCamUpThreadParam
{
	gint new_ch_mask;
	gchar file_path[255];
	gchar* file_stream;
	gint file_len;
} _NFIPCamUpThreadParam;
static void _nf_ipcam_upgrade_thread_func(_NFIPCamUpThreadParam *param);
static void _nf_ipcam_upgrade_thread_func_one(_NFIPCamUpThreadParam *param);

static int view_ipcam_option(NFIPCamOption_onvif *option);

static int nf_ipcam_get_nvr_eth0_ip(char* ip_str);
void nf_ipcam_zmq_server_start();
void nf_ipcam_zmq_server_stop();

extern void _convert_pmask_area_by_corridor_view(int ch, NFIPCamPrivacyMask *param);
extern void _convert_roi_area_by_corridor_view(int ch, NFIPCamSetupROIArea *param);

/* ssl mutex setup functions - begin */
static MUTEX_TYPE *nf_ipcam_ssl_mutex_buf=NULL;

static void nf_ipcam_ssl_locking_function(int mode, int n, const char * file, int line)
{
	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(nf_ipcam_ssl_mutex_buf[n]);
	else
		MUTEX_UNLOCK(nf_ipcam_ssl_mutex_buf[n]);
}

static unsigned long nf_ipcam_ssl_id_function(void)
{
	return ((unsigned long)THREAD_ID);
}

extern int nf_openssl_thread_setup(void)
{
	int i;

	nf_ipcam_ssl_mutex_buf = malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!nf_ipcam_ssl_mutex_buf)
		return 0;
	for (i = 0;  i < CRYPTO_num_locks(  );  i++)
		MUTEX_SETUP(nf_ipcam_ssl_mutex_buf[i]);
	CRYPTO_set_id_callback(nf_ipcam_ssl_id_function);
	CRYPTO_set_locking_callback(nf_ipcam_ssl_locking_function);
	return 1;
}

extern int nf_openssl_thread_cleanup(void)
{
	int i;
	if (!nf_ipcam_ssl_mutex_buf)
		return 0;
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	for (i = 0;  i < CRYPTO_num_locks(  );  i++)
		MUTEX_CLEANUP(nf_ipcam_ssl_mutex_buf[i]);
	free(nf_ipcam_ssl_mutex_buf);
	nf_ipcam_ssl_mutex_buf = NULL;
	return 1;
}
/* ssl mutex setup functions - end */


/**
 * @brief IPCAM 전역 변수등을 초기화하고 카메라 연결 루틴을 초기화한다.
 */
void nf_ipcam_init(void)
{
	gint8 cmd[200];
	IPCAM_DBG(MAJOR, "start\n");

	IPCAM_DBG(MINOR, "Switch status %s\n", (use_multi_switch ? "MULTI":"SINGLE"));

	_h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
	g_return_if_fail(_h_mrtp_pipe != NULL);
	memset(_port_status, 0x00, sizeof(NFIPCamPortStatus) * AVAILABLE_MAX_CH);
	nmf_mrtp_pipe_set_header_x_callback(_h_mrtp_pipe, &nf_ipcam_header_x_callback, NULL);
	nmf_mrtp_pipe_set_onvif_meta_callback(_h_mrtp_pipe, &nf_ipcam_onvif_metadata_callback, NULL);

	nf_ipcam_packet_dump_init();

	nf_notify_connect_cb("sysdb_change", _api_sysdb_reload_cb_func, NULL);
	nf_notify_connect_cb("sysdb_tmp_change", _api_ipcam_tmp_reload_cb_func, NULL);
	nf_notify_connect_cb("sysdb_ipcam_change", _api_sysdb_ipcam_reload_cb_func, NULL);
	nf_notify_connect_cb("sysdb_change", nf_api_aibox_url_change_cb_func, NULL);

	nf_notify_fire_params("ipcam_fwver_warn", 0, 0, 0, 0);
	nf_notify_fire_params("vloss", 0xffffffff, 0,0,0);

	//if (nf_ipcam_is_vendor_vicon())
	//{
	//	support_static_ip_onvif_cam = 1;
	//}

	sprintf(cmd, "cp -af %s %s", "/etc/dibbler/client_eth0.conf", "/etc/dibbler/client.conf");
	proxy_system(cmd, 1, 3);

	if (is_open_mode == 0)
	{
		nf_ipcam_init_poe_off_time();
#if USE_PND
		plug_and_detect_init(_h_mrtp_pipe, NUM_IPX_CHANNEL);
#else
		no_pnd_init(_h_mrtp_pipe);
#endif // NO_PND
	}
	else
	{
		nf_openmode_init();
	}

	nf_ipcam_write_maps();
	nf_ipcam_boost_priority();

	IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 마무리 작업으로 전역 변수들을 초기화한다.
 */
void nf_ipcam_finalize(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	_h_mrtp_pipe = NULL;
	memset(_port_status, 0x00, sizeof(NFIPCamPortStatus) * AVAILABLE_MAX_CH);
	IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 카메라 연결 루틴을 시작한다.
 */
void nf_ipcam_start(void)
{
	int i;
	int rtn;
	IPCAM_DBG(MAJOR, "start\n");

	/* moved from make_db.sh - can't change default value of cam category. */
	set_vendor_default_db();

	if (nf_get_running_mode() == 0)
	{
		IPCAM_DBG(MINOR, "CCTV mode\n");
		vhub_set_data_clear();
		vhub_set_data_rebuild();
		set_running_state(DISCOVERY_RUNNING);
	}
	else
	{
		IPCAM_DBG(MINOR, "Open-NVR mode\n");
		nf_openmode_start();
	}

    init_aibox_conn_state();

	nf_ipcam_zmq_server_start();

    nf_api_aibox_rule_update_all();
    aibox_configure_status_update_all();
	nf_api_load_host_info_list();
    nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, 0, 0xFFFFFFFF, 0);
	IPCAM_DBG(MAJOR, "end\n");
}
/**
 * @brief 카메라 연결 루틴을 종료한다.
 * 또한 관련 runtime 변수 등을 초기화한다.
 */
void nf_ipcam_stop(void)
{
	int i = 0;
	mtable *runtime = NULL;
	dtable *discovery = NULL;
	GAsyncQueue *event_queue = NULL;
	GAsyncQueue *vloss_queue = get_vloss_queue();
	GobjMrtpSrc *mrtpsrc = gst_mrtp_src_get_object();


	IPCAM_DBG(MAJOR, "start\n");

	if (nf_get_running_mode() == 0)
	{
		g_return_if_fail( _h_mrtp_pipe != NULL );

		runtime = get_runtime();
		discovery = get_dtable();
		g_return_if_fail( runtime != NULL );

		event_queue = get_queue();
		g_return_if_fail( event_queue != NULL );

		set_running_state(DISCOVERY_STOPPED);
#if 0
		while (is_setup_th_ready_to_stop() == 0)
		{
			sleep(1);
		}
#endif
		cam_setup_restart();
		gst_mrtp_src_close_all(mrtpsrc,NULL);
		pnd_init_status();
		memset(discovery, 0x00, sizeof(dtable)*AVAILABLE_MAX_CH);
		for (i = 0; i < NUM_ACTIVE_CH; i++)
		{
#if 0
			IPX_PND_EVENT *new_event = NULL;

			if ((runtime[i].state &
				(MGMT_STATE_LINKED|MGMT_STATE_READY|MGMT_STATE_CONFIGURED)) == 0)
			{
				continue;
			}

			nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
			runtime[i].state = MGMT_STATE_UNLINKED;
			runtime[i].sys.transaction = 0;
			runtime[i].sys.ipaddr = 0;
			memset(runtime[i].sys.macaddr, 0x00, 6);
#else
			runtime[i].state = MGMT_STATE_UNLINKED;
			runtime[i].sys.transaction = 0;
			runtime[i].sys.ipaddr = 0;
			memset(runtime[i].sys.macaddr, 0x00, 6);
#endif
		}

		{
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
			g_async_queue_push(vloss_queue, evt);
		}
	}
	else
	{
#if 0
		g_return_if_fail( _h_mrtp_pipe != NULL );

		runtime = get_runtime();
		g_return_if_fail( runtime != NULL );

		for (i = 0; i < NUM_ACTIVE_CH; i++)
		{
			nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, i);
			IPCAM_DBG(MINOR, "[CH%d] MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
					i, runtime[i].state);
			runtime[i].state = MGMT_STATE_UNLINKED;
			runtime[i].sys.transaction = 0;
			runtime[i].sys.ipaddr = 0;
			memset(runtime[i].sys.macaddr, 0x00, 6);
		}

		nf_openmode_stop();
		{
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
			g_async_queue_push(vloss_queue, evt);
		}
#endif
		nf_openmode_stop();
	}
	nf_ipcam_zmq_server_stop();

	IPCAM_DBG(MAJOR, "end vloss_state set(0000ffff)\n");
}

/**
* booting mode(installation mode) setting
*/
void nf_set_installation_mode(void)
{
	is_custom_mode = nf_sysdb_get_bool("cam.install.dual_lan");
	is_open_mode = nf_sysdb_get_bool("cam.install.mode");
}

/**
 * @brief 현재 CUSTOM모드(CONFIGURABLE CCTV MODE) 상태를 조회한다.
 * @return 0 - NOT CUSTOM, 1 - CUSTOM MODE.
 */
gboolean nf_get_custom_mode(void)
{
	return is_custom_mode;
}

/**
 * @brief 현재 CCTV/OPEN모드 상태를 조회한다.
 * @return 0 - CCTV, 1 - OPEN모드.
 */
gint nf_get_running_mode(void)
{
	return is_open_mode;
}

/**
 * @brief 해당 채널 카메라의 접속을 끊는다(일시정지) - Don't use.
 * @param ch 채널 번호.
 */
void nf_ipcam_pause_ch(int ch)
{
	mtable *runtime = get_runtime();
	GAsyncQueue *vloss_queue = get_vloss_queue();
	unsigned int vloss_status = 0;


	IPCAM_DBG(MAJOR, "start\n");
	g_return_if_fail( _h_mrtp_pipe != NULL );
	g_return_if_fail( runtime != NULL );
	
	{
		IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
		evt->port = ch;
		g_async_queue_push(vloss_queue, evt);
	}
	nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, ch);
	IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 일시정지한 카메라 연결을 다시 재생한다.
 * @param ch 채널 번호.
 * @deprecated Pause 및 Resume 기능을 사용하지 않음.
 */
void nf_ipcam_resume_ch(int ch)
{
	mtable *runtime = get_runtime();

	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_if_fail( _h_mrtp_pipe != NULL );
	g_return_if_fail( runtime != NULL );

	/* Open channel */
	{
		NMFMrtpPipeChannel info;

		memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

		info.ch_num = ch;
		info.model_code = (guint)runtime[ch].sys.model_code;
		info.username = &runtime[ch].username[0];
		info.password = &runtime[ch].password[0];
		info.video_cnt = 2;
		info.video[0].resolution = RES_1920x1080;
		info.video[0].ip_addr = runtime[ch].sys.ipaddr;
		info.video[0].rtsp_port = runtime[ch].sys.rtsp_port[0];
		info.video[0].rtsp_addr = runtime[ch].sys.rtsp_url[0];
		info.video[1].resolution = RES_640x360;
		info.video[1].ip_addr = runtime[ch].sys.ipaddr;
		info.video[1].rtsp_port = runtime[ch].sys.rtsp_port[1];
		info.video[1].rtsp_addr = runtime[ch].sys.rtsp_url[1];

		if (runtime[ch].audio.audio_tx)
		{
			info.audio_cnt = 1;
			info.audio.resolution = 0;
			info.audio.ip_addr = runtime[ch].sys.ipaddr;
			info.audio.rtsp_port = runtime[ch].sys.rtsp_port[0];
			info.audio.rtsp_addr = runtime[ch].sys.rtsp_url[0];
		}

		nmf_mrtp_pipe_set_dev_mac(_h_mrtp_pipe, ch, &runtime[ch].sys.macaddr[0]);
		nmf_mrtp_pipe_open_ch(_h_mrtp_pipe, &info);
	}
	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
}

/**
 * @brief 카메라 연결 루틴을 재시작한다.
 * NVR IP 변경시 수행된다.
 */
void nf_ipcam_ip_changed(void)
{
	IPCAM_DBG(MAJOR, "start\n");

	if (nf_get_running_mode() == 0)
	{
		nf_ipcam_stop();
		plug_and_detect_restart();
		set_running_state(DISCOVERY_RUNNING);
	}
	else
	{
		nf_openmode_start();		
	}
	IPCAM_DBG(MAJOR, "end\n");
}

static int _ipcam_upgrade_canceled = 0;

void nf_ipcam_cancel_upgrade_by_ui(void)
{
	_ipcam_upgrade_canceled = 1;
}
int nf_ipcam_is_upgrade_canceled(void)
{
	return _ipcam_upgrade_canceled;
}

/**
 * @brief 모션 raw data를 보여줄 채널을 설정한다.
 * @param ch 채널 번호.
 *
 * UI 모션설정 페이지에서 사용한다.
 */
void nf_ipcam_set_mraw_ch(gint ch)
{
	_ipcam_mraw_ch = ch;
}

/**
 * @brief 모션 raw data를 보여줄 채널을 조회한다.
 * @return 현재 모션 raw data 설정채널.
 */
gint nf_ipcam_get_mraw_ch(void)
{
	return _ipcam_mraw_ch;
}

/**
 * @brief 현재 멀티 스위치 사용 여부를 반환한다.
 * @return 멀티 스위치 사용여부.
 */
int nf_ipcam_using_multi_switch(void)
{
	return use_multi_switch;
}

typedef struct __PND_OSD_T
{
	NF_IPCAM_PND_OSD_E status;
	char *t_str;
} _PND_OSD_T;
#if 0
static NF_IPCAM_PND_OSD_E _cur_osd_status[16];
static _PND_OSD_T pnd_osd_status[PND_OSD_MAX] = {
	{ PND_OSD_NONE,			"NONE"         },
	{ PND_OSD_DETECT,		"DEVICE DETECT"},
	{ PND_OSD_DHCP,			"SETTING DHCP" },
	{ PND_OSD_IPREQUEST,	"SETTING IP"   },
	{ PND_OSD_STREAM,		"REQ STREAM"   },

	{ PND_OSD_MODEL,		"MODEL"        },
	{ PND_OSD_CONFIG_INIT,	"CONFIG INIT"  },
	{ PND_OSD_CONFIG_ALARM,	"CONFIG ALARM" },
	{ PND_OSD_CONFIG_VIDEO,	"CONFIG VIDEO" },
	{ PND_OSD_CONFIG_AUDIO,	"CONFIG AUDIO" },
	{ PND_OSD_CONFIG_IMAGE,	"CONFIG IMAGE" },
	{ PND_OSD_CONFIG_PMASK,	"CONFIG PMASK" },
	{ PND_OSD_CONFIG_MOTION,"CONFIG MOTION"},
	{ PND_OSD_CONFIG_VCA,	"CONFIG VCA"   },

	{ PND_OSD_CONFIG_REBOOT,"REBOOTING"    },

	{ PND_OSD_TIMEOUT,		"TIMEOUT"      },
	{ PND_OSD_CONFIG_FAIL,	"CONFIG FAIL"  },
	{ PND_OSD_STREAM_FAIL,	"STREAM FAIL"  }
};

/**
 * @brief Initialize ipcam discovery osd status.
 * @deprecated
 */
void nf_ipcam_init_pnd_osd_status(void)
{
	memset(_cur_osd_status, 0x00, sizeof(NF_IPCAM_PND_OSD_E)*16);
}

/**
 * @brief Get ipcam discovery osd status.
 * @param ch Request channel.
 * @return Current discovery osd status.
 * @deprecated
 */
NF_IPCAM_PND_OSD_E nf_ipcam_get_pnd_osd_status(gint ch)
{
	return (_cur_osd_status[ch]);
}
#endif
/**
 * @brief 카메라 연결 상태 OSD를 설정한다.
 * @param ch 채널 번호.
 * @param status 현재 연결 상태.
 * @deprecated 현재 사용 안함.
 */
void nf_ipcam_set_pnd_osd_status(gint ch, NF_IPCAM_PND_OSD_E status)
{
#if 0
	IPCAM_DBG(MINOR, "CH(%d) OSD(%s)\n", ch, pnd_osd_status[status].t_str);
	_cur_osd_status[ch] = status;
	nf_notify_fire_params("pnd_osd", status, ch, 0, 0);
#endif
}

/**
 * @brief 허브에 연결된 카메라를 POE 리붓시킨다(PRO 16채널 전용).
 *
 * IPX PRO의 경우 세트가 리셋될 때 0~7채널은 전원이 나가며 카메라가 리셋되지만
 * 허브에 연결된 채널들은 그런거 없다. 따라서 S1 factory clear 시나리오 등 문제의 소지가 있어
 * 허브 채널도 리셋을 해줌.
 */
void nf_ipcam_opmode_change(void)
{
	int i=0;
	IPCAM_DBG(MAJOR, "start\n");
#if defined(GUI_32CH_SUPPORT) || defined(GUI_16CH_SUPPORT)
	for (i=8; i<AVAILABLE_MAX_CH; i++)
	{
		nf_ipcam_poe_reboot(i, NULL,NULL,NULL);
	}
#endif
	IPCAM_DBG(MAJOR, "end\n");
}


/**
 * @brief Stream setup 메뉴에 사용할 encoder 정보를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] info Encoder 정보.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_encoder_capability(gint ch, NFIPCamEncoderCap* info)
{
	gint i;
	guint __fps;
	mtable *runtime = get_runtime();
	char key[64];
	int is_vcam;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	if (ch < 0 || ch >= NUM_IPX_CHANNEL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		memset(info, 0x00, sizeof(NFIPCamEncoderCap));
		info->ch = ch;
		info->encoder_cnt = 0;

		return IPCAM_SETUP_RTN_DONE;
	}

	snprintf(key, 63, "cam.logininfo.L%d.vcam", ch);
	is_vcam = nf_sysdb_get_uint(key);

	if(is_vcam)
	{
		memset(info, 0x00, sizeof(NFIPCamEncoderCap));
		info->ch = ch;
		info->encoder_cnt = 0;

		return IPCAM_SETUP_RTN_DONE;
	}


	if (strcmp(runtime[ch].sys.vendor, "H264") == 0)
	{
		info->ch = ch;
		info->encoder_cnt = 0;
		return IPCAM_SETUP_RTN_DONE;
	}

	memset(info, 0x00, sizeof(NFIPCamEncoderCap));
	info->ch = ch;
	info->encoder_cnt = runtime[ch].video.stream_cnt;
	if (info->encoder_cnt < 0 || info->encoder_cnt > 3)
	{
		IPCAM_DBG(ERROR, "encoder count(%d)\n", info->encoder_cnt);
		info->ch = ch;
		info->encoder_cnt = 0;
		return IPCAM_SETUP_RTN_FAILED;
	}

	for (i=0; i<info->encoder_cnt; i++)
	{
		info->res_support[i] = runtime[ch].encoder.res_support[i];
		info->fps_max[i] = runtime[ch].encoder.max_framerate[i];
		info->bitrate_max[i] = runtime[ch].encoder.max_bitrate[i];
		info->bitrate_min[i] = runtime[ch].encoder.min_bitrate[i];
		info->bitctrl_support[i] = runtime[ch].encoder.bitctrl[i];
		info->vcodec_support[i] = runtime[ch].encoder.vcodec[i];
		info->res_default[i] = runtime[ch].video.resolution.resolution[i];
		if ((info->res_support[i] & runtime[ch].video.resolution.resolution[i]) == 0)
		{
			runtime[ch].encoder.res_support[i] |= info->res_default[i];
			info->res_support[i] |= info->res_default[i];
		}
		{
			//char key[64];

			snprintf(key, 64, "cam.C%d.stream.S%d.fps", ch, i);
			__fps = nf_sysdb_get_uint(key);
			if(__fps > info->fps_max[i] && info->fps_max[i] > 0) 
			{
				__fps = info->fps_max[i];
			}

		}
		info->fps_max_default[i] = __fps;
		info->br_min_default[i] = runtime[ch].video.quality[i][NF_IPCAM_QUALITY_LOW];
		info->br_max_default[i] = runtime[ch].video.quality[i][NF_IPCAM_QUALITY_SUPER];
		info->bitctrl_default[i] = runtime[ch].video.bitctrl[i];
		info->vcodec_default[i] = runtime[ch].video.vcodec[i];
	}

#if 0
	info->res_support[1] = runtime[ch].encoder.res_support[1];
	info->fps_max[1] = runtime[ch].encoder.max_framerate[1];
	info->bitrate_max[1] = runtime[ch].encoder.max_bitrate[1];
	info->bitrate_min[1] = runtime[ch].encoder.min_bitrate[1];

	info->res_default[1] = runtime[ch].video.resolution.resolution[1];
	info->fps_max_default[1] = 30;
	info->br_min_default[1] = runtime[ch].video.quality[1][NF_IPCAM_QUALITY_LOW];
	info->br_max_default[1] = runtime[ch].video.quality[1][NF_IPCAM_QUALITY_SUPER];
#endif

	IPCAM_DBG(MAJOR, "end(%08x:%08x %d:%d %d:%d %d:%d, %08x:%08x %d:%d %d:%d %d:%d\n",
			info->res_support[0], info->res_default[0],
			info->fps_max[0], info->fps_max_default[0],
			info->bitrate_max[0], info->br_max_default[0],
			info->bitrate_min[0], info->br_min_default[0],
			info->res_support[1], info->res_default[1],
			info->fps_max[1], info->fps_max_default[1],
			info->bitrate_max[1], info->br_max_default[1],
			info->bitrate_min[1], info->br_min_default[1]
	);

	return IPCAM_SETUP_RTN_DONE;
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
		case 'D':
		case 'E': { rtn = NF_IPCAM_RES_704x480; break; }
		case 'F': { rtn = NF_IPCAM_RES_352x288; break; }
		//case 'G':
		case 'H':
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
		default:  { rtn = 0; break; }
	}

	return rtn;
}

/**
 *  * @brief Stream setup 메뉴에 사용할 Resolution 별MAX FPS 정보를 조회한다.
 *   * @param[in] ch- 채널 번호, resolution- 해상도info
 *    * @param[out] fps- MAX FPS 정보.
 *     * @return SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *      */
int nf_ipcam_get_max_fps_information(gint ch, char resolution, unsigned int *fps)
{
	mtable *runtime = get_runtime();

	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	int stream_cnt = 0;
	int onvif_fps = 0;
	uint64_t ret_resol;
	int sdkver[4] = {0, };
	int is_pal = nf_sysdb_get_bool("sys.info.sig_type");

	sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &sdkver[0], &sdkver[1], &sdkver[2], &sdkver[3]);

	ret_resol = _change_szdb_to_camgr(resolution);

	if(ret_resol == NF_IPCAM_RES_2592x1520 && runtime[ch].sys.model_code < NF_IPCAM_MODEL_ONVIF)
	{
		if(strncmp(runtime[ch].sys.swver, "48100", 5) == 0)
		{
			if(sdkver[1] == ITX_CAM_SDK_TYPE_HS && sdkver[3] > 8)
				*fps = 25;
			else
				*fps = 20;
		}
		else *fps = 30;
	}
	else
		return IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "CH(%d) return max fps(%d) \n", ch, *fps);
	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 VA설정 xml을 가져온다.
 * @param[out] p VA xml.
 * @param[in] chan 채널 번호.
 * @return none.
 */
int nf_ipcam_va_config_xml_export(char* p, int chan, int is_modified)
{
	__encap_va_xml_export(p,chan,is_modified);
}
/**
 * @brief VA 설정을 저장한다.
 * @param[in] p VA 설정 xml.
 * @param[in] chan 채널번호.
 * @return none.
 */
int nf_ipcam_va_config_xml_import(char* p, int chan)
{
	__encap_va_xml_import(p,chan);
}

/**
 * @brief 현재 VA설정 xml을 가져온다.
 * @param[out] p VA xml.
 * @param[in] chan 채널 번호.
 * @return none.
 */
int nf_ipcam_ai_config_xml_export(char* p, int chan, int is_modified)
{
	__encap_ai_xml_export(p,chan,is_modified);
}
/**
 * @brief VA 설정을 저장한다.
 * @param[in] p VA 설정 xml.
 * @param[in] chan 채널번호.
 * @return none.
 */
int nf_ipcam_ai_config_xml_import(char* p, int chan)
{
	__encap_ai_xml_import(p,chan);
}

/**
 * @brief 숫자로 된 fps로 NF_IPCAM_FPS_E enum값을 반환한다.
 * @param fps FPS.
 * @return NF_IPCAM_FPS_E.
 */
extern gint _ipcam_convert_fps (gint fps)
{
	gint ret = NF_IPCAM_FPS_300;
	
	switch ( fps )
	{
		case 300: ret = NF_IPCAM_FPS_300; break;
		case 250: ret = NF_IPCAM_FPS_250; break;
		case 150: ret = NF_IPCAM_FPS_150; break;
		case 120: ret = NF_IPCAM_FPS_120; break;
		case 100: ret = NF_IPCAM_FPS_100; break;
		case  70: ret = NF_IPCAM_FPS_70;  break;
		case  60: ret = NF_IPCAM_FPS_60;  break;
		case  40: ret = NF_IPCAM_FPS_40;  break;
		case  30: ret = NF_IPCAM_FPS_30;  break;
		case  20: ret = NF_IPCAM_FPS_20;  break;
		case  10: ret = NF_IPCAM_FPS_10;  break;	
		default: ret = NF_IPCAM_FPS_300; break;
	}
	
	return ret;			
}

/**
 * @brief 현재 카메라 채널의 접속 상태를 설정한다.
 * @param ch 채널 번호.
 * @param port_status 채널 상태 struct.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 파라메터 오류.
 *
 * UI에서 보여주기 용도로 사용.
 */
int nf_ipcam_set_port_status(gint ch, NFIPCamPortStatus* port_status, GError **error)
{
	g_return_val_if_fail(ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch < AVAILABLE_MAX_CH, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(port_status != NULL, IPCAM_SETUP_RTN_FAILED);

	_port_status[ch].status = port_status->status;
	_port_status[ch].device_class = port_status->device_class;
	snprintf(_port_status[ch].vendor, 64, port_status->vendor);
	snprintf(_port_status[ch].model, 64, port_status->model);
	snprintf(_port_status[ch].detail, 256, port_status->detail);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 'I frame only' 설정을 mrtp library 로 보낸다.
 * @param ch_mask 채널 마스크.
 * @deprecated 현재 사용안하는 기능.
 */
void nf_ipcam_req_iframe_only(guint ch_mask)
{
	IPCAM_DBG(MAJOR, "start\n");
	g_return_if_fail( _h_mrtp_pipe != NULL);

	nmf_mrtp_pipe_i_only_req(_h_mrtp_pipe, ch_mask);
	IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 모션 콜백 함수를 mrtp library에 등록한다.
 * @param cb_func 콜백 함수.
 * @param user_data 사용자 데이타.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 파라메터 오류.
 */
int nf_ipcam_set_motion_callback
(NFIPCamMotionCallback* cb_func, gpointer user_data, GError **error)
{
	GobjMrtpPipe *h_mrtp_pipe = NULL;

	h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();

	g_return_val_if_fail(h_mrtp_pipe != NULL, IPCAM_SETUP_RTN_FAILED);

	nmf_mrtp_pipe_set_motion_callback(h_mrtp_pipe, (void*)cb_func, user_data);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 알람 콜백 함수를 mrtp library에 등록한다.
 * @param cb_func 콜백 함수.
 * @param user_data 사용자 데이타.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 파라메터 오류.
 */
int nf_ipcam_set_alarm_callback
(NFIPCamAlarmCallback* cb_func, gpointer user_data, GError **error)
{
	GobjMrtpPipe *h_mrtp_pipe = NULL;

	h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();

	g_return_val_if_fail(h_mrtp_pipe != NULL, IPCAM_SETUP_RTN_FAILED);

	nmf_mrtp_pipe_set_alarm_callback(h_mrtp_pipe, (void*)cb_func, user_data);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라에서 지원되는 기능 목록을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] func 지원 기능 비트마스크.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @sa NF_IPCAM_FUNC_E
 */
int nf_ipcam_get_supported_func(gint ch, guint *func, GError **error)
{
	int rtn = IPCAM_SETUP_RTN_DONE;
	mtable* runtime = get_runtime();

	if (func == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (ch >= NUM_IPX_CHANNEL)
	{
		*func = 0;
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (runtime == NULL)
	{
		*func = 0;
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		*func = 0;
		return IPCAM_SETUP_RTN_FAILED;
	}

	*func = runtime[ch].func;

	//IPCAM_DBG(MAJOR, "end CH(%d) functions(%08x)\n", ch, *func);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라가 지원하는 image설정을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] func 지원 image설정 비트마스크.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated Image profile조회로 변경됨.
 * @sa NF_IPCAM_IMAGE_E
 */
int nf_ipcam_get_image_supported_func(gint ch, guint *func, GError **error)
{
	int rtn = 0;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	if (func == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if (ch >= NUM_IPX_CHANNEL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0) { return IPCAM_SETUP_RTN_FAILED; }

	*func = runtime[ch].image.supported;
	//IPCAM_DBG(MAJOR, "end CH(%d) functions(%08x)\n", ch, *func);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 iris기능 지원 여부를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] canIris Iris기능 지원여부.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated Image profile조회로 변경됨.
 */
int nf_ipcam_get_iris_supported(gint ch, gboolean *canIris)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( canIris != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	if((runtime[ch].image.supported & NF_IPCAM_IMAGE_PIRIS)
	&& (runtime[ch].image.exposure.value == NF_IPCAM_IMAGE_EXP_MODE_MANUAL || runtime[ch].image.iris.value == NF_IPCAM_IMAGE_PIRIS_MANUAL))
	{
		*canIris = 1;
	}
	else
	{
		*canIris = 0;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) canIris(%d)\n", ch, *canIris);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라 PTZ기능 지원여부를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] func 조회하고자 하는 기능. @see NF_IPCAM_IMAGE_E
 * @param[out] support 지원 여부.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_ptz_support(gint ch, guint func, gboolean *support)
{
	mtable* runtime = NULL;
	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( support != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( func != 0 , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		switch(func)
		{
			case NF_IPCAM_IMAGE_PAN:
			//case NF_IPCAM_IMAGE_TILT:
				*support = (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_CONTINUOUS_PANTILT);
				break;

			case NF_IPCAM_IMAGE_ZOOM:
				*support = (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_CONTINUOUS_ZOOM);
				break;

			case NF_IPCAM_IMAGE_FOCUS:
				*support = (runtime[ch].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS);
				break;

			case NF_IPCAM_IMAGE_CALIBRATION:
				*support = (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_GOTOHOME);
				break;

			case NF_IPCAM_IMAGE_ONEPUSH:
				// FIXME this is only for s1 protocol, implement common onvif protocol
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_ONEPUSH);
				break;
			case NF_IPCAM_IMAGE_PIRIS:
				*support = 0;
				break;
			case NF_IPCAM_IMAGE_PRESET:
				*support = (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_PRESET);
				break;
			case NF_IPCAM_IMAGE_AUXILIARY:
				*support = (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_AUXILIARY);
				break;

			default:
				*support = 0;
				break;

		}
	}
	else
	{
		switch(func)
		{
			case NF_IPCAM_IMAGE_ZOOM:
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_ZOOM) || (runtime[ch].ptz.supported & PTZ_SETUP_ZOOM_NONPTZ);
				break;
			case NF_IPCAM_IMAGE_FOCUS:
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_FOCUS) || (runtime[ch].ptz.supported & PTZ_SETUP_FOCUS_NONPTZ);
				break;
			case NF_IPCAM_IMAGE_ONEPUSH:
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_ONEPUSH);
				break;

			case NF_IPCAM_IMAGE_CALIBRATION:
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_CALIBRATION);
				break;

			case NF_IPCAM_IMAGE_PAN:
			//case NF_IPCAM_IMAGE_TILT:
				*support = (runtime[ch].func & NF_IPCAM_FUNC_PTZ);
				break;

			case NF_IPCAM_IMAGE_PIRIS:
				if((runtime[ch].ptz.supported & PTZ_SETUP_IRIS) || (runtime[ch].ptz.supported & PTZ_SETUP_IRIS_NONPTZ))
				{
					unsigned int manual = NF_IPCAM_IMAGE_EXP_MODE_MANUAL | NF_IPCAM_IMAGE_EXP_MODE_MANUAL_I3 | NF_IPCAM_IMAGE_EXP_MODE_MANUAL_INX |
						NF_IPCAM_IMAGE_EXP_MODE_MANUAL_NPT | NF_IPCAM_IMAGE_EXP_MODE_MANUAL_NPT_X10 | NF_IPCAM_IMAGE_EXP_MODE_HI_MANUAL; 
					unsigned int iris = NF_IPCAM_IMAGE_PIRIS_MANUAL;
					*support = (runtime[ch].image.exposure.value & manual) || (runtime[ch].image.iris.value & iris);
				}
				else
				{
					*support = 0;
				}
				break;

			case NF_IPCAM_IMAGE_PRESET:
				*support = (runtime[ch].ptz.supported & PTZ_SETUP_PRESET);
				break;

			case NF_IPCAM_IMAGE_AUXILIARY:
				*support = 0;

			default:
				*support = 0;
				break;

		}
	}
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라 PTZ Compensation기능 지원여부를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] func 조회하고자 하는 기능. @see NF_IPCAM_IMAGE_E
 * @param[out] support 지원 여부.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_focus_profile(gint ch, NFIPCamFocusCompProfile* profile)
{
	int support = 0;
	int value = 0;
	mtable* runtime = NULL;
	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( profile != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	memset(profile, 0x00, sizeof(NFIPCamFocusCompProfile));

	if(runtime[ch].focus.supported & NF_IPCAM_FOCUS_DNN_COMP) {
		profile->supported |= NF_IPCAM_FOCUS_DNN_COMP;
		value = runtime[ch].focus.dnn_comp.value;

		if(value == 0 || value == 1)
			profile->dnn_comp_mode = value;
		else
			profile->dnn_comp_mode = 1;

	}
	else
	{
		profile->dnn_comp_mode = 0;
	}


	if(runtime[ch].focus.supported & NF_IPCAM_FOCUS_TEM_COMP) {
		profile->supported |= NF_IPCAM_FOCUS_TEM_COMP;
		value = runtime[ch].focus.tem_comp.value;

		if(value == 0 || value == 1)
			profile->tem_comp_mode = value;
		else
			profile->tem_comp_mode = 1;
	}
	else
	{
		profile->tem_comp_mode = 0;
	}

	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 image설정 관련 capability struct를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] prof Image profile.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated ONVIF카메라 겸용 UI로 변경되며 2차 변경점부터 사용안함.
 */
int nf_ipcam_get_image_profile(gint ch, NFIPCamImageProfile* prof)
{
	mtable* runtime = NULL;
	unsigned int video_support = 0;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( prof != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	memset(prof, 0x00, sizeof(NFIPCamImageProfile));

	/* Supported functions */
	prof->supported = runtime[ch].image.supported;
	video_support = runtime[ch].video.supported;

	/* TODO. brightness contrast color tint(hue) */

	/* Sharpness */
	if (prof->supported & NF_IPCAM_IMAGE_SHARPNESS)
	{
		prof->sharpness.category = NF_IPCAM_IMAGE_SHARPNESS;
		prof->sharpness.value = runtime[ch].image.sharpness.value;
		prof->sharpness.min = runtime[ch].image.sharpness.min;
		prof->sharpness.max = runtime[ch].image.sharpness.max;
		prof->sharpness.dependent_category = 0;
	}

	if (video_support & VIDEO_SETUP_MIRROR)
	{
		int i=0, cnt=0;
		prof->mirror_cnt = 0;

		for (i=0; i<NF_IPCAM_MIRROR_NR; i++)
		{
			if ((1<<i) & runtime[ch].video.mirror.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_MIRRORING, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->mirror[prof->mirror_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].video.mirror.value)
				{
					prof->mirror[prof->mirror_cnt].selected = 1;
				}
				prof->mirror_cnt++;
			}
		}
	}

	/* Exposure */
	//if (prof->supported & NF_IPCAM_IMAGE_EXPOSURE)
	{
		int i=0, cnt=0;
		prof->exposure_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_EXP_MODE_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.exposure.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_EXPOSURE, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->exposure[prof->exposure_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.exposure.value)
				{
					prof->exposure[prof->exposure_cnt].selected = 1;
				}
				prof->exposure_cnt++;
			}
		}
	}

	/* AGC gain */
	//if (prof->supported & NF_IPCAM_IMAGE_AGC)
	{
		prof->agc.category = NF_IPCAM_IMAGE_AGC;
		prof->agc.value    = runtime[ch].image.agc.value;
		prof->agc.min      = runtime[ch].image.agc.min;
		prof->agc.max      = runtime[ch].image.agc.max;
		prof->agc.dependent_category = NF_IPCAM_IMAGE_EXPOSURE;
	}

	/* e-Shutter speed */
	//if (prof->supported & NF_IPCAM_IMAGE_ESHUTTER)
	{
		prof->eshutter.category = NF_IPCAM_IMAGE_ESHUTTER;
		prof->eshutter.value    = runtime[ch].image.eshutter_speed.value;
		prof->eshutter.min      = runtime[ch].image.eshutter_speed.min;
		prof->eshutter.max      = runtime[ch].image.eshutter_speed.max;
		prof->eshutter.dependent_category = NF_IPCAM_IMAGE_EXPOSURE;
	}

	/* Slow shutter */
	//if (prof->supported & NF_IPCAM_IMAGE_SLOWSHUTTER)
	{
		int i=0, cnt=0;
		prof->slowshutter_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_SLSH_MODE_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.slow_shutter.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_SLOWSHUTTER, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->slowshutter[prof->slowshutter_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.slow_shutter.value)
				{
					prof->slowshutter[prof->slowshutter_cnt].selected = 1;
				}
				prof->slowshutter_cnt++;
			}
		}
	}

	/* Max AGC */
	//if (prof->supported & NF_IPCAM_IMAGE_MAXAGC)
	{
		int i=0, cnt=0;
		prof->maxagc_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_MAGC_MODE_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.max_agc.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_MAXAGC, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->maxagc[prof->maxagc_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.max_agc.value)
				{
					prof->maxagc[prof->maxagc_cnt].selected = 1;
				}
				prof->maxagc_cnt++;
			}
		}
	}

	/* DC-IRIS Control */
	//if (prof->supported & NF_IPCAM_IMAGE_DCIRIS)
	{
		int i=0, cnt=0;
		prof->dc_iris_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_IRIS_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.iris.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_DCIRIS, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->dc_iris[prof->dc_iris_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.iris.value)
				{
					prof->dc_iris[prof->dc_iris_cnt].selected = 1;
				}
				prof->dc_iris_cnt++;
			}
		}
	}

	/* BLC Control */
	//if (prof->supported & NF_IPCAM_IMAGE_BLC)
	{
		int i=0, cnt=0;
		prof->blc_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_BLC_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.blc.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_BLC, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->blc[prof->blc_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.blc.value)
				{
					prof->blc[prof->blc_cnt].selected = 1;
				}
				prof->blc_cnt++;
			}
		}
	}

	/* White Balance Control */
	//if (prof->supported & NF_IPCAM_IMAGE_WB)
	{
		int i=0, cnt=0;
		prof->wb_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_WB_MODE_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.wb.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_WB, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->wb[prof->wb_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.wb.value)
				{
					prof->wb[prof->wb_cnt].selected = 1;
				}
				prof->wb_cnt++;
			}
		}
	}

	/* Manual White Balance Control */
	//if (prof->supported & NF_IPCAM_IMAGE_MWB)
	{
		int i=0, cnt=0;
		prof->mwb_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_MWB_MODE_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.mwb.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_MWB, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->mwb[prof->mwb_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.mwb.value)
				{
					prof->mwb[prof->mwb_cnt].selected = 1;
				}
				prof->mwb_cnt++;
			}
		}
	}

	/* Day and Night Control */
	//if (prof->supported & NF_IPCAM_IMAGE_DNN)
	{
		int i=0, cnt=0;
		prof->dnn_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_DN_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.day_night.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_DNN, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->dnn[prof->dnn_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.day_night.value)
				{
					prof->dnn[prof->dnn_cnt].selected = 1;
				}
				prof->dnn_cnt++;
			}
		}
	}

	/* Day and Night Toggle Control */
	//if (prof->supported & NF_IPCAM_IMAGE_DNN_TOGGLE)
	{
		int i=0, cnt=0;
		prof->dnn_toggle_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_TGTIME_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.tg_time.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_DNN_TOGGLE, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->dnn_toggle[prof->dnn_toggle_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.tg_time.value)
				{
					prof->dnn_toggle[prof->dnn_toggle_cnt].selected = 1;
				}
				prof->dnn_toggle_cnt++;
			}
		}
	}

	/* P-IRIS Control */
	//if (prof->supported & NF_IPCAM_IMAGE_PIRIS)
	{
		int i=0, cnt=0;
		prof->p_iris_cnt = 0;

		for (i=0; i<NF_IPCAM_IMAGE_IRIS_NR; i++)
		{
			if ((1<<i) & runtime[ch].image.iris.support)
			{
				NFIPCamOption *get_tuple;

				get_tuple = get_cam_model_option(NF_IPCAM_IMAGE_PIRIS, (1<<i));
				if (get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d)\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->p_iris[prof->p_iris_cnt], get_tuple, sizeof(NFIPCamOption));
				if ((1<<i) & runtime[ch].image.iris.value)
				{
					prof->p_iris[prof->p_iris_cnt].selected = 1;
				}
				prof->p_iris_cnt++;
			}
		}
	}

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

int smart_motion_parse_db_string(NFIPCamMotionSmartOption *smart_motion_options, const char *db_string)
{
	char* rbuf;
	int size = 0;
	int idx = 0;
	int len;
	char *buffer = NULL;
	char *p_str = NULL;

	if(smart_motion_options == NULL || db_string == NULL)
	{
		printf("[%s:%d] error smart_motion_options[%p] db_string[%p]\n", __func__, __LINE__, smart_motion_options, db_string);
		return 0;
	}
	len = strlen(db_string);
	if(len <=0){
		return 0;
	}
	
	buffer = (char *)malloc(len + 1);
	if(buffer == NULL){
		printf("[%s:%d] error malloc\n", __func__, __LINE__);
		return 0;
	}
	memcpy(buffer, db_string, len);
	buffer[len] = '\0';

	for(idx = 0, p_str = strtok_r(buffer, ",", &rbuf); p_str!=NULL; idx++, p_str = strtok_r(NULL,",", &rbuf)){
		sscanf(p_str, "%[^:]%*[:]%03d%*[:]%01d", smart_motion_options[idx].name, &(smart_motion_options[idx].threshold), &(smart_motion_options[idx].enable));
		/*
		if(smart_motion_options[idx].threshold == 0){
			smart_motion_options[idx].enable = 0;
		}else{
			smart_motion_options[idx].enable = 1;
		}
		*/
	}
	size = idx;

	free(buffer);
	return size;
}

const char *smart_motion_capa_to_db_string(char *buffer, size_t buffer_size,
		NFIPCamMotionSmartOption *smart_motion_options, unsigned int smart_motion_option_size)
{
	int i, pos;
	static char _buffer[200] = {0,};

	if(smart_motion_options == NULL || smart_motion_option_size == 0)
	{
		printf("[%s:%d] error smart_motion_options[%p] smart_motion_option_size[%d]\n", __func__, __LINE__, smart_motion_options, smart_motion_option_size);
		return NULL;
	}

	if(buffer == NULL || buffer_size == 0){
		buffer = _buffer;
		buffer_size = 200;
	}

	for(i=0, pos=0; i<smart_motion_option_size && pos <buffer_size; i++){
		//pos += snprintf(buffer+pos, buffer_size - pos, "%s:%03d,", smart_motion_options[i].name, smart_motion_options[i].enable ? smart_motion_options[i].threshold : 0);
		pos += snprintf(buffer+pos, buffer_size - pos, "%s:%03d:%01d,", smart_motion_options[i].name, smart_motion_options[i].threshold % 100, smart_motion_options[i].enable != 0);
	}
	if(pos > buffer_size)
		pos=buffer_size;
	buffer[pos-1] = '\0';
	return buffer;
}

int nf_ipcam_sync_image(int ch)
{
	int rc;
	mtable* runtime = NULL;
	image_info image;

	runtime = get_runtime();
	memset(&image, 0, sizeof(image_info));

	rc = cam_get_image_x(&image, ch);
	if(rc != 0){
		printf("[%s:%d] rc error [%d] \n", __func__, __LINE__, rc);
		return rc;
	}

	runtime[ch].image.exposure.value = 1 << image.ae;
	runtime[ch].image.agc.value = image.agc;
	runtime[ch].image.eshutter_speed.value = image.shutter;
	runtime[ch].image.slow_shutter.value = 1 <<image.ss;
	runtime[ch].image.max_agc.value = 1 << image.max_agc;
	runtime[ch].image.iris.value = 1 << image.iris;
	runtime[ch].image.blc.value = 1 << image.blc;

	// dnn nvr 기준
	/*
	if(image.dnn ==13)
	{
		if(get_ircut_dnn_now(ch) ==  0)
		{
			if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_DAY)
				image.dnn = 1;
			else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT_DAY)
				image.dnn = 0;
			else if(runtime[ch].image.day_ngiht.support & NF_IPCAM_IMAGE_DN_NPT3_Day)
				image.dnn = 11;
		}
		else
		{
			if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NIGHT)
				image.dnn = 2;
			else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT_NIGHT)
				image.dnn = 9;
			else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT3_NIGHT)
				image.dnn = 12;
		}
	}
	runtime[ch].image.day_night.value = image.dnn;
	runtime[ch].image.tg_time.value = 1 <<image.dnn_time;
	
	runtime[ch].image.dnn_schedule.start.hour = image.dnn_start_hour;
	runtime[ch].image.dnn_schedule.start.min = image.dnn_start_min;
	runtime[ch].image.dnn_schedule.end.hour = image.dnn_end_hour;
	runtime[ch].image.dnn_schedule.end.min = image.dnn_end_min;
	*/
	runtime[ch].image.wb.value = 1 << image.awb;
	runtime[ch].image.mwb.value = 1 << image.mwb;
	runtime[ch].image.sharpness.value = image.sharpness;
	runtime[ch].image.wd.value = 1 << image.wd;
	//check
	runtime[ch].image.focus_mode.value = 1 << image.focus_mode;
	runtime[ch].image.dnr_ctr.value = 1 <<image.dnr_ctr;
	runtime[ch].image.adaptive_ir.value = 1 << image.adaptive_ir;
	runtime[ch].image.defog.value = 1 << image.defog;
	//check
	runtime[ch].image.hlc.value = 1 << image.hlc;

	runtime[ch].image.anti_flicker.value = 1 << image.ff_mode;
	runtime[ch].image.max_shutter.value = 1 << image.max_shutter;

	runtime[ch].image.brightness.value = image.brightness;
	runtime[ch].image.contrast.value = image.contrast;
	runtime[ch].image.color.value = image.color;
	//check
	runtime[ch].image.tint.value = image.tint;

	runtime[ch].image.colorvu_ctrl.value = 1 << image.colorvu_ctrl;
	runtime[ch].image.colorvu_level.value = image.colorvu_level;
	/*
	if(runtime[ch].image.supported & NF_IPCAM_IMAGE_COLORVU){
	}
	*/

	//check
	/*
	runtime[ch].image.focus.limit_value = 1 << image.focus_limit;
	runtime[ch].image.stabilizer.value = 1 <<image.stabilizer;
	runtime[ch].image.ir_correction.value = 1 << image.ir_correction;
	*/
	return 0;
}

int nf_ipcam_sync_focus(int ch)
{
	int rc;
	mtable* runtime = NULL;
	image_info image;
	int focus = 0;

	runtime = get_runtime();
	memset(&image, 0, sizeof(image_info));

	rc = cam_get_focus(&focus,ch);
	if(rc != 0 ){
		printf("[%s:%d] ch[%d] http api call error \n", __func__, __LINE__, ch);
		return -1;
	}

	return 0;
}

int nf_ipcam_sync_focus_comp(int ch)
{
	int rc;
	mtable* runtime = NULL;
	focus_comp_info focus;

	runtime = get_runtime();
	memset(&focus, 0, sizeof(focus_comp_info));
	rc = cam_get_focus_comp(&focus, ch);
	if(rc != 0){
		printf("[%s:%d] ch[%d] http api call error\n", __func__, __LINE__, ch);
		return -1;
	}
	
	runtime[ch].focus.dnn_comp.value = focus.dnn_comp_mode;
	return 0;
}

int nf_ipcam_sync_dnn(int ch)
{
	int rc;
	mtable* runtime = NULL;
	image_info image;

	runtime = get_runtime();
	memset(&image, 0, sizeof(image_info));

	rc = cam_get_dnn_adjust_d2n(&image, ch);
	if(rc != 0 ){
		printf("[%s:%d] ch[%d] http api call error \n", __func__, __LINE__, ch);
		return -1;
	}

	runtime[ch].image.dnn_sense_ntod.value = image.dnn_sense_ntod;
	runtime[ch].image.dnn_sense_dton.value = image.dnn_sense_dton;
	return 0;
}

int nf_ipcam_sync_mirror(int ch)
{
	int rc;
	mtable* runtime = NULL;
	int mirror = 0;

	runtime = get_runtime();

	rc = cam_get_mirror_mode(&mirror, ch);
	if(rc != 0){
		printf("[%s:%d] ch[%d] http api call error \n", __func__, __LINE__, ch);
		return -1;
	}

	runtime[ch].video.mirror.value = 1 << mirror;
	return 0;
}

int nf_ipcam_sync_ai_motion(int ch)
{
	int rc;
	mtable* runtime = NULL;
	NFIPCamSetupMotionSmart motion_info;
	GValue set_value = {0,};

	runtime = get_runtime();
	memset(&motion_info, 0, sizeof(motion_info));

	if(!runtime[ch].motion.smart_motion_support){
		return 0;
	}

	rc = cam_get_motion_smart(&motion_info, ch);
	if(rc != 0){
		printf("[%s:%d] ch[%d] http api call error \n", __func__, __LINE__, ch);
		return -1;
	}

	if(motion_info.smart_motion_option_size == 0){
		//default setting
		motion_info.smart_motion_option_size = 1;
		snprintf(motion_info.smart_motion_options[0].name, sizeof(motion_info.smart_motion_options[0].name),"person");
		motion_info.smart_motion_options[0].enable = 1;
		motion_info.smart_motion_options[0].threshold = 100;
	}

	//runtime update
	runtime[ch].motion.smart_motion_enable = motion_info.smart_motion_enable;
	memcpy(runtime[ch].motion.smart_motion_options, motion_info.smart_motion_options, sizeof(runtime[ch].motion.smart_motion_options));
	runtime[ch].motion.smart_motion_option_size = motion_info.smart_motion_option_size;

	//db set
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, (gint)runtime[ch].motion.smart_motion_enable);
	nf_sysdb_set_key1("alarm.motion.M%d.smart_motion", ch, &set_value, NULL);
	g_value_unset(&set_value);

	return 0;
}

int nf_ipcam_sync_pmask(int ch)
{
	int rc;
	mtable* runtime = NULL;

	//NFIPCamPrivacyMaskProfile pmask;
	NFIPCamPrivacyMask pmask;

	GValue set_value = {0,};
	char key[64];
	int i;

	runtime = get_runtime();
	memset(&pmask, 0, sizeof(pmask));

	rc = cam_get_pmask(&pmask, ch);
	if(rc != 0){
		printf("[%s:%d] ch[%d] http api call error \n", __func__, __LINE__, ch);
		return -1;
	}

	//db update
	for(i=0; i<runtime[ch].privacymask.max_rect; i++)
	{
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, pmask.lt[i].x);
		if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.sx", ch, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			printf("[%s:%d] db set error cam.privacy.P%d.area.A%d.sx value[%d]\n", __func__, __LINE__, ch, i, pmask.lt[i].x);
			continue;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, pmask.lt[i].y);
		if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.sy", ch, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			printf("[%s:%d] db set error cam.privacy.P%d.area.A%d.sx value[%d]\n", __func__, __LINE__, ch, i, pmask.lt[i].y);
			continue;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, pmask.rb[i].x);
		if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.ex", ch, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			printf("[%s:%d] db set error cam.privacy.P%d.area.A%d.sx value[%d]\n", __func__, __LINE__, ch, i, pmask.rb[i].x);
			continue;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, pmask.rb[i].y);
		if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.ey", ch, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			printf("[%s:%d] db set error cam.privacy.P%d.area.A%d.sx value[%d]\n", __func__, __LINE__, ch, i, pmask.rb[i].y);
			continue;
		}
		g_value_unset(&set_value);
	}	
	return 0;
}

int nf_ipcam_sync_cam_db(int ch)
{
	nf_ipcam_sync_image(ch);
	//nf_ipcam_sync_dnn(ch);
	//nf_ipcam_sync_focus(ch);
	nf_ipcam_sync_focus_comp(ch);
	nf_ipcam_sync_mirror(ch);
	//nf_ipcam_sync_motion(ch);
	//nf_ipcam_sync_ai_motion(ch);
	nf_ipcam_sync_pmask(ch);

	//runtime[ch].video.mirror.value;
	return 0;
}

/**
 * @brief 카메라의 image설정 관련 capability struct를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] prof Image profile.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @sa nf_ipcam_get_exposure_profile_onvif
 */
int nf_ipcam_get_image_profile_onvif(gint ch, NFIPCamImageProfile_onvif* prof)
{
	int i, j;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( prof != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);

	memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));

	NFIPCamFocusCompProfile focus_profile;
	memset(&focus_profile, 0x00, sizeof(focus_profile));
	nf_ipcam_get_focus_profile(ch, &focus_profile);

	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		prof->supported_image = runtime[ch].image_onvif.supported_image;

		char key[64];

		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
		{
			prof->brightness.category = NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;
			prof->brightness.min = runtime[ch].image_onvif.brightness.min ;
			prof->brightness.max = runtime[ch].image_onvif.brightness.max;
			prof->brightness.value = runtime[ch].image_onvif.brightness.value;
			prof->brightness.dependent_category = 0;
		}

		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
		{
			prof->contrast.category = NF_IPCAM_IMAGE_ONVIF_CONTRAST;
			prof->contrast.min = runtime[ch].image_onvif.contrast.min;
			prof->contrast.max = runtime[ch].image_onvif.contrast.max;
			prof->contrast.value = runtime[ch].image_onvif.contrast.value;
			prof->contrast.dependent_category = 0;
		}

		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
		{
			prof->color.category = NF_IPCAM_IMAGE_ONVIF_COLOR;
			prof->color.min = runtime[ch].image_onvif.color.min;
			prof->color.max = runtime[ch].image_onvif.color.max;
			prof->color.value = runtime[ch].image_onvif.color.value;
			prof->color.dependent_category = 0;
		}

		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
		{
			prof->sharpness.category = NF_IPCAM_IMAGE_ONVIF_SHARPNESS;
			prof->sharpness.min = runtime[ch].image_onvif.sharpness.min;
			prof->sharpness.max = runtime[ch].image_onvif.sharpness.max;
			prof->sharpness.value = runtime[ch].image_onvif.sharpness.value;
			prof->sharpness.dependent_category = 0;
		}
		{
			prof->tint.category = NF_IPCAM_IMAGE_ONVIF_TINT;
			prof->tint.min = 0;
			prof->tint.max = 0;
			prof->tint.value = 0;
			prof->tint.dependent_category = 0;
		}

		prof->wb_cnt = 0;
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_WB_MODE)
		{
			for(i = 0; i < NF_IPCAM_WB_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.wb.mode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_WB_MODE, (1 << i));
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->wb[prof->wb_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.wb.mode.value)
					{
						prof->wb[prof->wb_cnt].selected = 1;
					}
					prof->wb_cnt++;
				}
			}
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
		{
			prof->crgain.category = NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN;
			prof->crgain.min = runtime[ch].image_onvif.wb.crgain.min;
			prof->crgain.max = runtime[ch].image_onvif.wb.crgain.max;
			prof->crgain.value = runtime[ch].image_onvif.wb.crgain.value;
			if(prof->crgain.value < prof->crgain.min || prof->crgain.value > prof->crgain.max)
			{
				prof->crgain.value = prof->crgain.min;
			}
			//prof->crgain.dependent_category = NF_IPCAM_IMAGE_ONVIF_WB_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
		{
			prof->cbgain.category = NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN;
			prof->cbgain.min = runtime[ch].image_onvif.wb.cbgain.min;
			prof->cbgain.max = runtime[ch].image_onvif.wb.cbgain.max;
			prof->cbgain.value = runtime[ch].image_onvif.wb.cbgain.value;
			if(prof->cbgain.value < prof->cbgain.min || prof->cbgain.value > prof->cbgain.max)
			{
				prof->cbgain.value = prof->cbgain.min;
			}
			//prof->cbgain.dependent_category = NF_IPCAM_IMAGE_ONVIF_WB_MODE;
		}

		prof->focus_cnt = 0;
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE)
		{
			for(i = 0; i < NF_IPCAM_FOCUS_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.focus.mode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE, (1 << i));
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->focus[prof->focus_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.focus.mode.value)
					{
						prof->focus[prof->focus_cnt].selected = 1;
					}
					prof->focus_cnt++;
				}
			}
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
		{
			prof->defaultspeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED;
			prof->defaultspeed.min = runtime[ch].image_onvif.focus.defaultspeed.min;
			prof->defaultspeed.max = runtime[ch].image_onvif.focus.defaultspeed.max;
			prof->defaultspeed.value = runtime[ch].image_onvif.focus.defaultspeed.value;
			if(prof->defaultspeed.value < prof->defaultspeed.min || prof->defaultspeed.value > prof->defaultspeed.max)
			{
				prof->defaultspeed.value = prof->defaultspeed.min;
			}
			//prof->defaultspeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
		{
			prof->nearlimit.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT;
			prof->nearlimit.min = runtime[ch].image_onvif.focus.nearlimit.min;
			prof->nearlimit.max = runtime[ch].image_onvif.focus.nearlimit.max;
			prof->nearlimit.value = runtime[ch].image_onvif.focus.nearlimit.value;
			if(prof->nearlimit.value < prof->nearlimit.min || prof->nearlimit.value > prof->nearlimit.max)
			{
				prof->nearlimit.value = prof->nearlimit.min;
			}
			//prof->nearlimit.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
		{
			prof->farlimit.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT;
			prof->farlimit.min = runtime[ch].image_onvif.focus.farlimit.min;
			prof->farlimit.max = runtime[ch].image_onvif.focus.farlimit.max;
			prof->farlimit.value = runtime[ch].image_onvif.focus.farlimit.value;
			if(prof->farlimit.value < prof->farlimit.min || prof->farlimit.value > prof->farlimit.max)
			{
				prof->farlimit.value = prof->farlimit.min;
			}
			//prof->farlimit.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION)
		{
			prof->abposition.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION;
			prof->abposition.min = runtime[ch].image_onvif.move.abposition.min;
			prof->abposition.max = runtime[ch].image_onvif.move.abposition.max;
			prof->abposition.value = runtime[ch].image_onvif.move.abposition.value;
			//prof->abposition.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED)
		{
			prof->abspeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED;
			prof->abspeed.min = runtime[ch].image_onvif.move.abspeed.min;
			prof->abspeed.max = runtime[ch].image_onvif.move.abspeed.max;
			prof->abspeed.value = runtime[ch].image_onvif.move.abspeed.value;
			// 1 to least(abs(min), abs(max))
			if(prof->abspeed.min < 0)
			{
				prof->abspeed.min = 1;
				prof->abspeed.max = abs(runtime[ch].image_onvif.move.abspeed.min) < abs(runtime[ch].image_onvif.move.abspeed.max) ? abs(runtime[ch].image_onvif.move.abspeed.min) : abs(runtime[ch].image_onvif.move.abspeed.max);

				prof->abspeed.value = 1;
			}
			//prof->abspeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE)
		{
			prof->redistance.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE;
			prof->redistance.min = runtime[ch].image_onvif.move.redistance.min;
			prof->redistance.max = runtime[ch].image_onvif.move.redistance.max;
			prof->redistance.value = runtime[ch].image_onvif.move.redistance.value;
			//prof->redistance.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED)
		{
			prof->respeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED;
			prof->respeed.min = runtime[ch].image_onvif.move.respeed.min;
			prof->respeed.max = runtime[ch].image_onvif.move.respeed.max;
			prof->respeed.value = runtime[ch].image_onvif.move.respeed.value;

			if(prof->respeed.min < 0)
			{
				prof->respeed.min = 1;
				prof->respeed.max = abs(runtime[ch].image_onvif.move.respeed.min) < abs(runtime[ch].image_onvif.move.respeed.max) ? abs(runtime[ch].image_onvif.move.respeed.min) : abs(runtime[ch].image_onvif.move.respeed.max);

				prof->respeed.value = 1;
			}
			//prof->respeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED)
		{
			prof->cospeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED;
			prof->cospeed.min = runtime[ch].image_onvif.move.cospeed.min;
			prof->cospeed.max = runtime[ch].image_onvif.move.cospeed.max;
			prof->cospeed.value = runtime[ch].image_onvif.move.cospeed.value;

			if(prof->cospeed.min < 0)
			{
				prof->cospeed.min = 1;
				prof->cospeed.max = abs(runtime[ch].image_onvif.move.cospeed.min) < abs(runtime[ch].image_onvif.move.cospeed.max) ? abs(runtime[ch].image_onvif.move.cospeed.min) : abs(runtime[ch].image_onvif.move.cospeed.max);

				prof->cospeed.value = 1;
			}
			//prof->cospeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}

		{
			for(j = 0; j < NF_IPCAM_MIRROR_NR; j++)
			{
				if((1 << j) & runtime[ch].video.mirror.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_ROTATION, 1 << j);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->mirror[prof->mirror_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].video.mirror.value)
					{
						prof->mirror[prof->mirror_cnt].selected = 1;
					}
					prof->mirror_cnt++;
				}
			}
		}
	}
	else
	{
		prof->supported_image = 0;

		//IPCAM_DBG(MINOR, "image support : %x\n", runtime[ch].image.supported);

		char key[64];

		for(i = 0; i < NF_IPCAM_IMAGE_NR; i++)
		{
			if((1LL << i) & runtime[ch].image.supported)
			{
				switch(1LL << i)
				{
					case NF_IPCAM_IMAGE_SHARPNESS:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_SHARPNESS;

						break;
					case NF_IPCAM_IMAGE_BRIGHTNESS:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;

						break;
					case NF_IPCAM_IMAGE_CONTRAST:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_CONTRAST;

						break;
					case NF_IPCAM_IMAGE_COLOR:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_COLOR;

						break;
					case NF_IPCAM_IMAGE_TINT:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_TINT;

						break;

					case NF_IPCAM_IMAGE_CALIBRATION:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME;
						break;
					case NF_IPCAM_IMAGE_WB:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_MODE;

						break;
					case NF_IPCAM_IMAGE_MWB:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_PRESET;

						break;
					case NF_IPCAM_IMAGE_MIRRORING:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_ROTATION;

						break;
					case NF_IPCAM_IMAGE_FOCUS:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARFAR;
						//prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION;

						break;
					case NF_IPCAM_IMAGE_ONEPUSH:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_HOME;
						break;

					case NF_IPCAM_IMAGE_FOCUS_LIMIT:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT;
						break;

					case NF_IPCAM_IMAGE_STABILIZER:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_STABILIZER;
						break;

					case NF_IPCAM_IMAGE_IR_CORRECTION:
						prof->supported_image |= NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION;
						break;

					default:
						break;
				}
			}
		}
		{
			prof->sharpness.category = NF_IPCAM_IMAGE_ONVIF_SHARPNESS;
			prof->sharpness.min = runtime[ch].image.sharpness.min;
			prof->sharpness.max = runtime[ch].image.sharpness.max;
			prof->sharpness.value = runtime[ch].image.sharpness.value;
			prof->sharpness.dependent_category = 0;
		}

		{
			prof->brightness.category = NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;
			prof->brightness.min = runtime[ch].image.brightness.min;
			prof->brightness.max = runtime[ch].image.brightness.max;
			prof->brightness.value = runtime[ch].image.brightness.value;
			prof->brightness.dependent_category = 0;
		}
		{
			prof->contrast.category = NF_IPCAM_IMAGE_ONVIF_CONTRAST;
			prof->contrast.min = runtime[ch].image.contrast.min;
			prof->contrast.max = runtime[ch].image.contrast.max;
			prof->contrast.value = runtime[ch].image.contrast.value;
			prof->contrast.dependent_category = 0;
		}
		{
			prof->color.category = NF_IPCAM_IMAGE_ONVIF_COLOR;
			prof->color.min = runtime[ch].image.color.min;
			prof->color.max = runtime[ch].image.color.max;
			prof->color.value = runtime[ch].image.color.value;
			prof->color.dependent_category = 0;
		}
		{
			prof->tint.category = NF_IPCAM_IMAGE_ONVIF_TINT;
			prof->tint.min = runtime[ch].image.tint.min;
			prof->tint.max = runtime[ch].image.tint.max;
			prof->tint.value = runtime[ch].image.tint.value;
			prof->tint.dependent_category = 0;
		}

		{
			prof->crgain.category = NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN;
			//prof->crgain.dependent_category = NF_IPCAM_IMAGE_ONVIF_WB_MODE;
		}
		{
			prof->cbgain.category = NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN;
			//prof->cbgain.dependent_category = NF_IPCAM_IMAGE_ONVIF_WB_MODE;
		}
		{
			prof->defaultspeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED;
			//prof->defaultspeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->nearlimit.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT;
			//prof->nearlimit.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->farlimit.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT;
			//prof->farlimit.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->abposition.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION;
			//prof->abposition.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
			//prof->abposition.min = runtime[ch].ptz.focus.min;
			//prof->abposition.max = runtime[ch].ptz.focus.max;
			//prof->abposition.value = runtime[ch].ptz.focus.value;
		}
		{
			prof->abspeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED;
			//prof->abspeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->redistance.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE;
			//prof->redistance.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->respeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED;
			//prof->respeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}
		{
			prof->cospeed.category = NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED;
			//prof->cospeed.dependent_category = NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE;
		}

		{
			for(j = 0; j < NF_IPCAM_IMAGE_WB_MODE_NR; j++)
			{
				if((1 << j) & runtime[ch].image.wb.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_WB_MODE_MANUAL:
							convertv = NF_IPCAM_WB_MODE_ITX_MANUAL;
							break;
						case NF_IPCAM_IMAGE_WB_MODE_AUTO:
							convertv = NF_IPCAM_WB_MODE_ONVIF_AUTO;
							break;
						case NF_IPCAM_IMAGE_WB_MODE_AUTO_W:
							convertv = NF_IPCAM_WB_MODE_ITX_AUTO_W;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_WB_MODE, convertv);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->wb[prof->wb_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.wb.value)
					{
						prof->wb[prof->wb_cnt].selected = 1;
					}
					prof->wb_cnt++;
				}
			}
		}
		{
			int major, type, subtype, minor;

			for(j = 0; j < NF_IPCAM_IMAGE_MWB_MODE_NR; j++)
			{
				if((1 << j) & runtime[ch].image.mwb.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_WB_PRESET, 1 << j);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->mwb[prof->mwb_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.mwb.value)
					{
						prof->mwb[prof->mwb_cnt].selected = 1;
					}

					sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);
					// change caption
					if(type == 3 && (prof->mwb[prof->mwb_cnt].value == NF_IPCAM_IMAGE_MWB_MODE_INDOOR))
					{
						strncpy(prof->mwb[prof->mwb_cnt].caption, "INDOOR (3200K)", 32);
					}
					if(type == 3 && (prof->mwb[prof->mwb_cnt].value == NF_IPCAM_IMAGE_MWB_MODE_OUTDOOR))
					{
						strncpy(prof->mwb[prof->mwb_cnt].caption, "OUTDOOR (5800K)", 32);
					}
					prof->mwb_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_FOCUS_MODE_ONVIF_NR; j++)
			{
				if((1 << j) & runtime[ch].image.focus_mode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE, 1 << j);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->focus[prof->focus_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.focus_mode.value)
					{
						prof->focus[prof->focus_cnt].selected = 1;
					}
					prof->focus_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_MIRROR_NR; j++)
			{
				if((1 << j) & runtime[ch].video.mirror.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_ROTATION, 1 << j);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->mirror[prof->mirror_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].video.mirror.value)
					{
						prof->mirror[prof->mirror_cnt].selected = 1;
					}
					prof->mirror_cnt++;
				}
			}
		}
		//if(!(runtime[ch].ptz.supported & PTZ_SETUP_FOCUS) && (runtime[ch].sys.stdver[0] == '\0'))
		if(0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE, NF_IPCAM_FOCUS_MODE_ITX_ABSOLUTE);
			if(get_tuple == NULL)
			{
				memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->focus[prof->focus_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->focus[prof->focus_cnt].selected = 0;
			prof->focus_cnt++;
		}
		if(prof->focus_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE, NF_IPCAM_FOCUS_MODE_ITX_CONTINUOUS);
			if(get_tuple == NULL)
			{
				memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->focus[prof->focus_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->focus[prof->focus_cnt].selected = 1;
			prof->focus_cnt++;
		}

		{
			for(j = 0; j < NF_IPCAM_IMAGE_FOCUS_LIMIT_NR; j++)
			{
				if((1 << j) & runtime[ch].image.focus_limit.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_FOCUS_LIMIT_30:
							convertv = NF_IPCAM_FOCUS_LIMIT_30;
							break;
						case NF_IPCAM_IMAGE_FOCUS_LIMIT_100:
							convertv = NF_IPCAM_FOCUS_LIMIT_100;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_LIMIT, convertv);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->focus_limit[prof->focus_limit_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.focus_limit.value)
					{
						prof->focus_limit[prof->focus_limit_cnt].selected = 1;
					}
					prof->focus_limit_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_STABILIZER_NR; j++)
			{
				if((1 << j) & runtime[ch].image.stabilizer.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_STABILIZER_ON:
							convertv = NF_IPCAM_STABILIZER_ON;
							break;
						case NF_IPCAM_IMAGE_STABILIZER_OFF:
							convertv = NF_IPCAM_STABILIZER_OFF;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_STABILIZER, convertv);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->stabilizer[prof->stabilizer_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.stabilizer.value)
					{
						prof->stabilizer[prof->stabilizer_cnt].selected = 1;
					}
					prof->stabilizer_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_IR_CORRECTION_NR; j++)
			{
				if((1 << j) & runtime[ch].image.ir_correction.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_IR_CORRECTION_ON:
							convertv = NF_IPCAM_IR_CORRECTION_ON;
							break;
						case NF_IPCAM_IMAGE_IR_CORRECTION_OFF:
							convertv = NF_IPCAM_IR_CORRECTION_OFF;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_IR_CORRECTION, convertv);
					if(get_tuple == NULL)
					{
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->ir_correction[prof->ir_correction_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.ir_correction.value)
					{
						prof->ir_correction[prof->ir_correction_cnt].selected = 1;
					}
					prof->ir_correction_cnt++;
				}
			}
		}
	}
	{
		if(prof->focus_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE, NF_IPCAM_FOCUS_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->focus[prof->focus_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->focus[prof->focus_cnt].selected = 1;
			prof->focus_cnt++;
		}
	}
	{
		if(prof->wb_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_image(NF_IPCAM_IMAGE_ONVIF_WB_MODE, NF_IPCAM_WB_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->wb[prof->wb_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->wb[prof->wb_cnt].selected = 1;
			prof->wb_cnt++;
		}
	}

#if 0
	puts("\e[31m");
	printf("[%s:%d] CH(%d) prof->supported_image = (0x%llx)\n", __FUNCTION__, __LINE__,ch, prof->supported_image);
	puts("\e[0mm");

	puts("===============================================================");
	puts("Stabilizer");
	puts("===============================================================");

	for(i = 0; i < prof->stabilizer_cnt; i++)
	{
		view_ipcam_option(&prof->stabilizer[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("IR Correction");
	puts("===============================================================");

	for(i = 0; i < prof->ir_correction_cnt; i++)
	{
		view_ipcam_option(&prof->ir_correction[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("Focus Near Limit");
	puts("===============================================================");

	for(i = 0; i < prof->focus_limit_cnt; i++)
	{
		view_ipcam_option(&prof->focus_limit[i]);
	}
	puts("\n");
#endif

	IPCAM_DBG(MINOR,"prof->supported_exposure = (%llu)\n", prof->supported_image);
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 exposure설정 관련 capability struct를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] prof Image profile.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @sa nf_ipcam_get_image_profile_onvif
 */
int nf_ipcam_get_exposure_profile_onvif(gint ch, NFIPCamExposureProfile_onvif* prof)
{
	int i, j;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( prof != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);

	memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));

	/* Supported functions */
	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		prof->supported_exposure = runtime[ch].image_onvif.supported_exposure;

		prof->mode_cnt = 0;
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE)
		{
			for(i = 0; i < NF_IPCAM_EXPOSURE_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.exposure.mode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_MODE, (1 << i));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF Exposure Mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->mode[prof->mode_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.exposure.mode.value)
					{
						prof->mode[prof->mode_cnt].selected = 1;
					}
					prof->mode_cnt++;
				}
			}
		}

		prof->ircut_cnt = 0;
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
		{
			for(i = 0; i < NF_IPCAM_IRCUT_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.ircut.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_EXPOSURE_ONVIF_IRCUT, (1 << i));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF IRCUT\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->ircut[prof->ircut_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.ircut.value)
					{
						prof->ircut[prof->ircut_cnt].selected = 1;
					}
					prof->ircut_cnt++;
				}
			}
		}
		prof->priority_cnt = 0;
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
		{
			for(i = 0; i < NF_IPCAM_PRIORITY_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.exposure.priority.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_PRIORITY, (1 << i));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF PRIORITY\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->priority[prof->priority_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.exposure.priority.value)
					{
						prof->priority[prof->priority_cnt].selected = 1;
					}
					prof->priority_cnt++;
				}
			}
		}

		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
		{
			prof->minetime.category = NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
			prof->minetime.min = runtime[ch].image_onvif.exposure.minetime.min;
			prof->minetime.max = runtime[ch].image_onvif.exposure.minetime.max;
			prof->minetime.value = runtime[ch].image_onvif.exposure.minetime.value;
			//prof->minetime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
		{
			prof->maxetime.category = NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
			prof->maxetime.min = runtime[ch].image_onvif.exposure.maxetime.min;
			prof->maxetime.max = runtime[ch].image_onvif.exposure.maxetime.max;
			prof->maxetime.value = runtime[ch].image_onvif.exposure.maxetime.value;
			//prof->maxetime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
		{
			prof->etime.category = NF_IPCAM_EXPOSURE_ONVIF_ETIME;
			prof->etime.min = runtime[ch].image_onvif.exposure.etime.min;
			prof->etime.max = runtime[ch].image_onvif.exposure.etime.max;
			prof->etime.value = runtime[ch].image_onvif.exposure.etime.value;
			//prof->etime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
		{
			prof->mingain.category = NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
			prof->mingain.min = runtime[ch].image_onvif.exposure.mingain.min;
			prof->mingain.max = runtime[ch].image_onvif.exposure.mingain.max;
			prof->mingain.value = runtime[ch].image_onvif.exposure.mingain.value;
			//prof->mingain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
		{
			prof->maxgain.category = NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
			prof->maxgain.min = runtime[ch].image_onvif.exposure.maxgain.min;
			prof->maxgain.max = runtime[ch].image_onvif.exposure.maxgain.max;
			prof->maxgain.value = runtime[ch].image_onvif.exposure.maxgain.value;
			//prof->maxgain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
		{
			prof->gain.category = NF_IPCAM_EXPOSURE_ONVIF_GAIN;
			prof->gain.min = runtime[ch].image_onvif.exposure.gain.min;
			prof->gain.max = runtime[ch].image_onvif.exposure.gain.max;
			prof->gain.value = runtime[ch].image_onvif.exposure.gain.value;
			//prof->gain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
		{
			prof->miniris.category = NF_IPCAM_EXPOSURE_ONVIF_MINIRIS;
			prof->miniris.min = runtime[ch].image_onvif.exposure.miniris.min;
			prof->miniris.max = runtime[ch].image_onvif.exposure.miniris.max;
			prof->miniris.value = runtime[ch].image_onvif.exposure.miniris.value;
			//prof->miniris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
		{
			prof->maxiris.category = NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS;
			prof->maxiris.min = runtime[ch].image_onvif.exposure.maxiris.min;
			prof->maxiris.max = runtime[ch].image_onvif.exposure.maxiris.max;
			prof->maxiris.value = runtime[ch].image_onvif.exposure.maxiris.value;
			//prof->maxiris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if(prof->supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
		{
			prof->iris.category = NF_IPCAM_EXPOSURE_ONVIF_IRIS;
			prof->iris.min = runtime[ch].image_onvif.exposure.iris.min;
			prof->iris.max = runtime[ch].image_onvif.exposure.iris.max;
			prof->iris.value = runtime[ch].image_onvif.exposure.iris.value;
			//prof->iris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		//if (prof->supported & NF_IPCAM_IMAGE_DCIRIS)
		prof->dc_iris_cnt = 0;
		{
			for(j = 0; j < NF_IPCAM_IMAGE_IRIS_NR; j++)
			{
				if((1 << j) & runtime[ch].image_onvif.exposure.iris_mode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_DCIRIS, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) ONVIF Exp DCIris\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->dc_iris[prof->dc_iris_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image_onvif.exposure.iris_mode.value)
					{
						prof->dc_iris[prof->dc_iris_cnt].selected = 1;
					}
					prof->dc_iris_cnt++;
				}
			}
		}
		prof->blc_cnt = 0;
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_BLC_MODE)
		{
			for(i = 0; i < NF_IPCAM_BLC_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.blcmode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE, (1 << i));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF BLC mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->blc[prof->blc_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.blcmode.value)
					{
						prof->blc[prof->blc_cnt].selected = 1;
					}
					prof->blc_cnt++;
				}
			}
		}
		//if(prof->supported_image & NF_IPCAM_IMAGE_ONVIF_BLC_LEVEL)
		{
			prof->blclevel.category = NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL;
			prof->blclevel.min = runtime[ch].image_onvif.blclevel.min;
			prof->blclevel.max = runtime[ch].image_onvif.blclevel.max;
			prof->blclevel.value = runtime[ch].image_onvif.blclevel.value;
			//prof->blclevel.dependent_category = NF_IPCAM_IMAGE_ONVIF_BLC_MODE;
		}

		prof->wdr_cnt = 0;
		{
			for(i = 0; i < NF_IPCAM_WDR_MODE_ONVIF_NR; i++)
			{
				if((1 << i) & runtime[ch].image_onvif.wdrmode.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE, (1 << i));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF WDR Mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->wdr[prof->wdr_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << i) & runtime[ch].image_onvif.wdrmode.value)
					{
						prof->wdr[prof->wdr_cnt].selected = 1;
					}
					prof->wdr_cnt++;
				}
			}
		}

		{
			prof->wdrlevel.category = NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL;
			prof->wdrlevel.min = runtime[ch].image_onvif.wdrlevel.min;
			prof->wdrlevel.max = runtime[ch].image_onvif.wdrlevel.max;
			prof->wdrlevel.value = runtime[ch].image_onvif.wdrlevel.value;
			if(prof->wdrlevel.value < prof->wdrlevel.min || prof->wdrlevel.value > prof->wdrlevel.max)
			{
				prof->wdrlevel.value = prof->wdrlevel.min;
			}
		}
	}
	else
	{
		prof->supported_exposure = 0;

		for(i = 0; i < NF_IPCAM_IMAGE_NR; i++)
		{
			if((1LL << i) & runtime[ch].image.supported)
			{
				switch(1LL << i)
				{
					case NF_IPCAM_IMAGE_COLORVU:
						prof->supported_colorvu = 1;

						prof->colorvu_level.min = runtime[ch].image.colorvu_level.min;
						prof->colorvu_level.max = runtime[ch].image.colorvu_level.max;
						prof->colorvu_level.value = runtime[ch].image.colorvu_level.value;

						prof->colorvu_ctrl.value = runtime[ch].image.colorvu_ctrl.value;
						break;
					case NF_IPCAM_IMAGE_EXPOSURE:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MODE;

						break;
					case NF_IPCAM_IMAGE_AGC:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_GAIN;

						break;
					case NF_IPCAM_IMAGE_ESHUTTER:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_ETIME;

						break;
					case NF_IPCAM_IMAGE_SLOWSHUTTER:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_SLOWSHUTTER;

						break;
					case NF_IPCAM_IMAGE_MAXAGC:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_MAXAGC;

						break;
					case NF_IPCAM_IMAGE_DCIRIS:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_DCIRIS;

						break;
					case NF_IPCAM_IMAGE_PIRIS:
						//prof->supported_exposure |= (NF_IPCAM_EXPOSURE_DCIRIS | NF_IPCAM_EXPOSURE_ONVIF_IRIS);
						prof->supported_exposure |= (NF_IPCAM_EXPOSURE_DCIRIS);

						break;

					case NF_IPCAM_IMAGE_BLC:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;

						break;

					case NF_IPCAM_IMAGE_ANTIFLICKER:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ANTI_FLICKER | NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;

						break;

					case NF_IPCAM_IMAGE_MAX_SHUTTER:
						prof->supported_exposure |= (NF_IPCAM_EXPOSURE_MAX_SHUTTER_50 | NF_IPCAM_EXPOSURE_MAX_SHUTTER_60 | NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF | NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF);

						break;
						
					case NF_IPCAM_IMAGE_DNR:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_DNR_MODE;

						break;

					case NF_IPCAM_IMAGE_DNR_AUTO_SMART:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_DNR_MODE_AUTO_SMART;

						break;

					case NF_IPCAM_IMAGE_WDR:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE;

						break;

					case NF_IPCAM_IMAGE_ICF:
					case NF_IPCAM_IMAGE_DNN:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_DN_SCHEDULE;
						break;
					case NF_IPCAM_IMAGE_IRCUTM:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRCUTM;
						break;

					case NF_IPCAM_IMAGE_DNN_TOGGLE:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE;

						break;

					case NF_IPCAM_IMAGE_ADAPTIVEIR:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR;
						break;

					case NF_IPCAM_IMAGE_DEFOG:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DEFOG;
						break;

					case NF_IPCAM_IMAGE_HLC:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_HLC;
						break;
						
					case NF_IPCAM_IMAGE_DNN_SENSE_NTOD:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD;
						break;
						
					case NF_IPCAM_IMAGE_DNN_SENSE_DTON:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON;
						break;
					case NF_IPCAM_IMAGE_DC_IRIS_CAL:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_DC_IRIS_CAL;
						break;

					case NF_IPCAM_IMAGE_MAX_SHUTTER_MOTION_OFF_ON:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
						break;
					
					case NF_IPCAM_IMAGE_MAX_SHUTTER_AUTO_OFF_OFF:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
						break;
					
					case NF_IPCAM_IMAGE_MAX_SHUTTER_AUTO_OFF_ON_TV:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;
						break;
					
					case NF_IPCAM_IMAGE_MAX_SHUTTER_AUTO_OFF_ON:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;
						break;

					case NF_IPCAM_IMAGE_ANTI_FLICKER_AUTO_OFF:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;
						break;

					case NF_IPCAM_IMAGE_ANTI_FLICKER_MOTION_OFF:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;
						break;

					case NF_IPCAM_IMAGE_DCIRIS_MOTION:
						prof->supported_exposure |= NF_IPCAM_EXPOSURE_DCIRIS_MOTION;
						break;
					
					case NF_IPCAM_IMAGE_BASE_SHUTTER:
						prof->supported_exposure |= (NF_IPCAM_EXPOSURE_BASE_SHUTTER_50					|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_60	| NF_IPCAM_EXPOSURE_BASE_SHUTTER_100		|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_120	| NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300	|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000| NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360	|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000| NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262	|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262	| NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100	|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300	| NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000|
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120	| NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360 |
						NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000);
						break;

					default:
						break;
				}
			}
		}

		{
			prof->minetime.category = NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
			//prof->minetime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->maxetime.category = NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
			//prof->maxetime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->mingain.category = NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
			//prof->mingain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->maxgain.category = NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
			//prof->maxgain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->miniris.category = NF_IPCAM_EXPOSURE_ONVIF_MINIRIS;
			//prof->miniris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->maxiris.category = NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS;
			//prof->maxiris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->iris.category = NF_IPCAM_EXPOSURE_ONVIF_IRIS;
			//prof->iris.min = runtime[ch].ptz.iris.min;
			//prof->iris.max = runtime[ch].ptz.iris.max;
			//prof->iris.value = runtime[ch].ptz.iris.value;
			//prof->iris.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->blclevel.category = NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL;
			//prof->blclevel.dependent_category = NF_IPCAM_IMAGE_ONVIF_BLC_MODE;
		}
		{
			prof->gain.category = NF_IPCAM_EXPOSURE_ONVIF_GAIN;
			prof->gain.min = runtime[ch].image.agc.min;
			prof->gain.max = runtime[ch].image.agc.max;
			prof->gain.value = runtime[ch].image.agc.value;
			//prof->gain.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			prof->wdrlevel.category = NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL;
		}
		{
			prof->dnn_sense_ntod.category = NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD;
			prof->dnn_sense_ntod.min = runtime[ch].image.dnn_sense_ntod.min;
			prof->dnn_sense_ntod.max = runtime[ch].image.dnn_sense_ntod.max;
			prof->dnn_sense_ntod.value = runtime[ch].image.dnn_sense_ntod.value;
			prof->dnn_sense_ntod.difference = runtime[ch].image.dnn_sense_ntod.diff;
			prof->dnn_sense_ntod.dependent_category = runtime[ch].image.dnn_sense_ntod.dep_cate;

			prof->dnn_sense_dton.category = NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON;
			prof->dnn_sense_dton.min = runtime[ch].image.dnn_sense_dton.min;
			prof->dnn_sense_dton.max = runtime[ch].image.dnn_sense_dton.max;
			prof->dnn_sense_dton.value = runtime[ch].image.dnn_sense_dton.value;
			prof->dnn_sense_dton.difference = runtime[ch].image.dnn_sense_dton.diff;
			prof->dnn_sense_dton.dependent_category = runtime[ch].image.dnn_sense_dton.dep_cate;
		}
		{
			prof->dnn_schedule.category = NF_IPCAM_EXPOSURE_DN_SCHEDULE;
			prof->dnn_schedule.start.hour = runtime[ch].image.dnn_schedule.start.hour;
			prof->dnn_schedule.start.min = runtime[ch].image.dnn_schedule.start.min;
			prof->dnn_schedule.end.hour = runtime[ch].image.dnn_schedule.end.hour;
			prof->dnn_schedule.end.min = runtime[ch].image.dnn_schedule.end.min;
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_EXP_MODE_NR; j++)
			{
				if((1 << j) & runtime[ch].image.exposure.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_EXP_MODE_MANUAL:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_INDOOR:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_INDOOR;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_OUTDOOR:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_OUTDOOR;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_MOTION:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_MOTION;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_I3:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_I3;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_MANUAL_I3:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_I3;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_INX:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_INX;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_MOTION_INX:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_MOTION_INX;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_MANUAL_INX:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_INX;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_NPT:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_NPT;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_MANUAL_NPT:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_AUTO_NPT_X10:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_NPT_X10;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_MANUAL_NPT_X10:
							convertv = NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT_X10;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_HI_AUTO_MOTION:
							convertv = NF_IPCAM_EXPOSURE_MODE_HI_AUTO_MOTION;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_HI_AUTO:
							convertv = NF_IPCAM_EXPOSURE_MODE_HI_AUTO;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_HI_MANUAL:
							convertv = NF_IPCAM_EXPOSURE_MODE_HI_MANUAL;
							break;
						case NF_IPCAM_IMAGE_EXP_MODE_HI_AUTO_MOTION_INDH:
							convertv = NF_IPCAM_EXPOSURE_MODE_HI_AUTO_MOTION_INDH;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_MODE, convertv);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->mode[prof->mode_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.exposure.value)
					{
						prof->mode[prof->mode_cnt].selected = 1;
					}
					// exclude manual IRIS on DCIRIS support
					// FIXME
					// TODO can modify IRIS v with DCIRIS CONTROL
					if((runtime[ch].image.iris.support & NF_IPCAM_IMAGE_PIRIS_MANUAL) == 0)
					{
						prof->mode[prof->mode_cnt].enable_category &= ~(NF_IPCAM_EXPOSURE_ONVIF_IRIS);
						prof->mode[prof->mode_cnt].disable_category |= NF_IPCAM_EXPOSURE_ONVIF_IRIS;
						//prof->mode[prof->mode_cnt].disable_category &= ~(NF_IPCAM_EXPOSURE_ONVIF_IRIS);
						//prof->mode[prof->mode_cnt].visible_category &= ~(NF_IPCAM_EXPOSURE_ONVIF_IRIS);
						//prof->mode[prof->mode_cnt].invisible_category |= NF_IPCAM_EXPOSURE_ONVIF_IRIS;
					}
					prof->mode_cnt++;
				}
			}
		}
		{
			prof->etime.category = NF_IPCAM_EXPOSURE_ONVIF_ETIME;
			prof->etime.min = runtime[ch].image.eshutter_speed.min;
			prof->etime.max = runtime[ch].image.eshutter_speed.max;
			prof->etime.value = runtime[ch].image.eshutter_speed.value;
			//prof->etime.dependent_category = NF_IPCAM_EXPOSURE_ONVIF_MODE;
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_BLC_NR; j++)
			{
				if((1 << j) & runtime[ch].image.blc.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_BLC_OFF:
							convertv = NF_IPCAM_BLC_MODE_ONVIF_OFF;
							break;
						case NF_IPCAM_IMAGE_BLC_ON:
							convertv = NF_IPCAM_BLC_MODE_ONVIF_ON;
							break;
						case NF_IPCAM_IMAGE_BLC_ADAPTIVE:
							convertv = NF_IPCAM_BLC_MODE_ITX_ADAPTIVE;
							break;
						case NF_IPCAM_IMAGE_BLC_ZONE_LOWER:
							convertv = NF_IPCAM_BLC_MODE_ITX_ZONE_LOWER;
							break;
						case NF_IPCAM_IMAGE_BLC_ZONE_MIDDLE:
							convertv = NF_IPCAM_BLC_MODE_ITX_ZONE_MIDDLE;
							break;
						case NF_IPCAM_IMAGE_BLC_ZONE_UPPER:
							convertv = NF_IPCAM_BLC_MODE_ITX_ZONE_UPPER;
							break;
						case NF_IPCAM_IMAGE_BLC_ZONE_LEFT:
							convertv = NF_IPCAM_BLC_MODE_ITX_ZONE_LEFT;
							break;
						case NF_IPCAM_IMAGE_BLC_ZONE_RIGHT:
							convertv = NF_IPCAM_BLC_MODE_ITX_ZONE_RIGHT;
							break;
						case NF_IPCAM_IMAGE_BLC_PTZ_ON:
							convertv = NF_IPCAM_BLC_MODE_NPT_ON;
							break;
						case NF_IPCAM_IMAGE_BLC_PTZ_OFF:
							convertv = NF_IPCAM_BLC_MODE_NPT_OFF;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE, convertv);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF BLC mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->blc[prof->blc_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.blc.value)
					{
						prof->blc[prof->blc_cnt].selected = 1;
					}
					prof->blc_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_SLSH_MODE_NR; j++)
			{
				if((1 << j) & runtime[ch].image.slow_shutter.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_SLOWSHUTTER, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp slow shutter\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->slowshutter[prof->slowshutter_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.slow_shutter.value)
					{
						prof->slowshutter[prof->slowshutter_cnt].selected = 1;
					}
					// change caption
					if(((strcmp(runtime[ch].sys.sdkver, "1.0.0.4") >= 0 || (strncmp(runtime[ch].sys.stdver, "IN" ,2) == 0))
					&& strncmp(runtime[ch].sys.stdver, "NPT", 3) !=0 && prof->slowshutter[prof->slowshutter_cnt].value == NF_IPCAM_IMAGE_SLSH_MODE_ON) )
					{
						strncpy(prof->slowshutter[prof->slowshutter_cnt].caption, "X2", 32);
					}
					prof->slowshutter_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_MAGC_MODE_NR; j++)
			{
				if((1 << j) & runtime[ch].image.max_agc.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAXAGC, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp maxagc\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->maxagc[prof->maxagc_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.max_agc.value)
					{
						prof->maxagc[prof->maxagc_cnt].selected = 1;
					}
					prof->maxagc_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_IMAGE_IRIS_NR; j++)
			{
				if((1 << j) & runtime[ch].image.iris.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_DCIRIS, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp DCIris\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->dc_iris[prof->dc_iris_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.iris.value)
					{
						prof->dc_iris[prof->dc_iris_cnt].selected = 1;
					}
					prof->dc_iris_cnt++;
				}
			}
		}
		{
			for(j = 2; j < 4; j++)
			{
				if((1 << j) & runtime[ch].image.iris.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_DCIRIS, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp DCIris\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->dc_iris_motion[prof->dc_iris_motion_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					prof->dc_iris_motion[prof->dc_iris_motion_cnt].category = NF_IPCAM_EXPOSURE_DCIRIS_MOTION;
					if((1 << j) & runtime[ch].image.iris.value)
					{
						prof->dc_iris_motion[prof->dc_iris_motion_cnt].selected = 1;
					}
					prof->dc_iris_motion_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_ANTI_FLICKER_MODE_NR; j++)
			{
				//if(runtime[ch].image.supported & NF_IPCAM_IMAGE_ANTIFLICKER)
				if((1 << j) & runtime[ch].image.anti_flicker.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp AntiFlicker\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->antiflicker[prof->antiflicker_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.anti_flicker.value)
					{
						prof->antiflicker[prof->antiflicker_cnt].selected = 1;
					}
					prof->antiflicker_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_ANTI_FLICKER_MODE_NR; j++)
			{
				//if(runtime[ch].image.supported & NF_IPCAM_IMAGE_ANTIFLICKER)
				if((1 << j) & runtime[ch].image.anti_flicker.support)
				{
					NFIPCamOption_onvif *get_tuple;
					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER, NF_IPCAM_ANTI_FLICKER_MODE_MOTION_OFF);
					else
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER, (1 << j));

					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp AntiFlicker\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->antiflicker_motion[prof->antiflicker_motion_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					prof->antiflicker_motion[prof->antiflicker_motion_cnt].category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION;

					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						prof->antiflicker_motion[prof->antiflicker_motion_cnt].value = NF_IPCAM_ANTI_FLICKER_MODE_OFF;

					if((1 << j) & runtime[ch].image.anti_flicker.value)
					{
						prof->antiflicker_motion[prof->antiflicker_motion_cnt].selected = 1;
					}
					prof->antiflicker_motion_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_ANTI_FLICKER_MODE_NR; j++)
			{
				//if(runtime[ch].image.supported & NF_IPCAM_IMAGE_ANTIFLICKER)
				if((1 << j) & runtime[ch].image.anti_flicker.support)
				{
					NFIPCamOption_onvif *get_tuple;
					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER,NF_IPCAM_ANTI_FLICKER_MODE_HI_MOTION_OFF);
					else
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER, (1 << j));

					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp AntiFlicker\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->antiflicker_motion_off[prof->antiflicker_motion_off_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					prof->antiflicker_motion_off[prof->antiflicker_motion_off_cnt].category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_MOTION_OFF;

					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						prof->antiflicker_motion_off[prof->antiflicker_motion_off_cnt].value = NF_IPCAM_ANTI_FLICKER_MODE_OFF;

					if((1 << j) & runtime[ch].image.anti_flicker.value)
					{
						prof->antiflicker_motion_off[prof->antiflicker_motion_off_cnt].selected = 1;
					}
					prof->antiflicker_motion_off_cnt++;
				}
			}
		}
		{
			for(j = 0; j < NF_IPCAM_ANTI_FLICKER_MODE_NR; j++)
			{
				//if(runtime[ch].image.supported & NF_IPCAM_IMAGE_ANTIFLICKER)
				if((1 << j) & runtime[ch].image.anti_flicker.support)
				{
					NFIPCamOption_onvif *get_tuple;
					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER,NF_IPCAM_ANTI_FLICKER_MODE_HI_AUTO_OFF);
					else
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ANTI_FLICKER, (1 << j));

					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp AntiFlicker\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->antiflicker_auto_off[prof->antiflicker_auto_off_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					prof->antiflicker_auto_off[prof->antiflicker_auto_off_cnt].category = NF_IPCAM_EXPOSURE_ANTI_FLICKER_AUTO_OFF;

					if((1<<j) == NF_IPCAM_ANTI_FLICKER_MODE_OFF)
						prof->antiflicker_auto_off[prof->antiflicker_auto_off_cnt].value = NF_IPCAM_ANTI_FLICKER_MODE_OFF;

					if((1 << j) & runtime[ch].image.anti_flicker.value)
					{
						prof->antiflicker_auto_off[prof->antiflicker_auto_off_cnt].selected = 1;
					}
					prof->antiflicker_auto_off_cnt++;
				}
			}
		}

			// Max Shutter Speed: Anti-Flicker(50Hz)
			for(j = 0; j < 3; j++)
			{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_PAL\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->max_shutter_50[prof->max_shutter_50_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.max_shutter.value)
					{
						prof->max_shutter_50[prof->max_shutter_50_cnt].selected = 1;
					}
					prof->max_shutter_50_cnt++;
				}

			//  Max Shutter Speed:Anti-Flicker(60Hz)
			for(j = 0; j < 3; j++)
			{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_NTSC\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->max_shutter_60[prof->max_shutter_60_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.max_shutter.value)
					{
						prof->max_shutter_60[prof->max_shutter_60_cnt].selected = 1;
					}
					prof->max_shutter_60_cnt++;
				}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Auto) + WDR(OFF)
			for(j = 0; j < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR - 1; j++)
			{
				NFIPCamOption_onvif *get_tuple;
				int is_pal = nf_sysdb_get_bool("sys.info.sig_type");
				if(is_pal)
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
			        }
				else
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
				}
				if(get_tuple == NULL)
			        {
					IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_OFF\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
			        }
				memcpy(&prof->max_shutter_auto_off_off[prof->max_shutter_auto_off_off_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->max_shutter_auto_off_off[prof->max_shutter_auto_off_off_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_OFF;
				if((1 << j) & runtime[ch].image.max_shutter.value)
				{
					prof->max_shutter_auto_off_off[prof->max_shutter_auto_off_off_cnt].selected = 1;
				}
				prof->max_shutter_auto_off_off_cnt++;
			}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Motion) + WDR(OFF) // Existing Motion Off
			for(j = 0; j < 3; j++)
			{
				NFIPCamOption_onvif *get_tuple;
				int is_pal = nf_sysdb_get_bool("sys.info.sig_type");
				if(is_pal)
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
				}
				else
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
				}
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_Motion_OFF\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->max_shutter_motion_off[prof->max_shutter_motion_off_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->max_shutter_motion_off[prof->max_shutter_motion_off_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF;
				if((1 << j) & runtime[ch].image.max_shutter.value)
			{
					prof->max_shutter_motion_off[prof->max_shutter_motion_off_cnt].selected = 1;//
				}
				prof->max_shutter_motion_off_cnt++;
			}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Motion) + WDR(ON) // Normal, T/TV
			for(j = 0; j < 3; j++)
				{
					NFIPCamOption_onvif *get_tuple;
					int is_pal = nf_sysdb_get_bool("sys.info.sig_type");
				if(strstr(runtime[ch].sys.stdver, "VT") != NULL || strstr(runtime[ch].sys.stdver, "RT") != NULL)
					is_pal = 0;

				if(is_pal)
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
				}
				else
				{
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
				}
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_Motion_OFF\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->max_shutter_motion_off_on[prof->max_shutter_motion_off_on_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->max_shutter_motion_off_on[prof->max_shutter_motion_off_on_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_MOTION_OFF_ON;
				if((1 << j) & runtime[ch].image.max_shutter.value)
				{
					prof->max_shutter_motion_off_on[prof->max_shutter_motion_off_on_cnt].selected = 1;//
				}
				prof->max_shutter_motion_off_on_cnt++;
			}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Auto) + WDR(ON) // Existing OFF, Normal, 3M T/TV
			for(j = 0; j < 7; j++)
			{
				NFIPCamOption_onvif *get_tuple;
				int is_pal = nf_sysdb_get_bool("sys.info.sig_type");

				if(strstr(runtime[ch].sys.stdver, "VT") != NULL || strstr(runtime[ch].sys.stdver, "RT") != NULL)
					is_pal = 0;

					if(is_pal)
					{
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
					}
					else
					{
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
					}
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_OFF\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->max_shutter_off[prof->max_shutter_off_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					prof->max_shutter_off[prof->max_shutter_off_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_OFF;
					if((1 << j) & runtime[ch].image.max_shutter.value)
					{
						prof->max_shutter_off[prof->max_shutter_off_cnt].selected = 1;
					}
					prof->max_shutter_off_cnt++;
				}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Auto) + WDR(ON) // 2M T/TV
			for(j = 0; j < 5; j++)
			{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));

				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_Motion_OFF\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
			}
				memcpy(&prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				if(prof->max_shutter_auto_off_on_tv_cnt == 4) {
					strncpy(prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt].caption, "1/262", 32 );
					prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt].value = NF_IPCAM_MAX_SHUTTER_MODE_NTSC_262;
				}

				prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON_TV;

				if((1 << j) & runtime[ch].image.max_shutter.value)
					prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt].selected = 1;
				else if(NF_IPCAM_MAX_SHUTTER_MODE_NTSC_262 & runtime[ch].image.max_shutter.value)
					prof->max_shutter_auto_off_on_tv[prof->max_shutter_auto_off_on_tv_cnt].selected = 1;

				prof->max_shutter_auto_off_on_tv_cnt++;
			}

			//  Max Shutter Speed:Anti-Flicker(OFF) + AE(Auto) + WDR(ON) // 2M HE/HV/NCX
			for(j = 0; j < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR - 1; j++)
				{
					NFIPCamOption_onvif *get_tuple;
					int is_pal = nf_sysdb_get_bool("sys.info.sig_type");
					if(is_pal)
					{
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_50, (1 << j));
					}
					else
					{
						get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_MAX_SHUTTER_60, (1 << j));
					}
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp MaxShutter_Motion_OFF\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
				memcpy(&prof->max_shutter_auto_off_on[prof->max_shutter_auto_off_on_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->max_shutter_auto_off_on[prof->max_shutter_auto_off_on_cnt].category = NF_IPCAM_EXPOSURE_MAX_SHUTTER_AUTO_OFF_ON;
					if((1 << j) & runtime[ch].image.max_shutter.value)
					{
					prof->max_shutter_auto_off_on[prof->max_shutter_auto_off_on_cnt].selected = 1;//
				}				
				prof->max_shutter_auto_off_on_cnt++;
			}

			// WDR
			prof->wdr_cnt = 0;
			for(j = 0; j < NF_IPCAM_WDR_MODE_ONVIF_NR; j++)
			{
				if((1 << j) & runtime[ch].image.wd.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE, 1 << j);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF WDR Mode\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->wdr[prof->wdr_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.wd.value)
					{
						prof->wdr[prof->wdr_cnt].selected = 1;
					}
					prof->wdr_cnt++;
				}
			}


			prof->adaptive_ir_cnt = 0;
			for(j = 0; j < NF_IPCAM_ADAPTIVE_IR_NR; j++)
			{
				if((1 << j) & runtime[ch].image.adaptive_ir.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_ADAPTIVEIR, 1 << j);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->adaptive_ir[prof->adaptive_ir_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.adaptive_ir.value)
					{
						prof->adaptive_ir[prof->adaptive_ir_cnt].selected = 1;
					}
					prof->adaptive_ir_cnt++;
				}
			}
			// DNR MODE
			prof->dnr_cnt = 0;
			for(j = 0; j < NF_IPCAM_DNR_MODE_NR -1; j++)//reserved (auto_smart)
			{
				if( (1 << j) & runtime[ch].image.dnr_ctr.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_DNR_MODE, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp DNR OFF\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->dnr[prof->dnr_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1<<j) & runtime[ch].image.dnr_ctr.value)
					{
						prof->dnr[prof->dnr_cnt].selected = 1;
					}
					prof->dnr_cnt++;
				}
			}

			// DNR MODE AUTO SMART
			prof->dnr_cnt = 0;
			for(j = 0; j < NF_IPCAM_DNR_MODE_NR; j++)
			{
				if( (1 << j) & runtime[ch].image.dnr_ctr.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_DNR_MODE_AUTO_SMART, (1 << j));
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp DNR AUTO OFF\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->dnr[prof->dnr_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1<<j) & runtime[ch].image.dnr_ctr.value)
					{
						prof->dnr[prof->dnr_cnt].selected = 1;
					}
					prof->dnr_cnt++;
				}
			}
		{

			prof->ircut_cnt = 0;
			for(j = 0; j < NF_IPCAM_IMAGE_DN_NR; j++)
			{
				if((1 << j) & runtime[ch].image.day_night.support)
				{
					int convertv = 0;
					switch(1 << j)
					{
						case NF_IPCAM_IMAGE_DN_AUTO:
							convertv = NF_IPCAM_IRCUT_MODE_ITX_AUTO;
							break;
						case NF_IPCAM_IMAGE_DN_DAY:
							convertv = NF_IPCAM_IRCUT_MODE_ITX_DAYTIME;
							break;
						case NF_IPCAM_IMAGE_DN_NIGHT:
							convertv = NF_IPCAM_IRCUT_MODE_ITX_NIGHTTIME;
							break;
						case NF_IPCAM_IMAGE_DN_NPT_AUTO:
							convertv = NF_IPCAM_IRCUT_MODE_NPT_AUTO;
							break;
						case NF_IPCAM_IMAGE_DN_NPT_DAY:
							convertv = NF_IPCAM_IRCUT_MODE_NPT_DAYTIME;
							break;
						case NF_IPCAM_IMAGE_DN_NPT_NIGHT:
							convertv = NF_IPCAM_IRCUT_MODE_NPT_NIGHTTIME;
							break;
						case NF_IPCAM_IMAGE_DN_NPT3_AUTO:
							convertv = NF_IPCAM_IRCUT_MODE_NPT3_AUTO;
							break;
						case NF_IPCAM_IMAGE_DN_NPT3_DAY:
							convertv = NF_IPCAM_IRCUT_MODE_NPT3_DAYTIME;
							break;
						case NF_IPCAM_IMAGE_DN_NPT3_NIGHT:
							convertv = NF_IPCAM_IRCUT_MODE_NPT3_NIGHTTIME;
							break;
						case NF_IPCAM_IMAGE_DN_SCHEDULE:
							convertv = NF_IPCAM_IRCUT_MODE_ITX_SCHEDULE;
							break;
						case NF_IPCAM_IMAGE_DN_EXTERNAL:
							convertv = NF_IPCAM_IRCUT_MODE_ITX_EXTERNAL;
							break;
						default:
							break;
					}

					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_image(NF_IPCAM_EXPOSURE_ONVIF_IRCUT, convertv);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF IRCUT\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->ircut[prof->ircut_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.day_night.value)
					{
						prof->ircut[prof->ircut_cnt].selected = 1;
					}
					prof->ircut_cnt++;
				}
			}

			prof->ircutm_cnt = 0;
			{
				for(j = 0; j < NF_IPCAM_IMAGE_DN_NR; j++)
				{
					if((1 << j) & runtime[ch].image.day_night.support)
					{
						int convertv = 0;
						switch(1 << j)
						{
							case NF_IPCAM_IMAGE_DN_AUTO:
								convertv = NF_IPCAM_IRCUT_MODE_ITX_AUTO;
								break;
							case NF_IPCAM_IMAGE_DN_DAY:
								convertv = NF_IPCAM_IRCUT_MODE_ITX_DAYTIME;
								break;
							case NF_IPCAM_IMAGE_DN_NIGHT:
								convertv = NF_IPCAM_IRCUT_MODE_ITX_NIGHTTIME;
								break;
							case NF_IPCAM_IMAGE_DN_NPT_AUTO:
								convertv = NF_IPCAM_IRCUT_MODE_NPT_AUTO;
								break;
							case NF_IPCAM_IMAGE_DN_NPT_DAY:
								convertv = NF_IPCAM_IRCUT_MODE_NPT_DAYTIME;
								break;
							case NF_IPCAM_IMAGE_DN_NPT_NIGHT:
								convertv = NF_IPCAM_IRCUT_MODE_NPT_NIGHTTIME;
								break;
							case NF_IPCAM_IMAGE_DN_NPT3_AUTO:
								convertv = NF_IPCAM_IRCUT_MODE_NPT3_AUTO;
								break;
							case NF_IPCAM_IMAGE_DN_NPT3_DAY:
								convertv = NF_IPCAM_IRCUT_MODE_NPT3_DAYTIME;
								break;
							case NF_IPCAM_IMAGE_DN_NPT3_NIGHT:
								convertv = NF_IPCAM_IRCUT_MODE_NPT3_NIGHTTIME;
								break;
							case NF_IPCAM_IMAGE_DN_SCHEDULE:
								convertv = NF_IPCAM_IRCUT_MODE_ITX_SCHEDULE;
								break;
							case NF_IPCAM_IMAGE_DN_EXTERNAL:
								convertv = NF_IPCAM_IRCUT_MODE_ITX_EXTERNAL;
								break;
							default:
								break;
						}

						if(convertv == NF_IPCAM_IRCUT_MODE_NPT_AUTO)
						{
							continue;
						}
						if(convertv == NF_IPCAM_IRCUT_MODE_NPT3_AUTO)
						{
							continue;
						}
						

						NFIPCamOption_onvif *get_tuple;
						get_tuple = get_cam_model_option_image(NF_IPCAM_EXPOSURE_ONVIF_IRCUT, convertv);
						if(get_tuple == NULL)
						{
							IPCAM_DBG(WARN, "CH(%d) Exp ONVIF IRCUTM\n", ch);
							memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
							return IPCAM_SETUP_RTN_FAILED;
						}
						memcpy(&prof->ircutm[prof->ircutm_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
						prof->ircutm[prof->ircutm_cnt].category = NF_IPCAM_EXPOSURE_ONVIF_IRCUTM;
						if((1 << j) & runtime[ch].image.day_night.value)
						{
							prof->ircutm[prof->ircutm_cnt].selected = 1;
						}
						prof->ircutm_cnt++;
					}
				}

			}

			prof->dnn_toggle_cnt = 0;
			{
				for(j = 0; j < NF_IPCAM_IMAGE_TGTIME_NR; j++)
				{
					if((1 << j) & runtime[ch].image.tg_time.support)
					{
						NFIPCamOption_onvif *get_tuple;
						get_tuple = get_cam_model_option_image(NF_IPCAM_EXPOSURE_ONVIF_DNN_TOGGLE, 1 << j);
						if(get_tuple == NULL)
						{
							IPCAM_DBG(WARN, "CH(%d) Exp ONVIF DNN TOGGLE\n", ch);
							memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
							return IPCAM_SETUP_RTN_FAILED;
						}
						memcpy(&prof->dnn_toggle[prof->dnn_toggle_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
						if((1 << j) & runtime[ch].image.tg_time.value)
						{
							prof->dnn_toggle[prof->dnn_toggle_cnt].selected = 1;
						}
						prof->dnn_toggle_cnt++;
					}
				}
			}
		}

		prof->base_shutter_100_cnt = 0;
		for(j = 0; j < 1; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, NF_IPCAM_BASE_SHUTTER_MODE_PAL_100);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_100[prof->base_shutter_100_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_100[prof->base_shutter_100_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_100[prof->base_shutter_100_cnt].selected = 1;
				}
				prof->base_shutter_100_cnt++;
		}

		prof->base_shutter_120_cnt = 0;
		for(j = 0; j < 1; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, NF_IPCAM_BASE_SHUTTER_MODE_NTSC_120);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_120[prof->base_shutter_120_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_120[prof->base_shutter_120_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_120[prof->base_shutter_120_cnt].selected = 1;
				}
				prof->base_shutter_120_cnt++;
		}

		prof->base_shutter_100_300_cnt = 0;
		for(j = 2; j < 7; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_100_300[prof->base_shutter_100_300_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_100_300[prof->base_shutter_100_300_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_300;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_100_300[prof->base_shutter_100_300_cnt].selected = 1;
				}
				prof->base_shutter_100_300_cnt++;
		}

		prof->base_shutter_100_5000_cnt = 0;
		for(j = 2; j < 16; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_100_5000[prof->base_shutter_100_5000_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_100_5000[prof->base_shutter_100_5000_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_100_5000;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_100_5000[prof->base_shutter_100_5000_cnt].selected = 1;
				}
				prof->base_shutter_100_5000_cnt++;
		}

		prof->base_shutter_120_360_cnt = 0;
		for(j = 2; j < 7; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_120_360[prof->base_shutter_120_360_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_120_360[prof->base_shutter_120_360_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_360;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_120_360[prof->base_shutter_120_360_cnt].selected = 1;
				}
				prof->base_shutter_120_360_cnt++;
		}

		prof->base_shutter_120_5000_cnt = 0;
		for(j = 2; j < 16; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_120_5000[prof->base_shutter_120_5000_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_120_5000[prof->base_shutter_120_5000_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_5000;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_120_5000[prof->base_shutter_120_5000_cnt].selected = 1;
				}
				prof->base_shutter_120_5000_cnt++;
		}

		prof->base_shutter_120_262_cnt = 0;
		for(j = 2; j < 5; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				if(j != 4)
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				else // j == 4
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, NF_IPCAM_BASE_SHUTTER_MODE_NTSC_262);


				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_120_262[prof->base_shutter_120_262_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_120_262[prof->base_shutter_120_262_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_120_262;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_120_262[prof->base_shutter_120_262_cnt].selected = 1;
				}
				prof->base_shutter_120_262_cnt++;
		}

		prof->base_shutter_30_262_cnt = 0;
		for(j = 0; j < 5; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				if(j != 4)
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				else // j == 4
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, NF_IPCAM_BASE_SHUTTER_MODE_NTSC_262);


				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_30_262[prof->base_shutter_30_262_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_30_262[prof->base_shutter_30_262_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_262;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_30_262[prof->base_shutter_30_262_cnt].selected = 1;
				}
				prof->base_shutter_30_262_cnt++;
		}

		prof->base_shutter_25_100_cnt = 0;
		for(j = 0; j < 3; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_25_100[prof->base_shutter_25_100_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_25_100[prof->base_shutter_25_100_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_100;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_25_100[prof->base_shutter_25_100_cnt].selected = 1;
				}
				prof->base_shutter_25_100_cnt++;
		}

		prof->base_shutter_25_300_cnt = 0;
		for(j = 0; j < 7; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_25_300[prof->base_shutter_25_300_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_25_300[prof->base_shutter_25_300_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_300;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_25_300[prof->base_shutter_25_300_cnt].selected = 1;
				}
				prof->base_shutter_25_300_cnt++;
		}

		prof->base_shutter_25_5000_cnt = 0;
		for(j = 0; j < 16; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_50, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_25_5000[prof->base_shutter_25_5000_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_25_5000[prof->base_shutter_25_5000_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_25_5000;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_25_5000[prof->base_shutter_25_5000_cnt].selected = 1;
				}
				prof->base_shutter_25_5000_cnt++;
		}

		prof->base_shutter_30_120_cnt = 0;
		for(j = 0; j < 3; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_30_120[prof->base_shutter_30_120_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_30_120[prof->base_shutter_30_120_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_120;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_30_120[prof->base_shutter_30_120_cnt].selected = 1;
				}
				prof->base_shutter_30_120_cnt++;
		}

		prof->base_shutter_30_360_cnt = 0;
		for(j = 0; j < 7; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_30_360[prof->base_shutter_30_360_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_30_360[prof->base_shutter_30_360_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_360;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_30_360[prof->base_shutter_30_360_cnt].selected = 1;
				}
				prof->base_shutter_30_360_cnt++;
		}

		prof->base_shutter_30_5000_cnt = 0;
		for(j = 0; j < 16; j++)
		{
				NFIPCamOption_onvif *get_tuple;
				get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_BASE_SHUTTER_60, 1<<j);
				if(get_tuple == NULL)
				{
					IPCAM_DBG(WARN, "CH(%d) Exp ONVIF ADAPTIVEIR\n", ch);
					memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
					return IPCAM_SETUP_RTN_FAILED;
				}
				memcpy(&prof->base_shutter_30_5000[prof->base_shutter_30_5000_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
				prof->base_shutter_30_5000[prof->base_shutter_30_5000_cnt].category = NF_IPCAM_EXPOSURE_BASE_SHUTTER_30_5000;
				if((1 << j) & runtime[ch].image.base_shutter.value)
				{
					prof->base_shutter_30_5000[prof->base_shutter_30_5000_cnt].selected = 1;
				}
				prof->base_shutter_30_5000_cnt++;
		}

		memcpy(prof->base_shutter_speed_table, runtime[ch].image.base_shutter_table, sizeof(uint64_t) * 15 * 6);

		prof->defog_cnt = 0;
		{
			for(j = 0; j < NF_IPCAM_DEFOG_NR; j++)
			{
				if((1 << j) & runtime[ch].image.defog.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_DEFOG, 1 << j);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF DEFOG\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->defog[prof->defog_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.defog.value)
					{
						prof->defog[prof->defog_cnt].selected = 1;
					}
					prof->defog_cnt++;
				}
			}
		}
		prof->hlc_cnt = 0;
		{
			for(j = 0; j < NF_IPCAM_HLC_NR; j++)
			{
				if((1 << j) & runtime[ch].image.hlc.support)
				{
					NFIPCamOption_onvif *get_tuple;
					get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_HLC, 1 << j);
					if(get_tuple == NULL)
					{
						IPCAM_DBG(WARN, "CH(%d) Exp ONVIF HLC\n", ch);
						memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
						return IPCAM_SETUP_RTN_FAILED;
					}
					memcpy(&prof->hlc[prof->hlc_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
					if((1 << j) & runtime[ch].image.hlc.value)
					{
						prof->hlc[prof->hlc_cnt].selected = 1;
					}
					prof->hlc_cnt++;
				}
			}
		}
	}
	{

		if(prof->mode_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_MODE, NF_IPCAM_EXPOSURE_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				IPCAM_DBG(WARN, "CH(%d) Exp ONVIF DUMMY\n", ch);
				memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->mode[prof->mode_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->mode[prof->mode_cnt].selected = 1;
			prof->mode_cnt++;
		}
	}
	{
		if(prof->blc_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE, NF_IPCAM_BLC_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				IPCAM_DBG(WARN, "CH(%d) Exp ONVIF BLC DUMMY\n", ch);
				memset(prof, 0x00, sizeof(NFIPCamExposureProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->blc[prof->blc_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->blc[prof->blc_cnt].selected = 1;
			prof->blc_cnt++;
		}
	}
	{
		if(prof->wdr_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_exposure(NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE, NF_IPCAM_WDR_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				IPCAM_DBG(WARN, "CH(%d) Exp ONVIF WDR DUMMY\n", ch);
				memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->wdr[prof->wdr_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->wdr[prof->wdr_cnt].selected = 1;
			prof->wdr_cnt++;
		}
	}
	{
		if(prof->ircut_cnt == 0)
		{
			NFIPCamOption_onvif *get_tuple;
			get_tuple = get_cam_model_option_image(NF_IPCAM_EXPOSURE_ONVIF_IRCUT, NF_IPCAM_IRCUT_MODE_DUMMY);
			if(get_tuple == NULL)
			{
				IPCAM_DBG(WARN, "CH(%d) Exp ONVIF IRCUT DUMMY\n", ch);
				memset(prof, 0x00, sizeof(NFIPCamImageProfile_onvif));
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(&prof->ircut[prof->ircut_cnt], get_tuple, sizeof(NFIPCamOption_onvif));
			prof->ircut[prof->ircut_cnt].selected = 1;
			prof->ircut_cnt++;
		}
	}

#if 0
	puts("\e[31m");
	printf("[%s:%d] CH(%d) prof->supported_export = (0x%llx)\n", __FUNCTION__, __LINE__,ch, prof->supported_exposure);
	puts("\e[0mm");
	// DEBUG
	puts("===============================================================");
	puts("WDR");
	puts("===============================================================");

	for(i = 0; i < prof->wdr_cnt; i++)
	{
		view_ipcam_option(&prof->wdr[i]);
	}
	puts("\n");

	// DEBUG
	puts("===============================================================");
	puts("DEFOG");
	puts("===============================================================");

	for(i = 0; i < prof->defog_cnt; i++)
	{
		view_ipcam_option(&prof->defog[i]);
	}
	puts("\n");

	// DEBUG
	puts("===============================================================");
	puts("HLC");
	puts("===============================================================");

	for(i = 0; i < prof->hlc_cnt; i++)
	{
		view_ipcam_option(&prof->hlc[i]);
	}
	puts("\n");
	
	puts("===============================================================");
	puts("BLC");
	puts("===============================================================");

	for(i = 0; i < prof->blc_cnt; i++)
	{
		view_ipcam_option(&prof->blc[i]);
	}
	puts("\n");
	

	puts("===============================================================");
	puts("Anti-Flicker");
	puts("===============================================================");

	for(i = 0; i < prof->antiflicker_cnt; i++)
	{
		view_ipcam_option(&prof->antiflicker[i]);
	}
	puts("\n");
	
	puts("===============================================================");
	puts("IR Cut Filter");
	puts("===============================================================");

	for(i = 0; i < prof->ircut_cnt; i++)
	{
		view_ipcam_option(&prof->ircut[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("IR Cut Filter Manual Mode");
	puts("===============================================================");

	for(i = 0; i < prof->ircutm_cnt; i++)
	{
		view_ipcam_option(&prof->ircutm[i]);
	}
	puts("\n");


	puts("===============================================================");
	puts("Exposuer Mode");
	puts("===============================================================");

	for(i = 0; i < prof->mode_cnt; i++)
	{
		view_ipcam_option(&prof->mode[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("DNR");
	puts("===============================================================");

	for(i = 0; i < prof->dnr_cnt; i++)
	{
		view_ipcam_option(&prof->dnr[i]);
	}
	puts("\n");


	puts("===============================================================");
	puts("Adaptive IR");
	puts("===============================================================");

	for(i = 0; i < prof->adaptive_ir_cnt; i++)
	{
		view_ipcam_option(&prof->adaptive_ir[i]);
	}
	puts("\n");

	
	puts("===============================================================");
	puts("Max Shutter 60");
	puts("===============================================================");

	for(i = 0; i < prof->max_shutter_60_cnt; i++)
	{
		view_ipcam_option(&prof->max_shutter_60[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("Max Shutter 50");
	puts("===============================================================");

	for(i = 0; i < prof->max_shutter_50_cnt; i++)
	{
		view_ipcam_option(&prof->max_shutter_50[i]);
	}
	puts("\n");

	
	puts("===============================================================");
	puts("Max AGC");
	puts("===============================================================");

	for(i = 0; i < prof->maxagc_cnt; i++)
	{
		view_ipcam_option(&prof->maxagc[i]);
	}
	puts("\n");

	puts("===============================================================");
	puts("Slow Shutter");
	puts("===============================================================");

	for(i = 0; i < prof->slowshutter_cnt; i++)
	{
		view_ipcam_option(&prof->slowshutter[i]);
	}
	puts("\n");

	// DEBUG
	puts("===============================================================");
	puts("IRIS");
	puts("===============================================================");

	for(i = 0; i < prof->dc_iris_cnt; i++)
	{
		view_ipcam_option(&prof->dc_iris[i]);
	}
	puts("\n");
	
#endif

	IPCAM_DBG(MINOR,"prof->supported_exposure = (%llu)\n", prof->supported_exposure);
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 iris모드 지원여부 및 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] capable Iris지원여부.
 * @param[out] current Iris 현재값.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated Image profile조회로 변경됨.
 */
int nf_ipcam_get_iris_mode(gint ch, guint* capable, guint* current, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	*capable = runtime[ch].image.iris.support;
	*current = runtime[ch].image.iris.value;

	//IPCAM_DBG(MINOR, "end CH(%d) capable(%08x) current(%08x)\n", ch, *capable, *current);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 day/night toggletime 지원여부 및 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] capable Day/night toggletime지원여부.
 * @param[out] current Day/night toggletime 현재값.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated Image profile조회로 변경됨.
 */
int nf_ipcam_get_dnn_toggle_mode(gint ch, guint* capable, guint* current, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	*capable = runtime[ch].image.tg_time.support;
	*current = runtime[ch].image.tg_time.value;

	//IPCAM_DBG(MAJOR, "end CH(%d) capable(%08x) current(%08x)\n", ch, *capable, *current);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 접속 상태를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] port_status 카메라의 접속 상태.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_port_status(gint ch, NFIPCamPortStatus* port_status, GError **error)
{
	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail(ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch < AVAILABLE_MAX_CH, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(port_status != NULL, IPCAM_SETUP_RTN_FAILED);

	port_status->status = _port_status[ch].status;
	port_status->device_class = _port_status[ch].device_class;
	//snprintf(port_status->vendor, 64, _port_status[ch].vendor);
	memset(port_status->vendor, 0x00, 64);
	snprintf(port_status->model, 64, _port_status[ch].model);
	snprintf(port_status->detail, 256, _port_status[ch].detail);

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 연결 상태를 조회한다.
 * @param[out] status 카메라 전 채널의 연결 상태(array).
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @warning Status 파라메터가 채널수만큼 memory 할당이 되어있어야 함.
 *
 * @see PND_TYPE_UNPLUGGED
 */
int nf_ipcam_get_pnd_status(gint *status)
{
	int i = 0;

	//IPCAM_DBG(MAJOR, "start\n");
	g_return_val_if_fail(status != NULL, IPCAM_SETUP_RTN_FAILED);

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		*(status + i) = get_pnd_status(i);
	}

	//IPCAM_DBG(MAJOR, "end\n");
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 전 채널의 카메라 연결을 해제한다.
 * @return 연결 해제한 카메라 건수.
 */
int nf_ipcam_is_all_ch_unplugged()
{
	int i = 0;
	int plugged_ch_cnt = 0;
	mtable *runtime = get_runtime();
	gint ipcam_st[32];

	//IPCAM_DBG(MAJOR, "start\n");

	nf_ipcam_get_pnd_status(ipcam_st);
	for (i = 0; i < NUM_ACTIVE_CH; i++) {
		if (ipcam_st[i] != PND_TYPE_UNPLUGGED)
		{
			if (nf_get_running_mode() == 0)
			{
				nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
			}
			plugged_ch_cnt++;
		}
	}

	IPCAM_DBG(MAJOR, "end result(%d)\n", plugged_ch_cnt);
	return plugged_ch_cnt++;
}

/**
 * @brief 카메라별 NTSC/PAL 설정을 조회한다.
 * @param mode_mask PAL로 설정된 채널의 비트 마스크.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 * @deprecated
 */
int nf_ipcam_get_af_mode(guint *mode_mask)
{
	int i = 0;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start\n");

	runtime = get_runtime();
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( mode_mask != NULL, IPCAM_SETUP_RTN_FAILED);

	*mode_mask = 0;
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].video.anti_flicker.value == NF_IPCAM_AF_PAL)
		{
			*mode_mask |= (0x1<<i);
		}
	}

	//IPCAM_DBG(MAJOR, "end\n");
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 해상도 지원여부 및 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] stream 스트림 번호(0, 1).
 * @param[out] capable 지원 해상도 비트마스크.
 * @param[out] current 현재 설정된 해상도.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @see NF_IPCAM_RES_E
 */
int nf_ipcam_get_resol(gint ch, gint stream, 
		uint64_t* capable, uint64_t* current, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) stream(%d)\n", ch, stream);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(! (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	*capable = runtime[ch].video.resolution.supported;
	*current = runtime[ch].video.resolution.resolution[stream];

	if (*capable == 0)
	{
		IPCAM_DBG(WARN, "initial setup going CH(%d)\n", ch);
		return IPCAM_SETUP_RTN_FAILED;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) stream(%d) capable(%08x) current(%08x)\n",
	//		ch, stream, *capable, *current);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 해상도 지원여부 및 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] stream 스트림 번호(0, 1).
 * @param[out] capable 지원 해상도 비트마스크.
 * @param[out] current 현재 설정된 해상도.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @see NF_IPCAM_RES_E
 */
int nf_ipcam_get_ch_stream_ratio(gint ch, gint stream, gint *width_ratio, gint *height_ratio)
{
	int width = 0, height = 0;
	uint64_t current = 0;
	int ow, oh;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) stream(%d)\n", ch, stream);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( width_ratio != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( height_ratio != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( stream < MAX_VIDEO_STREAM, IPCAM_SETUP_RTN_FAILED);

	*width_ratio = 0;
	*height_ratio = 0;

	if(! (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0)
		return IPCAM_SETUP_RTN_FAILED;

	current = runtime[ch].video.resolution.resolution[stream];

	if (current == 0) {
		IPCAM_DBG(WARN, "resolution value is 0, ch(%d) stream(%d)\n", stream);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if(nf_ipcam_change_ipcamres_to_size(current, &width, &height) == IPCAM_SETUP_RTN_DONE) {
		if(_get_stream_ratio(width, height, &ow, &oh) == IPCAM_SETUP_RTN_DONE) {
			*width_ratio = ow;
			*height_ratio = oh;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 해상도 비율 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] stream 스트림 번호(0, 1).
 * @param[out] capable 지원 해상도 비트마스크.
 * @param[out] current 현재 설정된 해상도.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @see NF_IPCAM_RES_E
 */
int nf_ipcam_get_resol_ratio(gint ch, gint stream, 
		uint64_t* capable, uint64_t* current, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) stream(%d)\n", ch, stream);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(! (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	*current = runtime[ch].video.resolution.resolution[stream];

	if (*current == 0)
	{
		IPCAM_DBG(WARN, "initial setup going CH(%d)\n", ch);
		return IPCAM_SETUP_RTN_FAILED;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) stream(%d) capable(%08x) current(%08x)\n",
	//		ch, stream, *capable, *current);
	return IPCAM_SETUP_RTN_DONE;
}

/*
 * @brief 현재 카메라가 지원하는 1st stream 최대 해상도의 비율이 몇인지 확인한다.
 * @param[in] ch 채널 번호
 * @param[in] stream 스트림 번호
 * @return RESOL_16_9, RESOL_4_3, RESOL_5_4, RESOL_1_1 (define enum)
 */

extern int nf_ipcam_get_max_resol_ratio(int ch)
{
	int i=0;
	uint64_t res=0;
	int result = RESOL_1_1; // default
	mtable *runtime = get_runtime();

	// 최대 pixel 순서대로 정렬된 테이블
	uint64_t res_table[44] = {
		SHIFT_64_16,
		SHIFT_64_00,
		SHIFT_64_01,
		SHIFT_64_02,
		SHIFT_64_43,
		SHIFT_64_03,
		SHIFT_64_15,
		SHIFT_64_17,
		SHIFT_64_18,
		SHIFT_64_04,
		SHIFT_64_05,
		SHIFT_64_07,
		SHIFT_64_19,
		SHIFT_64_06,
		SHIFT_64_42,
		SHIFT_64_08,
		SHIFT_64_21,
		SHIFT_64_11,
		SHIFT_64_09,
		SHIFT_64_10,
		SHIFT_64_20,
		SHIFT_64_12,
		SHIFT_64_41,
		SHIFT_64_22,
		SHIFT_64_13,
		SHIFT_64_14,
		SHIFT_64_23,
		SHIFT_64_24,
		SHIFT_64_25,
		SHIFT_64_38,
		SHIFT_64_26,
		SHIFT_64_27,
		SHIFT_64_40,
		SHIFT_64_28,
		SHIFT_64_29,
		SHIFT_64_30,
		SHIFT_64_31,
		SHIFT_64_32,
		SHIFT_64_33,
		SHIFT_64_34,
		SHIFT_64_35,
		SHIFT_64_36,
		SHIFT_64_37,
		SHIFT_64_39,
	};

	// res_table
	

	if(ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s][%d] Not support ch(%d) \n", __func__, __LINE__, ch);
	}

	// 최대 해상도 뽑기
	for(i=0; i<NF_IPCAM_RES_MAX; i++)
	{
		if(runtime[ch].encoder.res_support[0] & res_table[i])
		{
			res = res_table[i];
		}
	}

	// 우선 4:3 과 1:1을 체크하고, 그 외는 16:9로 반환
	// 추가 요청사항이 왔을 시, 이 부분을 디테일하게 수정 필요
	//
	switch(res)
	{
		case SHIFT_64_36: // 3200x2400
		case SHIFT_64_34: // 2880x2160
		case SHIFT_64_30: // 2592x1944
		case SHIFT_64_29: // 2592x1920
		case SHIFT_64_28: // 2560x1920
		case SHIFT_64_24: // 1048x1536
		case SHIFT_64_22: // 1600x1200
		case SHIFT_64_21: // 800x600
		case SHIFT_64_12: // 1280x1024
		case SHIFT_64_11: // 1024x768
		case SHIFT_64_08: // 720x576
			result = RESOL_4_3;
			break;

	case SHIFT_64_43: // 320x320
	case SHIFT_64_42: // 640x640
	case SHIFT_64_41: // 1280x1280
	case SHIFT_64_40: // 2048x2048
	case SHIFT_64_39: // 3000x3000
		result = RESOL_1_1;
		break;

	default:
		result = RESOL_16_9;
		break;
	}

	return result;
}


/**
 * @brief 현재 카메라의 FPS 지원여부 및 값을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[in] stream 스트림 번호(0, 1).
 * @param[out] capable 지원 fps 비트마스크.
 * @param[out] current 현재 설정된 fps.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @see NF_IPCAM_FPS_E
 */
int nf_ipcam_get_fps(gint ch, gint stream, 
		guint* capable, guint* current, GError **error)
{
	int ntpal;
	mtable* runtime = NULL;

	/* default fps */
	modes default_fps[2][3] = {
		{	// ntsc
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_150|NF_IPCAM_FPS_70|NF_IPCAM_FPS_10, 0
			},
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_150|NF_IPCAM_FPS_70|NF_IPCAM_FPS_10, 0
			},
			{
				0, 0
			}
		},
		{	// pal
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_120|NF_IPCAM_FPS_60|NF_IPCAM_FPS_10, 0
			},
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_120|NF_IPCAM_FPS_60|NF_IPCAM_FPS_10, 0
			},
			{
				0, 0
			}
		}
	};

	/* default fps (virtual camera) */
	modes default_fps_vcam[2][3] = {
		{	// ntsc
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_10, NF_IPCAM_FPS_300
			},
			{
				NF_IPCAM_FPS_300|NF_IPCAM_FPS_10, NF_IPCAM_FPS_300
			},
			{
				0, 0
			}
		},
		{	// pal
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_10, NF_IPCAM_FPS_250
			},
			{
				NF_IPCAM_FPS_250|NF_IPCAM_FPS_10, NF_IPCAM_FPS_250
			},
			{
				0, 0
			}
		}
	};
	//IPCAM_DBG(MAJOR, "start CH(%d) stream(%d)\n", ch, stream);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0, IPCAM_SETUP_RTN_FAILED);

	if (runtime[ch].state & MGMT_STATE_CONFIGURED)
	{
		char key[64];
		int is_vcam;

		snprintf(key, 63, "cam.logininfo.L%d.vcam", ch);
		is_vcam = nf_sysdb_get_uint(key);

		if(is_vcam)
		{
			ntpal = get_tv_system();
			*capable = default_fps_vcam[ntpal][stream].support;
			*current = default_fps_vcam[ntpal][stream].value;
		}
		else
		{
			ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
			*capable = runtime[ch].video.fps[ntpal][stream].support;
			*current = runtime[ch].video.fps[ntpal][stream].value;
		}

		if (*capable == 0)
		{
			IPCAM_DBG(WARN, "initial setup going CH(%d)\n", ch);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	else
	{
		ntpal = get_tv_system();
		*capable = default_fps[ntpal][stream].support;
		*current = default_fps[ntpal][stream].value;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) stream(%d) capable(%08x) current(%08x)\n",
	//		ch, stream, *capable, *current);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 모델 정보를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] model_info 카메라 모델 정보 struct.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_model_info(gint ch, NFIPCamModelInfo* model_info, GError **error)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( model_info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	
	memcpy(model_info->mac, runtime[ch].sys.macaddr, 6);
	strncpy(model_info->name, runtime[ch].sys.model,
			sizeof(runtime[ch].sys.model));
	if (runtime[ch].sys.swver2[0] != '\0')
	{
		strncpy(model_info->swver, runtime[ch].sys.swver2,
				sizeof(runtime[ch].sys.swver2));
	}
	else
	{
		strncpy(model_info->swver, runtime[ch].sys.swver,
				sizeof(runtime[ch].sys.swver));
	}
#if 1
	strncpy(model_info->vendor, runtime[ch].sys.vendor,
			sizeof(runtime[ch].sys.vendor));
#else
	memset(model_info->vendor, 0x00, 64);
#endif

#if 0
	IPCAM_DBG(MINOR, "mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
			model_info->mac[0], model_info->mac[1],
			model_info->mac[2], model_info->mac[3],
			model_info->mac[4], model_info->mac[5]);
	IPCAM_DBG(MINOR, "model(%s)\n", model_info->name);
	IPCAM_DBG(MINOR, "fw ver(%s)\n", model_info->swver);
	IPCAM_DBG(MINOR, "vendor(%s)\n", model_info->vendor);

	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 현재 카메라의 렌즈설정 capability를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] afcapa_info 렌즈설정 capability struct.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_af_capability(gint ch, NFIPCamAFCapa* afcapa_info, GError **error)
{
	mtable *runtime = NULL;
	cam_info info;
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	int (*func44)(cam_info*, int);

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( afcapa_info != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].funcs[NF_IPCAM_TYPE_GET_LENSCAP] != NULL,
			IPCAM_SETUP_RTN_FAILED);

	memset(&info, 0x00, sizeof(cam_info));
	memset(afcapa_info, 0x00, sizeof(NFIPCamAFCapa));

	if (runtime[ch].funcs[NF_IPCAM_TYPE_GET_LENSCAP] != NULL)
	{
		func44 = runtime[ch].funcs[NF_IPCAM_TYPE_GET_LENSCAP];
		rtn = func44(&info, ch);
	}

	if (rtn != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(WARN, "AF CAPABILITY get fail CH(%d)\n", ch);
		return IPCAM_SETUP_RTN_FAILED;
	}

	afcapa_info->mfz = info.afcapa.mfz;
	afcapa_info->iris_mode = info.afcapa.iris;
	afcapa_info->zoom_min = info.afcapa.zoom_min;
	afcapa_info->zoom_max = info.afcapa.zoom_max;
	afcapa_info->focus_min = info.afcapa.focus_min;
	afcapa_info->focus_max = info.afcapa.focus_max;
	afcapa_info->iris_min = info.afcapa.iris_min;
	afcapa_info->iris_max = info.afcapa.iris_max;
	afcapa_info->iris = info.afcapa.iris;

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 접속 정보 등을 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] conf_info 카메라 접속 정보 struct.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_config_info(gint ch, NFIPCamConfInfo* conf_info, GError **error)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( conf_info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!(runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	conf_info->configurable = 0;
	snprintf(conf_info->ipaddr, 16, "%d.%d.%d.%d",
					( runtime[ch].sys.ipaddr&0xff),
					((runtime[ch].sys.ipaddr&0xff00)>>8),
					((runtime[ch].sys.ipaddr&0xff0000)>>16),
					((runtime[ch].sys.ipaddr&0xff000000)>>24));
	snprintf(conf_info->username, 64, "%s", runtime[ch].username);
	snprintf(conf_info->password, 64, "%s", runtime[ch].password);
	conf_info->http_port = runtime[ch].sys.http_port;
	conf_info->rtsp_port = runtime[ch].sys.rtsp_port[0];
	conf_info->protocol = runtime[ch].sys.rx_method;
	conf_info->video = (runtime[ch].state & MGMT_STATE_CONFIGURED) ? 1 : 0;
	conf_info->audio_in = (runtime[ch].func & NF_IPCAM_FUNC_AUDIO_RX) ? 1 : 0;
	conf_info->audio_out = (runtime[ch].func & NF_IPCAM_FUNC_AUDIO_TX) ? 1 : 0;;
	conf_info->alarm_in = (runtime[ch].func & NF_IPCAM_FUNC_ALARM_IN) ? 1 : 0;
	conf_info->alarm_out = (runtime[ch].func & NF_IPCAM_FUNC_ALARM_OUT) ? 1 : 0;
	
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 모션 설정 정보를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] info 카메라 모션 설정 정보 struct.
 * @param[out] error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_motion_profile(gint ch, NFIPCamMotionProfile* info, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	info->sensitivity_min		= runtime[ch].motion.sensitivity.min;
	info->sensitivity_max		= runtime[ch].motion.sensitivity.max;
	//info->block_width			= runtime[ch].motion.block_width;
	//info->block_height			= runtime[ch].motion.block_height;
	if(runtime[ch].video.corridor_support == 1)
	{
		if(runtime[ch].video.corridor_mode_value == 0)
		{
			if(runtime[ch].motion.block_width > runtime[ch].motion.block_height)
			{
				info->block_width = runtime[ch].motion.block_width;
				info->block_height = runtime[ch].motion.block_height;
			}
			else
			{
				info->block_width = runtime[ch].motion.block_height;
				info->block_height = runtime[ch].motion.block_width;
			}
		}
		else
		{
			if(runtime[ch].motion.block_width > runtime[ch].motion.block_height)
			{
				info->block_width = runtime[ch].motion.block_height;
				info->block_height = runtime[ch].motion.block_width;
			}
			else
			{
				info->block_width = runtime[ch].motion.block_width;
				info->block_height = runtime[ch].motion.block_height;
			}
		}
	}
	else
	{
		info->block_width			= runtime[ch].motion.block_width;
		info->block_height			= runtime[ch].motion.block_height;
	}
	
	info->min_block				= runtime[ch].motion.min_block;
	info->num_blocks			= runtime[ch].motion.num_block;
	info->area_method			= runtime[ch].motion.method;
	info->smart_motion_support	= runtime[ch].motion.smart_motion_support;

	memcpy(info->smart_motion_options, runtime[ch].motion.smart_motion_options, sizeof(info->smart_motion_options));
	info->smart_motion_option_size = runtime[ch].motion.smart_motion_option_size;

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
		smart_motion_capa_to_db_string(info->db_str, sizeof(info->db_str),
				info->smart_motion_options,info->smart_motion_option_size);
	}
	else
	{
		memset(info->db_str, 0x00, 256);
	}

	if(info->area_method == MAM_SIM_RECTANGLE)
	{
		info->area_method = MAM_RECTANGLE;
	}
	info->rect_num			= runtime[ch].motion.max_rect;

	if(runtime[ch].sys.model_code >= NF_IPCAM_MODEL_ONVIF && runtime[ch].sys.model_code <= NF_IPCAM_MODEL_ONVIF_GRUNDIG)
	{
		info->is_itx_cam 	= 0;
	}
	else
	{
		info->is_itx_cam 	= 1;
	}
#if 0
	IPCAM_DBG(MINOR, "sensitivity min value(%d)\n", info->sensitivity_min);
	IPCAM_DBG(MINOR, "sensitivity max value(%d)\n", info->sensitivity_max);
	IPCAM_DBG(MINOR, "block width(%d)\n", info->block_width);
	IPCAM_DBG(MINOR, "block height(%d)\n", info->block_height);
	IPCAM_DBG(MINOR, "min_block(%d)\n", info->min_block);
	IPCAM_DBG(MINOR, "num_blocks(%d)\n", info->num_blocks);
	IPCAM_DBG(MINOR, "area_method(%d)\n", info->area_method);
	IPCAM_DBG(MINOR, "rect_num(%d)\n", info->rect_num);
	IPCAM_DBG(MINOR, "is_itx_cam(%d)\n", info->is_itx_cam);
#endif
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 사생활 보호 설정 정보를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] info 카메라 사생활 보호 설정 정보 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @todo 현재 NPT카메라는 시나리오 문제로 지원 안하며, NPT 지원하기 위해서는 sysdb및 UI등 변경 필요.
 */
int nf_ipcam_get_privacy_mask_profile(gint ch, NFIPCamPrivacyMaskProfile* info)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	info->ch = ch;

	if (strncmp(runtime[ch].sys.stdver, "NPT", 3) == 0)
	{
		info->is_support = 0;
	}
	else
	{
		info->is_support = (runtime[ch].func & NF_IPCAM_FUNC_PRIVACY_MASK) ? 1:0;
		//info->block_width = runtime[ch].privacymask.block_width;
		//info->block_height = runtime[ch].privacymask.block_height;
		if(runtime[ch].video.corridor_support == 1) // (SWPFOURCE-970)
		{
			if(runtime[ch].video.corridor_mode_value == 0)  //16:9
			{
				if(runtime[ch].privacymask.block_width > runtime[ch].privacymask.block_height) // (SWPFOURCE-958 이슈가 해결되면 수정 필요)
				{
					info->block_width = runtime[ch].privacymask.block_width;
					info->block_height = runtime[ch].privacymask.block_height;
				}
				else
				{
					info->block_width = runtime[ch].privacymask.block_height;
					info->block_height = runtime[ch].privacymask.block_width;
				}

			}
			else // need converting 9:16
			{
				if(runtime[ch].privacymask.block_width > runtime[ch].privacymask.block_height)
				{
					info->block_width = runtime[ch].privacymask.block_height;
					info->block_height = runtime[ch].privacymask.block_width;
				}
				else
				{
					info->block_width = runtime[ch].privacymask.block_width;
					info->block_height = runtime[ch].privacymask.block_height;
				}
			}
		}
		else
		{
			info->block_width = runtime[ch].privacymask.block_width;
			info->block_height = runtime[ch].privacymask.block_height;
		}
		info->area_num = runtime[ch].privacymask.max_rect;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 ROI 설정 정보를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] info 카메라 사생활 보호 설정 정보 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_get_roi_area_profile(gint ch, NFIPCamROIAreaProfile* info)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(info, 0x00, sizeof(NFIPCamROIAreaProfile));

	info->isSupport = (runtime[ch].func & NF_IPCAM_FUNC_ROI) ? 1:0;
	info->ch = ch;
	//info->block_width = runtime[ch].roi_area.block_width;
	//info->block_height = runtime[ch].roi_area.block_height;
	if(runtime[ch].video.corridor_support == 1) // (SWPFOURCE-970)
		{
			if(runtime[ch].video.corridor_mode_value == 0)  //16:9
			{
				if(runtime[ch].privacymask.block_width > runtime[ch].privacymask.block_height) // (SWPFOURCE-958 이슈가 해결되면 수정 필요)
				{
					info->block_width = runtime[ch].privacymask.block_width;
					info->block_height = runtime[ch].privacymask.block_height;
				}
				else
				{
					info->block_width = runtime[ch].privacymask.block_height;
					info->block_height = runtime[ch].privacymask.block_width;
				}

			}
			else // need converting 9:16
			{
				if(runtime[ch].privacymask.block_width > runtime[ch].privacymask.block_height)
				{
					info->block_width = runtime[ch].privacymask.block_height;
					info->block_height = runtime[ch].privacymask.block_width;
				}
				else
				{
					info->block_width = runtime[ch].privacymask.block_width;
					info->block_height = runtime[ch].privacymask.block_height;
				}
			}
		}
		else
		{
			info->block_width = runtime[ch].privacymask.block_width;
			info->block_height = runtime[ch].privacymask.block_height;
		}
	info->area_num = runtime[ch].roi_area.max_rect;
	info->chipset = runtime[ch].roi_area.chipset;

	//chipset option check 
	if((info->chipset == NF_IPCAM_HISILICON_CHIPSET_3516A) || (info->chipset == NF_IPCAM_HISILICON_CHIPSET_3516D))
	{
		if(runtime[ch].roi_area.options == 0)
		{
			info->isOption |= ROI_OFF;
			info->isOption |= ROI_MANUAL;
			info->isOption |= ROI_AUTO;
		}
		else
			info->isOption = runtime[ch].roi_area.options;
	}
	else if(info->chipset == NF_IPCAM_HISILICON_CHIPSET_3516C)
	{
		if(runtime[ch].roi_area.options == 0)
		{
			info->isOption |= ROI_OFF;
			info->isOption |= ROI_MANUAL;
		}
		else
			info->isOption = runtime[ch].roi_area.options;
	}
	else
		info->isOption =0;

	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

int nf_ipcam_get_corridor_mode(gint ch, gint* value)
{
	mtable * runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);

	/*
	if(! (runtime[ch].state & MGMT_STATE_CONFIGURED) != 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	*/

	//*min = runtime[ch].video.corridor_mode_min;
	//*max = runtime[ch].video.corridor_mode_max;
	*value = runtime[ch].video.corridor_mode_value;
	//*support = runtime[ch].video.corridor_support;

	return IPCAM_SETUP_RTN_DONE;
}

int nf_ipcam_get_is_supported_corridor(gint ch)
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);

	if(runtime[ch].video.corridor_support == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/**
 * @brief 카메라의 SSL사용 여부를 조회한다.(S1)
 * @param ch 채널 번호.
 * @return SSL 사용 여부.
 */
int nf_ipcam_is_using_ssl(gint ch)
{
	mtable *runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, 0);

	return (runtime[ch].sys.use_ssl);
}

/**
 * @brief NVR의 audio out으로 재생하고자 하는 카메라 채널을 설정한다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_live_audio_ch(gint ch)
{
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	nmf_mrtp_pipe_set_live_audio_ch(_h_mrtp_pipe, ch);

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 audio out으로 오디오를 전달한다.
 * @param[in] ch 채널 번호.
 * @param[in] audio 오디오 rawdata를 포함한 struct.
 * @param[out] rtn 실제 전송한 byte수.
 * @param[in] user_data user data.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_send_audio(gint ch, NFIPCamAudioRaw* audio, gint *rtn, gpointer user_data)
{
	mtable *runtime = NULL;

	//IPCAM_DBG("[%s] ch(%d) type(%d)\n", __FUNCTION__, ch, audio->type);
	runtime = get_runtime();

	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if (rtn == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0) { return IPCAM_SETUP_RTN_FAILED; }
	if ((runtime[ch].func & NF_IPCAM_FUNC_AUDIO_RX) == 0) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime[ch].audio.audio_rx == 0) { return IPCAM_SETUP_RTN_FAILED; }

	*rtn = nmf_mrtp_pipe_send_audio(_h_mrtp_pipe, audio, user_data);
	//IPCAM_DBG("[%s] send bytes(%d)\n", __FUNCTION__, *rtn);

	return IPCAM_SETUP_RTN_DONE;
}


/**
 * @brief NF_IPCAM_RES_ define 을 해상도 width, height 로 변환.
 * @param[in] ch 채널 번호.
 * @param[out] width 해상도의 width.
 * @param[out] height 해상도의 height.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @see NF_IPCAM_RES_
 */
int nf_ipcam_change_ipcamres_to_size(const uint64_t camres, unsigned int *width, unsigned int *height)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	if(width != NULL && height != NULL) {
		if(camres == NF_IPCAM_RES_320x240) {
			*width = 320; *height = 240;
		} else if(camres == NF_IPCAM_RES_352x240) {
			*width = 352; *height = 240;
		} else if(camres == NF_IPCAM_RES_352x288) {
			*width = 352; *height = 288;
		} else if(camres == NF_IPCAM_RES_640x352) {
			*width = 640; *height = 352;
		} else if(camres == NF_IPCAM_RES_640x480) {
			*width = 640; *height = 480;
		} else if(camres == NF_IPCAM_RES_704x480) {
			*width = 704; *height = 480;
		} else if(camres == NF_IPCAM_RES_704x576) {
			*width = 704; *height = 576;
		} else if(camres == NF_IPCAM_RES_720x480) {
			*width = 720; *height = 480;
		} else if(camres == NF_IPCAM_RES_720x576) {
			*width = 720; *height = 576;
		} else if(camres == NF_IPCAM_RES_1280x720I) {
			*width = 1280; *height = 720;
		} else if(camres == NF_IPCAM_RES_1280x720) {
			*width = 1280; *height = 720;
		} else if(camres == NF_IPCAM_RES_1024x768) {
			*width = 1024; *height = 768;
		} else if(camres == NF_IPCAM_RES_1280x1024) {
			*width = 1280; *height = 1024;
		} else if(camres == NF_IPCAM_RES_1920x1080I) {
			*width = 1920; *height = 1080;
		} else if(camres == NF_IPCAM_RES_1920x1080) {
			*width = 1920; *height = 1080;
		} else if(camres == NF_IPCAM_RES_640x360) {
			*width = 640; *height = 360;
		} else if(camres == NF_IPCAM_RES_320x180) {
			*width = 320; *height = 180;
		} else if(camres == NF_IPCAM_RES_640x360I) {
			*width = 640; *height = 360;
		} else if(camres == NF_IPCAM_RES_640x400) {
			*width = 640; *height = 400;
		} else if(camres == NF_IPCAM_RES_800x450) {
			*width = 800; *height = 450;
		} else if(camres == NF_IPCAM_RES_1440x900) {
			*width = 1440; *height = 900;
		} else if(camres == NF_IPCAM_RES_800x600) {
			*width = 800; *height = 600;
		} else if(camres == NF_IPCAM_RES_1600x1200) {
			*width = 1600; *height = 1200;
		} else if(camres == NF_IPCAM_RES_2304x1296) {
			*width = 2304; *height = 1296;
		} else if(camres == NF_IPCAM_RES_2048x1536) {
			*width = 2048; *height = 1536;
		} else if(camres == NF_IPCAM_RES_2560x1440) {
			*width = 2560; *height = 1440;
		} else if(camres == NF_IPCAM_RES_2688x1520) {
			*width = 2688; *height = 1520;
		} else if(camres == NF_IPCAM_RES_2560x1600) {
			*width = 2560; *height = 1600;
		} else if(camres == NF_IPCAM_RES_2560x1920) {
			*width = 2560; *height = 1920;
		} else if(camres == NF_IPCAM_RES_2592x1920) {
			*width = 2592; *height = 1920;
		} else if(camres == NF_IPCAM_RES_2592x1944) {
			*width = 2592; *height = 1944;
		} else if(camres == NF_IPCAM_RES_2992x1680) {
			*width = 2992; *height = 1680;
		} else if(camres == NF_IPCAM_RES_2880x1800) {
			*width = 2880; *height = 1800;
		} else if(camres == NF_IPCAM_RES_3200x1800) {
			*width = 3200; *height = 1800;
		} else if(camres == NF_IPCAM_RES_2880x2160) {
			*width = 2880; *height = 2160;
		} else if(camres == NF_IPCAM_RES_3072x2048) {
			*width = 3072; *height = 2048;
		} else if(camres == NF_IPCAM_RES_3200x2400) {
			*width = 3200; *height = 2400;
		} else if(camres == NF_IPCAM_RES_3840x2160) {
			*width = 3840; *height = 2160;
		} else if(camres == NF_IPCAM_RES_2592x1520) {
			*width = 2592; *height = 1520;
		} else if(camres == NF_IPCAM_RES_3000x3000) {
			*width = 3000; *height = 3000;
		} else if(camres == NF_IPCAM_RES_2048x2048) {
			*width = 2048; *height = 2048;
		} else if(camres == NF_IPCAM_RES_1280x1280) {
			*width = 1280; *height = 1280;
		} else if(camres == NF_IPCAM_RES_640x640) {
			*width = 640; *height = 640;
		} else if(camres == NF_IPCAM_RES_320x320) {
			*width = 320; *height = 320;
		} else {
			*width = 0; *height = 0;
		}
		rtn = IPCAM_SETUP_RTN_DONE;
	}
	else {
		rtn = IPCAM_SETUP_RTN_FAILED;
	}
	return rtn;
}

static int _get_stream_ratio(int iw, int ih,  int *ow, int *oh)
{
	int width = 0, height = 0, max = 0, min = 0, gcd = 0, temp = 0;
	int rtn = IPCAM_SETUP_RTN_FAILED;

	if(iw <= 0 || ih <= 0) {
		return -1;
	}

	if(ow != NULL && oh != NULL) {

		width = iw;
		height = ih;
		if(width < height)
		{ 
			max = width;      
			min = height;
		} else
		{
			max = height;
			min = width;
		}

		while(max%min != 0) 
		{  
			temp = max % min;  
			max = min;         
			min = temp;        

		}

		gcd = min;             

		*ow = width/gcd;      
		*oh = height/gcd;

		rtn = IPCAM_SETUP_RTN_DONE;
	}
	else {
		rtn = IPCAM_SETUP_RTN_FAILED;
	}
	return rtn;
}

int nf_ipcam_get_hisilicon_chipset_code(gint ch)
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	
	if (strncmp(runtime[ch].sys.swver, "40300", 5) == 0)// 3516A
	{
		return NF_IPCAM_HISILICON_CHIPSET_3516A;
	}
	else if (strncmp(runtime[ch].sys.swver, "38100", 5) == 0) //3516C
	{
		return NF_IPCAM_HISILICON_CHIPSET_3516C;
	}
	else if(strncmp(runtime[ch].sys.swver, "48100", 5) == 0) // 3516D 
	{
		return NF_IPCAM_HISILICON_CHIPSET_3516D;
	}
	else // not hisilicon camera
	{
		return -1;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
}

static unsigned char _ti_motion_map(unsigned char k, const int min, const int max)
{
	unsigned char rtn = 0;
	if (max == 0x1e)
	{
		if (k>27) { rtn=10; }
		else if (k>24) { rtn=9; }
		else if (k>21) { rtn=8; }
		else if (k>18) { rtn=7; }
		else if (k>15) { rtn=6; }
		else if (k>12) { rtn=5; }
		else if (k>10) { rtn=4; }
		else if (k>8) { rtn=3; }
		else if (k>5) { rtn=2; }
		else if (k>3) { rtn=1; }
		else { rtn=0; }
	}
	else
	{
		if (k>0xf0) { rtn=10; }
		else if (k>0xd0) { rtn=9; }
		else if (k>0xb0) { rtn=8; }
		else if (k>0x90) { rtn=7; }
		else if (k>0x70) { rtn=6; }
		else if (k>0x50) { rtn=5; }
		else if (k>0x40) { rtn=4; }
		else if (k>0x30) { rtn=3; }
		else if (k>0x20) { rtn=2; }
		else if (k>0x10) { rtn=1; }
		else { rtn=0; }
	}

	return rtn;
}
/** @var hx_call_cnt
 *  @brief 특정 카메라 모델에 대해 모션처리 빈도를 조절하기 위한 변수.
 */
static unsigned int hx_call_cnt[AVAILABLE_MAX_CH] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
/**
 * @brief mrtp library로부터 RTP Extension정보를 받아 분석한다.
 * @param[in] ch 채널 번호.
 * @param[in] p RTP Extension의 최초 시작 위치.
 * @param[in] len RTP Extension의 길이.
 * @param[out] rtn_hx 분석한 알람 및 모션 등의 정보가 들어갈 struct.
 * @param[in] user_data user data.
 *
 * Mrtp library에서 RTP 재생 중 RTP Extension을 발견할 때마다 이 callback함수를 호출한다.
 * (30fps 기준 1초에 30회)
 *
 * @todo 현재 TI카메라의 경우 parsing을 payload위치 기준으로 하기 때문에
 * 카메라 모델 추가 등 이슈에 문제의 소지가 있음. ID기준으로 parsing하도록 수정 필요.
 * @todo ONVIF 카메라의 경우 기존 카메라 루틴을 동일하게 사용하기 위해
 * runtime에 motion_flag, alarm_flag 임의 변수를 추가하여 사용하고 있으나 시나리오에 일관성이 없어짐.
 * ONVIF metadata, event service 용도의 API 추가 필요.
 */
static void nf_ipcam_header_x_callback(gint ch, guchar *p, gint len, NMFMrtpPipeHeaderX *rtn_hx, gpointer user_data)
{
	guint mcode = 0;
	guchar *payload = p;
	mtable *runtime = NULL;
	int i,j, rtp_x_len;
	unsigned char x_id, x_len;
	unsigned short x_flag;

	runtime = get_runtime();
	g_return_if_fail(runtime != NULL);
#if 0
	if (p == NULL) goto no_x_data;
	if (len == NULL) goto no_x_data;
#endif
	mcode = runtime[ch].sys.model_code;
	/* FIXME NPT camera */
	if(runtime[ch].motion.method == MAM_RECTANGLE && mcode == NF_IPCAM_MODEL_TI_368)
	{
		mcode = NF_IPCAM_MODEL_AMB_A2;
	}

	memset(rtn_hx, 0x00, sizeof(NMFMrtpPipeHeaderX));
	switch(mcode)
	{
		case NF_IPCAM_MODEL_AMB_A2:
		case NF_IPCAM_MODEL_AMB_D1:
			hx_call_cnt[ch] = (hx_call_cnt[ch] + 1) % 6;
			if (hx_call_cnt[ch] != 1)
			{
				break;
			}
			if (p == NULL)
			{
				break;
			}

			if(runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
			{
				payload = p;
				rtn_hx->aflag = *(payload+7);
				//if(*(payload + 40) == 0x04)	// 04 15 00 00 0f 00 00 00 - old packet
				if(*(payload + 40) == 0x15 || *(payload + 41) == 0x15)	// 04 15 or 15 04 // 0x15 Event
				{
					//NPT RTSP m_flag 44 or 47 is area info (45 & 46 = 00)
					rtn_hx->mflag = *(payload+44) + *(payload+45) + *(payload+46) + *(payload+47);

				}
				else
				{
					rtn_hx->mflag = *(payload+47);
				}
			}
			else
			{
				payload = p + 0x10;
				rtn_hx->aflag = *(payload+7) | *(payload+6) << 8;
				rtn_hx->mflag = *(payload+3) | *(payload+2) << 8;
			}

			if (rtn_hx->mflag & 0xf)
			{
				gint i;
				gint x0,y0,x1,y1;
				gchar key[64];

				//rtn_hx->mrd_len = 96;
				//rtn_hx->mrd_width = 12;
				//rtn_hx->mrd_height = 8;
				rtn_hx->mrd_len = runtime[ch].motion.num_block;
				rtn_hx->mrd_width = runtime[ch].motion.block_width;
				rtn_hx->mrd_height = runtime[ch].motion.block_height;

				rtn_hx->mrd_min = 0;
				rtn_hx->mrd_max = 0xff;
				memset(&rtn_hx->mraw[0], 0x00, rtn_hx->mrd_len);
				/* generate motion rawdata for A2 */
				for(i = 0; i < runtime[ch].motion.max_rect; i++)
				{
					if (rtn_hx->mflag & (1<<i))
					{
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", ch, i);
						y0 = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", ch, i);
						x0 = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", ch, i);
						y1 = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", ch, i);
						x1 = nf_sysdb_get_uint(key);

						{
							gint ii, jj;

							for (ii = 0; ii < rtn_hx->mrd_height; ii++)
							{
								for (jj = 0; jj < rtn_hx->mrd_width; jj++)
								{
									if ((y0 <= ii && ii <= y1) && (x0 <= jj && jj<= x1))
									{
										//printf("O ");
										rtn_hx->mraw[ii*rtn_hx->mrd_width + jj] = 0xff;
									}
									else
									{
										//printf("X ");
									}
								}
								//printf("\n");
							}
						}
					}
				}
				if (0)
				{
					int ii, jj;
					printf("---------------------- %x ---------------------\n", rtn_hx->mflag);
					for (ii = 0; ii < 8; ii++)
					{
						for (jj = 0; jj < 12; jj++)
						{
							if (rtn_hx->mraw[ii*12+jj]==0)
							{
								printf("X ");
							}
							else
							{
								printf("O ");
							}
						}
						printf("\n");
					}
				}
#if 0
				printf("\n\n\n");
				for (i=0; i < 8; i++)
				{
					printf("%03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d\n",
						rtn_hx->mraw[i*12], rtn_hx->mraw[i*12 + 1], 
						rtn_hx->mraw[i*12 + 2], rtn_hx->mraw[i*12 + 3], 
						rtn_hx->mraw[i*12 + 4], rtn_hx->mraw[i*12 + 5], 
						rtn_hx->mraw[i*12 + 6], rtn_hx->mraw[i*12 + 7], 
						rtn_hx->mraw[i*12 + 8], rtn_hx->mraw[i*12 + 9], 
						rtn_hx->mraw[i*12 + 10], rtn_hx->mraw[i*12 + 11]);
				}
				printf("\n\n\n");
#endif
			}
			else
			{
				//rtn_hx->mrd_len = 96;
				//rtn_hx->mrd_width = 12;
				//rtn_hx->mrd_height = 8;
				rtn_hx->mrd_len = runtime[ch].motion.num_block;
				rtn_hx->mrd_width = runtime[ch].motion.block_width;
				rtn_hx->mrd_height = runtime[ch].motion.block_height;
				rtn_hx->mrd_min = 0;
				rtn_hx->mrd_max = 0xff;
				memset(&rtn_hx->mraw[0], 0x00, 96);
			}
			break;
		case NF_IPCAM_MODEL_TI_368:
			if (p == NULL)
			{
				break;
			}
			if ((*(p+0x4) != 0x12) || (*(p+0x8) != 0x13) || (*(p+0xc) != 0x11))
			{
				IPCAM_DBG(ERROR, "header extension error detected\n");
				break;
			}

			payload = p + 0x04;
			rtn_hx->aflag = *(payload+3);
			payload = p + 0x0c;
			rtn_hx->mflag = 0;
			rtn_hx->mrd_len = *(payload+3) | *(payload+2) << 8;
			rtn_hx->mrd_width = *(payload+4);
			rtn_hx->mrd_height = *(payload+5);
			rtn_hx->mrd_min = *(payload+6);
			rtn_hx->mrd_max = *(payload+7);
			for (i=0; i<rtn_hx->mrd_height; i++)
			{
				for (j=0; j<rtn_hx->mrd_width; j++)
				{
					rtn_hx->mraw[(i*rtn_hx->mrd_width)+j] = _ti_motion_map(*((unsigned char*)(payload+8+(i*rtn_hx->mrd_width)+j)), rtn_hx->mrd_min, rtn_hx->mrd_max);
				}
			}
			//memcpy(&rtn_hx->mraw[0], payload+8, (rtn_hx->mrd_width)*(rtn_hx->mrd_height));
#if 1
			//ai event & ai counter
			{
				gboolean act_chk=FALSE;
				char str_tmp[32]={0,};

				int i;
				guint event_len=0;
				guint event_cnt=0;
				
				size_t size;
				void *r;
				char* event_start;

				NF_AI_META_EXT* tmp;
				ai_rule_event_t ai_evt[AI_META_MAX] = {0,};

				char* ai_event_start = (payload+8+8+((rtn_hx->mrd_width)*(rtn_hx->mrd_height)));
				char aievt_id = *ai_event_start;
				guint aievt_len = 0;
				aievt_len = ((*(ai_event_start + 2)<<8) & 0xff00) | ((*(ai_event_start + 3)) & 0xff);

				sprintf(str_tmp, "cam.dvabx.cfg.R%d.devcam", ch);
				act_chk = nf_sysdb_get_bool(str_tmp);
				if(act_chk == FALSE)
					break;
				
				if(aievt_id == 0x16){
					event_len = aievt_len-2;
					event_cnt = event_len/(sizeof(NF_AI_META_EXT));
					//IPCAM_DBG(MINOR, "aievt_id(0x%02x) aievt_len(%d) event_len(%d)\n", aievt_id, aievt_len, event_len);

					if (event_len % sizeof(NF_AI_META_EXT) != 0) {
						//printf("never be here : Length(%d) of AI-Event-Data is error\n", event_len);
						event_len = 0;
					}

					if(event_len >0){
						event_start = ai_event_start + 4;

						size = 2 * sizeof(int) + (sizeof(ai_rule_event_t)*event_cnt);

						tmp = event_start;

						for(i=0;i<event_cnt;i++){

							if (tmp->rule_id < 0 || tmp->rule_id > 15) {
								//printf("never be here: rule_id(%d) is error\n", tmp->rule_id);
								break;
							}

							ai_evt[i].type = tmp->type;
							ai_evt[i].object_id = tmp->object_id;
							ai_evt[i].rule_id = tmp->rule_id;
							strcpy(ai_evt[i].object_class, tmp->object_class);
							ai_evt[i].bbx_position[0] = tmp->bbx_position[0];
							ai_evt[i].bbx_position[1] = tmp->bbx_position[1];
							ai_evt[i].bbx_position[2] = tmp->bbx_position[2];
							ai_evt[i].bbx_position[3] = tmp->bbx_position[3];
							ai_evt[i].timestamp = tmp->timestamp;
							ai_evt[i].timestampl = tmp->timestampl;
							ai_evt[i].confidence = tmp->confidence;
							ai_evt[i].ch = ch;

							#if 0
							printf("\033[0;32m %d) type %d object_id %s rule_id %d pos[%2f %2f %2f %2f]\033[0;39m\n", \
							i, ai_evt[i].type, ai_evt[i].object_class, ai_evt[i].rule_id, ai_evt[i].bbx_position[0], \
							ai_evt[i].bbx_position[1],ai_evt[i].bbx_position[2],ai_evt[i].bbx_position[3]);
							#endif

							tmp = tmp + 1;
						}


						r = malloc(size);
						((int *)r)[0] = ch;
						((int *)r)[1] = event_cnt;
				
						memcpy((int *)r + 2, (void *)ai_evt, (sizeof(ai_rule_event_t)*event_cnt));
						nf_notify_fire_pointer("ai_event", r, (int)size);
						free(r);
					}
				}

				ai_meta_cnt_t* cnt_tmp;
				ai_meta_cnt_t ai_cnt[AI_COUNTER_MAX] = {0,};

				char* ai_counter_start = NULL;
				char aicnt_id = 0;
				guint aicnt_len = 0;

				if(event_len>0) //ai counter offset (ai event size next)
					ai_counter_start = (payload+8+8+((rtn_hx->mrd_width)*(rtn_hx->mrd_height))+4+event_len);
				else
					ai_counter_start = (payload+8+8+((rtn_hx->mrd_width)*(rtn_hx->mrd_height)));
				
				aicnt_len = ((*(ai_counter_start + 2)<<8) & 0xff00) | ((*(ai_counter_start + 3)) & 0xff);
				aicnt_id = *ai_counter_start;
				
				if(aicnt_id == 0x16){
					event_cnt = aicnt_len/(sizeof(ai_meta_cnt_t));

					if (aicnt_len % sizeof(ai_meta_cnt_t) != 0) {
						//printf("never be here : Length(%d) of AI-Counter-Data is error\n", event_len);
						break;
					}

					if(aicnt_len >0){
						event_start = ai_counter_start + 4;

						size = (2 * sizeof(int)) + aicnt_len;

						cnt_tmp = event_start;

						for(i=0;i<event_cnt;i++){
							ai_cnt[i].id = cnt_tmp->id;
							ai_cnt[i].reserved = cnt_tmp->reserved;
							ai_cnt[i].value = cnt_tmp->value;
							#if 0
							printf("\033[0;32m %d) id %d value %d \033[0;39m\n", i, ai_cnt[i].id, ai_cnt[i].value);
							#endif
							cnt_tmp = cnt_tmp + 1;
						}

						r = malloc(size);
						((int *)r)[0] = ch;
						((int *)r)[1] = event_cnt;
				
						memcpy((int *)r + 2, (void *)ai_cnt, aicnt_len);
						nf_notify_fire_pointer("ai_counter", r, (int)size);
						free(r);
					}
				}
			}
#endif
			break;


		default:
			// Motion Set
			if (runtime[ch].motion.method == MAM_GENERAL)
			{
				rtn_hx->mflag = 0;
				rtn_hx->mrd_len = runtime[ch].motion.num_block;
				rtn_hx->mrd_width = runtime[ch].motion.block_width;
				rtn_hx->mrd_height = runtime[ch].motion.block_height;
				rtn_hx->mrd_min = 0;
				rtn_hx->mrd_max = 0xff;
				
				if(runtime[ch].motion.stream_snap != NULL)
				{
					memcpy(&rtn_hx->mraw[0], runtime[ch].motion.stream_snap, runtime[ch].motion.num_block);
				}

			}
			else
			{
				if (runtime[ch].motion_flag > 120)
				{
					rtn_hx->mrd_len = 0;
					rtn_hx->mflag = 0;
					runtime[ch].motion_flag = 0;
				}
				else if (runtime[ch].motion_flag > 0)
				{
					rtn_hx->mrd_len = 0;
					rtn_hx->mflag = 1;

					if (runtime[ch].sys.model_code == NF_NVS_MODEL_ITX)
					{
						runtime[ch].motion_flag--;
					}
				}
				else
				{
					rtn_hx->mrd_len = 0;
					rtn_hx->mflag = 0;
					runtime[ch].motion_flag = 0;
				}
			}

			// Alarm Set
			if (runtime[ch].alarm_flag)
			{
				rtn_hx->aflag = 1;
				//runtime[ch].alarm_flag = 0;
			}
			else
			{
				rtn_hx->aflag = 0;
			}
			break;
	}

	return;

no_x_data:
	rtn_hx->aflag = 0;
	rtn_hx->mflag = 0;
	rtn_hx->mrd_len = 4;
	rtn_hx->mrd_width = 0;
	rtn_hx->mrd_height = 0;
	rtn_hx->mrd_min = 0;
	rtn_hx->mrd_max = 0;
	memset(&rtn_hx->mraw[0], 0x00, sizeof(rtn_hx->mraw));
}

/**
 * @brief mrtp library로부터 metadata stream을 받아 분석한다.
 * @param[in] ch 채널 번호.
 * @param[in] payload Metadata stream의 최초 시작 위치.
 * @param[in] len Payload의 길이.
 * @param[in] user_data user data.
 *
 * Mrtp library에서 metadata stream xml을 수신할 때마다 이 callback 함수를 호출한다.
 */
static void nf_ipcam_onvif_metadata_callback(gint ch, guchar* payload, gint len, gpointer user_data)
{
	char value_in_msg[128];
	char *temp_str = NULL;
	char key[64];
	int alarm_op_mode;
	mtable *runtime = NULL;
	runtime = get_runtime();
	g_return_if_fail(runtime != NULL);

	if(len <= 0){
		// Payload Size 0
		return;
	}

	temp_str = (char*)malloc(len+1);
	
	// malloc fail
	if(temp_str == NULL) return;
	
	memset(temp_str, 0x00, (len+1));
	memcpy(temp_str, payload, len);

	memset(&key, 0x00, sizeof(key));
	snprintf(key, 64,"alarm.sensor.S%d.op_type",ch);
	alarm_op_mode = nf_sysdb_get_bool(key);
//	if(nf_sysman_get_fwver_vendor() == 83 || nf_sysman_get_fwver_vendor() == 183 || nf_ipcam_is_vendor("VICON"))
	{
		if(nf_ipcam_is_onvif_support(ch) == 1)
		{
			int event_state;

			//IPCAM_DBG(MINOR, "ch %d len %d:\n%s\n\n\n", ch, len, temp_str);
			
			event_state =  nf_onvif_metadata_parser(temp_str);
			//printf("\e[31m ONVIF METADATA : [%d]\n\e[0m",event_state);
			
			// Motion Event
			if(event_state & 1)
			{
				runtime[ch].motion_flag = 0;

			}
			else if(event_state & 2)
			{
				runtime[ch].motion_flag = 90;
			}

			// Alarm Event
			if(event_state & 4)
			{
				runtime[ch].alarm_flag = alarm_op_mode ? 1 : 0;	
			}
			else if(event_state & 8)
			{
				runtime[ch].alarm_flag = alarm_op_mode ? 0 : 1;
			}
		}
		else
		{
			nf_ipcam_parse_vabox_metadata_stream(ch,temp_str);
		}
	}
	free(temp_str);
}

/**
 * @brief 한 채널에 대한 sysdb 변경 notify를 받았을 시("sysdb_tmp_change") 처리한다.
 * @param pinfo Notify 정보 struct, param[0] = 카테고리, param[1] = 채널번호.
 * @param data user data형식이며, 현재 항상 NULL임.
 *
 * @see DAL_notify_fire_DB_sync
 */
static void _api_ipcam_tmp_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	mtable *runtime = NULL;
	dtable *discovery = NULL;
	GAsyncQueue *queue = NULL;

	gchar *area;

	gint i;
	gint result;
	guint sense_d;
	gint port;

	NFIPCamSetupMotionArea *info;
	gchar key[64];

	queue = get_queue();
	runtime = get_runtime();
	g_return_if_fail(runtime != NULL);
	g_return_if_fail(pinfo != NULL);

	//IPCAM_DBG(MINOR, "start %d\n", pinfo->d.params[0]);

	switch(pinfo->d.params[0])
	{
	case NF_SYSDB_TMP_CHANGE_EVENTID_PTZ:
	{
		char key[64];
		unsigned int onoff;
		port = pinfo->d.params[1];

		// Set auto focus
		if(runtime[port].ptz.supported & PTZ_SETUP_FOCUS)
		{
			memset(&key, 0x00, sizeof(key));
			snprintf(key, 64, "cam.ptz.P%d.auto_focus", port);
			onoff = nf_sysdb_get_bool(key);
			nf_ipcam_set_auto_focus(port, onoff);
		}

		// Set auto iris
		if(runtime[port].ptz.supported & PTZ_SETUP_IRIS)
		{
			memset(&key, 0x00, sizeof(key));
			snprintf(key, 64, "cam.ptz.P%d.auto_iris", port);
			onoff = nf_sysdb_get_bool(key);
			nf_ipcam_set_auto_iris(port, onoff);
		}
	}
	break;
	case NF_SYSDB_TMP_CHANGE_EVENTID_MOTION:
	{
		port = pinfo->d.params[1];
		if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0) { return; }
		if ((runtime[port].func & NF_IPCAM_FUNC_MOTION) == 0) { return; }
		if (runtime[port].motion.method != MAM_RECTANGLE && runtime[port].motion.method != MAM_SIM_RECTANGLE) { return; }

		info = (NFIPCamSetupMotionArea*) malloc(sizeof(NFIPCamSetupMotionArea));
		memset(info, 0x00, sizeof(NFIPCamSetupMotionArea));
		snprintf(key, 64, "alarm.motion.M%d.area", port);
		area = nf_sysdb_get_str_nocopy(key);
		strncpy(info->area, area, 1400);

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
		info->area_num = nf_sysdb_get_uint(key);

		if (info->area_num == 0)
		{
			info->ch = port;
			info->block_width = runtime[port].motion.block_width;
			info->block_height = runtime[port].motion.block_height;
			info->method = runtime[port].motion.method;
			info->area_num = 0;
			info->marea[0].sensitivity = sense_d;
			_get_rect_points(
					info->block_width,
					info->block_height,
					area,
					&info->marea[0].FIGURE.RECTANGLE.left_top,
					&info->marea[0].FIGURE.RECTANGLE.right_bottom
			);
		}
		else 
		{
			info->ch = port;
			info->block_width = runtime[port].motion.block_width;
			info->block_height = runtime[port].motion.block_height;
			info->method = runtime[port].motion.method;
			for (i = 0; i < info->area_num; i++)
			{
				info->marea[i].sensitivity = sense_d;
				snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", port, i);
				info->marea[i].FIGURE.RECTANGLE.left_top.y = nf_sysdb_get_uint(key);
				snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", port, i);
				info->marea[i].FIGURE.RECTANGLE.left_top.x = nf_sysdb_get_uint(key);
				snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", port, i);
				info->marea[i].FIGURE.RECTANGLE.right_bottom.y = nf_sysdb_get_uint(key);
				snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", port, i);
				info->marea[i].FIGURE.RECTANGLE.right_bottom.x = nf_sysdb_get_uint(key);
			}
		}
		GAsyncQueue *dsq = get_recevt_queue();
		IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_MOTION);
		evt->port = port;
		evt->p.len = sizeof(info);
		evt->p.ptr = info;
		g_async_queue_push(dsq, evt);
#if 0
		result = nf_ipcam_set_motion_area(port, &info, NULL, NULL, NULL);

		if (queue != NULL)
		{
			if (result == IPCAM_SETUP_RTN_FAILED)
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
				g_warning("[%s] Motion area setup failed(CH:%d)\n", __FUNCTION__, port);
			}
		}
#endif
	}
	break;
	case NF_SYSDB_TMP_CHANGE_EVENTID_ONVIF:
	{
		gchar u_buf[32];
		gchar p_buf[32];
		gchar *u,*p;

		port = pinfo->d.params[1];
		if (nf_get_running_mode())
		{
			nf_openmode_retry_login(port);
		}
		else
		{
			runtime[port].state &= ~MGMT_STATE_IDPASS_WAITING;
			switch(runtime[port].sys.model_code)
			{
				case NF_IPCAM_MODEL_AMB_D1:
				case NF_IPCAM_MODEL_AMB_A2:
				case NF_IPCAM_MODEL_TI_368:
				if (queue != NULL)
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
				}
				break;
				default:
				break;
			}
		}
		nf_pnd_prog_notify_fire(port, 7, __LINE__, __FILE__);
		return;

#if 0
		discovery[port].state = IPCAM_DISC_STATE_NONE;
		runtime[port].state = MGMT_STATE_UNLINKED;
		break;
		if (runtime[port].sys.model_code <= NF_IPCAM_MODEL_AMB_D1)
		{
			nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
			break;
		}
#endif

#if 0
		snprintf(u_buf, 32, "cam.C%d.id", port);
		snprintf(p_buf, 32, "cam.C%d.pwd", port);

		u = nf_sysdb_get_str_nocopy(u_buf);
		p = nf_sysdb_get_str_nocopy(p_buf);

		strncpy(runtime[port].username, u, 32);
		strncpy(runtime[port].password, p, 32);

		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
		runtime[port].state &= ~MGMT_STATE_IDPASS_WAITING;
		runtime[port].state |= (MGMT_STATE_HANDLING_LINK|MGMT_STATE_HANDLING);
#endif
	}
		break;

	case NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE:
	{
		int sha, bri, con, col, tin, rot, etc;
		char key[64];

		port = pinfo->d.params[1];

		//IPCAM_DBG(MINOR, "CH(%d) NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE event occured\n", port);

		if(nf_ipcam_is_onvif_support(port) != 1)
		{
			NFIPCamSetupImage info;

			memset(&info, 0x00, sizeof(NFIPCamSetupImage));

			snprintf(key, 64, "cam.C%d.sharpness", port);
			sha = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.bright", port);
			bri = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.contrast", port);
			con = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.tint", port);
			tin = nf_sysdb_get_uint(key);
			snprintf(key, 64, "cam.C%d.color", port);
			col = nf_sysdb_get_uint(key);

			info.sharpness = sha;
			info.brightness = bri;
			info.contrast = con;
			info.color = col;
			info.tint = tin;

			runtime[port].image.sharpness.value = sha;
			runtime[port].image.brightness.value = bri;
			runtime[port].image.contrast.value = con;
			runtime[port].image.tint.value = tin;
			runtime[port].image.color.value = col;

			snprintf(key, 64, "cam.C%d.exposure_mode", port);
			etc = nf_sysdb_get_uint(key);
			info.exposure = etc;
			if (etc & runtime[port].image.exposure.support)
			{
				runtime[port].image.exposure.value = etc;
			}

			snprintf(key, 64, "cam.C%d.gain", port);
			etc = nf_sysdb_get_int(key);
			info.agc = etc;
			runtime[port].image.agc.value = etc;

			snprintf(key, 64, "cam.C%d.dnn_ntod", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_sense_ntod = etc;
			runtime[port].image.dnn_sense_ntod.value = etc;

			snprintf(key, 64, "cam.C%d.dnn_dton", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_sense_dton = etc;
			runtime[port].image.dnn_sense_dton.value = etc;

			snprintf(key, 64, "cam.C%d.etime", port);
			etc = nf_sysdb_get_int(key);
			info.eshutter_speed = etc;
			runtime[port].image.eshutter_speed.value = etc;

			snprintf(key, 64, "cam.C%d.slow_shutter", port);
			etc = nf_sysdb_get_uint(key);
			info.slow_shutter = etc;
			if (etc & runtime[port].image.slow_shutter.support)
			{
				runtime[port].image.slow_shutter.value = etc;
			}

			snprintf(key, 64, "cam.C%d.max_agc", port);
			etc = nf_sysdb_get_uint(key);
			info.max_agc = etc;
			if (etc & runtime[port].image.max_agc.support)
			{
				runtime[port].image.max_agc.value = etc;
			}

			snprintf(key, 64, "cam.C%d.iris_control", port);
			etc = nf_sysdb_get_uint(key);
			info.iris = etc;
			if (etc & runtime[port].image.iris.support)
			{
				runtime[port].image.iris.value = etc;
			}

			snprintf(key, 64, "cam.C%d.blc_control", port);
			etc = nf_sysdb_get_uint(key);
			info.blc = etc;
			if (etc & runtime[port].image.blc.support)
			{
				runtime[port].image.blc.value = etc;
			}

			snprintf(key, 64, "cam.C%d.day_night_mode", port);
			etc = nf_sysdb_get_uint(key);
			info.day_night = etc;
			if (etc & runtime[port].image.day_night.support)
			{
				runtime[port].image.day_night.value = etc;
			}

			snprintf(key, 64, "cam.C%d.day_night_duration", port);
			etc = nf_sysdb_get_uint(key);
			info.det_time = etc;
			if (etc & runtime[port].image.tg_time.support)
			{
				runtime[port].image.tg_time.value = etc;
			}

			snprintf(key, 64, "cam.C%d.dnn_start_hour", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_start_hour = etc;
			runtime[port].image.dnn_schedule.start.hour = etc;

			snprintf(key, 64, "cam.C%d.dnn_start_min", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_start_min = etc;
			runtime[port].image.dnn_schedule.start.min = etc;

			snprintf(key, 64, "cam.C%d.dnn_end_hour", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_end_hour = etc;
			runtime[port].image.dnn_schedule.end.hour = etc;

			snprintf(key, 64, "cam.C%d.dnn_end_min", port);
			etc = nf_sysdb_get_uint(key);
			info.dnn_end_min = etc;
			runtime[port].image.dnn_schedule.end.min = etc;

			snprintf(key, 64, "cam.C%d.defog", port);
			etc = nf_sysdb_get_uint(key);
			info.defog= etc;
			if (etc & runtime[port].image.defog.support)
			{
				runtime[port].image.defog.value = etc;
			}

			snprintf(key, 64, "cam.C%d.hlc", port);
			etc = nf_sysdb_get_uint(key);
			info.hlc= etc;
			if (etc & runtime[port].image.hlc.support)
			{
				runtime[port].image.hlc.value = etc;
			}

			snprintf(key, 64, "cam.C%d.wb_mode", port);
			etc = nf_sysdb_get_uint(key);
			info.white_balance = etc;
			if (etc & runtime[port].image.wb.support)
			{
				runtime[port].image.wb.value = etc;
			}

			snprintf(key, 64, "cam.C%d.mwb_mode", port);
			etc = nf_sysdb_get_uint(key);
			info.mwb = etc;
			if (etc & runtime[port].image.mwb.support)
			{
				runtime[port].image.mwb.value = etc;
			}

			snprintf(key, 64, "cam.C%d.wdr_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.wd.support)
			{
				info.wdr= etc;
				runtime[port].image.wd.value = etc;
			}
			else
			{
				info.wdr = runtime[port].image.wd.value;
			}

			snprintf(key, 64, "cam.C%d.adaptive_ir", port);
			etc = nf_sysdb_get_uint(key);
			if(etc & runtime[port].image.adaptive_ir.support)
			{
				info.adaptive_ir = etc;
				runtime[port].image.adaptive_ir.value =etc;
			}
			else
			{
				info.adaptive_ir = runtime[port].image.adaptive_ir.value;
			}

			snprintf(key, 64, "cam.C%d.focus_mode", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.focus_mode.support)
			{
				info.focus_mode = etc;
				runtime[port].image.focus_mode.value = etc;
			}
			else
			{
				info.focus_mode = runtime[port].image.focus_mode.value;
			}

			snprintf(key, 64, "cam.C%d.antiflicker", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.anti_flicker.support)
			{
				info.anti_flicker = etc;
				runtime[port].image.anti_flicker.value = etc;
			}
			else
			{
				info.anti_flicker = runtime[port].image.anti_flicker.value;
			}

			snprintf(key, 64, "cam.C%d.max_shutter", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.max_shutter.support)
			{
				info.max_shutter = etc;
				runtime[port].image.max_shutter.value = etc;
			}
			else
			{
				info.max_shutter = runtime[port].image.max_shutter.value;
			}

			snprintf(key, 64, "cam.C%d.base_shutter", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.base_shutter.support)
			{
				info.base_shutter = etc;
				runtime[port].image.base_shutter.value = etc;
			}
			else
			{
				info.base_shutter = runtime[port].image.base_shutter.value;
			}

			snprintf(key, 64, "cam.C%d.rotate", port);
			rot = nf_sysdb_get_uint(key);

			if (runtime[port].sys.model_code != NF_IPCAM_MODEL_TI_368)
			{
				if((rot != runtime[port].video.mirror.value)
				&& (rot & runtime[port].video.mirror.support))
				{

					//runtime[port].video.mirror.value = rot;
					//nf_ipcam_set_vcodec_sysdb(port, NULL, NULL, NULL);
				}
			}
			else
			{
				if
				((rot != runtime[port].video.mirror.value)
				&& (rot & runtime[port].video.mirror.support))
				{
					runtime[port].video.mirror.value = rot;
					nf_ipcam_set_rotation(port, rot, NULL, NULL, NULL);
				}
			}

			// DNR CONTROL MODE
			snprintf(key, 64, "cam.C%d.dnr_control", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.dnr_ctr.support)
			{
				info.dnr_ctr= etc;
				runtime[port].image.dnr_ctr.value = etc;
			}
			else
			{
				info.dnr_ctr= runtime[port].image.dnr_ctr.value;
			}

			// ITX NPT Focus Near Limit
			snprintf(key, 64, "cam.C%d.focus_limit", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.focus_limit.support)
			{
				info.focus_limit = etc;
				runtime[port].image.focus_limit.value = etc;
			}
			else
			{
				info.focus_limit = runtime[port].image.focus_limit.value;
			}

			// Stabilizer
			snprintf(key, 64, "cam.C%d.stabilizer", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.stabilizer.support)
			{
				info.stabilizer = etc;
				runtime[port].image.stabilizer.value = etc;
			}
			else
			{
				info.stabilizer = runtime[port].image.stabilizer.value;
			}

			//IR Correction 
			snprintf(key, 64, "cam.C%d.ir_correction", port);
			etc = nf_sysdb_get_uint(key);
			if (etc & runtime[port].image.ir_correction.support)
			{
				info.ir_correction = etc;
				runtime[port].image.ir_correction.value = etc;
			}
			else
			{
				info.ir_correction = runtime[port].image.ir_correction.value;
			}

			//ColorVu level
			if(runtime[port].image.supported & NF_IPCAM_IMAGE_COLORVU){
				snprintf(key, 64, "cam.C%d.illumination_level", port);
				etc = nf_sysdb_get_uint(key);
				info.colorvu_level = etc;
				runtime[port].image.colorvu_level.value = etc;

				snprintf(key, 64, "cam.C%d.illumination_level_ctl", port);
				etc = nf_sysdb_get_uint(key);
				info.colorvu_ctrl = etc;
				runtime[port].image.colorvu_ctrl.value = etc;
			}


			nf_ipcam_set_image(port, &info, NULL, NULL, NULL);

			int major, type, subtype, minor;
			sscanf(runtime[port].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);
	


			if((strncmp(runtime[port].sys.stdver,"NPT", 3) != 0 ) ||
					(strncmp(runtime[port].sys.stdver,"NPT", 3) == 0 && type > 3))
			{
				if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_NTOD)
				{
					nf_ipcam_set_dnn_adjust_n2d(port, &info, NULL, NULL, NULL);
				}

				if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_DTON)
				{
					nf_ipcam_set_dnn_adjust_d2n(port, &info, NULL, NULL, NULL);
				}
			}

			if((runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS)
			&& (runtime[port].image.iris.value == NF_IPCAM_IMAGE_PIRIS_MANUAL || runtime[port].image.exposure.value == NF_IPCAM_IMAGE_EXP_MODE_MANUAL))
			{
				snprintf(key, 64, "cam.C%d.iris", port);
				etc = nf_sysdb_get_int(key);
				//IPCAM_DBG(MINOR, "manualiris(%d) ", etc);

				nf_ipcam_set_iris(port, etc, NULL, NULL, NULL);
			}

			snprintf(key, 64, "cam.C%d.focus_mode", port);
			etc = nf_sysdb_get_uint(key);

			if((runtime[port].image.supported & NF_IPCAM_IMAGE_FOCUS)
			&& etc == NF_IPCAM_FOCUS_MODE_ITX_ABSOLUTE)
			{
				snprintf(key, 64, "cam.C%d.focus_abposition", port);
				etc = nf_sysdb_get_int(key);
				//IPCAM_DBG(MINOR, "manualiris(%d) ", etc);

				nf_ipcam_set_focus(port, etc, NULL, NULL, NULL);
			}

			NFIPCamSetupFocusComp focus_comp_info;
			memset(&focus_comp_info, 0x00, sizeof(focus_comp_info));
			// Focus Compensation
			if(runtime[port].focus.supported & NF_IPCAM_FOCUS_DNN_COMP)
			{
				// Focus Day and Night Compensation
				snprintf(key, 64, "cam.C%d.focus_dnn_comp", port);
				etc = nf_sysdb_get_bool(key);

				if(etc == 0 || etc == 1)
					focus_comp_info.dnn_comp_mode = etc;
				else
					focus_comp_info.dnn_comp_mode = 1;
			}

			if(runtime[port].focus.supported & NF_IPCAM_FOCUS_TEM_COMP)
			{
				// Focus Temperature Compensation
				snprintf(key, 64, "cam.C%d.focus_tem_comp", port);
				etc = nf_sysdb_get_bool(key);

				if(etc == 0 || etc == 1)
					focus_comp_info.tem_comp_mode = etc;
				else
					focus_comp_info.tem_comp_mode = 1;
			}

			nf_ipcam_set_focus_compensation(port, &focus_comp_info, NULL, NULL, NULL);

		}
		else if(__OFM(NF_ONVIF_SERVICE_IMAGE) & runtime[port].onvif.onvif_service)
		{
			NFIPCamSetupImage_onvif info;
			memset(&info, 0x00, sizeof(NFIPCamSetupImage_onvif));

			int value;

			snprintf(key, 64, "cam.C%d.bright", port);
			value = nf_sysdb_get_uint(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
			{
				info.brightness = value; 
			}
			else
			{
				info.brightness = runtime[port].image_onvif.brightness.value;
			}

			snprintf(key, 64, "cam.C%d.color", port);
			value = nf_sysdb_get_uint(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
			{
				info.color = value;
			}
			else
			{
				info.color = runtime[port].image_onvif.color.value;
			}

			snprintf(key, 64, "cam.C%d.contrast", port);
			value = nf_sysdb_get_uint(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
			{
				info.contrast = value;
			}
			else
			{
				info.contrast = runtime[port].image_onvif.contrast.value;
			}

			snprintf(key, 64, "cam.C%d.sharpness", port);
			value = nf_sysdb_get_uint(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
			{
				info.sharpness = value;
			}
			else
			{
				info.sharpness = runtime[port].image_onvif.sharpness.value;
			}

			snprintf(key, 64, "cam.C%d.focus_mode", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE)
			&& (runtime[port].image_onvif.focus.mode.support & value))
			{
				info.focus_mode = value;
			}
			else
			{
				info.focus_mode = runtime[port].image_onvif.focus.mode.value;
			}

			snprintf(key, 64, "cam.C%d.focus_default_speed", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
			{
				info.default_speed = value;
			}
			else
			{
				info.default_speed = runtime[port].image_onvif.focus.defaultspeed.value;
			}

			snprintf(key, 64, "cam.C%d.focus_near_limit", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
			{
				info.near_limit = value;
			}
			else
			{
				info.near_limit = runtime[port].image_onvif.focus.nearlimit.value;
			}

			snprintf(key, 64, "cam.C%d.focus_far_limit", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
			{
				info.far_limit = value;
			}
			else
			{
				info.far_limit = runtime[port].image_onvif.focus.farlimit.value;
			}

			snprintf(key, 64, "cam.C%d.wb_mode", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_MODE)
			&& (runtime[port].image_onvif.wb.mode.support & value))
			{
				info.white_balance = value;
			}
			else
			{
				info.white_balance = runtime[port].image_onvif.wb.mode.value;
			}

			snprintf(key, 64, "cam.C%d.wb_crgain", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
			{
				info.crgain = value;
			}
			else
			{
				info.crgain = runtime[port].image_onvif.wb.crgain.value;
			}

			snprintf(key, 64, "cam.C%d.wb_cbgain", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
			{
				info.cbgain = value;
			}
			else
			{
				info.cbgain = runtime[port].image_onvif.wb.cbgain.value;
			}



			NFIPCamSetupExposure_onvif info2;
			memset(&info2, 0x00, sizeof(NFIPCamSetupExposure_onvif));

			snprintf(key, 64, "cam.C%d.exposure_mode", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE)
			&& (runtime[port].image_onvif.exposure.mode.support & value))
			{
				info2.mode = value;
			}
			else
			{
				info2.mode = runtime[port].image_onvif.exposure.mode.value;
			}

			snprintf(key, 64, "cam.C%d.exposure_priority", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
			&& (runtime[port].image_onvif.exposure.priority.support & value))
			{
				info2.priority = value;
			}
			else
			{
				info2.priority = runtime[port].image_onvif.exposure.priority.value;
			}

			snprintf(key, 64, "cam.C%d.min_etime", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
			{
				info2.minetime = value;
			}
			else
			{
				info2.minetime = runtime[port].image_onvif.exposure.minetime.value;
			}

			snprintf(key, 64, "cam.C%d.max_etime", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
			{
				info2.maxetime = value;
			}
			else
			{
				info2.maxetime = runtime[port].image_onvif.exposure.maxetime.value;
			}

			snprintf(key, 64, "cam.C%d.etime", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
			{
				info2.etime = value;
			}
			else
			{
				info2.etime = runtime[port].image_onvif.exposure.etime.value;
			}

			snprintf(key, 64, "cam.C%d.min_gain", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
			{
				info2.mingain = value;
			}
			else
			{
				info2.mingain = runtime[port].image_onvif.exposure.mingain.value;
			}

			snprintf(key, 64, "cam.C%d.max_gain", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
			{
				info2.maxgain = value;
			}
			else
			{
				info2.maxgain = runtime[port].image_onvif.exposure.maxgain.value;
			}

			snprintf(key, 64, "cam.C%d.gain", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
			{
				info2.gain = value;
			}
			else
			{
				info2.gain = runtime[port].image_onvif.exposure.gain.value;
			}

			snprintf(key, 64, "cam.C%d.min_iris", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
			{
				info2.miniris = value;
			}
			else
			{
				info2.miniris = runtime[port].image_onvif.exposure.miniris.value;
			}

			snprintf(key, 64, "cam.C%d.max_iris", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
			{
				info2.maxiris = value;
			}
			else
			{
				info2.maxiris = runtime[port].image_onvif.exposure.maxiris.value;
			}

			snprintf(key, 64, "cam.C%d.iris", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
			{
				info2.iris = value;
			}
			else
			{
				info2.iris = runtime[port].image_onvif.exposure.iris.value;
			}

			snprintf(key, 64, "cam.C%d.iris_control", port);
			etc = nf_sysdb_get_uint(key);
			if (runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS)
			{
				info2.iris_control = etc;
			}
			else
			{
				info2.iris_control = runtime[port].image_onvif.exposure.iris_mode.value;
			}

			snprintf(key, 64, "cam.C%d.blc_control", port);
			value = nf_sysdb_get_uint(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE)
			{
				info2.blc_mode = value;
			}
			else
			{
				info2.blc_mode = runtime[port].image_onvif.blcmode.value;
			}

			snprintf(key, 64, "cam.C%d.blc_level", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
			{
				info2.blc_level = value;
			}
			else
			{
				info2.blc_level = runtime[port].image_onvif.blclevel.value;
			}

			snprintf(key, 64, "cam.C%d.wdr_mode", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE)
			&& (runtime[port].image_onvif.wdrmode.support & value))
			{
				info2.wide_dynamic_mode = value;
			}
			else
			{
				info2.wide_dynamic_mode = runtime[port].image_onvif.wdrmode.value;
			}

			snprintf(key, 64, "cam.C%d.wdr_level", port);
			value = nf_sysdb_get_int(key);
			if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
			{
				info2.wide_level = value;
			}
			else
			{
				info2.wide_level = runtime[port].image_onvif.wdrlevel.value;
			}

			snprintf(key, 64, "cam.C%d.day_night_mode", port);
			value = nf_sysdb_get_uint(key);
			if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
			&& (runtime[port].image_onvif.ircut.support & value))
			{
				info2.ircut = value;
			}
			else
			{
				info2.ircut = runtime[port].image_onvif.ircut.value;
			}

			//nf_ipcam_set_exposure_onvif(port, &info2, NULL, NULL, NULL);
			nf_ipcam_set_image_exp_onvif(port, &info, &info2, NULL, NULL, NULL);

			if(info.focus_mode != NF_IPCAM_FOCUS_MODE_ONVIF_AUTO)
			{
				NFIPCamSetupFocus_onvif info3;
				memset(&info3, 0x00, sizeof(NFIPCamSetupFocus_onvif));

				info3.mode = info.focus_mode;

				snprintf(key, 64, "cam.C%d.focus_abposition", port);
				value = nf_sysdb_get_int(key);
				if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
				&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
				{
					info3.position = value;
				}
				snprintf(key, 64, "cam.C%d.focus_abspeed", port);
				value = nf_sysdb_get_int(key);
				if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
				&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
				{
					info3.speed = value;
				}
				snprintf(key, 64, "cam.C%d.focus_redistance", port);
				value = nf_sysdb_get_int(key);
				if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				{
					info3.distance = value;
				}
				snprintf(key, 64, "cam.C%d.focus_respeed", port);
				value = nf_sysdb_get_int(key);
				if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				{
					info3.speed = value;
				}
				snprintf(key, 64, "cam.C%d.focus_cospeed", port);
				value = nf_sysdb_get_int(key);
				if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
				&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
				{
					info3.speed = value;
				}

				if(info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE || info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
				{
					nf_ipcam_set_focus_onvif(port, &info3, NULL, NULL, NULL);
				}
			}


			// TODO onvif rotate
			snprintf(key, 64, "cam.C%d.rotate", port);
			value = nf_sysdb_get_uint(key);

			if
			((value != runtime[port].video.mirror.value)
			&& (value & runtime[port].video.mirror.support) && (runtime[port].video.supported & VIDEO_SETUP_MIRROR))
			{
				nf_ipcam_set_rotation(port, value, NULL, NULL, NULL);
			}

		}
	}
		break;

	default:
		break;

	}
}

/**
 * @brief Sysdb 녹화설정의 bitrate control 옵션을 카메라 설정에서 쓰이는 bitrate control enum값으로 변경한다.
 * @param a Sysdb 녹화설정 char.
 * @return 카메라 해상도 enum값.
 *
 * @see _NF_IPCAM_BITRATE_CONTROLS_E
 */
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

/**
 * @brief Sysdb 녹화설정의 video codec 옵션을 카메라 설정에서 쓰이는 vcodec enum값으로 변경한다.
 * @param a Sysdb 녹화설정 char.
 * @return 카메라 해상도 enum값.
 *
 * @see __IPX_CODEC_TYPE_E_
 */
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

/**
 * @brief 전 채널에 대한 sysdb 변경 notify를 받았을 시("sysdb_change") 처리한다.
 * @param pinfo Notify 정보 struct, param[0] = 카테고리.
 * @param data user data형식이며, 현재 항상 NULL임.
 *
 * UI에서 apply 버튼을 누를 때 등, 전체 채널에 적용해야 할 때 이 callback함수가 호출된다.
 *
 * @todo 너무 느림.
 *
 * @see DAL_notify_fire_DB_change
 */
static void _api_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	mtable *runtime = NULL;
	int i = 0;

	//IPCAM_DBG(MINOR, "start %d\n", pinfo->d.params[0]);

	runtime = get_runtime();

	g_return_if_fail(runtime != NULL);
	g_return_if_fail(pinfo != NULL);

	if (pinfo->d.params[0] == NF_SYSDB_CATE_CAM)
	{
		unsigned int sha, bri, con, col, tin, rot, etc;
		int port;
		char key[64];
		gchar *val;
		int is_vcam;

		nf_va_object_detector_update_roi();

#if 1
		for (port =0; port < NUM_IPX_CHANNEL; port++)
		{
			int change_vcodec = 0;
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

			if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0)
			{
				continue;
			}
			if(strcmp(runtime[port].sys.vendor, "H264") == 0)
			{
				continue;
			}

			/*if (runtime[port].sys.model_code > NF_IPCAM_MODEL_AMB_D1)
			{
				continue;
			}*/
			
			snprintf(key, 64, "cam.C%d.stream.S0.size", port);
			val = nf_sysdb_get_str_nocopy(key);
			if (val != NULL)
				sz_1st = _change_szdb_to_camgr(*val);
			else
				sz_1st = _change_szdb_to_camgr(0);

			snprintf(key, 64, "cam.C%d.stream.S0.fps", port);
			fps_1st = nf_sysdb_get_uint(key);

			snprintf(key, 64, "cam.C%d.stream.S0.max_bps", port);
			maxbps_1st = nf_sysdb_get_uint(key);

			snprintf(key, 64, "cam.C%d.stream.S0.min_bps", port);
			minbps_1st = nf_sysdb_get_uint(key);
			
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

			snprintf(key, 64, "cam.C%d.stream.S1.max_bps", port);
			maxbps_2nd = nf_sysdb_get_uint(key);

			snprintf(key, 64, "cam.C%d.stream.S1.min_bps", port);
			minbps_2nd = nf_sysdb_get_uint(key);

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

			change_vcodec = 0;
			if (sz_1st != 0 && runtime[port].video.resolution.resolution[0] != sz_1st)
			{
				IPCAM_DBG(MINOR, "sz_1st(%08x) runtime(%08x) change codec\n",
						sz_1st, runtime[port].video.resolution.resolution[0]);
				change_vcodec = 1;
			}
			if (runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER] != maxbps_1st)
			{
				IPCAM_DBG(MINOR, "maxbps_1st(%d) runtime(%d) change codec\n",
						maxbps_1st, runtime[port].video.quality[0][NF_IPCAM_QUALITY_SUPER]);
				change_vcodec = 1;
			}
			if (runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW] != minbps_1st)
			{
				IPCAM_DBG(MINOR, "minbps_1st(%d) runtime(%d) change codec\n",
						minbps_1st, runtime[port].video.quality[0][NF_IPCAM_QUALITY_LOW]);

				change_vcodec = 1;
			}
			if (fps_1st != runtime[port].encoder.cur_maxfps[0])
			{
				IPCAM_DBG(MINOR, "maxfps_1st(%d) runtime(%d) change codec\n",
						fps_1st, runtime[port].encoder.cur_maxfps[0]);

				change_vcodec = 1;
			}

			if (bitctrl_1st != 0 && runtime[port].video.bitctrl[0] != bitctrl_1st)
			{
				IPCAM_DBG(MINOR, "bitctrl_1st(%08x) runtime(%08x) change codec\n",
						bitctrl_1st, runtime[port].video.bitctrl[0]);

				change_vcodec = 1;
			}

			if (vcodec_1st != 0 && runtime[port].video.vcodec[0] != vcodec_1st)
			{
				IPCAM_DBG(MINOR, "vcodec_1st(%08x) runtime(%08x) change codec\n",
						vcodec_1st, runtime[port].video.vcodec[0]);
				change_vcodec = 1;

			}

			if (runtime[port].video.stream_cnt > 1)
			{
				if (sz_2nd != 0 && runtime[port].video.resolution.resolution[1] != sz_2nd)
				{
					IPCAM_DBG(MINOR, "sz_2nd(%08x) runtime(%08x) change codec\n",
							sz_2nd, runtime[port].video.resolution.resolution[1]);

					change_vcodec = 1;
				}
				if (runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER] != maxbps_2nd)
				{
					IPCAM_DBG(MINOR, "minbps_1st(%d) runtime(%d) change codec\n",
							minbps_1st, runtime[port].video.quality[1][NF_IPCAM_QUALITY_SUPER]);
					change_vcodec = 1;

				}
				if (runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW] != minbps_2nd)
				{
					IPCAM_DBG(MINOR, "minbps_2nd(%d) runtime(%d) change codec\n",
							minbps_2nd, runtime[port].video.quality[1][NF_IPCAM_QUALITY_LOW]);
					change_vcodec = 1;

				}
				if (fps_2nd != runtime[port].encoder.cur_maxfps[1])
				{
					IPCAM_DBG(MINOR, "maxfps_2nd(%d) runtime(%d) change codec\n",
							fps_2nd, runtime[port].encoder.cur_maxfps[1]);
					change_vcodec = 1;

				}

				if (bitctrl_2nd != 0 && runtime[port].video.bitctrl[1] != bitctrl_2nd)
				{
					IPCAM_DBG(MINOR, "bitctrl_2nd(%08x) runtime(%08x) change codec\n",
							bitctrl_2nd, runtime[port].video.bitctrl[1]);
					change_vcodec = 1;

				}

				if (vcodec_2nd != 0 && runtime[port].video.vcodec[1] != vcodec_2nd)
				{
					IPCAM_DBG(MINOR, "vcodec_2nd(%08x) runtime(%08x) change codec\n",
							vcodec_2nd, runtime[port].video.vcodec[1]);
					change_vcodec = 1;

				}

			}

			snprintf(key, 63, "cam.logininfo.L%d.vcam", port);
			is_vcam = nf_sysdb_get_uint(key);
			if (change_vcodec && !is_vcam)
			{
				if (nf_get_running_mode())
				{
					nf_openmode_reconnect_ch(port);
				}
				else
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
					runtime[port].state &= ~MGMT_STATE_CONFIGURED;
				}
			}

			if(runtime[port].video.corridor_support == 1)
			{
				nf_ipcam_set_corridor_mode(port, NULL,NULL,NULL,NULL); 
			}else
			{
			}
		}
#endif

#if 0
		for (port = 0; port < NUM_IPX_CHANNEL; port++)
		{
			// SKSHIN
			if ((runtime[port].func & NF_IPCAM_FUNC_VA) != 0)
			{
				nf_ipcam_set_va_option(port, NULL, NULL, NULL);
				nf_ipcam_set_va_config(port, NULL, NULL, NULL);
				nf_ipcam_set_va_enable(port, NULL, NULL, NULL);
			}
		}
#endif

		for (port = 0; port < NUM_IPX_CHANNEL; port++)
		{
			if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0)
			{
				continue;
			}
			if (runtime[port].func & NF_IPCAM_FUNC_PRIVACY_MASK)
			{
				unsigned int _act, _acnt, _clr;
				NFIPCamPrivacyMask _pmask;

				memset(&_pmask, 0x00, sizeof(NFIPCamPrivacyMask));

				snprintf(key, 64, "cam.privacy.P%d.act", port);
				_act = nf_sysdb_get_bool(key);
				snprintf(key, 64, "cam.privacy.P%d.area.ACNT", port);
				_acnt = nf_sysdb_get_uint(key);
				if (_act == 0)
				{
					_pmask.ch = port;
					_pmask.rect_cnt = 0;
					for(i = 0; i < 10; i++) // max area number : 10
					{
						_pmask.lt[i].x = (-1);
					}

					nf_ipcam_set_privacy_mask(port, &_pmask, NULL, NULL, NULL);
					/* 2018-02-13 pmask on / off buf fix */
					memcpy(&(runtime[port].prev_pmask), &_pmask, sizeof(_pmask));
					continue;
				}

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

				/* 
				 * 20171024 add to pmask DB changed check
				 */
				if(memcmp(&_pmask, &(runtime[port].prev_pmask), sizeof(_pmask)) != 0)
				{
					_convert_pmask_area_by_corridor_view(port, &_pmask);
					nf_ipcam_set_privacy_mask(port, &_pmask, NULL, NULL, NULL);
					memcpy(&(runtime[port].prev_pmask), &_pmask, sizeof(_pmask));
				}
			}
			if (runtime[port].func & NF_IPCAM_FUNC_ROI)
			{
				int width = 0, height = 0;
				uint64_t current = 0;

				current = runtime[port].video.resolution.resolution[0];

				if(nf_ipcam_change_ipcamres_to_size(current, &width, &height) == IPCAM_SETUP_RTN_DONE)
				{
					NFIPCamSetupROIArea _roi_area;
					memset(&_roi_area, 0x00, sizeof(NFIPCamSetupROIArea));

					//set ROI area
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
					nf_ipcam_set_roi_area(port, &_roi_area, NULL, NULL, NULL);
				}
			}
			if (runtime[port].fisheye.fisheye_supported & NF_IPCAM_FISHEYE_MOUNT)
			{
				NF_IPCAM_MOUNT_TYPES_E mount;

				snprintf(key, 64, "cam.fisheye.f%d.mount_type", port);
				mount = nf_sysdb_get_uint(key);

				IPCAM_DBG(MINOR, "CH(%d) NF_IPCAM_FISHEYE_MOUNT db(%d), runtime(%d)\n", 
								port, mount, runtime[port].fisheye.mount.value);

				if(mount != runtime[port].fisheye.mount.value)
				{
					nf_ipcam_set_mount(port, &mount, NULL, NULL, NULL);
					runtime[port].fisheye.mount.value = mount;
				}
			}
			if (runtime[port].fisheye.fisheye_supported & NF_IPCAM_FISHEYE_DEWARP)
			{
				NF_IPCAM_DEWARP_MODES_E dewarp;

				snprintf(key, 64, "cam.fisheye.f%d.dewarp_mode", port);
				dewarp = nf_sysdb_get_uint(key);

				IPCAM_DBG(MINOR, "CH(%d) NF_IPCAM_FISHEYE_DEWARP db(%d), runtime(%d)\n", 
								port, dewarp, runtime[port].fisheye.dewarp.value);

				if(dewarp != runtime[port].fisheye.dewarp.value)
				{
					nf_ipcam_set_dewarp(port, &dewarp, NULL, NULL, NULL);
					runtime[port].fisheye.dewarp.value = dewarp;
				}
			}
		}

		for (port = 0; port < NUM_IPX_CHANNEL; port++)
		{
			if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0)
			{
				continue;
			}
			if(nf_ipcam_is_onvif_support(port) != 1)
			{
				NFIPCamSetupImage info;

				memset(&info, 0x00, sizeof(NFIPCamSetupImage));

//printf("SYSDB_CB(CH:%d) ", port);
	     snprintf(key, 64, "cam.C%d.sharpness", port);
	     sha = nf_sysdb_get_uint(key);
//printf("sharpness(%d) ", sha);
	     snprintf(key, 64, "cam.C%d.bright", port);
	     bri = nf_sysdb_get_uint(key);
//printf("brightness(%d) ", bri);
	     snprintf(key, 64, "cam.C%d.contrast", port);
	     con = nf_sysdb_get_uint(key);
//printf("contrast(%d) ", con);
	     snprintf(key, 64, "cam.C%d.tint", port);
	     tin = nf_sysdb_get_uint(key);
//printf("tint(%d) ", tin);
	     snprintf(key, 64, "cam.C%d.color", port);
	     col = nf_sysdb_get_uint(key);
//printf("color(%d) ", col);

				info.sharpness = sha;
				info.brightness = bri;
				info.contrast = con;
				info.color = col;
				info.tint = tin;

				runtime[port].image.sharpness.value = sha;
				runtime[port].image.brightness.value = bri;
				runtime[port].image.contrast.value = con;
				runtime[port].image.tint.value = tin;
				runtime[port].image.color.value = col;

				snprintf(key, 64, "cam.C%d.exposure_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("exposure(%d) ", etc);
				//info.exposure = (1<<etc);
				//
				if (etc & runtime[port].image.exposure.support)
				{
					info.exposure = etc;
					runtime[port].image.exposure.value = etc;
				}
				else
				{
					info.exposure = runtime[port].image.exposure.value;
				}

	#if 0
				if (etc == 0) { runtime[port].img.exposure.value = NF_IPCAM_IMAGE_EXP_MODE_MANUAL; }
				else if (etc == 1) { runtime[port].img.exposure.value = NF_IPCAM_IMAGE_EXP_MODE_AUTO; }
	#endif
				snprintf(key, 64, "cam.C%d.agc_gain", port);
				etc = nf_sysdb_get_uint(key);
//printf("agc_gain(%d) ", etc);
				// fixme agc_gain = gain
				if(etc != 0)
				{
					info.agc = etc;
					runtime[port].image.agc.value = etc;
				}

				snprintf(key, 64, "cam.C%d.gain", port);
				etc = nf_sysdb_get_int(key);
//printf("gain(%d) ", etc);
				if(etc != 0)
				{
					info.agc = etc;
					runtime[port].image.agc.value = etc;
				}

				snprintf(key, 64, "cam.C%d.dnn_ntod", port);
				etc = nf_sysdb_get_uint(key);
				if(etc != 0)
				{
					info.dnn_sense_ntod = etc;
					runtime[port].image.dnn_sense_ntod.value = etc;
				}

				snprintf(key, 64, "cam.C%d.dnn_dton", port);
				etc = nf_sysdb_get_uint(key);
				if(etc != 0)
				{
					info.dnn_sense_dton = etc;
					runtime[port].image.dnn_sense_dton.value = etc;
				}

				snprintf(key, 64, "cam.C%d.shutter_speed", port);
				etc = nf_sysdb_get_uint(key);
//printf("shutter_speed(%d) ", etc);
				if(etc != 0)
				{
					info.eshutter_speed = etc;
					runtime[port].image.eshutter_speed.value = etc;
				}

				snprintf(key, 64, "cam.C%d.etime", port);
				etc = nf_sysdb_get_int(key);
//printf("exp_time(%d) ", etc);
				if(etc != 0)
				{
					info.eshutter_speed = etc;
					runtime[port].image.eshutter_speed.value = etc;
				}


				snprintf(key, 64, "cam.C%d.slow_shutter", port);
				etc = nf_sysdb_get_uint(key);
//printf("slow_shutter(%d) ", etc);
				//info.slow_shutter = (1<<etc);
				//info.slow_shutter = etc;
				if (etc & runtime[port].image.slow_shutter.support)
				{
					info.slow_shutter = etc;
					runtime[port].image.slow_shutter.value = etc;
				}
				else
				{
					info.slow_shutter = runtime[port].image.slow_shutter.value;
				}

				snprintf(key, 64, "cam.C%d.max_agc", port);
				etc = nf_sysdb_get_uint(key);
//printf("max_agc(%d) ", etc);
				//info.max_agc = (1<<etc);
				//info.max_agc = etc;
				if (etc & runtime[port].image.max_agc.support)
				{
					info.max_agc = etc;
					runtime[port].image.max_agc.value = etc;
				}
				else
				{
					info.max_agc = runtime[port].image.max_agc.value;
				}

				snprintf(key, 64, "cam.C%d.iris_control", port);
				etc = nf_sysdb_get_uint(key);
//printf("iris(%d) ", etc);
				//info.iris = (1<<etc);
				//info.iris = etc;
				if (etc & runtime[port].image.iris.support)
				{
					info.iris = etc;
					runtime[port].image.iris.value = etc;
				}
				else
				{
					info.iris = runtime[port].image.iris.value;
				}

				snprintf(key, 64, "cam.C%d.blc_control", port);
				etc = nf_sysdb_get_uint(key);
//printf("blc(%d) ", etc);
				//info.blc = (1<<etc);
				//info.blc = etc;
				if (etc & runtime[port].image.blc.support)
				{
					info.blc = etc;
					runtime[port].image.blc.value = etc;
				}
				else
				{
					info.blc = runtime[port].image.blc.value;
				}

				snprintf(key, 64, "cam.C%d.day_night_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("dnn(%d) ", etc);
				//info.day_night = (1<<etc);
				//info.day_night = etc;
				if (etc & runtime[port].image.day_night.support)
				{
					info.day_night = etc;
					runtime[port].image.day_night.value = etc;
				}
				else
				{
					info.day_night = runtime[port].image.day_night.value;
				}

				snprintf(key, 64, "cam.C%d.day_night_duration", port);
				etc = nf_sysdb_get_uint(key);
//printf("toggle(%d) ", etc);
				//info.det_time = (1<<etc);
				//info.det_time = etc;
				if (etc & runtime[port].image.tg_time.support)
				{
					info.det_time = etc;
					runtime[port].image.tg_time.value = etc;
				}
				else
				{
					info.det_time = runtime[port].image.tg_time.value;
				}

				snprintf(key, 64, "cam.C%d.dnn_start_hour", port);
				etc = nf_sysdb_get_uint(key);
				info.dnn_start_hour = etc;
				runtime[port].image.dnn_schedule.start.hour = etc;

				snprintf(key, 64, "cam.C%d.dnn_start_min", port);
				etc = nf_sysdb_get_uint(key);
				info.dnn_start_min = etc;
				runtime[port].image.dnn_schedule.start.min = etc;

				snprintf(key, 64, "cam.C%d.dnn_end_hour", port);
				etc = nf_sysdb_get_uint(key);
				info.dnn_end_hour = etc;
				runtime[port].image.dnn_schedule.end.hour = etc;

				snprintf(key, 64, "cam.C%d.dnn_end_min", port);
				etc = nf_sysdb_get_uint(key);
				info.dnn_end_min = etc;
				runtime[port].image.dnn_schedule.end.min = etc;

				snprintf(key, 64, "cam.C%d.wb_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("wb(%d) ", etc);
				//info.white_balance = (1<<etc);
				//info.white_balance = etc;
				if (etc & runtime[port].image.wb.support)
				{
					info.white_balance = etc;
					runtime[port].image.wb.value = etc;
				}
				else
				{
					info.white_balance = runtime[port].image.wb.value;
				}

				snprintf(key, 64, "cam.C%d.mwb_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("mwb(%d) ", etc);
				//info.mwb = (1<<etc);
				//info.mwb = etc;
				if (etc & runtime[port].image.mwb.support)
				{
					info.mwb = etc;
					runtime[port].image.mwb.value = etc;
				}
				else
				{
					info.mwb = runtime[port].image.mwb.value;
				}

				snprintf(key, 64, "cam.C%d.wdr_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("wdr(%d) ", etc);
				if (etc & runtime[port].image.wd.support)
				{
					info.wdr= etc;
					runtime[port].image.wd.value = etc;
				}
				else
				{
					info.wdr = runtime[port].image.wd.value;
				}

				snprintf(key, 64, "cam.C%d.adaptive_ir", port);
				etc = nf_sysdb_get_uint(key);
				if(etc & runtime[port].image.adaptive_ir.support)
				{
					info.adaptive_ir = etc;
					runtime[port].image.adaptive_ir.value = etc;
				}
				else
				{
					info.adaptive_ir = runtime[port].image.adaptive_ir.value;
				}

				snprintf(key, 64, "cam.C%d.focus_mode", port);
				etc = nf_sysdb_get_uint(key);
//printf("focus(%d)\n", etc);
				if (etc & runtime[port].image.focus_mode.support)
				{
					info.focus_mode = etc;
					runtime[port].image.focus_mode.value = etc;
				}
				else
				{
					info.focus_mode = runtime[port].image.focus_mode.value;
				}

				snprintf(key, 64, "cam.C%d.antiflicker", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "antiflicker(%d)\n", etc);
				if (etc & runtime[port].image.anti_flicker.support)
				{
					info.anti_flicker = etc;
					runtime[port].image.anti_flicker.value = etc;
				}
				else
				{
					info.anti_flicker = runtime[port].image.anti_flicker.value;
				}

				snprintf(key, 64, "cam.C%d.defog", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "defog(%d)\n", etc);
				if (etc & runtime[port].image.defog.support)
				{
					info.defog= etc;
					runtime[port].image.defog.value = etc;
				}
				else
				{
					info.defog= runtime[port].image.defog.value;
				}

				snprintf(key, 64, "cam.C%d.hlc", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "hlc(%d)\n", etc);
				if (etc & runtime[port].image.hlc.support)
				{
					info.hlc= etc;
					runtime[port].image.hlc.value = etc;
				}
				else
				{
					info.hlc= runtime[port].image.hlc.value;
				}

				snprintf(key, 64, "cam.C%d.max_shutter", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "max_shutter(%d)\n", etc);
				if (etc & runtime[port].image.max_shutter.support)
				{
					info.max_shutter = etc;
					runtime[port].image.max_shutter.value = etc;
				}
				else
				{
					info.max_shutter = runtime[port].image.max_shutter.value;
				}

				snprintf(key, 64, "cam.C%d.base_shutter", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "base_shutter(%d)\n", etc);
				if (etc & runtime[port].image.base_shutter.support)
				{
					info.base_shutter = etc;
					runtime[port].image.base_shutter.value = etc;
				}
				else
				{
					info.base_shutter = runtime[port].image.base_shutter.value;
				}

				// DNR CONTROL MODE
				snprintf(key, 64, "cam.C%d.dnr_control", port);
				etc = nf_sysdb_get_uint(key);
				IPCAM_DBG(MINOR, "dnr_control(%d)\n", etc);
				if (etc & runtime[port].image.dnr_ctr.support)
				{
					info.dnr_ctr= etc;
					runtime[port].image.dnr_ctr.value = etc;
				}
				else
				{
					info.dnr_ctr= runtime[port].image.dnr_ctr.value;
				}

				// ITX NPT Focus Near Limit
				snprintf(key, 64, "cam.C%d.focus_limit", port);
				etc = nf_sysdb_get_uint(key);
				if (etc & runtime[port].image.focus_limit.support)
				{
					info.focus_limit = etc;
					runtime[port].image.focus_limit.value = etc;
				}
				else
				{
					info.focus_limit = runtime[port].image.focus_limit.value;
				}

				// Stabilizer
				snprintf(key, 64, "cam.C%d.stabilizer", port);
				etc = nf_sysdb_get_uint(key);
				if (etc & runtime[port].image.stabilizer.support)
				{
					info.stabilizer = etc;
					runtime[port].image.stabilizer.value = etc;
				}
				else
				{
					info.stabilizer = runtime[port].image.stabilizer.value;
				}

				//IR Correction 
				snprintf(key, 64, "cam.C%d.ir_correction", port);
				etc = nf_sysdb_get_uint(key);
				if (etc & runtime[port].image.ir_correction.support)
				{
					info.ir_correction = etc;
					runtime[port].image.ir_correction.value = etc;
				}
				else
				{
					info.ir_correction = runtime[port].image.ir_correction.value;
				}


				snprintf(key, 64, "cam.C%d.rotate", port);
				rot = nf_sysdb_get_uint(key);
//printf("rotation(%d) ", rot);

				if (runtime[port].sys.model_code != NF_IPCAM_MODEL_TI_368)
				{
					if((rot != runtime[port].video.mirror.value)
					&& (rot & runtime[port].video.mirror.support))
					{
						runtime[port].video.mirror.value = rot;
						nf_ipcam_set_vcodec_sysdb(port, NULL, NULL, NULL);
					}
				}
				else
				{
					if
					((rot != runtime[port].video.mirror.value)
					&& (rot & runtime[port].video.mirror.support))
					{
						runtime[port].video.mirror.value = rot;
						nf_ipcam_set_rotation(port, rot, NULL, NULL, NULL);
					}
				}

				//ColorVU Level
				if(runtime[port].image.supported & NF_IPCAM_IMAGE_COLORVU){
					snprintf(key, 64, "cam.C%d.illumination_level", port);
					etc = nf_sysdb_get_uint(key);
				    info.colorvu_level = etc;
					runtime[port].image.colorvu_level.value = etc;
				
				    snprintf(key, 64, "cam.C%d.illumination_level_ctl", port);
				    etc = nf_sysdb_get_uint(key);
					info.colorvu_ctrl = etc;
					runtime[port].image.colorvu_ctrl.value = etc;
				}

				nf_ipcam_set_image(port, &info, NULL, NULL, NULL);

				int major, type, subtype, minor;
				sscanf(runtime[port].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);


				if((strncmp(runtime[port].sys.stdver,"NPT", 3) != 0 ) ||
						(strncmp(runtime[port].sys.stdver,"NPT", 3) == 0 && type > 3))
				{
					if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_NTOD)
					{
						nf_ipcam_set_dnn_adjust_n2d(port, &info, NULL, NULL, NULL);
					}

					if(runtime[port].image.supported & NF_IPCAM_IMAGE_DNN_SENSE_DTON)
					{
						nf_ipcam_set_dnn_adjust_d2n(port, &info, NULL, NULL, NULL);
					}
				}

				if((runtime[port].image.supported & NF_IPCAM_IMAGE_PIRIS)
				&& (runtime[port].image.iris.value == NF_IPCAM_IMAGE_PIRIS_MANUAL || runtime[port].image.exposure.value == NF_IPCAM_IMAGE_EXP_MODE_MANUAL))
				{
					snprintf(key, 64, "cam.C%d.iris", port);
					etc = nf_sysdb_get_int(key);
					//IPCAM_DBG(MINOR, "manualiris(%d) ", etc);

					nf_ipcam_set_iris(port, etc, NULL, NULL, NULL);
				}

				snprintf(key, 64, "cam.C%d.focus_mode", port);
				etc = nf_sysdb_get_uint(key);

				if((runtime[port].image.supported & NF_IPCAM_IMAGE_FOCUS)
				&& etc == NF_IPCAM_FOCUS_MODE_ITX_ABSOLUTE)
				{
					snprintf(key, 64, "cam.C%d.focus_abposition", port);
					etc = nf_sysdb_get_int(key);
					//IPCAM_DBG(MINOR, "manualiris(%d) ", etc);

					nf_ipcam_set_focus(port, etc, NULL, NULL, NULL);
				}

				// Set auto focus
				if(runtime[port].ptz.supported & PTZ_SETUP_FOCUS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_focus", port);
					etc = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_focus(port, etc);
				}


				// Set auto iris
				if(runtime[port].ptz.supported & PTZ_SETUP_IRIS)
				{
					snprintf(key, 64, "cam.ptz.P%d.auto_iris", port);
					etc = nf_sysdb_get_bool(key);
					nf_ipcam_set_auto_iris(port, etc);
				}


				NFIPCamSetupFocusComp focus_comp_info;
				memset(&focus_comp_info, 0x00, sizeof(focus_comp_info));
				// Focus Compensation
				if(runtime[port].focus.supported & NF_IPCAM_FOCUS_DNN_COMP)
				{
					// Focus Day and Night Compensation
					snprintf(key, 64, "cam.C%d.focus_dnn_comp", port);
					etc = nf_sysdb_get_bool(key);

					if(etc == 0 || etc == 1)
						focus_comp_info.dnn_comp_mode = etc;
					else
						focus_comp_info.dnn_comp_mode = 1;
				}

				if(runtime[port].focus.supported & NF_IPCAM_FOCUS_TEM_COMP)
				{
					// Focus Temperature Compensation
					snprintf(key, 64, "cam.C%d.focus_tem_comp", port);
					etc = nf_sysdb_get_bool(key);

					if(etc == 0 || etc == 1)
						focus_comp_info.tem_comp_mode = etc;
					else
						focus_comp_info.tem_comp_mode = 1;
				}

				nf_ipcam_set_focus_compensation(port, &focus_comp_info, NULL, NULL, NULL);

			}
			else if(__OFM(NF_ONVIF_SERVICE_IMAGE) & runtime[port].onvif.onvif_service)
			{
printf(MINOR, "SYSDB_CB(CH:%d) ", port);
				NFIPCamSetupImage_onvif info;
				memset(&info, 0x00, sizeof(NFIPCamSetupImage_onvif));

				int value;

				snprintf(key, 64, "cam.C%d.bright", port);
				value = nf_sysdb_get_uint(key);
printf(MINOR, "bright(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
				{
					info.brightness = value;
				}
				else
				{
					info.brightness = runtime[port].image_onvif.brightness.value;
				}

				snprintf(key, 64, "cam.C%d.color", port);
				value = nf_sysdb_get_uint(key);
//printf("color(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
				{
					info.color = value;
				}
				else
				{
					info.color = runtime[port].image_onvif.color.value;
				}

				snprintf(key, 64, "cam.C%d.contrast", port);
				value = nf_sysdb_get_uint(key);
//printf("contrast(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
				{
					info.contrast = value;
				}
				else
				{
					info.contrast = runtime[port].image_onvif.contrast.value;
				}

				snprintf(key, 64, "cam.C%d.sharpness", port);
				value = nf_sysdb_get_uint(key);
//printf("sharpness(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
				{
					info.sharpness = value;
				}
				else
				{
					info.sharpness = runtime[port].image_onvif.sharpness.value;
				}

				snprintf(key, 64, "cam.C%d.focus_mode", port);
				value = nf_sysdb_get_uint(key);
//printf("focus_mode(%d) ", value);
				if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_MODE)
				&& (runtime[port].image_onvif.focus.mode.support & value))
				{
					info.focus_mode = value;
				}
				else
				{
					info.focus_mode = runtime[port].image_onvif.focus.mode.value;
				}

				snprintf(key, 64, "cam.C%d.focus_default_speed", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
				{
					info.default_speed = value;
				}
				else
				{
					info.default_speed = runtime[port].image_onvif.focus.defaultspeed.value;
				}

				snprintf(key, 64, "cam.C%d.focus_near_limit", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
				{
					info.near_limit = value;
				}
				else
				{
					info.near_limit = runtime[port].image_onvif.focus.nearlimit.value;
				}

				snprintf(key, 64, "cam.C%d.focus_far_limit", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
				{
					info.far_limit = value;
				}
				else
				{
					info.far_limit = runtime[port].image_onvif.focus.farlimit.value;
				}

				snprintf(key, 64, "cam.C%d.wb_mode", port);
				value = nf_sysdb_get_uint(key);
//printf("wb_mode(%d) ", value);
				if((runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_MODE)
				&& (runtime[port].image_onvif.wb.mode.support & value))
				{
					info.white_balance = value;
				}
				else
				{
					info.white_balance = runtime[port].image_onvif.wb.mode.value;
				}

				snprintf(key, 64, "cam.C%d.wb_crgain", port);
				value = nf_sysdb_get_int(key);
//printf("wb_crgain(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
				{
					info.crgain = value;
				}
				else
				{
					info.crgain = runtime[port].image_onvif.wb.crgain.value;
				}

				snprintf(key, 64, "cam.C%d.wb_cbgain", port);
				value = nf_sysdb_get_int(key);
//printf("wb_cbgain(%d) ", value);
				if(runtime[port].image_onvif.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
				{
					info.cbgain = value;
				}
				else
				{
					info.cbgain = runtime[port].image_onvif.wb.cbgain.value;
				}



				NFIPCamSetupExposure_onvif info2;
				memset(&info2, 0x00, sizeof(NFIPCamSetupExposure_onvif));

				snprintf(key, 64, "cam.C%d.exposure_mode", port);
				value = nf_sysdb_get_uint(key);
//printf("exp_mode(%d) \n", value);
				if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MODE)
				&& (runtime[port].image_onvif.exposure.mode.support & value))
				{
					info2.mode = value;
				}
				else
				{
					info2.mode = runtime[port].image_onvif.exposure.mode.value;
				}

				snprintf(key, 64, "cam.C%d.exposure_priority", port);
				value = nf_sysdb_get_uint(key);
				if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_PRIORITY)
				&& (runtime[port].image_onvif.exposure.priority.support & value))
				{
					info2.priority = value;
				}
				else
				{
					info2.priority = runtime[port].image_onvif.exposure.priority.value;
				}

				snprintf(key, 64, "cam.C%d.min_etime", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
				{
					info2.minetime = value;
				}
				else
				{
					info2.minetime = runtime[port].image_onvif.exposure.minetime.value;
				}

				snprintf(key, 64, "cam.C%d.max_etime", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
				{
					info2.maxetime = value;
				}
				else
				{
					info2.maxetime = runtime[port].image_onvif.exposure.maxetime.value;
				}


				snprintf(key, 64, "cam.C%d.etime", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
				{
					info2.etime = value;
				}
				else
				{
					info2.etime = runtime[port].image_onvif.exposure.etime.value;
				}

				snprintf(key, 64, "cam.C%d.min_gain", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
				{
					info2.mingain = value;
				}
				else
				{
					info2.mingain = runtime[port].image_onvif.exposure.mingain.value;
				}

				snprintf(key, 64, "cam.C%d.max_gain", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
				{
					info2.maxgain = value;
				}
				else
				{
					info2.maxgain = runtime[port].image_onvif.exposure.maxgain.value;
				}

				snprintf(key, 64, "cam.C%d.gain", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
				{
					info2.gain = value;
				}
				else
				{
					info2.gain = runtime[port].image_onvif.exposure.gain.value;
				}

				snprintf(key, 64, "cam.C%d.min_iris", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
				{
					info2.miniris = value;
				}
				else
				{
					info2.miniris = runtime[port].image_onvif.exposure.miniris.value;
				}

				snprintf(key, 64, "cam.C%d.max_iris", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
				{
					info2.maxiris = value;
				}
				else
				{
					info2.maxiris = runtime[port].image_onvif.exposure.maxiris.value;
				}

				snprintf(key, 64, "cam.C%d.iris_control", port);
				value = nf_sysdb_get_uint(key);
				if (runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_DCIRIS)
				{
					info2.iris_control = value;
				}
				else
				{
					info2.iris_control = runtime[port].image_onvif.exposure.iris_mode.value;
				}

				snprintf(key, 64, "cam.C%d.iris", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
				{
					info2.iris = value;
				}
				else
				{
					info2.iris = runtime[port].image_onvif.exposure.iris.value;
				}

				snprintf(key, 64, "cam.C%d.blc_control", port);
				value = nf_sysdb_get_uint(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE)
				{
					info2.blc_mode = value;
				}
				else
				{
					info2.blc_mode = runtime[port].image_onvif.blcmode.value;
				}

				snprintf(key, 64, "cam.C%d.blc_level", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
				{
					info2.blc_level = value;
				}
				else
				{
					info2.blc_level = runtime[port].image_onvif.blclevel.value;
				}

				snprintf(key, 64, "cam.C%d.wdr_mode", port);
				value = nf_sysdb_get_uint(key);
				if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE)
				&& (runtime[port].image_onvif.wdrmode.support & value))
				{
					info2.wide_dynamic_mode = value;
				}
				else
				{
					info2.wide_dynamic_mode = runtime[port].image_onvif.wdrmode.value;
				}

				snprintf(key, 64, "cam.C%d.wdr_level", port);
				value = nf_sysdb_get_int(key);
				if(runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
				{
					info2.wide_level = value;
				}
				else
				{
					info2.wide_level = runtime[port].image_onvif.wdrlevel.value;
				}


				snprintf(key, 64, "cam.C%d.day_night_mode", port);
				value = nf_sysdb_get_uint(key);
//printf("dn_mode(%d) ", value);
				if((runtime[port].image_onvif.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRCUT)
				&& (runtime[port].image_onvif.ircut.support & value))
				{
					info2.ircut = value;
				}
				else
				{
					info2.ircut = runtime[port].image_onvif.ircut.value;
				}
				//nf_ipcam_set_exposure_onvif(port, &info2, NULL, NULL, NULL);

				nf_ipcam_set_image_exp_onvif(port, &info, &info2, NULL, NULL, NULL);
/*
				if(info.focus_mode != NF_IPCAM_FOCUS_MODE_ONVIF_AUTO)
				{
					NFIPCamSetupFocus_onvif info3;
					memset(&info3, 0x00, sizeof(NFIPCamSetupFocus_onvif));

					info3.mode = info.focus_mode;

					snprintf(key, 64, "cam.C%d.focus_abposition", port);
					value = nf_sysdb_get_int(key);
					if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
					&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
					{
						info3.position = value;
					}
					snprintf(key, 64, "cam.C%d.focus_abspeed", port);
					value = nf_sysdb_get_int(key);
					if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
					&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE)
					{
						info3.speed = value;
					}
					snprintf(key, 64, "cam.C%d.focus_redistance", port);
					value = nf_sysdb_get_int(key);
					if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
					&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
					{
						info3.distance = value;
					}
					snprintf(key, 64, "cam.C%d.focus_respeed", port);
					value = nf_sysdb_get_int(key);
					if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
					&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
					{
						info3.speed = value;
					}
					snprintf(key, 64, "cam.C%d.focus_cospeed", port);
					value = nf_sysdb_get_int(key);
					if((runtime[port].image_onvif.focus.mode.support & NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
					&& info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS)
					{
						info3.speed = value;
					}

					if(info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE || info3.mode == NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE)
					{
						nf_ipcam_set_focus_onvif(port, &info3, NULL, NULL, NULL);
					}
				}
*/

				snprintf(key, 64, "cam.C%d.rotate", port);
				value = nf_sysdb_get_uint(key);

				if
				((value != runtime[port].video.mirror.value)
				&& (value & runtime[port].video.mirror.support) && (runtime[port].video.supported & VIDEO_SETUP_MIRROR))
				{
					nf_ipcam_set_rotation(port, value, NULL, NULL, NULL);
				}

			}
		}

		for(port = 0; port < NUM_IPX_CHANNEL; port++)
		{
			// only clair-2nd
			if(nf_ipcam_get_cam_ai_type(port)!=CAM_AI_TYPE_CLAIR2)
				continue;

			if(_is_updated_ai_rule_engine(port))
			{
				
				pthread_t _ai_rule_engine_setup_th;
				pthread_create(&_ai_rule_engine_setup_th, NULL, (void*)&itx_cam_set_ai_rule_engine, port);
				pthread_detach(_ai_rule_engine_setup_th);
			}

			if(_is_updated_dl_option(port))
			{
				pthread_t _dl_option_setup_th;
				pthread_create(&_dl_option_setup_th, NULL, (void*)&itx_cam_set_ai_dl_option, port);
				pthread_detach(_dl_option_setup_th);
			}

			if(_is_updated_embedded_osd(port))
			{
				pthread_t _e_osd_setup_th;
				pthread_create(&_e_osd_setup_th, NULL, (void*)&itx_cam_set_embedded_osd, port);
				pthread_detach(_e_osd_setup_th);
			}
		}
	}
	else if (pinfo->d.params[0] == NF_SYSDB_CATE_ALARM)
	{
		int i;
		int result;
		unsigned int sense_d;
		int port;
		char key[64];
		int ch_mask = 0;
		GAsyncQueue *queue = get_queue();

		for (port = 0; port < NUM_IPX_CHANNEL; port++)
		{
				ch_mask |= (1<<port);
		}

		nf_ipcam_set_motion_thread(ch_mask);

		for (port = 0; port < NUM_IPX_CHANNEL; port++)
		{
			if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
			if ((runtime[port].func & NF_IPCAM_FUNC_ALARM_IN) == 0) { continue; }
			snprintf(key, 64, "alarm.sensor.S%d.op_type", port);
			sense_d = nf_sysdb_get_bool(key);
			if ((runtime[port].alarm.alarm_in_type.support & (1<<sense_d)) != 0)
			{
				nf_ipcam_set_alarm_in(port, 1, 1<<sense_d, NULL, NULL, NULL);
			}
		}
	}
/*
	if(runtime[i].video.corridor_support == 1 )
	{
		nf_ipcam_set_corridor_mode(i, NULL,NULL,NULL,NULL); 
	}*/
}

/**
 * @brief 일부 채널들에 대한 sysdb 변경 notify를 받았을 시("sysdb_ipcam_change") 처리한다.
 * @param pinfo Notify 정보 struct, param[0] = 카테고리, param[1] = 채널 마스크.
 * @param data user data형식이며, 현재 항상 NULL임.
 *
 * 현재 VCA설정 용도로만 사용한다.
 *
 * @see DAL_notify_fire_ipcam_change
 */
static void _api_sysdb_ipcam_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint ch_mask=0;
	gint i=0;
	gint active = 0;
	gchar key[64];
	gint ret = 0;
	mtable *runtime = get_runtime();

	if (pinfo->d.params[0] == NF_IPCAM_CATE_VCA)
	{
		ch_mask = pinfo->d.params[1];
		for (i=0; i<NUM_ACTIVE_CH; i++)
		{
			if ((ch_mask & (1<<i)) != 0)
			{
				if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0)
				{
					continue;
				}
				if ((runtime[i].func & NF_IPCAM_FUNC_VA) == 0)
				{
					continue;
				}

				//nf_ipcam_set_va_option(i, NULL, NULL, NULL);
				nf_ipcam_set_va_config(i, NULL, NULL, NULL);
			}
		}
	}else
	if (pinfo->d.params[0] == NF_IPCAM_CATE_STREAM)
	{
		unsigned int sense_d;
		ch_mask = pinfo->d.params[1];
		for (i=0; i<NUM_ACTIVE_CH; i++)
		{
			if ((ch_mask & (1<<i)) != 0)
			{
				if(runtime[i].func & NF_IPCAM_FUNC_MOTION)
				{	/* Motion Area */
					NFIPCamSetupMotionArea info;
					gchar *area;

					memset(&info, 0x00, sizeof(NFIPCamSetupMotionArea));
					snprintf(key, 64, "alarm.motion.M%d.area", i);
					area = nf_sysdb_get_str_nocopy(key);
					strncpy(info.area, area, 1400);

					if (get_dn_now(i) == 1)
					{
						snprintf(key, 64, "alarm.motion.M%d.sense_d", i);
					}
					else
					{
						snprintf(key, 64, "alarm.motion.M%d.sense_n", i);
					}
					sense_d = nf_sysdb_get_uint(key);
					snprintf(key, 64, "alarm.motion.M%d.rect.RCNT", i);
					info.area_num = nf_sysdb_get_uint(key);

					printf("[%s:%d] db areanum[%d]\n", __func__, __LINE__, info.area_num);

					if (info.area_num == 0)
					{
						info.ch = i;
						info.block_width = runtime[i].motion.block_width;
						info.block_height = runtime[i].motion.block_height;
						info.method = runtime[i].motion.method;
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
						int j = 0;
						info.ch = i;
						info.block_width = runtime[i].motion.block_width;
						info.block_height = runtime[i].motion.block_height;
						info.method = runtime[i].motion.method;
						for (j = 0; j < info.area_num; j++)
						{
							info.marea[j].sensitivity = sense_d;
							snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", i, j);
							info.marea[j].FIGURE.RECTANGLE.left_top.y = nf_sysdb_get_uint(key);
							snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", i, j);
							info.marea[j].FIGURE.RECTANGLE.left_top.x = nf_sysdb_get_uint(key);
							snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", i, j);
							info.marea[j].FIGURE.RECTANGLE.right_bottom.y = nf_sysdb_get_uint(key);
							snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", i, j);
							info.marea[j].FIGURE.RECTANGLE.right_bottom.x = nf_sysdb_get_uint(key);
						}
					}

					ret = nf_ipcam_set_motion_area(i, &info, NULL, NULL, NULL);
				}

				if(runtime[i].motion.smart_motion_support)
				{	/* Smart Motion*/
					NFIPCamSetupMotionSmart info;
					memset(&info, 0x00, sizeof(NFIPCamSetupMotionSmart));

					//db값으로 runtime 객체 update
					info.ch = i;

					snprintf(key, 64, "alarm.motion.M%d.smart_motion", i);
					info.smart_motion_enable = nf_sysdb_get_bool(key);

					snprintf(key, 64, "alarm.motion.M%d.use_ai_alarmevt", i);
					info.ai_alarm_event = nf_sysdb_get_bool(key);

					snprintf(key, 64, "alarm.motion.M%d.smart_interest_obj", i);
					info.smart_motion_option_size = smart_motion_parse_db_string(info.smart_motion_options, nf_sysdb_get_str_nocopy(key));

					ret = nf_ipcam_set_motion_smart(i, &info, NULL, NULL, NULL);
				}
			}
		}
	}
}

/**
 * @brief Switch register 설정을 변경하여 내부 포트의 카메라를 외부에서 접속할 수 있도록 한다.
 * @param nvr_mask 사용안함.
 * @param extension_mask 사용안함.
 * @return 항상 IPCAM_SETUP_RTN_DONE.
 *
 * 카메라 펌웨어 업데이트 모드(치트 입력)에서 사용한다.
 */
int nf_ipcam_switch_mode(guint32 nvr_mask, guint32 extension_mask)
{
	// Set Vlan OPEN MODE. 
	nf_dev_switch_init(NF_UTIL_SWITCH_OPEN_MODE);
	
	/* if (16ch) hub_fwup_mode go~ */
	hub_camfwup_request();

	return IPCAM_SETUP_RTN_DONE;
}




static int _is_valid_version(int model, const char* my_swver);

/**
 * @brief 카메라의 상태(주로 버전 정보)를 조회한다.
 * @param[in] ch_num 채널 번호.
 * @param[out] info 카메라 상태 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @todo 현재 카메라 모델간 버전체계가 틀려 버전체크 루틴이 주석처리됨.
 * @todo 비슷한 API들 통합.
 * @sa nf_ipcam_get_config_info, nf_ipcam_get_model_info, nf_ipcam_get_port_status
 */
int nf_ipcam_status_check(int ch_num, NFIPCamStatusInfo* info)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail(info != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(info, 0x00, sizeof(NFIPCamStatusInfo));

	if (runtime[ch_num].state & MGMT_STATE_CONFIGURED == 0)
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_OK;
		snprintf(info->description, 256, "NO DEVICE");
		return IPCAM_SETUP_RTN_DONE;
	}
#if 0
	if (runtime[ch_num].sys.model_code != NF_IPCAM_MODEL_AMB_A2 &&
		runtime[ch_num].sys.model_code != NF_IPCAM_MODEL_AMB_D1)
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_OK;
		snprintf(info->description, 256, "OK");
		return IPCAM_SETUP_RTN_DONE;
	}

	if (_is_valid_version(1936, runtime[ch_num].sys.model_code, runtime[ch_num].sys.swver))
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_OK;
		snprintf(info->description, 256, "OK");
		return IPCAM_SETUP_RTN_DONE;
	}
	if (_is_valid_version(1935, runtime[ch_num].sys.model_code, runtime[ch_num].sys.swver))
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_OK;
		snprintf(info->description, 256, "OK");
		return IPCAM_SETUP_RTN_DONE;
	}

	info->ch = ch_num;
	info->status = NF_IPCAM_STATUS_WARN_FW_VERSION;
	snprintf(info->description, 256, "UNSUPPORTED F/W VERSION");
	snprintf(info->recommendation, 256, "F/W MINOR VERSION 1935-1936 WAS TESTED");
#else
#if 0
	if (strncmp(runtime[ch_num].sys.sdkver, "1.", 2) != 0)
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_WARN_SDKVER;
		snprintf(info->description, 256, "UNTESTED IPCAM F/W VERSION");
		snprintf(info->recommendation, 256, "UPGRADE NVR F/W or DOWNGRADE IPCAM F/W");
		return IPCAM_SETUP_RTN_DONE;
	}
#endif
#if 0 // 2014-08-08 jykim
	switch (runtime[ch_num].sys.model_code)
	{
		case NF_IPCAM_MODEL_TI_368:
		{
			if ((strncmp(runtime[ch_num].sys.sdkver, "1.0.0.2", 7) != 0) &&
				(strncmp(runtime[ch_num].sys.sdkver, "1.0.0.3", 7) != 0) &&
				(strncmp(runtime[ch_num].sys.sdkver, "1.0.0.4", 7) != 0))
			{
				info->ch = ch_num;
				info->status = NF_IPCAM_STATUS_WARN_FW_VERSION;
				snprintf(info->description, 256, "UNSUPPORTED F/W VERSION");
				snprintf(info->recommendation, 256, "UPGRADE IPCAM F/W");
				return IPCAM_SETUP_RTN_DONE;
			}
			break;
		}
	}
#endif
#if 0
	if (_is_valid_version(runtime[ch_num].sys.model_code, runtime[ch_num].sys.swver))
	{
		info->ch = ch_num;
		info->status = NF_IPCAM_STATUS_OK;
		snprintf(info->description, 256, "OK");
		return IPCAM_SETUP_RTN_DONE;
	}
#endif

	info->ch = ch_num;
	info->status = NF_IPCAM_STATUS_OK;
	snprintf(info->description, 256, "OK");
#endif

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 버전이 '권장하는' 버전인지 확인한다.
 * @param model 카메라 모델 코드. @see NF_IPCAM_MODEL_CODE_E
 * @param my_swver 카메라의 버전(ex 51110.2.1536.100).
 * @return TRUE - 권장하는 버전, FALSE -  권장하지 않는 버전.
 *
 * @deprecated 기존 및 신규모델간 버전체계가 틀려저 현재 사용불가.
 */
static int _is_valid_version(int model, const char* my_swver)
{
	const int min_ti_ver = 1030;
	const int min_a2_ver = 1935;
	const int min_ptz_ver = 1030;
	char vendor[8];
	int g1;
	int minor_a;
	int vendor_code;
	int rtn = 0;

	memset(vendor, 0x00, 8);
	sscanf(my_swver, "%c%c%c%c%c.%d.%d.%d",
			&vendor[0], &vendor[1], &vendor[2], &vendor[3], &vendor[4],
			&g1, &minor_a, &vendor_code);

	switch(model)
	{
		case NF_IPCAM_MODEL_AMB_A2:
		case NF_IPCAM_MODEL_AMB_D1:
			//printf("[%s] A2-Cams %s.%d.%d.%d\n", __FUNCTION__,
			//		vendor, g1, minor_a, vendor_code);
			if (min_a2_ver <= minor_a)
			{
				rtn = 1;
			}
			break;
		case NF_IPCAM_MODEL_TI_368:
			//printf("[%s] TI-Cams %s.%d.%d.%d\n", __FUNCTION__,
			//		vendor, g1, minor_a, vendor_code);
			if (min_ti_ver <= minor_a)
			{
				rtn = 1;
			}
			break;
		default:
			rtn = 1;
			break;
	}

	return rtn;
}
#if 0
static int _is_valid_version(int ver, int model, const char* my_swver)
{
	const int a2_vendor_cnt = 10;
	char *a2_vendor_names[] = {
		"51110", "ZN2C0", "KBD01", "ERV01", "PSP01",
		"VCN01", "GPS01", "SPS01", "LTV01", "SCS01"
	};
	int a2_vendor_codes[] = {
		100, 32, 78, 92, 100,
		96, 95, 79, 42, 88
	};

	int i = 0;
	char sw_ver[128];


	printf("[%s] ver(%d), my_swver(%s)\n", __FUNCTION__, ver, my_swver);

	if (model == NF_IPCAM_MODEL_AMB_A2 || model == NF_IPCAM_MODEL_AMB_D1)
	{
		for (i = 0; i < a2_vendor_cnt; i++)
		{
			snprintf(sw_ver, 128, "%s.2.%d.%d", a2_vendor_names[i], ver, a2_vendor_codes[i]);

			if (strncmp(my_swver, sw_ver, 128) == 0)
			{
				return (1);
			}
		}
	}

	return (0);
}
#endif

/**
 * @brief 해당 채널의 ONVIF 카메라 여부를 조회한다.
 * @param ch 채널 번호.
 * @return 1 - ONVIF 카메라, 0 - 기타.
 */
int nf_ipcam_is_onvif_support(int ch)
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, 0);
	if(!(runtime[ch].state & MGMT_STATE_READY))
	{
		return 0;
	}

	if(runtime[ch].sys.model_code == NF_IPCAM_MODEL_ONVIF
    || runtime[ch].sys.model_code == NF_IPCAM_MODEL_ONVIF_L1
    || runtime[ch].sys.model_code == NF_IPCAM_MODEL_ONVIF_GRUNDIG)
	{
		return 1;
	}
	return 0;
}

int nf_ipcam_camera_vendor_is_itx(int ch)
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	if(runtime == NULL && (ch >= 0 && ch < AVAILABLE_MAX_CH))
		return 0;
	if(!(runtime[ch].state & MGMT_STATE_READY))
		return 0;

    if(runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
		return 1;
	if(runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2)
		return 1;
    if(runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1)
		return 1;


	return 0;
}

unsigned int nf_ipcam_get_supported_dlva_ch_mask()
{
	int i;
	unsigned int ret = 0;

	for(i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		//if(nf_ipcam_camera_vendor_is_itx(i))
			ret |= 1 << i;
	}

	return ret;
}

/**
 * @brief 해당 카메라의 녹화설정(fps, bitrate)을 변경할 것인지를 결정한다.
 * @param ch 채널 번호.
 * @return 1, 2 - 변경 가능, 0 - 변경 불가.
 */
int nf_ipcam_is_config_changable(int ch)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, 0);

	if (nf_ipcam_is_vendor_orion() || nf_ipcam_is_vendor_g4s() || nf_ipcam_is_vendor_asp() || nf_ipcam_is_vendor("CBC"))
	{
		rtn = 1;
	}
	else if (runtime[ch].sys.model_code <= NF_IPCAM_MODEL_AMB_D1 ||
		runtime[ch].sys.model_code == NF_NVS_MODEL_ITX)
	{
		rtn = 2;
	}
	else if ((runtime[ch].video.supported & runtime[ch].video.onthefly & VIDEO_SETUP_BITRATE)
		&& (runtime[ch].video.supported & runtime[ch].video.onthefly & VIDEO_SETUP_FPS))
	{
		rtn = 1;
	}
	else
	{
		rtn = 0;
	}
	return rtn;
}

/**
 * @brief 해당 채널의 Hisilicon 카메라 여부를 조회한다.
 * @param ch 채널 번호.
 * @return 1 - Hisilicon 카메라, 0 - 기타.
 */
int nf_ipcam_is_hisilicon_camera(int ch)
{
	mtable* runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, 0);

	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version
	sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);	

	if(runtime[ch].sys.sdkver == NULL)
		return 0;

	if(type == ITX_CAM_SDK_TYPE_HS)
		return 1;

	return 0;
}

// AUXILIARY COMMAND API

/**
 * @brief Camera의 Auxiliary 명령을 가져온다.
 * @param[in] ch 전송하고자 하는 채널
 * @param[out] commands 명령정보
 * @param[out] error 에러정보
 * @returns 성공(0), 잘못된 카테고리(1), Command 가 0개(2)
 */
int nf_ipcam_get_auxiliary_commands(gint ch, gint category, NFIPCamAuxiliary* info, GError **error)
{
	int rtn = -1;
	mtable *runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL && ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);

	memset(info, 0x00, sizeof(NFIPCamAuxiliary));

	switch(category)
	{
		case AUXILIARY_CATEGORY_PTZ:
			if( runtime[ch].ptz_onvif.auxiliary.size > 0 && MAX_AUXILIARY_COMMANDS >= runtime[ch].ptz_onvif.auxiliary.size )
			{
				memcpy(info, &runtime[ch].ptz_onvif.auxiliary, sizeof(NFIPCamAuxiliary));

				rtn = 0;
				goto end_label;
			}
			else
			{
				rtn = 1;
				goto end_label;
			}

			break;

		case AUXILIARY_CATEGORY_DEVICE:
			if( runtime[ch].onvif.auxiliary.size > 0 && MAX_AUXILIARY_COMMANDS >= runtime[ch].onvif.auxiliary.size )
			{
				memcpy(info, &runtime[ch].onvif.auxiliary, sizeof(NFIPCamAuxiliary));

				rtn = 0;
				goto end_label;
			}
			else
			{
				rtn = 1;
				goto end_label;
			}

			break;

		default:
			rtn = -1;
			break;
	}

end_label:
	return rtn;
}

/**
 * @brief 카메라에 Auxiliary 명령을 전달한다.
 * @param[in] ch 전송하고자 하는 채널
 * @param[in] command_num 전송하고자 하는 명령어 번호. 
 * @param[out] error 에러정보
 */
int nf_ipcam_send_auxiliary_command(gint ch, gint category, gint command_index, GError **error)
{
	int rtn = -1;
	int size = 0;

	mtable *runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL && ch >= 0, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime != NULL, 0);

	switch(category)
	{
		case AUXILIARY_CATEGORY_PTZ:
			size = runtime[ch].ptz_onvif.auxiliary.size; 
			if(size > command_index && command_index >= 0)
			{
				rtn = nf_onvif_ptz_send_auxiliary_command(ch, runtime[ch].ptz_onvif.auxiliary.commands[command_index]);
			}
			break;

		case AUXILIARY_CATEGORY_DEVICE:
			size = runtime[ch].onvif.auxiliary.size; 
			if(size > command_index && command_index >= 0)
			{
				rtn = nf_onvif_device_send_auxiliary_command(ch, runtime[ch].onvif.auxiliary.commands[command_index]);
			}
			break;

		default:
			rtn = -1;
			break;
	}

	return rtn;
}

#include "nf_util_netif.h"

/**
 * @brief IP주소를 문자열로 변경한다.
 * @param[out] buff 대상 문자열.
 * @param[in] buff_len 문자열 길이.
 * @param[in] addr IP주소.
 */
static void my_inet_ntoa( gchar *buff, guint buff_len, guint addr)
{
	struct sockaddr_in in_addr;	
	
	g_return_if_fail( buff );
	
	in_addr.sin_addr.s_addr = addr;	
	snprintf(buff, buff_len, inet_ntoa( in_addr.sin_addr ) );
		
}

/**
 * @brief 카메라의 Direct Config 지원 여부를 조회한다.
 * @param ch 채널 번호.
 * @return TRUE - 지원함.
 *
 * 현재는 카메라가 연결만 되어 있으면 지원함.
 */
gboolean nf_ipcam_is_dconf_support( gint ch )
{
	gboolean rtn = 1;
	mtable *runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, 0);

	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		rtn = 0;
	}
#if 0
	if (runtime[ch].sys.model_code < NF_IPCAM_MODEL_ONVIF)
	{
		rtn = 0;
	}
#endif

	return rtn;
}

/**
 * @brief Iptables 커맨드를 통해 내부 포트의 카메라를 외부에서 접속할 수 있도록 설정한다.
 * @param ch 채널 번호.
 * @return 1 - 성공, 0 - 실패.
 */
gboolean nf_ipcam_direct_config_start( gint ch )
{
	char cmd_buff[1024];
	unsigned int nvr_web_port = 0, ipcam_http_port = 0;
	unsigned int nvr_ip = 0, ipcam_ip = 0;
	int i=0;

	char nvr_ipaddr_str[128], ipcam_ipaddr_str[128];

	NF_NETIF_GET_INFO ret_info;

	guint vloss_status = 0;
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	guint16 hport = nf_ipcam_get_dconf_port(ch);	
	guint16 use_ssl = runtime[ch].sys.use_ssl;
	guchar vendor[64];
	memcpy(vendor, runtime[ch].sys.vendor, 64);
	
	//IPCAM_DBG(MAJOR, "start CH(%d)", ch);

	ipcam_http_port = nf_ipcam_get_dconf_port(ch);	
	ipcam_ip = nf_ipcam_get_ipaddr(ch);	

	if( ipcam_http_port == 0 || ipcam_ip == 0 || ipcam_ip == 0xffffffff)
	{
		IPCAM_DBG(ERROR, "ipcam addr[%08x] http_port[%d]\n", ipcam_ip, ipcam_http_port);
		return 0;
	}
	my_inet_ntoa(ipcam_ipaddr_str, sizeof(ipcam_ipaddr_str), ipcam_ip );	


	nvr_web_port = nf_sysdb_get_uint("net.proto.webport");		
	memset( &ret_info, 0x00, sizeof(ret_info));	
	nf_netif_get_info( &ret_info);	

	if( ret_info.ipaddr == 0 || ret_info.ipaddr == 0xffffffff)
	{
		IPCAM_DBG(ERROR, "nvr addr[%08x] http_port[%d]\n", ret_info.ipaddr, nvr_web_port);
		return 0;
	}
	my_inet_ntoa(nvr_ipaddr_str, sizeof(nvr_ipaddr_str), htonl(ret_info.ipaddr) );
			
	// enable DNAT							
	proxy_system("/sbin/sysctl -w net.core.rmem_default=144280",1,3);
	proxy_system("echo 1 > /proc/sys/net/ipv4/ip_forward",1,3);
	proxy_system("iptables -t nat -F",1,3); 
			
	// for DNAT ( external )
	snprintf(cmd_buff, sizeof(cmd_buff), 
				"iptables -t nat -A PREROUTING -d %s  -p tcp --dport %d -j DNAT --to %s:%d", 
				nvr_ipaddr_str, nvr_web_port, ipcam_ipaddr_str, ipcam_http_port	);
	proxy_system(cmd_buff ,1,3); 
	
	// for DNAT (all connected ipcam)
	for(i=0;i<NUM_ACTIVE_CH;++i)
	{
		unsigned int ipcam_http_port = 0;
		unsigned int ipcam_ip = 0;
		char ipcam_ipaddr_str[128];

		ipcam_http_port = nf_ipcam_get_http_port(i);	
		ipcam_ip = nf_ipcam_get_ipaddr(i);
		if( ipcam_http_port == 0 || ipcam_ip == 0 || ipcam_ip == 0xffffffff)
			continue;

		//IPCAM_DBG(MINOR, "ch[%d] ipcam addr[%08x] http_port[%d]\n", i, ipcam_ip, ipcam_http_port);
							
		my_inet_ntoa(ipcam_ipaddr_str, sizeof(ipcam_ipaddr_str), ipcam_ip );	

		// for DNAT ( external )
		snprintf(cmd_buff, sizeof(cmd_buff), 
				"iptables -t nat -A PREROUTING -d %s  -p tcp --dport %d -j DNAT --to %s:%d", 
				nvr_ipaddr_str, 51001+i, ipcam_ipaddr_str, ipcam_http_port	);
		proxy_system(cmd_buff ,1,3);
		
		g_usleep(100000);				
	}

	set_running_state(DISCOVERY_STOPPED);
	nf_pnd_queue_push(ch, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);

	// for DNAT ( internal )
	snprintf(cmd_buff, sizeof(cmd_buff), 
				"iptables -t nat -A PREROUTING -d %s -j DNAT --to %s", 
				nvr_ipaddr_str, ipcam_ipaddr_str );
				
	proxy_system(cmd_buff ,1,3); 

	proxy_system("iptables -t nat -nL -v",1,3); 	

	{
		{
			GAsyncQueue *vloss_queue = get_vloss_queue();
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
			evt->port = ch;
			g_async_queue_push(vloss_queue, evt);
		}

		memset(&runtime[ch], 0x00, sizeof(mtable));
		memset(&discovery[ch], 0x00, sizeof(dtable));
		runtime[ch].sys.http_port = hport;
		runtime[ch].sys.use_ssl = use_ssl;
		memcpy(runtime[ch].sys.vendor, vendor, 64);
		//IPCAM_DBG(MINOR, "vloss state set %08x\n", vloss_status);
	}

	return 1;
}

/**
 * @brief Direct configure 모드를 해제한다.
 * @return 항상 1.
 */
gboolean nf_ipcam_direct_config_stop( )
{
	
	IPCAM_DBG(MAJOR, "start");
		
	// disable NAT
	proxy_system("iptables -t nat -nL -v",1,3); 	
	proxy_system("iptables -t nat -F",1,3); 
	proxy_system("echo 0 > /proc/sys/net/ipv4/ip_forward",1,3);
	proxy_system("/sbin/sysctl -w net.core.rmem_default=108544",1,3);
			
	set_running_state(DISCOVERY_RUNNING);
	return 1;
}

static pthread_mutex_t _direct_config_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _direct_config_command_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _direct_config_timer_sec = -1;
static pthread_t _direct_config_th;
static time_t _direct_config_start_time = 0;

static void _set_direct_config_timer(int sec)
{
	pthread_mutex_lock(&_direct_config_timer_mutex);
    _direct_config_timer_sec = sec;
    _direct_config_start_time = time(NULL);
	pthread_mutex_unlock(&_direct_config_timer_mutex);

    printf("[%s:%d] set sec[%d]\n", __func__, __LINE__, sec);
}

static int _get_direct_config_timer()
{
    return _direct_config_timer_sec;
}

static int _direct_config_start_all(int start_port)
{
	int i=0;
	char cmd_buff[1024];
	char nvr_ipaddr_str[128], ipcam_ipaddr_str[128];
	NF_NETIF_GET_INFO ret_info;

    //get nvr ipaddr
	memset( &ret_info, 0x00, sizeof(ret_info));	
	nf_netif_get_info( &ret_info);	
	if( ret_info.ipaddr == 0 || ret_info.ipaddr == 0xffffffff)
	{
        printf("[%s:%d] nvr_ipaddr get failed\n", __func__, __LINE__);
		return 0;
	}
	my_inet_ntoa(nvr_ipaddr_str, sizeof(nvr_ipaddr_str), htonl(ret_info.ipaddr) );

	proxy_system("echo 1 > /proc/sys/net/ipv4/ip_forward",1,3);
	proxy_system("iptables -t nat -F",1,3); 

	for(i=0;i<NUM_ACTIVE_CH;++i)
	{
		unsigned int ipcam_http_port = 0;
		unsigned int ipcam_ip = 0;
		char ipcam_ipaddr_str[128];

		ipcam_http_port = nf_ipcam_get_http_port(i);	
		ipcam_ip = nf_ipcam_get_ipaddr(i);
		if( ipcam_http_port == 0 || ipcam_ip == 0 || ipcam_ip == 0xffffffff){
			continue;
        }

		my_inet_ntoa(ipcam_ipaddr_str, sizeof(ipcam_ipaddr_str), ipcam_ip );	

		// for DNAT ( external )
		snprintf(cmd_buff, sizeof(cmd_buff), 
				"iptables -t nat -A PREROUTING -d %s  -p tcp --dport %d -j DNAT --to %s:%d", 
				nvr_ipaddr_str, start_port+i, ipcam_ipaddr_str, ipcam_http_port	);
        printf("[%s:%d] ch[%d] ipaddr[%s] cmd[%s]\n", __func__, __LINE__, i, ipcam_ipaddr_str, cmd_buff);
		proxy_system(cmd_buff ,1,3);
		
		g_usleep(100000);				
	}
    return 1;
}

static void _direct_config_stop_all()
{
	proxy_system("iptables -t nat -F",1,3); 
    proxy_system("echo 0 > /proc/sys/net/ipv4/ip_forward",1,3);
}

static int _nf_ipcam_direct_config_thread_func(void *arg)
{
    int rc;
    int t;
    time_t curr_time;

    printf("[%s:%d] time[%d]\n", __func__, __LINE__, _get_direct_config_timer());

    rc = _direct_config_start_all(50001);
    if(rc == 0){
        printf("[%s:%d] _direct_config_start_all failed\n", __func__, __LINE__);
        return -1;
    }


	pthread_mutex_lock(&_direct_config_timer_mutex);
    _direct_config_start_time = time(NULL);
	pthread_mutex_unlock(&_direct_config_timer_mutex);

    while(1)
    {
        pthread_mutex_lock(&_direct_config_timer_mutex);
        curr_time = time(NULL);
        if((t = _direct_config_timer_sec - (curr_time - _direct_config_start_time)) <= 0){
            //printf("[%s:%d] [%u] [%u] [%u]\n", __func__, __LINE__, _direct_config_start_time, curr_time, curr_time - _direct_config_start_time);
            _direct_config_timer_sec = 0;
            pthread_mutex_unlock(&_direct_config_timer_mutex);
            break;
        }

        pthread_mutex_unlock(&_direct_config_timer_mutex);
        sleep(1);
    }

    _direct_config_stop_all();
    _set_direct_config_timer(-1);

    return 0;
}

static int _direct_config_stop_req()
{
    int ret = 0;
	pthread_mutex_lock(&_direct_config_timer_mutex);
    printf("[%s:%d] current _direct_config_timer_sec[%d]\n", __func__, __LINE__, _direct_config_timer_sec);
    if(_direct_config_timer_sec > 0){
        _direct_config_timer_sec = 0;
        ret = 1;
    }
	pthread_mutex_unlock(&_direct_config_timer_mutex);

    return ret;
}

/**
 * @brief 전채 채널 카메라의 direct config 모드를 해제한다.
 * @return 1 - 성공, 0 - 실패.
 */
int nf_ipcam_direct_config_stop_all()
{
    if(nf_get_running_mode() == 1){
        printf("[%s:%d] error openmode\n", __func__, __LINE__);
        return 0;
    }

	pthread_mutex_lock(&_direct_config_command_mutex);
    _direct_config_stop_req();
	pthread_mutex_unlock(&_direct_config_command_mutex);

    return 1;
}

/**
 * @brief 전채 채널 카메라의 direct config 기능을 활성화 시킨다
 * @param[in] sec NAT 지속시간
 * @return 1 - 성공, 0 - 실패.
 */
int nf_ipcam_direct_config_start_all(int sec)
{
    int err_count = 5;
    int config_timer;
    int ret = 0;

    if(nf_get_running_mode() == 1){
        printf("[%s:%d] error openmode\n", __func__, __LINE__);
        return 0;
    }

    if(sec <= 0){
        printf("[%s:%d] argument error sec[%d]\n", __func__, __LINE__, sec);
        return 0;
    }

	pthread_mutex_lock(&_direct_config_command_mutex);
    while(1)
    {
        config_timer = _get_direct_config_timer();
        printf("[%s:%d] curr timer[%d] req timer[%d]\n", __func__, __LINE__, config_timer, sec);

        if(config_timer > 0){   
            pthread_mutex_lock(&_direct_config_timer_mutex);
            if(_get_direct_config_timer() > 0){
                _direct_config_timer_sec = sec;
                _direct_config_start_time = time(NULL);
                pthread_mutex_unlock(&_direct_config_timer_mutex);
            }else{
                pthread_mutex_unlock(&_direct_config_timer_mutex);
                continue;
            }
        }else if(config_timer == 0){
            if(err_count <= 0){
                _set_direct_config_timer(-1);
                sleep(1);
            }else{
                err_count--;
                sleep(1);
                printf("[%s:%d] thread close wait count[%d]\n", __func__, __LINE__, err_count);
            }
            continue;
        }else{
            _set_direct_config_timer(sec);
            g_thread_create(_nf_ipcam_direct_config_thread_func, NULL, FALSE, NULL);
        }
        break;
    }
	pthread_mutex_unlock(&_direct_config_command_mutex);

    return 1;
}


static int _get_ch_port(unsigned int ipaddr)
{
    int ch;
	mtable* runtime = NULL;
	runtime = get_runtime();

    char buf1[200], buf2[200];

    for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
    {
        if(runtime[ch].sys.ipaddr == ipaddr) return ch;
    }

    return -1;
}


static void _get_nat_urls(NFIPCamDirectConfigInfo *info)
{
    char cmd[256] = {0};
    char line[1024] = {0};
    FILE *fp = NULL;
    int fd = -1;
    int ret = 0;
    char *ptr;

    int ch;
	mtable* runtime = NULL;
	runtime = get_runtime();
	NF_NETIF_GET_INFO ret_info;

    char nvr_ip[20];
    char target_ip[20];
    int target_port;
    int sport;

	memset( &ret_info, 0x00, sizeof(ret_info));	
	nf_netif_get_info( &ret_info);	
	if( ret_info.ipaddr == 0 || ret_info.ipaddr == 0xffffffff)
	{
		printf("[%s:%d] nvr addr[%08x] \n", __func__, __LINE__, ret_info.ipaddr);
        goto ends_label;
	}

    snprintf(cmd, sizeof(cmd), "iptables -t nat -nL -v");

    fp = proxy_popen((const char *)cmd, "r", &fd);
    if(fp == NULL || fd < 0)
    {
        printf("%s proxy_popen fail(cmd:%s)\n", __func__, cmd);
        goto ends_label;
    }

    _ip_to_str(ret_info.ipaddr, nvr_ip);
    info->url_ch_mask = 0;

    while(fgets(line, sizeof(line), fp) != NULL)
    {
        if(strstr(line, "DNAT") != NULL){
            sscanf(line, "%*[^:]:%d%*[ \t]to:%[^:]:%d", &sport, target_ip, &target_port);
            ch = _get_ch_port(inet_addr(target_ip));
            if(ch >= 0)
            {
                info->url_ch_mask |= (1 << ch);
                snprintf(info->url[ch], sizeof(info->url[ch]), "%s://%s:%d/", 
                        runtime[ch].sys.use_ssl ? "https":"http",
                        nvr_ip,
                        sport);
            }
        }
    }

ends_label:

    if(fp != NULL || fd > 0)
    {
        proxy_pclose(fp, fd, 3);
    }

    //return ret;
}

int nf_ipcam_get_direct_config_status(NFIPCamDirectConfigInfo *info)
{
    int req_time;

    if(info == NULL) return -1;
    req_time = _get_direct_config_timer();

    if(req_time > 0){
        info->running_state = 1;
        _get_nat_urls(info);
    }else{
        info->running_state = 0;
        info->url_ch_mask = 0;
    }

    return 0;
}

int nf_ipcam_get_camera_connection_urls(NFIPCamCamConnURLs *urls)
{
    int rc;
    int ch;
    unsigned int ipcam_ip;
    int ipcam_http_port;
    char ip_buf[20];
	mtable *runtime = get_runtime();

    if(urls == NULL) return -1;

    urls->is_openmode = nf_get_running_mode();

    if(nf_get_running_mode())
    {
        urls->nat_required = 0;
        urls->url_ch_mask = 0;


        for(ch = 0; ch < NUM_ACTIVE_CH; ch++)
        {
            ipcam_http_port = nf_ipcam_get_dconf_port(ch);	
            ipcam_ip = nf_ipcam_get_ipaddr(ch);	
            if( ipcam_http_port == 0 || ipcam_ip == 0 || ipcam_ip == 0xffffffff)
                continue;
        
            urls->url_ch_mask |= (1 << ch);
            snprintf(urls->url[ch], sizeof(urls->url[ch]), "%s://%s:%d/", 
                    runtime[ch].sys.use_ssl ? "https":"http",
                    _ip_to_str(ntohl(ipcam_ip), ip_buf),
                    ipcam_http_port);
        }
    }
    else
    {
        NFIPCamDirectConfigInfo info;
        rc = nf_ipcam_get_direct_config_status(&info);
        urls->nat_required = !info.running_state;
        urls->url_ch_mask = info.url_ch_mask;
        memcpy(urls->url, info.url, sizeof(urls->url));
    }

    return 0;
}

/**
 * @brief Sysdb에서 PTZ관련 항목을 조회한다.
 * @param[in] ch 대상 채널.
 * @param[out] PtzProp 대상 Ptz정보 struct.
 * @return 0 - 성공, else 실패.
 *
 * 현재 PTZ 이동시에 PTZ속도 조회용도로 사용.
 */
int nf_ipcam_get_ptz_property(int ch, PtzData *PtzProp)
{
	PtzData ptzdata;

	// rtn 0 for success
	int rtn = DAL_get_ptz_data(&ptzdata, ch);

	memcpy(PtzProp, &ptzdata, sizeof(PtzData));

	return rtn;
}

int nf_ipcam_get_onvif_bookmark_presets(int ch, onvif_bookmark *info) 
{
	int i = 0;
	int rtn = 0;
	mtable* runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	//nf_ipcam_prepare routine
	{
		rtn = nf_onvif_ptz_get_support(ch);

		if(rtn == 0 && (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_PRESET))
		{
			rtn = nf_onvif_ptz_get_preset(ch);

			rtn = nf_ipcam_sync_onvif_preset(ch);
		}
	}

	memset(info, 0x00, sizeof(onvif_bookmark));

	if(rtn != 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	info->bookmark_cnt = runtime[ch].preset.preset_cnt;
	if(info->bookmark_cnt > 16) info->bookmark_cnt = 16;
	//printf("\e[95m >> [debug] bookmark cnt : %d \e[0m\n\n", info->bookmark_cnt); 

	for(i = 0; i < info->bookmark_cnt; i++)
	{
		info->bookmark_preset_number[i] = runtime[ch].preset.preset_number[i] + 1;
		if(info->bookmark_cnt > 16) info->bookmark_cnt = 16;
		//printf("\e[95m >> [debug] [%d] bookmark presset number: %d \e[0m\n\n", i, info->bookmark_preset_number[i]); 
	}

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief ONVIF 카메라의 ptz 지원 정보를 조회한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, else 실패.
 */
int nf_ipcam_prepare_onvif_ptz(int ch)
{
	int rtn = 0;
	mtable* runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), (-1));
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, (-1));
	g_return_val_if_fail(nf_ipcam_is_onvif_support(ch) == 1, (-1));

	{
		rtn = nf_onvif_ptz_get_support(ch);

		if(rtn == 0 && (runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_PRESET))
		{
			rtn = nf_onvif_ptz_get_preset(ch);

			rtn = nf_ipcam_sync_onvif_preset(ch);
		}
	}

	return rtn;
}

static void matching_bookmark_preset(int ch)
{
	int bookmark_max = 16;
	int bookmark_cnt = 0;
	int i = 0;

	mtable* runtime = get_runtime();

	for(i = 0; i < ONVIF_PRESET_MAX; i++)
	{
		if(strlen(runtime[ch].ptz_onvif.preset_token[i]) > 0)
		{
			//snprintf(runtime[ch].preset.preset_name[bookmark_cnt], 64, runtime[ch].ptz_onvif.preset_name[i]);
			snprintf(runtime[ch].preset.preset_token[bookmark_cnt], 64, runtime[ch].ptz_onvif.preset_token[i]);
			runtime[ch].preset.preset_number[bookmark_cnt] = runtime[ch].ptz_onvif.preset_num[i];

			bookmark_cnt ++;

			if(bookmark_cnt == bookmark_max)
				break;
		}
	}

	if(runtime[ch].ptz_onvif.current_preset_cnt > 16)
		runtime[ch].preset.preset_cnt = 16;
	else
		runtime[ch].preset.preset_cnt = runtime[ch].ptz_onvif.current_preset_cnt;
}

static void rearrange_preset(int ch)
{
	int i, j = 0;
	int temp_index = 0;
	char* ptr = NULL;
	char temp_num[5];

	char itx_preset_token[ONVIF_PRESET_MAX][64];
	char itx_preset_name[ONVIF_PRESET_MAX][64];
	int itx_preset_num = 0;

	char others_preset_token[ONVIF_PRESET_MAX][64];
	char others_preset_name[ONVIF_PRESET_MAX][64];
	int others_preset_num = 0;

	memset(itx_preset_token, 0x00, 64 * ONVIF_PRESET_MAX);
	memset(itx_preset_name, 0x00, 64 * ONVIF_PRESET_MAX);

	memset(others_preset_token, 0x00, 64 * ONVIF_PRESET_MAX);
	memset(others_preset_token, 0x00, 64 * ONVIF_PRESET_MAX);

	memset(temp_num, 0x00, 5);

	mtable* runtime = get_runtime();

	//parse itx_preset OR others_preset
	//for (i = 0; i < runtime[ch].ptz_onvif.supported_preset_cnt; i++)
	for (i = 0; i < ONVIF_PRESET_MAX; i++)
	{
		//ITX preset parsing routine	//test code
		if (strstr(runtime[ch].ptz_onvif.preset_name[i], "XB") != NULL)
		{
			ptr = strstr(runtime[ch].ptz_onvif.preset_name[i], "XB");
			ptr = ptr + 2;

			snprintf(temp_num, 4, ptr);

			temp_index = atoi(temp_num);

			snprintf(itx_preset_token[temp_index], 64, runtime[ch].ptz_onvif.preset_token[i]);
			snprintf(itx_preset_name[temp_index], 64, runtime[ch].ptz_onvif.preset_name[i]);

			itx_preset_num ++;// index +1;
		}
		else
		{
			snprintf(others_preset_token[others_preset_num], 64, runtime[ch].ptz_onvif.preset_token[i]);
			snprintf(others_preset_name[others_preset_num], 64, runtime[ch].ptz_onvif.preset_name[i]);
			others_preset_num ++;
		}
	}

	for(i = 0; i < ONVIF_PRESET_MAX; i++)
	{
		if(strlen(itx_preset_token[i]) > 0)
		{
			snprintf(runtime[ch].ptz_onvif.preset_token[i], 64, itx_preset_token[i]);
			snprintf(runtime[ch].ptz_onvif.preset_name[i], 64, itx_preset_name[i]);
			runtime[ch].ptz_onvif.preset_num[i] = i;
		}
		else
		{
			snprintf(runtime[ch].ptz_onvif.preset_token[i], 64, others_preset_token[j]);
			snprintf(runtime[ch].ptz_onvif.preset_name[i], 64, others_preset_name[j]);
			runtime[ch].ptz_onvif.preset_num[i] = i;
			j++;
		}
	}
#if 1
	//debug 
	for(i = 0; i < ONVIF_PRESET_MAX; i++)
	{
		if(strlen(runtime[ch].ptz_onvif.preset_token[i]) > 0)
		{
			//printf("\e[31m >> [sjlim87] =========================== \e[0m\n" );
			//printf("\e[31m >> [sjlim87] [%s][%d] token : %s \e[0m\n", __FUNCTION__, __LINE__, runtime[ch].ptz_onvif.preset_token[i]);
			//printf("\e[33m >> [sjlim87] [%s][%d] name : %s \e[0m\n", __FUNCTION__, __LINE__, runtime[ch].ptz_onvif.preset_name[i]);
			//printf("\e[95m >> [sjlim87] [%s][%d] preset_num : %d \e[0m\n", __FUNCTION__, __LINE__, runtime[ch].ptz_onvif.preset_num[i]);
		}
	}
#endif
}



/**
 * @brief ONVIF PTZ카메라의 preset을 sysdb preset 수와 맞게 삭제한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, else 실패.
 *
 * @todo ONVIF PTZ preset 관리에 문제 있음.
 * Sysdb의 preset과 카메라에 설정되어 있는 preset간 매칭할 수 있는 방법이 없다.
 * (sysdb는 숫자로 관리하며, ONVIF는 사용자 설정 불가능한 token으로 관리한다.)
 * 따라서 현재는 단순히 sysdb index = ONVIF index 식으로 관리하나,
 * sysdb index가 뒤섞일 때 버그 발생.
 */
int nf_ipcam_sync_onvif_preset(int ch)
{
	int rtn = 0, i, j;
	mtable* runtime = get_runtime();
	char endpoint[256];
	ipcam_onvif_auth_info_t auth_info;


	memset(endpoint, 0x00, 256);
	memset(&auth_info, 0x00, sizeof(ipcam_onvif_auth_info_t));
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	IPCAM_DBG(MINOR, "sysdb preset cnt : %d onvif preset cnt : %d\n", runtime[ch].preset.preset_cnt, runtime[ch].ptz_onvif.preset_cnt);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	//parsing ITX preset name
	rearrange_preset(ch);

	//matching bookmark preset (ITX + others)
	matching_bookmark_preset(ch);

	return rtn;
}

/*
 * @brief 카메라의 정보를 조회한다. 
 *        Fisheye Camera 조회를 위해 추가 함
 * @param ch        채널번호
 *        profile   카메라 정보를 넣을 구조체 포인터
 *        user_data 사용자 데이터 // 미사용
 *        error     에러 // 미사용
 */
int nf_ipcam_get_camera_profile(gint ch, NFIPCamTypeProfile *profile, gpointer user_data, GError **error)
{
	mtable* runtime = NULL;

	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail(profile != NULL,      IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch >= 0,              IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime != NULL,      IPCAM_SETUP_RTN_FAILED);

	profile->cam_type = runtime[ch].cam_type;

	IPCAM_DBG(MAJOR, "CH(%d) CAM(%d)\n", ch, profile->cam_type);

	return IPCAM_SETUP_RTN_DONE;
}

/*
 * @brief Fisheye 카메라의 지원 기능 및 옵션을 조회 한다.
 * @param ch        채널번호
 *        profile   Fisheye 카메라 지원 기능을 넣을 구조체 포인터
 *        user_data 사용자 데이터 // 미사용
 *        error     에러 // 미사용
 */
int nf_ipcam_get_fisheye_profile(gint ch, NFIPCamFisheyeProfile *profile, gpointer user_data, GError **error)
{
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail(profile != NULL,      IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch >= 0,              IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime != NULL,      IPCAM_SETUP_RTN_FAILED);

	profile->fisheye_supported = runtime[ch].fisheye.fisheye_supported;

	// MountType
	if(profile->fisheye_supported & NF_IPCAM_FISHEYE_MOUNT)
	{
		int idx = 0;

		profile->mount_cnt = 0;

		for(idx = 0; idx < NF_IPCAM_MOUNT_NR; idx++)
		{
			if((1<<idx) & runtime[ch].fisheye.mount.support)
			{
				NFIPCamOption_onvif *get_tuple = NULL;

				get_tuple = get_cam_model_option_fisheye(NF_IPCAM_FISHEYE_MOUNT, (1 << idx));
				if(get_tuple == NULL)
				{
					memset(profile, 0x00, sizeof(NFIPCamFisheyeProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}

				memcpy(&(profile->mount), get_tuple, sizeof(NFIPCamOption_onvif));
				if((1 << idx) & runtime[ch].fisheye.mount.value)
				{
					profile->mount[profile->mount_cnt].selected = 1;
				}
				profile->mount_cnt++;
			}
		}
	}

	// DewarpMode
	if(profile->fisheye_supported & NF_IPCAM_FISHEYE_DEWARP)
	{
		int idx = 0;

		profile->dewarp_cnt = 0;

		for(idx = 0; idx < NF_IPCAM_DEWARP_NR; idx++)
		{
			if((1<<idx) & runtime[ch].fisheye.dewarp.support)
			{
				NFIPCamOption_onvif *get_tuple = NULL;

				get_tuple = get_cam_model_option_fisheye(NF_IPCAM_FISHEYE_DEWARP, (1 << idx));
				if(get_tuple == NULL)
				{
					memset(profile, 0x00, sizeof(NFIPCamFisheyeProfile));
					return IPCAM_SETUP_RTN_FAILED;
				}

				memcpy(&(profile->dewarp), get_tuple, sizeof(NFIPCamOption_onvif));
				if((1 << idx) & runtime[ch].fisheye.dewarp.value)
				{
					profile->dewarp[profile->dewarp_cnt].selected = 1;
				}
				profile->dewarp_cnt++;
			}
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

/*
 * @brief Fisheye 카메라의 ePTZ Layout을 조회 한다.
 * @param ch        채널번호
 *        layout    카메라 스트림 화면의 PTZ 영역 분할 정보
 *        user_data 사용자 데이터 // 미사용
 *        error     에러 // 미사용
 */
int nf_ipcam_ePTZ_get_layout(gint ch, NFIPCamEPTZLayout *layout, gpointer user_data, GError **error)
{
	mtable* runtime = NULL;
	
	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail(layout != NULL,       IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(ch >= 0,              IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime != NULL,      IPCAM_SETUP_RTN_FAILED);

	if(runtime[ch].fisheye.fisheye_supported & NF_IPCAM_FISHEYE_EPTZ)
	{
		IPCAM_DBG(MAJOR, "fisheye_supported EPTZ CH(%d)\n", ch);

		if(runtime[ch].funcs[NF_IPCAM_TYPE_GET_EPTZ_LAYOUT] != NULL)
		{
			int (*get_layout_func)(NFIPCamEPTZLayout*, int);

			IPCAM_DBG(MAJOR, "Have get_layout_func CH(%d)\n", ch);

			get_layout_func = runtime[ch].funcs[NF_IPCAM_TYPE_GET_EPTZ_LAYOUT];

			get_layout_func(ch, layout);
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

extern int itx_installation_mode_on(int cam_id, system_t *sys, char* usr, char* pwd);
extern int itx_installation_mode_off(int cam_id, system_t *sys, char* usr, char* pwd);

static unsigned int _temp_state[AVAILABLE_MAX_CH];
static system_t _temp_sys[AVAILABLE_MAX_CH];
static char _temp_usr[AVAILABLE_MAX_CH][64];
static char _temp_pwd[AVAILABLE_MAX_CH][64];

/**
 * @brief A2카메라에 한해 설치 모드를 실행한다.
 * @return 항상 IPCAM_SETUP_RTN_DONE.
 *
 * Network status 팝업에서 리모컨을 통해 설치 모드에 진입할 수 있다.
 *
 * @todo TI변경점 카메라 및 신규 모델에 대한 작업 필요.
 */
int nf_ipcam_install_mode_on(void)
{
	gint i = 0;
	mtable *runtime = NULL;

	memset(_temp_state, 0x00, (sizeof(unsigned int)*AVAILABLE_MAX_CH));
	memset(_temp_sys, 0x00, (sizeof(system_t)*AVAILABLE_MAX_CH));
	memset(_temp_usr, 0x00, AVAILABLE_MAX_CH*64);
	memset(_temp_pwd, 0x00, AVAILABLE_MAX_CH*64);

	runtime = get_runtime();

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		_temp_state[i] = runtime[i].state;
		memcpy(&_temp_sys[i], &runtime[i].sys, sizeof(system_t));
		memcpy(&_temp_usr[i], runtime[i].username, 64);
		memcpy(&_temp_pwd[i], runtime[i].password, 64);
	}

	nf_ipcam_stop();
	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((_temp_state[i] & MGMT_STATE_CONFIGURED) == 0) {continue;}
		if ((_temp_sys[i].model_code != NF_IPCAM_MODEL_AMB_A2)&&
			(_temp_sys[i].model_code != NF_IPCAM_MODEL_AMB_D1)&&
			(_temp_sys[i].model_code != NF_IPCAM_MODEL_TI_368)
		) {continue;}

		itx_installation_mode_on(i, &_temp_sys[i], _temp_usr[i], _temp_pwd[i]);
	}

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라 설치 모드를 종료한다.
 * @return 항상 IPCAM_SETUP_RTN_DONE.
 */
int nf_ipcam_install_done(void)
{
	gint i = 0;

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((_temp_state[i] & MGMT_STATE_CONFIGURED) == 0) {continue;}
		if ((_temp_sys[i].model_code != NF_IPCAM_MODEL_AMB_A2)&&
			(_temp_sys[i].model_code != NF_IPCAM_MODEL_AMB_D1)
		) {continue;}
		itx_installation_mode_off(i, &_temp_sys[i], _temp_usr[i], _temp_pwd[i]);
	}
	sleep(2);
	nf_ipcam_start();

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 해상도, fps, quality별 bitrate를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] table Bitrate 정보 struct.
 * @return 1 - 성공, 0 - 실패.
 *
 * 녹화용량 계산시 사용한다.
 */
int nf_ipcam_get_bps_table(gint ch, NFIPCamBpsTable *table)
{
	int rtn = 0;
	int i, j, k;
	int size[2];	// max no of record stream
	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version
	int bitrate_table_id = 0;
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MINOR, "ch(%d)\n", ch);

	g_return_val_if_fail( table != NULL , 0);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, 0);
	g_return_val_if_fail( runtime != NULL , 0);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, 0);

	//
	float _BITRATE_COVERAGE_RATE_[][5] = 
	{
		/* base(model code : NF_IPCAM_MODEL_TI_368) : 0*/	
		{ 0.75, 0.63, 0.49, 0.47, 0.45 },
		/* 5M resolution, 4000 bitrate : 1 */
		{ 1.0, 0.73, 0.68, 0.62, 0.56 },
		/* onvif : 2*/	
		{ 0.7, 0.5, 0.4, 0.35, 0.3},
		/* A2 : 3*/	
		{ 0.5, 0.25, 0.2, 0.16, 0.125}
	};

	sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);
	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		bitrate_table_id = 2;
	}
	else if ((runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2) || (runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1))
	{
		bitrate_table_id = 3;
	}
	else
	{
#if defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO) \
		|| defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO) || defined(_IPX_1648P4) \
		|| defined(_IPX_1648L4)
		if(strstr(runtime[ch].sys.stdver, "-5007") != NULL && i == 0 && type == ITX_CAM_SDK_TYPE_HS )
		{
			bitrate_table_id = 1;
		}
		else
		{
			bitrate_table_id = 0;
		}
#else
		bitrate_table_id = 0;
#endif
	}

	memset(table, 0x00, sizeof(NFIPCamBpsTable));

	for(i = 0; i < 2; i++)
	{
		uint64_t res = runtime[ch].video.resolution.resolution[i];
		for(j = 0; j < NF_IPCAM_RES_MAX; j++)
		{
			if(shift_res_table[j] == res) break;
		}
		size[i] = j;

		for(j = 0; j < NF_IPCAM_QUALITY_MAX; j++)
		{
			if(runtime[ch].video.quality[i][j] == 0)
			{
				table->org_video_bps[size[i]][0][j] = (3200 + j * 1200)/(1 + i * 3);	// fixme default ti quality
			}
			else
			{
				if ((runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2) || (runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1))
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.bitrate[i].value;
				}
				else
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j];
				}
				// get max fps, build 30 fps first
#if 0
				guint capable, current;
				if(nf_ipcam_get_fps(ch, i, &capable, &current, 0) != 1)
				{
					continue;
				}
				if((capable & NF_IPCAM_FPS_300) || (capable & NF_IPCAM_FPS_250))
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j];
				}
				else if((capable & NF_IPCAM_FPS_150) || (capable & NF_IPCAM_FPS_120))
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j] / _BITRATE_COVERAGE_RATE_[bitrate_table_id][0];
				}
				else if((capable & NF_IPCAM_FPS_70) || (capable & NF_IPCAM_FPS_60))
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j] / _BITRATE_COVERAGE_RATE_[bitrate_table_id][1];
				}
				else if(capable & NF_IPCAM_FPS_30)
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j] / _BITRATE_COVERAGE_RATE_[bitrate_table_id][2];
				}
				else if(capable & NF_IPCAM_FPS_20)
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j] / _BITRATE_COVERAGE_RATE_[bitrate_table_id][3];
				}
				else if(capable & NF_IPCAM_FPS_10)
				{
					table->org_video_bps[size[i]][0][j] = runtime[ch].video.quality[i][j] / _BITRATE_COVERAGE_RATE_[bitrate_table_id][4]; //fix me
				}
#endif
			}

			for(k = 0; k < NF_IPCAM_FPS_MAX; k++)
			{
				switch(1 << k)
				{
					case NF_IPCAM_FPS_250:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j];
						break;

					case NF_IPCAM_FPS_150:
					case NF_IPCAM_FPS_120:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j] * _BITRATE_COVERAGE_RATE_[bitrate_table_id][0];
						break;

					case NF_IPCAM_FPS_70:
					case NF_IPCAM_FPS_60:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j] * _BITRATE_COVERAGE_RATE_[bitrate_table_id][1];
						break;

					case NF_IPCAM_FPS_30:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j] * _BITRATE_COVERAGE_RATE_[bitrate_table_id][2];
						break;

					case NF_IPCAM_FPS_20:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j] * _BITRATE_COVERAGE_RATE_[bitrate_table_id][3];
						break;

					case NF_IPCAM_FPS_10:
						table->org_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][0][j] * _BITRATE_COVERAGE_RATE_[bitrate_table_id][4];	// fixme
						break;

					default:
						break;
				}
				if(nf_ipcam_is_onvif_support(ch) != 1)
				{
					if(table->org_video_bps[size[i]][k][j] < 512)
					{
						table->org_video_bps[size[i]][k][j] = 512;
					}
				}
				table->sum_video_bps[size[i]][k][j] = table->org_video_bps[size[i]][k][j];
			}
		}
	}

	for(i = 0; i < 2; i++)
	{
		table->bitctrl[0] = runtime[ch].video.bitctrl[0];
		table->bitctrl[1] = runtime[ch].video.bitctrl[1];
	}

	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < NF_IPCAM_QUALITY_MAX; j++)
		{
			for(k = 0; k < NF_IPCAM_FPS_MAX; k++)
			{
				if (i == 0) // if 1st stream :  + 2nd stream bitrate
					table->sum_video_bps[size[i]][k][j] += table->org_video_bps[size[1]][k][j];
				else // if 2nd stream :  + 1st stream bitrate
					table->sum_video_bps[size[i]][k][j] += table->org_video_bps[size[0]][k][j];
			}
		}
	}
	table->audio_bps = 8 * 8;	// u-law : 8kbyte

	return 1;
}

/** @var _fw_upgrade_lock
 *  @brief 카메라 펌웨어 업그레이드 동기화 관련 lock.
 */
/** @var _fw_state
 *  @brief 카메라별 펌웨어 업그레이드 상태 변수.
 */
/** @var g_file_stream
 *  @brief 펌웨어 파일 내용(ITX카메라 전용).
 */
static GStaticMutex _fw_upgrade_lock = G_STATIC_MUTEX_INIT;
static NFIPCamUpgradeState _fw_state[NUM_IPX_CHANNEL];
static char *g_file_stream = NULL;
static int _fwup_unlink_cnt[NUM_IPX_CHANNEL] = {0,};

/**
 * @brief 펌웨어 파일명(Ti368) 또는 header정보(hisilicon)를 통해 업그레이드 가능한 카메라를 찾는다.
 * @param[in] file_path 펌웨어 파일 전체경로.
 * @param[out] out_ch_mask 업그레이드 가능한 채널 bitmask.
 * @return TRUE - 성공, FALSE - 실패.
 *
 * @todo ONVIF 카메라 지원.
 */
gboolean nf_ipcam_fw_capability_chk(gchar* file_path, guint* out_ch_mask)
{
	char* rbuf;
	g_message("[%s] file path : %s", __FUNCTION__, file_path);
	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version

	*out_ch_mask = 0;
	int i = 0;

	mtable *runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, FALSE);

	char file_nm[255] = {0,};
	char model_cd[255];
	char fw_ver[255];
	char extension[255];
	char *token = NULL;
	char token_ver[255] ={0, };
	char *fw_token = NULL;
	char *swver = NULL;

	char full_fw_version[64] = {0,}; // parsing fw header
	char ver_parse[4][16] = {0,}; // fw file header data

	int swver_num = 0;
	int file_validation = 0;
	int pcode[4] = {0,0,0,0};
	int pcode_iter[4] = {0,0,0,0};
	int is_hisilicon = 0;

	token = file_path;
	while(strstr(token, "/") !=  NULL)
	{
		token = strstr(token, "/") + 1;
	}

	if(strcmp(token, "(null)") == 0)
	{
		*out_ch_mask = 0;
		return TRUE;
	}

	// 1. API 조회. Ti, hisilicon 분류
	nf_fw_get_version_info(file_path, full_fw_version, 1, 0); // get product code


	if (strncmp(full_fw_version, "", 64) != 0)
	{
		strncpy(token_ver, full_fw_version, 64);
		fw_token = strtok_r(token_ver, ".", &rbuf);

		strcpy(ver_parse[0], fw_token);

		i++;
		while(fw_token = strtok_r(NULL, ".", &rbuf))
		{
			strcpy(ver_parse[i], fw_token);
			i++;
		}
	}

	if(strncmp(ver_parse[0], "", 64) != 0) // product code check
	{
		is_hisilicon = 1;
	}

	snprintf(file_nm, 255, "%s", token);
	file_validation = _is_valid_fw_name(file_nm, &pcode[0], is_hisilicon);
	if (file_validation < 0)
	{
		*out_ch_mask = 0;
		return TRUE;
	}

	if(is_hisilicon == 0) // Ti 
	{
		if (pcode[2] < 1000 || pcode[2] >= 2000)
		{
			*out_ch_mask = 0;
			return TRUE;
		}
		g_message("[%s] Ti camera FW", __FUNCTION__);
	}
	else //hisilicon
	{
		if (ver_parse[2] == NULL) // check version
		{
			*out_ch_mask = 0;
			return TRUE; 
		}
		if (ver_parse[3] == NULL) // check vendor
		{
			*out_ch_mask = 0;
			return TRUE; 
		}
		g_message("[%s] Hisilicon camera FW", __FUNCTION__);
	}

	for (i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		// 2. 공통 점검사항 체크

		/* skips no connected channels */
		if (!(runtime[i].state & MGMT_STATE_READY))
		{
			g_message("[%s] ch:%d not ready to step ", __FUNCTION__, i);
			continue;
		}
		if (get_pnd_status(i) != PND_TYPE_VIDEO_START)
		{
			g_message("[%s] ch:%d not ready to pnd type PND_TYPE_VIDEO_START ", __FUNCTION__, i);
			continue;
		}

		/* skips onvif camera */
		if (nf_ipcam_is_onvif_support(i) == 1)
		{
			g_message("[%s] ch:%d cam is onvif camera ", __FUNCTION__, i);
			continue;
		}

		/* skips xiongmai camera */
		if (strcmp(runtime[i].sys.vendor, "H264") == 0)
		{
			g_message("[%s] ch:%d cam is xiongmai camera ", __FUNCTION__, i);
			continue;
		}

		/* skips non TI 368 camera */
		if (runtime[i].sys.model_code != NF_IPCAM_MODEL_TI_368)
		{
			g_message("[%s] ch:%d model code not TI368", __FUNCTION__, i);
			continue;
		}

		/* skips S1 camera */
		if (strncmp(runtime[i].sys.model, "VBR", 3) == 0 ||
				strncmp(runtime[i].sys.model, "VDR", 3) == 0)
		{
			g_message("[%s] ch:%d cam is s1 camera ", __FUNCTION__, i);
			continue;
		}
		sscanf(runtime[i].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);

		// 3. Ti. Hisilicon 분류 및 각각 점검
		if(is_hisilicon == 1) // Hisilicon
		{
			swver_num = _find_version_number(runtime[i].sys.swver, &pcode_iter[0]);
			if(type != 12 || (type == 12 && minor < 2))
			{
				g_message("[%s] ch:%d hisilicon cam version error: type:%d, minor:%d  ", __FUNCTION__, i, type, minor);
				continue;
			}
			if (atoi(ver_parse[3]) != pcode_iter[3]) // compare vendor
			{
				g_message("[%s] ch:%d fw vendor miss match(file:%s|cam:%d)  ", __FUNCTION__, i, ver_parse[3], pcode_iter[3]);
				continue;
			}

			if (strncmp(runtime[i].sys.swver, ver_parse[0], 5) != 0)  // compare product code 
			{
				g_message("[%s] ch:%d product code miss match  ", __FUNCTION__, i);
				continue;
			}
			else
			{
				if (strstr(runtime[i].sys.swver, ver_parse[2]) != NULL) // compare version
				{
					g_message("[%s] ch:%d fw version same(%s == %s)  ", __FUNCTION__, i, ver_parse[2], runtime[i].sys.swver2);
					continue;
				}
				else
				{
					*out_ch_mask |= (1 << i);
				}
			}
		}
		else // Ti 
		{
			if(type == 12)
			{
				g_message("[%s] ch:%d ti cam, type is 12  ", __FUNCTION__, i);
				continue;
			}
			swver_num = _find_version_number(runtime[i].sys.swver, &pcode_iter[0]);
			if (pcode_iter[2] < 1000 || pcode_iter[2] >= 2000)
			{
				g_message("[%s] ch:%d ti cam, version:%d   ", __FUNCTION__, i, pcode_iter[2]);
				continue;
			}
			*out_ch_mask |= (1 << i);
		}
	}

	return TRUE;
}

/**
 * @brief 카메라 펌웨어를 업로드한다.
 * @param ch_mask 업그레이드할 채널 bitmask.
 * @param file_path 펌웨어 파일 경로.
 * @param is_nonblocking 1 - nonblock(쓰레드 실행), 0 - block.
 * @return TRUE - 성공, FALSE - 실패.
 *
 * @todo ONVIF 카메라 지원.
 */
gboolean nf_ipcam_fw_upgrade(guint ch_mask, gchar* file_path, gboolean is_nonblocking)
{
	g_message("[%s] ch : %02x file : %s", __FUNCTION__, ch_mask, file_path);
	if(!g_static_mutex_trylock(&_fw_upgrade_lock))
	{
		IPCAM_DBG(ERROR, "ipcam upgrade already running...\n");
		return FALSE;
	}

	memset(_fw_state, 0x00, sizeof(NFIPCamUpgradeState) * NUM_IPX_CHANNEL);
	memset(_fwup_unlink_cnt, 0x00, sizeof(int) * NUM_IPX_CHANNEL);
	_ipcam_upgrade_canceled = 0;

	int i, rtn, new_ch_mask, file_len;
	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		if(ch_mask & (1 << i))
		{
			_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_PRE, 0, NF_IPCAM_FW_ERR_OK, 0);
		}
		else
		{
			_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_NOT_TARGET, 0, NF_IPCAM_FW_ERR_OK, 0);
		}
	}
	if (_ipcam_upgrade_canceled)
	{
		g_warning("[%s] upgrade canceled", __FUNCTION__);
		g_static_mutex_unlock(&_fw_upgrade_lock);
		return FALSE;
	}

	//char *file_stream = NULL;
	FILE *fp = NULL;
	if(!(fp = fopen(file_path, "rb")))
	{
		IPCAM_DBG(ERROR, "invalid file path : %s", file_path);
		g_static_mutex_unlock(&_fw_upgrade_lock);
		return FALSE;
	}

	fseek(fp, 0L, SEEK_END);
	file_len = ftell(fp);
#if 0
	if(file_len > 35 * 1024 * 1024) // TODO: FW Size Check
	{
		IPCAM_DBG(ERROR, "too large file size : %s %d", file_path, file_len);
		for(i = 0; i < NUM_IPX_CHANNEL; i++)
		{
			_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_PRE, 1, NF_IPCAM_FW_ERR_BINARY_SIZE, 100);
		}
		fclose(fp);
		g_static_mutex_unlock(&_fw_upgrade_lock);
		return FALSE;
	}
#endif

	fseek(fp, 0L, SEEK_SET);
	g_file_stream = (char *)malloc(file_len);

	fread(g_file_stream, sizeof(char), file_len, fp);
	fclose(fp);

	new_ch_mask = 0;

	if (!nf_get_running_mode())
	{
		IPCAM_DBG(MAJOR, "fw upgrade start, discovery state stopping...(%d)\n", get_running_state());
		set_running_state(DISCOVERY_STOPPED);
		IPCAM_DBG(MAJOR, "fw upgrade start, discovery state stopped(%d)\n", get_running_state());
	}
	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		if(ch_mask & (1 << i))
		{
			if(nf_ipcam_is_onvif_support(i) == 1)
			{
				_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_CHK_VER, 1, NF_IPCAM_FW_ERR_ONVIF, 100);
				continue;
			}
#if 0 
			{
				int chk_ver_mask = 0;
				nf_ipcam_fw_capability_chk(file_path, &chk_ver_mask);
				if(!(chk_ver_mask & (1 << i)))
				{
					_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_CHK_VER, 1, NF_IPCAM_FW_ERR_READY, 100);

					continue;
				}
			}
#endif

			if(!is_nonblocking)
			{
				_NFIPCamUpThreadParam *oneParam;
				oneParam = g_malloc0(sizeof(_NFIPCamUpThreadParam));
				oneParam->new_ch_mask = i;
				strcpy(oneParam->file_path, file_path);
				oneParam->file_stream = g_file_stream;
				oneParam->file_len = file_len;
				_nf_ipcam_upgrade_thread_func_one(oneParam);
			}
			else
			{
				new_ch_mask |= 1 << i;
			}
		}
	}

#if 0
	//nf_ipcam_stop();
	if(!nf_get_running_mode())
	{
		set_running_state(DISCOVERY_STOPPED);
	}
	else
	{
		nf_openmode_set_state(OPENMODE_STATE_INIT);
	}
#endif


	if(is_nonblocking)
	{
		_NFIPCamUpThreadParam *param;
		param = g_malloc0(sizeof(_NFIPCamUpThreadParam));
		param->new_ch_mask = new_ch_mask;
		strcpy(param->file_path, file_path);
		param->file_stream = g_file_stream;
		param->file_len = file_len;
		g_thread_create(_nf_ipcam_upgrade_thread_func, param, FALSE, NULL);
	}
	else
	{
		if(g_file_stream != NULL)
		{
			free(g_file_stream);
			g_file_stream = NULL;
		}

		//nf_ipcam_start();

		g_static_mutex_unlock(&_fw_upgrade_lock);

		//g_message("[%s] ch : %04x upgrade complete(blocking)!!", __FUNCTION__, ch_mask);
	}

	return TRUE;
}

/** @def MAX_NO_UPGRADE_IPCAM
 *  @brief Nonblocking모드에서 동시에 업그레이드할 카메라 수.
 */
#define MAX_NO_UPGRADE_IPCAM	16

/**
 * @brief 카메라 업그레이드 개별 쓰레드를 관리하는 쓰레드 함수.
 * @param param 카메라 업그레이드 정보가 담긴 struct.
 */
static void _nf_ipcam_upgrade_thread_func(_NFIPCamUpThreadParam *param)
{
	IPCAM_DBG(MAJOR, "ch : %04x file_path : %s main thread start!!\n", param->new_ch_mask, param->file_path);
	GThread *pThread[NUM_IPX_CHANNEL];
	int i = 0, j = 0, rtn, cur_up_cam = 0;

	/* old routine - start all once */
#if 0
	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		pThread[i] = NULL;
		if(param->new_ch_mask & (1 << i))
		{
			_NFIPCamUpThreadParam *oneParam;
			oneParam = g_malloc0(sizeof(_NFIPCamUpThreadParam));
			oneParam->new_ch_mask = i;
			strcpy(oneParam->file_path, param->file_path);
			oneParam->file_stream = param->file_stream;
			oneParam->file_len = param->file_len;
			pThread[i] = g_thread_create(_nf_ipcam_upgrade_thread_func_one, oneParam, TRUE, NULL);
			if(pThread[i] == NULL)
			{
				g_warning("[%s] ch : %02d thread create fail!!", __FUNCTION__, i);
				_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_CHK_VER, 1, NF_IPCAM_FW_ERR_GENERAL, 100);
				g_free(oneParam);
				continue;
			}
			usleep(1000 * 1000);
		}
	}
	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		if(pThread[i])
		{
			g_thread_join(pThread[i]);
			g_message("[%s] ch : %02d thread waiting end!!", __FUNCTION__, i);
		}
	}
#endif
	while(1)
	{
		// thread factory - always run MAX_NO_UPGRADE_IPCAM threads
		while(1)
		{
			pThread[i] = NULL;
			if(cur_up_cam < MAX_NO_UPGRADE_IPCAM && i < NUM_IPX_CHANNEL && (param->new_ch_mask & (1 << i)))
			{
				_NFIPCamUpThreadParam *oneParam;
				oneParam = g_malloc0(sizeof(_NFIPCamUpThreadParam));
				oneParam->new_ch_mask = i;
				strcpy(oneParam->file_path, param->file_path);
				oneParam->file_stream = param->file_stream;
				oneParam->file_len = param->file_len;
				pThread[i] = g_thread_create(_nf_ipcam_upgrade_thread_func_one, oneParam, TRUE, NULL);
				if(pThread[i] == NULL)
				{
					IPCAM_DBG(ERROR, "ch : %02d thread create fail!!\n", i);
					_nf_ipcam_set_upgrade_state(i, NF_IPCAM_FW_CHK_VER, 1, NF_IPCAM_FW_ERR_READY, 99);
					g_free(oneParam);
					//continue;
				}
				else
				{
					cur_up_cam++;
				}

				usleep(1000 * 1000);
			}
			i++;

			if(i >= NUM_IPX_CHANNEL || cur_up_cam >= MAX_NO_UPGRADE_IPCAM)
			{
				//g_message("[%s] thread factory mini loop end, i : %d cur : %d", __FUNCTION__, i, cur_up_cam);
				break;
			}
		}
		if(i >= NUM_IPX_CHANNEL)
		{
			//g_message("[%s] thread factory end!!", __FUNCTION__);
			break;
		}

		// thread closer
		if(cur_up_cam >= MAX_NO_UPGRADE_IPCAM)
		{
			j = 0;
			while(1)
			{
				if(pThread[j])
				{
					NFIPCamUpgradeState state;
					nf_ipcam_get_upgrade_state(j, &state);
					if(state.state == NF_IPCAM_FW_UP_COMPLETE || state.is_error == 1)
					{
						g_thread_join(pThread[j]);
						//g_message("[%s] ch : %02d thread waiting end, start next ch!!", __FUNCTION__, j);
						pThread[j] = NULL;
						cur_up_cam--;
						break;
					}
					else
					{
						usleep(3 * 1000 * 1000);
					}
				}

				j++;
				if(j >= i) j = 0;
			}
		}
	}

	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		if(pThread[i])
		{
			g_thread_join(pThread[i]);
			//g_message("[%s] ch : %02d thread waiting end!!", __FUNCTION__, i);
			pThread[i] = NULL;
		}
	}

	if(param->file_stream != NULL)
	{
		free(param->file_stream);
	}
	g_free(param);

	//nf_ipcam_start();

	if(nf_get_running_mode())
	{
		for(i = 0; i < NUM_IPX_CHANNEL; i++)
		{
			nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, i);
		}
	}

	g_static_mutex_unlock(&_fw_upgrade_lock);

	if (!nf_get_running_mode())
	{
		IPCAM_DBG(MAJOR, "fw upgrade done, discovery state starting...(%d)\n", get_running_state());
		set_running_state(DISCOVERY_RUNNING);
		IPCAM_DBG(MAJOR, "fw upgrade done, discovery state started(%d)\n", get_running_state());
	}

	IPCAM_DBG(MAJOR, "upgrade complete(nonblocking)!!\n");
}

/**
 * @brief 카메라 한 채널 업그레이드 함수(쓰레드).
 * @param param 카메라 업그레이드 정보가 담긴 struct.
 */
static void _nf_ipcam_upgrade_thread_func_one(_NFIPCamUpThreadParam *param)
{
	IPCAM_DBG(MAJOR, "ch : %02d file_path : %s sub thread start!!\n", param->new_ch_mask, param->file_path);
	int i, ch, rtn;
	mtable *runtime = get_runtime();
	ipcam_upgrade_ctx fwup_ctx;
	ch = param->new_ch_mask;
	NFIPCamUpgradeState state;

	if(!nf_get_running_mode())
	{
		nf_ipcam_pause_ch(ch);
	}

	if (_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_CHK_VER, 0, NF_IPCAM_FW_ERR_OK, 5))
	{
		g_warning("[%s] upgrade canceled\n", __FUNCTION__);
		g_free(param);
		return;
	}

	if (itx_cam_check_fwup_status(ch, &fwup_ctx) == IPCAM_SETUP_RTN_DONE)
	{
		_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_SET_FWMODE, 0, NF_IPCAM_FW_ERR_OK, 5);
		g_message("[%s] ch %d new upgrade protocol supported\n", __FUNCTION__, ch);
		/* 
		 * ITX network camera upgrade protocol designed at 2015.01 
		 *  details are described at following page
		 *   - http://222.112.8.34:8090/display/SWG/FW+Update+API
		 */
		memset(&fwup_ctx, 0x00, sizeof(fwup_ctx));
		fwup_ctx.ch = ch;
		fwup_ctx.fw_len = param->file_len;
		fwup_ctx.fw_name = param->file_path;
		fwup_ctx.fw_data_stream = param->file_stream;
		rtn = ipcam_upgrade_start(&fwup_ctx);
	}
	else
	{
		/*
		 * ITX upgrade protocol doesn't be supported
		 *  use old version
		 */
		g_message("[%s] ch %d new upgrade protocol not supported\n", __FUNCTION__, ch);
		rtn = cam_upload_fw(ch, param->file_path, param->file_stream, param->file_len);
	}

	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		nf_ipcam_get_upgrade_state(ch, &state);
		_nf_ipcam_set_upgrade_state(ch, state.state, 1, state.error_no, 99);
		g_message("[%s] ch %d fw upload fail [state:%d|error_no:%d] ", __FUNCTION__, ch, state.state, state.error_no);
		if(!nf_get_running_mode())
		{
			nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
		}
	}
	else
	{
		g_message("[%s] ch %d fw upload success ", __FUNCTION__, ch);
		//IPCAM_DBG(MAJOR, "ch %d fw upload success\n ", ch);

		if (fwup_ctx.status == IPCAM_FWUP_STATUS_MIN)
		{
			for(i = 0; i < 5; i++)
			{
				usleep(14 * 1000 * 1000);	// 70 sec
				if (_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 95 + i))
				{
					break;
				}
			}

			if(!nf_get_running_mode())
			{
				sleep(2);
				nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
			}
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_UP_COMPLETE, 0, NF_IPCAM_FW_ERR_OK, 100);
		}
		else
		{
			//_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 90);
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_UP_COMPLETE, 0, NF_IPCAM_FW_ERR_OK, 100);
			sleep(2);
			nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
			memset(&_fw_state[ch], 0x00, sizeof(NFIPCamUpgradeState));
		}
	}
	g_free(param);
}

/**
 * @brief 현재 카메라 업그레이드 상태를 조회한다.
 * @param[in] ch 채널 번호.
 * @param[out] state 카메라 업그레이드 상태 struct.
 */
void nf_ipcam_get_upgrade_state(gint ch, NFIPCamUpgradeState *state)
{
	g_return_if_fail(state != NULL);
	g_return_if_fail(ch < NUM_IPX_CHANNEL);

	memcpy(state, &_fw_state[ch], sizeof(NFIPCamUpgradeState));
}

/**
 * @brief 현재 카메라 업그레이드 상태를 설정한다.
 * @param ch 채널 번호.
 * @param state 업그레이드 상태. @see NF_IPCAM_FW_UPGRADE_STATE
 * @param error 에러 여부(0, 1).
 * @param err_no 에러 코드. @see NF_IPCAM_FW_UPGRADE_ERR
 * @param progress 진행 상태(0 ~ 100).
 */
int _nf_ipcam_set_upgrade_state(int ch, int state, int error, int err_no, int progress)
{
	int layer,linked;
	ipcam_disc_port_link_state(ch,&layer,&linked);
	if (!linked)
	{
		_fwup_unlink_cnt[ch]++;
	}
	if (_fwup_unlink_cnt[ch] > 2)
	{
		_fw_state[ch].state = state;
		_fw_state[ch].is_error = 1;
		_fw_state[ch].error_no = NF_IPCAM_FW_ERR_DISCONNECT;
		_fw_state[ch].cur_progress = progress;
		return 1;
	}
	if (_ipcam_upgrade_canceled)
	{
		_fw_state[ch].state = state;
		_fw_state[ch].is_error = 1;
		_fw_state[ch].error_no = NF_IPCAM_FW_ERR_GENERAL;
		_fw_state[ch].cur_progress = progress;
		return 1;
	}
	else
	{
		_fw_state[ch].state = state;
		_fw_state[ch].is_error = error;
		_fw_state[ch].error_no = err_no;
		_fw_state[ch].cur_progress = progress;
		return 0;
	}

	return 0;
}

/**
 * @brief 카메라 업그레이드 진행중 여부를 조회한다.
 * @return TRUE - 카메라 업그레이드 진행 중.
 */
extern int nf_ipcam_is_cam_upgrade(void)
{
	if(!g_static_mutex_trylock(&_fw_upgrade_lock))
	{
		return TRUE;
	}
	g_static_mutex_unlock(&_fw_upgrade_lock);
	return FALSE;
}
static NFIPCamDCIrisCalState dciris_cal_state;

int nf_ipcam_update_dc_iris_cal_state(void)
{
	return dciris_cal_state.status;
}

static void _nf_ipcam_dc_iris_cal_thread_func(void *temp_state)
{
	NFIPCamDCIrisCalState *data = (NFIPCamDCIrisCalState *)temp_state;

	int timer = 0, rtn = -1, ret = 0;
	int ch;

	ch = data->ch;	
//	data->iris_type = "d";

	for(timer  = 0; timer < 300 ; timer++)
	{
		usleep(2*1000*1000);

		ret = _common_get_dc_iris_cal_status(ch);
	
		if(ret == -1){
			data->status = RTN_ERROR;
			//IPCAM_DBG(MINOR, "DC IRIS CALIBRATION ERROR!\n");
			return;	
		}
		else if(ret == 0){
			data->status = RTN_NORMAL;
		}
		else if(ret == 1){
			data->status = RTN_RUNNING;
		}
		else{
			data->status = RTN_END;
			//IPCAM_DBG(MINOR, "DC IRIS CALIBRATION SUCCESS!\n");
			break;
		}
		timer ++;
	}
	
	if(data->status == RTN_NORMAL || data->status == RTN_RUNNING){
		data->status = RTN_TIMEOUT;
		//IPCAM_DBG(MINOR, "DC IRIS CALIBRATION TIMEOUT!\n");
	}
	return;
}

int nf_ipcam_get_dc_iris_cal_start_func(int ch)
{
	int rtn = 1;
	GThread *pThread;

	pThread = NULL;

	memset(&dciris_cal_state, 0x00, sizeof(dciris_cal_state));
	dciris_cal_state.status = RTN_NORMAL;
	dciris_cal_state.ch = ch;

	pThread = g_thread_create(_nf_ipcam_dc_iris_cal_thread_func, (void *)&dciris_cal_state, FALSE, NULL);
	
	if(pThread == NULL)
	{
		rtn = 0;
	}

	return rtn;
}

extern int vhub_get_hub_macaddr(unsigned char *p_hub_macaddr);
gboolean nf_ipcam_hub_get_macaddr(unsigned char *p_hub_macaddr)
{
	int ret;

	ret = vhub_get_hub_macaddr(p_hub_macaddr);
	if (ret)
	{
		/*
		printf("[%s][%d] macaddr : [%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__, __LINE__,
				p_hub_macaddr[0], p_hub_macaddr[1], p_hub_macaddr[2],
				p_hub_macaddr[3], p_hub_macaddr[4], p_hub_macaddr[5]);
		*/
		return 1;
	}

	return 0;
}


extern int vhub_get_hub_fwver(unsigned char *p_hub_fwver);
gboolean nf_ipcam_hub_get_fwver(unsigned char *p_hub_fwver)
{
	int ret;

	ret = vhub_get_hub_fwver(p_hub_fwver);
	if (ret)
	{
		//printf("[%s][%d] fwver : [%s]\n", __FUNCTION__, __LINE__, p_hub_fwver);
		return 1;
	}

	return 0;
}

extern gint nf_ipcam_is_support_va(gint ch)
{
	gint rtn = 0;
	mtable *runtime = get_runtime();

	if ((runtime[ch].state & MGMT_STATE_CONFIGURED)==0)
	{
		return rtn;
	}

	if (runtime[ch].func & NF_IPCAM_FUNC_VA)
	{
		rtn = 1;
	}

	return rtn;
}
extern gint nf_ipcam_list_channels_va(gchar* buf)
{
	gint i=0;
	mtable *runtime = get_runtime();

	if (buf == NULL)
	{
		return 0;
	}

	memset(buf, 0x00, AVAILABLE_MAX_CH);
	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED)==0)
		{
			buf[i] = '0';
			continue;
		}
		if (runtime[i].func & NF_IPCAM_FUNC_VA)
		{
			buf[i] = '1';
		}
		else
		{
			buf[i] = '0';
		}
	}

	return 1;
}

extern int nf_ipcam_is_vendor_zig(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p==NULL) { return 0; }
	if (strcmp(p, "IPCAM_ZIG") == 0)
	{
		return 1;
	}

	return 0;
}

extern int nf_ipcam_is_vendor_vicon(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p==NULL) { return 0; }
	if (strcmp(p, "VICON") == 0)
	{
		return 1;
	}

	return 0;
}

extern int nf_ipcam_support_static_ip_onvif_cam(void)
{
	return support_static_ip_onvif_cam;
}

extern void nf_ipcam_write_maps(void)
{
	FILE *fp = NULL;
	int pid = 0;
	int sz = 0;
	char fname[256];
	const char* wfname = "/NFDVR/maps-data/maps.txt";
	const char* wfname_backup = "/NFDVR/maps-data/maps_backup.txt";
	char cmd[256];
	char mvcmd[256];
	time_t timer;
	struct tm *t;
	int y,m,d,h,mi,s;

	timer = time(NULL);
	t = localtime(&timer);

	y = t->tm_year+1900;
	m = t->tm_mon+1;
	d = t->tm_mday;
	h = t->tm_hour;
	mi = t->tm_min;
	s = t->tm_sec;

	proxy_system("mkdir /NFDVR/maps-data", 1, 3);

	fp = fopen(wfname,"r");
	if (fp != NULL)
	{
		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fclose(fp);
		if (sz > 1024*512)
		{
			snprintf(mvcmd, 256, "mv %s %s", wfname, wfname_backup);
			proxy_system(mvcmd, 1, 3);
		}
	}
	pid = getpid();

	memset(cmd, 0x00, 256);
	snprintf(cmd, 256, "echo ========================== Maps %d%02d%02d_%02d%02d%02d ========================== >> %s", y,m,d,h,mi,s,wfname);
	proxy_system(cmd, 1, 3);

	memset(fname, 0x00, 256);
	snprintf(fname, 256, "/proc/%d/maps", pid);
	snprintf(cmd, 256, "cat %s >> %s", fname, wfname);
	proxy_system(cmd, 1, 3);

	memset(cmd, 0x00, 256);
	snprintf(cmd, 256, "echo ========================================================================= >> %s", wfname);
	proxy_system(cmd, 1, 3);

	memset(cmd, 0x00, 256);
	snprintf(cmd, 256, "echo >> %s", wfname);
	proxy_system(cmd, 1, 3);

	memset(cmd, 0x00, 256);
	snprintf(cmd, 256, "echo >> %s", wfname);
	proxy_system(cmd, 1, 3);

	proxy_system("ln -s /NFDVR/maps-data /tmp/webra-info/maps-data", 1, 3);
}
extern void nf_ipcam_boost_priority(void)
{
	FILE *fp = NULL;
	int pid = 0;
	int sz = 0;
	char fname[256];
	char cmd[256];
	char mvcmd[256];
	time_t timer;
	struct tm *t;
	int y,m,d,h,mi,s;

	pid = getpid();
	snprintf(cmd, 256, "renice -20 -p %d\n", pid);
	proxy_system(cmd);
}

void nf_ipcam_get_sysproc_uuid(char* rbuf)
{
	const char* uuid_fname = "/proc/sys/kernel/random/uuid";
	FILE *fp;

	fp = fopen(uuid_fname,"r");
	if (fp == NULL)
	{
		return;
	}

	memset(rbuf, 0x00, sizeof(rbuf));
	fread(rbuf, 36, 1, fp);
	fclose(fp);
}

#if CAM_ZIG_MODE
extern int cam_set_time_info(int);
extern int itx_cam_get_mf_info(int, NFIPCamMFInfo*);

gboolean nf_ipcam_set_time_info(gint ch)
{
	mtable *runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail(ch >= 0, 0);
	g_return_val_if_fail(ch < AVAILABLE_MAX_CH, 0);
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0) { return 0; }
	if (runtime[ch].sys.model_code >= NF_IPCAM_MODEL_ONVIF) { return 0; }

	cam_set_time_info(ch);

	return 1;
}

gboolean nf_ipcam_get_mf_info(gint ch, NFIPCamMFInfo* info)
{
	mtable *runtime = NULL;
	runtime = get_runtime();

	g_return_val_if_fail(ch >= 0, 0);
	g_return_val_if_fail(ch < AVAILABLE_MAX_CH, 0);
	g_return_val_if_fail(info != NULL, 0);
	memset(info, 0x00, sizeof(NFIPCamMFInfo));

	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0) { return 0; }
	if (runtime[ch].sys.model_code >= NF_IPCAM_MODEL_ONVIF) { return 0; }

	itx_cam_get_mf_info(ch, info);

	return 1;
}

gboolean nf_ipcam_scan_start(void)
{
	nf_ipcam_start();
	return 1;
}

extern int cam_set_dhcpon(int);
gboolean nf_ipcam_works_done(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	if (nf_get_running_mode() == 1)
	{
		IPCAM_DBG(WARN, "OPEN mode!\n");
		return 0;
	}

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].state & MGMT_STATE_CONFIGURED == 0) { continue; }
		cam_set_dhcpon(i);
		//nf_ipcam_factory_default(i, NULL,NULL,NULL);
	}
	nf_ipcam_stop();
	// FIXME. factory default, pnd stop
}


void nf_ipcam_zig_zoom_wide(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_ZOOM] == NULL) { continue; }
		cam_zig_set_zoom(runtime[i].ptz.zoom.min, i);
	}
}

void nf_ipcam_zig_zoom_tele(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_ZOOM] == NULL) { continue; }
		cam_zig_set_zoom(runtime[i].ptz.zoom.max, i);
	}
}

void nf_ipcam_zig_focus_near(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_FOCUS] == NULL) { continue; }
		cam_zig_set_focus(runtime[i].ptz.focus.min, i);
	}
}

void nf_ipcam_zig_focus_far(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_FOCUS] == NULL) { continue; }
		cam_zig_set_focus(runtime[i].ptz.focus.max, i);
	}
}

void nf_ipcam_zig_iris_open(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_IRIS] == NULL) { continue; }
		cam_zig_set_iris(runtime[i].ptz.iris.min, i);
	}
}

void nf_ipcam_zig_iris_close(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_IRIS] == NULL) { continue; }
		cam_zig_set_iris(runtime[i].ptz.iris.max, i);
	}
}

void nf_ipcam_zig_origin(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_ORIGIN] == NULL) { continue; }
		cam_zig_set_origin(i);
	}
}

void nf_ipcam_zig_onepush(void)
{
	gint i = 0;
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if ((runtime[i].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if (runtime[i].funcs[NF_IPCAM_TYPE_SET_ONESHOT] == NULL) { continue; }
		cam_zig_set_onepush(i);
	}
}

static unsigned int ipx_resol_cm2icodec(unsigned int cam_manager_resol)
{
	switch(cam_manager_resol)
	{
		case NF_IPCAM_RES_352x240:		return RES_NTSC_CIF;
		case NF_IPCAM_RES_352x288:		return RES_PAL_CIF;
		case NF_IPCAM_RES_640x352:		return RES_640x352;
		case NF_IPCAM_RES_640x480:		return RES_640x480;
		case NF_IPCAM_RES_704x480:		return RES_NTSC_4CIFP;
		case NF_IPCAM_RES_704x576:		return RES_PAL_4CIFP;
		case NF_IPCAM_RES_720x480:		return RES_720x480;
		case NF_IPCAM_RES_720x576:		return RES_720x576;
		case NF_IPCAM_RES_1280x720I:	return RES_1280x720I;
		case NF_IPCAM_RES_1280x720:		return RES_1280x720;
		case NF_IPCAM_RES_1024x768:		return RES_1024x768;
		case NF_IPCAM_RES_1280x1024:	return RES_1280x1024;
		case NF_IPCAM_RES_1920x1080I:	return RES_1920x1080I;
		case NF_IPCAM_RES_1920x1080:	return RES_1920x1080;
		case NF_IPCAM_RES_640x360:		return RES_640x360;
		case NF_IPCAM_RES_640x400:		return RES_640x400;
		case NF_IPCAM_RES_800x450:		return RES_800x450;
		case NF_IPCAM_RES_1440x900:		return RES_1440x900;
		case NF_IPCAM_RES_800x600:		return RES_800x600;
		case NF_IPCAM_RES_1600x1200:	return RES_1600x1200;
		case NF_IPCAM_RES_320x240:
		case NF_IPCAM_RES_320x180:
		default:
			break;
	}

	return RES_NTSC_NONE;
}

static void _zig_timer_cb(gint _ch)
{
	gint ch = _ch;
	NMFMrtpPipeChannel info;
	unsigned int vloss_status = 0;
	mtable *runtime = NULL;

	runtime = get_runtime();

	memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

	info.ch_num = ch;
	info.model_code = runtime[ch].sys.model_code;
	info.username = &runtime[ch].username[0];
	info.password = &runtime[ch].password[0];
	info.video_cnt = runtime[ch].video.stream_cnt;
	info.video[0].resolution = ipx_resol_cm2icodec(runtime[ch].video.resolution.resolution[NF_IPCAM_STREAM_1ST]);
	info.video[0].ip_addr = runtime[ch].sys.ipaddr;
	info.video[0].rtsp_port = runtime[ch].sys.rtsp_port[0];
	info.video[0].rtsp_addr = runtime[ch].sys.rtsp_url[0];
	if (info.video_cnt > 1)
	{
		info.video[1].resolution = ipx_resol_cm2icodec(runtime[ch].video.resolution.resolution[NF_IPCAM_STREAM_2ND]);
		info.video[1].ip_addr = runtime[ch].sys.ipaddr;
		info.video[1].rtsp_port = runtime[ch].sys.rtsp_port[1];
		info.video[1].rtsp_addr = runtime[ch].sys.rtsp_url[1];
	}
	if (info.video_cnt > 2)
	{
		info.video[2].resolution = ipx_resol_cm2icodec(runtime[ch].video.resolution.resolution[2]);
		info.video[2].ip_addr = runtime[ch].sys.ipaddr;
		info.video[2].rtsp_port = runtime[ch].sys.rtsp_port[2];
		info.video[2].rtsp_addr = runtime[ch].sys.rtsp_url[2];
	}

	if (runtime[ch].audio.audio_tx)
	{
		info.audio_cnt = 1;
		info.audio.resolution = 0;
		info.audio.ip_addr = runtime[ch].sys.ipaddr;
		info.audio.rtsp_port = runtime[ch].sys.rtsp_port[0];
		info.audio.rtsp_addr = runtime[ch].sys.rtsp_url[0];
	}

	nmf_mrtp_pipe_set_dev_mac(_h_mrtp_pipe, ch, &runtime[ch].sys.macaddr[0]);
	nmf_mrtp_pipe_open_ch(_h_mrtp_pipe, &info);
	vloss_status = nf_notify_get_param0("vloss");
	vloss_status &= (~(1<<ch));
#if MAKE_NOTIFY_FIRE
	nf_notify_fire_params("vloss", vloss_status, 0, 0, 0);
#endif
	printf("%s [%s] vloss state set %08x\n", CAM_LOG_DOMAIN, __FUNCTION__, vloss_status);
	return TRUE;
}

void nf_ipcam_zig_left(gint ch, gint spd)
{
	int p_ch = 0;
	int x_pos = 0;
	int result = 0;
	unsigned int vloss_status = 0;
	mtable *runtime = NULL;

	runtime = get_runtime();

	if (runtime[ch].comp_init == 0)
	{
		printf("[%s] No init to setup comp CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (runtime[ch].comp_x == 0)
	{
		printf("[%s] Current X pos is '0' CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (spd <= 0)
	{
		printf("[%s] speed %d\n", __FUNCTION__, spd);
		return;
	}

	x_pos = runtime[ch].comp_x - spd;
	if (x_pos < 0)
	{
		x_pos = 0;
	}

	{
		nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, ch);
		vloss_status = nf_notify_get_param0("vloss");
		vloss_status |= (1<<ch);
		nf_notify_fire_params("vloss", vloss_status, 0,0,0);
		printf("%s [%s] vloss state set %08x\n", CAM_LOG_DOMAIN, __FUNCTION__, vloss_status);
	}

	result = itx_cam_set_compx(ch, x_pos);
	if (result == IPCAM_SETUP_RTN_DONE)
	{
		runtime[ch].comp_x = x_pos;
	}

	_zig_timer_cb(ch);
}

void nf_ipcam_zig_right(gint ch, gint spd)
{
	int p_ch = 0;
	int x_pos = 0;
	int result = 0;
	unsigned int vloss_status = 0;
	mtable *runtime = NULL;

	runtime = get_runtime();

	if (runtime[ch].comp_init == 0)
	{
		printf("[%s] No init to setup comp CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (runtime[ch].comp_x == 6)
	{
		printf("[%s] Current X pos is '6' CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (spd <= 0)
	{
		printf("[%s] speed %d\n", __FUNCTION__, spd);
		return;
	}

	x_pos = runtime[ch].comp_x + spd;
	if (x_pos > 6)
	{
		x_pos = 6;
	}

	{
		nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, ch);
		vloss_status = nf_notify_get_param0("vloss");
		vloss_status |= (1<<ch);
		nf_notify_fire_params("vloss", vloss_status, 0,0,0);
		printf("%s [%s] vloss state set %08x\n", CAM_LOG_DOMAIN, __FUNCTION__, vloss_status);
	}
	result = itx_cam_set_compx(ch, x_pos);
	if (result == IPCAM_SETUP_RTN_DONE)
	{
		runtime[ch].comp_x = x_pos;
	}

	_zig_timer_cb(ch);
}

void nf_ipcam_zig_up(gint ch)
{
	int y_pos = 0;
	int result = 0;
	mtable *runtime = NULL;

	runtime = get_runtime();

	if (runtime[ch].comp_init == 0)
	{
		printf("[%s] No init to setup comp CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (runtime[ch].comp_y == 0)
	{
		printf("[%s] Current Y pos is '0' CH-%02d\n", __FUNCTION__ , ch);
		return;
	}

	result = itx_cam_set_compy(ch, runtime[ch].comp_y - 1);
	if (result == IPCAM_SETUP_RTN_DONE)
	{
		runtime[ch].comp_y--;
	}
}

void nf_ipcam_zig_down(gint ch)
{
	int y_pos = 0;
	int result = 0;
	mtable *runtime = NULL;

	runtime = get_runtime();

	if (runtime[ch].comp_init == 0)
	{
		printf("[%s] No init to setup comp CH-%02d\n", __FUNCTION__ , ch);
		return;
	}
	if (runtime[ch].comp_y == 7)
	{
		printf("[%s] Current Y pos is '7' CH-%02d\n", __FUNCTION__ , ch);
		return;
	}

	result = itx_cam_set_compy(ch, runtime[ch].comp_y + 1);
	if (result == IPCAM_SETUP_RTN_DONE)
	{
		runtime[ch].comp_y++;
	}
}



#endif  // CAM_ZIG_MODE


static int _is_valid_fw_name(const char* filename, int* out_pcode, int is_hisilicon)
{
	const char *prefix_368 = "mega-web-";
	const char *prefix_385_368ptz = "web-";
	char *prefix = NULL;
	char *v = filename;
	char *s1,*s2,*s3,*s4;
	char *e1,*e2,*e3,*e4;
	char b1[16] = {0,};
	char b2[16] = {0,};
	char b3[16] = {0,};
	char b4[16] = {0,};
	int n1,n2,n3,n4;

	int rtn = (-1);

	/* integrity check */
	if (!v)
	{
		goto skip_rest_check;
	}

	if(is_hisilicon == 0) //ti
	{
		if (strstr(v, prefix_368) == NULL)
		{
			goto skip_rest_check;
		}
		else
		{
			prefix = prefix_368;
		}

		/* exists 4 dots */
		s1 = v + strlen(prefix);

		s2 = strstr(s1, ".");
		if (!s2)
		{
			goto skip_rest_check;
		}
		e1 = s2++;

		s3 = strstr(s2, ".");
		if (!s3)
		{
			goto skip_rest_check;
		}
		e2 = s3++;

		s4 = strstr(s3, ".");
		if (!s4)
		{
			goto skip_rest_check;
		}
		e3 = s4++;

		e4 = strstr(s4, ".");
		if (!e4)
		{
			goto skip_rest_check;
		}

		strncpy(b1, s1, (e1-s1));
		strncpy(b2, s2, (e2-s2));
		strncpy(b3, s3, (e3-s3));
		strncpy(b4, s4, (e4-s4));

		out_pcode[0] = atoi(b1);
		out_pcode[1] = atoi(b2);
		out_pcode[2] = atoi(b3);
		out_pcode[3] = atoi(b4);
	}

	else //hisilicon
	{
		if (strstr(v, prefix_368) != NULL)
		{
			prefix = prefix_368;
		}
		else if (strstr(v, prefix_385_368ptz) != NULL)
		{
			prefix = prefix_385_368ptz;
		}
		else
		{
			prefix = "";
		}
	}

	rtn = 1;

skip_rest_check:
	return rtn;
}

static int _find_version_number(const char* version_str, int* out_pcode)
{
	char *v = version_str;
	char *s1,*s2,*s3,*s4;
	char *e1,*e2,*e3,*e4;
	char b1[16] = {0,};
	char b2[16] = {0,};
	char b3[16] = {0,};
	char b4[16] = {0,};
	int n1,n2,n3,n4;

	int rtn = (-1);

	/* integrity check */
	if (!v)
	{
		goto skip_rest_process;
	}
	//if (strlen(v) > 16 || strlen(v) < 14)
	//{
	//	goto skip_rest_process;
	//}
	if (v[5] != '.')
	{
		goto skip_rest_process;
	}

	/* exists 4 dots */
	s1 = v;

	s2 = strstr(s1, ".");
	if (!s2)
	{
		goto skip_rest_process;
	}
	e1 = s2++;

	s3 = strstr(s2, ".");
	if (!s3)
	{
		goto skip_rest_process;
	}
	e2 = s3++;

	s4 = strstr(s3, ".");
	if (!s4)
	{
		goto skip_rest_process;
	}
	e3 = s4++;

	e4 = v + strlen(v) - 1;

	strncpy(b1, s1, (e1-s1));
	strncpy(b2, s2, (e2-s2));
	strncpy(b3, s3, (e3-s3));
	strncpy(b4, s4, ((e4+1)-s4));

	out_pcode[0] = atoi(b1);
	out_pcode[1] = atoi(b2);
	out_pcode[2] = atoi(b3);
	out_pcode[3] = atoi(b4);

	rtn = 1;

skip_rest_process:
	return rtn;
}

static int view_ipcam_option(NFIPCamOption_onvif *option)
{
	printf("Category:[0x%010llx] Dependent:[0x%010llx] Enable:[0x%010llx] Visible:[0x%010llx] Val:[0x%05x] Sel:[%d] Name:[%s] \n",
			option->category, option->dependent_category,
			option->enable_category, option->visible_category, option->value, option->selected, option->caption);
}


static unsigned char _pmotion_map(unsigned char k)
{
	unsigned char rtn = 0;
	if (k>0xa0) { rtn=10; }
	else if (k>0x90) { rtn=9; }
	else if (k>0x80) { rtn=8; }
	else if (k>0x70) { rtn=7; }
	else if (k>0x60) { rtn=6; }
	else if (k>0x50) { rtn=5; }
	else if (k>0x40) { rtn=4; }
	else if (k>0x30) { rtn=3; }
	else if (k>0x20) { rtn=2; }
	else if (k>0x10) { rtn=1; }
	else { rtn=0; }

	return rtn;
}

extern int nf_ipcam_general_motion_cb_func(DispmuxMotionCbParam *minfo, char* stream_buf)
{
	int i,j;
	int ch = minfo->ch;
	int width = minfo->width;
	int height = minfo->height;
	mtable *runtime = get_runtime();


	if (runtime[ch].state & MGMT_STATE_CONFIGURED == 0)
	{
		return 0;
	}
	switch(runtime[ch].sys.model_code)
	{
		case NF_IPCAM_MODEL_ONVIF:
		case NF_IPCAM_MODEL_ONVIF_L1:
			break;
		default:
			return;
			break;
	}

	if(nf_ipcam_is_vendor("CBC") == 1)
	{
		printf("[%s] : CBC NVR motion exception\n", __func__);
		return 0;
	}

	if (runtime[ch].motion.method == 0)
	{
		runtime[ch].motion.method = MAM_GENERAL;
		runtime[ch].motion.sensitivity.min = 1;
		runtime[ch].motion.sensitivity.max = 10;
		runtime[ch].motion.block_width = width;
		runtime[ch].motion.block_height = height;
		runtime[ch].motion.min_block = 1;
		runtime[ch].motion.num_block = width*height;
		runtime[ch].motion.min_block = 1;
	}

	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) && runtime[ch].motion.stream_snap == NULL)
	{
		runtime[ch].motion.stream_snap = (unsigned char*) malloc((size_t)(width*height));
	}

	if(runtime[ch].motion.stream_snap != NULL)
	{
		for (i=0; i<height; i++)
		{
			for (j=0; j<width; j++)
			{
				runtime[ch].motion.stream_snap[(i*width)+j] = _pmotion_map(*((unsigned char*)stream_buf+(i*width)+j));
			}
		}
	}
	//printf("jykim\n");
	//for (i=0; i<height; i++)
	//{
	//	for (j=0; j<width; j++)
	//	{
	//		printf("%02x ", runtime[ch].motion.stream_snap[(i*width)+j]);
	//	}
	//	printf("\n");
	//}

	//memcpy(runtime[ch].motion.stream_snap, stream_buf, runtime[ch].motion.num_block);

#if 0
	NF_ONVIF_DBG(ERROR, "CH(%d) Width(%d) Height(%d) method(%d)\n", minfo->ch, minfo->width, minfo->height, runtime[ch].motion.method);
	for (i=0; i<minfo->height; i++)
	{
		for (j=0; j<minfo->width; j++)
		{
			printf("%02x ", *((unsigned char*)(stream_buf+(i*minfo->width)+j)));
		}
		printf("\n");
	}
#endif
}

gboolean nf_eventlog_put_ipcam_msg(const char *msg, int ch)
{
	if(msg == NULL) { return; }

	GTimeVal tval;
	char log_buf[128];

	gettimeofday(&tval, NULL);
	snprintf(log_buf, 128, "[IPCAM] %s : CH(%d)", msg, ch);
	return nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_STRING, log_buf);
}                                                                                                  

int nf_ipcam_set_camera_license_key(int ch, NFIPCamLicenseKeyInfo *p_info)
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	if (ch >= NUM_IPX_CHANNEL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime[ch].state & MGMT_STATE_CONFIGURED == 0) { return IPCAM_SETUP_RTN_FAILED; }

	int rtn;
	const char *path = "/cgi-bin/action.fcgi";
	char query_buf[512] = {0 ,};
	const char *query = "api=set_setup.system.license&license_key=%s&created=%d";


	const unsigned int buffer_size = 2048;
	char *buffer = (char*)malloc(buffer_size);

	icm_http http;
	icm_http_ch_init(&http, ch);

	memset(buffer, 0x00, buffer_size);
	memset(query_buf, 0x00, 512);

	snprintf(query_buf, 512, query, p_info->key, p_info->acquired_date);
	//printf("\e[33m [q_buf]:%s \e[0m\n", query_buf);

	rtn = icm_http_get_request(&http, path, query_buf, buffer, buffer_size -1, NULL);

	if(rtn != ICM_RTN_OK)
	{
		free(buffer);
		return IPCAM_SETUP_RTN_FAILED;
	}

	//debug
	//printf("\e[31m [%s][%d] buffer : %s \e[0m\n", __FUNCTION__, __LINE__, buffer);

	free(buffer);

	return IPCAM_SETUP_RTN_DONE;
}

static int _nf_ipcam_get_parsed_license(char* buffer, NFIPCamLicenseKeyList *p_list)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	char *s_ptr = NULL;
	char *e_ptr = NULL;
	char tmp[64] = {0, };
	int license_count = 0;
	int i = 0;

	const char l_cnt[16] = "license_cnt=";
	const char *str_name = "name%d=";
	const char *str_key = "key%d=";
	const char *str_created_date = "created%d="; 
	const char *str_param1 = "param%d_1=";
	const char *str_param2 = "param%d_2=";

	char parse_str[16] = {0, };

	if(!buffer){ 
		return rtn ;
	}

	s_ptr = strstr(buffer, l_cnt);
	if(!s_ptr)
		return rtn;

	s_ptr = s_ptr + strlen(l_cnt);
	e_ptr = strstr(s_ptr, "\r\n");
	if(!e_ptr)
		return rtn;

	memcpy(tmp, s_ptr, (size_t)(e_ptr - s_ptr));
	license_count = atoi(tmp);	
	p_list->count = license_count;

	for(i = 0; i < license_count; i++)
	{
		s_ptr = NULL;
		e_ptr = NULL;
		memset(parse_str, 0x00, 16);
		snprintf(parse_str, 16, str_key, i);
	
		s_ptr = buffer;

		s_ptr = strstr(s_ptr, parse_str);
		if(!s_ptr) { return rtn; }
		s_ptr = s_ptr + strlen(parse_str);
		e_ptr = strstr(s_ptr, "\r\n");
		if(!e_ptr) { return rtn; }

		memcpy(p_list->key_data[i].key, s_ptr, (size_t)(e_ptr - s_ptr)); 

		memset(parse_str, 0x00, 16);
		snprintf(parse_str, 16, str_created_date, i);

		s_ptr = buffer;
		s_ptr = strstr(s_ptr, parse_str);
		if(!s_ptr) { return rtn; }
		s_ptr = s_ptr + strlen(parse_str);
			
		e_ptr = strstr(s_ptr, "\r\n");
		if(!e_ptr) { 
			e_ptr = strstr(s_ptr, "\r\n");
			if(!e_ptr) { 
			return rtn;
			}
		}

		memset(tmp, 0x00, 64);
		memcpy(tmp, s_ptr, (size_t)(e_ptr - s_ptr));
		p_list->key_data[i].acquired_date = atoi(tmp);

		//name
		memset(parse_str, 0x00, 16);
		snprintf(parse_str, 16, str_name, i);

		s_ptr = buffer;
		s_ptr = strstr(s_ptr, parse_str);
		if(!s_ptr) { return rtn; }
		s_ptr = s_ptr + strlen(parse_str);

		e_ptr = strstr(s_ptr, "&");
		if(!e_ptr) { 
			e_ptr = strstr(s_ptr, "\r\n");
			if(!e_ptr) { 
			return rtn;
			}
		}
		memset(tmp, 0x00, 64);
		memcpy(tmp, s_ptr, (size_t)(e_ptr - s_ptr));
		memcpy(p_list->key_data[i].name,  s_ptr, (size_t)(e_ptr - s_ptr));

		//param1
		memset(parse_str, 0x00, 16);
		snprintf(parse_str, 16, str_param1, i);

		s_ptr = buffer;
		s_ptr = strstr(s_ptr, parse_str);
		if(!s_ptr) { return rtn; }
		s_ptr = s_ptr + strlen(parse_str);

		e_ptr = strstr(s_ptr, "&");
		if(!e_ptr) { 
			e_ptr = strstr(s_ptr, "\r\n");
			if(!e_ptr) { 
			return rtn;
			}
		}
		memset(tmp, 0x00, 64);
		memcpy(tmp, s_ptr, (size_t)(e_ptr - s_ptr));
		p_list->key_data[i].param1 = (unsigned int)strtol(tmp, NULL, 16);

		//param2
		memset(parse_str, 0x00, 16);
		snprintf(parse_str, 16, str_param2, i);

		s_ptr = buffer;
		s_ptr = strstr(s_ptr, parse_str);
		if(!s_ptr) { return rtn; }
		s_ptr = s_ptr + strlen(parse_str);

		e_ptr = strstr(s_ptr, "&");
		if(!e_ptr) { 
			e_ptr = strstr(s_ptr, "\r\n");
			if(!e_ptr) { 
			return rtn;
			}
		}
		memset(tmp, 0x00, 64);
		memcpy(tmp, s_ptr, (size_t)(e_ptr - s_ptr));
		p_list->key_data[i].param2 = (unsigned int)strtol(tmp, NULL, 16);

	}

	/*	//debug
	for(i = 0; i < license_count; i++)
	{
		printf("\e[95m key(%d) : %s \e[0m\n", i, p_list->key_data[i].key);
		printf("\e[95m date(%d) : %d \e[0m\n", i, p_list->key_data[i].acquired_date);
		printf("\e[95m date(%d) : %d \e[0m\n", i, p_list->key_data[i].acquired_date);
		printf("\e[95m param(%d)_1 : %08x \e[0m\n", i, p_list->key_data[i].param1);
		printf("\e[95m param(%d)_2 : %08x \e[0m\n", i, p_list->key_data[i].param2);
	}
	*/

	return IPCAM_SETUP_RTN_DONE;
}

int nf_ipcam_get_camera_license_key(int ch, NFIPCamLicenseKeyList *p_list)
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	if (ch >= NUM_IPX_CHANNEL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime[ch].state & MGMT_STATE_CONFIGURED == 0) { return IPCAM_SETUP_RTN_FAILED; }
	if(nf_ipcam_is_onvif_support(ch)) { return IPCAM_SETUP_RTN_FAILED; }
	if(!(runtime[ch].func & NF_IPCAM_FUNC_VA)) { return IPCAM_SETUP_RTN_FAILED; }

	int rtn;
	const char *path = "/cgi-bin/action.fcgi";
	//const char *query = "api=get_setup.system.license";
	const char *query = "action=get_setup&menu=system.license";

	const unsigned int buffer_size = 2048;
	char *buffer = (char*)malloc(buffer_size);

	icm_http http;
	icm_http_ch_init(&http, ch);

	memset(buffer, 0x00, buffer_size);
	rtn = icm_http_get_request(&http, path, query, buffer, buffer_size -1, NULL);

	if(rtn != ICM_RTN_OK)
	{
		free(buffer);
		return IPCAM_SETUP_RTN_FAILED;
	}

	//debug
	//printf("\e[31m [%s][%d] buffer : %s \e[0m\n", __FUNCTION__, __LINE__, buffer);

	//parse
	rtn = _nf_ipcam_get_parsed_license(buffer, p_list);

	free(buffer);

	return rtn;
}

static guint _nf_ipcam_set_maxbps(StreamData data, guint channel)
{
	GValue set_value = {0,};
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for (i = 0; i < 2; i++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.max_bps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.max_bps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
    		return 3;
    	}
    	g_value_unset(&set_value);
    }

    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    return 0;
}

int nf_ipcam_set_bitrate_ssc_off(void)
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }

	StreamData info;
	int i = 0;
	unsigned int channel = 0;

	if(nf_ipcam_is_vendor_videcon())	//Videcon vendor : IPXUHD-2064 
	{
		for(i = 0; i < NUM_IPX_CHANNEL; i++)
		{
			if(runtime[i].state & MGMT_STATE_CONFIGURED != 0)
			{
				memset(&info, 0x00, sizeof(StreamData));
				DAL_get_stream_data(&info, i);

				info.max_bps[0] = runtime[i].encoder.max_bitrate[0];
				info.max_bps[1] = runtime[i].encoder.max_bitrate[1];

				_nf_ipcam_set_maxbps(info, i);
				channel = (0xfff & (1<<i));
			}
		}

		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, channel, 0, 0);
	}
	return IPCAM_SETUP_RTN_DONE;
}

extern void nf_ipcam_get_default_login_info(char *p_id, char *p_pw)
{
	if(nf_ipcam_is_vendor_dayou() && (!nf_sysman_qcmode_is_enable()))
	{
		sprintf(p_id, "ADMIN");
		sprintf(p_pw, "111111");
	}
	else
	{
		//ITX default
		sprintf(p_id, "ADMIN");
		sprintf(p_pw, "1234");
	}
}

static int nf_ipcam_get_nvr_eth0_ip(char* ip_str)
{
	guint ipaddr = 0;
	ipaddr = get_netif_ip(HOST_ETH_DEV) ;

	snprintf(ip_str, 128, "%d.%d.%d.%d",
			(ipaddr&0xff),
			(ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16,
			(ipaddr&0xff000000)>>24);

	return IPCAM_SETUP_RTN_DONE;
}

void nf_ipcam_zmq_server_start()
{
	IPCAM_DBG(MAJOR, "start\n");
	int rtn;

	char ipaddr[128] = {0,};
	ZMQ_RECV_MANAGER* manager;
	manager = nf_ipcam_get_zmq_manager();
	nf_ipcam_get_nvr_eth0_ip(ipaddr);

	rtn = zmq_receiver_initialize(&(manager->g_zmq_ctx), &(manager->g_zmq_sock), ipaddr);
	if(!rtn)
		IPCAM_DBG(MAJOR, "error\n");

	rtn = zmq_receiver_start();
	if(!rtn)
		IPCAM_DBG(MAJOR, "error\n");

	rtn = nf_ipcam_vabox_data_operator_start();
	if(!rtn)
		IPCAM_DBG(MAJOR, "error\n");

	rtn = nf_ipcam_vabox_time_updator_start();
	if(!rtn)
		IPCAM_DBG(MAJOR, "error\n");

	IPCAM_DBG(MAJOR, "end\n");
}

void nf_ipcam_zmq_server_stop()
{
	IPCAM_DBG(MAJOR, "start\n");
	ZMQ_RECV_MANAGER *manager;
 	manager = nf_ipcam_get_zmq_manager(); 
	nf_ipcam_vabox_data_operator_stop();
	nf_ipcam_vabox_time_updator_stop();
	zmq_receiver_stop();
	zmq_receiver_finalize(&(manager->g_zmq_ctx), &(manager->g_zmq_sock));

	IPCAM_DBG(MAJOR, "end\n");
}

int nf_ipcam_get_channel_from_ipaddr(char *ipaddr)
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	char cam_ip[16] = {0,};
	int channel = -1;
	int i = 0;


	for(i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		memset(cam_ip, 0x00, sizeof(cam_ip));
		snprintf(cam_ip, 16, "%d.%d.%d.%d",
						( runtime[i].sys.ipaddr&0xff),
						((runtime[i].sys.ipaddr&0xff00)>>8),
						((runtime[i].sys.ipaddr&0xff0000)>>16),
						((runtime[i].sys.ipaddr&0xff000000)>>24));
						
		if(strncmp(ipaddr, cam_ip, 15) == 0)
		{
			channel = i;
			break;
		}
	}

	if (channel == -1)
	{
		g_message("[%s, %d] Not found channel from IP(%s)", __func__, __LINE__, ipaddr);
	}

	return channel;
}

int nf_ipcam_get_channel_from_macaddr(char *p_str)
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	int channel = -1;
	int i = 0, j = 0;
	char ch_mac[32];

	for(i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		memset(ch_mac, 0x00, 32);
		for(j = 0; j < 6; j++)
		{
			sprintf(ch_mac+(j*2), "%02x", runtime[i].sys.macaddr[j]);
		}
		//printf("\e[104m [%s][%d] p_str(%s) ch_mac(%s) \e[0m\n", __func__, __LINE__, p_str, ch_mac);
		if(strncasecmp(p_str, ch_mac, strlen(p_str)) == 0)
		{
			channel = i;
			//printf("\e[95m return channel is (%d) \e[0m\n", channel);
			break;
		}
	}

	return channel;
}

extern void _convert_pmask_area_by_corridor_view(int ch, NFIPCamPrivacyMask *pmask)
{
	char key[64];
	int corridor_view = 0;
	int i;
	
	int x1, y1;
	int x2, y2;
	int width, height;

	width = 16;
	height = 9;

	mtable* runtime = NULL;
	runtime = get_runtime();

	snprintf(key, 64, "cam.C%d.corridor_view", ch);
	corridor_view = nf_sysdb_get_uint(key);

	for(i=0; i<runtime[ch].privacymask.max_rect; i++)
	{
		x1 = pmask->lt[i].x;
		y1 = pmask->lt[i].y;
		x2 = pmask->rb[i].x;
		y2 = pmask->rb[i].y;

		if(x1==-1 && y1==-1 && x2==-1 && y2==-1)
		{
			continue;
		}

		if(corridor_view == 0)
		{
			continue;
		}
		else if(corridor_view == 1)
		{
			pmask->lt[i].x = height - y2 -1;
			pmask->lt[i].y = x1;

			pmask->rb[i].x = height - y1 -1;
			pmask->rb[i].y = x2;
		}
		else if(corridor_view == 2)
		{
			pmask->lt[i].x = y1;
			pmask->lt[i].y = width - x2 -1;

			pmask->rb[i].x = y2;
			pmask->rb[i].y = width - x1 -1;
		}
	}
}


extern void _convert_roi_area_by_corridor_view(int ch, NFIPCamSetupROIArea *roi)
{
	char key[64];
	int corridor_view = 0;
	int i;

	int x1, y1;
	int x2, y2;
	int width, height;

	// default setting
	width = 16; // default
	height = 9; 

	mtable* runtime = NULL;
	runtime = get_runtime();
	
	snprintf(key, 64, "cam.C%d.corridor_view", ch);
	corridor_view = nf_sysdb_get_uint(key);

	for(i=0; i<runtime[ch].roi_area.max_rect; i++)
	{
		x1 = roi->roi_area[i].left_top.x;
		y1 = roi->roi_area[i].left_top.y;
		x2 = roi->roi_area[i].right_bottom.x;
		y2 = roi->roi_area[i].right_bottom.y;

		if(x1==0 && y1==0 && x2==0 && y2==0)
		{
			continue;
		}

		if(corridor_view == 0) // Not rotate
		{
			continue;
		}
		else if(corridor_view == 1) // 90 degree rotate , lt(x1,y1), rb(x2,y2) -> lt'(h-y2,x1), rb'(h-h1, x2)
		{
			roi->roi_area[i].left_top.x = height - y2;
			roi->roi_area[i].left_top.y = x1;
			
			roi->roi_area[i].right_bottom.x = height - y1;
			roi->roi_area[i].right_bottom.y = x2;
				
		}
		else if(corridor_view == 2) // 270 degree rotate, lt(x1,y1), rb(x2,y2) -> lt'(y1,w-x2),rb'(y2, w-x1)
		{
			roi->roi_area[i].left_top.x = y1;
			roi->roi_area[i].left_top.y = width - x2;

			roi->roi_area[i].right_bottom.x = y2;
			roi->roi_area[i].right_bottom.y = width - x1;
		}
	}
}

static int _is_updated_ai_rule_engine(int ch)
{
	mtable* runtime = get_runtime();
	ai_rule_engine engine_info;
	rule_info info;
	char key[64];
	int i=0;
	int result = 0;

	int zone_num = 0;
	int count_num = 0;
	

	// en_engine (""cam.dvabx.cfg.R0.en_engine"")
	snprintf(key, 64, "cam.dvabx.cfg.R%d.en_engine", ch);
	engine_info.en_engine = nf_sysdb_get_bool(key);
	if(runtime[ch].rule_engine.en_engine != engine_info.en_engine)
	{
		result = 1;
	}
	runtime[ch].rule_engine.en_engine = engine_info.en_engine;

	// DB Updated Check


	// z_cnt ("cam.dvabx.rule.R0.nzones")
	snprintf(key, 64, "cam.dvabx.rule.R%d.nzones", ch);
	engine_info.z_cnt = nf_sysdb_get_uint(key);
	if(runtime[ch].rule_engine.z_cnt != engine_info.z_cnt)
	{
		result = 1;
	}
	runtime[ch].rule_engine.z_cnt = engine_info.z_cnt;
	zone_num = engine_info.z_cnt;

	// Zone DB Check
	for(i=0; i<zone_num; i++)
	{
		// Z_name ("cam.dvabx.face.rule.R0.Z0.name")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.name", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].zone_name)!=0)
			{
				//runtime[ch].rule_engine.rule[i].zone_name[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].zone_name, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_name));
				result=1;
			}
		}
		else
		{
			strncpy(info.zone_name, nf_sysdb_get_str_nocopy(key), sizeof(info.zone_name));
			if(strcmp(runtime[ch].rule_engine.rule[i].zone_name, info.zone_name))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].zone_name, info.zone_name, sizeof(info.zone_name));
		}

		// zone_id ("cam.dvabx.rule.R0.Z%d.id")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.id", ch, i);
		info.zone_id = nf_sysdb_get_int(key);
		if(runtime[ch].rule_engine.rule[i].zone_id != info.zone_id)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].zone_id = info.zone_id;

		// type ("cam.dvabx.rule.R0.Z%d.type")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.type", ch, i);
		info.type = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].type != info.type)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].type = info.type;

		// zone_color ("cam.dvabx.rule.R0.Z%d.d_color")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.d_color", ch, i);
		info.zone_color = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].zone_color != info.zone_color)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].zone_color = info.zone_color;

		// zone_active ("cam.dvabx.rule.R0.Z%d.active")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.active", ch,i);
		info.zone_active = nf_sysdb_get_bool(key);
		if(runtime[ch].rule_engine.rule[i].zone_active != info.zone_active)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].zone_active = info.zone_active;

		// object ("cam.dvabx.rule.R0.Z%d.interest_obj")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.interest_obj", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].zone_object)!=0)
			{
				//runtime[ch].rule_engine.rule[i].zone_object[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].zone_object, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_object));
				result=1;
			}

		}
		else
		{
			strncpy(info.zone_object, nf_sysdb_get_str_nocopy(key), sizeof(info.zone_object));
			if(strcmp(runtime[ch].rule_engine.rule[i].zone_object, info.zone_object))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].zone_object, info.zone_object, sizeof(info.zone_object));
		}

		// Z_event ("cam.dvabx.rule.R0.Z%d.enabled")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.enabled", ch, i);
		info.zone_event = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].zone_event != info.zone_event)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].zone_event = info.zone_event;

		// timeout ("cam.dvabx.rule.R0.Z%d.time_sarlf")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.time_sarlf", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].zone_timeout)!=0)
			{
				//runtime[ch].rule_engine.rule[i].zone_timeout[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].zone_timeout, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_timeout));
				result=1;
			}

		}
		else
		{
			strncpy(info.zone_timeout, nf_sysdb_get_str_nocopy(key), sizeof(info.zone_timeout));
			if(strcmp(runtime[ch].rule_engine.rule[i].zone_timeout, info.zone_timeout))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].zone_timeout, info.zone_timeout, sizeof(info.zone_timeout));
		}

		// c_threshold ("cam.dvabx.rule.R0.Z%d.c_threshold")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.c_threshold", ch, i);
		info.count_threshold = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].count_threshold != info.count_threshold)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].count_threshold = info.count_threshold;

		// Z_area ("cam.dvabx.rule.R0.Z%d.pt")
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.pt", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].zone_area)!=0)
			{
				//runtime[ch].rule_engine.rule[i].zone_area[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].zone_area, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_area));
				result=1;
			}
		}
		else
		{
			strncpy(info.zone_area, nf_sysdb_get_str_nocopy(key), sizeof(info.zone_area));
			if(strcmp(runtime[ch].rule_engine.rule[i].zone_area, info.zone_area))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].zone_area, info.zone_area, sizeof(info.zone_area));
		}

		// npts
		snprintf(key, 64, "cam.dvabx.rule.R%d.Z%d.npts", ch, i);
		info.npts = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].npts != info.npts)
		{
			result = 1;

		}
		runtime[ch].rule_engine.rule[i].npts = info.npts;
	}
	

	// c_cnt ("cam.dvabx.rule.R0.ncounters")
	snprintf(key, 64, "cam.dvabx.rule.R%d.ncounters", ch);
	engine_info.c_cnt = nf_sysdb_get_uint(key);
	if(runtime[ch].rule_engine.c_cnt != engine_info.c_cnt)
	{
		result = 1;
	}
	runtime[ch].rule_engine.c_cnt = engine_info.c_cnt;
	count_num = engine_info.c_cnt;

	// Count DB Check
	for(i=0; i<count_num; i++)
	{
		// count_name ("cam.dvabx.face.rule.R0.C0.name")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.name", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].count_name)!=0)
			{
				//runtime[ch].rule_engine.rule[i].count_name[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].count_name, 0x00, sizeof(runtime[ch].rule_engine.rule[i].count_name));
				result=1;
			}

		}
		else
		{
			strncpy(info.count_name, nf_sysdb_get_str_nocopy(key), sizeof(info.count_name));
			if(strcmp(runtime[ch].rule_engine.rule[i].count_name, info.count_name))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].count_name, info.count_name, sizeof(info.count_name));
		}

		// count_id ("cam.dvabx.rule.R0.C%d.id")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.id", ch, i);
		info.count_id = nf_sysdb_get_int(key);
		if(runtime[ch].rule_engine.rule[i].count_id != info.count_id)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].count_id = info.count_id;

			
		
		// count_color ("cam.dvabx.rule.R0.C%d.d_color")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.d_color", ch, i);
		info.count_color = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].count_color != info.count_color)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].count_color = info.count_color;

		
		// count_active ("cam.dvabx.rule.R0.C%d.active")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.active", ch,i);
		info.count_active = nf_sysdb_get_bool(key);
		if(runtime[ch].rule_engine.rule[i].count_active != info.count_active)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].count_active = info.count_active;

		
		// C_event ("cam.dvabx.rule.R0.S%d.enabled"")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.enabled", ch, i);
		info.count_event = nf_sysdb_get_uint(key);
		if(runtime[ch].rule_engine.rule[i].count_event != info.count_event)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].count_event = info.count_event;

	
		
		// e_value ("cam.dvabx.rule.R0.C%d.e_value ")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.e_value", ch, i);
		info.e_value = nf_sysdb_get_int(key);
		if(runtime[ch].rule_engine.rule[i].e_value != info.e_value)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].e_value = info.e_value;


		// re_alert ("cam.dvabx.rule.R0.C%d.reset_alert")
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.reset_alert", ch, i);
		info.reset_alert = nf_sysdb_get_bool(key);
		if(runtime[ch].rule_engine.rule[i].reset_alert != info.reset_alert)
		{
			result = 1;
		}
		runtime[ch].rule_engine.rule[i].reset_alert = info.reset_alert;

		// zid (""cam.dvabx.rule.R0.C%d.zid"")
		snprintf(key, 64,"cam.dvabx.rule.R%d.C%d.zid", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].zid)!=0)
			{
				//runtime[ch].rule_engine.rule[i].zid[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].zid, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zid));
				result=1;
			}

		}
		else
		{
			strncpy(info.zid, nf_sysdb_get_str_nocopy(key), sizeof(info.zid));
			if(strcmp(runtime[ch].rule_engine.rule[i].zid, info.zid))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].zid, info.zid, sizeof(info.zid));
		}

		
		// C_area ("cam.dvabx.rule.R0.C%d.pt"
		snprintf(key, 64, "cam.dvabx.rule.R%d.C%d.pt", ch, i);
		if(nf_sysdb_get_str_nocopy(key)==NULL)
		{
			if(strlen(runtime[ch].rule_engine.rule[i].count_area)!=0)
			{
				//runtime[ch].rule_engine.rule[i].count_area[0] = '\0';
				memset(runtime[ch].rule_engine.rule[i].count_area, 0x00, sizeof(runtime[ch].rule_engine.rule[i].count_area));
				result=1;
			}

		}
		else
		{

			strncpy(info.count_area, nf_sysdb_get_str_nocopy(key), sizeof(info.count_area));
			if(strcmp(runtime[ch].rule_engine.rule[i].count_area, info.count_area))
			{
				result =1;
			}
			strncpy(runtime[ch].rule_engine.rule[i].count_area, info.count_area, sizeof(info.count_area));
		}
	}

	return result;
}

static _is_updated_embedded_osd(int ch)
{
	mtable* runtime = get_runtime();
	embedded_osd e_osd;
	char key[64];
	int result = 0;

	// mode
	snprintf(key, 64, "cam.dvabx.osd.R%d.display_mode", ch);
	e_osd.display_mode = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.display_mode != e_osd.display_mode)
	{
		runtime[ch].e_osd.display_mode = e_osd.display_mode;
		result = 1;
	}

	// object_color
	snprintf(key, 64, "cam.dvabx.osd.R%d.object_color", ch);
	e_osd.object_color = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.object_color != e_osd.object_color)
	{
		runtime[ch].e_osd.object_color = e_osd.object_color;
		result = 1;
	}

	// rule_color
	snprintf(key, 64, "cam.dvabx.osd.R%d.rule_color", ch);
	e_osd.rule_color = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.rule_color != e_osd.rule_color)
	{
		runtime[ch].e_osd.rule_color = e_osd.rule_color;
		result = 1;
	}

	// event_color
	snprintf(key, 64, "cam.dvabx.osd.R%d.event_color", ch);
	e_osd.event_color = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.event_color != e_osd.event_color)
	{
		runtime[ch].e_osd.event_color = e_osd.event_color;
		result = 1;
	}

	// line_width
	snprintf(key, 64, "cam.dvabx.osd.R%d.line_width", ch);
	e_osd.line_width = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.line_width != e_osd.line_width)
	{
		runtime[ch].e_osd.line_width = e_osd.line_width;
		result = 1;
	}

	// line_transparency
	snprintf(key, 64, "cam.dvabx.osd.R%d.line_transparency", ch);
	e_osd.line_transparency = nf_sysdb_get_uint(key);
	if(runtime[ch].e_osd.line_transparency != e_osd.line_transparency)
	{
		runtime[ch].e_osd.line_transparency = e_osd.line_transparency;
		result = 1;
	}

	// object_type
	snprintf(key, 64, "cam.dvabx.osd.R%d.object_type", ch);
	if(nf_sysdb_get_str_nocopy(key)==NULL)
	{
		if(strlen(runtime[ch].e_osd.object_type)!=0)
		{
			memset(runtime[ch].e_osd.object_type, 0x00, sizeof(runtime[ch].e_osd.object_type));
			result=1;
		}
	}
	else
	{
		strncpy(e_osd.object_type, nf_sysdb_get_str_nocopy(key), sizeof(e_osd.object_type));
		if(strcmp(runtime[ch].e_osd.object_type, e_osd.object_type))
		{
			strncpy(runtime[ch].e_osd.object_type, e_osd.object_type, sizeof(e_osd.object_type));
			result = 1;
		}
	}

	return result;
}

static int _is_updated_dl_option(int ch)
{
	mtable *runtime = get_runtime();
	ai_rule_engine engine_info;
	ai_dl_option dl_option;
	char key[64];
	int result = 0;

	// en_engine
	snprintf(key, 64, "cam.dvabx.cfg.R%d.en_engine", ch);
	engine_info.en_engine = nf_sysdb_get_bool(key);
	if(runtime[ch].rule_engine.en_engine != engine_info.en_engine)
	{
		result = 1;
		runtime[ch].rule_engine.en_engine = engine_info.en_engine;
	}
	
	// track_ref
	snprintf(key, 64, "cam.dvabx.opt.R%d.track_ref", ch);
	dl_option.track_ref = nf_sysdb_get_uint(key);
	if(dl_option.track_ref != runtime[ch].dl_option.track_ref)
	{
		result =1;
		runtime[ch].dl_option.track_ref = dl_option.track_ref;
	}
	
	// en_ignore
	snprintf(key, 64, "cam.dvabx.opt.R%d.en_static_filter", ch);
	dl_option.en_static_filter = nf_sysdb_get_bool(key);
	if(dl_option.en_static_filter != runtime[ch].dl_option.en_static_filter)
	{
		result = 1;
		runtime[ch].dl_option.en_static_filter = dl_option.en_static_filter;
	}
	
	// ignore_level
	snprintf(key, 64, "cam.dvabx.opt.R%d.static_filter_sense", ch);
	dl_option.static_filter_sense = nf_sysdb_get_uint(key);
	if(dl_option.static_filter_sense != runtime[ch].dl_option.static_filter_sense)
	{
		result = 1;
		runtime[ch].dl_option.static_filter_sense = dl_option.static_filter_sense;
	}

	return result;
}


// if exist rule_engine, return 1
// else return 0
extern int nf_ipcam_get_cam_ai_type(int ch)
{
	mtable *runtime = get_runtime();
	if(runtime[ch].ai.model_type_value == NF_AI_MODEL_AICAM_PRO)
	{
		return CAM_AI_TYPE_PRO;
	}
	else if(runtime[ch].rule_engine.have_ai_engine==1)
	{
		return CAM_AI_TYPE_CLAIR2;
	}
	else if(runtime[ch].ai.ai_support==1)
	{
		return CAM_AI_TYPE_CLAIR1;
	}
	else
	{
		return CAM_AI_TYPE_NORMAL;
	}
}

// return
// 0 : success
// -1 : not 200ok
// -2 : curl timeout
// -3 : not clair-2nd
extern int nf_ipcam_get_ai_rule_engine(int ch)
{
	// ai rule engine
	int ret=0;
	ret = itx_cam_get_ai_rule_engine_and_save_db(ch);
	if(ret !=0)
	{
		printf("[%s:%d] Get Failed (rule_engine), errno:(%d)\n", __func__, __LINE__, ret);
		return ret;
	}
	
	// dl option
	ret = itx_cam_get_dl_option_and_save_db(ch);
	if(ret != 0 )
	{
		printf("[%s:%d] Get Failed (dl_option), errno:(%d)\n",__func__, __LINE__, ret);
	}
	
	return ret;
}

extern int nf_ipcam_get_embedded_osd(int ch)
{
	int ret = 0;
	ret = itx_cam_get_embedded_osd_and_save_db(ch);
	return ret;
}


#endif //__NF_API_IPCAM_C__

