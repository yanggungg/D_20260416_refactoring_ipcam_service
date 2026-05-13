#include "nf_afx.h"

#include "nf_api_play.h"
#include "nf_sysman.h"


#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/nf_ui_image.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nftab.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfbutton.h"

#include "vw.h"
#include "vw_playback_main.h"
#include "vw_playback_internal.h"

#include "vw_playback_statusbar.h"
#include "vw_playback_start_menu.h"
#include "vw_playback_shortcut_menu.h"
#include "vw_playback_control_box.h"
#include "vw_arch_play_cbox.h"
#include "vw_preserve_play_cbox.h"

#include "vw_zoom_pip.h"
#include "vw_live_statusbar.h"
#include "vw_timeline.h"
#include "vw_archiving.h"
#include "vw_search_main.h"
#include "vw_playback_arch.h"
#include "vw_playback_control_box_div.h"
#include "vw_playback_control_box_func.h"

#include "tools/nf_ui_function.h"
#include "smt.h"
#include "ssm.h"
#include "evt.h"

#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#elif defined(GUI_16CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)         ((kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16) || (kpid >= KEYPAD_CH17 && kpid <= KEYPAD_CH32))
#endif


////////////////////////////////////////////////////////////
//
// protected data type
//


typedef struct _PLAYBACK_T
{
	NFWINDOW*		curwnd;
	NFOBJECT* 		statusbar;			// playback status bar window
	NFOBJECT* 		startmenu;			// playback start menu window
	NFOBJECT* 		shortcutmenu;		// playback shortcut menu window
	NFOBJECT* 		controlbox;			// playback control box window
	NFOBJECT* 		div_box;			// playback div box window
	NFOBJECT* 		func_box;			// playback func box window

	NFOBJECT* 		arch_play_cbox;		// archived play control box window

	NFOBJECT* 		preserve_play_cbox;		// archived play control box window

	LIVESTART_T		*lst;
	PB_OPEN_BY 		from;

	gboolean		is_arch;
	gint 			osd_status;
	guint 			enable_snap;
	gint			is_again_msg;
	gint			tml_tab_idx;
} PLAYBACK_T;




////////////////////////////////////////////////////////////
//
// private variable
//

static PLAYBACK_T ipb = {0, };

////////////////////////////////////////////////////////////
//
// private interfaces
//


static void _playback_obj_init()
{
	ipb.statusbar = NULL;
	ipb.startmenu = NULL;
	ipb.shortcutmenu = NULL;
	ipb.controlbox = NULL;
	ipb.div_box = NULL;
	ipb.func_box = NULL;
	ipb.is_arch = FALSE;
}


static void _playback_open(NFWINDOW *parent)
{
	_playback_obj_init();

	ipb.shortcutmenu = vw_playback_shortcut_menu_open(parent);
	ipb.controlbox = vw_playback_control_box_open(parent);
	ipb.div_box = vw_playback_control_box_div_open(parent);
	ipb.func_box = vw_playback_control_box_func_open(parent);
	ipb.startmenu = vw_playback_start_menu_open(parent);
	ipb.statusbar = vw_playback_statusbar_open(parent);

	VW_Timeline_ChangeMode(TL_PLAY);
	VW_Timeline_Show();
}

static void _playback_obj_destory()
{
	nfui_nfobject_destroy(ipb.statusbar);
	nfui_nfobject_destroy(ipb.startmenu);
	nfui_nfobject_destroy(ipb.shortcutmenu);
	nfui_nfobject_destroy(ipb.controlbox);
	nfui_nfobject_destroy(ipb.div_box);
	nfui_nfobject_destroy(ipb.func_box);
}

static void _playback_main_destroy()
{
	nfui_nfobject_destroy(ipb.curwnd);
	ipb.curwnd = 0;
}

static void _playback_closed()
{
	vsm_playback_stop();
	_playback_obj_destory();
	_playback_main_destroy();
}


static void _arch_play_obj_init()
{
	ipb.arch_play_cbox = NULL;
}


static void _arch_play_open()
{
	_arch_play_obj_init();

	ipb.arch_play_cbox = vw_arch_play_cbox_open(ipb.curwnd);

	VW_Timeline_Hide();
}

