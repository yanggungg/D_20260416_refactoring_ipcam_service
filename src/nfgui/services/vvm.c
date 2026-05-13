/*
 * vvm.c
 *        - dependency :
 *
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#include "nf_afx.h"
#include "vw_vwnd.h"

#include "../support/util.h"

#include "vsm.h"
#include "vsm_internal.h"

#include "vw_vwnd.h"
#include "ix_conf.h"
#include "../viewers/vw_vwnd.h"
#include "dtf.h"
#include "ssm.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
// #include <gst/nf/gstmrtpsrc.h>
#else
#include "ipx_cam_api.h"
#endif
#include "nf_dva_event.h"
#include "evt.h"
#include "var.h"

#include "vw.h"
#include "dit.h"
#include "vaa.h"
#include "dvaa.h"
#include "dvaa_itx.h"
#include "vw_dit_vca.h"
#include "vw_dit_dva.h"

#include "pos.h"


#define DBG_LEVEL       9
#define DBG_MODULE      "VVM"

#define STR_INVASION        "RULE_INVASION"
#define STR_LOITERING       "RULE_LOITERING"
#define STR_ABANDON         "RULE_ABANDON"
#define STR_STEAL           "RULE_STEAL"
#define STR_TOPPLE          "RULE_TOPPLE"
#define STR_FENCE           "RULE_FENCE"
#define STR_COUNT           "RULE_COUNT"
#define STR_TAMPERING       "RULE_TAMPERING"
#define STR_PRIVACY         "RULE_PRIVACY"

#define STR_CNTR    " : %d"

static gchar *vca_name[] = {STR_INVASION, STR_LOITERING, STR_ABANDON, STR_STEAL,
        STR_TOPPLE, STR_FENCE, STR_COUNT, STR_TAMPERING, STR_PRIVACY};

////////////////////////////////////////////////////////////
//
// private data type
//

typedef struct _CH_INFO_T {
    gchar       title_text[VWND_TEXT_LEN+1];
    gchar       alarm_text[VWND_TEXT_LEN+1];
    gchar       rec_mode;
    gboolean    audio;
    gboolean    mic;
    gboolean    freeze;
    gboolean    motion;
    gboolean    covert;
    gboolean    no_video;
    gboolean    vloss;
    gboolean    alarm_evt;
    gint        pnd_err;
    gint        pnd_rate;
    gint        pb_status;
    guint       vca_nbit;   // vca active rule bitmask
    gchar       vca_text[VWND_VCA_MAX_CNT][VWND_TEXT_LEN+1];
    guint       vca_ebit;   // vca event occur bitmask
    gint        vca_cntr[IVCA_MAX_CNTRS];
    guint       dvabx_nbit;   // dvabx event occur bitmask
    guint       dvabx_ebit;   // dvabx event occur bitmask
    gint        dvabx_cntr[IVCA_MAX_CNTRS];
    guint       dlva_cntr_person;
    guint       dlva_cntr_vehicle;
    guint       dlva_cntr_animal;
    gchar       debug_text[VWND_DEBUG_LINE][128];
    POSX_T      *lv_posx;
    POSX_T      *pb_posx;
	gint        corridor_mode;
	gint        stream_ratio_w;
	gint        stream_ratio_h;
} CH_INFO_T;

typedef struct _VVM_T {
    CH_INFO_T   cinfo[GUI_CHANNEL_CNT];

//  gchar       lv_user[VWND_TEXT_LEN+1];
//  gboolean    lv_alarm;
//  gboolean    lv_net_count;
//  gint        lv_disk_usage;
//  gint        lv_arch_rate;
} VVM_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static VVM_T ivvm;

static VWND_T lv_mode;
static VWND_T pb_mode;

#ifndef GUI_32CH_SUPPORT
static guint prev_alarm_status = 0;
#else
static guint prev_dvr_al_status = 0;
static guint prev_cam_al_status = 0;
#endif
static gint prev_fan_status = 0;
static gint prev_ipconflict_status =0;
static gint prev_temperature_status = 0;
static gint prev_poe_status = 0;
static gint prev_poe_hub_status = 0;
static gint ipconflict_status = 0;

static void _print_vvm_vwnd_data_dump(void);


////////////////////////////////////////////////////////////
//
// private functions
//

static void _vwnd_init_data(VWND_T *pmode)
{
    g_return_if_fail(pmode != NULL);

    memset(pmode, 0, sizeof(VWND_T));

}

static VWND_T *_vwnd_get_curr_mode()
{
    if (vsm_get_vmode() == VMODE_LV)
        return &lv_mode;
    else
        return &pb_mode;
}

static gint _vwnd_cam_title_update(VWND_T *pmode, gchar win_id, gchar *text)
{
    OsdData osddata;
    gchar preBuf[VWND_TEXT_LEN+1];

    g_return_if_fail(win_id != -1);

    memset(preBuf, 0x00, VWND_TEXT_LEN+1);
    strcpy(preBuf, pmode->winfo[win_id].title_text);

    DAL_get_osd_data(&osddata);
    memset(pmode->winfo[win_id].title_text, 0x00, VWND_TEXT_LEN+1);

    if (osddata.camTitle)
        strcpy(pmode->winfo[win_id].title_text, text);

    if (!strcmp(pmode->winfo[win_id].title_text, preBuf))
        return 0;

    return 1;
}

static gint _vwnd_video_status_update(VWND_T *pmode, gint ch, gchar win_id, VIDEO_ST vst)
{
    VIDEO_ST pre_vst;

    g_return_if_fail(win_id != -1);

    pre_vst = pmode->winfo[win_id].video_st;

    if (vst == VST_FAIL_LOGIN)
    {
        if(ssm_get_cur_idx() < 0)
        {
            if(ivvm.cinfo[ch].no_video)
                vst = VST_NO_VIDEO;
            else
                vst = VST_VLOSS;
        }
    }

    pmode->winfo[win_id].video_st = vst;

    if (pmode->winfo[win_id].video_st == pre_vst)
        return 0;

    return 1;
}

static gint _vwnd_alarm_text_update(VWND_T *pmode, gint ch, gchar win_id, gboolean status, gchar *text)
{
    gchar preBuf[VWND_TEXT_LEN+1];

    g_return_if_fail(win_id != -1);

    memset(preBuf, 0x00, VWND_TEXT_LEN+1);
    strcpy(preBuf, pmode->winfo[win_id].alarm_text);

    memset(pmode->winfo[win_id].alarm_text, 0, VWND_TEXT_LEN+1);

    if (status)
    {
        strcpy(pmode->winfo[win_id].alarm_text, text);

        if (ivvm.cinfo[ch].covert)
        {
            if (!ssm_check_access_auth(USR_AUTH_COVERT))
                memset(pmode->winfo[win_id].alarm_text, 0, VWND_TEXT_LEN+1);
        }
    }


    if (!strcmp(pmode->winfo[win_id].alarm_text, preBuf))
        return 0;

    return 1;
}

static gint _vwnd_motion_update(VWND_T *pmode, gint ch, gchar win_id, gboolean status)
{
    OsdData osddata;
    gboolean pre_motion;
    MotionData motdata;
    gboolean ret;

    g_return_if_fail(win_id != -1);

    pre_motion = pmode->winfo[win_id].motion;
    pmode->winfo[win_id].motion = FALSE;

    ret = DAL_get_motionsensor_detect(ch);

    if (ret)
    {
        if (status)
        {
            pmode->winfo[win_id].motion = status;

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT))
                    pmode->winfo[win_id].motion = FALSE;
            }
        }
    }

    if (pre_motion == pmode->winfo[win_id].motion)
        return 0;

    return 1;
}


static gint _vwnd_rec_mode_update(VWND_T *pmode, gint ch, gchar win_id, gchar rec_mode)
{
    OsdData osddata;
    RECORD_ICON pre_rec;

    g_return_if_fail(win_id != -1);

    pre_rec = pmode->winfo[win_id].rec_mode;
    pmode->winfo[win_id].rec_mode = REC_ICON_NONE;

    DAL_get_osd_data(&osddata);

    if (osddata.evtIcon)
    {
        switch(rec_mode) {
            case 'p':
                pmode->winfo[win_id].rec_mode = REC_ICON_PRE;
            break;
            case 'T':
                pmode->winfo[win_id].rec_mode = REC_ICON_CONT;
            break;
            case 'A':
                pmode->winfo[win_id].rec_mode = REC_ICON_ALARM;
            break;
            case 'M':
                pmode->winfo[win_id].rec_mode = REC_ICON_MOT;
            break;
            case 'P':
                pmode->winfo[win_id].rec_mode = REC_ICON_PANIC;
            break;
            case ' ':
                pmode->winfo[win_id].rec_mode = REC_ICON_NONE;
            break;
        }

        if (ivvm.cinfo[ch].covert) {
            if (!ssm_check_access_auth(USR_AUTH_COVERT))
                pmode->winfo[win_id].rec_mode = REC_ICON_NONE;
        }
    }

    if (pre_rec == pmode->winfo[win_id].rec_mode)
        return 0;

    return 1;
}

static gint _vwnd_audio_update(VWND_T *pmode, gint ch, gchar win_id, gboolean status)
{
    OsdData osddata;
    gboolean pre_audio;

    g_return_if_fail(win_id != -1);

    pre_audio = pmode->winfo[win_id].audio;
    pmode->winfo[win_id].audio = FALSE;

    DAL_get_osd_data(&osddata);

    if (osddata.audio)
    {

        if (status)
        {
            pmode->winfo[win_id].audio = status;

            if(!DAL_get_support_audio())
                pmode->winfo[win_id].audio = FALSE;

            if (scm_get_audio_in_supp(ch) == -1)
                pmode->winfo[win_id].audio = FALSE;

            if (!ssm_check_access_auth(USR_AUTH_AUDIO))
                pmode->winfo[win_id].audio = FALSE;

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT))
                    pmode->winfo[win_id].audio = FALSE;
            }
        }
    }

    if (pre_audio == pmode->winfo[win_id].audio)
        return 0;

    return 1;
}

static gint _vwnd_mic_update(VWND_T *pmode, gint ch, gchar win_id, gboolean status)
{
    OsdData osddata;
    gboolean pre_mic;

    g_return_if_fail(win_id != -1);

    pre_mic = pmode->winfo[win_id].mic;
    pmode->winfo[win_id].mic = FALSE;

    DAL_get_osd_data(&osddata);

    if (osddata.audio)
    {
        if (status)
        {
            pmode->winfo[win_id].mic = status;

            if (scm_get_mic_out_supp(ch) == -1)
                pmode->winfo[win_id].mic = FALSE;

            if (!ssm_check_access_auth(USR_AUTH_MIC))
                pmode->winfo[win_id].mic = FALSE;

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT))
                    pmode->winfo[win_id].mic = FALSE;
            }
        }
    }

    if (pre_mic == pmode->winfo[win_id].mic)
        return 0;

    return 1;
}

static gint _vwnd_pnd_rate_update(VWND_T *pmode, gint ch, gchar win_id, gint rate)
{
    gint pre_rate;

    g_return_if_fail(win_id != -1);

    pre_rate = pmode->winfo[win_id].pnd_rate;

    if (ivvm.cinfo[ch].covert)
        pmode->winfo[win_id].pnd_rate = 0;
    else
        pmode->winfo[win_id].pnd_rate = rate;

    if (pre_rate == pmode->winfo[win_id].pnd_rate)
        return 0;

    return 1;
}

static gint _vwnd_pnd_err_update(VWND_T *pmode, gint ch, gchar win_id, gint err)
{
    gboolean pre_loginfail;

    g_return_val_if_fail(win_id != -1, 0);

    pre_loginfail = pmode->winfo[win_id].login_fail;

    if ( (ivvm.cinfo[ch].covert) || (err != PERR_LOGIN_FAIL) || (ssm_get_cur_idx() < 0))
        pmode->winfo[win_id].login_fail = FALSE;
    else
        pmode->winfo[win_id].login_fail = TRUE;

    if (pre_loginfail == pmode->winfo[win_id].login_fail)
        return 0;

    return 1;
}

static gint _vwnd_live_set_alarm_status_update(gboolean status)
{
    ALARM_ST pre_alarm_st;

    pre_alarm_st = lv_mode.alarm_st;

    if (status) lv_mode.alarm_st = AST_ON;
    else lv_mode.alarm_st = AST_OFF;

    if (pre_alarm_st == lv_mode.alarm_st)
        return 0;

    return 1;
}

static gint _vwnd_connecting_text_update(VWND_T *pmode, gint ch, gchar win_id, gboolean is_connect)
{
    gboolean pre_connecting;

    g_return_if_fail(win_id != -1);

    pre_connecting = pmode->winfo[win_id].is_connecting;

    if (ivvm.cinfo[ch].covert)
        pmode->winfo[win_id].is_connecting = FALSE;
    else
        pmode->winfo[win_id].is_connecting = is_connect;

    if (pre_connecting == pmode->winfo[win_id].is_connecting)
        return 0;

    return 1;
}

static gint _vwnd_live_set_net_count(gint cnt)
{
    NET_ST pre_net_st;
    guint pre_cnt;

    pre_net_st = lv_mode.net_st;
    pre_cnt = lv_mode.net_cnt;

    if (cnt) lv_mode.net_st = NST_NORMAL;
    else lv_mode.net_st = NST_ERROR;

    lv_mode.net_cnt = cnt;

    if(ipconflict_status) lv_mode.net_st = NST_CONFLICT;

    if ((pre_net_st == lv_mode.net_st) && (pre_cnt == lv_mode.net_cnt))
        return 0;

    return 1;
}

static gint _vwnd_live_set_net_status(gboolean status)
{
    NET_ST pre_net_st;

    pre_net_st = lv_mode.net_st;
    lv_mode.net_st = status;

    if(ipconflict_status) lv_mode.net_st = NST_CONFLICT;

    if (pre_net_st == lv_mode.net_st)
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_usage(gint usage)
{
    DISK_ST pre_disk_st;
    guint pre_usage;

    pre_usage = lv_mode.disk_usage;
    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_usage = usage;
    lv_mode.disk_st |= (1 << DST_NORMAL);

    if ((pre_disk_st == lv_mode.disk_st) && (pre_usage == lv_mode.disk_usage))
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_full()
{
    DISK_ST pre_disk_st;
    guint pre_usage;

    pre_usage = lv_mode.disk_usage;
    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_usage = 100;
    lv_mode.disk_st |= (1 << DST_FULL);

    if ((pre_disk_st == lv_mode.disk_st) && (pre_usage == lv_mode.disk_usage))
        return 0;

    return 1;
}

static gint _vwnd_live_unset_disk_full()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st &= ~(1<< DST_FULL);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_overwrite()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

        lv_mode.disk_st |= (1<< DST_OW);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_unset_disk_overwrite()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st &= ~(1<< DST_OW);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_smart()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st |= (1 << DST_NEEDCHECK);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_unset_disk_smart()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st &= ~(1 << DST_NEEDCHECK);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_smart_err()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st |= (1 << DST_SMT_ERR);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_unset_disk_smart_err()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st &= ~(1 << DST_SMT_ERR);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static gint _vwnd_live_set_disk_raid(gint status)
{
    RAID_ST pre_raid_st;

    pre_raid_st = lv_mode.raid_st;

    lv_mode.raid_st = status;

    if (pre_raid_st == lv_mode.raid_st)
        return 0;

    return 1;
}


static gint _vwnd_live_set_no_disk()
{
    DISK_ST pre_disk_st;

    pre_disk_st = lv_mode.disk_st;

    lv_mode.disk_st |= (1 << DST_NO);

    if (pre_disk_st == lv_mode.disk_st)
        return 0;

    return 1;
}

static void _vwnd_live_set_arch_rate(gint rate)
{

}

static gint _vwnd_border_update(VWND_T *pmode)
{
    OsdData osddata;
    guint pre_border;
    guint pre_border_color;

    pre_border = pmode->border;
    pre_border_color = pmode->border_color;

    DAL_get_osd_data(&osddata);
    pmode->border = osddata.border;
    pmode->border_color = osddata.borderColor;

    if ((pre_border == pmode->border) && (pre_border_color == pmode->border_color))
        return 0;

    return 1;
}

static gint _vwnd_debug_text_update(VWND_T *pmode, gchar win_id, gint line, gchar *text)
{
    gchar preBuf[128];

    g_return_if_fail(win_id != -1);

    memset(preBuf, 0x00, 128);
    strcpy(preBuf, pmode->winfo[win_id].debug_text[line]);

    memset(pmode->winfo[win_id].debug_text[line], 0x00, 128);
    strcpy(pmode->winfo[win_id].debug_text[line], text);

    if (!strcmp(pmode->winfo[win_id].debug_text[line], preBuf))
        return 0;

    return 1;
}

static gint _vwnd_vcaline_status_update(VWND_T *pmode, gchar win_id, VSM_DIV_E div)
{
    g_return_if_fail(win_id != -1);

    if (div == VSM_DIV1)
        pmode->winfo[win_id].vcaline = TRUE;
    else
        pmode->winfo[win_id].vcaline = FALSE;

    return 1;
}

static gint _get_itx_vca_nbit(gint ch)
{
    VCAData vca_data;

    DAL_get_vca_data(&vca_data, ch);
    if (!vca_data.prop.active)  return 0;

    return 0xffff;
}

static guint _get_s1_vca_nbit(gint ch)
{
    VCAData vca_data;
    gint i;
    guint mask = 0;

    DAL_get_vca_data(&vca_data, ch);

    if (!vca_data.prop.active)  return mask;

    for (i = 0; i < IVCA_MAX_ZONES; i++)
    {
        if (vca_data.zonelist.zone[i].active)
        {
            mask |= (1 << i);
        }
    }

    if (vca_data.prop.en_tamper)
        mask |= (1 << 7);   // vca tamper rule idx

    return mask;
}


static gint _vwnd_vca_info_update(gint ch)
{
    if (var_get_vendor_code() != 30)
    {
        ivvm.cinfo[ch].vca_nbit = _get_itx_vca_nbit(ch);
    }
    else
    {
        ivvm.cinfo[ch].vca_nbit = _get_s1_vca_nbit(ch);
    }

    return 1;
}

static guint _update_itx_vca_event(VWND_T *pmode, gint ch, gchar win_id, guint vca_ebit)
{
    gint pre_vcaIcon[VWND_VCA_MAX_CNT];

    gint i, cnt = 0;
    guint ret_val = 0;

    g_return_if_fail(win_id != -1);

    for (i = 0; i < VWND_VCA_MAX_CNT; i++)
    {
        pre_vcaIcon[i] = pmode->winfo[win_id].vca_itx_icon[i];
        pmode->winfo[win_id].vca_itx_icon[i] = 0;
    }

    if (ivvm.cinfo[ch].covert == FALSE)
    {
        for (i = 0; i < IVCA_MAX_ZONES; i++)
        {
            if (ivvm.cinfo[ch].vca_nbit & (1 << i))
            {
                if (cnt >= VWND_VCA_MAX_CNT) g_assert(0);

                if (vca_ebit & (1 << i))
                {
                    pmode->winfo[win_id].vca_itx_icon[cnt] = ITX_VCA_ICON_DIR_NEG_N+i*2+1;
                    cnt++;
                }
            }
        }
    }

    for (i = 0; i < VWND_VCA_MAX_CNT; i++)
    {
        if (pmode->winfo[win_id].vca_itx_icon[i] != pre_vcaIcon[i])
        {
            ret_val |= (1 << i);
        }
    }

    return ret_val;
}

static guint _update_s1_vca_event(VWND_T *pmode, gint ch, gchar win_id, guint vca_ebit)
{
    gchar pre_vcaBuf[VWND_VCA_MAX_CNT][VWND_TEXT_LEN+1];
    gint pre_vcaIcon[VWND_VCA_MAX_CNT];

    gint i, cnt = 0;
    guint ret_val = 0;
    gchar tmpBuf[16];

    g_return_if_fail(win_id != -1);

    for (i = 0; i < VWND_VCA_MAX_CNT; i++)
    {
        memset(pre_vcaBuf[i], 0x00, VWND_TEXT_LEN+1);

        pre_vcaIcon[i] = pmode->winfo[win_id].vca_s1_icon[i];
        strcpy(pre_vcaBuf[i], pmode->winfo[win_id].vca_text[i]);

        pmode->winfo[win_id].vca_s1_icon[i] = 0;
        memset(pmode->winfo[win_id].vca_text[i], 0x00, VWND_TEXT_LEN+1);
    }

    if (ivvm.cinfo[ch].covert == FALSE)
    {
        for (i = 0; i < IVCA_MAX_ZONES; i++)
        {
            if (ivvm.cinfo[ch].vca_nbit & (1 << i))
            {
                if (cnt >= VWND_VCA_MAX_CNT) g_assert(0);

                if (vca_ebit & (1 << i))
                {
                    pmode->winfo[win_id].vca_s1_icon[cnt] = S1_VCA_ICON_INVASION_N+i*2+1;
                    strcpy(pmode->winfo[win_id].vca_text[cnt], lookup_string(vca_name[i]));

                    if (ivvm.cinfo[ch].vca_cntr[i]) {
                        memset(tmpBuf, 0x00, 16);
                        g_sprintf(tmpBuf, STR_CNTR, ivvm.cinfo[ch].vca_cntr[i]);
                        strcat(pmode->winfo[win_id].vca_text[cnt], tmpBuf);
                    }
                }
            }
            else { // nomal
                pmode->winfo[win_id].vca_s1_icon[cnt] = S1_VCA_ICON_INVASION_N+i*2;
            }

            cnt++;
        }
    }

    for (i = 0; i < VWND_VCA_MAX_CNT; i++)
    {
        if ((strcmp(pmode->winfo[win_id].vca_text[i], pre_vcaBuf[i]))
            || (pmode->winfo[win_id].vca_s1_icon[i] != pre_vcaIcon[i]))
        {
            ret_val |= (1 << i);
        }
    }

    return ret_val;
}

static guint _vwnd_vca_event_update(VWND_T *pmode, gint ch, gchar win_id, guint vca_ebit)
{
    guint ret;

    if (var_get_vendor_code() != 30)
    {
        ret = _update_itx_vca_event(pmode, ch, win_id, vca_ebit);
    }
    else
    {
        ret = _update_s1_vca_event(pmode, ch, win_id, vca_ebit);
    }

    return ret;
}

static gint _update_vca_itx_cntr(VWND_T *pmode, gint ch, gchar win_id, gint ruleid, gint vca_cntr)
{

    return -1;
}

static gint _update_vca_s1_cntr(VWND_T *pmode, gint ch, gchar win_id, gint ruleid, gint vca_cntr)
{
    gchar pre_vcaBuf[VWND_TEXT_LEN+1];
    gint pre_vcaIcon;

    gint i, cnt = 0;
    gint idx = -1;
    gchar tmpBuf[16];

    for (i = 0; i < IVCA_MAX_ZONES; i++)
    {
        if (ivvm.cinfo[ch].vca_nbit & (1 << i))
        {
            if (i == ruleid) idx = cnt;
            cnt++;
        }
    }

    if (idx == -1) return 0;

    pre_vcaIcon = pmode->winfo[win_id].vca_s1_icon[idx];
    strcpy(pre_vcaBuf, pmode->winfo[win_id].vca_text[idx]);

    pmode->winfo[win_id].vca_s1_icon[idx] = 0;
    memset(pmode->winfo[win_id].vca_text[idx], 0x00, VWND_TEXT_LEN+1);

    if (ivvm.cinfo[ch].covert == FALSE)
    {
        if (ivvm.cinfo[ch].vca_ebit & (1 << ruleid))
        {
            pmode->winfo[win_id].vca_s1_icon[idx] = 1+ruleid*2+1;
            strcpy(pmode->winfo[win_id].vca_text[idx], lookup_string(vca_name[ruleid]));

            if (vca_cntr) {
                memset(tmpBuf, 0x00, 16);
                g_sprintf(tmpBuf, STR_CNTR, vca_cntr);
                strcat(pmode->winfo[win_id].vca_text[idx], tmpBuf);
            }
        }
        else {
            pmode->winfo[win_id].vca_s1_icon[idx] = 1+ruleid*2;
        }
    }

    if ((strcmp(pmode->winfo[win_id].vca_text[idx], pre_vcaBuf))
        && (pmode->winfo[win_id].vca_s1_icon[idx] != pre_vcaIcon))
    {
        return -1;
    }

    return idx;
}

static gint _vwnd_vca_cntr_update(VWND_T *pmode, gint ch, gchar win_id, gint ruleid, gint vca_cntr)
{
    gint ret;

    if (var_get_vendor_code() != 30)
    {
        ret = _update_vca_itx_cntr(pmode, ch, win_id, ruleid, vca_cntr);
    }
    else
    {
        ret = _update_vca_s1_cntr(pmode, ch, win_id, ruleid, vca_cntr);
    }

    return ret;
}

static gint _vwnd_dvabxline_status_update(VWND_T *pmode, gchar win_id, VSM_DIV_E div)
{
    g_return_if_fail(win_id != -1);

    if (div == VSM_DIV1)
        pmode->winfo[win_id].dvabxline = TRUE;
    else
        pmode->winfo[win_id].dvabxline = FALSE;

    return 1;
}

static gint _get_itx_dvabx_nbit(gint ch)
{
    DvaBxData dvabx_data;

    DAL_get_dvabx_data(&dvabx_data, ch);
    if (!dvabx_data.prop.active)  return 0;

    return 0xffff;
}

static gint _vwnd_dvabx_info_update(gint ch)
{
    ivvm.cinfo[ch].dvabx_nbit = _get_itx_dvabx_nbit(ch);
    return 1;
}

static guint _update_itx_dvabx_event(VWND_T *pmode, gint ch, gchar win_id, guint dvabx_ebit)
{
    gint pre_dvabxIcon[VWND_DVABX_MAX_CNT];

    gint i, cnt = 0;
    guint ret_val = 0;

    g_return_if_fail(win_id != -1);

    for (i = 0; i < VWND_DVABX_MAX_CNT; i++)
    {
        pre_dvabxIcon[i] = pmode->winfo[win_id].dvabx_itx_icon[i];
        pmode->winfo[win_id].dvabx_itx_icon[i] = 0;
    }

    if (ivvm.cinfo[ch].covert == FALSE)
    {
        for (i = 0; i < VWND_DVABX_MAX_CNT; i++)
        {
            if (ivvm.cinfo[ch].dvabx_nbit & (1 << i))
            {
                if (dvabx_ebit & (1 << i))
                {
                    pmode->winfo[win_id].dvabx_itx_icon[cnt] = ITX_DVABX_ICON_DIR_NEG_N+i*2+1;
                    cnt++;
                }               
            }
        }
    }

    for (i = 0; i < VWND_DVABX_MAX_CNT; i++)
    {
        if (pmode->winfo[win_id].dvabx_itx_icon[i] != pre_dvabxIcon[i])
        {
            ret_val |= (1 << i);
        }
    }

    return ret_val;
}

static guint _vwnd_dvabx_event_update(VWND_T *pmode, gint ch, gchar win_id, guint dvabx_ebit)
{
    guint ret;

    ret = _update_itx_dvabx_event(pmode, ch, win_id, dvabx_ebit);
    return ret;
}

static gint _update_dvabx_itx_cntr(VWND_T *pmode, gint ch, gchar win_id, gint ruleid, gint dvabx_cntr)
{

    return -1;
}

static gint _vwnd_dvabx_cntr_update(VWND_T *pmode, gint ch, gchar win_id, gint ruleid, gint dvabx_cntr)
{
    gint ret;

    ret = _update_dvabx_itx_cntr(pmode, ch, win_id, ruleid, dvabx_cntr);
    return ret;
}

static gint _vwnd_deeplearning_cntr_update(VWND_T *pmode, gint ch, gchar win_id, guint person_cnt, guint vehicle_cnt, guint animal_cnt)
{
    DVAPropData prop_data;
    gchar item_list[512];

	DLVA_CNTR_ICON 	pre_icon[3];
	guint			pre_val[3];
    gint i, idx = 0;

    for (i = 0; i < 3; i++)
    {
        pre_icon[i] = pmode->winfo[win_id].dlva_cntr_icon[i];
        pre_val[i] = pmode->winfo[win_id].dlva_cntr_val[i];
        pmode->winfo[win_id].dlva_cntr_icon[i] = 0;
        pmode->winfo[win_id].dlva_cntr_val[i] = 0;
    }

    memset(&prop_data, 0x00, sizeof(DVAPropData));
    DAL_get_dva_prop_data(&prop_data, ch);

    if ((ivvm.cinfo[ch].covert != 1)
        && (prop_data.active == 1)
        && (prop_data.idz.active == 1)
        && (prop_data.idz.cntr.active == 1)
        && (prop_data.idz.cntr.display_color != 0))
    {
        if (strstr(prop_data.idz.human_item_list, ":1")) {
            pmode->winfo[win_id].dlva_cntr_icon[idx] = DLVA_CNTR_ICON_HUMAN;
            pmode->winfo[win_id].dlva_cntr_val[idx] = person_cnt;
            idx++;
        }

        if (strstr(prop_data.idz.vehicle_item_list, ":1")) {
            pmode->winfo[win_id].dlva_cntr_icon[idx] = DLVA_CNTR_ICON_VEHICLE;
            pmode->winfo[win_id].dlva_cntr_val[idx] = vehicle_cnt;
            idx++;        
        }

        if (strstr(prop_data.idz.animal_item_list, ":1")) {
            pmode->winfo[win_id].dlva_cntr_icon[idx] = DLVA_CNTR_ICON_ANIMAL;
            pmode->winfo[win_id].dlva_cntr_val[idx] = animal_cnt;
            idx++;        
        }    
    }

    for (i = 0; i < 3; i++)
    {
        if (pre_icon[i] != pmode->winfo[win_id].dlva_cntr_icon[i]) return 1;
        if (pre_val[i] != pmode->winfo[win_id].dlva_cntr_val[i]) return 1;
    }

    return 0;
}

static gint _set_vca_ditid()
{
	VAAID vaaid;
	DITID ditid;
	gint ch;

	VCAData vca_data;

	if (!vaa_is_supported()) return -1;
    if (vsm_get_vmode() != VMODE_LV) return -1;

#if 0
	if (vsm_get_div() != VSM_DIV1)
	{
		vw_dit_display_set_vca_ditid(0);
		return -1;
	}

    ch = vsm_get_current_ch(0);

    if (ivvm.cinfo[ch].covert)
    {
		vw_dit_display_set_vca_ditid(0);
		return -1;        
    }

    DAL_get_vca_data(&vca_data, ch);
    if (!vca_data.prop.active)
	{
		vw_dit_display_set_vca_ditid(0);
		return -1;
	}

	if(scm_get_ipcam_vca_supp(ch))
	{
	    vw_dit_display_set_vca_ditid(0);
	    return -1;
	}

	vaaid = vaa_get_vaaid(ch);
	vaa_activate_all_rule(vaaid);
	vaa_activate_meta(vaaid, VAA_META_BBOX);
	vaa_deactivate_calb(vaaid);

	ditid = vaa_get_ditid(vaaid);
	vw_dit_display_set_vca_ditid(ditid);
#endif
	return 0;
}

static gint _set_dva_ditid()
{
	DVAAID dvaaid;
	DITID ditid;
	gint ch;

	DvaBxData dvabx_data;

	if (!dvaa_is_supported()) return -1;
    if (vsm_get_vmode() != VMODE_LV) return -1;

#if 0
	if (vsm_get_div() != VSM_DIV1)
	{
		vw_dit_display_set_dva_ditid(0);
		return -1;
	}

    ch = vsm_get_current_ch(0);

    if (ivvm.cinfo[ch].covert)
    {
		vw_dit_display_set_dva_ditid(0);
		return -1;        
    }

    DAL_get_dvabx_data(&dvabx_data, ch);
    if (!dvabx_data.prop.active)
	{
		vw_dit_display_set_dva_ditid(0);
		return -1;
	}

	dvaaid = dvaa_get_dvaaid(ch);
	dvaa_activate_all_rule(dvaaid);
	dvaa_activate_meta(dvaaid, DVAA_META_BBOX);
	dvaa_deactivate_calb(dvaaid);

	ditid = dvaa_get_ditid(dvaaid);
	vw_dit_display_set_dva_ditid(ditid);
#endif
	return 0;
}

static void _vwnd_update_area_queue_draw(GdkRectangle *area)
{
    GtkWidget *widget = NULL;

    widget = vwnd_get_main_widget();
    g_return_if_fail(widget != NULL);

//  g_message("%s, %d, area-> x:%d, y:%d, w:%d, h:%d",
//      __FUNCTION__, __LINE__, area->x, area->y, area->width, area->height);

    gdk_window_invalidate_rect(widget->window, area, TRUE);
}

static void _vwnd_update_area_process_updates()
{
    GtkWidget *widget = NULL;

    widget = vwnd_get_main_widget();
    g_return_if_fail(widget != NULL);

    gdk_window_process_updates(widget->window, TRUE);
}

////////////////////////////////////////////////////////////
//
// private interfaces (common)
//

void _vvm_init()
{
    guint ch;
    gint i;

    DVA_OBJ_COUNTER *dlva_cntr;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    memset(&ivvm, 0, sizeof(VVM_T));
    memset(&lv_mode, 0, sizeof(VWND_T));
    memset(&pb_mode, 0, sizeof(VWND_T));

    for (ch = 0; ch <GUI_CHANNEL_CNT; ch++ )
    {
        var_get_camtitle(ivvm.cinfo[ch].title_text, ch);
        DAL_get_almSen_name(ch, ivvm.cinfo[ch].alarm_text);
        _vwnd_vca_info_update(ch);
        _vwnd_dvabx_info_update(ch);

        ivvm.cinfo[ch].audio = FALSE;
        ivvm.cinfo[ch].rec_mode = 0;
        ivvm.cinfo[ch].mic = FALSE;
        ivvm.cinfo[ch].freeze = FALSE;
        ivvm.cinfo[ch].motion = FALSE;
        ivvm.cinfo[ch].covert = FALSE;
        ivvm.cinfo[ch].no_video = TRUE;
        ivvm.cinfo[ch].vloss = TRUE;
        ivvm.cinfo[ch].alarm_evt = FALSE;
        ivvm.cinfo[ch].pnd_err = PERR_NONE;
        ivvm.cinfo[ch].pnd_rate = 0;
        ivvm.cinfo[ch].pb_status = -1;
        ivvm.cinfo[ch].lv_posx = posx_create(ch);
        ivvm.cinfo[ch].pb_posx = posx_create(32+ch);
        ivvm.cinfo[ch].corridor_mode = 0;

        dlva_cntr = nf_dva_get_obj_count(ch);
        ivvm.cinfo[ch].dlva_cntr_person = dlva_cntr->person;
        ivvm.cinfo[ch].dlva_cntr_vehicle = dlva_cntr->vehicle;
        ivvm.cinfo[ch].dlva_cntr_animal = dlva_cntr->animal;
    }

    lv_mode.play_st = DR_NONE;
    lv_mode.alarm_st = AST_OFF;
    lv_mode.net_st = NST_NONE;
    lv_mode.disk_st = DST_NONE;
    lv_mode.raid_st = RST_NORMAL;
    lv_mode.arch_st = AST_NONE;

    pb_mode.play_st = DR_NONE;
    pb_mode.alarm_st = AST_NONE;
    pb_mode.net_st = NST_NONE;
    pb_mode.disk_st = DST_NONE;
    lv_mode.raid_st = RST_NORMAL;
    pb_mode.arch_st = ARST_NONE;

    lv_mode.osd_off = FALSE;
    pb_mode.osd_off = FALSE;

    lv_mode.fill = FILL_ON;
    pb_mode.fill = FILL_ON;

    lv_mode.plt_w = MODE_FULL_ACTIVE_W;
    lv_mode.plt_h = MODE_FULL_ACTIVE_H;

    if (DAL_get_disk_write_mode())
        lv_mode.disk_st |= (1<< DST_OW);

    lv_mode.pos_onoff = posx_get_live_display_onoff();
    pb_mode.pos_onoff = posx_get_playback_display_onoff();

    if (scm_is_clon_device() == 0)
    {
        lv_mode.clondev = 1;
        pb_mode.clondev = 1;
    }

    posx_get_pos_conf(&lv_mode.pos_cf);
    posx_get_pos_conf(&pb_mode.pos_cf);

    vwnd_set_sfc_mode(&lv_mode);
    vwnd_set_item_position();
}


void _vvm_set_vwnd_sfc_mode(VDO_MODE_E mode)
{
    if (mode == VMODE_LV)
        vwnd_set_sfc_mode(&lv_mode);
    else
        vwnd_set_sfc_mode(&pb_mode);
}

void _vvm_draw_win_focus(VSM_ID_E win_id)
{
    gint win = 0;
    GdkRectangle area;

    if (vsm_get_vmode() == VMODE_LV)
    {
        for (win = 0; win<VSM_WIN_MAX; win++)
        {
            if (lv_mode.winfo[win].is_focus)
            {
                evt_send_to_local(INFY_VWND_ERASE_FOCUS, win, 0, 0);
                lv_mode.winfo[win].is_focus = FALSE;
            }
        }

        evt_send_to_local(INFY_VWND_CHANGE_BORDER, 0, 0, 0);
        evt_send_to_local(INFY_VWND_DRAW_FOCUS, win_id, 0, 0);
        lv_mode.winfo[win_id].is_focus = TRUE;
    }
    else
    {
        for (win = 0; win<VSM_WIN_MAX; win++)
        {
            if(pb_mode.winfo[win].is_focus)
            {
                pb_mode.winfo[win].is_focus = FALSE;
                evt_send_to_local(INFY_VWND_ERASE_FOCUS, win, 0, 0);
            }
        }

        evt_send_to_local(INFY_VWND_CHANGE_BORDER, 0, 0, 0);
        evt_send_to_local(INFY_VWND_DRAW_FOCUS, win_id, 0, 0);
        pb_mode.winfo[win_id].is_focus = TRUE;
    }
}

void _vvm_refresh_seq_icon(void)
{
    if (vsm_get_vmode() == VMODE_LV)
        _vwnd_update_area_queue_draw(&lv_mode.seqIcon_area);
}

void _vvm_refresh_queue_draw(void)
{
    _vwnd_update_area_queue_draw(0);
}

void _vvm_refresh_process_updates(void)
{
    _vwnd_update_area_process_updates();
    gdk_flush();
}

void _vvm_set_clear_vwnd(void)
{
    lv_mode.fill = FILL_OFF;
    pb_mode.fill = FILL_OFF;
}

void _vvm_unset_clear_vwnd(void)
{
    lv_mode.fill = FILL_ON;
    pb_mode.fill = FILL_ON;
}

void _vvm_time_label_off(void)
{
    lv_mode.time_off = TRUE;
}

void _vvm_time_label_on(void)
{
    lv_mode.time_off = FALSE;
}

void _vvm_set_blind_vwnd(void)
{
    vwnd_set_blind();
}

void _vvm_unset_blind_vwnd(void)
{
    vwnd_unset_blind();
}

////////////////////////////////////////////////////////////
//
// private interfaces (live)
//

void _vvm_live_change_osd(SFC_T *psfc)
{
    guint ch;
    gchar win_id;
    gchar strBuf[VWND_TEXT_LEN+1];
    gint line;
    VCA_CLON vca_clon;
    DVA_CLON dva_clon;

    for (win_id = 0; win_id < VSM_WIN_MAX; win_id++)
    {
        vca_clon.dic_cnt = lv_mode.winfo[win_id].vca_dic_cnt;
        vca_clon.pdics = lv_mode.winfo[win_id].vca_pdics;
        vw_dit_display_free_vca_diclist(vca_clon);

        dva_clon.dic_cnt = lv_mode.winfo[win_id].dvabx_dic_cnt;
        dva_clon.pdics = lv_mode.winfo[win_id].dvabx_pdics;
        vw_dit_display_free_dva_diclist(dva_clon);        
    }

    memset(lv_mode.winfo, 0, sizeof(WIN_INFO_T)*VSM_WIN_MAX);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = psfc->cinfo[ch].win_id;
        ivvm.cinfo[ch].covert = psfc->cinfo[ch].covert;

        if (win_id != -1)
        {
            lv_mode.winfo[win_id].is_focus = FALSE;
            _vwnd_rec_mode_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].rec_mode);
            _vwnd_audio_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].audio);
            _vwnd_mic_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].mic);
            _vwnd_motion_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].motion);
            _vwnd_alarm_text_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].alarm_evt, ivvm.cinfo[ch].alarm_text);

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT))
                    _vwnd_video_status_update(&lv_mode, ch, win_id, VST_NO_VIDEO);
                else
                    _vwnd_video_status_update(&lv_mode, ch, win_id, VST_COVERT);
            }
            else if (ivvm.cinfo[ch].pnd_rate != 0)
            {
                _vwnd_pnd_rate_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_rate);
                _vwnd_connecting_text_update(&lv_mode, ch, win_id, TRUE);
            }
            else if (ivvm.cinfo[ch].vloss)
            {
                if (ivvm.cinfo[ch].pnd_err)
                {
                    switch(ivvm.cinfo[ch].pnd_err)
                    {
                        case PERR_UNKNOWN:
                            _vwnd_video_status_update(&lv_mode, ch, win_id, VST_UNKNOWN_DEV);
                        break;
                        case PERR_UNSUPPORT:
                            _vwnd_video_status_update(&lv_mode, ch, win_id, VST_UNSUPPORT_CAM);
                        break;
                        case PERR_CONNECT_FAIL:
                            _vwnd_video_status_update(&lv_mode, ch, win_id, VST_FAIL_CONNECT);
                        break;
                        case PERR_LOGIN_FAIL:
                            _vwnd_video_status_update(&lv_mode, ch, win_id, VST_FAIL_LOGIN);
                            _vwnd_pnd_err_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_err);
                        break;
                        case PERR_CONFIG_FAIL:
                            _vwnd_video_status_update(&lv_mode, ch, win_id, VST_FAIL_CONFIG);
                        break;
                        default:
                            g_warning("%s, %d, unsupported error number");
                        break;
                    }
                }
                else if (ivvm.cinfo[ch].no_video)
                {
                    _vwnd_video_status_update(&lv_mode, ch, win_id, VST_NO_VIDEO);
                }
                else
                {
                    _vwnd_video_status_update(&lv_mode, ch, win_id, VST_VLOSS);
                }
            }
            else
            {
                _vwnd_video_status_update(&lv_mode, ch, win_id, VST_NONE);
            }

            var_get_camtitle(ivvm.cinfo[ch].title_text, ch);
            _vwnd_cam_title_update(&lv_mode, win_id, ivvm.cinfo[ch].title_text);

            _vwnd_deeplearning_cntr_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].dlva_cntr_person, ivvm.cinfo[ch].dlva_cntr_vehicle, ivvm.cinfo[ch].dlva_cntr_animal);

            _vwnd_vcaline_status_update(&lv_mode, win_id, psfc->div);
            _vwnd_vca_event_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].vca_ebit);

            if (lv_mode.winfo[win_id].vcaline)
            {
                VCA_CLON clon;

                memset(&clon, 0x00, sizeof(VCA_CLON));
                vw_dit_display_get_vca_diclist(&clon);
                lv_mode.winfo[win_id].vca_dic_cnt = clon.dic_cnt;
                lv_mode.winfo[win_id].vca_pdics = clon.pdics;
            }

            _vwnd_dvabxline_status_update(&lv_mode, win_id, psfc->div);
            _vwnd_dvabx_event_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].dvabx_ebit);

            if (lv_mode.winfo[win_id].dvabxline)
            {
                DVA_CLON clon;

                memset(&clon, 0x00, sizeof(DVA_CLON));
                vw_dit_display_get_dva_diclist(&clon);
                lv_mode.winfo[win_id].dvabx_dic_cnt = clon.dic_cnt;
                lv_mode.winfo[win_id].dvabx_pdics = clon.pdics;
            }

            lv_mode.winfo[win_id].corridor_mode = ivvm.cinfo[ch].corridor_mode;
            lv_mode.winfo[win_id].stream_ratio_w = ivvm.cinfo[ch].stream_ratio_w;
            lv_mode.winfo[win_id].stream_ratio_h = ivvm.cinfo[ch].stream_ratio_h;

#ifdef ENABLE_IPCAM_ZIG
            for (line = 0; line < 5; line++)
                _vwnd_debug_text_update(&lv_mode, win_id, line, ivvm.cinfo[ch].debug_text[line]);
#endif
        }
    }

    _vwnd_border_update(&lv_mode);
    lv_mode.dtype = psfc->div;
    lv_mode.plt_w = psfc->w;
    lv_mode.plt_h = psfc->h;

    vwnd_set_item_position();
}

void _vvm_live_osd_on(void)
{
    lv_mode.osd_off = FALSE;
    _vwnd_update_area_queue_draw(0);
}

void _vvm_live_osd_off(void)
{
    lv_mode.osd_off = TRUE;
    _vwnd_update_area_queue_draw(0);
}

void _vvm_live_set_seq_icon(void)
{
    lv_mode.seq_icon = TRUE;
}

void _vvm_live_unset_seq_icon(void)
{
    lv_mode.seq_icon = FALSE;
}

#if defined(ENABLE_PROJECT_KUMMI)||defined(ENABLE_PROJECT_KMW)
extern void mrtpsrc_get_recv_rate(gchar*);
#endif

gint _vvm_live_set_current_time()
{
    OsdData osddata;
    gchar buf[VWND_TEXT_LEN];

    DAL_get_osd_data(&osddata);

#if defined(ENABLE_PROJECT_KUMMI)||defined(ENABLE_PROJECT_KMW)
    gchar buf_rate[32];
#endif

    memset(buf, 0, sizeof(buf));
#if defined(ENABLE_PROJECT_KUMMI)||defined(ENABLE_PROJECT_KMW)
    gst_mrtp_src_get_recv_rate(buf_rate);
    strcpy(buf, buf_rate);
#else
    dtf_get_current_datetime(buf);
#endif
    
    if(!osddata.time)
    {
        g_sprintf(buf,"%s"," ");
        strcpy(lv_mode.time_text, buf);
        _vwnd_update_area_queue_draw(&lv_mode.time_area);
        return 0;
    }
    else
    {
        if(lv_mode.time_off)
        {
            g_sprintf(buf,"%s"," ");
            strcpy(lv_mode.time_text, buf);
            _vwnd_update_area_queue_draw(&lv_mode.time_area);
            return 0;
        }
    }

    if (!strcmp(lv_mode.time_text, buf)) return 0;

    strcpy(lv_mode.time_text, buf);
    _vwnd_update_area_queue_draw(&lv_mode.time_area);

    return 1;
}

gint _vvm_live_set_user(gchar uid[VWND_TEXT_LEN])
{
    OsdData osddata;

    DAL_get_osd_data(&osddata);

    if (osddata.user_name)
    {
        if (!strcmp(lv_mode.user, uid)) return 0;

        strcpy(lv_mode.user, uid);
    }
    else
    {
        if (!strcmp(lv_mode.user, "")) return 0;

        strcpy(lv_mode.user, "");
    }

    _vwnd_update_area_queue_draw(&lv_mode.user_area);
    return 1;
}

void _vvm_live_reset_livestatus(void)
{
    memset(lv_mode.time_text, 0, VWND_TEXT_LEN+1);
    memset(lv_mode.user, 0, VWND_TEXT_LEN+1);
}



////////////////////////////////////////////////////////////
//
// private interfaces (playback)
//

void _vvm_playback_change_osd(SFC_T *psfc)
{
    guint ch;
    gchar win_id;
    VCA_CLON vca_clon;
    DVA_CLON dva_clon;

    for (win_id = 0; win_id < VSM_WIN_MAX; win_id++)
    {
        vca_clon.dic_cnt = pb_mode.winfo[win_id].vca_dic_cnt;
        vca_clon.pdics = pb_mode.winfo[win_id].vca_pdics;
        vw_dit_display_free_vca_diclist(vca_clon);

        dva_clon.dic_cnt = pb_mode.winfo[win_id].dvabx_dic_cnt;
        dva_clon.pdics = pb_mode.winfo[win_id].dvabx_pdics;
        vw_dit_display_free_dva_diclist(dva_clon);
    }

    memset(pb_mode.winfo, 0, sizeof(WIN_INFO_T)*VSM_WIN_MAX);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = psfc->cinfo[ch].win_id;
        ivvm.cinfo[ch].covert = psfc->cinfo[ch].covert;

        if (win_id != -1)
        {
            pb_mode.winfo[win_id].is_focus = FALSE;
            _vwnd_cam_title_update(&pb_mode, win_id, ivvm.cinfo[ch].title_text);

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT))
                    _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NO_VIDEO);
                else
                    _vwnd_video_status_update(&pb_mode, ch, win_id, VST_COVERT);
            }
            else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_NORECORD)
                _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NO_REC);
            else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_ENDVIDEO)
                _vwnd_video_status_update(&pb_mode, ch, win_id, VST_END_VIDEO);
            else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_OVERLAPPED)
                _vwnd_video_status_update(&pb_mode, ch, win_id, VST_OVERLAPPED);
            else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_NONE)
                _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NONE);

//          _vwnd_vcaline_status_update(&pb_mode, win_id, psfc->div);
        }
    }

    _vwnd_border_update(&pb_mode);
    pb_mode.dtype = psfc->div;
    pb_mode.plt_w = psfc->w;
    pb_mode.plt_h = psfc->h;

    vwnd_set_item_position();
}

void _vvm_playback_osd_on(void)
{
    pb_mode.osd_off = FALSE;
    _vwnd_update_area_queue_draw(0);
}

void _vvm_playback_osd_off(void)
{
    pb_mode.osd_off = TRUE;
    _vwnd_update_area_queue_draw(0);
}

gint _vvm_playback_set_video_status(gint ch, NF_PLAY_STATUS_E status)
{
    gchar buf[VWND_TEXT_LEN];
    gchar win_id;

    memset(buf, 0, sizeof(buf));

    if (ivvm.cinfo[ch].pb_status == status)
        return 0;

    evt_send_to_local(INFY_PB_PLAY_STATUS, ch, 0, GINT_TO_POINTER(status));

    ivvm.cinfo[ch].pb_status = status;

    win_id = _vsm_get_win(ch);

    if (win_id == -1)
        return 0;

    if (ivvm.cinfo[ch].covert)
    {
        if (!ssm_check_access_auth(USR_AUTH_COVERT))
            _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NO_VIDEO);
        else
            _vwnd_video_status_update(&pb_mode, ch, win_id, VST_COVERT);
    }
    else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_NORECORD)
        _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NO_REC);
    else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_ENDVIDEO)
        _vwnd_video_status_update(&pb_mode, ch, win_id, VST_END_VIDEO);
    else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_OVERLAPPED)
        _vwnd_video_status_update(&pb_mode, ch, win_id, VST_OVERLAPPED);
    else if (ivvm.cinfo[ch].pb_status == NF_PLAY_STATUS_NONE)
        _vwnd_video_status_update(&pb_mode, ch, win_id, VST_NONE);

    _vwnd_update_area_queue_draw(&pb_mode.winfo[win_id].video_area);

    return 1;
}

gint _vvm_playback_set_dir_rate(DIR_RATE_E status)
{
    if (pb_mode.play_st == status)
        return 0;

    pb_mode.play_st = status;
    _vwnd_update_area_queue_draw(&pb_mode.playST_area);

    return 1;
}

gint _vvm_playback_set_playtime(GTimeVal time)
{
    gchar buf[VWND_TEXT_LEN];

    memset(buf, 0, sizeof(buf));

    dtf_get_local_datetime(time.tv_sec, buf);

    if (!strcmp(pb_mode.time_text, buf))
        return 0;

    strcpy(pb_mode.time_text, buf);
    _vwnd_update_area_queue_draw(&pb_mode.time_area);

    return 1;
}

void _vvm_playback_reset_playstatus(void)
{
    guint ch;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        ivvm.cinfo[ch].pb_status = -1;

    pb_mode.osd_off = FALSE;
    pb_mode.play_st = DR_NONE;
    memset(pb_mode.time_text, 0, VWND_TEXT_LEN+1);
}

gint _vvm_set_alarm_status(gboolean alarm_on)
{
    _vwnd_live_set_alarm_status_update(alarm_on);

    if (vsm_get_vmode() == VMODE_LV)
        _vwnd_update_area_queue_draw(&lv_mode.alarmST_area);

    return 1;
}

gboolean _vvm_vca_draw_invalid_region(gpointer data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;
    VWND_T *pmode;

    pmode = _vwnd_get_curr_mode();

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = _vsm_get_win(ch);

        if (win_id != -1)
        {
            if (pmode->winfo[win_id].vcaline) evt_send_to_local(INFY_VWND_DRAW_NEW_VCA, win_id, 0, 0);
        }
    }

    return TRUE;
}

gboolean _vvm_dvabx_draw_invalid_region(gpointer data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;
    VWND_T *pmode;

    pmode = _vwnd_get_curr_mode();

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = _vsm_get_win(ch);

        if (win_id != -1)
        {
            if (pmode->winfo[win_id].dvabxline) evt_send_to_local(INFY_VWND_DRAW_NEW_DVABX, win_id, 0, 0);
        }
    }

    return TRUE;
}

gboolean _vvm_pos_draw_invalid_region(gpointer data)
{
    VSM_T *pvsm;
    VWND_T *pmode;

    POSX_T *posx;
    POS_CONF_T conf;

    gchar win_id;
    guint ch;
    gint i, all_update = 0;

    if (vsm_get_vmode() == VMODE_NONE) return TRUE;

    pvsm = (VSM_T*)data;
    pmode = _vwnd_get_curr_mode();

    posx_get_pos_conf(&conf);

    if (memcmp(&pmode->pos_cf, &conf, sizeof(POS_CONF_T)))
    {
        memcpy(&pmode->pos_cf, &conf, sizeof(POS_CONF_T));
        vwnd_set_item_position();
        all_update = 1;
    }

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (posx_get_live_display_onoff() != pmode->pos_onoff)
        {
            pmode->pos_onoff = posx_get_live_display_onoff();
            all_update = 1;
        }
    }
    else
    {
        if (posx_get_playback_display_onoff() != pmode->pos_onoff)
        {
            pmode->pos_onoff = posx_get_playback_display_onoff();
            all_update = 1;
        }
    }

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = _vsm_get_win(ch);

        if (win_id == -1) continue;
        if (pmode->winfo[win_id].pos_row == 0) continue;

        if (vsm_get_vmode() == VMODE_LV)
        {
            if (posx_get_live_display_onoff() == 0) continue;

            posx = ivvm.cinfo[ch].lv_posx;
            if (posx_get_pos_table(posx, pmode->winfo[win_id].pos_row) == -1) continue;
        }
        else
        {
            if (posx_get_playback_display_onoff() == 0) continue;

            posx = ivvm.cinfo[ch].pb_posx;
            if (posx_get_pos_table_with_time(posx, pmode->winfo[win_id].pos_row, pvsm->play_time.tv_sec) == -1) continue;
        }

        for (i = 0; i < pmode->winfo[win_id].pos_row; i++)
        {
            if (strcmp(pmode->winfo[win_id].pos_text[i], posx->data[i].text) || (pmode->winfo[win_id].pos_color[i] != posx->data[i].font_color))
            {
                pmode->winfo[win_id].pos_color[i] = posx->data[i].font_color;
                strcpy(pmode->winfo[win_id].pos_text[i], posx->data[i].text);
                if (!all_update) _vwnd_update_area_queue_draw(&pmode->winfo[win_id].pos_area[i]);
            }
        }

        if (posx->data) ifree(posx->data);
        posx->data = 0;
    }

    if (all_update) _vwnd_update_area_queue_draw(0);

    return TRUE;
}

////////////////////////////////////////////////////////////
//
// public notify interfaces
//

void vvm_notify_cam_title_change(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;
    VWND_T *pmode;

    pmode = _vwnd_get_curr_mode();

    for (ch = 0; ch <GUI_CHANNEL_CNT; ch++ )
    {
        var_get_camtitle(ivvm.cinfo[ch].title_text, ch);
        win_id = _vsm_get_win(ch);

        if (win_id != -1)
        {
            if (_vwnd_cam_title_update(pmode, _vsm_get_win(ch), ivvm.cinfo[ch].title_text))
                _vwnd_update_area_queue_draw(&pmode->winfo[win_id].title_area);
        }
    }
}

void vvm_notify_covert(NF_NOTIFY_INFO *data)
{
    gchar *usr_name = NULL;

    if (vsm_get_vmode() == VMODE_LV)
    {
        usr_name = ssm_get_cur_id(NULL);

        if (strlen(usr_name))
            vsm_set_covert_by_user(usr_name);
        else
            vsm_set_covert_by_logout();
    }
}

void vvm_notify_osd(NF_NOTIFY_INFO *data)
{
    gint win;
    guint ch;
    gchar win_id;
    VWND_T *pmode;

    pmode = _vwnd_get_curr_mode();

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = _vsm_get_win(ch);
        if (win_id != -1)
        {
            var_get_camtitle(ivvm.cinfo[ch].title_text, ch);
            if (_vwnd_cam_title_update(pmode, _vsm_get_win(ch), ivvm.cinfo[ch].title_text))
                _vwnd_update_area_queue_draw(&pmode->winfo[win_id].title_area);

            if (vsm_get_vmode() == VMODE_LV)
            {
                if (_vwnd_rec_mode_update(pmode, ch, _vsm_get_win(ch), ivvm.cinfo[ch].rec_mode))
                    _vwnd_update_area_queue_draw(&pmode->winfo[win_id].recIcon_area);

                if (_vwnd_audio_update(pmode, ch, _vsm_get_win(ch), ivvm.cinfo[ch].audio))
                    _vwnd_update_area_queue_draw(&pmode->winfo[win_id].audioIcon_area);

                if (_vwnd_mic_update(pmode, ch, _vsm_get_win(ch), ivvm.cinfo[ch].mic))
                    _vwnd_update_area_queue_draw(&pmode->winfo[win_id].micIcon_area);
            }
        }
    }

    if (_vwnd_border_update(pmode))
    {
        evt_send_to_local(INFY_VWND_CHANGE_BORDER, 0, 0, 0);

        for (win = 0; win < VSM_WIN_MAX; win++)
        {
            if (pmode->winfo[win].is_focus)
                evt_send_to_local(INFY_VWND_DRAW_FOCUS, win, 0, 0);
        }
#if 0
        if (win != VSM_WIN_MAX)
            evt_send_to_local(INFY_VWND_DRAW_FOCUS, win, 0, 0);
#endif
    }
}

void vvm_notify_alarm_text_change(NF_NOTIFY_INFO *data)
{
    gint ch;
    gchar win_id;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (data->d.params[0] & (1 << ch))
            DAL_get_almSen_name(ch, ivvm.cinfo[ch].alarm_text);

        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            if (_vwnd_alarm_text_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].alarm_evt, ivvm.cinfo[ch].alarm_text))
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].alarm_area);
        }
    }
}

void vvm_notify_analog_rec(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;

    if (data) {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            ivvm.cinfo[ch].rec_mode = data->c.chmap[ch];
            // g_message("[%s, %d] data->c.chmap[%d] : %c", __FUNCTION__, __LINE__, ch, data->c.chmap[ch]);

            switch(ivvm.cinfo[ch].rec_mode) {
                case 'p':
                    _vsm_stop_panic_duration();
                break;
                case 'T':
                    _vsm_stop_panic_duration();
                break;
                case 'A':
                    _vsm_stop_panic_duration();
                break;
                case 'M':
                    _vsm_stop_panic_duration();
                break;
                case 'P':
                    _vsm_start_panic_duration();
                break;
                case ' ':
                break;
            }

            win_id = _vsm_get_win(ch);

            if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
            {
                if (_vwnd_rec_mode_update(&lv_mode, ch, _vsm_get_win(ch), ivvm.cinfo[ch].rec_mode))
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].recIcon_area);
            }
        }
    }
}

void vvm_notify_audio_status(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;

    if (data) {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (data->d.params[0] & (1 << ch))
                ivvm.cinfo[ch].audio = TRUE;
            else
                ivvm.cinfo[ch].audio = FALSE;

            win_id = _vsm_get_win(ch);

            if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
            {
                if (_vwnd_audio_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].audio))
                {
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].audioIcon_area);
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].micIcon_area);
                }
            }
        }
    }
}

void vvm_notify_mic_status(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;

    if (data) {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (data->d.params[0] & (1 << ch))
                ivvm.cinfo[ch].mic = TRUE;
            else
                ivvm.cinfo[ch].mic = FALSE;

            win_id = _vsm_get_win(ch);

            if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
            {
                if (_vwnd_mic_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].mic))
                {
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].audioIcon_area);
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].micIcon_area);
                }
            }
        }
    }
}

void vvm_notify_video_loss(NF_NOTIFY_INFO *data)
{
    guint ch;
    gchar win_id;
    VIDEO_ST vst = VST_NONE;
    VSM_DIV_E dtype;

    _set_vca_ditid();
    _set_dva_ditid();

    if (data) {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (data->d.params[0] & (1 << ch))
            {
                if (ivvm.cinfo[ch].pnd_rate != 0)
                {
                    ivvm.cinfo[ch].pnd_rate++;
                }
                else if (ivvm.cinfo[ch].pnd_err)
                {
                    switch(ivvm.cinfo[ch].pnd_err) {
                        case PERR_UNKNOWN:
                            vst = VST_UNKNOWN_DEV;
                        break;
                        case PERR_UNSUPPORT:
                            vst = VST_UNSUPPORT_CAM;
                        break;
                        case PERR_CONNECT_FAIL:
                            vst = VST_FAIL_CONNECT;
                        break;
                        case PERR_LOGIN_FAIL:
                            vst = VST_FAIL_LOGIN;
                        break;
                        case PERR_CONFIG_FAIL:
                            vst = VST_FAIL_CONFIG;
                        break;
                    }
                }
                else if (ivvm.cinfo[ch].no_video)
                    vst = VST_NO_VIDEO;
                else
                    vst = VST_VLOSS;

                ivvm.cinfo[ch].vloss = TRUE;
                ivvm.cinfo[ch].corridor_mode = 0;
            }
            else
            {
                vst = VST_NONE;
                ivvm.cinfo[ch].vloss = FALSE;
                ivvm.cinfo[ch].no_video = FALSE;
                ivvm.cinfo[ch].pnd_err = PERR_NONE;

                ivvm.cinfo[ch].corridor_mode = scm_get_ipcam_corridor_mode(ch);
                scm_ipcam_get_main_stream_ratio(ch, &ivvm.cinfo[ch].stream_ratio_w, &ivvm.cinfo[ch].stream_ratio_h);
            }

            if (ivvm.cinfo[ch].covert)
            {
                if (!ssm_check_access_auth(USR_AUTH_COVERT)) vst = VST_NO_VIDEO;
                else vst = VST_COVERT;
            }

            win_id = _vsm_get_win(ch);

            if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
            {
                if (_vwnd_pnd_rate_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_rate))
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].pndPrg_area);

                if(_vwnd_video_status_update(&lv_mode, ch, win_id, vst))
                {
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].video_area);
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vlogo_area);
                }

                if (_vwnd_pnd_err_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_err))
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].loginFail_area);

                if (_vwnd_audio_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].audio) ||
                    _vwnd_mic_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].mic))
                {
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].audioIcon_area);
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].micIcon_area);
                }

                var_get_camtitle(ivvm.cinfo[ch].title_text, ch);
                if (_vwnd_cam_title_update(&lv_mode, _vsm_get_win(ch), ivvm.cinfo[ch].title_text))
                    _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].title_area);

/*
    //The ratio of the video is updated by the split mode. Therefore, do not update the ratio of the ui.
                if (lv_mode.dtype == VSM_DIV1) {
                    lv_mode.winfo[win_id].corridor_mode = ivvm.cinfo[ch].corridor_mode;
                    lv_mode.winfo[win_id].stream_ratio_w = ivvm.cinfo[ch].stream_ratio_w;
                    lv_mode.winfo[win_id].stream_ratio_h = ivvm.cinfo[ch].stream_ratio_h;
                    _vwnd_update_area_queue_draw(0);
                }
*/
            }
        }
    }
}

