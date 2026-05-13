/**
 * @file nf_onvif_media.c
 * @brief ONVIF MEDIA 구현
 * @author jykim
 * @date 2012-02-03
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */

#ifndef __NF_ONVIF_MEDIA_C__
#define __NF_ONVIF_MEDIA_C__


#include <stdsoap2.h>
#include <onvifStub.h>
#include <onvifH.h>
// #include <nmf.h>
// #include <nmf_display.h>
// #include <nmf_mrtp_pipe.h>
// #include <gst/nf/gstmrtpsrc.h>
#include <gobj.h>
#include <gobjmrtppipe.h>

#include <nf_ipcam_defs.h>
#define _GNU_SOURCE 
#include <string.h>
#include "nf_ipcam_driver_sony.h"


/** @def MAX_ONVIF_STREAM
 *  @brief ONVIF 카메라의 stream 수 제한.
 */
#if DISABLE_ONVIF_2ND_STREAM
#define MAX_ONVIF_STREAM 1
#else
#define MAX_ONVIF_STREAM 2
#endif

#define TOKEN_LEN 	(64) // > UUID SIZE 
#define NAME_LEN 	(64) // > UUID SIZE
#define MAX_ATT		(32) // MAX ATTRIBUTE
#define MAX_RES_SIZE	(32) // MAX Video Resolution Number

struct onvif_profile {
	char token[TOKEN_LEN];
	char name[NAME_LEN];
	char vs_token[TOKEN_LEN];	// VideoSourceToken
	char vsc_token[TOKEN_LEN];	// VideoSourceConfigurationToken
	char as_token[TOKEN_LEN];
	char asc_token[TOKEN_LEN];	// AudioSourceConfigurationToken
	char vec_token[TOKEN_LEN];	// VideoEncoderConfigurationToken
	char aec_token[TOKEN_LEN];	// AudioEncoderConfigurationToken
	char aoc_token[TOKEN_LEN];	// AudioOutputConfigurationToken
	char adc_token[TOKEN_LEN];	// AudioDecoderConfigurationToken
	char ptz_token[TOKEN_LEN];	// PTZConfigurationToken
	char vac_token[TOKEN_LEN];	// VideoAnalyticsToken
	char meta_token[TOKEN_LEN];	// MetadataConfigurationToken
	float vec_quality;
	int h264_ProfilesSupported;
	int motion_type;
};

struct onvif_profiles {
	int size;
	struct onvif_profile profile[MAX_ATT];
};

struct onvif_video_encoder {
	char token[TOKEN_LEN];
	char name[NAME_LEN];
	int encoding;
};

struct onvif_video_encoders {
	int size;
	struct onvif_video_encoder video_encoder[MAX_ATT];
};

struct onvif_video_encoder_option {
	int h264_supported;
	int width;
	int height;
	int profile_supported;
};

struct onvif_audio_outputs {
	int size;
	char token[MAX_ATT][TOKEN_LEN];
};

struct onvif_audio_decoders {
	int size;
	char token[MAX_ATT][TOKEN_LEN];
};

struct onvif_audio_encoder {
	char token[TOKEN_LEN];
	int bitrate;
	int samplerate;
	int encoding;
};

struct onvif_audio_encoders {
	int size;
	struct onvif_audio_encoder enc[MAX_ATT];
};

struct onvif_ptz_nodes {
	int size;
	char token[MAX_ATT][TOKEN_LEN];
};

struct onvif_property {
	int ch;
	int auth;
	const char* endpoint;
	const char* endpoint2;
	const char* user;
	const char* pass;
};

struct onvif_audio_sources {
	int size;
	char token[MAX_ATT][TOKEN_LEN];
};

struct onvif_motion_info {
	int type;
	char token[TOKEN_LEN];
};

typedef struct __nf_local_resolution
{
	int width;
	int height;
	int pixel;
} _nf_local_resolution;
typedef struct __nf_local_resolution_ratio
{
	int width_ratio;
	int height_ratio;
} _nf_local_resolution_ratio;

extern int _get_fps_num(NF_IPCAM_FPS_E fps);
extern gint _ipcam_convert_fps (gint fps);
void nmf_mrtp_pipe_set_dev_mac(GobjMrtpPipe *h_mrtp_pipe, gint ch, gchar* mac);
extern int nf_ipcam_is_vendor(const char* vendor);
extern GobjMrtpPipe* nf_live_get_mrtp_pipe_handle(void);
extern int set_onvif_onthefly_db(const char* vendor, const char* model, const char* swver, int resolution, int fps, int bitrate);
extern int get_onvif_onthefly_db(const char* vendor, const char* model, const char* swver, int* resolution, int* fps, int* bitrate);
extern int  nf_onvif_get_onthefly_cap(int ch);
extern int SOAP_SSL_CLIENT_CONTEXT(struct soap* soap, unsigned short a, const char* b, const char* c, const char* d, const char* e, const char* f);

ONVIF_MSG _nf_onvif_media_create_profile(ipcam_onvif_auth_info_t* auth_info, const char* endpoint,char* name, char* token);
ONVIF_MSG _nf_onvif_media_delete_profile(ipcam_onvif_auth_info_t* auth_info,const char* endpoint,char* name);
ONVIF_MSG _nf_onvif_media_add_video_source_configuration(ipcam_onvif_auth_info_t* auth_info, const char *endpoint,const char *profile_token,const char* vsc_token);
ONVIF_MSG _nf_onvif_media_add_video_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, const char* endpoint,	const char* profile_token,const char* vec_token);
ONVIF_MSG _nf_onvif_media_get_video_encoder_configuration_options2(ipcam_onvif_auth_info_t* auth_info, const char*, const char*, const int, _nf_local_resolution*, int* ,video_t*, encoder_t*, values*);
ONVIF_MSG _nf_onvif_media_get_video_encoder_configurations(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* profile_token,	int stream_cnt,	char (*vec_token)[64]);

ONVIF_MSG _nf_onvif_media_get_video_source_configuration_options(ipcam_onvif_auth_info_t* auth_info, const char *,const char* ,const char* ,image_t_onvif *);
ONVIF_MSG _nf_onvif_media_get_video_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, int stream_no, video_t* video, encoder_t* encoder);
ONVIF_MSG _nf_onvif_media_set_video_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, const video_encoder_onvif info, const values govLength);
ONVIF_MSG _nf_onvif_add_audio_source_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *asrc_token);
ONVIF_MSG _nf_onvif_get_audio_source_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, struct onvif_audio_sources *asc);
ONVIF_MSG _nf_onvif_get_audio_encoder_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property * property, struct onvif_audio_encoders *aecs);
ONVIF_MSG _nf_onvif_add_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *aec_token);
ONVIF_MSG _nf_onvif_set_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *aec_token);
ONVIF_MSG _nf_onvif_media_get_stream_uri(ipcam_onvif_auth_info_t*, const char* endpoint, const char* profile_token, char* uri);

ONVIF_MSG _nf_onvif_media_get_metadata_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property* property, char* meta_token);
ONVIF_MSG _nf_onvif_media_add_metadata_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property* property, const char* profile_token, const char* meta_token);
ONVIF_MSG _nf_onvif_media_get_video_analytics_configurations(ipcam_onvif_auth_info_t* auth_info, 
		struct onvif_property *property, struct onvif_motion_info *omi);
ONVIF_MSG _nf_onvif_media_add_ptz_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property* property, char* profile_token, char* ptz_token);
ONVIF_MSG _nf_onvif_ptz_get_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property* property, struct onvif_ptz_nodes *ptz_nodes);

ONVIF_MSG _nf_onvif_get_h264_profiles(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_profiles *profiles);
ONVIF_MSG _nf_onvif_get_profiles(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_profiles *profiles);
ONVIF_MSG _nf_onvif_get_video_encoder_configurations(ipcam_onvif_auth_info_t* auth_info,	struct onvif_property *property, struct onvif_video_encoders *video_encoders);
ONVIF_MSG _nf_onvif_get_video_encoder_options(ipcam_onvif_auth_info_t* auth_info,	 struct onvif_property *property,
		char *token, struct onvif_video_encoder_option *option);
ONVIF_MSG _nf_onvif_set_video_encoder_h264(ipcam_onvif_auth_info_t* auth_info,
	struct onvif_property *property, struct onvif_video_encoder *info, struct onvif_video_encoder_option *option);

ONVIF_MSG _nf_onvif_get_audio_output_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_audio_outputs* audio_out);
ONVIF_MSG _nf_onvif_get_audio_decoder_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_audio_decoders* audio_dec);

ONVIF_MSG _nf_onvif_add_audio_decoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property,  char *profile_token, char *configuration_token);
ONVIF_MSG _nf_onvif_add_audio_output_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *configuration_token);

ONVIF_MSG _nf_onvif_get_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, 
		struct onvif_property *property, char *token, struct onvif_audio_encoder *audio_enc);

ONVIF_MSG _nf_onvif_media_set_metadata_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char* meta_token);
ONVIF_MSG _nf_onvif_h264_codec_change(
		ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, struct onvif_video_encoder_option*);
ONVIF_MSG _nf_onvif_profile_check(ipcam_onvif_auth_info_t* auth_info, struct onvif_property* property, struct onvif_profiles* profiles);
static int _nf_onvif_search_res(_nf_local_resolution* resol, const int size);

/**
 * @brief 해상도 비교 함수.(qsort에서 사용)
 * @param a
 * @param b
 * @return b - a
 *
 * 1. 640 x 360을 기타 640 해상도보다 먼저 선택.
 */
int _nf_onvif_compare_res(const void* a, const void* b)
{
	// 내림차순 정렬(left big = -1, right big, = 1)
	// 오름차순 정렬(left big = 1, right big = -1)
	const int left_big = -1;
	const int right_big = 1;

	const int equal = 0;
	int rtn = 0;
	_nf_local_resolution* left = (_nf_local_resolution*)a;
	_nf_local_resolution* right = (_nf_local_resolution*)b;

	if(left->width > right->width)
	{
		if(left->height > right->height)
		{
			// Left Big
			rtn = left_big;
		}
		else if(left->height < right->height)
		{
			if(right->pixel > left->pixel)
			{
				// Left Big
				rtn = left_big;
			}
			else if(right->pixel < left->pixel)
			{
				// Right BIg
				rtn = right_big;
			}
			else
			{
				// Left Big
				rtn = left_big;
			}
		}
		else
		{
			// Left Big
			rtn = left_big;
		}
	}
	else if(left->width < right->width)
	{
		if(left->height < right->height)
		{
			// Right Big
			rtn = right_big;
		}
		else if(left->height > right->height)
		{
			if(left->pixel < right->pixel)
			{
				// Right Big
				rtn = right_big;
			}
			else if(left->pixel > right->pixel)
			{
				// Left Big
				rtn = left_big;
			}
			else 
			{
				// Right Big
				rtn = right_big;
			}
		}
		else
		{
			// Right Big
			rtn = right_big;
		}
	}
	else if(left->width == right->width)
	{
		if(left->height < right->height)
		{
			// Right Big
			rtn = right_big;
		}
		else if(left->height > right->height)
		{
			// Left Big
			rtn = left_big;
		}
		else
		{
			rtn = equal;
		}
	}

	if(left->width == 640 && right->width == 640)
	{
		if(left->height == 360)
			rtn = left_big;
		else if(right->height == 360)
			rtn = right_big;
	}

	return rtn;
}

static int _nf_onvif_set_res(int ch, _nf_local_resolution* resol, const int size, int stream_no);
static void _get_width_height(const uint64_t resol, int* w, int* h);
static int _get_stream_ratio(_nf_local_resolution_ratio *resol_ratio, int w, int h);



/* -------------   ONVIF related APIs for the IPX scenario   --------------- */


