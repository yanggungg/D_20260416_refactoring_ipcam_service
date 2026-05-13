/*
 * vw_timeline.c
 * 	- timeline viewer
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 17, 2011
 *
 */

#include <time.h>

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>


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

#include "vw_menu.h"
#include "vw_playback_main.h"
#include "vw_playback_statusbar.h"
#include "vw_live_statusbar.h"
#include "nf_api_play.h"
#include "ix_func.h"
#include "wrk.h"
#include "vw.h"
#include "dtf.h"
#include "uxm.h"
#include "stm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VW_TML"


#define VT_WIN_POS_X				(DISPLAY_ACTIVE_WIDTH - VT_WIN_SIZE_W)
#define VT_WIN_POS_Y				(0)
#define VT_WIN_SIZE_W				(192)
#define VT_WIN_SIZE_H				(972)

#define VT_TIMEOUT_INTV				(300)
#define VT_AUTO_UPDATE_INTV			(60000)
#define VT_MANUAL_MODE_INTV			(15000)


////////////////////////////////////////////////////////////////
//
// private variables
//

typedef struct _TWND_DATA {
	TLINE_MODE_E	mode;
	guint			disp_mode;
} TWND_DATA;

static TWND_DATA itd;
static gint g_shown_tab = 1;

static guint t_id = 0;
static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_vtWin = NULL;
static NFOBJECT *g_vtline;
static NFOBJECT *g_vtDate;
static IXTIMELINE *tml;
WRK_ID iwrk = 0;

static NFOBJECT *g_dlva_btn;

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

static void _update_date()
{
	time_t new_date = 0;
	struct tm ttm;
	gchar strTime[32];
	GTimeVal gtv;

	memset(&gtv, 0x00, sizeof(GTimeVal));
	tml_get_cur_time(tml, &gtv);
	dtf_get_local_date(gtv.tv_sec, strTime);

	nfui_nfbutton_set_text((NFBUTTON*)g_vtDate, strTime);
	nfui_signal_emit((NFOBJECT*)g_vtDate, GDK_EXPOSE, FALSE);	

}

static void _update_display_mode()
{
    if(scm_is_qc_mode() == 0){
        DAL_set_live_timeline_on_mode(TLINE_ALWAYS_ON);
    }
	itd.disp_mode = DAL_get_live_timeline_on_mode();
}

static gboolean display_timeline(gpointer data)
{
	PAGEID cur_pgid = nfui_get_cur_page(NULL);

	//g_message("%s ::::::::::::: current page id: %d ", __FUNCTION__, cur_pgid);
	if(cur_pgid != PGID_LIVECTRLBAR) 
		return TRUE;

	NFUTIL_THREADS_ENTER();
	if(itd.disp_mode == TLINE_ALWAYS_OFF) 		VW_Timeline_Hide();
	else if(itd.disp_mode == TLINE_ALWAYS_ON) 	VW_Timeline_Show();	
	NFUTIL_THREADS_LEAVE();
	
	t_id = 0;
	
	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	switch (event->type) {
	case GDK_EXPOSE:
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfutil_draw_image(drawable, gc, IMG_VTIMELINE_BG, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
		break;

	case GDK_DELETE:
		wrk_destroy_worker(iwrk);
		nfui_page_close(PGID_LIVE_TIMELINE, g_vtWin);

		if (t_id) g_source_remove(t_id);
		t_id = 0;

		VW_Timeline_PopUp_Destroy();
		VW_Calendar_Close();
		break;

	default:
		break;
	}

	return FALSE;
}

static gboolean post_ai_tab_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_Timeline_Tab_Hide();
		VW_Timeline_DeepLearning_Tab_Show();

		VW_Timeline_Hide();
		VW_Timeline_DeepLearning_Show();
	}

	return FALSE;
}

static gboolean post_date_button_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	time_t tmp;
	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		tmp = tml_get_cur_time_t(tml);
		tml_set_update_mode(tml, TML_MANUAL_UPDATE);
		if (itd.mode == TL_PLAY) {
			vw_playback_statusbar_playstatus_end();
			vsm_playback_play_pause_by_menu_opened();
		}
		VW_Calendar_Show(tmp);
	
	}

	return FALSE;
}

static int _replay(time_t playtime)
{
	// safe logic against to heavy clicking over the timeline
	wrk_clear_job(iwrk);
	wrk_run_msg(iwrk, IMSG_NONE, playtime, 0, 0);
	return 0;
}

