/*
 * ITX Security
 *  System software group
 *
 *  2016-02-24 sjlim
 */

#ifndef __NF_IPCAM_DRIVER_SONY_C__
#define __NF_IPCAM_DRIVER_SONY_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include "nf_ipcam_driver_sony.h"

/* -------------  LOCAL DEFINE   --------------- */

#define PRINT_HTTP_API_SEND	(1)

#define SONY_MOTION_COLUMNS	1	
#define SONY_MOTION_ROWS	1

#define SONY_MOTION_WIDTH	1	
#define SONY_MOTION_HIGHT	1	

#define SONY_MOTION_AREA_WIDTH	((SONY_MOTION_WIDTH)/(SONY_MOTION_COLUMNS))
#define SONY_MOTION_AREA_HIGHT	((SONY_MOTION_HIGHT)/(SONY_MOTION_ROWS))

/* -------------  LOCAL DATA --------------- */

static const char param_api_raw[] = "GET /command/camera.cgi?%s HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

//2nd_profile api : fw_ver check or KOBI vendor check
static const char param_api_2nd_profile_setting[] = 
		"AutoRateCtrl1=off&AutoRateCtrl2=off&ImageCodec1=h264"
		"&ImageSize1=1920,1080&FrameRate1=30&IFrameRatio1=30&H264Profile1=high&CBR1=off"
		"&H264Quality1=6&VBRMode1=standard&ImageCodec2=h264&ImageSize2=640,360"
		"&FrameRate2=30&IFrameRatio2=30&H264Profile2=high&CBR2=off&H264Quality2=6"
		"&VBRMode2=standard&ImageCodec3=off&IFrameInterval1=0&IFrameInterval2=0";

/* -------------  Define FUNC --------------- */
extern int sony_init_2nd_profiles(int cam_id);

/* -------------  IPX SONY Camera API   --------------- */

extern int sony_init_2nd_profiles(int cam_id)
{
	char 	auth_encbuf[256];
	char	username[64];
	char 	password[64];
	char	ip_str[64];

	char 	http_api[2048] = {0,};
	char	sock_buf[4096] = {0,};

	int 	sock = 0;
	int 	rtn = 0;

	unsigned short http_port = 0;
	struct sockaddr_in sin;
	int rtnVal = 0, recvLen = 0, totalRecvLen = 0;

	// Get Server IP
	nf_ipcam_get_ipstr(cam_id, ip_str);

	// Get Server Port
	http_port = nf_ipcam_get_http_port(cam_id);

	// Get Auth Info
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	// Base 64 Encode
	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);//username

	//set api 
	snprintf(http_api, 2047, param_api_2nd_profile_setting);
	snprintf(sock_buf, 4095, param_api_raw, http_api, ip_str, auth_encbuf);

	// Set Socket
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family 	= AF_INET;
	sin.sin_port 	= htons(http_port);
	sin.sin_addr.s_addr = inet_addr(ip_str);

	// Create Socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		rtn = sock;
		goto end_label;
	}

	//time out check
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(WARN, "Sony set 2nd stream profile socket recv timeout fail. CH(%d) \n", cam_id);
			perror("setsockopt");
			close(sock);
			rtn = sock;
			goto end_label;
		}
	}

	// Connection check
	if(connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) 
	{
		IPCAM_DBG(WARN, "Sony set 2nd stream profile socket connect fail. CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		rtn = sock;
		goto end_label;
	}

#if PRINT_HTTP_API_SEND
	printf("[%s] Sony http_api : %s\n", __FUNCTION__, param_api_raw);
#endif

	// Send check
	if(send(sock, sock_buf, strlen(sock_buf), 0) < 0) 
	{
		IPCAM_DBG(WARN, "Sony set image send fail. CH(%d) \n", cam_id);
		perror("send");
		close(sock);
		rtn = sock;
		goto end_label;
	}
	

end_label:

	return rtn;
}

/* -------------   STATIC FUNCTION   --------------- */

#endif //__NF_IPCAM_DRIVER_SONY_C__
