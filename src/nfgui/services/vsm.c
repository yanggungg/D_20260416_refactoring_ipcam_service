/*
 * vsm.c
 *        - dependency :
 *
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */


#include "nf_afx.h"

#include "../support/color.h"
#include "../support/util.h"

#include "cmm.h"
#include "scm.h"
#include "smt.h"

#include "ix_mem.h"
#include "nfdal.h"

#include "dit.h"
#include "vaa.h"
#include "dvaa.h"
#include "dvaa_itx.h"
#include "vw_dit_dva.h"

#include "vsm.h"
#include "evt.h"
#include "vsm_internal.h"

#include "vw_timeline.h"
#include "vw_timeline_deeplearning.h"
#include "vw_live_statusbar.h"
#include "vw_live_shortcut_menu.h"
#include "vw_playback_control_box.h"
#include "vw_playback_shortcut_menu.h"
#include "vw_userpwd.h"
#include "ssm.h"

#include "iux_afx.h"

#include "nf_api_live.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VSM"


//#define DBG_VOM_DISABLE



////////////////////////////////////////////////////////////
//
// private variable
//

static VSM_T ivsm;

static gboolean zoom_mode_on = FALSE;

////////////////////////////////////////////////////////////
//
// private interfaces
//
static gint _ai_analysis_meta_on(gint ch);

static OSD_MODE_E _get_osd_mode()
{
	return ivsm.ssm.omode;
}

static void _set_osd_mode(OSD_MODE_E mode)
{
	ivsm.ssm.omode = mode;
}

static VDO_MODE_E _get_video_mode()
{
	return ivsm.ssm.vmode;
}

static void _set_video_mode(VDO_MODE_E mode)
{
	ivsm.ssm.vmode = mode;
}

static void _live_change(SFC_T *psfc)
{
	int i;
#ifndef	DBG_VOM_DISABLE
    if(!zoom_mode_on)
    	_vom_live_change_video(psfc);
#endif
	_vvm_live_change_osd(psfc);

	if(psfc->div == VSM_DIV1){
		for(i = 0; i < NUM_ACTIVE_CH; i++)
	   	{
			if(psfc->cinfo[i].win_id != -1)
				_ai_analysis_meta_on(i);
	    	}
	}
}

static void _playback_start_video(SFC_T *psfc, GTimeVal time, PLAYBACK_TYPE_E type)
{
#ifndef	DBG_VOM_DISABLE
	if (type == PLAYBACK_PANO1)
		_vom_playback_start_video(psfc, time, PB_PANO1);
	else if (type == PLAYBACK_PANO2)
		_vom_playback_start_video(psfc, time, PB_PANO2);
	else
		_vom_playback_start_video(psfc, time, PB_NORMAL);
#endif
}

static void _preserve_playback_start_video(SFC_T *psfc, GTimeVal time, PLAYBACK_TYPE_E type, guint sess_id)
{
#ifndef	DBG_VOM_DISABLE
	if (type == PLAYBACK_PANO1)
		_vom_preserve_playback_start_video(psfc, time, PB_PANO1, sess_id);
	else if (type == PLAYBACK_PANO2)
		_vom_preserve_playback_start_video(psfc, time, PB_PANO2, sess_id);
	else
		_vom_preserve_playback_start_video(psfc, time, PB_NORMAL, sess_id);
#endif
}

static int _ready_play_cmd()
{
	ivsm.prev_play_time.tv_sec = ~0;
	ivsm.prev_play_time.tv_usec = 0;

	ivsm.ret_msg_play = INFY_PLAYBACK_STARTED;
	return 0;
}

static void _playback_change(SFC_T *psfc)
{
	_ready_play_cmd();
#ifndef	DBG_VOM_DISABLE
	_vom_playback_change_video(psfc);
#endif
	_vvm_playback_change_osd(psfc);
}

static gboolean _import_ssm_live()
{
	DMSG(9, "_get_video_mode:%d", _get_video_mode());

	if (_get_video_mode() == VMODE_NONE)
	{
		memcpy(&ivsm.ssm, &ivsm.ssm_lv, sizeof(SSM_T));
		return TRUE;
	}

	return FALSE;
}

static gboolean _export_ssm_live()
{
	DMSG(9, "_get_video_mode:%d", _get_video_mode());

	if (_get_video_mode() == VMODE_LV)
	{
		memcpy(&ivsm.ssm_lv, &ivsm.ssm, sizeof(SSM_T));
		_set_video_mode(VMODE_NONE);

		ivsm.ssm_pb.cstlayout_idx = _vsm_set_cstlayout_idx(VSM_DEFAULT_DIV, 0);
		ivsm.ssm_pb.cstlayout_elm = _vsm_get_cstlayout_element(VSM_DEFAULT_DIV, 0);
		_vsm_get_sfc_cstlayout(&ivsm.ssm_pb.sfc, ivsm.ssm_pb.cstlayout_elm);
		return TRUE;
	}

	return FALSE;
}

static gboolean _import_ssm_playback()
{
	DMSG(9, "_get_video_mode:%d", _get_video_mode());

	if (_get_video_mode() == VMODE_NONE)
	{
		memcpy(&ivsm.ssm, &ivsm.ssm_pb, sizeof(SSM_T));
		return TRUE;
	}

	return FALSE;
}

static gboolean _export_ssm_playback()
{
	DMSG(9, "_get_video_mode:%d", _get_video_mode());

	if (_get_video_mode() == VMODE_PB)
	{
		memcpy(&ivsm.ssm_pb, &ivsm.ssm, sizeof(SSM_T));
		_set_video_mode(VMODE_NONE);

		// playback���� ���̾ƿ��� �����ߴٸ�, live�� �������ֱ� ����..
		if (ivsm.ssm_lv.cstlayout_elm)
			_vsm_get_sfc_cstlayout(&ivsm.ssm_lv.sfc, ivsm.ssm_lv.cstlayout_elm);
		return TRUE;
	}

	return FALSE;
}

static void _set_clear_sfc(gboolean update)
{
	_vvm_set_clear_vwnd();

	if (update)	{
        _vvm_refresh_queue_draw();
        _vvm_refresh_process_updates();
    }
}

static void _unset_clear_sfc(gboolean update)
{
	_vvm_unset_clear_vwnd();

	if (update)	{
        _vvm_refresh_queue_draw();
        _vvm_refresh_process_updates();
    }
}

static void _set_blind_sfc(gboolean update)
{
	_vvm_set_blind_vwnd();

	if (update)	{
        _vvm_refresh_queue_draw();
        _vvm_refresh_process_updates();
    }
}

static void _unset_blind_sfc(gboolean update)
{
	_vvm_unset_blind_vwnd();

	if (update)	{
        _vvm_refresh_queue_draw();
        _vvm_refresh_process_updates();
    }
}

static gboolean _unset_blind_delay(gpointer data)
{
	_vvm_unset_blind_vwnd();
    _vvm_refresh_queue_draw();
    _vvm_refresh_process_updates();

	return FALSE;
}

static void _unset_blind_sfc_delay(guint delay)
{
#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined (_IPX_1648M4) || defined(_IPX_0824P4E) || defined (_IPX_1648P4E) \
 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	g_timeout_add(delay, _unset_blind_delay, NULL);
#else
	g_timeout_add(0, _unset_blind_delay, NULL);
#endif
}

static gchar _get_win_id(gchar ch)
{
	return ivsm.ssm.sfc.cinfo[ch].win_id;
}

static gchar _get_ch(gchar win_id)
{
	gchar ch;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (_get_win_id(ch) == win_id)
			break;
	}

	if (ch == GUI_CHANNEL_CNT) {
	    DMSG(1, "win_id : %d", win_id);
	    iassert(0);
    }

	return ch;
}

static VSM_DIV_E _get_dtype(void)
{
	return ivsm.ssm.sfc.div;
}

static void _set_dtype(SFC_T *psfc, VSM_DIV_E type)
{
	psfc->div = type;
}

static void _set_focus(gchar win_id)
{
	ivsm.focus_win = win_id;
}

static gchar _get_focus()
{
	return ivsm.focus_win;
}

static void _reset_focus()
{
	ivsm.focus_win = 0xffff;
}

static void _get_current_sfc(SFC_T *psfc)
{
	memcpy(psfc, &ivsm.ssm.sfc, sizeof(SFC_T));
}

static void _init_current_sfc(SFC_T *psfc)
{
	evt_send_to_local(INFY_DIV_CHANGE, 0, 0, GINT_TO_POINTER(psfc->div));
	memcpy(&ivsm.ssm.sfc, psfc, sizeof(SFC_T));
}

static gint _set_vca_ditid(SFC_T *psfc)
{
	VAAID vaaid;
	DITID ditid;
	gint ch;

	VCAData vca_data;

	if (!vaa_is_supported()) return -1;

#if 1
	vw_dit_display_set_vca_ditid(0);
#else
	if (psfc->div != VSM_DIV1)
	{
		vw_dit_display_set_vca_ditid(0);
		return -1;
	}

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (psfc->cinfo[ch].win_id == 0) break;
	}

	if (ch == GUI_CHANNEL_CNT) iassert(0);

	if (psfc->cinfo[ch].covert)
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

static gint _unset_vca_ditid()
{
	if (!vaa_is_supported()) return -1;

	vw_dit_display_set_vca_ditid(0);
	return 0;
}

static gint _set_dvabx_ditid(SFC_T *psfc)
{
	DVAAID dvaaid;
	DITID ditid;
	gint ch;

	DvaBxData dvabx_data;

	if (!dvaa_is_supported()) return -1;

#if 1
	vw_dit_display_set_dva_ditid(0);
#else
	if (psfc->div != VSM_DIV1)
	{
		vw_dit_display_set_dva_ditid(0);
		return -1;
	}

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (psfc->cinfo[ch].win_id == 0) break;
	}

	if (ch == GUI_CHANNEL_CNT) iassert(0);

	if (psfc->cinfo[ch].covert)
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

