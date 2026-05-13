/*
 * iva_cntr.c
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Mar 14, 2019
 *
 */

#include <string.h>
#include "iux_afx.h"
#include "nf_notify.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "iva_cntr.h"

#include "ivca_def.h"
#include "libivcam.h"
#include "nf_meta_data.h"
#include "itx_ai_def.h"
#include "nf_api_dva_eventlog.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"IVA_CNTR"



////////////////////////////////////////////////////////////
//
// private data type 
//

#define MAX_SLOT			(24*7)

#define ICNTR_LOCK()		g_mutex_lock(g_mtx_icntr)
#define ICNTR_UNLOCK()	g_mutex_unlock(g_mtx_icntr)

typedef struct _ICNTR {
	time_t	from_time;
	time_t	to_time;
	int	value[32];
} ICNTR;



////////////////////////////////////////////////////////////
//
// private variable
//

static GList *g_icntr_list = 0;
static GMutex *g_mtx_icntr = 0;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _delete_over_time_link(time_t event_time)
{
	GList *plist;
	int list_cnt;
	ICNTR *cntr_data;

	plist = g_icntr_list;

	while (plist) 
	{
		GList *next = g_list_next(plist);

		cntr_data = (ICNTR*)plist->data;		
		if (event_time < cntr_data->from_time) {
			//g_message("%s, %d", __FUNCTION__, __LINE__);
			ifree(plist->data);
			g_icntr_list = g_list_delete_link(g_icntr_list, plist);
		}
		plist = next;
	}

	return 0;
}

static int _delete_over_slot_link(time_t event_time)
{
	GList *plist;
	int list_cnt;
	ICNTR *cntr_data;

	struct tm end_ttm;
	struct tm evt_ttm;

	list_cnt = g_list_length(g_icntr_list);
	if (list_cnt < MAX_SLOT) return -1;

	//g_message("%s, %d", __FUNCTION__, __LINE__);

	localtime_r(&event_time, &evt_ttm);

	plist = g_list_nth(g_icntr_list, list_cnt-1);
	cntr_data = (ICNTR*)plist->data;
	localtime_r(&cntr_data->to_time, &end_ttm);

	if (evt_ttm.tm_hour != end_ttm.tm_hour) {
		ifree(plist->data);
		g_icntr_list = g_list_delete_link(g_icntr_list, plist);
	}
	return 0;
}

static int _add_slot_link(time_t event_time, int ch, int cnt)
{
	GList *plist;
	int list_cnt;
	ICNTR *cntr_data, *new_data;

	struct tm end_ttm;
	struct tm evt_ttm;

	list_cnt = g_list_length(g_icntr_list);

	if (list_cnt == 0)
	{
		//g_message("%s, %d", __FUNCTION__, __LINE__);
		new_data = imalloc(sizeof(ICNTR));
		new_data->from_time = event_time;		
		new_data->to_time = event_time;
		new_data->value[ch] = cnt;
		//g_message("%s, %d, ch:%d, val:%d", __FUNCTION__, __LINE__, ch, new_data->value[ch]);

		g_icntr_list = g_list_append(g_icntr_list, new_data);
	}
	else
	{
		localtime_r(&event_time, &evt_ttm);

		plist = g_list_nth(g_icntr_list, list_cnt-1);
		cntr_data = (ICNTR*)plist->data;
		localtime_r(&cntr_data->to_time, &end_ttm);

		if (evt_ttm.tm_hour != end_ttm.tm_hour) {
			new_data = imalloc(sizeof(ICNTR));
			new_data->from_time = event_time;		
			new_data->to_time = event_time;
			new_data->value[ch] = cnt;		
			//g_message("%s, %d, ch:%d, val:%d", __FUNCTION__, __LINE__, ch, new_data->value[ch]);

			g_icntr_list = g_list_append(g_icntr_list, new_data);
		}
		else {
			cntr_data->to_time = event_time;
			cntr_data->value[ch] += cnt;
			//g_message("%s, %d, ch:%d, val:%d", __FUNCTION__, __LINE__, ch, cntr_data->value[ch]);			
		}
	}
	return 0;
}

static int _put_ai_dvabx_event(NF_NOTIFY_INFO *pnotify)
{
	int *p = pnotify->p.ptr;
	int ch = p[0], cnt = p[1];
	ai_rule_event_t *pevt = p + 2;

	_delete_over_time_link(pnotify->timestamp.tv_sec);
	_delete_over_slot_link(pnotify->timestamp.tv_sec);
	_add_slot_link(pnotify->timestamp.tv_sec, ch, cnt);
	return 0;
}

static int _put_ai_builtin_event(NF_NOTIFY_INFO *pnotify)
{
	DVA_MSG *dva_event = pnotify->p.ptr;
	int ch = dva_event->ch, cnt = 1;

	_delete_over_time_link(pnotify->timestamp.tv_sec);
	_delete_over_slot_link(pnotify->timestamp.tv_sec);
	_add_slot_link(pnotify->timestamp.tv_sec, ch, cnt);
	return 0;
}

static int _put_classic_va_event(NF_NOTIFY_INFO *pnotify)
{
	int *p = pnotify->p.ptr;
	int ch = p[0], cnt = p[1];
	ivca_rule_event_t *pevt = p + 2;

	_delete_over_time_link(pnotify->timestamp.tv_sec);
	_delete_over_slot_link(pnotify->timestamp.tv_sec);
	_add_slot_link(pnotify->timestamp.tv_sec, ch, cnt);
	return 0;
}

static int _free_icntr_data(gpointer data, gpointer user_data)
{
	if (data) ifree(data);
	return;
}

