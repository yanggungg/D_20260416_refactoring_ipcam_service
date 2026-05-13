/*
 * scm_ipcam.c
 *  - ip-cam services
 *  - dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 6, 2011
 *
 */

#include "scm.h"
#include "iux_afx.h"
#include "nf_api_ipcam.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif
#include "scm_internal.h"
#include "ix_mem.h"
#include "evt.h"
#include "nf_hdi_camtype.h"

#define DBG_LEVEL       0
#define DBG_MODULE      "SCM_CAM"

#define _MASK(a)    (1ULL << (a))

#if defined(_HDI_MODEL_UX)
#define SD_AREA_ROWS        (0)
#define SD_AREA_COLS        (0)
#define HD_AREA_ROWS        (18)
#define HD_AREA_COLS        (20)
#elif defined(_HDY_MODEL_UX)
#define SD_AREA_ROWS        (24)
#define SD_AREA_COLS        (32)
#define HD_AREA_ROWS        (24)
#define HD_AREA_COLS        (48)
#elif defined(_DVR_MODEL_UX)
#define SD_AREA_ROWS        (12)
#define SD_AREA_COLS        (16)
#define HD_AREA_ROWS        (0)
#define HD_AREA_COLS        (0)
#else
#define SD_AREA_ROWS        (0)
#define SD_AREA_COLS        (0)
#define HD_AREA_ROWS        (8)
#define HD_AREA_COLS        (12)
#endif

static guchar mraw_data[GUI_CHANNEL_CNT][1024] = {0, };

////////////////////////////////////////////////////////////
//
// private functions
//

static int _get_cam_default_resol(int ch, RESOL_INFO_T *presol)
{
    presol->cap |= _MASK(CAP_RES_1920x1080);
    presol->cap |= _MASK(CAP_RES_1280x720);
    presol->cap |= _MASK(CAP_RES_640x352);
    return 0;
}

static int _get_cam_default_fps(int ch, FPS_INFO_T *pfps)
{
    BITMASK signal = scm_get_cam_signal();

    if (signal & (1 << ch))
    {
        pfps->cap |= _MASK(CAP_FPS_25);
        pfps->cap |= _MASK(CAP_FPS_12);
        pfps->cap |= _MASK(CAP_FPS_06);
        pfps->cap |= _MASK(CAP_FPS_01);
    }
    else
    {
        pfps->cap |= _MASK(CAP_FPS_30);
        pfps->cap |= _MASK(CAP_FPS_15);
        pfps->cap |= _MASK(CAP_FPS_07);
        pfps->cap |= _MASK(CAP_FPS_01);
    }

    return 0;
}

static int _trans_ipcam_resol(guint64 in_resol, guint64 *out_resol)
{
    if (in_resol & NF_IPCAM_RES_320x180)        *out_resol |= _MASK(CAP_RES_320x180);
    if (in_resol & NF_IPCAM_RES_320x240)        *out_resol |= _MASK(CAP_RES_UNKNOWN);
    if (in_resol & NF_IPCAM_RES_352x240)        *out_resol |= _MASK(CAP_RES_NTSC_CIF);
    if (in_resol & NF_IPCAM_RES_352x288)        *out_resol |= _MASK(CAP_RES_PAL_CIF);
    if (in_resol & NF_IPCAM_RES_640x352)        *out_resol |= _MASK(CAP_RES_640x352);
    if (in_resol & NF_IPCAM_RES_640x480)        *out_resol |= _MASK(CAP_RES_640x480);
    if (in_resol & NF_IPCAM_RES_704x480)        *out_resol |= _MASK(CAP_RES_NTSC_4CIF);
    if (in_resol & NF_IPCAM_RES_704x576)        *out_resol |= _MASK(CAP_RES_PAL_4CIF);
    if (in_resol & NF_IPCAM_RES_720x480)        *out_resol |= _MASK(CAP_RES_720x480);
    if (in_resol & NF_IPCAM_RES_720x576)        *out_resol |= _MASK(CAP_RES_720x576);
    if (in_resol & NF_IPCAM_RES_1280x720I)      *out_resol |= _MASK(CAP_RES_1280x720I);
    if (in_resol & NF_IPCAM_RES_1280x720)       *out_resol |= _MASK(CAP_RES_1280x720);
    if (in_resol & NF_IPCAM_RES_1024x768)       *out_resol |= _MASK(CAP_RES_1024x768);
    if (in_resol & NF_IPCAM_RES_1280x1024)      *out_resol |= _MASK(CAP_RES_1280x1024);
    if (in_resol & NF_IPCAM_RES_1920x1080I)     *out_resol |= _MASK(CAP_RES_1920x1080I);
    if (in_resol & NF_IPCAM_RES_1920x1080)      *out_resol |= _MASK(CAP_RES_1920x1080);
    if (in_resol & NF_IPCAM_RES_640x360)        *out_resol |= _MASK(CAP_RES_640x360);
    if (in_resol & NF_IPCAM_RES_640x360I)       *out_resol |= _MASK(CAP_RES_640x360I);
    if (in_resol & NF_IPCAM_RES_640x400)        *out_resol |= _MASK(CAP_RES_640x400);
    if (in_resol & NF_IPCAM_RES_800x450)        *out_resol |= _MASK(CAP_RES_800x450);
    if (in_resol & NF_IPCAM_RES_1440x900)       *out_resol |= _MASK(CAP_RES_1440x900);
    if (in_resol & NF_IPCAM_RES_800x600)        *out_resol |= _MASK(CAP_RES_800x600);
    if (in_resol & NF_IPCAM_RES_1600x1200)      *out_resol |= _MASK(CAP_RES_1600x1200);
    if (in_resol & NF_IPCAM_RES_2304x1296)      *out_resol |= _MASK(CAP_RES_2304x1296);
    if (in_resol & NF_IPCAM_RES_2048x1536)      *out_resol |= _MASK(CAP_RES_2048x1536);
    if (in_resol & NF_IPCAM_RES_2560x1440)      *out_resol |= _MASK(CAP_RES_2560x1440);
    if (in_resol & NF_IPCAM_RES_2688x1520)      *out_resol |= _MASK(CAP_RES_2688x1520);
    if (in_resol & NF_IPCAM_RES_2560x1600)      *out_resol |= _MASK(CAP_RES_2560x1600);
    if (in_resol & NF_IPCAM_RES_2560x1920)      *out_resol |= _MASK(CAP_RES_2560x1920);
    if (in_resol & NF_IPCAM_RES_2592x1920)      *out_resol |= _MASK(CAP_RES_2592x1920);
    if (in_resol & NF_IPCAM_RES_2592x1944)      *out_resol |= _MASK(CAP_RES_2592x1944);
    if (in_resol & NF_IPCAM_RES_2992x1680)      *out_resol |= _MASK(CAP_RES_2992x1680);
    if (in_resol & NF_IPCAM_RES_2880x1800)      *out_resol |= _MASK(CAP_RES_2880x1800);
    if (in_resol & NF_IPCAM_RES_3200x1800)      *out_resol |= _MASK(CAP_RES_3200x1800);
    if (in_resol & NF_IPCAM_RES_2880x2160)      *out_resol |= _MASK(CAP_RES_2880x2160);
    if (in_resol & NF_IPCAM_RES_3072x2048)      *out_resol |= _MASK(CAP_RES_3072x2048);
    if (in_resol & NF_IPCAM_RES_3200x2400)      *out_resol |= _MASK(CAP_RES_3200x2400);
    if (in_resol & NF_IPCAM_RES_3840x2160)      *out_resol |= _MASK(CAP_RES_3840x2160);
    if (in_resol & NF_IPCAM_RES_2592x1520)      *out_resol |= _MASK(CAP_RES_2592x1520);
    if (in_resol & NF_IPCAM_RES_3000x3000)      *out_resol |= _MASK(CAP_RES_3000x3000);  
    if (in_resol & NF_IPCAM_RES_2048x2048)      *out_resol |= _MASK(CAP_RES_2048x2048);  
    if (in_resol & NF_IPCAM_RES_1280x1280)      *out_resol |= _MASK(CAP_RES_1280x1280);  
    if (in_resol & NF_IPCAM_RES_640x640)        *out_resol |= _MASK(CAP_RES_640x640);  
    if (in_resol & NF_IPCAM_RES_320x320)        *out_resol |= _MASK(CAP_RES_320x320);  

    return 0;
}

