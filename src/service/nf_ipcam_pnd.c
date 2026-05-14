/**
 * @file nf_ipcam_pnd.c
 * @brief IP카메라 접속 관리(Plug and Detect).
 * @author Jae-young Kim
 * @date 2010-08-26
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */

#ifndef __NF_IPCAM_PND_C__
#define __NF_IPCAM_PND_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>

#include <dispmux.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <gobjmrtppipe.h>
#include <nf_codec_header.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_sysdb.h>
#include <nf_notify.h>

#include <openssl/ssl.h>

#if MAKE_NOTIFY_FIRE
#include <nf_api_eventlog.h>
#include <nf_logevtdef.h>
#endif

#include "nf_common.h"
#include "nf_util_device.h"
#include "board_pp.h"

/** @def MAKE_RANDOM
 *  @brief 0부터 x사이의 난수 발생.
 */
#define MAKE_RANDOM(x) (rand() % (((x)+1)&0xFFFFFFF))


const char *_IPCAM_EVENT_STR__[] = {
	"IPCAM_EVENT_NULL",

	"IPCAM_EVENT_LINK_CHANGED",

	"IPCAM_EVENT_DELAY_TO_CONN",
	"IPCAM_EVENT_DELAY_READY",
	"IPCAM_EVENT_DEVICE_READY",
	"IPCAM_EVENT_DEVICE_OUT",

	"IPCAM_EVENT_NVS_SUBCH_READY",
	"IPCAM_EVENT_NVS_SUBCH_CLOSE",

	"IPCAM_EVENT_STREAM_READY",
	"IPCAM_EVENT_ONVIF_PRIME_READY",
	"IPCAM_EVENT_ONVIF_GENERAL_READY",

	"IPCAM_EVENT_UNKNOWN_DEVICE",
	"IPCAM_EVENT_CONNECTION_FAIL",
	"IPCAM_EVENT_LOGIN_FAIL",
	"IPCAM_EVENT_MODEL_UNSUPPORT",
	"IPCAM_EVENT_CONFIG_FAIL",
	"IPCAM_EVENT_STREAM_FAIL"
};
const char *_IPCAM_PND_EVENT_STR__[] = {
	"PND_TYPE_UNPLUGGED",
	"PND_TYPE_PLUGGED",
	"PND_TYPE_MAC_RESOLVED",
	"PND_TYPE_IP_REQUESTED",
	"PND_TYPE_IP_DONE",
	"PND_TYPE_SETUP_REQUESTED",
	"PND_TYPE_SETUP_DONE",
	"PND_TYPE_VIDEO_START",

	"PND_TYPE_TIMEOUT",
	"PND_TYPE_UNKNOWN",
	"PND_TYPE_UNSUPPORTED",
	"PND_TYPE_CONNECTION_FAIL",
	"PND_TYPE_CONFIG_FAIL",
	"PND_TYPE_STREAM_FAIL",

	"PND_TYPE_LOGIN_FAIL",
	"PND_TYPE_SOMETHING_FAIL",

	"PND_TYPE_HUB_STATUS"
};

/** @struct NETWORK_INFO_T
 *  @brief No PND(KMW 등) 시나리오에서 사용하는 채널별 접속 정보.
 */
typedef struct _NETWORK_INFO_T__
{
	int            ch_num;
	int            model_code;
	char           username[64];
	char           password[64];
	int            video_cnt;
	int            resolution[3];
	unsigned int   ip_addr;
	char           rtsp_addr[3][256];
	unsigned short rtsp_port[3];
	unsigned short http_port;
} NETWORK_INFO_T;

/** @var resource_fail_count
 *  @brief RTSP stream open시에 socket에러 발생시 증가. 현재 사용안함.
 */
static int resource_fail_count = 0;
/** @var is_day
 *  @brief 모션 설정 중 낮과 밤 구분변수.
 */
static int is_day[AVAILABLE_MAX_CH];
/** @var product_portnum, external_portnum
 *  @brief 각각 NVR내부, 외부 포트수. 현재는 항상 8, 8임 - IPX 16ch로 동작.
 */
static int product_portnum = 0;
static int external_portnum = 0;
/** @var cur_sub_network_addr
 *  @brief 현재 내부 포트관리용 보조interface ip.(10.x.x.1)
 */
static unsigned int cur_sub_network_addr = 0;
#ifdef DUAL_LAN_NETWORK
static unsigned int cur_hub_sub_network_addr = 0;
#endif
static int portscan_trig = 0;
//static int ip_to_set = 0;
/** @var tv_system
 *  @brief 현재 시스템의 NTSC/PAL 구분. 0 - NTSC, 1 - PAL.
 */
static int tv_system = 0;

#ifdef DUAL_LAN_NETWORK
static unsigned int hub_interface_ip = 0;
#endif

/** @var vloss_status
 *  @brief 현재 Video Loss상태 채널 bitmask.
 */
/** @var _vloss_old
 *  @brief Notify용도로 보관하는 기존 Video Loss상태.
 */
static unsigned int vloss_status = 0xffffffff&NUM_ACTIVE_CH_MASK;
static unsigned int _vloss_old = 0xffffffff&NUM_ACTIVE_CH_MASK;

static pthread_t discovery_th;
static pthread_t discovery_onvif_th;
static pthread_t discovery_rcv_th;
static pthread_t discovery_onvif_rcv_th;
static pthread_t event_th;
static pthread_t rate_th;
static pthread_t vloss_th;
static pthread_t static_cam_th;
static pthread_t recovery_th;
static pthread_t noti_th;
static pthread_t xm_noti_th;

static pthread_t s1_event_th;

/** @var runtime
 *  @brief 카메라에 대한 모든 정보가 들어가는 global variable.
 */
static mtable runtime[AVAILABLE_MAX_CH];
static GAsyncQueue *event_queue = NULL;
static GAsyncQueue *recevt_queue = NULL;
static GAsyncQueue *vloss_queue = NULL;
static GobjMrtpPipe *h_iplive = NULL;

static pthread_mutex_t _switch_syscall_mtx = PTHREAD_MUTEX_INITIALIZER;

//static int nf_rtsp_errno[AVAILABLE_MAX_CH][NF_IPCAM_STREAM_MAX];
static int nf_pnd_status[AVAILABLE_MAX_CH];
static int nf_pnd_recovery_cnt[AVAILABLE_MAX_CH];

static unsigned int mac_probed = 0;
static unsigned char host_mac[6] = { 0, 0, 0, 0, 0, 0 };
static NETWORK_INFO_T *net_info;

static int dnn_is_day[AVAILABLE_MAX_CH];

static void ipx_cmd_callback_func(gint, gint, gint, gint, gpointer);
static void nf_ipcam_discovery_init(void);
static void _cam_event_init(void);
static void _cam_manager_init(void);
static void ipx_ipcam_free_event(IPX_PND_EVENT* evt);
//static void ipx_ip_manager(void*);
static void ipx_vloss_handler_func(void);
static void ipx_delayed_setup_handler_func(void);
static void ipx_event_handler_func(void);
static void ipx_event_noti_server(void *param);
static void ipx_xm_event_noti_server(void *param);
extern void nf_ipcam_event_notification_server(void);
extern void onvif_event_puller(void);
#ifdef DUAL_LAN_NETWORK
extern unsigned int get_hub_info(void);
#endif

static unsigned int ipx_resol_cm2icodec(uint64_t);
static uint64_t ipx_resol_icodec2cm(unsigned int);

static void pnd_recovery_func(void*);
static unsigned int a8tohex(char*);

static void _send_arp_req(unsigned int, unsigned int);
static void _unref_vnet(int);

static void _init_switch_device(void);


extern gboolean nf_notify_fire_params(const gchar*, guint, guint, guint, guint);
extern void cam_setup_init(void);
extern int cam_reboot_poe(int);
extern int cam_reboot(int);

extern int nf_ipcam_set_bitrate(int, unsigned int, unsigned int, NFIPCamSetupCallback, gpointer, GError**);
extern int nf_ipcam_set_oneshot(int, NFIPCamSetupCallback, gpointer, GError**);
extern void ipcam_discovery_main_loop(void);
extern void ipcam_discovery_onvif_loop(void);
extern void ipcam_discovery_recv_loop(void);
extern void ipcam_discovery_onvif_recv_loop(void);

#ifdef DUAL_LAN_NETWORK
unsigned int get_hub_interface_addr(void)
{
	return hub_interface_ip;
}
#endif

extern void switch_mtx_lock(void)
{
	pthread_mutex_lock(&_switch_syscall_mtx);
}

extern void switch_mtx_unlock(void)
{
	pthread_mutex_unlock(&_switch_syscall_mtx);
}

extern unsigned int get_sub_network(void)
{
	return cur_sub_network_addr;
}

#ifdef DUAL_LAN_NETWORK
extern unsigned int get_hub_sub_network(void)
{
	return cur_hub_sub_network_addr;
}
#endif

extern int get_tv_system(void)
{
	return tv_system;
}

extern int get_dn_now(int ch)
{
	return is_day[ch];
}

/**
 * @brief 각 채널별로 모션설정 중 낮과 밤을 불러와 현재 시간과 비교한다.
 */
extern void calculate_day_night(void)
{
	int i, hour;
	time_t timer;
	struct tm *t;
	char key[64];
	int start;
	int end;
	int _is_day;

	timer = time(NULL);
	t = localtime(&timer);

	hour = t->tm_hour;

	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		snprintf(key, 64, "alarm.motion.M%d.time_start", i);
		start = nf_sysdb_get_uint(key);
		snprintf(key, 64, "alarm.motion.M%d.time_end", i);
		end = nf_sysdb_get_uint(key);

		if (start == end)
		{
			_is_day = 0;
		}
		else if (start < end)
		{
			if (hour >= start && hour < end) { _is_day = 1; }
			else { _is_day = 0; }
		}
		else
		{
			if (hour >= end && hour < start) { _is_day = 0; }
			else { _is_day = 1; }
		}

		if (is_day[i] != _is_day)
		{
			is_day[i] = _is_day;
			nf_notify_fire_params("sysdb_tmp_change", NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, i, 0, 0);
		}
	}
}

extern int get_ircut_dnn_now(int ch)
{
	return dnn_is_day[ch];
}

extern void calculate_dnn(void)
{
	int 	now_hour;
	int 	now_min;
	int 	start_hour;
	int 	start_min;
	int		end_hour;
	int 	end_min;
	int 	i;
	char 	key[64];
	time_t 	timer;
	struct 	tm *t;
	int 	_is_day = 0;

	timer 		= time(NULL);
	t 			= localtime(&timer);

	now_hour 	= t->tm_hour;
	now_min		= t->tm_min;




	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		start_hour = runtime[i].image.dnn_schedule.start.hour;
		start_min = runtime[i].image.dnn_schedule.start.min;
		end_hour = runtime[i].image.dnn_schedule.end.hour;
		end_min = runtime[i].image.dnn_schedule.end.min;

		if (start_hour == end_hour)
		{
			if(start_min == end_min)
			{
				_is_day = 0;
			}
			else if(start_min < end_min)
			{
				if((now_hour == start_hour && now_hour == end_hour)
				&& (now_min >= start_min && now_min < end_min))
				{
					_is_day = 0;
				}
				else
				{
					_is_day = 1;
				}
			}
			else if(start_min > end_min)
			{
				if((now_hour == start_hour && now_hour == end_hour)
				&& (now_min < start_min && now_min >= end_min))
				{
					_is_day = 1;
				}
				else
				{
					_is_day = 0;
				}
			}
		}
		else if (start_hour < end_hour)
		{
			if (now_hour >= start_hour && now_hour <= end_hour) 
			{
				_is_day = 0; 

				if(((now_hour == start_hour) && (now_min < start_min))
				|| ((now_hour == end_hour) && (now_min >= end_min)))
				{
					_is_day = 1;
				}
			}
			else 
			{ 
				_is_day = 1; 
			}
		}
		else if (start_hour > end_hour)
		{
			if (now_hour >= end_hour && now_hour <= start_hour) 
			{
				_is_day = 1; 

				if(((now_hour == start_hour) && (now_min >= start_min))
				|| ((now_hour == end_hour) && (now_min < end_min)))
				{
					_is_day = 0;
				}
			}
			else 
			{
				_is_day = 0; 
			}
		}

		if (dnn_is_day[i] != _is_day)
		{
			dnn_is_day[i] = _is_day;
			nf_notify_fire_params("sysdb_tmp_change", NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, i, 0, 0);
		}
	}
}

/**
 * @brief PND 이벤트 관련 notification을 외부로 발생시킨다.
 * @param ch 채널 번호.
 * @param type 이벤트 유형.
 * @param l __LINE__.
 * @param f __FILE__.
 */
extern void nf_pnd_evt_notify_fire(int ch, int type, int l, char* f)
{
#if MAKE_NOTIFY_FIRE
	IPCAM_DBG(MINOR, "PND_EVENT CH(%d) TYPE(%s) %s:%d\n",ch,_IPCAM_PND_EVENT_STR__[type],f,l);
	nf_notify_fire_params("pnd_event", type, (guint)ch, 0, 0);
#endif
}

/**
 * @brief PND progess 관련 notification을 외부로 발생시킨다.
 * @param ch 채널 번호.
 * @param prog Progress 단계.
 * @param l __LINE__.
 * @param f __FILE__.
 */
extern void nf_pnd_prog_notify_fire(int ch, int prog, int l, char* f)
{
#if MAKE_NOTIFY_PROG
	IPCAM_DBG(MINOR, "PND_PROGRESS CH(%d) PROGRESS(%d) %s:%d\n",ch,prog,f,l);
	nf_notify_fire_params("pnd_progress", prog, (guint)ch, 0, 0);
#endif
}

/**
 * @brief PND event를 enque한다.
 * @param ch 채널 번호.
 * @param type Event 유형. @see __IPX_IPCAM_EVENT_TYPE_E
 * @param l __LINE__.
 * @param f __FILE__.
 */
extern void nf_pnd_queue_push(int ch, int type, int l, char* f)
{
	GAsyncQueue *queue = get_queue();
	IPX_PND_EVENT *evt = ipx_ipcam_new_event(type);
	evt->port = ch;
	g_async_queue_push(queue, evt);
	IPCAM_DBG(MINOR, "QUEUE_PUSH CH(%d) TYPE(%s) %s:%d\n",ch,_IPCAM_EVENT_STR__[type],f,l);
}