/**
 * @brief OPEN모드에서 미리보기 profile의 rtsp url을 조회한다.
 * @param[in] xaddr_media ONVIF Media xaddr.
 * @param[in] auth 인증방식.
 * @param[in] u 사용자 ID.
 * @param[in] p 사용자 password.
 * @param[in] token profile token.
 * @param[out] rtn_uri 대상 rtsp URL.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_get_preview_uri(char *xaddr_media, char *xaddr_device, int auth, char* u, char* p, char* token, char* rtn_uri)
{
	int i = 0;
	int rtn = (-1);

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START\n");

	auth_info.auth_method = auth;
	auth_info.username = u;
	auth_info.password = p;
	auth_info.endpoint = xaddr_device;
	rtn = _nf_onvif_media_get_stream_uri(&auth_info, xaddr_media, token, rtn_uri);

ends_label:
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

/**
 * @brief OPEN모드에서 미리보기에 적합한 H264 profile token을 조회한다.
 * @param[in] xaddr_media ONVIF Media xaddr.
 * @param[in] auth 인증방식.
 * @param[in] u 사용자 ID.
 * @param[in] p 사용자 password.
 * @param[out] rtn_token 대상 profile token
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_get_preview_profile(char *xaddr_media, ipcam_onvif_auth_info_t *auth_info, char* rtn_token)
{
	int i = 0;
	int rtn = (-1);
	struct soap *soap;
	struct _media__GetProfiles req;
	struct _media__GetProfilesResponse res;
	struct tt__Profile *prof;
	char *u = auth_info->username;


	NF_ONVIF_DBG(MAJOR, "START(%s, %s:*)\n", xaddr_media, u);

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(xaddr_media, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _media__GetProfiles));
	memset(&res, 0x00, sizeof(struct _media__GetProfilesResponse));
	rtn = soap_call___media__GetProfiles
			(soap, xaddr_media, NULL, &req, &res);
	if (rtn != 0)
	{
		char buf[1024];
		NF_ONVIF_DBG(WARN, "profile get fail\n");
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
		goto ends_label;
	}

	if (res.__sizeProfiles == 0 || res.Profiles == NULL)
	{
		NF_ONVIF_DBG(WARN, "profile get fail\n");
		rtn = -1;
		goto ends_label;
	}


	for (i = 0; i < res.__sizeProfiles; i++)
	{
		prof = &res.Profiles[i];
		if (prof->token == NULL)
		{
			continue;
		}
		if (prof->VideoEncoderConfiguration == NULL)
		{
			continue;
		}
		if (prof->VideoEncoderConfiguration->Encoding != tt__VideoEncoding__H264)
		{
			continue;
		}
		if (prof->VideoEncoderConfiguration->H264 == NULL)
		{
			continue;
		}

		if(prof->token != NULL && strlen(prof->token) > 0)
		{
			snprintf(rtn_token, 64, "%s", prof->token);
			break;
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}
 
/**
 * @brief ONVIF 카메라의 profile을 조회하고 적정 token들을 가져온다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_get_appropriate_profile(int ch)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = NULL;
	onvif_t *onvif = NULL;
	int auth;
	char *user, *pass;
	int stream_no = 0;

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	onvif = &runtime[ch].onvif;
	g_return_val_if_fail(onvif != NULL, (-1));

	int profile_size = 0;
	struct onvif_profiles profiles;
	struct onvif_profile *pro;
	struct onvif_ptz_nodes nodes;
	struct onvif_property property;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START\n");


	auth = runtime[ch].onvif.auth_method;
	user = runtime[ch].username;
	pass = runtime[ch].password;

	memset(&property,0x00, sizeof(property));

	property.ch = ch;
	property.auth = auth;
	property.user = user;
	property.pass = pass;
	property.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];
	property.endpoint2 = NULL;

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	if(strlen(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]) > 0)
	{
		property.endpoint2 = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ];
	}

	// Check Video Encoder
	// VideoEncoder 조회후 H264 Encoder가 2개 이하이면 H264로 코덱 변경.(Grundig)
	{
		struct onvif_video_encoders venc;
		struct onvif_video_encoder_option option;

		memset(&venc, 0x00, sizeof(venc));
		memset(&option, 0x00, sizeof(option));
		_nf_onvif_get_video_encoder_configurations(&auth_info, &property, &venc);

		int i;
		int h264_count = 0;

		if(venc.size > 0) {
			for(i = 0; i < venc.size; i++) {
				if(venc.video_encoder[i].encoding == 2) {
					h264_count++;
				}
			}
		}

		if(h264_count < 2) {
			for(i = 0; i < venc.size; i++) {
				if(venc.video_encoder[i].encoding != 2) {
					_nf_onvif_get_video_encoder_options(&auth_info, &property, venc.video_encoder[i].token, &option);
					if(option.h264_supported == 1) {
						_nf_onvif_set_video_encoder_h264(&auth_info, &property, &venc.video_encoder[i], &option);
					}
				}
			}
		}
	}

	memset(&profiles, 0x00, sizeof(profiles));
	rtn = _nf_onvif_get_profiles(&auth_info, &property, &profiles);
	if(rtn != 0 )
	{
		NF_ONVIF_DBG(WARN, "Profile Get Failed(rtn:%d)\n",rtn);
		rtn = -1;
		nf_eventlog_put_ipcam_msg("Get Profiles Fail", ch);
		goto ends_label;
	}
	
	// axis exception code
	if((strcmp(runtime[ch].sys.vendor, "AXIS") == 0)
			&& (strcmp(runtime[ch].sys.model, "AXIS M3007") != 0))
	{
		struct onvif_video_encoders venc;
		struct onvif_video_encoder_option option;

		memset(&venc, 0x00, sizeof(venc));
		memset(&option, 0x00, sizeof(option));
		
		if(profiles.size > 1)
		{
			_nf_onvif_get_video_encoder_configurations(&auth_info, &property, &venc);

			for(i = 0; i < venc.size; i++)
			{
				//test code
				if(strstr(venc.video_encoder[i].token, profiles.profile[1].vec_token) != NULL)
				{
					_nf_onvif_get_video_encoder_options(&auth_info, &property, venc.video_encoder[i].token, &option);

					if(option.h264_supported == 1) 
					{
						char tmp_endpoint[64];
						strncpy(tmp_endpoint, property.endpoint, 64);
						
						rtn = _nf_onvif_h264_codec_change(
								&auth_info,
								tmp_endpoint,
								venc.video_encoder[i].token,
								&option);
					}
				}
			}
		}
	}

	memset(&profiles, 0x00, sizeof(profiles));
	rtn = _nf_onvif_get_h264_profiles(&auth_info, &property, &profiles);
	if(rtn != 0 )
	{
		NF_ONVIF_DBG(WARN, "Profile Get Failed(rtn:%d)\n",rtn);
		rtn = -1;
		goto ends_label;
	}

	// 카메라에서 지원하는 기능을 Profile에 추가한다.
	rtn = _nf_onvif_profile_check(&auth_info, &property, &profiles);
	NF_ONVIF_DBG(MINOR, "Profile Check (rtn:%d)\n",rtn);


	// Search Profiles
	memset(onvif->profile_token, 0x00, 64 * MAX_VIDEO_STREAM);
	memset(onvif->vsc_token, 0x00, 64 * MAX_VIDEO_STREAM);
	memset(onvif->vec_token, 0x00, 64 * MAX_VIDEO_STREAM);

	memset(&profiles, 0x00, sizeof(profiles));

	// H264 코덱을 지원하는 Profile을 조회한다.
	rtn = _nf_onvif_get_h264_profiles(&auth_info, &property, &profiles);
		
	//sony camera exception code 
	if(profiles.size < 2)
	{
		if(strncasecmp(runtime[ch].sys.vendor, "Sony", 64) == 0)
		{
			int ret = 0;

			//sony camera http api send
			ret = sony_init_2nd_profiles(ch);

			if(ret == 0)
				printf("[%s:%d] Sony 2nd profile get start\n", __FUNCTION__, __LINE__);

			sleep(5);

			memset(&profiles, 0x00, sizeof(profiles));
			// H264 코덱을 지원하는 Profile을 조회한다.
			rtn = _nf_onvif_get_h264_profiles(&auth_info, &property, &profiles);
		}
	}

	if(rtn != 0 || profiles.size < 1)
	{
		NF_ONVIF_DBG(WARN, "Profile Get Failed(rtn:%d|profiles.size:%d)\n",
				rtn, profiles.size);
		rtn = -1;
		nf_eventlog_put_ipcam_msg("Get Profiles Fail", ch);
		goto ends_label;
	}

	stream_no = 0;
	profile_size = profiles.size;

	for(i = 0; i< profile_size ; i++)
	{
		pro = &profiles.profile[i];
		if(strlen(pro->token) > 0)
		{
			if(strstr(pro->token, "mobile"))
			{
				continue;
			}
			strcpy(onvif->profile_token[stream_no], pro->token);
		}
		else
		{
			continue;
		}

		if((strcmp(runtime[ch].sys.vendor, "AXIS") == 0)
				&& (strcmp(runtime[ch].sys.model, "AXIS M3007") == 0))
		{
			if(strcmp(pro->token, "profile_3_h264") <0 || stream_no  != 0)
			{
				continue;
		}
			else
			{
				if(stream_no == 0)
				{
					// M3007 Metadata Exception Code (profile_3_h264 & meta token : 0 resetting)
					char meta_token[TOKEN_LEN];
					memset(meta_token, 0x00, TOKEN_LEN);
					rtn = _nf_onvif_media_get_metadata_configurations(&auth_info, &property, meta_token);
					if(rtn == 0)
					{
						rtn = _nf_onvif_media_add_metadata_configuration(&auth_info, &property, pro->token, meta_token);
						rtn = _nf_onvif_media_set_metadata_configuration(&auth_info, &property, meta_token);
					}
					NF_ONVIF_DBG(MINOR, "M3007 : Retry Profile Check (rtn:%d)\n",rtn);
				}
			}
		}
		if(strcmp(runtime[ch].sys.vendor, "IQeye by Vicon") == 0)
		{
			if(strcmp(pro->token, "stream2_4_3") == 0 || strcmp(pro->token, "stream1_4_3") == 0)
			{
				NF_ONVIF_DBG(WARN, "Pass IQinVision ProfileToken(%s)\n", pro->token);
				continue;
			}
		}


		if(strlen(pro->vsc_token) <= 0 || strlen(pro->vec_token) <= 0)
		{
			continue;
		}

		if(strlen(pro->vec_token) > 0)
		{
			if(stream_no == 0)
			{
				strcpy(onvif->vec_token[stream_no], pro->vec_token);
			}
			else
			{
				int j;
				int eq = 0;
				for(j = 0; j < stream_no; j++)
				{
					if(strcmp(onvif->vec_token[j], pro->vec_token) == 0)
					{
						// Profile 중에 같은 Encoder를 사용하는 Profile은 제외
						eq = 1;
						break;
					}
				}

				if(eq == 1)
				{
					continue;
				}

				strcpy(onvif->vec_token[stream_no], pro->vec_token);
			}
			runtime[ch].onvif.vec_cur_quality[i] = pro->vec_quality;
		}

		if(strlen(pro->vsc_token) > 0)
		{
			strcpy(onvif->vsc_token[stream_no], pro->vsc_token);

			if(stream_no == 0)
				strcpy(onvif->vs_token, pro->vs_token);
		}


		if(strlen(pro->aec_token) > 0 && strlen(pro->asc_token) > 0)
		{
			if(strncasecmp(runtime[ch].sys.vendor, "Hikvision", 64) != 0
					&& strncmp(runtime[ch].sys.vendor, "ONVIF", 64) != 0 )	//hikvision audio issue
			{
			if(stream_no == 0)
			{
				struct onvif_audio_encoder audio_enc;
				memset(&audio_enc, 0x00, sizeof(audio_enc));
				rtn = _nf_onvif_get_audio_encoder_configuration(&auth_info, &property, pro->aec_token, &audio_enc);

				if(rtn == 0 && audio_enc.encoding == 0) //G711 = 0, G726 = 1, AAC = 2
				{
					if((audio_enc.samplerate / 1000) == 8 || (audio_enc.samplerate % 1000) == 8)
					{
						onvif->audio_enable = 1;
						onvif->audio_bitrate = audio_enc.bitrate;
						onvif->audio_samplerate =audio_enc.samplerate;
						runtime[ch].audio.audio_tx = 1;
						runtime[ch].audio.acodec.support = NF_IPCAM_ACODEC_G711_ULAW;
						runtime[ch].audio.acodec.value = NF_IPCAM_ACODEC_G711_ULAW;
						runtime[ch].audio.mic_volume.min = 0;
						runtime[ch].audio.mic_volume.max = 6;
						runtime[ch].audio.mic_volume.value = 3 ;
						runtime[ch].func |= NF_IPCAM_FUNC_AUDIO_TX;
					}
				}
			}
		}
		}

		if(strlen(pro->adc_token) > 0 && strlen(pro->aoc_token) > 0)
		{
#if 0
			if(stream_no == 0)
			{
				runtime[ch].audio.audio_rx = 1;
				runtime[ch].audio.speaker_volume.min = 0;
				runtime[ch].audio.speaker_volume.max = 6;
				runtime[ch].audio.speaker_volume.value = 3;
				runtime[ch].func |= NF_IPCAM_FUNC_AUDIO_RX;
			}
#endif
		}

		if(strlen(pro->ptz_token) > 0)
		{
			strcpy(onvif->ptz_token, pro->ptz_token);
		}

		if(strlen(pro->vac_token) > 0)
		{
			struct onvif_motion_info omi;
			int tmp_rtn = 0;

			memset(&omi, 0x00, sizeof(omi));
			tmp_rtn = _nf_onvif_media_get_video_analytics_configurations(&auth_info, &property, &omi);

			if(tmp_rtn == 0 && omi.type != 0 )
			{
				onvif->vac_type = omi.type;
				strcpy(onvif->vac_token, pro->vac_token);
			}
		}
		else
		{
			// Vendor 예외처리
			if(nf_ipcam_is_vendor("VICON") && strcmp(runtime[ch].sys.vendor, "VICON") == 0)
			{
				runtime[ch].onvif.vac_type = OMT_REGION;
				snprintf(onvif->vac_token, 64, "%s","1");
			}
		}

		stream_no++;

		if(MAX_VIDEO_STREAM <= stream_no)
			break;
	}

#if ((SUPPORT_3RD_STREAM) == 0)
	if(stream_no > 2)
		stream_no = 2;
#else
	if(stram_no > 2)
		stream_no = 3;
#endif

	runtime[ch].video.stream_cnt = stream_no;

ends_label:
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

/**
 * @brief ONVIF Media에서 비디오 설정 옵션들을 조회한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, -2 - 접속불가
 */
ONVIF_API nf_onvif_get_video_capability(int ch)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = NULL;
	onvif_t *onvif = NULL;
	int auth;
	char *user, *pass;
	//int stream_no = 0;

	ipcam_onvif_auth_info_t auth_info;

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	onvif = &runtime[ch].onvif;
	g_return_val_if_fail(onvif != NULL, (-1));

	NF_ONVIF_DBG(MAJOR, "START\n");

	int size_res = 0;
	_nf_local_resolution resol[100];	/* temp */
	memset(resol, 0x00, sizeof(_nf_local_resolution) * 100);
	memset(&runtime[ch].video.resolution, 0x00, sizeof(resolution_t));

	values govLength;
	memset(&govLength, 0x00, sizeof(values));

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	for(i = 0; i < runtime[ch].video.stream_cnt; i++)
	{
		if(strlen(runtime[ch].onvif.profile_token[i]) == 0 || runtime[ch].onvif.profile_token[i] == NULL)
		{
			continue;
		}
		
		// set video&encoder capability //not modify
		rtn = _nf_onvif_media_get_video_encoder_configuration_options2(
			&auth_info,
			runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
			runtime[ch].onvif.vec_token[i],
			i,
			resol,
			&size_res, 
			&runtime[ch].video,
			&runtime[ch].encoder, 
			&govLength);


		if(rtn != 0 || size_res == 0)
		{
			nf_eventlog_put_ipcam_msg("Not Found Resolution", ch);
			return IPCAM_SETUP_RTN_FAILED;
			//continue;
		}

		//resolution sort by descending order (encoder options : resolution)
		{
			int size_sort[size_res];
			memset(size_sort, 0x00, sizeof(int)*size_res);
			
			int j = 0;
			int k = 0;

			_nf_local_resolution temp_resol;	
			memset(&temp_resol, 0x00, sizeof(_nf_local_resolution)*1);

			for(j = 0; j < size_res; j++)
			{
				size_sort[j] = resol[j].width * resol[j].height;
				//printf("\e[95m >>pixel : %d  width : %d height : %d \e[0m\n", 
				//		size_sort[j],resol[j].width, resol[j].height);
			}

			for(j = 0; j < size_res; j++)
			{
				for(k = j+1; k < size_res; k++)
				{
					if(size_sort[j] < size_sort[k])
					{
						temp_resol.width = resol[j].width;
						temp_resol.height =  resol[j].height;
						temp_resol.pixel = resol[j].pixel;

						resol[j].width = resol[k].width;
						resol[j].height = resol[k].height;
						resol[j].pixel = resol[k].pixel;

						resol[k].width = temp_resol.width;
						resol[k].height = temp_resol.height;
						resol[k].pixel = temp_resol.pixel;
						//memset(&temp_resol, 0x00, sizeof(_nf_local_resolution)*1);
					}
				}
			}
#if 0
			printf("\e[31m >> sorting END \e[0m\n");
			
			for(j = 0; j < size_res; j++)
			{
				printf("\e[95m >>pixel : %d  width : %d height : %d \e[0m\n", 
						size_sort[j],resol[j].width, resol[j].height);
			}
#endif
		}

		// set video.resolution
		rtn = _nf_onvif_set_res(ch, resol, size_res, i);
		
		// cannot connect this camera..(cannot find suitable resolution)
		if(rtn == 0 && i == 0)
		{
			return -2;
		}
		
		// decrease stream_cnt (cannot find suitable 2nd resolultion)
		if(rtn == 0 && i == 1)
		{
			runtime[ch].video.stream_cnt--;
			continue;
		}
	
		// get video curr value
		rtn = _nf_onvif_media_get_video_encoder_configuration(
				&auth_info, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
				runtime[ch].onvif.vec_token[i], i, &runtime[ch].video, &runtime[ch].encoder
				);

		// get rtsp url
		rtn = _nf_onvif_media_get_stream_uri(
				&auth_info,
				runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
				runtime[ch].onvif.profile_token[i],
				runtime[ch].sys.rtsp_url[i]
				);

		if(rtn != 0)
		{
			// todo reset i-th resolution
			//runtime[ch].sys.rtsp_port[i] = 554;
			//continue;

			// GetStreamUri fail
			nf_eventlog_put_ipcam_msg("Not Found stream URI", ch);
			return -2;
		}

		char tempurl[256];
		snprintf(tempurl, 256, "%s", runtime[ch].sys.rtsp_url[i]);


		if(strstr(tempurl + 6, ":") == NULL)
		{
			runtime[ch].sys.rtsp_port[i] = 554;
		}
		else
		{
			char *tempstart = strstr(tempurl + 6, ":");
			unsigned short port = (unsigned  short)strtoul(tempstart + 1, NULL, 0);
			//if(port == 554)
			{
				runtime[ch].sys.rtsp_port[i] = port;
			}
		}
	}

	// DAYOU -> onthefly 기능 확인 (SWPFOURE-555)
	if(nf_ipcam_is_vendor_dayou())
	{
		nf_onvif_get_onthefly_cap(ch);
	}
	else
	{
		runtime[ch].video.supported |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
		runtime[ch].video.onthefly |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
		runtime[ch].onvif.query_onthefly_state = 0;
		//nf_onvif_get_onthefly_cap(ch);
	}
													
	// ELSE -> 모든 카메라는 onthefly가 되어있다고 가정

	//all support onthefly 
	//runtime[ch].video.supported |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
	//runtime[ch].video.onthefly |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
	//runtime[ch].onvif.query_onthefly_state = 0;

ends_label:
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

/**
 * @brief ONVIF카메라가 'onthefly'로 fps, bitrate 조절가능한지 테스트한다.
 * @param ch 채널번호.
 * @return 항상 0.
 */
