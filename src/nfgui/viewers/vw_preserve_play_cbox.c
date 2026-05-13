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

#include "vw_playback_main.h"
#include "vw_playback_internal.h"
#include "vw_playback_control_box.h"
#include "vw_timeline.h"
#include "vw_set_date_time.h"

#include "uxm.h"


#if defined(GUI_4CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			((50+1)*2)
#elif defined(GUI_8CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			(50+1)
#else
#define BOX_X_POSITION_CONTROL			(0)
#endif

#if defined(_SUPPORT_PRESERVE_SET_TIME)
#define CTL_TIME_SIZE_W					(50+1)
#else
#define CTL_TIME_SIZE_W					(0)
#endif

#define CTL_MAIN_BOX_SIZE_W				(539+CTL_TIME_SIZE_W)
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

static GdkPixbuf *div_img[4][NFOBJECT_STATE_COUNT];

static NFOBJECT *ctl_box_win = NULL;
static NFWINDOW *g_curwnd = 0;

static guint divbutton_pos_x = 0;
static guint ctl_box_mv_x = 0;
static guint ctl_box_mv_y = 0;
static gint	osd_status = 0;
static NFOBJECT* g_osd_obj;
static NFOBJECT* g_bwd_step_obj;
static NFOBJECT* g_fwd_step_obj;
static NFOBJECT 	*wait_pop = NULL;

static time_t g_start_time = 0;
static time_t g_end_time = 0;


static void _change_divbutton(gint div)
{
	NFOBJECT* obj;

	obj = (NFOBJECT*)nfui_nfobject_get_data(ctl_box_win, "div button");

	if (div == VSM_DIV16)
	{
		nfui_nfbutton_set_image(NF_BUTTON(obj), div_img[3]);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);	
	}
	else if (div == VSM_DIV9)
	{
		nfui_nfbutton_set_image(NF_BUTTON(obj), div_img[2]);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);	
	}
	else if (div == VSM_DIV4)
	{
		nfui_nfbutton_set_image(NF_BUTTON(obj), div_img[1]);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);			
	}
	else if (div == VSM_DIV1)
	{
		nfui_nfbutton_set_image(NF_BUTTON(obj), div_img[0]);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);			
	}
}

static gboolean _check_pause_playstatus()
{
	DIR_RATE_E play_dir_rate;
	
	play_dir_rate = vsm_playback_get_dir_rate();

	if ((play_dir_rate == DR_PAUSE) || (play_dir_rate == DR_BWD_NEXT_FRAME)
		|| (play_dir_rate == DR_FWD_NEXT_FRAME))
		return TRUE;
		
	return FALSE;	
}

static gboolean _check_stop_playstatus()
{
	DIR_RATE_E play_dir_rate;
	
	play_dir_rate = vsm_playback_get_dir_rate();

	if (play_dir_rate == DR_STOP)
		return TRUE;
		
	return FALSE;	
}

static void _change_stepbutton(gint status)
{
#if defined(STEP_BUTTON_BLOCK)
	return;
#endif

	if (status)
	{
		if (nfui_nfobject_is_disabled(g_bwd_step_obj))
		{
			nfui_nfobject_enable(g_bwd_step_obj);

			if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
				nfui_nfobject_disable(g_bwd_step_obj);

			nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
		}

		if (nfui_nfobject_is_disabled(g_fwd_step_obj))
		{
			nfui_nfobject_enable(g_fwd_step_obj);
			nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);
		}
	}
	else
	{
		if (!nfui_nfobject_is_disabled(g_bwd_step_obj))
		{
			nfui_nfobject_disable(g_bwd_step_obj);
			nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
		}

		if (!nfui_nfobject_is_disabled(g_fwd_step_obj))
		{
			nfui_nfobject_disable(g_fwd_step_obj);			
			nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);
		}
	}	
}



static gboolean _exit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		// temporarily
		// due to yesing (JM39X)

		vw_preserve_play_out();
	}	
	return FALSE;
}

static gboolean _ctl_box_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_EXPOSE:
			break;	
		case GDK_DELETE:
			nfui_page_close(PGID_ARCH_PLAY_CBOX, obj);
		break;
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
			{
				if ((kpid == KEYPAD_REW) || (kpid == KEYPAD_BACKWARD)) return FALSE;
			}
			
			vw_playback_process_common_KeyEvent(kpid);
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
				vw_preserve_play_out();
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

			vsm_jog_event(code);
		}		
		break;
		case NFEVENT_SHUTTLE_CHANGE:
		{
			GdkEventKey *kevt;
			guint code;

			kevt = (GdkEventKey*)event;
			code = kevt->keyval;

			vsm_shuttle_event(code);
		}		
		break;		
		default:
		break;
	}

	return FALSE;

}

