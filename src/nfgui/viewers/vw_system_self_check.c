
#include "nf_afx.h"
#include "scm.h"
#include "iux_msg.h"
#include "uxm.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfimage.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "support/nf_ui_page_manager.h"
#include "objects/nfimglabel.h"

#include "ix_mem.h"

#include "modules/ssm.h"
#include "vw_vkeyboard.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_system_self_internal.h"
#include "vw_sys_main.h"

#define SSC_WIN_SIZE_X					(610)
#define SSC_WIN_SIZE_Y					(320)
#define SSC_WIN_SIZE_W					(700)
#define SSC_WIN_SIZE_H					(430)

#define SSC_TITLE_H					(36)
#define SSC_TITLE_X					(4)
#define SSC_TITLE_Y					(4)

#define SSC_CELL_W					(50)

#define SSC_LABEL_H					(40)
#define SSC_LABEL_W					(400)
#define SSC_LABEL_TEXT_MARGIN				(20)

#define SSC_TABLE_ROW					(SSC_OBJ_MAX)
#define SSC_TABLE_COL					(2)

#define SSC_TABLE_ROW_SPACE				(2)
#define SSC_TABLE_COL_SPACE				(1)

#define SSC_TABLE_W					(SSC_LABEL_W + SSC_CELL_W + SSC_TABLE_ROW_SPACE)

#define SSC_TABLE_X					(SSC_TITLE_X + SSC_LABEL_TEXT_MARGIN + 5)
#define SSC_TABLE_Y					(SSC_TITLE_H + SSC_LABEL_H + (SSC_TABLE_ROW_SPACE * 2))

// <---- CANCEL, RESULT BUTTON
#define SSC_BTN_X					(255)
#define SSC_BTN_Y					(SSC_WIN_SIZE_H - 75)
#define SSC_BTN_W					(174)


enum {
	SSC_POWER = 0,
	SSC_NET,
	SSC_HDD,
	SSC_PORT,
	SSC_OBJ_MAX
};

static NFWINDOW *g_curwnd = 0;
static NFWINDOW *g_parent = 0;
static NFOBJECT *g_wait_obj;
static guint g_st_timer = 0;
static guint g_waiting_timer = 0;
static SELF_CHECK_MASK_E g_mask;
static SELF_RESULT_T result;
static NFOBJECT *chkImg_obj[SSC_OBJ_MAX];
static NFOBJECT *cancel_obj;
static NFOBJECT *result_obj;
static NFOBJECT *complete_obj;
static gint g_current_step = -1;

static gint g_running_signal = 0;
static NFOBJECT *g_stop_wait = NULL;



static gboolean _check_waiting_timer(gpointer data)
{
	static gint img_idx = 0;
	
	if (img_idx >= 6) img_idx = 0;

	NFUTIL_THREADS_ENTER();

	if (img_idx == 0) 	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_01);
	else if (img_idx == 1)	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_02);
	else if (img_idx == 2)	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_03);
	else if (img_idx == 3)	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_04);
	else if (img_idx == 4)	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_05);
	else if (img_idx == 5)	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_06);

	nfui_signal_emit(g_wait_obj, GDK_EXPOSE, TRUE);

	NFUTIL_THREADS_LEAVE();

	img_idx++;

	return TRUE;
}

static gint _run_waiting_timer()
{
	g_waiting_timer = g_timeout_add(250, _check_waiting_timer, 0);
	return 0;
}

static gint _end_waiting_timer()
{
	if (g_waiting_timer)
	{
		g_source_remove(g_waiting_timer);
		g_waiting_timer = 0;
	}

	return 0;
}

static gint _end_process()
{
	_end_waiting_timer();

	nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_LOADING_0);
	nfui_nfobject_set_size(g_wait_obj, 89, 89);
	nfui_signal_emit(g_wait_obj, GDK_EXPOSE, TRUE);

	nfui_nfobject_hide(cancel_obj);
	nfui_signal_emit(cancel_obj, GDK_EXPOSE, TRUE);

	nfui_nfobject_show(complete_obj);
	nfui_signal_emit(complete_obj, GDK_EXPOSE, TRUE);

	nfui_nfobject_show(result_obj);
	nfui_signal_emit(result_obj, GDK_EXPOSE, TRUE);

	return 0;
}

