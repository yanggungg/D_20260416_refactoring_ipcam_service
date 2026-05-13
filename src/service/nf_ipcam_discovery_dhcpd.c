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

#include <nf_ipcam_defs.h>


#define DHCP_SVR_PORT 		(67)
#define DHCP_CLI_PORT 		(68)
#define SOCK_BUF_SZ			(0x600)
#define DATA_SIZE			(548)
#define MAPPING_SIZE		(32)
#define MAPPING_TABLE_PATH  "/NFDVR/log/mapping_table"

#define PRINT_IP(x)	((x)&0xff>>0),(((x)&0xff00)>>8),(((x)&0xff0000)>>16),(((x)&0xff000000)>>24)

// USE UDHCPD, 
static char* UDHCPD_CONFIG = "/etc/udhcpd.conf";
static char* UDHCPD_LEASE_FILE = "/etc/udhcpd.lease";
static char* DHCP_GW = "192.168.10.1";
static char* DHCP_START = "192.168.10.2";
static char* DHCP_END = "192.168.10.254";
static char* DHCP_NETMASK = "255.255.255.0";
static char* DHCP_DNS1 = "8.8.8.8";
static char* DHCP_DNS2 = "8.8.4.4";
static char* DHCP_LEASE = "86400"; //1 day = 86400
static char* DHCP_DOMAIN = "local";

typedef struct __OPENMODE_MAC_LIST_
{
	unsigned char mac[6];
	unsigned int offer_count;
}openmode_mac_list;

struct psuedohdr  {
	struct in_addr source_address;
	struct in_addr dest_address;
	unsigned char place_holder;
	unsigned char protocol;
	unsigned short length;
} psuedohdr;

struct _DHCP_TRANSACTION
{
	unsigned int tran_id;
	int port_num;				// 0~15

	unsigned int my_ip;			// host ip
	unsigned int cl_ip;			// client ip
	unsigned int req_ip;		// requested ip
	unsigned int subnet;		// subnet mask
	unsigned char my_mac[6];	// host mac
	unsigned char cl_mac[6];	// client mac
};
typedef struct _DHCP_TRANSACTION DHCP_TRANSACTION;

struct _MAC_IP_MAPPING_TABLE
{
	unsigned char mac[6]; // MAC
	unsigned int ip; // IP, C class, Last 8 bit
};
typedef struct _MAC_IP_MAPPING_TABLE MAC_IP_MAPPING_TABLE;
static MAC_IP_MAPPING_TABLE mac_ip_mapping_table[MAPPING_SIZE]; // 임시로 32
static mapping_table_index = 0;
void read_mapping_table();
void write_mapping_table();

static GAsyncQueue *openmode_dhcp_queue = NULL;

static int send_sock = 0;
static DHCP_TRANSACTION tran;


static int check_if_valid_req(DHCP_TRANSACTION*);
static int init_send_sock(void);

static void send_offer(int eth);
static void send_nack(int eth);
static void send_ack(int eth);
static void nf_ipcam_dhcpd(void);
static size_t _openmode_dhcp_ip_conflict(const char*, const char*);

unsigned int get_bridge_dhcpd_available_ip(void);
int compare_mapping_mac(MAC_IP_MAPPING_TABLE m_t);
int check_conflict_my_dhcp_server_ip(unsigned check_ip);
void print_mac_table();

static pthread_t dhcpd_th;
extern void nf_ipcam_dhcpd_init(void)
{
	pthread_create(&dhcpd_th, NULL, (void*)&nf_ipcam_dhcpd, NULL);
}

