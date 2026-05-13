#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <syslog.h>
#include "onvif_common.h"
#include "../ezxml/ezxml.h"
#include "nvs_discovery.h"
#include "nf_common.h"
#include "nvs_onvif_app_util.h"

#define TTL 5
#define MAXLINE 8192

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
#else
#define MODELNAME "IPX"
#endif

extern char* nf_onvif_get_eth_str(void);

extern int g_onvif_service_stop;

static int _onvif_discovery_loop(void);
int multi_hello(void);
void multi_bye();
static int multi_init(struct sockaddr_in *multi_addr);
static int device_discovery(char *recv_msg, char *send_buf);
// static int get_ipaddress(char *tmp_ip);
static void get_macaddress(char *tmp_mac);
static void get_scope(char (*scope)[COMMON_SIZE*2], int *scope_cnt);
static int scope_check(const char *ncx_scope, char *clnt_scope);
//static void Probe_Error_Send(char *send_buf, const char *probe_uuid);

static ezxml_t f2, sub2;
static char uuid_arr[64] = {0,};
static char urn_arr[64] = {0,};
static char soap_fault_msg[];
static char mac_addr[64]={0,};
static char *onvif_hello;

#define ONVIF_MCAST_PORT_LISTEN			3702
//#define WITH_IPX_ONVIF_DISCOVERY

#ifndef WITH_IPX_ONVIF_DISCOVERY
#define _WS_DISC_PORT 					(3702)
static int multi_sock = (-1);

static int nf_ipcam_get_msock_onvif(void)
{
	return multi_sock;
}

