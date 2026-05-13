#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_spot.h"
#include "vw_sys_disp_spot_conf.h"
#include "vw_sys_disp_spot_conf_submenu.h"
#include "vw_sys_disp_spot_setup.h"

#define	CONF_MENU_WIDTH		(guint)(DISPLAY_IS_D1 ? 100:200)
#define	CONF_MENU_HEIGHT	(guint)(DISPLAY_IS_D1 ? 76:168)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *conf_fixed[16];

static NFOBJECT *menu_win = NULL;
static gchar bg_img_name[64];

static gint ret_val;

static gboolean post_modify_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint ret;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		ret_val	= 0;
		VW_Destroy_Spot_SubMenu();	
	}

	return FALSE;
}

static gboolean post_delete_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		guint i;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{					
			return FALSE;
		}

		ret_val	= 1;	
		VW_Destroy_Spot_SubMenu();	
	}

	return FALSE;
}

static gboolean post_cancel_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		ret_val	= 2;	
		VW_Destroy_Spot_SubMenu();
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    GdkDrawable *drawable = NULL;
		GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if(evt->type == GDK_DELETE) {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////
//
//
//
	
gint VW_Create_Spot_SubMenu(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT *menu_fixed;
	NFOBJECT *modify_btn, *delete_btn, *cancel_btn;

	gint btn_x, btn_y;

	ret_val	= -1;

	menu_win = nfui_nfwindow_new(parent, x, y, CONF_MENU_WIDTH, CONF_MENU_HEIGHT);
	g_curwnd = menu_win;
	menu_fixed = nfui_nffixed_new();
	nfui_nfobject_show(menu_fixed);
	nfui_nfwindow_add((NFWINDOW*)menu_win, menu_fixed);
	nfui_regi_post_event_callback(menu_win, post_win_event_handler);

	modify_btn = nftool_normal_button_create_type1("MODIFY", 162);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(modify_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(modify_btn);

	delete_btn = nftool_normal_button_create_type1("DELETE", 162);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(delete_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(delete_btn);

	cancel_btn = nftool_normal_button_create_type1("CANCEL", 162);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(cancel_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(cancel_btn);

	btn_x = (CONF_MENU_WIDTH - NORMAL_BTN_WIDTH)/2;
	btn_y = (CONF_MENU_HEIGHT - NORMAL_BTN_HEIGHT)/2 - NORMAL_BTN_HEIGHT - (guint)(DISPLAY_IS_D1 ? 8:18);

	nfui_nffixed_put((NFFIXED*)menu_fixed, modify_btn, btn_x, btn_y-8);		btn_y += NORMAL_BTN_HEIGHT + (guint)(DISPLAY_IS_D1 ? 8:18);
	nfui_nffixed_put((NFFIXED*)menu_fixed, delete_btn, btn_x, btn_y-8);		btn_y += NORMAL_BTN_HEIGHT + (guint)(DISPLAY_IS_D1 ? 8:18);
	nfui_nffixed_put((NFFIXED*)menu_fixed, cancel_btn, btn_x, btn_y-8);

	nfui_nfobject_show(modify_btn);
	nfui_nfobject_show(delete_btn);
	nfui_nfobject_show(cancel_btn);

	nfui_regi_post_event_callback(modify_btn, post_modify_btn_event_handler);
	nfui_regi_post_event_callback(delete_btn, post_delete_btn_event_handler);
	nfui_regi_post_event_callback(cancel_btn, post_cancel_btn_event_handler);
	nfui_regi_pre_event_callback(menu_fixed, pre_main_fixed_event_handler);


	nfui_run_main_event_handler(menu_win);
	nfui_nfobject_show(menu_win);

	nfui_make_key_hierarchy((NFWINDOW*)menu_win);
	nfui_set_key_focus(modify_btn, TRUE);
	nfui_page_open(PGID_SPOT_CONF_MENU, menu_win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SPOT_CONF_MENU, menu_win);

	return ret_val;
}


void VW_Destroy_Spot_SubMenu()
{
	nfui_nfobject_destroy(menu_win);
	menu_win = NULL;
}