/**
 * @brief PND event를 다음 event 및 delay정보와 같이 enque한다.
 * @param ch 채널 번호.
 * @param next 다음 event enum.
 * @param delay delay만큼 대기 후 다음 event가 발생한다.
 * @param l __LINE__.
 * @param f __FILE__.
 *
 * IP변경 및 S1 factory clear시나리오시 n초간 카메라 상태변경 대기 후 연결하기 위하여 개발됨.
 */
extern void nf_pnd_queue_delay(int ch, int next, int delay, int l, char *f)
{
	GAsyncQueue *queue = get_queue();
	IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_EVENT_DELAY_TO_CONN);
	evt->port = ch;
	evt->d.params[0] = next;
	evt->d.params[1] = delay;
	g_async_queue_push(queue, evt);
	IPCAM_DBG(MINOR, "QUEUE_PUSH CH(%d) TYPE(%s) do after %d seconds %s:%d\n",ch,_IPCAM_EVENT_STR__[next],delay,f,l);
}

/**
 * @brief 내부 포트 관리용 network interface를 생성한다.
 * @return SUB_NETWORK_CHANGED - 내부 IP변경시, 아니면 SUB_NETWORK_UNCHANGED.
 */
extern int set_sub_network(void)
{
	unsigned int h = 0;
	unsigned int m = 0;
	unsigned int g = 0;
	unsigned int new_sub_addr = 0;
#ifdef DUAL_LAN_NETWORK
	unsigned int new_hub_sub_addr = 0;
#endif
#if defined(ENABLE_PROJECT_KMW)||defined(ENABLE_PROJECT_KUMMI)
	const char temp_ip[] = "%d.%u.%u.250";
#else
	const char temp_ip[] = "%d.%u.%u.1";
#endif
	char ip_str[16];
	char system_cmd_str[256];
	unsigned char sys_mac[8];

	IPCAM_DBG(MAJOR, "re-configure\n");

	memset(system_cmd_str, 0x00, 256);

	h = get_init_info();
	if ((h&0xff) == 10)
	{
		snprintf(ip_str, 16, temp_ip, 192, 168, 254);
	}
	else
	{
#if defined(ENABLE_PROJECT_KUMMI)
		snprintf(ip_str, 16, temp_ip, 10, 1, 10);
#elif defined(ENABLE_PROJECT_KMW)
		snprintf(ip_str, 16, temp_ip, 192, 168, 200);
#else
		nf_netif_get_mac_str(&sys_mac[0]);
		snprintf(ip_str, 16, temp_ip, 10, sys_mac[4], sys_mac[5]);
		//snprintf(ip_str, 16, temp_ip, 10, 1, 10);
#endif		

	}

	snprintf(system_cmd_str, 256, "ifconfig " LOCAL_ETH_DEVICE " %s netmask 255.255.255.0", ip_str);
#if IPCAM_UNIT_TEST
	IPCAM_DBG(MINOR, "system(%s)\n", system_cmd_str);
	system(system_cmd_str);
#else
	IPCAM_DBG(MINOR, "proxy_system(%s)\n", system_cmd_str);
	proxy_system(system_cmd_str, 1, 3);
#endif
	new_sub_addr = inet_addr(ip_str);

#ifdef DUAL_LAN_NETWORK
	if ((h&0xff) == 10)
	{
		snprintf(ip_str, 16, temp_ip, 192, 168, 253);
	}
	else
	{
        snprintf(ip_str, 16, temp_ip, 10, sys_mac[4], (sys_mac[5] + 1)%256);
	}
	printf("[%s:%d] %s set ip_str[%s]\n", __func__, __LINE__, HUB_ETH_DEVICE, ip_str);

	snprintf(system_cmd_str, 256, "ifconfig %s %s netmask 255.255.255.0", HUB_ETH_DEVICE, ip_str);
	proxy_system(system_cmd_str, 1, 3);
	inet_pton(AF_INET, ip_str, (void*)&hub_interface_ip);
	new_hub_sub_addr = inet_addr(ip_str);
#endif

#ifdef DUAL_LAN_NETWORK
	if ((new_sub_addr != cur_sub_network_addr) || (new_hub_sub_addr != cur_hub_sub_network_addr))
	{
		cur_sub_network_addr = new_sub_addr;
		cur_hub_sub_network_addr = new_hub_sub_addr;
		return SUB_NETWORK_CHANGED;
	}
#else
	if (new_sub_addr != cur_sub_network_addr)
	{
		cur_sub_network_addr = new_sub_addr;
		return SUB_NETWORK_CHANGED;
	}
#endif
	else
	{
		return SUB_NETWORK_UNCHANGED;
	}
	sleep(1);

	return 0;	// just to make the compiler happy
}

/**
 * @brief Switch Device 을 통해 임의 맥어드레스를 가진 카메라가 연결된 포트번호를 찾는다.
 * @param mac 맥어드레스 문자열.
 * @return 해당하는 포트 번호, -1 - 없음.
 */
extern int get_interrupt_port(unsigned char* mac)
{
	int rtn = (-1);
	int port = 0;	
	
	rtn = nf_dev_switch_get_mac_portnum(mac, &port);
	if (port >= product_portnum || rtn == FALSE)
	{
		return (-1);
	}

	return port;
}

/**
 * @brief Switch register를 읽어 임의 맥어드레스를 가진 카메라가 연결된 포트번호를 찾는다.
 * @param mac 맥어드레스 문자열.
 * @return 해당하는 포트 번호, -1 - 없음.
 */
extern int get_interrupt_port_set_camport(unsigned char* mac)
{
	struct timespec now_time;
	static int check_time = 0;
	static unsigned char macaddr[8][6];

	int ret, i;

	clock_gettime(CLOCK_REALTIME, &now_time);

	/* per 1s */
	if (now_time.tv_sec != check_time)
	{
		check_time = now_time.tv_sec;
		ret = nf_dev_switch_get_port_mac(macaddr);
		if (!ret)
		{
			return (-1);
		}
	}

	for (i = 0; i < 8; i++)
	{
		if (!memcmp(macaddr[i], mac, 6))
			return i;
	}
	
	return (-1);
}

extern int is_extention_macaddr(unsigned char* mac)
{
	int rtn = (-1);
	int port = 0;	

#ifdef DUAL_LAN_NETWORK
	return 1;
#endif
	
	nf_dev_switch_get_mac_portnum(mac, &port);
	if (port == HUB_PORT)
	{
		return 1;
	}

	return 0;
}

static guint if_get_gateway(void)
{
    char buf[256];
    static char iface[256];
    unsigned int destination, gateway, flags, refcnt, use, metric, mask;
    int ret;

  	FILE* fp;
	fp = fopen("/proc/net/route", "r");

	g_return_val_if_fail ( fp != NULL , 0 );


    while (fgets(buf, 255, fp)) {
        if (!strncmp(buf, "Iface", 5))
            continue;

        ret = sscanf(buf, "%s\t%x\t%x\t%d\t%d\t%d\t%d\t%x",
                    iface, &destination, &gateway, &flags,
                    &refcnt, &use, &metric, &mask);

        if (ret != 8) {
            g_warning("%s ERROR: line read error\n", __FUNCTION__);
            continue;
        }

		if (!destination)								//MTU : 00000000
		{
			fclose(fp);
			return (gateway);
		}
    }

    fclose(fp);
	return 0;

}

static char _get_hex_from_char(char a)
{
	if (a >= '0' && a <= '9')
		return (a-'0');

	if (a >= 'a' && a <= 'f')
		return ((a-'a')+0xa);

	if (a >= 'A' && a <= 'F')
		return ((a-'A')+0xa);

	return (-1);
}

static void _get_mac_from_str(char* src, char* dst)
{
	int i=0;
	int j=0;
	char val[12];

	if (strlen(src) > 17)
	{
		memset(dst, 0x00, 6);
		return;
	}

	for (i=0; i<strlen(src); i++)
	{
		val[j] = _get_hex_from_char(src[i]);

		if (val[j] >= 0)
			j++;
	}

	dst[0] = (val[0]<<4) + val[1];
	dst[1] = (val[2]<<4) + val[3];
	dst[2] = (val[4]<<4) + val[5];
	dst[3] = (val[6]<<4) + val[7];
	dst[4] = (val[8]<<4) + val[9];
	dst[5] = (val[10]<<4) + val[11];
}

static int nf_swt_ctl_get_mac_from_ip(unsigned char *p_ipaddr, unsigned char *p_macaddr)
{
	char buf[1024] = { 0, };
	char macaddr[14] = { 0, };
	FILE *fp = NULL;
	int ret;

	// get mac address
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		return 0;

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		char buf_ipaddr[32] = { 0, };
		char buf_hw_type[32] = { 0, };
		char buf_flags[32] = { 0, };
		char buf_hw_address[32] = { 0, };
		char buf_mask[32] = { 0, };
		char buf_device[32] = { 0, };

		sscanf(buf, "%s\t%s\t%s\t%s\t%s\t%s",
				buf_ipaddr, buf_hw_type, buf_flags, buf_hw_address, buf_mask, buf_device);

		if (!strcmp(p_ipaddr, buf_ipaddr))
		{
			int i, j = 0;

			for (i = 0; i < strlen(buf_hw_address); i++)
				if (buf_hw_address[i] != ':')
					macaddr[j++] = buf_hw_address[i];

			_get_mac_from_str(macaddr, p_macaddr);

			fclose(fp);
			return 1;
		}

		memset(buf, 0x00, sizeof(buf));
	}

	fclose(fp);
	return 0;
}

static int nf_swt_ctl_check_arp_mac(unsigned char *p_macaddr)
{
	char buf[1024] = { 0, };
	char macaddr_str[14] = { 0, };
	unsigned char macaddr_ch[6] = { 0, };
	FILE *fp = NULL;
	int ret;

	// get mac address
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		return 0;

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		char buf_ipaddr[32] = { 0, };
		char buf_hw_type[32] = { 0, };
		char buf_flags[32] = { 0, };
		char buf_hw_address[32] = { 0, };
		char buf_mask[32] = { 0, };
		char buf_device[32] = { 0, };

		sscanf(buf, "%s\t%s\t%s\t%s\t%s\t%s",
				buf_ipaddr, buf_hw_type, buf_flags, buf_hw_address, buf_mask, buf_device);

		{
			int i, j = 0;

			for (i = 0; i < strlen(buf_hw_address); i++)
				if (buf_hw_address[i] != ':')
					macaddr_str[j++] = buf_hw_address[i];

			_get_mac_from_str(macaddr_str, macaddr_ch);

			if (!memcmp(macaddr_ch, p_macaddr, sizeof(macaddr_ch)))
			{
				fclose(fp);
				return 1;
			}
		}
	}

	fclose(fp);
	return 0;
}

typedef struct mac_array_entry
{
	unsigned char mac_addr[6];
} MAC_ARRAY_ENTRY;

extern void nf_swt_ctl_wan_insert_mac(unsigned char *p_ipaddr)
{
	static int is_init = 0;
	static GArray *mac_array = 0;
	static GMutex *swt_ctl_mutex = 0;
	static unsigned char fake_default_gw[32] = { 0, };

	struct mac_load_info _atu_info;
	MAC_ARRAY_ENTRY mac_array_entry;
	unsigned char gateway[32] = { 0, };

	int i, ret;

#ifndef _NVR_MODEL
	return;
#endif

	if (nf_get_running_mode() != 0)
		return;

	printf("[%s] start\n", __FUNCTION__);

	if (!is_init)
	{
		unsigned int addr = 0;
		addr = get_local_net_ip();
		snprintf(fake_default_gw, 16, "%d.%d.%d.%d", addr&0xff, (addr&0xff00)>>8, (addr&0xff0000)>>16, (addr&0xff000000)>>24);

		swt_ctl_mutex = g_mutex_new();
		mac_array = g_array_new(0, 0, sizeof(MAC_ARRAY_ENTRY));

		is_init = 1;
	}

	g_mutex_lock(swt_ctl_mutex);

	// purge
	{
		for (i = 0; i < mac_array->len; i++)
		{
			MAC_ARRAY_ENTRY entry;
			entry = g_array_index(mac_array, MAC_ARRAY_ENTRY, i);

			ret = nf_swt_ctl_check_arp_mac(entry.mac_addr);
			if (!ret)
			{
				_atu_info.port_num = NF_UTIL_SWITCH_PORT_WAN;
				
				memcpy(_atu_info.mac_addr, entry.mac_addr, sizeof(entry.mac_addr));
				ret = nf_dev_port_purge_mac_addr(_atu_info);
				if (!ret)
					printf("[%s][%d] Error : ioctl BOARD_PP_SMI_PURGE_PORT_MAC failed\n", __FUNCTION__, __LINE__);

				printf("[%s][%d] purge - macaddr : [%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__, __LINE__,
						_atu_info.mac_addr[0],_atu_info.mac_addr[1],_atu_info.mac_addr[2],_atu_info.mac_addr[3],_atu_info.mac_addr[4],_atu_info.mac_addr[5]);

				g_array_remove_index(mac_array, i);
				i--;
			}
		}
	}

	// find mac address
	ret = nf_swt_ctl_get_mac_from_ip(p_ipaddr, _atu_info.mac_addr);
	if (!ret)
	{
		// check gateway address
		unsigned int tmp;

		tmp = if_get_gateway();
		snprintf(gateway, 16, "%d.%d.%d.%d", tmp&0xff, (tmp&0xff00)>>8, (tmp&0xff0000)>>16, (tmp&0xff000000)>>24);

		if (!strcmp(gateway, fake_default_gw))
		{
			printf("[%s][%d] end\n", __FUNCTION__, __LINE__);
			g_mutex_unlock(swt_ctl_mutex);
			return;
		}

		ret = nf_swt_ctl_get_mac_from_ip(gateway, _atu_info.mac_addr);
		if (!ret)
		{
			printf("[%s][%d] end\n", __FUNCTION__, __LINE__);
			g_mutex_unlock(swt_ctl_mutex);
			return;
		}

		memcpy(mac_array_entry.mac_addr, _atu_info.mac_addr, sizeof(_atu_info.mac_addr));
	}
	else
	{
		memcpy(mac_array_entry.mac_addr, _atu_info.mac_addr, sizeof(_atu_info.mac_addr));
	}

	// insert
	{
		int is_match = 0;

		for (i = 0; i < mac_array->len; i++)
		{
			MAC_ARRAY_ENTRY entry;
			entry = g_array_index(mac_array, MAC_ARRAY_ENTRY, i);

			if (!memcmp(&entry, &mac_array_entry, sizeof(MAC_ARRAY_ENTRY)))
				is_match++;
		}

		if (!is_match)
		{
			g_array_append_val(mac_array, mac_array_entry);

#if defined(_IPX_0412VE3) || defined(_IPX_0824VE3) || defined(_IPX_1648VE3)
			_atu_info.port_num = 8;
#else
			_atu_info.port_num = 9;
#endif
			ret = nf_dev_port_insert_mac_addr(_atu_info);
			if (!ret)
				printf("[%s][%d] Error : ioctl BOARD_PP_SMI_LOAD_PORT_MAC failed\n", __FUNCTION__, __LINE__);

			printf("[%s][%d] insert - macaddr : [%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__, __LINE__,
					_atu_info.mac_addr[0],_atu_info.mac_addr[1],_atu_info.mac_addr[2],_atu_info.mac_addr[3],_atu_info.mac_addr[4],_atu_info.mac_addr[5]);
		}
	}

	printf("[%s][%d] end\n", __FUNCTION__, __LINE__);
	g_mutex_unlock(swt_ctl_mutex);
	return;
}

