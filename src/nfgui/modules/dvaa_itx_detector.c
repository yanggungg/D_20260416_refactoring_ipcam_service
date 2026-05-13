/*
 * dvaa_itx.c
 *  - deeplearning video analytics agent for itx
 *  - dependencies :
 *      
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <math.h>
#include "dvaa_itx.h"
#include "dvaa_itx_internal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_color.h"
#include "../support/nf_ui_font.h"
#include "../support/util.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       7   
#define DBG_MODULE      "DVAA_ITX"



////////////////////////////////////////////////////////////
//
// private data type
//

#define FIGURE IFIGURE


////////////////////////////////////////////////////////////
//
// private variables
//






////////////////////////////////////////////////////////////
//
// private functions
//

static void _prinf_emosd_data(DvaBxEOsdData *data, gchar *callfunc)
{
    g_message("[%s, %d] call function : %s", __FUNCTION__, __LINE__, callfunc);
    g_message("[%s, %d] display_mode : %d", __FUNCTION__, __LINE__, data->display_mode);
    g_message("[%s, %d] object_color : %d", __FUNCTION__, __LINE__, data->object_color);
    g_message("[%s, %d] rule_color : %d", __FUNCTION__, __LINE__, data->rule_color);
    g_message("[%s, %d] event_color : %d", __FUNCTION__, __LINE__, data->event_color);
    g_message("[%s, %d] line_width  : %d", __FUNCTION__, __LINE__, data->line_width);
    g_message("[%s, %d] line_transparency : %d", __FUNCTION__, __LINE__, data->line_transparency);
    g_message("[%s, %d] object_type : %s", __FUNCTION__, __LINE__, data->object_type);
    
}

static void _prinf_emosd_data_ch(DvaBxEOsdData *data, gint ch, gchar *callfunc)
{
    g_message("[%s, %d] ch : %d call function : %s", __FUNCTION__, __LINE__, ch, callfunc);
    g_message("[%s, %d] display_mode : %d", __FUNCTION__, __LINE__, data->display_mode);
    g_message("[%s, %d] object_color : %d", __FUNCTION__, __LINE__, data->object_color);
    g_message("[%s, %d] rule_color : %d", __FUNCTION__, __LINE__, data->rule_color);
    g_message("[%s, %d] event_color : %d", __FUNCTION__, __LINE__, data->event_color);
    g_message("[%s, %d] line_width  : %d", __FUNCTION__, __LINE__, data->line_width);
    g_message("[%s, %d] line_transparency : %d", __FUNCTION__, __LINE__, data->line_transparency);
    g_message("[%s, %d] object_type : %s", __FUNCTION__, __LINE__, data->object_type);
    
}

static int _init_zone(DVAZONEPTR pzone)
{
    memset(pzone, 0x00, sizeof(ITX_DVAZONE));
    pzone->zn_data.id = -1;
    pzone->dipage = DIT_PAGE_ZONE;
    return 0;   
}

static int _init_cntr(DVACNTRPTR pcntr)
{
    memset(pcntr, 0x00, sizeof(ITX_DVACNTR));   
    pcntr->ct_data.id = -1;
    pcntr->dipage = DIT_PAGE_CNTR;
    return 0;   
}

static int _init_calb(DVACALBPTR pcalb)
{
    memset(pcalb, 0x00, sizeof(ITX_DVACALB));   
    pcalb->dipage = DIT_PAGE_CALB;
    return 0;   
}

static int _copy_prop_to_db(DvaBxPropData *src, DvaBxData *pdb)
{
    memcpy(&pdb->prop, src, sizeof(DvaBxPropData));
    return 0;
}

static int _copy_db_to_prop(DvaBxData *pdb, DvaBxPropData *dst)
{
    memcpy(dst, &pdb->prop, sizeof(DvaBxPropData));
    return 0;
}

static int _load_zone_data(DvaBxZone *zn_data, DVAZONEPTR pzone)
{
    if (zn_data) memcpy(&pzone->zn_data, zn_data, sizeof(DvaBxZone));
    return 0;
}

static int _load_cntr_data(DvaBxCntr *ct_data, DVACNTRPTR pcntr)
{
    if (ct_data) memcpy(&pcntr->ct_data, ct_data, sizeof(DvaBxCntr));
    return 0;
}

static int _unload_zone_data(DVAZONEPTR pzone, DvaBxZone *zn_data)
{
    if (zn_data) memcpy(zn_data, &pzone->zn_data, sizeof(DvaBxZone));
    return 0;
}

static int _unload_cntr_data(DVACNTRPTR pcntr, DvaBxCntr *ct_data)
{
    if (ct_data) memcpy(ct_data, &pcntr->ct_data, sizeof(DvaBxCntr));
    return 0;
}

static int _load_calb_data(DvaBxCalb *cb_data, DVACALBPTR pcalb)
{
    if (cb_data) memcpy(&pcalb->cb_data, cb_data, sizeof(DvaBxCalb));
    return 0;
}

static int _unload_calb_data(DVACALBPTR pcalb, DvaBxCalb *cb_data)
{
    if (cb_data) memcpy(cb_data, &pcalb->cb_data, sizeof(DvaBxCalb));
    return 0;
}

static int _copy_calbres_to_db(DvaBxCalbResData *src, DvaBxData *pdb)
{
    memcpy(&pdb->calbres, src, sizeof(DvaBxCalbResData));
    return 0;
}

static int _copy_db_to_calbres(DvaBxData *pdb, DvaBxCalbResData *dst)
{
    memcpy(dst, &pdb->calbres, sizeof(DvaBxCalbResData));
    return 0;
}

static int _copy_eosd_to_db(DvaBxEOsdData *src, DvaBxData *pdb)
{
    memcpy(&pdb->eosd, src, sizeof(DvaBxEOsdData));
    // _prinf_emosd_data(&pdb->eosd, __FUNCTION__);
    return 0;
}

static int _copy_db_to_eosd(DvaBxData *pdb, DvaBxEOsdData *dst)
{
    memcpy(dst, &pdb->eosd, sizeof(DvaBxEOsdData));
    // _prinf_emosd_data(dst, __FUNCTION__);
    return 0;
}

static int _load_db(ITXDVAA_T *pdvaa)
{
    DAL_get_dvabx_data(&pdvaa->db, pdvaa->ch);
    // _prinf_emosd_data_ch(&pdvaa->db.eosd, pdvaa->ch, __FUNCTION__);
    return 0;
}

static int _save_db(ITXDVAA_T *pdvaa)
{
    // _prinf_emosd_data_ch(&pdvaa->db.eosd, pdvaa->ch, __FUNCTION__);
    DAL_set_dvabx_data(pdvaa->db, pdvaa->ch);
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

static int _is_point_in_figure(IGPOINT mpt, IGPOINT *fpt, int fcnt, float scale_x, float scale_y)
{
    int i, ret;
    IXPOLYGON p;
    
    for (i = 0; i < fcnt; ++i) {
        p.pt[i].x = fpt[i].x+(int)(fpt[i].dx*scale_x);
        p.pt[i].y = fpt[i].y+(int)(fpt[i].dy*scale_y);
    }

    p.cnt = fcnt;
    ret = ifn_point_is_inside_polygon(mpt.x, mpt.y, 16, &p);    
    return (ret > 0 ? 0 : -1); 
}

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

static int _is_point_in_figure_degree(IGPOINT mpt, IGPOINT *fpt, int fcnt, float scale_x, float scale_y, int degree)
{
    int i, ret;
    IXPOLYGON p;
    IXPOINT	npt;

    p.pt[0].x = fpt[0].x+(int)(fpt[0].dx*scale_x);
    p.pt[0].y = fpt[0].y+(int)(fpt[0].dy*scale_y);

    for (i = 1; i < fcnt; ++i) {
        npt.x = fpt[i].x+(int)(fpt[i].dx*scale_x);
        npt.y = fpt[i].y+(int)(fpt[i].dy*scale_y);
        p.pt[i] = rotate_point(p.pt[0].x, p.pt[0].y, npt.x, npt.y, degree);
    }

    p.cnt = fcnt;
    ret = ifn_point_is_inside_polygon(mpt.x, mpt.y, 16, &p);    
    return (ret > 0 ? 0 : -1); 
}

static int _is_point_in_arc(IGPOINT mpt, IGPOINT fpt, int width, int height)
{

    return -1; 
}

static IGPOINT _rotate_point(IGPOINT src, IGPOINT datum_pt, gdouble cost, gdouble sint)
{
    IGPOINT dst = {0,};

    dst.x = (int)ceil(((src.x - datum_pt.x) * cost) +  ((src.y - datum_pt.y) * sint) + datum_pt.x);
    dst.y = (int)ceil(((datum_pt.x - src.x) * sint) +  ((src.y - datum_pt.y) * cost) + datum_pt.y);
    return dst;
}

static int _get_point_from_zone(IGPOINT *pt, int *cnt, DvaBxZone *pz)
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

static int _get_name_position(IGPOINT *pt, int cnt, IGPOINT *dst)
{
    int i;
    int min_y, y_idx = 0;

    min_y = 1080 * 2;

    for (i = 0; i < cnt; ++i) {
        if(min_y > pt[i].y)
        {
            min_y = pt[i].y;
            y_idx = i;
        }
    }

    dst->x = pt[y_idx].x;
    dst->y = pt[y_idx].y;
    
    return 0;
}

static int _get_zone_value_position(IGPOINT *pt, int cnt, IGPOINT *dst)
{
    int i;
    int max_y, y_idx = 0;

    max_y = 0;

    for (i = 0; i < cnt; ++i) {
        if(max_y < pt[i].y)
        {
            max_y = pt[i].y;
            y_idx = i;
        }
    }

    dst->x = pt[y_idx].x;
    dst->y = pt[y_idx].y;
    
    return 0;
}

static int _get_cntr_value_position(IGPOINT *pt, int cnt, IGPOINT *dst)
{
    int i;

    dst->x = pt[0].x;
    dst->y = pt[0].y;

    for (i = 0; i < cnt; ++i) {
        if (pt[i].x < dst->x) dst->x = pt[i].x;
        if (pt[i].y < dst->y) dst->y = pt[i].y;        
    }
   
    return 0;
}

static int _get_calb_value_position(IGPOINT *pt, int cnt, IGPOINT *dst)
{
    int y_idx = 0;

    if (pt[0].y < pt[1].y)
    {
       y_idx = 1;
    }

    dst->x = pt[y_idx].x;
    dst->y = pt[y_idx].y;
    
    return 0;
}

static int _get_dir_position(IGPOINT pt1, IGPOINT pt2, IGPOINT *dst)
{
    GdkPoint arrow_pt[7] = {{-2,-6}, {-2, 3}, {-5, 3}, {0, 8}, {5, 3}, {2, 3}, {2,-6}};
    int i;

    for (i = 0; i < 7; i++) 
    {
        dst[i].x = 0;
        dst[i].y = 0;
        dst[i].dx = arrow_pt[i].x;
        dst[i].dy = arrow_pt[i].y;
    }
    
    return 0;
}

static int _get_calb_upper_position(IGPOINT pt1, IGPOINT pt2, IGPOINT *dst, int *width, int *height)
{
    float distance;
    float diameter;
    IGPOINT tmp;
    
    distance = sqrt(pow(pt1.x - pt2.x, 2) + pow(pt1.y - pt2.y, 2));
    diameter = distance * 0.15;

    tmp.x = pt1.x;
    tmp.y = pt1.y + (diameter/2);
    tmp = _rotate_point(tmp, pt1, (pt2.y-pt1.y)/distance, (pt2.x-pt1.x)/distance);

    dst->x = tmp.x - (diameter/2); 
    dst->y = tmp.y - (diameter/2);

    *width = diameter;
    *height = diameter;    
    return 0;
}

static int _get_calb_lower_position(IGPOINT pt1, IGPOINT pt2, IGPOINT *dst, int *cnt)
{
    int i;
    float distance;
    float diameter;
    
    distance = sqrt(pow(pt1.x - pt2.x, 2) + pow(pt1.y - pt2.y, 2));
    diameter = distance * 0.15;

    dst[0].x = pt1.x - distance * 0.13; 
    dst[0].y = pt1.y + distance * 0.19; 

    dst[1].x = pt1.x - (diameter / 2); 
    dst[1].y = pt1.y + diameter; 

    dst[2].x = pt1.x; 
    dst[2].y = pt1.y + (distance / 4); 

    dst[3].x = pt1.x + (diameter / 2); 
    dst[3].y = pt1.y + diameter; 

    dst[4].x = pt1.x + distance * 0.13; 
    dst[4].y = pt1.y + distance * 0.19; 

    dst[5].x = pt1.x + distance * 0.12; 
    dst[5].y = pt1.y + distance * 0.52; 

    dst[6].x = pt1.x + distance * 0.094; 
    dst[6].y = pt1.y + distance * 0.57; 

    dst[7].x = pt1.x + distance * 0.036; 
    dst[7].y = pt1.y + distance; 

    dst[8].x = pt1.x - distance * 0.036; 
    dst[8].y = pt1.y + distance; 

    dst[9].x = pt1.x - distance * 0.094; 
    dst[9].y = pt1.y + distance * 0.57; 

    dst[10].x = pt1.x - distance * 0.12; 
    dst[10].y = pt1.y + distance * 0.52; 

    for (i = 0; i < 11; i++)
        dst[i] = _rotate_point(dst[i], pt1, (pt2.y-pt1.y)/distance, (pt2.x-pt1.x)/distance);

    *cnt = 11;
    
    return 0;
}

static int _get_zone_dir_figure(DVAZONEPTR pzone, IGPOINT *figure, int *cnt)
{
    IGPOINT zone_pt[16] = {0,};
    int zone_cnt = 0;

    memset(zone_pt, 0x00, sizeof(zone_pt));

    _get_point_from_zone(zone_pt, &zone_cnt, &pzone->zn_data);
    _get_dir_position(zone_pt[0], zone_pt[1], figure);
    *cnt = 7;
    return 0;
}

static int _get_zone_figure(DVAZONEPTR pzone, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    *cnt = pzone->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pzone->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);   
    
    return 0;
}

static int _get_zone_name(DVAZONEPTR pzone, char *str)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    g_stpcpy(str, pzone->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _get_point_from_cntr(IGPOINT *pt, int *cnt, DvaBxCntr *pc)
{
    int i;
    
    for (i = 0; i < 4; ++i) {
        pt[i].x = pc->pt[i].x;
        pt[i].y = pc->pt[i].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }
    *cnt = 4;

    return 0;
}

static int _get_cntr_figure(DVACNTRPTR pcntr, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    *cnt = pcntr->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pcntr->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = dvaa_itx_convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    return 0;
}

static int _get_cntr_name(DVACNTRPTR pcntr, char *str)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    g_stpcpy(str, pcntr->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _get_point_from_calb(IGPOINT *pt, int *cnt, DvaBxCalb *pc)
{
    int i;
    
    for (i = 0; i < 2; ++i) {
        pt[i].x = pc->pt[i].x;
        pt[i].y = pc->pt[i].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }
    *cnt = 2;

    return 0;
}

static int _get_calb_figure(DVACALBPTR pcalb, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (!pcalb->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    *cnt = pcalb->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pcalb->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = pcalb->dic[FIGURE]->ctns.f.cl.i;      
    return 0;
}

static int _get_calb_icon_upper(DVACALBPTR pcalb, IGPOINT *pt, int *width, int *height, int *color_idx)
{    
    if (!pcalb->dic[CALB_ICUPP]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    *pt = pcalb->dic[CALB_ICUPP]->ctns.a.pt;
    *width = pcalb->dic[CALB_ICUPP]->ctns.a.w;
    *height = pcalb->dic[CALB_ICUPP]->ctns.a.h;    
    *color_idx = pcalb->dic[CALB_ICUPP]->ctns.a.cl.i; 
    return 0;
}

static int _get_calb_icon_lower(DVACALBPTR pcalb, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (!pcalb->dic[CALB_ICLOW]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    *cnt = pcalb->dic[CALB_ICLOW]->ctns.f.cnt;
    memcpy(pt, pcalb->dic[CALB_ICLOW]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = pcalb->dic[CALB_ICLOW]->ctns.f.cl.i;   
    
    return 0;
}

static int _get_calb_value(DVACALBPTR pcalb, char *str)
{
    if (!pcalb->dic[INFO]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    g_stpcpy(str, pcalb->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _set_zone_type(DVAZONEPTR pzone, int type)
{
    DMSG(9, "");

    pzone->type = 0;
    
    if (type == IVCA_RT_AREA)
    {
        pzone->type |= MASK_AREA;
    }
    else if (type == IVCA_RT_LINE)
    {
        pzone->type |= MASK_LINE;
        pzone->type |= MASK_DIRCTRL;
    }

    pzone->occupied = 1;
    pzone->blink_step = -1;
    pzone->event_time = -1;    
    return 0;
}

static int _set_zone_direction_to_dic(DVAZONEPTR pzone, int color_idx)
{
    IGPOINT zone_pt[16];
    IGPOINT dir_pt[16];
    int zone_cnt = 0;

    if (!pzone->dic[FIGURE]) return -1;

    memset(zone_pt, 0x00, sizeof(zone_pt));

    _get_point_from_zone(zone_pt, &zone_cnt, &pzone->zn_data);
    _get_dir_position(zone_pt[0], zone_pt[1], dir_pt);

    pzone->dic[DIRCTRL]->ctns.d.ln[0] = zone_pt[0];
    pzone->dic[DIRCTRL]->ctns.d.ln[1] = zone_pt[1];
    pzone->dic[DIRCTRL]->ctns.d.cnt = 7;
    memcpy(pzone->dic[DIRCTRL]->ctns.d.pt, dir_pt, sizeof(IGPOINT) * 7);
    
    if (pzone->event_time > 0) pzone->dic[DIRCTRL]->ctns.d.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[DIRCTRL]->ctns.d.cl.i = color_idx;
    return 0;
}

static int _set_zone_direction(DVAZONEPTR pzone, int color_idx)
{
    _set_zone_direction_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_name_to_dic(DVAZONEPTR pzone, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    int cnt;

    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _get_name_position(pt, cnt, &dst);

    memset(pzone->dic[INFO]->ctns.t.txt, 0x00, sizeof(pzone->dic[INFO]->ctns.t.txt));
    g_stpcpy(pzone->dic[INFO]->ctns.t.txt, str);

    pzone->dic[INFO]->ctns.t.pt.x = dst.x;
    pzone->dic[INFO]->ctns.t.pt.y = dst.y;

    pzone->dic[INFO]->ctns.t.pt.dx = 0;
    pzone->dic[INFO]->ctns.t.pt.dy = -5;

    pzone->dic[INFO]->ctns.t.layout_psx = 1;   
    pzone->dic[INFO]->ctns.t.layout_psy = -1;   

    if (pzone->event_time > 0) pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[INFO]->ctns.t.cl.i = color_idx;
    
    pzone->dic[INFO]->ctns.t.size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _set_zone_name_to_zndata(DVAZONEPTR pzone, char *str)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    strcpy(pzone->zn_data.name, str);
    return 0;
}

static int _set_zone_name(DVAZONEPTR pzone, char *str, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_name_to_dic(pzone, str, color_idx);
    _set_zone_name_to_zndata(pzone, str);
    return 0;
}

static int _set_zone_value_to_dic(DVAZONEPTR pzone, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    char strBuf[16];
    int cnt;

    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _get_zone_value_position(pt, cnt, &dst);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", pzone->cnt_value);
    
    memset(pzone->dic[COUNT_VALUE]->ctns.t.txt, 0x00, sizeof(pzone->dic[COUNT_VALUE]->ctns.t.txt));
    g_stpcpy(pzone->dic[COUNT_VALUE]->ctns.t.txt, strBuf);

    pzone->dic[COUNT_VALUE]->ctns.t.pt.x = dst.x;
    pzone->dic[COUNT_VALUE]->ctns.t.pt.y = dst.y;

    pzone->dic[COUNT_VALUE]->ctns.t.pt.dx = 0;
    pzone->dic[COUNT_VALUE]->ctns.t.pt.dy = 5;

    pzone->dic[COUNT_VALUE]->ctns.t.layout_psx = 1;   
    pzone->dic[COUNT_VALUE]->ctns.t.layout_psy = 1;   
    
    if (pzone->event_time > 0) pzone->dic[COUNT_VALUE]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[COUNT_VALUE]->ctns.t.cl.i = color_idx;
    
    pzone->dic[COUNT_VALUE]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_zone_value(DVAZONEPTR pzone, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_value_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_figure_to_dic(DVAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pzone->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    
    if (pzone->event_time > 0) pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);    
    else pzone->dic[FIGURE]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_zone_figure_to_zndata(DVAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    for (i = 0; i < cnt; ++i) {
        pzone->zn_data.pt[i].x = pt[i].x;
        pzone->zn_data.pt[i].y = pt[i].y;
    }

    pzone->zn_data.npts = cnt;    
    pzone->zn_data.color = dvaa_itx_convert_to_coloridx_rgb_color(color_idx);    
    return 0;
}

static int _set_zone_figure(DVAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    _set_zone_figure_to_dic(pzone, pt, cnt, color_idx);
    _set_zone_figure_to_zndata(pzone, pt, cnt, color_idx);
    return 0;
}

static int _set_cntr_type(DVACNTRPTR pcntr)
{
    DMSG(9, "");

    pcntr->type |= MASK_AREA;
    pcntr->type |= MASK_COUNT;
    pcntr->occupied = 1;
    pcntr->blink_step = -1;
    pcntr->event_time = -1;        
    return 0;
}

static int _set_cntr_name_to_dic(DVACNTRPTR pcntr, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    int cnt;

    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_name_position(pt, cnt, &dst);

    memset(pcntr->dic[INFO]->ctns.t.txt, 0x00, sizeof(pcntr->dic[INFO]->ctns.t.txt));
    g_stpcpy(pcntr->dic[INFO]->ctns.t.txt, str);

    pcntr->dic[INFO]->ctns.t.pt.x = dst.x;
    pcntr->dic[INFO]->ctns.t.pt.y = dst.y;

    pcntr->dic[INFO]->ctns.t.pt.dx = 0;
    pcntr->dic[INFO]->ctns.t.pt.dy = -36;

    pcntr->dic[INFO]->ctns.t.layout_psx = 1;    
    pcntr->dic[INFO]->ctns.t.layout_psy = -1;

    pcntr->dic[INFO]->ctns.t.cl.i = color_idx;
    pcntr->dic[INFO]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_cntr_name_to_ctdata(DVACNTRPTR pcntr, char *str)
{
    int i;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    strcpy(pcntr->ct_data.name, str);
    return 0;
}

static int _set_cntr_name(DVACNTRPTR pcntr, char *str, int color_idx)
{
    guint y;

    if (!pcntr->dic[FIGURE]) return -1;

    _set_cntr_name_to_dic(pcntr, str, color_idx);
    _set_cntr_name_to_ctdata(pcntr, str);
    return 0;
}

static int _set_cntr_value_to_dic(DVACNTRPTR pcntr, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    char strBuf[16];
    int cnt;

    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_cntr_value_position(pt, cnt, &dst);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", pcntr->cnt_value);
    
    memset(pcntr->dic[COUNT_VALUE]->ctns.t.txt, 0x00, sizeof(pcntr->dic[COUNT_VALUE]->ctns.t.txt));
    g_stpcpy(pcntr->dic[COUNT_VALUE]->ctns.t.txt, strBuf);

    pcntr->dic[COUNT_VALUE]->ctns.t.pt.x = dst.x;
    pcntr->dic[COUNT_VALUE]->ctns.t.pt.y = dst.y;

    pcntr->dic[COUNT_VALUE]->ctns.t.pt.dx = 10;
    pcntr->dic[COUNT_VALUE]->ctns.t.pt.dy = -18;

    pcntr->dic[COUNT_VALUE]->ctns.t.layout_psx = 1;   
    pcntr->dic[COUNT_VALUE]->ctns.t.layout_psy = 0;   

    pcntr->dic[COUNT_VALUE]->ctns.t.cl.i = color_idx;
    pcntr->dic[COUNT_VALUE]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_cntr_value(DVACNTRPTR pcntr, int color_idx)
{
    guint y;

    if (!pcntr->dic[FIGURE]) return -1;

    _set_cntr_value_to_dic(pcntr, color_idx);
    return 0;
}

static int _set_cntr_figure_to_dic(DVACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pcntr->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    pcntr->dic[FIGURE]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_cntr_figure_to_ctdata(DVACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    for (i = 0; i < cnt; ++i) {
        pcntr->ct_data.pt[i].x = pt[i].x;
        pcntr->ct_data.pt[i].y = pt[i].y;
    }

    pcntr->ct_data.color = dvaa_itx_convert_to_coloridx_rgb_color(color_idx);
    return 0;
}

static int _set_cntr_figure(DVACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    _set_cntr_figure_to_dic(pcntr, pt, cnt, color_idx);
    _set_cntr_figure_to_ctdata(pcntr, pt, cnt, color_idx);
    return 0;
}

static int _set_calb_type(DVACALBPTR pcalb)
{
    DMSG(9, "");

    pcalb->type = MASK_LINE;
    pcalb->type |= MASK_CALB;
    pcalb->occupied = 1;
    pcalb->blink_step = -1;
    pcalb->event_time = -1;
    return 0;
}

static int _set_calb_figure_to_dic(DVACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    if (!pcalb->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pcalb->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    pcalb->dic[FIGURE]->ctns.f.cl.i = color_idx;    
    return 0;
}

static int _set_calb_icon_upper_to_dic(DVACALBPTR pcalb, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    int cnt;
    int width, height;

    if (!pcalb->dic[CALB_ICUPP]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_upper_position(pt[0], pt[1], &dst, &width, &height);

    pcalb->dic[CALB_ICUPP]->ctns.a.pt = dst;
    pcalb->dic[CALB_ICUPP]->ctns.a.w = width;
    pcalb->dic[CALB_ICUPP]->ctns.a.h = height;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = color_idx;    
    return 0;
}

static int _set_calb_icon_lower_to_dic(DVACALBPTR pcalb, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst[MAX_PT] = {0,};
    int cnt;
    int width, height;

    if (!pcalb->dic[CALB_ICLOW]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_lower_position(pt[0], pt[1], dst, &cnt);

    pcalb->dic[CALB_ICLOW]->ctns.f.cnt = cnt;
    memcpy(pcalb->dic[CALB_ICLOW]->ctns.f.pt, dst, sizeof(IGPOINT) * cnt);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = color_idx;    
    return 0;
}

static int _set_calb_figure_to_cbdata(DVACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pcalb)) return -1;

    for (i = 0; i < cnt; ++i) {
        pcalb->cb_data.pt[i].x = pt[i].x;
        pcalb->cb_data.pt[i].y = pt[i].y;
    }

    return 0;
}

static int _set_calb_value_to_dic(DVACALBPTR pcalb, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT] = {0,};
    IGPOINT dst;
    int cnt;

    if (!pcalb->dic[INFO]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_value_position(pt, cnt, &dst);

    memset(pcalb->dic[INFO]->ctns.t.txt, 0x00, sizeof(pcalb->dic[INFO]->ctns.t.txt));
    g_stpcpy(pcalb->dic[INFO]->ctns.t.txt, str);

    pcalb->dic[INFO]->ctns.t.pt.x = dst.x;
    pcalb->dic[INFO]->ctns.t.pt.y = dst.y;

    pcalb->dic[INFO]->ctns.t.pt.dx = 0;
    pcalb->dic[INFO]->ctns.t.pt.dy = 5;

    pcalb->dic[INFO]->ctns.t.layout_psx = 0;
    pcalb->dic[INFO]->ctns.t.layout_psy = 1;

    pcalb->dic[INFO]->ctns.t.cl.i = color_idx;
    pcalb->dic[INFO]->ctns.t.size = NFFONT_MINI_SEMI_5;
    return 0;
}

static int _set_calb_value(DVACALBPTR pcalb, char *str, int color_idx)
{
    _set_calb_value_to_dic(pcalb, str, color_idx);
    return 0;
}

static int _set_calb_figure(DVACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    _set_calb_figure_to_dic(pcalb, pt, cnt, color_idx);
    _set_calb_figure_to_cbdata(pcalb, pt, cnt, color_idx);
    return 0;
}

static int _set_calb_icon_upper(DVACALBPTR pcalb, int color_idx)
{
    _set_calb_icon_upper_to_dic(pcalb, color_idx);
    return 0;
}

static int _set_calb_icon_lower(DVACALBPTR pcalb, int color_idx)
{
    _set_calb_icon_lower_to_dic(pcalb, color_idx);
    return 0;
}

static int _make_zone_direction_highlight(DVAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[DIRCTRL]->ctns.d.wi = 5;
    pzone->dic[DIRCTRL]->ctns.d.fi = 1;    
    return 0;
}

static int _make_zone_direction_lowlight(DVAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[DIRCTRL]->ctns.d.wi = 3;
    pzone->dic[DIRCTRL]->ctns.d.fi = 0;
    return 0;
}

static int _make_zone_highlight(DVAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[FIGURE]->ctns.f.wi = 5;
    return 0;
}

static int _make_zone_lowlight(DVAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[FIGURE]->ctns.f.wi = 3;
    return 0;
}

static int _make_zone_focus(DVAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 1;
    return 0;
}

static int _make_zone_unfocus(DVAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 0;
    return 0;
}

static int _make_cntr_highlight(DVACNTRPTR pcntr)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->sty_highlight = 1;
//    pcntr->dic[FIGURE]->ctns.f.vt = 1;
    pcntr->dic[FIGURE]->ctns.f.wi = 5;
    return 0;
}

static int _make_cntr_lowlight(DVACNTRPTR pcntr)
{   
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->sty_highlight = 0;
//    pcntr->dic[FIGURE]->ctns.f.vt = 0;
    pcntr->dic[FIGURE]->ctns.f.wi = 3;
    return 0;
}

static int _make_calb_highlight(DVACALBPTR pcalb)
{
    if (!pcalb->dic[FIGURE]) return -1;
    if (!pcalb->dic[CALB_ICUPP]) return -1;
    if (!pcalb->dic[CALB_ICLOW]) return -1;
    if (!pcalb->dic[INFO]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->sty_highlight = 1;
    pcalb->dic[FIGURE]->ctns.f.vt = 1;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    pcalb->dic[INFO]->ctns.t.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);    
    return 0;
}

static int _make_calb_lowlight(DVACALBPTR pcalb)
{   
    if (!pcalb->dic[FIGURE]) return -1;
    if (!pcalb->dic[CALB_ICUPP]) return -1;
    if (!pcalb->dic[CALB_ICLOW]) return -1;
    if (!pcalb->dic[INFO]) return -1;
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->sty_highlight = 0;
    pcalb->dic[FIGURE]->ctns.f.vt = 0;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    pcalb->dic[INFO]->ctns.t.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);    
    return 0;
}

static int _apply_rule_prop(ITXDVAA_T *pdvaa, ITX_DVARULE_PROP *prop)
{
//    pdvaa->calbres.paramvalid = res->paramvalid;
    pdvaa->prop.en_engine = prop->en_engine;
    pdvaa->prop.unit = prop->unit;
    pdvaa->prop.en_shadowrm = prop->en_shadowrm;
    pdvaa->prop.track_ref = prop->track_ref;
    pdvaa->prop.en_usecalib = prop->en_usecalib;
    pdvaa->prop.min_width3d = prop->min_width3d;
    pdvaa->prop.min_height3d = prop->min_height3d;
    pdvaa->prop.en_static_filter = prop->en_static_filter;
    pdvaa->prop.static_filter_sense = prop->static_filter_sense;
    pdvaa->prop.sw_obj_bb = prop->sw_obj_bb;
    pdvaa->prop.sw_obj_tr = prop->sw_obj_tr;
    pdvaa->prop.sw_obj_id = prop->sw_obj_id;
    pdvaa->prop.sw_obj_w3d = prop->sw_obj_w3d;
    pdvaa->prop.sw_obj_h3d = prop->sw_obj_h3d;
    pdvaa->prop.sw_obj_s3d = prop->sw_obj_s3d;
    pdvaa->prop.sw_rule = prop->sw_rule;    

    return 0;
}

static int _bring_rule_prop(ITXDVAA_T *pdvaa, ITX_DVARULE_PROP *prop)
{
//    res->paramvalid = pdvaa->calbres.paramvalid;
    prop->en_engine = pdvaa->prop.en_engine;
    prop->unit = pdvaa->prop.unit;
    prop->en_shadowrm = pdvaa->prop.en_shadowrm;
    prop->track_ref = pdvaa->prop.track_ref;
    prop->en_usecalib = pdvaa->prop.en_usecalib;
    prop->min_width3d = pdvaa->prop.min_width3d;
    prop->min_height3d = pdvaa->prop.min_height3d;
    prop->en_static_filter = pdvaa->prop.en_static_filter;
    prop->static_filter_sense = pdvaa->prop.static_filter_sense;
    prop->sw_obj_bb = pdvaa->prop.sw_obj_bb;
    prop->sw_obj_tr = pdvaa->prop.sw_obj_tr;
    prop->sw_obj_id = pdvaa->prop.sw_obj_id;
    prop->sw_obj_w3d = pdvaa->prop.sw_obj_w3d;
    prop->sw_obj_h3d = pdvaa->prop.sw_obj_h3d;
    prop->sw_obj_s3d = pdvaa->prop.sw_obj_s3d;
    prop->sw_rule = pdvaa->prop.sw_rule; 

    return 0;
}

static int _update_zone_count(ITXDVAA_T *pdvaa)
{
    int id;
    DVAZONEPTR pzone;

    pdvaa->cnt_zone = 0;

    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, id);
        if (pzone->use) pdvaa->cnt_zone++;
    }

    return 0;
}

static int _apply_zone_conf(ITX_DVAZONE_CONF *conf, DVAZONEPTR pzone)
{
    int i;

    pzone->use = conf->use_zone;
    pzone->focus = conf->focus;
    
    pzone->zn_data.id = pzone->id;    
    pzone->zn_data.active = conf->active;

    if (_IS_LINE(pzone)) pzone->zn_data.type = IVCA_RT_LINE;
    else pzone->zn_data.type = IVCA_RT_AREA;

    pzone->dic[FIGURE]->en = conf->use_zone & conf->active;
    pzone->dic[INFO]->en = conf->use_zone & conf->active;
    
    if (pzone->value_en) pzone->dic[COUNT_VALUE]->en = conf->use_zone & conf->active;
    else pzone->dic[COUNT_VALUE]->en = 0;
    
    if (_IS_LINE(pzone)) pzone->dic[DIRCTRL]->en = conf->use_zone & conf->active;
    else pzone->dic[DIRCTRL]->en = 0;

    pzone->zn_data.all_detect_obj = conf->all_detect_obj;

    memset(pzone->zn_data.interest_obj, 0x00, sizeof(pzone->zn_data.interest_obj));
    snprintf(pzone->zn_data.interest_obj, sizeof(pzone->zn_data.interest_obj)-1, "%s", conf->interest_obj);

    pzone->zn_data.enabled = 0;

    if(conf->forward)   pzone->zn_data.enabled |= IVCA_ET_DIR_POS;
    if(conf->reverse)   pzone->zn_data.enabled |= IVCA_ET_DIR_NEG;
    if(conf->intrusion) pzone->zn_data.enabled |= IVCA_ET_INTRUSION;
    if(conf->enter)     pzone->zn_data.enabled |= IVCA_ET_ENTER;
    if(conf->exit)      pzone->zn_data.enabled |= IVCA_ET_EXIT;
    if(conf->stopped)   pzone->zn_data.enabled |= IVCA_ET_STOPPED;
    if(conf->removed)   pzone->zn_data.enabled |= IVCA_ET_REMOVED;
    if(conf->loitering) pzone->zn_data.enabled |= IVCA_ET_LOITERED;
    if(conf->use_filter_color)  pzone->zn_data.enabled |= IVCA_ET_COLOR;
    if(conf->use_filter_size)   pzone->zn_data.enabled |= IVCA_ET_SIZE;
    if(conf->use_filter_speed)  pzone->zn_data.enabled |= IVCA_ET_SPEED;

    pzone->zn_data.loiter_time = conf->cfg_loiter_time;
    pzone->zn_data.stop_time = conf->cfg_stop_time;
    pzone->zn_data.remove_time = conf->cfg_remove_time;

    pzone->zn_data.ecolor = dvaa_itx_convert_to_coloridx_rgb_color(conf->filter_color_idx);
    pzone->zn_data.ecolor_sens = conf->filter_color_prct;
    pzone->zn_data.size_min[0] = conf->filter_width_from;
    pzone->zn_data.size_max[0] = conf->filter_width_to;
    pzone->zn_data.size_min[1] = conf->filter_height_from;
    pzone->zn_data.size_max[1] = conf->filter_height_to;
    pzone->zn_data.speed_min = conf->filter_speed_from;
    pzone->zn_data.speed_max = conf->filter_speed_to;
    
    pzone->zn_data.c_threshold = conf->c_threshold;

    memset(pzone->zn_data.event_audio, 0x00, sizeof(pzone->zn_data.event_audio));

    for (i=0; i<8; i++)
    {
        snprintf(pzone->zn_data.event_audio[i], sizeof(pzone->zn_data.event_audio[i])-1, "%s", conf->event_audio[i]);
    }

    if (conf->focus) 
    {
        _make_zone_highlight(pzone);
        _make_zone_focus(pzone);
        if (_IS_DIRCTRL(pzone)) _make_zone_direction_highlight(pzone);      
    }
    else
    {
        _make_zone_lowlight(pzone);
        _make_zone_unfocus(pzone);
        if (_IS_DIRCTRL(pzone)) _make_zone_direction_lowlight(pzone);
    }

    return 0;
}

static int _bring_zone_conf(DVAZONEPTR pzone, ITX_DVAZONE_CONF *conf)
{
    int i;

    conf->zoneid = pzone->id;
    conf->type = pzone->zn_data.type;
    conf->use_zone = pzone->use;   
    conf->focus = pzone->focus;
    
    conf->active = pzone->zn_data.active;  

    conf->all_detect_obj = pzone->zn_data.all_detect_obj;

    memset(conf->interest_obj, 0x00, sizeof(conf->interest_obj));
    snprintf(conf->interest_obj, sizeof(conf->interest_obj)-1, "%s", pzone->zn_data.interest_obj);

    if (pzone->zn_data.enabled & IVCA_ET_DIR_POS) conf->forward = 1;
    else conf->forward = 0;
    
    if(pzone->zn_data.enabled & IVCA_ET_DIR_NEG) conf->reverse = 1;
    else    conf->reverse = 0;

    if(pzone->zn_data.enabled & IVCA_ET_INTRUSION) conf->intrusion = 1; 
    else conf->intrusion = 0;

    if(pzone->zn_data.enabled & IVCA_ET_ENTER) conf->enter = 1; 
    else conf->enter = 0;

    if(pzone->zn_data.enabled & IVCA_ET_EXIT) conf->exit = 1;
    else conf->exit= 0;

    if(pzone->zn_data.enabled & IVCA_ET_STOPPED) conf->stopped = 1;
    else conf->stopped = 0;

    if(pzone->zn_data.enabled & IVCA_ET_REMOVED) conf->removed = 1;
    else conf->removed= 0;

    if(pzone->zn_data.enabled & IVCA_ET_LOITERED) conf->loitering = 1;
    else conf->loitering = 0;

    if(pzone->zn_data.enabled & IVCA_ET_COLOR) conf->use_filter_color = 1;
    else conf->use_filter_color = 0;

    if(pzone->zn_data.enabled & IVCA_ET_SIZE) conf->use_filter_size = 1;
    else conf->use_filter_size = 0;

    if(pzone->zn_data.enabled & IVCA_ET_SPEED) conf->use_filter_speed = 1;
    else conf->use_filter_speed =0;
    
    conf->cfg_stop_time = pzone->zn_data.stop_time;
    conf->cfg_remove_time = pzone->zn_data.remove_time;  
    conf->cfg_loiter_time = pzone->zn_data.loiter_time;

    conf->filter_color_idx = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.ecolor);
    conf->filter_color_prct = pzone->zn_data.ecolor_sens;
    conf->filter_width_from = pzone->zn_data.size_min[0];
    conf->filter_width_to = pzone->zn_data.size_max[0];
    conf->filter_height_from = pzone->zn_data.size_min[1];
    conf->filter_height_to = pzone->zn_data.size_max[1];
    conf->filter_speed_from = pzone->zn_data.speed_min;
    conf->filter_speed_to = pzone->zn_data.speed_max;
    
    conf->c_threshold = pzone->zn_data.c_threshold;

    memset(conf->event_audio, 0x00, sizeof(conf->event_audio));

    for (i=0; i<8; i++)
    {
        snprintf(conf->event_audio[i], sizeof(conf->event_audio[i])-1, "%s", pzone->zn_data.event_audio[i]);
    }

    return 0;
}

static int _bring_zone_confs_all(ITXDVAA_T *pdvaa, ITX_DVAZONE_CONF conf[32], int *cnt)
{
    int i;
    DVAZONEPTR pzone;

    for (i = 0; i < MAX_ZONE; ++i) 
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, (ITX_ZONEID)i);
        _bring_zone_conf(pzone, &conf[i]);
    }

    if (cnt) *cnt = pdvaa->cnt_zone;

    return 0;
}

static int _update_cntr_count(ITXDVAA_T *pdvaa)
{
    int id;
    DVACNTRPTR pcntr;

    pdvaa->cnt_cntr = 0;

    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, id);    
        if (pcntr->use) pdvaa->cnt_cntr++;
    }

    return 0;
}

static int _apply_cntr_conf(ITX_DVACNTR_CONF *conf, DVACNTRPTR pcntr)
{
    pcntr->use = conf->use_cntr;
    pcntr->focus = conf->focus;

    pcntr->ct_data.id = pcntr->id;    
    pcntr->ct_data.type = IVCA_RT_CNTR;        
    pcntr->ct_data.active = conf->active;

    pcntr->dic[FIGURE]->en = conf->use_cntr & conf->active;
    pcntr->dic[INFO]->en = conf->use_cntr & conf->active;
    
    if (pcntr->value_en) pcntr->dic[COUNT_VALUE]->en = conf->use_cntr & conf->active;
    else pcntr->dic[COUNT_VALUE]->en = 0;
    
    if(conf->use_counter_event)   pcntr->ct_data.enabled |= IVCA_ET_COUNTER;
    else    pcntr->ct_data.enabled &= ~IVCA_ET_COUNTER;
    
    pcntr->ct_data.resetalert = conf->use_reset_value;

    pcntr->ct_data.evalue = conf->counter_event_val;
    pcntr->ct_data.zid_up = conf->source_up;
    pcntr->ct_data.zid_dn = conf->source_down;

    if (conf->focus) _make_cntr_highlight(pcntr);
    else _make_cntr_lowlight(pcntr);

    return 0;
}

static int _bring_cntr_conf(DVACNTRPTR pcntr, ITX_DVACNTR_CONF *conf)
{
    conf->cntrid = pcntr->id;    
    conf->use_cntr = pcntr->use;
    conf->focus = pcntr->focus;
    
    conf->active = pcntr->ct_data.active;

    if (pcntr->ct_data.enabled & IVCA_ET_COUNTER) conf->use_counter_event = 1;
    else conf->use_counter_event = 0;

    conf->use_reset_value = pcntr->ct_data.resetalert;

    conf->counter_event_val = pcntr->ct_data.evalue;
    conf->source_up = pcntr->ct_data.zid_up;
    conf->source_down = pcntr->ct_data.zid_dn;

    return 0;
}

static int _bring_cntr_confs_all(ITXDVAA_T *pdvaa, ITX_DVACNTR_CONF conf[32], int *cnt)
{
    int i;
    DVACNTRPTR pcntr;

    for (i = 0; i < MAX_CNTR; ++i) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, (ITX_CNTRID)i);
        _bring_cntr_conf(pcntr, &conf[i]);
    }

    if (cnt) *cnt = pdvaa->cnt_cntr;

    return 0;
}

static int _update_calb_count(ITXDVAA_T *pdvaa)
{
    int id;
    DVACALBPTR pcalb;

    pdvaa->cnt_calb = 0;

    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, id);
        if (pcalb->use) pdvaa->cnt_calb++;
    }

    return 0;
}

static int _apply_calb_conf(ITX_DVACALB_CONF *conf, DVACALBPTR pcalb)
{
    if (!pcalb->dic[FIGURE]) return -1;
    if (!pcalb->dic[CALB_ICUPP]) return -1;
    if (!pcalb->dic[CALB_ICLOW]) return -1;
    if (!pcalb->dic[INFO]) return -1;
    
    pcalb->use = conf->use_calb;
    pcalb->focus = conf->focus;
    pcalb->cb_data.height = conf->height;
    
    pcalb->dic[FIGURE]->en = conf->use_calb;
    pcalb->dic[CALB_ICUPP]->en = conf->use_calb;
    pcalb->dic[CALB_ICLOW]->en = conf->use_calb;
    pcalb->dic[INFO]->en = conf->use_calb;
    
    if (conf->focus) _make_calb_highlight(pcalb);
    else _make_calb_lowlight(pcalb);

    return 0;
}

static int _bring_calb_conf(DVACALBPTR pcalb, ITX_DVACALB_CONF *conf)
{
    conf->calbid = pcalb->id;    
    conf->use_calb = pcalb->use;
    conf->focus = pcalb->focus;    

    conf->height = pcalb->cb_data.height;
    return 0;
}

static int _bring_calb_confs_all(ITXDVAA_T *pdvaa, ITX_DVACALB_CONF conf[32], int *cnt)
{
    int i;
    DVACALBPTR pcalb;

    for (i = 0; i < MAX_CALB; ++i) 
    {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, (ITX_CALBID)i);
        _bring_calb_conf(pcalb, &conf[i]);
    }

    if (cnt) *cnt = pdvaa->cnt_calb;

    return 0;
}

static int _apply_calb_res(ITXDVAA_T *pdvaa, ITX_DVACALB_RESULT *res)
{
    pdvaa->calbres.paramvalid = res->paramvalid;
    pdvaa->calbres.focal = res->focal;
    pdvaa->calbres.height = res->cam_height;
    pdvaa->calbres.tilt = res->cam_tilt;
    pdvaa->calbres.p_width = res->p_width;
    pdvaa->calbres.p_height = res->p_height;

    return 0;
}

static int _bring_calb_res(ITXDVAA_T *pdvaa, ITX_DVACALB_RESULT *res)
{
    res->paramvalid = pdvaa->calbres.paramvalid;
    res->focal = pdvaa->calbres.focal;
    res->cam_height = pdvaa->calbres.height;
    res->cam_tilt = pdvaa->calbres.tilt;
    res->p_width = pdvaa->calbres.p_width;
    res->p_height = pdvaa->calbres.p_height;

    return 0;
}

static int _apply_eosd_conf(ITXDVAA_T *pdvaa, ITX_DVAEOSD_CONF *conf)
{
    pdvaa->eosd.display_mode = conf->display_mode;
    pdvaa->eosd.object_color = conf->object_color;
    pdvaa->eosd.rule_color = conf->rule_color;
    pdvaa->eosd.event_color = conf->event_color;
    pdvaa->eosd.line_width = conf->line_width;
    pdvaa->eosd.line_transparency = conf->line_transparency;
    strcpy(pdvaa->eosd.object_type, conf->object_type);

    return 0;
}

static int _bring_eosd_conf(ITXDVAA_T *pdvaa, ITX_DVAEOSD_CONF *conf)
{
    conf->display_mode = pdvaa->eosd.display_mode;
    conf->object_color = pdvaa->eosd.object_color;
    conf->rule_color = pdvaa->eosd.rule_color;
    conf->event_color = pdvaa->eosd.event_color;
    conf->line_width = pdvaa->eosd.line_width;
    conf->line_transparency = pdvaa->eosd.line_transparency;
    strcpy(conf->object_type, pdvaa->eosd.object_type);

    return 0;
}

static int _init_zone_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    DVAZONEPTR pzone;
    DvaBxZone *zone_data;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 0;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(DvaBxZone));
    } 
    
    return 0;
}

static int _init_zone_pb_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    DVAZONEPTR pzone;
    DvaBxZone *zone_data;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 1;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(DvaBxZone));
    } 
    
    return 0;
}

static int _init_cntr_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    DVACNTRPTR pcntr;
    DvaBxCntr *cntr_data;
    
    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, id);
        
        _init_cntr(pcntr);        

        pcntr->use = 0;
        pcntr->value_en = 1;
        pcntr->id = id;
        _set_cntr_type(pcntr);
        memset(&pcntr->ct_data, 0x00, sizeof(DvaBxCntr));
    }
    
    return 0;
}

static int _reload_cntr_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    DVACNTRPTR pcntr;
    DvaBxCntr *cntr_data;

    int pre_value;
    
    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, id);
        pre_value = pcntr->cnt_value;
        
        _init_cntr(pcntr);        

        pcntr->use = 0;
        pcntr->value_en = 1;
        pcntr->id = id;
        pcntr->cnt_value = pre_value;
        _set_cntr_type(pcntr);
        memset(&pcntr->ct_data, 0x00, sizeof(DvaBxCntr));
    }
    
    return 0;
}

static int _init_calb_data(ITXDVAA_T *pdvaa)
{
    int id;      
    
    DVACALBPTR pcalb;
    DvaBxCalb *calb_data;
    
    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, id);
        
        _init_calb(pcalb);        

        pcalb->use = 0;
        pcalb->id = id;
        _set_calb_type(pcalb);
        memset(&pcalb->cb_data, 0x00, sizeof(DvaBxCalb));
    }
    
    return 0;
}

static int _init_pb_default_data(ITXDVAA_T *pdvaa)
{
    pdvaa->prop.active = TRUE;
    pdvaa->prop.sw_rule = TRUE;
    pdvaa->prop.sw_obj_bb = TRUE;
    pdvaa->prop.sw_obj_id = TRUE;
    pdvaa->prop.sw_obj_w3d = TRUE;
    pdvaa->prop.sw_obj_h3d = TRUE;
    pdvaa->prop.sw_obj_s3d = TRUE;
    pdvaa->prop.sw_obj_tr = TRUE;
    return 0;
}

static int _translate_vcdata_into_rule(ITXDVAA_T *pdvaa, DvaBxData *pdb)
{
    int id;
       
    DVAZONEPTR pzone;
    DvaBxZone *zone_data;
    
    DVACNTRPTR pcntr;
    DvaBxCntr *cntr_data;
    gint cnt;
    
    for (id = 0; id < pdb->zonelist.cnt; ++id) 
    {   
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, pdb->zonelist.zone[id].id);
        zone_data = &pdb->zonelist.zone[id];

        pzone->use = 1;
        _load_zone_data(zone_data, pzone);
        _set_zone_type(pzone, zone_data->type);
    } 

    cnt = pdb->zonelist.cnt;
    for(id = 0; id < MAX_ZONE; id++)
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, id);

        if(!pzone->use)
        {
            zone_data = &pdb->zonelist.zone[cnt++];
            _load_zone_data(zone_data, pzone);
            _set_zone_type(pzone, zone_data->type);
        }
    }
    pdvaa->cnt_zone = pdb->zonelist.cnt;

    for (id = 0; id < pdb->cntrlist.cnt; ++id) 
    {    
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, pdb->cntrlist.cntr[id].id);
        cntr_data = &pdb->cntrlist.cntr[id];
        
        pcntr->use = 1;
        _load_cntr_data(cntr_data, pcntr);
        _set_cntr_type(pcntr);
    }

    cnt = pdb->cntrlist.cnt;
    for(id = 0; id < MAX_CNTR; id++)
    {    
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, pdb->cntrlist.cntr[id].id);

        if(!pcntr->use)
        {
            pcntr->use = 0;
            cntr_data = &pdb->cntrlist.cntr[++cnt];
            _load_cntr_data(cntr_data, pcntr);
            _set_cntr_type(pcntr);
        }
    }

    pdvaa->cnt_cntr = pdb->cntrlist.cnt;

    return 0;
}

static int _translate_vcdata_into_calb(ITXDVAA_T *pdvaa, DvaBxData *pdb)
{
    int id;       
    
    DVACALBPTR pcalb;
    DvaBxCalb *calb_data;

    for (id = 0; id < pdb->calblist.cnt; ++id) 
    {   
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, id);

        if (id < pdb->calblist.cnt) pcalb->use = 1;
        else pcalb->use = 0;      

        calb_data = &pdb->calblist.calb[id];    
        _load_calb_data(calb_data, pcalb);
        _set_calb_type(pcalb);
    }

    pdvaa->cnt_calb = pdb->calblist.cnt;

    return 0;
}


static int _translate_rule_into_vcdata(ITXDVAA_T *pdvaa, DvaBxData *pdb)
{
    int id, cnt;
       
    DVAZONEPTR pzone;
    DVACNTRPTR pcntr;

    DvaBxZone *zone_data;
    DvaBxCntr *cntr_data;

    cnt = 0;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, id);
        if (pdvaa->db.zonelist.zone[id].c_threshold == 0) {
            pdvaa->db.zonelist.zone[id].c_threshold = 1;
        }
        
        if (!pzone->use) continue;

        zone_data = &pdb->zonelist.zone[cnt++];
        _unload_zone_data(pzone, zone_data);        
    }

    cnt = 0;

    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, id);
        if (!pcntr->use) continue;
    
        cntr_data = &pdb->cntrlist.cntr[cnt++];
        _unload_cntr_data(pcntr, cntr_data);
    }

    pdb->zonelist.cnt = pdvaa->cnt_zone;
    pdb->cntrlist.cnt = pdvaa->cnt_cntr;
    
    return 0;
}

static int _translate_calb_into_vcdata(ITXDVAA_T *pdvaa, DvaBxData *pdb)
{
    int id, cnt;
       
    DVACALBPTR pcalb;
    DvaBxCalb *calb_data;

    cnt = 0;    

    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, id);
        if (!pcalb->use) continue;
    
        calb_data = &pdb->calblist.calb[cnt++];
        _unload_calb_data(pcalb, calb_data);
    }

    pdb->calblist.cnt = pdvaa->cnt_calb;
    
    return 0;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _dvaa_itx_detector_init(ITXDVAA_T *pdvaa)
{
    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    _init_cntr_data(pdvaa);    
    _init_calb_data(pdvaa);
    _translate_vcdata_into_rule(pdvaa, &pdvaa->db);
    _translate_vcdata_into_calb(pdvaa, &pdvaa->db);    
    _copy_db_to_prop(&pdvaa->db, &pdvaa->prop);
    _copy_db_to_calbres(&pdvaa->db, &pdvaa->calbres);
    _copy_db_to_eosd(&pdvaa->db, &pdvaa->eosd);
    return 0;
}

int _dvaa_itx_detector_pb_init(ITXDVAA_T *pdvaa)
{
    _init_zone_pb_data(pdvaa);
    _init_pb_default_data(pdvaa);
    return 0;
}

int _dvaa_itx_detector_reload(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    //_init_cntr_data(pdvaa);    
    _reload_cntr_data(pdvaa);
    _init_calb_data(pdvaa);
    _translate_vcdata_into_rule(pdvaa, &pdvaa->db);
    _translate_vcdata_into_calb(pdvaa, &pdvaa->db);
    _copy_db_to_prop(&pdvaa->db, &pdvaa->prop);
    _copy_db_to_calbres(&pdvaa->db, &pdvaa->calbres);
    _copy_db_to_eosd(&pdvaa->db, &pdvaa->eosd);
    return 0;
}

int _dvaa_itx_detector_load_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    _init_cntr_data(pdvaa);    
    _init_calb_data(pdvaa);    
    _translate_vcdata_into_rule(pdvaa, &pdvaa->db);
    _translate_vcdata_into_calb(pdvaa, &pdvaa->db);
    _copy_db_to_prop(&pdvaa->db, &pdvaa->prop);
    _copy_db_to_calbres(&pdvaa->db, &pdvaa->calbres);
    _copy_db_to_eosd(&pdvaa->db, &pdvaa->eosd);
    return 0;
}

int _dvaa_itx_detector_save_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    memset(&pdvaa->db, 0x00, sizeof(DvaBxData));
    _copy_prop_to_db(&pdvaa->prop, &pdvaa->db);
    _copy_calbres_to_db(&pdvaa->calbres, &pdvaa->db);    
    _copy_eosd_to_db(&pdvaa->eosd, &pdvaa->db);
    _translate_rule_into_vcdata(pdvaa, &pdvaa->db);
    _translate_calb_into_vcdata(pdvaa, &pdvaa->db);
    _save_db(pdvaa);
    return 0;
}

int _dvaa_itx_detector_is_db_changed(DVAAID id)
{
    DvaBxData tmpdb;  
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return 0;

    memset(&tmpdb, 0x00, sizeof(DvaBxData));
    memmove(&tmpdb, &pdvaa->db, sizeof(DvaBxData));
    _copy_prop_to_db(&pdvaa->prop, &tmpdb);
    _copy_calbres_to_db(&pdvaa->calbres, &tmpdb);
    _copy_eosd_to_db(&pdvaa->eosd, &tmpdb);
    _translate_rule_into_vcdata(pdvaa, &tmpdb);  
    _translate_calb_into_vcdata(pdvaa, &tmpdb);    
//  DMSG(1, "%d", (memcmp(&pdvaa->db, &tmpdb, sizeof(DvaBxData))));
    return (memcmp(&pdvaa->db, &tmpdb, sizeof(DvaBxData)) != 0);
}



////////////////////////////////////////////////////////////
//
// public interfaces 
//

int dvaa_itx_detector_add_zone_line_default_template(DVAAID id, ITX_ZONEID zone)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_DVAZONE_CONF conf;
    ITX_DVAZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_DVAZONE_SHAPE));    

    pzone = _dvaa_itx_get_zone_ptr((ITXDVAA_T*)id, zone);
    _set_zone_type(pzone, IVCA_RT_LINE);
    
    conf.use_zone = 1;
    _apply_zone_conf(&conf, pzone);
    _update_zone_count(pdvaa);
    
    shape.ptcnt = 2;    
    shape.pt[0].x = 1440;
    shape.pt[0].y = 1260;
    shape.pt[1].x = 2400;
    shape.pt[1].y = 900;
    
    _set_zone_figure(pzone, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_FF003F));
    if (_IS_DIRCTRL(pzone)) _set_zone_direction(pzone, COLOR_PRG_IDX(UX_COLOR_FF003F));

    _set_zone_name(pzone, " ", COLOR_PRG_IDX(UX_COLOR_FF003F));  
    _set_zone_value(pzone, COLOR_PRG_IDX(UX_COLOR_FF003F));
    return 0;
}

int dvaa_itx_detector_add_zone_area_default_template(DVAAID id, ITX_ZONEID zone)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_DVAZONE_CONF conf;
    ITX_DVAZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_DVAZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_DVAZONE_SHAPE));    

    pzone = _dvaa_itx_get_zone_ptr((ITXDVAA_T*)id, zone);
    _set_zone_type(pzone, IVCA_RT_AREA);

    conf.use_zone = 1;
    _apply_zone_conf(&conf, pzone);
    _update_zone_count(pdvaa);

    shape.ptcnt = 4;    
    shape.pt[0].x = 1440;
    shape.pt[0].y = 1260;
    shape.pt[1].x = 1440;
    shape.pt[1].y = 900;    
    shape.pt[2].x = 2400;
    shape.pt[2].y = 900;    
    shape.pt[3].x = 2400;
    shape.pt[3].y = 1260;
    _set_zone_figure(pzone, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_FF003F));

    _set_zone_name(pzone, " ", COLOR_PRG_IDX(UX_COLOR_FF003F));    
    _set_zone_value(pzone, COLOR_PRG_IDX(UX_COLOR_FF003F));
    return 0;
}

int dvaa_itx_detector_get_zone_shape(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_SHAPE *shape)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;

    if (!pdvaa) return -1;

    DMSG(9, "zoneid = %d\n", zone);
    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    _get_zone_figure(pzone, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_zone_dir_figure(pzone, &shape->dir_pt, &shape->dir_ptcnt);
    _get_zone_name(pzone, shape->name);       
    return 0;
}

int dvaa_itx_detector_set_zone_shape(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_SHAPE *shape)
{
    DVAZONEPTR pzone;
    int ret = 0;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (shape->ptcnt > MAX_PT) return -1;

    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    if (_IS_LINE(pzone) && shape->ptcnt > 2) return -1;
    if (_IS_AREA(pzone) && shape->ptcnt < 3) return -2;
    if (_IS_AREA(pzone) && shape->ptcnt > MAX_PT_ITX) return -3;

    _set_zone_figure(pzone, shape->pt, shape->ptcnt, shape->color_idx);
    if (_IS_DIRCTRL(pzone)) _set_zone_direction(pzone, shape->color_idx);
    _set_zone_name(pzone, shape->name, shape->color_idx);
    _set_zone_value(pzone, shape->color_idx);
    return 0;
}

int dvaa_itx_detector_get_zone_conf(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_CONF *conf)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    _bring_zone_conf(pzone, conf);
    return 0;
}

int dvaa_itx_detector_get_zone_confs_all(DVAAID id, ITX_DVAZONE_CONF conf[32], int *cnt)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    *cnt = 0;
    if (!pdvaa) return -1;

    _bring_zone_confs_all(pdvaa, conf, cnt);
    return 0;
}

int dvaa_itx_detector_set_zone_conf(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_CONF *conf)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    _apply_zone_conf(conf, pzone);
    _update_zone_count(pdvaa);
    return 0;
}

int dvaa_itx_detector_get_zone_value(DVAAID id, ITX_ZONEID zone, int *en, int *color_idx, int *count)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    *en = pzone->dic[COUNT_VALUE]->en;
    *color_idx = pzone->dic[COUNT_VALUE]->ctns.t.cl.i;    
    *count = pzone->cnt_value;
    return 0;
}

int dvaa_itx_detector_set_zone_value(DVAAID id, ITX_ZONEID zone, int en, int color_idx, int count)
{
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
    pzone->dic[COUNT_VALUE]->en = en;
    pzone->cnt_value = count;
    _set_zone_value(pzone, color_idx);
    return 0;
}

ITX_ZONEID dvaa_itx_detector_find_zone(DVAAID id, int x, int y, float scale_x, float scale_y)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    DVAZONEPTR pzone;
    ITX_ZONEID zone;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    IGPOINT line_pt[2];
    ITX_ZONEID tmp_rule[MAX_ZONE];
    int tmp_area[MAX_ZONE]; 
    int rcnt = 0;
    IXSQUARE sq;
    int min = 0;
    int i;
    int fcnt = 0;
    int color_idx;
    IXPOLYGON p;

    int mptx, mpty;
    int dx, dy;
    int dir_x, dir_y;
    float l;

    if (!pdvaa) return -1;
    if (!dit_page_is_enabled(pdvaa->ditid, pdvaa->dvazone[0].dipage)) return -1;

    mpt.x = x;
    mpt.y = y;
    
    rcnt = 0;
    
    memset(&p, 0x00, sizeof(IXPOLYGON));
    memset(tmp_rule, 0x00, sizeof(tmp_rule));
    memset(tmp_area, 0x00, sizeof(tmp_area));
    memset(&sq, 0x00, sizeof(IGBOX));
    
    for (zone = MAX_ZONE-1; zone >= 0; zone--) 
	{
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, (ITX_ZONEID)zone);
        if (!pzone) continue;
        if (!pzone->use) continue;

        if (_get_zone_figure(pzone, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt, scale_x, scale_y) == 0) 
		{
            _make_polygon(fpt, fcnt, &p);
            if (ifn_point_is_over_line(mpt.x, mpt.y, MAX_ZONE, &p)) return zone;
            
            tmp_rule[rcnt] = zone;
            ifn_get_outbound_square(&p, &sq);
            tmp_area[rcnt] = ifn_get_area(&sq);
            rcnt++;
        }
        else
        {
            if (_IS_DIRCTRL(pzone)) 
            {
                line_pt[0].x = (int)(pzone->dic[DIRCTRL]->ctns.d.ln[0].x/scale_x);
                line_pt[0].y = (int)(pzone->dic[DIRCTRL]->ctns.d.ln[0].y/scale_y);
                line_pt[1].x = (int)(pzone->dic[DIRCTRL]->ctns.d.ln[1].x/scale_x);
                line_pt[1].y = (int)(pzone->dic[DIRCTRL]->ctns.d.ln[1].y/scale_y);

                mptx = (line_pt[0].x + line_pt[1].x) >> 1;
                mpty = (line_pt[0].y + line_pt[1].y) >> 1;
                dx = line_pt[1].x - line_pt[0].x;
                dy = line_pt[1].y - line_pt[0].y;
                l = sqrtf((float)(dx * dx + dy * dy));

                _get_zone_dir_figure(pzone, fpt, &fcnt);

                for (i = 0; i < fcnt; i++) 
                {
                    dir_x = fpt[i].dx;
                    dir_y = fpt[i].dy;

                    fpt[i].x = (int)(((float)((dir_x * dx - dir_y * dy) * 4 / l) + mptx) * scale_x);
                    fpt[i].y = (int)(((float)((dir_y * dx + dir_x * dy) * 4 / l) + mpty) * scale_y);
                }

                if (_is_point_in_figure(mpt, fpt, fcnt, scale_x, scale_y) == 0) return zone;
            }
        }
    }
    min = 0x7fffffff;
    zone = -1;
    for (i = 0; i < rcnt; ++i) 
	{
        if (min > tmp_area[i]) 
		{
            zone = tmp_rule[i]; 
            min = tmp_area[i];
        }
    }

    return zone;
}

int dvaa_itx_detector_add_cntr_default_template(DVAAID id, ITX_CNTRID cntr)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_DVACNTR_CONF conf;
    ITX_DVACNTR_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_DVACNTR_CONF));
    memset(&shape, 0x00, sizeof(ITX_DVACNTR_SHAPE));    

    pcntr = _dvaa_itx_get_cntr_ptr((ITXDVAA_T*)id, cntr);
    _set_cntr_type(pcntr);
            
    conf.use_cntr = 1;   
    _apply_cntr_conf(&conf, pcntr);
    _update_cntr_count(pdvaa);

    shape.ptcnt = 4;    
    shape.pt[0].x = 1620;
    shape.pt[0].y = 1152;
    shape.pt[1].x = shape.pt[0].x;
    shape.pt[1].y = shape.pt[0].y;
    shape.pt[2].x = shape.pt[0].x;
    shape.pt[2].y = shape.pt[0].y;
    shape.pt[3].x = shape.pt[0].x;
    shape.pt[3].y = shape.pt[0].y;
    shape.pt[1].dx = 0;
    shape.pt[1].dy = -36;
    shape.pt[2].dx = 140;
    shape.pt[2].dy = -36;
    shape.pt[3].dx = 140;
    shape.pt[3].dy = 0;
    _set_cntr_figure(pcntr, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_FF003F));

    _set_cntr_name(pcntr, " ", COLOR_PRG_IDX(UX_COLOR_FF003F));
    _set_cntr_value(pcntr, COLOR_PRG_IDX(UX_COLOR_FF003F));
    return 0;
}

int dvaa_itx_detector_get_cntr_shape(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_SHAPE *shape)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    DMSG(9, "cntrid = %d\n", cntr);
    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);
    _get_cntr_figure(pcntr, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_cntr_name(pcntr, shape->name);
    return 0;
}

int dvaa_itx_detector_set_cntr_shape(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_SHAPE *shape)
{
    DVACNTRPTR pcntr;
    int ret = 0;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (shape->ptcnt > MAX_PT) return -1;

    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);
    if (_IS_LINE(pcntr) && shape->ptcnt > 2) return -1;
    if (_IS_AREA(pcntr) && shape->ptcnt < 3) return -2;
    if (_IS_AREA(pcntr) && shape->ptcnt > MAX_PT_ITX) return -3;

    _set_cntr_figure(pcntr, shape->pt, shape->ptcnt, shape->color_idx);
    _set_cntr_name(pcntr, shape->name, shape->color_idx);    
    _set_cntr_value(pcntr, shape->color_idx);
    return 0;
}

int dvaa_itx_detector_get_cntr_conf(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_CONF *conf)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);
    _bring_cntr_conf(pcntr, conf);
    return 0;
}

int dvaa_itx_detector_get_cntr_confs_all(DVAAID id, ITX_DVACNTR_CONF conf[16], int *cnt)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    *cnt = 0;
    if (!pdvaa) return -1;

    _bring_cntr_confs_all(pdvaa, conf, cnt);
    return 0;
}

int dvaa_itx_detector_set_cntr_conf(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_CONF *conf)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);
    _apply_cntr_conf(conf, pcntr);
    _update_cntr_count(pdvaa);
    return 0;
}

int dvaa_itx_detector_get_cntr_value(DVAAID id, ITX_CNTRID cntr, int *en, int *count)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);

    *en = pcntr->dic[COUNT_VALUE]->en;
    *count = pcntr->cnt_value;    
    return 0;
}

int dvaa_itx_detector_set_cntr_value(DVAAID id, ITX_CNTRID cntr, int en, int count)
{
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, cntr);

    pcntr->dic[COUNT_VALUE]->en = en;
    pcntr->cnt_value = count;
    return 0;
}

ITX_CNTRID dvaa_itx_detector_find_cntr(DVAAID id, int x, int y, float scale_x, float scale_y, int degree)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    DVACNTRPTR pcntr;
    ITX_CNTRID cntr;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    GdkPoint npt;
    int fcnt = 0;
    int color_idx;
    
    if (!pdvaa) return -1;
    if (!dit_page_is_enabled(pdvaa->ditid, pdvaa->dvacntr[0].dipage)) return -1;

    mpt.x = x;
    mpt.y = y;
    
    for (cntr = 0; cntr < MAX_CNTR; cntr++) {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, (ITX_CNTRID)cntr);
        if (!pcntr) continue;
        if (!pcntr->use) continue;

        if (_get_cntr_figure(pcntr, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure_degree(mpt, fpt, fcnt, scale_x, scale_y, degree) == 0) return cntr;
    }
    return -1;
}

int dvaa_itx_detector_get_rule_prop(DVAAID id, ITX_DVARULE_PROP *prop)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _bring_rule_prop(pdvaa, prop);    
    return 0;
}

int dvaa_itx_detector_set_rule_prop(DVAAID id, ITX_DVARULE_PROP *prop)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _apply_rule_prop(pdvaa, prop);
    dvaa_itx_activate_all_rule(id);    
    return 0;
}

int dvaa_itx_detector_add_calb_default_template(DVAAID id, ITX_CALBID calb)
{
    DVACALBPTR pcalb;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_DVACALB_CONF conf;
    ITX_DVACALB_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_DVACALB_CONF));
    memset(&shape, 0x00, sizeof(ITX_DVACALB_SHAPE));    

    pcalb = _dvaa_itx_get_calb_ptr((ITXDVAA_T*)id, calb);
    _set_calb_type(pcalb);
    
    conf.use_calb = 1;
    conf.height = 175;
    memset(shape.value, 0x00, sizeof(shape.value));
    g_sprintf(shape.value, "%d", conf.height);
    
    _apply_calb_conf(&conf, pcalb);
    _update_calb_count(pdvaa);

    shape.ptcnt = 2;    
    shape.pt[0].x = 1920;
    shape.pt[0].y = 950;
    shape.pt[1].x = 1920;
    shape.pt[1].y = 1210;    
    _set_calb_figure(pcalb, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_FF007F));
    _set_calb_icon_upper(pcalb, COLOR_PRG_IDX(UX_COLOR_80FFFF));
    _set_calb_icon_lower(pcalb, COLOR_PRG_IDX(UX_COLOR_80FFFF));
    _set_calb_value(pcalb, shape.value, COLOR_PRG_IDX(UX_COLOR_80FFFF));
    return 0;
}

int dvaa_itx_detector_get_calb_shape(DVAAID id, ITX_CALBID calb, ITX_DVACALB_SHAPE *shape)
{
    DVACALBPTR pcalb;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    DMSG(9, "calbid = %d\n", calb);
    pcalb = _dvaa_itx_get_calb_ptr(pdvaa, calb);

    _get_calb_figure(pcalb, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_calb_icon_upper(pcalb, &shape->iupp_pt[0], &shape->iupp_w, &shape->iupp_h, &shape->iupp_color_idx);    
    _get_calb_icon_lower(pcalb, shape->ilow_pt, &shape->ilow_ptcnt, &shape->ilow_color_idx);
    _get_calb_value(pcalb, shape->value);
    return 0;
}

int dvaa_itx_detector_set_calb_shape(DVAAID id, ITX_CALBID calb, ITX_DVACALB_SHAPE *shape)
{
    DVACALBPTR pcalb;
    int ret = 0;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (shape->ptcnt > 2) return -1;

    pcalb = _dvaa_itx_get_calb_ptr(pdvaa, calb);

    _set_calb_figure(pcalb, shape->pt, shape->ptcnt, COLOR_PRG_IDX(UX_COLOR_FF007F));
    _set_calb_icon_upper(pcalb, shape->iupp_color_idx);
    _set_calb_icon_lower(pcalb, shape->ilow_color_idx);   
    _set_calb_value(pcalb, shape->value, shape->iupp_color_idx);    
    return 0;
}

int dvaa_itx_detector_get_calb_conf(DVAAID id, ITX_CALBID calb, ITX_DVACALB_CONF *conf)
{
    DVACALBPTR pcalb;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcalb = _dvaa_itx_get_calb_ptr(pdvaa, calb);
    _bring_calb_conf(pcalb, conf);
    return 0;
}

int dvaa_itx_detector_set_calb_conf(DVAAID id, ITX_CALBID calb, ITX_DVACALB_CONF *conf)
{
    DVACALBPTR pcalb;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pcalb = _dvaa_itx_get_calb_ptr(pdvaa, calb);
    _apply_calb_conf(conf, pcalb);
    _update_calb_count(pdvaa);
    return 0;
}

int dvaa_itx_detector_get_calb_confs_all(DVAAID id, ITX_DVACALB_CONF conf[32], int *cnt)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    *cnt = 0;
    if (!pdvaa) return -1;

    _bring_calb_confs_all(pdvaa, conf, cnt);
    return 0;
}

ITX_CALBID dvaa_itx_detector_find_calb(DVAAID id, int x, int y, float scale_x, float scale_y)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    DVACALBPTR pcalb;
    ITX_CALBID calb;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    int fcnt = 0;
    int color_idx;
    int width, height;
    
    if (!pdvaa) return -1;
    if (!dit_page_is_enabled(pdvaa->ditid, pdvaa->dvacalb[0].dipage)) return -1;
    
    mpt.x = x;
    mpt.y = y;
    
    for (calb = 0; calb < MAX_CALB; calb++) {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, (ITX_CALBID)calb);
        if (!pcalb) continue;
        if (!pcalb->use) continue;

        if (_get_calb_figure(pcalb, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt, scale_x, scale_y) == 0) return calb;

        if (_get_calb_icon_upper(pcalb, &fpt[0], &width, &height, &color_idx) == -1) continue;
        if (_is_point_in_arc(mpt, fpt[0], width, height) == 0) return calb;        

        if (_get_calb_icon_lower(pcalb, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt, scale_x, scale_y) == 0) return calb;        
    }
    return -1;
}

int dvaa_itx_detector_get_calb_result(DVAAID id, ITX_DVACALB_RESULT *res)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _bring_calb_res(pdvaa, res);
    return 0;
}

int dvaa_itx_detector_set_calb_result(DVAAID id, ITX_DVACALB_RESULT *res)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _apply_calb_res(pdvaa, res);
    return 0;
}

int dvaa_itx_detector_get_eosd_conf(DVAAID id, ITX_DVAEOSD_CONF *conf)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _bring_eosd_conf(pdvaa, conf);
    return 0;
}

int dvaa_itx_detector_set_eosd_conf(DVAAID id, ITX_DVAEOSD_CONF *conf)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _apply_eosd_conf(pdvaa, conf);
    return 0;
}
