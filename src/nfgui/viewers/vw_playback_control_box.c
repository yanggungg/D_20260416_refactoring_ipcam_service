
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

static GdkPixbuf *div_img[4][NFOBJECT_STATE_COUNT];
static GdkPixbuf *osd_img[2][NFOBJECT_STATE_COUNT];
static GdkPixbuf *bookmark_img[2][NFOBJECT_STATE_COUNT];

static NFOBJECT *ctl_box_win = NULL;
static NFOBJECT *ctrl_fixed1;

static NFOBJECT *wait_pop = NULL;

static guint divbutton_pos_x = 0;
static guint funcbutton_pos_x = 0;

static guint ctl_box_mv_x = 0;
static guint ctl_box_mv_y = 0;



////////////////////////////////////////////////////////////
//
// PLAYBACK CONTROL BOX - MAIN BOX 
//

static void _process_exit()
{
	vsm_change_normal_mode();
	vw_playback_control_box_Hide();	
	vw_playback_statusbar_show();
	VW_Timeline_Show();		
}

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

static gboolean _div_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;
	NFOBJECT *div_box_win = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
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

static gboolean _func_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;
	NFOBJECT *func_box_win = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_window_pos(ctl_box_win, &mv_x, &mv_y);

		x =  mv_x + CTL_DISP_BOX_SIZE_W + 44 + BOX_X_POSITION_CONTROL;
		y =  mv_y - CTL_DISP_BOX_SIZE_H - 1;

		ctl_box_mv_x = mv_x;
		ctl_box_mv_y = mv_y;

		funcbutton_pos_x = mv_x + CTL_MAIN_FUNC_BTN_POS_X;
		
		func_box_win = vw_playback_get_func_box();
		
		nfui_nfobject_move(func_box_win, x, y);		

		if (!nfui_nfobject_is_shown(func_box_win))
		{
			nfui_nfobject_show(func_box_win);
			nfui_page_open(PGID_PLAYBACK_CONTROL_FUNC_BOX, func_box_win, nfui_get_last_user());
		}
	}	
	return FALSE;
}

static gboolean _exit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_process_exit();
	}	
	return FALSE;
}


static gboolean _ctl_box_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_EXPOSE:
			break;	
		case GDK_DELETE:
			nfui_page_close(PGID_PLAYBACK_CONTROL_BOX, obj);
		break;
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == RMC_RESERVE)
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
					wait_pop = nftool_mbox_wait(ctl_box_win, "WAIT", "Please wait...");
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
				_process_exit();
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

static gboolean _ctl_box_fixed1_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	gboolean ret;
	
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
			
			_change_divbutton(div);
		}
		break;

		case INFY_CAPTURE_IMAGE:
		{
			gint result = ((CMM_MESSAGE_T *)data)->param;

			uxm_unreg_imsg_event(ctl_box_win, INFY_CAPTURE_IMAGE);

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

				VW_Snapshot_Open(ctl_box_win, &info, SS_MODE_BURN_RESERVE);
			}
			else
				nftool_mbox(ctl_box_win, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);

			vsm_playback_play_recover_by_menu_closed();				
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

			b_obj = (NFOBJECT*)nfui_nfobject_get_data(ctl_box_win, "step backward button");
			f_obj = (NFOBJECT*)nfui_nfobject_get_data(ctl_box_win, "step forward button");

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
		
		case GDK_BUTTON_RELEASE:
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