static int _trans_ipcam_fps(guint in_resol, guint *out_resol)
{
    if (in_resol & NF_IPCAM_FPS_300)        *out_resol |= _MASK(CAP_FPS_30);
    if (in_resol & NF_IPCAM_FPS_150)        *out_resol |= _MASK(CAP_FPS_15);
    if (in_resol & NF_IPCAM_FPS_100)        *out_resol |= _MASK(CAP_FPS_UNKNOWN);
    if (in_resol & NF_IPCAM_FPS_70)         *out_resol |= _MASK(CAP_FPS_07);
    if (in_resol & NF_IPCAM_FPS_40)         *out_resol |= _MASK(CAP_FPS_04);
    if (in_resol & NF_IPCAM_FPS_20)         *out_resol |= _MASK(CAP_FPS_02);
    if (in_resol & NF_IPCAM_FPS_10)         *out_resol |= _MASK(CAP_FPS_01);
    if (in_resol & NF_IPCAM_FPS_250)        *out_resol |= _MASK(CAP_FPS_25);
    if (in_resol & NF_IPCAM_FPS_120)        *out_resol |= _MASK(CAP_FPS_12);
    if (in_resol & NF_IPCAM_FPS_60)         *out_resol |= _MASK(CAP_FPS_06);
    if (in_resol & NF_IPCAM_FPS_30)         *out_resol |= _MASK(CAP_FPS_03);

    return 0;
}

static int _get_cam_resol_profile(int ch, RESOL_INFO_T *presol)
{
    guint64 tmp_cap, tmp_cur;

    if (nf_ipcam_get_resol(ch, 0, &tmp_cap, &tmp_cur, 0) == 1)
    {
        _trans_ipcam_resol(tmp_cap, &presol->cap);
        _trans_ipcam_resol(tmp_cur, &presol->cur);
    }
    else
    {
        // g_warning("***** %s, %d, ch:%d, get resol fail", __FUNCTION__, __LINE__, ch);
        _get_cam_default_resol(ch, presol);
    }
    return 0;
}

static int _get_cam_fps_profile(int ch, FPS_INFO_T *pfps)
{
    guint tmp_cap, tmp_cur;
    if (nf_ipcam_get_fps(ch, 0, &tmp_cap, &tmp_cur, 0) == 1)
    {
        _trans_ipcam_fps(tmp_cap, &pfps->cap);
        _trans_ipcam_fps(tmp_cur, &pfps->cur);
    }
    else
    {
        // g_warning("***** %s, %d, ch:%d, get fps fail", __FUNCTION__, __LINE__, ch);
        _get_cam_default_fps(ch, pfps);
    }
    return 0;
}

static int _get_cam_profile(int ch, CAM_PROFILE_T *prof)
{
    int ret;
    ret = nf_ipcam_get_model_info(ch, &prof->model, NULL);
    if (ret == 1) {
        prof->connected = 1;
        nf_ipcam_get_config_info(ch, &prof->conf, NULL);
    }
    else {
        prof->connected = 0;
    }

    _get_cam_resol_profile(ch, &prof->resol);
    _get_cam_fps_profile(ch, &prof->fps);
    return 0;
}

