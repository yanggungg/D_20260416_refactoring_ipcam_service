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

static ITX_ZONEID _get_highlight_zoneid(DVAAID dvaaid)
{
	ITX_DVAZONE_CONF conf;
    int i;

    for (i = 0; i < 16; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));
    	dvaa_itx_detector_get_zone_conf(dvaaid, i, &conf);
    	if (conf.use_zone && conf.focus) return i;
    }

	return -1;
}

static ITX_CNTRID _get_highlight_cntrid(DVAAID dvaaid)
{
	ITX_DVACNTR_CONF conf;
    int i;

    for (i = 0; i < 16; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));
    	dvaa_itx_detector_get_cntr_conf(dvaaid, i, &conf);
    	if (conf.use_cntr && conf.focus) return i;
    }

	return -1;
}

static ITX_CALBID _get_highlight_calbid(DVAAID dvaaid)
{
	ITX_DVACALB_CONF conf;
    int i;

    for (i = 0; i < 32; i++)
    {
    	memset(&conf, 0x00, sizeof(ITX_DVACALB_CONF));
    	dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
    	if (conf.use_calb && conf.focus) return i;
    }

	return -1;
}

static int _set_zone_highlight_shape(DVAAID dvaaid, ITX_ZONEID zoneid, int mpt_x, int mpt_y)
{
	ITX_DVAZONE_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_DVAZONE_SHAPE));
	dvaa_itx_detector_get_zone_shape(dvaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0) {
		shape.pt[pt_idx].opt = 1;
	}

	dvaa_itx_detector_set_zone_shape(dvaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_lowlight_shape(DVAAID dvaaid, ITX_ZONEID zoneid)
{
	ITX_DVAZONE_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_DVAZONE_SHAPE));
	dvaa_itx_detector_get_zone_shape(dvaaid, zoneid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	dvaa_itx_detector_set_zone_shape(dvaaid, zoneid, &shape);
	return 0;
}

static int _set_zone_highlight_conf(DVAAID dvaaid, ITX_ZONEID zoneid)
{
	ITX_DVAZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	conf.focus = 1;
	dvaa_itx_detector_set_zone_conf(dvaaid, zoneid, &conf);
	return 0;
}

static int _set_zone_lowlight_conf(DVAAID dvaaid, ITX_ZONEID zoneid)
{
	ITX_DVAZONE_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	conf.focus = 0;
	dvaa_itx_detector_set_zone_conf(dvaaid, zoneid, &conf);
	return 0;
}

static int _set_cntr_highlight_shape(DVAAID dvaaid, ITX_CNTRID cntrid, int mpt_x, int mpt_y)
{
	ITX_DVACNTR_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_DVACNTR_SHAPE));
	dvaa_itx_detector_get_cntr_shape(dvaaid, cntrid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0) {
		shape.pt[pt_idx].opt = 1;
	}

	dvaa_itx_detector_set_cntr_shape(dvaaid, cntrid, &shape);
	return 0;
}

static int _set_cntr_lowlight_shape(DVAAID dvaaid, ITX_CNTRID cntrid)
{
	ITX_DVACNTR_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_DVACNTR_SHAPE));
	dvaa_itx_detector_get_cntr_shape(dvaaid, cntrid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	dvaa_itx_detector_set_cntr_shape(dvaaid, cntrid, &shape);
	return 0;
}

static int _set_cntr_highlight_conf(DVAAID dvaaid, ITX_CNTRID cntrid)
{
	ITX_DVACNTR_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));
	dvaa_itx_detector_get_cntr_conf(dvaaid, cntrid, &conf);
	conf.focus = 1;
	dvaa_itx_detector_set_cntr_conf(dvaaid, cntrid, &conf);
	return 0;
}

static int _set_cntr_lowlight_conf(DVAAID dvaaid, ITX_CNTRID cntrid)
{
	ITX_DVACNTR_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));
	dvaa_itx_detector_get_cntr_conf(dvaaid, cntrid, &conf);
	conf.focus = 0;
	dvaa_itx_detector_set_cntr_conf(dvaaid, cntrid, &conf);
	return 0;
}

static int _set_calb_highlight_shape(DVAAID dvaaid, ITX_CALBID calbid, int mpt_x, int mpt_y)
{
	ITX_DVACALB_SHAPE shape;
    int i, pt_idx;

	memset(&shape, 0x00, sizeof(ITX_DVACALB_SHAPE));
	dvaa_itx_detector_get_calb_shape(dvaaid, calbid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, shape.pt, shape.ptcnt, &pt_idx) == 0) {
		shape.pt[pt_idx].opt = 1;
	}

	dvaa_itx_detector_set_calb_shape(dvaaid, calbid, &shape);
	return 0;
}

