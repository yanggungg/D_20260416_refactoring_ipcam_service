/*
 * scm_videofs.c
 * 	- video filesystem service
 *	- dependency :
 *		var, usr
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 6, 2011
 *
 */

#include <glib.h>
#include "nf_api_disk.h"
#include "scm.h"
#include "nfdal.h"
#include "scm_internal.h"
#include "ix_mem.h"
#include "vfs.h"
#include "evt.h"
#include "ssm.h"


/*
 * file system sequencial operation
 *

 <STOP>
    network stop --> rec stop --> filesystem stop

 <START>
    apply rec time limit --> filesystem start --> rec start --> network start


 */   

 

//DECLARE DBG_SYSTEM

#define DBG_LEVEL		9
//#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"SCM_VIDEOFS"

#define DCNT		16		// disk count

#define IS_SYSTEM_REMOVED(cap, i) 	(cap.disk_unit[i].state & NF_DISK_INFO_FLAG_SYSTEM_REMOVED)
#define IS_CONFLICTED(cap, i) 	(cap.disk_unit[i].state & NF_DISK_INFO_FLAG_SYSTEM_CONFLICTED)
#define WAS_EXIST(cap, i) 		(cap.disk_unit[i].state & NF_DISK_INFO_FLAG_EXIST)
#define IS_NEED_FORMAT(cap, i)	(!(cap.disk_unit[i].state & NF_DISK_INFO_FLAG_FORMATTED))
#define IS_VFS_SUCCESS()		(result == VSR_SUCCESS)

#define IS_SUCCESS()		(result == 0)
#define	RETRY_CNT			3

#define SMART_NOERROR		0
#define SMART_WARNING		2
#define SMART_FAIL			3

////////////////////////////////////////////////////////////
//
// private data type
//


////////////////////////////////////////////////////////////
//
// private variables
//



////////////////////////////////////////////////////////////
//
// private functions
//

static int _scm_cleanup_unc_check(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_unc);
	piscm->wrk_unc = 0;
	return 0;
}

static int _proc_detect_uncerr(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	guint key = 0;
	int result = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;
	CMMPORT cmmpt;

	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	cmmpt = cmmack.cmmpt;

	if (piscm->st_unc_err) return -1;
	nf_smart_get_boot_unc(&key, NULL);
	DMSG(0, "BOOT UNC CHECK : %d\n", key);		

	if (key != 0) {
		DMSG(1, "UNC error was detected (0x%x). The system will be reboot soon.\n", key);
		cmm_send_message(cmmpt, INFY_CFRM_UNC_ERROR_DETECTED, key, 0, 0);
		scm_reboot_system(RR_UNC_ERROR, 30000);
		piscm->st_unc_err = 1;
		result = 1;
	}

	return result;
}

static int _get_disk_data(NF_DISK_INFO *pdi)
{
	gboolean ret;
	ret = nf_disk_get_info(pdi, NULL);
	if (!ret) return -1; 
	return 0;
}

static int _make_disk_cap_info(STRG_TYPE_E type, NF_DISK_INFO *ndi, DISK_CAPINFO_T *cap)
{
	int i;
	int pn;
	if(ndi == NULL) return -1;

	memset(cap, 0x00, sizeof(DISK_CAPINFO_T));

	cap->tsize = 0;
	DMSG(1, "DISK TYPE = [%d]\n", type);
	cap->tdisk_count = ndi->group_disks[type];
	DMSG(1, "DISK COUNT = [%d]\n", cap->tdisk_count);
	for (i = 0; i < DCNT; ++i) {
		pn = ndi->ucPortNo[type][i];
		if (pn == 0xff) continue;
		
		if( !(ndi->disk_state[type][i] & NF_DISK_INFO_FLAG_MIRROR) )
			cap->disk_unit[pn].valid = DISK_VALID;

		memset(cap->disk_unit[pn].model, 0x00, 32);
		memset(cap->disk_unit[pn].serial, 0x00, 32);
		memcpy(cap->disk_unit[pn].model, ndi->model_num[type][i], 12);	// just 12 bytes are used in IPX
		memcpy(cap->disk_unit[pn].serial, ndi->sSerialNo[type][i], 22);	// just 22 bytes are used in IPX
		cap->disk_unit[pn].model[12] = 0;

		ifn_strtrim(cap->disk_unit[pn].model);
		ifn_strtrim(cap->disk_unit[pn].serial);
		cap->disk_unit[pn].sys_disk = (ndi->model_num[type][i][30] == 'S');
		cap->disk_unit[pn].state = ndi->disk_state[type][i];

		if( !(ndi->disk_state[type][i] & NF_DISK_INFO_FLAG_MIRROR) ) {
			cap->disk_unit[pn].exist = ndi->disk_state[type][i] & NF_DISK_INFO_FLAG_EXIST;
			cap->disk_unit[pn].id = ndi->model_num[type][i][28];
			cap->disk_unit[pn].size = ndi->disk_size[type][i];
			cap->disk_unit[pn].use = IN_USE;
		
			cap->tsize += cap->disk_unit[pn].size;
		}
	}
	DMSG(1, "DISK SIZE = [%llu]\n", cap->tsize);
	return 0;
}

static int _make_disk_rec_info(STRG_TYPE_E type, RECDATA_HIDE_E hide, DISK_RECINFO_T *rec)
{
	NF_DISK_REC_DISK_TIME rt;
	int i;
	int pn;
	gboolean ret;

	memset(rec, 0x00, sizeof(DISK_RECINFO_T));

	for (i = 0; i < DCNT; ++i) {
		ret = nf_disk_rec_disk_time(type, i, hide, &rt, NULL);
		if (!ret) continue;
		if (rt.ucPortNo[type][i] == 0xff) continue;
		pn = rt.ucPortNo[type][i];

		rec->disk_unit[pn].valid = DISK_VALID;
		if (rec->trec_start == 0) rec->trec_start = rt.min_time;
		if (rt.min_time < rec->trec_start) rec->trec_start = rt.min_time;
		if (rt.max_time > rec->trec_end) rec->trec_end = rt.max_time;

		rec->disk_unit[pn].rec_start = rt.min_time;
		rec->disk_unit[pn].rec_end = rt.max_time;;
	}
	return 0;
}

