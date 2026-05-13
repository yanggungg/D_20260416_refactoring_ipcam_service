
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
#include "vw_playback_statusbar.h"
#include "vw_timeline.h"
#include "uxm.h"


#if defined(GUI_4CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			((50+1)*3)
#elif defined(GUI_8CH_SUPPORT)
#define BOX_X_POSITION_CONTROL			((50+1)*2)
#elif defined(GUI_16H_SUPPORT)
#define BOX_X_POSITION_CONTROL			(50+1)
#else
#define BOX_X_POSITION_CONTROL			(0)
#endif

#define CTL_MAIN_BOX_SIZE_W				(589)
#define CTL_MAIN_BOX_SIZE_H				(74)
#define CTL_MAIN_BOX_SIZE_X				(DISPLAY_ACTIVE_WIDTH - CTL_MAIN_BOX_SIZE_W - 5)
#define CTL_MAIN_BOX_SIZE_Y				(DISPLAY_ACTIVE_HEIGHT - CTL_MAIN_BOX_SIZE_H - 13)

#define CTL_DISP_BOX_SIZE_W				(322 - BOX_X_POSITION_CONTROL)
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

static GdkPixbuf *div_img[5][NFOBJECT_STATE_COUNT];
static GdkPixbuf *osd_img[2][NFOBJECT_STATE_COUNT];

static NFOBJECT *div_box_win = NULL;

static NFOBJECT* g_osd_obj;

static guint divbutton_pos_x = 0;

static guint ctl_box_mv_x = 0;
static guint ctl_box_mv_y = 0;





////////////////////////////////////////////////////////////
//
// PLAYBACK CONTROL BOX - DIV BOX 
//

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


static gboolean _div_box_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	GdkEventMotion *m_event;
	guint result;
	m_event = (GdkEventMotion*)event;

	switch(event->type) {
		case NFOUTEVT_MOTION_NOTIFY:
			result = _mouse_notify_detect((guint)m_event->x_root, (guint)m_event->y_root, divbutton_pos_x);
		
			switch(result)
			{
				case CTL_BOX_IN_NOTI:
				case CTL_BOX_OUT_NOTI:
				break;
				case TBUTTON_OUT_NOTI:
					if (nfui_nfobject_is_shown(obj))
					{
						nfui_nfobject_hide(obj);
						nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, obj);
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
				nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, obj);
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
				nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, obj);
				return TRUE;				
			}
		}
		break;			
		case GDK_DELETE:
			nfui_page_close(PGID_PLAYBACK_CONTROL_DISP_BOX, obj);
			break;
		default:
			break;
	}

	return FALSE;

}

static gboolean _ctl_box_fixed2_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	
	switch(event->type) {
		case GDK_EXPOSE:
		{
			gchar name[32];
			GdkGC *gc;
			GdkDrawable *drawable;
			
			g_sprintf(name, "MKB_IMG_PB_CB_BG_02_%d", CTL_DISP_BOX_SIZE_W);			
			
			nf_ui_create_image_button_method(name, CTL_DISP_BOX_SIZE_W, IMG_PLAY_FULL_BG_02_01, IMG_PLAY_FULL_BG_02_02, IMG_PLAY_FULL_BG_02_03);

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


/////////////////////////////////////////////////////////////////////////
//
//
//


NFOBJECT* vw_playback_control_box_div_open(NFWINDOW *parent)
{
	NFOBJECT *fixed2 = NULL;
	NFOBJECT *nfwidget;

	guint i, btn_cnt, pos_x = 0;
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

	div_img[3][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV16), NULL);
	div_img[3][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV16), NULL);
	div_img[3][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV16), NULL);
	div_img[3][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV16), NULL);

	div_img[4][0] = nfui_get_image_from_file((IMG_N_PLAY_FULL_DIV32), NULL);
	div_img[4][1] = nfui_get_image_from_file((IMG_O_PLAY_FULL_DIV32), NULL);
	div_img[4][2] = nfui_get_image_from_file((IMG_P_PLAY_FULL_DIV32), NULL);
	div_img[4][3] = nfui_get_image_from_file((IMG_D_PLAY_FULL_DIV32), NULL);

	osd_img[0][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_OFF, NULL);
	osd_img[0][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_OFF, NULL);
			
	osd_img[1][0] = nfui_get_image_from_file(IMG_N_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][1] = nfui_get_image_from_file(IMG_O_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][2] = nfui_get_image_from_file(IMG_P_PLAY_FULL_OSD_ON, NULL);
	osd_img[1][3] = nfui_get_image_from_file(IMG_D_PLAY_FULL_OSD_ON, NULL);


	div_box_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTL_DISP_BOX_SIZE_X, (guint)CTL_DISP_BOX_SIZE_Y, (guint)CTL_DISP_BOX_SIZE_W, (guint)CTL_DISP_BOX_SIZE_H);
	nfui_nfobject_modify_bg(div_box_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfwindow_use_outside_evt((NFWINDOW*)div_box_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)div_box_win, GDK_MOTION_NOTIFY, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)div_box_win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(div_box_win, _div_box_win_event_handler);


	fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_pre_event_callback(fixed2, _ctl_box_fixed2_event_handler);
	nfui_nfobject_set_size(fixed2, CTL_DISP_BOX_SIZE_W, CTL_DISP_BOX_SIZE_H);
	nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed2);



// make div button 
	pos_x = 9;
	nfui_get_image_size(IMG_N_PLAY_FULL_DIV1, &size_w, &size_h);

#if defined(GUI_4CH_SUPPORT)
	btn_cnt = 2;
#elif defined(GUI_8CH_SUPPORT)
	btn_cnt = 3;
#elif defined(GUI_16CH_SUPPORT)
	btn_cnt = 4;
#else
	btn_cnt = 5;
#endif

	for (i = 0; i < btn_cnt; i++)	
	{
		nfwidget = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(nfwidget), div_img[i]);
		nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(nfwidget);
		nfui_nffixed_put((NFFIXED*)fixed2, nfwidget, pos_x, 8);

		if (i == 0) 	nfui_regi_post_event_callback(nfwidget, pb_div1_button_event_handler);
		else if (i == 1) 	nfui_regi_post_event_callback(nfwidget, pb_div4_button_event_handler);
		else if (i == 2)	nfui_regi_post_event_callback(nfwidget, pb_div9_button_event_handler);
		else if (i == 3)	nfui_regi_post_event_callback(nfwidget, pb_div16_button_event_handler);	
		else if (i == 4)	nfui_regi_post_event_callback(nfwidget, pb_div36_button_event_handler);	

		pos_x += (size_w + 1);
	}

// make osd button 

	nfui_get_image_size(IMG_N_PLAY_FULL_DIV1, &size_w, &size_h);

	nfwidget = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(nfwidget), osd_img[0]);
	nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(nfwidget);
	nfui_nffixed_put((NFFIXED*)fixed2, nfwidget, pos_x, 8);
	nfui_regi_post_event_callback(nfwidget, pb_osd_button_event_handler);
	g_osd_obj = nfwidget;

	nfui_nfwindow_add((NFWINDOW*)div_box_win, fixed2);
	nfui_run_main_event_handler(div_box_win);

	nfui_nfobject_show(div_box_win);
	nfui_make_key_hierarchy((NFWINDOW*)div_box_win);
	nfui_nfobject_hide(div_box_win);
	
	return div_box_win;

}

void vw_playback_control_box_change_osd_img(gint status)
{
	nfui_nfbutton_set_image(NF_BUTTON(g_osd_obj), osd_img[status]);
	nfui_signal_emit(g_osd_obj, GDK_EXPOSE, FALSE);
}