static gboolean post_timeline_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GTimeVal st, et;
	GdkEventButton *bevent = (GdkEventButton *)event; 
	gchar *user_id = NULL;
	int ret;
	CMM_MESSAGE_T *pmsg;
	GTimeVal va;
	int pplay;
	SecurityData secdata;
	gboolean use_dl = 0;

	memset(&va, 0x00, sizeof(GTimeVal));
	memset(&st, 0x00, sizeof(GTimeVal));
	memset(&et, 0x00, sizeof(GTimeVal));
	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		if(event->button.button == MOUSE_RIGTH_BUTTON) {
			// only show when user log-on
			user_id = ssm_get_cur_id(NULL);
			if(!strlen(user_id))
				break;

			tml_get_section_time(obj, &st, &et);

			if (st.tv_sec != 0 && et.tv_sec != 0) {
				VW_Timeline_PopUp_Show((VT_WIN_POS_X + 4), bevent->y, st, et, itd.mode);
			}
		}
		break;
	case GDK_2BUTTON_PRESS:
		break;

	case NFEVENT_TML_RULER_DRAGUP:
		break;

	case NFEVENT_TML_RULER_DRAGDOWN:
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, INFY_TML_DATE_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_PLAY_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_DOUBLE_CLICKED);
		uxm_unreg_imsg_event(obj, INFY_TML_START_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_END_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_UP);
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_DOWN);
		break;

	case INFY_TML_DATE_CHANGED:
		_update_date();
		break;

	case INFY_TML_PLAY_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (itd.mode == TL_LIVE) return FALSE;
		if (obj != pmsg->data) return FALSE;

		// only show when user log-on
		user_id = ssm_get_cur_id(NULL);
		if (!strlen(user_id)) break;

		va.tv_sec = pmsg->param;
		pplay = tml_get_play_pos(obj);//(int)pmsg->data;
		tml_set_cti_position(tml, pplay);
		tml_repaint(tml);
        vsm_change_hide_video();
        
		_update_date();
		_replay(pmsg->param);
		break;

	case INFY_TML_DOUBLE_CLICKED:	    
		pmsg = (CMM_MESSAGE_T *)data;
		if (itd.mode == TL_PLAY) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		if (obj != pmsg->data) return FALSE;

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
		if(!strlen(user_id))
			break;

		if(!ssm_check_access_auth(USR_AUTH_SEARCH))
			break;

		va.tv_sec = pmsg->param;
		pplay = tml_get_play_pos(pmsg->data);//(int)pmsg->data;
		if (va.tv_sec == 0) return FALSE;

		VW_Live_StatusBar_Hide();									

		tml_set_cti_position(tml, pplay);
		tml_change_cur_time_t(tml, va.tv_sec); 

		vsm_live_stop();
		vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_LIVE_TL);
		vsm_playback_start(0xffff, va, PLAYBACK_NORMAL);
		break;

	case INFY_TML_START_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		stm_set_time_t(pmsg->param);
		break;

	case INFY_TML_END_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		stm_set_endtime_t(pmsg->param);
		break;

	case INFY_TML_SCROLL_UP:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (itd.mode == TL_LIVE) tml_go_ahead(tml);
		else tml_slide_down(tml);
		break;

	case INFY_TML_SCROLL_DOWN:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (itd.mode == TL_LIVE) tml_go_back(tml);
		else tml_slide_up(tml);
		break;

	default:
		break;
	}

	return TRUE;
}

static gboolean post_zoomin_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
	case GDK_BUTTON_RELEASE:
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		tml_zoom_in(tml);
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean post_zoomout_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
	case GDK_BUTTON_RELEASE:
	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)  return FALSE;
		tml_zoom_out(tml);
		break;
	default:
		break;
	}
	return FALSE;
}

static gboolean post_move_up_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (itd.mode == TL_LIVE) tml_go_ahead(tml);
		else tml_slide_down(tml);
	}

	return FALSE;
}

static gboolean post_move_down_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (itd.mode == TL_LIVE) tml_go_back(tml);
		else tml_slide_up(tml);
	}

	return FALSE;
}

static time_t _get_timeline_time()
{
	GTimeVal tv;
	memset(&tv, 0x00, sizeof(GTimeVal));
	tv = vsm_playback_get_playtime();
	return tv.tv_sec;
}

