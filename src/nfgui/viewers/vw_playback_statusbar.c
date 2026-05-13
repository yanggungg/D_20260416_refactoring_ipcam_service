
/* 
 * vw_live_statusbar.c
 *
 * written by seongho
 *
 */



#include <time.h>

#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
// TODO: renaming..
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

#include "vsm.h"
#include "dtf.h"
#include "stm.h"
#include "uxm.h"
#include "ssm.h"
#include "vw_playback_main.h"
#include "vw_playback_internal.h"
#include "vw_playback_statusbar.h"
#include "vw_playback_control_box.h"
#include "vw_timeline.h"
#include "vw_playback_start_menu.h"
#include "vw_snapshot.h"

#include "wnd.h"

#define LIVE_WIN_SIZE_W				DISPLAY_ACTIVE_WIDTH
#define LIVE_WIN_SIZE_H				DISPLAY_ACTIVE_HEIGHT

#define CTRLBAR_SIZE_W				LIVE_WIN_SIZE_W
#define CTRLBAR_SIZE_H				(108)
#define CTRLBAR_POS_X				(0)
#define CTRLBAR_POS_Y				(LIVE_WIN_SIZE_H - CTRLBAR_SIZE_H)

#define TIMELABEL_SIZE_X			(624)
#define TIMELABEL_SIZE_W			(442)
#define TIMELABEL_SIZE_H			(34)

#define INACT_TIMELABEL_SIZE_W		(230)
#define INACT_TIMELABEL_SIZE_H		(40)

// AM/PM label size
#define AMPM_LABEL_W				(70)
#define AMPM_LABEL_H				(35)

enum {
	T24_HOUR_FORMAT = 0,
	T12_HOUR_FORMAT 
};

static GdkPixbuf *osd_img[2][NFOBJECT_STATE_COUNT];
static GdkPixbuf *bookmark_img[2][NFOBJECT_STATE_COUNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ctrl_win = NULL;	
static NFOBJECT* g_osd_obj;
static NFOBJECT* g_snap_obj;
static NFOBJECT* g_bookmark_obj;
static NFOBJECT* g_date_label;
static NFOBJECT* g_time_label;
static NFOBJECT* g_status;

static guint st_timer_id = 0;
static guint bm_timer_id = 0;

static DIR_RATE_E pre_dir_rate;
static GTimeVal pre_time;
static NFOBJECT *ctrl_fixed1;
static NFOBJECT *wait_pop = NULL;

static gboolean ctrl_win_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data);


static void playback_full_mode()
{
	vw_playback_statusbar_playstatus_end();
	vsm_change_full_mode();
	vw_playback_statusbar_hide();
	VW_Timeline_Hide();
	vw_playback_control_box_Show();	
}

static void _change_zoombutton(gint status)
{
	NFOBJECT* obj;

	obj = (NFOBJECT*)nfui_nfobject_get_data(g_ctrl_win, "zoom button");

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


static gboolean _set_dir_rate(DIR_RATE_E status)
{
	if (pre_dir_rate == status)
		return FALSE;

	if (status == DR_BWD_001) 				nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_001);
	else if (status == DR_BWD_002) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_002);
	else if (status == DR_BWD_004) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_004);
	else if (status == DR_BWD_008) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_008);
	else if (status == DR_BWD_016) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_016);
	else if (status == DR_BWD_032) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_032);
	else if (status == DR_BWD_064) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_064);
	else if (status == DR_BWD_128) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_BWD_128);
	else if (status == DR_FWD_001) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_001);
	else if (status == DR_FWD_002) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_002);
	else if (status == DR_FWD_004) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_004);
	else if (status == DR_FWD_008) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_008);
	else if (status == DR_FWD_016) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_016);
	else if (status == DR_FWD_032) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_032);
	else if (status == DR_FWD_064) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_064);
	else if (status == DR_FWD_128) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_FWD_128);
	else if (status == DR_BWD_SLOW_002) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_002);
	else if (status == DR_BWD_SLOW_004) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_004);
	else if (status == DR_BWD_SLOW_008) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_008);
	else if (status == DR_BWD_SLOW_016) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_016);
	else if (status == DR_BWD_SLOW_032) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_032);
	else if (status == DR_BWD_SLOW_064) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_064);
	else if (status == DR_BWD_SLOW_128) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_BWD_128);
	else if (status == DR_FWD_SLOW_002) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_002);
	else if (status == DR_FWD_SLOW_004) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_004);
	else if (status == DR_FWD_SLOW_008) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_008);
	else if (status == DR_FWD_SLOW_016) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_016);
	else if (status == DR_FWD_SLOW_032) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_032);
	else if (status == DR_FWD_SLOW_064) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_064);
	else if (status == DR_FWD_SLOW_128) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_SLOW_FWD_128);
	else if (status == DR_BWD_NEXT_FRAME) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_NEXT_BWD);
	else if (status == DR_FWD_NEXT_FRAME) 	nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_NEXT_FWD);
	else if (status == DR_PAUSE) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_PAUSE);
	else if (status == DR_STOP) 			nfui_nfimage_change_image((NFIMAGE*)g_status, IMG_PLAY_STATUS_STOP);
	else g_message("%s, %d, %08X not yet", __FUNCTION__, __LINE__, status);

	pre_dir_rate = status;

	return TRUE;
}

