/*
 * ITX Security
 *  System software group
 *
 *  2011-12-12 jykim
 */

#ifndef __NF_IPCAM_DISCOVERY_ONVIF_C__
#define __NF_IPCAM_DISCOVERY_ONVIF_C__

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
#include <nf_ipcam_defs.h>
#include <nf_util_device.h>
#include <nf_util_netif.h>



#define REG_PORT_STATUS					(0)
#define REG_PORT_CONTROL				(4)
#define LINK_BIT_MASK					(0x0800)

#define _WS_DISC_PORT 					(3702)
#define _WS_DISC_CLI_PORT 				(3700)
#define _MCAST_GRP_ADDR					"239.255.255.250"
#define _MCAST_GRP_PORT					(ONVIF_VNET_MAX)
//#define _MSG_ID_FORMAT					"uuid:%08x-36c7-480e-9283-0000%08x"
#define _MSG_ID_FORMAT					"uuid:%s"

#define SOCK_BUF_SZ						(0x600)	// 1536
#define VENDOR_401						(4)	//get_dev_info 401 return vendor


#define xstr(s) str(s)
#define str(s) #s

#define ARP_CACHE       "/proc/net/arp"
#define ARP_STRING_LEN  1023
#define ARP_BUFFER_LEN  (ARP_STRING_LEN + 1)

#define ARP_LINE_FORMAT "%" xstr(ARP_STRING_LEN) "s %*s %*s " \
						"%" xstr(ARP_STRING_LEN) "s %*s " \
						"%" xstr(ARP_STRING_LEN) "s"


enum __FINDING_SVC_ADDR_RESULT_
{
	SVC_ERROR_NOADDR = 0,
	SVC_ERROR_HEAD,
	SVC_ERROR_ENDPOINT,
	SVC_ERROR_DELAY,
	SVC_OK_FOUND
};

typedef struct _RAW_DATA_RECV RAW_DATA;
struct _RAW_DATA_RECV
{
	unsigned char  dest_mac[6];
	unsigned char  src_mac[6];
	unsigned short eth_type;

	union {
		struct
		{
			unsigned int   ip_hlen:4;
			unsigned int   ip_ver:4;
			unsigned char  tos;
			unsigned short tot_len;
			unsigned short id;
			unsigned short frag_offset;
			unsigned char  ttl;
			unsigned char  proto;
			unsigned short ip_chksum;
			unsigned int   s_addr;
			unsigned int   d_addr;
			unsigned short udp_s_port;
			unsigned short udp_d_port;
			unsigned short udp_len;
			unsigned short udp_chksum;
		} __attribute__((__packed__)) ip;

		struct
		{
			unsigned short hw_type;
			unsigned short proto_type;
			unsigned char hw_size;
			unsigned char proto_size;
			unsigned short op;
			unsigned char sender_mac[6];
			unsigned int sender_ip;
			unsigned char target_mac[6];
			unsigned int target_ip;
		} __attribute__((__packed__)) arp;
	};

	unsigned char udp_data[2000];
}__attribute__((__packed__));


typedef struct _VNET_INFO_T__ VNET_INFO_T;
struct _VNET_INFO_T__
{
	int ref_cnt;
	unsigned int addr;
	int port;
};
static VNET_INFO_T _vnet_info[ONVIF_VNET_MAX];


static int multi_sock = (-1);
static int hikvision_sock = (-1);
static pthread_t th_arp;

static unsigned char _my_mac_addr[6];

static struct arp_table arp_pile[ARP_PILE_MAX];
static int arp_pile_index = 0;
static int arp_pile_using = 0;
static gint is_openmode = 0;

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

static const unsigned char* model_scope_401[VENDOR_401] ={
/*HikVision_KOBI*/	"NET",
/*HikVision		*/	"HIKVISION",
/*AXIS		   	*/	"DUMMY2",//"AXIS",
/*check		  	*/	"DUMMY1"
};
static void _arp_piling_func(void);
static void _add_arp_pile(RAW_DATA*);
static int  _init_multicast_sock(void);
static void _close_multicast_sock(void);
static void _send_onvif_discovery(int);
static void _onvif_parse_discovery(unsigned char*, struct sockaddr_in*);
static int  _find_service_tail(int, unsigned char*);
static int  _find_login_info(int);
//static int  _axis_discovery(int port);
static int _init_hikvision_sock(void);
static int _send_hikvision_discovery(int);
static char * _init_hikvision_data(char* );
static int _set_hikvision_dhcp(int, unsigned int);
static void _search_hikvision(void);

static void _arp_pile_search(void);
static void _try_onvif_discovery(void);
static int _get_wsd_ns(const char* msg, char* buf);
extern void ipcam_disc_port_link_state(int, int*, int*);



extern void cam_onvif_init(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	memset((void*)&_vnet_info[0], 0x00, sizeof(VNET_INFO_T) * ONVIF_VNET_MAX);
	if (nf_ipcam_support_static_ip_onvif_cam())
	{
		pthread_create(&th_arp, NULL, (void*)&_arp_piling_func, NULL);
		pthread_detach(th_arp);
	}
	_init_multicast_sock();

	//Hikvision_CAM : CCTV mode raw sock setting
	{
		is_openmode = nf_sysdb_get_bool("cam.install.mode"); 

		if(is_openmode == 0){
			_init_hikvision_sock();
		}
	}
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_ipcam_onvif_start(void)
{
	int rtn;
	IPCAM_DBG(MAJOR, "start\n");
	rtn = _init_multicast_sock();
	rtn =_init_hikvision_sock();
	IPCAM_DBG(MAJOR, "end\n");
}

extern void nf_ipcam_onvif_stop(void)
{
	IPCAM_DBG(MAJOR, "start\n");
	_close_multicast_sock();
	IPCAM_DBG(MAJOR, "end\n");
}

extern int arp_pile_try_lock(void)
{
	if (arp_pile_using != 0)
	{
		return 0;
	}
	arp_pile_using = 1;
	return 1;
}

extern void arp_pile_unlock(void)
{
	arp_pile_using = 0;
}

extern struct arp_table* get_arp_pile(void)
{
	return (arp_pile);
}

extern int nf_ipcam_get_msock(void)
{
	return multi_sock;
}

extern void nf_onvif_discovery_handler(char* buf, struct sockaddr* cin)
{
	_onvif_parse_discovery(buf, cin);
}

extern void ipcam_onvif_search(void)
{
	int i = 0;
	int ret = 0;
	dtable *discovery = get_dtable();
	mtable *runtime = get_runtime();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].state & MGMT_STATE_IDPASS_WAITING)
		{
			continue;
		}
		if (discovery[i].polling_delay > 0)
		{
			continue;
		}
		if (discovery[i].state == IPCAM_DISC_STATE_IPSEARCH)
		{
			if(is_openmode == 0){_send_hikvision_discovery(hikvision_sock);}

			break;
		}
		if (discovery[i].state == IPCAM_DISC_STATE_VNET)
		{
			break;
		}
		if (discovery[i].state == IPCAM_DISC_STATE_IPDONE)
		{
			break;
		}
	}
	if (i >= AVAILABLE_MAX_CH)
	{
		return;
	}
	IPCAM_DBG(MINOR, "ONVIF discovery start\n");
	if (nf_ipcam_support_static_ip_onvif_cam())
	{
		_arp_pile_search();
		if(is_openmode == 0){_search_hikvision();}
	}
	_try_onvif_discovery();
}

