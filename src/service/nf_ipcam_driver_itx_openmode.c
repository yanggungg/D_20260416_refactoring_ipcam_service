/*
 * ITX Security
 *  System software group
 *
 *  2013-07-14 jykim
 */

#ifndef __NF_IPCAM_DRIVER_ITX_OPENMODE_C__
#define __NF_IPCAM_DRIVER_ITX_OPENMODE_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <glib.h>
// #include <gst/gst.h>
#include <nf_api_ipcam.h>
#include <nf_api_openmode.h>
#include <nf_ipcam_defs.h>

#include <openssl/ssl.h>
#include "nf_ipcam_driver_itx_md5.h"


#define WAIT_REPLY_SECS		(2)
#define PRINT_HTTP_API_SEND	(0)

#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)

static int _ssl_get_network_digest(
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	char *rbuf,
	NFOpenmodeSetupNetwork* info
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.ipsetup";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
		memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
        int ret;
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (ret < 0)
        {
            perror("setsockopt");
            _release_resource(&sock, NULL, NULL, NULL);
            return (0);
        }
    }

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "Send - %s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	str_len += len;
		strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "Read_1 - %s\n", sock_buf);
#endif
	if (strstr(sock_buf, "200 OK") == NULL)
	{
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		/*
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) <= 0)
			{
				break;
			}
			else
			{
				if(str_len > SOCK_BUF_LENGTH){
					return -1;
				}
				strncpy(recv_msg[str_len], sock_buf, len);
				str_len += len;
			}
		}
		*/

			_release_resource(&sock, NULL, &ssl, &ctx);
			{
				const char f_dhcpon[] = "dhcpon=";
				const char f_ipaddr[] = "ipaddr=";
				const char f_subnet[] = "subnet=";
				const char f_gateway[] = "gateway=";
				const char f_dns1[] = "dns1=";
				const char f_dns2[] = "dns2=";
				char *s, *e;
				char pbuf[16];

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dhcpon);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dhcpon);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->is_dhcp = (pbuf[0] == 'y') ? 1:0;

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_ipaddr);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_ipaddr);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->ipaddr = ntohl(inet_addr(pbuf));

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_subnet);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_subnet);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->mask = ntohl(inet_addr(pbuf));

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_gateway);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_gateway);
				e = strstr(s, "&");
				if (s == e) { info->gw = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->gw = ntohl(inet_addr(pbuf));
				}

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dns1);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dns1);
				e = strstr(s, "&");
				if (s == e) { info->dns1 = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->dns1 = ntohl(inet_addr(pbuf));
				}

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dns2);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dns2);
				e = strstr(s, "&");
				if (s == e) { info->dns2 = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->dns2 = ntohl(inet_addr(pbuf));
				}
		}
	}

ends_label:

	return 1;
}

static int _ssl_get_network(unsigned int ip, unsigned short port, char* u, char* p, NFOpenmodeSetupNetwork* info)
{
	const char get_model_raw[] = "action=get_setup&menu=network.ipsetup";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp = 0;
	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	unsigned char cal_mac[3];

	SSL *ssl;
	SSL_CTX *ctx;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_release_resource(0, NULL, NULL, NULL);
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			rtsp = _ssl_get_network_digest(ip, port, u, p, sock_buf, info);
			return rtsp;
		}
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		/*
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) <= 0)
			{
				break;
			}
			else
			{
				if(str_len > SOCK_BUF_LENGTH){
					return -1;
				}
				strncpy(recv_msg[str_len], sock_buf, len);
				str_len += len;
			}
		}
		*/


			_release_resource(&sock, NULL, &ssl, &ctx);
			{
				const char f_dhcpon[] = "dhcpon=";
				const char f_ipaddr[] = "ipaddr=";
				const char f_subnet[] = "subnet=";
				const char f_gateway[] = "gateway=";
				const char f_dns1[] = "dns1=";
				const char f_dns2[] = "dns2=";
				char *s, *e;
				char pbuf[16];

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dhcpon);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dhcpon);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->is_dhcp = (pbuf[0] == 'y') ? 1:0;

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_ipaddr);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_ipaddr);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->ipaddr = ntohl(inet_addr(pbuf));

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_subnet);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_subnet);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				info->mask = ntohl(inet_addr(pbuf));

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_gateway);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_gateway);
				e = strstr(s, "&");
				if (s == e) { info->gw = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->gw = ntohl(inet_addr(pbuf));
				}

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dns1);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dns1);
				e = strstr(s, "&");
				if (s == e) { info->dns1 = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->dns1 = ntohl(inet_addr(pbuf));
				}

				memset(pbuf, 0x00, 16);
				s = strstr(recv_msg, f_dns2);
				if (s == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
					return 0;
				}
				s += strlen(f_dns2);
				e = strstr(s, "&");
				if (s == e) { info->dns2 = 0; }
				else
				{
					strncpy(pbuf, s, (e-s));
					info->dns2 = ntohl(inet_addr(pbuf));
				}
			}
	}

