/*
 * ITX Security
 *  System software group
 *
 *  2010-12-08 jykim
 */

#ifndef __NF_IPCAM_DRIVER_ITX_PTZ_C__
#define __NF_IPCAM_DRIVER_ITX_PTZ_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <glib.h>
// #include <gst/gst.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_ptz.h>
#include <nfdal.h>

#include <openssl/ssl.h>


#define UNIT_TEST
#undef UNIT_TEST

#define WAIT_REPLY_SECS		(2)
#define PRINT_HTTP_API_SEND	(0)
#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)


static unsigned short cam_setup_reboot_idx = 0xea2e;

static const char str_api_raw[] = 
	"POST /cgi-bin/action.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"Referer: http://%s/html/%s\r\n"	// IP address / video_codec.htm
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

static const char str_reboot_raw[] = 
	"POST /cgi-bin/reboot.cgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Referer: http://%s/html/system_maintenance.htm\r\n"	// IP address
	"Accept-Language: ko\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Content-Type: multipart/form-data; boundary=---------------------------7da%04x70674\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: 148\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"-----------------------------7da%04x70674\r\n"
	"Content-Disposition: form-data; name=\"control\"\r\n"
	"\r\n"
	"%s\r\n"
	"-----------------------------7da%04x70674--\r\n";

static char* resol_string[] = {
	"1920x1080", "1920x1080_w", "1280x1024", "1024x768", "1280x720",
	"1280x720_w", "720x576", "720x480", "704x576", "704x480",
	"640x480", "640x352", "352x288", "352x240", "320x240",
	"640x360", "320x180", "640x360_w", "600x400", "800x450",
	"1440x900", "800x600", "1600x1200","2304x1296", "2048x1536",
	"2560x1440", "2688x1520",  "2560x1600","2560x1920", "2592x1920",
	"2592x1944", "2992x1680", "2880x1800", "3200x1800", "2880x2160",
    "3072x2048", "3200x2400", "3840x2160", "2592x1520" };
static char* fps_string[] = {
	"30", "15", "10", "7", "4", "2", "1", "25", "12", "6", "3", "12.5"
};



extern void nf_ipcam_waiting_settime(int port, int msec);




static int get_resol_index(char* resol_str)
{
	int i = 0;

	for (i = 0; i < 11; i++)
	{
		if (strcmp(resol_str, resol_string[i]) == 0)
			return i;
	}

	return (0xff);
}

static char* get_resol_string(uint64_t resol_code)
{
	if(resol_code == NF_IPCAM_RES_1920x1080)
		return resol_string[0];
	else if(resol_code == NF_IPCAM_RES_1920x1080I)
		return resol_string[1];
	else if(resol_code == NF_IPCAM_RES_1280x1024)
		return resol_string[2];
	else if(resol_code == NF_IPCAM_RES_1024x768)
		return resol_string[3];
	else if(resol_code == NF_IPCAM_RES_1280x720)
		return resol_string[4];
	else if(resol_code == NF_IPCAM_RES_1280x720I)
		return resol_string[5];
	else if(resol_code == NF_IPCAM_RES_720x576)
		return resol_string[6];
	else if(resol_code == NF_IPCAM_RES_720x480)
		return resol_string[7];
	else if(resol_code == NF_IPCAM_RES_704x576)
		return resol_string[8];
	else if(resol_code == NF_IPCAM_RES_704x480)
		return resol_string[9];
	else if(resol_code == NF_IPCAM_RES_640x480)
		return resol_string[10];
	else if(resol_code == NF_IPCAM_RES_640x352)
		return resol_string[11];
	else if(resol_code == NF_IPCAM_RES_352x288)
		return resol_string[12];
	else if(resol_code == NF_IPCAM_RES_352x240)
		return resol_string[13];
	else if(resol_code == NF_IPCAM_RES_320x240)
		return resol_string[14];
	else if(resol_code == NF_IPCAM_RES_640x360)
		return resol_string[15];
	else if(resol_code == NF_IPCAM_RES_320x180)
		return resol_string[16];
	else if(resol_code == NF_IPCAM_RES_640x360I)
		return resol_string[17];
	else if(resol_code == NF_IPCAM_RES_640x400)
		return resol_string[18];
	else if(resol_code == NF_IPCAM_RES_800x450)
		return resol_string[19];
	else if(resol_code == NF_IPCAM_RES_1440x900)
		return resol_string[20];
	else if(resol_code == NF_IPCAM_RES_800x600)
		return resol_string[21];
	else if(resol_code == NF_IPCAM_RES_1600x1200)
		return resol_string[22];
	else if(resol_code == NF_IPCAM_RES_2304x1296)
		return resol_string[23];
	else if(resol_code == NF_IPCAM_RES_2048x1536)
		return resol_string[24];
	else if(resol_code == NF_IPCAM_RES_2560x1440)
		return resol_string[25];
	else if(resol_code == NF_IPCAM_RES_2688x1520)
		return resol_string[26];
	else if(resol_code == NF_IPCAM_RES_2560x1600)
		return resol_string[27];
	else if(resol_code == NF_IPCAM_RES_2560x1920)
		return resol_string[28];
	else if(resol_code == NF_IPCAM_RES_2592x1920)
		return resol_string[29];
	else if(resol_code == NF_IPCAM_RES_2592x1944)
		return resol_string[30];
	else if(resol_code == NF_IPCAM_RES_2992x1680)
		return resol_string[31];
	else if(resol_code == NF_IPCAM_RES_2880x1800)
		return resol_string[32];
	else if(resol_code == NF_IPCAM_RES_3200x1800)
		return resol_string[33];
	else if(resol_code == NF_IPCAM_RES_2880x2160)
		return resol_string[34];
	else if(resol_code == NF_IPCAM_RES_3072x2048)
		return resol_string[35];
	else if(resol_code == NF_IPCAM_RES_3200x2400)
		return resol_string[36];
	else if(resol_code == NF_IPCAM_RES_3840x2160)
		return resol_string[37];
	else if(resol_code == NF_IPCAM_RES_2592x1520)
		return resol_string[38];
	else if(resol_code == NF_IPCAM_RES_3000x3000)
		return resol_string[39];
	else if(resol_code == NF_IPCAM_RES_2048x2048)
		return resol_string[40];
	else if(resol_code == NF_IPCAM_RES_1280x1280)
		return resol_string[41];
	else if(resol_code == NF_IPCAM_RES_640x640)
		return resol_string[42];
	else if(resol_code == NF_IPCAM_RES_320x320)
		return resol_string[43];
	else 
		return resol_string[0];
}

