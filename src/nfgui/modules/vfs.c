/*
 * vfs.c
 * 	- video filesystem module
 *	- dependencies :
 *			GThread
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 29, 2010
 *
 */

#include "ix_func.h"
#include "cmm.h"
#include "iux_afx.h"
#include "vfs.h"

#include "nf_record.h"
#include "nf_network.h"
#include "nf_api_disk.h"
#include "ix_mem.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"VFS"

/*
 * if the return value of thread proc is -1, the loop will be terminated.
 */
typedef int (*THREADPROC)(void *data);

#define IS_RTL_SET_ORDER(pivfs) 		(pivfs->chart.opr & VFO_RTL_SET)
#define IS_FS_START_ORDER(pivfs) 		(pivfs->chart.opr & VFO_FS_START)
#define IS_REC_START_ORDER(pivfs) 		(pivfs->chart.opr & VFO_REC_START)
#define IS_NET_START_ORDER(pivfs) 		(pivfs->chart.opr & VFO_NET_START)
#define IS_REC_STOP_ORDER(pivfs)		(pivfs->chart.opr & VFO_REC_STOP)
#define IS_NET_STOP_ORDER(pivfs) 		(pivfs->chart.opr & VFO_NET_STOP)
#define IS_FS_STOP_ORDER(pivfs) 		(pivfs->chart.opr & VFO_FS_STOP)
#define IS_DATA_DELETE_ORDER(pivfs)		(pivfs->chart.opr & VFO_DATA_DELETE)
#define IS_ERASE_CH_ORDER(pivfs)		(pivfs->chart.opr & VFO_ERASE_CH)
#define IS_CHANGE_WMODE_ORDER(pivfs)	(pivfs->chart.opr & VFO_CHANGE_WMODE)
#define IS_FORMAT_ORDER(pivfs)	 		(pivfs->chart.opr & VFO_FORMAT)

#define VFS_LOCK()		g_mutex_lock(ivfs.mtx)
#define VFS_UNLOCK()	g_mutex_unlock(ivfs.mtx)
#define PVFS_LOCK()		g_mutex_lock(pivfs->mtx)
#define PVFS_UNLOCK()	g_mutex_unlock(pivfs->mtx)

////////////////////////////////////////////////////////////
//
// private data type 
//

/* specific operation what action VFS module should do
 *
 * 		- some part of operation can be bitwised value
 */
typedef enum _VFS_OPERATION_E {
	VFO_REC_STOP			= 0x0001,
	VFO_NET_STOP			= 0x0002,
	VFO_FS_STOP				= 0x0004,
	VFO_FS_START			= 0x0040,
	VFO_REC_START			= 0x0020,
	VFO_NET_START			= 0x0010,

	// following operation cannot be bitwised to the above
	// must be used individually
	VFO_RTL_SET				= 0x0100,
	VFO_DATA_DELETE			= 0x0200,
	VFO_CHANGE_WMODE		= 0x0400,
	VFO_FORMAT				= 0x0800,
	VFO_ERASE_CH			= 0x1000,
} VFS_OPERATION_E;


typedef struct _VFS_CHART_T {
	CMMACK_T			cmmack;
	VFS_OPERATION_E		opr;
	int					rtl;
	unsigned int		delete_tv;
	ERASE_CHINFO_T		erase_info;
	int					format_mode;
	RS_SRVSTOP_E		reason;
} VFS_CHART_T;

typedef enum _RUN_STAT_E {
	STOP	= 0,
	RUN		= 1,
} RUN_STAT_E;

typedef struct _VFS_T {
	// internal data
	GThread				*thd;
	CMMPORT				cmmpt;	
	GMutex				*mtx;
	int					loop_mode;
	RUN_STAT_E			run;

	RUN_STAT_E			rec_s;	// record state
	RUN_STAT_E			net_s;
	RUN_STAT_E			fs_s;
	RUN_STAT_E			vfs_s;

	VFS_CHART_T			chart;
	VFS_RESULT_E		result;
	
} VFS_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static VFS_T ivfs;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _set_loop_mode(VFS_T *pivfs)
{
	pivfs->loop_mode = 1;
	return 0;
}

