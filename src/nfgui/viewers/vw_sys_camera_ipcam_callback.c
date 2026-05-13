#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"

#include "vw_set_date_time_table.h"
#include "vw_sys_camera_ipcam_internal.h"

static guint tid;

static gint _sync_spin_object(FIXED_INFO_T *fixed_info, NFOBJECT *obj, gint val)
{
    gint i, j;
    guint64 key = 0;
    gchar strBuf[8];

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", val);

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (fixed_info->row_info[i]->col_info[j]->obj == obj)
                key = fixed_info->row_info[i]->col_info[j]->key;
        }
    }

    if (!key) return -1;

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if ((fixed_info->row_info[i]->col_info[j]->key == key)
                && (fixed_info->row_info[i]->col_info[j]->obj_info->obj_type == OBJ_SPIN))
            {
                nfui_spin_button_set_text((NFSPINBUTTON*)fixed_info->row_info[i]->col_info[j]->obj, strBuf);
            }
        }
    }

    return 0;
}

static gint _sync_slider_object(FIXED_INFO_T *fixed_info, NFOBJECT *obj, gint val)
{
    gint i, j;
    guint64 key = 0;

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (fixed_info->row_info[i]->col_info[j]->obj == obj)
                key = fixed_info->row_info[i]->col_info[j]->key;
        }
    }

    if (!key) return -1;

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if ((fixed_info->row_info[i]->col_info[j]->key == key)
                && (fixed_info->row_info[i]->col_info[j]->obj_info->obj_type == OBJ_SLIDER))
            {
                cw_slider_set_value(fixed_info->row_info[i]->col_info[j]->obj, val);
                nfui_signal_emit(fixed_info->row_info[i]->col_info[j]->obj, GDK_EXPOSE, TRUE);
            }
        }
    }

    return 0;
}

static gint _check_matched_object(FIXED_INFO_T *fixed_info, NFOBJECT *obj)
{
    gint i, j;
    guint64 key = 0;
    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (fixed_info->row_info[i]->col_info[j]->obj == obj) return 0;
        }
    }
    return -1;
}

static NFOBJECT *_get_nickname_object(FIXED_INFO_T *fixed_info, gchar *nickname)
{
    gint i, j;
    guint64 key = 0;
    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (strcmp(fixed_info->row_info[i]->col_info[j]->obj_info->nickname, nickname) == 0) {
                return fixed_info->row_info[i]->col_info[j]->obj;
            }
        }
    }
    return 0;
}

static gint _get_data_dnn_sense_dton_val(gint ch)
{
    return g_ipcamData[ch].dnn_sense_dton;
}

static gint _get_data_dnn_sense_dton_diff_val(gint ch)
{
    return g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.difference;
}

static gint _get_data_dnn_sense_ntod_val(gint ch)
{
    return g_ipcamData[ch].dnn_sense_ntod;
}

static gint _get_data_dnn_sense_ntod_diff_val(gint ch)
{
    return g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.difference;
}


static void _dnn_sense_conf_notice_popup(gpointer data)
{
    gint ret;

    NFUTIL_THREADS_ENTER();
    ret = nftool_mbox_4_line(NULL, "NOTICE", "\n", "1.The difference between day and night sensitivity is set as 3.",
                                                                  "2.The sensitivity to day is set higher than night.",
                                                                  "\n", NFTOOL_MB_OK);
    if(ret == NFTOOL_MB_OK){
        g_source_remove(tid);
        tid = 0;
    }
    NFUTIL_THREADS_LEAVE();
}

static gboolean _check_diff_val_of_dnn_sense_dton(gint ch, gint *val)
{
    gint dton_val, ntod_val, diff_val;
    gint dton_diff, ntod_diff;

    dton_val = *val;
    dton_diff = _get_data_dnn_sense_dton_diff_val(ch);
    ntod_val = _get_data_dnn_sense_ntod_val(ch);
    ntod_diff = _get_data_dnn_sense_ntod_diff_val(ch);

    diff_val = ntod_val - dton_val;

    if(diff_val < ntod_diff)
    {
        *val = g_ipcamData[ch].dnn_sense_dton;
        return FALSE;
    }

    return TRUE;
}

static gboolean _check_diff_val_of_dnn_sense_ntod(gint ch, gint *val)
{
    gint dton_val, ntod_val, diff_val;
    gint dton_diff, ntod_diff;

    dton_val = _get_data_dnn_sense_dton_val(ch);
    dton_diff = _get_data_dnn_sense_dton_diff_val(ch);
    ntod_val = *val;
    ntod_diff = _get_data_dnn_sense_ntod_diff_val(ch);

    diff_val = ntod_val - dton_val;

    if(diff_val < ntod_diff)
    {
        *val = g_ipcamData[ch].dnn_sense_ntod;
        return FALSE;
    }

    return TRUE;
}


static gint _check_compare_with_max_shutter_speed(gchar *base_caption, gint ch)
{
 	gchar *m, *b;
 	gchar base_text[12];
 	gchar max_text[12];
 	gint i;

    memset(base_text, 0x00, sizeof(base_text));
    memset(max_text, 0x00, sizeof(max_text));

    for (i = 0; i < g_ipcam_data_temp[ch].max_shutter_temp_cnt; i++)
    {
        if(g_ipcam_data_temp[ch].max_shutter_temp[i].value == g_ipcamData[ch].max_shutter)  break;
    }

	m = strchr(&g_ipcam_data_temp[ch].max_shutter_temp[i].caption, '/');
	if (m) strcpy(max_text, m + 1);

	b = strchr(base_caption, '/');
	if (b) strcpy(base_text, b + 1);

    if(atoi(base_text) < atoi(max_text)) return 1;

    return 0;
}

static gint _check_compare_with_base_shutter_speed(gchar *max_caption, gint ch)
{
    gchar *m, *b;
    gchar base_text[12];
    gchar max_text[12];
    gint i;

    memset(base_text, 0x00, sizeof(base_text));
    memset(max_text, 0x00, sizeof(max_text));

    if(!get_supported_base_shutter_speed_support(ch)) return 0;

    for (i = 0; i < g_ipcam_data_temp[ch].base_shutter_temp_cnt; i++)
    {
        if(g_ipcam_data_temp[ch].base_shutter_temp[i].value == g_ipcamData[ch].base_shutter)
        break;
    }

    b = strchr(&g_ipcam_data_temp[ch].base_shutter_temp[i].caption, '/');
    if (b) strcpy(base_text, b + 1);

    m = strchr(max_caption, '/');
    if (m) strcpy(max_text, m + 1);

    if(atoi(base_text) < atoi(max_text)) return 1;

    return 0;
}



static gint _set_ipcam_data(gint ch)
{
    DAL_set_IPCamSetup_data(g_ipcamData[ch], ch);
    DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE, ch);
    return 0;
}

