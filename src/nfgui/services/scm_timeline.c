/*
 * scm_timeline.c
 * 	- scm timeline service
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 4, 2011
 *
 */
#include <string.h>

#include "iux_afx.h"
#include "iux_types.h"
#include "scm.h"
#include "nf_api_play.h"
#include "modules/var.h"
#include "scm_internal.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_TML"

#define TML_LOCK()		g_mutex_lock(iscm.mtx_tml)
#define TML_UNLOCK()	g_mutex_unlock(iscm.mtx_tml)

////////////////////////////////////////////////////////////////
//
// private interfaces
//

static int _get_data(time_t start, int res, int count, BITMASK64 chmask, gchar **pdata)
{
	NF_TIMELINE_PARAM time_param;
	GTimeVal tv;
	BITMASK64 mask = 0x00;
	int i;
	int max_ch = 0;

	memset(&tv, 0x00, sizeof(GTimeVal));
	memset(&time_param, 0, sizeof(NF_TIMELINE_PARAM));

	if (var_get_ch_mask() == chmask) max_ch = var_get_ch_count();
	else {
		for (i = 0; i < 64; ++i)
			if (chmask & (1ULL << i)) ++max_ch;
	}

	tv.tv_sec = start;

	time_param.time_begin = tv;
	time_param.resolution = res;
	time_param.count = count;
	time_param.max_channel = max_ch;
	time_param.split_channel = 0;
	time_param.channel_mask = chmask;
	time_param.hide = 0;

	DMSG(1, "======== CALENDAR PARAMETER ========");
	DMSG(1, "time_begin: [%u]", time_param.time_begin);
	DMSG(1, "resolution: [%d]", time_param.resolution);
	DMSG(1, "count:      [%d]", time_param.count);
	DMSG(1, "max_ch:     [%d]", time_param.max_channel);
	DMSG(1, "split_ch:   [%d]", time_param.split_channel);
	DMSG(1, "ch_mask:    [0x%llX]", time_param.channel_mask);
	DMSG(1, "hide:       [%d]", time_param.hide);

#if 0
{
	static int even = 0;
		
	// dummy data
	int i;
	unsigned char t;

	*pdata = g_malloc0(count);
	for (i = 0; i < count; i++)  {

		t = (unsigned char)(ifn_rand() % 2);
		(*pdata)[i] = t;
	}
	usleep(500000);
}
#else
	if (!nf_timeline_get(&time_param, pdata, NULL)) return -1;

	// debug code
	{
		int i;
		
		DMSG(1, "=============== CALENDAR INFO ===============");
		for (i = 0; i < 31; i++)
			printf("[%d,%d]", i, (*pdata)[i]);
		printf("[end]\n");
	}
#endif	
	return 0;

}

static int _get_hourly_data(time_t start, int count, BITMASK64 chmask, gchar **pdata)
{
	return _get_data(start, 3600, count, chmask, pdata);
}

static int _get_daily_data(time_t start, int count, BITMASK64 chmask, gchar **pdata)
{
	return _get_data(start, 86400, count, chmask, pdata);
}

static int _convert_to_daily_data(time_t start, char *in, int in_cnt, char **out, int out_cnt)
{
	int i = 0, j = 0;
	int pos = 0;
	int flag = 0;
	time_t tt;
	int exit = 0;

	*out = g_malloc0(out_cnt);

	while (1) {
		flag = 0;
		tt = start;
		while (1) {
			if (!ifn_is_same_day(start, tt)) break;
			flag |= in[pos];
			tt += 3600;
			++pos;
			if (in_cnt == pos) { exit = 1; break; }
		}

		(*out)[i] = (flag > 0) ? 1 : 0;
		if (exit) break;
		++i;
		start = tt;
	}

	return 0;
}

static int _get_recinfo(int year, int month, BITMASK64 chmask, gchar **pdata)
{
	time_t start;
	int days;
	char *tmp = NULL;

	start = ifn_get_gmt_from_local(year, month, 1, 0, 0, 0);
	if (ifn_has_dst_change(year, month)) {
		days = ifn_get_days_in_month(year, month);
		_get_hourly_data(start, days * 24, chmask, &tmp);
		if (tmp) {
			_convert_to_daily_data(start, tmp, days * 24, pdata, 31);
			g_free(tmp);
		}
		else return -1;
	}
	else {
		return _get_daily_data(start, 31, chmask, pdata);
	}
	return 0;
}

