/**
 * @file nf_ipcam_driver_s1.c
 * @brief S1 이기종 프로토콜 구현.
 * @author Jae-young Kim
 * @date 2012-03-17
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_IPCAM_DRIVER_S1_C__
#define __NF_IPCAM_DRIVER_S1_C__

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
#include <nf_ptz.h>

#include <openssl/ssl.h>

#include "nf_ipcam_driver_s1.h"
#include "nf_ipcam_driver_itx_md5.h"

#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)
#define PRINT_HTTP_API_SEND (0)

static const char str_api_post[] =
	"POST /cgi-bin/action.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"%s";	// HTTP API
static const char str_api_post_digest[] =
	"POST /cgi-bin/action.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"	// Digest auth string
	"\r\n"
	"%s";	// HTTP API

static const char str_api_get[] =
	"GET /cgi-bin/%s HTTP/1.1\r\n"	// api str
	"Host: %s\r\n"					// IP
	"Connection: keep-alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"	// b64
	"User-Agent: IPX-NVR\r\n"
	"Accept: */*\r\n"
	"Accept-Encoding: gzip,deflate,sdch\r\n"
	"Accept-Language: ko,en-US,en;q=0.8\r\n"
	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

static const char str_api_get_noauth[] =
	"GET /cgi-bin-noauth/%s HTTP/1.1\r\n"	// api str
	"Host: %s\r\n"					// IP
	"Connection: keep-alive\r\n"
	"Cache-Control: no-cache\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Accept: */*\r\n"
	"Accept-Encoding: gzip,deflate,sdch\r\n"
	"Accept-Language: ko,en-US,en;q=0.8\r\n"
	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

int digest_auth_create(char *username, char *password, char *api_str, char *http_method, int *noncecount, char *recv_buf, char *auth_str);

static int _ssl_factory_clear(int cam_id, char* user, char* pass, char* macaddr);
static int _ssl_factory_clear_digest(int cam_id, char* user, char* pass, char* macaddr, char* rbuf);

static int _ssl_factory_default(int cam_id);
static int _ssl_factory_default_digest(int cam_id, char* rbuf);

static int _ssl_send(int cam_id, char* api_raw, char* sock_buf);
static int _ssl_send_digest(int cam_id, char* api_raw, char* sock_buf);

static int _plain_send(int cam_id, char* api_raw, char* sock_buf);
static int _plain_send_digest(int cam_id, char* api_raw, char* sock_buf);

/**
 * @brief Base64 기본 인증 문자열을 생성한다.
 * @param[in] user 사용자 ID.
 * @param[in] pass 사용자 비밀번호.
 * @param[out] auth_str 인증 문자열.
 * @return 인증 문자열의 길이.
 */
static size_t itx_basic_auth_str(char *user, char *pass, char *auth_str)
{
	char username[256];
	char tmp[256];

	memset(username, 0x00, 256);

	strncpy(username, user, 256);
	strcat(username, ":");
	strcat(username, pass);

	b64_encode_string_to_buffer(tmp, 256, username);

	snprintf(auth_str, 256, "Authorization: Basic %s", tmp);

	return strlen(auth_str);
}