static int _reset_loop_mode(VFS_T *pivfs)
{
	pivfs->loop_mode = 0;
	return 0;
}

static int _is_loop_mode(VFS_T *pivfs)
{
	return pivfs->loop_mode;
}

static int _input_message(IMSG msgid)
{
	CMMPORT cmmpt = vfs_get_cmmport();
	return cmm_send_message(cmmpt, msgid, 0, 0, 0);
}

static int _ack_message(VFS_T *pivfs, int ret)
{
	CMMPORT cmmpt = pivfs->chart.cmmack.cmmpt;
	IMSG msgid = pivfs->chart.cmmack.msgid;
	void *data = pivfs->chart.cmmack.data;

	return cmm_send_message(cmmpt, msgid, ret, 0, data);
}

// below codes are not grace, but it will be modified later
//
static int _notify_message(VFS_T *pivfs, IMSG msgid, int param)
{
	CMMPORT cmmpt = pivfs->chart.cmmack.cmmpt;
	void *data = pivfs->chart.cmmack.data;

	return cmm_send_message(cmmpt, msgid, param, 0, data);
}

static int _work_message(VFS_T *pivfs, CMM_MESSAGE_T *pmsg)
{
	switch (pmsg->msgid) {
	case IMSG_NONE:
		break;

	case iNFY_RTL_SET_BEGIN:
	case iNFY_DATA_DELETE_BEGIN:
	case iNFY_FS_STOP_BEGIN:
	case iNFY_FS_START_BEGIN:
	case iNFY_FS_RECOVERY_BEGIN:
	case iNFY_FS_RECOVERY_CMPL:
	case iNFY_FORMAT_BEGIN:
	case iNFY_PRIVACY_BEGIN:
		DMSG(9, "");
		_notify_message(pivfs, pmsg->msgid, 0);
		break;

	case iNFY_NF_FILESYSTEM_START:
		DMSG(9, "");
		_notify_message(pivfs, INFY_RECOVERY_CMPL, 0);
		_reset_loop_mode(pivfs);
		break;

	case iNFY_NF_FILESYSTEM_SET_RTLIMIT:
		DMSG(9, "");
		_notify_message(pivfs, INFY_RTL_SET_CMPL, 0);
		_reset_loop_mode(pivfs);
		break;

	case iNFY_PRIVACY_CMPL:
	case iNFY_NF_FILESYSTEM_DELETE_DATA:
	case iNFY_NF_FILESYSTEM_STOP:
	case iNFY_NF_DISK_FORMAT:
		DMSG(9, "");
		_reset_loop_mode(pivfs);
		break;

	case IRPL_CFRM_DATA_DELETE_FAIL:
		DMSG(9, "");
		_reset_loop_mode(pivfs);
		break;
	}

	if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
	return 0;
}

static int _post_rtl_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_RTLIMIT, &progress)) { 

		if (progress.type != NF_SST_PRGT_RTLIMIT) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "RECORD TIME LIMIT PROGRESS RATE       [%d] = %d * 100 / %d\n", rate, progress.current, progress.total);
		DMSG(1, "RTL -> current : %d, total : %d",progress.current,progress.total);

		_notify_message(pivfs, INFY_RTL_SET_RATE, rate);
	}   
	else DMSG(1, "RTL: GET PROGRESS FAIL\n");
	return 0;
}

static int _post_delete_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_DELETE, &progress)) { 

		if (progress.type != NF_SST_PRGT_DELETE) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "DATA DELETE PROGRESS RATE       [%d] = %d * 100 / %d\n", rate, progress.current, progress.total);

		_notify_message(pivfs, INFY_DATA_DELETE_RATE, rate);
	}   
	else DMSG(1, "DELETE: GET PROGRESS FAIL\n");
	return 0;
}

