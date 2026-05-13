/*
 * ITX Security
 *  System software group
 *
 *  2010-12-08 jykim
 */

#ifndef __NF_NVS_DRIVER_ITX_C__
#define __NF_NVS_DRIVER_ITX_C__

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
#include <nf_util_device.h>

#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)

typedef enum _NVS_PTZ_CONTROL_TYPE_E {
	NVS_PTZ_CMD_PT,
	NVS_PTZ_CMD_ZOOM,
	NVS_PTZ_CMD_FOCUS,
	NVS_PTZ_CMD_IRIS,
	NVS_PTZ_CMD_PTZ_STOP,
	NVS_PTZ_CMD_PRESET_SET,
	NVS_PTZ_CMD_PRESET_CLEAR,
	NVS_PTZ_CMD_PRESET_GO,
	NVS_PTZ_CMD_SET_AUTO_FOCUS,
	NVS_PTZ_CMD_SET_AUTO_IRIS,

	NVS_PTZ_CMD_NR,

} NVS_PTZ_CONTROL_TYPE_E;

static const char str_api_raw[] =
	"POST /cgi-bin/webra_fcgi.fcgi HTTP/1.1\r\n"
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

static int nvs_get_live_status(int *video_loss_status, int *motion_status, int *alarm_status, int cam_id);
static int nvs_ptz_control(NVS_PTZ_CONTROL_TYPE_E, int, int);
static int nvs_ptz_check_control_cmd_type(NVS_PTZ_CONTROL_TYPE_E, int, int, int*);

static void get_ipstr(unsigned int ip, char* outbuf)
{
	snprintf(outbuf, 16, "%d.%d.%d.%d", (ip&0xff), ((ip&0xff00)>>8),
			((ip&0xff0000)>>16), ((ip&0xff000000)>>24));
}
#if 0
static void ipcam_disc_port_link_state(int ch, int* p_layer, int* p_linked)
{
	int rtn = 0;
	int is_linked = 0;

#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
	if (ch >= 0 && ch < NUM_ACTIVE_CH_DVR)
#else
	if (ch >= 0 && ch < 8)
#endif
	{
		is_linked = ch;
		/* Port link state polling */
		nf_dev_switch_get_link_status(&is_linked);
		if (is_linked)
		{
			*p_layer = IPCAM_DISC_LAYER_DVR;
			*p_linked = 1;
			return;
		}
	}

	rtn = ipx_hub_current_link_status(ch);
	if (rtn) {
		*p_layer = IPCAM_DISC_LAYER_VHUB;
		*p_linked = 1;
		return;
	}

	*p_layer = 0;
	*p_linked = 0;
	return;
}
#endif
extern void ipcam_disc_port_link_state(int ch, int* p_layer, int* p_linked);

extern void nf_ipcam_waiting_settime(int port, int msec);

