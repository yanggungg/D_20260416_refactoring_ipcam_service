
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"

#include "vw_modify_point_popup.h"

#define POPUP_BTN_SIZE_X			(20)
#define POPUP_BTN_SIZE_W			(180)
#define POPUP_BTN_SIZE_H			(34)

#define POPUP_BTN1_SIZE_Y			(20)
#define POPUP_BTN2_SIZE_Y			(POPUP_BTN1_SIZE_Y + POPUP_BTN_SIZE_H + 4)

#define POPUP_WIN_SIZE_W			(POPUP_BTN_SIZE_X + POPUP_BTN_SIZE_W + 18)
#define POPUP_WIN_SIZE_H			(POPUP_BTN1_SIZE_Y + POPUP_BTN_SIZE_H*2 + 8 + 18)	

static NFWINDOW *g_curwnd = 0;
static guint retVal = -1;


static gboolean post_add_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		retVal = ADD_POINT;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;

}


static gboolean post_del_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		retVal = DEL_POINT;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;

}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	switch(evt->type)
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
		break;	
	
		case GDK_EXPOSE:

		break;

		case GDK_DELETE:
			g_curwnd = 0;
			gtk_main_quit();
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
    gint i;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE) 
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

gint VW_modify_point_popup_open(NFWINDOW *parent, gint pos_x, gint pos_y, guint enable)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	retVal = -1;
	
	if (pos_x + POPUP_WIN_SIZE_W > DISPLAY_ACTIVE_WIDTH) pos_x = DISPLAY_ACTIVE_WIDTH - POPUP_WIN_SIZE_W;
	if (pos_y + POPUP_WIN_SIZE_H > DISPLAY_ACTIVE_HEIGHT) pos_y = DISPLAY_ACTIVE_HEIGHT - POPUP_WIN_SIZE_H;

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)pos_x, (guint)pos_y, (guint)POPUP_WIN_SIZE_W, (guint)POPUP_WIN_SIZE_H);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_window_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, (guint)POPUP_WIN_SIZE_W, (guint)POPUP_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);	

	obj = nftool_normal_button_create_popup_type2("ADD POINT", POPUP_BTN_SIZE_W);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, POPUP_BTN_SIZE_X, POPUP_BTN1_SIZE_Y);
	nfui_regi_post_event_callback(obj, post_add_event_cb);

	if (enable & (1 << ADD_POINT)) nfui_nfobject_enable(obj);

	obj = nftool_normal_button_create_popup_type2("DELETE POINT", POPUP_BTN_SIZE_W);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, POPUP_BTN_SIZE_X, POPUP_BTN2_SIZE_Y);
	nfui_regi_post_event_callback(obj, post_del_event_cb);

	if (enable & (1 << DEL_POINT)) nfui_nfobject_enable(obj);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_MODIFY_POINT_POPUP, win, nfui_get_last_user());
	gtk_main();
	nfui_page_close(PGID_MODIFY_POINT_POPUP, win);

	return retVal;
}

