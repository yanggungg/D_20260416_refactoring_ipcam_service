/*
 * qry_bmk.c
 * 	- bookmark archiving 
 * 	- instance type
 * 		: single instance
 *	- dependencies
 *		: stm
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jun 23, 2011
 *
 */

/*
 * cps : capacity per sec 
 * unit : the interval to monitoring
 */


#include "qry.h"
#include "qry_internal.h"
#include "cmm.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include <math.h>
#include <memory.h>
#include "ix_func.h"
#include "stm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"BMK"

////////////////////////////////////////////////////////////
//
// private data type 
//


typedef enum _WORK_STAT_E {
	READY	= 0,
	STOP	= 1,
	RUN		= 2,
	PAUSE	= 3,
} WORK_STAT_E;

typedef enum _STOP_STAT_E {
	NOTYET	= 0,
	YES		= 1,
} STOP_STAT_E;


typedef struct _BMK_T {
	GThread			*thd;
	CMMPORT			cmmpt;
	WORK_STAT_E		work;
	STOP_STAT_E		stopped;
	int				sleep_time;
	int				cps;
	int				is_calculated;
	int				is_overed;
	int				is_querying;
	int				is_manual_query;

	NF_ARCH_AVI_PARAM  		aparam;
	NF_ARCH_PB_AVI_PARAM 	pparam;
	AVI_QINFO_T				avi;
//	AVI_QINFO_T				*copy_avi;
//	int						copy_result;

	time_t			base;			// time of first scene
//	time_t			bmk_start;		// time of start to bookmark
	time_t			bmk_qry;		// time of query
	time_t			lower;			// next check time toward past
	time_t			upper;			// next check time toward future
	int				size;
	int				elap;

} BMK_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static BMK_T ibmk;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _make_unit_param(NF_ARCH_AVI_PARAM *aparam, NF_ARCH_PB_AVI_PARAM *pparam)
{
	memset(aparam, 0x00, sizeof(NF_ARCH_AVI_PARAM));

	aparam->ch_mask 		= pparam->ch_mask;
	aparam->audio_mask 		= pparam->audio_mask;
	aparam->inc_log 		= pparam->inc_log;		
	aparam->inc_text		= pparam->inc_text;
	aparam->inc_ri			= pparam->inc_ri;
	aparam->inc_codec		= pparam->inc_codec;	
	aparam->inc_player		= pparam->inc_player;		
	aparam->no_audio_text	= pparam->no_audio_text;
	aparam->inc_pos_log		= pparam->inc_pos_log;
	memcpy(aparam->tag, pparam->tag, 32);
	memcpy(aparam->user, pparam->user, 32);
	memcpy(aparam->memo, pparam->memo, 256);
	return 0;
}

static int _set_unit_start_time(NF_ARCH_AVI_PARAM *aparam, time_t start)
{
	aparam->start_time.tv_sec = start;
	return 0;
}

static int _set_unit_end_time(NF_ARCH_AVI_PARAM *aparam, time_t end)
{
	aparam->end_time.tv_sec = end;
	return 0;
}

static int _cleanup_thread(BMK_T *pibmk)
{
	if (pibmk->cmmpt == 0) return -1;
    if (cmm_mount_off_thread(pibmk->cmmpt) == -1) return -1; 
    return 0;
}

static int _pause(BMK_T *pibmk)
{
	pibmk->work = PAUSE;
	usleep(20000);
	return 0;
}

static int _calc_cps(BMK_T *pibmk, int size, int elap)
{
	DMSG(0, "SIZE = %d, ELAP = %d\n", size, elap);
	if (size == 0 || elap == 0) pibmk->cps = 0;
	else pibmk->cps = size / elap;	// Mbyte per sec
	return 0;
}

static int _set_boundary(BMK_T *pibmk)
{
	int offset = pibmk->cps == 0 ? 0 : pibmk->size / pibmk->cps;
	pibmk->lower = pibmk->base - offset;
	pibmk->upper = pibmk->base + offset;
	DMSG(1, "lower = %u, upper = %u\n", pibmk->lower, pibmk->upper);
	return 0;
}

static int _stop(BMK_T *pibmk)
{
	pibmk->work = STOP;
	DMSG(1, "STOP = %d\n", STOP);
	return 0;
}

