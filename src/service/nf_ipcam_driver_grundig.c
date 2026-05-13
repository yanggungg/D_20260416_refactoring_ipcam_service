/*
 * ITX Security
 *  System software group
 *
 *  2012-03-17 jykim
 */

#ifndef __NF_IPCAM_DRIVER_GRUNDIG_C__
#define __NF_IPCAM_DRIVER_GRUNDIG_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <glib.h>
// #include <gst/gst.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_ptz.h>
#include "nf_ipcam_driver_grundig.h"


#define UNIT_TEST
#undef UNIT_TEST

#define WAIT_REPLY_SECS		(2)
#define PRINT_HTTP_API_SEND	(0)
#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)


static const char str_api_raw[] =
"GET /cgi-bin/videoctrl.cgi?%s HTTP/1.1\r\n" // set_api_str
"Host: %s\r\n"	// 192.168.100.62
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n"	// auth_str
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Referer: http://%s/lang1/%s\r\n" // 192.168.100.62 server_video.html
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

static const char str_ptz_raw[] =
"GET /cgi-bin/com/ptz.cgi?%s HTTP/1.1\r\n"	// set_api_str
"Host: %s\r\n"	// 192.168.100.62
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n\r\n";	// auth_str

static const char str_ptzconfig_raw[] =
"GET /cgi-bin/com/ptzconfig.cgi?%s HTTP/1.1\r\n"	// set_api_str
"Host: %s\r\n"	// 192.168.100.62
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n\r\n";	// auth_str

static const char param_api_raw[] =
"GET /cgi-bin/admin/param.cgi?%s HTTP/1.1\r\n" // motion_detect.cgi set_api_str
"Host: %s\r\n"  // 192.168.100.62
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n"   // auth_str
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

static const char str_common_raw[] =
"GET /cgi-bin/%s?%s HTTP/1.1\r\n" // set_api_str
"Host: %s\r\n"	// 192.168.100.62
"Connection: keep-alive\r\n"
"Authorization: Basic %s\r\n"	// auth_str
"User-Agent: IPX-NVR\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
//"Referer: http://%s/lang1/%s\r\n" // 192.168.100.62 server_video.html
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

extern void nf_ipcam_waiting_settime(int port, int msec);
static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);

static int _grundig_set_fps(int cam_id, int stream_no, int fps);
static int _grundig_set_bitrate(int cam_id, int stream_no, int bitrate);