ONVIF_API nf_onvif_get_onthefly_cap(int ch)
{
	int rtn = 0, loop = 0, tmp, tmp1, tmp2;
	int resolution = 0, fps = 1, bitrate = 1;
	mtable* runtime = get_runtime();
	int befo_stream_cnt = runtime[ch].video.stream_cnt;

	GobjMrtpPipe* h_iplive = nf_live_get_mrtp_pipe_handle();

	runtime[ch].video.stream_cnt = 1;
	{
		/* 1. open some video stream */
		NMFMrtpPipeChannel info;

		memset(&info, 0x00, sizeof(NMFMrtpPipeChannel));

		info.ch_num = ch;
		//info.model_code = NF_IPCAM_MODEL_ONVIF;
		info.model_code = (guint) -1;
		info.username = runtime[ch].username;
		info.password = runtime[ch].password;
		info.video_cnt = 1;
		info.video[0].resolution = RES_1920x1080;
		info.video[0].ip_addr = runtime[ch].sys.ipaddr;
		info.video[0].rtsp_port = runtime[ch].sys.rtsp_port[0];
		info.video[0].rtsp_addr = runtime[ch].sys.rtsp_url[0];
		nmf_mrtp_pipe_set_dev_mac(h_iplive, ch,
				&runtime[ch].sys.macaddr[0]);

		nmf_mrtp_pipe_open_ch(h_iplive, &info);

		/* 2. wait until 15 sec or disconnect */
		while(loop < 15)
		{
			if(runtime[ch].onvif.query_onthefly_state == 2)
			{	/* if disconnected, onthefly test fail!  */
				fps = 0;
				bitrate = 0;
				break;
			}
			usleep(1000 * 1000);
			loop++;
		}

		nmf_mrtp_pipe_close_ch(h_iplive, ch);

		if(fps == 1 && bitrate == 1)
		{
			NF_ONVIF_DBG(MINOR, "ch(%d) support OntheFly!!!\n",ch);
		}
		else
		{
			NF_ONVIF_DBG(MINOR, "ch(%d) unsupport OntheFly!!!\n",ch);
			runtime[ch].video.onthefly &= ~VIDEO_SETUP_FPS;
			runtime[ch].video.onthefly &= ~VIDEO_SETUP_BITRATE;
			runtime[ch].video.supported &= ~VIDEO_SETUP_FPS;
			runtime[ch].video.supported &= ~VIDEO_SETUP_BITRATE;
			runtime[ch].onvif.query_onthefly_state = 2;

			return 0;
		}

		/* 3. write result to onthefly db */
		if(get_onvif_onthefly_db(runtime[ch].sys.vendor,
				runtime[ch].sys.model,
				runtime[ch].sys.swver, &tmp, &tmp1, &tmp2) == 0)
		{
			rtn = set_onvif_onthefly_db(runtime[ch].sys.vendor,
				runtime[ch].sys.model,
				runtime[ch].sys.swver, resolution, fps, bitrate);
		}
		if(runtime[ch].onvif.query_onthefly_state == 1)
		{
			runtime[ch].video.supported |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
			runtime[ch].video.onthefly |= (VIDEO_SETUP_FPS | VIDEO_SETUP_BITRATE);
		}
		runtime[ch].onvif.query_onthefly_state = 0;
	}

	runtime[ch].video.stream_cnt = befo_stream_cnt;

	return 0;
}

/**
 * @brief ONVIF 카메라의 비디오 스트림을 설정한다.
 * @param ch 채널 번호.
 * @param stream_info 비디오 스트림 정보 struct.
 * @return 항상 IPCAM_SETUP_RTN_DONE.
 */
ONVIF_API nf_onvif_set_stream(int ch, cam_info* stream_info)
{
	int rtn = 0;
	mtable *runtime = get_runtime();
	if(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_MEDIA2))
	{
		rtn = nf_onvif_set_stream_media2(ch, stream_info);
	}
	else
	{
		rtn = nf_onvif_set_stream_media(ch, stream_info);
	}

	return rtn;
}

ONVIF_API nf_onvif_set_stream_media(int ch, cam_info* stream_info)
{
	int i, rtn;
	video_encoder_onvif info;
	mtable *runtime = get_runtime();
	values govGarbage;

	memset(&govGarbage, 0x00, sizeof(govGarbage));

	ipcam_onvif_auth_info_t auth_info;

	for(i = 0 ; i < runtime[ch].video.stream_cnt; i++)
	{
		memset(&info, 0x00, sizeof(video_encoder_onvif));

		_get_width_height((uint64_t)stream_info->vcodec.resolution[i], &info.width, &info.height);
		info.bitratelimit = (int) stream_info->vcodec.bitrate[i];
		info.frameratelimit = _get_fps_num(stream_info->vcodec.fps[i]);
		govGarbage.min = info.frameratelimit;

		// QV exception (SWIPXXP-554)
		if(strncasecmp(runtime[ch].sys.vendor, "Qualvision", 64) == 0)
		{
			printf("[%s] ch(%d) Qualvision cam set gop size(10) \e[0m\n", __func__, ch);
			info.govlength = 10;
			govGarbage.min = 10;
		}

		auth_info.auth_method = runtime[ch].onvif.auth_method;
		auth_info.username = runtime[ch].username;
		auth_info.password = runtime[ch].password;
		auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

		rtn = _nf_onvif_media_set_video_encoder_configuration(
				&auth_info,
				runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
				runtime[ch].onvif.vec_token[i],
				info,
				govGarbage	// set gov 2012-09-17
				);
	}

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_VCODEC, -1);


	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 1st stream만 존재하는 ONVIF카메라에 대해 임의이 2nd stream을 추가한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 *
 * @deprecated Techwin카메라 등 일부 카메라에만 적용가능하여 사용금지.
 */
ONVIF_API nf_onvif_profile_factory(int ch)
{
	int i, rtn;
	mtable *runtime = get_runtime();

	ipcam_onvif_auth_info_t auth_info;
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];


	NF_ONVIF_DBG(MAJOR, "ch %d  START\n", ch);

	// TODO start with 0 profile
	// assume one profile exists
	i = 1;
	char profile_name[64];
	snprintf(profile_name, 64, "iprofile_%02d", i + 1);
	// 0. optional - delete profile
	rtn = _nf_onvif_media_delete_profile(
				&auth_info,
				runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
				profile_name
				);
	if(rtn != 0)
	{
		NF_ONVIF_DBG(MINOR, "ch %d profile delete fail, continue...!\n", ch);
	}
	else
	{
		NF_ONVIF_DBG(MINOR, "ch %d profile delete success!\n", ch);
	}

	// 1. create profile
	// TODO use exist profile
	rtn = _nf_onvif_media_create_profile(
			&auth_info,
			runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
			profile_name,
			runtime[ch].onvif.profile_token[i]
			);

	if(rtn != 0)
	{
		return rtn;
	}

	// 2. get capable video encoder configuration
	// TODO test other cams
	rtn = _nf_onvif_media_get_video_encoder_configurations(
			&auth_info,
			runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
			runtime[ch].onvif.profile_token[i],
			i,
			runtime[ch].onvif.vec_token
			);

	if(rtn != 0)
	{
		return rtn;
	}

	// 3. add video source cfg
	if(runtime[ch].onvif.vsc_token[0])
	{
		rtn = _nf_onvif_media_add_video_source_configuration(
				&auth_info,
				runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
				runtime[ch].onvif.profile_token[i],
				runtime[ch].onvif.vsc_token[0]
				);
	}
	else
	{
		// TODO get video source cfg
	}

	if(rtn != 0)
	{
		return rtn;
	}

	// 4. add video enc cfg
	rtn = _nf_onvif_media_add_video_encoder_configuration(
			&auth_info,
			runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA],
			runtime[ch].onvif.profile_token[i],
			runtime[ch].onvif.vec_token[i]
			);

	if(rtn != 0)
	{
		return rtn;
	}

	NF_ONVIF_DBG(MAJOR, "ch %d END\n", ch);

	return rtn;
}

ONVIF_API nf_onvif_get_video_analytics_configurations(int ch)
{
	int rtn;
	mtable *runtime;

	runtime = get_runtime();

	ipcam_onvif_auth_info_t auth_info;

	struct onvif_property property;
	struct onvif_motion_info omi;

	memset(&property, 0x00, sizeof(property));

	property.ch = ch;
	property.user = runtime[ch].username;
	property.pass = runtime[ch].password;
	property.auth = runtime[ch].onvif.auth_method;
	property.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA];

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_media_get_video_analytics_configurations(&auth_info, &property, &omi);

	return rtn;
}




/* -------------   ONVIF MSG exchanging methods - Internal use   --------------- */


ONVIF_MSG _nf_onvif_media_create_profile(ipcam_onvif_auth_info_t* auth_info, const char* endpoint,char* name, char* token)
{
	int i = 0;
	int rtn = (-1);

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(name != NULL, (-1));
	g_return_val_if_fail(token != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__CreateProfile req;
	struct _media__CreateProfileResponse res;

	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
			goto ends_label;
		}
	}

	/* request setup */
	memset(&req, 0x00, sizeof(struct _media__CreateProfile));
	req.Name = soap_malloc(soap, strlen(name) + 1);
	strcpy(req.Name, name);

	/* soap call */
	rtn = soap_call___media__CreateProfile
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0 || res.Profile == NULL)
	{
		NF_ONVIF_DBG(WARN, "profile creation fail\n");

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
			goto ends_label;
		}

		goto ends_label;
	}

	strcpy(token, res.Profile->token);

	NF_ONVIF_DBG(MINOR, "name(%s) response(%s) token(%s)\n", name, 
			res.Profile->token,	token);


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


