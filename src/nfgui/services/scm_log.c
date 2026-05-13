/*
 * scm_log.c
 * 	- scm log service
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 25, 2011
 *
 */

#include "log.h"
#include "scm.h"
#include "scm_internal.h"
#include "iux_afx.h"
#include "ssm.h"
#include "vfs.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_LOG"


////////////////////////////////////////////////////////////
//
// private data type 
//


////////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_put_log(PUTLOG_TYPE_E type, int param, char *text)
{
	DMSG(1, "EVENT WRITTEN = (%x) (%x) (%s)\n", type, param, text ? text : "NO TEXT");
	logx_put_log(type, 1, param, text);
	return 0;
}

int _scm_put_log_with_tra(SCM_T *piscm, PUTLOG_TYPE_E type, int param1, int param2, TRANSACTION_E tra)
{
	char user[64];
	CALLID callid = piscm->chart[tra].caller;

	memset(user, 0x00, sizeof(user));

	if (callid == LOCAL_CALL) ssm_get_cur_id(user);
	else if (callid == WEB_CALL) sprintf(user, "WEB Viewer");
	else sprintf(user, "<SYSTEM>");
	
	return _scm_put_log(type, param2, user);
}

int _scm_put_log_with_tra_prev_user(SCM_T *piscm, PUTLOG_TYPE_E type, int param1, int param2, TRANSACTION_E tra)
{
	char user[64];
	CALLID callid = piscm->chart[tra].caller;

	memset(user, 0x00, sizeof(user));

	if (callid == LOCAL_CALL) ssm_get_prev_id(user);
	else if (callid == WEB_CALL) sprintf(user, "WEB Viewer");
	else sprintf(user, "<SYSTEM>");
	
	return _scm_put_log(type, param2, user);
}


////////////////////////////////////////////////////////////////
//
// public interfaces
//

LOGCTX scm_open_log_ctx()
{
	LOGX_T *ctx;
	int ch = var_get_ch_count();
	ctx = logx_create(ch);
	return (LOGCTX)ctx;
}

int scm_close_log_ctx(LOGCTX logctx)
{
	if (logctx) logx_destroy(logctx);
	return 0;
}

int scm_reset_log_filter(LOGCTX logctx, LF_RESET_E rtype)
{
	return logx_reset_log_filter(logctx, rtype);
}

int scm_set_log_filter_ch(LOGCTX logctx, int ch, int onoff)
{
	return logx_set_log_filter_ch(logctx, ch, onoff);
}

int scm_set_log_filter_type(LOGCTX logctx, unsigned int chmask, LF_CAT_E lcat, int onoff)
{
	return logx_set_log_filter_type(logctx, chmask, lcat, onoff);
}

int scm_set_log_filter_order(LOGCTX logctx, LF_ORDER_E order)
{
	return logx_set_log_filter_order(logctx, order);
}

int scm_get_log(LOGCTX logctx, GTimeVal *older, GTimeVal *later, int count, LOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = logx_get_log(logctx, older, later, count, log);
	DMSG(9, "(%ld), (%ld)", older->tv_sec, later->tv_sec);
	if (next) *next = (gboolean)logx_has_log_next_next(logctx, count);
	return cnt;
}

int scm_get_log_next(LOGCTX logctx, int count, LOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = logx_get_log_next(logctx, count, log);
	if (next) *next = (gboolean)logx_has_log_next_next(logctx, count);
	return cnt;
}

int scm_get_log_prev(LOGCTX logctx, int count, LOG_DATA_T *log, gboolean *prev)
{
	int cnt;
	cnt = logx_get_log_prev(logctx, count, log);
	if (prev) *prev = (gboolean)logx_has_log_prev_prev(logctx, count);
	return cnt;
}

int scm_put_log(PUTLOG_TYPE_E type, int param1, int param2)
{
	char user[64];

	memset(user, 0x00, sizeof(user));
	ssm_get_cur_id(user);
	return _scm_put_log(type, param2, user);
}

int scm_put_log_t(PUTLOG_TYPE_E type, int param1, int param2, char *text)
{
	if (!text) return -1;
	return _scm_put_log(type, param2, text);
}

TLOGCTX scm_open_tlog_ctx()
{
	TLOGX_T *tctx;
	int ch = var_get_ch_count();
	tctx = tlogx_create(ch);
	return (TLOGCTX)tctx;
}

int scm_close_tlog_ctx(TLOGCTX tlogctx)
{
	if (tlogctx) tlogx_destroy(tlogctx);
	return 0;
}

int scm_reset_tlog_filter(TLOGCTX tlogctx)
{
	return tlogx_reset_tlog_filter(tlogctx);
}

int scm_set_tlog_filter_ch(TLOGCTX tlogctx, int ch, int onoff)
{
	return tlogx_set_tlog_filter_ch(tlogctx, ch, onoff);
}