static gboolean post_tml_wnd_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	CMM_MESSAGE_T *pmsg;
	gchar *user_id = NULL;
	GTimeVal va;
	int pplay;

	memset(&va, 0x00, sizeof(GTimeVal));
	switch (evt->type) {
	case INFY_CALENDAR_CLOSED:
		if (itd.mode == TL_PLAY) {

			va.tv_sec = stm_get_time_t();
			vw_playback_statusbar_playstatus_start();
			vsm_playback_play_time_by_menu_closed(va);
		}
		tml_set_update_mode(tml, TML_AUTO_UPDATE);
		break;

	case IRET_SCM_CHANGE_SYSTEM_TIME:
	case INFY_DISK_OW_NOTIFY:
	case IRET_SCM_FORMAT_STORAGE:
		tml_update(tml);
		_update_date();
		break;

	case INFY_DSPDB_CHANGE_NOTIFY:
		_update_date();
		_update_display_mode();
		if(itd.mode == TL_LIVE) {
			if(!vsm_get_omode())		// normal mode
				break;

			if(!t_id)
				t_id = g_timeout_add(500, display_timeline, 0);
		}
		break;

	case INFY_SYSDB_CHANGE_NOTIFY:
		_update_date();
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(g_vtWin, INFY_CALENDAR_CLOSED);
		uxm_unreg_imsg_event(g_vtWin, IRET_SCM_FORMAT_STORAGE);
		uxm_unreg_imsg_event(g_vtWin, INFY_DISK_OW_NOTIFY);
		uxm_unreg_imsg_event(g_vtWin, INFY_DSPDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(g_vtWin, INFY_SYSDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(g_vtWin, IRET_SCM_CHANGE_SYSTEM_TIME);

		g_curwnd = 0;
		break;
	default:
		break;

	}

	return FALSE;
}


static int _proc_change_play(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
	GTimeVal va;
	memset(&va, 0x00, sizeof(GTimeVal));
	va.tv_sec = pmsg->param;
	vsm_playback_restart_by_time(va);

	return 0;
}

static int _init_worker()
{
	iwrk = wrk_create_worker(_proc_change_play, 0);
	wrk_change_sleep_time(iwrk, 300000);
	return 0;
}

////////////////////////////////////////////////////////////////
//
// public interfaces
//

void VW_Timeline_Tab_Show()
{
	if (!g_curwnd) return;

	g_shown_tab = 1;	
}

void VW_Timeline_Tab_Hide()
{
	if (!g_curwnd) return;

	g_shown_tab = 0;	
}

void VW_Timeline_Open(NFWINDOW *parent) 
{
	TML_FIGURE_T figure;
	NFOBJECT *window;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	GdkPixmap *div_img[NFOBJECT_STATE_COUNT];
	gint size_w, size_h;
	gint pos_x;

	GdkPixbuf *tml_tab_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ai_tab_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoomin[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoomout[NFOBJECT_STATE_COUNT];
	GdkPixbuf *up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *down[NFOBJECT_STATE_COUNT];

	gchar strTime[32];
	time_t cur_time = 0;
	gchar strModel[64];


	tml_tab_img[0] = nfui_get_image_from_file("live_tab_timeline_s.png", NULL);
	tml_tab_img[1] = nfui_get_image_from_file("live_tab_timeline_s.png", NULL);
	tml_tab_img[2] = nfui_get_image_from_file("live_tab_timeline_s.png", NULL);
	tml_tab_img[3] = nfui_get_image_from_file("live_tab_timeline_d.png", NULL);

	ai_tab_img[0] = nfui_get_image_from_file("live_tab_ai_n.png", NULL);
	ai_tab_img[1] = nfui_get_image_from_file("live_tab_ai_o.png", NULL);
	ai_tab_img[2] = nfui_get_image_from_file("live_tab_ai_p.png", NULL);
	ai_tab_img[3] = nfui_get_image_from_file("live_tab_ai_d.png", NULL);

	zoomin[0] = nfui_get_image_from_file((IMG_N_ZOOMIN), NULL);
	zoomin[1] = nfui_get_image_from_file((IMG_O_ZOOMIN), NULL);
	zoomin[2] = nfui_get_image_from_file((IMG_P_ZOOMIN), NULL);
	zoomin[3] = nfui_get_image_from_file((IMG_D_ZOOMIN), NULL);

	zoomout[0] = nfui_get_image_from_file((IMG_N_ZOOMOUT), NULL);
	zoomout[1] = nfui_get_image_from_file((IMG_O_ZOOMOUT), NULL);
	zoomout[2] = nfui_get_image_from_file((IMG_P_ZOOMOUT), NULL);
	zoomout[3] = nfui_get_image_from_file((IMG_D_ZOOMOUT), NULL);

	up[0] = nfui_get_image_from_file((IMG_N_TIMELINE_UP), NULL);
	up[1] = nfui_get_image_from_file((IMG_O_TIMELINE_UP), NULL);
	up[2] = nfui_get_image_from_file((IMG_P_TIMELINE_UP), NULL);
	up[3] = nfui_get_image_from_file((IMG_D_TIMELINE_UP), NULL);

	down[0] = nfui_get_image_from_file((IMG_N_TIMELINE_DOWN), NULL);
	down[1] = nfui_get_image_from_file((IMG_O_TIMELINE_DOWN), NULL);
	down[2] = nfui_get_image_from_file((IMG_P_TIMELINE_DOWN), NULL);
	down[3] = nfui_get_image_from_file((IMG_D_TIMELINE_DOWN), NULL);

	memset(&itd, 0x00, sizeof(TWND_DATA));

	_update_display_mode();


	// window
	g_vtWin = (NFOBJECT*)nfui_nfwindow_new(parent, VT_WIN_POS_X, VT_WIN_POS_Y, VT_WIN_SIZE_W, VT_WIN_SIZE_H);
	g_curwnd = g_vtWin;
	nfui_nfwindow_set_title(g_vtWin, "TIMELINE");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_vtWin)->main_widget), FALSE);
	nfui_nfobject_modify_bg(g_vtWin, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(g_vtWin, post_tml_wnd_event_cb);


	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, VT_WIN_SIZE_W, VT_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	// logo 
	if (var_get_vendor_code() != 28 && ivsc.vendor_code != 128)
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

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ai_tab_img);
	nfui_nfobject_set_size(obj, 92, 36);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 96, 58);
	nfui_nfobject_hide(obj);
	nfui_regi_post_event_callback(obj, post_ai_tab_button_event_cb);
	g_dlva_btn = obj;

	if (scm_is_supported_dlva() || scm_is_supported_aicam() || scm_is_supported_aibox()) {
		if (ivsc.dfunc.support_dl_timeline) {
			nfui_nfobject_show(obj);
		}
	}

	if (scm_license_is_activated_dlva() || scm_license_is_activated_aicam_mask() || scm_license_is_activated_aibox()) {
		nfui_nfobject_enable(obj);
	}

	memset(strTime, 0x00, sizeof(strTime));
	g_vtDate = nftool_normal_button_create_popup_type2(strTime, 179);
	nfui_regi_post_event_callback(g_vtDate, post_date_button_event_cb);
	nfui_nfobject_show(g_vtDate);
	nfui_nffixed_put((NFFIXED*)fixed, g_vtDate, 6, 94);

	// zoom in & out
	pos_x = 6;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoomout);
	nfui_nfobject_set_size(obj, 44, 26);
	nfui_regi_post_event_callback(obj, post_zoomout_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, VT_WIN_SIZE_H-35);

	pos_x += (44 + 1);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoomin);
	nfui_nfobject_set_size(obj, 44, 26);
	nfui_regi_post_event_callback(obj, post_zoomin_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, VT_WIN_SIZE_H-35);

	// up & down
	pos_x += (44 + 2);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, up);
	nfui_nfobject_set_size(obj, 44, 26);
	nfui_regi_post_event_callback(obj, post_move_up_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, VT_WIN_SIZE_H-35);
	
	pos_x += (44 + 1);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, down);
	nfui_nfobject_set_size(obj, 44, 26);
	nfui_regi_post_event_callback(obj, post_move_down_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, VT_WIN_SIZE_H-35);

	nfui_nfwindow_add((NFWINDOW*)g_vtWin, fixed);

	// timeline

	memset(&figure, 0x00, sizeof(TML_FIGURE_T));
	figure.len = LEN_800;
	figure.type = TML_VERTICAL;
	figure.cr_bg = UX_COLOR(200);								// BG_TIMELINE
	figure.cr_rbar[0] = UX_COLOR(200);							// BG_TIMELINE
	figure.cr_rbar[1] = UX_COLOR(348);							// SBAR_TIMER
	figure.cr_rbar[2] = UX_COLOR(350);							// SBAR_ALARM
	figure.cr_rbar[3] = UX_COLOR(352);							// SBAR_MOT
	//figure.cr_rbar[4] = UX_COLOR(TODO_COLOR); 				// SBAR_USER
	figure.cr_rbar[4].red = 0x0000;
	figure.cr_rbar[4].green = 0x0000;
	figure.cr_rbar[4].blue = 0xff00;
	figure.cr_rbar[5] = UX_COLOR(354);							// SBAR_PANIC
	figure.cr_rbar[6] = UX_COLOR(356);							// SBAR_PRE
	figure.cr_rbar[7] = UX_COLOR(200);							// BT_TIMELINE
	figure.cr_rbar[8] = UX_COLOR(349);							// SBAR_TIMER_I
	figure.cr_rbar[9] = UX_COLOR(351);							// SBAR_ALARM_I
	figure.cr_rbar[10] = UX_COLOR(353);							// SBAR_MOT_I
	//figure.cr_rbar[11] = UX_COLOR(TODO_COLOR); 				// SBAR_USER_I
	figure.cr_rbar[11].red = 0x0000;
	figure.cr_rbar[11].green = 0x0000;
	figure.cr_rbar[11].blue = 0xff00;
	figure.cr_rbar[12] = UX_COLOR(355);							// SBAR_PANIC_I
	figure.cr_rbar[13] = UX_COLOR(357);							// SBAR_PRE_I
	figure.cr_ruler_bg = UX_COLOR(200);							// BG_TIMELINE
	figure.cr_ruler_text = UX_COLOR(347);						// RULER_TEXT
	//figure.cr_date = UX_COLOR(TODO_COLOR);					// DATE
	figure.cr_date = UX_COLOR(433);
	//figure.cr_date.red = 0xff00;
	//figure.cr_date.green = 0xff00;
	//figure.cr_date.blue = 0x0000;
	figure.cr_playbar = UX_COLOR(358);							// PLAYBAR
	figure.cr_cti = UX_COLOR(360);								// CTI
	figure.ruler_brd = 46;
	figure.ch_brd = (var_get_ch_count() == 16) ? 8 : 17;
	figure.ch_brd = (var_get_ch_count() == 32) ? 4 : figure.ch_brd;

	figure.pb_curtain = nfui_get_image_from_file(VRT_CURTAIN, NULL);

	tml = tml_new(g_vtWin);
	nfui_nfobject_set_size(tml, 180, 800);
	nfui_nffixed_put((NFFIXED*)fixed, tml, 6, 100);
	tml_init(tml, &figure, 6, 134, 180, 800, var_get_ch_count(), -1);
	tml_set_cti_position(tml, -1);
	tml_set_scale_depth(tml, 99);
	tml_change_cur_time_t(tml, time(0));
	tml_set_update_mode(tml, TML_AUTO_UPDATE);
	tml_set_style(tml, TML_GHOSTCTI);

	nfui_nfobject_show(tml);
	nfui_regi_post_event_callback(tml, post_timeline_event_cb);

	nfui_run_main_event_handler(g_vtWin);
	nfui_nfobject_show(g_vtWin);

	if(itd.disp_mode != TLINE_ALWAYS_ON) 									
		nfui_nfobject_hide(g_vtWin);
	
	nfui_make_key_hierarchy((NFWINDOW*)g_vtWin);

	// create popup menu
	VW_Timeline_PopUp_New(g_curwnd);
	VW_Calendar_Open(g_curwnd);

	nfui_page_open(PGID_LIVE_TIMELINE, g_vtWin, ssm_get_cur_id(NULL));


	uxm_reg_imsg_event(tml, INFY_TML_DATE_CHANGED);
	uxm_reg_imsg_event(g_vtWin, INFY_CALENDAR_CLOSED);
	uxm_reg_imsg_event(tml, INFY_TML_PLAY_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_DOUBLE_CLICKED);
	uxm_reg_imsg_event(g_vtWin, IRET_SCM_FORMAT_STORAGE);
	uxm_reg_imsg_event(g_vtWin, INFY_DISK_OW_NOTIFY);
	uxm_reg_imsg_event(g_vtWin, INFY_DSPDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(g_vtWin, INFY_SYSDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(tml, INFY_TML_START_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_END_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_UP);
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_DOWN);
	uxm_reg_imsg_event(g_vtWin, IRET_SCM_CHANGE_SYSTEM_TIME);

	uxm_monitor_on_imsg_event(g_vtWin, IRET_SCM_FORMAT_STORAGE);
	uxm_monitor_on_imsg_event(tml, INFY_TML_DATE_CHANGED);
	uxm_monitor_on_imsg_event(g_vtWin, INFY_DSPDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_vtWin, INFY_SYSDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_vtWin, IRET_SCM_CHANGE_SYSTEM_TIME);
	uxm_monitor_on_imsg_event(g_vtWin, INFY_DISK_OW_NOTIFY);

	_update_date();
	tml_refresh(tml);

	_init_worker();
}

