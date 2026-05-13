/**
 * @file nf_ipcam_driver_itx.c
 * @brief ITX IP카메??driver.
 * @author Jae-young Kim
 * @date 2010-12-08
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */

#ifndef __NF_IPCAM_DRIVER_ITX_C__
#define __NF_IPCAM_DRIVER_ITX_C__

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
#include <nf_util_device.h>

#include <openssl/ssl.h>
#include "nf_ipcam_driver_itx_md5.h"

#include "nf_ipcam_utils.h"
#include "nf_api_http.h"

//captainnn
#include "ivca_def.h"
#include "nfdal.h"

#ifndef min
#define min(a, b) (a) > (b) ? (b) : (a)
#endif

#ifndef max
#define max(a, b) (a) > (b) ? (a) : (b)
#endif


#define WAIT_REPLY_SECS		(2)
#define PRINT_HTTP_API_SEND	(0)
#define SOCK_BUF_LENGTH			(2048)
#define SOCK_BUF_RECV_LENGTH	(2047)


#define	CURL_TIMEOUT	(4)

typedef enum
{
	ITX_RTN_OK = 0,
	ITX_RTN_FAIL,
	ITX_RTN_MAX
} ITX_RTN_TYPE;

//extern void nf_ipcam_waiting_settime(int port, int msec);

static void PRINT_SSL_ERROR(int ssl_err, int cam_id);
/** @var _last_http_api
 *  @brief Job??형별로 직전 ??송??도????문 string. digest??에 ??활????도?????용.
 */
static char *_last_http_api[AVAILABLE_MAX_CH][NF_IPCAM_TYPE_MAX];


static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);
static char* get_resol_string(uint64_t resol_code);
static char* get_fps_string(unsigned int fps);
static char* get_gop_string(unsigned int fps_code);
static char* get_bitctrl_string(unsigned int bitctrl_code);
static char* get_vcodec_string(unsigned int vcodec_code);
static int get_index_from_bitmask(unsigned int value);
static void _set_last_api_str(int cam_id, int type, char* str);

static int _cam_setup_send_plain(int cam_id, int type, char *buf);
static int _cam_setup_send_ssl(int cam_id, int type, char *buf);
static int _ssl_digest_setup_again(int cam_id, int type, char* rbuf);
static int _cam_digest_setup_again(int cam_id, int type, char* rbuf);

/* SSL encrypted API */
static int _ssl_get_model_info_raw(cam_model_info*, unsigned int, unsigned short, char*, char*);
static int _ssl_get_model_info_by_sysdb(cam_model_info*, int);
static int _ssl_get_model_info_by_backdoor(cam_model_info*, int);

static int _ssl_set_osd_off(int cam_id);
static int _common_set_osd_off(int cam_id);

static int _ssl_reboot(int cam_id);
static int _common_reboot(int cam_id);

static int _ssl_set_vcodec(cam_info* info_set, int cam_id);
static int _ti368_set_vcodec(cam_info* info_set, int cam_id);

static int _ssl_set_acodec(cam_info* info_set, int cam_id);
static int _common_set_acodec(cam_info* info_set, int cam_id);

static int _ssl_set_image(image_info* info_set, int cam_id);
static int _ti368_set_image(image_info* info_set, int cam_id);

static int _ssl_set_alarm(cam_info* info_set, int cam_id);
static int _common_set_alarm(cam_info* info_set, int cam_id);

static int _ssl_get_afcapa(cam_info* info, int cam_id);
static int _common_get_afcapa(cam_info* info, int cam_id);

static int _ssl_set_origin(int cam_id);
static int _common_set_origin(int cam_id);

static int _ssl_set_zoom(ptz_info *info, int cam_id);
static int _common_set_zoom(ptz_info *info, int cam_id);

static int _ssl_set_focus(ptz_info *info, int cam_id);
static int _common_set_focus(ptz_info *info, int cam_id);

static int _ssl_set_piris(int value, int cam_id);
static int _common_set_piris(int value, int cam_id);

static int _ssl_set_onepush(int cam_id);
static int _common_set_onepush(int cam_id);

static int _ssl_set_dc_iris_cal(int cam_id);
static int _common_set_dc_iris_cal(int cam_id);
static int _ssl_get_zoom(int *value, int cam_id);
static int _common_get_zoom(int *value, int cam_id);

static int _ssl_get_focus(int *value, int cam_id);
static int _common_get_focus(int *value, int cam_id);

static int _ssl_get_piris(int *value, int cam_id);
static int _common_get_piris(int *value, int cam_id);

static int _ssl_set_ptz_stop(PTZ_FUNCS_E e, int cam_id);
static int _common_set_ptz_stop(PTZ_FUNCS_E e, int cam_id);

static int _ssl_factory_default(int cam_id);
static int _ti368_factory_default(int cam_id);

static int __ssl_factory_default(int cam_id);
static int __ti368_factory_default(int cam_id);

static int _ssl_get_image(image_info* info_set, int cam_id);
static int _common_get_image(image_info* info_set, int cam_id);


static int _ssl_set_dhcpon(int cam_id);
static int _ti368_set_dhcpon(int cam_id);
static int _ssl_sn_get(struct sn_info *info, int cam_id, int v);


static int _common_reboot_fwmode(int cam_id);
static int _common_set_fwmode(int cam_id);
static int _common_befo_fw_up(int cam_id);
static int _common_upload_fw_hisilicon(int cam_id, char *file_nm, char *file_stream, int file_len);
static int _common_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len);
static int _ssl_reboot_fwmode(int cam_id);
static int _ssl_set_fwmode(int cam_id);
static int _ssl_befo_fw_up(int cam_id);
static int _ssl_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len);
static int _ssl_upload_fw_hisilicon(int cam_id, char *file_nm, char *file_stream, int file_len);

//static int _common_reboot_fwmode_digest(int cam_id);
//static int _common_set_fwmode_digest(int cam_id);
//static int _common_befo_fw_up_digest(int cam_id);
//static int _common_upload_fw_digest(int cam_id, char *file_nm, char *file_stream, int file_len);

static int _common_factory_mode(int cam_id);
static int _common_set_passwd(int cam_id);

static int _ssl_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id);
static int _ti368_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id);

static guint _get_vca_prop_data_all(VCAPropData *data, guint ch_num);
static guint _get_vca_opt_data(ivca_option_t*option, guint channel);
static guint _get_vca_rule_data(ivca_rule_t*rule, guint channel);

static int _ssl_set_va_config(ivca_rule_t* value, int cam_id);
static int _common_set_va_config(ivca_rule_t* value, int cam_id);
static int _itx_call_api_upload(int cam_id, const char *path, const char *query, icm_http_upload *upload, int timeout);
static int _curl_set_va_config_gzip(char *rule_str, size_t str_len, int cam_id);

static int _curl_set_va_config(ivca_rule_t* value, int cam_id);

static int _ssl_set_reset_va(int cam_id);
static int _common_set_reset_va(int cam_id);

static int _ssl_set_timezone_info(int ch);
static int _cam_set_timezone_info(int ch);

static int _ssl_set_roi_area(NFIPCamSetupROIArea *roi_info, int ch);
static int _cam_set_roi_area(NFIPCamSetupROIArea *roi_info, int ch);

static int itx_cam_get_timezone_info(int ch, char* buf);
static int itx_cam_get_current_info(int ch, int* current);

static int sn_parsing_to_info(char *string, struct sn_info *info);
static int _ti386_sn_get(struct sn_info *info, int cam_id, int v);

static int _ssl_set_dnn_adjust_d2n(image_info* info_set, int cam_id);
static int _ssl_set_dnn_adjust_n2d(image_info* info_set, int cam_id);
static int _ti368_set_dnn_adjust_d2n(image_info* info_set, int cam_id);
static int _ti368_set_dnn_adjust_n2d(image_info* info_set, int cam_id);

static int _ti368_set_focus_compensation(focus_comp_info* info, int cam_id);
static int  _ssl_set_focus_compensation(focus_comp_info* info, int cam_id);

static int _ti368_get_focus_compensation(focus_comp_info* info, int cam_id);
static int  _ssl_get_focus_compensation(focus_comp_info* info, int cam_id);

static int _ti368_set_corridor_mode(int cam_id, int corridor_mode);
static int _ssl_set_corridor_mode(int cam_id, int corridor_mode);

static int _ai_color_convertor(char* color);
static char* _ai_color_convertor_reverse(int color);

extern int itx_cam_get_ai_rule_engine_and_save_db(int ch);
extern void itx_cam_set_ai_rule_engine(void *arg);

static int _get_cam_dl_option(int ch);
static void _save_dl_option_in_db(int ch);
extern int itx_cam_get_ai_dl_option_and_save_db(int ch);
extern void itx_cam_set_ai_dl_option(int ch);

static int _get_cam_embedded_osd(int ch);
static void _save_embedded_osd_info_in_db(int ch);
extern int itx_cam_get_embedded_osd_and_save_db(int ch);

extern void itx_cam_set_embedded_osd(int ch);
static char* _e_osd_convert_int_to_str(int color);


// SWIPXXP-790 Exception 
static int _is_low_firmware_novatek_cam(char* firmware)
{
	// firmware standard list
	// NOVATEK 1,2 : 89100(90100).1.282.100 under
	// Clair-N 2nd : 89100.1.1023.100 under
	// Sens-N 2nd : 90100.1.1023.100 under
	int CAM_TYPE_CLAIR = 89100;
    int CAM_TYPE_SENS = 90100;
	int CAM_VERSION_NOVATEK_1ST = 282; // 89100.1.0282
	int CAM_VERSION_NOVATEK_2ND = 282; // 90100.1.0282
    int CAM_VERSION_CLAIR_2ND = 1023;
    int CAM_VERSION_SENS_2ND = 1023;
	
	int ret=0;

	// cam firmware parsing
	int cam_f_version = 0;
    int cam_f_type = 0;
	char *token, *save_ptr;

	token = strtok_r(firmware, ".", &save_ptr);
	if(token == NULL)
	{
		printf("[khkh] [%s:%d] cam firmware version error\n", 
			__func__, __LINE__);
		return 0;
	}
	cam_f_type = atoi(token);

	token = strtok_r(save_ptr, ".", &save_ptr); // '1'
	token = strtok_r(save_ptr, ".", &save_ptr); // '1022'
	if(token == NULL)
	{
		printf("[khkh] [%s:%d] cam firmware version error\n", 
			__func__, __LINE__);
		return 0;
	}
	cam_f_version = atoi(token);


	// firmware type check
	if(cam_f_type == CAM_TYPE_CLAIR) // 89100
	{
		if(cam_f_version < 1000) // NOVATEK
		{
			if(cam_f_version < CAM_VERSION_NOVATEK_1ST)
				ret = 1;
		}
		else
		{
			if(cam_f_version < CAM_VERSION_CLAIR_2ND)
				ret = 1;
		}
	}
	else if(cam_f_type == CAM_TYPE_SENS) // 90100
	{
		if(cam_f_version < 1000) // NOVATEK
		{
			if(cam_f_version < CAM_VERSION_NOVATEK_2ND)
				ret = 1;
		}
		else
		{
			if(cam_f_version < CAM_VERSION_SENS_2ND)
				ret = 1;
		}
	}

	return ret;
}

/**
 * @brief ??문 ??사??static변??초기??
 */
extern void init_remember_buf(void)
{
	int i = 0, j = 0;
	for (i=0; i< AVAILABLE_MAX_CH; i++)
	{
		for (j=0; j<NF_IPCAM_TYPE_MAX; j++)
		{
			_last_http_api[i][j] = NULL;
		}
	}
}

static const char *str_null_to_blank(const char *str)
{
	static const char blank[2] = {0,};
	if(str) return str;
	return blank;
}

/**
 * @brief ??신????문??parsing??여 결과 처리??다.
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param recv_buf ??신????문.
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
extern int itx_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
{
	mtable *runtime = get_runtime();

	if (runtime == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) TYPE(%d) - %s\n", cam_id, type, recv_buf);
#endif


	switch(type)
	{
		case NF_IPCAM_TYPE_SET_ORIGIN:
		{
			const char f_focus[] = "focus=";
			const char f_zoom[] = "zoom=";
			char *s, *p;
			char buf[16];

			/* zoom parsing */
			memset(buf, 0x00, 16);
			s = recv_buf;
			p = strstr(s, f_zoom);
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			s = p + strlen(f_zoom);
			p = strstr(s, "&");
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			memset(buf, 0x00, 16);
			memcpy(buf, s, (size_t)(p - s));
			runtime[cam_id].ptz.zoom.value = atoi(buf);
			IPCAM_DBG(MINOR, "Zoom changed CH(%d) - %d\n", cam_id, runtime[cam_id].ptz.zoom.value);

			/* focus parsing */
			memset(buf, 0x00, 16);
			s = recv_buf;
			p = strstr(s, f_focus);
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			s = p + strlen(f_focus);
			p = strstr(s, "&");
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			memset(buf, 0x00, 16);
			memcpy(buf, s, (size_t)(p - s));
			runtime[cam_id].ptz.focus.value = atoi(buf);
			IPCAM_DBG(MINOR, "Focus changed CH(%d) - %d\n", cam_id, runtime[cam_id].ptz.focus.value);
			break;
		}
		case NF_IPCAM_TYPE_SET_ONESHOT:
		case NF_IPCAM_TYPE_SET_ZOOM:
		{
			const char f_focus[] = "focus=";
			const char f_zoom[] = "zoom=";
			char *s, *p;
			char buf[16];

			/* focus parsing */
			memset(buf, 0x00, 16);
			s = recv_buf;
			p = strstr(s, f_focus);
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			s = p + strlen(f_focus);
			p = strstr(s, "&");
			if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
			memset(buf, 0x00, 16);
			memcpy(buf, s, (size_t)(p - s));
			runtime[cam_id].ptz.focus.value = atoi(buf);
			IPCAM_DBG(MINOR, "Focus changed CH(%d) - %d\n", cam_id, runtime[cam_id].ptz.focus.value);
			break;
		}
		case NF_IPCAM_TYPE_SET_STOP:
			//IPCAM_DBG(MINOR, "Current Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

			if(runtime[cam_id].ptz.moving & (PTZ_SETUP_PAN|PTZ_SETUP_TILT))
			{
				runtime[cam_id].ptz.moving &= ~(PTZ_SETUP_PAN|PTZ_SETUP_TILT);
			}
			if (runtime[cam_id].ptz.moving & PTZ_SETUP_ZOOM)
			{
				runtime[cam_id].ptz.moving &= ~PTZ_SETUP_ZOOM;
			}
			if (runtime[cam_id].ptz.moving & PTZ_SETUP_FOCUS)
			{
				runtime[cam_id].ptz.moving &= ~PTZ_SETUP_FOCUS;
			}
			if (runtime[cam_id].ptz.moving & PTZ_SETUP_IRIS)
			{
				runtime[cam_id].ptz.moving &= ~PTZ_SETUP_IRIS;
			}

			//IPCAM_DBG(MINOR, "Stop Moving CH(%d) - %08x\n", cam_id, runtime[cam_id].ptz.moving);

			break;
		default:
			break;
	}

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief Basic??증 ??패??Digest??증????도??다.
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param rbuf Basic??증????신????문.
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
extern int itx_digest_setup_again(int cam_id, int type, char* rbuf)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_digest_setup_again(cam_id, type, rbuf);
	}
	else
	{
		rtn = _cam_digest_setup_again(cam_id, type, rbuf);
	}

	return rtn;
}

extern int cam_set_osd_off(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_osd_off(cam_id);
	}
	else
	{
		rtn = _common_set_osd_off(cam_id);
	}

	return rtn;
}

extern int _cam_factory_default(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = __ssl_factory_default(cam_id);
	}
	else
	{
		rtn = __ti368_factory_default(cam_id);
	}

	return rtn;
}

extern int cam_set_dhcpon(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_dhcpon(cam_id);
	}
	else
	{
		rtn = _ti368_set_dhcpon(cam_id);
	}

	return rtn;
}


extern int cam_reboot(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_reboot(cam_id);
	}
	else
	{
		rtn = _common_reboot(cam_id);
	}

	return rtn;
}

extern int cam_set_dnn_adjust_d2n(image_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_dnn_adjust_d2n(info_set, cam_id);
	}
	else
	{
		rtn = _ti368_set_dnn_adjust_d2n(info_set, cam_id);
	}

	return rtn;
}

extern int cam_get_dnn_adjust_d2n(image_info* info_set, int cam_id)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i;

	if(cam_id < 0)
	{
		printf("[%s:%d] argument error cam_id[%d] \n", __func__,__LINE__, cam_id);
		goto endl;
	}

	http_init(&ctx);

	// http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.video.dnn_adjust");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){// CURLE_OPERATION_TIMEDOUT : 28
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error [%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	ret = 0;

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		//printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		
		if(strstr(key,"d2n")){
			info_set->dnn_sense_dton = atoi(value);
		}else if(strstr(key,"n2d")){
			info_set->dnn_sense_ntod = atoi(value);
		}
		release_token(&iter_data);
	}
	release_token(&iter);

endl:
	http_release(&ctx);
	return ret;
}

extern int cam_set_dnn_adjust_n2d(image_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_dnn_adjust_n2d(info_set, cam_id);
	}
	else
	{
		rtn = _ti368_set_dnn_adjust_n2d(info_set, cam_id);
	}

	return rtn;
}

extern int ti368_factory_default(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_factory_default(cam_id);
	}
	else
	{
		rtn = _ti368_factory_default(cam_id);
	}

	return rtn;
}

extern int cam_set_vcodec(cam_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();
	
	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_vcodec(info_set, cam_id);
	}
	else
	{
		rtn = _ti368_set_vcodec(info_set, cam_id);
	}

	return rtn;
}

extern int cam_set_acodec(cam_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_acodec(info_set, cam_id);
	}
	else
	{
		rtn = _common_set_acodec(info_set, cam_id);
	}

	return rtn;
}

extern int cam_set_image_nmx(image_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_image(info_set, cam_id);
	}
	else
	{
		rtn = _ti368_set_image(info_set, cam_id);
	}

	return rtn;
}

extern int cam_get_image(image_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_get_image(info_set, cam_id);
	}
	else
	{
		rtn = _common_get_image(info_set, cam_id);
	}

	return rtn;
}

#define SET_VALUE_FROM_STRFORMAT(x, value, str_format, support)\
	{\
		int __i;\
		for(__i = 0; __i < (sizeof((str_format))/sizeof(char *)); __i++){\
			if(strlen((str_format)[__i]) <= 0) continue;\
			if(strcmp((value),(str_format)[__i]) == 0 && ((1<<__i) & (support))){\
				(x) = __i;\
				break;\
			}\
		}\
	}

extern int cam_get_image_x(image_info* info_set, int cam_id)
{
	//check
	const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto","auto_m", "auto", "manual", "auto", "auto_m", "manual", "","","","","auto_m", "auto", "manual", "auto_m"};
	//check
	const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
	const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	const char *dnn_mode_str[] = { "auto", "day", "night", "schedule", "", "", "","auto", "day", "night", "auto", "day", "night", "schedule", "auto", "on", "off", "external" };
	const char *dnn_det_time[] = 
		{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
	const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
	const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
	// MAX Shutter Str modify
	const char *max_shutter_str[] = { "shut_0", "shut_1", "shut_2", "shut_3", "shut_4", "shut_5", "shut_6", "shut_7", "shut_8", "shut_9", "shut_10", "shut_11", "shut_12", "shut_13", "shut_14", "shut_15", "shut_16" };
	const char *base_shutter_str[] = { "bshut_0", "bshut_1", "bshut_2", "bshut_3", "bshut_4", "bshut_5", "bshut_6", "bshut_7", "bshut_8", "bshut_9", 
													"bshut_10", "bshut_11", "bshut_12", "bshut_13", "bshut_14", "bshut_15", "bshut_16"};
	const char *ff_mode_str[] = { "60", "50", "off" };
	const char *slow_shutter_str[] = {"off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8"};
	const char *wdr_mode_str[] = { "off", "", "", "off", "on", "low", "middle", "high", "low", "mid", "high", "on", "low", "mid", "high", "on", "low", "middle", "high", "low", "low", "mid", "high", "on", "off", "on", "low", "mid", "high" };
						
	const char *blc_mode_str[] = { "off", "on", "", "adap", "zone" };
	const char *blc_zone_str[] = { "", "", "", "", "lower", "middle", "upper", "left", "right" };

	// 3D DNR
	const char *dnr_ctr[] = {"off-low","off-low","auto-low", "auto-mid", "auto-high", "manual-low",   "manual-mid",   "manual-high",   "auto-smart"};
	/*
	 	const char *dnr_ctr[] = {"off", "off", "auto","auto","auto", "manual","manual","manual", "auto"};
		const char *dnr_level[] = {"low","low","low", "mid", "high", "low",   "mid",   "high",   "smart"};
	*/

	// AdaptiveIR
	const char *adaptive_ir [] = { "off", "on"};
	const char *defog_str[] = {"off", "on", "low", "mid", "high", "off", "low", "mid", "high", "off", "low", "mid", "high"};
	const char *colorvu_ctrl_str[] = { "", "manual", "auto"};

	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
	
	Token_iterator iter;
	char *str;
	int i;

	char dnr_ctr_buffer[64];
	char dnr_manual_buffer[64];
	char blc_mode_buffer[64];
	char str_buffer[64];

	char colorvu_mode_buffer[64] = {0, };
	char colorvu_ae_mode_buffer[64] = {0, };

	unsigned int colorvu_support = 0;

	mtable* runtime = NULL;
	runtime = get_runtime();

	if(cam_id < 0)
	{
		printf("[%s:%d] argument error cam_id[%d]\n", __func__, __LINE__, cam_id);
        goto endl;
	}

	if(runtime[cam_id].image.supported & NF_IPCAM_IMAGE_COLORVU){
		colorvu_support = 1;
	}
	
	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "action", "get_setup");
	http_data_set(&ctx, HTTP_ADD_QUERY, "menu", "video.camera_x");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}
	ret = 0;

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		if(strcmp(key,"ae_mode")==0){
			//support check
			SET_VALUE_FROM_STRFORMAT(info_set->ae, value, ae_mode_str, runtime[cam_id].image.exposure.support);
		}else if(strstr(key,"me_agc")){
			info_set->agc = atoi(value);
		}else if(strstr(key,"me_shutter")){
			info_set->shutter = atoi(value);
		}else if(strstr(key, "ss_mode")){
			//support check
			SET_VALUE_FROM_STRFORMAT(info_set->ss, value, slow_shutter_str, runtime[cam_id].image.slow_shutter.support);
		}else if(strstr(key,"me_agc")){
		}else if(strstr(key,"max_agc")){
		    SET_VALUE_FROM_STRFORMAT(info_set->max_agc, value, magc_str, runtime[cam_id].image.max_agc.support);
		}else if(strstr(key,"iris_mode")){
			SET_VALUE_FROM_STRFORMAT(info_set->iris, value, offon_str, runtime[cam_id].image.iris.support);
		}else if(strstr(key,"blc_ctrl")){
		   /*
		   if(info_set->blc < 4){
		   SET_VALUE_FROM_STRFORMAT(info_set->blc, value, blc_mode_str, runtime[cam_id].image.blc.support);
		   if(info_set->blc == 0 && strcmp(value, "on") == 0){
		   SET_VALUE_FROM_STRFORMAT(info_set->blc, "adap", blc_mode_str, runtime[cam_id].image.blc.support);
		 	 }
		   }
		   */
	       snprintf(blc_mode_buffer, sizeof(blc_mode_buffer), "%s", str_null_to_blank(value));
		}else if(strstr(key,"awb_mode")){
		    SET_VALUE_FROM_STRFORMAT(info_set->awb, value, awb_mode_str, runtime[cam_id].image.wb.support);
		}else if(strstr(key,"mwb_mode")){
		    SET_VALUE_FROM_STRFORMAT(info_set->mwb, value, mwb_mode_str, runtime[cam_id].image.mwb.support);
		}else if(strstr(key,"img_sharp")){
		    info_set->sharpness = atoi(value);
		}else if(strstr(key,"img_bright")){
		    info_set->brightness = atoi(value);
		}else if(strstr(key,"img_contrast")){
		    info_set->contrast = atoi(value);
		}else if(strstr(key,"img_color")){
			info_set->color = atoi(value);
	    }else if(strstr(key,"max_shutter")){
	        SET_VALUE_FROM_STRFORMAT(info_set->max_shutter, value, max_shutter_str, runtime[cam_id].image.max_shutter.support);
	    }else if(strstr(key,"img_sharp")){
	    }else if(strstr(key,"base_shutter")){
		    SET_VALUE_FROM_STRFORMAT(info_set->base_shutter, value, base_shutter_str, runtime[cam_id].image.base_shutter.support);
	    }else if(strstr(key,"dnn_det_time")){
            SET_VALUE_FROM_STRFORMAT(info_set->dnn_time, value, dnn_det_time, runtime[cam_id].image.tg_time.support);
		}else if(strstr(key,"anti_mode")){
		    SET_VALUE_FROM_STRFORMAT(info_set->ff_mode, value, ff_mode_str, runtime[cam_id].image.anti_flicker.support);
		}else if(strstr(key,"blc_zone_ctrl")){
		    SET_VALUE_FROM_STRFORMAT(info_set->blc, value, blc_zone_str, runtime[cam_id].image.blc.support);
		}else if(strstr(key,"wdr_ctrl")){
		    SET_VALUE_FROM_STRFORMAT(info_set->wd, value, wdr_mode_str, runtime[cam_id].image.wd.support);
			if(info_set->wd == 0 && strcmp(value, "low") == 0){
		     	SET_VALUE_FROM_STRFORMAT(info_set->wd, "on", wdr_mode_str, runtime[cam_id].image.wd.support);
		    }	
		}else if(strstr(key,"dnr_ctrl")){
		    snprintf(dnr_ctr_buffer, sizeof(dnr_ctr_buffer), "%s", str_null_to_blank(value));
		}else if(strstr(key,"dnr_manual")){
		    snprintf(dnr_manual_buffer, sizeof(dnr_manual_buffer), "%s", str_null_to_blank(value));
		}else if(strstr(key,"defog_ctrl")){
		    SET_VALUE_FROM_STRFORMAT(info_set->defog, value, defog_str, runtime[cam_id].image.defog.support);
		}else if(strstr(key,"dnn_set_hour1")){
		    info_set->dnn_start_hour = atoi(value);
		}else if(strstr(key,"dnn_set_hour2")){
		    info_set->dnn_end_hour = atoi(value);
		}else if(strstr(key,"dnn_set_min1")){
		    info_set->dnn_start_min = atoi(value);
		}else if(strstr(key,"dnn_set_min2")){
		    info_set->dnn_end_min = atoi(value);
		}else if(strstr(key,"img_hue")){
		    info_set->tint = atoi(value);
		/************************************************
		 *          * Dnn
		 *************************************************/
		//}else if(strstr(key,"ir_led")){
		}else if(strstr(key,"dnn_mode")){
			SET_VALUE_FROM_STRFORMAT(info_set->dnn, value, dnn_mode_str, runtime[cam_id].image.day_night.support);
		}else if(strstr(key,"adap_ir")){
            SET_VALUE_FROM_STRFORMAT(info_set->adaptive_ir, value, adaptive_ir, runtime[cam_id].image.adaptive_ir.support);
		/************************************************
		 *          * ColorVU
		 *************************************************/
		}else if(strstr(key,"colorvu_ctrl")){   // manual auto
            SET_VALUE_FROM_STRFORMAT(info_set->colorvu_ctrl, value, colorvu_ctrl_str, runtime[cam_id].image.colorvu_ctrl.support);
	    }else if(strstr(key,"colorvu_mode")){   // auto off on schedule
	        snprintf(colorvu_mode_buffer, sizeof(colorvu_mode_buffer), "%s", str_null_to_blank(value));
	    }else if(strstr(key,"colorvu_ae_mode")){
	        snprintf(colorvu_ae_mode_buffer, sizeof(colorvu_ae_mode_buffer), "%s", str_null_to_blank(value));
	    }else if(strstr(key,"colorvu_level")){  //integer
	        info_set->colorvu_level = atoi(value);
		}
		release_token(&iter_data);
	}
	release_token(&iter);

	snprintf(str_buffer, sizeof(str_buffer), "%s-%s",str_null_to_blank(dnr_ctr_buffer), str_null_to_blank(dnr_manual_buffer));
	SET_VALUE_FROM_STRFORMAT(info_set->dnr_ctr, str_buffer, dnr_ctr, runtime[cam_id].image.dnr_ctr.support);

	if(info_set->blc < 4){
		SET_VALUE_FROM_STRFORMAT(info_set->blc, blc_mode_buffer, blc_mode_str, runtime[cam_id].image.blc.support);
		if(info_set->blc == 0 && strcmp(blc_mode_buffer, "on") == 0){
			SET_VALUE_FROM_STRFORMAT(info_set->blc, "adap", blc_mode_str, runtime[cam_id].image.blc.support);
		}
	}

	if(colorvu_support)
	{
		//off->day, on->night
		if(strstr(colorvu_mode_buffer, "off")){
			snprintf(colorvu_mode_buffer, sizeof(colorvu_mode_buffer), "day");
		}else if(strstr(colorvu_mode_buffer, "on")){
			snprintf(colorvu_mode_buffer, sizeof(colorvu_mode_buffer), "night");
		}
		SET_VALUE_FROM_STRFORMAT(info_set->dnn, colorvu_mode_buffer, dnn_mode_str, runtime[cam_id].image.day_night.support);
			//manual->off
		if(strstr(colorvu_ae_mode_buffer, "manual")){
			snprintf(colorvu_ae_mode_buffer, sizeof(colorvu_ae_mode_buffer), "off");
		}
		SET_VALUE_FROM_STRFORMAT(info_set->adaptive_ir, colorvu_ae_mode_buffer, adaptive_ir, runtime[cam_id].image.adaptive_ir.support);
	}
endl:
	http_release(&ctx);
	return ret;

}

extern int cam_get_mirror_mode(int *mirror, int cam_id)
{
	const char *mirr_str[] = {"none", "none", "h_mirror", "v_mirror", "hv_mirror"};

	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	mtable* runtime = NULL;
	runtime = get_runtime();

	Token_iterator iter;
	char *str;
	int i;

	if(cam_id < 0 || mirror == NULL)
	{
		printf("[%s:%d] argument error cam_id[%d] mirror[%p]\n", __func__, __LINE__, cam_id, mirror);
		goto endl;
	}

	http_init(&ctx);
	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "action", "get_setup");
	http_data_set(&ctx, HTTP_ADD_QUERY, "menu", "video.codec");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d] \n", __func__, __LINE__, ctx.status );
		ret = -1;
		goto endl;
	}

	*mirror = 0;
	ret = 0;

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		//printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		if(strstr(key, "mirror_mode")){
			//ret = atoi(value);
			*mirror = 0;
			//SET_VALUE_FROM_STRFORMAT(*mirror, value, mirr_str, 0x1f);
			SET_VALUE_FROM_STRFORMAT(*mirror, value, mirr_str, runtime[cam_id].video.mirror.support);
			release_token(&iter_data);
			break;
		}
		release_token(&iter_data);
	}
	release_token(&iter);

endl:
	http_release(&ctx);
	return ret;
}


extern int cam_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_ptz_stop(e, cam_id);
	}
	else
	{
		rtn = _common_set_ptz_stop(e, cam_id);
	}

	return rtn;
}

extern int cam_set_zoom(ptz_info *info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_zoom(info, cam_id);
	}
	else
	{
		rtn = _common_set_zoom(info, cam_id);
	}

	return rtn;
}

extern int cam_set_focus(ptz_info *info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_focus(info, cam_id);
	}
	else
	{
		rtn = _common_set_focus(info, cam_id);
	}

	return rtn;
}

extern int cam_set_piris(int value, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_piris(value, cam_id);
	}
	else
	{
		rtn = _common_set_piris(value, cam_id);
	}

	return rtn;
}

extern int cam_set_origin(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_origin(cam_id);
	}
	else
	{
		rtn = _common_set_origin(cam_id);
	}

	return rtn;
}

extern int cam_set_onepush(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_onepush(cam_id);
	}
	else
	{
		rtn = _common_set_onepush(cam_id);
	}

	return rtn;
}

extern int cam_get_zoom(int *value, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_get_zoom(value, cam_id);
	}
	else
	{
		rtn = _common_get_zoom(value, cam_id);
	}

	return rtn;
}

extern int cam_get_focus(int *value, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_get_focus(value, cam_id);
	}
	else
	{
		rtn = _common_get_focus(value, cam_id);
	}

	return rtn;
}

extern int cam_get_piris(int *value, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_get_piris(value, cam_id);
	}
	else
	{
		rtn = _common_get_piris(value, cam_id);
	}

	return rtn;
}

extern int cam_set_alarm(cam_info* info_set, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_alarm(info_set, cam_id);
	}
	else
	{
		rtn = _common_set_alarm(info_set, cam_id);
	}

	return rtn;
}

extern int cam_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_pmask(pmask_info, cam_id);
	}
	else
	{
		rtn = _ti368_set_pmask(pmask_info, cam_id);
	}

	return rtn;
}

extern int cam_set_reset_va( int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_reset_va(cam_id);
	}
	else
	{
		rtn = _common_set_reset_va(cam_id);
	}
	//_va_config_xml_import(NULL,0);

	return rtn;
}

extern int cam_set_va_config(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();
	ivca_rule_t rule;

	IPCAM_DBG(MINOR, "start");

	memset(&rule, 0x00, sizeof(rule));

	_get_vca_rule_data(&rule,cam_id);

#if 0
	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_va_config(&rule, cam_id);
	}
	else
	{
		rtn = _common_set_va_config(&rule, cam_id);
	}
#endif

	rtn = _curl_set_va_config(&rule, cam_id);

	IPCAM_DBG(MINOR, "end");

	return rtn;
}

extern int cam_factory_mode(int cam_id)
{
	const char factory_mode_raw[] =
"GET /cgi-bin/action.fcgi?api=set_setup.security.factory_clear HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
#if 0
	if (nf_sysman_get_fwver_vendor() == 30)
	{
		http_port = 8082;
	}
	else
	{
		http_port = 80;
	}
#else
	http_port = nf_ipcam_get_http_port(cam_id);
#endif
	//http_port = nf_ipcam_get_http_port(cam_id);
#if 0
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);
#endif
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MAJOR, "CH(%d) user(%s)\n", cam_id, username);
#endif

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, sizeof(sock_buf), factory_mode_raw, ip_str, auth_encbuf);

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
		IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) FactoryClear Send MSG - %s\n", cam_id, sock_buf);
#endif

#if AUTO_SSL_OFF
	usleep(100*1000);
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_set_focus_comp(focus_comp_info* info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn =_ssl_set_focus_compensation(info, cam_id);
	}
	else
	{
		rtn = _ti368_set_focus_compensation(info, cam_id);
	}

	return rtn;
}

extern int cam_get_focus_comp(focus_comp_info* info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn =_ssl_get_focus_compensation(info, cam_id);
	}
	else
	{
		rtn = _ti368_get_focus_compensation(info, cam_id);
	}

	return rtn;
}

extern int cam_set_dc_iris_cal(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_dc_iris_cal(cam_id);
	}
	else
	{
		rtn = _common_set_dc_iris_cal(cam_id);
	}

	return rtn;
}
static int _common_factory_mode(int cam_id)
{
	const char factory_mode_raw[] =
"GET /cgi-bin/action.fcgi?api=set_setup.security.factory_clear HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);

	http_port = nf_ipcam_get_http_port(cam_id);

#if 0
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);
#endif
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MAJOR, "CH(%d) user(%s)\n", cam_id, username);
#endif

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, sizeof(sock_buf), factory_mode_raw, ip_str, auth_encbuf);

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

	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) FactoryClear Send MSG - %s\n", cam_id, sock_buf);
#endif

#if AUTO_SSL_OFF
	usleep(100*1000);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_DONTWAIT) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Installmode OFF Recv MSG - %s\n", cam_id, sock_buf);
#endif

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

const static char passwd_s1_18[] = "~qaz1wsx";

static int _common_set_passwd(int cam_id)
{
	const char set_passwd_raw[] =
"action=set_setup&menu=security.factory&passwd=%s";

	char auth_encbuf[256];
	char auth_str[256];
	char http_api[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	//mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 1024, set_passwd_raw, passwd_s1_18);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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

	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Set Password Send MSG - %s\n", cam_id, sock_buf);
#endif

	usleep(1000 * 1000);
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_LENGTH, MSG_DONTWAIT) < 0)
	{
		//perror("recv");
		//close(sock);
		//sock = (-1);
		//return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Set Password Recv MSG - %s\n", cam_id, sock_buf);
#endif

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}

extern int itx_installation_mode_off(int cam_id, system_t *sys, char* usr, char* pwd) {
	const char *vof_str[] = { "ntsc", "pal" };
	const char set_install_mode_raw[] =
			"action=set_setup&menu=video.install&"
			"install_mode=off&video_format=%s";
	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	unsigned int ntp;
	char key[64];

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(sys->ipaddr&0xff), (sys->ipaddr&0xff00)>>8,
			(sys->ipaddr&0xff0000)>>16, (sys->ipaddr&0xff000000)>>24);
	http_port = sys->http_port;
	snprintf(username, 64, "%s", usr);
	snprintf(password, 64, "%s", pwd);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(key, 64, "sys.info.sig_type");
	ntp = nf_sysdb_get_bool(key);

	snprintf(http_api, 256, set_install_mode_raw, vof_str[ntp]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Installmode OFF Send MSG - %s\n", cam_id, sock_buf);
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

extern int itx_installation_mode_on(int cam_id, system_t *sys, char* usr, char* pwd)
{
	const char *vof_str[] = { "ntsc", "pal" };
	const char set_install_mode_raw[] =
			"action=set_setup&menu=video.install&"
			"install_mode=%s&video_format=%s";
	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	unsigned int ntp;
	char key[64];

	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(sys->ipaddr&0xff), (sys->ipaddr&0xff00)>>8,
			(sys->ipaddr&0xff0000)>>16, (sys->ipaddr&0xff000000)>>24);
	http_port = sys->http_port;
	snprintf(username, 64, "%s", usr);
	snprintf(password, 64, "%s", pwd);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(key, 64, "sys.info.sig_type");
	ntp = nf_sysdb_get_bool(key);

	if (sys->model_code == NF_IPCAM_MODEL_TI_368)
	{
		snprintf(http_api, 256, set_install_mode_raw, "off", vof_str[ntp]);
	}
	else
	{
		snprintf(http_api, 256, set_install_mode_raw, "on", vof_str[ntp]);
	}
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Installmode ON Send MSG - %s\n", cam_id, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	usleep(100*1000);
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_set_install_mode_off(int cam_id)
{
	const char set_install_mode_raw[] =
			"action=set_setup&menu=video.install&"
			"install_mode=off&video_format=ntsc";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
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


	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_INIT);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_install_mode_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_INIT);
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
			IPCAM_DBG(WARN, "CH(%d) InstallMode OFF setsocketopt error\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_INIT);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "CH(%d) InstallMode OFF connect error\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_INIT);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Installmode OFF Send MSG - %s\n", cam_id, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		IPCAM_DBG(WARN, "CH(%d) InstallMode OFF send error\n", cam_id);
		perror("send");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_INIT);
		return (0);
	}

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_INIT, http_api);

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_INIT, sock);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_reboot(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_reboot";
	char auth_encbuf[256];
	char auth_str[256];
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
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_reboot_raw, "reboot", ip_str, auth_encbuf);
	cam_setup_reboot_idx++;

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_REBOOT_SOFT, http_api);
	int rtn = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_REBOOT_SOFT, sock_buf);

	return rtn;
}

static int _ssl_reboot(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_reboot";
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_reboot_raw, "reboot", ip_str, auth_encbuf);
	cam_setup_reboot_idx++;

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_REBOOT_SOFT, http_api);
	int rtn = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_REBOOT_SOFT, sock_buf);

	return rtn;
}

extern int cam_factory_default(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_factoryload";
	char auth_encbuf[256];
	char auth_str[256];
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


	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_reboot_raw, "factory", ip_str, auth_encbuf);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
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
			IPCAM_DBG(ERROR, "CH(%d) Factory default setsockopt error\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d) Factory default connect error\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Factory default Send MSG - %s\n", cam_id, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) Factory default Send MSG - %s\n", cam_id, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT);
		return IPCAM_SETUP_RTN_FAILED;
	}

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, http_api);

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_set_vcodec_af(cam_info* info_set, int cam_id)
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
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
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
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];
	snprintf(http_api, 512, set_vcodec_raw,
			get_resol_string(info_set->vcodec.resolution[0]), 
			info_set->vcodec.bitrate[0],
			get_fps_string(info_set->vcodec.fps[0]),
			get_resol_string(info_set->vcodec.resolution[1]), 
			info_set->vcodec.bitrate[1],
			get_fps_string(info_set->vcodec.fps[1]),
			info_set->vcodec.af, mirr,
			get_fps_string(info_set->vcodec.fps[0]),
			get_fps_string(info_set->vcodec.fps[1]));
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

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
	sleep(10);
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, MSG_DONTWAIT) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	close(sock);
	sock = (-1);
	sleep(3);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id)
{
	const char set_pmask_raw[] =
			"action=set_setup&menu=live.ctrl_privacymask&"
			"maskarea0=%d&areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d&maskcolor0=%d&"
			"maskarea1=%d&areatx1=%d&areaty1=%d&areabx1=%d&areaby1=%d&maskcolor1=%d&"
			"maskarea2=%d&areatx2=%d&areaty2=%d&areabx2=%d&areaby2=%d&maskcolor2=%d&"
			"maskarea3=%d&areatx3=%d&areaty3=%d&areabx3=%d&areaby3=%d&maskcolor3=%d&"

			"pixelmode=0";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];

	unsigned short http_port;

	int in_type;

	int len;
	int sock;
	struct sockaddr_in sin;

	int p_array[24];

	mtable *runtime = get_runtime();


	if (pmask_info->lt[0].x < 0)
	{
		p_array[0] = 0;
		p_array[1] = 0;
		p_array[2] = 0;
		p_array[3] = 1;
		p_array[4] = 1;
		p_array[5] = 0;
	}
	else
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x+1;
		p_array[4] = pmask_info->rb[0].y+1;
		p_array[5] = pmask_info->color[0];
	}
	if (pmask_info->lt[1].x < 0)
	{
		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else
	{
		p_array[6]  = 1;
		p_array[7]  = pmask_info->lt[1].x;
		p_array[8]  = pmask_info->lt[1].y;
		p_array[9]  = pmask_info->rb[1].x+1;
		p_array[10] = pmask_info->rb[1].y+1;
		p_array[11] = pmask_info->color[1];
	}
	if (pmask_info->lt[2].x < 0)
	{
		p_array[12] = 0;
		p_array[13] = 0;
		p_array[14] = 0;
		p_array[15] = 1;
		p_array[16] = 1;
		p_array[17] = 0;
	}
	else
	{
		p_array[12] = 1;
		p_array[13] = pmask_info->lt[2].x;
		p_array[14] = pmask_info->lt[2].y;
		p_array[15] = pmask_info->rb[2].x+1;
		p_array[16] = pmask_info->rb[2].y+1;
		p_array[17] = pmask_info->color[2];
	}
	if (pmask_info->lt[3].x < 0)
	{
		p_array[18] = 0;
		p_array[19] = 0;
		p_array[20] = 0;
		p_array[21] = 1;
		p_array[22] = 1;
		p_array[23] = 0;
	}
	else
	{
		p_array[18] = 1;
		p_array[19] = pmask_info->lt[3].x;
		p_array[20] = pmask_info->lt[3].y;
		p_array[21] = pmask_info->rb[3].x+1;
		p_array[22] = pmask_info->rb[3].y+1;
		p_array[23] = pmask_info->color[3];
	}


#if 0
	if (pmask_info->rect_cnt == 0)
	{
		p_array[0] = 0;
		p_array[1] = 0;
		p_array[2] = 0;
		p_array[3] = 1;
		p_array[4] = 1;
		p_array[5] = 0;

		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else if (pmask_info->rect_cnt == 1)
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x;
		p_array[4] = pmask_info->rb[0].y;
		p_array[5] = pmask_info->color[0];

		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else if (pmask_info->rect_cnt == 2)
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x;
		p_array[4] = pmask_info->rb[0].y;
		p_array[5] = pmask_info->color[0];

		p_array[6]  = 1;
		p_array[7]  = pmask_info->lt[1].x;
		p_array[8]  = pmask_info->lt[1].y;
		p_array[9]  = pmask_info->rb[1].x;
		p_array[10] = pmask_info->rb[1].y;
		p_array[11] = pmask_info->color[1];
	}
	else
	{
		printf("[%s] ERROR | rect_cnt(%d) CH(%d)\n", __FUNCTION__, pmask_info->rect_cnt, cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}
#endif


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_pmask_raw,
			p_array[0], p_array[1], p_array[2], p_array[3], p_array[4], p_array[5],
			p_array[6], p_array[7], p_array[8], p_array[9], p_array[10], p_array[11],
			p_array[12], p_array[13], p_array[14], p_array[15], p_array[16], p_array[17],
			p_array[18], p_array[19], p_array[20], p_array[21], p_array[22], p_array[23]);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_PMASK, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_PMASK, sock_buf);

	return len;
}
static int _ti368_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id)
{
	const char set_pmask_raw[] =
			"action=set_setup&menu=live.ctrl_privacymask&"
			"maskarea0=%d&areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d&maskcolor0=%d&"
			"maskarea1=%d&areatx1=%d&areaty1=%d&areabx1=%d&areaby1=%d&maskcolor1=%d&"
			"maskarea2=%d&areatx2=%d&areaty2=%d&areabx2=%d&areaby2=%d&maskcolor2=%d&"
			"maskarea3=%d&areatx3=%d&areaty3=%d&areabx3=%d&areaby3=%d&maskcolor3=%d&"

			"pixelmode=0";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];

	unsigned short http_port;

	int in_type;
	int len;

	int sock;
	struct sockaddr_in sin;

	int p_array[24];


	if (pmask_info->lt[0].x < 0)
	{
		p_array[0] = 0;
		p_array[1] = 0;
		p_array[2] = 0;
		p_array[3] = 1;
		p_array[4] = 1;
		p_array[5] = 0;
	}
	else
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x+1;
		p_array[4] = pmask_info->rb[0].y+1;
		p_array[5] = pmask_info->color[0];
	}
	if (pmask_info->lt[1].x < 0)
	{
		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else
	{
		p_array[6]  = 1;
		p_array[7]  = pmask_info->lt[1].x;
		p_array[8]  = pmask_info->lt[1].y;
		p_array[9]  = pmask_info->rb[1].x+1;
		p_array[10] = pmask_info->rb[1].y+1;
		p_array[11] = pmask_info->color[1];
	}
	if (pmask_info->lt[2].x < 0)
	{
		p_array[12] = 0;
		p_array[13] = 0;
		p_array[14] = 0;
		p_array[15] = 1;
		p_array[16] = 1;
		p_array[17] = 0;
	}
	else
	{
		p_array[12] = 1;
		p_array[13] = pmask_info->lt[2].x;
		p_array[14] = pmask_info->lt[2].y;
		p_array[15] = pmask_info->rb[2].x+1;
		p_array[16] = pmask_info->rb[2].y+1;
		p_array[17] = pmask_info->color[2];
	}
	if (pmask_info->lt[3].x < 0)
	{
		p_array[18] = 0;
		p_array[19] = 0;
		p_array[20] = 0;
		p_array[21] = 1;
		p_array[22] = 1;
		p_array[23] = 0;
	}
	else
	{
		p_array[18] = 1;
		p_array[19] = pmask_info->lt[3].x;
		p_array[20] = pmask_info->lt[3].y;
		p_array[21] = pmask_info->rb[3].x+1;
		p_array[22] = pmask_info->rb[3].y+1;
		p_array[23] = pmask_info->color[3];
	}


#if 0
	if (pmask_info->rect_cnt == 0)
	{
		p_array[0] = 0;
		p_array[1] = 0;
		p_array[2] = 0;
		p_array[3] = 1;
		p_array[4] = 1;
		p_array[5] = 0;

		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else if (pmask_info->rect_cnt == 1)
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x;
		p_array[4] = pmask_info->rb[0].y;
		p_array[5] = pmask_info->color[0];

		p_array[6] = 0;
		p_array[7] = 0;
		p_array[8] = 0;
		p_array[9] = 1;
		p_array[10] = 1;
		p_array[11] = 0;
	}
	else if (pmask_info->rect_cnt == 2)
	{
		p_array[0] = 1;
		p_array[1] = pmask_info->lt[0].x;
		p_array[2] = pmask_info->lt[0].y;
		p_array[3] = pmask_info->rb[0].x;
		p_array[4] = pmask_info->rb[0].y;
		p_array[5] = pmask_info->color[0];

		p_array[6]  = 1;
		p_array[7]  = pmask_info->lt[1].x;
		p_array[8]  = pmask_info->lt[1].y;
		p_array[9]  = pmask_info->rb[1].x;
		p_array[10] = pmask_info->rb[1].y;
		p_array[11] = pmask_info->color[1];
	}
	else
	{
		printf("[%s] ERROR | rect_cnt(%d) CH(%d)\n", __FUNCTION__, pmask_info->rect_cnt, cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}
#endif

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_pmask_raw,
			p_array[0], p_array[1], p_array[2], p_array[3], p_array[4], p_array[5],
			p_array[6], p_array[7], p_array[8], p_array[9], p_array[10], p_array[11],
			p_array[12], p_array[13], p_array[14], p_array[15], p_array[16], p_array[17],
			p_array[18], p_array[19], p_array[20], p_array[21], p_array[22], p_array[23]);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_PMASK, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_PMASK, sock_buf);
	return len;
}

extern int cam_get_pmask(NFIPCamPrivacyMask *pmask_info, int cam_id)
{
		int ret = -1;
		HTTP_CTX ctx;
		int rc = 0;
		int len = 0;

		int enable[10] = {0, };

		Token_iterator iter;
		char *str;
		int i, n;
		NF_IPCAM_POINT *point;

		http_init(&ctx);

		//http setting
		aicam_http_default_setting(&ctx, cam_id);
		http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
		http_data_set(&ctx, HTTP_ADD_QUERY, "action", "get_setup");
		http_data_set(&ctx, HTTP_ADD_QUERY, "menu", "live.ctrl_privacymask");
		
		//http request
		rc = http_request(&ctx);
		if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
			ret = -2;
			goto endl;
		}
		if(ctx.status != 200){
			printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
			ret = -1;
			goto endl;
		}

		pmask_info->ch = cam_id;

		for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str !=NULL; str = tok_get_next(&iter))
		{
			Token_iterator iter_data;
			const char *key = tok_iterator_init(&iter_data, str, "= \t");
			const char *value = tok_get_next(&iter_data);

			if(strncmp(key, "area", 4)==0){
				n = key[6] - '0';
				if(key[4] == 't'){
					point = &(pmask_info->lt[n]);
				}else{
					point = &(pmask_info->rb[n]);
				}

				if(key[5] =='x'){
					point->x = atoi(value);
				}else{
					point->y = atoi(value);
				}
			}
			else if(strncmp(key, "maskarea", 8) == 0){
	            sscanf(key+8,"%d", &n);
	            enable[n] = atoi(value);
	        }else if(strncmp(key, "maskcolor", 9) == 0){
	            sscanf(key+9,"%d", &n);
	            pmask_info->color[n] = atoi(value);
	        }else{
	        }
			release_token(&iter_data);
		}
		release_token(&iter);

		for(i = 0; i<10; i++){
			if(enable[i] > 0){
				pmask_info->rect_cnt = i+1;
			}else{
				pmask_info->lt[i].x = -1;
				pmask_info->lt[i].y = -1;
				pmask_info->rb[i].x = -1;
				pmask_info->rb[i].y = -1;
			}
		}

		ret = 0;

endl:
		http_release(&ctx);
		return ret;
}

// -1,-2 : API Error / -3 : Not Supported view mode / 1,2,3,4 : return view mode
extern int cam_get_view_mode(int cam_id)
{
	int ret = -3;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i;

    if(cam_id < 0)
    {
        printf("[%s:%d] argument error cam_id[%d]\n", __func__, __LINE__, cam_id);
        goto endl;
    }

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.video.hardware.dewarping");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

    //ret = 0;

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
        //printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strstr(key, "view_mode")){
            ret = atoi(value);
            release_token(&iter_data);
            break;
        }
		release_token(&iter_data);
	}
	release_token(&iter);

endl:
	http_release(&ctx);
	// -1,-2 : API Error / -3 : Not Supported view mode / 1,2,3,4 : return view mode
	return ret;
}

extern int cam_get_encode_mode(int cam_id)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i;

    if(cam_id < 0)
    {
        printf("[%s:%d] argument error cam_id[%d]\n", __func__, __LINE__, cam_id);
        goto endl;
    }

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.video.hardware.dewarping");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){ /* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error [%d] \n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	ret = 0;

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str!=NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		//
		if(strstr(key,"enc_mode")){
			ret = atoi(value);
			release_token(&iter_data);
			break;
		}
		release_token(&iter_data);
	}
	release_token(&iter);

endl:
	http_release(&ctx);
	return ret;
}

extern int cam_set_encode_mode(int cam_id, int mode)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
	char value_buffer[100];

	Token_iterator iter;
	char *str;
	int i;

	if(cam_id < 0 || mode < 0)
	{
		printf("[%s:%d] argument error cam_id[%d] mode[%d]\n", __func__, __LINE__, cam_id, mode);
		goto endl;
	}

	snprintf(value_buffer, sizeof(value_buffer), "%d", mode);

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.video.hardware.dewarping");
	http_data_set(&ctx, HTTP_ADD_QUERY, "enc_mode", value_buffer);

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){ /* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error [%d] \n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx),"\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		release_token(&iter_data);
	}
	release_token(&iter);

	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}

extern int cam_set_fisheye_init_legacy(int cam_id, int enc_mode)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
    char value_buffer[100];
    char value_buffer2[100];

	Token_iterator iter;
	char *str;
	int i;

    if(cam_id < 0 || enc_mode < 0)
    {
        printf("[%s:%d] argument error cam_id[%d] enc_mode[%d]\n", __func__, __LINE__, cam_id, enc_mode);
        goto endl;
    }

    snprintf(value_buffer, sizeof(value_buffer), "%d", enc_mode);

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.video.hardware.dewarping");
	http_data_set(&ctx, HTTP_ADD_QUERY, "enc_mode", value_buffer);

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		//printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		release_token(&iter_data);
	}
	release_token(&iter);
	
	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}
extern int cam_set_fisheye_init(int cam_id, int enc_mode, int view_mode)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
    char value_buffer[100];
    char value_buffer2[100];

	Token_iterator iter;
	char *str;
	int i;

    if(cam_id < 0 || enc_mode < 0 || view_mode < 0)
    {
        printf("[%s:%d] argument error cam_id[%d] enc_mode[%d] view_mode[%d]\n", __func__, __LINE__, cam_id, enc_mode, view_mode);
        goto endl;
    }

    snprintf(value_buffer, sizeof(value_buffer), "%d", enc_mode);
    snprintf(value_buffer2, sizeof(value_buffer), "%d", view_mode);

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.video.hardware.dewarping");
	http_data_set(&ctx, HTTP_ADD_QUERY, "enc_mode", value_buffer);
	http_data_set(&ctx, HTTP_ADD_QUERY, "view_mode", value_buffer2);

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);
		//printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		release_token(&iter_data);
	}
	release_token(&iter);
	
	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}

extern int cam_get_motion_area(NFIPCamSetupMotionArea *motion_info, int cam_id)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i;

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.event.motion");
	//http_data_set(&ctx, HTTP_ADD_QUERY, "alarm", motion_info->ai_alarm_event ? "on" : "off");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);

		//printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		release_token(&iter_data);
	}
	release_token(&iter);
	
	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}

extern int cam_set_motion_area(NFIPCamSetupMotionArea *motion_info, int cam_id)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
	char area[1400];

	Token_iterator iter;
	char *str;
	int i;

	http_init(&ctx);

	memcpy(area, motion_info->area, motion_info->block_height * motion_info->block_width);
	area[motion_info->block_height * motion_info->block_width] = 0;

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.event.motion");
/*
	http_data_set(&ctx, HTTP_ADD_QUERY, "area0", area);
	http_data_set(&ctx, HTTP_ADD_QUERY, "area1", area);
	http_data_set(&ctx, HTTP_ADD_QUERY, "area2", area);
	http_data_set(&ctx, HTTP_ADD_QUERY, "area3", area);
*/
	http_data_set(&ctx, HTTP_ADD_QUERY, "area0", "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
	http_data_set(&ctx, HTTP_ADD_QUERY, "area1", "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
	http_data_set(&ctx, HTTP_ADD_QUERY, "area2", "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
	http_data_set(&ctx, HTTP_ADD_QUERY, "area3", "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");

	http_data_set(&ctx, HTTP_ADD_QUERY, "sensitivity0", "15");
	http_data_set(&ctx, HTTP_ADD_QUERY, "sensitivity1", "15");
	http_data_set(&ctx, HTTP_ADD_QUERY, "sensitivity2", "15");
	http_data_set(&ctx, HTTP_ADD_QUERY, "sensitivity3", "15");
	
	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);

		release_token(&iter_data);
	}
	release_token(&iter);
	
	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}

static int _add_string(char *string_buffer, int pos, size_t buffer_size, const char *add_string)
{
	int len = 0;
	if(add_string == NULL){
		printf("[%s:%d] warn add_string[%p]\n", __FUNCTION__, __LINE__, add_string);
		return -1;
	}

	if(add_string[0] == '\0'){
		printf("[%s:%d] warn add_string[%s]\n",__FUNCTION__, __LINE__,add_string);
		return -1;
	}
	if(pos < 0 || pos >= buffer_size){
		printf("[%s:%d] warn pos[%d] buffer_size[%u]\n", __func__, __LINE__, pos, buffer_size);
		return -1;
	}

	if(string_buffer[0] != '\0'){
		string_buffer[pos++] = ',';
		string_buffer[pos] = '\0';
	}

	len = snprintf(string_buffer+pos, buffer_size-pos, "%s", (add_string));
	len += pos;

	if(len >= buffer_size){
		string_buffer[buffer_size] = '\0';
		printf("[%s:%d] warn len[%d] > buffer_size[%u] string_buffer[%s]\n", __func__, __LINE__, len, buffer_size, string_buffer);
		len = buffer_size;
		return len;
	}
	return len;
}

extern int cam_set_motion_smart(NFIPCamSetupMotionSmart *motion_info, int cam_id)
{
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i;

	char smartmotion_class[256] = {0,};
	char key[128];
	char value[128];

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.event.smartmotion");
	http_data_set(&ctx, HTTP_ADD_QUERY, "enable", motion_info->smart_motion_enable ? "on" : "off");
	//http_data_set(&ctx, HTTP_ADD_QUERY, "alarmevt", motion_info->ai_alarm_event ? "on" : "off");
	
	for(i=0; i < motion_info->smart_motion_option_size; i++){
		if(motion_info->smart_motion_options[i].enable){
			len = _add_string(smartmotion_class, len, sizeof(smartmotion_class), motion_info->smart_motion_options[i].name);

			snprintf(key, sizeof(key), "%s_threshold", motion_info->smart_motion_options[i].name);
			snprintf(value, sizeof(value), "%d", motion_info->smart_motion_options[i].threshold);
			http_data_set(&ctx, HTTP_ADD_QUERY, key, value);
		}
	}
	if(len > 0){
		http_data_set(&ctx, HTTP_ADD_QUERY, "smartmotion_class", smartmotion_class);
	}

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);

		release_token(&iter_data);
	}
	release_token(&iter);
	
	ret = 0;

endl:
	http_release(&ctx);
	return ret;
}

static int _enable_smart_options(NFIPCamSetupMotionSmart* motion_info, const char *str)
{
	int i;
	if(motion_info == NULL || str == NULL) return -1;

	for(i=0; i < motion_info->smart_motion_option_size; i++)
	{
		if(strncmp(str, motion_info->smart_motion_options[i].name, strlen(str)) == 0){
			motion_info->smart_motion_options[i].enable = 1;
			return 1;
		}
	}

	return 0;
}

extern int cam_get_motion_smart(NFIPCamSetupMotionSmart *motion_info, int cam_id)
{
	char* rbuf;
	int ret = -1;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

	Token_iterator iter;
	char *str;
	int i = 0;
	char smartmotion_class[256] = {0,};
	char nbuf[50];

	if(motion_info == NULL){
		printf("[%s:%d] error motion_info is null\n", __func__, __LINE__);
		return -1;
	}
	memset(motion_info, 0, sizeof(NFIPCamSetupMotionSmart));

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, cam_id);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.event.smartmotion");

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){ /* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}

	if(ctx.status != 200){
		printf("[%s:%d] http response error [%d] \n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	motion_info->ch = cam_id;
	i = 0;
	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *value = tok_get_next(&iter_data);

		if(key == NULL || value == NULL){
			release_token(&iter_data);
			continue;
		}
		
		if(strstr(key, "_threshold")){
			memset(nbuf, 0x00, sizeof(nbuf));
			snprintf(nbuf, sizeof(nbuf), "%s", key);
			memset(strstr(nbuf, "_threshold"), 0x00, sizeof(nbuf) - (strstr(nbuf, "_threshold") - nbuf));	// remove "_threshold" string from key
			snprintf(motion_info->smart_motion_options[i].name, sizeof(motion_info->smart_motion_options[i].name), "%s", nbuf);
			motion_info->smart_motion_options[i].threshold = atoi(value);
			i++;
		}else if(strstr(key, "enable")){
			motion_info->smart_motion_enable = (strstr(value, "on") != NULL) ? 1:0;
		}else if(strstr(key, "alarmevt")){
			motion_info->ai_alarm_event = (strstr(value, "on") != NULL) ? 1:0;
		}else if(strstr(key, "smartmotion_class")){
			snprintf(smartmotion_class, sizeof(smartmotion_class), "%s", str_null_to_blank(value));
		}

		release_token(&iter_data);
	}
	release_token(&iter);
	motion_info->smart_motion_option_size = i;

	if(strlen(smartmotion_class) > 0){
		for(str = strtok_r(smartmotion_class, ",", &rbuf); str !=NULL; str=strtok_r(NULL,",", &rbuf)){
			if(strlen(str) > 0){
				_enable_smart_options(motion_info, str);
			}
		}
	}
	ret = 0;
endl:
	http_release(&ctx);
	return ret;
}

extern int cam_set_motion_area_a2(NFIPCamSetupMotionArea *motion_info, int cam_id)
{
	const char set_marea_raw[] =
		"action=set_setup&menu=event.motion&"
		"area0=%s&sensitivity0=%d&"
		"area1=%s&sensitivity1=%d&"
		"area2=%s&sensitivity2=%d&"
		"area3=%s&sensitivity3=%d";
	const char* MAREA_METHOD_STR[] = {
		"NONE",
		"RECTANGLE",
		"POLYGON",
		"CELL",
		"RAW_STREAM"
	};

	char http_api[1536];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int len;
	int i, j;
	int cell_cnt;
	int area_max = 4;
	char *cells[4];
	int x0,y0,x1,y1,cur_x,cur_y;


	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_SET_MOTION);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	cell_cnt = motion_info->block_width * motion_info->block_height;

	for(i = 0; i < motion_info->area_num; i++)
	{
		cells[i] = (char*) malloc((size_t)(cell_cnt+1));
		memset(cells[i], 0x00, (size_t)(cell_cnt+1));
		x0 = motion_info->marea[i].FIGURE.RECTANGLE.left_top.x;
		y0 = motion_info->marea[i].FIGURE.RECTANGLE.left_top.y;
		x1 = motion_info->marea[i].FIGURE.RECTANGLE.right_bottom.x;
		y1 = motion_info->marea[i].FIGURE.RECTANGLE.right_bottom.y;
		for (j = 0; j < cell_cnt; j++)
		{
			cur_x = j % motion_info->block_width;
			cur_y = j / motion_info->block_width;

			if (cur_x >= x0 && cur_x <= x1 && cur_y >= y0 && cur_y <= y1)
			{
				cells[i][j] = '1';
			}
			else
			{
				cells[i][j] = '0';
			}
		}
	}
	for (i = motion_info->area_num; i < area_max; i++)
	{
		cells[i] = (char*) malloc((size_t)(cell_cnt+1));
		memset(cells[i], 0x00, (size_t)(cell_cnt+1));
		for (j = 0; j < cell_cnt; j++)
		{
			cells[i][j] = '0';
		}
	}

	snprintf(http_api, 1536, set_marea_raw,
			cells[0], motion_info->marea[0].sensitivity * 10,
			cells[1], motion_info->marea[0].sensitivity * 10,
			cells[2], motion_info->marea[0].sensitivity * 10,
			cells[3], motion_info->marea[0].sensitivity * 10);

	free(cells[0]);
	free(cells[1]);
	free(cells[2]);
	free(cells[3]);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_MOTION, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_MOTION, sock_buf);

	return len;
}

extern int cam_get_af_capa(cam_info *info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_get_afcapa(info, cam_id);
	}
	else
	{
		rtn = _common_get_afcapa(info, cam_id);
	}

	return rtn;
}

extern int cam_set_roi_area(NFIPCamSetupROIArea *roi_info, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();
	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_roi_area(roi_info, cam_id);
	}
	else
	{
		rtn = _cam_set_roi_area(roi_info, cam_id);
	}

}

/*
 * Hisilicon SDK version 5.12.x.7 integration
 * ROI SET API
 */
static int _cam_set_roi_area(NFIPCamSetupROIArea *roi_info, int cam_id)
{
	const char set_roi_api_raw[] =
		"action=set_setup.live.roi&"
		"width=%d&"
		"height=%d&"
		"mode=%s&"
		"quality=%s&"
		"%s";

	const char set_roi_area_raw[] =
		"areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d&level0=%d&"
		"areatx1=%d&areaty1=%d&areabx1=%d&areaby1=%d&level1=%d&"
		"areatx2=%d&areaty2=%d&areabx2=%d&areaby2=%d&level2=%d&"
		"areatx3=%d&areaty3=%d&areabx3=%d&areaby3=%d&level3=%d&"
		"areatx4=%d&areaty4=%d&areabx4=%d&areaby4=%d&level4=%d&"
		"areatx5=%d&areaty5=%d&areabx5=%d&areaby5=%d&level5=%d&"
		"areatx6=%d&areaty6=%d&areabx6=%d&areaby6=%d&level6=%d&"
		"areatx7=%d&areaty7=%d&areabx7=%d&areaby7=%d&level7=%d";

	const char* ROI_MODE_STR[] = {
		"off",
		"auto",
		"manual",
	};

	const char* ROI_QUALITY_STR[] = {
		"low",
		"mid",
		"high",
	};

	char http_api[1536];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int len;
	int i, j;
	int roi_area_max = 8;

	char area_info[1024];
	int area_info_set[8][5];

	int roi_str_num = 0;
	for(i=0; i < ROI_MODE_NR; i++)
	{
		if((1 << i) == roi_info->roi_mode){
			roi_str_num = i;
			break;
		}
	}
	
	memset(area_info, 0x00, 1024);

	for(i = 0; i < 8; i++)
	{
		memset(area_info_set[i], 0x00, sizeof(area_info_set[i]));
	}

	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_SET_ROI);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	for(i = 0; i < roi_info->roi_area_num; i++)
	{
		area_info_set[i][0] = roi_info->roi_area[i].left_top.x;
		area_info_set[i][1] = roi_info->roi_area[i].left_top.y;
		area_info_set[i][2] = roi_info->roi_area[i].right_bottom.x;
		area_info_set[i][3] = roi_info->roi_area[i].right_bottom.y;

		area_info_set[i][4] = roi_info->roi_area[i].interest_level;
	}

	snprintf(area_info, 1024, set_roi_area_raw, 
			area_info_set[0][0], area_info_set[0][1], area_info_set[0][2], area_info_set[0][3], area_info_set[0][4],
			area_info_set[1][0], area_info_set[1][1], area_info_set[1][2], area_info_set[1][3], area_info_set[1][4],
			area_info_set[2][0], area_info_set[2][1], area_info_set[2][2], area_info_set[2][3], area_info_set[2][4],
			area_info_set[3][0], area_info_set[3][1], area_info_set[3][2], area_info_set[3][3], area_info_set[3][4],
			area_info_set[4][0], area_info_set[4][1], area_info_set[4][2], area_info_set[4][3], area_info_set[4][4],
			area_info_set[5][0], area_info_set[5][1], area_info_set[5][2], area_info_set[5][3], area_info_set[5][4],
			area_info_set[6][0], area_info_set[6][1], area_info_set[6][2], area_info_set[6][3], area_info_set[6][4],
			area_info_set[7][0], area_info_set[7][1], area_info_set[7][2], area_info_set[7][3], area_info_set[7][4]);

	snprintf(http_api, 1536, set_roi_api_raw, 
			roi_info->width, roi_info->height, 
			ROI_MODE_STR[roi_str_num], ROI_QUALITY_STR[roi_info->roi_quality], area_info);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ROI, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ROI, sock_buf);

	return len;
}

static int _ssl_set_roi_area(NFIPCamSetupROIArea *roi_info, int cam_id)
{
	const char set_roi_api_raw[] =
		"action=set_setup.live.roi&"
		"width=%d&"
		"height=%d&"
		"mode=%s&"
		"quality=%s&"
		"%s";

	const char set_roi_area_raw[] =
		"areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d&level0=%d&"
		"areatx1=%d&areaty1=%d&areabx1=%d&areaby1=%d&level1=%d&"
		"areatx2=%d&areaty2=%d&areabx2=%d&areaby2=%d&level2=%d&"
		"areatx3=%d&areaty3=%d&areabx3=%d&areaby3=%d&level3=%d&"
		"areatx4=%d&areaty4=%d&areabx4=%d&areaby4=%d&level4=%d&"
		"areatx5=%d&areaty5=%d&areabx5=%d&areaby5=%d&level5=%d&"
		"areatx6=%d&areaty6=%d&areabx6=%d&areaby6=%d&level6=%d&"
		"areatx7=%d&areaty7=%d&areabx7=%d&areaby7=%d&level7=%d";

	const char* ROI_MODE_STR[] = {
		"off",
		"auto",
		"manual",
	};

	const char* ROI_QUALITY_STR[] = {
		"low",
		"mid",
		"high",
	};

	char http_api[1536];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	int len;
	int i, j;
	int roi_area_max = 8;

	char area_info[1024];
	int area_info_set[8][5];


	int roi_str_num = 0;
	for(i = 0; i < ROI_MODE_NR; i++)
	{
		if ((1 << i) == roi_info->roi_mode) {
			roi_str_num = i;
			break;
		}
	}
	
	memset(area_info, 0x00, 1024);

	for(i = 0; i < 8; i++)
	{
		memset(area_info_set[i], 0x00, sizeof(area_info_set[i]));
	}

	nf_ipcam_setup_sending(cam_id, NF_IPCAM_TYPE_SET_ROI);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	for(i = 0; i < roi_info->roi_area_num; i++)
	{
		area_info_set[i][0] = roi_info->roi_area[i].left_top.x;
		area_info_set[i][1] = roi_info->roi_area[i].left_top.y;
		area_info_set[i][2] = roi_info->roi_area[i].right_bottom.x;
		area_info_set[i][3] = roi_info->roi_area[i].right_bottom.y;

		area_info_set[i][4] = roi_info->roi_area[i].interest_level;
	}

	snprintf(area_info, 1024, set_roi_area_raw, 
			area_info_set[0][0], area_info_set[0][1], area_info_set[0][2], area_info_set[0][3], area_info_set[0][4],
			area_info_set[1][0], area_info_set[1][1], area_info_set[1][2], area_info_set[1][3], area_info_set[1][4],
			area_info_set[2][0], area_info_set[2][1], area_info_set[2][2], area_info_set[2][3], area_info_set[2][4],
			area_info_set[3][0], area_info_set[3][1], area_info_set[3][2], area_info_set[3][3], area_info_set[3][4],
			area_info_set[4][0], area_info_set[4][1], area_info_set[4][2], area_info_set[4][3], area_info_set[4][4],
			area_info_set[5][0], area_info_set[5][1], area_info_set[5][2], area_info_set[5][3], area_info_set[5][4],
			area_info_set[6][0], area_info_set[6][1], area_info_set[6][2], area_info_set[6][3], area_info_set[6][4],
			area_info_set[7][0], area_info_set[7][1], area_info_set[7][2], area_info_set[7][3], area_info_set[7][4]);

	snprintf(http_api, 1536, set_roi_api_raw, 
			roi_info->width, roi_info->height, 
			ROI_MODE_STR[roi_str_num], ROI_QUALITY_STR[roi_info->roi_quality], area_info);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ROI, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ROI, sock_buf);

	return len;

}

extern int cam_sn_get(struct sn_info *info, int cam_id, int v)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_sn_get(info, cam_id, v);
	}
	else
	{
		rtn = _ti386_sn_get(info, cam_id, v);
	}

	return rtn;
}


static int cam_get_model_info_raw_digest(
	cam_model_info* info_buf,
	unsigned int ip, 
	unsigned short port,
	char *u,
	char *p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

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
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		const char f_auth[]		= "401 Unauthorized";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			errcode = strstr(sock_buf, f_auth);
			if (errcode != NULL)
			{
				_release_resource(&sock, NULL, NULL, NULL);
				return (1);
			}
			else
			{
				_release_resource(&sock, NULL, NULL, NULL);
				return (0);
			}
		}
	}

	{
		_release_resource(&sock, NULL, NULL, NULL);
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_swver2[]	= "swver2=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* sdkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}
		/* vendor fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver2);
		if (p == NULL)
		{
			memset(info_buf->swver2, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_swver2);
			//strcpy(info_buf->swver2, s);
#if 1
			p = strstr(s, "&");
			if (p == NULL)
			{
				int k = 0;
				p = s;
				for (k = 0; k < 64; k++)
				{
					p++;
					if (*p == 0x0d) { break; }
				}
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->swver2, buf);
#endif
		}
	}

	return 3;
}

extern int cam_get_model_info_raw(
	cam_model_info* info_buf,
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	int *use_ssl
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

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


	*use_ssl = 0;
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
		return (-2);
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

proceed_no_ssl:
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

	if (len == 0) // for lightppd (past compatible)
	{
		IPCAM_DBG(MINOR, "use SSL connection addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		*use_ssl = 1;
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_model_info_raw(info_buf, ip, port, u, p));
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		const char f_auth[]		= "401 Unauthorized";
		const char f_ssl[]		= "HTTP/1.1 497";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				IPCAM_DBG(MINOR, "use DIGEST authentication addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				_release_resource(&sock, NULL, NULL, NULL);
				len = cam_get_model_info_raw_digest(info_buf, ip, port, u, p, sock_buf);
				return len;
			}
if(strstr(sock_buf, f_ssl) != NULL)
			{
				*use_ssl = 1;
				_release_resource(&sock, NULL, NULL, NULL);
				return (_ssl_get_model_info_raw(info_buf, ip, port, u, p));
			}
			IPCAM_DBG(WARN, "HTTP error addr(%s:%d) login(%s:*)\n", ip_str, port, u);
			return (0);
		}
	}

	{
		_release_resource(&sock, NULL, NULL, NULL);
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_swver2[]	= "swver2=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* sdkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}

		/* vendor fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver2);
		if (p == NULL)
		{
			memset(info_buf->swver2, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_swver2);
			//strcpy(info_buf->swver2, s);
#if 1
			p = strstr(s, "&");
			if (p == NULL)
			{
				int k = 0;
				p = s;
				for (k = 0; k < 64; k++)
				{
					p++;
					if (*p == 0x0d) { break; }
				}
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->swver2, buf);
#endif
		}
	}

	return 3;
}

extern int cam_get_model_info_by_sysdb(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

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

	mtable *runtime = NULL;
	unsigned char cal_mac[3];


	runtime = get_runtime();
	while (runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = runtime[cam_id].admin_http;

	{
		gchar key_u[64];
		gchar key_p[64];
		gchar *u,*p;
		snprintf(key_u, 64, "cam.logininfo.L%d.id", cam_id);
		snprintf(key_p, 64, "cam.logininfo.L%d.pwd", cam_id);
		u = nf_sysdb_get_str_nocopy(key_u);
		p = nf_sysdb_get_str_nocopy(key_p);
		strncpy(username, u, 64);
		strncpy(password, p, 64);
		strncpy(runtime[cam_id].username, u, 64);
		strncpy(runtime[cam_id].password, p, 64);
	}

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
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
		printf("[%s] USE SSL(CH%d)\n", __FUNCTION__, cam_id);
		runtime[cam_id].sys.use_ssl = 1;
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_model_info_by_sysdb(info_buf, cam_id));
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		const char f_auth[]		= "401 Unauthorized";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, f_auth) != NULL)
			{
				return (1);
			}
			return (0);
		}
	}

	{
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_swver2[]	= "swver2=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* sdkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}
		/* vendor fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver2);
		if (p == NULL)
		{
			memset(info_buf->swver2, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_swver2);
			//strcpy(info_buf->swver2, s);
#if 1
			p = strstr(s, "&");
			if (p == NULL)
			{
				int k = 0;
				p = s;
				for (k = 0; k < 64; k++)
				{
					p++;
					if (*p == 0x0d) { break; }
				}
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->swver2, buf);
#endif
		}
	}

	return 3;
}

extern int cam_get_model_info_by_backdoor(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

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

	mtable *runtime = NULL;
	unsigned char cal_mac[3];


	runtime = get_runtime();
	while (runtime == NULL)
	{
		usleep(10*1000);
		runtime = get_runtime();
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
#if 0
	if (nf_sysman_get_fwver_vendor() == 30)
	{
		http_port = 8082;
	}
	else
	{
		http_port = 80;
	}
#else
	http_port = 8082;
#endif
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		{
			close(sock);
			http_port = 80;
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
					_release_resource(&sock, NULL, NULL, NULL);
					return (-1);
				}
			}
			if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
			{
				printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
				perror("connect");
				_release_resource(&sock, NULL, NULL, NULL);
				return (-1);
			}
			else
			{
				goto proceed_no_ssl;
			}
		}
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
		printf("[%s] USE SSL(CH%d)\n", __FUNCTION__, cam_id);
		runtime[cam_id].sys.use_ssl = 1;
		_release_resource(&sock, NULL, NULL, NULL);
		return (_ssl_get_model_info_by_backdoor(info_buf, cam_id));
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (0);
		}
	}

	{
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_swver2[]	= "swver2=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* sdkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}
		/* vendor fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver2);
		if (p == NULL)
		{
			memset(info_buf->swver2, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_swver2);
			strcpy(info_buf->swver2, s);
#if 0
			p = strstr(s, "&");
			if (p == NULL)
			{
				p == strstr(s, "\r\n");
				if (p == NULL)
				{
					return 0;
				}
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->swver2, buf);
#endif
		}
	}

	return 3;
}

extern int cam_get_model_info(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

	char auth_encbuf[256];
	char auth_str[256];
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
	http_port = 80;
#ifdef ENABLE_PROJECT_KMW
	strcpy(username, "admin");
	strcpy(password, "12345678");
#else
	strcpy(username, "ADMIN");
	strcpy(password, "1234");
#endif

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

#if 0
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return (-1);
	}
#endif

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
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (0);
		}
	}

	{
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}
		/* stkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}
	}

	return 3;
}

extern int cam_get_model_info_nopnd(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = runtime[cam_id].sys.http_port;
	strcpy(username, runtime[cam_id].username);
	strcpy(password, runtime[cam_id].password);

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

#if 0
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = WAIT_REPLY_SECS;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return (-1);
	}
#endif

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
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return (0);
		}
	}

	{
		const char f_name[]		= "hwver=";
		const char f_swver[]	= "swver=";
		const char f_vendor[]	= "brand=";
		const char f_mac[]		= "macaddr=";
		const char f_std[]		= "stdver=";
		const char f_sdk[]		= "httpapi_ver=";
		const char f_zoom_module_name[]		= "zoom_module_name=";
		const char f_zoom_module_fwver[]	= "zoom_module_fwver=";
		const char f_capa_version[] = "capa_version=";

		char *s = NULL;
		char *p = NULL;
		char buf[128];


		/* mac parsing */
		s = sock_buf;
		p = strstr(s, f_mac);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_mac);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 6);
		memcpy(buf, s, (size_t)(p - s));
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				&info_buf->mac[0], &info_buf->mac[1],
				&info_buf->mac[2], &info_buf->mac[3],
				&info_buf->mac[4], &info_buf->mac[5]);

		/* model name parsing */
		s = sock_buf;
		p = strstr(s, f_name);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_name);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->name, buf);

		/* fw version parsing */
		s = sock_buf;
		p = strstr(s, f_swver);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_swver);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->swver, buf);

		/* vendor parsing */
		s = sock_buf;
		p = strstr(s, f_vendor);
		if (p == NULL)
		{
			return 0;
		}
		s = p + strlen(f_vendor);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return 0;
		}
		memset(buf, 0x00, 128);
		memcpy(buf, s, (size_t)(p - s));
		strcpy(info_buf->vendor, buf);

		/* stdver parsing */
		s = sock_buf;
		p = strstr(s, f_std);
		if (p == NULL) 
		{
			memset(info_buf->stdver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_std);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->stdver, buf);
		}

		/* zoom_module_name parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_name);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_name, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_name);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_name, buf);
			}
		}

		/* zoom_module_fwver parsing */
		s = sock_buf;
		p = strstr(s, f_zoom_module_fwver);
		if (p == NULL) 
		{
			memset(info_buf->zoom_module_fwver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_zoom_module_fwver);
			p = strstr(s, "\r\n");	//parse EOF because of "Not &"
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->zoom_module_fwver, buf);
			}
		}
		/* capability version parsing */
		s = sock_buf;
		p = strstr(s, f_capa_version);
		if (p == NULL) 
		{
			memset(info_buf->capa_version, 0x00, 16);
		}
		else
		{
			s = p + strlen(f_capa_version);
			p = strstr(s, "&");
			if (p == NULL)
			{
				//return 0;	current No essential factor(under fwver6601)
			}
			else
			{
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->capa_version, buf);
			}
		}

		/* stkver parsing */
		s = sock_buf;
		p = strstr(s, f_sdk);
		if (p == NULL) 
		{
			memset(info_buf->sdkver, 0x00, 64);
		}
		else
		{
			s = p + strlen(f_sdk);
			p = strstr(s, "&");
			if (p == NULL)
			{
				return 0;
			}
			memset(buf, 0x00, 128);
			memcpy(buf, s, (size_t)(p - s));
			strcpy(info_buf->sdkver, buf);
		}
	}

	return 3;
}

static void _jykim_str_replace(char* s, char* from, char* to)
{
	int cp_len;
	int s_len, from_len, to_len;
	char *p = NULL;
	char temp_buf[64];

	s_len = strlen(s);
	from_len = strlen(from);
	to_len = strlen(to);

	p = strstr(s, from);

	if (p == NULL) { return; }

	cp_len = (p-s);
	p += from_len;
	memset(temp_buf, 0x00, 64);
	memcpy(temp_buf, s, cp_len);
	memcpy(temp_buf+cp_len, to, to_len);
	memcpy(temp_buf+cp_len+to_len, p, s_len-(p-s));
	strcpy(s, temp_buf);
}

static int _ssl_set_dhcpon(int cam_id)
{
	const char set_api_raw[] =
			"action=set_setup&menu=network.ipsetup&dhcpon=yes";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}
static int _ti368_set_dhcpon(int cam_id)
{
	const char set_api_raw[] =
			"action=set_setup&menu=network.ipsetup&dhcpon=yes";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_set_time_info(int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if (runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_timezone_info(cam_id);
	}
	else
	{
		rtn = _cam_set_timezone_info(cam_id);
	}
}

extern int cam_set_corridor_mode(int corridor_mode, int cam_id)
{
	int rtn = 0;
	mtable *runtime = get_runtime();

	if(runtime[cam_id].sys.use_ssl)
	{
		rtn = _ssl_set_corridor_mode(cam_id, corridor_mode);
	}
	else
	{
		rtn = _ti368_set_corridor_mode(cam_id, corridor_mode);
	}
	return rtn;
}

extern int itx_cam_get_mf_info(int ch, NFIPCamMFInfo* info)
{
#if 0
	char time_str[64];
	int current = 0;
	mtable *runtime = get_runtime();

	itx_cam_get_timezone_info(ch, &time_str[0]);
	itx_cam_get_current_info(ch, &current);

	info->ipaddr = htonl(runtime[ch].sys.ipaddr);
	memcpy(info->macaddr, runtime[ch].sys.macaddr, 6);
	strcpy(info->name, runtime[ch].sys.model);
	if (runtime[ch].sys.swver2[0] != '\0')
	{
		strcpy(info->swver, runtime[ch].sys.swver2);
	}
	else
	{
		strcpy(info->swver, runtime[ch].sys.swver);
	}
	strcpy(info->timeinfo, time_str);
	info->current = current;

	return IPCAM_SETUP_RTN_DONE;
#endif
}

extern int itx_cam_get_compxy(int ch, int *x_pos, int *y_pos)
{
	const char set_api_raw[] =
			"action=get_setup&menu=video.comp_center";

	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;


	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 1024, set_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	IPCAM_DBG(MINOR, "SEND\n%s\n", sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("recv");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	printf("[%s] RECV\n%s\n", __FUNCTION__, sock_buf);
#endif

	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_x[]		= "offsetx=";
		const char f_y[]		= "offsety=";
		char temp_buf[4];

		/* offsetx */
		memset(temp_buf, 0x00, 4);

		s = sock_buf;
		p = strstr(s, f_x);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_x);

		temp_buf[0] = *s;
		*x_pos = atoi(temp_buf);

		/* offsety */
		memset(temp_buf, 0x00, 4);
		s = sock_buf;
		p = strstr(s, f_y);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_y);

		temp_buf[0] = *s;
		*y_pos = atoi(temp_buf);
	}
	return IPCAM_SETUP_RTN_DONE;
}
extern int itx_cam_set_compx(int ch, int x_pos)
{
	const char set_api_raw[] =
			"action=set_setup&menu=video.comp_horiz&offsetx=%d";

	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;


	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 1024, set_api_raw, x_pos);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	printf("[%s] SEND\n%s\n", __FUNCTION__, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("recv");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	printf("[%s] RECV\n%s\n", __FUNCTION__, sock_buf);
#endif

	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	return IPCAM_SETUP_RTN_DONE;
}
extern int itx_cam_set_compy(int ch, int y_pos)
{
	const char set_api_raw[] =
			"action=set_setup&menu=video.comp_vert&offsety=%d";

	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;


	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 1024, set_api_raw, y_pos);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	printf("[%s] SEND\n%s\n", __FUNCTION__, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("recv");
		close(sock);
		sock = (-1);
		return (0);
	}

//#if PRINT_HTTP_API_SEND
#if 1
	printf("[%s] RECV\n%s\n", __FUNCTION__, sock_buf);
#endif

	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	return IPCAM_SETUP_RTN_DONE;
}






static int _ssl_get_model_info_by_sysdb(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();
	unsigned char cal_mac[3];

	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = runtime[cam_id].admin_http;

	{
		gchar key_u[64];
		gchar key_p[64];
		gchar *u,*p;
		snprintf(key_u, 64, "cam.logininfo.L%d.id", cam_id);
		snprintf(key_p, 64, "cam.logininfo.L%d.pwd", cam_id);
		u = nf_sysdb_get_str_nocopy(key_u);
		p = nf_sysdb_get_str_nocopy(key_p);
		strncpy(username, u, 64);
		strncpy(password, p, 64);
		strncpy(runtime[cam_id].username, u, 64);
		strncpy(runtime[cam_id].password, p, 64);
	}

#if PRINT_HTTP_API_SEND
	printf("[%s] CH%02d user(%s) pass(%s)\n",
			__FUNCTION__, cam_id,
			runtime[cam_id].username, runtime[cam_id].password
	);
#endif

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}
	if (ctx == NULL)
	{
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
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
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

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			printf("[%s] ERROR: HTTP Auth fail(%s)\n", __FUNCTION__, sock_buf);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return (1);
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

			{
				const char f_name[]		= "hwver=";
				const char f_swver[]	= "swver=";
				const char f_swver2[]	= "swver2=";
				const char f_vendor[]	= "brand=";
				const char f_mac[]		= "macaddr=";
				const char f_std[]		= "stdver=";
				const char f_sdk[]		= "httpapi_ver=";

				char *s = NULL;
				char *p = NULL;
				char buf[128];


				/* mac parsing */
				s = recv_msg;
				p = strstr(s, f_mac);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_mac);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 6);
				memcpy(buf, s, (size_t)(p - s));
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
						&info_buf->mac[0], &info_buf->mac[1],
						&info_buf->mac[2], &info_buf->mac[3],
						&info_buf->mac[4], &info_buf->mac[5]);

				/* model name parsing */
				s = recv_msg;
				p = strstr(s, f_name);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_name);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->name, buf);

				/* fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_swver);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->swver, buf);

				/* vendor parsing */
				s = recv_msg;
				p = strstr(s, f_vendor);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_vendor);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->vendor, buf);

				/* stdver parsing */
				s = recv_msg;
				p = strstr(s, f_std);
				if (p == NULL) 
				{
					memset(info_buf->stdver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_std);
					p = strstr(s, "&");
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->stdver, buf);
				}

				/* sdkver parsing */
				s = recv_msg;
				p = strstr(s, f_sdk);
				if (p == NULL) 
				{
					memset(info_buf->sdkver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_sdk);
					p = strstr(s, "&");
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->sdkver, buf);
				}

				/* vendor fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver2);
				if (p == NULL)
				{
					//memset(info_buf->swver2, 0x00, 64);
				}
				else
				{
					//s = p + strlen(f_swver2);
					//strcpy(info_buf->swver2, s);
				}
			}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

static int _ssl_get_model_info_raw_digest(
	cam_model_info* info_buf,
	unsigned int ip,
	unsigned short port,
	char *u,
	char *p,
	char *rbuf
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

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
	//const char *method = "POST";
	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e;

	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


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

	//snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, port);
	//snprintf(uri, 128, "/cgi-bin/action.fcgi");
	snprintf(uri, 128, "/cgi-bin/action.fcgi?action=get_setup&menu=system.info");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_get, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_get, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_get, ip_str, auth_str);

	IPCAM_DBG(MAJOR, "start addr(%s:%d) login(%s:*)\n", ip_str, port, u);

	memset(&sin, 0x00, sizeof(sin));
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
		IPCAM_DBG(ERROR, "SSL connect fail addr(%s:%d) login(%s:*)\n", ip_str, port, u);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (-1);
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG addr(%s:%d) - %s\n", ip_str, port, sock_buf);
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
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV 1 MSG addr(%s:%d) - %s\n", ip_str, port, sock_buf);
#endif

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		IPCAM_DBG(WARN, "HTTP error addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (1);
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(ERROR, "Non-chunked type addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
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
			{
				const char f_name[]		= "hwver=";
				const char f_swver[]	= "swver=";
				const char f_swver2[]	= "swver2=";
				const char f_vendor[]	= "brand=";
				const char f_mac[]		= "macaddr=";
				const char f_std[]		= "stdver=";
				const char f_sdk[]		= "httpapi_ver=";

				char *s = NULL;
				char *p = NULL;
				char buf[128];
				
				char *split = "\n"; // SWIPXXP-790 , for GET
				//char *split = split; // for POST

				/* mac parsing */
				s = recv_msg;
				p = strstr(s, f_mac);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_mac);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 6);
				memcpy(buf, s, (size_t)(p - s));
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
						&info_buf->mac[0], &info_buf->mac[1],
						&info_buf->mac[2], &info_buf->mac[3],
						&info_buf->mac[4], &info_buf->mac[5]);

				/* model name parsing */
				s = recv_msg;
				p = strstr(s, f_name);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_name);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->name, buf); //(SWIPXXP-791)
				strncpy(info_buf->name, buf, strlen(buf)-1);

				/* fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_swver);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->swver, buf);
				strncpy(info_buf->swver, buf, strlen(buf)-1);
				// SWIPXXP-790 Exception
				if(_is_low_firmware_novatek_cam(buf))
				{
					printf("[khkh] THIS FIRMWARE(%s) DOES NOT SUPPORT HTTPS DIGEST, PLEASE UPGRADE THE FIRMWARE \n",info_buf->swver);
					_release_resource(&sock, NULL, &ssl, &ctx);
					return -3;
				}
				else
					printf("[khkh] THIS FIRMWARE(%s) SUPPORT HTTPS DIGEST !!\n", info_buf->swver);


				/* vendor parsing */
				s = recv_msg;
				p = strstr(s, f_vendor);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_vendor);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->vendor, buf);
				strncpy(info_buf->vendor, buf, strlen(buf)-1);

				/* stdver parsing */
				s = recv_msg;
				p = strstr(s, f_std);
				if (p == NULL) 
				{
					memset(info_buf->stdver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_std);
					p = strstr(s, split);
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					//strcpy(info_buf->stdver, buf);
					strncpy(info_buf->stdver, buf, strlen(buf)-1);
				}

				/* sdkver parsing */
				s = recv_msg;
				p = strstr(s, f_sdk);
				if (p == NULL) 
				{
					memset(info_buf->sdkver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_sdk);
					p = strstr(s, split);
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					//strcpy(info_buf->sdkver, buf);
					strncpy(info_buf->sdkver, buf, strlen(buf)-1);
				}

				/* vendor fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver2);
				if (p == NULL)
				{
					//memset(info_buf->swver2, 0x00, 64);
				}
				else
				{
					//s = p + strlen(f_swver2);
					//strcpy(info_buf->swver2, s);
#if 0
					p = strstr(s, split);
					if (p == NULL)
					{
						p == strstr(s, "\r\n");
						if (p == NULL)
						{
							return 0;
						}
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->swver2, buf);
#endif
				}
			}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

static int _ssl_get_model_info_raw(
	cam_model_info* info_buf,
	unsigned int ip,
	unsigned short port,
	char* u, char* p
)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	unsigned char cal_mac[3];

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	snprintf(ip_str, 16, "%d.%d.%d.%d",
			(ip&0xff), (ip&0xff00)>>8, (ip&0xff0000)>>16, (ip&0xff000000)>>24);
	http_port = port;
	strncpy(username, u, 64);
	strncpy(password, p, 64);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	// SWIPXXP-790 exception
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_get, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_get, ip_str, auth_str);

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
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 MSG - %s\n", sock_buf);
#endif

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			if (strstr(sock_buf, "WWW-Authenticate: Digest") != NULL)
			{
				IPCAM_DBG(MINOR, "use DIGEST authentication addr(%s:%d) login(%s:*)\n", ip_str, port, u);
				_release_resource(&sock, NULL, &ssl, &ctx);
				len = _ssl_get_model_info_raw_digest(info_buf, ip, port, u, p, sock_buf);
				return len;
			}
			_release_resource(&sock, NULL, &ssl, &ctx);
			return (1);
		}
		IPCAM_DBG(WARN, "HTTP error addr(%s:%d) login(%s:*)\n%s\n", ip_str, port, u, sock_buf);
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

			{
				const char f_name[]		= "hwver=";
				const char f_swver[]	= "swver=";
				const char f_swver2[]	= "swver2=";
				const char f_vendor[]	= "brand=";
				const char f_mac[]		= "macaddr=";
				const char f_std[]		= "stdver=";
				const char f_sdk[]		= "httpapi_ver=";

				char *s = NULL;
				char *p = NULL;
				char buf[128];

				char *split = "\n"; // SWIPXXP-790 , for GET
				//char *split = "&"; // for POST

				/* mac parsing */
				s = recv_msg;
				p = strstr(s, f_mac);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_mac);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 6);
				memcpy(buf, s, (size_t)(p - s));
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
						&info_buf->mac[0], &info_buf->mac[1],
						&info_buf->mac[2], &info_buf->mac[3],
						&info_buf->mac[4], &info_buf->mac[5]);

				/* model name parsing */
				s = recv_msg;
				p = strstr(s, f_name);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_name);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->name, buf); (SWIPXXP-791)
				strncpy(info_buf->name, buf, strlen(buf)-1);

				/* fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_swver);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->swver, buf);
				strncpy(info_buf->swver, buf, strlen(buf)-1);
				// SWIPXXP-790 Exception
				if(_is_low_firmware_novatek_cam(buf))
				{
					printf("[khkh] THIS FIRMWARE(%s) DOES NOT SUPPORT HTTPS BASIC, PLEASE UPGRADE THE FIRMWARE \n", info_buf->swver);
					_release_resource(&sock, NULL, &ssl, &ctx);
					return -3;
				}
				else
					printf("[khkh] THIS FIRMWARE(%s) SUPPORT HTTPS BASIC !!\n", info_buf->swver);


				/* vendor parsing */
				s = recv_msg;
				p = strstr(s, f_vendor);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_vendor);
				p = strstr(s, split);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				//strcpy(info_buf->vendor, buf);
				strncpy(info_buf->vendor, buf, strlen(buf)-1);

				/* stdver parsing */
				s = recv_msg;
				p = strstr(s, f_std);
				if (p == NULL) 
				{
					memset(info_buf->stdver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_std);
					p = strstr(s, split);
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					//strcpy(info_buf->stdver, buf);
					strncpy(info_buf->stdver, buf, strlen(buf)-1);
				}

				/* sdkver parsing */
				s = recv_msg;
				p = strstr(s, f_sdk);
				if (p == NULL) 
				{
					memset(info_buf->sdkver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_sdk);
					p = strstr(s, split);
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					//strcpy(info_buf->sdkver, buf);
					strncpy(info_buf->sdkver, buf, strlen(buf)-1);
				}

				/* vendor fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver2);
				if (p == NULL)
				{
					//memset(info_buf->swver2, 0x00, 64);
				}
				else
				{
					//s = p + strlen(f_swver2);
					//strcpy(info_buf->swver2, s);
#if 0
					p = strstr(s, split);
					if (p == NULL)
					{
						p == strstr(s, "\r\n");
						if (p == NULL)
						{
							return 0;
						}
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->swver2, buf);
#endif
				}
			}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	//_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

static int _ssl_get_model_info_by_backdoor(cam_model_info* info_buf, int cam_id)
{
	const char get_model_raw[] = "action=get_setup&menu=system.info";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();
	unsigned char cal_mac[3];

	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
#if 0
	if (nf_sysman_get_fwver_vendor() == 30)
	{
		http_port = 8082;
	}
	else
	{
		http_port = 80;
	}
#else
		http_port = 8082;
#endif
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return (-1);
	}

	if (ctx == NULL)
	{
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
			close(sock);
			//_release_resource(&sock, NULL, NULL, &ctx);
			return (-1);
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
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

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
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

			{
				const char f_name[]		= "hwver=";
				const char f_swver[]	= "swver=";
				const char f_swver2[]	= "swver2=";
				const char f_vendor[]	= "brand=";
				const char f_mac[]		= "macaddr=";
				const char f_std[]		= "stdver=";
				const char f_sdk[]		= "httpapi_ver=";

				char *s = NULL;
				char *p = NULL;
				char buf[128];


				/* mac parsing */
				s = recv_msg;
				p = strstr(s, f_mac);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_mac);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 6);
				memcpy(buf, s, (size_t)(p - s));
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
						&info_buf->mac[0], &info_buf->mac[1],
						&info_buf->mac[2], &info_buf->mac[3],
						&info_buf->mac[4], &info_buf->mac[5]);

				/* model name parsing */
				s = recv_msg;
				p = strstr(s, f_name);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_name);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->name, buf);

				/* fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_swver);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->swver, buf);

				/* vendor parsing */
				s = recv_msg;
				p = strstr(s, f_vendor);
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				s = p + strlen(f_vendor);
				p = strstr(s, "&");
				if (p == NULL)
				{
					_release_resource(&sock, NULL, &ssl, &ctx);
					return 0;
				}
				memset(buf, 0x00, 128);
				memcpy(buf, s, (size_t)(p - s));
				strcpy(info_buf->vendor, buf);

				/* stdver parsing */
				s = recv_msg;
				p = strstr(s, f_std);
				if (p == NULL) 
				{
					memset(info_buf->stdver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_std);
					p = strstr(s, "&");
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->stdver, buf);
				}

				/* sdkver parsing */
				s = recv_msg;
				p = strstr(s, f_sdk);
				if (p == NULL) 
				{
					memset(info_buf->sdkver, 0x00, 64);
				}
				else
				{
					s = p + strlen(f_sdk);
					p = strstr(s, "&");
					if (p == NULL)
					{
						_release_resource(&sock, NULL, &ssl, &ctx);
						return 0;
					}
					memset(buf, 0x00, 128);
					memcpy(buf, s, (size_t)(p - s));
					strcpy(info_buf->sdkver, buf);
				}

				/* vendor fw version parsing */
				s = recv_msg;
				p = strstr(s, f_swver2);
				if (p == NULL)
				{
					//memset(info_buf->swver2, 0x00, 64);
				}
				else
				{
					//s = p + strlen(f_swver2);
					//strcpy(info_buf->swver2, s);
				}
			}
	}

	_release_resource(&sock, NULL, &ssl, &ctx);
	return (3);
}

static int _ssl_set_osd_off(int cam_id)
{
	const char set_osd_off_raw[] =
			"action=set_setup&menu=live.setup&"
			"proto=tcp_rtsp&buffer_time=0&event_state=off"; // (SWIPXXP-479)
			//"proto=tcp_rtsp&buffer_time=0&date=off&resolution=off&event_state=off";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_osd_off_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_CUSTOM0, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock_buf);

	return len;
}

static int _common_set_osd_off(int cam_id)
{
	const char set_osd_off_raw[] =
			"action=set_setup&menu=live.setup&"
			"proto=tcp_rtsp&buffer_time=0&event_state=off"; // (SWIPXXP-479)
			//"proto=tcp_rtsp&buffer_time=0&date=off&resolution=off&event_state=off";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	unsigned short http_port;
	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_osd_off_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_CUSTOM0, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_CUSTOM0, sock_buf);

	return len;
}

static int _ssl_set_vcodec(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
			"action=set_setup&menu=video.ctl_codec&"
			"codec0=%s&resolution0=%s&bitctrl0=%s&bitavr0=%d&fps0=%s&"
			"codec1=%s&resolution1=%s&bitctrl1=%s&bitavr1=%d&fps1=%s&"
			"ff_mode=%d&mirror_mode=%s&bandon=no&bandwidth=5000&"
			"gopsize0=%s&gopsize1=%s&jpegqual=80";

	const char *mirr_str[] = { "none", "none", "h_mirror", "v_mirror", "hv_mirror" };

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	char *mirr;

	int len = 0;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];
	snprintf(http_api, 512, set_vcodec_raw,
			get_vcodec_string(info_set->vcodec.vcodec[0]),
			get_resol_string(info_set->vcodec.resolution[0]), 
			get_bitctrl_string(info_set->vcodec.bitctrl[0]),
			info_set->vcodec.bitrate[0],
			get_fps_string(info_set->vcodec.fps[0]),
			get_vcodec_string(info_set->vcodec.vcodec[1]),
			get_resol_string(info_set->vcodec.resolution[1]), 
			get_bitctrl_string(info_set->vcodec.bitctrl[1]),
			info_set->vcodec.bitrate[1],
			get_fps_string(info_set->vcodec.fps[1]),
			info_set->vcodec.af, mirr,
			get_fps_string(info_set->vcodec.fps[0]),
			get_gop_string(info_set->vcodec.fps[1]));

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VCODEC, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock_buf);

	return len;
}

static int _ti368_set_vcodec(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
			"action=set_setup&menu=video.ctl_codec&"
			"codec0=h264&resolution0=%s&bitctrl0=cbr&bitavr0=%d&fps0=%s&"
			"codec1=h264&resolution1=%s&bitctrl1=cbr&bitavr1=%d&fps1=%s&"
			"ff_mode=%d&mirror_mode=%s&bandon=no&bandwidth=5000&"
			"gopsize0=%s&gopsize1=%s&jpegqual=80";

	const char set_vcodec_raw_inx[] =
		"action=set_setup&menu=video.ctl_codec&"
		"codec0=%s&"
		"codec1=%s&"
		"codec2=%s&"
		"resolution0=%s&"
		"resolution1=%s&"
		"resolution2=%s&"
		"bitctrl0=%s&"
		"bitctrl1=%s&"
		"bitctrl2=%s&"
		"bitavr0=%d&"
		"bitavr1=%d&"
		"bitavr2=%d&"
		"fps0=%s&"
		"fps1=%s&"
		"fps2=%s&"
		"bfps0=%s&"
		"bfps1=%s&"
		"bfps2=%s&"
		"bgop0=%s&"
		"bgop1=%s&"
		"bgop2=%s&"
		"gopsize0=%s&"
		"gopsize1=%s&"
		"gopsize2=%s&"
		"jpegqual=%d&"
		"ff_mode=%d&"
		"mirror_mode=%s&"
		"bandon=%s&"
		"bandwidth=%d&"
		"boost_mode=%s&"
		"capture_mode=%s";


	const char *mirr_str[] = { "none", "none", "h_mirror", "v_mirror", "hv_mirror" };
	const char *capture_str[] = { "", "0", "1", "2", "3", "4"};

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	char *mirr;
	char *capture;

	int len = 0;

	mtable *runtime = get_runtime();
	int sdk_major,sdk_type,sdk_subtype,sdk_minor;

	sscanf(runtime[cam_id].sys.sdkver, "%d.%d.%d.%d", &sdk_major, &sdk_type, &sdk_subtype, &sdk_minor);


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];
	capture = capture_str[get_index_from_bitmask(info_set->vcodec.capture)];

	if(strncmp(runtime[cam_id].sys.stdver, "IN",2) == 0 || (sdk_type == 12)) 
	{
		if(strstr(runtime[cam_id].sys.stdver, "-4007") != NULL)
		{
			capture = capture_str[1];	
		}

		snprintf(http_api, 512, set_vcodec_raw_inx,
				get_vcodec_string(info_set->vcodec.vcodec[0]),		// VC0 Video Stream Compression Format
				get_vcodec_string(info_set->vcodec.vcodec[1]),		// VC1 Video Stream Compression Format
				"none",											// VC2 Video Stream Compression Format

				get_resol_string(info_set->vcodec.resolution[0]), 	// VC0 Video Stream Resolution
				get_resol_string(info_set->vcodec.resolution[1]), 	// VC1 Video Stream Resolution
				"640x360",											// VC1 Video Stream Resolution

				get_bitctrl_string(info_set->vcodec.bitctrl[0]),	// VC0 Bitrate Contorl
				get_bitctrl_string(info_set->vcodec.bitctrl[1]),	// VC1 Bitrate Contorl
				"cbr",												// VC2 Bitrate Contorl

				info_set->vcodec.bitrate[0],						// VC0 Compressed Image Quality
				info_set->vcodec.bitrate[1],						// VC1 Compressed Image Quality
				1000,												// VC2 Compressed Image Quality

				get_fps_string(info_set->vcodec.fps[0]),			// VC0 FPS
				get_fps_string(info_set->vcodec.fps[1]),			// VC1 FPS
				"1",												// VC2 FPS

				get_fps_string(info_set->vcodec.fps[0]),			// VC0 Boost FPS
				get_fps_string(info_set->vcodec.fps[1]),			// VC1 Boost FPS
				"1",												// VC2 Boost FPS

				get_gop_string(info_set->vcodec.fps[0]),			// VC0 BGOP Keyframe Interva;
				get_gop_string(info_set->vcodec.fps[1]),			// VC1 BGOP Keyframe Interva;
				"1",												// VC2 BGOP Keyframe Interva;

				get_fps_string(info_set->vcodec.fps[0]),			// VC0 GOP Size
				get_gop_string(info_set->vcodec.fps[1]),			// VC1 GOP Size
				"1",													// VC2 GOP Size

				80,													// mjpeg quality
				info_set->vcodec.af,								// ff_mode (unit: Hz)
				mirr,												// Mirroring Operation
				"no",												// Limitation of overall bandwidth
				5000,												// bandwidth ?
				"no",												// boost mode
				capture												// catprue mode
					);
		printf("[CH:%d] %s\n",cam_id, http_api);
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
				info_set->vcodec.af, mirr,
				get_fps_string(info_set->vcodec.fps[0]),
				get_gop_string(info_set->vcodec.fps[1]));
		printf("[CH:%d] %s\n",cam_id, http_api);
	}





	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VCODEC, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock_buf);

	return len;
}

extern int cam_set_vcodec_a2(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
			"action=set_setup&menu=video.codec&"
			"codec0=h264&resolution0=%s&bitctrl0=cbr&bitavr0=%d&fps0=%s&"
			"codec1=h264&resolution1=%s&bitctrl1=cbr&bitavr1=%d&fps1=%s&"
			"ff_mode=%d&mirror_mode=%s&bandon=no&bandwidth=5000&"
			"gopsize0=%s&gopsize1=%s&jpegqual=80";

	const char *mirr_str[] = { "none", "none", "h_mirror", "v_mirror", "hv_mirror" };

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char *mirr;
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	int fps1, fps2;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];

	switch (info_set->vcodec.fps[0])
	{
		case NF_IPCAM_FPS_120:
			fps1 = NF_IPCAM_FPS_125;
			break;
		case NF_IPCAM_FPS_70:
			fps1 = 75;
			break;
		case NF_IPCAM_FPS_60:
			fps1 = 63;
			break;
		default:
			fps1 = info_set->vcodec.fps[0];
			break;
	}
	switch (info_set->vcodec.fps[1])
	{
		case NF_IPCAM_FPS_120:
			fps2 = NF_IPCAM_FPS_125;
			break;
		case NF_IPCAM_FPS_70:
			fps2 = 75;
			break;
		case NF_IPCAM_FPS_60:
			fps2 = 63;
			break;
		default:
			fps2 = info_set->vcodec.fps[0];
			break;
	}
#if 0
	if (info_set->vcodec.fps[0] == NF_IPCAM_FPS_120)
	{
		fps1 = NF_IPCAM_FPS_125;
	}
	else
	{
		fps1 = info_set->vcodec.fps[0];
	}
	if (info_set->vcodec.fps[1] == NF_IPCAM_FPS_120)
	{
		fps2 = NF_IPCAM_FPS_125;
	}
	else
	{
		fps2 = info_set->vcodec.fps[1];
	}
#endif

	if (info_set->vcodec.af == 60)
	{
		snprintf(http_api, 512, set_vcodec_raw,
				get_resol_string(info_set->vcodec.resolution[0]), 
				info_set->vcodec.bitrate[0],
				get_fps_string(fps1),
				get_resol_string(info_set->vcodec.resolution[1]), 
				info_set->vcodec.bitrate[1],
				get_fps_string(fps2),
				info_set->vcodec.af, mirr,
				"15", "15");
	}
	else if (info_set->vcodec.af == 50)
	{
		snprintf(http_api, 512, set_vcodec_raw,
				get_resol_string(info_set->vcodec.resolution[0]), 
				info_set->vcodec.bitrate[0],
				get_fps_string(fps1),
				get_resol_string(info_set->vcodec.resolution[1]), 
				info_set->vcodec.bitrate[1],
				get_fps_string(fps2),
				info_set->vcodec.af, mirr,
				"12", "12");
	}
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VCODEC, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock_buf);

	return len;
}

extern int cam_set_vcodec_d1(cam_info* info_set, int cam_id)
{
	const char set_vcodec_raw[] =
			"action=set_setup&menu=video.codec&"
			"codec0=h264&resolution0=%s&bitctrl0=cbr&bitavr0=%d&fps0=%s&"
			"codec1=h264&resolution1=%s&bitctrl1=cbr&bitavr1=%d&fps1=%s&"
			"ff_mode=%d&mirror_mode=%s&bandon=no&bandwidth=5000&"
			"gopsize0=%s&gopsize1=%s&jpegqual=80";

	const char *mirr_str[] = { "none", "none", "h_mirror", "v_mirror", "hv_mirror" };

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char *mirr;
	unsigned short http_port;

	int len;
	int sock;
	struct sockaddr_in sin;

	int fps1, fps2;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	mirr = mirr_str[get_index_from_bitmask(info_set->vcodec.mirror)];

	switch (info_set->vcodec.fps[0])
	{
		case NF_IPCAM_FPS_120:
			fps1 = NF_IPCAM_FPS_125;
			break;
		case NF_IPCAM_FPS_70:
			fps1 = 75;
			break;
		case NF_IPCAM_FPS_60:
			fps1 = 63;
			break;
		default:
			fps1 = info_set->vcodec.fps[0];
			break;
	}
	switch (info_set->vcodec.fps[1])
	{
		case NF_IPCAM_FPS_120:
			fps2 = NF_IPCAM_FPS_125;
			break;
		case NF_IPCAM_FPS_70:
			fps2 = 75;
			break;
		case NF_IPCAM_FPS_60:
			fps2 = 63;
			break;
		default:
			fps2 = info_set->vcodec.fps[0];
			break;
	}
#if 0
	if (info_set->vcodec.fps[0] == NF_IPCAM_FPS_120)
	{
		fps1 = NF_IPCAM_FPS_125;
	}
	else
	{
		fps1 = info_set->vcodec.fps[0];
	}
	if (info_set->vcodec.fps[1] == NF_IPCAM_FPS_120)
	{
		fps2 = NF_IPCAM_FPS_125;
	}
	else
	{
		fps2 = info_set->vcodec.fps[1];
	}
#endif

	if (info_set->vcodec.af == 60)
	{
		snprintf(http_api, 512, set_vcodec_raw,
				"704x480",
				info_set->vcodec.bitrate[0],
				get_fps_string(fps1),
				"352x240",
				info_set->vcodec.bitrate[1],
				get_fps_string(fps2),
				info_set->vcodec.af, mirr,
				"15", "15");
	}
	else if (info_set->vcodec.af == 50)
	{
		snprintf(http_api, 512, set_vcodec_raw,
				"704x480",
				info_set->vcodec.bitrate[0],
				get_fps_string(fps1),
				"352x240",
				info_set->vcodec.bitrate[1],
				get_fps_string(fps2),
				info_set->vcodec.af, mirr,
				"12", "12");
	}
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VCODEC, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_VCODEC, sock_buf);

	return len;
}


static int _ssl_set_acodec(cam_info* info_set, int cam_id)
{
	const char set_acodec_raw[] =
			"action=set_setup&menu=audio.setup&"
			"audioon=%s&codec=%s&mic_volume=%d&spk_volume=%d";
	const char* onoff[] = { "no", "yes" };
	const char* acodec[] = { "ulaw", "alaw" };

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_acodec_raw,
			onoff[info_set->acodec.audio_tx], acodec[info_set->acodec.audio_codec],
			info_set->acodec.mic_volume, info_set->acodec.speaker_volume);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ACODEC, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ACODEC, sock_buf);

	return len;
}

static int _common_set_acodec(cam_info* info_set, int cam_id)
{
	const char set_acodec_raw[] =
			"action=set_setup&menu=audio.setup&"
			"audioon=%s&codec=%s&mic_volume=%d&spk_volume=%d";
	const char* onoff[] = { "no", "yes" };
	const char* acodec[] = { "ulaw", "alaw" };

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_acodec_raw,
			onoff[info_set->acodec.audio_tx], acodec[info_set->acodec.audio_codec],
			info_set->acodec.mic_volume, info_set->acodec.speaker_volume);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ACODEC, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ACODEC, sock_buf);

	return len;
}

static int _ssl_set_image(image_info* info_set, int cam_id)
{
	const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto","auto_m", "auto", "manual", "auto", "auto_m", "manual", "","","","","auto_m", "auto", "manual", "auto_m"};
	const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
	const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	const char *dnn_mode_str[] = { "auto", "day", "night", "schedule", "", "", "","auto", "day", "night", "auto", "day", "night", "schedule", "auto", "on", "off", "external" };
	const char *dnn_det_time[] = 
		{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
	const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
	const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
	// MAX Shutter Str modify
	const char *max_shutter_str[] = { "shut_0", "shut_1", "shut_2", "shut_3", "shut_4", "shut_5", "shut_6", "shut_7", "shut_8", "shut_9", "shut_10", "shut_11", "shut_12", "shut_13", "shut_14", "shut_15", "shut_16" };
	const char *base_shutter_str[] = { "bshut_0", "bshut_1", "bshut_2", "bshut_3", "bshut_4", "bshut_5", "bshut_6", "bshut_7", "bshut_8", "bshut_9", 
										"bshut_10", "bshut_11", "bshut_12", "bshut_13", "bshut_14", "bshut_15", "bshut_16"};
	const char *ff_mode_str[] = { "60", "50", "off" };
	const char *slow_shutter_str[] = {"off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8"};
	const char *wdr_mode_str[] = { "off", "", "", "off", "on", "low", "middle", "high", "low", "mid", "high", "on", "low", "mid", "high", "on", "low", "middle", "high", "low", "low", "mid", "high", "on", "off", "on", "low", "mid", "high" };
	
	const char *blc_mode_str[] = { "off", "on", "", "adap", "zone" };
	const char *blc_zone_str[] = { "", "", "", "", "lower", "middle", "upper", "left", "right" };
	
	// 3D DNR
	const char *dnr_ctr[] = {"off", "off", "auto","auto","auto", "manual","manual","manual", "auto"};
	const char *dnr_level[] = {"low","low","low", "mid", "high", "low",   "mid",   "high",   "smart"};

	// AdaptiveIR
	const char *adaptive_ir [] = { "off", "on"};
	const char *defog_str[] = {"off", "on", "low", "mid", "high", "off", "low", "mid", "high", "off", "low", "mid", "high"};

	const char set_image_raw[] =
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=%s&blc_ctrl=%s&dnn_mode=%s&dnn_det_time=%s&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d%s";

	const char set_image_raw_v104[] =
			"action=set_setup&menu=video.camera_x&"
			"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
			"max_shutter=%s&"				// added
			"iris_mode=%s&blc_ctrl=%s&"
			//"blc_zone_ctrl=%s&ff_mode=%s&"
			"blc_zone_ctrl=%s&anti_mode=%s&"	// added
			"dnn_mode=%s&dnn_det_time=%s&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d&"
			//"dnn_set_hour1=%d&dnn_set_min1=%d&dnn_set_hour2=%d&dnn_set_min2=%d&"
			"dnn_set_hour1=7&dnn_set_min1=0&dnn_set_hour2=20&dnn_set_min2=0"	// added
			"%s";
	// ITX INx HTTP API V2.3_20140612 Camera Setup Argument
	const char set_image_raw_inx[] =
		"action=set_setup&menu=video.camera_x&"
		"ae_mode=%s&" 			// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
		"me_agc=%d&"			// AGC Gain (ME Only)
		"me_shutter=%d&"		// Sutter Speed( ME Only)
		"ss_mode=%s&"			// Slow Shutter (off|x2|x4|x8)
		"dnr_ctrl=%s&"			// Select 3D DNR Function (off|auto|manual)
		"dnr_manual=%s&"		// Select 3D DNR Strength (low|mid|high)
		"wdr_ctrl=%s&"			// Select WDR Function (off|low|mid|high)
		"max_agc=%s&"			// MAX AGC value (24|30|36|42)
		"max_shutter=%s&"		// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
		"iris_mode=%s&"			// P-Iris Control (manual|auto|boq|dof)
		"anti_mode=%s&"			// Anti-Flicker (50|60|off)
		"blc_ctrl=%s&"			// BLC Control (off|adap|zone)
		"blc_zone_ctrl=%s&"		// BLC Zone Contorl (lower|middle|upper|left|right)
		"dnn_mode=%s&"			// Day and Night Mode (auto|day|night|schedule)
		"dnn_det_time=%s&"		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
		"dnn_set_hour1=%d&"		// Hour of the start time
		"dnn_set_min1=%d&"		// Minute of the start time
		"dnn_set_hour2=%d&"		// Hour of the end time
		"dnn_set_min2=%d&"		// Minute of the end time
		"adap_ir=%s&"			// Adaptive IR function (on|off)
		"awb_mode=%s&"			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
		"mwb_mode=%s&"			// White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
		"img_sharp=%d&"			// Image Sharpness Filter Strength (1~15)
		"img_bright=%d&"		// Image Brightness (0~30:15)
		"img_contrast=%d&"		// Image Contrast (0~30:15)
		"img_color=%d&"			// Image Color (0~30:15)
		"img_hue=%d&"			// Image HUE (0~30:15)                                                               
		"defog_ctrl=%s&"			// DEFOG (OFF|LOW|MID|HIGH)                                                               
		"base_shutter=%s";		// Base Shutter Speed


	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;
	int sdk_major,sdk_type,sdk_subtype,sdk_minor;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	sscanf(runtime[cam_id].sys.sdkver, "%d.%d.%d.%d", &sdk_major, &sdk_type, &sdk_subtype, &sdk_minor);

	if (sdk_major < 6)
	{
		// FIXME this..ti camera
		if (strcmp(runtime[cam_id].sys.sdkver, "1.0.0.4") >= 0 && strstr(runtime[cam_id].sys.stdver, "VPR") == NULL && strstr(runtime[cam_id].sys.stdver, "WR") == NULL)
		{
			snprintf(http_api, 512, set_image_raw_v104,
					ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
					slow_shutter_str[info_set->ss], magc_str[info_set->max_agc],
					max_shutter_str[info_set->max_shutter],
					offon_str[info_set->iris], blc_mode_str[min(info_set->blc, 4)],
					blc_zone_str[max(info_set->blc, 4)], /* off, adap -> send lower */
					ff_mode_str[info_set->ff_mode],
					dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
					awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
					info_set->sharpness, info_set->brightness, info_set->contrast,
					info_set->color, info_set->tint, "");
		}
		else
		{
			if (info_set->iris >= NF_IPCAM_IMAGE_PIRIS_AUTO &&
				info_set->iris <= NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
			{
				if (info_set->iris == NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING ||
					info_set->iris == NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
				{
					snprintf(http_api, 512, set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "&tracking=1");
				}
				else
				{
					snprintf(http_api, 512, set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "&tracking=0");
				}
			}
			else
			{
				snprintf(http_api, 512, set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "");
			}
		}
		if (strncmp(runtime[cam_id].sys.stdver, "IN", 2) == 0
			|| (sdk_type == 12 && sdk_minor >= 1) )	//hisilicon cam && base shutter supported 
		{
			snprintf(http_api, 512, set_image_raw_inx,
					ae_mode_str[info_set->ae], 				// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
					info_set->agc, 							// AGC Gain (ME Only)
					info_set->shutter,						// Sutter Speed( ME Only)
					slow_shutter_str[info_set->ss],			// Slow Shutter (off|x2|x4|x8)
					dnr_ctr[info_set->dnr_ctr] ,			// Select 3D DNR Function (off|auto|manual)
					dnr_level[info_set->dnr_ctr],			// Select 3D DNR Strength (low|mid|high)
					wdr_mode_str[info_set->wd],				// Select WDR Function (off|low|mid|high)
					magc_str[info_set->max_agc],			// MAX AGC value (24|30|36|42)
					max_shutter_str[info_set->max_shutter],	// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
					offon_str[info_set->iris],				// P-Iris Control (manual|auto|boq|dof)
					ff_mode_str[info_set->ff_mode],			// Anti-Flicker (50|60|off)
					blc_mode_str[min(info_set->blc, 4)],	// BLC Control (off|adap|zone)
					blc_zone_str[max(info_set->blc, 4)],	// BLC Zone Contorl (lower|middle|upper|left|right)
					dnn_mode_str[info_set->dnn], 			// Day and Night Mode (auto|day|night|schedule)
					dnn_det_time[info_set->dnn_time],		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
					7,										// Hour of the day start time
					0,										// Minute of the day start time
					18,										// Hour of the day end time
					0,										// Minute of the end time
					adaptive_ir[info_set->adaptive_ir],								// Adaptive IR function (on|off)
					awb_mode_str[info_set->awb],			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
					mwb_mode_str[info_set->mwb],			// Maual White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
					info_set->sharpness,					// Image Sharpness Filter Strength (1~15)
					info_set->brightness,					// Image Brightness (0~30:15)
					info_set->contrast,						// Image Contrast (0~30:15)
					info_set->color,						// Image Color (0~30:15)
					info_set->tint,							// Image HUE (0~30:15)                                                               
					defog_str[info_set->defog],			// DEFOG (OFF|LOW|MID|HIGH)
					base_shutter_str[info_set->base_shutter]// Base Shutter Speed
						);
		}
	}
	else	// sdk_major >= 6
	{
		snprintf(http_api, 512, set_image_raw_inx,
				ae_mode_str[info_set->ae], 				// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
				info_set->agc, 							// AGC Gain (ME Only)
				info_set->shutter,						// Sutter Speed( ME Only)
				slow_shutter_str[info_set->ss],			// Slow Shutter (off|x2|x4|x8)
				dnr_ctr[info_set->dnr_ctr] ,			// Select 3D DNR Function (off|auto|manual)
				dnr_level[info_set->dnr_ctr],			// Select 3D DNR Strength (low|mid|high)
				wdr_mode_str[info_set->wd],				// Select WDR Function (off|low|mid|high)
				magc_str[info_set->max_agc],			// MAX AGC value (24|30|36|42)
				max_shutter_str[info_set->max_shutter],	// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
				offon_str[info_set->iris],				// P-Iris Control (manual|auto|boq|dof)
				ff_mode_str[info_set->ff_mode],			// Anti-Flicker (50|60|off)
				blc_mode_str[min(info_set->blc, 4)],	// BLC Control (off|adap|zone)
				blc_zone_str[max(info_set->blc, 4)],	// BLC Zone Contorl (lower|middle|upper|left|right)
				dnn_mode_str[info_set->dnn], 			// Day and Night Mode (auto|day|night|schedule)
				dnn_det_time[info_set->dnn_time],		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
				7,										// Hour of the day start time
				0,										// Minute of the day start time
				18,										// Hour of the day end time
				0,										// Minute of the end time
				adaptive_ir[info_set->adaptive_ir],								// Adaptive IR function (on|off)
				awb_mode_str[info_set->awb],			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
				mwb_mode_str[info_set->mwb],			// Maual White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
				info_set->sharpness,					// Image Sharpness Filter Strength (1~15)
				info_set->brightness,					// Image Brightness (0~30:15)
				info_set->contrast,						// Image Contrast (0~30:15)
				info_set->color,						// Image Color (0~30:15)
				info_set->tint,							// Image HUE (0~30:15)                                                               
				defog_str[info_set->defog],			// DEFOG (OFF|LOW|MID|HIGH)
				base_shutter_str[info_set->base_shutter]// Base Shutter Speed
					);               
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_IMAGE, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_IMAGE, sock_buf);

	return len;
}

static int _ti368_set_image(image_info* info_set, int cam_id)
{
	const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto","auto_m", "auto", "manual", "auto", "auto_m", "manual", "","","","","auto_m", "auto", "manual" ,"auto_m"};
	const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
	const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	const char *dnn_mode_str[] = { "auto", "day", "night", "schedule", "", "", "","auto", "day", "night", "auto", "day", "night", "schedule", "auto", "on", "off", "external" };
	const char *dnn_det_time[] = 
		{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
	const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
	const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };

	// MAX Shutter Str modify
	const char *max_shutter_str[] = { "shut_0", "shut_1", "shut_2", "shut_3", "shut_4", "shut_5", "shut_6", "shut_7", "shut_8", "shut_9", "shut_10", "shut_11", "shut_12", "shut_13", "shut_14", "shut_15", "shut_16"};
	
	const char *base_shutter_str[] = { "bshut_0", "bshut_1", "bshut_2", "bshut_3", "bshut_4", "bshut_5", "bshut_6", "bshut_7", "bshut_8", "bshut_9", 
										"bshut_10", "bshut_11", "bshut_12", "bshut_13", "bshut_14", "bshut_15", "bshut_16"};
	const char *ff_mode_str[] = { "60", "50", "off", "", "", "", "", "off", "off", "off"};
	const char *slow_shutter_str[] = {"off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8", "off", "on_x2", "on_x4", "on_x8"};
	const char *wdr_mode_str[] = { "off", "", "", "off", "on", "low", "middle", "high", "low", "mid", "high", "on", "low", "mid", "high", "on", "low", "middle", "high", "low", "low", "mid", "high", "on", "off", "on", "low", "mid", "high" ,"on"};

	const char *blc_mode_str[] = { "off", "on", "", "adap", "zone" };
	const char *blc_zone_str[] = { "", "", "", "", "lower", "middle", "upper", "left", "right" };

	// 3D DNR Function
	const char *dnr_ctr[] = {"off", "off", "auto","auto","auto", "manual","manual","manual", "auto"};
	const char *dnr_level[] = {"low","low","low", "mid", "high", "low",   "mid",   "high",   "smart"};

	// AdaptiveIR
	const char *adaptive_ir [] = { "off", "on"};
	const char *defog_str[] = {"off", "on", "low", "mid", "high", "off", "low", "mid", "high", "off", "low", "mid", "high"};
	const char *colorvu_ctrl_str[] = { "", "manual", "auto"};

	const char set_image_raw[] =
		"action=set_setup&menu=video.camera&"
		"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
		"iris_mode=%s&blc_ctrl=%s&dnn_mode=%s&dnn_det_time=%s&"
		"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
		"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d%s";

	const char set_image_raw_v104[] =
			"action=set_setup&menu=video.camera_x&"
			"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
			"max_shutter=%s&"				// added
			"iris_mode=%s&blc_ctrl=%s&"
			//"blc_zone_ctrl=%s&ff_mode=%s&"
			"blc_zone_ctrl=%s&anti_mode=%s&"	// added
			"dnn_mode=%s&dnn_det_time=%s&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d&"
			//"dnn_set_hour1=%d&dnn_set_min1=%d&dnn_set_hour2=%d&dnn_set_min2=%d&"
			"dnn_set_hour1=7&dnn_set_min1=0&dnn_set_hour2=20&dnn_set_min2=0"	// added
			"%s";

	// ITX INx HTTP API V2.3_20140612 Camera Setup Argument
	const char set_image_raw_inx[] =
		"action=set_setup&menu=video.camera_x&"
		"ae_mode=%s&" 			// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
		"me_agc=%d&"			// AGC Gain (ME Only)
		"me_shutter=%d&"		// Sutter Speed( ME Only)
		"ss_mode=%s&"			// Slow Shutter (off|x2|x4|x8)
		"dnr_ctrl=%s&"			// Select 3D DNR Function (off|auto|manual)
		"dnr_manual=%s&"		// Select 3D DNR Strength (low|mid|high)
		"wdr_ctrl=%s&"			// Select WDR Function (off|low|mid|high)
		"max_agc=%s&"			// MAX AGC value (24|30|36|42)
		"max_shutter=%s&"		// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
		"iris_mode=%s&"			// P-Iris Control (manual|auto|boq|dof)
		"anti_mode=%s&"			// Anti-Flicker (50|60|off)
		"blc_ctrl=%s&"			// BLC Control (off|adap|zone)
		"blc_zone_ctrl=%s&"		// BLC Zone Contorl (lower|middle|upper|left|right)
		"dnn_mode=%s&"			// Day and Night Mode (auto|day|night|schedule)
		"dnn_det_time=%s&"		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
		"dnn_set_hour1=%d&"		// Hour of the start time
		"dnn_set_min1=%d&"		// Minute of the start time
		"dnn_set_hour2=%d&"		// Hour of the end time
		"dnn_set_min2=%d&"		// Minute of the end time
		"adap_ir=%s&"			// Adaptive IR function (on|off)
		"awb_mode=%s&"			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
		"mwb_mode=%s&"			// White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
		"img_sharp=%d&"			// Image Sharpness Filter Strength (1~15)
		"img_bright=%d&"		// Image Brightness (0~30:15)
		"img_contrast=%d&"		// Image Contrast (0~30:15)
		"img_color=%d&"			// Image Color (0~30:15)
		"img_hue=%d&"			// Image HUE (0~30:15)                                                               
		"defog_ctrl=%s&"			// DEFOG (OFF|LOW|MID|HIGH)                                                               
		"base_shutter=%s";		// Base Shutter Speed

	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;
	char dnn_mode_buffer[64]={0,};
	char adap_ir_buffer[64]={0,};
	int sdk_major,sdk_type,sdk_subtype,sdk_minor;
	mtable *runtime = get_runtime();

	snprintf(dnn_mode_buffer, sizeof(dnn_mode_buffer), "%s", str_null_to_blank(dnn_mode_str[info_set->dnn]));
    snprintf(adap_ir_buffer, sizeof(adap_ir_buffer), "%s", str_null_to_blank(adaptive_ir[info_set->adaptive_ir]));

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	sscanf(runtime[cam_id].sys.sdkver, "%d.%d.%d.%d", &sdk_major, &sdk_type, &sdk_subtype, &sdk_minor);

	if (sdk_major < 6)
	{
		if (strcmp(runtime[cam_id].sys.sdkver, "1.0.0.4") >= 0)
		{
			unsigned int val = 0;
			nf_ipcam_get_supported_func(cam_id, &val, NULL);
			if(val & NF_IPCAM_FUNC_VA)
			{
				snprintf(http_api, sizeof(http_api), set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						slow_shutter_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_buffer, dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "");
			}
			else
			{

				snprintf(http_api, sizeof(http_api), set_image_raw_v104,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						slow_shutter_str[info_set->ss], magc_str[info_set->max_agc],
						max_shutter_str[info_set->max_shutter],
						offon_str[info_set->iris], blc_mode_str[min(info_set->blc, 4)],
						blc_zone_str[max(info_set->blc, 4)], /* off, adap -> send lower */
						ff_mode_str[info_set->ff_mode],
						dnn_mode_buffer, dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "");
			}

		}
		else
		{
			if (info_set->iris >= NF_IPCAM_IMAGE_PIRIS_AUTO &&
				info_set->iris <= NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
			{
				if (info_set->iris == NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING ||
					info_set->iris == NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
				{
					snprintf(http_api, sizeof(http_api), set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_buffer, dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "&tracking=1");
				}
				else
				{
					snprintf(http_api, sizeof(http_api), set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_buffer, dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "&tracking=0");
				}
			}
			else
			{
				snprintf(http_api, sizeof(http_api), set_image_raw,
						ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
						offon_str[info_set->ss], magc_str[info_set->max_agc],
						offon_str[info_set->iris], offon_str[info_set->blc],
						dnn_mode_buffer, dnn_det_time[info_set->dnn_time],
						awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
						info_set->sharpness, info_set->brightness, info_set->contrast,
						info_set->color, info_set->tint, "");
			}
		}

		if (strncmp(runtime[cam_id].sys.stdver, "IN", 2) == 0
			|| (sdk_type == 12 && sdk_minor >= 1) )	//hisilicon cam && base shutter supported 
		{
			snprintf(http_api, sizeof(http_api), set_image_raw_inx,
					ae_mode_str[info_set->ae], 				// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
					info_set->agc, 							// AGC Gain (ME Only)
					info_set->shutter,						// Sutter Speed( ME Only)
					slow_shutter_str[info_set->ss],			// Slow Shutter (off|x2|x4|x8)
					dnr_ctr[info_set->dnr_ctr] ,			// Select 3D DNR Function (off|auto|manual)
					dnr_level[info_set->dnr_ctr],			// Select 3D DNR Strength (low|mid|high)
					wdr_mode_str[info_set->wd],				// Select WDR Function (off|low|mid|high)
					magc_str[info_set->max_agc],			// MAX AGC value (24|30|36|42)
					max_shutter_str[info_set->max_shutter],	// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
					offon_str[info_set->iris],				// P-Iris Control (manual|auto|boq|dof)
					ff_mode_str[info_set->ff_mode],			// Anti-Flicker (50|60|off)
					blc_mode_str[min(info_set->blc, 4)],	// BLC Control (off|adap|zone)
					blc_zone_str[max(info_set->blc, 4)],	// BLC Zone Contorl (lower|middle|upper|left|right)
					dnn_mode_buffer, 			// Day and Night Mode (auto|day|night|schedule)
					dnn_det_time[info_set->dnn_time],		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
					7,										// Hour of the day start time
					0,										// Minute of the day start time
					18,										// Hour of the day end time
					0,										// Minute of the end time
					adap_ir_buffer,								// Adaptive IR function (on|off)
					awb_mode_str[info_set->awb],			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
					mwb_mode_str[info_set->mwb],			// Maual White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
					info_set->sharpness,					// Image Sharpness Filter Strength (1~15)
					info_set->brightness,					// Image Brightness (0~30:15)
					info_set->contrast,						// Image Contrast (0~30:15)
					info_set->color,						// Image Color (0~30:15)
					info_set->tint,							// Image HUE (0~30:15)                                                               
					defog_str[info_set->defog],			// DEFOG (OFF|LOW|MID|HIGH)
					base_shutter_str[info_set->base_shutter]// Base Shutter Speed
						);              
		}
	}
	else	// sdk_major >= 6
	{
		snprintf(http_api, sizeof(http_api), set_image_raw_inx,
				ae_mode_str[info_set->ae], 				// AE Mode (AE(Auto Exposure) or ME(Menual Exposure))
				info_set->agc, 							// AGC Gain (ME Only)
				info_set->shutter,						// Sutter Speed( ME Only)
				slow_shutter_str[info_set->ss],			// Slow Shutter (off|x2|x4|x8)
				dnr_ctr[info_set->dnr_ctr] ,			// Select 3D DNR Function (off|auto|manual)
				dnr_level[info_set->dnr_ctr],			// Select 3D DNR Strength (low|mid|high)
				wdr_mode_str[info_set->wd],				// Select WDR Function (off|low|mid|high)
				magc_str[info_set->max_agc],			// MAX AGC value (24|30|36|42)
				max_shutter_str[info_set->max_shutter],	// Set Exposure time or Shutter Speed (shut_0|shut_1|shut_2)
				offon_str[info_set->iris],				// P-Iris Control (manual|auto|boq|dof)
				ff_mode_str[info_set->ff_mode],			// Anti-Flicker (50|60|off)
				blc_mode_str[min(info_set->blc, 4)],	// BLC Control (off|adap|zone)
				blc_zone_str[max(info_set->blc, 4)],	// BLC Zone Contorl (lower|middle|upper|left|right)
				dnn_mode_buffer, 			// Day and Night Mode (auto|day|night|schedule)
				dnn_det_time[info_set->dnn_time],		// Day&Night Transition time(Night to Day only) (det_0|det_5|det_10|det_15|det_30|det_60|det_alarm_in)
				7,										// Hour of the day start time
				0,										// Minute of the day start time
				18,										// Hour of the day end time
				0,										// Minute of the end time
				adap_ir_buffer,								// Adaptive IR function (on|off)
				awb_mode_str[info_set->awb],			// Auto White Balance Mode (auto|w_auto|manual|custom|stop)
				mwb_mode_str[info_set->mwb],			// Maual White Balance pre-set, awb_mode = manual (indoor|outdoor|fluorescent)
				info_set->sharpness,					// Image Sharpness Filter Strength (1~15)
				info_set->brightness,					// Image Brightness (0~30:15)
				info_set->contrast,						// Image Contrast (0~30:15)
				info_set->color,						// Image Color (0~30:15)
				info_set->tint,							// Image HUE (0~30:15)                                                               
				defog_str[info_set->defog],			// DEFOG (OFF|LOW|MID|HIGH)
				base_shutter_str[info_set->base_shutter]// Base Shutter Speed
					);               
	}

	if(runtime[cam_id].image.supported & NF_IPCAM_IMAGE_COLORVU){
		len = strlen(http_api);
		if(strstr(dnn_mode_buffer, "day")){
			snprintf(dnn_mode_buffer, sizeof(dnn_mode_buffer), "off");
		}else if(strstr(dnn_mode_buffer, "night")){
			snprintf(dnn_mode_buffer, sizeof(dnn_mode_buffer), "on");
		}

		len += snprintf(http_api + len, sizeof(http_api) - len, "&colorvu_mode=%s&colorvu_ae_mode=%s&colorvu_level=%d",
				                dnn_mode_buffer, adap_ir_buffer, info_set->colorvu_level);

		if(info_set->colorvu_ctrl > 0 && info_set->colorvu_ctrl < NF_IPCAM_COLORVU_CTRL_NR){
			snprintf(http_api + len, sizeof(http_api) - len, "&colorvu_ctrl=%s",colorvu_ctrl_str[info_set->colorvu_ctrl]);
		}
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_IMAGE, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_IMAGE, sock_buf);

	return len;
}

extern int cam_set_image(image_info* info_set, int cam_id)
{
	const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto", "", "", "auto", "manual" };
	const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
	const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	const char *dnn_mode_str[] = { "auto", "day", "night" };
	const char *dnn_det_time[] = 
		{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
	const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
	const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
	const char set_image_raw[] =
			"action=set_setup&menu=video.camera&"
			"ae_mode=%s&me_agc=%d&me_shutter=%d&ss_mode=%s&max_agc=%s&"
			"iris_mode=%s&blc_ctrl=%s&dnn_mode=%s&dnn_det_time=%s&"
			"awb_mode=%s&mwb_mode=%s&img_sharp=%d&"
			"img_bright=%d&img_contrast=%d&img_color=%d&img_hue=%d%s";

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


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	if ((1<<info_set->iris) >= NF_IPCAM_IMAGE_PIRIS_AUTO &&
		(1<<info_set->iris) <= NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
	{
		if ((1<<info_set->iris) == NF_IPCAM_IMAGE_PIRIS_BQ_TRACKING ||
			(1<<info_set->iris) == NF_IPCAM_IMAGE_PIRIS_DF_TRACKING)
		{
			printf("info_set->iris: (%08x) BQ_TRACKING || DF_TRACKING\n",info_set->iris);
			snprintf(http_api, 512, set_image_raw,
				ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
				offon_str[info_set->ss], magc_str[info_set->max_agc],
				offon_str[info_set->iris], offon_str[info_set->blc],
				dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, "&tracking=1");
		}
		else
		{
			printf("info_set->iris: (%08x) !BQ_TRACKING && !DF_TRACKING\n", info_set->iris);
			snprintf(http_api, 512, set_image_raw,
				ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
				offon_str[info_set->ss], magc_str[info_set->max_agc],
				offon_str[info_set->iris], offon_str[info_set->blc],
				dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, "&tracking=0");
		}
	}
	else
	{
		snprintf(http_api, 512, set_image_raw,
				ae_mode_str[info_set->ae], info_set->agc, info_set->shutter,
				offon_str[info_set->ss], magc_str[info_set->max_agc],
				offon_str[info_set->iris], offon_str[info_set->blc],
				dnn_mode_str[info_set->dnn], dnn_det_time[info_set->dnn_time],
				awb_mode_str[info_set->awb], mwb_mode_str[info_set->mwb],
				info_set->sharpness, info_set->brightness, info_set->contrast,
				info_set->color, info_set->tint, "");
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_IMAGE, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_IMAGE, sock_buf);

	return len;
}


static int _ssl_set_alarm(cam_info* info_set, int cam_id)
{
	const char* no_yes[] = { "no", "yes" };
	const char* no_nc[] = { "no", "nc" };
	const char* off_on[] = { "useroff", "useron" };

	const char set_alarm_raw[] =
			"action=set_setup&menu=event.alarm_port&"
			"in_enable=%s&in_type=%s&in_text=ALARM%201&"
			"out_oper=%s&out_mode=transparent&out_dwell=5";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	int in_type = 0;

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	if (info_set->alarm.in_type == NF_IPCAM_ALARM_TYPE_NO) in_type = 0;
	else if (info_set->alarm.in_type == NF_IPCAM_ALARM_TYPE_NC) in_type = 1;

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_alarm_raw,
			no_yes[info_set->alarm.in_onoff],
			no_nc[in_type],
			off_on[info_set->alarm.out_onoff]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ALARM, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ALARM, sock_buf);

	return len;
}

static int _common_set_alarm(cam_info* info_set, int cam_id)
{
	const char* no_yes[] = { "no", "yes" };
	const char* no_nc[] = { "no", "nc" };
	const char* off_on[] = { "useroff", "useron" };

	const char set_alarm_raw[] =
			"action=set_setup&menu=event.alarm_port&"
			"in_enable=%s&in_type=%s&in_text=ALARM%201&"
			"out_oper=%s&out_mode=transparent&out_dwell=5";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	int in_type = 0;

	mtable *runtime = get_runtime();


	if (info_set->alarm.in_type == NF_IPCAM_ALARM_TYPE_NO) in_type = 0;
	else if (info_set->alarm.in_type == NF_IPCAM_ALARM_TYPE_NC) in_type = 1;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_alarm_raw,
			no_yes[info_set->alarm.in_onoff],
			no_nc[in_type],
			off_on[info_set->alarm.out_onoff]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ALARM, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ALARM, sock_buf);

	return len;
}

static int _ssl_get_afcapa_digest(cam_info* info, int cam_id, char *rbuf)
{
	const char get_af_capa_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.af.capability HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"%s\r\n"	// Digest auth string
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

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
	char buf[16];
	const char *method = "GET";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	int len;
	int str_len = 0;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	unsigned short http_port;

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	//snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw_digest, ip_str, strlen(get_model_raw), auth_str, get_model_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, get_af_capa_raw, ip_str, auth_str);

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

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "new SSL_CTX alloc CH(%d)\n", cam_id);
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			return IPCAM_SETUP_RTN_FAILED;
		}

		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif
	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_mfz[]			= "mfz=";
				const char f_piris[]		= "piris=";
				const char f_zmin[]			= "zoom_min=";
				const char f_zmax[]			= "zoom_max=";
				const char f_fmin[]			= "focus_min=";
				const char f_fmax[]			= "focus_max=";
				const char f_pmin[]			= "piris_min=";
				const char f_pmax[]			= "piris_max=";
				const char f_endline[]		= "\r\n";

				/* mfz */
				s = recv_msg;
				p = strstr(s, f_mfz);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_mfz);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.mfz = atoi(buf);

				/* piris */
				s = recv_msg;
				p = strstr(s, f_piris);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_piris);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris = atoi(buf);

				/* zoom_min */
				s = recv_msg;
				p = strstr(s, f_zmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.zoom_min = atoi(buf);

				/* zoom_max */
				s = recv_msg;
				p = strstr(s, f_zmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.zoom_max = atoi(buf);

				/* focus_min */
				s = recv_msg;
				p = strstr(s, f_fmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_fmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.focus_min = atoi(buf);

				/* focus_max */
				s = recv_msg;
				p = strstr(s, f_fmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_fmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.focus_max = atoi(buf);

				/* iris_min */
				s = recv_msg;
				p = strstr(s, f_pmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_pmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris_min = atoi(buf);

				/* iris_max */
				s = recv_msg;
				p = strstr(s, f_pmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_pmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris_max = atoi(buf);
			}
	}

	IPCAM_DBG(MAJOR, "end CH(%d)\n", cam_id);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_afcapa(cam_info* info, int cam_id)
{
	const char get_af_capa_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.af.capability HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;
	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);
#if 0
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);
#endif

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, get_af_capa_raw, ip_str, auth_encbuf);

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
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "new SSL_CTX alloc CH(%d)\n", cam_id);
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ctx_g = NULL;
			//runtime[cam_id].sys.ssl_g = NULL;
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) < 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			IPCAM_DBG(MINOR, "Auth fail digest goes CH(%d)\n", cam_id);
			//SSL_shutdown(ssl);
			//close(sock);
			_release_resource(&sock, NULL, &ssl, &ctx);
			len = _ssl_get_afcapa_digest(info, cam_id, sock_buf);
			return len;
		}
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		//SSL_shutdown(ssl);
		//close(sock);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_mfz[]			= "mfz=";
				const char f_piris[]		= "piris=";
				const char f_zmin[]			= "zoom_min=";
				const char f_zmax[]			= "zoom_max=";
				const char f_fmin[]			= "focus_min=";
				const char f_fmax[]			= "focus_max=";
				const char f_pmin[]			= "piris_min=";
				const char f_pmax[]			= "piris_max=";
				const char f_endline[]		= "\r\n";

				/* mfz */
				s = recv_msg;
				p = strstr(s, f_mfz);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_mfz);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.mfz = atoi(buf);

				/* piris */
				s = recv_msg;
				p = strstr(s, f_piris);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_piris);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris = atoi(buf);

				/* zoom_min */
				s = recv_msg;
				p = strstr(s, f_zmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.zoom_min = atoi(buf);

				/* zoom_max */
				s = recv_msg;
				p = strstr(s, f_zmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.zoom_max = atoi(buf);

				/* focus_min */
				s = recv_msg;
				p = strstr(s, f_fmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_fmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.focus_min = atoi(buf);

				/* focus_max */
				s = recv_msg;
				p = strstr(s, f_fmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_fmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.focus_max = atoi(buf);

				/* iris_min */
				s = recv_msg;
				p = strstr(s, f_pmin);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_pmin);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris_min = atoi(buf);

				/* iris_max */
				s = recv_msg;
				p = strstr(s, f_pmax);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_pmax);
				p = strstr(s, f_endline);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->afcapa.iris_max = atoi(buf);
			}
	}
	IPCAM_DBG(MAJOR, "end CH(%d)\n", cam_id);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_afcapa_digest(cam_info* info, int cam_id, char *rbuf)
{
	const char get_af_capa_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.af.capability HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"%s\r\n"	// Digest auth string
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, get_af_capa_raw, ip_str, auth_str);

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
	IPCAM_DBG(MINOR, "send CH(%d) - %s\n", cam_id, sock_buf);
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
	tv.tv_sec = 3;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_mfz[]			= "mfz=";
		const char f_piris[]		= "piris=";
		const char f_zmin[]			= "zoom_min=";
		const char f_zmax[]			= "zoom_max=";
		const char f_fmin[]			= "focus_min=";
		const char f_fmax[]			= "focus_max=";
		const char f_pmin[]			= "piris_min=";
		const char f_pmax[]			= "piris_max=";
		const char f_endline[]		= "\r\n";

		/* mfz */
		s = sock_buf;
		p = strstr(s, f_mfz);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_mfz);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.mfz = atoi(buf);

		/* piris */
		s = sock_buf;
		p = strstr(s, f_piris);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_piris);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris = atoi(buf);

		/* zoom_min */
		s = sock_buf;
		p = strstr(s, f_zmin);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_zmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.zoom_min = atoi(buf);

		/* zoom_max */
		s = sock_buf;
		p = strstr(s, f_zmax);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_zmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.zoom_max = atoi(buf);

		/* focus_min */
		s = sock_buf;
		p = strstr(s, f_fmin);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_fmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.focus_min = atoi(buf);

		/* focus_max */
		s = sock_buf;
		p = strstr(s, f_fmax);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_fmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.focus_max = atoi(buf);

		/* iris_min */
		s = sock_buf;
		p = strstr(s, f_pmin);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_pmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris_min = atoi(buf);

		/* iris_max */
		s = sock_buf;
		p = strstr(s, f_pmax);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_pmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris_max = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d)\n", cam_id);
	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_afcapa(cam_info* info, int cam_id)
{
	const char get_af_capa_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.af.capability HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
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

	char *s, *p;
	char buf[16];


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, get_af_capa_raw, ip_str, auth_encbuf);

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
	tv.tv_sec = 3;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					len = _common_get_afcapa_digest(info, cam_id, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_mfz[]			= "mfz=";
		const char f_piris[]		= "piris=";
		const char f_zmin[]			= "zoom_min=";
		const char f_zmax[]			= "zoom_max=";
		const char f_fmin[]			= "focus_min=";
		const char f_fmax[]			= "focus_max=";
		const char f_pmin[]			= "piris_min=";
		const char f_pmax[]			= "piris_max=";
		const char f_endline[]		= "\r\n";

		/* mfz */
		s = sock_buf;
		p = strstr(s, f_mfz);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_mfz);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.mfz = atoi(buf);

		/* piris */
		s = sock_buf;
		p = strstr(s, f_piris);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_piris);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris = atoi(buf);

		/* zoom_min */
		s = sock_buf;
		p = strstr(s, f_zmin);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_zmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.zoom_min = atoi(buf);

		/* zoom_max */
		s = sock_buf;
		p = strstr(s, f_zmax);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_zmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.zoom_max = atoi(buf);

		/* focus_min */
		s = sock_buf;
		p = strstr(s, f_fmin);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_fmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.focus_min = atoi(buf);

		/* focus_max */
		s = sock_buf;
		p = strstr(s, f_fmax);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_fmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.focus_max = atoi(buf);

		/* iris_min */
		s = sock_buf;
		p = strstr(s, f_pmin);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_pmin);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris_min = atoi(buf);

		/* iris_max */
		s = sock_buf;
		p = strstr(s, f_pmax);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_pmax);
		p = strstr(s, f_endline);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 16);
		memcpy(buf, s, (size_t)(p - s));
		info->afcapa.iris_max = atoi(buf);
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_set_origin(int cam_id)
{
	const char set_origin_raw[] = "action=set_setup&menu=video.af&af_cmd=origin";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_origin_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ORIGIN, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ORIGIN, sock_buf);

	return len;
}

static int _common_set_origin(int cam_id)
{
	const char set_origin_raw[] = "action=set_setup&menu=video.af&af_cmd=origin";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_origin_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ORIGIN, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ORIGIN, sock_buf);

	return len;
}

static int _ssl_set_zoom(ptz_info *info, int cam_id)
{
	char *set_zoom_raw = NULL;
	const char old_zoom_raw[] =
			"action=set_setup&menu=video.af&af_cmd=zoom&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_zoom_raw[] =
			"action=set_setup&menu=video.af_start&mode=zoom&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;

	char get_spd_sysdb[64];
	int speed = 0;

	mtable *runtime = get_runtime();


	IPCAM_DBG(MAJOR, "start\n");
	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	if(info == NULL)
		return -1;

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	use_old = 0;

	set_zoom_raw = use_old ? old_zoom_raw:new_zoom_raw;

	if (use_old)
	{
		snprintf(http_api, 256, set_zoom_raw, info->cmd);
	}
	else
	{
		speed = info->zoom_speed;
		if (info->cmd == NF_PTZ_CMD_ZOOM_WIDE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[0], speed);
		}
		else if (info->cmd== NF_PTZ_CMD_ZOOM_TELE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[1], speed);
		}
		else
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ZOOM, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ZOOM, sock_buf);

	return len;
}

static int _common_set_zoom(ptz_info *info, int cam_id)
{
	char *set_zoom_raw = NULL;
	const char old_zoom_raw[] =
			"action=set_setup&menu=video.af&af_cmd=zoom&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_zoom_raw[] =
			"action=set_setup&menu=video.af_start&mode=zoom&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;

	char get_spd_sysdb[64];
	int speed = 0;

	mtable *runtime = get_runtime();


	IPCAM_DBG(MAJOR, "start\n");
	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	if(info == NULL)
		return -1;

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	use_old = 0;
	if(runtime[cam_id].sys.model_code == NF_IPCAM_MODEL_AMB_A2 ||runtime[cam_id].sys.model_code == NF_IPCAM_MODEL_AMB_D1 )
	{
		use_old = 1;
	}

	set_zoom_raw = use_old ? old_zoom_raw:new_zoom_raw;

	if (use_old)
	{
		snprintf(http_api, 256, set_zoom_raw, info->cmd);
	}
	else
	{
		speed = info->zoom_speed;
		if (info->cmd== NF_PTZ_CMD_ZOOM_WIDE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[0], speed);
		}
		else if (info->cmd == NF_PTZ_CMD_ZOOM_TELE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[1], speed);
		}
		else
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ZOOM, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ZOOM, sock_buf);

	return len;
}

static int _ssl_set_focus(ptz_info *info, int cam_id)
{
	char *set_focus_raw = NULL;
	const char old_focus_raw[] =
			"action=set_setup&menu=video.af&af_cmd=focus&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_focus_raw[] =
			"action=set_setup&menu=video.af_start&mode=focus&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;

	char get_spd_sysdb[64];
	int speed = 0;

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	if(info == NULL)
		return -1;

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	use_old = 0;

	set_focus_raw = use_old ? old_focus_raw:new_focus_raw;


	if (use_old)
	{
		snprintf(http_api, 256, set_focus_raw, info->cmd);
	}
	else
	{
		speed = info->focus_speed;
		if (info->cmd== NF_PTZ_CMD_FOCUS_NEAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[0], speed);
		}
		else if (info->cmd == NF_PTZ_CMD_FOCUS_FAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[1], speed);
		}
		else
		{
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_FOCUS);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_FOCUS, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_FOCUS, sock_buf);

	return len;
}

static int _common_set_focus(ptz_info *info, int cam_id)
{
	char *set_focus_raw = NULL;
	const char old_focus_raw[] =
			"action=set_setup&menu=video.af&af_cmd=focus&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_focus_raw[] =
			"action=set_setup&menu=video.af_start&mode=focus&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;

	char get_spd_sysdb[64];
	int speed = 0;

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	if(info == NULL)
		return -1;

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	use_old = 0;
	if(runtime[cam_id].sys.model_code == NF_IPCAM_MODEL_AMB_A2 ||runtime[cam_id].sys.model_code == NF_IPCAM_MODEL_AMB_D1 )
	{
		use_old = 1;
	}


	set_focus_raw = use_old ? old_focus_raw:new_focus_raw;


	if (use_old)
	{
		snprintf(http_api, 256, set_focus_raw, info->cmd);
	}
	else
	{
		speed = info->focus_speed;
		if (info->cmd == NF_PTZ_CMD_FOCUS_NEAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[0], speed);
		}
		else if (info->cmd == NF_PTZ_CMD_FOCUS_FAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[1], speed);
		}
		else
		{
			nf_ipcam_setup_send_done(cam_id, NF_IPCAM_TYPE_SET_FOCUS);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_FOCUS, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_FOCUS, sock_buf);

	return len;
}

static int _ssl_set_onepush(int cam_id)
{
	const char set_onepush_raw[] = "action=set_setup&menu=video.af&af_cmd=oneshot";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_onepush_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ONESHOT, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ONESHOT, sock_buf);

	return len;
}

static int _common_set_onepush(int cam_id)
{
	const char set_onepush_raw[] = "action=set_setup&menu=video.af&af_cmd=oneshot";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_onepush_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ONESHOT, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ONESHOT, sock_buf);

	return len;
}

static int _ssl_set_reset_va(int cam_id)
{
	const char set_reset_va[] = "action=set_setup&menu=vca.restart";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_reset_va);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_RESET_VA, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_RESET_VA, sock_buf);

	return len;
}
static int _ssl_set_dc_iris_cal(int cam_id)
{
	const char set_dc_iris_cal_raw[] = "action=run.iris.calibration";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_dc_iris_cal_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_DC_IRIS_CAL, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_DC_IRIS_CAL, sock_buf);

	return len;
}

static int _common_set_dc_iris_cal(int cam_id)
{
	const char set_dc_iris_cal_raw[] = "action=run.iris.calibration";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_dc_iris_cal_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_DC_IRIS_CAL, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_DC_IRIS_CAL, sock_buf);
	return len;
}

/*
 *	return param is "DC IRIS CALIBRATION status"
 */
extern int _common_get_dc_iris_cal_status(int cam_id)
{
	const char get_dc_iris_cal_raw[] = "action=get_setup&menu=iris.calibration.status";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char totalrecv_sock[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	
	unsigned short http_port;

	int len = 0;
	int recv_len = 0;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);
	http_port = nf_ipcam_get_http_port(cam_id);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, get_dc_iris_cal_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	// Set Socket
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family 	= AF_INET;
	sin.sin_port 	= htons(http_port);
	sin.sin_addr.s_addr = inet_addr(ip_str);

	// Create Socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return RTN_ERROR;
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
			perror("setsockopt");
			close(sock);
			return RTN_ERROR;
		}
	}

	// Connection check
	if(connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) 
	{
		perror("connect");
		close(sock);
		return RTN_ERROR;
	}

	// Send check
	if(send(sock, sock_buf, strlen(sock_buf), 0) < 0) 
	{
		perror("send");
		close(sock);
		return RTN_ERROR;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	memset(totalrecv_sock, 0x00, SOCK_BUF_LENGTH);
	while((len = recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0)) > 0)
	{
		strncpy(totalrecv_sock + recv_len, sock_buf, len);
		recv_len += len;
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	}

	if (strstr(totalrecv_sock, "HTTP/1.1 200") != NULL)
	{
		if (strstr(totalrecv_sock, "SUCCESS") != NULL)
		{
			char *compare;
			compare = strstr(totalrecv_sock, "status=");
			compare = compare + 7*sizeof(char);
			int temp_state;
			temp_state = atoi(compare);
			return temp_state;
		}
	}
	return RTN_NORMAL;
}

static int _common_set_reset_va(int cam_id)
{
	const char set_reset_va[] = "action=set_setup&menu=vca.restart";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_reset_va);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	printf("%s\n",sock_buf);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_RESET_VA, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_RESET_VA, sock_buf);

	return len;
}

static int _ssl_set_piris(int value, int cam_id)
{
	const char set_piris_raw[] =
			"action=set_setup&menu=video.af&af_cmd=piris&af_value=%d";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_piris_raw, value);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_IRIS, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_IRIS, sock_buf);

	return len;
}

static int _common_set_piris(int value, int cam_id)
{
	const char set_piris_raw[] =
			"action=set_setup&menu=video.af&af_cmd=piris&af_value=%d";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_piris_raw, value);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_IRIS, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_IRIS, sock_buf);

	return len;
}

static int _ssl_get_zoom_digest(int *value, int cam_id, char *rbuf)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *e, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	char uri[128];
	char realm[128];
	char nonce[128];

	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_zoom_raw), auth_str, set_zoom_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(WARN, "sock fail CH(%d)\n", cam_id);
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(WARN, "setsocketopt fail CH(%d)\n", cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return (0);
	}

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "new SSL_CTX alloc CH(%d)\n", cam_id);
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			close(sock);
			//_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);
	if (SSL_connect(ssl) < 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (0);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}	
	str_len += len;
	strncpy(recv_msg, sock_buf, len);
	
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_zoom[]		= "zoom=";

				/* zoom parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_zoom);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zoom);
				p = strstr(s, "&");
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_zoom(int *value, int cam_id)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_zoom_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "new SSL_CTX alloc CH(%d)\n", cam_id);
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			close(sock);
			//_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ssl_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			IPCAM_DBG(MINOR, "Auth fail digest goes CH(%d)\n", cam_id);
			len = _ssl_get_zoom_digest(value, cam_id, sock_buf);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return len;
		}
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_zoom[]		= "zoom=";

				/* zoom parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_zoom);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zoom);
				p = strstr(s, "&");
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_zoom_digest(int *value, int cam_id, char* rbuf)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";


	char http_api[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(http_api, 256, set_zoom_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_zoom[]		= "zoom=";

		/* zoom parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_zoom);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_zoom);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}
static int _common_get_zoom(int *value, int cam_id)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
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

	char *s, *p;
	char buf[16];


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_zoom_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					len = _common_get_zoom_digest(value, cam_id, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_zoom[]		= "zoom=";

		/* zoom parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_zoom);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_zoom);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_focus_digest(int *value, int cam_id, char *rbuf)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *e, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	char uri[128];
	char realm[128];
	char nonce[128];

	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_zoom_raw), auth_str, set_zoom_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(ERROR, "fd fail\n");
		perror("socket");
		return (0);
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
		IPCAM_DBG(ERROR, "connect fail\n");
		_release_resource(&sock, NULL, NULL, NULL);
		return (0);
	}

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			close(sock);
			//_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);
	if (SSL_connect(ssl) < 0)
	{
		IPCAM_DBG(ERROR, "SSL connection fail\n");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_zoom[]		= "focus=";

				/* zoom parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_zoom);
				if (p == NULL)
				{
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zoom);
				p = strstr(s, "&");
				if (p == NULL)
				{
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_focus(int *value, int cam_id)
{
	const char set_focus_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);


	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_focus_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			close(sock);
			//_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			len = _ssl_get_focus_digest(value, cam_id, sock_buf);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return len;
		}
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_focus[]		= "focus=";

				/* focus parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_focus);
				if (p == NULL)
				{
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_focus);
				p = strstr(s, "&");
				if (p == NULL)
				{
					return IPCAM_SETUP_RTN_FAILED;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_focus_digest(int *value, int cam_id, char* rbuf)
{
	const char set_focus_raw[] =
			"action=get_setup&menu=video.af";


	char http_api[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(http_api, 256, set_focus_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND CH(%d) - %s\n", cam_id, sock_buf);
#endif

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
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
		IPCAM_DBG(WARN, "send fail CH(%d)\n", cam_id);
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_focus[]		= "focus=";

		/* focus parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_focus);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_focus);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}
static int _common_get_focus(int *value, int cam_id)
{
	const char set_focus_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
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

	char *s, *p;
	char buf[16];


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_focus_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND CH(%d) - %s\n", cam_id, sock_buf);
#endif

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
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
		IPCAM_DBG(WARN, "send fail CH(%d)\n", cam_id);
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					len = _common_get_focus_digest(value, cam_id, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_focus[]		= "focus=";

		/* focus parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_focus);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_focus);
		p = strstr(s, "&");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_piris_digest(int *value, int cam_id, char *rbuf)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *e, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	char uri[128];
	char realm[128];
	char nonce[128];

	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "https://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_zoom_raw), auth_str, set_zoom_raw);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(WARN, "sock fail CH(%d)\n", cam_id);
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 4;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(WARN, "setsocketopt fail CH(%d)\n", cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return (0);
	}

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) < 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return (0);
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL_read 3 CH(%d) - %s\n", cam_id, sock_buf);
#endif
			_release_resource(&sock, NULL, &ssl, &ctx);
			{
				const char f_zoom[]		= "piris=";

				/* zoom parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_zoom);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_zoom);
				p = strstr(s, "&");
				if (p == NULL)
				{
					memset(buf, 0x00, 8);
					memcpy(buf, s, strlen(s));
					*value = atoi(buf);
					return IPCAM_SETUP_RTN_DONE;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_piris(int *value, int cam_id)
{
	const char set_zoom_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int len = 0;
	int str_len = 0;

	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_zoom_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "new SSL_CTX alloc CH(%d)\n", cam_id);
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL_CTX alloc fail CH(%d)\n", cam_id);
			close(sock);
			//_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL alloc fail CH(%d)\n", cam_id);
			close(sock);
			SSL_CTX_free(ctx);
			//runtime[cam_id].sys.ssl_g = NULL;
			//runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail CH(%d)\n", cam_id);
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_write CH(%d) - %s\n", cam_id, sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		IPCAM_DBG(WARN, "SSL_read fail CH(%d)\n", cam_id);
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL_read 1 CH(%d) - %s\n", cam_id, sock_buf);
#endif
	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		if (strstr(sock_buf, "401 Unauthorized") != NULL)
		{
			IPCAM_DBG(MINOR, "Auth fail digest goes CH(%d)\n", cam_id);
			len = _ssl_get_piris_digest(value, cam_id, sock_buf);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return len;
		}
		IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char f_focus[]		= "piris=";

				/* focus parsing */
				memset(buf, 0x00, 16);
				s = recv_msg;
				p = strstr(s, f_focus);
				if (p == NULL)
				{
					IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
				s = p + strlen(f_focus);
				p = strstr(s, "&");
				if (p == NULL)
				{
					//IPCAM_DBG(WARN, "parsing error CH(%d)\n", cam_id);
					memset(buf, 0x00, 8);
					strcpy(buf, s);
					*value = atoi(buf);
					return IPCAM_SETUP_RTN_DONE;
				}
				memset(buf, 0x00, 8);
				memcpy(buf, s, (size_t)(p - s));
				*value = atoi(buf);
			}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_piris_digest(int *value, int cam_id, char* rbuf)
{
	const char get_piris_raw[] =
			"action=get_setup&menu=video.af";


	char http_api[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(http_api, 256, get_piris_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_piris[]		= "piris=";

		/* piris parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_piris);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_piris);
		p = strstr(s, "\r\n");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_piris(int *value, int cam_id)
{
	const char get_piris_raw[] =
			"action=get_setup&menu=video.af";

	char http_api[256];
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

	char *s, *p;
	char buf[16];


	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, get_piris_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					len = _common_get_piris_digest(value, cam_id, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_piris[]		= "piris=";

		/* piris parsing */
		memset(buf, 0x00, 16);
		s = sock_buf;
		p = strstr(s, f_piris);
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		s = p + strlen(f_piris);
		p = strstr(s, "\r\n");
		if (p == NULL) { return IPCAM_SETUP_RTN_FAILED; }
		memset(buf, 0x00, 8);
		memcpy(buf, s, (size_t)(p - s));
		*value = atoi(buf);
	}

	IPCAM_DBG(MAJOR, "end CH(%d) value(%d)\n", cam_id, *value);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	const char *modes[] = { "zoom", "focus" };
	const char http_api_raw[] =
			"action=set_setup&menu=video.af_stop&mode=%s";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int id;
	int len = 0;

	mtable *runtime = get_runtime();
	NF_IPCAM_SETUP_TYPE_E type = NF_IPCAM_TYPE_SET_STOP;


	switch(e)
	{
		case PTZ_SETUP_ZOOM:
			id = 0;
			break;
		case PTZ_SETUP_FOCUS:
			id = 1;
			break;
		default:
			return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, http_api_raw, modes[id]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_STOP, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_STOP, sock_buf);

	return len;
}

static int _common_set_ptz_stop(PTZ_FUNCS_E e, int cam_id)
{
	const char *modes[] = { "zoom", "focus" };
	const char http_api_raw[] =
			"action=set_setup&menu=video.af_stop&mode=%s";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int id;
	int len = 0;

	mtable *runtime = get_runtime();
	NF_IPCAM_SETUP_TYPE_E type = NF_IPCAM_TYPE_SET_STOP;


	switch(e)
	{
		case PTZ_SETUP_ZOOM:
			id = 0;
			break;
		case PTZ_SETUP_FOCUS:
			id = 1;
			break;
		default:
			return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, http_api_raw, modes[id]);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_STOP, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_STOP, sock_buf);

	return len;
}

static int _ssl_factory_default(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_factoryload";
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();


	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

	strcat(username, ":");
	strcat(username, password);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_reboot_raw, "factory", ip_str, auth_encbuf);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, sock_buf);

	return len;
}

static int _ti368_factory_default(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_factoryload";
	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];

	int len = 0;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();


	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

	strcat(username, ":");
	strcat(username, password);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_reboot_raw, "factory", ip_str, auth_encbuf);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_FACTORY_DEFAULT, sock_buf);

	return len;
}

static int __ssl_factory_default(int cam_id)
{
	char http_api[] = "action=set_setup&menu=system.manage_factoryload";
	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);

	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

	IPCAM_DBG(MINOR, "CH(%d) mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
			cam_id,
			runtime[cam_id].sys.macaddr[0], runtime[cam_id].sys.macaddr[1],
			runtime[cam_id].sys.macaddr[2], runtime[cam_id].sys.macaddr[3],
			runtime[cam_id].sys.macaddr[4], runtime[cam_id].sys.macaddr[5]
	);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_factory_raw, ip_str, auth_str);

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
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT] == NULL)
	{
		runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT] = SSL_CTX_new(SSLv23_client_method());

		if (runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT] == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT], SSL_OP_ALL);
		runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT] = SSL_new(runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
		if (runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT] == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock);
	len = SSL_connect(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
	}

	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	len = SSL_read(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	//if (strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "GET";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;
		memset(username, 0x00, 64);
		memset(password, 0x00, 64);

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);

		strcpy(username, "s_admin");
		cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
		cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
		cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
		snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

		snprintf(uri, 128, "http://%s:%d/cgi-bin/reboot.cgi", ip_str, http_port);
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_factory_raw, ip_str, auth_str);


		len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		len = SSL_read(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif

	}

	if (strstr(sock_buf, "Unauthorized") != NULL)
	{
/*		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "GET";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;
		memset(username, 0x00, 64);
		memset(password, 0x00, 64);

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);

		strcpy(username, "s_admin");
		cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
		cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
		cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
		snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

		snprintf(uri, 128, "http://%s:%d/cgi-bin/reboot.cgi", ip_str, http_port);
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
*/
		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_factory_raw, ip_str, auth_str);


		len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		len = SSL_read(runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif

	}

	_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[NF_IPCAM_TYPE_FACTORY_DEFAULT], &runtime[cam_id].sys.ctx[NF_IPCAM_TYPE_FACTORY_DEFAULT]);
	set_switch_polling_delay(cam_id, 10);
	nf_pnd_queue_push(cam_id, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);

	return IPCAM_SETUP_RTN_DONE;
}

static int __ti368_factory_default(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);

	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

	IPCAM_DBG(MAJOR, "CH(%d) mac(%02x-%02x-%02x-%02x-%02x-%02x)\n",
			cam_id,
			runtime[cam_id].sys.macaddr[0], runtime[cam_id].sys.macaddr[1],
			runtime[cam_id].sys.macaddr[2], runtime[cam_id].sys.macaddr[3],
			runtime[cam_id].sys.macaddr[4], runtime[cam_id].sys.macaddr[5]
	);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_factory_raw, ip_str, auth_str);

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

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "GET";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;
		memset(username, 0x00, 64);
		memset(password, 0x00, 64);

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);

		strcpy(username, "s_admin");
		cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
		cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
		cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
		snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);

		snprintf(uri, 128, "http://%s:%d/cgi-bin/reboot.cgi", ip_str, http_port);
		s = strstr(rbuf, f_str_realm);
		if (s == NULL) { close(sock); return IPCAM_SETUP_RTN_FAILED; }
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL) { close(sock); return IPCAM_SETUP_RTN_FAILED; }
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL) { close(sock); return IPCAM_SETUP_RTN_FAILED; }
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL) { close(sock); return IPCAM_SETUP_RTN_FAILED; }
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_factory_raw, ip_str, auth_str);

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

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) <= 0)
		{
			perror("recv");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		printf("\n%s\n", sock_buf);
#endif
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	close(sock);
	set_switch_polling_delay(cam_id, 10);
#if 0
	nf_pnd_queue_push(cam_id, IPCAM_EVENT_DEVICE_OUT, __LINE__, __FILE__);
#endif

	return IPCAM_SETUP_RTN_DONE;
}

/* DNN ADJUST */


static int  _ssl_set_dnn_adjust_d2n(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_d2n_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=d2n&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_d2n_raw,
			info_set->dnn_sense_dton);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, sock_buf);
	return len;
}

static int  _ssl_set_dnn_adjust_n2d(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_n2d_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=n2d&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_n2d_raw,
			info_set->dnn_sense_ntod);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, sock_buf);

	return len;
}

static int _ti368_set_dnn_adjust_d2n(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_d2n_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=d2n&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_d2n_raw,
			info_set->dnn_sense_dton);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ADJUST_D2N, sock_buf);

	return len;
}

static int _ti368_set_dnn_adjust_n2d(image_info* info_set, int cam_id)
{
	const char set_dnn_adjust_n2d_raw[] =
		"action=set_setup&menu=video.dnn_adjust&"
		"dnn_cmd=n2d&dnn_value=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_dnn_adjust_n2d_raw,
			info_set->dnn_sense_ntod);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_ADJUST_N2D, sock_buf);

	return len;
}

static int _ti368_set_focus_compensation(focus_comp_info* info, int cam_id)
{
	const char set_focus_com[] = 
		"action=set_setup&menu=video.comp&"
		"comp_mode=%d&dnn_comp_mode=%d&maskarea0=%d"
		"&areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d";
	char http_api[512] = {0,};
	char auth_encbuf[256] = {0,};
	char auth_str[256] = {0,};
	char sock_buf[SOCK_BUF_LENGTH] = {0,};
	char username[64] = {0,};
	char password[64] = {0,};
	char ip_str[16] = {0,};
	int len = 0;

	mtable *runtime = get_runtime();
	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_focus_com, info->tem_comp_mode, info->dnn_comp_mode, 
			info->maskarea[0], info->areatx[0], info->areaty[0], info->areabx[0], info->areaby[0]);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_FOCUS_COMP, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_FOCUS_COMP, sock_buf);

	return IPCAM_SETUP_RTN_DONE;
}

static int  _ssl_set_focus_compensation(focus_comp_info* info, int cam_id)
{
	const char set_focus_com[] = 
		"action=set_setup&menu=video.comp&"
		"comp_mode=%d&dnn_comp_mode=%d&maskarea0=%d"
		"&areatx0=%d&areaty0=%d&areabx0=%d&areaby0=%d";

	char http_api[512];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char username[64];
	char password[64];
	char ip_str[16];
	int len = 0;
	int str_len = 0;
	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 512, set_focus_com, info->tem_comp_mode, info->dnn_comp_mode, 
			info->maskarea[0], info->areatx[0], info->areaty[0], info->areabx[0], info->areaby[0]);

	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_FOCUS_COMP, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_FOCUS_COMP, sock_buf);
	return len;
}

static int _ti368_get_focus_compensation(focus_comp_info* info, int cam_id)
{
	const char get_focus_com[] = "action=get_setup&menu=video.comp";
	char http_api[512] = {0,};
	char auth_encbuf[256] = {0,};
	char auth_str[256] = {0,};
	char sock_buf[SOCK_BUF_LENGTH] = {0,};
	char username[64] = {0,};
	char password[64] = {0,};
	char ip_str[16] = {0,};

	int len = 0;
	int sock = -1;
	unsigned short http_port = 0;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	char buf[16];

	mtable *runtime = get_runtime();
	http_port = nf_ipcam_get_http_port(cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, get_focus_com);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), 
			auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		IPCAM_DBG(WARN, "sock fail CH(%d)\n", cam_id);
		perror("socket");
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
			IPCAM_DBG(WARN, "setsocketopt fail CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND CH(%d) - %s\n", cam_id, sock_buf);
#endif

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
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
		IPCAM_DBG(WARN, "send fail CH(%d)\n", cam_id);
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 4;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					//len = _common_get_focus_digest(value, cam_id, sock_buf);
					return len;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}



	char *tem_comp_str = "comp_mode=";
	char *dnn_comp_str = "dnn_comp_mode=";
	char *maskarea_str = "maskarea0=";
	char *areatx_str = "areatx0=";
	char *areaty_str = "areaty0=";
	char *areabx_str = "areabx0=";
	char *areaby_str = "areaby0=";

	// Parsing
	memset(buf, 0x00, 16);
	if(sock_buf[0] != '\0' && info != NULL)
	{
		s = sock_buf;
		p = strstr(s, tem_comp_str);
		if(p != NULL){
			s = p + strlen(tem_comp_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }

			p = t;
			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->tem_comp_mode = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, dnn_comp_str);
		if(p != NULL){
			s = p + strlen(dnn_comp_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->dnn_comp_mode = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, maskarea_str);
		if(p != NULL){
			s = p + strlen(maskarea_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->maskarea[0] = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, areatx_str);
		if(p != NULL){
			s = p + strlen(areatx_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->areatx[0] = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, areaty_str);
		if(p != NULL){
			s = p + strlen(areaty_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->areaty[0] = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, areabx_str);
		if(p != NULL){
			s = p + strlen(areabx_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->areabx[0] = atoi(buf);
			}
		}

		s = sock_buf;
		p = strstr(s, areaby_str);
		if(p != NULL){
			s = p + strlen(areaby_str);

			char *t = p;
			while(*t != '&' && *t != '\0'){ t++; }
			p = t;

			if(p != NULL){
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info->areaby[0] = atoi(buf);
			}
		}

	}


	IPCAM_DBG(MAJOR, "end CH(%d) \n", cam_id);
	return IPCAM_SETUP_RTN_DONE;
}

static int  _ssl_get_focus_compensation(focus_comp_info* info, int cam_id)
{
	const char get_focus_com[] = "action=get_setup&menu=video.comp";
	char http_api[512] = {0,};
	char auth_encbuf[256] = {0,};
	char auth_str[256] = {0,};
	char sock_buf[SOCK_BUF_LENGTH] = {0,};
	char username[64] = {0,};
	char password[64] = {0,};
	char ip_str[16] = {0,};

	int len = 0;
	int sock = -1;
	unsigned short http_port = 0;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	char *s, *p;
	char buf[16];
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();
	http_port = nf_ipcam_get_http_port(cam_id);
	SSL *ssl = runtime[cam_id].sys.ssl_g;
	SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, get_focus_com);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), 
			auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		IPCAM_DBG(WARN, "sock fail CH(%d)\n", cam_id);
		perror("socket");
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
			IPCAM_DBG(WARN, "setsocketopt fail CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND CH(%d) - %s\n", cam_id, sock_buf);
#endif

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "connect fail CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
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
			SSL_CTX_free(ctx);
			close(sock);
			runtime[cam_id].sys.ssl_g = NULL;
			runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		runtime[cam_id].sys.ssl_g = ssl;
		runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		SSL_shutdown(ssl);
		close(sock);
		//_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	else if (strstr(sock_buf, "chunked") != NULL)
	{
		while (1)
		{
			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				SSL_shutdown(ssl);
				close(sock);
				//_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
			{
				perror("read");
				SSL_shutdown(ssl);
				close(sock);
				//_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
			{
				const char f_ok[]		= "HTTP/1.1 200";
				char *errcode = sock_buf;

				errcode = strstr(errcode, f_ok);
				if (errcode == NULL)
				{
					if (strstr(sock_buf, "401 Unauthorized") != NULL)
					{
						if (strstr(sock_buf, "Digest") != NULL)
						{
							IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
							//len = _common_get_focus_digest(value, cam_id, sock_buf);
							return len;
						}
					}
					IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
					return IPCAM_SETUP_RTN_FAILED;
				}
			}



			char *tem_comp_str = "comp_mode=";
			char *dnn_comp_str = "dnn_comp_mode=";
			char *maskarea_str = "maskarea0=";
			char *areatx_str = "areatx0=";
			char *areaty_str = "areaty0=";
			char *areabx_str = "areabx0=";
			char *areaby_str = "areaby0=";

			// Parsing
			memset(buf, 0x00, 16);
			if(sock_buf[0] != '\0' && info != NULL)
			{
				s = sock_buf;
				p = strstr(s, tem_comp_str);
				if(p != NULL){
					s = p + strlen(tem_comp_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }

					p = t;
					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->tem_comp_mode = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, dnn_comp_str);
				if(p != NULL){
					s = p + strlen(dnn_comp_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->dnn_comp_mode = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, maskarea_str);
				if(p != NULL){
					s = p + strlen(maskarea_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->maskarea[0] = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, areatx_str);
				if(p != NULL){
					s = p + strlen(areatx_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->areatx[0] = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, areaty_str);
				if(p != NULL){
					s = p + strlen(areaty_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->areaty[0] = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, areabx_str);
				if(p != NULL){
					s = p + strlen(areabx_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->areabx[0] = atoi(buf);
					}
				}

				s = sock_buf;
				p = strstr(s, areaby_str);
				if(p != NULL){
					s = p + strlen(areaby_str);

					char *t = p;
					while(*t != '&' && *t != '\0'){ t++; }
					p = t;

					if(p != NULL){
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info->areaby[0] = atoi(buf);
					}
				}

			}
		}
	}

	SSL_shutdown(ssl);
	close(sock);

	IPCAM_DBG(MAJOR, "end CH(%d) \n", cam_id);
	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_get_image(image_info* info_set, int cam_id)
{
	const char get_image_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.camera HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;
	char buf[16];

	unsigned char cal_mac[3];
	int str_len = 0;
	int len = 0;
	mtable *runtime = get_runtime();
	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);
#if 0
	strcpy(username, "s_admin");
	cal_mac[0] = (unsigned char)(((runtime[cam_id].sys.macaddr[0] + runtime[cam_id].sys.macaddr[5]) & 0xff) / 2);
	cal_mac[1] = (unsigned char)((runtime[cam_id].sys.macaddr[1] + runtime[cam_id].sys.macaddr[3]) & 0xff);
	cal_mac[2] = (unsigned char)((runtime[cam_id].sys.macaddr[2] + runtime[cam_id].sys.macaddr[4]) & 0xff) / 2;
	snprintf(password, 64, "%02X%02X%02X", cal_mac[0], cal_mac[1], cal_mac[2]);
#endif

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, get_image_raw, ip_str, auth_encbuf);

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

	if (ctx == NULL)
	{
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
			SSL_CTX_free(ctx);
			close(sock);
			runtime[cam_id].sys.ssl_g = NULL;
			runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		printf("[%s] ERROR: Non-chunked type\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
				const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto", "", "", "auto", "manual" };
				const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
				const char *blc_zone_str[] = { "", "", "", "", "lower", "middle", "upper", "left", "right" };
				const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
				const char *dnn_mode_str[] = { "auto", "day", "night" };
				const char *dnn_det_time[] =
					{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
				const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
				const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
				int i = 0;

				const char f_ae_mode[]		= "ae_mode=";
				const char f_me_agc[]		= "me_agc=";
				const char f_me_shutter[]	= "me_shutter=";
				const char f_ss_mode[]		= "ss_mode=";
				const char f_max_agc[]		= "max_agc=";
				const char f_iris_mode[]	= "iris_mode=";
				const char f_blc_ctrl[]		= "blc_ctrl=";
				const char f_awb_mode[]		= "awb_mode=";
				const char f_mwb_mode[]		= "mwb_mode=";
				const char f_dnn_mode[]		= "dnn_mode=";
				const char f_dnn_det_time[]	= "dnn_det_time=";
				const char f_img_sharp[]	= "img_sharp=";
				const char f_img_bright[]	= "img_bright=";
				const char f_img_contrast[]	= "img_contrast=";
				const char f_img_color[]	= "img_color=";
				const char f_img_hue[]		= "img_hue=";
				const char f_endline[]		= "\r\n";

				/* auto exposure */
				s = recv_msg;
				p = strstr(s, f_ae_mode);
				if (p)
				{
					s = p + strlen(f_ae_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_EXP_MODE_NR; i++)
						{
							if(strcmp(ae_mode_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_EXP_MODE_NR)
						{
							info_set->ae = i;
						}
					}
				}
				/* agc gain */
				s = recv_msg;
				p = strstr(s, f_me_agc);
				if (p)
				{
					s = p + strlen(f_me_agc);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->agc = atoi(buf);
					}
				}
				/* e-shutter speed */
				s = recv_msg;
				p = strstr(s, f_me_shutter);
				if (p)
				{
					s = p + strlen(f_me_shutter);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->shutter = atoi(buf);
					}
				}
				/* slowshutter mode */
				s = recv_msg;
				p = strstr(s, f_ss_mode);
				if (p)
				{
					s = p + strlen(f_ss_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_SLSH_MODE_NR; i++)
						{
							if(strcmp(offon_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_SLSH_MODE_NR)
						{
							info_set->ss = i;
						}
					}
				}
				/* max agc */
				s = recv_msg;
				p = strstr(s, f_max_agc);
				if (p)
				{
					s = p + strlen(f_max_agc);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_MAGC_MODE_DEFAULT; i++)// not nr
						{
							if(strcmp(magc_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_MAGC_MODE_DEFAULT)
						{
							info_set->max_agc = i;
						}
					}
				}
				/* iris mode */
				s = recv_msg;
				p = strstr(s, f_iris_mode);
				if (p)
				{
					s = p + strlen(f_iris_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_IRIS_NR; i++)
						{
							if(strcmp(offon_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_IRIS_NR)
						{
							info_set->iris = i;
						}
					}
				}
				/* blc control */
				s = recv_msg;
				p = strstr(s, f_blc_ctrl);
				if (p)
				{
					s = p + strlen(f_blc_ctrl);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_BLC_NR; i++)
						{
							if(strcmp(blc_zone_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_BLC_NR)
						{
							info_set->blc = i;
						}
					}
				}
				/* white balance */
				s = recv_msg;
				p = strstr(s, f_awb_mode);
				if (p)
				{
					s = p + strlen(f_awb_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_WB_MODE_NR; i++)
						{
							if(strcmp(awb_mode_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_WB_MODE_NR)
						{
							info_set->awb = i;
						}
					}
				}
				/* manual white balance */
				s = recv_msg;
				p = strstr(s, f_mwb_mode);
				if (p)
				{
					s = p + strlen(f_mwb_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_MWB_MODE_NR; i++)
						{
							if(strcmp(mwb_mode_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_MWB_MODE_NR)
						{
							info_set->mwb = i;
						}
					}
				}
				/* day_night mode */
				s = recv_msg;
				p = strstr(s, f_dnn_mode);
				if (p)
				{
					s = p + strlen(f_dnn_mode);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_DN_NR; i++)
						{
							if(strcmp(dnn_mode_str[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_DN_NR)
						{
							info_set->dnn = i;
						}
					}
				}
				/* day_night time */
				s = recv_msg;
				p = strstr(s, f_dnn_det_time);
				if (p)
				{
					s = p + strlen(f_dnn_det_time);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						for(i = 0; i < NF_IPCAM_IMAGE_TGTIME_NR; i++)
						{
							if(strcmp(dnn_det_time[i], buf) == 0) break;
						}
						if(i < NF_IPCAM_IMAGE_TGTIME_NR)
						{
							info_set->dnn_time = i;
						}
					}
				}
				/* sharpness */
				s = recv_msg;
				p = strstr(s, f_img_sharp);
				if (p)
				{
					s = p + strlen(f_img_sharp);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->sharpness = atoi(buf);
					}
				}
				/* bright */
				s = recv_msg;
				p = strstr(s, f_img_bright);
				if (p)
				{
					s = p + strlen(f_img_bright);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->brightness = atoi(buf);
					}
				}
				/* contrast */
				s = recv_msg;
				p = strstr(s, f_img_contrast);
				if (p)
				{
					s = p + strlen(f_img_contrast);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->contrast = atoi(buf);
					}
				}
				/* color */
				s = recv_msg;
				p = strstr(s, f_img_color);
				if (p)
				{
					s = p + strlen(f_img_color);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->color = atoi(buf);
					}
				}
				/* hue */
				s = recv_msg;
				p = strstr(s, f_img_hue);
				if (p)
				{
					s = p + strlen(f_img_hue);
					p = strstr(s, f_endline);
					if (p)
					{
						memset(buf, 0x00, 16);
						memcpy(buf, s, (size_t)(p - s));
						info_set->tint = atoi(buf);
					}
				}
			}// end setup
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_get_image(image_info* info_set, int cam_id)
{
	const char get_image_raw[] =
"GET /cgi-bin/action.fcgi?api=get_setup.video.camera HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

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

	char *s, *p;
	char buf[16];


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, get_image_raw, ip_str, auth_encbuf);

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

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char *ae_mode_str[] = { "","","","","manual", "auto", "auto_out", "auto", "", "", "auto", "manual" };
		const char *offon_str[] = { "off", "on", "0", "1", "2", "3", "2", "3" };
		const char *blc_zone_str[] = { "", "", "", "", "lower", "middle", "upper", "left", "right" };
		const char *magc_str[] = {"24","30", "", "", "", "", "", "36", "42", "30", "36", "42", "46", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
		const char *dnn_mode_str[] = { "auto", "day", "night" };
		const char *dnn_det_time[] =
			{ "det_0", "det_5", "det_10", "det_15", "det_30", "det_60", "det_alarm_in" };
		const char *awb_mode_str[] = { "","","","manual", "auto", "w_auto" };
		const char *mwb_mode_str[] = { "indoor", "outdoor", "fluorescent" };
		int i = 0;

		const char f_ae_mode[]		= "ae_mode=";
		const char f_me_agc[]		= "me_agc=";
		const char f_me_shutter[]	= "me_shutter=";
		const char f_ss_mode[]		= "ss_mode=";
		const char f_max_agc[]		= "max_agc=";
		const char f_iris_mode[]	= "iris_mode=";
		const char f_blc_ctrl[]		= "blc_ctrl=";
		const char f_awb_mode[]		= "awb_mode=";
		const char f_mwb_mode[]		= "mwb_mode=";
		const char f_dnn_mode[]		= "dnn_mode=";
		const char f_dnn_det_time[]	= "dnn_det_time=";
		const char f_img_sharp[]	= "img_sharp=";
		const char f_img_bright[]	= "img_bright=";
		const char f_img_contrast[]	= "img_contrast=";
		const char f_img_color[]	= "img_color=";
		const char f_img_hue[]		= "img_hue=";
		const char f_endline[]		= "\r\n";

		/* auto exposure */
		s = sock_buf;
		p = strstr(s, f_ae_mode);
		if (p)
		{
			s = p + strlen(f_ae_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_EXP_MODE_NR; i++)
				{
					if(strcmp(ae_mode_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_EXP_MODE_NR)
				{
					info_set->ae = i;
				}
			}
		}
		/* agc gain */
		s = sock_buf;
		p = strstr(s, f_me_agc);
		if (p)
		{
			s = p + strlen(f_me_agc);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->agc = atoi(buf);
			}
		}
		/* e-shutter speed */
		s = sock_buf;
		p = strstr(s, f_me_shutter);
		if (p)
		{
			s = p + strlen(f_me_shutter);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->shutter = atoi(buf);
			}
		}
		/* slowshutter mode */
		s = sock_buf;
		p = strstr(s, f_ss_mode);
		if (p)
		{
			s = p + strlen(f_ss_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_SLSH_MODE_NR; i++)
				{
					if(strcmp(offon_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_SLSH_MODE_NR)
				{
					info_set->ss = i;
				}
			}
		}
		/* max agc */
		s = sock_buf;
		p = strstr(s, f_max_agc);
		if (p)
		{
			s = p + strlen(f_max_agc);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_MAGC_MODE_DEFAULT; i++)// not nr
				{
					if(strcmp(magc_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_MAGC_MODE_DEFAULT)
				{
					info_set->max_agc = i;
				}
			}
		}
		/* iris mode */
		s = sock_buf;
		p = strstr(s, f_iris_mode);
		if (p)
		{
			s = p + strlen(f_iris_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_IRIS_NR; i++)
				{
					if(strcmp(offon_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_IRIS_NR)
				{
					info_set->iris = i;
				}
			}
		}
		/* blc control */
		s = sock_buf;
		p = strstr(s, f_blc_ctrl);
		if (p)
		{
			s = p + strlen(f_blc_ctrl);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_BLC_NR; i++)
				{
					if(strcmp(blc_zone_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_BLC_NR)
				{
					info_set->blc = i;
				}
			}
		}
		/* white balance */
		s = sock_buf;
		p = strstr(s, f_awb_mode);
		if (p)
		{
			s = p + strlen(f_awb_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_WB_MODE_NR; i++)
				{
					if(strcmp(awb_mode_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_WB_MODE_NR)
				{
					info_set->awb = i;
				}
			}
		}
		/* manual white balance */
		s = sock_buf;
		p = strstr(s, f_mwb_mode);
		if (p)
		{
			s = p + strlen(f_mwb_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_MWB_MODE_NR; i++)
				{
					if(strcmp(mwb_mode_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_MWB_MODE_NR)
				{
					info_set->mwb = i;
				}
			}
		}
		/* day_night mode */
		s = sock_buf;
		p = strstr(s, f_dnn_mode);
		if (p)
		{
			s = p + strlen(f_dnn_mode);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_DN_NR; i++)
				{
					if(strcmp(dnn_mode_str[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_DN_NR)
				{
					info_set->dnn = i;
				}
			}
		}
		/* day_night time */
		s = sock_buf;
		p = strstr(s, f_dnn_det_time);
		if (p)
		{
			s = p + strlen(f_dnn_det_time);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				for(i = 0; i < NF_IPCAM_IMAGE_TGTIME_NR; i++)
				{
					if(strcmp(dnn_det_time[i], buf) == 0) break;
				}
				if(i < NF_IPCAM_IMAGE_TGTIME_NR)
				{
					info_set->dnn_time = i;
				}
			}
		}
		/* sharpness */
		s = sock_buf;
		p = strstr(s, f_img_sharp);
		if (p)
		{
			s = p + strlen(f_img_sharp);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->sharpness = atoi(buf);
			}
		}
		/* bright */
		s = sock_buf;
		p = strstr(s, f_img_bright);
		if (p)
		{
			s = p + strlen(f_img_bright);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->brightness = atoi(buf);
			}
		}
		/* contrast */
		s = sock_buf;
		p = strstr(s, f_img_contrast);
		if (p)
		{
			s = p + strlen(f_img_contrast);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->contrast = atoi(buf);
			}
		}
		/* color */
		s = sock_buf;
		p = strstr(s, f_img_color);
		if (p)
		{
			s = p + strlen(f_img_color);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->color = atoi(buf);
			}
		}
		/* hue */
		s = sock_buf;
		p = strstr(s, f_img_hue);
		if (p)
		{
			s = p + strlen(f_img_hue);
			p = strstr(s, f_endline);
			if (p)
			{
				memset(buf, 0x00, 16);
				memcpy(buf, s, (size_t)(p - s));
				info_set->tint = atoi(buf);
			}
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}


static int _ti386_sn_get(struct sn_info *info, int cam_id, int v)
{
	const char get_sn[] =
		"GET /cgi-bin/action.fcgi?api=get_sn HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: gzip,deflate,sdch\r\n"
		"Accept-Language: ko,en-US,en;q=0.8\r\n"
		"Accept-Charset: ISO-8859-1,utf-8,q=0.7,*;q=0.3\r\n\r\n";

	const char get_cbc_sn[] =
		"GET /cgi-bin/action.fcgi?gnz_api=get_sn HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: gzip,deflate,sdch\r\n"
		"Accept-Language: ko,en-US,en;q=0.8\r\n"
		"Accept-Charset: ISO-8859-1,utf-8,q=0.7,*;q=0.3\r\n\r\n";

	int rtn = 0;
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

	char *s, *p;
	char buf[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	if(v == IPCAM_SN_VENDOR_ITX)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, get_sn, ip_str, auth_encbuf);
	}
	else if (v == IPCAM_SN_VENDOR_CBC)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, get_cbc_sn, ip_str, auth_encbuf);
	}


	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		if(ret < 0) {
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setcockopt");
			close(sock);
			sock = (-1);
			return -1;
		}
	}

	if(connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return -1;
	}


	if(send(sock, sock_buf, strlen(sock_buf), 0) < 0) {
		perror("send");
		close(sock);
		sock = (-1);
		return -1;
	}

	/*
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock +1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if(state == 0) {
		close(sock);
		sock = (-1);
		return -1;
	}
	*/

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if(recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0) {
		perror("recv");
		close(sock);
		sock = (-1);
		return -1;
	}

	close(sock);
	sock = (-1);

	rtn = sn_parsing_to_info(sock_buf, info);
	return rtn;
}

static int _ssl_sn_get(struct sn_info *info, int cam_id, int v)
{
	const char get_sn[] =
		"GET /cgi-bin/action.fcgi?api=get_sn HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: gzip,deflate,sdch\r\n"
		"Accept-Language: ko,en-US,en;q=0.8\r\n"
		"Accept-Charset: ISO-8859-1,utf-8,q=0.7,*;q=0.3\r\n\r\n";

	const char get_cbc_sn[] =
		"GET /cgi-bin/action.fcgi?gnz_api=get_sn HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Authorization: Basic %s\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: gzip,deflate,sdch\r\n"
		"Accept-Language: ko,en-US,en;q=0.8\r\n"
		"Accept-Charset: ISO-8859-1,utf-8,q=0.7,*;q=0.3\r\n\r\n";

	char auth_encbuf[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char recv_msg[SOCK_BUF_LENGTH];
	memset(recv_msg, 0x00, SOCK_BUF_LENGTH);
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
	char buf[16];
	int len = 0;
	int str_len = 0;
	mtable* runtime = get_runtime();

	//SSL *ssl = runtime[cam_id].sys.ssl_g;
	//SSL_CTX *ctx = runtime[cam_id].sys.ctx_g;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	if(v == IPCAM_SN_VENDOR_ITX)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, get_sn, ip_str, auth_encbuf);
	}
	else if (v == IPCAM_SN_VENDOR_CBC)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, get_cbc_sn, ip_str, auth_encbuf);
	}


	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		if(ret < 0) {
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setcockopt");
			close(sock);
			sock = (-1);
			return -1;
		}
	}

	if(connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return -1;
	}

	if (ctx == NULL)
	{
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
			SSL_CTX_free(ctx);
			close(sock);
			runtime[cam_id].sys.ssl_g = NULL;
			runtime[cam_id].sys.ctx_g = NULL;
			//_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		//runtime[cam_id].sys.ssl_g = ssl;
		//runtime[cam_id].sys.ctx_g = ctx;
	}
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		printf("[%s] SSL connection failed\n" , __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) < 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif


	/*
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock +1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if(state == 0) {
		close(sock);
		sock = (-1);
		return -1;
	}
	*/

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if ((len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH)) < 0)
	{
		perror("read");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	str_len += len;
	strncpy(recv_msg, sock_buf, len);

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		printf("[%s] ERROR: HTTP error\n", __FUNCTION__);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (strstr(sock_buf, "Content-Length") != NULL)
	{
		IPCAM_DBG(WARN, "Non-chunked CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
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
			sn_parsing_to_info(sock_buf, info);
	}

	return 0;
}

static int sn_parsing_to_info(char *string, struct sn_info *info)
{
	const char* SN = "sn=";
	char* str = NULL;
	char* tmp = NULL;

	if(string == NULL)
		return -1;

	if(info == NULL)
		return -2;

	memset(info, 0x00, sizeof(struct sn_info));

	str = strstr(string, SN);

	if(str != NULL) {
		tmp = str + 3;
		if(strncmp(tmp, "NON", 3) == 0)
			return -3;

		strncpy(info->vendor, tmp, 3);	// Vendor

		tmp = tmp + 4;
		strncpy(info->nation, tmp, 2);	// Nation

		tmp = tmp + 3;
		strncpy(info->date, tmp, 12);	// Date + Index

		tmp = tmp + 13;
		strncpy(info->mega, tmp, 2);	// Mega Pixel

		tmp = tmp + 3;
		strncpy(info->lens, tmp, 2);	// Lens Type

		tmp = tmp + 3;
		strncpy(info->heater, tmp, 1);	// Heater Type

		tmp = tmp + 1;
		strncpy(info->irled, tmp, 1);	// IR LED Type

		tmp = tmp + 1;
		strncpy(info->sensor, tmp, 1);	// Sensor Type

		tmp = tmp + 1;
		strncpy(info->ads, tmp, 1); // Adjust DNN Sensitivity
	} else {
		return -4;
	}

	return 0;
}

static const char str_api_upload[] =
	"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
	"Accept: text/html, application/xhtml+xml, */*\r\n"
	"Accept-Language: ko\r\n"
	//"Content-Type: application/x-www-form-urlencoded\r\n"
	"Content-Type: multipart/form-data; boundary=---------------------------7dd19424304f4\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"%s";	// HTTP API

static const char str_api_upload_digest[] =
	"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
	"Accept: text/html, application/xhtml+xml, */*\r\n"
	"Accept-Language: ko\r\n"
	//"Content-Type: application/x-www-form-urlencoded\r\n"
	"Content-Type: multipart/form-data; boundary=---------------------------7dd19424304f4\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"	// auth
	"\r\n"
	"%s";	// HTTP API

/**
 * @brief ITX카메??의 ??웨????데??트?????행??다.
 * @param cam_id 채널 번호.
 * @param file_nm ??일???
 * @param file_stream ??일 ??용.
 * @param file_len ??일 길이.
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
extern int cam_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len)
{
	int i, rtn = 0;
	int is_s1_special = 0;
	mtable* runtime = get_runtime();

	if (_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 5))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	//IPCAM_DBG(MINOR, "ch (%02d) upload start!!", cam_id);

	IPCAM_DBG(MINOR, "ch (%02d) fwmode reboot start!!", cam_id);

	char username[64];
	char password[64];

	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if(strcmp(username, "ADMIN") == 0 && strcmp(password, "1234") == 0 && nf_sysman_get_fwver_vendor() == 30)
	{
		//IPCAM_DBG(MINOR, "ch (%02d) S1 backdoor scenario warning!!", cam_id);
		is_s1_special = 1;
	}

	/* 0. set temp password for s1 factory default scenario */
	if(is_s1_special)
	{
		rtn = _common_set_passwd(cam_id);
		if( rtn != IPCAM_SETUP_RTN_DONE)
		{
			IPCAM_DBG(ERROR, "ch (%02d) set passwd(from factory default) fail!!", cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_GENERAL, 5);
			return rtn;
		}

		for(i = 0; i < 3; i++)
		{
			usleep(10 * 1000 * 1000);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 5 + i * 2);
		}
		{
			strncpy(runtime[cam_id].username, "ADMIN", 64);
			strncpy(runtime[cam_id].password, passwd_s1_18, 64);
		}
	}

	/* 1. reboot fwmode(upload api) */
	if (strstr(runtime[cam_id].sys.swver2,"51110") != NULL ||
		strstr(runtime[cam_id].sys.swver2,"61110") != NULL)
	{
		if (nf_ipcam_is_using_ssl(cam_id))
		{
			rtn = _ssl_reboot_fwmode(cam_id);
		}
		else
		{
			rtn = _common_reboot_fwmode(cam_id);
		}
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			IPCAM_DBG(MINOR, "ch (%02d) fwmode reboot fail!!", cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_GENERAL, 10);
			if(is_s1_special)
			{
				IPCAM_DBG(MINOR, "ch (%02d) S1 return to factory default !!", cam_id);
				_cam_factory_default(cam_id);
			}
			return rtn;
		}
		if (_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 10))
		{
			return IPCAM_SETUP_RTN_FAILED;
		}

		/* 2. send fwmode api */
		if (nf_ipcam_is_using_ssl(cam_id))
		{
			rtn = _ssl_set_fwmode(cam_id);
		}
		else
		{
			rtn = _common_set_fwmode(cam_id);
		}
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			IPCAM_DBG(MINOR, "ch (%02d) set fwmode fail, continue!!", cam_id);
			//return rtn;
		}

		/* wait until reboot complete */
		for(i = 0; i < 11; i++)
		{
			usleep(6 * 1000 * 1000);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FWMODE_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 12 + i * 2);
		}

		if (_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_SET_FWMODE, 0, NF_IPCAM_FW_ERR_OK, 35))
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	IPCAM_DBG(MINOR, "ch (%02d) set upmode start!!", cam_id);

	if(is_s1_special)
	{
		runtime[cam_id].sys.use_ssl = 1;
	}

	/* 3. send fw_upload api */
	if (nf_ipcam_is_using_ssl(cam_id))
	{
		rtn = _ssl_befo_fw_up(cam_id);
	}
	else
	{
		rtn = _common_befo_fw_up(cam_id);
	}

	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(MINOR, "ch (%02d) set upmode fail, continue!!", cam_id);
	}

	if (_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 0, NF_IPCAM_FW_ERR_OK, 40))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}


	IPCAM_DBG(MINOR, "ch (%02d) fw upload start!!", cam_id);

	/* 4. upload file */
	if (nf_ipcam_is_using_ssl(cam_id))
	{
		rtn = _ssl_upload_fw(cam_id, file_nm, file_stream, file_len);
#if 0
		if (rtn == IPCAM_SETUP_RTN_FAILED)
		{
			g_warning("[%s] ch (%02d) fw upload failed. retry after 5 seconds...", __FUNCTION__, cam_id);
			sleep(5);
			rtn = _ssl_upload_fw(cam_id, file_nm, file_stream, file_len);
		}
#endif
	}
	else
	{
		rtn = _common_upload_fw(cam_id, file_nm, file_stream, file_len);
	}


	if(rtn != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(MINOR, "ch (%02d) fw upload fail!!", cam_id);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_GENERAL, 90);
	}

	/* 5. factory default (to chg default password) */
	if(is_s1_special)
	{
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 70);
		for(i = 0; i < 7; i++)
		{
			usleep(10 * 1000 * 1000);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 70 + i * 2);
		}
		IPCAM_DBG(MINOR, "ch (%02d) S1 return to factory default !!", cam_id);
		rtn = _cam_factory_default(cam_id);
		IPCAM_DBG(MINOR, "ch (%02d) S1 factory default (1st) rslt ==> %s\n", cam_id, (rtn == IPCAM_SETUP_RTN_DONE) ? "SUCC" : "FAIL");

		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			usleep(10 * 1000 * 1000);
			rtn = _cam_factory_default(cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 70 + 7 * 2);
			IPCAM_DBG(MINOR, "ch (%02d) S1 factory default (2nd) rslt ==> %s\n", cam_id, (rtn == IPCAM_SETUP_RTN_DONE) ? "SUCC" : "FAIL");
		}
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			usleep(10 * 1000 * 1000);
			rtn = _cam_factory_default(cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 70 + 8 * 2);
			IPCAM_DBG(MINOR, "ch (%02d) S1 factory default (3rd) rslt ==> %s\n", cam_id, (rtn == IPCAM_SETUP_RTN_DONE) ? "SUCC" : "FAIL");
		}
		if(rtn != IPCAM_SETUP_RTN_DONE)
		{
			usleep(10 * 1000 * 1000);
			rtn = _cam_factory_default(cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, 70 + 9 * 2);
			IPCAM_DBG(MINOR, "ch (%02d) S1 factory default (final) rslt ==> %s\n", cam_id, (rtn == IPCAM_SETUP_RTN_DONE) ? "SUCC" : "FAIL");
		}
		rtn = IPCAM_SETUP_RTN_DONE;
	}

	return rtn;
}

static int _ssl_reboot_fwmode(int cam_id)
{
	const char set_fwmode_content[] =
		"-----------------------------7dd19424304f4\r\n"
		"Content-Disposition: form-data; name=\"control\"\r\n"
		"\r\n"
		"firmmode\r\n"
		"-----------------------------7dd19424304f4--\r\n";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_upload,
			ip_str, strlen(set_fwmode_content), auth_encbuf, set_fwmode_content);

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
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());

		if (ctx == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(ssl, sock);
	len = SSL_connect(ssl);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(ssl);
	}

	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif

	//if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	if (strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "POST";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);
		nf_ipcam_get_username(cam_id, username);
		nf_ipcam_get_password(cam_id, password);

		//snprintf(uri, 128, "http://%s:%d/cgi-bin/upload.cgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/upload.cgi");

		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_upload_digest,
				ip_str, strlen(set_fwmode_content), auth_str, set_fwmode_content);


		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif

		if (strstr(sock_buf, "Unauthorized") != NULL)
		{
			snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_upload_digest,
					ip_str, strlen(set_fwmode_content), auth_str, set_fwmode_content);


			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
	#endif

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
	#endif
		}

	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_reboot_fwmode_digest(int cam_id, char* rbuf)
{
	const char set_fwmode_content[] =
		"-----------------------------7dd19424304f4\r\n"
		"Content-Disposition: form-data; name=\"control\"\r\n"
		"\r\n"
		"firmmode\r\n"
		"-----------------------------7dd19424304f4--\r\n";

	char http_api[256];
	char auth_str[1024];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_upload_digest, ip_str,
			strlen(set_fwmode_content), auth_str, set_fwmode_content);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_reboot_fwmode(int cam_id)
{
	const char set_fwmode_content[] =
		"-----------------------------7dd19424304f4\r\n"
		"Content-Disposition: form-data; name=\"control\"\r\n"
		"\r\n"
		"firmmode\r\n"
		"-----------------------------7dd19424304f4--\r\n";

	char auth_encbuf[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;

	int rtn;
	char *s, *p;
	char buf[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_upload,
			ip_str, strlen(set_fwmode_content), auth_encbuf, set_fwmode_content);

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
			IPCAM_DBG(WARN, "ERROR | CH(%d)\n",  cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "ERROR | CH(%d)\n", cam_id);
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

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					rtn = _common_reboot_fwmode_digest(cam_id, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	close(sock);
	sock = (-1);

	return IPCAM_SETUP_RTN_DONE;
}


static int _ssl_set_fwmode(int cam_id)
{
	const char set_fwmode_raw[] =
		"action=set_setup&menu=system.manage_firmmode";

	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());

		if (ctx == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(ssl, sock);
	len = SSL_connect(ssl);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(ssl);
	}

	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif

	//if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	if (strstr(sock_buf, "Unauthorized") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "POST";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);
		nf_ipcam_get_username(cam_id, username);
		nf_ipcam_get_password(cam_id, password);

		snprintf(uri, 128, "http://%s:%d/cgi-bin/action.cgi", ip_str, http_port);
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
				ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);


		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
		len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif

		if (strstr(sock_buf, "Unauthorized") != NULL)
		{
			snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
					ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);


			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
	#endif

			memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
			len = SSL_read(ssl, sock_buf, SOCK_BUF_RECV_LENGTH);
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
	#endif
		}

	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_set_fwmode_digest(int cam_id, char* rbuf)
{
	const char set_fwmode_raw[] =
		"action=set_setup&menu=system.manage_firmmode";

	char http_api[256];
	char auth_str[1024];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.cgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.cgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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
	tv.tv_sec = 1;
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
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_set_fwmode(int cam_id)
{
	const char set_fwmode_raw[] =
		"action=set_setup&menu=system.manage_firmmode";

	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[8192];
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
	char buf[16];
	int rtn;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					rtn = _common_set_fwmode_digest(cam_id, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}


static int _ssl_befo_fw_up(int cam_id)
{
	const char set_fwmode_raw[] =
			"action=set_setup&menu=system.fw_upload";

	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];

	mtable *runtime = get_runtime();

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, 8192, str_api_raw,
			ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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
		tv.tv_sec = 15;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());

		if (ctx == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(ssl, sock);
	len = SSL_connect(ssl);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(ssl);
	}

	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG\n%s\n", sock_buf);
#endif

	memset(sock_buf, 0x00, 8192);
	len = SSL_read(ssl, sock_buf, 8191);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif

	//if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	if (strstr(sock_buf, "Unauthorized") != NULL || strstr(sock_buf, "forgotPassword") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "POST";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);
		nf_ipcam_get_username(cam_id, username);
		nf_ipcam_get_password(cam_id, password);

		snprintf(uri, 128, "http://%s:%d/cgi-bin/action.cgi", ip_str, http_port);
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		snprintf(sock_buf, 8192, str_api_raw,
				ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);


		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
#endif

		memset(sock_buf, 0x00, 8192);
		len = SSL_read(ssl, sock_buf, 8191);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif

		if (strstr(sock_buf, "Unauthorized") != NULL || strstr(sock_buf, "forgotPassword") != NULL)
		{
			snprintf(sock_buf, 8192, str_api_raw,
					ip_str, strlen(set_fwmode_raw), auth_str, set_fwmode_raw);


			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG\n%s\n", sock_buf);
	#endif

			memset(sock_buf, 0x00, 8192);
			int _offset = 0;
			do
			{
				len = SSL_read(ssl, sock_buf+_offset, (8192-_offset));
				if (len <= 0) { printf("\n\njykimjykim\n\n");break; }
				_offset += len;
			} while (strstr(sock_buf, "0\r\n\r\n") == NULL);
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
	#endif
		}

	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_befo_fw_up_digest(int cam_id, char* rbuf)
{
	const char set_fwmode_raw[] =
		"action=set_setup&menu=system.fw_upload";

	char http_api[256];
	char auth_str[1024];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
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

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/action.fcgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);


	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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

	memset(sock_buf, 0x00, 8192);
	if (recv(sock, sock_buf, 8192, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_befo_fw_up(int cam_id)
{
	const char set_fwmode_raw[] =
		"action=set_setup&menu=system.fw_upload";

	char auth_encbuf[256];
	char auth_str[1024];
	char sock_buf[8192];
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
	char buf[16];
	int rtn;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(sock_buf, 8192, str_api_raw, ip_str,
			strlen(set_fwmode_raw), auth_str, set_fwmode_raw);

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
	tv.tv_sec = 15;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(sock_buf, 0x00, 8192);
	if (recv(sock, sock_buf, 8192, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "\n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					rtn = _common_befo_fw_up_digest(cam_id, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	return IPCAM_SETUP_RTN_DONE;
}

static int _fw_upload_progress_thread(int* cam_id)
{
	NFIPCamUpgradeState state;

	while(1)
	{
		usleep(5 * 1000 * 1000); // 5 sec

		nf_ipcam_get_upgrade_state(*cam_id, &state);
		if(state.cur_progress > 89 || state.is_error || state.state == NF_IPCAM_FW_FINAL_REBOOT) break;

		if (_nf_ipcam_set_upgrade_state(*cam_id, state.state, state.is_error, state.error_no, state.cur_progress + 4))
		{
			break;
		}
	}

	g_free(cam_id);
	return 0;
}


static int _ssl_upload_fw_hisilicon(int cam_id, char *file_nm, char *file_stream, int file_len)
{
	const char upload_fw_raw[] =
"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
//"Accept: text/html, application/xhtml+xml, */*\r\n"
"Accept: */*\r\n"
"Accept-Language: ko\r\n"
"Content-Type: multipart/form-data; boundary=%s\r\n"	// boundary
"Accept-Encoding: gzip, deflate\r\n"
"User-Agent: IPX-NVR\r\n"
"Host: %s\r\n"	// IP address
"Content-Length: %d\r\n" // length
"DNT: 1"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"%s\r\n"	//
"\r\n";

	const char content_start_raw[] =
"--%s\r\n"
"Content-Disposition: form-data; name=\"control\"\r\n"
"\r\n"
"\r\n"
"--%s\r\n"	// boundary start
"Content-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
"Content-Type: application/x-gzip-compressed\r\n"
"\r\n";

	const char content_end_raw[] =
"\r\n"
"--%s--\r\n";

	const char boundary_str[] = "---------------------------7dd19424304f4";

	char auth_encbuf[256];
	char auth_str[1024];
	char start_str[1024];
	char end_str[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];
	int _offset = 0;
	int _send_len = 0;
	int _sent_len = 0;
	int _send_len_total = 0;
	int _remain_len = file_len;
	int _upload_progress = 1;
	const int _frag_len = 8192*4;
	NFIPCamUpgradeState state;

	{
		int* param;
		param = (int *)g_malloc0(sizeof(int));
		*param = cam_id;
		g_thread_create(_fw_upload_progress_thread, param, FALSE, NULL);
	}

	mtable *runtime = get_runtime();

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(start_str, 1024, content_start_raw, boundary_str, boundary_str, file_nm);
	snprintf(end_str, 256, content_end_raw, boundary_str);

	int content_len = strlen(start_str) + file_len + strlen(end_str);

	snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
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
		tv.tv_sec = 180;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());

		if (ctx == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(ssl, sock);
	len = SSL_connect(ssl);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(ssl);
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG1\n%s\n", sock_buf);
#endif

	len = SSL_write(ssl, start_str, strlen(start_str));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, start_str, strlen(start_str));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG2\n%s\n", start_str);
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}


	IPCAM_DBG(ERROR, "CH(%d) file send 1 %d\n", cam_id, __LINE__);
	{
		_send_len = 0;
		_sent_len = 0;
		_send_len_total = 0;
		_remain_len = file_len;
		_upload_progress = 1;

		while(_remain_len > 0)
		{
			_send_len = _frag_len>_remain_len?_remain_len:_frag_len;
			_sent_len = SSL_write(ssl, file_stream+_send_len_total, _send_len);

			if (_send_len != _sent_len)
			{
				IPCAM_DBG(ERROR, "CH(%d) _send_len(%d) != _sent_len(%d)\n", cam_id, _send_len, _sent_len);
				printf("==============================================\n");
				printf("  Upload IPCAM F/W CH(%d) failed report\n", cam_id);
				printf("    file length    : %d\n", file_len);
				printf("    sent data      : %d\n", _send_len_total);
				printf("    cound not send : %d\n", _remain_len);
				printf("==============================================\n");

				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

			_send_len_total += _sent_len;
			_remain_len -= _sent_len;

			if (_send_len_total > file_len/10*_upload_progress)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload progress %d%... (%d/%d)\n", cam_id, _upload_progress*10, _send_len_total, file_len);
				_upload_progress++;
			}

			if (_remain_len <= 0)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload done\n", cam_id);
				break;
			}
		}
	}
#if 0
	len = SSL_write(ssl, file_stream, file_len);
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, file_stream, file_len);
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_write(ssl, end_str, strlen(end_str));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, end_str, strlen(end_str));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}IPCAM_DBG(ERROR, "CH(%d) file send 1 %d\n", cam_id, __LINE__);
	{
		_send_len = 0;
		_sent_len = 0;
		_send_len_total = 0;
		_remain_len = file_len;
		_upload_progress = 1;

		while(_remain_len > 0)
		{
			_send_len = _frag_len>_remain_len?_remain_len:_frag_len;
			_sent_len = SSL_write(ssl, file_stream+_send_len_total, _send_len);

			if (_send_len != _sent_len)
			{
				IPCAM_DBG(ERROR, "CH(%d) _send_len(%d) != _sent_len(%d)\n", cam_id, _send_len, _sent_len);
				printf("==============================================\n");
				printf("  Upload IPCAM F/W CH(%d) failed report\n", cam_id);
				printf("    file length    : %d\n", file_len);
				printf("    sent data      : %d\n", _send_len_total);
				printf("    cound not send : %d\n", _remain_len);
				printf("==============================================\n");

				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

			_send_len_total += _sent_len;
			_remain_len -= _sent_len;

			if (_send_len_total > file_len/10*_upload_progress)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload progress %d%... (%d/%d)\n", cam_id, _upload_progress*10, _send_len_total, file_len);
				_upload_progress++;
			}

			if (_remain_len <= 0)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload done\n", cam_id);
				break;
			}
		}
	}
#if 0
	len = SSL_write(ssl, file_stream, file_len);
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, file_stream, file_len);
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_write(ssl, end_str, strlen(end_str));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, end_str, strlen(end_str));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG3\n%s\n", end_str);
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}



#if 0
	memset(sock_buf, 0x00, 8192);
	len = SSL_read(ssl, sock_buf, 8192);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif
#else
	_offset = 0;
	memset(sock_buf, 0x00, 8192);
	do
	{
		len = SSL_read(ssl, sock_buf+_offset, (8192-_offset));
		if (len <= 0) { printf("\n\njykimjykim\n\n");break; }
		_offset += len;
	} while (strstr(sock_buf, "0\r\n\r\n") == NULL);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG 1\n%s\n", sock_buf);
#endif
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}


	_release_resource(&sock, NULL, &ssl, &ctx);
	if (strstr(sock_buf, "Digest") != NULL && (strstr(sock_buf, "Unauthorized") != NULL || strstr(sock_buf, "forgotPassword") != NULL))
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "POST";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);
		nf_ipcam_get_username(cam_id, username);
		nf_ipcam_get_password(cam_id, password);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		{
			perror("socket");
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}

		{
			int ret;
			struct timeval tv;
			tv.tv_sec = 180;
			tv.tv_usec = 0;
			ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			if (ret < 0)
			{
				IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
				perror("setsockopt");
				close(sock);
				sock = (-1);
				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				return IPCAM_SETUP_RTN_FAILED;
			}
		}
		if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		{
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("connect");
			close(sock);
			sock = (-1);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if (ctx == NULL)
		{
			ctx = SSL_CTX_new(SSLv23_client_method());

			if (ctx == NULL)
			{
				IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				_release_resource(&sock, NULL, NULL, NULL);
				return IPCAM_SETUP_RTN_FAILED;
			}
			SSL_CTX_set_options(ctx, SSL_OP_ALL);
			ssl = SSL_new(ctx);
			if (ssl == NULL)
			{
				IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				_release_resource(&sock, NULL, NULL, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
		}
		SSL_set_fd(ssl, sock);
		len = SSL_connect(ssl);
		if (len < 0)
		{
			IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		if (len == 0)
		{
			IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_connect(ssl);
		}

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		//snprintf(uri, 128, "http://%s:%d/cgi-bin/upload.cgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/upload.cgi");
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);


		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL SEND MSG1\n%s\n", sock_buf);
#endif

		len = SSL_write(ssl, start_str, strlen(start_str));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, start_str, strlen(start_str));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL SEND MSG2\n%s\n", start_str);
#endif

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		IPCAM_DBG(ERROR, "CH(%d) file send 2 %d\n", cam_id, __LINE__);
		{
			_send_len = 0;
			_sent_len = 0;
			_send_len_total = 0;
			_remain_len = file_len;
			_upload_progress = 1;

			while(_remain_len > 0)
			{
				_send_len = _frag_len>_remain_len?_remain_len:_frag_len;
				_sent_len = SSL_write(ssl, file_stream+_send_len_total, _send_len);

				if (_send_len != _sent_len)
				{
					IPCAM_DBG(ERROR, "CH(%d) _send_len(%d) != _sent_len(%d)\n", cam_id, _send_len, _sent_len);
					printf("==============================================\n");
					printf("  Upload IPCAM F/W CH(%d) failed report\n", cam_id);
					printf("    file length    : %d\n", file_len);
					printf("    sent data      : %d\n", _send_len_total);
					printf("    cound not send : %d\n", _remain_len);
					printf("==============================================\n");

					_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
					_release_resource(&sock, NULL, &ssl, &ctx);
					return IPCAM_SETUP_RTN_FAILED;
				}

				_send_len_total += _sent_len;
				_remain_len -= _sent_len;

				if (_send_len_total > file_len/10*_upload_progress)
				{
					IPCAM_DBG(MINOR, "CH(%d) upload progress %d%... (%d/%d)\n", cam_id, _upload_progress*10, _send_len_total, file_len);
					_upload_progress++;
				}

				if (_remain_len <= 0)
				{
					IPCAM_DBG(MINOR, "CH(%d) upload done\n", cam_id);
					break;
				}
			}
		}
#if 0
		len = SSL_write(ssl, file_stream, file_len);
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, file_stream, file_len);
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#endif

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		len = SSL_write(ssl, end_str, strlen(end_str));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL+DIGEST write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, end_str, strlen(end_str));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL+DIGEST write failed again CH(%d)\n" , cam_id);
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG3\n%s\n", end_str);
#endif

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(sock_buf, 0x00, 8192);
		len = SSL_read(ssl, sock_buf, 8192);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG 2\n%s\n", sock_buf);
#endif
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	_release_resource(&sock, NULL, &ssl, &ctx);

	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len)
{
	const char upload_fw_raw[] =
		"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
		"Accept: text/html, application/xhtml+xml, */*\r\n"
		"Accept-Language: ko\r\n"
		"Content-Type: multipart/form-data; boundary=%s\r\n"	// boundary
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Host: %s\r\n"	// IP address
		"Content-Length: %d\r\n" // length
		"DNT: 1"
		"Connection: Keep-Alive\r\n"
		"Cache-Control: no-cache\r\n"
		"%s\r\n"	//
		"\r\n";

	const char content_start_raw[] =
		"--%s\r\n"
		"Content-Disposition: form-data; name=\"control\"\r\n"
		"\r\n"
		"\r\n"
		"--%s\r\n"	// boundary start
		"Content-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
		"Content-Type: application/x-gzip-compressed\r\n"
		"\r\n";

	const char content_end_raw[] =
		"\r\n"
		"--%s--\r\n";

	const char boundary_str[] = "---------------------------7dd19424304f4";

	char auth_encbuf[256];
	char auth_str[1024];
	char start_str[1024];
	char end_str[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int len = 0;
	int sock;
	struct sockaddr_in sin;
	unsigned char cal_mac[3];
	NFIPCamUpgradeState state;

	{
		int* param;
		param = (int *)g_malloc0(sizeof(int));
		*param = cam_id;
		g_thread_create(_fw_upload_progress_thread, param, FALSE, NULL);
	}

	mtable *runtime = get_runtime();

	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(start_str, 1024, content_start_raw, boundary_str, boundary_str, file_nm);
	snprintf(end_str, 256, content_end_raw, boundary_str);

	int content_len = strlen(start_str) + file_len + strlen(end_str);

	snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
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
		tv.tv_sec = 180;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (ctx == NULL)
	{
		ctx = SSL_CTX_new(SSLv23_client_method());

		if (ctx == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_new failed CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	SSL_set_fd(ssl, sock);
	len = SSL_connect(ssl);
	if (len < 0)
	{
		IPCAM_DBG(ERROR, "SSL_connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len == 0)
	{
		IPCAM_DBG(ERROR, "SSL connection failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_connect(ssl);
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL connect failed CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG1\n%s\n", sock_buf);
#endif

	len = SSL_write(ssl, start_str, strlen(start_str));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, start_str, strlen(start_str));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG2\n%s\n", start_str);
#endif

	len = SSL_write(ssl, file_stream, file_len);
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, file_stream, file_len);
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_write(ssl, end_str, strlen(end_str));
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
		len = SSL_get_error(ssl, len);
		PRINT_SSL_ERROR(len, cam_id);
		usleep(20*1000);
		len = SSL_write(ssl, end_str, strlen(end_str));
	}
	if (len <= 0)
	{
		IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG3\n%s\n", end_str);
#endif

	memset(sock_buf, 0x00, 8191);
	len = SSL_read(ssl, sock_buf, 8191);
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG\n%s\n", sock_buf);
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}
	//if (strstr(sock_buf, "HTTP/1.1 401 Unauthorized") != NULL)
	if (strstr(sock_buf, "Unauthorized") != NULL || strstr(sock_buf, "forgotPassword") != NULL)
	{
		char realm[128];
		char nonce[128];
		char uri[128];
		char *method = "POST";
		char *s,*e, *rbuf;
		const char *f_str_realm = "realm=\"";
		const char *f_str_nonce = "nonce=\"";

		rbuf = sock_buf;

		nf_ipcam_get_ipstr(cam_id, ip_str);
		http_port = nf_ipcam_get_http_port(cam_id);
		nf_ipcam_get_username(cam_id, username);
		nf_ipcam_get_password(cam_id, password);

		//snprintf(uri, 128, "http://%s:%d/cgi-bin/upload.cgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/upload.cgi");
		s = strstr(rbuf, f_str_realm);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_realm);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(realm, 0x00, 128);
		memcpy(realm, s, e-s);

		s = strstr(rbuf, f_str_nonce);
		if (s == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		s += strlen(f_str_nonce);
		e = strstr(s, "\"");
		if (e == NULL)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(nonce, 0x00, 128);
		memcpy(nonce, s, e-s);

		itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);

		snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);


		len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL SEND MSG1\n%s\n", sock_buf);
	#endif

		len = SSL_write(ssl, start_str, strlen(start_str));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, start_str, strlen(start_str));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL SEND MSG2\n%s\n", start_str);
	#endif

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		len = SSL_write(ssl, file_stream, file_len);
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, file_stream, file_len);
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		len = SSL_write(ssl, end_str, strlen(end_str));
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed CH(%d)\n" , cam_id);
			len = SSL_get_error(ssl, len);
			PRINT_SSL_ERROR(len, cam_id);
			usleep(20*1000);
			len = SSL_write(ssl, end_str, strlen(end_str));
		}
		if (len <= 0)
		{
			IPCAM_DBG(ERROR, "SSL write failed again CH(%d)\n" , cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
	#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL SEND MSG3\n%s\n", end_str);
	#endif

		nf_ipcam_get_upgrade_state(cam_id, &state);
		if (state.is_error)
		{
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(sock_buf, 0x00, 8192);
		len = SSL_read(ssl, sock_buf, 8191);
#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG\n%s\n", sock_buf);
#endif
#if 1
		if (strstr(sock_buf, "Unauthorized") != NULL || strstr(sock_buf, "forgotPassword") != NULL)
		{
			snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);

			len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, sock_buf, strlen(sock_buf));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
		#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "22 SSL SEND MSG1\n%s\n", sock_buf);
		#endif

			nf_ipcam_get_upgrade_state(cam_id, &state);
			if (state.is_error)
			{
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
			len = SSL_write(ssl, start_str, strlen(start_str));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, start_str, strlen(start_str));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
		#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "22 SSL SEND MSG2\n%s\n", start_str);
		#endif

			nf_ipcam_get_upgrade_state(cam_id, &state);
			if (state.is_error)
			{
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
			len = SSL_write(ssl, file_stream, file_len);
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, file_stream, file_len);
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}

			nf_ipcam_get_upgrade_state(cam_id, &state);
			if (state.is_error)
			{
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
			len = SSL_write(ssl, end_str, strlen(end_str));
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed CH(%d)\n" , cam_id);
				len = SSL_get_error(ssl, len);
				PRINT_SSL_ERROR(len, cam_id);
				usleep(20*1000);
				len = SSL_write(ssl, end_str, strlen(end_str));
			}
			if (len <= 0)
			{
				IPCAM_DBG(ERROR, "22 SSL write failed again CH(%d)\n" , cam_id);
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
		#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "22 SSL SEND MSG3\n%s\n", end_str);
		#endif

			nf_ipcam_get_upgrade_state(cam_id, &state);
			if (state.is_error)
			{
				_release_resource(&sock, NULL, &ssl, &ctx);
				return IPCAM_SETUP_RTN_FAILED;
			}
			memset(sock_buf, 0x00, 8192);
			len = SSL_read(ssl, sock_buf, 8191);
	#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "22 SSL+DIGEST RECV MSG\n%s\n", sock_buf);
	#endif

		}
#endif
	}

	_release_resource(&sock, NULL, &ssl, &ctx);

	//cam_reboot(cam_id);

	return IPCAM_SETUP_RTN_DONE;
}

static int _common_upload_fw_digest(int cam_id, char *file_nm, char *file_stream, int file_len, char *rbuf)
{
	const char upload_fw_raw[] =
"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
"Accept: text/html, application/xhtml+xml, */*\r\n"
"Accept-Language: ko\r\n"
"Content-Type: multipart/form-data; boundary=%s\r\n"	// boundary
"Accept-Encoding: gzip, deflate\r\n"
"User-Agent: IPX-NVR\r\n"
"Host: %s\r\n"	// IP address
"Content-Length: %d\r\n" // length
"DNT: 1"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"%s\r\n"	//
"\r\n";

	const char content_start_raw[] =
"--%s\r\n"
"Content-Disposition: form-data; name=\"control\"\r\n"
"\r\n"
"\r\n"
"--%s\r\n"	// boundary start
"Content-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
"Content-Type: application/x-gzip-compressed\r\n"
"\r\n";

	const char content_end_raw[] =
"\r\n"
"--%s--\r\n";

	const char boundary_str[] = "---------------------------7dd19424304f4";

	char http_api[256];
	char auth_str[1024];
	char start_str[1024];
	char end_str[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];
	char uri[128];
	char realm[128];
	char nonce[128];
	char buf[16];
	const char *method = "POST";
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";
	char *s, *e, *p;

	int _offset = 0;
	int _send_len = 0;
	int _sent_len = 0;
	int _send_len_total = 0;
	int _remain_len = file_len;
	int _upload_progress = 1;
	const int _frag_len = 8192*4;


	int len;
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int sock_state = 0;

	unsigned short http_port;
	NFIPCamUpgradeState state;

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	IPCAM_DBG(MAJOR, "start CH(%d)\n", cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (rbuf == NULL ) { return (0); }
	if (rbuf[0] == '\0') { return (0); }
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { return (0); }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { return (0); }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { return (0); }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//snprintf(uri, 128, "http://%s:%d/cgi-bin/upload.cgi", ip_str, http_port);
	snprintf(uri, 128, "/cgi-bin/upload.cgi");

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	snprintf(start_str, 1024, content_start_raw, boundary_str, boundary_str, file_nm);
	snprintf(end_str, 256, content_end_raw, boundary_str);

	int content_len = strlen(start_str) + file_len + strlen(end_str);

	snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_str);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
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

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
	printf("\n%s\n", start_str);
	printf("\n%s\n", end_str);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send start
	if(send(sock, start_str, strlen(start_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}

	IPCAM_DBG(ERROR, "CH(%d) file send 1 %d\n", cam_id, __LINE__);
	{
		_send_len = 0;
		_sent_len = 0;
		_send_len_total = 0;
		_remain_len = file_len;
		_upload_progress = 1;

		while(_remain_len > 0)
		{
			_send_len = _frag_len>_remain_len?_remain_len:_frag_len;
			_sent_len = send(sock, file_stream+_send_len_total, _send_len, 0);

			if (_send_len != _sent_len)
			{
				IPCAM_DBG(ERROR, "CH(%d) _send_len(%d) != _sent_len(%d)\n", cam_id, _send_len, _sent_len);
				printf("==============================================\n");
				printf("  Upload IPCAM F/W CH(%d) failed report\n", cam_id);
				printf("    file length    : %d\n", file_len);
				printf("    sent data      : %d\n", _send_len_total);
				printf("    cound not send : %d\n", _remain_len);
				printf("==============================================\n");
				close(sock);sock=(-1);
				return IPCAM_SETUP_RTN_FAILED;
			}

			_send_len_total += _sent_len;
			_remain_len -= _sent_len;

			if (_send_len_total > file_len/10*_upload_progress)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload progress %d%... (%d/%d)\n", cam_id, _upload_progress*10, _send_len_total, file_len);
				_upload_progress++;
			}

			if (_remain_len <= 0)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload done\n", cam_id);
				break;
			}
		}
	}

	// send file
	if(send(sock, file_stream, file_len, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send end
	if(send(sock, end_str, strlen(end_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 2 * 120;
	tv.tv_usec = 0;

	sock_state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (sock_state == 0)
	{
		IPCAM_DBG(WARN, "recv timeout CH(%d)\n", cam_id);
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(sock_buf, 0x00, 8192);
	if (recv(sock, sock_buf, 8192, 0) < 0)
	{
		IPCAM_DBG(WARN, "recv fail CH(%d)\n", cam_id);
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV CH(%d) - %s\n", cam_id, sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	return IPCAM_SETUP_RTN_DONE;
}

static int _common_upload_fw_hisilicon(int cam_id, char *file_nm, char *file_stream, int file_len)
{
	const char upload_fw_raw[] =
		//"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
		"POST /cgi-bin/upload.cgi?api HTTP/1.1\r\n"
		"Accept: text/html, application/xhtml+xml, */*\r\n"
		"Accept-Language: ko\r\n"
		"Content-Type: multipart/form-data; boundary=%s\r\n"	// boundary
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: IPX-NVR\r\n"
		"Host: %s\r\n"	// IP address
		"Content-Length: %d\r\n" // length
		//"DNT: 1"
		"Connection: Keep-Alive\r\n"
		"Cache-Control: no-cache\r\n"
		"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
		"\r\n";

	const char content_start_raw[] =
		//"--%s\r\n"
		//"Content-Disposition: form-data; name=\"control\"\r\n"
		//"\r\n"
		//"\r\n"
		"--%s\r\n"	// boundary start
		"Content-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
		//"Content-Type: application/x-gzip-compressed\r\n"
		"Content-Type: application/octet-stream\r\n"
		"\r\n";

	const char content_end_raw[] =
		"\r\n"
		"--%s--\r\n";

	const char boundary_str[] = "---------------------------7dd19424304f4";

	char auth_encbuf[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];

	char start_str[1024];
	char end_str[256];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int sock_state = 0;

	char *s, *p;
	char buf[16];
	int content_len = 0;
	int _offset = 0;
	int _send_len = 0;
	int _sent_len = 0;
	int _send_len_total = 0;
	int _remain_len = file_len;
	int _upload_progress = 1;
	const int _frag_len = 8192*4;
	NFIPCamUpgradeState state;

	{
		int* param;
		param = (int *)g_malloc0(sizeof(int));
		g_return_val_if_fail(param, IPCAM_SETUP_RTN_FAILED);
		*param = cam_id;
		g_thread_create(_fw_upload_progress_thread, param, FALSE, NULL);
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	//snprintf(start_str, 1024, content_start_raw, boundary_str, boundary_str, file_nm);
	snprintf(start_str, 1024, content_start_raw, boundary_str, file_nm);
	snprintf(end_str, 256, content_end_raw, boundary_str);

	content_len = strlen(start_str) + file_len + strlen(end_str);

	snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_encbuf);



	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5 * 60;
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

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s", sock_buf);
	printf("%s", start_str);
	printf("%s", end_str);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send start
	if(send(sock, start_str, strlen(start_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}


	IPCAM_DBG(MINOR, "CH(%d) file send 1 %d\n", cam_id, __LINE__);
	{
		_send_len = 0;
		_sent_len = 0;
		_send_len_total = 0;
		_remain_len = file_len;
		_upload_progress = 1;

		while(_remain_len > 0)
		{
			_send_len = _frag_len>_remain_len?_remain_len:_frag_len;
			_sent_len = send(sock, file_stream+_send_len_total, _send_len, 0);

			if (_send_len != _sent_len)
			{
				IPCAM_DBG(ERROR, "CH(%d) _send_len(%d) != _sent_len(%d)\n", cam_id, _send_len, _sent_len);
				printf("==============================================\n");
				printf("  Upload IPCAM F/W CH(%d) failed report\n", cam_id);
				printf("    file length    : %d\n", file_len);
				printf("    sent data      : %d\n", _send_len_total);
				printf("    cound not send : %d\n", _remain_len);
				printf("==============================================\n");
				close(sock);sock=(-1);

				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
				return IPCAM_SETUP_RTN_FAILED;
			}

			_send_len_total += _sent_len;
			_remain_len -= _sent_len;

			if (_send_len_total > file_len/10*_upload_progress)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload progress %d%... (%d/%d)\n", cam_id, _upload_progress*10, _send_len_total, file_len);
				_upload_progress++;
			}

			if (_remain_len <= 0)
			{
				IPCAM_DBG(MINOR, "CH(%d) upload done\n", cam_id);
				break;
			}
		}
	}

	// send end
	if(send(sock, end_str, strlen(end_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_UPLOAD, 1, NF_IPCAM_FW_ERR_UPLOAD, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 0, NF_IPCAM_FW_ERR_OK, state.cur_progress);

	{
		int status_ready_cnt = 0;
		int status_min_cnt = 0;
		while(1)
		{
			nf_ipcam_get_upgrade_state(cam_id, &state);

			ipcam_upgrade_ctx tmp_ctx;
			memset(&tmp_ctx, 0x00, sizeof(tmp_ctx));

			if (itx_cam_check_fwup_status(cam_id, &tmp_ctx) != IPCAM_SETUP_RTN_DONE)
			{
				g_message("[%s] CH(%d) status check failed(after upload) \n",__FUNCTION__, cam_id);
				return IPCAM_SETUP_RTN_FAILED;
			}

			sleep(1);

			if (tmp_ctx.status == IPCAM_FWUP_STATUS_MIN) // status 0 : unknown
			{
				IPCAM_DBG(MAJOR, "CH(%d) check status unknown...\n", cam_id);
				status_min_cnt++;
				if(status_min_cnt > 2)
				{
					g_message("[%s] CH(%d) status check failed (state error) -1 \n",__FUNCTION__, cam_id);
					//_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
					return IPCAM_SETUP_RTN_FAILED;
				}
				continue;
			}
			else if (tmp_ctx.status == IPCAM_FWUP_STATUS_READY)
			{
				IPCAM_DBG(MAJOR, "CH(%d) check status ready...\n", cam_id);
				status_ready_cnt++;
				if(status_ready_cnt > 10)
				{
					//IPCAM_DBG(ERROR, "CH(%d) status check failed (fixed ready state) \n", cam_id);
					g_message("[%s] CH(%d) status check failed (fixed ready state) \n",__FUNCTION__, cam_id);
					_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
					return IPCAM_SETUP_RTN_FAILED;
				}
				continue;
			}
			else if (tmp_ctx.status == IPCAM_FWUP_STATUS_VERIFYING)
			{
				IPCAM_DBG(MAJOR, "CH(%d) check status verifying...\n", cam_id);
				continue;
			}
			else if (tmp_ctx.status == IPCAM_FWUP_STATUS_WRITING)
			{
				IPCAM_DBG(MAJOR, "CH(%d) check status writing...\n", cam_id);
				continue;
			}
			else if (tmp_ctx.status == IPCAM_FWUP_STATUS_DONE)
			{
				IPCAM_DBG(MAJOR, "CH(%d) check status upgrade process done\n", cam_id);
				break;
			}
			else
			{
				g_message("[%s] CH(%d) status check failed 3 \n",__FUNCTION__, cam_id);

				_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
				return IPCAM_SETUP_RTN_FAILED;
			}

		}
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 2 * 120;
	tv.tv_usec = 0;

	sock_state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (sock_state == 0)
	{
		close(sock);
		sock = (-1);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(sock_buf, 0x00, 8192);
	if (recv(sock, sock_buf, 8191, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("upload rslt : \n%s\n", sock_buf);
#endif
#if 0
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					int rtn = _common_upload_fw_digest(cam_id, file_nm, file_stream, file_len, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
#endif

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		_nf_ipcam_set_upgrade_state(cam_id, NF_IPCAM_FW_WRITE, 1, NF_IPCAM_FW_ERR_WRITE, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}
	return IPCAM_SETUP_RTN_DONE;
}

static int _common_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len)
{
	const char upload_fw_raw[] =
"POST /cgi-bin/upload.cgi HTTP/1.1\r\n"
"Accept: text/html, application/xhtml+xml, */*\r\n"
"Accept-Language: ko\r\n"
"Content-Type: multipart/form-data; boundary=%s\r\n"	// boundary
"Accept-Encoding: gzip, deflate\r\n"
"User-Agent: IPX-NVR\r\n"
"Host: %s\r\n"	// IP address
"Content-Length: %d\r\n" // length
"DNT: 1"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
"\r\n";

	const char content_start_raw[] =
"--%s\r\n"
"Content-Disposition: form-data; name=\"control\"\r\n"
"\r\n"
"\r\n"
"--%s\r\n"	// boundary start
"Content-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
"Content-Type: application/x-gzip-compressed\r\n"
"\r\n";

	const char content_end_raw[] =
"\r\n"
"--%s--\r\n";

	const char boundary_str[] = "---------------------------7dd19424304f4";

	char auth_encbuf[256];
	char sock_buf[8192];
	char ip_str[16];
	char username[64];
	char password[64];

	char start_str[1024];
	char end_str[256];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int sock_state = 0;

	char *s, *p;
	char buf[16];
	int content_len = 0;
	NFIPCamUpgradeState state;

	{
		int* param;
		param = (int *)g_malloc0(sizeof(int));
		*param = cam_id;
		g_thread_create(_fw_upload_progress_thread, param, FALSE, NULL);
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	snprintf(start_str, 1024, content_start_raw, boundary_str, boundary_str, file_nm);
	snprintf(end_str, 256, content_end_raw, boundary_str);

	content_len = strlen(start_str) + file_len + strlen(end_str);

	snprintf(sock_buf, 8192, upload_fw_raw, boundary_str, ip_str, content_len, auth_encbuf);



	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return IPCAM_SETUP_RTN_FAILED;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 1 * 60;
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

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	printf("\n%s\n", sock_buf);
	printf("\n%s\n", start_str);
	printf("\n%s\n", end_str);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send start
	if(send(sock, start_str, strlen(start_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send file
	if(send(sock, file_stream, file_len, 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	// send end
	if(send(sock, end_str, strlen(end_str), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 1 * 120;
	tv.tv_usec = 0;

	sock_state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (sock_state == 0)
	{
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(sock_buf, 0x00, 8192);
	if (recv(sock, sock_buf, 8192, 0) < 0)
	{
		perror("recv");
		close(sock);
		sock = (-1);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		close(sock);
		return IPCAM_SETUP_RTN_FAILED;
	}

	close(sock);
	sock = (-1);

#if PRINT_HTTP_API_SEND
	printf("upload rslt : \n%s\n", sock_buf);
#endif
	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			if (strstr(sock_buf, "401 Unauthorized") != NULL)
			{
				if (strstr(sock_buf, "Digest") != NULL)
				{
					IPCAM_DBG(MINOR, "Digest goes CH(%d)\n", cam_id);
					int rtn = _common_upload_fw_digest(cam_id, file_nm, file_stream, file_len, sock_buf);
					return rtn;
				}
			}
			IPCAM_DBG(WARN, "HTTP error CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	nf_ipcam_get_upgrade_state(cam_id, &state);
	if (state.is_error)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	return IPCAM_SETUP_RTN_DONE;
}


/* CAM_ZIG_MODE scenario */

static int itx_cam_get_timezone_info(int ch, char* buf)
{
	const char set_api_raw[] =
			"action=get_setup&menu=system.datetime_cgi";

	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char *s, *p;


	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	snprintf(http_api, 1024, set_api_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return (0);
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			return (0);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("connect");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("[%s] SEND\n%s\n", __FUNCTION__, sock_buf);
#endif
	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("send");
		close(sock);
		sock = (-1);
		return (0);
	}

	memset(sock_buf, 0x00, SOCK_BUF_LENGTH);
	if (recv(sock, sock_buf, SOCK_BUF_RECV_LENGTH, 0) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, ch);
		perror("recv");
		close(sock);
		sock = (-1);
		return (0);
	}

#if PRINT_HTTP_API_SEND
	printf("[%s] RECV\n%s\n", __FUNCTION__, sock_buf);
#endif

	close(sock);
	sock = (-1);

	{
		const char f_ok[]		= "HTTP/1.1 200";
		char *errcode = sock_buf;

		errcode = strstr(errcode, f_ok);
		if (errcode == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	{
		const char f_datetime[]		= "datetime=";
		const char f_timezone[]		= "timezone=";

		/* datetime */
		s = sock_buf;
		p = strstr(s, f_datetime);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_datetime);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memset(buf, 0x00, 64);
		memcpy(buf, s, (size_t)(p - s));
		_jykim_str_replace(buf, "\%2b", "+");
		_jykim_str_replace(buf, "\%2d", "-");
		_jykim_str_replace(buf, "\%3a", ":");
		_jykim_str_replace(buf, "\%20", " ");
		_jykim_str_replace(buf, "\%2f", "/");
		strcat(buf, " ");

		/* timezone */
		s = sock_buf;
		p = strstr(s, f_timezone);
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		s = p + strlen(f_timezone);
		p = strstr(s, "&");
		if (p == NULL)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		memcpy(buf + strlen(buf), s, (size_t)(p - s));
		_jykim_str_replace(buf, "\%2b", "+");
		_jykim_str_replace(buf, "\%2d", "-");
		_jykim_str_replace(buf, "\%3a", ":");
		_jykim_str_replace(buf, "\%20", " ");
		_jykim_str_replace(buf, "\%2f", "/");
	}
	return IPCAM_SETUP_RTN_DONE;
}

static int _ssl_set_timezone_info(int ch)
{
	const char set_api_raw[] =
			"action=set_setup&menu=system.datetime_cgi&"
			"dateformat=0&timeformat=0&timezone=%s&dst=%d&"
			"set_year=%d&set_month=%02d&set_date=%02d&"
			"set_hour=%02d&set_min=%02d&set_sec=%02d";

	int len;
	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char _timezone[64];
	int _is_dst;
	int _year;
	int _month;
	int _date;
	int _hour;
	int _min;
	int _sec;

	nf_ipcam_setup_sending(ch, NF_IPCAM_TYPE_INIT);
	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	{
		gchar key_tz[64];
		gchar key_dst[64];
		gchar _tz_str[64];
		guint _tz;
		guint _dst;
		time_t timer;
		struct tm *t;

		snprintf(key_tz, 64, "sys.date.tz_index");
		snprintf(key_dst, 64, "sys.date.daylight");
		_tz = nf_sysdb_get_uint(key_tz);
		_dst = nf_sysdb_get_bool(key_dst);

		snprintf(_timezone, 64, "%s", nf_zoneinfo_get_string(_tz));
		_is_dst = _dst;

		timer = time(NULL);
		t = localtime(&timer);

		_year = t->tm_year + 1900;
		_month = t->tm_mon + 1;
		_date = t->tm_mday;
		_hour = t->tm_hour;
		_min = t->tm_min;
		_sec = t->tm_sec;
	}

	_jykim_str_replace(_timezone, "+", "\%2B");
	_jykim_str_replace(_timezone, "-", "\%2D");
	_jykim_str_replace(_timezone, ":", "\%3A");
	_jykim_str_replace(_timezone, " ", "\%20");
	_jykim_str_replace(_timezone, "/", "\%2F");

	snprintf(http_api, 1024, set_api_raw, _timezone, _is_dst,
			_year, _month, _date, _hour, _min, _sec);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(ch, NF_IPCAM_TYPE_INIT, http_api);
	len = _cam_setup_send_ssl(ch, NF_IPCAM_TYPE_INIT, sock_buf);

	return len;
}

static int _cam_set_timezone_info(int ch)
{
	const char set_api_raw[] =
			"action=set_setup&menu=system.datetime_cgi&"
			"dateformat=0&timeformat=0&timezone=%s&dst=%d&"
			"set_year=%d&set_month=%02d&set_date=%02d&"
			"set_hour=%02d&set_min=%02d&set_sec=%02d";

	int len;
	char http_api[1024];
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	char _timezone[64];
	int _is_dst;
	int _year;
	int _month;
	int _date;
	int _hour;
	int _min;
	int _sec;

	nf_ipcam_setup_sending(ch, NF_IPCAM_TYPE_INIT);
	nf_ipcam_get_ipstr(ch, ip_str);
	http_port = nf_ipcam_get_http_port(ch);
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);

	{
		gchar key_tz[64];
		gchar key_dst[64];
		gchar _tz_str[64];
		guint _tz;
		guint _dst;
		time_t timer;
		struct tm *t;

		snprintf(key_tz, 64, "sys.date.tz_index");
		snprintf(key_dst, 64, "sys.date.daylight");
		_tz = nf_sysdb_get_uint(key_tz);
		_dst = nf_sysdb_get_bool(key_dst);

		snprintf(_timezone, 64, "%s", nf_zoneinfo_get_string(_tz));
		_is_dst = _dst;

		timer = time(NULL);
		t = localtime(&timer);

		_year = t->tm_year + 1900;
		_month = t->tm_mon + 1;
		_date = t->tm_mday;
		_hour = t->tm_hour;
		_min = t->tm_min;
		_sec = t->tm_sec;

	}

	_jykim_str_replace(_timezone, "+", "\%2B");
	_jykim_str_replace(_timezone, "-", "\%2D");
	_jykim_str_replace(_timezone, ":", "\%3A");
	_jykim_str_replace(_timezone, " ", "\%20");
	_jykim_str_replace(_timezone, "/", "\%2F");

	snprintf(http_api, 1024, set_api_raw, _timezone, _is_dst,
			_year, _month, _date, _hour, _min, _sec);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str,
			strlen(http_api), auth_str, http_api);

	_set_last_api_str(ch, NF_IPCAM_TYPE_INIT, http_api);
	len = _cam_setup_send_plain(ch, NF_IPCAM_TYPE_INIT, sock_buf);

	return len;
}


static int itx_cam_get_current_info(int ch, int* current)
{
#if 0 // don't used by jykim
	int i = 0;
	NF_UTIL_POE_PORT_INFO info;

#if !defined(_IPX_0412ECO)&&!defined(_IPX_0824ECO)
	nf_event_get_zig_info(&info);
#endif
	*current = info.info[ch].current_mA;

	return IPCAM_SETUP_RTN_DONE;
#endif
}

extern int cam_zig_set_zoom(int value, int cam_id)
{
	char *set_zoom_raw = NULL;
	const char old_zoom_raw[] =
			"action=set_setup&menu=video.af&af_cmd=zoom&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_zoom_raw[] =
			"action=set_setup&menu=video.af_start&mode=zoom&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable* runtime;

	char get_spd_sysdb[64];
	int speed = 0;


	runtime = get_runtime();
	use_old = 1;

	set_zoom_raw = use_old ? old_zoom_raw:new_zoom_raw;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	if (use_old)
	{
		snprintf(http_api, 256, set_zoom_raw, value);
	}
	else
	{
		snprintf(get_spd_sysdb, 64, "cam.ptz.P%d.zoom_spd", cam_id);
		speed = nf_sysdb_get_uint(get_spd_sysdb);
		speed /= 10;
		if (value == NF_PTZ_CMD_ZOOM_WIDE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[0], speed);
		}
		else if (value == NF_PTZ_CMD_ZOOM_TELE)
		{
			snprintf(http_api, 256, set_zoom_raw, dir_str[1], speed);
		}
		else
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

	printf("%s : \n%s\n", __FUNCTION__, sock_buf);

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
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_zig_set_focus(int value, int cam_id)
{
	char *set_focus_raw = NULL;
	const char old_focus_raw[] =
			"action=set_setup&menu=video.af&af_cmd=focus&af_value=%d";

	const char *dir_str[] = { "minus", "plus" };
	const char new_focus_raw[] =
			"action=set_setup&menu=video.af_start&mode=focus&dir=%s&speed=%d";
	int use_old = 0;

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock;
	struct sockaddr_in sin;

	mtable *runtime;

	char get_spd_sysdb[64];
	int speed = 0;


	runtime = get_runtime();
	use_old = 1;

	set_focus_raw = use_old ? old_focus_raw:new_focus_raw;

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	if (use_old)
	{
		snprintf(http_api, 256, set_focus_raw, value);
	}
	else
	{
		snprintf(get_spd_sysdb, 64, "cam.ptz.P%d.focus_spd", cam_id);
		speed = nf_sysdb_get_uint(get_spd_sysdb);
		speed /= 10;
		if (value == NF_PTZ_CMD_FOCUS_NEAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[0], speed);
		}
		else if (value == NF_PTZ_CMD_FOCUS_FAR)
		{
			snprintf(http_api, 256, set_focus_raw, dir_str[1], speed);
		}
		else
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	}

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

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
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_zig_set_origin(int cam_id)
{
	const char set_origin_raw[] = "action=set_setup&menu=video.af&af_cmd=origin";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_origin_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

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
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_zig_set_onepush(int cam_id)
{
	const char set_onepush_raw[] = "action=set_setup&menu=video.af&af_cmd=oneshot";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_onepush_raw);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

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
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}

extern int cam_zig_set_iris(int value, int cam_id)
{
	const char set_piris_raw[] =
			"action=set_setup&menu=video.af&af_cmd=piris&af_value=%d";

	char http_api[256];
	char auth_encbuf[256];
	char auth_str[256];
	char *sock_buf;
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

	sock_buf = (char*) malloc(SOCK_BUF_LENGTH);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 256, set_piris_raw, value);
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw, ip_str, strlen(http_api), auth_str, http_api);

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
	close(sock);

	return IPCAM_SETUP_RTN_DONE;
}


/**
 * @brief SSL ??러코드???출력??다.
 * @param ssl_err SSL ??러코드.
 * @param cam_id 채널 번호.
 */
static void PRINT_SSL_ERROR(int ssl_err, int cam_id)
{
	switch(ssl_err)
	{
		case SSL_ERROR_NONE:
			//IPCAM_DBG(WARN, "SSL_ERROR_NONE CH%02d\n",  cam_id);
			break;
		case SSL_ERROR_ZERO_RETURN:
			IPCAM_DBG(WARN, "SSL_ERROR_ZERO_RETURN CH%02d\n", cam_id);
			break;
		case SSL_ERROR_WANT_READ:
			IPCAM_DBG(WARN, "SSL_ERROR_WANT_READ CH%02d\n", cam_id);
			break;
		case SSL_ERROR_WANT_WRITE:
			IPCAM_DBG(WARN, "SSL_ERROR_WANT_WRITE CH%02d\n", cam_id);
			break;
		case SSL_ERROR_WANT_CONNECT:
			IPCAM_DBG(WARN, "SSL_ERROR_WANT_CONNECT CH%02d\n", cam_id);
			break;
		case SSL_ERROR_WANT_ACCEPT:
			IPCAM_DBG(WARN, "SSL_ERROR_WANT_ACCEPT CH%02d\n", cam_id);
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			IPCAM_DBG(WARN, "SSL_ERROR_WANT_X509_LOOKUP CH%02d\n", cam_id);
			break;
		case SSL_ERROR_SYSCALL:
			IPCAM_DBG(WARN, "SSL_ERROR_SYSCALL CH%02d\n", cam_id);
			break;
		case SSL_ERROR_SSL:
			IPCAM_DBG(WARN, "SSL_ERROR_SSL CH%02d\n", cam_id);
			break;
		default:
			IPCAM_DBG(WARN, "SSL_ERROR_? CH%02d\n", cam_id);
			break;
	}
}

/**
 * @brief 지??경로??서 VA??정 xml??일????어??인??
 * @param p VA??정??일??????되????는 경로.
 * @param chan 채널 번호.
 * @return ???? 0.
 */
static int _va_config_xml_import(char* p, int chan){
	
	char *path = p;
	
	
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);
	
	_nf_sysdb_load_internal("cam", path, (1<<2));
	nf_sysdb_save("cam");
	DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, 0x1 << chan);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	//evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);

	return 0;

}

/**
 * @brief Sysdb??서 VA??정????어??여 xml??식??로 반환??다.
 * @param[out] p ????xml문자??
 * @param[in] chan 채널 번호.
 * @return ???? 0.
 */
static int _va_config_xml_export(char* p, int chan, int is_modified)
{
	
	VCAPropData data[NUM_ACTIVE_CH];
	ivca_option_t opt;
	ivca_rule_t rule;
	gchar key[64], *str;
	char temp[64];
	ivca_zone_t *zonelist;
	ivca_cntr_t*cntrlist;
	int i,j;
	char *ptemp;
	ivca_calib_t cal;

	memset(&rule, 0x00, sizeof(rule));

	prvGetVCARuleData(&rule,chan);
	DAL_get_vca_prop_data_all(&data,NUM_ACTIVE_CH);
	prvGetVCAOptData(&opt,chan);
	prvGetVCACalibData(&cal,chan);

	sprintf(key, "cam.vca.cfg.R%u.sched", chan);
	str = nf_sysdb_get_str_nocopy(key);
	
	//p += sprintf(p, "<nf_sysdb>\r\n");
	//p += sprintf(p, "<sys>\r\n");
	
	p += sprintf(p, "<vca>\r\n");
	p += sprintf(p, "<item key=\"cam.vca.cfg.R%d.act\" type=\"BOOL\" val=\"%u\" />\r\n",chan, data[chan].active);
	p += sprintf(p, "<item key=\"cam.vca.cfg.R%d.day\" type=\"UINT\" val=\"0\" />\r\n",chan);
	p += sprintf(p, "<item key=\"cam.vca.cfg.R%d.detect\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data[chan].detect);
	p += sprintf(p, "<item key=\"cam.vca.cfg.R%d.sched\" type=\"STRING\" val=\"%s\" />\r\n",chan,str);
	p += sprintf(p, "<item key=\"cam.vca.cfg.R%d.unit\" type=\"UINT\" val=\"%u\" />\r\n",chan,data[chan].unit);

	// rule
	p += sprintf(p, "<item key=\"cam.vca.rule.R%d.n_height\" type=\"UINT\" val=\"%d\" />\r\n",chan,rule.n_height);
	p += sprintf(p, "<item key=\"cam.vca.rule.R%d.n_width\" type=\"UINT\" val=\"%d\" />\r\n",chan,rule.n_width);	
	p += sprintf(p, "<item key=\"cam.vca.rule.R%d.ncounters\" type=\"UINT\" val=\"%d\" />\r\n",chan,rule.ncntrs);
	p += sprintf(p, "<item key=\"cam.vca.rule.R%d.nzones\" type=\"UINT\" val=\"%d\" />\r\n",chan,rule.nzones);	
	
	// zone
	zonelist = rule.zonelist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,zonelist[i].active);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(zonelist[i].color[0]) |(zonelist[i].color[1] << 8)|(zonelist[i].color[2] << 16));
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].enabled);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.f_class\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].eclass);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.f_colorsens\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(zonelist[i].ecolor_sens <<24) | (zonelist[i].ecolor[0]) |(zonelist[i].ecolor[1] << 8)|(zonelist[i].ecolor[2] << 16));
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.f_size\" type=\"STRING\" val=\"0 0 0 0\" />\r\n",chan,i);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.f_speed\" type=\"STRING\" val=\"0 0\" />\r\n",chan,i);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,zonelist[i].id);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,zonelist[i].name);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].npts);
		ptemp = temp;
		for(j=0;j<zonelist[i].npts;j++)
			ptemp += sprintf(ptemp, "%d %d ",zonelist[i].pt[j].x,zonelist[i].pt[j].y);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d %d",zonelist[i].stop_time,zonelist[i].abandon_time,zonelist[i].remove_time,zonelist[i].loiter_time,zonelist[i].fall_time);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.time_sarlf\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].type);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.Z%d.sensitivity\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].sensitivity);
			

	}

	// counter
	cntrlist = rule.cntrlist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,cntrlist[i].active);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(cntrlist[i].color[0]) |(cntrlist[i].color[1] << 8)|(cntrlist[i].color[2] << 16));
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.e_value\" type=\"INT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].evalue);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].enabled);

		if ( is_modified )
			cntrlist[i].id += 16;
		
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].id);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,cntrlist[i].name);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d %d %d %d %d",cntrlist[i].pt[0].x,cntrlist[i].pt[0].y,cntrlist[i].pt[1].x,cntrlist[i].pt[1].y,cntrlist[i].pt[2].x,cntrlist[i].pt[2].y,cntrlist[i].pt[3].x,cntrlist[i].pt[3].y);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.reset_alert\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,cntrlist[i].resetalert);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d",cntrlist[i].zid_up,cntrlist[i].zid_dn);	
		p += sprintf(p, "<item key=\"cam.vca.rule.R%d.C%d.zid\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);

	}

	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_prediction\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_prediction);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_roi\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_roi);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_shadowrm\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_shadowrm);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_snapshot\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_snapshot);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_tamper\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_tamper);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.en_privacy\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.en_privacy);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d %d %d",opt.roi.x,opt.roi.y,opt.roi.w,opt.roi.h);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.roi_xywh\" type=\"STRING\" val=\"%s\" />\r\n",chan,temp);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_dbg_fg\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_dbg_fg);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_dbg_info\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_dbg_info);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_dbg_sh\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_dbg_sh);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_ar\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_ar);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_id\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_id);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_tm\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_tm);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_cl\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_cl);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_tr\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_tr);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_s3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_s3d);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_w3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_w3d);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_obj_h3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_obj_h3d);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_roi\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_roi);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_rule);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_rule_name);
	p += sprintf(p, "<item key=\"cam.vca.opt.R%d.track_ref\" type=\"UINT\" val=\"%d\" />\r\n",chan,opt.track_ref);

	//cal data
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.focal\" type=\"STRING\" val=\"%f\" />\r\n",chan,cal.focal);
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.height\" type=\"STRING\" val=\"%f\" />\r\n",chan,cal.height);
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.tilt\" type=\"STRING\" val=\"%f\" />\r\n",chan,cal.tilt);
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.ntargets\" type=\"UINT\" val=\"%d\" />\r\n",chan,cal.ntargets);	
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.p_height\" type=\"UINT\" val=\"%d\" />\r\n",chan,cal.p_height);	
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.p_width\" type=\"UINT\" val=\"%d\" />\r\n",chan,cal.p_width);	
	p += sprintf(p, "<item key=\"cam.vca.calib.R%d.paramvalid\" type=\"BOOL\" val=\"%u\" />\r\n",chan,cal.paramvalid);	

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++) {
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d",cal.targetlist[i].pt[0].x,cal.targetlist[i].pt[0].y,cal.targetlist[i].pt[1].x,cal.targetlist[i].pt[1].y);
		p += sprintf(p, "<item key=\"cam.vca.calib.R%d.T%u.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		p += sprintf(p, "<item key=\"cam.vca.calib.R%d.T%u.height\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,cal.targetlist[i].height);	
	}
	
	p += sprintf(p, "</vca>\r\n");
	//p += sprintf(p, "</sys>\r\n");
	//p += sprintf(p, "</nf_sysdb>\r\n");

	return 0;
}

extern int __encap_va_xml_export(char* p, int chan, int is_modified)
{
	return (_va_config_xml_export(p,chan,is_modified));
}
extern int __encap_va_xml_import(char* p, int chan)
{
	return (_va_config_xml_import(p,chan));
}

/**
 * @brief 지??경로??서 VA??정 xml??일????어??인??
 * @param p VA??정??일??????되????는 경로.
 * @param chan 채널 번호.
 * @return ???? 0.
 */
static int _ai_config_xml_import(char* p, int chan){
	
	char *path = p;
	
	
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);
	
	_nf_sysdb_load_internal("cam", path, (1<<2));
	nf_sysdb_save("cam");
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	//evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);

	return 0;

}

/**
 * @brief Sysdb??서 VA??정????어??여 xml??식??로 반환??다.
 * @param[out] p ????xml문자??
 * @param[in] chan 채널 번호.
 * @return ???? 0.
 */
static int _ai_config_xml_export(char* p, int chan, int is_modified)
{
	
	DvaBxData data;
	ivca_option_t opt;
	ivca_rule_t rule;
	gchar key[64], *str;
	char temp[64];
	ivca_zone_t *zonelist;
	ivca_cntr_t*cntrlist;
	int i,j;
	char *ptemp;
	ivca_calib_t cal;
	fr_lpr_rule_t fr_rule;
	fr_lpr_rule_t lpr_rule;
	fr_lpr_option_t fr_opt;
	fr_lpr_option_t lpr_opt;
	fr_lpr_zone_t *fr_lpr_zonelist;
	

	

	memset(&rule, 0x00, sizeof(rule));
	memset(&data, 0x00, sizeof(data));

	DAL_get_dvabx_data(&data, chan);

	prvGetAIRuleData(&rule, chan);

	prvGetFRRuleData(&fr_rule, chan);
	prvGetFROptData(&fr_opt, chan);
	prvGetLPRRuleData(&lpr_rule, chan);
	prvGetLPROptData(&lpr_opt, chan);

	sprintf(key, "cam.dvabx.cfg.R%u.sched", chan);
	str = nf_sysdb_get_str_nocopy(key);
	
	p += sprintf(p, "<dvabx>\r\n");
	p += sprintf(p, "<item key=\"cam.dvabx.cfg.R%d.act\" type=\"BOOL\" val=\"%u\" />\r\n",chan, data.prop.active);
	p += sprintf(p, "<item key=\"cam.dvabx.cfg.R%d.day\" type=\"UINT\" val=\"0\" />\r\n",chan);
	p += sprintf(p, "<item key=\"cam.dvabx.cfg.R%d.detect\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.detect);
	p += sprintf(p, "<item key=\"cam.dvabx.cfg.R%d.sched\" type=\"STRING\" val=\"%s\" />\r\n",chan,str);
	p += sprintf(p, "<item key=\"cam.dvabx.cfg.R%d.unit\" type=\"UINT\" val=\"%u\" />\r\n",chan,data.prop.unit);

	// rule
	p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.ncounters\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.cntrlist.cnt);
	p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.nzones\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.zonelist.cnt);	

	// zone
	zonelist = rule.zonelist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
		if(i < data.zonelist.cnt){
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,zonelist[i].active);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(zonelist[i].color[0]) |(zonelist[i].color[1] << 8)|(zonelist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].enabled);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.f_class\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].eclass);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.f_colorsens\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(zonelist[i].ecolor_sens <<24) | (zonelist[i].ecolor[0]) |(zonelist[i].ecolor[1] << 8)|(zonelist[i].ecolor[2] << 16));
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.f_size\" type=\"STRING\" val=\"0 0 0 0\" />\r\n",chan,i);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.f_speed\" type=\"STRING\" val=\"0 0\" />\r\n",chan,i);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,zonelist[i].id);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,zonelist[i].name);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].npts);
			ptemp = temp;
			for(j=0;j<zonelist[i].npts;j++)
				ptemp += sprintf(ptemp, "%d %d ",zonelist[i].pt[j].x,zonelist[i].pt[j].y);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d",zonelist[i].stop_time,zonelist[i].abandon_time,zonelist[i].remove_time,zonelist[i].loiter_time,zonelist[i].fall_time);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.time_sarlf\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].type);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.sensitivity\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].sensitivity);
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.c_threshold\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,zonelist[i].c_threshold);
		}

		{
			
			GValue ret_value = {0,};
			p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.all_detect_obj\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,zonelist[i].all_detect_obj);
			if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.interest_obj", chan, i, &ret_value, NULL))
				p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.Z%d.interest_obj\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,g_value_get_string(&ret_value));
		}
	}

	// counter
	cntrlist = rule.cntrlist;
	for(i=0;i<data.cntrlist.cnt;i++){
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,cntrlist[i].active);
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(cntrlist[i].color[0]) |(cntrlist[i].color[1] << 8)|(cntrlist[i].color[2] << 16));
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.e_value\" type=\"INT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].evalue);
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].enabled);
		
		if ( is_modified )
			cntrlist[i].id += 16;
		
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,cntrlist[i].id);
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,cntrlist[i].name);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d %d %d %d %d",cntrlist[i].pt[0].x,cntrlist[i].pt[0].y,cntrlist[i].pt[1].x,cntrlist[i].pt[1].y,cntrlist[i].pt[2].x,cntrlist[i].pt[2].y,cntrlist[i].pt[3].x,cntrlist[i].pt[3].y);
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.reset_alert\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,cntrlist[i].resetalert);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d",cntrlist[i].zid_up,cntrlist[i].zid_dn);	
		p += sprintf(p, "<item key=\"cam.dvabx.rule.R%d.C%d.zid\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);

	}

//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_prediction\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_roi\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_roi);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_shadowrm\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_shadowrm);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_snapshot\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_snapshot);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_tamper\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_tamper);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_privacy\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_privacy);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d %d %d",data.prop.roi_rect.x,data.prop.roi_rect.y,data.prop.roi_rect.w,data.prop.roi_rect.h);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.roi_xywh\" type=\"STRING\" val=\"%s\" />\r\n",chan,temp);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_dbg_fg\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_dbg_fg);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_dbg_info\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_dbg_info);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_dbg_sh\" type=\"BOOL\" val=\"%u\" />\r\n",chan,opt.sw_dbg_sh);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_ar\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_ar);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_bb);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_id\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_id);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_tm\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_tm);
//	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_cl\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_cl);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_tr\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_tr);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_s3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_s3d);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_w3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_w3d);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_obj_h3d\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_obj_h3d);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_roi\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_roi);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_rule);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.sw_rule_name);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.track_ref\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.prop.track_ref);

	
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.en_static_filter\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.prop.en_static_filter);
	p += sprintf(p, "<item key=\"cam.dvabx.opt.R%d.static_filter_sense\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.prop.static_filter_sense);

	//cal data
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.focal\" type=\"STRING\" val=\"%f\" />\r\n",chan,data.calbres.focal);
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.height\" type=\"STRING\" val=\"%f\" />\r\n",chan,data.calbres.height);
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.tilt\" type=\"STRING\" val=\"%f\" />\r\n",chan,data.calbres.tilt);
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.ntargets\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.calblist.cnt);	
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.p_height\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.calbres.p_height);	
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.p_width\" type=\"UINT\" val=\"%d\" />\r\n",chan,data.calbres.p_width);	
	p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.paramvalid\" type=\"BOOL\" val=\"%u\" />\r\n",chan,data.calbres.paramvalid);	

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++) {
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d",data.calblist.calb[i].pt[0].x,data.calblist.calb[i].pt[0].y,data.calblist.calb[i].pt[1].x,data.calblist.calb[i].pt[1].y);
		p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.T%u.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
		p += sprintf(p, "<item key=\"cam.dvabx.calib.R%d.T%u.height\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,data.calblist.calb[i].height);	
	}

	// FR Data
	// FR Option
	
	p += sprintf(p, "<item key=\"cam.dvabx.face.opt.R%d.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",chan,fr_opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"cam.dvabx.face.opt.R%d.sw_grp_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,fr_opt.sw_grp_name);
	p += sprintf(p, "<item key=\"cam.dvabx.face.opt.R%d.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",chan,fr_opt.sw_rule);
	p += sprintf(p, "<item key=\"cam.dvabx.face.opt.R%d.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,fr_opt.sw_rule_name);
	
	// FR Rule

	fr_lpr_zonelist = fr_rule.zonelist;
	for(i=0;i<FR_LPR_MAX_ZONES;i++){
		if(i < fr_rule.nzones){
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,fr_lpr_zonelist[i].name);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].id);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].type);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.act\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,fr_lpr_zonelist[i].active);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(fr_lpr_zonelist[i].color[0]) |(fr_lpr_zonelist[i].color[1] << 8)|(fr_lpr_zonelist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].npts);
			memset(temp, 0x00, sizeof(temp));
			ptemp = temp;
			for(j=0;j<fr_lpr_zonelist[i].npts;j++)
				ptemp += sprintf(ptemp, "%d %d ",fr_lpr_zonelist[i].pt[j].x,fr_lpr_zonelist[i].pt[j].y);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.threshold\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].c_threshold);
			p += sprintf(p, "<item key=\"cam.dvabx.face.rule.R%d.Z%d.group_filter\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,fr_lpr_zonelist[i].group_filter);
		}
	}

	// LPR Data
	// LPR Option
	
	p += sprintf(p, "<item key=\"cam.dvabx.plateno.opt.R%d.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",chan,lpr_opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"cam.dvabx.plateno.opt.R%d.sw_grp_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,lpr_opt.sw_grp_name);
	p += sprintf(p, "<item key=\"cam.dvabx.plateno.opt.R%d.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",chan,lpr_opt.sw_rule);
	p += sprintf(p, "<item key=\"cam.dvabx.plateno.opt.R%d.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",chan,lpr_opt.sw_rule_name);
	p += sprintf(p, "<item key=\"cam.dvabx.plateno.opt.R%d.sw_plate_number\" type=\"BOOL\" val=\"%u\" />\r\n",chan,lpr_opt.sw_plate_number);

	// LPR Rule

	fr_lpr_zonelist = lpr_rule.zonelist;
	for(i=0;i<FR_LPR_MAX_ZONES;i++){
		if(i < lpr_rule.nzones){
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,fr_lpr_zonelist[i].name);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].id);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].type);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.act\" type=\"BOOL\" val=\"%u\" />\r\n",chan,i,fr_lpr_zonelist[i].active);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,(fr_lpr_zonelist[i].color[0]) |(fr_lpr_zonelist[i].color[1] << 8)|(fr_lpr_zonelist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].npts);
			memset(temp, 0x00, sizeof(temp));
			ptemp = temp;
			for(j=0;j<fr_lpr_zonelist[i].npts;j++)
				ptemp += sprintf(ptemp, "%d %d ",fr_lpr_zonelist[i].pt[j].x,fr_lpr_zonelist[i].pt[j].y);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",chan,i,temp);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.threshold\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].c_threshold);
			p += sprintf(p, "<item key=\"cam.dvabx.plateno.rule.R%d.Z%d.grp_mask\" type=\"UINT\" val=\"%d\" />\r\n",chan,i,fr_lpr_zonelist[i].grp_mask);
		}
	}
	
	p += sprintf(p, "</dvabx>\r\n");
	//p += sprintf(p, "</sys>\r\n");
	//p += sprintf(p, "</nf_sysdb>\r\n");

	return 0;
}

extern int __encap_ai_xml_export(char* p, int chan, int is_modified)
{
	return (_ai_config_xml_export(p,chan,is_modified));
}
extern int __encap_ai_xml_import(char* p, int chan)
{
	return (_ai_config_xml_import(p,chan));
}
static int _ssl_set_va_config(ivca_rule_t* value, int cam_id)
{
	char http_api[24*1024];
	char *p;
	char *ptemp;
	char auth_encbuf[256];
	char sock_buf[1024*24];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int i,j;
	ivca_zone_t *zonelist;
	ivca_cntr_t*cntrlist;
	char temp[64];

	int sock;
	struct sockaddr_in sin;
	int len = 0;
	gchar key[64], *str;

	NF_IPCAM_SETUP_TYPE_E type = NF_IPCAM_TYPE_SET_VA_CONFIG;

	mtable *runtime = get_runtime();

	int enable;
	VCAPropData data[NUM_ACTIVE_CH];

	ivca_option_t opt;
	ivca_calib_t cal;


	DAL_get_vca_prop_data_all(&data,NUM_ACTIVE_CH);
	enable = data[cam_id].active;

	prvGetVCAOptData(&opt,cam_id);

	prvGetVCACalibData(&cal,cam_id);

	nf_ipcam_get_ipstr(cam_id, ip_str);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);

	
	sprintf(key, "cam.vca.cfg.R%u.sched", cam_id);
	str = nf_sysdb_get_str_nocopy(key);

	p = http_api;
	
///////////////

#if 1

	p += sprintf(p, "-----------------------------7db301281e86059e\r\n");
	p += sprintf(p, "Content-Disposition: form-data; name=\"vca\"; filename=\"rule.xml\"\r\n");
	p += sprintf(p, "Content-Type: application/octet-stream\r\n");

	p += sprintf(p, "\r\n");
	p += sprintf(p, "<vca>\r\n");
	//option
	
	p += sprintf(p, "<item key=\"vca.cfg.R0.act\" type=\"BOOL\" val=\"%d\" />\r\n", enable);
//	p += sprintf(p, "<item key=\"vca.cfg.R0.day\" type=\"UINT\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.cfg.R0.detect\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.cfg.R0.sched\" type=\"STRING\" val=\"%s\" />\r\n",str);
	p += sprintf(p, "<item key=\"vca.cfg.R0.unit\" type=\"UINT\" val=\"%d\" />\r\n", data[cam_id].unit);
/*	p += sprintf(p, "<item key=\"vca.cfg.R0.sched\" type=\"STRING\" val=\".\" />\r\n");
	p += sprintf(p, "<item key=\"vca.cfg.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.en_prediction\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.en_roi\" type=\"BOOL\" val=\"0\" />\r\n ");
	p += sprintf(p, "<item key=\"vca.opt.R0.en_shadowrm\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.en_snapshot\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.en_tamper\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.roi_xywh\" type=\"STRING\" val=\"120 90 3600 1980\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_fg\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_info\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_sh\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_ar\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_bb\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_id\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tm\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_cl\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tr\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_roi\" type=\"BOOL\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule_name\" type=\"BOOL\" val=\"1\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.R0.track_ref\" type=\"UINT\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.opt.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	*/

	p += sprintf(p, "<item key=\"vca.opt.R0.en_usecalib\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_usecalib);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d",opt.min_width3d,opt.min_height3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.min_objsize\" type=\"STRING\" val=\"%s\" />\r\n",temp);

//cal data

	p += sprintf(p, "<item key=\"vca.calib.R0.focal\" type=\"STRING\" val=\"%f\" />\r\n",cal.focal);
	p += sprintf(p, "<item key=\"vca.calib.R0.height\" type=\"STRING\" val=\"%f\" />\r\n",cal.height);
	p += sprintf(p, "<item key=\"vca.calib.R0.tilt\" type=\"STRING\" val=\"%f\" />\r\n",cal.tilt);
	p += sprintf(p, "<item key=\"vca.calib.R0.ntargets\" type=\"UINT\" val=\"%d\" />\r\n",cal.ntargets);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_height\" type=\"UINT\" val=\"%d\" />\r\n",cal.p_height);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_width\" type=\"UINT\" val=\"%d\" />\r\n",cal.p_width);	
	p += sprintf(p, "<item key=\"vca.calib.R0.paramvalid\" type=\"UINT\" val=\"%d\" />\r\n",cal.paramvalid);	

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++) {
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d",cal.targetlist[i].pt[0].x,cal.targetlist[i].pt[0].y,cal.targetlist[i].pt[1].x,cal.targetlist[i].pt[1].y);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.pt\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.height\" type=\"UINT\" val=\"%d\" />\r\n",i,cal.targetlist[i].height);	

	}


////
			
	// rule
	p += sprintf(p, "<item key=\"vca.rule.R0.n_height\" type=\"UINT\" val=\"%d\" />\r\n",value->n_height);
	p += sprintf(p, "<item key=\"vca.rule.R0.n_width\" type=\"UINT\" val=\"%d\" />\r\n",value->n_width);	
	p += sprintf(p, "<item key=\"vca.rule.R0.ncounters\" type=\"UINT\" val=\"%d\" />\r\n",value->ncntrs);
	p += sprintf(p, "<item key=\"vca.rule.R0.nzones\" type=\"UINT\" val=\"%d\" />\r\n",value->nzones);	
	p += sprintf(p, "<item key=\"vca.rule.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	

int idx = 0;
	// zone
	zonelist = value->zonelist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
//		if(i < value->nzones){
		if (zonelist[i].active == 0) continue;
		else {
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",idx,zonelist[i].active);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",idx,(zonelist[i].color[0]) |(zonelist[i].color[1] << 8)|(zonelist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",idx,zonelist[i].enabled);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_class\" type=\"UINT\" val=\"%d\" />\r\n",idx,zonelist[i].eclass);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_colorsens\" type=\"UINT\" val=\"%d\" />\r\n",idx,(zonelist[i].ecolor_sens <<24) | (zonelist[i].ecolor[0]) |(zonelist[i].ecolor[1] << 8)|(zonelist[i].ecolor[2] << 16));
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_size\" type=\"STRING\" val=\"0 0 0 0\" />\r\n",idx);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_speed\" type=\"STRING\" val=\"0 0\" />\r\n",idx);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",idx,zonelist[i].id);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",idx,zonelist[i].name);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",idx,zonelist[i].npts);
			ptemp = temp;
			for(j=0;j<zonelist[i].npts;j++)
				ptemp += sprintf(ptemp, "%d %d ",zonelist[i].pt[j].x,zonelist[i].pt[j].y);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",idx,temp);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d",zonelist[i].stop_time,zonelist[i].abandon_time,zonelist[i].remove_time,zonelist[i].loiter_time,zonelist[i].fall_time);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.time_sarlf\" type=\"STRING\" val=\"%s\" />\r\n",idx,temp);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",idx,zonelist[i].type);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.sensitivity\" type=\"UINT\" val=\"%d\" />\r\n",idx,zonelist[i].sensitivity);
			

		}
/*		else{
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.active\" type=\"BOOL\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.d_color\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.enabled\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_class\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_colorsens\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_size\" type=\"STRING\" val=\"0 0 0 0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_speed\" type=\"STRING\" val=\"0 0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.id\" type=\"INT\" val=\"-1\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.name\" type=\"STRING\" val=\"\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.npts\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.pt\" type=\"STRING\" val=\"\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.time_sarlf\" type=\"STRING\" val=\"\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.type\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.sensitivity\" type=\"UINT\" val=\"0\" />\r\n",i);
		}*/
idx++;
	}

	// counter
	cntrlist = value->cntrlist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
//		if(i < value->nzones){
		if (cntrlist[i].active == 0) continue;
		else {
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",i,cntrlist[i].active);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",i,(cntrlist[i].color[0]) |(cntrlist[i].color[1] << 8)|(cntrlist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.e_value\" type=\"INT\" val=\"%d\" />\r\n",i,cntrlist[i].evalue);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",i,cntrlist[i].enabled);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.id\" type=\"INT\" val=\"%d\" />\r\n",i,cntrlist[i].id);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.name\" type=\"STRING\" val=\"%s\" />\r\n",i,cntrlist[i].name);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d %d %d %d",cntrlist[i].pt[0].x,cntrlist[i].pt[0].y,cntrlist[i].pt[1].x,cntrlist[i].pt[1].y,cntrlist[i].pt[2].x,cntrlist[i].pt[2].y,cntrlist[i].pt[3].x,cntrlist[i].pt[3].y);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.reset_alert\" type=\"BOOL\" val=\"%u\" />\r\n",i,cntrlist[i].resetalert);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d",cntrlist[i].zid_up,cntrlist[i].zid_dn);	
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.zid\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);

		}
/*		else{
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.active\" type=\"BOOL\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.d_color\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.e_value\" type=\"INT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.enabled\" type=\"UINT\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.id\" type=\"INT\" val=\"-1\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.name\" type=\"STRING\" val=\"\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.pt\" type=\"STRING\" val=\"\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.reset_alert\" type=\"BOOL\" val=\"0\" />\r\n",i);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.zid\" type=\"STRING\" val=\"-1 -1\" />\r\n",i);
		}*/
	}

	p += sprintf(p, "<item key=\"vca.opt.R0.en_prediction\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_prediction);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_roi\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_shadowrm\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_shadowrm);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_snapshot\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_snapshot);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_tamper\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_tamper);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_privacy\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_privacy);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d %d %d",opt.roi.x,opt.roi.y,opt.roi.w,opt.roi.h);
	p += sprintf(p, "<item key=\"vca.opt.R0.roi_xywh\" type=\"STRING\" val=\"%s\" />\r\n",temp);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_fg\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_fg);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_info\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_info);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_sh\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_sh);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_ar\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_ar);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_id\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_id);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tm\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_tm);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_cl\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_cl);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tr\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_tr);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_s3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_s3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_w3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_w3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_h3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_h3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_roi\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_rule);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_rule_name);
	p += sprintf(p, "<item key=\"vca.opt.R0.track_ref\" type=\"UINT\" val=\"%d\" />\r\n",opt.track_ref);
	p += sprintf(p, "<item key=\"vca.opt.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	
	p += sprintf(p, "</vca>\r\n");
	p += sprintf(p, "\r\n");
	
	p += sprintf(p, "-----------------------------7db301281e86059e--\r\n");
	p += sprintf(p, "\r\n");

	#endif
///////////

#if PRINT_HTTP_API_SEND
	printf("[%s] CH(%d) VA_CONFIG\n%s\n", __FUNCTION__, cam_id, http_api);
#endif

	snprintf(sock_buf, 1024*24, str_api_raw_va, ip_str,
			strlen(http_api), auth_encbuf, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VA_CONFIG, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_VA_CONFIG, sock_buf);

	return len;
}

static int _common_set_va_config(ivca_rule_t* value, int cam_id)
{
	//const char set_va_config_raw[] =
			//"action=set_setup.vca.alg&enable=%d";
		//	"action=set_setup&menu=vca.restart";

	char http_api[64*1024];
	char *p;
	char *ptemp;
	char auth_encbuf[256];
	char sock_buf[1024*64];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int i,j;
	int len;
	ivca_zone_t *zonelist;
	ivca_cntr_t*cntrlist;
	char temp[64];

	int sock;
	struct sockaddr_in sin;

	int enable;
	VCAPropData data[NUM_ACTIVE_CH];

	ivca_option_t opt;
	ivca_calib_t cal;
	gchar key[64], *str;

	mtable *runtime = get_runtime();

	DAL_get_vca_prop_data_all(&data,NUM_ACTIVE_CH);
	enable = data[cam_id].active;

	prvGetVCAOptData(&opt,cam_id);
	
	prvGetVCACalibData(&cal,cam_id);

	NF_IPCAM_SETUP_TYPE_E type = NF_IPCAM_TYPE_SET_VA_CONFIG;

	printf("cam_set_va_config ENTER enable %d nzones %d  ncntrs %d @@@@@@\n",enable,value->nzones,value->ncntrs);


	nf_ipcam_setup_sending(cam_id, type);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	//temp = (char*) malloc(64);
	//http_api = (char*) malloc(1024*8);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	//snprintf(http_api, 256, set_enable_va_raw, value);
	//snprintf(http_api, 256, set_va_config_raw);

//	zonelist = value->zonelist;
//	printf("z1 active %d npts %d \n ",(zonelist[0].active),(zonelist[0].npts));
//	printf("z2 active %d npts %d \n ",(zonelist[1].active),(zonelist[1].npts));


	sprintf(key, "cam.vca.cfg.R%u.sched", cam_id);
	str = nf_sysdb_get_str_nocopy(key);

	p = http_api;
	
///////////////

#if 1

	p += sprintf(p, "-----------------------------7db301281e86059e\r\n");
	p += sprintf(p, "Content-Disposition: form-data; name=\"vca\"; filename=\"rule.xml\"\r\n");
	p += sprintf(p, "Content-Type: application/octet-stream\r\n");

	p += sprintf(p, "\r\n");
	p += sprintf(p, "<vca>\r\n");
	//option
	
	p += sprintf(p, "<item key=\"vca.cfg.R0.act\" type=\"BOOL\" val=\"%d\" />\r\n", enable);
//	p += sprintf(p, "<item key=\"vca.cfg.R0.day\" type=\"UINT\" val=\"0\" />\r\n");
	p += sprintf(p, "<item key=\"vca.cfg.R0.detect\" type=\"BOOL\" val=\"%d\" />\r\n",data[cam_id].detect);
	p += sprintf(p, "<item key=\"vca.cfg.R0.unit\" type=\"UINT\" val=\"%d\" />\r\n",data[cam_id].unit);
	p += sprintf(p, "<item key=\"vca.cfg.R0.sched\" type=\"STRING\" val=\"%s\" />\r\n",str);
	
#if 1	
//cal data

	p += sprintf(p, "<item key=\"vca.calib.R0.focal\" type=\"STRING\" val=\"%f\" />\r\n",cal.focal);
	p += sprintf(p, "<item key=\"vca.calib.R0.height\" type=\"STRING\" val=\"%f\" />\r\n",cal.height);
	p += sprintf(p, "<item key=\"vca.calib.R0.tilt\" type=\"STRING\" val=\"%f\" />\r\n",cal.tilt);
	p += sprintf(p, "<item key=\"vca.calib.R0.ntargets\" type=\"UINT\" val=\"%d\" />\r\n",cal.ntargets);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_height\" type=\"UINT\" val=\"%d\" />\r\n",cal.p_height);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_width\" type=\"UINT\" val=\"%d\" />\r\n",cal.p_width);	
	p += sprintf(p, "<item key=\"vca.calib.R0.paramvalid\" type=\"BOOL\" val=\"%u\" />\r\n",cal.paramvalid);	

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++) {
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d",cal.targetlist[i].pt[0].x,cal.targetlist[i].pt[0].y,cal.targetlist[i].pt[1].x,cal.targetlist[i].pt[1].y);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.pt\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.height\" type=\"UINT\" val=\"%d\" />\r\n",i,cal.targetlist[i].height);	

	}


////
#endif
	// rule
	p += sprintf(p, "<item key=\"vca.rule.R0.n_height\" type=\"UINT\" val=\"%d\" />\r\n",value->n_height);
	p += sprintf(p, "<item key=\"vca.rule.R0.n_width\" type=\"UINT\" val=\"%d\" />\r\n",value->n_width);	
	p += sprintf(p, "<item key=\"vca.rule.R0.ncounters\" type=\"UINT\" val=\"%d\" />\r\n",value->ncntrs);
	p += sprintf(p, "<item key=\"vca.rule.R0.nzones\" type=\"UINT\" val=\"%d\" />\r\n",value->nzones);	
	p += sprintf(p, "<item key=\"vca.rule.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	

int idx = 0;
	// zone
	zonelist = value->zonelist;
	for(i=0;i<IVCA_MAX_ZONES;i++){
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",i,zonelist[i].active);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",i,(zonelist[i].color[0]) |(zonelist[i].color[1] << 8)|(zonelist[i].color[2] << 16));
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",i,zonelist[i].enabled);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_class\" type=\"UINT\" val=\"%d\" />\r\n",i,zonelist[i].eclass);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_colorsens\" type=\"UINT\" val=\"%d\" />\r\n",i,(zonelist[i].ecolor_sens <<24) | (zonelist[i].ecolor[0]) |(zonelist[i].ecolor[1] << 8)|(zonelist[i].ecolor[2] << 16));
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d",zonelist[i].size_min[0],zonelist[i].size_min[1],zonelist[i].size_max[0],zonelist[i].size_max[1]);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_size\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d",zonelist[i].speed_min,zonelist[i].speed_max);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_speed\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",i,zonelist[i].id);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",i,zonelist[i].name);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",i,zonelist[i].npts);
			ptemp = temp;
			for(j=0;j<zonelist[i].npts;j++)
				ptemp += sprintf(ptemp, "%d %d ",zonelist[i].pt[j].x,zonelist[i].pt[j].y);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d",zonelist[i].stop_time,zonelist[i].abandon_time,zonelist[i].remove_time,zonelist[i].loiter_time,zonelist[i].fall_time);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.time_sarlf\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",i,zonelist[i].type);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.sensitivity\" type=\"UINT\" val=\"%d\" />\r\n",i,zonelist[i].sensitivity);
//idx++;		
	}

	// counter
	cntrlist = value->cntrlist;
	for(i=0;i<IVCA_MAX_CNTRS;i++){
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",i,cntrlist[i].active);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",i,(cntrlist[i].color[0]) |(cntrlist[i].color[1] << 8)|(cntrlist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.e_value\" type=\"INT\" val=\"%d\" />\r\n",i,cntrlist[i].evalue);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",i,cntrlist[i].enabled);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.id\" type=\"INT\" val=\"%d\" />\r\n",i,cntrlist[i].id);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.name\" type=\"STRING\" val=\"%s\" />\r\n",i,cntrlist[i].name);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d %d %d %d",cntrlist[i].pt[0].x,cntrlist[i].pt[0].y,cntrlist[i].pt[1].x,cntrlist[i].pt[1].y,cntrlist[i].pt[2].x,cntrlist[i].pt[2].y,cntrlist[i].pt[3].x,cntrlist[i].pt[3].y);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.pt\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.reset_alert\" type=\"BOOL\" val=\"%u\" />\r\n",i,cntrlist[i].resetalert);
			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d",cntrlist[i].zid_up,cntrlist[i].zid_dn);	
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.zid\" type=\"STRING\" val=\"%s\" />\r\n",i,temp);
	}

	p += sprintf(p, "<item key=\"vca.opt.R0.en_prediction\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_prediction);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_roi\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_shadowrm\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_shadowrm);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_snapshot\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_snapshot);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_tamper\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_tamper);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_privacy\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_privacy);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d %d %d",opt.roi.x,opt.roi.y,opt.roi.w,opt.roi.h);
	p += sprintf(p, "<item key=\"vca.opt.R0.roi_xywh\" type=\"STRING\" val=\"%s\" />\r\n",temp);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_fg\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_fg);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_info\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_info);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_sh\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_dbg_sh);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_ar\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_ar);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_id\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_id);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tm\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_tm);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_cl\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_cl);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tr\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_tr);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_roi\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_rule);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_rule_name);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_s3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_s3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_w3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_w3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_h3d\" type=\"BOOL\" val=\"%u\" />\r\n",opt.sw_obj_h3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.track_ref\" type=\"UINT\" val=\"%d\" />\r\n",opt.track_ref);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_usecalib\" type=\"BOOL\" val=\"%u\" />\r\n",opt.en_usecalib);
	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d",opt.min_width3d,opt.min_height3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.min_objsize\" type=\"STRING\" val=\"%s\" />\r\n",temp);
	p += sprintf(p, "<item key=\"vca.opt.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	
	p += sprintf(p, "</vca>\r\n");
	p += sprintf(p, "\r\n");
	
	p += sprintf(p, "-----------------------------7db301281e86059e--\r\n");
	p += sprintf(p, "\r\n");

	#endif
///////////

#if PRINT_HTTP_API_SEND
	printf("[%s] CH(%d) VA_CONFIG\n%s\n", __FUNCTION__, cam_id, http_api);
#endif

	snprintf(sock_buf, 1024*64, str_api_raw_va, ip_str,
			strlen(http_api), auth_encbuf, http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_VA_CONFIG, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_VA_CONFIG, sock_buf);

	//printf("\n%s\n", sock_buf);

	return len;
#if 0
	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		//free(temp);
		//free(http_api);
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
			printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
			perror("setsockopt");
			close(sock);
			sock = (-1);
			//free(temp);
			//free(http_api);
			nf_ipcam_setup_send_done(cam_id, type);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		//free(temp);
		//free(http_api);
		nf_ipcam_setup_send_done(cam_id, type);
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
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	_set_last_api_str(cam_id, type, http_api);
	//free(temp);
	//free(http_api);
	printf("cam_set_va_config EXIT @@@@\n");
	nf_ipcam_setup_waiting(cam_id, type, sock);

	return IPCAM_SETUP_RTN_DONE;
#endif
}

static int _itx_call_api_upload(int cam_id, const char *path, const char *query, icm_http_upload *upload, int timeout)
{
	int rtn = ITX_RTN_FAIL;
	icm_http ctx = { 0, };
	icm_response res = { 0, };

	// init ctx , res
	memset(&ctx, 0x00, sizeof(ctx));
	memset(&res, 0x00, sizeof(res));
	icm_http_ch_init_tout(&ctx, cam_id, timeout);

	if(icm_http_upload_from_buffer(&ctx, path, query, upload, &res) != ICM_RTN_OK)
	{
		IPCAM_DBG(MAJOR, "Curl http api Fail(%s)\n", res.msg);
		goto ends_label;
	}

	rtn = ITX_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return rtn;

}

static int _curl_set_va_config_gzip(char *rule_str, size_t str_len, int cam_id)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	char *path = "/cgi-bin/vca.fcgi";
	char *form = "vca";
	char *file = "rule.xml.gz";
	icm_zlib_stream dest = { 0, };
	icm_gzip_header head = { 0, };
	icm_http_upload upload = { 0, };

	memset(&dest, 0x00, sizeof(dest));
	memset(&head, 0x00, sizeof(head));

	if(rule_str == NULL)
	{
		IPCAM_DBG(ERROR, "Input Error\n");
		goto ends_label;
	}

	head.file_type = ICM_GZIP_TEXT;
	head.make_time = time(NULL);
	head.os_code   = ICM_OS_UNIX;
	snprintf(head.name, sizeof(head.name), file);

	if(icm_zlib_deflate_to_gzip(rule_str, str_len, &dest, &head) != ICM_ZLIB_RTN_SUCC)
	{
		IPCAM_DBG(MAJOR, "icm_zlib_deflate_to_gzip fail(CH:%d)\n", cam_id);
		goto ends_label;
	}

	memset(&upload, 0x00, sizeof(upload));
	snprintf(upload.form_name, sizeof(upload.form_name), form);
	snprintf(upload.file_name, sizeof(upload.file_name), file);
	upload.file_buffer      = dest.stream;
	upload.file_buffer_size = dest.useLen;

	icm_http_add_header(&upload, "Accept-Encoding: gzip, deflate");

	if(_itx_call_api_upload(cam_id, path, (const char *)"", &upload, CURL_TIMEOUT) != ITX_RTN_OK)
	{
		IPCAM_DBG(MAJOR, "_itx_call_api_upload fail(CH:%d)\n", cam_id);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	icm_zlib_stream_free(&dest);

	return rtn;
}

static int _curl_set_va_config(ivca_rule_t* value, int cam_id)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	char *rule_str = NULL;
	rule_str = (char*) malloc (64*1024);
	memset(rule_str, 0x00, 64*1024);

	char *p = NULL;
	char *ptemp = NULL;

	int i = 0, j = 0;
	ivca_zone_t *zonelist = NULL;
	ivca_cntr_t*cntrlist = NULL;
	char temp[64] = { 0, };

	int enable = 0;
	VCAPropData data[NUM_ACTIVE_CH];

	ivca_option_t opt;
	ivca_calib_t cal;
	gchar key[64] = { 0, }, *str = NULL;

	mtable *runtime = get_runtime();

	if(value == NULL)
	{
		IPCAM_DBG(ERROR, "Input Error\n");
		goto ends_label;
	}

	memset(&data, 0x00, sizeof(data));
	DAL_get_vca_prop_data_all(&data, NUM_ACTIVE_CH);
	enable = data[cam_id].active;

	memset(&opt, 0x00, sizeof(opt));
	prvGetVCAOptData(&opt, cam_id);
	
	memset(&cal, 0x00, sizeof(cal));
	prvGetVCACalibData(&cal, cam_id);

	NF_IPCAM_SETUP_TYPE_E type = NF_IPCAM_TYPE_SET_VA_CONFIG;

	IPCAM_DBG(MAJOR, "cam_set_va_config ENTER enable %d nzones %d  ncntrs %d @@@@@@\n",
		             enable, value->nzones, value->ncntrs);


	memset(&key, 0x00, sizeof(key));
	sprintf(key, "cam.vca.cfg.R%u.sched", cam_id);
	str = nf_sysdb_get_str_nocopy(key);

	p = rule_str;

#if 1
	p += sprintf(p, "<vca>\r\n");

	//option
	p += sprintf(p, "<item key=\"vca.cfg.R0.act\" type=\"BOOL\" val=\"%d\" />\r\n",     enable);
	p += sprintf(p, "<item key=\"vca.cfg.R0.detect\" type=\"BOOL\" val=\"%d\" />\r\n",  data[cam_id].detect);
	p += sprintf(p, "<item key=\"vca.cfg.R0.unit\" type=\"UINT\" val=\"%d\" />\r\n",    data[cam_id].unit);
	p += sprintf(p, "<item key=\"vca.cfg.R0.sched\" type=\"STRING\" val=\"%s\" />\r\n", str);
	
	//cal data
	p += sprintf(p, "<item key=\"vca.calib.R0.focal\" type=\"STRING\" val=\"%f\" />\r\n",    cal.focal);
	p += sprintf(p, "<item key=\"vca.calib.R0.height\" type=\"STRING\" val=\"%f\" />\r\n",   cal.height);
	p += sprintf(p, "<item key=\"vca.calib.R0.tilt\" type=\"STRING\" val=\"%f\" />\r\n",     cal.tilt);
	p += sprintf(p, "<item key=\"vca.calib.R0.ntargets\" type=\"UINT\" val=\"%d\" />\r\n",   cal.ntargets);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_height\" type=\"UINT\" val=\"%d\" />\r\n",   cal.p_height);	
	p += sprintf(p, "<item key=\"vca.calib.R0.p_width\" type=\"UINT\" val=\"%d\" />\r\n",    cal.p_width);	
	p += sprintf(p, "<item key=\"vca.calib.R0.paramvalid\" type=\"BOOL\" val=\"%u\" />\r\n", cal.paramvalid);	

	for(i = 0; i < IVCA_MAX_CALIB_TARGETS; i++) 
	{
	
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d", cal.targetlist[i].pt[0].x, cal.targetlist[i].pt[0].y, 
									 cal.targetlist[i].pt[1].x, cal.targetlist[i].pt[1].y);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.pt\" type=\"STRING\" val=\"%s\" />\r\n",   i, temp);
		p += sprintf(p, "<item key=\"vca.calib.R0.T%u.height\" type=\"UINT\" val=\"%d\" />\r\n", i, cal.targetlist[i].height);
	}

	// rule
	p += sprintf(p, "<item key=\"vca.rule.R0.n_height\" type=\"UINT\" val=\"%d\" />\r\n",  value->n_height);
	p += sprintf(p, "<item key=\"vca.rule.R0.n_width\" type=\"UINT\" val=\"%d\" />\r\n",   value->n_width);	
	p += sprintf(p, "<item key=\"vca.rule.R0.ncounters\" type=\"UINT\" val=\"%d\" />\r\n", value->ncntrs);
	p += sprintf(p, "<item key=\"vca.rule.R0.nzones\" type=\"UINT\" val=\"%d\" />\r\n",    value->nzones);	
	p += sprintf(p, "<item key=\"vca.rule.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	
	// zone
	zonelist = value->zonelist;
	for(i = 0; i < IVCA_MAX_ZONES; i++)
	{
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",      i, zonelist[i].active);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n",     i, (zonelist[i].color[0]) | (zonelist[i].color[1] << 8) | (zonelist[i].color[2] << 16));
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n",     i, zonelist[i].enabled);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_class\" type=\"UINT\" val=\"%d\" />\r\n",     i, zonelist[i].eclass);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_colorsens\" type=\"UINT\" val=\"%d\" />\r\n", i, (zonelist[i].ecolor_sens << 24) | (zonelist[i].ecolor[0]) | (zonelist[i].ecolor[1] << 8) | (zonelist[i].ecolor[2] << 16));

		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d", zonelist[i].size_min[0], zonelist[i].size_min[1], 
									 zonelist[i].size_max[0], zonelist[i].size_max[1]);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_size\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);

		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d", zonelist[i].speed_min, zonelist[i].speed_max);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.f_speed\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.id\" type=\"INT\" val=\"%d\" />\r\n",         i, zonelist[i].id);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.name\" type=\"STRING\" val=\"%s\" />\r\n",    i, zonelist[i].name);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.npts\" type=\"UINT\" val=\"%d\" />\r\n",      i, zonelist[i].npts);

		ptemp = temp;
		for(j = 0; j < zonelist[i].npts; j++)
		{
			ptemp += sprintf(ptemp, "%d %d ", zonelist[i].pt[j].x, zonelist[i].pt[j].y);
		}
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.pt\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);

		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%d %d %d %d %d", zonelist[i].stop_time, 
				                        zonelist[i].abandon_time, 
										zonelist[i].remove_time, 
										zonelist[i].loiter_time, 
										zonelist[i].fall_time);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.time_sarlf\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);

		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.type\" type=\"UINT\" val=\"%d\" />\r\n",        i, zonelist[i].type);
		p += sprintf(p, "<item key=\"vca.rule.R0.Z%d.sensitivity\" type=\"UINT\" val=\"%d\" />\r\n", i, zonelist[i].sensitivity);
	}

	// counter
	cntrlist = value->cntrlist;
	for(i = 0; i < IVCA_MAX_CNTRS; i++)
	{
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.active\" type=\"BOOL\" val=\"%u\" />\r\n",  i, cntrlist[i].active);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.d_color\" type=\"UINT\" val=\"%d\" />\r\n", i, (cntrlist[i].color[0]) | (cntrlist[i].color[1] << 8) | (cntrlist[i].color[2] << 16));
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.e_value\" type=\"INT\" val=\"%d\" />\r\n",  i, cntrlist[i].evalue);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.enabled\" type=\"UINT\" val=\"%d\" />\r\n", i, cntrlist[i].enabled);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.id\" type=\"INT\" val=\"%d\" />\r\n",       i, cntrlist[i].id);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.name\" type=\"STRING\" val=\"%s\" />\r\n",  i, cntrlist[i].name);

			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d %d %d %d %d %d %d", cntrlist[i].pt[0].x, cntrlist[i].pt[0].y, 
					                                 cntrlist[i].pt[1].x, cntrlist[i].pt[1].y, 
													 cntrlist[i].pt[2].x, cntrlist[i].pt[2].y, 
													 cntrlist[i].pt[3].x, cntrlist[i].pt[3].y);
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.pt\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);

			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.reset_alert\" type=\"BOOL\" val=\"%u\" />\r\n", i, cntrlist[i].resetalert);

			memset(temp, 0x00, sizeof(temp));
			sprintf(temp, "%d %d", cntrlist[i].zid_up,
					               cntrlist[i].zid_dn);	
			p += sprintf(p, "<item key=\"vca.rule.R0.C%d.zid\" type=\"STRING\" val=\"%s\" />\r\n", i, temp);
	}

	p += sprintf(p, "<item key=\"vca.opt.R0.en_prediction\" type=\"BOOL\" val=\"%u\" />\r\n", opt.en_prediction);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_roi\" type=\"BOOL\" val=\"%u\" />\r\n",        opt.en_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_shadowrm\" type=\"BOOL\" val=\"%u\" />\r\n",   opt.en_shadowrm);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_snapshot\" type=\"BOOL\" val=\"%u\" />\r\n",   opt.en_snapshot);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_tamper\" type=\"BOOL\" val=\"%u\" />\r\n",     opt.en_tamper);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_privacy\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.en_privacy);

	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d %d %d", opt.roi.x, opt.roi.y, 
								 opt.roi.w, opt.roi.h);
	p += sprintf(p, "<item key=\"vca.opt.R0.roi_xywh\" type=\"STRING\" val=\"%s\" />\r\n", temp);

	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_fg\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_dbg_fg);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_info\" type=\"BOOL\" val=\"%u\" />\r\n",  opt.sw_dbg_info);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_dbg_sh\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_dbg_sh);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_ar\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_ar);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_bb\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_bb);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_id\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_id);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tm\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_tm);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_cl\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_cl);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_tr\" type=\"BOOL\" val=\"%u\" />\r\n",    opt.sw_obj_tr);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_roi\" type=\"BOOL\" val=\"%u\" />\r\n",       opt.sw_roi);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule\" type=\"BOOL\" val=\"%u\" />\r\n",      opt.sw_rule);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_rule_name\" type=\"BOOL\" val=\"%u\" />\r\n", opt.sw_rule_name);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_s3d\" type=\"BOOL\" val=\"%u\" />\r\n",   opt.sw_obj_s3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_w3d\" type=\"BOOL\" val=\"%u\" />\r\n",   opt.sw_obj_w3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.sw_obj_h3d\" type=\"BOOL\" val=\"%u\" />\r\n",   opt.sw_obj_h3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.track_ref\" type=\"UINT\" val=\"%d\" />\r\n",    opt.track_ref);
	p += sprintf(p, "<item key=\"vca.opt.R0.en_usecalib\" type=\"BOOL\" val=\"%u\" />\r\n",  opt.en_usecalib);

	memset(temp, 0x00, sizeof(temp));
	sprintf(temp, "%d %d", opt.min_width3d, opt.min_height3d);
	p += sprintf(p, "<item key=\"vca.opt.R0.min_objsize\" type=\"STRING\" val=\"%s\" />\r\n",temp);

	p += sprintf(p, "<item key=\"vca.opt.RCNT\" type=\"UINT\" val=\"1\" />\r\n");
	
	p += sprintf(p, "</vca>\r\n");
#endif

#if PRINT_HTTP_API_SEND
	printf("[%s] CH(%d) VA_CONFIG\n%s\n", __FUNCTION__, cam_id, rule_str);
#endif

	rtn = _curl_set_va_config_gzip(rule_str, strlen(rule_str), cam_id);

	nf_ipcam_setup_waiting(cam_id, NF_IPCAM_TYPE_SET_VA_CONFIG, -1);

ends_label:
	if(rule_str != NULL)
		free(rule_str);

	return rtn;
}

static guint _get_vca_prop_data_all(VCAPropData *data, guint ch_num)
{
	gchar key[64];
	guint ch;

	for (ch = 0; ch < ch_num; ch++) {
		sprintf(key, "cam.vca.cfg.R%u.act", ch);
		data[ch].active = nf_sysdb_get_bool(key);
		sprintf(key, "cam.vca.cfg.R%u.detect", ch);
		data[ch].detect = nf_sysdb_get_bool(key);
	}
	
	return 0;
}

static guint _get_vca_opt_data(ivca_option_t*option, guint channel)
{
	gchar key[64], *p, *str;
	gint m;

	p = key + sprintf(key, "cam.vca.opt.R%u.", channel);

	sprintf(p, "en_shadowrm");
	option->en_shadowrm = nf_sysdb_get_bool(key);
	sprintf(p, "en_prediction");
	option->en_prediction = nf_sysdb_get_bool(key);
	sprintf(p, "en_roi");
	option->en_roi = nf_sysdb_get_bool(key);
	sprintf(p, "en_tamper");
	option->en_tamper = nf_sysdb_get_bool(key);
	sprintf(p, "en_snapshot");
	option->en_snapshot = nf_sysdb_get_bool(key);
	sprintf(p, "en_privacy");
	option->en_privacy= nf_sysdb_get_bool(key);
	sprintf(p, "roi_xywh");
	str = nf_sysdb_get_str_nocopy(key);
	m = str ? sscanf(str, "%hd %hd %hd %hd", &option->roi.x, &option->roi.y,
			&option->roi.w, &option->roi.h) : 0;
	if ( m < 4 ) {
		option->roi.x = 120;
		option->roi.y = 90;
		option->roi.w = 3600;
		option->roi.h = 1980;
	}
	
	sprintf(p, "track_ref");
	option->track_ref = (guint8)nf_sysdb_get_uint(key);
	sprintf(p, "sw_obj_bb");
	option->sw_obj_bb = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_id");
	option->sw_obj_id = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_ar");
	option->sw_obj_ar = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tm");
	option->sw_obj_tm = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tr");
	option->sw_obj_tr = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule");
	option->sw_rule = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule_name");
	option->sw_rule_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_roi");
	option->sw_roi = nf_sysdb_get_bool(key);
	
	sprintf(p, "sw_dbg_fg");
	option->sw_dbg_fg = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_sh");
	option->sw_dbg_sh = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_info");
	option->sw_dbg_info = nf_sysdb_get_bool(key);
	
	return 0;
}

/**
 * @brief Sysdb??서 VCA Rule ??보?????어??다.
 * @param[out] rule Vca rule struct.
 * @param[in] channel 채널 번호.
 * @return ???? 0.
 */
static guint _get_vca_rule_data(ivca_rule_t*rule, guint channel)
{
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	ivca_zone_t*z = rule->zonelist;
	ivca_cntr_t*c = rule->cntrlist;

	p = key + sprintf(key, "cam.vca.rule.R%u.", channel);
	
	sprintf(p, "n_width");
	rule->n_width = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "n_height");
	rule->n_height = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "nzones");
	rule->nzones = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "ncounters");
	rule->ncntrs = (guint16)nf_sysdb_get_uint(key);

	for (n = 0; n < IVCA_MAX_ZONES; n++, z++) {
		p = key + sprintf(key, "cam.vca.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->name, str ? str : "", sizeof(z->name));
		sprintf(p, "id");
		z->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "type");
		z->type = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "active");
		z->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		z->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "time_sarlf");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu %hu", &z->stop_time,
				&z->abandon_time, &z->remove_time, &z->loiter_time,&z->fall_time) : 0;
		if ( m < 5 )
			z->stop_time = z->abandon_time =
					z->remove_time = z->loiter_time = z->fall_time= 5;
		sprintf(p, "f_colorsens");
		*(guint *)z->ecolor = (0x00ffffff & nf_sysdb_get_uint(key));
		z->ecolor_sens = (0xff000000 & nf_sysdb_get_uint(key)) >>24;
		sprintf(p, "f_size");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu", &z->size_min[0],
				&z->size_min[1], &z->size_max[0], &z->size_max[1]) : 0;
		if ( m < 4 )
			z->size_min[0] = z->size_min[0] =
					z->size_max[0] = z->size_max[0] = 0;
		sprintf(p, "f_speed");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu", &z->speed_min, &z->speed_max) :
				0;
		if ( m < 2 )
			z->speed_min = z->speed_max = 0;
		sprintf(p, "f_class");
		z->eclass = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		z->color[0] = (guint8)m;
		z->color[1] = (guint8)(m >> 8);
		z->color[2] = (guint8)(m >> 16);
		sprintf(p, "npts");
		z->npts = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "pt");
		i = 0;
		str = nf_sysdb_get_str(key);
		if ( str ) {
			for (last = str, i = 0; i < z->npts; i++) {
				tokx = strtok_r(last, " \t,.", &last);
				if ( !tokx )
					break;
				toky = strtok_r(last, " \t,.", &last);
				if ( !toky )
					break;
				z->pt[i].x = (gint16)atoi(tokx);
				z->pt[i].y = (gint16)atoi(toky);
			}
			if ( i < z->npts ) {
				i = 0;
				z->npts = 0;
			}
			free(str);
		}
		for ( ; i < IVCA_MAX_PTSPERZONE; i++)
			z->pt[i].x = z->pt[i].y = 0;
		
		sprintf(p, "sensitivity");
		z->sensitivity = nf_sysdb_get_uint(key);
	}

	for (n = 0; n < IVCA_MAX_CNTRS; n++, c++) {
		p = key + sprintf(key, "cam.vca.rule.R%u.C%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(c->name, str ? str : "", sizeof(c->name));
		sprintf(p, "id");
		c->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "active");
		c->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		c->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "zid");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hd %hd", &c->zid_up, &c->zid_dn) : 0;
		if ( m < 2 )
			c->zid_up = c->zid_dn = -1;
		sprintf(p, "e_value");
		c->evalue = nf_sysdb_get_int(key);
		sprintf(p, "reset_alert");
		c->resetalert = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		c->color[0] = (guint8)m;
		c->color[1] = (guint8)(m >> 8);
		c->color[2] = (guint8)(m >> 16);
		sprintf(p, "pt");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hd %hd %hd %hd %hd %hd %hd %hd",
				&c->pt[0].x, &c->pt[0].y, &c->pt[1].x, &c->pt[1].y,
				&c->pt[2].x, &c->pt[2].y, &c->pt[3].x, &c->pt[3].y) : 0;
		if ( m < 8 )
			memset(c->pt, 0, sizeof(c->pt));
	}

	return 0;
}

/**
 * @brief Plain??결???카메??에 ??문????송??다.
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param buf ??송????문.
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
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

/**
 * @brief SSL??결???카메??에 ??문????송??다.
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param buf ??송????문.
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
static int _cam_setup_send_ssl(int cam_id, int type, char* buf)
{
	int sock;
	struct sockaddr_in sin;
	int len = 0;

	char ip_str[16];
	unsigned short http_port;

	mtable *runtime = get_runtime();

	SSL *ssl = runtime[cam_id].sys.ssl[type];
	SSL_CTX *ctx = runtime[cam_id].sys.ctx[type];


	IPCAM_DBG(MAJOR, "start CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));


	nf_ipcam_setup_sending(cam_id, type);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		IPCAM_DBG(WARN, "socket alloc fail CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, type);
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
			IPCAM_DBG(WARN, "socket rcv timeout fail. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
			perror("setsockopt");
			close(sock);
			nf_ipcam_setup_send_done(cam_id, type);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		IPCAM_DBG(WARN, "socket connect fail. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		perror("connect");
		close(sock);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (ctx == NULL)
	{
		IPCAM_DBG(MINOR, "Created SSL Context CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		runtime[cam_id].sys.ctx[type] = SSL_CTX_new(SSLv23_client_method());
		if (runtime[cam_id].sys.ctx[type] == NULL)
		{
			IPCAM_DBG(ERROR, "SSL_CTX creation failed. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
			close(sock);
			nf_ipcam_setup_send_done(cam_id, type);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(runtime[cam_id].sys.ctx[type], SSL_OP_ALL);
		runtime[cam_id].sys.ssl[type] = SSL_new(runtime[cam_id].sys.ctx[type]);
		if (runtime[cam_id].sys.ssl[type] == NULL)
		{
			IPCAM_DBG(ERROR, "SSL creation failed. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
			close(sock);
			_release_resource(&sock, NULL, NULL, &runtime[cam_id].sys.ctx[type]);
			nf_ipcam_setup_send_done(cam_id, type);
			return IPCAM_SETUP_RTN_FAILED;
		}
		pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_INIT;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
	}

	pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_INIT && 
		runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_SHUTDOWN)
	{
		IPCAM_DBG(ERROR, "SSL is NULL now CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_set_fd(runtime[cam_id].sys.ssl[type], sock);
	runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_CONNECTING;
	len = SSL_connect(runtime[cam_id].sys.ssl[type]);
	pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);

	if (len < 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	IPCAM_DBG(MINOR, "SSL open OK - CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));

	pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_CONNECTING)
	{
		IPCAM_DBG(ERROR, "SSL is not CONNECTING state CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_write(runtime[cam_id].sys.ssl[type], buf, strlen(buf));
	runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_WAITING;
	pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (len < 0)
	{
		IPCAM_DBG(WARN, "SSL_write fail. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) type(%s) - %s\n", cam_id, ipcam_get_type_str(type), buf);
#endif

	nf_ipcam_setup_send_done(cam_id, type);
	nf_ipcam_setup_waiting(cam_id, type, sock);

	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief SSL??결??서 ??증 ??패??digest??증??로 ??시??한??
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param rbuf Basic??증????신????문.(nonce??parsing??도)
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
static int _ssl_digest_setup_again(int cam_id, int type, char* rbuf)
{
	char *http_api = NULL;
	char auth_encbuf[256];
	char sock_buf[24*1024];
	char ip_str[16];

	char username[64];
	char password[64];
	char realm[128];
	char nonce[128];
	char uri[128];
	char *method = "POST";
	char *s, *e;
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";

	struct sockaddr_in sin;

	char auth_str[1024];

	unsigned short http_port;

	int sock;
	int len = 0;

	mtable *runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));

	nf_ipcam_setup_sending(cam_id, type);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	if (type < NF_IPCAM_TYPE_SET_ENABLE_VA || type > NF_IPCAM_TYPE_SET_VA_OPTION)
	{
		//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/action.fcgi");
	}
	else
	{
		//snprintf(uri, 128, "http://%s:%d/cgi-bin/vca.fcgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/vca.fcgi");
	}
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	http_api = _last_http_api[cam_id][type];
	if (type < NF_IPCAM_TYPE_SET_ENABLE_VA || type > NF_IPCAM_TYPE_SET_VA_OPTION)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
				ip_str, strlen(http_api), auth_str, http_api);
	}
	else
	{
		snprintf(sock_buf, 24*1024, str_api_raw_va_digest,
				ip_str, strlen(http_api), auth_str, http_api);
	}

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		nf_ipcam_setup_send_done(cam_id, type);
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
			IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			nf_ipcam_setup_send_done(cam_id, type);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_INIT && 
		runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_SHUTDOWN)
	{
		IPCAM_DBG(ERROR, "SSL is NULL now CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_set_fd(runtime[cam_id].sys.ssl[type], sock);
	runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_CONNECTING;
	len = SSL_connect(runtime[cam_id].sys.ssl[type]);
	pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);

	IPCAM_DBG(MINOR, "SSL open OK - CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));

	if (len < 0)
	{
		IPCAM_DBG(WARN, "SSL_connect fail. CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (runtime[cam_id].sys.ssl_state[type] != IPCAM_SSL_CONNECTING)
	{
		IPCAM_DBG(ERROR, "SSL is not CONNECTING state CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}
	len = SSL_write(runtime[cam_id].sys.ssl[type], sock_buf, strlen(sock_buf));
	runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_WAITING;
	pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
	if (len <= 0)
	{
		IPCAM_DBG(WARN, "SSL write failed CH(%d) type(%s)\n", cam_id, ipcam_get_type_str(type));
		pthread_mutex_lock(&runtime[cam_id].sys.ssl_mtx[type]);
		_release_resource(&sock, NULL, &runtime[cam_id].sys.ssl[type], &runtime[cam_id].sys.ctx[type]);
		runtime[cam_id].sys.ssl_state[type] = IPCAM_SSL_NOT_AVAILABLE;
		pthread_mutex_unlock(&runtime[cam_id].sys.ssl_mtx[type]);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}
#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "CH(%d) type(%s) - %s\n", cam_id, ipcam_get_type_str(type), sock_buf);
#endif

 	nf_ipcam_setup_send_done(cam_id, type); 
	nf_ipcam_setup_waiting(cam_id, type, sock);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief Plain??결??서 ??증 ??패??digest??증??로 ??시??한??
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param rbuf Basic??증????신????문.(nonce??parsing??도)
 * @return IPCAM_SETUP_RTN_DONE - ??공, IPCAM_SETUP_RTN_FAILED - ??패.
 */
static int _cam_digest_setup_again(int cam_id, int type, char* rbuf)
{
	char *http_api = NULL;
	char auth_encbuf[256];
	char sock_buf[24*SOCK_BUF_LENGTH];
	char ip_str[16];

	char username[64];
	char password[64];
	char realm[128];
	char nonce[128];
	char uri[128];
	char *method = "POST";
	char *s, *e;
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";

	struct sockaddr_in sin;

	char auth_str[1024];

	unsigned short http_port;

	int sock;

	mtable *runtime = get_runtime();


	nf_ipcam_setup_sending(cam_id, type);
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	
	if (type < NF_IPCAM_TYPE_SET_ENABLE_VA || type > NF_IPCAM_TYPE_SET_VA_OPTION)
	{
		//snprintf(uri, 128, "http://%s:%d/cgi-bin/action.fcgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/action.fcgi");
	}
	else
	{
		//snprintf(uri, 128, "http://%s:%d/cgi-bin/vca.fcgi", ip_str, http_port);
		snprintf(uri, 128, "/cgi-bin/vca.fcgi");
	}
	s = strstr(rbuf, f_str_realm);
	if (s == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	memset(realm, 0x00, 128);
	memcpy(realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL) { nf_ipcam_setup_send_done(cam_id, type); return IPCAM_SETUP_RTN_FAILED; }
	memset(nonce, 0x00, 128);
	memcpy(nonce, s, e-s);

	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, rbuf, auth_str);

	http_api = _last_http_api[cam_id][type];
	if (type < NF_IPCAM_TYPE_SET_ENABLE_VA || type > NF_IPCAM_TYPE_SET_VA_OPTION)
	{
		snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
				ip_str, strlen(http_api), auth_str, http_api);
	}
	else
	{
		snprintf(sock_buf, 24*SOCK_BUF_LENGTH, str_api_raw_va_digest,
				ip_str, strlen(http_api), auth_str, http_api);
	}

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
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
			//IPCAM_DBG(ERROR, "CH(%d)\n", cam_id);
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			nf_ipcam_setup_send_done(cam_id, type);
			return (-1);
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		//printf("[%s] ERROR | CH(%d)\n", __FUNCTION__, cam_id);
		perror("connect");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "%s\n", sock_buf);
#endif

	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0)
	{
		perror("send");
		close(sock);
		sock = (-1);
		nf_ipcam_setup_send_done(cam_id, type);
		return IPCAM_SETUP_RTN_FAILED;
	}

	nf_ipcam_setup_send_done(cam_id, type);
	nf_ipcam_setup_waiting(cam_id, type, sock);

	return IPCAM_SETUP_RTN_DONE;
}


#if 0
extern int cam_set_va_confg_init(int cam_id)
{
	ivca_rule_t rule;
	ivca_option_t opt;
	int enable;
	VCAPropData data[NUM_ACTIVE_CH];

	printf("cam_set_va_confg_init CHANNEL = %d, ENTER @@@@@\n", cam_id);

	//_get_vca_opt_data(&opt,cam_id);
	cam_set_va_option(cam_id);
	
	//_get_vca_rule_data(&rule,cam_id);
	cam_set_va_config(cam_id);

	//_get_vca_prop_data_all(&data,NUM_ACTIVE_CH);
	//enable = data[cam_id].active;
	cam_set_enable_va(cam_id);
}
#endif

/**
 * @brief 해상도 enum값을 해상도 문자로  변환
 * @param resol_code 해상도 enum값 (uint64_t로 type 변경)
 * @return 해상도 character 문자
 */
static char* get_resol_string(uint64_t resol_code)
{
	if(resol_code == NF_IPCAM_RES_1920x1080)
	return resol_string[0];
	if(resol_code == NF_IPCAM_RES_1920x1080I)
		return resol_string[1];
	if(resol_code == NF_IPCAM_RES_1280x1024)
		return resol_string[2];
	if(resol_code == NF_IPCAM_RES_1024x768)
		return resol_string[3];
	if(resol_code == NF_IPCAM_RES_1280x720)
		return resol_string[4];
	if(resol_code == NF_IPCAM_RES_1280x720I)
		return resol_string[5];
	if(resol_code == NF_IPCAM_RES_720x576)
		return resol_string[6];
	if(resol_code == NF_IPCAM_RES_720x480)
		return resol_string[7];
	if(resol_code == NF_IPCAM_RES_704x576)
		return resol_string[8];
	if(resol_code == NF_IPCAM_RES_704x480)
		return resol_string[9];
	if(resol_code == NF_IPCAM_RES_640x480)
		return resol_string[10];
	if(resol_code == NF_IPCAM_RES_640x352)
		return resol_string[11];
	if(resol_code == NF_IPCAM_RES_352x288)
		return resol_string[12];
	if(resol_code == NF_IPCAM_RES_352x240)
		return resol_string[13];
	if(resol_code == NF_IPCAM_RES_320x240)
		return resol_string[14];
	if(resol_code == NF_IPCAM_RES_640x360)
		return resol_string[15];
	if(resol_code == NF_IPCAM_RES_320x180)
		return resol_string[16];
	if(resol_code == NF_IPCAM_RES_640x360I)
		return resol_string[17];
	if(resol_code == NF_IPCAM_RES_640x400)
		return resol_string[18];
	if(resol_code == NF_IPCAM_RES_800x450)
		return resol_string[19];
	if(resol_code == NF_IPCAM_RES_1440x900)
		return resol_string[20];
	if(resol_code == NF_IPCAM_RES_800x600)
		return resol_string[21];
	if(resol_code == NF_IPCAM_RES_1600x1200)
		return resol_string[22];
	if(resol_code == NF_IPCAM_RES_2304x1296)
		return resol_string[23];
	if(resol_code == NF_IPCAM_RES_2048x1536)
		return resol_string[24];
	if(resol_code == NF_IPCAM_RES_2560x1440)
		return resol_string[25];
	if(resol_code == NF_IPCAM_RES_2688x1520)
		return resol_string[26];
	if(resol_code == NF_IPCAM_RES_2560x1600)
		return resol_string[27];
	if(resol_code == NF_IPCAM_RES_2560x1920)
		return resol_string[28];
	if(resol_code == NF_IPCAM_RES_2592x1920)
		return resol_string[29];
	if(resol_code == NF_IPCAM_RES_2592x1944)
		return resol_string[30];
	if(resol_code == NF_IPCAM_RES_2992x1680)
		return resol_string[31];
	if(resol_code == NF_IPCAM_RES_2880x1800)
		return resol_string[32];
	if(resol_code == NF_IPCAM_RES_3200x1800)
		return resol_string[33];
	if(resol_code == NF_IPCAM_RES_2880x2160)
		return resol_string[34];
	if(resol_code == NF_IPCAM_RES_3072x2048)
		return resol_string[35];
	if(resol_code == NF_IPCAM_RES_3200x2400)
		return resol_string[36];
	if(resol_code == NF_IPCAM_RES_3840x2160)
		return resol_string[37];
	if(resol_code == NF_IPCAM_RES_2592x1520)
		return resol_string[38];
	if(resol_code == NF_IPCAM_RES_3000x3000)
		return resol_string[39];
	if(resol_code == NF_IPCAM_RES_2048x2048)
		return resol_string[40];
	if(resol_code == NF_IPCAM_RES_1280x1280)
		return resol_string[41];
	if(resol_code == NF_IPCAM_RES_640x640)
		return resol_string[42];
	if(resol_code == NF_IPCAM_RES_320x320)
		return resol_string[43];
}

/**
 * @brief Fps enum값을 ??해 ??당??는 fps number string??구한??
 * @param fps_code Fps enum???
 * @return Fps 문자??ex NF_IPCAM_FPS_300 -> "30")
 */
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
		case 75:			   { rtn_id = 12; break; }
		case 63:			   { rtn_id = 13; break; }
		default:               { rtn_id =  0; break; }
	}

	return fps_string[rtn_id];
}

/**
 * @brief Fps enum값을 ??해 ??반????당??는 gop number string??구한??
 * @param fps_code Fps enum???
 * @return Gop 문자??ex NF_IPCAM_FPS_300 -> "15")
 */
static char* get_gop_string(unsigned int fps_code)
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
		case 75:			   { rtn_id = 12; break; }
		case 63:			   { rtn_id = 13; break; }
		default:               { rtn_id =  0; break; }
	}

	return gop_string[rtn_id];
}

static char* get_bitctrl_string(unsigned int bitctrl_code)
{
	switch(bitctrl_code)
	{
		case NF_IPCAM_BITRATE_CONTROL_CBR :  return bitctrl_string[0];
		case NF_IPCAM_BITRATE_CONTROL_VBR :  return bitctrl_string[1];
		case NF_IPCAM_BITRATE_CONTROL_MBR :  return bitctrl_string[2];
		case NF_IPCAM_BITRATE_CONTROL_VBR_PLUS :  return bitctrl_string[3];
		default : break;
	}

	return bitctrl_string[0];
}

static char* get_vcodec_string(unsigned int vcodec_code)
{
	switch(vcodec_code)
	{
		case NF_IPCAM_VCODEC_H264 :  return vcodec_string[0];
		case NF_IPCAM_VCODEC_H265 :  return vcodec_string[1];
		default : break;
	}

	return vcodec_string[0];
}

/**
 * @brief Bitmask값으로????1??bit??index???구한??(1<<31 --> 31)
 * @param value Bitmask???
 * @return 1??bit??index.
 */
static int get_index_from_bitmask(unsigned int value)
{
	int i = 0;

	while ((1<<i != value) && i < 32) i++;

	if (i == 32) return (0);

	return i;
}

/**
 * @brief ???Job??형별로 ??신????문??????한??(??사????도)
 * @param cam_id 채널 번호.
 * @param type Job??형.
 * @param str ??신????문.
 */
static void _set_last_api_str(int cam_id, int type, char* str)
{
	if (_last_http_api[cam_id][type] == NULL)
	{
		switch(type)
		{
			case NF_IPCAM_TYPE_SET_VA_CONFIG:
				_last_http_api[cam_id][type] = (char*) malloc(64*1024);
				break;
			case NF_IPCAM_TYPE_SET_VA_OPTION:
			case NF_IPCAM_TYPE_SET_ENABLE_VA:
			case NF_IPCAM_TYPE_SET_RESET_VA:
			case NF_IPCAM_TYPE_INIT:
			case NF_IPCAM_TYPE_CUSTOM0:
			case NF_IPCAM_TYPE_CUSTOM1:
			case NF_IPCAM_TYPE_CUSTOM2:
			case NF_IPCAM_TYPE_CUSTOM3:
			case NF_IPCAM_TYPE_REBOOT_SOFT:
			case NF_IPCAM_TYPE_FACTORY_DEFAULT:
			case NF_IPCAM_TYPE_SET_VCODEC:
			case NF_IPCAM_TYPE_SET_ACODEC:
			case NF_IPCAM_TYPE_SET_IMAGE:
			case NF_IPCAM_TYPE_SET_ALARM:
			case NF_IPCAM_TYPE_SET_MOTION:
			case NF_IPCAM_TYPE_SET_PMASK:
			case NF_IPCAM_TYPE_GET_LENSCAP:
			case NF_IPCAM_TYPE_SET_ORIGIN:
			case NF_IPCAM_TYPE_SET_AF_MODE:
			case NF_IPCAM_TYPE_SET_AR_MODE:
			case NF_IPCAM_TYPE_SET_PAN_TILT:
			case NF_IPCAM_TYPE_SET_ZOOM:
			case NF_IPCAM_TYPE_SET_FOCUS:
			case NF_IPCAM_TYPE_SET_IRIS:
			case NF_IPCAM_TYPE_SET_ONESHOT:
			case NF_IPCAM_TYPE_GET_PAN:
			case NF_IPCAM_TYPE_GET_TILT:
			case NF_IPCAM_TYPE_GET_ZOOM:
			case NF_IPCAM_TYPE_GET_FOCUS:
			case NF_IPCAM_TYPE_GET_IRIS:
			case NF_IPCAM_TYPE_SET_STOP:
			case NF_IPCAM_TYPE_PRESET_SET:
			case NF_IPCAM_TYPE_PRESET_CLEAR:
			case NF_IPCAM_TYPE_PRESET_GO:
			case NF_IPCAM_TYPE_POLL_EVENT:
			case NF_IPCAM_TYPE_SET_IMAGE_ONVIF:
			case NF_IPCAM_TYPE_SET_EXP_ONVIF:
			case NF_IPCAM_TYPE_SET_FOCUS_ONVIF:
			case NF_IPCAM_TYPE_SET_ADJUST_D2N:
			case NF_IPCAM_TYPE_SET_ADJUST_N2D:
			case NF_IPCAM_TYPE_SET_FOCUS_COMP:
			case NF_IPCAM_TYPE_SET_DC_IRIS_CAL:
			default:
				_last_http_api[cam_id][type] = (char*) malloc(SOCK_BUF_LENGTH);
				break;
		}
	}

	strcpy(_last_http_api[cam_id][type], str);
}













static char *_ipcam_api_template =
//"GET /cgi-bin/action.fcgi?api=get_setup.system.fwup.status HTTP/1.1\r\n"
"%s %s?api=%s HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
//"Authorization: Basic %s\r\n"
"%s"	// Authorization string. ***** Must add '\r\n' *****
"User-Agent: IPX-NVR\r\n"
"Accept: */*\r\n"
"Accept-Encoding: gzip,deflate,sdch\r\n"
"Accept-Language: ko,en-US,en;q=0.8\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";


static int _local_get_sock(char* ip_str, unsigned short http_port, int timeout)
{
	int sock = 0;
	struct sockaddr_in sin;


	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip_str);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket");
		return 0;
	}
	{
		int ret;
		struct timeval tv;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0)
		{
			perror("setsockopt");
			_release_resource(&sock, NULL, NULL, NULL);
			return 0;
		}
	}
	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		perror("connect");
		_release_resource(&sock, NULL, NULL, NULL);
		return 0;
	}

	return sock;
}

static int _local_parse_digest_info(const char* srcbuf, char* rtn_realm, char* rtn_nonce)
{
	char *s,*e, *rbuf;
	const char *f_str_realm = "realm=\"";
	const char *f_str_nonce = "nonce=\"";

	rbuf = srcbuf;

	s = strstr(rbuf, f_str_realm);
	if (s == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_str_realm);
	e = strstr(s, "\"");
	if (e == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(rtn_realm, 0x00, 128);
	memcpy(rtn_realm, s, e-s);

	s = strstr(rbuf, f_str_nonce);
	if (s == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_str_nonce);
	e = strstr(s, "\"");
	if (e == NULL)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	memset(rtn_nonce, 0x00, 128);
	memcpy(rtn_nonce, s, e-s);

	return IPCAM_SETUP_RTN_DONE;
}



extern int itx_cam_cancel_upgrade(ipcam_upgrade_ctx *ctx)
{
	ctx->cancel_required = 1;
}



static int _ssl_check_fwup_status(int cam_id, ipcam_upgrade_ctx *fwup_ctx)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/action.fcgi";
	char *method = "GET";

	const char *rapi = "get_setup.system.fwup.status";

	const char *f_status_str = "status=";
	const char *f_timeout_str = "reboot_sec=";
	const char *f_endl_str = "\r\n";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, "");


	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
		if (len <= 0) {
			IPCAM_DBG(ERROR, "SSL_read error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL && strstr(sock_buf,"401 Unauthorized") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		SSL_set_fd(ssl, sock);

		if (SSL_connect(ssl) <= 0)
		{
			IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, &ssl, &ctx);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		// exception: not supported
		if (strstr(sock_buf, "WEBSVR_ERR_RET_PARAMETER") != NULL)
		{
			IPCAM_DBG(WARN, "CH(%d) doesn't support ITX upgrade protocol\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") != NULL)
		{
			char *s,*e;
			char status_str[8];
			char timeout_str[8];
			char status_desc_str[64];
			int _status;
			int _timeout;

			/* status parsing */
			if ((s=strstr(sock_buf, f_status_str)) == NULL)
			{
				IPCAM_DBG(ERROR, "CH(%d) upgrade status query failed\n", cam_id);
				fwup_ctx->status = IPCAM_FWUP_STATUS_ERR_NETWORK;
				return IPCAM_SETUP_RTN_FAILED;
			}
			s += strlen(f_status_str);
			e = strstr(s, f_endl_str);
			memset(status_str, 0x00, sizeof(status_str));
			strncpy(status_str, s, (e-s));
			_status = atoi(status_str);
#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "CH(%d) status get str[%s] int[%d]\n", cam_id, status_str, _status);
#endif

			/* timeout parsing if exist */
			if ((s=strstr(sock_buf, f_timeout_str)) != NULL)
			{
				s += strlen(f_timeout_str);
				e = strstr(s, f_endl_str);
				memset(timeout_str, 0x00, sizeof(timeout_str));
				strncpy(timeout_str, s, (e-s));
				_timeout = atoi(timeout_str);

				fwup_ctx->time_waiting = 0;
				fwup_ctx->timeout = _timeout;
#if PRINT_HTTP_API_SEND
				IPCAM_DBG(MINOR, "CH(%d) timeout get str[%s] int[%d]\n", cam_id, timeout_str, _timeout);
#endif
			}

			if (_status > IPCAM_FWUP_STATUS_MIN && _status < IPCAM_FWUP_STATUS_MAX)
			{
#if PRINT_HTTP_API_SEND
				IPCAM_DBG(MINOR, "CH(%d) upgrade status now(%s)\n", cam_id, IPCAM_UPGRADE_STATUS_STR[_status]);
#endif
				fwup_ctx->status = _status;
			}
			else if (_status > IPCAM_FWUP_STATUS_ERR_MIN && _status < IPCAM_FWUP_STATUS_ERR_MAX)
			{
				IPCAM_DBG(ERROR, "CH(%d) upgrade status now(%s)\n", cam_id, IPCAM_UPGRADE_ERR_STR[_status-IPCAM_FWUP_STATUS_ERR_MIN]);
				fwup_ctx->status = _status;
			}

			return IPCAM_SETUP_RTN_DONE;
		}
	}

	return IPCAM_SETUP_RTN_FAILED;
}

static int _plain_check_fwup_status(int cam_id, ipcam_upgrade_ctx *fwup_ctx)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/action.fcgi";
	char *method = "GET";

	const char *rapi = "get_setup.system.fwup.status";

	const char *f_status_str = "status=";
	const char *f_timeout_str = "reboot_sec=";
	const char *f_endl_str = "\r\n";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s\r\n", auth_encbuf);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

	nf_ipcam_get_username(cam_id, username);	// prepare for the digest auth token creation


	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
		if (len <= 0) {
			IPCAM_DBG(ERROR, "recv error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL && strstr(sock_buf,"401 Unauthorized") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);

	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
	//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
	unsigned int noncecount = 0;
	digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				perror("recv");
				_release_resource(&sock, NULL, NULL, NULL);
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, NULL, NULL);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		// exception: not supported
		if (strstr(sock_buf, "WEBSVR_ERR_RET_PARAMETER") != NULL)
		{
			IPCAM_DBG(WARN, "CH(%d) doesn't support ITX upgrade protocol\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") != NULL)
		{
			char *s,*e;
			char status_str[8];
			char timeout_str[8];
			char status_desc_str[64];
			int _status;
			int _timeout;

			/* status parsing */
			if ((s=strstr(sock_buf, f_status_str)) == NULL)
			{
				IPCAM_DBG(ERROR, "CH(%d) upgrade status query failed\n", cam_id);
				fwup_ctx->status = IPCAM_FWUP_STATUS_ERR_NETWORK;
				return IPCAM_SETUP_RTN_FAILED;
			}
			s += strlen(f_status_str);
			e = strstr(s, f_endl_str);
			memset(status_str, 0x00, sizeof(status_str));
			strncpy(status_str, s, (e-s));
			_status = atoi(status_str);
#if PRINT_HTTP_API_SEND
			IPCAM_DBG(MINOR, "CH(%d) status get str[%s] int[%d]\n", cam_id, status_str, _status);
#endif

			/* timeout parsing if exist */
			if ((s=strstr(sock_buf, f_timeout_str)) != NULL)
			{
				s += strlen(f_timeout_str);
				e = strstr(s, f_endl_str);
				memset(timeout_str, 0x00, sizeof(timeout_str));
				strncpy(timeout_str, s, (e-s));
				_timeout = atoi(timeout_str);

				fwup_ctx->time_waiting = 0;
				fwup_ctx->timeout = _timeout;
#if PRINT_HTTP_API_SEND
				IPCAM_DBG(MINOR, "CH(%d) timeout get str[%s] int[%d]\n", cam_id, timeout_str, _timeout);
#endif
			}

			if (_status > IPCAM_FWUP_STATUS_MIN && _status < IPCAM_FWUP_STATUS_MAX)
			{
#if PRINT_HTTP_API_SEND
				IPCAM_DBG(MINOR, "CH(%d) upgrade status now(%s)\n", cam_id, IPCAM_UPGRADE_STATUS_STR[_status]);
#endif
				fwup_ctx->status = _status;
			}
			else if (_status > IPCAM_FWUP_STATUS_ERR_MIN && _status < IPCAM_FWUP_STATUS_ERR_MAX)
			{
				IPCAM_DBG(ERROR, "CH(%d) upgrade status now(%s)\n", cam_id, IPCAM_UPGRADE_ERR_STR[_status-IPCAM_FWUP_STATUS_ERR_MIN]);
				fwup_ctx->status = _status;
			}

			return IPCAM_SETUP_RTN_DONE;
		}
	}

	return IPCAM_SETUP_RTN_FAILED;
}

extern int itx_cam_check_fwup_status(int cam_id, ipcam_upgrade_ctx *ctx)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	int use_ssl = nf_ipcam_is_using_ssl(cam_id);

	if (use_ssl)
	{
		rtn = _ssl_check_fwup_status(cam_id, ctx);
	}
	else
	{
		rtn = _plain_check_fwup_status(cam_id, ctx);
	}

	return rtn;
}




static int _ssl_request_fwup_mode(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/upload.cgi";
	char *method = "GET";

	const char *rapi = "firmmode";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, "");


	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
		if (len <= 0) {
			IPCAM_DBG(ERROR, "SSL_read error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		SSL_set_fd(ssl, sock);

		if (SSL_connect(ssl) <= 0)
		{
			IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, &ssl, &ctx);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
		{
			IPCAM_DBG(ERROR, "CH(%d) cam report error,\n%s\n", cam_id, sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		return IPCAM_SETUP_RTN_DONE;
	}

	return IPCAM_SETUP_RTN_FAILED;
}
static int _plain_request_fwup_mode(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/upload.cgi";
	char *method = "GET";

	const char *rapi = "firmmode";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s\r\n", auth_encbuf);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, "");

	nf_ipcam_get_username(cam_id, username);

	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
		if (len <= 0) {
			IPCAM_DBG(ERROR, "recv error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = recv(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = recv(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, NULL, NULL);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
		{
			IPCAM_DBG(ERROR, "CH(%d) cam report error,\n%s\n", cam_id, sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		return IPCAM_SETUP_RTN_DONE;
	}

	return IPCAM_SETUP_RTN_FAILED;
}

extern int itx_cam_request_fwup_mode(int cam_id)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	int use_ssl = nf_ipcam_is_using_ssl(cam_id);

	if (use_ssl)
	{
		rtn = _ssl_request_fwup_mode(cam_id);
	}
	else
	{
		rtn = _plain_request_fwup_mode(cam_id);
	}

	return rtn;
}










static int _ssl_prepare_upload(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	const char *uri = "/cgi-bin/action.fcgi";
	const char *method = "GET";

	const char *rapi = "set_setup.system.fwup";

	const char *f_request_str = "request=";
	const char *f_endl_str = "\r\n";
	char *s,*e;
	char request_str[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, "");


	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
		if (len <= 0) {
			IPCAM_DBG(ERROR, "SSL_read error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		SSL_set_fd(ssl, sock);

		if (SSL_connect(ssl) <= 0)
		{
			IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, &ssl, &ctx);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		IPCAM_DBG(ERROR, "CH(%d) HTTP error,\n%s\n", cam_id, sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}
	if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
	{
		IPCAM_DBG(ERROR, "CH(%d) cam report error,\n%s\n", cam_id, sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	/* request result parsing */
	if ((s = strstr(sock_buf, f_request_str)) == NULL)
	{
		IPCAM_DBG(ERROR, "CH(%d) msg parse error 1\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_request_str);
	e = strstr(s, f_endl_str);
	memset(request_str, 0x00, sizeof(request_str));
	strncpy(request_str, s, (e-s));

	if (strcmp(request_str, "accepted") == 0)
	{
		IPCAM_DBG(MINOR, "CH(%d) request %s\n", cam_id, request_str);
		return IPCAM_SETUP_RTN_DONE;
	}
	else if (strcmp(request_str, "denied") == 0)
	{
		IPCAM_DBG(ERROR, "CH(%d) request %s\n", cam_id, request_str);
		return IPCAM_SETUP_RTN_FAILED;
	}
	else
	{
		IPCAM_DBG(ERROR, "CH(%d) msg parse error 2\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	return IPCAM_SETUP_RTN_FAILED;
}

static int _plain_prepare_upload(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	const char *uri = "/cgi-bin/action.fcgi";
	const char *method = "GET";

	const char *rapi = "set_setup.system.fwup";

	const char *f_request_str = "request=";
	const char *f_endl_str = "\r\n";
	char *s,*e;
	char request_str[16];

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s\r\n", auth_encbuf);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

	nf_ipcam_get_username(cam_id, username);

	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		g_message("[%s] CH(%d) socket fail \n",__FUNCTION__, cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
		if (len <= 0) {
			IPCAM_DBG(ERROR, "recv error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			g_message("[%s] CH(%d) socket fail 2 \n",__FUNCTION__, cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, NULL, NULL);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") == NULL)
	{
		return IPCAM_SETUP_RTN_DONE;
	}
	if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
	{
		g_message("[%s] CH(%d) cam report error \n%s\n",__FUNCTION__, cam_id, sock_buf);
		return IPCAM_SETUP_RTN_FAILED;
	}

	/* request result parsing */
	if ((s = strstr(sock_buf, f_request_str)) == NULL)
	{
		g_message("[%s] CH(%d) msg parse error 1 \n",__FUNCTION__, cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}
	s += strlen(f_request_str);
	e = strstr(s, f_endl_str);
	memset(request_str, 0x00, sizeof(request_str));
	strncpy(request_str, s, (e-s));

	if (strcmp(request_str, "accepted") == 0)
	{
		IPCAM_DBG(MINOR, "CH(%d) request %s\n", cam_id, request_str);
		return IPCAM_SETUP_RTN_DONE;
	}
	else if (strcmp(request_str, "denied") == 0)
	{
		g_message("[%s] CH(%d) request %s \n",__FUNCTION__, cam_id, request_str);
		return IPCAM_SETUP_RTN_FAILED;
	}
	else
	{
		g_message("[%s] CH(%d) msg parse error 2 \n",__FUNCTION__, cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	g_message("[%s] CH(%d) end, fail \n",__FUNCTION__, cam_id);
	return IPCAM_SETUP_RTN_FAILED;
}

extern int itx_cam_prepare_upload(int cam_id)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	int use_ssl = nf_ipcam_is_using_ssl(cam_id);

	if (use_ssl)
	{
		rtn = _ssl_prepare_upload(cam_id);
	}
	else
	{
		rtn = _plain_prepare_upload(cam_id);
	}

	return rtn;
}





extern int itx_cam_upload_fw(ipcam_upgrade_ctx *ctx)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	int cam_id = ctx->ch;
	int use_ssl = nf_ipcam_is_using_ssl(cam_id);

	if (use_ssl)
	{
		rtn = _ssl_upload_fw_hisilicon(cam_id, ctx->fw_name, ctx->fw_data_stream, ctx->fw_len);
	}
	else
	{
		rtn = _common_upload_fw_hisilicon(cam_id, ctx->fw_name, ctx->fw_data_stream, ctx->fw_len);
	}

	return rtn;
}





static int _ssl_force_reboot(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/reboot.cgi";
	char *method = "GET";

	const char *rapi = "reboot";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, "");


	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}
	SSL_CTX_set_options(ctx, SSL_OP_ALL);
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, NULL, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) <= 0)
	{
		IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, &ssl, &ctx);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
		if (len <= 0) {
			IPCAM_DBG(ERROR, "SSL_read error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL && strstr(sock_buf, "401 Unauthorized") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, &ssl, &ctx);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			IPCAM_DBG(WARN, "SSL context creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}
		SSL_CTX_set_options(ctx, SSL_OP_ALL);
		ssl = SSL_new(ctx);
		if (ssl == NULL)
		{
			IPCAM_DBG(WARN, "SSL creation failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, NULL, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		SSL_set_fd(ssl, sock);

		if (SSL_connect(ssl) <= 0)
		{
			IPCAM_DBG(WARN, "SSL connect failed - CH(%d)\n", cam_id);
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = SSL_write(ssl, sock_buf, strlen(sock_buf))) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, &ssl, &ctx);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = SSL_read(ssl, sock_buf+_offset, (sizeof(sock_buf)-_offset));
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, &ssl, &ctx);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
		{
			IPCAM_DBG(ERROR, "CH(%d) cam report error,\n%s\n", cam_id, sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		return IPCAM_SETUP_RTN_DONE;
	}

	return IPCAM_SETUP_RTN_FAILED;
}
static int _plain_force_reboot(int cam_id)
{
	char auth_encbuf[256];
	char auth_str[256];
	char sock_buf[SOCK_BUF_LENGTH];
	char ip_str[16];
	char username[64];
	char password[64];
	unsigned short http_port;

	int sock = 0;
	int len = 0;

	mtable *runtime = get_runtime();
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;

	int _offset = 0;

	char realm[128];
	char nonce[128];
	char *uri = "/cgi-bin/reboot.cgi";
	char *method = "GET";

	const char *rapi = "reboot";


	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);
	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s\r\n", auth_encbuf);

	snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

	nf_ipcam_get_username(cam_id, username);

	sock = _local_get_sock(ip_str, http_port, 5);
	if (sock <= 0)
	{
		IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
		return IPCAM_SETUP_RTN_FAILED;
	}

	if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
	{
		perror("send");
		_release_resource(&sock, NULL, NULL, NULL);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL SEND MSG 1\n%s\n", sock_buf);
#endif

	_offset = 0;
	memset(sock_buf, 0x00, sizeof(sock_buf));
	do
	{
		len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
		if (len <= 0) {
			IPCAM_DBG(ERROR, "recv error\n");
			break;
		}
		_offset += len;
	} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL && strstr(sock_buf, "401 Unauthorized") == NULL);

#if PRINT_HTTP_API_SEND
	IPCAM_DBG(MINOR, "SSL RECV MSG 1\n%s\n", sock_buf);
#endif

	_release_resource(&sock, NULL, NULL, NULL);


	if (strstr(sock_buf, "Digest") != NULL && strstr(sock_buf, "401 Unauthorized") != NULL)
	{
		if (_local_parse_digest_info(sock_buf, realm, nonce) == IPCAM_SETUP_RTN_FAILED)
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		//itx_digest_auth_str(username, password, realm, nonce, uri, method, auth_str);
		unsigned int noncecount = 0;
		digest_auth_create(username, password, uri, method, &noncecount, sock_buf, auth_str);
		strcat(auth_str, "\r\n");

		snprintf(sock_buf, sizeof(sock_buf), _ipcam_api_template, method, uri, rapi, ip_str, auth_str);

		sock = _local_get_sock(ip_str, http_port, 5);
		if (sock <= 0)
		{
			IPCAM_DBG(WARN, "socket fail CH(%d)\n", cam_id);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if ((len = send(sock, sock_buf, strlen(sock_buf), 0)) <= 0)
		{
			perror("send");
			_release_resource(&sock, NULL, NULL, NULL);
			return IPCAM_SETUP_RTN_FAILED;
		}

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST SEND MSG 2\n%s\n", sock_buf);
#endif

		_offset = 0;
		memset(sock_buf, 0x00, sizeof(sock_buf));
		do
		{
			len = recv(sock, sock_buf+_offset, (sizeof(sock_buf)-_offset), 0);
			if (len <= 0) {
				IPCAM_DBG(ERROR, "SSL_read error\n");
				break;
			}
			_offset += len;
		} while (strstr(sock_buf, "\r\n0\r\n\r\n") == NULL);

#if PRINT_HTTP_API_SEND
		IPCAM_DBG(MINOR, "SSL+DIGEST RECV MSG 2\n%s\n", sock_buf);
#endif

		_release_resource(&sock, NULL, NULL, NULL);
	}

	if (strstr(sock_buf, "HTTP/1.1 200") != NULL)
	{
		if (strstr(sock_buf, "WEBSVR_ERR_RET_SUCCESS") == NULL)
		{
			IPCAM_DBG(ERROR, "CH(%d) cam report error,\n%s\n", cam_id, sock_buf);
			return IPCAM_SETUP_RTN_FAILED;
		}
		return IPCAM_SETUP_RTN_DONE;
	}

	return IPCAM_SETUP_RTN_FAILED;
}
extern int itx_cam_force_reboot(int cam_id)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	int use_ssl = nf_ipcam_is_using_ssl(cam_id);

	if (use_ssl)
	{
		rtn = _ssl_force_reboot(cam_id);
	}
	else
	{
		rtn = _plain_force_reboot(cam_id);
	}

	return rtn;
}




extern int ipcam_upgrade_start(ipcam_upgrade_ctx *ctx)
{
	const int max_wait_cnt = 120;
	const int max_upload_buffer_cnt = 5;
	int loop_cnt = 0;
	int ch = ctx->ch;
	NFIPCamUpgradeState state;


	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	/* initial status check */
	if (itx_cam_check_fwup_status(ch, ctx) != IPCAM_SETUP_RTN_DONE)
	{
		g_message("[%s] CH(%d) status check failed 1 \n",__FUNCTION__, ch);
		_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_SET_FWMODE, 1, NF_IPCAM_FW_ERR_READY, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}

	/* 
	 * ITX IP camera has 2 kind of initial upgrade status.
	 *   Case 1. reboot required to change to the f/w upgrade mode
	 *   Case 2. ready to upgrade
	 * If a camera has a status 'Case 1', change the mode first.
	 * If 'Case 2', skip this process and just go ahead.
	 */
	if (ctx->status != IPCAM_FWUP_STATUS_READY)
	{
		if (ctx->status != IPCAM_FWUP_STATUS_MODE_REQUIRED)
		{
			g_message("[%s] CH(%d) status error(%s) \n",__FUNCTION__, ch, IPCAM_UPGRADE_STATUS_STR[ctx->status]);
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_READY, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}

		IPCAM_DBG(MINOR, "CH(%d) request f/w upgrade mode\n", ch);

		if (itx_cam_request_fwup_mode(ch) != IPCAM_SETUP_RTN_DONE)
		{
			g_message("[%s] CH(%d) f/w upgrade mode request failed \n",__FUNCTION__, ch);
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_READY, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}

		ctx->time_waiting = 0;
		while(ctx->time_waiting < ctx->timeout)
		{
			sleep(1);
			ctx->time_waiting++;
			IPCAM_DBG(MINOR, "CH(%d) f/w upgrade mode changing...(%d/%d)\n", ch, ctx->time_waiting, ctx->timeout);
		}

		if (itx_cam_check_fwup_status(ch, ctx) != IPCAM_SETUP_RTN_DONE)
		{
			g_message("[%s] CH(%d) status check failed 2\n",__FUNCTION__, ch);
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_READY, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}

		if (ctx->status != IPCAM_FWUP_STATUS_READY)
		{
			g_message("[%s] CH(%d) status error(%s) \n",__FUNCTION__, ch, IPCAM_UPGRADE_STATUS_STR[ctx->status]);
			_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FWMODE_REBOOT, 1, NF_IPCAM_FW_ERR_READY, 5);
			return IPCAM_SETUP_RTN_FAILED;
		}
	}
	_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_SET_FWMODE, 0, NF_IPCAM_FW_ERR_READY, 5);

	/*
	 * If IP camera is in a upgrade ready state, request to prepare upload
	 */
	if (itx_cam_prepare_upload(ch) != IPCAM_SETUP_RTN_DONE)
	{
		g_message("[%s] CH(%d) failed preparing to upload \n",__FUNCTION__, ch);
		_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_SET_FWMODE, 1, NF_IPCAM_FW_ERR_READY, 5);
		return IPCAM_SETUP_RTN_FAILED;
	}



	_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_UPLOAD, 0, NF_IPCAM_FW_ERR_OK, 5);
	/*
	 * Upload f/w data stream
	 */
	if (itx_cam_upload_fw(ctx) != IPCAM_SETUP_RTN_DONE)
	{
		g_message("[%s] CH(%d) failed upload f/w \n",__FUNCTION__, ch);
		nf_ipcam_get_upgrade_state(ch, &state);
		_nf_ipcam_set_upgrade_state(ch, state.state, 1, state.error_no, state.cur_progress);
		return IPCAM_SETUP_RTN_FAILED;
	}
	nf_ipcam_get_upgrade_state(ch, &state);
	_nf_ipcam_set_upgrade_state(ch, NF_IPCAM_FW_FINAL_REBOOT, 0, NF_IPCAM_FW_ERR_OK, state.cur_progress);

	IPCAM_DBG(MAJOR, "CH(%d) check status start\n", ch);


	/* Verification/write done, force reboot */
#if 0
	if (itx_cam_force_reboot(ch) != IPCAM_SETUP_RTN_DONE)
	{
		IPCAM_DBG(ERROR, "CH(%d) failed reboot request\n", ch);
		return IPCAM_SETUP_RTN_FAILED;
	}
#else
	itx_cam_force_reboot(ch);
#endif

	IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

static int _ti368_set_corridor_mode(int cam_id, int corridor_mode)
{
	const char set_corridor_raw[] = "api=set_setup.video.codec&corridor_view=%d";

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

	mtable *runtime = get_runtime();
	
	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_corridor_raw, corridor_mode );
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_CORRIDOR_MODE, http_api);
	len = _cam_setup_send_plain(cam_id, NF_IPCAM_TYPE_SET_CORRIDOR_MODE, sock_buf);

	return len;
}

static int _ssl_set_corridor_mode(int cam_id, int corridor_mode)
{
	const char set_corridor_raw[] = "api=set_setup.video.codec&corridor_view=%d";

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

	mtable *runtime = get_runtime();

	nf_ipcam_get_ipstr(cam_id, ip_str);
	http_port = nf_ipcam_get_http_port(cam_id);
	nf_ipcam_get_username(cam_id, username);
	nf_ipcam_get_password(cam_id, password);

	strcat(username, ":");
	strcat(username, password);

	b64_encode_string_to_buffer(auth_encbuf, 256, username);
	snprintf(auth_str, 256, "Authorization: Basic %s", auth_encbuf);
	snprintf(http_api, 512, set_corridor_raw, corridor_mode );
	snprintf(sock_buf, SOCK_BUF_LENGTH, str_api_raw,
			ip_str,
			strlen(http_api),
			auth_str,
			http_api);

	_set_last_api_str(cam_id, NF_IPCAM_TYPE_SET_CORRIDOR_MODE, http_api);
	len = _cam_setup_send_ssl(cam_id, NF_IPCAM_TYPE_SET_CORRIDOR_MODE, sock_buf);

	return len;
}

static int _ai_color_convertor(char* color)
{
	int ret = 0;
		if(strcmp(color, "black")==0)
			ret = 0;
		else if(strcmp(color, "white")==0)
			ret = 16777215;
		else if(strcmp(color, "pink_red")==0)
			ret = 4129023;
		else if(strcmp(color, "bright_pink")==0)
			ret = 8323327;
		else if(strcmp(color, "magenta")==0)
			ret = 12517631;
		else if(strcmp(color, "fuchsia")==0)
			ret = 16711935;
		else if(strcmp(color, "purple")==0)
			ret = 16711871;
		else if(strcmp(color, "violet")==0)
			ret = 16711807;
		else if(strcmp(color,  "light_gray")==0)
			ret = 10526880;
		else if(strcmp(color,  "marine")==0)
			ret = 16711743;
		else if(strcmp(color,  "ford_blue")==0)
			ret = 16711680;
		else if(strcmp(color,  "blue")==0)
			ret = 16727808;
		else if(strcmp(color,  "azure")==0)
			ret = 16744192;
		else if(strcmp(color, "deep_sky_blue")==0)
			ret = 16760576;
		else if(strcmp(color,  "aqua")==0)
			ret = 16776960;
		else if(strcmp(color,  "medium_gray")==0)
			ret = 6316128;
		else if(strcmp(color, "green_cyan")==0)
			ret = 12582656;
		else if(strcmp(color, "spring_green")==0)
			ret = 8388352;
		else if(strcmp(color,  "green")==0)
			ret = 4194048;
		else if(strcmp(color,  "lime")==0)
			ret = 65280;
		else if(strcmp(color,  "harlequin")==0)
			ret = 65343;
		else if(strcmp(color,  "chartreuse")==0)
			ret = 65407;
		else if(strcmp(color,  "dark_gray")==0)
			ret = 2105376;
		else if(strcmp(color,  "bitter_lime")==0)
			ret = 65471;
		else if(strcmp(color,  "yellow")==0)
			ret = 65535;
		else if(strcmp(color,  "amber")==0)
			ret = 49151;
		else if(strcmp(color,  "orange")==0)
			ret = 32767;
		else if(strcmp(color,  "red_orange")==0)
			ret = 16383;
		else if(strcmp(color,  "red")==0)
			ret = 255;
	return ret;
}

static char* _ai_color_convertor_reverse(int color)
{
	char* ret;
		if(color==0)
			ret = "black";
		else if(color == 16777215)
			ret = "white";
		else if(color== 4129023)
			ret = "pink_red";
		else if(color== 8323327)
			ret = "bright_pink";
		else if(color==12517631)
			ret = "magenta";
		else if(color==16711935)
			ret = "fuchsia";
		else if(color==16711871)
			ret = "purple";
		else if(color==16711807)
			ret = "violet";
		else if(color==10526880)
			ret = "light_gray";
		else if(color==16711743)
			ret = "marine";
		else if(color==16711680)
			ret = "ford_blue";
		else if(color==16727808)
			ret = "blue";
		else if(color==16744192)
			ret = "azure";
		else if(color==16760576)
			ret = "deep_sky_blue";
		else if(color==16776960)
			ret = "aqua";
		else if(color==6316128)
			ret = "medium_gray";
		else if(color==12582656)
			ret = "green_cyan";
		else if(color==8388352)
			ret = "spring_green";
		else if(color==4194048)
			ret = "green";
		else if(color==65280)
			ret = "lime";
		else if(color==65343)
			ret = "harlequin";
		else if(color==65407)
			ret = "chartreuse";
		else if(color==2105376)
			ret = "dark_gray";
		else if(color==65471)
			ret = "bitter_lime";
		else if(color==65535)
			ret = "yellow";
		else if(color==49151)
			ret = "amber";
		else if(color==32767)
			ret = "orange";
		else if(color==16383)
			ret = "red_orange";
		else if(color==255)
			ret = "red";
		else
			ret = "black";
	return ret;
}

extern int itx_cam_get_ai_rule_engine_and_save_db(int ch)
{
	int i=0;
	int ret = 0;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;
	char* str;
	Token_iterator iter;
	int port = ch;

	int zone_num = 0;
	int count_num = 0;

	mtable* runtime = get_runtime();

	if(runtime[ch].rule_engine.have_ai_engine!=1) // clair-2nd 카멜 아니면 반환a
	{
		return -3;
	}
	// 초기화, cam으로부터 alert / value 필드가 아예 안올 수 있다.
	for(i=0; i<16; i++)
	{
		runtime[ch].rule_engine.rule[i].e_value = 0;
		runtime[ch].rule_engine.rule[i].reset_alert = 0;
	}

	http_init(&ctx);

	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH,"/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.rule.engine");

	rc = http_request_always_init(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = -2;
		goto endl;
	}
	if(ctx.status!=200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}
	
	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str != NULL; str = tok_get_next(&iter)){
		Token_iterator iter_data;
		//const char *key = tok_iterator_init(&iter_data, str, "= \t");
		const char *key = tok_iterator_init(&iter_data, str, "=");
		const char *value = tok_get_next(&iter_data);
		char* compare_key;
		char index;
        //printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
		//

        if(strstr(key,"z_cnt")){
            runtime[ch].rule_engine.z_cnt = atoi(value);
			zone_num = runtime[ch].rule_engine.z_cnt;
        }else if(strstr(key,"c_cnt")){
            runtime[ch].rule_engine.c_cnt = atoi(value);
			count_num = runtime[ch].rule_engine.c_cnt;
        }else if(strstr(key,"en_engine")){
			if(strcmp(value,"on")==0)
				runtime[ch].rule_engine.en_engine = 1;
			else
				runtime[ch].rule_engine.en_engine = 0;
        }
		
		// zone
		if(strstr(key,"type")){
			if(strlen(key) == 5)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==6)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}
			if(value==NULL)
			{
				runtime[ch].rule_engine.rule[i].type = 0;
			}
			else
			{
				if(strstr(value, "zone"))
					runtime[ch].rule_engine.rule[i].type = 1;
				else
					runtime[ch].rule_engine.rule[i].type = 0;
			}
		}else if(strstr(key,"Z_name")){
			if(strlen(key) == 7)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].zone_name[0] =	"";
				memset(runtime[ch].rule_engine.rule[i].zone_name, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_name));
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].zone_name, value, 32);
		}else if(strstr(key,"Z_id")){
			if(strlen(key)==5)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==6)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}


			if(value==NULL)
				runtime[ch].rule_engine.rule[i].zone_id = 0;
			else
	            runtime[ch].rule_engine.rule[i].zone_id = atoi(value);
	    }else if(strstr(key,"Z_color")){  // color covertor 필요
			if(strlen(key)==8)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==9)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
				runtime[ch].rule_engine.rule[i].zone_color = 0;
			else
	            runtime[ch].rule_engine.rule[i].zone_color = _ai_color_convertor(value);
	    }else if(strstr(key,"Z_active")){
			if(strlen(key)==9)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==10)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
            	runtime[ch].rule_engine.rule[i].zone_active = 0;
			else
			{
				if(strcmp(value, "on")==0)
		            runtime[ch].rule_engine.rule[i].zone_active = 1;
				else
		            runtime[ch].rule_engine.rule[i].zone_active = 0;
			}
	    }else if(strstr(key,"object")){
			if(strlen(key)==7)	
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].zone_object[0] = "";
				memset(runtime[ch].rule_engine.rule[i].zone_object, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_object));
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].zone_object, value, 256);
		}else if(strstr(key,"Z_event")){
			if(strlen(key)==8)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==9)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
	           runtime[ch].rule_engine.rule[i].zone_event = 0;
			}
			else
	            runtime[ch].rule_engine.rule[i].zone_event = atoi(value);
	    }else if(strstr(key,"timeout")){
			if(strlen(key)==8)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==9)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].zone_timeout[0]="";
				memset(runtime[ch].rule_engine.rule[i].zone_timeout, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_timeout));
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].zone_timeout, value, 24);
		}else if(strstr(key,"zid")){
			if(strlen(key)==4)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==5)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].zid[0] = "";
				memset(runtime[ch].rule_engine.rule[i].zid, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zid));
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].zid, value, 12);
		}else if(strstr(key,"Z_area")){
			if(strlen(key)==7)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].zone_area[0] ="";
				memset(runtime[ch].rule_engine.rule[i].zone_area, 0x00, sizeof(runtime[ch].rule_engine.rule[i].zone_area));
			}
			else
			{
				strncpy(runtime[ch].rule_engine.rule[i].zone_area, value, 192);
			}
		}
		else if(strstr(key, "pt_cnt"))
		{
			if(strlen(key)==7)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}
			if(value==NULL)
			{
				runtime[ch].rule_engine.rule[i].npts = 0;
			}
			else
			{
	            runtime[ch].rule_engine.rule[i].npts = atoi(value);
			}
		}

		// count
		else if(strstr(key, "C_name"))
		{
			if(strlen(key)==7)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].count_name[0]="";
				memset(runtime[ch].rule_engine.rule[i].count_name, 0x00, sizeof(runtime[ch].rule_engine.rule[i].count_name));
				
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].count_name, value, 32);

		}else if(strstr(key, "c_threshold")){
			if(strlen(key)==12)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==13)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(strcmp(value, "")==0)
				runtime[ch].rule_engine.rule[i].count_threshold = 1;
			else
				runtime[ch].rule_engine.rule[i].count_threshold = atoi(value);

		}else if(strstr(key, "C_color")){
			if(strlen(key)==8)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==9)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(strcmp(value, "")==0)
				runtime[ch].rule_engine.rule[i].count_color = 0;
			else
				runtime[ch].rule_engine.rule[i].count_color = _ai_color_convertor(value);
		}else if(strstr(key, "C_id")){
			if(strlen(key)==5)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==6)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(strcmp(value, "")==0)
				runtime[ch].rule_engine.rule[i].count_id = 0;
			else
				runtime[ch].rule_engine.rule[i].count_id = atoi(value);
		}else if(strstr(key,"C_active")){
			if(strlen(key)==9)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==10)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
            	runtime[ch].rule_engine.rule[i].count_active = 0;
			else
			{
				if(strcmp(value, "on")==0)
		            runtime[ch].rule_engine.rule[i].count_active = 1;
				else
		            runtime[ch].rule_engine.rule[i].count_active = 0;
			}
	    }else if(strstr(key, "C_area")){
			if(strlen(key)==7)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==8)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(value==NULL)
			{
				//runtime[ch].rule_engine.rule[i].count_area[0] = "";
				memset(runtime[ch].rule_engine.rule[i].count_area, 0x00, sizeof(runtime[ch].rule_engine.rule[i].count_area));
			}
			else
				strncpy(runtime[ch].rule_engine.rule[i].count_area, value, 192);
		}

		// cf
		else if(strstr(key, "e_value"))
		{
			if(strlen(key)==8)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==9)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(strcmp(value, "")==0)
				runtime[ch].rule_engine.rule[i].e_value = 0;
			else
				runtime[ch].rule_engine.rule[i].e_value = atoi(value);
		}else if(strstr(key, "re_alert")){
			if(strlen(key)==9)
				i = key[strlen(key)-1] - '0';
			else if(strlen(key)==10)
				i = (key[strlen(key)-2] - '0')*10 + (key[strlen(key)-1] - '0');
			else{
				goto next;
			}

			if(strcmp(value,"off")==0)
				runtime[ch].rule_engine.rule[i].reset_alert = 0;
			else
				runtime[ch].rule_engine.rule[i].reset_alert = 1;
		}

next:
		release_token(&iter_data);
	}
	release_token(&iter);

	// DB Setting
	if(1)
	{
		nf_sysdb_lock(NF_SYSDB_CATE_CAM);
		GValue _value = {0,};

		//en_engine
		g_value_init(&_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&_value, runtime[port].rule_engine.en_engine);
		nf_sysdb_set_key1("cam.dvabx.cfg.R%d.en_engine",port,&_value,NULL);
		g_value_unset(&_value);

		// z_cnt
		g_value_init(&_value, G_TYPE_UINT);
		g_value_set_uint(&_value, runtime[port].rule_engine.z_cnt);
		nf_sysdb_set_key1("cam.dvabx.rule.R%d.nzones",port,&_value,NULL);
		g_value_unset(&_value);

		// c_cnt 
		g_value_init(&_value, G_TYPE_UINT);
		g_value_set_uint(&_value, runtime[port].rule_engine.c_cnt);
		nf_sysdb_set_key1("cam.dvabx.rule.R%d.ncounters",port,&_value,NULL);
		g_value_unset(&_value);

		//zone db set
		for(i=0; i<zone_num; i++)
		{
			// rule.type 
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].type);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.type",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.zone_name 
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].zone_name);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.name",port,i,&_value,NULL);
			g_value_unset(&_value);


			// rule.zone_id 
			g_value_init(&_value, G_TYPE_INT);
			g_value_set_int(&_value, runtime[port].rule_engine.rule[i].zone_id);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.id",port,i,&_value,NULL);
			g_value_unset(&_value);


			// rule.zone_color 
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].zone_color);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.d_color",port,i,&_value,NULL);
			g_value_unset(&_value);


			// rule.zone_active
			g_value_init(&_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&_value, runtime[port].rule_engine.rule[i].zone_active);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.active",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.zone_object
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].zone_object);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.interest_obj",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.zone_event
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].zone_event);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.enabled",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.zone_timeout	
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].zone_timeout);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.time_sarlf",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.zone_area
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].zone_area);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.pt",port,i,&_value,NULL);
			g_value_unset(&_value);
			
			// rule.count_threshold - uint
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].count_threshold);
			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.c_threshold",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.pt_cnt (ntps)
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].npts);
			nf_sysdb_set_key2("cam.dvabx.rule.R%d.Z%d.npts", port, i, &_value, NULL);
			g_value_unset(&_value);

		}

		//count db set


		for(i=0; i<count_num; i++)
		{
			
			// rule.zid
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].zid);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.zid",port,i,&_value,NULL);
			g_value_unset(&_value);

			//
			// rule.count_name - char
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].count_name);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.name",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.count_id - int
			g_value_init(&_value, G_TYPE_INT);
			g_value_set_int(&_value, runtime[port].rule_engine.rule[i].count_id);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.id",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.count_color - color
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].count_color);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.d_color",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.count_active - bool
			g_value_init(&_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&_value, runtime[port].rule_engine.rule[i].count_active);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.active",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.count_event - int
			g_value_init(&_value, G_TYPE_UINT);
			g_value_set_uint(&_value, runtime[port].rule_engine.rule[i].count_event);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.enabled",port,i,&_value,NULL);
			g_value_unset(&_value);

			

			// rule.count_area - char
			g_value_init(&_value, G_TYPE_STRING);
			g_value_set_string(&_value, runtime[port].rule_engine.rule[i].count_area);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.pt",port,i,&_value,NULL);
			g_value_unset(&_value);

				
			// rule.e_value - ing
			g_value_init(&_value, G_TYPE_INT);
			g_value_set_int(&_value, runtime[port].rule_engine.rule[i].e_value);
			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.e_value",port,i,&_value,NULL);
			g_value_unset(&_value);

			// rule.reset_alert - bool
			g_value_init(&_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&_value, runtime[port].rule_engine.rule[i].reset_alert);

			nf_sysdb_set_key2("cam.dvabx.rule.R%d.C%d.reset_alert",port,i,&_value,NULL);
			g_value_unset(&_value);
		}
	//nf_sysdb_save("cam");  , nand write
		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
		//nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	}

	
endl:
	http_release(&ctx);
	return ret;
}

static int _get_cam_dl_option(int ch)
{
	int i=0;
	int ret = -1;
	char* str = 0;
	HTTP_CTX ctx;
	int rc = 0;
	mtable* runtime = get_runtime();
	Token_iterator iter;

	if(runtime[ch].rule_engine.have_ai_engine!=1)
		return -3;

	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.engine.option");

	rc = http_request_always_init(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT)
	{
		ret = -2;
		goto endl;
	}
	if(ctx.status!=200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&"); str!=NULL; str = tok_get_next(&iter))
	{
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "=");
		const char *value = tok_get_next(&iter_data);
		char* compare_key;
		char index;

		if(strstr(key, "track_ref")){
			if(strstr(value, "cent"))
			{
				runtime[ch].dl_option.track_ref = 0;
			}
			else if(strstr(value, "ground"))
			{
				runtime[ch].dl_option.track_ref = 1;
			}
			else
			{
				printf("[%s:%d] cam response error (track_ref:%s), set default (centroid) \n", __func__, __LINE__, value);
				runtime[ch].dl_option.track_ref = 0;
			}

		}else if(strstr(key, "en_ignore"))
		{
			if(strstr(value, "on"))
			{
				runtime[ch].dl_option.en_static_filter = 1;
			}
			else if(strstr(value,"off"))
			{
				runtime[ch].dl_option.en_static_filter = 0;
			}
			else
			{
				printf("[%s:%d] cam response error (en_ignore:%s), set default (off) \n", __func__, __LINE__, value);
				runtime[ch].dl_option.en_static_filter = 0;
			}
		}else if(strstr(key, "ignore_level"))
		{
			if(strstr(value, "low"))
			{
				runtime[ch].dl_option.static_filter_sense = 0;
			}
			else if(strstr(value, "mid"))
			{
				runtime[ch].dl_option.static_filter_sense = 1;
			}
			else if(strstr(value, "high"))
			{
				runtime[ch].dl_option.static_filter_sense = 2;
			}
			else
			{
				printf("[%s:%d] cam response error (ignore_level:%s), set default (low) \n", __func__, __LINE__, value);
				runtime[ch].dl_option.static_filter_sense = 0;
			}


		}
		release_token(&iter_data);
	}
	release_token(&iter);
	ret = 0;
endl:
	return ret;

}

static void _save_dl_option_in_db(int ch)
{
	mtable *runtime = get_runtime();
	GValue _value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	// track_ref
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].dl_option.track_ref);
	nf_sysdb_set_key1("cam.dvabx.opt.R%d.track_ref", ch, &_value, NULL);
	g_value_unset(&_value);
	
	// en_ignore
	g_value_init(&_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&_value, (gint)runtime[ch].dl_option.en_static_filter);
	nf_sysdb_set_key1("cam.dvabx.opt.R%d.en_static_filter", ch, &_value, NULL);
	g_value_unset(&_value);
	
	// ignore_level
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].dl_option.static_filter_sense);
	nf_sysdb_set_key1("cam.dvabx.opt.R%d.static_filter_sense", ch, &_value, NULL);
	g_value_unset(&_value);

	
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
}

extern int itx_cam_get_dl_option_and_save_db(int ch)
{
	int ret = 0;
	mtable* runtime = get_runtime();
	if(runtime[ch].rule_engine.have_ai_engine != 1)
		return -3;

	ret = _get_cam_dl_option(ch);
	if(ret == 0)
		_save_dl_option_in_db(ch);
	else
		return ret;
	
	return ret;
}

extern void itx_cam_set_ai_dl_option(int ch)
{
	int ret = 0;
	int rc = 0;
	char *en_engine;
	char *track_ref;
	char *en_ignore;
	char *ignore_level;
	mtable *runtime = get_runtime();

	HTTP_CTX ctx;
	if(runtime[ch].rule_engine.have_ai_engine != 1)
	{
		return;
	}
	
	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.engine.option");

	// convert (DB -> query string)
	
	// en_engine
	if(runtime[ch].rule_engine.en_engine == 1)
		en_engine = "on";
	else
		en_engine = "off";
	http_data_set(&ctx, HTTP_ADD_QUERY, "en_engine", en_engine);
	
	// track_ref
	if(runtime[ch].dl_option.track_ref == 0)
		track_ref = "cent";
	else if(runtime[ch].dl_option.track_ref == 1)
		track_ref = "ground";
	else
	{
		printf("[%s:%d] DB(DL Option-track_ref) is wrong value(%d), default value set .. \n", __FUNCTION__, __LINE__, runtime[ch].dl_option.track_ref);
		track_ref = "cent";
	}
	http_data_set(&ctx, HTTP_ADD_QUERY, "track_ref", track_ref);

	// en_ignore
	if(runtime[ch].dl_option.en_static_filter == 0)
		en_ignore = "off";
	else
		en_ignore = "on";
	http_data_set(&ctx, HTTP_ADD_QUERY, "en_ignore", en_ignore);

	// ignore_level
	if(runtime[ch].dl_option.static_filter_sense == 0)
		ignore_level = "low";
	else if(runtime[ch].dl_option.static_filter_sense == 1)
		ignore_level = "mid";
	else if(runtime[ch].dl_option.static_filter_sense == 2)
		ignore_level = "high";
	else
	{
		printf("[%s:%d] DB(DL Option-ignore_level) not supported value(%d), default value set(low) .. \n", __FUNCTION__, __LINE__, runtime[ch].dl_option.static_filter_sense);
	}
	http_data_set(&ctx, HTTP_ADD_QUERY, "ignore_level", ignore_level);

	rc = http_request_always_init(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT)
	{
		ret = -2;
		goto endl;
	}
	if(ctx.status!=200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;

	}
endl:
	http_release(&ctx);
	return;
}


// 파리미터로 받은 rule_engine 값들을 카메라에 SET 
extern void itx_cam_set_ai_rule_engine(void *arg)
{
	json_t *json_z = json_object();
	json_t *json_c = json_object();
	json_t *_api_z = json_object();
	json_t *_api_c = json_object();

	char* json_str=NULL;

	int ch = (int)arg;
	int i=0;
	int ret = -1;
	int rc = 0;
	char* str;
	int zone_num = 0;
	int count_num = 0;
	mtable* runtime = get_runtime();

	// on/off, color_name 
	char z_cnt[32];
	char c_cnt[32];
	char en_engine[32];

	//char *zone_name;
	char zone_id[32];
	char zone_color[32];
	char zone_active[32];
	char zone_event[32];
	char zone_type[32];
	char npts[32];
	//char *zone_object;
	//char *zone_timeout;
	//char zone_area*;
	//char *zid;
	//
	//char *count_name;
	char count_id[32];
	char count_color[32];
	char count_active[32];
	char count_threshold[32];
	char count_event[32];
	//char *count_area;
	
	char e_value[32];
	char reset_alert[32];

	// for query
	char q_Z_name[10];
	char q_C_name[10];
	char q_Z_id[10];
	char q_C_id[10];
	char q_type[10];
	char q_Z_color[10];
	char q_C_color[10];
	char q_Z_active[15];
	char q_C_active[15];
	char q_object[10];
	char q_Z_event[10];
	char q_C_event[10];
	char q_timeout[10];
	char q_c_threshold[15];
	char q_e_value[10];
	char q_re_alert[15];
	char q_zid[10];
	char q_Z_area[10];
	char q_C_area[10];
	char q_npts[10];

	if(runtime[ch].rule_engine.en_engine == 1)
		strncpy(en_engine, "on", sizeof(en_engine));
	else
		strncpy(en_engine, "off", sizeof(en_engine));
	json_object_set_new(_api_z, "en_engine", json_string(en_engine));

	snprintf(z_cnt, sizeof(z_cnt), "%d", runtime[ch].rule_engine.z_cnt);
	snprintf(c_cnt, sizeof(c_cnt), "%d", runtime[ch].rule_engine.c_cnt);

	json_object_set_new(_api_z, "z_cnt", json_string(z_cnt));
	//json_object_set_new(_api_z, "c_cnt", json_string(c_cnt));
	json_object_set_new(_api_z, "is_zone", json_string("zone"));

	// test
	HTTP_CTX ctx;	
	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action_json.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.rule.engine");

	// z_cnt , include en_engine 
	zone_num = runtime[ch].rule_engine.z_cnt;
	for(i=0; i<zone_num; i++)
	{	
		snprintf(q_Z_name,sizeof(q_Z_name), "Z_name%d", i);
		snprintf(q_Z_id, sizeof(q_Z_id), "Z_id%d", i);
		snprintf(q_type, sizeof(q_type), "type%d", i);
		snprintf(q_Z_color, sizeof(q_Z_color), "Z_color%d", i);
		snprintf(q_Z_active, sizeof(q_Z_active), "Z_active%d", i);
		snprintf(q_object, sizeof(q_object), "object%d", i);
		snprintf(q_Z_event, sizeof(q_Z_event), "Z_event%d", i);
		snprintf(q_timeout, sizeof(q_timeout), "timeout%d", i);
		snprintf(q_Z_name, sizeof(q_Z_name), "Z_name%d", i);
		snprintf(q_Z_area, sizeof(q_Z_area), "Z_area%d", i);
		snprintf(q_c_threshold, sizeof(q_c_threshold), "c_threshold%d", i);
		snprintf(q_npts, sizeof(q_npts), "pt_cnt%d",i);

		
		// zone_id
		snprintf(zone_id, sizeof(zone_id),"%d", runtime[ch].rule_engine.rule[i].zone_id);
		json_object_set_new(_api_z, q_Z_id, json_string(zone_id));

		// color convertor
		strncpy(zone_color, _ai_color_convertor_reverse(runtime[ch].rule_engine.rule[i].zone_color), sizeof(zone_color));
		json_object_set_new(_api_z, q_Z_color, json_string(zone_color));


		// zone_active
		if(runtime[ch].rule_engine.rule[i].zone_active==1)
			strncpy(zone_active ,"on",32);
		else
			strncpy(zone_active ,"off",32);
		json_object_set_new(_api_z, q_Z_active, json_string(zone_active));
		
		// type
		if(runtime[ch].rule_engine.rule[i].type == 0)
		{
			strncpy(zone_type,"line",32);
		}
		else
			strncpy(zone_type, "zone", 32);
		json_object_set_new(_api_z, q_type, json_string(zone_type));

		// threshold
		snprintf(count_threshold, sizeof(count_threshold),"%d",runtime[ch].rule_engine.rule[i].count_threshold);
		json_object_set_new(_api_z, q_c_threshold, json_string(count_threshold));

		// zone_event
		snprintf(zone_event, sizeof(zone_event),"%d", runtime[ch].rule_engine.rule[i].zone_event);
		json_object_set_new(_api_z, q_Z_event, json_string(zone_event));

		// Zone_name
		json_object_set_new(_api_z, q_Z_name, json_string(runtime[ch].rule_engine.rule[i].zone_name));

		// zone_object
		json_object_set_new(_api_z, q_object, json_string(runtime[ch].rule_engine.rule[i].zone_object));

		//zone_timeout
		json_object_set_new(_api_z, q_timeout, json_string(runtime[ch].rule_engine.rule[i].zone_timeout));

		// zone area
		json_object_set_new(_api_z, q_Z_area, json_string(runtime[ch].rule_engine.rule[i].zone_area));

		//npts
		snprintf(npts, sizeof(npts),"%d", runtime[ch].rule_engine.rule[i].npts);
		json_object_set_new(_api_z, q_npts, json_string(npts));

	}
	json_object_set_new(json_z, "api",_api_z);
	json_str = json_dumps(json_z, JSON_ENCODE_ANY);
	http_data_set(&ctx, HTTP_SET_POST, json_str);
	
	rc = http_request_always_init(&ctx);
	free(json_str);
	// free json
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] (Zone:%d cam) http response Time out[%d]\n", __func__, __LINE__, ch, ctx.status);
		ret = -2;
		http_release(&ctx);
		
	goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] (Zone:%d cam) http response error[%d]\n", __func__, __LINE__, ch, ctx.status);
		ret = -1;
		http_release(&ctx);
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] (Zone:%d cam) http response error[%d]\n", __func__, __LINE__,ch, ctx.status);
		ret = -1;
		http_release(&ctx);
		goto endl;
	}
	http_release(&ctx);


	// Count
	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action_json.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.rule.engine");
	
	//json_object_set_new(_api_c, "z_cnt", json_string(z_cnt));
	json_object_set_new(_api_c, "c_cnt", json_string(c_cnt));
	json_object_set_new(_api_c, "is_zone", json_string("counter"));

	// c_cnt
	count_num = runtime[ch].rule_engine.c_cnt;
	for(i=0; i<count_num; i++)
	{
		snprintf(q_C_name,sizeof(q_C_name), "C_name%d", i);
		snprintf(q_C_id, sizeof(q_C_id), "C_id%d", i);
		snprintf(q_C_color, sizeof(q_C_color), "C_color%d", i);
		snprintf(q_C_active, sizeof(q_C_active), "C_active%d", i);
		snprintf(q_C_event, sizeof(q_C_event), "C_event%d", i);
		snprintf(q_e_value, sizeof(q_e_value), "e_value%d", i);
		snprintf(q_re_alert, sizeof(q_re_alert), "re_alert%d", i);
		snprintf(q_zid, sizeof(q_zid), "zid%d", i);
		snprintf(q_C_area, sizeof(q_C_area), "C_area%d", i);

		// C_id
		
		snprintf(count_id, sizeof(count_id),"%d", runtime[ch].rule_engine.rule[i].count_id);
		json_object_set_new(_api_c, q_C_id, json_string(count_id));

		// count_event
		snprintf(count_event, sizeof(count_event),"%d", runtime[ch].rule_engine.rule[i].count_event);
		json_object_set_new(_api_c, q_C_event, json_string(count_event));

		// count color
		strncpy(count_color, _ai_color_convertor_reverse(runtime[ch].rule_engine.rule[i].count_color), sizeof(count_color));
		json_object_set_new(_api_c, q_C_color, json_string(count_color));

		// count active
		if(runtime[ch].rule_engine.rule[i].count_active==1)
			strncpy(count_active ,"on",32);
		else
			strncpy(count_active ,"off",32);
		json_object_set_new(_api_c, q_C_active, json_string(count_active));

		// count e_value
		snprintf(e_value, sizeof(e_value),"%d", runtime[ch].rule_engine.rule[i].e_value);
		json_object_set_new(_api_c, q_e_value, json_string(e_value));

		// reset_alert
		if(runtime[ch].rule_engine.rule[i].reset_alert==1)
			strncpy(reset_alert ,"on",32);
		else
			strncpy(reset_alert , "off",32);
		json_object_set_new(_api_c, q_re_alert, json_string(reset_alert));

		// count_name
		json_object_set_new(_api_c, q_C_name, json_string(runtime[ch].rule_engine.rule[i].count_name));

		// count_area
		json_object_set_new(_api_c, q_C_area, json_string(runtime[ch].rule_engine.rule[i].count_area));

		// count zid
		json_object_set_new(_api_c, q_zid, json_string(runtime[ch].rule_engine.rule[i].zid));

	}

	json_object_set_new(json_c, "api",_api_c);
	json_str = json_dumps(json_c, JSON_ENCODE_ANY);
	http_data_set(&ctx, HTTP_SET_POST, json_str);


	rc = http_request_always_init(&ctx);
	free(json_str);
	// fre json
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] (Counter:%d cam) http response Time out[%d]\n", __func__, __LINE__, i, ctx.status);
		ret = -2;	
		http_release(&ctx);
	goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] (Counter:%d cam) http response error[%d]\n", __func__, __LINE__, i, ctx.status);
		ret = -1;
		http_release(&ctx);
		goto endl;
	}
	if(ctx.status != 200){
		printf("[%s:%d] (Counter:%d cam) http response error[%d]\n", __func__, __LINE__,i, ctx.status);
		ret = -1;
		http_release(&ctx);
		goto endl;
	}
	http_release(&ctx);

	ret = 1;
endl:
	json_decref(json_z);
	json_decref(json_c);
	json_decref(_api_z);
	json_decref(_api_c);
	json_z = NULL;
	json_c = NULL;
	_api_z = NULL;
	_api_c = NULL;

}


// get embedded osd infos from ITX cam
static int _get_cam_embedded_osd(int ch)
{
	int rc=0;	
	int ret = 0;
	char* str;
	Token_iterator iter;
	HTTP_CTX ctx;
	mtable* runtime = get_runtime();

	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH,"/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.ai.osd");
	rc = http_request_always_init(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT)
	{
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}

	for(str = tok_iterator_init(&iter, http_get_res(&ctx), "\r\n&" ); str != NULL; str = tok_get_next(&iter))
	{
		Token_iterator iter_data;
		const char *key = tok_iterator_init(&iter_data, str, "=");
		const char *value = tok_get_next(&iter_data);
		char* compare_key;
		char index;


		if(strstr(key, "mode"))
		{
			if(strstr(value, "off"))
				runtime[ch].e_osd.display_mode = 0;
			else if(strncmp(value, "on",2)==0)
				runtime[ch].e_osd.display_mode = 1;
			else if(strstr(value, "event_on"))
				runtime[ch].e_osd.display_mode = 2;
			else
			{
				printf("[%s:%d] cam response error (mode:%s), set default \n", __func__, __LINE__, value);
				runtime[ch].e_osd.display_mode = 0;
			}
		}
		else if(strstr(key,"obj_color"))
		{
			if(strstr(value, "black"))
				runtime[ch].e_osd.object_color = 0;
			else if(strstr(value,"white"))
				runtime[ch].e_osd.object_color = 1;
			else if(strstr(value,"magenta"))
				runtime[ch].e_osd.object_color = 2;
			else if(strstr(value,"orange"))
				runtime[ch].e_osd.object_color = 3;
			else if(strstr(value,"yellow"))
				runtime[ch].e_osd.object_color = 4;
			else if(strstr(value,"red"))
				runtime[ch].e_osd.object_color = 5;
			else if(strstr(value,"blue"))
				runtime[ch].e_osd.object_color = 6;
			else if(strstr(value,"green"))
				runtime[ch].e_osd.object_color = 7;
			else
			{
				printf("[%s:%d] cam response error (object_color:%s), set default \n", __func__, __LINE__, value);
				runtime[ch].e_osd.object_color = 0;

			}
		}
		else if(strstr(key,"rule_color"))
		{
			if(strstr(value, "black"))
				runtime[ch].e_osd.rule_color = 0;
			else if(strstr(value,"white"))
				runtime[ch].e_osd.rule_color = 1;
			else if(strstr(value,"magenta"))
				runtime[ch].e_osd.rule_color = 2;
			else if(strstr(value,"orange"))
				runtime[ch].e_osd.rule_color = 3;
			else if(strstr(value,"yellow"))
				runtime[ch].e_osd.rule_color = 4;
			else if(strstr(value,"red"))
				runtime[ch].e_osd.rule_color = 5;
			else if(strstr(value,"blue"))
				runtime[ch].e_osd.rule_color = 6;
			else if(strstr(value,"green"))
				runtime[ch].e_osd.rule_color = 7;
			else
			{
				printf("[%s:%d] cam response error (rule_color:%s), set default \n", __func__, __LINE__, value);
				runtime[ch].e_osd.rule_color = 0;

			}

		}
		else if(strstr(key,"event_color"))
		{
			if(strstr(value, "black"))
				runtime[ch].e_osd.event_color = 0;
			else if(strstr(value,"white"))
				runtime[ch].e_osd.event_color = 1;
			else if(strstr(value,"magenta"))
				runtime[ch].e_osd.event_color = 2;
			else if(strstr(value,"orange"))
				runtime[ch].e_osd.event_color = 3;
			else if(strstr(value,"yellow"))
				runtime[ch].e_osd.event_color = 4;
			else if(strstr(value,"red"))
				runtime[ch].e_osd.event_color = 5;
			else if(strstr(value,"blue"))
				runtime[ch].e_osd.event_color = 6;
			else if(strstr(value,"green"))
				runtime[ch].e_osd.event_color = 7;
			else
			{
				printf("[%s:%d] cam response error (event_color:%s), set default \n", __func__, __LINE__, value);
				runtime[ch].e_osd.event_color = 0;

			}

		}
		else if(strstr(key,"line_width"))
		{
			runtime[ch].e_osd.line_width = atoi(value)-1;
		}
		else if(strstr(key,"line_transparency"))
		{
			runtime[ch].e_osd.line_transparency = atoi(value);
		}
		else if(strstr(key,"bbox_obj_type"))
		{
			if(value==NULL)
			{
				memset(runtime[ch].e_osd.object_type, 0x00, sizeof(runtime[ch].e_osd.object_type));
			}
			else
				strncpy(runtime[ch].e_osd.object_type, value, sizeof(runtime[ch].e_osd.object_type));
		}
		release_token(&iter_data);
	}
	release_token(&iter);
	ret = 0; // success

endl:
	http_release(&ctx);
	return ret;
}

// save embedded osd info by runtime value
static void _save_embedded_osd_info_in_db(int ch)
{
	mtable* runtime = get_runtime();
	GValue _value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);


	// display mode
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.display_mode);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.display_mode", ch, &_value, NULL);
	g_value_unset(&_value);

	// object_color
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.object_color);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.object_color", ch, &_value, NULL);
	g_value_unset(&_value);

	// rule_color
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.rule_color);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.rule_color", ch, &_value, NULL);
	g_value_unset(&_value);

	// event_color
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.event_color);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.event_color", ch, &_value, NULL);
	g_value_unset(&_value);

	// line_width
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.line_width);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.line_width", ch, &_value, NULL);
	g_value_unset(&_value);

	//line_transparency
	g_value_init(&_value, G_TYPE_UINT);
	g_value_set_uint(&_value, runtime[ch].e_osd.line_transparency);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.line_transparency", ch, &_value, NULL);
	g_value_unset(&_value);

	//object_type
	g_value_init(&_value, G_TYPE_STRING);
	g_value_set_string(&_value, runtime[ch].e_osd.object_type);
	nf_sysdb_set_key1("cam.dvabx.osd.R%d.object_type", ch, &_value, NULL);
	g_value_unset(&_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
}

// return 
// 0 : success
// -1 : cam http response error (not 200 ok)
// -2 : connect time out
// -3 : not clair-2nd cam
extern int itx_cam_get_embedded_osd_and_save_db(int ch)
{
	int ret = 0;
	mtable* runtime = get_runtime();
	if(runtime[ch].rule_engine.have_ai_engine != 1)
		return -3;

	ret = _get_cam_embedded_osd(ch);
	if(ret == 0) // if get cam info success
		_save_embedded_osd_info_in_db(ch);
	else
		return ret;

	return ret;
}

// set camera 'embedded osd' by runtime value
// 0 : OK
// -1 : cam http response error (not 200 ok)
// -2 : connect time out
// -3 : not clair-2nd cam
extern void itx_cam_set_embedded_osd(int ch)
{
	int ret = 0;
	int rc=0;
	char* mode;
	char* obj_color;
	char* rule_color;
	char* event_color;
	char line_width[5];
	char line_transparency[5];
	char* bbox_obj_type;
	HTTP_CTX ctx;
	mtable* runtime = get_runtime();
	if(runtime[ch].rule_engine.have_ai_engine != 1)
		return;

	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.ai.osd");

	// convert (DB -> api param foramt)
	// mode
	if(runtime[ch].e_osd.display_mode == 0)
		mode = "off";
	else if(runtime[ch].e_osd.display_mode == 1)
		mode = "on";
	else if(runtime[ch].e_osd.display_mode == 2)
		mode = "event_on";
	http_data_set(&ctx, HTTP_ADD_QUERY, "mode", mode);
	
	// obj_color
	obj_color = _e_osd_convert_int_to_str(runtime[ch].e_osd.object_color);
	http_data_set(&ctx, HTTP_ADD_QUERY, "obj_color", obj_color);

	// event_color
	event_color = _e_osd_convert_int_to_str(runtime[ch].e_osd.event_color);
	http_data_set(&ctx, HTTP_ADD_QUERY, "event_color", event_color);
	
	// rule_color
	rule_color = _e_osd_convert_int_to_str(runtime[ch].e_osd.rule_color);
	http_data_set(&ctx, HTTP_ADD_QUERY, "rule_color", rule_color);
		
	
	// line_width
	snprintf(line_width, sizeof(line_width), "%d",runtime[ch].e_osd.line_width+1);
	http_data_set(&ctx, HTTP_ADD_QUERY, "line_width", line_width);
	

	// line_transparency
	snprintf(line_transparency, sizeof(line_transparency), "%d", runtime[ch].e_osd.line_transparency);
	http_data_set(&ctx, HTTP_ADD_QUERY, "line_transparency", line_transparency);
	

	// bbox_obj_type
	if(strlen(runtime[ch].e_osd.object_type) == 0)
	{
		http_data_set(&ctx, HTTP_ADD_QUERY, "bbox_obj_type", "");
	}
	else
	{
		http_data_set(&ctx, HTTP_ADD_QUERY, "bbox_obj_type", runtime[ch].e_osd.object_type);
	}


	rc = http_request_always_init(&ctx);

	if(rc == CURLE_OPERATION_TIMEDOUT)
	{
		ret = -2;
		goto endl;
	}
	if(ctx.status != 200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		ret = -1;
		goto endl;
	}
endl:
	//return ret;
	http_release(&ctx);
	return;
}

// set camera 'embedded_osd - mode only' by flag
/* parameter - flag
// 0 : off
// 1 : DB Value
*/
/* return
// 0 : ok
// -1 : cam http response error (not 200 ok )
// -2 : connect time out 
// -3 : not clair-2nd camera
*/
extern int nf_ipcam_set_embedded_osd_mode(int ch, int flag)
{
	HTTP_CTX ctx;
	char* mode;
	mtable *runtime = get_runtime();
	int rc=0;
	char key[64];
	unsigned int display_mode = 0;

	if(runtime[ch].rule_engine.have_ai_engine != 1)
	{
		return -3;
	}

	http_init(&ctx);
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.ai.osd");

	if(flag == 0)
	{
		mode = "off";
	}
	else if(flag == 1)
	{
		//mode = "on";
		snprintf(key, 64, "cam.dvabx.osd.R%d.display_mode", ch);
		display_mode = nf_sysdb_get_uint(key);
		runtime[ch].e_osd.display_mode = display_mode; // runtime update
		
		if(display_mode == 0)
			mode = "off";
		else if(display_mode == 1)
			mode = "on";
		else if(display_mode == 2)
			mode = "event_on";
		else
			mode = "on"; // default

	}
	else
	{
		// to do anything
	}

	http_data_set(&ctx, HTTP_ADD_QUERY, "mode", mode);

	rc = http_request_always_init(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT)
	{
		http_release(&ctx);
		return -2;
	}
	if(ctx.status != 200)
	{
		printf("[%s:%d] http response error[%d]\n", __func__, __LINE__, ctx.status);
		http_release(&ctx);
		return -1;
	}
	else
	{
		http_release(&ctx);
		return 0;
	}
}

static char* _e_osd_convert_int_to_str(int color)
{
	if(color==0)
		return "black";
	else if(color==1)
		return "white";
	else if(color==2)
		return "magenta";
	else if(color==3)
		return "orange";
	else if(color==4)
		return "yellow";
	else if(color==5)
		return "red";
	else if(color==6)
		return "blue";
	else if(color==7)
		return "green";
	else
		return "black"; // default set

}

#endif //__NF_IPCAM_DRIVER_ITX_C__