static void nf_ipcam_dhcpd(void)
{
	int i = 0;
	int len = 0;
	int rcv_sock = 0;
	int port_hub = 0;
	struct sockaddr_in sin;

	unsigned char msg_buff[SOCK_BUF_SZ];

	unsigned char *p;
	unsigned short *p_short;
	unsigned int *p_int;
	unsigned int tran_id;		// transaction id
	unsigned char op_tag = 0;	// option tag
	unsigned char op_len = 0;	// option length
	unsigned char msg_type_rcv;
	unsigned char msg_type_send;

	//init_send_sock();
	memset(&tran, 0x00, sizeof(tran));
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(DHCP_SVR_PORT);

	IPCAM_DBG(MAJOR, "start\n");

	if ((rcv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		IPCAM_DBG(MINOR, "socket error\n");
		perror("socket");
		return ;
	}

	if (bind(rcv_sock, (struct sockaddr*) &sin, sizeof(sin)) == -1)
	{
		IPCAM_DBG(MINOR, "bind error\n");
		perror("bind");
		return ;
	}

	while (1)
	{
		memset(msg_buff, 0x00, SOCK_BUF_SZ);
		msg_type_rcv = 0;
		msg_type_send = 0;
		usleep(50*1000);
		len = recvfrom(rcv_sock, msg_buff, SOCK_BUF_SZ, 0, NULL, NULL);

		if (len == 0)
		{
			printf("%s [%s] ERROR: %s:%d socket broken\n", CAM_LOG_DOMAIN, __FUNCTION__, __FILE__, __LINE__);
			sleep(1);
			continue;
		}

		if (len < 0)
		{
			usleep(10*1000);
			continue;
		}

		if (nf_ipcam_is_vendor_zig())
		{
			sleep(1);
			continue;
		}

#if 0
		{
			int i=0;
			printf("     0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
			while (i < len)
			{
				if (i % 16 == 0)
				{
					printf("\n%02x  ", i/16);
				}
				else if (i % 8 == 0)
				{
					printf(" ");
				}
				printf("%02x ", msg_buff[i++]);
			}
			printf("\n\n\n");
		}
#endif

		p = &msg_buff[0];

		/***************************** bootp start *****************************/
		/* MSG type check: Boot request */
		if (*p++ != 0x01)
		{
			IPCAM_DBG(MINOR, "MSG_TYPE is not a 'Bootstrap_Request'\n");
			continue;
		}

		/* HW type check: Ethernet */
		if (*p++ != 0x01)
		{
			IPCAM_DBG(MINOR, "HW_TYPE is not an 'Ethernet'\n");
			continue;
		}

		/* HW address length: 6 bytes */
		if (*p++ != 0x06)
		{
			IPCAM_DBG(MINOR, "HW_ADDR_LEN fault(%02x)\n", *--p);
			continue;
		}

		/* Hops check: This must be 0 */
		if (*p++ != 0x00)
		{
			IPCAM_DBG(MINOR, "None-zero HOPS\n");
			continue;
		}

		/* transaction id */
		p_int = (void*)p;
		tran_id = ntohl(*p_int++);
		p = (void*)p_int;

		/* seconds elapsed */
		p += 2;
		/* bootp flags: unicast */
		p += 2;
		/* ip addresses */
		p += 16;

		/* client mac address */
		memcpy(&tran.cl_mac[0], p, 6);
		p += 16;

		/* server host name */
		p += 0x40;
		/* boot file name */
		p += 0x80;
		/* magic cookie */
		p += 4;

		tran.tran_id = tran_id;
		switch_mtx_lock();
		tran.port_num = get_interrupt_port(&tran.cl_mac[0]);
		switch_mtx_unlock();
		port_hub = hub_find_port((char*)&tran.cl_mac[0]);
		IPCAM_DBG(MINOR, "local/hub port(%d/%d)\n", tran.port_num, port_hub);
		IPCAM_DBG(MINOR, "port_mac (%02x:%02x:%02x:%02x:%02x:%02x)\n",
				tran.cl_mac[0], tran.cl_mac[1],	tran.cl_mac[2],	
				tran.cl_mac[3],	tran.cl_mac[4],	tran.cl_mac[5]);

		tran.my_ip = get_host_info();
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
		if(port_hub >= 0)
			tran.my_ip = get_hub_info();
}
#endif
		if(nf_get_running_mode())
			tran.my_ip = get_bridge_info();

		if (tran.port_num < 0 && port_hub < 0)
		{
			IPCAM_DBG(MINOR, "WAN port request\n");
			memset(&tran, 0x00, sizeof(tran));
			continue;
		}
		else if (tran.port_num < 0)
		{
			dtable *discovery = get_dtable();
            if(discovery[port_hub].layer == IPCAM_DISC_LAYER_VHUB)
                tran.port_num = port_hub;
            else
                continue;
		}
		tran.cl_ip = get_available_ip(tran.port_num);
		tran.subnet = get_host_netmask();

		// IPCAM DISCOVERY STATE CHECK
		{
			dtable *discovery = get_dtable();
			if(discovery[tran.port_num].state == IPCAM_DISC_STATE_CAPA ||
				discovery[tran.port_num].state == IPCAM_DISC_STATE_DONE)
			{
				IPCAM_DBG(WARN, "ch(%d) Discovery Status : [%s]\n",
						tran.port_num, discovery[tran.port_num].state == IPCAM_DISC_STATE_CAPA  
						? "IPCAM_DISC_STATE_CAPA" : "IPCAM_DISC_STATE_DONE");
				continue;
			}
		}

		/* bootp option fields */
		op_tag = *p++;
		while (op_tag != 0xff)
		{
			op_len = *p++;
			switch(op_tag)
			{
				case 50:	// DHCP requested IP address
					tran.req_ip = *p | (*(p+1) << 8) | (*(p+2) << 16) | (*(p+3) << 24);
					p += op_len;
					IPCAM_DBG(MINOR, "Option: request ip(%d.%d.%d.%d)\n",
								PRINT_IP(tran.req_ip));
					break;
				case 53:	// DHCP message type
					IPCAM_DBG(MINOR, "Option: dhcp msg\n");
					msg_type_rcv = *p++;
					break;
				case 55:	// Parameter request list
				case 57:	// Maximum DHCP message size
				case 60:	// Vendor class identifier
				case 61:	// Client identifier
				default:	// Skip this option
					p += op_len;
					break;
			}
			op_tag = *p++;
		}

		switch(msg_type_rcv)
		{
			case 1:		// DHCP discover
				IPCAM_DBG(MINOR, "DHCP DISCOVER | send_offer(%d.%d.%d.%d)\n", PRINT_IP(tran.cl_ip));
				send_offer(port_hub);
				break;
			case 3:		// DHCP request
				IPCAM_DBG(MINOR, "DHCP REQUEST\n");
				if (check_if_valid_req(&tran))
				{
					tran.cl_ip = tran.req_ip;
					IPCAM_DBG(MINOR, "send_ack(%d.%d.%d.%d)\n", PRINT_IP(tran.cl_ip));
					set_switch_polling_delay(tran.port_num, 5);

					send_ack(port_hub);

					{
						dtable *discovery = get_dtable();

						if (discovery == NULL) break;
						if (discovery[tran.port_num].layer <= 0) break;

						discovery[tran.port_num].ipaddr = tran.cl_ip;
						memcpy(discovery[tran.port_num].macaddr, tran.cl_mac, 6);
						discovery[tran.port_num].vnet_id = 1;
						discovery[tran.port_num].state = IPCAM_DISC_STATE_IPDONE;
						discovery[tran.port_num].state_cnt = 0;
						nf_eventlog_put_ipcam_msg("DHCP assignment complete",  tran.port_num);
					}
				}
				else
				{
					IPCAM_DBG(MINOR, "send_nack(%d.%d.%d.%d)\n", PRINT_IP(tran.cl_ip));
					send_nack(port_hub);
				}
				break;
			default:
				break;
		}

		memset(&tran, 0x00, sizeof(tran));
		/****************************** bootp end ******************************/
	}
}

static int init_send_sock(void)
{
	IPCAM_DBG(MAJOR, "start\n");

	send_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(send_sock == -1)
	{
		IPCAM_DBG(ERROR, "send_sock fail\n");
		perror("socket");
		return (-1);
	}

	IPCAM_DBG(MAJOR, "end send_sock(%d)\n", send_sock);
	return send_sock;
}

static unsigned short in_cksum(unsigned short *addr,int len)
{
	register int sum = 0;
	u_short answer = 0;
	register u_short *w = addr;
	register int nleft = len;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;                          /* truncate to 16 bits */
	return(answer);
}

static unsigned short trans_check(unsigned char proto,
		char *packet,
		int length,
		struct in_addr source_address,
		struct in_addr dest_address)
{
	char *psuedo_packet;
	unsigned short answer;

	psuedohdr.protocol = proto;
	psuedohdr.length = htons(length);
	psuedohdr.place_holder = 0;

	psuedohdr.source_address = source_address;
	psuedohdr.dest_address = dest_address;

	if((psuedo_packet = malloc(sizeof(psuedohdr) + length)) == NULL)  {
		perror("malloc");
		exit(1);
	}

	memcpy(psuedo_packet,&psuedohdr,sizeof(psuedohdr));
	memcpy((psuedo_packet + sizeof(psuedohdr)),
			packet,length);

	answer = (unsigned short)in_cksum((unsigned short *)psuedo_packet,
			(length + sizeof(psuedohdr)));
	free(psuedo_packet);
	return answer;
}

static void send_offer(int eth)
{
	char* p;
	char* udph_p;
	unsigned short* p_short;
	unsigned int* p_int;
	unsigned char out_buf[SOCK_BUF_SZ];
	int len = (-1);

	struct in_addr s, d;
	struct ethhdr *eh;
	struct iphdr ip;
	struct udphdr udp;

	struct sockaddr_ll sll;

	p = &out_buf[0];
	memset(out_buf, 0x00, SOCK_BUF_SZ);
	eh = (void*) p;

	s.s_addr = tran.cl_ip;
	d.s_addr = tran.my_ip;

	/* Ethernet header */
	memcpy((void*)p, (void*)&tran.cl_mac[0], ETH_ALEN);
	p += ETH_ALEN;
	memcpy((void*)p, (void*)&tran.my_mac[0], ETH_ALEN);
	p += ETH_ALEN;
	eh->h_proto = htons(ETH_P_IP);
	p += sizeof(short);

	/* IP header */
	memset((void*)&ip, 0x00, sizeof(struct iphdr));
	ip.version	= IPVERSION;
	ip.ihl		= 5;
	ip.protocol	= IPPROTO_UDP;
	//ip.tot_len	= htons(sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE);
	ip.tot_len	= htons(576);
	ip.id		= 0;
	ip.frag_off	= htons(0x4000);
	ip.ttl		= IPDEFTTL;
	ip.saddr	= tran.my_ip;
	ip.daddr	= tran.cl_ip;
	ip.check	= (unsigned short)in_cksum((unsigned short *)&ip, sizeof(struct iphdr));
	memcpy((void*)p, (void*)&ip, sizeof(struct iphdr));
	p += sizeof(struct iphdr);

	/* UDP header */
	udph_p = p;
	memset((void*)&udp, 0x00, sizeof(struct udphdr));
	udp.source = htons(DHCP_SVR_PORT);
	udp.dest = htons(DHCP_CLI_PORT);
	//udp.len = htons(sizeof(struct udphdr) + DATA_SIZE);
	udp.len = htons(556);
	memcpy((void*)p, (void*)&udp, sizeof(struct udphdr));
	p += sizeof(struct udphdr);

	/* Bootstrap protocol */
	*p++ = 0x02;						// Boot reply
	*p++ = 0x01;						// HW type: Ethernet
	*p++ = 0x06;						// HW addr length
	*p++ = 0x00;						// Hops
	p_int = (void*)p;
	*p_int++ = htonl(tran.tran_id);	// transaction id
	p_short = (void*)p_int;
	*p_short++ = 0x0000;				// seconds elapsed
	*p_short++ = 0x0000;				// Bootp flags: unicast
	p_int = (void*)p_short;
	*p_int++ = 0x00000000;				// Client IP address
	*p_int++ = (tran.cl_ip);			// Your IP address
	*p_int++ = 0x00000000;				// Next server IP address
	*p_int++ = 0x00000000;				// Relay agent IP address
	p = (void*)p_int;
	memcpy(p, &tran.cl_mac[0], 6);		// Client MAC address
	p += 6;
	memset(p, 0x00, 0x40);				// Server host name not given
	p += 0x4a;
	memset(p, 0x00, 0x80);				// Boot file name not given
	p += 0x80;
	*p++ = 0x63;
	*p++ = 0x82;
	*p++ = 0x53;
	*p++ = 0x63;						// Magic cookie (OK)

	/* Bootstrap protocol options fields */
	*p++ = 53;						// DHCP Msg type
	*p++ = 1;						//   length
	*p++ = 2;						//   type: Offer

	*p++ = 54;						// Server identifier
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = (tran.my_ip);		//   host ip address
	p = (void*)p_int;

	*p++ = 51;						// Lease time
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = 0xffffffff;			//   infinity
	p = (void*)p_int;

	*p++ = 1;						// Subnet mask
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = tran.subnet;			//   host subnet mask
	p = (void*)p_int;

	*p++ = 3;						// Router ip
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int = get_netif_ip(LOCAL_ETH_DEVICE)&get_netif_mask(LOCAL_ETH_DEVICE) | 0x01000000;	//   host router address
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	if(eth >= 0)
		*p_int = get_netif_ip(HUB_ETH_DEVICE)&get_netif_mask(HUB_ETH_DEVICE) | 0x01000000;	//   host router address
}
#endif

	if(nf_get_running_mode())
		*p_int = get_netif_ip(HUB_ETH_DEVICE)&get_netif_mask(HUB_ETH_DEVICE) | 0x01000000;	//   host router address

	p_int++;
	p = (void*)p_int;

#if 0
	*p++ = 6;						// Domain name server ip
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = (tran.my_ip);		//   host ip address
	p = (void*)p_int;
#endif

	*p++ = 0xff;					// End option

	/* UDP checksum */
	udp.check = trans_check(IPPROTO_UDP, udph_p, sizeof(struct udphdr) + DATA_SIZE, s, d);
	memcpy((void*)udph_p, (void*)&udp, sizeof(struct udphdr));

	memset(&sll, 0x00, sizeof(sll));
	sll.sll_family   = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_ifindex  = if_nametoindex(LOCAL_ETH_DEVICE); // index of hub_eth
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	if(eth >= 0)
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth
}
#endif

	if(nf_get_running_mode())
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth

	sll.sll_hatype   = ARPHRD_ETHER;
	sll.sll_pkttype  = PACKET_BROADCAST;
	sll.sll_halen    = ETH_ALEN;
	/*MAC - begin*/
	sll.sll_addr[0]  = tran.cl_mac[0];
	sll.sll_addr[1]  = tran.cl_mac[1];
	sll.sll_addr[2]  = tran.cl_mac[2];
	sll.sll_addr[3]  = tran.cl_mac[3];
	sll.sll_addr[4]  = tran.cl_mac[4];
	sll.sll_addr[5]  = tran.cl_mac[5];
	/*MAC - end*/
	sll.sll_addr[6]  = 0x00;/*not used*/
	sll.sll_addr[7]  = 0x00;/*not used*/

	len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE;
	if (init_send_sock() > 0)
	{
		len = sendto(send_sock, out_buf, len, 0, (struct sockaddr*)&sll, sizeof(sll));
		IPCAM_DBG(MAJOR, "DHCP OFFER(%d)\n", len);

		if (len <= 0)
		{
			IPCAM_DBG(WARN, "DHCP OFFER sendto failed\n");
			perror("sendto");
		}
		//printf("★★★ Offer.. \n ");
		//print_mac_table();

		close(send_sock);
	}
	else
	{
		IPCAM_DBG(WARN, "DHCP OFFER failed\n");
	}
}

