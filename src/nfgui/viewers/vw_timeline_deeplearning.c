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
#include "itx_ai_def.h"
#include "nf_meta_data.h"

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
#include "vw_deeplearning_image_popup.h"
#include "vw_timeline_deeplearning_option.h"
#include "nf_api_play.h"
#include "ix_func.h"
#include "wrk.h"
#include "vw.h"
#include "dtf.h"
#include "uxm.h"
#include "stm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VW_TML_DEEPLEARNING"


#define VT_WIN_POS_X				(DISPLAY_ACTIVE_WIDTH - VT_WIN_SIZE_W)
#define VT_WIN_POS_Y				(0)
#define VT_WIN_SIZE_W				(192)
#define VT_WIN_SIZE_H				(972)

#define MAX_BUFF_CNT				(99)
#define MAX_SHOW_CNT				(5)

#define EVENT_FRAME_WIDTH			(176)
#define EVENT_FRAME_HEIGHT			(86)

#define SCALE_SIZE_W				(640)
#define SCALE_SIZE_H				(360)


typedef struct {
	gchar caption[128];
	gchar title[128];	
	gchar description[512];
} __attribute__((packed)) GENERIC_OBJECT;

typedef struct {
    gchar rule_name[64];
	gchar class_name[64];
} __attribute__((packed)) DEFAULT_OBJECT;

typedef struct {
	gint face_id;	
	gchar name[256];
	gchar group[256];
	gchar gender[256];
	gint age;
} __attribute__((packed)) FACE_OBJECT;

typedef struct {
	gchar number[256];
	gchar country[16];
} __attribute__((packed)) PLATENO_OBJECT;

typedef struct _OBJECT_INFO_T {
	GTimeVal ttime;
	gint confidence;
	gfloat coords[4];
	union {
		GENERIC_OBJECT gnr;
		DEFAULT_OBJECT dft;
		FACE_OBJECT face;
		PLATENO_OBJECT plateno;
	} __attribute__((packed));
} OBJECT_INFO_T;

typedef struct _EVENT_DETECT_T {
	gint ch;
	gint evt_type;
	guint obj_id;
	GTimeVal mk_time;
	gint obj_cnt;
    OBJECT_INFO_T obj_info[6];
	GdkPixbuf *cover_pbuf;
	gint focus;
} EVENT_DETECT_T;




////////////////////////////////////////////////////////////////
//
// private variables
//

static WRK_ID iwrk_change_play = 0; 

static NFWINDOW *g_curwnd = NULL;
//static NFOBJECT *g_info_object[MAX_SHOW_CNT];
static NFOBJECT *g_event_object[MAX_SHOW_CNT];

static GList *g_event_buff_list = 0;

static gint g_shown_fixed = 0;

static TLINE_MODE_E g_tml_mode = 0;
static gint g_shown_tab = 0;

static gpointer g_focus_buff_ptr = 0;



////////////////////////////////////////////////////////////////
//
// private interfaces
//


static gint _get_model_property(NFOBJECT *obj)
{
    SysInfoData data;

    memset(&data, 0x00, sizeof(SysInfoData));
    DAL_get_sysInfo_data(&data);

    if (ivsc.model_code == IPX_M4_MODEL)
    {
        if (!strcmp(data.model, "VUHDIP-4") ||
            !strcmp(data.model, "VUHDIP-8") ||
            !strcmp(data.model, "VUHDIP-16") ||
			!strcmp(data.model, "VUHDIP-4V2") ||
            !strcmp(data.model, "VUHDIP-8V2") ||
            !strcmp(data.model, "VUHDIP-16V2")||
            !strcmp(data.model, "VUHDIP-32"))
        {
			nfui_nflabel_set_text((NFLABEL*)obj, "Concept Pro NVR");
            nfui_nflabel_set_pango_font((NFLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(206));
        }
        else if (!strcmp(data.model, "VUHDIPL-4") ||
                 !strcmp(data.model, "VUHDIPL-8") ||
                 !strcmp(data.model, "VUHDIPL-16") ||
				 !strcmp(data.model, "VUHDIPL-4V2") ||
                 !strcmp(data.model, "VUHDIPL-8V2") ||
                 !strcmp(data.model, "VUHDIPL-16V2")||
                 !strcmp(data.model, "VUHDIPL-32V2"))
        {
			nfui_nflabel_set_text((NFLABEL*)obj, "Concept Pro Lite NVR");
            nfui_nflabel_set_pango_font((NFLABEL*)obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_5), COLOR_IDX(206));
            nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
        }
    }
    else if (ivsc.model_code == IPX_P4E_MODEL)
    {
        if (!strcmp(data.model, "VUHDIPE-8V2") ||
            !strcmp(data.model, "VUHDIPE-16V2"))
        {
			nfui_nflabel_set_text((NFLABEL*)obj, "Concept Pro Elite NVR");
            nfui_nflabel_set_pango_font((NFLABEL*)obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_5), COLOR_IDX(206));
            nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
        }
    }    

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

