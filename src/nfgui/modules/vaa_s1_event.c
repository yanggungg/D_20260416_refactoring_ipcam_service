/*
 * vaa_s1_event.c
 * 	- video analytics agent for s1
 *	- dependencies :
 *		viewer
 *		dit		
 *
 * Written by JUNGKYU PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, May 16, 2013
 *
 */


#include <math.h>
#include "nf_afx.h"
#include "ix_mem.h"
#include "nf_meta_data.h"
#include "vw_s1_vca_setup_main.h"

#include "vw_vca.h"
#include "vaa.h"
#include "vaa_s1.h"
#include "iux_afx.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		9
#define DBG_MODULE		"VAA_S1_EVENT"


////////////////////////////////////////////////////////////
//
// private data type 
//


typedef struct _MPOINT {
	int		x;
	int		y;
} MPOINT;

typedef struct _HL_RULE {
	RULEID 	ruleid;
	MPOINT	dmpt;	
} HL_RULE;


////////////////////////////////////////////////////////////
//
// private variable
//

static HL_RULE hl_rule;




////////////////////////////////////////////////////////////
//
// private functions
//

static gint _set_highlight_ruleid(RULEID ruleid)
{
	hl_rule.ruleid = ruleid;
	return 0;
}

static RULEID _get_highlight_ruleid()
{
	return hl_rule.ruleid;
}

static gint _get_highlight_point(IGPOINT *figure_pt, int figure_cnt)
{
	int i;
	for (i = 0; i < figure_cnt; ++i) 
		if (figure_pt[i].opt == 1) return i;

	return -1;
}

static gint _set_highlight_mpt_distance(gint mpt_x, gint mpt_y)
{
	gint i;

	hl_rule.dmpt.x = mpt_x;
	hl_rule.dmpt.y = mpt_y;

	return 0;
}

static gint _get_highlight_mpt_distance(MPOINT *dmpt)
{
	if (hl_rule.ruleid == -1) return -1;

	dmpt->x = hl_rule.dmpt.x;
	dmpt->y = hl_rule.dmpt.y;
	return 0;
}

static gint _trans_vpoint(gint vcnvs, gint rpoint, gint rcnvs)
{
	gint vpoint;	
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));
	return vpoint;
}

static gint _get_cnvs_size(VAAID vaaid, gint *cnvs_w, gint *cnvs_h)
{
	DITID ditid;
	
	ditid = vaa_s1_get_dit(vaaid);
	dit_get_cnvs_info(ditid, cnvs_w, cnvs_h);
	return 0;
}

static gint _find_figure_point(gint mpt_x, gint mpt_y, IGPOINT *figure_pt, gint figure_cnt, gint *point_index)
{
	gint i;

	for (i = 0; i < figure_cnt; i++)
	{
		if ((abs(figure_pt[i].x - mpt_x) < 24) && (abs(figure_pt[i].y - mpt_y) < 24))
		{
			*point_index = i;
			return 0;
		}
	}

	*point_index = -1;
	return -1;	
}

static gint _move_highlight_ruleid(gint mpt_x, gint mpt_y, MPOINT dmpt, IGPOINT *figure_pt, gint figure_cnt)
{
	gint i;

	for (i = 0; i < figure_cnt; i++)
	{
		figure_pt[i].x += (mpt_x - dmpt.x);
		figure_pt[i].y += (mpt_y - dmpt.y);
	}

	return 0;
}

static gint _move_highlight_point(gint mpt_x, gint mpt_y, gint pt_idx, IGPOINT *figure_pt, gint figure_cnt)
{
	if (pt_idx >= figure_cnt) return -1;

	figure_pt[pt_idx].x = mpt_x;
	figure_pt[pt_idx].y = mpt_y;
	return 0;
}

static gint _check_valid_point(gint cnvs_x, gint cnvs_y, gint cnvs_w, gint cnvs_h, IGPOINT *figure_pt, gint figure_cnt)
{
	gint i;

	for (i = 0; i < figure_cnt; i++)
	{
		if ((figure_pt[i].x < cnvs_x) || (figure_pt[i].y < cnvs_y)
			|| (figure_pt[i].x > cnvs_x+cnvs_w) || (figure_pt[i].y > cnvs_y+cnvs_h))
		{
			return -1;
		}
	}

	return 0;
}