static void _arch_play_obj_destory()
{
	nfui_nfobject_destroy(ipb.arch_play_cbox);
}

static void _arch_play_closed()
{
	vsm_archived_play_stop();
	_arch_play_obj_destory();
	_playback_main_destroy();
}


static void _preserve_play_obj_init()
{
	ipb.preserve_play_cbox = NULL;
	ipb.div_box = NULL;
}

static void _preserve_play_open()
{
	_preserve_play_obj_init();

	ipb.preserve_play_cbox = vw_preserve_play_cbox_open(ipb.curwnd);
	ipb.div_box = vw_playback_control_box_div_open(ipb.curwnd);

	VW_Timeline_Hide();
}

static void _preserve_play_obj_destory()
{
	nfui_nfobject_destroy(ipb.preserve_play_cbox);
	nfui_nfobject_destroy(ipb.div_box);
}

static void _preserve_play_closed()
{
	vsm_archived_play_stop();
	_preserve_play_obj_destory();
	_playback_main_destroy();
}


static gboolean _ask_running_reserved(NFWINDOW *parent, gchar *buf)
{
	mb_type ret;

	ret = nftool_mbox(parent, "WARNING", buf, NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_stop_bookmark();
		scm_exit_bookmark();

		return TRUE;
	}
	else
		return FALSE;
}

static gboolean _enable_step_button()
{
	vw_playback_statusbar_change_stepbutton(1);
	vw_playback_control_box_change_stepbutton(1);

	return TRUE;
}

static gboolean _disable_step_button()
{
	vw_playback_statusbar_change_stepbutton(0);
	vw_playback_control_box_change_stepbutton(0);

	return TRUE;
}

static gboolean _check_pause_playstatus()
{
	DIR_RATE_E play_dir_rate;

	play_dir_rate = vsm_playback_get_dir_rate();

	if ((play_dir_rate == DR_PAUSE) || (play_dir_rate == DR_BWD_NEXT_FRAME)
		|| (play_dir_rate == DR_FWD_NEXT_FRAME))
		return TRUE;

	return FALSE;
}

static gboolean _check_stop_playstatus()
{
	DIR_RATE_E play_dir_rate;

	play_dir_rate = vsm_playback_get_dir_rate();

	if (play_dir_rate == DR_STOP)
		return TRUE;

	return FALSE;
}

static void _send_signal_step_backward()
{
	vsm_playback_step_backward();

	if (_check_stop_playstatus())
		vsm_playback_change_dir_rate(DIR_FWD);
}

static void _send_signal_ds_backward()
{
	vsm_playback_change_dir_rate(DIR_DS_BWD);
}

static void _send_signal_backward()
{
	_enable_step_button();
	vsm_playback_change_dir_rate(DIR_BWD);
}

static void _send_signal_pause()
{
	if (_check_pause_playstatus())
		_enable_step_button();
	else
		_disable_step_button();

	vsm_playback_change_dir_rate(DIR_PAUSE);
}

static void _send_signal_forward()
{
	_enable_step_button();
	vsm_playback_change_dir_rate(DIR_FWD);
}

static void _send_signal_ds_forward()
{
	vsm_playback_change_dir_rate(DIR_DS_FWD);
}

