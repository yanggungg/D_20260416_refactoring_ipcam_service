/*
 * uxm.c
 * 	- dependency :
 *
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */


#include <glib.h>
#include "cmm.h"
#include "iux_afx.h"
#include "nfdal.h"
#include <assert.h>
#include "../viewers/objects/nfobject.h"
#include "../viewers/vw_userpwd.h"
#include "../viewers/vw_calendar.h"
#include "../viewers/vw_alarm_name_popup.h"
#include "../viewers/vw_vwnd.h"
#include "../viewers/vw_boot_info.h"
#include "../viewers/vw_timeline.h"
#include "../viewers/vw_live_statusbar.h"
#include "../viewers/vw_live_start_menu.h"
#include "../viewers/vw_live_shortcut_menu.h"
#include "../viewers/vw_playback_shortcut_menu.h"
#include "../viewers/vw_fac_init.h"
#include "../viewers/vw_net_fw_up.h"
#include "../viewers/vw_default_value_settings.h"
#include "../support/multi_language_support.h"
#include "vw_live_event_log.h"
#include "vw_live_audio_input.h"
#include "vw_live_audio_output.h"
#include "vw_live_alarm_status.h"
#include "vw_live_disk_status.h"
#include "vw_live_div_popup.h"
#include "vw_live_net_status.h"
#include "vw_live_net_ipcam_popup.h"
#include "uxm.h"
#include "vsm.h"
#include "ssm.h"
#include "wnd.h"
#include "evt.h"
#include "dvaa.h"
#include "ix_mem.h"
#include "ix_conf.h"
#include "smt.h"
#include "../tools/nf_ui_function.h"
#include "../tools/nf_ui_tool.h"
#include "vw.h"
#include "nf_sysman.h"

#if defined(CONFIG_FWUPGRADE_SINGLE)
	#include "nf_util_fw_single.h"
#endif

#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#endif

DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"UXM"

#define ENABLE_WAIT_WINDOW

#define UXM_LOCK()		g_mutex_lock(iuxm.mtx)
#define UXM_UNLOCK()	g_mutex_unlock(iuxm.mtx)

#define GET_MSGIDX(m)	(m & (~VWMSG_MASK))

#define MINFO_SLOT_SIZE	16

////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct _GEAR{
	gint t_id;
	guint vloss_mask;
	guint draw_mask;
} GEAR;

#define	BS_READY				0
#define	BS_BOOTWND_OPEN			1
#define	BS_SCENARIO_CMPL		2
#define	BS_BOOTWND_CLOSE		3
#define BS_ENTER_LIVE			4
#define TZ_SHANGHAI             27
#define TZ_KOREA                29
#define TZ_ENGLISH               7

typedef struct _UXM_T {
	GMutex			*mtx;
	GtkWidget		*wuxm;
	int				service_timer;
	CALLID			callid;
	int				boot_step;

	NFOBJECT		*wnd_ats;
	GEAR            gear;
} UXM_T;

typedef struct _MSGT_INFO {
	NFOBJECT 	*obj[MINFO_SLOT_SIZE];
	int			monitor[MINFO_SLOT_SIZE];
} MSGT_INFO;

typedef struct _MSGT {
	MSGT_INFO	minfo[256];
} MSGT;

#ifdef ENABLE_WAIT_WINDOW
/** extern function definition **/
extern int proxy_system(const char *str, int mode, int timeout_sec);
#endif

////////////////////////////////////////////////////////////
//
// private variables
//

static MSGT	imsgt[4];		// 4 categories
static UXM_T iuxm;

static guint login_tid = 0;

////////////////////////////////////////////////////////////
//
// private interfaces
//

#if 0
static int _free_nfnotify_data(CMM_MESSAGE_T *pmsg)
{
	NF_NOTIFY_INFO *src;

	switch(pmsg->msgid) {
		case INFY_CAM_TITLE_NOTIFY:
		case INFY_NOVIDEO_NOTIFY:
		case INFY_NET_NOTIFY:
		case INFY_ENC_NOTIFY:
		case INFY_DISK_USAGE_NOTIFY:
		case INFY_DISK_FULL_NOTIFY:
		case INFY_DISK_OW_NOTIFY:
		case INFY_SYSDB_CHANGE_NOTIFY:
		case INFY_NETDB_CHANGE_NOTIFY:
		case INFY_AUDDB_CHANGE_NOTIFY:
		case INFY_DSKDB_CHANGE_NOTIFY:
		case INFY_CAMDB_CHANGE_NOTIFY:
		case INFY_USRDB_CHANGE_NOTIFY:
		case INFY_ALMDB_CHANGE_NOTIFY:
		case INFY_ACTDB_CHANGE_NOTIFY:
		case INFY_DSPDB_CHANGE_NOTIFY:
		case INFY_RECDB_CHANGE_NOTIFY:
		case INFY_COVERT_NOTIFY:
		case INFY_SMART_ERROR_NOTIFY:
		case INFY_ANALOG_REC_NOTIFY:
		case INFY_IPCAM_REC_NOTIFY:
		case INFY_VLOSS_NOTIFY:
		case INFY_SENSOR_NOTIFY:
		case INFY_MOTION_NOTIFY:
		case INFY_ALARM_NOTIFY:
		case INFY_PND_NOTIFY:
		case INFY_PND_RATE_NOTIFY:
		case INFY_PND_HUB_NOTIFY:
		case INFY_WAN_NOTIFY:
		case INFY_DDNS_NOTIFY:
		case INFY_WRITEFAIL_NOTIFY:
		case INFY_EXHAUST_NOTIFY:
		case INFY_NODISK_NOTIFY:
		case INFY_SMART_WARN_NOTIFY:
		case INFY_SYSFAN_NOTIFY:
		case INFY_TEMPERATURE_NOTIFY:
		case INFY_POE_NOTIFY:
    	case INFY_POE_HUB_NOTIFY:
		case INFY_POE_PORT_NOTIFY:
		case INFY_DVRLOGINFAIL_NOTIFY:
		case INFY_NETLOGINFAIL_NOTIFY:
		case INFY_NET_RXTX:
		case INFY_AUDIOCH_NOTIFY:
		case INFY_MICOUT_NOTIFY:
		case INFY_BUZZER_NOTIFY:
		case INFY_IP_CHANGED_NOTIFY:
		case INFY_VCA_EVENT_NOTIFY:
		case INFY_VCA_TRACKINFO_NOTIFY:
		case INFY_VCA_COUNTER_NOTIFY:
		case INFY_VCA_META_DATA_NOTIFY:
			src = (NF_NOTIFY_INFO *)pmsg->data;
			if (src->type == NF_NOTIFY_POINTER) ifree(src->p.ptr);
			break;
		default:
			break;
	}

	return 0;
}
#endif

static int _is_nf_notify(CMM_MESSAGE_T *pmsg)
{
	return (pmsg->msgid >= INFY_NFNOTIFY_MIN) && (pmsg->msgid <= INFY_NFNOTIFY_MAX);
}

static int _free_nfnotify_data(CMM_MESSAGE_T *pmsg)
{
    NF_NOTIFY_INFO *src;

	if (_is_nf_notify(pmsg)) {
		src = (NF_NOTIFY_INFO *)pmsg->data;
		if (src->type == NF_NOTIFY_POINTER) ifree(src->p.ptr);
	}

	return 0;
}

static int _free_captrue_data(CMM_MESSAGE_T *pmsg)
{
    gint result = pmsg->param;
    CAPTURE_IMAGE_T *image = pmsg->data;

    if (result != 0) return -1;

    DMSG(1, "");
    free(image->buffer);
    DMSG(1, "");
	return 0;
}

static int _free_aibox_analytics_additional_event(CMM_MESSAGE_T *pmsg)
{
    ANALYTICS_ADDITIONAL_EVENT_T *event = pmsg->data;

	if (!event) return -1;

    if (event->jpeg_buff) {
		ifree(event->jpeg_buff);		
	}
	return 0;
}

static int _get_msg_category(int msgid)
{
	int i;
	int cat = (msgid & VWMSG_MASK) >> 12;
    for (i = 0; i < 4 && !(cat & (1 << i)); ++i);
	return i;
}

