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
#include "viewers/objects/nftable.h"

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
#include "vw_deeplearning_face_animate_popup.h"
#include "nf_api_play.h"
#include "nf_api_dlva.h"
#include "ix_func.h"
#include "wrk.h"
#include "vw.h"
#include "dtf.h"
#include "uxm.h"
#include "stm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VW_DEEPLEARNING_FACE_ANIMATE_POPUP"


#define MAX_ANIMATE_CNT				(10)

#define EVENT_FRAME_WIDTH			(160)
#define EVENT_FRAME_HEIGHT			(220)



////////////////////////////////////////////////////////////////
//
// private variables
//

typedef struct _THUMBNAIL_PARAM_T {
	gint ch;
    GTimeVal ttime;
} THUMBNAIL_PARAM_T;


static WRK_ID iwrk = 0; 

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_animate_fixed = NULL;

static NFOBJECT *g_ch_obj;
static NFOBJECT *g_time_obj;
static NFOBJECT *g_rule_obj;
static NFOBJECT *g_grp_obj;
static NFOBJECT *g_face_info1_obj;
static NFOBJECT *g_face_info2_obj;

static GtkWidget *g_gtkwindow = 0;
static GtkWidget *g_gtkimage = 0;

static FACE_AOBJECT_INFO_T g_animate_info = {0, };
static GdkPixbuf *g_animate_pbuf[MAX_ANIMATE_CNT] = {0, };
static GdkPixbuf *g_snap_pbuf = 0;
static GdkPixbuf *g_regist_pbuf = 0;

static gint g_rcv_idx = -1;
static gint g_rcv_frame_cnt = -1;

static gchar g_animate_text[64];



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
    gint i;

    GTimeVal stime, etime, ftime;
	gint jpeg_w, jpeg_h;
	gint jpeg_size;
	guchar *jpeg_buffer = 0;
	void *send_buffer = 0;
	gboolean retVal;

	stime.tv_sec = param->ttime.tv_sec;
	stime.tv_usec = param->ttime.tv_usec;

    for (i = 0; i < MAX_ANIMATE_CNT; i++)
	{
		etime.tv_sec = stime.tv_sec+1;
		etime.tv_usec = stime.tv_usec;

		retVal = nf_play_get_thumbnail_jpeg(param->ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &jpeg_buffer, &ftime);

		if (retVal) {
			send_buffer = imalloc(jpeg_size+sizeof(gint));
			((gint*)send_buffer)[0] = jpeg_size;
			memcpy((gint*)send_buffer+1, jpeg_buffer, jpeg_size);
			evt_send_to_local(INFY_DEEPLEARNING_ANIMATE_THUMBNAIL, i, 1, send_buffer);
		}
		else {
			evt_send_to_local(INFY_DEEPLEARNING_ANIMATE_THUMBNAIL, i, 0, 0);
		}

		if (jpeg_buffer) free(jpeg_buffer);
		jpeg_buffer = 0;

		stime.tv_sec += 1;
	}

	return 0;
}

static int _init_worker()
{
	iwrk = wrk_create_worker(_proc_get_thumbnail, 0);
	wrk_change_sleep_time(iwrk, 30000);
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

static gint _set_animate_widget(gint win_x, gint win_y)
{
	GdkPixbufSimpleAnim *simpleani = 0;
	gint i;

	gtk_image_clear(g_gtkimage);
	simpleani = gdk_pixbuf_simple_anim_new(500, 281, 3);

	for (i = 0; i < MAX_ANIMATE_CNT; i++)
	{
		if (g_animate_pbuf[i]) {
			gdk_pixbuf_simple_anim_add_frame(simpleani, g_animate_pbuf[i]);
			if (g_animate_pbuf[i]) g_object_unref(g_animate_pbuf[i]);
			g_animate_pbuf[i] = 0;
		}
	}

	gdk_pixbuf_simple_anim_set_loop(simpleani, TRUE);
	gtk_image_set_from_animation(g_gtkimage, simpleani);

	if (simpleani) g_object_unref(simpleani);
	simpleani = 0;

	return 0;
}

static gint _draw_snap_frame(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));
	gdk_draw_rectangle(drawable, gc, FALSE, 522, 105, EVENT_FRAME_WIDTH, EVENT_FRAME_HEIGHT);
	gdk_draw_rectangle(drawable, gc, FALSE, 522+EVENT_FRAME_WIDTH+4, 105, EVENT_FRAME_WIDTH, EVENT_FRAME_HEIGHT);

	if (strlen(g_animate_text))
	{
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(200), drawable, gc, g_animate_text, 18, 44, 500, 281, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(389), NFALIGN_CENTER, 0);
	}

	if (g_snap_pbuf)
	{
		gdk_draw_pixbuf(drawable, gc, g_snap_pbuf, 0, 0,
			522+(EVENT_FRAME_WIDTH-gdk_pixbuf_get_width(g_snap_pbuf))/2, 105+(EVENT_FRAME_HEIGHT-gdk_pixbuf_get_height(g_snap_pbuf))/2, 
			-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
	}

	if (g_regist_pbuf)
	{
		gdk_draw_pixbuf(drawable, gc, g_regist_pbuf, 0, 0,
			522+EVENT_FRAME_WIDTH+4+(EVENT_FRAME_WIDTH-gdk_pixbuf_get_width(g_regist_pbuf))/2, 105+(EVENT_FRAME_HEIGHT-gdk_pixbuf_get_height(g_regist_pbuf))/2, 
			-1, -1, GDK_RGB_DITHER_NONE, 0, 0);			
	}

    nfui_nfobject_gc_unref(gc);
    return 0;
}