static int _post_erase_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_PRIVACY, &progress)) { 

		if (progress.type != NF_SST_PRGT_PRIVACY) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "ERASE CH PROGRESS RATE       [%d] = %d * 100 / %d\n", rate, progress.current, progress.total);

		_notify_message(pivfs, INFY_ERASE_CH_RATE, rate);
	}   
	else DMSG(1, "ERASE: GET PROGRESS FAIL\n");
	return 0;
}

static int _post_format_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_FORMAT, &progress)) { 

		if (progress.type != NF_SST_PRGT_FORMAT) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "DATA FORMAT PROGRESS RATE [%d] = %d * 100 / %d\n", rate, progress.current, progress.total);

		_notify_message(pivfs, INFY_FORMAT_RATE, rate);
	}   
	else DMSG(1, "FORMAT: GET PROGRESS FAIL\n");
	return 0;
}

static int _post_fs_start_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_FSSTART, &progress)) { 

		if (progress.type != NF_SST_PRGT_FSSTART) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "FS START PROGRESS RATE [%d] = %d * 100 / %d\n", rate, progress.current, progress.total);

		// intentioned, INFY_RECOVERY_RATE
		_notify_message(pivfs, INFY_RECOVERY_RATE, rate);
	}   
	else DMSG(1, "FSSTART: GET PROGRESS FAIL\n");
	return 0;
}

// not used
static int _post_recovery_progress_rate(VFS_T *pivfs)
{
	NF_SST_PROGRESS progress;
	int rate;

	DMSG(9, "");
	if (nf_filesystem_get_progress(NF_SST_PRGT_RECOVERY, &progress)) { 

		if (progress.type != NF_SST_PRGT_RECOVERY) return 0;
		if (progress.total <= 0) return 0;

		rate = progress.current * 100 / progress.total;
		DMSG(1, "RECOVERY PROGRESS RATE       [%d] = %d *100 / %d\n", rate, progress.current, progress.total);

		_notify_message(pivfs, INFY_RECOVERY_RATE, rate);
	}   
	else DMSG(1, "RECOVERY: GET PROGRESS FAIL\n");
	return 0;
}

// not used
static int _trace_recovery_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_recovery_progress_rate(pivfs);
	return 0;
}

static int _trace_rtl_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_rtl_progress_rate(pivfs);
	return 0;
}

static int _trace_delete_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_delete_progress_rate(pivfs);
	return 0;
}

static int _trace_erase_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_erase_progress_rate(pivfs);
	return 0;
}

static int _trace_format_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_format_progress_rate(pivfs);
	return 0;
}

static int _trace_fs_start_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
	_post_fs_start_progress_rate(pivfs);
	return 0;
}

static int _trace_fs_stop_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (_is_loop_mode(pivfs) || nf_filesystem_is_online()) return 0;
	else return -1;
}

static int _trace_ipcam_stop_process(VFS_T *pivfs)
{
	DMSG(9, "");
	if (!_is_loop_mode(pivfs)) return -1;
#if defined (_IPX_MODEL_UX)
	if (nf_ipcam_is_all_ch_unplugged()) return 0;
#endif
	return -1;
}

static int _wait_to_finish(VFS_T *pivfs)
{
	if (!_is_loop_mode(pivfs)) return -1;
	return 0;
}

static int _wait_ret_message(VFS_T *pivfs)
{
	if (!_is_loop_mode(pivfs)) return -1;
	return 0;
}

static void _loop_thread(VFS_T *pivfs, THREADPROC proc, void *data, int sleep_time)
{
	int ret;
	CMM_MESSAGE_T msg;
	while (pivfs->run) {
		usleep(sleep_time);
		if (cmm_get_message(&msg) == 0) _work_message(pivfs, &msg);

		if (proc) ret = proc(data);
		if (ret == -1) break;
	}
	DMSG(9, "");
}

static int _start_record(VFS_T *pivfs)
{
	DMSG(9, "");
	nf_record_start(NULL);
	return 0;
}

static int _stop_record(VFS_T *pivfs)
{
	DMSG(9, "");
//	nf_panic_record_stop(NULL); // disable choissi 
	nf_record_stop(NULL);
	return 0;
}

