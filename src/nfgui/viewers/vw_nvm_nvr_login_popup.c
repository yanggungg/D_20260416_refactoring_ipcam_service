#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "services/scm.h"
#include "tools/nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vkeyboard.h"
#include "nvm.h"

#define NVR_LOGIN_WIN_W      (670)
#define NVR_LOGIN_WIN_H      (230)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_id = NULL;
static NFOBJECT *g_pw = NULL;
static gint g_nvr_idx = 0;
static gint g_ret = 0;


static gboolean post_user_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		guint x, y;
		gchar *strTemp;

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_NORMAL);

		if (strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_password_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		guint x, y;
		gchar *strTemp;

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 15, VKEY_PASSWORD);

		if (strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
    NVR_DATA_T *ndata = NULL;
    gint res = 0;
    
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		ndata = nvm_get_nvr_data(g_nvr_idx);
		xmm_update_nvr_info(ndata->hostname, ndata->http_port, nfui_nflabel_get_text((NFLABEL*)g_id), nfui_nflabel_get_text((NFLABEL*)g_pw));

		ifree(ndata);

	    g_ret = 1;
		top = nfui_nfobject_get_top(obj);
		if (top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_ret = 0;
        
		top = nfui_nfobject_get_top(obj);
		if (top) nfui_nfobject_destroy(top);
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
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_NVM_Nvr_Login_Popup_Open(NFWINDOW *parent, gint nvr_idx)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *ok_btn;
    gint pos_x, pos_y;


    g_nvr_idx = nvr_idx;
    g_ret = 0;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH-NVR_LOGIN_WIN_W)/2, (DISPLAY_ACTIVE_HEIGHT-NVR_LOGIN_WIN_H)/2, NVR_LOGIN_WIN_W, NVR_LOGIN_WIN_H);
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, NVR_LOGIN_WIN_W, NVR_LOGIN_WIN_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LOG IN", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, NVR_LOGIN_WIN_W - 8, 36);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 26;
	pos_y = 64;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_user_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);
	g_id = obj;

    pos_y += (40 + 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_password_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);
	g_pw = obj;

    pos_y += (40 + 4);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (NVR_LOGIN_WIN_W/2)-174-10, NVR_LOGIN_WIN_H-56);
	ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (NVR_LOGIN_WIN_W/2)+10, NVR_LOGIN_WIN_H-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(ok_btn, TRUE);

	nfui_page_open(PGID_CAMERA_INSTALL_NVR_LOGIN_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAMERA_INSTALL_NVR_LOGIN_POPUP, win);

	return g_ret;
}


