
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"

#include "vw_live_ptz_full.h"

static gint g_ch = -1;

static NFWINDOW *g_curwnd = 0;


static gboolean _post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
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
	
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_PTZ_Full_Open(NFWINDOW *parent, gint ch)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *obj = NULL;

    gint win_width = 0, win_height = 0;

    win_width  = 117;
    win_height = 44;

    g_ch = ch;   

	vsm_change_sfc_by_zoom_opened_ptz(ch);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, DISPLAY_ACTIVE_WIDTH-22-win_width, DISPLAY_ACTIVE_HEIGHT-30-win_height, win_width, win_height);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, win_width, win_height);
	nfui_nfobject_show(main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_cb);	

	obj = nftool_normal_button_create_type2("EXIT", win_width);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 0, 0);
	nfui_regi_post_event_callback(obj, _post_close_button_event_handler);	

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_PTZ_FULL_MODE, win, nfui_get_last_user());

    gtk_main();

	nfui_page_close(PGID_PTZ_FULL_MODE, win);

	vsm_recover_sfc_by_zoom_closed_ptz();

	return g_ch;
}