static char* get_fps_string(unsigned int fps_code)
{
	int rtn_id = 0;

	switch(fps_code)
	{
		case NF_IPCAM_FPS_300: { rtn_id =  0; break; }
		case NF_IPCAM_FPS_150: { rtn_id =  1; break; }
		case NF_IPCAM_FPS_100: { rtn_id =  2; break; }
		case NF_IPCAM_FPS_70:  { rtn_id =  3; break; }
		case NF_IPCAM_FPS_40:  { rtn_id =  4; break; }
		case NF_IPCAM_FPS_20:  { rtn_id =  5; break; }
		case NF_IPCAM_FPS_10:  { rtn_id =  6; break; }
		case NF_IPCAM_FPS_250: { rtn_id =  7; break; }
		case NF_IPCAM_FPS_120: { rtn_id =  8; break; }
		case NF_IPCAM_FPS_60:  { rtn_id =  9; break; }
		case NF_IPCAM_FPS_30:  { rtn_id = 10; break; }
		case NF_IPCAM_FPS_125: { rtn_id = 11; break; }
		default:               { rtn_id =  0; break; }
	}

	return fps_string[rtn_id];
}

static int get_index_from_bitmask(unsigned int value)
{
	int i = 0;

	while ((1<<i != value) && i < 32) i++;

	if (i == 32) return (0);

	return i;
}

static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);
static int get_resol_index(char* resol_str);
static char* get_resol_string(uint64_t resol_code);
static char* get_fps_string(unsigned int fps);
static int get_index_from_bitmask(unsigned int value);
static int npt_get_eshutter_index(int speed, int ch);
static int npt_get_eshutter_index_x10(int speed, int ch);
static int npt_get_gain_index(int dB);


/* SSL encrypted API */




extern int npt_set_install_mode_off(int cam_id)
{
	const char set_install_mode_raw[] =
			"action=set_setup&menu=video.install&"
			"install_mode=off&video_format=off";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, set_install_mode_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "install.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return (0);
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
			free(sock_buf);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
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
		free(sock_buf);
		return (0);
	}

	free(sock_buf);
	nf_ipcam_waiting_settime(cam_id, 10000);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_INIT, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_vcodec(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
			"action=set_setup&menu=video.ctl_codec&"
			"codec0=h264&resolution0=%s&bitctrl0=cbr&bitavr0=%d&fps0=%s&"
			"codec1=h264&resolution1=%s&bitctrl1=cbr&bitavr1=%d&fps1=%s&"
			"ff_mode=%d&mirror_mode=%s&bandon=no&bandwidth=5000&"
			"gopsize0=%s&gopsize1=%s&jpegqual=80";

	const char *mirr_str[] = { "none", "none", "h_mirror", "v_mirror", "hv_mirror" };

	char http_api[512];
	char auth_encbuf[256];
	char *sock_buf;
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

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];
	if (info_set->vcodec.af == 60)
	{
		snprintf(http_api, 512, set_vcodec_raw,
				get_resol_string(info_set->vcodec.resolution[0]), 
				info_set->vcodec.bitrate[0],
				get_fps_string(info_set->vcodec.fps[0]),
				get_resol_string(info_set->vcodec.resolution[1]), 
				info_set->vcodec.bitrate[1],
				get_fps_string(info_set->vcodec.fps[1]),
				//"7",
				info_set->vcodec.af, mirr,
				get_fps_string(info_set->vcodec.fps[0]),
				get_fps_string(info_set->vcodec.fps[1])
				);
				//"7");
	}
	else
	{
		snprintf(http_api, 512, set_vcodec_raw,
				get_resol_string(info_set->vcodec.resolution[0]), 
				info_set->vcodec.bitrate[0],
				get_fps_string(info_set->vcodec.fps[0]),
				get_resol_string(info_set->vcodec.resolution[1]), 
				info_set->vcodec.bitrate[1],
				get_fps_string(info_set->vcodec.fps[1]),
				//"6",
				info_set->vcodec.af, mirr,
				get_fps_string(info_set->vcodec.fps[0]),
				get_fps_string(info_set->vcodec.fps[0])
				);
				//"6");
	}

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			"video_codec.htm",
			ip_str,
			strlen(http_api),
			auth_encbuf,
			http_api);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock);

	return IPCAM_SETUP_RTN_DONE;
}


