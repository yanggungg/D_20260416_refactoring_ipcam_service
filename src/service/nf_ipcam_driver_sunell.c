/*
 * ITXM2M
 * SUNELL HTTP API
 * 2017-10-26
 */

/* ================================================================================ */
// Include

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_driver_sunell.h"
#include "nf_ipcam_utils.h"

/* ================================================================================ */
// Define

#define	PARAM_PATH			"/cgi-bin/param.cgi"
#define	PTZ_PATH			"/cgi-bin/ptz.cgi"
#define	BUFF_LEN			(1024)
#define CURL_TIMEOUT		(4)

#define HTTP_RES_LINE_MAX	(100)

#define LAYOUT_SCALE_F		(0.01f)

#define PT_SPEED_MIN		(-63)
#define PT_SPEED_MAX		(63)
#define ZOOM_SPEED_IN		(1)
#define ZOOM_SPEED_OUT		(-1)

/** @def MULTIPLIER_0_TO_1
 *  @brief the range should be defined as the full range of the PTZ unit normalized to the range -1 to 1(onvif ptz space)
 */
#define MULTIPLIER_0_TO_1   (10.0)

/* ================================================================================ */
// Enum

typedef enum
{
	SUNELL_RTN_OK = 0,
	SUNELL_RTN_FAIL,
	SUNELL_RTN_MAX
} SUNELL_RTN_TYPE;

typedef enum
{
	SUNELL_INFO_CMP_VAL = 0,
	SUNELL_INFO_CMP_STR,
	SUNELL_INFO_CMP_MAX
} SUNELL_INFO_CMP_TYPE;

typedef enum
{
	SUNELL_CALC_PTZ_SP_PAN = 0,
	SUNELL_CALC_PTZ_SP_TILT,
	SUNELL_CALC_PTZ_SP_ZOOM,
	SUNELL_CALC_PTZ_SP_MAX
} SUNELL_CALC_PTZ_SP_TYPE; 

/* ================================================================================ */
// Global Variable

struct mount_info_tbl
{
	NF_IPCAM_MOUNT_TYPES_E type;
	const char *str;
};

struct dewarp_info_tbl
{
	NF_IPCAM_DEWARP_MODES_E mode;
	const char *str;
};

/* ================================================================================ */
// Global Variable

static struct mount_info_tbl _mount_info_tbl[] =
{
	{ NF_IPCAM_MOUNT_WALL,    "0"  },
	{ NF_IPCAM_MOUNT_CELLING, "1"  },
	{ NF_IPCAM_MOUNT_TABLE,   "2"  },
	{ -1,  NULL }, 
};

static struct dewarp_info_tbl _dewarp_info_tbl[] = 
{
	{ NF_IPCAM_DEWARP_IP_FISHEYE,      "0"  },
	{ NF_IPCAM_DEWARP_IP_S_PANORAMA,   "10" },
	{ NF_IPCAM_DEWARP_IP_D_PANORAMA,   "2"  }, 
	{ NF_IPCAM_DEWARP_IP_4PTZ,         "8"  },
	{ NF_IPCAM_DEWARP_IP_FISHEYE_3PTZ, "4"  },
	{ NF_IPCAM_DEWARP_IP_FISHEYE_5PTZ, "5"  },
	{ NF_IPCAM_DEWARP_IP_FISHEYE_7PTZ, "6"  },
	{ -1, NULL },
};

/* ================================================================================ */
// Static Function

static int _sunell_call_api(int ch, const char *path, const char *query, char *buf, size_t buf_size, int timeout)
{
	int rtn = SUNELL_RTN_FAIL;
	icm_http ctx;
	icm_response res;

	// init ctx, res
	memset(&ctx, 0x00, sizeof(ctx));
	icm_http_ch_init_tout(&ctx, ch, timeout);

	// init res
	memset(&res, 0x00, sizeof(res));

	if(icm_http_new_get_request(&ctx, path, query, &res) != ICM_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Curl http api Fail\n");
		goto ends_label;
	}

	if(res.status == 200)
	{
		rtn = SUNELL_RTN_OK;
	}
	else
	{
		IPCAM_DBG(ERROR, "Camera Web Not Ok Return(CH:%d,(status:%d,msg:%s))\n", 
				ch, res.status, res.msg);
	}

	if(res.msg != NULL && res.size > 0)
	{
#if (SUNELL_DEBUG)
		SUNELL_PRINT("curl call response \n%s\n\n", res.msg);
#endif
		if(buf != NULL && buf_size > 0)
		{
			snprintf(buf, buf_size, res.msg);
		}
	}

ends_label:

	icm_http_response_free(&res);

	return rtn;
}

