/*
 * acp.c
 * 	- archived file handler
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 18, 2011
 *
 */

#include "acp.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "cmm.h"
#include "ix_func.h"
#include "var.h"
#include <string.h>

#define DBG_LEVEL		9
#define DBG_MODULE		"ACP"


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef enum _WORK_STAT_E {
	READY	= 0,
	STOP	= 1,
	RUN		= 2,
	PAUSE	= 3,
} WORK_STAT_E;

typedef enum _STOP_STAT_E {
	NOTYET	= 0,
	YES		= 1,
} STOP_STAT_E;


typedef GList			ALIST;

typedef struct _ACP_CHART_T {
	AFILEID			afile_id;
	CMMACK_T		cmmack;
} ACP_CHART_T;

struct _ACP_T {
	ALIST		 	*list;	
	int				device_id;
	char			*fname[512];

	GThread			*rate_thd;
	WORK_STAT_E		rate_work;
	STOP_STAT_E		rate_stopped;
	GThread			*verify_thd;
	WORK_STAT_E		verify_work;
	STOP_STAT_E		verify_stopped;

	ACP_CHART_T		chart;	
};

////////////////////////////////////////////////////////////
//
// private variable
//


////////////////////////////////////////////////////////////
//
// private functions
//

static int _init_verify_chart(ACP_CHART_T *chart, AFILEID id, CMMACK_T *pcmmack)
{
	chart->afile_id = id;
	if (pcmmack) chart->cmmack = *pcmmack;
	return 0;
}

static int _init_verify_resources(ACP_T *pacp)
{
	pacp->rate_thd = 0;
	pacp->rate_work = READY;
	pacp->rate_stopped = YES;
	pacp->verify_thd = 0;
	pacp->verify_work = READY;
	pacp->verify_stopped = YES;

	return 0;
}

static int _check_md5(gchar *data)
{
	return nf_avi_player_check_md5(data, NULL);
}

static int _start_rate_monitoring(ACP_T *pacp)
{
	pacp->rate_work = RUN;
	pacp->rate_stopped = NOTYET;
	return 0;
}

static int _stop_rate_monitoring(ACP_T *pacp)
{
	pacp->rate_work = STOP;
	return 0;
}

static int _start_checking(ACP_T *pacp)
{
	pacp->verify_work = RUN;
	pacp->verify_stopped = NOTYET;
	return 0;
}

static int _stop_checking(ACP_T *pacp)
{
	pacp->verify_work = STOP;
	return 0;
}

static int _wait_check_run_signal(ACP_T *pacp)
{
	while (pacp->verify_work == READY) usleep(100000);
	return 0;
}

static int _wait_rate_run_signal(ACP_T *pacp)
{
	while (pacp->rate_work == READY) usleep(100000);
	return 0;
}

static int _is_rate_work(ACP_T *pacp)
{
	return (pacp->rate_work == RUN || pacp->rate_work == PAUSE);
}

static int _ack_message(ACP_T *pacp, int ret)
{
	CMMPORT cmmpt = pacp->chart.cmmack.cmmpt;
	IMSG msgid = pacp->chart.cmmack.msgid;
	void *data = pacp->chart.cmmack.data;

	return cmm_send_message(cmmpt, msgid, ret, 0, data);
}

// below codes are not grace, but it will be modified later
//
static int _notify_message(ACP_T *pacp, IMSG msgid, int param)
{
	CMMPORT cmmpt = pacp->chart.cmmack.cmmpt;
	void *data = pacp->chart.cmmack.data;

	return cmm_send_message(cmmpt, msgid, param, 0, data);
}

static int _proc_check_rate(void *data) 
{
	ACP_T *pacp = (ACP_T *)data;
	int rate;
	
	DMSG(9, "");
	_wait_rate_run_signal(pacp);
	while (_is_rate_work(pacp)) {
		usleep(300000);
		rate = nf_avi_player_md5_get_progress();
		DMSG(1, "VERIFY RATE = [%d]\n", rate);
		if (rate < -1 || rate > 100) continue;
		if (rate == -1 || rate == 100) break;
		_notify_message(pacp, INFY_VERIFY_RATE, rate);
	}
	_notify_message(pacp, INFY_VERIFY_RATE, 100);
	pacp->rate_stopped = YES;
	pacp->rate_thd = 0;
	DMSG(9, "");
	return 0;
}

