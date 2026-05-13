/*
 * scm_sys.c
 * 	- system 
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

#include <glib.h>
#include "iux_afx.h"
#include "scm_internal.h"
#include "scm.h"
#include "vfs.h"
#include "nf_util_device.h"
#include "evt.h"
#include "smt.h"
#include "nf_network.h"
#include "nf_sysman.h"
#include "nf_api_archive.h"
#include "nf_api_eventlog.h"
#include "nf_api_openmode.h"
#include "nf_va_object_detector.h"
#if defined(CONFIG_FWUPGRADE_SINGLE)
	#include "nf_util_fw_single.h"
#endif

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_SYS"

#define IS_SUCCESS()		(result == 0)
#define RETRY_CNT		3

static gint _gpu_mode = 0;
static gint _dlva_detector_setup[GUI_CHANNEL_CNT] = {0, };


////////////////////////////////////////////////////////////
//
// private functions
//

static int _req_ipcam_info_display(SCM_T *piscm)
{
	_scm_send_msg_to_viewer(INFY_DISP_IP_CAMERA_INFO, 0);
	return 0;
}

static int _req_disk_info_display(SCM_T *piscm)
{
	_scm_send_msg_to_viewer(INFY_DISP_INT_DISK_INFO, 0);
	return 0;
}

static int _req_odd_info_display(SCM_T *piscm)
{
	_scm_send_msg_to_viewer(INFY_DISP_ODD_INFO, 0);
	return 0;
}

static int _req_estg_info_display(SCM_T *piscm)
{
	_scm_send_msg_to_viewer(INFY_DISP_EXT_STORAGE_INFO, 0);
	return 0;
}
	
static int _req_wizard_init(SCM_T *piscm, TRANSACTION_E tra, int need)
{
	DMSG(9, "");
	_scm_track_tra(piscm, IRPL_WIZARD_INIT, tra);
	_scm_send_msg_to_viewer(IREQ_WIZARD_INIT, need);
	return 0;
}

static int _reset_board()
{
	int i;

	scm_buzzer_on();
	for (i = 0; i < 3; ++i) {
		DMSG(1, "BOARD RESET !!!");
		usleep(200000);		// just for debugging msg
		nf_dev_board_reset();
		sleep(3);
	}

	g_assert(0);
	return 0;
}

static gboolean _proc_reset_system(gpointer data)
{
	SCM_T *piscm = (SCM_T *)data;
	REBOOT_REASON_E reason = piscm->reboot.reason;
	gint i, rslt;

	DMSG(1, "REASON = [%d]", reason);
	switch (reason) {
	case RR_NA:
		_scm_put_log(BOOTING_FAIL, 0, "BOOTING FAIL : UNKNOWN");
		break;
	case RR_UNC_ERROR:
		_scm_put_log(SYSTEM_SHUTDOWN, 0, 0);
		break;
	case RR_FWUP:
		nf_sysman_set_normal_boot();
		_scm_put_log(FW_UPGRADE, 0, 0);
		break;	
	case RR_CAM_UPGRADE:
		break;	
	case RR_NODISK:
//		_scm_increase_err_cnt(piscm, ET_DISK);
		_scm_put_log(BOOTING_FAIL, 0, "BOOTING FAIL : NO DISK");
		break;
	case RR_DISKERROR:
//		_scm_increase_err_cnt(piscm, ET_DISK);
		_scm_put_log(BOOTING_FAIL, 0, "BOOTING FAIL : CONFIRM TIMEOUT");
		break;
	case RR_SMART:
//		_scm_increase_err_cnt(piscm, ET_SMART);
		break;
	case RR_INV_BOARD:
		break;
	case RR_RCVREXPIRED:
		{
			NF_SST_PROGRESS progress;
			//captainnn
			if (nf_filesystem_get_progress(NF_SST_PRGT_FSSTART, &progress)) { 
				if (progress.type == NF_SST_PRGT_FSSTART && progress.total >= 0){
					DMSG(1, "FS START PROGRESS RATE [%d] = %d * 100 / %d DISK PDID %d \n", progress.current * 100 / progress.total, progress.current, progress.total,progress.pdid);
				}
			}   
			_scm_increase_err_cnt(piscm, ET_RECOVERY,progress.pdid);
		_scm_put_log(BOOTING_FAIL, 0, "BOOTING FAIL : RECOVERY EXPIRED");
		}
		break;
	case RR_WATCHDOG:
		break;
	case RR_CAM_CHANGE:
		break;
	case RR_SIGNAL_CHANGE:
		break;		
	case RR_INV_PASSWD:
		break;		
	case RR_NORMAL_BOOT:
		break;			
	case RR_INSTALLMODE_CHANGE:
#if defined(_IPXVE_MODEL_UX)
	/* manual poe reset for VE model */
		for(i = 0; i < 8; i++)
		{

			nf_dev_poe_port_onoff(i, 0, &rslt);
			usleep(100 * 1000);
		}
		for(i = 0; i < 8; i++)
		{
			nf_dev_poe_port_onoff(i, 1, &rslt);
			usleep(100 * 1000);
		}
		usleep(3 * 1000 * 1000);