static gint _draw_thumbnail_slidebar()
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	gchar strBuf[64];
	gint bar_width, bg_width;
	gint pos_x, focus_x;
	gint buff_idx;
	gdouble per;

	drawable = nfui_nfobject_get_window((NFOBJECT*)g_curwnd);

	gc = nfui_nfobject_get_gc((NFOBJECT*)g_curwnd);
	nfutil_draw_image(drawable, gc, "ai_slidebg.png", 24, VT_WIN_SIZE_H-26, -1, -1, NFALIGN_LEFT, 0);

	if ((g_event_buff_list) && (g_list_length(g_event_buff_list) > 0))
	{		
		if (g_list_length(g_event_buff_list) > MAX_SHOW_CNT)
		{
			bg_width = 143;

			bar_width = bg_width/(g_list_length(g_event_buff_list)-MAX_SHOW_CNT+1);

			if (bar_width < 30) {
				bar_width = 30;
			}

			bg_width -= bar_width;

			buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
			per = (gdouble)(bg_width) / (gdouble)(g_list_length(g_event_buff_list)-MAX_SHOW_CNT);
			focus_x = (gint)((gdouble)buff_idx*(gdouble)per);


			pos_x = 24+focus_x;
			if (pos_x < 24) pos_x = 24;
			if (pos_x > 24+143-bar_width) pos_x = 24+143-bar_width;				
		}
		else
		{
			bar_width = 143;
			pos_x = 24;
		}

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "MK_AI_SLIDEBAR_IMAGE_""%d", bar_width);
		nf_ui_create_image_button_method(strBuf, bar_width, "ai_slidebar_l.png", "ai_slidebar_m.png", "ai_slidebar_r.png");
		nfutil_draw_image(drawable, gc, strBuf, pos_x, VT_WIN_SIZE_H-26, -1, -1, NFALIGN_LEFT, 0);
	}

	nfui_nfobject_gc_unref(gc);	
}

static gint _draw_event_detect_info(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint bg_color, font_color;

	EVENT_DETECT_T *event_info;
	gint buff_idx;
	GTimeVal detect_time;

	gchar strBuf[128];
    gint gap_x, gap_y;
	gint text_w, text_h;
	gint valid_cnt = 0;

	gint mon, day, hour, min, sec;

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if (buff_idx < g_list_length(g_event_buff_list))
	{
		event_info = (EVENT_DETECT_T*)g_list_nth_data(g_event_buff_list, buff_idx);

		if (g_focus_buff_ptr == event_info) {
			bg_color = 439;
			font_color = 441;
		}
		else {
			bg_color = 435;
			font_color = 437;
		}

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, 176, 63);	

		memset(strBuf, 0x00, sizeof(strBuf));
		g_snprintf(strBuf, sizeof(strBuf), "CH%d", event_info->ch+1);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+8, gap_y+4, 60, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);

		memset(strBuf, 0x00, sizeof(strBuf));
		//ifn_get_local_hourmin_text((time_t)event_info->obj_info.ttime.tv_sec, 0, strBuf);

		ifn_get_local_day((time_t)event_info->obj_info[0].ttime.tv_sec, 0, &mon, &day);
		ifn_get_local_hourmin((time_t)event_info->obj_info[0].ttime.tv_sec, &hour, &min, &sec);
		g_sprintf(strBuf, "%02d-%02d %02d:%02d:%02d", mon, day, hour, min, sec);

		text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);
		//text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+obj->width-7-text_w, gap_y+4, text_w, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);

		if (event_info->evt_type == DEVT_TYPE_GNR)
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "%s", event_info->obj_info[0].gnr.caption);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+24, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);

			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "%s", event_info->obj_info[0].gnr.title);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+44, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);
		}	
		else if (event_info->evt_type == DEVT_TYPE_FR)
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			if (strlen(event_info->obj_info[0].face.name)) g_snprintf(strBuf, sizeof(strBuf), "%s", event_info->obj_info[0].face.name);
			else g_snprintf(strBuf, sizeof(strBuf), "%s", "Unregistered");

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+44, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);			
		}
		else if (event_info->evt_type == DEVT_TYPE_LPR)
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "%s(%d%%)", event_info->obj_info[0].plateno.number, event_info->obj_info[0].confidence);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+44, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);			
		}
		else if (event_info->evt_type == DEVT_TYPE_DFT)
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "%s", event_info->obj_info[0].dft.rule_name);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+24, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);

			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "%s(%d%%)", lookup_string(event_info->obj_info[0].dft.class_name), event_info->obj_info[0].confidence);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, 120);
			nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+8, gap_y+44, 120, 20, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);
		}

		if (event_info->obj_cnt > 1)
		{
			if (g_focus_buff_ptr == event_info)
				nfutil_draw_image(drawable, gc, "MKB_IMG_POPUP_BTN2_S_40", gap_x+133, gap_y+24, -1, -1, NFALIGN_LEFT, 0);
			else
				nfutil_draw_image(drawable, gc, "MKB_IMG_POPUP_BTN2_N_40", gap_x+133, gap_y+24, -1, -1, NFALIGN_LEFT, 0);

			memset(strBuf, 0x00, sizeof(strBuf));
			g_snprintf(strBuf, sizeof(strBuf), "+%d", event_info->obj_cnt);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, strBuf, gap_x+133, gap_y+24, 40, 34, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
		}
	}
	else
	{
		bg_color = 435;
		font_color = 437;

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+2, 176, 63);	

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "-", gap_x+8, gap_y+4, 165, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "-", gap_x+8, gap_y+24, 165, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);	

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "-", gap_x+8, gap_y+44, 165, 20, 
			nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(font_color), NFALIGN_LEFT, 0);	
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

	EVENT_DETECT_T *event_info;
	gint buff_idx;

    gint gap_x, gap_y;

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if (buff_idx < g_list_length(g_event_buff_list))
	{
		event_info = (EVENT_DETECT_T*)g_list_nth_data(g_event_buff_list, buff_idx);

		if (g_focus_buff_ptr == event_info) {
			bg_color = 440;
			font_color = 441;
		}
		else {
			bg_color = 436;
			font_color = 437;
		}

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+65, 176, 86);

		if (ssm_get_covert_mask() & (1 << event_info->ch))
		{
			if (!ssm_get_covert_shown_as())
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
				nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "NO IMAGE", gap_x+2, gap_y+65, 176, 86, 
					nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
			}
			else
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
				nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "COVERT", gap_x+2, gap_y+65, 176, 86, 
					nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
			}
		}
		else if (event_info->cover_pbuf) 
		{
			gdk_draw_pixbuf(drawable, gc, event_info->cover_pbuf, 0, 0, 
				gap_x+3+(EVENT_FRAME_WIDTH-gdk_pixbuf_get_width(event_info->cover_pbuf))/2, gap_y+65+(EVENT_FRAME_HEIGHT-gdk_pixbuf_get_height(event_info->cover_pbuf))/2, 
				-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}
		else
		{
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
			nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "NO IMAGE", gap_x+2, gap_y+65, 176, 86, 
				nffont_get_pango_font(NFFONT_MINI_SEMI_3), &UX_COLOR(font_color), NFALIGN_CENTER, 0);
		}
	}
	else
	{
		bg_color = 436;
		font_color = 437;

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x+2, gap_y+65, 176, 86);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, "NO IMAGE", gap_x+2, gap_y+65, 176, 86, 
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

	EVENT_DETECT_T *event_info;
	gint buff_idx;

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));

	if (buff_idx < g_list_length(g_event_buff_list))
	{
		event_info = (EVENT_DETECT_T*)g_list_nth_data(g_event_buff_list, buff_idx);

		if (g_focus_buff_ptr == event_info)
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(438));
		else 
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));
	}
	else
	{
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(434));
	}

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
	gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, 180, 153);	

    _draw_event_detect_info(obj);
    _draw_event_detect_frame(obj);
    return 0;
}

