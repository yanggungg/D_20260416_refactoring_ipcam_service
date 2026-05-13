	/**
 * @file nf_onvif_ptz.c
 * @brief ONVIF PTZ 구현
 * @author
 * @date 2012-09-10
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_ONVIF_PTZ_C__
#define __NF_ONVIF_PTZ_C__

#include <stdsoap2.h>
#include <onvifH.h>

#include <nf_ipcam_defs.h>


#if 1
#define NF_ONVIF_DBG printf
#else
#define NF_ONVIF_DBG while(0) printf
#endif
#define D_SECS(a) ((a).tv_sec),(((a).tv_nsec)/1000)

/** @def MULTIPLIER_0_TO_1
 *  @brief the range should be defined as the full range of the PTZ unit normalized to the range -1 to 1(onvif ptz space)
 */
#define MULTIPLIER_0_TO_1	10.0

struct _T_ONVIF_AUXILIARY_COMMAND
{
	char profile_token[64];
	char command[32];
};

/* Static msg exchange funcs */

ONVIF_MSG _nf_onvif_ptz_get_nodes(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token, ptz_t_onvif* ptz_onvif);

//ONVIF_MSG _nf_onvif_ptz_get_node(const char*, const char*, const char*, const char*);

ONVIF_MSG _nf_onvif_ptz_get_configurations(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* nodeToken, char* configToken);

//ONVIF_MSG _nf_onvif_ptz_get_configuration(const char*, const char*, const char*, const char*);

ONVIF_MSG _nf_onvif_ptz_get_configuration_options(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token, ptz_t_onvif* ptz_onvif);

ONVIF_MSG _nf_onvif_ptz_set_configuration(int auth, const char* endpoint, const char* user, const char* pass, const char* nodeToken, const char* configToken);

ONVIF_MSG _nf_onvif_ptz_absolute_move(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token, ptz_info_onvif info );

ONVIF_MSG _nf_onvif_ptz_relative_move(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token, ptz_info_onvif info );

ONVIF_MSG _nf_onvif_ptz_continuous_move(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token, ptz_info_onvif info, long long int timeout );

ONVIF_MSG _nf_onvif_ptz_continuous_stop(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token);

ONVIF_MSG _nf_onvif_ptz_stop(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token);

//ONVIF_MSG _nf_onvif_ptz_get_status(const char*, const char*, const char*, const char*, struct tt__PTZStatus*);

ONVIF_MSG _nf_onvif_ptz_set_preset(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* profileToken, char* presetToken, char* presetName);

ONVIF_MSG _nf_onvif_ptz_get_presets(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token, ptz_t_onvif* ptz_onvif);

ONVIF_MSG _nf_onvif_ptz_goto_preset(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* profileToken, const char* presetToken);

ONVIF_API _nf_onvif_ptz_remove_preset(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* profileToken, const char* presetToken);

ONVIF_MSG _nf_onvif_ptz_goto_home_position(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, const char* token);

//ONVIF_MSG _nf_onvif_ptz_set_home_position(const char*, const char*, const char*, const char*);

ONVIF_MSG _nf_onvif_ptz_send_auxiliary_command(ipcam_onvif_auth_info_t *auth_info, const char* endpoint, struct _T_ONVIF_AUXILIARY_COMMAND *auxiliary);

ONVIF_MSG _nf_onvif_ptz_get_preset_tours(int auth, const char* endpoint, const char* user, const char* pass, const char* token);

ONVIF_MSG _nf_onvif_create_itx_preset_name(int preset_seq, char *tempName);

//ONVIF_MSG _nf_onvif_ptz_get_preset_tour(const char*, const char*, const char*, const char*, struct tt__PresetTour*);

// convert float-range into value(min, max, multiplier)
//static void _nf_set_value_with_mux(const struct tt__FloatRange, values*);

/**
 * @brief Preset 번호로 Preset index를 구한다.
 * @param ch 채널 번호.
 * @param preset_no NVR이 관리하는 preset no(0 ~ 239).
 * @return Preset index.
 */
static int _nf_get_onvif_preset_seq(int ch, int preset_no)
{
	int rtn = -1, i;
	mtable *runtime = get_runtime();
	for(i = 0; i < runtime[ch].preset.preset_cnt; i++)
	{
		if(runtime[ch].preset.preset_number[i] == preset_no) rtn = i;
	}

	return rtn;
}

/* -------------   ONVIF related APIs for the IPX scenario   --------------- */
/**
 * @brief ONVIF PTZ를 NVR에서 지원 가능한지 조회한다.
 * @param ch 채널 번호.
 * @return 0 success, -2 fail, -9 not support
 *
 * 1. GetNodes를 통해 nodetoken을 구한다.
 * 2. nodeToken을 통해 GetConfigurations를 실행하고 Configuration Token을 구한다.
 * 3. GetConfigurationOption를 통해 ptz지원여부를 설정한다.
 */
