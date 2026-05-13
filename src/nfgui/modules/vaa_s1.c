/*
 * vaa_s1.c
 * 	- video analytics agent for s1
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#include "dit.h"
#include "vaa_s1.h"
#include "vaa_s1_internal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "../support/nf_ui_image.h"
#include <math.h>

DECLARE DBG_SYSTEM

#define DBG_LEVEL		7	
#define DBG_MODULE		"VAA_S1"

#define S1_MAX_RULE		9

#define CR_NORMAL_LINE	1102
#define CR_NORMAL_AREA	1105
#define CR_NORMAL_TBOX	1104
#define CR_EVENT_TBOX	1103
#define IS_LINE_FIGURE(f)	(f->cnt == 2)
#define	SEC_2			(4)		// 2000ms

#define FIGURE		0
#define DIRCTRL		1

////////////////////////////////////////////////////////////
//
// private data type
//

typedef VARULE*		VARULEPTR;

////////////////////////////////////////////////////////////
//
// private variables
//

static DB_LINK s_dblink[16] = {
	{0, -1},
	{1, -1},
	{2, -1},
	{3, -1},
	{4, -1},
	{5, -1},
	{6, 0},
	{7, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
	{-1, -1},
};


static char s_table_patt[PATTERN_MAX] = {
	PATTERN_INVASION,
	PATTERN_LOITERING,
	PATTERN_ABANDON,
	PATTERN_STEAL,
	PATTERN_TOPPLE,
	PATTERN_FENCE,
	PATTERN_COUNT,
	PATTERN_TAMPERING,
	PATTERN_PRIVACY,
};

static char *s_dirimg_name[8] = {	// placed in counterclockwise
		IMG_VA_ARROW_LEFT,
		IMG_VA_ARROW_DOWNLEFT,
		IMG_VA_ARROW_DOWN,
		IMG_VA_ARROW_DOWNRIGHT,
		IMG_VA_ARROW_RIGHT,
		IMG_VA_ARROW_UPRIGHT,
		IMG_VA_ARROW_UP,
		IMG_VA_ARROW_UPLEFT,
};




////////////////////////////////////////////////////////////
//
// private functions
//

static int _init_patt(S1VAA_T *pvaa)
{
	pvaa->cnt_patt = 8;

	pvaa->vapatt[0].patt = PATTERN_INVASION;
	pvaa->vapatt[0].dipage = 0;

	pvaa->vapatt[1].patt = PATTERN_LOITERING;
	pvaa->vapatt[1].dipage = 1;

	pvaa->vapatt[2].patt = PATTERN_ABANDON;
	pvaa->vapatt[2].dipage = 2;

	pvaa->vapatt[3].patt = PATTERN_STEAL;
	pvaa->vapatt[3].dipage = 3;

	pvaa->vapatt[4].patt = PATTERN_TOPPLE;
	pvaa->vapatt[4].dipage = 4;

	pvaa->vapatt[5].patt = PATTERN_FENCE;
	pvaa->vapatt[5].dipage = 5;

	pvaa->vapatt[6].patt = PATTERN_COUNT;
	pvaa->vapatt[6].dipage = 6;

	pvaa->vapatt[7].patt = PATTERN_TAMPERING;
	pvaa->vapatt[7].dipage = 7;

//	pvaa->vapatt[8].patt = PATTERN_PRIVACY;
//	pvaa->vapatt[8].dipage = 8;

	return 0;
}

static int _init_meta(S1VAA_T *pvaa)
{
	pvaa->cnt_meta = 1;

	pvaa->vameta[0].meta = VAA_META_BBOX;
	pvaa->vameta[0].dipage = 9;

	return 0;
}

static int _init_rule(VARULEPTR prule)
{
	memset(prule, 0x00, sizeof(VARULE));

	prule->zn_data.id = -1;
	prule->ct_data.id = -1;
/*	prule->use = 0;
	prule->type = type;
	prule->patt = patt;
	prule->dbid = dbid;

	_init_zone_db(&prule->zn_data);	
	_init_cntr_db(&prule->ct_data);	*/
	return 0;	
}

static int _init(S1VAA_T *pvaa, int ch)
{
	memset(pvaa, 0x00, sizeof(S1VAA_T));
	pvaa->ch = ch;

	_init_patt(pvaa);
	_init_meta(pvaa);

	return 0;	
}

static int _load_rule_data(VCAZone *pzone, VCACntr *pcntr, VARULEPTR prule)
{
	if (pzone) memcpy(&prule->zn_data, pzone, sizeof(VCAZone));
	if (pcntr) memcpy(&prule->ct_data, pcntr, sizeof(VCACntr));

	return 0;
}

static int _unload_rule_data(VARULEPTR prule, VCAZone *pzone, VCACntr *pcntr)
{
	if (pzone) memcpy(pzone, &prule->zn_data, sizeof(VCAZone));
	if (pcntr) memcpy(pcntr, &prule->ct_data, sizeof(VCACntr));

	return 0;
}

static int _load_prop_data(S1VAA_T *pvaa)
{
	memcpy(&pvaa->prop, &pvaa->db, sizeof(VCAPropData));
	return 0;
}

static int _get_dir_position(IGPOINT pt1, IGPOINT pt2, IGPOINT *dst)
{
	gint mptx = (pt1.x + pt2.x) >> 1;
	gint mpty = (pt1.y + pt2.y) >> 1;	
	dst->x = mptx;// - 96;
	dst->y = mpty;// - 96;
	
	return 0;
}

static char *_get_dir_img_name(IGPOINT pt1, IGPOINT pt2, int dir)
{
	float df = 0;
	int di = 0;

	gint mptx = (pt1.x + pt2.x) >> 1;
	gint mpty = (pt1.y + pt2.y) >> 1;	
	gint dx = pt2.x - pt1.x, dy = pt2.y - pt1.y;	

	df = atan2(dx, dy) * 180 / M_PI;
	if (df < 0) df += 360;
	di = (((int)(df + 45/2)) % 360) / 45;

	if (dir == 2) di = (di + 4) % 8;
//	else if (dir == 3) di += 8; 
	return s_dirimg_name[di];
}

static int _get_point_from_zone(IGPOINT *pt, int *cnt, VCAZone *pz)
{
	int i;
	int npts = pz->npts;
	
	DMSG(9, "npts = %d\n", npts);
	for (i = 0; i < npts; ++i) {
		pt[i].x = pz->pt[i].x;
		pt[i].y = pz->pt[i].y;
		DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
		pt[i].opt = 0;
	}
	*cnt = npts;

	return 0;
}