extern int grundig_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
{
#if PRINT_HTTP_API_SEND
	printf("[%s] GRUNDIG received(%d, %d)\n%s", __FUNCTION__, cam_id, type, recv_buf);
#endif

	mtable *runtime = get_runtime();
	switch(type)
	{
		case NF_IPCAM_TYPE_CUSTOM0:
			if (strstr(recv_buf, "fixed cam") == NULL)
			{
//				mtable *runtime = get_runtime();

				runtime[cam_id].image.supported |=
						NF_IPCAM_IMAGE_ZOOM|NF_IPCAM_IMAGE_FOCUS|
						NF_IPCAM_IMAGE_ONEPUSH|NF_IPCAM_IMAGE_CALIBRATION;
				runtime[cam_id].func |= NF_IPCAM_FUNC_PTZ;
				runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_AF_MODE] = &grundig_ipptz_set_origin;
				runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_ZOOM] = &grundig_ipptz_set_zoom;
				runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_FOCUS] = &grundig_ipptz_set_focus;
				runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_STOP] = &grundig_ipptz_set_ptz_stop;
			}
			break;
		case NF_IPCAM_TYPE_CUSTOM1:
			/*
			if (strstr(recv_buf, "Properties.Audio.Audio=yes") != NULL)
			{
//				mtable *runtime = get_runtime();

				runtime[cam_id].func |= NF_IPCAM_FUNC_AUDIO_RX;
				runtime[cam_id].func |= NF_IPCAM_FUNC_AUDIO_TX;
				runtime[cam_id].audio.acodec.support = NF_IPCAM_ACODEC_G711_ULAW;
				runtime[cam_id].audio.acodec.value = NF_IPCAM_ACODEC_G711_ULAW;
				runtime[cam_id].audio.audio_rx = 1;
				runtime[cam_id].audio.audio_tx = 1;
				runtime[cam_id].audio.mic_volume.min = 0;
				runtime[cam_id].audio.mic_volume.max = 6;
				runtime[cam_id].audio.mic_volume.value = 3;
				runtime[cam_id].audio.speaker_volume.min = 0;
				runtime[cam_id].audio.speaker_volume.max = 6;
				runtime[cam_id].audio.speaker_volume.value = 3;
				runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_ACODEC] = &grundig_set_audio;
			}
			*/
			if (strstr(recv_buf, "BitRate") != NULL)
			{
				runtime[cam_id].audio.acodec.support = NF_IPCAM_ACODEC_G711_ULAW;
				runtime[cam_id].audio.acodec.value = NF_IPCAM_ACODEC_G711_ULAW;
			}
			if (strstr(recv_buf, "InputGain") != NULL)
			{
				runtime[cam_id].func |= NF_IPCAM_FUNC_AUDIO_TX;
				runtime[cam_id].audio.audio_tx = 1;
				runtime[cam_id].audio.mic_volume.min = 0;
				runtime[cam_id].audio.mic_volume.max = 6;
				runtime[cam_id].audio.mic_volume.value = 3;
			}
			if (strstr(recv_buf, "OutputGain") != NULL)
			{
				runtime[cam_id].func |= NF_IPCAM_FUNC_AUDIO_RX;
				runtime[cam_id].audio.audio_rx = 1;
				runtime[cam_id].audio.speaker_volume.min = 0;
				runtime[cam_id].audio.speaker_volume.max = 6;
				runtime[cam_id].audio.speaker_volume.value = 3;
			}
			runtime[cam_id].funcs[NF_IPCAM_TYPE_SET_ACODEC] = &grundig_set_audio;
			runtime[cam_id].sys.model_code = NF_IPCAM_MODEL_ONVIF_GRUNDIG;
			break;

		case NF_IPCAM_TYPE_CUSTOM2:
			if(strstr(recv_buf, "Resolution=1") != NULL)
			{
				runtime[cam_id].video.supported |= VIDEO_SETUP_RESOLUTION;
				runtime[cam_id].video.onthefly |= VIDEO_SETUP_RESOLUTION;
			}
			if(strstr(recv_buf, "root.OnFlyCommand.Image.I0.Appearance.H264Bitrate =1") != NULL)
			{
				runtime[cam_id].video.supported |= VIDEO_SETUP_BITRATE;
				runtime[cam_id].video.onthefly |= VIDEO_SETUP_BITRATE;
			}
			if(strstr(recv_buf, "root.OnFlyCommand.Framerate.H264=1") != NULL)
			{
				runtime[cam_id].video.supported |= VIDEO_SETUP_FPS;
				runtime[cam_id].video.onthefly |= VIDEO_SETUP_FPS;
			}
			break;
		default:
			break;
	}
	return 1;
}

extern int grundig_common_set_audio_property(int cam_id)
{
	/* old api : recv from "Properties.Audio.Audio=yes" */
	//const char set_api_raw[] = "action=list&group=Properties.Audio.Audio";
	
	const char set_api_raw[] = "action=list&group=AudioSource";

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if 0
	{
		struct timeval tv;
		tv.tv_sec = 3; // 1 Secs Timeout
		tv.tv_usec = 0;
		int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("%s | limiting waiting time failed %d", __FUNCTION__, ret);
			free(http_api);
			free(sock_buf);
			close(sock);
			return (-1);
		}
	}