static gint _set_event_object_detect(gint ch, gint evt_type, guint id, OBJECT_INFO_T *obj_info, GdkPixbuf *npixbuf)
{
	GList *plist;
	EVENT_DETECT_T *event_info = 0, *tmp_info;
	gint i, buff_cnt;

	gint img_w, img_h;
	gfloat coords[4];

	buff_cnt = g_list_length(g_event_buff_list);

	for (i = 0; i < buff_cnt; i++)
	{
		plist = g_list_nth(g_event_buff_list, i);
		tmp_info = (EVENT_DETECT_T*)plist->data;

		if ((tmp_info->ch == ch) && (tmp_info->evt_type == evt_type) && (abs(tmp_info->mk_time.tv_sec-obj_info->ttime.tv_sec) < 60))
		{
			event_info = imalloc(sizeof(EVENT_DETECT_T));
			if (!event_info) return 0;

			if (g_focus_buff_ptr == tmp_info) g_focus_buff_ptr = event_info;

			event_info->ch = tmp_info->ch;
			event_info->evt_type = tmp_info->evt_type;
			event_info->obj_id = tmp_info->obj_id;
			event_info->mk_time.tv_sec = tmp_info->mk_time.tv_sec;
			event_info->mk_time.tv_usec = tmp_info->mk_time.tv_usec;
			event_info->obj_cnt = tmp_info->obj_cnt;

			if (tmp_info->obj_cnt == 6) memcpy(&event_info->obj_info[1], tmp_info->obj_info, sizeof(OBJECT_INFO_T)*(tmp_info->obj_cnt-1));
			else memcpy(&event_info->obj_info[1], tmp_info->obj_info, sizeof(OBJECT_INFO_T)*tmp_info->obj_cnt);

			if (event_info->obj_cnt < 6) event_info->obj_cnt++;

			if (tmp_info->cover_pbuf) g_object_unref(tmp_info->cover_pbuf);
			tmp_info->cover_pbuf = 0;

			ifree(plist->data);
			g_event_buff_list = g_list_delete_link(g_event_buff_list, plist);
			break;
		}
	}

	if (!event_info)
	{
		event_info = imalloc(sizeof(EVENT_DETECT_T));
		if (!event_info) return 0;

		event_info->ch = ch;
		event_info->evt_type = evt_type;
		event_info->obj_id = id;
		event_info->mk_time.tv_sec = obj_info->ttime.tv_sec;
		event_info->mk_time.tv_usec = obj_info->ttime.tv_usec;
		event_info->obj_cnt = 1;
	}

	buff_cnt = g_list_length(g_event_buff_list);

	if (buff_cnt >= MAX_BUFF_CNT) 
	{
        plist = g_list_nth(g_event_buff_list, buff_cnt-1);
		tmp_info = (EVENT_DETECT_T*)plist->data;

		if (g_focus_buff_ptr == tmp_info) g_focus_buff_ptr = 0;

		if (tmp_info->cover_pbuf) g_object_unref(tmp_info->cover_pbuf);
		tmp_info->cover_pbuf = 0;

        ifree(plist->data);
		g_event_buff_list = g_list_delete_link(g_event_buff_list, plist);
	}

	memcpy(&event_info->obj_info[0], obj_info, sizeof(OBJECT_INFO_T));

	if (npixbuf) 
	{
		img_w = gdk_pixbuf_get_width(npixbuf);
		img_h = gdk_pixbuf_get_height(npixbuf);
		event_info->cover_pbuf = _get_target_pixbuf(img_w, img_h, npixbuf, event_info->obj_info[0].coords);
	}

	g_event_buff_list = g_list_prepend(g_event_buff_list, event_info);

	return 0;
}