static int _start_network(VFS_T *pivfs)
{
	// if network is running already, below function returns false;
	// therefore, don't have to check the state of network
	DMSG(9, "");
	nf_network_start();
	sleep(2);

#if defined (_IPX_MODEL_UX)
	nf_ipcam_start();
#endif
	return 0;
}

static int _stop_network(VFS_T *pivfs)
{
	DMSG(9, "");
	nf_network_stop(pivfs->chart.reason);	
	sleep(2);		// original SNF codes have this.

#if defined (_IPX_MODEL_UX)
	nf_ipcam_stop();
#endif
	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_ipcam_stop_process, (void *)pivfs, 300000);
	
	return 0;
}

static void _cb_data_delete(NF_SST_EVT_E event, int arg, void *sst_context)
{
	DMSG(9, "");
	switch (event) {
	case NF_SST_EVT_DELDATABEG:
		DMSG(1, "%s\n","DATA DELETE BEGIN");
		_input_message(iNFY_DATA_DELETE_BEGIN);
		break;

	case NF_SST_EVT_DELDATACMPL:
		DMSG(1, "%s\n","DATA DELETE COMPLETE");
		_input_message(iNFY_NF_FILESYSTEM_DELETE_DATA);
		break;
	}
}

static void _cb_erase_ch(NF_SST_EVT_E event, int arg, void *sst_context)
{
	DMSG(9, "%d", event);
	switch (event) {
	case NF_SST_EVT_PRIVACYBEG:
		DMSG(1, "%s\n","ERASE DELETE BEGIN");
		_input_message(iNFY_PRIVACY_BEGIN);
		break;

	case NF_SST_EVT_PRIVACYCMPL:
		DMSG(1, "%s\n","ERASE DELETE COMPLETE");
		_input_message(iNFY_PRIVACY_CMPL);
		break;
	}
}

static void _cb_fs_stop(NF_SST_EVT_E event, int arg, void *sst_context)
{
	DMSG(9, "");
	switch (event) {
	case NF_SST_EVT_FSSTOPBEG: 
		_input_message(iNFY_FS_STOP_BEGIN);
		break;
	case NF_SST_EVT_FSSTOPCMPL:
		DMSG(1, "FILESYSTEM NOTIFY : END\n");
		_input_message(iNFY_NF_FILESYSTEM_STOP);
		break;
	default:
		break;
	}
}

static void _cb_rtl_set(NF_SST_EVT_E event, int arg, void *sst_context)
{
	switch (event) {
	case NF_SST_EVT_RTLIMITSTART:
		DMSG(1, "%s\n","RECORD TIME LIMIT BEGIN");
		_input_message(iNFY_RTL_SET_BEGIN);
		break;

	case NF_SST_EVT_RTLIMITCOMPL:
		DMSG(1, "%s\n","RECORD TIME LIMIT COMPLETE");
		_input_message(iNFY_NF_FILESYSTEM_SET_RTLIMIT);
		break;
	}
}

static void _cb_fs_start(NF_SST_EVT_E event, int arg, void *sst_context)
{
	DMSG(9, "");
	switch (event) {
	case NF_SST_EVT_FSSTARTBEG:
		DMSG(1, "%s\n","FILE SYSTEM START BEGIN");
		_input_message(iNFY_FS_START_BEGIN);
		break;

	case NF_SST_EVT_RECOVERYBEG:
		DMSG(1, "%s\n","RECOVERY FILE SYSTEM START BEGIN");
		_input_message(iNFY_FS_RECOVERY_BEGIN);
		break;

	case NF_SST_EVT_RECOVERYCMPL:
		DMSG(1, "%s\n","RECOVERY FILE SYSTEM START BEGIN");
		_input_message(iNFY_FS_RECOVERY_CMPL);
		break;

	case NF_SST_EVT_FSSTARTCMPL:
		DMSG(1, "FILE SYSTEM START COMPLETE[ERR:%d]\n", arg);
		if (arg) {
			// error ???
		}
		_input_message(iNFY_NF_FILESYSTEM_START);
		break;
	}
}