void vvm_notify_no_video(NF_NOTIFY_INFO *data)
{
    guint ch;

    if (data) {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (data->d.params[0] & (1 << ch))
                ivvm.cinfo[ch].no_video = TRUE;
            else
                ivvm.cinfo[ch].no_video = FALSE;
        }
    }
}

void vvm_notify_pnd(NF_NOTIFY_INFO *data)
{
    guint ch;
    gchar win_id;
    VIDEO_ST vst = VST_NONE;
    VSM_DIV_E dtype;

    if (data) {
        ch = data->d.params[1];

        if (data->d.params[0] == PND_TYPE_PLUGGED)
        {
            ivvm.cinfo[ch].pnd_err = PERR_NONE;
            vst = VST_NONE;
        }
        else if (data->d.params[0] == PND_TYPE_UNPLUGGED)
        {
            ivvm.cinfo[ch].pnd_err = PERR_NONE;

            if (ivvm.cinfo[ch].no_video)
                vst = VST_NO_VIDEO;
            else
            vst = VST_VLOSS;
        }
        else if (data->d.params[0] == PND_TYPE_UNKNOWN)
        {
            ivvm.cinfo[ch].pnd_err = PERR_UNKNOWN;
            vst = VST_UNKNOWN_DEV;
        }
        else if (data->d.params[0] == PND_TYPE_UNSUPPORTED)
        {
            ivvm.cinfo[ch].pnd_err = PERR_UNSUPPORT;
            vst = VST_UNSUPPORT_CAM;
        }
        else if (data->d.params[0] == PND_TYPE_CONNECTION_FAIL)
        {
            ivvm.cinfo[ch].pnd_err = PERR_CONNECT_FAIL;
            vst = VST_FAIL_CONNECT;
        }
        else if (data->d.params[0] == PND_TYPE_LOGIN_FAIL)
        {
            g_message(">>>>>>>>>>>> %s, %d, ch:%d, PND_TYPE_LOGIN_FAIL", __FUNCTION__, __LINE__, ch);
            ivvm.cinfo[ch].pnd_err = PERR_LOGIN_FAIL;
            vst = VST_FAIL_LOGIN;
        }
        else if (data->d.params[0] == PND_TYPE_CONFIG_FAIL)
        {
            ivvm.cinfo[ch].pnd_err = PERR_CONFIG_FAIL;
            vst = VST_FAIL_CONFIG;
        }
        else
        {
            DMSG(1, "%s, %d, status:%d", __FUNCTION__, __LINE__, data->d.params[0]);
            return;
        }

        if (ivvm.cinfo[ch].covert)
        {
            if (!ssm_check_access_auth(USR_AUTH_COVERT)) vst = VST_NO_VIDEO;
            else vst = VST_COVERT;
        }

        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            if (_vwnd_pnd_rate_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_rate))
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].pndPrg_area);

            if (_vwnd_video_status_update(&lv_mode, ch, win_id, vst))
            {
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].video_area);
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vlogo_area);
            }

            if (_vwnd_pnd_err_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_err))
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].loginFail_area);
        }
    }
}