static ALIST *_find_id(ACP_T *pacp, AFILEID id)
{
	ALIST *plist;
	AFILE_INFO_T *pinfo;

	for (plist = pacp->list; plist; plist = g_list_next(plist)) {
		pinfo = (AFILE_INFO_T *)(plist->data);
		if (pinfo->id == id) return plist;
	}

	g_assert(0);
	return 0;
}

static char *_get_file_name(ACP_T *pacp, AFILEID id)
{
	ALIST *plist = _find_id(pacp, id);
	AFILE_INFO_T *pinfo = (AFILE_INFO_T *)(plist->data);
	return pinfo->full_name;
}

static int _proc_check_md5(void *data) 
{
	ACP_T *pacp = (ACP_T *)data;
	ACP_CHART_T *chart = &pacp->chart;
	char *file_name;
	int ret;

	DMSG(9, "");
	_wait_check_run_signal(pacp);
	file_name = _get_file_name(pacp, chart->afile_id);
	ret = _check_md5(file_name);

	_ack_message(pacp, ret); 
	_stop_rate_monitoring(pacp);

	pacp->verify_stopped = YES;
	pacp->verify_thd = 0;
	DMSG(9, "");
	return 0;
}

static GThread *_make_rate_thread(ACP_T *pacp)
{
	GThread *th;
	th = ifn_make_thread(_proc_check_rate, pacp);
	return th;
}

static GThread *_make_verify_thread(ACP_T *pacp)
{
	GThread *th;
	th = ifn_make_thread(_proc_check_md5, pacp);
	return th;
}

static gboolean _is_multi_file(gchar *filename)
{
	if(strstr(filename, ".mul"))
		return 1;

	return 0;
}

static guint _get_channel_info(guint *v_frames)
{
	guint ch_info = 0;
	int i;
	int ch_cnt = var_get_ch_count();

	for (i = 0; i < ch_cnt; i++) {
		if(v_frames[i] > 0) ch_info |= (1 << i);
	}

	return ch_info;
}

static int _make_alist(ACP_T *pacp, int count)
{
	int i;
	AFILE_INFO_T *pinfo;

	for (i = 0; i < count; ++i) {
		pinfo = imalloc(sizeof(AFILE_INFO_T));
		pacp->list = g_list_append(pacp->list, pinfo);
	}

	return 0;
}

static void _free_info(gpointer data, gpointer user_data)
{
	if (data) ifree(data);
}

static int _free_alist(ACP_T *pacp)
{
	g_list_foreach(pacp->list, _free_info, 0);
	g_list_free(pacp->list);
	return 0;
}

static int _get_file_names(ACP_T *pacp)
{
	int cnt = g_list_length(pacp->list);
	int i;
	for (i = 0; i < cnt; ++i) {
		pacp->fname[i] = imalloc(1024);
		memset(pacp->fname[i], 0x00, 1024);
	}
	nf_avi_player_get_file_list(pacp->device_id, pacp->fname);
	return 0;
}

static int _free_file_names(ACP_T *pacp)
{
	int cnt = g_list_length(pacp->list);
	int i;
	for (i = 0; i < cnt; ++i) ifree(pacp->fname[i]);
	return 0;
}

static int _fill_file_names(ACP_T *pacp)
{
	ALIST *plist;
	AFILE_INFO_T *pinfo;
	int i = 0;
	AFILEID id = 1;	// 1 is the start id

	for (plist = pacp->list; plist; plist = g_list_next(plist)) {
		pinfo = (AFILE_INFO_T *)(plist->data);

		pinfo->id = id++;
		strcpy(pinfo->full_name, pacp->fname[i]);
		++i;
	}
	return 0;
}

static int _get_details(ACP_T *pacp, AFILEID start, AFILEID end)
{
	ALIST *list_start, *list_end, *plist;
	AFILE_INFO_T *pinfo;
	NF_AVI_PLAYER_INFO_IN_JUNK info;

	list_start = _find_id(pacp, start);
	list_end = _find_id(pacp, end);
	
	for (plist = list_start; plist; plist = g_list_next(plist)) {
		pinfo = (AFILE_INFO_T *)(plist->data);
		memset(&info, 0x00, sizeof(NF_AVI_PLAYER_INFO_IN_JUNK));
		nf_avi_player_mul_get_info(pinfo->full_name, &info, NULL);

		pinfo->movie_start = (time_t)info.arch_log_info.movie_start;
		pinfo->movie_end = (time_t)info.arch_log_info.movie_end;
		strcpy(pinfo->user, info.arch_log_info.user);
		pinfo->file_size = info.arch_log_info.file_size/1024;//bytes to KBytes
		pinfo->is_mul = _is_multi_file(pinfo->full_name);
		pinfo->ch_mask = _get_channel_info(info.v_frames);

		if (plist == list_end) break;
	}

	return 0;
}

