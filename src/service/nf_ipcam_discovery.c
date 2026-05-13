/*
 * ITX Security
 *  System software group
 *
 *  2011-01-06 Author jykim
 *  2011-09-01 jykim revision framework
 */

#ifndef __NF_IPCAM_DISCOVERY__
#define __NF_IPCAM_DISCOVERY__

#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>

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
#include <nf_api_ipcam.h>
#include <nf_api_live.h>
#include <nf_ipcam_defs.h>
#include <nf_util_device.h>
#include "nf_nvs_driver_itx.h"
#include "board_pp.h"
#include "nf_ipcam_driver_xiongmai.h"
#include "nf_api_eventlog.h"
#include "nf_api_dlva.h"
#include "nf_util_netif.h"



#define ADMIN_SVR_PORT					(32679)
#define ADMIN_CLI_PORT					(32678)
#define SOCK_BUF_SZ						(1536)
#define STATE_PRINT						(1)
#define PRS_INTERVAL					(1)





static pthread_mutex_t sendp_lock_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t recvp_lock_mtx = PTHREAD_MUTEX_INITIALIZER;

static unsigned int cam_mgr_state = DISCOVERY_STOPPED;

static int send_sock = (-1);
static int rcv_sock = (-1);
static unsigned int cur_link_status = 0;
static unsigned int cur_delay_status = 0;
static unsigned int cur_wait_status = 0;

static mtable *runtime = NULL;
static GAsyncQueue *queue = NULL;

static dtable discovery[AVAILABLE_MAX_CH];
static rtbl rcvd_cur[64];
static int rcvd_cnt = 0;
//static pthread_t th_ipmanager;
//static pthread_t th_ipassign;

static void ipcam_init_discovery(void);
static void ipcam_stop_discovery(void);
static void ipcam_start_discovery(void);
static void ipcam_localport_hwscan(void);
static void ipcam_itxcamera_swscan(void);
static void ipcam_analyze_netconf(void);
static void ipcam_request_ipaddr(void);
static void ipcam_finalize_cycle(void);
static void nvs_video_handler(void);

static void send_msgp(netconf_msg*);
static void PRINT_DTABLE(void);
static void ipcam_power_supply(void);

//--- < ipcam_discovery_main_loop > thread check---------------------------
static pthread_t discovery_check;
static long long func_check_time = 0LL;
static int check_step = 0;
static int check_ch = 0;
static long long get_milisec();
static void discovery_check_set(int step, int ch);
static void ipcam_discovery_main_loop_check(void);
static int loop_stop_cnt = 0;
// ------------------------------------------------------------

extern int if_nametoindex(char*);
extern gboolean nf_notify_fire_params(const gchar*, guint, guint, guint, guint);
extern void check_set_network_for_discovery(void);
extern int ipx_hub_current_link_status(int);
extern int hub_find_port(unsigned char*);


/*
 * IPCAM stop??발생?�키???�작�??�계?�여 ?�용?�게 ?�는 기능
 *  - IP 변�? ?�스???�맷 ?�의 network interface가 ?�려가???�스???�정??경우
 *  - 기�? switch port monitoring??진행?��? ?�아???�는 경우 discovery stop?�기 ?�한 ?�정
 */
extern void set_running_state(unsigned int val)
{
	IPCAM_DBG(MAJOR, "start val(%u)\n", val);

	cam_mgr_state = val;

	switch(cam_mgr_state)
	{
		case DISCOVERY_STOPPED:
		{
			IPCAM_DBG(MINOR, "DISCOVERY_STOPPED\n");
			ipcam_stop_discovery();
			break;
		}
		case DISCOVERY_RUNNING:
		{
			IPCAM_DBG(MINOR, "DISCOVERY_RUNNING\n");
			ipcam_start_discovery();
			break;
		}
		case DISCOVERY_HANDLER_RUNNING:
		{
			IPCAM_DBG(ERROR, "DISCOVERY_HANDLER_RUNNING\n");
			break;
		}
		default:
		{
			break;
		}
	}
	IPCAM_DBG(MAJOR, "end\n");
}

/*
 * set_running_state?� ?�계?�어?�는 조회 기능
 */
extern unsigned int get_running_state()
{
	return cam_mgr_state;
}

/*
 * discovery management data table 조회
 */
extern dtable* get_dtable(void)
{
	return (&discovery[0]);
}

/*
 * deprecated.
 */
extern void set_switch_polling_delay(int ch, int secs)
{
	//discovery[ch].polling_delay = secs;
}

/*
 * switch  링크 ?�태 조회 결과�?parameter return?�로 ?�달?��???
 * iHUB�?cascading?�서 ?�용?????�으므�? local port??경우 vhub???�태까�? ?�께 조회?�야?�다.
 */
extern void ipcam_disc_port_link_state(int ch, int* p_layer, int* p_linked)
{
	int rtn = 0;
	int is_linked = 0;
#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
	if (ch >= 0 && ch < NUM_ACTIVE_CH_DVR)
#else
	if (ch >= 0 && ch < 8)
#endif
	{
		is_linked = ch;
		/* Port link state polling */
		nf_dev_switch_get_link_status(&is_linked);
		if (is_linked)
		{
			*p_layer = IPCAM_DISC_LAYER_DVR;
			*p_linked = 1;
			return;
		}
	}

	rtn = ipx_hub_current_link_status(ch);
	if (rtn) {
		*p_layer = IPCAM_DISC_LAYER_VHUB;
		*p_linked = 1;
		return;
	}

	*p_layer = 0;
	*p_linked = 0;
	return;
}

static long long get_milisec()
{
	struct timeval tv;
	long long msec;

	gettimeofday(&tv, NULL);
	msec = tv.tv_sec * 1000LL + (long long)(tv.tv_usec / 1000LL);
	return msec;
}

static void discovery_check_set(int step, int ch)
{
	func_check_time = get_milisec();
	check_step = step;
	check_ch = ch;
	loop_stop_cnt = 0;
}

static void ipcam_discovery_main_loop_check(void)
{
	long long discovery_check_time = 0LL;
	long long discovery_repeat_time = 0LL;
	int discovery_mul = 1;
	gchar buf[128] = {0,};

	while (1)
	{
		if (check_step == 0)
		{
			g_usleep(10*1000);
			continue;
		}

		discovery_check_time = get_milisec();

		if (func_check_time + 60000LL * discovery_mul < discovery_check_time)
		{
			discovery_check_time = func_check_time;
			printf("[%s][%d] step : %d\n", __FUNCTION__, __LINE__, check_step);
			printf("[%s][%d] ch : %d\n", __FUNCTION__, __LINE__, check_ch);
			printf("[%s][%d] blocking time : %lld\n", __FUNCTION__, __LINE__, func_check_time);

			if (discovery_repeat_time == func_check_time)
				discovery_mul++;
			else
				discovery_mul = 1;

			discovery_repeat_time = func_check_time;

			loop_stop_cnt++;
			if (loop_stop_cnt > 10)
			{
				// push nand log
				snprintf(buf, 128, "Error : ipcam_discovery_main_loop stopped [step:%d][ch:%d][blocking time:%lld] ", check_step, check_ch, func_check_time);
				nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
				sleep(3);
				g_assert(0);
			}
		}
		sleep(1);
	}

}