static void send_nack(int eth)
{
	char* p;
	char* udph_p;
	unsigned short* p_short;
	unsigned int* p_int;
	unsigned char out_buf[SOCK_BUF_SZ];
	int len = (-1);

	struct in_addr s, d;
	struct ethhdr *eh;
	struct iphdr ip;
	struct udphdr udp;

	struct sockaddr_ll sll;

	p = &out_buf[0];
	memset(out_buf, 0x00, SOCK_BUF_SZ);
	eh = (void*) p;

	s.s_addr = 0xffffffff;
	d.s_addr = tran.my_ip;

	/* Ethernet header */
	*p++ = 0xff;*p++ = 0xff;*p++ = 0xff;*p++ = 0xff;*p++ = 0xff;*p++ = 0xff;
	memcpy((void*)p, (void*)&tran.my_mac[0], ETH_ALEN);
	p += ETH_ALEN;
	eh->h_proto = htons(ETH_P_IP);
	p += sizeof(short);

	/* IP header */
	memset((void*)&ip, 0x00, sizeof(struct iphdr));
	ip.version	= IPVERSION;
	ip.ihl		= 5;
	ip.protocol	= IPPROTO_UDP;
	//ip.tot_len	= htons(sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE);
	ip.tot_len	= htons(576);
	ip.id		= 0;
	ip.frag_off	= htons(0x4000);
	ip.ttl		= IPDEFTTL;
	ip.saddr	= tran.my_ip;
	ip.daddr	= 0xffffffff;
	ip.check	= (unsigned short)in_cksum((unsigned short *)&ip, sizeof(struct iphdr));
	memcpy((void*)p, (void*)&ip, sizeof(struct iphdr));
	p += sizeof(struct iphdr);

	/* UDP header */
	udph_p = p;
	memset((void*)&udp, 0x00, sizeof(struct udphdr));
	udp.source = htons(DHCP_SVR_PORT);
	udp.dest = htons(DHCP_CLI_PORT);
	//udp.len = htons(sizeof(struct udphdr) + DATA_SIZE);
	udp.len = htons(556);
	memcpy((void*)p, (void*)&udp, sizeof(struct udphdr));
	p += sizeof(struct udphdr);

	/* Bootstrap protocol */
	*p++ = 0x02;						// Boot reply
	*p++ = 0x01;						// HW type: Ethernet
	*p++ = 0x06;						// HW addr length
	*p++ = 0x00;						// Hops
	p_int = (void*)p;
	*p_int++ = htonl(tran.tran_id);	// transaction id
	p_short = (void*)p_int;
	*p_short++ = 0x0000;				// seconds elapsed
	*p_short++ = 0x0000;				// Bootp flags: unicast
	p_int = (void*)p_short;
	*p_int++ = 0x00000000;				// Client IP address
	*p_int++ = 0x00000000;				// Your IP address
	*p_int++ = 0x00000000;				// Next server IP address
	*p_int++ = 0x00000000;				// Relay agent IP address
	p = (void*)p_int;
	memcpy(p, &tran.cl_mac[0], 6);		// Client MAC address
	p += 16;
	memset(p, 0x00, 0x40);				// Server host name not given
	p += 0x40;
	memset(p, 0x00, 0x80);				// Boot file name not given
	p += 0x80;
	*p++ = 0x63;
	*p++ = 0x82;
	*p++ = 0x53;
	*p++ = 0x63;						// Magic cookie (OK)

	/* Bootstrap protocol options fields */
	*p++ = 53;						// DHCP Msg type
	*p++ = 1;						//   length
	*p++ = 6;						//   type: Nack

	*p++ = 54;						// Server identifier
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = (tran.my_ip);		//   host ip address
	p = (void*)p_int;

	*p++ = 0xff;					// End option

	/* UDP checksum */
	udp.check = trans_check(IPPROTO_UDP, udph_p, sizeof(struct udphdr) + DATA_SIZE, s, d);
	memcpy((void*)udph_p, (void*)&udp, sizeof(struct udphdr));

	memset(&sll, 0x00, sizeof(sll));
	sll.sll_family   = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_ifindex  = if_nametoindex(LOCAL_ETH_DEVICE); // index of hub_eth
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	if(eth >= 0)
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth
}
#endif

	if(nf_get_running_mode())
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth

	sll.sll_hatype   = ARPHRD_ETHER;
	sll.sll_pkttype  = PACKET_BROADCAST;
	sll.sll_halen    = ETH_ALEN;
	/*MAC - begin*/
	sll.sll_addr[0]  = tran.cl_mac[0];
	sll.sll_addr[1]  = tran.cl_mac[1];
	sll.sll_addr[2]  = tran.cl_mac[2];
	sll.sll_addr[3]  = tran.cl_mac[3];
	sll.sll_addr[4]  = tran.cl_mac[4];
	sll.sll_addr[5]  = tran.cl_mac[5];
	/*MAC - end*/
	sll.sll_addr[6]  = 0x00;/*not used*/
	sll.sll_addr[7]  = 0x00;/*not used*/

	len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE;
	if (init_send_sock() > 0)
	{
		len = sendto(send_sock, out_buf, len, 0, (struct sockaddr*)&sll, sizeof(sll));
		IPCAM_DBG(MAJOR, "DHCP NACK(%d)\n", len);

		if (len <= 0)
		{
			IPCAM_DBG(WARN, "DHCP NACK sendto failed\n");
			perror("sendto");
		}

		close(send_sock);
	}
	else
	{
		IPCAM_DBG(WARN, "DHCP NACK failed\n");
	}
}