static void _cb_format(NF_SST_EVT_E event, int arg, void *sst_context)
{
	DMSG(9, "");
	switch (event) {
	case NF_SST_EVT_FORMATBEG:
		DMSG(1, "%s\n"," FORMAT BEGIN");
		_input_message(iNFY_FORMAT_BEGIN);
		break;

	case NF_SST_EVT_FORMATCMPL:
		DMSG(1, "FORMAT COMPLETE[ERR:%d]\n", arg);
		if (arg) {
			// error ???
		}
		_input_message(iNFY_NF_DISK_FORMAT);
		break;
	}
}

static int _delete_data(VFS_T *pivfs)
{
	unsigned int delete_time = pivfs->chart.delete_tv;

	if (!nf_filesystem_delete_data(delete_time, _cb_data_delete, 0, NULL)) return -1;

	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_delete_process, (void *)pivfs, 300000);

	return 0;
}

static int _erase_ch(VFS_T *pivfs)
{
	NF_PRIVACY_TIME_INFO tinfo;
	ERASE_CHINFO_T *info = &pivfs->chart.erase_info;

	memset(&tinfo, 0x00, sizeof(NF_PRIVACY_TIME_INFO));
	tinfo.start_time = info->start;
	tinfo.end_time = info->end;

	if (!nf_filesystem_set_privacy_section(info->chmask, &tinfo, _cb_erase_ch, 0, NULL)) return -1;

	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_erase_process, (void *)pivfs, 300000);

	return 0;
}

static int _set_rtl_mode(VFS_T *pivfs)
{
	unsigned int rtl = pivfs->chart.rtl; 

	if (!nf_filesystem_set_rtlimit(rtl, 0, _cb_rtl_set, 0, NULL)) {
		DMSG(1, "FAILED TO APPLY RTL\n");
		//
		// ????
		return -1;
	}
	
	DMSG(9, "");
	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_rtl_process, (void *)pivfs, 300000);

	return 0;
}

static int _start_fs(VFS_T *pivfs)
{
	DMSG(9, "");
	if (nf_filesystem_is_online()) return 0;
	DMSG(9, "");
	if (!nf_filesystem_start(0, _cb_fs_start, 0, NULL)) return -1; 
	DMSG(9, "");

	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_fs_start_process, (void *)pivfs, 300000);

	return 0;
}

static int _stop_fs(VFS_T *pivfs)
{
	if (!nf_filesystem_is_online()) return 0;
	if (!nf_filesystem_stop(_cb_fs_stop, 0, NULL)) return -1;

	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_fs_stop_process, (void *)pivfs, 300000);

	return 0;
}

static int _format(VFS_T *pivfs)
{
	int mode = pivfs->chart.format_mode;
	if (!nf_disk_format(mode, 0, _cb_format, NULL, NULL)) return -1;

	_set_loop_mode(pivfs);
	_loop_thread(pivfs, _trace_format_process, (void *)pivfs, 300000);

	return 0;
}

static int _cleanup_thread(VFS_T *pivfs)
{
	if (cmm_mount_off_thread(pivfs->cmmpt) == -1) return -1;
	return 0;
}

/* this functions must be called in the worker thread only */
static int _cleanup_controller(VFS_T *pivfs)
{
	_cleanup_thread(pivfs);
	pivfs->thd = 0;
	pivfs->cmmpt = 0;
	pivfs->loop_mode = 0;
	memset(&pivfs->chart, 0x00, sizeof(VFS_CHART_T));
	pivfs->result = VSR_SUCCESS;
	return 0;
}

static int _init_result_value(VFS_T *pivfs, unsigned int init_value)
{
	pivfs->result = init_value;
	return 0;
}

static int _work_rtl_set_opr(VFS_T *pivfs)
{
	if (!IS_RTL_SET_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_set_rtl_mode(pivfs) == -1) {
		DMSG(1, "RTL SET FAIL\n");
		return -1;
	}

	pivfs->result &= ~VFO_RTL_SET; 
	DMSG(1, "RTL SET SUCCESS\n");
	return 0;
}

