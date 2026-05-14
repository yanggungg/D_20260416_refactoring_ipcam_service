/**
 * @file nf_onvif_device.c
 * @brief ONVIF DEVICE 구현
 * @author jykim
 * @date 2012-01-12
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_ONVIF_DEVICE_C__
#define __NF_ONVIF_DEVICE_C__

#include <stdsoap2.h>
#include <onvifH.h>

#include <nf_ipcam_defs.h>
#include <nf_api_openmode.h>

extern gint nf_get_running_mode(void);
extern dtable* get_dtable(void);
extern int nf_ipcam_is_vendor_s1(void);
extern int SOAP_SSL_CLIENT_CONTEXT(struct soap* soap, unsigned short a, const char* b, const char* c, const char* d, const char* e, const char* f);

#if 1
#define NF_ONVIF_DBG printf
#else
#define NF_ONVIF_DBG while(0) printf
#endif
#define D_SECS(a) ((a).tv_sec),(((a).tv_nsec)/1000)

#ifdef DUAL_LAN_NETWORK
#include <linux/if.h>
int connect_nonb(int sockfd, const struct sockaddr *saptr, int salen, int nsec)
{
	int             flags, n, error;
	socklen_t       len;
	fd_set          rset, wset;
	struct timeval  tval;

	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return(-1);

	/* Do whatever we want while the connect is taking place. */

	if (n == 0)
		goto done;  /* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd+1, &rset, &wset, NULL,
					nsec ? &tval : NULL)) == 0) {
		close(sockfd);      /* timeout */
		errno = ETIMEDOUT;
		return(-1);
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return(-1);         /* Solaris pending error */
	} else
		printf("select error: sockfd not set\n");

done:
	fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
	if (error) {
		close(sockfd);      /* just in case */
		errno = error;
		return(-1);
	}
	return(0);
}

SOAP_SOCKET lan_open(struct soap *soap, const char *endpoint, const char *host, int port) 
{
	int sock;
	int rtn;
	struct ifreq ifr;
	struct sockaddr_in serv_addr;
	sock = socket(AF_INET, SOCK_STREAM, 0);

	//printf("[%s:%d] host(%s) endpoint(%s) port(%d) \n", __FUNCTION__, __LINE__, host, endpoint, port);

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HUB_ETH_DEVICE);
	if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 

	inet_pton(AF_INET, host, &serv_addr.sin_addr);

	rtn = connect_nonb(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr), 5);
	soap->errnum = soap_socket_errno(soap->socket);
	if(rtn != 0)
	{
		soap->socket = SOAP_INVALID_SOCKET;
		soap->sendfd = SOAP_INVALID_SOCKET;
		soap->recvfd = SOAP_INVALID_SOCKET;
		soap->error = SOAP_TCP_ERROR;
		return SOAP_INVALID_SOCKET;
	}

	soap->recvfd = soap->sendfd = soap->socket = sock;
	return soap->socket;
}
#endif

const char* _NF_TF_STR[] = { "[false]", "[true]" };
const char* _NF_AUTH_STR[] = { "NONE", "TEXT", "DIGEST" };

#define SERVICE_SIZE (8)
static char service_namespace[SERVICE_SIZE][128] = {
	"http://www.onvif.org/ver10/device/wsdl",
	"http://www.onvif.org/ver10/media/wsdl",
	"http://www.onvif.org/ver20/media/wsdl",
	"http://www.onvif.org/ver20/imaging/wsdl",
	"http://www.onvif.org/ver10/events/wsdl",
	"http://www.onvif.org/ver20/analytics/wsdl",
	"http://www.onvif.org/ver20/ptz/wsdl",
	NULL
};

ONVIF_MSG _dev_get_services_response_parser(struct _device__GetServicesResponse *res, char *device, char *media, char *image, char *event, char *ptz, char *analytics, char *media2);
ONVIF_MSG _nf_onvif_dev_get_services(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, char* device, char* media, char* image, char* event, char* ptz, char* analytics, char* media2);
/* Static msg exchange funcs */
ONVIF_MSG _nf_onvif_dev_get_media_xaddr(
	int,
	const char*, const char*, const char*,
	char*
);
ONVIF_MSG _nf_onvif_dev_get_service_addrs(
	ipcam_onvif_auth_info_t* auth_info,
	const char *endpoint, char* device, char* media, char* image, char* event, char* ptz, char* analytics,
	NFIPCamAuxiliary *auxiliary
);
ONVIF_MSG _nf_onvif_dev_get_network_interfaces_token(
	int,
	const char*, const char*, const char*,
	char*, enum xsd__boolean*, int
);
ONVIF_MSG _nf_onvif_dev_set_network_interfaces(
	int,
	const char*, const char*, const char*,
	char*, const unsigned int
);
ONVIF_MSG _nf_onvif_dev_set_network_dhcp(
	int,
	const char*, const char*, const char*,
	char*, int
);
ONVIF_MSG _nf_onvif_dev_system_reboot(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, int interface
);
ONVIF_MSG _nf_onvif_dev_get_device_info(
	int ch,
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint,
	cam_model_info *rtn_info,
	int interface
);
ONVIF_MSG _nf_onvif_dev_create_user(
    ipcam_onvif_auth_info_t* auth_info,
    const char *new_user,
    const char *new_pass,
    const char *new_level,
    const char *endpoint
);
ONVIF_MSG _nf_onvif_dev_set_network_default_gateway(
	int,
	const char*, const unsigned int
);
ONVIF_MSG _nf_onvif_dev_get_netinfo(
	int, const char*, const char*, const char*, unsigned char*, NFOpenmodeSetupNetwork*
);
ONVIF_MSG _nf_onvif_dev_get_default_gateway(
	int, const char*, const char*, const char*, NFOpenmodeSetupNetwork*
);
ONVIF_MSG _nf_onvif_dev_get_dns(
	int, const char*, const char*, const char*, NFOpenmodeSetupNetwork*
);
ONVIF_MSG _nf_onvif_dev_set_user(
	int, const char*, const char*, const char*, const char*
);

ONVIF_MSG _nf_onvif_dev_get_capabilities(ipcam_onvif_auth_info_t* auth_info,	const char *endpoint, onvif_service_t *service);
ONVIF_MSG _nf_onvif_dev_send_auxiliary_command(int auth, const char* endpoint, const char* user, const char* pass, char *auxiliary_command);

static int _url_change_host(char *url, const char *host);

