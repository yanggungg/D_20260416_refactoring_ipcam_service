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



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_menu_win;

static GdkPixbuf *g_menuBG[2][NFOBJECT_STATE_COUNT];

static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};

static gint g_rv = -1;



static void make_menu_bg_images()
{
	GdkPixbuf *pbArrow[NFOBJECT_STATE_COUNT];

	// menu button bg
	g_menuBG[0][0] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_N_BUTTON, MENUBUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[0][1] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_O_BUTTON, MENUBUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[0][2] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_P_BUTTON, MENUBUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[0][3] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_D_BUTTON, MENUBUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	// arrow button bg
	g_menuBG[1][0] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_N_BUTTON_ARROW, MENUBUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[1][1] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_O_BUTTON_ARROW, MENUBUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[1][2] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_P_BUTTON_ARROW, MENUBUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[1][3] = nf_ui_create_image_button_no_alpha(MK_IMG_MOTION_SENSOR_D_BUTTON_ARROW, MENUBUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	// arrow 
	pbArrow[0] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_N), NULL);
	pbArrow[1] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_O), NULL);
	pbArrow[2] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_P), NULL);
	pbArrow[3] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_D), NULL);

	gdk_pixbuf_composite(pbArrow[0], g_menuBG[1][0], (MENUBUTTON_SIZE_W - 21), 13, 7, 13, (MENUBUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[1], g_menuBG[1][1], (MENUBUTTON_SIZE_W - 21), 13, 7, 13, (MENUBUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[2], g_menuBG[1][2], (MENUBUTTON_SIZE_W - 21), 13, 7, 13, (MENUBUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbArrow[3], g_menuBG[1][3], (MENUBUTTON_SIZE_W - 21), 13, 7, 13, (MENUBUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);

}


static gboolean 
post_ch_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			channel_submenu_show();
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_RIGHT) 
					channel_submenu_show();
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean 
post_select_all_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		VW_select_menu_item(SELECT_ALL);
		VW_MotionSensorMenu_Destroy();
	}

	return FALSE;
}

static gboolean 
post_deselect_all_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		VW_select_menu_item(DESELECT_ALL);
		VW_MotionSensorMenu_Destroy();
	}
	
	return FALSE;
}

static gboolean 
post_sennblock_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
		
		VW_select_menu_item(OPEN_CONF);
		VW_MotionSensorMenu_Destroy();
	}
	return FALSE;
}

static gboolean 
post_save_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		VW_select_menu_item(SAVE);
		VW_MotionSensorMenu_Destroy();
	}
	
	return FALSE;
}

static gboolean 
post_savenexit_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		VW_select_menu_item(SAVE_N_EXIT);
		VW_MotionSensorMenu_Destroy();
	}
	return FALSE;
}

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		VW_select_menu_item(CANCEL);
		VW_MotionSensorMenu_Destroy();
	}

	return FALSE;
}