static void _update_dir_rate()
{
	nfui_signal_emit(g_status, GDK_EXPOSE, FALSE);
}

static gboolean _set_date_time(GTimeVal cur_time, gchar *strTime)
{
	gchar buf[64];

	if ((pre_time.tv_sec != cur_time.tv_sec) &&(cur_time.tv_sec != 0))
	{
		dtf_get_local_datetime(cur_time.tv_sec, strTime);
		pre_time.tv_sec = cur_time.tv_sec;
		return TRUE;
	}
	else
		return FALSE;
}

static void _update_playtime(char *text) 
{
	gchar *str = NULL;

	text[10] = '\0';

#if 1
	// date
	str = nfui_nfimage_get_text((NFIMAGE*)g_date_label);
	if(str) {
		if(strcmp(str, text)) {
			nfui_nfimage_set_text((NFIMAGE*)g_date_label, text);
			nfui_signal_emit(g_date_label, GDK_EXPOSE, TRUE);
		}
	}else {
		nfui_nfimage_set_text((NFIMAGE*)g_date_label, text);
		nfui_signal_emit(g_date_label, GDK_EXPOSE, TRUE);
	}

	// time
	str = nfui_nfimage_get_text((NFIMAGE*)g_time_label);
	if(str) {
		if(strcmp(str, &text[11])) {
			nfui_nfimage_set_text((NFIMAGE*)g_time_label, &text[11]);
			nfui_signal_emit(g_time_label, GDK_EXPOSE, TRUE);
		}
	}
	else {
		nfui_nfimage_set_text((NFIMAGE*)g_time_label, &text[11]);
		nfui_signal_emit(g_time_label, GDK_EXPOSE, TRUE);
	}
#else
	nfui_nflabel_set_text((NFLABEL*)g_date_label, text);
	nfui_nflabel_set_text((NFLABEL*)g_time_label, &text[11]);

	nfui_signal_emit(g_date_label, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_time_label, GDK_EXPOSE, TRUE);
#endif
}

static gboolean _statusbar_update_playstatus(gpointer data)
{
	DIR_RATE_E play_dir_rate;
	GTimeVal play_time;
	gchar buf[64];

	memset(&play_time, 0x00, sizeof(GTimeVal));
	play_dir_rate = vsm_playback_get_dir_rate();
	play_time = vsm_playback_get_playtime();
	stm_set_time_t(play_time.tv_sec);

	if (nfui_nfobject_is_shown(g_ctrl_win))
	{
		if (_set_dir_rate(play_dir_rate))
			_update_dir_rate(play_dir_rate);
	
		if (_set_date_time(play_time, buf))
			_update_playtime(buf);
	}

	return TRUE;
}

static gboolean _display_bookmark_status(gpointer data)
{
	static gint status = 0;

	if(status == 0)			status = 1;
	else if(status == 1)	status = 0;

	nfui_nfbutton_set_image(NF_BUTTON(g_bookmark_obj), bookmark_img[status]);
	nfui_signal_emit(g_bookmark_obj, GDK_EXPOSE, FALSE);

	return TRUE;
}

static void _reset_bookmark_status(gpointer data)
{
	nfui_nfbutton_set_image(NF_BUTTON(g_bookmark_obj), bookmark_img[0]);
	nfui_signal_emit(g_bookmark_obj, GDK_EXPOSE, FALSE);
}


