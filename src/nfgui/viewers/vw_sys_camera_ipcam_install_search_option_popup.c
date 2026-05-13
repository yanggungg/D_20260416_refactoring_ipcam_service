#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"
#include "modules/ssm.h"

//#include "services/uxm.h"

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
#include "viewers/objects/nfcheckbutton.h"
#include "vw_vkeyboard.h"

#define NNO_WIN_SIZE_W	(600)
#define NNO_WIN_SIZE_H	(220)

#define NNO_POS_X		((DISPLAY_ACTIVE_WIDTH - NNO_WIN_SIZE_W)/4*2)
#define NNO_POS_Y		((DISPLAY_ACTIVE_HEIGHT - NNO_WIN_SIZE_H)/2)

static IPCamInstOptData *g_optdata = NULL;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_auto_scan_chk;


static void _get_data_from_obj()
{
    g_optdata->auto_scan = nfui_check_button_get_active((NFCHECKBUTTON*)g_auto_scan_chk);
}

static gboolean post_apply_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _get_data_from_obj();

        DAL_set_ipcam_install_opt_data(g_optdata);
        
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);		
	}

	return FALSE;
}

static gboolean post_cancel_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) 
	{
		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
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

///////////////////////////////////////////////////////////////////////
//
//
//

gint VW_Camera_Install_Option_Popup_Open(NFWINDOW *parent, IPCamInstOptData *data)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

	gint size_w, size_h;
	gint pos_x, pos_y;

    g_optdata = data;
    
	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, NNO_POS_X, NNO_POS_Y, NNO_WIN_SIZE_W, NNO_WIN_SIZE_H);
	nfui_regi_post_event_callback(win, post_win_event_handler);
	g_curwnd = win;

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, NNO_WIN_SIZE_W, NNO_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);
	
	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OPTION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));	
	nfui_nfobject_set_size(obj, NNO_WIN_SIZE_W - 8, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 20;
	pos_y = 70;

	obj = nfui_checkbutton_new(g_optdata->auto_scan);
	nfui_check_button_set_skin_type((NFCHECKBUTTON*)obj, NFCHECK_TYPE_POPUP_NORMAL);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	g_auto_scan_chk = obj;

	nfui_check_get_size((NFCHECKBUTTON*)obj, &size_w, &size_h);

	pos_x += size_w + 10;
	pos_y += (size_h - 40) / 2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Automatically search camera on startup", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nfobject_set_size(obj, NNO_WIN_SIZE_W - 60, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = nftool_normal_button_create_type1("APPLY", 150);
	nfui_regi_post_event_callback(obj, post_apply_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (NNO_WIN_SIZE_W/2) - 152, NNO_WIN_SIZE_H - 60);

	obj = nftool_normal_button_create_type1("CANCEL", 150);
	nfui_regi_post_event_callback(obj, post_cancel_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (NNO_WIN_SIZE_W/2) + 2, NNO_WIN_SIZE_H - 60);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_page_open(PGID_CAMERA_INSTALL_OPTION_POPUP, win, nfui_get_last_user());
	
	gtk_main();

	nfui_page_close(PGID_CAMERA_INSTALL_OPTION_POPUP, win);

	return 0;
}