#endif
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, sock);
#else
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_set_audio(cam_info *info, int cam_id)
{
	const char set_api_raw[] =
		"action=update&"
		"Audio.DuplexMode=full&"
		"AudioSource.A0.BitRate=ulaw&"
		"AudioSource.A0.InputGain=3&"
		"AudioSource.A0.OutputGain=3"
		;

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	g_return_val_if_fail(info != NULL, IPCAM_SETUP_RTN_FAILED);

	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ACODEC, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_set_alarm(cam_info *info, int cam_id)
{
	const char *yn[] = { "yes", "no" };
	const char set_api_raw[] =
		"action=update&"
		"Event.E0.Enabled=yes&"
		"Event.E0.Actions.A8.Enabled=yes&"
		"Event.E0.Actions.A8.Server=H0&"
		"Event.E0.Actions.A8.CustomParams=alarm_%02d&"
		"EventServers.HTTP.H0.Address=%d.%d.%d.%d:%d/event_noti.cgi"
		;

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	g_return_val_if_fail(info != NULL, IPCAM_SETUP_RTN_FAILED);

	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw,
			cam_id,
			host_ip&0xff,
			(host_ip&0xff00)>>8,
			(host_ip&0xff0000)>>16,
			(host_ip&0xff000000)>>24,
			NF_IPCAM_EVENT_NOTI_PORT
			);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ALARM, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_set_motion_area(NFIPCamSetupMotionArea *motion_info, int cam_id)
{
	const char *yn[] = { "yes", "no" };
	const char set_api_raw[] =
		"action=update&"
		"Motion.M0.Enabled=%s&"
		//"Motion.M0.Enabled=yes&"
		"Motion.M0.Left=%d&Motion.M0.Right=%d&Motion.M0.Top=%d&Motion.M0.Bottom=%d&"
		//"Motion.M0.Left=0&Motion.M0.Right=30&Motion.M0.Top=0&Motion.M0.Bottom=25&"
		"Motion.M0.Sensitivity=%d&"
		"Motion.M1.Enabled=%s&"
		"Motion.M1.Left=%d&Motion.M1.Right=%d&Motion.M1.Top=%d&Motion.M1.Bottom=%d&"
		"Motion.M2.Enabled=%s&"
		"Motion.M2.Left=%d&Motion.M2.Right=%d&Motion.M2.Top=%d&Motion.M2.Bottom=%d&"
		"Motion.M3.Enabled=%s&"
		"Motion.M3.Left=%d&Motion.M3.Right=%d&Motion.M3.Top=%d&Motion.M3.Bottom=%d&"
		"Motion.M4.Enabled=%s&"
		"Motion.M4.Left=%d&Motion.M4.Right=%d&Motion.M4.Top=%d&Motion.M4.Bottom=%d&"
		"Motion.M5.Enabled=%s&"
		"Motion.M5.Left=%d&Motion.M5.Right=%d&Motion.M5.Top=%d&Motion.M5.Bottom=%d&"
		"Motion.M6.Enabled=%s&"
		"Motion.M6.Left=%d&Motion.M6.Right=%d&Motion.M6.Top=%d&Motion.M6.Bottom=%d&"
		"Motion.M7.Enabled=%s&"
		"Motion.M7.Left=%d&Motion.M7.Right=%d&Motion.M7.Top=%d&Motion.M7.Bottom=%d&"
		"Motion.M8.Enabled=%s&"
		"Motion.M8.Left=%d&Motion.M8.Right=%d&Motion.M8.Top=%d&Motion.M8.Bottom=%d&"
		"Motion.M9.Enabled=%s&"
		"Motion.M9.Left=%d&Motion.M9.Right=%d&Motion.M9.Top=%d&Motion.M9.Bottom=%d&"
		"Motion.Sensitivity=%d&"
		"Event.E1.Enabled=yes&"
		"Event.E1.Actions.A8.Enabled=yes&"
		"Event.E1.Actions.A8.Server=H0&"
		"Event.E1.Actions.A8.CustomParams=motion_%02d&"
		"EventServers.HTTP.H0.Address=%d.%d.%d.%d:%d/event_noti.cgi"
		;

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	g_return_val_if_fail(motion_info != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(motion_info->method == MAM_RECTANGLE, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(motion_info->area_num <= 10, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(motion_info->area_num >= 0, IPCAM_SETUP_RTN_FAILED);

	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw,
#if 1
			yn[((motion_info->area_num > 0) ? 0:1)],
			motion_info->marea[0].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[0].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[0].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[0].FIGURE.RECTANGLE.right_bottom.y,
#endif
			motion_info->marea[0].sensitivity,
			yn[((motion_info->area_num > 1) ? 0:1)],
			motion_info->marea[1].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[1].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[1].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[1].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 2) ? 0:1)],
			motion_info->marea[2].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[2].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[2].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[2].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 3) ? 0:1)],
			motion_info->marea[3].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[3].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[3].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[3].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 4) ? 0:1)],
			motion_info->marea[4].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[4].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[4].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[4].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 5) ? 0:1)],
			motion_info->marea[5].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[5].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[5].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[5].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 6) ? 0:1)],
			motion_info->marea[6].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[6].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[6].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[6].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 7) ? 0:1)],
			motion_info->marea[7].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[7].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[7].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[7].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 8) ? 0:1)],
			motion_info->marea[8].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[8].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[8].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[8].FIGURE.RECTANGLE.right_bottom.y,
			yn[((motion_info->area_num > 9) ? 0:1)],
			motion_info->marea[9].FIGURE.RECTANGLE.left_top.x,
			motion_info->marea[9].FIGURE.RECTANGLE.right_bottom.x,
			motion_info->marea[9].FIGURE.RECTANGLE.left_top.y,
			motion_info->marea[9].FIGURE.RECTANGLE.right_bottom.y,
			motion_info->marea[0].sensitivity,
			cam_id,
			host_ip&0xff,
			(host_ip&0xff00)>>8,
			(host_ip&0xff0000)>>16,
			(host_ip&0xff000000)>>24,
			NF_IPCAM_EVENT_NOTI_PORT
			);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 5000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_MOTION, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