ONVIF_API nf_onvif_ptz_get_support(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	char nodeToken[64];
	char configToken[64];

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), (-1));
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	memset(nodeToken, 0x00, 64);
	memset(configToken, 0x00, 64);

	memset(&runtime[ch].ptz_onvif, 0x00, sizeof(ptz_t_onvif));

	rtn = _nf_onvif_ptz_get_nodes(&auth_info, endpoint, nodeToken, &runtime[ch].ptz_onvif);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) get nodes not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			rtn = -2;
		}
		goto ends_label;
	}

	rtn = _nf_onvif_ptz_get_configurations(&auth_info, endpoint, nodeToken, configToken);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) get configuration not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			rtn = -2;
		}
		goto ends_label;
	}

	rtn = _nf_onvif_ptz_get_configuration_options(&auth_info, endpoint, configToken, &runtime[ch].ptz_onvif);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) get configuration option not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{
			rtn = -2;
		}
		goto ends_label;
	}

	/*rtn = _nf_onvif_ptz_set_configuration(
			runtime[ch].onvif.auth_method,
			endpoint, runtime[ch].username, runtime[ch].password, nodeToken, configToken);*/

	//memset(&runtime[ch].ptz, 0x00, sizeof(ptz_t));
	if(runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_CONTINUOUS_PANTILT)
	{
		//NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | support Continuous Pan/Tilt !\n",
		//												__FUNCTION__);
		runtime[ch].func |= NF_IPCAM_FUNC_PTZ;
		runtime[ch].ptz.supported |= PTZ_SETUP_PAN;
		runtime[ch].ptz.supported |= PTZ_SETUP_TILT;
		runtime[ch].ptz.pan.min = runtime[ch].ptz_onvif.continuous_pan.min;
		runtime[ch].ptz.pan.max = runtime[ch].ptz_onvif.continuous_pan.max;
		runtime[ch].ptz.tilt.min = runtime[ch].ptz_onvif.continuous_tilt.min;
		runtime[ch].ptz.tilt.max = runtime[ch].ptz_onvif.continuous_tilt.max;
	}
	if(runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_CONTINUOUS_ZOOM)
	{
		//NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | support Continuous Zoom !\n",
		//												__FUNCTION__);
		runtime[ch].image.supported |= NF_IPCAM_IMAGE_ZOOM;
		runtime[ch].ptz.supported |= PTZ_SETUP_ZOOM;
		runtime[ch].ptz.zoom.min = runtime[ch].ptz_onvif.continuous_zoom.min;
		runtime[ch].ptz.zoom.max = runtime[ch].ptz_onvif.continuous_zoom.max;
	}
	if(runtime[ch].ptz_onvif.support_ptz & NF_IPCAM_PTZ_ONVIF_PRESET)
	{
		//NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | support Set Preset !\n",
		//												__FUNCTION__);
		runtime[ch].ptz.supported |= PTZ_SETUP_PRESET;
	}


ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

/**
 * @brief ONVIF PTZ(pan, tilt, zoom) 이동을 실시한다.
 * @param ch 채널 번호.
 * @param info PTZ 이동 정보 struct.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_move(int ch, ptz_info_onvif info)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	switch(info.mode)
	{
		case NF_IPCAM_PTZ_MODE_ONVIF_ABSOLUTE:
			rtn = _nf_onvif_ptz_absolute_move(
					&auth_info,	endpoint, runtime[ch].onvif.profile_token[0], info);
			break;

		case NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE:
			rtn = _nf_onvif_ptz_relative_move(
					&auth_info,	endpoint, runtime[ch].onvif.profile_token[0], info);
			break;

		case NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS:
			/* revert */
			//info.speed_pan *= -1;
			info.speed_tilt *= -1;
			rtn = _nf_onvif_ptz_continuous_move(
					&auth_info,	endpoint, runtime[ch].onvif.profile_token[0], info, 0/*runtime[ch].ptz_onvif.timeout.max*/);
			break;
	}

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) move not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{

		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	//return rtn;
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF PTZ 이동을 멈춘다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_stop(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0;

	ipcam_onvif_auth_info_t auth_info;
	
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	/*rtn = _nf_onvif_ptz_stop(
			runtime[ch].onvif.auth_method,
			endpoint, runtime[ch].username, runtime[ch].password, runtime[ch].onvif.profile_token[0]);*/

	rtn = _nf_onvif_ptz_continuous_stop(
			&auth_info,
			endpoint, runtime[ch].onvif.profile_token[0]);


	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) stop not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{

		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	//return rtn;
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF PTZ home 위치로 이동한다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_goto_home(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_ptz_goto_home_position(
			&auth_info,	endpoint, runtime[ch].onvif.profile_token[0]);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) gotohome not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{

		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	//return rtn;
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF PTZ preset정보를 조회한다.
 * @param ch 채널 번호.
 * @return 0 - 성공, 기타 - 실패.
 */
ONVIF_API nf_onvif_ptz_get_preset(int ch)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), (-1));
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, (-1));

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_ptz_get_presets(
			&auth_info,
			endpoint, runtime[ch].onvif.profile_token[0], &runtime[ch].ptz_onvif);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) get presets not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
		else
		{

		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn;
}