/* -------------   ONVIF related APIs for the IPX scenario   --------------- */
ONVIF_API nf_onvif_set_user(
	unsigned int ipaddr,
	unsigned short port,
	char *u,
	char *p,
	char *new_p
)
{
	char endpoint[256];
	struct timespec now_time;
	int rtn;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(%d.%d.%d.%d:%d) (%s:%s->%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			(ipaddr&0xff),
			(ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16,
			(ipaddr&0xff000000)>>24,
			port,
			//u,p,new_p
			u,"****", "****"
			);

	memset(endpoint, 0x00, 256);
	snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
			(ipaddr&0xff), (ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24,
			port
	);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__, endpoint
			);

	rtn = _nf_onvif_dev_set_user(NF_ONVIF_AUTH_DIGEST, endpoint, u, p, new_p);
	if (rtn != 0)
	{
		rtn = _nf_onvif_dev_set_user(NF_ONVIF_AUTH_TEXT, endpoint, u, p, new_p);
	}

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_get_net_info(
	unsigned int ipaddr,
	unsigned short port,
	int auth,
	int use_ssl,
	char *u,
	char *p,
	unsigned char *rtn_info,
	NFOpenmodeSetupNetwork* netinfo
)
{
	char endpoint[256];
	struct timespec now_time;
	int rtn;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(%d.%d.%d.%d:%d) %s %s/%s\n",
			D_SECS(now_time),
			__FUNCTION__,
			(ipaddr&0xff),
			(ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16,
			(ipaddr&0xff000000)>>24,
			port,
			_NF_AUTH_STR[auth],
			//u,p
			u,"****"
			);

	memset(endpoint, 0x00, 256);
	if (use_ssl == 1)
	{
		snprintf(endpoint, 256, "https://%d.%d.%d.%d:%d/onvif/device_service",
				(ipaddr&0xff), (ipaddr&0xff00)>>8,
				(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24,
				port
		);
	}
	else
	{
		snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
				(ipaddr&0xff), (ipaddr&0xff00)>>8,
				(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24,
				port
		);
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__, endpoint
			);

	rtn = _nf_onvif_dev_get_netinfo(auth, endpoint, u, p, rtn_info, netinfo);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

	rtn = _nf_onvif_dev_get_default_gateway(auth, endpoint, u, p, netinfo);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

	rtn = _nf_onvif_dev_get_dns(auth, endpoint, u, p, netinfo);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}
ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_get_dev_info_raw(
	unsigned int ipaddr,
	unsigned short port,
	int auth,
	int use_ssl,
	char *u,
	char *p,
	cam_model_info *rtn_info
)
{
	char endpoint[256];
	struct timespec now_time;
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(%d.%d.%d.%d:%d) %s %s/%s\n",
			D_SECS(now_time),
			__FUNCTION__,
			(ipaddr&0xff),
			(ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16,
			(ipaddr&0xff000000)>>24,
			port,
			_NF_AUTH_STR[auth],
			//u,p
			u,"****"
			);

	memset(endpoint, 0x00, 256);
	if (use_ssl == 1)
	{
		snprintf(endpoint, 256, "https://%d.%d.%d.%d:%d/onvif/device_service",
				(ipaddr&0xff), (ipaddr&0xff00)>>8,
				(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24,
				((port == 0) ? 443:port)
		);
	}
	else
	{
		snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
				(ipaddr&0xff), (ipaddr&0xff00)>>8,
				(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24,
				((port == 0) ? 80:port)
		);
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__, endpoint
			);

	auth_info.auth_method = auth;
	auth_info.username = u;
	auth_info.password = p;
	auth_info.endpoint = endpoint;

	rtn = _nf_onvif_dev_get_device_info(
			(-1), &auth_info, endpoint, rtn_info, 0);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_get_dev_info(int ch, int interface)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	struct timespec now_time;
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH %d) interface(%d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch, interface
			);
	g_return_val_if_fail(runtime != NULL, (-1));


	memset(endpoint, 0x00, 256);
	if (nf_ipcam_is_vendor_s1())
	{
		snprintf(endpoint, 256, "https://%d.%d.%d.%d:%d/onvif/device_service",
				(discovery[ch].ipaddr&0xff),
				(discovery[ch].ipaddr&0xff00)>>8,
				(discovery[ch].ipaddr&0xff0000)>>16,
				(discovery[ch].ipaddr&0xff000000)>>24,
				((runtime[ch].sys.http_port==0)?443:runtime[ch].sys.http_port)
				);
	}
	else
	{
		snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
				(discovery[ch].ipaddr&0xff),
				(discovery[ch].ipaddr&0xff00)>>8,
				(discovery[ch].ipaddr&0xff0000)>>16,
				(discovery[ch].ipaddr&0xff000000)>>24,
				((runtime[ch].sys.http_port==0)?80:runtime[ch].sys.http_port)
				);
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__, endpoint
			);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = endpoint;

	rtn = _nf_onvif_dev_get_device_info(
			ch, &auth_info,	endpoint, NULL, interface);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_reboot_request(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));


	memset(endpoint, 0x00, 256);
	snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
			(runtime[ch].sys.ipaddr&0xff),
			(runtime[ch].sys.ipaddr&0xff00)>>8,
			(runtime[ch].sys.ipaddr&0xff0000)>>16,
			(runtime[ch].sys.ipaddr&0xff000000)>>24,
			runtime[ch].sys.http_port
			);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = endpoint;
	
	rtn = _nf_onvif_dev_system_reboot(
			&auth_info,	endpoint, 0);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: reboot REQ fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}


ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_get_capabilities(int ch, onvif_service_t *srv)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));


	memset(endpoint, 0x00, 256);
	snprintf(endpoint, 256, "http://%d.%d.%d.%d:%d/onvif/device_service",
			(runtime[ch].sys.ipaddr&0xff),
			(runtime[ch].sys.ipaddr&0xff00)>>8,
			(runtime[ch].sys.ipaddr&0xff0000)>>16,
			(runtime[ch].sys.ipaddr&0xff000000)>>24,
			runtime[ch].sys.http_port
			);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = endpoint;

	rtn = _nf_onvif_dev_get_capabilities(
			&auth_info,	endpoint, srv);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: reboot REQ fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}


ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_create_user(int ch, const char *new_user, const char *new_pass, const char *new_level)
{
    char                    endpoint[256]   = { 0, };
    mtable                  *runtime        = get_runtime();
    dtable                  *discovery      = get_dtable();
    int                     rtn             = -1;
    struct timespec         now_time;
    ipcam_onvif_auth_info_t auth_info;

    g_return_val_if_fail(runtime   != NULL, (-1));
    g_return_val_if_fail(discovery != NULL, (-1));

    memset(endpoint,   0x00, sizeof(endpoint));
    memset(&now_time,  0x00, sizeof(now_time));
    memset(&auth_info, 0x00, sizeof(auth_info));

    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH %d)\n",
                 D_SECS(now_time), __FUNCTION__, ch);

    if (nf_ipcam_is_vendor_s1())
    {
        snprintf(endpoint, sizeof(endpoint), "https://%d.%d.%d.%d:%d/onvif/device_service",
                (discovery[ch].ipaddr&0xff),
                (discovery[ch].ipaddr&0xff00)>>8,
                (discovery[ch].ipaddr&0xff0000)>>16,
                (discovery[ch].ipaddr&0xff000000)>>24,
                ((runtime[ch].sys.http_port==0)?443:runtime[ch].sys.http_port)
                );
    }
    else
    {
        snprintf(endpoint, sizeof(endpoint), "http://%d.%d.%d.%d:%d/onvif/device_service",
                (discovery[ch].ipaddr&0xff),
                (discovery[ch].ipaddr&0xff00)>>8,
                (discovery[ch].ipaddr&0xff0000)>>16,
                (discovery[ch].ipaddr&0xff000000)>>24,
                ((runtime[ch].sys.http_port==0)?80:runtime[ch].sys.http_port)
                );
    }

    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
                 D_SECS(now_time), __FUNCTION__, endpoint);

    auth_info.auth_method   = runtime[ch].onvif.auth_method;
    auth_info.username      = runtime[ch].username;
    auth_info.password      = runtime[ch].password;
    auth_info.endpoint      = endpoint;

    rtn = _nf_onvif_dev_create_user(&auth_info, new_user, new_pass, new_level, endpoint);

    if (rtn != 0)
    {
        clock_gettime(CLOCK_REALTIME, &now_time);
        NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: rpc returns fail(%d)\n",
                     D_SECS(now_time), __FUNCTION__, rtn);
        goto ends_label;
    }

ends_label:
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
                 D_SECS(now_time), __FUNCTION__);
    return rtn;
}