static int _get_cam_resol_profile_HDI(int ch, RESOL_INFO_T *presol)
{
    CameraData camdata;
    int ret;
    CONNENT_CAM_E cam;

    cam = scm_get_cam_connect(ch);

    if (cam == CONNENT_CAM_SD)
    {
        if(DISPLAY_IS_PAL)
        {
            presol->cap |= _MASK(CAP_RES_PAL_4CIF);
            presol->cap |= _MASK(CAP_RES_PAL_2CIF);
            presol->cap |= _MASK(CAP_RES_PAL_CIF);
        }
        else
        {
            presol->cap |= _MASK(CAP_RES_NTSC_4CIF);
            presol->cap |= _MASK(CAP_RES_NTSC_2CIF);
            presol->cap |= _MASK(CAP_RES_NTSC_CIF);
        }
    }
    else
    {
        DAL_get_camera_data(&camdata, ch);

        switch(camdata.type)
        {
            case HDI_VIDEO_IN_TYPE_1920x1080p25:
            case HDI_VIDEO_IN_TYPE_1920x1080p30:
                presol->cap |= _MASK(CAP_RES_1920x1080);
                break;

            case HDI_VIDEO_IN_TYPE_1280x720p25:
            case HDI_VIDEO_IN_TYPE_1280x720p30:
            case HDI_VIDEO_IN_TYPE_1280x720p50:
            case HDI_VIDEO_IN_TYPE_1280x720p60:
                presol->cap |= _MASK(CAP_RES_1280x720);
                break;
        }

        presol->cap |= _MASK(CAP_RES_640x360);
    }

    return 0;
}

static int _get_cam_fps_profile_HDI(int ch, FPS_INFO_T *pfps)
{
    CameraData camdata;
    int ret;
    CONNENT_CAM_E cam;

    cam = scm_get_cam_connect(ch);

    if (cam == CONNENT_CAM_SD)
    {
        if(DISPLAY_IS_PAL)
        {
            pfps->cap |= _MASK(CAP_FPS_25);
            pfps->cap |= _MASK(CAP_FPS_12);
            pfps->cap |= _MASK(CAP_FPS_07);
            pfps->cap |= _MASK(CAP_FPS_03);
            pfps->cap |= _MASK(CAP_FPS_02);
            pfps->cap |= _MASK(CAP_FPS_01);
        }
        else
        {
            pfps->cap |= _MASK(CAP_FPS_30);
            pfps->cap |= _MASK(CAP_FPS_15);
            pfps->cap |= _MASK(CAP_FPS_07);
            pfps->cap |= _MASK(CAP_FPS_03);
            pfps->cap |= _MASK(CAP_FPS_02);
            pfps->cap |= _MASK(CAP_FPS_01);
        }
    }
    else
    {
        DAL_get_camera_data(&camdata, ch);
        switch(camdata.type)
        {
            case HDI_VIDEO_IN_TYPE_1920x1080p30:
            case HDI_VIDEO_IN_TYPE_1280x720p30:
            case HDI_VIDEO_IN_TYPE_1280x720p60:
                pfps->cap |= _MASK(CAP_FPS_30);
                pfps->cap |= _MASK(CAP_FPS_15);
                pfps->cap |= _MASK(CAP_FPS_07);
                pfps->cap |= _MASK(CAP_FPS_03);
                pfps->cap |= _MASK(CAP_FPS_02);
                pfps->cap |= _MASK(CAP_FPS_01);
                break;

            case HDI_VIDEO_IN_TYPE_1920x1080p25:
            case HDI_VIDEO_IN_TYPE_1280x720p25:
            case HDI_VIDEO_IN_TYPE_1280x720p50:
                pfps->cap |= _MASK(CAP_FPS_25);
                pfps->cap |= _MASK(CAP_FPS_12);
                pfps->cap |= _MASK(CAP_FPS_07);
                pfps->cap |= _MASK(CAP_FPS_03);
                pfps->cap |= _MASK(CAP_FPS_02);
                pfps->cap |= _MASK(CAP_FPS_01);
                break;
        }
    }

    return 0;
}

static int _get_cam_profile_HDI(int ch, CAM_PROFILE_T *prof)
{
    NF_NOTIFY_INFO buf;

    scm_get_vloss_data(&buf);

    if (buf.d.params[0] & (1 << ch))
        prof->connected = 0;
    else
        prof->connected = 1;

    _get_cam_resol_profile_HDI(ch, &prof->resol);
    _get_cam_fps_profile_HDI(ch, &prof->fps);

    return 0;
}

static int _get_cam_resol_profile_NetraDVR(int ch, RESOL_INFO_T *presol)
{
    if(DISPLAY_IS_PAL)
    {
        presol->cap |= _MASK(CAP_RES_960x576);
        presol->cap |= _MASK(CAP_RES_PAL_4CIF);
        presol->cap |= _MASK(CAP_RES_PAL_2CIF);
        presol->cap |= _MASK(CAP_RES_PAL_CIF);
    }
    else
    {
        presol->cap |= _MASK(CAP_RES_960x480);
        presol->cap |= _MASK(CAP_RES_NTSC_4CIF);
        presol->cap |= _MASK(CAP_RES_NTSC_2CIF);
        presol->cap |= _MASK(CAP_RES_NTSC_CIF);
    }

    return 0;
}