/**
 * @brief SSL을 사용하여 해당 카메라로 전문을 송수신한다.
 * @param[in] cam_id 채널 번호.
 * @param[in] api_raw 전송할 전문.
 * @param[out] sock_buf 수신 전문.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
static int _ssl_send(int cam_id, char* api_raw, char* sock_buf)
{
	char auth_encbuf[256];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int len = 0;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);

	//sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);
	itx_basic_auth_str(username, password, auth_encbuf);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, api_raw, ip_str, auth_encbuf);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		//free(sock_buf);
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
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(MINOR, "SSL connection failed\n");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			len = _ssl_send_digest(cam_id, api_raw, sock_buf);
			//free(sock_buf);
			return len;
		}
		IPCAM_DBG(MINOR, "ERROR: HTTP error\n");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "chunk 2 : \n%s\n", sock_buf);
#endif
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief SSL을 사용하여 해당 카메라로 전문을 송수신한다.(Digest 인증과정 거침)
 * @param[in] cam_id 채널 번호.
 * @param[in] api_raw 전송할 전문.
 * @param[in, out] sock_buf 수신 전문.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
static int _ssl_send_digest(int cam_id, char* api_raw, char* sock_buf)
{
	char auth_str[1024];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];

	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (sock_buf == NULL ) { return (IPCAM_SETUP_RTN_FAILED); }
	if (sock_buf[0] == '\0') { return (IPCAM_SETUP_RTN_FAILED); }
	s = strstr(sock_buf, f_str_realm);
	if (s == NULL) { return (IPCAM_SETUP_RTN_FAILED); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (IPCAM_SETUP_RTN_FAILED); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(sock_buf, f_str_nonce);
	if (s == NULL) { return (IPCAM_SETUP_RTN_FAILED); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (IPCAM_SETUP_RTN_FAILED); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);

	itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);

	//sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, api_raw, ip_str, auth_str);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		//free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
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
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		//free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "SSL context creation failed\n" );
		_release_resource(&sock, NULL, NULL, NULL);
		return (IPCAM_SETUP_RTN_FAILED);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(MINOR, "SSL creation failed\n" );
		_release_resource(&sock, NULL, NULL, &ctx);
		return (IPCAM_SETUP_RTN_FAILED);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(MINOR, "SSL connection failed\n" );
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (IPCAM_SETUP_RTN_FAILED);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (IPCAM_SETUP_RTN_FAILED);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (IPCAM_SETUP_RTN_FAILED);
	}


#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		IPCAM_DBG(MINOR, "ERROR: HTTP error(%s)\n", sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (IPCAM_SETUP_RTN_FAILED);
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "chunk 2 : \n%s\n", sock_buf);
#endif
	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 일반 http 통신을 사용하여 해당 카메라로 전문을 송수신한다.
 * @param[in] cam_id 채널 번호.
 * @param[in] api_raw 전송할 전문.
 * @param[out] sock_buf 수신 전문.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
static int _plain_send(int cam_id, char* api_raw, char* sock_buf)
{
	char auth_encbuf[256];
	char auth_str[256];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	int rtn;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, api_raw, ip_str, auth_str);

	memset(&sin, 0x00, sizeof(sin));
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
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_WAITALL) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					rtn = _plain_send_digest(cam_id, api_raw, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}

		/*memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
		{
			perror("recv");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif*/
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 일반 http 통신을 사용하여 해당 카메라로 전문을 송수신한다.(Digest 인증과정 거침)
 * @param[in] cam_id 채널 번호.
 * @param[in] api_raw 전송할 전문.
 * @param[in, out] sock_buf 수신 전문.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