unsigned int is_https_required(void)
{
	if ( nf_sysdb_get_bool("net.proto.httpson")){
		return 1;
	}
	else {
		return 0;
	}
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
	sin_m.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_m.sin_port = htons(_WS_DISC_PORT);
	multi_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (multi_sock < 0)
	{
//		IPCAM_DBG(ERROR, "multicast rcv socket init failed\n");
		perror("recv socket");
		return (-1);
	}
	if (bind(multi_sock, (struct sockaddr*) &sin_m, sizeof(sin_m)) < 0)
	{
//		IPCAM_DBG(ERROR, "multicast rcv socket bind failed\n");
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
#endif


static unsigned int get_port(void)
{	
#ifdef _ONVIF_STAND_ALONE
    return nf_sysdb_get_uint("net.http.onvifport");
#else
	if( nf_sysdb_get_uint("net.http.sslon") == 2 ){ // SSL required.
		return nf_sysdb_get_uint("net.http.sslport");
	}
	else {
		return nf_sysdb_get_uint("net.proto.webport");
	}
#endif
}

void *onvif_discovery(void *arg)
{

#ifndef WITH_IPX_ONVIF_DISCOVERY
    	multi_sock = _init_multicast_sock();
#endif
        
       while( dvrReady_info() == 0 || nf_ipcam_get_msock_onvif() < 0 ){
            _TTY_LOG_ONVIF("wait for dvr_ready");
            sleep(2);
       }
        _TTY_LOG_ONVIF("discovery socket[%d]", nf_ipcam_get_msock_onvif());       

	f2 = ezxml_parse_str(probe_match, strlen(probe_match));
	get_macaddress(mac_addr);
	sprintf(uuid_arr, "uuid:08d34766-0347-11df-b53f-%s", mac_addr);
	sprintf(urn_arr, "urn:uuid:08d34766-0347-11df-b53f-%s", mac_addr);

	pthread_cleanup_push(multi_bye, (void *)NULL);
	_onvif_discovery_loop();
	pthread_cleanup_pop(0);

	pthread_exit(NULL);
}


static int _onvif_discovery_loop(void)
{
	//int send_sock;
	struct sockaddr_in server_addr;
	struct sockaddr_in from;
	//char recv_msg[MAXLINE];
	//int n;
	//unsigned int set = 1;
	//char *device_xml = NULL;
	socklen_t len;
	int udp_sock;
	struct sockaddr_in addr;
	struct ip_mreq join_addr;
	int state, str_len;
	int multi_sock;    
	char buf[MAXLINE];
	char send_buf[MAXLINE];

g_printf("\e[31m####################### %s ##################\e[0m\n", __FUNCTION__);

	if(multi_hello() < 0)
	{
		_TTY_LOG_ONVIF( "%s %d multi_hello() failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

#if 0 //[[ Jeonghun_0130718_BEGIN -- ipx onvif
	udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(udp_sock == -1)
	{
		close(udp_sock);
		perror("socket() error");
		//baek.debug
		_TTY_LOG_ONVIF("2");
		return -1;
	}

	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(ONVIF_MCAST_PORT_LISTEN);

	if(bind(udp_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		close(udp_sock);
		perror("bind() error");
		//baek.debug
		_TTY_LOG_ONVIF("3");
		return -1;
	}

	join_addr.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

	state = setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr));
	if(state)
	{
		close(udp_sock);
		perror("setsockopt() error");
		//baek.debug
		_TTY_LOG_ONVIF("4");
		return -1;
	}
#endif //]] Jeonghun_0130718_END -- ipx onvif

	multi_sock = nf_ipcam_get_msock_onvif();

	join_addr.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

	state = setsockopt(multi_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr));
	if(state)
	{
		close(multi_sock);
		perror("setsockopt() error");
		return -1;
	}
            
	len = sizeof(from);

	while(1) {
    		usleep(100*1000);
    		multi_sock = nf_ipcam_get_msock_onvif();
		if (multi_sock <= 0) { continue; }
		memset(buf, 0x00, MAXLINE);
		str_len = recvfrom(multi_sock, buf, MAXLINE-1, 0, (struct sockaddr *)&from, &len);
		if(str_len < 0)
			break;
		buf[str_len]=0;

		if( !device_discovery(buf, send_buf) ) continue;
		if(send_buf[0] == '\0')
			continue;

		int ret;
		ret = sendto(multi_sock, send_buf, strlen(send_buf), 0, (struct sockaddr *)&from, len);
		if( ret < 0 ){
		    perror("[ONVIF] onvif discovery send fail!!!");
		}
        
		memset(send_buf, 0x00, sizeof(send_buf));
	}
	close(multi_sock);

	return 0;
}

static int device_discovery(char *recv_msg, char *send_buf)
{
	ezxml_t f1, sub;
	static char device_xml[MAXLINE] = {0,};
	char *parse = NULL;
	//FILE *fp;
	char tmp[COMMON_SIZE*2] = {0,};
	char scope_tmp[MAX_SCOPE][COMMON_SIZE*2];
	char scope[1024]={0,};
	char addr[64]={0,};
	int i;
	char ip_addr[64]={0,};
	char probe_uuid[64]={0,};
	int scope_cnt = 0;
	unsigned int is_https = is_https_required();
	
	memset(send_buf, 0x00, sizeof(MAXLINE));

	if(get_ipaddress(ip_addr) < 0) {
		_TTY_LOG_ONVIF( "%s error\n", __FUNCTION__);
		return false;
	}
	memset(scope_tmp, 0x00, sizeof(scope_tmp));
	get_scope(scope_tmp, &scope_cnt);
	if(scope_cnt > 0) {
		for(i=0;i<scope_cnt;i++)
		{
			sprintf(tmp, "%s ", scope_tmp[i]);
			strcat(scope, tmp);
		}
	}

	sprintf(addr, "%s://%s:%d/onvif/device_service", is_https?"https":"http",ip_addr, get_port());

	f1 = ezxml_parse_str(recv_msg, strlen(recv_msg));

	//_TTY_LOG_ONVIF( "--------------------------------------------------\n");
	/*
	 ** 1. find MessageID:uuid
	 */
	//_TTY_LOG_ONVIF( "=============>>>>>>>>>>>>> Finding Header->MessageID:uuid...\n");
	sub = ezxml_strchild(f1, "Header");
	sub = ezxml_strchild(sub, "MessageID");
	if( sub ){
		if(sub->txt != NULL){
#if 0
			if( sub->txt[1] == 'r'){ // delete 'urn'
				strcpy(probe_uuid, &sub->txt[4]);
			}
			else
#endif	
			{
				strcpy(probe_uuid, sub->txt);
			}
		}
	}
	else{
		/*
		 ** Invalid messagid
		 */
		//_TTY_LOG_ONVIF( ">>>>>>>>>>>>> [ERR-Return]. Invalid MessageID..\n");
		ezxml_free(f1);		
		return false;
	}

	/*
	 ** 2. find Scopes
	 */
	//_TTY_LOG_ONVIF( "=============>>>>>>>>>>>>> Finding Body->Probe->Scopes...\n");
	sub = ezxml_strchild(f1, "Body");
	sub = ezxml_strchild(sub, "Probe");
	sub = ezxml_strchild(sub, "Scopes");
	if(sub)
	{
#if 0 //junsun.checkme.aaaaa
		if(sub->attr[0]) {
			strcpy(send_buf, soap_fault_msg);
			_TTY_LOG_ONVIF( ">>>>>>>>>>>>> Scopes Attr error.Sending [soap_fault_msg].\n");
			return true;
		}
#endif
		/*
		 ** 8.1.3.1 - NVT SEARCH WITH OMITTED DEVICE AND SCOPE TYPES
		 */
		if(sub->txt[0] == 0) {
			//_TTY_LOG_ONVIF( "8.1.3.1 - NVT SEARCH WITH OMITTED SCOPES\n");
		}
		else{
			//_TTY_LOG_ONVIF( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
			//_TTY_LOG_ONVIF( "%s\n", sub->txt);
			//_TTY_LOG_ONVIF( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
			/*
			 ** found scopes
			 */
			if(scope_check(scope, sub->txt) == false) {
				//_TTY_LOG_ONVIF( ">>>>>>>>>>>>> Scopes Not Found...\n");
				ezxml_free(f1);				
				return false;
			}
			//else if(sub->attr[0] != NULL) 
			//{
			//Probe_Error_Send(send_buf, probe_uuid);
			//return true;
			//}


			//_TTY_LOG_ONVIF( ">>>>>>>>>>>>> Scopes Found[%s]...\n", sub->txt);
		}
	}
	else{
		/*
		 ** 8.1.3.2 - NVT RESPONSE TO INVALID SEARCH REQUEST
		 */
		//_TTY_LOG_ONVIF( ">>>>>>>>>>>>> Scopes Attr warning.Skipping NVT8.1.3.1.\n");
	}


	/*
	 ** 3. find Types:NetworkVideoTransmitter
	 ** junsun.2010.10.15 don't need to find
	 */
	//_TTY_LOG_ONVIF( "=============>>>>>>>>>>>>> Finding Types:NetworkVideoTransmitter...\n");
	sub = ezxml_strchild(f1, "Body");
	sub = ezxml_strchild(sub, "Probe");
	sub = ezxml_strchild(sub, "Types");

	if(sub)
	{
		/*
		 ** 8.1.3.1 - NVT SEARCH WITH OMITTED DEVICE AND SCOPE TYPES
		 */
		if(sub->txt[0] == 0)
		{
			//_TTY_LOG_ONVIF( "8.1.3.1 - NVT SEARCH WITH OMITTED DEVICE TYPES\n");
		}
		else
		{
			if(strstr(sub->txt, "NetworkVideoTransmitter"))
			{
				//_TTY_LOG_ONVIF( "@@@@@@@@@@@@ NetworkVideoTransmitter name: %s\n", sub->name);
				//_TTY_LOG_ONVIF( "@@@@@@@@@@@@ NetworkVideoTransmitter text: %s\n", sub->txt);
			}
			else{
				//_TTY_LOG_ONVIF( "NetworkVideoTransmitter Type miss match 1\n");
				ezxml_free(f1);
				return false;
			}
		}
	}
	else{
		/*
		 ** 8.1.3.2 - NVT RESPONSE TO INVALID SEARCH REQUEST
		 */
		//_TTY_LOG_ONVIF( ">>>>>>>>>>>>> Types Attr warning.Skipping NVT8.1.3.1.\n");
		ezxml_free(f1);		
		return false;
	}

	//_TTY_LOG_ONVIF(  "@@@@@@@@@@@@<<<<<< uuid  : %s>>>>>>\n", probe_uuid);
	//_TTY_LOG_ONVIF(  "@@@@@@@@@@@@<<<<<< scope : %s>>>>>>\n", scope);

	ezxml_free(f1);

	sub2 = ezxml_child(f2, "soap:Header");
	sub2 = ezxml_child(sub2, "wsadis:MessageID");
	sub2 = ezxml_set_txt(sub2, probe_uuid);

	sub2 = ezxml_child(f2, "soap:Header");
	sub2 = ezxml_child(sub2, "wsadis:RelatesTo");
	sub2 = ezxml_set_txt(sub2, probe_uuid);

	sub2 = ezxml_child(f2, "soap:Body");
	sub2 = ezxml_child(sub2, "d:ProbeMatches");
	sub2 = ezxml_child(sub2, "d:ProbeMatch");
	sub2 = ezxml_child(sub2, "d:Scopes");
	sub2 = ezxml_set_txt(sub2, scope);

	sub2 = ezxml_child(f2, "soap:Body");
	sub2 = ezxml_child(sub2, "d:ProbeMatches");
	sub2 = ezxml_child(sub2, "d:ProbeMatch");
	sub2 = ezxml_child(sub2, "wsadis:EndpointReference");
	sub2 = ezxml_child(sub2, "wsadis:Address");
	sub2 = ezxml_set_txt(sub2, urn_arr);

	sub2 = ezxml_child(f2, "soap:Body");
	sub2 = ezxml_child(sub2, "d:ProbeMatches");
	sub2 = ezxml_child(sub2, "d:ProbeMatch");
	sub2 = ezxml_child(sub2, "d:XAddrs");
	sub2 = ezxml_set_txt(sub2, addr);

	parse = ezxml_toxml(f2);
	strncpy(device_xml, parse, strlen(parse)+1);
	strcpy(send_buf, device_xml);
	
	if( parse != NULL ) 	free(parse);
	return true;
}

int multi_hello(void)
{
	int sock, i;
	struct sockaddr_in multi_addr;
	char ip_addr[64]={0,};
	char tmp[128]={0,};
	char scope_tmp[MAX_SCOPE][COMMON_SIZE*2];
	char scope[1024]={0,};
	char addr[64]={0,};
	//int cnt = 0, n=0;
	int scope_cnt = 0;
	char tmp_hello[1024];
	ezxml_t f1, sub;
	char *onvif_hello = NULL;
	unsigned int is_https = is_https_required();
	
	if( g_onvif_service_stop ) {
		return -1;
	}
	
	if(get_ipaddress(ip_addr) < 0) {
		_TTY_LOG_ONVIF( "%s error\n", __FUNCTION__);
		return -1;
	}
	get_scope(scope_tmp, &scope_cnt);
	if(scope_cnt > 0) {
		for(i=0;i<scope_cnt;i++)
		{
			sprintf(tmp, "%s ", scope_tmp[i]);
			strcat(scope, tmp);
		}
	}
	sock = multi_init(&multi_addr);
	if(sock < 0)
		return -1;
	if( strlen(hello_response) > 1023 ){
		_TTY_LOG_ONVIF( "========== MULTICAST HELLO size error......\n");
	}
	strncpy(tmp_hello, hello_response, 1024 - 1);
	tmp_hello[1023] = 0;
	sprintf(addr, "%s://%s:%d/onvif/device_service", is_https?"https":"http", ip_addr, get_port());
	f1 = ezxml_parse_str(tmp_hello, strlen(tmp_hello));
	sub = ezxml_child(f1, "soap:Header");
	sub = ezxml_child(sub, "wsadis:MessageID");
	sub = ezxml_set_txt(sub, uuid_arr);

	sub = ezxml_child(f1, "soap:Header");
	sub = ezxml_child(sub, "wsadis:RelatesTo");
	sub = ezxml_set_txt(sub, uuid_arr);

	sub = ezxml_child(f1, "soap:Body");
	sub = ezxml_child(sub, "d:Hello");
	sub = ezxml_child(sub, "wsadis:EndpointReference");
	sub = ezxml_child(sub, "wsadis:Address");
	sub = ezxml_set_txt(sub, urn_arr);

	sub = ezxml_child(f1, "soap:Body");
	sub = ezxml_child(sub, "d:Hello");
	sub = ezxml_child(sub, "d:Scopes");
	sub = ezxml_set_txt(sub, scope);

	sub = ezxml_child(f1, "soap:Body");
	sub = ezxml_child(sub, "d:Hello");
	sub = ezxml_child(sub, "d:XAddrs");
	sub = ezxml_set_txt(sub, addr);
	onvif_hello = ezxml_toxml(f1);
	if(sendto(sock, onvif_hello, strlen(onvif_hello), 0,
				(struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d sendto failed [%s] \n", __FUNCTION__, __LINE__, strerror(errno));
		if( onvif_hello != NULL ) free(onvif_hello);
		ezxml_free(f1);		
		return -1;
	}
	_TTY_LOG_ONVIF( "========================== ......\n");
	_TTY_LOG_ONVIF( "========== MULTICAST HELLO o.k......\n");
	_TTY_LOG_ONVIF( "========================== ......\n");
	close(sock);
	if( onvif_hello != NULL ) free(onvif_hello);
	ezxml_free(f1);	
	return 0;
}

#define MAX_BYE_MESSAGE_SIZE 4096
void multi_bye()
{
	char onvif_bye[MAX_BYE_MESSAGE_SIZE] = {0, };
	int sock;
	struct sockaddr_in multi_addr;
	char addr[64] = {0, };
	char tmp[64] = {0, };
	char scope_tmp[MAX_SCOPE][COMMON_SIZE*2];
	char scope[1024] = {0, };
	char ip_addr[64] = {0, };
	//FILE *fp;
	int i;
	int scope_cnt = 0;
	char uuid_bye[64] = {0, };
	unsigned int is_https = is_https_required();


	int discovery_mode=0;

	discovery_mode = nf_sysdb_get_uint("onvif.common.discovery");

	if( discovery_mode == 0 ){
		return;
	}	

	memset(scope_tmp, 0x00, sizeof(scope_tmp));
	
	if (get_ipaddress(ip_addr) < 0)
	{
		_TTY_LOG_ONVIF( "%s function error\n", __FUNCTION__);
		return ;
	}
	get_scope(scope_tmp, &scope_cnt);
	if (scope_cnt > 0)
	{
		for (i = 0;i < scope_cnt;i++)
		{
			sprintf(tmp, "%s ", scope_tmp[i]);
			strcat(scope, tmp);
		}
	}

	sock = multi_init(&multi_addr);

	sprintf(uuid_bye, "uuid:13aa4766-3347-11df-b53f-%s", mac_addr);
	sprintf(addr, "%s://%s:%d/onvif/device_service", is_https ? "https" : "http", ip_addr, get_port());

	sprintf(onvif_bye, bye_templet, uuid_arr, urn_arr, scope, addr);
	//sleep(1);

	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}

	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}
	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}
	sleep(2);
	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}
	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}
	if (sendto(sock, onvif_bye, strlen(onvif_bye), 0, (struct sockaddr *)&multi_addr, sizeof(multi_addr)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d ========================== MULTICAST BYE failed......\n", __FUNCTION__, __LINE__);
		return ;
	}


	_TTY_LOG_ONVIF( "========================== ......\n");
	_TTY_LOG_ONVIF( "========== MULTICAST BYE o.k(size=%d)......\n", strlen(onvif_bye));
	_TTY_LOG_ONVIF( "========================== ......\n");

	close(sock);
}




