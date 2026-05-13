
#include "nf_afx.h"
#include "nf_api_disk.h"


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
#include "vw_arch_play_cbox.h"


#define STEP_BUTTON_BLOCK	1
#define FAST_BUTTON_BLOCK	1

#define BUTTON_CNT			(7)

#if defined(STEP_BUTTON_BLOCK)
#define INC_STEP_BTN_W					((50+1)*2)
#else
#define INC_STEP_BTN_W					(0)
#endif

#if defined(FAST_BUTTON_BLOCK)
#define INC_FAST_BTN_W					((50+1)*2)
#else
#define INC_FAST_BTN_W					(0)
#endif

#define CTL_MAIN_BOX_SIZE_W				(589 - INC_STEP_BTN_W - INC_FAST_BTN_W)
#define CTL_MAIN_BOX_SIZE_H				(74)
#define CTL_MAIN_BOX_SIZE_X				(DISPLAY_ACTIVE_WIDTH - CTL_MAIN_BOX_SIZE_W - 5)
#define CTL_MAIN_BOX_SIZE_Y				(DISPLAY_ACTIVE_HEIGHT - CTL_MAIN_BOX_SIZE_H - 13)

#define CTL_MAIN_DISP_BTN_POS_X			(9)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT* g_osd_obj;
static NFOBJECT* g_bwd_step_obj;
static NFOBJECT* g_fwd_step_obj;

static GdkPixbuf *osd_img[2][NFOBJECT_STATE_COUNT];
static gint	osd_status = 0;
static NFOBJECT 	*wait_pop = NULL;




////////////////////////////////////////////////////////////
//
// PLAYBACK CONTROL BOX - MAIN BOX 
//

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
			nfui_nfobject_enable(g_fwd_step_obj);

			nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
			nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);
		}
	}
	else
	{
		if (!nfui_nfobject_is_disabled(g_bwd_step_obj))
		{
			nfui_nfobject_disable(g_bwd_step_obj);
			nfui_nfobject_disable(g_fwd_step_obj);
			
			nfui_signal_emit(g_bwd_step_obj, GDK_EXPOSE, FALSE);
			nfui_signal_emit(g_fwd_step_obj, GDK_EXPOSE, FALSE);
		}
	}	
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
	wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	g_timeout_add(10, _delayed_div, 0);
}

static gboolean _div_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (vsm_has_next_arch_ch()) _run_delayed_div();
	}	
	return FALSE;
}

static gboolean _osd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(osd_status ^= 1)
			vsm_osd_off();
		else
			vsm_osd_on();

		nfui_nfbutton_set_image(NF_BUTTON(g_osd_obj), osd_img[osd_status]);
		nfui_signal_emit(g_osd_obj, GDK_EXPOSE, FALSE);
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


static gboolean _exit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint mv_x, mv_y, x, y;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		// temporarily
		// due to yesing (JM39X)
		nf_set_using_usb(FALSE);

		vw_arch_play_out();
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

			uxm_unreg_imsg_event(obj, INFY_PLAYBACK_STARTED);
			g_curwnd = 0;
		break;

		case INFY_PLAYBACK_STARTED:
			_remove_waitbox();
			break;
		
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_DISP)
			{
				vsm_change_sfc_arch_play();
			}			
#if !defined(STEP_BUTTON_BLOCK)						
			else if (kpid == RMC_BJUMP)
			{
				vsm_playback_step_backward();
				
				if (_check_stop_playstatus())
					vsm_playback_change_dir_rate(DIR_FWD);
			}
#endif			
#if !defined(FAST_BUTTON_BLOCK)			
			else if (kpid == KEYPAD_REW)
			{
				vsm_playback_change_dir_rate(DIR_DS_BWD);
			}
#endif			
			else if (kpid == KEYPAD_BACKWARD)
			{
				_change_stepbutton(1);
				vsm_playback_change_dir_rate(DIR_BWD);
			}
			else if (kpid == KEYPAD_PAUSE)
			{
				if (_check_pause_playstatus())
				{
					_change_stepbutton(1);
					vsm_playback_change_dir_rate(DIR_FWD);					
				}
				else
				{
					_change_stepbutton(0);
					vsm_playback_change_dir_rate(DIR_PAUSE);					
				}
			}
			else if (kpid == KEYPAD_FORWARD)
			{
				_change_stepbutton(1);
				vsm_playback_change_dir_rate(DIR_FWD);
			}
