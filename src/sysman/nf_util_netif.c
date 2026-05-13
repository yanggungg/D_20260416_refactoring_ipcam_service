#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
// onvif_porting
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <errno.h>
// onvif_porting

#include <linux/types.h>   

typedef unsigned short u16;   
typedef unsigned int u32;   
typedef unsigned long long u64;   
typedef unsigned char u8;   

#include <linux/ethtool.h>   
#include <linux/sockios.h>   
#include "proxy_cli.h"

#include "nf_common.h"
#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
#include "nf_util_device.h"
#endif
#include "nf_util_netif.h"
#include "nf_debug.h"
#include "nf_sysman.h"
#include "nf_logevtdef.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_netif"

#define DEBUG_NETIF_JBSHELL

#ifdef DEBUG_NETIF_JBSHELL
	#include "jbshell.h"
#endif

#define USE_PROXY_SYSTEM

#define ENABLE_IPV6

// onvif_porting
#define ENABLE_NETIF_PING_TEST
#ifdef ENABLE_ONVIF_DEVICE

struct arpMsg {
    struct ethhdr ethhdr;           /* Ethernet header */
    u_short htype;              /* hardware type (must be ARPHRD_ETHER) */
    u_short ptype;              /* protocol type (must be ETH_P_IP) */
    u_char  hlen;               /* hardware address length (must be 6) */
    u_char  plen;               /* protocol address length (must be 4) */
    u_short operation;          /* ARP opcode */
    u_char  sHaddr[6];          /* sender's hardware address */
    u_char  sInaddr[4];         /* sender's IP address */
    u_char  tHaddr[6];          /* target's hardware address */
    u_char  tInaddr[4];         /* target's IP address */
    u_char  pad[18];            /* pad for min. Ethernet payload (60 bytes) */
};

/* function prototypes */
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char *arp, char *interface);
#endif
// onvif_porting

//#define ENABLE_NETIF_PING_TEST

gboolean nf_netif_init();

static void nf_netif_dhcp_server_onoff();
static gint apply_dhcp();
// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
static gint apply_dhcp_gateway();
static gint apply_dhcp_dns();
static gint apply_dhcp_ipaddr();
#endif
// onvif_porting
static gint apply_ethernet(guint addr, guint gw, guint netmask);
static gint apply_ethernet_lan2(guint addr, guint gw, guint netmask);
static gint apply_dns(guint dns1, guint dns2);

#ifdef ENABLE_IPV6
static int apply_ipv6();

static int netif_ipv6_enable(int is_enable);
static int netif_ipv6_add_ip( const char *dev, const char *addr, int prefix_len);
static int netif_ipv6_add_dns(  const char *addr);
static int netif_ipv6_del_ip( const char *dev, const char *addr, int prefix_len);
static int netif_ipv6_add_gw( const char *dev, const char *addr);
static int netif_ipv6_del_gw( const char *dev, const char *addr);

typedef struct _IPV6_DNS{
	int count;
	gchar	ipv6_dns[IPV6_DNS_MAX][IPV6_STR_MAX];
} IPV6_DNS;
static guint netif_ipv6_get_dns(IPV6_DNS *ipv6_dns);

static guint netif_ipv6_get_gateway(char *ipv6_gateway);

static int if_get_ipv6(const char *dev, int filter_scope, char *addr, int addr_len, guint *prefix_val);

typedef struct _IPV6_ADDR_PREFIX{
	int count;
	gchar	ipv6_addr[IPV6_ADDR_MAX][IPV6_STR_MAX];
	guint	ipv6_prefix[IPV6_PREFIX_MAX];
} IPV6_ADDR_PREFIX;
static int if_get_ipv6_all(const char *dev, int filter_scope, IPV6_ADDR_PREFIX *addr, int addr_len, int max_count);

/*
'<item key="net.ipv6.using"     type="UINT"     min="0" max="2" val="0" />'
'<item key="net.ipv6.linklocal" type="STRING"   min="0" max="50" val="0" />'

'<item key="net.ipv6.addr0"     type="STRING"   min="0" max="50" val="0" />'
'<item key="net.ipv6.addr1"     type="STRING"   min="0" max="50" val="0" />'
'<item key="net.ipv6.addr2"     type="STRING"   min="0" max="50" val="0" />'
'<item key="net.ipv6.addr3"     type="STRING"   min="0" max="50" val="0" />'

'<item key="net.ipv6.ddn1"      type="STRING"   min="0" max="50" val="0" />'
'<item key="net.ipv6.ddn2"      type="STRING"   min="0" max="50" val="0" />'
'<item key="net.ipv6.gateway"   type="STRING"   min="0" max="50" val="0" />'

'<item key="net.ipv6.prefix0"   type="UINT"     min="0" max="128" val="" />'
'<item key="net.ipv6.prefix1"   type="UINT"     min="0" max="128" val="" />'
'<item key="net.ipv6.prefix2"   type="UINT"     min="0" max="128" val="" />'
'<item key="net.ipv6.prefix3"   type="UINT"     min="0" max="128" val="" />'
*/	
	
#endif

static guint if_get_ip(const char *dev);
static guint if_get_broadcast(guint ip, guint netmask);
static guint if_get_netmask(const char *dev);
static guint if_get_gateway(void);
static guint if_get_dns(int index);
static gint  if_get_mac(const char *dev, unsigned char *buff);

static void pr_exit(int status);

#if 0
<item key="net.proto.ipaddr"			type="UINT" 	min="0" max="" val="0xC0A86449" />
<item key="net.proto.gateway"			type="UINT" 	min="0" max="" val="0xC0A86401" />
<item key="net.proto.subnet"			type="UINT" 	min="0" max="" val="0xffffff00" />
<item key="net.proto.dns1"				type="UINT" 	min="0" max="" val="0xC0A86401" />
<item key="net.proto.dns2"				type="UINT" 	min="0" max="" val="0xC0A86401" />
#endif

static int netif_ipv4_link_local_enable(const char *dev);
static int netif_ipv4_link_local_make_ip(char *ip_buf, size_t buf_size);
static size_t netif_ipv4_ip_conflict(const char *dev, const char *ip);
static int netif_ipv4_add_ip(const char *dev, const char *ip, const char *netmask);

static char *_eth_dev = HOST_ETH_DEV;

static guint _netif_init_done = 0;

#define DHCP_INIT		"/tmp/dhcp.proc.init"

static void _dhcp_start(void)
{
	gint ret = 0;	
	gchar cmd[1024];
	NF_NETIF_IP ret_ip;

	snprintf(cmd, sizeof(cmd), "/bin/touch %s", DHCP_INIT);
	proxy_system(cmd,1,3);

#ifdef USE_PROXY_SYSTEM
	snprintf(cmd, sizeof(cmd), "udhcpc -b -i %s", _eth_dev);	
 	ret = proxy_system(cmd, 1, 10);
#else
	snprintf(cmd, sizeof(cmd), "udhcpc -b -i %s", _eth_dev);
	ret = system(cmd);
	pr_exit(ret);
#endif

	memset(&ret_ip, 0x0, sizeof(NF_NETIF_IP));

	nf_netif_get_ip(&ret_ip);

	if( (ret_ip.ip_addr[0] & 0xff) == 0x0 || (ret_ip.ip_addr[0] & 0xff) == 0xff )
	{
	    g_message( "%s - DHCP INIT FAIL", __FUNCTION__);		
		nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 0, LP2_NETWORK_DHCP_FAIL, NULL);
	}
	else
	{
		gchar ip_str[64];
		
		snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ret_ip.ip_addr[0], ret_ip.ip_addr[1], ret_ip.ip_addr[2], ret_ip.ip_addr[3]);

	    g_message( "%s - DHCP INIT SUCCESS", __FUNCTION__);
		
		nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 1, LP2_NETWORK_DHCP_OK, ip_str);
	}

	remove(DHCP_INIT);

	g_message("%s sleep(3)", __FUNCTION__ );
	sleep(3); // FIXME  for dns setup delay

	// get network information.
	res_init();	
}

static void _dhcp_stop(void)
{
	
#ifdef USE_PROXY_SYSTEM
	proxy_system("killall -9 udhcpc", 1, 3);
#else
	system("killall -9 udhcpc");
#endif

}

static void _dhcp_ipv6_start()
{
	gchar cmd[1024];

	snprintf(cmd, sizeof(cmd), "dibbler-client start");
	proxy_system(cmd,1,3);
}	

static void _dhcp_ipv6_stop()
{
	gchar cmd[1024];

	snprintf(cmd, sizeof(cmd), "dibbler-client stop");
	proxy_system(cmd,1,3);
}

static gboolean _is_skip_mode(void)
{
	struct stat buf;
	
	if( nf_sysman_hotkey_is_nfs() )
	{
		g_warning("%s nfs mode!! skip init", __FUNCTION__);
		return TRUE;
	}

	if( nf_sysman_hotkey_is_console_en() )
	{
		g_warning("%s console mode!! skip init", __FUNCTION__);
		return TRUE;
	}

	if( stat( "/NFDVR/data/debug_mode", &buf ) == 0 )
	{
		g_warning("%s debug_mode!! skip init", __FUNCTION__);
		return TRUE;						
	}

	return FALSE;
}

gboolean nf_netif_dhcp_restart(void)
{
	gint is_dhcp=0;
	gint ipv6_mode=0;	

	if( _is_skip_mode() == TRUE )
		return FALSE;

	is_dhcp = nf_sysdb_get_bool( "net.proto.dhcpon");
	ipv6_mode = nf_sysdb_get_uint( "net.ipv6.using");	

	if(	is_dhcp )
	{
		_dhcp_stop();
		_dhcp_start();
	}
	
	if( ipv6_mode == 2 )
	{
		_dhcp_ipv6_stop();
		_dhcp_ipv6_start();
	}

	return TRUE;	
}

static char *_ip_to_str(unsigned int ip, char *buf)
{
	static char ret[20];
	if(buf == NULL){
		buf = ret;
	}
	snprintf(buf, 16, "%d.%d.%d.%d",
			(ip&0xff000000)>>24,
			(ip&0xff0000)>>16,
			(ip&0xff00)>>8,
			(ip&0xff));
	return buf;
}