static int _make_disk_smart_info(STRG_TYPE_E type, DISK_SMARTINFO_T *smart)
{
	NF_SMART_DISK_INFO smt;
	int i;
	int pn;
	gboolean ret;

	memset(smart, 0x00, sizeof(DISK_SMARTINFO_T));

	for (i = 0; i < DCNT; ++i) {
		ret = nf_smart_get_info (type, i, &smt, NULL);
		if (!ret) continue;
		if (smt.ucPortNo[type][i] == 0xff) continue;
		pn = smt.ucPortNo[type][i];

		smart->disk_unit[pn].valid = DISK_VALID;
		smart->disk_unit[pn].update_time = smt.update_time;
		smart->disk_unit[pn].disk_status = smt.disk_status;
		smart->disk_unit[pn].raw_read_error_rate = smt.raw_read_error_rate;
		smart->disk_unit[pn].spin_up_time = smt.spin_up_time;
		smart->disk_unit[pn].reallocated_sector_ct = smt.reallocated_sector_ct;
		smart->disk_unit[pn].seek_error_rate = smt.seek_error_rate;		
		smart->disk_unit[pn].reallocation_event_ct = smt.reallocation_event_ct;
		smart->disk_unit[pn].current_pending_sector = smt.current_pending_sector;
		smart->disk_unit[pn].offline_uncorrectable = smt.offline_uncorrectable;
		smart->disk_unit[pn].start_stop_cnt = smt.start_stop_cnt;
		smart->disk_unit[pn].power_on_hours = smt.power_on_hours;
		smart->disk_unit[pn].spin_retry_cnt = smt.spin_retry_cnt;
		smart->disk_unit[pn].power_cycle_cnt = smt.power_cycle_cnt;
		smart->disk_unit[pn].temperature_celsius = smt.temperature_celsius;
		smart->disk_unit[pn].temperature_status = smt.temperature_status;

	}
	return 0;
}

static int _make_disk_db(NF_DISK_INFO *pdi, DISK_DB_T *ddb)
{
	ddb->added_disks = pdi->added_disks;
	ddb->removed_disks = pdi->removed_disks;

	_make_disk_cap_info(INTERNAL, pdi, &ddb->cap[INTERNAL]);
	_make_disk_cap_info(EXTERNAL, pdi, &ddb->cap[EXTERNAL]);

	// below codes are not excuted in booting due to slow latency
//	_make_disk_rec_info(INTERNAL, pdi, &ddb->rec[INTERNAL]);
//	_make_disk_rec_info(EXTERNAL, pdi, &ddb->rec[EXTERNAL]);

	return 0;
}

static int _proc_get_disk_info(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    SCM_T *piscm = (SCM_T *)pmsg->param;
    int ack_ret = 0;
    CMMACK_T cmmack;
    TRANSACTION_E tra;
    STRG_TYPE_E type;
    DISK_DB_T *pddb = imalloc(sizeof(DISK_DB_T));

    DMSG(9, "");
    wrk_get_cmmack(wrkid, &cmmack);
    tra = (TRANSACTION_E)cmmack.data;

    for (type = 0; type < 2; type++)
    {
        scm_get_disk_capinfo(type, &pddb->cap[type]);
        scm_get_disk_recinfo(type, &pddb->rec[type]);
        scm_get_disk_smartinfo(type, &pddb->smart[type]);
    }
    
    if (var_get_supported_raid())
    {
        if (scm_get_disk_raidinfo(&iscm.ddb->raid) != -1)
        {
        	memcpy(&pddb->raid, &iscm.ddb->raid, sizeof(DISK_RAIDINFO_T));
        }
    }
    
    piscm->chart[tra].result = pddb;

    return ack_ret;
}

static int _is_conflicted(DISK_DB_T *ddb)
{
	int i;
	for (i = 0; i < DCNT; ++i) {
		if (IS_CONFLICTED(ddb->cap[0], i)) return 1;
		if (IS_CONFLICTED(ddb->cap[1], i)) return 1;
	}

	return 0;
}

static int _is_added(DISK_DB_T *ddb)
{
	return (ddb->added_disks > 0);
}

static int _is_removed(DISK_DB_T *ddb)
{
	return (ddb->removed_disks > 0);
}

static int _is_added_n_removed(DISK_DB_T *ddb)
{
	return (_is_added(ddb) && _is_removed(ddb));
}

static int _is_need_format_all(DISK_DB_T *ddb)
{
	int i;
	for (i = 0; i < DCNT ; ++i) {
		if (!WAS_EXIST(ddb->cap[0], i)) continue;
		if (IS_NEED_FORMAT(ddb->cap[0], i)) return 1;
	}

	for (i = 0; i < DCNT ; ++i) {
		if (!WAS_EXIST(ddb->cap[1], i)) continue;
		if (IS_NEED_FORMAT(ddb->cap[1], i)) return 1;
	}

	return 0;
}

static int _is_system_removed(DISK_DB_T *ddb)
{
	int i;
	for (i = 0; i < DCNT; ++i) {
		if (IS_SYSTEM_REMOVED(ddb->cap[0], i)) return 1;
		if (IS_SYSTEM_REMOVED(ddb->cap[1], i)) return 1;
	}

	return 0;
}

static DISK_CONF_E _get_disk_conf_data(DISK_DB_T *ddb)
{
	if (_is_system_removed(ddb)) return DC_SYSTEM_REMOVED;
	if (_is_conflicted(ddb)) return DC_CONFLICTED;
	if (_is_added_n_removed(ddb)) return DC_ADDED_N_REMOVED;
	if (_is_removed(ddb)) return DC_REMOVED;
	if (_is_added(ddb)) return DC_ADDED;
	if (_is_need_format_all(ddb)) return DC_NEED_FORMAT_ALL;

	// FIXME: temporary
	//return DC_NEED_FORMAT_ALL;
	return DC_NOCHANGE;
}

static int _cleanup_get_disk_info(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_disk_info);
	piscm->wrk_disk_info = 0;
	return 0;
}

static int _wait_to_vfs_cleanup()
{
	DMSG(1, "Resource cleanup start\n");
	while (vfs_is_running()) usleep(10);
	DMSG(1, "Resource cleanup end\n");
	return 0;
}

static int _scm_change_wmode_to_ow(SCM_T *piscm)
{
	DiskManageData disk_data;
	memset(&disk_data, 0, sizeof(DiskManageData));
	DAL_get_diskManage_data(&disk_data);

	disk_data.overWrite = NF_DISK_WRITE_MODE_OVERWRITE;
	disk_data.timeLimit = 0;	
	disk_data.timeType = 0;

	DAL_set_diskManage_data(&disk_data);								
	DAL_save_db("disk");

	_scm_change_wmode(piscm, NF_DISK_WRITE_MODE_OVERWRITE);
	return 0;
}