////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_playback_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		SecurityData secdata;
		gboolean use_dl = 0;
		gchar *user_id = NULL;

		guint ch_mask = 0;
		GTimeVal stimeval = {0, };

		DAL_get_use_double_login(&use_dl);

		if (use_dl && !ssm_is_admin())
		{
			if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
			if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return FALSE;
		}
		else
		{
			DAL_get_security_data(&secdata);
			if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
		}

		// only show when user log-on
		user_id = ssm_get_cur_id(NULL);
		if(!strlen(user_id)) return FALSE;
		if(!ssm_check_access_auth(USR_AUTH_SEARCH)) return FALSE;

		VW_deepLearning_group_popup_hide();
		VW_deepLearning_face_animate_popup_hide();

		ch_mask = 1 << g_animate_info.ch;

		stimeval.tv_sec = g_animate_info.ttime.tv_sec-1;
		stimeval.tv_usec = 0;

		VW_Live_StatusBar_Hide();									

		vsm_live_stop();
		vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_DLVA_TL);
		vsm_playback_start(ch_mask, stimeval, PLAYBACK_NORMAL);
	}
	
	return FALSE;
}

static gboolean post_wnd_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch (event->type) 
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
			GdkEventButton *bevent;
			gboolean retVal = FALSE;

			if (g_snap_pbuf) g_object_unref(g_snap_pbuf);
			g_snap_pbuf = 0;

			if (g_regist_pbuf) g_object_unref(g_regist_pbuf);
			g_regist_pbuf = 0;

			gtk_image_clear(g_gtkimage);
			gtk_widget_hide(g_gtkwindow);

			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_POPUPWND, obj);	

			bevent = (GdkEventButton*)event;

			if (VW_deepLearning_group_popup_is_shown())
			{
				retVal = VW_deepLearning_group_popup_clicked((gint)bevent->x_root, (gint)bevent->y_root);
				if (!retVal) VW_deepLearning_group_popup_hide();
			}
			else
			{
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

			_draw_snap_frame(obj);
		}
		break;

		case INFY_DEEPLEARNING_ANIMATE_THUMBNAIL:
		{
			gint idx = ((CMM_MESSAGE_T *)data)->param;
			gint *rcv_buffer = (gint*)((CMM_MESSAGE_T *)data)->data;

			gint jpeg_size;
			guchar *jpeg_buffer;

			GInputStream *event_stream = 0;
			GdkPixbuf *event_npixbuf = 0;

			GInputStream *regist_stream = 0;
			GdkPixbuf *regist_npixbuf = 0;

			jpeg_image_data recv_jpeg_data;
			AiAnalysisActData analysis_data;

			gint win_x, win_y;

			g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, idx);

			if (g_rcv_idx+1 != idx) return FALSE;

			if (g_animate_pbuf[idx]) g_object_unref(g_animate_pbuf[idx]);
			g_animate_pbuf[idx] = 0;

			if (rcv_buffer) 
			{
				jpeg_size = rcv_buffer[0];
				jpeg_buffer = rcv_buffer + 1;

				event_stream = g_memory_input_stream_new_from_data(jpeg_buffer, jpeg_size, NULL);
				event_npixbuf = gdk_pixbuf_new_from_stream(event_stream, NULL, NULL);

				g_animate_pbuf[idx] = gdk_pixbuf_scale_simple(event_npixbuf, 500, 281, GDK_INTERP_HYPER);

				if (idx == MAX_ANIMATE_CNT-1)
				{
					if (g_snap_pbuf) g_object_unref(g_snap_pbuf);
					g_snap_pbuf = 0;
					g_snap_pbuf = _get_target_pixbuf(gdk_pixbuf_get_width(event_npixbuf), gdk_pixbuf_get_height(event_npixbuf), event_npixbuf, g_animate_info.coords);

					if (g_regist_pbuf) g_object_unref(g_regist_pbuf);
					g_regist_pbuf = 0;

					memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
					DAL_get_aianalysis_act_data(&analysis_data, g_animate_info.ch);

					if (analysis_data.dvabox_active) recv_jpeg_data = nf_api_fr_get_image(analysis_data.dvabox_ipaddr, g_animate_info.face_id, 0);
					else recv_jpeg_data = nf_api_aicam_fr_get_image(g_animate_info.ch, g_animate_info.face_id, 0);

					if (recv_jpeg_data.size)
					{
						regist_stream = g_memory_input_stream_new_from_data(recv_jpeg_data.memory, recv_jpeg_data.size, NULL);
						regist_npixbuf = gdk_pixbuf_new_from_stream(regist_stream, NULL, NULL);
						g_regist_pbuf = gdk_pixbuf_scale_simple(regist_npixbuf, 160, 160, GDK_INTERP_BILINEAR);

						if (recv_jpeg_data.memory) free(recv_jpeg_data.memory);
					}
				}

				if (event_stream) g_object_unref(event_stream);
				if (event_npixbuf) g_object_unref(event_npixbuf);

				if (regist_stream) g_object_unref(regist_stream);
				if (regist_npixbuf) g_object_unref(regist_npixbuf);

				g_rcv_frame_cnt++;
			}

			if ((idx == MAX_ANIMATE_CNT-1) && (g_rcv_frame_cnt == 0))
			{
				memset(g_animate_text, 0x00, sizeof(g_animate_text));
				strcpy(g_animate_text, "NO IMAGE");
				nfui_signal_emit(g_animate_fixed, GDK_EXPOSE, TRUE);
			}
			else if ((idx == MAX_ANIMATE_CNT-1) && (g_rcv_frame_cnt > 0))
			{
				memset(g_animate_text, 0x00, sizeof(g_animate_text));
				nfui_signal_emit(g_animate_fixed, GDK_EXPOSE, TRUE);

				nfui_nfobject_get_offset((NFOBJECT*)g_curwnd, &win_x, &win_y);
				_set_animate_widget(win_x, win_y);
				gtk_widget_show(g_gtkwindow);
			}

			g_rcv_idx = idx;
		}
		break;

		case GDK_DELETE:
		{
			wrk_destroy_worker(iwrk);
			uxm_unreg_imsg_event(obj, INFY_DEEPLEARNING_ANIMATE_THUMBNAIL);

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
	nfui_nfobject_hide(top);
	return FALSE;
}