static int multi_init(struct sockaddr_in *multi_addr)
{
	int sock;
	//int set = 1;
	int multi_TTL = TTL;
	char ip_addr[64]={0,};
#if 0
	struct sockaddr_in
	{	
		sa_family_t		sin_family;		// �ּ�ü��
		uint16_t		sin_port;		// 16��Ʈ TCP/UDP PORT��ȣ
		struct in_addr	sin_addr;		// 32��Ʈ IP�ּ�
		char			sin_zero[8];	// ������ ����
	};
	sockaddr_in����ü�� bind�Լ��� �ּ������� ������ �� ����Ѵ�.

		�׸��� in_addr�� ������ ������ ����.

		struct in_addr
		{
			in_addr_t		s_addr;			//32��Ʈ IPv4 ���ͳ� �ּ�
		};
#endif

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock == -1) {
		_TTY_LOG_ONVIF( "%s %d socket failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(get_ipaddress(ip_addr) < 0) {
		_TTY_LOG_ONVIF( "%s error\n", __FUNCTION__);
		return -1;
	}
	memset(multi_addr, 0x00, sizeof(struct sockaddr_in));
	multi_addr->sin_family = AF_INET;
	multi_addr->sin_addr.s_addr = inet_addr("239.255.255.250");
	multi_addr->sin_port = htons(ONVIF_MCAST_PORT_LISTEN);


	if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multi_TTL, sizeof(multi_TTL)) < 0)
	{
		_TTY_LOG_ONVIF( "%s %d setsockopt failed\n", __FUNCTION__, __LINE__);
		close(sock);
		return -1;
	}

	return sock;
}