static gint _get_data_rotate(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.mirror[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.mirror_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.mirror[i].value == g_ipcamData[ch].rotate)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.mirror[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.mirror[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.mirror[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_focus_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.focus[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.focus_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.focus[i].value == g_ipcamData[ch].focus_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.focus[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.focus[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.focus[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_focus_limit(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.focus_limit[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.focus_limit_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.focus_limit[i].value == g_ipcamData[ch].focus_limit)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.focus_limit[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.focus_limit[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.focus_limit[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_ir_correction(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.ir_correction[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.ir_correction_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.ir_correction[i].value == g_ipcamData[ch].ir_correction)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.ir_correction[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.ir_correction[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.ir_correction[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_stabilizer(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.stabilizer[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.stabilizer_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.stabilizer[i].value == g_ipcamData[ch].stabilizer)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.stabilizer[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.stabilizer[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.stabilizer[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_wb_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.wb[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.wb_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.wb[i].value == g_ipcamData[ch].wb_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.wb[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.wb[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.wb[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_mwb_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].image.profile.supported_image & g_ipcam_manage[ch].image.profile.mwb[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].image.profile.mwb_cnt; i++)
    {
        if (g_ipcam_manage[ch].image.profile.mwb[i].value == g_ipcamData[ch].mwb_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].image.profile.mwb[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].image.profile.mwb[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].image.profile.mwb[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_ircut_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.ircut[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.ircut[i].value == g_ipcamData[ch].day_night_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.ircut[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.ircut[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.ircut[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_ircutm_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.ircutm[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.ircutm_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.ircutm[i].value == g_ipcamData[ch].day_night_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.ircutm[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.ircutm[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.ircutm[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_dnn_toggle(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.dnn_toggle[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dnn_toggle_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].value == g_ipcamData[ch].day_night_duration)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_adaptive_ir(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.adaptive_ir[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.adaptive_ir_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].value == g_ipcamData[ch].adaptive_ir)
        {

            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].enable_category;
        }
    }

    return 0;
}


static gint _get_data_exposure_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.mode[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.mode_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.mode[i].value == g_ipcamData[ch].exposure_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.mode[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.mode[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.mode[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_priority(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.priority[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.priority_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.priority[i].value == g_ipcamData[ch].exposure_priority)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.priority[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.priority[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.priority[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_slowshutter(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.slowshutter[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.slowshutter_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.slowshutter[i].value == g_ipcamData[ch].slow_shutter)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.slowshutter[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.slowshutter[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.slowshutter[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_maxagc(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.maxagc[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.maxagc_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.maxagc[i].value == g_ipcamData[ch].max_agc)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.maxagc[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.maxagc[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.maxagc[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_blc_control(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.blc[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.blc_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.blc[i].value == g_ipcamData[ch].blc_control)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.blc[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.blc[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.blc[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_hlc(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.hlc[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.hlc_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.hlc[i].value == g_ipcamData[ch].hlc)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.hlc[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.hlc[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.hlc[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_defog(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.defog[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.defog_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.defog[i].value == g_ipcamData[ch].defog)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.defog[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.defog[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.defog[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_wdr_mode(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.wdr[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.wdr_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.wdr[i].value == g_ipcamData[ch].wdr_mode)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.wdr[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.wdr[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.wdr[i].enable_category;
        }
    }

    return 0;
}

static gint _get_data_exposure_dnr(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable)
{
    gint i;

    if (g_ipcam_manage[ch].exposure.profile.supported_exposure & g_ipcam_manage[ch].exposure.profile.dnr[0].category == 0) return -1;

    for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt; i++)
    {
        if (g_ipcam_manage[ch].exposure.profile.dnr[i].value == g_ipcamData[ch].dnr)
        {
            if (dependent) *dependent = g_ipcam_manage[ch].exposure.profile.dnr[i].dependent_category;
            if (visible) *visible = g_ipcam_manage[ch].exposure.profile.dnr[i].visible_category;
            if (enable) *enable = g_ipcam_manage[ch].exposure.profile.dnr[i].enable_category;
        }
    }

    return 0;
}

static gint _get_visible_image_category(gint ch, guint64 *visible)
{
    guint64 dependent_category, visible_category;
    gint support;

    *visible = g_ipcam_manage[ch].image.profile.supported_image;

    g_message("[IPCAM_CALLBACK] %s, %d, ch:%d, visible:%08llX", __FUNCTION__, __LINE__, ch, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_rotate(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_focus_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_focus_limit(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_ir_correction(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_stabilizer(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_wb_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_mwb_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);


    return 0;
}

static gint _get_enable_image_category(gint ch, guint64 *enable)
{
    guint64 dependent_category, enable_category;
    gint support;

    if (g_ipcam_manage[ch].image_default)
    {
        *enable = 0;
        return -1;
    }

    *enable = g_ipcam_manage[ch].image.profile.supported_image;

    g_message("[IPCAM_CALLBACK] %s, %d, ch:%d, enable:%08llX", __FUNCTION__, __LINE__, ch, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_rotate(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_focus_mode(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_focus_limit(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_ir_correction(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_stabilizer(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_wb_mode(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_mwb_mode(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);


    return 0;
}

static gint _get_visible_exposure_category(gint ch, guint64 *visible)
{
    guint64 dependent_category, visible_category;
    gint support;

    *visible = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    //g_message("[IPCAM_CALLBACK] %s, %d, ch:%d, visible:%08llX", __FUNCTION__, __LINE__, ch, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_wdr_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_priority(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_slowshutter(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_maxagc(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_blc_control(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = get_dc_iris_category_callback(ch, &dependent_category, &visible_category, 0, visible);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = get_antiflicker_category_callback(ch, &dependent_category, &visible_category, 0, visible);
    *visible &= ~(dependent_category & ~visible_category);

    //    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
    //    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = get_maxshutter_speed_category_callback(ch, &dependent_category, &visible_category, 0, visible);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = get_base_shutter_speed_category_callback(ch, &dependent_category, &visible_category, 0, visible);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_dnr(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_ircut_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_ircutm_mode(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_dnn_toggle(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_adaptive_ir(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_hlc(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    dependent_category = 0;
    visible_category = 0;
    support = _get_data_exposure_defog(ch, &dependent_category, &visible_category, 0);
    *visible &= ~(dependent_category & ~visible_category);

//    g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, visible_category:%08llX", __FUNCTION__, __LINE__, dependent_category, visible_category);
//    g_message("[IPCAM_CALLBACK] %s, %d, visible:%08llX", __FUNCTION__, __LINE__, *visible);

    return 0;
}

static gint _get_enable_exposure_category(gint ch, guint64 *enable)
{
    guint64 dependent_category, enable_category;
    gint support;

    if (g_ipcam_manage[ch].exposure_default)
    {
        *enable = 0;
        return -1;
    }

    *enable = g_ipcam_manage[ch].exposure.profile.supported_exposure;

    //g_message("[IPCAM_CALLBACK] %s, %d, ch:%d, enable:%08llX", __FUNCTION__, __LINE__, ch, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_mode(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_wdr_mode(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_priority(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_slowshutter(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_maxagc(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_blc_control(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = get_dc_iris_category_callback(ch, &dependent_category, 0, &enable_category, enable);
     *enable &= ~(dependent_category & ~enable_category);

     //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
     //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = get_antiflicker_category_callback(ch, &dependent_category, 0, &enable_category, enable);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = get_maxshutter_speed_category_callback(ch, &dependent_category, 0, &enable_category, enable);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = get_base_shutter_speed_category_callback(ch, &dependent_category, 0, &enable_category, enable);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_dnr(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    if(g_ipcamData[ch].exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT ||
       g_ipcamData[ch].exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT_X10)
    {
        dependent_category = 0;
        enable_category = 0;
        support = _get_data_ircutm_mode(ch, &dependent_category, 0, &enable_category);
        *enable &= ~(dependent_category & ~enable_category);

        //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
        //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);
    }
    else
    {
        dependent_category = 0;
        enable_category = 0;
        support = _get_data_ircut_mode(ch, &dependent_category, 0, &enable_category);
        *enable &= ~(dependent_category & ~enable_category);

 //     g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //     g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);
    }

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_dnn_toggle(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_adaptive_ir(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_hlc(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    dependent_category = 0;
    enable_category = 0;
    support = _get_data_exposure_defog(ch, &dependent_category, 0, &enable_category);
    *enable &= ~(dependent_category & ~enable_category);

 //   g_message("[IPCAM_CALLBACK] %s, %d, dependent_category:%08llX, enable_category:%08llX", __FUNCTION__, __LINE__, dependent_category, enable_category);
 //   g_message("[IPCAM_CALLBACK] %s, %d, enable:%08llX", __FUNCTION__, __LINE__, *enable);

    return 0;
}

static gint _change_image_fixed(gint ch, gint expose)
{
    guint64 visible, enable;
    gint changed = 0;

    _get_visible_image_category(ch, &visible);
    _get_enable_image_category(ch, &enable);

    if ((g_ipcam_subFixed[ch].imageSet.visible != visible) || (g_ipcam_subFixed[ch].imageSet.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].imageSet.visible = visible;
        g_ipcam_subFixed[ch].imageSet.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].imageSet);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].imageSet);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].imageSet.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].rotate.visible != visible) || (g_ipcam_subFixed[ch].rotate.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].rotate.visible = visible;
        g_ipcam_subFixed[ch].rotate.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].rotate);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].rotate);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].rotate.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].focusMode.visible != visible) || (g_ipcam_subFixed[ch].focusMode.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].focusMode.visible = visible;
        g_ipcam_subFixed[ch].focusMode.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].focusMode);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].focusMode);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].focusMode.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].wbMode.visible != visible) || (g_ipcam_subFixed[ch].wbMode.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].wbMode.visible = visible;
        g_ipcam_subFixed[ch].wbMode.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].wbMode);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].wbMode);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].wbMode.fixed, GDK_EXPOSE, TRUE);
    }

    return changed;
}

static gint _change_exposure_fixed(gint ch, gint expose)
{
    guint64 visible, enable;
    gint changed = 0;

    _get_visible_exposure_category(ch, &visible);
    _get_enable_exposure_category(ch, &enable);

    if ((g_ipcam_subFixed[ch].exposureMode.visible != visible) || (g_ipcam_subFixed[ch].exposureMode.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureMode.visible = visible;
        g_ipcam_subFixed[ch].exposureMode.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureMode);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureMode);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureMode.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureTime.visible != visible) || (g_ipcam_subFixed[ch].exposureTime.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureTime.visible = visible;
        g_ipcam_subFixed[ch].exposureTime.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureTime);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureTime);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureTime.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureGain.visible != visible) || (g_ipcam_subFixed[ch].exposureGain.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureGain.visible = visible;
        g_ipcam_subFixed[ch].exposureGain.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureGain);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureGain);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureGain.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureIris.visible != visible) || (g_ipcam_subFixed[ch].exposureIris.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureIris.visible = visible;
        g_ipcam_subFixed[ch].exposureIris.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureIris);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureIris);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureIris.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureBlc.visible != visible) || (g_ipcam_subFixed[ch].exposureBlc.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureBlc.visible = visible;
        g_ipcam_subFixed[ch].exposureBlc.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureBlc);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureBlc);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureBlc.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureHlc.visible != visible) || (g_ipcam_subFixed[ch].exposureHlc.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureHlc.visible = visible;
        g_ipcam_subFixed[ch].exposureHlc.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureHlc);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureHlc);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureHlc.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureDefog.visible != visible) || (g_ipcam_subFixed[ch].exposureDefog.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureDefog.visible = visible;
        g_ipcam_subFixed[ch].exposureDefog.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureDefog);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureDefog);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureDefog.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureAnti.visible != visible) || (g_ipcam_subFixed[ch].exposureAnti.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureAnti.visible = visible;
        g_ipcam_subFixed[ch].exposureAnti.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureAnti);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureAnti);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureAnti.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureWdr.visible != visible) || (g_ipcam_subFixed[ch].exposureWdr.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureWdr.visible = visible;
        g_ipcam_subFixed[ch].exposureWdr.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureWdr);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureWdr);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureWdr.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].exposureDnr.visible != visible) || (g_ipcam_subFixed[ch].exposureDnr.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].exposureDnr.visible = visible;
        g_ipcam_subFixed[ch].exposureDnr.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].exposureDnr);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].exposureDnr);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].exposureDnr.fixed, GDK_EXPOSE, TRUE);
    }

    if ((g_ipcam_subFixed[ch].dnMode.visible != visible) || (g_ipcam_subFixed[ch].dnMode.enable != enable))
    {
        changed = 1;

        g_ipcam_subFixed[ch].dnMode.visible = visible;
        g_ipcam_subFixed[ch].dnMode.enable = enable;

        _set_visible_subFixed(&g_ipcam_subFixed[ch].dnMode);
        _set_enable_subFixed(&g_ipcam_subFixed[ch].dnMode);
        if (expose) nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.fixed, GDK_EXPOSE, TRUE);
    }
    return changed;
}





//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
//
// combo handler
//

gboolean _post_rotate_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].rotate, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].rotate = g_ipcam_manage[ch].image.profile.mirror[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].rotate, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.mirror_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.mirror[i].value == g_ipcamData[ch].rotate)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_focus_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].focus_mode = g_ipcam_manage[ch].image.profile.focus[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.focus_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.focus[i].value == g_ipcamData[ch].focus_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_focus_limit_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].focus_limit = g_ipcam_manage[ch].image.profile.focus_limit[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.focus_limit_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.focus_limit[i].value == g_ipcamData[ch].focus_limit)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_ir_correction_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].ir_correction= g_ipcam_manage[ch].image.profile.ir_correction[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.ir_correction_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.ir_correction[i].value == g_ipcamData[ch].ir_correction)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_stabilizer_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].stabilizer= g_ipcam_manage[ch].image.profile.stabilizer[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.stabilizer_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.stabilizer[i].value == g_ipcamData[ch].stabilizer)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_white_balance_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].wb_mode = g_ipcam_manage[ch].image.profile.wb[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.wb_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.wb[i].value == g_ipcamData[ch].wb_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_ircut_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;


    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;
        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].day_night_mode = g_ipcam_manage[ch].exposure.profile.ircut[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            if(index == 1)
            {
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }
            else
            {
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }

            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj, GDK_EXPOSE, TRUE);
        }
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        _change_exposure_fixed(ch, 0);
        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.ircut_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.ircut[i].value == g_ipcamData[ch].day_night_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            if(index == 1)
            {
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }
            else
            {
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }

            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

gboolean _post_ircutm_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;
        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].day_night_mode = g_ipcam_manage[ch].exposure.profile.ircutm[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            if(index == 1)
            {
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }
            else
            {
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }

            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj, GDK_EXPOSE, TRUE);
        }
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);
        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.ircutm_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.ircutm[i].value == g_ipcamData[ch].day_night_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            if(index == 1)
            {
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }
            else
            {
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj);
                nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj);
            }

            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[6]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[0]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[3]->obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}


gboolean _post_mwb_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].mwb_mode = g_ipcam_manage[ch].image.profile.mwb[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_image_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_image_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].image.profile.mwb_cnt; i++)
        {
            if (g_ipcam_manage[ch].image.profile.mwb[i].value == g_ipcamData[ch].mwb_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_dnn_toggle_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].day_night_duration = g_ipcam_manage[ch].exposure.profile.dnn_toggle[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dnn_toggle_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.dnn_toggle[i].value == g_ipcamData[ch].day_night_duration)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_adaptive_ir_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].adaptive_ir = g_ipcam_manage[ch].exposure.profile.adaptive_ir[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.adaptive_ir_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.adaptive_ir[i].value == g_ipcamData[ch].adaptive_ir)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;

}

