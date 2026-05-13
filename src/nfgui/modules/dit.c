/*
 * dit.c
 * 	- drawing information table
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#include "dit.h"
#include "ix_mem.h"
#include "iux_afx.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		7
#define DBG_MODULE		"DIT"


#define MAX_PAGE	16

////////////////////////////////////////////////////////////
//
// private data type
//

typedef enum _IG_ARROW_E {
	NONE	= 0,
	POS		= 1,
	NEG		= 2,
	BTH		= 3
} IG_ARROW_E;

typedef struct _DIPAGE_HEADER {
	int				cnt_dic;
	int				en;
	IGCOLOR			color;
	IGRECT			invalid;
} DIPAGE_HEADER;

typedef struct _DIPAGE {
	DIPAGE_HEADER	header;
	GList			*dics;
} DIPAGE;

typedef struct _DIT_HEADER {
	int				enable;
	int				cnt_page;
	int				cnvs_w;
	int				cnvs_h;
	IGRECT			updated;
} DIT_HEADER;

typedef struct _DIT_T {
	DIT_HEADER	header;
	DIPAGE		dipage[MAX_PAGE];
} DIT_T;



////////////////////////////////////////////////////////////
//
// private interfaces
//

static int _init_updated_rect(DIT_T *pdit)
{
	pdit->header.updated.x = 0;
	pdit->header.updated.y = 0;
	pdit->header.updated.w = pdit->header.cnvs_w;
	pdit->header.updated.h = pdit->header.cnvs_h;

	return 0;
}

static int _init(DIT_T *pdit, int cnt_page, int cnvs_w, int cnvs_h)
{
	pdit->header.enable = 0;
	pdit->header.cnt_page = cnt_page;
	pdit->header.cnvs_w = cnvs_w;
	pdit->header.cnvs_h = cnvs_h;
	_init_updated_rect(pdit);

	return 0;
}

static int _print_dic_figure(DICPTR dic)
{
	int i;

	DMSG(9, "print start, cnt = %d", dic->ctns.f.cnt);
	for (i = 0; i < dic->ctns.f.cnt; ++i)
		printf("[%d, %d] ", dic->ctns.f.pt[i].x, dic->ctns.f.pt[i].y);

	printf("\n");
	DMSG(9, "print end");

	return 0;	
}

static DICPTR _add_dic_figure(DIT_T *pdit, int page, FIGURE_INFO *info)
{
	GList *dics = pdit->dipage[page].dics;
	DICPTR dic = imalloc(sizeof(DIC));

	DMSG(9, "%p, %p", dics, dic);
	memcpy(&dic->ctns, info, sizeof(FIGURE_INFO));
	dic->en = 1;
	dic->page = page;
	dic->type = DIC_FIG;
	pdit->dipage[page].dics = g_list_append(dics, dic);

//	_print_dic_figure(dic);

	return dic;
}

static DICPTR _add_dic_direction(DIT_T *pdit, int page, DIRECTION_INFO *info)
{
	GList *dics = pdit->dipage[page].dics;
	DICPTR dic = imalloc(sizeof(DIC));

	DMSG(9, "%p, %p", dics, dic);
	memcpy(&dic->ctns, info, sizeof(DIRECTION_INFO));
	dic->en = 1;
	dic->page = page;
	dic->type = DIC_DIR;
	pdit->dipage[page].dics = g_list_append(dics, dic);

//	_print_dic_figure(dic);

	return dic;
}

static DICPTR _add_dic_image(DIT_T *pdit, int page, IMAGE_INFO *info)
{
	GList *dics = pdit->dipage[page].dics;
	DICPTR dic = imalloc(sizeof(DIC));

	DMSG(9, "");
	memcpy(&dic->ctns, info, sizeof(IMAGE_INFO));
	dic->en = 1;
	dic->page = page;
	dic->type = DIC_IMG;
	pdit->dipage[page].dics = g_list_append(dics, dic);

	return dic;
}

static DICPTR _add_dic_text(DIT_T *pdit, int page, TEXT_INFO *info)
{
	GList *dics = pdit->dipage[page].dics;
	DICPTR dic = imalloc(sizeof(DIC));

	memcpy(&dic->ctns, info, sizeof(TEXT_INFO));
	dic->en = 1;
	dic->page = page;
	dic->type = DIC_TXT;
	pdit->dipage[page].dics = g_list_append(dics, dic);

	return dic;
}

static DICPTR _add_dic_arc(DIT_T *pdit, int page, ARC_INFO *info)
{
	GList *dics = pdit->dipage[page].dics;
	DICPTR dic = imalloc(sizeof(DIC));

	memcpy(&dic->ctns, info, sizeof(ARC_INFO));
	dic->en = 1;
	dic->page = page;
	dic->type = DIC_ARC;
	pdit->dipage[page].dics = g_list_append(dics, dic);

	return dic;
}

static int _remove_dic(DIT_T *pdit, int page, DICPTR dic)
{
	GList *dics = pdit->dipage[page].dics;
	dics = g_list_remove(dics, dic);
	ifree(dic);
	pdit->dipage[page].dics = dics;

	return 0;
}

static int _sum_rect(IGRECT *rect, IGRECT *sum)
{

}

static int _get_enabled_dic_count_in_page(DIPAGE *ppage)
{
	GList *pdics = NULL;
	GList *dics = NULL;
	int i;
	int cnt = 0;
	DICPTR dic;

	if (!ppage->header.en) return 0;
	dics = ppage->dics;
		
	for (pdics = dics; pdics; pdics = g_list_next(pdics)) {
		dic = (DICPTR)pdics->data;
		if (dic->en) ++cnt;
	}

	return cnt;
}

/*
static int _make_dic_list(DIPAGE *ppage, DICPTR *pdic)
{
	GList *pdics = NULL;
	GList *dics = NULL;
	int i = 0;
	int cnt = 0;
	DICPTR *pdic = NULL;
	DICPTR dic;

	DMSG(9, "");
	if (!ppage->header.en) return NULL;

	pdics = ppage->dics;
	for (pdics = dics; pdics; pdics = g_list_next(pdics)) {
		dic = (DICPTR)pdics ->data;
		if (dic->en) pdic[i++] = dic;
	}

	DMSG(9, "count = %d", cnt);
	return cnt;
}*/