static void _send_signal_step_forward()
{
	vsm_playback_step_forward();

	if (_check_stop_playstatus())
		vsm_playback_change_dir_rate(DIR_FWD);
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	// manual closing only if playback windows
	// because its exit process demands to obey some sequence while escaping
	// but, it is abnormal and it has not hierarchy between windows.
	//
	if(evt->type == WND_PRE_CLOSE)
	{
		if (nfui_nfobject_is_shown(ipb.controlbox)) {
			vw_playback_control_box_exit();
		}
		vw_playback_out_live_open();
	}
	else if (evt->type == NFEVENT_EXIT_PLAYBACK) {
		if (nfui_nfwindow_find("BOOKMARK"))
			evt_send_to_window("BOOKMARK", WND_CLOSE, obj, 0, 0);
		else
			evt_send_to_window("PLAYBACK", WND_CLOSE, 0, 0, 0);
	}
	else if (evt->type == WND_CLOSED) {
		evt_send_to_window("PLAYBACK", WND_CLOSE, 0, 0, 0);
	}

	return FALSE;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_PB_PLAY_STATUS)
	{
        guint tmp_mask = 0;

    	CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;;
        gint ch = pmsg->param;
        gint status = GPOINTER_TO_INT(pmsg->data);;

        if (status == NF_PLAY_STATUS_NONE)
        {
            tmp_mask = (1 << ch);
            ipb.enable_snap |= tmp_mask;
        }
        else
        {
            tmp_mask = (1 << ch);
            ipb.enable_snap &= ~tmp_mask;
        }

		vw_playback_shortcut_set_played(ch, status);
	}
	else if (evt->type == INFY_NOT_SUPPORT_PLAYRATE)
	{
		if (ipb.is_again_msg == 0)
			vw_mbox_check(ipb.curwnd, "NOTICE", IMBX_NOT_SUPPORT_PLAYRATE, IMBX_NOT_SHOW_AGAIN, &ipb.is_again_msg, NFTOOL_MB_OK);
	}
	else if (evt->type == GDK_DELETE)
	{
		if (ipb.tml_tab_idx == 1) 
		{
			VW_Timeline_Tab_Hide();
			VW_Timeline_DeepLearning_Tab_Show();
			VW_Timeline_Hide();
			VW_Timeline_DeepLearning_Show();
		}
		else 
		{
			VW_Timeline_DeepLearning_Tab_Hide();
			VW_Timeline_Tab_Show();
			VW_Timeline_DeepLearning_Hide();
			VW_Timeline_Show();	
		}

    	uxm_unreg_imsg_event(obj, INFY_PB_PLAY_STATUS);
    	uxm_unreg_imsg_event(obj, INFY_NOT_SUPPORT_PLAYRATE);
	}

	return FALSE;
}

////////////////////////////////////////////////////////////
//
// 	public interfaces
//

void vw_playback_open(NFWINDOW *parent, LIVESTART_T *lst, PB_OPEN_BY from)
{
	NFWINDOW *win;

	if (lst == NULL)
	{
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		g_assert(0);
	}

    if (ivsc.dfunc.support_protect)
    {
        if (scm_is_clon_device() == 0) {
            vw_unblock_authcode_popup_open(parent);

			lst->start();
			vsm_destroy_livestart_obj(lst);
			return;
        }
    }
	// SKSHIN, special code

	win = (NFOBJECT*)nfui_nfwindow_new(parent, -1, -1, 0, 0);
	ipb.curwnd = win;
	nfui_nfwindow_set_title(win, "PLAYBACK");
	nfui_regi_pre_event_callback(win, pre_main_wnd_event_handler);
	nfui_regi_pre_event_callback(win, post_main_wnd_event_handler);
	nfui_run_main_event_handler(win);

	uxm_reg_imsg_event(win, INFY_PB_PLAY_STATUS);
    uxm_monitor_on_imsg_event(win, INFY_PB_PLAY_STATUS);
	uxm_reg_imsg_event(win, INFY_NOT_SUPPORT_PLAYRATE);
    uxm_monitor_on_imsg_event(win, INFY_NOT_SUPPORT_PLAYRATE);

	///////////////////////////////////

	if (VW_Live_StatusBar_IsShown())
		VW_Live_StatusBar_Hide();

 	ipb.lst = lst;
	ipb.from = from;
	ipb.osd_status = 0;
	ipb.enable_snap = 0xffffffff;

	if (VW_Timeline_DeepLearning_IsShown()) ipb.tml_tab_idx = 1;
	else ipb.tml_tab_idx = 0;

	if (from != OPEN_BY_DLVA_TL)
	{
		VW_Timeline_DeepLearning_Tab_Hide();
		VW_Timeline_Tab_Show();
		VW_Timeline_DeepLearning_Hide();
		VW_Timeline_Show();	
	}

	if (from == OPEN_BY_ARCH_PLAY)
	{
		_arch_play_open();
		smt_set_service(SMT_ARCH_PLAY);
	}
	else if (from == OPEN_BY_PRESERVE_PLAY)
	{
		g_message("aaaaaaaaaaaaaaaaaaaaaaaa");
		_preserve_play_open();
		smt_set_service(SMT_ARCH_PLAY);
	}
	else
	{
		_playback_open(ipb.curwnd);
		smt_set_service(SMT_PLAYBACK);
	}

	vsm_shuttle_init();
}

