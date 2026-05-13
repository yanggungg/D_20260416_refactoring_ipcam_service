/*
 * msm_internal.h
 * 	- mda internal header file
 * 	- not exposed to outside
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

#ifndef __MDA_INTERNAL_H
#define __MDA_INTERNAL_H

#include "mda.h"
#include "cmm.h"
#include "nf_api_archive.h"


#define MAX_DEVICE_CNT	(4)		// snf's setting
#define MAX_MEDIA_INFO	16



////////////////////////////////////////////////////////////
//
// protected data type 
//

typedef struct _DEVICE_REPORT_T {
	NF_ARCH_DEV_INFO 	xinfo[MAX_DEVICE_CNT];
	int					cnt_dinfo;
} DEVICE_REPORT_T;



////////////////////////////////////////////////////////////
//
// protected interfaces
//


// device

int _mda_init_dsr();
int _mda_cleanup_dsr();
int _mda_request_device_info();
int _mda_reload_device_data(int cnt);
int _mda_get_device_report(DEVICE_REPORT_T *dr, int *cnt);


// table

int _mda_init_mit();
int _mda_cleanup_mit();
int _mda_update_media_info();
MEDIA_INFO_T *_mda_new_media_list(int *ret_cnt);
int _mda_free_media_list(MEDIA_INFO_T *minfo);
int _mda_get_media_count();
int _mda_is_valid_media_id(MEDIA_ID id);
int _mda_is_mounted_media(MEDIA_ID id);
int _mda_get_mounted_path(MEDIA_ID id, char *path, int path_len);
MEDIA_TYPE_E _mda_get_media_type(MEDIA_ID id);
int _mda_get_device_id(MEDIA_ID id);
int _mda_mount_media(MEDIA_ID id, char *dir);
int _mda_umount_media(MEDIA_ID id);

#endif

