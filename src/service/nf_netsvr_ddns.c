#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#include "nf_common.h"

#include <endian.h>

# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define htobe16(x) __bswap_16 (x)
#  define htole16(x) (x)
#  define be16toh(x) __bswap_16 (x)
#  define le16toh(x) (x)

#  define htobe32(x) __bswap_32 (x)
#  define htole32(x) (x)
#  define be32toh(x) __bswap_32 (x)
#  define le32toh(x) (x)

#  define htobe64(x) __bswap_64 (x)
#  define htole64(x) (x)
#  define be64toh(x) __bswap_64 (x)
#  define le64toh(x) (x)
# else
#  define htobe16(x) (x)
#  define htole16(x) __bswap_16 (x)
#  define be16toh(x) (x)
#  define le16toh(x) __bswap_16 (x)

#  define htobe32(x) (x)
#  define htole32(x) __bswap_32 (x)
#  define be32toh(x) (x)
#  define le32toh(x) __bswap_32 (x)

#  define htobe64(x) (x)
#  define htole64(x) __bswap_64 (x)
#  define be64toh(x) (x)
#  define le64toh(x) __bswap_64 (x)
# endif


#include "nf_sysdb.h"
#include "nf_network.h"
#include "unp.h"
#include "unpthread.h"
#include "ddns2_manager.h"
#include "nf_util_netif.h"

#include "nf_api_disk.h"

static gint _Link_Status = 0;		// for lan cable connect check
static gint ddns_disk_count = 1;
 
/** ********************************************************************* **
 ** defines
 ** ********************************************************************* **/
#define ENABLE_DDNS_SYSDB_RELOAD
#define ENABLE_DDNS_STATUS_GET_IP
//#define ENABLE_YOICS

#define DDNS_REFRESH_SLEEP_SEC	7

#define DDNS_TCP_CONNECT_TIMEOUT	(4500000)

#define UDEFAULTIP					0x7f000001	//127.0.0.1
#define UDEFAULTGWIP				0x7f000001	//127.0.0.1
#define UDEFAULTMASK				0xffffff00	//255.255.255.0

#define DDNS_PROTOCOL_VERSION		1
#define INTELLIX_DDNS_PORT			918
#define DEFAULT_CLASSID				20001	//IDVR400T
#define DEFAULT_ACTION				0x0001	//update
	
//DDNS Error No.
#define EOK							0
#define EINVDDNSREQ					1
#define ENEEDRETRY					2
#define ECANNOTFINDHOST				3

#define DYNDNS_DDNS_PORT			8245		
#define FUJIKO_DDNS_PORT			80

#define DNS_ERR_MAX					20
#define GETIP_ERR_MAX				12 // 60�� 

#define UPDATE_ERR_MAX				6  // 30��
#define DDNS_UPDATE_TIME			(60*60*6)

#define DDNS_FORCE_UPDATE_MAX		50

#define S1_IP_SVR 			"203.254.192.21"
#define S1_DDNS_SVR			"ddns.s1.co.kr"
#define S1_DDNS_PORT 		9003
#define S1_CONNECT_IP		0
#define S1_CONNECT_DOMAIN	1

#define S1_DDNS_SERVER_IDX  3

/** ********************************************************************* **
 ** debuging
 ** ********************************************************************* **/
#define _DEBUG_NET
#define _DEBUG_NET_DDNS
//#define _DEBUG_FAST_TIME_NET
/** ********************************************************************* **
 ** typedefs
 ** ********************************************************************* **/
typedef struct _ddns_req_t
{
  unsigned char mac[6];
  unsigned short version;
  unsigned short action_type;
  unsigned short class_id;
  char reserved1[52];
} ddns_req_t;
#pragma STRUCT_ALIGN (ddns_req_t,4);

typedef struct _ddns_res_t
{
  time_t time;
  unsigned int reserved1;
} ddns_res_t;
#pragma STRUCT_ALIGN (ddns_res_t,4);

static void sleep_ex(long usec);
void make_ddns_req(void);
void convert_hexstr_to_hexint(char *src, char *des, int size);
char *my_str_trim(char *str);

int connect_test(const SA *saptr, socklen_t salen, int usec);
extern    int nf_upnp_port_reforwording(int int_port, int desc_type);

static gint _ddns_refresh = 0;
static NF_DDNS_STATUS _ddns_status;
static GStaticMutex _ddns_status_lock = G_STATIC_MUTEX_INIT;

static int _ddns_make_hostaddr( char *buff, int buff_len);

int nf_get_external_ip(char *external_ip);
int dau_ddns_force_register(void);

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void sleep_ex(long usec)
{
	struct timespec req, rem;

	if (usec < 0)
		return;

	req.tv_sec  = (usec / 1000000);
	req.tv_nsec = (usec % 1000000) * 1000;

__do_sleep__:
	if (nanosleep(&req, &rem) < 0) {
		if (errno == EINTR) {
			req = rem;
			goto __do_sleep__;
		}
	}

	return;
}


/*****************************************************************************
 * ����	  : �Է¹��� ��Ʈ���� trim�Ѵ�.
 * Prototype : char *my_str_trim(char *str) 
 * Arguments : char *str : trim�� ���ڿ�
 * Return	: char *str
 ****************************************************************************/
char *my_str_trim(char *str) 
{
  int i, j;

  if(!str) return NULL;

  for(j = 0; str[j] == ' ' || str[j] == 9 || str[j] == '\r' || str[j] == '\n'; j++);
  for(i = 0; str[j] != '\0'; i++, j++) str[i] = str[j];
  for(i--; (i >= 0) && (str[i] == ' ' || str[i] == 9 || str[i] == '\r' || str[i] == '\n'); i--);
  str[i+1] = '\0';

  return str;
} 


#define DDNS_BUF_LEN	4096

//////////////////////////////////////////////////////////////////////////////////////
static char ddns_base64_table[] =
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
	};
static char ddns_base64_pad = '=';
 