gboolean nf_netif_eth_init(void)
{
	gint is_openmode = 0, is_custom = 0;

#ifdef DUAL_LAN_NETWORK
	is_openmode = nf_get_running_mode();//nf_sysdb_get_bool("cam.install.mode");
	is_custom = nf_get_custom_mode();//nf_sysdb_get_bool("cam.install.dual_lan");
	if(is_openmode == 1)
	{
		if(is_custom)
		{
			_eth_dev = HOST_ETH_DEV;

			return TRUE;
		}
		gchar cmd[256] = {0};
		gchar host_ip[32] = {0};
		gchar gw_ip[32] = {0};
		guchar buff_mac[6] = {0};
		gchar host_mac[32] = {0};
		gint gateway = 0;
		unsigned int eth_netmask = 0;
		NF_NETIF_IP ret_ip;

		// netmask
		eth_netmask = ntohl(if_get_netmask(HOST_ETH_DEV));

		// make bridge
		snprintf(cmd, sizeof(cmd), "brctl addbr %s", BRIDGE_DEV);
		proxy_system((const char *)cmd, 1, 3);
	
		// bridge multicast opt
		snprintf(cmd, sizeof(cmd), "echo 0 > /sys/devices/virtual/net/%s/bridge/multicast_snooping", BRIDGE_DEV);
		proxy_system((const char *)cmd, 1, 3);

		if(nf_sysman_hotkey_is_nfs() == 0)
		{
			// host eth dev interface
			snprintf(cmd, sizeof(cmd), "ifconfig %s 0.0.0.0", HOST_ETH_DEV);
			proxy_system((const char *)cmd, 1, 3);
		}

		// hub eth dev interface
		snprintf(cmd, sizeof(cmd), "ifconfig %s 0.0.0.0", HUB_ETH_DEV);
		proxy_system((const char *)cmd, 1, 3);

		// bridge write mac address
		memset(buff_mac, 0x00, sizeof(buff_mac));
		if_get_mac((const char *)HOST_ETH_DEV, buff_mac);	
		snprintf(host_mac, sizeof(host_mac),"%02x:%02x:%02x:%02x:%02x:%02x",
				buff_mac[0],buff_mac[1],buff_mac[2],
				buff_mac[3],buff_mac[4],buff_mac[5]);

		snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s", BRIDGE_DEV, host_mac);
		proxy_system((const char *)cmd, 1, 3);

		// bridge interface
		memset(&ret_ip, 0x00, sizeof(ret_ip));
		nf_netif_get_ip_dev(HOST_ETH_DEV, &ret_ip);

		if((ret_ip.ip_addr[0] & 0xff) == 0x0 || (ret_ip.ip_addr[0] & 0xff) == 0xff)
		{
			snprintf(host_ip, sizeof(host_ip), "0.0.0.0");
		}
		else
		{
			snprintf(host_ip, sizeof(host_ip), "%d.%d.%d.%d", 
					ret_ip.ip_addr[0], 
					ret_ip.ip_addr[1],
					ret_ip.ip_addr[2],
					ret_ip.ip_addr[3]);
		}

		snprintf(cmd, sizeof(cmd), "ifconfig %s %s", BRIDGE_DEV, host_ip);
		if(eth_netmask != ((unsigned int) -1)){
			snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd) - 1, " netmask %s", _ip_to_str(eth_netmask, NULL));
		}
		proxy_system((const char *)cmd, 1, 3);

		// add bridge eht0, eht1
		snprintf(cmd, sizeof(cmd), "brctl addif %s %s", BRIDGE_DEV, HUB_ETH_DEV);
		proxy_system((const char *)cmd, 1, 3);

		snprintf(cmd, sizeof(cmd), "brctl addif %s %s", BRIDGE_DEV, HOST_ETH_DEV);
		proxy_system((const char *)cmd, 1, 3);

		// add default route bridge
		memset(&ret_ip, 0x00, sizeof(ret_ip));
		nf_netif_get_gw_ip(&ret_ip);

		if((ret_ip.ip_addr[0] & 0xff) != 0x0)
		{
			snprintf(gw_ip, sizeof(gw_ip), "%d.%d.%d.%d", 
					ret_ip.ip_addr[0], 
					ret_ip.ip_addr[1],
					ret_ip.ip_addr[2],
					ret_ip.ip_addr[3]);

			snprintf(cmd, sizeof(cmd), "route add default gw %s %s", gw_ip, BRIDGE_DEV);
			proxy_system((const char *)cmd, 1, 3);
		}

		if(nf_sysman_hotkey_is_nfs() == 1)
		{
			// host eth dev interface
			snprintf(cmd, sizeof(cmd), "ifconfig %s 0.0.0.0", HOST_ETH_DEV);
			proxy_system((const char *)cmd, 1, 3);
		}

		// show bridge
		snprintf(cmd, sizeof(cmd), "brctl show");
		proxy_system((const char *)cmd, 1, 3);

		_eth_dev = BRIDGE_DEV;
	}
	else
	{
		_eth_dev = HOST_ETH_DEV;
	}
#else
		_eth_dev = HOST_ETH_DEV;
#endif

	return TRUE;
}

gint nf_set_irq_affinity(int irq) {
    gchar path[256];
    snprintf(path, sizeof(path), "/proc/irq/%d/smp_affinity", irq);
    FILE *file = fopen(path, "w");
    
    if (!file) {
        printf("%s %d smp_affinity fopen failed\n", __FUNCTION__, __LINE__);
        return 1;
    }
    
    if (fwrite("8", 1, 1, file) != 1) {
        printf("%s %d smp_affinity fwrite failed\n", __FUNCTION__, __LINE__);
        fclose(file);
        return 1;
    }
    
    fflush(file);
    fclose(file);
    printf("smp_affinity set to 8 successfully.\n");
    return 0;
}

/**
	@brief				�ý��� ��Ʈ��ũ ���� (sysdb ���� ���)	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_netif_init()
{
	guint addr=0, gateway=0, netmask=0;
	guint lan2_addr=0, lan2_gateway=0, lan2_netmask=0;
	guint dns1=0, dns2=0;
	gint is_dhcp=0, is_custom=0;
	guchar buff_mac[6];
	guchar buff_mac_str[32];
	

	is_dhcp = nf_sysdb_get_bool( "net.proto.dhcpon");			
	is_custom = nf_get_custom_mode();//nf_sysdb_get_bool("cam.install.dual_lan");

	nf_wpa_start(); // IEEE 802.1x supplcant start
		
	if_get_mac( _eth_dev, buff_mac);	
	snprintf(buff_mac_str, sizeof(buff_mac_str),"%02x%02x%02x%02x%02x%02x",
				buff_mac[0],buff_mac[1],buff_mac[2],
				buff_mac[3],buff_mac[4],buff_mac[5]);
	
	nf_sysdb_set_str("sys.info.mac", buff_mac_str);
#if 0 // choissi  2013.4.4 	
	nf_sysdb_set_str("sys.info.sysid", buff_mac_str);
#endif	
	g_message("%s is_dhcp[%d] mac[%s]",__FUNCTION__,is_dhcp, buff_mac_str);

	//local ethernet setting get
	lan2_addr 	= htonl( nf_sysdb_get_uint(LOCAL_NET_IPADDR) );
	lan2_gateway = htonl( nf_sysdb_get_uint(LOCAL_NET_GATEWAY) );
	lan2_netmask = 0xffffff; //255.255.255.0 fix__jr695

	if(nf_sysdb_get_uint(LOCAL_NET_IPADDR)==0)
	{
		printf("Default eth0 Local IP [10.20.0.1] Setting\n");
		lan2_addr = 0x100140A;
		nf_sysdb_set_uint(LOCAL_NET_IPADDR, ntohl(lan2_addr));
	}

	if(is_custom)
	{
		if(nf_netif_check_lan_conflict())
			return FALSE;
	}

	if( _is_skip_mode() == TRUE ){
// Temporarily excluded
#if 1
		// IPv4 local IP setting
		if(is_custom)
		{
			if(apply_ethernet_lan2(lan2_addr, lan2_gateway, lan2_netmask) < 0)
			{
				perror("apply_ethernet_lan2()");
				return FALSE;
			}
		}
		// IPv4 link-local address enable 
		if(is_custom)
		{
			//netif_ipv4_link_local_enable(HOST_ETH_DEV);
			//netif_ipv4_link_local_enable(HUB_ETH_DEV);
		}
		else
		{
			//netif_ipv4_link_local_enable(_eth_dev);
		}
		
		//DHCP SERVER ON/OFF
		if(is_custom)
		{
			nf_netif_dhcp_server_onoff();
		}
#endif

#ifdef ENABLE_IPV6		
		goto SET_IPv6;
#endif		
		return 0;						
	}

	_dhcp_stop();

	// Takenaka BPM-738
// 	proxy_system("rm -f /etc/reslov.conf", 1, 10);
 		
	if(is_dhcp) 
	{	
#if 0
#if defined(USE_MV6095F)
		nf_ipcam_stop();
#endif
#endif
		if(apply_dhcp() < 0) 
		{
			//perror("apply_dhcp()");
			err_msg("%s apply_dhcp() error", __FUNCTION__);
#if 0
#if defined(USE_MV6095F)
			nf_ipcam_ip_changed();
#endif
#endif
			return FALSE;
		}
#if 0
#if defined(USE_MV6095F)
		nf_ipcam_ip_changed();
#endif
#endif
	}
	else 
	{			// static
		addr 	= htonl( nf_sysdb_get_uint(NET_IPADDR) );
		gateway = htonl( nf_sysdb_get_uint(NET_GATEWAY) );
		netmask = htonl( nf_sysdb_get_uint(NET_SUBNET) );
													
		dns1 = htonl( nf_sysdb_get_uint(NET_DNS1) );
		dns2 = htonl( nf_sysdb_get_uint(NET_DNS2) );

		g_message("%s STATIC %s =======================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);

		if(apply_ethernet(addr, gateway, netmask) < 0) 
		{
			perror("apply_ethernet()");
			return FALSE;
		}

		//if(dns1 || dns2) 
		{
			if(apply_dns(dns1, dns2) < 0) 
			{
				perror("apply_dns()");
				return FALSE;
			}
		}
	}

// Temporarily excluded
#if 1
	// IPv4 local IP setting
	if(is_custom)
	{
		if(apply_ethernet_lan2(lan2_addr, lan2_gateway, lan2_netmask) < 0)
		{
			perror("apply_ethernet_lan2()");
			return FALSE;
		}
	}

	// IPv4 link-local address enable 
	if(is_custom)
	{
		//netif_ipv4_link_local_enable(HOST_ETH_DEV);
		//netif_ipv4_link_local_enable(HUB_ETH_DEV);
	}
	else
    {
		//netif_ipv4_link_local_enable(_eth_dev);
	}

	//DHCP SERVER ON/OFF
	if(is_custom)
	{
		nf_netif_dhcp_server_onoff();
	}
#endif

#ifdef ENABLE_IPV6

/*
#define IPV6_NET_USING		"net.ipv6.using"			uint
#define IPV6_NET_IPADDR_LL	"net.ipv6.linklocal"		string
#define IPV6_NET_IPADDR0	"net.ipv6.ipaddr0"
#define IPV6_NET_IPADDR1	"net.ipv6.ipaddr1"
#define IPV6_NET_IPADDR2	"net.ipv6.ipaddr2"
#define IPV6_NET_IPADDR3	"net.ipv6.ipaddr3"
#define IPV6_NET_DNS1		"net.ipv6.dns1"
#define IPV6_NET_DNS2		"net.ipv6.dns2"
#define IPV6_NET_GATEWAY	"net.ipv6.gateway"
*/

SET_IPv6:		
	apply_ipv6();	
	
#endif

	nf_netif_ipfilter_init();

	nf_set_irq_affinity(56);

	return TRUE;
}

/**
	@brief				LAN1 - LAN2 IP Conflict Check
	@return	gboolean	%TRUE on conflict, %FALSE if not conflict
*/
gboolean nf_netif_check_lan_conflict()
{
	guint lan1_ip=0, lan2_ip=0;
	guint lan1_tok[4]={0,}, lan2_tok[4]={0,};
	gint sector=0, conflict_cnt=0;

	lan1_ip = nf_sysdb_get_uint(NET_IPADDR);
	lan2_ip = nf_sysdb_get_uint(LOCAL_NET_IPADDR);

	for(sector=0;sector<4;sector++)
	{
		lan1_tok[sector]=((lan1_ip & (0xff << (8*sector))) >> (8*sector));
		lan2_tok[sector]=((lan2_ip & (0xff << (8*sector))) >> (8*sector));
	} //if lan1_ip=192.168.101.176 => tok[0]=176~tok[3]=192

	for(sector=3;sector>0;sector--)
	{
		if(lan1_tok[sector]==lan2_tok[sector])
			conflict_cnt++;
	}

	if(conflict_cnt==3)
	{
		printf("%s Lan IP is conflict :: LAN1[0x%x] LAN2[0x%x]\n",__FUNCTION__,lan1_ip, lan2_ip);
		return TRUE;
	}

	return FALSE;
}

