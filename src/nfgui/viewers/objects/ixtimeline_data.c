/*
 * ixtimeline_data.c
 * 	- timeline widget data module
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 7, 2011
 *
 */

#include "iux_afx.h"
#include "nf_api_play.h"
#include "ixtimeline_internal.h"
#include "ix_mem.h"
#include "scm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"TML_DATA"

#define	INTEND_DELAY	15		// 15 secs

/*
 *
 *
 *                 ----O===============O----------------------------------
 *                     ^               ^
 *                     |               |
 *                     |               |
 *                    px_s            px_e
 *                    (included)      (included)
 *
 */                    


////////////////////////////////////////////////////////////
//
// private data type
//

typedef enum NEW_POS_E {
	OUT_OF_RANGE	= 0,
	OVER_THE_BASE	= 1,
	OVER_THE_EDGE	= 2,
	INSIDE_RANGE	= 3
} NEW_POS_E;

////////////////////////////////////////////////////////////
//
// private functions
//

static time_t _get_system_time()
{
	return time(0) - INTEND_DELAY;
}

static int _get_timeline(BITMASK64 chmask, time_t base, int res, int count, gchar **pdata)
{
	NF_TIMELINE_PARAM time_param;

	memset(&time_param, 0, sizeof(NF_TIMELINE_PARAM));
	ifn_timet_to_gtv(base, &time_param.time_begin);
	time_param.resolution = res;
	time_param.count = count;
	time_param.max_channel = var_get_ch_count();//ch_cnt;
	time_param.split_channel = 1;
	time_param.channel_mask = chmask;
	time_param.hide = 0;

	return scm_get_timeline_ex(&time_param, pdata);
}

static int _change_backward_info(TML_QRYINFO_T *qi, time_t base, int spp, int *count)
{
	*count = (qi->qrt->prev_tbase - base) / spp;
	if (*count > qi->qrt->org_count) *count = qi->qrt->org_count; // oversize shifting
	return 0;
}

static int _change_forward_info(TML_QRYINFO_T *qi, int spp, time_t *base, int *count)
{
	time_t edge;
	time_t new_base = *base;
	edge = *base + (spp * (*count));
	*base = qi->qrt->prev_tbase + (qi->qrt->prev_spp * qi->qrt->prev_count);
	if (*base < new_base) *base = new_base;
	*count = (edge - *base) / spp;
	return 0;
}

static int _change_interacing_info(TML_QRYINFO_T *qi, time_t base, int *spp, int *count)
{
	time_t edge;
	edge = base + ((*spp) * (*count));
	// below expression may not be divided accurately.
	// but it's ok in interacing mode.
	qi->qrt->mag = *count / qi->intr_step;
	*count = qi->intr_step;
	*spp = (edge - base) / qi->intr_step;
	return 0;
}

static int _change_query_info(TML_QRYINFO_T *qi, time_t *base, int *spp, int *count)
{
	if (qi->qrt->prev_tbase == 0) qi->qt = NORMAL;
	switch (qi->qt) {
	case NORMAL: break;
	case SHIFT:
		if (qi->qrt->prev_tbase > *base) _change_backward_info(qi, *base, *spp, count);
		else if (qi->qrt->prev_tbase < *base) _change_forward_info(qi, *spp, base, count);
		else return -1;
		break;
	case INTERACING:
		_change_interacing_info(qi, *base, spp, count);
		break;
	}
	return 0;
}

void _free_rbar(gpointer data, gpointer user_data)
{
	if (data) ifree(data);
	return;
}

static int _free_rlist(int ch_cnt, RLIST *rlist[])
{
	int i;
	for (i = 0; i < ch_cnt; ++i) {
		g_list_foreach(rlist[i], _free_rbar, 0);
		g_list_free(rlist[i]);
	}
	return 0;
}

static int _remove_old_data(int ch_cnt, RLIST *rlist[])
{
	_free_rlist(ch_cnt, rlist);
	return 0;
}

static RLIST *_make_rlist(int count, char *pdata)
{
	char reason;
	int i;
	int px_s;

	RLIST *list = NULL;
	SZ_RBAR_T *rbar;

	reason = *pdata;
	px_s = 0;
	for (i = 1; i < count; ++i) {
		if (*(pdata + i) != reason) {
			rbar = imalloc(sizeof(SZ_RBAR_T));

			rbar->reason = reason;
			rbar->px_s = px_s;
			rbar->px_e = i - 1;
			list = g_list_append(list, rbar);

			reason = *(pdata + i);
			px_s = i;
		}
	}
	rbar = imalloc(sizeof(SZ_RBAR_T));
	rbar->reason = reason;
	rbar->px_s = px_s;
	rbar->px_e = i - 1;
	list = g_list_append(list, rbar);
	return list;
}