void vvm_gear_delete(guint ch)
{
    gchar win_id;

    ivvm.cinfo[ch].pnd_rate = 0;

    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        if (_vwnd_pnd_rate_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_rate))
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].pndPrg_area);

        if (_vwnd_connecting_text_update(&lv_mode, ch, win_id, FALSE)){
			vwnd_set_item_position_connecting(ch);
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].connecting_area);
        }
    }

}


void vvm_gear_draw(guint ch)
{
    gchar win_id;
    gboolean  disp_connecting;
    VIDEO_ST vst = VST_NONE;

    ivvm.cinfo[ch].pnd_rate++;

    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        if (ivvm.cinfo[ch].covert)
        {
            if (!ssm_check_access_auth(USR_AUTH_COVERT)) vst = VST_NO_VIDEO;
            else vst = VST_COVERT;
        }

        if(_vwnd_video_status_update(&lv_mode, ch, win_id, vst))
        {
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].video_area);
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vlogo_area);
        }

        if (_vwnd_connecting_text_update(&lv_mode, ch, win_id, TRUE))
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].connecting_area);

        if (_vwnd_pnd_rate_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].pnd_rate))
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].pndPrg_area);

        if (_vwnd_pnd_err_update(&lv_mode, ch, win_id, PERR_NONE))
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].loginFail_area);
    }
}

