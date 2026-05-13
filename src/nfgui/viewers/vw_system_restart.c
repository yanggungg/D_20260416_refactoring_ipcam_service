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

#include "viewers/vw_system_restart.h"

//#include "nf_ui_arch_burn.h"

#include "modules/ssm.h"

#include "scm.h"
#include "vw.h"


// TODO: need new vkey
#include "ix_mem.h"
#include "smt.h"
#include "uxm.h"


#define RESTART_WIN_SIZE_W			(678)
#define RESTART_WIN_SIZE_H			(220)

#define RESTART_POS_X				((DISPLAY_ACTIVE_WIDTH - RESTART_WIN_SIZE_W)/4*2)
#define RESTART_POS_Y				((DISPLAY_ACTIVE_HEIGHT - RESTART_WIN_SIZE_H)/2) - 15


enum {
	STOP_BTN,
	REBOOT_BTN,
	CANCEL_BTN,
	BUTTON_CNT,
};

static NFOBJECT *rsWin;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_rsWin;
static NFOBJECT *g_prog;
static NFOBJECT *g_ex_btns[BUTTON_CNT];
static NFOBJECT *g_tag;
static NFOBJECT *g_memo;
static NFOBJECT *g_msg1_obj;
static NFOBJECT *g_msg2_obj;

static NFOBJECT *dev_obj = NULL;
static NFOBJECT *form_obj = NULL;
static NFOBJECT *wait_pop = NULL;


static gboolean post_stop_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	char *tag;

	if(evt->type == GDK_BUTTON_PRESS) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
			
		nfui_nflabel_erase(g_msg1_obj);
		nfui_nflabel_set_text(g_msg1_obj, "Please wait to stop the system.");
		nfui_signal_emit(g_msg1_obj, GDK_EXPOSE, FALSE);
		scm_enter_cam_upgrade_mode(IRET_SCM_ENTER_CAMUP_MODE);

		nfui_nfobject_disable(g_ex_btns[STOP_BTN]);
		nfui_signal_emit(g_ex_btns[STOP_BTN], GDK_EXPOSE, FALSE);

		nfui_nfobject_disable(g_ex_btns[CANCEL_BTN]);
		nfui_signal_emit(g_ex_btns[CANCEL_BTN], GDK_EXPOSE, FALSE);
	}

	return FALSE;
}

static gboolean post_reboot_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	int ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		scm_reboot_system(RR_CAM_UPGRADE, 0);
	}

	return FALSE;
} 

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean _proc_auto_reboot(void *data)
{
	g_message("CAM UPGRADE TIMEOUT!!!!\n");
	scm_reboot_system(RR_CAM_UPGRADE, 0);
	return FALSE;
}

static int _on_enter_camup_mode(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	nfui_nflabel_erase(g_msg1_obj);
	nfui_nflabel_set_text(g_msg1_obj, "The system has been stopped.\nYou may upgrade the IP camera.");
	nfui_signal_emit(g_msg1_obj, GDK_EXPOSE, FALSE);

	nfui_nflabel_set_text(g_msg2_obj, "Reboot the system after upgrade.");
	nfui_signal_emit(g_msg2_obj, GDK_EXPOSE, FALSE);

	nfui_nfobject_enable(g_ex_btns[REBOOT_BTN]);
	nfui_signal_emit(g_ex_btns[REBOOT_BTN], GDK_EXPOSE, FALSE);

	g_timeout_add(30 * 60 * 1000, _proc_auto_reboot, 0);
	return 0;
}

static gboolean post_rsWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = -1;
	NFOBJECT *top = NULL;
	
	switch (evt->type)
	{
		case GDK_DELETE:
			{
				uxm_unreg_imsg_event(obj, IRET_SCM_ENTER_CAMUP_MODE);

				g_curwnd = 0;

				gtk_main_quit();
			}
			break;

	case IRET_SCM_ENTER_CAMUP_MODE:
		_on_enter_camup_mode(obj, evt, data);
		break;

		default:
			break;
	}

	return FALSE;
}


static gboolean 
post_ae_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

/////////////////////////////////////////////////////////////////////
//
//
//

void VW_SystemRestart_Open(NFWINDOW *parent)
{
	NFOBJECT *rsFixed;
	NFOBJECT *obj;
	gint i;

	char msg1[] = "If you want to upgrade the IP camera,\nyou must stop the system."; 


	/* window */
	g_rsWin = (NFOBJECT*)nfui_nfwindow_new(parent, RESTART_POS_X, RESTART_POS_Y, RESTART_WIN_SIZE_W, RESTART_WIN_SIZE_H);
	g_curwnd = g_rsWin;
	nfui_regi_post_event_callback(g_rsWin, post_rsWin_event_handler);


	/* fixed */
	rsFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(rsFixed, RESTART_WIN_SIZE_W, RESTART_WIN_SIZE_H);
	nfui_regi_post_event_callback(rsFixed, post_ae_fixed_event_cb);
	nfui_nfobject_show(rsFixed);


	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP CAMERA UPGRADE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 620, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)rsFixed, obj, 4, 4);

	/* subtitle */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(msg1, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(398));
	g_msg1_obj = obj;
	nfui_nflabel_set_multi_line_type(obj, TRUE);
	nfui_nfobject_set_size(obj, 610, 44);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)rsFixed, obj, 30, 61);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(398));
	g_msg2_obj = obj;
	nfui_nfobject_set_size(obj, 610, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)rsFixed, obj, 30,120);

	/* button */
	g_ex_btns[STOP_BTN] = nftool_normal_button_create_type1("STOP SYSTEM", 182);
	nfui_regi_post_event_callback(g_ex_btns[STOP_BTN], post_stop_button_event_cb);
	nfui_nfobject_show(g_ex_btns[STOP_BTN]);
	nfui_nffixed_put((NFFIXED*)rsFixed, g_ex_btns[STOP_BTN], 30, 153);

	g_ex_btns[REBOOT_BTN] = nftool_normal_button_create_type1("REBOOT SYSTEM", 227);
	nfui_regi_post_event_callback(g_ex_btns[REBOOT_BTN], post_reboot_button_event_cb);
	nfui_nfobject_show(g_ex_btns[REBOOT_BTN]);
	nfui_nffixed_put((NFFIXED*)rsFixed, g_ex_btns[REBOOT_BTN], 222, 153);

	g_ex_btns[CANCEL_BTN] = nftool_normal_button_create_type1("CANCEL", 187);
	nfui_regi_post_event_callback(g_ex_btns[CANCEL_BTN], post_cancel_button_event_cb);
	nfui_nfobject_show(g_ex_btns[CANCEL_BTN]);
	nfui_nffixed_put((NFFIXED*)rsFixed, g_ex_btns[CANCEL_BTN], 459, 153);

	nfui_nfwindow_add((NFWINDOW*)g_rsWin, rsFixed);
	nfui_run_main_event_handler(g_rsWin);
	nfui_nfobject_show(g_rsWin);

	nfui_make_key_hierarchy((NFWINDOW*)g_rsWin);

	uxm_reg_imsg_event(g_rsWin, IRET_SCM_ENTER_CAMUP_MODE);

	smt_set_service(SMT_CAMFW_UPGRADE);
	nfui_page_open(PGID_IPCAM_UPGRADE, g_rsWin, nfui_get_last_user());

	nfui_nfobject_disable(g_ex_btns[REBOOT_BTN]);
	gtk_main();

	nfui_page_close(PGID_IPCAM_UPGRADE, g_rsWin);
	smt_return_to_previous();
}