static gint _unset_dvabx_ditid()
{
	if (!dvaa_is_supported()) return -1;

	vw_dit_display_set_dva_ditid(0);
	return 0;
}

static gint _ai_analysis_meta_on(gint ch)
{
	if (!vaa_is_supported() && !dvaa_is_supported()) return -1;

	nf_meta_data_display_live_on(ch, 0);
	return 0;
}

static gint _ai_analysis_meta_off()
{
	if (!vaa_is_supported() && !dvaa_is_supported()) return -1;

	nf_meta_data_display_live_off();
	return 0;
}

static gint _init_gpu_mode()
{
	CamItxFisheyeData fisheye_data;
	DVAPropData dlva_data;
	gint i, gpu_mode = GPU_NONE;

	for (i = 0; i < var_get_ch_count(); i++)
	{
		memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
		DAL_get_camera_itx_fisheye_data(&fisheye_data, i);
		if (fisheye_data.act) {
			gpu_mode = GPU_FISHEYE;
			break;
		}

		if ((ivsc.dfunc.support_dlva_itx == 1) && (scm_license_is_activated_dlva() == 1))
		{
			memset(&dlva_data, 0x00, sizeof(DVAPropData));
			DAL_get_dva_prop_data(&dlva_data, i);
			if (dlva_data.active) {				
				gpu_mode = GPU_DLVA;
				break;
			}
		}
	}

	scm_set_gpu_mode_function(gpu_mode);
	return 0;
}

static gint _init_fisheye_dewarpch()
{
	ItxTmpFisheyeData tmpfisheye_data;
	gint i;

	ivsm.dewarp_ch = -1;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
	    memset(&tmpfisheye_data, 0x00, sizeof(ItxTmpFisheyeData));
        DAL_get_itx_tmp_fisheye_data(&tmpfisheye_data, i);
        if (tmpfisheye_data.dewarp == 1) {
			ivsm.dewarp_ch = i;
			break;
		}
    }	

	g_message("%s, %d, dewarp_ch:%d", __FUNCTION__, __LINE__, ivsm.dewarp_ch);
	return 0;
}

static gint _run_fisheye_dewarping()
{
	CamItxFisheyeData fisheye_data;
	gint i;

	if (ivsm.dewarp_ch == -1) 
	{
		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
			DAL_get_camera_itx_fisheye_data(&fisheye_data, i);
			if (fisheye_data.act) {
				nf_live_fisheye_set_enable(i);
				ivsm.dewarp_ch = i;
				return -1;
			}
		}

		nf_live_fisheye_set_enable(ivsm.dewarp_ch);
		return -1;
	}

	memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
	DAL_get_camera_itx_fisheye_data(&fisheye_data, ivsm.dewarp_ch);
	if (fisheye_data.act) nf_live_fisheye_set_enable(ivsm.dewarp_ch);
	else nf_live_fisheye_set_enable(-1);

	return 0;
}

static gint _run_dlva_object_detector()
{
	gchar sysdb_key[32];
	guint i, chmask = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.act", i);
		if (nf_sysdb_get_bool(sysdb_key)) chmask |= (1 << i);
	}

	scm_dlva_detector_live_start_chmask(chmask);

	return 0;
}

#if 0
void _set_save_win_id(gint from_ch, gint to_ch)
{
    gint win_id[VSM_WIN_MAX];
    gint i;
    gint temp;

    DAL_get_winid_data(win_id);

    temp = win_id[to_ch];
    win_id[to_ch]=win_id[from_ch];
    win_id[from_ch]=temp;

    DAL_set_winid_data(win_id);
}
#endif

gint get_recover_sfc_pointer()
{
    SFC_T psfc;
    gint ch_cnt;

    _get_current_sfc(&psfc);


    switch(psfc.div)
    {
        case VSM_DIV1 :     ch_cnt  = 1;    break;
        case VSM_DIV4 :     ch_cnt  = 4;    break;
        case VSM_DIV6 :     ch_cnt  = 6;    break;
        case VSM_DIV8 :     ch_cnt  = 8;    break;
        case VSM_DIV9 :     ch_cnt  = 9;    break;
        case VSM_DIV16:     ch_cnt  =16;    break;
    }

    g_message("\n jaeyuoung ch_cnt : %d >> %s >> %d >> \n",ch_cnt, __FUNCTION__, __LINE__);

    return ch_cnt;
}

#if 0
static void _get_live_ch_winid(SFC_T *psfc)
{
    gint win_id[VSM_WIN_MAX];
    gint i;

    DAL_get_winid_data(win_id);

    for(i=0; i< GUI_CHANNEL_CNT;i++)
    {
        psfc->cinfo[i].win_id = win_id[i];
    }
}
#endif

static void _change_fisheye_dewarp(SFC_T *psfc)
{
	CamItxFisheyeData fisheye_data;
	gint dewarp_ch;
	gint i;

	NF_FISHEYE_VIDEO_PARAM param;

	if (psfc->div != VSM_DIV1) return;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (psfc->cinfo[i].win_id != -1) dewarp_ch = i;
	}
	if (!nf_live_fisheye_is_support(dewarp_ch)) return;

	memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
	DAL_get_camera_itx_fisheye_data(&fisheye_data, dewarp_ch);
	if (!fisheye_data.act) return;

	nf_live_fisheye_set_enable(dewarp_ch);
	feye_set_dewarping_channel(dewarp_ch);
}

static void _set_multi_info(SFC_T *psfc)
{
	gchar ch;

	if (psfc->div == VSM_DIV1) return;

	for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		ivsm.ssm.mul_info.win_id[ch] = psfc->cinfo[ch].win_id;

	ivsm.ssm.mul_info.div = psfc->div;
	ivsm.ssm.mul_info.cstlayout_idx = ivsm.ssm.cstlayout_idx;
	ivsm.ssm.mul_info.cstlayout_elm = ivsm.ssm.cstlayout_elm;
}

static void _set_current_sfc(SFC_T *psfc)
{
    if (ivsm.ssm.sfc.div != psfc->div)
        evt_send_to_local(INFY_DIV_CHANGE, 0, 0, GINT_TO_POINTER(psfc->div));

    memcpy(&ivsm.ssm.sfc, psfc, sizeof(SFC_T));
    _set_multi_info(psfc);
}

static void _change_sfc(SFC_T *psfc, gboolean update)
{
	if (_get_video_mode() == VMODE_LV)
		_live_change(psfc);
	else if (_get_video_mode() == VMODE_PB)
		_playback_change(psfc);
	else return;

	if (update)	_vvm_refresh_queue_draw();

	_set_current_sfc(psfc);
}

static void _get_multi_info(SFC_T *psfc)
{
	gchar ch;

	_get_current_sfc(psfc);

	for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		psfc->cinfo[ch].win_id = ivsm.ssm.mul_info.win_id[ch];

	psfc->div = ivsm.ssm.mul_info.div;
	ivsm.ssm.cstlayout_idx = ivsm.ssm.mul_info.cstlayout_idx;
	ivsm.ssm.cstlayout_elm = ivsm.ssm.mul_info.cstlayout_elm;
}

static void _reset_win_id(SFC_T *psfc)
{
	gchar ch;

	for (ch = 0; ch < VSM_WIN_MAX; ch++)
	{
		psfc->cinfo[ch].win_id = 0xffff;
    }
}

static void _reset_multi_info()
{
	SFC_T isfc;
	gchar ch;

	_reset_win_id(&isfc);
		
	if (ivsm.ssm.mul_info.cstlayout_elm) {
		_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.mul_info.cstlayout_elm);
	}
	else {
		ivsm.ssm.mul_info.cstlayout_elm = _vsm_get_cstlayout_element(VSM_DEFAULT_DIV, 0);
		_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.mul_info.cstlayout_elm);
	}

	for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		ivsm.ssm.mul_info.win_id[ch] = isfc.cinfo[ch].win_id;
	}
	ivsm.ssm.mul_info.div = ivsm.ssm.mul_info.cstlayout_elm->div;
}

static gboolean _get_recover_sfc(SFC_T *psfc)
{
	gchar ch;
	gboolean ret = FALSE;

	if (ivsm.is_rc)
	{
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
			psfc->cinfo[ch].win_id = ivsm.rc_info.win_id[ch];

		psfc->div = ivsm.rc_info.div;
		ivsm.ssm.cstlayout_idx = ivsm.rc_info.cstlayout_idx;
		ivsm.ssm.cstlayout_elm = ivsm.rc_info.cstlayout_elm;
		ivsm.is_rc = FALSE;
		ret = TRUE;
	}

	return ret;
}

static gboolean _set_recover_sfc()
{
	gchar ch;
	gboolean ret = FALSE;

	if (!ivsm.is_rc)
	{
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
			ivsm.rc_info.win_id[ch] = _get_win_id(ch);

		ivsm.rc_info.div = ivsm.ssm.sfc.div;
		ivsm.rc_info.cstlayout_idx = ivsm.ssm.cstlayout_idx;
		ivsm.rc_info.cstlayout_elm = ivsm.ssm.cstlayout_elm;
		ivsm.is_rc = TRUE;
		ret = TRUE;
	}

	return ret;
}

static void _reset_recover_sfc()
{
	ivsm.is_rc = FALSE;
}

