/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
16/01/2013 Jae-young Kim  Created. (Based on nmf 2.0 api )
*/

#ifndef __NF_API_OPENMODE_C__
#define __NF_API_OPENMODE_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>

#include <regex.h>
#include <ctype.h>

#include <openssl/ssl.h>

#include <gobj.h>
#include <gobjmrtppipe.h>
#include <gobjmrtpsrc.h>

#include <nf_common.h>
#include <nf_sysdb.h>
#include <nf_notify.h>
#include <nf_util_device.h>
#include <nf_util_netif.h>

#include <nf_codec_header.h>
#include <nf_api_eventlog.h>
#include <nf_logevtdef.h>
#include <nf_api_ipcam.h>
#include <nf_api_openmode.h>
#include <nf_ipcam_defs.h>
#include <glib.h>

#include "nf_ipcam_driver_itx_md5.h"
#include "jbshell.h"

#include "nf_ipcam_utils.h"
#include "nf_api_dlva.h"
#include "nf_api_http.h"

#include "proxy_cli.h"

#define _ADMIN_SVR_PORT					(32679)
#define _ADMIN_CLI_PORT					(32678)
#define _WS_DISC_PORT 					(3702)
#define _WS_DISC_CLI_PORT 				(31000)
#define _MCAST_GRP_ADDR					"239.255.255.250"
#define _MSG_ID_FORMAT					"uuid:%s"

#define NBUF_SZ         (1536)  // 1024 + 512
#define USER_AGENT_STR      "IPX-NVR"

#define ONVIF_XADDR_LEN					(256)

#define INSTALLATION_STATE_FINAL		(-3)
#define INSTALLATION_STATE_FINAL_PROG	(-2)
#define INSTALLATION_STATE_NONE			(0)
#define INSTALLATION_STATE_STOPPED		(1)
#define INSTALLATION_STATE_START		(2)

#define MAX_SEARCHABLE_CAM_COUNT		(256)

#define BLOCKING_SCAN					(1)

static const char discovery_msg[] = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<Envelope xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" "
		"xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"
	"<Header>"
		"<wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">"
			"%s"
		"</wsa:MessageID>"
		"<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">"
			"urn:schemas-xmlsoap-org:ws:2005:04:discovery"
		"</wsa:To>"
		"<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">"
			"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe"
		"</wsa:Action>"
	"</Header>"
	"<Body>"
		"<Probe "
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
			"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" "
			"xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
			"<Types>dn:NetworkVideoTransmitter</Types>"
			"<Scopes />"
		"</Probe>"
	"</Body>"
"</Envelope>";

static const char onvif_raw[] =
"POST %s HTTP/1.1\r\n"	// /onvif/device_service
"Host: %s\r\n"			// 192.168.100.63
"Content-Type: application/soap+xml; charset=utf-8\r\n"
"Content-Length: %d\r\n"
"\r\n"
"%s";

static const char* __OPENMODE_CAM_STATE_STR_[] =
{
	"INIT",
	"DISCOVERED",
	"REQ_IP",
	"DEV_INFO",
	"DEV_INFO_ONVIF",
	"ASSIGN_CH",
	"OK",

	"INVALID_IP",
	"PW_CHG_REQUIRED",

	"CONN_FAIL",
	"LOGIN_FAIL",
	"STREAM_FAIL",
	"CLOSING"
};

static const char *_preview_rtsp[] =
{
/* 1  */	"rtsp://%d.%d.%d.%d/live/main",
/* 3  */	"rtsp://%d.%d.%d.%d/gnz_media/main"
};

/********************* sysdb info ************************/
static gchar     _db_host[AVAILABLE_MAX_CH][256];
static guint16   _db_http_port[AVAILABLE_MAX_CH] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};
static guint8    _db_u[AVAILABLE_MAX_CH][64] = {
	"","","","", "","","","", "","","","", "","","","",
	"","","","", "","","","", "","","","", "","","",""
};
static guint8    _db_p[AVAILABLE_MAX_CH][64] = {
	"","","","", "","","","", "","","","", "","","","",
	"","","","", "","","","", "","","","", "","","",""
};
static guint8    _db_rtsp_addr_main[AVAILABLE_MAX_CH][256] = {
	"","","","", "","","","", "","","","", "","","","",
	"","","","", "","","","", "","","","", "","","",""
};
static guint8    _db_rtsp_addr_second[AVAILABLE_MAX_CH][256] = {
	"","","","", "","","","", "","","","", "","","","",
	"","","","", "","","","", "","","","", "","","",""
};
static guint16  _db_vcam[AVAILABLE_MAX_CH] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};
static guint16  _db_vcam_cnt[AVAILABLE_MAX_CH] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};
static guint16  _db_rtsp_port[AVAILABLE_MAX_CH] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};
static guint8    _db_model[AVAILABLE_MAX_CH][64] = {
	"","","","", "","","","", "","","","", "","","","",
	"","","","", "","","","", "","","","", "","","",""
};
static gchar	_db_ethernet[AVAILABLE_MAX_CH][5]; // eth0 , eth1

/*********************************************************/

/* for link-local address */

#define NET_INF_MAX		(10)
struct NET_INF_INFO
{
	char inf_name[32];
	char inf_addr[32];
	char inf_mask[32];
	int inf_type;
	int inf_pri;
};

struct ONVIF_DISCOVERY_INFO
{
	char inf_name[32];
	uint32_t inf_addr;
	uint32_t inf_mask;
	int inf_type;
	int inf_pri;
	int multi_sock;
	int port;
};

typedef enum
{
	NET_INF_WAN = 0,
	NET_INF_LAN,
	NET_INF_UNKNOWN,
	NET_INF_TYPE_MAX
} NET_INF_TYPE;

typedef enum
{
	NET_ADDR_MAIN = 0, // interface main address
	NET_ADDR_STATIC,   // static address
	NET_ADDR_LINK_LO,  // link-local address
	NET_ADDR_UNKNOWN,  // unknown address
	NET_ADDR_PRI_NAX
} NET_ADDR_PRIORITY;

typedef enum
{
	HASH_TYPE_UUID = 0, // hash to uuid string
	HASH_TYPE_ADDR,     // hash to ip address string
	HASH_TYPE_HASH_MAX
} CAM_INFO_HASH_TYPE;

#define URN_UUID_LEN	(64)
#define HASH_KEY_LEN	(64)
typedef struct hash_info
{
	char key[HASH_KEY_LEN];

	NFOpenmodeCamInfo *cam_entry;

	struct hash_info *prev;
	struct hash_info *next;
} HASH_INFO;

typedef struct
{
	HASH_INFO *head;
} HASH_LIST;

#define UUDI_HASH_MAX	(10)
#define ADDR_HASH_MAX	(10)
typedef struct
{
	HASH_LIST uuid_hash[UUDI_HASH_MAX];
	HASH_LIST addr_hash[ADDR_HASH_MAX];
} CAM_INFO_HASH;

struct __NFIPCamOpenmodeDhcpThreadInfo__
{
	int is_run;
	GThread *thread_id;
};
struct __NFIPCamOpenmodeDhcpThreadInfo__ g_openmode_dhcp_info;

static int _net_inf_cnt = 0;
static struct NET_INF_INFO _inf_tbl[NET_INF_MAX] = {{0,},};
static pthread_mutex_t _inf_tbl_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _init_net_inf_tbl(void);
static int _get_net_inf_priority(const char *ip);
static int _get_net_inf_info(void);
static void _clear_net_inf_tbl(void);
static int _is_host_ipaddr(unsigned int ipaddr);
static int _is_host_subnet(unsigned int ipaddr);

static int _discovery_cnt = 0;
static struct ONVIF_DISCOVERY_INFO _discovery_tbl[NET_INF_MAX] = {{0,},};
static pthread_mutex_t _discovery_tbl_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _init_discovery_tbl(void);
static void _clear_discovery_tbl(void);
static void _make_discovery_tbl(void);
static int _get_discovery_tbl_count(void);
static int _get_discovery_multi_sock(int idx);
static int _get_inf_priority_to_addr(uint32_t ipaddr);

static struct timespec _scan_ts = {0};
static int _scan_time_sec = 5;
static int _aibox_scan_mode = 0;
static void _openmode_cam_detect_notify(void);

static pthread_mutex_t _hash_mutex = PTHREAD_MUTEX_INITIALIZER;
static CAM_INFO_HASH _cam_info_hash = {0,};

static void _init_cam_info_hash(void);
static void _empty_cam_info_hash(void);
static HASH_INFO* _get_cam_info_hash(char *key, int key_type);
static OPENMODE_RTN_ENUM _add_cam_info_hash(char *key, int key_type, void *ptr);
static OPENMODE_RTN_ENUM _del_cam_info_hash(char *key, int key_type);
static size_t _make_hash_code(char *key, size_t hash_size);

/* for link-local address */

static const GobjMrtpPipe* h_iplive;
static int _rcv_sock = (-1);
static int _send_sock = (-1);
static int _max_ch = AVAILABLE_MAX_CH;
static unsigned int _call_cnt = 0;
static unsigned int _vloss_old = 0xffffffff;

static int _init_stay[AVAILABLE_MAX_CH];

static int _live_running = 0;
static int _discovery_running = INSTALLATION_STATE_NONE;
static OPENMODE_STATE_ENUM openmode_state = OPENMODE_STATE_CLOSE;
static NFOpenmodeDeviceList _openmode_detection_list;
static NFOpenmodeDeviceList _openmode_ch_list;
static NFOpenmodeDeviceList _openmode_live_list;
static NFOpenmodeDeviceList _openmode_recorder_list;
static NFOpenmodeCamInfo _openmode_live_data[AVAILABLE_MAX_CH];
static pthread_mutex_t _live_mtx[AVAILABLE_MAX_CH];
static unsigned int msg_id_tail = 0x0c5ba065;
static int _alloc_cnt = 0;
static int _running_detail = 0;
static int _running_live_detail = 0;
static int _last_preview_id = (-1);
static int _detail_stop_requested = 0;
static int _live_stop_requested = 0;

static unsigned int _host_ip = 0;
static unsigned int _host_subnet = 0;

static unsigned int _login_wait_cnt[AVAILABLE_MAX_CH];
static int _setup_hang_cnt[AVAILABLE_MAX_CH];

static pthread_t _openmode_rcv_th;
static pthread_mutex_t _detection_list_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _discovery_running_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t _running_live_detail_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_t _openmode_live_th[NUM_ACTIVE_CH];
static pthread_t _openmode_detection_th;
static pthread_t _openmode_vloss_th;
static pthread_t _openmode_aibox_recovery_th;

static char *_eth_dev = HOST_ETH_DEV;

static void _openmode_live_mtx_lock(int ch);
static void _openmode_live_mtx_unlock(int ch);
static void _openmode_list_mtx_lock(void);
static void _openmode_list_mtx_unlock(void);
static void _openmode_send_itx_discovery(void);
static void _openmode_load_sysdb(void);

static void _openmode_live_setup(int);
static void _openmode_recovery(int);

static void _init_switch_device(void);
static int _init_multicast_sock(void);
static void _init_admin_socks(void);
static void _send_itx_search(void);
static void _send_itx_ip_assign(netconf_msg*);
static void _send_onvif_search(void);
static int _get_available_ch(void);
static NFOpenmodeCamInfo* _list_find_entry_by_mac(NFOpenmodeDeviceList*, unsigned char*);
static NFOpenmodeCamInfo* _list_find_entry_by_ip(NFOpenmodeDeviceList*, unsigned int);
static NFOpenmodeCamInfo* _list_find_entry_by_ip_port(NFOpenmodeDeviceList*, unsigned int, unsigned short);
static NFOpenmodeCamInfo* _list_find_entry_by_index(NFOpenmodeDeviceList*, int);
static NFOpenmodeCamInfo* _list_find_entry_by_ch(NFOpenmodeDeviceList*, int);
static void _cmd_callback_func(gint op, gint cbch, gint stream, gint en, gpointer user_data);
static void _get_mac_from_str(char* src, char* dst);
static OPENMODE_RTN_ENUM _dns_lookup(char *host, char *ipstr);
static OPENMODE_RTN_ENUM _list_compare_recorder_list(NFOpenmodeCamInfo* entry);

static void _req_stop_detail(void);
static void _list_search_model_detail_live(int);
static void _list_search_model_detail(void);


static void _openmode_th_func(void);
static void _openmode_vloss_func(void);
static void _openmode_th_live_detail(void *arg);
static void _openmode_th_detection_detail(void);
static void _openmode_th_aibox_recovery(void);

static OPENMODE_RTN_ENUM _discovery_buf_find_nvt(gchar*);
static OPENMODE_RTN_ENUM _discovery_buf_find_hw(gchar*, NFOpenmodeCamInfo*);
static OPENMODE_RTN_ENUM _discovery_buf_find_uuid(gchar*, char *uuid_buf, size_t buf_size);
static OPENMODE_RTN_ENUM _discovery_buf_find_addr(gchar*, NFOpenmodeCamInfo*);
static OPENMODE_RTN_ENUM _discovery_buf_find_name_and_auth(gchar* msg, NFOpenmodeCamInfo* entry);
static OPENMODE_RTN_ENUM _discovery_detection_list_update(NFOpenmodeCamInfo*);
static OPENMODE_RTN_ENUM _discovery_onvif_handler(gchar*, struct sockaddr_in*, gchar*);
static OPENMODE_RTN_ENUM _discovery_admin_handler(netconf_msg*, struct sockaddr_in*);

static OPENMODE_RTN_ENUM _openmode_list_add(NFOpenmodeDeviceList*, NFOpenmodeCamInfo*);
static OPENMODE_RTN_ENUM _openmode_list_remove(NFOpenmodeDeviceList* list, NFOpenmodeCamInfo* del_entry);
static OPENMODE_RTN_ENUM _add_existing_ch(gint);

static OPENMODE_RTN_ENUM _openmode_init_ch_list(void);
static OPENMODE_RTN_ENUM _openmode_init_live_list(void);
static OPENMODE_RTN_ENUM _openmode_init_live_list_from_ch_list(void);
static OPENMODE_RTN_ENUM _openmode_update_live_list(void);
static OPENMODE_RTN_ENUM _openmode_finalize_live_list(void);

//static OPENMODE_RTN_ENUM _openmode_start_stream(int ch);
static OPENMODE_RTN_ENUM _openmode_start_stream(int ch, NFOpenmodeCamInfo *info);
static OPENMODE_RTN_ENUM _openmode_stop_stream(int ch);

static OPENMODE_RTN_ENUM _get_address_tail(gchar*, gchar*);
static OPENMODE_RTN_ENUM _is_itx_mac_range(guchar* dev_mac);

static void _slist_notify(void);
static unsigned int _get_ipaddr_from_hostname(gchar *hostname);

#if defined(ENABLE_PROJECT_KMW)
static void set_kmw_network(void);
#endif

static NFOpenmodeCamInfo* _sort_select_lan_mode(int from);
static NFOpenmodeCamInfo* _sort_select_model_min(int from);
static NFOpenmodeCamInfo* _sort_select_ip_min(int from);
static NFOpenmodeCamInfo* _sort_select_status_min(int from);
static NFOpenmodeCamInfo* _sort_select_ch_min(int from);
static void _sort_change_entry_pos(NFOpenmodeCamInfo *e1, NFOpenmodeCamInfo *e2);
static void	_sort_change_order(void);
static OPENMODE_RTN_ENUM _parsing_from_rtsp_address(char* rtsp_addr, NFOpenmodeVirtualCamera *vcam);
static int vcam_get_state_info_from_describe_test(gchar *username, gchar *password, gchar *rtsp_addr, guint ip_addr, guint rtsp_port, guint *audio_flag);
static gchar *vcam_remove_account_from_rtsp_addres(gchar* host, gchar* account, gchar* replace);
static gchar* _get_endline(gchar* src, gint* len);

static int _get_macaddress_by_ip(unsigned char *mac, char *ip);
int _get_macaddress_using_arping_by_ip(unsigned char *mac, char *ip, char *eth);
static int _is_linklocal_address(unsigned int ip);
static int _is_mac_null(unsigned char *mac);
static int _is_mac_equal(unsigned char *mac1, unsigned char *mac2);
// static char *_ip_to_str(unsigned int ip, char *buf);
static char *_mac_to_str(unsigned char *mac, char *buf);
static int _host_to_ip(const char *host);
static char* get_net_inf_from_cam(guint); 
static int _init_recorder_list();

extern GobjMrtpPipe* nf_live_get_mrtp_pipe_handle(void);
extern void gst_mrtp_src_rtsp_digest_auth_str(char*, char*, char*, char*, char*, char*, char*);

extern char* nf_netif_get_eth_str(void);
static uint32_t subnet_table_big_endian[32] = { // :1 ~ :32 subnet, big_endian
        128,196,224,240, 248,252,254,255, // subnet :1 ~ :8
        33023,49407,57599,61695, 63743,64767,65279,65535, // :9~:16
        8454143,12648447,14745599,15794175, 16318463,16580607,16711679,16777215, // :17~:24
        2164260863,3238002687,3774873599,4043309055, 4177526783,4244635647,4278190079,4294967295 // :25~:32
};

extern void nf_ipcam_openmode_dhcpd(void);
extern void nf_ipcam_start_udhcpd(void);

static int _init_recorder_list()
{
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;
	
	iter = _openmode_recorder_list.head;

	while(iter != NULL)
	{	
		next = iter->next;
		free(iter);
		iter = next;
	}

	memset(&_openmode_recorder_list, 0x00, sizeof(NFOpenmodeDeviceList));
	return OPENMODE_RTN_OK;
}

extern void nf_openmode_init(void)
{
    int i = 0;
	mtable *runtime;
	GAsyncQueue *vloss_queue = NULL;


	IPCAM_DBG(MAJOR, "start\n");

#if defined(GUI_32CH_SUPPORT)
	_max_ch = 32;
#elif defined(GUI_16CH_SUPPORT)
	_max_ch = 16;
#elif defined(GUI_8CH_SUPPORT)
	_max_ch = 8;
#elif defined(GUI_4CH_SUPPORT)
	_max_ch = 4;
#endif

	_eth_dev = nf_netif_get_eth_str();

	_init_net_inf_tbl();
	_init_discovery_tbl();
	_init_cam_info_hash();

	new_ds_queue();
	memset(_init_stay, 0x00, sizeof(int)*AVAILABLE_MAX_CH);
	memset(_login_wait_cnt, 0x00, sizeof(unsigned int)*AVAILABLE_MAX_CH);

	memset(&_openmode_detection_list, 0x00, sizeof(NFOpenmodeDeviceList));
	memset(&_openmode_live_list, 0x00, sizeof(NFOpenmodeDeviceList));
	memset(&_openmode_ch_list, 0x00, sizeof(NFOpenmodeDeviceList));
	memset(&_openmode_recorder_list, 0x00, sizeof(NFOpenmodeDeviceList));
    memset(&_live_mtx, 0x00, sizeof(_live_mtx));
    for(i = 0; i < AVAILABLE_MAX_CH; i++){
        pthread_mutex_init(&(_live_mtx[i]), NULL);
    }

	SSL_library_init();
	runtime = get_runtime();
	memset(runtime, 0x00, sizeof(mtable)*_max_ch);
	openmode_state = OPENMODE_STATE_INIT;
	_init_switch_device();
	h_iplive = nf_live_get_mrtp_pipe_handle();
	ipcam_vloss_queue_init();
	pthread_create(&_openmode_vloss_th, NULL, (void*)&_openmode_vloss_func, NULL);
	pthread_create(&_openmode_rcv_th, NULL, (void*)&_openmode_th_func, NULL);
	{
		vloss_queue = get_vloss_queue();
		IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
		g_async_queue_push(vloss_queue, evt);
	}
	//pthread_create(&_openmode_live_th, NULL, (void*)&_openmode_th_live_detail, NULL);
	for(i=0; i < NUM_ACTIVE_CH; i++)
	{
		pthread_create(&_openmode_live_th[i], NULL, (void*)&_openmode_th_live_detail, i);
	}
	pthread_create(&_openmode_detection_th, NULL, (void*)&_openmode_th_detection_detail, NULL);
	pthread_create(&_openmode_aibox_recovery_th, NULL, (void*)&_openmode_th_aibox_recovery, NULL);
	nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
	ipcam_manager(_max_ch);
	nmf_mrtp_pipe_set_cmd_callback(h_iplive, &_cmd_callback_func, NULL);
	_openmode_load_sysdb();

	nf_ipcam_event_notification_server();

	IPCAM_DBG(MAJOR, "end\n");
}

extern int get_openmode_state()
{
	return openmode_state;
}

extern void set_openmode_state(int state)
{
	openmode_state = state;
}
extern int get_discovery_state()
{
	return _discovery_running;
}
extern void set_discovery_state(int discovery)
{
	_discovery_running = discovery;
}

extern void nf_openmode_start(void)
{
	int i;
	mtable *runtime = get_runtime();
	IPCAM_DBG(MAJOR, "start\n");

#if 0
	if(nf_get_dual_lan_mode()){
		// POE RESET Test
			int tmp;
			printf("\e[31mPOE RESET START\e[0m\n");
		for(i=0;i<16;i++)
			nf_live_poe_port_onoff(i,0,&tmp,0);
		for(i=0;i<16;i++)
			nf_live_poe_port_onoff(i,1,&tmp,0);
			printf("\e[31mPOE RESET END\e[0m\n");
	}
#endif
	
	for (i = 0; i < _max_ch; i++)
	{
		IPCAM_DBG(MINOR, "[CH%d] MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
				i, runtime[i].state);
		runtime[i].state = MGMT_STATE_UNLINKED;
		nf_pnd_evt_notify_fire(i, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
	}

	memset(_setup_hang_cnt, 0x00, sizeof(int)*AVAILABLE_MAX_CH);

	_host_ip = get_netif_ip(_eth_dev);
	_host_subnet = get_netif_mask(_eth_dev);

	_init_multicast_sock();
	_init_admin_socks();
	_openmode_load_sysdb();
	_openmode_init_live_list();
	_openmode_init_ch_list();

	openmode_state = OPENMODE_STATE_RUNNING;

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_stop(void)
{
	GobjMrtpSrc *mrtpsrc = gst_mrtp_src_get_object();
	gint i;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *temp;
    int ch;

	IPCAM_DBG(MAJOR, "start\n");
	/* init socks */
	_clear_net_inf_tbl();
	_clear_discovery_tbl();

	if (_send_sock > 0)
	{
		close(_send_sock);
		_send_sock = (-1);
	}
	if (_rcv_sock > 0)
	{
		close(_rcv_sock);
		_rcv_sock = (-1);
	}

	while(1)
	{
		usleep(100*1000);
		if (_running_live_detail == 0)
		{
			break;
		}
	}
	openmode_state = OPENMODE_STATE_INIT;
	_live_running = 0;
	_discovery_running = INSTALLATION_STATE_NONE;

	{
		mtable *runtime = NULL;
		g_return_if_fail(h_iplive != NULL );

		runtime = get_runtime();
		g_return_if_fail( runtime != NULL );

		gst_mrtp_src_close_all(mrtpsrc, NULL);
		for (i = 0; i < NUM_ACTIVE_CH; i++)
		{
			//nmf_mrtp_pipe_close_ch(h_iplive, i);
			IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
					i, runtime[i].state);
			runtime[i].state = MGMT_STATE_UNLINKED;
			runtime[i].sys.transaction = 0;
			runtime[i].sys.ipaddr = 0;
			memset(runtime[i].sys.macaddr, 0x00, 6);
		}
		{
			GAsyncQueue *vloss_queue = get_vloss_queue();
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
			g_async_queue_push(vloss_queue, evt);
		}
	}

	memset(_db_host, 0x00, 256*AVAILABLE_MAX_CH);
	memset(_db_http_port, 0x00, sizeof(guint16)*AVAILABLE_MAX_CH);

	iter = _openmode_live_list.head;
	memset(&_openmode_live_list, 0x00, sizeof(NFOpenmodeDeviceList));
	if (iter != NULL)
	{
		for (i=0; i< _max_ch; i++)
		{
            ch = iter->ch;
            _openmode_live_mtx_lock(ch);
			temp = iter->next;
			memset(iter, 0x00, sizeof(NFOpenmodeCamInfo));
            iter->ch = -1;
			_alloc_cnt--;
			iter = temp;
            _openmode_live_mtx_unlock(ch);
		}
	}
	memset(_init_stay, 0x00, sizeof(int)*AVAILABLE_MAX_CH);

	iter = _openmode_ch_list.head;
	memset(&_openmode_ch_list, 0x00, sizeof(NFOpenmodeDeviceList));
	if (iter != NULL)
	{
		for (i=0; i< _max_ch; i++)
		{
			temp = iter->next;
			memset(iter, 0x00, sizeof(NFOpenmodeCamInfo));
			free(iter);
			_alloc_cnt--;
			iter = temp;
		}
	}

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_finalize(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	IPCAM_DBG(MAJOR, "end\n");
}





extern unsigned int nf_openmode_get_state(void)
{
	return openmode_state;
}

extern unsigned int nf_openmode_get_discovery(void)
{
	return _discovery_running;
}

extern int nf_openmode_is_installing(void)
{
	int rtn = 0;
	if (_discovery_running> 0)
	{
		rtn = 1;
	}

	return rtn;
}

extern unsigned int nf_openmode_get_live(void)
{
	return _live_running;
}

extern void nf_openmode_set_state(int state)
{
	openmode_state = state;
}

extern NFOpenmodeDeviceList* nf_openmode_get_ch_list(void)
{
	gint i = 0;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *temp;
	NFOpenmodeCamInfo *new_entry;


	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return NULL;
	}

	for (i=0; i<_max_ch; i++)
	{
		iter = _list_find_entry_by_ch(&_openmode_detection_list, i);
		temp = _list_find_entry_by_ch(&_openmode_ch_list, i);
		if (temp == NULL)
		{
			return NULL;
		}
		NFOpenmodeCamInfo *old_prev = temp->prev;
		NFOpenmodeCamInfo *old_next = temp->next;

		if (iter == NULL)
		{
			memset(temp, 0x00, sizeof(NFOpenmodeCamInfo));
		}
		else
		{
			memcpy(temp, iter, sizeof(NFOpenmodeCamInfo));
		}
		temp->index = i;
		temp->ch = i;
		temp->prev = old_prev;
		temp->next = old_next;
	}

	return (&_openmode_ch_list);
}

extern NFOpenmodeDeviceList* nf_openmode_get_list(void)
{
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return NULL;
	}
	return (&_openmode_detection_list);
}

extern NFOpenmodeDeviceList* nf_openmode_get_recorder_list(void)
{
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return NULL;
	}
	return (&_openmode_recorder_list);
}

extern void nf_openmode_stop_streaming(void)
{
	gint i, j;
	GAsyncQueue *vloss_queue = get_vloss_queue();
	NFOpenmodeCamInfo *entry = NULL;
	GobjMrtpSrc *mrtpsrc = gst_mrtp_src_get_object();


	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}
	if (_live_running == 0)
	{
		IPCAM_DBG(WARN, "closed already\n");
		return ;
	}

	_live_running = 0;
	_discovery_running = INSTALLATION_STATE_STOPPED;
	IPCAM_DBG(MAJOR, "_discovery_running state changed(INSTALLATION_STATE_STOPPED)\n");
	gst_mrtp_src_close_all(mrtpsrc, NULL);
	for (i=0; i<_max_ch; i++)
	{
		//nmf_mrtp_pipe_close_ch(h_iplive, i);
		entry = _list_find_entry_by_ch(&_openmode_live_list, i);
        if(entry){
            _openmode_live_mtx_lock(entry->ch);
            entry->state = OPENMODE_CAM_STATE_INIT;
            _openmode_live_mtx_unlock(entry->ch);
        }
	}
	{
		IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_ALL);
		evt->port = i;
		g_async_queue_push(vloss_queue, evt);
	}
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_init_detection_list(void)
{
	gint i = 0;
	guint _host = get_netif_ip(_eth_dev);
	guint _subnet = get_netif_mask(_eth_dev);
	OPENMODE_RTN_ENUM rtn;
	NFOpenmodeCamInfo *entry;
	NFOpenmodeCamInfo *det_entry;

	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}

	if (_host == 0 || _subnet == 0)
	{
		IPCAM_DBG(WARN, "setup network needed\n");
		return;
	}

	if (_discovery_running > INSTALLATION_STATE_NONE)
	{
		IPCAM_DBG(MINOR, "detection list created already\n");
		return;
	}
	else
	{
		_discovery_running = INSTALLATION_STATE_START;
	}

	for (i=0; i<_max_ch; i++)
	{
		det_entry = _list_find_entry_by_ch(&_openmode_detection_list, i);
		if (det_entry != NULL) { continue; }
#if 0
		entry = _list_find_entry_by_ch(&_openmode_live_list, i);
		if (entry == NULL) { continue; }
		if (entry->state == OPENMODE_CAM_STATE_INIT) { continue; }
#endif
		rtn = _add_existing_ch(i);
	}

	_init_recorder_list();

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_cancel(void)
{
	gint i = 0;
	OPENMODE_RTN_ENUM rtn;
	NFOpenmodeCamInfo *entry;
	NFOpenmodeCamInfo *det_entry;

	_last_preview_id = (-1);

	/* Preview channel close */
	//nmf_mrtp_pipe_close_ch(h_iplive, 0);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}
	_empty_cam_info_hash();
	nf_openmode_empty_list();
	//nf_openmode_init_detection_list();
	for (i=0; i<_max_ch; i++)
	{
		det_entry = _list_find_entry_by_ch(&_openmode_detection_list, i);
		if (det_entry != NULL) { continue; }
#if 0
		entry = _list_find_entry_by_ch(&_openmode_live_list, i);
		if (entry == NULL) { continue; }
		if (entry->state == OPENMODE_CAM_STATE_INIT) { continue; }
#endif
		rtn = _add_existing_ch(i);
	}

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}
}

extern void nf_openmode_apply(void)
{
	gint i;

	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}
	
	_last_preview_id = (-1);
#if 0
	gint i;
	gint rtn;
	NFOpenmodeCamInfo *c_iter;
	NFOpenmodeCamInfo *l_iter;
	NFOpenmodeCamInfo *l_prev;
	NFOpenmodeCamInfo *l_next;


	IPCAM_DBG(MAJOR, "start\n");

	/* Preview channel close */
	nmf_mrtp_pipe_close_ch(h_iplive, 0);
	l_iter = _openmode_live_list.head;
	l_prev = l_iter->prev;
	l_next = l_iter->next;
	for (i=0; i<_max_ch; i++)
	{
		c_iter = _list_find_entry_by_ch(&_openmode_ch_list, i);
		if (c_iter == NULL)
		{
			memset(l_iter, 0x00, sizeof(NFOpenmodeCamInfo));
			l_iter->index = i;
			l_iter->ch = i;
			l_iter->prev = l_prev;
			l_iter->next = l_next;
			l_iter = l_iter->next;
			l_prev = l_iter->prev;
			l_next = l_iter->next;
			continue;
		}
		memcpy(l_iter, c_iter, sizeof(NFOpenmodeCamInfo));
		l_iter->index = i;
		l_iter->prev = l_prev;
		l_iter->next = l_next;
		l_iter = l_iter->next;
		l_prev = l_iter->prev;
		l_next = l_iter->next;
	}
#endif

	/* Preview channel close */
	//nmf_mrtp_pipe_close_ch(h_iplive, 0);
	//_openmode_finalize_live_list();
	//_openmode_load_sysdb();
	//_openmode_init_live_list_from_ch_list();
	for (i=0; i<_max_ch; i++)
	{
		nmf_mrtp_pipe_close_ch(h_iplive, i);
	}

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_finalize_installation(void)
{
	gint i;
	NFOpenmodeCamInfo* iter;


	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}
#if 0
	iter = _openmode_live_list.head;
	for (i=0; i<_max_ch; i++)
	{
		if (iter->state != OPENMODE_CAM_STATE_INIT)
		{
			iter->state = OPENMODE_CAM_STATE_DISCOVERED;
		}
		iter = iter->next;
	}
	for (i=0; i<_max_ch; i++)
	{
		nmf_mrtp_pipe_close_ch(h_iplive, i);
	}
#endif
	if (_last_preview_id >= 0)
	{
		nmf_mrtp_pipe_close_ch(h_iplive, 0);
		_last_preview_id = (-1);
	}
	//_live_running = 0;
	if (_live_running == 0)
	{
		_openmode_finalize_live_list();
		_openmode_load_sysdb();
		_openmode_init_live_list_from_ch_list();
	}
	_empty_cam_info_hash();
	nf_openmode_empty_list();
	_discovery_running = INSTALLATION_STATE_FINAL;
	IPCAM_DBG(MAJOR, "end\n");
}


extern void nf_openmode_scan_camera(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return;
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
		IPCAM_DBG(WARN, "scanning now\n");
		return;
	}

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		openmode_state = OPENMODE_STATE_SCANNING;
	}
	//_scan_time_sec = 3;
	memset(&_scan_ts, 0x00, sizeof(_scan_ts));
	clock_gettime(CLOCK_REALTIME, &_scan_ts);

	nmf_mrtp_pipe_close_ch(h_iplive, 0);
	//nf_openmode_empty_list();
	_send_onvif_search();
	_send_itx_search();
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_scan_aibox(int sec)
{
	IPCAM_DBG(MAJOR, "start\n");
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return;
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
		IPCAM_DBG(WARN, "scanning now\n");
		return;
	}

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		openmode_state = OPENMODE_STATE_SCANNING;
	}
	
	_aibox_scan_mode = 1;
	if(sec <= 0){
		_scan_time_sec = 3;
	}else{
		_scan_time_sec = sec;
	}
	memset(&_scan_ts, 0x00, sizeof(_scan_ts));
	clock_gettime(CLOCK_REALTIME, &_scan_ts);

	_send_itx_search();
	IPCAM_DBG(MAJOR, "end\n");
}


extern gint nf_openmode_get_network(gint index, NFOpenmodeSetupNetwork* info)
{
	NFOpenmodeCamInfo *iter;
	OPENMODE_RTN_ENUM rtn;


	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return OPENMODE_RTN_FAIL;
	}

	iter = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (iter == NULL) { return OPENMODE_RTN_FAIL; }

	cam_get_network(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, info);
}

