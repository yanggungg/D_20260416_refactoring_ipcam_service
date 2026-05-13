/*
 * vw_sys_camera_analysis_prop.c
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

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_menu.h"
#include "vw_sys_camera_analysis_act.h"
#include "vw_sys_camera_analysis_prop.h"
#include "vw_sys_camera_analysis_schd.h"

#include "vw_dvabx_prop.h"
#include "vw_sys_camera_deeplearning_property.h"
#include "vw_vca_rev_prop.h"

#include "scm.h"

#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"
#include "nf_ipcam_defs.h"






////////////////////////////////////////////////////////////
//
// private data types
//




////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_type_fixed[3];
static NFOBJECT *g_ch_combo = 0;
static NFOBJECT *g_engine_label = 0;

static gint g_pre_channel = 0;

static AiAnalysisActData g_analysis_data[GUI_CHANNEL_CNT];



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static void _init_db_data()
{
    gint i;

    memset(g_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_aianalysis_act_data(&g_analysis_data[i], i);
    }
}

static gint _show_engine_property(gint ch)
{
    if (g_analysis_data[ch].dvabox_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "AI BOX");
        _analysis_dvabx_prop_show_page(ch, g_analysis_data);
        nfui_nfobject_show(g_type_fixed[0]);
    }
    else if (g_analysis_data[ch].dvacam_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "AI CAM");
        _analysis_dvabx_prop_show_page(ch, g_analysis_data);
        nfui_nfobject_show(g_type_fixed[0]);
    }
    else if (g_analysis_data[ch].builtin_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "BUILT-IN AI");
        _analysis_builtin_prop_show_page(ch, g_analysis_data);
        nfui_nfobject_show(g_type_fixed[1]);
    }
    else if (g_analysis_data[ch].classic_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "CLASSIC VA");
        _analysis_classic_prop_show_page(ch, g_analysis_data);
        nfui_nfobject_show(g_type_fixed[2]);
    }
    else {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "NONE");
        _analysis_dvabx_prop_show_page(ch, g_analysis_data);
        nfui_nfobject_show(g_type_fixed[0]);
    } 

    return 0;
}

static gint _post_show_engine_property(gint ch)
{
    if (g_analysis_data[ch].dvabox_active) {
        _post_analysis_dvabx_prop_show_page();
    }
    else if (g_analysis_data[ch].dvacam_active) {

    }
    else if (g_analysis_data[ch].builtin_active) {

    }
    else if (g_analysis_data[ch].classic_active) {

    }
    else {

    } 

    return 0;
}

static gint _hide_engine_property(gint ch)
{
    if (g_analysis_data[ch].dvabox_active) {
        _analysis_dvabx_prop_hide_page(ch);
        nfui_nfobject_hide(g_type_fixed[0]);
    }
    else if (g_analysis_data[ch].dvacam_active) {
        _analysis_dvabx_prop_hide_page(ch);
        nfui_nfobject_hide(g_type_fixed[0]);
    }
    else if (g_analysis_data[ch].builtin_active) {
        _analysis_builtin_prop_hide_page(ch);
        nfui_nfobject_hide(g_type_fixed[1]);
    }
    else if (g_analysis_data[ch].classic_active) {
        _analysis_classic_prop_hide_page(ch);
        nfui_nfobject_hide(g_type_fixed[2]);
    }
    else {
        _analysis_dvabx_prop_hide_page(ch);
        nfui_nfobject_hide(g_type_fixed[0]);
    }
    
    return 0;
}

static gint _load_changed_data(gint ch)
{
    if (g_analysis_data[ch].dvabox_active || g_analysis_data[ch].dvacam_active) {
        _analysis_dvabx_prop_load_changed_data(ch);
    }

    if (g_analysis_data[ch].builtin_active) {
        _analysis_builtin_prop_load_changed_data(ch);
    }

    if (g_analysis_data[ch].classic_active) {
        _analysis_classic_prop_load_changed_data(ch);
    }
    return 0;
}

static gint _is_changed_data(gint ch)
{
    if (g_analysis_data[ch].dvabox_active || g_analysis_data[ch].dvacam_active) {
        return _analysis_dvabx_prop_is_changed_data();
    }

    if (g_analysis_data[ch].builtin_active) {
        return _analysis_builtin_prop_is_changed_data();
    }

    if (g_analysis_data[ch].classic_active) {
        return _analysis_classic_prop_is_changed_data();
    }

    return 0; 
}

static gint _save_data(gint ch)
{
    gint i;

    if (g_analysis_data[ch].dvabox_active || g_analysis_data[ch].dvacam_active) {
        _analysis_dvabx_prop_save_data();
    }

    if (g_analysis_data[ch].builtin_active) {
        _analysis_builtin_prop_save_data();
    }

    if (g_analysis_data[ch].classic_active) {
        _analysis_classic_prop_save_data();
    }

    scm_put_log(CHANGE_CAM_VCA, 0, 0);
    return 0;
}

static gint _cancel_data(gint ch, gint expose)
{
    if (g_analysis_data[ch].dvabox_active || g_analysis_data[ch].dvacam_active) {
        _analysis_dvabx_prop_cancel_data(ch, expose);
    }

    if (g_analysis_data[ch].builtin_active) {
        _analysis_builtin_prop_cancel_data(ch, expose);
    }

    if (g_analysis_data[ch].classic_active) {
        _analysis_classic_prop_cancel_data(ch, expose);
    }

    return 0;
}




////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
        NFOBJECT *wait_pop = NULL;
        mb_type ret;

        _load_changed_data(g_pre_channel);
        if (_is_changed_data(g_pre_channel)) 
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

            if (ret == NFTOOL_MB_OK) {
                _save_data(g_pre_channel);
                syscam_set_changeflag(1);
            }
            else {
                _cancel_data(g_pre_channel, 0);
            }
        }
        
        if (scm_get_ipcam_ai_type(g_pre_channel) == CAM_AI_TYPE_CLAIR2) {
            scm_set_ipcam_eosd_noshow_toggle(g_pre_channel, 1, __FUNCTION__);
        }

        _hide_engine_property(g_pre_channel);

        if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_CLAIR2) {
            // wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
            // dvaa_sync_aicam(ch);
            scm_set_ipcam_eosd_noshow_toggle(ch, 0, __FUNCTION__);
        }
        _show_engine_property(ch);

        nfui_signal_emit(g_engine_label, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_type_fixed[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_type_fixed[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_type_fixed[2], GDK_EXPOSE, TRUE);
        nfui_make_key_hierarchy(g_curwnd);

        _post_show_engine_property(ch);
        // if (wait_pop) nftool_remove_waitbox(wait_pop);
        g_pre_channel = ch;
    }
	else if ((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY))
	{
		gchar strCh[STRING_SIZE_CAMTITLE+8];
		gint i, j;
    	gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		nfui_combobox_remove_all((NFCOMBOBOX*)obj);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) 
		{
			memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
			j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
			var_get_camtitle(&strCh[j], (guint)i);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
		}

		nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), ch);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == GDK_DELETE)  
	{
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	}
    
    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));        

        _load_changed_data(ch);
        _cancel_data(ch, 1);
	}

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

        _load_changed_data(ch);
        if (_is_changed_data(ch)) 
        {
            _save_data(ch);
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            syscam_set_changeflag(1);            
        }
	}

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        VW_analysis_prop_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE :
        break;
        
        case GDK_DELETE:
        {

        }
        break;
            
        default :
        break;
    }

    return FALSE;

}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
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

gint VW_analysis_prop_init_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *type_fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;

    gchar strCh[STRING_SIZE_CAMTITLE+8];
    gchar strBuf[STRING_SIZE_CAMTITLE];
    
    gint pos_x, pos_y;  
    gint i, j;


    g_curwnd = nfui_nfobject_get_top(parent);


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    
    pos_x = 0;
    pos_y = 10;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 150, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 150;

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, 240, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_ch_combo = obj;

    g_pre_channel = 0;

	uxm_reg_imsg_event(g_ch_combo, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_ch_combo, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(g_ch_combo, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_ch_combo, INFY_USRDB_CHANGE_NOTIFY);

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(strCh, 0x00, sizeof(strCh)); 
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);        
        var_get_camtitle(&strCh[j], (guint)i);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
    }

    pos_x += 330;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 160;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(921));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    g_engine_label = obj;

    type_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(type_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(type_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H-50);
    nfui_nffixed_put((NFFIXED*)content_fixed, type_fixed, 0, 50);
    g_type_fixed[0] = type_fixed;

    _analysis_dvabx_prop_init_page(type_fixed);

    type_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(type_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(type_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H-50);
    nfui_nffixed_put((NFFIXED*)content_fixed, type_fixed, 0, 50);
    g_type_fixed[1] = type_fixed;

    _analysis_builtin_prop_init_page(type_fixed);

    type_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(type_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(type_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H-50);
    nfui_nffixed_put((NFFIXED*)content_fixed, type_fixed, 0, 50);
    g_type_fixed[2] = type_fixed; 

    _analysis_classic_prop_init_page(type_fixed); 

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    return 0;
}

gboolean VW_analysis_prop_tab_in_handler()
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

    _init_db_data();
    _show_engine_property(ch);
    nfui_make_key_hierarchy(g_curwnd);
    g_pre_channel = ch;
    
    return FALSE;
}

gboolean VW_analysis_prop_tab_out_handler()
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
    mb_type ret;

    _load_changed_data(ch);
    _hide_engine_property(ch);
    
    if (!_is_changed_data(ch)) 
    {
        if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_CLAIR2) {
            scm_set_ipcam_eosd_noshow_toggle(ch, 1, __FUNCTION__);
        }
        return FALSE;
    }

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK) {
        _save_data(ch);
        syscam_set_changeflag(1);
    }
    else {
        _cancel_data(ch, 0);
    }

    if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_CLAIR2) {
        scm_set_ipcam_eosd_noshow_toggle(ch, 1, __FUNCTION__);
    }

    return FALSE;
}

gboolean VW_analysis_prop_tab_changed_handler()
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
    NFOBJECT *wait_pop = NULL;

    if (scm_get_ipcam_ai_type(ch) == CAM_AI_TYPE_CLAIR2) {
        // wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
        // dvaa_sync_aicam(ch);
        scm_set_ipcam_eosd_noshow_toggle(ch, 0, __FUNCTION__);
        nfui_user_signal_emit(g_type_fixed[0], NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }
    _post_show_engine_property(ch);

	// if (wait_pop) nftool_remove_waitbox(wait_pop);
    return FALSE;
}
