/*
 *  ITXM2M
 *  System software group
 *
 *  2012-03-05 jykim
 *  2017-05-22 KimJungJin ( VAPIX2 -> VAPIX3 version up )
 */

#ifndef __NF_IPCAM_DRIVER_AXIS_C__
#define __NF_IPCAM_DRIVER_AXIS_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_driver_axis.h"
#include "nf_ipcam_utils.h"


/* -------------  LOCAL DEFINE   --------------- */


#define PRINT_HTTP_API_SEND		(0)

#define QUERY_LEN				(1024)
#define CURL_TIMEOUT			(4)


#define AXIS_CGI_PATH			"/axis-cgi"
#define AXIS_PARAM_PATH			AXIS_CGI_PATH"/param.cgi"
#define AXIS_PWDGRP_PATH		AXIS_CGI_PATH"/pwdgrp.cgi"
#define AXIS_PTZ_PATH			AXIS_CGI_PATH"/com/ptz.cgi"
#define AXIS_SOFT_FD_PATH		AXIS_CGI_PATH"/factorydefault.cgi"
#define AXIS_RESTART_PATH		AXIS_CGI_PATH"/restart.cgi"

#define AXIS_MOTION_MAX			(10)
#define AXIS_MOTION_COLUMNS		(33)
#define AXIS_MOTION_ROWS		(33)

#define AXIS_MOTION_WIDTH		(9999)
#define AXIS_MOTION_HIGHT		(9999)

#define AXIS_MOTION_AREA_WIDTH	((AXIS_MOTION_WIDTH)/(AXIS_MOTION_COLUMNS))
#define AXIS_MOTION_AREA_HIGHT	((AXIS_MOTION_HIGHT)/(AXIS_MOTION_ROWS))


enum 
{
	AXIS_RTN_OK	= 0,
	AXIS_RTN_FAILED
} AXIS_RTN_TYPE;


/* -------------  LOCAL DATA --------------- */


struct axis_motion_info
{
	int 	top;
	int 	bottom;
	int 	left;
	int 	right;
	int 	sensitivity;
	int 	history;
	int 	objectsize;
};


/* -------------   STATIC FUNCTION   --------------- */

// Util
static void _intType_option_setter(struct DataType* ptr, values* value)
{
	if(ptr != NULL && value != NULL)
	{
		value->min = ptr->iData.min;
		value->max = ptr->iData.max;
	}
}

static int _cam_setup_index_finder(int temp)
{
	int cnt = 0;

	if(temp == 0) 
		return 0;

	while(temp != 1)
	{
		temp /= 2;
		cnt++;
	}

	return cnt;
}

// Motion
static int _axis_get_motion_max(int ch, int *motion_max)
{
	const char		*param_motion_max = "action=list&group=Properties.Motion.MaxNbrOfWindows";
	
    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	icm_str_array	str_array			= NULL;
	int				max_tokens			= 2;

	if(motion_max == NULL)
	{
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_motion_max);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	str_array = icm_str_split((const char *)res.msg, "=", max_tokens);
	if(str_array == NULL)
	{
        IPCAM_DBG(ERROR, "AXIS Motion Properties icm_str_split Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
	}
	else
	{
		*motion_max = atoi(str_array[1]);
	}

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);
	icm_str_array_free(str_array, max_tokens);

	return axis_rtn;
}

