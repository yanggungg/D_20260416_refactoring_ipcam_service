/*
 * vw_sys_camera_analysis_lpr_database.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Aug 20, 2019
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
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nftile.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_menu.h"
#include "vw_sys_camera_analysis_act.h"
#include "vw_sys_camera_analysis_prop.h"
#include "vw_sys_camera_analysis_schd.h"
#include "vw_sys_camera_analysis_plateno_database.h"

#include "vw_ai_analytics_add_plateno_popup.h"
#include "vw_ai_analytics_edit_group_popup.h"
#include "vw_vkeyboard.h"

#include "scm.h"

#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"
#include "nf_ipcam_sql_utils.h"






////////////////////////////////////////////////////////////
//
// private data types
//

#define MAX_REGIST_PLATENO             10000

#define MAX_DISP_PLATENO               16

#define ALLGROUP_STRING             "All Groups"
#define UNASSIGNED_STRING           "Unassigned"
#define NEED_AIBOX_LOGIN_STRING     "You need to log in to the AI Box to set up the license plate database."



////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ch_combo;
static NFOBJECT *g_engine_label;
static NFOBJECT *g_group_combo;
static NFOBJECT *g_login_label;
static NFOBJECT *g_login_button;
static NFOBJECT *g_add_button;
static NFOBJECT *g_bulk_button;
static NFOBJECT *g_alldelete_button;
static NFOBJECT *g_plateno_label[MAX_DISP_PLATENO];
static NFOBJECT *g_name_label[MAX_DISP_PLATENO];
static NFOBJECT *g_phone_label[MAX_DISP_PLATENO];
static NFOBJECT *g_memo_label[MAX_DISP_PLATENO];
static NFOBJECT *g_edit_button[MAX_DISP_PLATENO];
static NFOBJECT *g_delete_button[MAX_DISP_PLATENO];
static NFOBJECT *g_page_label;

static AiAnalysisActData g_org_analysis_data[GUI_CHANNEL_CNT];
static AiAnalysisActData g_analysis_data[GUI_CHANNEL_CNT];
static guint g_curr_page = 0;

static gchar g_search_text_string[64] = {0, };

static lpr_group_info *g_dev_group_info = 0;
static gint g_dev_group_cnt = 0;

static lp_info *g_dev_plateno_info = 0;
static gint g_dev_plateno_cnt = 0;




////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _check_plateno_algorithm(gint ch, gint expose)
{
    gchar strBuf[64];
    gint i;

    if (g_analysis_data[ch].dvabox_active) {
        memset(strBuf, 0x00, sizeof(strBuf));
        snprintf(strBuf, sizeof(strBuf), "%s (%s)", lookup_string("AI BOX"), g_analysis_data[ch].dvabox_mac);
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, strBuf);

        if (strlen(g_analysis_data[ch].dvabox_id) && strlen(g_analysis_data[ch].dvabox_pass))
        {
            if (nf_api_aibox_login_check(g_analysis_data[ch].dvabox_ipaddr, g_analysis_data[ch].dvabox_id, g_analysis_data[ch].dvabox_pass) == 0)
            {
                nfui_nflabel_set_text((NFLABEL*)g_login_label, "");
                nfui_nfobject_disable(g_login_button);
                nfui_nfobject_enable(g_group_combo);
                nfui_nfobject_enable(g_add_button);
                nfui_nfobject_enable(g_bulk_button);
                nfui_nfobject_enable(g_alldelete_button);
            }
            else
            {
                nfui_nflabel_set_text((NFLABEL*)g_login_label, NEED_AIBOX_LOGIN_STRING);
                nfui_nfobject_enable(g_login_button);
                nfui_nfobject_disable(g_group_combo);
                nfui_nfobject_disable(g_add_button);
                nfui_nfobject_disable(g_bulk_button);
                nfui_nfobject_disable(g_alldelete_button);                
            }
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL*)g_login_label, NEED_AIBOX_LOGIN_STRING);
            nfui_nfobject_enable(g_login_button);
            nfui_nfobject_disable(g_group_combo);
            nfui_nfobject_disable(g_add_button);
            nfui_nfobject_disable(g_bulk_button);
            nfui_nfobject_disable(g_alldelete_button);            
        }
    }
    else if (g_analysis_data[ch].dvacam_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "AI CAM");
        nfui_nfobject_enable(g_group_combo);
        nfui_nfobject_enable(g_add_button);
        nfui_nfobject_enable(g_bulk_button);
        nfui_nfobject_enable(g_alldelete_button);        
        nfui_nflabel_set_text((NFLABEL*)g_login_label, "");
        nfui_nfobject_disable(g_login_button);
    }
    else if (g_analysis_data[ch].builtin_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "BUILT-IN AI");
        nfui_nfobject_disable(g_group_combo);
        nfui_nfobject_disable(g_add_button);
        nfui_nfobject_disable(g_bulk_button);
        nfui_nfobject_disable(g_alldelete_button);
        nfui_nflabel_set_text((NFLABEL*)g_login_label, "");
        nfui_nfobject_disable(g_login_button);
    }
    else if (g_analysis_data[ch].classic_active) {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "CLASSIC VA");
        nfui_nfobject_disable(g_group_combo);
        nfui_nfobject_disable(g_add_button);
        nfui_nfobject_disable(g_bulk_button);
        nfui_nfobject_disable(g_alldelete_button);
        nfui_nflabel_set_text((NFLABEL*)g_login_label, "");
        nfui_nfobject_disable(g_login_button);
    }
    else {
        nfui_nflabel_set_text((NFLABEL*)g_engine_label, "NONE");
        nfui_nfobject_disable(g_group_combo);
        nfui_nfobject_disable(g_add_button);
        nfui_nfobject_disable(g_bulk_button);
        nfui_nfobject_disable(g_alldelete_button);
        nfui_nflabel_set_text((NFLABEL*)g_login_label, "");
        nfui_nfobject_disable(g_login_button);
    } 

    if (expose)
    {
        nfui_signal_emit(g_engine_label, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_login_label, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_login_button, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_group_combo, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_add_button, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_bulk_button, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_alldelete_button, GDK_EXPOSE, TRUE);                
    }
    return 0;
}

static gint _change_group_list(gint ch)
{
    lpr_group_info *dev_group_info = 0;
    gint dev_group_cnt = 0;

    gint pre_group_id;
    gint i, set_idx = 0;

    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo) == 0) pre_group_id = AIBOX_GROUP_SERACH_ALL;
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo) == g_slist_length(NF_COMBOBOX(g_group_combo)->data)-1) pre_group_id = AIBOX_GROUP_SERACH_UNASSIGNED;
    else pre_group_id = g_dev_group_info[nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo)-1].id;

    if (g_dev_group_info) free(g_dev_group_info);
    g_dev_group_info = 0;
    g_dev_group_cnt = 0;

    nfui_combobox_remove_all((NFCOMBOBOX*)g_group_combo);

    if (nfui_nfobject_is_disabled(g_group_combo)) return -1;
    
    if (g_analysis_data[ch].dvabox_active) nf_api_lpr_group_list_get(g_analysis_data[ch].dvabox_ipaddr, &dev_group_info, &dev_group_cnt);
    //else nf_api_lpr_group_list_get(ch, &dev_group_info, &dev_group_cnt);

    nfui_combobox_append_data((NFCOMBOBOX*)g_group_combo, lookup_string(ALLGROUP_STRING));

    for (i = 0; i < dev_group_cnt; i++) {
        nfui_combobox_append_data((NFCOMBOBOX*)g_group_combo, dev_group_info[i].name);
        if (pre_group_id == dev_group_info[i].id) set_idx = i;
    }

    nfui_combobox_append_data((NFCOMBOBOX*)g_group_combo, lookup_string(UNASSIGNED_STRING));

    if (pre_group_id == AIBOX_GROUP_SERACH_ALL) set_idx = 0;
    else if (pre_group_id == AIBOX_GROUP_SERACH_UNASSIGNED) set_idx = g_slist_length(NF_COMBOBOX(g_group_combo)->data)-1;

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_group_combo, set_idx);

    g_dev_group_info = dev_group_info;
    g_dev_group_cnt = dev_group_cnt;    
    return 0;
}

static gint _change_plateno_list(gint ch, gint page)
{
    lp_info *dev_plateno_info = 0;
    gint dev_plateno_cnt = 0;

    gint group_id = 0;

    lpr_db_search_option search_option;
    gint max_page;
    gint retVal;

    NFOBJECT *wait_pop;

    if (g_dev_plateno_info) free(g_dev_plateno_info);
    g_dev_plateno_info = 0;
    dev_plateno_cnt = 0;

    if (nfui_nfobject_is_disabled(g_group_combo)) return -1;

    //nftool_mbox_sleep_auto(g_curwnd, 1, "NOTICE", "Please wait...");
    //wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait...");

    if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo) == 0) {
        group_id = AIBOX_GROUP_SERACH_ALL;
    }
    else if (nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo) == g_slist_length(NF_COMBOBOX(g_group_combo)->data)-1) {
        group_id = AIBOX_GROUP_SERACH_UNASSIGNED;
    }
    else {
        group_id = g_dev_group_info[nfui_combobox_get_cur_index((NFCOMBOBOX*)g_group_combo)-1].id;
    }

    memset(&search_option, 0x00, sizeof(lpr_db_search_option));
    if (strlen(g_search_text_string)) search_option.plate = g_search_text_string;

    if (g_analysis_data[ch].dvabox_active) 
    {
        retVal = nf_api_aibox_lpr_list_get(g_analysis_data[ch].dvabox_ipaddr, page, MAX_DISP_PLATENO, 
                                            group_id, &search_option, &max_page, &dev_plateno_info, &dev_plateno_cnt);
    }
    else 
    {
        //retVal = nf_api_aicam_fr_face_list_get(ch, &dev_face_info, &dev_face_cnt);
    }

    //nftool_remove_waitbox(wait_pop);

#if 0
    if (retVal < 0) {
        memset(strLookup, 0x00, sizeof(strLookup));
        snprintf(strLookup, sizeof(strLookup)-1, lookup_string("Failed to get a list of face photos.\nCheck the status of the AI Box and camera or check the number of registered photos.\nPossible to link face photos up to %d."), MAX_REGIST_FACE);
        nftool_mbox(g_curwnd, "NOTICE", strLookup, NFTOOL_MB_OK);
        return -1;
    }
#endif

    g_message("%s, %d, dev_plateno_cnt:%d", __FUNCTION__, __LINE__, dev_plateno_cnt);
    g_dev_plateno_info = dev_plateno_info;
    g_dev_plateno_cnt = dev_plateno_cnt;

    return 0;
}

static gint _set_draw_plateno_data(gint ch, guint page)
{
    lp_info *dev_plateno_info = g_dev_plateno_info;
    gint dev_plateno_cnt = g_dev_plateno_cnt;
    gint i;

    for (i = 0; i < MAX_DISP_PLATENO; i++)
    {
        if (i < dev_plateno_cnt)
        {
            nfui_nflabel_set_text((NFLABEL*)g_plateno_label[i], dev_plateno_info[i].plate);
            nfui_nflabel_set_text((NFLABEL*)g_name_label[i], dev_plateno_info[i].name);
            nfui_nflabel_set_text((NFLABEL*)g_phone_label[i], dev_plateno_info[i].phone);
            nfui_nflabel_set_text((NFLABEL*)g_memo_label[i], dev_plateno_info[i].memo);

            nfui_nfobject_set_data(g_plateno_label[i], "plateno_id", GINT_TO_POINTER(dev_plateno_info[i].id));
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL*)g_plateno_label[i], "");
            nfui_nflabel_set_text((NFLABEL*)g_name_label[i], "");
            nfui_nflabel_set_text((NFLABEL*)g_phone_label[i], "");
            nfui_nflabel_set_text((NFLABEL*)g_memo_label[i], "");

            nfui_nfobject_set_data(g_plateno_label[i], "plateno_id", GINT_TO_POINTER(-1));
        }
    }

    return 0;
}

static gint _set_plateno_page_cnt(guint page, gint expose)
{
    gchar strBuf[64];

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "- %d -", page+1);
    nfui_nflabel_set_text((NFLABEL*)g_page_label, strBuf);

    if (expose) nfui_signal_emit(g_page_label, GDK_EXPOSE, TRUE);
    return 0;
}

static gint _draw_plateno_data_list(gint i)
{
    nfui_signal_emit(g_plateno_label[i], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_name_label[i], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_label[i], GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_memo_label[i], GDK_EXPOSE, TRUE);

    return 0;
}

static gint _expose_plateno_data()
{
    gint i;

    for (i = 0; i < MAX_DISP_PLATENO; i++) {
        _draw_plateno_data_list(i);
    }
    return 0;
}

static gint _expose_group_data()
{
    nfui_signal_emit(g_group_combo, GDK_EXPOSE, TRUE);
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

        _check_plateno_algorithm(ch, 1);
        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();
        _set_plateno_page_cnt(g_curr_page, 1);
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

static gboolean post_group_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
    	gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        gint i;

        g_curr_page = 0;

        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _set_plateno_page_cnt(g_curr_page, 1);
        _expose_plateno_data();
    }
    
    return FALSE;
}

static gboolean post_aibox_login_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        gint i;

        gchar aibox_userid[128];
        gchar aibox_pass[128];
        gchar strBuf[128];

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(aibox_userid, 0x00, sizeof(aibox_userid));
        memset(aibox_pass, 0x00, sizeof(aibox_pass));

        memset(strBuf, 0x00, sizeof(strBuf));
        snprintf(strBuf, sizeof(strBuf)-1, "%s (%s)", lookup_string("Login"), lookup_string("AI BOX"));
        vw_manual_login_popup(g_curwnd, strBuf, aibox_userid, sizeof(aibox_userid), aibox_pass, sizeof(aibox_pass));

        if (!strlen(aibox_userid)) return FALSE;
        if (!strlen(aibox_pass)) return FALSE;

        if (nf_api_aibox_login_check(g_analysis_data[ch].dvabox_ipaddr, aibox_userid, aibox_pass) == 0)
        {
            nftool_mbox(g_curwnd, "NOTICE", "SUCCESS", NFTOOL_MB_OK);

            g_snprintf(g_analysis_data[ch].dvabox_id, sizeof(g_analysis_data[ch].dvabox_id)-1, "%s", aibox_userid);
            g_snprintf(g_analysis_data[ch].dvabox_pass, sizeof(g_analysis_data[ch].dvabox_pass)-1, "%s", aibox_pass);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (i == ch) continue;

                if ((g_analysis_data[i].dvabox_active) && (strcmp(g_analysis_data[ch].dvabox_mac, g_analysis_data[i].dvabox_mac) == 0))
                {
                    g_snprintf(g_analysis_data[i].dvabox_id, sizeof(g_analysis_data[i].dvabox_id)-1, "%s", aibox_userid);
                    g_snprintf(g_analysis_data[i].dvabox_pass, sizeof(g_analysis_data[i].dvabox_pass)-1, "%s", aibox_pass);
                }
            }

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (memcmp(&g_org_analysis_data[i], &g_analysis_data[i], sizeof(AiAnalysisActData)) != 0)
                {
                    DAL_set_aianalysis_act_data(g_analysis_data[i], i);
                    g_memmove(&g_org_analysis_data[i], &g_analysis_data[i], sizeof(AiAnalysisActData));
                    syscam_set_changeflag(1);
                }
            }

            g_curr_page = 0;
            _check_plateno_algorithm(ch, 1);
            _change_group_list(ch);
            _change_plateno_list(ch, g_curr_page);
            _set_draw_plateno_data(ch, g_curr_page);
            _expose_group_data();
            _expose_plateno_data();
        }
        else
        {
            nftool_mbox(g_curwnd, "NOTICE", "LOGIN FAIL", NFTOOL_MB_OK);
        }        
	}

    return FALSE;
}

static gboolean post_add_plateno_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        PLATENO_DEV_DATA_T plateno_data;
        gint retVal;

        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(&plateno_data, 0x00, sizeof(PLATENO_DEV_DATA_T));
        plateno_data.ch = ch;
        plateno_data.dvabox_ipaddr = g_analysis_data[ch].dvabox_ipaddr;
        plateno_data.plateno_id = -1;

        retVal = vw_ai_analytics_add_plateno_popup_open(g_curwnd, &plateno_data);
        if (retVal == -1) return FALSE;

        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();
	}

    return FALSE;
}

static gboolean post_bulk_upload_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        gint retVal;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        ssm_stop_auto_logout();

        retVal = vw_ai_analytics_plateno_bulk_upload_popup_open(g_curwnd, ch, g_analysis_data[ch].dvabox_ipaddr);
        ssm_start_auto_logout();

        if (retVal == -1) return FALSE;

        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();
	}

    return FALSE;
}

static gboolean post_delete_all_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        NFOBJECT *wait_pop;
        mb_type ret;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        ret = nftool_mbox(g_curwnd, "CONFIRM", "All group and face photo data in the AI box will be deleted.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;

        //nftool_mbox_sleep_auto(g_curwnd, 1, "NOTICE", "Please wait...");
        //wait_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Please wait...");

        if (g_analysis_data[ch].dvabox_active) nf_api_aibox_lpr_delete_all(g_analysis_data[ch].dvabox_ipaddr);
        //else nf_api_aicam_fr_face_delete_all(ch);

        //nftool_remove_waitbox(wait_pop);

        g_curr_page = 0;
        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();        
	}

    return FALSE;
}

static gboolean post_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        PLATENO_DEV_DATA_T plateno_data;
        gint i, plateno_id;
        gint retVal;

        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        for (i = 0; i < MAX_DISP_PLATENO; i++)
        {
            if (obj == g_edit_button[i]) break;
        }
        if (i == MAX_DISP_PLATENO) return FALSE;

        plateno_id = GPOINTER_TO_INT(nfui_nfobject_get_data(g_plateno_label[i], "plateno_id"));
        if (plateno_id == -1) return FALSE;

        memset(&plateno_data, 0x00, sizeof(PLATENO_DEV_DATA_T));
        plateno_data.ch = ch;
        plateno_data.dvabox_ipaddr = g_analysis_data[ch].dvabox_ipaddr;
        plateno_data.plateno_id = plateno_id;

        retVal = vw_ai_analytics_add_plateno_popup_open(g_curwnd, &plateno_data);
        if (retVal == -1) return FALSE;

        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();
	}

    return FALSE;
}

static gboolean post_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
        gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        for (i = 0; i < MAX_DISP_PLATENO; i++)
        {
            if (obj == g_delete_button[i]) break;
        }
        if (i == MAX_DISP_PLATENO) return FALSE;

        if (g_analysis_data[ch].dvabox_active) aibox_lpr_delete(g_analysis_data[ch].dvabox_ipaddr, nfui_nflabel_get_text((NFLABEL*)g_plateno_label[i]));

        _change_group_list(ch);
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_group_data();
        _expose_plateno_data();        
	}

    return FALSE;
}

static gboolean post_search_text_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		gchar *strBuf;
		gchar *strTemp;

		gint win_x, win_y; 
		gint gap_x, gap_y;
		
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
		gint i;

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		strTemp = VirtualKey_Open(g_curwnd, "", gap_x, gap_y+44, 63, VKEY_MULTIKEYPD);
		if (strTemp)
        {
            if (strlen(strTemp))
            {
                memset(g_search_text_string, 0x00, sizeof(g_search_text_string));
                snprintf(g_search_text_string, sizeof(g_search_text_string)-1, "%s", strTemp);

                nfui_nflabel_set_text((NFOBJECT*)obj, strTemp);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
            else
            {
                memset(g_search_text_string, 0x00, sizeof(g_search_text_string));

                nfui_nflabel_set_text((NFOBJECT*)obj, lookup_string("License plate search"));
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }

            ifree(strTemp);

            g_curr_page = 0;
            _change_plateno_list(ch, g_curr_page);
            _set_draw_plateno_data(ch, g_curr_page);
            _expose_plateno_data();
            _set_plateno_page_cnt(g_curr_page, 1);
        }
	}

    return FALSE;
}

static gboolean post_prev_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_curr_page == 0) return FALSE;

        g_curr_page--;
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_plateno_data();
        _set_plateno_page_cnt(g_curr_page, 1);
	}

    return FALSE;
}

static gboolean post_next_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_curr_page++;
        _change_plateno_list(ch, g_curr_page);
        _set_draw_plateno_data(ch, g_curr_page);
        _expose_plateno_data();
        _set_plateno_page_cnt(g_curr_page, 1);
	}

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_analysis_plateno_database_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curr_page = 0;

        memset(g_search_text_string, 0x00, sizeof(g_search_text_string));

        if (g_dev_group_info) free(g_dev_group_info);
        g_dev_group_info = 0;
        g_dev_group_cnt = 0;

        if (g_dev_plateno_info) free(g_dev_plateno_info);
        g_dev_plateno_info = 0;
        g_dev_plateno_cnt = 0;

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

gint VW_analysis_plateno_database_init_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
    NFOBJECT *group_fixed;
	NFOBJECT *fixed;
    NFOBJECT *number_tbl;
	NFOBJECT *obj;

	GdkPixbuf *group_tab_img[4];

    const guint tab_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(40), COLOR_IDX(40), COLOR_IDX(40), COLOR_IDX(40)};

    GSList *slist = NULL;
    guint number_tbl_w[] = {360, 240, 240, 310, 140, 140};
    gint radio_size_w, radio_size_h;
    gint i, j;

    gchar strCh[STRING_SIZE_CAMTITLE+8];
    gchar strBuf[64];
    gint pos_x, pos_y;

    gchar strLookup[1024];


	group_tab_img[0] = nfui_get_image_from_file(IMG_N_SUB_TAB, NULL);
	group_tab_img[1] = nfui_get_image_from_file(IMG_S_SUB_TAB, NULL);
	group_tab_img[2] = nfui_get_image_from_file(IMG_S_SUB_TAB, NULL);
	group_tab_img[3] = nfui_get_image_from_file(IMG_N_SUB_TAB, NULL);


    g_curwnd = nfui_nfobject_get_top(parent);
    g_curr_page = 0;

    pos_x = 20;
    pos_y = 10;

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+220, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_ch_combo = obj;

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(strCh, 0x00, sizeof(strCh)); 
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);        
        var_get_camtitle(&strCh[j], (guint)i);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(NEED_AIBOX_LOGIN_STRING, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 740, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+570, pos_y);
    nfui_nfobject_show(obj);
    g_login_label = obj;

    pos_x = 20;
    pos_y += 60;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(921));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+220, pos_y);
    nfui_nfobject_show(obj);
    g_engine_label = obj;

	obj = nftool_normal_button_create_type3("Login", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+570, pos_y);
    nfui_regi_post_event_callback(obj, post_aibox_login_button_event_handler);
    g_login_button = obj;    

    pos_x = 20;
    pos_y += 42;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+220, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_group_combo_event_handler);
    g_group_combo = obj;

	obj = nftool_normal_button_create_type3("ADD LICENSE PLATE", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+570, pos_y);
    nfui_regi_post_event_callback(obj, post_add_plateno_button_event_handler);
    g_add_button = obj;

    pos_y = 70;

	obj = nftool_normal_button_create_type3("BULK UPLOAD", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, MENU_V_SUBTAB_INNER_W-20-220*2-4, pos_y);
    nfui_regi_post_event_callback(obj, post_bulk_upload_button_event_handler);
    g_bulk_button = obj;

	obj = nftool_normal_button_create_type3("DELETE ALL", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, MENU_V_SUBTAB_INNER_W-20-220, pos_y);
    nfui_regi_post_event_callback(obj, post_delete_all_button_event_handler);
    g_alldelete_button = obj;

    pos_y += 44;

    obj = nfui_nfimage_new(IMG_N_START_MENU_SEARCH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, MENU_V_SUBTAB_INNER_W-20-400-40, pos_y);

    obj = nfui_nflabel_new_text_box(lookup_string("License plate search"), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, MENU_V_SUBTAB_INNER_W-20-400, pos_y);
    nfui_regi_post_event_callback(obj, post_search_text_label_event_handler);

    group_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(group_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_set_size(group_fixed, MENU_V_SUBTAB_INNER_W-40, MENU_V_SUBTAB_INNER_H-180);
    nfui_nfobject_show(group_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, group_fixed, 20, 170);

    number_tbl = (NFOBJECT*)nfui_nftable_new(4, 1, 1, 1, number_tbl_w, 34);
    //nfui_nftable_set_draw_outline((NFTABLE*)number_tbl, TRUE);
    //nfui_nfobject_modify_bg(number_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(number_tbl);
    nfui_nffixed_put((NFFIXED*)group_fixed, number_tbl, 16, 20);

    obj = nfui_nflabel_new_with_pango_font("PLATE NUMBER", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)number_tbl, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("OWNER NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)number_tbl, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("PHONE NUMBER", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)number_tbl, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("MEMO", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)number_tbl, obj, 3, 0);            

    number_tbl = (NFOBJECT*)nfui_nftable_new(6, MAX_DISP_PLATENO, 1, 1, number_tbl_w, 34);
    nfui_nftable_set_draw_outline((NFTABLE*)number_tbl, TRUE);
    nfui_nfobject_modify_bg(number_tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(number_tbl);
    nfui_nffixed_put((NFFIXED*)group_fixed, number_tbl, 16, 55);

    for (i = 0; i < MAX_DISP_PLATENO; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_support_multi_lang(obj, FALSE);        
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 0, i);
        nfui_nfobject_set_data(obj, "plateno_id", GINT_TO_POINTER(-1));
        g_plateno_label[i] = obj;

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_support_multi_lang(obj, FALSE);        
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 1, i);
        g_name_label[i] = obj;

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_support_multi_lang(obj, FALSE);        
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 2, i);
        g_phone_label[i] = obj;

    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);    	
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_support_multi_lang(obj, FALSE);        
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 3, i);
        g_memo_label[i] = obj;

    	obj = nftool_normal_button_create_popup_type1("EDIT", number_tbl_w[4]);
        nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), NULL);
    	nfui_nfobject_show(obj);   	
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 4, i);
        nfui_regi_post_event_callback(obj, post_edit_button_event_handler);
        g_edit_button[i] = obj;

    	obj = nftool_normal_button_create_popup_type1("DELETE", number_tbl_w[5]);
        nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), NULL);
    	nfui_nfobject_show(obj);   	
    	nfui_nftable_attach((NFTABLE*)number_tbl, obj, 5, i);
        nfui_regi_post_event_callback(obj, post_delete_button_event_handler);
        g_delete_button[i] = obj;
    }


    obj = nfui_nflabel_new_with_pango_font("- 1 -", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 240, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)group_fixed, obj, (group_fixed->width-240)/2, group_fixed->height-44);
    g_page_label = obj;

    memset(strBuf, 0x00, sizeof(strBuf));
    snprintf(strBuf, sizeof(strBuf)-1, "< %s", lookup_string("PREVIOUS"));

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2(strBuf, 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)group_fixed, obj, group_fixed->width-424, group_fixed->height-44);
    nfui_regi_post_event_callback(obj, post_prev_button_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    snprintf(strBuf, sizeof(strBuf)-1, "%s >", lookup_string("NEXT"));

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2(strBuf, 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)group_fixed, obj, group_fixed->width-240, group_fixed->height-44);
    nfui_regi_post_event_callback(obj, post_next_button_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    //nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    //nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

    obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", 192);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    return 0;
}

gboolean VW_analysis_plateno_database_tab_in_handler()
{
	uxm_reg_imsg_event(g_ch_combo, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_ch_combo, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(g_ch_combo, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_ch_combo, INFY_USRDB_CHANGE_NOTIFY);
    return FALSE;
}

gboolean VW_analysis_plateno_database_tab_changed_handler()
{
    gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_ch_combo));
    gint i;

    memset(g_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
    memset(g_org_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_aianalysis_act_data(&g_analysis_data[i], i);
    }
    g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);    

    _check_plateno_algorithm(ch, 1);
    _change_group_list(ch);
    _change_plateno_list(ch, g_curr_page);
    _set_draw_plateno_data(ch, g_curr_page);
    _expose_group_data();
    _expose_plateno_data();
    _set_plateno_page_cnt(g_curr_page, 1);
    return FALSE;
}

gboolean VW_analysis_plateno_database_tab_out_handler()
{
	uxm_unreg_imsg_event(g_ch_combo, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_unreg_imsg_event(g_ch_combo, INFY_USRDB_CHANGE_NOTIFY);
    return FALSE;
}