extern int nvs_itx_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
{
	mtable *runtime = get_runtime();

	if (runtime == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	return IPCAM_SETUP_RTN_DONE;
}

extern int nvs_get_model_info(cam_model_info* info_buf, int cam_id, int use_sysdb_idpass)
{
	const char get_model_raw[] = "api=get_setup.system.manage";
	// http://192.168.231.15:8080/cgi-bin/webra_fcgi.fcgi?api=get_setup.system.manage
	// http://192.168.231.55:8080/cgi-bin/webra_fcgi.fcgi?api=get_live.live.status&interval=3&curr_gmttime=0

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	const char nvs1ch_productcode[] = "23610";
	const char nvs4ch_productcode[] = "23110";
	const char nvs1ch_modelname[] = "NVS0103";
	const char nvs4ch_modelname[] = "NVS0424";

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	mtable *runtime = NULL;

	runtime = get_runtime();
	while (runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = runtime[cam_id].admin_http;
	if (use_sysdb_idpass == 1) {
		gchar key_u[64];
		gchar key_p[64];
		gchar *u,*p;
		snprintf(key_u, 64, "cam.logininfo.L%d.id", cam_id);
		snprintf(key_p, 64, "cam.logininfo.L%d.pwd", cam_id);
		u = nf_sysdb_get_str_nocopy(key_u);
		p = nf_sysdb_get_str_nocopy(key_p);
		strncpy(username, u, 64);
		strncpy(password, p, 64);
	}
	else {
		strcpy(username, "ADMIN");
		strcpy(password, "1234");
	}
	strncpy(runtime[cam_id].username, username, 64);
	strncpy(runtime[cam_id].password, password, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "setup_system_manage.htm", ip_str,
			strlen(get_model_raw), auth_encbuf, get_model_raw);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return (-1);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
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
				printf("[%s] CH%02d NO_SSL unauthrized\n", __FUNCTION__, cam_id);
				return (1);
			}
			printf("[%s] CH%02d NO_SSL connection fail\n", __FUNCTION__, cam_id);
			return (0);
		}
	}

	{
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_vstype[]	= "vstype=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver";

		char *s = NULL;
		char *p = NULL;
		char buf[128];

		memset(info_buf, 0x00, sizeof(cam_model_info));

		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			return 0;
		}

		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));

		sscanf(buf, "%02X%02X%02X%02X%02X%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* model name parsing */
		if (strncmp(info_buf->swver, nvs1ch_productcode, strlen(nvs1ch_productcode)) == 0) {
			strcpy(info_buf->name, nvs1ch_modelname);
		}
		else if (strncmp(info_buf->swver, nvs4ch_productcode, strlen(nvs4ch_productcode)) == 0) {
			strcpy(info_buf->name, nvs4ch_modelname);
		}
		else {
			return 0;
		}

		/* vstype parsing */
		s = sock_buf;
		p = strstr(s, f_vstype);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vstype);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));

		if (strncmp(buf, "PAL", strlen("PAL")) == 0) {
			info_buf->vstype = 1;
		}

		/* vendor parsing */
		strcpy(info_buf->vendor, "ITX");

		/* stdver parsing */
		strcpy(info_buf->stdver, info_buf->name);
	}

	info_buf->type = SYSTEM_DEVICE_NVS;
	info_buf->nvs_sub_ch = 0;

	// check. nvs subchs is overlapped? for 4ch
	if (strncmp(info_buf->name, nvs4ch_modelname, strlen(nvs4ch_modelname)) == 0) {
		int i, port_up, port_down, overlapped = 0;
		mtable *runtime = get_runtime();

		for (i = 1; i < 4; i++) {
			port_up = cam_id - i;

			if (port_up < 0)
				port_up += AVAILABLE_MAX_CH;

			if (strncmp(runtime[port_up].sys.model, nvs4ch_modelname, strlen(nvs4ch_modelname)) == 0 && runtime[port_up].sys.nvs_sub_ch == 0)
				overlapped += 1;

			port_down = cam_id + i;

			if (port_down >= AVAILABLE_MAX_CH)
				port_down %= AVAILABLE_MAX_CH;

			if (strncmp(runtime[port_down].sys.model, nvs4ch_modelname, strlen(nvs4ch_modelname)) == 0 && runtime[port_down].sys.nvs_sub_ch == 0)
				overlapped += 1;

			if (overlapped) {
				printf("[%s] nvs4ch check : nvs(port:%d) is overlapped\n", __FUNCTION__, cam_id);
				return 0;
			}
		}
	}

	// check. nvs subchs is overlapped? for 1ch
	if (strncmp(info_buf->name, nvs1ch_modelname, strlen(nvs1ch_modelname)) == 0) {
		int i, port_up, port_down, overlapped = 0;
		mtable *runtime = get_runtime();

		for (i = 1; i < 4; i++) {
			port_up = cam_id - i;

			if (port_up < 0)
				port_up += AVAILABLE_MAX_CH;

			if (strncmp(runtime[port_up].sys.model, nvs4ch_modelname, strlen(nvs4ch_modelname)) == 0 && runtime[port_up].sys.nvs_sub_ch == 0)
				overlapped += 1;

			if (overlapped) {
				printf("[%s] nvs1ch check : nvs(port:%d) is overlapped\n", __FUNCTION__, cam_id);
				return 0;
			}
		}
	}

	return 3;
}