static gboolean _ctl_box_fixed1_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	
	switch(event->type) {
		case GDK_EXPOSE:
		{
			gchar name[32];
			GdkGC *gc;
			GdkDrawable *drawable;
			
			g_sprintf(name, "MKB_IMG_PB_CB_BG_01_%d", CTL_MAIN_BOX_SIZE_W);			
			
			nf_ui_create_image_button_method(name, CTL_MAIN_BOX_SIZE_W, IMG_PLAY_FULL_BG_01_01, IMG_PLAY_FULL_BG_01_02, IMG_PLAY_FULL_BG_01_03);

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			nfutil_draw_image(drawable, gc, name, obj->x, obj->y, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);			
		}
		break;

		case INFY_DIV_CHANGE:
		{
			CMM_MESSAGE_T *pmsg;
			gint div;
			
			pmsg = (CMM_MESSAGE_T *)data;			
			div = GPOINTER_TO_INT(pmsg->data);
			
			_change_divbutton(div);
		}
		break;

		case INFY_PB_IMAGE_STATUS:
		{
			CMM_MESSAGE_T *pmsg;
			gint status;

			pmsg = (CMM_MESSAGE_T *)data;			
			status = GPOINTER_TO_INT(pmsg->data);

			if (status == 1)		//stop
			{
				nfui_nfobject_disable(g_bwd_step_obj);
				nfui_nfobject_disable(g_fwd_step_obj);

				nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
				nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);		
			}		
			else					//run
			{
				nfui_nfobject_enable(g_bwd_step_obj);
				nfui_nfobject_enable(g_fwd_step_obj);

				if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
					nfui_nfobject_disable(g_bwd_step_obj);

				nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
				nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);
			}			
		}
		break;

		case GDK_BUTTON_RELEASE:
		break;					
		
		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, INFY_PB_IMAGE_STATUS);		
			uxm_unreg_imsg_event(obj, INFY_DIV_CHANGE);					
			g_curwnd = 0;
		}
		break;
		
		default:
		break;
	}

	return FALSE;

}

static void _remove_waitbox(void)
{
	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}
}

static gboolean _delayed_div(void *data)
{
	if (vsm_change_sfc_arch_play() == -1) _remove_waitbox();
	return FALSE;
}

static void _run_delayed_div(void)
{
	wait_pop = nftool_mbox_wait(ctl_box_win, "WAIT", "Please wait...");
	g_timeout_add(10, _delayed_div, 0);
}

static gboolean _div_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint mv_x, mv_y, x, y;
		NFOBJECT *div_box_win = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_window_pos(ctl_box_win, &mv_x, &mv_y);

		x =  mv_x;
		y =  mv_y - CTL_DISP_BOX_SIZE_H - 1;

		ctl_box_mv_x = mv_x;
		ctl_box_mv_y = mv_y;
		
		divbutton_pos_x = mv_x + CTL_MAIN_DISP_BTN_POS_X;
		
		div_box_win = vw_playback_get_div_box();
		
		nfui_nfobject_move(div_box_win, x, y);

		if (!nfui_nfobject_is_shown(div_box_win))
		{
			nfui_nfobject_show(div_box_win);
			nfui_page_open(PGID_PLAYBACK_CONTROL_DISP_BOX, div_box_win, nfui_get_last_user());
		}
	}	

	return FALSE;
}

static gboolean _step_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static guint tid = 0;

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vsm_playback_step_backward();
		
		if (_check_stop_playstatus())
			vsm_playback_change_dir_rate(DIR_FWD);
	}
	
	return FALSE;
}

static gboolean _ds_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_playback_change_dir_rate(DIR_DS_BWD);
	}	
	
	return FALSE;
}

static gboolean _backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		_change_stepbutton(1);
		vsm_playback_change_dir_rate(DIR_BWD);
	}	
	return FALSE;
}

static gboolean _pause_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (_check_pause_playstatus())
			_change_stepbutton(1);
		else
			_change_stepbutton(0);

		vsm_playback_change_dir_rate(DIR_PAUSE);
	}	
	return FALSE;
}

static gboolean _forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_change_stepbutton(1);
		vsm_playback_change_dir_rate(DIR_FWD);
	}	
	return FALSE;
}