void vw_playback_out_search_open(void)
{
	NFOBJECT *topwin;
	LIVESTART_T *temp;

	if(ipb.is_arch)
	{
		if (!_ask_running_reserved(ipb.curwnd, "Archiving is in progress.\nDo you want to stop it\nand exit search menu?"))
			return;
	}

//	VW_Timeline_Hide();
	VW_Timeline_ChangeMode(TL_LIVE);

	_playback_closed();

	if (ipb.from == OPEN_BY_SEARCH)
	{
		VW_Search_Show(-1, ipb.lst);
		ipb.lst = NULL;
	}
	else
	{
		time_t cur_time;

		cur_time = stm_get_time_t();
		stm_set_time_t(cur_time-300);
		stm_set_endtime_t(cur_time);

		if (ipb.from == OPEN_BY_ARCH_DF)
			VW_Archiving_Close();

		temp = ipb.lst;
		ipb.lst = NULL;
		VW_Search_Open(ipb.curwnd, 0, temp);
	}

//	smt_set_service(SMT_SEARCH);
}

void vw_playback_out_arch_open(void)
{
	NFOBJECT *topwin;
	LIVESTART_T *temp;

	if(ipb.is_arch)
	{
		if (!_ask_running_reserved(ipb.curwnd, "Archiving is in progress.\nDo you want to stop it\nand exit archiving menu?"))
			return;
	}

//	VW_Timeline_Hide();
	VW_Timeline_ChangeMode(TL_LIVE);

	_playback_closed();

	if (ipb.from == OPEN_BY_ARCH_DF)
	{
		vw_archiving_show(ipb.lst);
		ipb.lst = NULL;
	}
	else
	{
		if (ipb.from == OPEN_BY_SEARCH)
			VW_Search_Destroy();

		temp = ipb.lst;
		ipb.lst = NULL;
		VW_Archiving_Open(NF_TOPWND, temp, 1);
	}

//	smt_set_service(SMT_ARCHIVE);
}

void vw_playback_out_live_open(void)
{
	NFOBJECT *topwin;

	if(ipb.is_arch)
	{
		if (!_ask_running_reserved(ipb.curwnd, "Archiving is in progress.\nDo you want to stop it\nand exit live?"))
			return;
	}

	_playback_closed();

	if (ipb.from == OPEN_BY_ARCH_DF)
		VW_Archiving_Close();
	else if (ipb.from == OPEN_BY_SEARCH)
		VW_Search_Destroy();

	if (ipb.lst)
	{
		ipb.lst->start();
		vsm_destroy_livestart_obj(ipb.lst);
	}

	smt_set_service(SMT_LIVE);
}

void vw_playback_out_opened(void)
{
	if (ipb.from == OPEN_BY_SEARCH)
		vw_playback_out_search_open();
	else if (ipb.from == OPEN_BY_ARCH_DF)
		vw_playback_out_arch_open();
	else
		vw_playback_out_live_open();
}

void vw_arch_play_out(void)
{
	_arch_play_closed();

	vw_archiving_show(ipb.lst);
	ipb.lst = NULL;

	smt_set_service(SMT_ARCHIVE);
}

void vw_preserve_play_out(void)
{
	_preserve_play_closed();

	vw_archiving_show(ipb.lst);
	ipb.lst = NULL;

	smt_set_service(SMT_ARCHIVE);
}