/*
 * !!!! 중요
 * CCTV 모드?�서 discovery  ?�작??주�??�는 main thread loop
 */
extern void ipcam_discovery_main_loop(void)
{
	int i = 0;
	int on = 1;
	unsigned int seq_id = 0;
	unsigned int addr = 0;
	struct timespec now_time;
	static unsigned int __loop_cnt = 0;


#if 1
	{
		struct sched_param sched;
		pthread_t thread;
		cpu_set_t cpuset;
		int s=0;

		thread = pthread_self();

		CPU_ZERO(&cpuset);
		CPU_SET(0, &cpuset);

		s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		if (s != 0)
			g_warning("pthread_setaffinity_np");
		/* Check the actual affinity mask assigned to the thread */

		s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		if (s != 0)
			g_warning("pthread_getaffinity_np");
	}
#endif
	IPCAM_DBG(MAJOR, "start\n");

	pthread_create(&discovery_check, NULL, (void*)&ipcam_discovery_main_loop_check, NULL);

	/* discovery initialization */
	ipcam_init_discovery();

	while(1)
	{
		usleep(300*1000);

		discovery_check_set(1, -2);

		if (cam_mgr_state == DISCOVERY_STOPPED)
		{
			continue;
		}

		/* Loop count */
		__loop_cnt++;

		discovery_check_set(1, -1);

		/* print discovery table every 4 loops */
		if ((__loop_cnt % 3) == 0) { PRINT_DTABLE(); }

		discovery_check_set(2, -1);

		clock_gettime(CLOCK_REALTIME, &now_time);
		//IPCAM_DBG(MINOR, "loop_cnt(%lu), time(%lu)\n", __loop_cnt, now_time.tv_sec);
		printf("[%s] loop_cnt(%lu), time(%lu)\n", __FUNCTION__, __loop_cnt, now_time.tv_sec);

	// System running중에 network interface?�정??변경된 경우 증상 ?�이 카메???�결???�되??경우가 발생?�게 ?�다.
	// ?�에 ?�???�외처리�?network interface???�??무결?�을 보장?????�는 ?�작??추�??�었??
	// IPX??MAC??1개만 ?�용?�고 물리?�으�??�절??2개의 ?�트?�크(?��??�트?�크+카메?�네?�워??�??�용
	//   ?�야 ?�는 구조?��?�??��? ?�트?�크 ?�태???�라 ?��? ?�면 DHCP lease time 만료???�른 ?��? IP ?�할???�과
	//   같�? ?�스???�벤?�에 ?�해 MAC???�려가??경우  카메???�트?�크가 ?�어지??문제?�이 발생?????�다.
	// ?�라???�시 network interface�??�인?�고 ?�떤 ?�유�?카메???�트?�크가 ?�라�?경우 ?��? ???�성??준??
		/* Sub-network check */
		discovery_check_set(3, -1);
		addr = get_local_net_ip();

		discovery_check_set(4, -1);
		if (addr == 0)
		{
			discovery_check_set(5, -1);
			set_sub_network();
		}
		discovery_check_set(6, -1);
		// 카메???�트?�크 ???�성 메커?�즘???�장?�에??default gateway가 ??�� ?�어지??경우가 발생?????�다.
	// ??경우 broadcast, multicast?� 같�?류의 packet ?�송??불�??�해 지므�?마찬가지�?default gateway가
	//   ?�스?�상?�서 ?�거??것이 감�??�면 카메???�트?�크?�서 NVR IP�?default gateway�?지?�해 주어
	//   카메???�결??문제?�이 발생?�는 것을 방�??�다.
		/* Gateway check */
		check_set_network_for_discovery();

		discovery_check_set(7, -1);

		/* Initial scan of each ports' link state - Linked or Unlinked */
		ipcam_localport_hwscan();

		discovery_check_set(8, -1);
		/* try power-on every 7 loops */
		if ((__loop_cnt % 7) == 0) 
		{ 
			discovery_check_set(9, -1);
			ipcam_power_supply(); 
			discovery_check_set(21, -1);
		}

		/* If the port state LINK is detected, Send ADMIN protocol 'MSG_IP_SEARCH' */
		discovery_check_set(22, -1);
		ipcam_itxcamera_swscan();
		discovery_check_set(24, -1);
		usleep(200*1000);

		/* Analyze and empty netconf stack entries */
		ipcam_analyze_netconf();
		discovery_check_set(25, -1);
		/* Send IP address configuration request to new IP Cameras */
		ipcam_request_ipaddr();
		discovery_check_set(26, -1);
		usleep(200*1000);

		/* Recover lost ports */
		ipcam_finalize_cycle();
		discovery_check_set(27, -1);

		/* Check nvs video start/stop(video loss) */
		nvs_video_handler();
		discovery_check_set(28, -1);
	}
}

/*
 * !!!! 중요
 * CCTV 모드?�서 ONVIF 카메?�의 discovery  ?�작??주�??�는 main thread loop
 */
extern void ipcam_discovery_onvif_loop(void)
{
	while(1)
	{
		sleep(3);

		/* JIG FW?�서??ONVIF 카메??검출을 ?��? ?�는?? */
		if (nf_ipcam_is_vendor_zig())
		{
			continue;
		}
		/* IPCAM stop ?�태?�서??ONVIF 카메??검출을 ?��? ?�는?? */
		if (cam_mgr_state == DISCOVERY_STOPPED)
		{
			continue;
		}

		/* ONVIF cam search */
		ipcam_onvif_search();
		/* ONVIF cam capability set */
		ipcam_onvif_set();
	}
}

/*
 * !!!! 중요
 * CCTV 모드?�서 ONVIF 카메?�로부???�송???�킷???�신?�기하??loop
 */