static int _plain_send_digest(int cam_id, char* api_raw, char* sock_buf)
{
	char http_api[256];
	char auth_str[1024];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (sock_buf == NULL ) { return (0); }
	if (sock_buf[0] == '\0') { return (0); }
	s = strstr(sock_buf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(sock_buf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	snprintf(uri, 128, "http://%s:%d/cgi-bin/action.cgi", ip_str, http_port);

	itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, api_raw, ip_str, auth_str);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(WARN, "sock fail CH(%d)\n", cam_id);
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
			IPCAM_DBG(WARN, "setsocketopt fail CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(WARN, "send fail CH(%d)\n", cam_id);
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 15;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		IPCAM_DBG(WARN, "recv timeout CH(%d)\n", cam_id);
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_WAITALL) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}

/*		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
		{
			perror("recv");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif*/
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 이기종 프로토콜을 통해 모션 capability를 조회한다.
 * @param[out] info 모션 설정 정보 struct.
 * @param[in] cam_id 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_get_event_cap(motion_t* info, int cam_id)
{
#if 0
	int rtn;
	if(1)
	{
		rtn = _ssl_event_cap_get(info, cam_id);
	}


#endif
	int rtn;
	//char *sock_buf, *s, *p;
	char *s, *p;
	char buf[16];
	char sock_buf[SOCK_BUF_LENGTH];
	//sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	char get_event_raw[] = "action.fcgi?api=get_setup.event.event_capability";

	/* FIXME must choose ssl / plain */
	if(0)
	{
		rtn = _ssl_send(cam_id, get_event_raw, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, get_event_raw, sock_buf);
	}

	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		//free(sock_buf);
		return rtn;
	}

	{
		const char f_motion_type[]		= "motion_type";
		const char f_area_count[]		= "area_count";
		const char f_block_width[]		= "block_width";
		const char f_block_height[]		= "block_height";
		const char f_sensitivity[]		= "sensitivity";
		const char f_sens_min[]			= "sens_min";
		const char f_sens_max[]			= "sens_max";
		const char f_preset_md_count[]	= "preset_md_count";
		const char f_endline[]			= "\n";


		s = sock_buf;
		p = strstr(s, f_motion_type);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_motion_type);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if(strstr(s, "none"))
		{
			memset(info, 0x00, sizeof(motion_t));
			info->method = MAM_NONE;
			//free(sock_buf);
			return IPCAM_SETUP_RTN_DONE;
		}
		else if(strstr(s, "rect"))
		{
			info->method = MAM_RECTANGLE;
		}
		else
		{
			info->method = MAM_NONE;
			//free(sock_buf);
			//return IPCAM_SETUP_RTN_FAILED;
		}

		s = sock_buf;
		p = strstr(s, f_area_count);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_area_count);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->max_rect = atoi(buf);


		s = sock_buf;
		p = strstr(s, f_block_width);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_block_width);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->block_width = atoi(buf);


		s = sock_buf;
		p = strstr(s, f_block_height);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_block_height);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->block_height = atoi(buf);

		info->min_block = 1;
		info->num_block = info->block_width * info->block_height;


		s = sock_buf;
		p = strstr(s, f_sensitivity);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_sensitivity);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if(strstr(s, "yes"))
		{

		}
		else if(strstr(s, "no"))
		{
			info->sensitivity.min = 0;
			info->sensitivity.max = 0;
			info->sensitivity.value = 0;
		}
		else
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}


		s = sock_buf;
		p = strstr(s, f_sens_min);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_sens_min);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->sensitivity.min = atoi(buf);


		s = sock_buf;
		p = strstr(s, f_sens_max);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_sens_max);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			//free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->sensitivity.max = atoi(buf);
		info->sensitivity.value = atoi(buf);

		/*
		s = sock_buf;
		p = strstr(s, f_preset_md_count);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_preset_md_count);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		 TODO implement preset motion
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris_max = atoi(buf);
		*/
	}

	/* simulate block draw for techwin(pixel) */
	if(info->method == MAM_RECTANGLE && info->block_width == 0)
	{
		info->method = MAM_SIM_RECTANGLE;
		info->block_width = 32;	// FIXME
		info->block_height = 18;
		info->num_block = 32 * 18;
	}

	//free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 모션 설정을 한다.
 * @param info 모션 설정 struct,
 * @param cam_id 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_set_motion_area(NFIPCamSetupMotionArea* info, int cam_id)
{
	int rtn;
#if 0
	if(1)
	{
		rtn = _ssl_set_motion_area_get(info, cam_id);
	}
#endif
	const char set_motion_raw[] = "action.fcgi?api=set_setup.event.motion%s%s%s%s%s%s%s%s%s%s";
	const char set_area_raw[] = "&area%d_left=%d"
								"&area%d_right=%d"
								"&area%d_top=%d"
								"&area%d_bottom=%d"
								"&area%d_sensitivity=%d";

	int len = 0, i, factor = 1;

	char temp[10][200];
	char tmpbuf[SOCK_BUF_LENGTH];	// XXX
	char *sock_buf, *s, *p;
	char buf[16];
	sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	memset(temp, 0x00, 10 * 200);
	mtable *runtime = get_runtime();
	/* convert block to pixel (for techwin) */
	if(info->method == MAM_SIM_RECTANGLE)
	{
		factor = runtime[cam_id].onvif.width / runtime[cam_id].motion.block_width; // (1920 / 64) = 30
	}
	for(i = 0; i < info->area_num; i++)
	{
		int right = info->marea[i].FIGURE.RECTANGLE.right_bottom.x + 1;
		int bottom = info->marea[i].FIGURE.RECTANGLE.right_bottom.y + 1;

		snprintf(temp[i], 200, set_area_raw,
				i, info->marea[i].FIGURE.RECTANGLE.left_top.x * factor + 1,
				i, right * factor,
				i, info->marea[i].FIGURE.RECTANGLE.left_top.y * factor + 1,
				i, bottom * factor,
				i, info->marea[i].sensitivity
				);
	}
	snprintf(tmpbuf, SOCK_BUF_LENGTH, set_motion_raw,
			temp[0], temp[1], temp[2], temp[3], temp[4],
			temp[5], temp[6], temp[7], temp[8], temp[9]
			);

	if(0)
	{
		rtn = _ssl_send(cam_id, tmpbuf, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, tmpbuf, sock_buf);
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_MOTION, -1);
	free(sock_buf);
	return rtn;
}

