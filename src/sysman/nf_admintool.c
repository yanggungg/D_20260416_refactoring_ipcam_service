#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>

#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "nf_sysman.h"
#include "nf_common.h"
#include "nf_util_netif.h"
#include "nf_admintool.h"
#include "../service/ddns2_md5.h"
#include "nf_notify.h"

//#define _ADMIN_DEBUG 
#define _ADMIN_AUTH_IPSETUP 

#if defined (_ADMIN_CBC)
	#if defined (_ADMIN_IPX_0412) || defined (_ADMIN_IPX_0412VE)
		#define MODELNAME "NR4HL"
	#elif defined (_ADMIN_IPX_0824) || defined (_ADMIN_IPX_0824VE)
		#define MODELNAME "NR8HL"
	#elif defined (_ADMIN_IPX_0824P) || defined (_ADMIN_IPX_0824P3ECO) || defined (_ADMIN_IPX_0824P3) || defined (_ADMIN_IPX_0824P4)
		#define MODELNAME "NR8H"
	#elif defined (_ADMIN_IPX_1648P) || defined (_ADMIN_IPX_1648VE) || defined(_ADMIN_IPX_1648P3) || defined (_ADMIN_IPX_1648P3ECO) || defined (_ADMIN_IPX_1648P4)
		#define MODELNAME "NR16H"
	#else
		#define MODELNAME "NR"
	#endif
#elif defined (_ADMIN_VICON)
	#if defined (_ADMIN_IPX_0412) || defined (_ADMIN_IPX_0412VE)
		#define MODELNAME "HDXPRESS-4"
	#elif defined (_ADMIN_IPX_0824) || defined (_ADMIN_IPX_0824VE)
		#define MODELNAME "HDXPRESS-8"
	#elif defined (_ADMIN_IPX_0824P) || defined (_ADMIN_IPX_0824P3ECO) || defined (_ADMIN_IPX_0824P3) || defined (_ADMIN_IPX_0824P4)
		#define MODELNAME "HDXPRESS-8"
	#elif defined (_ADMIN_IPX_1648P) || defined (_ADMIN_IPX_1648VE) || defined(_ADMIN_IPX_1648P3) || defined (_ADMIN_IPX_1648P3ECO) || defined (_ADMIN_IPX_1648P4)
		#define MODELNAME "HDXPRESS-16"
	#else
		#define MODELNAME "HDXPRESS"
	#endif
#else
	#if defined (_ADMIN_IPX_0412)
		#define MODELNAME "IPX_0412"
	#elif defined (_ADMIN_IPX_0824)
		#define MODELNAME "IPX_0824"
	#elif defined (_ADMIN_IPX_0824P)
		#define MODELNAME "IPX_0824P"
	#elif defined (_ADMIN_IPX_1648P)
		#define MODELNAME "IPX_1648P"
	#elif defined (_ADMIN_IPX_0412VE)
		#define MODELNAME "IPX_0412VE3"
	#elif defined (_ADMIN_IPX_0824VE)
		#define MODELNAME "IPX_0824VE"
	#elif defined (_ADMIN_IPX_0412ECO)
		#define MODELNAME "IPX_0412ECO"
	#elif defined (_ADMIN_IPX_0824ECO)
		#define MODELNAME "IPX_0824ECO"
	#elif defined (_ADMIN_IPX_0824P3ECO)
		#define MODELNAME "IPX_0824P3ECO"
	#elif defined (_ADMIN_IPX_1648VE)
		#define MODELNAME "IPX_1648VE3"
	#elif defined (_ADMIN_IPX_0824P3)
		#define MODELNAME "IPX_0824P3"
	#elif defined (_ADMIN_IPX_1648P3)
		#define MODELNAME "IPX_1648P3"
	#elif defined (_ADMIN_IPX_1648P3ECO)
		#define MODELNAME "IPX_1648P3ECO"
	#elif defined (_ADMIN_IPX_1648P4)
		#define MODELNAME "IPX_1648P4"
	#elif defined (_ADMIN_IPX_0824P4)
		#define MODELNAME "IPX_0824P4"
	#elif defined (_ADMIN_IPX_0412L4)
		#define MODELNAME "IPX_0412L4"
	#elif defined (_ADMIN_IPX_0824M4)
		#define MODELNAME "IPX_0824M4"
	#elif defined (_ADMIN_IPX_0412M4)
		#define MODELNAME "IPX_0412M4"
	#elif defined (_ADMIN_IPX_1648M4)
		#define MODELNAME "IPX_1648M4"
	#elif defined (_ADMIN_IPX_0824P4E)
		#define MODELNAME "IPX_0824P4E"
	#elif defined (_ADMIN_IPX_1648P4E)
		#define MODELNAME "IPX_1648P4E"
	#elif defined (_ADMIN_IPX_32P4E)
		#define MODELNAME "IPX_32P4E"
	#elif defined (_ADMIN_IPX_0824M4E)
		#define MODELNAME "IPX_0824M4E"
	#elif defined (_ADMIN_IPX_0412M4E)
		#define MODELNAME "IPX_0412M4E"
	#elif defined (_ADMIN_IPX_1648M4E)
		#define MODELNAME "IPX_1648M4E"	
	#elif defined (_ADMIN_IPX_32M4E)
		#define MODELNAME "IPX_32M4E"
	//_IPX_32P5
	#else
		#define MODELNAME "IPX"
	#endif