//////////////////////////////��//////////////////////////////////
//
// public interfaces
//

void VW_deepLearning_face_animate_popup_open(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GtkWidget *gtkwindow = NULL;
	GtkWidget *gtkimage;

	guint table_w[] = {130, 820/2-130, 130, 820/2-130};

	guint pos_x, pos_y;
	gint i;


	_init_worker();


	win = (NFOBJECT*)nfui_nfwindow_new(parent, 1920-192-204-7-860-7, (972-424)/2, 860, 424);
	nfui_nfwindow_set_title(win, "DEEPLEARNING_GROUP_POPUP");
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_wnd_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 860, 424);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);
	g_animate_fixed = fixed;

	uxm_reg_imsg_event(fixed, INFY_DEEPLEARNING_ANIMATE_THUMBNAIL);

	pos_y = 10;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(442));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 160, 34);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 18, 10);
	g_ch_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 320, 34);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win->width-320-18, 10);
	g_time_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, EVENT_FRAME_WIDTH*2+4, 30);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 522, 44);
	g_face_info1_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, EVENT_FRAME_WIDTH*2+4, 30);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 522, 75);
	g_face_info2_obj = obj;

    tbl = (NFOBJECT*)nfui_nftable_new(4, 1, 1, 1, table_w, 29);
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)fixed, tbl, 18, 333);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RULE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(387));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 1, 0);
	g_rule_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(387));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 2, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("-", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 3, 0);
	g_grp_obj = obj;

    obj = nftool_normal_button_create_popup_type2("PLAYBACK", 180);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)fixed, obj, win->width-198, win->height-52);
    nfui_regi_post_event_callback(obj, post_playback_button_event_handler);	

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_nfobject_hide(win);

	gtkwindow = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_size_request(gtkwindow, 500, 281);
	gtk_window_move(gtkwindow, win->x+18, win->y+44);
	g_gtkwindow = gtkwindow;

	gtkimage = gtk_image_new();
	gtk_widget_show(gtkimage);
	gtk_container_add(GTK_CONTAINER(gtkwindow), gtkimage);	
	g_gtkimage = gtkimage;	
}