extern int peak_port_macaddr_table(int port, unsigned char* mac)
{
	int rtn = (-1);
	int port_num = 0;	
	
	rtn = nf_dev_switch_get_mac_portnum(mac, &port_num);
	if (port != port_num || rtn == FALSE)
	{
		return (-1);
	}

	return port;
}

/**
 * @brief 포트 번호에 맞는 내부 ip를 반환한다.
 * @param port 포트 번호.
 * @return Ip주소.
 */
extern unsigned int get_available_ip(int port)
{
	unsigned int class_c_ipbase = get_host_info() & get_host_netmask();

	int i = 0;
	unsigned short int ip_flag = 0;
	unsigned int ip_address_available = 0;
	char ip_buf[16];
    dtable *discovery = get_dtable();

#ifdef DUAL_LAN_NETWORK
    if(discovery[port].layer == IPCAM_DISC_LAYER_VHUB)
    {
		class_c_ipbase = get_hub_info() & get_host_netmask();
    }
#endif

	/* change static ip_to_set to local */
	//ip_to_set = (ip_to_set+1) % 200;
	int ip_to_set = port;

	//ip_address_available = class_c_ipbase | ((port + 10) << 24);
	ip_address_available = class_c_ipbase | ((ip_to_set+10) << 24);

	return ip_address_available;
}

/**
 * @brief Video loss Que를 초기화 및 관리 쓰레드를 생성한다.
 */
extern void ipcam_vloss_queue_init(void)
{
	vloss_queue = g_async_queue_new();
	pthread_create(&vloss_th, NULL, (void*)&ipx_vloss_handler_func, NULL);
}

/**
 * @brief PND 루틴을 초기화한다.
 * @param obj MrtpPipeObj 핸들.
 * @param channel_num 최대 채널 갯수.
 * @return 1 - 성공, 0 - 실패.
 */
extern int plug_and_detect_init(GobjMrtpPipe *obj, int channel_num)
{
	int i,j;
	h_iplive = obj;

	IPCAM_DBG(MAJOR, "start Device detection start(%d-CH)\n", channel_num);
	if (channel_num == 4 || channel_num == 8)
	{
		product_portnum = channel_num;
		external_portnum = 0;
	}
	else if (channel_num == AVAILABLE_MAX_CH)
	{
		#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
			product_portnum = 16;
			external_portnum = 0;
		#else
			product_portnum = 8;
			external_portnum = 8;
		#endif
	}
	else
	{
		IPCAM_DBG(WARN, "channel number is wrong(%d)\n", channel_num);
		return 0;
	}

	for (i = 0; i< AVAILABLE_MAX_CH; i++)
	{
		for (j =0; j<NF_IPCAM_TYPE_MAX; j++)
		{
			runtime[i].sys.ssl[j] = NULL;
			runtime[i].sys.ctx[j] = NULL;
			pthread_mutex_init(&runtime[i].sys.ssl_mtx[j], NULL);
			runtime[i].sys.ssl_state[j] = IPCAM_SSL_NOT_AVAILABLE;
		}
		runtime[i].sys.ssl_g = NULL;
		runtime[i].sys.ctx_g = NULL;
	}

	/* init switch device */
	_init_switch_device();

	tv_system = nf_sysdb_get_bool("sys.info.sig_type");
	memset((void*)runtime, 0x00, sizeof(mtable) * AVAILABLE_MAX_CH);
	//memset((void*)nf_rtsp_errno, NF_RTSP_ERR_NO_ERR, sizeof(int) * AVAILABLE_MAX_CH * NF_IPCAM_STREAM_MAX);
	memset((void*)nf_pnd_status, PND_TYPE_UNPLUGGED, sizeof(int) * AVAILABLE_MAX_CH);
	memset((void*)nf_pnd_recovery_cnt, 0x00, sizeof(int) * AVAILABLE_MAX_CH);
	portscan_trig = 0;

	g_thread_init(NULL);
	event_queue = g_async_queue_new();
	recevt_queue = g_async_queue_new();

	ipcam_vloss_queue_init();

	/* Register command callback */
	nmf_mrtp_pipe_set_cmd_callback(obj, &ipx_cmd_callback_func, NULL);

	nf_ipcam_discovery_init();
	_cam_manager_init();
	vhub_manager_init();

	//cam_setup_init();
	//nf_onvif_soap_init();
	cam_onvif_init();
#if AUTO_ONVIF_CAM_CONNECTION
	nf_ipcam_dhcpd_init();
#endif

	_cam_event_init();
	nf_ipcam_event_notification_server();

	pthread_create(&recovery_th, NULL, (void*)&pnd_recovery_func, NULL);

#if 0
	{
		IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
		g_async_queue_push(vloss_queue, evt);
	}
	IPCAM_DBG(MINOR, "vloss state set 0000ffff\n");
#endif

	SSL_library_init();

	IPCAM_DBG(MAJOR, "end Device detection module initialized\n");
	return (1);
}

/**
 * @brief PND루틴을 초기화하지 않고 진행한다.(KMW)
 * @param obj MrtpPipeObj 핸들.
 * @return 항상 1.
 */
extern int no_pnd_init(GobjMrtpPipe *obj)
{
	int i;
	h_iplive = obj;

	IPCAM_DBG(MAJOR, "start\n");
	#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
		product_portnum = 16;
	#else
		product_portnum = 8;
	#endif

	tv_system = nf_sysdb_get_bool("sys.info.sig_type");
	memset((void*)runtime, 0x00, sizeof(mtable) * AVAILABLE_MAX_CH);
	//memset((void*)nf_rtsp_errno, NF_RTSP_ERR_NO_ERR, sizeof(int) * AVAILABLE_MAX_CH * NF_IPCAM_STREAM_MAX);

	g_thread_init(NULL);
	event_queue = g_async_queue_new();
	recevt_queue = g_async_queue_new();

	_cam_event_init();
	ipcam_vloss_queue_init();

	/* Register command callback */
	nmf_mrtp_pipe_set_cmd_callback(obj, &ipx_cmd_callback_func, NULL);

	_cam_manager_init();
	pthread_create(&recovery_th, NULL, (void*)&pnd_recovery_func, NULL);

	IPCAM_DBG(MAJOR, "end Cam(No PND) module initialized\n");
	return (1);
}

extern mtable* get_runtime(void)
{
	return runtime;
}

extern GAsyncQueue* get_queue(void)
{
	return (event_queue);
}

extern void new_ds_queue(void)
{
	// OPEN mode only
	recevt_queue = g_async_queue_new();
	pthread_create(&rate_th, NULL, (void*)&ipx_delayed_setup_handler_func, NULL);
}

extern GAsyncQueue* get_recevt_queue(void)
{
	return (recevt_queue);
}

extern GAsyncQueue* get_vloss_queue(void)
{
	return (vloss_queue);
}

extern int* get_portscan_trigger(void)
{
	return (&portscan_trig);
}

extern unsigned int get_vloss_status(void)
{
	return (vloss_status);
}

extern unsigned int *get_vloss_status_ptr(void)
{
	return (&vloss_status);
}

extern int get_pnd_status(int ch_num)
{
	return (nf_pnd_status[ch_num]);
}

/**
 * @brief 임의의 IPX_PND_EVENT를 생성한다.
 * @param event_type 생성하고자 하는 이벤트 유형.
 * @return IPX_PND_EVENT 객체. 반드시 ipx_ipcam_free_event를 호출해야 함.
 */
extern IPX_PND_EVENT* ipx_ipcam_new_event(int event_type)
{
	IPX_PND_EVENT* evt = NULL;

	g_return_val_if_fail(event_type < IPCAM_EVENT_TYPE_MAX, NULL);
	evt = g_malloc0(sizeof(IPX_PND_EVENT));
	g_return_val_if_fail(evt != NULL, NULL);
	evt->type = event_type;
	g_get_current_time(&evt->ctime);

	return evt;
}

extern int plug_and_detect_restart(void)
{
	int i = 0;

	IPCAM_DBG(MAJOR, "start\n");
	memset((void*)runtime, 0x00, sizeof(mtable) * AVAILABLE_MAX_CH);
	//memset((void*)nf_rtsp_errno, NF_RTSP_ERR_NO_ERR, sizeof(int) * AVAILABLE_MAX_CH * NF_IPCAM_STREAM_MAX);
	memset((void*)nf_pnd_status, PND_TYPE_UNPLUGGED, sizeof(int) * AVAILABLE_MAX_CH);
	portscan_trig = 10;
	//ip_to_set = 0;

	set_sub_network();
	nf_ipcam_zmq_server_start();
	IPCAM_DBG(MAJOR, "end\n");

	return (1);
}

extern void pnd_init_status(void)
{
	memset((void*)nf_pnd_status, PND_TYPE_UNPLUGGED, sizeof(int) * AVAILABLE_MAX_CH);
}

// If dvr network set dhcp and there is no dhcp server, default gw of dvr's network is empty.
// Then, dvr don't send broadcasting packets.
// If number of default gw is two or more, ddns and e-mail send is not working.
// So dvr must be only one default gw always.
extern void check_set_network_for_discovery(void)
{
	static char ipstr[16] = {0, };

	unsigned int gate = 0;
	unsigned int addr = 0;
	unsigned int count_default_gw = 0;
	char system_cmd_str[256];

	// printf("[%s] Start\n", __FUNCTION__);

	count_default_gw = get_host_gateway();

	if (count_default_gw == 0) {
		// Setting fake default gw.
		addr = get_local_net_ip();
		if (addr == 0) { return; }
		snprintf(ipstr, 16, "%d.%d.%d.%d", addr&0xff, (addr&0xff00)>>8, (addr&0xff0000)>>16, (addr&0xff000000)>>24);
		snprintf(system_cmd_str, 256, "route add default gw %s", ipstr);
		IPCAM_DBG(MINOR, "proxy_system(%s)\n", system_cmd_str);
		proxy_system(system_cmd_str, 1, 3);
	}
	else if (count_default_gw == 2) {
		// Delete fake default gw.
		snprintf(system_cmd_str, 256, "route del default gw %s", ipstr);
		IPCAM_DBG(MINOR, "proxy_system(%s)\n", system_cmd_str);
		proxy_system(system_cmd_str, 1, 3);
		memset(ipstr, 0x00, sizeof(ipstr));
	}
	// printf("[%s] End\n", __FUNCTION__);
}

extern unsigned int get_init_info(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//

	unsigned int addr, netmask;


	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t) ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, HUB_ETH_DEVICE))
		{
			ifr++;
			continue;
		}
		addr = myaddr->sin_addr.s_addr;

		ioctl(dummy_sock, SIOCGIFNETMASK, (char*)ifr);
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;
		netmask = myaddr->sin_addr.s_addr;

		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return (addr & netmask);
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

extern unsigned int get_local_net_ip(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//

	unsigned int addr, netmask;


	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t) ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, LOCAL_ETH_DEVICE))
		{
			ifr++;
			continue;
		}
		addr = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return (addr);
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}


extern unsigned int get_host_info(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//
	unsigned int host_addr = 0;


#if 0
	if (host_addr != 0)
		return host_addr;
#endif

	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, LOCAL_ETH_DEVICE))
		{
			ifr++;
			continue;
		}

		host_addr = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return host_addr;
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

#ifdef DUAL_LAN_NETWORK
extern unsigned int get_hub_info(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//
	unsigned int host_addr = 0;


#if 0
	if (host_addr != 0)
		return host_addr;
#endif

	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, HUB_ETH_DEVICE))
		{
			ifr++;
			continue;
		}

		host_addr = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return host_addr;
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

extern unsigned int get_bridge_info(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//
	unsigned int host_addr = 0;

	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if(nf_get_dual_lan_mode())
		{
			if (strcmp(ifr->ifr_name, HUB_ETH_DEVICE))
			{
				ifr++;
				continue;
			}
		}
		else
		{
			if (strcmp(ifr->ifr_name, "br0"))
			{
				ifr++;
				continue;
			}
		}

		host_addr = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return host_addr;
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}
#endif

extern unsigned int get_host_gateway(void)
{
	FILE *fp = NULL;
	char buf_in[256];

	char iface[32];
	char dst[32];
	char gate[32];
	char flag[32];
	char refcnt[32];
	char use[32];
	char metric[32];
	char mask[32];
	char mtu[32];
	char window[32];
	char irtt[32];
	unsigned int host_gateway = 0;


#if 0
	if (host_gateway != 0)
		return host_gateway;
#endif

	if ((fp = fopen("/proc/net/route", "r")) == NULL)
	{
		return 0;
	}

	while (fgets(buf_in, 256, fp) != NULL)
	{
		memset(iface, 0x00, 32);
		memset(dst, 0x00, 32);
		memset(gate, 0x00, 32);
		memset(flag, 0x00, 32);
		memset(refcnt, 0x00, 32);
		memset(use, 0x00, 32);
		memset(metric, 0x00, 32);
		memset(mask, 0x00, 32);
		memset(mtu, 0x00, 32);
		memset(window, 0x00, 32);
		memset(irtt, 0x00, 32);

		sscanf(buf_in, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
			iface, dst, gate, flag, refcnt, use, metric, mask, mtu, window, irtt);

		if (strncmp(HUB_ETH_DEVICE, iface, 4))
		{
			continue;
		}

		if (strncmp("00000000", dst, 8) == 0)
			++host_gateway;

		memset(buf_in, 0x00, 256);
	}

	fclose(fp);
	return host_gateway;
}

