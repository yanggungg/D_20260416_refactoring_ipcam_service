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
#include "vw_ai_analytics_add_face_popup.h"
#include "vw_ai_analytics_add_face_usb_popup.h"
#include "vw_ai_analytics_add_face_cam_popup.h"

#include "nf_api_dlva.h"



typedef struct _UPLOAD_FACE_DATA_T {
    GdkPixbuf *pbuf;
    gchar *image_data;
    gint image_len;
} UPLOAD_FACE_DATA_T;


////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (720)
#define POPUP_SIZE_HEI          (620)

#define STR_SELECT_GROUP		"Please select a group."

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_picture_label_obj;
static NFOBJECT *g_face_name_obj;
static NFOBJECT *g_group_combo_obj;
static NFOBJECT *g_group_list_obj;

static UPLOAD_FACE_DATA_T g_upload_face_data;
static AiAnalysisActData g_analysis_data;
static gint g_ret_code = 0;

static FACE_DEV_DATA_T *g_face_dev_data = 0;

static fr_group_info *g_dev_group_info = 0;
static gint g_dev_group_cnt = 0;

static gint g_register_group_id[MAX_AIBOX_DB_GROUP_SIZE];
static gint g_register_group_len = 0;




////////////////////////////////////////////////////////////////
//
// private interfaces
//

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

static gboolean post_usb_upload_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		USB_FACE_DATA_T tmp_usb_face_data;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_hide(g_curwnd);

		memset(&tmp_usb_face_data, 0x00, sizeof(USB_FACE_DATA_T));
		vw_ai_analytics_add_face_usb_popup_open(g_curwnd, &tmp_usb_face_data);

		if (tmp_usb_face_data.pbuf)
		{
			if (g_upload_face_data.pbuf) g_object_unref(g_upload_face_data.pbuf);
			g_upload_face_data.pbuf = 0;		

			if (g_upload_face_data.image_data) ifree(g_upload_face_data.image_data);
			g_upload_face_data.image_data = 0;

			g_upload_face_data.pbuf = tmp_usb_face_data.pbuf;
			g_upload_face_data.image_data = tmp_usb_face_data.image_data;
			g_upload_face_data.image_len = tmp_usb_face_data.image_len;
		}

		nfui_nfobject_show(g_curwnd);
	}

	return FALSE;
}

static gboolean post_cam_upload_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		CAM_FACE_DATA_T tmp_cam_face_data;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_hide(g_curwnd);

		memset(&tmp_cam_face_data, 0x00, sizeof(CAM_FACE_DATA_T));
		vw_ai_analytics_add_face_cam_popup_open(g_curwnd, &tmp_cam_face_data);

		if (tmp_cam_face_data.pbuf)
		{
			if (g_upload_face_data.pbuf) g_object_unref(g_upload_face_data.pbuf);
			g_upload_face_data.pbuf = 0;		

			if (g_upload_face_data.image_data) ifree(g_upload_face_data.image_data);
			g_upload_face_data.image_data = 0;

			g_upload_face_data.pbuf = tmp_cam_face_data.pbuf;
			g_upload_face_data.image_data = tmp_cam_face_data.image_data;
			g_upload_face_data.image_len = tmp_cam_face_data.image_len;
		}

		nfui_nfobject_show(g_curwnd);
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

		retVal = nf_api_fr_group_add(g_face_dev_data->dvabox_ipaddr, strTemp);
		if (retVal != 0)
		{
			if (g_dev_group_info) free(g_dev_group_info);
			g_dev_group_info = 0;
			g_dev_group_cnt = 0;

			nfui_combobox_remove_all((NFCOMBOBOX*)g_group_combo_obj);
			nfui_combobox_set_display_string((NFCOMBOBOX*)g_group_combo_obj, lookup_string(STR_SELECT_GROUP));

			if (g_analysis_data.dvabox_active) dev_group_info = nf_api_fr_group_list_get(g_face_dev_data->dvabox_ipaddr, &dev_group_cnt);
			else dev_group_info = nf_api_aicam_fr_group_list_get(g_face_dev_data->ch, &dev_group_cnt);

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
		face_info info;
	jpeg_image_data register_imgage;
		gint i;
	gint retVal = 0;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (!g_upload_face_data.pbuf) {
			nftool_mbox(g_curwnd, "NOTICE", "Please register a face photo.", NFTOOL_MB_OK);
			return FALSE;
		}

		if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_face_name_obj))) {
			nftool_mbox(g_curwnd, "NOTICE", "Please enter a name to register with the face photo.", NFTOOL_MB_OK);
			return FALSE;
		}

		memset(&info, 0x00, sizeof(face_info));
		info.id = g_face_dev_data->face_id;
		g_snprintf(info.name, sizeof(info.name)-1, "%s", nfui_nflabel_get_text((NFLABEL*)g_face_name_obj));

		for (i = 0; i < g_register_group_len; i++) {
			info.group_list[i].id = g_register_group_id[i];
		}
		info.group_length = g_register_group_len;

		memset(&register_imgage, 0x00, sizeof(jpeg_image_data));
		register_imgage.memory = g_upload_face_data.image_data;
		register_imgage.size = g_upload_face_data.image_len;

		if (g_face_dev_data->face_id >= 0) 
		{
			if (g_analysis_data.dvabox_active) {
				retVal = nf_api_aibox_fr_modify(g_face_dev_data->dvabox_ipaddr, &info, &register_imgage);
			}
			else {
				retVal = nf_api_aicam_fr_face_modify(g_face_dev_data->ch, register_imgage, nfui_nflabel_get_text((NFLABEL*)g_face_name_obj), g_face_dev_data->face_id);
			}

			if (retVal == 0) nftool_mbox(g_curwnd, "NOTICE", "Registered the edited face information.", NFTOOL_MB_OK);
			else nftool_mbox(g_curwnd, "NOTICE", "Failed to register face information.", NFTOOL_MB_OK);			
		}
		else 
		{
			if (g_analysis_data.dvabox_active) {
				retVal = nf_api_fr_face_add(g_face_dev_data->dvabox_ipaddr, register_imgage, nfui_nflabel_get_text((NFLABEL*)g_face_name_obj), g_register_group_id, g_register_group_len);
			}
			else {
				retVal = nf_api_aicam_fr_face_add(g_face_dev_data->ch, register_imgage, nfui_nflabel_get_text((NFLABEL*)g_face_name_obj), g_register_group_id, g_register_group_len);
			}

			if (retVal > 0) nftool_mbox(g_curwnd, "NOTICE", "Registered new face information.", NFTOOL_MB_OK);
			else nftool_mbox(g_curwnd, "NOTICE", "Failed to register face information.", NFTOOL_MB_OK);
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

static gboolean post_picture_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

        nfui_nfobject_gc_unref(gc);

		if (g_upload_face_data.pbuf) _draw_face_pixbuf_image(obj, g_upload_face_data.pbuf);
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

		if (g_upload_face_data.pbuf) g_object_unref(g_upload_face_data.pbuf);
		g_upload_face_data.pbuf = 0;		

		if (g_upload_face_data.image_data) ifree(g_upload_face_data.image_data);
		g_upload_face_data.image_data = 0;

		gtk_main_quit();
	}

	return FALSE;
}



