
#include "nf_afx.h"


#include "../service/ddns2_manager.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nflistbox.h"

#include "vw.h"
#include "vw_user_guide.h"

static NFWINDOW *g_curwnd = 0;



static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}	
	else if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint win_w, win_h;
    gint img_w, img_h;

	if (evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &win_w, &win_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, win_w, win_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_get_image_size(IMG_MANUAL_KR_1, &img_w, &img_h);
        nfutil_draw_image(drawable, gc, IMG_MANUAL_KR_1, (win_w-img_w)/2, 80, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &win_w, &win_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, win_w, win_h);
	}
}

static gboolean close_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
	}

	return FALSE;
}


gboolean vw_open_user_guide(NFWINDOW *parent)
{
    NFOBJECT *main_wnd;
    NFOBJECT *main_fixed;
	NFOBJECT *obj = NULL;

	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, 0, 0, 1920, 1080, "USER GUIDE", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;
	
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);



// CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_regi_post_event_callback(obj, close_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (1920-192)/2, 1080-44);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);	
	nfui_make_key_hierarchy(main_wnd);
	
	nfui_page_open(PGID_USER_GUIDE, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_USER_GUIDE, main_wnd);

	return TRUE;
}