void vvm_detected_clondev(gint val)
{
    lv_mode.clondev = val;
    pb_mode.clondev = val;
    
    if (vsm_get_vmode() == VMODE_LV)
    {
        _vwnd_update_area_queue_draw(&lv_mode.clondev_area);            
    }
    
    if (vsm_get_vmode() == VMODE_PB)
    {
        _vwnd_update_area_queue_draw(&pb_mode.clondev_area);            
    }    
}

void vvm_notify_alarm(NF_NOTIFY_INFO *data)
{
    gint i;
    gint ch = -1;

    if (vsm_get_vmode() == VMODE_LV)
    {
#ifndef GUI_32CH_SUPPORT
        if (data->d.params[0]) {
            // camera alarm sensor
            for(i=0; i<CAM_ALARM_IN; i++)  {
                if (data->d.params[0] & (1 << i))
                {
                    if ((prev_alarm_status & (1 << i)) == 0)
                        ch = i;
                }
            }

            if(ch != -1)
                _vsm_open_sensor_osd_popup(ch);


            // dvr alarm sensor
            for(i=0; i<var_get_dvr_alarmIn_cnt(); i++)  {
                if (data->d.params[0] & (1 << (16 + i)))
                {
                    if ((prev_alarm_status & (1 << (16 + i))) == 0)
                        ch = (GUI_CHANNEL_CNT + i);
                }
            }

            if(ch != -1)
                _vsm_open_sensor_osd_popup(ch);
        }

        prev_alarm_status = data->d.params[0];
#else
        if (data->d.params[0]) 
        {
            // dvr alarm sensor
            for(i = 0; i < DVR_ALARM_IN; i++) {
                if (data->d.params[0] & (1 << i))
                {
                    if ((prev_dvr_al_status & (1 << i)) == 0)
                        ch = i + 32;
                }
            }

            if(ch != -1)
                _vsm_open_sensor_osd_popup(ch);
                
            prev_dvr_al_status = data->d.params[0];
        }

        if (data->d.params[1]) 
        {
            for(i = 0; i < CAM_ALARM_IN; i++) {
                if (data->d.params[1] & (1 << i))
                {
                    if ((prev_cam_al_status & (1 << i)) == 0)
                        ch = i;
                }
            }

            if(ch != -1)
                _vsm_open_sensor_osd_popup(ch);
                
            prev_cam_al_status = data->d.params[1];
        }
#endif
    }
}

