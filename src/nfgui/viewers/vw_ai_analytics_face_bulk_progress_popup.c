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
#include "vw_ai_analytics_face_bulk_progress_popup.h"

#include "nf_api_dlva.h"




////////////////////////////////////////////////////////////////
//
// private variables
//

typedef struct _REGIST_RESULT_T {
	gint index;
	gint status;		// 0 : ready,  1 : ing, 2:cmpl
	gint result;		// 0 : fail, 1 : success
} REGIST_RESULT_T;


#define POPUP_SIZE_WID		    (1080)
#define POPUP_SIZE_HEI          (850)

#define REGIST_DISP_CNT			(7)

#define IMAGE_SIZE_W			(90)
#define IMAGE_SIZE_H			(90)


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_regist_no_obj[REGIST_DISP_CNT];
static NFOBJECT *g_regist_status_obj[REGIST_DISP_CNT];
static NFOBJECT *g_regist_image_obj[REGIST_DISP_CNT];
static NFOBJECT *g_regist_property_obj[REGIST_DISP_CNT];
static NFOBJECT *g_regist_total_obj;
static NFOBJECT *g_regist_success_obj;
static NFOBJECT *g_regist_fail_obj;
static NFOBJECT *g_ok_obj;

static GdkPixbuf *g_regist_pixbuf[REGIST_DISP_CNT] = {0, };


static FR_UPLOAD_FILE_INFO *g_fr_upload_list = 0;
static gint g_fr_upload_cnt = 0;

static REGIST_RESULT_T *g_fr_upload_result = 0;



////////////////////////////////////////////////////////////////
//
// private interfaces
//

static int _bulk_upload_callback(FR_BULK_UPLOAD_CALLBACK_MESSAGE data)
{
	REGIST_RESULT_T *result;

	if (data.type == FR_BULK_TYPE_CONFIRM) {
		evt_send_to_local(INFY_FACE_BULK_UPLOAD_RESULT, 1, 0, 0);
	}
	else {
		g_message("%s, %d, %d", __FUNCTION__, __LINE__, data.index);

		result = imalloc(sizeof(REGIST_RESULT_T));
		result->index = data.index;
		result->result = data.result;
		evt_send_to_local(INFY_FACE_BULK_UPLOAD_RESULT, 0, 1, result);
	}
	return 0;
}

static gint _start_face_bulk_upload(gint ch, guint ipaddr, gint face_cnt, gchar *fullpath)
{
	AiAnalysisActData analysis_data;

	gint retval;
	gchar strBuf[1024];

	memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
	DAL_get_aianalysis_act_data(&analysis_data, ch);

	if (analysis_data.dvabox_active) retval = nf_api_fr_face_bulk_upload(ipaddr, fullpath, face_cnt, &g_fr_upload_list, &g_fr_upload_cnt, _bulk_upload_callback);
	else retval = nf_api_aicam_fr_face_bulk_upload(ch, fullpath, face_cnt, &g_fr_upload_list, &g_fr_upload_cnt, _bulk_upload_callback);

	if (retval != 0)
	{
		if (retval == FR_BULK_FILE_NOT_FOUND) {
			nftool_mbox(g_curwnd, "NOTICE", "No face file was found within the selected directory.", NFTOOL_MB_OK);
		}
		else if (retval == FR_BULK_FILE_AMOUNT_OVER) {
			memset(strBuf, 0x00, sizeof(strBuf));
			snprintf(strBuf, sizeof(strBuf)-1, 
				lookup_string("Impossible to proceed beyond the maximum number of faces that can be registered.\n\nNumber of available registration: %d\nNumber of photos in directory: %d"),
				face_cnt, g_fr_upload_cnt);
			nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OK);
		}

		if (g_fr_upload_list) free(g_fr_upload_list);
		g_fr_upload_list = 0;
		g_fr_upload_cnt = 0;

		return -1;
	}

	g_fr_upload_result = imalloc(sizeof(REGIST_RESULT_T) * g_fr_upload_cnt);
	return 0;
}