static int nvs_get_live_status(int *video_loss_status, int *motion_status, int *alarm_status, int cam_id)
{
	const char get_model_raw[] = "api=get_live.live.status&interval=3&curr_gmttime=0";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;
	int cnt, ret;

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
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "setup_system_manage.htm", ip_str,
			strlen(get_model_raw), auth_encbuf, get_model_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	/*
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return (-1);
	}
	*/
	sock = connect_timeout_hostname(ip_str, http_port, 250000*4);
	if (sock < 0) {
		printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
		perror("socket");
		return (-1);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (-1);
		}
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	cnt = 0;
	while (1) {
		ret = send(sock, sock_buf, strlen(sock_buf), MSG_DONTWAIT);

		if (ret >= 0) {
			break;
		}

		if (cnt++ >= 5)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			perror("send");
			close(sock);
			sock = (-1);
			return (-1);
		}
		usleep(100*1000);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	cnt = 0;
	while (1) {
		ret = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_DONTWAIT|MSG_PEEK);

		if (ret > 0) {
			ret = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_DONTWAIT);
			break;
		}

		if (cnt++ >= 5)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			perror("recv");
			close(sock);
			sock = (-1);
			return (-1);
		}
		usleep(100*1000);
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "200 OK";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return (0);
		}
	}

	{
		const char f_vloss[]	= "act_vloss=";
		const char f_motion[]	= "act_motion=";
		const char f_alarm[]	= "act_alarm=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];

		int i;
		int total_ch_num = 0;

		/* vloss parsing */
		s = sock_buf;
		p = strstr(s, f_vloss);
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}
		s = p + strlen(f_vloss);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}

		total_ch_num = (int)(p - s);

		memset(buf, 0x00, sizeof(buf));
		memcpy(buf, s, total_ch_num);

		*video_loss_status = 0;

		for (i = 0; i < total_ch_num; i++) {
			if (buf[i] == '0')
				*video_loss_status |= 1 << i;
		}

		/* motion parsing */
		s = sock_buf;
		p = strstr(s, f_motion);
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}
		s = p + strlen(f_motion);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}

		total_ch_num = (int)(p - s);

		memset(buf, 0x00, sizeof(buf));
		memcpy(buf, s, total_ch_num);

		*motion_status = 0;

		for (i = 0; i < total_ch_num; i++) {
			if (buf[i] == '1')
				*motion_status |= 1 << i;
		}

		/* alarm parsing */
		s = sock_buf;
		p = strstr(s, f_alarm);
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}
		s = p + strlen(f_alarm);
		p = strstr(s, "\r\n");
		if (p == NULL)
		{
			printf("[%s] ERROR | CH(%d) Line(%d)\n", __FUNCTION__, cam_id, __LINE__);
			return 0;
		}

		total_ch_num = (int)(p - s);

		memset(buf, 0x00, sizeof(buf));
		memcpy(buf, s, total_ch_num);

		*alarm_status = 0;

		for (i = 0; i < total_ch_num; i++) {
			if (buf[i] == '1')
				*alarm_status |= 1 << i;
		}
	}

	return 1;
}