static gint _send_diagnosis()
{
	gint i;
	guint chmask = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		chmask |= (1 << i);
	}

	for (i = g_current_step+1; i < SELF_CHECK_MSK_MAX; i++)
	{
		if (g_mask & (1 << i)) 
		{
			g_current_step = i;
			break;
		}
	}

	if (i == SELF_CHECK_MSK_MAX) return -1;

	if (g_current_step == MSK_POWER)
	{
		scm_diagnosis_ipcam_power(chmask, IRET_DIAGNOSIS_POWER);
		g_running_signal = 1;
	}
	else if (g_current_step == MSK_NET)
	{
		scm_diagnosis_ipcam_network(chmask, IRET_DIAGNOSIS_NET);
		g_running_signal = 1;
	}
	else if (g_current_step == MSK_HDD)
	{
		scm_diagnosis_disk(IRET_DIAGNOSIS_HDD);
		g_running_signal = 1;		
	}
	else if (g_current_step == MSK_PORT)
	{
		scm_diagnosis_service_port(IRET_DIAGNOSIS_PORT);
		g_running_signal = 1;		
	}

	return 0;
}

static gboolean _start_diagnosis(gpointer data)
{
	_send_diagnosis();
	_run_waiting_timer();
	g_st_timer = 0;
	return FALSE;
}

static int _wait_for_stop()
{
	if (!g_running_signal) return -1;

	g_stop_wait = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	gtk_main();	
	return 0;
}

static gboolean post_showResult_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		result.chkMask = g_mask;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

        ssm_fakestart_auto_logout();
		VW_System_Self_Result_Open(g_parent, &result);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_end_waiting_timer();

		nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_LOADING_0);
		nfui_nfobject_set_size(g_wait_obj, 89, 89);
		nfui_signal_emit(g_wait_obj, GDK_EXPOSE, TRUE);

		_wait_for_stop();

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if (g_st_timer)
		{
			g_source_remove(g_st_timer);
			g_st_timer = 0;
		}

		_end_waiting_timer();

		g_stop_wait = 0;
		g_curwnd = 0;
		g_current_step = -1;
		g_running_signal = 0;		
		gtk_main_quit();
	}
	else if (evt->type == NFEVENT_KEYPAD_RELEASE || evt->type == NFEVENT_REMOCON_RELEASE)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT)
		{
			_end_waiting_timer();
			_wait_for_stop();
			nfui_nfobject_destroy(obj);

			return TRUE;
		}	
	}


	return FALSE;
}
static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	GdkPixbuf *pbuf = NULL;
	GdkEventKey *kevt;

	KEYPAD_KID kpid = KEYPAD_NONE;

	gint size_w, size_h;
	guint i;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

   		nfui_nfobject_get_size(obj, &size_w, &size_h);
    		pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
    		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == IRET_DIAGNOSIS_POWER)
	{	
		g_running_signal = 0;
	
		if (g_stop_wait)
		{
			nftool_remove_waitbox(g_stop_wait);
			g_stop_wait = 0;
			gtk_main_quit();
			return FALSE;
		}
	
		if (_send_diagnosis() == -1)
	       	{
			_end_process();
		}

		nfui_nfobject_show(chkImg_obj[SSC_POWER]);
		nfui_signal_emit(chkImg_obj[SSC_POWER], GDK_EXPOSE, TRUE);
		
		DIAG_RES_T *res = (DIAG_RES_T*)(((CMM_MESSAGE_T *)data)->data);

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			result.power[i] = res->ipcam_power[i];
		}
	}
	else if(evt->type == IRET_DIAGNOSIS_NET)
	{
		g_running_signal = 0;
	
		if (g_stop_wait)
		{
			nftool_remove_waitbox(g_stop_wait);
			g_stop_wait = 0;
			gtk_main_quit();
			return FALSE;
		}
		
		if (_send_diagnosis() == -1)
	       	{
			_end_process();
		}

		nfui_nfobject_show(chkImg_obj[SSC_NET]);
		nfui_signal_emit(chkImg_obj[SSC_NET], GDK_EXPOSE, TRUE);

		DIAG_RES_T *res = (DIAG_RES_T*)(((CMM_MESSAGE_T *)data)->data);

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			result.net[i] = res->ipcam_net[i];
		}
	}
	else if(evt->type == IRET_DIAGNOSIS_HDD)
	{	
		g_running_signal = 0;
	
		if (g_stop_wait)
		{
			nftool_remove_waitbox(g_stop_wait);
			g_stop_wait = 0;
			gtk_main_quit();
			return FALSE;
		}
		
		if (_send_diagnosis() == -1)
	       	{
			_end_process();
		}

		nfui_nfobject_show(chkImg_obj[SSC_HDD]);
		nfui_signal_emit(chkImg_obj[SSC_HDD], GDK_EXPOSE, TRUE);

		DIAG_RES_T *res = (DIAG_RES_T*)(((CMM_MESSAGE_T *)data)->data);

		for (i = 0; i < IN_DISP_DISK_COUNT; i++)
		{
			result.hdd_in[i] = res->internal_disk[i];
			result.hdd_ex[i] = res->external_disk[i];
		}
	}
	else if(evt->type == IRET_DIAGNOSIS_PORT)
	{
		g_running_signal = 0;
	
		if (g_stop_wait)
		{
			nftool_remove_waitbox(g_stop_wait);
			g_stop_wait = 0;
			gtk_main_quit();
			return FALSE;
		}
		
		if (_send_diagnosis() == -1)
	       	{
			_end_process();
		}

		nfui_nfobject_show(chkImg_obj[SSC_PORT]);
		nfui_signal_emit(chkImg_obj[SSC_PORT], GDK_EXPOSE, TRUE);

		DIAG_RES_T *res = (DIAG_RES_T*)(((CMM_MESSAGE_T *)data)->data);

		result.rtsp_port = res->rtsp_port;
		result.web_port = res->web_port;
	}
	else if (evt->type == GDK_DELETE)
	{
		_end_waiting_timer();

		nfui_nfobject_get_size(obj, &size_w, &size_h);
		nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_POWER);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_NET);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_HDD);
		uxm_unreg_imsg_event(obj, IRET_DIAGNOSIS_PORT);

	}
	return FALSE;
}