extern unsigned int get_host_netmask(void)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//
	unsigned int host_subnet = 0;


#if 0
	if (host_subnet != 0)
		return host_subnet;
#endif

	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, LOCAL_ETH_DEVICE))
		{
			ifr++;
			continue;
		}

		ioctl(dummy_sock, SIOCGIFNETMASK, (char*)ifr);
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		host_subnet = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return host_subnet;
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

extern unsigned int get_netif_ip(const char* _devname)
{
	struct ifreq *ifr;
	struct sockaddr_in *myaddr;
	struct sockaddr *sa;
	struct ifconf ifcfg;
	int dummy_sock;
	int n;
	int data_port;
	const int numreqs = 30;
	unsigned int host_addr = 0;
	unsigned int addr = 0;


	if (_devname == NULL) { return 0; }
	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, _devname))
		{
			ifr++;
			continue;
		}
		addr = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return (addr);
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

extern unsigned int get_netif_mask(const char* _devname)
{
	struct ifreq *ifr;				//
	struct sockaddr_in *myaddr;		//
	struct sockaddr *sa;			//
	struct ifconf ifcfg;			// to get my ip address from ethX devices
	int dummy_sock;					//
	int n;							//
	int data_port;					//
	const int numreqs = 30;			//
	unsigned int host_subnet = 0;


#if 0
	if (host_subnet != 0)
		return host_subnet;
#endif

	memset(&ifcfg, 0x00, sizeof(ifcfg));

	dummy_sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc((size_t)ifcfg.ifc_len);

	if (ioctl(dummy_sock, SIOCGIFCONF, (char*)&ifcfg) < 0)
	{
		perror("SIOCGIFCONF");
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return 0;
	}
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n+=sizeof(struct ifreq))
	{
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		if (strcmp(ifr->ifr_name, _devname))
		{
			ifr++;
			continue;
		}

		ioctl(dummy_sock, SIOCGIFNETMASK, (char*)ifr);
		myaddr = (struct sockaddr_in *) &ifr->ifr_addr;

		host_subnet = myaddr->sin_addr.s_addr;
		close(dummy_sock);
		if (ifcfg.ifc_buf != NULL)
		{
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return host_subnet;
	}

	close(dummy_sock);
	if (ifcfg.ifc_buf != NULL)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}
	return 0;
}

/**
 * @brief 유형별 SSL 객체를 종료한다.
 * @param ch 채널 번호.
 * @param type Job 유형.
 */
extern void _ipcam_ssl_close(int ch, int type)
{
	int len = 0;
	pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[type]);
	if (runtime[ch].sys.ssl_state[type] == IPCAM_SSL_NOT_AVAILABLE)
	{
		pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[type]);
		return;
	}
	runtime[ch].sys.ssl_state[type] = IPCAM_SSL_SHUTDOWN;
	len = SSL_shutdown(runtime[ch].sys.ssl[type]);
	if (len == 0)
	{
		usleep(10*1000);
		len = SSL_shutdown(runtime[ch].sys.ssl[type]);
	}
	pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[type]);

	IPCAM_DBG(MINOR, "SSL close OK - CH(%d) type(%s)\n", ch, ipcam_get_type_str(type));
}

/**
 * @brief 카메라 연결에 사용한 소켓 및 SSL 객체들을 초기화한다.
 * @param sock 소켓.
 * @param sock_buf 버퍼.
 * @param ssl SSL 객체 포인터.
 * @param ctx SSL_CTX 객체 포인터.
 */
extern void _release_resource(int *sock, char* sock_buf, SSL **ssl, SSL_CTX **ctx)
{
	if (ssl != NULL && *ssl != NULL)
	{
		int sd = SSL_get_fd(*ssl);
		if (sd > -1)
		{
			SSL_set_quiet_shutdown(*ssl, 1);
			//printf("[%s] SSL_shutdown\n", __FUNCTION__);
			SSL_shutdown(*ssl);						
		}		

		//printf("[%s] SSL_free\n", __FUNCTION__);
		SSL_free(*ssl);
		*ssl = NULL;
	}
	if (ctx != NULL && *ctx != NULL)
	{
		//printf("[%s] SSL_CTX_free\n", __FUNCTION__);
		SSL_CTX_free(*ctx);
		*ctx = NULL;
	}
	if (sock != NULL && *sock > 0)
	{
		//printf("[%s] sock(%d) close\n", __FUNCTION__, *sock);
		close(*sock);
		*sock = 0;
	}
	if (sock_buf != NULL)
	{
		//printf("[%s] sock_buf free\n", __FUNCTION__);
		free(sock_buf);
	}
	//printf("[%s] end\n", __FUNCTION__);
}

extern int nf_ipcam_is_vendor_s1(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcmp(p, "S1") == 0)
	{
		return 1;
	}

	return 0;
}
extern int nf_ipcam_is_vendor_orion(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcmp(p, "ORION") == 0)
	{
		return 1;
	}

	return 0;
}
extern int nf_ipcam_is_vendor_g4s(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcmp(p, "G4S") == 0)
	{
		return 1;
	}

	return 0;
}
extern int nf_ipcam_is_vendor_asp(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcmp(p, "ASP") == 0)
	{
		return 1;
	}

	return 0;
}
extern int nf_ipcam_is_vendor_videcon(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }

	//if (strcmp(p, "VIDECON") == 0)
    if (strstr(p, "VIDECON") != NULL) 
	{
		return 1;
	}

	return 0;
}
extern int nf_ipcam_is_vendor_dayou(void)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }

	if (strcmp(p, "DAYOU") == 0)
	{
		return 1;
	}

	return 0;
}

extern int nf_ipcam_is_vendor_zicom(void)
{
	gchar *p;
	
	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcmp(p, "ZICOM") == 0 )
	{
		return 1;
	}

	return 0;
}

/**
 * @brief NVR의 벤더를 체크한다.
 * @param vendor 체크하고자 하는 벤더명(S1, CBC, ..)
 * @return 동일하면 1, 다르면 0.
 */
extern int nf_ipcam_is_vendor(const char* vendor)
{
	gchar *p;

	p = nf_sysdb_get_str_nocopy("sys.info.vendor");
	if (p == NULL) { return 0; }
	if (strcasecmp(p, vendor) == 0)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief Discovery 쓰레드를 실행시킨다.
 */
static void nf_ipcam_discovery_init(void)
{
	pthread_create(&discovery_th, NULL, (void*)&ipcam_discovery_main_loop, NULL);
	pthread_create(&discovery_rcv_th, NULL, (void*)&ipcam_discovery_recv_loop, NULL);
#if AUTO_ONVIF_CAM_CONNECTION
	pthread_create(&discovery_onvif_th, NULL, (void*)&ipcam_discovery_onvif_loop, NULL);
	pthread_create(&discovery_onvif_rcv_th, NULL, (void*)&ipcam_discovery_onvif_recv_loop, NULL);
#endif
}

/**
 * @brief 특정 벤더에 대해 이벤트 수신 쓰레드를 실행시킨다.
 */
extern void nf_ipcam_event_notification_server(void)
{
	if (nf_sysman_get_fwver_vendor() == 108) // Grundig
	{
		pthread_create(&noti_th, NULL, (void*)&ipx_event_noti_server, NULL);
	}
	//pthread_create(&xm_noti_th, NULL, (void*)&ipx_xm_event_noti_server, NULL);
#if 0
	if(nf_ipcam_is_vendor_s1())
	{
		pthread_create(&s1_event_th, NULL, (void *)&onvif_event_puller, NULL);
	}
#endif
}

/**
 * @brief 카메라 셋업 루틴을 초기화한다.
 */
static void _cam_manager_init(void)
{
	ipcam_manager(product_portnum);
}

/**
 * @brief 카메라 이벤트 처리 쓰레드를 실행한다.
 */
static void _cam_event_init(void)
{
	pthread_create(&event_th, NULL, (void*)&ipx_event_handler_func, NULL);
	pthread_create(&rate_th, NULL, (void*)&ipx_delayed_setup_handler_func, NULL);
}

#if 0
static void ipx_ip_manager(void *param)
{
	int *ch_num = (int*) param;
	ipaddr_manager(*ch_num);
}
#endif

/**
 * @brief 이벤트 수신 쓰레드 함수.
 */
static void ipx_event_noti_server(void *param)
{
	event_notification_server();
}

static void ipx_xm_event_noti_server(void *param)
{
	xm_event_notification_server();
}

static int try_http_ports(int ch)
{
	int rtn = 0;
	int sock = (-1);
	struct sockaddr_in sin;
	cam_model_info info;


	if (net_info == NULL)
	{
		return 0;
	}

	strncpy(runtime[ch].username, net_info[ch].username, 64);
	strncpy(runtime[ch].password, net_info[ch].password, 64);
	runtime[ch].sys.ipaddr = net_info[ch].ip_addr;
	strncpy(runtime[ch].sys.rtsp_url[0], net_info[ch].rtsp_addr[0], 256);
	strncpy(runtime[ch].sys.rtsp_url[1], net_info[ch].rtsp_addr[1], 256);
	strncpy(runtime[ch].sys.rtsp_url[2], net_info[ch].rtsp_addr[2], 256);
	runtime[ch].sys.rtsp_port[0] = net_info[ch].rtsp_port[0];
	runtime[ch].sys.rtsp_port[1] = net_info[ch].rtsp_port[1];
	runtime[ch].sys.rtsp_port[2] = net_info[ch].rtsp_port[2];
	runtime[ch].sys.http_port = net_info[ch].http_port;
	runtime[ch].sys.model_code = net_info[ch].model_code;

	IPCAM_DBG(MINOR, "Trying........ CH(%d) %d.%d.%d.%d:%d\n",
			ch,
			runtime[ch].sys.ipaddr&0xff,
			(runtime[ch].sys.ipaddr&0xff00)>>8,
			(runtime[ch].sys.ipaddr&0xff0000)>>16,
			(runtime[ch].sys.ipaddr&0xff000000)>>24,
			runtime[ch].sys.http_port);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = runtime[ch].sys.ipaddr;
	sin.sin_port = htons(runtime[ch].sys.http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
	{
		struct timeval tv;
		tv.tv_sec = 1; // 1 Secs Timeout
		tv.tv_usec = 0;
		int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "CH(%d) limiting waiting time failed (%d)\n", ch, ret);
			close(sock);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d) connect failed\n", ch);
		perror("connect"); 
		close(sock);
		sock = (-1);
		return (-1);
	}

	close(sock);
	rtn = 1;

	return rtn;
}

/* KMW Project IPCAM Address PRESET

2012-08-13 오후 10:14:19 choissi

// for 4ch

A_200 TYPE : MASTER 4, SLAVE : 0										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER	192	168	200	200	192.168.10.1	192.168.10.2	8080	1550	9080
2	MASTER	192	168	200	201	192.168.10.1	192.168.10.2	8081	1551	9081
3	MASTER	192	168	200	202	192.168.10.1	192.168.10.2	8082	1552	9082
4	MASTER	192	168	200	203	192.168.10.1	192.168.10.2	8083	1553	9083
										
										
B_200 TYPE : MASTER 1, SLAVE : 3										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	SLAVE3	192	168	100	13	192.168.10.1	192.168.10.2	8083	1553	9083
										
										
C_200 TYPE : MASTER 2, SLAVE : 2										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	MASTER2	192	168	200	201	192.168.101.1	192.168.101.2	8082	1552	9082
4	SLAVE2	192	168	100	11	192.168.10.1	192.168.10.2	8083	1553	9083
										
										
D_200 TYPE : MASTER 2, SLAVE : 2										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	MASTER2	192	168	200	201	192.168.101.1	192.168.101.2	8083	1553	9083


// for 8ch

A_200 TYPE : MASTER 8, SLAVE : 0										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER	192	168	200	200	192.168.10.1	192.168.10.2	8080	1550	9080
2	MASTER	192	168	200	201	192.168.10.1	192.168.10.2	8081	1551	9081
3	MASTER	192	168	200	202	192.168.10.1	192.168.10.2	8082	1552	9082
4	MASTER	192	168	200	203	192.168.10.1	192.168.10.2	8083	1553	9083
5	MASTER	192	168	200	204	192.168.10.1	192.168.10.2	8084	1554	9084
6	MASTER	192	168	200	205	192.168.10.1	192.168.10.2	8085	1555	9085
7	MASTER	192	168	200	206	192.168.10.1	192.168.10.2	8086	1556	9086
8	MASTER	192	168	200	207	192.168.10.1	192.168.10.2	8087	1557	9087

										
B_200 TYPE : MASTER 1, SLAVE : 7										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	SLAVE3	192	168	100	13	192.168.10.1	192.168.10.2	8083	1553	9083
5	SLAVE4	192	168	100	14	192.168.10.1	192.168.10.2	8084	1554	9084
6	SLAVE5	192	168	100	15	192.168.10.1	192.168.10.2	8085	1555	9085
7	SLAVE6	192	168	100	16	192.168.10.1	192.168.10.2	8086	1556	9086
8	SLAVE7	192	168	100	17	192.168.10.1	192.168.10.2	8087	1557	9087

																				
C_200 TYPE : MASTER 2, SLAVE : 6										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	SLAVE3	192	168	100	13	192.168.10.1	192.168.10.2	8083	1553	9083
5	MASTER2	192	168	200	201	192.168.100.1	192.168.100.2	8084	1554	9084
6	SLAVE4	192	168	100	11	192.168.10.1	192.168.10.2	8085	1555	9085
7	SLAVE5	192	168	100	12	192.168.10.1	192.168.10.2	8086	1556	9086
8	SLAVE6	192	168	100	13	192.168.10.1	192.168.10.2	8087	1557	9087
										
										
D_200 TYPE : MASTER 2, SLAVE : 6										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	SLAVE3	192	168	100	13	192.168.10.1	192.168.10.2	8083	1553	9083
5	SLAVE4	192	168	100	13	192.168.10.1	192.168.10.2	8084	1554	9084
6	MASTER2	192	168	200	201	192.168.100.1	192.168.100.2	8085	1555	9085
7	SLAVE5	192	168	100	11	192.168.10.1	192.168.10.2	8086	1556	9086
8	SLAVE6	192	168	100	12	192.168.10.1	192.168.10.2	8087	1557	9087
										
										
E_200 TYPE : MASTER 3, SLAVE : 5										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	SLAVE2	192	168	100	12	192.168.10.1	192.168.10.2	8082	1552	9082
4	MASTER2	192	168	200	201	192.168.100.1	192.168.100.2	8083	1553	9083
5	SLAVE3	192	168	100	11	192.168.10.1	192.168.10.2	8084	1554	9084
6	SLAVE4	192	168	100	12	192.168.10.1	192.168.10.2	8085	1555	9085
7	MASTER3	192	168	200	202	192.168.100.1	192.168.100.2	8086	1556	9086
8	SLAVE5	192	168	100	11	192.168.10.1	192.168.10.2	8087	1557	9087
										
										
F_200 TYPE : MASTER 4, SLAVE : 4										
										
NO	MODE	WAN				LAN	CAMERA	HTTP Port	RTSP Port	AP_Http_Port
1	MASTER1	192	168	200	200	192.168.100.1	192.168.100.2	8080	1550	9080
2	SLAVE1	192	168	100	11	192.168.10.1	192.168.10.2	8081	1551	9081
3	MASTER2	192	168	200	201	192.168.100.1	192.168.100.2	8082	1552	9082
4	SLAVE2	192	168	100	11	192.168.10.1	192.168.10.2	8083	1553	9083
5	MASTER3	192	168	200	202	192.168.100.1	192.168.100.2	8084	1554	9084
6	SLAVE3	192	168	100	11	192.168.10.1	192.168.10.2	8085	1555	9085
7	MASTER4	192	168	200	203	192.168.100.1	192.168.100.2	8086	1556	9086
8	SLAVE4	192	168	100	11	192.168.10.1	192.168.10.2	8087	1557	9087


<item key="sys.info.swmode" type="UINT" val="0" />

# GUI 변경 사항 

SYSTEM > SYSTEM management에서  업체 코드가 KMW일 때 ( 정해야 함. 생성해 주세요.) , 활성화 됨 
항목 이름 :
Switch Operation Mode :  

항목의 값 리스트 : 
0: Close Mode 
1: A Type Mode
2: B Type Mode
3: C Type Mode
4: D Type Mode
5: E Type Mode
5: F Type Mode
9999: Open Mode  ( 지금은 GUI적으로 표출하지 않음 )

*/