static gchar _set_win_id_from_mask(SFC_T *psfc, guint mask)
{
	gchar ch;
	gchar ch_cnt = 0;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (mask & (1 << ch))
		{
			psfc->cinfo[ch].win_id = ch_cnt;
			ch_cnt++;
		}
	}

	return ch_cnt;
}

static void _set_div_from_cnt(SFC_T *psfc, gint cnt)
{
	if ((cnt > 16) && (cnt <= 36))
		psfc->div = VSM_DIV36;	
	else if ((cnt > 9) && (cnt <= 16))
		psfc->div = VSM_DIV16;
	else if ((cnt > 4) && (cnt <= 9))
		psfc->div = VSM_DIV9;
	else if ((cnt > 1) && (cnt <= 4))
		psfc->div = VSM_DIV4;
	else
		psfc->div = VSM_DIV1;
}

static void _set_position(SFC_T *psfc, guint x, guint y, guint w, guint h)
{
	if (x % 2 == 1)		x--;
	if (y % 2 == 1)		y--;
	if (w % 2 == 1)		w++;
	if (h % 2 == 1)		h++;

	psfc->x = x;
	psfc->y = y;
	psfc->w = w;
	psfc->h = h;
}

static void _set_full_mode_1div(SFC_T *psfc, gchar ch)
{
	_get_current_sfc(psfc);
	_reset_win_id(psfc);
	_set_position(psfc, 0, 0, MODE_FULL_ACTIVE_W, MODE_FULL_ACTIVE_H);

	psfc->cinfo[ch].win_id = VSM_WIN_ID1;
	psfc->div = VSM_DIV1;
}

static void _reset_omode(SFC_T *psfc)
{
	if (_get_osd_mode() == OMODE_FULL)
		_set_position(psfc, 0, 0, MODE_FULL_ACTIVE_W, MODE_FULL_ACTIVE_H);
	else
		_set_position(psfc, 0, 0, MODE_NORMAL_ACTIVE_W, MODE_NORMAL_ACTIVE_H);
}

static gint _check_div_no_change(VSM_DIV_E dtype)
{
	gint ch;

	if ((ivsm.ssm.sfc.div == DEFAULT_DIV_MODE) && (dtype == DEFAULT_DIV_MODE))
	{
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		{
			if (_get_win_id(ch) != ch)
				break;
		}

		if (ch == GUI_CHANNEL_CNT)
		    return 1;
	}

	return 0;
}

static gchar _get_next_ch(VSM_DIV_E dtype)
{
	gchar focus_id;
	gchar ch, next_ch, tmp;

	if ((ivsm.ssm.sfc.div != VSM_DIV1) && (dtype == VSM_DIV1))
	{
		focus_id = _get_focus();

		if (focus_id != -1)
			return _get_ch(focus_id);
	}

	if (ivsm.ssm.sfc.div != dtype)
		return 0;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
        if(_get_win_id(ch) == VSM_WIN_ID1)
	        break;
	}
#if 0
	if (dtype == VSM_DIV4)
	{
		if (ch < 4) 		tmp = 4;
		else if (ch < 8)	tmp = 8;
		else if (ch < 12)	tmp = 12;
		else				tmp = 0;

		next_ch = tmp%GUI_CHANNEL_CNT;
	}
	else if (dtype == VSM_DIV9)
	{
		if (ch < 9) 	tmp = 8;
		else			tmp = 0;

		next_ch = tmp%GUI_CHANNEL_CNT;
	}
	else
	{
		tmp = ch + 1;
		next_ch = tmp%GUI_CHANNEL_CNT;
	}
#else
	if (_get_video_mode() == VMODE_PB)
	{
		if (dtype == VSM_DIV4)
			next_ch = (ch+4)%GUI_CHANNEL_CNT;
		else
			next_ch = (ch+1)%GUI_CHANNEL_CNT;
	}
	else
		next_ch = (ch+1)%GUI_CHANNEL_CNT;
#endif
	return next_ch;
}

static void _set_change_div(SFC_T *psfc, gchar next_ch, VSM_DIV_E dtype)
{
	gchar ch;

#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

	for (ch=0; ch<nr_channel[dtype]; ch++)
		psfc->cinfo[(ch+next_ch)%GUI_CHANNEL_CNT].win_id = ch;

}

static gchar _get_first_ch_arch_play(guint ch_mask)
{
	gchar ch;

	ivsm.play_ch_mask = ch_mask;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (ch_mask & (1 << ch))
			break;
	}

	if (ch == GUI_CHANNEL_CNT)
	{
		DMSG(9, "channel mask is wrong");
		iassert(0);
	}

	return ch;
}

static gboolean _get_next_ch_arch_play(SFC_T *psfc, gchar *arch_ch)
{
	gchar ch;
	gint pre_pos, post_pos, cnt = 0;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (ivsm.play_ch_mask & (1 << ch))
		{
			cnt++;

			if (psfc->cinfo[ch].win_id != -1)
				pre_pos = cnt;
			}
		}

	if (cnt == 1) return FALSE;

	post_pos = (pre_pos % cnt)+ 1;
	cnt = 0;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (ivsm.play_ch_mask & (1 << ch))
		{
			cnt++;

			if (post_pos == cnt)
			{
				*arch_ch = ch;
				break;
			}
		}
	}

	return TRUE;
}

static guint _get_chmask_arch_play(gchar ch)
{
	guint arch_ch_mask = 0;

	arch_ch_mask |= (1 << ch);
	return arch_ch_mask;
}

static gint _set_user_covert_param(SFC_T *psfc)
{
    gint ch;
	guint covert_mask = 0;

	covert_mask = ssm_get_covert_mask();

	for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
		psfc->cinfo[ch].covert = FALSE;
    }

	for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (covert_mask & (1 << ch))
		{
			psfc->cinfo[ch].covert = TRUE;
		}
	}

	return 0;
}

static gboolean _set_logout_covert_param(SFC_T *psfc)
{
	CameraData cdata;
	gchar ch;
	gboolean changed = FALSE;

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		DAL_get_camera_data(&cdata, ch);

		if (cdata.logoff)
		{
			if(psfc->cinfo[ch].covert == FALSE)
			{
				psfc->cinfo[ch].covert = TRUE;

				changed = TRUE;
			}
		}
		else
		{
			if(psfc->cinfo[ch].covert == TRUE)
			{
				psfc->cinfo[ch].covert = FALSE;

				changed = TRUE;
			}
		}
	}

	return changed;
}

static void _stop_vsm_func()
{
	_vsm_func_sequence_stop();
	_vsm_func_stop_popup();
}

static void _stop_vsm_event_func()
{
	_vsm_func_sequence_stop();
	_vsm_func_stop_event_popup();
}

static void _livestart_helper()
{
	DMSG(9, "");
	vsm_live_start();
	VW_Timeline_ChangeMode(TL_LIVE);

	if (_get_osd_mode() == OMODE_NORMAL)
	{
		VW_Live_StatusBar_Show();
		VW_Timeline_Show();
	}
	else if(_get_osd_mode() == OMODE_FULL)
	{
		if(VW_Live_StatusBar_On_Time() == 0)
			VW_Live_StatusBar_Show();
	}
}

static gboolean _run_live_timer()
{
	if (!ivsm.lvs_tid)
	{
		ivsm.lvs_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 300, _vsm_live_set_livestatus, &ivsm, _vsm_live_reset_livestatus);
		return TRUE;
	}

	return FALSE;
}

static gboolean _stop_live_timer()
{
	if (ivsm.lvs_tid)
	{
		g_source_remove(ivsm.lvs_tid);
		ivsm.lvs_tid = 0;
		return TRUE;
	}

	return FALSE;
}

static gboolean _run_pb_timer()
{
	if (!ivsm.pbs_tid)
	{
		ivsm.pbs_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 80, _vsm_playback_set_playstatus, &ivsm, _vsm_playback_reset_playstatus);
		return TRUE;
	}

	return FALSE;
}

static gboolean _stop_pb_timer()
{
	if (ivsm.pbs_tid)
	{
		g_source_remove(ivsm.pbs_tid);
		ivsm.pbs_tid = 0;
		return TRUE;
	}

	return FALSE;
}

static gboolean _run_pb_preview_timer()
{
	if (!ivsm.preview_tid)
	{
		ivsm.preview_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, _vsm_preview_set_playtime, &ivsm, _vsm_preview_reset_playtime);
		return TRUE;
	}

	return FALSE;
}

static gboolean _stop_pb_preview_timer()
{
	if (ivsm.preview_tid)
	{
		g_source_remove(ivsm.preview_tid);
		ivsm.preview_tid = 0;
		return TRUE;
	}

	return FALSE;
}

static gboolean _run_pb_smart_timer(void)
{
	if ( !ivsm.smart_tid ) {
		ivsm.smart_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 100,
				_vsm_smart_set_playtime, &ivsm, _vsm_smart_reset_playtime);
		return TRUE;
	}
	return FALSE;
}

static gboolean _stop_pb_smart_timer(void)
{
	if ( ivsm.smart_tid ) {
		g_source_remove(ivsm.smart_tid);
		ivsm.smart_tid = 0;
		return TRUE;
	}
	return FALSE;
}

static gint _run_display_timer(void)
{
	if (_get_video_mode() == VMODE_NONE) return -1;

#if 0
	if ( !ivsm.vca_tid ) {
		ivsm.vca_tid = g_timeout_add(120, _vvm_vca_draw_invalid_region, &ivsm);
	}

	if ( !ivsm.dvabx_tid ) {
		ivsm.dvabx_tid = g_timeout_add(120, _vvm_dvabx_draw_invalid_region, &ivsm);
	}
#endif

	if ( !ivsm.pos_tid ) {
		ivsm.pos_tid = g_timeout_add(330, _vvm_pos_draw_invalid_region, &ivsm);
	}

	return 0;
}