int scm_set_tlog_filter_text(TLOGCTX tlogctx, SEARCH_KEY_T *key_info, gboolean match_case, gboolean match_whole)
{
	return tlogx_set_tlog_filter_text(tlogctx, key_info, match_case, match_whole);
}

int scm_set_tlog_filter_order(TLOGCTX tlogctx, LF_ORDER_E order)
{
	return tlogx_set_tlog_filter_order(tlogctx, order);
}

int scm_get_tlog(TLOGCTX tlogctx, GTimeVal *older, GTimeVal *later, int count, TLOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = tlogx_get_tlog(tlogctx, older, later, count, log);
	DMSG(9, "(%ld), (%ld)", older->tv_sec, later->tv_sec);
	if (next) *next = (gboolean)tlogx_has_tlog_next_next(tlogctx, count);
	return cnt;
}

int scm_get_tlog_next(TLOGCTX tlogctx, int count, TLOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = tlogx_get_tlog_next(tlogctx, count, log);
	if (next) *next = (gboolean)tlogx_has_tlog_next_next(tlogctx, count);
	return cnt;
}

int scm_get_tlog_prev(TLOGCTX tlogctx, int count, TLOG_DATA_T *log, gboolean *prev)
{
	int cnt;
	cnt = tlogx_get_tlog_prev(tlogctx, count, log);
	if (prev) *prev = (gboolean)tlogx_has_tlog_prev_prev(tlogctx, count);
	return cnt;
}

DLOGCTX scm_open_dlog_ctx()
{
	DLOGX_T *dctx;
	int ch = var_get_ch_count();
	dctx = dlogx_create(ch);
	return (DLOGCTX)dctx;
}

int scm_close_dlog_ctx(DLOGCTX dlogctx)
{
	if (dlogctx) dlogx_destroy(dlogctx);
	return 0;
}

int scm_reset_dlog_filter(DLOGCTX dlogctx)
{
	return dlogx_reset_dlog_filter(dlogctx);
}

int scm_set_dlog_filter_ch(DLOGCTX dlogctx, int ch, int onoff)
{
	return dlogx_set_dlog_filter_ch(dlogctx, ch, onoff);
}

int scm_set_dlog_filter_algorithm(DLOGCTX dlogctx, guint algorithm)
{
	return dlogx_set_dlog_filter_algorithm(dlogctx, algorithm);
}

int scm_set_dlog_filter_event(DLOGCTX dlogctx, guint event_mask)
{
	return dlogx_set_dlog_filter_event(dlogctx, event_mask);
}

int scm_set_dlog_filter_sub(DLOGCTX dlogctx, char oper[2], gboolean match_case, gboolean match_whole)
{
	return dlogx_set_dlog_filter_sub(dlogctx, oper, match_case, match_whole);
}

int scm_set_dlog_filter_evt_text(DLOGCTX dlogctx, char *evt)
{
	return dlogx_set_dlog_filter_evt_text(dlogctx, evt);
}

int scm_set_dlog_filter_group_mask(DLOGCTX dlogctx, guint group_mask)
{
	return dlogx_set_dlog_filter_group_mask(dlogctx, group_mask);
}

int scm_set_dlog_filter_text(DLOGCTX dlogctx, char *key_str, char *key_str1, char *key_str2)
{
	return dlogx_set_dlog_filter_text(dlogctx, key_str, key_str1, key_str2);
}

int scm_set_dlog_filter_enable(DLOGCTX dlogctx, char *key_str, guint name_search, guint group_search, guint gender_search)
{
	return dlogx_set_dlog_filter_enable(dlogctx, key_str, name_search, group_search, gender_search);
}

int scm_set_dlog_filter_order(DLOGCTX dlogctx, LF_ORDER_E order)
{
	return dlogx_set_dlog_filter_order(dlogctx, order);
}

int scm_get_dlog(DLOGCTX dlogctx, GTimeVal *older, GTimeVal *later, int count, LOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = dlogx_get_dlog(dlogctx, older, later, count, log);
	DMSG(9, "(%ld), (%ld)", older->tv_sec, later->tv_sec);
	if (next) *next = (gboolean)dlogx_has_dlog_next_next(dlogctx, count);
	return cnt;
}

int scm_get_dlog_next(DLOGCTX dlogctx, int count, LOG_DATA_T *log, gboolean *next)
{
	int cnt;
	cnt = dlogx_get_dlog_next(dlogctx, count, log);
	if (next) *next = (gboolean)dlogx_has_dlog_next_next(dlogctx, count);
	return cnt;
}

int scm_get_dlog_prev(DLOGCTX dlogctx, int count, LOG_DATA_T *log, gboolean *prev)
{
	int cnt;
	cnt = dlogx_get_dlog_prev(dlogctx, count, log);
	if (prev) *prev = (gboolean)dlogx_has_dlog_prev_prev(dlogctx, count);
	return cnt;
}
