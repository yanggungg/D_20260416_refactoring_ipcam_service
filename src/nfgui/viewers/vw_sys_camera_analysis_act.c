/*
 * vw_sys_camera_analysis_act.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "scm.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"
#include "vaa.h"
#include "dvaa.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nfimglabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_menu.h"
#include "vw_sys_camera_analysis_act.h"
#include "vw_sys_camera_analysis_prop.h"
#include "vw_sys_camera_analysis_schd.h"

#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"
#include "nf_ipcam_defs.h"

#include "nf_sysman.h"

#define PAGE_FIXED_CNT 2
#define ROW_CNT_PER_PAGE (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define TABLE_LEFT (4)
#define TABLE_TOP (20)

#define LABEL_HEIGHT (40)

#define MAX_DVABOX_CNT (10)

#if defined(VIDECON)
#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0824P4E) || \
    defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
#define MAX_BUILTIN_ACT_CNT (4)
#else
#define MAX_BUILTIN_ACT_CNT (2)
#endif
#else
#define MAX_BUILTIN_ACT_CNT (2)
#endif

#define STR_LICENSE_SETTING "Please verify the license to use the AI engine.\nIf the AI engine is disabled, please register your license and continue setting up."
#define STR_AIBOX_SCAN "You can use Scan to search the AI Box connected to your local network.\nThe MAC address of the detected device will be displayed. Select the AI Box you want to connect."

#define STR_DISABLE_FISHEYE "You can not use the features such as the AI detection and the Fisheye Dewarping at the same time.\nThe configured Fisheye Dewarping will be automatically relieved in order to activate the feature of the AI detection.\nDo you want to continue?"
#define STR_OVER_ACT_CHANNEL "The AI detection function supports up to %d cameras only."

static AiAnalysisActData g_analysis_data[GUI_CHANNEL_CNT];
static AiAnalysisActData g_org_analysis_data[GUI_CHANNEL_CNT];

static gint g_dvabox_cnt = 0;
static aibox_connection_info g_dvabox_scan_info[MAX_DVABOX_CNT] = {
    0,
};
static ai_connection_info g_dvabox_act_info[GUI_CHANNEL_CNT];

static guint g_aibox_license = 0;
static guint g_aicam_license = 0;
static guint g_builtin_license = 0;
static guint g_classic_license = 0;

static guint g_aibox_capable = 0;
static guint g_aicam_capable = 0;
static guint g_builtin_capable = 0;
static guint g_classic_capable = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static NFOBJECT *g_title_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_none_radio[GUI_CHANNEL_CNT];
static NFOBJECT *g_aibox_radio[GUI_CHANNEL_CNT];
static NFOBJECT *g_aibox_label[GUI_CHANNEL_CNT];
static NFOBJECT *g_aibox_combo[GUI_CHANNEL_CNT];
static NFOBJECT *g_aicam_radio[GUI_CHANNEL_CNT];
static NFOBJECT *g_builtin_radio[GUI_CHANNEL_CNT];
static NFOBJECT *g_classic_radio[GUI_CHANNEL_CNT];

static NFOBJECT *g_radio_wait_pop = NULL;

////////////////////////////////////////////////////////////
//
// private data types
//

////////////////////////////////////////////////////////////
//
// private variable
//

static void _init_db_data()
{
    gint i;

    memset(g_analysis_data, 0x00, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);
    memset(g_org_analysis_data, 0x00, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_aianalysis_act_data(&g_analysis_data[i], i);
    }
    g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);
}

static void _init_license_data()
{
    gint i;

    g_aibox_license = 0;
    g_aicam_license = 0;
    g_builtin_license = 0;
    g_classic_license = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (scm_license_is_activated_aibox())
            g_aibox_license |= (1 << i);
        if (scm_license_is_activated_aicam(i))
            g_aicam_license |= (1 << i);
        if (scm_license_is_activated_dlva())
            g_builtin_license |= (1 << i);
        if (scm_license_is_activated_dmva(i))
            g_classic_license |= (1 << i);
    }
}

static void _init_capable_data()
{
    gint i;
    NF_NOTIFY_INFO vloss_info;

    g_aibox_capable = 0;
    g_aicam_capable = 0;
    g_builtin_capable = 0;
    g_classic_capable = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
        scm_get_vloss_data(&vloss_info);

        if (vloss_info.d.params[0] & (1 << i))
        {
            g_aibox_capable &= ~(1 << i);
            g_aicam_capable &= ~(1 << i);
            g_builtin_capable &= ~(1 << i);
            g_classic_capable &= ~(1 << i);
        }
        else
        {
            if (g_analysis_data[i].dvabox_ipaddr) g_aibox_capable |= (1 << i);
            if (scm_get_ipcam_ai_type(i) != CAM_AI_TYPE_NORMAL) g_aicam_capable |= (1 << i);
                g_builtin_capable |= (1 << i);
            if (scm_get_ipcam_vca_supp(i) == 0)
                g_classic_capable |= (1 << i);
        }
    }
}

static gint _is_analysis_enabled()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (g_analysis_data[i].dvabox_active)
            return 1;
        if (g_analysis_data[i].dvacam_active)
            return 1;
        if (g_analysis_data[i].builtin_active)
            return 1;
        if (g_analysis_data[i].classic_active)
            return 1;
    }

    return 0;
}

static gint _is_enable_fisheye_dewarping()
{
    CamItxFisheyeData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(CamItxFisheyeData));
        DAL_get_camera_itx_fisheye_data(&data, i);
        if (data.act)
            return 1;
    }

    return 0;
}

static guint _get_aibox_addr_idx(gint idx)
{
    if (idx >= g_dvabox_cnt)
        return 0;

    //    return g_dvabox_scan_info[idx].linklocal_ip;
    return nf_api_aibox_connection_info_get_ip(&g_dvabox_scan_info[idx]);
}

static gint _scan_local_network_aibox(gint timeout)
{
    aibox_search_list *aibox_list;
    gint dvabox_cnt;
    gint i;
    NF_NOTIFY_INFO vloss_info;

    aibox_connection_info *scan_dvabox_info;
    ai_connection_info *connect_dvabox_info;

    g_dvabox_cnt = 0;
    memset(g_dvabox_scan_info, 0x00, sizeof(aibox_connection_info) * MAX_DVABOX_CNT);
    memset(g_dvabox_act_info, 0x00, sizeof(ai_connection_info) * GUI_CHANNEL_CNT);

    aibox_list = nf_api_aibox_search_list(timeout);
    dvabox_cnt = nf_api_aibox_list_get_size(aibox_list);

    if (dvabox_cnt > MAX_DVABOX_CNT)
        dvabox_cnt = MAX_DVABOX_CNT;

    if (dvabox_cnt == 0)
    {
        g_aibox_capable = 0;
    }
    else
    {
        memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
        scm_get_vloss_data(&vloss_info);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (vloss_info.d.params[0] & (1 << i))
            {
                g_aibox_capable &= ~(1 << i);
            }
            else
            {
                g_aibox_capable |= (1 << i);
            }
        }

        for (i = 0; i < dvabox_cnt; i++)
        {
            scan_dvabox_info = nf_api_aibox_list_get_data(aibox_list, i);
            memcpy(&g_dvabox_scan_info[i], scan_dvabox_info, sizeof(aibox_connection_info));
        }

        connect_dvabox_info = nf_api_get_ai_connection_info(aibox_list);
        if (connect_dvabox_info)
        {
            memcpy(g_dvabox_act_info, connect_dvabox_info, sizeof(ai_connection_info) * GUI_CHANNEL_CNT);
            free(connect_dvabox_info);
        }
    }

    g_dvabox_cnt = dvabox_cnt;
    nf_api_aibox_list_free(aibox_list);
    return 0;
}

/*
static gint _get_changed_dvabox_new_stream(guint *new_owner_chmask, guint *new_stream_chmask)
{
    gchar nvr_owner[32];
    gchar aibox_owner[32];
    gint i;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if ((g_org_analysis_data[i].dvabox_active == 0) && (g_analysis_data[i].dvabox_active == 1))
        {
            if (nf_api_aibox_connection_is_alive(i, g_analysis_data[i].dvabox_ipaddr) == 0)
            {
                memset(nvr_owner, 0x00, sizeof(nvr_owner));
                memset(aibox_owner, 0x00, sizeof(aibox_owner));
                nf_api_get_nvr_owner(nvr_owner);
                nf_api_get_aibox_owner(g_analysis_data[i].dvabox_ipaddr, aibox_owner);
                g_message("%s, %d, i:%d, ipaddr:%u, %s, %s", __FUNCTION__, __LINE__, i, g_analysis_data[i].dvabox_ipaddr, nvr_owner, aibox_owner);

                if (strcmp(nvr_owner, aibox_owner) != 0) {
                    *new_owner_chmask |= 1 << i;
                }

                *new_stream_chmask |= 1 << i;
            }
        }
    }

    return 0;
}

static gint _connect_dvabox_new_owner(guint new_owner_chmask)
{
    guint new_owner_ipaddr[MAX_DVABOX_CNT] = {0, };
    gint new_owner_cnt = 0;
    gint is_new_owner;
    gint i, j;

    gint ret_code = DLVA_API_RET_OK;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (new_owner_chmask & (1 << i))
        {
            is_new_owner = 1;
            for (j = 0; j < new_owner_cnt; j++) {
                if (new_owner_ipaddr[j] == g_analysis_data[i].dvabox_ipaddr) is_new_owner = 0;
            }

            if (is_new_owner) {
                new_owner_ipaddr[new_owner_cnt] = g_analysis_data[i].dvabox_ipaddr;
                new_owner_cnt++;
            }
        }
    }

    for (i = 0; i < new_owner_cnt; i++)
    {
        ret_code = nf_api_set_aibox_owner(new_owner_ipaddr[i]);
        g_message(">>>> %s, %d, new_owner_ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, new_owner_ipaddr[i], ret_code);
    }

    return 0;
}

static gint _connect_dvabox_new_stream(guint new_stream_chmask)
{
    gint i;
    gint ret_code = DLVA_API_RET_OK;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (new_stream_chmask & (1 << i)) {
            ret_code = nf_api_aibox_set_video_stream(i, g_analysis_data[i].dvabox_ipaddr, NULL);
            g_message(">>>> new_stream %s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, i, g_analysis_data[i].dvabox_ipaddr, ret_code);
        }
    }

    return 0;
}

static gint _disconnect_dvabox_channel()
{
    gint i;
    gint ret_code = DLVA_API_RET_OK;

    g_message("%s, %d called", __FUNCTION__, __LINE__);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if ((g_analysis_data[i].dvabox_active == 0) && (g_org_analysis_data[i].dvabox_active != g_analysis_data[i].dvabox_active))
        {
            ret_code = nf_api_aibox_delete_video_stream(i, g_org_analysis_data[i].dvabox_ipaddr);
            g_message(">>>> disconnect %s, %d, ch:%d, ipaddr:%u, ret_code:%d", __FUNCTION__, __LINE__, i, g_analysis_data[i].dvabox_ipaddr, ret_code);
        }
    }

    return 0;
}
*/