void vvm_notify_motion(NF_NOTIFY_INFO *data)
{
    gchar win_id;
    gint ch;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (data->d.params[0] & (1 << ch))
            ivvm.cinfo[ch].motion = TRUE;
        else
            ivvm.cinfo[ch].motion = FALSE;

        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            if (_vwnd_motion_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].motion)) {
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].motionIcon_area);
            }
        }
    }
}

void vvm_notify_motion_mark(NF_NOTIFY_INFO *data)
{
    guint ch;
    gchar win_id;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            if (_vwnd_motion_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].motion))
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].motionIcon_area);
        }
    }
}

void vvm_notify_net_client(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if(data) {
        if(ipconflict_status) return ;

		ret_val = _vwnd_live_set_net_count(scm_get_net_session_count());

        if ((vsm_get_vmode() == VMODE_LV) && ret_val)
            _vwnd_update_area_queue_draw(&lv_mode.netST_area);
    }
}

void vvm_ipconflict_on_status()
{
    ipconflict_status = 1;
    lv_mode.net_st = NST_CONFLICT;
    _vwnd_update_area_queue_draw(&lv_mode.netST_area);

}

void vvm_ipconflict_off_status()
{
    ipconflict_status = 0;
    _vwnd_live_set_net_count(scm_get_net_session_count());
    _vwnd_update_area_queue_draw(&lv_mode.netST_area);
}

