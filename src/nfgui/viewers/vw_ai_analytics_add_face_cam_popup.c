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

#include "vw_ai_analytics_add_face_cam_popup.h"




////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (1000)
#define POPUP_SIZE_HEI          (920)


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_channel_combo = 0;
static NFOBJECT *g_captrue_btn = 0;
static NFOBJECT *g_captrue_view_label = 0;

static GdkPixbuf *g_jpeg_pixbuf = 0;

static gint g_image_crop_x = -1;
static gint g_image_crop_y = -1;
static guint g_image_crop_w = 0;
static guint g_image_crop_h = 0;

static CAM_FACE_DATA_T *g_face_data;


////////////////////////////////////////////////////////////////
//
// private interfaces
//

static gboolean post_play_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data);
static gboolean post_capture_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data);




////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		gint gap_x, gap_y;

		if (g_jpeg_pixbuf) g_object_unref(g_jpeg_pixbuf);
		g_jpeg_pixbuf = 0;

		g_image_crop_x = -1;
		g_image_crop_y = -1;
		g_image_crop_w = 0;
		g_image_crop_h = 0;

		nfui_nfobject_modify_bg(g_captrue_view_label, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
		nfui_signal_emit(g_captrue_view_label, GDK_EXPOSE, TRUE);

		nfui_nfobject_get_offset((NFOBJECT*)g_captrue_view_label, &gap_x, &gap_y);
		vsm_live_preview_start(1 << ch, (1920-POPUP_SIZE_WID)/2+gap_x, (1080-POPUP_SIZE_HEI)/2+gap_y, 960, 540);

		nfui_nfbutton_set_text((NFBUTTON*)g_captrue_btn, "PAUSE");
		nfui_regi_post_event_callback(g_captrue_btn, post_capture_button_event_handler);
		nfui_signal_emit(g_captrue_btn, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_play_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_channel_combo));
		gint gap_x, gap_y;

		if (g_jpeg_pixbuf) g_object_unref(g_jpeg_pixbuf);
		g_jpeg_pixbuf = 0;

		g_image_crop_x = -1;
		g_image_crop_y = -1;
		g_image_crop_w = 0;
		g_image_crop_h = 0;

		nfui_nfobject_modify_bg(g_captrue_view_label, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
		nfui_signal_emit(g_captrue_view_label, GDK_EXPOSE, TRUE);

		nfui_nfobject_get_offset((NFOBJECT*)g_captrue_view_label, &gap_x, &gap_y);
		vsm_live_preview_start(1 << ch, (1920-POPUP_SIZE_WID)/2+gap_x, (1080-POPUP_SIZE_HEI)/2+gap_y, 960, 540);

		nfui_nfbutton_set_text((NFBUTTON*)g_captrue_btn, "PAUSE");
		nfui_regi_post_event_callback(obj, post_capture_button_event_handler);
		nfui_signal_emit(g_captrue_btn, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_capture_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		void *jpeg_buffer = 0;		
		GInputStream *stream = 0;
		GdkPixbuf *npixbuf = 0;

		gint width, height;
		gint size;
		guint cur_time = 0;
		gint ret;

		gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(g_channel_combo));

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_jpeg_pixbuf) g_object_unref(g_jpeg_pixbuf);
		g_jpeg_pixbuf = 0;

		ret = nf_live_get_jpeg_snapshot(ch, &width, &height, &size, &jpeg_buffer, 0, 0, &cur_time);
		if (ret == 0) return FALSE;

		stream = g_memory_input_stream_new_from_data(jpeg_buffer, size, NULL);
		npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
		g_jpeg_pixbuf = gdk_pixbuf_scale_simple(npixbuf, 960, 540, GDK_INTERP_BILINEAR);

		nfui_nfbutton_set_text((NFBUTTON*)g_captrue_btn, "PLAY");
		nfui_regi_post_event_callback(obj, post_play_button_event_handler);
		nfui_signal_emit(g_captrue_btn, GDK_EXPOSE, TRUE);

		nfui_nfobject_modify_bg(g_captrue_view_label, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
		nfui_signal_emit(g_captrue_view_label, GDK_EXPOSE, TRUE);

		if (jpeg_buffer) free(jpeg_buffer);
		if (npixbuf) g_object_unref(npixbuf);
		if (stream) g_object_unref(stream);		
	}

	return FALSE;
}