static gboolean ctrl_win_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
			vw_playback_statusbar_playstatus_start();
		break;
		case GDK_DELETE:
		{
			if(bm_timer_id) 
			{
				g_source_remove(bm_timer_id);
				bm_timer_id = 0;
			}

			g_curwnd = 0;

			nfui_page_close(PGID_PLAYBACK_CTRLBAR, obj);
			vw_playback_statusbar_playstatus_end();
		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;
			NFWINDOW *fnd;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if ((fnd = wnd_find_window("PLAYBACK SHORTCUT"))) {
				nfui_nfobject_hide((NFOBJECT *)fnd);
			}

			if (kpid == RMC_MENU)
			{
				if (scm_is_bookmarking()) {
					nftool_mbox(g_curwnd, "WARNING", "It can't move to another menu while archiving.", NFTOOL_MB_OK);
					return FALSE;
				}
				vw_playback_start_menu_refresh();
			}
			else if (kpid == KEYPAD_SEARCH)
			{
				vw_playback_out_search_open();
			}
			else if (kpid == KEYPAD_ARCH)
			{
				if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
					break;
					
				vw_playback_out_arch_open();
			}	
			else if (kpid == RMC_RESERVE)
			{
				gboolean ret;
				
				if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
					break;
			
				ret = vw_playback_archive_func();
				if (ret) uxm_reg_imsg_event(ctrl_fixed1, INFY_QUERY_OVER);
				else uxm_unreg_imsg_event(ctrl_fixed1, INFY_QUERY_OVER);
			}	
			else if (kpid == KEYPAD_ZOOM)
			{
        		NFOBJECT *top;			

				if(vsm_get_div() == VSM_DIV1) 
				{
            		top = nfui_nfobject_get_top(obj);		
            		nfui_nfobject_hide(top);
            		VW_Timeline_Hide();			
            		
            		vw_playback_full_zoom_func();
            		nfui_nfobject_show(top);		
            		VW_Timeline_Show();
				}        		
			}
			else if (kpid == RMC_SNAPSHOT)
			{
				guint ch;

				ch = vsm_get_focused_channel();

				if (vw_playback_snap_func(ch))
				{
					wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
					uxm_reg_imsg_event(ctrl_fixed1, INFY_CAPTURE_IMAGE);
					uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_CAPTURE_IMAGE);
				}
			}			
			else	
			{
				vw_playback_process_common_KeyEvent(kpid);
			}
		}
		break;		
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_EXIT)
			{
				vw_playback_out_opened();
				return TRUE;
			}
		}
		break;		
		case NFEVENT_JOG_CHANGE:
		{
			GdkEventKey *kevt;
			guint code;

			kevt = (GdkEventKey*)event;
			code = kevt->keyval;

			vw_playback_process_common_JogEvent(code);
		}		
		break;
		case NFEVENT_SHUTTLE_CHANGE:
		{
			GdkEventKey *kevt;
			guint code;

			kevt = (GdkEventKey*)event;
			code = kevt->keyval;

			vw_playback_process_common_ShuttleEvent(code);
		}		
		break;
		
		default:
		break;
	}

	return FALSE;
}


static gboolean ctrl_fixed_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	char msgbuf[256];
	gboolean ret;

	switch(event->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable = NULL;
			GdkGC* gc = NULL;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfutil_draw_image(drawable, gc, IMG_STATUSBAR_BG, 
				(gint)(obj->x), (gint)(obj->y), (gint)(obj->width), (gint)(obj->height), NFALIGN_LEFT, 0);

			nfui_nfobject_gc_unref(gc);
		}
		break;

		case INFY_QUERY_OVER:
			uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);
			ret = vw_playback_archive_func();
			if (ret) uxm_reg_imsg_event(obj, INFY_QUERY_OVER);
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

		case INFY_PB_IMAGE_STATUS:
		{
			CMM_MESSAGE_T *pmsg;
			gint status;
			NFOBJECT* b_obj;
			NFOBJECT* f_obj;

			pmsg = (CMM_MESSAGE_T *)data;			
			status = GPOINTER_TO_INT(pmsg->data);

			b_obj = (NFOBJECT*)nfui_nfobject_get_data(g_ctrl_win, "step backward button");
			f_obj = (NFOBJECT*)nfui_nfobject_get_data(g_ctrl_win, "step forward button");

			if (status == 1)		//stop
			{
				nfui_nfobject_disable(b_obj);
				nfui_nfobject_disable(f_obj);

				nfui_signal_emit(b_obj, GDK_EXPOSE, FALSE);
				nfui_signal_emit(f_obj, GDK_EXPOSE, FALSE);		
			}		
			else					//run
			{
				nfui_nfobject_enable(b_obj);
				nfui_nfobject_enable(f_obj);

				nfui_signal_emit(b_obj, GDK_EXPOSE, FALSE);
				nfui_signal_emit(f_obj, GDK_EXPOSE, FALSE);
			}	
		}
		break;
		
		case INFY_CAPTURE_IMAGE:
		{
			gint result = ((CMM_MESSAGE_T *)data)->param;

			uxm_unreg_imsg_event(ctrl_fixed1, INFY_CAPTURE_IMAGE);

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
		
		case GDK_MOTION_NOTIFY:
		{

		}
		break;
		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);
			uxm_unreg_imsg_event(obj, INFY_DIV_CHANGE);
			uxm_unreg_imsg_event(obj, INFY_PB_IMAGE_STATUS);			
			uxm_unreg_imsg_event(obj, INFY_CAPTURE_IMAGE);			
		}
		break;

		default:
		break;
	}

	return FALSE;
}


