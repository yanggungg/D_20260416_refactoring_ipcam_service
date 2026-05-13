/*
 * vaa_itx_event.c
 * 	- video analytics agent for itx
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
#include "vaa_itx.h"
#include "iux_afx.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		9
#define DBG_MODULE		"VAA_ITX_EVENT"


////////////////////////////////////////////////////////////
//
// private data type 
//


typedef struct _VAAE_POINT {
	int		        x;
	int		        y;
} VAAE_POINT;

typedef struct _VAA_VMAP {
    int             use;
    int             x;
    int             y;
    int             w;
    int             h;    
} VAAE_VMAP;



////////////////////////////////////////////////////////////
//
// private variable
//

static VAAE_POINT g_last_mpt;
static VAAE_VMAP g_vmap;




////////////////////////////////////////////////////////////
//
// private functions
//

static ITX_ZONEID _get_highlight_zoneid(VAAID vaaid)
{
	ITX_VAZONE_CONF conf;
    int i;

    for (i = 0; i < 16; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));
    	vaa_itx_get_zone_conf(vaaid, i, &conf);

    	if (conf.use_zone && conf.focus) return i;
    }

	return -1;
}

static ITX_CNTRID _get_highlight_cntrid(VAAID vaaid)
{
	ITX_VACNTR_CONF conf;
    int i;

    for (i = 0; i < 16; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));
    	vaa_itx_get_cntr_conf(vaaid, i, &conf);

    	if (conf.use_cntr && conf.focus) return i;
    }

	return -1;
}

static ITX_CALBID _get_highlight_calbid(VAAID vaaid)
{
	ITX_VACALB_CONF conf;
    int i;

    for (i = 0; i < 32; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_VACALB_CONF));
    	vaa_itx_get_calb_conf(vaaid, i, &conf);

    	if (conf.use_calb && conf.focus) return i;
    }

	return -1;
}

static int _get_last_mpt(VAAE_POINT *dmpt)
{
	dmpt->x = g_last_mpt.x;
	dmpt->y = g_last_mpt.y;
	return 0;
}

static int _set_last_mpt(gint mpt_x, gint mpt_y)
{
	g_last_mpt.x = mpt_x;
	g_last_mpt.y = mpt_y;
	return 0;
}

static gint _get_highlight_point(IGPOINT *figure_pt, int figure_cnt)
{
	int i;
	for (i = 0; i < figure_cnt; ++i) 
		if (figure_pt[i].opt == 1) return i;

	return -1;
}

static int _trans_x_vpoint(gint vcnvs, int rpoint, int rcnvs)
{
	gint vpoint;	
	
    if (g_vmap.use) rpoint = rpoint*g_vmap.w/rcnvs + g_vmap.x;
    
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));	
	return vpoint;
}

static int _trans_y_vpoint(gint vcnvs, int rpoint, int rcnvs)
{
	gint vpoint;	
	
    if (g_vmap.use) rpoint = rpoint*g_vmap.h/rcnvs + g_vmap.y;
    
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));	
	return vpoint;
}

static int _get_cnvs_size(VAAID vaaid, int *cnvs_w, int *cnvs_h)
{
	DITID ditid;
	
	ditid = vaa_itx_get_dit(vaaid);
	dit_get_cnvs_info(ditid, cnvs_w, cnvs_h);
	return 0;
}

static int _find_figure_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int figure_cnt, int *point_index)
{
	int i;

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

static int _move_highlight_mpt(int mpt_x, int mpt_y, VAAE_POINT dmpt, IGPOINT *figure_pt, int figure_cnt)
{
	int i;

	for (i = 0; i < figure_cnt; i++)
	{
		figure_pt[i].x += (mpt_x - dmpt.x);
		figure_pt[i].y += (mpt_y - dmpt.y);
	}

	return 0;
}

static gint _move_highlight_point(int mpt_x, int mpt_y, int pt_idx, IGPOINT *figure_pt, int figure_cnt)
{
	if (pt_idx >= figure_cnt) return -1;

	figure_pt[pt_idx].x = mpt_x;
	figure_pt[pt_idx].y = mpt_y;
	return 0;
}

static int _check_valid_point(int cnvs_x, int cnvs_y, int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt)
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

static int _add_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
{
	int i, j = -1;
	IXPOINT pt1;
	IXPOINT pt2;

	/* Check if the point is near polygon points. */
	for (i = 0; i < *figure_cnt; i++)
	{
		if ((abs(figure_pt[i].x - mpt_x) < 16) && (abs(figure_pt[i].y - mpt_y) < 16))
		{
			return -1;
		}
	}

	/* Find the line segment to add a point. */
	for (i = 0; i < *figure_cnt; i++)
	{
		pt1.x = figure_pt[i].x;
		pt1.y = figure_pt[i].y;

		if ((*figure_cnt > 2) && (i == *figure_cnt-1))
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

			if (*figure_cnt+1 > MAX_PT) return 0;
			break;
		}
	}
			
	if (j == -1) return -1;

	for (i = 0; i < j; i++)
	{
		figure_pt[i].opt = 0;
	}

	for (i = *figure_cnt; i > j; i--)
	{
		figure_pt[i].x = figure_pt[i-1].x;
		figure_pt[i].y = figure_pt[i-1].y;
		figure_pt[i].opt = 0;		
	}

	figure_pt[j].x = mpt_x;
	figure_pt[j].y = mpt_y;
	figure_pt[j].opt = 1;

    *figure_cnt += 1;

	return 0;
}