static gboolean subwin_returnkey_proc(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	return FALSE;
}

gboolean VW_System_Self_Check_Open(NFWINDOW *parent, SELF_CHECK_MASK_E self_msk)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *tbl;

	GdkPixbuf *pbWaitImage[6];

	guint i;
	gint title_size;
	gint size_w, size_h;
	gint cnt = 0;
	guint pos_y;

	gchar *strLabel[] = {"CAMERA POWER CONSUMPTION",
			"CAMERA NETWORK",
			"HDD",
			"SERVICE PORT"};
	
	guint table_w[] = {SSC_CELL_W, SSC_LABEL_W};
	
	memset(&result, 0x00, sizeof(SELF_RESULT_T));

	g_mask = self_msk;
	g_parent = parent;

	ssm_fakestop_auto_logout();

	title_size = SSC_WIN_SIZE_W -20;

// <---- window
	win = (NFOBJECT*)nfui_nfwindow_new(parent, SSC_WIN_SIZE_X, SSC_WIN_SIZE_Y, SSC_WIN_SIZE_W, SSC_WIN_SIZE_H);
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, subwin_returnkey_proc);
	g_curwnd = win;


// <---- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, SSC_WIN_SIZE_W, SSC_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);
	nfui_nfobject_show(fixed);

// <---- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SELF-DIAGNOSIS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, title_size, SSC_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSC_TITLE_X, SSC_TITLE_Y);

// <----- CHECK LIST TITLE.
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CHECK LIST");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSC_TABLE_X-10, (SSC_TITLE_H + 23));