ONVIF_MSG _nf_onvif_media_delete_profile(ipcam_onvif_auth_info_t* auth_info,const char* endpoint,char* name)
{
	int i = 0;
	int rtn = (-1);
	char profile_token[256];

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(name != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__DeleteProfile req;
	struct _media__DeleteProfileResponse res;

	NF_ONVIF_DBG(MAJOR, "START\n");

	// get profile token from profile name
	{
		struct _media__GetProfiles _req;
		struct _media__GetProfilesResponse _res;

		memset(&_req, 0x00, sizeof(struct _media__GetProfiles));

		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

		if (rtn != 0)
		{
			NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
			goto ends_label;
		}

		if (strstr(endpoint, "https://") != NULL)
		{
			rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
					NULL, NULL, NULL, NULL, NULL);

			if (rtn != 0)
			{
				char buf[1024];
				soap_sprint_fault(soap, buf, 1024);
				NF_ONVIF_DBG(ERROR, "\n%s\n", buf);
				goto ends_label;
			}
		}

		rtn = soap_call___media__GetProfiles
				(soap, endpoint, NULL, &_req, &_res);

		if (rtn != 0 || _res.__sizeProfiles == 0 || _res.Profiles == NULL)
		{
			NF_ONVIF_DBG(WARN, "profile creation fail\n");

			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(ERROR, "\n%s\n", buf);

			goto ends_label;
		}

		for(i = 0; i < _res.__sizeProfiles; i++)
		{
			if(strcmp(_res.Profiles[i].Name, name) == 0)
			{
				break;
			}
		}
		if(i != _res.__sizeProfiles)
		{
			NF_ONVIF_DBG(MINOR, "found profile %s - %d\n", name, i);
			strcpy(profile_token, _res.Profiles[i].token);
		}
		else
		{
			NF_ONVIF_DBG(WARN, "profile %s not found, continue... \n", name);
			goto ends_label;
		}

	}

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	memset(&req, 0x00, sizeof(struct _media__DeleteProfile));
	req.ProfileToken= soap_malloc(soap, strlen(profile_token) + 1);
	strcpy(req.ProfileToken, profile_token);

	/* soap call */
	rtn = soap_call___media__DeleteProfile
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0 )
	{
		NF_ONVIF_DBG(ERROR, "profile delete fail\n");
		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


ONVIF_MSG _nf_onvif_media_add_video_source_configuration
(
	ipcam_onvif_auth_info_t* auth_info,
	const char *endpoint,
	const char *profile_token,
	const char* vsc_token
)
{
	int i = 0;
	int rtn = (-1);

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(profile_token != NULL, (-1));
	g_return_val_if_fail(vsc_token != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__AddVideoSourceConfiguration req;
	struct _media__AddVideoSourceConfigurationResponse res;


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	NF_ONVIF_DBG(MINOR, "endpoint(%s) profile(%s) source(%s)\n", endpoint,
			profile_token, vsc_token);

	/* request setup */
	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	req.ConfigurationToken = soap_malloc(soap, strlen(vsc_token) + 1);
	strcpy(req.ProfileToken, profile_token);
	strcpy(req.ConfigurationToken, vsc_token);

	/* soap call */
	rtn = soap_call___media__AddVideoSourceConfiguration
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video source configuration add fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_add_video_encoder_configuration
(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint,
	const char* profile_token,
	const char* vec_token
)
{
	int i = 0;
	int rtn = (-1);

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(profile_token != NULL, (-1));
	g_return_val_if_fail(vec_token != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__AddVideoEncoderConfiguration req;
	struct _media__AddVideoEncoderConfigurationResponse res;


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	NF_ONVIF_DBG(MINOR, "endpoint(%s) profile(%s) encoder(%s)\n", endpoint, 
			profile_token, vec_token);

	/* request setup */
	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	req.ConfigurationToken = soap_malloc(soap, strlen(vec_token) + 1);
	strcpy(req.ProfileToken, profile_token);
	strcpy(req.ConfigurationToken, vec_token);

	/* soap call */
	rtn = soap_call___media__AddVideoEncoderConfiguration
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration add fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_get_video_encoder_configurations
(
	ipcam_onvif_auth_info_t* auth_info,
	const char *endpoint,
	const char* profile_token,
	int stream_cnt,
	char (*vec_token)[64]
)
{
	int i, j = 0;
	int rtn = (-1);

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(vec_token != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	//struct _media__GetCompatibleVideoEncoderConfigurations req;
	//struct _media__GetCompatibleVideoEncoderConfigurationsResponse res;

	struct _media__GetVideoEncoderConfigurations req;
	struct _media__GetVideoEncoderConfigurationsResponse res;


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	NF_ONVIF_DBG(MINOR, "endpoint(%s) profile(%s) \n", endpoint, profile_token);


	/* request setup */
	//req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	//strncpy(req.ProfileToken, profile_token, strlen(profile_token));


	/* soap call */
	//rtn = soap_call___media__GetCompatibleVideoEncoderConfigurations
	//		(soap, endpoint, NULL, &req, &res);
	memset(&req, 0x00, sizeof(struct _media__GetVideoEncoderConfigurations));		

	rtn = soap_call___media__GetVideoEncoderConfigurations(soap, endpoint, NULL, 
			&req, &res);

	/* response handle */
	if (rtn != 0 || res.__sizeConfigurations == 0 || res.Configurations == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configurations get fail\n");
		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}

		goto ends_label;
	}

	// 0
	for(i = 0; i < res.__sizeConfigurations; i++)
	{
		NF_ONVIF_DBG(MINOR, "vec %d %s (usecount %d) found!!!\n",
				res.Configurations[i].Encoding, res.Configurations[i].token,
				res.Configurations[i].UseCount);
	}

	// 1. find unused h264 config - techwin scenario
	for(i = 0; i < res.__sizeConfigurations; i++)
	{
		if(res.Configurations[i].Encoding == tt__VideoEncoding__H264
		&& res.Configurations[i].UseCount == 0)
		{
			strncpy(vec_token[stream_cnt], res.Configurations[i].token, 64);
			NF_ONVIF_DBG(MINOR, "unused h264 vec %s found!!!\n", 
					vec_token[stream_cnt]);
			goto ends_label;
		}
	}

	// 2. find unused !h264 config
	for(i = 0; i < res.__sizeConfigurations; i++)
	{
		if(res.Configurations[i].Encoding != tt__VideoEncoding__H264
		&& res.Configurations[i].UseCount == 0)
		{
			NF_ONVIF_DBG(MINOR, "unused !h264 vec %s found!!! try to update...\n",
					res.Configurations[i].token	);

			video_encoder_onvif info;
			values govGarbage;
			memset(&govGarbage, 0x00, sizeof(govGarbage));

			memset(&info, 0x00, sizeof(video_encoder_onvif));

			if(res.Configurations[i].Resolution != NULL)
			{
				info.width = res.Configurations[i].Resolution->Width;
				info.height = res.Configurations[i].Resolution->Height;
			}
			else
			{
				info.width = 1920;
				info.height = 1080;
			}
			govGarbage.min = 9999;

			rtn = _nf_onvif_media_set_video_encoder_configuration(
					auth_info,
					endpoint,
					res.Configurations[i].token,
					info,
					govGarbage
					);

			if(rtn == 0)
			{
				strncpy(vec_token[stream_cnt], res.Configurations[i].token, 64);
				NF_ONVIF_DBG(MINOR, "unused !h264 vec %s update success!!!\n",
						vec_token[stream_cnt]);
				goto ends_label;
			}
		}
	}

	// 3. find not "chosen one"
	for(i = 0; i < res.__sizeConfigurations; i++)
	{
		for(j = 0; j < stream_cnt; j++)
		{
			if(strcmp(vec_token[j], res.Configurations[i].token) == 0)
				break;
		}
		if(j != stream_cnt) continue;

		{
			strncpy(vec_token[stream_cnt], res.Configurations[i].token, 64);
			NF_ONVIF_DBG(MINOR, "vec %s found!!! try to update...\n",
					vec_token[stream_cnt]);


			video_encoder_onvif info;
			values govGarbage;
			memset(&govGarbage, 0x00, sizeof(govGarbage));

			memset(&info, 0x00, sizeof(video_encoder_onvif));

			if(res.Configurations[i].Resolution != NULL)
			{
				info.width = res.Configurations[i].Resolution->Width;
				info.height = res.Configurations[i].Resolution->Height;
			}
			else
			{
				info.width = 1920;
				info.height = 1080;
			}
			govGarbage.min = 9999;

			rtn = _nf_onvif_media_set_video_encoder_configuration(
					auth_info,
					endpoint,
					res.Configurations[i].token,
					info,
					govGarbage
					);

			if(rtn == 0)
			{
				strncpy(vec_token[stream_cnt], res.Configurations[i].token, 64);
				NF_ONVIF_DBG(MINOR, "vec %s update success!!!\n",
						vec_token[stream_cnt]);
				goto ends_label;
			}
		}
	}

	if(rtn == 0)
	{
		rtn = -1;
		memset(vec_token, 0x00, 64);
		NF_ONVIF_DBG(MINOR, "capable vec not found!! \n");
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


ONVIF_MSG _nf_onvif_media_get_video_source_configuration_options
(
	ipcam_onvif_auth_info_t* auth_info,
	const char *endpoint,
	const char *profile_token,
	const char* vsc_token,
	image_t_onvif* image
)
{
	int i = 0;
	int rtn = (-1);

	g_return_val_if_fail(endpoint != NULL, (-1));
	g_return_val_if_fail(profile_token != NULL, (-1));
	g_return_val_if_fail(vsc_token != NULL, (-1));

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoSourceConfigurationOptions req;
	struct _media__GetVideoSourceConfigurationOptionsResponse res;


	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	NF_ONVIF_DBG(MINOR, "endpoint(%s) profile(%s) source(%s)\n", endpoint,
			profile_token, vsc_token);

	/* request setup */

	memset(&req, 0x00, sizeof(struct _media__GetVideoSourceConfigurationOptions));

	image->mirror.support = 0;
	image->mirror.value = 0;

	/* soap call */
	rtn = soap_call___media__GetVideoSourceConfigurationOptions
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video source configuration get fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

	if(res.Options == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video source configuration get fail\n");
		goto ends_label;
	}

	if(res.Options->Extension != NULL)
	{
		if(res.Options->Extension->Rotate != NULL)
		{
			NF_ONVIF_DBG(MINOR, "rotate mode size : %d\n", res.Options->Extension->Rotate->__sizeMode);
			if(res.Options->Extension->Rotate->DegreeList != NULL)
			{
				//image.mirror;
				for(i = 0; i < res.Options->Extension->Rotate->DegreeList->__sizeItems; i++)
				{
					NF_ONVIF_DBG(MINOR, "rotate degree %d  : %d\n", i, res.Options->Extension->Rotate->DegreeList->Items[i]);
					switch(res.Options->Extension->Rotate->DegreeList->Items[i])
					{
						case 90:
							break;
						case 180:
							break;
						case 270:
							break;
						case 360:
							break;
					}

				}
			}

		}
	}



ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	rtn = 0;// always success

	return rtn;
}

// get resolution, fps, gov
ONVIF_MSG _nf_onvif_media_get_video_encoder_configuration_options2(

	ipcam_onvif_auth_info_t* auth_info,	
	const char* endpoint,
	//const char* profile_token,
	const char* vec_token,
	const int stream_no,
	_nf_local_resolution* resol,
	int* size_res,
	video_t* video,
	encoder_t* encoder,
	values* govLength
)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = get_runtime();


	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoEncoderConfigurationOptions req;
	struct _media__GetVideoEncoderConfigurationOptionsResponse res;

	soap->recv_timeout = 10;	//sony cam timeout 
	NF_ONVIF_DBG(MAJOR, "START: VEC_TOKEN(%s) \n", vec_token);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	memset(&req, 0x00, sizeof(struct _media__GetVideoEncoderConfigurationOptions));
	req.ConfigurationToken = soap_malloc(soap, strlen(vec_token) + 1);
	strcpy(req.ConfigurationToken, vec_token);

	/* soap call */
	rtn = soap_call___media__GetVideoEncoderConfigurationOptions
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration option get fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

	if (res.Options == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration option get fail\n");
		goto ends_label;
	}

	struct tt__VideoEncoderConfigurationOptions *opt = res.Options;

	if(opt->QualityRange != NULL)
	{
	}


	if(opt->H264 != NULL)
	{
		encoder->vcodec[stream_no] = NF_IPCAM_VCODEC_H264;
		video->vcodec[stream_no] = NF_IPCAM_VCODEC_H264;

		// TODO set modes&values of video_t
		if(opt->H264->FrameRateRange != NULL)
		{
			encoder->max_framerate[stream_no] = (unsigned int) MIN(opt->H264->FrameRateRange->Max, 30);
			/* refer to sysdb_load_onvif */
			encoder->cur_maxfps[stream_no] = (unsigned int) MIN(opt->H264->FrameRateRange->Max, 30);

			video->fps[0][stream_no].support = NF_IPCAM_FPS_10;
			video->fps[1][stream_no].support = NF_IPCAM_FPS_10;

			//if (nf_ipcam_is_config_changable(0) == 1)
			{
				if(opt->H264->FrameRateRange->Min <= 2 && 2 <= opt->H264->FrameRateRange->Max)
				{
					video->fps[0][stream_no].support |= NF_IPCAM_FPS_20;
					video->fps[1][stream_no].support |= NF_IPCAM_FPS_20;
				}
				if(opt->H264->FrameRateRange->Min <= 3 && 3 <= opt->H264->FrameRateRange->Max)
				{
					video->fps[0][stream_no].support |= NF_IPCAM_FPS_30;
					video->fps[1][stream_no].support |= NF_IPCAM_FPS_30;
				}
				if(opt->H264->FrameRateRange->Min <= 6 && 6 <= opt->H264->FrameRateRange->Max)
				{
					//video->fps[0][stream_no].support |= NF_IPCAM_FPS_60;
					video->fps[1][stream_no].support |= NF_IPCAM_FPS_60;
					video->fps[1][stream_no].value = NF_IPCAM_FPS_60;
				}
				if(opt->H264->FrameRateRange->Min <= 7 && 7 <= opt->H264->FrameRateRange->Max)
				{
					video->fps[0][stream_no].support |= NF_IPCAM_FPS_70;
					//video->fps[1][stream_no].support |= NF_IPCAM_FPS_70;
					video->fps[0][stream_no].value = NF_IPCAM_FPS_70;
				}
				if(opt->H264->FrameRateRange->Min <= 12 && 12 <= opt->H264->FrameRateRange->Max)
				{
					//video->fps[0][stream_no].support |= NF_IPCAM_FPS_120;
					video->fps[1][stream_no].support |= NF_IPCAM_FPS_120;
					video->fps[1][stream_no].value = NF_IPCAM_FPS_120;
				}
				if(opt->H264->FrameRateRange->Min <= 15 && 15 <= opt->H264->FrameRateRange->Max)
				{
					video->fps[0][stream_no].support |= NF_IPCAM_FPS_150;
					//video->fps[1][stream_no].support |= NF_IPCAM_FPS_150;
					video->fps[0][stream_no].value = NF_IPCAM_FPS_150;
				}
			}

			if(opt->H264->FrameRateRange->Min <= 25 && 25 <= opt->H264->FrameRateRange->Max)
			{
				//video->fps[0][stream_no].support |= NF_IPCAM_FPS_120;
				video->fps[1][stream_no].support |= NF_IPCAM_FPS_250;
				video->fps[1][stream_no].value = NF_IPCAM_FPS_250;
			}
			if(opt->H264->FrameRateRange->Min <= 30 && 30 <= opt->H264->FrameRateRange->Max)
			{
				video->fps[0][stream_no].support |= NF_IPCAM_FPS_300;
				//video->fps[1][stream_no].support |= NF_IPCAM_FPS_150;
				video->fps[0][stream_no].value = NF_IPCAM_FPS_300;
			}
		}
		if(opt->H264->EncodingIntervalRange != NULL)
		{
		}
		if(opt->H264->GovLengthRange != NULL)
		{
			govLength->min = opt->H264->GovLengthRange->Min;
			govLength->max = opt->H264->GovLengthRange->Max;
		}
		for(i = 0; i < opt->H264->__sizeResolutionsAvailable; i++)
		{

			resol[i].width = opt->H264->ResolutionsAvailable[i].Width;
			resol[i].height = opt->H264->ResolutionsAvailable[i].Height;
			resol[i].pixel = resol[i].width * resol[i].height;
		}
		*size_res = opt->H264->__sizeResolutionsAvailable;

		if(opt->Extension != NULL)
		{
			if(opt->Extension->H264 != NULL)
			{
				NF_ONVIF_DBG(MINOR, " H264 Extension \n");
				NF_ONVIF_DBG(MINOR, " H264 Extension prof. no : %d \n", opt->Extension->H264->__sizeH264ProfilesSupported);

				if(opt->Extension->H264->FrameRateRange != NULL)
				{
				}
				if(opt->Extension->H264->EncodingIntervalRange != NULL)
				{
				}
				if(opt->Extension->H264->GovLengthRange != NULL)
				{
				}
				if(opt->Extension->H264->BitrateRange != NULL)
				{
					// set h264 bitrate range to encoder table

					int maxbr = opt->Extension->H264->BitrateRange->Max;
					int minbr = opt->Extension->H264->BitrateRange->Min;
					if(maxbr > 0 && minbr > 0)
					{
						if(maxbr > 20000) maxbr = 20000;
						if(minbr < 256) minbr = 256;

						encoder->max_bitrate[stream_no] = (unsigned int) maxbr;
						encoder->min_bitrate[stream_no] = (unsigned int) minbr;
					}
				}
			}
		}

	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


ONVIF_MSG _nf_onvif_media_get_video_encoder_resolution(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, const char *profile_token, _nf_local_resolution *resol, int *size_res)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = get_runtime();

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoEncoderConfigurationOptions req;
	struct _media__GetVideoEncoderConfigurationOptionsResponse res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap->recv_timeout = 10;	//sony cam timeout 
	NF_ONVIF_DBG(MAJOR, "START \n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);


	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	memset(&req, 0x00, sizeof(struct _media__GetVideoEncoderConfigurationOptions));
	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	strcpy(req.ProfileToken, profile_token);

	/* soap call */
	rtn = soap_call___media__GetVideoEncoderConfigurationOptions
			(soap, endpoint, NULL, &req, &res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration option get fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

	if (res.Options == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration option get fail\n");
		goto ends_label;
	}

	struct tt__VideoEncoderConfigurationOptions *opt = res.Options;

	if(opt->QualityRange != NULL)
	{
	}


	if(opt->H264 != NULL)
	{
		int res_size = 0;

		if(opt->H264->__sizeResolutionsAvailable > MAX_RES_SIZE)
			res_size = MAX_RES_SIZE;
		else
			res_size = opt->H264->__sizeResolutionsAvailable;

		for(i = 0; i < res_size; i++)
		{

			resol[i].width = opt->H264->ResolutionsAvailable[i].Width;
			resol[i].height = opt->H264->ResolutionsAvailable[i].Height;
			resol[i].pixel = resol[i].width * resol[i].height;
		}
		*size_res = res_size;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


ONVIF_MSG _nf_onvif_media_get_video_encoder_configuration(
		ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, int stream_no, video_t* video, encoder_t* encoder)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = get_runtime();

	time_t cam_time;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoEncoderConfiguration _req;
	struct _media__GetVideoEncoderConfigurationResponse _res;

	NF_ONVIF_DBG(MAJOR, "START \n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* _request setup */
	memset(&_req, 0x00, sizeof(struct _media__GetVideoEncoderConfiguration));
	_req.ConfigurationToken = soap_malloc(soap, strlen(vec_token) + 1);
	strcpy(_req.ConfigurationToken, vec_token);

	/* soap call */
	rtn = soap_call___media__GetVideoEncoderConfiguration
			(soap, endpoint, NULL, &_req, &_res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(_res.Configuration == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		goto ends_label;
	}

	struct tt__VideoEncoderConfiguration *cfg = _res.Configuration;
	if(cfg->Resolution != NULL)
	{
		NF_ONVIF_DBG(MINOR, "%d stream current resolution : %d x %d\n",
				stream_no, cfg->Resolution->Width, cfg->Resolution->Height);
	}
	if(cfg->RateControl != NULL)
	{
		NF_ONVIF_DBG(MINOR, "%d stream current fps %d bitrate %d\n", 
				stream_no, cfg->RateControl->FrameRateLimit, cfg->RateControl->BitrateLimit);

		if(cfg->RateControl->FrameRateLimit > 0)
		{
			video->fps[0][stream_no].value = _ipcam_convert_fps(cfg->RateControl->FrameRateLimit * 10);
			video->fps[1][stream_no].value = _ipcam_convert_fps(cfg->RateControl->FrameRateLimit * 10);

			//encoder->cur_maxfps[stream_no] = cfg->RateControl->FrameRateLimit;
		}
		if(cfg->RateControl->BitrateLimit > 0)
		{
			video->bitrate[stream_no].value = cfg->RateControl->BitrateLimit;
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_set_video_encoder_configuration(
		ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, const video_encoder_onvif info, const values govLength)
{
	int i = 0;
	int rtn = (-1);
	mtable* runtime = get_runtime();


	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoEncoderConfiguration _req;
	struct _media__GetVideoEncoderConfigurationResponse _res;
	struct _media__SetVideoEncoderConfiguration req;
	struct _media__SetVideoEncoderConfigurationResponse res;

	NF_ONVIF_DBG(MAJOR, "START \n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* _request setup */
	memset(&_req, 0x00, sizeof(struct _media__GetVideoEncoderConfiguration));
	_req.ConfigurationToken = soap_malloc(soap, strlen(vec_token) + 1);
	strcpy(_req.ConfigurationToken, vec_token);

	/* soap call */
	rtn = soap_call___media__GetVideoEncoderConfiguration
			(soap, endpoint, NULL, &_req, &_res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

	if(_res.Configuration == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		goto ends_label;
	}

	//if(_res.Configuration->Resolution->Height != info.height
	//|| _res.Configuration->Resolution->Width != info.width)
	{
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

		if (rtn != 0)
		{
			NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
			goto ends_label;
		}

		if (strstr(endpoint, "https://") != NULL)
		{
			rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
					NULL, NULL, NULL, NULL, NULL);

			if (rtn != 0)
			{
				SOAP_ERROR_PRINT(soap);
				goto ends_label;
			}
		}


		/* request setup */
		memset(&req, 0x00, sizeof(struct _media__SetVideoEncoderConfiguration));
		req.Configuration = _res.Configuration;

		/* for Panasonic(generate bad media:token by dom_attribute)
		 replace __anyattribute with null */
		struct soap_dom_attribute att;
		memcpy(&att, &req.Configuration->__anyAttribute, sizeof(struct soap_dom_attribute));
		memset(&req.Configuration->__anyAttribute, 0x00, sizeof(struct soap_dom_attribute));

		if(req.Configuration->Resolution == NULL)
		{
			req.Configuration->Resolution = soap_malloc(soap, sizeof(struct tt__VideoResolution));
		}

		if(info.width > 0 && info.height > 0)
		{
			req.Configuration->Resolution->Width = info.width;
			req.Configuration->Resolution->Height = info.height;
		}

		if(req.Configuration->Encoding != tt__VideoEncoding__H264)
		{
			req.Configuration->Encoding = tt__VideoEncoding__H264;
		}
		if(req.Configuration->H264 == NULL)
		{
			req.Configuration->H264 = soap_malloc(soap, sizeof(struct tt__H264Configuration));
			req.Configuration->H264->GovLength = 30;
			req.Configuration->H264->H264Profile = tt__H264Profile__Main;
		}


		if(req.Configuration->RateControl != NULL)
		{
			if(info.bitratelimit != 0)
			{
				req.Configuration->RateControl->BitrateLimit = info.bitratelimit;
			}
			if(info.frameratelimit != 0)
			{
				req.Configuration->RateControl->FrameRateLimit = info.frameratelimit;
			}
			if(info.govlength != 0)
			{
				req.Configuration->H264->GovLength = info.govlength;
			}

			if(req.Configuration->RateControl->FrameRateLimit != req.Configuration->H264->GovLength)
			{
				if(req.Configuration->RateControl->FrameRateLimit >= govLength.min)
				{
					req.Configuration->H264->GovLength = req.Configuration->RateControl->FrameRateLimit;
				}
			}
		}

		req.ForcePersistence = xsd__boolean__true_;

		/* soap call */
		rtn = soap_call___media__SetVideoEncoderConfiguration
				(soap, endpoint, NULL, &req, &res);

		/* revert it for soap_free */
		memcpy(&req.Configuration->__anyAttribute, &att, sizeof(struct soap_dom_attribute));

		/* response handle */
		if (rtn != 0)
		{
			NF_ONVIF_DBG(ERROR, "video encoder configuration option set fail\n");

			SOAP_ERROR_PRINT(soap);
		}
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_h264_codec_change(
		ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* vec_token, struct onvif_video_encoder_option *option)
{
	int i = 0;
	int rtn = (-1);

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetVideoEncoderConfiguration _req;			//get request
	struct _media__GetVideoEncoderConfigurationResponse _res;	//get response
	struct _media__SetVideoEncoderConfiguration req;
	struct _media__SetVideoEncoderConfigurationResponse res;

	//NF_ONVIF_DBG(MAJOR, "CHANGE CODEC START \n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* _request setup */
	memset(&_req, 0x00, sizeof(struct _media__GetVideoEncoderConfiguration));
	_req.ConfigurationToken = soap_malloc(soap, strlen(vec_token) + 1);
	strcpy(_req.ConfigurationToken, vec_token);

	/* soap call */
	rtn = soap_call___media__GetVideoEncoderConfiguration
			(soap, endpoint, NULL, &_req, &_res);

	/* response handle */
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		SOAP_ERROR_PRINT(soap);

		goto ends_label;
	}

	if(_res.Configuration == NULL)
	{
		NF_ONVIF_DBG(ERROR, "video encoder configuration get fail\n");

		goto ends_label;
	}

	{
		rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

		if (rtn != 0)
		{
			NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
			goto ends_label;
		}

		if (strstr(endpoint, "https://") != NULL)
		{
			rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
					NULL, NULL, NULL, NULL, NULL);

			if (rtn != 0)
			{
				SOAP_ERROR_PRINT(soap);
				goto ends_label;
			}
		}


		/* request setup */
		memset(&req, 0x00, sizeof(struct _media__SetVideoEncoderConfiguration));
		req.Configuration = _res.Configuration;
		//auto added resolution


		/* for Panasonic(generate bad media:token by dom_attribute)
		 replace __anyattribute with null */
		struct soap_dom_attribute att;
		memcpy(&att, &req.Configuration->__anyAttribute, sizeof(struct soap_dom_attribute));
		memset(&req.Configuration->__anyAttribute, 0x00, sizeof(struct soap_dom_attribute));

		if(req.Configuration->Resolution == NULL)
		{
			req.Configuration->Resolution = soap_malloc(soap, sizeof(struct tt__VideoResolution));
		}
		
		//if(info.width > 0 && info.height > 0)
		//{
		//	req.Configuration->Resolution->Width = _req.Configuration.width;
		//	req.Configuration->Resolution->Height = _req.Configuration.height;
		//}

		//not h264 -> h264 plz
		if(req.Configuration->Encoding != tt__VideoEncoding__H264)
		{
			req.Configuration->Encoding = tt__VideoEncoding__H264;
		}

		if(req.Configuration->H264 == NULL)
		{
			req.Configuration->H264 = soap_malloc(soap, sizeof(struct tt__H264Configuration));
			req.Configuration->H264->GovLength = 15;//need to get routine
			req.Configuration->H264->H264Profile = option->profile_supported;
		}
/*
		if(req.Configuration->RateControl != NULL)
		{
			if(info.bitratelimit != 0)
			{
				req.Configuration->RateControl->BitrateLimit = info.bitratelimit;
			}
			if(info.frameratelimit != 0)
			{
				req.Configuration->RateControl->FrameRateLimit = info.frameratelimit;
			}
			if(info.govlength != 0)
			{
				req.Configuration->H264->GovLength = info.govlength;
			}

			if(req.Configuration->RateControl->FrameRateLimit != req.Configuration->H264->GovLength)
			{
				if(req.Configuration->RateControl->FrameRateLimit >= govLength.min)
				{
					req.Configuration->H264->GovLength = req.Configuration->RateControl->FrameRateLimit;
				}
			}
		}
*/

		req.ForcePersistence = xsd__boolean__true_;

		/* soap call */
		rtn = soap_call___media__SetVideoEncoderConfiguration
				(soap, endpoint, NULL, &req, &res);

		/* revert it for soap_free */
		memcpy(&req.Configuration->__anyAttribute, &att, sizeof(struct soap_dom_attribute));

		/* response handle */
		if (rtn != 0)
		{
			NF_ONVIF_DBG(ERROR, "video encoder configuration option set fail\n");

			SOAP_ERROR_PRINT(soap);
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_get_stream_uri(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* profile_token, char* uri)
{
	int i = 0;
	int rtn = (-1);

	struct soap *soap;
	struct _media__GetStreamUri req;
	struct _media__GetStreamUriResponse res;
	struct tt__StreamSetup *stream;

	NF_ONVIF_DBG(MAJOR, "START\n");

	if(profile_token == NULL)
	{
		return  -1;
	}

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	soap->recv_timeout = 10;

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	req.StreamSetup = soap_malloc(soap, sizeof(struct tt__StreamSetup));
	memset(req.StreamSetup, 0x00, sizeof(struct tt__StreamSetup));
		stream = req.StreamSetup;

		stream->Stream = tt__StreamType__RTP_Unicast;
		stream->Transport = soap_malloc(soap, sizeof(struct tt__Transport));
		stream->Transport->Protocol = tt__TransportProtocol__RTSP;
		stream->Transport->Tunnel = NULL;
		stream->__size = 0;
		stream->__any = NULL;

	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	strcpy(req.ProfileToken, profile_token);

	rtn = soap_call___media__GetStreamUri
			(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}
	if(res.MediaUri == NULL)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}
	if(res.MediaUri->Uri == NULL)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}

	snprintf(uri, 256, "%s", res.MediaUri->Uri);
	NF_ONVIF_DBG(MINOR, "profile %s : uri %s\n", profile_token, uri);

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_get_metadata_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char* meta_token)
{
	int i = 0;
	int rtn = (-1);

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__GetMetadataConfigurations req;
	struct _media__GetMetadataConfigurationsResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	memset(&req, 0x00, sizeof(struct _media__GetMetadataConfigurations));
	rtn = soap_call___media__GetMetadataConfigurations
			(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs get fail\n");
		goto ends_label;
	}
	if(res.Configurations == NULL || res.__sizeConfigurations == 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs get fail\n");
		rtn = -9;
		goto ends_label;
	}

	snprintf(meta_token, TOKEN_LEN, "%s", res.Configurations[0].token);
	NF_ONVIF_DBG(MINOR, "ch(%d)metadata token %s selected\n",property->ch,  meta_token);

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_set_metadata_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char* meta_token)
{
	int i = 0;
	int rtn = (-1);

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	struct soap *soap;
	struct _media__GetMetadataConfiguration getReq;
	struct _media__GetMetadataConfigurationResponse getRes;

	struct _media__SetMetadataConfiguration setReq;
	struct _media__SetMetadataConfigurationResponse setRes;



	NF_ONVIF_DBG(MAJOR, "START\n");

	soap = soap_new();

	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	struct tt__EventSubscription event;


	memset(&getReq, 0x00, sizeof(getReq));
	memset(&getRes, 0x00, sizeof(getRes));
	memset(&setReq, 0x00, sizeof(setReq));
	memset(&setRes, 0x00, sizeof(setRes));
	memset(&event, 0x00, sizeof(event));

	getReq.ConfigurationToken = meta_token;


	rtn = soap_call___media__GetMetadataConfiguration(soap, endpoint, NULL, &getReq, &getRes);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs get fail: rtn: %d\n", rtn);
		goto ends_label;
	}

	if(getRes.Configuration == NULL)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs get fail: config == null\n");
		rtn = -9;
		goto ends_label;
	}

	setReq.Configuration = getRes.Configuration;

	if(getRes.Configuration->Events != NULL)
	{
		goto ends_label;
	}
	struct tt__PTZFilter *PTZStatus;
	setReq.Configuration->Events = &event;
	setReq.ForcePersistence = xsd__boolean__true_;
	PTZStatus = setReq.Configuration->PTZStatus;
	setReq.Configuration->PTZStatus = NULL;



	event.Filter = NULL;
	event.SubscriptionPolicy = NULL;
	event.__size = 0;
	event.__any = NULL;


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	rtn = soap_call___media__SetMetadataConfiguration(soap, endpoint, NULL, &setReq, &setRes);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs set fail:%d\n", rtn);
		goto ends_label;
	}
	setReq.Configuration->PTZStatus = PTZStatus;

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_add_metadata_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, const char* profile_token, const char* meta_token)
{
	int i = 0;
	int rtn = (-1);

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _media__AddMetadataConfiguration req;
	struct _media__AddMetadataConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	NF_ONVIF_DBG(MAJOR, "START\n");

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	/* request setup */
	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	strcpy(req.ProfileToken, profile_token);
	req.ConfigurationToken = soap_malloc(soap, strlen(meta_token) + 1);
	strcpy(req.ConfigurationToken, meta_token);

	rtn = soap_call___media__AddMetadataConfiguration
			(soap, endpoint, NULL, &req, &res);
	NF_ONVIF_DBG(MINOR, "metadata token ch %d \n", property->ch );
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs add fail\n");
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _nf_onvif_media_get_video_analytics_configurations(ipcam_onvif_auth_info_t* auth_info, 
		struct onvif_property *property, struct onvif_motion_info *omi)
{
	int rtn;
	char *token;
	struct soap *soap;
	mtable *runtime;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _media__GetVideoAnalyticsConfigurations req;
	struct _media__GetVideoAnalyticsConfigurationsResponse res;

	runtime = get_runtime();

	int ch 		= property->ch;
	int auth	= property->auth;
	const char *endpoint	= property->endpoint;
	const char *user	= property->user;
	const char *pass	= property->pass;


	int i = 0;
	int size = 0;
	int module_type;
	int rule_type;
	int motion_type;

	NF_ONVIF_DBG(MAJOR, "START\n");
	memset(&req, 0x00, sizeof(req));
	memset(omi, 0x00, sizeof(struct onvif_motion_info));

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}


	if (strstr(endpoint, "https://") != NULL) {
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	rtn = soap_call___media__GetVideoAnalyticsConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.__sizeConfigurations == 0 || res.Configurations == NULL) {
		goto ends_label;
	} 

	struct tt__VideoAnalyticsConfiguration *vaconf;
	size = res.__sizeConfigurations;

	module_type = OMT_NONE;
	for(i = 0; i < size; i++) {
		vaconf = &res.Configurations[i];

		if(vaconf->AnalyticsEngineConfiguration != NULL) {
			int j;
			int  conf_size;
			struct tt__Config *conf;
			struct tt__AnalyticsEngineConfiguration *aec;

			aec = vaconf->AnalyticsEngineConfiguration;
			conf_size = aec->__sizeAnalyticsModule;

			for(j = 0; j < conf_size; j++) {
				conf = &aec->AnalyticsModule[j];

				if(conf == NULL || conf->Type == NULL)
					continue;

				if(strstr(conf->Type, ":CellMotionEngine") != NULL) {
					module_type = OMT_CELL;
					break;
				} else if(strstr(conf->Type, ":RegionMotionEngine") != NULL) {
					module_type = OMT_REGION;
					break;
				} else {
					module_type = OMT_NONE;
				}
			}

		}

		if(vaconf->RuleEngineConfiguration != NULL) {
			int k;
			int conf_size;
			struct tt__Config *conf = NULL;
			struct tt__RuleEngineConfiguration *rc;
			rc = vaconf->RuleEngineConfiguration;
			conf_size = rc->__sizeRule;

			for(k = 0; k < conf_size; k++) {
				conf = &rc->Rule[k];

				if(conf == NULL || conf->Type == NULL)
					continue;

				if(strstr(conf->Type, ":CellMotionDetector") != NULL) {
					rule_type = OMT_CELL;
					break;

				} else if(strstr(conf->Type, ":RegionMotionDetector") != NULL) {
					rule_type = OMT_REGION;
					break;

				}
				else if(strstr(conf->Type, ":MotionDetector") != NULL) {
					rule_type = OMT_MOTION;
					break;
				}
			}
		}

		if(module_type == OMT_CELL) {
			motion_type = OMT_CELL;
			token = vaconf->token;
			break;

		} else if(module_type == OMT_REGION) {
			motion_type = OMT_REGION;
			token = vaconf->token;
			break;

		} else if(module_type == OMT_NONE && rule_type == OMT_MOTION) {
			motion_type = OMT_MOTION;
			token = vaconf->token;
			break;

		} else {
			motion_type == 0;
			token = vaconf->token;
		}
	}


	if(token != NULL && strlen(token) > 0) {
		strncpy(omi->token, token, strlen(token));
		omi->type = motion_type;
	}
	else {
		omi->type = 0;
		memset(omi->token, 0x00, TOKEN_LEN);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;

}

ONVIF_MSG _nf_onvif_media_add_video_analytics_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *conf_token)
{
	int rtn;
	struct soap *soap;
	struct _media__AddVideoAnalyticsConfiguration req;
	struct _media__AddVideoAnalyticsConfigurationResponse res;

	int ch 		= property->ch;
	int auth	= property->auth;
	const char *endpoint	= property->endpoint;
	const char *user	= property->user;
	const char *pass	= property->pass;

	soap = soap_new();

	nf_onvif_soap_init_set(soap);


	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ProfileToken = profile_token;
	req.ConfigurationToken = conf_token;


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL) {
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}


	rtn = soap_call___media__AddVideoAnalyticsConfiguration(soap, endpoint, NULL, &req, &res);

	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_media_add_ptz_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char* profile_token, char* ptz_token)
{
	int rtn;
	struct soap *soap;
	struct _media__AddPTZConfiguration req;
	struct _media__AddPTZConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}



	req.ProfileToken = profile_token;
	req.ConfigurationToken = ptz_token;

	rtn = soap_call___media__AddPTZConfiguration(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "ptz configs add fail\n");
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;


}

ONVIF_MSG _nf_onvif_ptz_get_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_ptz_nodes *ptz_nodes)
{
	int rtn;
	struct soap *soap;
	struct _ptz__GetConfigurations req;
	struct _ptz__GetConfigurationsResponse res;
	mtable *runtime = get_runtime();


	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}


	rtn = soap_call___ptz__GetConfigurations(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "metadata configs add fail\n");
		goto ends_label;
	}

	if(res.__sizePTZConfiguration > 0 && res.PTZConfiguration != NULL && res.PTZConfiguration[0].token != NULL)
	{
		int i = 0;;
		int size = res.__sizePTZConfiguration;

		if(size > MAX_ATT)
		{
			size = MAX_ATT;
		}

		for(i = 0; i < size; i++)
		{
			if(res.PTZConfiguration[i].token != NULL)
			{
				strncpy(ptz_nodes->token[i], res.PTZConfiguration[i].token, TOKEN_LEN);
			}
			else
			{
				i--;
			}
		}
		ptz_nodes->size = i;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;

}

/**
 * @brief ONVIF 카메라의 Profile 조회하여 Token 정보를 가져온다.
 */
ONVIF_MSG _nf_onvif_get_h264_profiles(
		ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_profiles *profiles)
{
	int rtn = -1;
	int size = 0;
	int i;
	struct soap *soap = NULL;
	struct _media__GetProfiles req;
	struct _media__GetProfilesResponse res;
	int h264_profile;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;


	memset(profiles, 0x00, sizeof(struct onvif_profiles));
	soap = soap_new();
	nf_onvif_soap_init_set(soap);


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetProfiles(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}


	if(res.__sizeProfiles == 0)
	{
		NF_ONVIF_DBG(ERROR,	"Profile get fail (size 0)");
		rtn = -1;
		goto ends_label;
	}

	size = res.__sizeProfiles;

	profiles->size = size;

	_nf_local_resolution resol[MAX_RES_SIZE];
	memset(&resol, 0x00, sizeof(resol));
	int res_size = 0;

	h264_profile = 0;
	for(i = 0; i < size; i++)
	{
		struct tt__Profile *prof;
		prof = &res.Profiles[i];

		if(prof->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].token, prof->token);
		}
		else
		{
			continue;
		}

		if(prof->Name != NULL)
		{
			strcpy(profiles->profile[h264_profile].name, prof->token);
		}
		else
		{
			continue;
		}

		if(prof->VideoSourceConfiguration != NULL && prof->VideoSourceConfiguration->token != NULL
				&& prof->VideoSourceConfiguration->SourceToken != NULL)
		{
			strcpy(profiles->profile[h264_profile].vsc_token, prof->VideoSourceConfiguration->token);
			strcpy(profiles->profile[h264_profile].vs_token, prof->VideoSourceConfiguration->SourceToken);
		}
		else
		{
			continue;
		}

		if(prof->AudioSourceConfiguration != NULL && prof->AudioSourceConfiguration->token != NULL
				&& prof->AudioSourceConfiguration->SourceToken != NULL)
		{
			strcpy(profiles->profile[h264_profile].asc_token, prof->AudioSourceConfiguration->token);
			strcpy(profiles->profile[h264_profile].as_token, prof->AudioSourceConfiguration->SourceToken);
		}

		if(prof->VideoEncoderConfiguration != NULL && prof->VideoEncoderConfiguration->token != NULL)
		{
			if(prof->VideoEncoderConfiguration->Encoding != tt__VideoEncoding__H264 || prof->VideoEncoderConfiguration->H264 == NULL)
			{
				continue;
			}
			strcpy(profiles->profile[h264_profile].vec_token, prof->VideoEncoderConfiguration->token);
			profiles->profile[h264_profile].vec_quality = prof->VideoEncoderConfiguration->Quality;
		}
		else
		{
			continue;
		}

		if(_nf_onvif_media_get_video_encoder_resolution(auth_info ,property, prof->token, resol, &res_size) != 0)
		{
			NF_ONVIF_DBG(WARN, "GetVideoEncoderResolution Fail: ProfileToken(%s)\n", prof->token);
			continue;
		}

		if(_nf_onvif_search_res(resol,res_size) < 0)
		{
			NF_ONVIF_DBG(WARN, "Supported Resolution Not Found: ProfileToken(%s)\n", prof->token);
			continue;
		}


		if(prof->AudioEncoderConfiguration != NULL && prof->AudioEncoderConfiguration->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].aec_token, prof->AudioEncoderConfiguration->token);
		}

		if(prof->VideoAnalyticsConfiguration != NULL && prof->VideoAnalyticsConfiguration->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].vac_token, prof->VideoAnalyticsConfiguration->token);
		}

		if(prof->PTZConfiguration != NULL && prof->PTZConfiguration->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].ptz_token, prof->PTZConfiguration->token);
		}

		if(prof->PTZConfiguration != NULL && prof->PTZConfiguration->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].ptz_token, prof->PTZConfiguration->token);
		}

		if(prof->MetadataConfiguration != NULL && prof->MetadataConfiguration->token != NULL)
		{
			strcpy(profiles->profile[h264_profile].meta_token, prof->MetadataConfiguration->token);
		}

		if(prof->Extension != NULL)
		{
			struct tt__ProfileExtension *extn;
			extn = prof->Extension;
			if(extn->AudioOutputConfiguration != NULL && extn->AudioOutputConfiguration->token != NULL)
			{
				strcpy(profiles->profile[h264_profile].aoc_token, extn->AudioOutputConfiguration->token);
			}

			if(extn->AudioDecoderConfiguration != NULL && extn->AudioDecoderConfiguration->token != NULL)
			{
				strcpy(profiles->profile[h264_profile].adc_token, extn->AudioDecoderConfiguration->token);
			}
		}

		h264_profile++;

		if(h264_profile >= MAX_ATT)
			break;
	}

	profiles->size = h264_profile;


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;

}