ends_label:

	return 1;
}

static int cam_get_network_digest(
	unsigned int ip, 
	unsigned short port,
	char *u,
	char *p,
	char *rbuf,
	NFOpenmodeSetupNetwork* info
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.ipsetup";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (1);
		}
	}


	{
		const char f_dhcpon[] = "dhcpon=";
		const char f_ipaddr[] = "ipaddr=";
		const char f_subnet[] = "subnet=";
		const char f_gateway[] = "gateway=";
		const char f_dns1[] = "dns1=";
		const char f_dns2[] = "dns2=";
		char *s, *e;
		char pbuf[16];

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dhcpon);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dhcpon);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->is_dhcp = (pbuf[0] == 'y') ? 1:0;

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_ipaddr);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_ipaddr);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->ipaddr = ntohl(inet_addr(pbuf));

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_subnet);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_subnet);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->mask = ntohl(inet_addr(pbuf));

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_gateway);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_gateway);
		e = strstr(s, "&");
		if (s == e) { info->gw = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->gw = ntohl(inet_addr(pbuf));
		}

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dns1);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dns1);
		e = strstr(s, "&");
		if (s == e) { info->dns1 = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->dns1 = ntohl(inet_addr(pbuf));
		}

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dns2);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dns2);
		e = strstr(s, "&");
		if (s == e) { info->dns2 = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->dns2 = ntohl(inet_addr(pbuf));
		}
	}

ends_label:

	_release_resource(&sock, NULL, NULL, NULL);
	return 1;
}

extern int cam_get_network(unsigned int ip, unsigned short port, char* u, char* p, NFOpenmodeSetupNetwork* info)
{
	const char get_model_raw[] = "action=get_setup&menu=network.ipsetup";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtn;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int use_ssl = 0;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_release_resource(0, NULL, NULL, NULL);
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

proceed_no_ssl:
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	if (len == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_network(ip, port, u, p, info));
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "200 OK";
		const char f_auth[]		= "401 Unauthorized";
		const char f_ssl[]		= "HTTP/1.1 497"; // SWPFOURCE-1329 - for nginx (webra 2.0)
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				rtn = cam_get_network_digest(ip, port, u, p, sock_buf, info);
				return rtn;
			}
			if(strstr(sock_buf, f_ssl) != NULL)
			{
				_release_resource(&sock, NULL, NULL, NULL);
				return (_ssl_get_network(ip, port, u, p, info));
			}

			return (0);
		}
	}

	{
		const char f_dhcpon[] = "dhcpon=";
		const char f_ipaddr[] = "ipaddr=";
		const char f_subnet[] = "subnet=";
		const char f_gateway[] = "gateway=";
		const char f_dns1[] = "dns1=";
		const char f_dns2[] = "dns2=";
		char *s, *e;
		char pbuf[16];

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dhcpon);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dhcpon);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->is_dhcp = (pbuf[0] == 'y') ? 1:0;

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_ipaddr);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_ipaddr);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->ipaddr = ntohl(inet_addr(pbuf));

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_subnet);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_subnet);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		info->mask = ntohl(inet_addr(pbuf));

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_gateway);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_gateway);
		e = strstr(s, "&");
		if (s == e) { info->gw = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->gw = ntohl(inet_addr(pbuf));
		}

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dns1);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dns1);
		e = strstr(s, "&");
		if (s == e) { info->dns1 = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->dns1 = ntohl(inet_addr(pbuf));
		}

		memset(pbuf, 0x00, 16);
		s = strstr(sock_buf, f_dns2);
		if (s == NULL)
		{
			memset(info, 0x00, sizeof(NFOpenmodeSetupNetwork));
			return 0;
		}
		s += strlen(f_dns2);
		e = strstr(s, "&");
		if (e == NULL) { e = strstr(s, "\r\n"); }
		if (s == e) { info->dns2 = 0; }
		else
		{
			strncpy(pbuf, s, (e-s));
			info->dns2 = ntohl(inet_addr(pbuf));
		}
	}