static gboolean post_menu_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;		
	gint px = 0, py = 0;

	if(evt->type == GDK_EXPOSE) {	
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(345));

		px = MENUBUTTON_POS_X + MENUBUTTON_MARGIN;
		py = MENUBUTTON_POS_Y + ((MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2));
		gdk_draw_rectangle(drawable,
				gc, TRUE,
				px, py,
				392, 1);

		py = MENUBUTTON_POS_Y + (3 * (MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2));
		py += 3;
		gdk_draw_rectangle(drawable,
				gc, TRUE,
				px, py,
				392, 1);

		py = MENUBUTTON_POS_Y + (4 * (MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2));
		py += 6;
		gdk_draw_rectangle(drawable,
				gc, TRUE,
				px, py,
				392, 1);

		py = MENUBUTTON_POS_Y + (6 * (MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2));
		py += 9;
		gdk_draw_rectangle(drawable,
				gc, TRUE,
				px, py,
				392, 1);
	
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean 
post_menu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS) {
		VW_MotionSensorMenu_Destroy();
	}else if(evt->type == NFEVENT_KEYPAD_RELEASE 
			|| evt->type == NFEVENT_REMOCON_RELEASE) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT) {
			VW_MotionSensorMenu_Destroy();
			return TRUE;
		}

	}else if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_MotionSensorMenu_Open(NFWINDOW *parent, int x, int y)
{
	NFOBJECT *menu_fixed;
	NFOBJECT *obj;
	NFOBJECT *chMenu;

	gchar *strMenu[] = {"CHANNEL", 
						"SELECT ALL", 
						"DESELECT ALL",
#if defined(_DVR_MODEL_UX)						
						"SENSITIVITY",
#else
						"SENSITIVITY & MINIMUM BLOCKS",
#endif						
						"SAVE",
						"SAVE & EXIT",
						"CANCEL" };
	guint pos_y = 0;
	gint i;
	int sx, sy;

	
	g_rv = -1;
		
	// make bg image
	make_menu_bg_images();


	if (x > DISPLAY_ACTIVE_WIDTH - MENU_SIZE_W) x = DISPLAY_ACTIVE_WIDTH - MENU_SIZE_W;
	if (y > DISPLAY_ACTIVE_HEIGHT - MENU_SIZE_H) y = DISPLAY_ACTIVE_HEIGHT - MENU_SIZE_H;
	/* window */
	g_menu_win = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, MENU_SIZE_W, MENU_SIZE_H);
	//g_menu_win = (NFOBJECT*)nfui_nfwindow_new(parent, MENU_POS_X, MENU_POS_Y, MENU_SIZE_W, MENU_SIZE_H);
	g_curwnd = g_menu_win;
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_menu_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_menu_win, GDK_BUTTON_PRESS, TRUE);
	gtk_widget_add_events(((NFWINDOW*)g_menu_win)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(g_menu_win, post_menu_window_event_cb);

	/* fixed */
	menu_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(menu_fixed, MENU_SIZE_W, MENU_SIZE_H);
	nfui_regi_post_event_callback(menu_fixed, post_menu_fixed_event_cb);
	nfui_nfobject_show(menu_fixed);

	pos_y = MENUBUTTON_POS_Y;
	
	for( i = 0 ; i < MENU_COUNTS ; i++) 
	{
		if(i > 0)
			obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[0], strMenu[i]);
		else
			obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[1], strMenu[i]);
		
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENUBUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, MENUBUTTON_SIZE_W, MENUBUTTON_SIZE_H);
		nfui_nfobject_show(obj);

		if(i == 0)		nfui_regi_post_event_callback(obj, post_ch_button_event_cb);
		else if(i == 1)	nfui_regi_post_event_callback(obj, post_select_all_button_event_cb);
		else if(i == 2)	nfui_regi_post_event_callback(obj, post_deselect_all_button_event_cb);
		else if(i == 3)	nfui_regi_post_event_callback(obj, post_sennblock_button_event_cb);
		else if(i == 4)	nfui_regi_post_event_callback(obj, post_save_button_event_cb);
		else if(i == 5)	nfui_regi_post_event_callback(obj, post_savenexit_button_event_cb);
		else if(i == 6)	nfui_regi_post_event_callback(obj, post_cancel_button_event_cb);

		if(i != 0) 
			pos_y += (MENUBUTTON_SIZE_H + MENUBUTTON_GAP_2);

		if(i == 1 || i == 3 || i == 4 || i == 6)
			pos_y += 3;

		nfui_nffixed_put((NFFIXED*)menu_fixed, obj, MENUBUTTON_POS_X, pos_y);

		if(i == 0) chMenu = obj;
	}

	
	nfui_nfwindow_add((NFWINDOW*)g_menu_win, menu_fixed);
	nfui_run_main_event_handler(g_menu_win);
	nfui_nfobject_show(g_menu_win);
	nfui_make_key_hierarchy((NFWINDOW*)g_menu_win);

	sx = x + MENU_SIZE_W + 4;
	sy = y;

	create_channel_submenu(g_curwnd, chMenu, sx, sy, MENU_SIZE_W);

	nfui_page_open(PGID_MOTION_AREA_MENU, g_menu_win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_MOTION_AREA_MENU, g_menu_win);

	return g_rv;
}

int VW_select_menu_item(int item)
{
	g_rv = item;
	return 0;
}

void VW_MotionSensorMenu_Destroy()
{
	nfui_nfobject_hide(g_menu_win);
	channel_submenu_hide();

	channel_submenu_destroy();
	nfui_nfobject_destroy(g_menu_win);
}

int VW_get_motion_sensor_menu_pos(int *x, int *y)
{
	*x = g_menu_win->x;
	*y = g_menu_win->y;
	return 0;
}

int VW_get_motion_sensor_menu_size(int *w, int *h)
{
	*w = g_menu_win->width;
	*h = g_menu_win->height;
	return 0;
}