extern void ipcam_discovery_onvif_recv_loop(void)
{
	int len = 0;
	int buf_sz = 8192;
	int multi_sock;
	int cin_len;
	struct sockaddr_in cin;
	char buf[buf_sz];

	while(1)
	{
		usleep(100*1000);
		multi_sock = nf_ipcam_get_msock();
		if (multi_sock <= 0) { continue; }
		memset(buf, 0x00, 8192);
		cin_len = sizeof(cin);
		len = recvfrom(multi_sock, buf, buf_sz, (MSG_DONTWAIT|MSG_PEEK), (struct sockaddr*)&cin, &cin_len);
		if (len > 0)
		{
			cin_len = sizeof(cin);
			len = recvfrom(multi_sock, buf, buf_sz, MSG_DONTWAIT, (struct sockaddr*)&cin, &cin_len);
		}
		else
		{
			continue;
		}
		if (len > 0)
		{
			nf_onvif_discovery_handler(buf, &cin);
		}
	}
}
static void _pirnt_mac(unsigned char *macaddr)
{
	int i = 0;
	printf("\033[0;36m %s [\033[0;39m", __FUNCTION__);
	for(i = 0; i < 6; i++)
		printf("\033[0;36m %02x\033[0;39m",macaddr[i]);
	printf("\033[0;36m ]\033[0;39m\n");
}
/*
 * !!!! 중요
 * CCTV 모드?�서 ITX 카메?�로부???�송??admin ?�로?�콜 ?�킷???�신?�기하??loop
 * ADMIN ?�로?�콜?� ?�래링크??기술?�어 ?�음
 * https://docs.google.com/spreadsheets/d/1cYusD5EucuV6KHo9os3636aveu-Cn-lAOeCkTBei1qA/edit#gid=2
 */
extern void ipcam_discovery_recv_loop(void)
{
	int i = 0;
	int j = 0;
	int len = 0;
	netconf_msg received;

	struct sockaddr_in cin;
	int cin_len;

	while(1)
	{
		memset((void*)&received, 0x00, sizeof(netconf_msg));
		len = recvfrom(rcv_sock, (void*)&received, sizeof(netconf_msg), 0, NULL, NULL);

		/* ?�신??admin packet??aibox ?��? ?�인 */
		if(len > 0 && _is_aibox(&received)){
			IPCAM_DBG(MINOR, "isDLVA\n");
			_discovery_aibox(&received, &cin);
			continue;
		}

#if 0
{
	netconf_msg a;
	memcpy(&a, &received, sizeof(netconf_msg));
	printf("version %d\n", a.version);
	printf("opcode  %d\n", a.opcode);
	printf("secs    %d\n", a.secs);
	printf("xid     %08x\n", a.xid);
	printf("magic   %08x\n", a.magic);
	printf("ciaddr  %d.%d.%d.%d\n",
			(a.ciaddr&0xff), (a.ciaddr&0xff00)>>8,
			(a.ciaddr&0xff0000)>>16, (a.ciaddr&0xff000000)>>24);
	printf("chaddr  %02x:%02x:%02x:%02x:%02x:%02x\n",
			a.chaddr[0], a.chaddr[1], a.chaddr[2], a.chaddr[3], a.chaddr[4], a.chaddr[5]);
	printf("yiaddr  %d.%d.%d.%d\n",
			(a.yiaddr&0xff), (a.yiaddr&0xff00)>>8,
			(a.yiaddr&0xff0000)>>16, (a.yiaddr&0xff000000)>>24);
	printf("miaddr  %d.%d.%d.%d\n",
			(a.miaddr&0xff), (a.miaddr&0xff00)>>8,
			(a.miaddr&0xff0000)>>16, (a.miaddr&0xff000000)>>24);
	printf("giaddr  %d.%d.%d.%d\n",
			(a.giaddr&0xff), (a.giaddr&0xff00)>>8,
			(a.giaddr&0xff0000)>>16, (a.giaddr&0xff000000)>>24);
	printf("dns1    %d.%d.%d.%d\n",
			(a.d1iaddr&0xff), (a.d1iaddr&0xff00)>>8,
			(a.d1iaddr&0xff0000)>>16, (a.d1iaddr&0xff000000)>>24);
	printf("dns2    %d.%d.%d.%d\n",
			(a.d2iaddr&0xff), (a.d2iaddr&0xff00)>>8,
			(a.d2iaddr&0xff0000)>>16, (a.d2iaddr&0xff000000)>>24);
	printf("http    %d\n", a.http_port);
	printf("https   %d\n", a.https_port);
	printf("rtsp    %d\n", a.rtsp_port);
	printf("reserve %d\n", a.reserve_port);
	printf("vend    %02x %02x %02x %02x %02x %02x\n",
			a.vend[0], a.vend[1], a.vend[2], a.vend[3], a.vend[4], a.vend[5]);
}
#endif

		/* NVR?�경??skip */
		if (received.vend[0] == 0x17 && received.vend[1] == 0x18 && received.vend[2] == 0x1)
		{
			continue;
		}

		/* ?�신??admin packet??opcode가 MSG_CAM_ACK??경우�??�기중??/
		if (received.opcode != MSG_CAM_ACK)
		{
			continue;
		}

		/* ?�신??admin packet??기다리고 ?�는 ?�트가 ?�는지 ?�인 */
		for (i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if (memcmp(discovery[i].macaddr, received.chaddr, 6) == 0)
			{
				break;
			}
		}

		// ?�기중???�트가 ?�다�?
		if (i < AVAILABLE_MAX_CH)
		{
			if (discovery[i].state == IPCAM_DISC_STATE_IPSETW && discovery[i].layer > 0	) // IP ?�당?�청 ??ack ?�기중??채널??경우 IP ?�업???�료??것임.
			{
				if (ntohl(received.xid) != discovery[i].transaction)
				{
					continue;
				}
				if (received.ciaddr != discovery[i].ipaddr)
				{
					continue;
				}

				IPCAM_DBG(MINOR, "CH(%d) ip assign(%d.%d.%d.%d) http(%d) rtsp(%d) transaction(%08x)\n",
						i,
						(received.ciaddr&0xff),
						((received.ciaddr&0xff00)>>8),
						((received.ciaddr&0xff0000)>>16),
						((received.ciaddr&0xff000000)>>24),
						ntohs(received.http_port),
						ntohs(received.rtsp_port),
						discovery[i].transaction);
				discovery[i].state = IPCAM_DISC_STATE_DONE;
				discovery[i].state_cnt = 0;
				discovery[i].retry_cnt = 0;
				discovery[i].vnet_id = 1;
				runtime[i].admin_http = ntohs(received.http_port);
				runtime[i].admin_rtsp = ntohs(received.rtsp_port);
				runtime[i].sys.ipaddr = discovery[i].ipaddr;
				runtime[i].sys.transaction = discovery[i].transaction;
				memcpy(&runtime[i].sys.macaddr[0], &discovery[i].macaddr[0], 6);

				{
					nf_pnd_evt_notify_fire(i, PND_TYPE_IP_DONE, __LINE__, __FILE__);
					runtime[i].state = (MGMT_STATE_LINKED|MGMT_STATE_READY);
					nf_pnd_queue_delay(i, IPCAM_EVENT_DEVICE_READY, 5, __LINE__, __FILE__);
				}
			}
			else
			{
				if (rcvd_cnt > 60)
				{
					continue;
				}
				pthread_mutex_lock(&recvp_lock_mtx);
				rcvd_cur[rcvd_cnt].transaction = ntohl(received.xid);
				memcpy(rcvd_cur[rcvd_cnt].macaddr, received.chaddr, 6);
				rcvd_cnt++;
				pthread_mutex_unlock(&recvp_lock_mtx);
			}
			continue;
		}

		if (rcvd_cnt > 60)
		{
			continue;
		}
		pthread_mutex_lock(&recvp_lock_mtx);
		rcvd_cur[rcvd_cnt].transaction = ntohl(received.xid);
		memcpy(rcvd_cur[rcvd_cnt].macaddr, received.chaddr, 6);
		rcvd_cnt++;
		pthread_mutex_unlock(&recvp_lock_mtx);
	}
}

