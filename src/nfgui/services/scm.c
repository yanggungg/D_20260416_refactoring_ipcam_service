/*
 * scm.c
 * 	- scenario manager
 *	- dependencies :
 *			GThread
 *			VFS
 *
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 29, 2010
 *
 */

#include "scm_internal.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "ix_conf.h"
#include "cmm.h"
#include "vfs.h"
#include "mda.h"
#include "wrk.h"
#include "uxm.h"
#include "scm.h"
#include "qry.h"

#include "nfdal.h"
#include "nf_common.h"
#include "nf_api_disk.h"
#include "nf_sysman.h"
#include "nf_util_device.h"
#include "evt.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"SCM"



#define SCM_LOCK()		g_mutex_lock(iscm.mtx)
#define SCM_UNLOCK()	g_mutex_unlock(iscm.mtx)


////////////////////////////////////////////////////////////
//
// private variable
//

SCM_T	iscm;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _process_message(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	switch (pmsg->msgid) {
	case iNFY_BOOTING_START:
		_scm_on_booting_start(piscm, pmsg);
		break;

	case iNFY_DISKINIT_START:
		DMSG(9, "");
		_scm_on_diskinit_start(piscm, pmsg);
		break;

	case iNFY_DISKRAID_CHECK:
		DMSG(9, "");
		_scm_on_diskraid_check(piscm, pmsg);
		break;

	case iNFY_ENHANCED_PASSWORD_CHECK:
		DMSG(9, "");
		_scm_on_enhanced_password_check(piscm, pmsg);
		break;

	case iNFY_128MB_FWUPGRADE_RESULT_CHECK:
		DMSG(9, "");
		_scm_on_128MB_fwupgrade_result_check(piscm, pmsg);
		break;

    case iNFY_WIZARD_CHECK:
		DMSG(9, "");
        _scm_on_wizard_check(piscm, pmsg);
        break;

	case iNFY_EMAIL_SEND:
		break;

	case iRET_WRK_GET_WANIP:
	case iRET_WRK_REG_RTSP:
	case iRET_WRK_RMV_RTSP:
	case iRET_WRK_TST_RTSP:
	case iRET_WRK_REG_WEB:
	case iRET_WRK_RMV_WEB:
	case iRET_WRK_TST_WEB:
	case iRET_WRK_REG_DDNS:
	case iRET_WRK_TST_DDNS:
	case iRET_WRK_REG_UNIMO:
	case iRET_WRK_GET_DDNS_STATUS:
	case iRET_WRK_TST_FTP:
		_scm_on_network_work_cmpl(piscm, pmsg);
		break;

    case iRET_WRK_ENTER_DMODE:
    case iRET_WRK_LEAVE_DMODE:
        _scm_on_dirmode_cmpl(piscm, pmsg);
        break;

	case iRET_WRK_CHECK_UNC:
		_scm_on_unc_cmpl(piscm, pmsg);
		break;

	case iRET_ACP_VERIFY_FILE:
		_scm_on_verify_cmpl(piscm, pmsg);
		break;

	case iRET_VFS_STOP_FS:
		DMSG(9, "");
		_scm_on_fs_stop(piscm, pmsg);
		break;

	case iRET_VFS_START_FS:
		DMSG(9, "");
		_scm_on_fs_start(piscm, pmsg);
		break;

	case iRET_VFS_STOP_REC:
		DMSG(9, "");
		_scm_on_rec_stop(piscm, pmsg);
		break;

	case iRET_VFS_START_REC:
		DMSG(9, "");
		_scm_on_rec_start(piscm, pmsg);
		break;

	case iRET_VFS_STOP_NET:
		DMSG(9, "");
		_scm_on_net_stop(piscm, pmsg);
		break;

	case iRET_VFS_START_NET:
		DMSG(9, "");
		_scm_on_net_start(piscm, pmsg);
		break;

	case iRET_VFS_DELETE_DATA:
		DMSG(9, "");
		_scm_on_delete_data_cmpl(piscm, pmsg);
		break;

	case iRET_VFS_FORMAT_STORAGE:
		_scm_on_format_cmpl(piscm, pmsg);
		break;

	case iRET_VFS_ERASE_CH:
		_scm_on_erase_ch_cmpl(piscm, pmsg);
		break;

	case iRET_VFS_BROKEN_RAID:
		_scm_on_broken_raid_cmpl(piscm, pmsg);
		break;

	case iRET_FU_RUN_FW_UPGRADE:
		_scm_on_fw_up(piscm, pmsg);
		break;

	case iRET_VFS_SET_RTL:
		_scm_on_rtl_cmpl(piscm, pmsg);
		break;

	case iRET_SMART_CHECK:
		_scm_on_smart_cmpl(piscm, pmsg);
		break;

	case iRET_WRK_CAPTURE_IMAGE:
		_scm_on_capture_work_cmpl(piscm, pmsg);
		break;

	case iRET_WRK_DIAGNOSIS:
		_scm_on_diagnosis_work_cmpl(piscm, pmsg);
		break;

	case iRET_WRK_CAMFW_UPGD:
		_scm_on_camfw_up(piscm, pmsg);
		break;

    case iRET_WRK_CHECK_CABLE:
        _scm_on_check_cable_cmpl(piscm, pmsg);
        break;

    case iRET_WRK_GET_DISK_INFO:
        _scm_on_get_disk_info_cmpl(piscm, pmsg);
       break;

    case iRET_WRK_GET_LICENSE:
        _scm_on_get_license_cmpl(piscm, pmsg);
        break;
        
	case iREQ_SPOT_CMD:
		_scm_on_spot_command(piscm, pmsg);
		break;

	case iRET_WRK_APPQC_TEST:
		_scm_on_appqc_work_cmpl(piscm, pmsg);
		break;

	case iNFY_RCVR_EXPIRED:
		_scm_on_rcvr_expired(piscm, pmsg);
		break;

	case iRET_WRK_PREPARE_FWUP:
        _scm_on_prepare_fwup_work_cmpl(piscm, pmsg);
		break;

	case iRET_WRK_EXPORT_DEBUG_DATA:
		_scm_on_export_debug_data_cmpl(piscm, pmsg);
		break;

	case iNFY_NET_FWUP:
#if defined(CONFIG_FWUPGRADE_SINGLE)
		_scm_do_ready_net_128MB_fw_upgrade(piscm, pmsg);
#else
		_scm_check_fw_state(piscm, pmsg);
#endif
		break;

	case iNFY_OPENMODE_IPSETUP:
		_scm_openmode_req_ip_assign(piscm, pmsg);
		break;

	case iNFY_OPENMODE_CHANGE_PW:
		_scm_openmode_change_pw(piscm, pmsg);
		break;

	case iNFY_OPENMODE_PORTSETUP:
		_scm_openmode_port_setup(piscm, pmsg);
		break;

	case iNFY_CAM_FWUP:
		_scm_work_camfw_upgrade(piscm, pmsg);
		break;

	case INFY_CFRM_UNC_ERROR_DETECTED:
		wrk_destroy_worker(piscm->wrk_unc);
		scm_buzzer_on();
		_scm_turnon_reboot_timer(piscm, 5000);
//		evt_send_msg_to_viewer(pmsg);
		_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
		break;

    case IRPL_BOOT_BROKEN_RAID:
		DMSG(9, "");
		_scm_on_disk_broken_raid_confirm(piscm, pmsg);
		break;

	case IRPL_BOOT_SMART_ERROR:
		_scm_on_smart_confirm(piscm, pmsg);
		break;

	case IRPL_BOOT_NODISK:
	case IRPL_BOOT_DISK_CONFLICT:
	case IRPL_BOOT_DISK_ADDED:
	case IRPL_BOOT_DISK_REMOVED:
	case IRPL_BOOT_DISK_CHANGED:
	case IRPL_BOOT_DISK_NEED_FORMAT:
	case IRPL_BOOT_SYSTEM_DISK_REMOVED:
		DMSG(9, "");
		_scm_on_disk_sync_confirm(piscm, pmsg);
		break;

	case IRPL_BOOT_FORMAT_RCVR:
	case IRPL_BOOT_ENFORCE_FORMAT:
		DMSG(9, "");
		_scm_on_disk_format_confirm(piscm, pmsg);
		break;

	case IRPL_CHANGE_ENHANCED_PASSWORD:
		DMSG(9, "");
		_scm_on_enhanced_password_cmpl(piscm, pmsg);
		break;

	case IRPL_WIZARD_INIT:
		DMSG(9, "");
		_scm_on_wizard_cmpl(piscm, pmsg);
		break;

	case IRPL_SCM_128MB_FWUP_RESULT:
		DMSG(9, "");
		_scm_on_128MB_fwupgrade_result_cmpl(piscm, pmsg);
		break;

	case INFY_LIVE_OPEN:
		_scm_on_live_open(piscm, pmsg);
		break;

	case IRPL_BOARD_CONFIRM:
		_scm_on_board_confirm(piscm, pmsg);
		break;

	case INFY_RECOVERY_RATE:
		_scm_on_rcvr_rate(piscm, pmsg);
		break;

	case INFY_RECOVERY_CMPL:
		_scm_stop_rcvr_timer(piscm);
		_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
		if (!nf_sysman_is_normal_boot())
			scm_put_log(SYSTEM_RECOVERED, 0, 0);
		break;

    case INFY_USER_ACTION_BROKEN_RAID:
    	_scm_cancel_reboot(piscm);
    	scm_buzzer_off();
        break;

	case INFY_QUERY_SUCCESS:
		_scm_save_qry_result(piscm, (NF_ARCH_AVI_INFO *)pmsg->data);
		_scm_dup_pointer_to_viewer(pmsg, sizeof(NF_ARCH_AVI_INFO));
		break;

	case INFY_SNAP_QUERY_SUCCESS:
		_scm_save_snap_qry_result(piscm, (NF_ARCH_SNAP_INFO *)pmsg->data);
		_scm_dup_pointer_to_viewer(pmsg, sizeof(NF_ARCH_SNAP_INFO));
		break;

	case INFY_ALT_EXPIRED:
		_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
		break;

	case IRPL_NET_FWUP:
		_scm_do_ready_net_fw_upgrade(piscm, pmsg);
		break;

	case IRPL_DETECTED_NEWFW:
#if defined(CONFIG_FWUPGRADE_SINGLE)
		_scm_do_ready_net_128MB_fw_upgrade(piscm, pmsg);
#else
		_scm_do_ready_net_fw_upgrade(piscm, pmsg);
#endif
		break;
		
    case IRPL_SCM_128MB_REMOTE_UPDATE_ALLINONE:
        _scm_do_prepare_fwup_validate_check_url(piscm, pmsg);
        break;

	case IRPL_S1_DETECTED_NEWFW:
		_scm_do_ready_s1_remotefw_upgrade(piscm, pmsg);
		break;

	case IRPL_S1_DETECTED_NEWCAMFW:
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_scm_do_ready_s1_remotecamfw_upgrade(piscm, pmsg);
		break;

    case IRPL_SCM_PREPARE_FWUP_IMAGESAVE:
		_scm_do_prepare_fwup_data_backup(piscm, pmsg);
        break;

	case INFY_QUERY_NO_VIDEODATA:
	case INFY_QUERY_OVER:
		_scm_dup_pointer_to_viewer(pmsg, sizeof(NF_ARCH_AVI_INFO));
		break;

	case INFY_QUERY_ERROR:
		_scm_send_data_to_viewer(pmsg->msgid, pmsg->param, pmsg->data);
		break;

	case INFY_MEDIA_STATUS_CHANGED:
	    cdump_change_media();
		_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
		break;

	case INFY_RTL_SET_CMPL:
	case INFY_DATA_DELETE_RATE:
	case INFY_ERASE_CH_RATE:
	case INFY_RTL_SET_RATE:
	case INFY_FORMAT_RATE:
	case INFY_VERIFY_RATE:
	default:
		_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
		break;

	}

	if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
	return 0;
}