ONVIF_MSG _nf_onvif_get_profiles(
		ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_profiles *profiles)
{
	int rtn = -1;
	int size = 0;
	int i;
	struct soap *soap = NULL;
	struct _media__GetProfiles req;
	struct _media__GetProfilesResponse res;
	int profile;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;


	memset(profiles, 0x00, sizeof(struct onvif_profiles));
	soap = soap_new();
	nf_onvif_soap_init_set(soap);


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetProfiles(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}


	if(res.__sizeProfiles == 0)
	{
		NF_ONVIF_DBG(ERROR,	"Profile get fail (size 0)");
		rtn = -1;
		goto ends_label;
	}

	size = res.__sizeProfiles;

	profiles->size = size;

	_nf_local_resolution resol[MAX_RES_SIZE];
	memset(&resol, 0x00, sizeof(resol));
	int res_size = 0;

	profile = 0;

	for(i = 0; i < size; i++)
	{
		struct tt__Profile *prof;
		prof = &res.Profiles[i];

		if(prof->token != NULL)
		{
			strcpy(profiles->profile[profile].token, prof->token);
		}
		else
		{
			continue;
		}

		if(prof->Name != NULL)
		{
			strcpy(profiles->profile[profile].name, prof->token);
		}
		else
		{
			continue;
		}

		if(prof->VideoSourceConfiguration != NULL && prof->VideoSourceConfiguration->token != NULL
				&& prof->VideoSourceConfiguration->SourceToken != NULL)
		{
			strcpy(profiles->profile[profile].vsc_token, prof->VideoSourceConfiguration->token);
			strcpy(profiles->profile[profile].vs_token, prof->VideoSourceConfiguration->SourceToken);
		}
		else
		{
			continue;
		}

		if(prof->AudioSourceConfiguration != NULL && prof->AudioSourceConfiguration->token != NULL
				&& prof->AudioSourceConfiguration->SourceToken != NULL)
		{
			strcpy(profiles->profile[profile].asc_token, prof->AudioSourceConfiguration->token);
			strcpy(profiles->profile[profile].as_token, prof->AudioSourceConfiguration->SourceToken);
		}

		if(prof->VideoEncoderConfiguration != NULL && prof->VideoEncoderConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].vec_token, prof->VideoEncoderConfiguration->token);
		}
		else
		{
			continue;
		}

		if(_nf_onvif_media_get_video_encoder_resolution(auth_info ,property, prof->token, resol, &res_size) != 0)
		{
			NF_ONVIF_DBG(WARN, "GetVideoEncoderResolution Fail: ProfileToken(%s)\n", prof->token);
			continue;
		}

		if(_nf_onvif_search_res(resol,res_size) < 0)
		{
			NF_ONVIF_DBG(WARN, "Supported Resolution Not Found: ProfileToken(%s)\n", prof->token);
			continue;
		}


		if(prof->AudioEncoderConfiguration != NULL && prof->AudioEncoderConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].aec_token, prof->AudioEncoderConfiguration->token);
		}

		if(prof->VideoAnalyticsConfiguration != NULL && prof->VideoAnalyticsConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].vac_token, prof->VideoAnalyticsConfiguration->token);
		}

		if(prof->PTZConfiguration != NULL && prof->PTZConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].ptz_token, prof->PTZConfiguration->token);
		}

		if(prof->PTZConfiguration != NULL && prof->PTZConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].ptz_token, prof->PTZConfiguration->token);
		}

		if(prof->MetadataConfiguration != NULL && prof->MetadataConfiguration->token != NULL)
		{
			strcpy(profiles->profile[profile].meta_token, prof->MetadataConfiguration->token);
		}

		if(prof->Extension != NULL)
		{
			struct tt__ProfileExtension *extn;
			extn = prof->Extension;
			if(extn->AudioOutputConfiguration != NULL && extn->AudioOutputConfiguration->token != NULL)
			{
				strcpy(profiles->profile[profile].aoc_token, extn->AudioOutputConfiguration->token);
			}

			if(extn->AudioDecoderConfiguration != NULL && extn->AudioDecoderConfiguration->token != NULL)
			{
				strcpy(profiles->profile[profile].adc_token, extn->AudioDecoderConfiguration->token);
			}
		}

		profile++;

		if(profile >= MAX_ATT)
			break;
	}

	profiles->size = profile;


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;

}