ends_label:

	_release_resource(&sock, NULL, NULL, NULL);
	return 1;
}




static int _ssl_get_rtsp_port_digest(
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int str_len = 0;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
        int ret;
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (ret < 0)
        {
            perror("setsockopt");
            _release_resource(&sock, NULL, NULL, NULL);
            return (0);
        }
    }


	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		/*
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) <= 0)
			{
				break;
			}
			else
			{
				if(str_len > SOCK_BUF_LENGTH){
					return -1;
				}
				strncpy(recv_msg[str_len], sock_buf, len);
				str_len += len;
			}
		}*/

			_release_resource(&sock, NULL, &ssl, &ctx);
			{
				const char f_rtsp_port[] = "rtspport=";
				char *s, *e;
				char pbuf[8];

				memset(pbuf, 0x00, 8);
				s = strstr(recv_msg, f_rtsp_port);
				if (s == NULL)
				{
					rtsp = 0;
					goto ends_label;
				}
				s += strlen(f_rtsp_port);
				e = strstr(s, "&");
				strncpy(pbuf, s, (e-s));
				rtsp = atoi(pbuf);
			}

	}

ends_label:

	return rtsp;
}

static int _ssl_get_rtsp_port(unsigned int ip, unsigned short port, char* u, char* p)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp = 0;
	int str_len = 0;
	int len;
	int sock;
	struct sockaddr_in sin;

	unsigned char cal_mac[3];

	SSL *ssl;
	SSL_CTX *ctx;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			rtsp = _ssl_get_rtsp_port_digest(ip, port, u, p, sock_buf);
			return rtsp;
		}
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		/*
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) <= 0)
			{
				break;
			}
			else
			{
				if(str_len > SOCK_BUF_LENGTH){
					return -1;
				}
				strncpy(recv_msg[str_len], sock_buf, len);
				str_len += len;
			}
		}*/

			_release_resource(&sock, NULL, &ssl, &ctx);
			{
				const char f_rtsp_port[] = "rtspport=";
				char *s, *e;
				char pbuf[8];

				memset(pbuf, 0x00, 8);
				s = strstr(recv_msg, f_rtsp_port);
				if (s == NULL)
				{
					rtsp = 0;
					goto ends_label;
				}
				s += strlen(f_rtsp_port);
				e = strstr(s, "&");
				memcpy(pbuf, s, (e-s));
				rtsp = atoi(pbuf);
			}

	}

ends_label:

	return rtsp;
}

static int cam_get_rtsp_port_digest(
	unsigned int ip, 
	unsigned short port,
	char *u,
	char *p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (1);
		}
	}


	{
		const char f_rtsp_port[] = "rtspport=";
		char *s, *e;
		char pbuf[8];

		memset(pbuf, 0x00, 8);
		s = strstr(sock_buf, f_rtsp_port);
		if (s == NULL)
		{
			rtsp = 0;
			goto ends_label;
		}
		s += strlen(f_rtsp_port);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		rtsp = atoi(pbuf);
	}

ends_label:

	_release_resource(&sock, NULL, NULL, NULL);
	return rtsp;
}

extern int cam_get_rtsp_port(unsigned int ip, unsigned short port, char* u, char* p)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int use_ssl = 0;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

proceed_no_ssl:
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	if (len == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_rtsp_port(ip, port, u, p));
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "200 OK";
		const char f_auth[]		= "401 Unauthorized";
		const char f_ssl[]		= "HTTP/1.1 497"; // SWPFOURCE-1329 - for nginx (webra 2.0)
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				rtsp = cam_get_rtsp_port_digest(ip, port, u, p, sock_buf);
				return rtsp;
			}
			if(strstr(sock_buf, f_ssl) != NULL)
			{
				_release_resource(&sock, NULL, NULL, NULL);
				return (_ssl_get_rtsp_port(ip, port, u, p));
			}
			return (0);
		}
	}

	{
		const char f_rtsp_port[] = "rtspport=";
		char *s, *e;
		char pbuf[8];

		memset(pbuf, 0x00, 8);
		s = strstr(sock_buf, f_rtsp_port);
		if (s == NULL)
		{
			rtsp = 0;
			goto ends_label;
		}
		s += strlen(f_rtsp_port);
		e = strstr(s, "&");
		strncpy(pbuf, s, (e-s));
		rtsp = atoi(pbuf);
	}