int VW_Timeline_Close()
{
	if (!g_curwnd) return;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_Timeline_Show()
{
	if (!g_curwnd) return;

	if (scm_is_supported_dlva() || scm_is_supported_aicam() || scm_is_supported_aibox()) 
	{
		if (scm_license_is_activated_dlva() || scm_license_is_activated_aicam_mask() || scm_license_is_activated_aibox()) {
			nfui_nfobject_enable(g_dlva_btn);
			nfui_signal_emit(g_dlva_btn, GDK_EXPOSE, TRUE);
		}
	}

	if (!nfui_nfobject_is_shown(g_vtWin)) {
		if (g_shown_tab == 1) nfui_nfobject_show(g_vtWin);
	}
	
	VW_Timeline_DeepLearning_Show();
}

void VW_Timeline_Hide()
{
	if (!g_curwnd) return;
	if(VW_Calendar_IsShown())
		VW_Calendar_Hide();
	if(nfui_nfobject_is_shown(g_vtWin))
		nfui_nfobject_hide(g_vtWin);

	VW_Timeline_DeepLearning_Hide(); 
}

gboolean VW_Timeline_IsShown()
{
	gint is_shown = 0;
	if (!g_curwnd) return FALSE;

	if (nfui_nfobject_is_shown(g_vtWin)) is_shown = 1;
	if (VW_Timeline_DeepLearning_IsShown()) is_shown = 1;

	return is_shown;
}

gboolean VW_Timeline_IsInArea(guint x, guint y)
{
	if (!g_curwnd) return FALSE;
	if((x >= VT_WIN_POS_X) && (x <= VT_WIN_POS_X + VT_WIN_SIZE_W)) {
		if((y >= VT_WIN_POS_Y) && (y <= VT_WIN_POS_Y + VT_WIN_SIZE_H)) {
			return TRUE;
		}
	}

	return FALSE;
}

int VW_Timeline_ChangeMode(TLINE_MODE_E mode)
{
	GTimeVal tv;
	memset(&tv, 0x00, sizeof(GTimeVal));

	itd.mode = mode;
	VW_Timeline_DeepLearning_ChangeMode(mode);

	if (!g_curwnd) return -1;
	switch (itd.mode) {
	case TL_LIVE:
		tml_reset_style(tml, TML_GHOSTPLAYSTICK);
		tml_set_style(tml, TML_GHOSTCTI);
		tml_set_external_time_proc(tml, 0);
		tv.tv_sec = time(0);
		tml_set_cti_position(tml, -1);
		tml_change_cur_time(tml, &tv); 
		tml_set_update_interval(tml, 0);
		if(vsm_get_omode()) {			// full mode
			if(itd.disp_mode == TLINE_ALWAYS_OFF) 		VW_Timeline_Hide();
			else if(itd.disp_mode == TLINE_ALWAYS_ON) 	VW_Timeline_Show();
		}
		_update_date();
		break;
	case TL_PLAY:
		tml_set_style(tml, TML_GHOSTPLAYSTICK);
		tml_reset_style(tml, TML_GHOSTCTI);
		tml_set_external_time_proc(tml, _get_timeline_time);
		tml_set_update_interval(tml, 1000);
		tml_repaint(tml);
		if(!VW_Timeline_IsShown()) {
			if (g_shown_tab == 1) VW_Timeline_Show();
		}
		_update_date();
		break;
	}
	return 0;
}

int VW_Timeline_Set_Date(time_t ti, gboolean expose)
{
	if (!g_curwnd) return -1;
	tml_set_manual_paint_mode(tml, 1);
	tml_zoom_min(tml);
	tml_change_cur_day(tml, ti);	
	tml_set_playstick_time_t(tml, ti);
	tml_refresh(tml);
	tml_set_manual_paint_mode(tml, 0);
	_update_date();
	return 0;
}

gint VW_Timeline_get_disp_mode()
{
	if (!g_curwnd) return 0;
	return itd.disp_mode;
}