void vvm_notify_net_rxtx(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if(data) {
        if(data->d.params[3] == 0)
            ret_val = _vwnd_live_set_net_status(FALSE);
        else
            ret_val = _vwnd_live_set_net_status(TRUE);

        if(ipconflict_status) return ;

        if ((vsm_get_vmode() == VMODE_LV) && ret_val)
            _vwnd_update_area_queue_draw(&lv_mode.netST_area);
    }
}

void vvm_notify_disk_usage(NF_NOTIFY_INFO *data)
{
    gint usage;
    gint ret_val = 0;

    if(data) {
        if (data->d.params[1] <= 0) return;
        if (data->d.params[0] < 0) return;
        usage = (((gfloat)data->d.params[0]/(gfloat)data->d.params[1]) * 100);
        if (usage > 100) return;

        ret_val = _vwnd_live_set_disk_usage(usage);

        if ((vsm_get_vmode() == VMODE_LV) && ret_val)
            _vwnd_update_area_queue_draw(&lv_mode.diskST_area);
    }
}

void vvm_notify_disk_full(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if(data) {
        if (data->d.params[0])
            ret_val = _vwnd_live_set_disk_full();
        else
            ret_val = _vwnd_live_unset_disk_full();

        if (vsm_get_vmode() == VMODE_LV)
        {
            if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);

            if (data->d.params[0])
                _vsm_open_disk_osd_popup(DISK_FULL_EVT_DATA, "DISK FULL EVENT");
        }
    }
}