static gboolean _pump_message(void *arg)
{
	CMM_MESSAGE_T msg;
	SCM_T *piscm = (SCM_T *)arg;

	usleep(piscm->sleep_time);
	if (cmm_get_message(&msg) == 0) _process_message(piscm, &msg);

	return TRUE;
}

static void* _service_proc(void *arg)
{
	GMainLoop *loop = arg;
	g_main_loop_run(loop);
}

static int _read_conf(SCM_T *piscm)
{
	int ret;

	ret = icf_get_value_by_int("scm", "dmsg");
	if (ret != -1) DBG_USE(ret);

	ret = icf_get_value_by_int("scm", "recovery_timeout");
	piscm->cf.rcvr_timeout = (ret < 1 ? 150: ret);
	DMSG(9, "RECOVERY TIMEOUT = %d\n", piscm->cf.rcvr_timeout);

	ret = icf_get_value_by_int("scm", "ats_watchdog_timeout");
	piscm->cf.ats_wtdg_timeout = (ret < 1 ? 900: ret);
	DMSG(1, "WATCHDOG TIMEOUT = %d\n", piscm->cf.ats_wtdg_timeout);

	ret = icf_get_value_by_int("scm", "ats_enforce_timeout");
	piscm->cf.ats_enfc_timeout = (ret < 1 ? 0 : ret);
	DMSG(1, "ENFORCE ATS TIMEOUT = %d\n", piscm->cf.ats_enfc_timeout);

	return 0;
}


