
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "../viewers/objects/nfimage.h"

#include "ssm.h"
#include "vw_playback_main.h"
#include "vw_playback_internal.h"
#include "vw_playback_control_box.h"
#include "vw_playback_statusbar.h"
#include "vw_timeline.h"
#include "uxm.h"
#include "vw_snapshot.h"


#if defined(GUI_4CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			((50+1)*2)
#elif defined(GUI_8CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			(50+1)
#else
#define BOX_X_POSITION_CONTROL			(0)
#endif

#define CTL_MAIN_BOX_SIZE_W				(589)
#define CTL_MAIN_BOX_SIZE_H				(74)
#define CTL_MAIN_BOX_SIZE_X				(DISPLAY_ACTIVE_WIDTH - CTL_MAIN_BOX_SIZE_W - 5)
#define CTL_MAIN_BOX_SIZE_Y				(DISPLAY_ACTIVE_HEIGHT - CTL_MAIN_BOX_SIZE_H - 13)

#define CTL_DISP_BOX_SIZE_W				(272 - BOX_X_POSITION_CONTROL)
#define CTL_DISP_BOX_SIZE_H				(67)
#define CTL_DISP_BOX_SIZE_X				(CTL_MAIN_BOX_SIZE_X)
#define CTL_DISP_BOX_SIZE_Y				(CTL_MAIN_BOX_SIZE_Y - CTL_DISP_BOX_SIZE_H - 1)

#define CTL_FUNC_BOX_SIZE_W				(170)
#define CTL_FUNC_BOX_SIZE_H				(CTL_DISP_BOX_SIZE_H)
#define CTL_FUNC_BOX_SIZE_X				(CTL_DISP_BOX_SIZE_X + CTL_DISP_BOX_SIZE_W + 44 + BOX_X_POSITION_CONTROL)
#define CTL_FUNC_BOX_SIZE_Y				(CTL_DISP_BOX_SIZE_Y)

#define CTL_MAIN_DISP_BTN_POS_X			(9)
#define CTL_MAIN_FUNC_BTN_POS_X			(427)



enum {
	CTL_BOX_OUT_NOTI = 0,
	CTL_BOX_IN_NOTI,
	TBUTTON_OUT_NOTI
};

static GdkPixbuf *bookmark_img[2][NFOBJECT_STATE_COUNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *fixed3 = NULL;
static NFOBJECT *ctl_box_win = NULL;
static NFOBJECT *func_box_win = NULL;

static NFOBJECT* g_bookmark_obj;
static NFOBJECT* g_snap_obj;
static NFOBJECT *wait_pop = NULL;

static guint funcbutton_pos_x = 0;

static guint ctl_box_mv_x = 0;
static guint ctl_box_mv_y = 0;

static guint bm_timer_id = 0;


////////////////////////////////////////////////////////////
//
// PLAYBACK CONTROL BOX - FUNC BOX 
//

static void _change_zoombutton(gint status)
{
	NFOBJECT* obj;

	obj = (NFOBJECT*)nfui_nfobject_get_data(func_box_win, "zoom button");

	if (status)
	{
		if (nfui_nfobject_is_disabled(obj))
		{
			nfui_nfobject_enable(obj);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}
	else
	{
		if (!nfui_nfobject_is_disabled(obj))
		{
			nfui_nfobject_disable(obj);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}	
}

static gboolean _display_bookmark_status(gpointer data)
{
	static gint status = 0;

	if(status == 0)         status = 1;
	else if(status == 1)    status = 0;

	nfui_nfbutton_set_image(NF_BUTTON(g_bookmark_obj), bookmark_img[status]);
	nfui_signal_emit(g_bookmark_obj, GDK_EXPOSE, FALSE);

	return TRUE;
}

static void _reset_bookmark_status(gpointer data)
{
	nfui_nfbutton_set_image(NF_BUTTON(g_bookmark_obj), bookmark_img[0]);
	nfui_signal_emit(g_bookmark_obj, GDK_EXPOSE, FALSE);
}

static guint _mouse_notify_detect(guint x_root, guint y_root, guint x_param)
{
	if((x_root > ctl_box_mv_x && x_root < ctl_box_mv_x + CTL_MAIN_BOX_SIZE_W) 
		&& (y_root > ctl_box_mv_y && y_root < ctl_box_mv_y + CTL_MAIN_BOX_SIZE_H))
	{
		if((x_root < x_param)
				|| (x_root > x_param + 50))
			return TBUTTON_OUT_NOTI;

		return CTL_BOX_IN_NOTI;
	}
	return CTL_BOX_OUT_NOTI;		
}

static gboolean _func_box_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	GdkEventMotion *m_event;
	guint result;

	m_event = (GdkEventMotion*)event;

	switch(event->type) {
		case GDK_EXPOSE:
		break;

		case NFOUTEVT_MOTION_NOTIFY:
			result = _mouse_notify_detect((guint)m_event->x_root, (guint)m_event->y_root, funcbutton_pos_x);
			
			switch(result)
			{
				case CTL_BOX_IN_NOTI:
				case CTL_BOX_OUT_NOTI:
				break;
				case TBUTTON_OUT_NOTI:
					if (nfui_nfobject_is_shown(obj))
					{
						nfui_nfobject_hide(obj);
						nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, obj);
					}
				break;
				default:
				break;
			}
			break;

		case NFOUTEVT_BUTTON_PRESS:
			if (nfui_nfobject_is_shown(obj))
			{
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, obj);
			}
			break;	
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_EXIT)
			{
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, obj);
				return TRUE;				
			}
		}
		break;					
		case GDK_DELETE:
			if(bm_timer_id)
			{
				g_source_remove(bm_timer_id);
				bm_timer_id = 0;
			}

			nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, obj);
			g_curwnd = 0;
			break;

		default:
			break;
	}

	return FALSE;

}

