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

#include "vw_timeline_popup_submenu_export.h"
#include "vw_timeline_popup_submenu_search.h"
#include "vw_timeline_popup_submenu_pano.h"


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
static NFOBJECT *pano_sub_menu;
static GdkPixbuf *g_subBG[2][NFOBJECT_STATE_COUNT];
static NFOBJECT *g_submenu_camtitle[GUI_CHANNEL_CNT];
static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};

////////////////////////////////////////////////////////////////////
//
//
//
//

static void make_vtp_bg_images()
{
	g_subBG[0][0] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_N_BUTTON2, 170, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_subBG[0][1] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_O_BUTTON2, 170, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_subBG[0][2] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_P_BUTTON2, 170, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_subBG[0][3] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_D_BUTTON2, 170, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

}


static gboolean post_pano_sub_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
	case GDK_DELETE:
		g_curwnd = 0;
		break;
	}
	return FALSE;
}


static gboolean post_pano_wnd_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	return FALSE;
}

static gboolean post_pano_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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



static gboolean panorama_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{
//				if(nfui_nfobject_is_shown(export_sub_menu)) {
//					VW_Hide_Export_SubMenu();
//				}

				if (VW_IsShown_Search_SubMenu()) {
					VW_Hide_Search_SubMenu();
				}
#ifdef VER2
				if (VW_IsShown_Pano_SubMenu()) {
					VW_Show_Pano_SubMenu();
				}
#endif
			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_panosub_menu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS)
		nfui_nfobject_hide(obj);

	return FALSE;

}

////////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_Pano_SubMenu(NFWINDOW *parent)
{
	NFOBJECT *vtp_f;
	NFOBJECT *obj;
	
	guint pos_y = 0;
	gint i;
	gchar panosub_strBtn[GUI_CHANNEL_CNT][16] = {"1.", "2.", "3.", "4.", "5.", "6.", "7.", "8."};

	/* window */
	pano_sub_menu = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 200, 380);
	g_curwnd = pano_sub_menu;
	nfui_nfwindow_set_title(pano_sub_menu, "TIMELINE POPUP - SUBMENU PANORAMA");
	nfui_regi_post_event_callback(pano_sub_menu, post_pano_sub_event_cb);
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)pano_sub_menu)->main_widget), FALSE);
//	nfui_nfwindow_use_outside_evt((NFWINDOW*)pano_sub_menu, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)pano_sub_menu, GDK_BUTTON_PRESS , TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)pano_sub_menu, GDK_MOTION_NOTIFY, TRUE);
	
//	gtk_widget_add_events(((NFWINDOW*)pano_sub_menu)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(pano_sub_menu, post_pano_wnd_event_cb);


	make_vtp_bg_images();

	/* fixed */
	vtp_f = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(vtp_f, 200, 380);
	nfui_regi_post_event_callback(vtp_f, post_pano_fixed_event_cb);
	nfui_nfobject_show(vtp_f);


	/* */
	pos_y = VTP_BUTTON_POS_Y;

	for(i=0; i<var_get_ch_count(); i++) {
			obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_subBG[0], panosub_strBtn[i]);
			g_submenu_camtitle[i] = obj;
			nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, VTP_BUTTON_MARGIN);
			nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
			nfui_nfobject_set_size(obj, 170, VTP_BUTTON_SIZE_H);

		if(i != 0) 
			pos_y += (VTP_BUTTON_SIZE_H + VTP_BUTTON_GAP_2);

		nfui_nffixed_put((NFFIXED*)vtp_f, obj, VTP_BUTTON_POS_X, pos_y);
		nfui_nfobject_show(obj);
	}

	nfui_nfwindow_add((NFWINDOW*)pano_sub_menu, vtp_f);

	nfui_run_main_event_handler(pano_sub_menu);
	nfui_nfobject_hide(pano_sub_menu);
	return TRUE;
}

gboolean VW_Hide_Pano_SubMenu()
{
#ifdef VER2
	nfui_unregi_semi_modal(pano_sub_menu);
	nfui_nfobject_hide(pano_sub_menu);
#endif

	return TRUE;
}


gboolean VW_Show_Pano_SubMenu()
{
	int i;
	char buf[STRING_SIZE_CAMTITLE];
	CameraData camdata[GUI_CHANNEL_CNT];

	memset(camdata, 0x00, sizeof(CameraData)*GUI_CHANNEL_CNT);
	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		DAL_get_camera_data(&camdata[i], i);
		strcpy(buf, camdata[i].title);
		nfui_nfbutton_set_text(g_submenu_camtitle[i], buf);
	}

	nfui_regi_semi_modal(pano_sub_menu);
	nfui_nfobject_show(pano_sub_menu);

	return TRUE;
}


gboolean VW_IsShown_Pano_SubMenu()
{
	return nfui_nfobject_is_shown(pano_sub_menu);
}

int VW_Move_Pano_SubMenu(int x, int y)
{
	nfui_nfobject_move(pano_sub_menu, x, y);
	return 0;
}

int VW_Get_Pano_SubMenu_Geometry(int *x, int *y, int *w, int *h)
{
	gdk_window_get_geometry(((NFWINDOW *)pano_sub_menu)->main_widget->window, 
		x, y, w, h, 0);
	return 0;
}