static int _run(BMK_T *pibmk)
{
	pibmk->stopped = NOTYET;
	pibmk->work = RUN;
	return 0;
}

static int _is_work(BMK_T *pibmk)
{
	return (pibmk->work == RUN || pibmk->work == PAUSE);
}

static int _was_running(BMK_T *pibmk)
{
	return (pibmk->work == RUN || pibmk->work == PAUSE);
}

static void _cb_manual_query(int result, void *context)
{
	BMK_T *pibmk = (BMK_T *)context;
	CMMPORT cmmpt = qry_get_cmmport();
	DMSG(1, "");
	cmm_send_message(cmmpt, iNFY_BOOKMARK_MANUAL, result, 0, 0);
	return 0;
}

static QRY_CODE_E _query_manual(BMK_T *pibmk)
{
	QRY_CODE_E ret_val = QRY_SUCCESS;
	DMSG(9, "QUERY ID = %d\n", pibmk->is_querying);
	if (pibmk->is_querying)	return QRY_CODE_INV_COMMAND;

	memset(&pibmk->avi, 0x00, sizeof(AVI_QINFO_T));
	pibmk->avi.info = imalloc(sizeof(NF_ARCH_AVI_INFO));
	pibmk->bmk_qry = stm_get_time_t();
	pibmk->is_querying = 2;

	if (!nf_arch_pb_query_avi_ex(pibmk->avi.info, _cb_manual_query, pibmk,
			&pibmk->avi.err, pibmk->size, 1, 0)) {
		if (pibmk->avi.err) {
			ret_val = pibmk->avi.err->code;
			g_error_free(pibmk->avi.err);
		}
		else ret_val = QRY_CODE_FAIL;

		pibmk->is_querying = 0;
		DMSG(1, "QUERY FAILED\n");
	}

	return ret_val;
}

static int _process_query_request(BMK_T *pibmk)
{
	QRY_CODE_E ret;
	_pause(pibmk);
	ret = _query_manual(pibmk);
	if (ret < 0) pibmk->is_manual_query = 1;
	return ret;
}

static int _process_unit_query_result(BMK_T *pibmk, int result)
{
	int elap;
	int size;

	DMSG(1, "RESULT = %d\n", result);
	if (result < 0) {
		pibmk->is_overed = 1;
		pibmk->is_manual_query = 0;
		_qry_send_avi_result(result, &pibmk->avi, QRY_BOOKMARK);
	}
	else {
		size = pibmk->avi.info->total_size / 1024;	// Mbyte
		elap = fabs(pibmk->bmk_qry - pibmk->base);
		_calc_cps(pibmk, size, elap);
		_set_boundary(pibmk);

		if (pibmk->is_manual_query) {
			pibmk->is_manual_query = 0;
			_process_query_request(pibmk);
		}
		else { 
			qry_release_avi();
			if (_was_running(pibmk)) _run(pibmk);
		}
	}
	return 0;
}

static int _process_whole_query_result(BMK_T *pibmk, int result)
{
	int elap;
	int size;

	DMSG(1, "RESULT = %d\n", result);
	if (result < 0) {
		pibmk->is_overed = 1;
		pibmk->is_manual_query = 0;
		_qry_send_avi_result(result, &pibmk->avi, QRY_BOOKMARK);
	}
	else {
		size = pibmk->avi.info->total_size / 1024;	// Mbyte
		elap = fabs(pibmk->bmk_qry - pibmk->base);
		_calc_cps(pibmk, size, elap);
		_set_boundary(pibmk);

		if (pibmk->is_manual_query) {
			pibmk->is_manual_query = 0;
			_qry_send_avi_result(result, &pibmk->avi, QRY_BOOKMARK);
		}
		else { 
			qry_release_avi();
			if (_was_running(pibmk)) _run(pibmk);
		}
	}
	return 0;
}

static void _cb_unit_query(int result, void *context)
{
	BMK_T *pibmk = (BMK_T *)context;
	CMMPORT cmmpt = qry_get_cmmport();
	DMSG(1, "");
	cmm_send_message(cmmpt, iNFY_BOOKMARK_UNIT_QUERY, result, 0, 0);
	return 0;
}