static NEW_POS_E _get_pos_rbar(TML_QRYINFO_T *qi, SZ_RBAR_T *rbar)
{
	int edge = qi->qrt->org_count - 1;

//	DMSG(1, "px_s = %d, px_e = %d", rbar->px_s, rbar->px_e);
	if (rbar->px_s < 0 && rbar->px_e < 0) return OUT_OF_RANGE;
	else if (rbar->px_s > edge && rbar->px_e > edge) return OUT_OF_RANGE;
	else if (rbar->px_s < 0 && rbar->px_e >= 0) return OVER_THE_BASE;
	else if (rbar->px_s <= edge && rbar->px_e > edge) return OVER_THE_EDGE;
	else return INSIDE_RANGE;
	return 0;
}

static RLIST *_shift_rlist(TML_QRYINFO_T *qi, int shift, RLIST *list)
{
	RLIST *plist, *qlist;
	SZ_RBAR_T *rbar;
	NEW_POS_E pos;

//	DMSG(1, "shift value = %d", shift);
	for (plist = list; plist; plist = plist ? g_list_next(plist) : list) {
		rbar = (SZ_RBAR_T *)(plist->data);

		rbar->px_s += shift;
		rbar->px_e += shift;

		pos = _get_pos_rbar(qi, rbar);
		switch (pos) {
		case OUT_OF_RANGE:
			ifree(rbar);
			qlist = g_list_previous(plist);
			list = g_list_delete_link(list, plist);
			plist = qlist;
			break;
		case OVER_THE_BASE:
			rbar->px_s = 0;
			break;
		case OVER_THE_EDGE:
			rbar->px_e = qi->qrt->org_count - 1;
			break;
		}
	}
	return list;
}

static int _magnify_rlist(TML_QRYINFO_T *qi, int mag, RLIST *list)
{
	RLIST *plist;
	SZ_RBAR_T *rbar;

	for (plist = list; plist; plist = g_list_next(plist)) {
		rbar = (SZ_RBAR_T *)(plist->data);
		rbar->intr = INTERACED;
		rbar->px_s *= mag;
		rbar->px_e = ((rbar->px_e + 1) * mag) - 1;
	}
	
	return 0;
}

static RLIST *_merge_rlist_base_tip(TML_QRYINFO_T *qi, RLIST *rlist)
{
	RLIST *tip;
	RLIST *tip_nx;
	SZ_RBAR_T *rbar;
	SZ_RBAR_T *rbar_nx;
	
	while (1) {
		tip = g_list_first(rlist);
		tip_nx = g_list_next(tip);
		if (!tip_nx) break;
		rbar = (SZ_RBAR_T *)tip->data;
		rbar_nx = (SZ_RBAR_T *)tip_nx->data;

		if (rbar->reason == rbar_nx->reason) {
			rbar_nx->px_s = rbar->px_s;
			ifree(rbar);
			rlist = g_list_delete_link(rlist, tip);
		}
		else break;
	}
	
	return rlist;	
}

static RLIST *_merge_rlist_edge_tip(TML_QRYINFO_T *qi, RLIST *rlist)
{
	RLIST *tip;
	RLIST *tip_pv;
	SZ_RBAR_T *rbar;
	SZ_RBAR_T *rbar_pv;
	
	while (1) {
		tip = g_list_last(rlist);
		tip_pv = g_list_previous(tip);
		if (!tip_pv) break;
		rbar = (SZ_RBAR_T *)tip->data;
		rbar_pv = (SZ_RBAR_T *)tip_pv->data;

		if (rbar->reason == rbar_pv->reason) {
			rbar_pv->px_e = rbar->px_e;
			ifree(rbar);
			rlist = g_list_delete_link(rlist, tip);
		}
		else break;
	}
	
	return rlist;	
}

static RLIST *_combine_rlist(TML_QRYINFO_T *qi, time_t mbase, RLIST *new, RLIST *old)
{
	RLIST *list = NULL;
	if (qi->qrt->prev_tbase < mbase) {				// shift to newer (go ahead)
		old = _shift_rlist(qi, -(qi->shift), old);
		new = _shift_rlist(qi, qi->qrt->org_count - (qi->shift), new);
		list = g_list_concat(old, new);
		list = _merge_rlist_edge_tip(qi, list);
	}
	else {
		old = _shift_rlist(qi, -(qi->shift), old);
		list = g_list_concat(new, old);
		list = _merge_rlist_base_tip(qi, list);
	}
	return list;
}