ONVIF_API nf_onvif_get_media_xaddr(
	unsigned int ip,
	unsigned short port,
	char* tail,
	int auth,
	int use_ssl,
	char* u,
	char* p,
	char* rtn_buf
)
{
	int i = 0;
	int rtn = 0;
	char endpoint[512];
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(%d.%d.%d.%d:%d/%s %s %s %s\n",
			D_SECS(now_time),
			__FUNCTION__,
			(ip&0xff),
			(ip&0xff00)>>8,
			(ip&0xff0000)>>16,
			(ip&0xff000000)>>24,
			port, tail, _NF_AUTH_STR[auth], u,"****"//p
			);

	if (use_ssl == 1)
	{
		snprintf(endpoint, 512, "https://%d.%d.%d.%d:%d/%s",
				(ip&0xff),
				(ip&0xff00)>>8,
				(ip&0xff0000)>>16,
				(ip&0xff000000)>>24,
				port,
				"onvif/device_service");
	}
	else
	{
		snprintf(endpoint, 512, "http://%d.%d.%d.%d:%d/%s",
				(ip&0xff),
				(ip&0xff00)>>8,
				(ip&0xff0000)>>16,
				(ip&0xff000000)>>24,
				port,
				"onvif/device_service");
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);

	rtn = _nf_onvif_dev_get_media_xaddr(
			auth,
			endpoint,
			u, p, rtn_buf
			);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | WARN: service addr get fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_get_service_capabilities(int ch)
{
	int i = 0;
	int rtn = 0;
	int rtn2 = 0;
	unsigned int ip_to_set = 0;
	char token_to_set[128];
	char endpoint[512];
	mtable *runtime = NULL;
	onvif_t *onvif = NULL;
	struct timespec now_time;

	char ip_buf[20];

	ipcam_onvif_auth_info_t auth_info;
	
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	onvif = &runtime[ch].onvif;

	if (nf_ipcam_is_vendor_s1())
	{
		snprintf(endpoint, 512, "https://%d.%d.%d.%d:%d/%s",
				 runtime[ch].sys.ipaddr&0xff,
				(runtime[ch].sys.ipaddr&0xff00)>>8,
				(runtime[ch].sys.ipaddr&0xff0000)>>16,
				(runtime[ch].sys.ipaddr&0xff000000)>>24,
				runtime[ch].sys.http_port,
				onvif->xaddr_dev_tail);
	}
	else
	{
		snprintf(endpoint, 512, "http://%d.%d.%d.%d:%d/%s",
				 runtime[ch].sys.ipaddr&0xff,
				(runtime[ch].sys.ipaddr&0xff00)>>8,
				(runtime[ch].sys.ipaddr&0xff0000)>>16,
				(runtime[ch].sys.ipaddr&0xff000000)>>24,
				runtime[ch].sys.http_port,
				onvif->xaddr_dev_tail);
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = endpoint;

	rtn = _nf_onvif_dev_get_service_addrs(
			&auth_info,	
			endpoint,
			onvif->xaddr[NF_ONVIF_SERVICE_DEVICE],
			onvif->xaddr[NF_ONVIF_SERVICE_MEDIA],
			onvif->xaddr[NF_ONVIF_SERVICE_IMAGE],
			onvif->xaddr[NF_ONVIF_SERVICE_EVENT],
			onvif->xaddr[NF_ONVIF_SERVICE_PTZ],
			onvif->xaddr[NF_ONVIF_SERVICE_ANALYTICS],
			&runtime[ch].onvif.auxiliary
			);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | WARN: service addr get fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		if (runtime[ch].onvif.auth_method == NF_ONVIF_AUTH_TEXT)
		{
			runtime[ch].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
			rtn = _nf_onvif_dev_get_service_addrs(
					&auth_info,
					endpoint,
					onvif->xaddr[NF_ONVIF_SERVICE_DEVICE],
					onvif->xaddr[NF_ONVIF_SERVICE_MEDIA],
					onvif->xaddr[NF_ONVIF_SERVICE_IMAGE],
					onvif->xaddr[NF_ONVIF_SERVICE_EVENT],
					onvif->xaddr[NF_ONVIF_SERVICE_PTZ],
					onvif->xaddr[NF_ONVIF_SERVICE_ANALYTICS],
					&runtime[ch].onvif.auxiliary
					);
		}
	}

	// MEDIA2 ���� �� ���� LOGIN FAIL� �� ������ �� ��
	// _nf_onvif_dev_get_service_addrs � _nf_onvif_dev_get_services � return �� ��
	rtn2 = _nf_onvif_dev_get_services(
			&auth_info,	
			endpoint,
			onvif->xaddr[NF_ONVIF_SERVICE_DEVICE],
			onvif->xaddr[NF_ONVIF_SERVICE_MEDIA],
			onvif->xaddr[NF_ONVIF_SERVICE_IMAGE],
			onvif->xaddr[NF_ONVIF_SERVICE_EVENT],
			onvif->xaddr[NF_ONVIF_SERVICE_PTZ],
			onvif->xaddr[NF_ONVIF_SERVICE_ANALYTICS],
			onvif->xaddr[NF_ONVIF_SERVICE_MEDIA2]);

	if (rtn2 != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | WARN: service addr get fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		if (runtime[ch].onvif.auth_method == NF_ONVIF_AUTH_TEXT)
		{
			runtime[ch].onvif.auth_method = NF_ONVIF_AUTH_DIGEST;
			rtn = _nf_onvif_dev_get_services(
					&auth_info,	
					endpoint,
					onvif->xaddr[NF_ONVIF_SERVICE_DEVICE],
					onvif->xaddr[NF_ONVIF_SERVICE_MEDIA],
					onvif->xaddr[NF_ONVIF_SERVICE_IMAGE],
					onvif->xaddr[NF_ONVIF_SERVICE_EVENT],
					onvif->xaddr[NF_ONVIF_SERVICE_PTZ],
					onvif->xaddr[NF_ONVIF_SERVICE_ANALYTICS],
					onvif->xaddr[NF_ONVIF_SERVICE_MEDIA2]);
		}
	}

	if(strlen(onvif->xaddr[NF_ONVIF_SERVICE_MEDIA2]) > 0)
	{
		runtime[ch].onvif.onvif_service |= __OFM(NF_ONVIF_SERVICE_MEDIA2);
	}

	for(i = 0; i < NF_ONVIF_SERVICE_NR; i++){
	    _ip_to_str(htonl(runtime[ch].sys.ipaddr), ip_buf);
	    printf("[%s:%d] ch[%d] ip[%s] host[%s]\n", __func__, __LINE__, ch, ip_buf, onvif->xaddr[i]);
	    _url_change_host(onvif->xaddr[i], ip_buf);
	}

	// xiongmai bundle camera��PTZ �������������� PTZ service������ ������ �߻��Ͽ� �ӽ���ġ��
	// jykim 2015.7.28
	if (strcmp(runtime[ch].sys.vendor, "H264") == 0)
	{
		memset(&onvif->xaddr[NF_ONVIF_SERVICE_PTZ], 0x00, 128);
	}
	if (rtn != 0)
	{
		if (nf_get_running_mode() == 0)
		{
			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: service addr get fail again(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					rtn
					);
			nf_pnd_queue_push(ch, IPCAM_EVENT_LOGIN_FAIL, __LINE__, __FILE__);
		}
		goto ends_label;
	}

	onvif->onvif_service = 0;
	for (i = NF_ONVIF_SERVICE_DEVICE; i < NF_ONVIF_SERVICE_NR; i++)
	{
		// xiongmai bundle camera��PTZ �������������� PTZ service������ ������ �߻��Ͽ� �ӽ���ġ��
		// jykim 2015.7.28
		if (strcmp(runtime[ch].sys.vendor, "H264") == 0)
		{
			if (i==NF_ONVIF_SERVICE_PTZ) continue;
		}

		if (nf_ipcam_is_vendor_s1())
		{
			if (strstr(onvif->xaddr[i], "https://") != NULL)
			{
				onvif->onvif_service |= __OFM(i);
			}
		}
		else
		{
			if (strstr(onvif->xaddr[i], "http://") != NULL)
			{
				onvif->onvif_service |= __OFM(i);
			}
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | Support mask result(%08x)\n",
			D_SECS(now_time),
			__FUNCTION__,
			onvif->onvif_service
			);

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_API nf_onvif_device_set_dhcp(int ch, int interface)
{
	int i = 0;
	int rtn = 0;
	int auth = 0;
	unsigned int ip_to_set = 0;
	enum xsd__boolean tf = 0;
	char token_to_set[128];
	char endpoint[512];
	char *username;
	char *password;
	mtable *runtime = NULL;
	dtable *discovery = NULL;
	struct timespec now_time;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH:%d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);

	discovery = get_dtable();
	g_return_val_if_fail(discovery != NULL, (-1));
	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	username = runtime[ch].username;
	password = runtime[ch].password;

	if (nf_ipcam_is_vendor_s1())
	{
		snprintf(endpoint, 512, "https://%d.%d.%d.%d:%d/%s",
				 discovery[ch].ipaddr&0xff,
				(discovery[ch].ipaddr&0xff00)>>8,
				(discovery[ch].ipaddr&0xff0000)>>16,
				(discovery[ch].ipaddr&0xff000000)>>24,
				runtime[ch].sys.http_port,
				runtime[ch].onvif.xaddr_dev_tail);
	}
	else
	{
		snprintf(endpoint, 512, "http://%d.%d.%d.%d:%d/%s",
				 discovery[ch].ipaddr&0xff,
				(discovery[ch].ipaddr&0xff00)>>8,
				(discovery[ch].ipaddr&0xff0000)>>16,
				(discovery[ch].ipaddr&0xff000000)>>24,
				runtime[ch].sys.http_port,
				runtime[ch].onvif.xaddr_dev_tail);
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);

	memset(token_to_set, 0x00, 128);
	if (runtime[ch].onvif.auth_method == NF_ONVIF_AUTH_NONE)
	{
		auth = NF_ONVIF_AUTH_NONE;
		rtn = _nf_onvif_dev_get_network_interfaces_token(
				NF_ONVIF_AUTH_NONE,
				endpoint,
				"", "",
				token_to_set,
				&tf,
				interface
				);
		if (rtn != 0)
		{
			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | Token failed(%d)\n",
					D_SECS(now_time), __FUNCTION__, rtn);
			return rtn;
		}
	}
	else
	{
		auth = NF_ONVIF_AUTH_DIGEST;
		rtn = _nf_onvif_dev_get_network_interfaces_token(
				NF_ONVIF_AUTH_DIGEST,
				endpoint,
				username, password,
				token_to_set,
				&tf,
				interface
				);

		if (rtn != 0)
		{
			auth = NF_ONVIF_AUTH_TEXT;
			rtn = _nf_onvif_dev_get_network_interfaces_token(
					NF_ONVIF_AUTH_TEXT,
					endpoint,
					username, password,
					token_to_set,
					&tf,
					interface
					);
		}

		if (rtn != 0)
		{
			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | Token failed(%d)\n",
					D_SECS(now_time), __FUNCTION__, rtn);
			return rtn;
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | Interface token(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			token_to_set
			);

	rtn = _nf_onvif_dev_set_network_dhcp(
		auth, endpoint, username, password, token_to_set, interface);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | ip setup result(%d)\n",
			D_SECS(now_time),
			__FUNCTION__,
			rtn
			);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];


	if (rtn == NF_ONVIF_REBOOTING || rtn == 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | reboot request(ch#%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				ch
				);
		_nf_onvif_dev_system_reboot(&auth_info, endpoint, interface);
		rtn = 0;
	}

	return rtn;
}

ONVIF_API nf_onvif_change_ip(int ch)
{
	int i = 0;
	int rtn = 0;
	unsigned int ip_to_set = 0;
	unsigned int gw_to_set = 0;
	unsigned int same_ip_port = 0;
	enum xsd__boolean tf = 0;
	char token_to_set[128];
	char endpoint[512];
	mtable *runtime = NULL;
	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start(CH:%d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_DEVICE), (-1));

	NF_ONVIF_DBG("[%lu.%06lu] %s | sp_mask(%08x)\n",
			D_SECS(now_time), __FUNCTION__, same_ip_port);

	snprintf(endpoint, 512, "http://%d.%d.%d.%d:%d/%s",
			 runtime[ch].sys.ipaddr&0xff,
			(runtime[ch].sys.ipaddr&0xff00)>>8,
			(runtime[ch].sys.ipaddr&0xff0000)>>16,
			(runtime[ch].sys.ipaddr&0xff000000)>>24,
			runtime[ch].sys.http_port,
			runtime[ch].onvif.xaddr_dev_tail);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	memset(token_to_set, 0x00, 128);
	rtn = _nf_onvif_dev_get_network_interfaces_token(
			runtime[ch].onvif.auth_method,
			endpoint,
			runtime[ch].username, runtime[ch].password,
			token_to_set,
			&tf,
			0
			);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | network interface token get fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				rtn
				);
		goto ends_label;
	}

	rtn = _nf_onvif_dev_set_network_dhcp(
		runtime[ch].onvif.auth_method,
		endpoint, runtime[ch].username, runtime[ch].password, token_to_set, 0);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | ip setup result(%d)\n",
			D_SECS(now_time),
			__FUNCTION__,
			rtn
			);

#if 1
	if (rtn == NF_ONVIF_REBOOTING)
	{
		runtime[ch].onvif.waiting = 120;
	}
	else if (rtn == 0)
	{
		runtime[ch].onvif.waiting = 20;
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ip setup failed\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		goto ends_label;
	}
#else
	if (rtn == NF_ONVIF_REBOOTING)
	{
		//nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);
	}
#endif

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}


ONVIF_API nf_onvif_device_send_auxiliary_command(int ch, char* command)
{
	int rtn;
	char endpoint[256];
	char auxiliary_command[32];
	mtable *runtime = get_runtime();

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);

	strncpy(auxiliary_command, command, 32);

	rtn = _nf_onvif_dev_send_auxiliary_command(runtime[ch].onvif.auth_method, endpoint, runtime[ch].username, runtime[ch].password, &auxiliary_command);

	if(rtn != 0)
	{
		NF_ONVIF_DBG("[%s] faild\n", __func__);
	}

	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/* XXX-------------   ONVIF MSG exchanging methods - Internal use   --------------- */