static gint _add_point_closer_line(gint mpt_x, gint mpt_y, IGPOINT *figure_pt, gint figure_cnt, gint *add_pt)
{
	gint i, j = -1;
	IXPOINT pt1;
	IXPOINT pt2;
	
	/* Check if the point is near polygon points. */
	for (i = 0; i < figure_cnt; i++)
	{
		if ((abs(figure_pt[i].x - mpt_x) < 16) && (abs(figure_pt[i].y - mpt_y) < 16))
		{
			return -1;
		}
	}

	/* Find the line segment to add a point. */
	for (i = 0; i < figure_cnt; i++)
	{
		pt1.x = figure_pt[i].x;
		pt1.y = figure_pt[i].y;

		if ((figure_cnt > 2) && (i == figure_cnt-1))
		{
			pt2.x = figure_pt[0].x;
			pt2.y = figure_pt[0].y;
		}
		else
		{
			pt2.x = figure_pt[i+1].x;
			pt2.y = figure_pt[i+1].y;
		}
	
		if (ifn_point_is_closer_line(mpt_x, mpt_y, 16, &pt1, &pt2))
		{
			j = i+1;
			break;
		}
	}
			
	if (j == -1) return -1;

	for (i = 0; i < j; i++)
	{
		figure_pt[i].opt = 0;
	}

	for (i = figure_cnt; i > j; i--)
	{
		figure_pt[i].x = figure_pt[i-1].x;
		figure_pt[i].y = figure_pt[i-1].y;
		figure_pt[i].opt = 0;		
	}

	figure_pt[j].x = mpt_x;
	figure_pt[j].y = mpt_y;
	figure_pt[j].opt = 1;

	*add_pt = j;
	return 0;
}

static gint _add_point(gint add_pt_idx, IGPOINT *figure_pt, gint figure_cnt)
{
	gint i;
	IXPOINT new_pt;
	
	if (add_pt_idx >= figure_cnt-1)
	{
		new_pt.x = (figure_pt[add_pt_idx].x+figure_pt[0].x) >> 1;
		new_pt.y = (figure_pt[add_pt_idx].y+figure_pt[0].y) >> 1;		
	}
	else
	{
		new_pt.x = (figure_pt[add_pt_idx].x+figure_pt[add_pt_idx+1].x) >> 1;
		new_pt.y = (figure_pt[add_pt_idx].y+figure_pt[add_pt_idx+1].y) >> 1;		
	}

	/* Check if the point is near polygon points. */
	for (i = 0; i < figure_cnt; i++)
	{
		if ((abs(figure_pt[i].x - new_pt.x) < 16) && (abs(figure_pt[i].y - new_pt.y) < 16))
		{
			return -1;
		}
	}

	add_pt_idx += 1;

	for (i = 0; i < add_pt_idx; i++)
	{
		figure_pt[i].opt = 0;
	}

	for (i = figure_cnt; i > add_pt_idx; i--)
	{
		figure_pt[i].x = figure_pt[i-1].x;
		figure_pt[i].y = figure_pt[i-1].y;
		figure_pt[i].opt = 0;		
	}

	figure_pt[add_pt_idx].x = new_pt.x;
	figure_pt[add_pt_idx].y = new_pt.y;
	figure_pt[add_pt_idx].opt = 1;

	return 0;
}

static gint _del_point(gint del_pt_idx, IGPOINT *figure_pt, gint figure_cnt)
{
	gint i, cnt = 0;
	IGPOINT cp_pt[MAX_PT];

	memset(cp_pt, 0x00, sizeof(IGPOINT)*MAX_PT);

	for (i = 0; i < figure_cnt; i++)
	{
		if (del_pt_idx != i) 
		{
			cp_pt[cnt].x = figure_pt[i].x;
			cp_pt[cnt].y = figure_pt[i].y;
			cp_pt[cnt].opt = 0;	
			cnt++;
		}
	}	

	memcpy(figure_pt, cp_pt, sizeof(IGPOINT)*(figure_cnt-1));
	return 0;
}