/**
 * @brief ONVIF PTZ preset 하나를 삭제한다.
 * @param ch 채널 번호.
 * @param preset_no Preset 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_remove_preset(int ch, int preset_no)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	int presetSeq = preset_no -1;
	if(!(strlen(runtime[ch].ptz_onvif.preset_token[presetSeq]) > 0))
		presetSeq = -1;

	g_return_val_if_fail(presetSeq != -1, IPCAM_SETUP_RTN_FAILED);

	char* presetToken = runtime[ch].ptz_onvif.preset_token[presetSeq];
	g_return_val_if_fail(strlen(presetToken) == 0, IPCAM_SETUP_RTN_FAILED);

	rtn = _nf_onvif_ptz_remove_preset(
			&auth_info,	endpoint, runtime[ch].onvif.profile_token[0], presetToken);

	if (rtn != 0)
	{
		if(rtn == -9)
		{
			clock_gettime(CLOCK_REALTIME, &now_time);
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) remove preset not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
	}
	else
	{
		memset(runtime[ch].preset.preset_token[presetSeq], 0x00, 64);
		memset(runtime[ch].ptz_onvif.preset_token[presetSeq], 0x00, 64);
		runtime[ch].preset.preset_number[presetSeq] = 0;
		runtime[ch].preset.preset_cnt--;
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/**
 * @brief ONVIF PTZ preset을 현 위치에 추가한다.
 * @param ch 채널 번호.
 * @param preset_no Preset 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_set_preset(int ch, int preset_no)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;
	
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	char tempToken[64];
	char tempName[64];
	int isUpdate = 0;
	int isITXname = 0;
	char *ptr = NULL;

	memset(tempToken, 0x00, 64);
	memset(tempName, 0x00, 64);

	int presetSeq = preset_no - 1;
	int presetITXCnt = 0;
	int index_tester = 0;

	//case 1. preset token is empty 
	if(!(strlen(runtime[ch].ptz_onvif.preset_token[presetSeq]) > 0))
	{
		isUpdate = 1;
		printf("\e[31m >>>> [%s][%d]<<<<<< \e[0m\n", __FUNCTION__, __LINE__);
	}

	//case 2. preset token is found 
	if(strlen(runtime[ch].ptz_onvif.preset_token[presetSeq]) > 0)
	{
		//case 2-1. preset name is ITX form.
		if (strstr(runtime[ch].ptz_onvif.preset_name[i], "XB") != NULL)
		{
			isITXname = 1;
			snprintf(tempToken, 64, "%s", runtime[ch].ptz_onvif.preset_token[presetSeq]);
			snprintf(tempName, 64, "%s", runtime[ch].ptz_onvif.preset_name[presetSeq]);
	}
		//case 2-2. preset name is empty or others form
	else
	{
			isITXname = 0;
			snprintf(tempToken, 64, "%s", runtime[ch].ptz_onvif.preset_token[presetSeq]);
		}
	}

	//case 2.  code
	if(isITXname == 0)
	{
		int rtn = 0;
		rtn = _nf_onvif_create_itx_preset_name(presetSeq, tempName); 
	}

	NF_ONVIF_DBG("update : %d presetToken : %s\n", isUpdate, tempToken);

	rtn = _nf_onvif_ptz_set_preset(
			&auth_info,	endpoint, runtime[ch].onvif.profile_token[0], tempToken, tempName);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) set preset not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
	}
	else
	{
		//case 1. token get and set
		if(isUpdate)
		{
			snprintf(runtime[ch].ptz_onvif.preset_token[presetSeq], 64, "%s", tempToken);
			printf("\e[31m >>> tempPresets.token : %s \e[0m\n", runtime[ch].ptz_onvif.preset_token[presetSeq]);

			//runtime[ch].preset.preset_number[runtime[ch].preset.preset_cnt] = preset_no;
			//runtime[ch].preset.preset_cnt++;
		}

		runtime[ch].ptz_onvif.preset_num[presetSeq] = presetSeq;
		printf("\e[31m >>> itx_preset_index : %d \e[0m\n", presetSeq);
	}

ends_label:
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	//return rtn;
	return IPCAM_SETUP_RTN_DONE;
}


ONVIF_MSG _nf_onvif_create_itx_preset_name(int preset_seq, char *tempName)
{
	int rtn = 0;
	char *ptr = NULL;

	snprintf(tempName, 64, "%s", "XB");
	ptr = strstr(tempName, "XB");
	ptr = ptr + 2;

	snprintf(ptr, 4, "%03d", preset_seq);
	strcat(tempName, "NAME");

	NF_ONVIF_DBG("[%s:%d] presetName : %s\n", __FUNCTION__, __LINE__, tempName);

	return rtn;
}

/**
 * @brief ONVIF PTZ preset 위치로 이동한다.
 * @param ch 채널 번호.
 * @param preset_no Preset 번호.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
ONVIF_API nf_onvif_ptz_goto_preset(int ch, int preset_no)
{
	char endpoint[256];
	mtable *runtime = get_runtime();
	struct timespec now_time;
	int rtn = 0, i = 0;

	ipcam_onvif_auth_info_t auth_info;
	
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start(CH %d)\n",
			D_SECS(now_time),
			__FUNCTION__, ch
			);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];


	int presetSeq;
	//presetSeq is (preset_no -1)
	presetSeq = preset_no -1;

	//arranged preset number
	if(presetSeq < 0){
		presetSeq = -1;
	}else if(!(strlen(runtime[ch].ptz_onvif.preset_token[presetSeq]) > 0)){
		presetSeq = -1;
	}

	g_return_val_if_fail(presetSeq != -1, IPCAM_SETUP_RTN_FAILED);
	char* presetToken = runtime[ch].ptz_onvif.preset_token[presetSeq];

	g_return_val_if_fail(presetToken != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(strlen(presetToken) > 0, IPCAM_SETUP_RTN_FAILED);
	//if(!(presetToken != NULL && strlen(presetToken) > 0)) return IPCAM_SETUP_RTN_FAILED;

	NF_ONVIF_DBG("%s | goto %d preset (seq %d token %s)\n", __FUNCTION__, preset_no, presetSeq, presetToken);

	rtn = _nf_onvif_ptz_goto_preset(
			&auth_info, endpoint, runtime[ch].onvif.profile_token[0], presetToken);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(rtn == -9)
		{
			NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | WARN: ch(%d) goto preset not support(%d)\n",
					D_SECS(now_time),
					__FUNCTION__,
					ch,
					rtn
					);
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
	//return rtn;
	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

ONVIF_API nf_onvif_ptz_send_auxiliary_command(int ch, char* command)
{
	int rtn;
	char endpoint[256];
	struct _T_ONVIF_AUXILIARY_COMMAND auxiliary;
	mtable *runtime = get_runtime();

	ipcam_onvif_auth_info_t auth_info;

	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].username != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].password != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_PTZ), IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ] != NULL, IPCAM_SETUP_RTN_FAILED);

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	strncpy(auxiliary.profile_token, runtime[ch].onvif.profile_token[0], 64);
	strncpy(auxiliary.command, command, 32);

	rtn = _nf_onvif_ptz_send_auxiliary_command(&auth_info, endpoint, &auxiliary);

	if(rtn != 0)
	{
		NF_ONVIF_DBG("[%s] faild\n", __func__);
	}

	return rtn == 0 ? IPCAM_SETUP_RTN_DONE : IPCAM_SETUP_RTN_FAILED;
}

/* XXX-------------   ONVIF MSG exchanging methods - Internal use   --------------- */
ONVIF_MSG _nf_onvif_ptz_get_nodes(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, char* token, ptz_t_onvif* ptz_onvif)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _ptz__GetNodes req;
	struct _ptz__GetNodesResponse res;

	char *action = "GetNodes";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	memset(ptz_onvif, 0x00, sizeof(ptz_t_onvif));

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GetNodes));

	rtn = soap_call___ptz__GetNodes(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	if (res.PTZNode == NULL || res.__sizePTZNode == 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: PTZ node null \n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = (-9);
		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetNodes Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

	// todo : multiple ptz node
	//NF_ONVIF_DBG("has %d ptz nodes\n", res.__sizePTZNode);
	//for(i = 0; i < res.__sizePTZNode; i++)
	for(i = 0; i < 1; i++)
	{
		NF_ONVIF_DBG("%d th ptz node -------------\n", i);
		if(res.PTZNode[i].Name != NULL)
		{
			//NF_ONVIF_DBG(" name : %s\n", res.PTZNode[i].Name);
		}
		if(res.PTZNode[i].token != NULL)
		{
			//NF_ONVIF_DBG(" token : %s\n", res.PTZNode[i].token);
			if(token == NULL || strlen(token) == 0)
			{
				snprintf(token, 64, "%s", res.PTZNode[i].token);
			}
		}
		//NF_ONVIF_DBG(" max no. of preset : %d\n", res.PTZNode[i].MaximumNumberOfPresets);
		if(res.PTZNode[i].MaximumNumberOfPresets > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_PRESET;

			//Onvif cam : supported preset cnt 
			ptz_onvif->supported_preset_cnt = res.PTZNode[i].MaximumNumberOfPresets;
		}
		//NF_ONVIF_DBG(" home support : %2d\n", res.PTZNode[i].HomeSupported);
		if(xsd__boolean__true_ == res.PTZNode[i].HomeSupported)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_GOTOHOME;
		}
		if(res.PTZNode[i].FixedHomePosition != NULL)
		{
			//NF_ONVIF_DBG(" fixed home : %2d\n", *res.PTZNode[i].FixedHomePosition);
			if(xsd__boolean__false_ == *res.PTZNode[i].FixedHomePosition)
			{
				ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_SETHOME;
			}
		}

		if(res.PTZNode[i].AuxiliaryCommands != NULL && res.PTZNode[i].__sizeAuxiliaryCommands > 0)
		{
			char *Command;
			int index;
			int size;
			size = res.PTZNode[i].__sizeAuxiliaryCommands;

			if(size > MAX_AUXILIARY_COMMANDS)
			{
				size = MAX_AUXILIARY_COMMANDS;
			}

			ptz_onvif->auxiliary.size = size;
			for(index = 0; index < size; index++)
			{
				strncpy(ptz_onvif->auxiliary.commands[index], res.PTZNode[i].AuxiliaryCommands[index], 32);
			}
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_AUXILIARY;

		}
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_get_configurations(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char *nodeToken, char* configToken)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _ptz__GetConfigurations req;
	struct _ptz__GetConfigurationsResponse res;

	char *action = "GetConfigurations";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}
	memset(&req, 0x00, sizeof(struct _ptz__GetConfigurations));

	rtn = soap_call___ptz__GetConfigurations(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	if (res.PTZConfiguration == NULL || res.__sizePTZConfiguration == 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: PTZ configuration null \n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = (-9);
		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetConfigurations Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

	for(i = 0; i < res.__sizePTZConfiguration; i++)
	{
		//NF_ONVIF_DBG("%d th ptz configuration ------------------------\n", i);
		if(res.PTZConfiguration[i].Name != NULL)
		{
			//NF_ONVIF_DBG( " name : %s\n", res.PTZConfiguration[i].Name);
		}
		//NF_ONVIF_DBG(" use count : %d\n", res.PTZConfiguration[i].UseCount);
		if(res.PTZConfiguration[i].NodeToken != NULL)
		{
			//NF_ONVIF_DBG( " nodeToken : %s\n", res.PTZConfiguration[i].NodeToken);
			if(strcmp(nodeToken, res.PTZConfiguration[i].NodeToken) == 0)
			{
				if(res.PTZConfiguration[i].token != NULL && strlen(configToken) == 0)
				{
					snprintf(configToken, 64, "%s", res.PTZConfiguration[i].token);
					NF_ONVIF_DBG( "\033[1;40;32m%s\033[0m | take token (%s) as default!\n", __FUNCTION__, configToken);
				}
			}
		}
#if 0
		if(res.PTZConfiguration[i].token != NULL)
		{
			NF_ONVIF_DBG( " token : %s\n", res.PTZConfiguration[i].token);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" absolute pt space : %s\n", res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" absolute z space : %s\n", res.PTZConfiguration[i].DefaultAbsoluteZoomPositionSpace);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" relative pt space : %s\n", res.PTZConfiguration[i].DefaultRelativePanTiltTranslationSpace);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" relative z space : %s\n", res.PTZConfiguration[i].DefaultRelativeZoomTranslationSpace);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" continuous pt space : %s\n", res.PTZConfiguration[i].DefaultContinuousPanTiltVelocitySpace);
		}
		if(res.PTZConfiguration[i].DefaultAbsolutePantTiltPositionSpace != NULL)
		{
			NF_ONVIF_DBG(" continuous z space : %s\n", res.PTZConfiguration[i].DefaultContinuousZoomVelocitySpace);
		}
		if(res.PTZConfiguration[i].DefaultPTZSpeed != NULL)
		{
			NF_ONVIF_DBG(" default ptz speed : \n");
			if(res.PTZConfiguration[i].DefaultPTZSpeed->PanTilt != NULL)
			{
				NF_ONVIF_DBG("  pt speed x : %f y : %f\n", res.PTZConfiguration[i].DefaultPTZSpeed->PanTilt->x, res.PTZConfiguration[i].DefaultPTZSpeed->PanTilt->y);
				if(res.PTZConfiguration[i].DefaultPTZSpeed->PanTilt->space != NULL)
				{
					NF_ONVIF_DBG("  pt space : %s\n", res.PTZConfiguration[i].DefaultPTZSpeed->PanTilt->space);
				}
			}
			if(res.PTZConfiguration[i].DefaultPTZSpeed->Zoom != NULL)
			{
				NF_ONVIF_DBG("  z speed x : %f \n", res.PTZConfiguration[i].DefaultPTZSpeed->Zoom->x);
				if(res.PTZConfiguration[i].DefaultPTZSpeed->Zoom->space != NULL)
				{
					NF_ONVIF_DBG("  z space : %s\n", res.PTZConfiguration[i].DefaultPTZSpeed->Zoom->space);
				}
			}
		}
		if(res.PTZConfiguration[i].DefaultPTZTimeout != NULL)
		{
			NF_ONVIF_DBG(" ptz timeout : %ld\n", *res.PTZConfiguration[i].DefaultPTZTimeout);
		}
		if(res.PTZConfiguration[i].PanTiltLimits != NULL)
		{
			if(res.PTZConfiguration[i].PanTiltLimits->Range != NULL)
			{
				NF_ONVIF_DBG(" pt limit : \n");
				if(res.PTZConfiguration[i].PanTiltLimits->Range->URI != NULL)
				{
					NF_ONVIF_DBG("  range uri : %s\n", res.PTZConfiguration[i].PanTiltLimits->Range->URI);
				}
				if(res.PTZConfiguration[i].PanTiltLimits->Range->XRange != NULL)
				{
					NF_ONVIF_DBG("  xrange : %f ~ %f\n", res.PTZConfiguration[i].PanTiltLimits->Range->XRange->Min, res.PTZConfiguration[i].PanTiltLimits->Range->XRange->Max);
				}
				if(res.PTZConfiguration[i].PanTiltLimits->Range->YRange != NULL)
				{
					NF_ONVIF_DBG("  yrange : %f ~ %f\n", res.PTZConfiguration[i].PanTiltLimits->Range->YRange->Min, res.PTZConfiguration[i].PanTiltLimits->Range->YRange->Max);
				}
			}
		}
		if(res.PTZConfiguration[i].ZoomLimits != NULL)
		{
			if(res.PTZConfiguration[i].ZoomLimits->Range != NULL)
			{
				NF_ONVIF_DBG(" z limit : \n");
				if(res.PTZConfiguration[i].ZoomLimits->Range->URI != NULL)
				{
					NF_ONVIF_DBG("  range uri : %s\n", res.PTZConfiguration[i].ZoomLimits->Range->URI);
				}
				if(res.PTZConfiguration[i].ZoomLimits->Range->XRange != NULL)
				{
					NF_ONVIF_DBG("  xrange : %f ~ %f\n", res.PTZConfiguration[i].ZoomLimits->Range->XRange->Min, res.PTZConfiguration[i].ZoomLimits->Range->XRange->Max);
				}
			}
		}