static gint _update_uploading_file_info(gint disp_idx, gint regist_idx)
{
	gchar strBuf[256];
	GdkPixbuf *pbImage;

	if (g_regist_pixbuf[disp_idx]) g_object_unref(g_regist_pixbuf[disp_idx]);
	g_regist_pixbuf[disp_idx] = 0;

	if ((regist_idx >= 0) && (regist_idx < g_fr_upload_cnt))
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		sprintf(strBuf, "%d", regist_idx+1);
		nfui_nflabel_set_text((NFLABEL*)g_regist_no_obj[disp_idx], strBuf);

		if (g_fr_upload_result[regist_idx].status == 2)
		{
			if (g_fr_upload_result[regist_idx].result == 0) nfui_nflabel_set_text((NFLABEL*)g_regist_status_obj[disp_idx], "Success");
			else nfui_nflabel_set_text((NFLABEL*)g_regist_status_obj[disp_idx], "Fail");			
		}
		else if (g_fr_upload_result[regist_idx].status == 1)
		{
			nfui_nflabel_set_text((NFLABEL*)g_regist_status_obj[disp_idx], "Registering");
		}

		pbImage = gdk_pixbuf_new_from_file(g_fr_upload_list[regist_idx].file_path, NULL);
		if (pbImage) {
			g_regist_pixbuf[disp_idx] = gdk_pixbuf_scale_simple(pbImage, IMAGE_SIZE_W-8, IMAGE_SIZE_H-8, GDK_INTERP_BILINEAR);
			g_object_unref(pbImage);
		}

		memset(strBuf, 0x00, sizeof(strBuf));
		snprintf(strBuf, sizeof(strBuf)-1, "%s\n%s", g_fr_upload_list[regist_idx].group, g_fr_upload_list[regist_idx].name);
		nfui_nflabel_set_text((NFLABEL*)g_regist_property_obj[disp_idx], strBuf);
	}
	else
	{
		nfui_nflabel_set_text((NFLABEL*)g_regist_no_obj[disp_idx], "");
		nfui_nflabel_set_text((NFLABEL*)g_regist_status_obj[disp_idx], "");
		nfui_nflabel_set_text((NFLABEL*)g_regist_property_obj[disp_idx], "");				
	}

	nfui_signal_emit(g_regist_no_obj[disp_idx], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_regist_status_obj[disp_idx], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_regist_image_obj[disp_idx], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_regist_property_obj[disp_idx], GDK_EXPOSE, TRUE);
	return 0;
}

static gint _update_upload_total_cnt(gint cnt)
{
	gchar strBuf[128];

	if (strlen(nfui_nflabel_get_text((NFLABEL*)g_regist_total_obj))) return 0;

	memset(strBuf, 0x00, sizeof(strBuf));
	sprintf(strBuf, "%d", cnt);
	nfui_nflabel_set_text((NFLABEL*)g_regist_total_obj, strBuf);
	nfui_signal_emit(g_regist_total_obj, GDK_EXPOSE, TRUE);
	return 0;
}

static gint _insert_file_success_list(gchar *file_name)
{
	nfui_listbox_set_text_single_column(NF_LISTBOX(g_regist_success_obj), file_name);
	nfui_signal_emit(g_regist_success_obj, GDK_EXPOSE, TRUE);
	return 0;
}

static gint _insert_file_fail_list(gchar *file_name)
{
	nfui_listbox_set_text_single_column(NF_LISTBOX(g_regist_fail_obj), file_name);
	nfui_signal_emit(g_regist_fail_obj, GDK_EXPOSE, TRUE);
	return 0;
}





////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_regist_image_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable = NULL;
        GdkGC *gc;

        gint gap_x, gap_y;
		gint i;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

		for (i = 0; i < REGIST_DISP_CNT; i++)
		{
			if (g_regist_image_obj[i] == obj)
			{
				if (g_regist_pixbuf[i])	{
					gdk_draw_pixbuf(drawable, gc, g_regist_pixbuf[i], 0, 0, gap_x+4, gap_y+4, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
				}
			}
		}

        nfui_nfobject_gc_unref(gc);
    }

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
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

	}
	else if (evt->type == INFY_FACE_BULK_UPLOAD_RESULT)
	{
		REGIST_RESULT_T *regist_result = ((REGIST_RESULT_T*)((CMM_MESSAGE_T *)data)->data);
		gint cmpl = ((CMM_MESSAGE_T *)data)->param;
		gint i;

		if (cmpl == 0)
		{
			gint index = regist_result->index;
			gint result = regist_result->result;
			gchar strBuf[256];

			g_fr_upload_result[index].index = index;
			g_fr_upload_result[index].status = 2;
			g_fr_upload_result[index].result = result;
			if (index+1 < g_fr_upload_cnt) g_fr_upload_result[index+1].status = 1;

			for (i = 0; i < REGIST_DISP_CNT; i++) {
				_update_uploading_file_info(i, index+i-REGIST_DISP_CNT+2);
			}
			_update_upload_total_cnt(g_fr_upload_cnt);

			memset(strBuf, 0x00, sizeof(strBuf));
			snprintf(strBuf, sizeof(strBuf)-1, "%s/%s", g_fr_upload_list[index].group, g_fr_upload_list[index].name);
			if (result == 0) _insert_file_success_list(strBuf);
			else _insert_file_fail_list(strBuf);
		}
		else
		{
			nftool_mbox(g_curwnd, "NOTICE", "Uploaded all the photos in the selected directory.", NFTOOL_MB_OK);

			nfui_nfobject_enable(g_ok_obj);
			nfui_signal_emit(g_ok_obj, GDK_EXPOSE, TRUE);
		}
	}
	else if (evt->type == GDK_DELETE)
	{
		gint i;

		for (i = 0; i < REGIST_DISP_CNT; i++)
		{
			if (g_regist_pixbuf[i]) g_object_unref(g_regist_pixbuf[i]);
			g_regist_pixbuf[i] = 0;
		}

		if (g_fr_upload_list) free(g_fr_upload_list);
		g_fr_upload_list = 0;
		g_fr_upload_cnt = 0;

		if (g_fr_upload_result) ifree(g_fr_upload_result);
		g_fr_upload_result = 0;

		uxm_unreg_imsg_event(obj, INFY_FACE_BULK_UPLOAD_RESULT);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}



