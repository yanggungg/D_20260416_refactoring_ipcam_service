#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"

#include "tools/nf_ui_function.h"

#include "vw_motion_sensor_menu.h"
#include "vw_motion_sensor_submenu_ch.h"

#define MENU_COUNTS							7
#define MENU_POS_X							((DISPLAY_ACTIVE_WIDTH - MENU_SIZE_W)/2)
#define MENU_POS_Y							((DISPLAY_ACTIVE_HEIGHT - MENU_SIZE_H)/2)
#define MENU_SIZE_W							(430)
#define MENU_SIZE_H							(316)

#define SUBMENU_POS_X						(MENU_POS_X + MENU_SIZE_W + 2)
#define SUBMENU_POS_Y						(MENU_POS_Y)
#define SUBMENU_SIZE_W						(290)
#define SUBMENU_SIZE_H						(42) * GUI_CHANNEL_CNT + 10

#define MENUBUTTON_POS_X					(8)
#define MENUBUTTON_POS_Y					(5)
#define MENUBUTTON_MARGIN					(15) 
#define MENUBUTTON_SIZE_W					(MENU_SIZE_W - MENUBUTTON_POS_X - MENUBUTTON_MARGIN) 
#define MENUBUTTON_SIZE_H					(40) 
#define MENUBUTTON_GAP_2					(2)

#define SUBMENUBUTTON_SIZE_W				(260)
#define SUBMENUBUTTON_SIZE_H				(40)

#define MENU_GAP					2


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_submenu_win;
static NFOBJECT *g_submenu[GUI_CHANNEL_CNT];
static NFOBJECT *g_chMenu;

static GdkPixbuf *g_submenuBG[NFOBJECT_STATE_COUNT];

static guint g_font_color[NFOBJECT_STATE_COUNT] = {340, 341, 343, 344};



/***********************************************************************************************
 *	CHANNEL SUBMENU
 ***********************************************************************************************/

static gboolean post_submenu_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_submenu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int mx, my, mw, mh;

	if(evt->type == NFOUTEVT_MOTION_NOTIFY) {
		GdkEventMotion *mevt;
		gint x, y;
		gint px, py;
		gint pedge;

		mevt = (GdkEventMotion*)evt;

		VW_get_motion_sensor_menu_pos(&mx, &my);
		VW_get_motion_sensor_menu_size(&mw, &mh);

		x = (gint)mevt->x_root - mx;
		y = (gint)mevt->y_root - my;

		px = (gint)mevt->x_root - g_chMenu->x;
		py = (gint)mevt->y_root - g_chMenu->y;
	
		if (px < obj->x) { 	// show on right side	
			if((x < g_chMenu->x || x > (mw + obj->width))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);
					
					nfui_set_key_focus(g_chMenu, FALSE);
					nfui_signal_emit(g_chMenu, GDK_EXPOSE, FALSE);
					return TRUE;
				}
			}

			if((y < g_chMenu->y || y > (g_chMenu->y + g_chMenu->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);

					nfui_set_key_focus(g_chMenu, FALSE);
					nfui_signal_emit(g_chMenu, GDK_EXPOSE, FALSE);
					return TRUE;
				}
			}
		}
		else {
			pedge = obj->x + obj->width + MENU_GAP + mw;

			if((gint)mevt->x_root < obj->x || (gint)mevt->x_root > pedge) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);
					
					nfui_set_key_focus(g_chMenu, FALSE);
					nfui_signal_emit(g_chMenu, GDK_EXPOSE, FALSE);
					return TRUE;
				}
			}

			if((y < g_chMenu->y || y > (g_chMenu->y + g_chMenu->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);

					nfui_set_key_focus(g_chMenu, FALSE);
					nfui_signal_emit(g_chMenu, GDK_EXPOSE, FALSE);
					return TRUE;
				}
			}
		}

		return TRUE;
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_LEFT) {
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);
		}
		else if(kpid == KEYPAD_EXIT) {
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);
			return TRUE;
		}
	}
	else if(evt->type == NFOUTEVT_BUTTON_PRESS) {
		VW_MotionSensorMenu_Destroy();
	}
	else if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		g_submenu_win = NULL;
		nfui_page_close(PGID_MOTION_AREA_SUBMENU, obj);
	}

	return FALSE;
}