static gint _disable_fisheye_dewarping()
{
    CamItxFisheyeData data;
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(&data, 0x00, sizeof(CamItxFisheyeData));
        DAL_get_camera_itx_fisheye_data(&data, i);
        if (data.act == 1)
        {
            data.act = 0;
            DAL_set_camera_itx_fisheye_data(data, i);
        }
    }
    syscam_set_changeflag(1);
    return 0;
}

static gint _check_over_builtin_act_max()
{
    gint i;
    gint cnt = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (nfui_radio_button_get_toggled((NFBUTTON *)g_builtin_radio[i]))
        {
            cnt++;
        }
    }

    if (cnt > MAX_BUILTIN_ACT_CNT)
        return 1;

    return 0;
}

static gint _set_analysis_dvabox_active()
{
    if (ivsc.dfunc.support_protect == 1)
    {
        if (DAL_get_agr_policy() == 0)
        {
            vw_provide_devinfo_notice2_open(g_curwnd);

            if (DAL_get_agr_policy() == 0)
                return 0;
        }
    }
    return 1;
}

static gint _set_analysis_dvacam_active()
{

    return 1;
}

static gint _set_analysis_builtin_active()
{
    gint ret;
    gchar strBuf[1024];

    if (_is_enable_fisheye_dewarping() == 1)
    {
        ret = nftool_mbox(g_curwnd, "CONFIRM", STR_DISABLE_FISHEYE, NFTOOL_MB_OKCANCEL);

        if (ret == NFTOOL_MB_OK)
        {
            _disable_fisheye_dewarping();
        }
        else if (ret == NFTOOL_MB_CANCEL)
        {
            return 0;
        }
    }

    if (_check_over_builtin_act_max())
    {
        snprintf(strBuf, sizeof(strBuf), lookup_string(STR_OVER_ACT_CHANNEL), MAX_BUILTIN_ACT_CNT);
        nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OK);
        return 0;
    }

    return 1;
}