static gint _set_deeplearning_object_detect(DVA_MSG *dva_event)
{
	OBJECT_INFO_T obj_info;

    GInputStream *stream = 0;
	GdkPixbuf *orgpixbuf = 0;
	GdkPixbuf *npixbuf = 0;

	memset(&obj_info, 0x00, sizeof(OBJECT_INFO_T));

	obj_info.ttime.tv_sec = (glong)dva_event->timestamp;
	obj_info.ttime.tv_usec = (glong)dva_event->timestampl*5*1000;

	if (dva_event->type == DVA_INTRUSION_DETECTION)
	{
		strcpy(obj_info.dft.rule_name, "intrusion");
		dvatext_translate_to_uxitem(dva_event->intrusion_detection.name, obj_info.dft.class_name, sizeof(obj_info.dft.class_name));
		obj_info.confidence = (gint)(dva_event->intrusion_detection.confidence*100);

		obj_info.coords[0] = dva_event->intrusion_detection.bbox.coords[0];
		obj_info.coords[1] = dva_event->intrusion_detection.bbox.coords[1];
		obj_info.coords[2] = dva_event->intrusion_detection.bbox.coords[2];
		obj_info.coords[3] = dva_event->intrusion_detection.bbox.coords[3];		
	}
	else if (dva_event->type == DVA_ILLEGAL_PARKING) 
	{
		strcpy(obj_info.dft.rule_name, "illegal parking");
		dvatext_translate_to_uxitem(dva_event->illegal_parking.name, obj_info.dft.class_name, sizeof(obj_info.dft.class_name));
		obj_info.confidence = (gint)(dva_event->illegal_parking.confidence*100);

		obj_info.coords[0] = dva_event->intrusion_detection.bbox.coords[0];
		obj_info.coords[1] = dva_event->intrusion_detection.bbox.coords[1];
		obj_info.coords[2] = dva_event->intrusion_detection.bbox.coords[2];
		obj_info.coords[3] = dva_event->intrusion_detection.bbox.coords[3];		
	}

    stream = g_memory_input_stream_new_from_data(dva_event->snapshot.data, dva_event->snapshot.size, NULL);
	orgpixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
	npixbuf = gdk_pixbuf_scale_simple(orgpixbuf, 1920, 1080, 0);
    _set_event_object_detect(dva_event->ch, DEVT_TYPE_DFT, 0, &obj_info, npixbuf);

	if (orgpixbuf) g_object_unref(orgpixbuf);
	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

    return 0;
}

static gint _set_dvabx_object_detect(gint ch, gint cnt, ai_rule_event_t *dva_evt, gchar *jpeg_buffer, guint jpeg_size)
{
	OBJECT_INFO_T obj_info;
	gint i;

    GInputStream *stream = 0;
	GdkPixbuf *npixbuf = 0;
	//gint jpeg_w, jpeg_h;
	//gint jpeg_size;
	//gchar *out_buffer = 0;
	//gboolean retVal;

	//GTimeVal stime, etime, ftime;

	//stime.tv_sec = (glong)dva_evt[0].timestamp;
	//stime.tv_usec = (glong)dva_evt[0].timestampl;
	//etime.tv_sec = stime.tv_sec+1;
	//etime.tv_usec = stime.tv_usec;

	//retVal = nf_play_get_thumbnail_jpeg(ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &out_buffer, &ftime);
	//if (!retVal) return -1;

	stream = g_memory_input_stream_new_from_data(jpeg_buffer, jpeg_size, NULL);
	npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

	for (i = 0; i < cnt; i++)
	{
		memset(&obj_info, 0x00, sizeof(OBJECT_INFO_T));

		obj_info.ttime.tv_sec = (glong)dva_evt[i].timestamp;
		obj_info.ttime.tv_usec = (glong)dva_evt[i].timestampl;
		obj_info.confidence = (gint)(dva_evt[i].confidence * 100);
		obj_info.coords[0] = dva_evt[i].bbx_position[0];
		obj_info.coords[1] = dva_evt[i].bbx_position[1];
		obj_info.coords[2] = dva_evt[i].bbx_position[2];
		obj_info.coords[3] = dva_evt[i].bbx_position[3];

		g_message("%s, %d, type:%08X", __FUNCTION__, __LINE__, dva_evt[i].type);

		if (dva_evt[i].type & IVCA_ET_INTRUSION) strcpy(obj_info.dft.rule_name, "intrusion");
		else if (dva_evt[i].type & IVCA_ET_ENTER) strcpy(obj_info.dft.rule_name, "enter");
		else if (dva_evt[i].type & IVCA_ET_EXIT) strcpy(obj_info.dft.rule_name, "exit");
		else if (dva_evt[i].type & IVCA_ET_STOPPED) strcpy(obj_info.dft.rule_name, "stopped");
		else if (dva_evt[i].type & IVCA_ET_LOITERED) strcpy(obj_info.dft.rule_name, "loitering");
		else if (dva_evt[i].type & IVCA_ET_FALL) strcpy(obj_info.dft.rule_name, "fall");
		else if (dva_evt[i].type & IVCA_ET_REMOVED) strcpy(obj_info.dft.rule_name, "removed");
		else if (dva_evt[i].type & IVCA_ET_DIR_POS) strcpy(obj_info.dft.rule_name, "forward direction");
		else if (dva_evt[i].type & IVCA_ET_DIR_NEG) strcpy(obj_info.dft.rule_name, "reverse direction");

		g_snprintf(obj_info.dft.class_name, sizeof(obj_info.dft.class_name), "%s", dva_evt[i].object_class);

		if (dva_evt[i].object_id < 0xff000000) _set_event_object_detect(ch, DEVT_TYPE_DFT, dva_evt[i].object_id, &obj_info, npixbuf);
		else _set_event_object_detect(ch, DEVT_TYPE_DFT, 0, &obj_info, npixbuf);
	}

	//if (out_buffer) free(out_buffer);
	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

    return 0;
}