static int _check_message(int cat, int msgidx, NFOBJECT *obj)
{
	int i;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		if (imsgt[cat].minfo[msgidx].obj[i] == obj) {
			DMSG(1, "WARNING: The message is registered as the same window already");
			return -1;
		}
	}
	return 0;
}

static int _register_message(int cat, int msgidx, NFOBJECT *obj)
{
	int i;

	if (_check_message(cat, msgidx, obj) < 0) return 0;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		if (imsgt[cat].minfo[msgidx].obj[i] == 0) break;
	}

	if (i >= MINFO_SLOT_SIZE) {
		DMSG(1, "category = 0x%08x, msgidx = 0x%08x\n", cat, msgidx);
		g_assert(i < MINFO_SLOT_SIZE);
	}
	imsgt[cat].minfo[msgidx].obj[i] = obj;
	imsgt[cat].minfo[msgidx].monitor[i] = 0;
	return 0;
}

static int _unregister_message(int cat, int msgidx, NFOBJECT *obj)
{
	int i;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		if (imsgt[cat].minfo[msgidx].obj[i] == obj) {
			imsgt[cat].minfo[msgidx].obj[i] = 0;
			imsgt[cat].minfo[msgidx].monitor[i] = 0;
			break;
		}
	}
	return 0;
}

static int _monitor_on_message(int cat, int msgidx, NFOBJECT *obj)
{
	int i;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		if (imsgt[cat].minfo[msgidx].obj[i] == obj) {
			imsgt[cat].minfo[msgidx].monitor[i] = 1;
			break;
		}
	}
	return 0;
}

static int _monitor_off_message(int cat, int msgidx, NFOBJECT *obj)
{
	int i;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		if (imsgt[cat].minfo[msgidx].obj[i] == obj) {
			imsgt[cat].minfo[msgidx].monitor[i] = 0;
			break;
		}
	}
	return 0;
}

static int _emit_signal(IMSG msg, gpointer data)
{
	int cat = _get_msg_category(msg);
	int msgidx = GET_MSGIDX(msg);
	int i;
	NFOBJECT *obj;
	int mon;

	for (i = 0; i < MINFO_SLOT_SIZE; ++i) {
		obj = imsgt[cat].minfo[msgidx].obj[i];
		mon = imsgt[cat].minfo[msgidx].monitor[i];
		if (obj != 0) {
			DMSG(9, "OBJ = [%p], MONITOR = [%d]\n", obj, mon);
			if (nfui_nfobject_is_shown(obj) || mon)
				nfui_signal_emit_data(obj, msg, FALSE, data);
		}
	}
	return 0;
}

static int _emit_nfevt(gpointer data)
{
	CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
	CMM_MESSAGE_T *pevt = (CMM_MESSAGE_T *)pmsg->data;
	NFOBJECT *dst = pmsg->param;
	IMSG msgid = pevt->msgid;

	DMSG(1, "%p, 0x%08x, %p", dst, msgid, pevt);
	nfui_signal_emit_data(dst, msgid, FALSE, pevt);
	if (pevt->dyn_data && pevt->data) ifree(pevt->data);

	return 0;
}

static int _init_language()
{
	OsdData osddata;
	DAL_get_osd_data(&osddata);

	DMSG(1, "LANGUAGE = [%s]\n", osddata.lang);
	if( 0 != init_multi_language_support(osddata.lang) )	{
		DMSG(1, "\n\n\ninit_multi_language_support error \n\n\n");
	}
	else {
		DMSG(1, "\n\n\ninit_multi_language_support success \n\n\n");
	}

	return 0;
}

static int _init_input_devices()
{
	nfui_keypad_init();
	nfui_remocon_init();
	nfui_mouse_init();
//	nfui_jog_init();
//	nfui_shuttle_init();
	nfui_485_init();

	return 0;
}

static int _init_vloss()
{
	iuxm.gear.vloss_mask = 0xffff;
	return 0;
}

static int _leave_from_live()
{
	char user[64];

	memset(user, 0x00, sizeof(user));
	ssm_get_cur_id(user);

	if (strlen(user)) scm_put_log(CLOSE_LIVE, 0, 0);

	smt_set_service(SMT_LOGOUT);
	ssm_init_auth();
	VW_Live_StatusBar_Menu_Disable();
	VW_Live_StatusBar_Set_User("");
	LiveStart_Popup_Menu_Disable();
	VW_EventLog_Hide();
	VW_AudioInput_Hide();
	VW_AudioOutput_Hide();
	VW_AlarmStatus_Hide();
	VW_NetStatus_Hide();
	VW_Live_Net_IPCam_Popup_Hide();
	VW_Destroy_DiskStatus();
	VW_Destroy_Div_Popup();
	LiveStart_Popup_Hide();
	VW_ShortCut_Menu_Hide();
	VW_ZoomPIP_Destroy();
	VW_Live_Ptz_Main_Destroy();
	vw_playback_shortcut_menu_hide();
	VW_Close_account_change_Popup();
	vsm_set_covert_by_logout();
    VW_Timeline_PopUp_Hide();
    VW_Hide_Search_SubMenu();
    VW_Authen_Info_Setup_close();
    VW_Hide_Image_Popup();
    VW_QR_code_Close();
    VW_System_Self_Start_Close();
    VW_System_Self_Result_Close();
	VW_destroy_live_ptz_ctrl();
	VW_hide_Set_DateTime();
    
	return 0;
}

static int _close_bootwnd()
{
	// FIXME: api
/*	if(!nf_sysman_is_normal_boot())
		scm_put_log(ABNORMAL_SHUTDOWN, 0, 0);

	scm_put_log(SYSTEM_STARTED, 0, 0);*/
	smt_set_service(SMT_LOGOUT);

	vsm_start();
	VW_Destroy_BootInfo(IRET_BOOTWND_CLOSED);

	_vvm_unset_clear_vwnd();
	vwnd_repaint();

	return 0;
}

static int _on_new_user(CMM_MESSAGE_T *pmsg)
{
	char *id = (char *)pmsg->data;
	DMSG(1, "NEW USER LOGON = [%s]\n", id);
	if (id)	ssm_set_new_user(id);

	return 0;
}

static int _open_default_windows()
{
	VW_Timeline_Open(NF_TOPWND);
	if (ivsc.dfunc.support_dl_timeline) {
		VW_Timeline_DeepLearning_Open(NF_TOPWND);
	}
	VW_Live_StatusBar_Open(NF_TOPWND);
	VW_Create_ShortCut_Menu(NF_TOPWND);
	VW_Create_OSD_Popup(NF_TOPWND);
	VW_Net_FWUpgrade_Open(NF_TOPWND);
	return 0;
}

static int _start_wizard()
{
	int wizard_func;
	WizardCheck wizard_data;

    if(scm_is_qc_mode() == 0)   return 0;

    memset(&wizard_data, 0x00, sizeof(WizardCheck));
    wizard_func = 0;

    DAL_get_Netwizard_func(&wizard_func);
	DAL_get_WizardCheck_Data(&wizard_data, 0);

    if ((wizard_func == 1) && (wizard_data.netwiz)) {
        vw_wizard_init(NF_TOPWND, 0);
    }


    return 0;
}

static int _enter_live()
{
	DMSG(9, "");
	_open_default_windows();
	VW_Timeline_Hide();
	VW_Live_StatusBar_Hide();

	scm_init_timesync();
	evt_send_to_local(INFY_INIT_LIVEVIEW, 0, 0, 0);
	return 0;
}

static int _check_abnormal_shutdown(gint *usr_idx)
{
	FILE *fp;

#if defined(_SUPPORT_ABNORMAL_LOGIN)
	fp = fopen("./data/gui/login_user.itx", "r");
	if(fp){
		if (fscanf(fp, "%d", usr_idx) != 1)
		{
			g_warning("%s data read error!!!", __FUNCTION__);
			fclose(fp);
			return 0;
		}

		fclose(fp);
	}

	if (*usr_idx != -1)
		return 1;
#endif

	return 0;
}