static int _scm_change_wmode_to_wo(SCM_T *piscm)
{
	DiskManageData disk_data;
	memset(&disk_data, 0, sizeof(DiskManageData));
	DAL_get_diskManage_data(&disk_data);

	if (disk_data.overWrite == NF_DISK_WRITE_MODE_WRITEONCE) return -1;

	disk_data.overWrite = NF_DISK_WRITE_MODE_WRITEONCE;
	disk_data.timeLimit = 0;		
	disk_data.timeType = 0;

	DAL_set_diskManage_data(&disk_data);								
	DAL_save_db("disk");

	_scm_change_wmode(piscm, NF_DISK_WRITE_MODE_WRITEONCE);
	return 0;
}

static int _work_fs_start(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_START_FS, (void *)tra };

	vfs_start_fs(&cmmack);
	return 0;
}

static int _work_fs_stop(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_STOP_FS, (void *)tra };

	vfs_stop_fs(&cmmack);
	return 0;
}

static int _work_format(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_FORMAT_STORAGE, (void *)tra };
	int mode = piscm->chart[tra].int_data; 

	vfs_format_storage(&cmmack, mode);
	return 0;
}

static int _work_broken_raid(SCM_T *piscm, TRANSACTION_E tra)
{
    _scm_send_msg_to_viewer(INFY_BROKEN_RAID_START, 0);
//	scm_send_to_viewer(INFY_BROKEN_RAID_START, 0, 0, 0);

    _scm_work_delete_raid(piscm, TRA_NONE);
	_scm_put_message(iRET_VFS_BROKEN_RAID, 0, 0, (void *)tra);
	return 0;
}

static int _work_erase_ch(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_ERASE_CH, (void *)tra };
	ERASE_CHINFO_T *erase_info = piscm->chart[tra].alloc_data;

	vfs_erase_ch(&cmmack, erase_info);
	return 0;
}

static int _work_rec_stop(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_STOP_REC, (void *)tra };

	vfs_stop_record(&cmmack);
	return 0;
}

static int _work_rec_start(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_START_REC, (void *)tra };

	vfs_start_record(&cmmack);
	return 0;
}

static int _work_net_start(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_START_NET, (void *)tra };

	vfs_start_network(&cmmack);
	return 0;
}

static int _work_data_delete(SCM_T *piscm, TRANSACTION_E tra, time_t delete_time)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_DELETE_DATA, (void *)tra };

	vfs_delete_data(&cmmack, delete_time);
	return 0;
}

static int _work_rtl(SCM_T *piscm, TRANSACTION_E tra)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_SET_RTL, (void *)tra };
	guint rtl = _scm_get_rtl_state();
	
	DMSG(9, "");
	vfs_set_rtl(&cmmack, rtl);
	return 0;
}

static int _work_net_stop(SCM_T *piscm, TRANSACTION_E tra, int reason)
{
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_STOP_NET, (void *)tra };
//	int reason = piscm->chart[tra].int_data;

	vfs_stop_network(&cmmack, reason);
	return 0;
}

static int _work_panicrec_start(SCM_T *piscm)
{
    if (!vfs_start_panic_record()) return -1;

    _scm_send_msg_to_viewer(INFY_PANIC_REC_STATUS, 1);
	return 0;
}

static int _work_panicrec_stop(SCM_T *piscm)
{
    if (!vfs_stop_panic_record()) return -1;

    _scm_send_msg_to_viewer(INFY_PANIC_REC_STATUS, 0);
	return 0;
}

static int _work_panicrec_toggle(SCM_T *piscm)
{   
    if (!vfs_toggle_panic_record()) return -1;
/*    
    if (scm_get_panic_record()) _scm_send_msg_to_viewer(INFY_PANIC_REC_STATUS, 1);
    else                        _scm_send_msg_to_viewer(INFY_PANIC_REC_STATUS, 0);
*/    
	return 0;
}

static int _work_wmode_change(SCM_T *piscm, int mode)
{
	return vfs_filesystem_change_wmode(mode);
}

static int _work_wmode_get(SCM_T *piscm, NF_DISK_WRITE_MODE_E *w_mode)
{
	return vfs_filesystem_get_wmode(w_mode);
}

static int _start_rcvr_timer(SCM_T *piscm)
{
	int timeout = piscm->cf.rcvr_timeout;
	if (piscm->rcvr_timer) tmr_free(piscm->rcvr_timer);
	piscm->rcvr_timer = tmr_new(timeout, CMMPT_SCM, iNFY_RCVR_EXPIRED);
	tmr_start(piscm->rcvr_timer);
	return 0;
}