/* ONVIF Pull 방식???�벤??조회 - ?�용?��? ?�음 */
extern void onvif_event_puller(void)
{
	int i, rtn;
	mtable* local_runtime = NULL;
	unsigned int event_active_state = 0;
	int event_fail_cnt[AVAILABLE_MAX_CH];
	memset(event_fail_cnt, 0x00, sizeof(int) * AVAILABLE_MAX_CH);

	g_message("%s | START !", __FUNCTION__);
	while(1)
	{
		usleep(1000*1000);
		local_runtime = get_runtime();
		if(local_runtime != NULL) break;
	}

	while(1)
	{
		usleep(500*1000);

		for(i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if((local_runtime[i].state & MGMT_STATE_CONFIGURED) && nf_ipcam_is_onvif_support(i) == 1)
			{
				if(event_fail_cnt[i] > 50) continue;	// FIXME
				if(event_active_state & (1 << i)) continue;
				if(!(local_runtime[i].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT))) continue;

				rtn = nf_onvif_create_event(i);

				if(rtn == 0)
				{	// success
					event_active_state |= (1 << i);
					event_fail_cnt[i] = 0;
				}
				else
				{	// failed
					event_fail_cnt[i]++;
				}
			}
		}

		usleep(500*1000);

		for(i = 0; i < AVAILABLE_MAX_CH; i++)
		{
			if((local_runtime[i].state & MGMT_STATE_CONFIGURED) && (event_active_state & (1 << i)) && nf_ipcam_is_onvif_support(i) == 1)
			{
				if(!(local_runtime[i].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT))) continue;

				rtn = nf_onvif_get_event(i);

				if(rtn != 0)
				{	// failed
					event_fail_cnt[i]++;
					if(event_fail_cnt[i] >= 5)
					{	// reset event
						event_active_state &= ~(1 << i);
						nf_onvif_delete_event(i);
					}
				}
			}
		}
	}
}

/* 
 * Xiongmai ?�벤?��? ?�신?�기 ?�한 server 
 *  - XM 카메?�는 ?�래 ?�버가 ?�기중???�트�??�벤?��? push?�다.
 */ 
extern void xm_event_notification_server(void)
{
	int noti_state = 0;
	int rtn;
	int sock = -1;
	int csock = -1;
	int clen = 0;
	int pos = 0;
	unsigned int csz = 0;
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	char *buf;
	struct timeval tv;


	buf = (char*) malloc(4096);
	while(1)
	{
		usleep(200*1000);
		if (noti_state == 0)
		{
			if (cam_mgr_state != DISCOVERY_STOPPED)
			{
				sin.sin_family = AF_INET;
				sin.sin_port = htons(XM_DEFAULT_ALARM_SERVER_PORT);
				sin.sin_addr.s_addr = get_host_info();

				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0)
				{
					perror("socket");
					continue;
				}

				rtn = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
				if (rtn < 0)
				{   
					perror("bind");
					close(sock);
					sock = (-1);
					continue;
				}   

				csz = sizeof(cin);
				rtn = listen(sock, 5);
				if (rtn < 0)
				{
					perror("listen");
					close(sock);
					sock = (-1);
					continue;
				}
				noti_state = 1;
			}
			else if (nf_get_running_mode()) // open mode
			{
				sin.sin_family = AF_INET;
				sin.sin_port = htons(XM_DEFAULT_ALARM_SERVER_PORT);
				sin.sin_addr.s_addr = get_host_info();

				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0)
				{
					perror("socket");
					continue;
				}

				rtn = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
				if (rtn < 0)
				{   
					perror("bind");
					close(sock);
					sock = (-1);
					continue;
				}   

				csz = sizeof(cin);
				rtn = listen(sock, 5);
				if (rtn < 0)
				{
					perror("listen");
					close(sock);
					sock = (-1);
					continue;
				}
				noti_state = 1;
			}
		}
		else
		{
			if (!nf_get_running_mode() && cam_mgr_state == DISCOVERY_STOPPED)
			{
				if (sock > 0)
				{
					close(sock);
					sock = (-1);
					noti_state = 0;
				}
				continue;
			}
			csock = accept(sock, (struct sockaddr*)&cin, &csz);
			if (csock <= 0)
			{
				perror("accept");
				close(sock);
				sock = (-1);
				noti_state = 0;
				continue;
			}

			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_XM_EVENT);
			GAsyncQueue *recevt_queue = get_recevt_queue();
			evt->port = csock;
			g_async_queue_push(recevt_queue, evt);
		}
	}
	free(buf);
	close(csock);
	close(sock);
}
/*
 * Grundig 카메???�벤??notification server
 */
