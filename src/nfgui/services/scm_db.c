/*
 * scm_db.c
 * 	- scm db service
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 6, 2011
 *
 */

#include "scm_internal.h"
#include "mda.h"
#include "ssm.h"
#include "ix_func.h"
#include "nf_common.h"
#include "nfdal.h"
#include "ix_mem.h"
#include "scm.h"
#include "vfs.h"
#include "support/multi_language_support.h"
#include "nf_util_time.h"
#include "evt.h"
#include "nf_issm_ctl.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_DB"


////////////////////////////////////////////////////////////
//
// private functions
//

static int _filter_db_file(char *file_name, void *filter_param) 
{
	int len;
	len = strlen(file_name);
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;

	if (len < 5) return 0;
	if (g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) return 0;

	return 1;
}

static char **_new_db_list(MEDIA_ID id, int *ret_cnt)
{
	char **flist;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".ndb");
	filter_param.condition = FF_NOFILTER;
	flist = ifn_new_filelist(mnt_path, _filter_db_file, &filter_param, ret_cnt);

	return flist;
}

static char **_new_audio_list(MEDIA_ID id, int *ret_cnt)
{
	char **flist;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".wav");
	filter_param.condition = FF_NOFILTER;
	flist = ifn_new_filelist(mnt_path, _filter_db_file, &filter_param, ret_cnt);

	return flist;
}

static int _apply_newdb_to_system()
{
	DAL_save_db_all();
	return 0;
}

static int _apply_db_by_fac_default()
{
	gint index;
	for (index = 0; index < NF_SYSDB_CATE_NR; index++)
		nf_notify_fire_params("sysdb_change", index, 0, 0, 0);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_FACTORY_DEFAULT, 0, 0, 0);

	return 0;
}

static int _start_factory_default(CALLID callid, TRANSACTION_E tra)
{
	gchar strLang[32];

	if (callid == LOCAL_CALL) {
	if (!DAL_factory_default()) return -1;
	}
	else if (callid == WEB_CALL) {
		if (!DAL_factory_default_without_network()) return -1;
	}
	else return -1;

	_scm_init_ddns_hostname();
	_apply_db_by_fac_default();

	return 0;
}

static int _load_db(CALLID callid, TRANSACTION_E tra)
{
	char *full_path = iscm.chart[tra].alloc_data;
	if (!ifn_is_file_exist(full_path)) return -1;
	if (callid == LOCAL_CALL) DAL_system_data_load(full_path);
	else if (callid == WEB_CALL) DAL_system_data_load_without_network(full_path);

	return 0;
}	

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_get_cur_lang(char *buf, int buf_len)
{
	OsdData osddata;
	int len;
	memset(&osddata, 0x00, sizeof(osddata));

	DAL_get_osd_data(&osddata);

	len = strlen(osddata.lang);
	if (len >= buf_len) return -1;
	if (len == 0) strcpy(buf, "ENGLISH");
	else strcpy(buf, osddata.lang);
	return 0;
}

int _scm_work_factory_default(SCM_T *piscm, TRANSACTION_E tra)
{
	int ret;
	CALLID callid = piscm->chart[tra].caller;

	DMSG(9, "");
	ret = _start_factory_default(callid, tra);
	if (ret < 0) _scm_mark_error_tra(piscm, tra, ER_DB_FACTORYING);
	ret = _apply_newdb_to_system();
	if (ret < 0) _scm_mark_error_tra(piscm, tra, ER_DB_APPLYING);

	_scm_init_audio(piscm);
	_scm_init_timesync(piscm);
	ssm_reconfig_auto_logout();
	nf_zoneinfo_init();
	nf_netif_init();
	nf_issm_ctl(ISSM_STOP, __FUNCTION__);
	nf_issm_ctl(ISSM_START, __FUNCTION__);
	_scm_change_wmode_by_db(piscm);

	_scm_work_service_start(piscm, tra);

	return 0;
}

int _scm_work_db_import(SCM_T *piscm, TRANSACTION_E tra)
{
	int ret;
	CALLID callid = piscm->chart[tra].caller;

	DMSG(9, "");
	ret = _load_db(callid, tra);
	if (ret < 0) _scm_mark_error_tra(piscm, tra, ER_DB_LOADING);
	ret = _apply_newdb_to_system();
	if (ret < 0) _scm_mark_error_tra(piscm, tra, ER_DB_APPLYING);

	_scm_init_audio(piscm);
	_scm_init_timesync(piscm);
	ssm_reconfig_auto_logout();
	nf_zoneinfo_init();
	nf_netif_init();
	nf_issm_ctl(ISSM_STOP, __FUNCTION__);
	nf_issm_ctl(ISSM_START, __FUNCTION__);
	_scm_change_wmode_by_db(piscm);
	evt_send_to_local(IREQ_CHANGE_LANG, 0, 0, 0);

	_scm_work_service_start(piscm, tra);

	return 0;
}

int _scm_finalize_db_change(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(9, "");
	_scm_inform_db_changing(tra);
	_scm_finalize_tra(piscm, tra, 0);
	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

char **scm_new_db_list(MEDIA_ID id, int *ret_cnt)
{
	return _new_db_list(id, ret_cnt);
}

char **scm_new_audio_list(MEDIA_ID id, int *ret_cnt)
{
	return _new_audio_list(id, ret_cnt);
}

int scm_import_db(char *full_path, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DB_IMPORT;
	if (!ifn_is_file_exist(full_path)) { DMSG(1, ""); return -1; }
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.chart[tra].alloc_data = imalloc(strlen(full_path)+ 1);
	strcpy(iscm.chart[tra].alloc_data, full_path);

	_scm_push_notification(INFY_DBIMPORT_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, RS_DBIMPORT);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_export_db(char *full_path)
{
	gboolean ret;

	DMSG(1, "FULLPATH: [%s]\n", full_path);
	ret = DAL_system_data_save(full_path);
	
	if (ret == TRUE) {
		if (IUX_CALLER() == LOCAL_CALL) scm_put_log(DB_SAVE, 0, 0);
		else _scm_put_log(DB_SAVE, 0, "WEB Viewer");
	}
	return (ret == TRUE ? 0 : -1);	
}

int scm_run_factory_default(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FACTORY_DEFAULT;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_push_notification(INFY_FACDEF_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, RS_FACDEF);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