/**
	@brief				�ý��� ��Ʈ��ũ ���� (sysdb ���� ���)	
	@param[out]	*mac	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_mac( NF_NETIF_MAC *ret_mac)
{	
	gint ret;	
	g_return_val_if_fail ( ret_mac , FALSE );
	
	memset( ret_mac, 0x00, sizeof(NF_NETIF_MAC));
	
	ret = if_get_mac(_eth_dev, ret_mac->mac_addr);
	
	return (ret == 0) ? 1 : 0;
}

gchar *nf_netif_get_mac_str(gchar *mac_buf)
{
	gint ret;
	g_return_val_if_fail( mac_buf, FALSE);

	memset(mac_buf, 0x00, 6);

	ret = if_get_mac(_eth_dev, mac_buf);

	return (ret == 0)? mac_buf : NULL;
}

gboolean nf_netif_get_ip(NF_NETIF_IP *ret_ip)
{
	guint ip = 0;
	
	g_return_val_if_fail( ret_ip, FALSE);

	memset( ret_ip, 0x0, sizeof(NF_NETIF_IP));
	
	ip = ntohl( if_get_ip(_eth_dev) );

	ret_ip->ip_addr[0] = (ip >> 24) & 0xff;
	ret_ip->ip_addr[1] = (ip >> 16) & 0xff;
	ret_ip->ip_addr[2] = (ip >> 8) & 0xff;
	ret_ip->ip_addr[3] = ip & 0xff;

	return TRUE;
}

gboolean nf_netif_get_ip_dev(const char *dev, NF_NETIF_IP *ret_ip)
{
	guint ip = 0;
	
	g_return_val_if_fail( dev, FALSE);
	g_return_val_if_fail( ret_ip, FALSE);

	memset( ret_ip, 0x0, sizeof(NF_NETIF_IP));
	
	ip = ntohl( if_get_ip(dev) );

	ret_ip->ip_addr[0] = (ip >> 24) & 0xff;
	ret_ip->ip_addr[1] = (ip >> 16) & 0xff;
	ret_ip->ip_addr[2] = (ip >> 8) & 0xff;
	ret_ip->ip_addr[3] = ip & 0xff;

	return TRUE;
}

gboolean nf_netif_get_gw_ip(NF_NETIF_IP *ret_ip)
{
	guint ip = 0;
	
	g_return_val_if_fail( ret_ip, FALSE);

	memset( ret_ip, 0x0, sizeof(NF_NETIF_IP));
	
	ip = ntohl( if_get_gateway() );

	ret_ip->ip_addr[0] = (ip >> 24) & 0xff;
	ret_ip->ip_addr[1] = (ip >> 16) & 0xff;
	ret_ip->ip_addr[2] = (ip >> 8) & 0xff;
	ret_ip->ip_addr[3] = ip & 0xff;

	return TRUE;
}

/**
	@brief				�ý��� ��Ʈ��ũ ������ ��� �´�.
	@param[out]	*ret	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_info( NF_NETIF_GET_INFO *ret_info)
{
	
	gint ret;	
	IPV6_ADDR_PREFIX addr_prefix;
	IPV6_DNS ipv6_dns;
	
	int i;

//res_state res = malloc(sizeof(struct __res_state));
	
	g_return_val_if_fail ( ret_info , FALSE );

	// get network information.
	res_init();
	
	memset( ret_info, 0x00, sizeof(NF_NETIF_GET_INFO) );
	memset( &addr_prefix, 0x0, sizeof(IPV6_ADDR_PREFIX));
	memset( &ipv6_dns, 0x0, sizeof(IPV6_DNS));	

	ret_info->ipaddr = ntohl( if_get_ip(_eth_dev) );
	ret_info->netmask = ntohl( if_get_netmask(_eth_dev) );
	ret_info->broadcast = ntohl( if_get_broadcast(ret_info->ipaddr, ret_info->netmask) );	    

	ret_info->gateway = ntohl( if_get_gateway() );
	ret_info->dnsserver1 = ntohl( if_get_dns(0) );	
	ret_info->dnsserver2 = ntohl( if_get_dns(1) );
	//ksi_test
	// if(nf_sysman_qcmode_is_enable() == 1)
	// 	nf_sysman_qc_atoi_mac(ret_info->mac_addr, sizeof(ret_info->mac_addr)/sizeof(gchar));
	// else
		if_get_mac(_eth_dev, ret_info->mac_addr);

	if(nf_get_custom_mode())
		ret_info->lan2_ipaddr = ntohl( if_get_ip(HUB_ETH_DEV) );

	if_get_ipv6(HOST_ETH_DEV, IPV6_ADDR_LINKLOCAL, ret_info->ipv6_linklocal, sizeof(ret_info->ipv6_linklocal), NULL);
//	if_get_ipv6(HOST_ETH_DEV, IPV6_ADDR_GLOBAL, ret_info->ipv6_addr[0], sizeof(ret_info->ipv6_addr[0]), &(ret_info->ipv6_prefix[0]));
	if_get_ipv6_all(HOST_ETH_DEV, IPV6_ADDR_GLOBAL, &addr_prefix, IPV6_STR_MAX, IPV6_ADDR_MAX);

	for( i=0; i< addr_prefix.count; i++)
	{
		memcpy(ret_info->ipv6_addr[i], addr_prefix.ipv6_addr[i], IPV6_STR_MAX);
		ret_info->ipv6_prefix[i] = addr_prefix.ipv6_prefix[i];
	}

	netif_ipv6_get_gateway(ret_info->ipv6_gateway);

	netif_ipv6_get_dns(&ipv6_dns);
	for( i=0; i< ipv6_dns.count; i++)
	{
		memcpy(ret_info->ipv6_dns[i], ipv6_dns.ipv6_dns[i], IPV6_STR_MAX);
	}
/*	
	netif_ipv6_get_dns(0, ret_info->ipv6_dns[0]);
	netif_ipv6_get_dns(1, ret_info->ipv6_dns[1]);
*/	
	return 1;
}

#if 0

Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo:2025598856 2700868    0    0    0     0          0         0 2025598856 2700868    0    0    0     0       0          0
  eth0:4254456546 22813178    0    0    0     0          0    294774 1149718543 25032466    0    0    0     0       0          0
  eth1:46859857  240385    0    0    0     0          0         0 21690505   31319    0    0    0     0       0          0
  sit0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
  		
#endif

gboolean nf_netif_get_stat( char *dev_name, NF_NETIF_GET_STAT *ret_info )
{
	
	gint ret = 0;	
	gchar buf[1024];
	
	FILE *file = NULL; 
	NF_NETIF_GET_STAT tmp;
	
	g_return_val_if_fail ( dev_name , FALSE );
	g_return_val_if_fail ( ret_info , FALSE );
		
	file = fopen( "/proc/net/dev", "r" );
	
	g_return_val_if_fail ( file , FALSE );

	memset( buf, 0x00, sizeof(buf) );
	memset( &tmp, 0x00, sizeof(NF_NETIF_GET_STAT) );
	memset( ret_info, 0x00, sizeof(NF_NETIF_GET_STAT) );

	if( !fgets(buf, sizeof(buf), file) )
	{
		g_warning("%s /proc/net/dev wrong format file 1", __FUNCTION__);
		goto parsing_error;
	}else{
		if( strstr( buf, "Receive" ) == NULL )
		{
			g_warning("%s /proc/net/dev wrong format file 2", __FUNCTION__);
			goto parsing_error;
		}
	}
	
	if( !fgets(buf, sizeof(buf), file) )
	{
		g_warning("%s /proc/net/dev wrong format file 3", __FUNCTION__);
		goto parsing_error;
	}else{
		if( strstr( buf, "compressed" ) == NULL )
		{
			g_warning("%s /proc/net/dev wrong format file 4", __FUNCTION__);
			goto parsing_error;
		}
	}
	
	ret = 0;
	while( fgets(buf, sizeof(buf), file) )
	{		
		char *tmp_delim = strchr(buf, ':');		
		if(tmp_delim) *tmp_delim = ' ';
		
		ret = sscanf( buf," %32s %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
						tmp.dev_name,
						&tmp.rx_byte,
						&tmp.rx_packet,
						&tmp.rx_error,
						&tmp.rx_drop,
						&tmp.rx_fifo,
						&tmp.rx_colls,
						&tmp.rx_carrier,
						&tmp.rx_compressed,
						&tmp.tx_byte,
						&tmp.tx_packet,
						&tmp.tx_error,
						&tmp.tx_drop,
						&tmp.tx_fifo,
						&tmp.tx_colls,
						&tmp.tx_carrier,
						&tmp.tx_compressed );
#if 0												
		g_message("ret[%d] [%s]", ret, buf);		
		nf_debug_hexdump( &tmp, sizeof(NF_NETIF_GET_STAT) );
#endif
		if( ret != 17 ) continue;
			
		if( strncmp(tmp.dev_name, dev_name, sizeof(tmp.dev_name)) == 0 )
		{
			memcpy( ret_info, &tmp, sizeof(NF_NETIF_GET_STAT));			
			g_get_current_time( &ret_info->tval);			

			ret = 1;
			break;
		}		
	}		

	fclose(file);	
	return ret;
	
parsing_error:
	
	fclose(file);
	return 0;
}


gboolean nf_netif_get_delta( char *dev_name, 
							NF_NETIF_GET_STAT *prev, 
							NF_NETIF_GET_STAT *next,
							NF_NETIF_GET_STAT *delta)
{
	
	NF_NETIF_GET_STAT cur_stat, delta_stat;
	guint64 time_diff =0;

	g_return_val_if_fail ( dev_name , FALSE );
	
	g_return_val_if_fail ( prev , FALSE );
	g_return_val_if_fail ( next , FALSE );
	g_return_val_if_fail ( delta , FALSE );
	
	if(nf_netif_get_stat( dev_name , &cur_stat))
	{
		time_diff = GTIMEVAL_TO_GUINT64(cur_stat.tval) - 
						GTIMEVAL_TO_GUINT64(prev->tval);
	
		GUINT64_TO_GTIMEVAL(time_diff, delta_stat.tval);
		
		time_diff = time_diff/1000000;				
				
		delta_stat.rx_byte       = (cur_stat.rx_byte       - prev->rx_byte      ) * 1000 / time_diff;
		delta_stat.rx_packet     = (cur_stat.rx_packet     - prev->rx_packet    ) * 1000 / time_diff;
		delta_stat.rx_error      = (cur_stat.rx_error      - prev->rx_error     ) * 1000 / time_diff;

		delta_stat.tx_byte       = (cur_stat.tx_byte       - prev->tx_byte      ) * 1000 / time_diff;
		delta_stat.tx_packet     = (cur_stat.tx_packet     - prev->tx_packet    ) * 1000 / time_diff;
		delta_stat.tx_error      = (cur_stat.tx_error      - prev->tx_error     ) * 1000 / time_diff;

#if 0
		delta_stat.rx_drop       = (cur_stat.rx_drop       - prev->rx_drop      ) * 1000 / time_diff;
		delta_stat.rx_fifo       = (cur_stat.rx_fifo       - prev->rx_fifo      ) * 1000 / time_diff;
		delta_stat.rx_colls      = (cur_stat.rx_colls      - prev->rx_colls     ) * 1000 / time_diff;
		delta_stat.rx_carrier    = (cur_stat.rx_carrier    - prev->rx_carrier   ) * 1000 / time_diff;
		delta_stat.rx_compressed = (cur_stat.rx_compressed - prev->rx_compressed) * 1000 / time_diff;

		delta_stat.tx_drop       = (cur_stat.tx_drop       - prev->tx_drop      ) * 1000 / time_diff;
		delta_stat.tx_fifo       = (cur_stat.tx_fifo       - prev->tx_fifo      ) * 1000 / time_diff;
		delta_stat.tx_colls      = (cur_stat.tx_colls      - prev->tx_colls     ) * 1000 / time_diff;
		delta_stat.tx_carrier    = (cur_stat.tx_carrier    - prev->tx_carrier   ) * 1000 / time_diff;
		delta_stat.tx_compressed = (cur_stat.tx_compressed - prev->tx_compressed) * 1000 / time_diff;
#endif		
		memcpy( next, &cur_stat, sizeof( NF_NETIF_GET_STAT ) );
		memcpy( delta, &delta_stat, sizeof( NF_NETIF_GET_STAT ) );			
		return 1;
	}	
	return 0;
}

char* nf_netif_get_eth_str(void)
{
	return _eth_dev;
}

#ifdef DEBUG_NETIF_JBSHELL

static char netif_get_help[] = "netif_get";
static int netif_get(int argc, char **argv)
{		
	gboolean ret;	
	NF_NETIF_GET_INFO ret_info;

	ret = nf_netif_get_info( &ret_info);
	g_print("nf_netif_get_info ret[%d]\n", ret);
	
	if(ret)
	{
		g_print("ipaddr     : %08x\n", ret_info.ipaddr       );
		g_print("gateway    : %08x\n", ret_info.gateway      );
		g_print("netmask    : %08x\n", ret_info.netmask      );
		g_print("broadcast  : %08x\n", ret_info.broadcast    );
		g_print("dnsserver1 : %08x\n", ret_info.dnsserver1   );
		g_print("dnsserver2 : %08x\n", ret_info.dnsserver2   );
		g_print("mac_addr   : %02x:%02x:%02x:%02x:%02x:%02x\n", 
						ret_info.mac_addr[0],
						ret_info.mac_addr[1],
						ret_info.mac_addr[2],
						ret_info.mac_addr[3],
						ret_info.mac_addr[4],
						ret_info.mac_addr[5]);
	}							
	return 0;
}
__commandlist(netif_get, "netif_get",  netif_get_help, netif_get_help);

