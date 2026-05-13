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
#include "nf_va_object_detector.h"

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

#include "viewers/vw_calendar.h"
#include "viewers/vw_timeline.h"
#include "viewers/vw_timeline_popup.h"
#include "viewers/objects/ixtimeline.h"
#include "scm.h"
#include "vsm.h"

#include "modules/ssm.h"

#include "vw_playback_main.h"
#include "vw_playback_statusbar.h"
#include "vw_live_statusbar.h"
#include "vw_timeline_deeplearning.h"
#include "vw_deeplearning_group_popup.h"
#include "vw_deeplearning_animate_popup.h"
#include "vw_deeplearning_generic_animate_popup.h"
#include "nf_api_play.h"
#include "ix_func.h"
#include "wrk.h"
#include "vw.h"
#include "dtf.h"
#include "uxm.h"
#include "stm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VW_DEEPLEARNING_GROUP_POPUP"


#define EVENT_FRAME_WIDTH			(176)
#define EVENT_FRAME_HEIGHT			(93)

#define MAX_GROUP_CNT				(6)


////////////////////////////////////////////////////////////////
//
// private variables
//

typedef struct _THUMBNAIL_PARAM_T {
	gint ch;
	gint idx;
    GTimeVal ttime;
} THUMBNAIL_PARAM_T;


static WRK_ID iwrk = 0; 

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_ch_obj = 0;
static NFOBJECT *g_group_object[MAX_GROUP_CNT] = {0, };
static NFOBJECT *g_focus_obj = 0;

static gint g_group_cnt = 0;
static GOBJECT_INFO_T g_group_info[MAX_GROUP_CNT] = {0, };
static GdkPixbuf *g_group_pbuf[MAX_GROUP_CNT] = {0, };

static gchar g_group_text[MAX_GROUP_CNT][64];



////////////////////////////////////////////////////////////////
//
// private interfaces
//

static void *_dup_pointer(void *data, int len)
{
	void *ptr = imalloc(len);
	memcpy(ptr, data, len);
	return ptr;
}

static int _proc_get_thumbnail(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
    THUMBNAIL_PARAM_T *param = (THUMBNAIL_PARAM_T*)pmsg->data;

    GTimeVal stime, etime, ftime;
	gint jpeg_w, jpeg_h;
	gint jpeg_size;
	guchar *jpeg_buffer = 0;
	void *send_buffer = 0;
	gboolean retVal = 0;

	stime.tv_sec = param->ttime.tv_sec;
	stime.tv_usec = param->ttime.tv_usec;

	etime.tv_sec = stime.tv_sec+1;
	etime.tv_usec = stime.tv_usec;

	// retVal = nf_play_get_thumbnail_jpeg(param->ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &jpeg_buffer, &ftime);

	if (retVal) {
		send_buffer = imalloc(jpeg_size+sizeof(gint));
		((gint*)send_buffer)[0] = jpeg_size;
		memcpy((gint*)send_buffer+1, jpeg_buffer, jpeg_size);
		evt_send_to_local(INFY_DEEPLEARNING_GROUP_THUMBNAIL, param->idx, 1, send_buffer);
	}
	else {
		evt_send_to_local(INFY_DEEPLEARNING_GROUP_THUMBNAIL, param->idx, 0, 0);
	}

	if (jpeg_buffer) free(jpeg_buffer);
	jpeg_buffer = 0;

	return 0;
}

static int _init_worker()
{
	iwrk = wrk_create_worker(_proc_get_thumbnail, 0);
	wrk_change_sleep_time(iwrk, 20000);
	return 0;
}

static gint _check_frame_margin_size(gint jpeg_w, gint jpeg_h, float *start_coord_x, float *start_coord_y, float *end_coord_x, float *end_coord_y)
{
	float margin_rate_w, margin_rate_h;
	float bbx_half, margin_half;

	margin_rate_w = (float)(THUMBNAIL_MARGIN_SIZE_W)/(float)jpeg_w;
	margin_rate_h = (float)(THUMBNAIL_MARGIN_SIZE_H)/(float)jpeg_h;

	if (*end_coord_x-*start_coord_x < margin_rate_w) 
	{
		bbx_half = (*end_coord_x-*start_coord_x)/2.0;
		margin_half = margin_rate_w/2.0;

		if (*start_coord_x + bbx_half - margin_half < 0.0) {
			*start_coord_x = 0;
			*end_coord_x = margin_rate_w;
		}
		else if (*start_coord_x + bbx_half + margin_half > 1.0) {
			*start_coord_x = 1.0 - margin_half;
			*end_coord_x = 1.0;
		}
		else {
			*start_coord_x += bbx_half - margin_half;
			*end_coord_x = *start_coord_x + margin_rate_w;
		} 
	}

	if (*end_coord_y-*start_coord_y < margin_rate_h) 
	{
		bbx_half = (*end_coord_y-*start_coord_y)/2.0;
		margin_half = margin_rate_h/2.0;

		if (*start_coord_y + bbx_half - margin_half < 0.0) {
			*start_coord_y = 0;
			*end_coord_y = margin_rate_h;
		}
		else if (*start_coord_y + bbx_half + margin_half > 1.0) {
			*start_coord_y = 1.0 - margin_half;
			*end_coord_y = 1.0;
		}
		else {
			*start_coord_y += bbx_half - margin_half;
			*end_coord_y = *start_coord_y + margin_rate_h;
		} 
	}

	return 0;
}