static int _update_direction_info(VARULEPTR prule)
{
	IGPOINT pt[MAX_PT];
	IGPOINT dst;
	int cnt;

	memset(pt, 0x00, sizeof(pt));
	memset(&dst, 0x00, sizeof(dst));

	_get_point_from_zone(pt, &cnt, &prule->zn_data);
	_get_dir_position(pt[0], pt[1], &dst);

	prule->dirinfo.pt = dst;
	return 0;
}

static char *_get_dir_img(VARULEPTR prule)
{
	IGPOINT	pt[16];
	int cnt = 0;
	char *img;
	int dir;

	memset(pt, 0x00, sizeof(pt));

	_get_point_from_zone(pt, &cnt, &prule->zn_data);
	dir = prule->zn_data.enabled & 0x03;
	img = _get_dir_img_name(pt[0], pt[1], dir);
	return img;
}

static int _get_dir_pos(VARULEPTR prule, IGPOINT *dst)
{
	IGPOINT	pt[16];
	int cnt = 0;

	memset(pt, 0x00, sizeof(IGPOINT));

	_get_point_from_zone(pt, &cnt, &prule->zn_data);
	_get_dir_position(pt[0], pt[1], dst);
	
	return 0;
}

static int _update_direction_to_dic(VARULEPTR prule)
{
	char *img;
	if (!prule->dic[FIGURE]) return -1;

	prule->dic[DIRCTRL]->ctns.i.pt = prule->dirinfo.pt;
	img = _get_dir_img(prule);
	prule->dic[DIRCTRL]->ctns.i.img.ptr = img;
	return 0;
}

static int _update_direction(VARULEPTR prule)
{
	_update_direction_info(prule);
	_update_direction_to_dic(prule);
	return 0;
}

static int _print_db(S1VAA_T *pvaa)
{
	int i;
	for (i = 0; i < S1_MAX_RULE; ++i) {
		printf("[%d] name           = [%s]\n", i, pvaa->db.zonelist.zone[i].name);
		printf("[%d] id             = [%d]\n", i, pvaa->db.zonelist.zone[i].id);
		printf("[%d] type           = [%u]\n", i, pvaa->db.zonelist.zone[i].type);
		printf("[%d] active         = [%d]\n", i, pvaa->db.zonelist.zone[i].active);
		printf("[%d] enabled        = [%d]\n", i, pvaa->db.zonelist.zone[i].enabled);
		printf("[%d] stop time      = [%u]\n", i, pvaa->db.zonelist.zone[i].stop_time);
		printf("[%d] abandon time   = [%u]\n", i, pvaa->db.zonelist.zone[i].abandon_time);
		printf("[%d] remove time    = [%u]\n", i, pvaa->db.zonelist.zone[i].remove_time);
		printf("[%d] roiter time    = [%u]\n", i, pvaa->db.zonelist.zone[i].loiter_time);
		printf("[%d] fail time      = [%u]\n", i, pvaa->db.zonelist.zone[i].fall_time);
		printf("[%d] ecolor         = [%u]\n", i, pvaa->db.zonelist.zone[i].ecolor);
		printf("[%d] ecolor_sens    = [%d]\n", i, pvaa->db.zonelist.zone[i].ecolor_sens);
		printf("[%d] speed_min      = [%u]\n", i, pvaa->db.zonelist.zone[i].speed_min);
		printf("[%d] speed_max      = [%u]\n", i, pvaa->db.zonelist.zone[i].speed_max);
		printf("[%d] eclass         = [%u]\n", i, pvaa->db.zonelist.zone[i].eclass);
		printf("[%d] color          = [%u]\n", i, pvaa->db.zonelist.zone[i].color);
		printf("[%d] npts           = [%d]\n", i, pvaa->db.zonelist.zone[i].npts);
		printf("[%d] pt             = [%d, %d], [%d, %d], [%d, %d], [%d, %d]\n", i, 
				pvaa->db.zonelist.zone[i].pt[0].x, pvaa->db.zonelist.zone[i].pt[0].y,
				pvaa->db.zonelist.zone[i].pt[1].x, pvaa->db.zonelist.zone[i].pt[1].y,
				pvaa->db.zonelist.zone[i].pt[2].x, pvaa->db.zonelist.zone[i].pt[2].y,
				pvaa->db.zonelist.zone[i].pt[3].x, pvaa->db.zonelist.zone[i].pt[3].y,
				pvaa->db.zonelist.zone[i].pt[4].x, pvaa->db.zonelist.zone[i].pt[4].y);

		printf("--------------------------------------------------------------------------\n");
	}

	return 0;
}

static int _load_db(S1VAA_T *pvaa)
{
	DAL_get_vca_data(&pvaa->db, pvaa->ch);

//	_print_db(pvaa);
	return 0;
}

static int _save_db(S1VAA_T *pvaa)
{
	DAL_set_vca_data(pvaa->db, pvaa->ch);

//	_print_db(pvaa);
	return 0;
}

static int _find_empty_db(S1VAA_T *pvaa)
{
	int i;
	for (i = 0; i < IVCA_MAX_ZONES; ++i) {
		if (pvaa->used.zn_data[i] == 0) return i;
	}

	return -1;
}

static int _get_empty_new_rule(S1VAA_T *pvaa)
{
	int i;
	for (i = 0; i < MAX_RULE; ++i) {
		if (pvaa->varule[i].occupied == 0) return i;
	}

	return -1;
}

static VARULEPTR _get_rule_ptr(S1VAA_T *pvaa, RULEID rule)
{
	return (VARULEPTR)&pvaa->varule[rule];
}

static int _init_zone_db(VCAZone *pz)
{
	return 0;
}

static int _init_cntr_db(VCACntr *pc)
{
	return 0;
}

static int _make_polygon(IGPOINT *fpt, int fcnt, IXPOLYGON *p)
{
	int i;
	
	for (i = 0; i < fcnt; ++i) {
		p->pt[i].x = fpt[i].x;
		p->pt[i].y = fpt[i].y;
	}

	p->cnt = fcnt;
	return 0;
}

static int _is_point_in_figure(IGPOINT mpt, IGPOINT *fpt, int fcnt)
{
	int i, ret;
	IXPOLYGON p;
	
	for (i = 0; i < fcnt; ++i) {
		p.pt[i].x = fpt[i].x;
		p.pt[i].y = fpt[i].y;
	}

	p.cnt = fcnt;
	ret = ifn_point_is_inside_polygon(mpt.x, mpt.y, 16, &p);	
	return (ret > 0 ? 0 : -1); 
}