////////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_add_face_popup_open(NFWINDOW *parent, FACE_DEV_DATA_T *data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	fr_group_info *dev_group_info = 0;
	gint dev_group_cnt = 0;

	face_info dev_face_info;
	jpeg_image_data face_jpeg_data;

    GInputStream *stream = 0;
    GdkPixbuf *npixbuf = 0;
    GdkPixbuf *pbuf = NULL;

	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;

	guint lc_size[] = {370, };
	guint li_size_w, li_size_h;
	gchar *list_text;


	memset(&g_upload_face_data, 0x00, sizeof(UPLOAD_FACE_DATA_T));
	g_ret_code = -1;


	g_face_dev_data = data;

	memset(&g_analysis_data, 0x00, sizeof(AiAnalysisActData));
	DAL_get_aianalysis_act_data(&g_analysis_data, data->ch);

	if (g_analysis_data.dvabox_active) dev_group_info = nf_api_fr_group_list_get(data->dvabox_ipaddr, &dev_group_cnt);
	else dev_group_info = nf_api_aicam_fr_group_list_get(data->ch, &dev_group_cnt);

    g_dev_group_info = dev_group_info;
    g_dev_group_cnt = dev_group_cnt;  

	memset(&dev_face_info, 0x00, sizeof(face_info));  
	memset(&face_jpeg_data, 0x00, sizeof(jpeg_image_data));  
	if (data->face_id != -1)
	{
		if (g_analysis_data.dvabox_active) nf_api_aibox_fr_get(data->dvabox_ipaddr, data->face_id, &dev_face_info, &face_jpeg_data);
		else nf_api_aicam_fr_get(data->ch, data->face_id, &dev_face_info, &face_jpeg_data);

		stream = g_memory_input_stream_new_from_data(face_jpeg_data.memory, face_jpeg_data.size, NULL);
		npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
		g_upload_face_data.pbuf = gdk_pixbuf_scale_simple(npixbuf, 240, 300, GDK_INTERP_BILINEAR);
		g_upload_face_data.image_data = imalloc(face_jpeg_data.size);
		memcpy(g_upload_face_data.image_data, face_jpeg_data.memory, face_jpeg_data.size);
		g_upload_face_data.image_len = face_jpeg_data.size;

		if (face_jpeg_data.memory) free(face_jpeg_data.memory);
		if (stream) g_object_unref(stream);
		if (npixbuf) g_object_unref(npixbuf);		
	}

	memset(g_register_group_id, 0x00, sizeof(gint)*MAX_AIBOX_DB_GROUP_SIZE);
	for (i = 0; i < dev_face_info.group_length; i++)	{
		g_register_group_id[i] = dev_face_info.group_list[i].id;
	}
	g_register_group_len = dev_face_info.group_length;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "FACE REGISTRATION", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 80;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(206));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 250, 300);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_picture_label_event_handler);
	g_picture_label_obj = obj;
	
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("Registration(USB)", 250);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+310);
	nfui_regi_post_event_callback(obj, post_usb_upload_button_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("Registration(CAM)", 250);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+350);
	nfui_regi_post_event_callback(obj, post_cam_upload_button_event_handler);

	pos_x += 300;
	pos_y += 0;

    obj = nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

	pos_y += 40;

	obj = nfui_nflabel_new_text_box(dev_face_info.name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 370, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_face_name_label_event_handler);
	g_face_name_obj = obj;

	pos_y += 70;

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
	nfui_nfobject_set_size(obj, 370, 200);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	g_group_list_obj = obj;

	for (i = 0; i < dev_face_info.group_length; i++) {
		nfui_listbox_set_text_single_column(NF_LISTBOX(g_group_list_obj), dev_face_info.group_list[i].name);
	}

	pos_y += 202;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+190, pos_y);	
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
