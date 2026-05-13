/*
 * ITX Security
 *  System software group
 *
 *  2012-03-17 jykim
 */

#ifndef __NF_IPCAM_DRIVER_VIVOTEK_C__
#define __NF_IPCAM_DRIVER_VIVOTEK_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>



#define PRINT_HTTP_API_SEND (0)

static const char str_api_raw[] =
	"POST /cgi-bin/admin/setparam.cgi HTTP/1.1\r\n"
	"Host: %s\r\n"	// 192.168.100.62
	"Connection: keep-alive\r\n"
	"Content-Length: %d\r\n"	// length
	"Cache-Control: max-age=0\r\n"
	"Origin: http://%s\r\n"	// IPAddress
	"Authorization: Basic %s\r\n"	// auth_str
	"User-Agent: Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\r\n"
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
	"Referer: http://%s/setup/%s\r\n" // 192.168.100.62 / video.html
	"Accept-Encoding: gzip,deflate,sdch\r\n"
	"Accept-Language: en-US,en;q=0.8\r\n"
	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n"
	"Cookie: activatemode=mechanical; g_mode=1\r\n"
	"\r\n"
	"%s";

static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);
static int _get_fps_int(unsigned int fps_mask);



extern int vivotek_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
{
#if PRINT_HTTP_API_SEND
	printf("[%s] received(%d, %d)\n%s", __FUNCTION__, cam_id, type, recv_buf);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

extern int vivotek_set_video(cam_info* info_set, int cam_id)
{
	const char set_api_raw[] =
		"videoin_c0_text=&videoin_c0_color=1&videoin_c0_cmosfreq=60&"
		"timeshift_c0_s0_allow=0&timeshift_c0_s1_allow=0&"
		"timeshift_c0_s2_allow=0&timeshift_c0_s3_allow=1&"
		"media_i0_videoclip_source=0&media_i1_videoclip_source=0&"
		"media_i2_videoclip_source=0&media_i3_videoclip_source=0&"
		"media_i4_videoclip_source=0&timeshift_Stream_num=3&videoin_c0_flip=0&"
		"videoin_c0_mirror=0&videoin_c0_imprinttimestamp=0&timeshift_enable=0&"
		"stream1_mpeg4_resolution=1280x720&videoin_c0_s0_mpeg4_maxframe=30&"
		"videoin_c0_s0_mpeg4_intraperiod=1000&videoin_c0_s0_mpeg4_ratecontrolmode=cbr&"
		"videoin_c0_s0_mpeg4_bitrate=2000000&videoin_c0_s0_mpeg4_qvalue=7&"
		"videoin_c0_s0_mpeg4_quant=3&videoin_c0_s0_codectype=h264&"
		"stream1_h264_resolution=1280x720&"
		"videoin_c0_s0_h264_maxframe=%d&"
		"videoin_c0_s0_h264_intraperiod=1000&videoin_c0_s0_h264_ratecontrolmode=cbr&"
		"videoin_c0_s0_h264_bitrate=%d000&videoin_c0_s0_h264_qvalue=30&"
		"videoin_c0_s0_h264_quant=99&stream1_mjpeg_resolution=1280x720&"
		"videoin_c0_s0_mjpeg_maxframe=30&videoin_c0_s0_mjpeg_qvalue=50&"
		"videoin_c0_s0_mjpeg_quant=3&stream2_mpeg4_resolution=640x360&"
		"videoin_c0_s1_mpeg4_maxframe=30&videoin_c0_s1_mpeg4_intraperiod=1000&"
		"videoin_c0_s1_mpeg4_ratecontrolmode=cbr&videoin_c0_s1_mpeg4_bitrate=512000&"
		"videoin_c0_s1_mpeg4_qvalue=7&videoin_c0_s1_mpeg4_quant=3&"
		"videoin_c0_s1_codectype=h264&stream2_h264_resolution=640x360&"
		"videoin_c0_s1_h264_maxframe=%d&videoin_c0_s1_h264_intraperiod=1000&"
		"videoin_c0_s1_h264_ratecontrolmode=cbr&"
		"videoin_c0_s1_h264_bitrate=%d000&"
		"videoin_c0_s1_h264_qvalue=30&videoin_c0_s1_h264_quant=4&"
		"stream2_mjpeg_resolution=640x360&videoin_c0_s1_mjpeg_maxframe=30&"
		"videoin_c0_s1_mjpeg_qvalue=50&videoin_c0_s1_mjpeg_quant=1&"
		"stream3_mpeg4_resolution=640x360&videoin_c0_s2_mpeg4_maxframe=30&"
		"videoin_c0_s2_mpeg4_intraperiod=1000&videoin_c0_s2_mpeg4_ratecontrolmode=cbr&"
		"videoin_c0_s2_mpeg4_bitrate=8000000&videoin_c0_s2_mpeg4_qvalue=7&"
		"videoin_c0_s2_mpeg4_quant=99&videoin_c0_s2_codectype=h264&"
		"stream3_h264_resolution=640x360&videoin_c0_s2_h264_maxframe=30&"
		"videoin_c0_s2_h264_intraperiod=1000&videoin_c0_s2_h264_ratecontrolmode=cbr&"
		"videoin_c0_s2_h264_bitrate=1500000&videoin_c0_s2_h264_qvalue=30&"
		"videoin_c0_s2_h264_quant=4&stream3_mjpeg_resolution=640x360&"
		"videoin_c0_s2_mjpeg_maxframe=1&videoin_c0_s2_mjpeg_qvalue=50&"
		"videoin_c0_s2_mjpeg_quant=1&stream4_mpeg4_resolution=176x144&"
		"videoin_c0_s3_mpeg4_maxframe=30&videoin_c0_s3_mpeg4_intraperiod=1000&"
		"videoin_c0_s3_mpeg4_ratecontrolmode=cbr&videoin_c0_s3_mpeg4_bitrate=512000&"
		"videoin_c0_s3_mpeg4_qvalue=7&videoin_c0_s3_mpeg4_quant=3&"
		"stream4_h264_resolution=176x144&videoin_c0_s3_h264_maxframe=30&"
		"videoin_c0_s3_h264_intraperiod=1000&videoin_c0_s3_h264_ratecontrolmode=cbr&"
		"videoin_c0_s3_h264_bitrate=512000&videoin_c0_s3_h264_qvalue=30&"
		"videoin_c0_s3_h264_quant=4&videoin_c0_s3_codectype=mjpeg&"
		"stream4_mjpeg_resolution=176x144&videoin_c0_s3_mjpeg_maxframe=1&"
		"videoin_c0_s3_mjpeg_qvalue=50&videoin_c0_s3_mjpeg_quant=1&"
		"ircutcontrol_bwmode=1&ircutcontrol_mode=auto&"
		"ircutcontrol_daymodebegintime=07\%3A00&ircutcontrol_daymodeendtime=18\%3A00&"
		"ircutcontrol_sensitivity=normal&ircutcontrol_disableirled=0&"
		"videoin_c0_s0_resolution=1280x720&videoin_c0_s1_resolution=640x360&"
		"videoin_c0_s2_resolution=640x360&videoin_c0_s3_resolution=176x144&"
		"roi_c0_s0_size=1280x720&roi_c0_s1_size=1280x720&roi_c0_s2_size=1280x720&"
		"roi_c0_s0_home=0\%2C0&roi_c0_s1_home=0\%2C0&roi_c0_s2_home=0\%2C0";

	char *http_api = NULL;
	char auth_encbuf[256];
	char sock_buf[5120];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int fps1, fps2;



	http_api = (char*) malloc(5120);
	if (http_api == NULL) return IPCAM_SETUP_RTN_FAILED;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	fps1 = _get_fps_int(info_set->vcodec.fps[0]);
	fps2 = _get_fps_int(info_set->vcodec.fps[1]);
	snprintf(http_api, 5120, set_api_raw,
			fps1, info_set->vcodec.bitrate[0], fps2, info_set->vcodec.bitrate[1]);
	snprintf(sock_buf, 5120, str_api_raw, ip_str, strlen(http_api),
			ip_str, auth_encbuf, ip_str, "video.html", http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(http_api);
		http_api = NULL;
		return (0);
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(http_api);
		http_api = NULL;
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
		http_api = NULL;
		return (0);
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock);

#if 0
	if (recv(sock, sock_buf, 2048, 0) < 0)
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

	free(http_api);
	http_api = NULL;
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

static int _get_fps_int(unsigned int fps_mask)
{
	int i = 0;
	int rtn_fps[] = { 30,15,10,7,4, 2,1,25,12,6, 3,1 };

	for (i = 0; (1<<i)<NF_IPCAM_FPS_MAX; i++)
	{
		if ((1<<i) == fps_mask)
		{
			return rtn_fps[i];
		}
	}

	return 0;
}

#endif	//__NF_IPCAM_DRIVER_VIVOTEK_C__