static int _set_calb_lowlight_shape(DVAAID dvaaid, ITX_CALBID calbid)
{
	ITX_DVACALB_SHAPE shape;
    int i;

	memset(&shape, 0x00, sizeof(ITX_DVACALB_SHAPE));
	dvaa_itx_detector_get_calb_shape(dvaaid, calbid, &shape);

    for (i = 0; i < shape.ptcnt; ++i) {
        shape.pt[i].opt = 0;
    }

	dvaa_itx_detector_set_calb_shape(dvaaid, calbid, &shape);
	return 0;
}

static int _set_calb_highlight_conf(DVAAID dvaaid, ITX_CALBID calbid)
{
	ITX_DVACALB_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVACALB_CONF));
	dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
	conf.focus = 1;
	dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);
	return 0;
}

static int _set_calb_lowlight_conf(DVAAID dvaaid, ITX_CALBID calbid)
{
	ITX_DVACALB_CONF conf;

	memset(&conf, 0x00, sizeof(ITX_DVACALB_CONF));
	dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
	conf.focus = 0;
	dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);
	return 0;
}

static int _proc_zone_mouse_left_press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
	ITX_ZONEID zoneid;
	ITX_DVAZONE_CONF conf;
	ITX_DVAZONE_SHAPE shape;
	int i, pt_idx;

	zoneid = dvaa_itx_detector_find_zone(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y);
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));	
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	_set_zone_highlight_shape(dvaaid, zoneid, evt_pt->x, evt_pt->y);
	_set_zone_highlight_conf(dvaaid, zoneid);	
	_set_last_mpt(evt_pt->x, evt_pt->y);

	evt_send_to_local(INFY_DVAA_ITX_PRESS_ZONE_ID, zoneid, 0, 0);	
	
	return 0;
}

static int _proc_cntr_mouse_left_press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
	ITX_CNTRID cntrid;
	ITX_DVACNTR_CONF conf;
	ITX_DVACNTR_SHAPE shape;
	int i, pt_idx;
		
	cntrid = dvaa_itx_detector_find_cntr(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y, scrInfo->degree);
	if (cntrid == -1) return -1;
		
	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));	
	dvaa_itx_detector_get_cntr_conf(dvaaid, cntrid, &conf);
	if (!conf.active) return -1;

	_set_cntr_highlight_conf(dvaaid, cntrid);	
	_set_last_mpt(evt_pt->x, evt_pt->y);

	evt_send_to_local(INFY_DVAA_ITX_PRESS_CNTR_ID, cntrid, 0, 0);

	return 0;
}

static int _proc_calb_mouse_left_press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
	ITX_CALBID calbid;
	ITX_DVACALB_SHAPE shape;
	int i, pt_idx;
		
	calbid = dvaa_itx_detector_find_calb(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y);
	if (calbid == -1) return -1;
		    
	_set_calb_highlight_shape(dvaaid, calbid, evt_pt->x, evt_pt->y);
	_set_calb_highlight_conf(dvaaid, calbid);	
	_set_last_mpt(evt_pt->x, evt_pt->y);

	evt_send_to_local(INFY_DVAA_ITX_PRESS_CALB_ID, calbid, 0, 0);

	return 0;
}

static int _proc_zone_mouse_left_2press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_ZONEID zoneid;
	ITX_DVAZONE_CONF conf;
    ITX_DVAZONE_SHAPE shape;

	zoneid = dvaa_itx_detector_find_zone(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y);	
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));	
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	dvaa_itx_detector_get_zone_shape(dvaaid, zoneid, &shape);

	if (shape.ptcnt == 2) {
    	evt_send_to_local(INFY_DVAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }
    
	if (_dvaa_event_add_point(evt_pt->x, evt_pt->y, shape.pt, &shape.ptcnt) == -1) {
    	evt_send_to_local(INFY_DVAA_ITX_2PRESS_ZONE_ID, zoneid, 0, 0);
	    return -1;
    }

    if (shape.ptcnt > MAX_PT) return -1;
        
   	dvaa_itx_detector_set_zone_shape(dvaaid, zoneid, &shape);
  	_set_last_mpt(evt_pt->x, evt_pt->y);	

    return 0;
}

static int _proc_cntr_mouse_left_2press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_CNTRID cntrid;
	ITX_DVACNTR_CONF conf;

	cntrid = dvaa_itx_detector_find_cntr(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y, scrInfo->degree);	
	if (cntrid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));	
	dvaa_itx_detector_get_cntr_conf(dvaaid, cntrid, &conf);
	if (!conf.active) return -1;

	evt_send_to_local(INFY_DVAA_ITX_2PRESS_CNTR_ID, cntrid, 0, 0);
    return 0;
}