static gint _set_dvabx_generic_object_detect(gint ch, gint cnt, ai_generic_event_t *dva_evt, gchar *jpeg_buffer, guint jpeg_size)
{
	OBJECT_INFO_T obj_info;
	gint i;

    GInputStream *stream = 0;
	GdkPixbuf *npixbuf = 0;
	//gint jpeg_w, jpeg_h;
	//gint jpeg_size;
	//gchar *out_buffer = 0;
	//gboolean retVal;

	//GTimeVal stime, etime, ftime;

	//stime.tv_sec = (glong)dva_evt[0].timestamp;
	//stime.tv_usec = (glong)dva_evt[0].timestampl;
	//etime.tv_sec = stime.tv_sec+1;
	//etime.tv_usec = stime.tv_usec;

	//retVal = nf_play_get_thumbnail_jpeg(ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &out_buffer, &ftime);
	//if (!retVal) return -1;

	stream = g_memory_input_stream_new_from_data(jpeg_buffer, jpeg_size, NULL);
	npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

	for (i = 0; i < cnt; i++)
	{
		memset(&obj_info, 0x00, sizeof(OBJECT_INFO_T));

		obj_info.ttime.tv_sec = (glong)dva_evt[i].timestamp;
		obj_info.ttime.tv_usec = (glong)dva_evt[i].timestampl;
		//obj_info.confidence = (gint)(dva_evt[i].confidence * 100);
		obj_info.coords[0] = dva_evt[i].event_area[0].x;
		obj_info.coords[1] = dva_evt[i].event_area[0].y;
		obj_info.coords[2] = dva_evt[i].event_area[1].x;
		obj_info.coords[3] = dva_evt[i].event_area[1].y;

		g_snprintf(obj_info.gnr.caption, sizeof(obj_info.gnr.caption), "%s", dva_evt[i].caption);
		g_snprintf(obj_info.gnr.title, sizeof(obj_info.gnr.title), "%s", dva_evt[i].title);	
		g_snprintf(obj_info.gnr.description, sizeof(obj_info.gnr.description), "%s", dva_evt[i].description);

		_set_event_object_detect(ch, DEVT_TYPE_GNR, 0, &obj_info, npixbuf);
	}

	//if (out_buffer) free(out_buffer);
	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

    return 0;
}

static gint _set_external_action_control_detect(gint ch, ANALYTICS_ADDITIONAL_EVENT_T *event)
{
	OBJECT_INFO_T obj_info;
	gint i;

    GInputStream *stream = 0;
	GdkPixbuf *npixbuf = 0;
	gint jpeg_w, jpeg_h;
	gint jpeg_size;
	gboolean retVal;

	memset(&obj_info, 0x00, sizeof(OBJECT_INFO_T));

	if (event->jpeg_buff) {
		stream = g_memory_input_stream_new_from_data(event->jpeg_buff, event->jpeg_len, NULL);
		npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
	}

	obj_info.ttime.tv_sec = (glong)event->timestamp.tv_sec;
	obj_info.ttime.tv_usec = (glong)event->timestamp.tv_usec;
	//obj_info.confidence = (gint)(dva_evt[i].confidence * 100);
	obj_info.coords[0] = 0;
	obj_info.coords[1] = 0;
	obj_info.coords[2] = 1;
	obj_info.coords[3] = 1;

	g_snprintf(obj_info.gnr.caption, sizeof(obj_info.gnr.caption), "%s", event->caption);
	g_snprintf(obj_info.gnr.title, sizeof(obj_info.gnr.title), "%s", event->title);	
	g_snprintf(obj_info.gnr.description, sizeof(obj_info.gnr.description), "%s", event->description);

	_set_event_object_detect(ch, DEVT_TYPE_GNR, 0, &obj_info, npixbuf);

	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

    return 0;
}

static gint _add_event_set_buff_index()
{
	gint i, buff_idx;

	buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));

	if (buff_idx+MAX_SHOW_CNT < g_list_length(g_event_buff_list))
	{
		if ((g_shown_fixed == 1) || (buff_idx != 0))
		{
			for (i = 0; i < MAX_SHOW_CNT; i++)
			{
				buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[i], "EVENT_BUFF_IDX"));
				nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx+1));
			}				
		}
	}

	for (i = 0; i < MAX_SHOW_CNT; i++) {
		nfui_signal_emit(g_event_object[i], GDK_EXPOSE, FALSE);
	}

	return 0;
}

static gint _start_playback(gint ch, time_t stime)
{
	SecurityData secdata;
	gboolean use_dl = 0;
	gchar *user_id = NULL;

	guint ch_mask = 0;
	GTimeVal stimeval = {0, };

	DAL_get_use_double_login(&use_dl);

	if (use_dl && !ssm_is_admin())
	{
		if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return -1;
		if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return -1;
	}
	else
	{
		DAL_get_security_data(&secdata);
		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return -1;
	}

	// only show when user log-on
	user_id = ssm_get_cur_id(NULL);
	if(!strlen(user_id)) return -1;
	if(!ssm_check_access_auth(USR_AUTH_SEARCH)) return -1;

	ch_mask = 1 << ch;

	stimeval.tv_sec = stime;
	stimeval.tv_usec = 0;

	VW_Live_StatusBar_Hide();									

	vsm_live_stop();
	vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_DLVA_TL);
	vsm_playback_start(ch_mask, stimeval, PLAYBACK_NORMAL);

	return 0;
}

static int _proc_change_play(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
	GTimeVal va;
	gint ch = 0;

	memset(&va, 0x00, sizeof(GTimeVal));

	ch = pmsg->param;
	va.tv_sec = GPOINTER_TO_UINT(pmsg->data);
	vsm_playback_restart_by_chtime(ch, va);
	return 0;
}

static int _init_worker()
{
	iwrk_change_play = wrk_create_worker(_proc_change_play, 0);
	wrk_change_sleep_time(iwrk_change_play, 300000);

	return 0;
}

static int _replay(gint ch, time_t playtime)
{
	// safe logic against to heavy clicking over the timeline
	wrk_clear_job(iwrk_change_play);
	wrk_run_msg(iwrk_change_play, IMSG_NONE, ch, 0, GUINT_TO_POINTER(playtime));
	return 0;
}

////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_tml_tab_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_Timeline_DeepLearning_Tab_Hide();
		VW_Timeline_Tab_Show();

		VW_Timeline_DeepLearning_Hide();
		VW_Timeline_Show();	
	}

	return FALSE;
}

static gboolean post_move_up_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_RELEASE) 
	{
		gint buff_idx = 0;
		gint i;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
		if (buff_idx == 0) return FALSE;

		buff_idx--;

		for (i = 0; i < MAX_SHOW_CNT; i++)
		{
			nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
			_draw_deeplearning_object_detect(g_event_object[i]);
			buff_idx += 1;
		}

		_draw_thumbnail_slidebar();
	}

	return FALSE;
}

