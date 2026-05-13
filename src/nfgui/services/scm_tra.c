/*
 * scm_tra.c
 * 	- scm transaction module
 *	- dependencies :
 *			GThread
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

#include "scm_internal.h"
#include "nfdal.h"
#include "iux_afx.h"
#include "nf_common.h"
#include "scm.h"
#include "ix_mem.h"
#include "wrk.h"
#include "ssm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_TRA"



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

static int _reset_chart(SCM_CHART_T *pchart)
{
	if (pchart->alloc_data) {
		ifree(pchart->alloc_data);
		pchart->alloc_data = 0;
	}
	memset(pchart, 0x00, sizeof(SCM_CHART_T));
	return 0;
}

static IMSG _release_tra(SCM_T *piscm, TRANSACTION_E tra)
{
	IMSG ret_msg = piscm->chart[tra].ret_msg;
	_reset_chart(&piscm->chart[tra]);
	return ret_msg;
}

static int _save_ttr(TRA_TRACK_T *ttr, IMSG rpl, TRANSACTION_E tra)
{
	int i;
	for (i = 0; i < MAX_TTR; ++i)
		if (ttr[i].rpl_msg == 0) break;

	ttr[i].rpl_msg = rpl;
	ttr[i].tra = tra;
	return 0;
}

TRANSACTION_E _track_ttr(TRA_TRACK_T *ttr, IMSG rpl)
{
	int i;
	for (i = 0; i < MAX_TTR; ++i)
		if (ttr[i].rpl_msg == rpl) return ttr[i].tra;

//	g_assert(0);
	return TRA_NONE;
}

int _drop_ttr(TRA_TRACK_T *ttr, IMSG rpl)
{
	int i;
	for (i = 0; i < MAX_TTR; ++i) {
		if (ttr[i].rpl_msg == rpl) {
			ttr[i].rpl_msg = 0;
			ttr[i].tra = 0;
		}
	}

	return 0;
}

static int _push_completion(TRANSACTION_E tra)
{
	switch (tra) {
	case TRA_IP_CHANGE:
	case TRA_SSL_INSTALL:
	case TRA_SSL_DELETE:
		DMSG(1, "");
		_scm_push_notification(INFY_NETCHANGE_API_CMPL, tra);
		break;

	case TRA_DB_IMPORT:
		_scm_push_notification(INFY_DBIMPORT_API_CMPL, tra);	
		break;

	case TRA_FACTORY_DEFAULT:
		DMSG(1, "");
		_scm_push_notification(INFY_FACDEF_API_CMPL, tra);
		break;

	case TRA_TIME_CHANGE:
		DMSG(1, "");
		_scm_push_notification(INFY_TIMECHANGE_API_CMPL, tra);
		break;

	case TRA_FW_UPGRADE:
	case TRA_DESIGN_UP:
	case TRA_IPCAM_FWUP_MODE:
		break;

	case TRA_ERASE_CH:
		break;

	case TRA_FORMAT:
		DMSG(1, "");
		_scm_push_notification(INFY_FORMAT_API_CMPL, tra);
		break;

	case TRA_RTL_SET:
	case TRA_RESTART_SERVICE:
		DMSG(1, "");
		_scm_push_notification(INFY_SVCRESTART_API_CMPL, tra);
		break;

	case TRA_SHUTDOWN:
		break;
	}

	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_mark_error_tra(SCM_T *piscm, TRANSACTION_E tra, ERROR_CODE_E er)
{
	piscm->chart[tra].err_code = er;
	return 0;
}

int _scm_track_tra(SCM_T *piscm, IMSG rpl_msg, TRANSACTION_E tra)
{
	_save_ttr(piscm->ttr, rpl_msg, tra);
	return 0;
}

TRANSACTION_E _scm_untrack_tra(SCM_T *piscm, IMSG rpl_msg)
{
	TRANSACTION_E tra = _track_ttr(piscm->ttr, rpl_msg);
	if (tra != TRA_NONE) _drop_ttr(piscm->ttr, rpl_msg);
	return tra;
}

int _scm_finalize_tra(SCM_T *piscm, TRANSACTION_E tra, int ret)
{
	CALLID callid = piscm->chart[tra].caller;
	void *data = piscm->chart[tra].result;
	int err_code = piscm->chart[tra].err_code;
	IMSG ret_msg;

	_push_completion(tra);
	ret_msg = _release_tra(piscm, tra);
	
	//	
	// released chart data of current tra
	//

	if (err_code != ER_NONE) ret = err_code;
	if (data) _scm_return_api(callid, ret_msg, ret, 1, data);
	else _scm_return_api(callid, ret_msg, ret, 0, 0);

	return 0;
}

int _scm_ready_tra(SCM_T *piscm, TRANSACTION_E tra, IMSG ret_msg, CALLID caller)
{
	_reset_chart(&piscm->chart[tra]);
	piscm->chart[tra].tra = tra;
	piscm->chart[tra].ret_msg = ret_msg;
	piscm->chart[tra].caller = caller;
	return 0;
}