static gint _set_analysis_classic_active()
{

    return 1;
}

static gint _apply_analysis_engine_active()
{
    gint i;
    VAAID vaaid;
    DVAAID dvaaid;
    guint vaa_changed = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        dvaaid = dvaa_get_dvaaid(i);
        dvaa_itx_disable(dvaaid);
        vaaid = vaa_get_vaaid(i);
        vaa_itx_disable(vaaid);

        if ((g_analysis_data[i].dvabox_active) || (g_analysis_data[i].dvacam_active))
        {
            dvaaid = dvaa_get_dvaaid(i);
            dvaa_itx_enable(dvaaid);
        }
        else if (g_analysis_data[i].builtin_active)
        {
            scm_set_gpu_mode_function(GPU_DLVA);
        }
        else if (g_analysis_data[i].classic_active)
        {
            vaaid = vaa_get_vaaid(i);
            vaa_itx_enable(vaaid);
        }

        dvaaid = dvaa_get_dvaaid(i);
        if (dvaa_itx_is_db_changed(dvaaid))
        {
            dvaa_itx_save_db(dvaaid);
        }

        vaaid = vaa_get_vaaid(i);
        if (vaa_itx_is_db_changed(vaaid))
        {
            vaa_itx_save_db(vaaid);
            vaa_changed |= (1 << i);
        }
    }

    if (vaa_changed)
    {
        DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, vaa_changed);
    }

    scm_put_log(CHANGE_CAM_VCA, 0, 0);
    return 1;
}

static void _prvSetDataToObjects()
{
    gint i;

    g_aibox_capable = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (g_analysis_data[i].dvabox_ipaddr > 0)
        {
            g_aibox_capable |= (1 << i);
            nfui_nflabel_set_text((NFLABEL *)g_aibox_label[i], g_analysis_data[i].dvabox_mac);
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL *)g_aibox_label[i], "-");
        }

        nfui_nfobject_show(g_aibox_label[i]);
        nfui_nfobject_hide(g_aibox_combo[i]);

        if ((g_aibox_license & (1 << i)) && (g_aibox_capable & (1 << i)))
            nfui_nfobject_enable(g_aibox_radio[i]);
        else
            nfui_nfobject_disable(g_aibox_radio[i]);

        if ((g_aicam_license & (1 << i)) && (g_aicam_capable & (1 << i)))
            nfui_nfobject_enable(g_aicam_radio[i]);
        else
            nfui_nfobject_disable(g_aicam_radio[i]);

        if ((g_builtin_license & (1 << i)) && (g_builtin_capable & (1 << i)))
            nfui_nfobject_enable(g_builtin_radio[i]);
        else
            nfui_nfobject_disable(g_builtin_radio[i]);

        if ((g_classic_license & (1 << i)) && (g_classic_capable & (1 << i)))
            nfui_nfobject_enable(g_classic_radio[i]);
        else
            nfui_nfobject_disable(g_classic_radio[i]);

        if (g_analysis_data[i].dvabox_active)
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_aibox_radio[i], TRUE);
        }
        else if (g_analysis_data[i].dvacam_active)
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_aicam_radio[i], TRUE);
        }
        else if (g_analysis_data[i].builtin_active)
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_builtin_radio[i], TRUE);
        }
        else if (g_analysis_data[i].classic_active)
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_classic_radio[i], TRUE);
        }
        else
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_none_radio[i], TRUE);
        }
    }
}