extern int nvs_video_handler_itx()
{
	int i, j, nvs_total_subch_num = 1;
	int layer, linked;
	const char nvs4ch_modelname[] = "NVS0424";
	mtable *runtime = get_runtime();
	static struct timespec handler_check_time[AVAILABLE_MAX_CH];
	static struct timespec now_time;

	memset(&handler_check_time, 0x00, sizeof(struct timespec));
	memset(&now_time, 0x00, sizeof(struct timespec));

	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		int video_loss_status = 0;
		int motion_status = 0;
		int alarm_status = 0;
		int ret = 0;

		// interval check
		clock_gettime(CLOCK_REALTIME, &now_time);
		if (handler_check_time[i].tv_sec + 1 > now_time.tv_sec) {
			return 1;
		}
		handler_check_time[i] = now_time;

		// is NVS main port?
		if (runtime[i].sys.type != SYSTEM_DEVICE_NVS || runtime[i].sys.nvs_sub_ch != 0)
			continue;

		if (strncmp(runtime[i].sys.model, nvs4ch_modelname, strlen(nvs4ch_modelname)) == 0) {
			nvs_total_subch_num = 4;
		}

		ipcam_disc_port_link_state(i, &layer, &linked);
		if (linked == 0)
			continue;

		ret = nvs_get_live_status(&video_loss_status, &motion_status, &alarm_status, i);
		//video_loss_status = 1 + 2 + 4 + 8;

		if (ret == -1)
			continue;

		for (j = 0; j < nvs_total_subch_num; j++) {
			int port_num = (i + j) % AVAILABLE_MAX_CH;
			char ip_buf[16];
			unsigned int ipx_vloss_status = 0;

			get_ipstr(runtime[i].sys.ipaddr, ip_buf);
			ipx_vloss_status = get_vloss_status();

			// no video loss
			if (video_loss_status & (1 << j)) {
				// Check : this port is linked?
				ipcam_disc_port_link_state(port_num, &layer, &linked);
				if (linked && j != 0) {
					printf("[%s] Port(%d) is physical linked.\n", __FUNCTION__, port_num);
					continue;
				}

				if (runtime[port_num].sys.nvs_stream_start == 0) {
					memcpy(&runtime[port_num], &runtime[i], sizeof(mtable));
					snprintf(runtime[port_num].sys.rtsp_url[0], 256, "rtsp://%s/live/main%d", ip_buf, j+1);
					runtime[port_num].sys.nvs_sub_ch = j;

					nvs_open_stream(port_num);
				}

				if (!(ipx_vloss_status & 1<<port_num)) {
					if (motion_status & 1<<j)
						runtime[port_num].motion_flag = 90;

					if (alarm_status & 1<<j)
						runtime[port_num].alarm_flag = 1;
				}
			}
			// video loss
			else {
				// Check : this port is linked?
				ipcam_disc_port_link_state(port_num, &layer, &linked);
				if (linked && j != 0) {
					printf("[%s] Port(%d) is physical linked.\n", __FUNCTION__, port_num);
					continue;
				}

				if (runtime[port_num].sys.type == SYSTEM_DEVICE_NVS && runtime[port_num].sys.nvs_stream_start == 1) {
					runtime[port_num].sys.nvs_stream_start = 0;
					printf("[%s] nvs_close_stream(ch:%d)\n", __FUNCTION__, i);
					nvs_close_stream(port_num);
				}
			}
		}
	}

	return 1;
}

