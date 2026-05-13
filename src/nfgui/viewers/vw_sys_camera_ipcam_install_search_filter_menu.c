#include "nf_afx.h"
#include "scm.h"

#include "services/uxm.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ocam.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflabel.h"

#include "vw_sys_camera_ipcam_install_search_filter_menu.h"
#include "vw_sys_camera_ipcam_install_search_filter_popup.h"



#define MAC_SIZE_W							(360)
#define MAC_SIZE_H							(112)

#define MAC_BUTTON_POS_X					(12)
#define MAC_BUTTON_POS_Y					(15)
#define MAC_BUTTON_MARGIN					(15) 
#define MAC_BUTTON_SIZE_W					(MAC_SIZE_W - MAC_BUTTON_POS_X - MAC_BUTTON_MARGIN) 
#define MAC_BUTTON_SIZE_H					(40) 
#define MAC_BUTTON_GAP_2					(2)


static NFWINDOW *g_curwnd = 0;
static GdkPixbuf *g_menuBG[2][NFOBJECT_STATE_COUNT];
static guint g_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};

static int g_filtered = 0;


////////////////////////////////////////////////////////////////////
//
//
//

static void make_mac_bg_images()
{
	// menu button bg
	g_menuBG[0][0] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_N_BUTTON, MAC_BUTTON_SIZE_W, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	g_menuBG[0][1] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_O_BUTTON, MAC_BUTTON_SIZE_W, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	g_menuBG[0][2] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_P_BUTTON, MAC_BUTTON_SIZE_W, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	g_menuBG[0][3] = nf_ui_create_image_button_method(MK_IMG_TIMELINE_POPUP_D_BUTTON, MAC_BUTTON_SIZE_W, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

}

static gboolean post_filter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    		    VW_Camera_Filter_Menu_hide();
    		    VW_Create_Camera_Filter_Popup_Open(g_curwnd);
				g_filtered = 1;
    		break;

    	default:
    		break;
	}

	return FALSE;
	
}

static gboolean post_resetfilter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    		    VW_Camera_Filter_Menu_hide();
        		ocam_clear_filter();
				g_filtered = 1;
    		break;

    	default:
    		break;
	}

	return FALSE;
	
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    
	switch (evt->type) 
	{
        case NFOUTEVT_BUTTON_PRESS :
        {          
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            VW_Camera_Filter_Menu_hide();
        }
        break;
        
    	case GDK_DELETE:
    	{
    		g_curwnd = 0;
		}
		break;

	}
	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_Camera_Filter_Menu_Open(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT *wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	
	gchar *strBtn[2] = {"FILTER", "FILTER INITIALIZATION"};
	guint pos_y = 0;
	gint i;

	/* window */
	wnd = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, MAC_SIZE_W, MAC_SIZE_H);
	g_curwnd = wnd;
	nfui_nfwindow_set_title(wnd, "CAMERA LIST FILTER");
	nfui_regi_post_event_callback(wnd, post_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)wnd, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)wnd, GDK_BUTTON_PRESS, TRUE);

	make_mac_bg_images();

	/* fixed */
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, MAC_SIZE_W, 100);
	nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	pos_y = MAC_BUTTON_POS_Y;

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(g_menuBG[0], strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MAC_BUTTON_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)g_font_color);
		nfui_nfobject_set_size(obj, MAC_BUTTON_SIZE_W, MAC_BUTTON_SIZE_H);

		if(i != 0) 
			pos_y += (MAC_BUTTON_SIZE_H + MAC_BUTTON_GAP_2);

		if (i == 0)
			nfui_regi_post_event_callback(obj, post_filter_event_handler);

		if (i == 1)
			nfui_regi_post_event_callback(obj, post_resetfilter_event_handler);

		nfui_nffixed_put((NFFIXED*)main_fixed, obj, MAC_BUTTON_POS_X, pos_y);
		nfui_nfobject_show(obj);

	}

	nfui_nfwindow_add((NFWINDOW*)wnd, main_fixed);
	nfui_run_main_event_handler(wnd);
    nfui_nfobject_hide(wnd);

	return TRUE;
}

int VW_Destroy_Camera_Filter_Menu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

int VW_Camera_Filter_Menu_show()
{
	if (!g_curwnd) return 0;
	g_filtered = 0;
	nfui_nfobject_show(g_curwnd);
	nfui_make_key_hierarchy(g_curwnd);

	gtk_main();
	
	return g_filtered;
}

int VW_Camera_Filter_Menu_hide()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_hide(g_curwnd);
	
	gtk_main_quit();
	
	return 0;
}


