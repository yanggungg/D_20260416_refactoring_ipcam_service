/*
 * mda.h
 * 	- media servie manager
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

#ifndef __MDA_H
#define __MDA_H

#include "cmm.h"
#include "iux_afx.h"
#include "ix_func.h"

#define MAX_NAME_LEN	IUX_MAX_NAME_LEN
#define MAX_PATH_LEN	IUX_MAX_PATH_LEN


////////////////////////////////////////////////////////////
//
// public data type 
//

typedef unsigned int 	MEDIA_ID;		// 0: error, valid from 1.

typedef struct _MEDIA_INFO_T {
	MEDIA_ID	id;
	char		title[MAX_NAME_LEN + 1];
} MEDIA_INFO_T;

typedef enum _MEDIA_TYPE_E {
	MTYPE_ERR		= -1,
	MTYPE_USB		= 0,
	MTYPE_ODD		= 1,
	MTYPE_FTP		= 2
} MEDIA_TYPE_E;

typedef enum _FWFILE_FILTER_E {
	FF_NOFILTER		= 0x0,
	FF_FNAME_EXT	= 0x1,
	FF_MODEL		= 0x2,
	FF_BUYERCODE	= 0x4,
	FF_UPPERVER		= 0x8
} FWFILE_FILTER_E;

typedef enum _FWFILE_OPT_E {	// bitwise
	FO_DEFAULT				= IFO_NONE,
	FO_INCLUDE_FULLPATH		= IFO_INCLUDE_FULLPATH,
	FO_RECURSIVE_SEARCH		= IFO_RECURSIVE_SEARCH,
} FWFILE_OPT_E;


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int mda_init();
int mda_cleanup();
MEDIA_INFO_T *mda_new_media_list(int *ret_cnt);
int mda_free_media_list(MEDIA_INFO_T *minfo);
int mda_get_media_count();
bool mda_is_valid_media_id(MEDIA_ID id);
int mda_is_mounted_media(MEDIA_ID id);
int mda_get_mounted_path(MEDIA_ID id, char *path, int path_len);
MEDIA_TYPE_E mda_get_media_type(MEDIA_ID id);
int mda_get_device_id(MEDIA_ID id);
int mda_mount_media(MEDIA_ID id, char *dir);
int mda_umount_media(MEDIA_ID id);
/*inline*/ CMMPORT mda_get_cmmport();
int mda_loosen(MEDIA_ID id);
int mda_tighten();
int mda_is_loosened();

#endif