#endif

static GThread *p_thread;
struct adminToolStruct g_adminToolData;
static unsigned int g_xid = 0;

gboolean nf_admintool_init(void);

static void print_msg(netconf_Message *msg)
{
	if (msg == NULL)
		return;

	g_message("OPCODE[%d] SECS[%d] XID[0x%08X] MAGIC[0x%08X] VERSION[%d]", msg->opcode,  msg->secs, msg->xid, msg->magic, msg->version);
	g_message("C-MAC: %02X:%02X:%02X:%02X:%02X:%02X", msg->chaddr[0], msg->chaddr[1], msg->chaddr[2], msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
	g_message("C-IP: %s", inet_ntoa(*(struct in_addr*)&msg->ciaddr)); // ip
	g_message("C-SM: %s", inet_ntoa(*(struct in_addr*)&msg->miaddr)); // subnet mask
	g_message("C-GW: %s", inet_ntoa(*(struct in_addr*)&msg->giaddr)); // gateway
	g_message("C-D1: %s", inet_ntoa(*(struct in_addr*)&msg->d1iaddr)); //
	g_message("C-D2: %s", inet_ntoa(*(struct in_addr*)&msg->d2iaddr));
	g_message("Y-IP: %s", inet_ntoa(*(struct in_addr*)&msg->yiaddr));
	g_message("H-PT: %d", ntohs(msg->http_port));

}

static int init_adminToolData(struct adminToolStruct *adminToolData, int reinit, int ack_result)
{
	char *sysmodel = NULL;
	netconf_Message *set_msg;
	NF_NETIF_GET_INFO net_info;
	u_int8_t vend_tmp[64] = {0,};

	if (adminToolData == NULL)
		return -1;

	if (!nf_netif_get_info(&net_info))
		return -1;

	if (reinit) {
		memset(&adminToolData->ipset.pre_netconf, 0, sizeof(netconf_Message));
		memset(&adminToolData->ipset.netconf, 0, sizeof(netconf_Message));
	}
	else {
		memset(adminToolData, 0, sizeof(struct adminToolStruct));
	}


	adminToolData->dhcpon = nf_sysdb_get_bool("net.proto.dhcpon");
	set_msg = &adminToolData->ipset.pre_netconf;

	set_msg->version	= (adminToolData->dhcpon ? 16: SET_VERSION);
	set_msg->magic		= htonl(SET_MAGIC);
	set_msg->opcode		= MSG_CAM_ACK;
	set_msg->secs		= 0;
	set_msg->xid 		= g_xid;

	set_msg->chaddr[0]	= net_info.mac_addr[0]; //0x00;
	set_msg->chaddr[1]	= net_info.mac_addr[1]; //0x11;
	set_msg->chaddr[2]	= net_info.mac_addr[2]; //0x5F;
	set_msg->chaddr[3]	= net_info.mac_addr[3]; //0xF0;
	set_msg->chaddr[4]	= net_info.mac_addr[4]; //0x03;
	set_msg->chaddr[5]	= net_info.mac_addr[5]; //0x00;

	set_msg->ciaddr		= htonl(net_info.ipaddr);	//inet_addr("192.168.10.30");
	set_msg->miaddr		= htonl(net_info.netmask);	//inet_addr("255.255.255.0");
	set_msg->giaddr		= htonl(net_info.gateway);	//inet_addr("192.168.10.1");
	set_msg->http_port	= htons(nf_sysdb_get_uint("net.proto.webport"));

	if ( nf_sysdb_get_bool("net.proto.httpson"))
	{
		set_msg->reserve_port = htons(1);
		set_msg->https_port = htons(nf_sysdb_get_uint("net.proto.webport"));
	}
	else
		set_msg->https_port	= 0;

	set_msg->rtsp_port	= htons(nf_sysdb_get_uint("net.proto.rtspport"));

	if (adminToolData->dhcpon) {
		set_msg->d1iaddr = htonl(nf_sysdb_get_uint("net.dhcp.dns1"));
		set_msg->d2iaddr = htonl(nf_sysdb_get_uint("net.dhcp.dns2"));
	}
	else {
		set_msg->d1iaddr = htonl(nf_sysdb_get_uint("net.proto.dns1"));
		set_msg->d2iaddr = htonl(nf_sysdb_get_uint("net.proto.dns2"));
	}
	
	/* make vend field */
	vend_tmp[0] = 0x17;
	vend_tmp[1] = 0x18;
	vend_tmp[2] = 0x1;
	vend_tmp[3] = NUM_ACTIVE_CH;
	vend_tmp[4] = 0x0;

	sysmodel = nf_sysdb_get_str_nocopy("sys.info.model");

	if(sysmodel)
	{	
		if(!strcmp(nf_sysdb_get_str_nocopy("sys.info.model"), ""))	// ITX
		{
			strcpy(&vend_tmp[5], MODELNAME);
		}
		else	// CBC, VICON...
		{
			strcpy(&vend_tmp[5], nf_sysdb_get_str_nocopy("sys.info.model"));
		}
	}
	else
	{
		strcpy(&vend_tmp[5], MODELNAME);
	}

	memcpy(set_msg->vend, vend_tmp, sizeof(u_int8_t)*64);

	set_msg->vend[30] = ack_result;
	
	//g_message("SET_MSG : [%x][%x][%x][%x][%x][%s][%d]",
	//				set_msg->vend[0],set_msg->vend[1],set_msg->vend[2],set_msg->vend[3],set_msg->vend[4],&set_msg->vend[5], set_msg->vend[30]);

	return 0;
}

static int init_listen_port(struct sockaddr_in *addr)
{
	int recv_sock;
	int so_reuseaddr = TRUE;

	if (addr == NULL)
		return -1;

	recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (recv_sock == -1)
		return -1;

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(LISTEN_PORT);

	if (setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR,	(void*)&so_reuseaddr, sizeof(so_reuseaddr))) {
		close(recv_sock);
		return -1;
	}

	if (bind(recv_sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1) {
		close(recv_sock);
		return -1;
	}

	return recv_sock;
}

static int init_send_port(struct sockaddr_in *addr)
{
	int send_sock;
	int on = 1;

	if (addr == NULL)
		return -1;

	send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (send_sock == -1)
		return -1;
	setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

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

static int do_write_ack(struct adminToolStruct *adminToolData)
{
	unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	struct in_addr source_addr,	dest_addr;

	if (adminToolData == NULL)
		return -1;

	netconf_Message *set_msg = &adminToolData->ipset.pre_netconf;

	memcpy((void*)set_msg, (void*)&adminToolData->ipset.pre_netconf, DATA_SIZE);
	set_msg->opcode = MSG_CAM_ACK;

#if _ADMIN_DEBUG
	print_msg(set_msg);
#endif

	/*prepare sockaddr_in*/
	struct sockaddr_in socket_address;
	memset(&socket_address, 0x00, sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = INADDR_BROADCAST;
	socket_address.sin_port = htons(SEND_PORT);

	/*send the packet*/
	//g_message("SET_MSG22 : [%x][%x][%x][%x][%x][%s][%d]",
	//				set_msg->vend[0],set_msg->vend[1],set_msg->vend[2],set_msg->vend[3],set_msg->vend[4],&set_msg->vend[5], set_msg->vend[30]);

	return sendto(adminToolData->send_sock, set_msg, sizeof(netconf_Message) , 0,	(struct sockaddr*)&socket_address, sizeof(socket_address));
}

static int set_sysdb_conf(struct adminToolStruct *adminToolData)
{
	netconf_Message *conf = &adminToolData->ipset.pre_netconf;
	gboolean dhcpon = (conf->version>>4) & 0x0F;

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	nf_sysdb_set_bool("net.proto.dhcpon", dhcpon);
	nf_sysdb_set_uint("net.proto.ipaddr", ntohl(conf->ciaddr));
	nf_sysdb_set_uint("net.proto.gateway", ntohl(conf->giaddr));
	nf_sysdb_set_uint("net.proto.subnet", ntohl(conf->miaddr));
	nf_sysdb_set_uint("net.proto.dns1", ntohl(conf->d1iaddr));
	nf_sysdb_set_uint("net.proto.dns2", ntohl(conf->d2iaddr));

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);
	nf_sysdb_save("net");
	nf_sysdb_save_flush();

	nf_network_stop(0x00010001);
	sleep(3);
	/* only NVR */	
	nf_ipcam_stop();		
	sleep(3);
	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);		
	
	nf_netif_init();
	sleep(3);
	
	/* only NVR */	
	nf_ipcam_ip_changed();
	nf_network_start();

	return 0;
}

static int response_request(struct adminToolStruct *adminToolData)
{
	netconf_Message *conf, *request_conf;
	unsigned char md5hash_hex[16] = {0,};
	unsigned char md5hash_str[33] = {0,};
	gchar *id = NULL;
	gchar *pass = NULL;
	gchar idpws[64+1] = {0,};
	guint dvr_status;
	int client_count = 0;
	
	char rcv_vend_buff[33] = {0,};
	int i = 0;

	if (adminToolData == NULL)
		return ACK_FAIL;

	conf = &adminToolData->ipset.pre_netconf;
	request_conf = &adminToolData->ipset.netconf;

	if (ntohl(request_conf->yiaddr) == 0xffffffff|| ntohl(request_conf->miaddr) == 0xffffffff)
		return ACK_FAIL;

#if defined(_ADMIN_AUTH_IPSETUP)
	/* ID/PW check */
	id = nf_sysdb_get_str_nocopy("usr.U0.name");
	pass = nf_sysdb_get_str_nocopy("usr.U0.pass");
	strcat(idpws, id);
	strcat(idpws, pass);
	strcat(idpws, AUTH_MAGIC);
//	g_message("IDPWS : [%s]", idpws);
	Md5Str(md5hash_hex, idpws);
	sprintf(md5hash_str,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
					md5hash_hex[0],md5hash_hex[1],md5hash_hex[2],md5hash_hex[3],
					md5hash_hex[4],md5hash_hex[5],md5hash_hex[6],md5hash_hex[7],
					md5hash_hex[8],md5hash_hex[9],md5hash_hex[10],md5hash_hex[11],
					md5hash_hex[12],md5hash_hex[13],md5hash_hex[14],md5hash_hex[15]);	
	g_message("Md5 		: [%s]", md5hash_str);
	g_message("rcv_vend : [%s]", &request_conf->vend[30]);
		
	if(strncmp(&request_conf->vend[30], md5hash_str, 32) != 0)
	{
		g_message("FAIL IP_SET : auth fail");
		return ACK_FAIL_AUTH;
	}

	dvr_status = nf_notify_get_param0("dvr_status");
	if(dvr_status > NF_DVR_STATUS_LIVE) 
	{
		g_message("FAIL IP_SET : dvr_status is not LIVE [%d]", dvr_status);
		return ACK_FAIL_LIVE;
	}
	client_count = nf_issm_ctl_get_session_count();
	if(client_count > 0)
	{
		g_message("FAIL IP_SET : connected client [%d]", client_count);
		return ACK_FAIL_WEB;	
	}
#endif

	conf->opcode = MSG_CAM_ACK;
	conf->ciaddr = request_conf->yiaddr;
	conf->miaddr = request_conf->miaddr;
	conf->giaddr = request_conf->giaddr;
	conf->d1iaddr = request_conf->d1iaddr;
	conf->d2iaddr = request_conf->d2iaddr;
	conf->version = request_conf->version;

	set_sysdb_conf(adminToolData);

	return ACK_OK;
}

static int process_packet(struct adminToolStruct *adminToolData)
{
	netconf_Message* msg;
	static int ack_result = 0;

	if (adminToolData == NULL)
		return -1;

	msg = &adminToolData->ipset.netconf;

	if ((msg->opcode != MSG_IP_SEARCH) && (msg->xid != g_xid))
		return 0;

	if (msg->magic != htonl(SET_MAGIC)) {
		g_message("Wrong Magic Number");
		return -1;
	}

	// -- ydh
#if _ADMIN_DEBUG
	g_message("from msg:");
	print_msg(msg);
#endif

	switch (msg->opcode) {
		case MSG_IP_SEARCH:
			init_adminToolData(adminToolData, 1, ack_result);
			
			if (do_write_ack(adminToolData) < 0)
				g_message("do_write_ack() fail");
			break;
		case MSG_IP_ACK:
			//Do nothing!!
			break;
		case MSG_IP_SET:
		case MSG_IP_SET_AUTH:
			ack_result = response_request(adminToolData);
			
			init_adminToolData(adminToolData, 1, ack_result);

			if (do_write_ack(adminToolData) < 0)
				g_message("do_write_ack() fail");

			break;
		default:
			g_message("Unknow msg code");
			return -1;
	}
	return 0;
}

static int do_read(struct adminToolStruct *adminToolData)
{
	int msg_len, addr_len;

	if (adminToolData == NULL)
		return -1;

	while(1) {
		addr_len = sizeof(adminToolData->recv_addr);
		msg_len = recvfrom(adminToolData->recv_sock, &adminToolData->ipset.netconf,
						   sizeof(netconf_Message), 0, &adminToolData->recv_addr, (socklen_t*)&addr_len);

		if (msg_len != sizeof(netconf_Message)) {
			g_message("msg len mismatch!");
			continue;
		}
		process_packet(adminToolData);
	}
	return 0;
}

static void init_admintool(void *data)
{
	struct adminToolStruct* adminToolData = (struct adminToolStruct*)data;

	if (init_adminToolData(adminToolData, 0, 0) < 0) {
		g_message("init_ip_set() error");
		return;
	}

	adminToolData->recv_sock = init_listen_port(&adminToolData->recv_addr);
	if (adminToolData->recv_sock == -1) {
		g_message("init_listen_port() error");
		return;
	}

	adminToolData->send_sock = init_send_port(&adminToolData->send_addr);
	if (adminToolData->send_sock == -1) {
		g_message("init_send_port error");
		close(adminToolData->recv_sock);
		return;
	}
		
	do_read(adminToolData);

	close(adminToolData->send_sock);
	close(adminToolData->recv_sock);
	return;
}

void random_bytes( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len; ++i )
		dest[i] = random() & 0xff;
}

gboolean nf_admintool_init(void)
{
	struct adminToolStruct* data=NULL;
	NF_NETIF_GET_INFO net_info;

	data = (struct adminToolStruct*)malloc(sizeof(struct adminToolStruct));

	/* Make XID */
	if (!nf_netif_get_info(&net_info))
	{
		g_message("XID generating fail");
	}
	else
	{
		random_bytes((unsigned char*)&g_xid, sizeof(g_xid) );
		g_xid =  net_info.mac_addr[3] << 24 | net_info.mac_addr[4] << 16 | net_info.mac_addr[5] <<8 | (g_xid & 0xFF);
	}

	p_thread = g_thread_create((GThreadFunc)init_admintool, (void*)&g_adminToolData, FALSE, data);
	return TRUE;
}