void vvm_notify_disk_overwrite(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if (DAL_get_disk_write_mode())
    {
        ret_val = _vwnd_live_set_disk_overwrite();
    }
    else
    {
        ret_val = _vwnd_live_unset_disk_overwrite();
    }

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);

        if (data->d.params[0])
            _vsm_open_disk_osd_popup(OW_START_EVT_DATA, "DISK OVERWRITE EVENT");
    }
}

void vvm_notify_disk_smart(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if (data->d.params[0])
        ret_val = _vwnd_live_set_disk_smart();
    else
        ret_val = _vwnd_live_unset_disk_smart();

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);

        if (data->d.params[0])
            _vsm_open_disk_osd_popup(SMART_EVT_DATA, "S.M.A.R.T EVENT");
    }
}

void vvm_notify_disk_smart_err(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if (data->d.params[0])
        ret_val = _vwnd_live_set_disk_smart_err();
    else
        ret_val = _vwnd_live_unset_disk_smart_err();

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);

        if (data->d.params[0])
            _vsm_open_disk_osd_popup(SMART_EVT_DATA, "S.M.A.R.T EVENT");
    }
}

void vvm_notify_disk_raid(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if (data->d.params[0] >= RST_NORMAL && data->d.params[0] <= RST_BROKEN)
        ret_val = _vwnd_live_set_disk_raid(data->d.params[0]);

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);
    }
}

void vvm_notify_raid_recover()
{
    nftool_mbox(NULL, "NOTICE", "The RAID volume has successfully recovered. Please check\nthe System Setup > Storage > Disk Configuration menu\nfor more detailed information.", NFTOOL_MB_OK);
}

void vvm_notify_raid_degrade()
{
    nftool_mbox(NULL, "WARNING", "A portion of the RAID volume is damaged. Please check\nthe System Setup > Storage > Disk Configuration menu\nfor more detailed information.", NFTOOL_MB_OK);
}

void vvm_notify_raid_rebuild()
{
    nftool_mbox(NULL, "NOTICE", "The RAID volume is currently recovering. Please check\nthe System Setup > Storage > Disk Configuration menu\nfor more detailed information.", NFTOOL_MB_OK);
}

void vvm_notify_raid_broken()
{
    nftool_mbox(NULL, "ERROR", "The RAID volume is completely damaged.", NFTOOL_MB_OK);
}

void vvm_notify_fan(NF_NOTIFY_INFO *data)
{
    if (vsm_get_vmode() == VMODE_LV)
    {
        if (data->d.params[0] != 0)
        {
            if (!prev_fan_status)
                _vsm_open_system_osd_popup(FAN_FAIL_EVT_DATA, "FAN FAIL EVENT");

            prev_fan_status = 1;
        }
        else
        {
            prev_fan_status = 0;
        }
    }
}

void vvm_notify_set_ip_conflict(NF_NOTIFY_INFO *data)
{
    if(vsm_get_vmode() == VMODE_LV || vsm_get_vmode() == VMODE_NONE)
    {
        if(data->d.params[0] != 0)
        {
            if(!prev_ipconflict_status) {
                if (vsm_get_vmode() == VMODE_LV) _vsm_open_system_osd_popup(IP_CONFLICT_EVT_DATA, "SYSTEM IP CONFLICT EVENT");
                vvm_ipconflict_on_status();
            }

            prev_ipconflict_status = 1;
        }
        else
        {
            prev_ipconflict_status = 0;
            vvm_ipconflict_off_status();
        }
    }
}

void vvm_notify_cam_ip_conflict(NF_NOTIFY_INFO *data)
{
    gchar str_buf[64];

    memset(str_buf, 0x00, sizeof(str_buf));

    if(vsm_get_vmode() == VMODE_LV || vsm_get_vmode() == VMODE_NONE)
    {
        if(data->d.params[0] != 0)
        {
            g_sprintf(str_buf, lookup_string("CAM%d IP CONFLICT EVENT"),data->d.params[1]+1);

            if(!prev_ipconflict_status) {
                if (vsm_get_vmode() == VMODE_LV) _vsm_open_system_osd_popup(IP_CONFLICT_EVT_DATA, str_buf);
                vvm_ipconflict_on_status();
            }

            prev_ipconflict_status = 1;
        }
        else
        {
            prev_ipconflict_status = 0;
            vvm_ipconflict_off_status();
        }
    }
}


void vvm_notify_temperature(NF_NOTIFY_INFO *data)
{
    if (vsm_get_vmode() == VMODE_LV)
    {
        if (data->d.params[0] != 0)
        {
            if (!prev_temperature_status)
                _vsm_open_system_osd_popup(TEMPER_FAIL_EVT_DATA, "TEMPERATURE FAIL EVENT");

            prev_temperature_status = 1;
        }
        else
        {
            prev_temperature_status = 0;
        }
    }
}

void vvm_notify_poe(NF_NOTIFY_INFO *data)
{
    if (vsm_get_vmode() == VMODE_LV)
    {
        if (data->d.params[0] != 0)
        {
            if (!prev_poe_status)
                _vsm_open_system_osd_popup(POE_FAIL_EVT_DATA, "POE FAIL EVENT");

            prev_poe_status = 1;
        }
        else
        {
            prev_poe_status = 0;
        }
    }
}

void vvm_notify_poe_hub(NF_NOTIFY_INFO *data)
{
    if (vsm_get_vmode() == VMODE_LV)
    {
        if (data->d.params[0] != 0)
        {
            if (!prev_poe_hub_status)
                _vsm_open_system_osd_popup(POE_FAIL_EVT_DATA, "POE FAIL EVENT");

            prev_poe_hub_status = 1;
        }
        else
        {
            prev_poe_hub_status = 0;
        }
    }
}

void vvm_notify_disk_exhaust(NF_NOTIFY_INFO *data)
{
    if (vsm_get_vmode() == VMODE_LV)
    {
        if (data->d.params[0])
            _vsm_open_disk_osd_popup(DISK_EXHAUSTED_EVT_DATA, "DISK EXHAUSTED EVENT");
    }
}

void vvm_notify_no_disk(NF_NOTIFY_INFO *data)
{
    gint ret_val = 0;

    if (data->d.params[0])
        ret_val = _vwnd_live_set_no_disk();

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (ret_val) _vwnd_update_area_queue_draw(&lv_mode.diskST_area);

        if (data->d.params[0])
            _vsm_open_disk_osd_popup(NO_DIST_EVT_DATA, "NO DISK EVENT");
    }
}

void vvm_notify_vca_rule_info(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;
    guint ret_val;
    gint i;

    _set_vca_ditid();

    for (ch = 0; ch <GUI_CHANNEL_CNT; ch++)
    {
        _vwnd_vca_info_update(ch);
        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            ret_val = _vwnd_vca_event_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].vca_ebit);

            for (i = 0; i <VWND_VCA_MAX_CNT; i++)
            {
                if (ret_val & (1 << i)) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vca_area[i]);
            }
        }
    }
}

void vvm_notify_vca_event_status(NF_NOTIFY_INFO *data)
{
    guint ch;
    gchar win_id;
    guint ret_val;
    gint i;

    ch = data->d.params[0];
    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        g_message("%s, %d, vca_event:%08X", __FUNCTION__, __LINE__, data->d.params[1]);

        ret_val = _vwnd_vca_event_update(&lv_mode, ch, win_id, data->d.params[1]);

        for (i = 0; i <VWND_VCA_MAX_CNT; i++)
        {
            if (ret_val & (1 << i)) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vca_area[i]);
        }
    }

    ivvm.cinfo[ch].vca_ebit = data->d.params[1];
}