static int _box_to_rect(IGBOX *box, IGRECT *rect)
{
	rect->x = box->x1;
	rect->w = box->x2 - box->x1 + 1;
	rect->y = box->y1;
	rect->h = box->y2 - box->y1 + 1;

	return 0;
}

static int _print_rect(IGRECT *rect)
{
	DMSG(9, "[%d,%d,%d,%d]\n",
			rect->x, rect->y, rect->w, rect->h);
	return 0;
}

static int _calc_dic_boundary(DICPTR *dic, int cnt, IGRECT *rect)
{
	int ptcnt;
	int i, j;
	IGPOINT *ppt;
	IGBOX box;
	int w_gap;
	int x, y;

rect->x = 0;
rect->y = 0;
rect->w = 3840;
rect->h = 2160;
return 0;

	if (cnt == 0) return -1;
	memset(rect, 0x00, sizeof(IGRECT));
	memset(&box, 0x00, sizeof(IGBOX));

	box.x1 = 99999;
	box.y1 = 99999;

	for (i = 0; i < cnt; ++i) {
		ptcnt = dic[i]->ctns.f.cnt;
		w_gap = dic[i]->ctns.f.wi / 2;

		for (j = 0; j < ptcnt; ++j) {
			ppt = &dic[i]->ctns.f.pt[j];
			x = ppt->x - w_gap;
			y = ppt->y - w_gap;

			if (x < box.x1) box.x1 = x;
			if (x > box.x2) box.x2 = x;

			if (y < box.y1) box.y1 = y;
			if (y > box.y2) box.y2 = y;
		}
	}

	_box_to_rect(&box, rect);
//	_print_rect(rect);
	return 0;
}

static int _convert_to_dit_coord(DIT_T *pdit, int x, int y, int *rx, int *ry)
{

}

static int _convert_to_2160_coord(DIT_T *pdit, int x,  int y, int *rx, int *ry)
{

}

static int _free_contents(gpointer data, gpointer user_data)
{
	if (data) ifree(data);
	return;
}

static int _free_dics(GList *dics)
{
	g_list_foreach(dics, _free_contents, 0);
	g_list_free(dics);
	return 0;
}

static int _get_enabled_dic_count(DITID id)
{
	int i;
	int cnt = 0;
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	for (i = 0; i < MAX_PAGE; ++i) 
		cnt += _get_enabled_dic_count_in_page(&pdit->dipage[i]);

	return cnt;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

DITID dit_create(int cnt_page, int cnvs_w, int cnvs_h)
{
	DITID ditid;

	ditid = (DITID)imalloc(sizeof(DIT_T));
	_init((DIT_T *)ditid, cnt_page, cnvs_w, cnvs_h);

	return ditid;
}

int dit_destroy(DITID id)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	ifree(pdit);
	return 0;
}