static int _add_point_next_pt(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
{
	int i, add_pt_idx;
	IXPOINT new_pt;

	if (_find_figure_point(mpt_x, mpt_y, figure_pt, *figure_cnt, &add_pt_idx) == -1) return -1;	
	
	if (add_pt_idx >= *figure_cnt-1)
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
	for (i = 0; i < *figure_cnt; i++)
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

	for (i = *figure_cnt; i > add_pt_idx; i--)
	{
		figure_pt[i].x = figure_pt[i-1].x;
		figure_pt[i].y = figure_pt[i-1].y;
		figure_pt[i].opt = 0;		
	}

	figure_pt[add_pt_idx].x = new_pt.x;
	figure_pt[add_pt_idx].y = new_pt.y;
	figure_pt[add_pt_idx].opt = 1;

    *figure_cnt += 1;

	return 0;
}

static int _del_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
{
	int i, cnt = 0;
	int del_pt_idx;
	IGPOINT cp_pt[MAX_PT];
	
	if (_find_figure_point(mpt_x, mpt_y, figure_pt, *figure_cnt, &del_pt_idx) == -1) return -1;

	memset(cp_pt, 0x00, sizeof(IGPOINT)*MAX_PT);

	for (i = 0; i < *figure_cnt; i++)
	{
		if (del_pt_idx != i) 
		{
			cp_pt[cnt].x = figure_pt[i].x;
			cp_pt[cnt].y = figure_pt[i].y;
			cp_pt[cnt].opt = 0;	
			cnt++;
		}
	}	

    *figure_cnt -= 1;
	memcpy(figure_pt, cp_pt, sizeof(IGPOINT)*(*figure_cnt));
	
	return 0;
}

static int _set_zone_highlight_shape(VAAID vaaid, ITX_ZONEID zoneid, int mpt_x, int mpt_y)
{
	ITX_VAZONE_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_VAZONE_SHAPE));
	vaa_itx_get_zone_shape(vaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	if (_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0)
	{
		shape.pt[pt_idx].opt = 1;
	}

	vaa_itx_set_zone_shape(vaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_lowlight_shape(VAAID vaaid, ITX_ZONEID zoneid)
{
	ITX_VAZONE_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_VAZONE_SHAPE));
	vaa_itx_get_zone_shape(vaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	vaa_itx_set_zone_shape(vaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_highlight_conf(VAAID vaaid, ITX_ZONEID zoneid)
{
	ITX_VAZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	conf.focus = 1;
	vaa_itx_set_zone_conf(vaaid, zoneid, &conf);
	return 0;
}

static int _set_zone_lowlight_conf(VAAID vaaid, ITX_ZONEID zoneid)
{
	ITX_VAZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	conf.focus = 0;
	vaa_itx_set_zone_conf(vaaid, zoneid, &conf);
	return 0;
}

static int _set_cntr_highlight_shape(VAAID vaaid, ITX_CNTRID cntrid, int mpt_x, int mpt_y)
{
	ITX_VACNTR_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_VACNTR_SHAPE));
	vaa_itx_get_cntr_shape(vaaid, cntrid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	if (_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0)
	{
		shape.pt[pt_idx].opt = 1;
	}

	vaa_itx_set_cntr_shape(vaaid, cntrid, &shape);
	return 0;
}

static int _set_cntr_lowlight_shape(VAAID vaaid, ITX_CNTRID cntrid)
{
	ITX_VACNTR_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_VACNTR_SHAPE));
	vaa_itx_get_cntr_shape(vaaid, cntrid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	vaa_itx_set_cntr_shape(vaaid, cntrid, &shape);
	return 0;
}

static int _set_cntr_highlight_conf(VAAID vaaid, ITX_CNTRID cntrid)
{
	ITX_VACNTR_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));
	vaa_itx_get_cntr_conf(vaaid, cntrid, &conf);
	conf.focus = 1;
	vaa_itx_set_cntr_conf(vaaid, cntrid, &conf);
	return 0;
}