#endif
//		_scm_put_log(CHANGE_IPCAM_INSTALLMODE, 0, 0);
		break;
	case RR_REBOOT_MENU:
		_scm_put_log(SYSTEM_SHUTDOWN, 0, 0);	    
		break;	

	case RR_RAID:
		break;	
	}

	piscm->reboot.timer = 0;	
	piscm->reboot.reason = 0;

	_reset_board();	
	return FALSE;
}

static int _is_over_retry_cnt(SCM_T *piscm)
{
	return (_scm_get_err_cnt(piscm) > RETRY_CNT);
}

static int _is_over_recovery_cnt(SCM_T *piscm)
{
	return (_scm_get_recovery_err_cnt(piscm) >= RETRY_CNT);
}

static int _is_pressed_igndisk_key(SCM_T *piscm)
{
	return 0;
}

static int _is_pressed_format_key(SCM_T *piscm)
{
	gboolean ret = nf_sysman_hotkey_is_format();
	if (ret) return 1;
	return 0;
}

static BOOT_MODE_E _get_boot_mode(SCM_T *piscm)
{
	BOOT_MODE_E bm = BM_NORMAL;
	DISK_CONF_E disk_conf;

	if (_scm_load_disk_info(piscm) != 0) disk_conf = DC_NODISK;
	else disk_conf = _scm_get_disk_change_info(piscm);

	if (_is_pressed_igndisk_key(piscm)) bm = BM_IGNDISK;
	else if (disk_conf == DC_NODISK) bm = BM_NODISK;
	else {
		if (_is_pressed_format_key(piscm)) bm = BM_ENFORCE_FORMAT;
		else {
			if (_is_over_recovery_cnt(piscm)) bm = BM_FORMAT;
			else if (_is_over_retry_cnt(piscm)) bm = BM_IGNDISK;
			else bm = BM_NORMAL;
		}
	}

	return bm;
}

static int _work_normal_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	DISK_CONF_E disk_conf;

	DMSG(9, "");
	_scm_start_unc_checking(piscm);
	_req_disk_info_display(piscm);
	_req_estg_info_display(piscm);

	disk_conf = _scm_get_disk_change_info(piscm);
	switch (disk_conf) {
	case DC_NOCHANGE:
		DMSG(1, "");
		_scm_change_wmode_by_db(piscm);
		_scm_work_smart_boot(piscm, tra);
		break;
	default:
		DMSG(1, "");
		_scm_reboot_system(piscm, RR_DISKERROR, 120000);
		_scm_req_disk_confirm(piscm, tra, disk_conf);
		scm_buzzer_on();
		break;
	}

	_scm_save_disk_info(piscm);

	return 0;
}

static int _work_format_boot(SCM_T *piscm, TRANSACTION_E tra, int opt)
{
	DMSG(1, "");
	_scm_reboot_system(piscm, RR_NA, 120000);
	if (opt == 0) _scm_req_disk_confirm(piscm, tra, DC_FORMAT_N_RCVR);
	else _scm_req_disk_confirm(piscm, tra, DC_ENFORCE_FORMAT);
	scm_buzzer_on();
	return 0;
}

static int _work_nodisk_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "");
	_scm_reboot_system(piscm, RR_NODISK, 30000);
	_scm_req_disk_confirm(piscm, tra, DC_NODISK);
	scm_buzzer_on();
	return 0;
}

static int _work_igndisk_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "");
	var_set_running_disk_count(0);
#if defined (_IPX_MODEL_UX)
	nf_ipcam_start();
#endif
	_scm_work_booting_end(piscm, SUCCESS);
	_scm_finalize_tra(piscm, tra, -1);
	return 0;
}

static int _work_brkraid_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "");

	_scm_track_tra(piscm, IRPL_BOOT_BROKEN_RAID, tra);
	_scm_send_msg_to_viewer(IREQ_BOOT_BROKEN_RAID, 0);
	//scm_send_to_viewer(IREQ_BOOT_BROKEN_RAID, 0, 0, 0);	
	_scm_reboot_system(piscm, RR_NA, 30000);
    scm_buzzer_on();    
	return 0;
}

static int _run_inv_board_scenario(SCM_T *piscm)
{
	TRANSACTION_E tra = TRA_INV_BOARD;
	_scm_ready_tra(piscm, tra, IMSG_NONE, NULL);

	_scm_track_tra(piscm, IRPL_BOARD_CONFIRM, tra);
	_scm_send_msg_to_viewer(IREQ_BOARD_CONFIRM, 0);
	_scm_reboot_system(piscm, RR_INV_BOARD, 60000);

	return 0;	
}

