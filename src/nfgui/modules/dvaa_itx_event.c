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

typedef struct _DVAAE_VMAP {
    int             use;
    int             x;
    int             y;
    int             w;
    int             h;    
} DVAAE_VMAP;



////////////////////////////////////////////////////////////
//
// private variable
//

static DVAAE_VMAP g_vmap;



////////////////////////////////////////////////////////////
//
// private functions
//

static IXPOINT rotate_point(int cx, int cy, int x, int y, int degree)
{
    float rad = degree*3.1415926/180;
    float s = sin(rad);
    float c = cos(rad);
    IXPOINT	npt;

    if (degree == 0) {
        npt.x = x;
        npt.y = y;
    }
    else {
        npt.x = cx + (x-cx) * c - (y-cy) * s;
        npt.y = cy + (x-cx) * s + (y-cy) * c;
    }
    return npt;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _dvaa_itx_event_set_vmap(DVA_VMAP_INFO *vmapInfo, gpointer user_data)
{
    g_vmap.use = vmapInfo->use;
    g_vmap.x = vmapInfo->x;
    g_vmap.y = vmapInfo->y;
    g_vmap.w = vmapInfo->w;
    g_vmap.h = vmapInfo->h;
	return 0;
}

int _dvaa_event_get_highlight_point(IGPOINT *figure_pt, int figure_cnt)
{
	int i;
	for (i = 0; i < figure_cnt; ++i) 
		if (figure_pt[i].opt == 1) return i;

	return -1;
}

int _dvaa_event_trans_x_vpoint(int vcnvs, int rpoint, int rcnvs)
{
	int vpoint;	
	
    if (g_vmap.use) rpoint = rpoint*g_vmap.w/rcnvs + g_vmap.x;
    
	vpoint = (int)((float)(rpoint*vcnvs/rcnvs));	
	return vpoint;
}

int _dvaa_event_trans_y_vpoint(int vcnvs, int rpoint, int rcnvs)
{
	int vpoint;	
	
    if (g_vmap.use) rpoint = rpoint*g_vmap.h/rcnvs + g_vmap.y;
    
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));	
	return vpoint;
}

int _dvaa_event_get_cnvs_size(DVAAID dvaaid, int *cnvs_w, int *cnvs_h)
{
	DITID ditid;
	
	ditid = dvaa_itx_get_dit(dvaaid);
	dit_get_cnvs_info(ditid, cnvs_w, cnvs_h);
	return 0;
}

int _dvaa_event_find_figure_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int figure_cnt, int *point_index)
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

int _dvaa_event_move_highlight_mpt(int mpt_x, int mpt_y, DVAAE_POINT dmpt, IGPOINT *figure_pt, int figure_cnt)
{
	int i;

	for (i = 0; i < figure_cnt; i++)
	{
		figure_pt[i].x += (mpt_x - dmpt.x);
		figure_pt[i].y += (mpt_y - dmpt.y);
	}

	return 0;
}

int _dvaa_event_move_highlight_point(int mpt_x, int mpt_y, int pt_idx, IGPOINT *figure_pt, int figure_cnt)
{
	if (pt_idx >= figure_cnt) return -1;

	figure_pt[pt_idx].x = mpt_x;
	figure_pt[pt_idx].y = mpt_y;
	return 0;
}

int _dvaa_event_check_valid_point(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y)
{
	int i;

	for (i = 0; i < figure_cnt; i++)
	{
		if ((figure_pt[i].x+(int)(figure_pt[i].dx*scale_x) < 0) || (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y) < 0)
			|| (figure_pt[i].x+(int)(figure_pt[i].dx*scale_x) > cnvs_w) || (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y) > cnvs_h))
		{
			return -1;
		}
	}

	return 0;
}

int _dvaa_event_check_valid_point_degree(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y, int degree)
{
    IXPOLYGON p;
    IXPOINT	npt;
	int i;

    p.pt[0].x = figure_pt[0].x+(int)(figure_pt[0].dx*scale_x);
    p.pt[0].y = figure_pt[0].y+(int)(figure_pt[0].dy*scale_y);

    for (i = 1; i < figure_cnt; ++i) {
        npt.x = figure_pt[i].x+(int)(figure_pt[i].dx*scale_x);
        npt.y = figure_pt[i].y+(int)(figure_pt[i].dy*scale_y);
        p.pt[i] = rotate_point(p.pt[0].x, p.pt[0].y, npt.x, npt.y, degree);
    }

	for (i = 0; i < figure_cnt; i++)
	{
		if ((p.pt[i].x < 0) || (p.pt[i].y < 0)
			|| (p.pt[i].x > cnvs_w) || (p.pt[i].y > cnvs_h))
		{
			return -1;
		}
	}

	return 0;
}