static void send_ack(int eth)
{
	char* p;
	char* udph_p;
	unsigned short* p_short;
	unsigned int* p_int;
	unsigned char out_buf[SOCK_BUF_SZ];
	int len = (-1);
	int i = 0;

	struct in_addr s, d;
	struct ethhdr *eh;
	struct iphdr ip;
	struct udphdr udp;

	struct sockaddr_ll sll;

	p = &out_buf[0];
	memset(out_buf, 0x00, SOCK_BUF_SZ);
	eh = (void*) p;

	s.s_addr = tran.cl_ip;
	d.s_addr = tran.my_ip;

	/* Ethernet header */
	memcpy((void*)p, (void*)&tran.cl_mac[0], ETH_ALEN);
	p += ETH_ALEN;
	memcpy((void*)p, (void*)&tran.my_mac[0], ETH_ALEN);
	p += ETH_ALEN;
	eh->h_proto = htons(ETH_P_IP);
	p += sizeof(short);

	/* IP header */
	memset((void*)&ip, 0x00, sizeof(struct iphdr));
	ip.version	= IPVERSION;
	ip.ihl		= 5;
	ip.protocol	= IPPROTO_UDP;
	//ip.tot_len	= htons(sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE);
	ip.tot_len	= htons(576);
	ip.id		= 0;
	ip.frag_off	= htons(0x4000);
	ip.ttl		= IPDEFTTL;
	ip.saddr	= tran.my_ip;
	ip.daddr	= tran.cl_ip;
	ip.check	= (unsigned short)in_cksum((unsigned short *)&ip, sizeof(struct iphdr));
	memcpy((void*)p, (void*)&ip, sizeof(struct iphdr));
	p += sizeof(struct iphdr);

	/* UDP header */
	udph_p = p;
	memset((void*)&udp, 0x00, sizeof(struct udphdr));
	udp.source = htons(DHCP_SVR_PORT);
	udp.dest = htons(DHCP_CLI_PORT);
	//udp.len = htons(sizeof(struct udphdr) + DATA_SIZE);
	udp.len = htons(556);
	memcpy((void*)p, (void*)&udp, sizeof(struct udphdr));
	p += sizeof(struct udphdr);

	/* Bootstrap protocol */
	*p++ = 0x02;						// Boot reply
	*p++ = 0x01;						// HW type: Ethernet
	*p++ = 0x06;						// HW addr length
	*p++ = 0x00;						// Hops
	p_int = (void*)p;
	*p_int++ = htonl(tran.tran_id);	// transaction id
	p_short = (void*)p_int;
	*p_short++ = 0x0000;				// seconds elapsed
	*p_short++ = 0x0000;				// Bootp flags: unicast
	p_int = (void*)p_short;
	*p_int++ = 0x00000000;				// Client IP address
	*p_int++ = (tran.cl_ip);			// Your IP address
	*p_int++ = 0x00000000;				// Next server IP address
	*p_int++ = 0x00000000;				// Relay agent IP address
	p = (void*)p_int;
	memcpy(p, &tran.cl_mac[0], 6);		// Client MAC address
	p += 6;
	memset(p, 0x00, 0x40);				// Server host name not given
	p += 0x4a;
	memset(p, 0x00, 0x80);				// Boot file name not given
	p += 0x80;
	*p++ = 0x63;
	*p++ = 0x82;
	*p++ = 0x53;
	*p++ = 0x63;						// Magic cookie (OK)

	/* Bootstrap protocol options fields */
	*p++ = 53;						// DHCP Msg type
	*p++ = 1;						//   length
	*p++ = 5;						//   type: Ack

	*p++ = 54;						// Server identifier
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = (tran.my_ip);		//   host ip address
	p = (void*)p_int;

	*p++ = 51;						// Lease time
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = 0xffffffff;			//   infinity
	p = (void*)p_int;

	*p++ = 1;						// Subnet mask
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = tran.subnet;			//   host subnet mask
	p = (void*)p_int;

	*p++ = 3;						// Router ip
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int = get_netif_ip(LOCAL_ETH_DEVICE)&get_netif_mask(LOCAL_ETH_DEVICE) | 0x01000000;	//   host router address
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	if(eth >= 0)
		*p_int = get_netif_ip(HUB_ETH_DEVICE)&get_netif_mask(HUB_ETH_DEVICE) | 0x01000000;	//   host router address
}
#endif

	if(nf_get_running_mode())
		*p_int = get_netif_ip(HUB_ETH_DEVICE)&get_netif_mask(HUB_ETH_DEVICE) | 0x01000000;	//   host router address
	p_int++;
	p = (void*)p_int;

#if 0
	*p++ = 6;						// Domain name server ip
	*p++ = 4;						//   length
	p_int = (void*)p;
	*p_int++ = (tran.my_ip);		//   host ip address
	p = (void*)p_int;