static int _sunell_get_mount_info(SUNELL_INFO_CMP_TYPE cmp, NF_IPCAM_MOUNT_TYPES_E type, const char *str, struct mount_info_tbl *buf)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;

	if(buf == NULL || (cmp == SUNELL_INFO_CMP_STR && str == NULL))
	{
		goto ends_label;
	}

	while(1)
	{
		if(_mount_info_tbl[idx].str == NULL)
		{
			break;
		}

		if(cmp == SUNELL_INFO_CMP_VAL)
		{
			if(_mount_info_tbl[idx].type == type)
			{
				memcpy(buf, &(_mount_info_tbl[idx]), sizeof(struct mount_info_tbl));
				rtn = SUNELL_RTN_OK;
				break;
			}
		}
		else if(cmp == SUNELL_INFO_CMP_STR)
		{
			if(strcmp(_mount_info_tbl[idx].str, str) == 0)
			{
				memcpy(buf, &(_mount_info_tbl[idx]), sizeof(struct mount_info_tbl));
				rtn = SUNELL_RTN_OK;
				break;
			}
		}

		idx++;
	}

ends_label:

	return rtn;
}

static int _sunell_get_dewarp_info(SUNELL_INFO_CMP_TYPE cmp, NF_IPCAM_DEWARP_MODES_E mode, const char *str, struct dewarp_info_tbl *buf)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;

	if(buf == NULL || (cmp == SUNELL_INFO_CMP_STR && str == NULL))
	{
		goto ends_label;
	}

	while(1)
	{
		if(_dewarp_info_tbl[idx].str == NULL)
		{
			break;
		}

		if(cmp == SUNELL_INFO_CMP_VAL)
		{
			if(_dewarp_info_tbl[idx].mode == mode)
			{
				memcpy(buf, &(_dewarp_info_tbl[idx]), sizeof(struct dewarp_info_tbl));
				rtn = SUNELL_RTN_OK;
				break;
			}
		}
		else if(cmp == SUNELL_INFO_CMP_STR)
		{
			if(strcmp(_dewarp_info_tbl[idx].str, str) == 0)
			{
				memcpy(buf, &(_dewarp_info_tbl[idx]), sizeof(struct dewarp_info_tbl));
				rtn = SUNELL_RTN_OK;
				break;
			}
		}

		idx++;
	}

ends_label:

	return rtn;
}

static int _get_http_msg_value(const char *str, const char *name, char token, char *buf, size_t buf_len)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;
	char *tag_pos = NULL;
	char *val_pos = NULL;
	icm_str_array array = NULL;

	if(str == NULL || name == NULL || buf == NULL)
	{
		goto ends_label;
	}

	array = icm_str_split(str, "\n", HTTP_RES_LINE_MAX);
	if(array == NULL)
	{
		IPCAM_DBG(ERROR, "icm_str_split fail\n");
		goto ends_label;
	}

	for(idx = 0; idx < HTTP_RES_LINE_MAX; idx++)
	{
		if(array[idx] == NULL)
			continue;

		if(array[idx][0] == '\0')
			continue;

		if(icm_str_trim(array[idx], strlen(array[idx])) != ICM_STR_RTN_SUCC)
			continue;

		tag_pos = strstr(array[idx], name);
		if(tag_pos == NULL)
		{
			continue;
		}
		else
		{
			val_pos = strchr(tag_pos, token);
			if(val_pos+1 == '\0')
			{
				continue;
			}
			else
			{
				snprintf(buf, buf_len, val_pos+1);
				rtn = SUNELL_RTN_OK;
				break;
			}
		}
	}
			 
ends_label:

	if(array != NULL)
	{
		icm_str_array_free(array, HTTP_RES_LINE_MAX);
	}

	return rtn;
}