static char netif_stat_help[] = "netif_stat";
static int netif_stat(int argc, char **argv)
{
	gboolean ret;	
	NF_NETIF_GET_STAT ret_info;

	ret = nf_netif_get_stat( _eth_dev, &ret_info);
	g_print("nf_netif_get_stat ret[%d]\n", ret);
	
	if(ret)
	{
		g_print("if[%s] tval[%d.%06d]\n", ret_info.dev_name, ret_info.tval.tv_sec, ret_info.tval.tv_usec);
		
		g_print("rx_byte       : %lld\n", ret_info.rx_byte       );
		g_print("rx_packet     : %lld\n", ret_info.rx_packet     );
		g_print("rx_error      : %lld\n", ret_info.rx_error      );
		g_print("rx_drop       : %lld\n", ret_info.rx_drop       );
		g_print("rx_fifo       : %lld\n", ret_info.rx_fifo       );
		g_print("rx_colls      : %lld\n", ret_info.rx_colls      );
		g_print("rx_carrier    : %lld\n", ret_info.rx_carrier    );
		g_print("rx_compressed : %lld\n", ret_info.rx_compressed );
		
		g_print("tx_byte       : %lld\n", ret_info.rx_byte       );
		g_print("tx_packet     : %lld\n", ret_info.rx_packet     );
		g_print("tx_error      : %lld\n", ret_info.rx_error      );
		g_print("tx_drop       : %lld\n", ret_info.rx_drop       );
		g_print("tx_fifo       : %lld\n", ret_info.rx_fifo       );
		g_print("tx_colls      : %lld\n", ret_info.rx_colls      );
		g_print("tx_carrier    : %lld\n", ret_info.rx_carrier    );
		g_print("tx_compressed : %lld\n", ret_info.rx_compressed );
		
	}					
	return 0;
}
__commandlist(netif_stat, "netif_stat",  netif_stat_help, netif_stat_help);



static char netif_delta_help[] = "netif_delta";
static int netif_delta(int argc, char **argv)
{
	static NF_NETIF_GET_STAT net_stat;
	static gint init = 0;	
		
	NF_NETIF_GET_STAT delta_stat;
	
	if(init == 0)
	{
		if( nf_netif_get_stat( _eth_dev, &net_stat) )
		{
			init = 1;
		}
		return 1;
	}
	
	// dev_name, prev, next, delta
	if( nf_netif_get_delta(_eth_dev, &net_stat, &net_stat, &delta_stat)  )
	{
		g_print("%s rx[%lld][%lld][%lld] tx[%lld][%lld][%lld] %d.%06d\n",  
					__FUNCTION__, 
					delta_stat.rx_byte, delta_stat.rx_packet, delta_stat.rx_error,
					delta_stat.tx_byte, delta_stat.tx_packet, delta_stat.tx_error,
					delta_stat.tval.tv_sec, delta_stat.tval.tv_usec);		
	}
	return 0;
}
__commandlist(netif_delta, "netif_delta",  netif_delta_help, netif_delta_help);


static char netif_dhcp_help[] = "netif_dhcp";
static int netif_dhcp(int argc, char **argv)
{

	if( nf_sysman_hotkey_is_nfs() )
	{
		g_warning("%s nfs mode!! skip init", __FUNCTION__);
		return 0;
	}

#ifdef USE_PROXY_SYSTEM
	proxy_system("killall -9 udhcpc", 1, 3);
#else
	system("killall -9 udhcpc");
#endif
	
	if(apply_dhcp() < 0) 
	{
		//perror("apply_dhcp()");
		err_msg("%s apply_dhcp() error", __FUNCTION__);
		return FALSE;
	}
	
	return 0;
}
__commandlist(netif_dhcp, "netif_dhcp",  netif_dhcp_help, netif_dhcp_help);


static char netif_static_ip_help[] = "netif_init_force";
static int netif_static_ip(int argc, char **argv)
{
	gboolean ret = 0;	
	
	nf_netif_init();
	return 0;
}
__commandlist(netif_static_ip, "netif_static_ip",  netif_static_ip_help, netif_static_ip_help);


#endif

static void nf_netif_dhcp_server_onoff()
{
	int i;
	int tmp;

	if(nf_sysdb_get_bool("net.eth2.dhcpsvr")) {
		//if(!nf_openmode_get_dhcpd_state()) {
			printf("\e[31mDHCP SERVER START\e[0m\n");
			nf_openmode_dhcpd_stop();
			nf_openmode_dhcpd_start();
			sleep(1);
		//}
#if 1
		if(nf_get_custom_mode()){
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
	}
	else
	{
		//if(nf_openmode_get_dhcpd_state()) {
		printf("\e[31mDHCP SERVER STOP\e[0m\n");
		nf_openmode_dhcpd_stop();
		sleep(1);
		
	}
}

void nf_netif_renew_dhcp()
{

	static guint prev_ip = 0;
	guint addr=0, netmask=0, gateway=0;
	guint dns1=0, dns2=0;
	gint ret=0;
				
	addr	= if_get_ip(_eth_dev);
	
	// FIXME 
	if( addr == 0xffffffff)  addr = 0;
		
	if( addr == prev_ip)
		return;
	else
		g_message("%s addr  [0x%08x]->[0x%08x]", __FUNCTION__, ntohl(prev_ip), ntohl(addr));


	g_message("%s sleep(3)", __FUNCTION__ );
	sleep(3); // FIXME  for dns setup delay

	res_init();

	netmask	= if_get_netmask(_eth_dev);

	// ����Ʈ���̰� �׻� �̷��� �ƴѵ�.
    //gateway = (addr & netmask) | 0x01000000;
    gateway = if_get_gateway();
        
    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);

	if( gateway == 0xffffffff)  gateway = 0;
	if( dns1 == 0xffffffff)  dns1 = 0;
	if( dns2 == 0xffffffff)  dns2 = 0;

	g_message("%s DHCP %s ==========================", __FUNCTION__, _eth_dev);
	g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
	g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
	g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
	g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
	g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
	g_message("%s ===================================", __FUNCTION__);
		
	nf_sysdb_set_uint(DHCP_IPADDR,		ntohl(addr) );
	nf_sysdb_set_uint(DHCP_GATEWAY,		ntohl(gateway) );
	nf_sysdb_set_uint(DHCP_SUBNET,		ntohl(netmask) );
	nf_sysdb_set_uint(DHCP_DNS1,		ntohl(dns1) );
	nf_sysdb_set_uint(DHCP_DNS2,		ntohl(dns2) );

#if 1
	nf_sysdb_set_uint(NET_IPADDR,	ntohl(addr) );
	nf_sysdb_set_uint(NET_GATEWAY,	ntohl(gateway) );
	nf_sysdb_set_uint(NET_SUBNET,	ntohl(netmask) );
	nf_sysdb_set_uint(NET_DNS1,		ntohl(dns1) );
	nf_sysdb_set_uint(NET_DNS2,		ntohl(dns2) );
#endif
	
	nf_notify_fire_params("net_ip_changed", addr,prev_ip,0,0);	
	prev_ip = addr;
			
}


static gint apply_dhcp()
{
	gchar cmd[256];
	guint addr=0, netmask=0, gateway=0;
	guint dns1=0, dns2=0;
	
	gint ret=0;
	
	_dhcp_start();

	addr	= if_get_ip(_eth_dev);
	netmask	= if_get_netmask(_eth_dev);

	// ����Ʈ���̰� �׻� �̷��� �ƴѵ�.
    //gateway = (addr & netmask) | 0x01000000;
    gateway = if_get_gateway();
        
    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);

	g_message("%s DHCP %s ==========================", __FUNCTION__, _eth_dev);
	g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
	g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
	g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
	g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
	g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
	g_message("%s ===================================", __FUNCTION__);
		
	nf_sysdb_set_uint(DHCP_IPADDR,		ntohl(addr) );
	nf_sysdb_set_uint(DHCP_GATEWAY,		ntohl(gateway) );
	nf_sysdb_set_uint(DHCP_SUBNET,		ntohl(netmask) );
	nf_sysdb_set_uint(DHCP_DNS1,		ntohl(dns1) );
	nf_sysdb_set_uint(DHCP_DNS2,		ntohl(dns2) );

#if 1
	nf_sysdb_set_uint(NET_IPADDR,	ntohl(addr) );
	nf_sysdb_set_uint(NET_GATEWAY,	ntohl(gateway) );
	nf_sysdb_set_uint(NET_SUBNET,	ntohl(netmask) );
	nf_sysdb_set_uint(NET_DNS1,		ntohl(dns1) );
	nf_sysdb_set_uint(NET_DNS2,		ntohl(dns2) );
#endif

    sprintf(cmd, "hostname NFDVR");
#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif


	g_message ( "NF_HOST done");

	return 0;
}

#if 0
char *my_inet_ntoa_r(struct in_addr ina, char *buf)
{
	unsigned char *ucp = (unsigned char *)&ina;

	sprintf(buf, "%d.%d.%d.%d",
		ucp[0] & 0xff,
		ucp[1] & 0xff,
		ucp[2] & 0xff,
		ucp[3] & 0xff);
	return buf;
}
#endif
// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
gint nf_netif_set_ipaddr(gint dhcp_on, guint ipaddr, guint netmask)
{
	return 1;
}

gint nf_netif_apply_dns()
{
	gchar cmd[256];
	guint addr=0, gateway=0, netmask=0;
	guint dns1=0, dns2=0;
	gint is_dhcp=0;
	gchar buff_gw[32];
	gint ret=0;
	
#if 1
	if( nf_sysman_hotkey_is_nfs() )
	{
		g_warning("%s nfs mode!! skip init", __FUNCTION__);
		nf_netif_renew_dhcp();  // for apply nfs network config		
		return 0;
	}
#endif

	is_dhcp = nf_sysdb_get_bool( "onvif.common.dns_dhcp");

	if(is_dhcp)  
	{	
		if(apply_dhcp_dns() < 0) 
		{
			err_msg("%s apply_dhcp_dns() error", __FUNCTION__);
			return FALSE;
		}
	}
	else 
	{		
		// static
		addr 	= htonl( nf_sysdb_get_uint(NET_IPADDR) );
		gateway = htonl( nf_sysdb_get_uint(NET_GATEWAY) );
		netmask = htonl( nf_sysdb_get_uint(NET_SUBNET) );

		dns1 = htonl( nf_sysdb_get_uint(NET_DNS1) );
		dns2 = htonl( nf_sysdb_get_uint(NET_DNS2) );
		g_message("%s STATIC %s dns ================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);

		if (dns1 || dns2)
		{
			if (apply_dns(dns1, dns2) < 0)
			{
				perror("apply_dns()");
				return FALSE;
			}
		}
		// get network information.
		res_init();
	}

	//nf_ntpd_restart();

	return TRUE;
}


#ifdef ENABLE_IPV6

#define MAX_IPV6_ADDR_CNT 	4
#define MAX_IPV6_DNS_CNT 	2

static int apply_ipv6()
{
	guint ipv6_mode	= 0;

	char  ipv6_addr_ll[64];		
	char  ipv6_gw[64];
	char  ipv6_addr[MAX_IPV6_ADDR_CNT][64];
	int	  ipv6_prefix[MAX_IPV6_ADDR_CNT];
	char  ipv6_dns[MAX_IPV6_DNS_CNT][64];

	memset ( ipv6_addr_ll, 0x00, sizeof(ipv6_addr_ll));

	memset ( ipv6_addr, 0x00, sizeof(ipv6_addr));
	memset ( ipv6_prefix, 0x00, sizeof(ipv6_prefix));	

	memset ( ipv6_dns, 0x00, sizeof(ipv6_dns));
	memset ( ipv6_gw, 0x00, sizeof(ipv6_gw));		

	_dhcp_ipv6_stop();
					
	// mode 0:disable 1:static 2:dhcp
	ipv6_mode = nf_sysdb_get_uint(IPV6_SYSDB_KEY_USING);
	//ipv6_mode = 1;
	if( ipv6_mode )	{
		
		netif_ipv6_enable(1);

		// get linklocal addr			
		if_get_ipv6(HOST_ETH_DEV, IPV6_ADDR_LINKLOCAL, ipv6_addr_ll, sizeof(ipv6_addr_ll), NULL);	
		nf_sysdb_set_str( IPV6_SYSDB_KEY_IPADDR_LL, ipv6_addr_ll);
									
		if( ipv6_mode == 1 )  { // static

			int idx=0;								
					
			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR0 ) ) 
				strncpy(ipv6_addr[0], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR0 ), 63);	
			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR1 ) ) 
				strncpy(ipv6_addr[1], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR1 ), 63);
			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR2 ) ) 
				strncpy(ipv6_addr[2], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR2 ), 63);
			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR3 ) ) 
				strncpy(ipv6_addr[3], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_IPADDR3 ), 63);

			ipv6_prefix[0] = nf_sysdb_get_uint( IPV6_SYSDB_KEY_PREFIX0);
			ipv6_prefix[1] = nf_sysdb_get_uint( IPV6_SYSDB_KEY_PREFIX1);
			ipv6_prefix[2] = nf_sysdb_get_uint( IPV6_SYSDB_KEY_PREFIX2);
			ipv6_prefix[3] = nf_sysdb_get_uint( IPV6_SYSDB_KEY_PREFIX3);

			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_GATEWAY ) ) 
				strncpy(ipv6_gw, nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_GATEWAY ), 63);	

			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_DNS1 ) ) 
				strncpy(ipv6_dns[0], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_DNS1 ), 63);	
			if( nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_DNS2 ) ) 
				strncpy(ipv6_dns[1], nf_sysdb_get_str_nocopy( IPV6_SYSDB_KEY_DNS2 ), 63);	
					
			for(idx=0;idx<	MAX_IPV6_ADDR_CNT;++idx){
				
				if(	strlen(ipv6_addr[idx]) > 0) 
				{
					g_message("[%s]ipv6[idx=%d] setting => %s", __FUNCTION__, idx, ipv6_addr[idx]);						
					netif_ipv6_add_ip(HOST_ETH_DEV, ipv6_addr[idx], ipv6_prefix[idx]);
				}
				else
					g_message("[%s]ipv6[idx=%d] not setting", __FUNCTION__, idx);					
			}
							
			if(	strlen(ipv6_gw) > 0) 
				netif_ipv6_add_gw(HOST_ETH_DEV, ipv6_gw);

			for(idx=0;idx<	MAX_IPV6_DNS_CNT;++idx){

				if(	strlen(ipv6_dns[idx]) > 0) 
					netif_ipv6_add_dns( ipv6_dns[idx]);
			}

		}else{	// dhcp
//			g_warning("%s not_impl..!!"	, __FUNCTION__);
			_dhcp_ipv6_start();
			sleep(5);			
		}			

	}else{			
		netif_ipv6_enable(0);	// disable
	}									
	
	return 0;
}