static int _work_fs_start_opr(VFS_T *pivfs)
{
	if (!IS_FS_START_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_start_fs(pivfs) == -1) {
		DMSG(1, "FS START FAIL\n");
		return -1;
	}

	pivfs->fs_s = RUN;
	pivfs->result &= ~VFO_FS_START; 
	DMSG(1, "FS START SUCCESS\n");
	ivfs.vfs_s = RUN;
	return 0;
}

static int _work_rec_start_opr(VFS_T *pivfs)
{
	if (!IS_REC_START_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_start_record(pivfs) == -1) {
		DMSG(1, "REC START FAIL\n");
		return -1;
	}

	pivfs->rec_s = RUN;
	pivfs->result &= ~VFO_REC_START; 
	DMSG(1, "REC START SUCCESS\n");
	return 0;
}

static int _work_net_start_opr(VFS_T *pivfs)
{
	if (!IS_NET_START_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_start_network(pivfs) == -1) {
		DMSG(1, "NET START FAIL\n");
		return -1;
	}

	pivfs->net_s = RUN;
	pivfs->result &= ~VFO_NET_START; 
	DMSG(1, "NET_START SUCCESS\n");
	return 0;
}

static int _work_rec_stop_opr(VFS_T *pivfs)
{
	if (!IS_REC_STOP_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_stop_record(pivfs) == -1) {
		DMSG(1, "REC STOP FAIL\n");
		return -1;
	}

	pivfs->rec_s = STOP;
	pivfs->result &= ~VFO_REC_STOP; 
	DMSG(1, "REC STOP SUCCESS\n");
	return 0;
}

static int _work_net_stop_opr(VFS_T *pivfs)
{
	if (!IS_NET_STOP_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_stop_network(pivfs) == -1) {
		DMSG(1, "NET STOP FAIL\n");
		return -1;
	}

	pivfs->net_s = STOP;
	pivfs->result &= ~VFO_NET_STOP; 
	DMSG(1, "NET STOP SUCCESS\n");
	return 0;
}

static int _work_fs_stop_opr(VFS_T *pivfs)
{
	if (!IS_FS_STOP_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_stop_fs(pivfs) == -1) {
		DMSG(1, "FS STOP FAIL\n");
		return -1;
	}

	pivfs->fs_s = STOP;
	pivfs->result &= ~VFO_FS_STOP; 
	DMSG(1, "FS STOP SUCCESS\n");
	return 0;
}

static int _work_data_delete_opr(VFS_T *pivfs)
{
	if (!IS_DATA_DELETE_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_delete_data(pivfs) == -1) {
		DMSG(1, "DATA DELETE FAIL\n");
		return -1;
	}

	pivfs->result &= ~VFO_DATA_DELETE; 
	DMSG(1, "DATA DELETE SUCCESS\n");
	return 0;
}

static int _work_erase_ch_opr(VFS_T *pivfs)
{
	if (!IS_ERASE_CH_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_erase_ch(pivfs) == -1) {
		DMSG(1, "ERASE CH FAIL\n");
		return -1;
	}

	pivfs->result &= ~VFO_ERASE_CH; 
	DMSG(1, "ERASE CH SUCCESS\n");
	return 0;
}

static int _work_format_opr(VFS_T *pivfs)
{
	if (!IS_FORMAT_ORDER(pivfs)) return 0;
	if (!pivfs->run) return -1;
	if (_format(pivfs) == -1) {
		DMSG(1, "FORMAT FAIL\n");
		return -1;
	}

	pivfs->result &= ~VFO_FORMAT; 
	DMSG(1, "FORMAT SUCCESS\n");
	return 0;
}

static int _stop(VFS_T *pivfs)
{
	PVFS_LOCK();
	pivfs->run = STOP;
	PVFS_UNLOCK();
	return 0;
}