static int _parse_mount_type(const char *str, NF_IPCAM_MOUNT_TYPES_E *mount)
{
	int rtn = SUNELL_RTN_FAIL;
	char mount_str[16] = "";
	struct mount_info_tbl mount_info;

	if(str == NULL || mount == NULL)
	{
		goto ends_label;
	}

#if (SUNELL_DEBUG)
	SUNELL_PRINT("mount type information\n%s\n\n", str);
#endif

	if(_get_http_msg_value(str, "mountType", '=', mount_str, sizeof(mount_str)) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	memset(&mount_info, 0x00, sizeof(mount_info));
	if(_sunell_get_mount_info(SUNELL_INFO_CMP_STR, -1, mount_str, &mount_info) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	*mount = mount_info.type;

	rtn = SUNELL_RTN_OK;
			 
ends_label:

	return rtn;
}

static int _sunell_get_mount(int ch, NF_IPCAM_MOUNT_TYPES_E *mount)
{
	const char *param = "userName=%s&password=%s&action=get&type=mountParam";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char buff[BUFF_LEN] = "";

	if(mount == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password);

	if(_sunell_call_api(ch, PARAM_PATH, (const char *)query, buff, BUFF_LEN, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	if(_parse_mount_type(buff, mount) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_parse_mount_type fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _sunell_set_mount(int ch, NF_IPCAM_MOUNT_TYPES_E mount)
{
	const char *param = "userName=%s&password=%s&action=set&type=mountParam&mountType=%s";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	struct mount_info_tbl mount_info;

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(&mount_info, 0x00, sizeof(mount_info));
	if(_sunell_get_mount_info(SUNELL_INFO_CMP_VAL, mount, NULL, &mount_info) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password, mount_info.str);

	if(_sunell_call_api(ch, PARAM_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _parse_dewarp_mode(const char *str, NF_IPCAM_DEWARP_MODES_E *dewarp)
{
	int rtn = SUNELL_RTN_FAIL;
	char dewarp_str[16] = "";
	struct dewarp_info_tbl dewarp_info;

	if(str == NULL || dewarp == NULL)
	{
		goto ends_label;
	}

#if (SUNELL_DEBUG)
	SUNELL_PRINT("dewarp information\n%s\n\n", str);
#endif

	if(_get_http_msg_value(str, "dewarpMode", '=', dewarp_str, sizeof(dewarp_str)) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	memset(&dewarp_info, 0x00, sizeof(dewarp_info));
	if(_sunell_get_dewarp_info(SUNELL_INFO_CMP_STR, -1, dewarp_str, &dewarp_info) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	*dewarp = dewarp_info.mode;

	rtn = SUNELL_RTN_OK;
			 
ends_label:

	return rtn;
}

static int _sunell_get_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E *dewarp)
{
	const char *param = "userName=%s&password=%s&action=get&type=dewarpParam&cameraID=1";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char buff[BUFF_LEN] = "";

	if(dewarp == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password);

	if(_sunell_call_api(ch, PARAM_PATH, (const char *)query, buff, BUFF_LEN, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	if(_parse_dewarp_mode(buff, dewarp) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_parse_dewarp_mode fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _sunell_set_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E dewarp)
{
	const char *param = "userName=%s&password=%s&action=set&type=dewarpParam&cameraID=1"
						"&dewarpMode=%s&videoMode=0";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	struct dewarp_info_tbl dewarp_info;

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(&dewarp_info, 0x00, sizeof(dewarp_info));
	if(_sunell_get_dewarp_info(SUNELL_INFO_CMP_VAL, dewarp, NULL, &dewarp_info) != SUNELL_RTN_OK)
	{
		goto ends_label;
	}

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password, dewarp_info.str);

	if(_sunell_call_api(ch, PARAM_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

#if 0 // Old FW API ( InviewBeeForS2Fisheye_v2.0.0703.1002.66.0.125.1.20_20160809 )
static int _save_ePTZ_layout(NFIPCamEPTZLayout *layout, char *tag, int ePTZAreaId, float f_value)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;
	NF_IPCAM_EPTZ_AREA_TYPES_E area_type = 0;

	if(layout == NULL || tag == NULL)
	{
		goto ends_label;
	}

	idx = (layout->area_cnt)-1;

	if(idx < 0 || idx > MAX_EPTZ_AREA)
	{
		goto ends_label;
	}

	if(ePTZAreaId == 0)
	{
		area_type = NF_IPCAM_EPTZ_AREA_BASE;
	}
	else
	{
		area_type = NF_IPCAM_EPTZ_AREA_PTZ;
	}

	layout->eptz_area[idx].area_type = area_type;
	layout->eptz_area[idx].area_no = ePTZAreaId;

	if(strcmp(tag, "StartX") == 0)
	{
		layout->eptz_area[idx].start_lt_x = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "StartY") == 0)
	{
		layout->eptz_area[idx].start_lt_y = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "Height") == 0)
	{
		layout->eptz_area[idx].height = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "Width") == 0)
	{
		layout->eptz_area[idx].width = (float)(f_value*LAYOUT_SCALE_F);
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

// line ex) StartX[0]=33.330000
static int _parse_ePTZ_layout_line(const char *line, char *tag, size_t tag_size, int *ePTZAreaId, float *f_value)
{
	int rtn = SUNELL_RTN_FAIL;
	char *ptr = NULL;

	if(line == NULL || tag == NULL || ePTZAreaId == NULL || f_value == NULL)
	{
		goto ends_label;
	}

	// parse tag
	ptr = strchr(line, '[');
	if(ptr == NULL)
	{
		goto ends_label;
	}
	else
	{
		size_t tag_len = (size_t)(ptr - line);

		if(tag_len+1 < tag_size)
		{
			snprintf(tag, tag_len+1, line);
		}
		else
		{
			goto ends_label;
		}
	}

	// parse ePTZAreaId
	*ePTZAreaId = atoi(ptr+1);

	// parse f_value
	ptr = strchr(line, '=');
	if(ptr == NULL)
	{
		goto ends_label;
	}
	else
	{
		*f_value = (float)atof(ptr+1);
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _parse_ePTZ_layout(const char *str, NFIPCamEPTZLayout *layout)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;
	icm_str_array array = NULL;
	char tag[32] = "";
	int ePTZAreaId = 0;
	float f_value = 0.0f;
	int prev_ePTZAreaId = -1;

	if(str == NULL || layout == NULL)
	{
		goto ends_label;
	}

	layout->area_cnt = 0;

	array = icm_str_split(str, "\n", HTTP_RES_LINE_MAX);
	if(array == NULL)
	{
		IPCAM_DBG(ERROR, "icm_str_split fail\n");
		goto ends_label;
	}

	for(idx = 0; idx < HTTP_RES_LINE_MAX; idx++)
	{
		if(array[idx] == NULL)
			continue;

		if(array[idx][0] == '\0')
			continue;

		if(strstr(array[idx], "CameraId") != NULL)
			continue;

		if(icm_str_trim(array[idx], strlen(array[idx])) != ICM_STR_RTN_SUCC)
			continue;

#if (SUNELL_DEBUG)
		SUNELL_PRINT("line : [%s]\n", array[idx]);
#endif

		if(_parse_ePTZ_layout_line(array[idx], tag, sizeof(tag), &ePTZAreaId, &f_value) != SUNELL_RTN_OK)
			continue;

#if (SUNELL_DEBUG)
		SUNELL_PRINT("tag:%s,ePTZAreaId:%d,f_value:%.2f\n", tag, ePTZAreaId, f_value);
#endif

		if(prev_ePTZAreaId != ePTZAreaId)
		{
			if(layout->area_cnt < (MAX_EPTZ_AREA-1))
			{
				layout->area_cnt++;
			}
			else
			{
				break;
			}

			prev_ePTZAreaId = ePTZAreaId;
		}

		if(_save_ePTZ_layout(layout, tag, ePTZAreaId, f_value) != SUNELL_RTN_OK)
		{
			IPCAM_DBG(ERROR, "_save_ePTZ_layout fail\n");
			goto ends_label;
		}
	}

	rtn = SUNELL_RTN_OK;
			 
ends_label:

	if(array != NULL)
	{
		icm_str_array_free(array, HTTP_RES_LINE_MAX);
	}

	return rtn;
}
#else // New FW API ( v3.4.0703.1003.3.0.101.0.1_20171116 )
static int _save_ePTZ_layout(NFIPCamEPTZLayout *layout, char *tag, int ePTZAreaId, float f_value)
{
	int rtn = SUNELL_RTN_FAIL;
	NF_IPCAM_EPTZ_AREA_TYPES_E area_type = 0;

	if(layout == NULL || tag == NULL)
	{
		goto ends_label;
	}

	if(ePTZAreaId < 0 || ePTZAreaId > MAX_EPTZ_AREA)
	{
		goto ends_label;
	}

	if(ePTZAreaId == 0)
	{
		area_type = NF_IPCAM_EPTZ_AREA_BASE;
	}
	else
	{
		area_type = NF_IPCAM_EPTZ_AREA_PTZ;
	}

	layout->eptz_area[ePTZAreaId].area_type = area_type;
	layout->eptz_area[ePTZAreaId].area_no = ePTZAreaId;

	if(strcmp(tag, "StartX") == 0)
	{
		layout->eptz_area[ePTZAreaId].start_lt_x = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "StartY") == 0)
	{
		layout->eptz_area[ePTZAreaId].start_lt_y = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "Height") == 0)
	{
		layout->eptz_area[ePTZAreaId].height = (float)(f_value*LAYOUT_SCALE_F);
	}
	else if(strcmp(tag, "Width") == 0)
	{
		layout->eptz_area[ePTZAreaId].width = (float)(f_value*LAYOUT_SCALE_F);
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

// DewarpMode = 6
// VideoRectCount = 8
// VideoNum = 0
// StartX = 33.333333
// ...
static int _parse_ePTZ_layout_line(const char *line, char *name, size_t name_size, char *val, size_t val_size)
{
	int rtn = SUNELL_RTN_FAIL;
	icm_str_array array = NULL;

	if(line == NULL || name == NULL || val == NULL)
	{
		goto ends_label;
	}

	array = icm_str_split(line, "=", 2);
	if(array == NULL)
	{
		IPCAM_DBG(ERROR, "icm_str_split fail\n");
		goto ends_label;
	}

	if(array[0][0] != '\0')
	{
		icm_str_trim(array[0], strlen(array[0]));
		snprintf(name, name_size, array[0]);
	}
	if(array[1][0] != '\0')
	{
		icm_str_trim(array[1], strlen(array[1]));
		snprintf(val, val_size, array[1]);
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	if(array != NULL)
	{
		icm_str_array_free(array, 2);
	}

	return rtn;
}

static int _parse_ePTZ_layout(const char *str, NFIPCamEPTZLayout *layout)
{
	int rtn = SUNELL_RTN_FAIL;
	int idx = 0;
	icm_str_array line = NULL;
	int videoNum = 0;
	char name[32] = "";
	char value[32] = "";
	float f_value = 0.0f;

	if(str == NULL || layout == NULL)
	{
		goto ends_label;
	}

	layout->area_cnt = 0;

	line = icm_str_split(str, "\n", HTTP_RES_LINE_MAX);
	if(line == NULL)
	{
		IPCAM_DBG(ERROR, "icm_str_split fail\n");
		goto ends_label;
	}

	if(_parse_ePTZ_layout_line(line[0], name, sizeof(name), value, sizeof(value)) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "ePTZ layout line parse fail \n");
		goto ends_label;
	}
	else
	{
		if(strcmp(name, "DewarpMode") != 0)
		{
			IPCAM_DBG(ERROR, "not found DewarpMode\n");
			goto ends_label;
		}
		else
		{
			struct dewarp_info_tbl dewarp_info;

			memset(&dewarp_info, 0x00, sizeof(dewarp_info));
			if(_sunell_get_dewarp_info(SUNELL_INFO_CMP_STR, -1, value, &dewarp_info) != SUNELL_RTN_OK)
			{
				IPCAM_DBG(ERROR, "Invalid DewarpMode(%s)\n", value);
				goto ends_label;
			}
			layout->dewarp = dewarp_info.mode;
		}
	}

	if(layout->dewarp == NF_IPCAM_DEWARP_IP_S_PANORAMA || layout->dewarp == NF_IPCAM_DEWARP_IP_D_PANORAMA)
	{
		IPCAM_DBG(WARN, "double or single panorama exception\n");
		layout->area_cnt = 0;
		layout->eptz_area[0].area_type  = NF_IPCAM_EPTZ_AREA_NONE;
		layout->eptz_area[0].area_no    = 0;
		layout->eptz_area[0].start_lt_x = 0.0;
		layout->eptz_area[0].start_lt_y = 0.0;
		layout->eptz_area[0].height     = 0.0;
		layout->eptz_area[0].width      = 0.0;
		goto ends_label;
	}

	if(_parse_ePTZ_layout_line(line[1], name, sizeof(name), value, sizeof(value)) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "ePTZ layout line parse fail \n");
		goto ends_label;
	}
	else
	{
		if(strcmp(name, "VideoRectCount") != 0)
		{
			IPCAM_DBG(ERROR, "not found VideoRectCount\n");
			goto ends_label;
		}
		else
		{
			layout->area_cnt = atoi(value);
		}
	}

	for(idx = 2; idx < HTTP_RES_LINE_MAX; idx++)
	{
		if(line[idx] == NULL || line[idx][0] == '\0')
			continue;

#if (SUNELL_DEBUG)
		SUNELL_PRINT("line : [%s]\n", line[idx]);
#endif

		if(_parse_ePTZ_layout_line(line[idx], name, sizeof(name), value, sizeof(value)) != SUNELL_RTN_OK)
		{
			IPCAM_DBG(ERROR, "ePTZ layout line parse fail \n");
			goto ends_label;
		}

		// start parse
		if(strcmp(name, "VideoRectBegin") == 0)
			continue;
		
		// skip
		if(strcmp(name, "next_VideoRectURL") == 0)
			continue;

		if(strcmp(name, "VideoNum") == 0)
		{
			videoNum = atoi(value);
			continue;
		}

		// end parse
		if(strcmp(name, "VideoRectEnd") == 0)
			break;

		if(strcmp(name, "StartX") == 0 || strcmp(name, "StartY") == 0 ||
		   strcmp(name, "Height") == 0 || strcmp(name, "Width") == 0)
		{
			f_value = (float)atof(value);

#if (SUNELL_DEBUG)
			SUNELL_PRINT("name:%s,videoNum:%d,f_value:%.2f\n", name, videoNum, f_value);
#endif

			if(_save_ePTZ_layout(layout, name, videoNum, f_value) != SUNELL_RTN_OK)
			{
				IPCAM_DBG(ERROR, "_save_ePTZ_layout fail\n");
				goto ends_label;
			}
		}
	}

	rtn = SUNELL_RTN_OK;
			 
ends_label:

	if(line != NULL)
	{
		icm_str_array_free(line, HTTP_RES_LINE_MAX);
	}

	return rtn;
}
#endif

static int _sunell_get_ePTZ_layout(int ch, NFIPCamEPTZLayout *layout)
{
	const char *param = "userName=%s&password=%s&action=get&type=videoLayout&cameraID=1";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	char buff[BUFF_LEN] = "";

	if(layout == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password);

	if(_sunell_call_api(ch, PARAM_PATH, (const char *)query, buff, BUFF_LEN, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	if(_parse_ePTZ_layout(buff, layout) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_parse_ePTZ_layout fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _sunell_calc_ptz_speed(int type, int speed)
{
	int rtn = 0;

	switch(type)
	{
		case SUNELL_CALC_PTZ_SP_PAN:
		{
			rtn = (int)((speed*PT_SPEED_MAX)/MULTIPLIER_0_TO_1);
		}
		break;
		case SUNELL_CALC_PTZ_SP_TILT:
		{
			rtn = (int)(((-1)*speed*PT_SPEED_MAX)/MULTIPLIER_0_TO_1);
		}
		break;
		case SUNELL_CALC_PTZ_SP_ZOOM:
		{
			if(speed < 0)
			{
				rtn = (int)(ZOOM_SPEED_OUT);
			}
			else
			{
				rtn = (int)(ZOOM_SPEED_IN);
			}
		}
		break;
	}

	return rtn;
}

static int _sunell_move_ePTZ_pt(int ch, ptz_info_onvif *ptz)
{
	const char *param = "userName=%s&password=%s&action=rotate&pan=%d&tilt=%d&cameraID=1&PTZID=%d";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	int pan = 0;
	int tilt = 0;

	if(ptz == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	pan  = _sunell_calc_ptz_speed(SUNELL_CALC_PTZ_SP_PAN,  ptz->speed_pan);
	tilt = _sunell_calc_ptz_speed(SUNELL_CALC_PTZ_SP_TILT, ptz->speed_tilt);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password, pan, tilt, ptz->ePTZAreaId);

	if(_sunell_call_api(ch, PTZ_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _sunell_move_ePTZ_zoom(int ch, ptz_info_onvif *ptz)
{
	const char *param = "userName=%s&password=%s&action=zoom&pan=%d&cameraID=1&PTZID=%d";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";
	int zoom = 0;

	if(ptz == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	zoom = _sunell_calc_ptz_speed(SUNELL_CALC_PTZ_SP_ZOOM, ptz->speed_zoom);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password, zoom, ptz->ePTZAreaId);

	if(_sunell_call_api(ch, PTZ_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

static int _sunell_stop_ePTZ(int ch, ptz_info_onvif *ptz)
{
	const char *param = "userName=%s&password=%s&action=stop&cameraID=1&PTZID=%d";
	char username[64] = "";
	char password[64] = "";
	int rtn = SUNELL_RTN_FAIL;
	char query[BUFF_LEN] = "";

	if(ptz == NULL)
	{
		goto ends_label;
	}

	memset(username, 0x00, sizeof(username));
	memset(password, 0x00, sizeof(password));
	nf_ipcam_get_username(ch, username);
	nf_ipcam_get_password(ch, password);

	memset(query, 0x00, BUFF_LEN);
	snprintf(query, BUFF_LEN, param, username, password, ptz->ePTZAreaId);

	if(_sunell_call_api(ch, PTZ_PATH, (const char *)query, NULL, 0, CURL_TIMEOUT) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "_sunell_call_api fail\n");
		goto ends_label;
	}

	rtn = SUNELL_RTN_OK;

ends_label:

	return rtn;
}

/* ================================================================================ */
// Extern Function

int nf_sunell_get_mount(int ch, NF_IPCAM_MOUNT_TYPES_E *mount)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL GET MOUNT TYPE START (CH:%d)\n", ch);

	if(mount == NULL)
	{
		IPCAM_DBG(ERROR, "Input Param Error (CH:%d)\n", ch);
		goto ends_label;
	}

	if(_sunell_get_mount(ch, mount) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Get Mount Type Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL GET MOUNT TYPE END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_set_mount(int ch, NF_IPCAM_MOUNT_TYPES_E mount)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL SET MOUNT TYPE START (CH:%d)\n", ch);

#if (SUNELL_DEBUG)
	SUNELL_PRINT("mount : %d\n", mount);
#endif

	if(_sunell_set_mount(ch, mount) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Set Mount Type Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL SET MOUNT TYPE END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_get_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E *dewarp)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL GET DEWARP MODE START (CH:%d)\n", ch);

	if(dewarp == NULL)
	{
		IPCAM_DBG(ERROR, "Input Param Error(CH:%d)\n", ch);
		goto ends_label;
	}

	if(_sunell_get_dewarp(ch, dewarp) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Get Dewarp Mode Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL GET DEWARO MODE END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_set_dewarp(int ch, NF_IPCAM_DEWARP_MODES_E dewarp)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL SET DEWARP MODE START (CH:%d)\n", ch);

#if (SUNELL_DEBUG)
	SUNELL_PRINT("dewarp : %d\n", dewarp);
#endif

	if(_sunell_set_dewarp(ch, dewarp) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Set Dewarp Mode Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL SET DEWARP MODE END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_get_ePTZ_layout(int ch, NFIPCamEPTZLayout *layout)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "SUNELL GET ePTZ LAYOUT START (CH:%d)\n", ch);

	if(layout == NULL)
	{
		IPCAM_DBG(ERROR, "Input Param Error(CH:%d)\n", ch);
		goto ends_label;
	}

	if(_sunell_get_ePTZ_layout(ch, layout) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Get ePTZ Layout Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	runtime = get_runtime();

	if(layout->dewarp != (NF_IPCAM_DEWARP_MODES_E)runtime[ch].fisheye.dewarp.value)
	{
		IPCAM_DBG(ERROR, "Dewarp(%d) != runtime(%d) (CH:%d)\n", 
						layout->dewarp, runtime[ch].fisheye.dewarp.value, ch);
		goto ends_label;
	}

	layout->mount = runtime[ch].fisheye.mount.value;

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL GET ePTZ LAYOUT END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_move_ePTZ(int ch, ptz_info_onvif *ptz)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL MOVE ePTZ START (CH:%d)\n", ch);

	if(ptz == NULL)
	{
		IPCAM_DBG(ERROR, "Input Param Error(CH:%d)\n", ch);
		goto ends_label;
	}

#if (SUNELL_DEBUG)
	SUNELL_PRINT("mode:          %d\n", ptz->mode);
	SUNELL_PRINT("ePTZAreaId:    %d\n", ptz->ePTZAreaId);
	SUNELL_PRINT("absolute_pan:  %d\n", ptz->absolute_pan);
	SUNELL_PRINT("absolute_tilt: %d\n", ptz->absolute_tilt);
	SUNELL_PRINT("absolute_zoom: %d\n", ptz->absolute_zoom);
	SUNELL_PRINT("relative_pan:  %d\n", ptz->relative_pan);
	SUNELL_PRINT("relative_tilt: %d\n", ptz->relative_tilt);
	SUNELL_PRINT("relative_zoom: %d\n", ptz->relative_zoom);
	SUNELL_PRINT("speed_pan:     %d\n", ptz->speed_pan);
	SUNELL_PRINT("speed_tilt:    %d\n", ptz->speed_tilt);
	SUNELL_PRINT("speed_zoom:    %d\n", ptz->speed_zoom);
#endif

	// Pan / Tilt Move
	if(ptz->speed_zoom == 0)
	{
		if(_sunell_move_ePTZ_pt(ch, ptz) != SUNELL_RTN_OK)
		{
			IPCAM_DBG(ERROR, "Sunell Move ePTZ Pan/Tilt Fail (CH:%d)\n", ch);
			goto ends_label;
		}
	}
	// ZoomIn / ZoomOut
	else
	{
		if(_sunell_move_ePTZ_zoom(ch, ptz) != SUNELL_RTN_OK)
		{
			IPCAM_DBG(ERROR, "Sunell ePTZ ZoomIn/ZoomOut Fail (CH:%d)\n", ch);
			goto ends_label;
		}
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL MOVE ePTZ END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_stop_ePTZ(int ch, ptz_info_onvif *ptz)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "SUNELL STOP ePTZ START (CH:%d)\n", ch);

	if(ptz == NULL)
	{
		IPCAM_DBG(ERROR, "Input Param Error(CH:%d)\n", ch);
		goto ends_label;
	}

#if (SUNELL_DEBUG)
	SUNELL_PRINT("mode:          %d\n", ptz->mode);
	SUNELL_PRINT("ePTZAreaId:    %d\n", ptz->ePTZAreaId);
	SUNELL_PRINT("absolute_pan:  %d\n", ptz->absolute_pan);
	SUNELL_PRINT("absolute_tilt: %d\n", ptz->absolute_tilt);
	SUNELL_PRINT("absolute_zoom: %d\n", ptz->absolute_zoom);
	SUNELL_PRINT("relative_pan:  %d\n", ptz->relative_pan);
	SUNELL_PRINT("relative_tilt: %d\n", ptz->relative_tilt);
	SUNELL_PRINT("relative_zoom: %d\n", ptz->relative_zoom);
	SUNELL_PRINT("speed_pan:     %d\n", ptz->speed_pan);
	SUNELL_PRINT("speed_tilt:    %d\n", ptz->speed_tilt);
	SUNELL_PRINT("speed_zoom:    %d\n", ptz->speed_zoom);
#endif

	if(_sunell_stop_ePTZ(ch, ptz) != SUNELL_RTN_OK)
	{
		IPCAM_DBG(ERROR, "Sunell Stop ePTZ Fail (CH:%d)\n", ch);
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "SUNELL STOP ePTZ END (CH:%d)\n", ch);

	return rtn;
}

int nf_sunell_enable_mount(int ch)
{
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "SUNELL ENABLE MOUNT START (CH %d)\n", ch);

	runtime = get_runtime();

	if(runtime == NULL)
		return IPCAM_SETUP_RTN_FAILED;

	runtime[ch].fisheye.mount.support = 0;
	runtime[ch].fisheye.mount.support = NF_IPCAM_MOUNT_WALL | NF_IPCAM_MOUNT_CELLING | 
										NF_IPCAM_MOUNT_TABLE;

	runtime[ch].funcs[NF_IPCAM_TYPE_SET_MOUNT] = &nf_sunell_set_mount;

	IPCAM_DBG(MINOR, "SUNELL ENABLE MOUNT END (CH %d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

int nf_sunell_enable_dewarp(int ch)
{
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "SUNELL ENABLE DEWARP START (CH %d)\n", ch);

	runtime = get_runtime();

	if(runtime == NULL)
		return IPCAM_SETUP_RTN_FAILED;

	runtime[ch].fisheye.dewarp.support = 0;
	runtime[ch].fisheye.dewarp.support = NF_IPCAM_DEWARP_IP_FISHEYE      | NF_IPCAM_DEWARP_IP_S_PANORAMA   |
										 NF_IPCAM_DEWARP_IP_D_PANORAMA   | NF_IPCAM_DEWARP_IP_4PTZ         |
										 NF_IPCAM_DEWARP_IP_FISHEYE_3PTZ | NF_IPCAM_DEWARP_IP_FISHEYE_5PTZ |
										 NF_IPCAM_DEWARP_IP_FISHEYE_7PTZ;

	runtime[ch].funcs[NF_IPCAM_TYPE_SET_DEWARP] = &nf_sunell_set_dewarp;

	IPCAM_DBG(MINOR, "SUNELL ENABLE DEWARP END (CH %d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}

int nf_sunell_enable_ePTZ(int ch)
{
	mtable *runtime = NULL;

	IPCAM_DBG(MINOR, "SUNELL ENABLE EPTZ START (CH %d)\n", ch);

	runtime = get_runtime();

	if(runtime == NULL)
		return IPCAM_SETUP_RTN_FAILED;

	runtime[ch].funcs[NF_IPCAM_TYPE_SET_PAN_TILT]    = &nf_sunell_move_ePTZ;
	runtime[ch].funcs[NF_IPCAM_TYPE_SET_STOP]        = &nf_sunell_stop_ePTZ;
	runtime[ch].funcs[NF_IPCAM_TYPE_GET_EPTZ_LAYOUT] = &nf_sunell_get_ePTZ_layout;

	IPCAM_DBG(MINOR, "SUNELL ENABLE EPTZ END (CH %d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
}