extern int npt_set_image(image_info* info_set, int cam_id)
{
	const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto","auto_m", "auto", "manual", "auto", "auto_m", "manual", "auto", "manual", "auto", "manual" };
	const char *offon_str[] = { "off", "on", "1", "2", "3", "4" };
	const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46" };
	const char *dnn_mode_str[] = { "auto", "day", "night", "", "", "", "", "auto", "day", "night", "auto", "day", "night", "schedule"};
	const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
	const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
	const char *wdr_mode_str[] = { "off", "", "", "off", "on", "low", "middle", "high", "low", "mid", "high", "on", "low", "mid", "high", "on", "low", "middle", "high"};
	const char *focus_mode_str[] = { "auto", "", "", "", "", "manual", "", "auto", "s_auto" };
	const char *defog[] = {"off", "on", "low", "mid", "high", "off", "low", "mid", "high", "off", "low", "mid", "high"};
	const char *hlc[] = {"off", "low", "mid", "high", "off", "low", "mid", "high"};
	const char *blc_mode_str[] = {"off", "on","", "", "", "", "", "", "", "off", "on"};
	const char *slow_shutter[] = {"off", "on", "", "", "off", "on"};
	const char *ff_mode_str[] = {"60", "50", "off", "off", "on", "off", "on"};
	const char *focus_limit[] = {"30","100"};
	const char set_image_raw[] =
#if 0
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=%s&blc_ctrl=%s&dnn_mode=%s&dnn_det_time=%s&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d%s";
#endif
#if 0
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=gain_%d&me_shutter=shut_%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=on&blc_ctrl=%s&dnn_mode=%s&dnn_det_time=det_0&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d";
#endif
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=gain_%d&me_shutter=shut_%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=on&blc_ctrl=%s&wd_mode=%s&dnn_mode=%s&dnn_det_time=det_0&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d&focus_mode=%s&"
			"anti_mode=%s&defog_ctrl=%s&hlc_ctrl=%s&dnn_sensitivity=%d";

			// NPT2 EV Model
	const char set_image_raw_ev[] =
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=gain_%d&me_shutter=shut_%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=on&blc_ctrl=%s&wdr_ctrl=%s&dnn_mode=%s&dnn_det_time=det_0&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d&focus_mode=%s&"
			"anti_mode=%s&defog_ctrl=%s&hlc_ctrl=%s&dnn_sensitivity=%d&"
			"focus_limit=%s&stabilizer=%s&ir_correction=%s";

			// NPT2 EH Model
	const char set_image_raw_eh[] =
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=gain_%d&me_shutter=shut_%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=on&blc_ctrl=%s&wd_mode=%s&dnn_mode=%s&dnn_det_time=det_0&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d&focus_mode=%s&"
			"anti_mode=%s&dnn_sensitivity=%d&focus_limit=%s&stabilizer=%s&ir_correction=%s";
	char http_api[512];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	// Camera SDK Version info
	int major, sdk_type, sub_type, minor;

	mtable* runtime = get_runtime();
	sscanf(runtime[cam_id].sys.sdkver, "%d.%d.%d.%d", &major, &sdk_type, &sub_type, &minor);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	if(major >=  6)
	{
		if(sub_type == 0 && sdk_type < 4)
		{
		const char *wdr_mode_str[] = { "off", "", "", "off", "on", "low", "middle", "high", "low", "mid", "high", "on", "low", "middle", "high", "on", "low", "middle", "high"};
		snprintf(http_api, 512, set_image_raw_eh,
				ae_mode_str[info_set->ae],
				npt_get_gain_index(info_set->agc),
				npt_get_eshutter_index(info_set->shutter, cam_id),
				slow_shutter[info_set->ss], magc_str[info_set->max_agc],
				blc_mode_str[info_set->blc], wdr_mode_str[info_set->wd], dnn_mode_str[info_set->dnn],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, focus_mode_str[info_set->focus_mode],
				ff_mode_str[info_set->ff_mode],info_set->dnn_sense_ntod,
				focus_limit[info_set->focus_limit],offon_str[info_set->stabilizer], offon_str[info_set->ir_correction] );
		}
		else if(sub_type == 1 || sdk_type > 3)
		{
			int eshutter_index = 4;

			if(strstr(runtime[cam_id].sys.stdver, "2003P10") != NULL)
				eshutter_index = npt_get_eshutter_index_x10(info_set->shutter, cam_id);
			else
				eshutter_index = npt_get_eshutter_index(info_set->shutter, cam_id);


		snprintf(http_api, 512, set_image_raw_ev,
				ae_mode_str[info_set->ae],
				npt_get_gain_index(info_set->agc),
				eshutter_index,
				slow_shutter[info_set->ss], magc_str[info_set->max_agc],
				blc_mode_str[info_set->blc], wdr_mode_str[info_set->wd], dnn_mode_str[info_set->dnn],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, focus_mode_str[info_set->focus_mode],
				ff_mode_str[info_set->ff_mode],
				defog[info_set->defog], hlc[info_set->hlc], info_set->dnn_sense_ntod,
				focus_limit[info_set->focus_limit],offon_str[info_set->stabilizer], offon_str[info_set->ir_correction] );
		}
	}
	else
	{

		snprintf(http_api, 512, set_image_raw,
				ae_mode_str[info_set->ae],
				npt_get_gain_index(info_set->agc),
				npt_get_eshutter_index(info_set->shutter, cam_id),
				slow_shutter[info_set->ss], magc_str[info_set->max_agc],
				blc_mode_str[info_set->blc], wdr_mode_str[info_set->wd], dnn_mode_str[info_set->dnn],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, focus_mode_str[info_set->focus_mode],
				ff_mode_str[info_set->ff_mode],
				defog[info_set->defog], hlc[info_set->hlc], info_set->dnn_sense_ntod);
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_camera_x.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_IMAGE, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_dnn_adjust_d2n(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_d2n_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=d2n&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	unsigned short http_port;
	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_d2n_raw,
			info_set->dnn_sense_dton);

//	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
//			strlen(http_api), auth_str, http_api);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_camera_x.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

//	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, http_api);
//	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, sock_buf);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, sock);

	return IPCAM_SETUP_RTN_DONE;

}

extern int npt_set_dnn_adjust_n2d(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_n2d_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=n2d&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	unsigned short http_port;
	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_n2d_raw,
			info_set->dnn_sense_ntod);

//	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
//			strlen(http_api), auth_str, http_api);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_camera_x.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

//	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, http_api);
//	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, sock_buf);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
//	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, sock);

	return IPCAM_SETUP_RTN_DONE;

}