#if 0
static int get_ipaddress(char *tmp_ip)
{
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct ifconf ifcfg;
	int fd;
	int n;
	int numreqs = 30;
	char *eth_str = nf_onvif_get_eth_str();

	if(!tmp_ip) {
		syslog(LOG_ERR, "%s arg tmp_ip is NULL\n", __FUNCTION__);
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0) {
		syslog(LOG_ERR, "%s socket error\n", __FUNCTION__);
		return -1;
	}

	memset(&ifcfg, 0, sizeof(ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc(ifcfg.ifc_len);
	if(!ifcfg.ifc_buf) {
		close(fd);
		syslog(LOG_ERR, "%s malloc error\n", __FUNCTION__);
		return -1;
	}

	if(ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0)
	{
		close(fd);
		syslog(LOG_ERR, "%s ioctol error\n", __FUNCTION__);
		free(ifcfg.ifc_buf);		
		return -1;
	}

	ifr = ifcfg.ifc_req;
	for(n=0;n<ifcfg.ifc_len;n+=sizeof(struct ifreq))
	{
        sin = (struct sockaddr_in *)&ifr->ifr_addr;
        if (strcmp(ifr->ifr_name, eth_str) == 0)
		{
			sin = (struct sockaddr_in *)&ifr->ifr_addr;
            // inet_ntoa is non-reentrant.
            //strcpy(tmp_ip, inet_ntoa(sin->sin_addr));
            inet_ntop(AF_INET, &sin->sin_addr, tmp_ip, sizeof(char)*16);
			break;
		}
		ifr++;
	}

	close(fd);
	
	free(ifcfg.ifc_buf);
	return 0;
}



/*
   static void get_ipaddress(char *tmp_ip)
   {
   struct ifreq *ifr;
   struct sockaddr_in *sin;
   struct ifconf ifcfg;
   int fd;
   int n;
   int numreqs = 30;
   fd = socket(AF_INET, SOCK_DGRAM, 0);

   memset(&ifcfg, 0, sizeof(ifcfg));
   ifcfg.ifc_buf = NULL;
   ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
   ifcfg.ifc_buf = malloc(ifcfg.ifc_len);
   if(ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0)
   {
   perror("SIOCGIFCONF");
   return;
   }

   ifr = ifcfg.ifc_req;
   for(n=0;n<ifcfg.ifc_len;n+=sizeof(struct ifreq))
   {
   if(!strcmp(ifr->ifr_name, "eth0"))
   {
   sin = (struct sockaddr_in *)&ifr->ifr_addr;
   strcpy(tmp_ip, inet_ntoa(sin->sin_addr));
   break;
   }
   ifr++;
   }
   close(fd);
   }
   */
#endif

static void get_macaddress(char *tmp_mac)
{
	int fd;
	struct ifreq ifbuf;
	unsigned char *hwaddr;
	char *eth_str = nf_onvif_get_eth_str();

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		_TTY_LOG_ONVIF( "%s socket failed\n", __FUNCTION__);
		return;
	}

	strcpy(ifbuf.ifr_name, eth_str);
	ioctl(fd, SIOCGIFHWADDR, &ifbuf);

	hwaddr = (unsigned char *)ifbuf.ifr_hwaddr.sa_data;

	sprintf(tmp_mac, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", hwaddr[0], hwaddr[1],
			hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

	close(fd);
}

static void get_scope(char (*scope)[COMMON_SIZE*2], int *scope_cnt)
{
	int cnt;
	char buff[COMMON_SIZE*2] = {0, };
	int i;

	cnt = (int)nf_sysdb_get_uint("onvif.common.scope_cnt");
    
	for(i = 0; i < cnt; i++)
	{
		if ( i == 3 )
		{
			snprintf(buff, COMMON_SIZE*2 - 1, "sys.info.model");
			if(!strcmp(nf_sysdb_get_str_nocopy("sys.info.model"), ""))
				snprintf(scope[i], sizeof(char)*128-1, "onvif://www.onvif.org/hardware/%s", MODELNAME);
			else
				snprintf(scope[i], sizeof(char)*128-1, "onvif://www.onvif.org/hardware/%s", nf_sysdb_get_str_nocopy(buff));
		}
		else if ( i == 4 )
		{
			snprintf(buff, COMMON_SIZE*2 - 1, "sys.info.model");
			if(!strcmp(nf_sysdb_get_str_nocopy("sys.info.model"), ""))
				snprintf(scope[i], sizeof(char)*128-1, "onvif://www.onvif.org/name/%s", MODELNAME);
			else
				snprintf(scope[i], sizeof(char)*128-1, "onvif://www.onvif.org/name/%s", nf_sysdb_get_str_nocopy(buff));

		}
		else
		{
			sprintf(buff, "onvif.scope%d.name", i);
			strncpy(scope[i], nf_sysdb_get_str_nocopy(buff), sizeof(char)*128-1);
		}
	}

	*scope_cnt = cnt;
}


static int scope_check(const char *ncx_scope, char *clnt_scope)
{
	//int i;
	//int match = false;
	char *token, *save = NULL;

	token = strtok_r(clnt_scope, " ", &save);
	while(token != NULL) {
    		usleep(100*1000);		
		if(strstr(ncx_scope, token))
			return true;
		token = strtok_r(NULL, " ", &save);
	}

	return false;
}

#if 0
static void Probe_Error_Send(char *send_buf, const char *probe_uuid)
{
	sprintf(send_buf, probe_error_response, probe_uuid);
}
#endif


static char soap_fault_msg[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
		<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
		xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
		xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
		xmlns:ns2=\"http://www.onvif.org/ver10/schema\" \
		xmlns:ns3=\"http://www.w3.org/2005/05/xmlmime\" \
		xmlns:ns5=\"http://www.w3.org/2005/08/addressing\" \
		xmlns:ns7=\"http://docs.oasis-open.org/wsn/t-1\" \
		xmlns:ns10=\"http://docs.oasis-open.org/wsrf/r-2\" \
		xmlns:ns6=\"http://docs.oasis-open.org/wsrf/bf-2\" \
		xmlns:ns16=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
		xmlns:ns17=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
		xmlns:ns11=\"http://www.onvif.org/ver10/media/wsdl\" \
		xmlns:ns12=\"http://www.onvif.org/ver10/ptz/wsdl\" \
		xmlns:ns13=\"http://www.onvif.org/ver10/device/wsdl\" \
		xmlns:ns14=\"http://www.onvif.org/ver10/imaging/wsdl\" \
		xmlns:ns19=\"http://www.onvif.org/ver10/analytics/wsdl/RuleEngineBinding\" \
		xmlns:ns20=\"http://www.onvif.org/ver10/analytics/wsdl/AnalyticsEngineBinding\" \
		xmlns:ns1=\"http://www.onvif.org/ver10/analytics/wsdl\" \
		xmlns:ns21=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\" \
		xmlns:ns22=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\" \
		xmlns:ns8=\"http://www.onvif.org/ver10/events/wsdl\" \
		xmlns:ns23=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" \
		xmlns:ns24=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\" \
		xmlns:ns25=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\" \
		xmlns:ns26=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\" \
		xmlns:ns27=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\" \
		xmlns:ns28=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\" \
		xmlns:ns4=\"http://docs.oasis-open.org/wsn/b-2\" \
		xmlns:ns29=\"http://www.onvif.org/ver10/network/wsdl/RemoteDiscoveryBinding\" \
		xmlns:ns30=\"http://www.onvif.org/ver10/network/wsdl/DiscoveryLookupBinding\" \
		xmlns:ns15=\"http://www.onvif.org/ver10/network/wsdl/\"> \
		<SOAP-ENV:Body><SOAP-ENV:Fault><SOAP-ENV:Code><SOAP-ENV:Value>SOAP-ENV:Sender</SOAP-ENV:Value><SOAP-ENV:Subcode><SOAP-ENV:Value>d:MatchingRuleNotSupported</SOAP-ENV:Value><SOAP-ENV:Subcode><SOAP-ENV:Value></SOAP-ENV:Value></SOAP-ENV:Subcode></SOAP-ENV:Subcode></SOAP-ENV:Code><SOAP-ENV:Reason><SOAP-ENV:Text xml:lang=\"en\"/></SOAP-ENV:Reason></SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>\r\n";
		