extern void ipcam_onvif_set(void)
{
	int i = 0;
	int temp_rtn = 0;
	dtable *discovery = get_dtable();
	mtable *runtime = get_runtime();
	GAsyncQueue *queue = get_queue();

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].state & MGMT_STATE_IDPASS_WAITING)
		{
			continue;
		}
		if (discovery[i].polling_delay > 0)
		{
			continue;
		}
		if (discovery[i].state == IPCAM_DISC_STATE_CAPA)	//discovery[i].layer > 0)
		{
			IPCAM_DBG(MINOR, "get service capabilities(CH:%d)\n", i);
			temp_rtn = nf_onvif_get_service_capabilities(i);
			if (temp_rtn != 0)
			{
				gchar key_u[64];
				gchar key_p[64];
				gchar *u,*p;
				snprintf(key_u, 64, "cam.logininfo.L%d.id", i);
				snprintf(key_p, 64, "cam.logininfo.L%d.pwd", i);
				u = nf_sysdb_get_str_nocopy(key_u);
				p = nf_sysdb_get_str_nocopy(key_p);
				strncpy(runtime[i].username, u, 64);
				strncpy(runtime[i].password, p, 64);

				runtime[i].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
				temp_rtn = nf_onvif_get_service_capabilities(i);
			}
			if (temp_rtn != 0)
			{
				if (runtime[i].onvif.auth_method == NF_ONVIF_AUTH_TEXT)
				{
					runtime[i].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
					temp_rtn = nf_onvif_get_service_capabilities(i);
				}
			}
			if (temp_rtn != 0)
			{
				nf_pnd_queue_push(i, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
				nf_eventlog_put_ipcam_msg("Get Device Capability Fail", i);
				continue;
			}

			IPCAM_DBG(MINOR, "get profile token to stream(CH:%d)\n", i);
			temp_rtn = nf_onvif_get_appropriate_profile(i);
			if (temp_rtn != 0)
			{
				gchar key_u[64];
				gchar key_p[64];
				gchar *u,*p;
				snprintf(key_u, 64, "cam.logininfo.L%d.id", i);
				snprintf(key_p, 64, "cam.logininfo.L%d.pwd", i);
				u = nf_sysdb_get_str_nocopy(key_u);
				p = nf_sysdb_get_str_nocopy(key_p);
				strncpy(runtime[i].username, u, 64);
				strncpy(runtime[i].password, p, 64);

				runtime[i].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
				temp_rtn = nf_onvif_get_appropriate_profile(i);
			}
			if (temp_rtn != 0)
			{
				if (runtime[i].onvif.auth_method == NF_ONVIF_AUTH_TEXT)
				{
					runtime[i].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
					temp_rtn = nf_onvif_get_appropriate_profile(i);
				}
			}
			if (temp_rtn != 0)
			{
				nf_pnd_queue_push(i, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
				continue;
			}

			discovery[i].state = IPCAM_DISC_STATE_DONE;
			discovery[i].state_cnt = 0;

			{
				runtime[i].state = (MGMT_STATE_LINKED|MGMT_STATE_READY);
				nf_pnd_queue_push(i, IPCAM_EVENT_ONVIF_GENERAL_READY, __LINE__, __FILE__);
			}
		}
	}
}

extern void ipcam_unref_vnet(void)
{
	int i = 0, j = 0;
	unsigned int h,m;
	char dev_name[16];
	char sbuf[256];
	dtable *discovery = get_dtable();

	g_return_if_fail(discovery != NULL);
	for (i = 2; i < ONVIF_VNET_MAX; i++)
	{
		snprintf(dev_name, 16, "%s:%d", HUB_ETH_DEV, i);
		h = get_netif_ip(dev_name);
		m = get_netif_mask(dev_name);
		if (h==0 || m==0)
		{
			continue;
		}

		for (j = 0; j < AVAILABLE_MAX_CH; j++)
		{
			if (discovery[j].vnet_id == i) { break; }
		}
		if (j >= AVAILABLE_MAX_CH)
		{
			snprintf(sbuf, 256, "ifconfig %s:%d down", HUB_ETH_DEV, i);
			IPCAM_DBG(MINOR, "%s\n", sbuf);
			proxy_system(sbuf, 1, 3);
		}
	}

#ifdef DUAL_LAN_NETWORK
	for (i = 2; i < ONVIF_VNET_MAX; i++)
	{
		snprintf(dev_name, 16, "%s:%d", HOST_ETH_DEV, i);
		h = get_netif_ip(dev_name);
		m = get_netif_mask(dev_name);
		if (h==0 || m==0)
		{
			continue;
		}

		for (j = 0; j < AVAILABLE_MAX_CH; j++)
		{
			if (discovery[j].vnet_id == i) { break; }
		}
		if (j >= AVAILABLE_MAX_CH)
		{
			snprintf(sbuf, 256, "ifconfig %s:%d down", HOST_ETH_DEV, i);
			IPCAM_DBG(MINOR, "%s\n", sbuf);
			proxy_system(sbuf, 1, 3);
		}
	}
#endif
}



static void _arp_pile_search(void)
{
	int i = 0;
	int j = 0;
	int port = 0;
	int port_hub = 0;
	int layer, linked;
	unsigned int h, m, s, d;
	char dev_name[16];
	struct arp_table *_arp_pile;
	struct arp_table arp_pile;
	dtable *discovery = NULL;
	mtable *runtime = NULL;

	discovery = get_dtable();
	runtime = get_runtime();
	_arp_pile = get_arp_pile();

	for (i = 0; i < ARP_PILE_MAX; i++)
	{
		for (j = 0; j < 10; j++) {
			if (arp_pile_try_lock())
				break;

			usleep(1000*100);
		}

		memcpy(&arp_pile, &_arp_pile[i], sizeof(struct arp_table));
		memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
		arp_pile_unlock();

		if (arp_pile.ip == 0)
		{
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
			continue;
		}
		switch_mtx_lock();
		port = get_interrupt_port((char*)&arp_pile.mac[0]);
		switch_mtx_unlock();

		port_hub = hub_find_port((char*)&arp_pile.mac[0]);

        //use port
		if (port >= 0 && (discovery[port].layer == IPCAM_DISC_LAYER_DVR))
		{
		}
        //use hub port
        else if(port_hub >= 0 && (discovery[port_hub].layer == IPCAM_DISC_LAYER_VHUB))
        {
			port = port_hub;
        }
        else
        {
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
			continue;
        }

		if (discovery[port].state >= IPCAM_DISC_STATE_IPSET &&
			discovery[port].state <= IPCAM_DISC_STATE_DONE)
		{
			if(discovery[port].ipaddr == arp_pile.ip)
				continue;
		}

		//		if(discovery[port].layer <= 0) continue;

		ipcam_disc_port_link_state(port, &layer, &linked);
		if (!linked)
			continue;

		s = arp_pile.ip;

		/* Compare with eth0 */
		h = get_netif_ip(HUB_ETH_DEV);
		m = get_netif_mask(HUB_ETH_DEV);
		if (h==0 || m==0)
		{
			IPCAM_DBG(MINOR, "Pass eth0 comparison(CH(%d))\n", port);
		}
#ifdef DUAL_LAN_NETWORK
		else if ((h&m)==(s&m)&& (discovery[port].layer == IPCAM_DISC_LAYER_DVR))
#else
		else if ((h&m)==(s&m))
#endif

		{
			IPCAM_DBG(MINOR, "No need vnet creation(CH(%d)-eth0)\n", port);
			discovery[port].ipaddr = s;
			memcpy(discovery[port].macaddr, arp_pile.mac, 6);
			discovery[port].vnet_id = ONVIF_VNET_MAX;
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
			discovery[port].state = IPCAM_DISC_STATE_VNET;
			discovery[port].state_cnt = 0;
			continue;
		}

		/* Compare with eth0:0 ~ eth0:15 */
		for (j = 0; j < ONVIF_VNET_MAX; j++)
		{
			snprintf(dev_name, 16, "%s:%d", HUB_ETH_DEV, j);
			h = get_netif_ip(dev_name);
			m = get_netif_mask(dev_name);
			if (h==0 || m==0)
			{	
				continue;
			}
#ifdef DUAL_LAN_NETWORK
			if ((h&m)==(s&m) && (discovery[port].layer == IPCAM_DISC_LAYER_DVR))
#else
			if ((h&m)==(s&m))
#endif
			{
				IPCAM_DBG(MINOR, "No need vnet creation(%d-eth0:%d)\n", port, j);
				discovery[port].ipaddr = s;
				memcpy(discovery[port].macaddr, arp_pile.mac, 6);
				discovery[port].vnet_id = j;
				memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
				discovery[port].state = IPCAM_DISC_STATE_VNET;
				discovery[port].state_cnt = 0;
				break;
			}
#ifdef DUAL_LAN_NETWORK
			h = get_netif_ip(HOST_ETH_DEV);
			if ((h&m)==(s&m) && (discovery[port].layer == IPCAM_DISC_LAYER_VHUB))
			{
				IPCAM_DBG(MINOR, "No need vnet creation(%d-eth1:%d)\n", port, j);
				discovery[port].ipaddr = s;
				memcpy(discovery[port].macaddr, arp_pile.mac, 6);
				discovery[port].vnet_id = j;
				memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
				discovery[port].state = IPCAM_DISC_STATE_VNET;
				discovery[port].state_cnt = 0;
				break;
			}
#endif
		}

		if (j < ONVIF_VNET_MAX)
		{
			if (j < 2)
			{
				discovery[port].state = IPCAM_DISC_STATE_IPDONE;
				discovery[port].state_cnt = 0;
			}
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
			continue;
		}

		/* Create virtual network */
		for (j = 2; j < ONVIF_VNET_MAX; j++)
		{
			snprintf(dev_name, 16, "%s:%d", HUB_ETH_DEV, j);
#ifdef DUAL_LAN_NETWORK
			if(discovery[port].layer == IPCAM_DISC_LAYER_VHUB)
				snprintf(dev_name, 16, "%s:%d", HOST_ETH_DEV, j);
#endif
			h = get_netif_ip(dev_name);
			if (h == 0) { break; }
		}
		if (j < ONVIF_VNET_MAX)
		{
			if(discovery[port].state < IPCAM_DISC_STATE_IPDONE)
			{
			char temp_sys_cmd[256];
			int ip_byte = 1;

			if ((s&0xff000000)>>24 == 1)
			{
				ip_byte = 2;
			}
			snprintf(temp_sys_cmd, 256,
					"ifconfig %s:%d %d.%d.%d.%d netmask 255.255.255.0",
					HUB_ETH_DEV, j, (s&0xff), (s&0xff00)>>8, (s&0xff0000)>>16, ip_byte);
#ifdef DUAL_LAN_NETWORK
			if(discovery[port].layer == IPCAM_DISC_LAYER_VHUB)
				snprintf(temp_sys_cmd, 256, "ifconfig %s:%d %d.%d.%d.%d netmask 255.255.255.0", HOST_ETH_DEV, j, (s&0xff), (s&0xff00)>>8, (s&0xff0000)>>16, ip_byte);
#endif
			IPCAM_DBG(MINOR, "Vnet creation(CH(%d)-eth0:%d)\n", port, j);
			IPCAM_DBG(MINOR, "%s\n", temp_sys_cmd);
			proxy_system(temp_sys_cmd, 1, 3);
			discovery[port].ipaddr = s;
			memcpy(discovery[port].macaddr, arp_pile.mac, 6);
			discovery[port].vnet_id = j;
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
			discovery[port].state = IPCAM_DISC_STATE_VNET;
			discovery[port].state_cnt = 0;
		}
		}
		else
		{
			IPCAM_DBG(MINOR, "No more vnet creation\n");
			memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
		}
		memset(&_arp_pile[i], 0x00, sizeof(struct arp_table));
	}
	//arp_pile_unlock();
}