static int _is_valid_board(SCM_T *piscm)
{
	guchar hwver = 0;
	int ch = var_get_ch_count();

	if (nf_sysman_hotkey_is_nfs()) return 1;
	// ksi_test
	// if (nf_dev_board_pp_get_hwver(&hwver)) {
	// 	DMSG(1, "");
	// 	if (hwver == 0 && ch == 16) return 0;
	// 	else return 1;
	// }
	
	DMSG(1, "");
	return 0;
}

static int _proc_leave_dirmode(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    nf_record_start(NULL);

    sleep(3);

	DMSG(1, "");
	ack_ret = nf_ipcam_direct_config_stop();	
	DMSG(1, "%d", ack_ret);
	return ack_ret == 1 ? 0 : -1;
}

static int _proc_enter_dirmode(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;
	int ch;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    nf_record_stop(NULL);

    sleep(3);

	DMSG(1, "");
    ch = piscm->chart[tra].int_data;	
	ack_ret = nf_ipcam_direct_config_start(ch);
	
	if (!ack_ret) {
	    sleep(3);
		nf_record_start(NULL);
	}
	
	DMSG(1, "%d", ack_ret);
	return ack_ret == 1 ? 0 : -1;

}

static int _cleanup_dmode_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_dmode);
	piscm->wrk_dmode = 0;
	return 0;
}

static int _cleanup_check_cable(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_cable_check);
	piscm->wrk_cable_check = 0;
	return 0;
}

static int _cleanup_export_debug_data(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_export_debug_data);
	piscm->wrk_export_debug_data = 0;
	return 0;
}

static int _is_need_change_enhanced_password()
{
	SecurityData secdata;
	gchar strBuf[64];

#if 0
    memset(&secdata, 0x00, sizeof(SecurityData));
    memset(strBuf, 0x00, sizeof(strBuf));

	DAL_get_security_data(&secdata);
	DAL_get_user_passwd(strBuf, 0);

	if ((secdata.usable_defpw == 0) && (strcmp(strBuf, "1234") == 0)) return 1;
#endif

    return 0;
}

static int _is_need_new_wizard(SCM_T *piscm)
{
	WizardCheck usrdata;
	int net_ret;
	int ret_val;

	DAL_get_WizardCheck_Data(&usrdata, 0);
	DAL_get_Netwizard_func(&net_ret);

	if(!net_ret)
		usrdata.netwiz = FALSE;

	DMSG(1, "[%d]", usrdata.usable_defpw);
	if (usrdata.usable_defpw) return 0;
	DMSG(1, "");
//	if ((strcmp(usrdata.pw, "1234") == 0) && usrdata.netwiz)	return 3;
	if (strcmp(usrdata.pw, "1234") == 0)	return 1;
//	else if (usrdata.netwiz)	return 2;
	else return 0;
}

static int _is_need_check_raid()
{
#ifdef SUPPORT_DISK_RAID
    return 1;
#endif
    return 0;
}

static gint _get_sata_port_in_raid(gint raid_id, gint disk_index, DISK_RAIDINFO_T *rai)
{
	if(!rai) return -1;
	if(rai->mode == 0)	return -1;
//	if(gRaid_i->mode & RAID_CONF_MODE)	return -1;
	if(raid_id < 0) return -1;
	if(disk_index >= rai->rinfo[raid_id].member_count)
		return -1;

	return rai->rinfo[raid_id].sata_index[disk_index];
}

static int _is_broken_raid()
{
    DISK_RAIDINFO_T rai;
	int i;
	int disk_port;
    
    memset(&rai, 0x00, sizeof(rai));
	if (_scm_load_disk_info(&iscm) != 0) return 0;
    if (scm_get_disk_raidinfo(&rai) != 0) return 0;
    
	for (i = 0; i<rai.raid_cnt; i++) 
	{
	    if (rai.rinfo[i].status == 0) return 1;
		//if (rai.rinfo[i].member_count == 0) continue;
		
	    //disk_port = _get_sata_port_in_raid(i, 0, &rai);
	    
    	//if (disk_port < 0) return 0;
	}
	
	return 0;
}

static int _start_disk(SCM_T *piscm, TRANSACTION_E tra)
{
	BOOT_MODE_E bmode;
	bmode = _get_boot_mode(piscm);
	DMSG(1, "BMODE = %d\n", bmode);

	switch (bmode) {
	case BM_NORMAL:
		_work_normal_boot(piscm, tra);
		break;
	case BM_NODISK:
		_work_nodisk_boot(piscm, tra);
		break;
	case BM_IGNDISK:
		_work_igndisk_boot(piscm, tra);
		break;
	case BM_FORMAT:
		_work_format_boot(piscm, tra, 0);
		break;
	case BM_ENFORCE_FORMAT:
		_work_format_boot(piscm, tra, 1);
		break;
	}

    return 0;
}

static int _check_diskraid(SCM_T *piscm, TRANSACTION_E tra)
{
    if (_is_broken_raid()) _work_brkraid_boot(piscm, tra);
    else _scm_put_message(iNFY_DISKINIT_START, 0, 0, (void *)tra);
	    
    return 0;
}

