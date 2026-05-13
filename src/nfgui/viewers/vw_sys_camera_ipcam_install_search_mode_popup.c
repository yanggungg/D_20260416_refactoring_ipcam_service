#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"
#include "ssm.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"

#include "vw.h"
#include "vw_sys_camera_ipcam_install_search_mode_popup.h"


static NFWINDOW *g_curwnd = 0;
static gint g_ret = 0;


static gboolean post_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint idx = 0;
	
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
		idx = nfui_radio_button_get_index((NFBUTTON*)obj);

		g_ret = idx;
	}

	return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
	
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_Camera_Install_Mode_Popup_Open(NFWINDOW *parent)
{
	NFOBJECT *win = NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj = NULL;
	NFOBJECT *ok_btn = NULL;

	gint i = 0;
    gint pos_x, pos_y;
    gint size_w, size_h;
    gint win_width, win_height;
    GSList *slist;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	gchar *strMode[2] = {"SINGLE MODE", "MULTI MODE"};
    
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    win_width = 500;
    win_height = 300;    

	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH - win_width) / 2, (DISPLAY_ACTIVE_HEIGHT - win_height) / 2, win_width, win_height);
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win_width, win_height);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA INSTALL MODE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, win_width - 8, 36);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x = 40;
	pos_y = 70;

	for (i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_mode_event_handler);

		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled((NFBUTTON*)obj, TRUE);
			g_ret = CAM_INSTALL_MODE_SINGLE;
		} else {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strMode[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, 250, size_h);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x + size_w + 10, pos_y);

		pos_y += size_h + 30;
	}

	obj = nftool_normal_button_create_type1("OK", 150);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfbutton_set_spacing((NFBUTTON*)obj, NORMAL_SPACING);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (win_width-150)/2, win_height-70);
	ok_btn = obj;

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(ok_btn, TRUE);
	
	nfui_page_open(PGID_IPCAMERA_INSTALL_MODE_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_IPCAMERA_INSTALL_MODE_POPUP, win);

	return g_ret;
}