typedef enum _NF_IPCAM_ADDR_PRESET {
	NF_IPCAM_ADDR_PRESET_CLOSE = 0,
	NF_IPCAM_ADDR_PRESET_A = 1,
	NF_IPCAM_ADDR_PRESET_B = 2,
	NF_IPCAM_ADDR_PRESET_C = 3,
	NF_IPCAM_ADDR_PRESET_D = 4,
	NF_IPCAM_ADDR_PRESET_E = 5,
	NF_IPCAM_ADDR_PRESET_F = 6,
	NF_IPCAM_ADDR_PRESET_OPEN = 9999	
} NF_IPCAM_ADDR_PRESET;           


/**
 * @brief Video Loss 이벤트 처리 쓰레드.
 */
static void ipx_vloss_handler_func(void)
{
	IPX_PND_EVENT *evt = NULL;

	while (1)
	{
		evt = g_async_queue_pop(vloss_queue);

		switch (evt->type)
		{
			case IPCAM_VLOSS_ADD_NEW_CH:
				vloss_status &= ~(1<<(evt->port));
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_VLOSS_REMOVE_CH:
				vloss_status |= (1<<(evt->port));
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_VLOSS_REMOVE_ALL:
				vloss_status = 0xffffffff&NUM_ACTIVE_CH_MASK;
				ipx_ipcam_free_event(evt);
				break;
			default:
				ipx_ipcam_free_event(evt);
				break;
		}
	}
}

/**
 * @brief 카메라 bitrate 조절 이벤트 처리 쓰레드.
 *
 * S1향에서만 사용한다.
 * modification => SET_ENCODER added, non-S1 vendor use this (2014-10-23)
 *
 * @see nf_ipcam_put_recorded_fcnt
 */
static void ipx_delayed_setup_handler_func(void)
{
	IPX_PND_EVENT *evt = NULL;
	gchar buf[4096];

	while (1)
	{
		evt = g_async_queue_pop(recevt_queue);

		switch (evt->type)
		{
			case IPCAM_DS_RATE_DOWN:
				nf_ipcam_rate_down(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_DS_RATE_RECOVER:
				nf_ipcam_rate_recover(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_DS_SET_ENCODER:
			{
				NF_RECEVT_SET_ENCODER_CMD_PARAM_T *param = evt->p.ptr;
				nf_ipcam_set_rec_vcodec_thread(
						param->change_mask,
						param->fps1, param->fps2,
						param->qual1, param->qual2);
				free(param);
				ipx_ipcam_free_event(evt);
				break;
			}
			case IPCAM_DS_MOTION:
			{
				gint ch = evt->port;
				NFIPCamSetupMotionArea *info = evt->p.ptr;
				nf_ipcam_set_motion_area(ch, info, NULL, NULL, NULL);
				free(evt->p.ptr);
				ipx_ipcam_free_event(evt);
				break;
			}
			case IPCAM_DS_XM_EVENT:
			{
				gint csock = evt->port;
				gint pos = 0;
				gint clen = 0;

				ipx_ipcam_free_event(evt);
				pos = 0;
				memset(buf, 0x00, 4096);
				do
				{
					clen = recv(csock, buf+pos, 4096-pos, 0);
					if (clen < 0)
					{
						printf("XM event processing - recv error below\n");
						perror("recv");
						usleep(1000*10);
						continue;
					}
					else if (clen == 0)
					{
						//printf("server closed\n");
						close(csock);
						break;
					}

					pos += clen;
				} while(1);

				xiongmai_event_notification_handler(&buf[0x14]);
				break;
			}
			default:
				ipx_ipcam_free_event(evt);
				break;
		}
	}
}

/**
 * @brief 카메라 상태변경 이벤트 처리 쓰레드.
 *
 * 카메라 연결, 접속 및 접속 해제 등 대부분의 처리를 여기서 한다.
 */
static void ipx_event_handler_func(void)
{
	IPX_PND_EVENT *evt = NULL;

	while (1)
	{
		evt = g_async_queue_pop(event_queue);

		switch (evt->type)
		{
			case IPCAM_EVENT_LINK_CHANGED:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_LINK_CHANGED\n");
				if (portscan_trig < 10) { portscan_trig += 3; }
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_ONVIF_PRIME_READY:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_ONVIF_PRIME_READY(CH(%d))\n", evt->port);
				onvif_supported_device_handler(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_ONVIF_GENERAL_READY:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_ONVIF_GENERAL_READY(CH(%d))\n", evt->port);
				onvif_device_handler(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_DELAY_TO_CONN:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_DELAY_TO_CONN(CH(%d))\n", evt->port);
				runtime[evt->port].next_event = evt->d.params[0];
				runtime[evt->port].delayed = evt->d.params[1];
				//runtime[evt->port].delayed = 10;
				//runtime[evt->port].next_event = evt->d.params[0];
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_DELAY_READY:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_DELAY_READY(CH(%d))\n", evt->port);
				device_initializer_reg(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_DEVICE_READY:
				runtime[evt->port].state = (MGMT_STATE_LINKED|MGMT_STATE_READY);
				nf_pnd_status[evt->port] = PND_TYPE_IP_DONE;
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_DEVICE_READY(CH(%d))\n", evt->port);
				discovered_device_handler_reg(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_NVS_SUBCH_READY:
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_PLUGGED, __LINE__, __FILE__);
				nf_pnd_prog_notify_fire(evt->port, 5, __LINE__, __FILE__);

				nf_pnd_status[evt->port] = PND_TYPE_IP_DONE;
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_NVS_SUBCH_READY(CH(%d)])\n", evt->port);
				nvs_device_handler(evt->port);
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_NVS_SUBCH_CLOSE:
			{
				gint ch = 0;
				gint i = 0;
				NFIPCamPortStatus port_status;

				ch = evt->port;
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_NVS_SUBCH_CLOSE(CH(%d))\n", ch);
				ipx_ipcam_free_event(evt);

				nmf_mrtp_pipe_close_ch(h_iplive, ch);

				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
					evt->port = ch;
					g_async_queue_push(vloss_queue, evt);
				}

				IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
						ch, runtime[ch].state);
				for (i=0; i<NF_IPCAM_TYPE_MAX; i++)
				{
					nf_ipcam_setup_clear(ch, i);
					pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[i]);
					runtime[ch].sys.ssl_state[i] = IPCAM_SSL_NOT_AVAILABLE;
					_release_resource(NULL, NULL, &runtime[ch].sys.ssl[i], &runtime[ch].sys.ctx[i]);
					pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[i]);
				}
				_release_resource(NULL, NULL, &runtime[ch].sys.ssl_g, &runtime[ch].sys.ctx_g);

				runtime[ch].state = MGMT_STATE_UNLINKED;
				runtime[ch].sys.nvs_stream_start = 0;

				memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
				nf_ipcam_set_port_status(ch, &port_status, NULL);
				nf_pnd_evt_notify_fire(ch, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
				nf_pnd_status[ch] = PND_TYPE_UNPLUGGED;

#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "NVS_SUBCH_CLOSE : %d", ch);
					nf_eventlog_put_param(&tval, LT_IPCAM, ch, LP2_IPCAM_DEVICE_OUT, log_buf);
				}
#endif
				break;
			}
			case IPCAM_EVENT_DEVICE_OUT:
			{
				gint ch = 0;
				gint i = 0;
				NFIPCamPortStatus port_status;

				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_DEVICE_OUT(CH(%d))\n", evt->port);
				ch = evt->port;
				ipx_ipcam_free_event(evt);

				runtime[ch].delayed = 0;
				runtime[ch].next_event = 0;

				// Close all nvs sub chs.
				nvs_close_all_sub_ch(ch);

				nmf_mrtp_pipe_close_ch(h_iplive, ch);

				// ONVIF 카메라 GENERAL MOTION type인 경우 메모리 초기화
				if (runtime[ch].motion.method == MAM_GENERAL)
				{
					DispmuxMotionCbParam p;
					char *sb = NULL;
					p.ch=ch;
					p.width = runtime[ch].motion.block_width;
					p.height = runtime[ch].motion.block_height;
					sb = malloc(p.width*p.height);
					memset(sb,0x00,(size_t)p.width*p.height);
					nf_ipcam_general_motion_cb_func(&p, sb);
					free(sb);
				}

				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
					evt->port = ch;
					g_async_queue_push(vloss_queue, evt);
				}

				IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
						ch, runtime[ch].state);
				for (i=0; i<NF_IPCAM_TYPE_MAX; i++)
				{
					nf_ipcam_setup_clear(ch, i);
					pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[i]);
					runtime[ch].sys.ssl_state[i] = IPCAM_SSL_NOT_AVAILABLE;
					pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[i]);
					_release_resource(NULL, NULL, &runtime[ch].sys.ssl[i], &runtime[ch].sys.ctx[i]);
				}
				_release_resource(NULL, NULL, &runtime[ch].sys.ssl_g, &runtime[ch].sys.ctx_g);
				{
					guint16 hport = runtime[ch].sys.http_port;
					guint16 use_ssl = runtime[ch].sys.use_ssl;
					memset(&runtime[ch], 0x00, sizeof(mtable));
					runtime[ch].sys.http_port = hport;
					runtime[ch].sys.use_ssl = use_ssl;
				}
				runtime[ch].state = MGMT_STATE_UNLINKED;
				{
					dtable *discovery = get_dtable();
					if (discovery != NULL)
					{
						memset(&discovery[ch], 0x00, sizeof(dtable));
					}
				}
				if (runtime[ch].motion.stream_snap != NULL)
				{
					free(runtime[ch].motion.stream_snap);
					runtime[ch].motion.stream_snap = NULL;
				}

				memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
				nf_ipcam_set_port_status(ch, &port_status, NULL);
				nf_pnd_evt_notify_fire(ch, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
				nf_pnd_status[ch] = PND_TYPE_UNPLUGGED;
				nf_pnd_recovery_cnt[ch] = 0;

#if (!defined(ENABLE_PROJECT_KMW))&&(!defined(ENABLE_PROJECT_KUMMI))
				if (ch < 8)
				{
					//syscall(SWITCH_SYSCALL, MVSW_CMD_WRITE_REG, ch+0x10, 4, 0x74);
				}
#endif

#if MAKE_NOTIFY_FIRE
				if((1<<ch) & NUM_ACTIVE_CH_MASK)
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "DEVICE OUT : CH(%d)", ch);
					nf_eventlog_put_param(&tval, LT_IPCAM, ch, LP2_IPCAM_DEVICE_OUT, log_buf);
				}