void VW_deepLearning_face_animate_popup_show(FACE_AOBJECT_INFO_T *animate_info)
{
	THUMBNAIL_PARAM_T thumbnail_param;

	gchar strBuf[128];
	gint win_x, win_y;

	if (!g_curwnd) return;
	if (nfui_nfobject_is_shown((NFOBJECT*)g_curwnd)) return;

	g_rcv_idx = -1;
	g_rcv_frame_cnt = 0;

	memset(g_animate_text, 0x00, sizeof(g_animate_text));
	strcpy(g_animate_text, "Please wait...");

	memcpy(&g_animate_info, animate_info, sizeof(FACE_AOBJECT_INFO_T));
	g_message("%s, %d, ch:%d, time:%ld", __FUNCTION__, __LINE__, animate_info->ch, animate_info->ttime.tv_sec);

	if (VW_deepLearning_group_popup_is_shown()) {
		win_x = 1920-192-204-7-860-7;
		win_y = (972-424)/2;
	}
	else {
		win_x = 1920-192-860-7;
		win_y = (972-424)/2;
	}

	memset(strBuf, 0x00, sizeof(strBuf));
	sprintf(strBuf, "CH""%d", animate_info->ch+1);
	nfui_nflabel_set_text((NFLABEL*)g_ch_obj, strBuf);

	memset(strBuf, 0x00, sizeof(strBuf));
	dtf_get_local_datetime(animate_info->ttime.tv_sec, strBuf);
	nfui_nflabel_set_text((NFLABEL*)g_time_obj, strBuf);

	memset(strBuf, 0x00, sizeof(strBuf));
	nfui_nflabel_set_text((NFLABEL*)g_rule_obj, "FACE RECOGNITION");

	memset(strBuf, 0x00, sizeof(strBuf));
	sprintf(strBuf, "%s", animate_info->group);
	nfui_nflabel_set_text((NFLABEL*)g_grp_obj, strBuf);		

	if (animate_info->age)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		if (strlen(animate_info->name)) sprintf(strBuf, "%s(""%d""%)", animate_info->name, animate_info->confidence);
		else sprintf(strBuf, "%s", "Unregistered");
		nfui_nflabel_set_text((NFLABEL*)g_face_info1_obj, strBuf);	

		memset(strBuf, 0x00, sizeof(strBuf));
		snprintf(strBuf, sizeof(strBuf), "%s:%s, %s:%d", lookup_string("Gender"), animate_info->gender, lookup_string("Age"), animate_info->age);
		nfui_nflabel_set_text((NFLABEL*)g_face_info2_obj, strBuf);	
	}
	else
	{
		nfui_nflabel_set_text((NFLABEL*)g_face_info1_obj, "");

		memset(strBuf, 0x00, sizeof(strBuf));
		if (strlen(animate_info->name)) sprintf(strBuf, "%s(""%d""%)", animate_info->name, animate_info->confidence);
		else sprintf(strBuf, "%s", "Unregistered");
		nfui_nflabel_set_text((NFLABEL*)g_face_info2_obj, strBuf);	
	}

	nfui_nfobject_move((NFOBJECT*)g_curwnd, win_x, win_y);

	nfui_nfobject_show((NFOBJECT*)g_curwnd);
	nfui_page_open(PGID_POPUPWND, (NFOBJECT*)g_curwnd, ssm_get_cur_id(NULL));	

	gtk_window_move(g_gtkwindow, win_x+18, win_y+44);

	memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
	thumbnail_param.ch = animate_info->ch;
	thumbnail_param.ttime.tv_sec = animate_info->ttime.tv_sec-MAX_ANIMATE_CNT+1;
	thumbnail_param.ttime.tv_usec = animate_info->ttime.tv_usec;
	wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
}

void VW_deepLearning_face_animate_popup_hide()
{
	if(!g_curwnd) return;

	if(nfui_nfobject_is_shown((NFOBJECT*)g_curwnd))
	{
		if (g_snap_pbuf) g_object_unref(g_snap_pbuf);
		g_snap_pbuf = 0;

		if (g_regist_pbuf) g_object_unref(g_regist_pbuf);
		g_regist_pbuf = 0;

		gtk_image_clear(g_gtkimage);
		gtk_widget_hide(g_gtkwindow);

		nfui_nfobject_hide((NFOBJECT*)g_curwnd);
		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)g_curwnd);	
	}
}