static int _set_cntr_lowlight_conf(VAAID vaaid, ITX_CNTRID cntrid)
{
	ITX_VACNTR_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));
	vaa_itx_get_cntr_conf(vaaid, cntrid, &conf);
	conf.focus = 0;
	vaa_itx_set_cntr_conf(vaaid, cntrid, &conf);
	return 0;
}

static int _set_calb_highlight_shape(VAAID vaaid, ITX_CALBID calbid, int mpt_x, int mpt_y)
{
	ITX_VACALB_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_VACALB_SHAPE));
	vaa_itx_get_calb_shape(vaaid, calbid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	if (_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0)
	{
		shape.pt[pt_idx].opt = 1;
	}

	vaa_itx_set_calb_shape(vaaid, calbid, &shape);
	return 0;
}

static int _set_calb_lowlight_shape(VAAID vaaid, ITX_CALBID calbid)
{
	ITX_VACALB_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_VACALB_SHAPE));
	vaa_itx_get_calb_shape(vaaid, calbid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) 
    {
        shape.pt[i].opt = 0;
    }

	vaa_itx_set_calb_shape(vaaid, calbid, &shape);
	return 0;
}

static int _set_calb_highlight_conf(VAAID vaaid, ITX_CALBID calbid)
{
	ITX_VACALB_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VACALB_CONF));
	vaa_itx_get_calb_conf(vaaid, calbid, &conf);
	conf.focus = 1;
	vaa_itx_set_calb_conf(vaaid, calbid, &conf);
	return 0;
}

static int _set_calb_lowlight_conf(VAAID vaaid, ITX_CALBID calbid)
{
	ITX_VACALB_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_VACALB_CONF));
	vaa_itx_get_calb_conf(vaaid, calbid, &conf);
	conf.focus = 0;
	vaa_itx_set_calb_conf(vaaid, calbid, &conf);
	return 0;
}

static int _proc_zone_mouse_left_press(VAAID vaaid, int mpt_x, int mpt_y)
{
	ITX_ZONEID zoneid;
	ITX_VAZONE_CONF conf;
	ITX_VAZONE_SHAPE shape;
	int i, pt_idx;

	zoneid = vaa_itx_find_zone(vaaid, mpt_x, mpt_y);
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));	
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	if (!conf.active) return -1;

	_set_zone_highlight_shape(vaaid, zoneid, mpt_x, mpt_y);
	_set_zone_highlight_conf(vaaid, zoneid);	
	_set_last_mpt(mpt_x, mpt_y);

	evt_send_to_local(INFY_VAA_ITX_PRESS_ZONE_ID, zoneid, 0, 0);	
	
	return 0;
}

