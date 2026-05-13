/*
 * vw_ai_analytics_add_plateno_popup.c
 * 	- timeline dlva viewer
 *	- dependencies :
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Sep 1, 2019
 *
 */

#include <time.h>

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>

#include "nf_common.h"
#include "nf_notify.h"


#include "scm.h"
#include "vsm.h"
#include "modules/ssm.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vkeyboard.h"
#include "vw_ai_analytics_add_plateno_popup.h"

#include "nf_api_dlva.h"



////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (700)
#define POPUP_SIZE_HEI          (640)

#define STR_SELECT_GROUP		"Please select a group."
#define STR_REQUIRED_GROUP		"* Required entry"

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_plateno_label;
static NFOBJECT *g_name_label;
static NFOBJECT *g_phone_label;
static NFOBJECT *g_memo_label;
static NFOBJECT *g_group_combo_obj;
static NFOBJECT *g_group_list_obj;

static AiAnalysisActData g_analysis_data;
static gint g_ret_code = 0;

static PLATENO_DEV_DATA_T *g_plateno_dev_data = 0;

static lpr_group_info *g_dev_group_info = 0;
static gint g_dev_group_cnt = 0;

static gint g_register_group_id[MAX_AIBOX_DB_GROUP_SIZE];
static gint g_register_group_len = 0;





////////////////////////////////////////////////////////////////
//
// private interfaces
//



////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_input_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
		gchar *strBuf;
		gchar *strTemp;

		gint win_x, win_y; 
		gint gap_x, gap_y; 

		gint max_str_cnt = 16;

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		if (obj == g_memo_label) max_str_cnt = 84;
		else if (obj == g_name_label) max_str_cnt = 84;
		else max_str_cnt = 16;

		strTemp = VirtualKey_Open(g_curwnd, "", gap_x, gap_y+44, max_str_cnt, VKEY_MULTIKEYPD);

		if (strTemp) 
		{
			if (strlen(strTemp))
			{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
			else
			{
				if (obj == g_plateno_label) nfui_nflabel_set_text((NFLABEL*)obj, STR_REQUIRED_GROUP);
				else nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		
			ifree(strTemp);
			strTemp = NULL;
		}		
	}

	return FALSE;
}

static gboolean post_device_add_group_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		lpr_group_info *dev_group_info = 0;
		gint dev_group_cnt = 0;
		gchar *strTemp;

		gint win_x, win_y; 
		gint gap_x, gap_y; 

		gint i;
		gint retVal;

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		strTemp = VirtualKey_Open(g_curwnd, "", gap_x, gap_y+44, 64, VKEY_NORMAL|VKEY_MULTIKEYPD);
		if (!strTemp) return FALSE;

		retVal = nf_api_lpr_group_add(g_plateno_dev_data->dvabox_ipaddr, strTemp);
		if (retVal != 0)
		{
			if (g_dev_group_info) free(g_dev_group_info);
			g_dev_group_info = 0;
			g_dev_group_cnt = 0;

			nfui_combobox_remove_all((NFCOMBOBOX*)g_group_combo_obj);
			nfui_combobox_set_display_string((NFCOMBOBOX*)g_group_combo_obj, lookup_string(STR_SELECT_GROUP));

			if (g_analysis_data.dvabox_active) nf_api_lpr_group_list_get(g_plateno_dev_data->dvabox_ipaddr, &dev_group_info, &dev_group_cnt);
			//else nf_api_lpr_group_list_get(g_plateno_dev_data->dvabox_ipaddr, &dev_group_info, &dev_group_cnt);

			for (i = 0; i < dev_group_cnt; i++) {
				nfui_combobox_append_data((NFCOMBOBOX*)g_group_combo_obj, dev_group_info[i].name);
			}
			nfui_signal_emit(g_group_combo_obj, GDK_EXPOSE, TRUE);

			g_dev_group_info = dev_group_info;
			g_dev_group_cnt = dev_group_cnt;    			
		}
		else
		{
			nftool_mbox(g_curwnd, "NOTICE", "Failed to add group.", NFTOOL_MB_OK);
		}
		
			ifree(strTemp);
		}		

	return FALSE;
}