static gboolean post_capture_view_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	gint gap_x, gap_y;
	gint8 dash_style[] = {5, 5};

	static gint press_x = -1;
	static gint press_y = -1;

	if (evt->type == GDK_EXPOSE)
    {
		if (g_jpeg_pixbuf)
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
			gdk_draw_pixbuf(drawable, gc, g_jpeg_pixbuf, 0, 0, gap_x, gap_y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
			nfui_nfobject_gc_unref(gc);
		}
    }
	else if (evt->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent;

		bevent = (GdkEventButton *)evt;
		if (!g_jpeg_pixbuf) return FALSE;
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
			gdk_draw_pixbuf(drawable, gc, g_jpeg_pixbuf, 0, 0, gap_x, gap_y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

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

		press_x = -1;
		press_y = -1;
	}		

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		GdkPixbuf *nbuf;
		FILE *fp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if ((g_jpeg_pixbuf != 0) && (g_image_crop_x != -1) && (g_image_crop_y != -1))
		{
			nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, g_image_crop_w, g_image_crop_h);
			gdk_pixbuf_copy_area(g_jpeg_pixbuf, g_image_crop_x, g_image_crop_y, g_image_crop_w, g_image_crop_h, nbuf, 0, 0);
			gdk_pixbuf_save(nbuf, "/tmp/face_cam_capture.jpg", "jpeg", NULL, "quality", "100", NULL);

			g_face_data->pbuf = gdk_pixbuf_scale_simple(nbuf, 240, 300, GDK_INTERP_BILINEAR);

			fp = fopen("/tmp/face_cam_capture.jpg", "r");

			if(fp){		
				fseek(fp, 0, SEEK_END);  
				g_face_data->image_len = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				g_face_data->image_data = imalloc(g_face_data->image_len);
				fread(g_face_data->image_data, 1, g_face_data->image_len, fp);
				fclose(fp);
			}

			g_object_unref(nbuf);
		}

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

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		if (g_jpeg_pixbuf) g_object_unref(g_jpeg_pixbuf);
		g_jpeg_pixbuf = 0;

		gtk_main_quit();
	}

	return FALSE;
}



/////////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_add_face_cam_popup_open(NFWINDOW *parent, CAM_FACE_DATA_T *data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;

    gchar strCh[STRING_SIZE_CAMTITLE+8];
    gchar strBuf[STRING_SIZE_CAMTITLE];


	g_face_data = data;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "Registration(CAM)", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 60;

    obj = nfui_nflabel_new_with_pango_font("Select a camera to register face photo.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 600, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 41;

	obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, 370, 40);		
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_channel_event_handler);
	g_channel_combo = obj;

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(strCh, 0x00, sizeof(strCh)); 
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);        
        var_get_camtitle(&strCh[j], (guint)i);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
    }

	pos_y += 80;

    obj = nfui_nflabel_new_with_pango_font("Click the 'PAUSE' button when the registered face appears on the camera.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 900, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("You can register a face after selecting the area by dragging the mouse on the video.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 900, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+35);

	pos_y += 71;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("PAUSE", 280);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_capture_button_event_handler);
	g_captrue_btn = obj;

	pos_y += 42;

    obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_set_size(obj, 960, 540);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	
	nfui_regi_post_event_callback(obj, post_capture_view_label_event_handler);
	g_captrue_view_label = obj;

	vsm_live_preview_start(0x1, (1920-POPUP_SIZE_WID)/2+pos_x, (1080-POPUP_SIZE_HEI)/2+pos_y, 960, 540);

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

	return 0;
}