ends_label:

	_release_resource(&sock, NULL, NULL, NULL);
	return rtsp;
}










struct _ipcam_conn_struct
{
	int is_digest;
	int is_ssl;
	char auth_str[256];
	SSL *ssl;
	SSL_CTX *ctx;
};


static int _ssl_get_conn_digest(
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	char *rbuf,
	struct _ipcam_conn_struct* conn
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
        int ret;
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (ret < 0)
        {
            perror("setsockopt");
            _release_resource(&sock, NULL, NULL, NULL);
            return (0);
        }
    }


	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %d - %s\n", __FUNCTION__, __LINE__, sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %d - %s\n", __FUNCTION__, __LINE__, sock_buf);
#endif

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %d - %s\n", __FUNCTION__, __LINE__, sock_buf);
#endif

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %d - %s\n", __FUNCTION__, __LINE__, sock_buf);
#endif

			conn->is_digest = 1;
			conn->is_ssl = 1;
			strncpy(conn->auth_str, auth_str, 256);
			conn->ssl = ssl;
			conn->ctx = ctx;
			break;
		}
	}

	return sock;
}

static int _ssl_get_conn(unsigned int ip, unsigned short port, char* u, char* p, struct _ipcam_conn_struct* conn)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);

	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp = 0;
	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	unsigned char cal_mac[3];

	SSL *ssl;
	SSL_CTX *ctx;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
	{
        int ret;
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (ret < 0)
        {
            perror("setsockopt");
            _release_resource(&sock, NULL, NULL, NULL);
            return (0);
        }
    }


	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);


#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			rtsp = _ssl_get_conn_digest(ip, port, u, p, sock_buf, conn);
			return rtsp;
		}
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		/*
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) <= 0)
			{
				break;
			}
			else
			{
				if(str_len > SOCK_BUF_LENGTH){
					return -1;
				}
				strncpy(recv_msg[str_len], sock_buf, len);
				str_len += len;

			}
		}*/

		_release_resource(&sock, NULL, &ssl, &ctx);
			conn->is_digest = 0;
			conn->is_ssl = 1;
			strncpy(conn->auth_str, auth_str, 256);
			conn->ssl = ssl;
			conn->ctx = ctx;
	}

	return sock;
}

static int cam_get_conn_digest(
	unsigned int ip, 
	unsigned short port,
	char *u,
	char *p,
	char *rbuf,
	struct _ipcam_conn_struct* conn
)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);

	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int rtsp = 0;
	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
        int ret;
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (ret < 0)
        {
            perror("setsockopt");
            _release_resource(&sock, NULL, NULL, NULL);
            return (0);
        }
    }


	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (1);
		}
	}

	conn->is_digest = 1;
	conn->is_ssl = 0;
	strncpy(conn->auth_str, auth_str, 256);
	conn->ssl = NULL;
	conn->ctx = NULL;

	return sock;
}

extern int cam_get_conn(unsigned int ip, unsigned short port, char* u, char* p, struct _ipcam_conn_struct* conn)
{
	const char get_model_raw[] = "action=get_setup&menu=network.service";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int use_ssl = 0;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

proceed_no_ssl:
#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	if (len == 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_conn(ip, port, u, p, conn));
	}

#if PRINT_HTTP_API_SEND
	printf("\n[%s] %s\n", __FUNCTION__, sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		const char f_auth[]		= "401 Unauthorized";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				rtsp = cam_get_conn_digest(ip, port, u, p, sock_buf, conn);
				close(sock);
				sock = (-1);
				return rtsp;
			}
			close(sock);
			sock = (-1);
			return (0);
		}
	}

	conn->is_digest = 0;
	conn->is_ssl = 0;
	strncpy(conn->auth_str, auth_str, 256);
	conn->ssl = NULL;
	conn->ctx = NULL;

	return sock;
}