static unsigned int _get_adjusted_rtl(unsigned int rtl)
{
	int cnt = 0;
	int year, month, day;
	time_t timet;
	int adj = 0;

	if (!_scm_is_rtl_movable()) return rtl;

	timet = time(0);	
	ifn_get_local_day(timet, &year, &month, &day);
	adj = scm_get_rtl_skip_holiday(rtl, year, month, day);

	g_assert(adj != -1);
	return adj;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_is_rtl_movable()
{
	DiskManageData disk_data;
	guint rtl = 0;
	memset(&disk_data, 0, sizeof(DiskManageData));

	DAL_get_diskManage_data(&disk_data);
	if (disk_data.overWrite == NF_DISK_WRITE_MODE_WRITEONCE) return 0;
	if (disk_data.timeType == 0 || disk_data.timeLimit == 0) return 0;
	if (disk_data.custom_cal == 0) return 0;

	return 1;
}

int _scm_change_wmode_by_db(SCM_T *piscm)
{
	int ret;
	DiskManageData dmdata;
	NF_DISK_WRITE_MODE_E wmode;
	memset(&dmdata, 0, sizeof(DiskManageData));

	DAL_get_diskManage_data(&dmdata);
	ret = _scm_get_wmode(piscm, &wmode);
	if (ret == -1) return -1;

	DMSG(1, "disk's mode = %d, DB's mode = %d\n", wmode, dmdata.overWrite);
	if (wmode == dmdata.overWrite) return 0; 
	_scm_change_wmode(piscm, dmdata.overWrite);
	return 0;
}

int _scm_req_disk_confirm(SCM_T *piscm, TRANSACTION_E tra, DISK_CONF_E disk_conf)
{
    int qc_mode = scm_is_qc_mode();
    int encoder_mode = var_get_running_encoder_mode();

	switch (disk_conf) {
	case DC_ERROR:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_ERROR, tra);
		_scm_send_msg_to_viewer(IREQ_BOOT_ERROR, 0);
		break;
	case DC_NODISK:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_NODISK, tra);
		if (encoder_mode) _scm_put_message(IRPL_BOOT_NODISK, DISK_NOSYNC, 0, 0);
		else _scm_send_msg_to_viewer(IREQ_BOOT_NODISK, 0);
		break;
	case DC_CONFLICTED:
		DMSG(9, "");
		_scm_put_log(HDD_REMOVED, 0, "HDD CONFILICTED");
		_scm_track_tra(piscm, IRPL_BOOT_DISK_CONFLICT, tra);
        if (qc_mode != 0) _scm_send_msg_to_viewer(IREQ_BOOT_DISK_CONFLICT, 0);
		else _scm_put_message(IRPL_BOOT_DISK_CONFLICT, DISK_SYNC, 0, 0);
		break;
	case DC_ADDED:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_DISK_ADDED, tra);
		if (qc_mode != 0) _scm_send_msg_to_viewer(IREQ_BOOT_DISK_ADDED, 0);
		else _scm_put_message(IRPL_BOOT_DISK_ADDED, DISK_SYNC, 0, 0);		
		break;
	case DC_REMOVED:
		DMSG(9, "");
		_scm_put_log(HDD_REMOVED, 0, "HDD REMOVED BY USER");
		_scm_track_tra(piscm, IRPL_BOOT_DISK_REMOVED, tra);
		if (qc_mode != 0) _scm_send_msg_to_viewer(IREQ_BOOT_DISK_REMOVED, 0);
		else _scm_put_message(IRPL_BOOT_DISK_REMOVED, DISK_SYNC, 0, 0);		
		break;
	case DC_ADDED_N_REMOVED:
		DMSG(9, "");
		_scm_put_log(HDD_REMOVED, 0, "HDD ADD&REMOVED BY USER");
		_scm_track_tra(piscm, IRPL_BOOT_DISK_CHANGED, tra);
		if (qc_mode != 0) _scm_send_msg_to_viewer(IREQ_BOOT_DISK_CHANGED, 0);
		else _scm_put_message(IRPL_BOOT_DISK_CHANGED, DISK_SYNC, 0, 0);		
		break;
	case DC_NEED_FORMAT_ALL:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_DISK_NEED_FORMAT, tra);
		_scm_send_msg_to_viewer(IREQ_BOOT_DISK_NEED_FORMAT, 0);
		break;
	case DC_FORMAT_N_RCVR:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_FORMAT_RCVR, tra);
		_scm_send_msg_to_viewer(IREQ_BOOT_FORMAT_RCVR, 0);
		break;
	case DC_ENFORCE_FORMAT:
		DMSG(9, "");
		_scm_track_tra(piscm, IRPL_BOOT_ENFORCE_FORMAT, tra);
		_scm_send_msg_to_viewer(IREQ_BOOT_ENFORCE_FORMAT, 0);
		break;
	case DC_SYSTEM_REMOVED:
		DMSG(9, "");
		_scm_put_log(HDD_REMOVED, 0, "SYSTEM HDD REMOVED BY USER");
		_scm_track_tra(piscm, IRPL_BOOT_SYSTEM_DISK_REMOVED, tra);
		if (qc_mode != 0) _scm_send_msg_to_viewer(IREQ_BOOT_SYSTEM_DISK_REMOVED, 0);
		else _scm_put_message(IRPL_BOOT_SYSTEM_DISK_REMOVED, DISK_SYNC, 0, 0);		
		break;
	}
	
	return 0;
}

guint _scm_get_rtl_state()
{
	DiskManageData disk_data;
	guint rtl = 0;
	memset(&disk_data, 0, sizeof(DiskManageData));

	DAL_get_diskManage_data(&disk_data);
	if (disk_data.overWrite == NF_DISK_WRITE_MODE_WRITEONCE) rtl = 0;
	else rtl = disk_data.timeLimit;
	
	rtl = _get_adjusted_rtl(rtl);	
	return rtl;
}

int _scm_save_disk_info(SCM_T *piscm)
{
	int cnt;
	if (!piscm->ddb) cnt = 0;
	else cnt = piscm->ddb->cap[0].tdisk_count + piscm->ddb->cap[1].tdisk_count;
	var_set_disk_count(cnt, cnt);
	nf_set_ddns_disk_count(cnt);
	return 0;
}

int _scm_get_disk_space(SCM_T *piscm, guint64 *total, guint64 *used)
{
	guint64 dSize = 0;
	guint64 dfillSize = 0;
	guint64 tfillSize = 0;
	guint64 total_used_amount = 0;
    gboolean disk_check_ret = 0;
	int ret;
	int i;

	*total = 0;
	*used = 0;
	if (!piscm->ddb) return -1;

	if (disk_check_ret == TRUE) {
		for (i = 0; i < DCNT; i++) {
			if (piscm->ddb->cap[0].disk_unit[i].state & 0x01)
				dSize += piscm->ddb->cap[0].disk_unit[i].size;
		}    

		for (i = 0; i < DCNT; i++) {
			if (piscm->ddb->cap[1].disk_unit[i].state & 0x01)
				dSize += piscm->ddb->cap[1].disk_unit[i].size;
		}    
		*total = dSize / 1024;

		// "0, 0" no means
		ret = nf_disk_get_usage(0, 0, &dfillSize, &tfillSize, NULL);
		total_used_amount += dfillSize;
		if (ret == TRUE) *used = total_used_amount / 1024;
		else *used = 0;
	}
	else {
		*total = 0;
		*used = 0;
	}
	return 0;
}

int _scm_load_disk_info(SCM_T *piscm)
{
	NF_DISK_INFO ndi;

	if (piscm->ddb) ifree(piscm->ddb);
	piscm->ddb = imalloc(sizeof(DISK_DB_T));
	DMSG(9, "");
	if (_get_disk_data(&ndi) < 0) {
		ifree(piscm->ddb);
		piscm->ddb= NULL;
		DMSG(9, "");
		return -1;
	}
	DMSG(9, "");

	_make_disk_db(&ndi, piscm->ddb);
	return 0;
}