/* -------------   ONVIF MSG exchanging methods - Internal use   --------------- */
ONVIF_MSG _nf_onvif_dev_get_media_xaddr(
	int auth,
	const char* endpoint, const char* user, const char* pass,
	char* rtn_buf
)
{
	int rtn = 0;
	struct soap *soap;
	struct _device__GetCapabilities req;
	struct _device__GetCapabilitiesResponse res;
	struct tt__Capabilities *cap;
	struct timespec now_time;


	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	g_return_val_if_fail(rtn_buf != NULL, (-1));

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	req.__sizeCategory = 1;
	req.Category = soap_malloc(soap, sizeof(enum tt__CapabilityCategory));
	req.Category[0] = tt__CapabilityCategory__Media;
	rtn = soap_call___device__GetCapabilities(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

	cap = res.Capabilities;
	if (cap->Media != NULL)
	{
		char *http_ptr;
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Media service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, cap->Media->XAddr
				);
		strncpy(rtn_buf, cap->Media->XAddr, 128);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end - %s\n",
			D_SECS(now_time),
			__FUNCTION__, rtn_buf
			);

	return rtn;
}

static void _get_address_head(char* addr, char* head)
{
	gchar *s, *e;
	const gchar *find_https = "https://";
	const gchar *find_http = "http://";
	gchar *find_str = NULL;

	if (addr == NULL) { return ; }
	if (head == NULL) { return ; }

	if (nf_ipcam_is_vendor_s1())
	{
		find_str = find_https;
	}
	else
	{
		find_str = find_http;
	}

	s = addr;
	e = strstr(addr, find_str);
	if (e == NULL)
	{
		return ;
	}
	e += strlen(find_str);
	e = strstr(e, "/");
	if (e == NULL)
	{
		return ;
	}
	e++;

	memset(head, 0x00, 128);
	strncpy(head, s, (e-s));
}

static void _get_address_tail(char* addr, char* tail)
{
	gchar *s, *e;
	const gchar *find_https = "https://";
	const gchar *find_http = "http://";
	gchar *find_str = NULL;

	if (addr == NULL) { return ; }
	if (tail == NULL) { return ; }

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
		return ;
	}
	s += strlen(find_str);
	s = strstr(s, "/");
	if (s == NULL)
	{
		return ;
	}
	s++;

	strcpy(tail, s);
}

ONVIF_MSG _nf_onvif_dev_get_service_addrs
(
	ipcam_onvif_auth_info_t* auth_info,
	const char *endpoint, char* device, char* media, char* image, char* event, char* ptz, char* analytics,
	NFIPCamAuxiliary *auxiliary
)
{
	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(device != NULL, (-1));
	g_return_val_if_fail(media != NULL, (-1));
	g_return_val_if_fail(image != NULL, (-1));
	g_return_val_if_fail(event != NULL, (-1));
	g_return_val_if_fail(ptz != NULL, (-1));
	g_return_val_if_fail(analytics != NULL, (-1));

	int i = 0, j = 0;
	int rtn = 0;
	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetCapabilities req;
	struct _device__GetCapabilitiesResponse res;
	struct tt__Capabilities *cap;
	struct timespec now_time;
	char *http_ptr;
	char addr_head[128];


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]
				);
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	}
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
#if 0
	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenText(soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		cam_time = _nf_onvif_get_time(endpoint);
		if (cam_time != 0)
		{
			rtn = soap_wsse_add_UsernameTokenDigestWithManulTime(soap, NULL, user, pass, cam_time);
		}
		else
		{
			rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
		}
	}