/*
		if(res.PTZConfiguration[i].Extension != NULL)
		{
			NF_ONVIF_DBG(" extension : \n");
			if(res.PTZConfiguration[i].Extension->PTControlDirection != NULL)
			{
				NF_ONVIF_DBG("  ptcontrol direction : \n");
				if(res.PTZConfiguration[i].Extension->PTControlDirection->EFlip != NULL)
				{
					NF_ONVIF_DBG("   eflip mode : %2d\n", res.PTZConfiguration[i].Extension->PTControlDirection->EFlip->Mode);
				}
				if(res.PTZConfiguration[i].Extension->PTControlDirection->Reverse != NULL)
				{
					NF_ONVIF_DBG("   reverse mode : %2d\n", res.PTZConfiguration[i].Extension->PTControlDirection->Reverse->Mode);
				}
			}
		}
*/
#endif
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_get_configuration_options(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char *token, ptz_t_onvif* ptz_onvif)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);
	struct _ptz__GetConfigurationOptions req;
	struct _ptz__GetConfigurationOptionsResponse res;

	char *action = "GetConfigurationOptions";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GetConfigurationOptions));
	req.ConfigurationToken = soap_malloc(soap, strlen(token) + 1);
	strcpy(req.ConfigurationToken, token);

	rtn = soap_call___ptz__GetConfigurationOptions(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	if (res.PTZConfigurationOptions == NULL)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: PTZ PTZConfigurationOptions null \n",
				D_SECS(now_time),
				__FUNCTION__
				);
		rtn = (-9);
		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetConfigurationOptions Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/
	if(res.PTZConfigurationOptions->Spaces != NULL)
	{
		if(res.PTZConfigurationOptions->Spaces->__sizeAbsolutePanTiltPositionSpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_ABSOLUTE_PANTILT;
			if(res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].XRange != NULL)
			{
				ptz_onvif->absolute_pan.min = res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_pan.max = res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_pan.multiplier = MULTIPLIER_0_TO_1;
			}
			if(res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].YRange != NULL)
			{
				ptz_onvif->absolute_tilt.min = res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].YRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_tilt.max = res.PTZConfigurationOptions->Spaces->AbsolutePanTiltPositionSpace[0].YRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_tilt.multiplier = MULTIPLIER_0_TO_1;
			}
		}
		if(res.PTZConfigurationOptions->Spaces->__sizeAbsoluteZoomPositionSpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_ABSOLUTE_ZOOM;
			if(res.PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace[0].XRange != NULL)
			{
				ptz_onvif->absolute_zoom.min = res.PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_zoom.max = res.PTZConfigurationOptions->Spaces->AbsoluteZoomPositionSpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->absolute_zoom.multiplier = MULTIPLIER_0_TO_1;
			}
		}
		if(res.PTZConfigurationOptions->Spaces->__sizeRelativePanTiltTranslationSpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_RELATIVE_PANTILT;
			if(res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].XRange != NULL)
			{
				ptz_onvif->relative_pan.min = res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_pan.max = res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_pan.multiplier = MULTIPLIER_0_TO_1;
			}
			if(res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].YRange != NULL)
			{
				ptz_onvif->relative_tilt.min = res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].YRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_tilt.max = res.PTZConfigurationOptions->Spaces->RelativePanTiltTranslationSpace[0].YRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_tilt.multiplier = MULTIPLIER_0_TO_1;
			}
		}
		if(res.PTZConfigurationOptions->Spaces->__sizeRelativeZoomTranslationSpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_RELATIVE_ZOOM;
			if(res.PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace[0].XRange != NULL)
			{
				ptz_onvif->relative_zoom.min = res.PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_zoom.max = res.PTZConfigurationOptions->Spaces->RelativeZoomTranslationSpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->relative_zoom.multiplier = MULTIPLIER_0_TO_1;
			}
		}
		if(res.PTZConfigurationOptions->Spaces->__sizeContinuousPanTiltVelocitySpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_CONTINUOUS_PANTILT;
			if(res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].XRange != NULL)
			{
				ptz_onvif->continuous_pan.min = res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_pan.max = res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_pan.multiplier = MULTIPLIER_0_TO_1;
			}
			if(res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].YRange != NULL)
			{
				ptz_onvif->continuous_tilt.min = res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].YRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_tilt.max = res.PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0].YRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_tilt.multiplier = MULTIPLIER_0_TO_1;
			}
		}
		if(res.PTZConfigurationOptions->Spaces->__sizeContinuousZoomVelocitySpace > 0)
		{
			ptz_onvif->support_ptz |= NF_IPCAM_PTZ_ONVIF_CONTINUOUS_ZOOM;
			if(res.PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace[0].XRange != NULL)
			{
				ptz_onvif->continuous_zoom.min = res.PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace[0].XRange->Min * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_zoom.max = res.PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace[0].XRange->Max * MULTIPLIER_0_TO_1;
				ptz_onvif->continuous_zoom.multiplier = MULTIPLIER_0_TO_1;
			}
		}
	}
	if(res.PTZConfigurationOptions->PTZTimeout != NULL)
	{
		//NF_ONVIF_DBG(" ptz timeout range : %ld ~ %ld\n", res.PTZConfigurationOptions->PTZTimeout->Min, res.PTZConfigurationOptions->PTZTimeout->Max);
		ptz_onvif->timeout.min = res.PTZConfigurationOptions->PTZTimeout->Min;
		ptz_onvif->timeout.max = res.PTZConfigurationOptions->PTZTimeout->Max;
	}



ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}