static int _proc_zone_mouse_right_2press(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_ZONEID zoneid;
	ITX_DVAZONE_CONF conf;
    ITX_DVAZONE_SHAPE shape;

	zoneid = dvaa_itx_detector_find_zone(dvaaid, evt_pt->x, evt_pt->y, scrInfo->scale_x, scrInfo->scale_y);	
	if (zoneid == -1) return -1;
	
	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));	
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

	dvaa_itx_detector_get_zone_shape(dvaaid, zoneid, &shape);
	if (shape.ptcnt == 2) return -1;
	if (shape.ptcnt == 3) return -1;
	if (_dvaa_event_del_point(evt_pt->x, evt_pt->y, shape.pt, &shape.ptcnt) == -1) return -1;
	
	dvaa_itx_detector_set_zone_shape(dvaaid, zoneid, &shape);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_zone_mouse_drag(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_ZONEID zoneid;
	ITX_DVAZONE_CONF conf;
    ITX_DVAZONE_SHAPE shape;
    DVAAE_POINT dmpt;
	int cnvs_w, cnvs_h;
    int pt_idx;
	int valid;
    
	zoneid = _get_highlight_zoneid(dvaaid);
	if (zoneid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));	
	dvaa_itx_detector_get_zone_conf(dvaaid, zoneid, &conf);
	if (!conf.active) return -1;

    _dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
	
	dvaa_itx_detector_get_zone_shape(dvaaid, zoneid, &shape);
	pt_idx = _dvaa_event_get_highlight_point(shape.pt, shape.ptcnt);

	if (pt_idx != -1)
	{
		_dvaa_event_move_highlight_point(evt_pt->x, evt_pt->y, pt_idx, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y);
	}
	else
	{
		_get_last_mpt(&dmpt);		
		_dvaa_event_move_highlight_mpt(evt_pt->x, evt_pt->y, dmpt, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y);		
		_set_last_mpt(evt_pt->x, evt_pt->y);		
	}

    if (valid == -1) return -1;

	dvaa_itx_detector_set_zone_shape(dvaaid, zoneid, &shape);
    return 0;
}

static int _proc_cntr_mouse_drag(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_CNTRID cntrid;
	ITX_DVACNTR_CONF conf;
    ITX_DVACNTR_SHAPE shape;
    DVAAE_POINT dmpt;
	int cnvs_w, cnvs_h;
    int pt_idx = -1;
	int valid;
    
	cntrid = _get_highlight_cntrid(dvaaid);
	if (cntrid == -1) return -1;

	memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));	
	dvaa_itx_detector_get_cntr_conf(dvaaid, cntrid, &conf);
	if (!conf.active) return -1;

    _dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
	
	dvaa_itx_detector_get_cntr_shape(dvaaid, cntrid, &shape);
	valid = _dvaa_event_check_valid_point_degree(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y, scrInfo->degree);
	g_message("%s, %d", __FUNCTION__, __LINE__);
	if (valid == -1) {
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_dvaa_event_get_valid_point_degree(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y, scrInfo->degree); 
		dvaa_itx_detector_set_cntr_shape(dvaaid, cntrid, &shape);
		return 0;
	}

	g_message("%s, %d", __FUNCTION__, __LINE__);
	_get_last_mpt(&dmpt);
	_dvaa_event_move_highlight_mpt(evt_pt->x, evt_pt->y, dmpt, shape.pt, shape.ptcnt);
	valid = _dvaa_event_check_valid_point_degree(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y, scrInfo->degree);
	_set_last_mpt(evt_pt->x, evt_pt->y);		
		
    if (valid == -1) 
		return -1;

	dvaa_itx_detector_set_cntr_shape(dvaaid, cntrid, &shape);
    return 0;
}

static int _proc_calb_mouse_drag(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt)
{
    ITX_CALBID calbid;
    ITX_DVACALB_SHAPE shape;
    DVAAE_POINT dmpt;
	int cnvs_w, cnvs_h;    
    int pt_idx = -1;
	int valid;
    
	calbid = _get_highlight_calbid(dvaaid);
	if (calbid == -1) return -1;

    _dvaa_event_get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
	
	dvaa_itx_detector_get_calb_shape(dvaaid, calbid, &shape);
	pt_idx = _dvaa_event_get_highlight_point(shape.pt, shape.ptcnt);

	if (pt_idx != -1)
	{
		_dvaa_event_move_highlight_point(evt_pt->x, evt_pt->y, pt_idx, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y);
	}
	else
	{
		_get_last_mpt(&dmpt);		
		_dvaa_event_move_highlight_mpt(evt_pt->x, evt_pt->y, dmpt, shape.pt, shape.ptcnt);
		valid = _dvaa_event_check_valid_point(cnvs_w, cnvs_h, shape.pt, shape.ptcnt, scrInfo->scale_x, scrInfo->scale_y);		
		_set_last_mpt(evt_pt->x, evt_pt->y);		
	}
		
    if (valid == -1) return -1;

	dvaa_itx_detector_set_calb_shape(dvaaid, calbid, &shape);
    return 0;
}