static int _proc_check_cable(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
    NF_SYSMAN_SWITCH_VCT_RES vct_res;

	CMMACK_T cmmack;
	TRANSACTION_E tra;
	int port;
	int device;

	CHKCABLE_RES_T *res = imalloc(sizeof(CHKCABLE_RES_T));

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
    port = piscm->chart[tra].int_data;	
    device = piscm->chart[tra].char_data;

    memset(&vct_res, 0x00, sizeof(NF_SYSMAN_SWITCH_VCT_RES));
    vct_res.port = port;    
    vct_res.dev_num = device;
    
    nf_sysman_switch_vct_result(&vct_res);

    if(port == 9) port = 8;
    else if(port == 8) port = 9;

    res->port = port;
    res->device = device;
    res->length = vct_res.length;
    res->result = vct_res.result;
    res->cable_num = vct_res.cable_num;
    
    piscm->chart[tra].result = (void *)res;

	return 0;
}

static int _proc_export_debug_data(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	int ret;
	char cmd[256];
	char *path = NULL;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	memset(cmd, 0, sizeof(cmd));

	path = piscm->chart[tra].alloc_data;
	if (path == NULL || strlen(path) == 0) {
		g_warning("[%s, %d] path is NULL or empty", __FUNCTION__, __LINE__);
		return -1;
	}

	g_snprintf(cmd, sizeof(cmd), "sh /NFDVR/syslog_manager.sh export_logs %s", path);
	g_message("[%s, %d] cmd:%s", __FUNCTION__, __LINE__, cmd);

	ret = proxy_system(cmd, 1, 3);

	return ret;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

guint _scm_turnon_reboot_timer(SCM_T *piscm, int msec)
{
	DMSG(1, "TIMEOUT = %u[ms]", msec);
	return _scm_add_timeout(&iscm, msec, _proc_reset_system, piscm);
}

int _scm_turnoff_reboot_timer(SCM_T *piscm, guint id)
{
	DMSG(1, "");
	_scm_remove_timeout(piscm, id);
	return 0;
}

int _scm_work_booting_end(SCM_T *piscm, RESULT_E result)
{
	int cnt_disk = var_get_detected_disk_count();
	int cnt_err = _scm_get_err_cnt(piscm);

	DMSG(1, "");	
	if (result == SUCCESS) {
		nf_set_ddns_disk_count(cnt_disk);
		nf_sysman_time_check();
	}
	if (cnt_err > 0) _scm_reset_err_cnt(piscm);
	_scm_stop_unc_checking(piscm);

	return 0;
}

int _scm_work_service_start(SCM_T *piscm, TRANSACTION_E tra)
{
	_scm_work_rtl(piscm, tra);
	return 0;
}

int _scm_work_service_stop(SCM_T *piscm, TRANSACTION_E tra, RS_SRVSTOP_E rs)
{
	_scm_work_net_stop(piscm, tra, rs);
	return 0;
}

int _scm_work_record_stop(SCM_T *piscm, TRANSACTION_E tra)
{
	_scm_work_rec_stop(piscm, tra);
	return 0;
}

int _scm_work_shutdown(SCM_T *piscm, TRANSACTION_E tra)
{
	nf_dev_buzzer_off();
	nf_arch_manager_stop();

	sync();
	sync();
	sync();

	nf_sysman_sync();
	nf_sysman_set_normal_boot();

	scm_buzzer_on();

	scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);
	_scm_finalize_tra(piscm, tra, 0);
	return 0;
}

int _scm_increase_err_cnt(SCM_T *piscm, BERR_TYPE_E etype ,u8 pdid)
{
	nf_event_inc_nand_err_cnt(etype, pdid);
	return 0;
}

int _scm_get_err_cnt(SCM_T *piscm)
{
	int cnt = 0;
	nf_event_read_nand_err_cnt(NAND_ERR_ALL, &cnt,NULL);
	return cnt;
}

int _scm_set_err_cnt(SCM_T *piscm, BERR_TYPE_E etype , int cnt)
{
	nf_event_set_nand_err_cnt(etype, cnt);
	return 0;
}

int _scm_get_recovery_err_cnt(SCM_T *piscm)
{
	int cnt = 0;
	nf_event_read_nand_err_cnt(NAND_ERR_RECOVERY_ERROR, &cnt, NULL);
	return cnt;
}

unsigned char _scm_get_recovery_err_disk_pdid(SCM_T *piscm)
{
	unsigned char pdid; 
	nf_event_read_nand_err_cnt(NAND_ERR_RECOVERY_ERROR, NULL, &pdid);
	return pdid;
}

int _scm_get_smart_err_cnt(SCM_T *piscm)
{
	int cnt = 0;
	nf_event_read_nand_err_cnt(NAND_ERR_SMART_ERROR, &cnt,NULL);
	return cnt;
}