#if 0
extern int k0523t_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"codecType=3&mjpegresolution=1080p&bnc_support2=Yes&h264_0_resolution=1080p&"
		"mjpeg_0_resolution=d1&bnc_support0=Yes&h264_6_resolution=1080p15&"
		"h2642_6_resolution=sxga15&mjpeg_6_resolution=720p15&h264_7_resolution=1080p15&"
		"h2642_7_resolution=sxga15&h2643_7_resolution=720p15&mjpeg_7_resolution=d115&"
		"h264resolution=1080p&bnc_support5=Yes&h264_3_resolution=1080p&"
		"h2642_3_resolution=vga&bnc_support3=Yes&h264_8_resolution=1080p15&"
		"h2642_8_resolution=sxga15&h2643_8_resolution=720p15&h264_9_resolution=1080p15&"
		"h2642_9_resolution=sxga15&h2643_9_resolution=720p15&h2644_9_resolution=d115";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int k1812w_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"codecType=3&mjpegresolution=1080p&bnc_support2=N\%2FA&h264_0_resolution=1080p&"
		"mjpeg_0_resolution=d1&bnc_support0=N\%2FA&h264_6_resolution=1080p15&"
		"h2642_6_resolution=sxga15&mjpeg_6_resolution=720p15&h264_7_resolution=1080p15&"
		"h2642_7_resolution=sxga15&h2643_7_resolution=720p15&mjpeg_7_resolution=d115&"
		"h264resolution=1080p&bnc_support5=N\%2FA&h264_3_resolution=1080p&"
		"h2642_3_resolution=vga&bnc_support3=N\%2FA&h264_8_resolution=1080p15&"
		"h2642_8_resolution=sxga15&h2643_8_resolution=720p15&h264_9_resolution=1080p15&"
		"h2642_9_resolution=sxga15&h2643_9_resolution=720p15&h2644_9_resolution=d115";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}