extern int cam_set_service_ports(unsigned int ip, unsigned short port, char* u, char* p, NFOpenmodeSetupPorts* info)
{
	const char get_model_raw[] = "action=set_setup&menu=network.service&httpport=%d&rtspport=%d";
	const char ack_raw[] = "action=set_setup&menu=network.http_ack";

	char http_api[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	struct _ipcam_conn_struct conn;


	memset(&conn, 0x00, sizeof(struct _ipcam_conn_struct));
	sock = cam_get_conn(ip, port, u, p, &conn);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "connection info get failed(%d.%d.%d.%d:%d, %s, %s)\n",
			(ip&0xff),
			(ip&0xff00)>>8,
			(ip&0xff0000)>>16,
			(ip&0xff000000)>>24,
			port, u, p
		);
		return 0;
	}

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	snprintf(http_api, 256, get_model_raw, info->http_port, info->rtsp_port);

	if (conn.is_digest)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), conn.auth_str, http_api);
	}
	else
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), conn.auth_str, http_api);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (conn.is_ssl)
	{
		len = SSL_write(conn.ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(WARN, "SSL_write fail\n");
			_release_resource(&sock, NULL, &conn.ssl, &conn.ctx);
			return (-1);
		}
		sleep(5);
	}
	else
	{
		len = send(sock, sock_buf, strlen(sock_buf), 0);
		if (len <= 0)
		{
			IPCAM_DBG(WARN, "send fail\n");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
		sleep(5);
	}

	if (conn.is_ssl)
	{
		_release_resource(&sock, NULL, &conn.ssl, &conn.ctx);
	}
	else
	{
		_release_resource(&sock, NULL, NULL, NULL);
	}

ends_label:

	return 1;
}



extern int cam_set_network(unsigned int ip, unsigned short port, char* u, char* p, NFOpenmodeSetupNetwork* info)
{
	const char get_model_raw[] = "action=set_setup&menu=network.ipsetup&"
			"dhcpon=%s&ipaddr=%s&subnet=%s&gateway=%s&dns1=%s&dns2=%s";

	const char *_tf_str[] = { "no", "yes" };
	char _ipaddr[16];
	char _mask[16];
	char _gw[16];
	char _dns1[16];
	char _dns2[16];

	char http_api[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtsp;
	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int use_ssl = 0;

	struct _ipcam_conn_struct conn;
	struct _ipcam_conn_struct temp_conn;


	memset(&conn, 0x00, sizeof(struct _ipcam_conn_struct));
	sock = cam_get_conn(ip, port, u, p, &conn);

	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "connection info get failed(%d.%d.%d.%d:%d, %s, %s)\n",
			(ip&0xff),
			(ip&0xff00)>>8,
			(ip&0xff0000)>>16,
			(ip&0xff000000)>>24,
			port, u, p
		);
		return 0;
	}

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	snprintf(_ipaddr, 16, "%d.%d.%d.%d",
			(info->ipaddr&0xff000000)>>24,
			(info->ipaddr&0xff0000)>>16,
			(info->ipaddr&0xff00)>>8,
			(info->ipaddr&0xff));
	snprintf(_mask, 16, "%d.%d.%d.%d",
			(info->mask&0xff000000)>>24,
			(info->mask&0xff0000)>>16,
			(info->mask&0xff00)>>8,
			(info->mask&0xff));
	snprintf(_gw, 16, "%d.%d.%d.%d",
			(info->gw&0xff000000)>>24,
			(info->gw&0xff0000)>>16,
			(info->gw&0xff00)>>8,
			(info->gw&0xff));
	if (info->dns1 == 0)
	{
		memset(_dns1, 0x00, 16);
	}
	else
	{
		snprintf(_dns1, 16, "%d.%d.%d.%d",
				(info->dns1&0xff000000)>>24,
				(info->dns1&0xff0000)>>16,
				(info->dns1&0xff00)>>8,
				(info->dns1&0xff));
	}
	if (info->dns2 == 0)
	{
		memset(_dns2, 0x00, 16);
	}
	else
	{
		snprintf(_dns2, 16, "%d.%d.%d.%d",
				(info->dns2&0xff000000)>>24,
				(info->dns2&0xff0000)>>16,
				(info->dns2&0xff00)>>8,
				(info->dns2&0xff));
	}

	snprintf(http_api, 256, get_model_raw,
			_tf_str[info->is_dhcp], _ipaddr, _mask, _gw, _dns1, _dns2);

	if (conn.is_digest)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), conn.auth_str, http_api);
	}
	else
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), conn.auth_str, http_api);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (conn.is_ssl)
	{
		int i;
		len = SSL_write(conn.ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(WARN, "SSL_write fail\n");
			_release_resource(&sock, NULL, &conn.ssl, &conn.ctx);
			rtsp = 0;
			goto ends_label;
		}
		sleep(3);
		_release_resource(&sock, NULL, &conn.ssl, &conn.ctx);
	}
	else
	{
		len = send(sock, sock_buf, strlen(sock_buf), 0);
		if (len <= 0)
		{
			IPCAM_DBG(WARN, "SSL_write fail\n");
			_release_resource(&sock, NULL, NULL, NULL);
			rtsp = 0;
			goto ends_label;
		}
		_release_resource(&sock, NULL, NULL, NULL);
	}

	rtsp = 1;