//////////////////////////////¤º//////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_face_bulk_progress_popup_open(NFWINDOW *parent, gint ch, guint ipaddr, gint face_cnt, gchar *fullpath)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tmp_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	
	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;

	guint progress_tbl_w[4] = {80, 180, 180, 180};
	guint result_tbl_w[] = {370};

	guint lc_size[] = {350, };
	guint li_size_w, li_size_h;	


	if (_start_face_bulk_upload(ch, ipaddr, face_cnt, fullpath) == -1) return -1;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "BULK UPLOAD", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 60;

	progress_tbl_w[0] = 80;
	progress_tbl_w[1] = 180;
	progress_tbl_w[2] = 360;

    tbl = (NFOBJECT*)nfui_nftable_new(3, 1, 1, 1, progress_tbl_w, 40);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);	

    obj = nfui_nflabel_new_with_pango_font("NO.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("PROPERTY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 2, 0);

    pos_y += 41;

	progress_tbl_w[0] = 80;
	progress_tbl_w[1] = 180;
	progress_tbl_w[2] = IMAGE_SIZE_W;
	progress_tbl_w[3] = 270;

    tbl = (NFOBJECT*)nfui_nftable_new(4, REGIST_DISP_CNT, 1, 1, progress_tbl_w, IMAGE_SIZE_H);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);	

	for (i = 0; i < REGIST_DISP_CNT; i++)
	{
	    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);	
		g_regist_no_obj[i] = obj;

	    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i);
		g_regist_status_obj[i] = obj;

	    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i);
		nfui_regi_post_event_callback(obj, post_regist_image_label_event_handler);		
		g_regist_image_obj[i] = obj;

	    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 3, i);
		g_regist_property_obj[i] = obj;
	}


    pos_x = 680;
    pos_y = 60;

    tbl = (NFOBJECT*)nfui_nftable_new(1, 1, 1, 1, result_tbl_w, 40);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);	

    obj = nfui_nflabel_new_with_pango_font("RESULT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);	

	pos_y += 41;

    tbl = (NFOBJECT*)nfui_nftable_new(1, 1, 1, 1, result_tbl_w, 640);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)main_fixed, tbl, pos_x, pos_y);		

    tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_show(tmp_fixed);
    nfui_nftable_attach((NFTABLE*)tbl, tmp_fixed, 0, 0);

    pos_x = 10;
    pos_y = 10;

    obj = nfui_nflabel_new_with_pango_font("TOTAL UPLOAD NUMBERS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 240, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x, pos_y);

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 110, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x+240, pos_y);
	g_regist_total_obj = obj;

	pos_y += 70;

    obj = nfui_nflabel_new_with_pango_font("SUCCESS FILE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 350, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x, pos_y);

	pos_y += 40;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 34);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 350, 220);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x, pos_y);
	g_regist_success_obj = obj;

	pos_y += 240;

    obj = nfui_nflabel_new_with_pango_font("FAILURE FILE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 350, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x, pos_y);

	pos_y += 40;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 34);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 350, 220);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, pos_x, pos_y);
	g_regist_fail_obj = obj;

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (POPUP_SIZE_WID-174)/2, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);
	g_ok_obj = obj;

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	uxm_reg_imsg_event(main_fixed, INFY_FACE_BULK_UPLOAD_RESULT);	
	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);
	return 0;
}
