/*
 * scm_reserve.c
 * 	- scm reserved data support module
 *	- dependencies :
 *
 *
 * Written by eddy.kim
 * Copyright (c) ITX security, Apr 6, 2011
 *
 */

#include "iux_afx.h"
#include "scm_internal.h"
#include "nf_api_archive.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "qry.h"
#include "scm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_RESERVE"

#define LIMIT_20G					(20 * 1024)



/////////////////////////////////////////////////////////////
//
// private variables
//

gboolean bookmark = FALSE;



////////////////////////////////////////////////////////////
//
// private interfaces
//

static int _get_avi_list_count()
{
	gint ret;
	ret = nf_arch_list_get_count(NF_ARCH_TYPE_AVI, NULL, NULL);
	if (ret < 0 && ret != -10) return ret;
	return (ret == -10) ? 0 : ret;		// -10 means empty
}

static int _get_snap_list_count()
{
	gint ret;
	ret = nf_arch_list_get_count(NF_ARCH_TYPE_SNAP, NULL, NULL);
	if (ret < 0 && ret != -10) return ret;
	return (ret == -10) ? 0 : ret;		// -10 means empty
}

static int _get_preserve_list_count()
{
	gint ret = 0;
	ret = nf_disk_preserve_list_cnt();
	//if (ret < 0 && ret != -10) return ret;
	//return (ret == -10) ? 0 : ret;		// -10 means empty
	return ret;
}

static int _get_bookmark_time_txt(guint64 beg, guint64 end, char *ext, char *buf)
{
	time_t tmp;
	char tbeg[128];
	char tend[128];

	tmp = ifn_convert_guint64_to_timet(beg);
	dtf_get_localtime_text(tmp, YYYYMMDD, H24, tbeg);
	tmp = ifn_convert_guint64_to_timet(end);
	dtf_get_localtime_text(tmp, YYYYMMDD, H24, tend);

	sprintf(buf, "%s: %s - %s", ext, tbeg, tend);
	return 0;
}

static int _get_snap_time_txt(guint64 img_time, char *ext, char *buf)
{
	time_t tmp;
	char tbeg[128];

	tmp = ifn_convert_guint64_to_timet(img_time);
	dtf_get_localtime_text(tmp, YYYYMMDD, H24, tbeg);

	sprintf(buf, "%s: %s", ext, tbeg);
	return 0;
}

static int _get_reserved_data_time(guint16 arch_id, guint64 *beg, guint64 *end)
{
	NF_ARCH_AVI_INFO data;

	if(!nf_arch_list_get_avi(NF_ARCH_DIR_FORWARD, arch_id, 1, &data, NULL, NULL))
		return -1;

	*beg = data.time_beg;
	*end = data.time_end;
	return 0;
}

static RSV_CODE_E _reserve_avi_data(SCM_T *piscm)
{
	GError *err = NULL;
	RSV_CODE_E ret_val = RSV_SUCCESS;
	char timetxt[256];

	if (!nf_arch_info_add(NULL, NULL, &err)) {
		if (err) {
			ret_val = err->code;
			g_error_free(err);
			return ret_val;
		}
		else return RSV_CODE_FAIL;
	}

	_get_bookmark_time_txt(
			piscm->qry_result.time_beg,
			piscm->qry_result.time_end, "RESERVED VIDEO", timetxt);

	DMSG(1, "[%s]", timetxt);
	_scm_put_log(DATA_RESV, 0, timetxt);
	return ret_val;
}

static RSV_CODE_E _reserve_snap_data(SCM_T *piscm)
{
	GError *err = NULL;
	RSV_CODE_E ret_val = RSV_SUCCESS;
	char timetxt[256];

	if (!nf_arch_info_add(NULL, NULL, &err)) {
		if (err) {
			ret_val = err->code;
			g_error_free(err);
			return ret_val;
		}
		else return RSV_CODE_FAIL;
	}
	_get_snap_time_txt(
			piscm->qry_snap_result.time_image,
			"RESERVED IMAGE", timetxt);

	DMSG(1, "[%s]", timetxt);
	_scm_put_log(DATA_RESV, 0, timetxt);
	return ret_val;
}