static void _try_onvif_discovery(void)
{
	int i = 0;
	int len = 0;
	int buf_sz = 8192;
	char *buf = NULL;
	struct sockaddr_in cin;
	int cin_len;
	dtable *discovery = NULL;

	//IPCAM_DBG(MAJOR, "start\n");

	discovery = get_dtable();
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "multicast socket init failed\n");
		return;
	}
	_send_onvif_discovery(_MCAST_GRP_PORT);
#if 0
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (discovery[i].state >= IPCAM_DISC_STATE_VNET &&
			discovery[i].state <= IPCAM_DISC_STATE_IPDONE)
		{
			IPCAM_DBG(MINOR, "send(%d)\n", i);
			_send_onvif_discovery(i);
		}
	}
#endif
}


static void _arp_piling_func(void)
{
	int i = 0;
	int j = 0;
	int switching = 0;
	int _raw_sock = (-1);
	struct sockaddr_in sin;
	RAW_DATA buf;

	unsigned int hik_sender_ip[2] = { 1073742016, 1073850560 }; 
	unsigned int hik_target_ip = 1;

	IPCAM_DBG(MAJOR, "start\n");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(0);

	if(is_openmode == 0)
		_raw_sock = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ALL));
	else 
	_raw_sock = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ARP));

	if(_raw_sock < 0)
	{
		IPCAM_DBG(MINOR, "arp_sock failed\n");
		perror("socket");
		return;
	}

	while (1)
	{
		if (get_running_state() == DISCOVERY_STOPPED)
		{
			sleep(1);
			continue;
		}

		//memset(&buf, 0x00, sizeof(RAW_DATA));
		i = recvfrom(_raw_sock, &buf, sizeof(RAW_DATA), MSG_DONTWAIT, NULL, NULL);
		//i=(-1);

		if (i <= 0)
		{
			usleep(10*1000);
			continue;
		}
		//NORMAL ARP PACKET IN
		if (ntohs(buf.eth_type) == 0x806)
		{
			_add_arp_pile(&buf);
			usleep(10*1000);
		}

		//FIXME Hikvision disc return packet(eth.type == 0x8033)
		if (ntohs(buf.eth_type) == 0x8033)
		{
			//Hikvision FAKE arp frame : build
			{
				memcpy(buf.arp.sender_mac, buf.src_mac, 6);
				memcpy(buf.arp.target_mac, buf.dest_mac, 6);
				buf.arp.sender_ip = hik_sender_ip[switching];
				buf.arp.target_ip = hik_target_ip;

				buf.eth_type = ntohs(ETH_P_ARP);//arp type change

				if(switching == 0)
					switching = 1;
				else
					switching = 0;
			}
			_add_arp_pile(&buf);
			usleep(10*1000);
		}
	}
	close(_raw_sock);
	IPCAM_DBG(MAJOR, "end\n");
}

static int _search_ip_from_arp_pile(uint32_t ip)
{
	int i = 0;
	for (i = 0; i < ARP_PILE_MAX; i++)
	{
		if (arp_pile[i].ip == ip)
			break;
	}

	if(i < ARP_PILE_MAX)
		return 1;
	else
		return 0;
}

static int _search_and_add_ip_from_arp_table(const uint32_t ip)
{
	FILE *arpCache = fopen(ARP_CACHE, "r");
	if (!arpCache)
	{
		perror("Arp Cache: Failed to open file \"" ARP_CACHE "\"");
		return 1;
	}

	char header[ARP_BUFFER_LEN];
	if (!fgets(header, sizeof(header), arpCache))
	{
		return 1;
	}

	mtable *runtime = get_runtime();

	int rtn = 0;
	int i = 0;
	uint32_t arp_ip = 0;
	uint32_t hwAddrs[6];
	char ipAddr[ARP_BUFFER_LEN], hwAddr[ARP_BUFFER_LEN], device[ARP_BUFFER_LEN];
	int count = 0;
	while (3 == fscanf(arpCache, ARP_LINE_FORMAT, ipAddr, hwAddr, device))
	{
		inet_pton(AF_INET, ipAddr, &arp_ip);
		sscanf(hwAddr, "%02X:%02X:%02X:%02X:%02X:%02X", &hwAddrs[0], &hwAddrs[1], &hwAddrs[2], &hwAddrs[3], &hwAddrs[4], &hwAddrs[5]);
		if(arp_ip == ip)
		{
			sscanf(hwAddr, "%02X:%02X:%02X:%02X:%02X:%02X", &hwAddrs[0], &hwAddrs[1], &hwAddrs[2], &hwAddrs[3], &hwAddrs[4], &hwAddrs[5]);

			// Lock
			for (i = 0; i < 10; i++) 
			{
				if (arp_pile_try_lock())
					break;

				usleep(1000*100);
			}

			// Add ARP
			arp_pile[arp_pile_index].mac[0] = (char) hwAddrs[0];
			arp_pile[arp_pile_index].mac[1] = (char) hwAddrs[1];
			arp_pile[arp_pile_index].mac[2] = (char) hwAddrs[2];
			arp_pile[arp_pile_index].mac[3] = (char) hwAddrs[3];
			arp_pile[arp_pile_index].mac[4] = (char) hwAddrs[4];
			arp_pile[arp_pile_index].mac[5] = (char) hwAddrs[5];
			arp_pile[arp_pile_index].ip = arp_ip;

			arp_pile_index = (arp_pile_index + 1) % ARP_PILE_MAX;
			arp_pile_unlock();

			rtn = 1;
			break;
		}
	}

	fclose(arpCache);

	return rtn;
}


