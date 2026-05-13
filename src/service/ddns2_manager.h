#ifndef _DDNS2_MANAGER_H_
#define _DDNS2_MANAGER_H_

typedef enum
{
  DDNS2_RES_OK_GOOD = 200,

  DDNS2_RES_BADREQUEST_ERR = 400,
  DDNS2_RES_UNAUTHORIZED_ERR = 401,  
  DDNS2_RES_NOTFOUND_ERR = 404,
  DDNS2_RES_NOTACCEPT_MSG_ERR = 406,
  DDNS2_RES_HOST_CONFLICT_ERR = 409,

  DDNS2_RES_INTERNEL_SERVER_ERR = 500,
  DDNS2_RES_SOCKET_CREATE_ERR = 501,
  DDNS2_RES_SERVER_CONNECT_ERR = 502,
  DDNS2_RES_DATA_SEND_ERR = 503,
  DDNS2_RES_DATA_SEND_NOT_REQUEST_ERR = 504,
  DDNS2_RES_RECV_ERR = 505,
  DDNS2_RES_RECV_IP_ERR = 506,
  DDNS2_RES_GET_DNSHOST_ERR = 550,

  S1_DDNS_ERR_NETORK = 600,
  S1_DDNS_ERR_EXTERN_IP = 601,
  S1_DDNS_ERR_UPDATE = 602,
  S1_DDNS_STOP = 603,

}ddns2_res_type;

typedef struct _NF_DDNS2_PARAM_T
{
  char owner_name[128];
  char host_name[128];
  int  video_type;
  int  disk_count;
  unsigned long long disk_size;
  unsigned int disk_filled;
  char lang[32];
}DDNS2_PARRAM;

void ddns2_set_param(DDNS2_PARRAM *send_par);
int nf_ddns2_register(DDNS2_PARRAM *send_par);
int send_ddns2_packet(DDNS2_PARRAM *send_par);
int nf_ddns_itx_get_myip(char *get_myIp_str);
int nf_ddns_itx_get_myip2(char *get_myIp_str);

typedef enum _UNIMO_CHECK_RET_MSG {
	UNIMO_CHECK_RES_SUCCESS = 0,
	UNIMO_CHECK_RES_INITIALIZE,
	UNIMO_CHECK_RES_INTERNAL_ERR,
	UNIMO_CHECK_RES_NETWORK_ERR,
	UNIMO_CHECK_RES_DATA_SEND_ERR,
	UNIMO_CHECK_RES_DATA_RECV_ERR,
	UNIMO_CHECK_RES_FAIL_ERR,
	UNIMO_CHECK_RES_BADAUTH_ERR,
	UNIMO_CHECK_RES_UNKNOWN_ERR,
	UNIMO_CHECK_RES_MAX,
}UNIMO_CHECK_RET_MSG;

int nf_ddns_get_unimo_checker_status(void);

#endif
