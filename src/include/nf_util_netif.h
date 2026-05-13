#ifndef __NF_UTIL_NETIF_H__
#define __NF_UTIL_NETIF_H__

#define NET_IPADDR		"net.proto.ipaddr"
#define NET_GATEWAY		"net.proto.gateway"
#define NET_SUBNET		"net.proto.subnet"
#define NET_DNS1		"net.proto.dns1"
#define NET_DNS2		"net.proto.dns2"

#define LOCAL_NET_IPADDR		"net.eth2.ipaddr"
#define LOCAL_NET_GATEWAY		"net.eth2.gateway"
#define LOCAL_NET_SUBNET		"net.eth2.subnet"

#define DHCP_IPADDR		"net.dhcp.ipaddr"
#define DHCP_GATEWAY	"net.dhcp.gateway"
#define DHCP_SUBNET		"net.dhcp.subnet"
#define DHCP_DNS1		"net.dhcp.dns1"
#define DHCP_DNS2		"net.dhcp.dns2"

#define IPV6_ADDR_GLOBAL        0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U

#define IPV6_SYSDB_KEY_USING		"net.ipv6.using"
#define IPV6_SYSDB_KEY_IPADDR_LL	"net.ipv6.linklocal"

#define IPV6_SYSDB_KEY_IPADDR0		"net.ipv6.addr0"
#define IPV6_SYSDB_KEY_IPADDR1		"net.ipv6.addr1"
#define IPV6_SYSDB_KEY_IPADDR2		"net.ipv6.addr2"
#define IPV6_SYSDB_KEY_IPADDR3		"net.ipv6.addr3"

#define IPV6_SYSDB_KEY_PREFIX0		"net.ipv6.prefix_length0"
#define IPV6_SYSDB_KEY_PREFIX1		"net.ipv6.prefix_length1"
#define IPV6_SYSDB_KEY_PREFIX2		"net.ipv6.prefix_length2"
#define IPV6_SYSDB_KEY_PREFIX3		"net.ipv6.prefix_length3"

#define IPV6_SYSDB_KEY_DNS1			"net.ipv6.dns1"
#define IPV6_SYSDB_KEY_DNS2			"net.ipv6.dns2"
#define IPV6_SYSDB_KEY_GATEWAY		"net.ipv6.gateway"

#define MAX_IPV6_ADDR_CNT 4
#define MAX_IPV6_DNS_CNT 2

#define HOST_ETH_DEV	"eth0"
#define HUB_ETH_DEV		"eth1"
#define BRIDGE_DEV		"br0"

#if defined(_SNF_MODEL) || defined(_IPX_0412VE3) || defined(_IPX_0824VE3) || defined(_IPX_1648VE3) || \
defined(_IPX_0824P3) || defined(_IPX_1648P3) || \
defined(_IPX_0412P4) || defined(_IPX_0824P4) || defined(_IPX_1648P4) \
|| defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
|| defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
#define USE_MV6095F
#endif

gboolean nf_netif_eth_init(void);