extern int nvs_close_all_sub_ch(int port_num)
{
	int i;

	dtable *discovery = get_dtable();
	mtable *runtime = get_runtime();
	GAsyncQueue *queue = get_queue();

	printf("[%s] start port(%d)\n", __FUNCTION__, port_num);

	while (queue == NULL) {
		usleep(500*1000);
		queue = get_queue();
	}


	if (runtime[port_num].sys.type != SYSTEM_DEVICE_NVS || runtime[port_num].sys.nvs_sub_ch != 0) {
		printf("[%s] Not nvs main port\n", __FUNCTION__);
		printf("[%s] end\n", __FUNCTION__);
		return 1;
	}

	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		printf("[%s] port(%d) subch(%d) type(%d) ipaddr(%d)\n", __FUNCTION__, i, runtime[i].sys.nvs_sub_ch, runtime[i].sys.type, runtime[port_num].sys.ipaddr);

		if (i == port_num)
			continue;

		if (runtime[port_num].sys.ipaddr == runtime[i].sys.ipaddr &&
			runtime[i].sys.type == SYSTEM_DEVICE_NVS &&
			runtime[i].sys.nvs_sub_ch > 0)
		{
			nf_pnd_queue_push(i, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
			memset(&discovery[i], 0x00, sizeof(dtable));
			runtime[i].sys.type == SYSTEM_DEVICE_NR;
			nf_pnd_evt_notify_fire(i, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
			printf("[%s] close nvs sub channel(%d)\n", __FUNCTION__, i);
		}
	}

	/*
	nf_pnd_queue_push(port_num, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
	*/
	memset(&discovery[port_num], 0x00, sizeof(dtable));
	runtime[port_num].sys.type == SYSTEM_DEVICE_NR;
	nf_pnd_evt_notify_fire(port_num, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);

	printf("[%s] end\n", __FUNCTION__);

	return 1;
}

extern void nvs_subch_check_physical_link_on(int target_port)
{
	mtable *runtime = get_runtime();
	dtable *discovery = get_dtable();
	GAsyncQueue *queue = get_queue();

	while (queue == NULL) {
		usleep(500*1000);
		queue = get_queue();
	}

	if (runtime[target_port].sys.type == SYSTEM_DEVICE_NVS && runtime[target_port].sys.nvs_sub_ch > 0) {
		nf_pnd_queue_push(target_port, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
		memset(&discovery[target_port], 0x00, sizeof(dtable));
		printf("[%s] close nvs sub channel(%d)\n", __FUNCTION__, target_port);
	}
}

extern void nvs_open_stream(int target_port)
{
	int layer, linked;
	mtable *runtime = get_runtime();
	GAsyncQueue *queue = get_queue();

	printf("[%s] start ch(%d)\n", __FUNCTION__, target_port);
	while (queue == NULL) {
		usleep(500*1000);
		queue = get_queue();
	}

	// Check : this port is linked?
	ipcam_disc_port_link_state(target_port, &layer, &linked);
	if (linked && runtime[target_port].sys.nvs_sub_ch != 0) {
		printf("[%s] Port(%d) is already used.\n", __FUNCTION__, target_port);
		printf("[%s] end subch(%d)\n", __FUNCTION__, target_port);
		return;
	}

	runtime[target_port].state = (MGMT_STATE_LINKED|MGMT_STATE_READY);
	runtime[target_port].sys.nvs_stream_start = 1;
	nf_pnd_queue_push(target_port, IPCAM_EVENT_NVS_SUBCH_READY, __LINE__, __FILE__);

	// FIXME : waiting for nvs initialize temporarily.
	nf_ipcam_waiting_settime(target_port, 3*1000);

	printf("[%s] end ch(%d)\n", __FUNCTION__, target_port);
}

extern void nvs_close_stream(int target_port)
{
	int layer, linked;
	mtable *runtime = get_runtime();
	GAsyncQueue *queue = get_queue();

	printf("[%s] start ch(%d)\n", __FUNCTION__, target_port);
	while (queue == NULL) {
		usleep(500*1000);
		queue = get_queue();
	}

	// Check : this port is linked?
	ipcam_disc_port_link_state(target_port, &layer, &linked);
	if (linked && runtime[target_port].sys.nvs_sub_ch != 0) {
		printf("[%s] Port(%d) is already used.\n", __FUNCTION__, target_port);
		printf("[%s] end subch(%d)\n", __FUNCTION__, target_port);
		return;
	}

	nf_pnd_queue_push(target_port, IPCAM_EVENT_NVS_SUBCH_CLOSE, __LINE__, __FILE__);

	// FIXME : waiting for nvs initialize temporarily.
	nf_ipcam_waiting_settime(target_port, 0);

	printf("[%s] end ch(%d)\n", __FUNCTION__, target_port);
}

static int nvs_ptz_control(NVS_PTZ_CONTROL_TYPE_E control_type, int command_type, int cam_id)
{
	int ptz_cmd_nvs_idx, param1 = 0;
	const char http_api_raw[] =
			"api=set_live.live.ptz_continue&channel=%d&cmd=%d&param1=%d&param2=null";

	const int ptz_cmd_nvs[] = {
		0, 1, 2, 3, 4,
		5, 6, 7, 8, 9,
		10, 11, 12, 13, 14,
		15, 16, 17, -1, -1,
		-1, 18, 19, 20, 21,
		22, 23
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

	mtable *runtime = get_runtime();

	printf("[%s] start ch(%d)(%d:%d)\n", __FUNCTION__, cam_id, control_type, command_type);
	ptz_cmd_nvs_idx = nvs_ptz_check_control_cmd_type(control_type, command_type, cam_id, &param1);
	if (ptz_cmd_nvs_idx == -1)
		return IPCAM_SETUP_RTN_FAILED;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	if (NVS_PTZ_CMD_PRESET_SET <= control_type && control_type <= NVS_PTZ_CMD_SET_AUTO_IRIS) {
		snprintf(http_api, 256, http_api_raw, runtime[cam_id].sys.nvs_sub_ch,
				ptz_cmd_nvs[ptz_cmd_nvs_idx], command_type);
	}
	else {
		snprintf(http_api, 256, http_api_raw, runtime[cam_id].sys.nvs_sub_ch,
				ptz_cmd_nvs[ptz_cmd_nvs_idx], param1);
	}
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "live.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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

	printf("[%s] end ch(%d)(%d:%d)\n", __FUNCTION__, cam_id, control_type, command_type);
	return IPCAM_SETUP_RTN_DONE;
}

static int nvs_ptz_check_control_cmd_type(NVS_PTZ_CONTROL_TYPE_E control_type, int command_type, int cam_id, int *param1)
{
	mtable *runtime = get_runtime();
	GValue ret_value = {0,};
	int key_value = 0;

	if (control_type == NVS_PTZ_CMD_PT) {
		switch(command_type) {
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
				printf("[%s] Non-PT command\n", __FUNCTION__);
				return -1;
		}

		if (nf_sysdb_get_key1("cam.ptz.P%d.pt_spd", cam_id,  &ret_value, NULL)) {
			key_value = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
			*param1 = key_value;
			return command_type;
		}
	}
	else if (control_type == NVS_PTZ_CMD_ZOOM) {
		switch(command_type) {
			case NF_PTZ_CMD_ZOOM_WIDE:
			case NF_PTZ_CMD_ZOOM_TELE:
			case NF_PTZ_CMD_STOP:
				break;
			default:
				printf("[%s] Non-Zoom command\n", __FUNCTION__);
				return -1;
		}
		if (nf_sysdb_get_key1("cam.ptz.P%d.zoom_spd", cam_id,  &ret_value, NULL)) {
			key_value = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
			*param1 = key_value;
			return command_type;
		}
	}
	else if (control_type == NVS_PTZ_CMD_FOCUS) {
		switch(command_type) {
			case NF_PTZ_CMD_FOCUS_NEAR:
			case NF_PTZ_CMD_FOCUS_FAR:
			case NF_PTZ_CMD_STOP:
				break;
			default:
				printf("[%s] Non-Focus command\n", __FUNCTION__);
				return -1;
		}
		if (nf_sysdb_get_key1("cam.ptz.P%d.focus_spd", cam_id,  &ret_value, NULL)) {
			key_value = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
			*param1 = key_value;
			return command_type;
		}
	}
	else if (control_type == NVS_PTZ_CMD_IRIS) {
		switch(command_type) {
			case NF_PTZ_CMD_IRIS_OPEN:
				printf("[%s] NPT iris open\n", __FUNCTION__);
				break;
			case NF_PTZ_CMD_IRIS_CLOSE:
				printf("[%s] NPT iris close\n", __FUNCTION__);
				break;
			case NF_PTZ_CMD_STOP:
				break;
			default:
				printf("[%s] Non-PT command\n", __FUNCTION__);
				return -1;
		}
		if (nf_sysdb_get_key1("cam.ptz.P%d.iris_spd", cam_id,  &ret_value, NULL)) {
			key_value = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
			key_value = (key_value / 10) == 0 ? 1 : key_value / 10;
			*param1 = key_value;
			return command_type;
		}
	}
	else if (control_type == NVS_PTZ_CMD_PTZ_STOP) {
		switch(command_type) {
			case PTZ_SETUP_PAN:
			case PTZ_SETUP_TILT:
			case PTZ_SETUP_ZOOM:
			case PTZ_SETUP_FOCUS:
			case PTZ_SETUP_IRIS:
				break;
			default:
				printf("[%s] Non-PT command\n", __FUNCTION__);
				return -1;
		}
		return NF_PTZ_CMD_STOP;
	}
	else if (control_type == NVS_PTZ_CMD_PRESET_SET) {
		return NF_PTZ_CMD_SET_PRESET;
	}
	else if (control_type == NVS_PTZ_CMD_PRESET_CLEAR) {
		return NF_PTZ_CMD_CLEAR_PRESET;
	}
	else if (control_type == NVS_PTZ_CMD_PRESET_GO) {
		return NF_PTZ_CMD_GOTO_PRESET;
	}
	else if (control_type == NVS_PTZ_CMD_SET_AUTO_FOCUS) {
		return NF_PTZ_CMD_SET_AUTO_FOCUS;
	}
	else if (control_type == NVS_PTZ_CMD_SET_AUTO_IRIS) {
		return NF_PTZ_CMD_SET_AUTO_IRIS;
	}

	return -1;
}

extern int nvs_set_pt(ptz_info *info, int cam_id)
{
	if(info != NULL)
		return nvs_ptz_control(NVS_PTZ_CMD_PT, info->cmd, cam_id);
	else
		return -1;
}
extern int nvs_set_zoom(ptz_info *info, int cam_id)
{
	if(info != NULL)
		return nvs_ptz_control(NVS_PTZ_CMD_ZOOM, info->cmd, cam_id);
	else
		return -1;
}
extern int nvs_set_focus(ptz_info *info, int cam_id)
{
	if(info != NULL)
		return nvs_ptz_control(NVS_PTZ_CMD_FOCUS, info->cmd, cam_id);
	else
		return -1;
}
extern int nvs_set_iris(NF_PTZ_CMD_E ptz_cmd, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_IRIS, ptz_cmd, cam_id);
}
extern int nvs_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_PTZ_STOP, e, cam_id);
}
extern int nvs_set_preset(int num, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_PRESET_SET, num, cam_id);
}
extern int nvs_clear_preset(int num, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_PRESET_CLEAR, num, cam_id);
}
extern int nvs_go_preset(int num, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_PRESET_GO, num, cam_id);
}
extern int nvs_set_autofocus_mode(int num, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_SET_AUTO_FOCUS, num, cam_id);
}
extern int nvs_set_autoiris_mode(int num, int cam_id)
{
	return nvs_ptz_control(NVS_PTZ_CMD_SET_AUTO_IRIS, num, cam_id);
}
extern int nvs_get_af_capa(cam_info *info, int cam_id)
{
	memset(info, 0x00, sizeof(cam_info));
	info->afcapa.mfz = 1;
	info->afcapa.iris = 1;
	info->afcapa.zoom_min = 0;
	info->afcapa.zoom_max = 500;
	info->afcapa.focus_min = 0;
	info->afcapa.focus_max = 500;
	info->afcapa.iris_min = 0;
	info->afcapa.iris_max = 500;
	return IPCAM_SETUP_RTN_DONE;
}
extern int nvs_set_vcodec(cam_info* info_set, int cam_id)
{
	char size, fps, quality;
	const char http_api_raw[] =
			"api=set_setup.encode.mainstream_ch&ch=%d&size=%c&fps=%c&quality=%c&audio=0";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	printf("[%s] start ch(%d)\n", __FUNCTION__, cam_id);

	size = 'D'; // NTSC/PAL D1

	// A(30), B(15), C(7), D(3), E(2), F(1), G(0)
	switch (info_set->vcodec.fps[0]) {
	case NF_IPCAM_FPS_300:
	case NF_IPCAM_FPS_250:
		fps = 'A';
		break;
	case NF_IPCAM_FPS_150:
	case NF_IPCAM_FPS_120:
		fps = 'B';
		break;
		/*
	case NF_IPCAM_FPS_70:
	case NF_IPCAM_FPS_60:
		fps = 'C';
		break;
	case NF_IPCAM_FPS_20:
		fps = 'E';
		break;
		*/
	case NF_IPCAM_FPS_10:
		fps = 'F';
		break;
	default:
		return IPCAM_SETUP_RTN_FAILED;
	}

	switch(info_set->vcodec.bitrate[0]) {
	case 2000:
		quality = 'A';
		break;
	case 1700:
		quality = 'B';
		break;
	case 1400:
		quality = 'C';
		break;
	case 1100:
	case 800:
		quality = 'D';
		break;
	default:
		quality = 'A';
		break;
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, runtime[cam_id].sys.nvs_sub_ch,
			size, fps, quality);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "live.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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

	printf("[%s] end ch(%d) size(%c) fps(%c) quality(%c)\n", __FUNCTION__, cam_id, size, fps, quality);
	return IPCAM_SETUP_RTN_DONE;
}