static gboolean _pb_start_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (scm_is_bookmarking()) {
			nftool_mbox(g_curwnd, "WARNING", "It can't move to another menu while archiving.", NFTOOL_MB_OK);
			return FALSE;
		}
		vw_playback_start_menu_refresh();
	}	
	return FALSE;
}

static gboolean _full_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		playback_full_mode();
	}	
	return FALSE;
}

static gboolean _pb_zoom_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
	
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		top = nfui_nfobject_get_top(obj);		
		nfui_nfobject_hide(top);
		VW_Timeline_Hide();			
		
		vw_playback_full_zoom_func();
		nfui_nfobject_show(top);		
		VW_Timeline_Show();			
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
			uxm_reg_imsg_event(ctrl_fixed1, INFY_CAPTURE_IMAGE);
			uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_CAPTURE_IMAGE);
        }
	}	
	return FALSE;
}


static gboolean _pb_archive_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		ret = vw_playback_archive_func();
		if (ret) uxm_reg_imsg_event(ctrl_fixed1, INFY_QUERY_OVER);
		else uxm_unreg_imsg_event(ctrl_fixed1, INFY_QUERY_OVER);

	}	
	return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//
//
//


NFOBJECT* vw_playback_statusbar_open(NFWINDOW *parent)
{
	NFOBJECT *ctrl_fixed2;
	NFOBJECT *nfwidget;
	NFOBJECT *focus;

	gint size_w, size_h;
	guint pos_x = 0;
	guint pwr_btn_w, pwr_btn_h;
	guint main_btn_w, main_btn_h;
	gint i, btn_cnt;

	time_t init_time;
	struct tm time_info;

	GdkGC *pmgc;

	GdkPixbuf *start_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *div_img[7][NFOBJECT_STATE_COUNT];
	GdkPixbuf *play_attr_img[7][NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoom_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *snapshot_img[NFOBJECT_STATE_COUNT];

	GdkPixbuf *pbBG = NULL;
	GdkPixbuf *bgImg = NULL;

	guint btn_fg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(500), 
										COLOR_IDX(501), 
										COLOR_IDX(503), 
										COLOR_IDX(504)};

/*<---------- load images */
	start_img[0] = nfui_get_image_from_file((IMG_BTN_N_PB_START), NULL);
	start_img[1] = nfui_get_image_from_file((IMG_BTN_O_PB_START), NULL);
	start_img[2] = nfui_get_image_from_file((IMG_BTN_P_PB_START), NULL);
	start_img[3] = nfui_get_image_from_file((IMG_BTN_D_PB_START), NULL);

	div_img[0][0] = nfui_get_image_from_file((IMG_N_DIV_FULL), NULL);
	div_img[0][1] = nfui_get_image_from_file((IMG_P_DIV_FULL), NULL);
	div_img[0][2] = nfui_get_image_from_file((IMG_O_DIV_FULL), NULL);
	div_img[0][3] = nfui_get_image_from_file((IMG_D_DIV_FULL), NULL);

	div_img[1][0] = nfui_get_image_from_file((IMG_N_DIV_1), NULL);
	div_img[1][1] = nfui_get_image_from_file((IMG_P_DIV_1), NULL);
	div_img[1][2] = nfui_get_image_from_file((IMG_O_DIV_1), NULL);
	div_img[1][3] = nfui_get_image_from_file((IMG_D_DIV_1), NULL);

	div_img[2][0] = nfui_get_image_from_file((IMG_N_DIV_4), NULL);
	div_img[2][1] = nfui_get_image_from_file((IMG_P_DIV_4), NULL);
	div_img[2][2] = nfui_get_image_from_file((IMG_O_DIV_4), NULL);
	div_img[2][3] = nfui_get_image_from_file((IMG_D_DIV_4), NULL);

	div_img[3][0] = nfui_get_image_from_file((IMG_N_DIV_9), NULL);
	div_img[3][1] = nfui_get_image_from_file((IMG_P_DIV_9), NULL);
	div_img[3][2] = nfui_get_image_from_file((IMG_O_DIV_9), NULL);
	div_img[3][3] = nfui_get_image_from_file((IMG_D_DIV_9), NULL);

	div_img[4][0] = nfui_get_image_from_file((IMG_N_DIV_16), NULL);
	div_img[4][1] = nfui_get_image_from_file((IMG_P_DIV_16), NULL);
	div_img[4][2] = nfui_get_image_from_file((IMG_O_DIV_16), NULL);
	div_img[4][3] = nfui_get_image_from_file((IMG_D_DIV_16), NULL);

	div_img[5][0] = nfui_get_image_from_file((IMG_N_DIV_32), NULL);
	div_img[5][1] = nfui_get_image_from_file((IMG_P_DIV_32), NULL);
	div_img[5][2] = nfui_get_image_from_file((IMG_O_DIV_32), NULL);
	div_img[5][3] = nfui_get_image_from_file((IMG_D_DIV_32), NULL);

	div_img[6][0] = nfui_get_image_from_file((IMG_N_DIV_USER), NULL);
	div_img[6][1] = nfui_get_image_from_file((IMG_O_DIV_USER), NULL);
	div_img[6][2] = nfui_get_image_from_file((IMG_P_DIV_USER), NULL);
	div_img[6][3] = nfui_get_image_from_file((IMG_D_DIV_USER), NULL);

	osd_img[0][0] = nfui_get_image_from_file((IMG_N_OSD_OFF), NULL);
	osd_img[0][1] = nfui_get_image_from_file((IMG_O_OSD_OFF), NULL);
	osd_img[0][2] = nfui_get_image_from_file((IMG_P_OSD_OFF), NULL);
	osd_img[0][3] = nfui_get_image_from_file((IMG_D_OSD_OFF), NULL);

	osd_img[1][0] = nfui_get_image_from_file((IMG_N_OSD_ON), NULL);
	osd_img[1][1] = nfui_get_image_from_file((IMG_O_OSD_ON), NULL);
	osd_img[1][2] = nfui_get_image_from_file((IMG_P_OSD_ON), NULL);
	osd_img[1][3] = nfui_get_image_from_file((IMG_D_OSD_ON), NULL);

	play_attr_img[0][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_STEP_BACK), NULL);
	play_attr_img[0][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_STEP_BACK), NULL);
	play_attr_img[0][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_STEP_BACK), NULL);
	play_attr_img[0][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_STEP_BACK), NULL);

	play_attr_img[1][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_DS_BACK), NULL);
	play_attr_img[1][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_DS_BACK), NULL);
	play_attr_img[1][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_DS_BACK), NULL);
	play_attr_img[1][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_DS_BACK), NULL);

	play_attr_img[2][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_BACK), NULL);
	play_attr_img[2][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_BACK), NULL);
	play_attr_img[2][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_BACK), NULL);
	play_attr_img[2][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_BACK), NULL);

	play_attr_img[3][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_PAUSE), NULL);
	play_attr_img[3][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_PAUSE), NULL);
	play_attr_img[3][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_PAUSE), NULL);
	play_attr_img[3][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_PAUSE), NULL);

	play_attr_img[4][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_FORWARD), NULL);
	play_attr_img[4][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_FORWARD), NULL);
	play_attr_img[4][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_FORWARD), NULL);
	play_attr_img[4][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_FORWARD), NULL);

	play_attr_img[5][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_DS_FORWARD), NULL);
	play_attr_img[5][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_DS_FORWARD), NULL);
	play_attr_img[5][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_DS_FORWARD), NULL);
	play_attr_img[5][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_DS_FORWARD), NULL);

	play_attr_img[6][0] = nfui_get_image_from_file((IMG_N_PLAY_ATTR_STEP_FORWARD), NULL);
	play_attr_img[6][1] = nfui_get_image_from_file((IMG_O_PLAY_ATTR_STEP_FORWARD), NULL);
	play_attr_img[6][2] = nfui_get_image_from_file((IMG_P_PLAY_ATTR_STEP_FORWARD), NULL);
	play_attr_img[6][3] = nfui_get_image_from_file((IMG_D_PLAY_ATTR_STEP_FORWARD), NULL);

	zoom_img[0] = nfui_get_image_from_file((IMG_N_BTN_ZOOM), NULL);
	zoom_img[1] = nfui_get_image_from_file((IMG_O_BTN_ZOOM), NULL);
	zoom_img[2] = nfui_get_image_from_file((IMG_P_BTN_ZOOM), NULL);
	zoom_img[3] = nfui_get_image_from_file((IMG_D_BTN_ZOOM), NULL);

	snapshot_img[0] = nfui_get_image_from_file((IMG_N_BTN_SNAPSHOT), NULL);
	snapshot_img[1] = nfui_get_image_from_file((IMG_O_BTN_SNAPSHOT), NULL);
	snapshot_img[2] = nfui_get_image_from_file((IMG_P_BTN_SNAPSHOT), NULL);
	snapshot_img[3] = nfui_get_image_from_file((IMG_D_BTN_SNAPSHOT), NULL);

	bookmark_img[0][0] = nfui_get_image_from_file((IMG_N_BTN_BOOKMARK_OFF), NULL);
	bookmark_img[0][1] = nfui_get_image_from_file((IMG_O_BTN_BOOKMARK_OFF), NULL);
	bookmark_img[0][2] = nfui_get_image_from_file((IMG_P_BTN_BOOKMARK_OFF), NULL);
	bookmark_img[0][3] = nfui_get_image_from_file((IMG_D_BTN_BOOKMARK_OFF), NULL);

	bookmark_img[1][0] = nfui_get_image_from_file((IMG_N_BTN_BOOKMARK_ON), NULL);
	bookmark_img[1][1] = nfui_get_image_from_file((IMG_O_BTN_BOOKMARK_ON), NULL);
	bookmark_img[1][2] = nfui_get_image_from_file((IMG_P_BTN_BOOKMARK_ON), NULL);
	bookmark_img[1][3] = nfui_get_image_from_file((IMG_D_BTN_BOOKMARK_ON), NULL);


	/* init data */
	memset(&pre_time, 0, sizeof(GTimeVal));
	pre_dir_rate = DR_NONE;

	/* ctrl window */
	g_ctrl_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTRLBAR_POS_X, (guint)CTRLBAR_POS_Y, (guint)CTRLBAR_SIZE_W, (guint)CTRLBAR_SIZE_H);
	g_curwnd = g_ctrl_win;
	nfui_nfwindow_set_title(g_ctrl_win, "PLAYBACK STATUS BAR");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_ctrl_win)->main_widget), FALSE);
	nfui_regi_post_event_callback(g_ctrl_win, ctrl_win_post_event_handler);
	nfui_nfobject_modify_bg(g_ctrl_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));

	/* fixed */
	ctrl_fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(ctrl_fixed1, CTRLBAR_SIZE_W, CTRLBAR_SIZE_H);
	nfui_regi_post_event_callback(ctrl_fixed1, ctrl_fixed_post_event_handler);
	nfui_nfobject_show(ctrl_fixed1);

	/* start button */
	pos_x += 8;
	nfui_get_image_size(IMG_BTN_N_START, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_text(NF_BUTTON(nfwidget), "MENU");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), start_img);
	nfui_nfbutton_set_pango_font(NF_BUTTON(nfwidget), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);	
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _pb_start_button_event_handler);
	focus = nfwidget;

	/* div mode button */
	pos_x += (size_w + 28);
	nfui_get_image_size(IMG_N_DIV_FULL, &size_w, &size_h);

	for (i = 0; i < 7; i++) 
	{
#if defined(GUI_4CH_SUPPORT)
		if ((i == 3) || (i == 4) || (i == 5)) continue;	
#elif defined(GUI_8CH_SUPPORT)
		if (i == 4 || i == 5) continue;
#elif defined(GUI_16CH_SUPPORT)
		if (i == 5) continue;		
#endif
#if defined(_IPX_MODEL_UX)
		if (i == 6) continue;
#endif

		nfwidget = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div_img[i]);
		nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(nfwidget);
		nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

		if (i == 0)			nfui_regi_post_event_callback(nfwidget, _full_button_event_handler);
		else if (i == 1) 	nfui_regi_post_event_callback(nfwidget, pb_div1_button_event_handler);
		else if (i == 2) 	nfui_regi_post_event_callback(nfwidget, pb_div4_button_event_handler);
		else if (i == 3)	nfui_regi_post_event_callback(nfwidget, pb_div9_button_event_handler);
		else if (i == 4)	nfui_regi_post_event_callback(nfwidget, pb_div16_button_event_handler);	
		else if (i == 5)	nfui_regi_post_event_callback(nfwidget, pb_div36_button_event_handler);	

		pos_x += (size_w + 1);
	}

	/* osd on-off button */
	nfui_get_image_size(IMG_N_OSD_OFF, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), osd_img[0]);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
	nfui_regi_post_event_callback(nfwidget, pb_osd_button_event_handler);
	g_osd_obj = nfwidget;

	pos_x = TIMELABEL_SIZE_X;

	// active ctrlbar time
	bgImg = nfui_get_image_from_file(IMG_STATUSBAR_BG, NULL);