int _scm_get_disk_err_cnt(SCM_T *piscm)
{
	int cnt = 0;
	nf_event_read_nand_err_cnt(NAND_ERR_DISK_ERROR, &cnt,NULL);
	return cnt;
}

int _scm_reset_err_cnt(SCM_T *piscm)
{
	nf_event_clear_nand_err_cnt(NAND_ERR_ALL);
	return 0;
}

int _scm_reboot_system(SCM_T *piscm, REBOOT_REASON_E reason, int after_msec)
{
	piscm->reboot.reason = reason;
	DMSG(1, "REBOOT AFTER [%u]ms", after_msec);
	if (after_msec == 0) _proc_reset_system(piscm);
	else piscm->reboot.timer = _scm_turnon_reboot_timer(piscm, after_msec);
	return 0;
}

int _scm_cancel_reboot(SCM_T *piscm)
{
	if (piscm->reboot.timer) 
		_scm_turnoff_reboot_timer(piscm, piscm->reboot.timer); 
	return 0;
}

int _scm_work_igndisk_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "");
	return _work_igndisk_boot(piscm, tra);
}

int _scm_work_ipcam_fwup_mode(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "");
#if defined(_IPX_MODEL_UX)
	nf_ipcam_switch_mode(0, 0xff);
#endif
	_scm_finalize_tra(piscm, tra, 0);
	return 0;
}

int _scm_on_live_open(SCM_T *piscm)
{

#if defined(GUI_16CH_SUPPORT)
	if (!_is_valid_board(piscm)) _run_inv_board_scenario(piscm);
#endif

	return 0;
}

int _scm_on_board_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	// no action
	//
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected handlers
//

HANDLER int _scm_on_booting_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	DMSG(9, "");
	_req_ipcam_info_display(piscm);
	_req_odd_info_display(piscm);

    _scm_put_message(iNFY_128MB_FWUPGRADE_RESULT_CHECK, 0, 0, (void *)tra);

	return 0;
}

HANDLER int _scm_on_128MB_fwupgrade_result_check(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
    int res = 0;

#if defined(CONFIG_FWUPGRADE_SINGLE)
    res = nf_fw_update_check();
#endif

	DMSG(9, "");
	if (res == NF_FWUP_STATUS_UPGRADE_DONE || res == NF_FWUP_STATUS_UPGRADE_FAIL || res == NF_FWUP_STATUS_DB_FAIL) {
    	_scm_track_tra(piscm, IRPL_SCM_128MB_FWUP_RESULT, tra);
    	_scm_send_msg_to_viewer(IREQ_SCM_128MB_FWUP_RESULT, res);
    }
    else {
        _scm_put_message(iNFY_ENHANCED_PASSWORD_CHECK, 0, 0, (void *)tra);
    }

	return 0;
}

HANDLER int _scm_on_128MB_fwupgrade_result_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	if (tra == TRA_NONE) return -1;

	DMSG(9, "");

    if (pmsg->param == 0) {
#if defined(CONFIG_FWUPGRADE_SINGLE)
        nf_fw_update_clear();
#endif
        _scm_put_message(iNFY_ENHANCED_PASSWORD_CHECK, 0, 0, (void *)tra);
    }
    else {
        _scm_turnon_reboot_timer(piscm, 2000);
    }

	return 0;
}

HANDLER int _scm_on_enhanced_password_check(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	DMSG(9, "");
	if (_is_need_change_enhanced_password()) {
    	_scm_track_tra(piscm, IRPL_CHANGE_ENHANCED_PASSWORD, tra);
    	_scm_send_msg_to_viewer(IREQ_CHANGE_ENHANCED_PASSWORD, 0);
    }
    else {
        _scm_put_message(iNFY_WIZARD_CHECK, 0, 0, (void *)tra);
    }

	return 0;
}

HANDLER int _scm_on_enhanced_password_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	if (tra == TRA_NONE) return -1;

	DMSG(9, "");
    _scm_put_message(iNFY_WIZARD_CHECK, 0, 0, (void *)tra);
    
	return 0;
}

HANDLER int _scm_on_wizard_check(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    int need = 0;
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	DMSG(9, "");
	need = _is_need_new_wizard(piscm);
	DMSG(1, "PWD NEED = %d", need);

	if (need) _req_wizard_init(piscm, tra, need);
	else _scm_put_message(iNFY_DISKRAID_CHECK, 0, 0, (void *)tra);

	return 0;
}

HANDLER int _scm_on_wizard_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	if (tra == TRA_NONE) return -1;

	DMSG(9, "");
    _scm_put_message(iNFY_DISKRAID_CHECK, 0, 0, (void *)tra);
	return 0;
}

HANDLER int _scm_on_diskraid_check(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	DMSG(9, "");
	if (_is_need_check_raid()) _check_diskraid(piscm, tra);
	else _scm_put_message(iNFY_DISKINIT_START, 0, 0, (void *)tra);
	return 0;
}