ONVIF_MSG _nf_onvif_ptz_set_configuration(int auth, const char* endpoint, const char* user, const char* pass, const char* nodeToken, const char* configToken)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__SetConfiguration req;
	struct _ptz__SetConfigurationResponse res;

	char *action = "SetConfiguration";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__SetConfiguration));

	req.ForcePersistence = xsd__boolean__true_;
	req.PTZConfiguration = soap_malloc(soap, sizeof(struct tt__PTZConfiguration));
	memset(req.PTZConfiguration, 0x00, sizeof(struct tt__PTZConfiguration));

	req.PTZConfiguration->token = soap_malloc(soap, strlen(configToken) + 1);
	strcpy(req.PTZConfiguration->token, configToken);

	req.PTZConfiguration->NodeToken = soap_malloc(soap, strlen(nodeToken) + 1);
	strcpy(req.PTZConfiguration->NodeToken, nodeToken);

	//req.PTZConfiguration->DefaultPTZTimeout = soap_malloc(soap, sizeof(long long int));
	//*req.PTZConfiguration->DefaultPTZTimeout = 0;

	rtn = soap_call___ptz__SetConfiguration(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | SetConfiguration Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_absolute_move(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token, ptz_info_onvif info)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__AbsoluteMove req;
	struct _ptz__AbsoluteMoveResponse res;

	char *action = "AbsoluteMove";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__AbsoluteMove));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.Position = soap_malloc(soap, sizeof(struct tt__PTZVector));
	req.Position->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	req.Position->PanTilt->x = roundi(info.absolute_pan / MULTIPLIER_0_TO_1);
	req.Position->PanTilt->y = roundi(info.absolute_tilt / MULTIPLIER_0_TO_1);
	req.Position->PanTilt->space = NULL;

	req.Position->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
	req.Position->Zoom->x = roundi(info.absolute_zoom / MULTIPLIER_0_TO_1);
	req.Position->Zoom->space = NULL;

	req.Speed = soap_malloc(soap, sizeof(struct tt__PTZSpeed));
	req.Speed->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	req.Speed->PanTilt->x = roundi(info.speed_pan / MULTIPLIER_0_TO_1);
	req.Speed->PanTilt->y = roundi(info.speed_tilt / MULTIPLIER_0_TO_1);
	req.Speed->PanTilt->space = NULL;

	req.Speed->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
	req.Speed->Zoom->x = roundi(info.speed_zoom / MULTIPLIER_0_TO_1);
	req.Speed->Zoom->space = NULL;

	rtn = soap_call___ptz__AbsoluteMove(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | AbsoluteMove Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_relative_move(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token, ptz_info_onvif info)
{
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__RelativeMove req;
	struct _ptz__RelativeMoveResponse res;

	char *action = "RelativeMove";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__RelativeMove));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.Translation = soap_malloc(soap, sizeof(struct tt__PTZVector));
	req.Translation->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	req.Translation->PanTilt->x = (float)info.relative_pan / MULTIPLIER_0_TO_1;
	req.Translation->PanTilt->y = (float)info.relative_tilt / MULTIPLIER_0_TO_1;
	req.Translation->PanTilt->space = NULL;

	req.Translation->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
	req.Translation->Zoom->x = (float)info.relative_zoom / MULTIPLIER_0_TO_1;
	req.Translation->Zoom->space = NULL;

	req.Speed = soap_malloc(soap, sizeof(struct tt__PTZSpeed));
	req.Speed->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	//req.Speed->PanTilt->x = roundi(info.speed_pan / MULTIPLIER_0_TO_1);
	req.Speed->PanTilt->x = 1;
	//req.Speed->PanTilt->y = roundi(info.speed_tilt / MULTIPLIER_0_TO_1);
	req.Speed->PanTilt->y = 1;
	req.Speed->PanTilt->space = NULL;

	req.Speed->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
	//req.Speed->Zoom->x = roundi(info.speed_zoom / MULTIPLIER_0_TO_1);
	req.Speed->Zoom->x = 1;
	req.Speed->Zoom->space = NULL;

	//NF_ONVIF_DBG("[%s] pan : %f tilt : %f zoom : %f\n", __FUNCTION__, req.Translation->PanTilt->x, req.Translation->PanTilt->y, req.Translation->Zoom->x);

	rtn = soap_call___ptz__RelativeMove(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | RelativeMove Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_continuous_move(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token, ptz_info_onvif info, long long int timeout)
{
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__ContinuousMove req;
	struct _ptz__ContinuousMoveResponse res;

	char *action = "ContinuousMove";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__ContinuousMove));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.Velocity = soap_malloc(soap, sizeof(struct tt__PTZSpeed));
	req.Velocity->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	req.Velocity->PanTilt->x = (float)info.speed_pan / MULTIPLIER_0_TO_1;
	req.Velocity->PanTilt->y = (float)info.speed_tilt / MULTIPLIER_0_TO_1;
	req.Velocity->PanTilt->space = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";

	if(info.speed_zoom == 0)
	{
		req.Velocity->Zoom = NULL;
	}
	else
	{
		req.Velocity->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
		req.Velocity->Zoom->x = (float)info.speed_zoom / MULTIPLIER_0_TO_1;
		req.Velocity->Zoom->space = NULL;
	}

	if(timeout == 0)
	{
		req.Timeout = NULL;
	}
	else
	{
		//req.Timeout = soap_malloc(soap, sizeof(long long int));
		//*req.Timeout = timeout;
	}
	//NF_ONVIF_DBG("[%s] pan : %f tilt : %f zoom : %f timeout : %lld\n", __FUNCTION__, req.Velocity->PanTilt->x, req.Velocity->PanTilt->y, req.Velocity->Zoom->x, timeout);

	rtn = soap_call___ptz__ContinuousMove(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ContinuousMove Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_continuous_stop(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token)
{

	int rtn;
	rtn = _nf_onvif_ptz_stop(auth_info, endpoint, token);
#if 0
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__ContinuousMove req;
	struct _ptz__ContinuousMoveResponse res;

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__ContinuousMove));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.Velocity = soap_malloc(soap, sizeof(struct tt__PTZSpeed));
	req.Velocity->PanTilt = soap_malloc(soap, sizeof(struct tt__Vector2D));
	req.Velocity->PanTilt->x = 0;
	req.Velocity->PanTilt->y = 0;
	req.Velocity->PanTilt->space = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";

	req.Velocity->Zoom = soap_malloc(soap, sizeof(struct tt__Vector1D));
	req.Velocity->Zoom->x = 0;
	req.Velocity->Zoom->space = NULL;

	req.Timeout = NULL;

	rtn = soap_call___ptz__ContinuousMove(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ContinuousStop Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/



ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);
#endif
	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_get_presets(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token, ptz_t_onvif* ptz_onvif)
{
	int i = 0, j = 0;
	int rtn = 0;
	int preset_cnt = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__GetPresets req;
	struct _ptz__GetPresetsResponse res;

	char *action = "GetPresets";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GetPresets));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	rtn = soap_call___ptz__GetPresets(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetPresets Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

	if(res.__sizePreset > 256)
		preset_cnt = 256;
	else
		preset_cnt = res.__sizePreset;

	ptz_onvif->preset_cnt = preset_cnt;

	//current preset_cnt 
	ptz_onvif->current_preset_cnt = preset_cnt;

	if(preset_cnt > 0)
	{
		/*NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | %d presets return \n",
						D_SECS(now_time),
						__FUNCTION__, res.__sizePreset
						);*/
		for(i = 0; i < preset_cnt; i++)
		{
			snprintf(ptz_onvif->preset_token[i], 64, "%s", res.Preset[i].token);
			snprintf(ptz_onvif->preset_name[i], 64, "%s", res.Preset[i].Name);
			//NF_ONVIF_DBG(" %d th preset: name (%s) token (%s)\n", i, res.Preset[i].Name, res.Preset[i].token);
		}
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}
ONVIF_MSG _nf_onvif_zoom_continuous_stop(int auth, const char *endpoint, const char* user, const char* pass, const char* token)
{
	int rtn = 0;

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__Stop req;
	struct _ptz__StopResponse res;

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__Stop));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.PanTilt = NULL;

	req.Zoom = soap_malloc(soap, sizeof(enum xsd__boolean));
	*(req.Zoom) = xsd__boolean__true_;

	rtn = soap_call___ptz__Stop(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Stop Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_set_preset(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* profileToken, char* presetToken, char* presetName)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__SetPreset req;
	struct _ptz__SetPresetResponse res;

	char *action = "SetPreset";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__SetPreset));

	req.ProfileToken = soap_malloc(soap, strlen(profileToken)+1);
	strcpy(req.ProfileToken, profileToken);

	//NF_ONVIF_DBG("orig presetToken : %s\n", presetToken);

	if(presetToken != NULL && strlen(presetToken) > 0)
	{	// update
		req.PresetToken = soap_malloc(soap, strlen(presetToken) + 1);
		strcpy(req.PresetToken, presetToken);
	}
	else
	{	// insert
		req.PresetToken = NULL;
	}
	
	//set preset name itx scenario
	if(presetName != NULL && strlen(presetName) > 0)
	{
		req.PresetName = soap_malloc(soap, strlen(presetName) + 1);
		strcpy(req.PresetName, presetName);
	}

	//NF_ONVIF_DBG("req presetToken : %s\n", req.PresetToken);

	/* setpreset takes some seconds... */
	soap->recv_timeout = 20;

	rtn = soap_call___ptz__SetPreset(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | SetPreset Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/
	{
		//NF_ONVIF_DBG("%s | return token : %s\n", __FUNCTION__, res.PresetToken);
		snprintf(presetToken, 64, "%s", res.PresetToken);
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}


ONVIF_API _nf_onvif_ptz_remove_preset(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* profileToken, const char* presetToken)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__RemovePreset req;
	struct _ptz__RemovePresetResponse res;

	char *action = "RemovePreset";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	
	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__RemovePreset));

	req.ProfileToken = soap_malloc(soap, strlen(profileToken)+1);
	strcpy(req.ProfileToken, profileToken);

	req.PresetToken = soap_malloc(soap, strlen(presetToken)+1);
	strcpy(req.PresetToken, presetToken);

	rtn = soap_call___ptz__RemovePreset(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | RemovePreset Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_goto_preset(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, const char* profileToken, const char* presetToken)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__GotoPreset req;
	struct _ptz__GotoPresetResponse res;

	char *action = "GotoPreset";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GotoPreset));

	req.ProfileToken = soap_malloc(soap, strlen(profileToken)+1);
	strcpy(req.ProfileToken, profileToken);

	req.PresetToken = soap_malloc(soap, strlen(presetToken)+1);
	strcpy(req.PresetToken, presetToken);

	req.Speed = NULL;

	rtn = soap_call___ptz__GotoPreset(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GotoPreset Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

ONVIF_MSG _nf_onvif_ptz_stop(ipcam_onvif_auth_info_t *auth_info, const char *endpoint, const char* token)
{
	int rtn = 0;

	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__Stop req;
	struct _ptz__StopResponse res;

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__Stop));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	req.PanTilt = soap_malloc(soap, sizeof(enum xsd__boolean));
	*(req.PanTilt) = xsd__boolean__true_;

	req.Zoom = soap_malloc(soap, sizeof(enum xsd__boolean));
	*(req.Zoom) = xsd__boolean__true_;

	rtn = soap_call___ptz__Stop(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | Stop Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}


