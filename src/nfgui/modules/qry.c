/*
 * qry.c
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

#include "qry.h"
#include "iux_msg.h"
#include "qry_internal.h"
#include "cmm.h"
#include "ix_mem.h"
#include "scm.h"

#include "iux_afx.h"
#include "nf_api_archive.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"QRY"


////////////////////////////////////////////////////////////
//
// private data types
//


typedef struct _QRY_T {
	NF_ARCH_AVI_PARAM  	aparam;
	NF_ARCH_SNAP_PARAM 	sparam;

	AVI_QINFO_T			avi;
	SNAP_QINFO_T		snap;

	int 				size;
} QRY_T;


////////////////////////////////////////////////////////////
//
// private variables
//

static QRY_T iqry;



////////////////////////////////////////////////////////////
//
// private functions
//
static int _send_message(IMSG msg, int param, int dyn_data, void* data)
{
	cmm_send_message(CMMPT_SCM, msg, param, dyn_data, data);
	return 0;
}

static int _send_result(int result, AVI_QINFO_T *pqry, QRY_TYPE_E qt)
{
	DMSG(1, "RESULT = %d, qt=%d\n", result, qt);
	QRY_CODE_E qr = (QRY_CODE_E)result;

	if (qr ==  QRY_SUCCESS) {
		_send_message(INFY_QUERY_SUCCESS, 0, 1, pqry->info);
	}
	else {
		if (qr == QRY_CODE_AUTO_TRIM) {
			_send_message(INFY_QUERY_OVER, 0, 1, pqry->info);
		}
		else if (qr == QRY_CODE_NO_VIDEODATA) {
			_send_message(INFY_QUERY_NO_VIDEODATA, 0, 1, pqry->info);
		}
		else {
			if (pqry->err == 0) {
				_send_message(INFY_QUERY_ERROR, -1, 0, (void *)qr);
				DMSG(1, "error code = 0, result = %d\n", qr);
			}
			else {
				_send_message(INFY_QUERY_ERROR, -1, 0, pqry->err->code);
				DMSG(1, "error code = 0x%x, result = %d\n", pqry->err->code, qr);
			}

			ifree(pqry->info);
			pqry->info = 0;
		}
	}

	if (pqry->err) g_error_free(pqry->err);
	pqry->info = 0;
	pqry->err = 0;

	return 0;
}

static int _send_snap_result(NF_ARCH_SNAP_INFO *snap_info)
{
	_send_message(INFY_SNAP_QUERY_SUCCESS, 0, 1, snap_info);
	return 0;
}

static void _cb_avi_query_cmpl(int result, void *context)
{
	_send_result(result, (AVI_QINFO_T *)context, QRY_NORMAL);
	return 0;
}

static int _release_prev_query_avi()
{
	nf_arch_info_destroy(NULL, NULL, NULL);
	return 0;
}

static int _release_prev_query_snap()
{
	nf_arch_info_destroy(NULL, NULL, NULL);

	if (iqry.snap.err) g_error_free(iqry.snap.err);
	iqry.snap.err = 0;
	iqry.snap.info = 0;
	return 0;
}

static QRY_CODE_E _query_avi(QRY_T *piqry)
{
	QRY_CODE_E ret_val = QRY_SUCCESS;

	memset(&iqry.avi, 0x00, sizeof(AVI_QINFO_T));
	piqry->aparam.cb_event = _cb_avi_query_cmpl;
	piqry->aparam.cb_context = &iqry.avi;
	iqry.avi.info = imalloc(sizeof(NF_ARCH_AVI_INFO));

	if (!nf_arch_query_avi_ex(&piqry->aparam, iqry.avi.info,
			&iqry.avi.err, iqry.size, 1, 0)) {
		if (iqry.avi.err) {
			ret_val = iqry.avi.err->code;
			g_error_free(iqry.avi.err);
		}
		else ret_val = QRY_CODE_FAIL;
	}

	return ret_val;
}

static int _query_snap(QRY_T *piqry)
{
	iqry.snap.info = imalloc(sizeof(NF_ARCH_SNAP_INFO));
	if (!nf_arch_query_snap(&piqry->sparam, iqry.snap.info, NULL)) {

		iqry.snap.err = 0;
		ifree(iqry.snap.info);
		iqry.snap.info = 0;
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _qry_send_avi_result(int result, AVI_QINFO_T *pqry, QRY_TYPE_E qt)
{
	DMSG(9, "");
	return _send_result(result, pqry, qt);
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int qry_init()
{
	memset(&iqry, 0x00, sizeof(QRY_T));
	_qry_init_bookmark();
	return 0;
}

QRY_CODE_E qry_query_avi(NF_ARCH_AVI_PARAM *param, int limit)
{
	QRY_CODE_E ret;
	iqry.size = limit;
	memcpy(&iqry.aparam, param, sizeof(NF_ARCH_AVI_PARAM));
	_release_prev_query_avi();
	ret = _query_avi(&iqry);
	return ret;
}

int qry_query_snap(NF_ARCH_SNAP_PARAM *param)
{
	memcpy(&iqry.sparam, param, sizeof(NF_ARCH_SNAP_PARAM));
	_release_prev_query_snap();
	if (_query_snap(&iqry) != 0) return -1;
	_send_snap_result(iqry.snap.info);
	// some more
	return 0;
}

int qry_release_avi()
{
	_release_prev_query_avi();
	return 0;
}

int qry_release_snap()
{
	_release_prev_query_snap();
	return 0;
}