/**
 * @brief Mirror capability를 조회한다.
 * @param[out] capability Mirror 지원 모드 bitmask. @see _NF_IPCAM_MIRROR_MODES_E
 * @param[in] cam_id 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_get_mirror_cap(int* capability, int cam_id)
{
	int rtn;
#if 0
	if(1)
	{
		rtn = _ssl_mirror_cap_get(capability, cam_id);
	}
#endif
	char mirror_cap_raw[] = "action.fcgi?api=get_setup.mirror.capability";

	char *sock_buf, *s, *p;
	char buf[16];
	sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	if(0)
	{
		rtn = _ssl_send(cam_id, mirror_cap_raw, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, mirror_cap_raw, sock_buf);
	}

	{
		*capability = 0;

		const char f_vertical[]		= "vertical";
		const char f_horizontal[]	= "horizontal";
		const char f_flip[]			= "flip";
		const char f_endline[]		= "\n";

		s = sock_buf;
		p = strstr(s, f_vertical);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_vertical);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		if(atoi(buf) == 1)
		{
			*capability |= NF_IPCAM_MIRROR_VERTICAL;
		}

		s = sock_buf;
		p = strstr(s, f_horizontal);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_horizontal);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		if(atoi(buf) == 1)
		{
			*capability |= NF_IPCAM_MIRROR_HORIZONTAL;
		}

		s = sock_buf;
		p = strstr(s, f_flip);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_flip);
		/*p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}*/
		while(*s != '=') s++;
		s++;
		//memset(buf, 0x00, 16);
		//memcpy(buf, s, (size_t)(p - s));
		//if(atoi(buf) == 1)
		if(atoi(s) == 1)
		{
			*capability |= NF_IPCAM_MIRROR_FLIP;
		}

	}

	if(*capability != 0)
	{
		*capability |= NF_IPCAM_MIRROR_NONE;
	}

	free(sock_buf);
	return rtn;
}