#endif
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(auxiliary, 0x00, sizeof(NFIPCamAuxiliary));

	req.__sizeCategory = 1;
	req.Category = soap_malloc(soap, sizeof(enum tt__CapabilityCategory));
	req.Category[0] = tt__CapabilityCategory__All;
	rtn = soap_call___device__GetCapabilities(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	if (res.Capabilities == NULL)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC response MSG wrong\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = (-1);
		goto ends_label;
	}

	_get_address_head(endpoint, addr_head);

	cap = res.Capabilities;
	if (cap->Device != NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Device service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->Device->XAddr
				);
		_get_address_tail(cap->Device->XAddr, addr_tail);
		memset(device, 0x00, 128);
		strncpy(device, addr_head, strlen(addr_head));
		strcat(device, addr_tail);
		//strncpy(device, cap->Device->XAddr, 128);
		
		// Auxiliary Command Search
		if(cap->Device->IO != NULL && cap->Device->IO->Extension != NULL)
		{
			struct tt__IOCapabilitiesExtension *io;
			io = cap->Device->IO->Extension;
			if(io->Auxiliary != NULL && *(io->Auxiliary) == xsd__boolean__true_)
			{
				if( io->__sizeAuxiliaryCommands > 0  )
				{
					int size;
					size = io->__sizeAuxiliaryCommands;
					auxiliary->size = size;

					if( MAX_AUXILIARY_COMMANDS <= io->__sizeAuxiliaryCommands)
					{
						size = MAX_AUXILIARY_COMMANDS;
					}

					for(i = 0; i < size; i++)
					{
						strncpy(auxiliary->commands[i], io->AuxiliaryCommands[i], 32);
					}

				}

			}
		}
	}

	if (cap->Media != NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Media service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->Media->XAddr
				);
		_get_address_tail(cap->Media->XAddr, addr_tail);
		memset(media, 0x00, 128);
		strncpy(media, addr_head, strlen(addr_head));
		strcat(media, addr_tail);
		//strncpy(media, cap->Media->XAddr, 128);
	}
	if (cap->Imaging != NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Imaging service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->Imaging->XAddr
				);
		_get_address_tail(cap->Imaging->XAddr, addr_tail);
		memset(image, 0x00, 128);
		strncpy(image, addr_head, strlen(addr_head));
		strcat(image, addr_tail);
		//strncpy(image, cap->Imaging->XAddr, 128);
	}
	if (cap->Events != NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Event service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->Events->XAddr
				);
		_get_address_tail(cap->Events->XAddr, addr_tail);
		memset(event, 0x00, 128);
		strncpy(event, addr_head, strlen(addr_head));
		strcat(event, addr_tail);
		//strncpy(event, cap->Events->XAddr, 128);
	}
	if (cap->PTZ!= NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | PTZ service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->PTZ->XAddr
				);
		_get_address_tail(cap->PTZ->XAddr, addr_tail);
		memset(ptz, 0x00, 128);
		strncpy(ptz, addr_head, strlen(addr_head));
		strcat(ptz, addr_tail);
		//strncpy(ptz, cap->PTZ->XAddr, 128);
	}
	if(cap->Analytics != NULL)
	{
		char addr_tail[128];
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Analytics service available(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				cap->Analytics->XAddr
				);
		_get_address_tail(cap->Analytics->XAddr, addr_tail);
		memset(analytics, 0x00, 128);
		strncpy(analytics, addr_head, strlen(addr_head));
		strcat(analytics, addr_tail);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_get_services(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, char* device, char* media, char* image, char* event, char* ptz, char* analytics, char* media2)
{
	int rtn = 0;
	struct timespec now_time;
	struct soap *soap;
	struct _device__GetServices req;
	struct _device__GetServicesResponse res;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]
				);
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	}
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	req.IncludeCapability = xsd__boolean__false_;

	rtn = soap_call___device__GetServices(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

	_dev_get_services_response_parser(&res, device, media, image, event, ptz, analytics, media2);
	
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _dev_get_services_response_parser(struct _device__GetServicesResponse *res, char *device, char *media, char *image, char *event, char *ptz, char *analytics, char *media2)
{
	int size;
	int rtn = 0, i = 0, j = 0;
	struct device__Service *service;

	char *service_list[8] = {
		device,
		media,
		media2,
		image,
		event,
		analytics,
		ptz,
		NULL
	};

	size = res->__sizeService;
	service = res->Service;

	for(i = 0; i < size; i++)
	{
		for(j = 0; j < SERVICE_SIZE - 1; j++)
		{
			if(service[i].Namespace == NULL) continue;
			if(strcmp(service[i].Namespace, service_namespace[j]) == 0)
			{
				if(service_list[j] != NULL)
				{
					strncpy(service_list[j], service[i].XAddr, 128);
				}
			}
		}
	}

	return rtn;
}

#if 0
ONVIF_MSG _nf_onvif_dev_get_capabilities
(
	const char* endpoint, const char* user, const char* pass,
	struct _device__GetCapabilities* req,
	struct _device__GetCapabilitiesResponse* res
)
{
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	char *action = "GetCapabilities";
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);


	//soap_init(soap);
	if (user != NULL && pass != NULL)
	{
		rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
		g_return_val_if_fail(rtn == 0, rtn);
		NF_ONVIF_DBG("add username token digest(%d)\n", rtn);
	}

	req->__sizeCategory = 1;
	req->Category = soap_malloc(soap, sizeof(enum tt__CapabilityCategory));
	req->Category[0] = tt__CapabilityCategory__All;

	rtn = soap_call___device__GetCapabilities(soap, endpoint, NULL, req, res);

	if (res->Capabilities == NULL)
	{
		NF_ONVIF_DBG("NULL Reponse\n");
		goto ends_label;
	}
	if (res->Capabilities[0].Device != NULL)
	{
		struct tt__DeviceCapabilities *nf_dev = res->Capabilities[0].Device;
		NF_ONVIF_DBG("--Device\n");
		if (nf_dev->XAddr != NULL)
		{
			NF_ONVIF_DBG("----XAddr(%s)\n", nf_dev->XAddr);
		}
		if (nf_dev->Network != NULL)
		{
			struct tt__NetworkCapabilities *nf_net = nf_dev->Network;
			NF_ONVIF_DBG("----Network\n");
			if (nf_net->IPFilter != NULL)
			{
				NF_ONVIF_DBG("------IPFilter                         - %s\n", _NF_TF_STR[nf_net->IPFilter[0]]);
			}
			if (nf_net->ZeroConfiguration != NULL)
			{
				NF_ONVIF_DBG("------ZeroConfiguration                - %s\n", _NF_TF_STR[nf_net->ZeroConfiguration[0]]);
			}
			if (nf_net->IPVersion6 != NULL)
			{
				NF_ONVIF_DBG("------IPv6                             - %s\n", _NF_TF_STR[nf_net->IPVersion6[0]]);
			}
			if (nf_net->DynDNS != NULL)
			{
				NF_ONVIF_DBG("------DynDNS                           - %s\n", _NF_TF_STR[nf_net->DynDNS[0]]);
			}
			if (nf_net->Extension != NULL)
			{
				NF_ONVIF_DBG("------Extension                        - skip\n");
			}
		}
		if (nf_dev->System != NULL)
		{
			struct tt__SystemCapabilities *nf_sys = nf_dev->System;
			NF_ONVIF_DBG("----System\n");
			NF_ONVIF_DBG("------Extension                        - skip\n");
			NF_ONVIF_DBG("------DiscoveryResolve                 - %s\n", _NF_TF_STR[nf_sys->DiscoveryResolve]);
			NF_ONVIF_DBG("------DiscoveryBye                     - %s\n", _NF_TF_STR[nf_sys->DiscoveryBye]);
			NF_ONVIF_DBG("------RemoteDiscovery                  - %s\n", _NF_TF_STR[nf_sys->RemoteDiscovery]);
			NF_ONVIF_DBG("------SystemBackup                     - %s\n", _NF_TF_STR[nf_sys->SystemBackup]);
			NF_ONVIF_DBG("------SystemLogging                    - %s\n", _NF_TF_STR[nf_sys->SystemLogging]);
			NF_ONVIF_DBG("------FirmwareUpgrade                  - %s\n", _NF_TF_STR[nf_sys->FirmwareUpgrade]);
			NF_ONVIF_DBG("------__sizeSupportedVersions          - %d\n", nf_sys->__sizeSupportedVersions);
			if (nf_sys->SupportedVersions != NULL)
			{
				NF_ONVIF_DBG("------SupportedVersions                - %d.%d\n", nf_sys->SupportedVersions[0].Major, nf_sys->SupportedVersions[0].Minor);
			}
			if (nf_sys->Extension != NULL)
			{
				NF_ONVIF_DBG("------Extension                        - skip\n");
			}
		}
		if (nf_dev->Security != NULL)
		{
			struct tt__SecurityCapabilities *nf_sec = nf_dev->Security;
			NF_ONVIF_DBG("----Security\n");
			NF_ONVIF_DBG("------TLS1                             - %s\n", _NF_TF_STR[nf_sec->TLS1_x002e1]);
			NF_ONVIF_DBG("------TLS2                             - %s\n", _NF_TF_STR[nf_sec->TLS1_x002e2]);
			NF_ONVIF_DBG("------OnboardKeyGeneration             - %s\n", _NF_TF_STR[nf_sec->OnboardKeyGeneration]);
			NF_ONVIF_DBG("------AccessPolicyConfig               - %s\n", _NF_TF_STR[nf_sec->AccessPolicyConfig]);
			NF_ONVIF_DBG("------X_x002e509Token                  - %s\n", _NF_TF_STR[nf_sec->X_x002e509Token]);
			NF_ONVIF_DBG("------SAMLToken                        - %s\n", _NF_TF_STR[nf_sec->SAMLToken]);
			NF_ONVIF_DBG("------KerberosToken                    - %s\n", _NF_TF_STR[nf_sec->KerberosToken]);
			NF_ONVIF_DBG("------RELToken                         - %s\n", _NF_TF_STR[nf_sec->RELToken]);
			if (nf_sec->Extension != NULL)
			{
				NF_ONVIF_DBG("------Extension                        - skip\n");
			}
		}
		if (nf_dev->IO != NULL)
		{
			NF_ONVIF_DBG("----IO                                 - skip\n");
		}
		if (nf_dev->Extension != NULL)
		{
			NF_ONVIF_DBG("----Extension                          - skip\n");
		}
	}
	if (res->Capabilities[0].Imaging != NULL)
	{
		struct tt__ImagingCapabilities *nf_img = res->Capabilities[0].Imaging;
		NF_ONVIF_DBG("--Imaging\n");
		if (nf_img->XAddr != NULL)
		{
			NF_ONVIF_DBG("----XAddr(%s)\n", nf_img->XAddr);
		}
	}
	if (res->Capabilities[0].Media != NULL)
	{
		struct tt__MediaCapabilities *nf_media = res->Capabilities[0].Media;
		NF_ONVIF_DBG("--Media\n");
		if (nf_media->XAddr != NULL)
		{
			NF_ONVIF_DBG("----XAddr(%s)\n", nf_media->XAddr);
			struct tt__RealTimeStreamingCapabilities *nf_sc = nf_media->StreamingCapabilities;
			if (nf_sc != NULL)
			{
				NF_ONVIF_DBG("----StreamingCapabilities\n");
				if (nf_sc->RTPMulticast != NULL)
				{
					NF_ONVIF_DBG("------RTP Multicast                    - %s\n", _NF_TF_STR[nf_sc->RTPMulticast[0]]);
				}
				if (nf_sc->RTP_USCORETCP != NULL)
				{
					NF_ONVIF_DBG("------RTP_TCP                          - %s\n", _NF_TF_STR[nf_sc->RTP_USCORETCP[0]]);
				}
				if (nf_sc->RTP_USCORERTSP_USCORETCP != NULL)
				{
					NF_ONVIF_DBG("------RTP_RTSP_TCP                     - %s\n", _NF_TF_STR[nf_sc->RTP_USCORERTSP_USCORETCP[0]]);
				}
				if (nf_sc->Extension != NULL)
				{
					NF_ONVIF_DBG("------Extension                        - skip\n");
				}
			}
			if (nf_media->Extension != NULL)
			{
				NF_ONVIF_DBG("----Extension                              - skip\n");
			}
		}
	}
	if (res->Capabilities[0].PTZ != NULL)
	{
		struct tt__PTZCapabilities *nf_ptz = res->Capabilities[0].PTZ;
		NF_ONVIF_DBG("--PTZ\n");
		if (nf_ptz->XAddr != NULL)
		{
			NF_ONVIF_DBG("----XAddr(%s)\n", nf_ptz->XAddr);
		}
	}
	if (res->Capabilities[0].Analytics != NULL)
	{
		NF_ONVIF_DBG("--Analytics                            - skip\n");
	}
	if (res->Capabilities[0].Events != NULL)
	{
		NF_ONVIF_DBG("--Events                               - skip\n");
	}
	if (res->Capabilities[0].Extension != NULL)
	{
		NF_ONVIF_DBG("--Extension                            - skip\n");
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_get_network_default_gateway(const char* endpoint)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetNetworkDefaultGateway req;
	struct _device__GetNetworkDefaultGatewayResponse res;
	struct tt__NetworkGateway *nf_gw;
	char *action = "GetNetworkDefaultGateway";
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	//soap_init(soap);

	rtn = soap_call___device__GetNetworkDefaultGateway(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("Failed request\n");
		return rtn;
	}

	nf_gw = res.NetworkGateway;
	NF_ONVIF_DBG("IPv4\n");
	for (i = 0; i < nf_gw->__sizeIPv4Address; i++)
	{
		NF_ONVIF_DBG("%s\n", nf_gw->IPv4Address[i]);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

#endif


ONVIF_MSG _nf_onvif_dev_system_reboot(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, int interface
)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__SystemReboot req;
	struct _device__SystemRebootResponse res;
	char *action = "SystemReboot";
	struct timespec now_time;
	time_t cam_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			auth_info->username, auth_info->password
			);

	if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]
				);
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	}
#if 0
	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenText(soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		cam_time = _nf_onvif_get_time(endpoint);
		if (cam_time != 0)
		{
			rtn = soap_wsse_add_UsernameTokenDigestWithManulTime(soap, NULL, user, pass, cam_time);
		}
		else
		{
			rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
		}
	}
#endif
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

#ifdef DUAL_LAN_NETWORK
	if(!nf_get_dual_lan_mode())
	{
		if(interface > 0 && nf_get_running_mode() == 0)
		{
			soap->fopen = lan_open;
		}
	}