static int _makeup_figure_rule(VARULEPTR prule, FIGURE_INFO *info)
{
	if (IS_LINE_FIGURE(info))
		info->cl.i	= CR_NORMAL_LINE;
	else
		info->cl.i	= CR_NORMAL_AREA;

	if (prule->sty_highlight) {
		info->vt = 1;
		info->wi = 5;
	}
	else {
		info->vt = 0;
		info->wi = 3;
	}

	return 0;
}

static int _add_rule_figure_to_display(S1VAA_T *pvaa, int page, VARULEPTR prule)
{
	FIGURE_INFO finfo;
	IGPOINT		pt[16];
	int cnt = 0;
	DITID ditid;
	DICPTR pdic;

	if (_IS_NOFIGURE(prule)) return -1;

	ditid = pvaa->ditid;
	_vaa_init_figure(&finfo);
	_get_point_from_zone(pt, &cnt, &prule->zn_data);
	_vaa_construct_figure(pt, cnt, &finfo);
	_makeup_figure_rule(prule, &finfo);

	pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
	prule->dic[FIGURE] = pdic;
	prule->dic[FIGURE]->en = prule->zn_data.active;

	return 0;
}

static int _add_rule_direction_to_display(S1VAA_T *pvaa, int page, VARULEPTR prule)
{
	IMAGE_INFO iinfo;
	DITID ditid;
	DICPTR pdic;

	if (_IS_NOFIGURE(prule)) return -1;
	if (!_IS_DIRCTRL(prule)) return -1;

	ditid = pvaa->ditid;
	_vaa_init_image(&iinfo);
	_vaa_construct_direction(prule, &iinfo);

	pdic = dit_add_dic(ditid, page, DIC_IMG, &iinfo);
	prule->dic[DIRCTRL] = pdic; 	
	prule->dic[DIRCTRL]->en = prule->zn_data.active;
	DMSG(9, "%d", prule->zn_data.active);

	return 0;
}

static int _add_rule_to_display(S1VAA_T *pvaa, int page, VARULEPTR prule)
{
	if (!vaa_s1_is_dit_linked(pvaa)) return -1;

	DMSG(9, "%d\n", page);
	_add_rule_figure_to_display(pvaa, page, prule);
	_add_rule_direction_to_display(pvaa, page, prule);
	return 0;
}

static int _makeup_figure_trackbox(FIGURE_INFO *info, int evt)
{
	if (evt) info->cl.i	= CR_EVENT_TBOX;	
	else info->cl.i	= CR_NORMAL_TBOX;
	return 0;
}

static int _get_meta_bound(ivca_meta_obj_t *mobj, IGRECT *rect)
{
	rect->x = mobj->rc.x;
	rect->y = mobj->rc.y;
	rect->w = mobj->rc.w;
	rect->h = mobj->rc.h;
	
	return 0;
}

static int _is_evented_meta(ivca_meta_obj_t *mobj)
{
	return (mobj->nevents > 0);
}

static DICPTR _add_trackbox_to_display(S1VAA_T *pvaa, int page, IGRECT *rect, int evt)
{
	FIGURE_INFO info;
	DITID ditid;
	DICPTR dic;

	if (!vaa_s1_is_dit_linked(pvaa)) return -1;
	ditid = pvaa->ditid;

	_vaa_init_figure(&info);
	_vaa_construct_figure_rect(rect, &info);
	_makeup_figure_trackbox(&info, evt);

	dic = dit_add_dic(ditid, page, DIC_FIG, &info);
	return dic;
}

static int _get_page_by_ruleid(RULEID id)
{
	return id;
}

static int _add_pattern_rules_to_display(S1VAA_T *pvaa, PATTERN_E patt)
{
	int i;
	VARULEPTR prule;
	DITID	ditid = pvaa->ditid;
	int page;

	for (i = 0; i < pvaa->cnt_rule; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)i);
		if (patt != prule->patt) continue;

		page = pvaa->vapatt[prule->patt].dipage;
		_add_rule_to_display(pvaa, page, prule);
	}

	return 0;
}

static int _set_rule(VARULEPTR prule, PATTERN_E patt, char lz, char lc)
{
	prule->occupied = 1;

	DMSG(9, "");
	switch (patt) {
	case PATTERN_INVASION:
	case PATTERN_LOITERING:
	case PATTERN_ABANDON:
	case PATTERN_STEAL:
	case PATTERN_TOPPLE:
		prule->type |= MASK_AREA;
		break;

	case PATTERN_FENCE:
		prule->type |= MASK_LINE;
		prule->type |= MASK_DIRCTRL;
		break;

	case PATTERN_COUNT:
		prule->type |= MASK_LINE;
		prule->type |= MASK_COUNT;
		prule->type |= MASK_DIRCTRL;
		break;

	case PATTERN_TAMPERING:
	case PATTERN_PRIVACY:
		prule->type &= ~MASK_FIGURE;
		break;
	}

	prule->patt = patt;
	prule->dlink.lz = lz;
	prule->dlink.lc = lc;
	prule->blink_step = -1;
	prule->blink_en = 0;

	return 0;
}

static PATTERN_E  _convert_ruleid_to_patt(RULEID ruleid)
{
	return s_table_patt[ruleid];
}

static int _translate_vcdata_into_rule(S1VAA_T *pvaa, VCAData *pdb)
{
	RULEID ruleid;
	VCAZone *pzone;
	VCACntr *pcntr;
	PATTERN_E patt;
	VARULEPTR prule;
	char lz, lc;

	pvaa->zn_count = pdb->zonelist.cnt;
	pvaa->ct_count = pdb->cntrlist.cnt;

	pvaa->cnt_rule = S1_MAX_RULE;
	for (ruleid = 0; ruleid < pvaa->cnt_rule; ++ruleid) {
		lz = s_dblink[ruleid].lz;
		lc = s_dblink[ruleid].lc;
		DMSG(9, "LZ = %d\n", lz);
		DMSG(9, "LC = %d\n", lc);

		if (lz != -1) pzone = &pdb->zonelist.zone[lz];
		else pzone = NULL;
		if (lc != -1) pcntr = &pdb->cntrlist.cntr[lc];
		else pcntr = NULL;

		prule = _get_rule_ptr(pvaa, ruleid);
		patt = _convert_ruleid_to_patt(ruleid);

		_init_rule(prule);
		_set_rule(prule, patt, lz, lc);
		_load_rule_data(pzone, pcntr, prule);

		if (_IS_DIRCTRL(prule)) _update_direction_info(prule);

		DMSG(9, "zone id=%d, cntr id=%d\n", prule->zn_data.id, prule->ct_data.id);
	}

	return 0;
}