extern void event_notification_server(void)
{
	int noti_state = 0;
	int rtn;
	int sock = -1;
	int csock = -1;
	int clen = 0;
	int pos = 0;
	unsigned int csz = 0;
	struct sockaddr_in sin;
	struct sockaddr_in cin;
	char *buf;
	struct timeval tv;


	tv.tv_sec = 3;
	tv.tv_usec = 0;
	buf = (char*) malloc(4096);
	while(1)
	{
		usleep(200*1000);
		if (noti_state == 0)
		{
			if (cam_mgr_state != DISCOVERY_STOPPED)
			{
				sin.sin_family = AF_INET;
				sin.sin_port = htons(NF_IPCAM_EVENT_NOTI_PORT);
				sin.sin_addr.s_addr = get_host_info();

				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0)
				{
					perror("socket");
					continue;
				}

				rtn = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
				if (rtn < 0)
				{   
					perror("bind");
					close(sock);
					sock = (-1);
					continue;
				}   

				csz = sizeof(cin);
				rtn = listen(sock, 5);
				if (rtn < 0)
				{
					perror("listen");
					close(sock);
					sock = (-1);
					continue;
				}

				rtn = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
				if (rtn < 0)
				{
					perror("setsockopt");
					close(sock);
					sock = (-1);
					continue;
				}

				noti_state = 1;
			}
			else if (nf_get_running_mode()) // open mode
			{
				sin.sin_family = AF_INET;
				sin.sin_port = htons(NF_IPCAM_EVENT_NOTI_PORT);
				sin.sin_addr.s_addr = get_host_info();

				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0)
				{
					perror("socket");
					continue;
				}

				rtn = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
				if (rtn < 0)
				{   
					perror("bind");
					close(sock);
					sock = (-1);
					continue;
				}   

				csz = sizeof(cin);
				rtn = listen(sock, 5);
				if (rtn < 0)
				{
					perror("listen");
					close(sock);
					sock = (-1);
					continue;
				}

				rtn = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
				if (rtn < 0)
				{
					perror("setsockopt");
					close(sock);
					sock = (-1);
					continue;
				}

				noti_state = 1;
			}
		}
		else
		{
			if (!nf_get_running_mode() && cam_mgr_state == DISCOVERY_STOPPED)
			{
				if (sock > 0)
				{
					close(sock);
					sock = (-1);
					noti_state = 0;
				}
				continue;
			}
			csock = accept(sock, (struct sockaddr*)&cin, &csz);
			if (csock <= 0)
			{
				perror("accept");
				close(sock);
				sock = (-1);
				noti_state = 0;
				continue;
			}
			memset(buf, 0x00, 4096);
			rtn = recv(csock, buf, 4096-1, 0);
			if (rtn < 0)
			{   
				perror("recv");
				close(csock);
				csock = (-1);
				continue;
			}
			send(csock, "HTTP/1.0 200 OK\r\n\r\n", 18, 0);
			close(csock);
			csock = (-1);

			buf[rtn] = '\0';

			{
				char *p = NULL;
				int w10 = 0;
				int w1 = 0;
				int ch;
				const char *find_str_motion = "motion_";
				const char *find_str_alarm = "alarm_";

				if ((p = strstr(buf, find_str_motion)) != NULL)
				{
					p += strlen(find_str_motion);
					w10 = *p - '0';
					p += 1;
					w1 = *p - '0';
					ch = (w10 * 10) + w1;
					if( ch >= 0 && ch < NUM_ACTIVE_CH)  // CH: 0 ~ (NUM_ACTIVE_CH-1)
						runtime[ch].motion_flag = 90;
				}
				if ((p = strstr(buf, find_str_alarm)) != NULL)
				{
					p += strlen(find_str_alarm);
					w10 = *p - '0';
					p += 1;
					w1 = *p - '0';
					ch = (w10 * 10) + w1;
					if( ch >= 0 && ch < NUM_ACTIVE_CH)  // CH: 0 ~ (NUM_ACTIVE_CH-1)
						runtime[ch].alarm_flag = 1;
				}
			}
		}
	}
	free(buf);
	close(csock);
	close(sock);
}

/*
 * Admin protocol ?�이?��? broadcast ?�신?�다
 */