static int _proc_all_rule_lowlight(DVAAID dvaaid)
{
	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;
	ITX_CALBID calbid;

	zoneid = _get_highlight_zoneid(dvaaid);

	if (zoneid != -1) 
	{
        _set_zone_lowlight_shape(dvaaid, zoneid);
        _set_zone_lowlight_conf(dvaaid, zoneid);
	}

	cntrid = _get_highlight_cntrid(dvaaid);

	if (cntrid != -1) 
	{
        _set_cntr_lowlight_shape(dvaaid, cntrid);
        _set_cntr_lowlight_conf(dvaaid, cntrid);
	}

	calbid = _get_highlight_calbid(dvaaid);

	if (calbid != -1) 
	{
        _set_calb_lowlight_shape(dvaaid, calbid);
        _set_calb_lowlight_conf(dvaaid, calbid);
	}
	
    return 0;
}

static int _proc_select_zone_highlight(DVAAID dvaaid, ITX_ZONEID zoneid)
{
    _set_zone_highlight_shape(dvaaid, zoneid, 0, 0);
    _set_zone_highlight_conf(dvaaid, zoneid);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_select_cntr_highlight(DVAAID dvaaid, ITX_CNTRID cntrid)
{
//    _set_cntr_highlight_shape(dvaaid, cntrid, 0, 0);
    _set_cntr_highlight_conf(dvaaid, cntrid);
	_set_last_mpt(0, 0);
    return 0;
}

static int _proc_select_calb_highlight(DVAAID dvaaid, ITX_CALBID calbid)
{
    _set_calb_highlight_shape(dvaaid, calbid, 0, 0);
    _set_calb_highlight_conf(dvaaid, calbid);
	_set_last_mpt(0, 0);
    return 0;
}




////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _dvaa_itx_detector_event_mouse_left_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
    int ret;

	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;
	ITX_CALBID calbid;	

	dvaaid = (DVAAID)user_data;

	cntrid = _get_highlight_cntrid(dvaaid);
	if (cntrid != -1) _set_cntr_lowlight_conf(dvaaid, cntrid);
	
	zoneid = _get_highlight_zoneid(dvaaid);
	if (zoneid != -1) _set_zone_lowlight_conf(dvaaid, zoneid);

	calbid = _get_highlight_calbid(dvaaid);
	if (calbid != -1) _set_calb_lowlight_conf(dvaaid, calbid);
    
    ret = _proc_cntr_mouse_left_press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  
   
    ret = _proc_zone_mouse_left_press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  

    ret = _proc_calb_mouse_left_press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  

    evt_send_to_local(INFY_DVAA_ITX_PRESS_NONE_ID, 0, 0, 0);
    
    _set_last_mpt(0, 0);        
	return -1;
}

int _dvaa_itx_detector_event_mouse_left_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
    int ret;
	
	dvaaid = (DVAAID)user_data;
    
    ret = _proc_cntr_mouse_left_2press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  
    
    ret = _proc_zone_mouse_left_2press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  

	return -1;
}

int _dvaa_itx_detector_event_mouse_right_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{

	return 0;
}

int _dvaa_itx_detector_event_mouse_right_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
    int ret;
	
	dvaaid = (DVAAID)user_data;

    ret = _proc_zone_mouse_right_2press(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;
    
	return -1;
}

int _dvaa_itx_detector_event_mouse_left_drag(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data)
{
	DVAAID dvaaid;
    int ret;
	
	dvaaid = (DVAAID)user_data;
    
    ret = _proc_cntr_mouse_drag(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  
    
    ret = _proc_zone_mouse_drag(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  

    ret = _proc_calb_mouse_drag(dvaaid, scrInfo, evt_pt);
    if (ret == 0) return 0;  

	return -1;
}

int _dvaa_itx_detector_event_select_ruleid(int type, int select_ruleid, gpointer user_data)
{
	DVAAID dvaaid;

	dvaaid = (DVAAID)user_data;

     _proc_all_rule_lowlight(dvaaid);

    if (type == 0) 
    {    
        if (_get_highlight_zoneid(dvaaid) == select_ruleid) return -1;
       
        _proc_select_zone_highlight(dvaaid, select_ruleid);
    }
    else if (type == 1) 
    {    
        if (_get_highlight_cntrid(dvaaid) == select_ruleid) return -1;
        
        _proc_select_cntr_highlight(dvaaid, select_ruleid);
    }
    else if (type == 2) 
    {
        if (_get_highlight_calbid(dvaaid) == select_ruleid) return -1;

        _proc_select_calb_highlight(dvaaid, select_ruleid);
    }

	return 0;
}

int _dvaa_itx_detector_remove_highlight(DVAAID dvaaid)
{
     _proc_all_rule_lowlight(dvaaid);
	return 0;
}