/*
static int _cam_setup_send_plain(int cam_id, int type, char *buf)
{
	int sock;
	struct sockaddr_in sin;
	int len = 0;

	char ip_str[16];
	unsigned short http_port;



	IPCAM_DBG(MAJOR, "start CH(%d) type(%d)\n", cam_id, type);

	nf_ipcam_setup_sending(cam_id, type);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);


	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(WARN, "socket alloc fail CH(%d) type(%d)\n", cam_id, type);
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, type);
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
			IPCAM_DBG(WARN, "socket rcv timeout fail. CH(%d) type(%d)\n", cam_id, type);
			perror("setsockopt");
			close(sock);
			nf_ipcam_setup_send_done(cam_id, type);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "socket connect fail. CH(%d) type(%d)\n", cam_id, type);
		perror("connect");
		close(sock);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (send(sock, buf, strlen(buf), 0) < 0)
	{
		IPCAM_DBG(WARN, "send fail. CH(%d) type(%d)\n", cam_id, type);
		perror("send");
		close(sock);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) type(%d)\n%s\n", cam_id, type, buf);
#endif

	nf_ipcam_setup_waiting(cam_id, type, sock);

	return IPCAM_SETUP_RTN_DONE;
}
*/

typedef struct __privacy_mask_ptz
{
	int ch;
	int mask_no;
	int flag;
	int color;
	int tx;
	int ty;
	int bx;
	int by;
	int pixelmode;
} _privacy_mask_ptz;
static int _npt_set_pmask(_privacy_mask_ptz* pmask_info, int cam_id);

#define _NPT_MAX_PRIVACY_CNT 8

extern int npt_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id)
{
	int i;
	int rtn = IPCAM_SETUP_RTN_DONE;

	_privacy_mask_ptz param[_NPT_MAX_PRIVACY_CNT];
	memset(&param, 0x00, sizeof(_privacy_mask_ptz) * _NPT_MAX_PRIVACY_CNT);

	for(i = 0; i < _NPT_MAX_PRIVACY_CNT; i++)
	{

		param[i].ch = cam_id;
		param[i].mask_no = i;
		param[i].pixelmode = 0;//TBD
		if (pmask_info->lt[i].x < 0)
		{
			param[i].flag = 2;
			param[i].tx = 0;
			param[i].ty = 0;
			param[i].bx = 0;
			param[i].by = 0;
			param[i].color = 0;
		}
		else
		{
			param[i].flag = 1;
			param[i].tx = pmask_info->lt[i].x;
			param[i].ty = pmask_info->lt[i].y;
			param[i].bx = pmask_info->rb[i].x+1;
			param[i].by = pmask_info->rb[i].y+1;
			param[i].color = pmask_info->color[i];
		}
	}
	rtn = _npt_set_pmask(param, cam_id);
	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		printf("[%s] ERROR | CH(%d) privacy mask set fail\n", __FUNCTION__, cam_id);
	}
	else
	{
		printf("[%s] CH(%d) privacy mask set success\n", __FUNCTION__, cam_id);
	}
	return IPCAM_SETUP_RTN_DONE;

}

