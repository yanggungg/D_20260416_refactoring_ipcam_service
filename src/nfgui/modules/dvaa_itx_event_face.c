/*
 * dvaa_itx_event.c
 * 	- deeplearning video analytics agent for itx
 *	- dependencies :
 *		viewer
 *		dit		
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */


#include <math.h>
#include "nf_afx.h"
#include "ix_mem.h"
#include "nf_meta_data.h"

#include "vw_vca.h"
#include "dvaa.h"
#include "dvaa_itx.h"
#include "iux_afx.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		9
#define DBG_MODULE		"DVAA_ITX_EVENT"


////////////////////////////////////////////////////////////
//
// private data type 
//






////////////////////////////////////////////////////////////
//
// private variable
//

static DVAAE_POINT g_last_mpt;




////////////////////////////////////////////////////////////
//
// private functions
//

static int _get_last_mpt(DVAAE_POINT *dmpt)
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

static ITX_FRZONEID _get_highlight_zoneid(DVAAID dvaaid)
{
	ITX_FRZONE_CONF conf;
    int i;

    for (i = 0; i < 16; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));
    	dvaa_itx_face_get_zone_conf(dvaaid, i, &conf);
    	if (conf.use_zone && conf.focus) return i;
    }

	return -1;
}

static int _set_zone_highlight_shape(DVAAID dvaaid, ITX_FRZONEID zoneid, int mpt_x, int mpt_y)
{
	ITX_FRZONE_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_FRZONE_SHAPE));
	dvaa_itx_face_get_zone_shape(dvaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0) {
		shape.pt[pt_idx].opt = 1;
	}

	dvaa_itx_face_set_zone_shape(dvaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_lowlight_shape(DVAAID dvaaid, ITX_FRZONEID zoneid)
{
	ITX_FRZONE_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_FRZONE_SHAPE));
	dvaa_itx_face_get_zone_shape(dvaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	dvaa_itx_face_set_zone_shape(dvaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_highlight_conf(DVAAID dvaaid, ITX_FRZONEID zoneid)
{
	ITX_FRZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	conf.focus = 1;
	dvaa_itx_face_set_zone_conf(dvaaid, zoneid, &conf);
	return 0;
}

static int _set_zone_lowlight_conf(DVAAID dvaaid, ITX_FRZONEID zoneid)
{
	ITX_FRZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	conf.focus = 0;
	dvaa_itx_face_set_zone_conf(dvaaid, zoneid, &conf);
	return 0;
}

static int _proc_zone_mouse_left_press(DVAAID dvaaid, int mpt_x, int mpt_y)
{
	ITX_FRZONEID zoneid;
	ITX_FRZONE_CONF conf;
	ITX_FRZONE_SHAPE shape;
	int i, pt_idx;

	zoneid = dvaa_itx_face_find_zone(dvaaid, mpt_x, mpt_y);
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));	
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	_set_zone_highlight_shape(dvaaid, zoneid, mpt_x, mpt_y);
	_set_zone_highlight_conf(dvaaid, zoneid);	
	_set_last_mpt(mpt_x, mpt_y);

	evt_send_to_local(INFY_DVAA_ITX_PRESS_ZONE_ID, zoneid, 0, 0);	
	
	return 0;
}

static int _proc_zone_mouse_left_2press(DVAAID dvaaid, int mpt_x, int mpt_y)
{
    ITX_FRZONEID zoneid;
	ITX_FRZONE_CONF conf;
    ITX_FRZONE_SHAPE shape;

	zoneid = dvaa_itx_face_find_zone(dvaaid, mpt_x, mpt_y);	
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));	
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	dvaa_itx_face_get_zone_shape(dvaaid, zoneid, &shape);

	if (shape.ptcnt == 2) {
    	evt_send_to_local(INFY_DVAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }
    
	if (_dvaa_event_add_point(mpt_x, mpt_y, shape.pt, &shape.ptcnt) == -1) {
    	evt_send_to_local(INFY_DVAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }

    if (shape.ptcnt > MAX_PT) return -1;
        
   	dvaa_itx_face_set_zone_shape(dvaaid, zoneid, &shape);
  	_set_last_mpt(mpt_x, mpt_y);	

    return 0;
}

static int _proc_zone_mouse_right_2press(DVAAID dvaaid, int mpt_x, int mpt_y)
{
    ITX_FRZONEID zoneid;
	ITX_FRZONE_CONF conf;
    ITX_FRZONE_SHAPE shape;

	zoneid = dvaa_itx_face_find_zone(dvaaid, mpt_x, mpt_y);	
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));	
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	dvaa_itx_face_get_zone_shape(dvaaid, zoneid, &shape);
	if (shape.ptcnt == 2) return -1;
	if (shape.ptcnt == 3) return -1;
	if (_dvaa_event_del_point(mpt_x, mpt_y, shape.pt, &shape.ptcnt) == -1) return -1;
	
	dvaa_itx_face_set_zone_shape(dvaaid, zoneid, &shape);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_zone_mouse_drag(DVAAID dvaaid, int mpt_x, int mpt_y, float scale_x, float scale_y)
{
    ITX_FRZONEID zoneid;
	ITX_FRZONE_CONF conf;
    ITX_FRZONE_SHAPE shape;
    DVAAE_POINT dmpt;
	int cnvs_w, cnvs_h;
    int pt_idx;
	int valid;
    
	zoneid = _get_highlight_zoneid(dvaaid);
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));	
	dvaa_itx_face_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

    _dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
	
	dvaa_itx_face_get_zone_shape(dvaaid, zoneid, &shape);
	pt_idx = _dvaa_event_get_highlight_point(shape.pt, shape.ptcnt);

	if (pt_idx != -1)
	{
		_dvaa_event_move_highlight_point(mpt_x, mpt_y, pt_idx, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scale_x, scale_y);
	}
	else
	{
		_get_last_mpt(&dmpt);		
		_dvaa_event_move_highlight_mpt(mpt_x, mpt_y, dmpt, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scale_x, scale_y);		
		_set_last_mpt(mpt_x, mpt_y);		
	}

    if (valid == -1) return -1;

	dvaa_itx_face_set_zone_shape(dvaaid, zoneid, &shape);
    return 0;
}