HANDLER int _scm_on_diskinit_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	_start_disk(piscm, tra);
	return 0;
}

HANDLER int _scm_on_dirmode_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_ENTER_DIRMODE:
	case TRA_LEAVE_DIRMODE:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_cleanup_dmode_wrk(piscm);
		break;
	}
	return 0;
}

HANDLER int _scm_on_check_cable_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_CABLE_CHECK:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_cleanup_check_cable(piscm);
		break;
	}
	return 0;
}

HANDLER int _scm_on_export_debug_data_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_EXPORT_DEBUG_DATA:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_cleanup_export_debug_data(piscm);
		break;
	}
	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_bootup_system(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_BOOTUP_SYSTEM;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	DMSG(9, "");

	_scm_put_message(iNFY_BOOTING_START, 0, 0, (void *)tra);

/*
    if (scm_is_qc_mode() != 0)
    {
    	_scm_put_message(iNFY_BOOTING_START, 0, 0, (void *)tra);
    }
    else
    {
    	_scm_put_message(iRET_VFS_START_REC, 0, 0, (void *)tra);
    }
*/
	DMSG(9, "");
	return 0;
}

int scm_reboot_system(REBOOT_REASON_E reason, int after_msec)
{
	int owner = 0;

    if (reason == RR_REBOOT_MENU) {
        if (smt_get_service() != SMT_LIVE) return -1;
    }

	if (IUX_CALLER() == LOCAL_CALL) owner = 0;
	else if (IUX_CALLER() == WEB_CALL) owner = 1;

	_scm_send_msg_to_viewer(INFY_REBOOT_SYSTEM, owner);
	return _scm_reboot_system(&iscm, reason, after_msec);
}

int scm_cancel_reboot()
{
	return _scm_cancel_reboot(&iscm);
}