ONVIF_MSG _nf_onvif_get_video_encoder_configurations(
		ipcam_onvif_auth_info_t* auth_info,	struct onvif_property *property, struct onvif_video_encoders *video_encoders)
{
	int i;
	int rtn = -1;
	int size = 0;
	struct soap *soap = NULL;
	struct _media__GetVideoEncoderConfigurations req;
	struct _media__GetVideoEncoderConfigurationsResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetVideoEncoderConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.__sizeConfigurations == 0 || res.Configurations == NULL)
		goto ends_label;

	size = res.__sizeConfigurations;

	struct tt__VideoEncoderConfiguration *vec;
	vec = res.Configurations;

	struct onvif_video_encoder *venc = video_encoders->video_encoder;

	if(size > MAX_ATT)
	{
		size = MAX_ATT;
	}

	video_encoders->size = size;

	for(i = 0; i < size; i++)
	{
		if(vec[i].token != NULL)
		{
			strcpy(venc[i].token, vec[i].token);
			strcpy(venc[i].name, vec[i].Name);
			venc[i].encoding = vec[i].Encoding;
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_get_video_encoder_options(
		ipcam_onvif_auth_info_t* auth_info,	 struct onvif_property *property,
		char *token, struct onvif_video_encoder_option *option)
{
	int rtn;
	struct soap *soap = NULL;
	struct _media__GetVideoEncoderConfigurationOptions req;
	struct _media__GetVideoEncoderConfigurationOptionsResponse res;
 
	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	if(token != NULL)
	{
		req.ConfigurationToken = token;
	}

	rtn = soap_call___media__GetVideoEncoderConfigurationOptions(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.Options != NULL)
	{
		int i;
		struct tt__VideoEncoderConfigurationOptions *opt;
		opt = res.Options;
		if(opt->H264 != NULL)
		{
			int resSize = 0;
			int width;
			int height;

			option->h264_supported = 1;

			struct tt__H264Options *h264;
			h264 = opt->H264;

			resSize = h264->__sizeResolutionsAvailable;
			for(i = 0; i < resSize; i++)
			{
				width = h264->ResolutionsAvailable[i].Width;
				height = h264->ResolutionsAvailable[i].Height;
			}

			option->width = width;
			option->height = height;

			option->profile_supported = *(h264->H264ProfilesSupported);
		}
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_set_video_encoder_h264(ipcam_onvif_auth_info_t* auth_info,
	struct onvif_property *property, struct onvif_video_encoder *info, struct onvif_video_encoder_option *option)
{
	int rtn;

	struct soap *soap;
	struct _media__SetVideoEncoderConfiguration req;
	struct _media__SetVideoEncoderConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	struct tt__VideoEncoderConfiguration conf;
	struct tt__VideoResolution resolution;
	memset(&conf, 0x00, sizeof(conf));
	memset(&resolution, 0x00, sizeof(resolution));

	resolution.Width = option->width;
	resolution.Height= option->height;

	conf.Name = info->name;
	conf.UseCount = 1;
	conf.token = info->token;
	conf.Encoding = 2;
	conf.Resolution = &resolution;
	//conf.Quality = info->quality;

	req.ForcePersistence = xsd__boolean__true_;
	req.Configuration = &conf;


	rtn = soap_call___media__SetVideoEncoderConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_get_audio_output_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_audio_outputs* audio_out)
{
	int rtn = -1;
	int ao_size = 0;
	int i = 0;;
	struct soap *soap;
	const char *soap_endpoint;
	const char *soap_action;
	struct _media__GetAudioOutputConfigurations req;
	struct _media__GetAudioOutputConfigurationsResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetAudioOutputConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.__sizeConfigurations == 0 || res.Configurations == NULL)
		goto ends_label;
	ao_size = res.__sizeConfigurations;

	if(ao_size < 1)
	{
		NF_ONVIF_DBG(ERROR, "Fail - AudioOutputs Size < 1\n");
		rtn = -1;
		goto ends_label;
	}

	if(ao_size > 4)
	{
		ao_size = 4;
	}

	audio_out->size = ao_size;

	for(i = 0; i < ao_size; i++)
	{
		if(res.Configurations[i].token != NULL)
		{
			strcpy(audio_out->token[i], res.Configurations[i].token);
		}
	}
	

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_get_audio_decoder_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_audio_decoders* audio_dec)
{
	int rtn;
	int adc_size = 0; // AudioDecoderConfigurations Size
	int i;
	struct soap *soap;
	struct _media__GetAudioDecoderConfigurations req;
	struct _media__GetAudioDecoderConfigurationsResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));


	rtn = soap_call___media__GetAudioDecoderConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	adc_size = res.__sizeConfigurations;
	if(adc_size < 1 || res.Configurations == NULL)
	{
		NF_ONVIF_DBG(ERROR, "Fail - AudioDeocderConfiguratons Size < 1\n");
		goto ends_label;
	}

	if(adc_size > MAX_ATT)
	{
		adc_size = MAX_ATT;
	}

	audio_dec->size = adc_size;

	for(i = 0; i < adc_size; i++)
	{
		if(res.Configurations[i].token != NULL)
		{
			strcpy(audio_dec->token[i], res.Configurations[i].token);
		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
} //END _nf_onvif_get_audio_outputs

ONVIF_MSG _nf_onvif_add_audio_output_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *configuration_token)
{
	int rtn = -1;
	struct soap *soap;
	const char *soap_endpoint;
	const char *soap_action;
	struct _media__AddAudioOutputConfiguration req;
	struct _media__AddAudioOutputConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ProfileToken = profile_token;
	req.ConfigurationToken = configuration_token;

	rtn = soap_call___media__AddAudioOutputConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_add_audio_decoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property,  char *profile_token, char *configuration_token)
{
	int rtn = -1;
	struct soap *soap;
	struct _media__AddAudioDecoderConfiguration req;
	struct _media__AddAudioDecoderConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ProfileToken = profile_token;
	req.ConfigurationToken = configuration_token;

	rtn = soap_call___media__AddAudioDecoderConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_get_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, 
		struct onvif_property *property, char *token, struct onvif_audio_encoder *audio_enc)
{
	int rtn;
	int adc_size = 0; // AudioDecoderConfigurations Size
	int i;
	struct soap *soap;
	struct _media__GetAudioEncoderConfiguration req; 
	struct _media__GetAudioEncoderConfigurationResponse res;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* user = property->user;
	const char* pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ConfigurationToken = token;

	rtn = soap_call___media__GetAudioEncoderConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.Configuration != NULL)
	{
		struct tt__AudioEncoderConfiguration *conf;
		conf = res.Configuration;
		if(conf->token != NULL)
		{
			strcpy(audio_enc->token, conf->token);
			audio_enc->bitrate = conf->Bitrate;
			audio_enc->samplerate = conf->SampleRate;
			audio_enc->encoding = conf->Encoding;
		}
	}

	
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
} //END _nf_onvif_get_audio_outputs

ONVIF_MSG _nf_onvif_get_audio_encoder_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property * property, struct onvif_audio_encoders *aecs)
{
	int i;
	int rtn = 0;
	int size = 0;
	struct soap *soap;
	struct _media__GetAudioEncoderConfigurations req;
	struct _media__GetAudioEncoderConfigurationsResponse res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") !=  NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetAudioEncoderConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	if(res.__sizeConfigurations < 1 || res.Configurations == NULL) 
		goto ends_label;

	struct tt__AudioEncoderConfiguration *conf;
	conf = res.Configurations;
	size = res.__sizeConfigurations;
	aecs->size = size;

	for(i = 0; i < size; i++) {
		strncpy(aecs->enc[i].token, conf[i].token, TOKEN_LEN);
		aecs->enc[i].bitrate = conf[i].Bitrate;
		aecs->enc[i].samplerate = conf[i].SampleRate;
		aecs->enc[i].encoding = conf[i].Encoding;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_add_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *aec_token)
{
	int rtn = 0;
	struct soap *soap;
	struct _media__AddAudioEncoderConfiguration req;
	struct _media__AddAudioEncoderConfigurationResponse res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}


	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ProfileToken = profile_token;
	req.ConfigurationToken = aec_token;

	rtn = soap_call___media__AddAudioEncoderConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_get_audio_source_configurations(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, struct onvif_audio_sources *asc)
{
	int rtn = 0;
	int i;
	struct soap *soap;
	const char *soap_action;
	struct _media__GetAudioSourceConfigurations req;
	struct _media__GetAudioSourceConfigurationsResponse res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	rtn = soap_call___media__GetAudioSourceConfigurations(soap, endpoint, NULL, &req, &res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}
	
	if(res.__sizeConfigurations < 1 || res.Configurations == NULL) {
		goto ends_label;
	}

	asc->size = res.__sizeConfigurations;

	for(i = 0; i < res.__sizeConfigurations; i++) {
		strncpy(asc->token[i], res.Configurations[i].token,TOKEN_LEN);
	}

ends_label:

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_add_audio_source_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *profile_token, char *asrc_token)
{
	int rtn = 0;
	struct soap *soap;
	struct _media__AddAudioSourceConfiguration req;
	struct _media__AddAudioSourceConfigurationResponse res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0) {
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	req.ProfileToken = profile_token;
	req.ConfigurationToken = asrc_token;
	rtn = soap_call___media__AddAudioSourceConfiguration(soap, endpoint, NULL, &req, &res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

ends_label:

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

// Set Audio Encoder G711
ONVIF_MSG _nf_onvif_set_audio_encoder_configuration(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, char *aec_token)
{
	int rtn = 0;
	struct soap *soap;

	struct _media__GetAudioEncoderConfiguration get_req; 
	struct _media__GetAudioEncoderConfigurationResponse get_res;

	struct _media__SetAudioEncoderConfiguration set_req;
	struct _media__SetAudioEncoderConfigurationResponse set_res;

	const int ch = property->ch;
	const int auth = property->auth;
	const char *endpoint = property->endpoint;
	const char *user = property->user;
	const char *pass = property->pass;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0){
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	memset(&get_req, 0x00, sizeof(get_req));
	memset(&get_res, 0x00, sizeof(get_res));
	memset(&set_req, 0x00, sizeof(set_req));
	memset(&set_res, 0x00, sizeof(set_res));

	// Get Audio Encoder
	get_req.ConfigurationToken = aec_token;
	rtn = soap_call___media__GetAudioEncoderConfiguration(soap, endpoint, NULL, &get_req, &get_res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

	struct tt__AudioEncoderConfiguration *conf;

	conf = get_res.Configuration;

	if (!conf)
	{
		goto ends_label;
	}

	if(conf->Encoding == tt__AudioEncoding__G711 &&
			conf->Bitrate == 64 &&
			conf->SampleRate == 8) {
		goto ends_label;
	}


	// Set Audio Encoder
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0){
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL) {
		rtn = soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if(rtn != 0) {
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	set_req.Configuration = get_res.Configuration;
	set_req.Configuration->Encoding = tt__AudioEncoding__G711;
	set_req.Configuration->Bitrate = 64;
	set_req.Configuration->SampleRate = 8;
	set_req.ForcePersistence = xsd__boolean__false_;

	rtn = soap_call___media__SetAudioEncoderConfiguration(soap, endpoint, NULL, &set_req, &set_res);
	if(rtn != 0) {
		SOAP_ERROR_PRINT(soap);
		goto ends_label;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

ONVIF_MSG _nf_onvif_profile_check(ipcam_onvif_auth_info_t* auth_info, struct onvif_property *property, struct onvif_profiles* profiles)
{
	int i;
	int rtn = 0;
	int profile_size = 0;
	int stream_no = 0;
	char *fst= NULL;

	struct onvif_audio_outputs aoc_tokens;
	struct onvif_audio_decoders adc_tokens;

	struct onvif_profile *pro;
	struct onvif_ptz_nodes nodes;

	if(property == NULL || profiles == NULL)
		return -1;

	int ch = property->ch;
	int auth = property->auth;
	const char* endpoint = property->endpoint;
	const char* endpoint2 = NULL;
	const char* user = property->user;
	const char* pass = property->pass;


	if(profiles->size < 1 )
		return -2;


	if(property->endpoint2 != NULL)
		endpoint2 = property->endpoint2;

	// Profile
	{

		profile_size = profiles->size;
		stream_no = 0;
		for(i = 0; i< profile_size ; i++)
		{
			pro = &profiles->profile[i];
			if(strlen(pro->token) > 0)
			{
				if(strcasestr(pro->token, "mobile"))
				{
					continue;
				}
			}
			else
			{
				continue;
			}

			if(strlen(pro->vsc_token) < 1)
				continue;

			if(strlen(pro->vec_token) < 1) {
				continue;
			} else {

				if(stream_no != 0) {
					int j, eq = 0;
					for(j = 0; j < stream_no; j++)
					{
						if(strcmp(pro->vec_token, fst) == 0) {
							eq = 1;
							break;
						}
					}

					if(eq == 1)
						continue;
				}
				else
				{
					fst = pro->vec_token;
				}

				
			}



			if(strlen(pro->asc_token) < 1 && stream_no == 0)
			{
				// Audio Source  가 없으면 프로파일에 추가
				int tmp = 0;
				struct onvif_audio_sources asc;
				memset(&asc, 0x00, sizeof(asc));
				if(strlen(pro->asc_token) < 1) {
					tmp = _nf_onvif_get_audio_source_configurations(auth_info, property, pro->token, &asc);
					if(tmp == 0 && asc.size > 0) {
						tmp = _nf_onvif_add_audio_source_configuration(auth_info, property, pro->token, asc.token[0]);
					}
				}
			}

			if(strlen(pro->aec_token) < 1 && stream_no == 0)
			{
				// Audio Encoder 가 없으면 프로파일에 추가
				int tmp = 0;
				struct onvif_audio_encoders aec;
				memset(&aec, 0x00, sizeof(aec));
				tmp = _nf_onvif_get_audio_encoder_configurations(auth_info ,property, &aec);
				if(tmp == 0 && aec.size > 0) {
					tmp == _nf_onvif_set_audio_encoder_configuration(auth_info, property, aec.enc[0].token);
					tmp == _nf_onvif_add_audio_encoder_configuration(auth_info, property, pro->token, aec.enc[0].token);
				}
			}

			if(strlen(pro->aoc_token) < 1 && stream_no == 0)
			{
				memset(&aoc_tokens, 0x00, sizeof(aoc_tokens));
				rtn = _nf_onvif_get_audio_output_configurations(auth_info, property, &aoc_tokens);
				if(rtn == 0)
				{
					if(aoc_tokens.size > 0)
					{
						rtn = _nf_onvif_add_audio_output_configuration(auth_info, property, pro->token, aoc_tokens.token[0]);
					}
				}

			}

			if(strlen(pro->adc_token) < 1 && stream_no == 0)
			{
				memset(&adc_tokens, 0x00, sizeof(adc_tokens));
				rtn = _nf_onvif_get_audio_decoder_configurations(auth_info, property, &adc_tokens);
				if(rtn == 0)
				{
					if(adc_tokens.size > 0)
					{
						rtn = _nf_onvif_add_audio_decoder_configuration(auth_info, property, pro->token, adc_tokens.token[0]);
					}
				}
			}

			if(strlen(pro->ptz_token) < 1 && stream_no == 0)
			{
				if(endpoint2 != NULL)
				{
					struct onvif_property ptz_property;
					memset(&ptz_property, 0x00, sizeof(ptz_property));
					ptz_property.auth = auth;
					ptz_property.user = user;
					ptz_property.pass = pass;
					ptz_property.endpoint = endpoint2;

					rtn = _nf_onvif_ptz_get_configurations(auth_info, &ptz_property, &nodes);
					if(nodes.size > 0)
					{
						if(endpoint != NULL)
						{	
							//AddPtzConfig need to media endpoint 
							ptz_property.endpoint = endpoint;
							rtn  = _nf_onvif_media_add_ptz_configuration(auth_info, &ptz_property, pro->token, nodes.token[0]);
							ptz_property.endpoint = endpoint2;
						}
					}
				}
			}

			if(strlen(pro->vac_token) < 1 && stream_no == 0) 
			{
				struct onvif_motion_info omi;
				int tmp_rtn = 0;
				memset(&omi, 0x00, sizeof(omi));
				tmp_rtn = _nf_onvif_media_get_video_analytics_configurations(auth_info, property, &omi);

				if(tmp_rtn == 0 && strlen(omi.token) > 0) {
					_nf_onvif_media_add_video_analytics_configuration(auth_info, property, pro->token, omi.token);
				}
			}

			if(strlen(pro->meta_token) < 1 && stream_no == 0)
			{
				char meta_token[TOKEN_LEN];
				memset(meta_token, 0x00, TOKEN_LEN);
				rtn = _nf_onvif_media_get_metadata_configurations(auth_info, property, meta_token);
				if(rtn == 0)
				{
					rtn = _nf_onvif_media_add_metadata_configuration(auth_info, property, pro->token, meta_token);
					rtn = _nf_onvif_media_set_metadata_configuration(auth_info, property, meta_token);

				}
			}
			else if(stream_no == 0)
			{
				rtn = _nf_onvif_media_set_metadata_configuration(auth_info, property, pro->meta_token);
			}
			stream_no++;
		}
	}
	return rtn;
}



/**
 * @brief 주어진 해상도 목록으로 NVR 1st stream, 2nd stream에 적합한 해상도 값을 찾는다.
 * @param ch 채널 번호.
 * @param resol 해상도 목록 struct.
 * @param size 해상도 목록 크기.
 * @param stream_no Stream index.
 * @return 적합한 해상도를 찾았을 시 1.
 */
static int _nf_onvif_set_res(int ch, _nf_local_resolution* resol, const int size, int stream_no)
{
	mtable *runtime = get_runtime();

	int i, set_res = 0, set_res_ratio = 0, second_flag = 0;
	uint64_t video_resol_default = 0x00;
	_nf_local_resolution_ratio resol_ratio, second_resol_ratio;	
	memset(&resol_ratio, 0x00, sizeof(_nf_local_resolution_ratio));
	memset(&second_resol_ratio, 0x00, sizeof(_nf_local_resolution_ratio));	

	// sort by pixel : 1st stream 비율에 따라 2nd stream 비율을 결정하게 되면서 사용 안함
	qsort(resol, (unsigned int)size, sizeof(_nf_local_resolution), _nf_onvif_compare_res);
	
	// 2nd strema 결정하기 전 1st stream 비율 계산
	if(stream_no == 1)
	{
		if(set_res_ratio == 0)
		{
			if(_get_stream_ratio(&resol_ratio, runtime[ch].onvif.width, runtime[ch].onvif.height) != 0)
				NF_ONVIF_DBG(WARN, "1st stream resolution ratio get fail\n");
			set_res_ratio++; 
		}
	}

	for(i = 0; i < size; i++)
	{
		int w = resol[i].width;
		int h = resol[i].height;
		int pixel = resol[i].pixel;

		if(stream_no == 1)
		{
			if(_get_stream_ratio(&second_resol_ratio, w, h) != 0)
				NF_ONVIF_DBG(WARN, "2nd stream resolution ratio get fail\n");
		}


		// TODO resolution over HD
		if(pixel == 0 || pixel > 3840*2160) continue;

		// first stream only
		if(stream_no == 0)
		{
			if(w == 3840 && h == 2160)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_3840x2160; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_3840x2160;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_3840x2160;
			}
			if(w == 3000 && h == 3000)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_3000x3000; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_3000x3000;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_3000x3000;
			}
			else if(w == 3200 && h == 2400)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_3200x2400; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_3200x2400;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_3200x2400;
			}
			else if(w == 3072 && h == 2048)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_3072x2048; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_3072x2048;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_3072x2048;
			}
			else if(w == 2880 && h == 2160)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2880x2160; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2880x2160;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2880x2160;
			}
			else if(w == 3200 && h == 1800)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_3200x1800; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_3200x1800;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_3200x1800;
			}
			else if(w == 2880 && h == 1800)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2880x1800; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2880x1800;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2880x1800;
			}
			else if(w == 2992 && h == 1680)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2992x1680; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2992x1680;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2992x1680;
			}
			else if(w == 2592 && h == 1944)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2592x1944; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2592x1944;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2592x1944;
			}
			else if(w == 2592 && h == 1920)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2592x1920; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2592x1920;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2592x1920;
			}
			else if(w == 2560 && h == 1920)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2560x1920; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2560x1920;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2560x1920;
			}
			else if(w == 2560 && h == 1600)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2560x1600; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2560x1600;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2560x1600;
			}
			else if(w == 2688 && h == 1520)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2688x1520; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2688x1520;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2688x1520;
			}
			else if(w == 2048 && h == 2048)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2048x2048; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2048x2048;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2048x2048;
			}
			else if(w == 2592 && h == 1520)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2592x1520; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2592x1520;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2592x1520;
			}
			else if(w == 2560 && h == 1440)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2560x1440; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2560x1440;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2560x1440;
			}
			else if(w == 2048 && h == 1536)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2048x1536; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2048x1536;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2048x1536;
			}
			else if(w == 2304 && h == 1296)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_2304x1296; 
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_2304x1296;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_2304x1296;
			}
			else if(w == 1920 && h == 1080)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1920x1080;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1920x1080;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1920x1080;
			}
			else if(w == 1280 && h == 1280)
			{
				if(set_res == 0) 
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1280x1280;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1280x1280;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1280x1280;
			}
			else if(w == 1280 && h == 720)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1280x720;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1280x720;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1280x720;
			}
			else if (w == 1600 && h == 1200)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1600x1200;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1600x1200;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1600x1200;
			}
			else if (w == 1440 && h == 900)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1440x900;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1440x900;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1440x900;
			}
			else if (w == 1280 && h == 1024)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1280x1024;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1280x1024;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1280x1024;
			}
			else if (w == 1280 && h == 800)	// fixme new resolution
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1280x720;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1280x720;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1280x720;
			}
			else if (w == 1024 && h == 768)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_1024x768;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_1024x768;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_1024x768;
			}
			else if (w == 800 && h == 600)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_800x600;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_800x600;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_800x600;
			}
			else if (w == 800 && h == 450)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_800x450;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_800x450;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_800x450;
			}
			else if (w == 720 && h == 576)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_720x576;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_720x576;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_720x576;
			}
			else if (w == 720 && h == 480)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_720x480;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_720x480;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_720x480;
			}
			else if (w == 704 && h == 576)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_704x576;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_704x576;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_704x576;
			}
			else if (w == 704 && h == 480)
			{
				if(set_res == 0)
				{
					runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_704x480;
					runtime[ch].video.resolution.supported |= NF_IPCAM_RES_704x480;
					set_res++;
				}
				runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_704x480;
			}
		}