static void _add_arp_pile(RAW_DATA *data)
{
	int print_gru = 0;
	int i;
	mtable *runtime = get_runtime();


	//if (print_gru) printf("[\033[1;49;34m%s\033[0m] start\n", __FUNCTION__);
	/* ARP packet from me */
	nf_netif_get_mac_str(&_my_mac_addr[0]);
	if (memcmp(&data->src_mac[0], _my_mac_addr, 6) == 0)
	{
		//if (print_gru) printf("[\033[1;49;34m%s\033[0m] ARP from me\n", __FUNCTION__);
		return;
	}

	/* Same packet piled already and unprocessed */
	for (i=0; i<ARP_PILE_MAX; i++)
	{
		if (memcmp(arp_pile[i].mac, data->src_mac, 6) == 0 && arp_pile[i].ip == data->arp.sender_ip)
		{
			return;
		}
	}

	/* ARP packet from already known device */
	for (i=0; i<NUM_ACTIVE_CH; i++)
	{
		if ((runtime[i].state &
				(MGMT_STATE_CONFIGURED|MGMT_STATE_READY|MGMT_STATE_USING)) == 0)
		{
			continue;
		}
		if (memcmp((unsigned char*)&data->src_mac[0], &runtime[i].sys.macaddr[0], 6) == 0)
		{
			break;
		}
	}
	if (i < NUM_ACTIVE_CH)
	{
		//if (print_gru) printf("[\033[1;49;34m%s\033[0m] ARP from known device\n", __FUNCTION__);
		return;
	}

	//if (print_gru) printf("[\033[1;49;34m%s\033[0m] ADD arp\n", __FUNCTION__);
	for (i = 0; i < 10; i++) {
		if (arp_pile_try_lock())
			break;

		usleep(1000*100);
	}
	memcpy(arp_pile[arp_pile_index].mac, data->src_mac, 6);
	arp_pile[arp_pile_index].ip = data->arp.sender_ip;
	arp_pile_index = (arp_pile_index + 1) % ARP_PILE_MAX;
	arp_pile_unlock();
}

static int _init_multicast_sock(void)
{
	const char ttl = 1;
	struct sockaddr_in sin_m;

	if (multi_sock > 0)
	{
		return multi_sock;
	}

	memset(&sin_m, 0x00, sizeof(sin_m));
	sin_m.sin_family = AF_INET;
	sin_m.sin_addr.s_addr = INADDR_ANY;
	sin_m.sin_port = htons(_WS_DISC_CLI_PORT);
	multi_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (multi_sock < 0)
	{
		IPCAM_DBG(ERROR, "multicast rcv socket init failed\n");
		perror("recv socket");
		return (-1);
	}
	if (bind(multi_sock, (struct sockaddr*) &sin_m, sizeof(sin_m)) < 0)
	{
		IPCAM_DBG(ERROR, "multicast rcv socket bind failed\n");
		perror("recv bind");
		close(multi_sock);
		multi_sock = (-1);
		return (-1);
	}
	//setsockopt(multi_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*) &ttl, sizeof(ttl));

	return multi_sock;
}

static void _close_multicast_sock(void)
{
	if (multi_sock > 0)
	{
		close(multi_sock);
		multi_sock = (-1);
	}
}


static void _send_onvif_discovery(int port)
{
	char buf[2048];
	int cin_len, n;

	char msg_id[64];
	char uuid_buf[40];

	struct sockaddr_in cin;
	dtable *discovery = NULL;

#ifdef DUAL_LAN_NETWORK
	struct ifreq ifr;
#endif

	IPCAM_DBG(MAJOR, "start CH(%d)\n", port);

	srand( (unsigned)time( NULL ) );
	discovery = get_dtable();

#if 1
	if (port == _MCAST_GRP_PORT)
	{
		if (multi_sock <= 0)
		{
			IPCAM_DBG(ERROR, "multi-sock not inited yet\n");
			return;
		}

		memset(&cin, 0x00, sizeof(cin));
		cin.sin_family = AF_INET;
		cin.sin_addr.s_addr = inet_addr(_MCAST_GRP_ADDR);
		cin.sin_port = htons(_WS_DISC_PORT);
		cin_len = sizeof(cin);
	}
	else
#endif
	{
		if (multi_sock <= 0)
		{
			IPCAM_DBG(ERROR, "multi-sock not inited yet\n");
			return;
		}

		memset(&cin, 0x00, sizeof(cin));
		cin.sin_family = AF_INET;
		cin.sin_addr.s_addr = discovery[port].ipaddr;
		cin.sin_port = htons(_WS_DISC_PORT);
		cin_len = sizeof(cin);
	}

	memset(msg_id, 0x00, 64);
	memset(uuid_buf, 0x00, sizeof(uuid_buf));
	nf_ipcam_get_sysproc_uuid(uuid_buf);
	snprintf(msg_id, 64, _MSG_ID_FORMAT, uuid_buf);
	memset(buf, 0x00, 2048);
	snprintf(buf, 2048, discovery_msg, msg_id);

#ifdef DUAL_LAN_NETWORK
	// SEND SWITCH PORT
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HUB_ETH_DEV);
	if (setsockopt(multi_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif
	n = sendto(multi_sock, buf, strlen(buf), 0, (struct sockaddr*)&cin, cin_len);

	if (n < 0)
	{
		IPCAM_DBG(ERROR, "discovery send failed\n");
		perror("sendto");
	}

#ifdef DUAL_LAN_NETWORK
	// SEND HUB PORT
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HOST_ETH_DEV);
	if (setsockopt(multi_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}

	n = sendto(multi_sock, buf, strlen(buf), 0, (struct sockaddr*)&cin, cin_len);

	if (n < 0)
	{
		char message[128] = {0};
		IPCAM_DBG(ERROR, "broadcast send eth1 failed\n");
		snprintf(message, 127, "[%s:%d] sendto", __FUNCTION__, __LINE__);
		perror(message);
	}

	// RESET SO_BINDTODEVICE
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "");
	if (setsockopt(multi_sock, SOL_SOCKET, SO_BINDTODEVICE, NULL, 0) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif

	return ;
}

static int _is_itx_mac_range(unsigned char* dev_mac)
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

	int rtn = 0;
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
			rtn = 1;
			break;
		}
	}

	return (rtn);
}