static void _prvSetDataToObjects_aibox()
{
    gint i;

    g_aibox_capable = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (g_analysis_data[i].dvabox_ipaddr > 0)
        {
            g_aibox_capable |= (1 << i);
            nfui_nflabel_set_text((NFLABEL *)g_aibox_label[i], g_analysis_data[i].dvabox_mac);
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL *)g_aibox_label[i], "-");
        }

        nfui_nfobject_show(g_aibox_label[i]);
        nfui_nfobject_hide(g_aibox_combo[i]);

        if ((g_aibox_license & (1 << i)) && (g_aibox_capable & (1 << i)))
            nfui_nfobject_enable(g_aibox_radio[i]);
        else
            nfui_nfobject_disable(g_aibox_radio[i]);

        if (g_analysis_data[i].dvabox_active)
        {
            nfui_radio_button_set_toggled((NFBUTTON *)g_aibox_radio[i], TRUE);
        }
    }
}

static void _prvLoadDataFromObjects()
{
    gint i, idx;
    gchar *strBuf;
    gint mac[8];
    guint ipaddr;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        g_analysis_data[i].dvabox_active = nfui_radio_button_get_toggled((NFBUTTON *)g_aibox_radio[i]);
        g_analysis_data[i].dvacam_active = nfui_radio_button_get_toggled((NFBUTTON *)g_aicam_radio[i]);
        g_analysis_data[i].builtin_active = nfui_radio_button_get_toggled((NFBUTTON *)g_builtin_radio[i]);
        g_analysis_data[i].classic_active = nfui_radio_button_get_toggled((NFBUTTON *)g_classic_radio[i]);

        if (g_analysis_data[i].dvabox_active)
        {
            if (((NFCOMBOBOX *)g_aibox_combo[i])->data)
            {
                strBuf = nfui_combobox_get_value((NFCOMBOBOX *)g_aibox_combo[i]);
                memset(g_analysis_data[i].dvabox_mac, 0x00, sizeof(g_analysis_data[i].dvabox_mac));
                strcpy(g_analysis_data[i].dvabox_mac, strBuf);

                idx = nfui_combobox_get_cur_index((NFCOMBOBOX *)g_aibox_combo[i]);
                g_analysis_data[i].dvabox_ipaddr = _get_aibox_addr_idx(idx);
                if (g_analysis_data[i].dvabox_ipaddr == 0) {
                    g_analysis_data[i].dvabox_ipaddr = g_dvabox_scan_info[idx].ip;
                }
            }
        }
        else
        {
            g_analysis_data[i].dvabox_ipaddr = 0;
            memset(g_analysis_data[i].dvabox_mac, 0x00, sizeof(g_analysis_data[i].dvabox_mac));
        }
    }
}

////////////////////////////////////////////////////////////
//
// private interfaces
//

