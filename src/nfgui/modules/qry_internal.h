/*
 * qry_internal.h
 * 	- bookmark archiving 
 * 	- instance type
 * 		:
 *	- dependencies
 *		:
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jun 23, 2011
 *
 */

#ifndef __QRY_INTERNAL_H
#define __QRY_INTERNAL_H

#include "nf_api_archive.h"


////////////////////////////////////////////////////////////
//
// protected data type 
//

typedef enum _QRY_TYPE_E {
	QRY_NORMAL		= 0,
	QRY_BOOKMARK	= 1
} QRY_TYPE_E;

typedef struct _AVI_QINFO_T {
	NF_ARCH_AVI_INFO 	*info;
	GError 				*err;
} AVI_QINFO_T;

typedef struct _SNAP_QINFO_T {
	NF_ARCH_SNAP_INFO 	*info;
	GError 				*err;
} SNAP_QINFO_T;



////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _qry_send_avi_result(int result, AVI_QINFO_T *pqry, QRY_TYPE_E qt);
int _qry_init_bookmark();

#endif