extern gint nf_openmode_add_device_manual(gchar* host, guint port)
{
	gchar ipstr[16];
	guint ipaddr=0;
	OPENMODE_RTN_ENUM rtn;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *new_entry;


	IPCAM_DBG(MAJOR, "start host(%s:%d)\n", host, port);

	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return OPENMODE_RTN_FAIL;
	}
	new_entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
	_alloc_cnt++;
	if (new_entry == NULL)
	{
		IPCAM_DBG(ERROR, "DetectionList Manual add entry allocation failure\n");
		_alloc_cnt--;
		return OPENMODE_RTN_FAIL;
	}

	ipaddr = inet_addr(host);
	if (ipaddr == 0xffffffff)
	{
		rtn = _dns_lookup(host, ipstr);
		if (rtn == OPENMODE_RTN_FAIL)
		{
			IPCAM_DBG(WARN, "DNS lookup failed(%s)\n", host);
			free(new_entry);
			_alloc_cnt--;
			return OPENMODE_RTN_FAIL;
		}
		IPCAM_DBG(MINOR, "DNS lookup (%s: %s)\n", host, ipstr);
		ipaddr = inet_addr(ipstr);
	}

	ipaddr = ntohl(ipaddr);
	iter = _list_find_entry_by_ip_port(&_openmode_detection_list, ipaddr, port);
	if (iter != NULL)
	{
		IPCAM_DBG(WARN, "duplicated IP Address\n");
		free(new_entry);
		_alloc_cnt--;
		return OPENMODE_RTN_FAIL;
	}

	memset(new_entry, 0x00, sizeof(NFOpenmodeCamInfo));
	new_entry->index = _openmode_detection_list.entry_cnt;
	new_entry->state = OPENMODE_CAM_STATE_DISCOVERED;
	new_entry->ch = _get_available_ch();
	new_entry->ipaddr = ipaddr;
	new_entry->http_port = port;
	/*
	snprintf(new_entry->hostname, 16, "%d.%d.%d.%d",
			(ipaddr&0xff000000)>>24,
			(ipaddr&0xff0000)>>16,
			(ipaddr&0xff00)>>8,
			(ipaddr&0xff));
	*/
	snprintf(new_entry->hostname, sizeof(new_entry->hostname), "%s", host);
	_openmode_list_add(&_openmode_detection_list, new_entry);
	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,1,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,1,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,1,0,0);
#endif
	}

	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

extern gint nf_openmode_add_virtual_camera(gchar* host_main, gchar* host_second, gchar* model_name, OPENMODE_STATE_VIRTUAL_CAM_ADD *v_state)
{
	int vcam_cnt, i;
	OPENMODE_RTN_ENUM rtn;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *new_entry;
	NFOpenmodeVirtualCamera vcam[2];

	IPCAM_DBG(MAJOR, "start rtsp host_main(%s)\n", host_main);
	IPCAM_DBG(MAJOR, "start rtsp host_second(%s)\n", host_second);

	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL;
		return OPENMODE_RTN_FAIL;
	}
	new_entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
	_alloc_cnt++;
	if (new_entry == NULL)
	{
		IPCAM_DBG(ERROR, "DetectionList virtual camera add entry allocation failure\n");
		_alloc_cnt--;
		*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL;
		return OPENMODE_RTN_FAIL;
	}
	memset(new_entry, 0x00, sizeof(NFOpenmodeCamInfo));
	memset(&vcam[0], 0x00, sizeof(NFOpenmodeVirtualCamera));
	memset(&vcam[1], 0x00, sizeof(NFOpenmodeVirtualCamera));

	new_entry->http_port = 80;

	if((strcmp(host_main, "") == 0 || host_main == NULL ) && (strcmp(host_second, "") == 0 || host_main == NULL))
	{
		vcam_cnt = 0;
	}
	else if((strcmp(host_main, "") == 0 || host_main == NULL ) || (strcmp(host_second, "") == 0 || host_main == NULL))
	{
		vcam_cnt = 1;
	}
	else
	{
		vcam_cnt = 2;
	}
	new_entry->vcam_cnt= vcam_cnt;
	
	for(i = 0; i < vcam_cnt; i++)
	{
		if( i == 0 ) 
		{
			rtn = _parsing_from_rtsp_address(host_main, &vcam[i]);
			if (rtn == OPENMODE_RTN_FAIL)
			{
				*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_MAIN_ADDR_FAIL;
				free(new_entry);
				_alloc_cnt--;
				return OPENMODE_RTN_FAIL;
			}
		}
		if( i == 1 ) 
		{
			rtn = _parsing_from_rtsp_address(host_second, &vcam[i]); 
			if (rtn == OPENMODE_RTN_FAIL)
			{
				*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_SECOND_ADDR_FAIL;
				free(new_entry);
				_alloc_cnt--;
				return OPENMODE_RTN_FAIL;
			}
		}
	}

	/* 계정 정보가 포함된 rtsp 주소에서 계정 제거 */
	gchar account_u[64];
	gchar account_p[64];
	gchar *semicolon = ":";
	gchar *at = "@";
	gchar account[256];

	for(i = 0; i < vcam_cnt; i++)
	{
		strncpy(account_u, vcam[i].u_id, 64);
		strncpy(account_p, vcam[i].u_password, 64);
		snprintf(account, 256, "%s%s%s%s", account_u, semicolon , account_p, at);
		if( i == 0 ) 
		{
			host_main = vcam_remove_account_from_rtsp_addres(host_main, account, "");
			if (host_main == NULL)
			{
				*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL;
				free(new_entry);
				_alloc_cnt--;
				return OPENMODE_RTN_FAIL;
			}
		}
		if( i == 1 ) 
		{
			host_second = vcam_remove_account_from_rtsp_addres(host_second, account, "");
			if (host_second == NULL)
			{
				*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL;
				free(new_entry);
				_alloc_cnt--;
				return OPENMODE_RTN_FAIL;
			}
		}
	}



	new_entry->index = _openmode_detection_list.entry_cnt;
	new_entry->state = OPENMODE_CAM_STATE_DISCOVERED;
	new_entry->ch = _get_available_ch();
	new_entry->ipaddr = vcam[0].ipaddr;
	strncpy(new_entry->model, model_name, 64);

	for( i = 0; i < vcam_cnt; i++ )
	{
		if( i == 1 )
		{
			strncpy(new_entry->vcam_rtsp_addr[i], host_second, 256);
			break;
		}
		strncpy(new_entry->u, vcam[i].u_id, 64);
		strncpy(new_entry->p, vcam[i].u_password, 64);
		strncpy(new_entry->vcam_rtsp_addr[i], host_main, 256);
		new_entry->rtsp_port= vcam[i].rtsp_port;
	}

	new_entry->virtual_camera = OPENMODE_STATE_VIRTUAL_SUPPORTED;
/*
	snprintf(new_entry->hostname, 16, "%d.%d.%d.%d",
			(vcam[0].ipaddr&0xff000000)>>24,
			(vcam[0].ipaddr&0xff0000)>>16,
			(vcam[0].ipaddr&0xff00)>>8,
			(vcam[0].ipaddr&0xff));
*/
	snprintf(new_entry->hostname, sizeof(new_entry->hostname), "%s", vcam[0].host);
	_openmode_list_add(&_openmode_detection_list, new_entry);
	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,1,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,1,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,1,0,0);
#endif
	}

	*v_state = OPENMODE_STATE_VIRTUAL_CAM_ADD_OK;
	IPCAM_DBG(MAJOR, "virtual_camera end\n");
	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_change_password(gint index, gchar *p)
{
	int rtn;
	NFOpenmodeCamInfo* entry;


	IPCAM_DBG(MAJOR, "start(%s)\n", p);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return OPENMODE_RTN_FAIL;
	}

	entry = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (entry == NULL)
	{
		IPCAM_DBG(WARN, "no entry index(%d)\n", index);
		return OPENMODE_RTN_FAIL;
	}
#if 0
	if (entry->use_ssl == 0)
	{
		IPCAM_DBG(WARN, "no initial state(%d-%s)\n", index, entry->model);
		return OPENMODE_RTN_FAIL;
	}
#endif
	if (entry->http_port == 0)
	{
		IPCAM_DBG(WARN, "no http port addressed(%d)\n", index);
		return OPENMODE_RTN_FAIL;
	}
	if (entry->ipaddr == 0)
	{
		IPCAM_DBG(WARN, "no ip address(%d)\n", index);
		return OPENMODE_RTN_FAIL;
	}

	if (entry->state != OPENMODE_CAM_STATE_PW_CHANGE)
	{
		IPCAM_DBG(MINOR, "Normal password change(%d)\n", index);
		rtn = cam_set_password(htonl(entry->ipaddr), entry->http_port, entry->u_done, entry->p_done, p);
		if (rtn > 2)
		{
			strncpy(entry->p, p, 64);
			entry->state = OPENMODE_CAM_STATE_DISCOVERED;
			return OPENMODE_RTN_OK;
		}
		return OPENMODE_RTN_FAIL;
	}

	IPCAM_DBG(MINOR, "Initial password change(%d)\n", index);
	char userID[64];
	char userPW[64];
	memset(userID, 0x00, 64);
	memset(userPW, 0x00, 64);

	nf_ipcam_get_default_login_info(userID, userPW);

	rtn = cam_set_initial_pw(htonl(entry->ipaddr), entry->http_port, userID, userPW, p);

	if (entry->state == OPENMODE_CAM_STATE_PW_CHANGE)
	{
		if (rtn > 2)
		{
			strncpy(entry->u, "ADMIN", 64);
			strncpy(entry->p, p, 64);
			strncpy(entry->u_done, "ADMIN", 64);
			strncpy(entry->p_done, p, 64);
			entry->state = OPENMODE_CAM_STATE_DISCOVERED;
			rtn = OPENMODE_RTN_OK;
		}
		else
		{
			rtn = OPENMODE_RTN_FAIL;
		}
	}

	IPCAM_DBG(MAJOR, "end(%d)\n", rtn);
	return rtn;
}

extern OPENMODE_RTN_ENUM nf_openmode_set_login_info(gint index, gchar *u, gchar *p)
{
	gint i=0;
	gint rtn_itx = 0;
	gint rtn_onvif = 0;
	gint rtn_vcam= 0;
	gint use_ssl = 0;
	guchar _temp_mac[8];
	NFOpenmodeCamInfo *iter;
	NFOpenmodeSetupNetwork netinfo;
	cam_model_info info;

	OPENMODE_RTN_ENUM rtn;


	IPCAM_DBG(MAJOR, "start index(%d) user(%s) pass(*)\n", index, u);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return (2);
		//return OPENMODE_RTN_FAIL;
	}

	if (index >= _openmode_detection_list.entry_cnt)
	{
		IPCAM_DBG(WARN, "Wrong index(%d), entry count(%d)\n", index, _openmode_detection_list.entry_cnt);
		return (2);
		//return OPENMODE_RTN_FAIL;
	}

	iter = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (iter == NULL)
	{
		return (2);
		//return OPENMODE_RTN_FAIL;
	}

#if 1
	strncpy(iter->u, u, 64);
	strncpy(iter->p, p, 64);
	
	if(iter->virtual_camera == OPENMODE_STATE_VIRTUAL_SUPPORTED)
	{
		int audio_flag = 0;
		rtn_vcam = vcam_get_state_info_from_describe_test(iter->u, iter->p, iter->vcam_rtsp_addr[0], iter->ipaddr, iter->rtsp_port, &audio_flag);

		if (rtn_vcam == 0)
		{
			rtn = OPENMODE_RTN_OK;

		}
		else if ( rtn_vcam == -1 )
		{
			rtn = 2;
		}
		else
		{
			rtn = 1;
		}
	}
	else
	{
#if 1
		rtn_itx = cam_get_model_info_raw(&info, htonl(iter->ipaddr), iter->http_port, iter->u, iter->p, &use_ssl);

		if (rtn_itx >= 2) 
		{
			rtn = OPENMODE_RTN_OK;
		}
#if 0
		else
		{
			rtn = OPENMODE_RTN_FAIL;
		}
#else
		else if (rtn_itx == 1)
		{
			rtn = 2;
		}
		else
		{
			if (nf_ipcam_is_vendor_s1())
			{
				rtn_onvif = nf_onvif_get_net_info(htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 1, iter->u, iter->p, _temp_mac, &netinfo);
			}
			else
			{
				rtn_onvif = nf_onvif_get_net_info(htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 0, iter->u, iter->p, _temp_mac, &netinfo);
			}

			if (rtn_onvif != 0)
			{
				if (nf_ipcam_is_vendor_s1())
				{
					rtn_onvif = nf_onvif_get_net_info(htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 1, iter->u, iter->p, _temp_mac, &netinfo);
				}
				else
				{
					rtn_onvif = nf_onvif_get_net_info(htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 0, iter->u, iter->p, _temp_mac, &netinfo);
				}
			}

			if (rtn_onvif != 0)
			{
				rtn = 2;
				//rtn = OPENMODE_RTN_FAIL;
				goto no_status_change;
			}
			else
			{
				rtn = 1;
				//rtn = OPENMODE_RTN_OK;
			}
		}
#endif

#endif

	}

	if (iter->state >= OPENMODE_CAM_STATE_CONN_FAIL)
	{
		IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
				__OPENMODE_CAM_STATE_STR_[iter->state],
				__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_DISCOVERED]);
		iter->state = OPENMODE_CAM_STATE_DISCOVERED;
	}
#endif


no_status_change:

	//IPCAM_DBG(MAJOR, "end - index(%d) username(%s) password(%s)\n", index, u, p);
	return rtn;
}


extern OPENMODE_RTN_ENUM nf_openmode_request_port_assign(int index, NFOpenmodeSetupPorts* info)
{
	gint i;
	gint rtn;
	NFOpenmodeCamInfo *iter;


	IPCAM_DBG(MAJOR, "start id(%d) http(%d) rtsp(%d)\n", index, info->http_port, info->rtsp_port);

	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return OPENMODE_RTN_FAIL;
	}

	iter = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (iter == NULL) { return OPENMODE_RTN_FAIL; }
	if (iter->state != OPENMODE_CAM_STATE_DEV_INFO && iter->state != OPENMODE_CAM_STATE_OK)
	{
		IPCAM_DBG(WARN, "camera status is what cannot be configured\n");
		return OPENMODE_RTN_FAIL;
	}
	_openmode_stop_stream(0);

	rtn = cam_set_service_ports(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, info);

	if (rtn <= 0)
	{
		IPCAM_DBG(WARN, "network configuration failed\n");
		return OPENMODE_RTN_FAIL;
	}

	iter->http_port = info->http_port;
	iter->rtsp_port = info->rtsp_port;

	nf_openmode_set_preview(iter->index, 0);
	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}

	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_request_ip_assign(int index, NFOpenmodeSetupNetwork* info)
{
	gint i;
	gint rtn;
	NFOpenmodeCamInfo *iter;
	netconf_msg ipsetmsg;


	IPCAM_DBG(MAJOR, "start\n");
	IPCAM_DBG(MINOR, "is_dhcp(%d) ip(%d.%d.%d.%d) mask(%d.%d.%d.%d) gateway(%d.%d.%d.%d) dns1(%d.%d.%d.%d) dns2(%d.%d.%d.%d) http(%d) rtsp(%d)\n",
			info->is_dhcp,
			(info->ipaddr&0xff000000)>>24,
			(info->ipaddr&0xff0000)>>16,
			(info->ipaddr&0xff00)>>8,
			(info->ipaddr&0xff),
			(info->mask&0xff000000)>>24,
			(info->mask&0xff0000)>>16,
			(info->mask&0xff00)>>8,
			(info->mask&0xff),
			(info->gw&0xff000000)>>24,
			(info->gw&0xff0000)>>16,
			(info->gw&0xff00)>>8,
			(info->gw&0xff),
			(info->dns1&0xff000000)>>24,
			(info->dns1&0xff0000)>>16,
			(info->dns1&0xff00)>>8,
			(info->dns1&0xff),
			(info->dns2&0xff000000)>>24,
			(info->dns2&0xff0000)>>16,
			(info->dns2&0xff00)>>8,
			(info->dns2&0xff),
			info->http_port,
			info->rtsp_port);

	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return OPENMODE_RTN_FAIL;
	}
#if 0
	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		openmode_state = OPENMODE_STATE_SCANNING;
	}

	/* Request to set the IP address to the IP Camera */
	memset((void*)&ipsetmsg, 0x00, sizeof(netconf_msg));
#endif

	iter = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (iter == NULL) { return OPENMODE_RTN_FAIL; }
	if (iter->state != OPENMODE_CAM_STATE_DEV_INFO && iter->state != OPENMODE_CAM_STATE_OK)
	{
		IPCAM_DBG(WARN, "camera status is what cannot be configured\n");
		return OPENMODE_RTN_FAIL;
	}

	_openmode_stop_stream(0);
	rtn = cam_set_network(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, info);

	if (rtn <= 0)
	{
		IPCAM_DBG(WARN, "network configuration failed\n");
		return OPENMODE_RTN_FAIL;
	}

#if 0
	return OPENMODE_RTN_FAIL;

	for (i=0; (iter->xid == 0); i++)
	{
		IPCAM_DBG(MINOR, "waiting transaction id(%d) ch(%d) xid(%08x)\n", iter->index, iter->ch, iter->xid);
		if (i > 5) { break; }

		_send_itx_search();
		usleep(100*1000);
	}
	if (iter->xid == 0 ) { return OPENMODE_RTN_FAIL; }

	ipsetmsg.version = (info->is_dhcp == 1) ? 0x10:0x01;
	ipsetmsg.opcode = MSG_IP_SET;
	ipsetmsg.secs = 0;
	ipsetmsg.xid = htonl(iter->xid);
	ipsetmsg.magic = htonl(0x69547843);
	ipsetmsg.ciaddr = htonl(iter->ipaddr);
	memcpy(&ipsetmsg.chaddr[0], &iter->macaddr[0], 6);
	ipsetmsg.yiaddr = htonl(info->ipaddr);
	ipsetmsg.miaddr = htonl(info->mask);
	ipsetmsg.giaddr = htonl(info->gw);
	ipsetmsg.d1iaddr = htonl(info->dns1);
	ipsetmsg.d2iaddr = htonl(info->dns2);
	ipsetmsg.http_port = htons(iter->http_port);
	ipsetmsg.rtsp_port = htons(iter->rtsp_port);
	_send_itx_ip_assign(&ipsetmsg);
#endif

	iter->is_dhcp = info->is_dhcp;
	iter->ipaddr = info->ipaddr;
	iter->mask = info->mask;
	iter->gw = info->gw;
	iter->dns1 = info->dns1;
	iter->dns2 = info->dns2;

	snprintf(iter->hostname, 256, "%d.%d.%d.%d",
			(info->ipaddr&0xff000000)>>24,
			(info->ipaddr&0xff0000)>>16,
			(info->ipaddr&0xff00)>>8,
			(info->ipaddr&0xff));
	snprintf(iter->gwstr, 16, "%d.%d.%d.%d",
			(info->gw&0xff000000)>>24,
			(info->gw&0xff0000)>>16,
			(info->gw&0xff00)>>8,
			(info->gw&0xff));
	snprintf(iter->maskstr, 16, "%d.%d.%d.%d",
			(info->mask&0xff000000)>>24,
			(info->mask&0xff0000)>>16,
			(info->mask&0xff00)>>8,
			(info->mask&0xff));
	snprintf(iter->dns1str, 16, "%d.%d.%d.%d",
			(info->dns1&0xff000000)>>24,
			(info->dns1&0xff0000)>>16,
			(info->dns1&0xff00)>>8,
			(info->dns1&0xff));
	snprintf(iter->dns2str, 16, "%d.%d.%d.%d",
			(info->dns2&0xff000000)>>24,
			(info->dns2&0xff0000)>>16,
			(info->dns2&0xff00)>>8,
			(info->dns2&0xff));
	//iter->state = OPENMODE_CAM_STATE_DISCOVERED;
	nf_openmode_set_preview(iter->index, 0);
	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}
	IPCAM_DBG(MAJOR, "end\n");

	return OPENMODE_RTN_OK;
}

extern void nf_openmode_set_preview(gint index, gint is_chlist)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	IPCAM_DBG(MAJOR, "start index(%d)\n", index);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}
	if (index >= _openmode_detection_list.entry_cnt)
	{
		IPCAM_DBG(ERROR, "wrong index, list count(%d)\n", _openmode_detection_list.entry_cnt);
		return;
	}
	if (_openmode_live_list.head != NULL && _openmode_live_list.head->state == OPENMODE_CAM_STATE_OK)
	{
		IPCAM_DBG(WARN, "live channel 0 is running\n");
		return;
	}
	if (index < 0)
	{
		IPCAM_DBG(ERROR, "close preview\n");
		nmf_mrtp_pipe_close_ch(h_iplive, 0);
		return;
	}

	nmf_mrtp_pipe_close_ch(h_iplive, 0);
	if (is_chlist)
	{
		iter = _list_find_entry_by_index(&_openmode_ch_list, index);
	}
	else
	{
		iter = _list_find_entry_by_index(&_openmode_detection_list, index);
	}
	if (iter == NULL) { return; }
	switch(iter->state)
	{
		case OPENMODE_CAM_STATE_DEV_INFO:
		{
			NMFMrtpPipeChannel info;

			memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

			IPCAM_DBG(MINOR, "itx cam(%d - %s)\n", index, iter->model);
			if (strncmp(iter->vendor, "GANZ", 4) == 0 && strncmp(iter->model, "ZN", 2) == 0)
			{
				snprintf(iter->preview_rtsp, 256, _preview_rtsp[1],
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff)
				);
			}
			else
			{
				snprintf(iter->preview_rtsp, 256, _preview_rtsp[0],
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff)
				);
			}

			info.ch_num = 0;
			info.model_code = NF_IPCAM_MODEL_AMB_A2;
			info.username = iter->u_done;
			info.password = iter->p_done;
			info.video_cnt = 1;
			info.video[0].resolution = RES_1920x1080;
			info.video[0].ip_addr = htonl(iter->ipaddr);
			info.video[0].rtsp_port = iter->rtsp_port;
			info.video[0].rtsp_addr = iter->preview_rtsp;
			nmf_mrtp_pipe_set_dev_mac(h_iplive, 0, &iter->macaddr[0]);

			_last_preview_id = index;

			nmf_mrtp_pipe_open_ch(h_iplive, &info);

			IPCAM_DBG(MINOR, "addr(%s)\n", iter->preview_rtsp);
			break;
		}
		case OPENMODE_CAM_STATE_DEV_INFO_ONVIF:
		{
			NMFMrtpPipeChannel info;

			memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

			IPCAM_DBG(MINOR, "onvif cam(%d - %s)\n", index, iter->model);

			info.ch_num = 0;
			info.model_code = NF_IPCAM_MODEL_ONVIF;
			info.username = iter->u_done;
			info.password = iter->p_done;
			info.video_cnt = 1;
			info.video[0].resolution = RES_1920x1080;
			info.video[0].ip_addr = htonl(iter->ipaddr);
			info.video[0].rtsp_port = iter->rtsp_port;
			info.video[0].rtsp_addr = iter->preview_rtsp;
			nmf_mrtp_pipe_set_dev_mac(h_iplive, 0, &iter->macaddr[0]);

			_last_preview_id = index;

			nmf_mrtp_pipe_open_ch(h_iplive, &info);

			IPCAM_DBG(MINOR, "addr(%s)\n", iter->preview_rtsp);
			break;
		}
		case OPENMODE_CAM_STATE_VIRTUAL_CAMERA:
		{
			NMFMrtpPipeChannel info;

			memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

			IPCAM_DBG(MINOR, "virtual cam(%d - %s)\n", index, iter->model);

			info.ch_num = 0;
			info.model_code = NF_IPCAM_MODEL_ONVIF;
			info.username = iter->u;
			info.password = iter->p;
			info.video_cnt = 1;
			//info.video[0].resolution = RES_1920x1080;
			info.video[0].resolution = RES_1920x1080;
			info.video[0].ip_addr = htonl(iter->ipaddr);
			info.video[0].rtsp_port = iter->rtsp_port;
			info.video[0].rtsp_addr = iter->vcam_rtsp_addr[0];
			nmf_mrtp_pipe_set_dev_mac(h_iplive, 0, &iter->macaddr[0]);

			_last_preview_id = index;

			nmf_mrtp_pipe_open_ch(h_iplive, &info);

			IPCAM_DBG(MINOR, "addr(%s)\n", iter->vcam_rtsp_addr[0]);
			break;
		}
		default:
		{
			_last_preview_id = index;
			break;
		}
	}

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_set_channel(int index, int ch)
{
	gint i;
	gint old_ch;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *entry;


	IPCAM_DBG(MAJOR, "start index(%d) ch(%d)\n", index, ch);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}

	if (ch >= _max_ch)
	{
		IPCAM_DBG(WARN, "wrong channel(%d)\n", ch);
		return;
	}

	entry = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (entry == NULL)
	{
		IPCAM_DBG(ERROR, "no index(%d) entry\n", index);
		return;
	}
	old_ch = entry->ch;
	entry->ch = ch;

	if (ch >= 0 && ch < _max_ch)
	{
		iter = _openmode_detection_list.head;
		for (i=0; i<_openmode_detection_list.entry_cnt; i++)
		{
			if (iter == entry)
			{
				iter = iter->next;
				continue;
			}
			if (iter->ch == ch)
			{
				iter->ch = old_ch;
				break;
			}
			iter = iter->next;
		}
	}

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_set_channel_no_noti(int index, int ch)
{
	gint i;
	gint old_ch;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *entry;


	IPCAM_DBG(MAJOR, "start index(%d) ch(%d)\n", index, ch);
	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}

	if (ch >= _max_ch)
	{
		IPCAM_DBG(WARN, "wrong channel(%d)\n", ch);
		return;
	}

	entry = _list_find_entry_by_index(&_openmode_detection_list, index);
	if (entry == NULL)
	{
		IPCAM_DBG(ERROR, "no index(%d) entry\n", index);
		return;
	}
	old_ch = entry->ch;
	entry->ch = ch;

	if (ch >= 0 && ch < _max_ch)
	{
		iter = _openmode_detection_list.head;
		for (i=0; i<_openmode_detection_list.entry_cnt; i++)
		{
			if (iter == entry)
			{
				iter = iter->next;
				continue;
			}
			if (iter->ch == ch)
			{
				iter->ch = old_ch;
				break;
			}
			iter = iter->next;
		}
	}

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_empty_list(void)
{
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *temp = NULL;

	if (openmode_state == OPENMODE_STATE_CLOSE)
	{
		IPCAM_DBG(WARN, "close mode now\n");
		return ;
	}

	IPCAM_DBG(MAJOR, "start\n");

	if (_running_detail)
	{
		_req_stop_detail();
	}
	while(_running_detail) { usleep(10*1000); }
	_openmode_list_mtx_lock();

	iter = _openmode_detection_list.head;

	_openmode_detection_list.head = NULL;
	_openmode_detection_list.tail = NULL;
	_openmode_detection_list.entry_cnt = 0;
	_openmode_detection_list.assigned_cnt = 0;
	_openmode_detection_list.setup_needed_cnt = 0;
	_openmode_detection_list.recognized_cnt = 0;

	while (iter != NULL)
	{
		temp = iter->next;
		memset(iter, 0x00, sizeof(NFOpenmodeCamInfo));
		free(iter);
		_alloc_cnt--;
		iter = temp;
	}
	_init_recorder_list();

	_openmode_list_mtx_unlock();

	if (openmode_state == OPENMODE_STATE_RUNNING)
	{
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
	}
	else if (openmode_state == OPENMODE_STATE_SCANNING)
	{
#if BLOCKING_SCAN
		nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
		nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
	}

	IPCAM_DBG(MAJOR, "end\n");
}