static int _get_cam_fps_profile_NetraDVR(int ch, FPS_INFO_T *pfps)
{
    if(DISPLAY_IS_PAL)
    {
        pfps->cap |= _MASK(CAP_FPS_25);
        pfps->cap |= _MASK(CAP_FPS_12);
        pfps->cap |= _MASK(CAP_FPS_07);
        pfps->cap |= _MASK(CAP_FPS_03);
        pfps->cap |= _MASK(CAP_FPS_02);
        pfps->cap |= _MASK(CAP_FPS_01);
    }
    else
    {
        pfps->cap |= _MASK(CAP_FPS_30);
        pfps->cap |= _MASK(CAP_FPS_15);
        pfps->cap |= _MASK(CAP_FPS_07);
        pfps->cap |= _MASK(CAP_FPS_03);
        pfps->cap |= _MASK(CAP_FPS_02);
        pfps->cap |= _MASK(CAP_FPS_01);
    }

    return 0;
}

static int _get_cam_profile_NetraDVR(int ch, CAM_PROFILE_T *prof)
{
    NF_NOTIFY_INFO buf;

    scm_get_vloss_data(&buf);

    if (buf.d.params[0] & (1 << ch))
        prof->connected = 0;
    else
        prof->connected = 1;

    _get_cam_resol_profile_NetraDVR(ch, &prof->resol);
    _get_cam_fps_profile_NetraDVR(ch, &prof->fps);

    return 0;
}

static int _get_supp_corridor(int ch)
{
    int ret = 0;
    
    ret = nf_ipcam_get_is_supported_corridor(ch);
    
    return ret;
}

static int _get_corridor_mode(int ch)
{
    int ret = 0;
    int val = 0;
    
    if (!_get_supp_corridor(ch)) return 0;
    
    ret = nf_ipcam_get_corridor_mode(ch, &val);
    
    DMSG(9, "corridor mode : %d", val);
    
    if (ret == 0) val = 0;
    
    return val;
}

static void _reset_ipcam_cb(NFIPCamProgress* progress, gpointer user_data)
{
    static gint rate = 0;

    if(progress) {

        switch(progress->status) {
            case NF_IPCAM_STATUS_BEGIN:
                {
                    rate = 0;
                    _scm_send_data_to_viewer(INFY_IPCAM_RESET_BEGIN, 0, GINT_TO_POINTER(rate));
                }
                break;

            case NF_IPCAM_STATUS_PENDING:
                    _scm_send_data_to_viewer(INFY_IPCAM_RESET_PENDING, 0, NULL);
                break;

            case NF_IPCAM_STATUS_FAILED_REQ:
                _scm_send_data_to_viewer(INFY_IPCAM_RESET_REQ_FAIL, 0, NULL);
                break;

            case NF_IPCAM_STATUS_TIMEOUT:
                _scm_send_data_to_viewer(INFY_IPCAM_RESET_TIMEOUT, 0, NULL);
                break;

            case NF_IPCAM_STATUS_END_SUCCESS:
                {
                    rate = 100;
                    _scm_send_data_to_viewer(INFY_IPCAM_RESET_END, 0, GINT_TO_POINTER(rate));
                }
                break;
        }
    }
}

static void _calibration_ipcam_cb(NFIPCamProgress* progress, gpointer user_data)
{
    static gint rate = 0;

    if(progress) {

        switch(progress->status) {
            case NF_IPCAM_STATUS_BEGIN:
                {
                    rate = 0;
                    _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_BEGIN, 0, GINT_TO_POINTER(rate));
                }
                break;

            case NF_IPCAM_STATUS_PENDING:
                    _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_PENDING, 0, NULL);
                break;

            case NF_IPCAM_STATUS_FAILED_REQ:
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_REQ_FAIL, 0, NULL);
                break;

            case NF_IPCAM_STATUS_TIMEOUT:
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_TIMEOUT, 0, NULL);
                break;

            case NF_IPCAM_STATUS_END_SUCCESS:
                {
                    rate = 100;
                    _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_END, 0, GINT_TO_POINTER(rate));
                }
                break;
        }
    }
}

static void _dc_iris_calibration_ipcam_cb(NFIPCamProgress* progress, gpointer user_data)
{
    static gint rate =0;

    if(progress) {

        switch(progress->status){
            case NF_IPCAM_STATUS_BEGIN:
            {
                rate = 0;
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_BEGIN, 0, GINT_TO_POINTER(rate));
            }
            break;

            case NF_IPCAM_STATUS_PENDING:
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_PENDING, 0, NULL);
                break;

            case NF_IPCAM_STATUS_FAILED_REQ:
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_REQ_FAIL, 0, NULL);
                break;

            case NF_IPCAM_STATUS_TIMEOUT:
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_TIMEOUT, 0, NULL);
                break;
            case NF_IPCAM_STATUS_END_SUCCESS:
            {
                rate = 100;
                _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_END, 0, GINT_TO_POINTER(rate));
            }
            break;
        }
    }

}

static void _onepush_ipcam_cb(NFIPCamProgress* progress, gpointer user_data)
{
    static gint rate = 0;

    if(progress) {

        switch(progress->status) {
            case NF_IPCAM_STATUS_BEGIN:
                {
                    rate = 0;
                    _scm_send_data_to_viewer(INFY_IPCAM_ONEPUSH_BEGIN, 0, GINT_TO_POINTER(rate));
                }
                break;

            case NF_IPCAM_STATUS_PENDING:
                    _scm_send_data_to_viewer(INFY_IPCAM_ONEPUSH_PENDING, 0, NULL);
                break;

            case NF_IPCAM_STATUS_FAILED_REQ:
                _scm_send_data_to_viewer(INFY_IPCAM_ONEPUSH_REQ_FAIL, 0, NULL);
                break;

            case NF_IPCAM_STATUS_TIMEOUT:
                _scm_send_data_to_viewer(INFY_IPCAM_ONEPUSH_TIMEOUT, 0, NULL);
                break;

            case NF_IPCAM_STATUS_END_SUCCESS:
                {
                    rate = 100;
                    _scm_send_data_to_viewer(INFY_IPCAM_ONEPUSH_END, 0, GINT_TO_POINTER(rate));
                }
                break;
        }
    }
}

