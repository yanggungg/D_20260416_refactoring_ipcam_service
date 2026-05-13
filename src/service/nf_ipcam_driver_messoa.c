/*
 * ITX Security
 *  System software group
 *
 *  2012-03-27 jykim
 */

#ifndef __NF_IPCAM_DRIVER_MESSOA_C__
#define __NF_IPCAM_DRIVER_MESSOA_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>


#define UNIT_TEST
#undef UNIT_TEST

#define WAIT_REPLY_SECS		(2)
#define PRINT_HTTP_API_SEND	(0)



static const char str_api_raw[] =
	"GET /cgi-bin/admin/param.cgi?%s HTTP/1.1\r\n"
	"Host: %s\r\n"	// IP address
	"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:10.0.2) Gecko/20100101 Firefox/10.0.2\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko-kr,en-us;q=0.7,en;q=0.3\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"Connection: keep-alive\r\n"
	"X-Requested-With: XMLHttpRequest\r\n"
	"Referer: http://%s/%s\r\n"		// IP address / setup.html
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n";



static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);


extern int messoa_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
{
	mtable *runtime = get_runtime();

	if (runtime == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("[%s] %s\n", __FUNCTION__, recv_buf);
#endif
	switch(type)
	{
		case NF_IPCAM_TYPE_POLL_EVENT:
		{
			char *p = NULL;
			const char *find_status = "Alarm.Status=";
			char val_str[16];
			guint alarm_status = 0;

			memset(val_str, 0x00, 16);
			p = strstr(recv_buf, find_status);

			if (p != NULL)
			{
				p += strlen(find_status);
				val_str[0] = *p;
				val_str[1] = *(p+1);
				val_str[2] = *(p+2);
				val_str[3] = *(p+3);
				alarm_status = strtoul(val_str,NULL,0);
				if (alarm_status&0x2)
				{
					runtime[cam_id].motion_flag = 90;
				}
#if 0
				else
				{
					runtime[cam_id].motion_flag = 0;
				}
#endif
				if (alarm_status&0x1)
				{
					runtime[cam_id].alarm_flag = 1;
				}
#if 0
				else
				{
					runtime[cam_id].alarm_flag = 0;
				}
#endif
			}
			break;
		}
		default:
			break;
	}

	return IPCAM_SETUP_RTN_DONE;
}

extern int i3_set_vcodec(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
		"action=update&"
		"Image.Appearance.RateControl=0&"
		"Image.Appearance.CodecMode=H264(720P)@H264(D1)@OFF(...)&"
		"Image.Appearance.JpegQuality=1&"
		"Image.Appearance.Mpeg41BitRate=%d&"
		"Image.Appearance.JpegFrameRate=1&"
		"Image.Appearance.Mpeg41FrameRate=15&"
		"Image.Appearance.CodecMode=H264(720P)@H264(D1)@OFF(...)&"
		"Image.Appearance.JpegQuality=1&"
		"Image.Appearance.Mpeg42BitRate=%d&"
		"Image.Appearance.Mpeg42FrameRate=15&"
		"Image.Appearance.GOP=15&"
		"ImageSource.Sensor.Flickerless=%d&"
		"ImageSource.Sensor.Mirror=%d";

	char http_api[2048];
	char auth_encbuf[256];
	char sock_buf[4096];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int af_mode;
	int mirror_mode;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	if (info_set->vcodec.af == 50)
	{
		af_mode = 0;
	}
	else
	{
		af_mode = 1;
	}

	if (info_set->vcodec.mirror == NF_IPCAM_MIRROR_FLIP)
	{
		mirror_mode = 3;
	}
	else
	{
		mirror_mode = 0;
	}
	snprintf(http_api, 2048, set_vcodec_raw,
			info_set->vcodec.bitrate[0],
			info_set->vcodec.bitrate[1],
			af_mode,
			mirror_mode
			);
	snprintf(sock_buf, 4096, str_api_raw,
			http_api,
			ip_str,
			ip_str, "setup.html",
			auth_encbuf);

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

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int i3_poll_alarm_status(int cam_id)
{
	const char set_api_raw[] =
		"action=list&group=Alarm.Status";

	char http_api[1024];
	char auth_encbuf[256];
	char sock_buf[2048];
	char ip_str[16];
	char username[64];
	char password[64];
	char *mirr;
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

	snprintf(http_api, 1024, set_api_raw);
	snprintf(sock_buf, 2048, str_api_raw,
			http_api,
			ip_str,
			ip_str, "setup.html",
			auth_encbuf);

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

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_POLL_EVENT, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int i3_set_image(image_info* info, int cam_id)
{
	const char set_api_raw[] =
		"action=update&"
		"ImageSource.Sensor.ExposureMode=%d&"
		"ImageSource.Sensor.Brightness=%d&"
		"ImageSource.Sensor.Contrast=%d&"
		"ImageSource.Sensor.Staturation=%d&"
		"ImageSource.Sensor.Sharpness=%d&"
		"ImageSource.Sensor.ME.ExpTime=%d.00&"
		"ImageSource.Sensor.ME.Gain=%d&"
		"ImageSource.Sensor.AE.AESpotWindow=00;00;00;00;00&"
		"ImageSource.Sensor.AE.Method=0&"
		"ImageSource.Sensor.AE.Ev=10&"
		"ImageSource.Sensor.AE.MaxExp=0&"
		"ImageSource.Sensor.AE.MinExp=5&"
		"ImageSource.Sensor.AE.Sensitivity=9&"
		"ImageSource.Sensor.AE.MaxGain=%d&"
		"ImageSource.Sensor.AE.AEWdrMode=0&"
		"ImageSource.Sensor.BlcMode=%d&"
		"ImageSource.Sensor.BlcWindow=F8;F8;F8;F8;F8"
		;

	char http_api[1024];
	char auth_encbuf[256];
	char sock_buf[2048];
	char ip_str[16];
	char username[64];
	char password[64];
	char *mirr;
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

	snprintf(http_api, 1024, set_api_raw,
			(info->ae - 8),
			info->brightness,
			info->contrast,
			info->color,
			info->sharpness,
			info->shutter,
			info->agc,
			(info->max_agc - 2),
			info->blc
			);
	snprintf(sock_buf, 2048, str_api_raw,
			http_api,
			ip_str,
			ip_str, "setup.html",
			auth_encbuf);

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

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_IMAGE, sock);

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

#endif //__NF_IPCAM_DRIVER_MESSOA_C__