static gboolean post_register_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint i, idx;

		idx = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		for (i = 0; i < g_register_group_len; i++)
		{
			if (g_dev_group_info[idx].id == g_register_group_id[i]) return FALSE;
		}
		
		g_register_group_id[g_register_group_len] = g_dev_group_info[idx].id;
		g_register_group_len++;

		nfui_listbox_set_text_single_column(NF_LISTBOX(g_group_list_obj), g_dev_group_info[idx].name);
		nfui_signal_emit(g_group_list_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_list_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint group_list[MAX_AIBOX_DB_GROUP_SIZE];
		gint i, idx;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_group_list_obj);
		if (idx < 0) return FALSE;

		g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, idx);

		memset(group_list, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
		memcpy(group_list, g_register_group_id, sizeof(gint)*g_register_group_len);
		group_list[idx] = 0;

		memset(g_register_group_id, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
		g_register_group_len = 0;

		for (i = 0; i < MAX_AIBOX_DB_GROUP_SIZE; i++)
		{
			if (group_list[i]) g_register_group_id[g_register_group_len++] = group_list[i];
		}

		nfui_listbox_delete((NFLISTBOX*)g_group_list_obj, idx);
		nfui_signal_emit(g_group_list_obj, GDK_EXPOSE, TRUE);
	}

			return FALSE;
		}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
		{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		lp_info info;
		gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_plateno_label))) {
			nftool_mbox(g_curwnd, "NOTICE", "Please enter a plate number.", NFTOOL_MB_OK);
			return FALSE;	
		}

		memset(&info, 0x00, sizeof(lp_info));
		info.id = g_plateno_dev_data->plateno_id;
		snprintf(info.plate, sizeof(info.plate)-1, "%s", nfui_nflabel_get_text((NFLABEL*)g_plateno_label));
		snprintf(info.name, sizeof(info.name)-1, "%s", nfui_nflabel_get_text((NFLABEL*)g_name_label));
		snprintf(info.phone, sizeof(info.phone)-1, "%s", nfui_nflabel_get_text((NFLABEL*)g_phone_label));
		snprintf(info.memo, sizeof(info.memo)-1, "%s", nfui_nflabel_get_text((NFLABEL*)g_memo_label));

		for (i = 0; i < g_register_group_len; i++) {
			info.group_list[i].id = g_register_group_id[i];
		}
		info.group_length = g_register_group_len;

		if (g_plateno_dev_data->plateno_id >= 0) 
		{
			if (g_analysis_data.dvabox_active) nf_api_lpr_modify(g_plateno_dev_data->dvabox_ipaddr, &info);
			//else nf_api_lpr_modify(unsigned int aibox_ip, lp_info *info);
		}
		else
		{
			if (g_analysis_data.dvabox_active) nf_api_lpr_add(g_plateno_dev_data->dvabox_ipaddr, &info);
			//else nf_api_lpr_add(unsigned int aibox_ip, lp_info *info);
		}

		nfui_nfobject_hide(g_curwnd);

		g_ret_code = 0;	
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_hide(g_curwnd);

		g_ret_code = -1;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		if (g_dev_group_info) free(g_dev_group_info);
		g_dev_group_info = 0;
		g_dev_group_cnt = 0;

		gtk_main_quit();
	}

	return FALSE;
}



////////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_add_plateno_popup_open(NFWINDOW *parent, PLATENO_DEV_DATA_T *data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	lpr_group_info *dev_group_info = 0;
	gint dev_group_cnt = 0;

	lp_info dev_plateno_info;

	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i;

	guint lc_size[] = {POPUP_SIZE_WID-220-40, };
	guint li_size_w, li_size_h;
	gchar *list_text;


	g_ret_code = -1;


	g_plateno_dev_data = data;

	memset(&g_analysis_data, 0x00, sizeof(AiAnalysisActData));
	DAL_get_aianalysis_act_data(&g_analysis_data, data->ch);

	if (g_analysis_data.dvabox_active) nf_api_lpr_group_list_get(data->dvabox_ipaddr, &dev_group_info, &dev_group_cnt);
	//else nf_api_lpr_group_list_get(data->dvabox_ipaddr, &dev_group_info, &dev_group_cnt);

    g_dev_group_info = dev_group_info;
    g_dev_group_cnt = dev_group_cnt;    

	memset(&dev_plateno_info, 0x00, sizeof(lp_info));
	if (data->plateno_id != -1) {
		if (g_analysis_data.dvabox_active) nf_api_aibox_lpr_get(data->dvabox_ipaddr, data->plateno_id, &dev_plateno_info);
		//else 
	}

	memset(g_register_group_id, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
	for (i = 0; i < dev_plateno_info.group_length; i++)	{
		g_register_group_id[i] = dev_plateno_info.group_list[i].id;
	}
	g_register_group_len = dev_plateno_info.group_length;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "LICENSE PLATE REGISTRATION", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 80;

    obj = nfui_nflabel_new_with_pango_font("PLATE NUMBER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    if (strlen(dev_plateno_info.plate)) obj = nfui_nflabel_new_text_box(dev_plateno_info.plate, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	else obj = nfui_nflabel_new_text_box(STR_REQUIRED_GROUP, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_input_label_event_handler);
	g_plateno_label = obj;

	pos_y += 42;

    obj = nfui_nflabel_new_with_pango_font("OWNER NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_nflabel_new_text_box(dev_plateno_info.name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_input_label_event_handler);
	g_name_label = obj;

	pos_y += 42;

    obj = nfui_nflabel_new_with_pango_font("PHONE NUMBER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_nflabel_new_text_box(dev_plateno_info.phone, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_input_label_event_handler);
	g_phone_label = obj;

	pos_y += 42;

    obj = nfui_nflabel_new_with_pango_font("MEMO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 220, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_nflabel_new_text_box(dev_plateno_info.memo, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_input_label_event_handler);
	g_memo_label = obj;

	pos_y += 60;

    obj = nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
		nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 40);		
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_register_group_event_handler);
	g_group_combo_obj = obj;

	for (i = 0; i < dev_group_cnt; i++) {
		nfui_combobox_append_data((NFCOMBOBOX*)obj, dev_group_info[i].name);
		}
	nfui_combobox_set_display_string((NFCOMBOBOX*)obj, lookup_string(STR_SELECT_GROUP));

	pos_y += 44;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, POPUP_SIZE_WID-220-40, 200);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	g_group_list_obj = obj;

	for (i = 0; i < dev_plateno_info.group_length; i++) {
		nfui_listbox_set_text_single_column(NF_LISTBOX(g_group_list_obj), dev_plateno_info.group_list[i].name);
	}

	pos_y += 202;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("ADD NEW GROUP", 220);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+220, pos_y);
	nfui_regi_post_event_callback(obj, post_device_add_group_button_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-180-20, pos_y);	
	nfui_regi_post_event_callback(obj, post_list_delete_button_event_handler);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2-174-4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2+4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return g_ret_code;
}