static gint _draw_event_detect_info(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint bg_color, font_color;

	gint buff_idx;

	gchar strBuf[128];
    gint gap_x, gap_y;

	gint valid_cnt = 0;

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if ((buff_idx < g_group_cnt) && (g_focus_obj == obj)) {
		bg_color = 439;
		font_color = 441;
	}
	else {
		bg_color = 435;
		font_color = 437;
	}

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, 176, 43);	

	if (buff_idx < g_group_cnt)
	{
		memset(strBuf, 0x00, sizeof(strBuf));

		if (g_group_info[buff_idx].evt_type == DEVT_TYPE_GNR) 
		{
			g_snprintf(strBuf, sizeof(strBuf), "%s", g_group_info[buff_idx].gnr.title);
		}
		else if (g_group_info[buff_idx].evt_type == DEVT_TYPE_DFT) 
		{
			g_snprintf(strBuf, sizeof(strBuf), "%s(%d%%)", lookup_string(g_group_info[buff_idx].dft.class_name), g_group_info[buff_idx].confidence);
		}

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 168);
		nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+5, gap_y+4, 168, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);

		memset(strBuf, 0x00, sizeof(strBuf));
		dtf_get_local_datetime((time_t)g_group_info[buff_idx].ttime.tv_sec, strBuf);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 168);
		nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+5, gap_y+24, 168, 20,
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);
	}
	else
	{
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "-", gap_x+5, gap_y+4, 168, 20, 
			nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "-", gap_x+5, gap_y+24, 168, 20, 
			nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);	
	}

    nfui_nfobject_gc_unref(gc);
    return 0;
}

static GdkPixbuf *_get_target_pixbuf(gint jpeg_w, gint jpeg_h, GdkPixbuf *org_pixbuf, gfloat coords[4])
{
	GdkDrawable *drawable = NULL;

    gfloat top_x = 0 , top_y = 0;
    gfloat bottom_x = 0, bottom_y = 0;

	gfloat obj_w, obj_h;
	gfloat objrate;

    GdkPixbuf *src_pixbuf = org_pixbuf;
	GdkPixbuf *copy_pixbuf = NULL;
	GdkPixbuf *dst_pixbuf = NULL;

	gint snap_width = gdk_pixbuf_get_width(src_pixbuf);
	gint snap_height = gdk_pixbuf_get_height(src_pixbuf);

	top_x = MAX(coords[0], 0);
	top_y = MAX(coords[1], 0);
	bottom_x = MIN(coords[2], 1);
	bottom_y = MIN(coords[3], 1);

	_check_frame_margin_size(jpeg_w, jpeg_h, &top_x, &top_y, &bottom_x, &bottom_y);

	obj_w = snap_width*(bottom_x-top_x);
	obj_h = snap_height*(bottom_y-top_y);

	if (EVENT_FRAME_WIDTH/obj_w >= EVENT_FRAME_HEIGHT/obj_h) objrate = EVENT_FRAME_HEIGHT/obj_h;
	else objrate = EVENT_FRAME_WIDTH/obj_w;

	copy_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, (gint)((bottom_x-top_x)*snap_width), (gint)((bottom_y-top_y)*snap_height));
	gdk_pixbuf_fill (copy_pixbuf, 0x000000ff);
	gdk_pixbuf_copy_area(src_pixbuf, (gint)(top_x*snap_width), (gint)(top_y*snap_height), 
			(gint)((bottom_x-top_x)*snap_width), (gint)((bottom_y-top_y)*snap_height), copy_pixbuf, 0, 0); 	

	dst_pixbuf = gdk_pixbuf_scale_simple(copy_pixbuf, (gint)(objrate*obj_w), (gint)(objrate*obj_h), GDK_INTERP_HYPER); 

	g_object_unref(copy_pixbuf);	
	return dst_pixbuf;
}

