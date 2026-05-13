/*
 * vw_calendar.c
 * 	- calendar viewer
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 7, 2011
 *
 */

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>


#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "iux_msg.h"
#include "uxm.h"
#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/cw_calendar.h"

#include "viewers/vw_calendar.h"
#include "viewers/vw_internal.h"

//#include "nf_api_disk.h"
#include "services/scm.h"
#include "modules/ssm.h"

#include "vw_timeline.h"
#include "stm.h"
#include "evt.h"


#define CLND_WIN_POS_X				(1728 - 323)
#define CLND_WIN_POS_Y				(48)
#define CLND_WIN_SIZE_W				(323)
#define CLND_WIN_SIZE_H				(386)


////////////////////////////////////////////////////////////////
//
// private variables
//

static time_t cur_time = 0;
static char rinfo[31];
static guint _tmr_update = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_clndWin = NULL;
static NFOBJECT *g_clnd_text = NULL;
static CWCALENDAR *g_clnd = NULL;



////////////////////////////////////////////////////////////////
//
// private interfaces
//

static time_t _get_first_time()
{
	time_t first_t = 0;
	scm_get_record_time(&first_t, 0);
	return first_t;
}

static time_t _get_last_time()
{
	time_t last_t = 0;
	scm_get_record_time(0, &last_t);
	return last_t;
}

static int _get_calendar_day(int *year, int *mon, int *day)
{
	if (year) *year	= cw_cld_get_current_year((CWCALENDAR*)g_clnd);
	if (mon) *mon = cw_cld_get_current_month((CWCALENDAR*)g_clnd);
	if (day) *day  = cw_cld_get_current_day((CWCALENDAR*)g_clnd);
	return 0;
}

static int _change_calendar_date(time_t timet)
{
	int year, mon, day;

	dtf_get_local_day(timet, &year, &mon, &day);
	cw_cld_change_date(g_clnd, year, mon, day);
	return 0;
}

static int _update_calendar_text(time_t timet)
{
	char buf[64];
	int year, mon;
	int length;

	ifn_get_local_day(timet, &year, &mon, 0);

	length = strlen(g_month_str[mon - 1]);
	g_utf8_strncpy(buf, g_month_str[mon - 1], g_utf8_strlen(g_month_str[mon - 1], -1));
	g_sprintf(&buf[length], " %d", year);

	nfui_nflabel_set_text((NFLABEL*)g_clnd_text, buf);
	nfui_signal_emit(g_clnd_text, GDK_EXPOSE, FALSE);
	return 0;
}

static int _set_cur_date(time_t tt)
{
	time_t tv;
	int year, mon, day;

	if (tt == 0) tv = time(0);
	else 		   tv = tt;

	dtf_get_local_day(tv, &year, &mon, &day);
	cw_cld_change_date(g_clnd, year, mon, day);
	return 0;
}

static time_t _get_new_calendar_time()
{
	int year, month, day, hour, min, sec;
	time_t tim;

	_get_calendar_day(&year, &month, &day);
	dtf_get_local_hourmin(cur_time, &hour, &min, &sec);
	tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
	return tim;
}

static int _stop_timer();
static gboolean post_clnd_window_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch (event->type) {
	case NFOUTEVT_BUTTON_PRESS:
		nfui_nfobject_hide(obj);
		nfui_page_close(PGID_LIVE_CALENDAR, obj);	
		_stop_timer();
		evt_send_to_local(INFY_CALENDAR_CLOSED, 0, 0, 0);
		break;
	case GDK_DELETE:

		uxm_unreg_imsg_event(g_clndWin, IRET_SCM_CHANGE_SYSTEM_TIME);
		uxm_unreg_imsg_event(g_clndWin, IRET_SCM_FORMAT_STORAGE);
		uxm_unreg_imsg_event(g_clndWin, INFY_DISK_OW_NOTIFY);

		g_curwnd = 0;
		g_clndWin = NULL;

		nfui_page_close(PGID_LIVE_CALENDAR, obj);
		break;

	case IRET_SCM_CHANGE_SYSTEM_TIME:
	case IRET_SCM_FORMAT_STORAGE:
	case INFY_DISK_OW_NOTIFY:
		cw_cld_reload_data(g_clnd);
		break;
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch (event->type) {
	case GDK_EXPOSE:
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);

		_change_calendar_date(cur_time);
		_update_calendar_text(cur_time);
		break;

	case GDK_DELETE:
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	
		if(g_clndWin) 	g_clndWin = NULL;
		if(g_clnd) 		g_clnd = NULL;
		if(g_clnd_text) g_clnd_text = NULL;
		break;
	}

	return FALSE;
}