int _dvaa_event_get_valid_point(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y)
{
	int dx = 0, dy = 0;
	int i;

	for (i = 0; i < figure_cnt; i++)
	{
		if (figure_pt[i].x+(int)(figure_pt[i].dx*scale_x) < 0) 
			dx = 0 - (figure_pt[i].x+(int)(figure_pt[i].dx*scale_x));
		else if (figure_pt[i].x+(int)(figure_pt[i].dx*scale_x) > cnvs_w) 
			dx = cnvs_w - (figure_pt[i].x+(int)(figure_pt[i].dx*scale_x));

		if (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y) < 0) 
			dy = 0 - (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y));
		else if (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y) > cnvs_h)
			dy = cnvs_h - (figure_pt[i].y+(int)(figure_pt[i].dy*scale_y));
	}

	for (i = 0; i < figure_cnt; i++)
	{
		figure_pt[i].x += dx;
		figure_pt[i].y += dy;
	}

	return 0;
}

int _dvaa_event_get_valid_point_degree(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y, int degree)
{
    IXPOLYGON p;
    IXPOINT	npt;
	int dx = 0, dy = 0;
	int i;

    p.pt[0].x = figure_pt[0].x+(int)(figure_pt[0].dx*scale_x);
    p.pt[0].y = figure_pt[0].y+(int)(figure_pt[0].dy*scale_y);

    for (i = 1; i < figure_cnt; ++i) {
        npt.x = figure_pt[i].x+(int)(figure_pt[i].dx*scale_x);
        npt.y = figure_pt[i].y+(int)(figure_pt[i].dy*scale_y);
        p.pt[i] = rotate_point(p.pt[0].x, p.pt[0].y, npt.x, npt.y, degree);
    }

	for (i = 0; i < figure_cnt; i++)
	{
		if (p.pt[i].x < 0) 
			dx = 0 - p.pt[i].x;
		else if (p.pt[i].x > cnvs_w) 
			dx = cnvs_w - p.pt[i].x;

		if (p.pt[i].y < 0) 
			dy = 0 - p.pt[i].y;
		else if (p.pt[i].y > cnvs_h) 
			dy = +cnvs_h - p.pt[i].y;
	}

	for (i = 0; i < figure_cnt; i++)
	{
		figure_pt[i].x += dx;
		figure_pt[i].y += dy;
	}

	return 0;
}

int _dvaa_event_add_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
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

int _dvaa_event_add_point_next_pt(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
{
	int i, add_pt_idx;
	IXPOINT new_pt;

	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, figure_pt, *figure_cnt, &add_pt_idx) == -1) return -1;	
	
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

