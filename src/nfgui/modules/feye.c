/*
 * feye.c
 *  - fisheye param module
 *
 * written by parangi
 *
 */

#include <string.h>
#include <glib.h>

#include "nfdal.h"
#include "feye.h"
#include "evt.h"
#include "tmr.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"FEYE"



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _VPARAM_T{
    gint modified;    
    NF_FISHEYE_VIDEO_PARAM p;
} VPARAM_T;

typedef struct _PPARAM_T{
    gint modified;    
    NF_FISHEYE_PTZ_PARAM p;
} PPARAM_T;

typedef struct _FISHEYE_T{
    TIMERID tmrid;	
    gint dewarp_ch;
	VPARAM_T vparam[16];
    PPARAM_T pparam[16];
} FISHEYE_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static FISHEYE_T ifeye;



////////////////////////////////////////////////////////////
//
// private functions
//


static gint _get_vparam_trans_data(VPARAM_T *param, ItxTmpFisheyeData *data)
{
	if (!param->modified) return -1;

//	data->dewarp = param->p.enable;
	data->viewtype = param->p.view_type;

	return 0;
}

static gint _get_pparam_trans_data(PPARAM_T *param, ItxTmpFisheyeData *data)
{
	gint i;

	if (!param->modified) return -1;

	for (i = 0; i < MAX_FISHEYE_VTYPE; i++) 
	{
		if (i < param->p.max_view)
		{
			data->pan[i] = param->p.view[i].pan;
			data->tilt[i] = param->p.view[i].tilt;
			data->zoom[i] = param->p.view[i].zoom;
			data->roll[i] = param->p.view[i].roll;
		}
	}

	return 0;
}

static gboolean _proc_flush_fisheye_data(gpointer data)
{
	FISHEYE_T *pifeye = (FISHEYE_T *)data;

    ItxTmpFisheyeData pre_data;
    ItxTmpFisheyeData post_data;
    gint i, is_changed = 0;

    memset(&pre_data, 0x00, sizeof(ItxTmpFisheyeData));
    memset(&post_data, 0x00, sizeof(ItxTmpFisheyeData));

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_itx_tmp_fisheye_data(&pre_data, i);
        g_memmove(&post_data, &pre_data, sizeof(ItxTmpFisheyeData));

        if (i == pifeye->dewarp_ch) post_data.dewarp = 1;
        else post_data.dewarp = 0;

        _get_vparam_trans_data(&pifeye->vparam[i], &post_data);
		_get_pparam_trans_data(&pifeye->pparam[i], &post_data);

        if (memcmp(&pre_data, &post_data, sizeof(ItxTmpFisheyeData)) != 0)
        {
            DAL_set_itx_tmp_fisheye_data(post_data, i);
            is_changed = 1;
        }
    }

    if (is_changed) {
        DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);
    }

    return FALSE;
}

static int _start_save_timer(FISHEYE_T *pifeye)
{
	tmr_stop(pifeye->tmrid);
	tmr_reset(pifeye->tmrid);
	tmr_start(pifeye->tmrid);
	return 0;
}

static int _stop_save_timer(FISHEYE_T *pifeye)
{
	tmr_stop(pifeye->tmrid);
	tmr_reset(pifeye->tmrid);
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int feye_init()
{
	memset(&ifeye, 0x00, sizeof(FISHEYE_T));
	ifeye.tmrid = tmr_new(300, CMMPT_EVT, INFY_FLUSH_FISHEYE_PARAM);
	return 0;
}

int feye_set_dewarping_channel(int ch)
{
    ifeye.dewarp_ch = ch;
    _start_save_timer(&ifeye);
    return 0;
}

int feye_set_video_param_data(int ch, NF_FISHEYE_VIDEO_PARAM *param)
{
    if (param) {
        ifeye.vparam[ch].modified = 1;      
        ifeye.dewarp_ch = ch;
        memcpy(&ifeye.vparam[ch].p, param, sizeof(NF_FISHEYE_VIDEO_PARAM));
		_start_save_timer(&ifeye);
    }
    return 0;
}

int feye_set_ptz_param_data(int ch, NF_FISHEYE_PTZ_PARAM *param)
{
    if (param) {
        ifeye.pparam[ch].modified = 1;        
        ifeye.dewarp_ch = ch;
        memcpy(&ifeye.pparam[ch].p, param, sizeof(NF_FISHEYE_PTZ_PARAM));
		_start_save_timer(&ifeye);
    }
    return 0;
}

int feye_block_save_data()
{
	_stop_save_timer(&ifeye);
	return 0;
}

int feye_flush_save_data()
{
    g_message("%s, %d", __FUNCTION__, __LINE__);
	_stop_save_timer(&ifeye);
	g_timeout_add(200, _proc_flush_fisheye_data, &ifeye);
	return 0;
}