/**
	@brief				�ý��� ��Ʈ��ũ ���� (sysdb ���� ���)	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_netif_init();

typedef struct _NF_NETIF_MAC_T
{
	gchar	mac_addr[6];	
} NF_NETIF_MAC;

typedef struct _NF_NETIF_IP_T
{
	guchar	ip_addr[4];
} NF_NETIF_IP;

/**
	@brief				LAN1 - LAN2 IP Conflict Check
	@return	gboolean	%TRUE on conflict, %FALSE if not conflict
*/
gboolean nf_netif_check_lan_conflict();
/**
	@brief				�ý����� MAC �ּҸ� ��� �´�.
	@param[out]	*ret	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_mac( NF_NETIF_MAC *ret);
gchar *nf_netif_get_mac_str(gchar *mac_buf);

#define IPV6_STR_MAX	64
#define IPV6_ADDR_MAX	4
#define IPV6_PREFIX_MAX	4
#define IPV6_DNS_MAX	2

typedef struct _NF_NETIF_GET_INFO_T
{
	guint	ipaddr;
	guint	gateway;
	guint	netmask;
	guint	broadcast;
	guint	lan2_ipaddr;
	
	guint	dnsserver1;
	guint	dnsserver2;
	
	gchar 	mac_addr[6];

	gchar	ipv6_linklocal[IPV6_STR_MAX];
	gchar	ipv6_addr[IPV6_ADDR_MAX][IPV6_STR_MAX];
	guint	ipv6_prefix[IPV6_PREFIX_MAX];
	gchar	ipv6_gateway[IPV6_STR_MAX];
	gchar	ipv6_dns[IPV6_DNS_MAX][IPV6_STR_MAX];
	
	gchar	reserved[2];
} NF_NETIF_GET_INFO;


/**
	@brief				�ý��� ��Ʈ��ũ ������ ��� �´�.
	@param[out]	*ret	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_info( NF_NETIF_GET_INFO *ret);


typedef struct _NF_NETIF_GET_STAT_T
{
	GTimeVal	tval;
	gchar	dev_name[32];		

	guint64	rx_byte;
	guint64	rx_packet;
	guint64	rx_error;
	guint64	rx_drop;
	guint64	rx_fifo;
	guint64	rx_colls;
	guint64	rx_carrier;
	guint64	rx_compressed;

	guint64	tx_byte;
	guint64	tx_packet;
	guint64	tx_error;
	guint64	tx_drop;
	guint64	tx_fifo;
	guint64	tx_colls;
	guint64	tx_carrier;
	guint64	tx_compressed;
	
} NF_NETIF_GET_STAT;

/**
	@brief				��Ʈ��ũ �������̽� ��� ������ ��� �´�.
	@param[out]	*ret	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_stat( char *dev_name, NF_NETIF_GET_STAT *ret);



/**
	@brief				��Ʈ��ũ �������̽� ��� ���� delta 
	@param[out]	*ret	������� ���Ϲ��� ����ü ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_delta( char *dev_name, 
							NF_NETIF_GET_STAT *prev, 
							NF_NETIF_GET_STAT *next,
							NF_NETIF_GET_STAT *delta);
							

typedef enum _NF_NETIF_LINK_STATUS_E
{
	NF_NETIF_LINK_STATUS_DOWN = 0,
	NF_NETIF_LINK_STATUS_UP = 1,
	NF_NETIF_LINK_STATUS_UNKNOWN= 2	
} NF_NETIF_LINK_STATUS_E;


/**
	@brief				��Ʈ��ũ link ���� üũ 
	@param[out]	*out_status	��ũ ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_link_status( char *dev_name, int *out_status );

gboolean nf_netif_dhcp_restart(void);

							

/**
	@brief	dhcp renew monitoring
*/							
void nf_netif_renew_dhcp();

#ifdef USE_MCAST
gboolean nf_netif_create_unique_ip(int base, int mask, int addr, unsigned int *gen_ip);							
#endif

/**
	@brief	apply ipfiltering
*/							
gboolean nf_netif_ipfilter_init();


/**
	@brief				�ý��� ��Ʈ��ũ ���¸� ��� �´�. ( text���� )
	@param[out]	*buff	������� ���Ϲ��� ����ü ������
	@param[out]	*buff_len	������� ���Ϲ��� ������ ũ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_get_netstat( gchar *buff, gint *buff_len );

/**
	@brief				�ý��� ��Ʈ��ũ ���¸� ��� �´�. ( text���� )
	@param[out]	*buff	������� ���Ϲ��� ����ü ������
	@param[out]	*buff_len	������� ���Ϲ��� ������ ũ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_ping_test( gchar *host, gchar *buff, gint *buff_len );

typedef struct _NF_NETIF_PING_TEST{              
           gchar    out[4096];
           guint    min; // ms
           guint    avg;
           guint    max;           
           guint    trans_cnt;
           guint    loss_cnt;                     
}NF_NETIF_PING_TEST;

gboolean nf_netif_ping_test_advanced( gchar *host, guint count, guint size, NF_NETIF_PING_TEST *test );

char* nf_netif_get_eth_str(void);

#endif
//onvif_porting
static void my_inet_ntoa( gchar *buff, guint buff_len, guint addr);
//onvif_porting

/**
	@brief					IPv6 ��ȿ�� üũ �� ���ó��
	@param[in]	p_in_addr6	üũ �� ����� IPv6 �ּ� (String)
	@param[out]	p_out_addr6	���� IPv6 �ּ� (String)
	@return	gboolean		%TRUE on success, %FALSE if an error occurred	
*/
gboolean nf_netif_compress_ipv6_addr(const char *p_in_addr6, char *p_out_addr6);