#endif

				IPCAM_DBG(MINOR, "Device out(CH(%d))\n", ch);
				break;
			}
			case IPCAM_EVENT_STREAM_READY:
			{
				gint open_rtn = 0;
				gint ch = 0;

				ch = evt->port;
				ipx_ipcam_free_event(evt);

				GTimeVal now_time;
				unsigned int now_sec;
				unsigned int setup_time;
				int diff_time;

				g_get_current_time(&now_time);
				now_sec = now_time.tv_sec;
				setup_time = runtime[ch].setup_time;

				diff_time = now_sec - setup_time;

				if(setup_time != 0 && diff_time - 4 < 0 && nf_ipcam_is_hisilicon_camera(ch)) 
				{
					IPCAM_DBG(MAJOR, "Event: Retry: IPCAM_EVENT_STREAM_READY(CH:[%02d] now time(%d) setup time (%d)\n", ch, now_sec, setup_time );

					nf_pnd_queue_push(ch, IPCAM_EVENT_STREAM_READY, __LINE__, __FILE__);

					break;
				}

				printf("[%s] Event: IPCAM_EVENT_STREAM_READY(CH:[%02d])\n", __func__, evt->port);

				//resetting runtime setup time  
				runtime[ch].setup_time = 0;

				nf_pnd_evt_notify_fire(ch, PND_TYPE_SETUP_DONE, __LINE__, __FILE__);
				nf_pnd_status[ch] = PND_TYPE_SETUP_DONE;
				IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> +MGMT_STATE_CONFIGURED)\n",
						ch, runtime[ch].state);
				runtime[ch].state |= MGMT_STATE_CONFIGURED;
				IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> -MGMT_STATE_WAITING)\n",
						ch, runtime[ch].state);
				runtime[ch].state &= ~MGMT_STATE_WAITING;

				/* Open channel */
				{
					NMFMrtpPipeChannel info;

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

					// Xiongmai 카메라 예외처리
					if(strcmp(runtime[ch].sys.vendor, "H264") == 0)
					{
						// Xiongmai 카메라는 audio session을 독립적인 세션으로 생성할수 없다.
						// 이러한 한계사양 때문에 영상세션에 interleave channel 로 구분된 오디오를 수신하는 예외동작이 발생한다
						// 해당 정보는 수신모듈에 Audio 세션 정보중 reserved field인 resulution 을 특정값(100)으로 설정하여 전달된다
						info.audio_cnt = 1;
						info.audio.resolution = 100;
						info.audio.ip_addr = runtime[ch].sys.ipaddr;
						info.audio.rtsp_port = runtime[ch].sys.rtsp_port[0];
						info.audio.rtsp_addr = runtime[ch].sys.rtsp_url[0];
					}
					else if (runtime[ch].audio.audio_tx)
					{
						info.audio_cnt = 1;
						info.audio.resolution = 0;
						info.audio.ip_addr = runtime[ch].sys.ipaddr;
						info.audio.rtsp_port = runtime[ch].sys.rtsp_port[0];
						info.audio.rtsp_addr = runtime[ch].sys.rtsp_url[0];
					}

					//if(nf_ipcam_is_vendor_g4s() || nf_ipcam_is_vendor_orion() || nf_ipcam_is_vendor("VICON"))
					{
						info.metadata_on = 1;
					}

					nmf_mrtp_pipe_set_dev_mac(h_iplive, ch, &runtime[ch].sys.macaddr[0]);
					nf_eventlog_put_ipcam_msg("Ready to stream open", ch);
					open_rtn = nmf_mrtp_pipe_open_ch(h_iplive, &info);
					if (open_rtn != 0)
					{
						nf_pnd_status[ch] = 0;
					}
				}

				/* FIXME for panasonic ptz - slow init */
				if(nf_ipcam_is_onvif_support(ch) && (runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ)))
				{
					int i;
					for(i = 0; i < 3; i++)
					{
						int rtn = nf_ipcam_prepare_onvif_ptz(ch);
						if (rtn != -2)
						{
							break;
						}
						usleep(10 * 1000 * 1000);
					}
				}
				break;
			}
			case IPCAM_EVENT_UNKNOWN_DEVICE:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_UNKNOWN_DEVICE(CH(%d))\n", evt->port);
				nf_pnd_status[evt->port] = PND_TYPE_UNKNOWN;
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_UNKNOWN;
					memset(&port_status.vendor, 0x00, 64);
					memset(&port_status.model, 0x00, 64);
					snprintf(port_status.detail, 256, "UNKNOWN DEVICE. CHECK IF CONNECTED DEVICE IS NETWORK CAMERA.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}
#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "UNKNOWN DEVICE : CH(%d)", evt->port);
					nf_eventlog_put_param(&tval, LT_IPCAM, evt->port, LP2_IPCAM_UNKNOWN_DEVICE, log_buf);
				}
#endif
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_CONNECTION_FAIL:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_CONNECTION_FAIL(CH(%d))\n", evt->port);
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				nf_pnd_status[evt->port] = PND_TYPE_CONNECTION_FAIL;
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_UNKNOWN;
					memset(&port_status.vendor, 0x00, 64);
					memset(&port_status.model, 0x00, 64);
					snprintf(port_status.detail, 256, "CONNECTION FAIL. CHECK RTSP/HTTP PORT NUMBERS, OR RECONNECT CAMERA CABLE.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}
#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "DEVICE CONNECTION FAILED : CH(%d)", evt->port);
					nf_eventlog_put_param(&tval, LT_IPCAM, evt->port, LP2_IPCAM_CONNECTION_FAIL, log_buf);
				}
#endif
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_LOGIN_FAIL:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_LOGIN_FAIL(CH(%d))\n", evt->port);

				{
					dtable *discovery = get_dtable();
					discovery[evt->port].login_retry = 0;
				}
				runtime[evt->port].state |=
						MGMT_STATE_LINKED | MGMT_STATE_IDPASS_WAITING;
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
				nf_pnd_status[evt->port] = PND_TYPE_LOGIN_FAIL;
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
					memset(&port_status.vendor, 0x00, 64);
					memset(&port_status.model, 0x00, 64);
					snprintf(port_status.detail, 256, "LOGIN FAIL. CHECK ID/PASSWORD.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}
#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", evt->port);
					nf_eventlog_put_param(&tval, LT_IPCAM, evt->port, LP2_IPCAM_LOGIN_FAIL, log_buf);
				}
#endif
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_MODEL_UNSUPPORT:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_MODEL_UNSUPPORT(CH(%d))\n", evt->port);
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_UNSUPPORTED, __LINE__, __FILE__);
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
					snprintf(port_status.vendor, 64, runtime[ch].sys.vendor);
					snprintf(port_status.model, 64, runtime[ch].sys.model);
					snprintf(port_status.detail, 256, "UNSUPPORTED MODEL.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}
#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "UNSUPPORTED MODEL : CH(%d)", evt->port);
					nf_eventlog_put_param(&tval, LT_IPCAM, evt->port, LP2_IPCAM_UNSUPPORT_MODEL, log_buf);
				}
#endif
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_CONFIG_FAIL:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_CONFIG_FAIL(CH(%d))\n", evt->port);
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
				nf_pnd_status[evt->port] = PND_TYPE_CONFIG_FAIL;
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
					snprintf(port_status.vendor, 64, runtime[ch].sys.vendor);
					snprintf(port_status.model, 64, runtime[ch].sys.model);
					snprintf(port_status.detail, 256, "CONFIGURATION FAIL. RECONNECT CAMERA CABLE.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}
#if MAKE_NOTIFY_FIRE
				{
					GTimeVal tval;
					char log_buf[128];

					gettimeofday(&tval, NULL);
					snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", evt->port);
					nf_eventlog_put_param(&tval, LT_IPCAM, evt->port, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
				}
#endif
				ipx_ipcam_free_event(evt);
				break;
			case IPCAM_EVENT_STREAM_FAIL:
				IPCAM_DBG(MAJOR, "Event: IPCAM_EVENT_STREAM_FAIL(CH(%d))\n", evt->port);
				nf_pnd_evt_notify_fire(evt->port, PND_TYPE_STREAM_FAIL, __LINE__, __FILE__);
				nf_pnd_status[evt->port] = PND_TYPE_STREAM_FAIL;
				nf_pnd_recovery_cnt[evt->port]++;
				runtime[evt->port].sys.retry_cnt = 0;
				runtime[evt->port].sys.progress = 0;
				{
					NFIPCamPortStatus port_status;
					GError *err = NULL;
					gint ch = evt->port;

					port_status.status = 1;
					port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
					snprintf(port_status.vendor, 64, runtime[ch].sys.vendor);
					snprintf(port_status.model, 64, runtime[ch].sys.model);
					snprintf(port_status.detail, 256, "STREAM REQUEST FAIL. RECONNECT CAMERA CABLE.");
					nf_ipcam_set_port_status(ch, &port_status, &err);
				}

				ipx_ipcam_free_event(evt);
				break;
			default:
				IPCAM_DBG(WARN, "Event: IPCAM_EVENT_??\n");
				ipx_ipcam_free_event(evt);
				break;
		}
	}
}

/**
 * @brief mrtp library에서 이벤트가 발생하였을때 호출하는 callback 함수.
 * @param op 분류.(0 - open channel, 1 - close channel, 2 - error ..)
 * @param cbch 채널 번호.
 * @param stream 스트림 번호.
 * @param en 상세 에러 코드.
 * @param user_data user data.
 */
static void ipx_cmd_callback_func(gint op, gint cbch, gint stream, gint en, gpointer user_data)
{
	int ch = cbch;

	if (op != 30)
	{
		IPCAM_DBG(MAJOR, "op(%d) ch(%d) stream(%d) en(%d)\n", op, ch, stream, en);
	}

	if (op == 0)	// op == 0: open channel
	{
		if (en == NF_RTSP_ERR_NO_ERR)
		{
			NFIPCamPortStatus port_status;

			runtime[ch].sys.progress = 100;
			nf_pnd_prog_notify_fire(ch, runtime[ch].sys.progress, __LINE__, __FILE__);
			runtime[ch].sys.progress = 0;
			runtime[ch].sys.retry_cnt = 0;
			{
				IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_ADD_NEW_CH);
				evt->port = ch;
				g_async_queue_push(vloss_queue, evt);
			}
			nf_pnd_evt_notify_fire(ch, PND_TYPE_VIDEO_START, __LINE__, __FILE__);
			nf_pnd_status[ch] = PND_TYPE_VIDEO_START;

			port_status.status = 0;
			port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
			snprintf(port_status.vendor, 64, runtime[ch].sys.vendor);
			snprintf(port_status.model, 64, runtime[ch].sys.model);
			memset(&port_status.detail, 0x00, 256);
			nf_ipcam_set_port_status(ch, &port_status, NULL);
			nf_ipcam_set_pnd_osd_status(ch, PND_OSD_NONE);
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE READY : %d", ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, ch, LP2_IPCAM_DEVICE_READY, log_buf);
			}
#endif

			printf("\e[104m >>>>>> CH(%d) PND_TYPE_VIDE_START noti!! \e[0m\n", ch);

		}
		else 
		{
			GAsyncQueue *queue = get_queue();

			nf_ipcam_set_pnd_osd_status(ch, PND_OSD_STREAM_FAIL);
			if (en == NF_RTSP_ERR_OPEN_RES_TEMP_UNAVAIL)
			{
				resource_fail_count++;
				IPCAM_DBG(ERROR, "Socket resource fail(%d)\n", resource_fail_count);

				if (resource_fail_count >= 20)
				{
					IPCAM_DBG(MAJOR, "set reboot to recover\n");
				}
			}
			nf_pnd_queue_push(ch, IPCAM_EVENT_STREAM_FAIL, __LINE__, __FILE__);
		}
	}
	else if (op == 1)	// op == 1: close channel
	{
		IPCAM_DBG(MAJOR, "close (CH(%d).%d: %s)\n", ch, stream, RTSP_ERR_STR[en]);

		if (en == NF_RTSP_ERR_CLOSE_INTERNAL)
		{
			int z = 0;
			int rtn = IPCAM_SETUP_RTN_FAILED;
			NFIPCamPortStatus port_status;

			rtn = nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
			if (rtn ==  IPCAM_SETUP_RTN_DONE)
			{
				IPCAM_DBG(MINOR, "poe reboot OK\n");
			}
			else
			{
				IPCAM_DBG(MINOR, "poe reboot fail\n");
			}

#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE RESET : %d", ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, ch, LP2_IPCAM_DEVICE_RESET, log_buf);
			}
#endif
			{
				//gint ch = 0;
				gint i = 0;
				NFIPCamPortStatus port_status;

				nmf_mrtp_pipe_close_ch(h_iplive, ch);

				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
					evt->port = ch;
					g_async_queue_push(vloss_queue, evt);
				}

				IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
						ch, runtime[ch].state);

				nvs_close_all_sub_ch(ch);

				{
					guint16 hport = runtime[ch].sys.http_port;
					guint16 use_ssl = runtime[ch].sys.use_ssl;
					memset(&runtime[ch], 0x00, sizeof(mtable));
					runtime[ch].state = MGMT_STATE_UNLINKED;
					runtime[ch].sys.http_port = hport;
					runtime[ch].sys.use_ssl = use_ssl;
				}

				memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
				nf_ipcam_set_port_status(ch, &port_status, NULL);
				nf_pnd_evt_notify_fire(ch, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
				nf_pnd_status[ch] = PND_TYPE_UNPLUGGED;
				nf_pnd_recovery_cnt[ch] = 0;
			}
			return;
		}
	}
	else if (op == 2)	// mrtpsrc callback ERROR
	{
		switch (en)
		{
			case 101:	// socket corrupt
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "SOCKET CORRUPTED : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] socket corrupted(CH(%d))\n", ch);
				break;
			}
			case 102:	// ring full
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "RING BUFFER FULL : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] ring buffer full(CH(%d))\n", ch);
				break;
			}
			case 103:	// magic A
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "MAGIC NO ERR CASE A : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case A(CH(%d))\n", ch);
				break;
			}
			case 104:	// magic B
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "MAGIC NO ERR CASE B : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case B(CH(%d))\n", ch);
				break;
			}
			case 105:	// magic C
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "MAGIC NO ERR CASE C : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case C(CH(%d))\n", ch);
				break;
			}
			case 106:	// mem fail
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "MEM ALLOC FAIL : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] mem alloc fail(CH(%d))\n", ch);
				break;
			}
			case 107:	// cmem fail
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "CMEM ALLOC FAIL : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] cmem alloc fail(CH(%d))\n", ch);
				break;
			}
			case 108:	// vlen zero
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "VLEN ZERO VIOLATION : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] vlen zero violation(CH(%d))\n", ch);
				break;
			}
			case 109:	// abnormal systime
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "ABNORMAL SYSTEM TIME : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] abnormal systime(CH(%d))\n", ch);
				break;
			}
			case 110:	// timestamp err
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "TIMESTAMP ERR : %d", ch);
				//nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
				nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] timestamp err(CH(%d))\n", ch);
				break;
			}
			case 201:	// connection loss
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "LONG LASTING PACKET LOSS : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] long lasting packet loss(CH(%d))\n", ch);
				break;
			}
			case 202:	// control frame mem fail
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "CONTROL FRAME ALLOC FAIL : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] control frame alloc fail(CH(%d))\n", ch);
				break;
			}
			case 203:	// control frame cmem fail
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "CONTROL CMEM ALLOC FAIL : %d", ch);
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] control cmem alloc fail(CH(%d))\n", ch);
				break;
			}
			case 301:	// rate control
			{
#if MAKE_NOTIFY_FIRE
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "RATE CONTROL : %d", ch);
#if 1
				nf_eventlog_put_param(&tval, LT_SYSTEM_DEBUG, 1,
						LP2_SYSTEM_DEBUG_STRING, log_buf);