extern void nf_openmode_retry_login(gint ch)
{
	NFOpenmodeCamInfo *iter;
	gchar key[128];

	IPCAM_DBG(MAJOR, "start ch(%d)\n", ch);
	iter = _list_find_entry_by_ch(&_openmode_live_list, ch);

	if (iter == NULL)
	{
		IPCAM_DBG(MINOR, "end - no assigned ch\n");
		return;
	}

    _openmode_live_mtx_lock(ch);
	if (iter->state != OPENMODE_CAM_STATE_LOGIN_FAIL)
	{
        _openmode_live_mtx_unlock(iter->ch);
		IPCAM_DBG(MINOR, "end - no login fail state\n");
		return;
	}

	snprintf(key, 128, "cam.logininfo.L%d.id", ch);
	strncpy(iter->u, nf_sysdb_get_str_nocopy(key), 256);
	snprintf(key, 128, "cam.logininfo.L%d.pwd", ch);
	strncpy(iter->p, nf_sysdb_get_str_nocopy(key), 256);

	//IPCAM_DBG(MINOR, "id(%s) pw(%s)\n", iter->u, iter->p);

	strncpy(_db_u[ch], iter->u, 64);
	strncpy(_db_p[ch], iter->p, 64);

	iter->state = OPENMODE_CAM_STATE_DISCOVERED;
    _openmode_live_mtx_unlock(ch);
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_reconnect_ch(gint ch)
{
	printf("[khkh] CH(%d) nf_openmode_reconnect_ch \n ", ch);
	NFOpenmodeCamInfo *iter;
	mtable *runtime = get_runtime();

	IPCAM_DBG(MAJOR, "start ch(%d)\n", ch);
	iter = _list_find_entry_by_ch(&_openmode_live_list, ch);

	if (iter == NULL)
	{
		IPCAM_DBG(WARN, "no entry(%d)\n", ch);
		return;
	}

    _openmode_live_mtx_lock(ch);
	nmf_mrtp_pipe_close_ch(h_iplive, ch);
	iter->state = OPENMODE_CAM_STATE_DISCOVERED;
    _openmode_live_mtx_unlock(ch);

	IPCAM_DBG(MAJOR, "end\n");
}


extern void nf_openmode_show_ch_list(void)
{
	gint i = 0;
	NFOpenmodeCamInfo *iter = NULL;


	IPCAM_DBG(MAJOR, "start\n");

	printf("========================================================================\n");
	printf("Assigned(%d) Discovered(%d) SetupNeeded(%d)\n",
			_openmode_ch_list.assigned_cnt, _openmode_ch_list.entry_cnt, _openmode_ch_list.setup_needed_cnt);
	iter = _openmode_ch_list.head;
	for (i=0; i<_openmode_ch_list.entry_cnt; i++)
	{
		printf("  %d - STATE(%s) CH(%d) HW(%s) VENDOR(%s) U(%s) XAddr(http://%s:%d/%s)\n",
				iter->index, __OPENMODE_CAM_STATE_STR_[iter->state], iter->ch,
				iter->model, iter->vendor,
				iter->u, iter->hostname,
				iter->http_port,
				iter->tail
		);
		iter = iter->next;
	}
	printf("========================================================================\n");

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_show_list(void)
{
	gint i = 0;
	NFOpenmodeCamInfo *iter = NULL;


	IPCAM_DBG(MAJOR, "start\n");

	printf("== 1. Live list ========================================================\n");
	printf(" Assigned(%d) Discovered(%d) SetupNeeded(%d)\n",
			_openmode_live_list.assigned_cnt, _openmode_live_list.entry_cnt, _openmode_live_list.setup_needed_cnt);
	iter = _openmode_live_list.head;
	for (i=0; i<_max_ch; i++)
	{
		printf("  %d - STATE(%s) CH(%d) HW(%s) VENDOR(%s) U(%s) XAddr(http://%s:%d/%s)\n",
				iter->index, __OPENMODE_CAM_STATE_STR_[iter->state], iter->ch,
				iter->model, iter->vendor,
				iter->u, iter->hostname,
				iter->http_port,
				iter->tail
		);
		iter = iter->next;
	}
	printf("== 2. Ch list ==========================================================\n");
	printf(" Assigned(%d) Discovered(%d) SetupNeeded(%d)\n",
			_openmode_ch_list.assigned_cnt, _openmode_ch_list.entry_cnt, _openmode_ch_list.setup_needed_cnt);
	iter = _openmode_ch_list.head;
	for (i=0; i<_openmode_ch_list.entry_cnt; i++)
	{
		printf("  %d - STATE(%s) CH(%d) HW(%s) VENDOR(%s) U(%s) XAddr(http://%s:%d/%s)\n",
				iter->index, __OPENMODE_CAM_STATE_STR_[iter->state], iter->ch,
				iter->model, iter->vendor,
				iter->u, iter->hostname,
				iter->http_port,
				iter->tail
		);
		iter = iter->next;
	}
	printf("== 3. Detection list ===================================================\n");
	printf(" Assigned(%d) Discovered(%d) SetupNeeded(%d)\n",
			_openmode_detection_list.assigned_cnt, _openmode_detection_list.entry_cnt, _openmode_detection_list.setup_needed_cnt);
	iter = _openmode_detection_list.head;
	for (i=0; i<_openmode_detection_list.entry_cnt; i++)
	{
		printf("  %d - STATE(%s) CH(%d) HW(%s) VENDOR(%s) U(%s) XAddr(http://%s:%d/%s)\n",
				iter->index, __OPENMODE_CAM_STATE_STR_[iter->state], iter->ch,
				iter->model, iter->vendor,
				iter->u, iter->hostname,
				iter->http_port,
				iter->tail
		);
		iter = iter->next;
	}
	printf("========================================================================\n");

	printf("== 4. Recorder list =======================================================\n");
	printf(" Recorder Count(%d)\n", _openmode_recorder_list.entry_cnt);
	iter = _openmode_recorder_list.head;
	for (i=0; i<_openmode_recorder_list.entry_cnt; i++)
	{
		printf("  %d - HW(%s) VENDOR(%s) U(%s) XAddr(http://%s:%d/%s)\n",
				iter->index,
				iter->model, iter->vendor,
				iter->u, iter->hostname,
				iter->http_port,
				iter->tail
		);
		iter = iter->next;
	}
	printf("========================================================================\n");

	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_openmode_show_entry(gchar *name, gint index)
{
	gint i=0;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeDeviceList *list = NULL;

	IPCAM_DBG(MAJOR, "start index(%d)\n", index);

	if (strcmp(name, "d") == 0)
	{
		list = &_openmode_detection_list;
	}
	else if (strcmp(name, "c") == 0)
	{
		list = &_openmode_ch_list;
	}
	else if (strcmp(name, "l") == 0)
	{
		list = &_openmode_live_list;
	}

	if(list != NULL)
	{
		if (index >= list->entry_cnt)
		{
			IPCAM_DBG(WARN, "Wrong index(%d), entry count(%d)\n", index, list->entry_cnt);
			return;
		}
	}
	else
	{

		IPCAM_DBG(WARN, "list is NULL\n");
		return;
	}


	iter = _list_find_entry_by_index(list, index);
#if 0
	iter = list->head;
	for (i=0; i<index; i++)
	{
		iter = iter->next;
	}
#endif

	printf("  =================================== %02d ==============================\n",
			iter->index);
	printf("    index (%d)\n    state (%s)\n    ch    (%d)\n    ip    (%d.%d.%d.%d)\n"
			"    gw    (%d.%d.%d.%d)\n    dns1  (%d.%d.%d.%d)\n    dns2  (%d.%d.%d.%d)\n"
			"    mask  (%d.%d.%d.%d)\n    xid   (%08x)\n    dhcp  (%d)\n",
			iter->index, __OPENMODE_CAM_STATE_STR_[iter->state], iter->ch,
			(iter->ipaddr&0xff000000)>>24,
			(iter->ipaddr&0xff0000)>>16,
			(iter->ipaddr&0xff00)>>8,
			(iter->ipaddr&0xff),
			(iter->gw&0xff000000)>>24,
			(iter->gw&0xff0000)>>16,
			(iter->gw&0xff00)>>8,
			(iter->gw&0xff),
			(iter->dns1&0xff000000)>>24,
			(iter->dns1&0xff0000)>>16,
			(iter->dns1&0xff00)>>8,
			(iter->dns1&0xff),
			(iter->dns2&0xff000000)>>24,
			(iter->dns2&0xff0000)>>16,
			(iter->dns2&0xff00)>>8,
			(iter->dns2&0xff),
			(iter->mask&0xff000000)>>24,
			(iter->mask&0xff0000)>>16,
			(iter->mask&0xff00)>>8,
			(iter->mask&0xff),
			iter->xid,
			iter->is_dhcp);
	printf("    http  (%d)\n    rtsp  (%d)\n    mac   (%02x-%02x-%02x-%02x-%02x-%02x)\n    model (%s)\n",
			iter->http_port, iter->rtsp_port, iter->macaddr[0], iter->macaddr[1], iter->macaddr[2], iter->macaddr[3],
			iter->macaddr[4], iter->macaddr[5], iter->model);
	printf("    fwver (%s)\n    vendor(%s)\n    tail  (%s)\n    media (%s)\n    token (%s)\n    user  (%s)\n    pass  (%s)\n",
			iter->firmware_version, iter->vendor, iter->tail, iter->media_xaddr, iter->token, iter->u, iter->p);
	printf("    u_done(%s)\n    p_done(%s)\n    auth  (%d)\n    ssl   (%d)\n    rtsp  (%s)\n",
			iter->u_done, iter->p_done, iter->auth, iter->use_ssl, iter->preview_rtsp);
	printf("    hostname(%s) gwstr(%s) maskstr(%s) dns1str(%s) dns2str(%s)\n",
			iter->hostname, iter->gwstr, iter->maskstr, iter->dns1str, iter->dns2str);
	printf("  =====================================================================\n");
}

extern OPENMODE_RTN_ENUM nf_custommode_sort_by_lan(int dir)
{
	NFOpenmodeDeviceList *_list = &_openmode_detection_list;
	NFOpenmodeCamInfo *start = _list->head;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;

	IPCAM_DBG(MAJOR, "Direction(%d)\n", dir);
	while (start != NULL)
	{
		next = start->next;
		iter = _sort_select_lan_mode(start->index);
		if (iter == NULL)
		{
			IPCAM_DBG(WARN, "Error: Entry not found\n");
			break;
		}
		_sort_change_entry_pos(start, iter);
		start = next;
		//nf_openmode_show_list();
	}

	if (dir != OPENMODE_SORT_ASC)
	{
		_sort_change_order();
	}
	nf_notify_fire_params("ipcam_slist", 0,0,0,0);

	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_sort_by_model(int dir)
{
	NFOpenmodeDeviceList *_list = &_openmode_detection_list;
	NFOpenmodeCamInfo *start = _list->head;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;

	IPCAM_DBG(MAJOR, "Direction(%d)\n", dir);
	while (start != NULL)
	{
		next = start->next;
		iter = _sort_select_model_min(start->index);
		if (iter == NULL)
		{
			IPCAM_DBG(WARN, "Error: Entry not found\n");
			break;
		}
		_sort_change_entry_pos(start, iter);
		start = next;
		//nf_openmode_show_list();
	}

	if (dir != OPENMODE_SORT_ASC)
	{
		_sort_change_order();
	}
	nf_notify_fire_params("ipcam_slist", 0,0,0,0);

	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_sort_by_ip(int dir)
{
	NFOpenmodeDeviceList *_list = &_openmode_detection_list;
	NFOpenmodeCamInfo *start = _list->head;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;

	while (start != NULL)
	{
		next = start->next;
		iter = _sort_select_ip_min(start->index);
		_sort_change_entry_pos(start, iter);
		start = next;
	}

	if (dir != OPENMODE_SORT_ASC)
	{
		_sort_change_order();
	}
	nf_notify_fire_params("ipcam_slist", 0,0,0,0);

	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_sort_by_status(int dir)
{
	NFOpenmodeDeviceList *_list = &_openmode_detection_list;
	NFOpenmodeCamInfo *start = _list->head;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;

	while (start != NULL)
	{
		next = start->next;
		iter = _sort_select_status_min(start->index);
		_sort_change_entry_pos(start, iter);
		start = next;
	}

	if (dir != OPENMODE_SORT_ASC)
	{
		_sort_change_order();
	}
	nf_notify_fire_params("ipcam_slist", 0,0,0,0);

	return OPENMODE_RTN_OK;
}

extern OPENMODE_RTN_ENUM nf_openmode_sort_by_ch(int dir)
{
	NFOpenmodeDeviceList *_list = &_openmode_detection_list;
	NFOpenmodeCamInfo *start = _list->head;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *next = NULL;

	while (start != NULL)
	{
		next = start->next;
		iter = _sort_select_ch_min(start->index);
		_sort_change_entry_pos(start, iter);
		start = next;
	}

	if (dir != OPENMODE_SORT_ASC)
	{
		_sort_change_order();
	}
	nf_notify_fire_params("ipcam_slist", 0,0,0,0);

	return OPENMODE_RTN_OK;
}

NFOpenmodeCamInfo* nf_openmode_get_dlist_info_by_chlist_index(int index)
{
	NFOpenmodeDeviceList *dlist;
	NFOpenmodeCamInfo *dinfo;
	NFOpenmodeDeviceList *clist;
	NFOpenmodeCamInfo *cinfo;

	dlist = &_openmode_detection_list;
	clist = &_openmode_ch_list;

	cinfo = _list_find_entry_by_index(clist, index);
	if (cinfo == NULL)
	{
		IPCAM_DBG(ERROR, "entry not found in channel list index(%d)", index);
		return NULL;
	}

	dinfo = _list_find_entry_by_ch(dlist, cinfo->ch);

	return (dinfo);
}

extern int nf_openmode_get_dhcpd_state(void)
{
	return 	g_openmode_dhcp_info.is_run;
}

extern int nf_openmode_dhcpd_start()
{
	g_openmode_dhcp_info.is_run = 1;
	g_openmode_dhcp_info.thread_id = g_thread_create((GThreadFunc)nf_ipcam_start_udhcpd, NULL, TRUE, NULL);
	
	if (g_openmode_dhcp_info.thread_id == 0) {
		printf("\e[31m [%s] openmode_dhcp_thread_create is failed... \e[0m\n", __FUNCTION__);
		//g_openmode_dhcp_info.is_run = 0;
		return IPCAM_SETUP_RTN_FAILED;
	}
	else{
		printf("\e[31m [%s] openmode_dhcp_thread created \e[0m\n", __FUNCTION__);
		return IPCAM_SETUP_RTN_DONE;
	}	
}

extern int nf_openmode_dhcpd_stop()
{
#ifdef USE_PROXY_SYSTEM
	proxy_system("killall -9 udhcpd",1,3);
#else
	system("killall -9 udhcpd");
#endif

	if(g_openmode_dhcp_info.is_run == 1)
	{

		if(g_openmode_dhcp_info.thread_id != 0)
		{
			printf("\e[31m [%s] openmode_dhcp_thread_join \e[0m\n", __FUNCTION__);
			g_thread_join(g_openmode_dhcp_info.thread_id);
		}
		else
		{
			printf("\e[31m [%s] openmode_dhcp_thread is not found ... \e[0m\n", __FUNCTION__);
		}
	}
	else
	{
		printf("\e[31m [%s] openmode_dhcp is not start ... \e[0m\n", __FUNCTION__);
	}
	g_openmode_dhcp_info.is_run = 0;

	return IPCAM_SETUP_RTN_DONE;
	/*
	if(g_openmode_dhcp_info.is_run) {
		printf("\e[31m [%s] openmode_dhcp_thread_join \e[0m\n", __FUNCTION__);
		g_openmode_dhcp_info.is_run = 0;

		proxy_system("killall -9 udhcpd",1,3);
		g_thread_join(g_openmode_dhcp_info.thread_id);
	}
	else
		printf("\e[31m [%s] openmode_dhcp_thread is not found ... \e[0m\n", __FUNCTION__);

	return IPCAM_SETUP_RTN_DONE;
	*/
}

extern int is_valid_c_class(unsigned int p_ip, unsigned int p_subnet)
{
	unsigned int compare_subnet = 4294967040;
	unsigned int result_subnet = compare_subnet & p_subnet;
	unsigned int last_subnet_mask = 0;

	if(((p_ip&0xff) < 192) || ((p_ip&0xff) > 239))
		return 0;
	if((p_subnet&0xff) != 255) 
		return 0;
	if(((p_subnet&0xff00)>>8) != 255)
		return 0;
	if(((p_subnet&0xff0000)>>16) != 255)
		return 0;

	last_subnet_mask = (p_subnet&0xff000000)>>24;

	switch(last_subnet_mask)
	{
		case 0:
			return 1;
		case 128:
			return 1;
		case 192:
			return 1;
		case 224:
			return 1;
		case 240:
			return 1;
		case 248:
			return 1;
		case 252:
			return 1;
		case 254:
			return 1;
		case 255:
			return 1;
		default:
			return 0; 
	}
}







static void _openmode_live_mtx_lock(int ch)
{
    if(ch < 0 || ch >= AVAILABLE_MAX_CH){
        printf("[%s:%d] error ch[%d]\n", __func__, __LINE__, ch);
        return;
    }
	pthread_mutex_lock(&(_live_mtx[ch]));
}

static void _openmode_live_mtx_unlock(int ch)
{
    if(ch < 0 || ch >= AVAILABLE_MAX_CH){
        printf("[%s:%d] error ch[%d]\n", __func__, __LINE__, ch);
        return;
    }
	pthread_mutex_unlock(&(_live_mtx[ch]));
}

static void _openmode_list_mtx_lock(void)
{
	pthread_mutex_lock(&_detection_list_mtx);
}

static void _openmode_list_mtx_unlock(void)
{
	pthread_mutex_unlock(&_detection_list_mtx);
}

static void _slist_notify(void)
{
	IPCAM_DBG(WARN, "notify fire\n");
	nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
}

static void _openmode_th_live_detail(void *arg)
{
	int ch = (int)arg;
	while(1)
	{
		//IPCAM_DBG(MINOR, "LIVE THREAD RUNNING\n");
		sleep(1);

		if (openmode_state == OPENMODE_STATE_INIT)
		{
			IPCAM_DBG(MINOR, "LIVE skipped by OPENMODE_STATE\n");
			continue;
		}
		if (_discovery_running > INSTALLATION_STATE_NONE)
		{
			IPCAM_DBG(MINOR, "LIVE skipped by INSTALLATION_STATE\n");
			continue;
		}


		pthread_mutex_lock(&_running_live_detail_mtx);
		_running_live_detail |= (1 << ch);
		pthread_mutex_unlock(&_running_live_detail_mtx);
		/* Connection check */
		_openmode_live_setup(ch);
		_openmode_recovery(ch);
		pthread_mutex_lock(&_running_live_detail_mtx);
		_running_live_detail &= ~(1 << ch);
		pthread_mutex_unlock(&_running_live_detail_mtx);
		//_live_stop_requested = 0;
	}
}

static void _openmode_th_detection_detail(void)
{
	while(1)
	{
		//IPCAM_DBG(MINOR, "DETECTION THREAD RUNNING\n");
		sleep(1);

		if (openmode_state == OPENMODE_STATE_INIT)
		{
			continue;
		}

		//_openmode_list_mtx_lock();
		_running_detail = 1;
		_list_search_model_detail();
		_running_detail = 0;
		_detail_stop_requested = 0;
		//_openmode_list_mtx_unlock();
	}
}

static void _openmode_th_aibox_recovery(void)
{
	while(1)
	{
		sleep(10);
		if(is_zmq_running())
		{
			//aibox_zmq_recovery_process();
			nf_api_aibox_conn_check();
		}
	}
}

static void _openmode_vloss_func(void)
{
	unsigned int vloss_status = 0;

	while (1)
	{
		sleep(1);

		vloss_status = get_vloss_status();
		if (vloss_status != _vloss_old)
		{
			IPCAM_DBG(MINOR, "vloss notify fire (%08x->%08x)\n", _vloss_old, vloss_status);
			_vloss_old = vloss_status;
			nf_notify_fire_params("vloss", vloss_status, 0,0,0);
		}
#if defined(ENABLE_PROJECT_KMW)
		{
			unsigned int addr = 0;
			sleep(1);
			addr = get_local_net_ip();
			if (addr == 0)
			{
				set_kmw_network();
			}
		}
#endif
	}
}

static void _openmode_cam_detect_notify(void)
{
	int assigned_cnt = 0;
	int setup_needed_cnt = 0;
	int recog_cnt = 0;
	NFOpenmodeCamInfo *iter = NULL;

	iter = _openmode_detection_list.head;
	for (iter=_openmode_detection_list.head; iter != NULL; iter=iter->next)
	{
		if (iter == NULL) { break; }
		if (iter->ch >= 0) { assigned_cnt++; }

		switch (iter->state)
		{
			case OPENMODE_CAM_STATE_DEV_INFO:
			case OPENMODE_CAM_STATE_DEV_INFO_ONVIF:
			case OPENMODE_CAM_STATE_OK:
				recog_cnt++;
				break;
			case OPENMODE_CAM_STATE_REQ_IP:
			case OPENMODE_CAM_STATE_PW_CHANGE:
			case OPENMODE_CAM_STATE_LOGIN_FAIL:
				setup_needed_cnt++;
				break;
			default:
				break;
		}
	}

	if ((assigned_cnt != _openmode_detection_list.assigned_cnt) ||
		(setup_needed_cnt != _openmode_detection_list.setup_needed_cnt) ||
		(recog_cnt != _openmode_detection_list.recognized_cnt)
	)
	{
		_openmode_detection_list.assigned_cnt = assigned_cnt;
		_openmode_detection_list.setup_needed_cnt = setup_needed_cnt;
		_openmode_detection_list.recognized_cnt = recog_cnt;
		if (openmode_state == OPENMODE_STATE_SCANNING)
		{
#if BLOCKING_SCAN
			nf_notify_fire_params("ipcam_slist", 1,0,0,0);
#else
			nf_notify_fire_params("ipcam_slist", 0,0,0,0);
#endif
		}
		else if (openmode_state == OPENMODE_STATE_RUNNING)
		{
			nf_notify_fire_params("ipcam_slist", 0,0,0,0);
		}
	}
}

static void _openmode_th_func(void)
{
	int idx = 0;
	int len = 0;
	int buf_sz = 8192;
	char *buf = NULL;
	struct sockaddr_in cin;
	int cin_len = 0;
	int recv_flag = 0;
	int discovery_cnt = 0;
	int multi_sock = -1;
	struct timespec now_ts;

	netconf_msg received;
	mtable *runtime = get_runtime();


	IPCAM_DBG(MAJOR, "thread start\n");

	buf = (char*) malloc(buf_sz);
	while(1)
	{

		IPCAM_DBG(MINOR, "alloc_cnt(%d) openmode_state(%d) _discovery_running(%d)\n", _alloc_cnt, openmode_state, _discovery_running);

		/* Discovery */
		if (openmode_state == OPENMODE_STATE_INIT)
		{
			sleep(1);
			continue;
		}

		if(openmode_state == OPENMODE_STATE_SCANNING)
		{
			memset(&now_ts, 0x00, sizeof(now_ts));
			clock_gettime(CLOCK_REALTIME, &now_ts);

			if ((now_ts.tv_sec - _scan_ts.tv_sec) > _scan_time_sec)
			{
				IPCAM_DBG(MINOR, "scan end[%d]\n", (int)(now_ts.tv_sec - _scan_ts.tv_sec));
				openmode_state = OPENMODE_STATE_RUNNING;
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
				_aibox_scan_mode = 0;
				continue;
			}

			discovery_cnt = _get_discovery_tbl_count();
			if (discovery_cnt > 0 || _rcv_sock > 0)
			{
				/* DISCOVERY response handling loop */
				do
				{
					recv_flag = 0;

					/* ONVIF camera */
					for(idx = 0; idx < discovery_cnt; idx++)
					{
						multi_sock = _get_discovery_multi_sock(idx);
						if(multi_sock < 0) { continue; }

						memset(buf, 0x00, (size_t)buf_sz);
						len = recvfrom(multi_sock, buf, buf_sz, MSG_DONTWAIT|MSG_PEEK, NULL, NULL);
						if (len > 0)
						{
							cin_len = sizeof(cin);
							len = recvfrom(multi_sock, buf, buf_sz, MSG_DONTWAIT, (struct sockaddr*)&cin, &cin_len);
							IPCAM_DBG(MINOR, "discovery onvif ip[%s]\n", _ip_to_str(ntohl(cin.sin_addr.s_addr), NULL));
							_discovery_onvif_handler(buf, &cin, _discovery_tbl[idx].inf_name);
							recv_flag = 1;
						}
					}

					/* ITX camera */
					memset((void*)&received, 0x00, sizeof(netconf_msg));
					len = recvfrom(_rcv_sock, (void*)&received, sizeof(netconf_msg), MSG_DONTWAIT|MSG_PEEK,NULL,NULL);
					if (len > 0)
					{
						cin_len = sizeof(cin);
						len = recvfrom(_rcv_sock, (void*)&received, sizeof(netconf_msg), MSG_DONTWAIT, (struct sockaddr*)&cin, &cin_len);

						if(_is_aibox(&received)){
							IPCAM_DBG(MINOR, "is aibox\n");
							_discovery_aibox(&received, &cin);
						}else{
							IPCAM_DBG(MINOR, "ipset discovery itx ip[%s]\n", _ip_to_str(ntohl(cin.sin_addr.s_addr), NULL));
							_discovery_admin_handler(&received, &cin);
							recv_flag = 1;
						}

					}

					usleep(1000);

				} while(recv_flag != 0);
			}
			else
			{
				IPCAM_DBG(ERROR, "multi_sock get fail\n");
			}
		}
		else
		{
			// nf_openmode_show_list();
			sleep(1);
		}

		if (_discovery_running > INSTALLATION_STATE_NONE)
		{
			_openmode_cam_detect_notify();
		}
	}
}

static void _openmode_live_setup(int ch)
{
	int i;
	int rtn;
	mtable *runtime = get_runtime();
	NFOpenmodeCamInfo *iter;

	GTimeVal now_time;
	unsigned int now_sec;
	unsigned int setup_time;
	int diff_time;

	//IPCAM_DBG(MINOR, "live setup\n");
	if (openmode_state == OPENMODE_STATE_INIT)
	{
		IPCAM_DBG(MINOR, "OPENMODE INIT state\n");
		return;
	}
	pthread_mutex_lock(&_discovery_running_mtx);
	if(_discovery_running != INSTALLATION_STATE_NONE)
	{
		if(_discovery_running == INSTALLATION_STATE_FINAL)
		{
			sleep(1);
			// 'Connecting' Notify 발생
			iter = _openmode_live_list.head;
			for(i=0; i<_max_ch; i++)
			{
				if (iter == NULL) {break;}
				if (runtime[i].state & MGMT_STATE_CONFIGURED) { iter=iter->next; continue; }
				if ((iter->ipaddr != 0 && iter->http_port != 0))
				{
					nf_pnd_prog_notify_fire(i, 5, __LINE__, __FILE__);
				}
				iter = iter->next;
			}

			sleep(2); // 기존 로직 그대로 계승 (총 3초 이후 live_running logic 실행)
			_discovery_running = INSTALLATION_STATE_NONE;
		}
	}

	pthread_mutex_unlock(&_discovery_running_mtx);

	_live_running = 1;
	iter = _openmode_live_list.head;
	for (i=0; i<_max_ch; i++)
	{
		if (iter == NULL) { break; }
		if (openmode_state == OPENMODE_STATE_INIT) { break; }
		if (_live_running == 0) { break; }
		if (i != ch){ iter = iter->next; continue;}

		_openmode_live_mtx_lock(iter->ch);
		switch(iter->state)
		{
			case OPENMODE_CAM_STATE_INIT:
			{
				runtime[i].state = MGMT_STATE_UNLINKED;
				if (_init_stay[i] > 0)
				{
					IPCAM_DBG(MINOR, "stay INIT CH(%d) state(%d)\n", i, _init_stay[i]);
					_init_stay[i]--;
					break;
				}
				
				if (iter->ipaddr != 0 && iter->http_port != 0)
				{
					iter->state = OPENMODE_CAM_STATE_DISCOVERED;
					g_message("###yanggungg : %s, %d DEVICE DISCOVERED : CH(%d) IP(%d.%d.%d.%d) HTTP_PORT(%d)\n", __func__, __LINE__, iter->ch,
							(iter->ipaddr&0xff000000)>>24,
							(iter->ipaddr&0xff0000)>>16,
							(iter->ipaddr&0xff00)>>8,
							(iter->ipaddr&0xff),
							iter->http_port);
				}
				break;
			}
			
			case OPENMODE_CAM_STATE_VIRTUAL_CAMERA:
			{
				if ((runtime[iter->ch].state & (
					OPENMODE_STATE_CONFIGURING|
					OPENMODE_STATE_CONFIG_FAIL| OPENMODE_STATE_READY))==0)
				{
					int k;
					runtime[iter->ch].state = OPENMODE_STATE_READY|MGMT_STATE_LINKED|MGMT_STATE_READY|MGMT_STATE_CONFIGURED;

					runtime[iter->ch].sys.model_code = NF_IPCAM_MODEL_ONVIF;
					strncpy(runtime[iter->ch].username, iter->u, 64);
					strncpy(runtime[iter->ch].password, iter->p, 64);
					runtime[iter->ch].video.stream_cnt = iter->vcam_cnt;
					runtime[iter->ch].sys.ipaddr = htonl(iter->ipaddr);

					runtime[iter->ch].sys.rtsp_port[0] = iter->rtsp_port;
					runtime[iter->ch].sys.rtsp_port[1] = iter->rtsp_port;

					strncpy(runtime[iter->ch].sys.rtsp_url[0], iter->vcam_rtsp_addr[0], 256);
					strncpy(runtime[iter->ch].sys.rtsp_url[1], iter->vcam_rtsp_addr[1], 256);

					strncpy(runtime[iter->ch].sys.model, iter->model, 64);
					runtime[iter->ch].audio.audio_tx = iter->vcam_audio_flag;
					if(iter->vcam_audio_flag == 1)
					{
						runtime[iter->ch].func |= NF_IPCAM_FUNC_AUDIO_TX;
					}
				}
				else if (runtime[iter->ch].state & OPENMODE_STATE_READY)
				{
					IPCAM_DBG(MINOR, "DEV_VCAM - READY CH(%d)\n", iter->ch);
					iter->state = OPENMODE_CAM_STATE_STREAM_OPEN_REQ;
					_openmode_start_stream(iter->ch, iter);
					_setup_hang_cnt[iter->ch] = 0;

#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE READY : %d", iter->ch); nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_DEVICE_READY, log_buf);
					}
#endif


				}
				else if (runtime[iter->ch].state == OPENMODE_STATE_CONFIG_FAIL)
				{
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					iter->state = OPENMODE_CAM_STATE_STREAM_FAIL;
					_setup_hang_cnt[iter->ch] = 0;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
					}
#endif
				}
				else	// configuring
				{
					// TODO. count timeout
					_setup_hang_cnt[iter->ch]++;
				}

				if (_setup_hang_cnt[iter->ch] > 100) 
				{
					IPCAM_DBG(MINOR, "DEV_INFO_VCAM - setup timeout CH(%d)\n", iter->ch);
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					_setup_hang_cnt[iter->ch] = 0;
					iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
					runtime[iter->ch].state = OPENMODE_STATE_CONFIG_FAIL;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
					}
#endif

				}
				break;
			}
			
			case OPENMODE_CAM_STATE_DEV_INFO:
			{
				if ((runtime[iter->ch].state & (
					OPENMODE_STATE_CONFIGURING|
					OPENMODE_STATE_CONFIG_FAIL| OPENMODE_STATE_READY))==0)
				{
					IPCAM_DBG(MINOR, "DEV_INFO - CONFIGURING CH(%d)\n", iter->ch);

					rtn = build_management_table_openmode(iter);

					if(rtn == IPX_SEARCH_FOUND_NOT_SUPPORTED) {
						g_message("###yanggungg : %s, %d DEVICE NOT SUPPORTED : CH(%d) IP(%d.%d.%d.%d) HTTP_PORT(%d)\n", __func__, __LINE__, iter->ch,
							(iter->ipaddr&0xff000000)>>24,
							(iter->ipaddr&0xff0000)>>16,
							(iter->ipaddr&0xff00)>>8,
							(iter->ipaddr&0xff),
							iter->http_port);
						nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_UNSUPPORTED, __LINE__, __FILE__);
						nf_eventlog_put_ipcam_msg("Not Supported(ITX)", iter->ch);
						iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
						break;
					}

					if (rtn < 4)
					{
						iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
						nf_eventlog_put_ipcam_msg("Connection Fail(ITX)", iter->ch);
						break;
					}

					nf_openmode_cam_prepare(iter->ch);

					_setup_hang_cnt[iter->ch] = 0;
				}
				else if (runtime[iter->ch].state & OPENMODE_STATE_READY)
				{
					IPCAM_DBG(MINOR, "DEV_INFO - READY CH(%d)\n", iter->ch);

					g_get_current_time(&now_time);
					now_sec = now_time.tv_sec;
					setup_time = runtime[iter->ch].setup_time;

					diff_time = now_sec - setup_time;

					if(setup_time != 0 && diff_time - 4 < 0 && nf_ipcam_is_hisilicon_camera(iter->ch))
					{
						IPCAM_DBG(MAJOR, "Event: Retry: IPCAM_EVENT_STREAM_READY(CH:[%02d] now time(%d) setup time (%d)\n", iter->ch, now_sec, setup_time );
						break;
					}

					//resetting runtime setup time  
					runtime[iter->ch].setup_time = 0;

					nf_eventlog_put_ipcam_msg("Ready to stream open", iter->ch);
					iter->state = OPENMODE_CAM_STATE_STREAM_OPEN_REQ;
					_openmode_start_stream(iter->ch, iter);
					_setup_hang_cnt[iter->ch] = 0;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE READY : %d", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_DEVICE_READY, log_buf);
					}
#endif
				}
				else if (runtime[iter->ch].state == OPENMODE_STATE_CONFIG_FAIL)
				{
					int k=0;
					IPCAM_DBG(MINOR, "DEV_INFO - CONFIG FAIL CH(%d)\n", iter->ch);
					for (k=0; k < NF_IPCAM_TYPE_MAX; k++)
					{
						nf_ipcam_setup_clear(iter->ch, k);
						pthread_mutex_lock(&runtime[iter->ch].sys.ssl_mtx[k]);
						runtime[iter->ch].sys.ssl_state[k] = IPCAM_SSL_NOT_AVAILABLE;
						_release_resource(0, NULL, &runtime[iter->ch].sys.ssl[k], &runtime[iter->ch].sys.ctx[k]);
						pthread_mutex_unlock(&runtime[iter->ch].sys.ssl_mtx[k]);
						//SSL_session_free(runtime[iter->ch].sys.ssn[i]);
					}
					_release_resource(NULL, NULL, &runtime[iter->ch].sys.ssl_g, &runtime[iter->ch].sys.ctx_g);
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					nf_eventlog_put_ipcam_msg("Configuration Fail(ITX)", iter->ch);
					iter->state = OPENMODE_CAM_STATE_STREAM_FAIL;
					_setup_hang_cnt[iter->ch] = 0;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
					}
#endif
				}
				else
				{
					IPCAM_DBG(MINOR, "DEV_INFO - no handler CH(%d)\n", iter->ch);
					// TODO. count timeout
					_setup_hang_cnt[iter->ch]++;
				}
				
				if (_setup_hang_cnt[iter->ch] > 100) 
				{
					IPCAM_DBG(MINOR, "DEV_INFO - setup timeout CH(%d)\n", iter->ch);
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					_setup_hang_cnt[iter->ch] = 0;
					iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
					runtime[iter->ch].state = OPENMODE_STATE_CONFIG_FAIL;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
					}
#endif
				}

				break;
			}

			case OPENMODE_CAM_STATE_DEV_INFO_ONVIF:
			{
				if ((runtime[iter->ch].state & (
					OPENMODE_STATE_CONFIGURING|
					OPENMODE_STATE_CONFIG_FAIL| OPENMODE_STATE_READY))==0)
				{
					rtn = build_management_table_openmode_onvif(iter);
					if (rtn < 4)
					{
						if(rtn == IPX_SEARCH_LOGIN_FAIL)
						{
							nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
							nf_eventlog_put_ipcam_msg("Login Fail(ONVIF)", iter->ch);
							iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
							break;
						}
						else
						{
							nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
							nf_eventlog_put_ipcam_msg("Connection Fail(ONVIF)", iter->ch);
							iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
							break;
						}
					}
					nf_openmode_cam_prepare_onvif(iter->ch);
					_setup_hang_cnt[iter->ch] = 0;
				}
				else if (runtime[iter->ch].state & OPENMODE_STATE_READY)
				{
					nf_eventlog_put_ipcam_msg("Ready to stream open", iter->ch);
					iter->state = OPENMODE_CAM_STATE_STREAM_OPEN_REQ;
					_openmode_start_stream(iter->ch, iter);
					_setup_hang_cnt[iter->ch] = 0;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE READY : %d", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_DEVICE_READY, log_buf);
					}
#endif
				}
				else if (runtime[iter->ch].state == OPENMODE_STATE_CONFIG_FAIL)
				{
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					iter->state = OPENMODE_CAM_STATE_STREAM_FAIL;
					_setup_hang_cnt[iter->ch] = 0;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
						nf_eventlog_put_ipcam_msg("Configuration Fail(ONVIF)", iter->ch);
					}
#endif
				}
				else	// configuring
				{
					// TODO. count timeout
					_setup_hang_cnt[iter->ch]++;
				}

				if (_setup_hang_cnt[iter->ch] > 100) 
				{
					IPCAM_DBG(MINOR, "DEV_INFO_ONVIF - setup timeout CH(%d)\n", iter->ch);
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
					_setup_hang_cnt[iter->ch] = 0;
					iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
					runtime[iter->ch].state = OPENMODE_STATE_CONFIG_FAIL;
#if MAKE_NOTIFY_FIRE
					{
						GTimeVal tval;
						char log_buf[128];

						gettimeofday(&tval, NULL);
						snprintf(log_buf, 128, "DEVICE CONFIGURATION FAILED : CH(%d)", iter->ch);
						nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_CONFIGURAION_FAIL, log_buf);
					}
#endif
				}
				break;
			}
#if 0
			case OPENMODE_CAM_STATE_OK:
			{
				if ((runtime[i].state & MGMT_STATE_USING) == 0)
				{
					iter->state = OPENMODE_CAM_STATE_DISCOVERED;
				}
				break;
			}
#endif
			default:
				break;
		}
            _openmode_live_mtx_unlock(iter->ch);

		iter = iter->next;
	}
	//_openmode_list_mtx_lock();
	_list_search_model_detail_live(ch);
	//_openmode_list_mtx_unlock();
}

static void _openmode_recovery(int ch)
{
	int i;
	unsigned int vloss_status;
	mtable *runtime = get_runtime();
	NFOpenmodeCamInfo *iter;
	static int _abnormal_cnt[AVAILABLE_MAX_CH] = {0, };//{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

	if (openmode_state == OPENMODE_STATE_INIT)
	{
		return;
	}

	if (_discovery_running != INSTALLATION_STATE_NONE)
	{
		IPCAM_DBG(MINOR, "discovery running\n");
		return;
	}

	//IPCAM_DBG(MAJOR, "recovery running\n");

	for(iter = _openmode_live_list.head; iter != NULL; iter = iter->next)
	{
		i = iter->ch;

		if(i != ch){ continue; }
		
		if (openmode_state == OPENMODE_STATE_INIT)
		{
			return;
		}

		if (_discovery_running != INSTALLATION_STATE_NONE)
		{
			IPCAM_DBG(MINOR, "discovery running\n");
			return;
		}

		if(i < 0 || i >= _max_ch){
			printf("[%s:%d] _openmode_live_list ch[%d] error", __func__, __LINE__, iter->ch);
			continue;
		}
#if 0
		IPCAM_DBG(ERROR, "CH(%d) abnormal_cnt(%d) runtime(%08x) openmode(%s)\n",
				i,
				_abnormal_cnt[i], runtime[i].state, __OPENMODE_CAM_STATE_STR_[iter->state]);
#endif
		if (_abnormal_cnt[i] > 20)
		{
			_openmode_live_mtx_lock(iter->ch);

			if (_abnormal_cnt[i] <= 20){
				_openmode_live_mtx_unlock(iter->ch);
				continue;
			}

			runtime[iter->ch].state = MGMT_STATE_LINKED;
			iter->state = OPENMODE_CAM_STATE_DISCOVERED;
			_abnormal_cnt[i] = 0;

			_openmode_live_mtx_unlock(iter->ch);

			continue;
		}

		if (iter->state == OPENMODE_CAM_STATE_LOGIN_FAIL)
		{
			_login_wait_cnt[iter->ch]++;
			g_message("###yanggungg : %s, %d LOGIN FAIL CH(%d) wait_cnt(%d)\n", __func__, __LINE__, iter->ch, _login_wait_cnt[iter->ch]);

			if (_login_wait_cnt[iter->ch] > 30)
			{
				_openmode_live_mtx_lock(iter->ch);

				if((iter->state != OPENMODE_CAM_STATE_LOGIN_FAIL) || (_login_wait_cnt[iter->ch] <= 30)){
					_openmode_live_mtx_unlock(iter->ch);
					continue;
				}

				runtime[iter->ch].state = MGMT_STATE_LINKED;
				iter->ipaddr = ntohl(_get_ipaddr_from_hostname(_db_host[iter->ch]));
				iter->state = OPENMODE_CAM_STATE_DISCOVERED;
				_login_wait_cnt[iter->ch] = 0;

				_openmode_live_mtx_unlock(iter->ch);
			}
			continue;
		}
		_login_wait_cnt[iter->ch] = 0;

		if (_db_host[i][0] != '\0' && _db_http_port[i] != 0)
		{
			vloss_status = get_vloss_status();
			if (vloss_status & (1<<(iter->ch)))
			{
				if ((runtime[i].state & OPENMODE_STATE_CONFIGURING) == 0)
				{
					_abnormal_cnt[i]++;
					continue;
				}
			}
		}
		_abnormal_cnt[i] = 0;
	}
}