static int _translate_rule_into_vcdata(S1VAA_T *pvaa, VCAData *pdb)
{
	RULEID ruleid;
	VCAZone *pzone;
	VCACntr *pcntr;
	RULEID rule;
	VARULEPTR prule;
	char lz, lc;

	for (ruleid = 0; ruleid < pvaa->cnt_rule; ++ruleid) {
		prule = _get_rule_ptr(pvaa, ruleid);

		lz = prule->dlink.lz;
		lc = prule->dlink.lc;

		DMSG(9, "LZ = %d\n", lz);
		DMSG(9, "LC = %d\n", lc);
		if (lz != -1) pzone = &pdb->zonelist.zone[lz];
		else pzone = NULL;
		if (lc != -1) pcntr = &pdb->cntrlist.cntr[lc];
		else pcntr = NULL;

		_unload_rule_data(prule, pzone, pcntr);
	}

	DMSG(9, "ZN CNT = %d\n", pvaa->zn_count);
	DMSG(9, "CT CNT = %d\n", pvaa->ct_count);
	pdb->zonelist.cnt = pvaa->zn_count;
	pdb->cntrlist.cnt = pvaa->ct_count;

	return 0;
}

static int _unlink_dic(VARULEPTR prule)
{
	int i;
	for (i = 0; i < MAX_DIC; ++i) prule->dic[i] = 0;
	return 0;
}

static int _redraw_patterns(S1VAA_T *pvaa)
{
	int i;
	for (i = 0; i < pvaa->cnt_patt; ++i) {
		DMSG(9, "ditid = %p, %d\n", pvaa->ditid, i);
		dit_clear_page(pvaa->ditid, pvaa->vapatt[i].dipage);
		_add_pattern_rules_to_display(pvaa, pvaa->vapatt[i].patt);
	}

	return 0;
}

static int _get_figure(VARULEPTR prule, IGPOINT *pt, int *cnt)
{
	*cnt = 0;
	if (_IS_NOFIGURE(prule)) return -1;

	*cnt = prule->dic[FIGURE]->ctns.f.cnt;
	memcpy(pt, prule->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
	return 0;
}

static int _get_dirpos(VARULEPTR prule, IGRECT *dirpos)
{
	if (_IS_NOFIGURE(prule)) return -1;

	dirpos->x = prule->dirinfo.pt.x-96;
	dirpos->y = prule->dirinfo.pt.y-96;
	dirpos->w = 96 * 2; 	// constant
	dirpos->h = 96 * 2; 
	return 0;	
}

static int _set_figure_to_dic(VARULEPTR prule, IGPOINT *pt, int cnt)
{
	if (_IS_NOFIGURE(prule)) return -1;

	prule->dic[FIGURE]->ctns.f.cnt = cnt;
	memcpy(prule->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
	return 0;
}

static int _set_figure_to_zndata(VARULEPTR prule, IGPOINT *pt, int cnt)
{
	int i;
	if (_IS_NOFIGURE(prule)) return -1;

	prule->zn_data.npts = cnt;
	for (i = 0; i < cnt; ++i) {
		prule->zn_data.pt[i].x = pt[i].x;
		prule->zn_data.pt[i].y = pt[i].y;
	}
	return 0;
}

static int _set_figure(VARULEPTR prule, IGPOINT *pt, int cnt)
{
	_set_figure_to_dic(prule, pt, cnt);
	_set_figure_to_zndata(prule, pt, cnt);
	return 0;
}

static int _make_highlight(VARULEPTR prule)
{
	if (_IS_NOFIGURE(prule)) return -1;

	prule->sty_highlight = 1;
	prule->dic[FIGURE]->ctns.f.vt = 1;
	prule->dic[FIGURE]->ctns.f.wi = 5;
	return 0;
}

static int _make_lowlight(VARULEPTR prule)
{
	if (_IS_NOFIGURE(prule)) return -1;

	prule->sty_highlight = 0;
	prule->dic[FIGURE]->ctns.f.vt = 0;
	prule->dic[FIGURE]->ctns.f.wi = 3;
	return 0;
}

static int _update_ruledb_count(S1VAA_T *pvaa, VARULEPTR prule, int change)
{
	if (change == 1) pvaa->zn_count++;
	else if (change == -1) pvaa->zn_count--;

	if (prule->patt == PATTERN_COUNT) {
		if (change == 1) pvaa->ct_count++;
		else if (change == -1) pvaa->ct_count--;
	}
	return 0;
}

static int _apply_conf(VARULE_CONF *conf, VARULEPTR prule)
{
	int prev_act = prule->zn_data.active;
	int cur_act;

	prule->patt = conf->patt;
	switch (prule->patt) {
	case PATTERN_INVASION:
	case PATTERN_TOPPLE:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.sensitivity = conf->cfg_sens;
		prule->dic[FIGURE]->en = conf->use_rule;
		break;
	case PATTERN_FENCE:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.enabled &= ~0x03;
		prule->zn_data.enabled |= (0x03 & conf->cfg_dir);
		prule->dic[FIGURE]->en = conf->use_rule;
		prule->dic[DIRCTRL]->en = conf->use_rule;
		break;
	case PATTERN_LOITERING:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.loiter_time = conf->cfg_time;
		prule->dic[FIGURE]->en = conf->use_rule;
		break;
	case PATTERN_ABANDON:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.stop_time = conf->cfg_time;
		prule->dic[FIGURE]->en = conf->use_rule;
		break;
	case PATTERN_STEAL:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.remove_time = conf->cfg_time;
		prule->dic[FIGURE]->en = conf->use_rule;
		break;
	case PATTERN_COUNT:
		prule->zn_data.active = conf->use_rule;
		prule->zn_data.enabled &= ~0x03;
		prule->zn_data.enabled |= (0x03 & conf->cfg_dir);
//		prule->ct_data.enabled &= ~0x03;
//		prule->ct_data.enabled |= (0x03 & conf->cfg_dir);
		prule->dic[FIGURE]->en = conf->use_rule;
		prule->dic[DIRCTRL]->en = conf->use_rule;
		if (conf->use_rule) prule->ct_data.active = 1;
		else prule->ct_data.active = 0;
		break;
	case PATTERN_TAMPERING:
		return 0;
		break;
	}
	cur_act = prule->zn_data.active;

	if (conf->sty_highlight) _make_highlight(prule);
	else _make_lowlight(prule);

	if (prule->blink_step != -1) {
		if (prule->blink_en) {
			if (prule->blink_step % 2 == 0) {
				prule->dic[FIGURE]->en = 0;
				if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 0;
			}
			else {
				prule->dic[FIGURE]->en = 1;
				if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 1;
			}
		}
	}

	if (_IS_DIRCTRL(prule)) {
		_update_direction(prule);
	}

	//  1: increase zone count
	// -1: decreace zone count
	return (cur_act - prev_act);
}

static int _apply_global_conf(VARULE_CONF *conf, S1VAA_T *pvaa) 
{
	switch (conf->patt) {
	case PATTERN_TAMPERING:
		pvaa->prop.en_tamper = conf->use_rule;
		break;
	}

	return 0;
}

static int _apply_confs(VARULE_CONF *conf, int cnt, S1VAA_T *pvaa)
{
	int change;
	int i;
	int idx = 0;
	VARULEPTR prule;
	if (cnt == 0) return -1;

	for (i = 0; i < cnt; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)conf[i].ruleid);

		change = _apply_conf(&conf[i], prule);
		_update_ruledb_count(pvaa, prule, change);
		_apply_global_conf(&conf[i], pvaa);

	}

	return 0;
}