static int netif_ipv6_enable(int is_enable)
{

	char cmd[256];
	gint ret;
	
	if( is_enable ) // kernel ipv6 workarround!!
	{
		snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.all.disable_ipv6=%d",  1 );	
		ret = proxy_system(cmd, 1, 3);

		snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.default.disable_ipv6=%d", 1 );	
		ret = proxy_system(cmd, 1, 3);		
		
		snprintf(cmd, sizeof(cmd), "ip -6 addr add fe80::21b:11ff:fec4:fc3/64  dev %s ",HOST_ETH_DEV);	
		ret = proxy_system(cmd, 1, 3);				
	}

	snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.all.disable_ipv6=%d",  is_enable ? 0:1 );	
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.default.disable_ipv6=%d",  is_enable ? 0:1 );	
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	return 0;
}

/*
	# /sbin/ip -6 addr add 2001:0db8:0:f101::1/64 dev eth0
	# /sbin/ip -6 addr del 2001:0db8:0:f101::1/64 dev eth0
	# /sbin/ip -6 route add default via 2001:0db8:0:f101::1
	# /sbin/ip -6 route del default via 2001:0db8:0:f101::1
*/
static int netif_ipv6_add_ip( const char *dev, const char *addr, int prefix_len)
{
	char cmd[256];
	gint ret;
	
	if( prefix_len )
		snprintf(cmd, sizeof(cmd), "ip -6 addr add %s/%d dev %s", addr, prefix_len, dev );	
	else 
		snprintf(cmd, sizeof(cmd), "ip -6 addr add %s dev %s", addr, dev );	
		
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif
		
	return 0;
}


static int netif_ipv6_add_dns(  const char *addr)
{
	char cmd[256];
	gint ret;
	
	snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", addr);
			
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif
		
	return 0;
}

static int netif_ipv6_del_ip( const char *dev, const char *addr, int prefix_len)
{
	char cmd[256];
	gint ret;
	
	if( prefix_len )
		snprintf(cmd, sizeof(cmd), "ip -6 addr del %s/%d dev %s", addr, prefix_len, dev );	
	else 
		snprintf(cmd, sizeof(cmd), "ip -6 addr del %s dev %s", addr, dev );	
		
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif
	
	return 0;
}

static int netif_ipv6_add_gw( const char *dev, const char *addr)
{
	char cmd[256];
	gint ret;

	if( dev )
		snprintf(cmd, sizeof(cmd), "ip -6 route add default via %s dev %s", addr, dev );
	else 
		snprintf(cmd, sizeof(cmd), "ip -6 route add default via %s", addr);
				
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	return 0;
}

static int netif_ipv6_del_gw( const char *dev, const char *addr)
{
	char cmd[256];
	gint ret;
	
	if( dev )
		snprintf(cmd, sizeof(cmd), "ip -6 route del default via %s dev %s", addr, dev );
	else 
		snprintf(cmd, sizeof(cmd), "ip -6 route del default via %s", addr);
		
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	return 0;
}

static guint netif_ipv6_get_dns(IPV6_DNS *ipv6_dns)
{	
    FILE *f;
	char serv_name[32];
	char serv_addr[IPV6_STR_MAX];

	int count = 0;
	
    f = fopen("/etc/resolv.conf", "r");
    if (f == NULL) {
        return 0;
    }

    while (2 == fscanf(f, "%s %s", serv_name, serv_addr)){
		if( strcmp(serv_name, "nameserver") == 0 )
		{
			if( strchr(serv_addr, ':') != NULL )
			{
				memcpy(ipv6_dns->ipv6_dns[count], serv_addr, IPV6_STR_MAX);
				
				count++;
				
				if( count >= 2 )
					break;
			}
		}
    }

	ipv6_dns->count = count;

    fclose(f); 

	return 0;
}

#define GATEWAY_MARK_START	"default via"

static guint netif_ipv6_get_gateway(char *ipv6_gateway)
{
	char cmd[256];
	gint ret;
	
	GTimeVal tval;
	gchar result_file[128];

    gchar  *contents = NULL;
    GError *error = NULL;
    gsize  length = 0;
	gchar *mark_start;

	gettimeofday(&tval, NULL);
	snprintf(result_file, sizeof(result_file), "/tmp/ipv6_gate_%ld_%06ld", tval.tv_sec, tval.tv_usec);
	snprintf(cmd, sizeof(cmd), "ip -6 route > %s", result_file);

#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

    if (!g_file_get_contents (result_file, &contents, &length, &error))
    {
            g_warning("%s\n", error->message);
            g_error_free (error);

            if(contents)
                    g_free(contents);
            ret = -1;
    }
    else
    {
        if(contents)
        {
			mark_start = strstr( contents, GATEWAY_MARK_START);
            if( mark_start == NULL )
            {
                    ret = -1;
            }
            else
            {
                gchar *tmp_str;
                int conflict;
				int i;

				gchar *start;
				gchar *end;

                tmp_str = mark_start + strlen(GATEWAY_MARK_START);

                while(isspace(*tmp_str))
                        tmp_str = tmp_str + 1;

				start = tmp_str;
				end = strchr(tmp_str, ' ');

				memcpy(ipv6_gateway, start, (end - start));
            }

            g_free(contents);
    	}
	    else
	    {
	            ret = -1;
	    }
    }

	remove(result_file);

	g_message("[%s][%s]", __FUNCTION__, ipv6_gateway);

	return 0;
}

static int if_get_ipv6(const char *dev, int filter_scope, char *addr, int addr_len, guint *prefix_val)
{

    FILE *f;
    int ret, scope, prefix;
    int dummy;
    unsigned char ipv6[16];
    char dname[IFNAMSIZ];
    char address[INET6_ADDRSTRLEN];
    char *scopestr;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return 0;
    }

    while (21 == fscanf(f,
                        "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %x %x %x %x %s",
                        &ipv6[0],
                        &ipv6[1],
                        &ipv6[2],
                        &ipv6[3],
                        &ipv6[4],
                        &ipv6[5],
                        &ipv6[6],
                        &ipv6[7],
                        &ipv6[8],
                        &ipv6[9],
                        &ipv6[10],
                        &ipv6[11],
                        &ipv6[12],
                        &ipv6[13],
                        &ipv6[14],
                        &ipv6[15],
                        &dummy,
                        &prefix,
                        &scope,
                        &dummy,
                        dname)) {
		
        if (strcmp(dev, dname) != 0) {
			continue;
        }

		if ( filter_scope != scope ) {
			continue;
		}			

		memset( address, 0x00, sizeof(address));
        if (inet_ntop(AF_INET6, ipv6, address, sizeof(address)) == NULL) {
			continue;
        }
        strncpy( addr, address, addr_len);

		if( prefix_val )
			*prefix_val = prefix;		
		
		++ret;		
        switch (scope) {
        case IPV6_ADDR_GLOBAL:
            scopestr = "Global";
            break;
        case IPV6_ADDR_LINKLOCAL:
            scopestr = "Link";
            break;
        case IPV6_ADDR_SITELOCAL:
            scopestr = "Site";
            break;
        case IPV6_ADDR_COMPATv4:
            scopestr = "Compat";
            break;
        case IPV6_ADDR_LOOPBACK:
            scopestr = "Host";
            break;
        default:
            scopestr = "Unknown";
        }

		g_message("%s IPv6 address: %s, prefix: %d, scope: %s\n", __FUNCTION__, addr, prefix, scopestr);

    }

    fclose(f);        
    return ret;
    
}

static int if_get_ipv6_all(const char *dev, int filter_scope, IPV6_ADDR_PREFIX *addr, int addr_len, int max_count)
{
    FILE *f;
    int ret, scope, prefix;
    int dummy;
    unsigned char ipv6[16];
    char dname[IFNAMSIZ];
    char address[INET6_ADDRSTRLEN];
    char *scopestr;
	int count = 0;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return 0;
    }

    while (21 == fscanf(f,
                        "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %x %x %x %x %s",
                        &ipv6[0],
                        &ipv6[1],
                        &ipv6[2],
                        &ipv6[3],
                        &ipv6[4],
                        &ipv6[5],
                        &ipv6[6],
                        &ipv6[7],
                        &ipv6[8],
                        &ipv6[9],
                        &ipv6[10],
                        &ipv6[11],
                        &ipv6[12],
                        &ipv6[13],
                        &ipv6[14],
                        &ipv6[15],
                        &dummy,
                        &prefix,
                        &scope,
                        &dummy,
                        dname)) {
		
        if (strcmp(dev, dname) != 0) {
			continue;
        }

		if ( filter_scope != scope ) {
			continue;
		}			

		memset( address, 0x00, sizeof(address));
        if (inet_ntop(AF_INET6, ipv6, address, sizeof(address)) == NULL) {
			continue;
        }

        strncpy( addr->ipv6_addr[count], address, addr_len);
		addr->ipv6_prefix[count] = prefix;

		count++;
		++ret;		
        switch (scope) {
        case IPV6_ADDR_GLOBAL:
            scopestr = "Global";
            break;
        case IPV6_ADDR_LINKLOCAL:
            scopestr = "Link";
            break;
        case IPV6_ADDR_SITELOCAL:
            scopestr = "Site";
            break;
        case IPV6_ADDR_COMPATv4:
            scopestr = "Compat";
            break;
        case IPV6_ADDR_LOOPBACK:
            scopestr = "Host";
            break;
        default:
            scopestr = "Unknown";
        }

		g_message("%s IPv6 address: %s, prefix: %d, scope: %s\n", __FUNCTION__, address, prefix, scopestr);

		if( count >= max_count )
			break;
    }

	addr->count = count;

    fclose(f);        
    return ret;
    
}

#endif 

gint run_cmd(char *func_name, char *cmd)
{
	gint ret;
//	g_printf("%s() : [%s]\n", func_name, cmd);
#ifdef USE_PROXY_SYSTEM
   ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	return ret;
}
gint nf_netif_apply_gateway()
{
	gchar cmd[256];
	guint addr=0, gateway=0, netmask=0;
	guint dns1=0, dns2=0;
	gint is_dhcp=0;
	gchar buff_gw[32];
	gint ret=0;
	
#if 1
	if( nf_sysman_hotkey_is_nfs() )
	{
		g_warning("%s nfs mode!! skip init", __FUNCTION__);
		nf_netif_renew_dhcp();  // for apply nfs network config		
		return 0;
	}
#endif

	is_dhcp = nf_sysdb_get_bool( "onvif.common.gw_dhcp");

#ifdef USE_PROXY_SYSTEM
	ret = proxy_system("route del default gw 0.0.0.0", 1, 5);
#else
	ret = system("route del default gw 0.0.0.0");
	pr_exit(ret);
#endif

	if(is_dhcp)  
	{	
		if(apply_dhcp_gateway() < 0) 
		{
			err_msg("%s apply_dhcp_gateway() error", __FUNCTION__);
			return FALSE;
		}
	}
	else 
	{		
		// static
		addr 	= htonl( nf_sysdb_get_uint(NET_IPADDR) );
		gateway = htonl( nf_sysdb_get_uint(NET_GATEWAY) );
		netmask = htonl( nf_sysdb_get_uint(NET_SUBNET) );

		dns1 = htonl( nf_sysdb_get_uint(NET_DNS1) );
		dns2 = htonl( nf_sysdb_get_uint(NET_DNS2) );
		g_message("%s STATIC %s gateway ================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);

		my_inet_ntoa(buff_gw, sizeof(buff_gw), (gateway) );

#if 1
	//default gateway ������� �ʴ� ���� ���� (SWINDTWO-36)
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "route del default");
	run_cmd(__FUNCTION__, cmd);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "route add %s %s", buff_gw, _eth_dev);
	run_cmd(__FUNCTION__, cmd);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "route add default gw %s %s", buff_gw, _eth_dev);
	run_cmd(__FUNCTION__, cmd);