static int _login_user(gint usr_idx)
{
	UserManageData userdata;
	gchar id[16];
	gchar passwd[16+1];
	gchar *idbuf;
	gint max_cnt;

	max_cnt = DAL_get_user_count();

	if (usr_idx >= max_cnt) return 0;

	DAL_get_user_id(id, usr_idx);
	DAL_get_user_passwd(passwd, usr_idx);

	if(ssm_logon(id, passwd)) {

		VW_Live_StatusBar_Menu_Enable();
		VW_Live_StatusBar_Set_User(id);

		LiveStart_Popup_Menu_Enable();

		scm_put_log(OPEN_LIVE, 0, 0);

		vsm_set_covert_by_user(id);

		idbuf = imalloc(sizeof(char) * 32);
		strcpy(idbuf, id);
		evt_send_to_local(INFY_USER_LOGON, 0, 1, idbuf);

    	smt_set_service(SMT_LIVE);

    	return 1;
	}

	return 0;
}

static int _auto_login()
{
	gchar id[16];
	gchar passwd[16+1];
	gchar *idbuf;
	gint max_cnt, usr_idx = 0;

	max_cnt = DAL_get_user_count();

	if (usr_idx >= max_cnt) return 0;

	DAL_get_user_id(id, usr_idx);
	DAL_get_user_passwd(passwd, usr_idx);

	if(ssm_logon(id, passwd)) {
    	VW_Live_StatusBar_Menu_Enable();
    	VW_Live_StatusBar_Set_User(id);

    	LiveStart_Popup_Menu_Enable();

    	scm_put_log(OPEN_LIVE, 0, 0);

    	vsm_set_covert_by_user(id);

    	idbuf = imalloc(sizeof(char) * 32);
    	strcpy(idbuf, id);
    	evt_send_to_local(INFY_USER_LOGON, 0, 1, idbuf);

    	smt_set_service(SMT_LIVE);
	}

    return 0;
}

static int _open_login()
{
	gint usr_idx = -1;
	gint res;

	if (_check_abnormal_shutdown(&usr_idx))
	{
		if (!_login_user(usr_idx))
		{
			if (VW_UserPwd_Open(NF_TOPWND, "LOG IN", -1))
                smt_set_service(SMT_LIVE);
        }
	}
	else if(scm_is_qc_mode() == 0)
	{
	    _auto_login();
	    VW_Timeline_Show();
	    VW_Live_StatusBar_Show();
	    vw_qc_test_show_new();

	    return 0;
	}
	else
	{
		if (VW_UserPwd_Open(NF_TOPWND, "LOG IN", -1))
            smt_set_service(SMT_LIVE);
    }

	if(VW_Timeline_get_disp_mode() == TLINE_ALWAYS_ON)
		VW_Timeline_Show();

	// not always mode
	if(VW_Live_StatusBar_On_Time() == 0)
		VW_Live_StatusBar_Show();

	return 0;
}

static int _put_fwupgrade_doneinfo()
{
	if (DAL_get_fwup_state() == 0) return -1;

	g_message("%s, %d, _fwup_first_boot", __FUNCTION__, __LINE__);

	if (DAL_get_fwup_state() == 1) {
		time_t uptime = time(0);
		DAL_set_fwup_date(uptime);
		scm_put_log(FW_UPGRADE_SUCC, 0, 0);
	}
	else if (DAL_get_fwup_state() == -1) {
		scm_put_log(FW_UPGRADE_FAIL, 0, 0);
	}

	DAL_set_fwup_state(0);
	DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

	return 0;
}

static int _check_net_upgrade_result()
{
	gint res;

#if defined(CONFIG_FWUPGRADE_SINGLE)
	res = nf_fw_update_check();

	if (res == NF_FWUP_STATUS_UPGRADE_RAM_DONE || res == NF_FWUP_STATUS_UPGRADE_RAM_FAIL) {
		evt_send_to_local(IREQ_SCM_128MB_FWUP_RAMDISK_RESULT, res, 0, 0);
		return 0;
    }

	if (res == NF_FWUP_STATUS_UPGRADE_NET_DONE || res == NF_FWUP_STATUS_UPGRADE_NET_FAIL) {
		evt_send_to_local(IREQ_SCM_128MB_FWUP_RESULT_BY_WEB, res, 0, 0);
		return 0;
    }
#endif

	var_set_enable_remote_upgrade(1);
	evt_send_to_local(INFY_CHECK_REMOTE_NEWFW, 0, 0, 0);
	return 0;
}