static int _bring_conf(VARULEPTR prule, VARULE_CONF *conf)
{
	conf->patt = prule->patt;

	switch (prule->patt) {
	case PATTERN_INVASION:
	case PATTERN_TOPPLE:
		conf->use_rule = prule->zn_data.active;
		conf->cfg_sens = prule->zn_data.sensitivity;
		break;
	case PATTERN_LOITERING:
		conf->use_rule = prule->zn_data.active;
		conf->cfg_time = prule->zn_data.loiter_time;
		break;
	case PATTERN_ABANDON:
		conf->use_rule = prule->zn_data.active;
		conf->cfg_time = prule->zn_data.stop_time;
		break;
	case PATTERN_STEAL:
		conf->use_rule = prule->zn_data.active;
		conf->cfg_time = prule->zn_data.remove_time;
		break;
	case PATTERN_FENCE:
	case PATTERN_COUNT:
		conf->use_rule = prule->zn_data.active;
		conf->cap_dirctrl = 0x06;	// support pos, neg
		break;
	}

	conf->sty_highlight = prule->sty_highlight;
	conf->cfg_dir = prule->zn_data.enabled & 0x03;

	return 0;
}

static int _bring_global_conf(PATTERN_E patt, S1VAA_T *pvaa, VARULE_CONF *conf)
{
	switch (patt) {
	case PATTERN_TAMPERING:
		conf->use_rule = pvaa->prop.en_tamper;
		break;
	}

	return 0;
}

static int _copy_prop_to_db(VCAPropData *src, VCAData *pdb)
{
	memcpy(&pdb->prop, src, sizeof(VCAPropData));
	return 0;
}

static int _copy_prop_to_rule(VCAData *pdb, VCAPropData *dst)
{
	memcpy(dst, &pdb->prop, sizeof(VCAPropData));
	return 0;
}

static int _bring_confs(S1VAA_T *pvaa, PATTERN_E patt, VARULE_CONF conf[16], int *cnt)
{
	int i;
	int idx = 0;
	VARULEPTR prule;

	for (i = 0; i < pvaa->cnt_rule; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)i);
		if (patt != prule->patt) continue;

		_bring_conf(prule, &conf[idx]);
		_bring_global_conf(patt, pvaa, &conf[idx]);
		++idx;
		if (idx + 1 > 16) return -1;
	}

	if (cnt) *cnt = idx;

	return 0;
}

static int _bring_confs_all(S1VAA_T *pvaa, VARULE_CONF conf[16], int *cnt)
{
	int i;
	VARULEPTR prule;

	for (i = 0; i < pvaa->cnt_rule; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)i);

		_bring_conf(prule, &conf[i]);
		_bring_global_conf(prule->patt, pvaa, &conf[i]);
	}
	if (cnt) *cnt = i;

	return 0;
}

static int _is_pattern_using(S1VAA_T *pvaa, PATTERN_E patt)
{
	return pvaa->varule[patt].zn_data.active;
}

static int _control_dit(S1VAA_T *pvaa)
{
	int i;
	PATTERN_E patt;

	if (vaa_s1_is_enabled((VAAID)pvaa)) dit_enable(pvaa->ditid);
	else dit_disable(pvaa->ditid);

	vaa_s1_activate_all_pattern((VAAID)pvaa);

/*
	for (i = 0; i < pvaa->cnt_rule; ++i) {
		patt = _convert_ruleid_to_patt(i);
		
		if (_is_pattern_using(pvaa, patt)) vaa_s1_activate_pattern((VAAID)pvaa, patt);
		else vaa_s1_deactivate_pattern((VAAID)pvaa, patt);
	}
*/	
}

static int _link_dit(S1VAA_T *pvaa, DITID ditid)
{
	pvaa->ditid = ditid;	
	dit_disable_all_page(pvaa->ditid);
	return 0;
}

static int _get_rule_id(S1VAA_T *pvaa, PATTERN_E patt, int evt_ruleid)
{
	int i;

	if (patt == PATTERN_COUNT) {
		for (i = 0; i < pvaa->cnt_rule; ++i)
			if (pvaa->varule[i].ct_data.id == evt_ruleid) return i;
	}
	else {
		for (i = 0; i < pvaa->cnt_rule; ++i)
			if (pvaa->varule[i].zn_data.id == evt_ruleid) return i;
	}

	return -1;

}

static int _show_blinking(VARULEPTR prule)
{
	if (prule->blink_step != -1) {
		--prule->blink_step;
		
		if (prule->blink_en) {
			if (prule->blink_step % 2 == 0) {
				prule->dic[FIGURE]->en = 0;
				if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 0;
			}
			else {
				prule->dic[FIGURE]->en = 1;
				if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 1;
			}
		}

		if (prule->blink_step == -1) {
			prule->blink_en = 0;
			_make_lowlight(prule);
		}
	}
	return 0;
}

static int _proc_blinking(S1VAA_T *pvaa)
{
	int i;
	VARULEPTR prule;
	for (i = 0; i < pvaa->cnt_rule; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)i);
		if (!prule->occupied) continue;

		_show_blinking(prule);
	}
	return 0;
}

static int _proc_meta_erasing(S1VAA_T *pvaa)
{
	int i;
	int dipage;

	dipage = pvaa->vameta[0].dipage;
	for (i = 0; i < pvaa->cnt_tbox; ++i) {
		if (pvaa->vatbox[i].dic == 0) continue;

		--pvaa->vatbox[i].age;
		if (pvaa->vatbox[i].age == 0) {
			dit_remove_dic(pvaa->ditid, dipage, pvaa->vatbox[i].dic);
			pvaa->vatbox[i].dic = 0;
		}
	}
	return 0;
}