static void _init_net_inf_tbl(void)
{
	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_inf_tbl_mutex);

	memset(&_inf_tbl, 0x00, sizeof(struct NET_INF_INFO)*NET_INF_MAX);

	_net_inf_cnt = 0;

	pthread_mutex_unlock(&_inf_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static void _get_net_inf_parse_dev(char *line, char *dev_buf, size_t buf_size)
{
	icm_str_array array = NULL;
	char *ptr = NULL;

	if(line == NULL || dev_buf == NULL)
	{
		goto ends_label;
	}

	array = icm_str_split((const char *)line, " ", 3);
	if(array == NULL) 
	{
		goto ends_label;
	}

	icm_str_trim(array[1], strlen(array[1]));

	ptr = strstr(array[1], ":");
	if(ptr != NULL)
	{
		*ptr = 0;
		snprintf(dev_buf, buf_size, "%s", array[1]);
	}

ends_label:

	if(array != NULL)
	{
		icm_str_array_free(array, 3);
	}
}

static int _get_net_inf_priority(const char *inf_ip)
{
	int rtn = NET_ADDR_UNKNOWN;
	struct in_addr in_addr;

	if(inf_ip == NULL)
	{
		goto ends_label;
	}

	/*
		1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue
			inet 127.0.0.1/8 scope host lo
			   valid_lft forever preferred_lft forever
		6: br0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue
			inet 172.16.0.3/16 brd 172.16.255.255 scope global br0
			   valid_lft forever preferred_lft forever
			inet 169.254.249.207/16 brd 169.254.255.255 scope global br0:0
			   valid_lft forever preferred_lft forever
	*/

	memset(&in_addr, 0x00, sizeof(in_addr));
	if(inet_aton(inf_ip, &in_addr) == 0)
	{
		IPCAM_DBG(WARN, "inet_aton fail(ip:%s)\n", inf_ip);
		goto ends_label;
	}
	
	//IPCAM_DBG(MINOR, "interface ip=%s, interface ipaddr=%u, host_ipaddr=%u\n", inf_ip, in_addr.s_addr, _host_ip);

	if(in_addr.s_addr == _host_ip)
	{
		rtn = NET_ADDR_MAIN;
	}
	else
	{
		if(strncmp(inf_ip, "169.254", strlen("169.254")) == 0)
		{
			rtn = NET_ADDR_LINK_LO;
		}
		else
		{
			rtn = NET_ADDR_STATIC;
		}
	}

ends_label:

	return rtn;
}

// static char *_ip_to_str(unsigned int ip, char *buf)
// {
// 	static char ret[20];
// 	if(buf == NULL){
// 		buf = ret;
// 	}
// 	snprintf(buf, 16, "%d.%d.%d.%d",
// 			(ip&0xff000000)>>24,
// 			(ip&0xff0000)>>16,
// 			(ip&0xff00)>>8,
// 			(ip&0xff));
// 	return buf;
// }

static char *_mac_to_str(unsigned char *mac, char *buf)
{
	static char ret[50];
	int i;

	if(buf == NULL){
		buf = ret;
	}

	buf[0] = 0;
	for(i = 0; i < 6; i++){
		sprintf(buf + strlen(buf), "%02X:", mac[i]);
	}

	buf[strlen(buf)-1] = 0;

	return buf;
}

static int _get_macaddress_from_arpcmd_line(unsigned char *mac, char *line){
	int i;
	int mac_check = 0;
	sscanf(line, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	for(i = 0; i < 6; i++){
		if(mac[i] == 0x00){
			mac_check++;	
		}
	}
	if(mac_check == 6){
		IPCAM_DBG(MINOR, "mac address line[%s]\n", line);
		return 0;
	}
	return 1;
}

static int _get_macaddress_by_ip(unsigned char *mac, char *ip)
{
	int ret = 0;
	FILE *fp = NULL;
	int fd = -1;
	char line[2048] = {0, };
	char *cmd = "arp";
	char *ptr;

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		goto ends_label;
	}

	while(fgets(line, sizeof(line), fp) != NULL)
	{
		if(strstr(line, ip) != NULL){
			if((ptr = strstr(line, "at ")) == NULL){
				IPCAM_DBG(MINOR, "strstr ret[%p]\n", strstr(line, "at "));
				continue;
			}
			ptr += 3;

			if(_get_macaddress_from_arpcmd_line(mac, ptr)){
				ret++;
			}
		}
	}

ends_label:
	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}
	return ret;
}

static int _is_mac_equal(unsigned char *mac1, unsigned char *mac2)
{
	int i;
	for(i = 0; i < 6; i++){
		if(mac1[i] != mac2[i]){
			return 0;
		}
	}
	return 1;
}
static int _is_mac_null(unsigned char *mac)
{
	int i;
	int count;

	for(i = 0, count = 0; i <6; i++){
		if(mac[i] == 0x00){
			count++;
		}
	}

	if(i == count){
		return 1;
	}else{
		return 0;
	}
}

static int _is_linklocal_address(unsigned int ip)
{
	//169.254.x.x
	//A9 FE
	int ret = 0;

	if(ip >> 16 == 0xA9FE){
		ret = 1;
	}else{
		ret = 0;
	}

	IPCAM_DBG(MINOR, "ip[0x%08x][%s] ret[%d]\n", ip, _ip_to_str(ip, NULL), ret);

	return ret;
}

int _get_macaddress_using_arping_by_ip(unsigned char *mac, char *ip, char *eth)
{
	char cmd[256] = {0};
	char line[1024] = {0};
	FILE *fp = NULL;
	int fd = -1;
	int ret = 0;
	char *ptr;
	
	snprintf(cmd, sizeof(cmd), "arping -c 1 -w 1 -D %s", ip);
	if(eth != NULL){
		snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd) - 1, " -I %s", eth);
	}

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		printf("%s proxy_popen fail(cmd:%s)\n", __func__, cmd);
		goto ends_label;
	}

	while(fgets(line, sizeof(line), fp) != NULL)
	{
		if(strstr(line, ip) != NULL){
			if((ptr = strstr(line, "[")) == NULL){
				IPCAM_DBG(MINOR, "strstr ret[%p] line[%s]\n", strstr(line, "["), line);
				continue;
			}
			ptr += 1;

			if(_get_macaddress_from_arpcmd_line(mac, ptr)){
				ret++;
			}
		}
	}
	
ends_label:

	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}
	return ret;
}

static int _get_net_inf_info(void)
{
	int rtn = 0;
	char cmd[256] = {0};
	char line_1[1024] = {0};
	char line_2[1024] = {0};
	char dev[NET_INF_MAX][32] = {0};
	FILE *fp = NULL;
	int fd = -1;
	struct NET_INF_INFO temp_tbl[NET_INF_MAX] = {0,};
	int cnt = 0, len = 0;
	int ip_alias = 1;
	icm_str_array inet_str = NULL;
	icm_str_array addr_str = NULL;

	IPCAM_DBG(MINOR, "start\n");

	memset(&temp_tbl, 0x00, sizeof(temp_tbl));

	snprintf(cmd, sizeof(cmd), "ip -f inet address");

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		IPCAM_DBG(ERROR, "proxy_popen fail(%s)\n", cmd);
		goto ends_label;
	}

	//eth device parse
	while(fgets(line_1, sizeof(line_1), fp) != NULL)
	{
		//eth device full
		if(cnt >= NET_INF_MAX) { break; }
		// continue loopback
		if(strstr(line_1, "lo:") != NULL) { continue; }

		if(isspace(line_1[0]) != 0)
		{
			if(strstr(line_1,"global") && strstr(line_1,":"))
			{
				if(cnt>0 && cnt<NET_INF_MAX)
				{
					len = strlen(line_1);
					line_1[len-1] = '\0'; //BSN remove
					snprintf(dev[cnt],"%s",strstr(line_1,dev[cnt-ip_alias]));
					ip_alias++;
					cnt++;
				}
			}
			continue;
		}

		_get_net_inf_parse_dev(line_1, dev[cnt], sizeof(dev));
		ip_alias=1;
		cnt++;
	}
	cnt=0;

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		IPCAM_DBG(ERROR, "proxy_popen fail(%s)\n", cmd);
		goto ends_label;
	}

	//ip addr parse
	while(fgets(line_2, sizeof(line_2), fp) != NULL)
	{
		if(strstr(line_2, "127.0.0.1") != NULL) { continue; }
		icm_str_trim(line_2, strlen(line_2));

		if(strncmp(line_2, "inet ", 5) == 0)
		{
			inet_str = icm_str_split((const char *)line_2, " ", 3);
			if(inet_str != NULL)
			{
				addr_str = icm_str_split((const char *)inet_str[1], "/", 2);

				icm_str_array_free(inet_str, 3);

				if(addr_str != NULL)
				{
					snprintf(temp_tbl[cnt].inf_name, sizeof(temp_tbl[cnt].inf_name), "%s", dev[cnt]);
					snprintf(temp_tbl[cnt].inf_addr, sizeof(temp_tbl[cnt].inf_addr), "%s", addr_str[0]);
					snprintf(temp_tbl[cnt].inf_mask, sizeof(temp_tbl[cnt].inf_mask), "%s", addr_str[1]);

					if(strncmp(temp_tbl[cnt].inf_name, HOST_ETH_DEV, 4)==0)
						temp_tbl[cnt].inf_type = NET_INF_WAN;	//wide net eth_dev
					else
						temp_tbl[cnt].inf_type = NET_INF_LAN;	//local net eth_dev

					temp_tbl[cnt].inf_pri = _get_net_inf_priority(addr_str[0]);

					IPCAM_DBG(MAJOR, "interface name=%s\n", temp_tbl[cnt].inf_name);
					IPCAM_DBG(MAJOR, "interface addr=%s\n", temp_tbl[cnt].inf_addr);
					IPCAM_DBG(MAJOR, "interface mask=%s\n", temp_tbl[cnt].inf_mask);
					IPCAM_DBG(MAJOR, "interface type=%d\n", temp_tbl[cnt].inf_type);
					IPCAM_DBG(MAJOR, "interface priority=%d\n", temp_tbl[cnt].inf_pri);

					icm_str_array_free(addr_str, 2);

					cnt++;
					if(cnt >= NET_INF_MAX)
					{
						IPCAM_DBG(WARN, "array size over(count:%d,size:%d)", cnt, NET_INF_MAX);
						goto result_label;
					}
				}
			}
		}
	}

result_label:

	pthread_mutex_lock(&_inf_tbl_mutex);
	if(memcmp(_inf_tbl, temp_tbl, sizeof(struct NET_INF_INFO)*NET_INF_MAX) != 0)
	{
		memcpy(_inf_tbl, temp_tbl, sizeof(struct NET_INF_INFO)*NET_INF_MAX);
		_net_inf_cnt = cnt;
		rtn = cnt;
	}
	pthread_mutex_unlock(&_inf_tbl_mutex);

ends_label:

	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}

	IPCAM_DBG(MINOR, "end\n");

	return rtn;
}

static void _clear_net_inf_tbl(void)
{
	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_inf_tbl_mutex);

	memset(&_inf_tbl, 0x00, sizeof(struct NET_INF_INFO)*NET_INF_MAX);

	_net_inf_cnt = 0;

	pthread_mutex_unlock(&_inf_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static void _init_discovery_tbl(void)
{
	int idx = 0;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_discovery_tbl_mutex);

	memset(&_discovery_tbl, 0x00, sizeof(struct ONVIF_DISCOVERY_INFO)*NET_INF_MAX);

	for(idx = 0; idx < NET_INF_MAX; idx++)
	{
		_discovery_tbl[idx].inf_type = NET_INF_UNKNOWN; 
		_discovery_tbl[idx].inf_pri = NET_ADDR_UNKNOWN;
		_discovery_tbl[idx].multi_sock = -1;
		_discovery_tbl[idx].port = -1;
	}

	_discovery_cnt = 0;

	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static void _clear_discovery_tbl(void)
{
	int idx = 0;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_discovery_tbl_mutex);

	for(idx = 0; idx < _discovery_cnt; idx++)
	{
		if(_discovery_tbl[idx].multi_sock > 0)
		{
			IPCAM_DBG(MAJOR, "close socket(%d)\n", _discovery_tbl[idx].multi_sock);
			close(_discovery_tbl[idx].multi_sock);
		}

		_discovery_tbl[idx].inf_name[0] = 0;
		_discovery_tbl[idx].inf_addr = 0;
		_discovery_tbl[idx].inf_mask = 0;
		_discovery_tbl[idx].inf_type = NET_INF_UNKNOWN;
		_discovery_tbl[idx].inf_pri = NET_ADDR_UNKNOWN;
		_discovery_tbl[idx].multi_sock = -1;
		_discovery_tbl[idx].port = -1;
	}

	_discovery_cnt = 0;

	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static void _make_discovery_tbl(void)
{
	int idx = 0;
	int mask_cnt = 0;
	struct sockaddr_in sin_m;
	struct in_addr in_addr;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_inf_tbl_mutex);
	pthread_mutex_lock(&_discovery_tbl_mutex);

	for(idx = 0; idx < _net_inf_cnt; idx++)
	{
		// interface dev name
		snprintf(_discovery_tbl[idx].inf_name, sizeof(_discovery_tbl[idx].inf_name), "%s", _inf_tbl[idx].inf_name);

		// interface ip
		memset(&in_addr, 0x00, sizeof(in_addr));
		if(inet_aton(_inf_tbl[idx].inf_addr, &in_addr) == 0)
		{
			IPCAM_DBG(ERROR, "multicast rcv socket inet_aton fail\n");
			perror("inet_aton");
			continue;
		}
		_discovery_tbl[idx].inf_addr = ntohl(in_addr.s_addr);
		
		// interface subnet
		mask_cnt = atoi(_inf_tbl[idx].inf_mask);
		_discovery_tbl[idx].inf_mask = 0;
		/*
		while(mask_cnt > 0)
		{
			_discovery_tbl[idx].inf_mask |= (1<<(mask_cnt-1));
			mask_cnt--;
		}
		*/
		_discovery_tbl[idx].inf_mask = subnet_table_big_endian[mask_cnt-1];
		_discovery_tbl[idx].inf_mask = ntohl(_discovery_tbl[idx].inf_mask);

		// interface type
		_discovery_tbl[idx].inf_type = _inf_tbl[idx].inf_type;

		// interface priority
		_discovery_tbl[idx].inf_pri = _inf_tbl[idx].inf_pri;

		// multicast socket
		_discovery_tbl[idx].multi_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (_discovery_tbl[idx].multi_sock < 0)
		{
			IPCAM_DBG(ERROR, "multicast rcv socket init failed\n");
			perror("socket");
			continue;
		}
		// port
		_discovery_tbl[idx].port = _WS_DISC_CLI_PORT+idx;

		// bind
		memset(&sin_m, 0x00, sizeof(sin_m));
		sin_m.sin_family = AF_INET;
		sin_m.sin_addr.s_addr = htonl(_discovery_tbl[idx].inf_addr);
		sin_m.sin_port = htons(_discovery_tbl[idx].port);

		if (bind(_discovery_tbl[idx].multi_sock, (struct sockaddr*) &sin_m, sizeof(sin_m)) < 0)
		{
			IPCAM_DBG(ERROR, "multicast rcv socket bind failed\n");
			perror("bind");
			close(_discovery_tbl[idx].multi_sock);
			_discovery_tbl[idx].multi_sock = (-1);
			continue;
		}
	
		IPCAM_DBG(MAJOR, "dev=%s, IP=%u, mask=%u, type=%d, priority=%d, port=%d, socket=%d\n", 
						_discovery_tbl[idx].inf_name,
						_discovery_tbl[idx].inf_addr,
						_discovery_tbl[idx].inf_mask,
						_discovery_tbl[idx].inf_type,
						_discovery_tbl[idx].inf_pri,
						_discovery_tbl[idx].port, 
						_discovery_tbl[idx].multi_sock);
	}

	_discovery_cnt = _net_inf_cnt;

	pthread_mutex_unlock(&_inf_tbl_mutex);
	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static int _get_discovery_tbl_count(void)
{
	int count = 0;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_discovery_tbl_mutex);

	count = _discovery_cnt;

	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");

	return count;
}

static int _get_discovery_multi_sock(int idx)
{
	int multi_sock = -1;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_discovery_tbl_mutex);

	if(idx >= _discovery_cnt)
	{
		multi_sock = -1;
	}
	else
	{
		multi_sock = _discovery_tbl[idx].multi_sock;
	}

	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");

	return multi_sock;
}

static int _get_inf_priority_to_addr(uint32_t ipaddr)
{
	int idx = 0;
	int pri = NET_ADDR_UNKNOWN;

	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_discovery_tbl_mutex);

	for(idx = 0; idx < _discovery_cnt; idx++)
	{
		if((ipaddr & _discovery_tbl[idx].inf_mask) == (_discovery_tbl[idx].inf_addr & _discovery_tbl[idx].inf_mask))
		{
			IPCAM_DBG(MINOR, "dev=%s, cam_ip=%u, inf_ip=%u, mask=%u, priority=%d\n", 
							_discovery_tbl[idx].inf_name, ipaddr, _discovery_tbl[idx].inf_addr, 
							_discovery_tbl[idx].inf_mask, _discovery_tbl[idx].inf_pri);

			pri = _discovery_tbl[idx].inf_pri;
			break;
		}
	}

	pthread_mutex_unlock(&_discovery_tbl_mutex);

	IPCAM_DBG(MINOR, "end\n");

	return pri;
}

static size_t _make_hash_code(char *key, size_t hash_size)
{
	char *ptr = NULL;
	unsigned long val = 0;
	size_t rtn = 0;

	IPCAM_DBG(MINOR, "start(key:%s,size:%u)\n", key, hash_size);

	if(key == NULL || hash_size == 0)
	{
		return 0;
	}

	// prime number hash
	for(ptr = key; *ptr != 0; ptr++)
	{
		val = (unsigned long)*ptr + (unsigned long)(val*31);
	}

	rtn = (size_t)(val % hash_size);

	IPCAM_DBG(MINOR, "end(rtn:%u)\n", rtn);

	return rtn;
}

static void _init_cam_info_hash(void)
{
	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_hash_mutex);

	memset(&_cam_info_hash, 0x00, sizeof(_cam_info_hash));

	pthread_mutex_unlock(&_hash_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static void _free_cam_info_hash(HASH_LIST hash_array[], size_t max_size)
{
	int idx = 0;
	HASH_INFO *iter = NULL;
	HASH_INFO *temp = NULL;

	if(hash_array == NULL)
	{
		return;
	}

	for(idx = 0; idx < max_size; idx++)
	{
		for(iter = hash_array[idx].head; iter != NULL; iter = temp)
		{
			temp = iter->next;

			free(iter);
		}

		hash_array[idx].head = NULL;
	}
}

static void _empty_cam_info_hash(void)
{
	IPCAM_DBG(MINOR, "start\n");

	pthread_mutex_lock(&_hash_mutex);

	_free_cam_info_hash(_cam_info_hash.uuid_hash, UUDI_HASH_MAX);

	_free_cam_info_hash(_cam_info_hash.addr_hash, ADDR_HASH_MAX);

	pthread_mutex_unlock(&_hash_mutex);

	IPCAM_DBG(MINOR, "end\n");
}

static HASH_INFO* _get_cam_info_hash(char *key, int type)
{
	size_t hash_cd = 0;
	HASH_INFO *iter = NULL;
	HASH_INFO *head = NULL;

	IPCAM_DBG(MINOR, "start(key=%s,type=%d)\n", key, type);

	if(key == NULL)
	{
		return NULL;
	}

	pthread_mutex_lock(&_hash_mutex);

	switch(type)
	{
		case HASH_TYPE_UUID:
		{
			hash_cd = _make_hash_code(key, UUDI_HASH_MAX);
			head = _cam_info_hash.uuid_hash[hash_cd].head;
		}
		break;
		case HASH_TYPE_ADDR:
		{
			hash_cd = _make_hash_code(key, ADDR_HASH_MAX);
			head = _cam_info_hash.addr_hash[hash_cd].head;
		}
		break;
		default:
		{
			head = NULL;
		}
	}

	IPCAM_DBG(MINOR, "head=%p\n", head);

	for(iter = head; iter != NULL; iter = iter->next)
	{
		if(strcmp(iter->key, key) == 0)
		{
			break;
		}
	}

	pthread_mutex_unlock(&_hash_mutex);

	IPCAM_DBG(MINOR, "end(iter=%p)\n", iter);

	return iter;
}

static OPENMODE_RTN_ENUM _add_cam_info_hash(char *key, int key_type, void *ptr)
{
	int rtn = OPENMODE_RTN_FAIL;
	size_t hash_cd = 0;
	HASH_INFO *iter = NULL;
	HASH_INFO *temp = NULL;
	HASH_INFO **head = NULL;

	IPCAM_DBG(MINOR, "start(key=%s,type=%d)\n", key, key_type);

	if(key == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}

	iter = _get_cam_info_hash(key, key_type);
	if(iter != NULL)
	{
		IPCAM_DBG(WARN, "cam information hash table key duplicate(key=%s)\n", key);
		goto ends_label;
	}

	pthread_mutex_lock(&_hash_mutex);

	switch(key_type)
	{
		case HASH_TYPE_UUID:
		{
			hash_cd = _make_hash_code(key, UUDI_HASH_MAX);
			head = &(_cam_info_hash.uuid_hash[hash_cd].head);
		}
		break;
		case HASH_TYPE_ADDR:
		{
			hash_cd = _make_hash_code(key, ADDR_HASH_MAX);
			head = &(_cam_info_hash.addr_hash[hash_cd].head);
		}
		break;
		default:
		{
			pthread_mutex_unlock(&_hash_mutex);
			goto ends_label;
		}
	}

	IPCAM_DBG(MINOR, "head=%p\n", head);

	temp = (HASH_INFO *)calloc(1, sizeof(HASH_INFO));
	if(temp == NULL)
	{
		pthread_mutex_unlock(&_hash_mutex);
		goto ends_label;
	}
	else
	{
		snprintf(temp->key, HASH_KEY_LEN, key);
		temp->cam_entry = (NFOpenmodeCamInfo *)ptr;
		temp->prev = NULL;
		temp->next = NULL;
	}

	if(*head == NULL)
	{
		*head = temp;
	}
	else
	{
		temp->next = *head;
		temp->next->prev = temp;
		*head = temp;
	}

	pthread_mutex_unlock(&_hash_mutex);

	rtn = OPENMODE_RTN_OK; 

ends_label:

	IPCAM_DBG(MINOR, "end\n");

	return rtn;
}

static OPENMODE_RTN_ENUM _del_cam_info_hash(char *key, int key_type)
{
	int rtn = OPENMODE_RTN_FAIL;
	HASH_INFO *iter = NULL;
	HASH_INFO *temp = NULL;

	IPCAM_DBG(MAJOR, "start\n");

	if(key == NULL)
	{
		goto ends_label;
	}

	iter = _get_cam_info_hash(key, key_type);
	if(iter == NULL)
	{
		IPCAM_DBG(WARN, "Not found cam information hash(key:%s)\n", key);
		goto ends_label;
	}

	pthread_mutex_lock(&_hash_mutex);

	if(iter != NULL)
	{
		temp = iter;

		if(iter->next != NULL)
		{
			iter->next->prev = temp->prev;
		}

		if(iter->prev != NULL)
		{
			iter->prev->next = temp->next;
		}

		free(iter);
	}

	pthread_mutex_unlock(&_hash_mutex);

	rtn = OPENMODE_RTN_OK;

ends_label:

	IPCAM_DBG(MAJOR, "end\n");

	return rtn;
}

static int _init_multicast_sock(void)
{
	int res = 0;

	IPCAM_DBG(MAJOR, "start\n");

	res = _get_net_inf_info();

	if(res == 0)
	{
		IPCAM_DBG(WARN, "multicast binding skipped\n");
		goto _label_end;
	}
	else
	{
		_clear_discovery_tbl();

		_make_discovery_tbl();
	}

_label_end:

	IPCAM_DBG(MAJOR, "end\n");

	return 0;
}

static void _init_admin_socks(void)
{
	int i = 0;
	int on = 1;
	struct sockaddr_in sin;

	IPCAM_DBG(MAJOR, "start\n");

	if (_rcv_sock > 0)
	{
		IPCAM_DBG(MINOR, "ITX admin protocol rcv port binding skipped\n");
		goto _label_send;
	}

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(_ADMIN_SVR_PORT);

	_rcv_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(_rcv_sock < 0)
	{
		IPCAM_DBG(ERROR, "rcv socket init failed\n");
		perror("socket");
		return ;
	}
	if (bind(_rcv_sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "rcv socket bind failed\n");
		perror("bind");
		close(_rcv_sock);
		_rcv_sock = (-1);
		return ;
	}

	IPCAM_DBG(MINOR, "ITX admin protocol rcv port bind done\n");

_label_send:
	if (_send_sock > 0)
	{
		IPCAM_DBG(MINOR, "ITX admin protocol snd sock init skipped\n");
		goto _label_end;
	}
	_send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (_send_sock < 0)
	{
		IPCAM_DBG(ERROR, "send socket init failed\n");
		perror("socket");
		close(_rcv_sock);
		_rcv_sock = (-1);
		return ;
	}
	setsockopt(_send_sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

	IPCAM_DBG(MINOR, "ITX admin protocol snd setopt done\n");

_label_end:
	IPCAM_DBG(MAJOR, "end\n");
}

static void _openmode_load_sysdb(void)
{
	gint i=0;
	gchar key[128];
	gchar *ptr;

	guint16 eth=0;

	IPCAM_DBG(MAJOR, "start\n");

	for (i=0; i<_max_ch; i++)
	{
		snprintf(key, 128, "cam.logininfo.L%d.hostname", i);
		strncpy(_db_host[i], nf_sysdb_get_str_nocopy(key), 256);

		snprintf(key, 128, "cam.logininfo.L%d.http_port", i);
		_db_http_port[i] = nf_sysdb_get_uint(key);

		snprintf(key, 128, "cam.logininfo.L%d.id", i);
		ptr = nf_sysdb_get_str_nocopy(key);
		strncpy(_db_u[i], ptr, 64);

		snprintf(key, 128, "cam.logininfo.L%d.pwd", i);
		ptr = nf_sysdb_get_str_nocopy(key);
		strncpy(_db_p[i], ptr, 64);

		snprintf(key, 128, "cam.logininfo.L%d.rtsp_port", i);
		_db_rtsp_port[i] = nf_sysdb_get_uint(key);

		snprintf(key, 128, "cam.logininfo.L%d.rtsp_addr_main", i);
		ptr = nf_sysdb_get_str_nocopy(key);
		strncpy(_db_rtsp_addr_main[i], ptr, 256);

		snprintf(key, 128, "cam.logininfo.L%d.rtsp_addr_second", i);
		ptr = nf_sysdb_get_str_nocopy(key);
		strncpy(_db_rtsp_addr_second[i], ptr, 256);

		snprintf(key, 128, "cam.logininfo.L%d.vcam", i);
		_db_vcam[i] = nf_sysdb_get_uint(key);

		snprintf(key, 128, "cam.logininfo.L%d.vcam_cnt", i);
		_db_vcam_cnt[i] = nf_sysdb_get_uint(key);

		snprintf(key, 128, "cam.C%d.model_nm", i);
		ptr = nf_sysdb_get_str_nocopy(key);
		strncpy(_db_model[i], ptr, 64);

		snprintf(key, 128, "cam.logininfo.L%d.ethernet", i);
		eth = nf_sysdb_get_uint(key);
		if(eth==0) // eth==0 -> LAN 1 (eth1)
		{
			strncpy(_db_ethernet[i], "eth0", 4);
		}
		else // eth==1 ->  LAN 2 (eth0)
		{
			strncpy(_db_ethernet[i], "eth1", 4);
		}
		
	}

	IPCAM_DBG(MAJOR, "end\n");
}

/* 인덱스를 0부터 순차적으로 재정렬하는 함수 */
static void _openmode_list_index_refresh(NFOpenmodeDeviceList *list)
{
    int idx = 0;
    NFOpenmodeCamInfo *iter = NULL;

    // 리스트 조작 시 동기화를 위해 Lock을 겁니다.
    _openmode_list_mtx_lock();

    iter = list->head;
    while (iter != NULL)
    {
        iter->index = idx;
        idx++;
        iter = iter->next;
    }
    
    // (선택사항) 만약 entry_cnt가 꼬일 가능성이 있다면 여기서 보정할 수도 있습니다.
    // list->entry_cnt = idx;

    _openmode_list_mtx_unlock();
    
    // IPCAM_DBG(MINOR, "List index refreshed. Total count: %d\n", idx);
}

static OPENMODE_RTN_ENUM _discovery_admin_handler(netconf_msg* received, struct sockaddr_in* cin)
{
    gint rtn;
    gint http;
    gint rtsp;
    guint xid;
    guint ip;
    guint linklocal_ip;
    guint cin_ip;
    guint gw;
    guint mask;
    guint dns1;
    guint dns2;
    guint is_dhcp;
    guchar mac[6];
    OPENMODE_DEV_CATEGORY dev_cate = 0;

    guint host_ip;
    guint host_subnet;

    NFOpenmodeCamInfo *entry = NULL;
    NFOpenmodeCamInfo *entry_ip = NULL;
    NFOpenmodeCamInfo *rec_entry = NULL; // 녹화기 리스트 검색용

    // IPCAM_DBG(MAJOR, "start\n");

    http = ntohs(received->http_port);
    rtsp = ntohs(received->rtsp_port);
    ip = received->ciaddr;
    linklocal_ip = *((unsigned int *)(received->vend + 33));
    gw = ntohl(received->giaddr);
    mask = ntohl(received->miaddr);
    dns1 = ntohl(received->d1iaddr);
    dns2 = ntohl(received->d2iaddr);
    xid = ntohl(received->xid);
    is_dhcp = (received->version >> 4);
    memcpy(mac, received->chaddr, 6);
    if(cin != NULL){
        cin_ip = cin->sin_addr.s_addr;
    }

    host_ip = _host_ip;
    host_subnet = _host_subnet;
    if (host_ip == 0 || host_subnet == 0)
    {
        return OPENMODE_RTN_FAIL;
    }

    // 1. 녹화기(NVR) 판별 로직 (ITX 프로토콜 벤더 코드 확인)
    if (received->vend[0] == 0x17 && received->vend[1] == 0x18 && received->vend[2] == 0x1)
    {
        dev_cate = OPENMODE_DEV_NVR;
    }

    if (ip == 0 || ip == 0xffffffff)
    {
        //linklocal field check
        if(linklocal_ip != 0){
            ip = linklocal_ip;
        }else{
            return OPENMODE_RTN_FAIL;
        }
    }

#if (!defined(ENABLE_PROJECT_KMW))
    if (!_is_host_subnet(ntohl(ip)))
    {
        // IPCAM_DBG(WARN, "Wrong ip subnet ip[%s]\n", _ip_to_str(ntohl(ip), NULL));
        return OPENMODE_RTN_FAIL;
    }
#endif

    // 2. 리스트 검색 및 중복/이동 처리
    
    // (A) 녹화기 리스트(recorder_list)에 이미 있는지 먼저 확인
    //     (이전에 ITX 프로토콜이 먼저 처리되었거나, 이미 이동된 경우)
    rec_entry = _list_find_entry_by_mac(&_openmode_recorder_list, mac);
    if(rec_entry == NULL) {
        rec_entry = _list_find_entry_by_ip(&_openmode_recorder_list, ntohl(ip));
    }

    if (rec_entry != NULL) 
    {
        // 이미 녹화기 리스트에 존재함 -> 해당 entry 사용 (정보 갱신만 수행)
        entry = rec_entry;
    }
    else 
    {
        // (B) 일반 카메라 리스트(detection_list) 검색
        //     (ONVIF가 먼저 발견했거나, 기존 카메라인 경우)
        entry = _list_find_entry_by_mac(&_openmode_detection_list, mac);
        entry_ip = _list_find_entry_by_ip(&_openmode_detection_list, ntohl(ip));
        
        if (entry == NULL && entry_ip == NULL)
        {
            /* [신규 장비 발견] - 두 리스트 모두 없음 */
            
            /* list cnt check */
            if (_openmode_detection_list.entry_cnt >= MAX_SEARCHABLE_CAM_COUNT)
            {
                IPCAM_DBG(WARN, "Max list entry count exceeded(256)\n");
                return OPENMODE_RTN_FAIL;
            }

            entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
            _alloc_cnt++;
            if (entry == NULL)
            {
                _alloc_cnt--;
                IPCAM_DBG(ERROR, "Memory allocation for the list entry failed\n");
                return OPENMODE_RTN_FAIL;
            }
            memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));
            entry->index = _openmode_detection_list.entry_cnt;
            entry->ch = _get_available_ch();

            if(_aibox_scan_mode){
                _alloc_cnt--;
                free(entry);
                return OPENMODE_RTN_FAIL;
            }
            else {
                if(!(strcmp(entry->eth_dev,HUB_ETH_DEV)==0 || strcmp(entry->eth_dev,HOST_ETH_DEV)==0))
                {
                    strncpy(entry->eth_dev, get_net_inf_from_cam(ip), 4);
                }

                // ★ 핵심 로직: NVR 여부에 따라 다른 리스트에 추가
                if (dev_cate == OPENMODE_DEV_NVR)
                {
                    printf("[%s] NVR Found (New)! Add to Recorder List. IP: %s\n", __func__, _ip_to_str(ntohl(ip), NULL));
                    _openmode_list_add(&_openmode_recorder_list, entry);
                }
                else
                {
                    _openmode_list_add(&_openmode_detection_list, entry);
                }
				_openmode_list_index_refresh(&_openmode_detection_list);
				_openmode_list_index_refresh(&_openmode_recorder_list);
            }
        }
        else 
        {
            /* [기존 장비 발견] - detection_list에 존재함 (주로 ONVIF가 먼저 등록한 경우) */
            if (entry == NULL) entry = entry_ip; // IP로 찾은 경우 매핑

            // ★ 핵심 로직: NVR로 판명되면 리스트 이동 (Detection -> Recorder)
            if (dev_cate == OPENMODE_DEV_NVR)
            {
                printf("[%s] NVR Identified (Move)! Moving from Detection to Recorder List. IP: %s\n", __func__, _ip_to_str(ntohl(ip), NULL));
                
                // 기존 리스트에서 제거하고 녹화기 리스트로 추가
                _openmode_list_remove(&_openmode_detection_list, entry);
                _openmode_list_add(&_openmode_recorder_list, entry);
				_openmode_list_index_refresh(&_openmode_detection_list);
				_openmode_list_index_refresh(&_openmode_recorder_list);
            }
        }
    }

    // 3. 공통 정보 갱신 (entry가 NULL이 아님이 보장됨)
    //    여기서부터는 entry가 recorder_list에 있든 detection_list에 있든 동일하게 정보 업데이트

    if (http != 0) { entry->http_port = http; }
    if (rtsp != 0) { entry->rtsp_port = rtsp; }
    if (ip != 0) {
        if(entry->ipaddr != 0 && _is_linklocal_address(ntohl(ip)) && !_is_linklocal_address(entry->ipaddr)){
             // 기존 IP가 정상이고 새 IP가 LinkLocal이면 변경하지 않음
             // IPCAM_DBG(MINOR, "discovery ip not change list_ip[0x%08x] curr_ip[0x%08x]\n", ntohl(ip), entry->ipaddr);
        } else {
            entry->ipaddr = ntohl(ip);
            snprintf(entry->hostname, 16, "%d.%d.%d.%d",
                    (entry->ipaddr&0xff000000)>>24,
                    (entry->ipaddr&0xff0000)>>16,
                    (entry->ipaddr&0xff00)>>8,
                    (entry->ipaddr&0xff)
                    );
        }
    }
    
    entry->is_dhcp = is_dhcp;
    entry->xid = xid;
    entry->gw = gw;
    snprintf(entry->gwstr, 16, "%d.%d.%d.%d",
            (entry->gw&0xff000000)>>24, (entry->gw&0xff0000)>>16, (entry->gw&0xff00)>>8, (entry->gw&0xff));
    entry->mask = mask;
    snprintf(entry->maskstr, 16, "%d.%d.%d.%d",
            (entry->mask&0xff000000)>>24, (entry->mask&0xff0000)>>16, (entry->mask&0xff00)>>8, (entry->mask&0xff));
    entry->dns1 = dns1;
    snprintf(entry->dns1str, 16, "%d.%d.%d.%d",
            (entry->dns1&0xff000000)>>24, (entry->dns1&0xff0000)>>16, (entry->dns1&0xff00)>>8, (entry->dns1&0xff));
    entry->dns2 = dns2;
    snprintf(entry->dns2str, 16, "%d.%d.%d.%d",
            (entry->dns2&0xff000000)>>24, (entry->dns2&0xff0000)>>16, (entry->dns2&0xff00)>>8, (entry->dns2&0xff));
    
    memcpy(entry->macaddr, mac, 6);
    entry->dev_cate = dev_cate;

    // printf("[%s:%d] Update IP[%s] Type[%d]\n", __func__, __LINE__, _ip_to_str(entry->ipaddr, NULL), dev_cate);

    if (!_is_host_subnet(ntohl(ip)))
    {
        entry->state = OPENMODE_CAM_STATE_INVALID_IP;
    }
    else if (entry->state == OPENMODE_CAM_STATE_INVALID_IP)
    {
        entry->state = OPENMODE_CAM_STATE_DISCOVERED;
    }

    if (entry->state >= OPENMODE_CAM_STATE_CONN_FAIL || entry->state == OPENMODE_CAM_STATE_INIT)
    {
        entry->state = OPENMODE_CAM_STATE_DISCOVERED;
    }
    
    if(!_aibox_scan_mode){
#if BLOCKING_SCAN
    nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
    nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
    }

    // IPCAM_DBG(MAJOR, "end\n");
    return OPENMODE_RTN_OK;
}