extern int npt_goto_pmask(int pmask_no, int cam_id)
{
	const char set_pmask_raw[] =
			"action=set_setup&menu=live.privacymask&"
			"maskarea%d=3&maskcolor%d=0&areatx%d=0&areaty%d=0&areabx%d=0&areaby%d=0&"
			"pixelmode=0";

	char http_api[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];

	unsigned short http_port;

	int in_type;
	int i;
	int sock;
	struct sockaddr_in sin;

	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_SET_PMASK);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);


	snprintf(http_api, SOCK_BUF_LENGTH, set_pmask_raw,
			pmask_no, pmask_no, pmask_no, pmask_no, pmask_no, pmask_no
			);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			"install.htm",
			ip_str,
			strlen(http_api),
			auth_encbuf,
			http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		//nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
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
			//nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		//nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
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
		//nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
		return IPCAM_SETUP_RTN_FAILED;
	}

	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_PMASK, sock);

	return IPCAM_SETUP_RTN_DONE;
}

static int _npt_set_pmask(_privacy_mask_ptz* pmask_info, int cam_id)
{
#if 0
	const char set_pmask_raw[] =
			"action=set_setup&menu=live.privacymask&"
			"maskarea%d=%d&maskcolor%d=%d&areatx%d=%d&areaty%d=%d&areabx%d=%d&areaby%d=%d&"
			"pixelmode=%d";
#endif
	const char set_pmask_raw[] =
			"action=set_setup&menu=live.privacymask&"
			"%s"
			"pixelmode=%d";

	const char pmask_content_raw[] =
			"maskarea%d=%d&maskcolor%d=%d&areatx%d=%d&areaty%d=%d&areabx%d=%d&areaby%d=%d&";

	char http_api[SOCK_BUF_LENGTH];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];

	unsigned short http_port;

	int in_type;
	int i;
	int sock;
	struct sockaddr_in sin;

	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_SET_PMASK);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	char temp[8][1024];
	char pmask_content[1024];
	for(i = 0; i < _NPT_MAX_PRIVACY_CNT; i++)
	{
		snprintf(temp[i], 1024, pmask_content_raw,
				pmask_info[i].mask_no, pmask_info[i].flag,
				pmask_info[i].mask_no, pmask_info[i].color,
				pmask_info[i].mask_no, pmask_info[i].tx,
				pmask_info[i].mask_no, pmask_info[i].ty,
				pmask_info[i].mask_no, pmask_info[i].bx,
				pmask_info[i].mask_no, pmask_info[i].by);
	}
	snprintf(pmask_content, 1024, "%s%s%s%s%s%s%s%s",
			temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]);


	snprintf(http_api, SOCK_BUF_LENGTH, set_pmask_raw,
			pmask_content,
			pmask_info[0].pixelmode);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			"install.htm",
			ip_str,
			strlen(http_api),
			auth_encbuf,
			http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
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
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
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
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_PMASK);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_PMASK, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_ptz_init(int cam_id)
{
	const char http_api_raw[] = "action=set_setup&menu=ptz.setup&action_mode=continue_mode";
	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return (0);
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
			free(sock_buf);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
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
		free(sock_buf);
		return (0);
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM1, sock);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_INIT, sock);

	return 1;
}

extern int npt_get_af_capa(cam_info *info, int cam_id)
{
	// TODO
	info->afcapa.mfz = 1;
	info->afcapa.zoom_min = 0;
	info->afcapa.zoom_max = 1024;
	info->afcapa.focus_min = 0;
	info->afcapa.focus_max = 768;
	info->afcapa.iris_min = 0;
	info->afcapa.iris_max = 13;
	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_onepush(int cam_id)
{
	const char http_api_raw[] = "action=set_setup&menu=ptz.setup&one_shot_focus=on";
	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return (0);
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
			free(sock_buf);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
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
		free(sock_buf);
		return (0);
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_INIT, sock);

	return 1;
}