int _scm_unload_disk_info(SCM_T *piscm)
{
	if (piscm->ddb) ifree(piscm->ddb);
	piscm->ddb = NULL;
	return 0;
}

DISK_CONF_E _scm_get_disk_change_info(SCM_T *piscm)
{
	if (!iscm.ddb) return DC_ERROR;
	return _get_disk_conf_data(iscm.ddb);
}

int _scm_stop_rcvr_timer(SCM_T *piscm)
{
	if (piscm->rcvr_timer) tmr_free(piscm->rcvr_timer);
	piscm->rcvr_timer = 0;
	return 0;
}

int _scm_retouch_rcvr_timer(SCM_T *piscm)
{
	if (piscm->rcvr_timer) tmr_reset(piscm->rcvr_timer);
	return 0;
}

int _scm_has_smart_error()
{
	int i;
	DISK_SMART_T *tmp;

	if (!iscm.ddb) return 0;

	_make_disk_smart_info(INTERNAL, &iscm.ddb->smart[INTERNAL]);
	for (i = 0; i < DCNT; ++i) {
		tmp = &iscm.ddb->smart[INTERNAL].disk_unit[i];
		DMSG(9, "STATUS = %d\n", tmp->disk_status);
		if (tmp->disk_status == SMART_WARNING) return SMART_WRN;
		if (tmp->disk_status == SMART_FAIL) return SMART_ERR;
	}

	_make_disk_smart_info(EXTERNAL, &iscm.ddb->smart[EXTERNAL]);
	for (i = 0; i < DCNT; ++i) {
		tmp = &iscm.ddb->smart[EXTERNAL].disk_unit[i];
		DMSG(9, "STATUS = %d\n", tmp->disk_status);
		if (tmp->disk_status == SMART_WARNING) return SMART_WRN;
		if (tmp->disk_status == SMART_FAIL) return SMART_ERR;
	}

	DMSG(9, "");
	return 0;
}

int _scm_start_unc_checking(SCM_T *piscm)
{
	TRANSACTION_E tra = TRA_UNC_CHECK;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CHECK_UNC, (void *)tra };
	if (piscm->wrk_unc) return 0;

	DMSG(9, "");
	_scm_ready_tra(piscm, tra, IMSG_NONE, NULL);
	piscm->wrk_unc = wrk_create_worker(_proc_detect_uncerr, &cmmack);
	wrk_run_loop(piscm->wrk_unc, IMSG_NONE, piscm, 0, 0);
	return 0;
}

int _scm_stop_unc_checking(SCM_T *piscm)
{
	if (!piscm->wrk_unc) return -1;
	wrk_stop(piscm->wrk_unc);
	wrk_wait_for_stop(piscm->wrk_unc);
	DMSG(1, "BOOT UNC CHECK COMPLETED\n");
	return 0;
}

int _scm_work_smart_boot(SCM_T *piscm, TRANSACTION_E tra)
{
	int ret;
	DMSG(9, "");
	ret = _scm_has_smart_error();
	DMSG(9, "SMART ERROR = %d\n", ret);
	_scm_put_message(iRET_SMART_CHECK, ret, 0, (void *)tra);

	return 0;
}

int _scm_work_rtl(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_SET_RTL, 0, 0, (void *)tra);
	else _work_rtl(piscm, tra);
	return 0;
}

int _scm_work_net_stop(SCM_T *piscm, TRANSACTION_E tra, int reason)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_STOP_NET, 0, 0, (void *)tra);
	else _work_net_stop(piscm, tra, reason);
	return 0;
}

int _scm_work_format(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_FORMAT_STORAGE, 0, 0, (void *)tra);
	else _work_format(piscm, tra);
	return 0;
}

int _scm_work_erase_ch(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_ERASE_CH, 0, 0, (void *)tra);
	else _work_erase_ch(piscm, tra);
	return 0;
}

int _scm_work_rec_start(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb && !var_get_running_encoder_mode()) _scm_put_message(iRET_VFS_START_REC, 0, 0, (void *)tra);
	else _work_rec_start(piscm, tra);
	return 0;
}

int _scm_work_rec_stop(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_STOP_REC, 0, 0, (void *)tra);
	else _work_rec_stop(piscm, tra);
	return 0;
}

int _scm_work_fs_stop(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_STOP_FS, 0, 0, (void *)tra);
	else _work_fs_stop(piscm, tra);
	return 0;
}

int _scm_work_fs_start(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb) _scm_put_message(iRET_VFS_START_FS, 0, 0, (void *)tra);
	else _work_fs_start(piscm, tra);
	return 0;
}

int _scm_work_net_start(SCM_T *piscm, TRANSACTION_E tra)
{
	if (!piscm->ddb || (scm_is_clon_device() == 0)) 
	{
		_scm_put_message(iRET_VFS_START_NET, 0, 0, (void *)tra);
	}
	else _work_net_start(piscm, tra);
	return 0;
}

int _scm_work_data_delete(SCM_T *piscm, TRANSACTION_E tra, time_t delete_time)
{
	if (!piscm->ddb || delete_time == 0) 
		_scm_put_message(iRET_VFS_DELETE_DATA, 0, 0, (void *)tra);
	else _work_data_delete(piscm, tra, delete_time);
	return 0;
}

int _scm_work_panicrec_start(SCM_T *piscm)
{
	if (!piscm->ddb) return -1;
	else _work_panicrec_start(piscm);
	return 0;
}

int _scm_work_panicrec_stop(SCM_T *piscm)
{
	if (!piscm->ddb) return -1;
	else _work_panicrec_stop(piscm);
	return 0;
}

int _scm_work_panicrec_toggle(SCM_T *piscm)
{
    if (scm_is_qc_mode() == 0) return -1;
    
	if (!piscm->ddb && !var_get_running_encoder_mode()) return -1;
	else _work_panicrec_toggle(piscm);
	return 0;
}

int _scm_change_wmode(SCM_T *piscm, int mode)
{
	if (!piscm->ddb) return -1;
	else _work_wmode_change(piscm, mode);
	return 0;
}

int _scm_get_wmode(SCM_T *piscm, NF_DISK_WRITE_MODE_E *w_mode)
{
	int ret = -1;
	if (!piscm->ddb) return -1;
	else ret = _work_wmode_get(piscm, w_mode);
	return ret;
}