// <---- CHECK IMAGE / LABEL
	pos_y = (SSC_TITLE_H + SSC_LABEL_H + 40);

	for (i = 0; i < SSC_OBJ_MAX; i++)
	{
		if( i == SSC_POWER )
		{
			if(!(g_mask & (1 << MSK_POWER)))
				continue;
		
			chkImg_obj[i] = nfui_nfimage_new(IMG_SELF_DIAGNOSIS);
			nfui_nfobject_hide(chkImg_obj[i]);
			nfui_nffixed_put((NFFIXED*)fixed, chkImg_obj[i], (SSC_TABLE_X + 30), pos_y);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_set_size(obj, SSC_LABEL_W, SSC_LABEL_H);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, (SSC_TABLE_X + SSC_CELL_W + 32), pos_y);
			
			pos_y += SSC_LABEL_H + 2;
		}
		else if( i == SSC_NET )
		{
			if(!(g_mask & (1 << MSK_NET)))
				continue;

			chkImg_obj[i] = nfui_nfimage_new(IMG_SELF_DIAGNOSIS);
			nfui_nfobject_hide(chkImg_obj[i]);
			nfui_nffixed_put((NFFIXED*)fixed, chkImg_obj[i], (SSC_TABLE_X + 30), pos_y);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_set_size(obj, SSC_LABEL_W, SSC_LABEL_H);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, (SSC_TABLE_X + SSC_CELL_W + 32), pos_y);

			pos_y += SSC_LABEL_H + 2;
		}
		else if( i == SSC_HDD )
		{
			if(!(g_mask & (1 << MSK_HDD)))
				continue;

			chkImg_obj[i] = nfui_nfimage_new(IMG_SELF_DIAGNOSIS);
			nfui_nfobject_hide(chkImg_obj[i]);
			nfui_nffixed_put((NFFIXED*)fixed, chkImg_obj[i], (SSC_TABLE_X + 30), pos_y);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_set_size(obj, SSC_LABEL_W, SSC_LABEL_H);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, (SSC_TABLE_X + SSC_CELL_W + 32), pos_y);

			pos_y += SSC_LABEL_H + 2;
		}
		else	//i == SSC_PORT
		{
			if(!(g_mask & (1 << MSK_PORT)))
				continue;

			chkImg_obj[i] = nfui_nfimage_new(IMG_SELF_DIAGNOSIS);
			nfui_nfobject_hide(chkImg_obj[i]);
			nfui_nffixed_put((NFFIXED*)fixed, chkImg_obj[i], (SSC_TABLE_X + 30), pos_y);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_set_size(obj, SSC_LABEL_W, SSC_LABEL_H);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, (SSC_TABLE_X + SSC_CELL_W + 32), pos_y);

			pos_y += SSC_LABEL_H + 2;
		}
	}

// <---- GEAR IMAGE
	obj = (NFOBJECT*)nfui_nfimage_new(IMG_LOADING_0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (SSC_TABLE_X + SSC_CELL_W +SSC_LABEL_W + 50), (SSC_TITLE_H + SSC_LABEL_H + 90));
	g_wait_obj = obj;

// <---- COMPLETE CHECK 
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Check has been completed.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, SSC_LABEL_W, SSC_LABEL_H);
	nfui_nfobject_hide(obj);
	complete_obj = obj;
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSC_BTN_X-120, SSC_BTN_Y-55);

// <---- RESULT BUTTON
	obj = nftool_normal_button_create_type1("RESULT", SSC_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_hide(obj);
	result_obj = obj;
	nfui_regi_post_event_callback(obj, post_showResult_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSC_BTN_X, SSC_BTN_Y);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", SSC_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	cancel_obj = obj;
	nfui_nffixed_put((NFFIXED*)fixed, obj, SSC_BTN_X, SSC_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_POWER);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_NET);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_HDD);
	uxm_reg_imsg_event(fixed, IRET_DIAGNOSIS_PORT);

	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_POWER);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_NET);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_HDD);
	uxm_monitor_on_imsg_event(fixed, IRET_DIAGNOSIS_PORT);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	g_st_timer = g_timeout_add(1000, _start_diagnosis, 0);

	nfui_page_open(PGID_SELF_CHECK_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_SELF_CHECK_POPUP, win);
}