gboolean _post_exposure_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;
    NFOBJECT *combo_obj;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(g_ipcamData[ch].exposure_mode == g_ipcam_manage[ch].exposure.profile.mode[index].value) return FALSE;

        g_ipcamData[ch].exposure_mode = g_ipcam_manage[ch].exposure.profile.mode[index].value;

        if (g_ipcamData[ch].exposure_mode == NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT ||
            g_ipcamData[ch].exposure_mode ==  NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_NPT_X10)
        {
            combo_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUTM, _post_ircutm_mode_combo_event_handler);
            index = nfui_combobox_get_cur_index((NFCOMBOBOX*)combo_obj);
			g_ipcamData[ch].day_night_mode = g_ipcam_manage[ch].exposure.profile.ircutm[index].value;
			if(combo_obj){
    			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)combo_obj, index);
    			nfui_signal_emit((NFCOMBOBOX*)combo_obj, GDK_EXPOSE, TRUE);
    	    }
		}
		else
		{
			combo_obj = find_obj_exposure(ch, NF_IPCAM_EXPOSURE_ONVIF_IRCUT, _post_ircut_mode_combo_event_handler);
			index = nfui_combobox_get_cur_index((NFCOMBOBOX*)combo_obj);
			g_ipcamData[ch].day_night_mode = g_ipcam_manage[ch].exposure.profile.ircut[index].value;
			if(combo_obj){
			    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)combo_obj, index);
			    nfui_signal_emit((NFCOMBOBOX*)combo_obj, GDK_EXPOSE, TRUE);
			}
		}

        _set_ipcam_data(ch);

        if(!strcmp(nfui_combobox_get_value(obj),"MANUAL"))   _set_wdr_value_sync(ch, "OFF");

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.mode_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.mode[i].value == g_ipcamData[ch].exposure_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_blc_control_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].blc_control = g_ipcam_manage[ch].exposure.profile.blc[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.blc_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.blc[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.blc[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.blc_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.blc[i].value == g_ipcamData[ch].blc_control)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_hlc_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureHlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].hlc = g_ipcam_manage[ch].exposure.profile.hlc[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureHlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.hlc_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.hlc[i].value == g_ipcamData[ch].hlc)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_defog_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureDefog, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].defog= g_ipcam_manage[ch].exposure.profile.defog[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.defog_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.defog[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.defog[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureDefog, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.defog_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.defog[i].value == g_ipcamData[ch].defog)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_exposure_priority_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].exposure_priority = g_ipcam_manage[ch].exposure.profile.priority[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.priority_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.priority[i].value == g_ipcamData[ch].exposure_priority)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_antiflicker_m_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch=0; ch< GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].antiflicker = g_ipcam_manage[ch].exposure.profile.antiflicker_motion[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.antiflicker_motion[index].selected = 1;

        _set_ipcam_data(ch);
        _set_antiflicker_value_sync(ch, nfui_combobox_get_value(obj));

        is_changed = _change_exposure_fixed(ch, 1);
        if( is_changed ) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch =0; ch < GUI_CHANNEL_CNT; ch ++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT ) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i = 0; i< g_ipcam_manage[ch].exposure.profile.antiflicker_motion_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.antiflicker_motion[i].value == g_ipcamData[ch].antiflicker)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_antiflicker_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].antiflicker = g_ipcam_manage[ch].exposure.profile.antiflicker[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.antiflicker[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.antiflicker[index].selected = 1;

        _set_ipcam_data(ch);
        _set_antiflicker_value_sync(ch, nfui_combobox_get_value(obj));

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.antiflicker[i].value == g_ipcamData[ch].antiflicker)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_antiflicker_m_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].antiflicker = g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[index].selected = 1;

        _set_ipcam_data(ch);
        _set_antiflicker_value_sync(ch, nfui_combobox_get_value(obj));

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.antiflicker_motion_off[i].value == g_ipcamData[ch].antiflicker)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_antiflicker_a_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].antiflicker = g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[index].selected = 1;

        _set_ipcam_data(ch);
        _set_antiflicker_value_sync(ch, nfui_combobox_get_value(obj));

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.antiflicker_auto_off[i].value == g_ipcamData[ch].antiflicker)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_motion_off_on_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_on[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_auto_off_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_off[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_auto_off_on_tv_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);


        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_tv[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;

}

gboolean _post_max_shutter_auto_off_on_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_auto_off_on[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_60_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_60[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_60[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_60[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_60_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_60[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_50_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_50[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_50[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_50[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_50_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_50[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_off[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_off[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_off[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_off_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.max_shutter_off[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_max_shutter_motion_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_base_shutter_speed(&g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_MAX_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].value == g_ipcamData[ch].max_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].max_shutter = g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.max_shutter_motion_off[i].value == g_ipcamData[ch].max_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_100_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_100[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_100[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_100[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_100_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_100[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_120_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_120[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_120[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_120[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_120_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_120[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_100_300_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_100_300_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_100_300[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_100_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_100_5000[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_120_360_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_120_360_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_360[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_120_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_5000[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_120_262_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_120_262_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_120_262[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_30_262_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_30_262_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_262[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_25_100_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_25_100_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_100[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_25_300_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_25_300_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_300[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_25_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_25_5000[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_30_120_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_30_120_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_120[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_30_360_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_30_360_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_360[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_base_shutter_30_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch =0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) ==0)
                break;
        }

        if(ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(_check_compare_with_max_shutter_speed(&g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[index].caption, ch))
        {
            nftool_mbox(NULL, "CONFIRM", "The max shutter speed must be greater than or equal to the base shutter speed.", NFTOOL_MB_OK);

            for (i = 0; i < NF_IPCAM_BASE_SHUTTER_MODE_NTSC_NR; i++)
            {
                if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].value == g_ipcamData[ch].base_shutter)
                {
                    nfui_combobox_set_index_no_expose(obj, i);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

                    break;
                }
            }

            return FALSE;
        }

        g_ipcamData[ch].base_shutter = g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
        if(is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if( evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureAnti, obj) == 0)
                break;
        }

        if( ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for( i =0 ; i< g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000_cnt; i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.base_shutter_30_5000[i].value == g_ipcamData[ch].base_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_exposure_dnr_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index = -1;
    gint is_changed;
    gchar *caption;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureDnr, obj) == 0 )
                break;
        }
        if ( ch == GUI_CHANNEL_CNT) return FALSE;

        caption = nfui_combobox_get_value((NFCOMBOBOX*)obj);

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt; i++)
        {
            if (strcmp(g_ipcam_manage[ch].exposure.profile.dnr[i].caption, caption) == 0)
            {
                index = i;
            }
        }
        if (index == -1) return FALSE;

        g_ipcamData[ch].dnr = g_ipcam_manage[ch].exposure.profile.dnr[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for( ch = 0; ch <GUI_CHANNEL_CNT ; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureDnr,obj) == 0)
            break;
        }

        if( ch == GUI_CHANNEL_CNT ) return FALSE;

        _change_exposure_fixed(ch, 0);

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt;i++)
        {
            if(g_ipcam_manage[ch].exposure.profile.dnr[i].value == g_ipcamData[ch].dnr)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj,i);
        }
    }

  return FALSE;
}


gboolean _post_exposure_slowshutter_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].slow_shutter = g_ipcam_manage[ch].exposure.profile.slowshutter[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.slowshutter_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.slowshutter[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.slowshutter[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.slowshutter_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.slowshutter[i].value == g_ipcamData[ch].slow_shutter)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_exposure_maxagc_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        g_ipcamData[ch].max_agc = g_ipcam_manage[ch].exposure.profile.maxagc[index].value;
        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.maxagc_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.maxagc[i].value == g_ipcamData[ch].max_agc)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_exposure_dciris_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].iris_control = g_ipcam_manage[ch].exposure.profile.dc_iris[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.dc_iris[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.dc_iris[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

    	if(!strcmp(nfui_combobox_get_value(obj),"DEPTH OF FIELD") || !strcmp(nfui_combobox_get_value(obj),"BEST QUALITY"))
        {
            _set_max_shutter_value_sync(ch, 0);
            _set_base_shutter_value_sync(ch, 0);
        }
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        _change_exposure_fixed(ch, 0);

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.dc_iris[i].value == g_ipcamData[ch].iris_control)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;
}

gboolean _post_exposure_dciris_motion_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].iris_control = g_ipcam_manage[ch].exposure.profile.dc_iris_motion[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_motion_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.dc_iris_motion[index].selected = 1;

        _set_ipcam_data(ch);

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

    	if(!strcmp(nfui_combobox_get_value(obj),"DEPTH OF FIELD") || !strcmp(nfui_combobox_get_value(obj),"BEST QUALITY"))
        {
            _set_max_shutter_value_sync(ch, 0);
            _set_base_shutter_value_sync(ch, 0);
        }
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        _change_exposure_fixed(ch, 0);

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.dc_iris_motion_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.dc_iris_motion[i].value == g_ipcamData[ch].iris_control)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }
    }

    return FALSE;

}

gboolean _post_calibration_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static NFOBJECT *wait_mbox = NULL;
    NFOBJECT *top;
    gint ch;
    top = nfui_nfobject_get_top(obj);

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        for(ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if(_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) ==0 )
                break;
        }

        if( ch == GUI_CHANNEL_CNT ) return FALSE;

        if(scm_req_ipcam_dc_iris_calibration(ch) < 0)
		{
			vw_mbox(nfui_nfobject_get_top(obj), "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
			return FALSE;
		}

		if(!wait_mbox)
		    wait_mbox = nftool_mbox_wait_with_graph(top, "WAIT", "Please wait...", "");

    	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_FAIL);
    	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_SUCCESS);
    	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_TIMEOUT);
    }
    else if(evt->type == INFY_IPCAM_CALIBRATION_FAIL || evt->type == INFY_IPCAM_CALIBRATION_SUCCESS || evt->type == INFY_IPCAM_CALIBRATION_TIMEOUT)
    {
        guint ret;

        uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_FAIL);
        uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_SUCCESS);
        uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_TIMEOUT);

        ret = ((CMM_MESSAGE_T *)data)->param;

        if(wait_mbox)
        {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

        if(ret == -1)     // ERROR
    		vw_mbox(top, "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
        else if(ret == 3) // TIME OUT
            vw_mbox(top, "ERROR", IMBX_IPCAM_CALI_TIMEOUT, NFTOOL_MB_OK);
        else              // SUCESS
            nftool_mbox(top, "NOTICE", "SUCCESS", NFTOOL_MB_OK);
    }

    return FALSE;
}


gboolean _post_wide_dynamic_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    NFOBJECT *dnr_obj;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        g_ipcamData[ch].wdr_mode = g_ipcam_manage[ch].exposure.profile.wdr[index].value;

        for(i = 0; i < g_ipcam_manage[ch].exposure.profile.wdr_cnt; i++)
            g_ipcam_manage[ch].exposure.profile.wdr[i].selected = 0;

        g_ipcam_manage[ch].exposure.profile.wdr[index].selected = 1;

        dnr_obj = _get_nickname_object(&g_ipcam_subFixed[ch].exposureDnr, "DNR_VALUE_OBJ");
        if (dnr_obj)
        {
            nfui_combobox_remove_all((NFCOMBOBOX*)dnr_obj);

            if (index == 0)
            {
                // insert dnr : auto-smart
                for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt; i++)
                {
                    nfui_combobox_append_data((NFCOMBOBOX*)dnr_obj, g_ipcam_manage[ch].exposure.profile.dnr[i].caption);
                }
            }
            else
            {
                // remove dnr : auto-smart
                if (g_ipcamData[ch].dnr == NF_IPCAM_DNR_MODE_AUTO_SMART) {
                    g_ipcamData[ch].dnr = NF_IPCAM_DNR_MODE_AUTO_MID;
                }

                for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt; i++)
                {
                    if (g_ipcam_manage[ch].exposure.profile.dnr[i].value == NF_IPCAM_DNR_MODE_AUTO_SMART) continue;

                    nfui_combobox_append_data((NFCOMBOBOX*)dnr_obj, g_ipcam_manage[ch].exposure.profile.dnr[i].caption);
                }
            }

            for(i = 0; i < g_ipcam_manage[ch].exposure.profile.dnr_cnt; i++)
            {
                if (g_ipcam_manage[ch].exposure.profile.dnr[i].value == g_ipcamData[ch].dnr)
                {
                    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)dnr_obj, g_ipcam_manage[ch].exposure.profile.dnr[i].caption);
                }
            }

            nfui_signal_emit(dnr_obj, GDK_EXPOSE, TRUE);
        }

        _set_ipcam_data(ch);

        if(!strcmp(nfui_combobox_get_value(obj),"ON"))
        {
            _set_blc_control_value_sync(ch, "OFF");
            _set_slow_shutter_value_sync(ch, "OFF");
            _set_defog_value_sync(ch, "OFF");
            _set_antiflicker_value_sync(ch, "OFF");
        }

        is_changed = _change_exposure_fixed(ch, 1);
    	if (is_changed) nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        for (i = 0; i < g_ipcam_manage[ch].exposure.profile.wdr_cnt; i++)
        {
            if (g_ipcam_manage[ch].exposure.profile.wdr[i].value == g_ipcamData[ch].wdr_mode)
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, i);
        }

    }

    return FALSE;
}




////////////////////////////////////////////////////////////
//
// slider, spin sync handler
//

gboolean _post_brightness_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].bright == val) return FALSE;

        g_ipcamData[ch].bright = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
            cw_slider_set_value(obj, g_ipcamData[ch].bright);
    }

    return FALSE;
}