////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_analysis_engine_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    static NFOBJECT *lost_focus = NULL;

    if (evt->type == NFEVENT_RADIO_LOST_FOCUS)
    {
        lost_focus = obj;
    }
    else if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        gint i = 0, index;

        index = nfui_radio_button_get_index((NFBUTTON *)obj);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_none_radio[i] == obj)
                break;
            if (g_aibox_radio[i] == obj)
                break;
            if (g_aicam_radio[i] == obj)
                break;
            if (g_builtin_radio[i] == obj)
                break;
            if (g_classic_radio[i] == obj)
                break;
        }
        if (i >= GUI_CHANNEL_CNT)
            return FALSE;

        if (index == 1)
        {
            if (!_set_analysis_dvabox_active())
            {
                g_radio_wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait...");
                evt_send_to_local(INFY_WAIT_DRAW_EXPOSE, 0, 0, lost_focus);
                return FALSE;
            }
        }
        else if (index == 2)
        {
            if (!_set_analysis_dvacam_active())
            {
                g_radio_wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait.");
                evt_send_to_local(INFY_WAIT_DRAW_EXPOSE, 0, 0, lost_focus);
                return FALSE;
            }
        }
        else if (index == 3)
        {
            if (!_set_analysis_builtin_active())
            {
                g_radio_wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait.");
                evt_send_to_local(INFY_WAIT_DRAW_EXPOSE, 0, 0, lost_focus);
                return FALSE;
            }
        }
        else if (index == 4)
        {
            if (!_set_analysis_classic_active())
            {
                g_radio_wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait.");
                evt_send_to_local(INFY_WAIT_DRAW_EXPOSE, 0, 0, lost_focus);
                return FALSE;
            }
        }

        if (nfui_nfobject_is_shown(g_aibox_combo[i]))
        {
            if (index == 1)
                nfui_nfobject_enable(g_aibox_combo[i]);
            else
                nfui_nfobject_disable(g_aibox_combo[i]);
            nfui_signal_emit(g_aibox_combo[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_licensebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        VW_SetupSystem_Open(g_curwnd, 0, mcf.sys_sub6.menu_pos[SYS_SUB6_LICENSE]);
        vsm_live_stop();

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (scm_license_is_activated_aibox())
                g_aibox_license |= (1 << i);
            if (scm_license_is_activated_aicam(i))
                g_aicam_license |= (1 << i);
            if (scm_license_is_activated_dlva())
                g_builtin_license |= (1 << i);
            if (scm_license_is_activated_dmva(i))
                g_classic_license |= (1 << i);
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((g_aibox_license & (1 << i)) && (g_aibox_capable & (1 << i)) && (nfui_nfobject_is_disabled(g_aibox_radio[i])))
            {
                nfui_nfobject_enable(g_aibox_radio[i]);
                nfui_signal_emit(g_aibox_radio[i], GDK_EXPOSE, TRUE);
            }

            if ((g_aicam_license & (1 << i)) && (g_aicam_capable & (1 << i)) && (nfui_nfobject_is_disabled(g_aicam_radio[i])))
            {
                nfui_nfobject_enable(g_aicam_radio[i]);
                nfui_signal_emit(g_aicam_radio[i], GDK_EXPOSE, TRUE);
            }

            if ((g_builtin_license & (1 << i)) && (g_builtin_capable & (1 << i)) && (nfui_nfobject_is_disabled(g_builtin_radio[i])))
            {
                nfui_nfobject_enable(g_builtin_radio[i]);
                nfui_signal_emit(g_builtin_radio[i], GDK_EXPOSE, TRUE);
            }

            if ((g_classic_license & (1 << i)) && (g_classic_capable & (1 << i)) && (nfui_nfobject_is_disabled(g_classic_radio[i])))
            {
                nfui_nfobject_enable(g_classic_radio[i]);
                nfui_signal_emit(g_classic_radio[i], GDK_EXPOSE, TRUE);
            }
        }
    }

    return FALSE;
}