static gboolean post_left_arrow_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if (evt->type == GDK_BUTTON_PRESS) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = cw_cld_set_prev_month((CWCALENDAR*)g_clnd);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	}

	return FALSE;
}

static gboolean post_right_arrow_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if (evt->type == GDK_BUTTON_PRESS) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = cw_cld_set_next_month((CWCALENDAR*)g_clnd);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	}

	return FALSE;
}

static gboolean post_first_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t first_time;

	switch (evt->type) {
	case GDK_BUTTON_PRESS:
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		first_time = _get_first_time();
		if (first_time != 0) {
			cur_time = first_time;
			_change_calendar_date(first_time);
			_update_calendar_text(first_time);
			VW_Timeline_Set_Date(cur_time, TRUE);
		}
		break;
	}
	return FALSE;
}

static gboolean post_last_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t last_time;

	switch (evt->type) {
	case GDK_BUTTON_PRESS:
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
				
		last_time = _get_last_time();
		if (last_time != 0) {
			cur_time = last_time;		
			_change_calendar_date(last_time);
			_update_calendar_text(last_time);
			VW_Timeline_Set_Date(cur_time, TRUE);
		}
		break;
	}
	return FALSE;
}

static gboolean post_calendar_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GTimeVal tv;

	switch(evt->type) {
	case GDK_BUTTON_PRESS:
		break;

	case NFEVENT_CALENDAR_CHANGED_RELEASE:
		cur_time = _get_new_calendar_time();
		stm_set_time_t(cur_time);
		_update_calendar_text(cur_time);
		VW_Timeline_Set_Date(cur_time, TRUE);
		break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	nfui_nfobject_hide(top);
	nfui_page_close(PGID_LIVE_CALENDAR, top);	
	evt_send_to_local(INFY_CALENDAR_CLOSED, 0, 0, 0);
	return FALSE;
}

static int _stop_timer()
{
	if (_tmr_update) g_source_remove(_tmr_update);
	_tmr_update = 0;
	return 0;
}

static gboolean _update_proc(void *data)
{
	time_t now;
	int year, mon, day;

	now = time(0);
	dtf_get_local_day(now, &year, &mon, &day);
	cw_cld_reload_data(g_clnd);
	cw_cld_update(g_clnd);
	cur_time = now;

	return TRUE;
}

static int _start_timer()
{
	if (_tmr_update) return 0;
	_tmr_update = g_timeout_add(3600 * 1000, _update_proc, 0);
	return 0;
}

////////////////////////////////////////////////////////////////
//
// public interfaces
//