static int _run(VFS_T *pivfs)
{
	PVFS_LOCK();
	pivfs->run = RUN;
	PVFS_UNLOCK();
	return 0;
}

static int _wait_run_signal(VFS_T *pivfs)
{
	while (!pivfs->run) usleep(10000);
	return 0;
}

static int _wait_for_stop(VFS_T *pivfs)
{
	while (pivfs->run == RUN) {
		DMSG(9, "WAIT FOR STOP VFS OPERATION..[%d]\n", pivfs->chart.opr);
		usleep(100000);
	}
	return 0;
}

static void	_vfs_service_proc(void *arg)
{
	CMMPORT ret_cmmpt;
	unsigned int ret_param;
	void *ret_data;
	VFS_T *pivfs = (VFS_T *)arg;
	int ret;

	_init_result_value(pivfs, (unsigned int)pivfs->chart.opr);
	_wait_run_signal(pivfs);
	DMSG(9, "operation = [%x]", pivfs->chart.opr);
	do {
		if ((ret = _work_net_stop_opr(pivfs)) == -1) break;
		if ((ret = _work_rec_stop_opr(pivfs)) == -1) break;
		if ((ret = _work_fs_stop_opr(pivfs)) == -1) break;
		if ((ret = _work_data_delete_opr(pivfs)) == -1) break;
		if ((ret = _work_erase_ch_opr(pivfs)) == -1) break;
		if ((ret = _work_format_opr(pivfs)) == -1) break;
		if ((ret = _work_rtl_set_opr(pivfs)) == -1) break;
		if ((ret = _work_fs_start_opr(pivfs)) == -1) break;
		if ((ret = _work_rec_start_opr(pivfs)) == -1) break;
		if ((ret = _work_net_start_opr(pivfs)) == -1) break;
	} while (0);

	_ack_message(pivfs, ret);
	_cleanup_controller(pivfs);
	_stop(pivfs);
	
	DMSG(9, "");
	return;
}

static int _read_conf(VFS_T *pivfs)
{
	return 0;
}

static int _ready_controller(VFS_T *pivfs)
{
	iassert(pivfs->run == 0);
	if (pivfs->run) { return -1; }

	iassert(pivfs->thd == 0);
	pivfs->thd = ifn_make_thread(_vfs_service_proc, pivfs);
	cmm_mount_on_thread(pivfs->thd);
	pivfs->cmmpt = pivfs->thd;
	return 0;
}

static int _init_delete_data_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack, time_t delete_tv)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_DATA_DELETE;
	if (pcmmack) pchart->cmmack = *pcmmack;
	pchart->delete_tv = delete_tv;

	return 0;
}

static int _init_sst_stop_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_FS_STOP;
	if (pcmmack) pchart->cmmack = *pcmmack;

	return 0;
}

static int _init_sst_start_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_FS_START;
	if (pcmmack) pchart->cmmack = *pcmmack;

	return 0;
}

static int _init_setting_rtl_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack, unsigned int rtl)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_RTL_SET;
	if (pcmmack) pchart->cmmack = *pcmmack;
	pchart->rtl = rtl;

	return 0;
}

static int _init_format_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack, int mode)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_FORMAT;
	if (pcmmack) pchart->cmmack = *pcmmack;
	pchart->format_mode = mode;

	return 0;
}

static int _init_erase_ch_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack, ERASE_CHINFO_T *pinfo)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_ERASE_CH;
	if (pcmmack) pchart->cmmack = *pcmmack;
	pchart->erase_info = *pinfo;

	return 0;
}

static int _init_record_start_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_REC_START;
	if (pcmmack) pchart->cmmack = *pcmmack;

	return 0;
}

static int _init_record_stop_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_REC_STOP;
	if (pcmmack) pchart->cmmack = *pcmmack;

	return 0;
}

static int _init_network_stop_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack, int reason)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_NET_STOP;
	if (pcmmack) pchart->cmmack = *pcmmack;
	pchart->reason = reason;

	return 0;
}