static void _cb_whole_query(int result, void *context)
{
	BMK_T *pibmk = (BMK_T *)context;
	CMMPORT cmmpt = qry_get_cmmport();
	DMSG(1, "");
	cmm_send_message(cmmpt, iNFY_BOOKMARK_WHOLE_QUERY, result, 0, 0);
	return 0;
}

static int _query_whole(BMK_T *pibmk)
{
	DMSG(9, "QUERY ID = %d\n", pibmk->is_querying);
	if (pibmk->is_querying) return -1;

	memset(&pibmk->avi, 0x00, sizeof(AVI_QINFO_T));
	pibmk->avi.info = imalloc(sizeof(NF_ARCH_AVI_INFO));
	pibmk->bmk_qry = stm_get_time_t();
	pibmk->is_querying = 1;
	if (!nf_arch_pb_query_avi_ex(pibmk->avi.info, _cb_whole_query, pibmk,
			&pibmk->avi.err, pibmk->size, 1, 0)) {
		pibmk->is_querying = 0;
		DMSG(1, "QUERY FAILED\n");
		return -1;
	}

	return 0;
}

static int _process_manual_query_result(BMK_T *pibmk, int result)
{
	int elap;
	int size;

	size = pibmk->avi.info->total_size / 1024;	// Mbyte
	elap = fabs(pibmk->bmk_qry - pibmk->base);
	_calc_cps(pibmk, size, elap);
	_set_boundary(pibmk);
	_qry_send_avi_result(result, &pibmk->avi, QRY_BOOKMARK);
	return 0;
}

static int _process_message(BMK_T *pibmk, CMM_MESSAGE_T *pmsg)
{
	switch (pmsg->msgid) {
	case iNFY_BOOKMARK_UNIT_QUERY:
		pibmk->is_querying = 0;
		_process_unit_query_result(pibmk, pmsg->param);
		break;
	case iNFY_BOOKMARK_WHOLE_QUERY:
		pibmk->is_querying = 0;
		_process_whole_query_result(pibmk, pmsg->param);
		break;
	case iNFY_BOOKMARK_MANUAL:
		pibmk->is_querying = 0;
		_process_manual_query_result(pibmk, pmsg->param);
		break;
	case iNFY_REQUEST_QUERY:
		_process_query_request(pibmk);
		break;
	}

	if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
	return 0;
}

static int _wait_run_signal(BMK_T *pibmk)
{
	while (pibmk->work == READY) usleep(100000);
	return 0;
}

static int _wait_for_stop(BMK_T *pibmk)
{
	while (pibmk->stopped == NOTYET) usleep(100000);
	return 0;
}

static int _is_paused(BMK_T *pibmk)
{
	return (pibmk->work == PAUSE || pibmk->work == STOP);
}

static int _is_run(BMK_T *pibmk)
{
	return (pibmk->work == RUN);
}

static int _query_unit(BMK_T *pibmk)
{
	DMSG(9, "QUERY ID = %d\n", pibmk->is_querying);
	if (pibmk->is_querying)	return -1;

	memset(&pibmk->avi, 0x00, sizeof(AVI_QINFO_T));
	pibmk->avi.info = imalloc(sizeof(NF_ARCH_AVI_INFO));
	pibmk->aparam.cb_event = _cb_unit_query;
	pibmk->aparam.cb_context = pibmk;
	pibmk->bmk_qry = stm_get_time_t();
	pibmk->is_querying = 3;
	if (!nf_arch_query_avi_ex(&pibmk->aparam, pibmk->avi.info, 
			&pibmk->avi.err, pibmk->size, 1, 0)) {
		pibmk->is_querying = 0;
		return -1;
	}

	return 0;
}

static QRY_CODE_E _start_marking(BMK_T *pibmk)
{
	GError *err = NULL;
	QRY_CODE_E ret_val = QRY_SUCCESS;

	if (!nf_arch_pb_start_avi(&pibmk->pparam, &err)) {
		if (err) {
			ret_val = err->code;	
			g_error_free(err);
		}
		else ret_val = QRY_CODE_FAIL;
	}
	return ret_val;
}

static QRY_CODE_E _stop_marking()
{
	if (!nf_arch_pb_stop_avi(NULL)) return QRY_CODE_FAIL;
	return QRY_SUCCESS;
}

