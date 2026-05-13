/*
 * qry.h
 * 	- bookmark archiving 
 * 	- instance type
 * 		: single instance
 *	- dependencies
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jun 23, 2011
 *
 */

#ifndef __QRY_H
#define __QRY_H

#include "nf_api_archive.h"
#include "cmm.h"


////////////////////////////////////////////////////////////////
//
// public data type
//

typedef enum _QRY_CODE_E {
	QRY_SUCCESS							= NF_ARCH_ERR_NONE,
	QRY_CODE_INV_COMMAND				= -NF_ARCH_ERR_INVMODE,
	QRY_CODE_INV_DEV					= -NF_ARCH_ERR_INVDEV,
	QRY_CODE_INV_PARAM					= -NF_ARCH_ERR_INVPARAM,
	QRY_CODE_INV_MEDIA					= -NF_ARCH_ERR_INVMEDIA,
	QRY_CODE_FAIL						= -NF_ARCH_ERR_FAIL,
	QRY_CODE_FAIL_WRITING				= -NF_ARCH_ERR_FAIL_DEVERR,
	QRY_CODE_FULL_LIST					= -NF_ARCH_ERR_LISTFULL,
	QRY_CODE_FAIL_LOCK					= -NF_ARCH_ERR_DATALOCKED,
	QRY_CODE_OVER_SIZE					= -NF_ARCH_ERR_MAX_AVAILABLE_SIZE,		// not used
	QRY_CODE_AUTO_TRIM					= -NF_ARCH_ERR_MAX_TIME_ADJUSTED,
	QRY_CODE_NO_VIDEODATA				= -NF_ARCH_ERR_NO_VIDEO_DATA,
} QRY_CODE_E;


////////////////////////////////////////////////////////////
//
// public interfaces
//

int qry_init();
QRY_CODE_E qry_query_avi(NF_ARCH_AVI_PARAM *param, int limit);
int qry_query_snap(NF_ARCH_SNAP_PARAM *param);
int qry_release_avi();
int qry_release_snap();


// bookmark
//
QRY_CODE_E qry_start_bookmark(NF_ARCH_PB_AVI_PARAM param, int limit);
int qry_pause_bookmark();
/*inline*/ CMMPORT qry_get_cmmport();
int qry_resume_bookmark();
QRY_CODE_E qry_stop_bookmark();
int qry_exit_bookmark();
int qry_request_bookmark_info();

#endif