static int _proc_cntr_mouse_left_press(VAAID vaaid, int mpt_x, int mpt_y)
{
	ITX_CNTRID cntrid;
	ITX_VACNTR_CONF conf;
	ITX_VACNTR_SHAPE shape;
	int i, pt_idx;
		
	cntrid = vaa_itx_find_cntr(vaaid, mpt_x, mpt_y);
	if (cntrid == -1) return -1;
		
	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));	
	vaa_itx_get_cntr_conf(vaaid, cntrid, &conf);
	if (!conf.active) return -1;

	_set_cntr_highlight_conf(vaaid, cntrid);	
	_set_last_mpt(mpt_x, mpt_y);

	evt_send_to_local(INFY_VAA_ITX_PRESS_CNTR_ID, cntrid, 0, 0);

	return 0;
}

static int _proc_calb_mouse_left_press(VAAID vaaid, int mpt_x, int mpt_y)
{
	ITX_CALBID calbid;
	ITX_VACALB_SHAPE shape;
	int i, pt_idx;
		
	calbid = vaa_itx_find_calb(vaaid, mpt_x, mpt_y);
	if (calbid == -1) return -1;
		    
	_set_calb_highlight_shape(vaaid, calbid, mpt_x, mpt_y);
	_set_calb_highlight_conf(vaaid, calbid);	
	_set_last_mpt(mpt_x, mpt_y);

	evt_send_to_local(INFY_VAA_ITX_PRESS_CALB_ID, calbid, 0, 0);

	return 0;
}

static int _proc_zone_mouse_left_2press(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_ZONEID zoneid;
	ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

	zoneid = vaa_itx_find_zone(vaaid, mpt_x, mpt_y);	
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));	
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	if (!conf.active) return -1;

	vaa_itx_get_zone_shape(vaaid, zoneid, &shape);

	if (shape.ptcnt == 2) 
	{
    	evt_send_to_local(INFY_VAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }
    
	if (_add_point(mpt_x, mpt_y, shape.pt, &shape.ptcnt) == -1) 
	{
    	evt_send_to_local(INFY_VAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }

    if (shape.ptcnt > MAX_PT) return -1;
        
   	vaa_itx_set_zone_shape(vaaid, zoneid, &shape);
  	_set_last_mpt(mpt_x, mpt_y);	

    return 0;
}

static int _proc_cntr_mouse_left_2press(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_CNTRID cntrid;
	ITX_VACNTR_CONF conf;

	cntrid = vaa_itx_find_cntr(vaaid, mpt_x, mpt_y);	
	if (cntrid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));	
	vaa_itx_get_cntr_conf(vaaid, cntrid, &conf);
	if (!conf.active) return -1;

	evt_send_to_local(INFY_VAA_ITX_2PRESS_CNTR_ID, cntrid, 0, 0);
    return 0;
}

static int _proc_zone_mouse_right_2press(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_ZONEID zoneid;
	ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

	zoneid = vaa_itx_find_zone(vaaid, mpt_x, mpt_y);	
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));	
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	if (!conf.active) return -1;

	vaa_itx_get_zone_shape(vaaid, zoneid, &shape);
	if (shape.ptcnt == 2) return -1;
	if (shape.ptcnt == 3) return -1;
	if (_del_point(mpt_x, mpt_y, shape.pt, &shape.ptcnt) == -1) return -1;
	
	vaa_itx_set_zone_shape(vaaid, zoneid, &shape);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_zone_mouse_drag(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_ZONEID zoneid;
	ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;
    VAAE_POINT dmpt;
	int cnvs_w, cnvs_h;
    int pt_idx;
	int valid;
    
	zoneid = _get_highlight_zoneid(vaaid);
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));	
	vaa_itx_get_zone_conf(vaaid, zoneid, &conf);
	if (!conf.active) return -1;

    _get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);
	
	vaa_itx_get_zone_shape(vaaid, zoneid, &shape);
	pt_idx = _get_highlight_point(shape.pt, shape.ptcnt);

	if (pt_idx != -1)
	{
		_move_highlight_point(mpt_x, mpt_y, pt_idx, shape.pt, shape.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, shape.pt, shape.ptcnt);
	}
	else
	{
		_get_last_mpt(&dmpt);		
		_move_highlight_mpt(mpt_x, mpt_y, dmpt, shape.pt, shape.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, shape.pt, shape.ptcnt);		
		_set_last_mpt(mpt_x, mpt_y);		
	}

    if (valid == -1) return -1;

	vaa_itx_set_zone_shape(vaaid, zoneid, &shape);
    return 0;
}