#else
		sprintf(cmd, "route add default gw %s", buff_gw);
#ifdef USE_PROXY_SYSTEM
		ret = proxy_system(cmd, 1, 3);
#else
		ret = system(cmd);
		pr_exit(ret);
#endif
#endif
	}

	//nf_ntpd_restart();

	return TRUE;
}

static gint apply_dhcp_ipaddr()
{
	gchar cmd[256];
	guint addr=0, netmask=0, gateway=0;
	guint dns1=0, dns2=0;
	
	gint ret=0;
	
#ifdef USE_PROXY_SYSTEM
 	//ret = proxy_system("udhcpc -q -n", 1, 10);
 	ret = proxy_system("udhcpc -n -s /NFDVR/udhcpc.ipaddr.script", 1, 10);
#else
	ret = system("udhcpc -n -s /NFDVR/udhcpc.ipaddr.script");
	pr_exit(ret);
#endif

	// get network information.
	res_init();

	addr	= if_get_ip(_eth_dev);
	netmask	= if_get_netmask(_eth_dev);

	// ????Ʈ???̰? ?׻? ?̷??? ?ƴѵ?.
    //gateway = (addr & netmask) | 0x01000000;
    gateway = if_get_gateway();
        
    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);


	if( addr == 0 || netmask == 0 ){
#ifndef NVR
		 //apply_pelco_ZAC();
#endif
	}
	else {
		g_message("%s DHCP %s ipaddr ===================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);
			
		nf_sysdb_set_uint(DHCP_IPADDR,		ntohl(addr) );
		nf_sysdb_set_uint(DHCP_SUBNET,		ntohl(netmask) );

		nf_sysdb_set_uint(NET_IPADDR,	ntohl(addr) );
		nf_sysdb_set_uint(NET_SUBNET,	ntohl(netmask) );
	}

	return 0;
}

static gint apply_dhcp_gateway()
{
	gchar cmd[256];
	guint addr=0, netmask=0, gateway=0;
	guint dns1=0, dns2=0;
	
	gint ret=0;

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system("udhcpc -n -s /NFDVR/udhcpc.gw.script", 1, 10);
#else
	ret = system("udhcpc -n -s /NFDVR/udhcpc.gw.script");
	pr_exit(ret);
#endif

	// get network information.
	res_init();

	addr	= if_get_ip(_eth_dev);
	netmask	= if_get_netmask(_eth_dev);
    gateway = if_get_gateway();
        
    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);

	if( addr == 0 || netmask == 0 || gateway == 0){
#ifndef NVR
		 //apply_pelco_ZAC();
#endif
	}
	else {
		g_message("%s DHCP %s gateway =================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);
			
		nf_sysdb_set_uint(DHCP_GATEWAY,		ntohl(gateway) );
	
		nf_sysdb_set_uint(NET_GATEWAY,	ntohl(gateway) );
	}

	return 0;
}

static gint apply_dhcp_dns()
{
	gchar cmd[256];
	guint addr=0, netmask=0, gateway=0;
	guint dns1=0, dns2=0;
	
	gint ret=0;

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system("udhcpc -n -s /NFDVR/udhcpc.dns.script", 1, 10);
#else
	ret = system("udhcpc -n -s /NFDVR/udhcpc.dns.script");
	pr_exit(ret);
#endif

	// get network information.
	res_init();

	addr	= if_get_ip(_eth_dev);
	netmask	= if_get_netmask(_eth_dev);

    gateway = if_get_gateway();
        
    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);


	if( addr == 0 || netmask == 0 || gateway == 0){
#ifndef NVR
		 //apply_pelco_ZAC();
#endif
	}
	else {
		g_message("%s DHCP %s DNS ======================", __FUNCTION__, _eth_dev);
		g_message("%s addr  [0x%08x]", __FUNCTION__, ntohl(addr));
		g_message("%s gw    [0x%08x]", __FUNCTION__, ntohl(gateway));
		g_message("%s subnet[0x%08x]", __FUNCTION__, ntohl(netmask));
		g_message("%s dns1  [0x%08x]", __FUNCTION__, ntohl(dns1));
		g_message("%s dns2  [0x%08x]", __FUNCTION__, ntohl(dns2));
		g_message("%s ===================================", __FUNCTION__);
			
		nf_sysdb_set_uint(DHCP_DNS1,		ntohl(dns1) );
		nf_sysdb_set_uint(DHCP_DNS2,		ntohl(dns2) );

		nf_sysdb_set_uint(NET_DNS1,		ntohl(dns1) );
		nf_sysdb_set_uint(NET_DNS2,		ntohl(dns2) );
	}

	return 0;
}
#endif
// onvif_porting

static void my_inet_ntoa( gchar *buff, guint buff_len, guint addr)
{
	struct sockaddr_in in_addr;	
	
	g_return_if_fail( buff );
	
	in_addr.sin_addr.s_addr = addr;	
	snprintf(buff, buff_len, inet_ntoa( in_addr.sin_addr ) );
		
}

static gint apply_ethernet(guint addr, guint gw, guint netmask)
{
	gchar cmd[256];
	guint broadcast;
	gint ret;

	gchar buff_addr[32];
	gchar buff_netmask[32];
	gchar buff_broadcast[32];
	gchar buff_gw[32];
	
    sprintf(cmd, "ifconfig %s down", _eth_dev);
#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif

    broadcast = if_get_broadcast(addr, netmask);
	
	my_inet_ntoa(buff_addr, sizeof(buff_addr), (addr) );
	my_inet_ntoa(buff_netmask, sizeof(buff_netmask), (netmask) );
	my_inet_ntoa(buff_broadcast, sizeof(buff_broadcast), (broadcast) );
    
    sprintf(cmd, "ifconfig %s %s netmask %s broadcast %s",
        _eth_dev, buff_addr, buff_netmask, buff_broadcast);

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif
// SWPFOURCE-1377
	sprintf(cmd, "route del default");

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif

	my_inet_ntoa(buff_gw, sizeof(buff_gw), (gw) );
    sprintf(cmd, "route add default gw %s", buff_gw);

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif

    sprintf(cmd, "hostname NFDVR");
#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
    ret = system(cmd);
	pr_exit(ret);
#endif


	return 0;
}

//LAN2(LOCAL) static ip setting
static gint apply_ethernet_lan2(guint addr, guint gw, guint netmask)
{
	gchar cmd[256];
	guint broadcast;
	gint ret;
	static int is_mac=0;

	gchar buff_addr[32];
	gchar buff_netmask[32];
	gchar buff_broadcast[32];
	guchar buff_mac[32];
	guint eth_high=0, eth_low=0;

	sprintf(cmd, "ifconfig %s down", HUB_ETH_DEV); //local network down
#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif

	if(is_mac == 0)
	{
		srand(time(NULL) | getpid());
		eth_high = (rand() & 0xfeff) | 0x0200;
		eth_low = rand();
		sprintf(buff_mac,"%02lx:%02lx:%02lx:%02lx:%02lx:%02lx", \
		eth_high >> 8, eth_high & 0xff, eth_low >> 24, \
		(eth_low >> 16) & 0xff, (eth_low >> 8) & 0xff, eth_low & 0xff);

		snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s", HUB_ETH_DEV,buff_mac);
#ifdef USE_PROXY_SYSTEM
		ret = proxy_system(cmd, 1, 3);
#else
		ret = system(cmd);
		pr_exit(ret);
#endif
		
		is_mac = 1;
	}

	broadcast = if_get_broadcast(addr, netmask);
	
	my_inet_ntoa(buff_addr, sizeof(buff_addr), (addr) );
	my_inet_ntoa(buff_netmask, sizeof(buff_netmask), (netmask) );
	my_inet_ntoa(buff_broadcast, sizeof(buff_broadcast), (broadcast) );
    
	sprintf(cmd, "ifconfig %s %s netmask %s broadcast %s",
	HUB_ETH_DEV, buff_addr, buff_netmask, buff_broadcast);

#ifdef USE_PROXY_SYSTEM
 	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif
/*
	sprintf(cmd, "hostname NFDVR");
#ifdef USE_PROXY_SYSTEM
	ret = proxy_system(cmd, 1, 3);
#else
	ret = system(cmd);
	pr_exit(ret);
#endif*/

	return 0;
}

static gint apply_dns(guint dns1, guint dns2)
{
	char cmd[256];
	gint ret;

	gchar buff_dns1[32];
	gchar buff_dns2[32];
	
//	if (dns1) {
		my_inet_ntoa(buff_dns1, sizeof(buff_dns1), (dns1) );
		snprintf(cmd, sizeof(cmd), "echo nameserver %s > /etc/resolv.conf", buff_dns1);

#ifdef USE_PROXY_SYSTEM
	 	ret = proxy_system(cmd, 1, 3);
#else
	    ret = system(cmd);
		pr_exit(ret);
#endif
			
//	}
//	if (dns2) {
		my_inet_ntoa(buff_dns2, sizeof(buff_dns2), (dns2) );
		snprintf(cmd, sizeof(cmd), "echo nameserver %s >> /etc/resolv.conf", buff_dns2);

#ifdef USE_PROXY_SYSTEM
	 	ret = proxy_system(cmd, 1, 3);
#else
	    ret = system(cmd);
		pr_exit(ret);
#endif

//	}

	return 0;
}

static guint if_get_ip(const char *dev)
{
    int   sockfd;
    unsigned long addr = (unsigned long)-1;

    struct ifreq ifr;
    struct sockaddr_in *sap;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return ((unsigned long)-1);
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    }
    else {
        sap  = (struct sockaddr_in*)&ifr.ifr_addr;
        addr = *((unsigned long*)&sap->sin_addr);
    }
    close(sockfd);

    return (addr);
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

static guint if_get_broadcast(guint ip, guint netmask) 
{
    guint broadcast;

    broadcast = ~netmask;
    broadcast |= ip;

    return broadcast;
}


static gint if_get_mac(const char *dev, unsigned char *buff)
{
    int   i, sockfd;
    unsigned char *pdata;

    struct ifreq  ifr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
    	close(sockfd);
        return -2;
    }

    pdata = (unsigned char*)ifr.ifr_hwaddr.sa_data;

    for (i = 0; i < 6; i++)
        buff[i] = *pdata++;

    close(sockfd);

    return 0;
}

static guint if_get_netmask(const char *dev)
{
    int   sockfd;
    unsigned long addr = (unsigned long)-1;

    struct ifreq ifr;
    struct sockaddr_in *sap;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("create socket");

        return ((unsigned long)-1);
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';
    ifr.ifr_netmask.sa_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
        perror("Unable to read subnet mask!");
    }
    else {
        sap  = (struct sockaddr_in*)&ifr.ifr_netmask;
        addr = *((unsigned long*)&sap->sin_addr);
    }

    close(sockfd);

    return (addr);
}

static guint if_get_dns(int index)
{
	struct sockaddr_in def = _res.nsaddr_list[index];
    char *ip = inet_ntoa(def.sin_addr);
#if 0  	
  	g_message("DNS[%d] IP = %s  [0x%08x]", index, ip, def.sin_addr); 
#endif  	
    return *(unsigned int*)&def.sin_addr;	
}