static int _init_network_start_chart(VFS_CHART_T *pchart, CMMACK_T *pcmmack)
{
	memset(pchart, 0x00, sizeof(VFS_CHART_T));
	pchart->opr = VFO_NET_START;
	if (pcmmack) pchart->cmmack = *pcmmack;

	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int vfs_start_fs(CMMACK_T *pcmmack)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_sst_start_chart(&ivfs.chart, pcmmack);
	if (_ready_controller(&ivfs) == -1) return -1;
	_run(&ivfs);
	return 0;
}

int vfs_stop_fs(CMMACK_T *pcmmack)
{
	ivfs.vfs_s = STOP;
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_sst_stop_chart(&ivfs.chart, pcmmack);
	if (_ready_controller(&ivfs) == -1) return -1;
	_run(&ivfs);
	return 0;
}

int vfs_set_rtl(CMMACK_T *pcmmack, unsigned int rtl)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_setting_rtl_chart(&ivfs.chart, pcmmack, rtl);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_delete_data(CMMACK_T *pcmmack, time_t delete_time)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_delete_data_chart(&ivfs.chart, pcmmack, delete_time);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_format_storage(CMMACK_T *pcmmack, int mode)
{
	DMSG(9, "");
	_wait_for_stop(&ivfs);

	_init_format_chart(&ivfs.chart, pcmmack, mode);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_erase_ch(CMMACK_T *pcmmack, ERASE_CHINFO_T *pinfo)
{
	DMSG(9, "");
	_wait_for_stop(&ivfs);

	_init_erase_ch_chart(&ivfs.chart, pcmmack, pinfo);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_stop_network(CMMACK_T *pcmmack, int reason)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_network_stop_chart(&ivfs.chart, pcmmack, reason);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_start_network(CMMACK_T *pcmmack)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_network_start_chart(&ivfs.chart, pcmmack);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_start_record(CMMACK_T *pcmmack)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_record_start_chart(&ivfs.chart, pcmmack);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

int vfs_stop_record(CMMACK_T *pcmmack)
{
	_wait_for_stop(&ivfs);
	DMSG(9, "");

	_init_record_stop_chart(&ivfs.chart, pcmmack);
	if (_ready_controller(&ivfs) == -1) return -1;
	DMSG(9, "");
	_run(&ivfs);
	return 0;
}

/*inline*/ extern CMMPORT vfs_get_cmmport()
{
	return ivfs.cmmpt;	
}

int vfs_filesystem_get_wmode(NF_DISK_WRITE_MODE_E *w_mode)
{
	DMSG(9, "");
	return nf_filesystem_get_wmode(w_mode, NULL) == TRUE ? 0 : -1;
}

int vfs_init()
{
	memset(&ivfs, 0x00, sizeof(VFS_T));
	_read_conf(&ivfs);

	ivfs.mtx = g_mutex_new();
	return 0;
}

int vfs_cleanup()
{
	g_mutex_free(ivfs.mtx);
	return 0;
}

int vfs_is_running()
{
	return (ivfs.run == 1);
}

gboolean vfs_filesystem_is_online()
{
	return nf_filesystem_is_online();
}

gboolean vfs_filesystem_change_wmode(unsigned int wmode)
{
	DMSG(9, "");
	return nf_filesystem_change_wmode(wmode, 1, NULL);
}

gboolean vfs_start_panic_record()
{
	DMSG(9, "");
	return nf_panic_record_start(NULL);
}

gboolean vfs_stop_panic_record()
{
	DMSG(9, "");
	return nf_panic_record_stop(NULL);
}

gboolean vfs_toggle_panic_record()
{
	DMSG(9, "");
	return nf_panic_record_toggle(NULL);
}

gboolean vfs_is_stopped()
{
	return !ivfs.vfs_s;
}

int vfs_stop_urgent()
{
	DMSG(1, "");
	nf_network_stop(NF_DISCONN_SVR_POWER_OFF);
	nf_record_stop(0);
	nf_filesystem_stop(0, 0, 0);
//	nf_dev_board_reset();
	DMSG(1, "");
	return 0;
}