static gboolean _pb_zoom_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *div_box_win;
	NFOBJECT *ctl_box_win;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		div_box_win = vw_playback_get_div_box();
		ctl_box_win = vw_playback_get_control_box();

		if (nfui_nfobject_is_shown(div_box_win))
		{
			nfui_nfobject_hide(div_box_win);
			nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, div_box_win);
		}

		if (nfui_nfobject_is_shown(func_box_win))
		{
			nfui_nfobject_hide(func_box_win);
			nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, func_box_win);
		}

		if (nfui_nfobject_is_shown(ctl_box_win))
		{
			nfui_nfobject_hide(ctl_box_win);		
			nfui_page_close(PGID_PLAYBACK_CONTROL_BOX, ctl_box_win);
		}
		
		vw_playback_full_zoom_func();
		
		nfui_nfobject_show(ctl_box_win);
		nfui_page_open(PGID_PLAYBACK_CONTROL_BOX, ctl_box_win, nfui_get_last_user());		
	}	
	return FALSE;
}

static gboolean _pb_snapshot_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        gint ch = vsm_get_focused_channel();

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        if (vw_playback_snap_func(ch))
        {
			wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
			uxm_reg_imsg_event(fixed3, INFY_CAPTURE_IMAGE);
			uxm_monitor_on_imsg_event(fixed3, INFY_CAPTURE_IMAGE);
        }
	}	
	return FALSE;
}


static gboolean _pb_archive_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean ret;
	gboolean use_dl;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        DAL_get_use_double_login(&use_dl);

        if (use_dl && !ssm_is_admin())
        {
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
        }

		ret = vw_playback_archive_func();
		if (ret) uxm_reg_imsg_event(fixed3, INFY_QUERY_OVER);
		else uxm_unreg_imsg_event(fixed3, INFY_QUERY_OVER);
	}	
	return FALSE;
}

static gboolean _ctl_box_fixed3_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	char msgbuf[256];
	gboolean ret;
	
	switch(event->type) {
		case GDK_EXPOSE:
		{
			gchar name[32];
			GdkGC *gc;
			GdkDrawable *drawable;
			
			g_sprintf(name, "MKB_IMG_PB_CB_BG_02_%d", CTL_FUNC_BOX_SIZE_W);			
			
			nf_ui_create_image_button_method(name, CTL_FUNC_BOX_SIZE_W, IMG_PLAY_FULL_BG_02_01, IMG_PLAY_FULL_BG_02_02, IMG_PLAY_FULL_BG_02_03);

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			nfutil_draw_image(drawable, gc, name, obj->x, obj->y, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);

			
		}
		break;

		case GDK_BUTTON_RELEASE:
		break;					


		case INFY_DIV_CHANGE:
		{
			CMM_MESSAGE_T *pmsg;
			gint div;

			pmsg = (CMM_MESSAGE_T *)data;
			div = GPOINTER_TO_INT(pmsg->data);

			if (div == VSM_DIV1)
				_change_zoombutton(1);
			else
				_change_zoombutton(0);
		}
		break;

		case INFY_CAPTURE_IMAGE:
		{
			gint result = ((CMM_MESSAGE_T *)data)->param;

			uxm_unreg_imsg_event(fixed3, INFY_CAPTURE_IMAGE);

			if (wait_pop) {
				nftool_remove_waitbox(wait_pop);
				wait_pop = NULL;
			}

			if (result == 0) 
			{
				CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
				SNAPSHOT_INFO_T info;
	
				info.ch = image->ch;
				info.time = image->time;
				info.size = image->size;
				info.width = image->width;
				info.height = image->height;
				info.buffer = image->buffer;

				VW_Snapshot_Open(g_curwnd, &info, SS_MODE_BURN_RESERVE);
			}
			else
				nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);

			vsm_playback_play_recover_by_menu_closed();				
		}
		break;			

		case INFY_QUERY_OVER:
			uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);
			ret = vw_playback_archive_func();
			if (ret) uxm_reg_imsg_event(obj, INFY_QUERY_OVER);
			break;


		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);
			uxm_unreg_imsg_event(obj, INFY_DIV_CHANGE);
			uxm_unreg_imsg_event(obj, INFY_PB_IMAGE_STATUS);
			uxm_unreg_imsg_event(obj, INFY_CAPTURE_IMAGE);						
		break;

		default:
		break;
	}

	return FALSE;

}