gboolean _post_brightness_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
        g_ipcamData[ch].bright = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_BRIGHTNESS)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].bright-g_ipcam_manage[ch].image.profile.brightness.min);
    }

    return FALSE;
}

gboolean _post_contrast_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].contrast == val) return FALSE;

        g_ipcamData[ch].contrast = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
            cw_slider_set_value(obj, g_ipcamData[ch].contrast);
    }

    return FALSE;
}

gboolean _post_contrast_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
        g_ipcamData[ch].contrast = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_CONTRAST)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].contrast-g_ipcam_manage[ch].image.profile.contrast.min);
    }

    return FALSE;
}

gboolean _post_tint_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].tint == val) return FALSE;

        g_ipcamData[ch].tint = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_TINT)
            cw_slider_set_value(obj, g_ipcamData[ch].tint);
    }

    return FALSE;
}

gboolean _post_tint_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;
    COL_INFO_T *col_info;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
        g_ipcamData[ch].tint = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_TINT)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].tint-g_ipcam_manage[ch].image.profile.tint.min);
    }

    return FALSE;
}

gboolean _post_color_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].color == val) return FALSE;

        g_ipcamData[ch].color = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
            cw_slider_set_value(obj, g_ipcamData[ch].color);
    }

    return FALSE;
}

gboolean _post_color_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
        g_ipcamData[ch].color = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_COLOR)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].color-g_ipcam_manage[ch].image.profile.color.min);
    }

    return FALSE;
}

