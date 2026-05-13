/*
 * ITX Security
 *  System software group
 *
 *  2012-07-20 jykim
 */

#ifndef __NF_IPCAM_DRIVER_XIONGMAI_C__
#define __NF_IPCAM_DRIVER_XIONGMAI_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include <glib.h>
// #include <gst/gst.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_ptz.h>
#include <nf_util_netif.h>
#include <jansson.h>
#include "nf_ipcam_driver_xiongmai.h"
#include "nf_ipcam_driver_itx_md5.h"




#define PRINT_HTTP_API_SEND	(0)

static const char xm_set_raw[] = 
	"POST /cgi-bin/setconfig.cgi HTTP/1.1\r\n"
	"Content-Length: %d\r\n" // length
	"\r\n"
	"%s";


static void _xmbuf_alloc(NF_IPCAM_XM_BUFS* xmbufs);
static void _xmbuf_free(NF_IPCAM_XM_BUFS* xmbufs);


static cam_info last_stream_info[AVAILABLE_MAX_CH];


extern int xiongmai_alarm_server_set(int cam_id)
{
	int sock;
	int len = 0;
	char *json_post_api = NULL;
	struct sockaddr_in sin;
	json_t *obj_root;
	json_t *arr_network_dot_alarm_server;
	json_t *obj_server;
	json_t *obj_server_config;
	NF_IPCAM_XM_BUFS xmbufs;

	char ip_str[16];
	char host_ip_str[16];
	mtable *runtime = get_runtime();


	// 최조 접속시에는 마지막 스트림설정이 초기화된다//
	memset(&last_stream_info[cam_id], 0x00, sizeof(cam_info));
	///////////////////////////////////////////////////
	if (nf_get_running_mode())
	{
		NF_NETIF_IP host_ip;
		if (nf_netif_get_ip(&host_ip) != TRUE)
		{
			printf("WARN [%s] CH(%d) nf_netif_get_ip failed\n", __FUNCTION__, cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		snprintf(host_ip_str, 16, "%d.%d.%d.%d",
				host_ip.ip_addr[0], host_ip.ip_addr[1],
				host_ip.ip_addr[2], host_ip.ip_addr[3]);
	}
	else
	{
		unsigned char sys_mac[8];
		nf_netif_get_mac_str(&sys_mac[0]);
		snprintf(host_ip_str, 16, "10.%d.%d.1", sys_mac[4], sys_mac[5]);
#if 0


		unsigned int host_ip = get_host_info();
		if (host_ip == 0)
		{
			printf("WARN [%s] CH(%d) host ip 0\n", __FUNCTION__, cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		snprintf(host_ip_str, 16, "%d.%d.%d.%d",
				host_ip&0xff, host_ip&0xff00>>8,
				host_ip&0xff0000>>16, host_ip&0xff000000>>24);
#endif
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);

	_xmbuf_alloc(&xmbufs);

	obj_root = json_object();
	md5("", xmbufs.pw_enc);
	json_object_set_new(obj_root, "PassWord", json_string(xmbufs.pw_enc));
	json_object_set_new(obj_root, "UserName", json_string("admin"));
	json_object_set_new(obj_root, "Name", json_string("NetWork.AlarmServer"));


	arr_network_dot_alarm_server = json_array();
		obj_server = json_object();
		json_object_set_new(obj_server, "Alarm", json_true());
		json_object_set_new(obj_server, "Enable", json_true());
		json_object_set_new(obj_server, "Log", json_false());
		json_object_set_new(obj_server, "Protocol", json_string("GENERAL"));
			obj_server_config = json_object();
			json_object_set_new(obj_server_config, "Address", json_string("0x00000000"));
			json_object_set_new(obj_server_config, "Anonymity", json_false());
			json_object_set_new(obj_server_config, "Name", json_string(host_ip_str));
			json_object_set_new(obj_server_config, "Password", json_string(""));
			json_object_set_new(obj_server_config, "Port", json_integer(XM_DEFAULT_ALARM_SERVER_PORT));
			json_object_set_new(obj_server_config, "UserName", json_string(""));
		json_object_set(obj_server, "Server", obj_server_config);
		json_decref(obj_server_config);
	json_array_append(arr_network_dot_alarm_server, obj_server);
	json_decref(obj_server);
		obj_server = json_object();
		json_object_set_new(obj_server, "Alarm", json_true());
		json_object_set_new(obj_server, "Enable", json_true());
		json_object_set_new(obj_server, "Log", json_false());
		json_object_set_new(obj_server, "Protocol", json_string("GENERAL"));
			obj_server_config = json_object();
			json_object_set_new(obj_server_config, "Address", json_string("0x00000000"));
			json_object_set_new(obj_server_config, "Anonymity", json_false());
			json_object_set_new(obj_server_config, "Name", json_string("AlarmServer"));
			json_object_set_new(obj_server_config, "Password", json_string(""));
			json_object_set_new(obj_server_config, "Port", json_integer(XM_DEFAULT_ALARM_SERVER_PORT));
			json_object_set_new(obj_server_config, "UserName", json_string(""));
		json_object_set(obj_server, "Server", obj_server_config);
		json_decref(obj_server_config);
	json_array_append(arr_network_dot_alarm_server, obj_server);
	json_array_append(arr_network_dot_alarm_server, obj_server);
	json_array_append(arr_network_dot_alarm_server, obj_server);
	json_array_append(arr_network_dot_alarm_server, obj_server);
	json_decref(obj_server);
	json_object_set(obj_root, "NetWork.AlarmServer", arr_network_dot_alarm_server);
	json_decref(arr_network_dot_alarm_server);


	json_post_api = json_dumps(obj_root, 0);
	if (json_post_api != NULL)
	{
		strcpy(xmbufs.post_api, json_post_api);
		free(json_post_api);
		json_post_api = NULL;
	}
	json_decref(obj_root);

	memset(xmbufs.buf, 0x00, XM_DATA_BUF_SIZE);
	snprintf(xmbufs.http_api, XM_DATA_BUF_SIZE, xm_set_raw, strlen(xmbufs.post_api), xmbufs.post_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(80);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = send(sock, xmbufs.http_api, strlen(xmbufs.http_api), 0);
#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) SEND\n%s\n", __FUNCTION__, cam_id, xmbufs.http_api);
#endif
	if (len < 0)
	{
		printf("send failed\n");
		perror("send");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	int pos = 0;
	do {
		len = recv(sock, xmbufs.buf+pos, XM_DATA_BUF_SIZE-pos, 0);
		if (len < 0)
		{
			printf("recv failed\n");
			perror("recv");
			close(sock);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
		else if (len == 0)
		{
			printf("server closed\n");
			perror("recv");
			close(sock);
			break;
		}

		pos += len;
	} while(1);

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) RECV\n%s\n", __FUNCTION__, cam_id, xmbufs.buf);
#endif
	_xmbuf_free(&xmbufs);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, -1);
	return IPCAM_SETUP_RTN_DONE;
}

extern int xiongmai_osd_off(int cam_id)
{
	int sock;
	int len = 0;
	char *json_post_api = NULL;
	struct sockaddr_in sin;
	json_t *obj_root;
	json_t *obj_video_widget;
	json_error_t err;
	NF_IPCAM_XM_BUFS xmbufs;
	const char *json_str_to_set = 
		"["
			"{"
				"\"ChannelTitle\" : {"
					"\"Name\" : \"CAM01\","
					"\"SerialNo\" : \"20081125\""
				"},"
				"\"ChannelTitleAttribute\" : {"
					"\"BackColor\" : \"0x80000000\","
					"\"EncodeBlend\" : false,"
					"\"FrontColor\" : \"0xF0FFFFFF\","
					"\"PreviewBlend\" : true,"
					"\"RelativePos\" : [ 570, 7552, 255, 24 ]"
				"},"
				"\"Covers\" : ["
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"},"
					"{"
						"\"BackColor\" : \"0x80000000\","
						"\"EncodeBlend\" : false,"
						"\"FrontColor\" : \"0xF0FFFFFF\","
						"\"PreviewBlend\" : false,"
						"\"RelativePos\" : [ 1024, 1024, 2048, 2048 ]"
					"}"
				"],"
				"\"CoversNum\" : 0,"
				"\"TimeTitleAttribute\" : {"
					"\"BackColor\" : \"0x80000000\","
					"\"EncodeBlend\" : false,"
					"\"FrontColor\" : \"0xF0FFFFFF\","
					"\"PreviewBlend\" : true,"
					"\"RelativePos\" : [ 5166, 512, 255, 24 ]"
				"}"
			"}"
		"]";
	char ip_str[16];
	mtable *runtime = get_runtime();


	_xmbuf_alloc(&xmbufs);
	nf_ipcam_get_ipstr(cam_id, ip_str);

	obj_root = json_object();
	md5("", xmbufs.pw_enc);

	json_object_set_new(obj_root, "PassWord", json_string(xmbufs.pw_enc));
	json_object_set_new(obj_root, "UserName", json_string("admin"));
	json_object_set_new(obj_root, "Name", json_string("AVEnc.VideoWidget"));

	obj_video_widget = json_loads(json_str_to_set,0,&err);
	json_object_set(obj_root, "AVEnc.VideoWidget", obj_video_widget);
	json_decref(obj_video_widget);

	json_post_api = json_dumps(obj_root, 0);
	if (json_post_api != NULL)
	{
		strcpy(xmbufs.post_api, json_post_api);
		free(json_post_api);
		json_post_api = NULL;
	}
	json_decref(obj_root);

	memset(xmbufs.buf, 0x00, XM_DATA_BUF_SIZE);
	snprintf(xmbufs.http_api, XM_DATA_BUF_SIZE, xm_set_raw, strlen(xmbufs.post_api), xmbufs.post_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(80);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = send(sock, xmbufs.http_api, strlen(xmbufs.http_api), 0);
#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) SEND\n%s\n", __FUNCTION__, cam_id, xmbufs.http_api);
#endif
	if (len < 0)
	{
		printf("send failed\n");
		perror("send");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	int pos = 0;
	do {
		len = recv(sock, xmbufs.buf+pos, XM_DATA_BUF_SIZE-pos, 0);
		if (len < 0)
		{
			printf("recv failed\n");
			perror("recv");
			close(sock);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
		else if (len == 0)
		{
			printf("server closed\n");
			perror("recv");
			close(sock);
			break;
		}

		pos += len;
	} while(1);

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) RECV\n%s\n", __FUNCTION__, cam_id, xmbufs.buf);
#endif
	_xmbuf_free(&xmbufs);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, -1);
	return IPCAM_SETUP_RTN_DONE;

}

extern int xiongmai_prevent_periodical_reboot(int cam_id)
{
	int sock;
	int len = 0;
	char *json_post_api = NULL;
	struct sockaddr_in sin;
	json_t *obj_root;
	json_t *obj_auto_maintain;
	json_error_t err;
	NF_IPCAM_XM_BUFS xmbufs;
	const char *json_str_to_set = 
		"{"
			"\"AutoDeleteFilesDays\" : 0,"
			"\"AutoRebootDay\" : \"Never\","
			"\"AutoRebootHour\" : 1"
		"}";
	char ip_str[16];
	mtable *runtime = get_runtime();

	_xmbuf_alloc(&xmbufs);
	nf_ipcam_get_ipstr(cam_id, ip_str);

	obj_root = json_object();
	md5("", xmbufs.pw_enc);

	json_object_set_new(obj_root, "PassWord", json_string(xmbufs.pw_enc));
	json_object_set_new(obj_root, "UserName", json_string("admin"));
	json_object_set_new(obj_root, "Name", json_string("General.AutoMaintain"));

	snprintf(xmbufs.buf, XM_DATA_BUF_SIZE, json_str_to_set);

	obj_auto_maintain = json_loads(xmbufs.buf,0,&err);
	json_object_set(obj_root, "General.AutoMaintain", obj_auto_maintain);
	json_decref(obj_auto_maintain);

	json_post_api = json_dumps(obj_root, 0);
	if (json_post_api != NULL)
	{
		strcpy(xmbufs.post_api, json_post_api);
		free(json_post_api);
		json_post_api = NULL;
	}
	json_decref(obj_root);

	memset(xmbufs.buf, 0x00, XM_DATA_BUF_SIZE);
	snprintf(xmbufs.http_api, XM_DATA_BUF_SIZE, xm_set_raw, strlen(xmbufs.post_api), xmbufs.post_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(80);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = send(sock, xmbufs.http_api, strlen(xmbufs.http_api), 0);
#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) SEND\n%s\n", __FUNCTION__, cam_id, xmbufs.http_api);
#endif
	if (len < 0)
	{
		printf("send failed\n");
		perror("send");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	int pos = 0;
	do {
		len = recv(sock, xmbufs.buf+pos, XM_DATA_BUF_SIZE-pos, 0);
		if (len < 0)
		{
			printf("recv failed\n");
			perror("recv");
			close(sock);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
		else if (len == 0)
		{
			printf("server closed\n");
			perror("recv");
			close(sock);
			break;
		}

		pos += len;
	} while(1);

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) RECV\n%s\n", __FUNCTION__, cam_id, xmbufs.buf);
#endif
	_xmbuf_free(&xmbufs);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM2, -1);
	return IPCAM_SETUP_RTN_DONE;

}

static int _xm_is_different_config(int cam_id, cam_info* info);
extern int xiongmai_stream_set(int cam_id, cam_info* info)
{
	int sock;
	int len = 0;
	char *json_post_api = NULL;
	struct sockaddr_in sin;
	json_t *obj_root;
	json_t *obj_simplify_encode;
	json_error_t err;
	NF_IPCAM_XM_BUFS xmbufs;
	const char *json_str_to_set = 
		"["
			"{"
				"\"ExtraFormat\" : {"
					"\"AudioEnable\" : false,"
					"\"Video\" : {"
						//"\"BitRate\" : 1500,"
						"\"BitRate\" : %d,"
						"\"BitRateControl\" : \"VBR\","
						"\"Compression\" : \"H.264\","
						//"\"FPS\" : 30,"
						"\"FPS\" : %d,"
						//"\"GOP\" : 12,"
						"\"GOP\" : 1,"
						//"\"Quality\" : 3,"
						"\"Quality\" : %d,"
						"\"Resolution\" : \"CIF\""
					"},"
					"\"VideoEnable\" : true"
				"},"
				"\"MainFormat\" : {"
					"\"Audio\" : {"
						"\"BitRate\" : 8,"
						"\"MaxVolume\" : 7,"
						"\"SampleRate\" : 8000"
					"},"
					"\"AudioEnable\" : true,"
					"\"Video\" : {"
						//"\"BitRate\" : 4000,"
						"\"BitRate\" : %d,"
						"\"BitRateControl\" : \"VBR\","
						"\"Compression\" : \"H.264\","
						//"\"FPS\" : 30,"
						"\"FPS\" : %d,"
						//"\"GOP\" : 12,"
						"\"GOP\" : 1,"
						//"\"Quality\" : 4,"
						"\"Quality\" : %d,"
						"\"Resolution\" : \"1080P\""
					"},"
					"\"VideoEnable\" : true"
				"}"
			"}"
		"]";
	char ip_str[16];
	mtable *runtime = get_runtime();


	if(!_xm_is_different_config(cam_id, info))
	{
		printf("%s CH(%d) same config skip\n", __FUNCTION__, cam_id);
		nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VCODEC, -1);
		return IPCAM_SETUP_RTN_DONE;
	}
	memcpy(&last_stream_info[cam_id], info, sizeof(cam_info));


	_xmbuf_alloc(&xmbufs);
	nf_ipcam_get_ipstr(cam_id, ip_str);

	obj_root = json_object();
	md5("", xmbufs.pw_enc);

	json_object_set_new(obj_root, "PassWord", json_string(xmbufs.pw_enc));
	json_object_set_new(obj_root, "UserName", json_string("admin"));
	json_object_set_new(obj_root, "Name", json_string("Simplify.Encode"));

#if defined(GUI_16CH_SUPPORT) || defined(GUI_32CH_SUPPORT)
	snprintf(xmbufs.buf, XM_DATA_BUF_SIZE, json_str_to_set,
			500+info->vcodec.bitrate[1]*100, _get_fps_num(info->vcodec.fps[1]), 2+info->vcodec.bitrate[1],
			1000+info->vcodec.bitrate[0]*800, _get_fps_num(info->vcodec.fps[0]), 2+info->vcodec.bitrate[0]);
#else
	snprintf(xmbufs.buf, XM_DATA_BUF_SIZE, json_str_to_set,
			500+info->vcodec.bitrate[1]*100, _get_fps_num(info->vcodec.fps[1]), 2+info->vcodec.bitrate[1],
			2000+info->vcodec.bitrate[0]*1500, _get_fps_num(info->vcodec.fps[0]), 2+info->vcodec.bitrate[0]);
#endif
	obj_simplify_encode = json_loads(xmbufs.buf,0,&err);
	json_object_set(obj_root, "Simplify.Encode", obj_simplify_encode);
	json_decref(obj_simplify_encode);

	json_post_api = json_dumps(obj_root, 0);
	if (json_post_api != NULL)
	{
		strcpy(xmbufs.post_api, json_post_api);
		free(json_post_api);
		json_post_api = NULL;
	}
	json_decref(obj_root);

	memset(xmbufs.buf, 0x00, XM_DATA_BUF_SIZE);
	snprintf(xmbufs.http_api, XM_DATA_BUF_SIZE, xm_set_raw, strlen(xmbufs.post_api), xmbufs.post_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(80);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = send(sock, xmbufs.http_api, strlen(xmbufs.http_api), 0);
#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) SEND\n%s\n", __FUNCTION__, cam_id, xmbufs.http_api);
#endif
	if (len < 0)
	{
		printf("send failed\n");
		perror("send");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	int pos = 0;
	do {
		len = recv(sock, xmbufs.buf+pos, XM_DATA_BUF_SIZE-pos, 0);
		if (len < 0)
		{
			printf("recv failed\n");
			perror("recv");
			close(sock);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
		else if (len == 0)
		{
			printf("server closed\n");
			perror("recv");
			close(sock);
			break;
		}

		pos += len;
	} while(1);

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) RECV\n%s\n", __FUNCTION__, cam_id, xmbufs.buf);
#endif
	_xmbuf_free(&xmbufs);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VCODEC, -1);
	return IPCAM_SETUP_RTN_DONE;

}

static void _xm_convert_marea_sysdb_to_xm(const char* sysdb_area, char **xm_area);
extern int xiongmai_motion_set(NFIPCamSetupMotionArea *motion_info, int cam_id)
{
	int sock;
	int len = 0;
	int i=0;
	int sensitivity = 0;
	char *json_post_api = NULL;
	struct sockaddr_in sin;
	json_t *obj_root;
	json_t *arr_detect_dot_motion_detect;
	json_t *obj_motion;
	json_t *obj_event_handler;
	json_t *arr_ptz_link;
	json_t *arr_ptz_link_tuple;
	json_t *arr_time_section;
	json_t *arr_time_section_tuple;
	json_t *arr_region;
	NF_IPCAM_XM_BUFS xmbufs;

	char ip_str[16];



	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);


	_xmbuf_alloc(&xmbufs);
	sensitivity = motion_info->marea[0].sensitivity;
	_xm_convert_marea_sysdb_to_xm(motion_info->marea[0].FIGURE.CELL.active_cell, xmbufs.dstbuf);


	obj_root = json_object();


	/* Auth info set */
	md5("", xmbufs.pw_enc);
	json_object_set_new(obj_root, "PassWord", json_string(xmbufs.pw_enc));
	json_object_set_new(obj_root, "UserName", json_string("admin"));
	json_object_set_new(obj_root, "Name", json_string("Detect.MotionDetect"));
	/* Auth info set end */


	/* Motion info set */
	arr_detect_dot_motion_detect = json_array();

		obj_motion = json_object();
		json_object_set_new(obj_motion, "Enable", json_true());

			obj_event_handler = json_object();
			json_object_set_new(obj_event_handler, "AlarmInfo", json_string(""));
			json_object_set_new(obj_event_handler, "AlarmOutEnable", json_false());
			json_object_set_new(obj_event_handler, "AlarmOutLatch", json_integer(10));
			json_object_set_new(obj_event_handler, "AlarmOutMask", json_string("0x00000000"));
			json_object_set_new(obj_event_handler, "BeepEnable", json_false());
			json_object_set_new(obj_event_handler, "EventLatch", json_integer(2));
			json_object_set_new(obj_event_handler, "FTPEnable", json_false());
			json_object_set_new(obj_event_handler, "LogEnable", json_false());
			json_object_set_new(obj_event_handler, "MailEnable", json_false());
			json_object_set_new(obj_event_handler, "MatrixEnable", json_false());
			json_object_set_new(obj_event_handler, "MatrixMask", json_string("0x00000000"));
			json_object_set_new(obj_event_handler, "MessageEnable", json_false());
			json_object_set_new(obj_event_handler, "MsgtoNetEnable", json_false());
			json_object_set_new(obj_event_handler, "MultimediaMsgEnable", json_false());
			json_object_set_new(obj_event_handler, "PtzEnable", json_true());

				arr_ptz_link = json_array();

				for (i=0;i<32;i++)
				{
					arr_ptz_link_tuple = json_array();
					json_array_append_new(arr_ptz_link_tuple, json_string("None"));
					json_array_append_new(arr_ptz_link_tuple, json_integer(0));

					json_array_append(arr_ptz_link, arr_ptz_link_tuple);
					json_decref(arr_ptz_link_tuple);
				}
			json_object_set(obj_event_handler, "PtzLink", arr_ptz_link);
			json_decref(arr_ptz_link);

			json_object_set_new(obj_event_handler, "RecordEnable", json_false());
			json_object_set_new(obj_event_handler, "RecordLatch", json_integer(10));
			json_object_set_new(obj_event_handler, "RecordMask", json_string("0x00000000"));
			json_object_set_new(obj_event_handler, "ShortMsgEnable", json_false());
			json_object_set_new(obj_event_handler, "ShowInfo", json_false());
			json_object_set_new(obj_event_handler, "ShowInfoMask", json_string("0x00000000"));
			json_object_set_new(obj_event_handler, "SnapEnable", json_false());
			json_object_set_new(obj_event_handler, "SnapShotMask", json_string("0x00000000"));

				arr_time_section = json_array();
				for (i=0;i<7;i++)
				{
					arr_time_section_tuple = json_array();
					json_array_append_new(arr_time_section_tuple, json_string("1 00:00:00-24:00:00"));
					json_array_append_new(arr_time_section_tuple, json_string("0 00:00:00-24:00:00"));
					json_array_append_new(arr_time_section_tuple, json_string("0 00:00:00-24:00:00"));
					json_array_append_new(arr_time_section_tuple, json_string("0 00:00:00-24:00:00"));
					json_array_append_new(arr_time_section_tuple, json_string("0 00:00:00-24:00:00"));
					json_array_append_new(arr_time_section_tuple, json_string("0 00:00:00-24:00:00"));

					json_array_append(arr_time_section, arr_time_section_tuple);
					json_decref(arr_time_section_tuple);
				}
			json_object_set(obj_event_handler, "TimeSection", arr_time_section);
			json_decref(arr_time_section);

			json_object_set_new(obj_event_handler, "TipEnable", json_false());
			json_object_set_new(obj_event_handler, "TourEnable", json_false());
			json_object_set_new(obj_event_handler, "TourMask", json_string("0x00000000"));
			json_object_set_new(obj_event_handler, "VoiceEnable", json_false());

		json_object_set(obj_motion, "EventHandler", obj_event_handler);
		json_decref(obj_event_handler);

		json_object_set_new(obj_motion, "Level", json_integer(sensitivity));

			arr_region = json_array();
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf)));   //01
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+1))); //02
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+2))); //03
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+3))); //04
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+4))); //05
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+5))); //06
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+6))); //07
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+7))); //08
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+8))); //09
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+9))); //10
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+10)));//11
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+11)));//12
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+12)));//13
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+13)));//14
			json_array_append_new(arr_region, json_string(*(xmbufs.dstbuf+14)));//15
			json_array_append_new(arr_region, json_string("0x00000000"));//unused below this
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));
			json_array_append_new(arr_region, json_string("0x00000000"));

		json_object_set(obj_motion, "Region", arr_region);
		json_decref(arr_region);

	json_array_append(arr_detect_dot_motion_detect, obj_motion);
	json_decref(obj_motion);

	json_object_set(obj_root, "Detect.MotionDetect", arr_detect_dot_motion_detect);
	json_decref(arr_detect_dot_motion_detect);

	json_post_api = json_dumps(obj_root, 0);
	if (json_post_api != NULL)
	{
		strcpy(xmbufs.post_api, json_post_api);
		free(json_post_api);
		json_post_api = NULL;
	}
	json_decref(obj_root);
	/* Motion info set end */


	/* HTTP set */
	memset(xmbufs.buf, 0x00, XM_DATA_BUF_SIZE);
	snprintf(xmbufs.http_api, XM_DATA_BUF_SIZE, xm_set_raw, strlen(xmbufs.post_api), xmbufs.post_api);
	/* HTTP set end */



	/* Network action */
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(80);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			close(sock);
			sock = (-1);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = send(sock, xmbufs.http_api, strlen(xmbufs.http_api), 0);

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) SEND\n%s\n", __FUNCTION__, cam_id, xmbufs.http_api);
#endif

	if (len < 0)
	{
		printf("send failed\n");
		perror("send");
		close(sock);
		_xmbuf_free(&xmbufs);
		return IPCAM_SETUP_RTN_FAILED;
	}

	int pos = 0;
	do {
		len = recv(sock, xmbufs.buf+pos, XM_DATA_BUF_SIZE-pos, 0);
		if (len < 0)
		{
			printf("recv failed\n");
			perror("recv");
			close(sock);
			_xmbuf_free(&xmbufs);
			return IPCAM_SETUP_RTN_FAILED;
		}
		else if (len == 0)
		{
			printf("server closed\n");
			perror("recv");
			close(sock);
			break;
		}

		pos += len;
	} while(1);
	/* Network action end */