void VW_Calendar_Open(NFWINDOW *parent) 
{
	NFOBJECT *window;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	gchar *strWeek[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

	gint size_w, size_h;
	gint pos_x;
	gint i;

	GdkPixbuf *first[NFOBJECT_STATE_COUNT];
	GdkPixbuf *last[NFOBJECT_STATE_COUNT];
	GdkPixbuf *up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *down[NFOBJECT_STATE_COUNT];
	GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];


	// window
	g_clndWin = (NFOBJECT*)nfui_nfwindow_new(parent, CLND_WIN_POS_X, CLND_WIN_POS_Y, CLND_WIN_SIZE_W, CLND_WIN_SIZE_H);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_clndWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_clndWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_clndWin, returnkey_proc);
	nfui_nfobject_modify_bg(g_clndWin, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(g_clndWin, post_clnd_window_event_cb);
	g_curwnd = g_clndWin;	
//	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_clndWin)->main_widget), TRUE);


	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, CLND_WIN_SIZE_W, CLND_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	// calendar button
	arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_N_L), NULL);
	arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_O_L), NULL);
	arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_P_L), NULL);
	arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_D_L), NULL);

	arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_N_R), NULL);
	arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_O_R), NULL);
	arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_P_R), NULL);
	arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_ARROW_D_R), NULL);

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[i]);
		nfui_nfobject_set_size(obj, 10, 22);
		nfui_nfobject_show(obj);

		if(i == 0) {
			nfui_regi_post_event_callback(obj, post_left_arrow_event_handler);
			nfui_nffixed_put((NFFIXED*)fixed, obj, 43, 11);
		}else 	   {
			nfui_regi_post_event_callback(obj, post_right_arrow_event_handler);
			nfui_nffixed_put((NFFIXED*)fixed, obj, 263, 11);
		}
	}

	// calendar month/year
	g_clnd_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(362));
	nfui_nfobject_set_size(g_clnd_text, (guint)(176) , (guint)(34));
	nfui_nflabel_set_align((NFLABEL*)g_clnd_text, NFALIGN_CENTER,0);
	nfui_nfobject_use_focus(g_clnd_text, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_clnd_text);
	nfui_nffixed_put((NFFIXED*)fixed, g_clnd_text, (guint)(70), (guint)(4));

	// week
	pos_x = 14;
	for(i=0; i<7; i++) {
		if(i == 0)
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strWeek[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(364));
		else if(i == 6)
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strWeek[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(366));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strWeek[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(365));

		if(i == 1)
			nfui_nflabel_set_spacing((NFLABEL*)obj, CONDENSED_SPACING);
		else
			nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
		nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
		nfui_nfobject_use_tooltip(obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)(41) , (guint)(36));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER,0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, (guint)pos_x, (guint)(44));

		pos_x += 41;
	}

	// calendar	
	cur_time = time(0);
	obj = (NFOBJECT*)cw_cld_new(cur_time, 40, 40);
	nfui_nfobject_set_size(obj, 288, 247);
	//nfui_nfobject_set_size(obj, 120 - 14 - 21, 386 - 80 - 59);
	nfui_regi_post_event_callback(obj, post_calendar_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 14, 80);

	g_clnd = (CWCALENDAR*)obj;


	// first in & last
	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("FIRST", 130);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_first_button_event_cb);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, 12, 329);

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("LAST", 130);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_last_button_event_cb);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, 12 + 130 + 26, 329);

	nfui_nfwindow_add((NFWINDOW*)g_clndWin, fixed);
	nfui_run_main_event_handler(g_clndWin);
	nfui_nfobject_show(g_clndWin);
	nfui_make_key_hierarchy((NFWINDOW*)g_clndWin);
	
	nfui_nfobject_hide(g_clndWin);
	
	uxm_reg_imsg_event(g_clndWin, IRET_SCM_CHANGE_SYSTEM_TIME);
	uxm_reg_imsg_event(g_clndWin, IRET_SCM_FORMAT_STORAGE);
	uxm_reg_imsg_event(g_clndWin, INFY_DISK_OW_NOTIFY);

//	nfui_page_open(PGID_LIVE_CALENDAR, g_clndWin, ssm_get_cur_id(NULL));
}

int VW_Calendar_Close() 
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_Calendar_Show(time_t ti)
{
	if(!g_clndWin) return;

	if(!nfui_nfobject_is_shown(g_clndWin))
	{
		nfui_nfobject_show(g_clndWin);
		nfui_page_open(PGID_LIVE_CALENDAR, g_clndWin, ssm_get_cur_id(NULL));	
	}

	// if (!ifn_is_same_day(cur_time, ti)) 
		cw_cld_reload_data(g_clnd);

	cur_time = ti;
	_start_timer();
}

void VW_Calendar_Hide()
{
	if(!g_clndWin) return;

	if(nfui_nfobject_is_shown(g_clndWin))
	{
		nfui_nfobject_hide(g_clndWin);
		nfui_page_close(PGID_LIVE_CALENDAR, g_clndWin);	
	}

	_stop_timer();
}

gboolean VW_Calendar_IsShown()
{
	if(!g_clndWin) return FALSE;

	return nfui_nfobject_is_shown(g_clndWin);
}