static AFILE_INFO_T *_make_afile_list(ACP_T *pacp, AFILEID start, AFILEID end)
{
	ALIST *list_start, *list_end, *plist;
	AFILE_INFO_T *pinfo;
	int cnt = end - start + 1;
	AFILE_INFO_T *pbuf = imalloc(sizeof(AFILE_INFO_T) * cnt);
	int i = 0;

	list_start = _find_id(pacp, start);
	list_end = _find_id(pacp, end);
	
	for (plist = list_start; plist; plist = g_list_next(plist)) {
		pinfo = (AFILE_INFO_T *)(plist->data);
		memcpy(&pbuf[i], pinfo, sizeof(AFILE_INFO_T));
		++i;
		
		if (plist == list_end) break;
	}

	return pbuf;
}

static int _get_avi_file_count(ACP_T *pacp)
{
	int ret = nf_avi_player_get_file_num(pacp->device_id);
	return (ret <= 0 ? 0 : ret);
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

ACP_T *acp_create(int device_id)
{
	ACP_T *pacp;
	int cnt;

	pacp = imalloc(sizeof(ACP_T));
	memset(pacp, 0x00, sizeof(ACP_T));
	pacp->device_id = device_id;

	cnt = _get_avi_file_count(pacp);
	_make_alist(pacp, cnt);
	_get_file_names(pacp);
	_fill_file_names(pacp);

	return pacp;
}

int acp_destroy(ACP_T *pacp)
{
	if (pacp->verify_thd) { while (pacp->verify_thd) usleep(10*1000); }
	if (pacp->rate_thd) _stop_rate_monitoring(pacp);

	_free_file_names(pacp);
	_free_alist(pacp);
	memset(pacp, 0x00, sizeof(ACP_T));
	ifree(pacp);
	return 0;
}

int acp_get_count(ACP_T *pacp)
{
	return g_list_length(pacp->list);
}

AFILE_INFO_T *acp_new_afile_list(ACP_T *pacp, AFILEID start, AFILEID end)
{
	AFILE_INFO_T *afile_list;
	_get_details(pacp, start, end);
	afile_list = _make_afile_list(pacp, start, end);
	return afile_list;
}

int acp_free_afile_list(AFILE_INFO_T *afile_list)
{
	if (afile_list) ifree(afile_list);
	return 0;
}

int acp_get_afile_name(ACP_T *pacp, AFILEID id, char *buf, int len)
{
	char *file_name;

	if (!buf) return -1;
	file_name = _get_file_name(pacp, id);
	if (!file_name) return -1;

	memset(buf, 0x00, len);
	strncpy(buf, file_name, len);
	return 0; 
}

int acp_get_afile_detail(ACP_T *pacp, AFILEID id, NF_AVI_PLAYER_INFO_IN_JUNK *detail)
{	
	char *file_name;

	file_name = _get_file_name(pacp, id);
	if (!nf_avi_player_mul_get_info(file_name, detail, NULL)) return -1;
	return 0;
}

int acp_play_afile(ACP_T *pacp, AFILEID id)
{	
	char *file_name;

	file_name = _get_file_name(pacp, id);
	if (!nf_avi_player_set_play_filename(file_name, NULL)) return -1;
	return 0;
}

int acp_verify_file(ACP_T *pacp, AFILEID id, CMMACK_T *pcmmack)
{
	if (pacp->rate_thd) return -1;
	if (pacp->verify_thd) return -1;

	DMSG(9, "");
	_init_verify_chart(&pacp->chart, id, pcmmack); 
	_init_verify_resources(pacp);

	pacp->rate_thd = _make_rate_thread(pacp);
	pacp->verify_thd = _make_verify_thread(pacp);

	_start_checking(pacp);
	_start_rate_monitoring(pacp);
	return 0;
}