#if PRINT_HTTP_API_SEND
	printf("%s CH(%d) RECV\n%s\n", __FUNCTION__, cam_id, xmbufs.buf);
#endif
	_xmbuf_free(&xmbufs);

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_MOTION, -1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int xiongmai_event_notification_handler(char* msg)
{
	json_t* event = NULL;
	json_error_t err;

	const char *Address;
	int Channel;
	const char *Descrip;
	const char *Event;
	const char *SerialID;
	const char *StartTime;
	const char *Status;
	const char *Type;

	int i=0;
	char *iter = NULL;
	unsigned char target_mac[8] = { 0, };

	mtable *runtime = get_runtime();


	event = json_loads(msg, 0, &err);
	if (!event)
	{
		printf("ERROR [%s] load failed\n", __FUNCTION__);
		return IPCAM_SETUP_RTN_FAILED;
	}

	json_unpack(event, "{s:s,s:i,s:s,s:s,s:s,s:s,s:s,s:s}",
			"Address", &Address,
			"Channel", &Channel,
			"Descrip", &Descrip,
			"Event", &Event,
			"SerialID", &SerialID,
			"StartTime", &StartTime,
			"Status", &Status,
			"Type", &Type);

	if (strcmp(Status, "Start") != 0 && strcmp(Status, "Stop") != 0)
	{
		json_decref(event);
		return IPCAM_SETUP_RTN_DONE;
	}
	if (strcmp(Event, "MotionDetect") != 0)
	{
		json_decref(event);
		return IPCAM_SETUP_RTN_DONE;
	}
	if (!SerialID || strlen(SerialID) != 12)
	{
		json_decref(event);
		return IPCAM_SETUP_RTN_DONE;
	}

	
	for (iter = &SerialID[0],i=0; iter<=&SerialID[11]; iter++, i++)
	{
		unsigned char val = 0;
		if (*iter >= '0' && *iter <= '9')
		{
			val = *iter - '0';
		}
		else if (*iter >= 'a' && *iter <= 'f')
		{
			val = *iter - 'a' + 10;
		}
		else if (*iter >= 'A' && *iter <= 'F')
		{
			val = *iter - 'A' + 10;
		}
		else
		{
			printf("ERROR [%s] read mac address\n", __FUNCTION__);
			json_decref(event);
			return IPCAM_SETUP_RTN_FAILED;
		}
		target_mac[i/2] += (i%2)?val:val*16;
	}

#if PRINT_HTTP_API_SEND
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
			target_mac[0],
			target_mac[1],
			target_mac[2],
			target_mac[3],
			target_mac[4],
			target_mac[5]);