#endif

	*p++ = 0xff;					// End option

	/* UDP checksum */
	udp.check = trans_check(IPPROTO_UDP, udph_p, sizeof(struct udphdr) + DATA_SIZE, s, d);
	memcpy((void*)udph_p, (void*)&udp, sizeof(struct udphdr));

	memset(&sll, 0x00, sizeof(sll));
	sll.sll_family   = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_ifindex  = if_nametoindex(LOCAL_ETH_DEVICE); // index of hub_eth
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	if(eth >= 0)
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth
}
#endif

	if(nf_get_running_mode())
		sll.sll_ifindex  = if_nametoindex(HUB_ETH_DEVICE); // index of hub_eth

	sll.sll_hatype   = ARPHRD_ETHER;
	sll.sll_pkttype  = PACKET_BROADCAST;
	sll.sll_halen    = ETH_ALEN;
	/*MAC - begin*/
	sll.sll_addr[0]  = tran.cl_mac[0];
	sll.sll_addr[1]  = tran.cl_mac[1];
	sll.sll_addr[2]  = tran.cl_mac[2];
	sll.sll_addr[3]  = tran.cl_mac[3];
	sll.sll_addr[4]  = tran.cl_mac[4];
	sll.sll_addr[5]  = tran.cl_mac[5];
	/*MAC - end*/
	sll.sll_addr[6]  = 0x00;/*not used*/
	sll.sll_addr[7]  = 0x00;/*not used*/

	len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + DATA_SIZE;
	if (init_send_sock() > 0)
	{
		len = sendto(send_sock, out_buf, len, 0, (struct sockaddr*)&sll, sizeof(sll));
		IPCAM_DBG(MAJOR, "DHCP ACK(%d)\n", len);

		if (len <= 0)
		{
			IPCAM_DBG(WARN, "DHCP ACK sendto failed\n");
			perror("sendto");
		}
		// #4. MAC_IP_Table Setting, Write MAC-IP
		for(i=0; i<MAPPING_SIZE; i++)
		{
			if(compare_mapping_mac(mac_ip_mapping_table[i]))
			{
				mac_ip_mapping_table[i].ip = tran.req_ip&0xffffffff;
				break;
			}
			else
			{
				if(i==MAPPING_SIZE-1) // Not Exist
				{
					//memcpy(mac_ip_mapping_table[mapping_table_index].mac, &tran.cl_mac[0], 6);
					mac_ip_mapping_table[mapping_table_index].mac[0] = tran.cl_mac[0];
					mac_ip_mapping_table[mapping_table_index].mac[1] = tran.cl_mac[1];
					mac_ip_mapping_table[mapping_table_index].mac[2] = tran.cl_mac[2];
					mac_ip_mapping_table[mapping_table_index].mac[3] = tran.cl_mac[3];
					mac_ip_mapping_table[mapping_table_index].mac[4] = tran.cl_mac[4];
					mac_ip_mapping_table[mapping_table_index].mac[5] = tran.cl_mac[5];
					mac_ip_mapping_table[mapping_table_index].ip = tran.cl_ip&0xffffffff;
					printf("★★★ Write MAC : %u:%u:%u:%u:%u:%u \n", 
							tran.cl_mac[0], tran.cl_mac[1],tran.cl_mac[2],tran.cl_mac[3],tran.cl_mac[4],tran.cl_mac[5]);
					mapping_table_index = (mapping_table_index+1)%MAPPING_SIZE; // next
				}
				else
					continue;
			}
		}
		//print_mac_table();
		write_mapping_table();
		

		close(send_sock);
	}
	else
	{
		IPCAM_DBG(WARN, "DHCP ACK failed\n");
	}
}

static int check_if_valid_req(DHCP_TRANSACTION* tran)
{
	int i = 0;
	unsigned int req = 0;
	unsigned int h,s;
	dtable *discovery = NULL;

	IPCAM_DBG(MAJOR, "start\n");

	if(tran == NULL)
	{
		IPCAM_DBG(MINOR, "invalid(NULL transaction)\n");
		return 0;
	}
	req = tran->req_ip;

	discovery = get_dtable();
	if (discovery == NULL) { return 0; }

	h = get_host_info();
	s = get_host_netmask();
#ifdef DUAL_LAN_NETWORK
if(!nf_get_custom_mode())
{
	//if(tran->port_num >= 8)
    if(discovery[tran->port_num].layer == IPCAM_DISC_LAYER_VHUB)
		h = get_hub_info();
}
#endif
	if(nf_get_running_mode()) {
		printf("\e[33m [%s][%d] custom mode valide check! \e[0m\n", __func__, __LINE__);
		h = get_bridge_info();
		s = get_netif_mask(HUB_ETH_DEVICE);
	}
	if ((h&s) != (req&s)) { return 0; }
#if 0
	if ((((req&0xff000000)>>24) - 10) != tran->port_num) { return 0; }

	for(i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (i == tran->port_num) { continue; }
		if (discovery[i].state != IPCAM_DISC_STATE_IPDONE && discovery[i].state != IPCAM_DISC_STATE_DONE) { continue; }
		if (discovery[i].ipaddr == req) { return 0; }
	}
#endif
	return 1;
}

static int check_if_openmode_valid_req(DHCP_TRANSACTION* tran)
{
	int i = 0;
	unsigned int req = 0;
	unsigned int h,s;

	IPCAM_DBG(MAJOR, "start\n");

	if(tran == NULL)
	{
		IPCAM_DBG(MINOR, "invalid(NULL transaction)\n");
		return 0;
	}
	req = tran->req_ip;

	if(nf_get_running_mode()) {
		h = get_bridge_info();
		s = get_netif_mask(HUB_ETH_DEVICE);
	}

	return 1;
}