#if 1
	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
	gdk_pixbuf_copy_area(bgImg, pos_x, 20, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H, pbBG, 0, 0);

	g_date_label = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_pango_font((NFIMAGE*)g_date_label, nffont_get_pango_font(NFFONT_XLARGE_SEMI), COLOR_IDX(505));
	nfui_nfimage_set_font_alignment((NFIMAGE*)g_date_label, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(g_date_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_date_label);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_date_label, (guint)pos_x, (guint)20);
#else
	g_date_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_XLARGE_SEMI), COLOR_IDX(505));
	nfui_nflabel_use_pango_cashing((NFLABEL*)g_date_label, 0, NULL);
	nfui_nflabel_set_align((NFLABEL*)g_date_label, NFALIGN_CENTER, 0);
	nfui_nfobject_use_tooltip(g_date_label, FALSE);
	nfui_nflabel_use_strip((NFLABEL*)g_date_label, FALSE);
	nfui_nfobject_modify_bg(g_date_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
	nfui_nfobject_use_focus(g_date_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(g_date_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
	nfui_nfobject_show(g_date_label);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_date_label, (guint)pos_x, (guint)20);
#endif


#if 1
	pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
	gdk_pixbuf_copy_area(bgImg, pos_x, (20 + TIMELABEL_SIZE_H), TIMELABEL_SIZE_W, TIMELABEL_SIZE_H, pbBG, 0, 0);

	g_time_label = nfui_nfimage_new_pixbuf(pbBG);
	nfui_nfimage_set_pango_font((NFIMAGE*)g_time_label, nffont_get_pango_font(NFFONT_XLARGE_SEMI), COLOR_IDX(505));
	nfui_nfimage_set_font_alignment((NFIMAGE*)g_time_label, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(g_time_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_time_label);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_time_label, (guint)pos_x, (guint)(20 + TIMELABEL_SIZE_H));
#else
	g_time_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_XLARGE_SEMI), COLOR_IDX(505));
	nfui_nflabel_set_spacing((NFLABEL*)g_time_label, SEMI_CONDENSED_SPACING);
	nfui_nflabel_use_pango_cashing((NFLABEL*)g_time_label, 0, NULL);
	nfui_nflabel_set_align((NFLABEL*)g_time_label, NFALIGN_CENTER, 0);
	nfui_nfobject_use_tooltip(g_time_label, FALSE);
	nfui_nfobject_modify_bg(g_time_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
	nfui_nfobject_use_focus(g_time_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(g_time_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
	nfui_nfobject_show(g_time_label);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_time_label, (guint)pos_x, (guint)(20 + TIMELABEL_SIZE_H));
#endif

// <--------------- play attr button
	pos_x += TIMELABEL_SIZE_W;
	nfui_get_image_size(IMG_N_PLAY_ATTR_STEP_BACK, &size_w, &size_h);

	for(i=0; i<7; i++) {
		nfwidget = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(nfwidget), play_attr_img[i]);
		nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(nfwidget);
		nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

		if (i == 0)
		{
			nfui_regi_post_event_callback(nfwidget, pb_play_step_backward_event_handler);
			nfui_nfobject_set_data(g_ctrl_win, "step backward button", nfwidget);
		}
		else if (i == 1)
			nfui_regi_post_event_callback(nfwidget, pb_play_ds_backward_event_handler);
		else if (i == 2)
			nfui_regi_post_event_callback(nfwidget, pb_play_backward_event_handler);
		else if (i == 3)
			nfui_regi_post_event_callback(nfwidget, pb_play_pause_event_handler);
		else if (i == 4)
			nfui_regi_post_event_callback(nfwidget, pb_play_forward_event_handler);
		else if (i == 5)
			nfui_regi_post_event_callback(nfwidget, pb_play_ds_forward_event_handler);
		else if (i == 6)
		{
			nfui_regi_post_event_callback(nfwidget, pb_play_step_forward_event_handler);
			nfui_nfobject_set_data(g_ctrl_win, "step forward button", nfwidget);
		}
			
		pos_x += (size_w + 1);
	}

// <--------------- play status button
	pos_x += 10;
	nfui_get_image_size(IMG_PLAY_STATUS_FWD_001, &size_w, &size_h);
	 
	g_status = (NFOBJECT*)nfui_nfimage_new(IMG_PLAY_STATUS_FWD_001);
	nfui_nfobject_show(g_status);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_status, pos_x, 19);
	