static gboolean post_select_submenu_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_BUTTON_PRESS:
		{
			gint i;

			if(evt->button.button == MOUSE_RIGTH_BUTTON) 			 
				return FALSE;

			for(i = 0 ; i < GUI_CHANNEL_CNT ; i++)
			{
				if(g_submenu[i] == obj) {
					VW_select_menu_item(i);
					VW_MotionSensorMenu_Destroy();
					break;
				}
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

void create_channel_submenu(NFWINDOW *parent, NFOBJECT *parent_item, int x, int y, int pw)
{
	NFOBJECT *submenu_fixed;
	NFOBJECT *obj;

	guint pos_y = 0;
	gint i;

	gchar strSubMenu[GUI_CHANNEL_CNT][STRING_SIZE_CAMTITLE+8];
	gchar buf[STRING_SIZE_CAMTITLE];

	g_chMenu = parent_item;

	memset(strSubMenu, 0x00, sizeof(strSubMenu));

	for(i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	{
		//DAL_get_camera_title(buf, i);
		var_get_camtitle(buf, i);

		g_sprintf(strSubMenu[i], "%d %s", i+1, buf);
	}

	// submenu bg
	g_submenuBG[0] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_N_SUBBUTTON, SUBMENUBUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_submenuBG[1] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_O_SUBBUTTON, SUBMENUBUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_submenuBG[2] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_P_SUBBUTTON, SUBMENUBUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_submenuBG[3] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_D_SUBBUTTON, SUBMENUBUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	if (x + SUBMENU_SIZE_W > DISPLAY_ACTIVE_WIDTH) x = x - (pw + SUBMENU_SIZE_W + 8);
	if (y + SUBMENU_SIZE_H > DISPLAY_ACTIVE_HEIGHT) y = (DISPLAY_ACTIVE_HEIGHT - SUBMENU_SIZE_H - 8);

	/* window */
	g_submenu_win = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, SUBMENU_SIZE_W, SUBMENU_SIZE_H);
	g_curwnd = (NFWINDOW*)g_submenu_win;
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_submenu_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_submenu_win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_submenu_win, GDK_MOTION_NOTIFY, TRUE);
	gtk_widget_add_events(((NFWINDOW*)g_submenu_win)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(g_submenu_win, post_submenu_window_event_cb);

	/* fixed */
	submenu_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(submenu_fixed, SUBMENU_SIZE_W, SUBMENU_SIZE_H);
	nfui_regi_post_event_callback(submenu_fixed, post_submenu_fixed_event_cb);
	nfui_nfobject_show(submenu_fixed);


	pos_y = MENUBUTTON_POS_Y;
	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_submenuBG, strSubMenu[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENUBUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, MENUBUTTON_SIZE_W, MENUBUTTON_SIZE_H);
		nfui_nfobject_show(obj);

		nfui_regi_post_event_callback(obj, post_select_submenu_button_event_cb);

		if(i != 0) 
			pos_y += (MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2);

		nfui_nffixed_put((NFFIXED*)submenu_fixed, obj, MENUBUTTON_POS_X, pos_y);

		g_submenu[i] = obj;
	}

	nfui_nfwindow_add((NFWINDOW*)g_submenu_win, submenu_fixed);
	nfui_run_main_event_handler(g_submenu_win);
	nfui_nfobject_show(g_submenu_win);
	nfui_make_key_hierarchy((NFWINDOW*)g_submenu_win);

	nfui_nfobject_hide(g_submenu_win);

}

void channel_submenu_show()
{
	if(g_submenu_win == NULL) return;
	if(!nfui_nfobject_is_shown(g_submenu_win)) {
		nfui_nfobject_show(g_submenu_win);
		nfui_page_open(PGID_MOTION_AREA_SUBMENU, g_submenu_win, nfui_get_last_user());
	}
}

void channel_submenu_hide()
{
	if(g_submenu_win == NULL) return;
	if(nfui_nfobject_is_shown(g_submenu_win)) {
		nfui_nfobject_hide(g_submenu_win);
		nfui_page_close(PGID_MOTION_AREA_SUBMENU, g_submenu_win);
	}
}

void channel_submenu_destroy()
{
	if(g_submenu_win == NULL) return;
	nfui_nfobject_destroy(g_submenu_win);
}