static void _onvif_parse_discovery(unsigned char* msg, struct sockaddr_in* cin)
{
	int i, j;
	int port = (-1);
	int rtn;
	int rtn_stail;
	int rtn_login;
	int rtn_dev = (-1);
	char wsd_ns[32];
	char scopes_ns[64];
	unsigned char *s,*s1, *e, *p1, *p2, *p3;
	const char find_type[] = "NetworkVideoTransmitter";
	const char find_model[] = "onvif://www.onvif.org/hardware/";
	const char find_scope[] = "</%s:Scopes>";
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	GAsyncQueue *queue = get_queue();


	//IPCAM_DBG(MAJOR, "start\n");
	//IPCAM_DBG(MINOR, "MSG parsing\n%s\n", msg);

	memset(wsd_ns, 0x00, 32);
	memset(scopes_ns, 0x00, 64);
	if (_get_wsd_ns(msg, wsd_ns) != 1)
	{
		IPCAM_DBG(WARN, "WS-Discovery namespace not found\n");
		return;
	}
	snprintf(scopes_ns, 64, find_scope, wsd_ns);

	if (cin->sin_addr.s_addr == 0)
	{
		//printf("%s [%s] Sender ip is 0\n", CAM_LOG_DOMAIN, __FUNCTION__);
		return;
	}
	if (msg != NULL && strstr(msg, "onvif://www.onvif.org/custom/hdnname") != NULL)
	{
		return;
	}

	/* find Port */
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		//if (cin->sin_addr.s_addr != runtime[i].sys.ipaddr)
		if (cin->sin_addr.s_addr != discovery[i].ipaddr)
		{
			continue;
		}

		port = i;
		break;
	}
	if (port < 0)
	{
		unsigned int h, m;
		h = get_netif_ip(LOCAL_ETH_DEVICE);
		m = get_netif_mask(LOCAL_ETH_DEVICE);

		if((h & m) == (cin->sin_addr.s_addr & m))
		{
			if(_search_ip_from_arp_pile(cin->sin_addr.s_addr) == 0)
			{
				if(_search_and_add_ip_from_arp_table(cin->sin_addr.s_addr) != 1)
				{
					printf("Unknown Device Discovery IP, maybe from wan\n");
				}
			}
		}
#if 0
		IPCAM_DBG(WARN, "port matching. Sender(%d.%d.%d.%d)\n",
				cin->sin_addr.s_addr&0xff,
				(cin->sin_addr.s_addr&0xff00)>>8,
				(cin->sin_addr.s_addr&0xff0000)>>16,
				(cin->sin_addr.s_addr&0xff000000)>>24
				);
#endif
		return;
	}

	if (discovery[port].state != IPCAM_DISC_STATE_IPDONE &&
		discovery[port].state != IPCAM_DISC_STATE_VNET)
	{
		return;
	}

	if(discovery[port].layer <= 0) return;
	
	if (runtime[port].state & MGMT_STATE_IDPASS_WAITING)
	{
		return;
	}

	/* find NetworkVideoTransmitter */
	s = strstr(msg, find_type);
	if (s == NULL)
	{
		IPCAM_DBG(ERROR, "NetworkVideoTransmitter not found\n");
		return;
	}

	/* find Model Name */
	s = strstr(msg, find_model);
	if (s == NULL)
	{
		IPCAM_DBG(ERROR, "model not found\n");
		return;
	}
	s += strlen(find_model);
	p1 = strstr(s, " ");
	p2 = strstr(s, scopes_ns);
	if (p1 == NULL)
	{
		if (p2 == NULL)
		{
			IPCAM_DBG(ERROR, "model end point\n");
			return;
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
	memset(runtime[port].sys.model, 0x00, 64);
	memcpy(runtime[port].sys.model, s, e-s);
	IPCAM_DBG(MINOR, "Model(%s)\n", runtime[port].sys.model);
	/* Don't handle NCx by ONVIF */

	if (_is_itx_mac_range(discovery[port].macaddr))
	{
		IPCAM_DBG(WARN, "CH(%d): itx mac range discovery\n", port);
		return;
	}
	if (_is_itx_mac_range(runtime[port].sys.macaddr))
	{
		IPCAM_DBG(WARN, "CH(%d): itx mac range\n", port);
		return;
	}
	//if no problem remove this code
	memset(runtime[port].username, 0x00, sizeof(runtime[port].username));
	memset(runtime[port].password, 0x00, sizeof(runtime[port].password));
	runtime[port].onvif.auth_method = NF_ONVIF_AUTH_NONE;

	/* find Service Address Tail */
	rtn_stail = _find_service_tail(port, msg);
	/* find Login Info */
	//rtn_login = _find_login_info(port);

#if 0
	rtn_dev = nf_onvif_get_dev_info(port);

	if (rtn_dev != 0)
	{
		if (runtime[port].onvif.auth_method == NF_ONVIF_AUTH_TEXT)
		{
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
			rtn_dev = nf_onvif_get_dev_info(port);
		}
	}
	if (rtn_dev == 28) // ITX cam connection fail
	{
		IPCAM_DBG(WARN, "CH(%d): Better to handle by itx protocol(CH(%d))\n", __LINE__, port);
		return;
	}
	else if (rtn_dev != 0)
	{
		nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);

		return;
	}
#endif

	/* This error may caused by the IPCAMERA bug(ONVIF or Network Configuration) */
	if (rtn_stail == SVC_ERROR_HEAD)
	{
		nf_ipcam_poe_reboot(port, NULL, NULL, NULL);
		//nf_onvif_reboot_request(port);
		return;
	}

	//sony exception code // ex) checker[port] > 4 case is poe_reboot
	if(rtn_stail == SVC_ERROR_DELAY)
	{
		return;
	}

	//get_dev_info return 401 error msg vendor managed routine 
	{
		unsigned char *temp_str1 = NULL;
		unsigned char *temp_str2 = NULL;
		temp_str1= strstr(msg, "Scopes");
		int l = 0;

		memset(runtime[port].username, 0x00, sizeof(runtime[port].username));
		memset(runtime[port].password, 0x00, sizeof(runtime[port].password));

		if(temp_str1 != NULL)
		{
			//FIXME KOBI : hikvision cam auto login exception code 
			for(l = 0; l < VENDOR_401; l++)
			{
				temp_str2 = strstr(temp_str1, model_scope_401[l]); 
				if(temp_str2 != NULL)
				{
					break;
				}
			}
			//here is get_dev_info 401 cam
			if(l == 0)	//HikVision OLD
			{
				strncpy(runtime[port].username, "admin", 64);
				strncpy(runtime[port].password, "12345", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
#ifdef DUAL_LAN_NETWORK
				rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
				rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
			}
			else if(l == 1)	//HikVision NEW
			{
				strncpy(runtime[port].username, "admin", 64);
				strncpy(runtime[port].password, "admin12345", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
#ifdef DUAL_LAN_NETWORK
				rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
				rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
			}
			else if(l == 2)	//AXIS
			{
				strncpy(runtime[port].username, "root", 64);
				strncpy(runtime[port].password, "pass", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
#ifdef DUAL_LAN_NETWORK
				rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
				rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
			}

			//here is get_dev_info no auth(normal)
			if(rtn_dev != 0)
			{
	memset(runtime[port].username, 0x00, sizeof(runtime[port].username));
	memset(runtime[port].password, 0x00, sizeof(runtime[port].password));
	runtime[port].onvif.auth_method = NF_ONVIF_AUTH_NONE;
#ifdef DUAL_LAN_NETWORK
    rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
	rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
			}
		}
	}

	if (rtn_dev == 0)
	{
		IPCAM_DBG(MINOR, "Manufacturer Name(%s)\n", runtime[port].sys.vendor);
		if (strcmp(runtime[port].sys.vendor, "MESSOA") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "1234", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "i3 International") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "1234", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strncasecmp(runtime[port].sys.vendor, "Grundig", 64) == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "1234", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strncasecmp(runtime[port].sys.vendor, "Orion", 64) == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "iplinker") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "Panasonic") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "12345", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "Sony") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "ACTi") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "123456", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
#if 0
		else if (strcmp(runtime[port].sys.vendor, "Samsung") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "4321", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
#endif
		else if (strcmp(runtime[port].sys.vendor, "CNB") == 0)
		{
			strncpy(runtime[port].username, "root", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "VICON") == 0)
		{
			strncpy(runtime[port].username, "ADMIN", 64);
			strncpy(runtime[port].password, "1234", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "Honeywell") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "1234", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "HITRON") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strcmp(runtime[port].sys.vendor, "IQeye by Vicon") == 0)
		{
			strncpy(runtime[port].username, "root", 64);
			strncpy(runtime[port].password, "system", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
		}
		else if (strcmp(runtime[port].sys.vendor, "HDPRO") == 0)
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strncasecmp(runtime[port].sys.vendor, "HDPRO", 64) == 0)
		{
			if(nf_ipcam_is_vendor_orion() == 1 || nf_ipcam_is_vendor_g4s() == 1)
			{
				strncpy(runtime[port].username, "admin", 64);
				strncpy(runtime[port].password, "admin", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
			}
		}
#if 0
        else if (strncasecmp(runtime[port].sys.vendor, "AXIS", 64) == 0)
        {
            strncpy(runtime[port].username, "root", 64);
            strncpy(runtime[port].password, "pass", 64);
            runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;

            /* AXIS auto conn TEST*/
            if(nf_onvif_create_user(port, "root", "pass", "Administrator") == 0)
            {
                nf_axis_set_user_raw(discovery[port].ipaddr,
                                     runtime[port].sys.http_port, "root", "pass");
                nf_axis_set_webservice_raw(discovery[port].ipaddr,
                                           runtime[port].sys.http_port, "root", "pass");
                nf_axis_restart_server_raw(discovery[port].ipaddr,
                                           runtime[port].sys.http_port, "root", "pass");
            }
            IPCAM_DBG(MINOR, "CH(%d) reset\n", port);
            memset(&discovery[port], 0x00, sizeof(dtable));
            memset(&runtime[port], 0x00, sizeof(mtable));
            runtime[port].state = MGMT_STATE_UNLINKED;
            nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
            set_switch_polling_delay(port,150);
            return;
        }
#endif
		else if (strncasecmp(runtime[port].sys.vendor, "ONVIF", 64) == 0)	//Hikvision : KOBI
		{
				strncpy(runtime[port].username, "admin", 64);
				strncpy(runtime[port].password, "12345", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else if (strncasecmp(runtime[port].sys.vendor, "Hikvision", 64) == 0)
		{
				strncpy(runtime[port].username, "admin", 64);
				strncpy(runtime[port].password, "admin12345", 64);
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
		else
		{
			strncpy(runtime[port].username, "admin", 64);
			strncpy(runtime[port].password, "admin", 64);
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		}
	}
	else
	{
		gchar key_u[64];
		gchar key_p[64];
		gchar *u,*p;
		snprintf(key_u, 64, "cam.logininfo.L%d.id", port);
		snprintf(key_p, 64, "cam.logininfo.L%d.pwd", port);
		u = nf_sysdb_get_str_nocopy(key_u);
		p = nf_sysdb_get_str_nocopy(key_p);
		strncpy(runtime[port].username, u, 64);
		strncpy(runtime[port].password, p, 64);

		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
#ifdef DUAL_LAN_NETWORK
        rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
		rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
		if (rtn_dev != 0)
		{
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
#ifdef DUAL_LAN_NETWORK
            rtn_dev = nf_onvif_get_dev_info(port, discovery[port].layer == IPCAM_DISC_LAYER_VHUB ? 1 : 0);
#else
			rtn_dev = nf_onvif_get_dev_info(port, 0);
#endif
		}
		if (rtn_dev != 0)
		{
			nf_eventlog_put_ipcam_msg("Get Device info Fail", port);
			nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);

			return;
		}
	}

	if (discovery[port].vnet_id != 0 && discovery[port].vnet_id != 1)
	{
#ifdef DUAL_LAN_NETWORK
		rtn = nf_onvif_device_set_dhcp(port, (discovery[port].layer == IPCAM_DISC_LAYER_VHUB) ? 1 : 0);
#else
		rtn = nf_onvif_device_set_dhcp(port, 0);
#endif
		if (rtn != 0)
		{
			char *temp_id;
			char *temp_pwd;
			char temp_key[128];


			snprintf(temp_key, 128, "cam.logininfo.L%d.id", port);
			temp_id = nf_sysdb_get_str_nocopy(temp_key);

			snprintf(temp_key, 128, "cam.logininfo.L%d.pwd", port);
			temp_pwd = nf_sysdb_get_str_nocopy(temp_key);

			strncpy(runtime[port].username, temp_id, 64);
			strncpy(runtime[port].password, temp_pwd, 64);

			if ((strlen(temp_id) == 0) && (strlen(temp_pwd) == 0))
			{
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_NONE;
			}
			else
			{
				runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
			}
#ifdef DUAL_LAN_NETWORK
            rtn = nf_onvif_device_set_dhcp(port, (discovery[port].layer == IPCAM_DISC_LAYER_VHUB) ? 1 : 0);
#else
			rtn = nf_onvif_device_set_dhcp(port, 0);
#endif
		}
		if (rtn != 0)
		{
			IPCAM_DBG(WARN, "CH(%d) set dhcp failed\n", port);
			nf_eventlog_put_ipcam_msg("Set DHCP Fail", port);
			nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
			return;
		}
		IPCAM_DBG(MINOR, "CH(%d) reset\n", port);
		memset(&discovery[port], 0x00, sizeof(dtable));
		memset(&runtime[port], 0x00, sizeof(mtable));
		runtime[port].state = MGMT_STATE_UNLINKED;
		{
			//nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
			rtn = nf_ipcam_poe_reboot(port, NULL, NULL, NULL);
			printf("[%s:%d] port[%d] poe reboot[%s]\n", __func__, __LINE__, port, (rtn ==  IPCAM_SETUP_RTN_DONE) ? "success":"failure");
			if(rtn == IPCAM_SETUP_RTN_DONE)
			{
				IPCAM_DBG(MINOR, "poe reboot OK \n");
			}
			else
			{
				IPCAM_DBG(MINOR, "poe reboot fail \n");
			}
			set_switch_polling_delay(port,5);
			return;
		}
		return;
	}

	IPCAM_DBG(MINOR, "CH(%d) onvif discovered\n", port);
	discovery[port].state = IPCAM_DISC_STATE_CAPA;
	discovery[port].state_cnt = 0;
	runtime[port].sys.ipaddr = discovery[port].ipaddr;
	memcpy(&runtime[port].sys.macaddr[0], &discovery[i].macaddr[0], 6);

	//IPCAM_DBG(MAJOR, "end\n");
}

static int _get_wsd_ns(const char* msg, char* buf)
{
	const char xmlns[] = "xmlns:";
	const char find_namespace[] = "\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"";
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

		p = strstr(xmlns_token, find_namespace);
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

static int _find_service_tail(int port, unsigned char* msg)
{
	unsigned char *s,*s1, *e, *p1, *p2, *p3;
	const char xaddr[] = "<%s:XAddrs>";
	const char xaddr_end[] = "</%s:XAddrs>";
	char find_svc[64];
	char find_svc_end[64];
	char wsd_ns[32];
	dtable *discovery = get_dtable();
	mtable *runtime = get_runtime();


	memset(wsd_ns, 0x00, 32);
	if (_get_wsd_ns(msg, wsd_ns) != 1)
	{
		IPCAM_DBG(WARN, "WS-Discovery namespace not found\n");
		return SVC_ERROR_NOADDR;
	}

	memset(find_svc, 0x00, 64);
	memset(find_svc_end, 0x00, 64);
	snprintf(find_svc, 64, xaddr, wsd_ns);
	snprintf(find_svc_end, 64, xaddr_end, wsd_ns);
	IPCAM_DBG(MINOR, "XAddr tag name is '%s'\n", find_svc);

	s = strstr(msg, find_svc);
	s1 = s;
	if (s == NULL)
	{
		IPCAM_DBG(ERROR, "No service address\n");
		return SVC_ERROR_NOADDR;
	}

	{
		char temp_head[128];
		char temp_ip[16];
		memset(temp_head, 0x00, 128);
		memset(temp_ip, 0x00, 16);
		unsigned int Check_IP = 0;//sony_link_local ip check

#if 0
		if (nf_ipcam_is_vendor_s1() == 0)
		{
			snprintf(temp_head, 128, "http://169.254.");
		}
		else
		{
			snprintf(temp_head, 128, "https://169.254.");
		}
		s = strstr(s, temp_head);
		if (s != NULL)
		{
			IPCAM_DBG(MINOR, "Link local address support\n");
			p1 = s+7;
			p2 = strstr(p1, "/");
			memcpy(temp_ip, p1, p2-p1);
			discovery[port].ipaddr = inet_addr(temp_ip);
			discovery[port].vnet_id = 0;
			s = p2+1;
		}
		else
#endif
		//SONY CAM CHECK : CHANGED IP to DHCP IP or LINK LOCAL IP 
		if (nf_ipcam_is_vendor_s1() == 0)
		{
			snprintf(temp_head, 128, "http://169.254.");
		}
		else
		{
			snprintf(temp_head, 128, "https://169.254.");
		}
		s = strstr(s, temp_head);
		if (s != NULL)
		{
			p1 = s+7;
			p2 = strstr(p1, "/");
			memcpy(temp_ip, p1, p2-p1);
			Check_IP = inet_addr(temp_ip);
		}

		{
			s = s1;
			memset(temp_head, 0x00, 128);
			if (nf_ipcam_is_vendor_s1() == 0)
			{
				snprintf(temp_head, 128, "http://%d.%d.%d.%d/",
						(discovery[port].ipaddr&0xff),
						(discovery[port].ipaddr&0xff00)>>8,
						(discovery[port].ipaddr&0xff0000)>>16,
						(discovery[port].ipaddr&0xff000000)>>24);
			}
			else
			{
				snprintf(temp_head, 128, "https://%d.%d.%d.%d/",
						(discovery[port].ipaddr&0xff),
						(discovery[port].ipaddr&0xff00)>>8,
						(discovery[port].ipaddr&0xff0000)>>16,
						(discovery[port].ipaddr&0xff000000)>>24);
			}
			IPCAM_DBG(MINOR, "find %s\n", temp_head);
			s = strstr(s, temp_head);
			if (s == NULL)
			{
#if 0
				s = s1;
				memset(temp_head, 0x00, 128);
				snprintf(temp_head, 128, "http://%d.%d.%d.%d:80/",
						(discovery[port].ipaddr&0xff),
						(discovery[port].ipaddr&0xff00)>>8,
						(discovery[port].ipaddr&0xff0000)>>16,
						(discovery[port].ipaddr&0xff000000)>>24);
				IPCAM_DBG(MINOR, "find %s\n", temp_head);
				s = strstr(s, temp_head);
				if (s == NULL)
				{
					IPCAM_DBG(ERROR, "XAddr head\n%s", msg);
					return SVC_ERROR_HEAD;
				}
#else
				char _portbuf[8];
				char *_port_start;
				char *_port_end;
				int sony_checker = (-1);

				s = s1;
				memset(temp_head, 0x00, 128);
				if (nf_ipcam_is_vendor_s1() == 0)
				{
					snprintf(temp_head, 128, "http://%d.%d.%d.%d:",
							(discovery[port].ipaddr&0xff),
							(discovery[port].ipaddr&0xff00)>>8,
							(discovery[port].ipaddr&0xff0000)>>16,
							(discovery[port].ipaddr&0xff000000)>>24);
				}
				else
				{
					snprintf(temp_head, 128, "https://%d.%d.%d.%d:",
							(discovery[port].ipaddr&0xff),
							(discovery[port].ipaddr&0xff00)>>8,
							(discovery[port].ipaddr&0xff0000)>>16,
							(discovery[port].ipaddr&0xff000000)>>24);
				}
				IPCAM_DBG(MINOR, "find %s\n", temp_head);
				s = strstr(s, temp_head);
				if (s == NULL)
				{
					{
						//Sony CAM  exception code : SET DHCP DELAY ROUTINE 
						if(Check_IP != 0)
						{
							unsigned char *compare_str1 = NULL;
							unsigned char *compare_str2 = NULL;

							if(msg != NULL)
								compare_str1 = strstr(msg, "Scopes");

							if(compare_str1 != NULL)
							{
								compare_str2 = strstr(compare_str1, "Sony");
								if(compare_str2 != NULL)
								{		
									return SVC_ERROR_DELAY;
								}
							}
						}
					}

					IPCAM_DBG(ERROR, "XAddr head\n%s", msg);
					return SVC_ERROR_HEAD;
				}

				_port_start = s + strlen(temp_head);
				_port_end = strstr(_port_start, "/");
				memset(_portbuf, 0x00, 8);
				memcpy(_portbuf, _port_start, (_port_end-_port_start));
				runtime[port].sys.http_port = atoi(_portbuf);
				s += (1+_port_end-_port_start);
#endif
			}
			else
			{
				if (nf_ipcam_is_vendor_s1() == 0)
				{
					runtime[port].sys.http_port = 80;
				}
				else
				{
					runtime[port].sys.http_port = 443;
				}
			}
			s += strlen(temp_head);
		}
	}
	p1 = strstr(s, " ");
	p2 = strstr(s, find_svc_end);
	if (p1 == NULL)
	{
		if (p2 == NULL)
		{
			IPCAM_DBG(ERROR, "device service end point\n");
			return SVC_ERROR_ENDPOINT;
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
			e = (p1 < p2) ? p1:p2;
		}
	}
	memset(runtime[port].onvif.xaddr_dev_tail, 0x00, 128);
	memcpy(runtime[port].onvif.xaddr_dev_tail, s, e-s);
	runtime[port].onvif.onvif_service |= __OFM(NF_ONVIF_SERVICE_DEVICE);

	IPCAM_DBG(MINOR, "xaddr_dev_tail(%s)\n", runtime[port].onvif.xaddr_dev_tail);

	return SVC_OK_FOUND;
}

static int _find_login_info(int port)
{
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	char *model = &runtime[port].sys.model[0];
	int rtn = 1;


#if 1
#if 0
	if (strcmp(model, "P3346") == 0 ||
		strcmp(model, "M3114") == 0 ||
		strcmp(model, "M3113") == 0)		// Axis
	{
		strncpy(runtime[port].onvif.mp_token, "quality_h264", 64);
		strncpy(runtime[port].onvif.sp_token, "balanced_h264", 64);
		strncpy(runtime[port].onvif.mp_vsc_token, "0", 64);
		strncpy(runtime[port].onvif.sp_vsc_token, "0", 64);
		strncpy(runtime[port].onvif.mp_vec_token, "quality_h264", 64);
		strncpy(runtime[port].onvif.sp_vec_token, "balanced_h264", 64);

		strncpy(runtime[port].username, "root", 64);
		strncpy(runtime[port].password, "pass", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else
#endif
#if 1
	if (strcmp(model, "SNC-DH210T") == 0)	// Sony
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "admin", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else
#endif
	if (strncmp(model, "GCI-", 4) == 0)
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "1234", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else if (strncmp(model, "OR-", 3) == 0)
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "admin", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else if (strcmp(model, "WV-SF135") == 0 ||
			 strcmp(model, "WV-SF336") == 0)	// Panasonic
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "12345", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else if (strncmp(model, "ANNEXXUS", 8) == 0 ||
			 strcmp(model, "NCR875PRO") == 0 ||
			 strcmp(model, "NDF821") == 0)	// Messoa
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "1234", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
	else if (strcmp(model, "SNV-5010") == 0 ||
			 strcmp(model, "SNO-7080R") == 0)	// Samsung
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "4321", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
	}
#if 0
	else if (strcmp(model, "AMZ-1100") == 0)	// Sunkwang
	{
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "admin", 64);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
	}
#endif
	else
	{
		char *temp_id;
		char *temp_pwd;
		char temp_key[128];


		snprintf(temp_key, 128, "cam.logininfo.L%d.id", port);
		temp_id = nf_sysdb_get_str_nocopy(temp_key);

		snprintf(temp_key, 128, "cam.logininfo.L%d.pwd", port);
		temp_pwd = nf_sysdb_get_str_nocopy(temp_key);

		strncpy(runtime[port].username, temp_id, 64);
		strncpy(runtime[port].password, temp_pwd, 64);

		if ((strlen(temp_id) == 0) && (strlen(temp_pwd) == 0))
		{
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_NONE;
		}
		else
		{
			runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
		}
		rtn = 0;
	}
#else
	char *temp_id;
	char *temp_pwd;
	char temp_key[128];


	snprintf(temp_key, 128, "cam.logininfo.L%d.id", port);
	temp_id = nf_sysdb_get_str_nocopy(temp_key);

	snprintf(temp_key, 128, "cam.logininfo.L%d.pwd", port);
	temp_pwd = nf_sysdb_get_str_nocopy(temp_key);

	strncpy(runtime[port].username, temp_id, 64);
	strncpy(runtime[port].password, temp_pwd, 64);

	if ((strlen(temp_id) == 0) && (strlen(temp_pwd) == 0))
	{
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_NONE;
	}
	else
	{
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_TEXT;
	}
	rtn = 0;
#endif

	return rtn;
}

/* Exception func because of needed to connection for Hikvision Cam(CCTV mode) */
static int _send_hikvision_discovery(int hikvision_sock)
{
	unsigned char src_mac[6] = {0,};
	int ret = (-1);
	int send_result = (-1);
	int j = 0;
	char * data_frame = NULL;
	//hikvision ethernet frame is 80 byte
	int PACKET_LEN = 80;

	//target
	struct sockaddr_ll socket_address;
	//buffer for ethernet frame
	void* buffer = (void*)malloc(PACKET_LEN);
	//pointer to ethenet header
	unsigned char* etherhead = buffer;
	//userdata in ethernet frame
	unsigned char* data = buffer + 14;
	//another pointer to ethernet header
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	
	//IPX MAC addr
	ret = nf_netif_get_mac(src_mac);

	if(!ret)
	{
		return (-1);
	}

	//broad cast mac addr 
	unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;	
	socket_address.sll_protocol = htons(ETH_P_IP);	

	/*index of the network device
	 * see full code later how to retrieve it*/
	socket_address.sll_ifindex  = 2;
	socket_address.sll_hatype   = ARPHRD_ETHER;

	//target is another host
	socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;		

	/*MAC - broad cast*/
	socket_address.sll_addr[0]  = 0xff;		
	socket_address.sll_addr[1]  = 0xff;		
	socket_address.sll_addr[2]  = 0xff;
	socket_address.sll_addr[3]  = 0xff;
	socket_address.sll_addr[4]  = 0xff;
	socket_address.sll_addr[5]  = 0xff;
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/

	//set the frame header
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);

	//FIXME hikvision type
	eh->h_proto = 0x3380;

	/*fill the frame with hikvision data >> FIXME 66 byets data*/
	char res[67] = {0,};
	data_frame = _init_hikvision_data(res);

	for (j = 0; j < 66; j++) 
	{
		data[j] = (unsigned char)data_frame[j+1];
	}
		
	send_result = sendto(hikvision_sock, buffer, PACKET_LEN, 0, 
			(struct sockaddr*)&socket_address, sizeof(socket_address));//
	
	if(send_result < 0)
	{
		printf("\e[31m >> raw sock sendto fail....\e[0m\n");
		perror("sendto");
		if(buffer != NULL)
			free(buffer);
		return send_result;
	}
		
	if(buffer != NULL)
		free(buffer);

	return 1;
}

static char * _init_hikvision_data(char* res)
{
	enum { BYTES = 67 };
	//char res[BYTES];
	const char * c = 	
		"21 02 01 42 00 00 11 40 06 04 03 00 99 3D 00 E0 4C 05 36 25 AC 10 00 0A " 
		"FF FF FF FF FF FF 00 00 00 00 00 00 00 00 FE 80 00 00 00 00 00 00 7C 1E " 
		"3F 57 2A 8F 72 33 00 00 00 00 00 00 00 00 00 00 00 00";
	const char * p = c;
	int i = 0;

	res[i] = 0;
	char ch = ' ';
	while (ch && i < BYTES){
		switch (ch){
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				ch -= '0' + 10 - 'A';
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				ch -= 'A' - 10;
				res[i] = res[i]*16 + ch;
				break;
			case ' ':
				if (*p != ' ') {
					if (i == BYTES-1){
						printf("parse error, throw exception\n");
						exit(-1);
					}
					res[++i] = 0;
				}
				break;
			case 0:
				break;
			default:
				printf("parse error, throw exception\n");
		}
		ch = *(p++);
	}
	if (i != BYTES-1){
		printf("parse error, throw exception\n");
	}
#if 0
	printf("\e[31m >> [hex data] >> \e[0m\n");
	for (i = 1 ; i < 67; i++){
		printf("%2.2x ", 0xFF & res[i]);
	}
	printf("\e[31m >> data init END\e[0m\n");
#endif
	return res;
}

static int _init_hikvision_sock(void)
{
	if (hikvision_sock > 0)
	{
		return hikvision_sock;
	}

	hikvision_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if (hikvision_sock < 0)
	{
		IPCAM_DBG(ERROR, "hikvision raw send socket init failed\n");
		perror("raw socket creat fail");
		return (-1);
	}
	return hikvision_sock;
}

static int _set_hikvision_dhcp(int port, unsigned int ipaddr)
{
	int rtn = -1;
	dtable *discovery = NULL;
	mtable *runtime = NULL;
	runtime = get_runtime();
	discovery = get_dtable();

	int temp_delay = 0;

	memset(runtime[port].username, 0x00, sizeof(runtime[port].username));
	memset(runtime[port].password, 0x00, sizeof(runtime[port].password));

	if(ipaddr == 1073742016)
	{
		runtime[port].sys.ipaddr =  ipaddr;
		strncpy(runtime[port].onvif.xaddr_dev_tail, "onvif/device_service", 128);
		runtime[port].sys.http_port = 80;
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "12345", 64);

#ifdef DUAL_LAN_NETWORK
        rtn = nf_onvif_device_set_dhcp(port, (discovery[port].layer == IPCAM_DISC_LAYER_VHUB) ? 1 : 0);
#else
		rtn = nf_onvif_device_set_dhcp(port, 0);
#endif
	}
	else if(ipaddr == 1073850560)
	{
		runtime[port].sys.ipaddr =  ipaddr;
		runtime[port].sys.http_port = 80;
		strncpy(runtime[port].onvif.xaddr_dev_tail, "onvif/device_service", 128);
		runtime[port].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
		strncpy(runtime[port].username, "admin", 64);
		strncpy(runtime[port].password, "admin12345", 64);

#ifdef DUAL_LAN_NETWORK
        rtn = nf_onvif_device_set_dhcp(port, (discovery[port].layer == IPCAM_DISC_LAYER_VHUB) ? 1 : 0);
#else
		rtn = nf_onvif_device_set_dhcp(port, 0);
#endif
	}

	if(rtn != 0)
	{
		discovery[port].vnet_id = 0;

		//Hikvision camera DHCP request is gotten by shutdowned camera
		memset(&discovery[port], 0x00, sizeof(dtable));
		memset(&runtime[port], 0x00, sizeof(mtable));
		runtime[port].state = MGMT_STATE_UNLINKED;
		//printf("Hikvision CAM DEVICE_OUT FOR DHCP REQUEST\e[0m\n");

		nf_pnd_queue_push(port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
		set_switch_polling_delay(port,5);
	}

	return rtn;
}

static void _search_hikvision()
{
	int rtn = 0;
	int port = 0;
	dtable *discovery = get_dtable();
	mtable *runtime = get_runtime();
	
	for(port = 0; port < NUM_ACTIVE_CH; port++)
	{
		if(discovery[port].ipaddr == 1073742016 || discovery[port].ipaddr == 1073850560)
		{
			if(discovery[port].state == IPCAM_DISC_STATE_VNET)
			{
				int rtn = (-1);

				rtn = _set_hikvision_dhcp(port, discovery[port].ipaddr);

				if (rtn != 0)
				{
					sleep(1);
					continue;
				}
			}
		}
	}
}


#if 0
static int _axis_discovery(int port)
{
	int i;
	axis_Discovery ttt;
	char ipstr[16];
	unsigned int ipaddr;
	mtable *runtime = get_runtime();
	profile* models = NULL;
	GAsyncQueue *queue = get_queue();


	ipaddr = runtime[port].sys.ipaddr;
	snprintf(ipstr, 16, "%d.%d.%d.%d",
			ipaddr&0xff, (ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24);
	ttt = IsAxisCamera(ipstr);

	if (ttt.isAxisCamera)
	{
		int db_cnt;

		switch (ttt.discoveryResult)
		{
			/* Connection fail */
			case AD_CONNECTION_FAIL:
			case AD_DISABLE_WEBSERVICE:
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_CONNECTION_FAIL, __LINE__, __FILE__);
				return (1);
			}
			/* Login fail */
			case AD_NOT_MATCH_ID_PASS:
			{
				nf_pnd_queue_push(port, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
				return (1);
			}
			default:
				break;
		}
	}

	return (0);
}
#endif

#endif	// __NF_IPCAM_DISCOVERY_ONVIF_C__