NFOBJECT* vw_playback_control_box_open(NFWINDOW *parent)
{
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *nfwidget;

	GdkPixbuf *attr_img[7][NFOBJECT_STATE_COUNT];
	GdkPixbuf *osdoff_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *osdon_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *func_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoom_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *snap_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *arch_img[NFOBJECT_STATE_COUNT];
	
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

	osdoff_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_OFF, NULL);
	osdoff_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_OFF, NULL);
	osdoff_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_OFF, NULL);
	osdoff_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_OFF, NULL);
			
	osdon_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_ON, NULL);
	osdon_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_ON, NULL);
	osdon_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_ON, NULL);
	osdon_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_ON, NULL);

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

	func_img[0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_FUNC, NULL);
	func_img[1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_FUNC, NULL);
	func_img[2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_FUNC, NULL);
	func_img[3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_FUNC, NULL);
			


	ctl_box_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTL_MAIN_BOX_SIZE_X, (guint)CTL_MAIN_BOX_SIZE_Y, (guint)CTL_MAIN_BOX_SIZE_W, (guint)CTL_MAIN_BOX_SIZE_H);
	nfui_nfwindow_set_title(ctl_box_win, "PLAYBACK CONTROL BOX");
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)ctl_box_win)->main_widget), FALSE);
	nfui_nfwindow_set_moving_area_size((NFWINDOW*)ctl_box_win, CTL_MAIN_BOX_SIZE_H);
	nfui_nfwindow_set_moving_limit((NFWINDOW*)ctl_box_win, TRUE);
	nfui_nfobject_modify_bg(ctl_box_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfwindow_set_mask((NFWINDOW*)ctl_box_win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(ctl_box_win, _ctl_box_win_event_handler);


	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_pre_event_callback(fixed1, _ctl_box_fixed1_event_handler);
	nfui_nfobject_set_size(fixed1, CTL_MAIN_BOX_SIZE_W, CTL_MAIN_BOX_SIZE_H);
	nfui_nfobject_show(fixed1);
	ctrl_fixed1 = fixed1;

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
			nfui_regi_post_event_callback(nfwidget, pb_play_step_backward_event_handler);
			nfui_nfobject_set_data(ctl_box_win, "step backward button", nfwidget);
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
			nfui_nfobject_set_data(ctl_box_win, "step forward button", nfwidget);				
		}

		pos_x += (size_w + 1);	
	}


// make func button 
	pos_x += 5;	
	
	nfui_get_image_size(IMG_N_PLAY_FULL_FUNC, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), func_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _func_button_event_handler);

// make exit button 
	pos_x += (size_w + 21);	
	
	nfwidget = nftool_normal_button_create_type1("EXIT", 72);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 11);
	nfui_regi_post_event_callback(nfwidget, _exit_button_event_handler);

	uxm_reg_imsg_event(fixed1, INFY_DIV_CHANGE);
	uxm_monitor_on_imsg_event(fixed1, INFY_DIV_CHANGE);
	uxm_reg_imsg_event(fixed1, INFY_PB_IMAGE_STATUS);
	uxm_monitor_on_imsg_event(fixed1, INFY_PB_IMAGE_STATUS);

	nfui_nfwindow_add((NFWINDOW*)ctl_box_win, fixed1);
	nfui_run_main_event_handler(ctl_box_win);

	nfui_nfobject_show(ctl_box_win);
	nfui_make_key_hierarchy((NFWINDOW*)ctl_box_win);
	nfui_set_key_focus(nfwidget, TRUE);	
	nfui_nfobject_hide(ctl_box_win);

	return ctl_box_win;

}

void vw_playback_control_box_Show()
{
	nfui_nfobject_show(ctl_box_win);
	nfui_page_open(PGID_PLAYBACK_CONTROL_BOX, ctl_box_win, nfui_get_last_user());	
}

void vw_playback_control_box_Hide()
{
	NFOBJECT *func_box_win = NULL;
	NFOBJECT *div_box_win = NULL;

	if(nfui_nfobject_is_shown(ctl_box_win))
	{
		nfui_nfobject_hide(ctl_box_win);
		nfui_page_close(PGID_PLAYBACK_CONTROL_BOX, ctl_box_win);
	}

	div_box_win = vw_playback_get_div_box();
	if(nfui_nfobject_is_shown(div_box_win))
	{
		nfui_nfobject_hide(div_box_win);
		nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, div_box_win);
	}

	func_box_win = vw_playback_get_func_box();
	if(nfui_nfobject_is_shown(func_box_win))
	{
		nfui_nfobject_hide(func_box_win);
		nfui_page_close(PGID_PLAYBACK_CONTROL_FUNC_BOX, func_box_win);
	}
}

gboolean vw_playback_control_box_IsShown()
{
	return nfui_nfobject_is_shown(ctl_box_win);
}

void vw_playback_control_box_change_stepbutton(gint status)
{
	NFOBJECT* b_obj;
	NFOBJECT* f_obj;

	b_obj = (NFOBJECT*)nfui_nfobject_get_data(ctl_box_win, "step backward button");
	f_obj = (NFOBJECT*)nfui_nfobject_get_data(ctl_box_win, "step forward button");

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

void vw_playback_control_box_exit()
{
	_process_exit();
}