static int _get_counter_hour_data(time_t stdtime, IVACR_T *icr, IVACR_DATA_T *data)
{
	GList *plist;
	ICNTR *cntr_data;
	int i, j;

	stdtime -= 60*60*23;

	for (i = 0; i < 24; i++) 
	{
		data->hour[i].ttime = stdtime;

		plist = g_icntr_list;
		while (plist) 
		{
			cntr_data = (ICNTR*)plist->data;
			if ((cntr_data->to_time > stdtime-60*60) && (cntr_data->to_time <= stdtime))
			{
				//g_message("%s, %d", __FUNCTION__, __LINE__);
				for (j = 0; j < var_get_ch_count(); j++) {
					if (icr->chmask & (1 << j)) data->hour[i].cnt += cntr_data->value[j];
				}
			}	
			plist = g_list_next(plist);
		}
		stdtime += 60*60;
	}
	return 0;	
}

static int _get_counter_day_data(time_t stdtime, IVACR_T *icr, IVACR_DATA_T *data)
{
	GList *plist;
	ICNTR *cntr_data;
	int i, j;

	stdtime -= 60*60*24*6;

	for (i = 0; i < 7; i++) 
	{
		data->day[i].ttime = stdtime;

		plist = g_icntr_list;
		while (plist) 
		{
			cntr_data = (ICNTR*)plist->data;
			if ((cntr_data->to_time > stdtime-60*60*24) && (cntr_data->to_time <= stdtime))
			{
				//g_message("%s, %d", __FUNCTION__, __LINE__);
				for (j = 0; j < var_get_ch_count(); j++) {
					if (icr->chmask & (1 << j)) data->day[i].cnt += cntr_data->value[j];
				}
			}	
			plist = g_list_next(plist);
		}
		stdtime += 60*60*24;
	}
	return 0;	
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int iva_cntr_init()
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	g_mtx_icntr = g_mutex_new();
	g_icntr_list = 0;
	return 0;
}

int iva_cntr_reset()
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	ICNTR_LOCK();
	g_list_foreach(g_icntr_list, _free_icntr_data, 0);
	g_list_free(g_icntr_list);
	g_icntr_list = 0;
	ICNTR_UNLOCK();	
	return 0;
}

int iva_cntr_put_ai_dvabx_event(NF_NOTIFY_INFO *pnotify)
{
	ICNTR_LOCK();
	_put_ai_dvabx_event(pnotify);
	ICNTR_UNLOCK();		
	return 0;
}

int iva_cntr_put_ai_builtin_event(NF_NOTIFY_INFO *pnotify)
{
	ICNTR_LOCK();	
	_put_ai_builtin_event(pnotify);
	ICNTR_UNLOCK();		
	return 0;
}

int iva_cntr_put_classic_va_event(NF_NOTIFY_INFO *pnotify)
{
/*	
	ICNTR_LOCK();
	_put_classic_va_event(pnotify);
	ICNTR_UNLOCK();		
*/
	return 0;
}

IVACR_T *iva_cntr_create()
{
	IVACR_T *icr;
	int i;

	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	icr = imalloc(sizeof(IVACR_T));
	memset(icr, 0x00, sizeof(IVACR_T));

	icr->chmask = var_get_ch_mask();
	return icr;
}

int iva_cntr_destroy(IVACR_T *icr)
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	ifree(icr);
	return 0;
}

int iva_cntr_set_filter_ch(IVACR_T *icr, int ch, int onoff)
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	if (onoff) icr->chmask |= (1 << ch);
	else icr->chmask &= ~(1 << ch);
	return 0;
}

int iva_cntr_set_filter_chmask(IVACR_T *icr, unsigned int chmask)
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	icr->chmask = chmask;
	return 0;
}

int iva_cntr_get_counter_data(IVACR_T *icr, IVACR_DATA_T *data)
{
	time_t curtime, stdtime;
	int year, month, day;
	int hour, min, sec;

	char strBuf[128];

	g_message("%s, %d, called", __FUNCTION__, __LINE__);

	memset(data, 0x00, sizeof(IVACR_DATA_T));

	ICNTR_LOCK();

	curtime = time(0);

	memset(strBuf, 0x00, sizeof(strBuf));
	dtf_get_local_datetime(curtime, strBuf);
	g_message("%s, %d, curtime:%s", __FUNCTION__, __LINE__, strBuf);

	ifn_get_local_day(curtime, &year, &month, &day);
	ifn_get_local_hourmin(curtime, &hour, &min, &sec);
	stdtime = ifn_get_gmt_from_local(year, month, day, hour, 0, 0);
	stdtime += 60*60-1;

	memset(strBuf, 0x00, sizeof(strBuf));
	dtf_get_local_datetime(stdtime, strBuf);
	g_message("%s, %d, hour_stdtime:%s", __FUNCTION__, __LINE__, strBuf);
	_get_counter_hour_data(stdtime, icr, data);

	ifn_get_local_day(curtime, &year, &month, &day);
	ifn_get_local_hourmin(curtime, &hour, &min, &sec);
	stdtime = ifn_get_gmt_from_local(year, month, day, 0, 0, 0);
	stdtime += 60*60*24-1;

	memset(strBuf, 0x00, sizeof(strBuf));
	dtf_get_local_datetime(stdtime, strBuf);
	g_message("%s, %d, day_stdtime:%s", __FUNCTION__, __LINE__, strBuf);
	_get_counter_day_data(stdtime, icr, data);

	ICNTR_UNLOCK();	
	return 0;
}