static int _init_scm_thread(SCM_T *piscm)
{
	GMainContext *context;
	context = g_main_context_new();
	piscm->loop = g_main_loop_new (context, FALSE);
	g_main_context_unref (context);

	iscm.thd = ifn_make_thread(_service_proc, piscm->loop);
	cmm_mount_on_thread(iscm.thd);
	iscm.cmmpt = iscm.thd;
	DMSG(1, "SCM CMMPT : [%p]\n", iscm.cmmpt);

	_scm_add_idle(piscm, _pump_message, (void *)piscm);
	return 0;
}

static int _apply_default_var()
{

}

static void *_dup_pointer(void *data, int len)
{
	void *ptr = imalloc(len);
	memcpy(ptr, data, len);
	return ptr;
}

static int _send_to_local(IMSG msgid, int param, bool dyn_data, void *data)
{
	return evt_send_to_local(msgid, param, dyn_data, data);
}

static int _send_to_web(IMSG msgid, int param, bool dyn_data, void *data)
{
	return cmm_send_message(CMMPT_WEB, msgid, param, dyn_data, data);
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_return_api(CALLID callid, IMSG ret_msg, int param, bool dyn_data, void *data)
{
	if (ret_msg == IMSG_NONE) return -1;
	DMSG(1, "CALLID : [%p]", callid);
	DMSG(1, "WEBCALL CALLID : [%p]", WEB_CALL);
	DMSG(1, "RET MSG : [%x]", ret_msg);
	if (callid == LOCAL_CALL) evt_send_to_local(ret_msg, param, dyn_data, data);
	if (callid == WEB_CALL) {
		cmm_send_message(CMMPT_WEB, ret_msg, param, dyn_data, data);
	}
}

guint _scm_add_timeout(SCM_T *piscm, guint interval, GSourceFunc func, gpointer data)
{
	GSource *src;
	guint id;

	src = g_timeout_source_new(interval);
	DMSG(1, "INTERVAL = %u", interval);
	g_source_set_callback(src, func, data, NULL);
	id = g_source_attach(src, g_main_loop_get_context(piscm->loop));
	g_source_unref(src);

	return id;
}

int _scm_remove_timeout(SCM_T *piscm, guint id)
{
	GSource *src;
	GMainContext *context;

	context = g_main_loop_get_context(piscm->loop);
	src = g_main_context_find_source_by_id(context, id);
	g_source_destroy(src);

	DMSG(1, "");
	return 0;
}

guint _scm_add_idle(SCM_T *piscm, GSourceFunc func, gpointer data)
{
	GSource *src;
	guint id;

	src = g_idle_source_new();
	g_source_set_callback(src, func, data, NULL);
	id = g_source_attach(src, g_main_loop_get_context(piscm->loop));
	g_source_unref(src);

	return id;
}

int _scm_remove_idle(SCM_T *piscm, guint id)
{
	GSource *src;
	GMainContext *context;

	context = g_main_loop_get_context(piscm->loop);
	src = g_main_context_find_source_by_id(context, id);
	g_source_destroy(src);

	return 0;
}

int _scm_put_message(IMSG msgid, int param, bool dyn_data, void *data)
{
	return cmm_send_message(CMMPT_SCM, msgid, param, dyn_data, data);
}

int _scm_send_data_msg_to_viewer(IMSG msgid, int param, bool dyn_data, void *data)
{
    _send_to_local(msgid, param, dyn_data, data);
    if (CMMPT_WEB) _send_to_web(msgid, param, dyn_data, data);
}

int _scm_send_msg_to_viewer(IMSG msgid, int param)
{
	_send_to_local(msgid, param, 0, 0);
	if (CMMPT_WEB) _send_to_web(msgid, param, 0, 0);
	return 0;
}

int _scm_send_data_to_viewer(IMSG msgid, int param, void *data)
{
	_send_to_local(msgid, param, 0, data);
	if (CMMPT_WEB) _send_to_web(msgid, param, 0, data);
	return 0;
}

int _scm_send_pointer_to_viewer(IMSG msgid, int param, void *data, int len)
{
	void *data_web = NULL;

	// sequence is very important, don't edit it

	if (CMMPT_WEB) data_web = _dup_pointer(data, len);
	_send_to_local(msgid, param, 1, data);
	if (CMMPT_WEB) _send_to_web(msgid, param, 1, data_web);

	return 0;
}

int _scm_dup_pointer_to_viewer(CMM_MESSAGE_T *pmsg, int len)
{
	void *data_web = NULL;

	// sequence is very important, don't edit it

	if (CMMPT_WEB) data_web = _dup_pointer(pmsg->data, len);
	_send_to_local(pmsg->msgid, pmsg->param, 1, pmsg->data);
	if (CMMPT_WEB) _send_to_web(pmsg->msgid, pmsg->param, 1, data_web);

	pmsg->dyn_data = 0;
	pmsg->data = 0;
	return 0;
}

int _scm_send_struct_to_viewer(IMSG msgid, int param, void *data, DUPL_PROC proc)
{
	void *data_web = NULL;

	// sequence is very important, don't edit it

	if (CMMPT_WEB) data_web = proc(data);
	_send_to_local(msgid, param, 1, data);
	if (CMMPT_WEB) _send_to_web(msgid, param, 1, data_web);

	return 0;
}

int _scm_dup_struct_to_viewer(CMM_MESSAGE_T *pmsg, DUPL_PROC proc)
{
	void *data_web = NULL;

	// sequence is very important, don't edit it

	if (CMMPT_WEB) data_web = proc(pmsg->data);
	_send_to_local(pmsg->msgid, pmsg->param, 1, pmsg->data);
	if (CMMPT_WEB) _send_to_web(pmsg->msgid, pmsg->param, 1, data_web);

	pmsg->dyn_data = 0;
	pmsg->data = 0;
	return 0;
}

int _scm_push_notification(IMSG msgid, TRANSACTION_E tra)
{
	CALLID callid = iscm.chart[tra].caller;
	int owner = 0;

	if (callid == LOCAL_CALL) owner = 0;
	else if (callid == WEB_CALL) owner = 1;
	else return -1;

	DMSG(1, "[%d]", owner);
	_scm_send_msg_to_viewer(msgid, owner);
	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_init()
{
	memset(&iscm, 0x00, sizeof(SCM_T));
	if (_read_conf(&iscm) == -1) return -1;

	iscm.mtx = g_mutex_new();
	iscm.sleep_time = 10000;
	_init_scm_thread(&iscm);

	_scm_init_timeline(&iscm);
//	_scm_register_syscb();
	_scm_check_novideo(&iscm);
	_scm_init_time_change();
	_scm_init_audio(&iscm);
	_scm_init_ipcam_poe_onoff(&iscm);
	_scm_init_mot_timer(&iscm);
	_scm_init_vca_event_timer(&iscm);
	_scm_init_dva_event_timer(&iscm);
	_scm_init_dvabx_event_timer(&iscm);
	_scm_init_notify(&iscm);
	_scm_init_ddns_hostname();
	_scm_init_spot_service(&iscm);
	_scm_license_init();
	_scm_init_moving_rtl_timer(&iscm);
	_scm_init_8021x_cert();
	mda_init();
	qry_init();
	_apply_default_var();
	return 0;
}

int scm_cleanup()
{
	g_mutex_free(iscm.mtx);
	g_mutex_free(iscm.mtx_tml);
}

/*inline*/ CMMPORT scm_get_cmmport()
{
	// don't worry about thread-safe
	return iscm.cmmpt;
}

int scm_put_message(IMSG msgid, int param, bool dyn_data, void *data)
{
	return _scm_put_message(msgid, param, dyn_data, data);
}