int _dvaa_event_del_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt)
{
	int i, cnt = 0;
	int del_pt_idx;
	IGPOINT cp_pt[MAX_PT];
	
	if (_dvaa_event_find_figure_point(mpt_x, mpt_y, figure_pt, *figure_cnt, &del_pt_idx) == -1) return -1;

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

int _dvaa_itx_link_event(DVAAID id)
{
	vw_itx_dvabx_attach_select_ruleid(_dvaa_itx_detector_event_select_ruleid, id);	
    vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_DETECTOR, DVA_MEVENT_LEFT_PRESS, _dvaa_itx_detector_event_mouse_left_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_DETECTOR, DVA_MEVENT_RIGHT_PRESS, _dvaa_itx_detector_event_mouse_right_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_DETECTOR, DVA_MEVENT_LEFT_2PRESS, _dvaa_itx_detector_event_mouse_left_2press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_DETECTOR, DVA_MEVENT_RIGHT_2PRESS, _dvaa_itx_detector_event_mouse_right_2press, id);	
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_DETECTOR, DVA_MEVENT_DRAG, _dvaa_itx_detector_event_mouse_left_drag, id);	

	//vw_itx_dvabx_face_attach_select_ruleid(_dvaa_itx_face_event_select_ruleid, id);
    vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_FACE, DVA_MEVENT_LEFT_PRESS, _dvaa_itx_face_event_mouse_left_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_FACE, DVA_MEVENT_RIGHT_PRESS, _dvaa_itx_face_event_mouse_right_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_FACE, DVA_MEVENT_LEFT_2PRESS, _dvaa_itx_face_event_mouse_left_2press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_FACE, DVA_MEVENT_RIGHT_2PRESS, _dvaa_itx_face_event_mouse_right_2press, id);	
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_FACE, DVA_MEVENT_DRAG, _dvaa_itx_face_event_mouse_left_drag, id);	

	vw_itx_dvabx_plateno_attach_select_ruleid(_dvaa_itx_plateno_event_select_ruleid, id);
    vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_PLATENO, DVA_MEVENT_LEFT_PRESS, _dvaa_itx_plateno_event_mouse_left_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_PLATENO, DVA_MEVENT_RIGHT_PRESS, _dvaa_itx_plateno_event_mouse_right_press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_PLATENO, DVA_MEVENT_LEFT_2PRESS, _dvaa_itx_plateno_event_mouse_left_2press, id);
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_PLATENO, DVA_MEVENT_RIGHT_2PRESS, _dvaa_itx_plateno_event_mouse_right_2press, id);	
	vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_PLATENO, DVA_MEVENT_DRAG, _dvaa_itx_plateno_event_mouse_left_drag, id);		

    vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_LEFT_PRESS, _dvaa_itx_detector_event_mouse_left_press, id);
    vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_RIGHT_PRESS, _dvaa_itx_detector_event_mouse_right_press, id);
    vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_LEFT_2PRESS, _dvaa_itx_detector_event_mouse_left_2press, id);
    vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_RIGHT_2PRESS, _dvaa_itx_detector_event_mouse_right_2press, id);
    vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_DRAG, _dvaa_itx_detector_event_mouse_left_drag, id);    
    vw_dvabx_calibration_zoom_set_vmap(_dvaa_itx_event_set_vmap, 0);	
	//vw_itx_dvabx_smart_search_attach_select_ruleid(dvaa_event_select_ruleid, id);	
	
	return 0;
}




////////////////////////////////////////////////////////////
//
// public interfaces
//

int dvaa_itx_remove_highlight(DVAAID dvaaid)
{
     _dvaa_itx_detector_remove_highlight(dvaaid);
	return 0;
}

int dvaa_itx_notify_event(NF_NOTIFY_INFO *data)
{
	ai_rule_event_t *pevt;
	gint *p;

	DVAAID dvaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[0]);
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[1]);
	if (p[0] >= 32) return -1;

	dvaaid = dvaa_get_dvaaid(p[0]);
	dvaa_itx_show_event(dvaaid, p[1], pevt);
	return 0;
}

int dvaa_itx_notify_generic_event(NF_NOTIFY_INFO *data)
{
	ai_generic_event_t *pevt;
	gint *p;

	DVAAID dvaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[0]);
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[1]);
	if (p[0] >= 32) return -1;

	dvaaid = dvaa_get_dvaaid(p[0]);
	dvaa_itx_show_generic_event(dvaaid, p[1], pevt);
	return 0;
}

int dvaa_itx_notify_track_info(NF_NOTIFY_INFO *data)
{
	ai_obj_t *pevt;
	gint *p;

	DVAAID dvaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[0]);
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[1]);
	if (p[0] >= 32) return -1;

	dvaaid = dvaa_get_dvaaid(p[0]);
	dvaa_itx_show_meta(dvaaid, p[1], pevt);
	return 0;
}

int dvaa_itx_notify_meta_data(NF_NOTIFY_INFO *data)
{

	return 0;
}

int dvaa_itx_notify_counter_info(NF_NOTIFY_INFO *data)
{
	gint *p;
	ai_meta_cnt_t *pevt;

	DVAAID dvaaid;

	p = data->p.ptr;
	pevt = p + 2;
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[0]);
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[1]);
	if (p[0] >= 32) return -1;

	dvaaid = dvaa_get_dvaaid(p[0]);
	dvaa_itx_show_counter_value_event(dvaaid, p[1], pevt);
	return 0;
}

int dvaa_itx_notify_analyze_event(NF_NOTIFY_INFO *data)
{
	ai_rule_event_t *pevt;
	gint *p;

	DVAAID dvaaid;
	
	p = data->p.ptr;
	pevt = p + 2;
	// printf("SKSHIN %s, %d] %d\n", __FUNCTION__, __LINE__, p[0]);

	dvaaid = dvaa_get_pb_dvaaid(p[0]-32);
//	dvaa_show_zone_value_event(dvaaid, p[1], pevt);
	return 0;
}