static int _proc_cntr_mouse_drag(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_CNTRID cntrid;
	ITX_VACNTR_CONF conf;
    ITX_VACNTR_SHAPE shape;
    VAAE_POINT dmpt;
	int cnvs_w, cnvs_h;    
    int pt_idx = -1;
	int valid;
    
	cntrid = _get_highlight_cntrid(vaaid);
	if (cntrid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));	
	vaa_itx_get_cntr_conf(vaaid, cntrid, &conf);
	if (!conf.active) return -1;

    _get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);
	
	vaa_itx_get_cntr_shape(vaaid, cntrid, &shape);

	_get_last_mpt(&dmpt);
	_move_highlight_mpt(mpt_x, mpt_y, dmpt, shape.pt, shape.ptcnt);
	valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, shape.pt, shape.ptcnt);
	_set_last_mpt(mpt_x, mpt_y);		
		
    if (valid == -1) return -1;

	vaa_itx_set_cntr_shape(vaaid, cntrid, &shape);
    return 0;
}

static int _proc_calb_mouse_drag(VAAID vaaid, int mpt_x, int mpt_y)
{
    ITX_CALBID calbid;
    ITX_VACALB_SHAPE shape;
    VAAE_POINT dmpt;
	int cnvs_w, cnvs_h;    
    int pt_idx = -1;
	int valid;
    
	calbid = _get_highlight_calbid(vaaid);
	if (calbid == -1) return -1;

    _get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);
	
	vaa_itx_get_calb_shape(vaaid, calbid, &shape);
	pt_idx = _get_highlight_point(shape.pt, shape.ptcnt);

	if (pt_idx != -1)
	{
		_move_highlight_point(mpt_x, mpt_y, pt_idx, shape.pt, shape.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, shape.pt, shape.ptcnt);
	}
	else
	{
		_get_last_mpt(&dmpt);		
		_move_highlight_mpt(mpt_x, mpt_y, dmpt, shape.pt, shape.ptcnt);
		valid = _check_valid_point(0, 0, cnvs_w, cnvs_h, shape.pt, shape.ptcnt);		
		_set_last_mpt(mpt_x, mpt_y);		
	}
		
    if (valid == -1) return -1;

	vaa_itx_set_calb_shape(vaaid, calbid, &shape);
    return 0;
}

static int _proc_all_rule_lowlight(VAAID vaaid)
{
	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;
	ITX_CALBID calbid;

	zoneid = _get_highlight_zoneid(vaaid);

	if (zoneid != -1) 
	{
        _set_zone_lowlight_shape(vaaid, zoneid);
        _set_zone_lowlight_conf(vaaid, zoneid);
	}

	cntrid = _get_highlight_cntrid(vaaid);

	if (cntrid != -1) 
	{
        _set_cntr_lowlight_shape(vaaid, cntrid);
        _set_cntr_lowlight_conf(vaaid, cntrid);
	}

	calbid = _get_highlight_calbid(vaaid);

	if (calbid != -1) 
	{
        _set_calb_lowlight_shape(vaaid, calbid);
        _set_calb_lowlight_conf(vaaid, calbid);
	}
	
    return 0;
}