static gboolean post_scanbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *wait_pop;

        gint i, ch;
        gchar strBuf[32];

        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            nfui_combobox_remove_all((NFCOMBOBOX *)g_aibox_combo[ch]);
        }

        nftool_mbox_sleep_auto(g_curwnd, 1, "NOTICE", "AI Box that is connected to the same local network will be automatically searched and listed.\nPlease wait for a while.");
        wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "AI Box that is connected to the same local network will be automatically searched and listed.\nPlease wait for a while.");

        _scan_local_network_aibox(10);

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            g_message(">>>> %s, %d, ch:%d, aibox_ip:%u", __FUNCTION__, __LINE__, ch, g_dvabox_act_info[ch].aibox_ip);

            if ((g_aibox_license & (1 << ch)) && (g_aibox_capable & (1 << ch)))
            {
                nfui_nfobject_enable(g_aibox_radio[ch]);

                if (nfui_radio_button_get_toggled((NFBUTTON *)g_aibox_radio[ch]))
                {
                    nfui_radio_button_set_toggled((NFBUTTON *)g_aibox_radio[ch], TRUE);
                }
            }
            else
            {
                nfui_nfobject_disable(g_aibox_radio[ch]);

                if (nfui_radio_button_get_toggled((NFBUTTON *)g_aibox_radio[ch]))
                {
                    nfui_clear_key_focus(g_none_radio[ch]);
                    nfui_set_key_focus(g_none_radio[ch], TRUE);
                    nfui_radio_button_set_toggled((NFBUTTON *)g_none_radio[ch], TRUE);
                }
            }

            if (nfui_radio_button_get_toggled((NFBUTTON *)g_aibox_radio[ch]))
                nfui_nfobject_enable(g_aibox_combo[ch]);
            else
                nfui_nfobject_disable(g_aibox_combo[ch]);

            if (g_dvabox_cnt == 0)
            {
                nfui_nflabel_set_text((NFLABEL *)g_aibox_label[ch], "-");

                nfui_nfobject_show(g_aibox_label[ch]);
                nfui_nfobject_hide(g_aibox_combo[ch]);
            }
            else
            {
                for (i = 0; i < g_dvabox_cnt; i++)
                {
                    memset(strBuf, 0x00, sizeof(strBuf));
                    g_sprintf(strBuf, "%02x:%02x:%02x:%02x:%02x:%02x",
                              g_dvabox_scan_info[i].mac[0], g_dvabox_scan_info[i].mac[1], g_dvabox_scan_info[i].mac[2],
                              g_dvabox_scan_info[i].mac[3], g_dvabox_scan_info[i].mac[4], g_dvabox_scan_info[i].mac[5]);
                    nfui_combobox_append_data((NFCOMBOBOX *)g_aibox_combo[ch], strBuf);

                    if (memcmp(g_dvabox_scan_info[i].mac, g_dvabox_act_info[ch].mac, sizeof(unsigned char) * 6) == 0)
                    {
                        nfui_combobox_set_data_no_expose((NFCOMBOBOX *)g_aibox_combo[ch], strBuf);
                    }
                }

                nfui_nfobject_hide(g_aibox_label[ch]);
                nfui_nfobject_show(g_aibox_combo[ch]);
            }
        }

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            nfui_signal_emit(g_none_radio[ch], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aibox_radio[ch], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aibox_label[ch], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aibox_combo[ch], GDK_EXPOSE, TRUE);
        }

        g_message("%s, %d, box_license:%08X, box_capable:%08X", __FUNCTION__, __LINE__, g_aibox_license, g_aibox_capable);

        nfui_make_key_hierarchy(g_curwnd);
        nftool_remove_waitbox(wait_pop);
    }

    return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == 0)
            return FALSE;

        nfui_on_backscr(obj);
        nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;

        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == (PAGE_FIXED_CNT - 1))
            return FALSE;

        nfui_on_backscr(obj);
        nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;

        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;

        g_memmove(g_analysis_data, g_org_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);
        _prvSetDataToObjects();

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_signal_emit(g_none_radio[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aibox_radio[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aicam_radio[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_builtin_radio[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_classic_radio[i], GDK_EXPOSE, TRUE);

            nfui_signal_emit(g_aibox_label[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_aibox_combo[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        guint new_owner_chmask = 0;
        guint new_stream_chmask = 0;

        mb_type ret;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        _prvLoadDataFromObjects();

        if (memcmp(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT))
        {
#if 0
            _get_changed_dvabox_new_stream(&new_owner_chmask, &new_stream_chmask);
            if (new_owner_chmask)
            {
                ret = nftool_mbox(g_curwnd, "CONFIRM", "If you change the setting of the AI Box from recorder, you need to initialize current setting from AI BOX.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
                if (ret == NFTOOL_MB_CANCEL) {
                    g_memmove(g_analysis_data, g_org_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
                    _prvSetDataToObjects();
                    for (i = 0; i < GUI_CHANNEL_CNT; i++)
                    {
                        nfui_signal_emit(g_none_radio[i], GDK_EXPOSE, TRUE);
                        nfui_signal_emit(g_aibox_radio[i], GDK_EXPOSE, TRUE);
                        nfui_signal_emit(g_aicam_radio[i], GDK_EXPOSE, TRUE);
                        nfui_signal_emit(g_builtin_radio[i], GDK_EXPOSE, TRUE);
                        nfui_signal_emit(g_classic_radio[i], GDK_EXPOSE, TRUE);

                        nfui_signal_emit(g_aibox_label[i], GDK_EXPOSE, TRUE);
                        nfui_signal_emit(g_aibox_combo[i], GDK_EXPOSE, TRUE);
                    }
                    return FALSE;
                }
                _connect_dvabox_new_owner(new_owner_chmask);
            }

            _connect_dvabox_new_stream(new_stream_chmask);
            _disconnect_dvabox_channel();
#endif
            _prvSetDataToObjects_aibox();

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                nfui_signal_emit(g_aibox_radio[i], GDK_EXPOSE, TRUE);
                nfui_signal_emit(g_aibox_label[i], GDK_EXPOSE, TRUE);
                nfui_signal_emit(g_aibox_combo[i], GDK_EXPOSE, TRUE);
            }

            DAL_set_aianalysis_act_data_all(g_analysis_data, GUI_CHANNEL_CNT);
            g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);

            _apply_analysis_engine_active();
            DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            syscam_set_changeflag(1);
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        VW_analysis_act_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch (evt->type)
    {
    case GDK_EXPOSE:
        break;

    case INFY_CAMDB_CHANGE_NOTIFY:
    case INFY_USRDB_CHANGE_NOTIFY:
    {
        gint i;
        gchar strBuf[STRING_SIZE_CAMTITLE];

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            var_get_camtitle(strBuf, i);
            nfui_nfimglabel_set_text((NFIMGLABEL *)g_title_obj[i], strBuf);
            nfui_signal_emit(g_title_obj[i], GDK_EXPOSE, TRUE);
        }
    }
    break;

    case INFY_WAIT_DRAW_EXPOSE:
    {
        NFOBJECT *lost_focus = ((CMM_MESSAGE_T *)data)->data;
        gint i = 0;

        if (g_radio_wait_pop)
        {
            nfui_nfobject_hide(g_radio_wait_pop);
            nftool_remove_waitbox(g_radio_wait_pop);
        }
        g_radio_wait_pop = 0;

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_none_radio[i] == lost_focus)
                break;
            if (g_aibox_radio[i] == lost_focus)
                break;
            if (g_aicam_radio[i] == lost_focus)
                break;
            if (g_builtin_radio[i] == lost_focus)
                break;
            if (g_classic_radio[i] == lost_focus)
                break;
        }
        if (i >= GUI_CHANNEL_CNT)
            return FALSE;

        nfui_clear_key_focus(lost_focus);
        nfui_set_key_focus(lost_focus, TRUE);
        nfui_radio_button_set_toggled((NFBUTTON *)lost_focus, TRUE);

        nfui_signal_emit(g_none_radio[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_radio[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aicam_radio[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_builtin_radio[i], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_classic_radio[i], GDK_EXPOSE, TRUE);

        if (nfui_nfobject_is_shown(g_aibox_combo[i]))
        {
            if (nfui_radio_button_get_toggled((NFBUTTON *)g_aibox_radio[i]))
                nfui_nfobject_enable(g_aibox_combo[i]);
            else
                nfui_nfobject_disable(g_aibox_combo[i]);
            nfui_signal_emit(g_aibox_combo[i], GDK_EXPOSE, TRUE);
        }
    }
    break;

    case GDK_DELETE:
    {
        uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_WAIT_DRAW_EXPOSE);
    }
    break;

    default:
        break;
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

////////////////////////////////////////////////////////////
//
// public interfaces
//

gint VW_analysis_act_init_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *tmp_fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GSList *slist = NULL;

    GdkPixbuf *pbCamImage[32];
    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    gchar strBuf[STRING_SIZE_CAMTITLE];
    gchar lfBuf[4096];

    guint width1[2] = {0, };
    guint width2[6] = {0, };
    guint width3[7] = {0, };
    gint gap = 0;
    gint pos_x, pos_y;
    guint size_w, size_h;
    gint i;
    gint page_num, row_num;
    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    pbCamImage[0] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL);
    pbCamImage[1] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL);
    pbCamImage[2] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL);
    pbCamImage[3] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL);
    pbCamImage[4] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);
    pbCamImage[5] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL);
    pbCamImage[6] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL);
    pbCamImage[7] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL);
    pbCamImage[8] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL);
    pbCamImage[9] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL);
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL);
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL);
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL);
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL);
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL);
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL);
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL);
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL);
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL);
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL);
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL);
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL);
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL);
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL);
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL);
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL);

    radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_INPUT_OFF), NULL);
    radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_INPUT_ON), NULL);
    radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_INPUT_ON), NULL);
    radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_INPUT_OFF), NULL);

    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
    prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
    prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

    next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
    next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
    next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    width3[0] = 240;
    width3[1] = 110;
    if (ivsc.dfunc.support_aibox_itx) {
        width3[2] = 50;
        width3[3] = 240;
    }
    if (ivsc.dfunc.support_aicam_itx) {
        width3[4] = 150;
    }
    if (ivsc.dfunc.support_dlva_itx) {
        width3[5] = 160;
    }
    if (ivsc.dfunc.support_dmva_itx) {
        width3[6] = 160;
    }

    width2[0] = width3[0];
    width2[1] = width3[1];
    width2[2] = width3[2] + width3[3] + 1;
    width2[3] = width3[4];
    width2[4] = width3[5];
    width2[5] = width3[6];

    width1[0] = width2[0];
    width1[1] = width2[1] + width2[2] + width2[3] + width2[4] + width2[5] + 3;

    g_curwnd = nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
    nfui_nffixed_put((NFFIXED *)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);

    uxm_reg_imsg_event(content_fixed, INFY_WAIT_DRAW_EXPOSE);

    pos_x = TABLE_LEFT;
    pos_y = TABLE_TOP;

    if (ivsc.dfunc.support_license)
    {
        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("LICENSE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 30);
        nfui_nfobject_set_size(obj, 360, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x, pos_y);

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(STR_LICENSE_SETTING), 300, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
        size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

        if (size_h > 200)
            size_h = 200;
            
        pos_y += 52;

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
        nfui_nfobject_set_size(obj, 300, size_h);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x + 30, pos_y);
        
        pos_y += size_h + 4;

        obj = (NFOBJECT *)nftool_normal_button_create_type3("LICENSE SETTING", 300);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        nfui_regi_post_event_callback(obj, post_licensebutton_event_handler);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x + 30, pos_y);
        
        pos_y = TABLE_TOP + 280;
    }
    
    if (ivsc.dfunc.support_aibox_itx)
    {
        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DEEP LEARNING PLUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 30);
        nfui_nfobject_set_size(obj, 360, 40);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x, pos_y);

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(STR_AIBOX_SCAN), 300, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
        size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

        if (size_h > 200)
            size_h = 200;
            
        pos_y += 52;

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
        nfui_nfobject_set_size(obj, 300, size_h);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x + 30, pos_y);
        
        pos_y += size_h + 4;

        obj = (NFOBJECT *)nftool_normal_button_create_type3("AI BOX SCAN", 300);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        nfui_regi_post_event_callback(obj, post_scanbutton_event_handler);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x + 30, pos_y);
    }

    if (ivsc.dfunc.support_license || ivsc.dfunc.support_aibox_itx)
        pos_x = TABLE_LEFT + 380;
    else
        pos_x = TABLE_LEFT;
        
    pos_y = TABLE_TOP;

    ntb = nfui_nftable_new(2, 1, 1, 1, width1, LABEL_HEIGHT);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED *)content_fixed, ntb, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("DEEP LEARNING ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, 1, 0);

    pos_y += (LABEL_HEIGHT + 1);

    ntb = nfui_nftable_new(6, 1, 1, 1, width2, LABEL_HEIGHT);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED *)content_fixed, ntb, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("NONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("AI BOX", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    if (ivsc.dfunc.support_aibox_itx) nfui_nftable_attach((NFTABLE *)ntb, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("AI CAM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    if (ivsc.dfunc.support_aicam_itx) nfui_nftable_attach((NFTABLE *)ntb, obj, 3, 0);

    obj = nfui_nflabel_new_with_pango_font("BUILT-IN AI", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    if (ivsc.dfunc.support_dlva_itx) nfui_nftable_attach((NFTABLE *)ntb, obj, 4, 0);

    obj = nfui_nflabel_new_with_pango_font("CLASSIC VA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    if (ivsc.dfunc.support_dmva_itx) nfui_nftable_attach((NFTABLE *)ntb, obj, 5, 0);

    pos_y += (LABEL_HEIGHT + 1);
    	
    size_w = 0;
    for (i = 0; i < 7; i++) {
        size_w += width3[i];
    }

    main_page_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED *)content_fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED *)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT *)nfui_nftable_new(7, ROW_CNT_PER_PAGE, 1, 1, width3, LABEL_HEIGHT);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED *)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_prev_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 100, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        var_get_camtitle(strBuf, i);

        obj = (NFOBJECT *)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL *)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL *)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 0, row_num);
        g_title_obj[i] = obj;

        tmp_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        nfui_nfobject_show(tmp_fixed);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], tmp_fixed, 1, row_num);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)tmp_fixed, obj, (width3[1] - size_w) / 2, (40 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_analysis_engine_event_handler);
        g_none_radio[i] = obj;

        slist = nfui_radio_button_get_group(NF_BUTTON(obj));

        tmp_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        nfui_nfobject_show(tmp_fixed);
        if (ivsc.dfunc.support_aibox_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], tmp_fixed, 2, row_num);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_aibox_itx) nfui_nffixed_put((NFFIXED *)tmp_fixed, obj, (width3[2] - size_w) / 2, (40 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_analysis_engine_event_handler);
        g_aibox_radio[i] = obj;

        nfui_radio_button_add_group(NF_BUTTON(obj), slist);

        obj = nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(921));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_aibox_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 3, row_num);
        g_aibox_label[i] = obj;

        obj = nfui_combobox_new(0, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
        nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        nfui_combobox_set_pango_font(NF_COMBOBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_disable(obj);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_aibox_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, 3, row_num);
        g_aibox_combo[i] = obj;

        tmp_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        nfui_nfobject_show(tmp_fixed);
        if (ivsc.dfunc.support_aicam_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], tmp_fixed, 4, row_num);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_aicam_itx) nfui_nffixed_put((NFFIXED *)tmp_fixed, obj, (width3[4] - size_w) / 2, (40 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_analysis_engine_event_handler);
        g_aicam_radio[i] = obj;

        nfui_radio_button_add_group(NF_BUTTON(obj), slist);

        tmp_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        nfui_nfobject_show(tmp_fixed);
        if (ivsc.dfunc.support_dlva_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], tmp_fixed, 5, row_num);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_dlva_itx) nfui_nffixed_put((NFFIXED *)tmp_fixed, obj, (width3[5] - size_w) / 2, (40 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_analysis_engine_event_handler);
        g_builtin_radio[i] = obj;

        nfui_radio_button_add_group(NF_BUTTON(obj), slist);

        tmp_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        nfui_nfobject_show(tmp_fixed);
        if (ivsc.dfunc.support_dmva_itx) nfui_nftable_attach((NFTABLE *)page_ntb[page_num], tmp_fixed, 6, row_num);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        if (ivsc.dfunc.support_dmva_itx) nfui_nffixed_put((NFFIXED *)tmp_fixed, obj, (width3[6] - size_w) / 2, (40 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_analysis_engine_event_handler);
        g_classic_radio[i] = obj;

        nfui_radio_button_add_group(NF_BUTTON(obj), slist);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE)
        {
            row_num = 0;
            page_num++;
        }
    }

    obj = (NFOBJECT *)nftool_normal_button_create_type1("CANCEL", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT *)nftool_normal_button_create_type1("APPLY", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT *)nftool_normal_button_create_type2("CLOSE", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    _init_db_data();
    _init_license_data();
    _init_capable_data();
    _prvSetDataToObjects();

    return 0;
}