static void _get_af_mode(guint *mask)
{
    int max_ch = var_get_ch_count();
    CameraData camdata;
    int i, ret;
    CONNENT_CAM_E cam;

    *mask = 0;

    for (i = 0; i < max_ch; i++) {
        cam = scm_get_cam_connect(i);

        if (cam == CONNENT_CAM_SD)
        {
            if(DISPLAY_IS_PAL)
                *mask |= (0x1<<i);
        }
        else
        {
            DAL_get_camera_data(&camdata, i);
            switch(camdata.type)
            {
                case HDI_VIDEO_IN_TYPE_1920x1080p30:
                case HDI_VIDEO_IN_TYPE_1280x720p30:
                case HDI_VIDEO_IN_TYPE_1280x720p60:
                    break;

                case HDI_VIDEO_IN_TYPE_1920x1080p25:
                case HDI_VIDEO_IN_TYPE_1280x720p25:
                case HDI_VIDEO_IN_TYPE_1280x720p50:
                    *mask |= (0x1<<i);
                    break;
            }
        }
    }
}








////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_openmode_req_ip_assign(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    int ret;
    int idx;

    TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
    IMSG ret_msg = iscm.chart[tra].ret_msg;
    NFOpenmodeSetupNetwork *net_info = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;

    idx = piscm->chart[tra].int_data;

    ret = nf_openmode_request_ip_assign(idx, net_info);
    _scm_send_msg_to_viewer(IRPL_OPENMODE_IPSETUP, ret);

    return 0;
}

int _scm_openmode_change_pw(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    int ret;
    int idx;

    TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
    IMSG ret_msg = iscm.chart[tra].ret_msg;

    idx = piscm->chart[tra].int_data;

    ret = nf_openmode_change_password(idx, (piscm->chart[tra].alloc_data));
    _scm_send_msg_to_viewer(IRPL_OPENMODE_CHANGE_PW, ret);

    return 0;
}

int _scm_openmode_port_setup(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    int ret;
    int idx;

    TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
    IMSG ret_msg = iscm.chart[tra].ret_msg;

    NFOpenmodeSetupPorts *port_info = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;

    idx = piscm->chart[tra].int_data;

    ret = nf_openmode_request_port_assign(idx, port_info );
    _scm_send_msg_to_viewer(IRPL_OPENMODE_PORTSETUP, 0);

    return 0;
}

int _scm_init_ipcam_poe_onoff(SCM_T *piscm)
{
    int i;
    gboolean onoff;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    for (i=0; i<GUI_CHANNEL_CNT; i++)
    {
        DAL_get_cam_poe_onoff(&onoff, i);
        scm_set_ipcam_poe_onoff(i, onoff);
    }

    return 0;
}

int _scm_apply_ipcam_poe_onoff()
{
    int i, onoff;
    unsigned int poe_status = 0;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    poe_status = nf_live_get_poe_port_status();

    g_message("%s, %d status:%08X", __FUNCTION__, __LINE__, poe_status);

    for (i=0; i<GUI_CHANNEL_CNT; i++)
    {
        DAL_get_cam_poe_onoff(&onoff, i);

        if (poe_status & (1 << i) != (onoff << i)) {
            scm_set_ipcam_poe_onoff(i, onoff);
        }
    }

	return 0;
}

static gboolean _dciris_calibration_wait_msg(gpointer data)
{
    int ret;

    ret = nf_ipcam_update_dc_iris_cal_state();

    switch(ret)
    {
        case RTN_ERROR:
        {
            _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_FAIL, RTN_ERROR, NULL);

            return FALSE;
        }
        break;
        case RTN_TIMEOUT:
        {
            _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_TIMEOUT, RTN_TIMEOUT, NULL);

            return FALSE;
        }
        break;

        case RTN_END:
        {
            _scm_send_data_to_viewer(INFY_IPCAM_CALIBRATION_SUCCESS, RTN_END, NULL);

            return FALSE;
        }
        break;

        default:
        break;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

CAM_PROFILE_T *scm_new_cam_profile()
{
    CAM_PROFILE_T *prof;
    int ch = var_get_ch_count();
    int i;

    prof = imalloc(sizeof(CAM_PROFILE_T) * ch);
    for (i = 0; i < ch; ++i) {
        memset(&prof[i], 0x00, sizeof(CAM_PROFILE_T));
        _get_cam_profile(i, &prof[i]);
    }

    return prof;
}

int scm_free_cam_profile(CAM_PROFILE_T *prof)
{
    if (prof) ifree(prof);
    return 0;
}

int scm_get_cam_profile(int ch, CAM_PROFILE_T *prof)
{
    memset(prof, 0x00, sizeof(CAM_PROFILE_T));
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
    _get_cam_profile_HDI(ch, prof);
#elif defined(_DVR_MODEL_UX)
    _get_cam_profile_NetraDVR(ch, prof);
#else
    _get_cam_profile(ch, prof);
#endif
    return 0;
}

int scm_get_encoder_capability(int ch, CAM_ENCODER_T *enc)
{
    nf_ipcam_get_encoder_capability(ch, &enc->capinfo);
    _trans_ipcam_resol(enc->capinfo.res_support[0], &enc->cap[0]);
    _trans_ipcam_resol(enc->capinfo.res_support[1], &enc->cap[1]);
    _trans_ipcam_resol(enc->capinfo.res_support[2], &enc->cap[2]);
    _trans_ipcam_resol(enc->capinfo.res_default[0], &enc->cur[0]);
    _trans_ipcam_resol(enc->capinfo.res_default[1], &enc->cur[1]);
    _trans_ipcam_resol(enc->capinfo.res_default[2], &enc->cur[2]);

    return 0;
}