static gint _stop_display_timer(void)
{
	if ( ivsm.vca_tid ) {
		g_source_remove(ivsm.vca_tid);
		ivsm.vca_tid = 0;
	}

	if ( ivsm.dvabx_tid ) {
		g_source_remove(ivsm.dvabx_tid);
		ivsm.dvabx_tid = 0;
	}

	if ( ivsm.pos_tid ) {
		g_source_remove(ivsm.pos_tid);
		ivsm.pos_tid = 0;
	}

	return 0;
}

static gint _get_valid_user_layout(LiveData *ld)
{
    gint valid_idx;

    valid_idx = var_get_active_layout();

    if (valid_idx == -1) return -1;

    DAL_get_live_data(ld, (guint)valid_idx);

	return 0;
}

static void _set_user_layout(LiveData *ld, SFC_T *psfc)
{
    guint i;
	gchar ch;
#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

	for(ch = 0; ch < nr_channel[ld->items.type]; ch++)
		psfc->cinfo[ld->items.conf[ch]].win_id = ch;

    psfc->div = ld->items.type;
}

////////////////////////////////////////////////////////////
//
//	VSM init & window create.
//

static void _load_sys_status(void)
{
//	scm_req_novideo_data();
	scm_req_vloss_data();
	scm_req_analog_data();
	scm_req_ipcam_data();
	scm_req_all_pnd_event_data();
	scm_req_net_status_data();
	scm_req_disk_full_data();
	scm_req_disk_usage_data();
	scm_req_disk_ow_event_data();
	scm_req_disk_smart_event_data();
	scm_req_disk_raid_event_data();
	scm_req_smart_event_data();
	scm_req_sensor_event_data();
	scm_req_motion_event_data();
	scm_req_disk_ow_event_data();
	scm_req_nodisk_event_data();
	scm_req_exhaust_event_data();
#ifdef ENABLE_IPCAM_ZIG
    scm_req_ipcam_mf_info();
#endif
}

static void _vsm_init(void)
{
	gchar ch;

	DMSG(9, "");
	for (ch = 0; ch < VSM_WIN_MAX; ch++)
	{
		ivsm.ssm_lv.sfc.cinfo[ch].win_id = 0xffff;
		ivsm.ssm_lv.sfc.cinfo[ch].covert = FALSE;
		ivsm.ssm_lv.mul_info.win_id[ch] = 0xffff;
	}

// live mode init.
	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		ivsm.ssm_lv.sfc.cinfo[ch].win_id = ch;
		ivsm.ssm_lv.sfc.cinfo[ch].covert = FALSE;

	}

	ivsm.ssm_lv.sfc.x = 0;
	ivsm.ssm_lv.sfc.y = 0;
	ivsm.ssm_lv.sfc.w = DISPLAY_ACTIVE_WIDTH;
	ivsm.ssm_lv.sfc.h = DISPLAY_ACTIVE_HEIGHT;
	ivsm.ssm_lv.sfc.div = DEFAULT_DIV_MODE;
    ivsm.ssm_lv.omode = OMODE_FULL;
	ivsm.ssm_lv.vmode = VMODE_LV;

	ivsm.ssm_lv.cstlayout_idx = _vsm_set_cstlayout_idx(DEFAULT_DIV_MODE, 0);
	ivsm.ssm_lv.cstlayout_elm = _vsm_get_cstlayout_element(DEFAULT_DIV_MODE, 0);
	
	_vsm_get_sfc_cstlayout(&ivsm.ssm_lv.sfc, ivsm.ssm_lv.cstlayout_elm);
	
	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		ivsm.ssm_lv.mul_info.win_id[ch] = ivsm.ssm_lv.sfc.cinfo[ch].win_id;

	ivsm.ssm_lv.mul_info.div = DEFAULT_DIV_MODE;
	ivsm.ssm_lv.mul_info.cstlayout_idx = 0;
	ivsm.ssm_lv.mul_info.cstlayout_elm = ivsm.ssm_lv.cstlayout_elm;

	ivsm.is_rc = FALSE;
	ivsm.focus_win = 0xffff;

// playback mode init.
	memcpy(&ivsm.ssm_pb, &ivsm.ssm_lv, sizeof(SSM_T));
    ivsm.ssm_pb.omode = OMODE_NORMAL;
	ivsm.ssm_pb.vmode = VMODE_PB;

    ivsm.lvs_tid = 0;
    ivsm.pbs_tid = 0;
    ivsm.preview_tid = 0;
	ivsm.is_played = FALSE;

	memset(&ivsm.play_time, 0, sizeof(GTimeVal));
	memset(&ivsm.preview_time, 0, sizeof(GTimeVal));
}

static void _vsm_start(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));
    evt_send_to_local(INFY_VWND_RUN_ALL_EVENT, 0, 0, 0);
    evt_send_to_local(INFY_VWND_RUN_RIGHT_PRESS, 0, 0, 0);
	memcpy(&ivsm.ssm, &ivsm.ssm_lv, sizeof(SSM_T));
	_get_current_sfc(&isfc);
	_set_logout_covert_param(&isfc);
	_change_sfc(&isfc, TRUE);
	_init_gpu_mode();		
	_init_fisheye_dewarpch();
	_run_fisheye_dewarping();
	_run_dlva_object_detector();
	_run_live_timer();
	_vsm_func_start();
	_run_display_timer();
}

static int _start_live_video()
{
    guint i;
	gchar ch_arr[32];
    gboolean covert_arr[32];
	NF_DISPLAY_E disp_mode;
	CameraData cdata;
	int ch_cnt = var_get_ch_count();
	guint default_div;

	for (i = 0; i < 32; i++) {
		ch_arr[i] = i;

		if (i < ch_cnt) {
	   		DAL_get_camera_data(&cdata, i);

			if (cdata.logoff) covert_arr[i] = TRUE;
			else covert_arr[i] = FALSE;
		}
		else {
	        covert_arr[i] = FALSE;
		}
    }

	if (ch_cnt == 4)
		default_div = NF_DISPLAY_QUAD;
	else if (ch_cnt == 8)
		default_div = NF_DISPLAY_NONA;
	else if (ch_cnt == 16) 
		default_div = NF_DISPLAY_HEXADECA;
	else 
		default_div = NF_DISPLAY_HEXATRICONTA;	

	nf_live_start(default_div, 0, 0, MODE_FULL_ACTIVE_W, MODE_FULL_ACTIVE_H,
					ch_arr, covert_arr, 0);
}

gint vsm_init(void)
{
#ifndef	DBG_VOM_DISABLE
	_start_live_video();
#endif

	vwnd_init(NF_TOPWND);

	_vsm_init_cstlayout();
	_vom_init();
	_vvm_init();
	_vsm_init();

	return 0;
}

gint vsm_start(void)
{
//	vwnd_show();
	_vsm_start();
	_load_sys_status();
//	smt_set_service(SMT_LOGOUT);
	return 0;
}



////////////////////////////////////////////////////////////
//
// 	public interfaces

//	live start/stop api.

gint vsm_live_start(void)
{
	SFC_T isfc;
	LiveData ld;
	gint gpu_mode;
	int i;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	if (!_import_ssm_live()) return -1;

	DMSG(9, "");
    evt_send_to_local(INFY_VWND_RUN_ALL_EVENT, 0, 0, 0);
	_reset_multi_info();
	_run_fisheye_dewarping();
	_run_dlva_object_detector();

	_get_current_sfc(&isfc);
	_get_recover_sfc(&isfc);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_live_start_video(&isfc, LV_NORMAL);
#endif
	_vvm_set_vwnd_sfc_mode(VMODE_LV);
	_vvm_live_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_vsm_func_run_popup();
	_init_current_sfc(&isfc);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_run_live_timer();
	_run_display_timer();
	_reset_focus();
//	smt_set_service(SMT_LIVE);

	if(isfc.div == VSM_DIV1)
	{
		for(i = 0; i < NUM_ACTIVE_CH; i++)
	   	{
			if(isfc.cinfo[i].win_id != -1)
				_ai_analysis_meta_on(i);
		}
	}

	return 0;
}

gint vsm_live_stop(void)
{
	DMSG(9, "");
	if (!_export_ssm_live()) return -1;
	if (!_stop_live_timer()) return -1;

	ivsm.dewarp_ch = nf_live_fisheye_get_enable();

	DMSG(9, "");
    evt_send_to_local(INFY_VWND_STOP_ALL_EVENT, 0, 0, 0);
	nf_live_fisheye_set_enable(-1);
	_unset_vca_ditid();
	_unset_dvabx_ditid();
	_stop_display_timer();
	_set_blind_sfc(TRUE);
	_stop_vsm_event_func();
#ifndef	DBG_VOM_DISABLE
	_vom_live_stop_video();
#endif
	_ai_analysis_meta_off();
   	return 0;
}

gint vsm_live_preview_start(guint ch_mask, guint x, guint y, guint w, guint h)
{
	SFC_T isfc;
	gchar ch_cnt = 0;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch_mask:%08X, x:%d, y:%d, w:%d, h:%d", ch_mask, x, y, w, h);
	if (!ch_mask) return -1;

	DMSG(9, "");
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	ch_cnt = _set_win_id_from_mask(&isfc, ch_mask);
	_set_div_from_cnt(&isfc, ch_cnt);
	_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_live_start_video(&isfc, LV_PREVIEW);
#endif
	return 0;
}