static int _clear_meta(S1VAA_T *pvaa)
{
	int i;
	int dipage;
	for (i = 0; i < MAX_TBOX; ++i) {
		pvaa->vatbox[i].dic = 0;
		pvaa->vatbox[i].age = 0;
	}
	pvaa->cnt_tbox = 0;

	dipage = pvaa->vameta[0].dipage;
	dit_clear_page(pvaa->ditid, dipage);
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _vaa_init_figure(FIGURE_INFO *info)
{
	memset(info, 0x00, sizeof(FIGURE_INFO));

	info->wi = 3;	// 3pixel
	info->st = 0;	// line
	info->vt = 0;	// vertices hide

	return 0;
}

int _vaa_init_image(IMAGE_INFO *info)
{
	memset(info, 0x00, sizeof(IMAGE_INFO));

	return 0;
}

int _vaa_construct_figure(IGPOINT *pt, int cnt, FIGURE_INFO *info)
{
	int i;

	DMSG(9, "cnt = %d\n", cnt);
	info->cnt = cnt;
	for (i = 0; i < cnt; ++i) {
		info->pt[i].x = pt[i].x;
		info->pt[i].y = pt[i].y;
		info->pt[i].opt = pt[i].opt;
	}

#if 0
	if (IS_LINE_FIGURE(info)) {
		info->ar = 1;
	}
#endif

	return 0;
}

int _vaa_construct_direction(VARULEPTR prule, IMAGE_INFO *info)
{
	IGPOINT dst;
	char *img;

    memset(&dst, 0x00, sizeof(IGPOINT));
    
	_get_dir_pos(prule, &dst);
	img = _get_dir_img(prule);

	info->pt = dst;
	info->img.ptr = img;
	return 0;
}

int _vaa_construct_figure_rect(IGRECT *rect, FIGURE_INFO *info)
{
	info->cnt = 4;
	info->pt[0].x = rect->x;
	info->pt[0].y = rect->y;

	info->pt[1].x = rect->x + rect->w;
	info->pt[1].y = rect->y;

	info->pt[2].x = rect->x + rect->w;
	info->pt[2].y = rect->y + rect->h;

	info->pt[3].x = rect->x;
	info->pt[3].y = rect->y + rect->h;

/*	DMSG(9, "pt: [%d,%d], [%d,%d], [%d,%d], [%d,%d]", 
		info->pt[0].x, info->pt[0].y,
		info->pt[1].x, info->pt[1].y,
		info->pt[2].x, info->pt[2].y,
		info->pt[3].x, info->pt[3].y
		);*/
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int vaa_s1_init(int ch, VAAID *pvaaid, DITID *pditid)
{
	S1VAA_T *pvaa;
	*pvaaid = vaa_s1_create(ch);
	*pditid = dit_create(10, 3840, 2160);

	pvaa = *pvaaid;
	_link_dit(pvaa, *pditid);

	_load_db(pvaa);
	_translate_vcdata_into_rule(pvaa, &pvaa->db);
	_copy_prop_to_rule(&pvaa->db, &pvaa->prop);
	_control_dit(pvaa);

	return 0;
}

VAAID vaa_s1_create(int ch)
{
	VAAID vaaid;
	S1VAA_T *pvaa;

	vaaid = (VAAID)imalloc(sizeof(S1VAA_T));
	pvaa = vaaid;
	_init(pvaa, ch);

	return vaaid;
}

int vaa_s1_destroy(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	ifree(pvaa);
	return 0;
}

int vaa_s1_reload(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	_load_db(pvaa);
	_translate_vcdata_into_rule(pvaa, &pvaa->db);
	_copy_prop_to_rule(&pvaa->db, &pvaa->prop);
	_control_dit(pvaa);
	return 0;
}

int vaa_s1_disable(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	pvaa->prop.active = FALSE;
	dit_disable(pvaa->ditid);
	return 0;
}

int vaa_s1_enable(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	pvaa->prop.active = TRUE;
	dit_enable(pvaa->ditid);
	return 0;
}

int vaa_s1_is_enabled(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	return (pvaa->prop.active == TRUE);
}

int vaa_s1_raiseup(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	pvaa->evt_linked = 1;
	_vaa_s1_link_event(id);	
//	vaa_s1_activate_all_pattern(id);	
	return 0;
}

int vaa_s1_link_dit(VAAID id, DITID ditid)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	_link_dit(pvaa, ditid);

	DMSG(9, "ditid = %p\n", pvaa->ditid);
	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_unlink_dit(VAAID id, DITID ditid)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	dit_disable_all_page(pvaa->ditid);
	pvaa->ditid = 0;	
	return 0;
}

int vaa_s1_is_dit_linked(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	return (pvaa->ditid != 0);
}

DITID vaa_s1_get_dit(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	return pvaa->ditid;
}

int vaa_s1_activate_pattern(VAAID id, PATTERN_E patt)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	dit_enable_page(pvaa->ditid, pvaa->vapatt[patt].dipage);

	pvaa->evt_linked_patt = patt;
	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_activate_all_pattern(VAAID id)
{
	int i;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	for (i = 0; i < PATTERN_MAX; ++i)
		dit_enable_page(pvaa->ditid, pvaa->vapatt[i].dipage);

	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_switch_pattern(VAAID id, PATTERN_E patt)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;
	if (!vaa_s1_is_dit_linked(id)) return -1;

	vaa_s1_deactivate_all_pattern(id);
	dit_enable_page(pvaa->ditid, pvaa->vapatt[patt].dipage);

	pvaa->evt_linked_patt = patt;
	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_deactivate_pattern(VAAID id, PATTERN_E patt)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	dit_disable_page(pvaa->ditid, pvaa->vapatt[patt].dipage);

	// To be fixed
	pvaa->evt_linked_patt = 0;
	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_deactivate_all_pattern(VAAID id)
{
	int i;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	for (i = 0; i < PATTERN_MAX; ++i)
		dit_disable_page(pvaa->ditid, pvaa->vapatt[i].dipage);

	// To be fixed
	pvaa->evt_linked_patt = 0;
	_redraw_patterns(pvaa);
	return 0;
}

int vaa_s1_activate_meta(VAAID id, VAA_META_E meta)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	dit_enable_page(pvaa->ditid, pvaa->vameta[meta].dipage);

	return 0;
}

int vaa_s1_activate_all_meta(VAAID id)
{
	int i;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	for (i = 0; i < MAX_META; ++i)
		dit_enable_page(pvaa->ditid, pvaa->vameta[i].dipage);

	return 0;
}

int vaa_s1_deactivate_meta(VAAID id, VAA_META_E meta)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;


	if (!vaa_s1_is_dit_linked(id)) return -1;
	dit_disable_page(pvaa->ditid, pvaa->vameta[meta].dipage);

	return 0;
}