#endif

extern int grundig_hdwdr_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"videoCodec=3&mjpeg1resolution=720p&h264resolution=720p&"
		"mjpeg2resolution=720p&mpegresolution=720p&mjpeg3resolution=720p&"
		"nptype=1&h2642resolution=720p15&h2643resolution=vga&"
		"h2644resolution=720p";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_vseries_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"codecType=3&resolution=3";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"codecType=3&resolution=23";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_fhdms_set_resolution(int cam_id)
{
	const char set_api_raw[] =
		"codecType=3&mjpegresolution=1080p&bnc_support2=Yes&h264_0_resolution=1080p&"
		"mjpeg_0_resolution=d1&bnc_support0=Yes&h264_6_resolution=1080p15&"
		"h2642_6_resolution=sxga15&mjpeg_6_resolution=720p15&h264_7_resolution=1080p15&"
		"h2642_7_resolution=sxga15&h2643_7_resolution=720p15&mjpeg_7_resolution=d115&"
		"h264resolution=1080p&bnc_support5=Yes&h264_3_resolution=1080p&"
		"h2642_3_resolution=vga&bnc_support3=Yes&h264_8_resolution=1080p15&"
		"h2642_8_resolution=sxga15&h2643_8_resolution=720p15&h264_9_resolution=1080p15&"
		"h2642_9_resolution=sxga15&h2643_9_resolution=720p15&h2644_9_resolution=d115";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 10000);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_set_gop(int cam_id)
{
	const char set_api_raw[] =
		"h264govset=12&h2642govset=12";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 5000);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

#if 0
extern int h0503b_set_gop(int cam_id)
{
	const char set_api_raw[] =
		"mpeg4govset=25&h264govset=12&h2642govset=12";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 5000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_gop(int cam_id)
{
	const char set_api_raw[] =
		"h264govset=12&h2642govset=12";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, set_api_raw, ip_str, auth_encbuf,
			ip_str, "server_video.html");

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

#if 1
	nf_ipcam_waiting_settime(cam_id, 5000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, sock);

#else
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	return IPCAM_SETUP_RTN_DONE;
}
#endif