gint vsm_live_preview_start_vca(gint ch, guint x, guint y, guint w, guint h)
{
	SFC_T isfc;
	gchar ch_cnt = 0;

	VAAID vaaid;
	DITID ditid;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	isfc.cinfo[ch].win_id = 0;
    isfc.div = VSM_DIV1;
	_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_live_start_video(&isfc, LV_PREVIEW);
#endif

	if (isfc.cinfo[ch].covert == 0)
	{
	    vaaid = vaa_get_vaaid(ch);
    	ditid = vaa_get_ditid(vaaid);
    	vw_dit_display_set_vca_ditid(ditid);
	}
	else
	{
		vw_dit_display_set_vca_ditid(0);
	}

	_ai_analysis_meta_on(ch);

	return 0;
}

gint vsm_live_preview_pause_vca(void)
{
	DMSG(9, "");
#ifndef	DBG_VOM_DISABLE
	_vom_live_stop_video();
#endif
	return 0;
}

gint vsm_live_preview_start_dva(gint ch, guint x, guint y, guint w, guint h)
{
	SFC_T isfc;
	gchar ch_cnt = 0;

	DVAAID dvaaid;
	DITID ditid;

	DMSG(9, "");
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	isfc.cinfo[ch].win_id = 0;
    isfc.div = VSM_DIV1;
	_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_live_start_video(&isfc, LV_PREVIEW);
#endif

	if (isfc.cinfo[ch].covert == 0)
	{
	    dvaaid = dvaa_get_dvaaid(ch);
    	ditid = dvaa_get_ditid(dvaaid);
    	vw_dit_display_set_dva_ditid(ditid);
	}
	else
	{
		vw_dit_display_set_dva_ditid(0);
	}

	_ai_analysis_meta_on(ch);

	return 0;
}

gint vsm_live_preview_pause_dva(void)
{
	DMSG(9, "");
#ifndef	DBG_VOM_DISABLE
	_vom_live_stop_video();
#endif
	return 0;
}

gint vsm_live_preview_stop(void)
{
	DMSG(9, "");
	_unset_vca_ditid();
	_unset_dvabx_ditid();
#ifndef	DBG_VOM_DISABLE
	_vom_live_stop_video();
#endif
	_ai_analysis_meta_off();
	return 0;
}

//	playback start/stop, etc..  api.

gint vsm_playback_start(guint ch_mask, GTimeVal start_time, PLAYBACK_TYPE_E type)
{
	SFC_T isfc;
	gchar ch_cnt = 0;
	gint gpu_mode;

	memset(&isfc, 0x00, sizeof(SFC_T));
	
	DMSG(9, "ch_mask:%08X", ch_mask);
	if (!_import_ssm_playback()) return -1;
	if (!ch_mask) return -1;

	DMSG(9, "");
	if (start_time.tv_usec)
	{
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		start_time.tv_usec = 0;
	}

    evt_send_to_local(INFY_VWND_RUN_ALL_EVENT, 0, 0, 0);
	_run_fisheye_dewarping();

	_reset_multi_info();
	_reset_win_id(&isfc);
	if (ch_mask == var_get_ch_mask() || ch_mask == 0xffff) {
		_get_current_sfc(&isfc);
	} else {
		ch_cnt = _set_win_id_from_mask(&isfc, ch_mask);
		_set_div_from_cnt(&isfc, ch_cnt);
	}
	_set_position(&isfc, 0, 0, MODE_NORMAL_ACTIVE_W, MODE_NORMAL_ACTIVE_H);
	_set_user_covert_param(&isfc);
	_playback_start_video(&isfc, start_time, type);
	_vvm_set_vwnd_sfc_mode(VMODE_PB);
	_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_init_current_sfc(&isfc);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_run_pb_timer();
	_run_display_timer();
	_reset_focus();
    ivsm.play_type = type;
//	smt_set_service(SMT_PLAYBACK);
	return 0;
}

gint vsm_preserve_playback_start(guint ch_mask, GTimeVal start_time, PLAYBACK_TYPE_E type, guint sess_id)
{
	SFC_T isfc;
	gchar ch_cnt = 0;

	memset(&isfc, 0x00, sizeof(SFC_T));
	
	DMSG(9, "ch_mask:%08X", ch_mask);
	if (!_import_ssm_playback()) return -1;
	if (!ch_mask) return -1;

	DMSG(9, "");
	if (start_time.tv_usec)
	{
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		start_time.tv_usec = 0;
	}

    evt_send_to_local(INFY_VWND_RUN_ALL_EVENT, 0, 0, 0);
	_reset_multi_info();
	_reset_win_id(&isfc);
	if (ch_mask == var_get_ch_mask() || ch_mask == 0xffff) {
		_get_current_sfc(&isfc);
	} else {
		ch_cnt = _set_win_id_from_mask(&isfc, ch_mask);
		_set_div_from_cnt(&isfc, ch_cnt);
	}
	_set_position(&isfc, 0, 0, MODE_FULL_ACTIVE_W, MODE_FULL_ACTIVE_H);
	_set_user_covert_param(&isfc);
	_preserve_playback_start_video(&isfc, start_time, type, sess_id);
	_vvm_set_vwnd_sfc_mode(VMODE_PB);
	_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_init_current_sfc(&isfc);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_run_pb_timer();
	_run_display_timer();
	_reset_focus();
    ivsm.play_type = type;
//	smt_set_service(SMT_PLAYBACK);
	return 0;
}

gint vsm_playback_stop(void)
{
	DMSG(9, "");
	if (!_export_ssm_playback()) return -1;
	if (!_stop_pb_timer()) return -1;

	ivsm.dewarp_ch = nf_live_fisheye_get_enable();

	DMSG(9, "");
    evt_send_to_local(INFY_VWND_STOP_ALL_EVENT, 0, 0, 0);
	nf_live_fisheye_set_enable(-1);
	_unset_vca_ditid();
	_unset_dvabx_ditid();
	_stop_display_timer();
	_set_blind_sfc(TRUE);
#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_video();
#endif

   	return 0;
}

gint vsm_playback_restart_by_time(GTimeVal time)
{
	SFC_T isfc;
	gchar *usr_name;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	if (!_stop_pb_timer()) return -1;

	DMSG(9, "");
	if (time.tv_usec)
	{
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		time.tv_usec = 0;
	}

#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_video();
#endif
	_get_current_sfc(&isfc);
	_playback_start_video(&isfc, time, ivsm.play_type);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_run_pb_timer();
	_run_display_timer();
	_vvm_unset_clear_vwnd();
	_vsm_func_run_popup();
	_unset_blind_sfc_delay(300);
   	return 0;
}

gint vsm_playback_restart_by_chtime(gint ch, GTimeVal time)
{
	SFC_T isfc;
	gchar *usr_name;

	DMSG(9, "");
	if (!_stop_pb_timer()) return -1;

	DMSG(9, "");
	if (time.tv_usec)
	{
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		time.tv_usec = 0;
	}
	
#ifndef	DBG_VOM_DISABLE	
	_vom_playback_stop_video();
#endif
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	isfc.cinfo[ch].win_id = 0;
    isfc.div = VSM_DIV1;		
	_playback_start_video(&isfc, time, ivsm.play_type);
	_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_init_current_sfc(&isfc);
	_set_vca_ditid(&isfc);	
	_set_dvabx_ditid(&isfc);
	_run_pb_timer();
	_run_display_timer();	
   	return 0;
}

gint vsm_playback_preview_start(guint ch_mask, GTimeVal time, guint x, guint y, guint w, guint h)
{
	SFC_T isfc;
	gchar ch_cnt = 0;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch_mask:%08X, x:%d, y:%d, w:%d, h:%d", ch_mask, x, y, w, h);

	if (time.tv_usec)
	{
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		time.tv_usec = 0;
	}

	_reset_win_id(&isfc);
	ch_cnt = _set_win_id_from_mask(&isfc, ch_mask);
	_set_div_from_cnt(&isfc, ch_cnt);
	_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_playback_start_video(&isfc, time, PB_PREVIEW);
#endif
	_run_pb_preview_timer();

   	return 0;
}

gint vsm_playback_preview_stop(void)
{
	DMSG(9, "");
	if (!_stop_pb_preview_timer()) return -1;

	DMSG(9, "");
	_unset_vca_ditid();
	_unset_dvabx_ditid();
#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_video();
#endif
   	return 0;
}

gint vsm_playback_smart_mainview_start(gint ch, GTimeVal time_start, GTimeVal time_end, NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	SFC_T isfc;
	gchar ch_cnt;

	memset(&isfc, 0x00, sizeof(SFC_T));

	if (time_start.tv_usec) {
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		time_start.tv_usec = 0;
	}

	_reset_win_id(&isfc);
	isfc.cinfo[ch].win_id = 0;
    isfc.div = VSM_DIV1;
	//_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_playback_start_smart_video(&isfc, time_start,time_end, search_mode);
#endif
	if(search_mode == NF_PLAY_SMART_SEARCH_META)
		_run_pb_smart_timer();
	return 0;
}

gint vsm_playback_smart_mainview_pause(NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	_vom_playback_pause_smart_video(search_mode, TRUE);
	return 0;
}

gint vsm_playback_smart_mainview_resume(NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	_vom_playback_pause_smart_video(search_mode, FALSE);
	return 0;
}

gint vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_MODE search_mode)
{
	if (search_mode == NF_PLAY_SMART_SEARCH_META && !_stop_pb_smart_timer() ) return -1;

#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_smart_video(search_mode);
#endif
   	return 0;
}