BITMASK scm_get_cam_conn_state()
{
    BITMASK ret = 0;

#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
    NF_NOTIFY_INFO buf;
    scm_get_vloss_data(&buf);
    ret = ~buf.d.params[0];
#else
    CAM_PROFILE_T *prof;
    int ch = var_get_ch_count();
    int i;

    prof = imalloc(sizeof(CAM_PROFILE_T) * ch);
    for (i = 0; i < ch; ++i) {
        memset(&prof[i], 0x00, sizeof(CAM_PROFILE_T));
        _get_cam_profile(i, &prof[i]);
        if (prof[i].connected == 1) ret |= (1 << i);
    }
    ifree(prof);
#endif
    return ret;
}
//onvif_porting
BITMASK scm_get_cam_signal()
{
    BITMASK signal_mask;

//  nf_ipcam_get_af_mode(&signal_mask);

    if (DAL_get_sig_type())
        signal_mask = 0xffffffff;
    else
        signal_mask = 0;

    return signal_mask;
}
//onvif_porting
int scm_cam_reset(gint ch)
{
    if(!nf_ipcam_reboot (ch, _reset_ipcam_cb, NULL, NULL))
        return -1;
    return 0;
}

int scm_get_ipcam_image_supp(int ch, guint *supp)
{
    int ret = 0;
#if defined(_IPX_MODEL_UX)
    ret = nf_ipcam_get_image_supported_func(ch, supp, NULL);
#endif
    if (ret == IPCAM_SETUP_RTN_DONE)
        return 0;

    *supp = 0xffff;
    return -1;
}

int scm_set_ipcam_image(NFIPCamSetupColor *info)
{
#if !defined(_IPX_MODEL_UX)
    if (nf_cam_set_attr(info->ch, info->brightness, info->contrast, info->tint, info->color))
        return 0;
#else
    if(nf_ipcam_set_color(info->ch, info, NULL, NULL, NULL))
        return 0;
#endif
    return -1;
}

int scm_set_ipcam_image_advanced(NFIPCamSetupImage *info)
{
    if(nf_ipcam_set_image(info->ch, info, NULL, NULL, NULL))
        return 0;
    return -1;
}

int scm_set_ipcam_rotation(int ch, int rotate)
{
    if(nf_ipcam_set_rotation(ch, rotate, NULL, NULL, NULL))
        return 0;
    return -1;
}

int scm_set_ipcam_antiflicker(int ch, int sig)
{
#if defined(_IPX_MODEL_UX)
//  if(nf_ipcam_set_ntpal(ch, sig, NULL, NULL, NULL))
//      return 0;
#endif
    return -1;
}

int scm_req_ipcam_calibration(int ch)
{
    if(nf_ipcam_set_lens_default(ch, _calibration_ipcam_cb, NULL, NULL))
        return 0;
    return -1;
}

int scm_req_ipcam_dc_iris_calibration(int ch)
{
    if(nf_ipcam_set_dc_iris_calibration(ch, _dc_iris_calibration_ipcam_cb, NULL, NULL))
    {
        g_timeout_add(5*1000, _dciris_calibration_wait_msg, NULL);

        return 0;
    }

    return -1;
}

int scm_req_ipcam_onepush(int ch)
{
    if(nf_ipcam_set_oneshot(ch, _onepush_ipcam_cb, NULL, NULL))
        return 0;
    return -1;
}

int scm_get_ipcam_auxiliary_supp(int ch)
{
    int i;

    int ret;
    unsigned int val = 0;

    ret = nf_ipcam_get_ptz_support(ch,NF_IPCAM_IMAGE_AUXILIARY,&val);

    if(ret == IPCAM_SETUP_RTN_DONE){
        if(val) return 0;
    }
    
    return -1;
}

int scm_get_ipcam_ptz_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    if (DAL_get_ptz_rs485_supported((guint)ch))
        return 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_PAN, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }

    return -1;
}

int scm_get_ipcam_preset_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    if(DAL_get_ptz_rs485_supported((guint)ch))
        return 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_PRESET, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }

    return -1;
}

int scm_get_ipcam_zoom_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    if(DAL_get_ptz_rs485_supported((guint)ch))
        return 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_ZOOM, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }
    
    return -1;
}

int scm_get_ipcam_focus_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    if(DAL_get_ptz_rs485_supported((guint)ch))
        return 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_FOCUS, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }
    return -1;
}

int scm_get_ipcam_iris_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    if(DAL_get_ptz_rs485_supported((guint)ch))
        return 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_PIRIS, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if(val) return 0;

        //if (val & NF_IPCAM_IMAGE_PIRIS) return 0;
    }
    return -1;
}

int scm_get_ipcam_onepush_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_ONEPUSH, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }
    return -1;
}

int scm_get_ipcam_calibration_supp(int ch)
{
    int i;
    //int cnt = var_get_ch_count();
    int ret;
    unsigned int val = 0;

    ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_CALIBRATION, &val);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val) return 0;
    }
    return -1;
}

int scm_get_ipcam_vca_supp(int ch)
{
    int i;
    int ret;
    unsigned int val = 0;

    ret = nf_ipcam_get_supported_func(ch, &val, NULL);

    if (ret == IPCAM_SETUP_RTN_DONE) {
        if (val & NF_IPCAM_FUNC_VA) return 0;
    }

    return -1;
}