#endif
#endif
				//nf_ipcam_rate_down(ch);
				//nf_eventlog_put_param(&tval, LT_SYSTEM_EVENT, 1, LP2_SYSTEM_EVENT_LANGUAGE_CHANGED, "RATE DOWN");
				//nf_eventlog_put_param(&tval, LT_SYSTEM_EVENT, 1, LP2_SYSTEM_EVENT_LANGUAGE_CHANGED, "RECONN_FAIL");
				IPCAM_DBG(MINOR, "rate down(CH(%d))\n", ch);
				break;
			}
			default:
				break;
		}
	}
	else if (op == 10)	// resolution changed
	{
		unsigned int old_resol;
		unsigned int new_resol;
		uint64_t or;
		uint64_t nr;

		old_resol = (en>>8);
		new_resol = (en&0xff);
		or = ipx_resol_icodec2cm(old_resol);
		nr = ipx_resol_icodec2cm(new_resol);
		runtime[ch].video.resolution.supported &= ~or;
		runtime[ch].video.resolution.supported |= nr;
		runtime[ch].video.resolution.resolution[stream] = nr;
	}
    else if (op == 20)
    {   // OntheFly TEST start!! 
    	sleep(1);
    	if(en != NF_RTSP_ERR_NO_ERR)
    	{
            IPCAM_DBG(MAJOR, "CH(%d) ERROR in OnTheFly Session Open, experiment end!\n", ch);
            runtime[ch].onvif.query_onthefly_state = 2;
            return;
    	}

        int port = ch;
        int ntpal = (runtime[port].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
        int i;

        NFIPCamSetupVCodec info;
        memset(&info, 0x00, sizeof(NFIPCamSetupVCodec));
        info.ch = port;
        info.stream_cnt = 1;
        for (i = 0; i < 1; i++)
        {
            info.resolution[i] = runtime[port].video.resolution.resolution[i];
			info.vcodec[i] = runtime[ch].video.vcodec[i];

            int fps_cur = runtime[port].video.fps[ntpal][i].value;
            int fps_supp = runtime[port].video.fps[ntpal][i].support;
            if(fps_cur != fps_supp)
            {
                fps_supp &= ~(fps_cur);
                if(fps_supp & NF_IPCAM_FPS_20)
                {
                    info.fps[i] = NF_IPCAM_FPS_20;
                }
                else if(fps_supp & NF_IPCAM_FPS_30)
                {
                    info.fps[i] = NF_IPCAM_FPS_30;
                }
                else
                {
                    info.fps[i] = NF_IPCAM_FPS_10;
                }

            }
            else
            {
                info.fps[i] = fps_cur;
            }

            int bit_cur = runtime[port].video.bitrate[i].value;
            //int bit_min = runtime[port].video.bitrate[i].min;
            //int bit_max = runtime[port].video.bitrate[i].max;
            int bit_min = runtime[port].encoder.min_bitrate[i];
            int bit_max = runtime[port].encoder.max_bitrate[i];
            if(bit_cur == bit_min)
            {
                info.bitrate[i] = bit_max;
            }
            else
            {
                info.bitrate[i] = bit_min;
            }

        }
        info.mirror = runtime[port].video.mirror.value;
        info.ntsc_pal = ntpal;
        int result = nf_ipcam_set_vcodec_onvif(port, &info, NULL, NULL, NULL);
    }
    else if (op == 21)
    {   // OntheFly TEST fail!!
        IPCAM_DBG(MAJOR, "CH(%d) No Support OnTheFly, experiment end!\n", ch);
        runtime[ch].onvif.query_onthefly_state = 2;
    }
	else if (op == 30)
	{
#if 0
		// To recover abnormal state AC power camera
		struct timespec now_time;
		dtable *discovery = get_dtable();

		clock_gettime(CLOCK_REALTIME, &now_time);
		if (en > 0 && discovery[ch].state == IPCAM_DISC_STATE_DONE && now_time.tv_sec - en > 60)
		{
			IPCAM_DBG(WARN, "CH(%d) Wrong discovery state while(%d~%d)\n", ch, en, now_time.tv_sec);
			nf_pnd_queue_push(ch, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
		}
#endif
	}

}

/**
 * @brief 수명이 다한 이벤트를 소멸 처리한다.
 * @param evt 삭제할 이벤트 객체.
 */
static void ipx_ipcam_free_event(IPX_PND_EVENT* evt)
{
	g_return_if_fail(evt != NULL);
	g_free(evt);
}

static unsigned int ipx_resol_cm2icodec(uint64_t cam_manager_resol)
{
	unsigned int rtn = RES_NTSC_NONE;
	if(cam_manager_resol == NF_IPCAM_RES_352x240) 	{ rtn = NF_RES_NTSC_CIF;  }
	if(cam_manager_resol == NF_IPCAM_RES_352x288) 	{ rtn = NF_RES_PAL_CIF;   }
	if(cam_manager_resol == NF_IPCAM_RES_640x352) 	{ rtn = NF_RES_640x352;   }
	if(cam_manager_resol == NF_IPCAM_RES_640x480) 	{ rtn = NF_RES_640x480;   }
	if(cam_manager_resol == NF_IPCAM_RES_704x480) 	{ rtn = NF_RES_NTSC_4CIFP;}
	if(cam_manager_resol == NF_IPCAM_RES_704x576) 	{ rtn = NF_RES_PAL_4CIFP; }
	if(cam_manager_resol == NF_IPCAM_RES_720x480) 	{ rtn = NF_RES_720x480;   }
	if(cam_manager_resol == NF_IPCAM_RES_720x576) 	{ rtn = NF_RES_720x576;   }
	if(cam_manager_resol == NF_IPCAM_RES_1280x720I) { rtn = NF_RES_1280x720I; }
	if(cam_manager_resol == NF_IPCAM_RES_1280x720)  { rtn = NF_RES_1280x720;  }
	if(cam_manager_resol == NF_IPCAM_RES_1024x768)  { rtn = NF_RES_1024x768;  }
	if(cam_manager_resol == NF_IPCAM_RES_1280x1024) { rtn = NF_RES_1280x1024; }
	if(cam_manager_resol == NF_IPCAM_RES_1920x1080I){ rtn = NF_RES_1920x1080I;}
	if(cam_manager_resol == NF_IPCAM_RES_1920x1080) { rtn = NF_RES_1920x1080; }
	if(cam_manager_resol == NF_IPCAM_RES_640x360) 	{ rtn = NF_RES_640x360;   }
	if(cam_manager_resol == NF_IPCAM_RES_640x400) 	{ rtn = NF_RES_640x400;   }
	if(cam_manager_resol == NF_IPCAM_RES_800x450) 	{ rtn = NF_RES_800x450;   }
	if(cam_manager_resol == NF_IPCAM_RES_1440x900) 	{ rtn = NF_RES_1440x900;  }
	if(cam_manager_resol == NF_IPCAM_RES_800x600) 	{ rtn = NF_RES_800x600;   }
	if(cam_manager_resol == NF_IPCAM_RES_1600x1200) { rtn = NF_RES_1600x1200; }
	if(cam_manager_resol == NF_IPCAM_RES_320x180) 	{ rtn = NF_RES_320x180;   }
	if(cam_manager_resol == NF_IPCAM_RES_2304x1296) { rtn = NF_RES_2304x1296; }
	if(cam_manager_resol == NF_IPCAM_RES_2048x1536) { rtn = NF_RES_2048x1536; }
	if(cam_manager_resol == NF_IPCAM_RES_2560x1440) { rtn = NF_RES_2560x1440; }
	if(cam_manager_resol == NF_IPCAM_RES_2688x1520) { rtn = NF_RES_2688x1520; }
	if(cam_manager_resol == NF_IPCAM_RES_2560x1600) { rtn = NF_RES_2560x1600; }
	if(cam_manager_resol == NF_IPCAM_RES_2560x1920) { rtn = NF_RES_2560x1920; }
	if(cam_manager_resol == NF_IPCAM_RES_2592x1920) { rtn = NF_RES_2592x1920; }
	if(cam_manager_resol == NF_IPCAM_RES_2592x1944) { rtn = NF_RES_2592x1944; }
	if(cam_manager_resol == NF_IPCAM_RES_2992x1680) { rtn = NF_RES_2992x1680; }
	if(cam_manager_resol == NF_IPCAM_RES_2880x1800) { rtn = NF_RES_2880x1800; }
	if(cam_manager_resol == NF_IPCAM_RES_3200x1800) { rtn = NF_RES_3200x1800; }
	if(cam_manager_resol == NF_IPCAM_RES_2880x2160) { rtn = NF_RES_2880x2160; }
	if(cam_manager_resol == NF_IPCAM_RES_3072x2048) { rtn = NF_RES_3072x2048; }
	if(cam_manager_resol == NF_IPCAM_RES_3200x2400) { rtn = NF_RES_3200x2400; }
	if(cam_manager_resol == NF_IPCAM_RES_3840x2160) { rtn = NF_RES_3840x2160; }
	if(cam_manager_resol == NF_IPCAM_RES_2592x1520) { rtn = NF_RES_2592x1520; }
	if(cam_manager_resol == NF_IPCAM_RES_3000x3000) { rtn = NF_RES_3000x3000; }
	if(cam_manager_resol == NF_IPCAM_RES_2048x2048) { rtn = NF_RES_2048x2048; }
	if(cam_manager_resol == NF_IPCAM_RES_1280x1280) { rtn = NF_RES_1280x1280; }
	if(cam_manager_resol == NF_IPCAM_RES_640x640) { rtn = NF_RES_640x640; }
	if(cam_manager_resol == NF_IPCAM_RES_320x320) { rtn = NF_RES_320x320; }

	return rtn;
}

static uint64_t ipx_resol_icodec2cm(unsigned int icodec_resol)
{
	uint64_t rtn;
	switch(icodec_resol)
	{
		case NF_RES_NTSC_CIF:	 { rtn = NF_IPCAM_RES_352x240;     break; }
		case NF_RES_PAL_CIF:	 { rtn = NF_IPCAM_RES_352x288;     break; }
		case NF_RES_640x352:	 { rtn = NF_IPCAM_RES_640x352;     break; }
		case NF_RES_640x480:	 { rtn = NF_IPCAM_RES_640x480;     break; }
		case NF_RES_NTSC_4CIFP: { rtn = NF_IPCAM_RES_704x480;     break; }
		case NF_RES_PAL_4CIFP:	 { rtn = NF_IPCAM_RES_704x576;     break; }
		case NF_RES_720x480:	 { rtn = NF_IPCAM_RES_720x480;     break; }
		case NF_RES_720x576:	 { rtn = NF_IPCAM_RES_720x576;     break; }
		case NF_RES_1280x720I:	 { rtn = NF_IPCAM_RES_1280x720I;   break; }
		case NF_RES_1280x720:	 { rtn = NF_IPCAM_RES_1280x720;    break; }
		case NF_RES_1024x768:	 { rtn = NF_IPCAM_RES_1024x768;    break; }
		case NF_RES_1280x1024:	 { rtn = NF_IPCAM_RES_1280x1024;   break; }
		case NF_RES_1920x1080I: { rtn = NF_IPCAM_RES_1920x1080I;  break; }
		case NF_RES_1920x1080:	 { rtn = NF_IPCAM_RES_1920x1080;   break; }
		case NF_RES_640x360:	 { rtn = NF_IPCAM_RES_640x360;     break; }
		case NF_RES_640x400:	 { rtn = NF_IPCAM_RES_640x400;     break; }
		case NF_RES_800x450:	 { rtn = NF_IPCAM_RES_800x450;     break; }
		case NF_RES_1440x900:	 { rtn = NF_IPCAM_RES_1440x900;    break; }
		case NF_RES_800x600:	 { rtn = NF_IPCAM_RES_800x600;     break; }
		case NF_RES_1600x1200:	 { rtn = NF_IPCAM_RES_1600x1200;   break; }
		case NF_RES_320x180:	 { rtn = NF_IPCAM_RES_320x180;     break; }
		case NF_RES_2304x1296:	 { rtn = NF_IPCAM_RES_2304x1296;   break; }
		case NF_RES_2048x1536:	 { rtn = NF_IPCAM_RES_2048x1536;   break; }
		case NF_RES_2560x1440:	 { rtn = NF_IPCAM_RES_2560x1440;   break; }
		case NF_RES_2688x1520:	 { rtn = NF_IPCAM_RES_2688x1520;   break; }
		case NF_RES_2560x1600:	 { rtn = NF_IPCAM_RES_2560x1600;   break; }
		case NF_RES_2560x1920:	 { rtn = NF_IPCAM_RES_2560x1920;   break; }
		case NF_RES_2592x1920:	 { rtn = NF_IPCAM_RES_2592x1920;   break; }
		case NF_RES_2592x1944:	 { rtn = NF_IPCAM_RES_2592x1944;   break; }
		case NF_RES_2992x1680:	 { rtn = NF_IPCAM_RES_2992x1680;   break; }
		case NF_RES_2880x1800:	 { rtn = NF_IPCAM_RES_2880x1800;   break; }
		case NF_RES_3200x1800:	 { rtn = NF_IPCAM_RES_3200x1800;   break; }
		case NF_RES_2880x2160:	 { rtn = NF_IPCAM_RES_2880x2160;   break; }
		case NF_RES_3072x2048:	 { rtn = NF_IPCAM_RES_3072x2048;   break; }
		case NF_RES_3200x2400:	 { rtn = NF_IPCAM_RES_3200x2400;   break; }
		case NF_RES_3840x2160:	 { rtn = NF_IPCAM_RES_3840x2160;   break; }
		case NF_RES_2592x1520:	 { rtn = NF_IPCAM_RES_2592x1520;   break; }
		case NF_RES_3000x3000:	 { rtn = NF_IPCAM_RES_3000x3000;   break; }
		case NF_RES_2048x2048:	 { rtn = NF_IPCAM_RES_2048x2048;   break; }
		case NF_RES_1280x1280:	 { rtn = NF_IPCAM_RES_1280x1280;   break; }
		case NF_RES_640x640:	 { rtn = NF_IPCAM_RES_640x640;   break; }
		case NF_RES_320x320:	 { rtn = NF_IPCAM_RES_320x320;   break; }
		default:             { rtn = 0;                        break; }
	}

	return rtn;
}

/**
 * @brief x번 접속시도가 실패하였을 때 처리를 한다.(poe reboot 등)
 * @param param NULL.
 */
static void pnd_recovery_func(void* param)
{
	const int skip_consts[] = { 0, 5, 10, 30, 60, 120, 300 };
	int i = 0;
	int skip_cnt[AVAILABLE_MAX_CH];
	int recovery_cnt[AVAILABLE_MAX_CH];
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	GAsyncQueue *queue = get_queue();

	while (queue == NULL) {
		usleep(500*1000);
		queue = get_queue();
	}

	memset(skip_cnt, 0x00, sizeof(int) * AVAILABLE_MAX_CH);
	memset(recovery_cnt, 0x00, sizeof(int) * AVAILABLE_MAX_CH);
	while(1)
	{
		sleep(1);

        if(is_zmq_running()){
            aibox_zmq_recovery_process();
            nf_api_aibox_conn_check();
        }
		
		if (vloss_status != _vloss_old)
		{
			IPCAM_DBG(MINOR, "vloss notify fire (%08x->%08x)\n", _vloss_old, vloss_status);
			_vloss_old = vloss_status&NUM_ACTIVE_CH_MASK;
			nf_notify_fire_params("vloss", vloss_status&NUM_ACTIVE_CH_MASK, 0,0,0);
		}

#if 1
		IPCAM_DBG(MINOR, "pnd_status(%d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d)\n",
				nf_pnd_status[0], nf_pnd_status[1],
				nf_pnd_status[2], nf_pnd_status[3],
				nf_pnd_status[4], nf_pnd_status[5],
				nf_pnd_status[6], nf_pnd_status[7],
				nf_pnd_status[8], nf_pnd_status[9],
				nf_pnd_status[10], nf_pnd_status[11],
				nf_pnd_status[12], nf_pnd_status[13],
				nf_pnd_status[14], nf_pnd_status[15]);

		IPCAM_DBG(MINOR, "rec_changed[%d,%d,%d,%d %d,%d,%d,%d] rate_control[%d,%d,%d,%d %d,%d,%d,%d]\n",
				runtime[0].rec_changed, runtime[1].rec_changed,
				runtime[2].rec_changed, runtime[3].rec_changed,
				runtime[4].rec_changed, runtime[5].rec_changed,
				runtime[6].rec_changed, runtime[7].rec_changed,
				runtime[0].rate_control, runtime[1].rate_control,
				runtime[2].rate_control, runtime[3].rate_control,
				runtime[4].rate_control, runtime[5].rate_control,
				runtime[6].rate_control, runtime[7].rate_control);
#endif

		{
			unsigned int video_loss_status = get_vloss_status();
			for (i = 0; i < AVAILABLE_MAX_CH; i++) {
				if ((video_loss_status & (1 << i)) && nf_pnd_status[i] == PND_TYPE_VIDEO_START) {
					if (runtime[i].sys.type == SYSTEM_DEVICE_NVS && runtime[i].sys.nvs_stream_start != 0) {
						IPCAM_DBG(MINOR, "nvs_close_stream(CH(%d))\n", i);
						nvs_close_stream(i);
					}
				}
			}
		}

		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (runtime[i].rec_changed > RATE_IGNORE_TIME)
			{
				runtime[i].rec_changed = 0;
				continue;
			}

			if (runtime[i].rec_changed > 0)
			{
				runtime[i].rec_changed--;
			}
		}

		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (nf_get_running_mode())
			{
				continue;
			}
			if (runtime[i].rate_control > RATE_SUSTENANCE_TIME)
			{
				runtime[i].rate_control = 0;
				continue;
			}
			if (runtime[i].rate_control > 0)
			{
				runtime[i].rate_control--;
				if (runtime[i].rate_control <= 0)
				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_RATE_RECOVER);
					evt->port = i;
					g_async_queue_push(recevt_queue, evt);
					runtime[i].rate_control = 0;
					//nf_ipcam_rate_recover(i);
				}
			}
		}

		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (get_running_state() == DISCOVERY_STOPPED) { continue; }
			if (nf_pnd_status[i] == PND_TYPE_UNSUPPORTED) { continue; }
			if (nf_pnd_status[i] >= PND_TYPE_UNKNOWN && nf_pnd_status[i] <= PND_TYPE_STREAM_FAIL)
			{
				if (skip_cnt[i] <= 0)
				{
					NFIPCamPortStatus port_status;

					if (recovery_cnt[i] != nf_pnd_recovery_cnt[i])
					{
						recovery_cnt[i] = nf_pnd_recovery_cnt[i];
						if (recovery_cnt[i] > 1 || recovery_cnt[i] < 1)
						{
							IPCAM_DBG(WARN, "pnd recovery makes poe reset(CH(%d))\n", i);

							// Retry if this port is nvs sub ch.
							if (runtime[i].sys.type == SYSTEM_DEVICE_NVS && runtime[i].sys.nvs_sub_ch > 0) {
								IPCAM_DBG(MINOR, "nvs_close_stream(CH(%d))\n", i);
								nvs_close_stream(i);
							}
							else {
								nf_ipcam_poe_reboot(i, NULL, NULL, NULL);
							}
							nf_pnd_recovery_cnt[i] = 0;
							nf_pnd_status[i] = PND_TYPE_UNPLUGGED;
#if defined(ENABLE_PROJECT_KMW)||defined(ENABLE_PROJECT_KUMMI)
							IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
									i, runtime[i].state);
							runtime[i].state = MGMT_STATE_UNLINKED;
							discovery[i].state = IPCAM_DISC_STATE_NONE;
							discovery[i].state_cnt = 0;
							discovery[i].retry_cnt = 0;
#else
							IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
																i, runtime[i].state);
							runtime[i].state = MGMT_STATE_UNLINKED;
							memset(&discovery[i], 0x00, sizeof(dtable));
#endif
							continue;
						}
						skip_cnt[i] = skip_consts[recovery_cnt[i]];
						IPCAM_DBG(MINOR, "pnd recovery will start after %d seconds(CH(%d), CNT:%d)\n",
								skip_cnt[i], i, recovery_cnt[i]);
						continue;
					}

					IPCAM_DBG(MINOR, "pnd recovery start CH(%d)\n", i);
					memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
					nf_ipcam_set_port_status(i, &port_status, NULL);
					nf_pnd_evt_notify_fire(i, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
					nf_pnd_status[i] = PND_TYPE_UNPLUGGED;

					// Retry if this port is nvs sub ch.
					if (runtime[i].sys.type == SYSTEM_DEVICE_NVS) {
						runtime[i].state = MGMT_STATE_UNLINKED;
						IPCAM_DBG(MINOR, "nvs_close_stream(CH(%d))\n", i);
						nvs_close_stream(i);
						continue;
					}

					IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
							i, runtime[i].state);
					runtime[i].state = MGMT_STATE_UNLINKED;

					/*
					if (discovery[i].layer == IPCAM_DISC_LAYER_VHUB) {
						if (!(runtime[i].sys.type == SYSTEM_DEVICE_NVS && runtime[i].sys.nvs_sub_ch > 0))
							hub_unlink_request(i);
					}
					*/

					memset(&discovery[i], 0x00, sizeof(dtable));
				}
				else
				{
					skip_cnt[i]--;
				}
			}
		}

		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (nf_get_running_mode())
			{
				continue;
			}
			if (runtime[i].delayed > 1)
			{
				IPCAM_DBG(MINOR, "IPCAM delay to connect(CH(%d), delay:%d, event(%s))\n",
						i, runtime[i].delayed, _IPCAM_EVENT_STR__[runtime[i].next_event]);
				runtime[i].delayed--;
				nf_pnd_prog_notify_fire(i, 50, __LINE__, __FILE__);
			}
			else if (runtime[i].delayed == 1)
			{
				IPCAM_DBG(MINOR, "IPCAM delay end(CH(%d), delay:%d, event(%s))\n",
						i, runtime[i].delayed, _IPCAM_EVENT_STR__[runtime[i].next_event]);
				nf_pnd_queue_push(i, runtime[i].next_event, __LINE__, __FILE__);
				runtime[i].delayed = 0;
				runtime[i].next_event = 0;
			}
		}
	}
}