// <--------------- zoom  button
	pos_x += (size_w + 34);
	nfui_get_image_size(IMG_N_BTN_ZOOM, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), zoom_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
	nfui_regi_post_event_callback(nfwidget, _pb_zoom_event_handler);
	nfui_nfobject_disable(nfwidget);
	nfui_nfobject_set_data(g_ctrl_win, "zoom button", nfwidget);


// <--------------- shapshot button
	pos_x += (size_w + 1);
	nfui_get_image_size(IMG_N_BTN_SNAPSHOT, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), snapshot_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
	nfui_regi_post_event_callback(nfwidget, _pb_snapshot_event_handler);
	g_snap_obj = nfwidget;
#ifndef _SUPPORT_SNAPSHOT
	nfui_nfobject_disable(nfwidget);
#endif
	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(nfwidget);

// <--------------- archive button
	pos_x += (size_w + 1);
	nfui_get_image_size(IMG_N_BTN_BOOKMARK_ON, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), bookmark_img[0]);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
	nfui_regi_post_event_callback(nfwidget, _pb_archive_event_handler);
	g_bookmark_obj = nfwidget;

    if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
        nfui_nfobject_disable(g_bookmark_obj);

	uxm_reg_imsg_event(ctrl_fixed1, INFY_DIV_CHANGE);
	uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_DIV_CHANGE);
	uxm_reg_imsg_event(ctrl_fixed1, INFY_PB_IMAGE_STATUS);
	uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_PB_IMAGE_STATUS);

	nfui_nfwindow_add((NFWINDOW*)g_ctrl_win, ctrl_fixed1);
	nfui_run_main_event_handler(g_ctrl_win);
	nfui_nfobject_show(g_ctrl_win);
	nfui_make_key_hierarchy((NFWINDOW*)g_ctrl_win);
	nfui_set_key_focus(focus, TRUE);

	nfui_page_open(PGID_PLAYBACK_CTRLBAR, g_ctrl_win, nfui_get_last_user());
	vw_playback_statusbar_playstatus_start();

	return g_ctrl_win;
}