#endif

	for (i=0; i<AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].state & MGMT_STATE_CONFIGURED)
		{
			if (memcmp(runtime[i].sys.macaddr, target_mac, 6) == 0)
			{
				if (strcmp(Status, "Start") == 0)
				{
					runtime[i].motion_flag = 90;
				}
				else if (strcmp(Status, "Stop") == 0)
				{
					runtime[i].motion_flag = 0;
				}
			}
		}
	}

	json_decref(event);
	return IPCAM_SETUP_RTN_DONE;
}

static void _xmbuf_alloc(NF_IPCAM_XM_BUFS* xmbufs)
{
	int i=0;

	xmbufs->pw_enc = (char*) malloc(XM_DATA_BUF_SIZE);
	memset(xmbufs->pw_enc, 0x00, XM_DATA_BUF_SIZE);

	xmbufs->post_api = (char*) malloc(XM_DATA_BUF_SIZE);
	memset(xmbufs->post_api, 0x00, XM_DATA_BUF_SIZE);

	xmbufs->http_api = (char*) malloc(XM_DATA_BUF_SIZE);
	memset(xmbufs->http_api, 0x00, XM_DATA_BUF_SIZE);

	xmbufs->buf = (char*) malloc(XM_DATA_BUF_SIZE);
	memset(xmbufs->buf, 0x00, XM_DATA_BUF_SIZE);
	xmbufs->dstbuf = malloc(sizeof(char*)*15);
	for(i=0;i<15;i++)
	{
		xmbufs->dstbuf[i] = malloc(16);
	}
}