int vaa_s1_deactivate_all_meta(VAAID id)
{
	int i;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	for (i = 0; i < MAX_META; ++i)
		dit_disable_page(pvaa->ditid, pvaa->vameta[i].dipage);

	return 0;
}

int vaa_s1_deactivate_all(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (!vaa_s1_is_dit_linked(id)) return -1;
	
	vaa_s1_deactivate_all_pattern(id);
	vaa_s1_deactivate_all_meta(id);

	return 0;
}

/*
PATTERN_E vaa_s1_get_active_pattern(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return 0;

	return pvaa->ap;
}
*/

int vaa_s1_load_db(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	_init(pvaa, pvaa->ch);
	_load_db(pvaa);
	_translate_vcdata_into_rule(pvaa, &pvaa->db);
	_copy_prop_to_rule(&pvaa->db, &pvaa->prop);
	_control_dit(pvaa);

	return 0;
}

int vaa_s1_save_db(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	_copy_prop_to_db(&pvaa->prop, &pvaa->db);
	_translate_rule_into_vcdata(pvaa, &pvaa->db);
	_save_db(pvaa);

	return 0;
}

int vaa_s1_is_active_changed(VAAID id)
{
	VCAData tmpdb;	
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return 0;

//_print_db(pvaa);
	memset(&tmpdb, 0x00, sizeof(VCAData));
	memmove(&tmpdb, &pvaa->db, sizeof(VCAData));
	_copy_prop_to_db(&pvaa->prop, &tmpdb);
//	DMSG(1, "%d", (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData))));

	if ((tmpdb.prop.active) && (!pvaa->db.prop.active)) return 1;
	return 0;
}

int vaa_s1_is_db_changed(VAAID id)
{
	VCAData tmpdb;	
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return 0;

//_print_db(pvaa);
	memset(&tmpdb, 0x00, sizeof(VCAData));
	memmove(&tmpdb, &pvaa->db, sizeof(VCAData));
	_copy_prop_to_db(&pvaa->prop, &tmpdb);
	_translate_rule_into_vcdata(pvaa, &tmpdb);	
//	DMSG(1, "%d", (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData))));
	return (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData)) != 0);
}

int vaa_s1_get_rule_count(VAAID id)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;


}

RULEID vaa_s1_find_rule(VAAID id, int x, int y)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	VARULEPTR prule;
	RULEID tmp_rule[16];
	int	tmp_area[16];
	int rcnt = 0;
	RULEID rule;
	IXSQUARE sq;
	int min = 0;
	int i;
	IXPOLYGON p;
	
	IGPOINT mpt;
	IGPOINT fpt[MAX_PT];
	int fcnt = 0;
	IGRECT dirpos;
	
	if (!pvaa) return -1;

	rcnt = 0;
	memset(&p, 0x00, sizeof(IXPOLYGON));
	memset(tmp_rule, 0x00, sizeof(tmp_rule));
	memset(tmp_area, 0x00, sizeof(tmp_area));
	memset(&sq, 0x00, sizeof(IGBOX));

	mpt.x = x;
	mpt.y = y;
	
	for (rule = 0; rule < pvaa->cnt_rule; rule++) {
		prule = _get_rule_ptr(pvaa, (RULEID)rule);
		if (!prule) continue;
		if (!prule->zn_data.active) continue;
//		if (pvaa->evt_linked_patt != prule->patt) continue;

		if (_get_figure(prule, fpt, &fcnt) == -1) continue;
		if (_is_point_in_figure(mpt, fpt, fcnt) == 0) {
			_make_polygon(fpt, fcnt, &p);
			if (ifn_point_is_over_line(mpt.x, mpt.y, 16, &p)) return rule;
			tmp_rule[rcnt] = rule;
			ifn_get_outbound_square(&p, &sq);
			tmp_area[rcnt] = ifn_get_area(&sq);
			rcnt++;
		}
		else {
		if (_IS_DIRCTRL(prule)) {
			_get_dirpos(prule, &dirpos);
				if (ifn_is_in_igrect(&dirpos, mpt.x, mpt.y) == 1) {
					tmp_rule[rcnt] = rule;
					_make_polygon(fpt, fcnt, &p);
					ifn_get_outbound_square(&p, &sq);
					tmp_area[rcnt] = ifn_get_area(&sq);
					rcnt++;
				}
			}
		}
	}

	min = 0x7fffffff;
	rule = -1;
	for (i = 0; i < rcnt; ++i) {
		if (min > tmp_area[i]) {
			rule = tmp_rule[i]; 
			min = tmp_area[i];
		}
	}

	return rule;
}


int vaa_s1_set_selecting_margin(VAAID id, int pixel)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;


}

int vaa_s1_get_rule_shape(VAAID id, RULEID rule, VARULE_SHAPE *shape)
{
	VARULEPTR prule;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	DMSG(9, "ruleid = %d\n", rule);
	prule = _get_rule_ptr(pvaa, rule);
	_get_figure(prule, shape->pt, &shape->ptcnt);
	DMSG(9, "");
	_get_dirpos(prule, &shape->dirpos);
	return 0;
}

int vaa_s1_set_rule_shape(VAAID id, RULEID rule, VARULE_SHAPE *shape)
{
	VARULEPTR prule;
	int ret = 0;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	if (shape->ptcnt > MAX_PT) return -1;

	prule = _get_rule_ptr(pvaa, rule);
	if (_IS_LINE(prule) && shape->ptcnt > 2) return -1;
	if (_IS_AREA(prule) && shape->ptcnt > MAX_PT_S1) return -2;
	if (_IS_AREA(prule) && shape->ptcnt < 3) return -1;

	_set_figure(prule, shape->pt, shape->ptcnt);
	if (_IS_DIRCTRL(prule)) _update_direction(prule);
	return 0;
}

int vaa_s1_get_rule_conf(VAAID id, RULEID rule, VARULE_CONF *conf)
{
	VARULEPTR prule;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	prule = _get_rule_ptr(pvaa, rule);
	_bring_conf(prule, conf);
	_bring_global_conf(prule->patt, pvaa, conf);

	return 0;
}

int vaa_s1_get_rule_confs(VAAID id, PATTERN_E page, VARULE_CONF conf[16], int *cnt)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	*cnt = 0;
	_bring_confs(pvaa, page, conf, cnt);
	return 0;
}

int vaa_s1_get_rule_confs_all(VAAID id, VARULE_CONF conf[16], int *cnt)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	*cnt = 0;
	if (!pvaa) return -1;

	_bring_confs_all(pvaa, conf, cnt);
	return 0;
}