int scm_get_ipcam_dva_supp(int ch)
{
    int i;
    int ret;
    unsigned int val = 0;

    if (nf_api_is_ai_camera(ch)) return 0;

    return -1;
}

int scm_get_ipcam_ai_type(int ch)
{
    int ret;

    ret = nf_ipcam_get_cam_ai_type(ch);
    return ret;
}

int scm_get_cam_type_HDI(int ch, gchar type[32])
{
    int ret;
    CONNENT_CAM_E cam;

#if defined(_HDI_MODEL_UX)
    memset(type, 0x00, 32);

    cam = scm_get_cam_connect(ch);

    if (cam == CONNENT_CAM_SD)
    {
        if(DISPLAY_IS_PAL)
            strcpy(type, "SD(PAL)");
        else
            strcpy(type, "SD(NTSC)");
    }
    else
    {
        ret = nf_live_get_video_standard(ch-NUM_SD_CH);

        if (ret == -1)  return -1;

        switch(ret) {
            case HDI_VIDEO_IN_TYPE_1920x1080p60:
                strcpy(type, "1920x1080p/60");
                break;
            case HDI_VIDEO_IN_TYPE_1920x1080i60:
                strcpy(type, "1920x1080i/60");
                break;
            case HDI_VIDEO_IN_TYPE_1680x1050p60:
                strcpy(type, "1680x1050p/60");
                break;
            case HDI_VIDEO_IN_TYPE_1280x720p60:
                strcpy(type, "1280x720p/60");
                break;
            case HDI_VIDEO_IN_TYPE_1920x1080p30:
                strcpy(type, "1920x1080p/30");
                break;
            case HDI_VIDEO_IN_TYPE_1280x720p30:
                strcpy(type, "1280x720p/30");
                break;
            case HDI_VIDEO_IN_TYPE_1920x1080p25:
                strcpy(type, "1920x1080p/25");
                break;
            case HDI_VIDEO_IN_TYPE_1280x720p50:
                strcpy(type, "1280x720p/50");
                break;
            case HDI_VIDEO_IN_TYPE_1280x720p25:
                strcpy(type, "1280x720p/25");
                break;
        }
    }
#endif

    return 0;
}

CONNENT_CAM_E scm_get_cam_connect(int ch)
{
#if defined(_HDI_MODEL_UX)
    if (ch < NUM_SD_CH)
        return CONNENT_CAM_SD;
#endif

    return CONNENT_CAM_HDSDI;
}

#if defined(_IPX_MODEL_UX)
int scm_get_ipcam_motion_profile(int ch, NFIPCamMotionProfile* info)
{
    int ret;
    int tmp = 0;

    nf_ipcam_sync_ai_motion(ch);
    ret = nf_ipcam_get_motion_profile(ch, info, NULL);

    if (ret == IPCAM_SETUP_RTN_DONE)
    {      
        return 0;
    }

    return -1;
}
#endif

#if defined(_IPX_MODEL_UX)
int scm_get_ipcam_status(int ch, NFIPCamStatusInfo* info)
{
    int ret;

    ret = nf_ipcam_status_check(ch, info, NULL);

    if (ret == IPCAM_SETUP_RTN_DONE)
        return 0;

    return -1;
}
#endif

#if defined(_IPX_MODEL_UX)
int scm_ipcam_is_ptz(int ch)
{
    NFIPCamModelInfo info;
    int ret;

    ret = nf_ipcam_get_model_info(ch, &info, NULL);

    if (ret == 1)
    {
        // must have 7 or 6fps
        if ( !strcmp(info.name, "NPT-2003"))
            return 0;
        if ( !strcmp(info.name, "NPTi-2003"))
            return 0;
        if ( !strcmp(info.name, "LTV-ISDNI20-M2"))
            return 0;
        if ( !strcmp(info.name, "LTV-ISDNO20-M2"))
            return 0;
    }

    return -1;
}
#endif

int scm_ipcam_is_fisheye(int ch)
{
    NFIPCamTypeProfile prof;
    int ret;
    
    memset(&prof, 0x00, sizeof(NFIPCamTypeProfile));
    ret = nf_ipcam_get_camera_profile(ch, &prof, NULL, NULL);

    if (ret == IPCAM_SETUP_RTN_FAILED)
    {
        DMSG(9, "nf_ipcam_get_camera_profile FAIL");
        return 0;
    }

//    DMSG(1, "prof.cam_type : %d", prof.cam_type);
    if (prof.cam_type == NF_IPCAM_CAM_TYPE_FISHEYE) return 1;
    g_message("%s => %d", __FUNCTION__, prof.cam_type);
    return 0;
}

int scm_get_motion_size(int ch, int *rows, int *cols)
{
#if defined(_IPX_MODEL_UX)
    NFIPCamMotionProfile mot_profile;

    if (scm_get_ipcam_motion_profile(ch, &mot_profile) == 0)
    {
        *rows = mot_profile.block_height;
        *cols = mot_profile.block_width;
    }
    else
    {
        *rows = HD_AREA_ROWS;
        *cols = HD_AREA_COLS;
    }
#else
    if (scm_get_cam_connect(ch) == CONNENT_CAM_SD)
    {
        *rows = SD_AREA_ROWS;
        *cols = SD_AREA_COLS;
    }
    else
    {
        *rows = HD_AREA_ROWS;
        *cols = HD_AREA_COLS;
    }
#endif
    return 0;
}

#if defined(_IPX_MODEL_UX)
int scm_get_mdraw_method(int ch, MOTION_AREA_METHOD_E *method)
{
    NFIPCamMotionProfile mot_profile;

    if (scm_get_ipcam_motion_profile(ch, &mot_profile) == 0)
        *method = mot_profile.area_method;
    else
        *method = MAM_NONE;

    return 0;
}