/////////////////////////////////////////////////////////////////
//
//
//



NFOBJECT* vw_playback_control_box_func_open(NFWINDOW *parent)
{
	NFOBJECT *nfwidget;

	GdkPixbuf *zoom_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *snap_img[NFOBJECT_STATE_COUNT];
	
	guint pos_x = 0;
	guint size_w, size_h;

/*<---------- load images */	
	zoom_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_ZOOM, NULL);
	zoom_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_ZOOM, NULL);
	zoom_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_ZOOM, NULL);
	zoom_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_ZOOM, NULL);
	
	snap_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_SNAP, NULL);
	snap_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_SNAP, NULL);
	snap_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_SNAP, NULL);
	snap_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_SNAP, NULL);
	
	bookmark_img[0][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_BOOKMARK_OFF, NULL);
	bookmark_img[0][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_BOOKMARK_OFF, NULL);
	bookmark_img[0][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_BOOKMARK_OFF, NULL);
	bookmark_img[0][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_BOOKMARK_OFF, NULL);

	bookmark_img[1][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_BOOKMARK_ON, NULL);
	bookmark_img[1][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_BOOKMARK_ON, NULL);
	bookmark_img[1][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_BOOKMARK_ON, NULL);
	bookmark_img[1][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_BOOKMARK_ON, NULL);

	func_box_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTL_FUNC_BOX_SIZE_X, (guint)CTL_FUNC_BOX_SIZE_Y, (guint)CTL_FUNC_BOX_SIZE_W, (guint)CTL_FUNC_BOX_SIZE_H);
	nfui_nfobject_modify_bg(func_box_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfwindow_use_outside_evt((NFWINDOW*)func_box_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)func_box_win, GDK_MOTION_NOTIFY, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)func_box_win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_pre_event_callback(func_box_win, _func_box_win_event_handler);
	g_curwnd = func_box_win;

	fixed3 = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_pre_event_callback(fixed3, _ctl_box_fixed3_event_handler);
	nfui_nfobject_set_size(fixed3, CTL_FUNC_BOX_SIZE_W, CTL_FUNC_BOX_SIZE_H);
	nfui_nfobject_modify_bg(fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed3);



// make zoom button 
	pos_x = 9;
	
	nfui_get_image_size(IMG_N_PLAY_FULL_ZOOM, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), zoom_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed3, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _pb_zoom_event_handler);
	nfui_nfobject_disable(nfwidget);
	nfui_nfobject_set_data(func_box_win, "zoom button", nfwidget);

// make snap button 
	pos_x += (size_w + 1);
	
	nfui_get_image_size(IMG_N_PLAY_FULL_SNAP, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), snap_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed3, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _pb_snapshot_event_handler);
	g_snap_obj = nfwidget;
#ifndef _SUPPORT_SNAPSHOT	
	nfui_nfobject_disable(nfwidget);
#endif
	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(nfwidget);

// make arch button 
	pos_x += (size_w + 1);
	
	nfui_get_image_size(IMG_N_PLAY_FULL_BOOKMARK_OFF, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), bookmark_img[0]);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed3, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _pb_archive_event_handler);
	g_bookmark_obj = nfwidget;

	if (!ssm_check_access_auth(USR_AUTH_ARCHIVE))
	    nfui_nfobject_disable(g_bookmark_obj);

	uxm_reg_imsg_event(fixed3, INFY_DIV_CHANGE);
	uxm_monitor_on_imsg_event(fixed3, INFY_DIV_CHANGE);
	uxm_reg_imsg_event(fixed3, INFY_PB_IMAGE_STATUS);
	uxm_monitor_on_imsg_event(fixed3, INFY_PB_IMAGE_STATUS);

	nfui_nfwindow_add((NFWINDOW*)func_box_win, fixed3);
	nfui_run_main_event_handler(func_box_win);

	nfui_nfobject_show(func_box_win);
	nfui_make_key_hierarchy((NFWINDOW*)func_box_win);
	nfui_nfobject_hide(func_box_win);

	return func_box_win;
}

void vw_playback_control_box_change_bookmark_img(gint status)
{
	if (status)
		bm_timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, _display_bookmark_status, NULL, _reset_bookmark_status);
	else
	{
		if(bm_timer_id) 
		{
			g_source_remove(bm_timer_id);
			bm_timer_id = 0;
		}
	}
}

void vw_playback_control_box_change_snap(gint enable)
{
#ifdef _SUPPORT_SNAPSHOT	
	if (enable) nfui_nfobject_enable(g_snap_obj);
	else		nfui_nfobject_disable(g_snap_obj);

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_snap_obj);

	nfui_signal_emit(g_snap_obj, GDK_EXPOSE, TRUE);
#endif		
}