gint vsm_playback_smart_preview_start(gint ch, GTimeVal time)
{
	SFC_T isfc;
	gchar ch_cnt;
	GTimeVal time_end;

	VAAID vaaid;
	DITID ditid;

	memset(&isfc, 0x00, sizeof(SFC_T));

	//captainnn
	time_end.tv_sec = time.tv_sec + 100000;

	if (time.tv_usec) {
		g_warning("<%s, %d>init tv_usec!!", __FUNCTION__, __LINE__);
		time.tv_usec = 0;
	}

	_reset_win_id(&isfc);
	isfc.cinfo[ch].win_id = 0;
    isfc.div = VSM_DIV1;
	//_set_position(&isfc, x, y, w, h);
	_set_user_covert_param(&isfc);
#ifndef	DBG_VOM_DISABLE
	_vom_playback_start_smart_video(&isfc, time,time_end, NF_PLAY_SMART_SEARCH_PREVIEW);
#endif
	_run_pb_preview_timer();

	return 0;
}

gint vsm_playback_smart_preview_stop(void)
{
	if ( !_stop_pb_preview_timer() ) return -1;

#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_smart_video(NF_PLAY_SMART_SEARCH_PREVIEW);
#endif
	return 0;
}

gint vsm_archived_play_start(guint ch_mask, time_t offset_time, ARCH_PLAY_TYPE_E type)
{
	SFC_T isfc;
	gchar ch, ch_cnt = 0;
	guint arch_ch_mask = 0;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch_mask:%08X", ch_mask);
	if (!_import_ssm_playback()) return -1;

	DMSG(9, "");
    evt_send_to_local(INFY_VWND_STOP_ALL_EVENT, 0, 0, 0);
	_reset_win_id(&isfc);
	ch = _get_first_ch_arch_play(ch_mask);
	nf_avi_player_mul_set_ch(ch);
	arch_ch_mask = _get_chmask_arch_play(ch);
	ch_cnt = _set_win_id_from_mask(&isfc, arch_ch_mask);
	_set_div_from_cnt(&isfc, ch_cnt);
	_set_position(&isfc, 0, 0, MODE_FULL_ACTIVE_W, MODE_FULL_ACTIVE_H);
	_set_user_covert_param(&isfc);

#ifndef	DBG_VOM_DISABLE
	if (type == ARCH_PLAY_MUL)
		_vom_playback_start_archived_video(&isfc, offset_time, PB_ARCH_MUL);
	else
		_vom_playback_start_archived_video(&isfc, offset_time, PB_ARCH_AVI);
#endif

	_vvm_set_vwnd_sfc_mode(VMODE_PB);
	_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_init_current_sfc(&isfc);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_run_pb_timer();
	_run_display_timer();
	_reset_focus();
    ivsm.play_type = type;
//	smt_set_service(SMT_ARCH_PLAY);

    return 0;
}

gint vsm_archived_play_start_ex(guint ch_mask, time_t offset_time, ARCH_PLAY_TYPE_E type)
{
	gint ret;

	DMSG(9, "ch_mask:%08X", ch_mask);
	_ready_play_cmd();
	ret = vsm_archived_play_start(ch_mask, offset_time, type);
    return ret;
}

gint vsm_archived_play_stop(void)
{
	DMSG(9, "");
	if (!_export_ssm_playback()) return -1;
	if (!_stop_pb_timer()) return -1;

	DMSG(9, "");
    evt_send_to_local(INFY_VWND_STOP_ALL_EVENT, 0, 0, 0);
	_unset_vca_ditid();
	_unset_dvabx_ditid();
	_stop_display_timer();
	_set_blind_sfc(TRUE);
#ifndef	DBG_VOM_DISABLE
	_vom_playback_stop_video();
#endif

    return 0;
}

gint vsm_playback_play_pause_by_menu_opened(void)
{
	DMSG(9, "");
#ifndef	DBG_VOM_DISABLE
	ivsm.is_played = _vom_playback_play_pause();
#endif
    return 0;
}

gint vsm_playback_play_recover_by_menu_closed(void)
{
	DMSG(9, "");
	if (!ivsm.is_played) return -1;

	DMSG(9, "");
#ifndef	DBG_VOM_DISABLE
	_vom_playback_play_start_after_pause();
#endif
    return 0;
}

gint vsm_playback_play_time_by_menu_closed(GTimeVal playtime)
{
	// don't check ivsm.is_played, need cmd "playback stop-start".

	DMSG(9, "");
#ifndef	DBG_VOM_DISABLE
	_vom_playback_play_start_after_pause_time(playtime);
#endif
    return 0;
}

DIR_RATE_E vsm_playback_get_dir_rate(void)
{
	return ivsm.play_dir_rate;
}

GTimeVal vsm_playback_get_playtime(void)
{
	return ivsm.play_time;
}

GTimeVal vsm_playback_get_previewtime(void)
{
	return ivsm.preview_time;
}

DIR_RATE_E vsm_playback_get_smart_dir_rate(void)
{
	return ivsm.smart_dir_rate;
}

GTimeVal vsm_playback_get_smarttime(void)
{
	return ivsm.smart_time;
}


////////////////////////////////////////////////////////////
//
// 	public interfaces
//
//	(from VVM, VOM, VWND)

gchar _vsm_get_ch(gchar win_id)
{
	return _get_ch(win_id);
}

gchar _vsm_get_win(gchar ch)
{
	return _get_win_id(ch);
}

void _vsm_set_recover_sfc_by_func()
{
	_set_recover_sfc();
}

void _vsm_reset_recover_sfc_by_func()
{
	_reset_recover_sfc();
}

guint _vsm_get_screen_ch_mask(void)
{
	gchar ch;
	guint ch_mask = 0;
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	if(_get_recover_sfc(&isfc))
	{
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		{
			if(isfc.cinfo[ch].win_id != -1)
				ch_mask |= (1 << ch);
		}
	}
	else
	{
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		{
			if (_get_win_id(ch) != -1)
				ch_mask |= (1 << ch);
		}
	}

	return ch_mask;
}

gboolean _vsm_copy_current_sfc(SFC_T *psfc)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	if(psfc) {
		_get_current_sfc(psfc);
		return TRUE;
	}

	return FALSE;
}

int _vsm_ack_play_cmd()
{
	if (ivsm.prev_play_time.tv_sec != 0) {
		if (ivsm.prev_play_time.tv_sec != ivsm.play_time.tv_sec) {
			evt_send_to_local(ivsm.ret_msg_play, 0, 0, 0);
			ivsm.prev_play_time.tv_sec = 0;
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////
//
// 	public interfaces
//
//	(from VIEWER)

LIVESTART_T *vsm_create_livestart_obj()
{
	LIVESTART_T *lst = imalloc(sizeof(LIVESTART_T));
	lst->start = _livestart_helper;
	return lst;
}

int vsm_destroy_livestart_obj(LIVESTART_T *lst)
{
	if (lst) ifree(lst);

	return 0;
}

VDO_MODE_E vsm_get_vmode(void)
{
	return _get_video_mode();
}

OSD_MODE_E vsm_get_omode(void)
{
	return _get_osd_mode();
}

VSM_DIV_E vsm_get_div(void)
{
	return _get_dtype();
}

gboolean vsm_is_focus_win(void)
{
	if (_get_focus() == -1)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

gchar vsm_get_focused_channel(void)
{
	gchar ch, find_win_id;

	if (_get_focus() == -1)
		find_win_id = VSM_WIN_ID1;
	else
		find_win_id = _get_focus();

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		if (_get_win_id(ch) == find_win_id)
			break;
	}

	if (ch == GUI_CHANNEL_CNT) iassert(0);

	DMSG(9, "focus ch : %d", ch);

	return ch;
}

gint vsm_change_full_mode(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	_set_osd_mode(OMODE_FULL);
	_get_current_sfc(&isfc);
	_set_position(&isfc, 0, 0, (guint)MODE_FULL_ACTIVE_W, (guint)MODE_FULL_ACTIVE_H);
	_set_blind_sfc(TRUE);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(300);
	return 0;
}

gint vsm_change_normal_mode(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	_set_osd_mode(OMODE_NORMAL);
	_get_current_sfc(&isfc);
	_set_position(&isfc, 0, 0, (guint)MODE_NORMAL_ACTIVE_W, (guint)MODE_NORMAL_ACTIVE_H);
	_set_blind_sfc(TRUE);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(300);
	return 0;
}

gint vsm_osd_on(void)
{
	if (_get_video_mode() == VMODE_LV)
		_vvm_live_osd_on();
	else if (_get_video_mode() == VMODE_PB)
		_vvm_playback_osd_on();

	return 0;
}

gint vsm_osd_off(void)
{
	if (_get_video_mode() == VMODE_LV)
		_vvm_live_osd_off();
	else if (_get_video_mode() == VMODE_PB)
		_vvm_playback_osd_off();

	return 0;
}

gint vsm_osd_refresh(void)
{
	_vvm_refresh_queue_draw();
	return 0;
}

gint vsm_draw_focus_win(gchar win_id)
{
#if defined(GUI_8CH_SUPPORT)
	if (win_id == VSM_WIN_ID9) return -1;
#endif
//	_vsm_func_stop_popup();
	_set_focus(win_id);
	_vvm_draw_win_focus(win_id);
//	_vsm_func_run_popup();
	return 0;
}

gchar vsm_get_current_win_id_size(gint *x, gint *y, gint *w, gint *h)
{
    vwnd_get_position_info(_get_dtype(), _get_focus(), x, y);
    vwnd_get_size_info(_get_dtype(), _get_focus(), w, h);

    return vsm_get_focused_channel();
}

gint vsm_show_shortcut_menu(guint pos_x, guint pos_y, gchar win_id)
{
	gchar ch;

#if defined(GUI_8CH_SUPPORT)
	if (win_id == VSM_WIN_ID9) return -1;
#endif

	_set_focus(win_id);
	_vvm_draw_win_focus(win_id);

	ch = vsm_get_focused_channel();

	if (_get_video_mode() == VMODE_LV)
		VW_ShortCut_Menu_Show((gint)pos_x, (gint)pos_y, (guint)ch);
	else if (_get_video_mode() == VMODE_PB)
		vw_playback_shortcut_menu_show((gint)pos_x, (gint)pos_y, (guint)ch);

	return 0;
}

gint vsm_change_sfc_by_ch(gchar ch)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch:%d", ch);
	if (ch >= GUI_CHANNEL_CNT) return -1;

	DMSG(9, "");
	_stop_vsm_func();
	_get_current_sfc(&isfc);
//	if (_get_dtype() != VSM_DIV1)
//		_set_multi_info(&isfc);
	_reset_win_id(&isfc);
	_set_change_div(&isfc, ch, VSM_DIV1);
	_set_dtype(&isfc, VSM_DIV1);
	_set_blind_sfc(TRUE);
    _change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();
	return 0;
}

gint vsm_change_sfc_by_array(gchar win_id[VSM_WIN_MAX], VSM_DIV_E dtype)
{
	SFC_T isfc;
	gchar ch;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "dtype:%d", dtype);
	_stop_display_timer();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
		isfc.cinfo[ch].win_id = win_id[ch];

	_set_dtype(&isfc, dtype);
	_set_blind_sfc(TRUE);
	_change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(500);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_reset_focus();
	_run_display_timer();
	return 0;
}

gint vsm_change_div_by_user(gint valid_idx)
{
	SFC_T isfc;
	gchar ch;
    LiveData ld;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "valid_idx:%d", valid_idx);
    DAL_get_live_data(&ld, (guint)valid_idx);

	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
    _set_user_layout(&ld, &isfc);
	_set_blind_sfc(TRUE);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_reset_focus();
    var_set_active_layout(valid_idx);
	return 0;
}