int vaa_s1_set_rule_conf(VAAID id, RULEID rule, VARULE_CONF *conf)
{
	VARULEPTR prule;
	int change;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	prule = _get_rule_ptr(pvaa, rule);
	change = _apply_conf(conf, prule);
	_update_ruledb_count(pvaa, prule, change);
	_apply_global_conf(conf, pvaa);

	return 0;
}

int vaa_s1_set_rule_confs(VAAID id, PATTERN_E page, VARULE_CONF *conf, int cnt)
{
	VARULEPTR prule;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

//	prule = _get_rule_ptr(pvaa, rule);
//	_apply_confs(conf, cnt, pvaa);
	return 0;
}

int vaa_s1_remove_rule(VAAID id, RULEID rule)
{
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	// always return in S1
	return 0;
}

int vaa_s1_show_meta(VAAID id, gint cnt, ivcam_obj_t* data)
{
	int i;
	IGRECT rect;
	int	dipage;
	int evt;

	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	dipage = pvaa->vameta[0].dipage;
	_clear_meta(pvaa);

	if (cnt > MAX_TBOX) cnt = MAX_TBOX;

	for (i = 0; i < cnt; ++i) {
		_get_meta_bound(&data[i].mobj, &rect);
		evt = _is_evented_meta(&data[i].mobj);
		pvaa->vatbox[i].dic = _add_trackbox_to_display(pvaa, dipage, &rect, evt);
		pvaa->vatbox[i].age = SEC_2;
	}
	pvaa->cnt_tbox = cnt;

	return 0;
}

static RULEID _find_event_rule(S1VAA_T *pvaa, ivca_u32_t type, ivca_s16_t rule_id)
{
	int i;
	VCACntr	*pcntr;
	VCAZone	*pzone;

	if (type & IVCA_ET_COUNTER) {
		for (i = 0; i < pvaa->cnt_rule; ++i) {
			pcntr = &pvaa->varule[i].ct_data;
			if (pcntr->active && pcntr->id == rule_id) return i;
		}
	}
	else {
		for (i = 0; i < pvaa->cnt_rule; ++i) { 
			pzone = &pvaa->varule[i].zn_data;
			if (pzone->active && pzone->id == rule_id) return i;
		}
	}
	return -1;
}

int vaa_s1_show_event(VAAID id, gint cnt, ivca_rule_event_t* data)
{
	int i;
	RULEID rule;
	VARULEPTR prule;
	VARULE_CONF	conf;

	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	for (i = 0; i < cnt; ++i) {
		rule = _find_event_rule(pvaa, data[i].type, data[i].rule_id);
		if (rule != -1) {
			prule = _get_rule_ptr(pvaa, rule);

			_make_highlight(prule);
			prule->blink_step = 20;
			// remove blinking function by S1
			prule->blink_en = 0;
			if (prule->blink_step != -1) {
				if (prule->blink_en) {
					if (prule->blink_step % 2 == 0) {
						prule->dic[FIGURE]->en = 0;
						if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 0;
					}
					else {
						prule->dic[FIGURE]->en = 1;
						if (_IS_DIRCTRL(prule)) prule->dic[DIRCTRL]->en = 1;
					}
				}
			}


			DMSG(9, "event type = %x\n", data->type);
		}
	}

	return 0;
}

RULEID vaa_s1_get_counted_rule(VAAID id, unsigned short countid)
{
	int i;
	S1VAA_T *pvaa = (S1VAA_T *)id;
	if (!pvaa) return -1;

	for (i = 0; i < pvaa->cnt_rule; ++i)
		if (pvaa->varule[i].ct_data.id == countid) return i;

	return -1;
}

int	vaa_s1_parse_event(VAAID vaaid, ivca_rule_event_t *pevt, int *zid, int *cid)
{
	PATTERN_E ret = PATTERN_NONE;
	S1VAA_T *pvaa = (S1VAA_T *)vaaid;
	if (!pvaa) return PATTERN_NONE;;

	if (pevt->type & IVCA_ET_TAMPER) {
		ret = PATTERN_TAMPERING; 
		*zid = -1;
		*cid = -1;
	}
	else if (pevt->type & IVCA_ET_COUNTER) {
		ret = PATTERN_COUNT; 
		*zid = -1;
		*cid = _get_rule_id(pvaa, PATTERN_COUNT, pevt->rule_id);
	}
	else {
		switch (pevt->type & 0xFFF) {
		case IVCA_ET_ENTER: 
			ret = PATTERN_INVASION; 
			*zid = _get_rule_id(pvaa, PATTERN_INVASION, pevt->rule_id);
			*cid = -1;
			break;

		case IVCA_ET_STOPPED:
			ret = PATTERN_ABANDON; 
			*zid = _get_rule_id(pvaa, PATTERN_ABANDON, pevt->rule_id);
			*cid = -1;
			break;

		case IVCA_ET_LOITERED:
			ret = PATTERN_LOITERING; 
			*zid = _get_rule_id(pvaa, PATTERN_LOITERING, pevt->rule_id);
			*cid = -1;
			break;

		case IVCA_ET_FALL:
			ret = PATTERN_TOPPLE; 
			*zid = _get_rule_id(pvaa, PATTERN_TOPPLE, pevt->rule_id);
			*cid = -1;
			break;

		case IVCA_ET_REMOVED:
			ret = PATTERN_STEAL; 
			*zid = _get_rule_id(pvaa, PATTERN_STEAL, pevt->rule_id);
			*cid = -1;
			break;

		case IVCA_ET_DIR_POS:
		case IVCA_ET_DIR_NEG:
			ret = PATTERN_FENCE; 
			*zid = _get_rule_id(pvaa, PATTERN_FENCE, pevt->rule_id);
			*cid = -1;
			break;
		}
	}

	return (int)ret;
}

int vaa_s1_timer_proc(VAAID vaaid)
{
	S1VAA_T *pvaa = (S1VAA_T *)vaaid;
	if (!pvaa) return -1;
	if (!vaa_s1_is_enabled(vaaid)) return -1;

	_proc_blinking(pvaa);
	_proc_meta_erasing(pvaa);

	return 0;
}

int vaa_s1_make_blinking(VAAID vaaid, PATTERN_E patt)
{
	int i;
	VARULEPTR prule;
	S1VAA_T *pvaa = (S1VAA_T *)vaaid;
	if (!pvaa) return -1;

	for (i = 0; i < pvaa->cnt_rule; ++i) {
		prule = _get_rule_ptr(pvaa, (RULEID)i);
		if (patt != prule->patt) continue;
		if (_IS_NOFIGURE(prule)) continue;

		prule->blink_step = 20;
		prule->blink_en = 1;
		_make_highlight(prule);
	}

	return 0;
}