////////////////////////////////////////////////////////////
//
// protected handlers
//

HANDLER int _scm_on_fs_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	_scm_disable_timeline();
	_wait_to_vfs_cleanup();
	switch (tra) {
	case TRA_FW_UPGRADE:
	case TRA_NET_FW_UPGRADE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_fw_upgrade(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_DESIGN_UP:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_design_update(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_IPCAM_FWUP_MODE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_ipcam_fwup_mode(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_TIME_CHANGE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_delete(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_FORMAT:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_format(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_ERASE_CH:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_erase_ch(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_FACTORY_DEFAULT:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_factory_default(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_DB_IMPORT:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_db_import(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;
		
	case TRA_RTL_SET:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rtl(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_RESTART_SERVICE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) {
			_scm_change_wmode_by_db(piscm);
			_scm_work_service_start(piscm, tra);
		}
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_SHUTDOWN:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_shutdown(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	case TRA_CREATE_RAID:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_create_raid(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING); 
		break;

	case TRA_DELETE_RAID:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_delete_raid(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _raid_upgrade_fw(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STOPPING);
		break;

	}

	return 0;
}

HANDLER int _scm_on_fs_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;
	ERASE_CHINFO_T *eInfo;
    gchar strBuf[68];	
    time_t tt1, tt2;
    
	_scm_enable_timeline();
	_wait_to_vfs_cleanup();
    scm_regi_password_to_hdd();
	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) {
		    _scm_send_msg_to_viewer(INFY_SET_WEB_FWUP_MOUNT_PATH, 0);
		    _scm_work_rec_start(piscm, tra);
	    }
		else {
			_scm_work_booting_end(piscm, FAIL);
			_scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		}
		break;

	case TRA_DB_IMPORT:
		_scm_put_log_with_tra_prev_user(piscm, DB_LOAD, 0, 0, tra);
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;

	case TRA_FACTORY_DEFAULT:
		_scm_put_log_with_tra_prev_user(piscm, FACTORY_DEFAULT, 0, 0, tra);
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;

	case TRA_FORMAT:
		DMSG(9, "");
		_scm_put_log_with_tra(piscm, FORMAT, 0, 0, tra);
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;

	case TRA_ERASE_CH:
		DMSG(9, "");
	    memset(strBuf, 0x00, sizeof(strBuf));	
	    eInfo = (ERASE_CHINFO_T*)piscm->chart[tra].alloc_data;
        tt1 = eInfo->start;
        tt2 = eInfo->end;
        dtf_get_local_datetime_range(tt1, tt2, strBuf);
		_scm_put_log_with_tra(piscm, ERASE, 0, 0, tra);      
		_scm_put_log(ERASED_RANGE, 0, strBuf);		
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;

	case TRA_RESTART_SERVICE:
	case TRA_RTL_SET:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;

	case TRA_TIME_CHANGE:
		DMSG(9, "");
	    memset(strBuf, 0x00, sizeof(strBuf));	
	    tt1 = piscm->chart[tra].time_data.tv_sec;   // new time
	    tt2 = piscm->chart[tra].int_data;           // cur time
        dtf_get_local_datetime_range(tt1, tt2, strBuf);
        if (piscm->chart[tra].err_code == ER_NONE)
        {
    		_scm_put_log_with_tra(piscm, TIME_CHANGED, 0, 0, tra);
            if ((tt1 != 0) && (tt1 < tt2)) _scm_put_log(ERASED_RANGE, 0, strBuf);
        }
		if (IS_VFS_SUCCESS()) _scm_work_rec_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_VFS_STARTING);
		break;
	}
	return 0;
}

HANDLER int _scm_on_delete_data_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	_wait_to_vfs_cleanup();
	switch (tra) {
	case TRA_TIME_CHANGE:
		if (IS_VFS_SUCCESS()) _scm_work_time_change(piscm, tra);
		else {
			_scm_mark_error_tra(piscm, tra, ER_DATA_DELETING);
			_scm_work_fs_start(piscm, tra);
		}
		break;
	}
	return 0;
}

HANDLER int _scm_on_erase_ch_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	_wait_to_vfs_cleanup();
	switch (tra) {
	case TRA_ERASE_CH:
		if (IS_VFS_SUCCESS())  _scm_work_fs_start(piscm, tra);
		else {
			_scm_mark_error_tra(piscm, tra, ER_ERASE_CH);
			_scm_work_fs_start(piscm, tra);
		}
		break;
	}
	return 0;
}

HANDLER int _scm_on_format_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	_wait_to_vfs_cleanup();
	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
		_scm_reset_err_cnt(piscm);
		if (IS_VFS_SUCCESS()) {
			_scm_push_notification(INFY_FORMAT_CMPL, tra);
//			_scm_change_wmode_to_ow(piscm);
			_scm_change_wmode_by_db(piscm);
			_scm_work_smart_boot(piscm, tra);
		}
		else {
			_scm_work_booting_end(piscm, FAIL);
			_scm_finalize_tra(piscm, tra, ER_FORMATTING);
		}
		break;
	case TRA_FORMAT:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) {
			_scm_push_notification(INFY_FORMAT_CMPL, tra);
//			_scm_change_wmode_to_ow(piscm);
			_scm_change_wmode_by_db(piscm);
			_scm_work_service_start(piscm, tra);
		}
		else {
			_scm_mark_error_tra(piscm, tra, ER_FORMATTING);
			_scm_work_fs_start(piscm, tra);
		}
		break;
	}
	return 0;
}

HANDLER int _scm_on_broken_raid_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
    		_scm_send_msg_to_viewer(INFY_BROKEN_RAID_CMPL, 0);
		//scm_send_to_viewer(INFY_BROKEN_RAID_CMPL, 0, 0, 0);		
        _scm_reboot_system(piscm, RR_NA, 30000);
		break;
	}
	return 0;
}

HANDLER int _scm_on_rtl_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_fs_start(piscm, tra);
		else {
			_scm_mark_error_tra(piscm, tra, ER_RTL_SETTING);
			_scm_work_fs_start(piscm, tra);
		}
		break;

	case TRA_ERASE_CH:
	case TRA_FORMAT:
	case TRA_RTL_SET:
	case TRA_RESTART_SERVICE:
	case TRA_TIME_CHANGE:
	case TRA_FW_UPGRADE:
	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
	case TRA_CREATE_RAID:
	case TRA_DELETE_RAID:
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_fs_start(piscm, tra);
		else {
			_scm_mark_error_tra(piscm, tra, ER_RTL_SETTING);
			_scm_work_fs_start(piscm, tra);
		}
		break;
	}

	return 0;
}