static int _proc_select_zone_highlight(VAAID vaaid, ITX_ZONEID zoneid)
{
    _set_zone_highlight_shape(vaaid, zoneid, 0, 0);
    _set_zone_highlight_conf(vaaid, zoneid);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_select_cntr_highlight(VAAID vaaid, ITX_CNTRID cntrid)
{
//    _set_cntr_highlight_shape(vaaid, cntrid, 0, 0);
    _set_cntr_highlight_conf(vaaid, cntrid);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_select_calb_highlight(VAAID vaaid, ITX_CALBID calbid)
{
    _set_calb_highlight_shape(vaaid, calbid, 0, 0);
    _set_calb_highlight_conf(vaaid, calbid);
	_set_last_mpt(0, 0);
    return 0;
}




////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _vaa_itx_link_event(VAAID id)
{
    vw_itx_vca_rev_attach_mevent(VCA_MEVENT_LEFT_PRESS, vaa_itx_event_mouse_left_press, id);
	vw_itx_vca_rev_attach_mevent(VCA_MEVENT_RIGHT_PRESS, vaa_itx_event_mouse_right_press, id);
	vw_itx_vca_rev_attach_mevent(VCA_MEVENT_LEFT_2PRESS, vaa_itx_event_mouse_left_2press, id);
	vw_itx_vca_rev_attach_mevent(VCA_MEVENT_RIGHT_2PRESS, vaa_itx_event_mouse_right_2press, id);	
	vw_itx_vca_rev_attach_mevent(VCA_MEVENT_DRAG, vaa_itx_event_mouse_left_drag, id);	

    vw_vca_rev_calibration_zoom_attach_mevent(VCA_MEVENT_LEFT_PRESS, vaa_itx_event_mouse_left_press, id);
    vw_vca_rev_calibration_zoom_attach_mevent(VCA_MEVENT_RIGHT_PRESS, vaa_itx_event_mouse_right_press, id);
    vw_vca_rev_calibration_zoom_attach_mevent(VCA_MEVENT_LEFT_2PRESS, vaa_itx_event_mouse_left_2press, id);
    vw_vca_rev_calibration_zoom_attach_mevent(VCA_MEVENT_RIGHT_2PRESS, vaa_itx_event_mouse_right_2press, id);
    vw_vca_rev_calibration_zoom_attach_mevent(VCA_MEVENT_DRAG, vaa_itx_event_mouse_left_drag, id);    

    vw_vca_rev_calibration_zoom_set_vmap(vaa_itx_event_set_vmap, 0);
	
	vw_itx_vca_rev_attach_select_ruleid(vaa_itx_event_select_ruleid, id);	
	vw_itx_search_by_smart_rev_attach_select_ruleid(vaa_itx_event_select_ruleid, id);	
	
	return 0;
}




////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vaa_itx_event_set_vmap(VCA_VMAP_INFO *vmapInfo, gpointer user_data)
{
    g_vmap.use = vmapInfo->use;
    g_vmap.x = vmapInfo->x;
    g_vmap.y = vmapInfo->y;
    g_vmap.w = vmapInfo->w;
    g_vmap.h = vmapInfo->h;     
	return 0;
}

gint vaa_itx_event_mouse_left_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;

	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;
	ITX_CALBID calbid;	

	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	cntrid = _get_highlight_cntrid(vaaid);
	if (cntrid != -1) _set_cntr_lowlight_conf(vaaid, cntrid);
	
	zoneid = _get_highlight_zoneid(vaaid);
	if (zoneid != -1) _set_zone_lowlight_conf(vaaid, zoneid);

	calbid = _get_highlight_calbid(vaaid);
	if (calbid != -1) _set_calb_lowlight_conf(vaaid, calbid);
    
    ret = _proc_cntr_mouse_left_press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  
   
    ret = _proc_zone_mouse_left_press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

    ret = _proc_calb_mouse_left_press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

    evt_send_to_local(INFY_VAA_ITX_PRESS_NONE_ID, 0, 0, 0);
    
    _set_last_mpt(0, 0);        
	return -1;
}

gint vaa_itx_event_mouse_left_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);
    
    ret = _proc_cntr_mouse_left_2press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  
    
    ret = _proc_zone_mouse_left_2press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

	return -1;
}