static unsigned int a8tohex(char* s)
{
	const int submt_0 = 48;
	const int submt_a = 87;
	const int submt_A = 55;

	int i = 0;
	int hex_val[8];
	int hex_val_byte[4];

	for(i = 0; i < 8; i++)
	{
		if (s[i] >= '0' && s[i] <= '9')
		{
			hex_val[i] = s[i] - submt_0;
		}
		else if (s[i] >= 'a' && s[i] <= 'f')
		{
			hex_val[i] = s[i] - submt_a;
		}
		else if (s[i] >= 'A' && s[i] <= 'F')
		{
			hex_val[i] = s[i] - submt_A;
		}
		else
		{
			return 0;
		}
	}

	hex_val_byte[0] = hex_val[0] * 16 + hex_val[1];
	hex_val_byte[1] = hex_val[2] * 16 + hex_val[3];
	hex_val_byte[2] = hex_val[4] * 16 + hex_val[5];
	hex_val_byte[3] = hex_val[6] * 16 + hex_val[7];

	return (hex_val_byte[0] << 24 | hex_val_byte[1] << 16 | hex_val_byte[2] << 8 | hex_val_byte[3]);
}

static void _send_arp_req(unsigned int my_ip, unsigned int dest_ip)
{
	int send_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	char* s;
	char* p;
	char* udph_p;
	unsigned short* p_short;
	unsigned int* p_int;
	unsigned char out_buf[2048];
	unsigned char _my_mac_addr[8];
	int len = (-1);
	int i,j;

	struct ethhdr *eh;
	struct iphdr ip;
	struct udphdr udp;

	struct sockaddr_ll sll;


	IPCAM_DBG(MAJOR, "start\n");

	if (send_sock < 0)
	{
		IPCAM_DBG(ERROR, "ARP send sock init failed\n");
		return;
	}

	if (!nf_netif_get_mac_str(&_my_mac_addr[0])) {
		IPCAM_DBG(ERROR, "Get Mac addr failed\n");
		return;
	}

	p = &out_buf[0];
	memset(out_buf, 0x00, 2048);
	eh = (void*) p;

	/* Ethernet header */
	/* Dest MAC */
	*(p+0) = 0xff;
	*(p+1) = 0xff;
	*(p+2) = 0xff;
	*(p+3) = 0xff;
	*(p+4) = 0xff;
	*(p+5) = 0xff;
	p += ETH_ALEN;
	/* Src MAC */
	memcpy((void*)p, (void*)&_my_mac_addr[0], ETH_ALEN);
	p += ETH_ALEN;
	/* Protocol */
	eh->h_proto = htons(ETH_P_ARP);
	p += sizeof(short);
	/* Ethernet header end */

	/* Address Resolution Protocol */
	/* HW type */
	*p++ = 0x00;
	*p++ = 0x01;
	/* Protocol type */
	*p++ = 0x08;
	*p++ = 0x00;
	/* HW size */
	*p++ = 0x06;
	/* Protocol size */
	*p++ = 0x04;
	/* Opcode - request */
	*p++ = 0x00;
	*p++ = 0x01;
	/* Sender MAC */
	memcpy((void*)p, (void*)&_my_mac_addr[0], ETH_ALEN);
	p += ETH_ALEN;
	/* Sender IP */
	memcpy((void*)p, (void*)&my_ip, sizeof(unsigned int));
	p += sizeof(unsigned int);
	/* Target MAC */
	*(p+0) = 0x00;
	*(p+1) = 0x00;
	*(p+2) = 0x00;
	*(p+3) = 0x00;
	*(p+4) = 0x00;
	*(p+5) = 0x00;
	p += ETH_ALEN;
	/* Target IP */
	memcpy((void*)p, (void*)&dest_ip, sizeof(unsigned int));
	p += sizeof(unsigned int);
	p += 0x12;		// trail 0s
	s = &out_buf[0];


	memset(&sll, 0x00, sizeof(sll));
	sll.sll_family   = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_ARP);
	sll.sll_ifindex  = if_nametoindex(LINK_LOCAL_DEVICE); // index of hub_eth_dev
	sll.sll_hatype   = ARPHRD_ETHER;
	sll.sll_pkttype  = PACKET_BROADCAST;
	sll.sll_halen    = ETH_ALEN;
	/*MAC - begin*/
	sll.sll_addr[0]  = 0xff;
	sll.sll_addr[1]  = 0xff;
	sll.sll_addr[2]  = 0xff;
	sll.sll_addr[3]  = 0xff;
	sll.sll_addr[4]  = 0xff;
	sll.sll_addr[5]  = 0xff;
	/*MAC - end*/
	sll.sll_addr[6]  = 0x00;/*not used*/
	sll.sll_addr[7]  = 0x00;/*not used*/

#if 0
	len = p - s;
	for (i=0; i<len; i++)
	{
		if (i == 0)
		{
			printf("     ");
			for (j=0; j<16; j++)
			{
				if (j==8)
				{
					printf(" ");
				}
				printf("%02x ", j);
			}
		}
		if (i%8 == 0)
		{
			printf(" ");
		}
		if (i%16 == 0)
		{
			printf("\n%04x ", i);
		}
		printf("%02x ", out_buf[i]);
	}
	printf("\n");
#endif
	len = sendto(send_sock, out_buf, len, 0, (struct sockaddr*)&sll, sizeof(sll));
	//printf("[%s] send(%d)\n", __FUNCTION__, len);
	close(send_sock);
}

static void _unref_vnet(int id)
{
	int i = 0;
	unsigned int h=0,m=0;
	char sys_cmd_str[256];
	mtable *runtime = NULL;

	runtime = get_runtime();
	g_return_if_fail(runtime != NULL);
	g_return_if_fail(id != 0);	// link local address
	g_return_if_fail(id != 1);	// camera network
	g_return_if_fail(id != ONVIF_VNET_MAX);

	for(i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].onvif.vnet_id == id)
		{
			IPCAM_DBG(MINOR, "Using vnet(%d)\n", id);
			return;
		}
	}

	snprintf(sys_cmd_str, 256, "ifconfig %s:%d down", HUB_ETH_DEVICE, id);
	IPCAM_DBG(MINOR, "%s\n", sys_cmd_str);
	proxy_system(sys_cmd_str, 1, 3);
}

static void _init_switch_device(void)
{
	nf_dev_switch_init(NF_UTIL_SWITCH_CCTV_MODE);
}

extern void nf_ipcam_init_switch_device(void)
{
	_init_switch_device();
}

#endif	// __IPX_PND_