static int _is_onvif_recv_aibox(char *msg)
{
	int ret = 0;	
	char *start;
	char *end;
	int len = 0;
	char *str = NULL;

	if(msg == NULL) return 0;
	

	start = strstr(msg, "<d:Scopes>");
	if(start == NULL) return 0;
	start += 10;
	end = strstr(msg, "</d:Scopes>");
	if(end == NULL) return 0;

	len = end - start;
	if(len <= 0) return 0;
	str = (char *)malloc(len + 1);
	if(str == NULL) return 0;
	
	snprintf(str, len+1, "%s", start);

    if(strstr(str, "onvif://www.onvif.org/custom/type/AIBOX")){
        ret = 1;
    }else
	if(strstr(str, "AI-BOX")){
		ret = 1;
	}else
	if(strstr(str, "AIBOX")){
		ret = 1;
	}else
	if(strstr(str, "DLVAPLUS")){
		ret = 1;
	}

endl:
	if(str) free(str);
	return ret;
}

static OPENMODE_RTN_ENUM  _discovery_onvif_handler(gchar* msg, struct sockaddr_in* cin, gchar *inf_name)
{
	gint rtn = OPENMODE_RTN_FAIL;
	NFOpenmodeCamInfo *entry = NULL;
	char uuid[URN_UUID_LEN] = {0,};
	HASH_INFO *hash_entry = NULL;

	char _ip[20];
	int i;

	if(_is_onvif_recv_aibox(msg)){
		printf("[%s:%d] aibox ip[%s]\n", __func__, __LINE__, _ip_to_str(ntohl(cin->sin_addr.s_addr), NULL));
		IPCAM_DBG(WARN, "is AIBOX\n");
		return rtn;
	}

	/* list cnt check */
	if (_openmode_detection_list.entry_cnt >= MAX_SEARCHABLE_CAM_COUNT)
	{
		IPCAM_DBG(WARN, "Max list entry count exceeded(256)\n");
		return rtn;
	}

	/* find NetworkVideoTransmitter */
	rtn = _discovery_buf_find_nvt(msg);
	if (rtn != OPENMODE_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Non-NVT\n");
		return rtn;
	}

	/* find & check uuid  */
	rtn = _discovery_buf_find_uuid(msg, uuid, sizeof(uuid));
	if(rtn == OPENMODE_RTN_OK)
	{
		hash_entry = _get_cam_info_hash(uuid, HASH_TYPE_UUID);
		if(hash_entry != NULL)
		{
			//IP Priority 설정
			char _ip1[20], _ip2[20];
			NFOpenmodeCamInfo new_entry;
			IPCAM_DBG(MINOR, "IP Priority current[%s] list[%s]\n", 	_ip_to_str(ntohl(cin->sin_addr.s_addr), _ip1), _ip_to_str(hash_entry->cam_entry->ipaddr, _ip2));
			IPCAM_DBG(WARN, "Duplicate UUID Msg(%s)\n", uuid);
			printf("[%s:%d] IP Priority current[%s] list[%s] uuid[%s] \n", __func__, __LINE__, _ip_to_str(ntohl(cin->sin_addr.s_addr), _ip1), _ip_to_str(hash_entry->cam_entry->ipaddr, _ip2), uuid);

			entry = hash_entry->cam_entry;
            _discovery_buf_find_addr(msg, &new_entry);

            if(_is_linklocal_address(new_entry.ipaddr) && !_is_linklocal_address(entry->ipaddr)){
                //not change 
                printf("[%s:%d] not changed curr_ip[%s] new_ip[%s]\n", __func__, __LINE__, _ip_to_str(entry->ipaddr, _ip1), _ip_to_str(new_entry.ipaddr, _ip2));
            }else{
                //change
                if(new_entry.ipaddr != entry->ipaddr){
                    printf("[%s:%d] changed curr_ip[%s] -> new_ip[%s]\n", __func__, __LINE__, _ip_to_str(entry->ipaddr, _ip1), _ip_to_str(new_entry.ipaddr, _ip2));
                    entry->ipaddr = new_entry.ipaddr;
                    if(!_aibox_scan_mode){
                        nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
                    }
                }
            }
			return OPENMODE_RTN_FAIL;
		}
		else
		{
			printf("[%s:%d] Find new device[%s] uuid[%s] \n", __func__, __LINE__, _ip_to_str(ntohl(cin->sin_addr.s_addr), NULL), uuid);
		}
	}

	entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
	_alloc_cnt++;
	if (entry == NULL)
	{
		IPCAM_DBG(ERROR, "Memory allocation for the list entry failed\n");
		return rtn;
	}
	memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));
	entry->ch = _get_available_ch();

#if 1
	/* find model name */
	rtn = _discovery_buf_find_hw(msg, entry);
	if (rtn != OPENMODE_RTN_OK)
	{
		IPCAM_DBG(ERROR, "hardware parsing error\n");
		free(entry);
		_alloc_cnt--;
		entry = NULL;
		return rtn;
	}
	IPCAM_DBG(MINOR, "HW NAME : %s\n", entry->model);
#endif

	/* find service tail */
	//printf("★★★ Test 1 : [%s] \n", entry->model);
	rtn = _discovery_buf_find_addr(msg, entry);

	//printf("★★★ Test 2 : [%s] \n", entry->model);
	if (rtn != OPENMODE_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Service tail parsing error(%s)\n", entry->model);
		free(entry);
		_alloc_cnt--;
		entry = NULL;
		return rtn;
	}

	//printf("★★★ Test 3 : [%s] \n", entry->model);
	IPCAM_DBG(MINOR, "%s - Service address : http://%d.%d.%d.%d:%d/%s\n",
			entry->model,
			(entry->ipaddr&0xff000000)>>24,
			(entry->ipaddr&0xff0000)>>16,
			(entry->ipaddr&0xff00)>>8,
			entry->ipaddr&0xff,
			entry->http_port, entry->tail);

	rtn = _discovery_buf_find_name_and_auth(msg, entry);

	_get_macaddress_using_arping_by_ip(entry->macaddr, _ip_to_str(entry->ipaddr, _ip), inf_name);
	strncpy(entry->eth_dev, inf_name, sizeof(inf_name));
	/*
	if(strncmp(inf_name, "eth0", 4)==0)
		strncpy(entry->eth_dev, "LAN0", 4);
	else
		strncpy(entry->eth_dev, "LAN1", 4);
	*/
	//printf("★★★ [%s]-[%s]\n", entry->hostname, entry->eth_dev);

#if 0
	{
		unsigned char *mac = entry->macaddr;
		IPCAM_DBG(MINOR, "ip[%s] mac(%02x-%02x-%02x-%02x-%02x-%02x)\n", 
				_ip_to_str(entry->ipaddr, NULL),
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
				);

	}
#endif

	/* add to device list */
	if(_aibox_scan_mode){
        rtn = OPENMODE_RTN_FAIL;
    }else{
		_openmode_list_mtx_lock();
		// compare recorder list first
		if(_list_compare_recorder_list(entry) != OPENMODE_RTN_OK)
		{
			// 이미 recorder_list에 존재함 -> 추가하지 않고 종료
			IPCAM_DBG(MINOR, "Device already exists in recorder list. Ignore ONVIF response.\n");
			free(entry);
			_alloc_cnt--;
			entry = NULL;
			rtn = OPENMODE_RTN_FAIL; // 혹은 OK로 처리하여 로그 방지
		}
		else
		{
			// Recorder 리스트에 없으므로 일단 detection_list(일반 카메라)에 추가
            // (나중에 ITX 프로토콜 핸들러에서 NVR로 판명되면 이동될 것임)
			rtn = _discovery_detection_list_update(entry);
		}
		_openmode_list_mtx_unlock();
	}
	
	if (rtn != OPENMODE_RTN_OK)
	{
		IPCAM_DBG(WARN, "Detection list doesn't be updated\n");
		free(entry);
		_alloc_cnt--;
		entry = NULL;
		return rtn;
	}

	/* add uuid hash table */
	if(uuid[0] != '\0')
	{
		_add_cam_info_hash(uuid, HASH_TYPE_UUID, (void *)entry);
	}
	if(!_aibox_scan_mode){
#if BLOCKING_SCAN
	nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
	nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
	}

	return rtn;
}

static OPENMODE_RTN_ENUM _discovery_buf_find_nvt(gchar* msg)
{
	const gchar find_type[] = "NetworkVideoTransmitter";

	if (strstr(msg, find_type) == NULL)
	{
		IPCAM_DBG(WARN, "response from non-NVT\n");
		return OPENMODE_RTN_FAIL;
	}

	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _discovery_buf_find_hw(gchar* msg, NFOpenmodeCamInfo* entry)
{
	gchar *s, *p1, *p2, *e;
	const gchar find_model[] = "onvif://www.onvif.org/hardware/";

	s = strstr(msg, find_model);
	if (s == NULL)
	{
		IPCAM_DBG(WARN, "model not found\n");
		return OPENMODE_RTN_FAIL;
	}
	s += strlen(find_model);
	p1 = strstr(s, " ");
	p2 = strstr(s, "</d:Scopes>");
	if (p1 == NULL)
	{
		if (p2 == NULL)
		{
			IPCAM_DBG(WARN, "model end point\n");
			return OPENMODE_RTN_FAIL;
		}

		e = p2;
	}
	else
	{
		if (p2 == NULL)
		{
			e = p1;
		}
		else
		{
			e = (p1 < p2) ? p1 : p2;
		}
	}
	memset(entry->model, 0x00, 64);
	memcpy(entry->model, s, (size_t)(e-s));

	return OPENMODE_RTN_OK;
}

static int _get_wsd_ns(const char* msg, const char *find_ns, char* buf)
{
	const char xmlns[] = "xmlns:";
	char xmlns_token[256];
	char *p, *s, *e, *env_end;
	int rtn = 0;


	if (msg == NULL)
	{
		IPCAM_DBG(WARN, "msg is null\n");
		return;
	}

	s = msg;

	env_end = strstr(s, ":Header");
	if (env_end == NULL)
	{
		IPCAM_DBG(WARN, ":Header not found\n");
		return;
	}
	env_end--;

	while (s != NULL)
	{
		s = strstr(s, xmlns);
		if (s == NULL)
		{
			IPCAM_DBG(MINOR, "no more xmlns\n");
			break;
		}
		e = strstr(s, " ");
		if (e == NULL)
		{
			IPCAM_DBG(MINOR, "end blank not found. use 'env_end' instead of 'e'\n");
			e = env_end;
		}

		memset(xmlns_token, 0x00, 256);
		if (e-s > 255)
		{
			e = s+255;
		}
		memcpy(xmlns_token, s, (e-s));

		p = strstr(xmlns_token, find_ns);
		if (p == NULL)
		{
			s = e+1;
			continue;
		}

		s = xmlns_token;
		s += strlen(xmlns);
		e = strstr(s, "=\"");
		memcpy(buf, s, e-s);
		rtn = 1;
		break;
	}

	IPCAM_DBG(MAJOR, "end(%s)\n", buf);
	return rtn;
}

static OPENMODE_RTN_ENUM _discovery_buf_find_uuid(gchar* msg, char *uuid_buf, size_t buf_size)
{
	const char find_namespace[] = "\"http://schemas.xmlsoap.org/ws/2004/08/addressing\"";
	char *uuid_st_fmt = "<%s:Address>urn:uuid:";
	char *uuid_ed_fmt = "</%s:Address>";
	char wsd_ns[32] = {0};
	char uuid_st_str[64] = {0};
	char uuid_ed_str[64] = {0};
	char *uuid_st_ptr = NULL;
	char *uuid_ed_ptr = NULL;
	size_t uuid_len = 0;

	if(msg == NULL || uuid_buf == NULL)
	{
		IPCAM_DBG(WARN, "msg is null\n");
		return OPENMODE_RTN_FAIL;
	}

	if (_get_wsd_ns(msg, find_namespace, wsd_ns) != 1)
	{
		IPCAM_DBG(WARN, "WS-Discovery namespace not found\n");
		return OPENMODE_RTN_FAIL;
	}

	snprintf(uuid_st_str, sizeof(uuid_st_str), uuid_st_fmt, wsd_ns);
	snprintf(uuid_ed_str, sizeof(uuid_ed_str), uuid_ed_fmt, wsd_ns);

	uuid_st_ptr = strstr(msg, uuid_st_str);
	if(uuid_st_ptr == NULL)
	{
		IPCAM_DBG(WARN, "not found uuid start string(%s)\n", uuid_st_str);
		return OPENMODE_RTN_FAIL;
	}
	uuid_st_ptr += strlen(uuid_st_str);

	uuid_ed_ptr = strstr(uuid_st_ptr, uuid_ed_str);
	if(uuid_ed_ptr == NULL)
	{
		IPCAM_DBG(WARN, "not found uuid end string(%s)\n", uuid_ed_str);
		return OPENMODE_RTN_FAIL;
	}

	uuid_len = uuid_ed_ptr - uuid_st_ptr;
	if((uuid_len+1) < buf_size)
	{
		memcpy(uuid_buf, uuid_st_ptr, uuid_len);
		uuid_buf[uuid_len] = 0;
	}
	else
	{
		memcpy(uuid_buf, uuid_st_ptr, buf_size-1);
		uuid_buf[buf_size-1] = 0;
	}

	IPCAM_DBG(MINOR, "uuid = (%s)\n", uuid_buf);

	return OPENMODE_RTN_OK;
}

static int _is_host_ipaddr(unsigned int ipaddr)
{
	int i;
	for(i = 0; i < _net_inf_cnt; i++){
		IPCAM_DBG(MINOR, "index[%x] ipaddr[%x] list[%x]\n", i, ipaddr, inet_addr(_inf_tbl[i].inf_addr));
		if(inet_addr(_inf_tbl[i].inf_addr) == ipaddr){
			return 1;
		}
	}
	return 0;
}

static int _is_host_subnet(unsigned int ipaddr)
{
	int idx;
	int discovery_tbl_max = _get_discovery_tbl_count();
	for(idx = 0; idx < discovery_tbl_max; idx++){
		/*
			char buf1[100];
			char buf2[100];
			char buf3[100];
			IPCAM_DBG(MINOR, "idx[%d] ipaddr[%s] discovery hostip[%s] netmask[%s]\n", 
					idx,
					_ip_to_str(ipaddr, buf1), 
					_ip_to_str(_discovery_tbl[idx].inf_addr, buf2), 
					_ip_to_str(_discovery_tbl[idx].inf_mask, buf3));
		*/
		if((ipaddr & _discovery_tbl[idx].inf_mask) == (_discovery_tbl[idx].inf_addr & _discovery_tbl[idx].inf_mask)){
			break;
		}
	}
	IPCAM_DBG(MINOR, "discovery_tbl_max[%d] idx[%d]\n", discovery_tbl_max, idx);
	if(idx == discovery_tbl_max){
		return 0;
	}
	return 1;
}

static OPENMODE_RTN_ENUM _discovery_buf_find_addr(gchar* msg, NFOpenmodeCamInfo* entry)
{
	const char find_namespace[] = "\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"";
	gint addr_cnt = 0;
	gchar *s,*s1, *e, *e1, *p1, *p2, *p3, *p4, *p5, *ps;
	gchar xaddr_entry[ONVIF_XADDR_LEN] = {0};
	size_t xaddr_len = 0;
	//uint32_t host_ipaddr = 0;
	uint32_t host_mask = 0;
	gchar ipstr[16];
	gchar portstr[8];
	const gchar xaddr[] = "<%s:XAddrs>";
	const gchar xaddr_end[] = "</%s:XAddrs>";
	gchar find_svc[64];
	gchar find_svc_end[64];
	gchar wsd_ns[32];
	const gchar find_http[] = "http://";
	const gchar find_https[] = "https://";
	uint32_t curr_ipaddr = 0;
	int curr_http_port = 0;
	char curr_hostname[16] = {0};
	char curr_tail[256] = {0};
	int curr_pri = NET_ADDR_UNKNOWN;
	uint32_t best_ipaddr = 0;
	int best_http_port = 0;
	char best_hostname[16] = {0};
	char best_tail[256] = {0};
	int best_pri = NET_ADDR_UNKNOWN;

	/* get host info */
#if 0
	host_ipaddr = ntohl(_host_ip);
	if (host_ipaddr == 0)
	{
		IPCAM_DBG(WARN, "Host ipaddress error\n");
		return OPENMODE_RTN_FAIL;
	}
	IPCAM_DBG(MINOR, "Host IP Address(%d.%d.%d.%d)\n",
			(host_ipaddr&0xff000000)>>24,
			(host_ipaddr&0xff0000)>>16,
			(host_ipaddr&0xff00)>>8,
			_host_ip&0xff);

	host_mask = ntohl(_host_subnet);
	if (host_mask == 0)
	{
		IPCAM_DBG(WARN, "Host netmask error\n");
		return OPENMODE_RTN_FAIL;
	}
	IPCAM_DBG(MINOR, "Host Subnet(%d.%d.%d.%d)\n",
			(host_mask&0xff000000)>>24,
			(host_mask&0xff0000)>>16,
			(host_mask&0xff00)>>8,
			host_mask&0xff);
#endif
	memset(wsd_ns, 0x00, 32);
	if (_get_wsd_ns(msg, find_namespace, wsd_ns) != 1)
	{
		IPCAM_DBG(WARN, "WS-Discovery namespace not found\n");
		return OPENMODE_RTN_FAIL;
	}

	memset(find_svc, 0x00, 64);
	memset(find_svc_end, 0x00, 64);
	snprintf(find_svc, 64, xaddr, wsd_ns);
	snprintf(find_svc_end, 64, xaddr_end, wsd_ns);
	IPCAM_DBG(MINOR, "XAddr tag name is '%s'\n", find_svc);

	/* Find <XAddrs> */
	s = strstr(msg, find_svc);
	if (s == NULL)
	{
		IPCAM_DBG(WARN, "No service address\n");
		return OPENMODE_RTN_FAIL;
	}
	else
	{
		s1 = s + strlen(find_svc);
		e1 = strstr(s1, find_svc_end);
	}

	s = s1;

	xaddr_len = (size_t)(e1-s);
	if(xaddr_len >= ONVIF_XADDR_LEN)
	{
		xaddr_len = ONVIF_XADDR_LEN-1;
	}

	memset(xaddr_entry, 0x00, ONVIF_XADDR_LEN);
	memcpy(xaddr_entry, s, xaddr_len);
	IPCAM_DBG(MINOR, "XAddr : %s\n", xaddr_entry);

	p1 = xaddr_entry;
	while (1)
	{
		if (nf_ipcam_is_vendor_s1() == 0)
		{
			p2 = strstr(p1, find_http);
			if (p2 == NULL) {
				ps = strstr(p1, find_https);
			}
		}
		else
		{
			p2 = strstr(p1, find_https);
		}
		if (p2 == NULL && ps == NULL)
		{
			IPCAM_DBG(WARN, "No more http address\n");
			break;
		}
		if (nf_ipcam_is_vendor_s1() == 0)
		{
			if (p2)
				s = p2 + strlen(find_http);
			else
				s = ps + strlen(find_https);
		}
		else
		{
			s = p2 + strlen(find_https);
		}
		
		p3 = strstr(s, ":");
		p4 = strstr(s, "/");		
		if ((p3 == NULL) || (p3>p4))
		{
			e = p4;
			memset(ipstr, 0x00, 16);
			memset(portstr, 0x00, 8);
			memcpy(ipstr, s, (size_t)(e-s));
		}
		else if (p3<p4)
		{
			e = p3;
			memset(ipstr, 0x00, 16);
			memset(portstr, 0x00, 8);
			memcpy(ipstr, s, (size_t)(e-s));
			memcpy(portstr, (p3+1), (size_t)(p4-(p3+1)));
		}

		curr_ipaddr = inet_addr(ipstr);
		if(_is_host_ipaddr(curr_ipaddr)) {
			IPCAM_DBG(WARN, "Host except\n");
			return OPENMODE_RTN_FAIL;
		}
		curr_ipaddr = ntohl(curr_ipaddr);

		strncpy(curr_hostname, ipstr,16);
		if (portstr[0] != 0x00)
		{
			curr_http_port = atoi(portstr);
		}
		else
		{
			if (nf_ipcam_is_vendor_s1() == 0)
			{
				curr_http_port = 80;
			}
			else
			{
				curr_http_port = 443;
			}
		}
		IPCAM_DBG(MINOR, "Address(%d.%d.%d.%d:%d)\n",
				(curr_ipaddr&0xff000000)>>24,
				(curr_ipaddr&0xff0000)>>16,
				(curr_ipaddr&0xff00)>>8,
				(curr_ipaddr&0xff),
				curr_http_port);

		s = p4+1;
		p5 = strstr(s, " ");
		if (p5 != NULL)
		{
			e = p5;
			memset(curr_tail, 0x00, 256);
			memcpy(curr_tail, s, (size_t)(e-s));
		}
		else
		{
			memset(curr_tail, 0x00, 256);
			strcpy(curr_tail, s);
		}

		curr_pri = _get_inf_priority_to_addr(curr_ipaddr);
		if(curr_pri < best_pri)
		{
			best_ipaddr = curr_ipaddr;
			best_http_port = curr_http_port;
			strncpy(best_hostname, curr_hostname, 16);
			strncpy(best_tail, curr_tail, 256);
			best_pri = curr_pri;
		}

		if(p5 == NULL)
		{
			break;
		}
		else
		{
			p1 = p5;
		}
	}

	if(best_pri == NET_ADDR_UNKNOWN)
	{
		IPCAM_DBG(WARN, "Not found valid address\n");
		return OPENMODE_RTN_FAIL;
	}
	else
	{
		entry->ipaddr = best_ipaddr;
		entry->http_port = best_http_port;
		strncpy(entry->hostname, best_hostname, 16);
		strncpy(entry->tail, best_tail, 256);
		entry->ipaddr_pri = best_pri;
	}
	printf("★★★ [%s] - IP : [%d.%d.%d.%d]\n", entry->hostname, 
			(entry->ipaddr&0xff000000)>>24,
			(entry->ipaddr&0xff0000)>>16,
			(entry->ipaddr&0xff00)>>8,
			entry->ipaddr&0xff);

	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _discovery_buf_find_name_and_auth(gchar* msg, NFOpenmodeCamInfo* entry)
{
	char* pt1 = NULL;
	char* pt2 = NULL;
	char* end = NULL;
	char name[64];
	char* onvif_name = "onvif://www.onvif.org/name/";

	if(msg == NULL || entry == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}

	pt1 = strstr(msg, onvif_name);

	if(pt1 == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}

	pt1 += strlen(onvif_name);

	pt2 = strstr(pt1, " ");

	if(pt2 == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}

	memset(name, 0x00, sizeof(name));
	// SWPFOURCE-915
	if(pt1[63]!=NULL)
	{
		memcpy(name, pt1, 63);
		name[63]=NULL;
	}
	else
	{
		memcpy(name, pt1, pt2-pt1);
	}
	//memcpy(name, pt1, pt2-pt1);
	IPCAM_DBG(MINOR, "ONVIF Device Discovery Name: [%s]\n", name);

	if(strcmp(name, "Grundig") == 0 || strcmp(name, "GRUNDIG") == 0)
	{
		strcpy(entry->u, "admin");
		strcpy(entry->p, "1234");
		strcpy(entry->u_done, "admin");
		strcpy(entry->p_done, "1234");
		entry->auth = NF_ONVIF_AUTH_TEXT;
	}

	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _discovery_detection_list_update(NFOpenmodeCamInfo* new_entry)
{
	gint i = 0;
	gint dup_ip = 0;
	gint dup_name = 0;
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeCamInfo *temp = NULL;


	IPCAM_DBG(MAJOR, "start\n");

	if (_openmode_detection_list.entry_cnt == 0)
	{
		new_entry->index = 0;
		new_entry->prev = NULL;
		new_entry->next = NULL;
		_openmode_detection_list.entry_cnt = 1;
		_openmode_detection_list.head = new_entry;
		_openmode_detection_list.tail = new_entry;
		new_entry->state = OPENMODE_CAM_STATE_DISCOVERED;
		IPCAM_DBG(MAJOR, "end - updated\n");
		return OPENMODE_RTN_OK;
	}

	/* find same entry by mac */
	iter = _list_find_entry_by_mac(&_openmode_detection_list, new_entry->macaddr);
	if (iter != NULL)
	{
		char buf1[100], buf2[100];
		if(_is_linklocal_address(new_entry->ipaddr) && !_is_linklocal_address(iter->ipaddr)){
			//not change 
			IPCAM_DBG(MINOR, "onvif discovery ip is not changed list_ip[0x%08x] curr_ip[0x%08x]\n", _ip_to_str(iter->ipaddr, buf1), _ip_to_str(new_entry->ipaddr, buf2));
		}else{
			//change
			if(new_entry->ipaddr != iter->ipaddr){
				IPCAM_DBG(MINOR, "onvif discovery ip is changed list_ip[0x%08x] curr_ip[0x%08x]\n", iter->ipaddr, new_entry->ipaddr);
				iter->ipaddr = new_entry->ipaddr;
			}
		}

		IPCAM_DBG(MAJOR, "end - Mac Address duplicated\n");
		return OPENMODE_RTN_FAIL;
	}


	/* find same entry */
	iter = _list_find_entry_by_ip(&_openmode_detection_list, new_entry->ipaddr);
	if (iter != NULL)
	{
		IPCAM_DBG(MAJOR, "end - IP Address duplicated\n");
		return OPENMODE_RTN_FAIL;
	}

	/* add entry */
	_openmode_detection_list.tail->next = new_entry;
	new_entry->prev = _openmode_detection_list.tail;
	new_entry->next = NULL;
	_openmode_detection_list.tail = new_entry;
	new_entry->index = _openmode_detection_list.entry_cnt++;
	new_entry->state = OPENMODE_CAM_STATE_DISCOVERED;
	IPCAM_DBG(MAJOR, "end - updated\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _list_compare_recorder_list(NFOpenmodeCamInfo* entry)
{
	NFOpenmodeCamInfo *rec_iter = NULL;

	rec_iter = _openmode_recorder_list.head;
	while (rec_iter != NULL)
	{
		if (entry->ipaddr == rec_iter->ipaddr)
		{
			// IPCAM_DBG(MINOR, "Found recorder list entry with same IP address\n");
			printf("★★★ Found recorder list entry with same IP address [%d.%d.%d.%d]\n",
					(entry->ipaddr&0xff000000)>>24,
					(entry->ipaddr&0xff0000)>>16,
					(entry->ipaddr&0xff00)>>8,
					entry->ipaddr&0xff);
			return OPENMODE_RTN_FAIL;
		}
		rec_iter = rec_iter->next;
	}

	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _add_existing_ch(gint i)
{
	NFOpenmodeCamInfo *entry;
	NFOpenmodeCamInfo *live_entry;

	IPCAM_DBG(MAJOR, "start CH(%d)\n", i);

	entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
	_alloc_cnt++;
	if (entry == NULL)
	{
		IPCAM_DBG(WARN, "DetectionList entry allocation failure\n");
		return OPENMODE_RTN_FAIL;
	}

	if (_db_host[i][0] == '\0' || _db_http_port[i] == 0)
	{
		free(entry);
		_alloc_cnt--;
		return OPENMODE_RTN_FAIL;
	}

	memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));

#if 0
	live_entry = _list_find_entry_by_ch(&_openmode_live_list, i);
	if (live_entry == NULL)
	{
		IPCAM_DBG(MINOR, "No live entry\n");
		free(entry);
		_alloc_cnt--;
		return OPENMODE_RTN_FAIL;
	}

	memcpy(entry, live_entry, sizeof(NFOpenmodeCamInfo));
#else
	live_entry = _list_find_entry_by_ch(&_openmode_live_list, i);
	if (live_entry != NULL && live_entry->state == OPENMODE_CAM_STATE_OK)
	{
		memcpy(entry, live_entry, sizeof(NFOpenmodeCamInfo));
		if(entry->virtual_camera == OPENMODE_STATE_VIRTUAL_SUPPORTED)
		{
			entry->state = OPENMODE_CAM_STATE_VIRTUAL_CAMERA;
		}
		else if (entry->media_xaddr[0] != '\0')
		{
			entry->state = OPENMODE_CAM_STATE_DEV_INFO_ONVIF;
		}
		else
		{
			entry->state = OPENMODE_CAM_STATE_DEV_INFO;
		}

	}
	else
	{
		entry->ch = i;
		entry->ipaddr = ntohl(_get_ipaddr_from_hostname(_db_host[i]));
		strncpy(entry->hostname, _db_host[i], 256);
		entry->http_port = _db_http_port[i];
		strncpy(entry->u, _db_u[i], 64);
		strncpy(entry->p, _db_p[i], 64);

		entry->rtsp_port = _db_rtsp_port[i];

		entry->virtual_camera = _db_vcam[i];
		entry->vcam_cnt= _db_vcam_cnt[i];

		strncpy(entry->vcam_rtsp_addr[0], _db_rtsp_addr_main[i], 256);
		strncpy(entry->vcam_rtsp_addr[1], _db_rtsp_addr_second[i], 256);

		if (entry->virtual_camera == OPENMODE_STATE_VIRTUAL_SUPPORTED)
		{
			strncpy(entry->model, _db_model[i], 64);
		}

		strncpy(entry->eth_dev, _db_ethernet[i], 4);

	}

#endif

	entry->index = _openmode_detection_list.entry_cnt;
	if (entry->state != OPENMODE_CAM_STATE_DEV_INFO && entry->state != OPENMODE_CAM_STATE_DEV_INFO_ONVIF
			&& entry->state != OPENMODE_CAM_STATE_VIRTUAL_CAMERA)
	{
		entry->state = OPENMODE_CAM_STATE_DISCOVERED;
	}
	entry->next = NULL;
	entry->prev = NULL;

	_openmode_list_add(&_openmode_detection_list, entry);
	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_list_add(NFOpenmodeDeviceList* list, NFOpenmodeCamInfo* new_entry)
{
	NFOpenmodeCamInfo *iter = list->tail;

	IPCAM_DBG(MAJOR, "start\n");

	if (iter == NULL)
	{
		_openmode_list_mtx_lock();
		list->head = new_entry;
		list->tail = new_entry;
		list->entry_cnt = 1;
		_openmode_list_mtx_unlock();
		IPCAM_DBG(MAJOR, "end\n");
		return OPENMODE_RTN_OK;
	}

	_openmode_list_mtx_lock();
	iter->next = new_entry;
	new_entry->prev = iter;
	new_entry->next = NULL;
	list->tail = new_entry;
	list->entry_cnt++;
	if (new_entry->ch >= 0 && new_entry->ch < _max_ch) { list->assigned_cnt++; }
	_openmode_list_mtx_unlock();

	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_list_remove(NFOpenmodeDeviceList* list, NFOpenmodeCamInfo* del_entry)
{
	IPCAM_DBG(MAJOR, "start\n");

	if (list->entry_cnt == 0 || del_entry == NULL)
	{
		IPCAM_DBG(WARN, "No entry to delete\n");
		return OPENMODE_RTN_FAIL;
	}

	_openmode_list_mtx_lock();

	if (del_entry->prev != NULL)
	{
		del_entry->prev->next = del_entry->next;
	}
	else
	{
		list->head = del_entry->next;
	}

	if (del_entry->next != NULL)
	{
		del_entry->next->prev = del_entry->prev;
	}
	else
	{
		list->tail = del_entry->prev;
	}

	list->entry_cnt--;
	if (del_entry->ch >= 0 && del_entry->ch < _max_ch) { list->assigned_cnt--; }

	_openmode_list_mtx_unlock();

	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static void _send_onvif_search(void)
{
	char buf[2048] = {0};
	int cin_len = 0;
	int send_len = 0;

	char msg_id[64] = {0};
	char uuid_buf[40] = {0};

	struct sockaddr_in cin;

	int idx = 0;
	int discovery_cnt = 0;
	int multi_sock = -1;

	IPCAM_DBG(MAJOR, "start\n");

	memset(&cin, 0x00, sizeof(cin));
	cin.sin_family = AF_INET;
	cin.sin_addr.s_addr = inet_addr(_MCAST_GRP_ADDR);
	cin.sin_port = htons(_WS_DISC_PORT);
	cin_len = sizeof(cin);

	discovery_cnt = _get_discovery_tbl_count();

	for(idx = 0; idx < discovery_cnt; idx++)
	{
		multi_sock = _get_discovery_multi_sock(idx);
		if(multi_sock < 0) { continue; }

		memset(msg_id, 0x00, 64);
		memset(uuid_buf, 0x00, sizeof(uuid_buf));
		nf_ipcam_get_sysproc_uuid(uuid_buf);
		snprintf(msg_id, 64, _MSG_ID_FORMAT, uuid_buf);
		memset(buf, 0x00, 2048);
		snprintf(buf, 2048, discovery_msg, msg_id);

		send_len = sendto(multi_sock, buf, strlen(buf), 0, (struct sockaddr*)&cin, cin_len);
		if (send_len < 0)
		{
			IPCAM_DBG(ERROR, "onvif search send failed\n");
			perror("sendto");
			continue;
		}
	}

	IPCAM_DBG(MAJOR, "end\n");
}

static void _send_itx_search(void)
{
    struct sockaddr_in sin;
    netconf_msg netconf;
    
    // 전송할 인터페이스 리스트 (기존 매크로 활용)
    const char* target_devs[] = {HOST_ETH_DEV, HUB_ETH_DEV};
    int i;

    if (_send_sock < 0) {
        IPCAM_DBG(ERROR, "_send_sock disabled\n");
        return;
    }

    // 패킷 데이터 초기화 (기존 로직 유지)
    memset(&netconf, 0x00, sizeof(netconf_msg));
    netconf.version = 1;
    netconf.opcode = MSG_IP_SEARCH;
    netconf.magic = htonl(0x69547843);

    // 각 인터페이스를 순회하며 전송
    for (i = 0; i < 2; i++)
	{
        guint ip = get_netif_ip(target_devs[i]);
        guint mask = get_netif_mask(target_devs[i]);

        if (ip == 0 || mask == 0) {
            IPCAM_DBG(MINOR, "Skip interface %s (No IP or Mask)\n", target_devs[i]);
            continue;
        }

        // 브로드캐스트 주소 계산: (IP | ~MASK)
        // 예: 10.20.0.1 | ~255.255.255.0 = 10.20.0.255
        guint bcast_ip = ip | (~mask);

        memset(&sin, 0x00, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = bcast_ip; // 계산된 브로드캐스트 주소 적용
        sin.sin_port = htons(_ADMIN_CLI_PORT);

        int len = sendto(_send_sock, (void*)&netconf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));
        
        if (len < 0) {
            g_message("[%s, %d]Failed to send to %s (%d.%d.%d.%d)", __FUNCTION__, __LINE__,
                target_devs[i], bcast_ip&0xff, (bcast_ip>>8)&0xff, (bcast_ip>>16)&0xff, (bcast_ip>>24)&0xff);
        } else {
            g_message("[%s, %d]Sent search to %s, IP: %d.%d.%d.%d", __FUNCTION__, __LINE__,
                target_devs[i], bcast_ip&0xff, (bcast_ip>>8)&0xff, (bcast_ip>>16)&0xff, (bcast_ip>>24)&0xff);
        }
    }
}

static void _send_itx_ip_assign(netconf_msg* conf)
{
	int i;
	unsigned char *p;
	int len = 0;
	struct sockaddr_in sin;

	IPCAM_DBG(MAJOR, "start\n");

#if 0
	IPCAM_DBG(MINOR, "version(%d) opcode(%d) secs(%d) xid(%08x) magic(%08x)\n",
			conf->version, conf->opcode, conf->secs, conf->xid, conf->magic);
	IPCAM_DBG(MINOR, "ci(%d.%d.%d.%d) ch(%02x-%02x-%02x-%02x-%02x-%02x) yi(%d.%d.%d.%d)\n",
			(conf->ciaddr&0xff),
			(conf->ciaddr&0xff00)>>8,
			(conf->ciaddr&0xff0000)>>16,
			(conf->ciaddr&0xff000000)>>24,
			conf->chaddr[0], conf->chaddr[1], conf->chaddr[2],
			conf->chaddr[3], conf->chaddr[4], conf->chaddr[5],
			(conf->yiaddr&0xff),
			(conf->yiaddr&0xff00)>>8,
			(conf->yiaddr&0xff0000)>>16,
			(conf->yiaddr&0xff000000)>>24);
	IPCAM_DBG(MINOR, "mi(%d.%d.%d.%d) gi(%d.%d.%d.%d) http(%d) https(%d) rtsp(%d)\n",
			(conf->miaddr&0xff),
			(conf->miaddr&0xff00)>>8,
			(conf->miaddr&0xff0000)>>16,
			(conf->miaddr&0xff000000)>>24,
			(conf->giaddr&0xff),
			(conf->giaddr&0xff00)>>8,
			(conf->giaddr&0xff0000)>>16,
			(conf->giaddr&0xff000000)>>24,
			conf->http_port, conf->https_port, conf->rtsp_port);
#endif
	if (_send_sock < 0)
	{
		IPCAM_DBG(ERROR, "_send_sock disabled\n");
		return;
	}

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_BROADCAST;
	sin.sin_port = htons(_ADMIN_CLI_PORT);

	len = sendto(_send_sock, (void*) conf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

	if (len < 0)
	{
		IPCAM_DBG(ERROR, "broadcast send failed\n");
		perror("sendto");
	}

	IPCAM_DBG(MAJOR, "end\n");
}

static void _init_switch_device(void)
{
	// Set Vlan OPEN MODE. 
	nf_dev_switch_init(NF_UTIL_SWITCH_OPEN_MODE);
}

static void _req_stop_detail(void)
{
	_detail_stop_requested = 1;
}

static void _list_search_model_detail_live(int ch)
{
	int i=0;
	int use_ssl;
	int rtn;
	char device_xaddr[256];
	mtable *runtime = get_runtime();
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeSetupNetwork netinfo;
	NFOpenmodeDeviceList *list = &_openmode_live_list;
	ipcam_onvif_auth_info_t auth_info;



	iter = _openmode_live_list.head;
	for (i=0; i<_max_ch; i++)
	{
		if (iter == NULL) { break; }
		if (iter->state == OPENMODE_CAM_STATE_DISCOVERED)
		{
			nf_pnd_prog_notify_fire(i, 5, __LINE__, __FILE__);
		}
		iter = iter->next;
	}
	//iter = list->head;
	//for (i=0; i<list->entry_cnt; i++)
	for (iter=list->head, i=0; iter != NULL; iter=iter->next,i++)
	{
		int _is_default_pw = 0;
		int rtn = 0;
		cam_model_info info;

		if (_live_stop_requested > 0)
		{
			IPCAM_DBG(WARN, "requested stopping detail search\n");
			break;
		}
		if (iter == NULL) { return; }
		if (iter->state != OPENMODE_CAM_STATE_DISCOVERED) { continue; }
			/* find model detail information */
		if (iter->virtual_camera == OPENMODE_STATE_VIRTUAL_SUPPORTED)
		{
			goto virtual_dev;
		}

		if(i != ch) continue;

		memset(&info, 0x00, sizeof(cam_model_info));
		//strncpy(iter->u_done, "ADMIN", 64);
		//strncpy(iter->p_done, "1234", 64);
		_is_default_pw = 1;

		nf_ipcam_get_default_login_info(iter->u_done, iter->p_done);
		rtn = cam_get_model_info_raw(&info, htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, &use_ssl);
		g_message("###yanggungg : %s, %d ### cam_get_model_info_raw rtn : %d, ip : %d.%d.%d.%d, port : %d, user : %s, pass : %s\n", __FUNCTION__, __LINE__, rtn,
				(iter->ipaddr&0xff000000)>>24,
				(iter->ipaddr&0xff0000)>>16,
				(iter->ipaddr&0xff00)>>8,
				iter->ipaddr&0xff,
				iter->http_port,
				iter->u_done,
				iter->p_done);
		if (rtn == 1)
		{
			IPCAM_DBG(WARN, "default login fail(%d)\n", iter->index);
			if (iter->u[0] == '\0')
			{
				IPCAM_DBG(WARN, "no login info specified(%d)\n", iter->index);
				goto login_result;
			}
			strncpy(iter->u_done, iter->u, 64);
			strncpy(iter->p_done, iter->p, 64);
			_is_default_pw = 0;
			rtn = cam_get_model_info_raw(&info, htonl(iter->ipaddr), iter->http_port, iter->u, iter->p, &use_ssl);
		}

login_result:
		if (rtn < (-1))
		{
			IPCAM_DBG(WARN, "socket connect failed(%d)\n", iter->index);
			iter->ipaddr = _host_to_ip(iter->hostname);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (list == &_openmode_live_list)
			{
				IPCAM_DBG(WARN, "connection fail pnd event notify\n");
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				nf_eventlog_put_ipcam_msg("Connection Fail(ITX)", iter->ch);
			}
			continue;
		}
		else if (rtn <= 0)
		{
			IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
			if (iter->xid != 0)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				nf_eventlog_put_ipcam_msg("Connection Fail(ITX)", iter->ch);
				iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
				continue;
			}
			runtime[iter->ch].conn_type = 1;
			goto onvif_dev;
		}
		runtime[iter->ch].conn_type = 0;
		if (rtn == 1)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
				nf_eventlog_put_ipcam_msg("Login Fail(ITX)", iter->ch);
			}
#endif
			continue;
		}
		if (use_ssl)
		{
			iter->use_ssl = 1;
		}
		else
		{
			iter->use_ssl = 0;
		}

		if(!_is_mac_null(iter->macaddr) && !_is_mac_equal(iter->macaddr, info.mac) && _is_linklocal_address(iter->ipaddr)){
			char logbuf[400];
			char ip[20];
			sprintf(ip, "%d.%d.%d.%d",
					(iter->ipaddr&0xff000000)>>24,
					(iter->ipaddr&0xff0000)>>16,
					(iter->ipaddr&0xff00)>>8,
					(iter->ipaddr&0xff));

			IPCAM_DBG(MAJOR, "itx protocol linklocal_ip conflict ip[%s] iter->mac(%02x-%02x-%02x-%02x-%02x-%02x) info.mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
					ip,
					iter->macaddr[0], iter->macaddr[1], iter->macaddr[2], iter->macaddr[3], iter->macaddr[4], iter->macaddr[5],
					info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]);

			//write nand log
			sprintf(logbuf, "itx linklocal conflict ip[%s] iter->mac(%02x-%02x-%02x-%02x-%02x-%02x) info.mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
					ip,
					iter->macaddr[0], iter->macaddr[1], iter->macaddr[2], iter->macaddr[3], iter->macaddr[4], iter->macaddr[5],
					info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]);
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, logbuf);

			// err code
			nf_notify_fire_params("set_ip_conflict", 1, 0, 0, 0);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			continue;
		}

#if 1
		iter->rtsp_port = cam_get_rtsp_port(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done);

		if (iter->rtsp_port <= 0)
		{
			nf_eventlog_put_ipcam_msg("Get RTSP port Fail(ITX)", iter->ch);
			if (_is_default_pw && iter->use_ssl)
			{
				IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
						__OPENMODE_CAM_STATE_STR_[iter->state],
						__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_PW_CHANGE]);
				iter->state = OPENMODE_CAM_STATE_PW_CHANGE;
				strcpy(iter->u, "ADMIN");
				strcpy(iter->u_done, "ADMIN");
			}
			else
			{
				IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
				if (list == &_openmode_live_list)
				{
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				}
				iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			}
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
			}
			continue;
		}

		memset(&netinfo, 0x00, sizeof(NFOpenmodeSetupNetwork));
		rtn = cam_get_network(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, &netinfo);
		if (rtn <= 0)
		{
			IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
			nf_eventlog_put_ipcam_msg("Get Network info Fail(ITX)", iter->ch);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
			}
			continue;
		}
		{
			unsigned int h,m,_gotip;
			h = _host_ip;
			m = _host_subnet;
			_gotip = htonl(netinfo.ipaddr);

			IPCAM_DBG(MINOR, "h(%08x) m(%08x) _nowip(%08x) _gotip(%08x)\n", h,m,iter->ipaddr,_gotip);

			if ((m != 0) && ((h&m) == (iter->ipaddr&m)))
			{
				iter->ipaddr = netinfo.ipaddr;
			}
		}
		iter->mask = netinfo.mask;
		iter->gw = netinfo.gw;
		iter->dns1 = netinfo.dns1;
		iter->dns2 = netinfo.dns2;