int dit_enable(DITID id)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	pdit->header.enable = 1;
	return 0;
}

int dit_is_enabled(DITID id)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return 0;

	return pdit->header.enable;
}

int dit_disable(DITID id)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	pdit->header.enable = 0;
	return 0;
}

int dit_enable_page(DITID id, int page)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	pdit->dipage[page].header.en = 1;
	return 0;
}

int dit_page_is_enabled(DITID id, int page)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return 0;

	return pdit->dipage[page].header.en;
}

int dit_disable_page(DITID id, int page)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	pdit->dipage[page].header.en = 0;
	return 0;
}

int dit_disable_all_page(DITID id)
{
	int i;
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	for (i = 0; i < pdit->header.cnt_page; ++i)
		pdit->dipage[i].header.en = 0;

	return 0;
}

int dit_get_cnvs_info(DITID id, int *w, int *h)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	*w = pdit->header.cnvs_w; 
	*h = pdit->header.cnvs_h; 
	return 0;
}

int dit_get_page_count(DITID id)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	return pdit->header.cnt_page;
}

DICPTR dit_add_dic(DITID id, int page, DIC_TYPE_E type, DIC_CONTENTS *ctns)
{
	DICPTR dic = 0;
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return 0;

	DMSG(9, "%p, %d", pdit, page);
	switch (type) {
	case DIC_FIG: dic = _add_dic_figure(pdit, page, &ctns->f); break;
	case DIC_DIR: dic = _add_dic_direction(pdit, page, &ctns->d); break;
	case DIC_IMG: dic = _add_dic_image(pdit, page, &ctns->i); break;
	case DIC_TXT: dic = _add_dic_text(pdit, page, &ctns->t); break;
	case DIC_ARC: dic = _add_dic_arc(pdit, page, &ctns->a); break;	
	}

	return dic;
}

int dit_remove_dic(DITID id, int page, DICPTR dic)
{
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	_remove_dic(id, page, dic);
	return 0;
}

DICPTR *dit_new_dic_list(DITID id, int *count, IGRECT *invalid)
{
	int i;
	int cnt = 0;
	GList *pdics = NULL;
	GList *dics = NULL;
	DIPAGE *ppage;
	DICPTR *pdic;
	DICPTR dic;
	DICPTR tmp;	
	int idx = 0;
	DIT_T *pdit = (DIT_T *)id;

	*count = 0;
	if (!pdit) return 0;

	if (!pdit->header.enable) 
	{
		// To be fixed
		*invalid = pdit->header.updated;		
		_calc_dic_boundary(pdic, *count, &pdit->header.updated);	
		return 0;
	}

	cnt = _get_enabled_dic_count(id);
	
	if (cnt == 0) {
		// To be fixed
		*invalid = pdit->header.updated;		
		_calc_dic_boundary(pdic, *count, &pdit->header.updated);	
		return 0;
	}

	pdic = imalloc(sizeof(DICPTR) * cnt);

	for (i = 0; i < MAX_PAGE; ++i) {
		ppage = &pdit->dipage[i];		
		if (!ppage->header.en) continue;

		dics = ppage->dics;
		for (pdics = dics; pdics; pdics = g_list_next(pdics)) {
			dic = (DICPTR)pdics->data;
			if (dic->en) {
//				pdic[idx++] = dic;
				tmp = imalloc(sizeof(DIC));
				memcpy(tmp, dic, sizeof(DIC));
				pdic[idx++] = tmp;
			}
		}
	}
	*count = cnt;

	*invalid = pdit->header.updated;
	_calc_dic_boundary(pdic, *count, &pdit->header.updated);
	return pdic;
}

DICPTR *dit_new_dic_list_under_pt(DITID id, IGPOINT pt, int *count, IGRECT *invalid)
{

}


int dit_free_dic_list(DICPTR *ditlist)
{
	if (ditlist) ifree(ditlist);
	return 0;
}

int dit_clear_page(DITID id, int page)
{
	GList *dics = 0;
	DIT_T *pdit = (DIT_T *)id;
	if (!pdit) return -1;

	DMSG(9, "clear page = %d\n", page);
	dics = pdit->dipage[page].dics;
	if (dics) {
		_free_dics(dics);
		pdit->dipage[page].dics = NULL;
	}

	_init_updated_rect(pdit);
	return 0;
}