static gint _draw_event_detect_frame(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint bg_color, font_color;

	gint buff_idx;

    gint gap_x, gap_y;

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if ((buff_idx < g_group_cnt) && (g_focus_obj == obj)) {
		bg_color = 440;
		font_color = 441;
	}
	else {
		bg_color = 436;
		font_color = 437;
	}

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
	gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+45, 176, 93);

	if (g_group_pbuf[buff_idx])
	{
		gdk_draw_pixbuf(drawable, gc, g_group_pbuf[buff_idx], 0, 0, 
			gap_x+1+(EVENT_FRAME_WIDTH-gdk_pixbuf_get_width(g_group_pbuf[buff_idx]))/2, gap_y+45+(EVENT_FRAME_HEIGHT-gdk_pixbuf_get_height(g_group_pbuf[buff_idx]))/2, 
			-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
	}
	else if (strlen(g_group_text[buff_idx]))
	{
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, g_group_text[buff_idx], gap_x+2, gap_y+45, 176, 93, 
			nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
	}

    nfui_nfobject_gc_unref(gc);
    return 0;
}
 
static gint _draw_deeplearning_object_detect(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gint gap_x, gap_y;

	gint buff_idx;

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if ((buff_idx < g_group_cnt) && (g_focus_obj == obj))
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(438));
	else 
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
	gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, 180, 140);	

    _draw_event_detect_info(obj);
    _draw_event_detect_frame(obj);
    return 0;
}


////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_event_obj_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	switch (event->type) 
	{
		case GDK_EXPOSE:
		{
			_draw_deeplearning_object_detect(obj);
		}
		break;

		case GDK_BUTTON_PRESS:
		{		
			gint i, buff_idx;

			buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));
			if (buff_idx >= g_group_cnt) return FALSE;
			if (!g_group_pbuf[buff_idx]) return FALSE;

			g_message("%s, %d, buff_idx:%d", __FUNCTION__, __LINE__, buff_idx);

			g_focus_obj = obj;

			for (i = 0; i < MAX_GROUP_CNT; i++)
			{
				_draw_deeplearning_object_detect(g_group_object[i]);
			}

			if (g_group_info[buff_idx].evt_type == DEVT_TYPE_GNR) 
			{
				GENERIC_AOBJECT_INFO_T animate_info;

				memset(&animate_info, 0x00, sizeof(GENERIC_AOBJECT_INFO_T));
				animate_info.ch = g_group_info[buff_idx].ch;
				memcpy(&animate_info.ttime, &g_group_info[buff_idx].ttime, sizeof(GTimeVal));
				memcpy(&animate_info.coords, &g_group_info[buff_idx].coords, sizeof(gfloat)*4);
				strcpy(animate_info.caption, g_group_info[buff_idx].gnr.caption);
				strcpy(animate_info.title, g_group_info[buff_idx].gnr.title);
				strcpy(animate_info.description, g_group_info[buff_idx].gnr.description);
				VW_deepLearning_generic_animate_popup_show(&animate_info);
			}
			else if (g_group_info[buff_idx].evt_type == DEVT_TYPE_DFT) 
			{
				AOBJECT_INFO_T animate_info;

				memset(&animate_info, 0x00, sizeof(AOBJECT_INFO_T));
				animate_info.ch = g_group_info[buff_idx].ch;
				memcpy(&animate_info.ttime, &g_group_info[buff_idx].ttime, sizeof(GTimeVal));
				memcpy(&animate_info.coords, &g_group_info[buff_idx].coords, sizeof(gfloat)*4);
				strcpy(animate_info.rule_name, g_group_info[buff_idx].dft.rule_name);
				strcpy(animate_info.class_name, g_group_info[buff_idx].dft.class_name);
				animate_info.confidence = g_group_info[buff_idx].confidence;
				VW_deepLearning_animate_popup_show(&animate_info);		
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_wnd_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	gint i;

	switch (event->type) 
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
			GdkEventButton *bevent;		

			wrk_clear_job(iwrk);	

			for (i = 0; i < MAX_GROUP_CNT; i++) {
				if (g_group_pbuf[i]) g_object_unref(g_group_pbuf[i]);
				g_group_pbuf[i] = 0;
			}			

			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_POPUPWND, obj);	

			bevent = (GdkEventButton *)event;

			if (VW_Timeline_DeepLearning_IsShown()) {
				VW_Timeline_DeepLearning_clicked((gint)bevent->x_root, (gint)bevent->y_root);			
			}
		}
		break;
		case GDK_DELETE:
		{
			nfui_page_close(PGID_POPUPWND, obj);
			g_curwnd = 0;
		}
		break;
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	GdkPixbuf *pbuf;

	switch (event->type) 
	{
		case GDK_EXPOSE:
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, obj->width, obj->height);
			nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

			nfui_nfobject_gc_unref(gc);
		}
		break;

		case INFY_DEEPLEARNING_GROUP_THUMBNAIL:
		{
			gint idx = ((CMM_MESSAGE_T *)data)->param;
			gint *rcv_buffer = (gint*)((CMM_MESSAGE_T *)data)->data;
			gint i;

			gint jpeg_size;
			guchar *jpeg_buffer;

			GInputStream *stream = 0;
			GdkPixbuf *npixbuf = 0;

			if (idx == 0)
			{
				for (i = 0; i < MAX_GROUP_CNT; i++) 
				{
					memset(g_group_text[i], 0x00, sizeof(g_group_text[i]));
					if (i < g_group_cnt) strcpy(g_group_text[i], "Please wait...");

					if (g_group_pbuf[i]) g_object_unref(g_group_pbuf[i]);					
					g_group_pbuf[i] = 0;
					nfui_signal_emit(g_group_object[i], GDK_EXPOSE, TRUE);
				}				
			}

			if (rcv_buffer) 
			{
				jpeg_size = rcv_buffer[0];
				jpeg_buffer = rcv_buffer + 1;

				stream = g_memory_input_stream_new_from_data(jpeg_buffer, jpeg_size, NULL);
				npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

				if (g_group_pbuf[idx]) g_object_unref(g_group_pbuf[idx]);
				g_group_pbuf[idx] = 0;
				g_group_pbuf[idx] = _get_target_pixbuf(gdk_pixbuf_get_width(npixbuf), gdk_pixbuf_get_height(npixbuf), npixbuf, g_group_info[idx].coords);
			}
			else
			{
				memset(g_group_text[idx], 0x00, sizeof(g_group_text[idx]));
				strcpy(g_group_text[idx], "NO IMAGE");
			}

			nfui_signal_emit(g_group_object[idx], GDK_EXPOSE, TRUE);

			if (stream) g_object_unref(stream);
			if (npixbuf) g_object_unref(npixbuf);
		}
		break;

		case GDK_DELETE:
		{
			wrk_destroy_worker(iwrk);
			uxm_unreg_imsg_event(obj, INFY_DEEPLEARNING_GROUP_THUMBNAIL);

			nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, obj->width, obj->height);
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	gint i;

	if(!g_curwnd) return;

	if(nfui_nfobject_is_shown((NFOBJECT*)g_curwnd))
	{
		wrk_clear_job(iwrk);

		for (i = 0; i < MAX_GROUP_CNT; i++) {
			if (g_group_pbuf[i]) g_object_unref(g_group_pbuf[i]);
			g_group_pbuf[i] = 0;
		}

		nfui_nfobject_hide((NFOBJECT*)g_curwnd);
		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)g_curwnd);	
	}
	return FALSE;
}



