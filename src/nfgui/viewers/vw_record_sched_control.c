
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfimage.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_sched_control.h"



#define REC_SCHE_CTL_BTN_SIZE_X			(20)
#define REC_SCHE_CTL_BTN_SIZE_W			(180)
#define REC_SCHE_CTL_BTN_SIZE_H			(34)

#define REC_SCHE_CTL_BTN1_SIZE_Y		(20)
#define REC_SCHE_CTL_BTN2_SIZE_Y		(REC_SCHE_CTL_BTN1_SIZE_Y + REC_SCHE_CTL_BTN_SIZE_H + 4)
#define REC_SCHE_CTL_BTN3_SIZE_Y		(REC_SCHE_CTL_BTN2_SIZE_Y + REC_SCHE_CTL_BTN_SIZE_H + 4)

#define REC_SCHE_CTL_WIN_SIZE_W			(REC_SCHE_CTL_BTN_SIZE_X + REC_SCHE_CTL_BTN_SIZE_W + 18)
#define REC_SCHE_CTL_WIN_SIZE_H			(REC_SCHE_CTL_BTN1_SIZE_Y + REC_SCHE_CTL_BTN_SIZE_H*3 + 8 + 18)	



static NFWINDOW *g_curwnd = 0;
static GdkPixbuf *btn_img[NFOBJECT_STATE_COUNT];
static guint ctl_ret = SCHE_CANCEL_EVT;



static gboolean post_on_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		ctl_ret = SCHE_ON_EVT;

		nfui_nfobject_destroy(top);
	}

	return FALSE;

}


static gboolean post_off_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		ctl_ret = SCHE_OFF_EVT;

		nfui_nfobject_destroy(top);
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

		ctl_ret = SCHE_CANCEL_EVT;

		nfui_nfobject_destroy(top);
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


guint VW_RecSched_Control_Page(NFWINDOW *parent, guint x, guint y)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *on_btn;

	ctl_ret = SCHE_CANCEL_EVT;
	
	if (x + REC_SCHE_CTL_WIN_SIZE_W > DISPLAY_ACTIVE_WIDTH) x = DISPLAY_ACTIVE_WIDTH - REC_SCHE_CTL_WIN_SIZE_W;
	if (y + REC_SCHE_CTL_WIN_SIZE_H > DISPLAY_ACTIVE_HEIGHT) y = DISPLAY_ACTIVE_HEIGHT - REC_SCHE_CTL_WIN_SIZE_H;

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 
				(guint)x, (guint)y, (guint)REC_SCHE_CTL_WIN_SIZE_W, (guint)REC_SCHE_CTL_WIN_SIZE_H);
	g_curwnd = win;
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_window_event_cb);


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, (guint)REC_SCHE_CTL_WIN_SIZE_W, (guint)REC_SCHE_CTL_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);	


	obj = nftool_normal_button_create_popup_type2("ON", REC_SCHE_CTL_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, REC_SCHE_CTL_BTN_SIZE_X, REC_SCHE_CTL_BTN1_SIZE_Y);
	nfui_regi_post_event_callback(obj, post_on_event_cb);
	on_btn = obj;

	obj = nftool_normal_button_create_popup_type2("OFF", REC_SCHE_CTL_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, REC_SCHE_CTL_BTN_SIZE_X, REC_SCHE_CTL_BTN2_SIZE_Y);
	nfui_regi_post_event_callback(obj, post_off_event_cb);

	obj = nftool_normal_button_create_popup_type2("CANCEL", REC_SCHE_CTL_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, REC_SCHE_CTL_BTN_SIZE_X, REC_SCHE_CTL_BTN3_SIZE_Y);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(on_btn, TRUE);
	nfui_page_open(PGID_REC_SCHED_CONTROL, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_REC_SCHED_CONTROL, win);

		
	return ctl_ret;
}


