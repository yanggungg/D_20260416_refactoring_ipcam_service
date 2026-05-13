
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"

#include "vw_alarm_sched_ctrl.h"



#define BTN_POS_X			(16)
#define BTN_SIZE_W			(180)
#define BTN_SIZE_H			(34)

#define BTN1_POS_Y			(16)
#define BTN2_POS_Y			(BTN1_POS_Y + BTN_SIZE_H + 4)
#define BTN3_POS_Y			(BTN2_POS_Y + BTN_SIZE_H + 4)
#define BTN4_POS_Y			(BTN3_POS_Y + BTN_SIZE_H + 4)

#define WIN_SIZE_W			(BTN_POS_X + BTN_SIZE_W + 18)
#define WIN_SIZE_H			(BTN1_POS_Y + BTN_SIZE_H*4 + 8 + 18)	


static NFWINDOW *g_curwnd = 0;
static GdkPixbuf *btn_img[NFOBJECT_STATE_COUNT];
static gint ctl_ret = -1;


static gboolean post_off_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		ctl_ret = 0;
	}

	return FALSE;

}


static gboolean post_on_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		ctl_ret = 1;
	}

	return FALSE;

}


static gboolean post_event_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		ctl_ret = 2;

	}

	return FALSE;

}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		ctl_ret = -1;

	}

	return FALSE;

}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		break;

		case GDK_DELETE:
		    g_curwnd = 0;
			gtk_main_quit();
		break;

		default:
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

	if(evt->type == GDK_EXPOSE) 
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
	}
    	
	return FALSE;
}


gint VW_AlmSched_Ctrl(NFWINDOW *parent, guint x, guint y)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *on_btn;

	ctl_ret = -1;

	if (x + WIN_SIZE_W > DISPLAY_ACTIVE_WIDTH) x = DISPLAY_ACTIVE_WIDTH - WIN_SIZE_W;
	if (y + WIN_SIZE_H > DISPLAY_ACTIVE_HEIGHT) y = DISPLAY_ACTIVE_HEIGHT - WIN_SIZE_H;

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)x, (guint)y, (guint)WIN_SIZE_W, (guint)WIN_SIZE_H);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_window_event_cb);
	g_curwnd = win;


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, (guint)WIN_SIZE_W, (guint)WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);	


	obj = nftool_normal_button_create_popup_type2("ON", BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, BTN_POS_X, BTN1_POS_Y);
	nfui_regi_post_event_callback(obj, post_on_event_cb);
	on_btn = obj;

	obj = nftool_normal_button_create_popup_type2("OFF", BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, BTN_POS_X, BTN2_POS_Y);
	nfui_regi_post_event_callback(obj, post_off_event_cb);

	obj = nftool_normal_button_create_popup_type2("EVENT", BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, BTN_POS_X, BTN3_POS_Y);
	nfui_regi_post_event_callback(obj, post_event_event_cb);

	obj = nftool_normal_button_create_popup_type2("CANCEL", BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, BTN_POS_X, BTN4_POS_Y);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(on_btn, TRUE);

	nfui_page_open(PGID_ALM_SCHED_CONTROL, win, nfui_get_last_user());

	gtk_main();
		
	nfui_page_close(PGID_ALM_SCHED_CONTROL, win);

	return ctl_ret;
}


