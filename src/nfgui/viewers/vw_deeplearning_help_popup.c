#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"

#include "vw_deeplearning_help_popup.h"
#include "ix_mem.h"
#include "scm.h"


static NFWINDOW *g_curwnd = 0;


static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;
	GdkPixbuf *pbuf;

	gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfutil_draw_image(drawable, gc, "ai_detection_help.png", (gint)obj->x+10, (gint)obj->y+50, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
		g_curwnd = 0;
	}

	return FALSE;
}

static gboolean post_exit_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

gboolean vw_deeplearning_help_popup_open(NFWINDOW *parent) 
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;

	main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-994-40)/2, (1080-598-100)/2, 994+40, 598+120);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)main_wnd, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size((NFOBJECT*)main_fixed, 994+40, 598+120);
    nfui_nfobject_show(main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	obj = nftool_normal_button_create_popup_type2("EXIT", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (994+40-174)/2, 598+120-44);
	nfui_regi_post_event_callback(obj, post_exit_btn_event_handler);

	nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
    nfui_run_main_event_handler(g_curwnd);
    nfui_nfobject_show(main_wnd);
    
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());
    
	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return 0;
}