ends_label:

	return rtsp;
}







static void _get_setuser_str(char* sock_buf, char* dst, char* pw)
{
	char i;
	char usrcnt;
	char userid[8][64];
	char groupid[8][8];
	char passwd[8][64];
	char email[8][128];
	char noti[8];
	char f_buf[16];
	char *s, *e;

	const char f_cnt[] = "usrcnt0=";
	const char f_uid[] = "usrid%d=";
	const char f_gid[] = "groupid%d=";
	const char f_pw[] = "passwd%d=";
	const char f_email[] = "email%d=";
	const char f_noti[] = "noti%d=";

	char buf_group[128];
	char buf_noti[72];
	char buf_info[1536];
	char buf_gunit[16];
	char buf_nunit[16];
	char buf_iunit[256];

	s = strstr(sock_buf, f_cnt);
	s += strlen(f_cnt);
	usrcnt = *s;
	usrcnt -= '0';

	for (i=0; i<usrcnt; i++)
	{
		memset(userid[i], 0x00, 64);
		memset(groupid[i], 0x00, 8);
		memset(passwd[i], 0x00, 64);
		memset(email[i], 0x00, 128);

		snprintf(f_buf, 16, f_uid, i);
		s = strstr(sock_buf, f_buf);
		s += strlen(f_buf);
		e = strstr(s, "&");
		strncpy(userid[i], s, (e-s));

		snprintf(f_buf, 16, f_gid, i);
		s = strstr(sock_buf, f_buf);
		s += strlen(f_buf);
		e = strstr(s, "&");
		strncpy(groupid[i], s, (e-s));

		snprintf(f_buf, 16, f_pw, i);
		s = strstr(sock_buf, f_buf);
		s += strlen(f_buf);
		e = strstr(s, "&");
		strncpy(passwd[i], s, (e-s));

		snprintf(f_buf, 16, f_email, i);
		s = strstr(sock_buf, f_buf);
		s += strlen(f_buf);
		e = strstr(s, "&");
		strncpy(email[i], s, (e-s));

		snprintf(f_buf, 16, f_noti, i);
		s = strstr(sock_buf, f_buf);
		s += strlen(f_buf);
		noti[i] = *s - '0';
	}

	memset(buf_group, 0x00, 128);
	memset(buf_noti, 0x00, 72);
	memset(buf_info, 0x00, 1536);

	snprintf(buf_group, 128, "groupid0=ADMIN&");
	snprintf(buf_info, 1536, "usrid0=ADMIN&passwd0=%s&email0=%s&", pw, email[0]);
	snprintf(buf_noti, 72, "noti0=%d&", noti[0]);
	for (i=1; i<8; i++)
	{
		memset(buf_gunit, 0x00, 16);
		memset(buf_nunit, 0x00, 16);
		memset(buf_iunit, 0x00, 256);
		if (i < usrcnt)
		{
			snprintf(buf_gunit, 16, "groupid%d=%s&", i, groupid[i]);
			snprintf(buf_nunit, 16, "noti%d=%d&", i, noti[i]);
			snprintf(buf_iunit, 256, "usrid%d=%s&passwd%d=%s&email%d=%s&", i, userid[i], i, passwd[i], i, email[i]);
			strcat(buf_info, buf_iunit);
		}
		else
		{
			snprintf(buf_gunit, 16, "groupid%d=ADMIN&", i);
			snprintf(buf_nunit, 16, "noti%d=0&", i);
		}
		strcat(buf_group, buf_gunit);
		strcat(buf_noti, buf_nunit);
	}

	snprintf(dst, SOCK_BUF_LENGTH, "action=set_setup&menu=system.usrmanage&%s%s%susrcnt0=%d",
			buf_group, buf_info, buf_noti, usrcnt);
}

