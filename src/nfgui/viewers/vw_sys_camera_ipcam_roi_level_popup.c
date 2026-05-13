
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

#include "vw_sys_camera_ipcam_roi_level_popup.h"




#define ROI_SEL_LV_BTN_POS_X			(20)
#define ROI_SEL_LV_BTN_SIZE_W			(180)
#define ROI_SEL_LV_BTN_SIZE_H			(34)

#define ROI_SEL_LV_BTN1_POS_Y		    (20)
#define ROI_SEL_LV_BTN2_POS_Y		    (ROI_SEL_LV_BTN1_POS_Y + ROI_SEL_LV_BTN_SIZE_H + 4)
#define ROI_SEL_LV_BTN3_POS_Y		    (ROI_SEL_LV_BTN2_POS_Y + ROI_SEL_LV_BTN_SIZE_H + 4)

#define ROI_SEL_LV_WIN_SIZE_W			(ROI_SEL_LV_BTN_POS_X + ROI_SEL_LV_BTN_SIZE_W + 18)
#define ROI_SEL_LV_WIN_SIZE_H			(ROI_SEL_LV_BTN1_POS_Y + ROI_SEL_LV_BTN_SIZE_H*3 + 8 + 18)



static NFWINDOW *g_curwnd = 0;
static GdkPixbuf *btn_img[NFOBJECT_STATE_COUNT];
static guint ctl_ret = ROI_CANCEL;



static gboolean post_interest_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		ctl_ret = ROI_INTEREST;

		nfui_nfobject_destroy(top);
	}

	return FALSE;

}


static gboolean post_no_interest_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		ctl_ret = ROI_NO_INTEREST;

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

		ctl_ret = ROI_CANCEL;

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


guint VW_ROI_Level_Select_Popup(NFWINDOW *parent, guint x, guint y)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *on_btn;

	ctl_ret = ROI_CANCEL;

	if (x + ROI_SEL_LV_WIN_SIZE_W > DISPLAY_ACTIVE_WIDTH) x = DISPLAY_ACTIVE_WIDTH - ROI_SEL_LV_WIN_SIZE_W;
	if (y + ROI_SEL_LV_WIN_SIZE_H > DISPLAY_ACTIVE_HEIGHT) y = DISPLAY_ACTIVE_HEIGHT - ROI_SEL_LV_WIN_SIZE_H;

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent,
				(guint)x, (guint)y, (guint)ROI_SEL_LV_WIN_SIZE_W, (guint)ROI_SEL_LV_WIN_SIZE_H);
	g_curwnd = win;
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_window_event_cb);


	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, (guint)ROI_SEL_LV_WIN_SIZE_W, (guint)ROI_SEL_LV_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);


	obj = nftool_normal_button_create_popup_type2("INTEREST", ROI_SEL_LV_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, ROI_SEL_LV_BTN_POS_X, ROI_SEL_LV_BTN1_POS_Y);
	nfui_regi_post_event_callback(obj, post_interest_event_cb);
	on_btn = obj;

	obj = nftool_normal_button_create_popup_type2("NO INTEREST", ROI_SEL_LV_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, ROI_SEL_LV_BTN_POS_X, ROI_SEL_LV_BTN2_POS_Y);
	nfui_regi_post_event_callback(obj, post_no_interest_event_cb);

	obj = nftool_normal_button_create_popup_type2("CANCEL", ROI_SEL_LV_BTN_SIZE_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, ROI_SEL_LV_BTN_POS_X, ROI_SEL_LV_BTN3_POS_Y);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(on_btn, TRUE);
	nfui_page_open(PGID_ROI_SELECT_LEVEL_POPUP, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_ROI_SELECT_LEVEL_POPUP, win);


	return ctl_ret;
}