gboolean VW_analysis_act_tab_in_handler()
{
    _init_db_data();
    _init_capable_data();
    return FALSE;
}

gboolean VW_analysis_act_tab_out_handler()
{
    mb_type ret;
    gint i;

    guint new_owner_chmask = 0;
    guint new_stream_chmask = 0;

    _prvLoadDataFromObjects();

    if (!memcmp(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT))
        return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK)
    {
#if 0        
        _get_changed_dvabox_new_stream(&new_owner_chmask, &new_stream_chmask);
        if (new_owner_chmask)
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "If you change the setting of the AI Box from recorder, you need to initialize current setting from AI BOX.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
            if (ret == NFTOOL_MB_CANCEL) {
                g_memmove(g_analysis_data, g_org_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
                _prvSetDataToObjects();
                return FALSE;
            }
            _connect_dvabox_new_owner(new_owner_chmask);
        }

        _connect_dvabox_new_stream(new_stream_chmask);
        _disconnect_dvabox_channel();
#endif
        _prvSetDataToObjects_aibox();

        DAL_set_aianalysis_act_data_all(g_analysis_data, GUI_CHANNEL_CNT);
        g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);

        _apply_analysis_engine_active();
        DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
        syscam_set_changeflag(1);
    }
    else
    {
        g_memmove(g_analysis_data, g_org_analysis_data, sizeof(AiAnalysisActData) * GUI_CHANNEL_CNT);
        _prvSetDataToObjects();
    }

    return FALSE;
}
