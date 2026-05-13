/*
 * mda.c
 * 	- media service manager
 *	- dependencies :
 *			GThread
 *			MIT
 *			DSR
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 11, 2011
 *
 */

#include "mda.h"
#include "nfdal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "mda_internal.h"
#include "cmm.h"
#include "scm.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"MDA"




////////////////////////////////////////////////////////////
//
// private data type 
//

typedef enum _RUN_STAT_E {
	STOP	= 0,
	RUN		= 1,
} RUN_STAT_E;


typedef struct _MDA_T {
	GThread			*thd;
	CMMPORT			cmmpt;
	RUN_STAT_E		run;

	// internal data
	int				sleep_time;

	char 			loose_path[MAX_PATH_LEN];
	MEDIA_ID		loose_id;	// loose media
} MDA_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static MDA_T imda;



////////////////////////////////////////////////////////////
//
// private functions
//


static int _cleanup_thread(MDA_T *pimda)
{
    if (cmm_mount_off_thread(pimda->cmmpt) == -1) return -1; 
    return 0;
}

static int _process_message(MDA_T *pimda, CMM_MESSAGE_T *pmsg)
{
	switch (pmsg->msgid) {
	case iRET_NF_ARCH_DEV_SET_NOTIFY:
		_mda_request_device_info();
		break;

	case iRET_NF_ARCH_DEV_GET_SIZE:
		_mda_reload_device_data(pmsg->param);
		_mda_update_media_info();
		cmm_send_message(CMMPT_SCM, INFY_MEDIA_STATUS_CHANGED, 0, 0, 0);
		break;
	}

	if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
	return 0;
}

static int _wait_run_signal(MDA_T *pimda)
{
	while (!pimda->run) usleep(10000);
	return 0;
}

static int _stop(MDA_T *pimda)
{
	pimda->run = STOP;
	return 0;
}

static int _run(MDA_T *pimda)
{
	pimda->run = RUN;
	return 0;
}

static void* _mda_service_proc(void *arg) 
{
	CMM_MESSAGE_T msg;
	MDA_T *pimda = (MDA_T *)arg;

	_wait_run_signal(pimda);
	while (1) {
		usleep(pimda->sleep_time);
		if (cmm_get_message(&msg) == 0) _process_message(pimda, &msg);
	}

	g_thread_exit(NULL);
}

static int _init_thread(MDA_T *pimda)
{
	pimda->thd = ifn_make_thread(_mda_service_proc, pimda);
	cmm_mount_on_thread(pimda->thd);
	pimda->cmmpt = pimda->thd;
	DMSG(1, "MDA CMMPT : [%p]\n", pimda->cmmpt);
	return 0;
}

static int _read_conf(MDA_T *pimda)
{
	pimda->sleep_time = 10000;//icf_get_value_by_int("mda", "thread_sleep_time");
	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int mda_init()
{
	memset(&imda, 0x00, sizeof(MDA_T));

	_read_conf(&imda);
	_init_thread(&imda);
	_mda_init_dsr();
	_mda_init_mit();
	_run(&imda);

	return 0;
}

int mda_cleanup()
{
	_cleanup_thread(&imda);
	return 0;
}

MEDIA_INFO_T *mda_new_media_list(int *ret_cnt)
{
	return _mda_new_media_list(ret_cnt);
}

int mda_free_media_list(MEDIA_INFO_T *minfo)
{
	return _mda_free_media_list(minfo);
}

int mda_get_media_count()
{
	return _mda_get_media_count();
}

bool mda_is_valid_media_id(MEDIA_ID id)
{
	return _mda_is_valid_media_id(id);
}

int mda_is_mounted_media(MEDIA_ID id)
{
	return _mda_is_mounted_media(id);
}

int mda_get_mounted_path(MEDIA_ID id, char *path, int path_len)
{
	return _mda_get_mounted_path(id, path, path_len);
}

MEDIA_TYPE_E mda_get_media_type(MEDIA_ID id)
{
	return _mda_get_media_type(id);
}

int mda_get_device_id(MEDIA_ID id)
{
	return _mda_get_device_id(id);
}

int mda_mount_media(MEDIA_ID id, char *dir)
{
	return _mda_mount_media(id, dir);
}

int mda_umount_media(MEDIA_ID id)
{
	return _mda_umount_media(id);
}

extern/*inline*/ CMMPORT mda_get_cmmport()
{
	return imda.cmmpt;	
}

int mda_loosen(MEDIA_ID id)
{
	int ret;

	if (imda.loose_id != 0) return -1;
	imda.loose_id = id;
	_mda_get_mounted_path(id, imda.loose_path, MAX_PATH_LEN);
	ret = _mda_umount_media(id);
	DMSG(9, "LOOSEN = %d\n", ret);
	return 0;
}

int mda_tighten()
{
	if (imda.loose_id == 0) return 0;
	_mda_mount_media(imda.loose_id, imda.loose_path); 
	imda.loose_id = 0;
	memset(imda.loose_path, 0x00, MAX_PATH_LEN);
	return 0;
}

int mda_is_loosened()
{
	return (imda.loose_id != 0);
}