extern int nvs_set_image(image_info* info_set, int cam_id)
{
	char size, fps, quality;
	const char http_api_raw[] =
			"api=set_setup.camera.color_ch&ch=%d&brightness=%d&contrast=%d&tint=%d&color=%d";
	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	printf("[%s] start ch(%d)\n", __FUNCTION__, cam_id);
	printf("[%s] ch(%d) brightness(%d) contrast(%d), color(%d) tint(%d)\n", __FUNCTION__, cam_id,
			info_set->brightness,info_set->contrast,info_set->color,info_set->tint);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, runtime[cam_id].sys.nvs_sub_ch,
			info_set->brightness,info_set->contrast,info_set->color,info_set->tint);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "live.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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

	printf("[%s] end ch(%d)\n", __FUNCTION__, cam_id);
	return IPCAM_SETUP_RTN_DONE;
}

extern int nvs_set_init(int cam_id)
{
	GTimeVal tv;
	GDateTime *dt;
	int year, month, day, hour, min, sec;
	guint timezone, dst;
	const char http_api_raw[] =
			"api=set_setup.system.datetime&"
			"curr_year=%d&curr_month=%d&curr_day=%d&curr_hour=%d&curr_minute=%d&curr_second=%d&"
			"dataformat=0&timeformat=0&timezone=%d&dst=%d&nettimesevsetup=POOL.NTP.ORG&auto_interval_min=0";

	char http_api[256];
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	g_get_current_time(&tv);
	dt		= g_date_time_new_from_timeval_local (&tv);
	year	= g_date_time_get_year(dt);
    month	= g_date_time_get_month(dt);
    day		= g_date_time_get_day_of_month(dt);
    hour	= g_date_time_get_hour(dt);
    min		= g_date_time_get_minute(dt);
    sec		= g_date_time_get_second(dt);
	timezone	= nf_sysdb_get_uint("sys.date.tz_index");
	dst			= nf_sysdb_get_bool("sys.date.daylight");
    g_date_time_unref(dt);

	printf("[%s] start ch(%d)\n", __FUNCTION__, cam_id);
	printf("[%s] ch(%d) year(%d) month(%d), day(%d)\n", __FUNCTION__, cam_id,
			year, month, day);
	printf("[%s] ch(%d) hour(%d) min(%d), sec(%d)\n", __FUNCTION__, cam_id,
			hour, min, sec);
	printf("[%s] ch(%d) timezone(%d) dst(%d)\n", __FUNCTION__, cam_id,
			timezone, dst);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(http_api, 256, http_api_raw, year, month, day, hour, min, sec, timezone, dst);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, "live.htm", ip_str,
			strlen(http_api), auth_encbuf, http_api);

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
	printf("\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock);
	return IPCAM_SETUP_RTN_DONE;
}
#endif //__NF_NVS_DRIVER_ITX_C__