ONVIF_MSG _nf_onvif_ptz_goto_home_position(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const char* token)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__GotoHomePosition req;
	struct _ptz__GotoHomePositionResponse res;

	char *action = "GotoHomePosition";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GotoHomePosition));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	rtn = soap_call___ptz__GotoHomePosition(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GotoHomePosition Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}


ONVIF_MSG _nf_onvif_ptz_send_auxiliary_command(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, struct _T_ONVIF_AUXILIARY_COMMAND *auxiliary)
{
	int rtn;
	struct soap *soap;
	struct _ptz__SendAuxiliaryCommand req;
	struct _ptz__SendAuxiliaryCommandResponse res;

	soap = soap_new();

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	req.ProfileToken = auxiliary->profile_token;
	req.AuxiliaryData = auxiliary->command;

	rtn = soap_call___ptz__SendAuxiliaryCommand(soap, endpoint, NULL, &req, &res);
	if (rtn != 0)
	{
		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}


ONVIF_MSG _nf_onvif_ptz_get_preset_tours(int auth, const char *endpoint, const char* user, const char* pass, const char* token)
{
	int i = 0, j = 0;
	int rtn = 0;
	struct soap *soap;
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	struct _ptz__GetPresetTours req;
	struct _ptz__GetPresetToursResponse res;

	char *action = "GetPresetTours";

	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | start\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG("[%s] ERROR: Add Security(%d)\n", __FUNCTION__, rtn);
		goto ends_label;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto ends_label;
		}
	}

	memset(&req, 0x00, sizeof(struct _ptz__GetPresetTours));

	req.ProfileToken = soap_malloc(soap, strlen(token)+1);
	strcpy(req.ProfileToken, token);

	rtn = soap_call___ptz__GetPresetTours(soap, endpoint, NULL, &req, &res);

	if (rtn != 0)
	{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | ERROR: RPC returns fail\n",
				D_SECS(now_time),
				__FUNCTION__
				);

		char buf[1024];
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG("\033[1;40;32m%s\033[0m | error txts : \n %s --------------------\n", __FUNCTION__, buf);

		// not supported
		if(strstr(buf, "NotSupported") != NULL)
		{
			rtn = -9;
		}

		goto ends_label;
	}
	/*{
		clock_gettime(CLOCK_REALTIME, &now_time);
		NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | GetPresetTours Success!\n",
				D_SECS(now_time),
				__FUNCTION__
				);
	}*/
#if 0
	if(res.__sizePresetTour == 0 || res.PresetTour == NULL)
	{

	}
	else
	{
		for(i = 0; i < res.__sizePresetTour; i++)
		{
			NF_ONVIF_DBG("%dth preset tour =====================\n", i);
			if(res.PresetTour[i].Name != NULL)
			{
				NF_ONVIF_DBG(" name : %s\n", res.PresetTour[i].Name);
			}
			NF_ONVIF_DBG(" token : %s\n", res.PresetTour[i].token);
			if(res.PresetTour[i].Status != NULL)
			{
				NF_ONVIF_DBG(" status : %d\n", res.PresetTour[i].Status->State);
			}
			if(res.PresetTour[i].StartingCondition != NULL)
			{

			}
			NF_ONVIF_DBG(" tourspot no : %d\n", res.PresetTour[i].__sizeTourSpot);
		}
	}
#endif

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	clock_gettime(CLOCK_REALTIME, &now_time);
	NF_ONVIF_DBG("[%lu.%06lu] \033[1;40;32m%s\033[0m | end\n",
			D_SECS(now_time),
			__FUNCTION__
			);

	return rtn;
}

#endif //__NF_ONVIF_PTZ_C__