static gboolean post_move_down_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_RELEASE) 
	{
		gint buff_idx = 0;
		gint i;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
		if (buff_idx >= MAX_BUFF_CNT-MAX_SHOW_CNT) return FALSE;
		if (buff_idx+MAX_SHOW_CNT >= g_list_length(g_event_buff_list)) return FALSE;

		buff_idx++;

		for (i = 0; i < MAX_SHOW_CNT; i++)
		{
			nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
			_draw_deeplearning_object_detect(g_event_object[i]);
			buff_idx += 1;
		}

		_draw_thumbnail_slidebar();
	}

	return FALSE;
}

static gboolean post_statistic_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		vw_deeplearning_statistic_popup_show();
	}

	return FALSE;
}

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

/*
		case GDK_2BUTTON_PRESS:
		{
			gint buff_idx;
			EVENT_DETECT_T *event_info;

			if (g_tml_mode == TL_PLAY) return FALSE;

			buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));
			if (buff_idx >= g_list_length(g_event_buff_list)) return FALSE;

			event_info = (EVENT_DETECT_T*)g_list_nth_data(g_event_buff_list, buff_idx);
			_start_playback(event_info->ch, event_info->obj_info[0].ttime.tv_sec);
		}
		break;
*/
		case GDK_BUTTON_PRESS:
		{
			gint buff_idx;
			EVENT_DETECT_T *event_info;

			GOBJECT_INFO_T group_info[6] = {0, };
			gint i;

			if (g_tml_mode == TL_PLAY) return FALSE;

			buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "EVENT_BUFF_IDX"));
			if (buff_idx >= g_list_length(g_event_buff_list)) return FALSE;

			g_message("%s, %d, buff_idx:%d", __FUNCTION__, __LINE__, buff_idx);

			event_info = (EVENT_DETECT_T*)g_list_nth_data(g_event_buff_list, buff_idx);
			g_focus_buff_ptr = event_info;

			for (i = 0; i < MAX_SHOW_CNT; i++)
			{
				_draw_deeplearning_object_detect(g_event_object[i]);
			}

			if (ssm_get_covert_mask() & (1 << event_info->ch)) return FALSE;

			for (i = 0; i < event_info->obj_cnt; i++)
			{
				group_info[i].ch = event_info->ch;
				group_info[i].confidence = event_info->obj_info[i].confidence;
				memcpy(&group_info[i].ttime, &event_info->obj_info[i].ttime, sizeof(GTimeVal));
				memcpy(&group_info[i].coords, &event_info->obj_info[i].coords, sizeof(gfloat)*4); 

				group_info[i].evt_type = event_info->evt_type;

				if (event_info->evt_type == DEVT_TYPE_GNR)
				{
					strcpy(group_info[i].gnr.caption, event_info->obj_info[i].gnr.caption);
					strcpy(group_info[i].gnr.title, event_info->obj_info[i].gnr.title);					
					strcpy(group_info[i].gnr.description, event_info->obj_info[i].gnr.description);
				}	
				else if (event_info->evt_type == DEVT_TYPE_DFT)
				{
					strcpy(group_info[i].dft.rule_name, event_info->obj_info[i].dft.rule_name);
					strcpy(group_info[i].dft.class_name, event_info->obj_info[i].dft.class_name);
				}
			}

			VW_deepLearning_group_popup_show(group_info, event_info->obj_cnt);
		}
		break;

		case GDK_SCROLL:
		{
			GdkEventScroll *sevt;
			gint buff_idx = 0;
			gint i;

			sevt = (GdkEventScroll*)event;

			if (sevt->direction == GDK_SCROLL_UP) 
			{	
				buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
				if (buff_idx == 0) return FALSE;

				buff_idx--;

				for (i = 0; i < MAX_SHOW_CNT; i++)
				{
					nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
					_draw_deeplearning_object_detect(g_event_object[i]);
					buff_idx += 1;
				}

				_draw_thumbnail_slidebar();
			}
			else if (sevt->direction == GDK_SCROLL_DOWN) 
			{
				buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
				if (buff_idx >= MAX_BUFF_CNT-MAX_SHOW_CNT) return FALSE;
				if (buff_idx+MAX_SHOW_CNT >= g_list_length(g_event_buff_list)) return FALSE;

				buff_idx++;

				for (i = 0; i < MAX_SHOW_CNT; i++)
				{
					nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
					_draw_deeplearning_object_detect(g_event_object[i]);
					buff_idx += 1;
				}

				_draw_thumbnail_slidebar();
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	switch (event->type) 
	{
		case GDK_EXPOSE:
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);
			nfutil_draw_image(drawable, gc, IMG_VTIMELINE_BG, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
			nfui_nfobject_gc_unref(gc);

			_draw_thumbnail_slidebar();
		}
		break;

		case GDK_SCROLL:
		{
			GdkEventScroll *sevt;
			gint buff_idx = 0;
			gint i;

			sevt = (GdkEventScroll*)event;

			if (sevt->direction == GDK_SCROLL_UP) 
			{	
				buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
				if (buff_idx == 0) return FALSE;

				buff_idx--;

				for (i = 0; i < MAX_SHOW_CNT; i++)
				{
					nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
					_draw_deeplearning_object_detect(g_event_object[i]);
					buff_idx += 1;
				}

				_draw_thumbnail_slidebar();
			}
			else if (sevt->direction == GDK_SCROLL_DOWN) 
			{
				buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_event_object[0], "EVENT_BUFF_IDX"));
				if (buff_idx >= MAX_BUFF_CNT-MAX_SHOW_CNT) return FALSE;
				if (buff_idx+MAX_SHOW_CNT >= g_list_length(g_event_buff_list)) return FALSE;

				buff_idx++;

				for (i = 0; i < MAX_SHOW_CNT; i++)
				{
					nfui_nfobject_set_data(g_event_object[i], "EVENT_BUFF_IDX", GUINT_TO_POINTER(buff_idx));
					_draw_deeplearning_object_detect(g_event_object[i]);
					buff_idx += 1;
				}

				_draw_thumbnail_slidebar();
			}
		}
		break;

		case INFY_DELAY_DEEPLEARNING_EVENT:
		{
            DVA_MSG *dva_event = (DVA_MSG*)(((CMM_MESSAGE_T*)data)->data);
			gint ret;

            ret = _set_deeplearning_object_detect(dva_event);
			if (ret != -1) {
				_add_event_set_buff_index();
				_draw_thumbnail_slidebar();
			}	
		}
		break;

		case INFY_DELAY_AI_EVENT:
		{
			guint jpeg_size = ((CMM_MESSAGE_T*)data)->param;
			gint *p = ((CMM_MESSAGE_T*)data)->data + jpeg_size;
			ai_rule_event_t *pevt;
			gint ch, cnt;
			gint ret;

			ch = p[0];
			cnt = p[1];
			pevt = p + 2;

			ret = _set_dvabx_object_detect(ch, cnt, pevt, ((CMM_MESSAGE_T*)data)->data, jpeg_size);
			if (ret != -1) {
				_add_event_set_buff_index();
				_draw_thumbnail_slidebar();
			}
		}
		break;

		case INFY_DELAY_AI_GENERIC_EVENT:
		{
			guint jpeg_size = ((CMM_MESSAGE_T*)data)->param;
			gint *p = ((CMM_MESSAGE_T*)data)->data + jpeg_size;
			ai_generic_event_t *pevt;
			gint ch, cnt;
			gint ret;

			ch = p[0];
			cnt = p[1];
			pevt = p + 2;

			ret = _set_dvabx_generic_object_detect(ch, cnt, pevt, ((CMM_MESSAGE_T*)data)->data, jpeg_size);
			if (ret != -1) {
				_add_event_set_buff_index();
				_draw_thumbnail_slidebar();
			}
		}
		break;

		case INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT:
		{
			ANALYTICS_ADDITIONAL_EVENT_T *event = ((CMM_MESSAGE_T*)data)->data;
			gint ch = ((CMM_MESSAGE_T*)data)->param;
			gint ret;

			ret = _set_external_action_control_detect(ch, event);
			if (ret != -1) {
				_add_event_set_buff_index();
				_draw_thumbnail_slidebar();
			}
		}
		break;

		case INFY_CAMDB_CHANGE_NOTIFY:
		case INFY_USRDB_CHANGE_NOTIFY:
		case INFY_USER_LOGON:
		{
			gint i;

			VW_deepLearning_group_popup_hide();
			VW_deepLearning_animate_popup_hide();

			for (i = 0; i < MAX_SHOW_CNT; i++) {
				nfui_signal_emit(g_event_object[i], GDK_EXPOSE, FALSE);
			}
		}
		break;

		case GDK_DELETE:
		    g_curwnd = 0;
			wrk_destroy_worker(iwrk_change_play);
			uxm_unreg_imsg_event(obj, INFY_DELAY_DEEPLEARNING_EVENT);
			uxm_unreg_imsg_event(obj, INFY_DELAY_AI_EVENT);
			uxm_unreg_imsg_event(obj, INFY_DELAY_AI_GENERIC_EVENT);
			uxm_unreg_imsg_event(obj, INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT);			
			uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_USER_LOGON);
		break;

		default:
		break;
	}

	return FALSE;
}