static gboolean _ds_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_playback_change_dir_rate(DIR_DS_FWD);
	}	
	return FALSE;
}

static gboolean _step_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static guint tid = 0;

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vsm_playback_step_forward();

		if (_check_stop_playstatus())
			vsm_playback_change_dir_rate(DIR_FWD);
	}
	
	return FALSE;
}

static gboolean _time_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;
	NFOBJECT *func_box_win = NULL;

	GTimeVal pre_time;
	GTimeVal post_time;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		memset(&pre_time, 0x00, sizeof(GTimeVal));
		memset(&post_time, 0x00, sizeof(GTimeVal));

		nfui_nfobject_get_window_pos(ctl_box_win, &mv_x, &mv_y);

		x =  mv_x + 40;
		y =  mv_y - 330;

		vsm_playback_play_pause_by_menu_opened();
	
		pre_time = vsm_playback_get_playtime();		
		post_time.tv_sec = VW_Set_DateTime_Open(g_curwnd, "DATE/TIME", x, y, pre_time.tv_sec, SDT_TYPE_SEC, g_start_time, g_end_time);

		if (post_time.tv_sec != 0) vsm_playback_play_time_by_menu_closed(post_time);
		else vsm_playback_play_recover_by_menu_closed();
	}	
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}


NFOBJECT* vw_preserve_play_cbox_open(NFWINDOW *parent)
{
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *nfwidget;

	GdkPixbuf *attr_img[7][NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoom_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *snap_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *arch_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *time_img[NFOBJECT_STATE_COUNT];
	
	guint i, pos_x = 0;
	guint size_w, size_h;

/*<---------- load images */
	div_img[0][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV1), NULL);
	div_img[0][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV1), NULL);
	div_img[0][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV1), NULL);
	div_img[0][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV1), NULL);

	div_img[1][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV4), NULL);
	div_img[1][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV4), NULL);
	div_img[1][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV4), NULL);
	div_img[1][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV4), NULL);

	div_img[2][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV9), NULL);
	div_img[2][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV9), NULL);
	div_img[2][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV9), NULL);
	div_img[2][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV9), NULL);

	div_img[3][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV16), NULL);
	div_img[3][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV16), NULL);
	div_img[3][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV16), NULL);
	div_img[3][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV16), NULL);

	attr_img[0][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_STEP_BACK, NULL);
	attr_img[0][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_STEP_BACK, NULL);
	attr_img[0][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_STEP_BACK, NULL);
	attr_img[0][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_STEP_BACK, NULL);
	
	attr_img[1][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_DS_BACK, NULL);
	attr_img[1][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_DS_BACK, NULL);
	attr_img[1][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_DS_BACK, NULL);
	attr_img[1][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_DS_BACK, NULL);
	
	attr_img[2][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_PLAY_BACK, NULL);
	attr_img[2][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_PLAY_BACK, NULL);
	attr_img[2][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_PLAY_BACK, NULL);
	attr_img[2][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_PLAY_BACK, NULL);
	
	attr_img[3][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_PAUSE, NULL);
	attr_img[3][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_PAUSE, NULL);
	attr_img[3][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_PAUSE, NULL);
	attr_img[3][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_PAUSE, NULL);
			
	attr_img[4][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_PLAY_FORWARD, NULL);
	attr_img[4][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_PLAY_FORWARD, NULL);
	attr_img[4][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_PLAY_FORWARD, NULL);
	attr_img[4][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_PLAY_FORWARD, NULL);
	
	attr_img[5][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_DS_FORWARD, NULL);
	attr_img[5][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_DS_FORWARD, NULL);
	attr_img[5][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_DS_FORWARD, NULL);
	attr_img[5][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_DS_FORWARD, NULL);
	
	attr_img[6][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_STEP_FORWARD, NULL);
	attr_img[6][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_STEP_FORWARD, NULL);
	attr_img[6][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_STEP_FORWARD, NULL);
	attr_img[6][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_STEP_FORWARD, NULL);

#if defined(_SUPPORT_PRESERVE_SET_TIME)
	time_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_TIME, NULL);
	time_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_TIME, NULL);
	time_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_TIME, NULL);
	time_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_TIME, NULL);