#if 0 //__NOT_USED
gint vsm_change_sfc_by_div(VSM_DIV_E dtype)
{
	SFC_T isfc;
	gchar next_ch;
	gint ret;
	gint i;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "dtype:%d", dtype);
	if (_check_div_no_change(dtype)) return -1;

	DMSG(9, "");
	_stop_vsm_func();
	_get_current_sfc(&isfc);
//	if ((dtype == VSM_DIV1) && (_get_dtype() != VSM_DIV1))
//		_set_multi_info(&isfc);
    _reset_win_id(&isfc);
    next_ch = _get_next_ch(dtype);
    _set_change_div(&isfc, next_ch, dtype);
	_set_dtype(&isfc, dtype);
	_set_clear_sfc(TRUE);
	_unset_clear_sfc(FALSE);
    _change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_set_vca_ditid(&isfc);
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();
	return 0;
}
#endif

gint vsm_change_sfc_cstlayout_load(VSM_DIV_E dtype)
{
	SFC_T isfc;

	DMSG(9, "dtype:%d", dtype);

	ivsm.ssm_lv.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
	ivsm.ssm_lv.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm_lv.cstlayout_idx);
	ivsm.ssm_pb.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
	ivsm.ssm_pb.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm_pb.cstlayout_idx);

	_vsm_get_sfc_cstlayout(&ivsm.ssm_lv.sfc, ivsm.ssm_lv.cstlayout_elm);
	_vsm_get_sfc_cstlayout(&ivsm.ssm_pb.sfc, ivsm.ssm_pb.cstlayout_elm);

	if (_get_video_mode() == VMODE_NONE) return 0;

	ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
	ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);

	memset(&isfc, 0x00, sizeof(SFC_T));
	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	_set_dtype(&isfc, dtype);
	_set_blind_sfc(TRUE);
	_change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);	
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();

	return 0;
}

gint vsm_change_sfc_cstlayout_by_ch(gint ch)
{
	SFC_T isfc;

	DMSG(9, "channel:%d", ch);

	ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(VSM_DIV1, ch);
	ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(VSM_DIV1, ch);

	memset(&isfc, 0x00, sizeof(SFC_T));
	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	_set_dtype(&isfc, VSM_DIV1);
	_set_blind_sfc(TRUE);
    _change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);	
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();

	return 0;
}

gint vsm_change_sfc_cstlayout_by_cdiv()
{
	SFC_T isfc;
	VSM_DIV_E dtype;

	DMSG(9, "dtype:%d", ivsm.ssm.sfc.div);

	if ((ivsm.ssm.sfc.div == VSM_DIV8) || (ivsm.ssm.sfc.div == VSM_DIV6)) {
		dtype = VSM_DIV4;
		ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
	}
	else if (ivsm.ssm.cstlayout_idx+1 < _vsm_get_cstlayout_itemcnt(ivsm.ssm.sfc.div)) {
		dtype = ivsm.ssm.sfc.div;
		ivsm.ssm.cstlayout_idx = _vsm_increase_cstlayout_idx(dtype, ivsm.ssm.cstlayout_idx);
	}
	else {
		if (ivsm.ssm.sfc.div == VSM_DIV16) dtype = VSM_DIV9;
		else if (ivsm.ssm.sfc.div == VSM_DIV9) dtype = VSM_DIV4;
		else if (ivsm.ssm.sfc.div == VSM_DIV4) dtype = VSM_DIV1;
		else dtype = VSM_DEFAULT_DIV;

		ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
	}

	ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);

	DMSG(9, "dtype:%d", dtype);

	memset(&isfc, 0x00, sizeof(SFC_T));
	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	_set_dtype(&isfc, dtype);
	_set_blind_sfc(TRUE);
	_change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(500);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);	
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();

	return 0;
}
gint vsm_change_sfc_cstlayout_prev(VSM_DIV_E dtype)
{
	SFC_T isfc;
	gint focus_ch = vsm_get_focused_channel();
	DispClayoutElement_t *first_elm = 0;

	DMSG(9, "dtype:%d", dtype);
	if (_check_div_no_change(dtype)) return -1;

	if (vsm_get_div() != dtype) 
	{
		ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
		ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);

		first_elm = ivsm.ssm.cstlayout_elm;

		while (_vsm_is_channel_check_cstlayout(ivsm.ssm.cstlayout_elm, focus_ch) == 0) {
			ivsm.ssm.cstlayout_idx = _vsm_decrease_cstlayout_idx(dtype, ivsm.ssm.cstlayout_idx);
			ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);

			if (first_elm == ivsm.ssm.cstlayout_elm) break;
		}
	}
	else 
	{
		ivsm.ssm.cstlayout_idx = _vsm_decrease_cstlayout_idx(dtype, ivsm.ssm.cstlayout_idx);
    	ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);
	}

	memset(&isfc, 0x00, sizeof(SFC_T));
	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	_set_dtype(&isfc, dtype);
	_set_blind_sfc(TRUE);
	_change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(500);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);	
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();

	return 0;
}

gint vsm_change_sfc_cstlayout_next(VSM_DIV_E dtype)
{
	SFC_T isfc;
	gint focus_ch = vsm_get_focused_channel();;
	DispClayoutElement_t *first_elm = 0;
	gchar next_ch;

	DMSG(9, "dtype:%d", dtype);
	if (_check_div_no_change(dtype)) return -1;

	if (vsm_get_div() != dtype)  
	{
		ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(dtype, 0);
		ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);
		
		first_elm = ivsm.ssm.cstlayout_elm;

		while (_vsm_is_channel_check_cstlayout(ivsm.ssm.cstlayout_elm, focus_ch) == 0) {
			ivsm.ssm.cstlayout_idx = _vsm_increase_cstlayout_idx(dtype, ivsm.ssm.cstlayout_idx);
			ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);

			if (first_elm == ivsm.ssm.cstlayout_elm) break;
    	}
	}
	else 
	{
		ivsm.ssm.cstlayout_idx = _vsm_increase_cstlayout_idx(dtype, ivsm.ssm.cstlayout_idx);
		ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(dtype, ivsm.ssm.cstlayout_idx);
	}

	memset(&isfc, 0x00, sizeof(SFC_T));
	_stop_vsm_func();
	_get_current_sfc(&isfc);
	_reset_win_id(&isfc);
	if (dtype == VSM_DIV6 || dtype == VSM_DIV8)
	{
		next_ch = _get_next_ch(dtype);
		_set_change_div(&isfc, next_ch, dtype);
	}
	else	
		_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	_set_dtype(&isfc, dtype);
	_set_blind_sfc(TRUE);
	_change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(500);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);	
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();

	return 0;
}

gint vsm_has_next_arch_ch()
{
	SFC_T isfc;
	gchar ch;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	_get_current_sfc(&isfc);
	if (!_get_next_ch_arch_play(&isfc, &ch)) return 0;
	return 1;
}

gint vsm_change_sfc_arch_play(void)
{
	SFC_T isfc;
	gchar ch;
	guint arch_ch_mask;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	_get_current_sfc(&isfc);
	if (!_get_next_ch_arch_play(&isfc, &ch)) return -1;

	DMSG(9, "");
	_reset_win_id(&isfc);
	nf_avi_player_mul_set_ch((guint)ch);
	arch_ch_mask = _get_chmask_arch_play(ch);
	_set_win_id_from_mask(&isfc, arch_ch_mask);
	_set_blind_sfc(TRUE);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	return 0;
}

gint vsm_change_sfc_by_2button_press(gchar win_id)
{
	gchar ch;
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "win_id:%d", win_id);
#if defined(GUI_8CH_SUPPORT)
	if (win_id == VSM_WIN_ID9) return -1;