int scm_shutdown_system(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_SHUTDOWN;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	scm_put_log(SYSTEM_SHUTDOWN, 0, 0);
	_scm_work_service_stop(&iscm, tra, RS_SHUTDOWN);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_enter_cam_upgrade_mode(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_IPCAM_FWUP_MODE;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_work_service_stop(&iscm, tra, RS_CAM_FWUP);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_restart_service(IMSG ret_msg, RS_SRVSTOP_E rs)
{
	TRANSACTION_E tra = TRA_RESTART_SERVICE;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_push_notification(INFY_SVCRESTART_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, rs);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_boot_service(IMSG msgid)
{
	int owner = 0;

	if (IUX_CALLER() == LOCAL_CALL) owner = 0;
	else if (IUX_CALLER() == WEB_CALL) owner = 1;

	_scm_send_msg_to_viewer(msgid, owner);

	return 0;
}

int scm_buzzer_on()
{
	nf_dev_buzzer_on();
	return 0;
}

int scm_buzzer_off()
{
	nf_dev_buzzer_off();
	return 0;
}

int scm_cntl_beep_irda()
{
	gboolean onoff;
	
	onoff = nf_sysdb_get_bool("audio.remote");
	if(onoff) nf_dev_board_pp_cntl_buzzer_irda();

	return 0;
}

gboolean scm_get_video_resolution(gchar buf[32])
{
	gchar *pStr;

	if(buf) {
		pStr = nf_sysman_get_resolution();
		strcpy(buf, pStr);
		return TRUE;
	}
	return FALSE;
}

gboolean scm_get_video_output(gchar buf[32])
{
	gchar *pStr;

	if(buf) {
		pStr = nf_sysman_get_output_mode();
		strcpy(buf, pStr);
		return TRUE;
	}
	return FALSE;
}

int scm_get_hw_ver(char *buf, int len)
{
	char *ver = nf_api_param_hw_get_hwver();

	memset(buf, 0x00, len);

	if (!ver) return -1;
	if (strlen(ver) >= len) return -1;

	strcpy(buf, ver);
	return 0;
}

int scm_get_poe_port_info(NF_UTIL_POE_PORT_INFO *info)
{
	memset(info, 0x00, sizeof(NF_UTIL_POE_PORT_INFO));

	if (nf_dev_poe_get_info(info))
		return 0;

	return -1;
}

int scm_enter_direct_mode(IMSG ret_msg, int ch)
{
	TRANSACTION_E tra = TRA_ENTER_DIRMODE;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_ENTER_DMODE, (void *)tra };
	if (iscm.wrk_dmode) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = ch;

	iscm.wrk_dmode = wrk_create_worker(_proc_enter_dirmode, &cmmack);
	wrk_run_once_param(iscm.wrk_dmode, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_leave_direct_mode(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_LEAVE_DIRMODE;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_LEAVE_DMODE, (void *)tra };
	if (iscm.wrk_dmode) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_dmode = wrk_create_worker(_proc_leave_dirmode, &cmmack);
	wrk_run_once_param(iscm.wrk_dmode, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_enter_openmode_install()
{
	int owner = 0;

	if (IUX_CALLER() == LOCAL_CALL) owner = 0;
	else if (IUX_CALLER() == WEB_CALL) owner = 1;

	nf_openmode_stop_streaming();
	_scm_send_msg_to_viewer(INFY_OPENMODE_ENTER_INSTALL, owner);
	return 0;
}

int scm_leave_openmode_install()
{
	int owner = 0;

	if (IUX_CALLER() == LOCAL_CALL) owner = 0;
	else if (IUX_CALLER() == WEB_CALL) owner = 1;

	nf_openmode_finalize_installation();
	_scm_send_msg_to_viewer(INFY_OPENMODE_LEAVE_INSTALL, owner);
	return 0;
}

int scm_check_cable(int ch, int device, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_CABLE_CHECK;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CHECK_CABLE, (void *)tra };
	if (iscm.wrk_cable_check) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = ch;
	iscm.chart[tra].char_data = device;
	
	iscm.wrk_cable_check = wrk_create_worker(_proc_check_cable, &cmmack);
	wrk_run_once_param(iscm.wrk_cable_check, IMSG_NONE, &iscm, 0, 0);
	
    return 0;
}

int scm_check_cable_test_processing()
{
    if(iscm.wrk_cable_check) return 1;

    return 0;
}

int scm_is_qc_mode()
{
    if (nf_sysman_qcmode_is_enable() == 1)
        return 0;

	return -1;
}

int scm_is_clon_device()
{
	guint ret = 0;
	GValue ret_value = {0,};

    if (ivsc.dfunc.support_protect)
    {
    	if (nf_sysdb_get_key0("sys.info.guard.dev_block", &ret_value, NULL))
    	{
    		ret = g_value_get_boolean(&ret_value);
    		g_value_unset(&ret_value);
    	
    	} 
    	else {
    		ret = 0;
        }

// TEST
#if 0
    	if(nf_sysdb_get_key0("sys.info.sysid", &ret_value, NULL))
    	{
    	    gchar tmp[64];

    	    memset(tmp, 0x00, sizeof(tmp));
    		g_stpcpy(tmp, g_value_get_string(&ret_value));
    		g_value_unset(&ret_value);

    		if (strcmp(tmp, "CLON_DEV") == 0) {
                ret = 1;
    		}
    	}
#endif
    }
    
    return ret == 1 ? 0 : -1;
}

REBOOT_INFO_E scm_get_reboot_info(REBOOT_INFO_CONT *cont)
{
	gboolean ret;
	NF_SYSMAN_REBOOT_INFO info;

	if (nf_sysman_is_normal_boot()) return REB_NORMAL_BOOT;

	ret = nf_sysman_get_reboot_info(&info);
	if (!ret) return REB_NONE;
	 
	memcpy(cont->content, info.content, sizeof(cont->content));
	return (REBOOT_INFO_E)info.error_code;
}

int scm_is_exist_usb_file(char *file)
{
	MEDIA_INFO_T *media_info = 0;
	int i, media_cnt = 0;
	int is_exist = 0;
	char dir[128];
	char path[256];

	media_info = scm_new_media_list(&media_cnt);
	if (!media_cnt) return 0;

	for (i = 0; i < media_cnt; i++)
	{
		if (scm_get_media_type(media_info[i].id) == MTYPE_USB)
		{
    		memset(dir, 0x00, sizeof(dir));
    		if (scm_get_mounted_path(media_info[i].id, dir, 128) < 0) continue;

            memset(path, 0x00, sizeof(path));
    		g_sprintf(path, "%s/%s", dir, file);

		    if (ifn_is_file_exist(path)) {
				is_exist = 1;
				break;
		    }
		}
	}

	if (media_info) {
		scm_free_media_list(media_info);
	}

    return is_exist;
}

int scm_is_enable_capture_mode()
{
	if (ifn_is_file_exist("/NFDVR/data/itx_enable_capture_mode.txt")) return 1;

	return 0;
}

int scm_is_enable_choissi_debug()
{
	char *tmp_testmail;

	tmp_testmail = nf_sysdb_get_str_nocopy("net.email.testmail");
	if (strcmp(tmp_testmail, "choissi@debug.com") == 0) return 1;

	return 0;
}

int scm_regi_password_to_hdd()
{
    gchar pw[16];
    gboolean ret;

    DMSG(1, "");
    
    memset(pw, 0x00, sizeof(pw));
    
    if (!nf_filesystem_is_online()) return -1;
    if (!DAL_get_user_passwd(pw, 0)) return -1;

    ret = nf_set_passwd(pw, strlen(pw));

    DMSG(1, "ret : %d", ret);

    return ret;
}

int scm_get_gpu_mode_function()
{
	g_message("%s, %d, gpu_mode:%d", __FUNCTION__, __LINE__, _gpu_mode);
	return _gpu_mode;
}

int scm_set_gpu_mode_function(GPU_MODE_E mode)
{
	g_message("%s, %d, mode:%d", __FUNCTION__, __LINE__, mode);	
	g_message("%s, %d, pre_gpu_mode:%d", __FUNCTION__, __LINE__, _gpu_mode);	

	if (mode == GPU_NONE) 
	{
		nf_live_fisheye_block(1);
		nf_va_object_detector_set_running(0);
	}
	else if (mode == GPU_FISHEYE)
	{
		nf_va_object_detector_set_running(0);
		nf_live_fisheye_block(0);
	}
	else if (mode == GPU_DLVA)
	{
		nf_live_fisheye_block(1);
		nf_va_object_detector_set_running(1);
	}

	if (_gpu_mode != mode) {
		_scm_send_msg_to_viewer(INFY_SWITCH_UX_GPU_MODE, mode);
		_gpu_mode = mode;
	}

	g_message("%s, %d, post_gpu_mode:%d", __FUNCTION__, __LINE__, _gpu_mode);	
	return 0;
}

int scm_dlva_detector_live_start_chmask(guint chmask)
{
	gint i;
	guint opt_flag = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (_dlva_detector_setup[i] > 0) {
			g_message("%s, %d, remained ref cnt - ch:%d, refcnt:%d", __FUNCTION__, __LINE__, i, _dlva_detector_setup[i]);
			return -1;	
		}
	}

	if (IUX_CALLER() == LOCAL_CALL) g_message("LOCAL_CALL %s, %d, chmask:%08X", __FUNCTION__, __LINE__, chmask);
	else g_message("WEB_CALL %s, %d, chmask:%08X", __FUNCTION__, __LINE__, chmask);

	nf_va_object_detector_set_event(1);
	nf_va_object_detector_set_ch_mask(chmask, opt_flag);
	return 0;
}

int scm_dlva_detector_setup_open_channel(guint ch)
{
	guint chmask = 0;
	gint i;
	guint opt_flag = 0;

	if (ch >= GUI_CHANNEL_CNT) return -1;

	_dlva_detector_setup[ch] += 1;
	if (IUX_CALLER() == LOCAL_CALL) g_message("LOCAL_CALL %s, %d, ch:%d, refcnt:%d", __FUNCTION__, __LINE__, ch, _dlva_detector_setup[ch]);
	else g_message("WEB_CALL %s, %d, ch:%d, refcnt:%d", __FUNCTION__, __LINE__, ch, _dlva_detector_setup[ch]);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (_dlva_detector_setup[i] > 0) chmask |= (1 << i);
	}

	opt_flag |= NF_VA_OBJ_SETUP;

	g_message("%s, %d, chmask:%08X", __FUNCTION__, __LINE__, chmask);
	nf_va_object_detector_set_event(0);
	nf_va_object_detector_set_ch_mask(chmask, opt_flag);
	return 0;
}

int scm_dlva_detector_setup_close_channel(guint ch)
{
	guint chmask = 0;
	gint i;
	guint opt_flag = 0;

	if (ch >= GUI_CHANNEL_CNT) return -1;

	if (_dlva_detector_setup[ch] > 0)
		_dlva_detector_setup[ch] -= 1;

	if (IUX_CALLER() == LOCAL_CALL) g_message("LOCAL_CALL %s, %d, ch:%d, refcnt:%d", __FUNCTION__, __LINE__, ch, _dlva_detector_setup[ch]);
	else g_message("WEB_CALL %s, %d, ch:%d, refcnt:%d", __FUNCTION__, __LINE__, ch, _dlva_detector_setup[ch]);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (_dlva_detector_setup[i] > 0) chmask |= (1 << i);
	}

	opt_flag |= NF_VA_OBJ_SETUP;

	g_message("%s, %d, chmask:%08X", __FUNCTION__, __LINE__, chmask);
	nf_va_object_detector_set_event(0);
	nf_va_object_detector_set_ch_mask(chmask, opt_flag);
	return 0;
}

int scm_reset_system()
{
	scm_stop_fs_urgent();
	_reset_board();
	return 0;
}

int scm_export_debug_data(IMSG ret_msg, char *usb_path)
{
	TRANSACTION_E tra = TRA_EXPORT_DEBUG_DATA;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_EXPORT_DEBUG_DATA, (void *)tra };

	if (strlen(usb_path) <= 0) return -1;
	if (iscm.wrk_export_debug_data) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(char) * (strlen(usb_path) + 1));
	if (!iscm.chart[tra].alloc_data) return -1;

	memset(iscm.chart[tra].alloc_data, 0x00, sizeof(iscm.chart[tra].alloc_data));
	g_strlcpy((char *)iscm.chart[tra].alloc_data, usb_path, strlen(usb_path) + 1);

	iscm.wrk_export_debug_data = wrk_create_worker(_proc_export_debug_data, &cmmack);
	wrk_run_once_param(iscm.wrk_export_debug_data, IMSG_NONE, &iscm, 0, 0);
	
	return 0;
}