gint vaa_itx_event_mouse_right_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
#if 0
	VAAID vaaid;
	ITX_RULEID pre_ruleid;
	ITX_RULEID find_ruleid;
	ITX_VARULE_SHAPE sh;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
	gint modify_pt_idx;
	int i;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

	find_ruleid = vaa_itx_find_rule(vaaid, mpt_x, mpt_y);
	if (find_ruleid == -1) return -1;

	vaa_itx_get_rule_shape(vaaid, find_ruleid, &sh);
	if (_find_figure_point(mpt_x, mpt_y, sh.pt, sh.ptcnt, &modify_pt_idx) == -1) return -1;

	pre_ruleid = _get_highlight_ruleid();

	if (pre_ruleid != -1) 
	{
		_make_lowlight(vaaid, pre_ruleid);

		vaa_itx_get_rule_shape(vaaid, pre_ruleid, &sh);
		for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;
		vaa_itx_set_rule_shape(vaaid, pre_ruleid, &sh);
	}

	evt_send_to_local(INFY_VAA_SELECT_RULE_ID, find_ruleid, 0, 0);

	_make_highlight(vaaid, find_ruleid);

	vaa_itx_get_rule_shape(vaaid, find_ruleid, &sh);
	for (i = 0; i < sh.ptcnt; ++i) sh.pt[i].opt = 0;

	sh.pt[modify_pt_idx].opt = 1;
	vaa_itx_set_rule_shape(vaaid, find_ruleid, &sh);
	_set_highlight_ruleid(find_ruleid);
	_set_last_mpt(mpt_x, mpt_y);

	if (sh.ptcnt >= 4)
		evt_send_to_local(INFY_VCA_SELECT_MODIFY_POINT, sh.ptcnt, 0, 0);
#endif		
	return 0;
}

gint vaa_itx_event_mouse_right_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

    ret = _proc_zone_mouse_right_2press(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;
    
	return -1;
}

gint vaa_itx_event_mouse_left_drag(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data)
{
	VAAID vaaid;
	
	gint cnvs_w, cnvs_h;
	gint mpt_x, mpt_y;
    int ret;
	
	vaaid = (VAAID)user_data;
	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	mpt_x = _trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);
    
    ret = _proc_cntr_mouse_drag(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  
    
    ret = _proc_zone_mouse_drag(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

    ret = _proc_calb_mouse_drag(vaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

	return -1;
}

gint vaa_itx_event_select_ruleid(int type, int select_ruleid, gpointer user_data)
{
	VAAID vaaid;

	vaaid = (VAAID)user_data;

     _proc_all_rule_lowlight(vaaid);

    if (type == 0) 
    {    
        if (_get_highlight_zoneid(vaaid) == select_ruleid) return -1;
       
        _proc_select_zone_highlight(vaaid, select_ruleid);
    }
    else if (type == 1) 
    {    
        if (_get_highlight_cntrid(vaaid) == select_ruleid) return -1;
        
        _proc_select_cntr_highlight(vaaid, select_ruleid);
    }
    else if (type == 2) 
    {
        if (_get_highlight_calbid(vaaid) == select_ruleid) return -1;

        _proc_select_calb_highlight(vaaid, select_ruleid);
    }

	return 0;
}

int vaa_itx_notify_vca_event(NF_NOTIFY_INFO *data)
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
	vaa_itx_show_event(vaaid, p[1], pevt);
	return 0;
}

int vaa_itx_notify_vca_track_info(NF_NOTIFY_INFO *data)
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
	vaa_itx_show_meta(vaaid, p[1], pevt);
	return 0;
}

int vaa_itx_notify_vca_meta_data(NF_NOTIFY_INFO *data)
{

	return 0;
}

int vaa_itx_notify_vca_counter_info(NF_NOTIFY_INFO *data)
{
	gint *p;
	ivca_meta_cnt_t *pevt;

	VAAID vaaid;

	p = data->p.ptr;
	pevt = p + 2;
	if (p[0] >= 16) return -1;

	vaaid = vaa_get_vaaid(p[0]);
	vaa_itx_show_counter_value_event(vaaid, p[1], pevt);
	return 0;
}

int vaa_itx_notify_vca_analyze_event(NF_NOTIFY_INFO *data)
{
	ivca_rule_event_t *pevt;
	gint *p;

	VAAID vaaid;
	
	p = data->p.ptr;
	pevt = p + 2;

	vaaid = vaa_get_pb_vaaid(p[0]-16);
//	vaa_itx_show_zone_value_event(vaaid, p[1], pevt);
	return 0;
}