#endif			

	osd_status = 0;

	ctl_box_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTL_MAIN_BOX_SIZE_X, (guint)CTL_MAIN_BOX_SIZE_Y, (guint)CTL_MAIN_BOX_SIZE_W, (guint)CTL_MAIN_BOX_SIZE_H);
	nfui_nfwindow_set_title(ctl_box_win, "PRESERVE PLAYBACK CONTROL BOX");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)ctl_box_win)->main_widget), FALSE);
	nfui_nfwindow_set_moving_area_size((NFWINDOW*)ctl_box_win, CTL_MAIN_BOX_SIZE_H);
	nfui_nfwindow_set_moving_limit((NFWINDOW*)ctl_box_win, TRUE);
	nfui_nfobject_modify_bg(ctl_box_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfwindow_set_mask((NFWINDOW*)ctl_box_win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(ctl_box_win, _ctl_box_win_event_handler);
	nfui_nfwindow_set_returnkey_proc(ctl_box_win, returnkey_proc);
	g_curwnd = ctl_box_win;

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_pre_event_callback(fixed1, _ctl_box_fixed1_event_handler);
	nfui_nfobject_set_size(fixed1, CTL_MAIN_BOX_SIZE_W, CTL_MAIN_BOX_SIZE_H);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed1);


// make div button 
	pos_x = CTL_MAIN_DISP_BTN_POS_X;
	
	nfui_get_image_size(IMG_N_PLAY_FULL_DIV1, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();

#if defined(GUI_4CH_SUPPORT)
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div_img[1]);
#elif defined(GUI_8CH_SUPPORT)
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div_img[2]);
#else	
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div_img[3]);
#endif	
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _div_button_event_handler);
	nfui_nfobject_set_data(ctl_box_win, "div button", nfwidget);


// make attr button 
	pos_x += (size_w + 6);	
	
	nfui_get_image_size(IMG_N_PLAY_FULL_STEP_BACK, &size_w, &size_h);

	for (i = 0; i < 7; i++)
	{
		nfwidget = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(nfwidget), attr_img[i]);
		nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(nfwidget);
		nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);

		if (i == 0)
		{
			nfui_regi_post_event_callback(nfwidget, _step_backward_event_handler);
			g_bwd_step_obj = nfwidget;
		}
		else if (i == 1)
			nfui_regi_post_event_callback(nfwidget, _ds_backward_event_handler);
		else if (i == 2)
			nfui_regi_post_event_callback(nfwidget, _backward_event_handler);
		else if (i == 3)
			nfui_regi_post_event_callback(nfwidget, _pause_event_handler);
		else if (i == 4)
			nfui_regi_post_event_callback(nfwidget, _forward_event_handler);
		else if (i == 5)
			nfui_regi_post_event_callback(nfwidget, _ds_forward_event_handler);
		else if (i == 6)
		{
			nfui_regi_post_event_callback(nfwidget, _step_forward_event_handler);
			g_fwd_step_obj = nfwidget;
		}

		if ((i == 0) || (i == 1) || (i == 2))
		{
			if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
				nfui_nfobject_disable(nfwidget);
		}

		pos_x += (size_w + 1);	
	}

#if defined(_SUPPORT_PRESERVE_SET_TIME)
// make time button 

	nfui_get_image_size(IMG_N_PLAY_FULL_TIME, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), time_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _time_button_event_handler);

	pos_x += (size_w + 1);	
#endif

// make exit button 
	pos_x += 21;	
	
	nfwidget = nftool_normal_button_create_type1("EXIT", 72);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 11);
	nfui_regi_post_event_callback(nfwidget, _exit_button_event_handler);

	uxm_reg_imsg_event(fixed1, INFY_PB_IMAGE_STATUS);
	uxm_monitor_on_imsg_event(fixed1, INFY_PB_IMAGE_STATUS);

	uxm_reg_imsg_event(fixed1, INFY_DIV_CHANGE);
	uxm_monitor_on_imsg_event(fixed1, INFY_DIV_CHANGE);

	nfui_nfwindow_add((NFWINDOW*)ctl_box_win, fixed1);
	nfui_run_main_event_handler(ctl_box_win);

	nfui_nfobject_show(ctl_box_win);
	nfui_make_key_hierarchy((NFWINDOW*)ctl_box_win);

	nfui_page_open(PGID_ARCH_PLAY_CBOX, ctl_box_win, nfui_get_last_user());
	
	nfui_set_key_focus(nfwidget, TRUE);	

	return ctl_box_win;
}

gint vw_preserve_play_timelimit(time_t start_time, time_t end_time)
{
	g_start_time = start_time;
	g_end_time = end_time;
	return 0;
}