void vw_playback_out_SearchPage_open(gint page)
{
	NFOBJECT *topwin;
	LIVESTART_T *temp;

	if(ipb.is_arch)
	{
		if (!_ask_running_reserved(ipb.curwnd, "Archiving is in progress.\nDo you want to stop it\nand exit search menu?"))
			return;
	}

//	VW_Timeline_Hide();
	VW_Timeline_ChangeMode(TL_LIVE);

	_playback_closed();

	if (ipb.from == OPEN_BY_SEARCH)
	{
		VW_Search_Show(2, ipb.lst);
		ipb.lst = NULL;
	}
	else
	{
		time_t cur_time;

		cur_time = stm_get_time_t();
		stm_set_time_t(cur_time-300);
		stm_set_endtime_t(cur_time);

		if (ipb.from == OPEN_BY_ARCH_DF)
			VW_Archiving_Close();

		temp = ipb.lst;
		ipb.lst = NULL;
		VW_Search_Open(ipb.curwnd, page, temp);
	}

//	smt_set_service(SMT_SEARCH);
}

// playback handler.
gboolean pb_div1_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_change_sfc_cstlayout_next(VSM_DIV1);
	}	
	return FALSE;
}

gboolean pb_div4_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_change_sfc_cstlayout_next(VSM_DIV4);
	}	
	return FALSE;
}

gboolean pb_div9_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_change_sfc_cstlayout_next(VSM_DIV9);
	}	
	return FALSE;
}

gboolean pb_div16_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_change_sfc_cstlayout_next(VSM_DIV16);
	}	
	return FALSE;
}

gboolean pb_div36_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vsm_change_sfc_cstlayout_next(VSM_DIV36);
	}	
	return FALSE;
}

gboolean pb_osd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(ipb.osd_status ^= 1)
		{
			vsm_osd_off();

			if (ipb.statusbar) vw_playback_statusbar_change_osd_img(1);
			if (ipb.div_box) vw_playback_control_box_change_osd_img(1);
		}
		else
		{
			vsm_osd_on();
			if (ipb.statusbar) vw_playback_statusbar_change_osd_img(0);
			if (ipb.div_box) vw_playback_control_box_change_osd_img(0);
		}
	}
	return FALSE;
}

gboolean pb_play_step_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static guint tid = 0;

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_step_backward();
	}

	return FALSE;
}

gboolean pb_play_ds_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_ds_backward();
	}
	return FALSE;
}

gboolean pb_play_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_backward();
	}
	return FALSE;
}

gboolean pb_play_pause_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_pause();
	}
	return FALSE;
}

gboolean pb_play_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_forward();
	}
	return FALSE;
}

gboolean pb_play_ds_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_ds_forward();
	}
	return FALSE;
}

gboolean pb_play_step_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static guint tid = 0;

	if (evt->type == GDK_BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		_send_signal_step_forward();
	}

	return FALSE;
}

void vw_playback_multi_zoom_func()
{


}