HANDLER int _scm_on_rec_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_ERASE_CH:
	case TRA_FORMAT:
	case TRA_RTL_SET:
	case TRA_RESTART_SERVICE:
	case TRA_TIME_CHANGE:
	case TRA_FW_UPGRADE:
	case TRA_DESIGN_UP:
	case TRA_IPCAM_FWUP_MODE:
	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
	case TRA_SHUTDOWN:
	case TRA_NET_FW_UPGRADE:
	case TRA_CREATE_RAID:
	case TRA_DELETE_RAID:
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_fs_stop(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;
	}

	return 0;
}

HANDLER int _scm_on_rec_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_net_start(piscm, tra);
		else {
			_scm_work_booting_end(piscm, FAIL);
			_scm_finalize_tra(piscm, tra, ER_COMMON);
		}
		break;

	case TRA_ERASE_CH:
	case TRA_FORMAT:
	case TRA_RTL_SET:
	case TRA_RESTART_SERVICE:
	case TRA_TIME_CHANGE:
	case TRA_FW_UPGRADE:
	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
	case TRA_CREATE_RAID:
	case TRA_DELETE_RAID:
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_net_start(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;
	}

	return 0;
}

HANDLER int _scm_on_net_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_IP_CHANGE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_ip_change(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_RTL_SETTING);
		break;

	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
	case TRA_TIME_CHANGE:
	case TRA_FW_UPGRADE:
	case TRA_DESIGN_UP:
	case TRA_IPCAM_FWUP_MODE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rec_stop(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;

	case TRA_ERASE_CH:
	case TRA_FORMAT:
	case TRA_RTL_SET:
	case TRA_RESTART_SERVICE:
	case TRA_SHUTDOWN:
	case TRA_CREATE_RAID:
	case TRA_DELETE_RAID:		
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_work_rec_stop(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;

	case TRA_SSL_INSTALL:
        DMSG(9, "");
        if (IS_VFS_SUCCESS()) _scm_work_ssl_install(piscm, tra);
        else _scm_finalize_tra(piscm, tra, ER_COMMON);
        break;

	case TRA_SSL_DELETE:
        DMSG(9, "");
        if (IS_VFS_SUCCESS()) _scm_work_ssl_delete(piscm, tra);
        else _scm_finalize_tra(piscm, tra, ER_COMMON);
        break;
	}

	return 0;
}

HANDLER int _scm_on_net_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	VFS_RESULT_E result = (VFS_RESULT_E)pmsg->param;

	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		DMSG(9, "");
		_scm_work_booting_end(piscm, SUCCESS);
		if (IS_VFS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;

	case TRA_IP_CHANGE:
#ifdef _IPX_MODEL_UX
		_scm_work_ipcam_ctrl(piscm, tra);
		_scm_put_log_with_tra(piscm, CHANGE_NET_IP, 0, 0, tra);
#endif
        nvm_update_oneself_nvr_info();
        
		if (IS_VFS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;

	case TRA_RTL_SET:
	case TRA_ERASE_CH:
	case TRA_FORMAT:
	case TRA_RESTART_SERVICE:
	case TRA_FW_UPGRADE:
	case TRA_CREATE_RAID:
	case TRA_DELETE_RAID:
	case TRA_JM_UPDATE:
		DMSG(9, "");
		if (IS_VFS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;

	case TRA_TIME_CHANGE:
		DMSG(9, "");
		_scm_work_post_ats(piscm, tra);
		if (IS_VFS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		_scm_init_time_change();
		break;

	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
		DMSG(9, "");
#ifdef _IPX_MODEL_UX
		_scm_work_ipcam_ctrl(piscm, tra);
#endif
		if (IS_VFS_SUCCESS()) _scm_finalize_db_change(piscm, tra);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		ssm_update_auth();
		break;

	case TRA_SSL_INSTALL:        
	case TRA_SSL_DELETE:        
		if (IS_VFS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, ER_COMMON);
		break;
	}

	return 0;
}

HANDLER int _scm_on_unc_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_UNC_CHECK:
		DMSG(9, "RESULT = [%d]", result);
		if (IS_SUCCESS()) _scm_finalize_tra(piscm, tra, 0);
		else _scm_finalize_tra(piscm, tra, -1);
		_scm_cleanup_unc_check(piscm);
		break;
	}
	return 0;
}

HANDLER int _scm_on_rcvr_expired(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	RESULT_E result = (RESULT_E)pmsg->param;

	DMSG(9, "");
	_scm_stop_rcvr_timer(piscm);
	scm_buzzer_on();
	_scm_send_msg_to_viewer(INFY_RECOVERY_EXPIRED, -1);
	_scm_reboot_system(piscm, RR_RCVREXPIRED, 10000);

	return 0;
}

HANDLER int _scm_on_smart_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	int result = (int)pmsg->param;

	DMSG(9, "%d", result);
	switch (tra) {
	case TRA_BOOTUP_SYSTEM:
		if (IS_SUCCESS()) {
			DMSG(9, "SMART OK");
			_start_rcvr_timer(piscm);
			_scm_work_service_start(piscm, tra);
		}
		else {
			DMSG(9, "SMART ERROR");
			_scm_track_tra(piscm, IRPL_BOOT_SMART_ERROR, tra);
			scm_buzzer_on();
			_scm_send_msg_to_viewer(IREQ_BOOT_SMART_ERROR, (int)result);
		}
		break;
	}

	return 0;
}

HANDLER int _scm_on_smart_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	DISK_CONFIRM_E confirm = pmsg->param;
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	if (tra == TRA_NONE) return -1;

	DMSG(9, "");
	_scm_cancel_reboot(piscm);
	scm_buzzer_off();

	switch (confirm) {
	case DISK_OK:
		DMSG(1, "USER OK CONFIRM");
		_start_rcvr_timer(piscm);
		_scm_work_service_start(piscm, tra);
		break;
	default:
		g_assert(0);			// enhanced error check
		break;
	}

	return 0;
}