gboolean _post_sharpness_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].sharpness == val) return FALSE;

        g_ipcamData[ch].sharpness = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
            cw_slider_set_value(obj, g_ipcamData[ch].sharpness);
    }

    return FALSE;
}

gboolean _post_sharpness_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].imageSet, obj, val);
        g_ipcamData[ch].sharpness = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].imageSet, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_SHARPNESS)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].sharpness-g_ipcam_manage[ch].image.profile.sharpness.min);
    }

    return FALSE;
}

gboolean _post_focus_near_limit_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_near_limit == val) return FALSE;

        g_ipcamData[ch].focus_near_limit = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_near_limit);
    }


    return FALSE;
}

gboolean _post_focus_near_limit_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_near_limit = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_NEARLIMIT)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_near_limit-g_ipcam_manage[ch].image.profile.nearlimit.min);
    }

    return FALSE;
}

gboolean _post_focus_far_limit_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_far_limit == val) return FALSE;

        g_ipcamData[ch].focus_far_limit = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_far_limit);
    }

    return FALSE;
}

gboolean _post_focus_far_limit_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_far_limit = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_FARLIMIT)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_far_limit-g_ipcam_manage[ch].image.profile.farlimit.min);
    }

    return FALSE;
}

gboolean _post_focus_default_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_default_speed == val) return FALSE;

        g_ipcamData[ch].focus_default_speed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_default_speed);
    }

    return FALSE;
}