static int _proc_all_rule_lowlight(DVAAID dvaaid)
{
	ITX_FRZONEID zoneid;

	zoneid = _get_highlight_zoneid(dvaaid);

	if (zoneid != -1) 
	{
        _set_zone_lowlight_shape(dvaaid, zoneid);
        _set_zone_lowlight_conf(dvaaid, zoneid);
	}
	
    return 0;
}

static int _proc_select_zone_highlight(DVAAID dvaaid, ITX_FRZONEID zoneid)
{
    _set_zone_highlight_shape(dvaaid, zoneid, 0, 0);
    _set_zone_highlight_conf(dvaaid, zoneid);
	_set_last_mpt(0, 0);
    return 0;
}





////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _dvaa_itx_face_event_mouse_left_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;

	ITX_FRZONEID zoneid;

	dvaaid = (DVAAID)user_data;
	_dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);

	mpt_x = _dvaa_event_trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _dvaa_event_trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);
	
	zoneid = _get_highlight_zoneid(dvaaid);
	if (zoneid != -1) _set_zone_lowlight_conf(dvaaid, zoneid);
       
    ret = _proc_zone_mouse_left_press(dvaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

    //evt_send_to_local(INFY_DVAA_ITX_PRESS_NONE_ID, 0, 0, 0);
    
    _set_last_mpt(0, 0);        
	return -1;
}

int _dvaa_itx_face_event_mouse_left_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;
	
	dvaaid = (DVAAID)user_data;
	_dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);

	mpt_x = _dvaa_event_trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _dvaa_event_trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);
    
    ret = _proc_zone_mouse_left_2press(dvaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;  

	return -1;
}

int _dvaa_itx_face_event_mouse_right_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{

	return 0;
}

int _dvaa_itx_face_event_mouse_right_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
	int cnvs_w, cnvs_h;
	int mpt_x, mpt_y;
    int ret;
	
	dvaaid = (DVAAID)user_data;
	_dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);

	mpt_x = _dvaa_event_trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _dvaa_event_trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

    ret = _proc_zone_mouse_right_2press(dvaaid, mpt_x, mpt_y);
    if (ret == 0) return 0;
    
	return -1;
}

int _dvaa_itx_face_event_mouse_left_drag(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
	gint cnvs_w, cnvs_h;
	float scale_x, scale_y;
	gint mpt_x, mpt_y;
    int ret;
	
	dvaaid = (DVAAID)user_data;
	_dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);

	scale_x = (float)cnvs_w/scrInfo->w;
	scale_y = (float)cnvs_h/scrInfo->h;

	mpt_x = _dvaa_event_trans_x_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
	mpt_y = _dvaa_event_trans_y_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);
    
    ret = _proc_zone_mouse_drag(dvaaid, mpt_x, mpt_y, scale_x, scale_y);
    if (ret == 0) return 0;  

	return -1;
}

int _dvaa_itx_face_event_select_ruleid(int type, int select_ruleid, gpointer user_data)
{
	DVAAID dvaaid;

	dvaaid = (DVAAID)user_data;

     _proc_all_rule_lowlight(dvaaid);

    if (type == 0) 
    {    
        if (_get_highlight_zoneid(dvaaid) == select_ruleid) return -1;
       
        _proc_select_zone_highlight(dvaaid, select_ruleid);
    }

	return 0;
}

int _dvaa_itx_face_remove_highlight(DVAAID dvaaid)
{
     _proc_all_rule_lowlight(dvaaid);
	return 0;
}