/* no support 320x240 resolution
		if (w == 320 && h == 240)
		{
			if(set_res == 0)
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_320x240;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_320x240;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_320x240;
		}
*/

#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
		if((strcmp(runtime[ch].sys.vendor, "HD-IP") == 0) ||
	       (strcmp(runtime[ch].sys.vendor, "IPCamera") == 0))
		{
			// Sunell Fisheye Exception
			if((strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/13") == 0) ||
			   (strcmp(runtime[ch].sys.stdver, "IPV56/60HDR/23") == 0) || 
			   (strcmp(runtime[ch].sys.stdver, "CBP360-IP") == 0))
			{
				if (w == 720 && h == 576)
				{
					if(stream_no == 1 && second_flag == 0)
					{			
						video_resol_default = NF_IPCAM_RES_720x576;	
						second_flag = 1;				
					}
					if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
								&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
					{
						runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_720x576;
						runtime[ch].video.resolution.supported |= NF_IPCAM_RES_720x576;
						set_res++;
					}
					runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_720x576;
				}
				else if (w == 720 && h == 480)
				{
					if(stream_no == 1 && second_flag == 0)
					{			
						video_resol_default = NF_IPCAM_RES_720x480;	
						second_flag = 1;				
					}
					if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
								&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
					{
						runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_720x480;
						runtime[ch].video.resolution.supported |= NF_IPCAM_RES_720x480;
						set_res++;
					}
					runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_720x480;
				}
				else if (w == 704 && h == 576)
				{
					if(stream_no == 1 && second_flag == 0)
					{
						video_resol_default = NF_IPCAM_RES_704x576;	
						second_flag = 1;
					}
					if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
								&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
					{
						runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_704x576;
						runtime[ch].video.resolution.supported |= NF_IPCAM_RES_704x576;
						set_res++;
					}
					runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_704x576;
				}
				else if (w == 704 && h == 480)
				{
					if(stream_no == 1 && second_flag == 0)
					{			
						video_resol_default = NF_IPCAM_RES_704x480;	
						second_flag = 1;				
					}
					if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
								&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
					{
						runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_704x480;
						runtime[ch].video.resolution.supported |= NF_IPCAM_RES_704x480;
						set_res++;
					}
					runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_704x480;
				}
			}
		}
#endif

		// 2nd stream 설정 시  1st stream 비율과 동일한 비율이어야 한다. (8.0차부터 적용)
		if (w == 704 && h == 480)
		{
			if(stream_no == 1 && second_flag == 0)
			{			
				video_resol_default = NF_IPCAM_RES_704x480;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_704x480;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_704x480;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_704x480;
		}
		else if (w == 640 && h == 480)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_640x480;	
				second_flag = 1;
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_640x480;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_640x480;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_640x480;
		}
		else if (w == 640 && h == 640)
		{
			if(stream_no == 1 && second_flag == 0)
			{			
				video_resol_default = NF_IPCAM_RES_640x640;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_640x640;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_640x640;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_640x640;
		}
		else if (w == 640 && h == 400)
		{
			if(stream_no == 1 && second_flag == 0)
			{			
				video_resol_default = NF_IPCAM_RES_640x400;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_640x400;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_640x400;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_640x400;
		}
		else if (w == 640 && h == 360)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_640x360;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_640x360;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_640x360;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_640x360;
		}
		else if (w == 640 && h == 352)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_640x352;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_640x352;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_640x352;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_640x352;
		}
		else if (w == 352 && h == 288)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_352x288;	
				second_flag = 1;			
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_352x288;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_352x288;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_352x288;
		}
		else if (w == 320 && h == 320)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_320x320;	
				second_flag = 1;			
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_320x320;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_320x320;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_320x320;
		}
		else if (w == 352 && h == 240)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_352x240;	
				second_flag = 1;				
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_352x240;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_352x240;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_352x240;
		}
		else if (w == 320 && h == 180)
		{
			if(stream_no == 1 && second_flag == 0)
			{
				video_resol_default = NF_IPCAM_RES_320x180;	
				second_flag = 1;
			}
			if(set_res == 0 && (second_resol_ratio.width_ratio == resol_ratio.width_ratio 
						&& second_resol_ratio.height_ratio == resol_ratio.height_ratio ))
			{
				runtime[ch].video.resolution.resolution[stream_no] = NF_IPCAM_RES_320x180;
				runtime[ch].video.resolution.supported |= NF_IPCAM_RES_320x180;
				set_res++;
			}
			runtime[ch].encoder.res_support[stream_no] |= NF_IPCAM_RES_320x180;
		}

		//not matching the first stream ratio -> 지원하는 2nd stream 중 가장 큰 resolution select
		if(second_flag != 0 && (i == (size-1) && runtime[ch].video.resolution.resolution[stream_no] == 0))
		{
			runtime[ch].video.resolution.resolution[1] = video_resol_default;
			runtime[ch].video.resolution.supported |= video_resol_default;
			set_res++;
		}

		if(runtime[ch].onvif.width == 0) runtime[ch].onvif.width = w;
		if(runtime[ch].onvif.height == 0) runtime[ch].onvif.height = h;

	}

	return set_res;
}