#endif

		IPCAM_DBG(MINOR, "index(%d)\n", iter->index);
		IPCAM_DBG(MINOR, "name(%s)\n", info.name);
		strncpy(iter->model, info.name, 64);
		IPCAM_DBG(MINOR, "swver(%s)\n", info.swver);
		strncpy(iter->firmware_version, info.swver, 64);
		IPCAM_DBG(MINOR, "swvers2(%s)\n", info.swver2);
		strncpy(iter->firmware_version2, info.swver2, 64);
		IPCAM_DBG(MINOR, "vendor(%s)\n", info.vendor);
		strncpy(iter->vendor, info.vendor, 64);
		IPCAM_DBG(MINOR, "stdver(%s)\n", info.stdver);
		strncpy(iter->model_std, info.stdver, 64);
		IPCAM_DBG(MINOR, "sdkver(%s)\n", info.sdkver);
		strncpy(iter->sdkver, info.sdkver, 64);
		IPCAM_DBG(MINOR, "mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
				info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]
		);

		if(info.zoom_module_name != NULL)
			strncpy(iter->zoom_module_name, info.zoom_module_name, 63);
		if(info.zoom_module_fwver != NULL)
			strncpy(iter->zoom_module_fwver, info.zoom_module_fwver, 63);

		IPCAM_DBG(MINOR, "ip(%d.%d.%d.%d)\n",
				(iter->ipaddr&0xff000000)>>24,
				(iter->ipaddr&0xff0000)>>16,
				(iter->ipaddr&0xff00)>>8,
				(iter->ipaddr&0xff));
		IPCAM_DBG(MINOR, "mask(%d.%d.%d.%d)\n",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));
		IPCAM_DBG(MINOR, "gw(%d.%d.%d.%d)\n",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));
		IPCAM_DBG(MINOR, "dns1(%d.%d.%d.%d)\n",
				(iter->dns1&0xff000000)>>24,
				(iter->dns1&0xff0000)>>16,
				(iter->dns1&0xff00)>>8,
				(iter->dns1&0xff));
		IPCAM_DBG(MINOR, "dns2(%d.%d.%d.%d)\n",
				(iter->dns2&0xff000000)>>24,
				(iter->dns2&0xff0000)>>16,
				(iter->dns2&0xff00)>>8,
				(iter->dns2&0xff));
		memcpy(iter->macaddr, info.mac, 6);
		strncpy(iter->u, iter->u_done, 64);
		strncpy(iter->p, iter->p_done, 64);


		//if (_is_default_pw && iter->use_ssl)
		if(0)
		{
			IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
					__OPENMODE_CAM_STATE_STR_[iter->state],
					__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_PW_CHANGE]);
			iter->state = OPENMODE_CAM_STATE_PW_CHANGE;
			strcpy(iter->u, "ADMIN");
			strcpy(iter->u_done, "ADMIN");
		}
		else
		{
			IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
					__OPENMODE_CAM_STATE_STR_[iter->state],
					__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_DEV_INFO]);
			iter->state = OPENMODE_CAM_STATE_DEV_INFO;
			if (_last_preview_id == iter->index)
			{
				nf_openmode_set_preview(iter->index, 0);
			}
		}

		continue;

onvif_dev:
		{
			guchar _zero[6];
			memset(_zero, 0x00, 6);
			if (memcmp(iter->macaddr, _zero, 6) != 0)
			{
				if (_is_itx_mac_range(iter->macaddr) == OPENMODE_RTN_OK)
				{
					IPCAM_DBG(WARN, "ITX cam index(%d) ch(%d)\n", iter->index, iter->ch);
					iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
					continue;
				}
			}
		}
		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_dev_info_raw(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 1,
					iter->u, iter->p, &info
			);
		}
		else
		{
			rtn = nf_onvif_get_dev_info_raw(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 0,
					iter->u, iter->p, &info
			);
		}

		if (rtn != 0)
		{
			if (iter->u[0] != '\0')
			{
				if (nf_ipcam_is_vendor_s1())
				{
					rtn = nf_onvif_get_dev_info_raw(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 1,
							iter->u, iter->p, &info
					);
				}
				else
				{
					rtn = nf_onvif_get_dev_info_raw(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 0,
							iter->u, iter->p, &info
					);
				}

				if (rtn != 0)
				{
					if (nf_ipcam_is_vendor_s1())
					{
						rtn = nf_onvif_get_dev_info_raw(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 1,
								iter->u, iter->p, &info
						);
					}
					else
					{
						rtn = nf_onvif_get_dev_info_raw(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 0,
								iter->u, iter->p, &info
						);
					}
				}
			}
		}

		if (rtn == 28) // connection fail
		{
			IPCAM_DBG(WARN, "connection fail(%d)\n", iter->index);
			nf_eventlog_put_ipcam_msg("Get Device info Fail", iter->ch);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
			}
			continue;
		}
		else if (rtn != 0)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			nf_eventlog_put_ipcam_msg("Get Device info Fail", iter->ch);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			//iter = iter->next;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
			}
#endif
			continue;
		}

		iter->auth = NF_ONVIF_AUTH_NONE;
		memset(iter->u_done, 0x00, 64);
		memset(iter->p_done, 0x00, 64);
		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_net_info(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 1,
					iter->u, iter->p, info.mac, &netinfo
			);
		}
		else
		{
			rtn = nf_onvif_get_net_info(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 0,
					iter->u, iter->p, info.mac, &netinfo
			);
		}

		if (rtn != 0)
		{
			if (iter->u[0] != '\0')
			{
				iter->auth = NF_ONVIF_AUTH_TEXT;
				strncpy(iter->u_done, iter->u, 64);
				strncpy(iter->p_done, iter->p, 64);
				if (nf_ipcam_is_vendor_s1())
				{
					rtn = nf_onvif_get_net_info(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 1,
							iter->u, iter->p, info.mac, &netinfo
					);
				}
				else
				{
					rtn = nf_onvif_get_net_info(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 0,
							iter->u, iter->p, info.mac, &netinfo
					);
				}

				if (rtn != 0)
				{
					iter->auth = NF_ONVIF_AUTH_DIGEST;
					strncpy(iter->u_done, iter->u, 64);
					strncpy(iter->p_done, iter->p, 64);
					if (nf_ipcam_is_vendor_s1())
					{
						rtn = nf_onvif_get_net_info(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 1,
								iter->u, iter->p, info.mac, &netinfo
						);
					}
					else
					{
						rtn = nf_onvif_get_net_info(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 0,
								iter->u, iter->p, info.mac, &netinfo
						);
					}
				}
			}
		}

		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->auth = NF_ONVIF_AUTH_NONE;
			memset(iter->u_done, 0x00, 64);
			memset(iter->p_done, 0x00, 64);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
				nf_eventlog_put_ipcam_msg("Get Network info Fail", iter->ch);
			}
#endif
			continue;
		}

		if(!_is_mac_null(iter->macaddr) && !_is_mac_equal(iter->macaddr, info.mac) && _is_linklocal_address(iter->ipaddr)){
			char logbuf[400];
			char ip[20];
			sprintf(ip, "%d.%d.%d.%d",
					(iter->ipaddr&0xff000000)>>24,
					(iter->ipaddr&0xff0000)>>16,
					(iter->ipaddr&0xff00)>>8,
					(iter->ipaddr&0xff));

			IPCAM_DBG(MAJOR, "onvif inklocal_ip conflict ip[%s] iter->mac(%02x-%02x-%02x-%02x-%02x-%02x) info.mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
					ip,
					iter->macaddr[0], iter->macaddr[1], iter->macaddr[2], iter->macaddr[3], iter->macaddr[4], iter->macaddr[5],
					info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]);

			//write nand log
			sprintf(logbuf, "onvif linklocal conflict ip[%s] iter->mac(%02x-%02x-%02x-%02x-%02x-%02x) info.mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
					ip,
					iter->macaddr[0], iter->macaddr[1], iter->macaddr[2], iter->macaddr[3], iter->macaddr[4], iter->macaddr[5],
					info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]);
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, logbuf);

			// err code
			nf_notify_fire_params("set_ip_conflict", 1, 0, 0, 0);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			continue;
		}


		strncpy(iter->u_done, iter->u, 64);
		strncpy(iter->p_done, iter->p, 64);
		strncpy(iter->model, info.name, 64);
		strncpy(iter->firmware_version, info.swver, 64);
		strncpy(iter->firmware_version2, info.swver2, 64);
		strncpy(iter->vendor, info.vendor, 64);
		strncpy(iter->model_std, info.name, 64);
		strncpy(iter->sdkver, info.sdkver, 64);
		
		if(info.zoom_module_name != NULL)
			strncpy(iter->zoom_module_name, info.zoom_module_name, 63);
		if(info.zoom_module_fwver != NULL)
			strncpy(iter->zoom_module_fwver, info.zoom_module_fwver, 63);
		
		memcpy(iter->macaddr, info.mac, 6);
		iter->is_dhcp = netinfo.is_dhcp;

		if (netinfo.mask == 24)
		{
			iter->mask = ntohl(inet_addr("255.255.255.0"));
		}
		else if (netinfo.mask == 16)
		{
			iter->mask = ntohl(inet_addr("255.255.0.0"));
		}
		else if (netinfo.mask == 8)
		{
			iter->mask = ntohl(inet_addr("255.0.0.0"));
		}
		snprintf(iter->maskstr, 16, "%d.%d.%d.%d",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));

		iter->gw = netinfo.gw;
		snprintf(iter->gwstr, 16, "%d.%d.%d.%d",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));

		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_media_xaddr(
					htonl(iter->ipaddr), iter->http_port, iter->tail,
					iter->auth, 1, iter->u_done, iter->p_done, iter->media_xaddr);
		}
		else
		{
			rtn = nf_onvif_get_media_xaddr(
					htonl(iter->ipaddr), iter->http_port, iter->tail,
					iter->auth, 0, iter->u_done, iter->p_done, iter->media_xaddr);
		}
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "media get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			//iter = iter->next;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
				nf_eventlog_put_ipcam_msg("Get Device Capability Fail", iter->ch);
			}
#endif
			continue;
		}

		{
			OPENMODE_RTN_ENUM _rtn;
			gchar _tail[256];
			_rtn = _get_address_tail(iter->media_xaddr, _tail);
			if (_rtn == OPENMODE_RTN_OK)
			{
				if (nf_ipcam_is_vendor_s1())
				{
					snprintf(iter->media_xaddr, 256, "https://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						_tail);
					snprintf(device_xaddr, 256, "https://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						"onvif/device_service");
				}
				else
				{
					snprintf(iter->media_xaddr, 256, "http://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						_tail);
					snprintf(device_xaddr, 256, "http://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						"onvif/device_service");
				}
			}
		}

		auth_info.auth_method = iter->auth;
		auth_info.username = iter->u;
		auth_info.password = iter->p;
		auth_info.endpoint = device_xaddr;
		rtn = nf_onvif_get_preview_profile(
				iter->media_xaddr, &auth_info, iter->token);
				//iter->media_xaddr, iter->auth, iter->u, iter->p, iter->token);
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "profile get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
				nf_eventlog_put_ipcam_msg("Get Profiles Fail", iter->ch);
			}
#endif
			continue;
		}

		rtn = nf_onvif_get_preview_uri(iter->media_xaddr, device_xaddr, iter->auth, iter->u, iter->p, iter->token, iter->preview_rtsp);
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "uri get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
				nf_eventlog_put_ipcam_msg("Get Stream URI Fail", iter->ch);
			}
#endif
			continue;
		}
		{
			char *p = iter->preview_rtsp;
			char *e = NULL;
			char _temp[8];

			memset(_temp, 0x00, 8);
			if (nf_ipcam_is_vendor_s1())
			{
				p = p + strlen("https://");
			}
			else
			{
				p = p + strlen("http://");
			}
			p = strstr(p, ":");
			if (p == NULL)
			{
				iter->rtsp_port = 554;
			}
			else
			{
				p++;
				e = strstr(p, "/");
				if (e == NULL)
				{
					iter->rtsp_port = 554;
				}
				else
				{
					memcpy(_temp, p, (e-p));
					iter->rtsp_port = atoi(_temp);
				}
			}
		}
		{
			OPENMODE_RTN_ENUM _rtn;
			gchar _tail[256];
			_rtn = _get_address_tail(iter->preview_rtsp, _tail);
			if (_rtn == OPENMODE_RTN_OK)
			{
				snprintf(iter->preview_rtsp, 256, "rtsp://%d.%d.%d.%d:%d/%s",
					(iter->ipaddr&0xff000000)>>24,
					(iter->ipaddr&0xff0000)>>16,
					(iter->ipaddr&0xff00)>>8,
					(iter->ipaddr&0xff),
					iter->rtsp_port,
					_tail);
			}
		}

		IPCAM_DBG(MINOR, "index(%d)\n", iter->index);
		IPCAM_DBG(MINOR, "name(%s)\n", info.name);
		IPCAM_DBG(MINOR, "swver(%s)\n", info.swver);
		IPCAM_DBG(MINOR, "swvers2(%s)\n", info.swver2);
		IPCAM_DBG(MINOR, "vendor(%s)\n", info.vendor);
		IPCAM_DBG(MINOR, "stdver(%s)\n", info.stdver);
		IPCAM_DBG(MINOR, "sdkver(%s)\n", info.sdkver);
		IPCAM_DBG(MINOR, "mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
				info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]
				);
		IPCAM_DBG(MINOR, "is_dhcp(%d)\n", netinfo.is_dhcp);
		IPCAM_DBG(MINOR, "prefix(%d)\n", netinfo.mask);
		IPCAM_DBG(MINOR, "mask(%d.%d.%d.%d)\n",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));
		IPCAM_DBG(MINOR, "gw(%d.%d.%d.%d)\n",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));
		IPCAM_DBG(MINOR, "media(%s)\n", iter->media_xaddr);
		IPCAM_DBG(MINOR, "profile(%s)\n", iter->token);
		IPCAM_DBG(MINOR, "uri(%s)\n", iter->preview_rtsp);
		IPCAM_DBG(MINOR, "rtsp_port(%d)\n", iter->rtsp_port);

		iter->state = OPENMODE_CAM_STATE_DEV_INFO_ONVIF;
		if (_last_preview_id == iter->index)
		{
			nf_openmode_set_preview(iter->index, 0);
		}
		continue;

virtual_dev:
		{
			int rtn = 0;
			int audio_flag = 0;
			rtn = vcam_get_state_info_from_describe_test(iter->u, iter->p, iter->vcam_rtsp_addr[0], iter->ipaddr, iter->rtsp_port, &audio_flag);

			if(rtn == -2)
			{
				iter->ipaddr = _host_to_ip(iter->hostname);
				IPCAM_DBG(ERROR, "connection fail(%s)\n", __FUNCTION__);
				iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
				if (list == &_openmode_live_list)
				{
					nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				}
				continue;
			}
			else if(rtn == -1)
			{

			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (list == &_openmode_live_list)
			{
				nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_LOGIN_FAIL, __LINE__, __FILE__);
			}
#if MAKE_NOTIFY_FIRE
			{
				GTimeVal tval;
				char log_buf[128];

				gettimeofday(&tval, NULL);
				snprintf(log_buf, 128, "DEVICE LOGIN FAILED : %d", iter->ch);
				nf_eventlog_put_param(&tval, LT_IPCAM, iter->ch, LP2_IPCAM_LOGIN_FAIL, log_buf);
			}
#endif
				continue;
			}

			if(rtn == 0)
			{
				iter->state = OPENMODE_CAM_STATE_VIRTUAL_CAMERA;
				//iter->state = OPENMODE_CAM_STATE_DEV_INFO_ONVIF;
				//strcpy(iter->model, "VCAM");
				//strcpy(iter->model_std, "VCAM");
				
				iter->vcam_audio_flag = audio_flag;
				
				if (_last_preview_id == iter->index)
				{
					nf_openmode_set_preview(iter->index, 0);
				}

			}

		}
	}
}

static void _list_search_model_detail(void)
{
	int i=0;
	int use_ssl;
	int rtn;
	char device_xaddr[256];
	unsigned char _cmp_ptn[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	NFOpenmodeCamInfo *iter = NULL;
	NFOpenmodeSetupNetwork netinfo;
	NFOpenmodeDeviceList *list = &_openmode_detection_list;
	ipcam_onvif_auth_info_t auth_info;


	//iter = list->head;
	//for (i=0; i<list->entry_cnt; i++)
	for (iter=list->head; iter != NULL; iter=iter->next)
	{
		int _is_default_pw = 0;

		if (_detail_stop_requested > 0)
		{
			IPCAM_DBG(WARN, "requested stopping detail search\n");
			break;
		}
		if (iter == NULL) { return; }
		if (iter->state != OPENMODE_CAM_STATE_DISCOVERED) { continue; }
		if (iter->virtual_camera == OPENMODE_STATE_VIRTUAL_SUPPORTED)
		{
			goto virtual_dev;
		}

		/* find model detail information */
		int rtn = 0;
		cam_model_info info;
		memset(&info, 0x00, sizeof(cam_model_info));
		//strncpy(iter->u_done, "ADMIN", 64);
		//strncpy(iter->p_done, "1234", 64);
		_is_default_pw = 1;
		nf_ipcam_get_default_login_info(iter->u_done, iter->p_done);
		rtn = cam_get_model_info_raw(&info, htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, &use_ssl);
		if (rtn == 1)
		{
			IPCAM_DBG(WARN, "default login fail(%d)\n", iter->index);
			if (iter->u[0] == '\0')
			{
				IPCAM_DBG(WARN, "no login info specified(%d)\n", iter->index);
				goto login_result;
			}
			strncpy(iter->u_done, iter->u, 64);
			strncpy(iter->p_done, iter->p, 64);
			_is_default_pw = 0;
			rtn = cam_get_model_info_raw(&info, htonl(iter->ipaddr), iter->http_port, iter->u, iter->p, &use_ssl);
		}

login_result:
		if (rtn < (-1))
		{
			IPCAM_DBG(WARN, "socket connect failed(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}
		else if (rtn <= 0)
		{
			IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
			if (iter->xid != 0)
			{
				//nf_pnd_evt_notify_fire(iter->ch, PND_TYPE_CONNECTION_FAIL, __LINE__, __FILE__);
				iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
				if (openmode_state == OPENMODE_STATE_RUNNING)
				{
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
				}
				else
				{
	#if BLOCKING_SCAN
					nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
	#else
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
	#endif
				}
				continue;
			}
			goto onvif_dev;
		}
		if (rtn == 1)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}
		if (use_ssl)
		{
			iter->use_ssl = 1;
		}
		else
		{
			iter->use_ssl = 0;
		}

#if 1
		iter->rtsp_port = cam_get_rtsp_port(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done);

		if (iter->rtsp_port <= 0)
		{
			if (_is_default_pw && iter->use_ssl)
			{
				IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
						__OPENMODE_CAM_STATE_STR_[iter->state],
						__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_PW_CHANGE]);
				iter->state = OPENMODE_CAM_STATE_PW_CHANGE;
				strcpy(iter->u, "ADMIN");
				strcpy(iter->u_done, "ADMIN");
			}
			else
			{
				IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
				iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			}
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}

		memset(&netinfo, 0x00, sizeof(NFOpenmodeSetupNetwork));
		rtn = cam_get_network(htonl(iter->ipaddr), iter->http_port, iter->u_done, iter->p_done, &netinfo);
		if (rtn <= 0)
		{
			IPCAM_DBG(WARN, "connection failed(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}
		{
			unsigned int h,m,_gotip;
			h = _host_ip;
			m = _host_subnet;
			_gotip = htonl(netinfo.ipaddr);

			IPCAM_DBG(MINOR, "h(%08x) m(%08x) _nowip(%08x) _gotip(%08x)\n", h,m,iter->ipaddr,_gotip);

			if ((m != 0) && ((h&m) == (iter->ipaddr&m)))
			{
				iter->ipaddr = netinfo.ipaddr;
			}
		}
		iter->mask = netinfo.mask;
		iter->gw = netinfo.gw;
		iter->dns1 = netinfo.dns1;
		iter->dns2 = netinfo.dns2;
#endif

		IPCAM_DBG(MINOR, "index(%d)\n", iter->index);
		IPCAM_DBG(MINOR, "name(%s)\n", info.name);
		strncpy(iter->model, info.name, 64);
		IPCAM_DBG(MINOR, "swver(%s)\n", info.swver);
		strncpy(iter->firmware_version, info.swver, 64);
		IPCAM_DBG(MINOR, "swvers2(%s)\n", info.swver2);
		strncpy(iter->firmware_version2, info.swver2, 64);
		IPCAM_DBG(MINOR, "vendor(%s)\n", info.vendor);
		strncpy(iter->vendor, info.vendor, 64);
		IPCAM_DBG(MINOR, "stdver(%s)\n", info.stdver);
		strncpy(iter->model_std, info.stdver, 64);
		IPCAM_DBG(MINOR, "sdkver(%s)\n", info.sdkver);
		strncpy(iter->sdkver, info.sdkver, 64);

		if(info.zoom_module_name != NULL)
			strncpy(iter->zoom_module_name, info.zoom_module_name, 63);
		if(info.zoom_module_fwver != NULL)
			strncpy(iter->zoom_module_fwver, info.zoom_module_fwver, 63);
		if(info.capa_version != NULL)
			strncpy(iter->capa_version, info.capa_version, 15);
		
		IPCAM_DBG(MINOR, "mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
				info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]
		);
		IPCAM_DBG(MINOR, "ip(%d.%d.%d.%d)\n",
				(iter->ipaddr&0xff000000)>>24,
				(iter->ipaddr&0xff0000)>>16,
				(iter->ipaddr&0xff00)>>8,
				(iter->ipaddr&0xff));
		IPCAM_DBG(MINOR, "mask(%d.%d.%d.%d)\n",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));
		IPCAM_DBG(MINOR, "gw(%d.%d.%d.%d)\n",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));
		IPCAM_DBG(MINOR, "dns1(%d.%d.%d.%d)\n",
				(iter->dns1&0xff000000)>>24,
				(iter->dns1&0xff0000)>>16,
				(iter->dns1&0xff00)>>8,
				(iter->dns1&0xff));
		IPCAM_DBG(MINOR, "dns2(%d.%d.%d.%d)\n",
				(iter->dns2&0xff000000)>>24,
				(iter->dns2&0xff0000)>>16,
				(iter->dns2&0xff00)>>8,
				(iter->dns2&0xff));

		if (memcmp(iter->macaddr, _cmp_ptn, 6) == 0)
		{
			memcpy(iter->macaddr, info.mac, 6);
		}
		strncpy(iter->u, iter->u_done, 64);
		strncpy(iter->p, iter->p_done, 64);


		//if (_is_default_pw && iter->use_ssl)
		if(0)
		{
			IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
					__OPENMODE_CAM_STATE_STR_[iter->state],
					__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_PW_CHANGE]);
			iter->state = OPENMODE_CAM_STATE_PW_CHANGE;
			strcpy(iter->u, "ADMIN");
			strcpy(iter->u_done, "ADMIN");
		}
		else
		{
			IPCAM_DBG(MINOR, "openmode list cam state tran(%s->%s)\n",
					__OPENMODE_CAM_STATE_STR_[iter->state],
					__OPENMODE_CAM_STATE_STR_[OPENMODE_CAM_STATE_DEV_INFO]);
			iter->state = OPENMODE_CAM_STATE_DEV_INFO;
			if (_last_preview_id == iter->index)
			{
				nf_openmode_set_preview(iter->index, 0);
			}
		}

		//iter = iter->next;
		if (openmode_state == OPENMODE_STATE_RUNNING)
		{
			nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
		}
		else
		{
#if BLOCKING_SCAN
			nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
			nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
		}
		/*
		rtn = search_ipcam_model(iter);
		if(rtn == IPX_SEARCH_FOUND_NOT_SUPPORTED) {

			float p_version = 0;
			p_version = atof(iter->capa_version);
				
			if(p_version == 0)
			{
				printf("", __FUNCTION__, __LINE__, p_version);
				iter->state = OPENMODE_CAM_STATE_UNSUPPORTED;
				continue;
			}
		}
		*/
		continue;