/**
 * @brief Mirror값을 설정한다.
 * @param info 카메라 설정 struct.
 * @param cam_id 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_set_mirror_val(cam_info* info, int cam_id)
{
	int rtn;
#if 0
	if(1)
	{
		rtn = _ssl_mirror_val_get(info->vcodec.mirror, cam_id);
	}
#endif
	const char mirror_set_raw[] = "action.fcgi?api=set_setup.mirror.mirror_mode&mode=%s";
	const char *str_mirror_mode[] = {"none", "hori", "vert", "flip"};

	char tmpbuf[1024];
	int m_idx;
	char *sock_buf, *s, *p;
	char buf[16];
	sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	switch(info->vcodec.mirror)
	{
		case NF_IPCAM_MIRROR_HORIZONTAL:
			m_idx = 1;
			break;
		case NF_IPCAM_MIRROR_VERTICAL:
			m_idx = 2;
			break;
		case NF_IPCAM_MIRROR_FLIP:
			m_idx = 3;
			break;
		default:
			m_idx = 0;
			break;
	}
	snprintf(tmpbuf, 1024, mirror_set_raw, str_mirror_mode[m_idx]);

	if(0)
	{
		rtn = _ssl_send(cam_id, tmpbuf, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, tmpbuf, sock_buf);
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_MIRROR, -1);

	free(sock_buf);
	return rtn;
}

/**
 * @brief Onepush capability를 조회한다.
 * @param[out] capability Onepush 지원 여부.
 * @param[in] cam_id 채널 번호/
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_get_onepush_cap(int* capability, int cam_id)
{
	int rtn;
#if 0
	if(1)
	{
		rtn = _ssl_onepush_cap_get(capability, cam_id);
	}
#endif
	char onepush_cap_raw[] = "action.fcgi?api=get_setup.onepush.capability";

	char *sock_buf, *s, *p;
	char buf[16];
	sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	if(0)
	{
		rtn = _ssl_send(cam_id, onepush_cap_raw, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, onepush_cap_raw, sock_buf);
	}
	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		free(sock_buf);
		return rtn;
	}

	{
		*capability = 0;

		const char f_onepush[]		= "onepush";
		const char f_endline[]		= "\n";

		s = sock_buf;
		p = strstr(s, f_onepush);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_onepush);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		while(*s != '=') s++;
		s++;
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		if(atoi(buf) == 1)
		{
			*capability = 1;
		}
	}

	free(sock_buf);
	return rtn;
}

/**
 * @brief Onepush 기능을 실행한다.
 * @param cam_id 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_set_onepush(int cam_id)
{
	int rtn;
#if 0
	if(1)
	{
		rtn = _ssl_set_onepush_get(cam_id);
	}
#endif
	char onepush_raw[] = "action.fcgi?api=set_setup.onepush.go";

	char *sock_buf, *s, *p;
	char buf[16];
	sock_buf = (char *)malloc(SOCK_BUF_LENGTH);

	if(0)
	{
		rtn = _ssl_send(cam_id, onepush_raw, sock_buf);
	}
	else
	{
		rtn = _plain_send(cam_id, onepush_raw, sock_buf);
	}

	//nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_ONESHOT);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ONESHOT, -1);
	free(sock_buf);

	return rtn;
}

/**
 * @brief S1 이기종 프로토콜에 맞게 factory clear 명령을 실행한다.
 * @param cam_id 채널 번호.
 * @param user 사용자 ID.
 * @param pass 사용자 비밀번호.
 * @param macaddr 카메라 맥주소.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
extern int s1_factory_clear(int cam_id, const char* user, const char* pass, char* macaddr)
{
	int rtn;
	if(1)
	{
		rtn = _ssl_factory_clear(cam_id, user, pass, macaddr);
	}

	return rtn;
}

/**
 * @brief SSL을 사용하여 factory clear 명령을 실행한다.
 * @param cam_id 채널 번호.
 * @param user 사용자 ID.
 * @param pass 사용자 비밀번호.
 * @param macaddr 카메라 맥주소.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
static int _ssl_factory_clear(int cam_id, char* user, char* pass, char* macaddr)
{
	const char factory_clear_raw[] = "factory.fcgi?api=set_setup.security.factory_clear&authtoken=%s&authpass=%s";

	char auth_encbuf[256];
	char factory_clear_api[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	char authtoken[64];
	unsigned char cal_mac[3];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;

	int len = 0;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);

#if PRINT_HTTP_API_SEND
	printf("%s port(%d) user(%s) pass(%s) mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
		__FUNCTION__,
		cam_id, user, pass,
		runtime[cam_id].sys.macaddr[0],
		runtime[cam_id].sys.macaddr[1],
		runtime[cam_id].sys.macaddr[2],
		runtime[cam_id].sys.macaddr[3],
		runtime[cam_id].sys.macaddr[4],
		runtime[cam_id].sys.macaddr[5]);
#endif

	cal_mac[0] = (unsigned char)((runtime[cam_id].sys.macaddr[0] + ((runtime[cam_id].sys.macaddr[5]&0xff)/2)));
	cal_mac[1] = (unsigned char)(runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + ((runtime[cam_id].sys.macaddr[4]&0xff)/3)));

	snprintf(authtoken, 64, "%02x%02x%02x", cal_mac[0], cal_mac[1], cal_mac[2]);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(factory_clear_api, 256, factory_clear_raw, authtoken, pass);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get_noauth, factory_clear_api, ip_str);

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
		free(sock_buf);
		return -1;
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
			_release_resource(&sock, sock_buf, NULL, NULL);
			return -1;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, sock_buf, NULL, NULL);
		return -1;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		_release_resource(&sock, sock_buf, NULL, NULL);
		return -1;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		_release_resource(&sock, sock_buf, NULL, &ctx);
		return -1;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return -1;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return -1;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return -1;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			//len = _ssl_factory_clear_digest(cam_id, user, pass, macaddr, sock_buf);
			free(sock_buf);
			return len;
		}
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return -1;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 2 : \n%s\n", sock_buf);
#endif
	}

	{
		const char f_result[]		= "result";
		const char f_authuser[]		= "authuser";
		const char f_macaddr[]		= "macaddr";
		const char f_endline[]		= "\r\n";

		s = sock_buf;
		p = strstr(s, f_result);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_result);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* api fail!(passwd chg fail) */
		if(strstr(s, "false"))
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return 0;
		}

		s = sock_buf;
		p = strstr(s, f_authuser);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_authuser);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* find user id */
		while(*s == '=') s++;
		while(*s == ' ') s++;

		memcpy(user, s, (size_t)(p - s));

		s = sock_buf;
		p = strstr(s, f_macaddr);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_macaddr);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* find mac address */
		while(!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))) s++;

		memset(macaddr, 0x00, 13);
		memcpy(macaddr, s, 12);
	}

	_release_resource(&sock, sock_buf, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief SSL을 사용하여 factory clear 명령을 실행한다.(Digest 인증과정 거침)
 * @param cam_id 채널 번호.
 * @param user 사용자 ID.
 * @param pass 사용자 비밀번호.
 * @param macaddr 카메라 맥주소.
 * @param rbuf Basic인증시 수신한 전문.(nonce등 parsing용도)
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 사용안함.
 */
static int _ssl_factory_clear_digest(int cam_id, char* user, char* pass, char* macaddr, char *rbuf)
{
	const char factory_clear_raw[] = "factory.fcgi?api=set_setup.security.factory_clear&authtoken=%s&authpass=%s";

	char factory_clear_api[256];
	char auth_str[1024];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	char authtoken[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	unsigned char cal_mac[3];

	mtable* runtime = get_runtime();
	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 3;
	snprintf(authtoken, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

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

	snprintf(uri, 128, "https://%s:%d/cgi-bin/factory.fcgi", ip_str, http_port);

	itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);

	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_digest, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(factory_clear_api, 256, factory_clear_raw, authtoken, pass);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, factory_clear_api, ip_str, auth_str);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return -1;
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
			_release_resource(&sock, sock_buf, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return -1;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (1);
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 2 : \n%s\n", sock_buf);
#endif
	}

	{
		const char f_result[]		= "result";
		const char f_authuser[]		= "authuser";
		const char f_macaddr[]		= "macaddr";
		const char f_endline[]		= "\n";

		s = sock_buf;
		p = strstr(s, f_result);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_result);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* api fail!(passwd chg fail) */
		if(strstr(s, "false"))
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return 0;
		}

		s = sock_buf;
		p = strstr(s, f_authuser);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_authuser);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* find user id */
		while(*s == '=') s++;
		while(*s == ' ') s++;

		memcpy(user, s, (size_t)(p - s));

		s = sock_buf;
		p = strstr(s, f_macaddr);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}
		s = p + strlen(f_macaddr);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return -1;
		}

		/* find mac address */
		while(!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))) s++;

		memset(macaddr, 0x00, 13);
		memcpy(macaddr, s, 12);
	}

	_release_resource(&sock, sock_buf, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

extern int s1_factory_default(int cam_id)
{
	int rtn;
	if(1)
	{
		rtn = _ssl_factory_default(cam_id);
	}

	return rtn;
}

static int _ssl_factory_default(int cam_id)
{
	const char factory_default_raw[] = "factory.fcgi?api=set_setup.security.factory_load";

	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned char cal_mac[3];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;

	int len = 0;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	//nf_ipcam_get_password(cam_id, password);
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 3;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	itx_basic_auth_str(username, password, auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, factory_default_raw, ip_str, auth_encbuf);

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
		free(sock_buf);
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
			perror("setsockopt");
			_release_resource(&sock, sock_buf, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, sock_buf, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		_release_resource(&sock, sock_buf, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		_release_resource(&sock, sock_buf, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			len = _ssl_factory_default_digest(cam_id, sock_buf);
			free(sock_buf);
			return len;
		}
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 2 : \n%s\n", sock_buf);
#endif
	}

	_release_resource(&sock, sock_buf, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_factory_default_digest(int cam_id, char *rbuf)
{
	const char factory_default_raw[] = "factory.fcgi?api=set_setup.security.factory_load";

	char auth_str[1024];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	unsigned char cal_mac[3];
	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	mtable* runtime = get_runtime();

	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl;
	SSL_CTX *ctx;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	//nf_ipcam_get_password(cam_id, password);
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 3;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

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

	snprintf(uri, 128, "https://%s:%d/cgi-bin/factory.fcgi", ip_str, http_port);

	itx_digest_auth_str(username,password,realm,nonce,uri,method,auth_str);

	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_digest, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_get, factory_default_raw, ip_str, auth_str);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
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
			_release_resource(&sock, sock_buf, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		printf("[%s] SSL context creation failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, NULL, NULL);
		return (-1);
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		printf("[%s] SSL creation failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, NULL, &ctx);
		return (-1);
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "200 OK") == NULL)
	{
		printf("[%s] ERROR: HTTP error(%s)\n", __FUNCTION__, sock_buf);
		_release_resource(&sock, sock_buf, &ssl, &ctx);
		return (1);
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("content : \n%s\n", sock_buf);
#endif
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 1 : \n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
		{
			perror("read");
			_release_resource(&sock, sock_buf, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("chunk 2 : \n%s\n", sock_buf);
#endif
	}

	_release_resource(&sock, sock_buf, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _s1_check_encoding_character(char *pass, int pass_len, char* encoding_pass)
{	
	//const char reserved_char[] = { '!', '*', '\'', '(', ')', ';', ':', '@', '&', '=', '+', '$', ',', '/', '?', '#', '[', ']', ' '};
	const char reserved_char[] = 
	{ 
		'~', '-', '=', '{', '}', '\\', '.', '"', '<', '>', '?', '`', '!', '@', '#', '$', 
		'%', '^', '&', '*', '(', ')', '_', '+', '[', ']', '|', ':', '\'', ',', ';', '/', ' '
	};

	int i, j = 0;
	int n;
	char buf[6];
	int change_flag = 0;

	if (pass == NULL || pass_len <= 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	for (i = 0, n = 0; i < pass_len; i++)
	{
		for (j = 0; j < sizeof(reserved_char) / sizeof(char); j++)
		{
			if (pass[i] == reserved_char[j])
			{
				change_flag = 1;
				snprintf(buf, 4, "%%%02X", reserved_char[j]);
				strcat(encoding_pass, buf);
				n += 2;
				break;
			}
		}
		if(change_flag == 0)
		{
			encoding_pass[n] = pass[i];
		}
		change_flag = 0;
		n++;
	}
	return IPCAM_SETUP_RTN_DONE;

}

static void random_id( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len / 2; ++i )
		sprintf( (char *)(dest + i * 2), "%02X",
				(unsigned int)( random() & 0xff ) );
	dest[len] = 0;
}

void _create_nonce(char *nonce)
{
	static char digest_secret[17];
	memset(digest_secret, 0x00, 17);
	random_id((unsigned char*)digest_secret, 16);
	create_nonce(nonce, digest_secret);
}

int digest_auth_create(char *username, char *password, char *api_str, char *http_method, int *noncecount, char *recv_buf, char *auth_str)
{

	char *s = NULL, *p = NULL;
	const char f_str_realm[] = "realm=\"";
	const char f_str_qop[] = "qop=\"";
	const char f_str_nonce[] = "nonce=\"";
	const char f_str_opaque[] = "opaque=\"";

	char qop[16];
	char opaque[128];
	char realm[128];
	char nonce[128];
	char clientnonce[128];

	memset(qop, 0x00, sizeof(qop));
	memset(opaque, 0x00, sizeof(opaque));
	memset(realm, 0x00, sizeof(realm));
	memset(nonce, 0x00, sizeof(nonce));
	memset(clientnonce, 0x00, sizeof(clientnonce));
	
	int qopused = 0;

	s = strstr(recv_buf, f_str_realm);
	if (s == NULL)
	{
		IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_str_realm);
	p = strstr(s, "\"");
	if (p == NULL)
	{
		IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(realm, 0x00, 128);
	memcpy(realm, s, p-s);

	s = strstr(recv_buf, f_str_nonce);
	if (s == NULL)
	{
		IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_str_nonce);
	p = strstr(s, "\"");
	if (p == NULL)
	{
		IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, p-s);

	/* RFC 2617 .. */
	s = strstr(recv_buf, f_str_qop);
	if(s != NULL)
	{
		qopused = 1;
		s += strlen(f_str_qop);
		p = strstr(s, "\"");
		if(p == NULL)
		{
			IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(qop, 0x00, 16);
		memcpy(qop, s, p-s);

		memset(opaque, 0x00, 128);
		s = strstr(recv_buf, f_str_opaque);
		if(s != NULL)
		{
			s+= strlen(f_str_opaque);
			p = strstr(s, "\"");
			if(p == NULL)
			{
				IPCAM_DBG(ERROR, "%d parsing digest ERROR!\n", __LINE__);
				return IPCAM_SETUP_RTN_FAILED;
			}
			memcpy(opaque, s, p-s);
		}
	}


	if(qopused)
	{
		(*noncecount)++;
		_create_nonce(clientnonce);
		itx_digest_auth_str_qop(username, password, realm, nonce, api_str, http_method, *noncecount, clientnonce, qop, opaque, auth_str);
	}
	else
	{
		itx_digest_auth_str(username, password, realm, nonce, api_str, http_method, auth_str);
	}

	return IPCAM_SETUP_RTN_DONE;
}

#endif	//__NF_IPCAM_DRIVER_S1_C__