#endif
	rtn = soap_call___device__SystemReboot(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("Failed request\n");

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	if (res.Message != NULL)
	{
		NF_ONVIF_DBG("Received(%s)\n", res.Message);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return 0;
}

ONVIF_MSG _nf_onvif_dev_get_network_interfaces_token(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	char* _token,
	enum xsd__boolean* _dhcp,
	int interface
)
{
	int i = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct tt__NetworkInterface *nf_inf;
	struct _device__GetNetworkInterfaces req;
	struct _device__GetNetworkInterfacesResponse res;
	char *action = "GetNetworkInterfaces";
	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			user, pass
			);

	if (_token == NULL)
	{
		NF_ONVIF_DBG("[%s] ERROR: Write buffer is NULL\n", __FUNCTION__);
		goto ends_label;
	}

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
#if 0
	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenText(soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
	}
#endif
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

#ifdef DUAL_LAN_NETWORK
	if(!nf_get_dual_lan_mode())
	{
		if(interface > 0 && nf_get_running_mode() == 0)
		{
			soap->fopen = lan_open;
		}
	}
#endif
	rtn = soap_call___device__GetNetworkInterfaces(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Failed request\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | Num of network interfaces: %d\n",
			D_SECS(now_time),
			__FUNCTION__,
			res.__sizeNetworkInterfaces
			);

	for (i = 0; i < res.__sizeNetworkInterfaces; i++)
	{
		nf_inf = &res.NetworkInterfaces[i];
		if (nf_inf->token == NULL) continue;

		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | token(%s) Enabled(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				nf_inf->token, nf_inf->Enabled
				);

		if (nf_inf->IPv4 != NULL)
		{
			strcpy(_token, nf_inf->token);
			if (nf_inf->IPv4->Config != NULL)
			{
				*_dhcp = nf_inf->IPv4->Config->DHCP;
			}
			else
			{
				*_dhcp = xsd__boolean__false_;
			}
			goto ends_label;
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_set_network_dhcp
(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	char* token,
	int interface
)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__SetNetworkInterfaces req;
	struct tt__NetworkInterfaceSetConfiguration *netif;
	struct tt__IPv4NetworkInterfaceSetConfiguration *netif_ipv4;

	struct _device__SetNetworkInterfacesResponse res;
	enum xsd__boolean t = xsd__boolean__true_;
	enum xsd__boolean f = xsd__boolean__false_;
	char *action = "SetNetworkInterfaces";
	char ip_str[16];
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	if (user != NULL && pass != NULL)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
				D_SECS(now_time),
				__FUNCTION__,
				user, pass
				);
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | token(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			token
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
#if 0
	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenText(soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
	}
#endif
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

#ifdef DUAL_LAN_NETWORK
	if(!nf_get_dual_lan_mode())
	{
		if(interface > 0 && nf_get_running_mode() == 0)
		{
			soap->fopen = lan_open;
		}
	}
#endif
	memset(&req, 0x00, sizeof(req));

	req.InterfaceToken = soap_malloc(soap, strlen(token)+1);
	req.NetworkInterface = soap_malloc(soap, sizeof(struct tt__NetworkInterfaceSetConfiguration));

	strncpy(req.InterfaceToken, token, strlen(token)+1);
	memset(req.NetworkInterface,0x00,sizeof(struct tt__NetworkInterfaceSetConfiguration));

	netif = req.NetworkInterface;
	netif->Enabled = soap_malloc(soap, sizeof(enum xsd__boolean));
	netif->Link = NULL;
	netif->MTU = NULL;
	netif->IPv4 = soap_malloc(soap, sizeof(struct tt__IPv4NetworkInterfaceSetConfiguration));
	netif->IPv6 = NULL;
	netif->Extension = NULL;
	//netif->__anyAttribute = NULL;

	memcpy(netif->Enabled, &t, sizeof(enum xsd__boolean));

	netif_ipv4 = netif->IPv4;
	netif_ipv4->Enabled = soap_malloc(soap, sizeof(enum xsd__boolean));
	memcpy(netif_ipv4->Enabled, &t, sizeof(enum xsd__boolean));
	netif_ipv4->__sizeManual = 0;
	netif_ipv4->Manual = NULL;
	netif_ipv4->DHCP = soap_malloc(soap, sizeof(enum xsd__boolean));
	memcpy(netif_ipv4->DHCP, &t, sizeof(enum xsd__boolean));

	rtn = soap_call___device__SetNetworkInterfaces(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("Failed request\n");

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | RebootNeeded(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			_NF_TF_STR[res.RebootNeeded]
			);

	if (res.RebootNeeded)
	{
		rtn = NF_ONVIF_REBOOTING;
		goto ends_label;
	}

#if 0
	if (res.RebootNeeded)
	{
		rtn = _nf_onvif_dev_system_reboot(auth, endpoint, user, pass);
		if (rtn == 0)
		{
			rtn = NF_ONVIF_REBOOTING;
		}
		goto ends_label;
	}
#endif

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_set_network_interfaces
(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	char* token,
	unsigned int ipaddr
)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__SetNetworkInterfaces req;
	struct tt__NetworkInterfaceSetConfiguration netif;
	struct tt__IPv4NetworkInterfaceSetConfiguration netif_ipv4;
	struct tt__PrefixedIPv4Address netif_ipv4_manual;

	struct _device__SetNetworkInterfacesResponse res;
	enum xsd__boolean t = xsd__boolean__true_;
	enum xsd__boolean f = xsd__boolean__false_;
	char *action = "SetNetworkInterfaces";
	char ip_str[16];
	struct timespec now_time;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
#if 0
	if (auth == NF_ONVIF_AUTH_TEXT)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: TEXT\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenText(soap, NULL, user, pass);
	}
	else if (auth == NF_ONVIF_AUTH_DIGEST)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: DIGEST\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = soap_wsse_add_UsernameTokenDigest(soap, NULL, user, pass);
	}
#endif
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	memset(&req, 0x00, sizeof(req));
	memset(&netif, 0x00, sizeof(netif));
	memset(&netif_ipv4, 0x00, sizeof(netif_ipv4));
	memset(&netif_ipv4_manual, 0x00, sizeof(netif_ipv4_manual));
	snprintf(ip_str, 16, "%d.%d.%d.%d", (ipaddr&0xff), (ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24);

	req.InterfaceToken = token; 
	req.NetworkInterface = &netif;
	netif.Enabled = &t;
	netif.IPv4 = &netif_ipv4;
	netif_ipv4.Enabled = &t;
	netif_ipv4.__sizeManual = 1;
	netif_ipv4.Manual = &netif_ipv4_manual;
	netif_ipv4_manual.Address = ip_str;
	netif_ipv4_manual.PrefixLength = 24;
	netif_ipv4.DHCP = &f;

	rtn = soap_call___device__SetNetworkInterfaces(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("Failed request\n");

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | RebootNeeded(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			_NF_TF_STR[res.RebootNeeded]
			);
	auth_info.auth_method = auth; 
	auth_info.username = user;
	auth_info.password = pass;
	auth_info.endpoint = endpoint;

	if (res.RebootNeeded)
	{
#if 1
		rtn = _nf_onvif_dev_system_reboot(&auth_info, endpoint, 0);
		if (rtn == 0)
		{
			rtn = NF_ONVIF_REBOOTING;
		}
#else
		rtn = NF_ONVIF_REBOOTING;
#endif
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_create_user(
    ipcam_onvif_auth_info_t* auth_info,
    const char *new_user,
    const char *new_pass,
    const char *new_level,
    const char *endpoint
)
{
    int                                 rtn         = 0;
    struct soap                         *soap       = NULL;
    char                                *action     = "CreateUsers";
    struct _device__CreateUsers         req;
    struct _device__CreateUsersResponse res;
    struct timespec                     now_time;
    time_t                              cam_time    = 0;


    memset(&req,      0x00, sizeof(req));
    memset(&res,      0x00, sizeof(res));
    memset(&now_time, 0x00, sizeof(now_time));

    soap = soap_new();
    if(soap == NULL)
    {
        NF_ONVIF_DBG("[%s] ERROR: soap_new is NULL\n", __FUNCTION__);
        goto ends_label;
    }

    nf_onvif_soap_init_set(soap);

    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
                D_SECS(now_time), __FUNCTION__);

    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
                D_SECS(now_time), __FUNCTION__, endpoint);

    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
                D_SECS(now_time), __FUNCTION__, auth_info->username, auth_info->password);

    if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
    {
        clock_gettime(CLOCK_REALTIME, &now_time);
        NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
                    D_SECS(now_time), __FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]);

        rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username,
                                 auth_info->password, auth_info->endpoint);
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &now_time);
        NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
                    D_SECS(now_time), __FUNCTION__);
        rtn = 0;
    }

    if (rtn != 0)
    {
        NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
        goto ends_label;
    }

    if (strstr(endpoint, "https://") != NULL)
    {
        rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
                                      NULL, NULL, NULL, NULL, NULL);

        if (rtn != 0)
        {
            char buf[1024] = { 0, };
            soap_sprint_fault(soap, buf, 1024);
            NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
            goto ends_label;
        }
    }

    req.__sizeUser = 1;
    if((req.User = soap_malloc(soap, sizeof(struct tt__User))) == NULL)
    {
        NF_ONVIF_DBG("[%s] ERROR: soap_malloc fail \n", __FUNCTION__);
        goto ends_label;
    }
    else
    {
        req.User[0].Username  = soap_malloc(soap, (strlen(new_user)+1));
        req.User[0].Password  = soap_malloc(soap, (strlen(new_pass)+1));
        req.User[0].Extension = soap_malloc(soap, sizeof(struct tt__UserExtension));
        if(req.User[0].Username == NULL || req.User[0].Password == NULL || req.User[0].Extension == NULL)
        {
            NF_ONVIF_DBG("[%s] ERROR: soap_malloc fail \n", __FUNCTION__);
            goto ends_label;
        }
        else
        {
            snprintf(req.User[0].Username, (strlen(new_user)+1), new_user);
            snprintf(req.User[0].Password, (strlen(new_pass)+1), new_pass);
            if(strcmp(new_level, "Administrator") == 0)
                req.User[0].UserLevel = tt__UserLevel__Administrator;
            else if(strcmp(new_level, "Operator") == 0)
                req.User[0].UserLevel = tt__UserLevel__Operator;
            else if(strcmp(new_level, "User") == 0)
                req.User[0].UserLevel = tt__UserLevel__User;
            else if(strcmp(new_level, "Anonymous") == 0)
                req.User[0].UserLevel = tt__UserLevel__Anonymous;
            else if(strcmp(new_level, "Extended") == 0)
                req.User[0].UserLevel = tt__UserLevel__Extended;
            else
                req.User[0].UserLevel = tt__UserLevel__Administrator;
        }
    }

    rtn = soap_call___device__CreateUsers(soap, endpoint, NULL, &req, &res);

    if (rtn != 0)
    {
        clock_gettime(CLOCK_REALTIME, &now_time);
        NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail(%d)\n",
                    D_SECS(now_time), __FUNCTION__, rtn);

        char buf[1024] = { 0, };
        soap_sprint_fault(soap, buf, 1024);
        NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
        goto ends_label;
    }

ends_label:
    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
                D_SECS(now_time), __FUNCTION__);

    return rtn;
}

ONVIF_MSG _nf_onvif_dev_get_device_info(
	int ch,
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint,
	cam_model_info *rtn_info, int interface
)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetDeviceInformation req;
	struct _device__GetDeviceInformationResponse res;
	char *action = "GetDeviceInformation";
	struct timespec now_time;
	time_t cam_time;
	mtable *runtime = get_runtime();

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			auth_info->username, auth_info->password
			);

	if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]
				);
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