#endif

	DMSG(9, "");
	_stop_vsm_func();

	if (_get_dtype() != VSM_DIV1)
	{
		_get_current_sfc(&isfc);

		ch = _get_ch(win_id);
		ivsm.ssm.cstlayout_idx = _vsm_set_cstlayout_idx(VSM_DIV1, ch);
		ivsm.ssm.cstlayout_elm = _vsm_get_cstlayout_element(VSM_DIV1, ch);
		_reset_win_id(&isfc);
		_vsm_get_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	}
	else
		_get_multi_info(&isfc);

	_set_blind_sfc(TRUE);
    _change_fisheye_dewarp(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_reset_recover_sfc();
	_reset_focus();
	_vsm_func_run_popup();
	return 0;
}

gint vsm_change_sfc_by_menu_opened(gchar ch)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch:%d", ch);
	_stop_vsm_event_func();
	_set_recover_sfc();
	_set_full_mode_1div(&isfc, ch);
	_set_blind_sfc(TRUE);
	if (_get_video_mode() == VMODE_LV)
		_live_change(&isfc);
	else if (_get_video_mode() == VMODE_PB)
		_playback_change(&isfc);
	_unset_blind_sfc_delay(120);
	_set_current_sfc(&isfc);
	_unset_vca_ditid();
	_unset_dvabx_ditid();
	_reset_focus();
	return 0;
}

gint vsm_recover_sfc_by_menu_closed(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	_set_blind_sfc(TRUE);
	_get_current_sfc(&isfc);
	_get_recover_sfc(&isfc);
	_reset_omode(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(120);
	_vsm_func_run_popup();
	return 0;
}

gint vsm_change_sfc_by_zoom_opened(gchar ch)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

    zoom_mode_on = TRUE;
	DMSG(9, "ch:%d", ch);
	_stop_vsm_event_func();
	_set_recover_sfc();
	_set_full_mode_1div(&isfc, ch);
	_set_blind_sfc(TRUE);
	if (_get_video_mode() == VMODE_LV)
		_vvm_live_change_osd(&isfc);
	else if (_get_video_mode() == VMODE_PB)
		_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_set_current_sfc(&isfc);
	_unset_vca_ditid();
	_unset_dvabx_ditid();
	_reset_focus();
	return 0;
}

gint vsm_recover_sfc_by_zoom_closed(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

    zoom_mode_on = FALSE;
	DMSG(9, "");
	_set_blind_sfc(TRUE);
	_get_current_sfc(&isfc);
	_get_recover_sfc(&isfc);
	_reset_omode(&isfc);
	_change_sfc(&isfc, TRUE);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	_unset_blind_sfc_delay(120);
	_vsm_func_run_popup();
	return 0;
}

gint vsm_change_sfc_by_zoom_changed(gchar ch)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch:%d", ch);
	_set_full_mode_1div(&isfc, ch);
	_set_blind_sfc(TRUE);
	if (_get_video_mode() == VMODE_LV)
		_vvm_live_change_osd(&isfc);
	else if (_get_video_mode() == VMODE_PB)
		_vvm_playback_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_set_current_sfc(&isfc);
	return 0;
}

gint vsm_change_sfc_by_zoom_opened_ptz(gchar ch)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "ch:%d", ch);
	_set_full_mode_1div(&isfc, ch);
	_set_blind_sfc(TRUE);
	_vom_live_change_video(&isfc);
	_vvm_live_change_osd(&isfc);
	_unset_blind_sfc_delay(120);
	_set_current_sfc(&isfc);
	_reset_focus();
	_set_video_mode(VMODE_LV);
	return 0;
}

gint vsm_recover_sfc_by_zoom_closed_ptz(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "");
	_set_blind_sfc(TRUE);
	_get_current_sfc(&isfc);
	_vom_live_stop_video();
	_unset_blind_sfc_delay(120);
	_set_video_mode(VMODE_NONE);
	return 0;
}

gint vsm_change_hide_video()
{
	DMSG(9, "");
	_stop_vsm_event_func();
	_vvm_set_clear_vwnd();
	_set_blind_sfc(TRUE);
	return 0;
}

gint vsm_change_show_video()
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));
	
	DMSG(9, "");
	_get_current_sfc(&isfc);
	_vvm_unset_clear_vwnd();
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc(TRUE);
	_vsm_func_run_popup();
	return 0;
}

gint vsm_set_covert_by_user(gchar *user_name)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "user_name:%s", user_name);
	DMSG(9, "_get_video_mode:%d", _get_video_mode());
	if (_get_video_mode() == VMODE_NONE) return -1;

	DMSG(9, "");
	_get_current_sfc(&isfc);
	_set_user_covert_param(&isfc);
	_change_sfc(&isfc, TRUE);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	return 0;
}

gint vsm_set_covert_by_logout(void)
{
	SFC_T isfc;

	memset(&isfc, 0x00, sizeof(SFC_T));

	DMSG(9, "_get_video_mode:%d", _get_video_mode());
	if (_get_video_mode() == VMODE_NONE) return -1;

	DMSG(9, "");
	_get_current_sfc(&isfc);
	_set_logout_covert_param(&isfc);
	_change_sfc(&isfc, TRUE);
	_set_vca_ditid(&isfc);
	_set_dvabx_ditid(&isfc);
	return 0;
}

gint vsm_mouse_motion_detect(guint x, guint y)
{
	gint dmode;

	if (_get_osd_mode() == OMODE_NORMAL) return -1;

	if(!VW_Timeline_IsShown() && VW_Timeline_IsInArea(x, y))
	{
		if (_get_video_mode() == VMODE_LV) {
			dmode = VW_Timeline_get_disp_mode();

			// always off, event on
			if(dmode == 1 || dmode == 3)
				goto out;
		}

		VW_Timeline_Show();
	}
	else if(VW_Timeline_IsShown() && !VW_Timeline_IsInArea(x, y))
	{
		if (_get_video_mode() == VMODE_LV) {
			dmode = VW_Timeline_get_disp_mode();

			// always on
			if(dmode == 2)
				goto out;
		}

		VW_Timeline_Hide();
	}
	out:

	if (_get_video_mode() == VMODE_LV) {
		if(!VW_Live_StatusBar_IsShown() && VW_Live_StatusBar_IsInArea(x, y)) {
			VW_Live_StatusBar_Show();
		}else if(VW_Live_StatusBar_IsShown() && !VW_Live_StatusBar_IsInArea(x, y))  {
			if(VW_Live_StatusBar_On_Time() == 1)
				VW_Live_StatusBar_Hide();
		}
	}

	return 0;
}

gchar vsm_get_current_ch(gchar win_id)
{
	return _vsm_get_ch(win_id);
}

gchar vsm_get_current_win(gchar ch)
{
	return _vsm_get_win(ch);
}

gint vsm_get_covert_state(gchar state[GUI_CHANNEL_CNT], guint ch)
{
	SFC_T isfc;
	gint i;

	memset(&isfc, 0x00, sizeof(SFC_T));

	_get_current_sfc(&isfc);
	_set_user_covert_param(&isfc);

	if(state)
	{
		for(i=0; i<GUI_CHANNEL_CNT; i++)
		{
			if(isfc.cinfo[i].covert) state[i] =  1;
			else 				  	 state[i] =  0;
		}
	}

	return isfc.cinfo[ch].covert;
}

gint vsm_untilkey_stop()
{
	gint retVal = -1;

	if (VW_OSD_Popup_is_untilkey() == 0)
	{
		_vsm_close_osd_popup();
		retVal = 0;
	}

	if (_vsm_func_popup_is_until_key() == 0)
	{
		_vsm_func_recover_popup();
		retVal = 0;
	}

	return retVal;
}

gint vsm_switch_channel(gint from_ch, gint to_ch)
{
    SFC_T isfc;
	gint is_matched;

	memset(&isfc, 0x00, sizeof(SFC_T));

    if(from_ch == to_ch)  return -1;
	if(_get_win_id(from_ch) == -1) return -1;

    _get_current_sfc(&isfc);
    _stop_vsm_func();
    _set_blind_sfc(TRUE);

	is_matched = _vsm_is_matched_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
	g_message("%s, %d, matched:%d", __FUNCTION__, __LINE__, is_matched);

    isfc.cinfo[from_ch].win_id = _get_win_id(to_ch);
    isfc.cinfo[to_ch].win_id = _get_win_id(from_ch);

    if(_get_video_mode() == VMODE_LV)
    {
        //_set_clear_sfc(TRUE);
        //_vvm_unset_clear_vwnd();
        _live_change(&isfc);
    }
    else
    {
        //_set_clear_sfc(TRUE);
        //_unset_clear_sfc(FALSE);
        _playback_change(&isfc);
    }

	_reset_focus();
    _set_current_sfc(&isfc);
	_change_sfc(&isfc, TRUE);
	_unset_blind_sfc_delay(600);
    _vsm_func_run_popup();
    var_set_active_layout(-1);

	if (is_matched) {
		_vsm_set_sfc_cstlayout(&isfc, ivsm.ssm.cstlayout_elm);
		_vsm_save_cstlayout();
	}

    return 0;
}

#if 0
// current not use
gint vsm_switch_select_channel(gint *data)
{
    SFC_T isfc;
    gint i;

    _get_current_sfc(&isfc);
    _stop_vsm_func();

    for(i=0;i<GUI_CHANNEL_CNT;i++)
    {
        isfc.cinfo[i].win_id = data[i];
    }

    if(_get_video_mode() == VMODE_LV)
    {
        _set_clear_sfc(TRUE);
        _vvm_unset_clear_vwnd();
        _live_change(&isfc);
        _vvm_refresh_vwnd_all();
    }
    else
    {
        _vom_playback_restart_by_sfc(&isfc);
        _vvm_playback_change_osd(&isfc);
        _vvm_refresh_vwnd_all();
    }

    _set_current_sfc(&isfc);
    _vsm_func_run_popup();
    var_set_active_layout(-1);


    return 0;
}
#endif

