#include <string.h>

#include "nf_afx.h"
#include "scm.h"

#include "services/uxm.h"

#include "modules/ssm.h"

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
#include "viewers/objects/nflabel.h"

#include "vw_internal.h"
#include "vsm.h"
#include "stm.h"
#include "dtf.h"

#include "vw_timeline.h"
#include "vw_arch_export.h"
#include "vw_live_statusbar.h"
#include "vw_search_main.h"
#include "vw_timeline_popup.h"
#include "vw_timeline_popup_submenu_search.h"


#define VTP_SIZE_W							(360)
#ifndef VER2
#define VTP_SIZE_H							(280 - 40)
#else
#define VTP_SIZE_H							(280)
#endif
#define VTP_XPOS_GAP						(8) // (4)

#define VTP_SUB_SIZE_W						(287)
#define VTP_SUB_SIZE_H						(104)

#define VTP_BUTTON_POS_X					(12)
#define VTP_BUTTON_POS_Y					(5)
#define VTP_BUTTON_MARGIN					(15) 
#define VTP_BUTTON_SIZE_W					(VTP_SIZE_W - VTP_BUTTON_POS_X - VTP_BUTTON_MARGIN) 
#define VTP_BUTTON_SIZE_H					(40) 
#define VTP_BUTTON_GAP_2					(2)

#define VTP_LABEL_SIZE_W					(VTP_BUTTON_SIZE_W)
#define VTP_LABEL_SIZE_H					(VTP_BUTTON_SIZE_H)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *search_sub_menu;
static GdkPixbuf *g_menuBG[2][NFOBJECT_STATE_COUNT];
static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};



////////////////////////////////////////////////////////////////////
//
//
//

static void make_vtp_bg_images()
{
	// menu button bg
	g_menuBG[0][0] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_N_BUTTON, VTP_BUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[0][1] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_O_BUTTON, VTP_BUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[0][2] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_P_BUTTON, VTP_BUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[0][3] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_D_BUTTON, VTP_BUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

}

static gboolean post_event_search_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GTimeVal stime, etime;
    gboolean use_dl = 0;
	SecurityData secdata;

	memset(&stime, 0x00, sizeof(GTimeVal));
	memset(&etime, 0x00, sizeof(GTimeVal));
	switch(evt->type) {
	case GDK_BUTTON_PRESS:
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (vsm_get_vmode() == VMODE_LV)
		{
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

    		get_time_data(&stime, &etime);
    		stm_set_time_t(stime.tv_sec);
    		stm_set_endtime_t(etime.tv_sec);
                
			VW_Live_StatusBar_Hide();
			VW_Timeline_Hide();
			vsm_live_stop();
			VW_Search_Open(NF_TOPWND, 2, vsm_create_livestart_obj());
		}
		else
			vw_playback_out_SearchPage_open(2);
		break;
	case GDK_BUTTON_RELEASE:
		break;

	default:
		break;
	}

	return FALSE;
	
}

static gboolean post_search_sub_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
	case GDK_DELETE:
		g_curwnd = 0;
		break;
	}
	return FALSE;
}

static gboolean post_search_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
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


static gboolean search_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{
				if(!nfui_nfobject_is_shown(search_sub_menu)) {
					VW_Show_Search_SubMenu();
				}
			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_Search_SubMenu(NFWINDOW *parent)
{
	NFOBJECT *vtp_f;
	NFOBJECT *obj;
	
	gchar *searchsub_strBtn[2] = {"EVENT SEARCH", "TEXT-IN DEVICE SEARCH"};
	guint pos_y = 0;
	gint i;

	/* window */
	search_sub_menu = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, VTP_SIZE_W, 100);
	g_curwnd = search_sub_menu;
	nfui_nfwindow_set_title(search_sub_menu, "TIMELINE POPUP - SUBMENU SEARCH");
	nfui_regi_post_event_callback(search_sub_menu, post_search_sub_event_cb);
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)search_sub_menu)->main_widget), FALSE);
//	nfui_nfwindow_use_outside_evt((NFWINDOW*)search_sub_menu, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)search_sub_menu, GDK_BUTTON_PRESS, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)search_sub_menu, GDK_MOTION_NOTIFY, TRUE);
	
//	gtk_widget_add_events(((NFWINDOW*)search_sub_menu)->main_widget, GDK_POINTER_MOTION_HINT_MASK);


	make_vtp_bg_images();

	/* fixed */
	vtp_f = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(vtp_f, VTP_SIZE_W, 100);
	nfui_regi_post_event_callback(vtp_f, post_search_fixed_event_cb);
	nfui_nfobject_show(vtp_f);


	/* */
	pos_y = VTP_BUTTON_POS_Y;

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[0], searchsub_strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, VTP_BUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, VTP_BUTTON_SIZE_W, VTP_BUTTON_SIZE_H);

		if(i != 0) 
			pos_y += (VTP_BUTTON_SIZE_H + VTP_BUTTON_GAP_2);

		if (i == 0)
			nfui_regi_post_event_callback(obj, post_event_search_btn_handler);

		if (i == 1)
			nfui_nfobject_disable(obj);


		nfui_nffixed_put((NFFIXED*)vtp_f, obj, VTP_BUTTON_POS_X, pos_y);
		nfui_nfobject_show(obj);

	}


	nfui_nfwindow_add((NFWINDOW*)search_sub_menu, vtp_f);

	nfui_run_main_event_handler(search_sub_menu);
	nfui_nfobject_hide(search_sub_menu);


	return TRUE;
}

int VW_Destory_Search_SubMenu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

gboolean VW_Hide_Search_SubMenu()
{
	nfui_unregi_semi_modal(search_sub_menu);
	nfui_nfobject_hide(search_sub_menu);

	return TRUE;
}

gboolean VW_Show_Search_SubMenu()
{
	nfui_regi_semi_modal(search_sub_menu);
	nfui_nfobject_show(search_sub_menu);

	return TRUE;
}

gboolean VW_IsShown_Search_SubMenu()
{
	return nfui_nfobject_is_shown(search_sub_menu);
}

int VW_Move_Search_SubMenu(int x, int y)
{
	nfui_nfobject_move(search_sub_menu, x, y);
	return 0;
}

int VW_Get_Search_SubMenu_Geometry(int *x, int *y, int *w, int *h)
{
	gdk_window_get_geometry(((NFWINDOW *)search_sub_menu)->main_widget->window, 
		x, y, w, h, 0);
	return 0;
}

