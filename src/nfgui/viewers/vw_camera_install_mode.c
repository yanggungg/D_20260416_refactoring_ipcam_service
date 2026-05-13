#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

//#include "services/uxm.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"

#include "vw.h"
#include "viewers/vw_camera_install_mode.h"

#include "modules/ssm.h"
#include "ix_mem.h"
#include "scm.h"
#include "smt.h"
#include "uxm.h"


#define IT_WIN_SIZE_W           (678)
#define IT_WIN_SIZE_H			(180)

#define IT_POS_X				((DISPLAY_ACTIVE_WIDTH - IT_WIN_SIZE_W)/4*2)
#define IT_POS_Y				((DISPLAY_ACTIVE_HEIGHT - IT_WIN_SIZE_H)/2 + 80)

enum {
	START_BTN,
	STOP_BTN,
	EXIT_BTN,
	BUTTON_CNT,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_itWin;
static NFOBJECT *g_ex_btns[BUTTON_CNT];

static NFOBJECT *wait_pop = NULL;


static gboolean post_start_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wbox = NULL;

	if(evt->type == GDK_BUTTON_PRESS) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        nfui_nfobject_disable(g_ex_btns[START_BTN]);
        nfui_nfobject_enable(g_ex_btns[STOP_BTN]);
        nfui_nfobject_disable(g_ex_btns[EXIT_BTN]);

    	nfui_set_key_focus(g_ex_btns[START_BTN], FALSE);
    	nfui_set_key_focus(g_ex_btns[STOP_BTN], TRUE);

        nfui_signal_emit(g_ex_btns[START_BTN], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ex_btns[STOP_BTN], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ex_btns[EXIT_BTN], GDK_EXPOSE, TRUE);
        
        scm_start_ipcam_install_mode();
	}

	return FALSE;
}

static gboolean post_stop_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *wbox = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        nfui_nfobject_disable(g_ex_btns[STOP_BTN]);
        nfui_nfobject_enable(g_ex_btns[START_BTN]);
        nfui_nfobject_enable(g_ex_btns[EXIT_BTN]);

    	nfui_set_key_focus(g_ex_btns[STOP_BTN], FALSE);
    	nfui_set_key_focus(g_ex_btns[EXIT_BTN], TRUE);

        nfui_signal_emit(g_ex_btns[START_BTN], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ex_btns[STOP_BTN], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ex_btns[EXIT_BTN], GDK_EXPOSE, TRUE);

        scm_stop_ipcam_install_mode();
	}

	return FALSE;
} 

static gboolean post_exit_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	int ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_destroy(g_curwnd);
	}

	return FALSE;
} 

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = -1;
	NFOBJECT *top = NULL;
	
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


static gboolean post_main_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
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

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	if(!nfui_nfobject_is_disabled(g_ex_btns[EXIT_BTN]))
		return TRUE;
	else
		return FALSE;
}


/////////////////////////////////////////////////////////////////////
//
//
//

void VW_Camera_InstallMode_Open(NFWINDOW *parent)
{
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	gint i;

	/* window */
	g_itWin = (NFOBJECT*)nfui_nfwindow_new(parent, IT_POS_X, IT_POS_Y, IT_WIN_SIZE_W, IT_WIN_SIZE_H);
	g_curwnd = g_itWin;
	nfui_regi_post_event_callback(g_itWin, post_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_itWin, returnkey_proc);

	/* fixed */
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, IT_WIN_SIZE_W, IT_WIN_SIZE_H);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_cb);
	nfui_nfobject_show(main_fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA INSTALLATION MODE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 620, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

	/* button */
	g_ex_btns[START_BTN] = nftool_normal_button_create_type1("START", 182);
	nfui_regi_post_event_callback(g_ex_btns[START_BTN], post_start_button_event_cb);
	nfui_nfobject_show(g_ex_btns[START_BTN]);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_ex_btns[START_BTN], 30, 120);
    
	g_ex_btns[STOP_BTN] = nftool_normal_button_create_type1("STOP", 227);
	nfui_regi_post_event_callback(g_ex_btns[STOP_BTN], post_stop_button_event_cb);
    nfui_nfobject_disable(g_ex_btns[STOP_BTN]);
	nfui_nfobject_show(g_ex_btns[STOP_BTN]);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_ex_btns[STOP_BTN], 222, 120);

	g_ex_btns[EXIT_BTN] = nftool_normal_button_create_type1("EXIT", 187);
	nfui_regi_post_event_callback(g_ex_btns[EXIT_BTN], post_exit_button_event_cb);
	nfui_nfobject_show(g_ex_btns[EXIT_BTN]);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_ex_btns[EXIT_BTN], 459, 120);

	nfui_nfwindow_add((NFWINDOW*)g_itWin, main_fixed);
	nfui_run_main_event_handler(g_itWin);
	nfui_nfobject_show(g_itWin);

	nfui_make_key_hierarchy((NFWINDOW*)g_itWin);
	nfui_set_key_focus(g_ex_btns[START_BTN], TRUE);

	smt_set_service(SMT_CAM_INSTALL_MODE);
	nfui_page_open(PGID_CAMERA_INSTALL_MODE_POPUP, g_itWin, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_CAMERA_INSTALL_MODE_POPUP, g_itWin);
	smt_return_to_previous();
}


