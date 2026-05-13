/*
 * msm_mit.c
 * 	- media information table
 *	- dependencies :
 *			GThread
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

//#include "nfdal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "mda.h"
#include "mda_internal.h"
#include <string.h>


#define DBG_LEVEL		9
#define DBG_MODULE		"MDA_TABLE"


#define MSMIT_LOCK()		g_mutex_lock(imit.mtx)
#define MSMIT_UNLOCK()		g_mutex_unlock(imit.mtx)


////////////////////////////////////////////////////////////
//
// private constans
//

//static const char *_fs_type[3] = { "ntfs", "iso9660", "ftp" };  
static const char *_fs_type[3] = { "vfat", "iso9660", "ftp" };  



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _DEVICE_INFO_T {
	int 			id;
	char			dev_name[MAX_PATH_LEN + 1];
	char			dev_path[MAX_PATH_LEN + 1];
	MEDIA_TYPE_E	type;
	char 			mnt_path[MAX_PATH_LEN + 1];
	int				mounted;		// 1:mounted, 0:unmounted
	gchar			dev_id;
	guint64			tot_space;
	guint64			free_space;
} DEVICE_INFO_T;


typedef struct _MIT_T {			// media information table
	GMutex			*mtx;

	MEDIA_INFO_T 	minfo[MAX_MEDIA_INFO];
	DEVICE_INFO_T	dinfo[MAX_MEDIA_INFO];
	MEDIA_ID		next_id;
	int				count;
} MIT_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static MIT_T imit;


////////////////////////////////////////////////////////////
//
// private functions - tools
//


static int _make_media_title(char *product, char *dev_type, char *title)
{
	int len;
	char trim_product[MAX_NAME_LEN];

	ifn_strtrim_b(product, trim_product);
	len = strlen(trim_product);

	if (strlen(product) == 0 || len == 0) {
		if(dev_type == 0) strcpy(title, "USB");
		else strcpy(title, "CD/DVD");
	}
	else strcpy(title, product);
}

static int _get_media_type(int raw_dev_type)
{
	switch (raw_dev_type) {
	case 0: 	return MTYPE_USB;
	case 5: 	return MTYPE_ODD;
	case 153: 	return MTYPE_FTP;
	}

	return 0;
}

static int _make_device_path(char *dev_name, char *dev_path)
{
	sprintf(dev_path, "/dev/%s", dev_name);
	return 0;
}

static int _make_mount_path(char *dev_name, char *mnt_path)
{
	sprintf(mnt_path, "/mnt/%s", dev_name);
	return 0;
}

static int _umount_media(DEVICE_INFO_T *dinfo)
{
	g_warning("%s %s(%s)",__FUNCTION__, dinfo->dev_path, dinfo->mnt_path);	
	if (ifn_unmount_device(dinfo->dev_path, dinfo->mnt_path) == -1) return -1;
	memset(dinfo->mnt_path, 0x00, MAX_PATH_LEN + 1);
	dinfo->mounted = 0;
	return 0;
}

static int _mount_device(DEVICE_INFO_T *dinfo, char *dir)
{
	if (dinfo->type == MTYPE_FTP) {
		// for future
		//
	}
	else if (dinfo->type == MTYPE_ODD) {
		// for future
		//
	}
	else {
		if (ifn_make_dir_p(dir) == -1) return -1;

		if (ifn_mount_device(dinfo->dev_path, dir, _fs_type[dinfo->type]) == -1) {
			g_warning("%s try1 failed %s(%s)",__FUNCTION__, dinfo->dev_path, dinfo->mnt_path);
			dinfo->dev_path[8] = 0;		// remove numeric end of dev_path "/dev/sdaX"
			if (ifn_mount_device(dinfo->dev_path, dir, _fs_type[dinfo->type]) == -1) {
				g_warning("%s try2 failed %s(%s)",__FUNCTION__, dinfo->dev_path, dinfo->mnt_path);
				return -1;
			}
		}
		strcpy(dinfo->mnt_path, dir);
		dinfo->mounted = 1;
	}
	return 0;
}

static int _is_empty_media_slot(MEDIA_INFO_T *pminfo)
{
	return (pminfo->id == 0);
}

static int _fill_minfo(MEDIA_ID id, MEDIA_INFO_T *minfo, NF_ARCH_DEV_INFO *ndinfo)
{
	char title[MAX_NAME_LEN];

	minfo->id = id;
	_make_media_title(ndinfo->product, ndinfo->dev_type, title);
	strcpy(minfo->title, title);
	return 0;
}

static int _fill_dinfo(MEDIA_ID id, DEVICE_INFO_T *dinfo, NF_ARCH_DEV_INFO *ndinfo)
{
	char dev_path[MAX_PATH_LEN + 1];
	char mnt_path[MAX_PATH_LEN + 1];

	_make_device_path(ndinfo->dev_name, dev_path);
	//_make_mount_path(ndinfo->dev_name, mnt_path);

	dinfo->id = id;
	strcpy(dinfo->dev_name, ndinfo->dev_name);
	strcpy(dinfo->dev_path, dev_path);
	//strcpy(dinfo->mnt_path, mnt_path);
	memset(dinfo->mnt_path, 0x00, sizeof(dinfo->mnt_path));
	dinfo->type = _get_media_type(ndinfo->dev_type);
	dinfo->dev_id = ndinfo->dev_id;
	dinfo->tot_space = ndinfo->media_size;
	dinfo->free_space = ndinfo->media_avail;
	dinfo->mounted = 0;
	return 0;
}

static int _init_mutex()
{
	imit.mtx = g_mutex_new();
	return 0;
}

static int _cleanup_mutex()
{
	g_mutex_free(imit.mtx);
	return 0;
}


////////////////////////////////////////////////////////////
//
// private functions - major
//

static int _find_matched_device(MIT_T *pimit, NF_ARCH_DEV_INFO *ndinfo)
{
	int i;
	
	for (i = 0; i < MAX_MEDIA_INFO; ++i) {
		if (_is_empty_media_slot(&pimit->minfo[i])) continue;
		if (strcmp(pimit->dinfo[i].dev_name, ndinfo->dev_name) == 0) return i;
	}

	return -1;
}

static int _erase_media_info(MIT_T *pimit, int idx)
{
	_umount_media(&pimit->dinfo[idx]);
	memset(&pimit->minfo[idx], 0x00, sizeof(MEDIA_INFO_T));
	memset(&pimit->dinfo[idx], 0x00, sizeof(DEVICE_INFO_T));

	pimit->count--;
	return 0;
}

static int _remove_undetected_dev(MIT_T *pimit, DEVICE_REPORT_T *dr, int cnt)
{
	int i, j;
	char dev_name[MAX_PATH_LEN + 1];

	for (i = 0; i < MAX_MEDIA_INFO; ++i) {
		if (_is_empty_media_slot(&pimit->minfo[i])) continue;
		for (j = 0; j < dr->cnt_dinfo; ++j) {
	
			strcpy(dev_name, dr->xinfo[j].dev_name);
			if (strcmp(pimit->dinfo[i].dev_name, dev_name) == 0) break;
		}

		if (j == dr->cnt_dinfo) _erase_media_info(pimit, i);
	}
	return 0;
}

static int _remove_all_dev(MIT_T *pimit)
{
	int i, j;

	for (i = 0; i < MAX_MEDIA_INFO; ++i) {
		if (_is_empty_media_slot(&pimit->minfo[i])) continue;
		_erase_media_info(pimit, i);
	}
	return 0;
}

static int _find_empty_media_slot(MIT_T *pimit)
{
	int i;
	for (i = 0; i < MAX_MEDIA_INFO; ++i) {
		if (_is_empty_media_slot(&pimit->minfo[i])) return i;
	}

	return -1;
}

static int _refresh_media_info(MIT_T *pimit, DEVICE_REPORT_T *dr, int cnt)
{
	int i, j;
	int idx;
	int ret = 0;
	char mnt_path[MAX_PATH_LEN + 1];

	_remove_all_dev(pimit);

	for (i = 0; i < cnt; ++i) {
		idx = _find_empty_media_slot(pimit);
		_make_mount_path(&dr->xinfo[i].dev_name, mnt_path);

		_fill_dinfo(pimit->next_id, &pimit->dinfo[idx], &dr->xinfo[i]);
		for (j = 0; j < 3; ++j) {	// retry
			ret = _mount_device(&pimit->dinfo[idx], mnt_path);
			if (ret == 0) break;
		}
		if (ret == -1) { DMSG(1, "UNABLE TO MOUNT"); continue; }
		_fill_minfo(pimit->next_id, &pimit->minfo[idx], &dr->xinfo[i]);

		pimit->next_id++;
		pimit->count++;
	}
	return pimit->count;
}

static int _find_media_index(MIT_T *pimit, MEDIA_ID id)
{
	int i;
	for (i = 0; i < MAX_MEDIA_INFO; ++i) {
		if (pimit->minfo[i].id == id) return i;
	}

	return -1;
}

static int _update_media_info_table(MIT_T *pimit)
{
	DEVICE_REPORT_T dr;
	int cnt;

	memset(&dr, 0x00, sizeof(DEVICE_REPORT_T));
	_mda_get_device_report(&dr, &cnt);
	_refresh_media_info(pimit, &dr, cnt);

	return pimit->count;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//


int _mda_init_mit()
{
	memset(&imit, 0x00, sizeof(MIT_T));
	_init_mutex();
	imit.next_id = 1;		// 1 is the first id
	return 0;
}

int _mda_cleanup_mit()
{
	memset(&imit, 0x00, sizeof(MIT_T));
	_cleanup_mutex();
	return 0;
}

int _mda_update_media_info()
{
	int cnt;

	MSMIT_LOCK();
	cnt = _update_media_info_table(&imit);
	MSMIT_UNLOCK();

	return cnt;
}

MEDIA_INFO_T *_mda_new_media_list(int *ret_cnt)
{
	MEDIA_INFO_T *minfo;
	int i, j;

	if (ret_cnt) *ret_cnt = 0;

	MSMIT_LOCK();
	if (imit.count == 0) {
		MSMIT_UNLOCK();
		return 0;
	}

	minfo = imalloc(imit.count * sizeof(MEDIA_INFO_T));
	for (i = 0, j = 0; i < MAX_MEDIA_INFO; ++i) {
		if (_is_empty_media_slot(&imit.minfo[i])) continue;

		minfo[j] = imit.minfo[i];
		++j;
	}
	if (ret_cnt) *ret_cnt = imit.count;
	MSMIT_UNLOCK();

	return minfo;
}

int _mda_free_media_list(MEDIA_INFO_T *minfo)
{
	if (minfo == 0) return -1;
	ifree(minfo);
	return 0;
}

int _mda_get_media_count()
{
	int ret;

	MSMIT_LOCK();
	ret = imit.count;
	MSMIT_UNLOCK();

	return ret;
}

int _mda_is_valid_media_id(MEDIA_ID id)
{
	int idx;
	int ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return 0; }
	MSMIT_UNLOCK();

	return 1;
}

int _mda_is_mounted_media(MEDIA_ID id)
{
	int idx;
	int ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return 0; }
	ret = ifn_is_mounted_dev(imit.dinfo[idx].dev_path);
	MSMIT_UNLOCK();

	DMSG(9, "IS_MOUNTED = %d\n", ret);
	return ret;
}

int _mda_get_mounted_path(MEDIA_ID id, char *path, int path_len)
{
	int idx;
	memset(path, 0x00, path_len);

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return -1; }

	DMSG(9, "DEV PATH = [%s]\n", imit.dinfo[idx].dev_path);
	if (!ifn_is_mounted_dev(imit.dinfo[idx].dev_path)) { 
		MSMIT_UNLOCK(); 
		strncpy(path, "/tmp/mnt_error", path_len); // for ntfs workaround  choissi
		return -1; 
	}
	strncpy(path, imit.dinfo[idx].mnt_path, path_len);
	path[path_len - 1] = 0;
	DMSG(9, "MOUNTED PATH = [%s]\n", path);
	MSMIT_UNLOCK();

	return 0;
}

MEDIA_TYPE_E _mda_get_media_type(MEDIA_ID id)
{
	int idx;
	MEDIA_TYPE_E ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return -1; }

	ret = imit.dinfo[idx].type;
	MSMIT_UNLOCK();

	return ret;
}

int _mda_get_device_id(MEDIA_ID id)
{
	int idx;
	int ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return -1; }

	ret = imit.dinfo[idx].dev_id;
	MSMIT_UNLOCK();

	return ret;
}

int _mda_mount_media(MEDIA_ID id, char *dir)
{
	int idx;
	int ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return -1; }

	ret = _mount_device(&imit.dinfo[idx], dir);
	MSMIT_UNLOCK();
	
	return ret;
}

int _mda_umount_media(MEDIA_ID id)
{
	int idx;
	int ret;

	MSMIT_LOCK();
	idx = _find_media_index(&imit, id);
	if (idx == -1) { MSMIT_UNLOCK(); return -1; }

	ret = _umount_media(&imit.dinfo[idx]);
	MSMIT_UNLOCK();
	
	DMSG(9, "UMOUNT ret = %d\n", ret);
	return ret;
}