static gboolean close_notice(gpointer data)
{
	NFOBJECT *obj;

	NFUTIL_THREADS_ENTER();

	obj = (NFOBJECT*)data;
	nftool_remove_waitbox(obj);

	gtk_main_quit();

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static int _notice_resolution()
{
	NFOBJECT *mbox;
	gchar strRes[32];
	gchar buf[1024];
	gchar *msg = "This monitor does not support 2160p(3840x2160) resolution.\nIt is recommended that you use a 2160p(3840x2160) monitor for optimal display.\n(Current screen Resolution : %s Hz)";

	if(!scm_get_video_resolution(strRes)) return -1;
	g_message("%s, %d, %s", __FUNCTION__, __LINE__, strRes);

	if(!strstr(strRes, "1080p") && !strstr(strRes, "3840x2160"))
	{
		memset(buf, 0x00, sizeof(buf));
		g_sprintf(buf, lookup_string(msg), strRes);
		mbox = nftool_mbox_wait(NF_TOPWND, "NOTICE", buf);
		g_timeout_add(5000, close_notice, mbox);
		gtk_main();
	}

	return 0;
}

#define INTERNAL_DISK_CNT 16
static int _check_prev_unc_error()
{
	int unc_error;
	char buf[1024];

	unc_error = scm_get_prev_unc_error();
	if (unc_error) {
		if ( (unc_error >> 26) < INTERNAL_DISK_CNT )
		{
			sprintf(buf,
					lookup_string("System detected a bad sector\non the %s disk %d,\nand it has been recovered.\n\n[ 0x%08X ]"),
					lookup_string("internal"), (unc_error >> 26) + 1, unc_error);
		}
		else
		{
			sprintf(buf,
					lookup_string("System detected a bad sector\non the %s disk %d,\nand it has been recovered.\n\n[ 0x%08X ]"),
					lookup_string("external"), (unc_error >> 26) - INTERNAL_DISK_CNT+1, unc_error);
		}
		nftool_mbox_auto_ok(NF_TOPWND, 30, "UNC ERROR", buf);
	}

	return 0;
}

static gboolean draw_gear(gpointer data)
{
	guint i;

	for( i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	{
		if(iuxm.gear.draw_mask & (1 << i))
			vvm_gear_draw(i);
	}

	return TRUE;
}

static gint start_gear(gint ch)
{
    if (iuxm.gear.draw_mask & (1 << ch))
        return -1;

    if (iuxm.gear.t_id)
    {
		g_source_remove(iuxm.gear.t_id);
		iuxm.gear.t_id = 0;
    }

	iuxm.gear.draw_mask |= (1 << ch);
	iuxm.gear.t_id = g_timeout_add(500, draw_gear, NULL);

	return 0;
}

static gint stop_gear(gint ch)
{
	if (!(iuxm.gear.draw_mask & (1 << ch)))
        return -1;

    if (iuxm.gear.t_id)
    {
		g_source_remove(iuxm.gear.t_id);
		iuxm.gear.t_id = 0;
    }

	iuxm.gear.draw_mask &= ~(1 << ch);
    vvm_gear_delete(ch);

    if (iuxm.gear.draw_mask)
    	iuxm.gear.t_id = g_timeout_add(500, draw_gear, NULL);

	return 0;
}

static gint _check_pndrate(NF_NOTIFY_INFO *data)
{
	guint ch;
	if(data) {
		ch = data->d.params[1];
		start_gear(ch);
	}

	return 0;
}

static gint _check_pnd(NF_NOTIFY_INFO *data)
{
	guint ch;

	if(data) {
		ch = data->d.params[1];

		if( (data->d.params[0] == PND_TYPE_UNPLUGGED)  ||
			(data->d.params[0] == PND_TYPE_UNKNOWN)  ||
			(data->d.params[0] == PND_TYPE_UNSUPPORTED)  ||
			(data->d.params[0] == PND_TYPE_CONNECTION_FAIL)  ||
			(data->d.params[0] == PND_TYPE_LOGIN_FAIL)  ||
			(data->d.params[0] == PND_TYPE_VIDEO_START)  ||
			(data->d.params[0] == PND_TYPE_CONFIG_FAIL))
		{
			stop_gear(ch);
		}
	}

	return 0;
}

static gint _check_vloss(NF_NOTIFY_INFO *data)
{
	guint i;
	gint is_changed = 0;

	if(data) {
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (iuxm.gear.vloss_mask & (1 << i))
			{
    			if (!(data->d.params[0] & (1 << i)))
					is_changed = 1;
			}

			if (is_changed)
			{
				stop_gear(i);
				is_changed = 0;
			}
		}

		iuxm.gear.vloss_mask = data->d.params[0];
	}
	return 0;
}

static gint _check_capture_boot_mode()
{
	gint i;

	if (!scm_is_enable_capture_mode()) return -1;

	for (i = 0; i < 3; i++)
	{
		nf_dev_buzzer_on();
		sleep(1);
		nf_dev_buzzer_off();
		sleep(1);
	}

	nftool_mbox(NF_TOPWND, "NOTICE", "Capture mode enabled.", NFTOOL_MB_OK);

	return 0;
}

static int _check_abnormal_reboot(int mode)
{
	REBOOT_INFO_CONT info;
	REBOOT_INFO_E ret;
	char buf[1024];

	memset(&info, 0x00, sizeof(info));

	ret = scm_get_reboot_info(&info);

	switch (ret) {
	case REB_NORMAL_BOOT:
		if (mode == 1) scm_put_log(SYSTEM_STARTED, 0, 0);
		return 0;
		break;

	case REB_CAM_ERROR:
		sprintf(buf, "%s%s%s%s%s%s%s",
			lookup_string("Poor communication with the camera is detected."),
			" (", info.content, ")\n",
			lookup_string("Please check the following precautions.\n\n"),
			lookup_string("Remedy:\n"),
			lookup_string("1. Make sure the NVR rear panel of the port LED is lit.\n"));
			//lookup_string("2. Using a cable test function that is available in the menu, please check the cables."));
		if (mode == 1) {
			scm_put_log(ABNORMAL_CAMERR, 0, 0);
			scm_put_log(SYSTEM_STARTED, 0, 0);
		}
		break;
	case REB_DISK_ERROR:
		sprintf(buf, "%s%s%s%s%s%s%s%s",
			lookup_string("Disk read / write error was detected during."),
			" (", info.content, ")\n",
			lookup_string("Please check the following precautions.\n\n"),
			lookup_string("Remedy:\n"),
			lookup_string("1. Please check that the disk is securely connected.\n"),
			lookup_string("2. aging disk or disks If physical damage is suspected, Use and replace with a new disk."));
		if (mode == 1) {
			scm_put_log(ABNORMAL_DISKERR, 0, 0);
			scm_put_log(SYSTEM_STARTED, 0, 0);
		}
		break;
	case REB_REC_ERROR:
		sprintf(buf, "%s%s%s%s%s%s",
			lookup_string("The recording defect has been detected.\n"),
			lookup_string("Please check the following precautions.\n\n"),
			lookup_string("Remedy:\n"),
			lookup_string("1. Please check the camera connection status, and disk connection.\n"),
			lookup_string("2. If you have repeated problems, please use after you replace or format the disc.\n"),
			lookup_string("   In this case, however, the stored data will be deleted."));
		if (mode == 1) {
			scm_put_log(ABNORMAL_RECERR, 0, 0);
			scm_put_log(SYSTEM_STARTED, 0, 0);
		}
		break;
	case REB_RECOVERY_ERROR:
		sprintf(buf, "%s%s%s%s%s",
			lookup_string("Disk Recovery failed.\n"),
			lookup_string("Please check the following precautions.\n\n"),
			lookup_string("Remedy:\n"),
			lookup_string("1. You can repair the disk using the format.\n"),
			lookup_string("2. If the problem is repeated, replace the disk."));
		if (mode == 1) {
			scm_put_log(ABNORMAL_RCVERR, 0, 0);
			scm_put_log(SYSTEM_STARTED, 0, 0);
		}
		break;

	case REB_NONE:
		// not logged intentionlly
		//scm_put_log(ABNORMAL_SHUTDOWN, 0, 0);
		if (mode == 1) scm_put_log(SYSTEM_STARTED, 0, 0);
		return 0;
		break;
	}

	if (mode == 0)
		nftool_mboxEx_auto_ok(NF_TOPWND, 5, "RECOVERY NOTIFICATION", buf);
	else
		nftool_mboxEx_ok(NF_TOPWND, 5, "RECOVERY NOTIFICATION", buf);

	return 0;
}


static gboolean _uxm_event_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CMM_MESSAGE_T msg;

	evt_unref();
	msg.msgid 		= event->client.data.l[0];
	msg.param 		= event->client.data.l[1];
	msg.dyn_data	= event->client.data.l[2];
	msg.data 		= event->client.data.l[3];

	DMSG(9, "MSGID = [0x%08x], PARAM = [%ld], DYN = [0x%08x], DATA = [%p]\n",
		msg.msgid, msg.param, msg.dyn_data, msg.data);

//    g_message("%s, %d, id:%08X, msg:%s", __FUNCTION__, __LINE__, msg.msgid, iux_translate_msg_desc(msg.msgid));

	switch(msg.msgid) {
	case IRET_BOOTWND_OPENED:
		DMSG(1, "");
		_check_capture_boot_mode();
		iuxm.boot_step = BS_BOOTWND_OPEN;
		_check_abnormal_reboot(0);
		_check_prev_unc_error();
		scm_bootup_system(IRET_SCM_BOOTUP_SYSTEM);
		break;

	case IRET_SCM_BOOTUP_SYSTEM:
		DMSG(1, "");
		iuxm.boot_step = BS_SCENARIO_CMPL;
		scm_reload_disk_info();
		_close_bootwnd();
		_put_fwupgrade_doneinfo();
		break;

	case IRET_BOOTWND_CLOSED:
		DMSG(1, "");
		iuxm.boot_step = BS_BOOTWND_CLOSE;
		_enter_live();
		if (dvaa_is_supported()) {
			nf_sql_create_facedata_table();
			nf_sql_create_lpr_table();
			nf_sql_create_default_lpr_group_table();
		}
		break;

	case INFY_INIT_LIVEVIEW:
		DMSG(1, "");
		iuxm.boot_step = BS_ENTER_LIVE;
		scm_put_message(INFY_LIVE_OPEN, 0, 0, 0);
		nfui_ui_unlock();
		_notice_resolution();
        _check_abnormal_reboot(1);
		_start_wizard();
		_open_login();
		_check_net_upgrade_result();
		break;

	case IRET_SCM_SHUTDOWN:
	case IRET_SCM_SHUTDOWN2:
	case IRET_SCM_SHUTDOWN_CAM_CHANGE:
	case IRET_SCM_SHUTDOWN_SIGNAL_CHANGE:
	case IRET_SCM_SHUTDOWN_HD_SPOT_CHANGE:
	case IRET_SCM_DUAL_MONITOR_CHANGE:
	case IRET_SCM_SHUTDOWN_AUTHCODE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_OPEN_ARCH_MANAGER:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_FORMAT_STORAGE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_ERASE_CH:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_APPLY_NETINFO:
	case IRET_SCM_APPLY_NETINFO2:
    case IRET_SCM_DHCP_RENEW:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_PLAYBACK_STARTED:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISP_IP_CAMERA_INFO:
	case INFY_DISP_INT_DISK_INFO:
	case INFY_DISP_ODD_INFO:
	case INFY_DISP_EXT_STORAGE_INFO:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VERIFY_RATE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_ARCH_VERIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_ENTER_CAMUP_MODE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_REG_RTSP:
	case IRET_SCM_RMV_RTSP:
	case IRET_SCM_REG_WEB:
	case IRET_SCM_RMV_WEB:
	case IRET_SCM_REG_DDNS:
	case IRET_SCM_TST_DDNS:
	case IRET_SCM_REG_UNIMO:
	case IRET_SCM_TST_FTP:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_DIAGNOSIS_POWER:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_DIAGNOSIS_NET:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_DIAGNOSIS_HDD:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_DIAGNOSIS_PORT:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CFRM_UNC_ERROR_DETECTED:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_FORMAT_CMPL:
	case INFY_RECOVERY_CMPL:
	case INFY_RTL_SET_CMPL:
	case INFY_BROKEN_RAID_START:
	case INFY_BROKEN_RAID_CMPL:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IREQ_CFRM_NODISK_DETECTED:
		DMSG(9, "");
		break;

	case INFY_ANALOG_REC_NOTIFY:
		DMSG(9, "");
		vvm_notify_analog_rec((NF_NOTIFY_INFO*)msg.data);
		_vsm_check_panic_duration((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_IPCAM_REC_NOTIFY:
		DMSG(9, "");
		break;

	case INFY_IPCAM_CHANGE_NOTIFY:
		DMSG(9, "");
		break;

	case INFY_VLOSS_NOTIFY:
		DMSG(9, "");
		_check_vloss((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_video_loss((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_NOVIDEO_NOTIFY:
		DMSG(9, "");
		break;

	case INFY_PND_NOTIFY:
		DMSG(9, "");
		_check_pnd((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_pnd((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, msg.data);
		break;

	case INFY_PND_RATE_NOTIFY:
		DMSG(9, "");
		_check_pndrate((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, msg.data);
		break;

	case INFY_PND_HUB_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, msg.data);
	    break;

	case INFY_NET_NOTIFY:
		DMSG(9, "");
		vvm_notify_net_client((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_NET_RXTX:
		DMSG(9, "");
//		vvm_notify_net_rxtx((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISK_USAGE_NOTIFY:
		DMSG(9, "");
		vvm_notify_disk_usage((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISK_FULL_NOTIFY:
		DMSG(9, "");
		vvm_notify_disk_full((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SMART_WARN_NOTIFY:
		DMSG(1, "");
		vvm_notify_disk_smart((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SMART_ERROR_NOTIFY:
		DMSG(1, "");
		vvm_notify_disk_smart_err((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISK_RAID_CHANGE_STATUS_NOTIFY:
		DMSG(1, "");
		vvm_notify_disk_raid((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_RAID_CHANGE_STATUS_RECOVER:
		DMSG(1, "");
		//if (vsm_get_vmode() != VMODE_LV) break;
		vvm_notify_raid_recover();
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_RAID_CHANGE_STATUS_DEGRADE:
		DMSG(1, "");
		//if (vsm_get_vmode() != VMODE_LV) break;
		vvm_notify_raid_degrade();
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_RAID_CHANGE_STATUS_REBUILD:
		DMSG(1, "");
		if (vsm_get_vmode() != VMODE_LV) break;
		vvm_notify_raid_rebuild();
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_RAID_CHANGE_STATUS_BROKEN:
		DMSG(1, "");
		//if (vsm_get_vmode() != VMODE_LV) break;
		vvm_notify_raid_broken();
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISK_OW_NOTIFY:
		DMSG(9, "");
		vvm_notify_disk_overwrite((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_POE_PORT_NOTIFY:
	case INFY_WRITEFAIL_NOTIFY:
	case INFY_WAN_NOTIFY:
	case INFY_DDNS_NOTIFY:
	case INFY_NETLOGINFAIL_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SYSFAN_NOTIFY:
		DMSG(9, "");
		vvm_notify_fan((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_TEMPERATURE_NOTIFY:
		DMSG(9, "");
		vvm_notify_temperature((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_SET_IP_CONFLICT_NOTIFY:
        DMSG(9, "");
        vvm_notify_set_ip_conflict((NF_NOTIFY_INFO*)msg.data);
        _emit_signal(msg.msgid, &msg);
        break;

    case INFY_CAM_IP_CONFLICT_NOTIFY:
        DMSG(9, "");
        vvm_notify_cam_ip_conflict((NF_NOTIFY_INFO*)msg.data);
        _emit_signal(msg.msgid, &msg);
        break;

	case INFY_POE_NOTIFY:
		DMSG(9, "");
		vvm_notify_poe((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_POE_HUB_NOTIFY:
		DMSG(9, "");
		vvm_notify_poe_hub((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_EXHAUST_NOTIFY:
		DMSG(9, "");
		vvm_notify_disk_exhaust((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_NODISK_NOTIFY:
		DMSG(9, "");
		vvm_notify_no_disk((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CAM_TITLE_NOTIFY:
		DMSG(9, "");
		vvm_notify_cam_title_change((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISPLAY_NOTIFY:
		DMSG(9, "");
		break;

	case INFY_COVERT_NOTIFY:
		DMSG(9, "");
		vvm_notify_covert((NF_NOTIFY_INFO*)msg.data);
		break;

	case INFY_ENC_NOTIFY:
		DMSG(9, "");
		break;

	case INFY_SYSDB_CHANGE_NOTIFY:
	case INFY_NETDB_CHANGE_NOTIFY:
	case INFY_AUDDB_CHANGE_NOTIFY:
	case INFY_DSKDB_CHANGE_NOTIFY:
	case INFY_RECDB_CHANGE_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CAMDB_CHANGE_NOTIFY:
		DMSG(9, "");
		vaa_reload();
		dvaa_reload();
		vvm_notify_corridor_mode_info((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_deeplearning_counter_property((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_vca_rule_info((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_dvabx_rule_info((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_ALMDB_CHANGE_NOTIFY:
		DMSG(9, "");
		vvm_notify_motion_mark((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DSPDB_CHANGE_NOTIFY:
		DMSG(9, "");
		vvm_notify_osd((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_USRDB_CHANGE_NOTIFY:
		DMSG(9, "");
		if (iuxm.boot_step >= BS_BOOTWND_CLOSE)
			vvm_notify_covert((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_ACTDB_CHANGE_NOTIFY:
		DMSG(9, "");
		vvm_notify_alarm_text_change((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case IREQ_FACTORY_DEFAULT:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SYSTEM_DATA_LOAD_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SENSOR_NOTIFY:
		DMSG(9, "");
		vsm_func_send_alarm_status((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_alarm((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_MOTION_NOTIFY:
		DMSG(9, "");
		vsm_func_send_motion_status((NF_NOTIFY_INFO*)msg.data);
		vvm_notify_motion((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_ALARM_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_PASSWD_INIT_BY_WEB:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CALENDAR_CLOSED:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_QUERY_NO_VIDEODATA:
	case INFY_QUERY_SUCCESS:
	case INFY_SNAP_QUERY_SUCCESS:
	case INFY_QUERY_OVER:
	case INFY_QUERY_ERROR:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_BURN_ERASING:
	case INFY_BURN_EXTRACTING:
	case INFY_BURN_PROG:
	case INFY_BURN_ERROR:
	case INFY_BURN_SUCCESS:
	case INFY_BURN_CANCEL:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_RESTART_SERVICE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_FWUP_RATE:
	case INFY_IPCAM_FWUP_RATE:
	case INFY_IPCAM_FWUP_CMPL:
	case INFY_IPCAM_FWUP_ERROR:
	case INFY_IPCAM_FWUP_ERROR_VERSION:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_MEDIA_STATUS_CHANGED:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DATA_DELETE_RATE:
		DMSG(9, "");
		break;

	case INFY_ERASE_CH_RATE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_RECOVERY_RATE:
	case INFY_RTL_SET_RATE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_FORMAT_RATE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CHECK_REMOTE_NEWFW:
	case INFY_CHECK_REMOTE_S1_NEWFW:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_TML_UNSET_SECTION:
	case INFY_TML_DATE_CHANGED:
	case INFY_TML_PLAY_CHANGED:
	case INFY_TML_START_CHANGED:
	case INFY_TML_END_CHANGED:
	case INFY_TML_SECTION_CHANGED:
	case INFY_TML_DOUBLE_CLICKED:
	case INFY_TML_SCROLL_UP:
	case INFY_TML_SCROLL_DOWN:
		_emit_signal(msg.msgid, &msg);
		break;

	case IREQ_BOOT_ERROR:
	case IREQ_BOOT_SMART_ERROR:
	case IREQ_BOOT_NODISK:
	case IREQ_BOOT_DISK_CONFLICT:
	case IREQ_BOOT_DISK_ADDED:
	case IREQ_BOOT_DISK_REMOVED:
	case IREQ_BOOT_DISK_CHANGED:
	case IREQ_BOOT_SYSTEM_DISK_REMOVED:
	case IREQ_BOOT_DISK_NEED_FORMAT:
	case IREQ_BOOT_FORMAT_RCVR:
	case IREQ_BOOT_ENFORCE_FORMAT:
	case IREQ_BOOT_BROKEN_RAID:
		_emit_signal(msg.msgid, &msg);
		break;

	case IREQ_CHANGE_ENHANCED_PASSWORD:
		_emit_signal(msg.msgid, &msg);
		break;

	case IREQ_WIZARD_INIT:
		_emit_signal(msg.msgid, &msg);
		break;

	case IRPL_SCM_128MB_FWUP_RAMDISK_RESULT:
	case IRPL_SCM_128MB_FWUP_RESULT_BY_WEB:
#if defined(CONFIG_FWUPGRADE_SINGLE)
        nf_fw_update_clear();
#endif
		var_set_enable_remote_upgrade(1);
		evt_send_to_local(INFY_CHECK_REMOTE_NEWFW, 0, 0, 0);
		break;

	case IREQ_CHANGE_LANG:
		DMSG(9, "");
		change_language_by_db();
		wnd_broadcast_event(GDK_EXPOSE);
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_FACTORY_DEFAULT_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SYSTEM_DATA_LOAD_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_UPGRADE_FW:
	case IRET_SCM_UPGRADE_IPCAM_FW:
	case IRET_SCM_DISK_CREATE_RAID:
	case IRET_SCM_DISK_DELETE_RAID:
	case IRET_SCM_JMFW_UPDATE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_CHANGE_SYSTEM_TIME:
	case IRET_SCM_CHANGE_SYSTEM_TIME2:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DISP_WAN_IP:
		DMSG(9, "");
        var_set_external_addr(msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AUDIOCH_NOTIFY:
		DMSG(9, "");
		vvm_notify_audio_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_MICOUT_NOTIFY:
		DMSG(9, "");
		vvm_notify_mic_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DVRLOGINFAIL_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_IPCAM_RESET_BEGIN:
	case INFY_IPCAM_RESET_END:
	case INFY_IPCAM_RESET_PENDING:
	case INFY_IPCAM_RESET_REQ_FAIL:
	case INFY_IPCAM_RESET_TIMEOUT:
	case INFY_IPCAM_CALIBRATION_BEGIN:
	case INFY_IPCAM_CALIBRATION_END:
	case INFY_IPCAM_CALIBRATION_PENDING:
	case INFY_IPCAM_CALIBRATION_REQ_FAIL:
	case INFY_IPCAM_CALIBRATION_TIMEOUT:
	case INFY_IPCAM_CALIBRATION_FAIL:
    case INFY_IPCAM_CALIBRATION_SUCCESS:
	case INFY_IPCAM_ONEPUSH_BEGIN:
	case INFY_IPCAM_ONEPUSH_END:
	case INFY_IPCAM_ONEPUSH_PENDING:
	case INFY_IPCAM_ONEPUSH_REQ_FAIL:
	case INFY_IPCAM_ONEPUSH_TIMEOUT:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_THUMBNAIL_CMPL_OBJ:
	case INFY_THUMBNAIL_CMPL_OBJ2:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_THUMBNAIL_TOTALLY_CMPL:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRPL_TML_GET_DATA:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_ALT_EXPIRED:
		DMSG(9, "");
		ssm_run_auto_logout();
		break;

	case INFY_FLUSH_FISHEYE_PARAM:
		feye_flush_save_data();
		break;

	case INFY_RECOVERY_EXPIRED:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case iNFY_NFEVT:
		_emit_nfevt(&msg);
		break;

	case INFY_DIV_CHANGE:
	case INFY_PB_IMAGE_STATUS:
    case INFY_PB_PLAY_STATUS:
    case INFY_NOT_SUPPORT_PLAYRATE:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_BUZZER_NOTIFY:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_IP_CHANGED_NOTIFY:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_USER_LOGON:
		_on_new_user(&msg);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_LANG_CHANGED:
		break;

	case IREQ_UI_LOCK:
		iuxm.wnd_ats = vw_mbox_wait(NF_TOPWND, "AUTO TIME SYNC", IMBX_AUTO_TIME_SYNC);
		cmm_send_message(CMMPT_SCM, IRPL_UI_LOCK, 0, 0, 0);		// perfunctory routine
		break;

	case IREQ_BOARD_CONFIRM:
		vw_open_board_confirm_mbox();
		break;

	case IREQ_UI_UNLOCK:
		vw_remove_waitbox(iuxm.wnd_ats);
		cmm_send_message(CMMPT_SCM, IRPL_UI_UNLOCK, 0, 0, 0);	// perfunctory routine
		break;

	case INFY_VWND_RUN_ALL_EVENT:
	case INFY_VWND_STOP_ALL_EVENT:
	case INFY_VWND_RUN_RIGHT_PRESS:
	case INFY_VWND_STOP_RIGHT_PRESS:
	case INFY_VWND_CHANGE_BORDER:
	case INFY_VWND_DRAW_FOCUS:
	case INFY_VWND_ERASE_FOCUS:
	case INFY_VWND_SWITCH_PRESS:
	case INFY_VWND_SWITCH_MOVE:
	case INFY_VWND_SWITCH_RELEASE:
	case INFY_VWND_SWITCH_CANCEL:
	case INFY_VWND_DRAW_NEW_VCA:
	case INFY_VWND_DRAW_NEW_DVABX:
    case INFY_VWND_DRAW_PTZARROW:
    case INFY_VWND_ERASE_PTZARROW:
    case INFY_VWND_DRAW_VIDEO_IMAGE:
    case INFY_VWND_ERASE_VIDEO_IMAGE:
		_emit_signal(msg.msgid, &msg);
		break;
	case INFY_HID_KPAD_NOTI:
		_emit_signal_hid_keypad_notify(msg.param, GPOINTER_TO_UINT(msg.data));
		break;
	case INFY_HID_CURSOR_CHANGE:
		nfui_signal_hid_mouse_cursor_change(msg.param, GPOINTER_TO_UINT(msg.data));
		break;
	case INFY_CAPTURE_IMAGE:
	case INFY_SHORTCUT_PIP_CAPTURE_IMAGE:
	case INFY_CAMSWITCH_CAPTURE_IMAGE:
	case INFY_AI_QUICK_REGIST_CAPTURE:
		_emit_signal(msg.msgid, &msg);
		_free_captrue_data(&msg);
		break;

	case INFY_RETRY_SHORTCUT_PIP_CAPTURE_IMAGE:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CAM_TITLE_CHANGED:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_PANIC_REC_STATUS:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

    case IRET_SCM_DCONFIG_MODE:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SCM_GET_DISK_INFO:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_GET_DDNS_STATUS:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_CHECK_CABLE_TEST:
        DMSG(9, "");
        _emit_signal(msg.msgid, &msg);
        break;

	case IREQ_NET_FWUP:
	case IREQ_DETECTED_NEWFW:
	case IREQ_S1_DETECTED_NEWFW:
	case IREQ_S1_DETECTED_NEWCAMFW:
	case IREQ_CHEAT_AUTO_FWUP:
	case INFY_DETECTED_NEWFW:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_QC_TEST_SET_BUTTON:
	case INFY_QC_TEST_INFO_ADD:
	case INFY_QC_TEST_INFO_UPDATE:
	case INFY_QC_TEST_INFO_CLEAR:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VKEY_SIZE_INCREASE:
	case INFY_VKEY_SIZE_DECREASE:
		_emit_signal(msg.msgid, &msg);
        break;

	case INFY_FORMAT_API_BEGIN:
	case INFY_FACDEF_API_BEGIN:
	case INFY_TIMECHANGE_API_BEGIN:
	case INFY_NETCHANGE_API_BEGIN:
	case INFY_SVCRESTART_API_BEGIN:
	case INFY_DBIMPORT_API_BEGIN:
	case INFY_ARCH_BURN_API_BEGIN:
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_emit_signal(msg.msgid, &msg);
        break;

	case INFY_FORMAT_API_CMPL:
	case INFY_FACDEF_API_CMPL:
	case INFY_TIMECHANGE_API_CMPL:
	case INFY_NETCHANGE_API_CMPL:
	case INFY_SVCRESTART_API_CMPL:
	case INFY_DBIMPORT_API_CMPL:
	case INFY_ARCH_BURN_API_CMPL:
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_emit_signal(msg.msgid, &msg);
        break;

	case INFY_OPENMODE_ENTER_INSTALL:
	case INFY_OPENMODE_LEAVE_INSTALL:
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_VWND_IPCAM_ZIG_INFO:
        vvm_set_ipcam_zig_info(&msg);
        break;

	case INFY_IPCAM_INSTALL_NOTIFY:
		DMSG(9, "");
		_emit_signal(msg.msgid, &msg);
		break;

	case IRPL_OPENMODE_IPSETUP:
	case IRPL_OPENMODE_CHANGE_PW:
	case IRPL_OPENMODE_PORTSETUP:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_REBOOT_SYSTEM:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_FORMAT_BY_WEB:
	case INFY_NODISK_BY_WEB:
	case INFY_USERINFO_INIT_BY_WEB:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_EVENT_NOTIFY:		
		vaa_notify_vca_event((NF_NOTIFY_INFO *)msg.data);
		vsm_func_send_vca_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_TRACKINFO_NOTIFY:
		vaa_notify_vca_track_info((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_META_DATA_NOTIFY:
		vaa_notify_vca_meta_data((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_COUNTER_NOTIFY:
		vaa_notify_vca_counter_info((NF_NOTIFY_INFO *)msg.data);
		vvm_notify_vca_cntr_status((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VAA_SELECT_RULE_ID:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_MODULAR_EVENT:
		vvm_notify_vca_event_status((NF_NOTIFY_INFO*)msg.data);
		vsm_func_send_vca_status((NF_NOTIFY_INFO*)msg.data);
		break;

	case INFY_VCA_ANALYZE_NOTIFY:
		vaa_notify_vca_analyze_event((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_EVENT_NOTIFY:
		dvaa_notify_event((NF_NOTIFY_INFO *)msg.data);
		vsm_func_send_dvabx_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_TRACKINFO_NOTIFY:
		dvaa_notify_track_info((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_META_DATA_NOTIFY:
		dvaa_notify_meta_data((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_COUNTER_NOTIFY:
		dvaa_notify_counter_info((NF_NOTIFY_INFO *)msg.data);
		vvm_notify_dvabx_cntr_status((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_MODULAR_EVENT:
		vvm_notify_dvabx_event_status((NF_NOTIFY_INFO*)msg.data);
        vsm_func_send_dvabx_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_ANALYZE_NOTIFY:
		dvaa_notify_analyze_event((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

#if 0
	case INFY_AI_FACE_EVENT_NOTIFY:
		_parse_algorithm_ai_face_event((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_FACE_TRACKINFO_NOTIFY:
		_parse_algorithm_ai_face_trackinfo((NF_NOTIFY_INFO *)msg.data);
		dvaa_face_notify_track_info((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_FACE_MODULAR_EVENT:
        vsm_func_send_dvabx_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;		

	case INFY_AI_PLATENO_EVENT_NOTIFY:
		_parse_algorithm_ai_plateno_event((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;		

	case INFY_AI_PLATENO_TRACKINFO_NOTIFY:
		_parse_algorithm_ai_plateno_trackinfo((NF_NOTIFY_INFO *)msg.data);
		dvaa_plateno_notify_track_info((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;		

	case INFY_AI_PLATENO_MODULAR_EVENT:
        vsm_func_send_dvabx_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;	
#endif

	case INFY_AI_GENERIC_EVENT_NOTIFY:
		dvaa_notify_generic_event((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AI_KEEP_ALIVE_NOTIFY:
		vvm_notify_ai_keepalive((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DELAY_DEEPLEARNING_EVENT:
	case INFY_DELAY_AI_EVENT:
	case INFY_DELAY_AI_FACE_EVENT:
	case INFY_DELAY_AI_PLATENO_EVENT:
	case INFY_DELAY_AI_GENERIC_EVENT:
	case INFY_DEEPLEARNING_GROUP_THUMBNAIL:
	case INFY_DEEPLEARNING_ANIMATE_THUMBNAIL:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SMS_TEST_RESULT:
	case INFY_EMAIL_VERIFICATION_RESULT:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_VCA_SELECT_MODIFY_POINT :
	case INFY_STREAM_DATA_RELOAD:
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_TIMECHANGE:
        _emit_signal(msg.msgid, &msg);
        break;

	case INFY_REMOTE_S1_FWUP_NOTIFY:
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_VAA_ITX_PRESS_ZONE_ID:
    case INFY_VAA_ITX_2PRESS_ZONE_ID:
    case INFY_VAA_ITX_PRESS_CNTR_ID:
    case INFY_VAA_ITX_2PRESS_CNTR_ID:
    case INFY_VAA_ITX_PRESS_CALB_ID:
    case INFY_VAA_ITX_2PRESS_CALB_ID:
    case INFY_VAA_ITX_PRESS_NONE_ID:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_DVAA_ITX_PRESS_ZONE_ID:
    case INFY_DVAA_ITX_2PRESS_ZONE_ID:
    case INFY_DVAA_ITX_PRESS_CNTR_ID:
    case INFY_DVAA_ITX_2PRESS_CNTR_ID:
    case INFY_DVAA_ITX_PRESS_CALB_ID:
    case INFY_DVAA_ITX_2PRESS_CALB_ID:
    case INFY_DVAA_ITX_PRESS_NONE_ID:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_DETECT_POSDEV_NOTIFY:
    case INFY_POS_STATUS_NOTIFY:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_POS_EVENT_NOTIFY:
		vsm_func_send_pos_status((NF_NOTIFY_INFO*)msg.data);
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_TEXTBOX_ADD_LINE:
    case INFY_TEXTBOX_MODIFY_LINE:
    case INFY_TEXTBOX_CLEAR:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_LEAVE_LIVE:
        _leave_from_live();
        break;

    case INFY_DETECTED_CLON_DEVICE:
        vvm_detected_clondev(msg.param);
		_emit_signal(msg.msgid, &msg);        
        break;

    case INFY_PACKET_DUMP_INSERT_MEDIA:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_PACKET_DUMP_REMOVE_MEDIA:
		_emit_signal(msg.msgid, &msg);
        break;
	case INFY_DEFAULT_LOAD_CSTLAYOUT:
		DMSG(9, "");	
		vsm_load_cstlayout();
		break;
		
    case INFY_SCREEN_LIVE_DIV_CHANGE:
    case INFY_SCREEN_LIVE_FULL_TOGGLE:
    case INFY_SCREEN_LIVE_OSD_TOGGLE:
    case INFY_SCREEN_LIVE_SEQ_TOGGLE:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_APPQC_AUTO_TEST_ALL:
    case INFY_APPQC_AUTO_TEST_ALARM:
    case INFY_APPQC_AUTO_TEST_AUDIO:
    case INFY_APPQC_AUTO_TEST_NETWORK:
    case INFY_APPQC_RTC_SYNC:
    case INFY_APPQC_FACTORY_DEFAULT:
    case INFY_APPQC_AUTO_TEST_RS485:
    case INFY_APPQC_AUTO_TEST_FAN:
    case INFY_APPQC_AUTO_TEST_TEMPER:
    case INFY_APPQC_DHCP_RENEW:
    case INFY_APPQC_AUTO_TEST_RS232:
    case INFY_APPQC_AUTO_TEST_POE:
		_emit_signal(msg.msgid, &msg);
        break;

    case INFY_SCM_PREPARE_FWUP_OPEN:
    case IREQ_SCM_PREPARE_FWUP_IMAGESAVE:
    case IRPL_SCM_PREPARE_FWUP_IMAGESAVE:
    case IRET_SCM_PREPARE_FWUP_VALIDATE:
    case IRET_SCM_PREPARE_FWUP_DATABACKUP:
    case IRET_SCM_PREPARE_FWUP_CMPL:
	case IREQ_SCM_128MB_FWUP_RESULT:
	case IREQ_SCM_128MB_FWUP_RAMDISK_RESULT:
	case IREQ_SCM_128MB_FWUP_RESULT_BY_WEB:
	case IREQ_SCM_CONFIRM_FWUP_BY_WEB:
	case IREQ_SCM_128MB_REMOTE_UPDATE_ALLINONE:
		_emit_signal(msg.msgid, &msg);
        break;

	case INFY_DEEPLEARNING_OBJECT_NOTIFY:
	case INFY_DEEPLEARNING_EVENT_NOTIFY:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DEEPLEARNING_COUNTER_NOTIFY:
		vvm_notify_deeplearning_counter_notify((NF_NOTIFY_INFO *)msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_DEEPLEARNING_MODULAR_EVENT:
		vsm_func_send_dva_status((NF_NOTIFY_INFO*)msg.data);
		break;

    case IRET_COMPLETED_GETTING_LICENSE:
		_emit_signal(msg.msgid, &msg);
		break;

    case INFY_WAIT_DRAW_EXPOSE:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_FACE_BULK_UPLOAD_RESULT:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_CAPTURE_NETWORK_MAP_SCREEN:
		{
			IMSG ret_msg = GPOINTER_TO_UINT(msg.data);
			int cat = _get_msg_category(msg.msgid);
			int msgidx = GET_MSGIDX(msg.msgid);	
			if (imsgt[cat].minfo[msgidx].obj[0]) _emit_signal(msg.msgid, &msg);
			else cmm_send_message(CMMPT_WEB, ret_msg, 0, 0, 0);		
		}
		break;

	case INFY_AIBOX_SETUP_CHANGE_NOTIFY:
		dvaa_update_external_airules();
		break;

	case INFY_EXTERNAL_ANALYTICS_RULES:
		dvaa_external_rules(msg.param, msg.data);
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_AIBOX_ANALYTICS_ADDITIONAL_EVENT:
		_emit_signal(msg.msgid, &msg);
		_free_aibox_analytics_additional_event(&msg);
		break;

	case INFY_LONG_DISTANCE_NOTIFY:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_NVM_DATA_IS_CHANGED:
	case INFY_OPENMODE_ENTER_INSTALL_BY_NVM:
	case INFY_OPENMODE_LEAVE_INSTALL_BY_NVM:
	case INFY_OPENMODE_READY_INSTALL_BY_NVM:
	case INFY_OPENMODE_FAIL_INSTALL_BY_NVM:
	case IRET_NVM_UPDATE_CONN_STATUS:
	case IRET_NVM_APPLY_CAMERA_SETTING:
	case IRET_NVM_APPLY_AND_FINALIZE:
	case IRET_NVM_START_CAMERA_INSTALL:
 	case INFY_GET_IPCAM_THUMBNAIL:
		_emit_signal(msg.msgid, &msg);
		break;

	case INFY_SHOW_OLD_VER_CAM_INSTALL:
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_SSL_INSTALL:
	case IRET_SSL_DELETE:
		_emit_signal(msg.msgid, &msg);
		break;

	case IRET_EXPORT_DEBUG_DATA:
		_emit_signal(msg.msgid, &msg);
		break;

	default:
		DMSG(9, "MSGID = [0x%08x]\n", msg.msgid);
		break;
	}

	_free_nfnotify_data(&msg);
	if (msg.dyn_data && msg.data) ifree(msg.data);

	return FALSE;
}

static int _read_conf(UXM_T *piuxm)
{
	int ret;
	ret = icf_get_value_by_int("uxm", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}

////////////////////////////////////////////////////////////
//
//	protected interfaces
//


////////////////////////////////////////////////////////////
//
//	public interfaces
//

int uxm_init()
{
	memset(&iuxm, 0x00, sizeof(iuxm));
	memset(&imsgt, 0x00, sizeof(imsgt));

	iuxm.mtx = g_mutex_new();

	iuxm.wuxm = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_default_size(iuxm.wuxm, 0, 0);
	gtk_window_move(iuxm.wuxm, -1, -1);
	gtk_widget_show(iuxm.wuxm);
	g_signal_connect(G_OBJECT(iuxm.wuxm),
					"client-event",
					G_CALLBACK(_uxm_event_handler),
                    NULL);

	_read_conf(&iuxm);

	DMSG(9, "");
	wnd_init(iuxm.wuxm);
	evt_init(iuxm.wuxm);
	nfui_nfwindow_link_wnd();
	// init beep
	nffunc_beep_on_off();

	init_event_loop();
	_init_input_devices();
	_init_language();
	_init_vloss();

	nftool_tooltip_init();

	nfui_create_image();
    vw_menu_init();
    VW_Create_Remote_Msg_Popup(NF_TOPWND);
    VW_Create_NVM_Msg_Popup(NF_TOPWND);
    vw_progress_fwup_128MB_open(NF_TOPWND);
	DMSG(9, "");
	return 0;
}

int uxm_reg_imsg_event(NFOBJECT *obj, IMSG msgid)
{
	int cat = _get_msg_category(msgid);
	int msgidx = GET_MSGIDX(msgid);
	return _register_message(cat, msgidx, obj);
}

int uxm_unreg_imsg_event(NFOBJECT *obj, IMSG msgid)
{
	int cat = _get_msg_category(msgid);
	int msgidx = GET_MSGIDX(msgid);
	return _unregister_message(cat, msgidx, obj);
}

int uxm_monitor_on_imsg_event(NFOBJECT *obj, IMSG msgid)
{
	int cat = _get_msg_category(msgid);
	int msgidx = GET_MSGIDX(msgid);
	return _monitor_on_message(cat, msgidx, obj);
}

int uxm_monitor_off_imsg_event(NFOBJECT *obj, IMSG msgid)
{
	int cat = _get_msg_category(msgid);
	int msgidx = GET_MSGIDX(msgid);
	return _monitor_off_message(cat, msgidx, obj);
}

#ifdef ENABLE_WAIT_WINDOW
gboolean close_wait_window(void)
{
	proxy_system("killall -9 wait_window", 1, 3);
	return 1;
}
#endif

int uxm_bootup()
{
    _vvm_set_clear_vwnd();
	vwnd_show();

    if(scm_is_qc_mode() != 0)
	{
    	//if((DAL_get_fac_init_func() && DAL_get_fac_init_run()))
    	if (vw_run_default_value_settings())
    	{
    		//VW_fac_init();
    		VW_Default_Value_Settings();
    	}
    }

    if (scm_is_qc_mode() == 0)
    {
        if (strcmp(nf_sysman_get_qcmode_option_lang(), "chinese") == 0)
        {
            DAL_set_language("CHINESE(T)");
    		change_language_by_db();
            DAL_set_timezone(TZ_SHANGHAI);
            scm_apply_timezone(TZ_SHANGHAI, NULL);
        }
	else if(strcmp(nf_sysman_get_qcmode_option_lang(), "korean") == 0)
        {
            DAL_set_language("KOREAN");
    		change_language_by_db();
            DAL_set_timezone(TZ_KOREA);
            scm_apply_timezone(TZ_KOREA, NULL);
        }
        else
        {
            DAL_set_language("ENGLISH");
    		change_language_by_db();
            DAL_set_timezone(TZ_ENGLISH);
            scm_apply_timezone(TZ_ENGLISH, NULL);
        }

    }

	VW_BootInfo(NF_TOPWND, IRET_BOOTWND_OPENED);
#ifdef ENABLE_WAIT_WINDOW
	close_wait_window();
#endif
	DMSG(9, "");
	return 0;
}

/*inline*/ GThread *uxm_get_callid()
{
	return iuxm.callid;
}

extern void evt_put_to_uxm(void *arg);

int uxm_run()
{
	iuxm.callid = g_thread_self();

	g_timeout_add(20, evt_put_to_uxm, 0);

//	gdk_threads_enter();
	gtk_main();
//	gdk_threads_leave();
	return 0;
}

int uxm_is_booting()
{
	return (iuxm.boot_step < BS_ENTER_LIVE);
}

//int uxm_leave_from_live()
//{
//	_leave_from_live();
//	return 0;
//}