int scm_get_ipcam_motion_rect_num(int ch, int *count)
{
    NFIPCamMotionProfile mot_profile;

    if (scm_get_ipcam_motion_profile(ch, &mot_profile) == 0) {
        *count = mot_profile.rect_num;
        return 0;
    }else {
        *count = -1;
    }

    return -1;
}

#endif

int _scm_set_mraw_data(NF_NOTIFY_INFO *pinfo)
{
    NFIPCamMotionRaw mo;

    memcpy(&mo, pinfo->p.ptr, sizeof(NFIPCamMotionRaw));

    if ((mo.ch < 0) || (mo.ch >= GUI_CHANNEL_CNT))
    {
        g_message("%s, %d, ch:%d, unsupported channel", __FUNCTION__, __LINE__, mo.ch);
        return -1;
    }

    memcpy(mraw_data[mo.ch], mo.mraw, sizeof(guchar)*256);

    return 0;
}

int scm_get_mraw_data(int ch, guchar data[1024])
{
    memcpy(data, mraw_data[ch], sizeof(guchar)*1024);
    return 0;
}

int scm_request_ipcam_factory_default()
{
    int i;

    for (i = 0; i< var_get_ch_count(); i++)
        nf_ipcam_factory_default(i, 0, 0, 0);

    return 0;
}

int scm_support_direct_config(int ch)
{
    if (DAL_get_cam_install_mode() == 0)        // cctv mode
    {
        if (nf_ipcam_is_dconf_support(ch))
            return 0;
    }

    return -1;
}

int scm_start_ipcam_install_mode()
{
    int ret;

    ret = nf_ipcam_install_mode_on();

    if (ret == IPCAM_SETUP_RTN_DONE)
        return 0;

    return -1;
}

int scm_stop_ipcam_install_mode()
{
    int ret;

    ret = nf_ipcam_install_done();

    if (ret == IPCAM_SETUP_RTN_DONE)
        return 0;

    return -1;
}

int scm_openmode_ipsetup(int index, NFOpenmodeSetupNetwork* info, IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_OPENMODE_CAM_SETUP;
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    iscm.chart[tra].int_data = index;
    iscm.chart[tra].void_data = imalloc(sizeof(NFOpenmodeSetupNetwork));
    memcpy(iscm.chart[tra].void_data, info, sizeof(NFOpenmodeSetupNetwork));

    _scm_put_message(iNFY_OPENMODE_IPSETUP, 0, 0, (void *)tra);

    return 0;
}

int scm_openmode_change_pw(int index, gchar *pw, IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_OPENMODE_CAM_SETUP;
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    iscm.chart[tra].int_data = index;
    iscm.chart[tra].alloc_data = imalloc(strlen(pw) + 1);
    strcpy(iscm.chart[tra].alloc_data, pw);

    _scm_put_message(iNFY_OPENMODE_CHANGE_PW, 0, 0, (void *)tra);

    return 0;
}

int scm_openmode_portsetup(int index, NFOpenmodeSetupPorts* info, IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_OPENMODE_CAM_SETUP;
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    iscm.chart[tra].int_data = index;
    iscm.chart[tra].void_data = imalloc(sizeof(NFOpenmodeSetupPorts));
    memcpy(iscm.chart[tra].void_data, info, sizeof(NFOpenmodeSetupPorts));

    _scm_put_message(iNFY_OPENMODE_PORTSETUP, 0, 0, (void *)tra);

    return 0;
}

int scm_set_ipcam_poe_onoff(int ch, int onoff)
{
    int ret;

    g_message("%s, %d called, onoff:%d", __FUNCTION__, __LINE__, onoff);

    nf_live_poe_port_onoff(ch, (gboolean)onoff, &ret, 1);

    return ret == 0 ? 0 : -1;
}

int scm_support_ipcam_focus_profile(gint ch, NFIPCamFocusCompProfile* profile)
{
    int ret;

    ret = nf_ipcam_get_focus_profile(ch, profile);

    return ret == 1? 1 : 0;
}

int scm_ipcam_get_max_stream_ratio(gint ch, gint *width_ratio, gint *height_ratio)
{
    int ratio = nf_ipcam_get_max_resol_ratio(ch);
    g_message("%s, %d, ratio:%d", __FUNCTION__, __LINE__, ratio);

    if (ratio == RESOL_4_3) {
        *width_ratio = 4;
        *height_ratio = 3;
    }
    else {
        *width_ratio = 16;
        *height_ratio = 9;
    }

    return 0;
}

int scm_ipcam_get_main_stream_ratio(gint ch, gint *width_ratio, gint *height_ratio)
{
    if (nf_ipcam_get_ch_stream_ratio(ch, 0, width_ratio, height_ratio) == IPCAM_SETUP_RTN_FAILED)
    {
        *width_ratio = 16;
        *height_ratio = 9;
        return -1;
    }

    return 0;
}

int scm_get_ipcam_corridor_supp(int ch)
{
    return _get_supp_corridor(ch);
}

int scm_get_ipcam_corridor_mode(int ch)
{
    return _get_corridor_mode(ch);
}

int scm_set_ipcam_eosd_noshow_toggle(int ch, int toggle, char *call_func)
{
    int ret = 0;
    
    ret = nf_ipcam_set_embedded_osd_mode(ch, toggle);
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

//onvif_porting
int onvif_get_cam_resol_profile(int ch, NFIPCamEncoderCap* info)
{
	int ret=0;
	ret = nf_ipcam_get_encoder_capability(ch, info);
	return ret;
}

int onvif_get_cam_resol_default_profile(int ch, RESOL_INFO_T *presol)
{
	int ret=0;
	ret = _get_cam_resol_profile(ch, presol);
	return ret;
}
//onvif_porting
