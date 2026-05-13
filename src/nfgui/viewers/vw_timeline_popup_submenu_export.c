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
static NFOBJECT *export_sub_menu;
static GdkPixbuf *g_subBG[2][NFOBJECT_STATE_COUNT];
static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};

////////////////////////////////////////////////////////////////////////
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

static gboolean post_event_search_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GTimeVal stime, etime;

	memset(&stime, 0x00, sizeof(GTimeVal));
	memset(&etime, 0x00, sizeof(GTimeVal));
	switch(evt->type) {
	case GDK_BUTTON_PRESS:
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		get_time_data(&stime, &etime);
		stm_set_time_t(stime.tv_sec);
		stm_set_endtime_t(etime.tv_sec);

		if (vsm_get_vmode() == VMODE_LV)
		{
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


static gboolean post_export_sub_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
	case GDK_DELETE:
		g_curwnd = 0;
		break;
	}
	return FALSE;
}




static gboolean post_export_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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





static gboolean export_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{

#ifdef VER2
				if (VW_IsShown_Pano_SubMenu()) {
					VW_Hide_Pano_SubMenu();
				}
#endif

				if (VW_IsShown_Search_SubMenu()) {
					VW_Hide_Search_SubMenu();
				}

//				if(!nfui_nfobject_is_shown(export_sub_menu)) {
//					VW_Show_Export_SubMenu();
//				}

			}
			break;
		case GDK_BUTTON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}





////////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_Export_SubMenu(NFWINDOW *parent)
{
	NFOBJECT *vtp_f;
	NFOBJECT *obj;
	
	gchar *exportsub_strBtn[3] = {"USB", "EMAIL", "FTP"};
	guint pos_y = 0;
	gint i;

	make_vtp_bg_images();

	/* window */
	export_sub_menu = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 200, 150);
	g_curwnd = export_sub_menu;
	nfui_nfwindow_set_title(export_sub_menu, "TIMELINE POPUP - SUBMENU EXPORT");
	nfui_regi_post_event_callback(export_sub_menu, post_export_sub_event_cb);
	gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)export_sub_menu)->main_widget), FALSE);
//	nfui_nfwindow_use_outside_evt((NFWINDOW*)export_sub_menu, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)export_sub_menu, GDK_BUTTON_PRESS, TRUE);
//	nfui_nfwindow_set_mask((NFWINDOW*)export_sub_menu, GDK_MOTION_NOTIFY, TRUE);
	
//	gtk_widget_add_events(((NFWINDOW*)export_sub_menu)->main_widget, GDK_POINTER_MOTION_HINT_MASK);


	/* fixed */
	vtp_f = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(vtp_f, 200, 100);
	nfui_regi_post_event_callback(vtp_f, post_export_fixed_event_cb);
	nfui_nfobject_show(vtp_f);


	/* */
	pos_y = VTP_BUTTON_POS_Y;

	for(i=0; i<3; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_subBG[0], exportsub_strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, VTP_BUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, 170, VTP_BUTTON_SIZE_H);

		if(i != 0) 
			pos_y += (VTP_BUTTON_SIZE_H + VTP_BUTTON_GAP_2);
			
			if (i == 1)
				nfui_nfobject_disable(obj);

		nfui_nffixed_put((NFFIXED*)vtp_f, obj, VTP_BUTTON_POS_X, pos_y);
		nfui_nfobject_show(obj);
	}

	nfui_nfwindow_add((NFWINDOW*)export_sub_menu, vtp_f);

	nfui_run_main_event_handler(export_sub_menu);
	nfui_nfobject_hide(export_sub_menu);


	return TRUE;
}

gboolean VW_Hide_Export_SubMenu()
{
	nfui_unregi_semi_modal(export_sub_menu);
	nfui_nfobject_hide(export_sub_menu);

	return TRUE;
}

gboolean VW_Show_Export_SubMenu()
{
	nfui_regi_semi_modal(export_sub_menu);
	nfui_nfobject_show(export_sub_menu);

	return TRUE;
}

gboolean VW_IsShown_Export_SubMenu()
{
	return nfui_nfobject_is_shown(export_sub_menu);
}