void vw_playback_statusbar_playstatus_start(void)
{
	if (st_timer_id == 0)
		st_timer_id = g_timeout_add(80, _statusbar_update_playstatus, NULL);
}

void vw_playback_statusbar_playstatus_end(void)
{
	if(st_timer_id != 0) {
		g_source_remove(st_timer_id);
		st_timer_id = 0;
	}	
}

void vw_playback_statusbar_hide(void)
{
	nfui_nfobject_hide(g_ctrl_win);	
	nfui_page_close(PGID_PLAYBACK_CTRLBAR, g_ctrl_win);
}

void vw_playback_statusbar_show(void)
{
	nfui_nfobject_show(g_ctrl_win);	
	nfui_page_open(PGID_PLAYBACK_CTRLBAR, g_ctrl_win, nfui_get_last_user());
}

void vw_playback_statusbar_change_osd_img(gint status)
{
    if (!g_osd_obj) return;

	nfui_nfbutton_set_image(NF_BUTTON(g_osd_obj), osd_img[status]);
	nfui_signal_emit(g_osd_obj, GDK_EXPOSE, FALSE);
}

void vw_playback_statusbar_change_bookmark_img(gint status)
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

void vw_playback_statusbar_change_stepbutton(gint status)
{
	NFOBJECT* b_obj;
	NFOBJECT* f_obj;

	b_obj = (NFOBJECT*)nfui_nfobject_get_data(g_ctrl_win, "step backward button");
	f_obj = (NFOBJECT*)nfui_nfobject_get_data(g_ctrl_win, "step forward button");

	if (status)
	{
		if (nfui_nfobject_is_disabled(b_obj))
		{
			nfui_nfobject_enable(b_obj);
			nfui_nfobject_enable(f_obj);

			nfui_signal_emit(b_obj, GDK_EXPOSE, FALSE);
			nfui_signal_emit(f_obj, GDK_EXPOSE, FALSE);
		}
	}
	else
	{
		if (!nfui_nfobject_is_disabled(b_obj))
		{
			nfui_nfobject_disable(b_obj);
			nfui_nfobject_disable(f_obj);
			
			nfui_signal_emit(b_obj, GDK_EXPOSE, FALSE);
			nfui_signal_emit(f_obj, GDK_EXPOSE, FALSE);
		}
	}	
}

void vw_playback_statusbar_change_snap(gint enable)
{
#ifdef _SUPPORT_SNAPSHOT
	if (enable)	nfui_nfobject_enable(g_snap_obj);
	else		nfui_nfobject_disable(g_snap_obj);

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_snap_obj);

	nfui_signal_emit(g_snap_obj, GDK_EXPOSE, TRUE);
#endif		
}