static int _axis_get_motion_list(int ch, int *motion_mask)
{
	const char		*param_motion_list = "action=list&group=Motion";
	
    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	if(motion_mask == NULL)
	{
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_motion_list);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	*motion_mask 	= 0;
	char *tmp	= res.msg;
	while((tmp = strstr(tmp, "root.Motion.M")) != NULL)
	{
		tmp += 13;
		(*motion_mask) |= 1<<(tmp[0]-48);
	}

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_remove_motion(int ch, int motion_id)
{
	const char 		*param_remove_motion = "action=remove&group=Motion.M%d";

	icm_http        ctx;
	icm_response    res;
	char            query[QUERY_LEN]	= { 0, };
	const char      *path       		= AXIS_PARAM_PATH;
	int             axis_rtn    		= AXIS_RTN_FAILED;


    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

    // set param
	snprintf(query, sizeof(query), param_remove_motion, motion_id);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
        axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_add_motion(int ch, struct axis_motion_info *ami)
{
	const char 		*param_add_motion = 
					"action=add&group=Motion&"
					"template=motion&Motion.M.Name=Motion&Motion.M.Top=%d&Motion.M.Bottom=%d&"
					"Motion.M.Left=%d&Motion.M.Right=%d&Motion.M.Sensitivity=%d&"
					"Motion.M.History=%d&Motion.M.ObjectSize=%d";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;


	if(ami == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS input param Error\n");
        axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
	}

	// init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_add_motion, 
								   ami->top, 
								   ami->bottom, 
								   ami->left, 
								   ami->right, 
								   ami->sensitivity, 
								   ami->history, 
								   ami->objectsize);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
        axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _aixs_motion_area_to_ami(MAREA *marea, struct axis_motion_info *ami)
{
	if(marea == NULL || ami == NULL)
		return AXIS_RTN_FAILED;

	ami->left		 = marea->FIGURE.RECTANGLE.left_top.x     * (AXIS_MOTION_AREA_WIDTH);
	ami->top		 = marea->FIGURE.RECTANGLE.left_top.y     * (AXIS_MOTION_AREA_HIGHT);
	ami->right		 = marea->FIGURE.RECTANGLE.right_bottom.x * (AXIS_MOTION_AREA_WIDTH) + (AXIS_MOTION_AREA_WIDTH);
	ami->bottom		 = marea->FIGURE.RECTANGLE.right_bottom.y * (AXIS_MOTION_AREA_HIGHT) + (AXIS_MOTION_AREA_HIGHT);
	ami->sensitivity = marea->sensitivity;
	ami->history	 = 90;
	ami->objectsize  = 15;

	return AXIS_RTN_OK;
}

// Image
static int _axis_imagesource_list(int ch)
{
	const char		*param_get_image_list = 
					"action=listdefinitions&listformat=xmlschema&group=ImageSource";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;


    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_get_image_list);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
        axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	if(nf_axis_get_imagesource(res.msg) != 0)
	{
		IPCAM_DBG(ERROR, "AXIS nf_axis_get_imagesource Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_set_image_option(image_t_onvif* image_t)
{
	struct DataType *searcher	= NULL;
	struct DataType *head		= NULL;

	if(image_t == NULL)
	{
		return AXIS_RTN_FAILED;
	}

	image_t->supported_image			= 0;		//IMAGE
	image_t->focus.mode.support			= 0;		//FocusMode
	image_t->ircut.support				= 0;		//IRcutFilter
	image_t->wdrmode.support			= 0;		//WDR control
	image_t->wb.mode.support			= 0;		//WhiteBalance
	image_t->exposure.mode.support		= 0;		//ExposureMode 
	image_t->exposure.priority.support	= 0;		//ExposurePriority
	image_t->supported_exposure			= 0;		//exposure supported
	image_t->exposure.iris_mode.support	= 0;		//DC iris supported

	head = getAxisImageListHead();

	if(head == NULL)
	{
		return AXIS_RTN_FAILED;
	}

	searcher = head->next;

	while(searcher != NULL)
	{
		if(searcher->type == 0)
		{
			if(strcmp("WhiteBalance", searcher->name) == 0)
			{
				image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_WB_MODE;//onvif_NR
				{		
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_AUTO;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_HOLD;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_FIXED_OUT1;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_FIXED_OUT2;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_FIXED_IN;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_FIXED_FLUORE1;
					image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_FIXED_FLUORE2;
					if(searcher->eData.enumCnt > 7)
					{
						image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_AUTO_OUT;
					}
					if(searcher->eData.enumCnt > 8)
					{
						image_t->wb.mode.support |= NF_IPCAM_WB_MODE_AXIS_MANUAL;
					}
				}
				if(strcmp("auto", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_AUTO;
				}
				else if(strcmp("auto_outdoor", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_AUTO_OUT;
				}
				else if(strcmp("hold", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_HOLD;
				}
				else if(strcmp("fixed_outdoor1", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_FIXED_OUT1;
				}
				else if(strcmp("fixed_outdoor2", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_FIXED_OUT2;
				}
				else if(strcmp("fixed_indoor", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_FIXED_IN;
				}
				else if(strcmp("fixed_fluor1", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_FIXED_FLUORE1;
				}
				else if(strcmp("fixed_fluor2", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_FIXED_FLUORE2;
				}
				else if(strcmp("manual", searcher->e_value) == 0)
				{
					image_t->wb.mode.value = NF_IPCAM_WB_MODE_AXIS_MANUAL;
				}
			}
			if(strcmp("IrCutFilter", searcher->name) == 0)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRCUT;//onvif_NR
				{
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_AXIS_AUTO;
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_AXIS_ON;
					image_t->ircut.support |= NF_IPCAM_IRCUT_MODE_AXIS_OFF;
				}
				
				if(strcmp("auto", searcher->e_value) == 0)
					image_t->ircut.value = NF_IPCAM_IRCUT_MODE_AXIS_AUTO;
				if(strcmp("on", searcher->e_value) == 0)
					image_t->ircut.value = NF_IPCAM_IRCUT_MODE_AXIS_ON;
				if(strcmp("off", searcher->e_value) == 0)
					image_t->ircut.value = NF_IPCAM_IRCUT_MODE_AXIS_OFF;
			}
			if(strcmp("Exposure", searcher->name) == 0)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MODE;//onvif_NR
				{
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_AUTO; 
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_FREE_50HZ; 
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_FREE_60HZ; 
					image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_HOLD; 

					if(searcher->eData.enumCnt >= 5) 
					{
						image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_REDUECED_50HZ; 
						image_t->exposure.mode.support |= NF_IPCAM_EXPOSURE_MODE_AXIS_REDUECED_60HZ; 
					}
				}
					
				if(strcmp("auto", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_AUTO;
				if(strcmp("flickerfree50", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_FREE_50HZ;
				if(strcmp("flickerfree60", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_FREE_60HZ;
				if(strcmp("hold", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_HOLD;
				if(strcmp("flickerreduced50", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_REDUECED_50HZ;
				if(strcmp("flickerreduced60", searcher->e_value) == 0)
					image_t->exposure.mode.value = NF_IPCAM_EXPOSURE_MODE_AXIS_REDUECED_60HZ;
			}
			if(strcmp("ExposurePriority", searcher->name) == 0)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_PRIORITY;

				image_t->exposure.priority.support |= NF_IPCAM_PRIORITY_MODE_ONVIF_DEFAULT;//NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE 
				image_t->exposure.priority.support |= NF_IPCAM_PRIORITY_MODE_ONVIF_MOTION;//NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE 
				image_t->exposure.priority.support |= NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE;//NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE 
				
				if(strcmp("50", searcher->e_value) == 0)
					image_t->exposure.priority.value = NF_IPCAM_PRIORITY_MODE_ONVIF_DEFAULT;
				if(strcmp("100", searcher->e_value) == 0)
					image_t->exposure.priority.value = NF_IPCAM_PRIORITY_MODE_ONVIF_MOTION;
				if(strcmp("0", searcher->e_value) == 0)
					image_t->exposure.priority.value = NF_IPCAM_PRIORITY_MODE_ONVIF_LOWNOISE;
			}
		}
		else if(searcher->type == 1)
		{
			if(strcmp("Position", searcher->name) == 0)//!DCIRIS Position 0~100
			{
				_intType_option_setter(searcher, &image_t->exposure.iris); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_IRIS;//need to change 
				image_t->exposure.iris.value = searcher->i_value;
			}
			if(strcmp("MaxExposureTime", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->exposure.maxetime); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXETIME;
				image_t->exposure.maxetime.value = searcher->i_value;
			}
			if(strcmp("MinExposureTime", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->exposure.minetime); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINETIME;
				image_t->exposure.minetime.value = searcher->i_value;
			}
			if(strcmp("MaxGain", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->exposure.maxgain); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN;
				image_t->exposure.maxgain.value = searcher->i_value;
			}
			if(strcmp("MinGain", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->exposure.mingain); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_MINGAIN;
				image_t->exposure.mingain.value = searcher->i_value;
			}
			if(strcmp("Brightness", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &(image_t->brightness)); 
				image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS;
				image_t->brightness.value = searcher->i_value;
			}
			if(strcmp("Contrast", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->contrast); 
				image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_CONTRAST;
				image_t->contrast.value = searcher->i_value;
			}
			if(strcmp("ColorLevel", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->color); 
				image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_COLOR;
				image_t->color.value = searcher->i_value;
			}
			if(strcmp("DynamicContrastLevel", searcher->name) == 0)
			{
				_intType_option_setter(searcher, &image_t->wdrlevel); 
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL;
				image_t->wdrlevel.value = searcher->i_value;
			}
			if(strcmp("Sharpness", searcher->name) == 0)//!
			{
				_intType_option_setter(searcher, &image_t->sharpness); 
				image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_SHARPNESS;
				image_t->sharpness.value = searcher->i_value;
			}
		}
		else if(searcher->type == 2)
		{
			if(strcmp("DynamicContrastEnabled", searcher->name) == 0)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_WDR_MODE;
				image_t->wdrmode.support = NF_IPCAM_WDR_MODE_ONVIF_OFF | NF_IPCAM_WDR_MODE_ONVIF_ON;
				image_t->wdrmode.value = NF_IPCAM_WDR_MODE_ONVIF_OFF;
				if(strcmp("yes", searcher->b_value) == 0)
				{
					image_t->wdrmode.value = NF_IPCAM_WDR_MODE_ONVIF_ON;
				}
			}
			if(strcmp("BacklightCompensation", searcher->name) == 0)
			{
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_ONVIF_BLC_MODE;
				image_t->blcmode.support = NF_IPCAM_BLC_MODE_AXIS_OFF | NF_IPCAM_BLC_MODE_AXIS_ON;
				image_t->blcmode.value = NF_IPCAM_BLC_MODE_AXIS_OFF;
				if(strcmp("yes", searcher->b_value) == 0)
				{
					image_t->blcmode.value = NF_IPCAM_BLC_MODE_AXIS_ON;
				}
			}
			if(strcmp("Enabled", searcher->name) == 0)
			{
				
				image_t->supported_exposure |= NF_IPCAM_EXPOSURE_DCIRIS;
				image_t->exposure.iris_mode.support = NF_IPCAM_IMAGE_DCIRIS_AXIS_ON | NF_IPCAM_IMAGE_DCIRIS_AXIS_OFF;

				image_t->exposure.iris_mode.value = NF_IPCAM_IMAGE_DCIRIS_AXIS_ON;
				
				if(strcmp("no", searcher->b_value) == 0)
				{
					image_t->exposure.iris_mode.value = NF_IPCAM_IMAGE_DCIRIS_AXIS_OFF;
				}
			}
		}
		searcher = searcher->next;
	}

	return AXIS_RTN_OK;
}

static int _axis_set_image(int ch, image_info_onvif* info_set)
{
	int 		wb_index			= 0;
	int 		exp_index			= 0;
	int 		exp_pri_index		= 0;
	int 		wdr_index			= 0;
	int 		blc_index			= 0;
	int 		ircut_index			= 0;
	int 		iris_control_index	= 0;

	const char *wb_mode_str[]		= { "", "", "",	"", "", "", "auto", "auto_outdoor", "hold", "fixed_outdoor1", "fixed_outdoor2", "fixed_indoor", "fixed_fluor1", "fixed_fluor2", "manual" };
	const char *exp_mode_str[]		= { "", "", "", "", "manual", "auto", "auto_out", "auto","auto_m", "auto", "manual", "auto", "auto_m", "manual", "", "", "", "", 
                                        "auto_m", "auto", "manual", "auto_m", "auto", "flickerfree50", "flickerfree60", "hold", "flickerreduced50", "flickerreduced60" };
	const char *exp_pri_mode_str[]	= { "0", "", "50", "100" };
	const char *wdr_mode_str[]		= { "", "no", "yes", "", "", "", "", "", "" };
	const char *blc_mode_str[]		= { "off", "on", "", "adap", "zone", "zone", "zone", "zone", "zone", "off", "on", "yes", "no" };
	const char *ircut_str[]			= { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "auto", "yes", "no" };
	const char *dc_iris_str[]		= { "", "", "", "", "", "", "", "", "yes", "no" };

	const char *param_set_image	= 
		"action=update"
		"&ImageSource.I0.Sensor.WhiteBalance=%s"				//whitebalance
		"&ImageSource.I0.Sensor.Exposure=%s"					//exposure mode
		"&ImageSource.I0.Sensor.ExposurePriority=%s"			//exposure priority
		"&ImageSource.I0.Sensor.DynamicContrastEnabled=%s"		//wdr control
		"&ImageSource.I0.Sensor.BacklightCompensation=%s"		//BLC
		"&ImageSource.I0.DayNight.IrCutFilter=%s"				//IRCUT
		"&ImageSource.I0.Sensor.MaxExposureTime=%d"				//MaxExposureTime
		"&ImageSource.I0.Sensor.MinExposureTime=%d"				//MinExposureTime
		"&ImageSource.I0.Sensor.MaxGain=%d"						//MaxGain
		"&ImageSource.I0.Sensor.MinGain=%d"						//MinGain
		"&ImageSource.I0.Sensor.Brightness=%d"					//Blightness
		"&ImageSource.I0.Sensor.Contrast=%d"					//Contrast 
		"&ImageSource.I0.Sensor.ColorLevel=%d"					//Color Level
		"&ImageSource.I0.Sensor.DynamicContrastLevel=%d"		//WDR Level
		"&ImageSource.I0.Sensor.Sharpness=%d"					//Sharpness
		"&ImageSource.I0.DCIris.Position=%d"					//DC iris Position
		"&ImageSource.I0.DCIris.Enabled=%s";					//DC iris Position
	
	icm_http		ctx;
	icm_response	res;
	char			query[QUERY_LEN]	= { 0, };
	const char		*path				= AXIS_PARAM_PATH;
	int				axis_rtn			= AXIS_RTN_FAILED;

	if(info_set == NULL)
	{
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

	// init ctx , res
	memset(&ctx, 0x00, sizeof(ctx));
	memset(&res, 0x00, sizeof(res));
	icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

    // set param
	wb_index			= _cam_setup_index_finder(info_set->wb.mode);
	exp_index			= _cam_setup_index_finder(info_set->exposure.mode);
	exp_pri_index		= _cam_setup_index_finder(info_set->exposure.priority);
	wdr_index			= _cam_setup_index_finder(info_set->wdrmode);
	blc_index			= _cam_setup_index_finder(info_set->blcmode);
	ircut_index			= _cam_setup_index_finder(info_set->ircut);
	iris_control_index	= _cam_setup_index_finder(info_set->exposure.iris_mode);

    snprintf(query, sizeof(query), param_set_image,
                             	   wb_mode_str[wb_index],
                             	   exp_mode_str[exp_index],
                             	   exp_pri_mode_str[exp_pri_index],
                             	   wdr_mode_str[wdr_index],
                             	   blc_mode_str[blc_index],
                             	   ircut_str[ircut_index],
                             	   info_set->exposure.maxetime,
                             	   info_set->exposure.minetime,
                             	   info_set->exposure.maxgain,
                             	   info_set->exposure.mingain,
                             	   info_set->brightness,
                             	   info_set->contrast,
                             	   info_set->color,
                             	   info_set->wdrlevel,
                             	   info_set->sharpness,
                             	   info_set->exposure.iris,
                             	   dc_iris_str[iris_control_index]);

	if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
	}

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

// Mirror 
static int _axis_image_list(int ch)
{
	const char 		*param_get_mirror_list = 
					"action=listdefinitions&listformat=xmlschema&group=Image.I0";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;


    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_get_mirror_list);
	
    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
        axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	if(nf_axis_get_appearance(res.msg) != 0)
	{
		IPCAM_DBG(ERROR, "AXIS nf_axis_get_appearance FAIL!\n");
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_set_mirror_option(image_t_onvif* image_t, video_t* video)
{
	int 			cnt			= 0;
	int 			m_case		= 0;
	int 			r_case		= 0;

	struct DataType *searcher	= NULL;
	struct DataType *head		= NULL;


	if(image_t == NULL || video == NULL)
	{
		return AXIS_RTN_FAILED;
	}

	image_t->mirror.support = 0;

	head = getAxisImageListHead();
	if(head == NULL)
	{
		return AXIS_RTN_FAILED; 
	}
	
	searcher = head->next;

	while(searcher != NULL)
	{
		if(searcher->type == 0)
		{
			if(strcmp("Rotation", searcher->name) == 0)
			{
				cnt ++;
				if(strcmp("0", searcher->e_value) == 0)
					r_case = 0;
				else if(strcmp("90", searcher->e_value) == 0)
					r_case = 1;
				else if(strcmp("180", searcher->e_value) == 0)
					r_case = 2;
				else if(strcmp("270", searcher->e_value) == 0)
					r_case = 3;
			}
		}
		if(searcher->type == 2)
		{
			if(strcmp("MirrorEnabled", searcher->name) == 0)
			{
				cnt ++;
				m_case = 0;
				if(strcmp("yes", searcher->b_value) == 0)
					m_case = 1;
			}
		}
		searcher = searcher->next;
	}

	if(cnt == 2)
	{
		image_t->supported_image |= NF_IPCAM_IMAGE_MIRRORING;//onvif_NR
		image_t->supported_image |= NF_IPCAM_IMAGE_ONVIF_ROTATION;//onvif_NR
		video->supported |= VIDEO_SETUP_MIRROR;
		video->onthefly |= VIDEO_SETUP_MIRROR;
		video->mirror.support = 0;
		video->mirror.support = NF_IPCAM_MIRROR_AXIS_R0	|
								  NF_IPCAM_MIRROR_AXIS_R90	|
								  NF_IPCAM_MIRROR_AXIS_R180	|
								  NF_IPCAM_MIRROR_AXIS_R270	|	
								  NF_IPCAM_MIRROR_AXIS_M0	|
								  NF_IPCAM_MIRROR_AXIS_M90	|	
								  NF_IPCAM_MIRROR_AXIS_M180	|
								  NF_IPCAM_MIRROR_AXIS_M270 ;

		if(m_case == 0)
		{
			if(r_case == 0)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_R0;
			if(r_case == 1)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_R90;
			if(r_case == 2)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_R180;
			if(r_case == 3)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_R270;
		}
		else
		{
			if(r_case == 0)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_M0;
			if(r_case == 1)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_M90;
			if(r_case == 2)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_M180;
			if(r_case == 3)
				video->mirror.value = NF_IPCAM_MIRROR_AXIS_M270;
		}
	}

	return AXIS_RTN_OK;
}

static int _axis_set_mirror(int ch, cam_info* info)
{
	int 			m_case	= 0;
	int 			r_case	= 0;

	const char		*param_set_mirror = 
					"action=update"
					"&Image.I0.Appearance.Rotation=%s"				//Rotation 0,90,180,270
					"&Image.I0.Appearance.MirrorEnabled=%s";		//Mirror yes,no

	const char		*rotation_str[]	= {"0", "90", "180", "270"};
	const char		*mirror_str[]	= {"no", "yes"};

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(info == NULL)
	{
		axis_rtn = AXIS_RTN_FAILED;
		goto ends_label;
	}

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	switch(info->vcodec.mirror)
	{
		case NF_IPCAM_MIRROR_AXIS_R0:
			r_case = 0;
			m_case = 0;
			break;
		case NF_IPCAM_MIRROR_AXIS_R90:
			r_case = 1;
			m_case = 0;
			break;
		case NF_IPCAM_MIRROR_AXIS_R180:
			r_case = 2;
			m_case = 0;
			break;
		case NF_IPCAM_MIRROR_AXIS_R270:
			r_case = 3;
			m_case = 0;
			break;
		case NF_IPCAM_MIRROR_AXIS_M0:
			r_case = 0;
			m_case = 1;
			break;
		case NF_IPCAM_MIRROR_AXIS_M90:
			r_case = 1;
			m_case = 1;
			break;
		case NF_IPCAM_MIRROR_AXIS_M180:
			r_case = 2;
			m_case = 1;
			break;
		case NF_IPCAM_MIRROR_AXIS_M270:
			r_case = 3;
			m_case = 1;
			break;
		default:
			r_case = 0;
			m_case = 0;
			break;
	}
	snprintf(query, sizeof(query), param_set_mirror, rotation_str[r_case], mirror_str[m_case]);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_set_ptz_enabled(int ch)
{
	const char		*param_set_ptz_enabled = 
					"action=update&PTZ.ImageSource.I0.PTZEnabled=true";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_set_ptz_enabled);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_get_ptz_nbrOfcam(int ch, int *nbrOfcam)
{
	const char		*param_ptz_nbrOfcam =
					"action=list&group=PTZ.NbrOfCameras";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(nbrOfcam == NULL)
    {
        IPCAM_DBG(ERROR, "AXIS input param Error\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_ptz_nbrOfcam);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	// parse nbrOfcamera
	// ex) PTZ.NbrOfCameras=8
	if(res.status == 200 && res.msg != NULL && res.size > 0)
	{
		char *tmp = NULL;
		tmp = strstr(res.msg, "=");
		if(tmp != NULL)
		{
			*nbrOfcam = atoi(tmp+1);
		}
		else
		{
			*nbrOfcam = 0;
		}
	}
	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_set_ptz_unlocked(int ch, int index)
{
	const char		*param_ptz_unlocked =
					"action=update&PTZ.Various.V%d.Locked=false";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ch_init_tout(&ctx, ch, CURL_TIMEOUT);

	// set param
	snprintf(query, sizeof(query), param_ptz_unlocked, index);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

// User
static int _axis_set_user(const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl)
{
	const char		*param_set_user =
					"action=update&user=%s&pwd=%s";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PWDGRP_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(username == NULL || password == NULL)
    {
        IPCAM_DBG(ERROR, "AXIS input param Error\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
	icm_http_ip_init(&ctx, ip, port, username, password, ssl);

	// set param
	snprintf(query, sizeof(query), param_set_user, username, password);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_set_webservice(const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl)
{
	const char		*param_set_webservice =
					"action=update&WebService.UsernameToken.ReplayAttackProtection=no";

    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_PARAM_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(username == NULL || password == NULL)
    {
        IPCAM_DBG(ERROR, "AXIS input param Error\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ip_init(&ctx, ip, port, username, password, ssl);

	// set param
	snprintf(query, sizeof(query), param_set_webservice);

    if(icm_http_new_post_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_post_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_soft_factory_default(const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl)
{
    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_SOFT_FD_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(username == NULL || password == NULL)
    {
        IPCAM_DBG(ERROR, "AXIS input param Error\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ip_init(&ctx, ip, port, username, password, ssl);

    if(icm_http_new_get_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_get_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}

static int _axis_restart_server(const unsigned int ip, const unsigned short port, const char *username, const char *password, const unsigned int ssl)
{
    icm_http        ctx;
    icm_response    res;
    char            query[QUERY_LEN]	= { 0, };
    const char      *path       		= AXIS_RESTART_PATH;
    int             axis_rtn    		= AXIS_RTN_FAILED;

	
	if(username == NULL || password == NULL)
    {
        IPCAM_DBG(ERROR, "AXIS input param Error\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

    // init ctx , res
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
    icm_http_ip_init(&ctx, ip, port, username, password, ssl);

    if(icm_http_new_get_request(&ctx, path, (const char *)query, &res) != ICM_RTN_OK)
    {
        IPCAM_DBG(ERROR, "AXIS icm_http_new_get_request Fail\n");
		axis_rtn = AXIS_RTN_FAILED;
        goto ends_label;
    }

	axis_rtn = AXIS_RTN_OK;

ends_label:

	icm_http_response_free(&res);

	return axis_rtn;
}


/* -------------  IPX Axis Camera API   --------------- */

extern int axis_recv_buf_handler(int ch, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf)
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
		case NF_IPCAM_TYPE_SET_ORIGIN:
		default:
			break;
	}

	return IPCAM_SETUP_RTN_DONE;
}

extern int axis_init_profiles(int ch)
{
#if 0
	mtable *runtime = NULL;
	const char* m_name = "NVR_MAIN_P";
	const char* s_name = "NVR_SUB_P";


	runtime = get_runtime();

	nf_onvif_init_profile(ch, m_name, runtime[ch].onvif.mp_token);
	nf_onvif_init_profile(ch, s_name, runtime[ch].onvif.sp_token);
#endif

	return IPCAM_SETUP_RTN_DONE;
}

extern int axis_set_vcodec_m311x(int ch)
{
	int 	rtn0 	= 0;
	int 	rtn1 	= 0;
	int 	rtn2 	= 0;
	mtable *runtime = NULL;

#if 0
	runtime = get_runtime();

	//rtn0 = nf_onvif_find_resolution(ch, 1440, 900);
	//axis_add_video_source_configuration(ch);

	rtn1 = nf_onvif_set_video_encoder_configuration
			(ch, runtime[ch].onvif.mp_vec_token, 1440, 900, 80, 30, 15, 0);
	rtn2 = nf_onvif_set_video_encoder_configuration
			(ch, runtime[ch].onvif.sp_vec_token, 640, 400, 30, 30, 15, 0);

	if (rtn1 != 0)
	{
		rtn1 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.mp_vec_token, 800, 450, 80, 30, 15, 0);
		rtn2 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.sp_vec_token, 640, 400, 30, 30, 15, 0);
	}

	if (rtn0 == 0)
	{
		rtn1 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.mp_vec_token, 1440, 900, 80, 30, 15, 0);
		rtn2 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.sp_vec_token, 640, 400, 30, 30, 15, 0);
	}
	else
	{
		rtn1 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.mp_vec_token, 800, 450, 80, 30, 15, 0);
		rtn2 = nf_onvif_set_video_encoder_configuration
				(ch, runtime[ch].onvif.sp_vec_token, 640, 400, 30, 30, 15, 0);
	}
#endif

	return IPCAM_SETUP_RTN_DONE;
}

extern int axis_set_vcodec_p3346(int ch)
{
	int 	rtn1 	= 0;
	int 	rtn2 	= 0;
	mtable *runtime = NULL;

#if 0
	runtime = get_runtime();

	//axis_add_video_source_configuration(ch);

	rtn1 = nf_onvif_set_video_encoder_configuration
			(ch, runtime[ch].onvif.mp_vec_token, 1920, 1080, 100, 30, 15, 0);
	rtn2 = nf_onvif_set_video_encoder_configuration
			(ch, runtime[ch].onvif.sp_vec_token, 640, 360, 30, 30, 15, 0);

	//axis_add_video_encoder_configuration(ch);
#endif

	return IPCAM_SETUP_RTN_DONE;
}

// NEW VAPIX 3 
extern int nf_axis_set_motion(NFIPCamSetupMotionArea *motion_info, int ch)
{
	int 						rtn			= IPCAM_SETUP_RTN_FAILED;
	int 						idx			= 0;
	int 						motion_mask	= 0;
	int							motion_max	= 0;
	struct	axis_motion_info 	ami;

	IPCAM_DBG(MINOR, "AXIS SET MOTION START (CH %d)\n", ch);

	if(motion_info == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS input param Error\n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	// motion max count get
	if((_axis_get_motion_max(ch, &motion_max) != AXIS_RTN_OK) || motion_max <= 0)
	{
		IPCAM_DBG(ERROR, "AXIS Get MotionMax Fail (CH %d)\n", ch);
		motion_max = AXIS_MOTION_MAX;
	}

	// legacy motion get
	if(_axis_get_motion_list(ch, &motion_mask) != AXIS_RTN_OK) 
	{
		IPCAM_DBG(ERROR, "AXIS Get MotionList Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	// legacy motion delete
	for(idx = 0; idx < motion_max; idx++) 
	{
		if(((1<<idx) & motion_mask) != 0) 
		{
			_axis_remove_motion(ch, idx);
		}
	}

	// create new motion
	for(idx = 0; idx < motion_info->area_num; idx++) 
	{
		memset(&ami, 0x00, sizeof(ami));
		if(_aixs_motion_area_to_ami(&motion_info->marea[idx], &ami) != AXIS_RTN_OK) 
		{
			IPCAM_DBG(ERROR, "AIXS Area to ami Fail (CH %d)\n", ch);
			rtn = IPCAM_SETUP_RTN_FAILED;
			goto ends_label;
		}

		if(_axis_add_motion(ch, &ami) != AXIS_RTN_OK) 
		{
			IPCAM_DBG(ERROR, "AXIS Add Motion Fail (CH %d)\n", ch);
			rtn = IPCAM_SETUP_RTN_FAILED;
			goto ends_label;
		}
	}

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MOTION, -1);
	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SET MOTION END (CH %d)\n", ch);

	return rtn;
}

extern int nf_axis_get_image(int ch)
{
	int 	axis_rtn		= AXIS_RTN_FAILED;
	int 	rtn				= IPCAM_SETUP_RTN_FAILED;

	mtable 	*runtime 		= get_runtime();
	

	IPCAM_DBG(MINOR, "AXIS GET IMAGE START (CH %d)\n", ch);

	if(runtime == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Get_runtime Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	axis_rtn = _axis_imagesource_list(ch);
	if(axis_rtn != AXIS_RTN_OK) 
	{
		IPCAM_DBG(ERROR, "AXIS Get ImageSource Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	axis_rtn = _axis_set_image_option(&(runtime[ch].image_onvif));
	if(axis_rtn != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS Set Image Option Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}
	nf_axis_image_free();

	axis_rtn = _axis_image_list(ch);
	if(axis_rtn != AXIS_RTN_OK) 
	{
		IPCAM_DBG(ERROR, "AXIS Get Image Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	axis_rtn = _axis_set_mirror_option(&(runtime[ch].image_onvif), &(runtime[ch].video));
	if(axis_rtn != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS Set Mirror Image Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}
	nf_axis_image_free();

	runtime[ch].onvif.onvif_service |= __OFM(NF_ONVIF_SERVICE_IMAGE);	//sysdb_onvif load & cbfunc
	rtn = IPCAM_SETUP_RTN_DONE;


ends_label:

	if(axis_rtn != AXIS_RTN_OK)
	{
		nf_axis_image_free();
	}

	IPCAM_DBG(MINOR, "AXIS GET IMAGE END (CH %d)\n", ch);

	return rtn;
}

extern int nf_axis_set_image(image_info_onvif* info_set, int ch)
{
	int			rtn	= IPCAM_SETUP_RTN_FAILED;


	IPCAM_DBG(MINOR, "AXIS SET IMAGE START (CH %d)\n", ch); 

	if(info_set == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input Param Error (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	if(_axis_set_image(ch, info_set) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_set_image Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_IMAGE_ONVIF, -1);
	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SET IMAGE END (CH %d)\n", ch); 

	return rtn;
}

extern int nf_axis_set_mirror(cam_info* info, int ch)
{
    int			rtn	= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS SET MIRROR START (CH %d)\n", ch);

	if(info == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input param Error\n");
        rtn = IPCAM_SETUP_RTN_FAILED;
        goto ends_label;
	}

	if(_axis_set_mirror(ch, info) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_set_mirror Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MIRROR, -1);
	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SET MIRROR END (CH %d)\n", ch);

	return rtn;
}

extern int nf_axis_init_ptz(int ch)
{
	int			idx			= 0;
	int			nbrOfcam	= 0;
    int			rtn			= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS INIT PTZ START (CH %d)\n", ch);

	if(_axis_set_ptz_enabled(ch) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_set_ptz_enabled Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	if(_axis_get_ptz_nbrOfcam(ch, &nbrOfcam) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_get_ptz_nbrOfcam Fail (CH %d)\n", ch);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	// AXIS Camera Various channel index start 1
	for(idx = 1; idx <= nbrOfcam; idx++)
	{
		_axis_set_ptz_unlocked(ch, idx);
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS INIT PTZ END (CH %d)\n", ch);

	return rtn;
}

extern int nf_axis_set_user_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password)
{
    int			rtn			= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS SET USER START \n");

	if(username == NULL || password == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input param Error\n");
        rtn = IPCAM_SETUP_RTN_FAILED;
        goto ends_label;
	}

	if(_axis_set_user(ip, port, username, password, 0) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_set_user Fail \n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SET USER END \n");

	return rtn;
}

extern int nf_axis_set_webservice_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password)
{
    int			rtn			= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS SET WEB SERVICE START \n");

	if(username == NULL || password == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input param Error\n");
        rtn = IPCAM_SETUP_RTN_FAILED;
        goto ends_label;
	}

	if(_axis_set_webservice(ip, port, username, password, 0) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_set_webservice Fail \n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SET WEB SERVICE END \n");

	return rtn;
}

extern int nf_axis_soft_factory_default_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password)
{
    int			rtn			= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS SOFT FACTORY DEFAULT START \n");

	if(username == NULL || password == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input param Error\n");
        rtn = IPCAM_SETUP_RTN_FAILED;
        goto ends_label;
	}

	if(_axis_soft_factory_default(ip, port, username, password, 0) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_soft_factory_default Fail \n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS SOFT FACTORY DEFAULT END \n");

	return rtn;
}

extern int nf_axis_restart_server_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password)
{
    int			rtn			= IPCAM_SETUP_RTN_FAILED;

	IPCAM_DBG(MINOR, "AXIS RESTART SERVER START \n");

	if(username == NULL || password == NULL)
	{
		IPCAM_DBG(ERROR, "AXIS Input param Error\n");
        rtn = IPCAM_SETUP_RTN_FAILED;
        goto ends_label;
	}

	if(_axis_restart_server(ip, port, username, password, 0) != AXIS_RTN_OK)
	{
		IPCAM_DBG(ERROR, "AXIS _axis_soft_factory_default Fail \n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto ends_label;
	}

	rtn = IPCAM_SETUP_RTN_DONE;

ends_label:

	IPCAM_DBG(MINOR, "AXIS RESTART SERVER END \n");

	return rtn;
}


#endif //__NF_IPCAM_DRIVER_AXIS_C__