static int _refine_raw_data(TML_QRYINFO_T *qi, time_t mbase, int ch_cnt, int count, char *pdata, RLIST *rlist[])
{
	RLIST *nlist = NULL;
	int i;

	if (qi->qt == SHIFT) {
		for (i = 0; i < ch_cnt; ++i) {
			nlist = _make_rlist(count, pdata + (i * count));
			nlist = _combine_rlist(qi, mbase, nlist, rlist[i]);
			rlist[i] = nlist;
		}
	}
	else if (qi->qt == INTERACING) {
		_remove_old_data(ch_cnt, rlist);
		for (i = 0; i < ch_cnt; ++i) {
			rlist[i] = _make_rlist(count, pdata + (i * count));
			_magnify_rlist(qi, qi->qrt->mag, rlist[i]);
		}

#if 0
	{
		RLIST *plist;	
		SZ_RBAR_T *rbar;
		int i;
		int cnt = 0;
		for (i = 0; i < var_get_ch_count(); ++i) {
			printf("CH:%02d] COUNT OF NODE = %d\n", i, g_list_length(rlist[i]));
			for (plist = rlist[i]; plist; plist = g_list_next(plist)) {
				rbar = (SZ_RBAR_T *)(plist->data);
//				if (i == 0) 
				printf("rbar, [%02d, %03d] (%d) s = %d, e = %d\n", 
						i, cnt++, rbar->reason, rbar->px_s, rbar->px_e);
			}
		}

	}
#endif
	}
	else {
		_remove_old_data(ch_cnt, rlist);
		for (i = 0; i < ch_cnt; ++i) {
			rlist[i] = _make_rlist(count, pdata + (i * count));
		}
	}

#if 0
	{
		RLIST *plist;	
		SZ_RBAR_T *rbar;
		int i;
		int cnt = 0;
		for (i = 0; i < var_get_ch_count(); ++i) {
			printf("CH:%02d] COUNT OF NODE = %d\n", i, g_list_length(rlist[i]));
			for (plist = rlist[i]; plist; plist = g_list_next(plist)) {
				rbar = (SZ_RBAR_T *)(plist->data);
//				if (i == 0) 
				printf("rbar, [%02d, %03d] (%d) s = %d, e = %d\n", 
						i, cnt++, rbar->reason, rbar->px_s, rbar->px_e);
			}
		}

	}
#endif
	return 0;
}

static BITMASK64 _make_ch_mask(int ch)
{
	BITMASK64 mask = 0;
	int i;

	if (ch != -1) {
		mask |= (1 << ch);
		return mask;
	}

	switch (var_get_ch_count()) {
	case 4: mask = 0xFll; break;
	case 8: mask = 0xFFll; break;
	case 16: mask = 0xFFFFll; break;
	case 32: mask = 0xFFFFFFFFll; break;
	}
	return mask;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _tml_get_data(TML_QRYINFO_T *qi)
{
	char *pdata = NULL;
	BITMASK64 ch_mask = _make_ch_mask(qi->ch);

	qi->qrt->org_tbase = qi->base;
	qi->qrt->org_spp = qi->spp;
	qi->qrt->org_count = qi->count;

	if (_change_query_info(qi, &qi->base, &qi->spp, &qi->count) < 0) return -1;
	if (qi->count == 0) return -1;

	if (_get_timeline(ch_mask, qi->base, qi->spp, qi->count, &pdata) == -1) return -1;
	_refine_raw_data(qi, qi->base, qi->ch_cnt, qi->count, pdata, qi->rlist);
	if (pdata) g_free(pdata);

	qi->qrt->prev_tbase = qi->qrt->org_tbase;
	qi->qrt->prev_spp = qi->qrt->org_spp;
	qi->qrt->prev_count = qi->qrt->org_count;

	return 0;
}

int _tml_free_data(int ch_cnt, RLIST *rlist[])
{
	_free_rlist(ch_cnt, rlist);
	return 0;
}

int _tml_scan_data(RLIST *rlist[], int ch, POS pos)
{
	RLIST *plist = NULL;
	SZ_RBAR_T *rbar;

	for (plist = rlist[ch]; plist; plist = g_list_next(plist)) {
		rbar = (SZ_RBAR_T *)(plist->data);
		if (rbar->px_s <= pos && pos <= rbar->px_e) return rbar->reason;
	}
	return -1;
}