//////////////////////////////��//////////////////////////////////
//
// public interfaces
//

void VW_Timeline_DeepLearning_Tab_Show()
{
	if (!g_curwnd) return;

	g_shown_tab = 1;
}

void VW_Timeline_DeepLearning_Tab_Hide()
{
	if (!g_curwnd) return;

	g_shown_tab = 0;
}

void VW_Timeline_DeepLearning_Open(NFWINDOW *parent) 
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	GdkPixbuf *tml_tab_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ai_tab_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *up_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *down_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *statistic_img[NFOBJECT_STATE_COUNT];

	guint pos_x, pos_y;
	gint i;
	gchar strModel[64];


	tml_tab_img[0] = nfui_get_image_from_file("live_tab_timeline_n.png", NULL);
	tml_tab_img[1] = nfui_get_image_from_file("live_tab_timeline_o.png", NULL);
	tml_tab_img[2] = nfui_get_image_from_file("live_tab_timeline_p.png", NULL);
	tml_tab_img[3] = nfui_get_image_from_file("live_tab_timeline_d.png", NULL);

	ai_tab_img[0] = nfui_get_image_from_file("live_tab_ai_s.png", NULL);
	ai_tab_img[1] = nfui_get_image_from_file("live_tab_ai_s.png", NULL);
	ai_tab_img[2] = nfui_get_image_from_file("live_tab_ai_s.png", NULL);
	ai_tab_img[3] = nfui_get_image_from_file("live_tab_ai_d.png", NULL);

	up_img[0] = nfui_get_image_from_file("ai_up_n.png", NULL);
	up_img[1] = nfui_get_image_from_file("ai_up_o.png", NULL);
	up_img[2] = nfui_get_image_from_file("ai_up_p.png", NULL);
	up_img[3] = nfui_get_image_from_file("ai_up_d.png", NULL);

	down_img[0] = nfui_get_image_from_file("ai_down_n.png", NULL);
	down_img[1] = nfui_get_image_from_file("ai_down_o.png", NULL);
	down_img[2] = nfui_get_image_from_file("ai_down_p.png", NULL);
	down_img[3] = nfui_get_image_from_file("ai_down_d.png", NULL);

	statistic_img[0] = nfui_get_image_from_file("ai_statistics_n.png", NULL);
	statistic_img[1] = nfui_get_image_from_file("ai_statistics_o.png", NULL);
	statistic_img[2] = nfui_get_image_from_file("ai_statistics_p.png", NULL);
	statistic_img[3] = nfui_get_image_from_file("ai_statistics_d.png", NULL);		


	nftool_create_popup_type2_btn_image(40);


	win = (NFOBJECT*)nfui_nfwindow_new(parent, VT_WIN_POS_X, VT_WIN_POS_Y, VT_WIN_SIZE_W, VT_WIN_SIZE_H);
	nfui_nfwindow_set_title(win, "TIMELINE_DEEPLEARNING");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)win)->main_widget), FALSE);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	//nfui_regi_post_event_callback(win, post_tml_wnd_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, VT_WIN_SIZE_W, VT_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	uxm_reg_imsg_event(fixed, INFY_DELAY_DEEPLEARNING_EVENT);
	uxm_monitor_on_imsg_event(fixed, INFY_DELAY_DEEPLEARNING_EVENT);
	
	uxm_reg_imsg_event(fixed, INFY_DELAY_AI_EVENT);
	uxm_monitor_on_imsg_event(fixed, INFY_DELAY_AI_EVENT);	

    uxm_reg_imsg_event(fixed, INFY_DELAY_AI_GENERIC_EVENT);
    uxm_monitor_on_imsg_event(fixed, INFY_DELAY_AI_GENERIC_EVENT);

	uxm_reg_imsg_event(fixed, INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT);
	uxm_monitor_on_imsg_event(fixed, INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT);

	uxm_reg_imsg_event(fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(fixed, INFY_CAMDB_CHANGE_NOTIFY);	
	uxm_reg_imsg_event(fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(fixed, INFY_USRDB_CHANGE_NOTIFY);	

	uxm_reg_imsg_event(fixed, INFY_USER_LOGON);
	uxm_monitor_on_imsg_event(fixed, INFY_USER_LOGON);	
	
	// logo 
	if (var_get_vendor_code() != 28 && var_get_vendor_code() != 128)
	{
    	obj = nfui_nfimage_new(IMG_ITX_LOGO);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, 6, 5);
	}
	else
	{
	    obj = nfui_nflabel_new("");
	    _get_model_property(obj);
	    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	    nfui_nfobject_set_size(obj, VT_WIN_SIZE_W-6, 44);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, 3, 4);
	}

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, tml_tab_img);
	nfui_nfobject_set_size(obj, 92, 36);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 58);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_tml_tab_button_event_cb);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ai_tab_img);
	nfui_nfobject_set_size(obj, 92, 36);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 96, 58);
	nfui_nfobject_show(obj);

	pos_y = 97;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, up_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 6, pos_y);
	nfui_regi_post_event_callback(obj, post_move_up_button_event_cb);	

	pos_y += 24;

	for (i = 0; i < MAX_SHOW_CNT; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(659));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nfobject_set_size(obj, 180, 153);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(434)); 
		nfui_nffixed_put((NFFIXED*)fixed, obj, 6, pos_y);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_event_obj_event_handler);
		g_event_object[i] = obj;

		nfui_nfobject_set_data(obj, "EVENT_BUFF_IDX", GUINT_TO_POINTER(i));

		pos_y += (153+6); 
	}	

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, down_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 6, fixed->height-58);
	nfui_regi_post_event_callback(obj, post_move_down_button_event_cb);	