#if !defined(FAST_BUTTON_BLOCK)			
			else if (kpid == KEYPAD_FF)
			{
				vsm_playback_change_dir_rate(DIR_DS_FWD);
			}
#endif			
#if !defined(STEP_BUTTON_BLOCK)			
			else if (kpid == RMC_FJUMP)
			{
				vsm_playback_step_forward();

				if (_check_stop_playstatus())
					vsm_playback_change_dir_rate(DIR_FWD);
			}			
#endif			
		}
		break;		

		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_EXIT)
			{
				// temporarily
				// due to yesing (JM39X)
				nf_set_using_usb(FALSE);

				vw_arch_play_out();
				return TRUE;
			}
		}
		break;
		
#if !defined(STEP_BUTTON_BLOCK)					
		case NFEVENT_JOG_CHANGE:
		{
			GdkEventKey *kevt;
			guint code;
		
			kevt = (GdkEventKey*)event;
			code = kevt->keyval;
		
			vsm_jog_event(code);
		}		
		break;
#endif		
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
		case GDK_BUTTON_RELEASE:
		break;					
		case GDK_DELETE:
		break;
		default:
		break;
	}

	return FALSE;

}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//
//


NFOBJECT* vw_arch_play_cbox_open(NFWINDOW *parent)
{
	NFOBJECT *ctl_box_win = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *nfwidget;

	GdkPixbuf *div1_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *attr_img[7][NFOBJECT_STATE_COUNT];
	
	guint i, pos_x = 0;
	guint size_w, size_h;

/*<---------- load images */
	div1_img[0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV1), NULL);
	div1_img[1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV1), NULL);
	div1_img[2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV1), NULL);
	div1_img[3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV1), NULL);

	osd_img[0][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_OFF, NULL);
			
	osd_img[1][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_ON, NULL);
	
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

	osd_status = 0;

	ctl_box_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTL_MAIN_BOX_SIZE_X, (guint)CTL_MAIN_BOX_SIZE_Y, (guint)CTL_MAIN_BOX_SIZE_W, (guint)CTL_MAIN_BOX_SIZE_H);
	g_curwnd = ctl_box_win;
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)ctl_box_win)->main_widget), FALSE);
	nfui_nfwindow_set_moving_area_size((NFWINDOW*)ctl_box_win, CTL_MAIN_BOX_SIZE_H);
	nfui_nfobject_modify_bg(ctl_box_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfwindow_set_mask((NFWINDOW*)ctl_box_win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(ctl_box_win, _ctl_box_win_event_handler);
	nfui_nfwindow_set_returnkey_proc(ctl_box_win, returnkey_proc);


	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_pre_event_callback(fixed1, _ctl_box_fixed1_event_handler);
	nfui_nfobject_set_size(fixed1, CTL_MAIN_BOX_SIZE_W, CTL_MAIN_BOX_SIZE_H);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed1);



// make div button 
	pos_x = CTL_MAIN_DISP_BTN_POS_X;
	
	nfui_get_image_size(IMG_N_PLAY_FULL_DIV1, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div1_img);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _div_button_event_handler);

	pos_x += (size_w + 10);	

// make osd button 
	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), osd_img[0]);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, _osd_button_event_handler);
	g_osd_obj = nfwidget;

	pos_x += (size_w + 10);	

// make attr button 
	nfui_get_image_size(IMG_N_PLAY_FULL_STEP_BACK, &size_w, &size_h);

	for (i = 0; i < BUTTON_CNT; i++)
	{
#if defined(STEP_BUTTON_BLOCK)
		if ((i == 0) || (i == 6))
			continue;
#endif
	
#if defined(FAST_BUTTON_BLOCK)
		if ((i == 1) || (i == 5))
			continue;
#endif
	
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

		pos_x += (size_w + 1);	
	}

// make exit button 
	pos_x += 10;	
	
	nfwidget = nftool_normal_button_create_type1("EXIT", 72);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, 11);
	nfui_regi_post_event_callback(nfwidget, _exit_button_event_handler);

	nfui_nfwindow_add((NFWINDOW*)ctl_box_win, fixed1);
	nfui_run_main_event_handler(ctl_box_win);
	nfui_nfobject_show(ctl_box_win);
	nfui_make_key_hierarchy((NFWINDOW*)ctl_box_win);
	nfui_page_open(PGID_ARCH_PLAY_CBOX, ctl_box_win, nfui_get_last_user());

	nfui_set_key_focus(nfwidget, TRUE);

	uxm_reg_imsg_event(g_curwnd, INFY_PLAYBACK_STARTED);

	return ctl_box_win;

}