static int _destroy_marking()
{
	if (!nf_arch_info_destroy(NULL, NULL, NULL)) return -1;
	return 0;
}

static int _is_time_to_check(BMK_T *pibmk, time_t play)
{
	if (pibmk->is_overed) return 0;
//	if (pibmk->cps == 0) return 0;
	if (pibmk->lower >= play) return 1;
	if (pibmk->upper <= play) return 1;
	return 0;
}

static int _is_able_to_get_cps(BMK_T *pibmk, time_t play)
{
	// 5 sec
	if (fabs(play - pibmk->base) < 5) return 0;
	return 1;
}

static int _is_time_elapsed(BMK_T *pibmk, time_t cur)
{
	// this version does not support.
	//
	return 0;
}

static int _cleanup(BMK_T *pibmk)
{
	memset(&ibmk, 0x00, sizeof(BMK_T));
	return 0;
}

static int _is_calculated(BMK_T *pibmk)
{
	return pibmk->is_calculated;
}

static void* _bmk_service_proc(void *arg) 
{
	CMM_MESSAGE_T msg;
	BMK_T *pibmk = (BMK_T *)arg;
	time_t play;

	_wait_run_signal(pibmk);
	while (_is_work(pibmk)) {
		usleep(pibmk->sleep_time);

		if (cmm_get_message(&msg) == 0) _process_message(pibmk, &msg);
		if (_is_paused(pibmk)) { usleep(20000); continue; }

		play = stm_get_time_t();
		if (!_is_calculated(pibmk)) {
			if (!_is_able_to_get_cps(pibmk, play)) continue;

			_pause(pibmk);
			_query_unit(pibmk);
			pibmk->is_calculated = 1;
		}

		DMSG(1, "lower=%ld, upper=%ld, %ld\n", pibmk->lower, pibmk->upper, play);
		if (_is_time_to_check(pibmk, play)) {
			DMSG(9, "");
			_pause(pibmk);
			_query_whole(pibmk);
		}
	}
	pibmk->work = READY;
	pibmk->stopped = YES;

	_cleanup_thread(pibmk);
	pibmk->cmmpt = 0;
	g_thread_exit(NULL);
}

static int _init_thread(BMK_T *pibmk)
{
	pibmk->thd = ifn_make_thread(_bmk_service_proc, pibmk);
	cmm_mount_on_thread(pibmk->thd);
	pibmk->cmmpt = pibmk->thd;
	pibmk->sleep_time = 100000;
	pibmk->work = READY;
	pibmk->stopped = YES;
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _qry_init_bookmark()
{
	memset(&ibmk, 0x00, sizeof(BMK_T));
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

QRY_CODE_E qry_start_bookmark(NF_ARCH_PB_AVI_PARAM param, int limit)
{
	QRY_CODE_E ret = QRY_SUCCESS;

	memset(&ibmk, 0x00, sizeof(BMK_T));
	ibmk.cps = 0;
	ibmk.size = limit;
	ibmk.elap = 0;			// not used
	ibmk.base = stm_get_time_t();
	ibmk.pparam = param;

	_make_unit_param(&ibmk.aparam, &ibmk.pparam);
	_set_unit_start_time(&ibmk.aparam, ibmk.base);

	_init_thread(&ibmk);
	ret = _start_marking(&ibmk);
	_run(&ibmk);	
	return ret;
}

int qry_pause_bookmark()
{
	_pause(&ibmk);
	return 0;
}

extern/*inline*/ CMMPORT qry_get_cmmport()
{
	return ibmk.cmmpt;	
}

int qry_resume_bookmark()
{
	_run(&ibmk);
	return 0;
}

QRY_CODE_E qry_stop_bookmark()
{
	return _stop_marking();
}

int qry_exit_bookmark()
{
	int ret = 0;
	_stop_marking();
	ret = _destroy_marking();
	_stop(&ibmk);
	sleep(1);
	_wait_for_stop(&ibmk);
	_cleanup(&ibmk);
	return ret;
}

int qry_request_bookmark_info()
{
	if (!ibmk.cmmpt) return -1;
	cmm_send_message(ibmk.cmmpt, iNFY_REQUEST_QUERY, 0, 0, 0);
	return 0;
}