////////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_disable_timeline()
{
	iscm.enable_tml = 0;
	return 0;
}

int _scm_enable_timeline()
{
	iscm.enable_tml = 1;
	return 0;
}

int _scm_init_timeline(SCM_T *piscm)
{
	piscm->mtx_tml = g_mutex_new();
	piscm->enable_tml = 1;
	return 0;
}

////////////////////////////////////////////////////////////////
//
// public interfaces
//
int scm_get_recinfo(int year, int month, BITMASK64 chmask, gchar buf[31])
{
	gchar *pdata = NULL;
	TML_LOCK();
	if (_get_recinfo(year, month, chmask, &pdata) == -1) { TML_UNLOCK(); return -1; }
	TML_UNLOCK();
	if (pdata) {
		memcpy(buf, pdata, 31);
		g_free(pdata);
	}
	return 0;
}

int scm_get_timeline_ex(NF_TIMELINE_PARAM *param, gchar **elem)
{
	gboolean ret;
	gchar *chdata;
	gchar *next_ch;
	int i;
	int chcnt;
	guint64	masking = 1;
	guint64	tmp_mask;
	guint64	mask;
	
	if (!iscm.enable_tml) return -1;

	tmp_mask = param->channel_mask;
	chcnt = var_get_ch_count();

	TML_LOCK();

	*elem = g_malloc0(param->count * chcnt);
	next_ch = *elem;

	ifn_init_time_elap();
	DMSG(1, "TIMELINE CH QRY START [%u]", time(0));
	for (i = 0; i < chcnt; ++i) {
		mask = tmp_mask & masking;
		param->channel_mask = mask;

		if (param->channel_mask) {
			ret = nf_timeline_get(param, &chdata, NULL);
			if (ret) {
				memcpy(next_ch, chdata, param->count);
				g_free(chdata);
			}
    		next_ch += param->count;
		}

		masking <<= 1;
	}
	DMSG(1, "TIMELINE CH QRY END");
	ifn_print_time_elap();

	TML_UNLOCK();
	return 0;
}

//#define _EMUL_TIMELINE
int scm_get_timeline(NF_TIMELINE_PARAM *param, gchar **elem)
{
	gboolean ret;
	
	if (!iscm.enable_tml) return -1;

	TML_LOCK();
#ifndef _EMUL_TIMELINE	
	ifn_init_time_elap();
	DMSG(1, "TIMELINE QRY START [%u]", time(0));
	ret = nf_timeline_get(param, elem, NULL);
	DMSG(1, "TIMELINE QRY END");
	ifn_print_time_elap();
	if (!ret) *elem = g_malloc0(param->count * var_get_ch_count());

#else
{
	static int even = 0;
	BITMASK64 chmask = param->channel_mask;
	int count = param->count;
	// dummy data
	int i;
	unsigned char t;
	int ch_cnt = 0;

	for (i = 0; i < var_get_ch_count(); ++i)
		if (chmask & (1ULL << i)) ch_cnt++;

	*elem = g_malloc0(count * ch_cnt);
	for (i = 0; i < count * ch_cnt; i++)  {

		t = (unsigned char)(ifn_rand() % 4);


		//if (even == 0) t = 1;
		//else t = 3;
		//even = !even;	

		(*elem)[i] = t;
	}
}

#endif

#if 0
{
	int i, j;
	int cnt = 0;
	int ch_cnt = 0;
	int count = param->count;
	BITMASK64 chmask = param->channel_mask;

	for (i = 0; i < var_get_ch_count(); ++i)
		if (chmask & (1ULL << i)) ch_cnt++;

	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	for (i = 0; i < ch_cnt; ++i) {
		for (j = 0; j < count; ++j) {
			printf("%d", (*elem)[cnt]);
			++cnt;
		}
		printf("\n");
	}
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

}
#endif

	TML_UNLOCK();

	return 0;
}