extern int npt_set_pt(ptz_info* info, int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.keypush&move=%s&speed=%d";

	const char* directions[] = {
		"left", "right", "up", "down",
		"","", "upleft", "downleft",
		"upright", "downright", "","",
		"","", "stop"
	};

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int speed = 0;
	PtzData ptzProp;

	switch(info->cmd)
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

	memset(http_api, 0x00, sizeof(http_api));
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

	speed = info->pt_speed;

	if(1 <= speed && 5 >= speed)
		speed = speed *2;
	else if(5 < speed && 10 >= speed)
		speed = (speed - 5) * 34;
	else
		speed = 10;

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[info->cmd], speed);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_zoom(ptz_info* info, int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.zoom&"
			"zoom_mode=keypush&zoom_action=%s&zoom_speed=%d";

	const char* directions[] = {
		"","","","","wide","tele",
		"","","","",
		"","","","",
		"stop"
	};

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int speed = 0;
	PtzData ptzProp;

	int sock;
	struct sockaddr_in sin;

	printf("\n\n\n[%s] start\n\n\n", __FUNCTION__);
	switch(info->cmd)
	{
		case NF_PTZ_CMD_ZOOM_WIDE:
		case NF_PTZ_CMD_ZOOM_TELE:
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-Zoom command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	memset(http_api, 0x00, sizeof(http_api));

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	speed = info->zoom_speed;
	speed = round((8.0 / 10.0) * speed);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[info->cmd], speed);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ZOOM, sock);

	free(sock_buf);

	mtable* runtime = get_runtime();
	IPCAM_DBG(MINOR, "Current Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

	if (runtime[cam_id].ptz.moving & PTZ_SETUP_ZOOM && info->cmd == NF_PTZ_CMD_STOP)
	{
		runtime[cam_id].ptz.moving &= ~PTZ_SETUP_ZOOM;
	}

	IPCAM_DBG(MINOR, "Stop Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_focus(ptz_info* info, int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.focus&"
			"focus_mode=keypush&focus_move=%s&focus_speed=%d";

	const char* directions[] = {
		"","","","","","",
		"","","","",
		"","","near","far",
		"stop"
	};

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	int speed = 0;
	PtzData ptzProp;

	switch(info->cmd)
	{
		case NF_PTZ_CMD_FOCUS_NEAR:
		case NF_PTZ_CMD_FOCUS_FAR:
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-Zoom command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	memset(http_api, 0x00, sizeof(http_api));

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	speed = info->focus_speed;

	speed = round((8.0 / 10.0) * speed);

	if(speed < 1 || speed > 8)
		speed = 5;

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[info->cmd], speed);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_FOCUS, sock);

	free(sock_buf);

	mtable* runtime = get_runtime();
	IPCAM_DBG(MINOR, "Current Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

	if (runtime[cam_id].ptz.moving & PTZ_SETUP_FOCUS && info->cmd== NF_PTZ_CMD_STOP)
	{
		runtime[cam_id].ptz.moving &= ~PTZ_SETUP_FOCUS;
	}

	IPCAM_DBG(MINOR, "Stop Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);
	return IPCAM_SETUP_RTN_DONE;
}
#if 0
extern int npt_set_iris(NF_PTZ_CMD_E ptz_cmd, int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.iris&"
			"iris_mode=keypush&iris_move=%s";

	const char* directions[] = {
		"","","","","","",
		"","","","",
		"open","close","","",
		"stop"
	};

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	printf("[%s] start\n", __FUNCTION__);
	switch(ptz_cmd)
	{
		case NF_PTZ_CMD_IRIS_OPEN:
			printf("[%s] NPT iris open\n", __FUNCTION__);
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			printf("[%s] NPT iris close\n", __FUNCTION__);
			break;
		case NF_PTZ_CMD_STOP:
			break;
		default:
		{
			printf("Non-IRIS command\n");
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	printf("[%s] %d\n", __FUNCTION__, __LINE__);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	printf("[%s] %d\n", __FUNCTION__, __LINE__);
	strcat(username, ":");
	strcat(username, password);

	printf("[%s] %d\n", __FUNCTION__, __LINE__);
	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, directions[ptz_cmd]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	printf("[%s] %d\n", __FUNCTION__, __LINE__);
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	printf("[%s] %d\n", __FUNCTION__, __LINE__);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}
	printf("[%s] %d\n", __FUNCTION__, __LINE__);
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
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
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
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_IRIS, sock);

	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}
#endif

extern int npt_set_iris(int value, int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.iris&"
			"iris_mode=direct&iris_direct=%d&iris_move=open";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int rtn;
	int sock;
	struct sockaddr_in sin;

	mtable* runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, value);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus_ptz.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto end_err;
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
			free(sock_buf);
			rtn = IPCAM_SETUP_RTN_FAILED;
			goto end_err;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto end_err;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto end_err;
	}

	close(sock);
	sock = (-1);
	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_IRIS, sock);

	free(sock_buf);
	rtn = IPCAM_SETUP_RTN_DONE;

end_err: 
	if (runtime[cam_id].ptz.moving & PTZ_SETUP_IRIS)
	{
		runtime[cam_id].ptz.moving &= ~PTZ_SETUP_IRIS;
	}
	return rtn;
}


extern int npt_set_origin(int cam_id)
{
	const char http_api_raw[] =
			"action=set_setup&menu=ptz.keypush&move=home";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_ORIGIN);
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
			free(sock_buf);
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_ORIGIN);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		free(sock_buf);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_ORIGIN);
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
		free(sock_buf);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_ORIGIN);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_ORIGIN, sock);

	free(sock_buf);
	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_autofocus_mode(int enable, int cam_id)
{
	const char http_api_raw[] = "action=set_setup&menu=ptz.setup&auto_focus=%s";
	const char *onoff[] = { "off", "on" };
	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	if (enable != 0 && enable != 1)
	{
		printf("param fail\n");
		return (0);
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, onoff[enable]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return (0);
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
			free(sock_buf);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
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
		free(sock_buf);
		return (0);
	}

	close(sock);
	sock = (-1);
	free(sock_buf);

	return 1;
}

extern int npt_set_autoiris_mode(int enable, int cam_id)
{
	const char http_api_raw[] = "action=set_setup&menu=ptz.setup&auto_iris=%s";
	const char *onoff[] = { "off", "on" };
	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;


	if (enable != 0 && enable != 1)
	{
		printf("param fail\n");
		return (0);
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, onoff[enable]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		free(sock_buf);
		return (0);
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
			free(sock_buf);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
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
		free(sock_buf);
		return (0);
	}

	close(sock);
	sock = (-1);
	free(sock_buf);
	return 1;
}