gboolean _post_focus_default_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_default_speed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_DEFAULTSPEED)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_default_speed-g_ipcam_manage[ch].image.profile.defaultspeed.min);
    }

    return FALSE;
}

gboolean _post_focus_absolute_position_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_abposition == val) return FALSE;

        g_ipcamData[ch].focus_abposition = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_abposition);
    }

    return FALSE;
}

gboolean _post_focus_absolute_position_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_abposition = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABPOSITION)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_abposition-g_ipcam_manage[ch].image.profile.abposition.min);
    }

    return FALSE;
}

gboolean _post_focus_absolute_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_abspeed == val) return FALSE;

        g_ipcamData[ch].focus_abspeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_abspeed);
    }

    return FALSE;
}

gboolean _post_focus_absolute_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_abspeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_ABSPEED)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_abspeed-g_ipcam_manage[ch].image.profile.abspeed.min);
    }

    return FALSE;
}

gboolean _post_focus_relative_distance_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_redistance == val) return FALSE;

        g_ipcamData[ch].focus_redistance = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE)
          cw_slider_set_value(obj, g_ipcamData[ch].focus_redistance);
    }

    return FALSE;
}

gboolean _post_focus_relative_distance_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_redistance = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_REDISTANCE)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_redistance-g_ipcam_manage[ch].image.profile.redistance.min);
    }

    return FALSE;
}

gboolean _post_focus_relative_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_respeed == val) return FALSE;

        g_ipcamData[ch].focus_respeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_respeed);
    }

    return FALSE;
}

gboolean _post_focus_relative_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_respeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_RESPEED)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_respeed-g_ipcam_manage[ch].image.profile.respeed.min);
    }

    return FALSE;
}

gboolean _post_focus_continuous_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].focus_cospeed == val) return FALSE;

        g_ipcamData[ch].focus_cospeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED)
            cw_slider_set_value(obj, g_ipcamData[ch].focus_cospeed);
    }

    return FALSE;
}

gboolean _post_focus_continuous_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].focusMode, obj, val);
        g_ipcamData[ch].focus_cospeed = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_FOCUS_COSPEED)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].focus_cospeed-g_ipcam_manage[ch].image.profile.cospeed.min);
    }

    return FALSE;
}

gboolean _post_cr_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].wbMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].wb_crgain == val) return FALSE;

        g_ipcamData[ch].wb_crgain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
            cw_slider_set_value(obj, g_ipcamData[ch].wb_crgain);
    }

    return FALSE;
}

gboolean _post_cr_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].wbMode, obj, val);
        g_ipcamData[ch].wb_crgain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CRGAIN)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].wb_crgain-g_ipcam_manage[ch].image.profile.crgain.min);
    }

    return FALSE;
}

gboolean _post_cb_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].wbMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].wb_cbgain == val) return FALSE;

        g_ipcamData[ch].wb_cbgain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
            cw_slider_set_value(obj, g_ipcamData[ch].wb_cbgain);
    }

    return FALSE;
}

gboolean _post_cb_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].wbMode, obj, val);
        g_ipcamData[ch].wb_cbgain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].wbMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].image.profile.supported_image & NF_IPCAM_IMAGE_ONVIF_WB_CBGAIN)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].wb_cbgain-g_ipcam_manage[ch].image.profile.cbgain.min);
    }

    return FALSE;
}