/////////////////////////////////////////////////////////////////
//
// public interfaces
//

void VW_deepLearning_group_popup_open(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	guint pos_x, pos_y;
	gint i;


	_init_worker();


	win = (NFOBJECT*)nfui_nfwindow_new(parent, 1920-192-204-7, (972-936)/2, 204, 936);
	nfui_nfwindow_set_title(win, "DEEPLEARNING_GROUP_POPUP");
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_wnd_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 204, 936);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	uxm_reg_imsg_event(fixed, INFY_DEEPLEARNING_GROUP_THUMBNAIL);

	pos_y = 10;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(442));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 120, 34);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 12, 10);
	g_ch_obj = obj;

	pos_y += 34;

	for (i = 0; i < MAX_GROUP_CNT; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(659));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nfobject_set_size(obj, 180, 140);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(434)); 
		nfui_nffixed_put((NFFIXED*)fixed, obj, 12, pos_y);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_event_obj_event_handler);
		g_group_object[i] = obj;

		nfui_nfobject_set_data(obj, "EVENT_BUFF_IDX", GUINT_TO_POINTER(i));

		pos_y += (140+8); 
	}

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_nfobject_hide(win);
}

void VW_deepLearning_group_popup_show(GOBJECT_INFO_T *group_info, gint group_cnt)
{
	THUMBNAIL_PARAM_T thumbnail_param;

	gchar strBuf[64];
	gint i, cnt = 0;

	if(!g_curwnd) return;

	if (group_cnt == 1)
	{
		if (group_info[0].evt_type == DEVT_TYPE_GNR) 
		{
			GENERIC_AOBJECT_INFO_T animate_info;

			memset(&animate_info, 0x00, sizeof(GENERIC_AOBJECT_INFO_T));
			animate_info.ch = group_info[0].ch;
			memcpy(&animate_info.ttime, &group_info[0].ttime, sizeof(GTimeVal));
			memcpy(&animate_info.coords, &group_info[0].coords, sizeof(gfloat)*4);
			strcpy(animate_info.caption, group_info[0].gnr.caption);
			strcpy(animate_info.title, group_info[0].gnr.title);
			strcpy(animate_info.description, group_info[0].gnr.description);
			VW_deepLearning_generic_animate_popup_show(&animate_info);
		}
		else if (group_info[0].evt_type == DEVT_TYPE_DFT) 
		{
			AOBJECT_INFO_T animate_info;

			memset(&animate_info, 0x00, sizeof(AOBJECT_INFO_T));
			animate_info.ch = group_info[0].ch;
			memcpy(&animate_info.ttime, &group_info[0].ttime, sizeof(GTimeVal));
			memcpy(&animate_info.coords, &group_info[0].coords, sizeof(gfloat)*4);
			strcpy(animate_info.rule_name, group_info[0].dft.rule_name);
			strcpy(animate_info.class_name, group_info[0].dft.class_name);
			animate_info.confidence = group_info[0].confidence;
			VW_deepLearning_animate_popup_show(&animate_info);		
		}
	}
	else
	{
		g_focus_obj = 0;

		g_group_cnt = group_cnt;

		memset(g_group_info, 0x00, sizeof(GOBJECT_INFO_T)*MAX_GROUP_CNT);
		memcpy(g_group_info, group_info, sizeof(GOBJECT_INFO_T)*group_cnt);

		for (i = 0; i < MAX_GROUP_CNT; i++)
		{
			memset(g_group_text[i], 0x00, sizeof(g_group_text[i]));

			if (i < group_cnt)
			{
				strcpy(g_group_text[i], "Please wait...");

				memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
				thumbnail_param.ch = group_info[i].ch;
				thumbnail_param.idx = i;
				thumbnail_param.ttime.tv_sec = g_group_info[i].ttime.tv_sec;
				thumbnail_param.ttime.tv_usec = g_group_info[i].ttime.tv_usec;
				wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
			}
		}

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "CH%d", group_info[0].ch+1);
		nfui_nflabel_set_text((NFLABEL*)g_ch_obj, strBuf);

		if(!nfui_nfobject_is_shown((NFOBJECT*)g_curwnd))
		{
			nfui_nfobject_show((NFOBJECT*)g_curwnd);
			nfui_page_open(PGID_POPUPWND, (NFOBJECT*)g_curwnd, ssm_get_cur_id(NULL));	
		}
	}
}