static int _free_snap_image(NF_ARCH_SNAP_INFO  *info)
{
	if (!info) return -1;
	if (!info->pimage) return -1;

	free(info->pimage);	// to free the memory allocated by arch_manager
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

NF_ARCH_AVI_INFO *scm_new_avi_list(int *ret_cnt)
{
	NF_ARCH_AVI_INFO *avi;
	int cnt;
	int ret;

	if (ret_cnt) *ret_cnt = 0;
	cnt = _get_avi_list_count();
	if (cnt <= 0) return 0;
	avi = imalloc(sizeof(NF_ARCH_AVI_INFO) * cnt);
	ret = nf_arch_list_get_avi(NF_ARCH_DIR_FORWARD, 0, cnt, avi, NULL, NULL);
	if (ret == 0) {
		ifree(avi);
		if (ret_cnt) *ret_cnt = 0;
		return 0;
	}
	if (ret_cnt) *ret_cnt = cnt;

	return avi;
}

int scm_free_avi_list(NF_ARCH_AVI_INFO *avi)
{
	if (avi) ifree(avi);
	return 0;
}

int scm_get_avi_info(NF_ARCH_AVI_INFO *avi, guint16 avi_id)
{
	gint ret;
	ret = nf_arch_list_get_avi(NF_ARCH_DIR_FORWARD, avi_id, 1, avi, NULL, NULL);
	return (ret == 1) ? 0 : -1;
}

int scm_get_avi_list_count()
{
	return _get_avi_list_count();
}

int scm_delete_avi(guint16 id)
{
	char timetxt[256];
	guint64 beg;
	guint64 end;
	gint ret;

	if (_get_reserved_data_time(id, &beg, &end) < 0) return -1;
	ret = nf_arch_list_delete(NF_ARCH_TYPE_AVI, id, NULL, NULL, NULL);
	if (ret == FALSE) return -1;

	DMSG(1, "");
	_get_bookmark_time_txt(beg, end, "REMOVED DATA", timetxt);
	DMSG(1, "%s", timetxt);
	_scm_put_log(REMOVE_DATA_RESV, 0, timetxt);
	return 0;
}

NF_ARCH_SNAP_INFO *scm_new_snap_list(int *ret_cnt)
{
	NF_ARCH_SNAP_INFO *snap;
	int cnt;
	int ret;

	if (ret_cnt) *ret_cnt = 0;
	cnt = _get_snap_list_count();
	if (cnt <= 0) return 0;
	snap = imalloc(sizeof(NF_ARCH_SNAP_INFO) * cnt);
	ret = nf_arch_list_get_snap(NF_ARCH_DIR_FORWARD, 0, cnt, TRUE, snap, NULL, NULL);
	if (ret != 0) {
		ifree(snap);
		if (ret_cnt) *ret_cnt = 0;
		return 0;
	}
	if (ret_cnt) *ret_cnt = cnt;
	return snap;
}

int scm_free_snap_list(NF_ARCH_SNAP_INFO *snap, int count)
{
	int i;
	if (!snap) return -1;
	for (i = 0; i < count; ++i) {
		if (snap[i].pimage) _free_snap_image(&snap[i]);
	}

	ifree(snap);
	return 0;
}

int scm_free_snap_image(NF_ARCH_SNAP_INFO  *info)
{
	_free_snap_image(info);
	return 0;
}

int scm_get_snap_list_count()
{
	return _get_snap_list_count();
}

int scm_delete_snap(guint16 id)
{
	gint ret;
	ret = nf_arch_list_delete(NF_ARCH_TYPE_SNAP, id, NULL, NULL, NULL);
	return (ret == TRUE) ? 0 : -1;
}

NF_DISK_PRESERVE_INFO *scm_new_preserve_list(int *ret_cnt)
{
	NF_DISK_PRESERVE_INFO *preserve;
	int cnt;
	int ret;

	if (ret_cnt) *ret_cnt = 0;
	cnt = _get_preserve_list_count();
	g_message("[%s]line(%d) , cnt = %d", __FUNCTION__, __LINE__, cnt);
	if (cnt <= 0) return 0;

	preserve = imalloc(sizeof(NF_DISK_PRESERVE_INFO) * cnt);
	ret = nf_disk_preserve_list_get(0, cnt, preserve);
	if (ret != 0) {
		ifree(preserve);
		if (ret_cnt) *ret_cnt = 0;
		return 0;
	}
	if (ret_cnt) *ret_cnt = cnt;

	return preserve;
}

int scm_get_preserve_list_count()
{
	return _get_preserve_list_count();
}

int scm_free_preserve_list(NF_DISK_PRESERVE_INFO *preserve)
{
	if (preserve) ifree(preserve);
	return 0;
}


int scm_get_avi_reserved_data(guint16 arch_id, guint16 count, NF_ARCH_AVI_INFO *data)
{
	if (!nf_arch_list_get_avi(NF_ARCH_DIR_FORWARD, arch_id, count, data, NULL, NULL))
		return -1;

	return 0;
}

int scm_get_snap_reserved_data(guint16 arch_id, guint16 count, NF_ARCH_SNAP_INFO *data)
{
	if (!nf_arch_list_get_snap(NF_ARCH_DIR_FORWARD, arch_id, count, TRUE, data, NULL, NULL))
		return -1;

	return 0;
}

int scm_get_preserve_reserved_data(guint16 preserve_id, guint16 count, NF_DISK_PRESERVE_INFO *data)
{
	if (!nf_disk_preserve_list_get(preserve_id, count, data))
		return -1;

	return 0;
}

int scm_get_reserved_data_count(NF_ARCH_TYPE_E type)
{
	return nf_arch_list_get_count(type, NULL, NULL);
}

RSV_CODE_E scm_reserve_avi()
{
	RSV_CODE_E ret;
	ret = _reserve_avi_data(&iscm);
	DMSG(1, "return = %d\n", ret);
	return ret;
}

int scm_delete_reserved_data(NF_ARCH_TYPE_E type, guint16 arch_id)
{
	char timetxt[256];
	guint64 beg;
	guint64 end;
	int ret;

	if (_get_reserved_data_time(arch_id, &beg, &end) < 0) return -1;
	if (!nf_arch_list_delete(type, arch_id, NULL, NULL, NULL))
		return -1;

	_get_bookmark_time_txt(beg, end, "REMOVED DATA", timetxt);
	_scm_put_log(REMOVE_DATA_RESV, 0, timetxt);
	return 0;
}

int scm_set_arch_info(gchar *tag, gchar *user, gchar *memo)
{
	if(nf_arch_info_modify(tag, user, memo))
		return 0;

	return -1;
}

QRY_CODE_E scm_start_avi_query(NF_ARCH_AVI_PARAM *avi_param)
{
	return qry_query_avi(avi_param, LIMIT_20G);
}

int scm_start_snapshot_query(NF_ARCH_SNAP_PARAM *snap_param, NF_ARCH_SNAP_INFO *snap_info)
{
	return qry_query_snap(snap_param);
}

int scm_end_query()
{
	return qry_release_avi();
}

int scm_end_snap_query()
{
	return qry_release_snap();
}

QRY_CODE_E scm_start_bookmark(NF_ARCH_PB_AVI_PARAM param)
{
	QRY_CODE_E ret = 0;
	ret = qry_start_bookmark(param, LIMIT_20G);
	if (ret == QRY_SUCCESS) bookmark = TRUE;
	DMSG(1, "return = %d\n", ret);
	return ret;
}

int scm_pause_bookmark()
{
	return qry_pause_bookmark();
}

int scm_resume_bookmark()
{
	return qry_resume_bookmark();
}

QRY_CODE_E scm_stop_bookmark()
{
	QRY_CODE_E ret;
	ret = qry_stop_bookmark();
	DMSG(1, "return = %d\n", ret);
	return ret;
}

int scm_exit_bookmark()
{
	bookmark = FALSE;
	return qry_exit_bookmark();
}

gboolean scm_is_bookmarking()
{
	return bookmark;
}

int scm_request_bookmark_info()
{
	qry_request_bookmark_info();
	return 0;
}

RSV_CODE_E scm_reserve_bookmark_info()
{
	return _reserve_avi_data(&iscm);
}

RSV_CODE_E scm_reserve_snap_info()
{
	return _reserve_snap_data(&iscm);
}