#define IPV4_LINK_LO_RETRY_MAX	(256)
#define IPV4_LINK_LO_NETMASK	"255.255.0.0"
static int netif_ipv4_link_local_enable(const char *dev)
{
	int rtn = FALSE;
	int idx = 0;
	char ip_buf[32] = {0};
	char link_lo_dev[32] = {0};
	size_t buf_size = 0;
	unsigned char dev_mac[6] = {0};
	char mac_str[32] = {0};
	unsigned int seed = 0;

	printf("%s : start\n", __func__);

	if(dev == NULL)
	{
		goto ends_label;
	}

	sleep(3);

	// set seed to mac address
	memset(dev_mac, 0x00, sizeof(dev_mac));
	if(if_get_mac(dev, dev_mac) != 0)
	{
		struct timespec ts;

		printf("%s if_get_mac fail(dev:%s)\n", __func__, dev);

		memset(&ts, 0x00, sizeof(ts));
		clock_gettime(CLOCK_REALTIME, &ts);
		seed = (unsigned int)(ts.tv_nsec);
	}
	else
	{
		snprintf(mac_str, sizeof(mac_str), "%02x%02x%02x%02x%02x%02x",
				dev_mac[0], dev_mac[1], dev_mac[2], 
				dev_mac[3], dev_mac[4], dev_mac[5]);

		// itx address
		for(idx = 11; idx >= 6; idx--)
		{
			seed = (unsigned int)*(mac_str+idx) + (unsigned int)(seed*31);
		}
		// oui address
		for(idx = 5; idx >= 0; idx--)
		{
			seed = (unsigned int)*(mac_str+idx) + (unsigned int)(seed);
		}

		printf("%s dev:%s mac:%s seed:%u\n", __func__, dev, mac_str, seed);
	}
	srand(seed);

	buf_size = sizeof(ip_buf);
	snprintf(link_lo_dev, sizeof(link_lo_dev), "%s:0", dev);

	for(idx = 0; idx < IPV4_LINK_LO_RETRY_MAX; idx++)
	{
		memset(ip_buf, 0x00, buf_size);
		if(netif_ipv4_link_local_make_ip(ip_buf, buf_size) != TRUE)
		{
			printf("%s make link-local ip fail\n", __func__);
			usleep(1000);
			continue;
		}

		if(netif_ipv4_ip_conflict(dev, ip_buf) > 0)
		{
			printf("%s link-local ip conflict(ip:%s)\n", __func__, ip_buf);
			usleep(1000);
			continue;
		}

		if(netif_ipv4_add_ip(link_lo_dev, ip_buf, IPV4_LINK_LO_NETMASK) != TRUE)
		{
			printf("%s dev enable fail(dev:%s,ip:%s)\n", __func__, link_lo_dev, ip_buf);
			usleep(1000);
			continue;
		}
		else
		{
			printf("%s dev:%s link_local address:%s enable\n", __func__, link_lo_dev, ip_buf);
			rtn = TRUE;
			goto ends_label;
		}
	}

ends_label:

	return rtn;
}

static int netif_ipv4_link_local_make_ip(char *ip_buf, size_t buf_size)
{
	int rtn = FALSE;
	int addr[4] = {0};

	if(ip_buf == NULL)
	{
		goto ends_label;
	}

	memset(addr, 0x00, sizeof(addr));

	addr[0] = 169;
	addr[1] = 254;
	addr[2] = (rand()%254)+1; // range 1 ~ 254 ( 1 or 255 is pre-reserved )
	addr[3] = (rand()%254)+2; // range 2 ~ 255 ( 1,0 may risk overlapping with the gateway )

	snprintf(ip_buf, buf_size, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);

	rtn = TRUE;

ends_label:

	return rtn;
}

#define CONFLICT_STR	"Received"
/* return value is conflict received count */
static size_t netif_ipv4_ip_conflict(const char *dev, const char *ip)
{
	char cmd[256] = {0};
	char line[1024] = {0};
	FILE *fp = NULL;
	int fd = -1;
	char *ptr = NULL;
	size_t recv_cnt = 0;

	if(dev == NULL || ip == NULL)
	{
		goto ends_label;
	}

	snprintf(cmd, sizeof(cmd), "arping -c 1 -w 1 -I %s -D %s", dev, ip);

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		printf("%s proxy_popen fail(cmd:%s)\n", __func__, cmd);
		goto ends_label;
	}

	while(fgets(line, sizeof(line), fp) != NULL)
	{
		printf("%s : arping result print\n%s\n", __func__, line);

		ptr = strstr(line, CONFLICT_STR);
		if(ptr == NULL)
		{
			continue;
		}

		ptr += strlen(CONFLICT_STR);

		while(*ptr != 0)
		{
			if(isspace(*ptr) == 0) 
			{
				break;
			}
			else
			{
				ptr++;
			}
		}

		if(*ptr != 0)
		{
			recv_cnt = strtol(ptr, NULL, 10);
		}
	}

ends_label:

	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}

	return recv_cnt;
}

static int netif_ipv4_add_ip(const char *dev, const char *ip, const char *netmask)
{
	int rtn = FALSE;
	char cmd[256] = {0};

	if(dev == NULL || ip == NULL || netmask == NULL)
	{
		goto ends_label;
	}

	snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s\n", dev, ip, netmask);
	proxy_system((const char *)cmd, 1, 3);

	rtn = TRUE;

ends_label:

	return rtn;
}

// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
guint ext_if_get_dns(int index)
{
	return if_get_dns(index);
}
#endif
// onvif_porting

static void pr_exit(int status)
{
	if(WIFEXITED(status)) {
		fprintf(stderr,"normal termination, exit status = %d\n",WEXITSTATUS(status));
	}
	else if(WIFSIGNALED(status)) {
		fprintf(stderr,"abnormal termination, signal number = %d\n",WTERMSIG(status));
	}
	else if(WIFSTOPPED(status)) {
		fprintf(stderr,"child stopped, signal number = %d\n",WSTOPSIG(status));
	}

	return;
}

#if 0 // SYSDB 
<item key="net.proto.dhcpon"			type="BOOL" 	min="" max="" val="" />
<item key="net.proto.ddnson"			type="BOOL" 	min="" max="" val="" />
<item key="net.proto.webon"				type="BOOL" 	min="" max="" val="" />
<item key="net.proto.clienton"			type="BOOL" 	min="" max="" val="" />

<item key="net.proto.ipaddr"			type="UINT" 	min="" max="" val="" />
<item key="net.proto.gateway"			type="UINT" 	min="" max="" val="" />
<item key="net.proto.subnet"			type="UINT" 	min="" max="" val="" />
<item key="net.proto.dns1"				type="UINT" 	min="" max="" val="" />
<item key="net.proto.dns2"				type="UINT" 	min="" max="" val="" />

<item key="net.proto.clientport"		type="UINT" 	min="" max="" val="" />
<item key="net.proto.webport"			type="UINT" 	min="" max="" val="" />
<item key="net.proto.maxtxspeed"		type="UINT" 	min="" max="" val="" />
<item key="net.proto.ddnssvr"			type="STRING" 	min="" max="" val="" />

http://nf.intellix.co.kr/phpBB/viewtopic.php?t=353&highlight=
#endif


static int detect_mii(int skfd, char *ifname) {   
	struct ifreq ifr;   
	unsigned short *data, mii_val;   
	unsigned phy_id;   
  
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);   
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {   
		(void) close(skfd);   
		return 2;   
	}   
  
	data = (unsigned short *)(&ifr.ifr_data);   
	phy_id = data[0];   
	data[1] = 1;   
  
	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {   
		return 2;   
	}   
  
	mii_val = data[3];   
	if ((mii_val & 0x0016) == 0x0004)   
		return 0;   
	else  
		return 1;   
}   
  
static int detect_ethtool(int skfd, char *ifname) {   
	struct ifreq ifr;   
	
	struct ethtool_value edata;   
  
	memset(&ifr, 0, sizeof(ifr));   
	edata.cmd = ETHTOOL_GLINK;   
  
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);   
	ifr.ifr_data = (char *) &edata;   
  
	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {   
		return 2;   
	}   
  
	if (edata.data)   
		return 0;   
	else  
		return 1;   
} 

/**
	@brief				��Ʈ��ũ link ���� üũ 
	@param[out]	*out_status	��ũ ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_link_status( char *dev_name, int *out_status )
{
    int skfd = -1;       
    int retval;   

	g_return_val_if_fail ( dev_name != NULL, FALSE );
	g_return_val_if_fail ( out_status != NULL, FALSE );

#if defined(USE_DEV_SWITCH)
	/*int port = NF_UTIL_SWITCH_PORT_WAN;
	nf_dev_switch_get_link_status(&port);
	if (port)
	{
		*out_status = NF_NETIF_LINK_STATUS_UP;
	}
	else
	{
		*out_status = NF_NETIF_LINK_STATUS_DOWN;
	}*/

	int port = 0;
	port = nf_dev_get_link_status(dev_name);
	if(port == 0x3)
		*out_status = NF_NETIF_LINK_STATUS_UP;
	else
		*out_status = NF_NETIF_LINK_STATUS_DOWN;
#else
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    g_return_val_if_fail ( skfd >= 0, FALSE );

	retval = detect_ethtool(skfd, dev_name);   
	if (retval == 2)   
		retval = detect_mii(skfd, dev_name);   
		
    close(skfd);


    if (retval == 2){	
    	*out_status = NF_NETIF_LINK_STATUS_UNKNOWN;
    }else if (retval == 1){	
    	*out_status = NF_NETIF_LINK_STATUS_DOWN;
    }else if (retval == 0){	
    	*out_status = NF_NETIF_LINK_STATUS_UP;    	
    }
#endif

#if 0
    g_message("%s dev[%s] [%d]", __FUNCTION__, dev_name, *out_status);
#endif
    
   	return 1;                  
}

/**
	@brief				Get link status of every ports on a switch
	@param[in]			sw_num		the index of the switch to get link status
	@param[out]			*out_mask	switch port mask(ex. 0x1 means port#0 linked)
	@return gboolean	%TRUE on success, %FALSE if an error occurred
*/

#ifdef USE_MCAST
/**
 	@brief				MAC�� ����Ͽ� unique �� IP ����.
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
 */
gboolean nf_netif_create_unique_ip(int base, int mask, int add, unsigned int *gen_ip )
{
	NF_NETIF_MAC mac;
	unsigned int res;

	g_return_val_if_fail( gen_ip, FALSE );

	if( nf_netif_get_mac(&mac) == FALSE ) return FALSE;

	mac.mac_addr[2] += (char)(add & 0x000000FF);
	res = mac.mac_addr[5] | (mac.mac_addr[4] << 8) | (mac.mac_addr[3] << 16) | (mac.mac_addr[2] << 24);

	res = (res & ~mask) | (base & mask);

	*gen_ip = res;

	return TRUE;
}
#endif

#if 0

/*
<item key="net.filter.enable" type="BOOL" min="0" max="1" val="0" />
<item key="net.filter.opmode" type="BOOL" min="0" max="1" val="0" />                  
	0:allow list   -- �⺻������ �����ϰ� ��Ͽ� �ִ� ����Ʈ�� ���
	1:deny list    -- �⺻������ ����ϰ� ��Ͽ� �ִ� ����Ʈ�� ����
<item key="net.filter.rule.RCNT" type="UINT" min="0" max="8" val="0" />
<item key="net.filter.rule.R0.type" type="UINT " min="0" max="32" val="0" />
	0: ������� ����.
	8/16/24:  A/B/C clss
	32: Ư�� IP ADDRESS    ����) ����   8~24 ���̿� �߰� ���� �߰� �� �� ����.
<item key="net.filter.rule.R0.addr" type="UINT " min="0" max="" val="" />                // ip addr
~~~~
<item key="net.filter.rule.R7.type" type="UINT" min="0" max="32" val="0" />
<item key="net.filter.rule.R7.addr" type="UINT " min="0" max="" val="" />                // ip addr
*/

// for allow list	
iptables -F 
iptables -A INPUT -i eth0 -p tcp --dport 8080 -s 192.168.100.73 -j ACCEPT
iptables -A INPUT -i eth0 -p tcp --dport 8080 -s 192.168.100.0/24 -j ACCEPT
iptables -A INPUT -i eth0 -p tcp --dport 8080 -s 10.0.0.0/24 -j ACCEPT
iptables -A INPUT -i eth0 -p tcp --dport 8080 -j DROP
iptables -L -n

// for deny list	
iptables -F
iptables -A INPUT -i eth0 -p tcp --dport 8080 -s 192.168.100.71 -j DROP
iptables -A INPUT -i eth0 -p tcp --dport 8080 -s 192.168.100.0/24 -j DROP
iptables -L -n

sysdb-Message: 05880 [GParamUInt      ][UINT      ] name[net.proto.rtspport              ] [5554]
sysdb-Message: 05886 [GParamUInt      ][UINT      ] name[net.proto.webport               ] [8080]

#endif 

enum  {
	NF_NETIF_IPFILTER_MODE_ALLOW = 0,
	NF_NETIF_IPFILTER_MODE_DENY = 1,	
} NF_NETIF_IPFILTER_MODE_E;