/**
 * @brief 주어진 해상도 목록으로 NVR 에 적합한 해상도 값을 찾는다.
 * @param resol 해상도 목록.
 * @param size 해상도 목록 크기.
 * @return 적합한 해상도를 찾았을 시 1.
 */
static int _nf_onvif_search_res(_nf_local_resolution* resol, const int size)
{
	mtable *runtime = get_runtime();

	int i, set_res = 0;

	// sort by pixel
	//qsort(resol, size, sizeof(_nf_local_resolution), _nf_onvif_compare_res);

	for(i = 0; i < size; i++)
	{
		int w = resol[i].width;
		int h = resol[i].height;
		int pixel = resol[i].pixel;


		// TODO resolution over HD
		if(pixel == 0 || pixel > 3840*2160) continue;

		// first stream only
		if(w == 3840 && h == 2160)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 3200 && h == 2400)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 3072 && h == 2048)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2880 && h == 2160)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 3200 && h == 1800)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2880 && h == 1800)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2992 && h == 1680)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2592 && h == 1944)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2592 && h == 1920)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2560 && h == 1920)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2560 && h == 1600)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2688 && h == 1520)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2560 && h == 1440)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2048 && h == 1536)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 2304 && h == 1296)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 1920 && h == 1080)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 1600&& h == 1200)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 1440 && h == 900)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 1280 && h == 1024)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if(w == 1280 && h == 720)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 1024 && h == 768)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 800 && h == 600)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 800 && h == 450)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 720 && h == 576)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 720 && h == 480)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 704 && h == 576)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 704 && h == 480)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 640 && h == 480)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 640 && h == 400)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 640 && h == 360)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 640 && h == 352)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 352 && h == 288)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 352 && h == 240)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
		else if (w == 320 && h == 180)
		{
			if(set_res == 0)
			{
				set_res++;
			}
		}
	}

	return set_res;
}

static void _get_width_height(const uint64_t resol, int* w, int* h)
{
	if(w == NULL || h == NULL) return;

	if(resol == NF_IPCAM_RES_3840x2160)
	{
		*w = 3840; *h = 2160;	
	}
	else if(resol == NF_IPCAM_RES_3200x2400)
	{
		*w = 3200; *h = 2400;	
	}
	else if(resol == NF_IPCAM_RES_3072x2048)
	{
		*w = 3072; *h = 2048;	
	}
	else if(resol == NF_IPCAM_RES_2880x2160)
	{
		*w = 2880; *h = 2160;	
	}
	else if(resol == NF_IPCAM_RES_3072x2048)
	{
		*w = 3072; *h = 2048;	
	}
	else if(resol == NF_IPCAM_RES_3200x1800)
	{
		*w = 3200; *h = 1800;	
	}
	else if(resol == NF_IPCAM_RES_2880x1800)
	{
		*w = 2880; *h = 1800;	
	}
	else if(resol == NF_IPCAM_RES_2992x1680)
	{
		*w = 2992; *h = 1680;	
	}
	else if(resol == NF_IPCAM_RES_2592x1944)
	{
		*w = 2592; *h = 1944;	
	}
	else if(resol == NF_IPCAM_RES_2592x1920)
	{
		*w = 2592; *h = 1920;	
	}
	else if(resol == NF_IPCAM_RES_2560x1920)
	{
		*w = 2560; *h = 1920;	
	}
	else if(resol == NF_IPCAM_RES_2560x1600)
	{
		*w = 2560; *h = 1600;	
	}
	else if(resol == NF_IPCAM_RES_2688x1520)
	{
		*w = 2688; *h = 1520;	
	}
	else if(resol == NF_IPCAM_RES_2592x1520)
	{
		*w = 2592; *h = 1520;	
	}
	else if(resol == NF_IPCAM_RES_2560x1440)
	{
		*w = 2560; *h = 1440;	
	}
	else if(resol == NF_IPCAM_RES_2048x1536)
	{
		*w = 2048; *h = 1536;	
	}
	else if(resol == NF_IPCAM_RES_2304x1296)
	{
		*w = 2304; *h = 1296;	
	}
	else if(resol == NF_IPCAM_RES_1920x1080)
	{
		*w = 1920; *h = 1080;	
	}
	else if(resol == NF_IPCAM_RES_1920x1080I)
	{
		*w = 1920; *h = 1080;
	}
	else if(resol == NF_IPCAM_RES_1280x720)
	{
		*w = 1280; *h = 720;	
	}
	else if(resol == NF_IPCAM_RES_1280x720I)
	{
		*w = 1280; *h = 720;	
	}
	else if(resol == NF_IPCAM_RES_1600x1200)
	{
		*w = 1600; *h = 1200;	
	}
	else if(resol == NF_IPCAM_RES_800x600)
	{
		*w = 800; *h = 600;		
	}
	else if(resol == NF_IPCAM_RES_1440x900)
	{
		*w = 1440; *h = 900;	
	}
	else if(resol == NF_IPCAM_RES_800x450)
	{
		*w = 800; *h = 450;		
	}
	else if(resol == NF_IPCAM_RES_640x400)
	{
		*w = 640; *h = 400;		
	}
	else if(resol == NF_IPCAM_RES_640x360)
	{
		*w = 640; *h = 360;		
	}
	else if(resol == NF_IPCAM_RES_640x360I)
	{
		*w = 640; *h = 360;		
	}
	else if(resol == NF_IPCAM_RES_320x180)
	{
		*w = 320; *h = 180;		
	}
	else if(resol == NF_IPCAM_RES_1280x1024)
	{
		*w = 1280; *h = 1024;	
	}
	else if(resol == NF_IPCAM_RES_1024x768)
	{
		*w = 1024; *h = 768;	
	}
	else if(resol == NF_IPCAM_RES_720x576)
	{
		*w = 720; *h = 576;		
	}
	else if(resol == NF_IPCAM_RES_720x480)
	{
		*w = 720; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_704x576)
	{
		*w = 704; *h = 576;		
	}
	else if(resol == NF_IPCAM_RES_704x480)
	{
		*w = 704; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_640x480)
	{
		*w = 640; *h = 480;		
	}
	else if(resol == NF_IPCAM_RES_640x352)
	{
		*w = 640; *h = 352;		
	}
	else if(resol == NF_IPCAM_RES_352x288)
	{
		*w = 352; *h = 288;		
	}
	else if(resol == NF_IPCAM_RES_352x240)
	{
		*w = 352; *h = 240;		
	}
	else if(resol == NF_IPCAM_RES_320x240)
	{
		*w = 320; *h = 240;		
	}
	else if(resol == NF_IPCAM_RES_3000x3000)
	{
		*w = 3000; *h = 3000;		
	}
	else if(resol == NF_IPCAM_RES_2048x2048)
	{
		*w = 2048; *h = 2048;		
	}
	else if(resol == NF_IPCAM_RES_1280x1280)
	{
		*w = 1280; *h = 1280;		
	}
	else if(resol == NF_IPCAM_RES_640x640)
	{
		*w = 640; *h = 640;		
	}
	else if(resol == NF_IPCAM_RES_320x320)
	{
		*w = 320; *h = 320;		
	}
	else
	{
		*w = 0; *h = 0;			
	}

}

/**
 * @brief 주어진 해상도에 대한 비율을 구한다.
 * @param resol_ratio  해상도 비율.
 * @param w  width.
 * @param h  height.
 * @return 0 - 성공, 기타 - 실패.
 */
static int _get_stream_ratio(_nf_local_resolution_ratio *resol_ratio, int w, int h)
{
	int width, height, max, min, gcd, temp;

	if(w <= 0 || h <= 0)
		return -1;

	width = w;
	height = h;
	if(width < height)
	{ 
		max = width;      
		min = height;
	}
	else
	{
		max = height;
		min = width;
	}

	while(max%min != 0) 
	{  
		temp = max % min;  
		max = min;         
		min = temp;        

	}

	gcd = min;             

	resol_ratio->width_ratio = width/gcd;      
	resol_ratio->height_ratio = height/gcd;

	return 0;
}

ONVIF_MSG _onvif_media_get_snapshot_uri(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char *profile_token, char *uri);
ONVIF_API nf_onvif_media_get_snapshot_uri(char *xaddr_media, char *xaddr_device, int auth, char* u, char* p, char* token, char* rtn_uri)
{
	int i = 0;
	int rtn = (-1);

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR, "START\n");

	auth_info.auth_method = auth;
	auth_info.username = u;
	auth_info.password = p;
	auth_info.endpoint = xaddr_device;
	rtn = _onvif_media_get_snapshot_uri(&auth_info, xaddr_media, token, rtn_uri);

ends_label:
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}

ONVIF_MSG _onvif_media_get_snapshot_uri(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char *profile_token, char *uri)
{
	int i = 0;
	int rtn = (-1);

	struct soap *soap;
    struct _media__GetSnapshotUri req;
	struct _media__GetSnapshotUriResponse res;

	NF_ONVIF_DBG(MAJOR, "START\n");

	if(profile_token == NULL)
	{
		return  -1;
	}

	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	soap->recv_timeout = 10;

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			SOAP_ERROR_PRINT(soap);
			goto ends_label;
		}
	}

	req.ProfileToken = soap_malloc(soap, strlen(profile_token) + 1);
	strcpy(req.ProfileToken, profile_token);

	rtn = soap_call___media__GetSnapshotUri
			(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}
	if(res.MediaUri == NULL)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}
	if(res.MediaUri->Uri == NULL)
	{
		NF_ONVIF_DBG(ERROR, "stream uri get fail\n");
		goto ends_label;
	}

	snprintf(uri, 256, "%s", res.MediaUri->Uri);
	NF_ONVIF_DBG(MINOR, "profile %s : uri %s\n", profile_token, uri);

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	NF_ONVIF_DBG(MAJOR, "END\n");

	return rtn;
}


#endif //__NF_ONVIF_MEDIA_C__
