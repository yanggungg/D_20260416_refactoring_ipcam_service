/*
 * ITXM2M
 * ONVIF HDPRO API
 * 2017-09-07
 */

/* ================================================================================ */
// Include

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_driver_hdpro.h"
#include "nf_ipcam_utils.h"

/* ================================================================================ */
// Define

#define	API_PATH			"/api/Web.cgi"
#define	BUFF_LEN			(1024)
#define PMASK_EXP_MAX		(4)
#define CURL_TIMEOUT		(4)
#define CURL_EXP_TIMEOUT	(1)
#define PMASK_EXP_USLEEP	(300000)

/* ================================================================================ */
// Enum

typedef enum
{
	HDPRO_RTN_OK = 0,
	HDPRO_RTN_FAIL,
	HDPRO_RTN_MAX
} HDPRO_RTN_TYPE;

/* ================================================================================ */
// Global Variable



/* ================================================================================ */
// Static Function

static int _hdpro_call_api(int ch, const char *path, const char *query, char *buf, size_t buf_size, int timeout)
{
	int rtn = HDPRO_RTN_FAIL;
	icm_http ctx;

	// init ctx , res
	memset(&ctx, 0x00, sizeof(ctx));
	icm_http_ch_init_tout(&ctx, ch, timeout);

	if(icm_http_post_request(&ctx, path, query, buf, buf_size) != ICM_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Curl http api Fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_mirror(cam_info *info, int ch)
{
	const char *param = "code=videoImage&mode=set&mirroring=%s&pivot=%s";
	char *mirror = NULL;
	char *pivot = NULL;
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";

	if(info == NULL)
	{
		goto ends_label;
	}

	switch(info->vcodec.mirror)
	{
		case NF_IPCAM_MIRROR_HDPRO_NONE:
		{
			mirror = "off";
			pivot  = "off";
		}
		break;
		case NF_IPCAM_MIRROR_HDPRO_R180:
		{
			mirror = "both";
			pivot  = "off";
		}
		break;
		case NF_IPCAM_MIRROR_HDPRO_HORIZONTAL:
		{
			mirror = "horizontal";
			pivot  = "off";
		}
		break;
		case NF_IPCAM_MIRROR_HDPRO_VERTICAL:
		{
			mirror = "vertical";
			pivot  = "off";
		}
		break;
		default:
		{
			mirror = "off";
			pivot  = "off";
		}
	}

	snprintf(query, BUFF_LEN, param, mirror, pivot);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO set mirror fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_pmask_enable(int ch)
{
	const char *param = "code=videoPrivacy&mode=set&cmd=use&usePrivacy=on";
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";

	snprintf(query, BUFF_LEN, param);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask enable fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_pmask_removeAll_old(int ch)
{
	const char *param = "code=videoPrivacy&mode=set&cmd=removeAll";
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";

	snprintf(query, BUFF_LEN, param);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_EXP_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask removeAll fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _parse_pmask_cell_count(char *str, int *max_width, int *max_height)
{
	int rtn = HDPRO_RTN_FAIL;
	char *cell = NULL;
	char *value = NULL;

	if(str == NULL || max_width == NULL || max_height == NULL)
	{
		goto ends_label;
	}

#if (HDPRO_DEBUG)
	HDPRO_PRINT("privacy mask information\n%s\n\n", str);
#endif

	cell = strstr(str, "maxWidth");
	if(cell == NULL)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask not found maxWidth(%s)\n", str);
		goto ends_label;
	}
	else
	{
		value = strchr(cell, '=');
		if(value == NULL)
		{
			IPCAM_DBG(ERROR, "HDPRO pmask not found maxWidth(%s)\n", str);
			goto ends_label;
		}
		else
		{
			*max_width = atoi(value+1);
		}
	}

	cell = strstr(str, "maxHeight");
	if(cell == NULL)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask not found maxHeight(%s)\n", str);
		goto ends_label;
	}
	else
	{
		value = strchr(cell, '=');
		if(value == NULL)
		{
			IPCAM_DBG(ERROR, "HDPRO pmask not found maxHeight(%s)\n", str);
			goto ends_label;
		}
		else
		{
			*max_height = atoi(value+1);
		}
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_get_pmask_cell(int ch, int *max_width, int *max_height)
{
	const char *param = "code=videoPrivacyAll&mode=get";
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char buff[BUFF_LEN] = "";

	if(max_width == NULL || max_height == NULL)
	{
		goto ends_label;
	}

	snprintf(query, BUFF_LEN, param);
	memset(&buff, 0x00, BUFF_LEN);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, buff, BUFF_LEN, CURL_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask get cell count fail\n");
		goto ends_label;
	}

	if(_parse_pmask_cell_count(buff, max_width, max_height) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask cell count parse fail(%s)\n", buff);
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_get_pmask_cell_old(int ch, int *max_width, int *max_height)
{
	const char *param = "code=videoPrivacy&mode=get";
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char buff[BUFF_LEN] = "";

	if(max_width == NULL || max_height == NULL)
	{
		goto ends_label;
	}

	snprintf(query, BUFF_LEN, param);
	memset(&buff, 0x00, BUFF_LEN);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, buff, BUFF_LEN, CURL_EXP_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask get cell count fail\n");
		goto ends_label;
	}

	if(_parse_pmask_cell_count(buff, max_width, max_height) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask cell count parse fail(%s)\n", buff);
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_pmask_calc_point(int org_val, float scale, int min_val, int max_val)
{
	int value = 0;

	value = (int)(org_val*scale);

	if(value < min_val)
	{
		value = min_val;
	}

	if(value > max_val)
	{
		value = max_val;
	}

	return value;
}

static int _hdpro_set_pmask_all(int ch, NFIPCamPrivacyMask *pmask_info, int max_width, int max_height)
{
	const char *param_cmd = "code=videoPrivacyAll&mode=set";
	const char *param_zone = "&useZone%d=%s&nameZone%d=AREA%d"
	                         "&topZone%d=%d&leftZone%d=%d&rightZone%d=%d&bottomZone%d=%d";
	int idx = 0;
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char *use = NULL;
	int off = 0;
	int top = 0;
	int left = 0;
	int right = 0;
	int bottom = 0;
	float scaleX = 0.0;
	float scaleY = 0.0;

	scaleX = (float)((float)max_width/(float)HDPRO_PMASK_CELL_NCOLS);
	scaleY = (float)((float)max_height/(float)HDPRO_PMASK_CELL_NROWS);

#if (HDPRO_DEBUG)
	HDPRO_PRINT("scaleX(%f), scaleY(%f)\n", scaleX, scaleY);
#endif

	off = snprintf(query, BUFF_LEN, param_cmd);

	for(idx = 0; idx < HDPRO_PMASK_AREA_MAX; idx++)
	{

#if (HDPRO_DEBUG)
		HDPRO_PRINT("AREA%d left-top(%d,%d), right-bottom(%d,%d)\n", 
					idx+1,
					pmask_info->lt[idx].x, pmask_info->lt[idx].y,
					pmask_info->rb[idx].x, pmask_info->rb[idx].y);
#endif

		if(pmask_info->lt[idx].x < 0)
		{
			top    = 0;
			left   = 0;
			right  = 0;
			bottom = 0;

			use = "off";
		}
		else
		{
			top    = _hdpro_set_pmask_calc_point(pmask_info->lt[idx].y,   scaleY, 0, max_height);
			left   = _hdpro_set_pmask_calc_point(pmask_info->lt[idx].x,   scaleX, 0, max_width);
			right  = _hdpro_set_pmask_calc_point(pmask_info->rb[idx].x+1, scaleX, 0, max_width);
			bottom = _hdpro_set_pmask_calc_point(pmask_info->rb[idx].y+1, scaleY, 0, max_height);

			use = "on";
		}

		off += snprintf(query+off, BUFF_LEN-off, param_zone,
													   idx+1, use,
													   idx+1, idx+1,
													   idx+1, top,
													   idx+1, left,
													   idx+1, right, 
													   idx+1, bottom);
	}

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO pmask add area fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_pmask_add_old(int ch, NFIPCamPrivacyMask *pmask_info, int max_width, int max_height)
{
	const char *param = "code=videoPrivacy&mode=set&cmd=add&name=AREA%d&top=%d&left=%d&right=%d&bottom=%d";
	int idx = 0;
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN] = "";
	int top = 0;
	int left = 0;
	int right = 0;
	int bottom = 0;
	float scaleX = 0.0;
	float scaleY = 0.0;

	scaleX = (float)((float)max_width/(float)HDPRO_PMASK_CELL_NCOLS);
	scaleY = (float)((float)max_height/(float)HDPRO_PMASK_CELL_NROWS);

#if (HDPRO_DEBUG)
	HDPRO_PRINT("scaleX(%f), scaleY(%f)\n", scaleX, scaleY);
#endif

	for(idx = 0; idx < PMASK_EXP_MAX; idx++)
	{
#if (HDPRO_DEBUG)
		HDPRO_PRINT("AREA%d left-top(%d,%d), right-bottom(%d,%d)\n", 
					idx,
					pmask_info->lt[idx].x, pmask_info->lt[idx].y,
					pmask_info->rb[idx].x, pmask_info->rb[idx].y);
#endif
		if(pmask_info->lt[idx].x < 0)
		{
			top    = 0;
			left   = 0;
			right  = 0;
			bottom = 0;
		}
		else
		{
			top    = _hdpro_set_pmask_calc_point(pmask_info->lt[idx].y,   scaleY, 0, max_height);
			left   = _hdpro_set_pmask_calc_point(pmask_info->lt[idx].x,   scaleX, 0, max_width);
			right  = _hdpro_set_pmask_calc_point(pmask_info->rb[idx].x+1, scaleX, 0, max_width);
			bottom = _hdpro_set_pmask_calc_point(pmask_info->rb[idx].y+1, scaleY, 0, max_height);
		}

		if(left >= right || top >= bottom)
		{
			continue;
		}

		snprintf(query, BUFF_LEN, param, idx+1, top, left, right, bottom);

		if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_EXP_TIMEOUT) != HDPRO_RTN_OK)
		{
			IPCAM_DBG(ERROR, "HDPRO pmask add area fail\n");
			goto ends_label;
		}

		usleep(PMASK_EXP_USLEEP);
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_privacy_mask(NFIPCamPrivacyMask *pmask_info, int ch)
{
	int rtn = HDPRO_RTN_FAIL;
	int max_width = 0;
	int max_height = 0;

	if(pmask_info == NULL)
	{
		goto ends_label;
	}

	// privacy mask enable
	if(_hdpro_set_pmask_enable(ch) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO privacy mask enable Fail(CH:%d)\n", ch);
		goto ends_label;
	}

	// get pmask max width / height cell count
	if(_hdpro_get_pmask_cell(ch, &max_width, &max_height) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO privacy mask get max cell count Fail(CH:%d)\n", ch);
		goto exeption;
	}

	// add area
	if(_hdpro_set_pmask_all(ch, pmask_info, max_width, max_height) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO privacy mask add area Fail(CH:%d)\n", ch);
		goto exeption;
	}

	rtn = HDPRO_RTN_OK;

exeption:

	// OLD Camera FW exception ( not have videoPrivacyAll api )
	if(rtn != HDPRO_RTN_OK)
	{
		// removeAll area
		if(_hdpro_set_pmask_removeAll_old(ch) != HDPRO_RTN_OK)
		{
			IPCAM_DBG(ERROR, "HDPRO privacy mask removeAll Fail(CH:%d)\n", ch);
			goto ends_label;
		}

		// get pmask max width / height cell count
		if(_hdpro_get_pmask_cell_old(ch, &max_width, &max_height) != HDPRO_RTN_OK)
		{
			IPCAM_DBG(ERROR, "HDPRO privacy mask get max cell count Fail(CH:%d)\n", ch);
			goto ends_label;
		}

		// add area
		if(_hdpro_set_pmask_add_old(ch, pmask_info, max_width, max_height) != HDPRO_RTN_OK)
		{
			IPCAM_DBG(ERROR, "HDPRO privacy mask add area Fail(CH:%d)\n", ch);
			goto ends_label;
		}
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

static int _hdpro_set_onepush(int ch)
{
	const char *param = "code=ptzCommand&mode=set&command=focusOnepush&step=0&speed=1";
	int rtn = HDPRO_RTN_FAIL;
	char query[BUFF_LEN]  = "";

	snprintf(query, BUFF_LEN, param);

	if(_hdpro_call_api(ch, API_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_hdpro_call_api fail\n");
		goto ends_label;
	}

	rtn = HDPRO_RTN_OK;

ends_label:

	return rtn;
}

/* ================================================================================ */
// Extern Function

int nf_hdpro_set_mirror(cam_info *info, int ch)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "HDPRO SET MIRROR START (CH %d)\n", ch);

	if(info == NULL)
	{
		IPCAM_DBG(ERROR, "HDPRO Input param Error\n");
		goto ends_label;
	}

	if(_hdpro_set_mirror(info, ch) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO _hdpro_set_mirror Fail (CH %d)\n", ch);
		goto ends_label;
	}

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MIRROR, -1);
	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "HDPRO SET MIRROR END (CH %d)\n", ch);

	return rtn;
}

int nf_hdpro_set_privacy_mask(NFIPCamPrivacyMask *pmask_info, int ch)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "HDPRO SET PMASK START (CH %d)\n", ch);

	if(pmask_info == NULL)
	{
		IPCAM_DBG(ERROR, "HDPRO Input param Error\n");
		goto ends_label;
	}

	if(_hdpro_set_privacy_mask(pmask_info, ch) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO _hdpro_set_privacy_mask Fail (CH %d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "HDPRO SET PMASK END (CH %d)\n", ch);

	return rtn;
}

int nf_hdpro_set_onepush(int ch)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "HDPRO SET ONEPUSH START (CH %d)\n", ch);

	if(_hdpro_set_onepush(ch) != HDPRO_RTN_OK)
	{
		IPCAM_DBG(ERROR, "HDPRO _hdpro_set_onepush Fail (CH %d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "HDPRO SET ONEPUSH END (CH %d)\n", ch);

	return rtn;
}

int nf_hdpro_enable_mirror(int ch)
{
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "HDPRO ENABLE MIRROR START (CH %d)\n", ch);

	runtime = get_runtime();

	// Mirror
	runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_MIRRORING;//onvif_NR
	runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_ROTATION;//onvif_NR
	runtime[ch].video.supported |= VIDEO_SETUP_MIRROR;
	runtime[ch].video.onthefly |= VIDEO_SETUP_MIRROR;
	runtime[ch].video.mirror.support = 0;
	runtime[ch].video.mirror.support = NF_IPCAM_MIRROR_HDPRO_NONE       | NF_IPCAM_MIRROR_HDPRO_R180 |
									   NF_IPCAM_MIRROR_HDPRO_HORIZONTAL | NF_IPCAM_MIRROR_HDPRO_VERTICAL;
	runtime[ch].video.mirror.value = NF_IPCAM_MIRROR_HDPRO_NONE; // dummy value
	runtime[ch].funcs[NF_IPCAM_TYPE_SET_MIRROR] = &nf_hdpro_set_mirror;

	IPCAM_DBG(MINOR, "HDPRO ENABLE MIRROR END (CH %d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

int nf_hdpro_enable_onepush(int ch)
{
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "HDPRO ENABLE ONEPUSH START (CH %d)\n", ch);

	runtime = get_runtime();

	// Onepushfocus
	runtime[ch].funcs[NF_IPCAM_TYPE_SET_ONESHOT] = &nf_hdpro_set_onepush;
	runtime[ch].image.supported |= NF_IPCAM_IMAGE_ONEPUSH;
	runtime[ch].image.onthefly |= NF_IPCAM_IMAGE_ONEPUSH;
	runtime[ch].image_onvif.supported_image |= NF_IPCAM_IMAGE_ONVIF_FOCUS_ONEPUSH;
	runtime[ch].ptz.supported |= PTZ_SETUP_ONEPUSH;

	IPCAM_DBG(MINOR, "HDPRO ENABLE ONEPUSH END (CH %d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