extern int grundig_ipptz_set_preset(int num, int cam_id)
{
	const char http_api_raw[] = "setserverpresetno=%d";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, num);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptzconfig_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_PRESET_SET, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_go_preset(int num, int cam_id)
{
	const char http_api_raw[] = "gotoserverpresetno=%d";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, num);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_PRESET_GO, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_pt(NF_PTZ_CMD_E ptz_cmd, int cam_id)
{
	const char http_api_raw[] = "continuouspantiltmove=%s";

	const char* directions[] = {
		"-10,0", "10,0", "0,10", "0,-10",
		"","", "-10,10", "-10,-10",
		"10,10", "10,-10", "","",
		"","", "0,0"
	};

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	switch(ptz_cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
		case NF_PTZ_CMD_PAN_RIGHT:
		case NF_PTZ_CMD_TILT_UP:
		case NF_PTZ_CMD_TILT_DOWN:
		case NF_PTZ_CMD_PT_LEFTUP:
		case NF_PTZ_CMD_PT_LEFTDOWN:
		case NF_PTZ_CMD_PT_RIGHTUP:
		case NF_PTZ_CMD_PT_RIGHTDOWN:
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-PT command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		struct timespec now_time;
		clock_gettime(CLOCK_REALTIME, &now_time);
		printf("[%lu.%06lu] [\033[1;49;33m%s\033[0m] CH%d\n", now_time.tv_sec, now_time.tv_nsec/1000, __FUNCTION__, cam_id);
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[ptz_cmd]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_zoom(NF_PTZ_CMD_E ptz_cmd, int cam_id)
{
	const char http_api_raw[] = "continuouszoommove=%s";

	const char* directions[] = {
		"","","","","-2","2",
		"","","","",
		"","","","",
		"0"
	};

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	switch(ptz_cmd)
	{
		case NF_PTZ_CMD_ZOOM_WIDE:
		case NF_PTZ_CMD_ZOOM_TELE:
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-PT command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		struct timespec now_time;
		clock_gettime(CLOCK_REALTIME, &now_time);
		printf("[%lu.%06lu] [\033[1;49;33m%s\033[0m] CH%d\n", now_time.tv_sec, now_time.tv_nsec/1000, __FUNCTION__, cam_id);
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[ptz_cmd]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_focus(NF_PTZ_CMD_E ptz_cmd, int cam_id)
{
	const char http_api_raw[] = "continuousfocusmove=%s";

	const char* directions[] = {
		"","","","","","",
		"","","","",
		"","","1","-1",
		"0"
	};

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	switch(ptz_cmd)
	{
		case NF_PTZ_CMD_FOCUS_NEAR:
		case NF_PTZ_CMD_FOCUS_FAR:
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-PT command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		struct timespec now_time;
		clock_gettime(CLOCK_REALTIME, &now_time);
		printf("[%lu.%06lu] [\033[1;49;33m%s\033[0m] CH%d\n", now_time.tv_sec, now_time.tv_nsec/1000, __FUNCTION__, cam_id);
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[ptz_cmd]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_origin(int cam_id)
{
	const char http_api_raw[] = "move=home";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_autofocus_mode(int enable, int cam_id)
{
	const char http_api_raw[] = "autofocus=%s";
	const char *onoff[] = { "off", "on" };

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	if (enable != 0 && enable == 1)
	{
		printf("[%s] param fail\n", __FUNCTION__);
		return (0);
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, onoff[enable]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_ptz_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_ipptz_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	switch(e)
	{
		case PTZ_SETUP_PAN:
		case PTZ_SETUP_TILT:
			return grundig_ipptz_set_pt(NF_PTZ_CMD_STOP, cam_id);
		case PTZ_SETUP_ZOOM:
			return grundig_ipptz_set_zoom(NF_PTZ_CMD_STOP, cam_id);
		case PTZ_SETUP_FOCUS:
			return grundig_ipptz_set_focus(NF_PTZ_CMD_STOP, cam_id);
		case PTZ_SETUP_IRIS:
		default:
			return IPCAM_SETUP_RTN_FAILED;
	}
	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_common_set_ptz_property(int cam_id)
{
	const char set_api_raw[] = "action=list&group=Properties.PTZ.PTZ";

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if 0
	{
		struct timeval tv;
		tv.tv_sec = 3; // 1 Secs Timeout
		tv.tv_usec = 0;
		int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("%s | limiting waiting time failed %d", __FUNCTION__, ret);
			free(http_api);
			free(sock_buf);
			close(sock);
			return (-1);
		}
	}
#endif
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);
#else
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_common_get_onthefly(int cam_id)
{
	const char set_api_raw[] = "action=list&group=OnFlyCommand";

	char auth_encbuf[256];
	char *http_api;
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;
	unsigned int host_ip;

	int sock;
	struct sockaddr_in sin;


	host_ip = get_host_info();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	http_api = (char*) malloc(SOCK_BUF_LENGTH);
	sock_buf = (char*) malloc(4096);
	snprintf(http_api, SOCK_BUF_LENGTH, set_api_raw);
	snprintf(sock_buf, 4096, param_api_raw, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		free(sock_buf);
		return (0);
	}
#if 0
	{
		struct timeval tv;
		tv.tv_sec = 3; // 1 Secs Timeout
		tv.tv_usec = 0;
		int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("%s | limiting waiting time failed %d", __FUNCTION__, ret);
			free(http_api);
			free(sock_buf);
			close(sock);
			return (-1);
		}
	}
#endif
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if 1
	//nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM2, sock);
#else
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(http_api);
		free(sock_buf);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	close(sock);
	sock = (-1);
#endif
	free(http_api);
	free(sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}

extern int grundig_common_adjust_vcodec(cam_info* info_set, int cam_id)
{
	int i, fps, rtn;
	for(i = 0; i < 2; i++)//max_no_stream
	{
		switch(info_set->vcodec.fps[i])
		{
			case NF_IPCAM_FPS_300:
				//fps = 30;
				fps = 25;
				break;
			case NF_IPCAM_FPS_250:
				fps = 25;
				break;
			case NF_IPCAM_FPS_150:
				fps = 15;
				break;
			case NF_IPCAM_FPS_120:
				fps = 12;
				break;
			case NF_IPCAM_FPS_70:
				fps = 7;
				break;
			case NF_IPCAM_FPS_60:
				fps = 6;
				break;
			case NF_IPCAM_FPS_30:
				fps = 3;
				break;
			case NF_IPCAM_FPS_20:
				fps = 2;
				break;
			case NF_IPCAM_FPS_10:
				fps = 1;
				break;
		}
		rtn = _grundig_set_fps(cam_id, i, fps);
		printf("[%s] CH(%d) fps rtn : %d\n", __FUNCTION__, cam_id, rtn);
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			return rtn;
		}
		rtn = _grundig_set_bitrate(cam_id, i, info_set->vcodec.bitrate[i]);
		printf("[%s] CH(%d) bitrate rtn : %d\n", __FUNCTION__, cam_id, rtn);
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			return rtn;
		}
	}
	return IPCAM_SETUP_RTN_DONE;
}

static int _grundig_set_fps(int cam_id, int stream_no, int fps)
{
	const char http_api_cmd[] = "frameskip.cgi";
	const char http_api_raw[] = "h264%sframerate=%d&Submit=Save";
	const char *stream_str[] = { "", "2", "3"};

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	if (fps > 30 || fps < 1 || stream_no < 0 || stream_no > 2)
	{
		printf("[%s] param fail(%d, %d)\n", __FUNCTION__, stream_no, fps);
		return (0);
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, stream_str[stream_no], fps);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_common_raw, http_api_cmd, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

static int _grundig_set_bitrate(int cam_id, int stream_no, int bitrate)
{
	const char http_api_cmd[] = "h264bitrate.cgi";
	const char http_api_raw[] = "h264%sbitrate=%d&Submit=Save";
	const char *stream_str[] = { "", "2", "3"};

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;


	if (bitrate > 8192 || bitrate < 64 || stream_no < 0 || stream_no > 2)
	{
		printf("[%s] param fail(%d, %d)\n", __FUNCTION__, stream_no, bitrate);
		return (0);
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, stream_str[stream_no], bitrate);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_common_raw, http_api_cmd, http_api, ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
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
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src)
{
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char in[3], out[4];
	int i, len, src_id, src_len = 0;
	size_t dst_len = 0;

	src_len = strlen(p_src);
	memset(p_dst, 0x00, i_dst);

	for (src_id = 0; src_id < src_len;)
	{
		len = 0;
		for (i=0; i<3; i++)
		{
			in[i] = *(p_src+src_id++);
			if (src_id <= src_len)
				len++;
			else
				in[i] = 0;
		}
		if (len)
		{
			out[0] = b64[in[0] >> 2];
			out[1] = b64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
			out[2] = (len > 1 ? b64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
			out[3] = (len > 2 ? b64[in[2] & 0x3f] : '=');
			for (i=0; i<4; i++)
			{
				*(p_dst+i) = out[i];
				dst_len++;
			}
			p_dst += 4;
		}
	}

	return dst_len;
}



#endif //__NF_IPCAM_DRIVER_GRUNDIG_C__