void vw_playback_full_zoom_func()
{
	guint ch;
	gint x, y;

	ch = vsm_get_focused_channel();

    if(vsm_get_covert_state(NULL, ch)) {
    	nftool_mbox(ipb.curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
    	return;
    }

	x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
	y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();

	VW_ZoomPIP_Open(ipb.curwnd, x, y, ch, ZOOM_PIP_NONE);
	_send_signal_forward();
}

gboolean vw_playback_snap_func(gint ch)
{
	gint retVal;

	if(!DAL_get_support_snapshot()) return FALSE;

    if(vsm_get_covert_state(NULL, ch)) {
    	nftool_mbox(ipb.curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
    	return FALSE;
    }

    if (ipb.enable_snap & (1 << ch))
    {
        vsm_playback_play_pause_by_menu_opened();
        retVal = scm_req_playback_capture(INFY_CAPTURE_IMAGE, ch);
		if (retVal != 0) {
			nftool_mbox(ipb.curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
			return FALSE;
		}
        return TRUE;
    }

    nftool_mbox(ipb.curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
    return FALSE;
}

gboolean vw_playback_notime_snap_msg_func(gint ch, IMSG ret_msg)
{
	gint retVal;

	if(!DAL_get_support_snapshot()) return FALSE;

    if(vsm_get_covert_state(NULL, ch)) {
    	nftool_mbox(ipb.curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
    	return FALSE;
    }

    if (ipb.enable_snap & (1 << ch))
    {
        vsm_playback_play_pause_by_menu_opened();
        retVal = scm_req_playback_capture_without_time(ret_msg, ch);
		if (retVal != 0) {
			nftool_mbox(ipb.curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
			return FALSE;
		}
        return TRUE;
    }

    nftool_mbox(ipb.curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
    return FALSE;
}

gboolean vw_playback_archive_func()
{
	guint ret;
	gboolean active;
	gboolean ispaused = FALSE;
	static int arch_wnd = 0;

	if (arch_wnd) return FALSE;

	vsm_playback_play_pause_by_menu_opened();

	vw_playback_statusbar_change_bookmark_img(0);
	vw_playback_control_box_change_bookmark_img(0);

	if(!ipb.is_arch)
	{
		arch_wnd = 1;
		ret = VW_PlaybackArch_Open(ipb.curwnd, START);
		arch_wnd = 0;

		if(ret == PA_RET_CLOSE)
			ipb.is_arch = FALSE;
		else
			ipb.is_arch = TRUE;
	}
	else
	{
		arch_wnd = 1;
		ret = VW_PlaybackArch_Open(ipb.curwnd, STOP);
		arch_wnd = 0;

		if(ret == PA_RET_CONTINUE)
			ipb.is_arch = TRUE;
		else
			ipb.is_arch = FALSE;
	}

	vsm_playback_play_recover_by_menu_closed();

	if (ipb.is_arch)
	{
		vw_playback_shortcut_change_bookmark_text(1);
		vw_playback_statusbar_change_bookmark_img(1);
		vw_playback_control_box_change_bookmark_img(1);

        vw_playback_shortcut_change_snap(0);
		vw_playback_statusbar_change_snap(0);
		vw_playback_control_box_change_snap(0);
		smt_set_service(SMT_PLAYBACK_BOOKMARK);
	}
	else
	{
		vw_playback_shortcut_change_bookmark_text(0);
		vw_playback_statusbar_change_bookmark_img(0);
		vw_playback_control_box_change_bookmark_img(0);

        vw_playback_shortcut_change_snap(1);
		vw_playback_statusbar_change_snap(1);
		vw_playback_control_box_change_snap(1);

		smt_set_service(SMT_PLAYBACK);
	}

	return ipb.is_arch;
}

gboolean vw_playback_process_common_KeyEvent(KEYPAD_KID kpid)
{
	if (GET_CHANNEL_KEY_SCOPE(kpid) || (kpid == KEYPAD_DISP) || (kpid == KEYPAD_HOLD))
		vsm_keypad_event(kpid);
	else
	{
		switch(kpid) {
			case RMC_BJUMP:
				if (!_check_pause_playstatus())
				_send_signal_step_backward();
			break;
			case KEYPAD_REW:
				_send_signal_ds_backward();
			break;
			case KEYPAD_BACKWARD:
				_send_signal_backward();
			break;
			case KEYPAD_PAUSE:
			{
				if (_check_pause_playstatus())
				{
					_enable_step_button();
					vsm_playback_change_dir_rate(DIR_FWD);
				}
				else
				{
					_disable_step_button();
					vsm_playback_change_dir_rate(DIR_PAUSE);
				}
			}
			break;
			case KEYPAD_FORWARD:
				_send_signal_forward();
			break;
			case KEYPAD_FF:
				_send_signal_ds_forward();
			break;
			case RMC_FJUMP:
				if (!_check_pause_playstatus())
				_send_signal_step_forward();
			break;
			default:
			break;
		}
	}

	return TRUE;
}

gboolean vw_playback_process_common_JogEvent(JOGID jog_id)
{
	vsm_jog_event(jog_id);
}

gboolean vw_playback_process_common_ShuttleEvent(SHUTTLEID shuttle_id)
{
	vsm_shuttle_event(shuttle_id);
}

NFOBJECT *vw_playback_get_control_box()
{
	return ipb.controlbox;
}

NFOBJECT *vw_playback_get_div_box()
{
	return ipb.div_box;
}

NFOBJECT *vw_playback_get_func_box()
{
	return ipb.func_box;
}