/*
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, statistic_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 155, fixed->height-35);
	nfui_regi_post_event_callback(obj, post_statistic_button_event_cb);
*/

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);

	VW_deepLearning_group_popup_open(g_curwnd);
	VW_deepLearning_animate_popup_open(g_curwnd);
	VW_deepLearning_generic_animate_popup_open(g_curwnd);
	vw_deeplearning_statistic_popup_open(g_curwnd);

	_init_worker();
}

void VW_Timeline_DeepLearning_Show()
{
	if (!g_curwnd) return;

	if (!nfui_nfobject_is_shown((NFOBJECT*)g_curwnd)) {
		if (g_shown_tab == 1) nfui_nfobject_show((NFOBJECT*)g_curwnd);
	}
}

void VW_Timeline_DeepLearning_Hide()
{
	if (!g_curwnd) return;

	if (nfui_nfobject_is_shown((NFOBJECT*)g_curwnd)) { 
		nfui_nfobject_hide((NFOBJECT*)g_curwnd);
	}
}

gboolean VW_Timeline_DeepLearning_IsShown()
{
	if (!g_curwnd) return FALSE;
	return nfui_nfobject_is_shown((NFOBJECT*)g_curwnd);
}

gboolean VW_Timeline_DeepLearning_clicked(gint x, gint y)
{
	gint win_x, win_y;
	gint off_x, off_y;
	gint i;

	if(!g_curwnd) return FALSE;

	nfui_nfobject_get_window_pos((NFOBJECT*)g_curwnd, &win_x, &win_y);

	if ((x >= win_x) && (x <= win_x+((NFOBJECT*)g_curwnd)->width)
		&& (y >= win_y) && (y <= win_y+((NFOBJECT*)g_curwnd)->height))
	{
		for (i = 0; i < MAX_SHOW_CNT; i++)
		{			
			nfui_nfobject_get_offset(g_event_object[i], &off_x, &off_y);
			off_x += win_x;
			off_y += win_y;

			if ((x >= off_x) && (x <= off_x+g_event_object[i]->width)
				&& (y >= off_y) && (y <= off_y+g_event_object[i]->height))
			{
				nfui_signal_emit(g_event_object[i], GDK_BUTTON_PRESS, TRUE);
				break;
			}
		}

		return TRUE;
	}

	return FALSE;
}

int VW_Timeline_DeepLearning_ChangeMode(TLINE_MODE_E mode)
{
	g_tml_mode = mode;
	return 0;
}