unsigned char *ddns_base64_encode(unsigned char *str, int length, int *ret_length) {
	const unsigned char *current = str;
	int i = 0;
	unsigned char *result = (unsigned char *)g_malloc0( ((length + 3 - length % 3) * 4 / 3 + 1) * sizeof(char) );
	
	if(result == NULL)
	{		
		g_warning("%s g_malloc failed", __FUNCTION__);
		return NULL;
	}
	
	while (length > 2) { /* keep going until we have less than 24 bits */
		result[i++] = ddns_base64_table[current[0] >> 2];
		result[i++] = ddns_base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
		result[i++] = ddns_base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
		result[i++] = ddns_base64_table[current[2] & 0x3f];

		current += 3;
		length -= 3; /* we just handle 3 octets of data */
	}

	/* now deal with the tail end of things */
	if (length != 0) {
		result[i++] = ddns_base64_table[current[0] >> 2];
		if (length > 1) {
			result[i++] = ddns_base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
			result[i++] = ddns_base64_table[(current[1] & 0x0f) << 2];
			result[i++] = ddns_base64_pad;
		}
		else {
			result[i++] = ddns_base64_table[(current[0] & 0x03) << 4];
			result[i++] = ddns_base64_pad;
			result[i++] = ddns_base64_pad;
		}
	}
	if(ret_length) {
		*ret_length = i;
	}
	result[i] =  '\0';
	return result;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int nf_get_ddns_disk_count(void)
{
	return ddns_disk_count;
}

void nf_set_ddns_disk_count(int disk_count)
{
	ddns_disk_count = disk_count;
}



int nf_ddns_get_status(void)
{
	return _ddns_status.status;
}

void ddns_set_status(int val)
{
	g_message("DDNS - %s - set:%d", __FUNCTION__, val);
	
	g_printf("### ddns_set_status :bef[%d] current val[%d]\n",_ddns_status.status,val);
	if(val == DDNS2_RES_OK_GOOD || val == 1)
	{
		nf_notify_fire_params("net_ddns_status", 0, 0, 0, 0);
		_ddns_status.status = 1;	// ok
	}
	else
	{								// error
		nf_notify_fire_params("net_ddns_status", 1, val, 0, 0);
		_ddns_status.status = val;
	}

}



static int _dyndns_write_data(const int s, 
			const char* id_pw, 
			const char* host_name, 
			const char* domain_ext, 
			const char* server,
			const char* cur_ip)
{
	char msg[DDNS_BUF_LEN];
	int ret = 0, len = 0;
	

	memset(msg, 0x00, sizeof(msg));


/*
GET /nic/update?hostname=yourhostname&myip=ipaddress&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG HTTP/1.0 
Host: members.dyndns.org 
Authorization: Basic base-64-authorization 
User-Agent: Company - Device - Version Number
*/
#if 0
	snprintf(msg, sizeof(msg), 
		"GET /nic/update?system=dyndns&hostname=%s.%s&wildcard=OFF&mx=NOCHG&backmx=NO&offline=NO HTTP/1.0\r\n" //No-use IP
		"Host: %s\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: myclient/1.0 me@null.net\r\n\r\n",
		host_name, domain_ext,
		server, id_pw );
#else
	snprintf(msg, sizeof(msg), 
		"GET /nic/update?hostname=%s.%s&myip=%s&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG HTTP/1.0\r\n" //No-use IP
		"Host: %s\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: myclient/1.0 me@null.net\r\n\r\n",
		host_name, domain_ext,cur_ip, server, id_pw );
#endif

	len = strnlen(msg, sizeof(msg) );

	g_message("%s write len[%d] dump[%s]",__FUNCTION__, len, msg);
	if( (ret=Writen(s, msg, len)) == len )
		return 1;
	else
		return 0;
}

static int _fujikodns_write_data(const int s, 
			const char* id_pw, 
			const char* host_name, 
			const char* domain_ext, 
			const char* server,
			const char* cur_ip)
{
	char msg[DDNS_BUF_LEN];
	int ret = 0, len = 0;

	memset(msg, 0x00, sizeof(msg));

	snprintf(msg, sizeof(msg),
		"GET /nic/update?hostname=%s.%s&myip=%s HTTP/1.0\r\n"
		"Host: %s\r\n"
		"Authorization: basic %s\r\n\r\n",
		host_name, domain_ext,cur_ip, server, id_pw );

	len = strnlen(msg, sizeof(msg) );

	g_message("%s write len[%d] dump[%s]",__FUNCTION__, len, msg);
	if( (ret=Writen(s, msg, len)) == len )
		return 1;
	else
		return 0;
}

static int _lorex_write_data(const int s, 
			const char* id_pw, 
			const char* domain_name, 
			const char* domain_ext, 
			const char* server)
{
	char msg[DDNS_BUF_LEN];
	int ret = 0, len = 0;
	

	memset(msg, 0x00, sizeof(msg));
	
	snprintf(msg, sizeof(msg), 
		"GET /nic/update?system=dyndns&hostname=%s.%s&wildcard=OFF&mx=NOCHG&backmx=NO&offline=NO HTTP/1.0\r\n" //No-use IP
		"Host: %s\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: myclient/1.0 me@null.net\r\n\r\n",
		domain_name, domain_ext,
		server, id_pw );

	len = strnlen(msg, sizeof(msg) );

	g_message("%s write len[%d] dump[%s]",__FUNCTION__, len, msg);
	if( (ret=Writen(s, msg, len)) == len )
		return 1;
	else
		return 0;
}

static const char *_lorex_ret_msg[] =
{
	"good",		// the updates successful
	"norchg",	// the updates chagnbed no setting, and is considered abusive
	
	"badauth",	// usernane and password specified are incorrect
	"badsys",	// parameter given is not valid
	"badagent",	// user agent blocked 

	"notfqdn",	// the hostname specified is not a fully-qualified domain name
	"nohost",	// the hostname specified does not exist
	"numhost",	// too many or too few hosts found
	"abuse",	// the hostname specified is blocked for update abuse

	"dnserr",	// DNS error
	"911",		// ddns internal error 
	NULL	
};

typedef enum _DYNDNS_RET_MSG {
	DYNDNS_RES_OK_GOOD = 0x01,
	DYNDNS_RES_NOCHG,
	
	DYNDNS_RES_ABUSE_ERR ,
	DYNDNS_RES_BADAGENT_ERR ,
	DYNDNS_RES_BADAUTH_ERR ,	
	DYNDNS_RES_BADSYS_ERR ,
	
	DYNDNS_RES_DNSERR_ERR ,
	DYNDNS_RES_DONATOR_ERR ,
	
	DYNDNS_RES_NOHOST_ERR ,
	DYNDNS_RES_NOTFQDN_ERR ,
	DYNDNS_RES_NUMHOST_ERR ,
	
	DYNDNS_RES_YOURS_ERR ,
	DYNDNS_RES_911_ERR ,
	
	DYNDNS_RES_MAX,
}DYNDNS_RET_MSG;

static const char *_dyndns_ret_msg[] =
{
	"asm_test",
	
	"good",		// the updates successful
	"nochg",	// the updates chagnbed no setting, and is considered abusive

	"abuse",	// the hostname specified is blocked for update abuse
	"badagent",	// user agent blocked 
	"badauth",	// usernane and password specified are incorrect
	"badsys",	// parameter given is not valid

	"dnserr",	// DNS error
	"!donator", // paid account feature

	"nohost",	// the hostname specified does not exist
	"notfqdn",	// the hostname specified is not a fully-qualified domain name
	"numhost",	// too many or too few hosts found

	"!yours",   // host not in this account
	"911",		// ddns internal error 

	"max_err_code"	
};

typedef enum _FUJIKODNS_RET_MSG {
	FUJIKODNS_RES_OK_GOOD = 21,
	FUJIKODNS_RES_NOCHG,
	FUJIKODNS_RES_BADAUTH_ERR,
	FUJIKODNS_RES_MAX,
}FUJIKODNS_RET_MSG;

static const char *_fujikodns_ret_msg[] =
{
	"good",		// the updates successful
	"nochg",	// the updates chagnbed no setting, and is considered abusive
	"badauth",	// usernane and password specified are incorrect
	"max_err_code"	
};

static int _dyndns_read_data(const int s)
{
	char msg[DDNS_BUF_LEN], *ptr1=0, *ptr2=0;
	int ret = 0;
	int i;
	
	memset(msg, 0x00, sizeof(msg));
		
	if( Readable_timeo(s, 5) <= 0) // 5 sec timeout
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
	
	if( (ret = read(s, msg, sizeof(msg)-1 )) < 0 )
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;

	if(msg != NULL)
		g_message("%s read len[%d] dump[%s]",__FUNCTION__, ret, msg);	

	if( (ptr1 = strstr(msg, "HTTP/1.1 200 OK")) )
	{
    	ptr1 += 17;	// strlen("HTTP/1.1 200 OK") + "\r\n" 

		for(i = 1; i < DYNDNS_RES_MAX; i++)
		{
			if ((strstr(ptr1, _dyndns_ret_msg[i])))
			{
				return i;	// succcess
			}
		}
	}

	return 0; // failed;
} 

static int _fujikodns_read_data(const int s)
{
	char msg[DDNS_BUF_LEN], *ptr1=0, *ptr2=0;
	int ret = 0;
	int i;
	
	memset(msg, 0x00, sizeof(msg));
		
	if( Readable_timeo(s, 5) <= 0) // 5 sec timeout
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
	
	if( (ret = read(s, msg, sizeof(msg)-1 )) < 0 )
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;

	if(msg != NULL)
		g_message("%s read len[%d] dump[%s]",__FUNCTION__, ret, msg);	

	if( (ptr1 = strstr(msg, "HTTP/1.1 200 OK")) )
	{
    	ptr1 += 17;	// strlen("HTTP/1.1 200 OK") + "\r\n" 

		for(i = 0; i < FUJIKODNS_RES_MAX - FUJIKODNS_RES_OK_GOOD; i++)
		{
			if ((strstr(ptr1, _fujikodns_ret_msg[i])))
			{
				i += FUJIKODNS_RES_OK_GOOD;
				return i;	// succcess
			}
		}
	}

	return 0; // failed;
} 


int dyn_ddns_force_register(char *tmp_host, char *tmp_id, char *tmp_pass)
{
	int		sock=-1, ret_code  = 0, ret;	
	char	*mac = nf_sysdb_get_str_nocopy("sys.info.mac");
	unsigned int port = nf_sysdb_get_uint("net.proto.webport");
	char tmp_id_pass[256] = {0,};
	char domain_ext[256] = {0,};
	char ddns_server[256] = {0,};
	char cur_ip[256] = {0,};
	
	char *idpw_base64 = NULL;
	int id_enc_len;
	
	ret = nf_ddns_dyn_get_myip(cur_ip);
	if(ret != 0)
	{
		ddns_set_status(ret);
		return ret;
	}
	
	strcpy(ddns_server,"members.dyndns.org");				
	sock = connect_timeout_hostname( ddns_server, DYNDNS_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s dns query fail[%s] sock[%d]", __FUNCTION__, ddns_server, sock);
		ddns_set_status(DDNS2_RES_SOCKET_CREATE_ERR);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		g_message("%s dns connect fail[%s] sock[%d]", __FUNCTION__, ddns_server, sock);
		ddns_set_status(DDNS2_RES_SOCKET_CREATE_ERR);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}

	strcpy(domain_ext,"dyndns.org");
	snprintf(tmp_id_pass, sizeof(tmp_id_pass)-1, "%s:%s", tmp_id, tmp_pass);	
	
	idpw_base64 = ddns_base64_encode(tmp_id_pass, strlen(tmp_id_pass), &id_enc_len);
		
	/* update */
	if( _dyndns_write_data(sock, idpw_base64, tmp_host, domain_ext, ddns_server, cur_ip) )
	{
		ret = _dyndns_read_data(sock);		
		g_message("%s : _dyndns_read_data ret[%d]\n", __FUNCTION__, ret);
		if(ret == DYNDNS_RES_OK_GOOD || ret == DYNDNS_RES_NOCHG) 
		{ // succcess
			ret_code = DYNDNS_RES_OK_GOOD;
		}
		else
		{
			ret_code = ret;	      
		}
				
	}else{
		g_message("%s : _dyndns_write_data failed", __FUNCTION__);
		ret_code = DDNS2_RES_DATA_SEND_ERR;
	}

	loop_out:

	if(idpw_base64) 
		free(idpw_base64);

	/* socket close */
	Close(sock);	
	ddns_set_status(ret_code);	
	return ret_code;
} 

int fujiko_ddns_force_register(char *tmp_host, char *tmp_id, char *tmp_pass)
{
	int	 sock=-1, ret_code  = 0, ret;
	char tmp_id_pass[256] = {0,};
	char domain_ext[256] = {0,};
	char ddns_server[256] = {0,};
	char cur_ip[256] = {0,};
	
	char *idpw_base64 = NULL;
	int id_enc_len;

	ret = nf_ddns_dyn_get_myip(cur_ip);
	if(ret !=0)
		ret = nf_ddns_itx_get_myip(cur_ip);

	if(ret != 0)
	{
		ddns_set_status(ret);
		return ret;
	}
	
	strcpy(ddns_server,"ddns.fujiko.biz");				
	sock = connect_timeout_hostname( ddns_server, FUJIKO_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s dns query fail[%s] sock[%d]", __FUNCTION__, ddns_server, sock);
		ddns_set_status(DDNS2_RES_SOCKET_CREATE_ERR);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		g_message("%s dns connect fail[%s] sock[%d]", __FUNCTION__, ddns_server, sock);
		ddns_set_status(DDNS2_RES_SOCKET_CREATE_ERR);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}

	strcpy(domain_ext,"fujiko.biz");
	snprintf(tmp_id_pass, sizeof(tmp_id_pass)-1, "%s:%s", tmp_id, tmp_pass);	
	
	idpw_base64 = ddns_base64_encode(tmp_id_pass, strlen(tmp_id_pass), &id_enc_len);

	/* update */
	if( _fujikodns_write_data(sock, idpw_base64, tmp_host, domain_ext, ddns_server, cur_ip) )
	{
		ret = _fujikodns_read_data(sock);
		g_message("%s : _fujikodns_read_data ret[%d]\n", __FUNCTION__, ret);
		if(ret == FUJIKODNS_RES_OK_GOOD || ret == FUJIKODNS_RES_NOCHG) 
		{ // succcess
			ret_code = 1;
		}
		else
		{
    		ret_code = ret;
		}				
	}
	else
	{	
		g_message("%s : _fujikodns_write_data failed", __FUNCTION__);
		ret_code = DDNS2_RES_DATA_SEND_ERR;
	}

	if(idpw_base64) 
		free(idpw_base64);

	/* socket close */
	Close(sock);	
	ddns_set_status(ret_code);	
	return ret_code;
} 

static int _dyndns_send_ddns_packet(const char *ddns_server, const char *cur_ip)
{
	int		sock=-1, ret_code  = 0, ret;	
	char	*mac = nf_sysdb_get_str_nocopy("sys.info.mac");
	unsigned int port = nf_sysdb_get_uint("net.proto.webport");
	char tmp_host[256] = {0,};
	char tmp_id[256] = {0,};
	char tmp_pass[256] = {0,};
	char tmp_id_pass[256] = {0,};
	char domain_ext[256] = {0,};
	
	char *idpw_base64 = NULL;
	int id_enc_len;

	sock = connect_timeout_hostname( ddns_server, DYNDNS_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s  dns query fail[%s] ret_code[%d]", __FUNCTION__, ddns_server, ret_code);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	
#if 1 // for normal
	snprintf(tmp_id, sizeof(tmp_id)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.username") );
	snprintf(tmp_pass, sizeof(tmp_pass)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.passwd"));
	snprintf(tmp_host,  sizeof(tmp_host)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.hostname"));
								
	my_str_trim(tmp_id);
	my_str_trim(tmp_pass);
	my_str_trim(tmp_host);

	strcpy(domain_ext,"dyndns.org");
	snprintf(tmp_id_pass, sizeof(tmp_id_pass)-1, "%s:%s", tmp_id, tmp_pass);	
#else	// for test
	strcpy(tmp_host,"itxssw");  // todo_test
	strcpy(tmp_id,"asm77");
	strcpy(tmp_pass,"gener77");
	strcpy(domain_ext,"dyndns.org");
	snprintf(tmp_id_pass, sizeof(tmp_id_pass)-1, "%s:%s", tmp_id, tmp_pass);	
#endif

	idpw_base64 = ddns_base64_encode(tmp_id_pass, strlen(tmp_id_pass), &id_enc_len);

	/* update */
	if( _dyndns_write_data(sock, idpw_base64, tmp_host, domain_ext, ddns_server, cur_ip) )
	{
		ret = _dyndns_read_data(sock);
		if(ret == DYNDNS_RES_OK_GOOD || ret == DYNDNS_RES_NOCHG) 
		{ // succcess
			ret_code = DYNDNS_RES_OK_GOOD;
		}
		else
		{
			ret_code = ret;	      
		}	
	}
	else
	{
		ret_code = DDNS2_RES_DATA_SEND_ERR;
	}

	loop_out:

	if(idpw_base64) 
		free(idpw_base64);

	/* socket close */
	Close(sock);
	ddns_set_status(ret_code);
	return ret_code;
} 

static int _fujikodns_send_ddns_packet(const char *ddns_server, const char *cur_ip)
{
	int	 sock=-1, ret_code  = 0, ret;
	char tmp_host[256] = {0,};
	char tmp_id[256] = {0,};
	char tmp_pass[256] = {0,};
	char tmp_id_pass[256] = {0,};
	char domain_ext[256] = {0,};
	
	char *idpw_base64 = NULL;
	int id_enc_len;
			
	sock = connect_timeout_hostname( ddns_server, FUJIKO_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s  dns query fail[%s] ret_code[%d]", __FUNCTION__, ddns_server, ret_code);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}

	snprintf(tmp_id, sizeof(tmp_id)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.username") );
	snprintf(tmp_pass, sizeof(tmp_pass)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.passwd"));
	snprintf(tmp_host,  sizeof(tmp_host)-1, "%s", nf_sysdb_get_str_nocopy("net.ddns.hostname"));
								
	my_str_trim(tmp_id);
	my_str_trim(tmp_pass);
	my_str_trim(tmp_host);

	strcpy(domain_ext,"fujiko.biz");
	snprintf(tmp_id_pass, sizeof(tmp_id_pass)-1, "%s:%s", tmp_id, tmp_pass);	

	idpw_base64 = ddns_base64_encode(tmp_id_pass, strlen(tmp_id_pass), &id_enc_len);

	/* update */
	if( _fujikodns_write_data(sock, idpw_base64, tmp_host, domain_ext, ddns_server, cur_ip) )
	{
		ret = _fujikodns_read_data(sock); 
		g_message("%s : _fujikodns_read_data ret[%d]\n", __FUNCTION__, ret);
		if(ret == FUJIKODNS_RES_OK_GOOD || ret == FUJIKODNS_RES_NOCHG) 
		{ // succcess
			ret_code = 1;
		}
		else
		{
			ret_code = ret;	      
		}	
	}
	else
	{
		ret_code = DDNS2_RES_DATA_SEND_ERR;
	}

	if(idpw_base64) 
		free(idpw_base64);

	/* socket close */
	Close(sock);
	ddns_set_status(ret_code);
	return ret_code;
} 

static gboolean _s1_is_connected_eth(void)
{
	gchar  tmp[256];
	gchar  buff[256];	
	struct addrinfo	hints, *res_hints = NULL;
	gboolean ret = FALSE;
	FILE *fp;
	
	bzero(&hints, sizeof(struct addrinfo));

	hints.ai_family =  AF_INET; // AF_UNSPEC;  AF_INET,AF_INET6
	hints.ai_socktype = SOCK_STREAM;

	if ( getaddrinfo(S1_DDNS_SVR, NULL, &hints, &res_hints) == 0 )
	{
		freeaddrinfo(res_hints);
		
		return TRUE;
	}	

	snprintf( tmp, sizeof(tmp), "ping %s -c 1 -W 1 | grep \"1 packets received\" | wc -l >& /tmp/wan_ping.txt", S1_IP_SVR);

	proxy_system(tmp, 1, 3);
	fp = fopen( "/tmp/wan_ping.txt", "r");
	if( fp != NULL)
	{
		if( fgets( buff, sizeof(buff), fp) )
		{
			if ( strchr( buff, '1') != NULL )
				ret = TRUE;
		}
		
		fclose( fp);
	}
	
	return ret;
}

gboolean _is_ddns_on(void)
{
	gboolean ret = FALSE;
	ret = nf_sysdb_get_bool("net.proto.ddnson");
	return ret;
}

int _get_extern_ip(char *extern_ip)
{
	int ret = 0;
	
	ret = nf_ddns_itx_get_myip(extern_ip);
	if(ret != 0)
	{
		ret = nf_ddns_dyn_get_myip(extern_ip);
	}

	if(ret != 0)
	{
		return -1;
	}
	else
	{
		return 0;	
	}		
}

#if defined(ENABLE_S1_DDNS_SCENARIO)
int _s1_send_ddns_packet(void)
{
	int	sock=-1, ret = 0;	
	int web_port=0;
	int rtsp_port=0;
	unsigned char mac[64]={0,};
	static int connect_type = S1_CONNECT_IP;
	char *ddns_svr = NULL;
	
	strcpy(mac, nf_sysdb_get_str_nocopy("sys.info.mac"));
	web_port = nf_sysdb_get_uint("net.proto.webport");	
	rtsp_port = nf_sysdb_get_uint("net.rtp.rtspport");

	if(connect_type == S1_CONNECT_IP)
		ddns_svr = S1_IP_SVR;
	else if(connect_type == S1_CONNECT_DOMAIN)
		ddns_svr = S1_DDNS_SVR;

	g_message("S1 DDNS SEND PACKET");

	sock = connect_timeout_hostname( ddns_svr, S1_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock < 0) {
		fprintf(stderr, "%s connect_timeout_hostname failed\n", __FUNCTION__);

		if(connect_type == S1_CONNECT_IP) 
			connect_type = S1_CONNECT_DOMAIN;
		else if(connect_type == S1_CONNECT_DOMAIN)
			connect_type = S1_CONNECT_IP;

//		ddns_set_status(DDNS2_RES_SERVER_CONNECT_ERR); 		
		return DDNS2_RES_SERVER_CONNECT_ERR; //-1;
	}

	if(_s1_register_write_data(sock, web_port, rtsp_port, mac) < 0) 
	{
		g_warning("%s write_data error\n", __FUNCTION__);
		ret = DDNS2_RES_DATA_SEND_ERR; //-1;
		goto ddns_ret;
	}

	if( Readable_timeo(sock, 5) <= 0) // 5sec timeout
	{
		g_warning("%s read timeout host\n", __FUNCTION__);
		ret = DDNS2_RES_RECV_ERR; //-1;	
		goto ddns_ret;
	}

	if(_s1_register_read_data(sock) < 0)
	{
		g_warning("%s read_data error\n", __FUNCTION__);
		ret = DDNS2_RES_RECV_ERR; //-1;
		goto ddns_ret;
	}

	ret = 1;

ddns_ret:
	
	/* socket close */
	Close(sock);
//	ddns_set_status(ret);
	return ret;
} 

int s1_ddns_force_register(void) {
	int ret;

	if( _s1_is_connected_eth() )
	{
		int ext_ret;
		char extern_ip[256];
		
		ext_ret = nf_get_external_ip(extern_ip);
		
		if( ext_ret == 0 )
		{
			gboolean send_ret;
//			send_ret = s1_send_ddns_command(60, 5);
			send_ret = _s1_send_ddns_packet();
			
			if( send_ret == 1 )
				ret = 1;
			else
				ret = S1_DDNS_ERR_UPDATE;
		}
		else
		{
			ret = S1_DDNS_ERR_EXTERN_IP;
		}		
	}
	else
	{
		ret = S1_DDNS_ERR_NETORK;
	}

	ddns_set_status(ret);

	return ret;
}

#else
int _s1_send_ddns_packet(void)
{
	int	sock=-1, ret = 0;	
	int web_port=0;
	int rtsp_port=0;
	unsigned char mac[64]={0,};
	static int connect_type = S1_CONNECT_IP;
	char *ddns_svr = NULL;
	
	strcpy(mac, nf_sysdb_get_str_nocopy("sys.info.mac"));
	web_port = nf_sysdb_get_uint("net.proto.webport");	
	rtsp_port = nf_sysdb_get_uint("net.rtp.rtspport");

	if(connect_type == S1_CONNECT_IP)
		ddns_svr = S1_IP_SVR;
	else if(connect_type == S1_CONNECT_DOMAIN)
		ddns_svr = S1_DDNS_SVR;

	sock = connect_timeout_hostname( ddns_svr, S1_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock < 0) {
		fprintf(stderr, "%s connect_timeout_hostname failed\n", __FUNCTION__);

		if(connect_type == S1_CONNECT_IP) 
			connect_type = S1_CONNECT_DOMAIN;
		else if(connect_type == S1_CONNECT_DOMAIN)
			connect_type = S1_CONNECT_IP;

		ddns_set_status(DDNS2_RES_SERVER_CONNECT_ERR); 		
		return DDNS2_RES_SERVER_CONNECT_ERR; //-1;
	}

	if(_s1_register_write_data(sock, web_port, rtsp_port, mac) < 0) 
	{
		g_warning("%s write_data error\n", __FUNCTION__);
		ret = DDNS2_RES_DATA_SEND_ERR; //-1;
		goto ddns_ret;
	}

	if( Readable_timeo(sock, 5) <= 0) // 5sec timeout
	{
		g_warning("%s read timeout host\n", __FUNCTION__);
		ret = DDNS2_RES_RECV_ERR; //-1;	
		goto ddns_ret;
	}

	if(_s1_register_read_data(sock) < 0)
	{
		g_warning("%s read_data error\n", __FUNCTION__);
		ret = DDNS2_RES_RECV_ERR; //-1;
		goto ddns_ret;
	}

	ret = 1;

ddns_ret:
	
	Close(sock);
	ddns_set_status(ret);
	return ret;
} 

int s1_ddns_force_register(void) {	
	return _s1_send_ddns_packet();	
}
#endif

static void my_inet_ntoa( gchar *buff, guint buff_len, guint addr)
{
	struct sockaddr_in in_addr;	
	
	g_return_if_fail( buff );
	
	in_addr.sin_addr.s_addr = addr;	
	snprintf(buff, buff_len, inet_ntoa( in_addr.sin_addr ) );
		
}

int _s1_query_ddns_packet( char *str_ipaddr, int str_len )
{
	int	sock=-1, ret = 0;	
	int web_port=0;
	unsigned char mac[64]={0,};
	unsigned int ipv4_addr = 0;
	char tmp[128];
	
		
	static int connect_type = S1_CONNECT_IP;
	char *ddns_svr = NULL;
	
	strcpy(mac, nf_sysdb_get_str_nocopy("sys.info.mac"));
	web_port = nf_sysdb_get_uint("net.proto.webport");

	if(connect_type == S1_CONNECT_IP)
		ddns_svr = S1_IP_SVR;
	else if(connect_type == S1_CONNECT_DOMAIN)
		ddns_svr = S1_DDNS_SVR;

	sock = connect_timeout_hostname( ddns_svr, S1_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock < 0) {
		fprintf(stderr, "%s connect_timeout_hostname failed\n", __FUNCTION__);

		if(connect_type == S1_CONNECT_IP) 
			connect_type = S1_CONNECT_DOMAIN;
		else if(connect_type == S1_CONNECT_DOMAIN)
			connect_type = S1_CONNECT_IP;

		return -1;
	}

	if(_s1_query_write_data(sock, mac) < 0) 
	{
		g_warning("%s write_data error\n", __FUNCTION__);
		ret = -1;
		goto ddns_ret;
	}

	if( Readable_timeo(sock, 5) <= 0) // 3sec timeout
	{
		g_warning("%s read timeout host\n", __FUNCTION__);
		ret = -1;	
		goto ddns_ret;
	}

	if(_s1_query_read_data(sock, &ipv4_addr) < 0)
	{
		g_warning("%s read_data error\n", __FUNCTION__);
		ret = -1;
		goto ddns_ret;
	}

	//my_inet_ntoa( tmp, sizeof(tmp), ntohl(ipv4_addr));
	//g_message("%s addr  [0x%08x][%s]", __FUNCTION__, ntohl(ipv4_addr), tmp);
	
	my_inet_ntoa( str_ipaddr, str_len, ipv4_addr);
	g_message("%s addr  [0x%08x][%s]", __FUNCTION__, ipv4_addr, str_ipaddr);
		
	ret = 1;

ddns_ret:
				
	/* socket close */
	Close(sock);
	return ret;
} 


int _s1_query_ddns_packet_for_test( char *str_ipaddr, int str_len, char *mac_addr)
{
	int	sock=-1, ret = 0;	
	int web_port=0;
	unsigned char mac[64]={0,};
	unsigned int ipv4_addr = 0;
	char tmp[128];
	
		
	static int connect_type = S1_CONNECT_IP;
	char *ddns_svr = NULL;
		
	strncpy(mac, mac_addr, sizeof(mac)-1 );	

	if(connect_type == S1_CONNECT_IP)
		ddns_svr = S1_IP_SVR;
	else if(connect_type == S1_CONNECT_DOMAIN)
		ddns_svr = S1_DDNS_SVR;

	sock = connect_timeout_hostname( ddns_svr, S1_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock < 0) {
		fprintf(stderr, "%s connect_timeout_hostname failed\n", __FUNCTION__);

		if(connect_type == S1_CONNECT_IP) 
			connect_type = S1_CONNECT_DOMAIN;
		else if(connect_type == S1_CONNECT_DOMAIN)
			connect_type = S1_CONNECT_IP;

		return -1;
	}

	if(_s1_query_write_data(sock, mac) < 0) 
	{
		g_warning("%s write_data error\n", __FUNCTION__);
		ret = -1;
		goto ddns_ret;
	}

	if( Readable_timeo(sock, 5) <= 0) // 3sec timeout
	{
		g_warning("%s read timeout host\n", __FUNCTION__);
		ret = -1;	
		goto ddns_ret;
	}

	if(_s1_query_read_data(sock, &ipv4_addr) < 0)
	{
		g_warning("%s read_data error\n", __FUNCTION__);
		ret = -1;
		goto ddns_ret;
	}

	//my_inet_ntoa( tmp, sizeof(tmp), ntohl(ipv4_addr));
	//g_message("%s addr  [0x%08x][%s]", __FUNCTION__, ntohl(ipv4_addr), tmp);
	
	my_inet_ntoa( str_ipaddr, str_len, ipv4_addr);
	g_message("%s addr  [0x%08x][%s]", __FUNCTION__, ipv4_addr, str_ipaddr);
		
	ret = 1;

ddns_ret:
				
	/* socket close */
	Close(sock);
	return ret;
} 


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int connect_test(const SA *saptr, socklen_t salen, int usec)
{
	int				flags, n, ret, try;
	int				sockfd;
	fd_set 			wset;
	struct timeval tval;

	// socket create		
	sockfd = Socket(AF_INET, SOCK_STREAM, 0); 
	if(sockfd < 0)
		return	(-1);
		
	flags = Fcntl(sockfd, F_GETFL, 0);
	Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	// ./main 00115fff3031.dvrlink.net 9900
	
	if(usec > 250000) {	// 0.250 sec		
		tval.tv_sec = usec/1000000;
		tval.tv_usec = usec%1000000;		
	} else{
		tval.tv_sec = 0;
		tval.tv_usec = 250000;
	}
		
	if ( (n = connect(sockfd, (struct sockaddr *)saptr, salen)) < 0)
	{	
		if ( !(errno == EINPROGRESS || errno == EALREADY ) )
		{
			//#define EALREADY	114 /* Operation already in progress */
			//#define EINPROGRESS 115 /* Operation now in progress */				
			
			g_warning("%s : connect errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
			ret = -2; goto done;
		}
		
		FD_ZERO (&wset);
		FD_SET (sockfd, &wset); 
		
		n = select ( sockfd+1, NULL, &wset, NULL, &tval);
		if( n < 0) {
			g_warning("%s : select errno[%d] %s\n", __FUNCTION__, errno, strerror(errno));
			ret = -2; goto done;						
		}else if (n){
			ret = 1; goto done;  // connect ok
		}
				
	}else{
		ret = 1; goto done; // connect ok
	}		
		
	ret = -3; //timeout
	
done:	
	Close(sockfd);	
	return ret;	// ok
} 

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int port_test(char *host, unsigned short port)
{
	int ret;		
	struct sockaddr_in addr;							
	
	bzero(&addr, sizeof(struct sockaddr_in));
		
	//g_message("%s : host[%s] port[%d]", __FUNCTION__, host, (int)port);
				
	ret =  my_gethostbyname( host, &addr);	
	if(ret != 1) 
	{
		g_warning("%s : my_gethostbyname ret[%d]",  __FUNCTION__, ret);	
		return -1;		
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	// 0.250 sec
	ret = connect_test( (SA *)&addr, sizeof(struct sockaddr_in), 3000000 );
	if(ret != 1)
	{
		g_warning("%s : connect_test ret[%d]",  __FUNCTION__, ret);
		return -2;
	}
	
	return 1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int port_test_netsvr( int is_ddns )
{
	// htr_message_t *msg;
	int ret = 0;
	int i;
	char host[256];

	unsigned short port = nf_sysdb_get_uint("net.rtp.rtspport");

	if( is_ddns){		  
		_ddns_make_hostaddr( host, sizeof(host) );
	}else{
		snprintf(host, sizeof(host), "127.0.0.1");
	}
	
	ret = port_test(host, port);		  
#ifdef _DEBUG_NET
	g_message("%s : host[%s] port[%d] ret[%d]", __FUNCTION__, host, port, ret);
#endif

	// choissi force ddns update 2009-11-17 ���� 8:10:18
	if( is_ddns == NF_NETWORK_PORT_TEST_ADDRESS_DDNS && ret != 1)
		ddns_force_register();

	return ret;		 
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int port_test_websvr( int is_ddns)
{
	// htr_message_t *msg;
	int ret = 0;
	unsigned char host[256];
	unsigned short port = nf_sysdb_get_uint("net.proto.webport");
	char *macaddr;

	if( is_ddns) {		  		  				
		_ddns_make_hostaddr( host, sizeof(host) );
	}else{
		snprintf(host, sizeof(host), "127.0.0.1");
	}									 
		
	ret = port_test(host, port);	
#ifdef _DEBUG_NET
	g_message("%s : host[%s] port[%d] ret[%d]", __FUNCTION__, host, port, ret);
#endif

	// choissi force ddns update 2009-11-17 ���� 8:10:18
	if( is_ddns == NF_NETWORK_PORT_TEST_ADDRESS_DDNS && ret != 1)
		ddns_force_register();

	return ret;		 
}

ddns_req_t ddns_req;
ddns_res_t ddns_res;

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void convert_hexstr_to_hexint(char *src, char *des, int size)
{
	char macchar;
	int i;

	g_return_if_fail(src != NULL);
	g_return_if_fail(des != NULL);
	g_return_if_fail(size >= 0);
	
	for(i = 0; i < size; i++)
	{
		macchar = src[i];

		if (macchar == 'a')
		{
			des[i] = 10;
		}
		else if (macchar == 'b')
		{
			des[i] = 11;
		}
		else if (macchar == 'c')
		{
			des[i] = 12;
		}
		else if (macchar == 'd')
		{
			des[i] = 13;
		}
		else if (macchar == 'e')
		{
			des[i] = 14;
		}
		else if (macchar == 'f')
		{
			des[i] = 15;
		}
		else
		{
			des[i] = atoi(&macchar);
		}
#ifdef _DEBUG_NETx
		g_message("%s mac[%d] : 0x%x",__FUNCTION__,i, des[i]);
#endif

	}

}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void make_ddns_req()
{
	//char *macaddr = "00115fff3031";
	char *macaddr;
	char mac[12];
	int i;
		
	macaddr = nf_sysdb_get_str_nocopy("sys.info.mac");		
	g_return_if_fail ( macaddr!= NULL );

	memset(mac, 0, sizeof(mac));
		
	convert_hexstr_to_hexint(macaddr, mac, 12);

	for (i = 0; i < 12; i+=2)
	{
		ddns_req.mac[i/2] = 0xff & ((mac[i] << 4) | mac[i+1]);
#ifdef _DEBUG_NETx
		g_message("ddns_req.mac[%d] : 0x%x",i/2, ddns_req.mac[i/2]);
#endif
	}
	
	ddns_req.version 	 = htole16 (DDNS_PROTOCOL_VERSION);
	ddns_req.action_type = htole16 (DEFAULT_ACTION);
	ddns_req.class_id	 = htole16 (DEFAULT_CLASSID);
		
}

int nf_get_external_ip(char *external_ip)
{
	int ret = 0;
	
	ret = nf_ddns_itx_get_myip(external_ip);
	if(ret != 0)
	{
		g_message("%s : nf_ddns_itx_get_myip ===>> ret[%d]",__FUNCTION__ ,ret);		
		
		ret = nf_ddns_dyn_get_myip(external_ip);
		if(ret != 0)
		{
			g_message("%s : nf_ddns_dyn_get_myip ===>> ret[%d]",__FUNCTION__ ,ret);								
			
			ret = nf_upnp_get_external_ip(external_ip);			
			if(ret != 0)
				g_message("%s : nf_upnp_get_external_ip ===>> ret[%d]",__FUNCTION__,ret);			
				
			return ret;
		}else{
			return ret;
		}
	}else{
		return ret;
	}
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int nf_get_public_ip_by_ddns(char *external_ip)
{
	int ret = 0;

	ret = nf_ddns_itx_get_myip2(external_ip);
	if(ret != 0)
	{
		g_message("%s : nf_ddns_itx_get_myip ===>> ret[%d]",__FUNCTION__ ,ret);

		ret = nf_ddns_dyn_get_myip2(external_ip);
		if(ret != 0)
		{
			g_message("%s : nf_ddns_dyn_get_myip ===>> ret[%d]",__FUNCTION__ ,ret);
		}
		return ret;
	}
	else
	{
		return ret;
	}
}

/*******************************************************************************
 * Function  :
 * Prototype :
 * Arguments :
 * Return :
 ******************************************************************************/
void init_ddns_client()
{
	memset(&ddns_req,0x00,sizeof(ddns_req_t));
	memset(&ddns_res,0x00,sizeof(ddns_res_t));
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int send_ddns_packet(char *ddns)
{
	#define PKT_LEN 	sizeof(ddns_req_t)

	int	 len, sock, ret; 	
	struct sockaddr_in ddnssvraddr;
	struct sockaddr_in ddnsrcvaddr;		
	unsigned short ddnsport = INTELLIX_DDNS_PORT;		
	unsigned char packetBuf[PKT_LEN +4];
	
	fd_set readfds;
	struct timeval waitTime;

	if( strnlen(ddns, 64) == 0 )
	{
		g_warning("%s : ddns svr error[%s]\n", __FUNCTION__, ddns);
		return -8;		
	}
		
	// Create a datagram/UDP socket
	sock = Socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		g_warning("%s : Can't Create Socket", __FUNCTION__);
		return -1;
	}

	// Construct the server address structure
	memset(&ddnssvraddr, 0, sizeof(ddnssvraddr));	// Zero out structure
	ddnssvraddr.sin_family 	= AF_INET;				// Internet addr family		
	ddnssvraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ddnssvraddr.sin_port 	= htons(ddnsport);		// Server port	
	
	if(Bind(sock, (struct sockaddr*)&ddnssvraddr, sizeof(ddnssvraddr))<0)
	{
		g_warning("%s : Bind Error", __FUNCTION__);
		Close(sock);
		return -2;
	}

	memset(&ddnssvraddr, 0, sizeof(ddnssvraddr));	// Zero out structure
	ret = my_gethostbyname( ddns, &ddnssvraddr);
	if(ret != 1)
	{
		g_warning("%s : my_gethostbyname fail ddns[%s]\n", __FUNCTION__, ddns);
		Close(sock);
		return -9;
	}	
	ddnssvraddr.sin_family 	= AF_INET;		// Internet addr family		
	ddnssvraddr.sin_port 	= htons(ddnsport);		// Server port
	memcpy(packetBuf, &ddns_req, sizeof(ddns_req_t));	

#ifdef _DEBUG_NET_DDNS
	// debug dump
	g_message("%s : ddns_req dump", __FUNCTION__);
	HexDump( &ddns_req, sizeof(ddns_req_t), 0);
	
//	g_message("%s : ddnssvraddr dump", __FUNCTION__);
//	HexDump( &ddnssvraddr, sizeof(ddnssvraddr), 0);
#endif			
	
	// send	
	len = sizeof(ddns_req_t);
	ret = Sendto(sock, packetBuf, len, 0, 
					(struct sockaddr *)&ddnssvraddr, sizeof(ddnssvraddr));
	if (ret == -1)
	{			
			g_warning("%s : Sendto error", __FUNCTION__);		
			Close(sock);
			return -3;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	waitTime.tv_sec = 3;
	waitTime.tv_usec = 0;

	ret = Select(sock+1, &readfds, NULL, NULL, &waitTime);
	if(ret == -1)		
	{
		g_warning("%s : select failed", __FUNCTION__ );
		Close(sock);				
		return -4;
	}
	else if (ret == 0 ) //timeout
	{
		g_warning("%s : select timeout", __FUNCTION__ );
		Close(sock);				
		return -5;
	}
				
	if(FD_ISSET(sock, &readfds))
	{
		char resData[256];
		int resDataLen;
		int addr_len = sizeof(ddnsrcvaddr);
		
		memset(&ddnsrcvaddr, 0x00, sizeof(ddnsrcvaddr));
		memset(resData, 0x00, sizeof(resData));
		
		resDataLen = Recvfrom(sock, resData, sizeof(resData), 0, 
							(struct sockaddr *)&ddnsrcvaddr, &addr_len);
#ifdef _DEBUG_NET_DDNS		
		g_message("rev data length : %d\n",resDataLen);
#endif
		if(resDataLen > 0)
		{
#ifdef _DEBUG_NET_DDNS		
			HexDump( resData, resDataLen, 0);
#endif			
			ddns_res_t *ptr_ddns_res = (ddns_res_t *)resData;
			g_message("%s : DDNS Response time[%d]", 
				__FUNCTION__, (ptr_ddns_res->time) );

		}
		if(*(int *)resData == 0)
		{
			g_warning("%s : Invalid DDNS Request", __FUNCTION__ );
			Close(sock);
			return -100;
		}				
	}	

	Close(sock);
	return 1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _get_addr_make_hostaddr( char *buff, int buff_len, char *tmp_host, int host_len)
{
	char *macaddr = NULL;
	char *ddns_server = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");

	if( !strcasecmp("dvrlink.net", ddns_server) )
	{
		if(host_len == 0)
		{
			macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
			snprintf(buff, buff_len-1, "%s.dvrlink.net", macaddr);
		}
		else
		{
			snprintf(buff, buff_len-1, "%s", tmp_host);
		}
	}
	else if( !strcasecmp("dyndns.org", ddns_server) )
	{
		if(host_len == 0)
		{
			macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
			snprintf(buff, buff_len-1, "%s.dyndns.org", macaddr);
			
		}
		else
		{
			snprintf(buff, buff_len-1, "%s", tmp_host);
		}
	}
	else if( !strcasecmp("fujiko.biz", ddns_server) )
	{
		if(host_len == 0)
		{
			macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
			snprintf(buff, buff_len-1, "%s.fujiko.biz", macaddr);
			
		}
		else
		{
			snprintf(buff, buff_len-1, "%s", tmp_host);
		}
	}
	else if( !strcasecmp("udrdns.net", ddns_server) )
	{
		macaddr = nf_sysdb_get_str_nocopy("sys.info.mac");
		snprintf(buff, buff_len-1, "%s.udrdns.net", macaddr);
	}	
	else
	{
		g_printf("!!!!!!!!!!!!! _ddns_make_hostaddr: host name error \n");
	}

	if(buff != NULL)
		g_printf("_ddns_make_hostaddr: ddns server name [%s] \n",buff);
	
	return 1;	
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int _ddns_make_hostaddr( char *buff, int buff_len)
{
	char *macaddr = NULL;

	char *ddns_server = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
	if( !strcasecmp("dvrlink.net", ddns_server) )
	{
		macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
		snprintf(buff, buff_len-1, "%s.dvrlink.net", macaddr);
	}
	else if( !strcasecmp("dyndns.org", ddns_server) )
	{
		macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
		snprintf(buff, buff_len-1, "%s.dyndns.org", macaddr);
	}
	else if( !strcasecmp("fujiko.biz", ddns_server) )
	{
		macaddr = nf_sysdb_get_str_nocopy("net.ddns.hostname");
		snprintf(buff, buff_len-1, "%s.fujiko.biz", macaddr);
	}
	else if( !strcasecmp("s1.co.kr", ddns_server) )
	{	
		//s1 ddns is not a standard dns system. so the hostname is just the mac address.
		macaddr = nf_sysdb_get_str_nocopy("sys.info.mac");
		snprintf(buff, buff_len-1, "%s", macaddr);
	}
	else if( !strcasecmp("udrdns.net", ddns_server) )
	{
		macaddr = nf_sysdb_get_str_nocopy("sys.info.mac");
		snprintf(buff, buff_len-1, "%s.udrdns.net", macaddr);
	}	
	else
	{
		g_printf("!!!!!!!!!!!!! _ddns_make_hostaddr: host name error \n");
	}

	if(buff != NULL)
		g_printf("_ddns_make_hostaddr: ddns server name [%s] \n",buff);
	
	return 1;	
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int ddns_get_hostaddr( char *buff, int buff_len)
{		
	return _ddns_make_hostaddr( buff, buff_len);	
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int get_dns_ip_addr(char *ip_addr, int char_len, char *host, int host_len)
{
	int ret = 0;
	int i;
	char tmp_ddns_host[256] = {0,};

	if( host != NULL && strstr( host, "s1.co.kr" ) != NULL ) {
		return _s1_query_ddns_packet( ip_addr, char_len);
	}else{					
		_get_addr_make_hostaddr( tmp_ddns_host, sizeof(tmp_ddns_host), host, host_len);
		ret =  my_dns_gethostbyname( tmp_ddns_host, ip_addr, char_len);	
	}	
	return ret;		 
}

// for internal use
int get_dns_ip_addr_s1(char *str_addr, int str_len)
{		
	return _s1_query_ddns_packet( str_addr, str_len);
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 1: success, others error
 ******************************************************************************/ 
int ddns_common_force_register( NF_DDNS_COMMON_REG_PARAM *param)
{
	int ret = 0;		
	
	g_return_val_if_fail( param != NULL, 0);	

	g_message("%s ddns_server[%s]",__FUNCTION__, param->ddns_server);
	g_message("%s hostname[%s]",__FUNCTION__, param->hostname);
	g_message("%s username[%s]",__FUNCTION__, param->username);
	g_message("%s passwd[%s]",__FUNCTION__, param->passwd);

	if( !strcasecmp("dvrlink.net", param->ddns_server) )
	{	
		DDNS2_PARRAM send_par;
		g_return_val_if_fail( param->hostname[0] != 0x00, 0);	
				
        memset(&send_par, 0x00, sizeof(DDNS2_PARRAM));
        ddns2_set_param(&send_par);	
        
		snprintf( send_par.host_name, sizeof(send_par.host_name)-1, "%s", param->hostname);
		ret = nf_ddns2_register(&send_par);
	}
	else if( !strcasecmp("dyndns.org", param->ddns_server) )
	{
		g_return_val_if_fail( param->hostname[0] != 0x00, 0);	
		g_return_val_if_fail( param->username[0] != 0x00, 0);	
		g_return_val_if_fail( param->passwd[0] != 0x00, 0);	

		ret = dyn_ddns_force_register(param->hostname,
										param->username, 
										param->passwd);

	}
	else if( !strcasecmp("fujiko.biz", param->ddns_server) )
	{
		g_return_val_if_fail( param->hostname[0] != 0x00, 0);	
		g_return_val_if_fail( param->username[0] != 0x00, 0);	
		g_return_val_if_fail( param->passwd[0] != 0x00, 0);	
	
		ret = fujiko_ddns_force_register(param->hostname,
										param->username, 
										param->passwd);
																				
	}
	else if( !strcasecmp("s1.co.kr", param->ddns_server) )
	{	
		ret = s1_ddns_force_register();
	}
	else if( !strcasecmp("udrdns.net", param->ddns_server) )
	{
		ret = dau_ddns_force_register();
	}	
	else	
	{
		g_warning("%s unkown server[%s]", __FUNCTION__,  param->ddns_server);
		return 0;
	}
			
	g_message("%s ===========>>>> ret[%d][%s]",__FUNCTION__, ret, (ret ==1) ? "SUCCESS":"FAILED" );	
	return ret;
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void ddns_dump_status(  NF_DDNS_STATUS *status )
{	
	g_return_if_fail( status != NULL );
	
	g_message("DDNS_STATUS update_tv    [%d.%06d]", status->update_tv.tv_sec, status->update_tv.tv_usec);
	g_message("DDNS_STATUS status       [%d]",status->status);
	g_message("DDNS_STATUS ip_addr      [%s]",status->ip_addr);
	g_message("DDNS_STATUS host_addr    [%s]",status->host_addr);

	g_message("DDNS_STATUS t_state      [%d]",status->thread_state);
	g_message("DDNS_STATUS t_sleep_sec  [%d]",status->thread_sleep_sec);
//	g_message("DDNS_STATUS t_test_cnt   [%d]",status->thread_test_cnt);
	
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int ddns_get_status( NF_DDNS_STATUS *status )
{
	struct sockaddr_in addr;
	int link_status = 0;
	g_return_val_if_fail( status != NULL, 0);
	
	// get host_addr
	_ddns_make_hostaddr( _ddns_status.host_addr, sizeof( _ddns_status.host_addr) );
	
	// update host_addr -> ip_addr
	bzero(&addr, sizeof(struct sockaddr_in));
	
#if defined( ENABLE_DDNS_STATUS_GET_IP )
	if( _ddns_status.status == 1 ){	
		if( _ddns_status.server_idx == 3 )  { // S1	
			memset( _ddns_status.ip_addr, 0x00, sizeof(_ddns_status.ip_addr));
			get_dns_ip_addr_s1 ( _ddns_status.ip_addr, sizeof(_ddns_status.ip_addr) );
			
		} else {
			my_gethostbyname( _ddns_status.host_addr, &addr);
			my_inet_ntoa_r( addr.sin_addr, _ddns_status.ip_addr, 
								sizeof(_ddns_status.ip_addr) );	
		}									
	}
#endif
									
	g_static_mutex_lock( &_ddns_status_lock );
	memcpy( status, &_ddns_status, sizeof( NF_DDNS_STATUS ));
	g_static_mutex_unlock( &_ddns_status_lock );

	if( _Link_Status && _ddns_status.status != 1 )
		ddns_force_register();
		
	return 1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int ddns_force_register()
{	
	++_ddns_refresh;	
	g_message("%s _ddns_refresh [%d]",__FUNCTION__, _ddns_refresh);		
	return _ddns_refresh;		
}

static void _ddns_set_param( char *ddns, int *ddns_server_index, 
		int *getip_err_wait_time, 
		int *update_err_wait_time,
		int *ddns_force_update_cnt)
{

	if( !strcasecmp("dvrlink.net", ddns) )
	{
		*ddns_server_index = 0;
		*getip_err_wait_time = 0;
		*update_err_wait_time = 0;
		*ddns_force_update_cnt = DDNS_FORCE_UPDATE_MAX;		
	}
	else if( !strcasecmp("dyndns.org", ddns) )
	{
		*ddns_server_index = 1;
		*getip_err_wait_time = 10;
		*update_err_wait_time = 2;
		*ddns_force_update_cnt = DDNS_FORCE_UPDATE_MAX;
	}
	else if(!strcasecmp("fujiko.biz", ddns))
	{
		*ddns_server_index = 2;
		*getip_err_wait_time = 10;
		*update_err_wait_time = 2;
		*ddns_force_update_cnt = DDNS_FORCE_UPDATE_MAX;
	}
	else if(!strcasecmp("s1.co.kr", ddns))
	{
		*ddns_server_index = 3;
		*getip_err_wait_time = 0;
		*update_err_wait_time = 0;
		*ddns_force_update_cnt = DDNS_FORCE_UPDATE_MAX;		
	}
	else if( !strcasecmp("udrdns.net", ddns) )
	{
		*ddns_server_index = 4;
		*getip_err_wait_time = 0;
		*update_err_wait_time = 0;
		*ddns_force_update_cnt = 170;
	}	
	else
	{
		*ddns_server_index = 0;
		*getip_err_wait_time = 0;
		*update_err_wait_time = 0;
		*ddns_force_update_cnt = DDNS_FORCE_UPDATE_MAX;
	}
	    			
}

static gboolean _ddns_is_on()
{
	gboolean ddnson;
	gboolean policy;
	gboolean dev_block;

	ddnson = nf_sysdb_get_bool("net.proto.ddnson");
	
#if defined(_IPX_1624M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1648P4E) || defined(_IPX_0824P4E) \
 || defined(_IPX_1648M4E) || defined(_IPX_0824M4E) || defined(_IPX_0412M4E)|| defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
	policy = nf_sysdb_get_bool("sys.info.agr_policy");
	dev_block = nf_sysdb_get_bool("sys.info.guard.dev_block");

	if(ddnson == TRUE && policy == TRUE && dev_block == FALSE)
		return TRUE;
	else
		return FALSE;
#else
	return ddnson;
#endif

}

#define ENABLE_NET_FW_CHECKER
#define NET_FW_CHECK_INTERNAL   (86400)

#ifdef ENABLE_NET_FW_CHECKER
int get_netfwup_check_interval(void)
{
	static int interval = -1;
	char *testmail = NULL;
	int len = 0;
	int i = 0;

	if(interval != -1)
		return interval;
    
	testmail = nf_sysdb_get_str_nocopy("net.email.testmail");
    if (strstr(testmail, "@netfwup.com"))
    {
    	len = strlen(testmail);
    	for(i = 0; i < len; i++)
    	{
    		if(testmail[i] == '@')
    		{
    			testmail[i] = '\0';
				interval = atoi(testmail);
				testmail[i] = '@';
				break;
    		}
    	}
    }
    
    if (interval > 0 && interval < 86400)
    {
    	g_printf(" * set netfwup_check_interval '%d' sec\n", interval);
    	return interval;
    }

    interval = -1;	
    return NET_FW_CHECK_INTERNAL;
}
#endif
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 

guint _get_net_fw_up_check_time(void)
{
	guint ret;
	
	ret = nf_sysdb_get_uint("sys.info.remotefw_check");

	return ret;
}

int s1_send_ddns_command(int wait, int retry_cnt)
{
	int retry = 0;
	int ret = 0;

	while(1)
	{
		g_message("DDNS - %s - wait:%d, retry:%d, retry_max:%d", __FUNCTION__, wait, retry, retry_cnt);
		
		ret = _s1_send_ddns_packet();
		if((ret == 1) || (retry >= retry_cnt))
			break;
		else{
			retry++;
			sleep(wait);		
		}
	}

	return ret;
}

#define S1_DDNS_LOOP_TIME	60
#define S1_DDNS_FORCE_TIME	900

void s1_ddns_scenario(void)
{
	static guint loop_timer = S1_DDNS_LOOP_TIME;
	static guint force_timer = 0;
	static guint wait_sec = 0;
	static guint wait_cnt = 0;
	static guint retry_cnt = 0;
	static int bef_link_status = 1;
	static int bef_wan_status = 9630;

	if(_Link_Status != 0 && bef_link_status == 0)
	{
		g_printf("S1 DBG connect Link_Status[%d] bef_link_status[%d]\n",_Link_Status,bef_link_status);
		nf_notify_fire_params("net_wan_status", 0, 0, 0, 0);
		bef_link_status = 1;
		bef_wan_status = 0;
	}		

	if( nf_sysdb_get_bool("net.proto.ddnson") != 1 
		|| _Link_Status == 0 )
	{	// ddns�� off �̸�
		if(_Link_Status == 0 && bef_link_status == 1)
		{
			g_printf("S1 DBG disconnect Link_Status[%d] bef_link_status[%d]\n",_Link_Status,bef_link_status);
			nf_notify_fire_params("net_wan_status", 1, 0, 0, 0);
			bef_link_status = 0;
		}
	}

	if( wait_cnt > 0 )
	{
		wait_cnt--;

		if(wait_cnt == 0)
		{
			int send_ret = 0;

			send_ret = _s1_send_ddns_packet();
							
			if(send_ret == 1)
			{
				retry_cnt = 0;
				ddns_set_status(1);
			}
			else
			{
				if( retry_cnt > 0 )
				{
					wait_cnt = wait_sec;
					retry_cnt--;
				}
				else					
					ddns_set_status(S1_DDNS_ERR_UPDATE);
			}
		}

		return;
	}

	if( _ddns_refresh > 0 )
	{
		_ddns_refresh = 0;
		loop_timer = S1_DDNS_LOOP_TIME;
		force_timer = S1_DDNS_FORCE_TIME;

		wait_sec = 0;
		wait_cnt = 0;
		retry_cnt = 0;
	}
	
	if( loop_timer >= S1_DDNS_LOOP_TIME )
	{
		g_message("DDNS - %s - loop_timer", __FUNCTION__);
		
		loop_timer = 0;
		
		if( _is_ddns_on() )
		{	
			if( _s1_is_connected_eth() )
			{
				int ip_ret;

				if(bef_wan_status != 0 )
				{
					g_printf("S1 DBG connect\n");
					nf_notify_fire_params("net_wan_status", 0, 0, 0, 0);
					bef_wan_status = 0;
				}	

				memset( _ddns_status.get_server_ip, 0x0, sizeof(_ddns_status.get_server_ip) );
				
				ip_ret = nf_get_external_ip(_ddns_status.get_server_ip);
				if( ip_ret == 0 )
				{
					if(strcmp(_ddns_status.get_server_ip, _ddns_status.save_ip))
					{
						memset(_ddns_status.save_ip, 0x00, sizeof(_ddns_status.save_ip) );
						strncpy(_ddns_status.save_ip, _ddns_status.get_server_ip, sizeof(_ddns_status.save_ip) );

						force_timer = 0;
						
						wait_sec = 15;
						wait_cnt = 1;
						retry_cnt = 10;						
/*						
						int send_ret;
						
						memset(_ddns_status.save_ip, 0x00, sizeof(_ddns_status.save_ip) );
						strncpy(_ddns_status.save_ip, _ddns_status.get_server_ip, sizeof(_ddns_status.save_ip) );

						send_ret = s1_send_ddns_command(15, 10);
						
						if( send_ret == 1 )
							ddns_set_status(1);
						else
							ddns_set_status(S1_DDNS_ERR_UPDATE);
*/							
					}
					else
					{	
						if( force_timer >= S1_DDNS_FORCE_TIME )
						{
							force_timer = 0;
							
							wait_sec = 60;
							wait_cnt = 1;
							retry_cnt = 5;							
/*							
							int send_ret;
							
							send_ret = s1_send_ddns_command(60, 5);
							force_timer = 0;
							
							if( send_ret == 1)
								ddns_set_status(1);
							else
								ddns_set_status(S1_DDNS_ERR_UPDATE);
*/								
						}
					}
				}
				else
				{
					ddns_set_status(S1_DDNS_ERR_EXTERN_IP);
				}
			}
			else
			{
				g_printf("S1 DBG disconnect\n");
				nf_notify_fire_params("net_wan_status", 1, 0, 0, 0);
				bef_wan_status = 1;
		
				ddns_set_status(S1_DDNS_ERR_NETORK);
			}
		}
		else
		{
			ddns_set_status(S1_DDNS_STOP);
		}
	}
	else
	{
		g_message("DDNS - %s - sleep", __FUNCTION__);		
		force_timer++;
		loop_timer++;
	}
}

static void _get_ip_addr_str(char *ip_str, int ip_str_size)
{
	NF_NETIF_IP ret_ip;

	nf_netif_get_ip(&ret_ip);
	snprintf(ip_str, ip_str_size, "%d.%d.%d.%d", ret_ip.ip_addr[0], ret_ip.ip_addr[1], ret_ip.ip_addr[2], ret_ip.ip_addr[3]);
}

static void _get_mac_addr_str(char *mac_str, int mac_str_size)
{
	NF_NETIF_MAC mac;

	memset(mac_str, 0x0, mac_str_size);

	nf_netif_get_mac(&mac);

#if 1	
	snprintf(mac_str, mac_str_size, "%02x.%02x.%02x.%02x.%02x.%02x",
		mac.mac_addr[0] & 0xff,
		mac.mac_addr[1] & 0xff, 
		mac.mac_addr[2] & 0xff,	
		mac.mac_addr[3] & 0xff,	
		mac.mac_addr[4] & 0xff,	
		mac.mac_addr[5] & 0xff);
#else
//	snprintf(mac_str, mac_str_size, "00115F300944");
	snprintf(mac_str, mac_str_size, "00115F3001AE");
#endif	
}

#define DAU_DDNS_SVR	"udrdns.net"
#define DAU_DDNS_PORT	7100

static int _dau_write_data(const int s, 
			const char* mac_addr, 
			const char* ip_addr)
{
	char msg[DDNS_BUF_LEN];
	int ret = 0, len = 0;
	
	memset(msg, 0x00, sizeof(msg));
	
	snprintf(msg, sizeof(msg), "%s\n%s\n", mac_addr, ip_addr );

	len = strnlen(msg, sizeof(msg) );

	g_message("%s write len[%d] dump[%s]",__FUNCTION__, len, msg);
	if( (ret=Writen(s, msg, len)) == len )
		return 1;
	else
		return 0;
}

static int _dau_read_data(const int s)
{
	char msg[DDNS_BUF_LEN];
	int ret = 0;
	
	memset(msg, 0x00, sizeof(msg));
		
	if( Readable_timeo(s, 5) <= 0) // 5 sec timeout
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;
	
	if( (ret = read(s, msg, sizeof(msg)-1 )) < 0 )
		return DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR;

	if(msg != NULL)
		g_message("%s read len[%d] dump[%s]",__FUNCTION__, ret, msg);	

	if( strstr(msg, "OK") )
		return 1; // DDNS2_RES_OK_GOOD;

	else
		return DDNS2_RES_RECV_ERR;
} 

static int _dau_send_ddns_packet(void)
{
	int		sock = -1, ret_code = 0, ret;
	char  mac_addr[64] = {0, };
	char  ip_addr[64] = {0, };

	sock = connect_timeout_hostname( DAU_DDNS_SVR, DAU_DDNS_PORT, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s  dns query fail[%s]", __FUNCTION__, DAU_DDNS_SVR);
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		return DDNS2_RES_SOCKET_CREATE_ERR; // dns fail 
	}

	_get_mac_addr_str(mac_addr, sizeof(mac_addr));
	_get_ip_addr_str(ip_addr, sizeof(ip_addr));

	if( _dau_write_data(sock, mac_addr, ip_addr) )
		ret_code = _dau_read_data(sock);
	else
		ret_code = DDNS2_RES_DATA_SEND_ERR;

	Close(sock);
	ddns_set_status(ret_code);
	return ret_code;
} 

int dau_ddns_force_register(void)
{
	int ret_code;
	
	ret_code = _dau_send_ddns_packet();
//	ddns_set_status(ret_code);

	return ret_code;
}

static int _dau_get_dns_ip(char *ip_addr, int ip_addr_len)
{
	char ddns_host[256] = {0, };
	NF_NETIF_MAC mac;
	int ret = 0;
	
	nf_netif_get_mac(&mac);
	
	snprintf(ddns_host, sizeof(ddns_host), "%02x%02x%02x%02x%02x%02x.%s",
		mac.mac_addr[0] & 0xff,
		mac.mac_addr[1] & 0xff, 
		mac.mac_addr[2] & 0xff,	
		mac.mac_addr[3] & 0xff,	
		mac.mac_addr[4] & 0xff,	
		mac.mac_addr[5] & 0xff,
		DAU_DDNS_SVR);

	ret =  my_dns_gethostbyname( ddns_host, ip_addr, ip_addr_len);	

	return ret;	
}

// 64byte
typedef struct _UNIMO_DATA{
	guint model;
	guint fwver;
	guint ipaddr;
	guint stat_flag;
	short port_num[4];
	guint rec_start_time;
	guint rec_end_time;
	guint hdd_capa;
	guint hdd_usage;
	guchar hdd_inst_map;
	guchar hdd_error_map;
	guchar hdd_temper_map;
	guchar reserve_0;
	guint vloss_map;
	guint reserve_server;
	guint boot_time;
	double reserve_1;
} UNIMO_DATA;

typedef struct _UNIMO_SEND_PACKET{
	guint fcc;
	guint cmd;
	guint pktsize;
	guint reserve_0;
	guchar mac[6];
	guchar reverve_1[10];
	UNIMO_DATA uni_data;
} UNIMO_SEND_PACKET;

typedef struct _UNIMO_RECV_PACKET{
	guint fcc;
	guint cmd;
	guint pktsize;
	int retcode;
} UNIMO_RECV_PACKET;

static guint _get_fw_version(void)
{
	char fw_ver[256];
	char *db_ptr;
	char *ptr;
	guint ret = 0;

	memset(fw_ver, 0x0, sizeof(fw_ver));

	db_ptr = nf_sysdb_get_str_nocopy("sys.info.swver");
	if( db_ptr )
	{		
		snprintf(fw_ver, sizeof(fw_ver), "%s", db_ptr);

		ptr = strchr(fw_ver, '.');
		if( ptr ){
			ptr = strchr(ptr+1, '.');
			if( ptr ){				
				ret = strtol( ptr+1, NULL, 10);
			}
		}
	}
	
	return ret;
}

static guint _get_hdd_usage(void)
{
	guint ret = 0;
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_usage");
	if (pnotify) {
		ret = pnotify->d.params[0];
		nf_notify_free(pnotify);
	}

	return ret;
}

static guint _get_video_loss_data(void)
{
	guint ret = 0;
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("vloss");
	if (pnotify) {
		ret = pnotify->d.params[0];
		nf_notify_free(pnotify);
	}

	return ret;
}

static guint _get_stat_flag(void)
{
	guint ret = 0;
	guint write_mode = 0;

	if( nf_panic_record_is_set() )
		ret |= (1 << 0);

	write_mode = nf_sysdb_get_uint("disk.write_mode");
	if( write_mode )
	{
		ret |= (1 << 2);
	}
	else
	{
		NF_NOTIFY_INFO *info = NULL;

		info = nf_notify_get("disk_full");
		if( info )
		{
			if(info->d.params[0])
				ret |= (1 << 1);

			nf_notify_free(info);
		}
	}

	printf("[%s] STAT[0x%x]\n", __FUNCTION__, ret);

	return ret;
}

static guint _convert_to_localtime(guint time)
{
	guint gmt_time, local_time;
	struct tm   tmval;
	struct timeval tval ={0, 0};
	guint differ_time;

	if( time == 0 )
	{
//		printf("[%s] Time is 0...\n", __FUNCTION__);		
		return 0;
	}
	
	gettimeofday((struct timeval *)&tval, NULL);

	gmtime_r(&(tval.tv_sec), &tmval);
	gmt_time = mktime(&tmval);
	
	localtime_r(&(tval.tv_sec), &tmval);
	local_time = mktime(&tmval);

	if( local_time >= gmt_time )
	{
		differ_time = local_time - gmt_time;
//		printf("[%s] differ_time = %d\n", __FUNCTION__, differ_time);
		return time + differ_time;
	}
	else
	{		
		differ_time = gmt_time - local_time;
//		printf("[%s] differ_time = -%d\n", __FUNCTION__, differ_time);
		return time - differ_time;
	}
}
	
static void _make_unimo_data(UNIMO_DATA *uni_data)
{
	NF_DISK_INFO disk_info;
	guint64 dSize = 0;
	guint db_uint = 0;
	char hdd_map = 0;
	int cnt;
	int i,j, disk_check;
	guint rec_start, rec_end;
	int ret;
	guint trec_start = 0, trec_end = 0;
	NF_DISK_REC_DISK_TIME rt;

	if(uni_data == NULL)
	{
		g_warning("[%s] uni_data == NULL", __FUNCTION__);
		return;
	}
	
	memset(uni_data, 0x0, sizeof(UNIMO_DATA));
	
#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
	uni_data->model = 2103;
#elif defined(_IPX_0824M4) || defined(_IPX_0824M4E)
	uni_data->model = 2102;
#elif defined(_IPX_0412M4) || defined(_IPX_0412M4E)
	uni_data->model = 2101;
#elif defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
	uni_data->model = 2103;
#elif defined(_IPX_0824P4E)
	uni_data->model = 2102;
#elif defined(_UTM6GB_0412D)
	uni_data->model = 2001;
#elif defined(_UTM6GB_0824D)
	uni_data->model = 2002;
#elif defined(_UTM6GB_1648D)
	uni_data->model = 2003;
#else
	uni_data->model = 0;
#endif

	uni_data->fwver = _get_fw_version();
	uni_data->ipaddr = nf_sysdb_get_uint("net.proto.ipaddr");
	uni_data->stat_flag = _get_stat_flag();

	uni_data->port_num[0] = nf_sysdb_get_uint("net.rtp.rtspport");
	uni_data->port_num[1] = nf_sysdb_get_uint("net.proto.webport");
	uni_data->port_num[2] = 0;
	uni_data->port_num[3] = 0;	

	disk_check = nf_disk_get_info(&disk_info, NULL);
    if(disk_check == 1){
		uni_data->hdd_usage = _get_hdd_usage();

		for(j = 0; j < 2; j++)
		{
	    	for(i=0; i<16; i++)
			{
	    		if(disk_info.disk_state[j][i] & 0x01)
	    		{
	    			dSize+=disk_info.disk_size[j][i];

					ret = nf_disk_rec_disk_time(j, i, 1, &rt, NULL);
					if (!ret) continue;
					if (rt.ucPortNo[j][i] == 0xff) continue;

					if (trec_start == 0) trec_start = rt.min_time;
					if (rt.min_time < trec_start) trec_start = rt.min_time;
					if (rt.max_time > trec_end) trec_end = rt.max_time;
					
	    		}
	    	}
		}
		
    	uni_data->hdd_capa = (guint)(dSize/1000);
    }
    else{
		uni_data->hdd_capa = 0;
		uni_data->hdd_usage = 0;
    }

	uni_data->rec_start_time = _convert_to_localtime(trec_start);
	uni_data->rec_end_time = _convert_to_localtime(trec_end);

	cnt = nf_get_ddns_disk_count();
	if( cnt > 0 ){
		hdd_map = 0;
		for(i=0; i<cnt; i++)
		{
			hdd_map |= (1 << i);
		}
	}
	
	uni_data->hdd_inst_map = hdd_map;
	uni_data->hdd_error_map = 0;
	uni_data->hdd_temper_map = 0;

	uni_data->vloss_map = _get_video_loss_data();
	uni_data->reserve_server = 0;
	uni_data->boot_time = _convert_to_localtime(nf_sysman_get_boot_time());

	uni_data->reserve_1 = 0;	

	printf("---------- UNIMO CHECK DATA --------\n");
	printf("MODEL[%d], FWVER[%d], STATFLAG[%x]\n",
		uni_data->model, uni_data->fwver, uni_data->stat_flag);	
	printf("IPADDR[0x%x], PORT0[%d], PORT1[%d], REC_START[%d], REC_END[%d]\n",
		uni_data->ipaddr, uni_data->port_num[0], uni_data->port_num[1], uni_data->rec_start_time, uni_data->rec_end_time);
	printf("HDD_CAPA[%d], HDD_USAGE[%d], HDD_INSTALL[%x], VLOSS[%x], BOOT_TIME[%d]\n",
		uni_data->hdd_capa, uni_data->hdd_usage, uni_data->hdd_inst_map, uni_data->vloss_map, uni_data->boot_time);
	printf("------------------------------------\n");	
}

#define UNIMO_CHECK_SVR		"ddns.uniddns.com"
//#define UNIMO_CHECK_SVR		"125.7.231.199"
//#define UNIMO_CHECK_SVR		"125.7.231.197"

#define UNIMO_CHECK_PORT	7300

static int _unimo_checker_status = UNIMO_CHECK_RES_INITIALIZE;

int nf_ddns_get_unimo_checker_status(void)
{
	return _unimo_checker_status;
}

int _set_unimo_checker_status(int status)
{
	_unimo_checker_status = status;
}

int _convert_unimo_status(int retcode)
{
	if( retcode == 0 )
		return UNIMO_CHECK_RES_SUCCESS;
	else if( retcode == 1 )
		return UNIMO_CHECK_RES_FAIL_ERR;
	else if( retcode == 2 )
		return UNIMO_CHECK_RES_BADAUTH_ERR;
	else 
		return UNIMO_CHECK_RES_UNKNOWN_ERR;
}

int _unimo_check_send_packet(gchar *server, guint port)
{
	UNIMO_SEND_PACKET send_packet;
	UNIMO_RECV_PACKET recv_packet;
	
	char msg[DDNS_BUF_LEN];
	int ret = 0, len = 0, sock = -1;

	NF_NETIF_MAC mac;

	char *unimo_server = NULL;
	int unimo_port = 0;

	if( server == NULL ){
		unimo_server = nf_sysdb_get_str_nocopy("net.proto.unimo.server");
		if( !unimo_server )
			return UNIMO_CHECK_RES_INTERNAL_ERR;
	}
	else{
		unimo_server = server;
	}

	if( port == 0 ){
		unimo_port = nf_sysdb_get_uint("net.proto.unimo.port");
		if( !unimo_port )
			return UNIMO_CHECK_RES_INTERNAL_ERR;
	}
	else{
		unimo_port = port;
	}

	sock = connect_timeout_hostname( unimo_server, unimo_port, DDNS_TCP_CONNECT_TIMEOUT );
	if (sock == -1 ) 
	{
		g_message("%s  dns query fail[%s]", __FUNCTION__, UNIMO_CHECK_SVR);
		return UNIMO_CHECK_RES_NETWORK_ERR; // dns fail 
	}
	else if( sock < 0) 
	{ // connect fail, try other server			
		return UNIMO_CHECK_RES_NETWORK_ERR; // dns fail 
	}

	memset(&send_packet, 0x0, sizeof(UNIMO_SEND_PACKET));

	send_packet.fcc = 0x33564444;	
	send_packet.cmd = 1;
	send_packet.pktsize = sizeof(UNIMO_SEND_PACKET);

	nf_netif_get_mac(&mac);	
	memcpy(send_packet.mac, mac.mac_addr, sizeof(send_packet.mac));

	printf("[%s] MAC:%02x%02x%02x%02x%02x%02x\n", __FUNCTION__,
		send_packet.mac[0] & 0xff,
		send_packet.mac[1] & 0xff, 
		send_packet.mac[2] & 0xff,	
		send_packet.mac[3] & 0xff,	
		send_packet.mac[4] & 0xff,	
		send_packet.mac[5] & 0xff);

	_make_unimo_data(&(send_packet.uni_data));

	len = send_packet.pktsize;

	g_message("%s write len[%d]",__FUNCTION__, len);
	if( (ret=Writen(sock, &send_packet, len)) != len )
	{
		Close(sock);
		return UNIMO_CHECK_RES_DATA_SEND_ERR;
	}

	memset(&recv_packet, 0x0, sizeof(UNIMO_RECV_PACKET));
		
	if( Readable_timeo(sock, 5) <= 0) // 5 sec timeout
	{
		Close(sock);
		return UNIMO_CHECK_RES_DATA_RECV_ERR;
	}

	ret = read(sock, &recv_packet, sizeof(UNIMO_RECV_PACKET));
	if( ret < 0 )
	{
		Close(sock);
		return UNIMO_CHECK_RES_DATA_RECV_ERR;
	}

	Close(sock);

	if( ret == 0 ){
		g_warning("[%s] read len=0", __FUNCTION__);
		return UNIMO_CHECK_RES_DATA_RECV_ERR;
	}
	else{
		g_message("[%s] read len[%d] retcode[%d]",__FUNCTION__, ret, recv_packet.retcode);
		return _convert_unimo_status(recv_packet.retcode);	
	}
} 

int nf_ddns_register_unimo_checker(gchar *server, guint port)
{
	int ret;
	
	ret = _unimo_check_send_packet(server, port);

	_set_unimo_checker_status(ret);

	return ret;
}

int unimo_check_delay = 540;

gboolean _is_unimo_check_enable(void)
{
	gboolean unimo_on = FALSE;
	gboolean policy;
	gboolean dev_block;
	
	unimo_on = nf_sysdb_get_bool("net.proto.unimo.checker");

#if defined(_IPX_1624M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1624P4E) || defined(_IPX_0824P4E) \
 || defined(_IPX_1648M4E) || defined(_IPX_0824M4E) || defined(_IPX_0412M4E)
	policy = nf_sysdb_get_bool("sys.info.agr_policy");
	dev_block = nf_sysdb_get_bool("sys.info.guard.dev_block");

	if(unimo_on == TRUE && policy == TRUE && dev_block == FALSE)
		return TRUE;
	else
		return FALSE;
#else
	return unimo_on;
#endif
}

void ddns_func(void *arg)
{
	int	ret = 0;	
	int update_time = 0;	
	char *ddns, *mac;
	char tmp_ddns_svr[256] = {0,};

    DDNS2_PARRAM send_par;
    int upnp_check_time = 0;
	int port, bef_link_status = 1;
	int last_update_time;
	int ddns_server_index, getip_err_wait_time, update_err_wait_time, ddns_force_update_cnt;
	int bef_wan_status = 9630;

	unsigned int save_ip = 0, cur_ip = 0;
    NF_NETIF_GET_INFO ret_net_info;

#ifdef ENABLE_NET_FW_CHECKER
    int fw_check_time = NET_FW_CHECK_INTERNAL;
#endif
#if defined(ENABLE_NET_FW_S1_SCENARIO)
    int fw_check_time_s1 = _get_net_fw_up_check_time;
#endif 

	int dau_ret = 0, dau_retry = 0, dau_delay = 0, dau_dns = 0;

	g_message("%s thread start!!", __FUNCTION__);
		
	ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
	mac = nf_sysdb_get_str_nocopy("sys.info.mac");

	g_return_if_fail(ddns != NULL);
	g_return_if_fail(mac != NULL);

	g_message("%s ddns svr[%s] mac[%s] sleep 10", __FUNCTION__, ddns, mac);	
	sleep(20);
	g_message("%s ddns ready to run", __FUNCTION__ );

	memset(&ret_net_info, 0x00, sizeof(ret_net_info));
	nf_netif_get_info(&ret_net_info);
	save_ip = ret_net_info.ipaddr;
	
enum _enum_DDNS_STATE
{
	DDNS_INIT = 0,
	DDNS_UPDATE,
	DDNS_PORT_TEST,
	
};

	_ddns_set_param( ddns, 
					&ddns_server_index,	
					&getip_err_wait_time, 
					&update_err_wait_time,
					&ddns_force_update_cnt);

	_ddns_status.server_idx = ddns_server_index;

	while(1) 	
	{
#ifdef ENABLE_NET_FW_CHECKER
		++fw_check_time;
		//g_message("%s fw_check_time[%d]", __FUNCTION__, fw_check_time);

		if(_Link_Status && fw_check_time > get_netfwup_check_interval())		
		{
			GError	*error = NULL;	
			int ret = 0;
			fw_check_time = 0;
						
			ret = nf_fw_network_upgrade_info_download( NULL );
			if( ret != 1 )
			{
				if(error){
					g_warning("%s nf_fw_network_upgrade_info_download error[%d][%s]", __FUNCTION__, error->code, error->message);
					g_error_free( error);
				}else{
					g_warning("%s nf_fw_network_upgrade_info_download ret[%d]", __FUNCTION__, ret);
				}
			}			
		}
#endif
		
#if defined(ENABLE_NET_FW_S1_SCENARIO)
		++fw_check_time_s1;

		if( _Link_Status )
		{
			guint fwup_time;			
			fwup_time = _get_net_fw_up_check_time();

			if( (fwup_time != 0) && (fw_check_time_s1 > fwup_time) )
			{				
				fw_check_time_s1 = 0;

				if( nf_fw_network_s1_get_update_state() == 0 )
				{
					nf_fw_network_s1_set_update_state(1);
				
					if( nf_fw_network_s1_update_profile() > 0 )
					{
						g_message("FW fw_up notify");
						
						nf_notify_fire_params("net_s1_fw_up", 0,0,0,0);
					}
						
					nf_fw_network_s1_set_update_state(0);
				}
				else
				{
					g_message("NET FW : update state is TRUE");
				}				
			}
		}
#endif	

#if defined(ENABLE_S1_DDNS_SCENARIO)
		if(_ddns_status.server_idx == S1_DDNS_SERVER_IDX)
		{
			s1_ddns_scenario();
			sleep(1);			
			continue;
		}
#endif

#if 0 //!defined(ENABLE_S1_UPNP_SCENARIO)
        if(++upnp_check_time > 60) // upnp check
        {
            upnp_check_time = 0;

            memset(&ret_net_info, 0x00, sizeof(ret_net_info));
            nf_netif_get_info(&ret_net_info);
                        
            cur_ip = ret_net_info.ipaddr;

            if(save_ip != cur_ip)
            {
	            g_printf("\n\n\n ############ cur_ip [0x%x] save_ip[0x%x]",cur_ip,save_ip);

                g_printf("upnp RTSP Port change : start  =>>>>>>>> \n");
                port = nf_sysdb_get_uint("net.rtp.rtspport");
                ret = nf_upnp_port_reforwording (port, 0);
                g_printf("upnp RTSP Port change : end  =>>>>>>>> \n");
                sleep(3);
                
                g_printf("upnp WEB Port change : start  =>>>>>>>> \n");
                port = nf_sysdb_get_uint("net.proto.webport");
                ret = nf_upnp_port_reforwording (port, 1);
                g_printf("upnp WEB Port change : start  =>>>>>>>> \n");
                sleep(1);
                save_ip = cur_ip;
                
                // 2010-01-19 ���� 5:26:13 choissi                
                ddns_force_register();
            }
        }
#endif

		if(_Link_Status != 0 && bef_link_status == 0)
		{
			g_printf("ddns_func ... Wan connect !!!!! _Link_Status[%d] bef_link_status[%d]\n",_Link_Status,bef_link_status);
			nf_notify_fire_params("net_wan_status", 0, 0, 0, 0);
			bef_link_status = 1;
			bef_wan_status = 0;
		}		

		if( (_Link_Status != 0) && (_is_unimo_check_enable() == TRUE) )
		{
			unimo_check_delay++;
			if( unimo_check_delay > 600 )
			{	
				int unimo_status;
				unimo_status = _unimo_check_send_packet(NULL,0);
				_set_unimo_checker_status(unimo_status);				

				unimo_check_delay = 0;
			}
		}

		if( _ddns_is_on() != 1 
			|| _Link_Status == 0 )
		{	// ddns�� off �̸�
			if(_Link_Status == 0 && bef_link_status == 1)
			{
				g_printf("ddns_func ... Wan disconnect !!!!! _Link_Status[%d] bef_link_status[%d]\n",_Link_Status,bef_link_status);
				nf_notify_fire_params("net_wan_status", 1, 0, 0, 0);
				bef_link_status = 0;
			}
			sleep(1);
			_ddns_status.thread_sleep_sec = 0;
			continue;
		}		 	

		//g_printf("DDNS running...\n");

		if( _ddns_refresh != 0 )	// force ddns refresh
		{
			_ddns_status.thread_sleep_sec = 0;
			_ddns_refresh = 0;				

			g_message("%s ddns_refresh sleep start %d sec", __FUNCTION__, DDNS_REFRESH_SLEEP_SEC );
			sleep(DDNS_REFRESH_SLEEP_SEC);			
			g_message("%s ddns_refresh sleep end", __FUNCTION__ );

			_ddns_status.force_update_count = 0;
			_ddns_status.dns_error_count = 0;
			_ddns_status.getIP_error_count = 0;

    		ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
    		g_return_if_fail(ddns != NULL);

			_ddns_set_param( ddns, 
					&ddns_server_index,	
					&getip_err_wait_time, 
					&update_err_wait_time,
					&ddns_force_update_cnt);

			goto ddns_update;
		}

		if(ddns_server_index == 4){
			if( dau_ret != 1 && (dau_retry > 0) ){
				dau_delay++;

				if(dau_delay > 60){
					dau_delay = 0;
				
					dau_ret = _dau_send_ddns_packet();

					if( dau_ret == 1 )
						dau_retry = 0;
					else
						dau_retry--;
				}	
			}

			dau_dns++;
			if( dau_dns > 600 )
			{
				char dau_dns_ip[256] = {0 };
				
				dau_dns = 0;

				if( _dau_get_dns_ip(dau_dns_ip, sizeof(dau_dns_ip)) >= 0 ){
					if(strcmp(_ddns_status.get_server_ip, dau_dns_ip)){
						printf("[%s] dau_dns_ip not equal...\n", __FUNCTION__ );
						dau_ret = _dau_send_ddns_packet();
					}
				}
			}
		}

		if(_ddns_status.thread_sleep_sec > 0)
		{
			--_ddns_status.thread_sleep_sec;
			sleep(1);
			continue;
		}

		ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
		g_return_if_fail(ddns != NULL);

		_ddns_set_param( ddns, 
				&ddns_server_index,	
				&getip_err_wait_time, 
				&update_err_wait_time,
				&ddns_force_update_cnt);

		g_printf("_____________ get myip start => ddns_server_index[%d]\n",ddns_server_index);

		ret = nf_ddns_itx_get_myip(_ddns_status.get_server_ip);
		if(ret != 0)
		{
			ret = nf_ddns_dyn_get_myip(_ddns_status.get_server_ip);
		}
				
		if(ret != 0)
		{
			g_printf("get_myip &&&&&& ret[%d] getIP_error_count[%d] \n",ret,_ddns_status.getIP_error_count);
	//		if(bef_wan_status == 0 || bef_wan_status == 9630)
			{
				// if we can not get the external ip address, this is "WANFAIL STATUS"
				nf_notify_fire_params("net_wan_status", 1, 0, 0, 0);
				bef_wan_status = ret;
			}
			_ddns_status.getIP_error_count++;
			ddns_set_status(ret);
						
			if(_ddns_status.getIP_error_count > GETIP_ERR_MAX)
				_ddns_status.thread_sleep_sec = 60 * (1 * (GETIP_ERR_MAX + getip_err_wait_time));
			else
				_ddns_status.thread_sleep_sec = 60  * (1 * (_ddns_status.getIP_error_count + getip_err_wait_time)) ;  // todo_test

			continue;
		}
		else
		{
			if(bef_wan_status != 0 )
			{
				nf_notify_fire_params("net_wan_status", 0, 0, 0, 0);
				bef_wan_status = ret;
			}			
			_ddns_status.getIP_error_count = 0;

			if(strcmp(_ddns_status.get_server_ip, _ddns_status.save_ip))
			{
				_ddns_status.dns_error_count = 0;
				_ddns_status.force_update_count = 0;
				memset(_ddns_status.save_ip, 0x00, sizeof(_ddns_status.save_ip) );
				strncpy(_ddns_status.save_ip, _ddns_status.get_server_ip, sizeof(_ddns_status.save_ip) );
				goto ddns_update;
			}			
		}
		
		memset(_ddns_status.get_dns_ip, 0x00, sizeof(_ddns_status.get_dns_ip));
		g_printf("\n get_dns_ip_addr ====================>>> start \n\n");
		
		if( ddns_server_index == 3) // s1
			ret = get_dns_ip_addr_s1( _ddns_status.get_dns_ip, sizeof(_ddns_status.get_dns_ip) );
		else 
			ret = get_dns_ip_addr(_ddns_status.get_dns_ip, sizeof(_ddns_status.get_dns_ip), NULL, 0);
		
		g_printf("<<========= get_dns_ip_addr end!!! ret[%d] dns_e_count[%d]\n",ret,_ddns_status.dns_error_count);

		if(ret < 0)
		{
			ddns_set_status(DDNS2_RES_GET_DNSHOST_ERR);
			if(_ddns_status.dns_error_count > DNS_ERR_MAX)
			{
				_ddns_status.dns_error_count = 0;
				_ddns_status.force_update_count = 0;
				goto ddns_update;
			}
			else
			{
				_ddns_status.dns_error_count++;
   				_ddns_status.thread_sleep_sec = 60 * (5 + getip_err_wait_time);
				continue;
			}			
		}
		else
		{
			_ddns_status.dns_error_count = 0;
			//g_printf(" get_dns_ip[%s] force_update_count[%d]\n",_ddns_status.get_dns_ip, _ddns_status.force_update_count);
			if(!strcmp(_ddns_status.get_server_ip, _ddns_status.get_dns_ip))
			{
				ddns_set_status(1);				
				if(_ddns_status.force_update_count > ddns_force_update_cnt)
				{
					_ddns_status.force_update_count = 0;
					goto ddns_update;
				}
				else
				{
					_ddns_status.force_update_count++;
	   				_ddns_status.thread_sleep_sec = 60 * (2 + getip_err_wait_time);
					
					continue;
				}
			}
			else
			{
				g_printf("\n\n ddns update. dns_ip and get_ip different\n");
				_ddns_status.force_update_count = 0;
				goto ddns_update;
			}
		}
				
ddns_update:
		g_printf("\n ### ddns_update ==>>>> start \n\n");
		ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");
		g_return_if_fail(ddns != NULL);

		memset(tmp_ddns_svr, 0x00, sizeof(tmp_ddns_svr));

		snprintf(tmp_ddns_svr, sizeof(tmp_ddns_svr) -1 ,"%s", ddns);			
		my_str_trim(tmp_ddns_svr);

		if(ddns_server_index == 0)
		{
	        memset(&send_par, 0x00, sizeof(DDNS2_PARRAM));
	        ddns2_set_param(&send_par);
        
	        ret = send_ddns2_packet(&send_par);
		}
		else if(ddns_server_index == 1) // DYNDNS
		{
			memset(tmp_ddns_svr, 0x00, sizeof(tmp_ddns_svr));
			strcpy(tmp_ddns_svr,"members.dyndns.org");
			ret = _dyndns_send_ddns_packet( tmp_ddns_svr, _ddns_status.get_server_ip );	

		}
		else if(ddns_server_index == 2) // SGD
		{		
			memset(tmp_ddns_svr, 0x00, sizeof(tmp_ddns_svr));
			strcpy(tmp_ddns_svr,"ddns.fujiko.biz");

			ret = _fujikodns_send_ddns_packet( tmp_ddns_svr, _ddns_status.get_server_ip );		
		}	
		else if(ddns_server_index == 3) // s1
		{		
			ret = _s1_send_ddns_packet();			
		}
		else if(ddns_server_index == 4) // udrdns
		{		
			dau_ret = _dau_send_ddns_packet();
			if(dau_ret == 1)
				dau_retry = 0;
			else
				dau_retry = 3;

			dau_delay = 0;
			ret = dau_ret;
		}		
	
		_ddns_status.server_idx = ddns_server_index;		
		g_get_current_time( &_ddns_status.update_tv);		
		if(ret == 1)   // no change or regist ok
		{		
			g_printf("\n\n ddns_update success !!!!!!!!\n");			
			_ddns_status.thread_sleep_sec = 60 * (5 + getip_err_wait_time);  // dyndns 15 min. dvrlin 5 min . wait
			ddns_set_status(1);	// Success
			_ddns_status.update_error_count = 0;
			g_get_current_time( &_ddns_status.update_success_tv);
		
		}else{
		
			g_printf("\n\n ddns_update fail !!!!!!!! ret[%d]\n",ret);
			if(ret == DDNS2_RES_NOTACCEPT_MSG_ERR || ret == DDNS2_RES_HOST_CONFLICT_ERR)
				_ddns_status.thread_sleep_sec = 60 * 1440; // 1 day
			else if(ret == DYNDNS_RES_911_ERR || ret == DYNDNS_RES_DNSERR_ERR) // dyndns server error or dns server error
			{
				_ddns_status.update_error_count++;				
				if(_ddns_status.update_error_count > UPDATE_ERR_MAX)
					_ddns_status.thread_sleep_sec = 60 * (5 * (UPDATE_ERR_MAX + 5));  // max :  55 min 
				else
					_ddns_status.thread_sleep_sec = 60 * (5 * (_ddns_status.update_error_count + 5)); // min : 30 min
			}else{
				_ddns_status.update_error_count++;				
				if(_ddns_status.update_error_count > UPDATE_ERR_MAX)
					_ddns_status.thread_sleep_sec = 60 * (5 * (UPDATE_ERR_MAX + update_err_wait_time));  // todo_test 1 -> 5
				else
					_ddns_status.thread_sleep_sec = 60 * (5 * (_ddns_status.update_error_count + update_err_wait_time));
			}
			ddns_set_status(ret);
		}	
	}
	
	g_message("%s thread end", __FUNCTION__);
	pthread_exit(NULL);	
}


static void
_ddns_net_rxtx_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{	
	g_return_if_fail(pinfo != NULL);
/*
5) net_rxtx   - ��Ʈ��ũ Ʈ���� ����� 
   NF_NOTIFY_PARAM type�� NF_NOTIFY_INFO�� ��� 
      d.params[0]      netsvr status 100~0 
                  Ʈ������ ���������� ǥ���� 
                  (������ ����Ʈ ��Ʈ�� ������ Ȯ���ؾ߰���) 
      d.params[1]      netsvr rx    bytes/sec
      d.params[2]      netsvr tx    bytes/sec
      d.params[3]      link status  0:offline 1:online
*/

	if( _Link_Status !=  pinfo->d.params[3] ){				
		if( pinfo->d.params[3] ) {
			ddns_force_register();
		}else{
			g_message("%s ddns status -> fail", __FUNCTION__);
			_ddns_status.status = 0;
			g_printf("@@@@@@ _ddns_net_rxtx_cb_func call \n");
		}		
		_Link_Status = pinfo->d.params[3];				
	}
	
}
#ifdef	ENABLE_DDNS_SYSDB_RELOAD

static void
_ddns_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{	
	g_return_if_fail(pinfo != NULL);

	//g_message("%s param[%d]",__FUNCTION__, pinfo->d.params[0]);	
	if(pinfo->d.params[0] == NF_SYSDB_CATE_NET){
		int ddns_server_index, getip_err_wait_time, update_err_wait_time, ddns_force_update_cnt;
		char *ddns;

		ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");

		g_return_if_fail(ddns != NULL);

		_ddns_set_param( ddns, 
						&ddns_server_index,	
						&getip_err_wait_time, 
						&update_err_wait_time,
						&ddns_force_update_cnt);

		_ddns_status.server_idx = ddns_server_index;
		
//	    _ddns_status.status = 0;
		g_printf("@@@@@@ _ddns_sysdb_reload_cb_func call \n");
		ddns_force_register();	
	}
}

#endif

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int create_ddns(void)
{
	pthread_t tid;
	char *ddns = NULL;

	ddns = nf_sysdb_get_str_nocopy("net.proto.ddnssvr");	
	g_message("%s __%s__", __FUNCTION__, ddns);	

	memset( &_ddns_status, 0x00, sizeof( NF_DDNS_STATUS ));

{	
	gulong cb_handle = 0;		
	
	// or notify_get_param_3
	nf_netif_get_link_status(nf_netif_get_eth_str(), &_Link_Status );
		
	cb_handle= nf_notify_connect_cb( "net_rxtx", _ddns_net_rxtx_cb_func , (gpointer)NULL );
	g_message("%s net_rxtx connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);	
}

#ifdef	ENABLE_DDNS_SYSDB_RELOAD

{
	gulong cb_handle = 0;		
	cb_handle= nf_notify_connect_cb( "sysdb_change", _ddns_sysdb_reload_cb_func , (gpointer)NULL );
	g_message("%s sysdb_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);	
}

#endif

#if 0//defined(ENABLE_S1_DDNS_SCENARIO)
   if(Pthread_create(&tid, NULL, s1_ddns_func, NULL)) {	
		g_warning("%s : thread create error", __FUNCTION__);
		return -1;		  
   }
#else	
   if(Pthread_create(&tid, NULL, ddns_func, NULL)) {	
		g_warning("%s : thread create error", __FUNCTION__);
		return -1;		  
   }
#endif
   
   gServer_info.ddns_tid = tid;
   Pthread_detach(tid);

#ifdef _DEBUG_NET
   g_message("%s : thread created! [ddns_func]", __FUNCTION__);
#endif			  

   return 0;
}