static void send_msgp(netconf_msg* conf)
{
	int len = 0;
	struct sockaddr_in sin;
#ifdef DUAL_LAN_NETWORK
	struct ifreq ifr;
#endif

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_BROADCAST;
	sin.sin_port = htons(ADMIN_CLI_PORT);

#ifdef DUAL_LAN_NETWORK
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HUB_ETH_DEV);
	if (setsockopt(send_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif

	len = sendto(send_sock, (void*) conf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

	if (len < 0)
	{
		char message[128] = {0};
		IPCAM_DBG(ERROR, "broadcast send failed\n");
		snprintf(message, 127, "[%s:%d] sendto", __FUNCTION__, __LINE__);
		perror(message);
	}

#ifdef DUAL_LAN_NETWORK
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HOST_ETH_DEV);
	if (setsockopt(send_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}

	len = sendto(send_sock, (void*) conf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

	if (len < 0)
	{
		char message[128] = {0};
		IPCAM_DBG(ERROR, "broadcast send eth1 failed\n");
		snprintf(message, 127, "[%s:%d] sendto", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif
}

/*
 * CCTV discovery 모듈 초기?? */
static void ipcam_init_discovery(void)
{
	int i = 0;
	int on = 1;
	struct sockaddr_in sin;

	IPCAM_DBG(MAJOR, "start\n");

	memset(discovery, 0x00, sizeof(AVAILABLE_MAX_CH*sizeof(dtable)));
	set_sub_network();

	runtime = get_runtime();
	queue = get_queue();
	while (queue == NULL)
	{
		usleep(500*1000);
		queue = get_queue();
	}
	memset((void*)rcvd_cur, 0x00, sizeof(rtbl) * 64);
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		IPCAM_DBG(MINOR, "CH(%d) MGMT STATE TRANS(0x%08x --> MGMT_STATE_UNLINKED)\n",
				i, runtime[i].state);
		runtime[i].state = MGMT_STATE_UNLINKED;
	}

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(ADMIN_SVR_PORT);

	rcv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(rcv_sock < 0)
	{
		IPCAM_DBG(ERROR, "rcv socket init failed\n");
		perror("recv socket");
		return ;
	}
	if (bind(rcv_sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "rcv socket bind failed\n");
		perror("recv bind");
		close(rcv_sock);
		rcv_sock = (-1);
		return ;
	}

	send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (send_sock < 0)
	{
		IPCAM_DBG(ERROR, "send socket init failed\n");
		perror("recv socket");
		close(rcv_sock);
		rcv_sock = (-1);
		return ;
	}
	struct timeval send_timeout;
	send_timeout.tv_sec = 5;
	send_timeout.tv_usec = 0;
	setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	setsockopt(send_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&send_timeout, sizeof(send_timeout));

	IPCAM_DBG(MAJOR, "end\n");
}

static void ipcam_stop_discovery(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	IPCAM_DBG(MAJOR, "end\n");
}

static void ipcam_start_discovery(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	IPCAM_DBG(MAJOR, "end\n");
}

/*
 * Switch link ?�태로�???discovery ?�태�??�정?�다
 */
static void ipcam_localport_hwscan(void)
{
	int i = 0;
	int layer, linked;
	int _max_ch = 4;

#if defined(GUI_32CH_SUPPORT)
	_max_ch = 16;
#elif defined(GUI_16CH_SUPPORT)
	_max_ch = 16;
#elif defined(GUI_8CH_SUPPORT)
	_max_ch = 8;
#elif defined(GUI_4CH_SUPPORT)
	_max_ch = 4;
#endif

	cur_link_status = 0;
	cur_delay_status = 0;
	cur_wait_status = 0;
	for (i = 0; i < _max_ch; i++)
	{
		/* Delayed job waiting - discovery pending state */
		if (runtime[i].state & MGMT_STATE_WAITING)
		{
			/* To prevent waiting permanently */
			if (discovery[i].loss_stat_cnt > 30)
			{
				memset((void*)&discovery[i], 0x00, sizeof(dtable));
			}
			else
			{
				cur_wait_status |= (1<<i);
				discovery[i].loss_stat_cnt += 1;
				continue;
			}
		}
		else
		{
			discovery[i].loss_stat_cnt = 0;
		}

		/*
		 * It may need to wait stable link state.
		 * Some configurations may affect to the link status(ex. IP assinging)
		 */
		if (discovery[i].polling_delay > 0)
		{
			cur_delay_status |= (1<<i);
			discovery[i].polling_delay -= 1;
			continue;
		}

		ipcam_disc_port_link_state(i, &layer, &linked);

		/* How's the i-th port's link status? */
		if (linked == 0)	// Unlinked
		{
			cur_link_status &= ~(1<<i);
			if (discovery[i].state != IPCAM_DISC_STATE_NONE)
			{
				printf("[%s] unlinked ch:[%d] layer:[%d]\n", __FUNCTION__, i, layer);

				/* Unlink job is enqueued immediately */
				nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
				memset(&discovery[i], 0x00, sizeof(dtable));
				nf_pnd_evt_notify_fire(i, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
				nf_ipcam_set_pnd_osd_status(i, PND_OSD_NONE);

                if(layer == IPCAM_DISC_LAYER_DVR)
				{
					struct mac_load_info _atu_info;
					int ret;

					_atu_info.port_num = i;
					ret = nf_dev_port_purge_all_mac_addr(_atu_info);

					if (!ret)
						printf("[%s][%d] Error : ioctl BOARD_PP_SMI_PURGE_ALL_PORT_MAC failed\n", __FUNCTION__, __LINE__);
				}
			}
		}
		else	// Linked
		{
			cur_link_status |= (1<<i);
			if (discovery[i].state == IPCAM_DISC_STATE_NONE)
			{
				/* State transition for the next port handling process */
				discovery[i].state = IPCAM_DISC_STATE_LINK;
				discovery[i].state_cnt = 0;
				discovery[i].layer = layer;
				nvs_subch_check_physical_link_on(i);

				printf("\e[31m [%s] linked ch:[%d] layer:[%d] layer:[%d]\e[0m\n", __FUNCTION__, i, layer, discovery[i].layer);
				nf_ipcam_set_pnd_osd_status(i, PND_OSD_DETECT);

                if(layer == IPCAM_DISC_LAYER_DVR)
				{
					struct mac_load_info _atu_info;
					int ret;

					_atu_info.port_num = i;
					ret = nf_dev_port_purge_all_mac_addr(_atu_info);

					if (!ret)
						printf("[%s][%d] Error : ioctl BOARD_PP_SMI_PURGE_ALL_PORT_MAC failed\n", __FUNCTION__, __LINE__);
				}
			}

			if (discovery[i].layer != layer) {
				if (discovery[i].state != IPCAM_DISC_STATE_NONE) {
					printf("[%s] layer changed ch:[%d] layer bef:[%d] aft:[%d]\n", __FUNCTION__, i, discovery[i].layer, layer);

					/* Unlink job is enqueued immediately */
					nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
					memset(&discovery[i], 0x00, sizeof(dtable));
					nf_pnd_evt_notify_fire(i, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
				}
			}
		}
	}
	//IPCAM_DBG(MINOR, "Current link(%08x) delayed(%08x) waiting(%08x)\n",
	//		cur_link_status, cur_delay_status, cur_wait_status);
	printf("[%s] Current link(%08x) delayed(%08x) waiting(%08x)\n",
			 __FUNCTION__, cur_link_status, cur_delay_status, cur_wait_status);
}

/*
 * Admin protocol??search 커맨?��? broadcast?�다.
 * ?�킷???�달받�? subnet??모든 카메?��? ?�답?�게 ?�다
 */
static void ipcam_itxcamera_swscan(void)
{
	int i = 0;
	int _new_link = 0;
	netconf_msg netconf;

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (discovery[i].state == IPCAM_DISC_STATE_LINK && discovery[i].layer > 0)
		{
			printf("[%s] ipsearch start ch(%d)\n", __FUNCTION__, i);

			_new_link++;
			discovery[i].state = IPCAM_DISC_STATE_IPSEARCH;
			discovery[i].state_cnt = 0;
			nf_pnd_evt_notify_fire(i, PND_TYPE_PLUGGED, __LINE__, __FILE__);
			nf_pnd_prog_notify_fire(i, 5, __LINE__, __FILE__);
		}
		if (discovery[i].state == IPCAM_DISC_STATE_IPSEARCH ||
			discovery[i].state == IPCAM_DISC_STATE_VNET ||
			discovery[i].state == IPCAM_DISC_STATE_IPDONE ||
			discovery[i].state == IPCAM_DISC_STATE_IPSET)
		{
			_new_link++;
		}
	}
	if (_new_link > 0)
	{
		discovery_check_set(23, -1);
		//rcvd_cnt = 0;
		//memset((void*)rcvd_cur, 0x00, sizeof(rtbl) * 64);
		memset((void*)&netconf, 0x00, sizeof(netconf_msg));
		netconf.version = 1;
		netconf.opcode = MSG_IP_SEARCH;
		netconf.magic = htonl(0x69547843);
		pthread_mutex_lock(&sendp_lock_mtx);
		send_msgp(&netconf);
		pthread_mutex_unlock(&sendp_lock_mtx);
	}
}

/*
 * Admin ?�로?�콜 search  ?�청???�?�응???�킷분석
 */
static void ipcam_analyze_netconf(void)
{
	int i, j, port_got;
	unsigned int iptoset;

	discovery_check_set(49, -1);
	pthread_mutex_lock(&recvp_lock_mtx);
	discovery_check_set(50, -1);
	for (i = 0; i < rcvd_cnt; i++)
	{
		port_got = (-1);
		for (j = 0; j < AVAILABLE_MAX_CH; j++)
		{
			if (memcmp(discovery[j].macaddr, rcvd_cur[i].macaddr, 6) == 0)
			{
				break;
			}
		}

		/* Registered item: don't care this item. */
		if (j < AVAILABLE_MAX_CH)
		{
			if (discovery[j].state < IPCAM_DISC_STATE_VNET ||
				discovery[j].state > IPCAM_DISC_STATE_IPDONE)
			{
				continue;
			}
		}

	discovery_check_set(51, i);
		if (nf_ipcam_is_vendor_zig())
		{
	discovery_check_set(52, i);
			/* Not registered item: find the port linked now */
			switch_mtx_lock();
	discovery_check_set(53, i);
			port_got = get_interrupt_port((unsigned char*)&rcvd_cur[i].macaddr[0]);
	discovery_check_set(54, i);
			switch_mtx_unlock();
	discovery_check_set(55, i);

			if (port_got < 0)
			{
				continue;
			}
			if ((cur_link_status & (1<<port_got)) == 0)
			{
				continue;
			}
		}
		else
		{
			/* Not registered item: find the port linked now */
	discovery_check_set(56, i);
			switch_mtx_lock();
	discovery_check_set(57, i);
			port_got = get_interrupt_port((unsigned char*)&rcvd_cur[i].macaddr[0]);
	discovery_check_set(58, i);

			switch_mtx_unlock();
	discovery_check_set(59, i);

			/* Linked port couldn't find from IPX local ports */
			if (port_got == -1)
			{
	discovery_check_set(60, i);
				/* Then from hub ports? */
				port_got = hub_find_port((unsigned char*)&rcvd_cur[i].macaddr[0]);
	discovery_check_set(61, i);
				if (port_got == -1)
				{
					continue;
				}
			}

			if ((cur_link_status & (1<<port_got)) == 0)
			{
				continue;
			}

			printf("[%s] j:[%d] layer:[%d]\n", __FUNCTION__, j, discovery[j].layer);
			printf("[%s] port_got:[%d]\n", __FUNCTION__, port_got);

			if(discovery[port_got].layer <= 0) {
				printf("[%s] ch(%d) discovery.layer:[%d] continue\n", __FUNCTION__, port_got, discovery[port_got].layer);
				continue;
			}
		}

		/* Found matching port */
		discovery[port_got].state = IPCAM_DISC_STATE_IPSET;
		discovery[port_got].state_cnt = 0;

	discovery_check_set(62, i);
		nf_pnd_evt_notify_fire(port_got, PND_TYPE_MAC_RESOLVED, __LINE__, __FILE__);
	discovery_check_set(63, i);
		nf_ipcam_set_pnd_osd_status(port_got, PND_OSD_IPREQUEST);
	discovery_check_set(64, i);

		/* Set mac address to the discovery table */
		memcpy(discovery[port_got].macaddr, rcvd_cur[i].macaddr, 6);
		IPCAM_DBG(MINOR, "mac address probed(%d: %02x:%02x:%02x:%02x:%02x:%02x)\n",
				port_got,
				rcvd_cur[i].macaddr[0], rcvd_cur[i].macaddr[1],
				rcvd_cur[i].macaddr[2], rcvd_cur[i].macaddr[3],
				rcvd_cur[i].macaddr[4], rcvd_cur[i].macaddr[5]);

		/* Set necessary entries to the discovery table */
	discovery_check_set(65, i);
		iptoset = get_available_ip(port_got);
	discovery_check_set(66, i);
		discovery[port_got].transaction = rcvd_cur[i].transaction;
		discovery[port_got].ipaddr = iptoset;
	}

	discovery_check_set(67, i);
	rcvd_cnt = 0;
	memset((void*)&rcvd_cur[0], 0x00, sizeof(rtbl)*64);
	pthread_mutex_unlock(&recvp_lock_mtx);
	discovery_check_set(68, i);
}

/*
 * Admin ?�로?�콜 IP set request ?�송
 */
static void ipcam_request_ipaddr(void)
{
	int i;
	netconf_msg ipsetmsg;

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (discovery[i].state == IPCAM_DISC_STATE_IPSET && discovery[i].layer > 0)
		{
			IPCAM_DBG(MINOR, "CH(%d) ip request(%d.%d.%d.%d) target mac[%02x:%02x:%02x:%02x:%02x:%02x]\n",
					i,
					(discovery[i].ipaddr&0xff),
					((discovery[i].ipaddr&0xff00)>>8),
					((discovery[i].ipaddr&0xff0000)>>16),
					((discovery[i].ipaddr&0xff000000)>>24),
					discovery[i].macaddr[0], discovery[i].macaddr[1], discovery[i].macaddr[2],
					discovery[i].macaddr[3], discovery[i].macaddr[4], discovery[i].macaddr[5]);

			/* Request to set the IP address to the IP Camera */
			memset((void*)&ipsetmsg, 0x00, sizeof(netconf_msg));

			ipsetmsg.version = 1;
			ipsetmsg.opcode = MSG_IP_SET;
			ipsetmsg.secs = 0;
			ipsetmsg.xid = htonl(discovery[i].transaction);
			ipsetmsg.magic = htonl(0x69547843);
			memcpy(&ipsetmsg.chaddr[0], &discovery[i].macaddr[0], 6);
			ipsetmsg.yiaddr = discovery[i].ipaddr;
			ipsetmsg.miaddr = inet_addr("255.255.255.0");
			ipsetmsg.giaddr = (discovery[i].ipaddr&inet_addr("255.255.255.0"))|inet_addr("0.0.0.1");
			//ipsetmsg.http_port = htons(80);
			//ipsetmsg.rtsp_port = htons(554);
			pthread_mutex_lock(&sendp_lock_mtx);
			send_msgp(&ipsetmsg);
			pthread_mutex_unlock(&sendp_lock_mtx);
			nf_pnd_evt_notify_fire(i, PND_TYPE_IP_REQUESTED, __LINE__, __FILE__);
			discovery[i].state = IPCAM_DISC_STATE_IPSETW;
			discovery[i].state_cnt = 0;
			set_switch_polling_delay(i, 5);
		}
	}
}

/*
 * CCTV 모드 plug and discovery??cycle ?�식?�로 timeout???�동?�다.
 * main_loop?�에 ?�의???�작??몇번반복?�는가???�해 ?�류, timeout과같?� 무응???�태가?�해진다
 * finalize_cycle?� ?�런 관?�에???�번??working cycle??종료?�었????discovery state 기반?�로
 * 발생?�야 ?�는 ?�벤?�들???�의?�어 ?�다.
 */
static void ipcam_finalize_cycle(void)
{
	int i, tmp, tmp_layer;
	int _pnd_recover_statecnt = 0;
	int _pnd_recover_retrycnt = 0;
	mtable *runtime = get_runtime();


	if (nf_ipcam_is_vendor_zig())
	{
		_pnd_recover_statecnt = 20;
		_pnd_recover_retrycnt = 1;
	}
	else
	{
		_pnd_recover_statecnt = 30;
		_pnd_recover_retrycnt = 2;
	}

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (discovery[i].polling_delay > 0) { continue; }
		if (runtime[i].state & MGMT_STATE_IDPASS_WAITING)
		{
			discovery[i].login_retry++;
			if (discovery[i].login_retry > 60)
			{
				runtime[i].state &= ~MGMT_STATE_IDPASS_WAITING;
				switch(runtime[i].sys.model_code)
				{
					case NF_IPCAM_MODEL_AMB_D1:
					case NF_IPCAM_MODEL_AMB_A2:
					case NF_IPCAM_MODEL_TI_368:
					if (queue != NULL)
					{
						nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
					}
					break;
					default:
					break;
				}
				discovery[i].login_retry = 0;
				nf_pnd_prog_notify_fire(i, 7, __LINE__, __FILE__);
			}
			continue;
		}

		discovery[i].state_cnt++;

		switch (discovery[i].state)
		{
			case IPCAM_DISC_STATE_IPSEARCH:
			case IPCAM_DISC_STATE_IPSET:
			case IPCAM_DISC_STATE_IPSETW:
			{
				if (discovery[i].state_cnt > _pnd_recover_statecnt)
				{
					IPCAM_DBG(MINOR, "CH(%d) recovered from(%s)\n",
							i, __DISCOVERY_STATE_STR[discovery[i].state]);
					tmp = discovery[i].retry_cnt;
					tmp_layer = discovery[i].layer;
					memset(&discovery[i], 0x00, sizeof(dtable));
					discovery[i].retry_cnt = ++tmp;
					discovery[i].layer = tmp_layer;

					if (discovery[i].retry_cnt > _pnd_recover_retrycnt)
					{
						printf("[%s] : DISCOVERY recovery case_0 CH(%d)\n", __func__, i);
						nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
						nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
					}
				}
				break;
			}
			case IPCAM_DISC_STATE_VNET:
			{
				if (discovery[i].state_cnt > 50)
				{
					IPCAM_DBG(MINOR, "Port(%d) recovered from(%s)\n",
							i, __DISCOVERY_STATE_STR[discovery[i].state]);
					discovery[i].state_cnt = 0;
					discovery[i].retry_cnt++;
					if (discovery[i].retry_cnt > 1)
					{
						printf("[%s] : DISCOVERY recovery case_1 CH(%d)\n", __func__, i);
						nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
						nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
					}
				}
				break;
			}
			case IPCAM_DISC_STATE_IPDONE:
			{
				if (discovery[i].state_cnt > 90)
				{
					IPCAM_DBG(MINOR, "Port(%d) recovered from(%s)\n",
							i, __DISCOVERY_STATE_STR[discovery[i].state]);
					discovery[i].state_cnt = 0;
					discovery[i].retry_cnt++;
					if (discovery[i].retry_cnt > 1)
					{
						printf("[%s] : DISCOVERY recovery case_1 CH(%d)\n", __func__, i);
						nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
						nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
					}
				}
				break;
			}
			case IPCAM_DISC_STATE_CAPA:
			{
				if (discovery[i].state_cnt > 10)
				{
					discovery[i].state_cnt = 0;
					discovery[i].retry_cnt++;
					if (discovery[i].retry_cnt > 5)
					{
						printf("[%s] : DISCOVERY recovery case_2 CH(%d) - capability query fail\n", __func__, i);
						nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
						nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
					}
				}
				break;
			}
			case IPCAM_DISC_STATE_DONE:
			{
				if (runtime[i].state == 0x101 && discovery[i].state_cnt > 450 && nf_ipcam_is_onvif_support(i) == 1)
				{
					printf("[%s] : DISCOVERY recovery case_3 CH(%d) - Onvif setup hang detected\n", __func__, i);
					nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
					nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
				}
				if (runtime[i].state == 0x101 && discovery[i].state_cnt > 200 && nf_ipcam_is_onvif_support(i) != 1)
				{
					printf("[%s] : DISCOVERY recovery case_3 CH(%d) - ITX setup hang detected\n", __func__, i);
					nf_ipcam_set_pnd_osd_status(i, PND_OSD_TIMEOUT);
					nf_pnd_queue_push(i, IPCAM_EVENT_UNKNOWN_DEVICE, __LINE__, __FILE__);
				}
				break;
			}
			case IPCAM_DISC_STATE_NONE:
			case IPCAM_DISC_STATE_LINK:
			default:
			break;
		}

	}
	printf("[%s] finalize end\n", __FUNCTION__);

	ipcam_unref_vnet();
}

static void nvs_video_handler(void)
{
	nvs_video_handler_itx();
}

static void PRINT_DTABLE(void)
{
	int i = 0;
	unsigned int ip = 0;
	unsigned int vloss_status = 0;
	unsigned int ipcam_vloss = 0;
	char ipstr[16];
	mtable *runtime = get_runtime();

	vloss_status = nf_notify_get_param0("vloss");
	ipcam_vloss = get_vloss_status();
	printf("=========================== PORT DISCOVERY STATE ===========================\n");
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (discovery[i].state == IPCAM_DISC_STATE_NONE) continue;
		snprintf(ipstr, 16, "%d.%d.%d.%d",
					(discovery[i].ipaddr&0xff),
					(discovery[i].ipaddr&0xff00)>>8,
					(discovery[i].ipaddr&0xff0000)>>16,
					(discovery[i].ipaddr&0xff000000)>>24);
		printf("CH(%d) %10s %s %s(%5d,%2d) vnet(%d) %15s runtime(%08x) %d\n",
				i,
				runtime[i].sys.model,
				runtime[i].sys.vendor,
				__DISCOVERY_STATE_STR[discovery[i].state],
				discovery[i].state_cnt,
				discovery[i].retry_cnt,
				discovery[i].vnet_id,
				ipstr,
				runtime[i].state,
				discovery[i].layer
		);
	}
	printf("================ CURRENT VLOSS STATE(%08x,%08x) ====================\n", vloss_status, ipcam_vloss);
}

/*
 * ?�트??poe  ?�원??강제 주입?�다.
 * ?�류 history - poe  ?�원??켜�?지 ?�는 카메?��? ?�었?? * PoE??HW 규격???�으�? link�?감�??�면 PoE ?�로?�콜 negociation???�게?�는??
 * nego가 ?�패?�는 경우 poe unlink?��? ?�으�??�원???�어?��? ?�게 ?�다.
 */
static void ipcam_power_supply(void)
{
	int i,j;
	int rtn;
	char key[64];
	GTimeVal now_time;
	GTimeVal ch_off_time;
	discovery_check_set(10, -1);

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		discovery_check_set(11, i);
		if (discovery[i].state == IPCAM_DISC_STATE_NONE)
		{
			discovery_check_set(12, i);
			snprintf(key,64,"cam.C%d.poe_on_off",i);
			if (nf_sysdb_get_bool(key))
			{
				discovery_check_set(13, i);
				g_get_current_time(&now_time);
				ch_off_time = nf_ipcam_get_poe_off_time(i);
				if (now_time.tv_sec - ch_off_time.tv_sec > 4)
				{
					discovery_check_set(14, i);
					#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
					if (i<NUM_ACTIVE_CH_DVR)
					#else
					if (i<8)
					#endif
					{
						discovery_check_set(15, i);
						nf_live_poe_port_onoff(i,1,&rtn,NF_LIVE_PSE_ACT_LOCAL);
						discovery_check_set(16, i);
						vhub_set_port_poe_on(i);
						discovery_check_set(17, i);
						rtn = 0;
					}
					else
					{
						discovery_check_set(18, i);
						vhub_set_port_poe_on(i);
						rtn = 0;
					}
				}
				else
				{
					discovery_check_set(19, i);
					rtn = 0;
				}
			}
			else 
			{
				discovery_check_set(20, i);
				rtn = 0;
			}

			usleep(30*1000);
		}
	}
}

#endif	//__NF_IPCAM_DISCOVERY__