static int _make_highlight(VAAID vaaid, RULEID ruleid)
{
	VARULE_CONF conf;

	memset(&conf, 0x00, sizeof(VARULE_CONF));
	vaa_s1_get_rule_conf(vaaid, ruleid, &conf);
	conf.sty_highlight = 1;
	vaa_s1_set_rule_conf(vaaid, ruleid, &conf);

	return 0;
}

static int _make_lowlight(VAAID vaaid, RULEID ruleid)
{
	VARULE_CONF conf;

	memset(&conf, 0x00, sizeof(VARULE_CONF));
	vaa_s1_get_rule_conf(vaaid, ruleid, &conf);
	conf.sty_highlight = 0;
	vaa_s1_set_rule_conf(vaaid, ruleid, &conf);

	return 0;
}

static int _change_direction(VAAID vaaid, RULEID ruleid)
{
	VARULE_CONF conf;
	int i;
	BITMASK cur = 0;

	memset(&conf, 0x00, sizeof(VARULE_CONF));
	vaa_s1_get_rule_conf(vaaid, ruleid, &conf);

	cur = 1 << conf.cfg_dir;
	for (i = 0; i < 4; ++i) {
		cur = cur == 0x08 ? 1 : cur << 1;
		if (conf.cap_dirctrl & cur) break;
	}

	for (i = 0; i < 4; ++i) {
		if (cur & 1 << i) break;
	}
	conf.cfg_dir = i;
	vaa_s1_set_rule_conf(vaaid, ruleid, &conf);

	return 0;
}

static int _is_in_dirctrl(IGRECT *dirpos, int x, int y)
{
	return ifn_is_in_igrect(dirpos, x, y);
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _vaa_s1_link_event(VAAID id)
{
	memset(&hl_rule, 0x00, sizeof(HL_RULE));
	hl_rule.ruleid = -1;

	VW_S1_VCA_attach_mevent(VCA_MEVENT_LEFT_PRESS, vaa_s1_event_mouse_left_press, id);
//	VW_S1_VCA_attach_mevent(VCA_MEVENT_LEFT_2PRESS, vaa_s1_event_mouse_left_2press, id);
	VW_S1_VCA_attach_mevent(VCA_MEVENT_RIGHT_PRESS, vaa_s1_event_mouse_right_press, id);
//	VW_S1_VCA_attach_mevent(VCA_MEVENT_RIGHT_2PRESS, vaa_s1_event_mouse_right_2press, id);
	VW_S1_VCA_attach_mevent(VCA_MEVENT_RELEASE, vaa_s1_event_mouse_release, id);
	VW_S1_VCA_attach_mevent(VCA_MEVENT_DRAG, vaa_s1_event_mouse_left_drag, id);	
	VW_S1_VCA_attach_mevent(VCA_MEVENT_ADD_POINT, vaa_s1_event_mouse_add_point, id);	
	VW_S1_VCA_attach_mevent(VCA_MEVENT_DEL_POINT, vaa_s1_event_mouse_del_point, id);		
	VW_S1_VCA_attach_select_ruleid(vaa_s1_event_select_ruleid, id);	
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vaa_s1_event_mouse_left_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID pre_ruleid;
	RULEID find_ruleid;
	VARULE_SHAPE sh;
		
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint pt_idx;
	int i;
	int is_dir;

	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	pre_ruleid = _get_highlight_ruleid();

	if (pre_ruleid != -1) 
	{
		_make_lowlight(vaaid, pre_ruleid);

		vaa_s1_get_rule_shape(vaaid, pre_ruleid, &sh);
		for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;
		vaa_s1_set_rule_shape(vaaid, pre_ruleid, &sh);
		_set_highlight_ruleid(-1);
		_set_highlight_mpt_distance(0, 0);
	}

	find_ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);
	evt_send_to_local(INFY_VAA_SELECT_RULE_ID, find_ruleid, 0, 0);

	if (find_ruleid == -1) 
	{
		_set_highlight_ruleid(-1);
		return -1;
	}
	
	_make_highlight(vaaid, find_ruleid);
	
	vaa_s1_get_rule_shape(vaaid, find_ruleid, &sh);

	for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;

	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &pt_idx) == 0)
	{
		sh.pt[pt_idx].opt = 1;
	}

	vaa_s1_set_rule_shape(vaaid, find_ruleid, &sh);

	is_dir = _is_in_dirctrl(&sh.dirpos, mpt_x, mpt_y);
	if (is_dir) _change_direction(vaaid, find_ruleid);

	_set_highlight_ruleid(find_ruleid);
	_set_highlight_mpt_distance(mpt_x, mpt_y);

	return 0;
}