#ifdef DUAL_LAN_NETWORK
	if(!nf_get_dual_lan_mode())
	{
		if(interface > 0 && nf_get_running_mode() == 0)
		{
			soap->fopen = lan_open;
		}
	}
#endif
	rtn = soap_call___device__GetDeviceInformation(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__, rtn
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

	if (res.Manufacturer != NULL && res.Manufacturer[0] != '\0')
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Manufacturer(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, res.Manufacturer
				);
		if (ch < 0)
		{
			strcpy(rtn_info->vendor, res.Manufacturer);
		}
		else
		{
			strcpy(runtime[ch].sys.vendor, res.Manufacturer);
		}
	}
	if (res.Model != NULL && res.Model[0] != '\0')
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Model(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, res.Model
				);
		if (ch < 0)
		{
			strcpy(rtn_info->name, res.Model);
		}
		else
		{
			strcpy(runtime[ch].sys.model, res.Model);
			strcpy(runtime[ch].sys.stdver, res.Model);
		}
	}
	if (res.FirmwareVersion != NULL && res.FirmwareVersion[0] != '\0')
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | FirmwareVersion(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, res.FirmwareVersion
				);
		if (ch < 0)
		{
			strcpy(rtn_info->swver, res.FirmwareVersion);
		}
		else
		{
			strcpy(runtime[ch].sys.swver, res.FirmwareVersion);
		}
	}
	if (res.SerialNumber != NULL && res.SerialNumber[0] != '\0')
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | SerialNumber(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, res.SerialNumber
				);
	}
	if (res.HardwareId != NULL && res.HardwareId[0] != '\0')
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | HardwareId(%s)\n",
				D_SECS(now_time),
				__FUNCTION__, res.HardwareId
				);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
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

	dst[0] = (val[0] * 16) + val[1];
	dst[1] = (val[2] * 16) + val[3];
	dst[2] = (val[4] * 16) + val[5];
	dst[3] = (val[6] * 16) + val[7];
	dst[4] = (val[8] * 16) + val[9];
	dst[5] = (val[10] * 16) + val[11];
}
ONVIF_MSG _nf_onvif_dev_get_dns(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	NFOpenmodeSetupNetwork* netinfo
)
{
	int i = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetDNS req;
	struct _device__GetDNSResponse res;
	char *action = "GetDNS";
	struct timespec now_time;
	struct tt__DNSInformation *dns;
	int _dns_cnt = 0;

	memset(&req, 0x00, sizeof(struct _device__GetDNS));

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			user, pass
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	rtn = soap_call___device__GetDNS(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Failed request\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

	if (res.DNSInformation == NULL)
	{
		rtn = 1;
		NF_ONVIF_DBG("[%s] ERROR: NULL DNSInformation\n", __FUNCTION__);
		goto ends_label;
	}

	dns = res.DNSInformation;
	if (dns->FromDHCP)
	{
		for (i=0; i<dns->__sizeDNSFromDHCP; i++)
		{
			if (dns->DNSFromDHCP[i].Type != tt__IPType__IPv4) { continue; }

			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | DHCP DNS(%s)\n",
					D_SECS(now_time),
					__FUNCTION__, dns->DNSFromDHCP[i].IPv4Address
					);
		}
	}
	else
	{
		for (i=0; i<dns->__sizeDNSManual; i++)
		{
			if (dns->DNSManual[i].Type != tt__IPType__IPv4) { continue; }

			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | Manual DNS(%s)\n",
					D_SECS(now_time),
					__FUNCTION__, dns->DNSManual[i].IPv4Address
					);
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_get_default_gateway(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	NFOpenmodeSetupNetwork* netinfo
)
{
	int i = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetNetworkDefaultGateway req;
	struct _device__GetNetworkDefaultGatewayResponse res;
	char *action = "GetNetworkDefaultGateway";
	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			user, pass
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	rtn = soap_call___device__GetNetworkDefaultGateway(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Failed request\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

	if (res.NetworkGateway != NULL)
	{
		if (res.NetworkGateway->__sizeIPv4Address > 0)
		{
			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] %s | Gateway(%s)\n",
					D_SECS(now_time),
					__FUNCTION__,
					res.NetworkGateway->IPv4Address[0]
					);
			netinfo->gw = ntohl(inet_addr(res.NetworkGateway->IPv4Address[0]));
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_get_netinfo(
	int auth,
	const char* endpoint,
	const char* user,
	const char* pass,
	unsigned char* _get_mac,
	NFOpenmodeSetupNetwork* netinfo
)
{
	int i = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct tt__NetworkInterface *nf_inf;
	struct _device__GetNetworkInterfaces req;
	struct _device__GetNetworkInterfacesResponse res;
	char *action = "GetNetworkInterfaces";
	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | endpoint(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			endpoint
			);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | user(%s) pass(%s)\n",
			D_SECS(now_time),
			__FUNCTION__,
			user, pass
			);

	if (auth != NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	rtn = soap_call___device__GetNetworkInterfaces(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Failed request\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | Num of network interfaces: %d\n",
			D_SECS(now_time),
			__FUNCTION__,
			res.__sizeNetworkInterfaces
			);

	for (i = 0; i < res.__sizeNetworkInterfaces; i++)
	{
		nf_inf = &res.NetworkInterfaces[i];
		if (nf_inf->token == NULL) { continue; }
		if (nf_inf->Enabled == 0) { continue; }

		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | token(%s) Enabled(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				nf_inf->token, nf_inf->Enabled
				);

		if (nf_inf->Info != NULL)
		{
			if (nf_inf->Info->HwAddress != NULL)
			{
				clock_gettime(CLOCK_REALTIME, &now_time);
				NF_ONVIF_DBG("[%lu.%06lu] %s | token(%s) HwAddress(%s)\n",
						D_SECS(now_time),
						__FUNCTION__,
						nf_inf->token,
						nf_inf->Info->HwAddress
						);
				_get_mac_from_str(nf_inf->Info->HwAddress, _get_mac);

				clock_gettime(CLOCK_REALTIME, &now_time);
				NF_ONVIF_DBG("[%lu.%06lu] %s | Mac transform(%02x-%02x-%02x-%02x-%02x-%02x)\n",
						D_SECS(now_time),
						__FUNCTION__,
						_get_mac[0], _get_mac[1], _get_mac[2],
						_get_mac[3], _get_mac[4], _get_mac[5]);
				break;
			}
		}
	}

	for (i = 0; i < res.__sizeNetworkInterfaces; i++)
	{
		nf_inf = &res.NetworkInterfaces[i];
		if (nf_inf->token == NULL) { continue; }
		if (nf_inf->Enabled == 0) { continue; }

		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | token(%s) Enabled(%d)\n",
				D_SECS(now_time),
				__FUNCTION__,
				nf_inf->token, nf_inf->Enabled
				);

		if (nf_inf->IPv4 != NULL)
		{
			if (nf_inf->IPv4->Enabled == 0) { continue; }

			if (nf_inf->IPv4->Config != NULL)
			{
				netinfo->is_dhcp = nf_inf->IPv4->Config->DHCP;
				if (nf_inf->IPv4->Config->DHCP)
				{
					if(nf_inf->IPv4->Config->FromDHCP == NULL)
						continue;
					if(nf_inf->IPv4->Config->FromDHCP->Address == NULL)
						continue;

					clock_gettime(CLOCK_REALTIME, &now_time);
					NF_ONVIF_DBG("[%lu.%06lu] %s | Address(%s) PrefixLength(%d)\n",
							D_SECS(now_time),
							__FUNCTION__,
							nf_inf->IPv4->Config->FromDHCP->Address,
							nf_inf->IPv4->Config->FromDHCP->PrefixLength
							);
					netinfo->ipaddr = ntohl(inet_addr(nf_inf->IPv4->Config->FromDHCP->Address));
					netinfo->mask = nf_inf->IPv4->Config->FromDHCP->PrefixLength;
				}
				else
				{
					if (nf_inf->IPv4->Config->Manual != NULL)
					{
						NF_ONVIF_DBG("[%lu.%06lu] %s | Address(%s) PrefixLength(%d)\n",
								D_SECS(now_time),
								__FUNCTION__,
								nf_inf->IPv4->Config->Manual->Address,
								nf_inf->IPv4->Config->Manual->PrefixLength
								);
						netinfo->ipaddr = ntohl(inet_addr(nf_inf->IPv4->Config->Manual->Address));
						netinfo->mask = nf_inf->IPv4->Config->Manual->PrefixLength;
					}
				}
			}
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_dev_set_network_default_gateway(
	int auth,
	const char* endpoint,
	const unsigned int ipaddr
)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__SetNetworkDefaultGateway req;
	struct _device__SetNetworkDefaultGatewayResponse res;
	char *action = "SetNetworkDefaultGateway";
	char ip_str[1][16];
	char *ip_a;
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);


	//soap_init(soap);
	snprintf(ip_str[0], 16, "%d.%d.%d.%d", (ipaddr&0xff), (ipaddr&0xff00)>>8,
			(ipaddr&0xff0000)>>16, (ipaddr&0xff000000)>>24);

	ip_a = ip_str[0];
	req.__sizeIPv4Address = 1;
	req.IPv4Address = &ip_a;
	req.__sizeIPv6Address = 0;
	req.IPv6Address = NULL;

	rtn = soap_call___device__SetNetworkDefaultGateway(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("Failed request\n");

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		//return rtn;


	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}


ONVIF_MSG _nf_onvif_dev_get_service_capabilities(int auth, const char* endpoint, const char* user, const char* pass)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__GetServiceCapabilities req;
	struct _device__GetServiceCapabilitiesResponse res;
	char *action = "GetServiceCapabilities";

	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = soap_call___device__GetServiceCapabilities(soap, endpoint, NULL, &req, &res);

	if(rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail(%d)\n",
				D_SECS(now_time),
				__FUNCTION__, rtn
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		goto ends_label;
	}

	if(res.Capabilities == NULL)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC response MSG wrong\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		goto ends_label;
	}

	if(res.Capabilities->Network != NULL)
	{
		if(res.Capabilities->Network->IPFilter != NULL)
		{
			NF_ONVIF_DBG("Network->IPFilter           : %s\n", _NF_TF_STR[*(res.Capabilities->Network->IPFilter)]);
		}
		if(res.Capabilities->Network->ZeroConfiguration != NULL)
		{
			NF_ONVIF_DBG("Network->ZeroConfiguration  : %s\n", _NF_TF_STR[*(res.Capabilities->Network->ZeroConfiguration)]);
		}
		if(res.Capabilities->Network->IPVersion6 != NULL)
		{
			NF_ONVIF_DBG("Network->IPVersion6         : %s\n", _NF_TF_STR[*(res.Capabilities->Network->IPVersion6)]);
		}
		if(res.Capabilities->Network->DynDNS != NULL)
		{
			NF_ONVIF_DBG("Network->DynDNS             : %s\n", _NF_TF_STR[*(res.Capabilities->Network->DynDNS)]);
		}
		if(res.Capabilities->Network->Dot11Configuration != NULL)
		{
			NF_ONVIF_DBG("Network->Dot11Configuration : %s\n", _NF_TF_STR[*(res.Capabilities->Network->Dot11Configuration)]);
		}
		if(res.Capabilities->Network->HostnameFromDHCP != NULL)
		{
			NF_ONVIF_DBG("Network->HostnameFromDHCP   : %s\n", _NF_TF_STR[*(res.Capabilities->Network->HostnameFromDHCP)]);
		}
		if(res.Capabilities->Network->NTP != NULL)
		{
			NF_ONVIF_DBG("Network->NTP                : %d\n", *(res.Capabilities->Network->NTP));
		}
	}
	if(res.Capabilities->Security != NULL)
	{
		if(res.Capabilities->Security->TLS1_x002e0 != NULL)
		{

		}
	}




ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

ONVIF_MSG _nf_onvif_dev_set_user(int auth, const char* endpoint, const char* u, const char* p, const char* new_p)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _device__SetUser req;
	struct _device__SetUserResponse res;
	char *action = "SetUser";
	struct timespec now_time;


	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	memset(&req, 0x00, sizeof(req));

	if (u[0] == '\0')
	{
		rtn = 1;
	}
	else if (p[0] == '\0' || auth == NF_ONVIF_AUTH_NONE)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = 0;
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
				D_SECS(now_time),
				__FUNCTION__, _NF_AUTH_STR[auth]
				);
		rtn = _nf_onvif_add_auth(soap, auth, u, p, endpoint);
	}
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	req.__sizeUser = 1;
	req.User = soap_malloc(soap, sizeof(struct tt__User));
	req.User[0].Username = soap_malloc(soap, (strlen(u)+1));
	strcpy(req.User[0].Username, u);
	req.User[0].Password = soap_malloc(soap, (strlen(new_p)+1));
	strcpy(req.User[0].Password, new_p);
	req.User[0].UserLevel = tt__UserLevel__Administrator;


	rtn = soap_call___device__SetUser(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("Failed request\n");
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}


/******************************************************************************/
ONVIF_MSG _nf_onvif_dev_get_capabilities(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, onvif_service_t *service)
{
  g_return_val_if_fail(endpoint != NULL, (-1));

  int i = 0, j = 0;
  int rtn = 0;
  time_t cam_time;
  struct soap *soap;
  soap = soap_new();
  nf_onvif_soap_init_set(soap);
  struct _device__GetCapabilities req;
  struct _device__GetCapabilitiesResponse res;
  struct tt__Capabilities *cap;
  struct timespec now_time;
  char *http_ptr;
  char addr_head[128];


  clock_gettime(CLOCK_REALTIME, &now_time);
  NF_ONVIF_DBG("[%lu.%06lu] %s | start\n",
      D_SECS(now_time),
      __FUNCTION__
      );

  if (auth_info->auth_method != NF_ONVIF_AUTH_NONE)
  {
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: %s\n",
        D_SECS(now_time),
        __FUNCTION__, _NF_AUTH_STR[auth_info->auth_method]
        );

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
  }
  if (rtn != 0)
  {
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
        D_SECS(now_time),
        __FUNCTION__
        );

    char buf[1024];
    soap_sprint_fault(soap, buf, 1024);
    NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
    goto ends_label;
  }
  else
  {
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | Auth: NONE\n",
        D_SECS(now_time),
        __FUNCTION__
        );
    rtn = 0;
  }
  if (rtn != 0)
  {
    NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
    goto ends_label;
  }

  if (strstr(endpoint, "https://") != NULL)
  {
    rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
        NULL, NULL, NULL, NULL, NULL);

    if (rtn != 0)
    {
      char buf[1024];
      soap_sprint_fault(soap, buf, 1024);
      NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
      goto ends_label;
    }
  }

  req.__sizeCategory = 1;
  req.Category = soap_malloc(soap, sizeof(enum tt__CapabilityCategory));
  req.Category[0] = tt__CapabilityCategory__All;
  rtn = soap_call___device__GetCapabilities(soap, endpoint, NULL, &req, &res);

  if (rtn != 0)
  {
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC returns fail\n",
        D_SECS(now_time),
        __FUNCTION__
        );

    char buf[1024];
    soap_sprint_fault(soap, buf, 1024);
    NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
    goto ends_label;
  }
  if (res.Capabilities == NULL)
  {
    clock_gettime(CLOCK_REALTIME, &now_time);
    NF_ONVIF_DBG("[%lu.%06lu] %s | ERROR: RPC response MSG wrong\n",
        D_SECS(now_time),
        __FUNCTION__
        );
    rtn = (-1);
    goto ends_label;
  }

  _get_address_head(endpoint, addr_head);

  cap = res.Capabilities;
  if (cap->Device != NULL)
  {
    struct tt__DeviceCapabilities *Device = res.Capabilities->Device;
    service->device.supported = 1;

    if(Device->IO != NULL)
    {
      service->device.IO.supported = 1;
      if(Device->IO->InputConnectors != NULL)
      {
        service->device.IO.InputConnectors = *Device->IO->InputConnectors;
      }

      if(Device->IO->RelayOutputs != NULL)
      {
        service->device.IO.RelayOutputs = *Device->IO->RelayOutputs;
        service->device.IO.InputConnectors =  *Device->IO->RelayOutputs;
      }
    }
  }

  if(cap->Analytics != NULL)
  {
    // 0: false, 1: true
    service->analytics.supported = 1;

    if(cap->Analytics->RuleSupport == 1) // if true
    {
      service->analytics.RuleSupport = 1;
    }
    else
    {
      service->analytics.RuleSupport = 0;
    }

    if(cap->Analytics->AnalyticsModuleSupport == 1) // if true
    {
      service->analytics.AnalyticsModuleSupport = 1;
    }

  }

  if (cap->Media != NULL)
  {

  }

  if (cap->Imaging != NULL)
  {

  }

  if (cap->Events != NULL)
  {

  }

  if (cap->PTZ!= NULL)
  {

  }