gboolean _post_wide_dynamic_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureWdr, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].wdr_level == val) return FALSE;

        g_ipcamData[ch].wdr_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
            cw_slider_set_value(obj, g_ipcamData[ch].wdr_level);
    }

    return FALSE;
}

gboolean _post_wide_dynamic_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    COL_INFO_T *col_info;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureWdr, obj, val);
        g_ipcamData[ch].wdr_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureWdr, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_WDR_LEVEL)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].wdr_level-g_ipcam_manage[ch].exposure.profile.wdrlevel.min);
    }

    return FALSE;
}

gboolean _post_blc_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureBlc, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].blc_level == val) return FALSE;

        g_ipcamData[ch].blc_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
            cw_slider_set_value(obj, g_ipcamData[ch].blc_level);
    }

    return FALSE;
}

gboolean _post_blc_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    COL_INFO_T *col_info;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureBlc, obj, val);
        g_ipcamData[ch].blc_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureBlc, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_BLC_LEVEL)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].blc_level-g_ipcam_manage[ch].exposure.profile.blclevel.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_time_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].min_etime == val) return FALSE;

        g_ipcamData[ch].min_etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
            cw_slider_set_value(obj, g_ipcamData[ch].min_etime);
    }

    return FALSE;
}

gboolean _post_auto_exposure_time_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
        g_ipcamData[ch].min_etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINETIME)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].min_etime-g_ipcam_manage[ch].exposure.profile.minetime.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_time_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

         _sync_spin_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].max_etime == val) return FALSE;

        g_ipcamData[ch].max_etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
            cw_slider_set_value(obj, g_ipcamData[ch].max_etime);
    }

    return FALSE;
}

gboolean _post_auto_exposure_time_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
        g_ipcamData[ch].max_etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXETIME)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].max_etime-g_ipcam_manage[ch].exposure.profile.maxetime.min);
    }

    return FALSE;
}

gboolean _post_manual_exposure_time_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    COL_INFO_T *col_info;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].etime == val) return FALSE;

        g_ipcamData[ch].etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
            cw_slider_set_value(obj, g_ipcamData[ch].etime);
    }

    return FALSE;

}

gboolean _post_manual_exposure_time_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureTime, obj, val);
        g_ipcamData[ch].etime = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureTime, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_ETIME)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].etime-g_ipcam_manage[ch].exposure.profile.etime.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_gain_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

         _sync_spin_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].min_gain== val) return FALSE;

        g_ipcamData[ch].min_gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
            cw_slider_set_value(obj, g_ipcamData[ch].min_gain);
    }

    return FALSE;
}

gboolean _post_auto_exposure_gain_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
        g_ipcamData[ch].min_gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINGAIN)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].min_gain-g_ipcam_manage[ch].exposure.profile.mingain.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_gain_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].max_gain == val) return FALSE;

        g_ipcamData[ch].max_gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
            cw_slider_set_value(obj, g_ipcamData[ch].max_gain);
    }

    return FALSE;
}

gboolean _post_auto_exposure_gain_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
        g_ipcamData[ch].max_gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXGAIN)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].max_gain-g_ipcam_manage[ch].exposure.profile.maxgain.min);
    }

    return FALSE;
}

gboolean _post_manual_exposure_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

         _sync_spin_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].gain == val) return FALSE;

        g_ipcamData[ch].gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
            cw_slider_set_value(obj, g_ipcamData[ch].gain);
    }

    return FALSE;
}

gboolean _post_manual_exposure_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureGain, obj, val);
        g_ipcamData[ch].gain = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureGain, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_GAIN)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].gain-g_ipcam_manage[ch].exposure.profile.gain.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_iris_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].min_iris == val) return FALSE;

        g_ipcamData[ch].min_iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
            cw_slider_set_value(obj, g_ipcamData[ch].min_iris);
    }

    return FALSE;
}

gboolean _post_auto_exposure_iris_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
        g_ipcamData[ch].min_iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MINIRIS)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].min_iris-g_ipcam_manage[ch].exposure.profile.miniris.min);
    }

    return FALSE;
}

gboolean _post_auto_exposure_iris_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].max_iris == val) return FALSE;

        g_ipcamData[ch].max_iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
            cw_slider_set_value(obj, g_ipcamData[ch].max_iris);
    }

    return FALSE;
}

gboolean _post_auto_exposure_iris_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
        g_ipcamData[ch].max_iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_MAXIRIS)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].max_iris-g_ipcam_manage[ch].exposure.profile.maxiris.min);
    }

    return FALSE;
}

gboolean _post_manual_exposure_iris_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].iris == val) return FALSE;

        g_ipcamData[ch].iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
            cw_slider_set_value(obj, g_ipcamData[ch].iris);
    }

    return FALSE;
}

gboolean _post_manual_exposure_iris_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        _sync_slider_object(&g_ipcam_subFixed[ch].exposureIris, obj, val);
        g_ipcamData[ch].iris = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].exposureIris, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_IRIS)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].iris-g_ipcam_manage[ch].exposure.profile.iris.min);
    }

    return FALSE;
}

gboolean _post_dnn_sense_dton_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        _sync_spin_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].dnn_sense_dton == val) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
        {
            if (!_check_diff_val_of_dnn_sense_dton(ch, &val))
            {
                if(!tid) tid = g_timeout_add(20, _dnn_sense_conf_notice_popup, NULL);
                else return FALSE;
                cw_slider_set_value(obj, val);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
        }
        _sync_spin_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
        g_ipcamData[ch].dnn_sense_dton = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON)
            cw_slider_set_value(obj, g_ipcamData[ch].dnn_sense_dton);
    }
    return FALSE;
}

gboolean _post_dnn_sense_dton_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    gchar strBuf[8];

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }
        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        if(g_ipcamData[ch].dnn_sense_dton == val) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
        {
            if(!_check_diff_val_of_dnn_sense_dton(ch, &val))
            {
                if(!tid) tid = g_timeout_add(1, _dnn_sense_conf_notice_popup, NULL);
                else return FALSE;
                memset(strBuf, 0x00, sizeof(strBuf));
                g_sprintf(strBuf, "%d", val);
                nfui_spin_button_set_text((NFSPINBUTTON*)obj, strBuf);
            }
        }
        _sync_slider_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
        g_ipcamData[ch].dnn_sense_dton = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_DTON)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].dnn_sense_dton-g_ipcam_manage[ch].exposure.profile.dnn_sense_dton.min);
    }

    return FALSE;
}