static int _ssl_set_password_digest(
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	char *new_p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.usrmanage";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

	IPCAM_DBG(MAJOR, "start addr(%s:%d) login(%s:*)\n", ip_str, port, u);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(ERROR, "SSL_CTX creation fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(ERROR, "SSL creation fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connection fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 1 MSG - %s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(ERROR, "SSL_write fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(ERROR, "SSL_read fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG - %s\n", sock_buf);
#endif
	if (strstr(sock_buf, "200 OK") == NULL)
	{
		IPCAM_DBG(ERROR, "HTTP error addr(%s:%d)\n", ip_str, port);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(ERROR, "Non-chunked addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2 MSG - %s\n", sock_buf);
#endif
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 3 MSG - %s\n", sock_buf);
#endif

			{
				const char *f_validation = "usrcnt0=";
				if (strstr(sock_buf, f_validation) == NULL)
				{
					IPCAM_DBG(WARN, "getuser string(%s)\n", sock_buf);
					_release_resource(&sock, NULL, &ssl, &ctx);
					return (-1);
				}
			}

			memset(buf_set,0x00,SOCK_BUF_LENGTH);
			_get_setuser_str(sock_buf, buf_set, new_p);

			snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
					strlen(buf_set), auth_str, buf_set);

			break;
		}
	}

	SSL_shutdown(ssl);
	close(sock);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connection fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 2 MSG - %s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(ERROR, "SSL_read fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2-1 MSG - %s\n", sock_buf);
#endif
	if (strstr(sock_buf, "200 OK") == NULL)
	{
		IPCAM_DBG(ERROR, "HTTP error addr(%s:%d)\n", ip_str, port);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(ERROR, "Non-chunked addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2-2 MSG - %s\n", sock_buf);
#endif
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2-3 MSG - %s\n", sock_buf);
#endif

			break;
		}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}


#if 0

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 4 MSG - %s\n", sock_buf);
#endif
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 5 MSG - %s\n", sock_buf);
#endif
			break;
		}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}
#endif