extern int npt_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	const char *http_api_raw[] =
	{
		"action=set_setup&menu=ptz.keypush&move=stop",
		"action=set_setup&menu=ptz.zoom&move=stop",
		"action=set_setup&menu=ptz.focus&move=stop",
		"action=set_setup&menu=ptz.iris&move=stop"
	};

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;


	ptz_info info;
	memset(&info, 0x00, sizeof(info));
	switch(e)
	{
		case PTZ_SETUP_PAN:
		case PTZ_SETUP_TILT:
			id = 0;
			break;
		case PTZ_SETUP_ZOOM:
			info.cmd = NF_PTZ_CMD_STOP;
			return npt_set_zoom(&info, cam_id);
		case PTZ_SETUP_FOCUS:
			info.cmd = NF_PTZ_CMD_STOP;
			return npt_set_focus(&info, cam_id);
		case PTZ_SETUP_IRIS:
			return IPCAM_SETUP_RTN_DONE;
			//return npt_set_iris(NF_PTZ_CMD_STOP, cam_id);
		default:
			return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(http_api, 256, http_api_raw[id]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);
	free(sock_buf);

	mtable* runtime = get_runtime();
	IPCAM_DBG(MINOR, "Current Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

	if(runtime[cam_id].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT))
	{
		runtime[cam_id].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT);
	}
	if (runtime[cam_id].ptz.moving & PTZ_SETUP_IRIS)
	{
		runtime[cam_id].ptz.moving &= ~PTZ_SETUP_IRIS;
	}

	IPCAM_DBG(MINOR, "Stop Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

	//nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_STOP, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_get_pan(int *value, int cam_id)
{
	*value = 0;
	return (1);
}

extern int npt_get_tilt(int *value, int cam_id)
{
	*value = 0;
	return (1);
}

extern int npt_get_zoom(int *value, int cam_id)
{
	const char http_api_raw[] =
		"action=get_setup&menu=ptz.query";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	char buf[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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
		tv.tv_sec = 7;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus_ptz.htm", ip_str,
			strlen(http_api_raw), auth_encbuf, http_api_raw);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 6;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_start[]		= "q_zoom_pos=";

		/* piris parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_start);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_start);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_GET_IRIS, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_get_focus(int *value, int cam_id)
{
	const char http_api_raw[] =
		"action=get_setup&menu=ptz.query";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	char buf[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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
		tv.tv_sec = 7;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus_ptz.htm", ip_str,
			strlen(http_api_raw), auth_encbuf, http_api_raw);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 6;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_start[]		= "q_focus_pos=";

		/* piris parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_start);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_start);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_GET_IRIS, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_get_iris(int *value, int cam_id)
{
	const char http_api_raw[] =
		"action=get_setup&menu=ptz.query";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	char buf[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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
		tv.tv_sec = 7;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus_ptz.htm", ip_str,
			strlen(http_api_raw), auth_encbuf, http_api_raw);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 6;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			free(sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_start[]		= "q_iris_pos=";

		/* piris parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_start);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_start);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_GET_IRIS, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_set_preset(int num, int cam_id)
{
	const char http_api_raw[] =
		"action=set_setup&menu=ptz.config&conf_mode=preset_set&set_preset_num=%d";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(http_api, 256, http_api_raw, num);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_PRESET_SET, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_clear_preset(int num, int cam_id)
{
	const char http_api_raw[] =
		"action=set_setup&menu=ptz.config&conf_mode=preset_rm&remove_preset_num=%d";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(http_api, 256, http_api_raw, num);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_PRESET_CLEAR, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int npt_go_preset(int num, int cam_id)
{
	const char http_api_raw[] =
		"action=set_setup&menu=ptz.gotopre&goto_preset_mode=number&goto_preset_num=%d";

	char http_api[256];
	char auth_encbuf[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int id;
	int sock;
	struct sockaddr_in sin;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(http_api, 256, http_api_raw, num);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "video_autofocus.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);


#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		free(sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	free(sock_buf);
	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_PRESET_GO, sock);

	return IPCAM_SETUP_RTN_DONE;
}


static int npt_get_gain_index(int dB)
{
	int result = 1;

	if (dB < 0 ) return 0;

	result = (dB / 2) + (dB % 2) + 1;

	if (result > 15) result = 15;

	return result;
}
static int npt_get_eshutter_index(int speed, int ch)
{
	int result = 5;
	int ntpal = 0;
	mtable *runtime = get_runtime();

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

	if (ntpal == 0) // NTSC
	{
		if (speed < 45) result = 5;
		else if (speed < 75) result = 6;
		else if (speed < 95) result = 7;
		else if (speed < 115) result = 8;
		else if (speed < 150) result = 9;
		else if (speed < 215) result = 10;
		else if (speed < 300) result = 11;
		else if (speed < 450) result = 12;
		else if (speed < 650) result = 13;
		else if (speed < 850) result = 14;
		else if (speed < 1250) result = 15;
		else if (speed < 1750) result = 16;
		else if (speed < 2500) result = 17;
		else if (speed < 3500) result = 18;
		else if (speed < 5000) result = 19;
		else if (speed < 8000) result = 20;
		else result = 21;
	}
	else	// PAL
	{
		if (speed < 40) result = 5;
		else if (speed < 60) result = 6;
		else if (speed < 90) result = 7;
		else if (speed < 110) result = 8;
		else if (speed < 135) result = 9;
		else if (speed < 180) result = 10;
		else if (speed < 270) result = 11;
		else if (speed < 370) result = 12;
		else if (speed < 520) result = 13;
		else if (speed < 800) result = 14;
		else if (speed < 1150) result = 15;
		else if (speed < 1500) result = 16;
		else if (speed < 2100) result = 17;
		else if (speed < 3000) result = 18;
		else if (speed < 4750) result = 19;
		else if (speed < 8000) result = 20;
		else result = 21;
	}

	return result;
}

/* NPT2 x10 */
static int npt_get_eshutter_index_x10(int speed, int ch)
{
	int result = 5;
	int ntpal = 0;
	mtable *runtime = get_runtime();

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

	if (ntpal == 0) // NTSC
	{
		if (speed < 45) result = 3;
		else if (speed < 90) result = 4;
		else if (speed < 180) result = 5;
		else if (speed < 370) result = 6;
		else if (speed < 750) result = 7;
		else if (speed < 1500) result = 8;
		else if (speed < 3000) result = 9;
		else if (speed < 6000) result = 10;
		else if (speed < 12000) result = 11;
		else if (speed < 23000) result = 12;
		else if (speed < 45000) result = 13;
		else result = 14;
	}
	else	// PAL
	{
		if (speed < 37) result = 3;
		else if (speed < 75) result = 4;
		else if (speed < 170) result = 5;
		else if (speed < 370) result = 6;
		else if (speed < 750) result = 6;
		else if (speed < 1500) result = 8;
		else if (speed < 3000) result = 9;
		else if (speed < 6000) result = 10;
		else if (speed < 12000) result = 11;
		else if (speed < 23000) result = 12;
		else if (speed < 45000) result = 13;
		else result = 14;
	}
	/* NPT2 x10 New Zoom Module */	
	if(runtime[ch].sys.zoom_module_name != NULL)
	{
		if((strstr(runtime[ch].sys.zoom_module_name, "MC-108") != NULL))
		//|| (atoi(runtime[ch].sys.zoom_module_fwver) >= 13 ) /* fwver check */
		{	
			if (ntpal == 0) // NTSC
			{
				if (speed < 45) result = 3;				//30
				else if (speed < 90) result = 4;		//60
				else if (speed < 185) result = 5;		//120
				else if (speed < 475) result = 6;		//250
				else if (speed < 850) result = 7;		//700
				else if (speed < 1300) result = 8;		//1000
				else if (speed < 2050) result = 9;		//1600
				else if (speed < 3750) result = 10;		//2500
				else if (speed < 6000) result = 11;		//5000
				else if (speed < 8500) result = 12;		//7000
				else if (speed < 20000) result = 13;	//10000
				else result = 14;						//30000
			}
			else    // PAL
			{
				if (speed < 37) result = 3;				//25	
				else if (speed < 75) result = 4;    	//50
				else if (speed < 175) result = 5;   	//100
				else if (speed < 475) result = 6;   	//250
				else if (speed < 850) result = 7;   	//700
				else if (speed < 1300) result = 8;  	//1000
				else if (speed < 2050) result = 9;  	//1600
				else if (speed < 3750) result = 10; 	//2500
				else if (speed < 6000) result = 11; 	//5000
				else if (speed < 8500) result = 12; 	//7000
				else if (speed < 20000) result = 13;	//10000
				else result = 14;                   	//30000
			}
		}
	}

	return result;
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


#ifdef UNIT_TEST
int main(int argc, char** argv)
{
	cam_reboot(0);
#if 0
	cam_info a;

	cam_get_install_mode(&a, 0);
	printf("%d\n", a.installation_mode);

	a.installation_mode = 0;
	cam_set_install_mode(&a, 0);
	memset(&a, 0x00, sizeof(cam_info));

	cam_get_install_mode(&a, 0);
	printf("%d\n", a.installation_mode);
#endif

#if 0
	cam_get_image(&a, 0);
	printf("%d %d %d %d\n", a.brightness, a.contrast, a.color, a.tint);
	a.brightness = 15;
	a.contrast = 15;
	a.color = 15;
	a.tint = 15;
	cam_set_image(&a, 0);
	memset(&a, 0x00, sizeof(cam_info));
	cam_get_image(&a, 0);
	printf("%d %d %d %d\n", a.brightness, a.contrast, a.color, a.tint);
#endif

#if 0
	a.resolution[0] = IPX_RES_1280x720;
	a.bitrate[0] = 8000;
	a.fps[0] = 30;
	a.second_stream = 1;
	a.resolution[1] = IPX_RES_640x480;
	a.bitrate[1] = 8000;
	a.fps[1] = 30;
	a.audio = 0;
	a.audio_codec = 0;
	cam_set_codec(&a, 0);

	memset(&a, 0x00, sizeof(a));
	cam_get_codec(&a, 0);

	printf("%d %d %d %d %d %d %d %d %d\n", a.resolution[0], a.bitrate[0], a.fps[0], a.second_stream, 
		a.resolution[1], a.bitrate[1], a.fps[1], a.audio, a.audio_codec);
#endif
}
#endif

#endif //__NF_IPCAM_DRIVER_ITX_C__