gboolean _post_dnn_sense_ntod_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

         _sync_spin_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].dnn_sense_ntod == val) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
        {
            if (!_check_diff_val_of_dnn_sense_ntod(ch, &val))
            {
                if(!tid) tid = g_timeout_add(20, _dnn_sense_conf_notice_popup, NULL);
                else return FALSE;
                cw_slider_set_value(obj, val);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
        }
        _sync_spin_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
        g_ipcamData[ch].dnn_sense_ntod = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
        {
            cw_slider_set_value(obj, g_ipcamData[ch].dnn_sense_ntod);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

gboolean _post_dnn_sense_ntod_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    gchar strBuf[8];

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        if(g_ipcamData[ch].dnn_sense_ntod == val) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
        {
            if(!_check_diff_val_of_dnn_sense_ntod(ch, &val))
            {
                if(!tid) tid = g_timeout_add(1, _dnn_sense_conf_notice_popup, NULL);
                else return FALSE;
                memset(strBuf, 0x00, sizeof(strBuf));
                g_sprintf(strBuf, "%d", val);
                nfui_spin_button_set_text((NFSPINBUTTON*)obj, strBuf);
            }
        }
        _sync_slider_object(&g_ipcam_subFixed[ch].dnMode, obj, val);
        g_ipcamData[ch].dnn_sense_ntod = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_exposure & NF_IPCAM_EXPOSURE_ONVIF_DNN_SENSE_NTOD)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, g_ipcamData[ch].dnn_sense_ntod-g_ipcam_manage[ch].exposure.profile.dnn_sense_ntod.min);
    }

    return FALSE;
}

gboolean _post_illumination_ctrl_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, index;
    gint is_changed;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        g_message("%s(%d)", __FUNCTION__, __LINE__);
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        if(index == 0) g_ipcamData[ch].illumination_control = NF_IPCAM_COLORVU_CTRL_MANUAL;
        else g_ipcamData[ch].illumination_control = NF_IPCAM_COLORVU_CTRL_AUTO;

        if(index == 0)
        {
            nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj);
            nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj);
        }
        else
        {
            nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj);
            nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj);
        }

        nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj, GDK_EXPOSE, TRUE);
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        g_message("%s(%d)", __FUNCTION__, __LINE__);
        //g_message("%s(%d) => %d %d", __FUNCTION__, __LINE__, nfui_combobox_get_cur_index((NFCOMBOBOX*)obj), g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value);
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        _change_exposure_fixed(ch, 0);

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_MANUAL)
                nfui_combobox_set_index_no_expose(obj, 0);
            else if(g_ipcam_manage[ch].exposure.profile.colorvu_ctrl.value == NF_IPCAM_COLORVU_CTRL_AUTO)
                nfui_combobox_set_index_no_expose(obj, 1);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if(index == 0)
        {
            nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj);
            nfui_nfobject_disable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj);
        }
        else
        {
            nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj);
            nfui_nfobject_enable(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj);
        }

        nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[0]->obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[3]->col_info[1]->obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;

}

gboolean _post_illumination_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    gchar strBuf[8];

    if (evt->type == NFEVENT_CWSLIDER_CHANGED_RELEASE)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", val);
        nfui_spin_button_set_text((NFSPINBUTTON*)g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[2]->obj, strBuf);
    }
    else if ((evt->type == GDK_LEAVE_NOTIFY) || (evt->type == GDK_BUTTON_RELEASE))
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = cw_slider_get_value(obj);

        if (g_ipcamData[ch].illumination_level == val) return FALSE;

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", val);
        g_ipcamData[ch].illumination_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
        {
            g_message("%s(%d) => %d %d", __FUNCTION__, __LINE__, g_ipcam_manage[ch].exposure.profile.colorvu_level.value, g_ipcamData[ch].illumination_level);
            cw_slider_set_value(obj,g_ipcamData[ch].illumination_level);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

gboolean _post_illumination_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, ch, val;
    gchar strBuf[8];

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        val = nfui_spin_button_get_value((NFSPINBUTTON*) obj);

        if(g_ipcamData[ch].illumination_level == val) return FALSE;

        cw_slider_set_value(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, val);
        nfui_signal_emit(g_ipcam_subFixed[ch].dnMode.row_info[7]->col_info[1]->obj, GDK_EXPOSE, TRUE);
        g_ipcamData[ch].illumination_level = val;
        _set_ipcam_data(ch);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        g_message("%s(%d)", __FUNCTION__, __LINE__);
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT) return FALSE;

        if (g_ipcam_manage[ch].exposure.profile.supported_colorvu)
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj,  g_ipcamData[ch].illumination_level - g_ipcam_manage[ch].exposure.profile.colorvu_level.min);
    }

    return FALSE;
}

gboolean _post_ddn_schedule_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    NFOBJECT *top;

    gint i;
    gint start_hour, start_min, end_hour, end_min;
    gint x, y;
    gint index;
    gchar str[64];

    memset(str, 0x00, sizeof(str));

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
            break;
    }

    if (ch == GUI_CHANNEL_CNT) return FALSE;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += (obj->width/2) - 240;
        y += (obj->height/2) - 140;

        VW_Set_DateTime_table_Open(top, "DAY MODE", x, y, ch);

        _set_ipcam_data(ch);

        g_sprintf(str,"%02d : %02d  ~  %02d : %02d",g_ipcamData[ch].dnn_start_hour, g_ipcamData[ch].dnn_start_min, g_ipcamData[ch].dnn_end_hour, g_ipcamData[ch].dnn_end_min);

        nfui_nflabel_set_text((NFLABEL*) obj, str);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == NFEVENT_IPCAMSETUP_DB_SYNC)
    {
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (_check_matched_object(&g_ipcam_subFixed[ch].dnMode, obj) == 0)
                break;
        }

        if (ch == GUI_CHANNEL_CNT)  return FALSE;

        g_sprintf(str,"%02d : %02d  ~  %02d : %02d",g_org_ipcamData[ch].dnn_start_hour, g_org_ipcamData[ch].dnn_start_min, g_org_ipcamData[ch].dnn_end_hour, g_org_ipcamData[ch].dnn_end_min);

        nfui_nflabel_set_text((NFLABEL*) obj, str);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

////////////////////////////////////////////////////////////
//
// button handler
//

gboolean _post_one_push_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;

	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE:
		{
			if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
            {
                if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                    break;
            }

            if (ch == GUI_CHANNEL_CNT) return FALSE;

			if(scm_req_ipcam_onepush(ch) < 0)
			{
				vw_mbox(nfui_nfobject_get_top(obj), "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
				return FALSE;
			}
		}
		break;

		case GDK_DELETE:
		break;

		default:
		break;
	}

	return FALSE;
}

gboolean _post_home_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;

	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE:
		{
			if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
            {
                if (_check_matched_object(&g_ipcam_subFixed[ch].focusMode, obj) == 0)
                    break;
            }

            if (ch == GUI_CHANNEL_CNT) return FALSE;

			if(scm_req_ipcam_calibration(ch) < 0)
			{
				vw_mbox(nfui_nfobject_get_top(obj), "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
				return FALSE;
			}
		}
		break;

		case GDK_DELETE:
		break;

		default:
		break;
	}

    return FALSE;
}

gboolean _post_continuous_near_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		scm_run_ptz_cmd_focus_near();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY)
	{
		scm_stop_ptz_cmd();
	}

	return FALSE;
}

gboolean _post_continuous_far_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		scm_run_ptz_cmd_focus_far();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY)
	{
		scm_stop_ptz_cmd();
	}

	return FALSE;
}

gboolean _change_image_notify(gint ch)
{
    _change_image_fixed(ch, 1);

    return FALSE;
}

gboolean _change_exposure_notify(gint ch)
{
    _change_exposure_fixed(ch, 1);

    return FALSE;
}