HANDLER int _scm_on_disk_format_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	int mode;
	DISK_CONFIRM_E confirm = pmsg->param;
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	if (tra == TRA_NONE) return -1;

	mode = NF_DISK_FORMAT_MODE_FORMAT;

	DMSG(9, "");
	_scm_cancel_reboot(piscm);
	scm_buzzer_off();

	switch (confirm) {
	case DISK_FORMAT:
		DMSG(1, "USER OK CONFIRM");
		piscm->chart[tra].int_data = mode;
		_scm_work_format(piscm, tra);
		break;
	case DISK_REBOOT:
		DMSG(1, "USER CANCEL CONFIRM");
		_scm_reboot_system(piscm, RR_NA, 0);
		break;
	case DISK_NOFORMAT:
		_scm_reboot_system(piscm, RR_NA, 30000);
		break;
	default:
		g_assert(0);			// enhanced error check
		break;
	}

	return 0;
}

HANDLER int _scm_on_disk_broken_raid_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	DISK_CONFIRM_E confirm = pmsg->param;	
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);

	DMSG(9, "");
	_scm_cancel_reboot(piscm);
	scm_buzzer_off();

	switch (confirm) {
	case DISK_DELRAID:
		DMSG(1, "USER OK CONFIRM");
		_work_broken_raid(piscm, tra);
		break;
	case DISK_REBOOT:
		DMSG(1, "USER CANCEL CONFIRM");
		_scm_reboot_system(piscm, RR_NA, 0);
		break;
	default:
		g_assert(0);			// enhanced error check
		break;
	}

	return 0;
}

HANDLER int _scm_on_disk_rebuild_error_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);

    scm_buzzer_off();
    _scm_put_message(iNFY_DISKINIT_START, 0, 0, (void *)tra);

    return 0;
}

HANDLER int _scm_on_rcvr_rate(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	int rate = pmsg->param;
	static int prev_rate = 0;

	if (prev_rate != rate) _scm_retouch_rcvr_timer(piscm);
	_scm_send_msg_to_viewer(pmsg->msgid, pmsg->param);
	prev_rate = rate;
	return 0;
}

HANDLER int _scm_on_disk_sync_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	int mode = NF_DISK_FORMAT_MODE_SYNC;
	DISK_CONFIRM_E confirm = pmsg->param;
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	CMMACK_T cmmack = { CMMPT_SCM, iRET_VFS_FORMAT_STORAGE, (void *)tra };
	if (tra == TRA_NONE) return -1;

	_scm_cancel_reboot(piscm);
	scm_buzzer_off();

	switch (confirm) {
	case DISK_SYNC:
		DMSG(1, "USER OK CONFIRM");
		piscm->chart[tra].int_data = mode;
		_scm_work_format(piscm, tra);
		break;
	case DISK_NOSYNC:
		DMSG(1, "USER CANCEL CONFIRM");
		_scm_work_igndisk_boot(piscm, tra);
		break;
	case DISK_REBOOT:
		DMSG(1, "REBOOT CONFIRM");
		_scm_reboot_system(piscm, RR_NA, 0);
		break;
	default:
		g_assert(0);			// enhanced error check
		break;
	}

	return 0;
}

HANDLER int _scm_on_get_disk_info_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) 
	{
    	case TRA_DISK_INFO:
    		DMSG(9, "");
    		_scm_finalize_tra(piscm, tra, 0);
    		_cleanup_get_disk_info(piscm);

    		break;
	}
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//


int scm_get_disk_capinfo(STRG_TYPE_E type, DISK_CAPINFO_T *cap_info)
{
	if (!cap_info) return -1;
	if (!iscm.ddb) return -1;
	memcpy(cap_info, &iscm.ddb->cap[type], sizeof(DISK_CAPINFO_T));
	return 0;
}

int scm_get_disk_recinfo(STRG_TYPE_E type, DISK_RECINFO_T *rec_info)
{
	if (!rec_info) return -1;
	if (!iscm.ddb) return -1;
	_make_disk_rec_info(type, HIDE, &iscm.ddb->rec[type]);
	memcpy(rec_info, &iscm.ddb->rec[type], sizeof(DISK_RECINFO_T));
	return 0;
}

int scm_get_disk_smartinfo(STRG_TYPE_E type, DISK_SMARTINFO_T *smart_info)
{
	if (!smart_info) return -1;
	if (!iscm.ddb) return -1;
	_make_disk_smart_info(type, &iscm.ddb->smart[type]);
	memcpy(smart_info, &iscm.ddb->smart[type], sizeof(DISK_SMARTINFO_T));
	return 0;
}

int scm_format_storage(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FORMAT;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = NF_DISK_FORMAT_MODE_FORMAT;

	_scm_push_notification(INFY_FORMAT_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, RS_FORMAT);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_get_prev_unc_error()
{
	gint key = 0;
	nf_smart_get_unc(&key, NULL);
	DMSG(1, "PREVIOUS UNC ERROR CHECK : %d\n", key);

	return key;
}

int scm_get_disk_count()
{
	return var_get_running_disk_count();
}

int scm_erase_ch(BITMASK chmask, time_t start, time_t end, IMSG ret_msg)
{
	NF_PRIVACY_TIME_INFO tinfo;
	ERASE_CHINFO_T *erase_info;
	TRANSACTION_E tra = TRA_ERASE_CH;

	memset(&tinfo, 0x00, sizeof(NF_PRIVACY_TIME_INFO));
	tinfo.start_time = start;
	tinfo.end_time = end;
	if (!nf_filesystem_check_privacy_section(chmask, &tinfo)) return -1;
	
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	erase_info = imalloc(sizeof(ERASE_CHINFO_T));
	erase_info->chmask = chmask;
	erase_info->start = start;
	erase_info->end = end;
	iscm.chart[tra].alloc_data = erase_info;

	_scm_work_service_stop(&iscm, tra, RS_ERASE_CH);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_reload_disk_info()
{
	return _scm_load_disk_info(&iscm);
}

int scm_get_disk_info(IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_DISK_INFO;
    CMMACK_T cmmack = {CMMPT_SCM, iRET_WRK_GET_DISK_INFO, (void*)tra};
    if (iscm.wrk_disk_info) return -1;

    DMSG(9, "");
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    iscm.wrk_disk_info = wrk_create_worker(_proc_get_disk_info, &cmmack);
    wrk_run_once_param(iscm.wrk_disk_info, IMSG_NONE, &iscm, 0, 0);
    
    return 0;
}

int scm_stop_fs_urgent()
{
	vfs_stop_urgent();
	return 0;
}