gint vaa_s1_event_mouse_left_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint add_pt;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);	
	DMSG(9, "FIND RULE ID:%d", ruleid);	
	if (ruleid == -1) return -1;
	
	vaa_s1_get_rule_shape(vaaid, ruleid, &sh);
	if (sh.ptcnt+1 >= MAX_PT) return -1;
	if (_add_point_closer_line(mpt_x, mpt_y, sh.pt, sh.ptcnt, &add_pt) == -1) return -1;
	
	++sh.ptcnt;
	vaa_s1_set_rule_shape(vaaid, ruleid, &sh);
	_set_highlight_mpt_distance(mpt_x, mpt_y);
	return 0;
}

gint vaa_s1_event_mouse_right_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID pre_ruleid;
	RULEID find_ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint modify_pt_idx;
	int i;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	find_ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);
	if (find_ruleid == -1) return -1;

	vaa_s1_get_rule_shape(vaaid, find_ruleid, &sh);
	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &modify_pt_idx) == -1) return -1;

	pre_ruleid = _get_highlight_ruleid();

	if (pre_ruleid != -1) 
	{
		_make_lowlight(vaaid, pre_ruleid);

		vaa_s1_get_rule_shape(vaaid, pre_ruleid, &sh);
		for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;
		vaa_s1_set_rule_shape(vaaid, pre_ruleid, &sh);
	}

	evt_send_to_local(INFY_VAA_SELECT_RULE_ID, find_ruleid, 0, 0);

	_make_highlight(vaaid, find_ruleid);

	vaa_s1_get_rule_shape(vaaid, find_ruleid, &sh);
	for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;

	sh.pt[modify_pt_idx].opt = 1;
	vaa_s1_set_rule_shape(vaaid, find_ruleid, &sh);
	_set_highlight_ruleid(find_ruleid);
	_set_highlight_mpt_distance(mpt_x, mpt_y);

	if (sh.ptcnt >= 4)
		evt_send_to_local(INFY_VCA_SELECT_MODIFY_POINT, sh.ptcnt, 0, 0);
	return 0;
}

gint vaa_s1_event_mouse_right_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint pt_idx;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);
	if (ruleid == -1) return -1;
	
	vaa_s1_get_rule_shape(vaaid, ruleid, &sh);
	if (sh.ptcnt == 2) return -1;
	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &pt_idx) == -1) return -1;
	if (_del_point(pt_idx, sh.pt, sh.ptcnt) == -1) return -1;
	
	--sh.ptcnt;
	vaa_s1_set_rule_shape(vaaid, ruleid, &sh);
	_set_highlight_mpt_distance(0, 0);
	return 0;
}