gboolean nf_netif_ipfilter_init()
{
	
	gint web_port, rtsp_port;
	gint op_mode = NF_NETIF_IPFILTER_MODE_ALLOW;	
	gint rule_cnt = 0;
	gchar  tmp[256];	
	int i;
	
	g_message("%s called!! ",__FUNCTION__);
															
	proxy_system("iptables -F",1,3);
	proxy_system("iptables -L -n",1,3);
	
	if( nf_sysdb_get_bool("net.filter.enable") == 0 )
		return 1;

	op_mode = nf_sysdb_get_bool("net.filter.opmode");
	rule_cnt = nf_sysdb_get_uint("net.filter.rule.RCNT");

	web_port = nf_sysdb_get_uint("net.proto.webport");
	rtsp_port = nf_sysdb_get_uint("net.proto.rtspport");

	g_message("%s op[%d] rule_cnt[%d] port[%d][%d]", __FUNCTION__, 
									op_mode, rule_cnt, web_port, rtsp_port);
												
	for(i=0;i<rule_cnt;i++){
		guint addr, type;
		char str_addr[128], postfix[32];
				
		snprintf(tmp, sizeof(tmp), "net.filter.rule.R%d.type", i);
		type = nf_sysdb_get_uint(tmp);

		if(type == 0) continue;
						
		snprintf(tmp, sizeof(tmp), "net.filter.rule.R%d.addr", i);
		addr = htonl(nf_sysdb_get_uint(tmp));
			
		my_inet_ntoa(str_addr, sizeof(str_addr), addr );				

		if( type != 32) {
			snprintf( postfix, sizeof(postfix), "/%d", type);
			strcat( str_addr, postfix);
		}
						
		g_message("%s --> (%d) : type[%d] addr[%08x] [%s]",__FUNCTION__, i, type, addr, str_addr);

		if( op_mode == NF_NETIF_IPFILTER_MODE_ALLOW  ) {
			
			snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -s %s -j ACCEPT", _eth_dev, web_port, str_addr);
			proxy_system( tmp,1,3);
			snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -s %s -j ACCEPT", _eth_dev, rtsp_port, str_addr);
			proxy_system( tmp,1,3);
			
		}else{ // NF_NETIF_IPFILTER_MODE_DENY

			snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -s %s -j DROP", _eth_dev, web_port, str_addr);
			proxy_system( tmp,1,3);
			snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -s %s -j DROP", _eth_dev, rtsp_port, str_addr);
			proxy_system( tmp,1,3);						
		}						
	}
		
	if( op_mode == NF_NETIF_IPFILTER_MODE_ALLOW)
	{
		snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -j DROP", _eth_dev, web_port);
		proxy_system( tmp,1,3);
		snprintf( tmp, sizeof(tmp), "iptables -A INPUT -i %s -p tcp --dport %d -j DROP", _eth_dev, rtsp_port);
		proxy_system( tmp,1,3);
	}
												
	proxy_system("iptables -L -n",1,3);
		
	return 1;
	
}

/*
sysdb_set net.filter.enable b 1
sysdb_set net.filter.opmode b 0
sysdb_set net.filter.rule.RCNT u 2
sysdb_set net.filter.rule.R0.type u 32 
sysdb_set net.filter.rule.R0.addr u 3232261191
sysdb_set net.filter.rule.R1.type u 24
sysdb_set net.filter.rule.R1.addr u 3232261191
sysdb_dump net.filter
netif_ipfilter
*/

static char netif_ipfilter_help[] = "netif_ipfilter";
static int netif_ipfilter(int argc, char **argv)
{	

	nf_netif_ipfilter_init();

	return 0;
}
__commandlist(netif_ipfilter,"netif_ipfilter", netif_ipfilter_help, netif_ipfilter_help);


/**
	@brief				�ý��� ��Ʈ��ũ ���¸� ��� �´�. ( text���� )
	@param[out]	*buff	������� ���Ϲ��� ����ü ������
	@param[out]	*buff_len	������� ���Ϲ��� ������ ũ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_netstat( gchar *buff, gint *buff_len )
{	

	gchar *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;

	g_return_val_if_fail ( buff != NULL , FALSE );
	g_return_val_if_fail ( *buff_len >= 4096 , FALSE );
				
	//proxy_system( "ifconfig eth1 > /tmp/netif_netstat.txt",1,3);
	proxy_system( "netstat -nr > /tmp/netif_netstat.txt",1,3);
	proxy_system( "netstat -na -t | grep -v LISTEN >> /tmp/netif_netstat.txt",1,3);
	proxy_system( "cat /proc/net/arp >> /tmp/netif_netstat.txt",1,3);

	if (!g_file_get_contents ("/tmp/netif_netstat.txt", &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return 0;
	}		

	if(contents)
	{
		//g_print("%*.*s", length, length, contents);
		int tmp_len  = *buff_len - 1;		

		*buff_len = length > tmp_len ? tmp_len : length;
		strncpy( buff, contents, *buff_len);								

		g_free(contents);
	}
				    	
	return 1;
}

#ifdef DEBUG_NETIF_JBSHELL

static char netif_netstat_help[] = "netif_netstat";
static int netif_netstat(int argc, char **argv)
{	
	
	int ret = 0;
	gchar buff[4096];
	gint buff_len = sizeof(buff);
	
	ret = nf_netif_get_netstat( buff, &buff_len);

	if(ret == 1)
		ipx_printf_large("%*.*s", buff_len, buff_len, buff);

	return 0;
}
__commandlist(netif_netstat,"netif_netstat", netif_netstat_help, netif_netstat_help);

#endif

#include <ctype.h>
 
/**
	@brief				�ý��� ��Ʈ��ũ ���¸� ��� �´�. ( text���� )
	@param[out]	*buff	������� ���Ϲ��� ����ü ������
	@param[out]	*buff_len	������� ���Ϲ��� ������ ũ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_ping_test( gchar *host, gchar *buff, gint *buff_len )
{
	gchar  tmp[1024];	
	int i, host_len;

	gchar  *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;	
	
	g_return_val_if_fail ( host != NULL , FALSE );
	g_return_val_if_fail ( buff != NULL , FALSE );
	g_return_val_if_fail ( *buff_len >= 4096 , FALSE );

	host_len = strlen(host);
	for(i=0; i<host_len; i++)
	{	
		int tmp_c = host[i];		
		if( !(isalnum(tmp_c) || isdigit(tmp_c) || tmp_c == '.' || tmp_c == '-') )
		{
			g_warning("%s wrong hostname[%s](%d)", __FUNCTION__, host,i);
			return FALSE;
		}
	}
			
	snprintf( tmp, sizeof(tmp), "ping %s -W 1 -c 3 -s 10000 >& /tmp/netif_ping.txt", host);
	proxy_system( tmp, 1, 5);	
	if (!g_file_get_contents ("/tmp/netif_ping.txt", &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return 0;
	}		

	if(contents)
	{
		//g_print("%*.*s", length, length, contents);
		int tmp_len  = *buff_len - 1;		

		*buff_len = length > tmp_len ? tmp_len : length;
		strncpy( buff, contents, *buff_len);								

		g_free(contents);
	}		
	return 1;
	
}

gboolean nf_netif_ping_test_advanced( gchar *host, guint count, guint size, NF_NETIF_PING_TEST *test )
{
	gchar  tmp[1024];
	gchar  tmp_rm[1024];	
	gchar  tmp_file[256];
	int i, host_len;
	GTimeVal tval;

	gchar  *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;	

	gboolean ret = FALSE;
	
	g_return_val_if_fail ( host != NULL , FALSE );
	g_return_val_if_fail ( test != NULL , FALSE );	

	if( (count >= 60) || (count == 0) || (size == 0) || (size >= 65507) )
		return FALSE;	

	host_len = strlen(host);
	for(i=0; i<host_len; i++)
	{	
		int tmp_c = host[i];		
		if( !(isalnum(tmp_c) || isdigit(tmp_c) || tmp_c == '.' || tmp_c == '-') )
		{
			g_warning("%s wrong hostname[%s](%d)", __FUNCTION__, host,i);
			return FALSE;
		}
	}

	memset(test->out, 0x0, sizeof(test->out));

	gettimeofday(&tval, NULL);
	
	snprintf( tmp_file, sizeof(tmp_file), "/tmp/netif_ping_%u.txt", tval.tv_usec);
	snprintf( tmp, sizeof(tmp), "ping %s -W 1 -c %u -s %u >& %s", host, count, size, tmp_file);
	snprintf( tmp_rm, sizeof(tmp_rm), "rm -f %s", tmp_file);
	
	proxy_system( tmp, 1, 5);	

	if (!g_file_get_contents (tmp_file, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = FALSE;
	}		
	else
	{
		if(contents)
		{
			int tmp_len;
			int trans, received;
			char *tmp_str;

			tmp_len = sizeof(test->out);		

			if( length < tmp_len )
				strncpy( test->out, contents, length);
			else
				strncpy( test->out, contents, tmp_len-1);								
			
			tmp_str = strstr( contents, "packets transmitted");
			if(tmp_str)
			{
				
				tmp_str = tmp_str - 1;
				while(isspace(*tmp_str))
					tmp_str = tmp_str - 1;

				while(isdigit(*tmp_str))
					tmp_str = tmp_str - 1;

				tmp_str = tmp_str + 1;

				trans = strtol(tmp_str, NULL, 10);			

				tmp_str = strstr( contents, "packets received");
				if(tmp_str)
				{	
					int temp;
					
					tmp_str = tmp_str - 1;
					while(isspace(*tmp_str))
						tmp_str = tmp_str - 1;

					while(isdigit(*tmp_str))
						tmp_str = tmp_str - 1;

					tmp_str = tmp_str + 1;

					received = strtol(tmp_str, NULL, 10);

					temp = trans - received;

					test->trans_cnt = trans;
					test->loss_cnt = temp;

					if(test->loss_cnt == 0)
						ret = TRUE;
					else
						ret = FALSE;
				}
				else
				{
					ret = FALSE;
				}
			}
			else
			{
				ret = FALSE;
			}

			tmp_str = strstr( contents, "min/avg/max");
			if(tmp_str)
			{	
				tmp_str = strstr( tmp_str, "=");
				if(tmp_str)
				{
					guint num[3];
					int i;
					double n = 0;
					
					tmp_str = tmp_str + 1;
					while(isspace(*tmp_str))
						tmp_str = tmp_str + 1;

					for(i = 0; i < 3; i++)
					{	
						n = strtod(tmp_str, &tmp_str);
				
						num[i] = (guint)n;
						tmp_str = tmp_str + 1;
					}

					test->min = num[0];
					test->avg = num[1];
					test->max = num[2];
				}
			}

			g_free(contents);
		}
		else
		{
			ret = FALSE;
		}
	}

	proxy_system( tmp_rm, 1, 5);
/*
	g_message("PING - min - %u", test->min);
	g_message("PING - avg - %u", test->avg);
	g_message("PING - max - %u", test->max);
	g_message("PING - tran - %u", test->trans_cnt);
	g_message("PING - loss - %u", test->loss_cnt);
*/	
	return ret;
}

#ifdef DEBUG_NETIF_JBSHELL

static char netif_ping_help[] = "netif_ping [hostname]";
static int netif_ping(int argc, char **argv)
{	
	
	int ret = 0;
	gchar *hostname = NULL;	
	gchar buff[4096];
	gint buff_len = sizeof(buff);	
	
	if(argc < 2){
		printf("%s\n",netif_ping_help);
		return -1;
	}		
	
	hostname = argv[1];
		
	ret = nf_netif_ping_test( hostname, buff, &buff_len);

	if(ret == 1)
		ipx_printf_large("%*.*s", buff_len, buff_len, buff);

	return 0;
}
__commandlist(netif_ping,"netif_ping", netif_ping_help, netif_ping_help);

#endif

/**
	@brief					IPv6 ��ȿ�� üũ �� ���ó��
	@param[in]	p_in_addr6	üũ �� ����� IPv6 �ּ� (String)
	@param[out]	p_out_addr6	���� IPv6 �ּ� (String)
	@return	gboolean		%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_compress_ipv6_addr(const char *p_in_addr6, char *p_out_addr6)
{
	struct sockaddr_in6 sa;
	char addr6_binary[16];
	int is_ipv6_addr_valid;
	struct in6_addr st_addr6;
	char *ret_addr6;
	
	is_ipv6_addr_valid = inet_pton(AF_INET6, p_in_addr6, &addr6_binary);
	if (is_ipv6_addr_valid != 1)
		return 0;
	
	memcpy((void *)&st_addr6, (void *)&addr6_binary, sizeof(st_addr6));
	inet_ntop(AF_INET6, (void *)&st_addr6, p_out_addr6, INET6_ADDRSTRLEN);
	
	return 1;
}