onvif_dev:
		{
			guchar _zero[6];
			memset(_zero, 0x00, 6);
			if (memcmp(iter->macaddr, _zero, 6) != 0)
			{
				if (_is_itx_mac_range(iter->macaddr) == OPENMODE_RTN_OK)
				{
					IPCAM_DBG(WARN, "ITX cam index(%d) ch(%d)\n", iter->index, iter->ch);
					iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
					if (openmode_state == OPENMODE_STATE_RUNNING)
					{
						nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
					}
					else
					{
#if BLOCKING_SCAN
						nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
						nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
					}
					continue;
				}
			}
		}
		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_dev_info_raw(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 1,
					iter->u, iter->p, &info
			);
		}
		else
		{
			rtn = nf_onvif_get_dev_info_raw(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 0,
					iter->u, iter->p, &info
			);
		}

		if (rtn != 0)
		{
			if (iter->u[0] != '\0')
			{
				if (nf_ipcam_is_vendor_s1())
				{
					rtn = nf_onvif_get_dev_info_raw(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 1,
							iter->u, iter->p, &info
					);
				}
				else
				{
					rtn = nf_onvif_get_dev_info_raw(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 0,
							iter->u, iter->p, &info
					);
				}

				if (rtn != 0)
				{
					if (nf_ipcam_is_vendor_s1())
					{
						rtn = nf_onvif_get_dev_info_raw(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 1,
								iter->u, iter->p, &info
						);
					}
					else
					{
						rtn = nf_onvif_get_dev_info_raw(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 0,
								iter->u, iter->p, &info
						);
					}
				}
			}
		}

		if (rtn == 28) // connection fail
		{
			IPCAM_DBG(WARN, "connection fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}
		else if (rtn != 0)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}

		iter->auth = NF_ONVIF_AUTH_NONE;
		memset(iter->u_done, 0x00, 64);
		memset(iter->p_done, 0x00, 64);
		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_net_info(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 1,
					iter->u, iter->p, info.mac, &netinfo
			);
		}
		else
		{
			rtn = nf_onvif_get_net_info(
					htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_NONE, 0,
					iter->u, iter->p, info.mac, &netinfo
			);
		}

		if (rtn != 0)
		{
			if (iter->u[0] != '\0')
			{
				iter->auth = NF_ONVIF_AUTH_TEXT;
				strncpy(iter->u_done, iter->u, 64);
				strncpy(iter->p_done, iter->p, 64);
				if (nf_ipcam_is_vendor_s1())
				{
					rtn = nf_onvif_get_net_info(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 1,
							iter->u, iter->p, info.mac, &netinfo
					);
				}
				else
				{
					rtn = nf_onvif_get_net_info(
							htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_TEXT, 0,
							iter->u, iter->p, info.mac, &netinfo
					);
				}

				if (rtn != 0)
				{
					iter->auth = NF_ONVIF_AUTH_DIGEST;
					strncpy(iter->u_done, iter->u, 64);
					strncpy(iter->p_done, iter->p, 64);
					if (nf_ipcam_is_vendor_s1())
					{
						rtn = nf_onvif_get_net_info(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 1,
								iter->u, iter->p, info.mac, &netinfo
						);
					}
					else
					{
						rtn = nf_onvif_get_net_info(
								htonl(iter->ipaddr), iter->http_port, NF_ONVIF_AUTH_DIGEST, 0,
								iter->u, iter->p, info.mac, &netinfo
						);
					}
				}
			}
		}

		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "login fail(%d)\n", iter->index);
			iter->auth = NF_ONVIF_AUTH_NONE;
			memset(iter->u_done, 0x00, 64);
			memset(iter->p_done, 0x00, 64);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}

		strncpy(iter->model, info.name, 64);
		strncpy(iter->firmware_version, info.swver, 64);
		strncpy(iter->firmware_version2, info.swver2, 64);
		strncpy(iter->vendor, info.vendor, 64);
		strncpy(iter->model_std, info.stdver, 64);
		strncpy(iter->sdkver, info.sdkver, 64);

		if(info.zoom_module_name != NULL)
			strncpy(iter->zoom_module_name, info.zoom_module_name, 63);
		if(info.zoom_module_fwver != NULL)
			strncpy(iter->zoom_module_fwver, info.zoom_module_fwver, 63);

		memcpy(iter->macaddr, info.mac, 6);
		iter->is_dhcp = netinfo.is_dhcp;

		if (netinfo.mask == 24)
		{
			iter->mask = ntohl(inet_addr("255.255.255.0"));
		}
		else if (netinfo.mask == 16)
		{
			iter->mask = ntohl(inet_addr("255.255.0.0"));
		}
		else if (netinfo.mask == 8)
		{
			iter->mask = ntohl(inet_addr("255.0.0.0"));
		}
		snprintf(iter->maskstr, 16, "%d.%d.%d.%d",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));

		iter->gw = netinfo.gw;
		snprintf(iter->gwstr, 16, "%d.%d.%d.%d",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));

		if (nf_ipcam_is_vendor_s1())
		{
			rtn = nf_onvif_get_media_xaddr(
					htonl(iter->ipaddr), iter->http_port, iter->tail,
					iter->auth, 1, iter->u_done, iter->p_done, iter->media_xaddr);
		}
		else
		{
			rtn = nf_onvif_get_media_xaddr(
					htonl(iter->ipaddr), iter->http_port, iter->tail,
					iter->auth, 0, iter->u_done, iter->p_done, iter->media_xaddr);
		}
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "media get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}

		{
			OPENMODE_RTN_ENUM _rtn;
			gchar _tail[256];
			_rtn = _get_address_tail(iter->media_xaddr, _tail);
			if (_rtn == OPENMODE_RTN_OK)
			{
				if (nf_ipcam_is_vendor_s1())
				{
					snprintf(iter->media_xaddr, 256, "https://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						_tail);
					snprintf(device_xaddr, 256, "https://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						"onvif/device_service");
				}
				else
				{
					snprintf(iter->media_xaddr, 256, "http://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						_tail);
					snprintf(device_xaddr, 256, "http://%d.%d.%d.%d:%d/%s",
						(iter->ipaddr&0xff000000)>>24,
						(iter->ipaddr&0xff0000)>>16,
						(iter->ipaddr&0xff00)>>8,
						(iter->ipaddr&0xff),
						iter->http_port,
						"onvif/device_service");
				}
			}
		}

		auth_info.auth_method = iter->auth;
		auth_info.username = iter->u;
		auth_info.password = iter->p;
		auth_info.endpoint = device_xaddr;
		rtn = nf_onvif_get_preview_profile(
				iter->media_xaddr, &auth_info, iter->token);
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "profile get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}

		rtn = nf_onvif_get_preview_uri(iter->media_xaddr, device_xaddr, iter->auth, iter->u, iter->p, iter->token, iter->preview_rtsp);
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "uri get fail(%d)\n", iter->index);
			iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
			if (openmode_state == OPENMODE_STATE_RUNNING)
			{
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
			}
			else
			{
#if BLOCKING_SCAN
				nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
				nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
			}
			continue;
		}
		{
			char *p = iter->preview_rtsp;
			char *e = NULL;
			char _temp[8];

			memset(_temp, 0x00, 8);
			if (nf_ipcam_is_vendor_s1())
			{
				p = p + strlen("https://");
			}
			else
			{
				p = p + strlen("http://");
			}
			p = strstr(p, ":");
			if (p == NULL)
			{
				iter->rtsp_port = 554;
			}
			else
			{
				p++;
				e = strstr(p, "/");
				if (e == NULL)
				{
					iter->rtsp_port = 554;
				}
				else
				{
					memcpy(_temp, p, (e-p));
					iter->rtsp_port = atoi(_temp);
				}
			}
		}
		{
			OPENMODE_RTN_ENUM _rtn;
			gchar _tail[256];
			_rtn = _get_address_tail(iter->preview_rtsp, _tail);
			if (_rtn == OPENMODE_RTN_OK)
			{
				snprintf(iter->preview_rtsp, 256, "rtsp://%d.%d.%d.%d:%d/%s",
					(iter->ipaddr&0xff000000)>>24,
					(iter->ipaddr&0xff0000)>>16,
					(iter->ipaddr&0xff00)>>8,
					(iter->ipaddr&0xff),
					iter->rtsp_port,
					_tail);
			}
		}


		IPCAM_DBG(MINOR, "index(%d)\n", iter->index);
		IPCAM_DBG(MINOR, "name(%s)\n", info.name);
		IPCAM_DBG(MINOR, "swver(%s)\n", info.swver);
		IPCAM_DBG(MINOR, "swvers2(%s)\n", info.swver2);
		IPCAM_DBG(MINOR, "vendor(%s)\n", info.vendor);
		IPCAM_DBG(MINOR, "stdver(%s)\n", info.stdver);
		IPCAM_DBG(MINOR, "sdkver(%s)\n", info.sdkver);
		IPCAM_DBG(MINOR, "mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
				info.mac[0], info.mac[1], info.mac[2], info.mac[3], info.mac[4], info.mac[5]
		);
		IPCAM_DBG(MINOR, "is_dhcp(%d)\n", netinfo.is_dhcp);
		IPCAM_DBG(MINOR, "prefix(%d)\n", netinfo.mask);
		IPCAM_DBG(MINOR, "mask(%d.%d.%d.%d)\n",
				(iter->mask&0xff000000)>>24,
				(iter->mask&0xff0000)>>16,
				(iter->mask&0xff00)>>8,
				(iter->mask&0xff));
		IPCAM_DBG(MINOR, "gw(%d.%d.%d.%d)\n",
				(iter->gw&0xff000000)>>24,
				(iter->gw&0xff0000)>>16,
				(iter->gw&0xff00)>>8,
				(iter->gw&0xff));
		IPCAM_DBG(MINOR, "media(%s)\n", iter->media_xaddr);
		IPCAM_DBG(MINOR, "profile(%s)\n", iter->token);
		IPCAM_DBG(MINOR, "uri(%s)\n", iter->preview_rtsp);
		IPCAM_DBG(MINOR, "rtsp_port(%d)\n", iter->rtsp_port);

		if (openmode_state == OPENMODE_STATE_RUNNING)
		{
			nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
		}
		else
		{
#if BLOCKING_SCAN
			nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
			nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
		}
		iter->state = OPENMODE_CAM_STATE_DEV_INFO_ONVIF;
		if (_last_preview_id == iter->index)
		{
			nf_openmode_set_preview(iter->index, 0);
		}
		continue;

virtual_dev:
		{
			int rtn = 0;
			int audio_flag = 0;
			rtn = vcam_get_state_info_from_describe_test(iter->u, iter->p, iter->vcam_rtsp_addr[0], iter->ipaddr, iter->rtsp_port, &audio_flag);

			if(rtn == -2)
			{
				iter->ipaddr = _host_to_ip(iter->hostname);
				IPCAM_DBG(ERROR, "connection fail(%s)\n", __FUNCTION__);
				iter->state = OPENMODE_CAM_STATE_CONN_FAIL;
				if (openmode_state == OPENMODE_STATE_RUNNING)
				{
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
				}
				else
				{
#if BLOCKING_SCAN
					nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
				}
				continue;
			}
			else if(rtn == -1)
			{
				IPCAM_DBG(ERROR, "login fail(%s)\n", __FUNCTION__);
				iter->state = OPENMODE_CAM_STATE_LOGIN_FAIL;
				if (openmode_state == OPENMODE_STATE_RUNNING)
				{
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
				}
				else
				{
#if BLOCKING_SCAN
					nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
				}
				continue;
			}

			if(rtn == 0)
			{
				iter->state = OPENMODE_CAM_STATE_VIRTUAL_CAMERA;
				//iter->state = OPENMODE_CAM_STATE_DEV_INFO_ONVIF;
				//strcpy(iter->model, "VCAM");
				//strcpy(iter->model_std, "VCAM");
				
				iter->vcam_audio_flag = audio_flag;

				if (openmode_state == OPENMODE_STATE_RUNNING)
				{
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
				}
				else
				{
#if BLOCKING_SCAN
					nf_notify_fire_params("ipcam_slist", 1, 0, 0, 0);
#else
					nf_notify_fire_params("ipcam_slist", 0, 0, 0, 0);
#endif
				}

				if (_last_preview_id == iter->index)
				{
					nf_openmode_set_preview(iter->index, 0);
				}

			}
			
		}

	}
}

static NFOpenmodeCamInfo* _list_find_entry_by_mac(NFOpenmodeDeviceList* list, unsigned char *mac)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	IPCAM_DBG(MAJOR, "start(%02x-%02x-%02x-%02x-%02x-%02x)\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
	);
	iter = list->head;
	if (iter == NULL) { return NULL; }
	for (iter=list->head; iter!=NULL; iter=iter->next)
	{
		if (iter == NULL) { return NULL; }
		if (memcmp(mac, iter->macaddr, 6) == 0)
		{
			IPCAM_DBG(MAJOR, "end - found\n");
			return iter;
		}
		//iter = iter->next;
	}
	IPCAM_DBG(MAJOR, "end - not found\n");
	return NULL;
}

static NFOpenmodeCamInfo* _list_find_entry_by_ip(NFOpenmodeDeviceList* list, unsigned int ip)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	IPCAM_DBG(MAJOR, "start(%d.%d.%d.%d)\n",
			(ip&0xff000000)>>24,
			(ip&0xff0000)>>16,
			(ip&0xff00)>>8,
			(ip&0xff)
	);
	iter = list->head;
	if (iter == NULL) { return NULL; }
	for (iter=list->head; iter!=NULL; iter=iter->next)
	{
		if (iter == NULL) { return NULL; }
		if (iter->ipaddr == ip)
		{
			IPCAM_DBG(MAJOR, "end - found\n");
			return iter;
		}
		//iter = iter->next;
	}
	IPCAM_DBG(MAJOR, "end - not found\n");
	return NULL;
}

static NFOpenmodeCamInfo* _list_find_entry_by_ip_port(NFOpenmodeDeviceList* list, unsigned int ip, unsigned short port)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	IPCAM_DBG(MAJOR, "start(%d.%d.%d.%d)\n",
			(ip&0xff000000)>>24,
			(ip&0xff0000)>>16,
			(ip&0xff00)>>8,
			(ip&0xff)
	);
	iter = list->head;
	if (iter == NULL) { return NULL; }
	for (iter=list->head; iter!=NULL; iter=iter->next)
	{
		if (iter == NULL) { return NULL; }
		if (iter->ipaddr == ip && iter->http_port == port)
		{
			IPCAM_DBG(MAJOR, "end - found\n");
			return iter;
		}
		//iter = iter->next;
	}
	IPCAM_DBG(MAJOR, "end - not found\n");
	return NULL;
}

static NFOpenmodeCamInfo* _list_find_entry_by_index(NFOpenmodeDeviceList* list, int index)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	iter = list->head;
	if (iter == NULL) { return NULL; }
	for (iter=list->head; iter!=NULL; iter=iter->next)
	{
		if (iter == NULL) { return NULL; }
		if (iter->index == index)
		{
			return iter;
		}
		//iter = iter->next;
	}

	return NULL;
}

static NFOpenmodeCamInfo* _list_find_entry_by_ch(NFOpenmodeDeviceList* list, int ch)
{
	gint i;
	NFOpenmodeCamInfo *iter;

	if (ch < 0 || ch >= _max_ch)
	{
		return NULL;
	}

	iter = list->head;
	if (iter == NULL) { return NULL; }
	for (iter=list->head; iter!=NULL; iter=iter->next)
	{
		if (iter == NULL) { return NULL; }
		if (iter->ch == ch)
		{
			return iter;
		}
		//iter = iter->next;
	}

	return NULL;
}

static int _get_available_ch(void)
{
	gint i;
	gint rtn = (-1);
	NFOpenmodeCamInfo *iter;

#if 0
	IPCAM_DBG(MAJOR, "start\n");
	for (i=0; i<_max_ch; i++)
	{
		iter = _openmode_detection_list.head;
		if (iter == NULL)
		{
			rtn = 0;
			break;
		}
		while (iter->ch != i)
		{
			iter = iter->next;
			if (iter == NULL) { break; }
		}
		if (iter == NULL)
		{
			rtn = i;
			break;
		}
		else
		{
			rtn = (-1);
		}
	}
	IPCAM_DBG(MAJOR, "end\n");
	return rtn;
#endif

    return -1;
}

static unsigned int _get_ipaddr_from_hostname(gchar *hostname)
{
	unsigned int ipaddr = inet_addr(hostname);
	char ip_str[16];
	OPENMODE_RTN_ENUM rtn;


	if (hostname[0] == '\0')
	{
		return (0);
	}

	if (ipaddr == 0 || ipaddr == 0xffffffff)
	{
		memset(ip_str, 0x00, 16);
		rtn = _dns_lookup(hostname, ip_str);
		if (rtn == OPENMODE_RTN_OK)
		{
			ipaddr = inet_addr(ip_str);
		}
	}

	if (ipaddr == 0xffffffff)
	{
		ipaddr = 0;
	}

	return ipaddr;
}

static OPENMODE_RTN_ENUM _openmode_init_live_list(void)
{
	gint i;
	NFOpenmodeCamInfo *entry = NULL;
    int ret;

    if(_openmode_live_list.entry_cnt > 0){
        printf("[%s:%d] error _openmode_live_list.entry_cnt[%d]\n", __func__, __LINE__, _openmode_live_list.entry_cnt);
        return OPENMODE_RTN_FAIL;
    }

	IPCAM_DBG(MAJOR, "start\n");
	for (i=0; i< _max_ch; i++)
	{
        entry = &_openmode_live_data[i];
		_alloc_cnt++;

		if (entry == NULL)
		{
			IPCAM_DBG(WARN, "Live list entry allocation failure\n");
			return OPENMODE_RTN_FAIL;
		}

		memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));

		entry->index = i;
		entry->ch = i;
		entry->ipaddr = ntohl(_get_ipaddr_from_hostname(_db_host[i]));
		strcpy(entry->hostname, _db_host[i]);
		entry->http_port = _db_http_port[i];
		entry->state = OPENMODE_CAM_STATE_INIT;

		strncpy(entry->u, _db_u[i], 64);
		strncpy(entry->p, _db_p[i], 64);


		entry->rtsp_port = _db_rtsp_port[i];

		strcpy(entry->vcam_rtsp_addr[0], _db_rtsp_addr_main[i]);
		strcpy(entry->vcam_rtsp_addr[1], _db_rtsp_addr_second[i]);

		entry->virtual_camera = _db_vcam[i];
		entry->vcam_cnt= _db_vcam_cnt[i];

		strcpy(entry->model, _db_model[i]);

		strncpy(entry->eth_dev, _db_ethernet[i], 4);

		_openmode_list_add(&_openmode_live_list, entry);
	}
	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_init_ch_list(void)
{
	gint i;
	NFOpenmodeCamInfo *entry = NULL;

    if(_openmode_ch_list.entry_cnt > 0){
        printf("[%s:%d] error _openmode_ch_list.entry_cnt[%d]\n", __func__, __LINE__, _openmode_ch_list.entry_cnt);
        return OPENMODE_RTN_FAIL;
    }

	IPCAM_DBG(MAJOR, "start\n");
	for (i=0; i< _max_ch; i++)
	{
		entry = (NFOpenmodeCamInfo*) malloc(sizeof(NFOpenmodeCamInfo));
		_alloc_cnt++;

		if (entry == NULL)
		{
			IPCAM_DBG(WARN, "Channel list entry allocation failure\n");
			return OPENMODE_RTN_FAIL;
		}

		memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));

		entry->index = i;
		entry->ch = i;
		entry->state = OPENMODE_CAM_STATE_INIT;

		_openmode_list_add(&_openmode_ch_list, entry);
	}
	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_init_live_list_from_ch_list(void)
{
	gint i;
	NFOpenmodeCamInfo *entry = NULL;
	NFOpenmodeCamInfo *ch_entry = NULL;

	IPCAM_DBG(MAJOR, "start\n");
	for (i=0; i< _max_ch; i++)
	{
        entry = &_openmode_live_data[i];
		_alloc_cnt++;

		memset(entry, 0x00, sizeof(NFOpenmodeCamInfo));

#if 0
		ch_entry = _list_find_entry_by_ch(&_openmode_detection_list, i);
		if (ch_entry != NULL && ((ch_entry->state == OPENMODE_CAM_STATE_DEV_INFO)||(ch_entry->state == OPENMODE_CAM_STATE_DEV_INFO_ONVIF)))
		{
			memcpy(entry, ch_entry, sizeof(NFOpenmodeCamInfo));
			entry->prev = NULL;
			entry->next = NULL;
		}
		else
#endif
		{
			entry->state = OPENMODE_CAM_STATE_INIT;
			entry->ipaddr = ntohl(_get_ipaddr_from_hostname(_db_host[i]));
			strncpy(entry->hostname, _db_host[i] ,256);
			entry->http_port = _db_http_port[i];

			strncpy(entry->u, _db_u[i], 64);
			strncpy(entry->p, _db_p[i], 64);

			entry->rtsp_port = _db_rtsp_port[i];

			strncpy(entry->vcam_rtsp_addr[0], _db_rtsp_addr_main[i], 256);
			strncpy(entry->vcam_rtsp_addr[1], _db_rtsp_addr_second[i], 256);

			entry->virtual_camera = _db_vcam[i];
			entry->vcam_cnt = _db_vcam_cnt[i];

			strncpy(entry->model, _db_model[i], 64);

			strncpy(entry->eth_dev , _db_ethernet[i], 4);

		}

		entry->index = i;
		entry->ch = i;
		_openmode_list_add(&_openmode_live_list, entry);
	}
	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_finalize_live_list(void)
{
	gint i=0;
	NFOpenmodeCamInfo *iter;
	NFOpenmodeCamInfo *temp;
    int ch;


	IPCAM_DBG(MAJOR, "start\n");

	_live_stop_requested = 1;
	while (_running_live_detail > 0)
	{
		usleep(100*1000);
	}
	_live_stop_requested = 0;
	_openmode_list_mtx_lock();
	_openmode_live_list.entry_cnt = 0;
	iter = _openmode_live_list.head;
	for (i=0; i<_max_ch; i++)
	{
		if (iter == NULL )
		{
			IPCAM_DBG(WARN, "end - Live list NULL entry\n");
			_openmode_list_mtx_unlock();
			return OPENMODE_RTN_FAIL;
		}
        ch = iter->ch;
        _openmode_live_mtx_lock(ch);
		temp = iter->next;
		memset(iter, 0x00, sizeof(NFOpenmodeCamInfo));
        iter->ch = -1;
		_alloc_cnt--;
		iter = temp;
        _openmode_live_mtx_unlock(ch);
	}
	memset(&_openmode_live_list, 0x00, sizeof(NFOpenmodeDeviceList));
	_openmode_list_mtx_unlock();

	IPCAM_DBG(MAJOR, "end\n");
	return OPENMODE_RTN_OK;
}

static unsigned int ipx_resol_cm2icodec(uint64_t cam_manager_resol);


/*thread pool*/
GThreadPool *openmode_work_pool = NULL;
typedef struct
{
    void (*func)(void *);
    void *data;
}cb_data;

static gpointer openmode_process_cb_func(gpointer data, gpointer user_data)
{
    cb_data *cb = (cb_data *)data;
    if(cb == NULL) return NULL;
    if(cb->func == NULL) return NULL;

    cb->func(cb->data);
    free(cb);

    return NULL;
}

static GThreadPool *_get_openmode_work_pool()
{
    if(openmode_work_pool == NULL){
        openmode_work_pool = g_thread_pool_new (
                (GFunc) openmode_process_cb_func,
                NULL,
                4,  //max threads
                TRUE,
                NULL);
    }
    return openmode_work_pool;
}

static void openmode_cb_work_push(void *func, void *data)
{
    cb_data *cb;
    GError *err = NULL;

    cb = (cb_data *)malloc(sizeof(cb_data));
    if(cb == NULL){
        printf("[%s:%d] error! memory alloc failed func[%p] data[%p]\n", __func__, __LINE__, func, data);
        return;
    }
    cb->func = func;
    cb->data = data;

    g_thread_pool_push (_get_openmode_work_pool(), cb, &err);

    if(err){
        printf("[%s:%d] critical error code[%d] message[%s]\n", __func__, __LINE__, err->code, err->message);
        if(openmode_work_pool){
            g_thread_pool_free(openmode_work_pool, TRUE, FALSE);
            openmode_work_pool = NULL;
        }
        g_clear_error(&err);
        if(cb) free(cb);
    }
}


typedef struct _mrtp_open_data
{
    int ch;
    NFOpenmodeCamInfo *info;
}mrtp_open_data;

static OPENMODE_RTN_ENUM _openmode_start_stream_thread_func(mrtp_open_data *data);
static OPENMODE_RTN_ENUM _openmode_start_stream(int ch, NFOpenmodeCamInfo *info)
{
    mrtp_open_data *data = NULL;
    if(ch < 0 || ch >= AVAILABLE_MAX_CH){
        printf("[%s:%d] error ch[%d]\n", __func__, __LINE__, ch);
        return OPENMODE_RTN_FAIL;
    }

    if(info == NULL){
        printf("[%s:%d] ch[%d] info[%p]\n", __func__, __LINE__, ch, info);
        return OPENMODE_RTN_FAIL;
    }

    data = (mrtp_open_data *)malloc(sizeof(mrtp_open_data));
    if(data == NULL){
        printf("[%s:%d] malloc failed ch[%d] data[%p]\n", __func__, __LINE__, ch, data);
        return OPENMODE_RTN_FAIL;
    }
    
    data->ch = ch;
    data->info = info;

    openmode_cb_work_push(_openmode_start_stream_thread_func, data);
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _openmode_start_stream_thread_func(mrtp_open_data *data)
{
    NFOpenmodeCamInfo *camInfo;
	NMFMrtpPipeChannel info;
	mtable *runtime = get_runtime();
	gchar key[128];
    int ch;
    int ret;

    if(data == NULL){
        printf("[%s:%d] error data[%p]\n", __func__, __LINE__, data);
        return OPENMODE_RTN_FAIL; 
    }

    ch = data->ch;
    camInfo = data->info;
    free(data);
    _openmode_live_mtx_lock(ch);

    if(camInfo == NULL){
        printf("[%s:%d] error ch[%d] caminfo[%p]\n", __func__, __LINE__, ch, camInfo);
        _openmode_live_mtx_unlock(ch);
        return OPENMODE_RTN_FAIL;
    }

    if(ch != camInfo->ch){
        printf("[%s:%d] error ch[%d] caminfo->ch[%d]\n", __func__, __LINE__, ch, camInfo->ch);
        _openmode_live_mtx_unlock(ch);
        return OPENMODE_RTN_FAIL;
    }

	memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

    if(camInfo->state == OPENMODE_CAM_STATE_STREAM_OPEN_REQ){

        info.ch_num = ch;
        info.model_code = runtime[ch].sys.model_code;
        info.username = &runtime[ch].username;
        info.password = &runtime[ch].password;
        info.video_cnt = runtime[ch].video.stream_cnt;
        info.video[0].resolution = ipx_resol_cm2icodec(runtime[ch].video.resolution.resolution[NF_IPCAM_STREAM_1ST]);
        info.video[0].ip_addr = runtime[ch].sys.ipaddr;
        info.video[0].rtsp_port = runtime[ch].sys.rtsp_port[0];
        info.video[0].rtsp_addr = runtime[ch].sys.rtsp_url[0];
        //info.video[0].codec_type = (runtime[ch].video.vcodec[0] == NF_IPCAM_VCODEC_H265) ? 2:1;

        if (info.video_cnt > 1)
        {
            info.video[1].resolution = ipx_resol_cm2icodec(runtime[ch].video.resolution.resolution[NF_IPCAM_STREAM_2ND]);
            info.video[1].ip_addr = runtime[ch].sys.ipaddr;
            info.video[1].rtsp_port = runtime[ch].sys.rtsp_port[1];
            info.video[1].rtsp_addr = runtime[ch].sys.rtsp_url[1];
            //info.video[1].codec_type = (runtime[ch].video.vcodec[1] == NF_IPCAM_VCODEC_H265) ? 2:1;
        }
        if (info.video_cnt > 2)
        {
            /* FIXME */
            info.video[2].resolution = RES_640x360;
            info.video[2].ip_addr = runtime[ch].sys.ipaddr;
            info.video[2].rtsp_port = runtime[ch].sys.rtsp_port[2];
            info.video[2].rtsp_addr = runtime[ch].sys.rtsp_url[2];
            //info.video[2].codec_type = (runtime[ch].video.vcodec[2] == NF_IPCAM_VCODEC_H265) ? 2:1;
        }

        // Xiongmai 카메라 예외처리
        if(strcmp(runtime[ch].sys.vendor, "H264") == 0)
        {
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
        //{
        //	info.metadata_on = 1;
        //}

        snprintf(key, 128, "cam.logininfo.L%d.vcam", ch);
        info.metadata_on = (nf_sysdb_get_uint(key) == 0)? 1:0; 

        nmf_mrtp_pipe_set_dev_mac(h_iplive, ch, &runtime[ch].sys.macaddr[0]);
        nmf_mrtp_pipe_open_ch(h_iplive, &info);

        camInfo->state = OPENMODE_CAM_STATE_OK;
        ret = OPENMODE_RTN_OK;
    }else{
        printf("[%s:%d] err ch[%d] camInfo->state[%d] -> OPENMODE_CAM_STATE_DISCOVERED\n", __func__, __LINE__, ch, camInfo->state);
        camInfo->state = OPENMODE_CAM_STATE_DISCOVERED;
        ret = _openmode_stop_stream(ch);
    }

    _openmode_live_mtx_unlock(ch);

    return ret;
}

static OPENMODE_RTN_ENUM _openmode_stop_stream(int ch)
{
	nmf_mrtp_pipe_close_ch(h_iplive, ch);
	return OPENMODE_RTN_OK;
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
		
		// add hallway res
		case NF_RES_360x640:	{ rtn = NF_IPCAM_RES_640x360;	break;	}
		case NF_RES_480x640:	{ rtn = NF_IPCAM_RES_640x480;	break;	}
		case NF_RES_480x704:	{ rtn = NF_IPCAM_RES_704x480;	break;	}
		case NF_RES_576x704:	{ rtn = NF_IPCAM_RES_704x576;	break;	}
		case NF_RES_720x1280:	{ rtn = NF_IPCAM_RES_1280x720;	break;	}
		case NF_RES_768x1024:	{ rtn = NF_IPCAM_RES_1024x768;	break;	}
		case NF_RES_1024x1280:	{ rtn = NF_IPCAM_RES_1280x1024;	break;	}
		case NF_RES_1080x1920:	{ rtn = NF_IPCAM_RES_1920x1080;	break;	}
		case NF_RES_1536x2048:	{ rtn = NF_IPCAM_RES_2048x1536;	break;	}
		case NF_RES_1296x2304:	{ rtn = NF_IPCAM_RES_2304x1296;	break;	}
		case NF_RES_1520x2592:	{ rtn = NF_IPCAM_RES_2592x1520;	break;	}
		case NF_RES_1944x2592:	{ rtn = NF_IPCAM_RES_2592x1944;	break;	}
		case NF_RES_2160x3840:	{ rtn = NF_IPCAM_RES_3840x2160;	break;	}
		default:             { rtn = 0;                        break; }
	}

	return rtn;
}

static void _cmd_callback_func(gint op, gint cbch, gint stream, gint en, gpointer user_data)
{
	int i;
	int ch = cbch;
	unsigned int vloss_status;
	mtable *runtime = get_runtime();
	NFIPCamPortStatus port_status;
	GAsyncQueue *vloss_queue = get_vloss_queue();

	if (op != 30)
	{
		IPCAM_DBG(MAJOR, "op(%d) ch(%d) stream(%d) en(%d)\n", op, ch, stream, en);
	}

	if (op == 0)	// op == 0: open channel
	{
		//if (_discovery_running) { return; }
		if (en == NF_RTSP_ERR_NO_ERR)
		{
			IPX_PND_EVENT *evt = NULL;
			if (_last_preview_id < 0)
			{
				evt = ipx_ipcam_new_event(IPCAM_VLOSS_ADD_NEW_CH);
				evt->port = ch;
				g_async_queue_push(vloss_queue, evt);
			}
			
			runtime[ch].state = MGMT_STATE_LINKED|MGMT_STATE_READY|MGMT_STATE_CONFIGURED|MGMT_STATE_USING;
			nf_pnd_evt_notify_fire(ch, PND_TYPE_VIDEO_START, __LINE__, __FILE__);

			memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
			port_status.status = 0;
			port_status.device_class = NF_IPCAM_DEVICE_NETWORK_CAMERA;
			snprintf(port_status.vendor, 64, runtime[ch].sys.vendor);
			snprintf(port_status.model, 64, runtime[ch].sys.model);
			memset(&port_status.detail, 0x00, 256);
			nf_ipcam_set_port_status(ch, &port_status, NULL);
			nf_ipcam_set_pnd_osd_status(ch, PND_OSD_NONE);
		}
		else 
		{
			nf_pnd_evt_notify_fire(ch, PND_TYPE_CONFIG_FAIL, __LINE__, __FILE__);
			nf_ipcam_set_pnd_osd_status(ch, PND_OSD_STREAM_FAIL);
		}
	}
	else if (op == 1)	// op == 1: close channel
	{
		IPCAM_DBG(MAJOR, "close (CH%d.%d: %s)\n", ch, stream, RTSP_ERR_STR[en]);
		g_message("###yanggungg : %s, %d, ch : %d stream : %d en : %d", __func__, __LINE__, ch, stream, en);

		if (en == NF_RTSP_ERR_CLOSE_INTERNAL)
		{
			IPCAM_DBG(WARN, "CLOSE requested from mrtpsrc CH(%d)\n", ch);
			// FIXME fw_upgrade special recipe
			if(!nf_ipcam_is_cam_upgrade())
			{
				g_message("###yanggungg : %s, %d, ch : %d", __func__, __LINE__, ch);
				nmf_mrtp_pipe_close_ch(h_iplive, ch);
			}
			//nmf_mrtp_pipe_close_ch(h_iplive, ch);
			_init_stay[ch] = 10;
			IPCAM_DBG(WARN, "CLOSE - from mrtpsrc CH(%d)\n", ch);
			return;
		}
#if 0
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
			evt->port = ch;
			g_async_queue_push(vloss_queue, evt);

			runtime[ch].state = MGMT_STATE_UNLINKED;
			IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
					ch, runtime[ch].state);

#if 0
			for (i=0; i < NF_IPCAM_TYPE_MAX; i++)
			{
				_release_resource(0, NULL, &runtime[ch].sys.ssl[i], &runtime[ch].sys.ctx[i]);
			}
#endif
			return;
		}
#endif
		g_message("###yanggungg : %s, %d, ch : %d", __func__, __LINE__, ch);
		{
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
			evt->port = ch;
			g_async_queue_push(vloss_queue, evt);
			IPCAM_DBG(MINOR, "CLOSE - vloss remove ch enqueued CH(%d)\n", ch);
		}
		nf_pnd_evt_notify_fire(ch, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
		IPCAM_DBG(MINOR, "CLOSE - pnd event notified CH(%d)\n", ch);

		memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
		nf_ipcam_set_port_status(ch, &port_status, NULL);
		IPCAM_DBG(MINOR, "CLOSE - port status initialized CH(%d)\n", ch);
		for (i=0; i < NF_IPCAM_TYPE_MAX; i++)
		{
			nf_ipcam_setup_clear(ch, i);
			//IPCAM_DBG(MINOR, "CLOSE - setup cleared CH(%d) type(%s)\n", ch, ipcam_get_type_str(i));
			pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[i]);
			//IPCAM_DBG(MINOR, "CLOSE - ssl mutex locked CH(%d) type(%s)\n", ch, ipcam_get_type_str(i));
			runtime[ch].sys.ssl_state[i] = IPCAM_SSL_NOT_AVAILABLE;
			//IPCAM_DBG(MINOR, "CLOSE - ssl state set CH(%d) type(%s)\n", ch, ipcam_get_type_str(i));
			_release_resource(0, NULL, &runtime[ch].sys.ssl[i], &runtime[ch].sys.ctx[i]);
			//IPCAM_DBG(MINOR, "CLOSE - ssl released CH(%d) type(%s)\n", ch, ipcam_get_type_str(i));
			pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[i]);
			//IPCAM_DBG(MINOR, "CLOSE - ssl mutex unlocked CH(%d) type(%s)\n", ch, ipcam_get_type_str(i));
		}
		_release_resource(NULL, NULL, &runtime[ch].sys.ssl_g, &runtime[ch].sys.ctx_g);
		IPCAM_DBG(MINOR, "CLOSE - setup attributes initialized CH(%d)\n", ch);

		memset(&runtime[ch], 0x00, sizeof(mtable));
		IPCAM_DBG(MINOR, "CLOSE - management table initialized CH(%d)\n", ch);
		runtime[ch].state = MGMT_STATE_UNLINKED;
		{
			NFOpenmodeCamInfo *iter = _list_find_entry_by_ch(&_openmode_live_list, ch);
			if (iter == NULL)
			{
				g_message("###yanggungg : %s, %d, ch : %d iter is null", __func__, __LINE__, ch);
				IPCAM_DBG(MINOR, "CLOSE - live entry deallocated already CH(%d)\n", ch);
			}
			else
			{
				iter->state = OPENMODE_CAM_STATE_INIT;
				g_message("###yanggungg : %s, %d, ch : %d iter is not null", __func__, __LINE__, ch);
				IPCAM_DBG(MINOR, "CLOSE - live entry initialized CH(%d)\n", ch);
			}
		}
		IPCAM_DBG(MINOR, "CLOSE - done CH(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] socket corrupted(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] ring buffer full(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case A(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case B(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] magic no err case C(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] mem alloc fail(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] cmem alloc fail(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] vlen zero violation(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] abnormal systime(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] timestamp err(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] long lasting packet loss(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] control frame alloc fail(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "[EVENTLOGPUT] control cmem alloc fail(%d)\n", ch);
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
				IPCAM_DBG(MINOR, "rate down(%d)\n", ch);
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
#if 0
	else if (op == 11)
	{
        static *codec_string[3] = {"", "H.264", "H.265"};
        GValue set_value = {0,};
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, codec_string[en]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.vcodec", cbch, stream, &set_value, NULL))
    	{
            printf("[%s:%d] db set error\n", __func__, __LINE__);
    	}

    	g_value_unset(&set_value);
        if(en == 1){    //H.264
            runtime[cbch].video.vcodec[stream] = NF_IPCAM_VCODEC_H264;
        }else
        if(en == 2){    //H.265
            runtime[cbch].video.vcodec[stream] = NF_IPCAM_VCODEC_H265;
        }

        printf("[%s:%d] codec_change ch[%d] stream[%d] runtime_codec[%d] string[%s]\n", __func__, __LINE__, cbch, stream, runtime[cbch].video.vcodec[stream], codec_string[en]);

        nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	}
#endif
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
}

static char _get_hex_from_char(char a)
{
	if (a >= '0' && a <= '9')
	{
		return (a-'0');
	}

	if (a >= 'a' && a <= 'f')
	{
		return ((a-'a')+0xa);
	}

	if (a >= 'A' && a <= 'F')
	{
		return ((a-'A')+0xa);
	}

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
		if (val[j] >= 0) { j++; }
	}

	dst[0] = (val[0]<<4) + val[1];
	dst[1] = (val[2]<<4) + val[3];
	dst[2] = (val[4]<<4) + val[5];
	dst[3] = (val[6]<<4) + val[7];
	dst[4] = (val[8]<<4) + val[9];
	dst[5] = (val[10]<<4) + val[11];
}

