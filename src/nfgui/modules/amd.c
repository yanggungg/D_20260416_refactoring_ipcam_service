/*
 * amd.c
 * 	- media manager for viewer
 * 	- high level module over the scm
 *	- dependency :
 *		acp
 *		scm
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 18, 2011
 *
 */

#include "amd.h"
#include "iux_afx.h"
#include "scm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"AMD"

#define MAX_AMDA		16
#define PAGE_SIZE		17

////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _ARCH_MEDIA_T {
	MEDIA_ID	mid;
	ACPCTX		acp;
	AFILEID		start;		// min : 1 ~
	AFILEID		end;		// max : equal to cnt
	int			cnt;
} ARCH_MEDIA_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static ARCH_MEDIA_T amda[MAX_AMDA];
static const MEDIA_INFO_T *minfo;

////////////////////////////////////////////////////////////
//
// private functions
//

static int _find_empty_amda()
{
	int i;
	for (i = 0; i < MAX_AMDA; ++i) {
		if (amda[i].acp == 0) break;
	}
	g_assert(i < MAX_AMDA);

	return i;
}

static int _find_device_id(MEDIA_ID mid)
{
	int cnt = scm_get_media_count();
	int i;

	for (i = 0; i < cnt; ++i) {
		if (minfo[i].id == mid) break;
	}

	if (i == cnt) return -1;
	return scm_get_device_id(mid);
}

static int _fill_amda(int idx, int mid)
{
	int cnt;
	int dev_id = _find_device_id(mid);

	if (dev_id < 0) { DMSG(1, "INVALID DEVICE"); return -1; }
	amda[idx].mid = mid;
	amda[idx].acp = scm_create_archplay_ctx(dev_id);
	amda[idx].start = 1;
	cnt = scm_get_afile_count(amda[idx].acp);
	amda[idx].end = cnt > PAGE_SIZE ? PAGE_SIZE : cnt;

	return 0;
}

static int _remove_amda(int idx)
{
	amda[idx].mid = 0;
	if (amda[idx].acp) scm_destroy_archplay_ctx(amda[idx].acp);
	amda[idx].acp = INVALID;
	amda[idx].start = -1;
	amda[idx].end = -1;
	return 0;
}

static int _remove_all_amda()
{
	int i;
	for (i = 0; i < MAX_AMDA; ++i) _remove_amda(i);
	return 0;
}

static MEDIA_ID _find_media_id_by_title(char *ptitle)
{
	int cnt = scm_get_media_count();
	int i;

	for (i = 0; i < cnt; ++i) {
		if (strcmp(minfo[i].title, ptitle) == 0) {
			return minfo[i].id;
		}
	}

	return 0;
}

static int _find_index(ACPCTX acp)
{
	int i;
	for (i = 0; i < MAX_AMDA; ++i) {
		if (amda[i].acp == acp) break;
	}
	g_assert(i < MAX_AMDA);

	return i;
}

static int _reload_acp(MEDIA_ID mid)
{
	int i;

	for (i = 0; i < MAX_AMDA; ++i) {
		if (amda[i].mid == mid) break;
	}
	if (i == MAX_AMDA) return -1;

	_remove_amda(i);
	_fill_amda(i, mid);
	
	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int amd_init(const MEDIA_INFO_T *media_info)
{
	int i;
	
	minfo = media_info;
	memset(amda, 0x00, sizeof(ARCH_MEDIA_T) * MAX_AMDA);
	for (i = 0; i < MAX_AMDA; ++i) {
		amda[i].start = -1;
		amda[i].end = -1;
	}
	return 0;
}

int amd_cleanup()
{
	_remove_all_amda();
	minfo = 0;
	return 0;
}

int amd_remove_all_amda()
{
	_remove_all_amda();
	return 0;
}

ACPCTX amd_get_acp(char *title)
{
	int i;
	int empty;
	int ret;
	int cnt;
	MEDIA_ID mid = _find_media_id_by_title(title);

	DMSG(1, "MEDIA TITLE = %s\n", title);
	if (mid == 0) return INVALID;
	for (i = 0; i < MAX_AMDA; ++i) {
		if (amda[i].mid == mid) break;
	}

	if (i == MAX_AMDA) {
		empty = _find_empty_amda();
		ret = _fill_amda(empty, mid);
		if (ret < 0) return INVALID;
		i = empty;
	}

	//if (mda_get_media_type(mid) == MTYPE_ODD) _reload_acp(mid);
	_reload_acp(mid);
	if (amda[i].acp == INVALID) return INVALID;

	amda[i].cnt = scm_get_afile_count(amda[i].acp);
	return amda[i].acp;
}

AFILEID amd_find_start_afile(ACPCTX acp)
{
	int idx = _find_index(acp);
	return amda[idx].start;
}

AFILEID amd_find_end_afile(ACPCTX acp)
{
	int idx = _find_index(acp);
	return amda[idx].end;
}

int amd_move_to_prev_page(ACPCTX acp)
{
	int idx = _find_index(acp);
	if (amda[idx].start - PAGE_SIZE < 0) return -1;	

	amda[idx].start -= PAGE_SIZE;
	amda[idx].end -= PAGE_SIZE;

	if (amda[idx].end < amda[idx].cnt) amda[idx].end = amda[idx].start + PAGE_SIZE - 1;

	return 0;
}

int amd_move_to_next_page(ACPCTX acp)
{
	int idx = _find_index(acp);
	if (amda[idx].start + PAGE_SIZE > amda[idx].cnt) return -1;	

	amda[idx].start += PAGE_SIZE;
	amda[idx].end += PAGE_SIZE;

	if (amda[idx].end > amda[idx].cnt) amda[idx].end = amda[idx].cnt;

	return 0;
}

AFILEID amd_get_selected_afile(ACPCTX acp, int offset)
{
	int idx = _find_index(acp);
	return amda[idx].start + offset;
}

int amd_get_afile_count(ACPCTX acp)
{
	return scm_get_afile_count(acp);
}

AFILE_INFO_T *amd_new_afile_list(ACPCTX acp, AFILEID start, AFILEID end)
{
	return scm_new_afile_list(acp, start, end);
}

int amd_free_afile_list(AFILE_INFO_T *afile_list)
{
	return scm_free_afile_list(afile_list);
}

int amd_set_play_afile(ACPCTX acp, AFILEID id)
{
	return scm_set_play_afile(acp, id);
}