gint vaa_s1_event_mouse_release(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{


	return 0;
}

gint vaa_s1_event_mouse_left_drag(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint pt_idx;

	MPOINT dmpt;
	gint valid = 0;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	ruleid = _get_highlight_ruleid();

	if (ruleid == -1) return -1;
	
	vaa_s1_get_rule_shape(vaaid, ruleid, &sh);
	pt_idx = _get_highlight_point(sh.pt, sh.ptcnt);

	if (pt_idx != -1)
	{
		_move_highlight_point(mpt_x, mpt_y, pt_idx, sh.pt, sh.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, sh.pt, sh.ptcnt);
	}
	else
	{
		_get_highlight_mpt_distance(&dmpt);
		_move_highlight_ruleid(mpt_x, mpt_y, dmpt, sh.pt, sh.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, sh.pt, sh.ptcnt);
		_set_highlight_mpt_distance(mpt_x, mpt_y);		
	}

	if (valid == 0) vaa_s1_set_rule_shape(vaaid, ruleid, &sh);

	return 0;
}

gint vaa_s1_event_mouse_add_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint pt_idx;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);	
	DMSG(9, "FIND RULE ID:%d", ruleid);	
	if (ruleid == -1) return -1;
	
	vaa_s1_get_rule_shape(vaaid, ruleid, &sh);
	if (sh.ptcnt >= 8) return -1;
	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &pt_idx) == -1) return -1;	
	if (_add_point(pt_idx, sh.pt, sh.ptcnt) == -1) return -1;
	
	++sh.ptcnt;
	vaa_s1_set_rule_shape(vaaid, ruleid, &sh);
	_set_highlight_mpt_distance(mpt_x, mpt_y);
	return 0;
}

gint vaa_s1_event_mouse_del_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	RULEID ruleid;
	VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint pt_idx;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	ruleid = vaa_s1_find_rule(vaaid, mpt_x, mpt_y);
	if (ruleid == -1) return -1;
	
	vaa_s1_get_rule_shape(vaaid, ruleid, &sh); 
	if (sh.ptcnt == 2) return -1;
	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &pt_idx) == -1) return -1;
	if (_del_point(pt_idx, sh.pt, sh.ptcnt) == -1) return -1;
	
	--sh.ptcnt;
	vaa_s1_set_rule_shape(vaaid, ruleid, &sh);
	_set_highlight_mpt_distance(0, 0);
	return 0;
}

gint vaa_s1_event_select_ruleid(int select_ruleid, gpointer user_data)
{
	VAAID vaaid;
	RULEID pre_ruleid;
	VARULE_SHAPE sh;
	int i;

	vaaid = (VAAID)user_data;

	pre_ruleid = _get_highlight_ruleid();

	if (pre_ruleid != -1) 
	{
		_make_lowlight(vaaid, pre_ruleid);

		vaa_s1_get_rule_shape(vaaid, pre_ruleid, &sh); 
		for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;
		vaa_s1_set_rule_shape(vaaid, pre_ruleid, &sh);
		_set_highlight_ruleid(-1);
		_set_highlight_mpt_distance(0, 0);
	}

	if (select_ruleid == -1) return -1;

	_make_highlight(vaaid, select_ruleid);
	
	vaa_s1_get_rule_shape(vaaid, select_ruleid, &sh);
	for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;

	vaa_s1_set_rule_shape(vaaid, select_ruleid, &sh);
	_set_highlight_ruleid(select_ruleid);
	_set_highlight_mpt_distance(0, 0);

	return 0;
}

int vaa_s1_notify_vca_event(NF_NOTIFY_INFO *data)
{
	ivca_rule_event_t *pevt;
	gint *p;

	VAAID vaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
//	printf("SKSHIN] %d\n", p[0]);
//	printf("SKSHIN] %d\n", p[1]);
	if (p[0] >= 16) return -1;

	vaaid = vaa_get_vaaid(p[0]);
	vaa_s1_show_event(vaaid, p[1], pevt);
	return 0;
}

int vaa_s1_notify_vca_track_info(NF_NOTIFY_INFO *data)
{
	ivcam_obj_t *pevt;
	gint *p;

	VAAID vaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
//	printf("SKSHIN] %d\n", p[0]);
//	printf("SKSHIN] %d\n", p[1]);
	if (p[0] >= 16) return -1;

	vaaid = vaa_get_vaaid(p[0]);
	vaa_s1_show_meta(vaaid, p[1], pevt);
	return 0;
}

int vaa_s1_notify_vca_meta_data(NF_NOTIFY_INFO *data)
{

	return 0;
}

int vaa_s1_notify_vca_counter(NF_NOTIFY_INFO *data)
{

	return 0;
}