int nf_openmode_dns_lookup(char *host, char *ip_str)
{
    unsigned int ipaddr = 0;

	ipaddr = inet_addr(host);
	if (ipaddr == 0xffffffff){
        return _dns_lookup(host, ip_str);
    }else{
        strncpy(ip_str, host, 16);
        return OPENMODE_RTN_OK;
    }
}

static int _host_to_ip(const char *host)
{
    int rc;
    char ip_str[16];
    unsigned int ipaddr = 0;

    
    rc = nf_openmode_dns_lookup(host, ip_str);
    if(rc == OPENMODE_RTN_OK){
        ipaddr = ntohl(inet_addr(ip_str));
    }else{
        ipaddr = 0;
    }

    printf("[%s:%d] host[%s] ip_str[%s] ipaddr[%08x]\n", __func__, __LINE__, host, ip_str, ipaddr);

    return ipaddr;
}

static OPENMODE_RTN_ENUM _dns_lookup(char *host, char *ip_str)
{
	struct addrinfo hints, *res, *p;
	int status;
	struct sockaddr_in *ipv4 = NULL;
	char ipstr[16];

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0)
	{
		return OPENMODE_RTN_FAIL;
	}

	for (p = res; p != NULL; p = p->ai_next)
	{
		void *addr;

		if (p->ai_family == AF_INET)
		{
			ipv4 = (struct sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
		}

		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		IPCAM_DBG(MINOR, "IP Address (%s)\n", ipstr);
	}


	freeaddrinfo(res);
	strncpy(ip_str, ipstr, 16);

	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _get_address_tail(gchar* addr, gchar* tail)
{
	gchar *s, *e;
	const gchar *find_https = "https://";
	const gchar *find_http = "http://";
	const gchar *find_rtsp = "rtsp://";
	gchar *find_str = NULL;

	if (addr == NULL) { return OPENMODE_RTN_FAIL; }
	if (tail == NULL) { return OPENMODE_RTN_FAIL; }

	if (nf_ipcam_is_vendor_s1())
	{
		find_str = find_https;
	}
	else
	{
		find_str = find_http;
	}

	s = strstr(addr, find_str);
	if (s == NULL)
	{
		find_str = find_rtsp;
		s = strstr(addr, find_str);
	}
	if (s == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}
	s += strlen(find_str);
	s = strstr(s, "/");
	if (s == NULL)
	{
		return OPENMODE_RTN_FAIL;
	}
	s++;

	strcpy(tail, s);
	return OPENMODE_RTN_OK;
}

static OPENMODE_RTN_ENUM _is_itx_mac_range(guchar* dev_mac)
{
	const unsigned char mh_min[7][6] = {
		{ 0x1c, 0x7c, 0x45, 0x00, 0x00, 0x00 },
		{ 0xb8, 0x41, 0x5f, 0x01, 0xd4, 0xc1 },
		{ 0x00, 0x1c, 0xb8, 0x30, 0x00, 0x00 },
		{ 0x00, 0x11, 0x60, 0xff, 0x00, 0x00 },
		{ 0x00, 0x1b, 0x9d, 0x02, 0xa3, 0x7b },
		{ 0x00, 0x11, 0x5f, 0x00, 0x00, 0x00 },
		{ 0x00, 0xa7, 0x84, 0x00, 0x00, 0x00 }
	};
	const unsigned char mh_max[7][6] = {
		{ 0x1c, 0x7c, 0x45, 0x07, 0xff, 0xff },
		{ 0xb8, 0x41, 0x5f, 0x01, 0xfb, 0xd0 },
		{ 0x00, 0x1c, 0xb8, 0x35, 0xff, 0xff },
		{ 0x00, 0x11, 0x60, 0xff, 0xff, 0xff },
		{ 0x00, 0x1b, 0x9d, 0x03, 0x0f, 0xff },
		{ 0x00, 0x11, 0x5f, 0xff, 0xff, 0xff },
		{ 0x00, 0xa7, 0x84, 0xff, 0xff, 0xff }
	};

	OPENMODE_RTN_ENUM rtn = OPENMODE_RTN_FAIL;
	int i,j;

	for (i=0; i<7; i++)
	{
		for (j=0; j<6; j++)
		{
			if (dev_mac[j] >= mh_min[i][j] && dev_mac[j] <= mh_max[i][j])
			{
			}
			else
			{
				break;
			}
		}

		if (j >= 6)
		{
			IPCAM_DBG(MINOR, "MAC range(%02x:%02x:%02x:%02x:%02x:%02x - %02x:%02x:%02x:%02x:%02x:%02x\n",
					mh_min[i][0], mh_min[i][1], mh_min[i][2],
					mh_min[i][3], mh_min[i][4], mh_min[i][5],
					mh_max[i][0], mh_max[i][1], mh_max[i][2],
					mh_max[i][3], mh_max[i][4], mh_max[i][5]);
			IPCAM_DBG(MINOR, "DEV MAC - %02x:%02x:%02x:%02x:%02x:%02x\n",
					dev_mac[0], dev_mac[1], dev_mac[2],
					dev_mac[3], dev_mac[4], dev_mac[5]);
			rtn = OPENMODE_RTN_OK;
			break;
		}
	}

	return (rtn);
}

#if defined(ENABLE_PROJECT_KMW)
static void set_kmw_network(void)
{
	const char temp_ip_set[] = "ifconfig eth0:1 192.168.200.100";
	proxy_system(temp_ip_set);
}
#endif

static NFOpenmodeCamInfo* _sort_select_lan_mode(int from)
{
	NFOpenmodeCamInfo* iter = _list_find_entry_by_index(&_openmode_detection_list, from);
	NFOpenmodeCamInfo* selected = iter;

	IPCAM_DBG(MAJOR, "from(%d)\n", from);
	while(iter != NULL)
	{
		if (strcmp(selected->eth_dev,iter->eth_dev) < 0)
		{
			selected = iter;
		}
		iter = iter->next;
	}

	return selected;
}

static NFOpenmodeCamInfo* _sort_select_model_min(int from)
{
	NFOpenmodeCamInfo* iter = _list_find_entry_by_index(&_openmode_detection_list, from);
	NFOpenmodeCamInfo* selected = iter;

	IPCAM_DBG(MAJOR, "from(%d)\n", from);
	while(iter != NULL)
	{
		if (strcmp(selected->model,iter->model) > 0)
		{
			selected = iter;
		}
		iter = iter->next;
	}

	return selected;
}

static NFOpenmodeCamInfo* _sort_select_ip_min(int from)
{
	NFOpenmodeCamInfo* iter = _list_find_entry_by_index(&_openmode_detection_list, from);
	NFOpenmodeCamInfo* selected = iter;

	while(iter != NULL)
	{
		if (selected->ipaddr > iter->ipaddr)
		{
			selected = iter;
		}
		iter = iter->next;
	}

	return selected;
}

static NFOpenmodeCamInfo* _sort_select_status_min(int from)
{
	NFOpenmodeCamInfo* iter = _list_find_entry_by_index(&_openmode_detection_list, from);
	NFOpenmodeCamInfo* selected = iter;

	while(iter != NULL)
	{
		if (selected->state > iter->state)
		{
			selected = iter;
		}
		iter = iter->next;
	}

	return selected;
}

static NFOpenmodeCamInfo* _sort_select_ch_min(int from)
{
	NFOpenmodeCamInfo* iter = _list_find_entry_by_index(&_openmode_detection_list, from);
	NFOpenmodeCamInfo* selected = iter;
	int selected_ch=(-1);
	int iter_ch=(-1);

	while(iter != NULL)
	{
		if (selected->ch < 0)
		{
			selected_ch = 100;
		}
		else
		{
			selected_ch = selected->ch;
		}
		if (iter->ch < 0)
		{
			iter_ch = 100;
		}
		else
		{
			iter_ch = iter->ch;
		}
		if (selected_ch > iter_ch)
		{
			selected = iter;
		}
		iter = iter->next;
	}

	return selected;
}

static void _sort_change_entry_pos(NFOpenmodeCamInfo *e1, NFOpenmodeCamInfo *e2)
{
	int temp = e1->index;
	NFOpenmodeCamInfo *e1_prev = e1->prev;
	NFOpenmodeCamInfo *e1_next = e1->next;
	NFOpenmodeCamInfo *e2_prev = e2->prev;
	NFOpenmodeCamInfo *e2_next = e2->next;
	int e1_is_head=0;
	int e1_is_tail=0;
	int e2_is_head=0;
	int e2_is_tail=0;
	if (e1 == e2) { return; }

	if (e1 == _openmode_detection_list.head)
	{
		e1_is_head = 1;
	}
	if (e1 == _openmode_detection_list.tail)
	{
		e1_is_tail = 1;
	}
	if (e2 == _openmode_detection_list.head)
	{
		e2_is_head = 1;
	}
	if (e2 == _openmode_detection_list.tail)
	{
		e2_is_tail = 1;
	}
	if (e1_next == e2)
	{
		e1_next = e1;
		e2_prev = e2;
	}

	e1->index = e2->index;
	e2->index = temp;

	e1->prev = e2_prev;
	e1->next = e2_next;
	if (e2_prev != NULL)
	{
		e2_prev->next = e1;
	}
	if (e2_next != NULL)
	{
		e2_next->prev = e1;
	}

	e2->prev = e1_prev;
	e2->next = e1_next;
	if (e1_prev != NULL)
	{
		e1_prev->next = e2;
	}
	if (e1_next != NULL)
	{
		e1_next->prev = e2;
	}

	if (e1_is_head)
	{
		_openmode_detection_list.head = e2;
	}
	if (e1_is_tail)
	{
		_openmode_detection_list.tail = e2;
	}
	if (e2_is_head)
	{
		_openmode_detection_list.head = e1;
	}
	if (e2_is_tail)
	{
		_openmode_detection_list.tail = e1;
	}
}

static void _sort_change_order(void)
{
	NFOpenmodeCamInfo *e1 = _openmode_detection_list.head;
	NFOpenmodeCamInfo *e2 = _openmode_detection_list.tail;
	NFOpenmodeCamInfo *e1_next = NULL;
	NFOpenmodeCamInfo *e2_next = NULL;

	while (e1 != NULL && e2 != NULL)
	{
		if (e1->index >= e2->index)
		{
			break;
		}
		e1_next = e1->next;
		e2_next = e2->prev;
		_sort_change_entry_pos(e1,e2);
		if (e1_next == e2)
		{
			break;
		}
		e1 = e1_next;
		e2 = e2_next;
	}
}


static int parsing_login_info_and_addr(const char *string, char *login_info, char *addr)
{
	const char *regex = "\\([[:alnum:]]\\+:[[:alnum:]~!@#$%^&*()`_+=<>,.?|:'\\\";\\-]\\+@\\)"; // USER
	int rtn = 0, rc = 0;
	regex_t    preg;
	size_t nmatch = 1;
	regmatch_t pmatch[1];

	rtn = 0;
	if (0 != (rc = regcomp(&preg, regex, 0))) 
	{
		printf("[%s] regcomp() failed, returning nonzero (%d)\n", __FUNCTION__, rc);
		rtn = 1;
		goto end_label;
	}

	if (0 == (rc = regexec(&preg, string, nmatch, pmatch, 0))) 
	{
		memcpy(login_info, &string[pmatch[0].rm_so], (pmatch[0].rm_eo - pmatch[0].rm_so) - 1);
		strcpy(addr, &string[(pmatch[0].rm_so + (pmatch[0].rm_eo - pmatch[0].rm_so))]);
	}
	else
	{
		// FAIL
		rtn = 1;
	}

	regfree(&preg);

end_label:
	return rtn;
}

static OPENMODE_RTN_ENUM _parsing_from_rtsp_address(char* rtsp_addr, NFOpenmodeVirtualCamera *vcam)
{
	gchar protocol[16] = {0};
	gchar account[256] = {0};
	gchar addr[256] = {0};
	gchar host[256] = {0};
	int rtsp_port = 0;

	char ip_str[20];
	char buffer[1024];

	strncpy(buffer, rtsp_addr, sizeof(buffer));

	/* rtsp 주소 정규식 검사 */
	{
		//char *regex = "^rts?p://(.*:.*@)?[[:digit:]]{1,3}([.]{1}[[:digit:]]{1,3}){3}(:[[:digit:]]{1,})?.*$";
		char *regex = "^rts?p://(.*:.*@)?.*$";

		regex_t ext_regex;
		int ret = regcomp(&ext_regex, regex, REG_EXTENDED);

		if( ret != 0 )
		{
			IPCAM_DBG(WARN, "regular expression is not valid\n");
			return OPENMODE_RTN_FAIL;
		}

		ret = regexec( &ext_regex, rtsp_addr, 0, NULL, 0);
		if( !ret )
		{
			IPCAM_DBG(MINOR, "Match RTSP Address expression\n", index);
		}
		else
		{
			IPCAM_DBG(WARN, "Not Match RTSP Address expression\n");
			regfree(&ext_regex);
			return OPENMODE_RTN_FAIL;
		}
		regfree(&ext_regex);
	}

	/*** login 정보 포함 ***/
	if(strstr(rtsp_addr, "@") != NULL)
	{
		if(!parsing_login_info_and_addr(rtsp_addr, account, addr))
		{
			// User Name and Password
			if(sscanf(account, "%10[^:]:%s", vcam->u_id , vcam->u_password) == 2 )
			{
				IPCAM_DBG(MINOR, "[vcamera ID:%s, PW:%s]\n", vcam->u_id, vcam->u_password);
			}

			if(sscanf(addr, "%256[^:]", host) == 1)
			{
				/*
				int ret = 0;
				unsigned char buf[sizeof(struct in6_addr)];
				ret = inet_pton(AF_INET, ip, buf);
				if(ret == 0)
				{
					return OPENMODE_RTN_FAIL;
				}
				*/
				IPCAM_DBG(MINOR, "[vcamera IP:%s ]\n", host);
			}

			char *p;
			p = strstr(addr, ":");
			if( p != NULL)
			{
				if(sscanf(p, ":%d", &rtsp_port) == 1)
				{
					IPCAM_DBG(MINOR, "[vcamera RTSP PORT:%d ]\n", rtsp_port);
				}

				if(rtsp_port == 0)
				{
					rtsp_port = 554;
				}
			}
			else
			{
				IPCAM_DBG(MINOR, "[vcamera RTSP PORT:%d ]\n", rtsp_port);
				rtsp_port = 554;
			}

			if(rtsp_port < 0 || 65535 < rtsp_port)
			{
					return OPENMODE_RTN_FAIL;
			}
		}   
		else
		{
			IPCAM_DBG(WARN, "auth info regular expression is not valid\n");
			return OPENMODE_RTN_FAIL;
		}
		snprintf(vcam->host, sizeof(vcam->host), "%s", host);
        nf_openmode_dns_lookup(host, ip_str); 
		vcam->ipaddr = ntohl(inet_addr(ip_str));
		vcam->rtsp_port = rtsp_port;
	}
	/*** login 정보 미포함 ***/
	else
	{
		if(sscanf(buffer,"%50[^:/]://%2000[^/]/", protocol, addr) == 2)
		{
			if(sscanf(addr, "%256[^:]", host) == 1)
			{
				/*
				int ret = 0;
				unsigned char buf[sizeof(struct in6_addr)];
				ret = inet_pton(AF_INET, ip, buf);
				if(ret == 0)
				{
					return OPENMODE_RTN_FAIL;
				}
				*/

				IPCAM_DBG(MINOR, "[vcamera IP:%s ]\n", host);
			}

			char *p;
			p = strstr(addr, ":");
			if( p != NULL)
			{
				if(sscanf(p, ":%d", &rtsp_port) == 1)
				{
					IPCAM_DBG(MINOR, "[vcamera RTSP PORT:%d ]\n", rtsp_port);
				}

				if(rtsp_port == 0)
				{
					rtsp_port = 554;
				}
			}
			else
			{
				IPCAM_DBG(MINOR, "[vcamera RTSP PORT:%d ]\n", rtsp_port);
				rtsp_port = 554;
			}

			if(rtsp_port < 0 || 65535 < rtsp_port)
			{
					return OPENMODE_RTN_FAIL;
			}
		}   
		else
		{
			return OPENMODE_RTN_FAIL;
		}
		snprintf(vcam->host, sizeof(vcam->host), "%s", host);
        nf_openmode_dns_lookup(host, ip_str); 
		vcam->ipaddr = ntohl(inet_addr(ip_str));
		vcam->rtsp_port = rtsp_port;
	}

	return OPENMODE_RTN_OK;
}

static int vcam_get_state_info_from_describe_test(gchar *username, gchar *password, gchar *rtsp_addr, guint ip_addr, guint rtsp_port, guint *audio_flag)
{

	//DESCRIBE:
	//ipaddr 는 ntohl(inet_addr(ip)) 한 것
	gint cnt = 0;
	gint len = 0;
	gchar buf[NBUF_SZ];
	unsigned char auth_str[2048];
	char *ok_string = "200 OK";
	char* auth_string = "401 Unauthorized";


	gint rtsp_sock;
	guint state;
	guint req_seq = 0;

	gchar nonce_result[512];
	gchar realm_result[512];

	ip_addr = htonl(ip_addr);

	/* Try rtsp connection */
	{
		int sock;
		struct sockaddr_in sin;

		memset(&sin, 0x00, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = ip_addr;
		sin.sin_port = htons(rtsp_port);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
		{
			IPCAM_DBG(ERROR, "socket descriptor get fail(%s)\n", __FUNCTION__);
			perror("socket");
			//state = STATE_INITIAL;
			return -2;
		}

		{
			int buf_size = 0;
			int len = 0;

			len = sizeof(int);

			// 소켓 버퍼 크기 구하는 함수
			getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_size, (socklen_t *)&len);

			buf_size = 1024*1024;
			setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, (socklen_t)len);

			len = sizeof(int);
			getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_size, (socklen_t *)&len);
		}
#if 1
		{
			struct timeval tv;
			tv.tv_sec = 10; // 10 Secs Timeout
			tv.tv_usec = 0;
			int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			if (ret < 0){}
		}
#endif


		if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) == -1)
		{
			IPCAM_DBG(ERROR, "socket connect fail(%s)\n", __FUNCTION__);
			close(sock);
			sock = (-1);
			rtsp_sock = (-1);
			return -2;
		}

		rtsp_sock = sock;
	}

	/* RTSP command 'DESCRIBE' */

	memset(buf, 0x00, NBUF_SZ);
	snprintf(buf, NBUF_SZ,                            
			"DESCRIBE %s RTSP/1.0\r\n"                      
			"CSeq: %u\r\n"                                  
			"Accept: application/sdp\r\n"                   
			"User-Agent: " USER_AGENT_STR "\r\n"
			"Data-Encryption: partial\r\n"                  
			"\r\n",                                         
			rtsp_addr, req_seq++);          

	if (send(rtsp_sock, buf, strlen(buf), 0) < 0)
	{
		IPCAM_DBG(ERROR, "message send fail(%s)\n", __FUNCTION__);
		perror("send");
		close(rtsp_sock);
		rtsp_sock = (-1);
		return -2;
	}

	memset(buf, 0x00, NBUF_SZ);
	len = recv(rtsp_sock, buf, NBUF_SZ, 0);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "message recv fail(%s)\n", __FUNCTION__);
		perror("recv");
		close(rtsp_sock);
		rtsp_sock = (-1);
		return -2;
	}

	{
		char *p = NULL;
		char *s = NULL;
		char *e = NULL;
		char *user = NULL;
		char *pass = NULL;
		char *realm;
		char *nonce;
		char *uri;
		char *method = "DESCRIBE";
		const char* f_str_auth = "401 Unauthorized";
		const char* f_str_realm = "realm=\"";
		const char* f_str_nonce = "nonce=\"";


		p = g_strstr_len(buf, NBUF_SZ, f_str_auth);

		if (p != NULL)
		{
			s = g_strstr_len(buf, NBUF_SZ, f_str_realm);
			if (s == NULL)
			{
				close(rtsp_sock);
				return -2;
			}
			s += strlen(f_str_realm);
			e = g_strstr_len(s, NBUF_SZ, "\"");
			if (e == NULL)
			{
				close(rtsp_sock);
				return -2;
			}
			memset(realm_result, 0x00, 512);
			memcpy(realm_result, s, (e-s)>511?511:(e-s));
			realm = &realm_result[0];

			s = g_strstr_len(buf, NBUF_SZ, f_str_nonce);
			if (s == NULL)
			{
				close(rtsp_sock);
				return -2;
			}
			s += strlen(f_str_nonce);
			e = g_strstr_len(s, NBUF_SZ, "\"");
			if (e == NULL)
			{
				close(rtsp_sock);
				return -2;
			}
			memset(nonce_result, 0x00, 512);
			memcpy(nonce_result, s, (e-s)>511?511:(e-s));
			nonce = &nonce_result[0];

			uri =  &rtsp_addr[0];
			user = &username[0];
			pass = &password[0];


			gst_mrtp_src_rtsp_digest_auth_str(user, pass, realm, nonce, uri, method, auth_str);
			//itx_digest_auth_str(user, pass, realm, nonce, uri, method, auth_str);
			memset(buf, 0x00, NBUF_SZ);
			snprintf(buf, NBUF_SZ,
					"DESCRIBE %s RTSP/1.0\r\n"
					"CSeq: %u\r\n"
					"%s"
					"User-Agent: " USER_AGENT_STR "\r\n"
					"Data-Encryption: partial\r\n"
					"\r\n",
					uri, req_seq++, auth_str);

			if (send(rtsp_sock, buf, strlen(buf), 0) < 0)
			{
				close(rtsp_sock);
				return -2;
			}

			memset(buf, 0x00, NBUF_SZ);
			len = recv(rtsp_sock, buf, NBUF_SZ, 0);
			if (len < 0)
			{
				close(rtsp_sock);
				return -2;
			}

		}

	}
	char *ok_result, *auth_error;
	ok_result = g_strstr_len(buf, NBUF_SZ, ok_string);
	auth_error = g_strstr_len(buf, NBUF_SZ, auth_string); 


	/*AUDIO: audio flag parsing */
	const gchar f_str_video[] = "m=video";
	const gchar f_str_audio[] = "m=audio";
	const gchar f_str_rtpavp[] = "RTP/AVP ";
	guint pt; // audio flag

	gchar *sdp_ptr = NULL;
	gchar *media_ptr = NULL;
	gchar *pt_ptr= NULL;

	/* Find media description */
	sdp_ptr = buf;
	
	if(ok_result != NULL)
	{
		/* Audio media description parsing */

		gchar temp_pt[8] = { 0, };

		if ((sdp_ptr = g_strstr_len(sdp_ptr, NBUF_SZ, f_str_audio)) == NULL)
		{
			//MRTPSRC_DBG(ERROR, "%s | No audio description", __FUNCTION__);
			close(rtsp_sock);
			rtsp_sock = (-1);
			return 0;
		}

		media_ptr = sdp_ptr + 7; //strlen(f_str_audio);
		if ((media_ptr = g_strstr_len(media_ptr, NBUF_SZ, f_str_rtpavp)) == NULL)
		{
			//MRTPSRC_DBG(ERROR, "%s | No audio payload type", __FUNCTION__);
			close(rtsp_sock);
			rtsp_sock = (-1);
			return 0;
		}

		/* Audio payload type set */
		pt_ptr = media_ptr + 8; //strlen(f_str_rtpavp);
		{
			gint __len;
			media_ptr = _get_endline(pt_ptr, &__len);
			if (media_ptr == NULL)
			{
				//MRTPSRC_DBG(ERROR, "%s | Audio payload type", __FUNCTION__);
				close(rtsp_sock);
				return 0;
			}
			memset(temp_pt, 0x00, 8);
			memcpy(temp_pt, pt_ptr, (media_ptr-pt_ptr)>7?7:(media_ptr-pt_ptr));
			pt = atoi(temp_pt);
			sdp_ptr = media_ptr + __len; //strlen("\r\n");
		}

		if(pt == 0)
			*audio_flag = 1; // G711 u_Law supported

		close(rtsp_sock);
		return 0;

	}

	else if(auth_error != NULL) // 401 Unauthorized
	{
		IPCAM_DBG(ERROR, "Authorization fail(%s)\n", __FUNCTION__);
		close(rtsp_sock);
		return -1;
	}
	else // etc.. ex) 401 Bad Request
	{
		IPCAM_DBG(ERROR, "Inadvertent result(%s)\n", __FUNCTION__);
		close(rtsp_sock);
		return -2;
	}

}

static gchar *vcam_remove_account_from_rtsp_addres(gchar* host, gchar* account, gchar* replace)
{
	gchar *result, *sr;
	size_t i, count = 0;
	size_t oldlen = strlen(account); 

	if (oldlen <= 2) // example -> :@ 
		return host;
	size_t newlen = strlen(replace);


	if (newlen != oldlen) 
	{
		for (i = 0; host[i] != '\0';) 
		{
			if (memcmp(&host[i], account, oldlen) == 0) 
			{
				count++;
				i += oldlen;
			}
			else 
				i++;
		}
	} 
	else 
		i = strlen(host);


	result = (char *) malloc(i + 1 + count * (newlen - oldlen));
	if (result == NULL) 
		return NULL;


	sr = result;

	while (*host) 
	{
		if (memcmp(host, account, oldlen) == 0) 
		{
			memcpy(sr, replace, newlen);
			sr += newlen;
			host  += oldlen;
		}
		else 
			*sr++ = *host++;
	}
	*sr = '\0';

	return result;
}

static gchar* _get_endline(gchar* src, gint* len)
{
	gchar* r;
	gchar* n;
	gchar* rn;

	rn = g_strstr_len(src, NBUF_SZ, "\r\n");
	if (rn != NULL)
	{
		*len = 2;
		return rn;
	}

	r = g_strstr_len(src, NBUF_SZ, "\r");
	n = g_strstr_len(src, NBUF_SZ, "\n");

	if (r==NULL && n==NULL)
	{
		*len = 0;
		return NULL;
	}

	if (r!=NULL && n!=NULL)
	{
		*len = 1;
		return ((r>n) ? n:r);
	}

	if (r==NULL)
	{
		*len = 1;
		return n;
	}
	if (n==NULL)
	{
		*len = 1;
		return r;
	}

	*len = 0;
	return NULL;
}

static char *_get_jpeg_url_path(NFOpenmodeCamInfo *info, char *buffer);
static jpeg_image_data _http_get_image_data(unsigned int ip, const char *id, const char *pw, const char *path, int port, int ssl);
jpeg_image_data nf_openmode_get_snapshot_image(NFOpenmodeCamInfo *info)
{
    jpeg_image_data jpg = {NULL, 0};
    char url_path[256] = {0, };
    const char *id = NULL;
    const char *pw = NULL;
    
    if(info == NULL){
        printf("[%s:%d] error info[%p]\n", __func__, __LINE__, info);
        return jpg;
    }

    if(strlen(info->u) > 0){
        id = info->u;
    }else if(strlen(info->u_done) > 0){
        id = info->u_done;
    }

    if(strlen(info->p) > 0){
        pw = info->p;
    }else if(strlen(info->p_done) > 0){
        pw = info->p_done;
    }

    if(id == NULL || pw == NULL){
        printf("[%s:%d] warn id[%s] pw[%s] u[%s] p[%s] u_done[%s] p_done[%s]\n", __func__, __LINE__, id, pw,
            info->u,
            info->p,
            info->u_done,
            info->p_done);
    }

    jpg = _http_get_image_data(info->ipaddr, id, pw, 
         _get_jpeg_url_path(info, url_path),
         info->http_port, info->use_ssl);

    return jpg;
}

static char *_get_jpeg_url_path(NFOpenmodeCamInfo *info, char *buffer)
{
    int rtn;
    char snapshot_uri[256] = {0, };
	char device_xaddr[256];
    const char *uri = NULL;

    if(info == NULL || buffer == NULL){
        printf("[%s:%d] argument error info[%p] buffer[%p]\n", __func__, __LINE__, info, buffer);
        return NULL;
    }

    if(strlen(info->media_xaddr) == 0){     //itx
        sprintf(buffer, "/cgi-bin/jpeg.fcgi?api=raw&chno=0");
    }else{   //onvif
        snprintf(device_xaddr, 256, "https://%d.%d.%d.%d:%d/%s",
                (info->ipaddr&0xff000000)>>24,
                (info->ipaddr&0xff0000)>>16,
                (info->ipaddr&0xff00)>>8,
                (info->ipaddr&0xff),
                info->http_port,
                "onvif/device_service");
        rtn = nf_onvif_media_get_snapshot_uri(info->media_xaddr, device_xaddr, info->auth, info->u, info->p, info->token, snapshot_uri);

            uri = strstr(snapshot_uri, "://");
            if(uri){
                uri+= 3;
            }else{
                uri = snapshot_uri;
            }

            uri = strstr(uri, "/");
            if(uri == NULL){
                uri = "/";
            }

        sprintf(buffer, "%s", uri);
    }

    return buffer;
}

static jpeg_image_data _http_get_image_data(unsigned int ip, const char *id, const char *pw, const char *path, int port, int ssl)
{
    int rc;
    char ipbuf[20];
    jpeg_image_data ret;
    HTTP_CTX ctx;

    memset(&ret, 0, sizeof(jpeg_image_data));

    //argument check
    if(ip == 0 || ip == 0xffffffff ||  path == NULL)// || image_index <= 0) // || image_name == NULL
    {
        printf("[%s:%d] ip[%08x] id[%p] pw[%p] path[%p]\n", __func__, __LINE__, ip, id, pw, path);
        return ret;
    }

    if(strlen(path) == 0)
    {
        printf("[%s:%d] path[%s]\n", __func__, __LINE__, id, pw, path);
        return ret;
    }

    /*
    printf("[%s:%d] ip[%s][%08x] id[%s] pw[%s] path[%s] port[%d] ssl[%d]\n", __func__, __LINE__, 
            _ip_to_str(ip, NULL), ip,
            id,
            pw,
            path,
            port,
            ssl);
            */

    memset(ipbuf, 0, sizeof(ipbuf));

    http_init(&ctx);
    if(id && pw){
        if(strlen(id) > 0 && strlen(pw) > 0){
            http_data_set(&ctx, HTTP_SET_USER, id);
            http_data_set(&ctx, HTTP_SET_PASSWD, pw);
        }
    }

    _ip_to_str(ip, ipbuf);
    http_data_set(&ctx, HTTP_SET_HOST, ipbuf);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_PORT, port);
    http_data_set(&ctx, HTTP_SET_SSL, ssl);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] CONNECTION_FAILED\n", __func__, __LINE__);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] RETCODE_ERROR\n", __func__, __LINE__);
        goto endl;
    }


    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        goto endl;
    }

    ret.memory = ctx.res_data.memory;
    ret.size = ctx.res_data.size;
    ctx.res_data.memory = NULL;

endl:
    http_release(&ctx);
    return ret;
}

static char* get_net_inf_from_cam(guint cam_ip)
{
	//printf("★★★ get_net_inf_from_cam .. \n");
	gint is_custom = nf_sysdb_get_bool("cam.install.dual_lan");
	guint _hub_ip;
	guint _subnet;
	if(is_custom)
	{
		_hub_ip = get_netif_ip(HUB_ETH_DEV);
		_subnet = get_netif_mask(HUB_ETH_DEV);
		if(_subnet==0)
			return HOST_ETH_DEV;
	/*	
		printf("★★★ cam_ip : [%d] \n", cam_ip);
		printf("★★★ subnet : [%d] \n", _subnet);
		printf("★★★ hub_ip : [%d] \n", _hub_ip);
	*/

		if((_hub_ip & _subnet) == (cam_ip & _subnet)) // 만약 HUB 네트워크를 사용한다면
			return HUB_ETH_DEV;
		else
			return HOST_ETH_DEV;
	}
	else
	{
		return HOST_ETH_DEV;
	}
}

#endif // __NF_API_OPENMODE_C__