void VW_deepLearning_group_popup_hide()
{
	gint i;

	if(!g_curwnd) return;

	if(nfui_nfobject_is_shown((NFOBJECT*)g_curwnd))
	{
		wrk_clear_job(iwrk);

		for (i = 0; i < MAX_GROUP_CNT; i++) {
			if (g_group_pbuf[i]) g_object_unref(g_group_pbuf[i]);
			g_group_pbuf[i] = 0;
		}

		nfui_nfobject_hide((NFOBJECT*)g_curwnd);
		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)g_curwnd);	
	}
}

gboolean VW_deepLearning_group_popup_is_shown()
{
	if(!g_curwnd) return;

	return nfui_nfobject_is_shown((NFOBJECT*)g_curwnd);
}

gboolean VW_deepLearning_group_popup_clicked(gint x, gint y)
{
	gint win_x, win_y;
	gint off_x, off_y;
	gint i;

	if(!g_curwnd) return FALSE;

	nfui_nfobject_get_window_pos((NFOBJECT*)g_curwnd, &win_x, &win_y);

	if ((x >= win_x) && (x <= win_x+((NFOBJECT*)g_curwnd)->width)
		&& (y >= win_y) && (y <= win_y+((NFOBJECT*)g_curwnd)->height))
	{
		for (i = 0; i < MAX_GROUP_CNT; i++)
		{			
			nfui_nfobject_get_offset(g_group_object[i], &off_x, &off_y);
			off_x += win_x;
			off_y += win_y;

			if ((x >= off_x) && (x <= off_x+g_group_object[i]->width)
				&& (y >= off_y) && (y <= off_y+g_group_object[i]->height))
			{
				nfui_signal_emit(g_group_object[i], GDK_BUTTON_PRESS, TRUE);
				break;
			}
		}

		return TRUE;
	}

	return FALSE;
}