static int _ssl_set_password(
	unsigned int ip,
	unsigned short port,
	char* u, char* p,
	char* new_p
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.usrmanage";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	unsigned char cal_mac[3];

	SSL *ssl;
	SSL_CTX *ctx;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

	IPCAM_DBG(MAJOR, "start addr(%s:%d) login(%s:*)\n", ip_str, port, u);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(ERROR, "SSL_CTX creation fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(ERROR, "SSL creation fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connection fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 1 MSG - %s\n", sock_buf);
#endif
	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(ERROR, "SSL_write fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(ERROR, "SSL_read fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG - %s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			if (strstr(sock_buf, "WWW-Authenticate: Digest") != NULL)
			{
				IPCAM_DBG(MINOR, "Digest setup again addr(%s:%d)\n", ip_str, port);
				_release_resource(&sock, NULL, &ssl, &ctx);
				len = _ssl_set_password_digest(ip, port, u, p, new_p, sock_buf);
				return len;
			}
			IPCAM_DBG(ERROR, "HTTP error addr(%s:%d)\n", ip_str, port);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return (-1);
		}
		IPCAM_DBG(ERROR, "HTTP error addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(ERROR, "Non-chunked addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2 MSG - %s\n", sock_buf);
#endif
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				IPCAM_DBG(ERROR, "SSL_read error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 3 MSG - %s\n", sock_buf);
#endif

			{
				const char *f_validation = "usrcnt0=";
				if (strstr(sock_buf, f_validation) == NULL)
				{
					IPCAM_DBG(WARN, "getuser string(%s)\n", sock_buf);
					_release_resource(&sock, NULL, &ssl, &ctx);
					return (-1);
				}
			}

			memset(buf_set,0x00,SOCK_BUF_LENGTH);
			_get_setuser_str(sock_buf, buf_set, new_p);

			snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
					strlen(buf_set), auth_str, buf_set);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 2 MSG - %s\n", sock_buf);
#endif
			if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
			{
				perror("send");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 4 MSG - %s\n", sock_buf);
#endif
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 5 MSG - %s\n", sock_buf);
#endif
			break;
		}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

static int cam_set_password_digest(
	unsigned int ip, 
	unsigned short port,
	char *u,
	char *p,
	char *new_p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.usrmanage";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;

	strncpy(username, u, 64);
	strncpy(password, p, 64);

	if (rbuf == NULL ) { return (1); }
	if (rbuf[0] == '\0') { return (1); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (1); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (1); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (1); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);

	IPCAM_DBG(MAJOR, "start addr(%s:%d) login(%s:*)\n", ip_str, port, u);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 1 MSG - %s\n", sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 1 MSG - %s\n", sock_buf);
#endif

	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(ERROR, "send fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		IPCAM_DBG(ERROR, "timeout addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		IPCAM_DBG(ERROR, "recv fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG - %s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (1);
		}
	}

	{
		const char *f_validation = "usrcnt0=";
		if (strstr(sock_buf, f_validation) == NULL)
		{
			IPCAM_DBG(WARN, "getuser string(%s)\n", sock_buf);
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}

	memset(buf_set,0x00,SOCK_BUF_LENGTH);
	_get_setuser_str(sock_buf, buf_set, new_p);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(buf_set), auth_str, buf_set);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 2 MSG - %s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(ERROR, "send fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		IPCAM_DBG(ERROR, "timeout addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		IPCAM_DBG(ERROR, "recv fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2 MSG - %s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);
	return 3;
}

extern int cam_set_password(
	unsigned int ip,
	unsigned short port,
	char* u, char* p,
	char* new_p
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.usrmanage";

	char buf_set[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int use_ssl = 0;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(get_model_raw), auth_str, get_model_raw);

	IPCAM_DBG(MAJOR, "start addr(%s:%d) login(%s:*)\n", ip_str, port, u);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

proceed_no_ssl:
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 1 MSG - %s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(ERROR, "send fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		IPCAM_DBG(ERROR, "timeout addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		IPCAM_DBG(ERROR, "recv fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	if (len == 0)
	{
		IPCAM_DBG(MINOR, "use SSL connection addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_set_password(ip, port, u, p, new_p));
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG - %s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "200 OK";
		const char f_auth[]		= "401 Unauthorized";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "use DIGEST authentication addr(%s:%d) login(%s:*)\n", ip_str, port, u);
					len = cam_set_password_digest(ip, port, u, p, new_p, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			return (0);
		}
	}

	{
		const char *f_validation = "usrcnt0=";
		if (strstr(sock_buf, f_validation) == NULL)
		{
			IPCAM_DBG(WARN, "getuser string(%s)\n", sock_buf);
			return (0);
		}
	}

	memset(buf_set,0x00,SOCK_BUF_LENGTH);
	_get_setuser_str(sock_buf, buf_set, new_p);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, 
			strlen(buf_set), auth_str, buf_set);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "socket fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "setsockopt fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND 2 MSG - %s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(ERROR, "send fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		IPCAM_DBG(ERROR, "timeout addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) < 0)
	{
		IPCAM_DBG(ERROR, "recv fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		perror("recv");
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2 MSG - %s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);
	return 3;
}

extern int cam_set_initial_pw(
	unsigned int ip,
	unsigned short port,
	char* u, char* p,
	char* new_p
)
{
	const char set_pw_raw[] = "action=set_setup&menu=security.factory&passwd=%s";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	SSL *ssl;
	SSL_CTX *ctx;

	memset(username, 0x00, 64);
	memset(password, 0x00, 64);

	IPCAM_DBG(MAJOR, "start addr(%d.%d.%d.%d:%d) u(%s) p(%s) new_p(%s)\n",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24,
			port,
			u, p, new_p
	);

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	memset(http_api, 0x00, 512);
	snprintf(http_api, 512, set_pw_raw, new_p);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (-1);
	}
#if 0
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
#endif
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) < 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG - %s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG - %s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		IPCAM_DBG(WARN, "HTTP error - %s\n", sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 2 MSG - %s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 3 MSG - %s\n", sock_buf);
#endif
#if 0
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return (-1);
			}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
			break;
		}
	}
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

#endif //__NF_IPCAM_DRIVER_ITX_OPENMODE_C__