static void _xmbuf_free(NF_IPCAM_XM_BUFS* xmbufs)
{
	int i=0;
	free(xmbufs->pw_enc);
	xmbufs->pw_enc = NULL;
	free(xmbufs->post_api);
	xmbufs->post_api = NULL;
	free(xmbufs->http_api);
	xmbufs->http_api = NULL;
	free(xmbufs->buf);
	xmbufs->buf = NULL;
	for(i=0;i<15;i++)
	{
		free(xmbufs->dstbuf[i]);
	}
	free(xmbufs->dstbuf);
}

static void _xm_convert_marea_sysdb_to_xm(const char* sysdb_area, char **xm_area)
{
	int i,j;
	unsigned char dsrc[15][8] = { { 0, }, };

	for (i=0; i<15; i++)
	{
		for (j=0; j<22; j++)
		{
			if (*(sysdb_area+i*22+j) == '1')
			{
				dsrc[i][7-j/4] |= (1<<(j%4));
			}
		}
	}

	for (i=0; i<15; i++)
	{
		snprintf(*(xm_area+i), 16, "0x%X%X%X%X%X%X%X%X",
				dsrc[i][0], dsrc[i][1], dsrc[i][2], dsrc[i][3],
				dsrc[i][4], dsrc[i][5], dsrc[i][6], dsrc[i][7]);
	}
}

static int _xm_is_different_config(int cam_id, cam_info* info)
{
	cam_info *last = &last_stream_info[cam_id];

	if (last->vcodec.bitrate[1] != info->vcodec.bitrate[1])
	{
		return 1;
	}
	if (last->vcodec.fps[1] != info->vcodec.fps[1])
	{
		return 1;
	}
	if (last->vcodec.bitrate[0] != info->vcodec.bitrate[1])
	{
		return 1;
	}
	if (last->vcodec.fps[0] != info->vcodec.fps[0])
	{
		return 1;
	}

	return 0;
}

#endif //__NF_IPCAM_DRIVER_XIONGMAI_C__
