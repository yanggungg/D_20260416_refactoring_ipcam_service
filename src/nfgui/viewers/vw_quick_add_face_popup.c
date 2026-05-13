/*
 * vw_timeline_deeplearning.c
 * 	- timeline dlva viewer
 *	- dependencies :
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 11, 2018
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
#include "vw_quick_add_face_popup.h"

#include "nf_api_dlva.h"





////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (1580)
#define POPUP_SIZE_HEI          (600)

#define STR_SELECT_GROUP		"Please select a group."

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_channel_obj;
static NFOBJECT *g_org_capture_label;
static NFOBJECT *g_crop_capture_label;
static NFOBJECT *g_login_button;
static NFOBJECT *g_engine_label;
static NFOBJECT *g_face_name_obj;
static NFOBJECT *g_group_combo_obj;
static NFOBJECT *g_group_list_obj;
static NFOBJECT *g_add_group_obj;
static NFOBJECT *g_del_group_obj;

static GdkPixbuf *g_org_capture_pixbuf = 0;
static GdkPixbuf *g_crop_capture_pixbuf = 0;

static AiAnalysisActData g_org_analysis_data[GUI_CHANNEL_CNT];
static AiAnalysisActData g_analysis_data[GUI_CHANNEL_CNT];

static fr_group_info *g_dev_group_info = 0;
static gint g_dev_group_cnt = 0;

static gint g_register_group_id[MAX_AIBOX_DB_GROUP_SIZE];
static gint g_register_group_len = 0;

static gint g_image_crop_x = -1;
static gint g_image_crop_y = -1;
static guint g_image_crop_w = 0;
static guint g_image_crop_h = 0;

static gint g_changed_db_flag = 0;


////////////////////////////////////////////////////////////////
//
// private interfaces
//

static void _init_group(void)
{
	memset(g_register_group_id, 0x00, sizeof(g_register_group_id));
}

static gint _changed_channel_engine(gint ch)
{
	gint i;
	gchar strBuf[128];

	if (g_dev_group_info) free(g_dev_group_info);
	g_dev_group_info = 0;
	g_dev_group_cnt = 0;

	nfui_combobox_remove_all((NFCOMBOBOX*)g_group_combo_obj);

	if (g_analysis_data[ch].dvabox_active) 
	{
        memset(strBuf, 0x00, sizeof(strBuf));
        snprintf(strBuf, sizeof(strBuf), "%s (%s)", lookup_string("AI BOX"), g_analysis_data[ch].dvabox_mac);
		nfui_nflabel_set_text((NFLABEL*)g_engine_label, strBuf);

		g_dev_group_info = nf_api_fr_group_list_get(g_analysis_data[ch].dvabox_ipaddr, &g_dev_group_cnt);

		if (strlen(g_analysis_data[ch].dvabox_id) && strlen(g_analysis_data[ch].dvabox_pass))
		{
            if (nf_api_aibox_login_check(g_analysis_data[ch].dvabox_ipaddr, g_analysis_data[ch].dvabox_id, g_analysis_data[ch].dvabox_pass) == 0)
            {
				nfui_nfobject_disable(g_login_button);
				nfui_nfobject_enable(g_org_capture_label);
				nfui_nfobject_enable(g_face_name_obj);
				nfui_nfobject_enable(g_group_combo_obj);
				nfui_nfobject_enable(g_group_list_obj);
				nfui_nfobject_enable(g_add_group_obj);
				nfui_nfobject_enable(g_del_group_obj);
            }
            else
            {
				nfui_nfobject_enable(g_login_button);
				nfui_nfobject_disable(g_org_capture_label);
				nfui_nfobject_disable(g_face_name_obj);
				nfui_nfobject_disable(g_group_combo_obj);
				nfui_nfobject_disable(g_group_list_obj);
				nfui_nfobject_disable(g_add_group_obj);
				nfui_nfobject_disable(g_del_group_obj);
            }
		}
		else
		{
			nfui_nfobject_enable(g_login_button);
			nfui_nfobject_disable(g_org_capture_label);
			nfui_nfobject_disable(g_face_name_obj);
			nfui_nfobject_disable(g_group_combo_obj);
			nfui_nfobject_disable(g_group_list_obj);
			nfui_nfobject_disable(g_add_group_obj);
			nfui_nfobject_disable(g_del_group_obj);
		}
	}
	else if (g_analysis_data[ch].dvacam_active) 
	{
		nfui_nflabel_set_text((NFLABEL*)g_engine_label, "AI CAM");

		g_dev_group_info = nf_api_aicam_fr_group_list_get(ch, &g_dev_group_cnt);
		nfui_nfobject_disable(g_login_button);
		nfui_nfobject_enable(g_org_capture_label);
		nfui_nfobject_enable(g_face_name_obj);
		nfui_nfobject_enable(g_group_combo_obj);
		nfui_nfobject_enable(g_group_list_obj);
		nfui_nfobject_enable(g_add_group_obj);
		nfui_nfobject_enable(g_del_group_obj);
	}
	else
	{
		if (g_analysis_data[ch].builtin_active) nfui_nflabel_set_text((NFLABEL*)g_engine_label, "BUILT-IN AI");
		else if (g_analysis_data[ch].classic_active) nfui_nflabel_set_text((NFLABEL*)g_engine_label, "CLASSIC VA");
		else nfui_nflabel_set_text((NFLABEL*)g_engine_label, "NONE");

		nfui_nfobject_disable(g_login_button);
		nfui_nfobject_disable(g_org_capture_label);
		nfui_nfobject_disable(g_face_name_obj);
		nfui_nfobject_disable(g_group_combo_obj);
		nfui_nfobject_disable(g_group_list_obj);
		nfui_nfobject_disable(g_add_group_obj);
		nfui_nfobject_disable(g_del_group_obj);
	}

    for (i = 0; i < g_dev_group_cnt; i++) {
        nfui_combobox_append_data((NFCOMBOBOX*)g_group_combo_obj, g_dev_group_info[i].name);
    }
	nfui_combobox_set_display_string((NFCOMBOBOX*)g_group_combo_obj, lookup_string("Please select a group."));	
	return 0;
}

static gint _draw_face_pixbuf_image(NFOBJECT *obj, GdkPixbuf *g_face_pixbuf)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	GdkPixbuf *nbuf;

	gint gap_x, gap_y;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

	nbuf = gdk_pixbuf_scale_simple(g_face_pixbuf, obj->width-8, obj->height-8, GDK_INTERP_BILINEAR);
	gdk_draw_pixbuf(drawable, gc, nbuf, 0, 0, gap_x+4, gap_y+4, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

	g_object_unref(nbuf);

	nfui_nfobject_gc_unref(gc);

	return 0;
}


////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_org_capture_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	gint gap_x, gap_y;
	gint8 dash_style[] = {5, 5};

	static gint press_x = -1;
	static gint press_y = -1;

	if (evt->type == GDK_EXPOSE)
    {
		if (g_org_capture_pixbuf)
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
			gdk_draw_pixbuf(drawable, gc, g_org_capture_pixbuf, 0, 0, gap_x, gap_y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
			nfui_nfobject_gc_unref(gc);
		}
    }
	else if (evt->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent;

		bevent = (GdkEventButton *)evt;
		if (!g_org_capture_pixbuf) return FALSE;
		if (bevent->button == MOUSE_RIGTH_BUTTON) return FALSE;

		press_x = (gint)bevent->x;
		press_y = (gint)bevent->y;

		g_image_crop_x = -1;
		g_image_crop_y = -1;
		g_image_crop_w = 0;
		g_image_crop_h = 0;		
	}
	else if (evt->type == GDK_MOTION_NOTIFY)
	{
		GdkEventMotion *mevent;

		gint start_x, start_y;
		gint end_x, end_y;

		mevent = (GdkEventMotion*)evt;	
		if (mevent->state & GDK_BUTTON1_MASK)
		{
			if (press_x == -1) return FALSE;
			if (press_y == -1) return FALSE;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
			gdk_draw_pixbuf(drawable, gc, g_org_capture_pixbuf, 0, 0, gap_x, gap_y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

			start_x = ((gint)mevent->x > press_x ? press_x : (gint)mevent->x);
			start_y = ((gint)mevent->y > press_y ? press_y : (gint)mevent->y);
			end_x = ((gint)mevent->x > press_x ? (gint)mevent->x : press_x);
			end_y = ((gint)mevent->y > press_y ? (gint)mevent->y : press_y);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(403));
			gdk_gc_set_line_attributes(gc, 4, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_ROUND);
			gdk_gc_set_dashes (gc, 0, dash_style, G_N_ELEMENTS(dash_style));
			gdk_draw_rectangle(drawable, gc, FALSE, start_x, start_y, end_x-start_x, end_y-start_y);

			nfui_nfobject_gc_unref(gc);

			g_image_crop_x = start_x-gap_x;
			g_image_crop_y = start_y-gap_y;
			g_image_crop_w = end_x-start_x;
			g_image_crop_h = end_y-start_y;
		}
	}
	else if (evt->type == GDK_BUTTON_RELEASE)
	{
		GdkEventButton *bevent;

		bevent = (GdkEventButton *)evt;
		if (bevent->button == MOUSE_RIGTH_BUTTON) return FALSE;

		if ((g_org_capture_pixbuf != 0) && (g_image_crop_x != -1) && (g_image_crop_y != -1))
		{
			if (g_crop_capture_pixbuf) g_object_unref(g_crop_capture_pixbuf);	
			g_crop_capture_pixbuf = 0;

			g_crop_capture_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, g_image_crop_w, g_image_crop_h);
			gdk_pixbuf_copy_area(g_org_capture_pixbuf, g_image_crop_x, g_image_crop_y, g_image_crop_w, g_image_crop_h, g_crop_capture_pixbuf, 0, 0);
			gdk_pixbuf_save(g_crop_capture_pixbuf, "/tmp/face_cam_capture.jpg", "jpeg", NULL, "quality", "100", NULL);
			nfui_signal_emit(g_crop_capture_label, GDK_EXPOSE, TRUE);
		}

		press_x = -1;
		press_y = -1;
	}		

	return FALSE;
}

static gboolean post_crop_capture_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable = NULL;
        GdkGC *gc;

        gint gap_x, gap_y;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(200)));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y, obj->width, obj->height);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(392)));
		gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
		gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, obj->width, obj->height);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(206)));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(COLOR_IDX(200)), drawable, gc, "PHOTO", gap_x+4, gap_y+20, obj->width-8, obj->height-40, 
			nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(COLOR_IDX(206)), NFALIGN_CENTER, 0);

		if (g_crop_capture_pixbuf) _draw_face_pixbuf_image(obj, g_crop_capture_pixbuf);

        nfui_nfobject_gc_unref(gc);
    }

	return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint i, idx;

		idx = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		_changed_channel_engine(idx);

		nfui_signal_emit(g_login_button, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_engine_label, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_org_capture_label, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_face_name_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_group_combo_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_group_list_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_add_group_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_del_group_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_aibox_login_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_channel_obj));
        gint i;

        gchar aibox_userid[128];
        gchar aibox_pass[128];
        gchar strBuf[128];

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

                if ((g_analysis_data[ch].dvabox_active) && (strcmp(g_analysis_data[ch].dvabox_mac, g_analysis_data[ch].dvabox_mac) == 0))
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
					g_changed_db_flag = 1;
                }
            }

			_changed_channel_engine(ch);
			nfui_signal_emit(g_login_button, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_engine_label, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_org_capture_label, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_face_name_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_group_combo_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_group_list_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_add_group_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_del_group_obj, GDK_EXPOSE, TRUE);			
        }
        else
        {
            nftool_mbox(g_curwnd, "NOTICE", "LOGIN FAIL", NFTOOL_MB_OK);
        }
	}

    return FALSE;
}

static gboolean post_face_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
		gchar *strBuf;
		gchar *strTemp;

		gint win_x, win_y; 
		gint gap_x, gap_y; 

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
		strTemp = VirtualKey_Open(g_curwnd, strBuf, gap_x, gap_y+44, 64, VKEY_MULTIKEYPD);

		if (strTemp) 
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
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

		fr_group_info *dev_group_info = 0;
		gint dev_group_cnt = 0;
		gchar *strTemp;
		gint ch;

		gint win_x, win_y; 
		gint gap_x, gap_y;
		
		gint i;
		gint retVal;

		ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_channel_obj));

		nfui_nfobject_get_window_pos((NFOBJECT*)obj, &win_x, &win_y);		
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);		

		gap_x += win_x;
		gap_y += win_y;

		strTemp = VirtualKey_Open(g_curwnd, "", gap_x, gap_y+44, 64, VKEY_NORMAL|VKEY_MULTIKEYPD);
		if (!strTemp) return FALSE;

		retVal = nf_api_fr_group_add(g_analysis_data[ch].dvabox_ipaddr, strTemp);
		if (retVal != 0)
		{
			if (g_dev_group_info) free(g_dev_group_info);
			g_dev_group_info = 0;
			g_dev_group_cnt = 0;

			nfui_combobox_remove_all((NFCOMBOBOX*)g_group_combo_obj);
			nfui_combobox_set_display_string((NFCOMBOBOX*)g_group_combo_obj, lookup_string(STR_SELECT_GROUP));

			if (g_analysis_data[ch].dvabox_active) dev_group_info = nf_api_fr_group_list_get(g_analysis_data[ch].dvabox_ipaddr, &dev_group_cnt);
			else dev_group_info = nf_api_aicam_fr_group_list_get(ch, &dev_group_cnt);

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
			g_message("TEST: %s(%d) / %d", g_dev_group_info[idx].name, g_dev_group_info[idx].id, g_register_group_id[i]);
			if (g_dev_group_info[idx].id == g_register_group_id[i]) return FALSE;
		}
		
		g_register_group_id[g_register_group_len] = g_dev_group_info[idx].id;
		g_register_group_len++;

		nfui_listbox_set_text_single_column(NF_LISTBOX(g_group_list_obj), g_dev_group_info[idx].name);
		nfui_signal_emit(g_group_list_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_list_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint group_list[MAX_AIBOX_DB_GROUP_SIZE];
		gint i, idx;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_group_list_obj);
		if (idx < 0) return FALSE;

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
	jpeg_image_data register_imgage;
	FILE *fp;
	gint ch;
	gint retVal;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (!g_crop_capture_pixbuf) {
			nftool_mbox(g_curwnd, "NOTICE", "Please register a face photo.", NFTOOL_MB_OK);
			return FALSE;
		}

		if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_face_name_obj))) {
			nftool_mbox(g_curwnd, "NOTICE", "Please enter a name to register with the face photo.", NFTOOL_MB_OK);
			return FALSE;
		}

		ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_channel_obj));

		memset(&register_imgage, 0x00, sizeof(jpeg_image_data));

		fp = fopen("/tmp/face_cam_capture.jpg", "r");
		if(fp){		
			fseek(fp, 0, SEEK_END);  
			register_imgage.size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			register_imgage.memory = imalloc(register_imgage.size);
			fread(register_imgage.memory, 1, register_imgage.size, fp);
			fclose(fp);
		}	

		if (g_analysis_data[ch].dvabox_active) {
			retVal = nf_api_fr_face_add(g_analysis_data[ch].dvabox_ipaddr, register_imgage, nfui_nflabel_get_text((NFLABEL*)g_face_name_obj), g_register_group_id, g_register_group_len);
		}
		else {
			retVal = nf_api_aicam_fr_face_add(ch, register_imgage, nfui_nflabel_get_text((NFLABEL*)g_face_name_obj), g_register_group_id, g_register_group_len);
		}

		if (register_imgage.memory) ifree(register_imgage.memory);
		register_imgage.memory = 0;

		if (retVal > 0) nftool_mbox(g_curwnd, "NOTICE", "Registered new face information.", NFTOOL_MB_OK);
		else nftool_mbox(g_curwnd, "NOTICE", "Failed to register face information.", NFTOOL_MB_OK);

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

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{
        GdkDrawable *drawable = NULL;
        GdkGC *gc;

        gint gap_x, gap_y;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(392)));
		gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
		gdk_draw_rectangle(drawable, gc, FALSE, gap_x+20+16*52+30, 52, POPUP_SIZE_WID-20-16*52-30-6, obj->height-56);

        nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		if (g_changed_db_flag)
		{
			DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);
		}

		if (g_dev_group_info) free(g_dev_group_info);
		g_dev_group_info = 0;
		g_dev_group_cnt = 0;

		if (g_org_capture_pixbuf) g_object_unref(g_org_capture_pixbuf);	
		g_org_capture_pixbuf = 0;

		if (g_crop_capture_pixbuf) g_object_unref(g_crop_capture_pixbuf);	
		g_crop_capture_pixbuf = 0;

		gtk_main_quit();
	}

	return FALSE;
}



////////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_quick_add_face_popup_open(NFWINDOW *parent, gint ch, void *jpeg_buff, gint jpeg_len, gboolean jpeg_chk)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

    GInputStream *stream = 0;
    GdkPixbuf *npixbuf = 0;

	gchar strCh[STRING_SIZE_CAMTITLE+8];

	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;

	guint lc_size[] = {370, };
	guint li_size_w, li_size_h;
	gchar *list_text;


	g_changed_db_flag = 0;

	g_image_crop_x = -1;
	g_image_crop_y = -1;
	g_image_crop_w = 0;
	g_image_crop_h = 0;

    memset(g_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
    memset(g_org_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
	memset(g_register_group_id, 0x00, sizeof(g_register_group_id));

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_aianalysis_act_data(&g_analysis_data[i], i);
    }
    g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);    	
	if (jpeg_chk)
	{
		stream = g_memory_input_stream_new_from_data(jpeg_buff, jpeg_len, NULL);
		npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
		g_org_capture_pixbuf = gdk_pixbuf_scale_simple(npixbuf, 16*52, 9*49, GDK_INTERP_BILINEAR);
	}
	else g_org_capture_pixbuf = gdk_pixbuf_scale_simple(jpeg_buff, 16*52, 9*49, GDK_INTERP_BILINEAR);


	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Registration(CAM)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, POPUP_SIZE_WID-16*52-90, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 16*52+60, 4);

    pos_x = 20;
    pos_y = 45;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(206));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nfobject_set_size(obj, 16*52, 9*49);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_org_capture_label_event_handler);
	g_org_capture_label = obj;

    obj = nfui_nflabel_new_with_pango_font("You can register a face after selecting the area by dragging the mouse on the video.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 16*52, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+9*49+10);	

    pos_x += (16*52+60);
	pos_y = 70;

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 370, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
	g_channel_obj = obj;

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(strCh, 0x00, sizeof(strCh)); 
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
        var_get_camtitle(&strCh[j], (guint)i);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
    }	
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, ch);

	pos_y += 44;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 370, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    g_engine_label = obj;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("Login", 220);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+380, pos_y);
	nfui_regi_post_event_callback(obj, post_aibox_login_button_event_handler);
	g_login_button = obj;    

	pos_y += 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(206));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 250, 300);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+20);
	nfui_regi_post_event_callback(obj, post_crop_capture_label_event_handler);
	g_crop_capture_label = obj;

	pos_x += 270;

    obj = nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

	pos_y += 40;

	obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 370, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_face_name_label_event_handler);
	g_face_name_obj = obj;

	pos_y += 60;

    obj = nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("ADD NEW GROUP", 220);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+150, pos_y);
	nfui_regi_post_event_callback(obj, post_device_add_group_button_event_handler);
	g_add_group_obj = obj;

	pos_y += 41;

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, 370, 40);		
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_register_group_event_handler);
	g_group_combo_obj = obj;

	pos_y += 44;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, 370, 120);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_group_list_obj = obj;

	pos_y += 122;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+190, pos_y);	
	nfui_regi_post_event_callback(obj, post_list_delete_button_event_handler);
	g_del_group_obj = obj;

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 16*52+30+(POPUP_SIZE_WID-16*52-30)/2-174-4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 16*52+30+(POPUP_SIZE_WID-16*52-30)/2+4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	_changed_channel_engine(ch);

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return 0;
}