void vvm_notify_vca_cntr_status(NF_NOTIFY_INFO *data)
{
    ivca_meta_cnt_t *pCnt;
    gint *p;
    gint cntr;
    VAAID vaaid;
    int ruleid;

    guint ch;
    gchar win_id;
    gint ret_val;

    p = data->p.ptr;
    pCnt = p + 2;
    ch = p[0];

    if (p[0] >= 32) return;

    vaaid = vaa_get_vaaid(ch);
    ruleid = vaa_get_counted_rule(vaaid, pCnt->id);
    if (ruleid == -1) return;

    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        ret_val = _vwnd_vca_cntr_update(&lv_mode, ch, win_id, ruleid, pCnt->value);
        if (ret_val >= 0) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].vca_area[ret_val]);
    }

    ivvm.cinfo[ch].vca_cntr[ruleid] = pCnt->value;
}

void vvm_set_vca_select_icon(gchar win_id, gint sel_idx)
{
    gint i, cnt = 0;
    gint ch;

    g_message("%s, %d, win_id:%d, sel_idx:%d", __FUNCTION__, __LINE__, win_id, sel_idx);
    ch = _vsm_get_ch(win_id);

    for (i = 0; i < IVCA_MAX_ZONES; i++)
    {
        if (ivvm.cinfo[ch].vca_nbit & (1 << i))
        {
            if (sel_idx == cnt) {
                g_message("%s, %d, ch:%d, zid:%d", __FUNCTION__, __LINE__, ch, i);
                vaa_blink_pattern(ch, i);
            }
            cnt++;
        }
    }
}

void vvm_notify_dvabx_rule_info(NF_NOTIFY_INFO *data)
{
    guint ch;
    VSM_DIV_E dtype;
    gchar win_id;
    guint ret_val;
    gint i;

    _set_dva_ditid();

    for (ch = 0; ch <GUI_CHANNEL_CNT; ch++)
    {
        _vwnd_dvabx_info_update(ch);
        win_id = _vsm_get_win(ch);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            ret_val = _vwnd_dvabx_event_update(&lv_mode, ch, win_id, ivvm.cinfo[ch].dvabx_ebit);

            for (i = 0; i <VWND_DVABX_MAX_CNT; i++)
            {
                if (ret_val & (1 << i)) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dvabx_area[i]);
            }
        }
    }
}

void vvm_notify_dvabx_event_status(NF_NOTIFY_INFO *data)
{
    guint ch;
    gchar win_id;
    guint ret_val;
    gint i;

    ch = data->d.params[0];
    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        ret_val = _vwnd_dvabx_event_update(&lv_mode, ch, win_id, data->d.params[1]);

        for (i = 0; i <VWND_DVABX_MAX_CNT; i++)
        {
            if (ret_val & (1 << i)) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dvabx_area[i]);
        }
    }

    ivvm.cinfo[ch].dvabx_ebit = data->d.params[1];
}

void vvm_notify_dvabx_cntr_status(NF_NOTIFY_INFO *data)
{
    ai_meta_cnt_t *pCnt;
    gint *p;
    gint cntr;
    DVAAID dvaaid;
    int ruleid;

    guint ch;
    gchar win_id;
    gint ret_val;

    p = data->p.ptr;
    pCnt = p + 2;
    ch = p[0];

    if (p[0] >= 32) return;

    dvaaid = dvaa_get_dvaaid(ch);
    ruleid = dvaa_get_counted_rule(dvaaid, pCnt->id);
    if (ruleid == -1) return;

    win_id = _vsm_get_win(ch);

    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        ret_val = _vwnd_dvabx_cntr_update(&lv_mode, ch, win_id, ruleid, pCnt->value);
        if (ret_val >= 0) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dvabx_area[ret_val]);
    }

    ivvm.cinfo[ch].dvabx_cntr[ruleid] = pCnt->value;
}

void vvm_set_dvabx_select_icon(gchar win_id, gint sel_idx)
{
    gint i, cnt = 0;
    gint ch;

    g_message("%s, %d, win_id:%d, sel_idx:%d", __FUNCTION__, __LINE__, win_id, sel_idx);
    ch = _vsm_get_ch(win_id);

    for (i = 0; i < IVCA_MAX_ZONES; i++)
    {
        if (ivvm.cinfo[ch].dvabx_nbit & (1 << i))
        {
            if (sel_idx == cnt) {
                g_message("%s, %d, ch:%d, zid:%d", __FUNCTION__, __LINE__, ch, i);
                dvaa_blink_pattern(ch, i);
            }
            cnt++;
        }
    }
}

void vvm_notify_deeplearning_counter_notify(NF_NOTIFY_INFO *data)
{
    DVA_OBJ_COUNTER *dva_obj_counter;
    guint ch;
    gchar win_id;
    gint ret_val;

    guint person_cnt;
    guint vehicle_cnt;
    guint animal_cnt;

    dva_obj_counter = (DVA_OBJ_COUNTER*)(data->p.ptr);
    ch = dva_obj_counter->ch;
    person_cnt = dva_obj_counter->person;
    vehicle_cnt = dva_obj_counter->vehicle;
    animal_cnt = dva_obj_counter->animal;

    win_id = _vsm_get_win(ch);
    
    if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
    {
        ret_val = _vwnd_deeplearning_cntr_update(&lv_mode, ch, win_id, person_cnt, vehicle_cnt, animal_cnt);
        if (ret_val >= 0) {
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[0]);
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[1]);
            _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[2]);                        
        }
    }

    ivvm.cinfo[ch].dlva_cntr_person = person_cnt;
    ivvm.cinfo[ch].dlva_cntr_vehicle = vehicle_cnt;
    ivvm.cinfo[ch].dlva_cntr_animal = animal_cnt;
}

void vvm_notify_deeplearning_counter_property(NF_NOTIFY_INFO *data)
{
    gint i;
    gchar win_id;
    gint ret_val;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        win_id = _vsm_get_win(i);

        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            ret_val = _vwnd_deeplearning_cntr_update(&lv_mode, i, win_id, ivvm.cinfo[i].dlva_cntr_person, ivvm.cinfo[i].dlva_cntr_vehicle, ivvm.cinfo[i].dlva_cntr_animal);
            if (ret_val >= 0) {
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[0]);
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[1]);
                _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].dlva_cntr_area[2]);                        
            }
        }
    }
}

void vvm_notify_ai_keepalive(NF_NOTIFY_INFO *data)
{
    EA_SysNetData net_data;
    guint aibox_state = data->d.params[2];

    if (vsm_get_vmode() == VMODE_LV)
    {
#if defined(_SUPPORT_AIBOX)
        memset(&net_data, 0x00, sizeof(EA_SysNetData));
        DAL_get_SysNet_data(&net_data, AIBOX_TROUBLE_EVT_DATA);

        if ((net_data.osd) && (aibox_state == NF_AIBOX_CONN_FAILED || aibox_state == NF_AIBOX_STREAM_CONN_FAILED))
        {
            VW_Show_OSD_Popup("AI BOX FAILURE EVENT");
        }
#endif
    }
}

void vvm_notify_corridor_mode_info(NF_NOTIFY_INFO *data)
{
    NF_NOTIFY_INFO buf;
    gchar win_id;
    gint i;

    scm_get_vloss_data(&buf);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        win_id = _vsm_get_win(i);

        if (buf.d.params[0] & (1 << i)) {
            ivvm.cinfo[i].corridor_mode = FALSE;
        }
        else {
            ivvm.cinfo[i].corridor_mode = scm_get_ipcam_corridor_mode(i);
            scm_ipcam_get_main_stream_ratio(i, &ivvm.cinfo[i].stream_ratio_w, &ivvm.cinfo[i].stream_ratio_h);
        }

/*
    //The ratio of the video is updated by the split mode. Therefore, do not update the ratio of the ui.
        if ((win_id != -1) && (vsm_get_vmode() == VMODE_LV))
        {
            if (lv_mode.dtype == VSM_DIV1) {
                lv_mode.winfo[win_id].corridor_mode = ivvm.cinfo[ch].corridor_mode;
                lv_mode.winfo[win_id].stream_ratio_w = ivvm.cinfo[ch].stream_ratio_w;
                lv_mode.winfo[win_id].stream_ratio_h = ivvm.cinfo[ch].stream_ratio_h;
                _vwnd_update_area_queue_draw(0);
            }
        }
*/
    }
}

void vvm_set_ipcam_zig_info(CMM_MESSAGE_T *pmsg)
{
    gint ch = pmsg->param;
    gchar win_id;
    NFIPCamMFInfo *info = (NFIPCamMFInfo*)pmsg->data;
    gint line, update = 0;
    gchar strTemp[64];

    win_id = _vsm_get_win(ch);

    if (win_id != -1)
    {
        for (line = 0; line < VWND_DEBUG_LINE; line++)
        {
            memset(ivvm.cinfo[ch].debug_text[line], 0, 128);

            if (line == 0)
            {
                strcpy(ivvm.cinfo[ch].debug_text[line], "IP ADDRESS : ");

                memset(strTemp, 0, 64);

                if (info->ipaddr)
                {
                    sprintf(strTemp, "%d.%d.%d.%d",
                            ((info->ipaddr & 0xff000000)>>24), ((info->ipaddr & 0xff0000)>>16),
                            ((info->ipaddr & 0xff00)>>8), (info->ipaddr & 0xff));
                }

                strcat(ivvm.cinfo[ch].debug_text[line], strTemp);
            }
            else if (line == 1)
            {
                strcpy(ivvm.cinfo[ch].debug_text[line], "MAC ADDRESS : ");

                memset(strTemp, 0, 64);

                if (info->macaddr[0] || info->macaddr[1] || info->macaddr[2] ||
                    info->macaddr[3] || info->macaddr[4] || info->macaddr[5])
                {
                    sprintf(strTemp,"%02x%02x%02x%02x%02x%02x",
                        (guchar)info->macaddr[0], (guchar)info->macaddr[1], (guchar)info->macaddr[2],
                        (guchar)info->macaddr[3], (guchar)info->macaddr[4], (guchar)info->macaddr[5]);
                }

                strcat(ivvm.cinfo[ch].debug_text[line], strTemp);
            }
            else if (line == 2)
            {
                strcpy(ivvm.cinfo[ch].debug_text[line], "NAME : ");
                strcat(ivvm.cinfo[ch].debug_text[line], info->name);
            }
            else if (line == 3)
            {
                strcpy(ivvm.cinfo[ch].debug_text[line], "SW VERSION : ");
                strcat(ivvm.cinfo[ch].debug_text[line], info->swver);
            }
            else if (line == 4)
            {
                strcpy(ivvm.cinfo[ch].debug_text[line], "TIME INFO : ");
                strcat(ivvm.cinfo[ch].debug_text[line], info->timeinfo);
            }
            else if (line == 5)
            {
                memset(strTemp, 0, 64);
                sprintf(strTemp, "%d", info->current);

                strcpy(ivvm.cinfo[ch].debug_text[line], "CURRENT : ");
                strcat(ivvm.cinfo[ch].debug_text[line], strTemp);
                strcat(ivvm.cinfo[ch].debug_text[line], "mA");
            }

            if (_vwnd_debug_text_update(&lv_mode, _vsm_get_win(ch), line, ivvm.cinfo[ch].debug_text[line]))
            {
                update++;
            }
        }
    }

    if (vsm_get_vmode() == VMODE_LV)
    {
        if (update) _vwnd_update_area_queue_draw(&lv_mode.winfo[win_id].debug_area);
    }
}

////////////////////////////////////////////////////////////
//
// for debug
//

static void _print_vvm_vwnd_data_dump(void)
{
    guint ch;
    guint win;
    const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16};






}