// dhcp server bug (DHCP Server is sending "Request, Ack Msg" is too late)
extern void nf_ipcam_openmode_dhcpd(void)
{
	int i = 0;
	int len = 0;
	int rcv_sock = 0;
	int port_hub = 0;
	struct sockaddr_in sin;

	unsigned char msg_buff[SOCK_BUF_SZ];

	unsigned char *p;
	unsigned short *p_short;
	unsigned int *p_int;
	unsigned int tran_id;		// transaction id
	unsigned char op_tag = 0;	// option tag
	unsigned char op_len = 0;	// option length
	unsigned char msg_type_rcv;
	unsigned char msg_type_send;
	unsigned int temp_ip; // mapping ip
	char *temp_ipstr[32]; // mapping ip str
	openmode_mac_list mac_list[NUM_ACTIVE_CH];

	//init_send_sock();
	memset(&tran, 0x00, sizeof(tran));
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(DHCP_SVR_PORT);

	IPCAM_DBG(MAJOR, "start\n");

	memset(&mac_list, 0x00, sizeof(openmode_mac_list) * NUM_ACTIVE_CH);

	if ((rcv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		IPCAM_DBG(MINOR, "socket error\n");
		perror("socket");
		return ;
	}

	if (bind(rcv_sock, (struct sockaddr*) &sin, sizeof(sin)) == -1)
	{
		IPCAM_DBG(MINOR, "bind error\n");
		perror("bind");
		return ;
	}
	printf("\e[31m [%s][%d] start \e[0m\n", __func__, __LINE__);
	// #1. Init mac_ip_mapping_table
	read_mapping_table();
	//print_mac_table();

	/*
	for(i=0; i<MAPPING_SIZE; i++)
	{
		mac_ip_mapping_table[i].ip = 0;
		mac_ip_mapping_table[i].mac[0] = 0;
		mac_ip_mapping_table[i].mac[1] = 0;
		mac_ip_mapping_table[i].mac[2] = 0;
		mac_ip_mapping_table[i].mac[3] = 0;
		mac_ip_mapping_table[i].mac[4] = 0;
		mac_ip_mapping_table[i].mac[5] = 0;
	}
	*/
	

	while (nf_openmode_get_dhcpd_state())
	{
		memset(msg_buff, 0x00, SOCK_BUF_SZ);
		msg_type_rcv = 0;
		msg_type_send = 0;
		usleep(50*1000);
		len = recvfrom(rcv_sock, msg_buff, SOCK_BUF_SZ, MSG_DONTWAIT, NULL, NULL);

		if (len == 0)
		{
			printf("%s [%s] ERROR: %s:%d socket broken\n", CAM_LOG_DOMAIN, __FUNCTION__, __FILE__, __LINE__);
			sleep(1);
			continue;
		}

		if (len < 0)
		{
			usleep(10*1000);
			continue;
		}

		if (nf_ipcam_is_vendor_zig())
		{
			sleep(1);
			continue;
		}

		p = &msg_buff[0];

		/***************************** bootp start *****************************/
		/* MSG type check: Boot request */
		if (*p++ != 0x01)
		{
			IPCAM_DBG(MINOR, "MSG_TYPE is not a 'Bootstrap_Request'\n");
			continue;
		}

		/* HW type check: Ethernet */
		if (*p++ != 0x01)
		{
			IPCAM_DBG(MINOR, "HW_TYPE is not an 'Ethernet'\n");
			continue;
		}

		/* HW address length: 6 bytes */
		if (*p++ != 0x06)
		{
			IPCAM_DBG(MINOR, "HW_ADDR_LEN fault(%02x)\n", *--p);
			continue;
		}

		/* Hops check: This must be 0 */
		if (*p++ != 0x00)
		{
			IPCAM_DBG(MINOR, "None-zero HOPS\n");
			continue;
		}

		/* transaction id */
		p_int = (void*)p;
		tran_id = ntohl(*p_int++);
		p = (void*)p_int;

		/* seconds elapsed */
		p += 2;
		/* bootp flags: unicast */
		p += 2;
		/* ip addresses */
		p += 16;

		/* client mac address */
		memcpy(&tran.cl_mac[0], p, 6);
		p += 16;

		/* server host name */
		p += 0x40;
		/* boot file name */
		p += 0x80;
		/* magic cookie */
		p += 4;

		tran.tran_id = tran_id;


		IPCAM_DBG(MINOR, "port_mac (%02x:%02x:%02x:%02x:%02x:%02x)\n",
				tran.cl_mac[0], tran.cl_mac[1],	tran.cl_mac[2],	
				tran.cl_mac[3],	tran.cl_mac[4],	tran.cl_mac[5]);

		tran.my_ip = get_bridge_info();
		if(tran.my_ip == 0)
			continue;

		//available ip check 
		tran.cl_ip = get_bridge_dhcpd_available_ip();
		if(tran.cl_ip == 0)
			continue;

		tran.subnet = get_netif_mask(HUB_ETH_DEVICE);
		if(tran.subnet == 0)
			continue;

		/* bootp option fields */
		op_tag = *p++;
		while (op_tag != 0xff)
		{
			op_len = *p++;
			switch(op_tag)
			{
				case 50:	// DHCP requested IP address
					{
						tran.req_ip = *p | (*(p+1) << 8) | (*(p+2) << 16) | (*(p+3) << 24);
						p += op_len;
						printf("[%s][%d] option: request (%d.%d.%d.%d)\n", __func__, __LINE__,
								PRINT_IP(tran.req_ip));
					}
					break;
				case 53:	// DHCP message type
					{
						printf("[%s][%d] option: dhcp msg\n", __func__, __LINE__);
						msg_type_rcv = *p++;
					}
					break;
				case 55:	// Parameter request list
				case 57:	// Maximum DHCP message size
				case 60:	// Vendor class identifier
				case 61:	// Client identifier
				default:	// Skip this option
					p += op_len;
					break;
			}
			op_tag = *p++;
		}

		switch(msg_type_rcv)
		{
			case 1:		// DHCP discover
				{
					int i = 0;
					int offer_flag = 0;
					for(i = 0; i < NUM_ACTIVE_CH; i++)
					{
						if(memcmp(&mac_list[i].mac[0], &tran.cl_mac[0], 6) == 0)
						{
							mac_list[i].offer_count++;
							offer_flag = 1;
							break;
						}
					}

					if(offer_flag == 1)
					{
						
						if(mac_list[i].offer_count > 3) // 원래 10, 임시로 3 변경
						{
							memset(&mac_list[i], 0x00, sizeof(openmode_mac_list));
							continue;
						}
					}
					else
					{
						for(i = 0; i < NUM_ACTIVE_CH; i++)
						{
							if(mac_list[i].mac[0] == 0x00 && mac_list[i].mac[1] == 0x00)
								break;
						}

						memcpy(&mac_list[i].mac[0], &tran.cl_mac[0], 6);
						mac_list[i].offer_count = 1;
						
						// #2// 1) mapping 되어있는 MAC이 있는지 체크
						for(i=0; i<MAPPING_SIZE; i++)
						{
							if(compare_mapping_mac(mac_ip_mapping_table[i])) // compare tran.cl_mac <-> mapping tbl mac
							{
								temp_ip = (tran.cl_ip&0x00ffffff)|(mac_ip_mapping_table[i].ip&0xff000000);
								memset(temp_ipstr, 0x00, sizeof(char)*32);
								snprintf(temp_ipstr, 16, "%d.%d.%d.%d",
										(temp_ip&0xff), (temp_ip&0xff00)>>8, (temp_ip&0xff0000)>>16, (temp_ip&0xff000000)>>24);
								
								//check my_ip
								if(check_conflict_my_dhcp_server_ip(temp_ip))
								{
									printf("\e[31m [%s][%d] mac_ip is dhcp server ip(%s) ...\e[0m\n", __func__, __LINE__, temp_ipstr);
									break;
								}

								//arping conflict check
								if(_openmode_dhcp_ip_conflict(HUB_ETH_DEVICE, temp_ipstr) > 0)
								{
									printf("\e[31m [%s][%d] mac_ip existed same ip(%s) ...\e[0m\n", __func__, __LINE__, temp_ipstr);
									break;
								}
								else
									tran.cl_ip = temp_ip; // bring mapping ip in mac table
							}
						}

						printf("[%s][%d] DHCP DISCOVER | send_offer(%d.%d.%d.%d) \n", __func__, __LINE__, PRINT_IP(tran.cl_ip));
						printf("[%s][%d] recv discover -> send offer \n", __func__, __LINE__);
						send_offer(1);
					}
				}
				break;
			case 3:		// DHCP request
				{
					if (check_if_openmode_valid_req(&tran))
					{
						printf("[%s][%d] DHCP REQUEST \n", __func__, __LINE__);
						tran.cl_ip = tran.req_ip;
						printf("[%s][%d] send_ack(%d.%d.%d.%d)\n", __func__, __LINE__, PRINT_IP(tran.cl_ip));
						printf("[%s][%d] recv request -> send ack \n", __func__, __LINE__);
						send_ack(1);

						//flush mac
						for(i = 0; i < NUM_ACTIVE_CH; i++)
						{
							if(memcmp(&mac_list[i].mac[0], &tran.cl_mac[0], 6) == 0)
							{
								printf("[%s][%d] mac flush..\n", __func__, __LINE__);
								memset(&mac_list[i], 0x00, sizeof(openmode_mac_list));
								break;
							}
						}
					}
					else
					{
						printf("[%s][%d] recv request -> send nack \n", __func__, __LINE__);
						send_nack(1);
					}
				}
				break;
			default:
				break;
		}

		memset(&tran, 0x00, sizeof(tran));
		/****************************** bootp end ******************************/
	}

	close(rcv_sock);
	rcv_sock = (-1);
	printf("\e[31m [%s][%d] end \e[0m\n", __func__, __LINE__);
}

// Start udhcpd instead of an existing DHCP Server
extern void nf_ipcam_start_udhcpd(void)
{

	// 1. Default Subnet : /24
	// 2. Default DNS : 8.8.8.8 , 8.8.4.4
	// 3. Default path at 'dhcp config, lease file' : /etc

	// config dhcp network from DB
	unsigned int dhcp_ip;
	unsigned int pool_start;
	unsigned int pool_end;
	unsigned int temp;

	char *ip_addr[32];
	char *start_addr[32];
	char *end_addr[32];
	// char *dns[32];

	dhcp_ip = nf_sysdb_get_uint("net.eth2.ipaddr");
	pool_start = nf_sysdb_get_uint("net.eth2.dhcp.pool_start");
	pool_end = nf_sysdb_get_uint("net.eth2.dhcp.pool_end");

	memset(ip_addr, 0x00, 32);
	snprintf(ip_addr, 16, "%d.%d.%d.%d",(dhcp_ip&0xff000000)>>24, (dhcp_ip&0xff0000)>>16, 
			(dhcp_ip&0xff00)>>8, (dhcp_ip&0xff));
	DHCP_GW = ip_addr;

	memset(start_addr, 0x00, 32);
	snprintf(start_addr, 16, "%d.%d.%d.%d",(pool_start&0xff000000)>>24, (pool_start&0xff0000)>>16, 
			(pool_start&0xff00)>>8, (pool_start&0xff));
	DHCP_START = start_addr;

	memset(end_addr, 0x00, 32);
	snprintf(end_addr, 16, "%d.%d.%d.%d",(pool_end&0xff000000)>>24, (pool_end&0xff0000)>>16, 
			(pool_end&0xff00)>>8, (pool_end&0xff));
	DHCP_END = end_addr;

	// print log 
	/*
	printf("[khkh][%s][%d][kh] \
			IP:[%s]\n\
			START:[%s]\n\
			END:[%s]\n\
			GW:[%s]\n\
			NETMASK:[%s]\n\
			DNS1:[%s]\n\
			DNS2:[%s]\n\
			LEASE:[%s]\n", __func__, __LINE__, DHCP_GW, DHCP_START,DHCP_END,DHCP_GW,DHCP_NETMASK,
			DHCP_DNS1,DHCP_DNS2,DHCP_LEASE);
			*/


	// make udhcpd.config and udhcpd.lease
#ifdef USE_PROXY_SYSTEM
	proxy_system("touch /etc/udhcpd.lease",1,3);
#else
	system("touch /etc/udhcpd.lease");
#endif
	
	FILE *fs;
	fs = fopen(UDHCPD_CONFIG, "w");
	if(fs == NULL)
	{
		printf("[%s][%d] '%s' fail to open, err:[%s]\n",__func__,__LINE__, UDHCPD_CONFIG, strerror(errno));
	}
	fprintf(fs, "start %s\n", DHCP_START);
	fprintf(fs, "end %s\n", DHCP_END);
	fprintf(fs, "option dns %s %s\n", DHCP_DNS1, DHCP_DNS2);
	fprintf(fs, "option subnet %s\n", DHCP_NETMASK);
	fprintf(fs, "option router %s\n", DHCP_GW);
	fprintf(fs, "option domain %s\n", DHCP_DOMAIN);
	fprintf(fs, "option lease %s\n", DHCP_LEASE);
	fprintf(fs, "lease_file %s\n", UDHCPD_LEASE_FILE);
	fprintf(fs, "interface %s", HUB_ETH_DEVICE);
	fclose(fs);

	// start udhcpd
#ifdef USE_PROXY_SYSTEM
	proxy_system("udhcpd -f &", 1, 3);
#else
	system("udhcpd -f &");
#endif

}

#define RECV_CONFLICT_STR	"Received"
static size_t _openmode_dhcp_ip_conflict(const char *dev, const char *ip)
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

		ptr = strstr(line, RECV_CONFLICT_STR);
		if(ptr == NULL)
		{
			continue;
		}

		ptr += strlen(RECV_CONFLICT_STR);

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

unsigned int get_bridge_dhcpd_available_ip()
{
	unsigned int ip_address_available = 0;
	int assinged_ip = 0;
	int i = 0;
	unsigned int netmask = 0;
	unsigned int br_ip = 0;
	unsigned int class_c_ipbase = 0;
	unsigned int assigned_ip = 0;

	char *ipstr[32];

	unsigned int ip_pool_start = 0; //0~1 : special purpose band (ex gateway)
	unsigned int ip_pool_end = 0; //255 : special purpose band (ex broadcast)
	unsigned int ip_pool_swap = 0;

	int is_mapping_ip_suggested_flag = 1; // Flag for Mapping IP Suggest

	netmask = get_netif_mask(HUB_ETH_DEVICE);
	if(netmask == 0)
		return 0;

	br_ip = get_bridge_info();
	if(br_ip == 0)
		return 0;

	ip_pool_start = nf_sysdb_get_uint("net.eth2.dhcp.pool_start");
	ip_pool_end = nf_sysdb_get_uint("net.eth2.dhcp.pool_end");
	ip_pool_start = (ip_pool_start&0xff);
	ip_pool_end = (ip_pool_end&0xff);

	if(ip_pool_start < 2 || ip_pool_start > 254)
		ip_pool_start = 2;
	if(ip_pool_end > 254 || ip_pool_end < 2)
		ip_pool_end = 254;
	if(ip_pool_start > ip_pool_end)
	{
		ip_pool_swap = ip_pool_end;
		ip_pool_end = ip_pool_start;
		ip_pool_start = ip_pool_swap;
	}

	while(1)
	{
		memset(ipstr, 0x00, 32);
		snprintf(ipstr, 16, "%d.%d.%d.%d", (br_ip&0xff), (br_ip&0xff00)>>8, (br_ip&0xff0000)>>16, (br_ip&0xff000000)>>24);
/*
		if(is_mapping_ip_suggested_flag)
		{
			// #2
			// 1) mapping 되어있는 MAC이 있는지 체크
			for(i=0; i<MAPPING_SIZE; i++)
			{
				if(compare_mapping_mac(mac_ip_mapping_table[i])) // compare tran.cl_mac <-> mapping tbl mac
				{
					printf("★★★ Find MAC!! \n");
					is_mapping_ip_suggested_flag = 0;
					assigned_ip = mac_ip_mapping_table[i].ip&0xff;
					break;
				}
				else
				{
					if(i==MAPPING_SIZE-1) // not exist
					{
						printf("★★★ Not Found MAC  ...\n");
						is_mapping_ip_suggested_flag = 0;
						assigned_ip = (rand() % (ip_pool_end-ip_pool_start+1)) + ip_pool_start; //range pool setting from DB
					}
					else
						continue;
				}
			}
		}
		else
		{
			printf("★★★ Normal .. \n");
			is_mapping_ip_suggested_flag = 0;
			assigned_ip = (rand() % (ip_pool_end-ip_pool_start+1)) + ip_pool_start; //range pool setting from DB
		}
*/
		assigned_ip = (rand() % (ip_pool_end-ip_pool_start+1)) + ip_pool_start; //range pool setting from DB

		class_c_ipbase = br_ip & netmask;
		ip_address_available = class_c_ipbase | (assigned_ip << 24);

		//host ip conflict check
		if(assigned_ip == ((br_ip&0xff000000)>>24)){
			printf("\e[31m [%s][%d] [br0] is same ip... reset up\e[0m\n", __func__, __LINE__);
			usleep(100);
			continue;
		}

		//arping conflict check 
		if(_openmode_dhcp_ip_conflict(HUB_ETH_DEVICE, ipstr) > 0) {
			printf("\e[31m [%s][%d] existed same ip(%s) ... reset up\e[0m\n", __func__, __LINE__, ipstr);
			usleep(100);
			continue;
		}

		memset(ipstr, 0x00, sizeof(char)*32);
		snprintf(ipstr, 16, "%d.%d.%d.%d", 
				(ip_address_available&0xff), 
				(ip_address_available&0xff00)>>8, 
				(ip_address_available&0xff0000)>>16, 
				(ip_address_available&0xff000000)>>24);

		//arping conflict check 
		if(_openmode_dhcp_ip_conflict(HUB_ETH_DEVICE, ipstr) > 0) {
			printf("\e[31m [%s][%d] existed same ip(%s) ... reset up\e[0m\n", __func__, __LINE__, ipstr);
			usleep(100);
			continue;
		}
		break;
	}

	char test_ip[16];
	memset(test_ip, 0x00, 16);
	snprintf(ipstr, 16, "%d.%d.%d.%d", (ip_address_available&0xff), (ip_address_available&0xff00)>>8,
			(ip_address_available&0xff0000)>>16, (ip_address_available&0xff000000)>>24);

	printf("[%s][%d] DHCP assigend available ip addr (%s) \n", __func__, __LINE__, ipstr);

	return ip_address_available;
}

int compare_mapping_mac(MAC_IP_MAPPING_TABLE m_t)
{
	int i=0;
	for(i=0; i<6; i++)
	{
		if(m_t.mac[i] != tran.cl_mac[i])
			return 0;
	}
	return 1; // m_t mac == tran.cl_mac
}

void print_mac_table()
{
	int i=0;
	for(i=0; i<MAPPING_SIZE; i++)
	{
		printf("\e[31m [ (%d) MAC : %u:%u:%u:%u:%u:%u , IP : %d.%d.%d.%d] \e[0m\n",
				i,
				mac_ip_mapping_table[i].mac[0],mac_ip_mapping_table[i].mac[1],mac_ip_mapping_table[i].mac[2],
				mac_ip_mapping_table[i].mac[3],mac_ip_mapping_table[i].mac[4],mac_ip_mapping_table[i].mac[5],
				PRINT_IP(mac_ip_mapping_table[i].ip));
	}
}

// if ip conflict, return 1;
int check_conflict_my_dhcp_server_ip(unsigned int check_ip)
{
	unsigned int my_ip = 0;
	//char *ipstr[32];

	my_ip = get_bridge_info();
	if(my_ip == 0)
		return 0;

	return check_ip == my_ip;
	//memset(ipstr, 0x00, 32);
	//snprintf(ipstr, 16, "%d.%d.%d.%d", (my_ip&0xff), (my_ip&0xff00)>>8, (my_ip&0xff0000)>>16, (my_ip&0xff000000)>>24);
	
}

// read file '/NFDVR/mapping_table'
void read_mapping_table()
{
	//mapping_table_path
	FILE *fp = fopen(MAPPING_TABLE_PATH, "r");
	char line[255];
	unsigned char mac[6];
	unsigned int ip[4];
	int i=0;
	
	mapping_table_index = 0;

	if(fp==NULL)
	{
		printf("\e[%s][%d] `%s` can't open \n", __func__, __LINE__, MAPPING_TABLE_PATH);
		
		//init mapping_table
		for(i=0; i<MAPPING_SIZE; i++)
		{
			memset(mac_ip_mapping_table[i].mac, 0x00, 6);
			mac_ip_mapping_table[i].ip = 0;
		}
		mapping_table_index = 0;
		
		return;
	}
	else
	{
		//printf("★★★ [%s][%d] READ FILE `%s` \n", __func__, __LINE__, MAPPING_TABLE_PATH);
		i=0;
		while(fgets(line, sizeof(line), fp) != NULL)
		{
			if(i>= MAPPING_SIZE)
			{
				fclose(fp);
				return;
			}
			sscanf(line, "%2X:%2X:%2X:%2X:%2X:%2X %u.%u.%u.%u",&mac[0],&mac[1], &mac[2], &mac[3], &mac[4], &mac[5],
					&ip[0],&ip[1],&ip[2],&ip[3]);
			//printf("★★★ [%d]-Reading, [%02X:%02X:%02X:%02X:%02X:%02X]-[%u]\n", i, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ip);
			mac_ip_mapping_table[i].mac[0] = mac[0];
			mac_ip_mapping_table[i].mac[1] = mac[1];
			mac_ip_mapping_table[i].mac[2] = mac[2];
			mac_ip_mapping_table[i].mac[3] = mac[3];
			mac_ip_mapping_table[i].mac[4] = mac[4];
			mac_ip_mapping_table[i].mac[5] = mac[5];
			
			/*
			printf("★★★ Table[%d], [%02X:%02X:%02X:%02X:%02X:%02X]-[%u]\n", i, 
					mac_ip_mapping_table[i].mac[0],
					mac_ip_mapping_table[i].mac[1],
					mac_ip_mapping_table[i].mac[2],
					mac_ip_mapping_table[i].mac[3],
					mac_ip_mapping_table[i].mac[4],
					mac_ip_mapping_table[i].mac[5],
					mac_ip_mapping_table[i].ip);
			*/
			mac_ip_mapping_table[i].ip = ((ip[3]<<24) | (ip[2]<<16) | (ip[1]<<8) | (ip[0]));

			//setting index
			if(ip!=0)
				mapping_table_index = (mapping_table_index+1)%MAPPING_SIZE;
			i=i+1;
		}
	}
	print_mac_table();

	fclose(fp);
	return;
}

void write_mapping_table()
{
	//printf("★★★ WRITE FILE \n");
	FILE *fp = fopen(MAPPING_TABLE_PATH, "w");
	int i=0;

	if(fp==NULL)
	{
		printf("\e[%s][%d] WRITE FILE CAN'T OPEN `%s` \n", __func__, __LINE__, MAPPING_TABLE_PATH);
		return;
	}
	for(i=0; i<MAPPING_SIZE; i++)
	{
		/*
		printf("★★★ [%d]-Writing Table, [%02X:%02X:%02X:%02X:%02X:%02X]-[%u]\n", i,
				mac_ip_mapping_table[i].mac[0],
				mac_ip_mapping_table[i].mac[1],
				mac_ip_mapping_table[i].mac[2],
				mac_ip_mapping_table[i].mac[3],
				mac_ip_mapping_table[i].mac[4],
				mac_ip_mapping_table[i].mac[5],
				mac_ip_mapping_table[i].ip);
		*/

		fprintf(fp,"%02X:%02X:%02X:%02X:%02X:%02X",
				mac_ip_mapping_table[i].mac[0],
				mac_ip_mapping_table[i].mac[1],
				mac_ip_mapping_table[i].mac[2],
				mac_ip_mapping_table[i].mac[3],
				mac_ip_mapping_table[i].mac[4],
				mac_ip_mapping_table[i].mac[5]);

		fprintf(fp, " %u.%u.%u.%u\n", 
				(mac_ip_mapping_table[i].ip&0x000000ff),
				(mac_ip_mapping_table[i].ip&0x0000ff00)>>8,
				(mac_ip_mapping_table[i].ip&0x00ff0000)>>16,
				(mac_ip_mapping_table[i].ip&0xff000000)>>24);
	}

	fclose(fp);
	return;
}