ends_label:
  soap_destroy(soap);
  soap_end(soap);
  soap_free(soap);
  clock_gettime(CLOCK_REALTIME, &now_time);
  NF_ONVIF_DBG("[%lu.%06lu] %s | end\n",
      D_SECS(now_time),
      __FUNCTION__
      );

  return rtn;
}


ONVIF_MSG _nf_onvif_dev_send_auxiliary_command(int auth, const char* endpoint, const char* user, const char* pass, char *auxiliary_command)
{
	int rtn;
	struct soap *soap;
	struct _device__SendAuxiliaryCommand req;
	struct _device__SendAuxiliaryCommandResponse res;

	soap = soap_new();

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	req.AuxiliaryCommand = auxiliary_command;
	rtn = soap_call___device__SendAuxiliaryCommand(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

static const char *_str_null_to_blank(const char *str)
{
    static const char blank[2] = {0, };
    if(str) return str;
    return blank;
}

static int _url_change_host(char *url, const char *host)
{
    char buffer[200];
    char *entry;
    char *tail;

    if(url == NULL || host == NULL){
        printf("[%s:%d] argument error url[%p] host[%p]\n", __func__, __LINE__, url, host);
        return -1;
    }

    if(strlen(url) == 0){
        printf("[%s:%d] url[%s]\n", __func__, __LINE__, url);
    }

    entry = strstr(url, "://");
    if(entry){
        entry += 3;
    }else{
        entry = url;
    }

    tail = strstr(entry, ":");
    if(tail){
    }else{
        tail = strstr(entry, "/");
        if(tail){
        }
    }

    snprintf(buffer, sizeof(buffer), "%s", _str_null_to_blank(tail));
    sprintf(entry, "%s%s", host, buffer);

    return 0;
}

#ifdef TEST_UNIT_ONVIF_DEVICE
int main()
{
	const char *endpoint = "http://192.168.0.110/onvif/device_service";
	int rtn;
	struct soap *soap;
	struct _device__GetNetworkInterfaces req;
	struct _device__GetNetworkInterfacesResponse res;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	memset(&req, 0x00, sizeof(req));

	soap->fopen = wan_open;
	soap->connect_timeout = 5;
	soap->send_timeout = 5;
	soap->recv_timeout = 5;

	rtn = soap_call___device__GetNetworkInterfaces(soap, endpoint, NULL, &req, &res);
	printf("[%s:%d] rtn(%d)\n", __FUNCTION__, __LINE__, rtn);
	if (rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("%s | error txts : \n %s --------------------\n", __FUNCTION__, buf);
	}
}
#endif //TEST_UNIT_ONVIF_DEVICE

#endif //__NF_ONVIF_DEVICE_C__
