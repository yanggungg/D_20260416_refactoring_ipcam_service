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

#include "dvaa_itx.h"
#include "dvaa_itx_internal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_color.h"
#include "../support/nf_ui_font.h"
#include "../support/util.h"
#include <math.h>


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

static int _init_zone(FRZONEPTR pzone)
{
    memset(pzone, 0x00, sizeof(ITX_FRZONE));
    pzone->zn_data.id = -1;
    pzone->dipage = DIT_PAGE_FRZONE;
    return 0;   
}

static int _copy_prop_to_db(DvaBxFacePropData *src, DvaBxFaceData *pdb)
{
    memcpy(&pdb->prop, src, sizeof(DvaBxFacePropData));
    return 0;
}

static int _copy_db_to_prop(DvaBxFaceData *pdb, DvaBxFacePropData *dst)
{
    memcpy(dst, &pdb->prop, sizeof(DvaBxFacePropData));
    return 0;
}

static int _load_zone_data(DvaBxFaceZone *zn_data, FRZONEPTR pzone)
{
    if (zn_data) memcpy(&pzone->zn_data, zn_data, sizeof(DvaBxFaceZone));
    return 0;
}

static int _unload_zone_data(FRZONEPTR pzone, DvaBxFaceZone *zn_data)
{
    if (zn_data) memcpy(zn_data, &pzone->zn_data, sizeof(DvaBxFaceZone));
    return 0;
}

static int _load_db(ITXDVAA_T *pdvaa)
{
    DAL_get_dvabx_face_data(&pdvaa->fr_db, pdvaa->ch);
    return 0;
}

static int _save_db(ITXDVAA_T *pdvaa)
{
    DAL_set_dvabx_face_data(pdvaa->fr_db, pdvaa->ch);
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

static int _is_point_in_arc(IGPOINT mpt, IGPOINT fpt, int width, int height)
{

    return -1; 
}

static IGPOINT _rotate_point(IGPOINT src, IGPOINT datum_pt, gdouble cost, gdouble sint)
{
    IGPOINT dst;

    dst.x = (int)ceil(((src.x - datum_pt.x) * cost) +  ((src.y - datum_pt.y) * sint) + datum_pt.x);
    dst.y = (int)ceil(((datum_pt.x - src.x) * sint) +  ((src.y - datum_pt.y) * cost) + datum_pt.y);
    return dst;
}

static int _get_point_from_zone(IGPOINT *pt, int *cnt, DvaBxFaceZone *pz)
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

static int _get_dir_position(IGPOINT pt1, IGPOINT pt2, IGPOINT *dst)
{
    GdkPoint arrow_pt[7] = {{-2,-7}, {-2, 3}, {-5, 3}, {0, 8}, {5, 3}, {3, 3}, {3, -7}};
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

static int _get_zone_dir_figure(FRZONEPTR pzone, IGPOINT *figure, int *cnt)
{
    IGPOINT zone_pt[16];
    int zone_cnt = 0;

    memset(zone_pt, 0x00, sizeof(zone_pt));

    _get_point_from_zone(zone_pt, &zone_cnt, &pzone->zn_data);
    _get_dir_position(zone_pt[0], zone_pt[1], figure);
    *cnt = 7;
    return 0;
}

static int _get_zone_figure(FRZONEPTR pzone, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    *cnt = pzone->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pzone->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);   
    
    return 0;
}

static int _get_zone_name(FRZONEPTR pzone, char *str)
{
    if (!pzone->dic[INFO]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    g_stpcpy(str, pzone->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _set_zone_type(FRZONEPTR pzone, int type)
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

static int _set_zone_direction_to_dic(FRZONEPTR pzone, int color_idx)
{
    IGPOINT zone_pt[16];
    IGPOINT dir_pt[16];
    int zone_cnt = 0;

    if (!pzone->dic[DIRCTRL]) return -1;
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

static int _set_zone_direction(FRZONEPTR pzone, int color_idx)
{
    _set_zone_direction_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_name_to_dic(FRZONEPTR pzone, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    int cnt;

    if (!pzone->dic[INFO]) return -1;
    
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

static int _set_zone_name_to_zndata(FRZONEPTR pzone, char *str)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    strcpy(pzone->zn_data.name, str);
    return 0;
}

static int _set_zone_name(FRZONEPTR pzone, char *str, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_name_to_dic(pzone, str, color_idx);
    _set_zone_name_to_zndata(pzone, str);
    return 0;
}

static int _set_zone_value_to_dic(FRZONEPTR pzone, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    char strBuf[16];
    int cnt;

    if (!pzone->dic[COUNT_VALUE]) return -1;
    
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

static int _set_zone_value(FRZONEPTR pzone, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_value_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_figure_to_dic(FRZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pzone->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    
    if (pzone->event_time > 0) pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);    
    else pzone->dic[FIGURE]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_zone_figure_to_zndata(FRZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    for (i = 0; i < cnt; ++i) {
        pzone->zn_data.pt[i].x = pt[i].x;
        pzone->zn_data.pt[i].y = pt[i].y;
    }

    pzone->zn_data.npts = cnt;    
    pzone->zn_data.d_color = dvaa_itx_convert_to_coloridx_rgb_color(color_idx);    
    return 0;
}

static int _set_zone_figure(FRZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    _set_zone_figure_to_dic(pzone, pt, cnt, color_idx);
    _set_zone_figure_to_zndata(pzone, pt, cnt, color_idx);
    return 0;
}

static int _make_zone_direction_highlight(FRZONEPTR pzone)
{
    if (!pzone->dic[DIRCTRL]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[DIRCTRL]->ctns.d.wi = 5;
    pzone->dic[DIRCTRL]->ctns.d.fi = 1;    
    return 0;
}

static int _make_zone_direction_lowlight(FRZONEPTR pzone)
{   
    if (!pzone->dic[DIRCTRL]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[DIRCTRL]->ctns.d.wi = 3;
    pzone->dic[DIRCTRL]->ctns.d.fi = 0;
    return 0;
}

static int _make_zone_highlight(FRZONEPTR pzone)
{
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[FIGURE]->ctns.f.wi = 5;
    return 0;
}

static int _make_zone_lowlight(FRZONEPTR pzone)
{   
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[FIGURE]->ctns.f.wi = 3;
    return 0;
}

static int _make_zone_focus(FRZONEPTR pzone)
{
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 1;
    return 0;
}

static int _make_zone_unfocus(FRZONEPTR pzone)
{   
    if (!pzone->dic[FIGURE]) return -1;
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 0;
    return 0;
}

static int _apply_rule_prop(ITXDVAA_T *pdvaa, ITX_FRRULE_PROP *prop)
{
    pdvaa->fr_prop.sw_obj_bb = prop->sw_obj_bb;
    pdvaa->fr_prop.sw_obj_grpname = prop->sw_obj_grpname;
    pdvaa->fr_prop.sw_rule = prop->sw_rule;    

    return 0;
}

static int _bring_rule_prop(ITXDVAA_T *pdvaa, ITX_FRRULE_PROP *prop)
{
    prop->sw_obj_bb = pdvaa->fr_prop.sw_obj_bb;
    prop->sw_obj_grpname = pdvaa->fr_prop.sw_obj_grpname;
    prop->sw_rule = pdvaa->fr_prop.sw_rule; 

    return 0;
}

static int _update_zone_count(ITXDVAA_T *pdvaa)
{
    int id;
    FRZONEPTR pzone;

    pdvaa->cnt_frzone = 0;

    for (id = 0; id < MAX_FRZONE; ++id) 
    {
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, id);
        if (pzone->use) pdvaa->cnt_frzone++;
    }

    return 0;
}

static int _apply_zone_conf(ITX_FRZONE_CONF *conf, FRZONEPTR pzone)
{
    if (!pzone->dic[FIGURE]) return -1;
    if (!pzone->dic[INFO]) return -1;
    if (!pzone->dic[DIRCTRL]) return -1;
    if (!pzone->dic[COUNT_VALUE]) return -1;
    
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

    pzone->trigger = conf->triggerid;

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

static int _bring_zone_conf(FRZONEPTR pzone, ITX_FRZONE_CONF *conf)
{
    conf->zoneid = pzone->id;
    conf->type = pzone->zn_data.type;
    conf->use_zone = pzone->use;   
    conf->focus = pzone->focus;
    
    conf->active = pzone->zn_data.active;  
    conf->triggerid = pzone->trigger;
    return 0;
}

static int _bring_zone_confs_all(ITXDVAA_T *pdvaa, ITX_FRZONE_CONF conf[32], int *cnt)
{
    int i;
    FRZONEPTR pzone;

    for (i = 0; i < MAX_FRZONE; ++i) 
    {
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, (ITX_FRZONEID)i);
        _bring_zone_conf(pzone, &conf[i]);
    }

    if (cnt) *cnt = pdvaa->cnt_frzone;

    return 0;
}

static int _init_zone_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    FRZONEPTR pzone;
    DvaBxFaceZone *zone_data;
    
    for (id = 0; id < MAX_FRZONE; ++id) 
    {
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 0;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(DvaBxFaceZone));
    } 
    
    return 0;
}

static int _init_zone_pb_data(ITXDVAA_T *pdvaa)
{
    int id;       
    
    FRZONEPTR pzone;
    DvaBxFaceZone *zone_data;
    
    for (id = 0; id < MAX_FRZONE; ++id) 
    {
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 1;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(DvaBxFaceZone));
    } 
    
    return 0;
}

static int _init_pb_default_data(ITXDVAA_T *pdvaa)
{
    pdvaa->fr_prop.sw_rule = TRUE;
    pdvaa->fr_prop.sw_obj_bb = TRUE;
    pdvaa->fr_prop.sw_obj_grpname = TRUE;
    return 0;
}

static int _translate_vcdata_into_rule(ITXDVAA_T *pdvaa, DvaBxFaceData *pdb)
{
    int id;
       
    FRZONEPTR pzone;
    DvaBxFaceZone *zone_data;
    gint cnt;
    
    for (id = 0; id < pdb->zonelist.cnt; ++id) 
    {   
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, pdb->zonelist.zone[id].id);
        zone_data = &pdb->zonelist.zone[id];

        pzone->use = 1;
        _load_zone_data(zone_data, pzone);
        _set_zone_type(pzone, zone_data->type);
    } 

    return 0;
}

static int _translate_rule_into_vcdata(ITXDVAA_T *pdvaa, DvaBxFaceData *pdb)
{
    int id, cnt;
       
    FRZONEPTR pzone;
    DvaBxFaceZone *zone_data;

    cnt = 0;
    
    for (id = 0; id < MAX_FRZONE; ++id) 
    {
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, id);
        if (!pzone->use) continue;
    
        zone_data = &pdb->zonelist.zone[cnt++];
        _unload_zone_data(pzone, zone_data);        
    }

    pdb->zonelist.cnt = pdvaa->cnt_frzone;
    
    return 0;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _dvaa_itx_face_init(ITXDVAA_T *pdvaa)
{
    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    _translate_vcdata_into_rule(pdvaa, &pdvaa->fr_db);
    _copy_db_to_prop(&pdvaa->fr_db, &pdvaa->fr_prop);
    return 0;
}

int _dvaa_itx_face_pb_init(ITXDVAA_T *pdvaa)
{
    _init_zone_pb_data(pdvaa);
    _init_pb_default_data(pdvaa);
    return 0;
}

int _dvaa_itx_face_reload(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    _translate_vcdata_into_rule(pdvaa, &pdvaa->fr_db);
    _copy_db_to_prop(&pdvaa->fr_db, &pdvaa->fr_prop);
    return 0;
}

int _dvaa_itx_face_load_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _load_db(pdvaa);
    _init_zone_data(pdvaa);
    _translate_vcdata_into_rule(pdvaa, &pdvaa->fr_db);
    _copy_db_to_prop(&pdvaa->fr_db, &pdvaa->fr_prop);
    return 0;
}

int _dvaa_itx_face_save_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    memset(&pdvaa->fr_db, 0x00, sizeof(DvaBxFaceData));
    _copy_prop_to_db(&pdvaa->fr_prop, &pdvaa->fr_db);
    _translate_rule_into_vcdata(pdvaa, &pdvaa->fr_db);
    _save_db(pdvaa);
    return 0;
}

int _dvaa_itx_face_is_db_changed(DVAAID id)
{
    DvaBxFaceData tmpdb;  
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return 0;

    memset(&tmpdb, 0x00, sizeof(DvaBxFaceData));
    memmove(&tmpdb, &pdvaa->fr_db, sizeof(DvaBxFaceData));
    _copy_prop_to_db(&pdvaa->fr_prop, &tmpdb);
    _translate_rule_into_vcdata(pdvaa, &tmpdb);  
//  DMSG(1, "%d", (memcmp(&pdvaa->fr_db, &tmpdb, sizeof(DvaBxFaceData))));
    return (memcmp(&pdvaa->fr_db, &tmpdb, sizeof(DvaBxFaceData)) != 0);
}



////////////////////////////////////////////////////////////
//
// public interfaces 
//

int dvaa_itx_face_add_zone_line_default_template(DVAAID id, ITX_FRZONEID zone)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_FRZONE_CONF conf;
    ITX_FRZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_FRZONE_SHAPE));    

    pzone = _dvaa_itx_get_frzone_ptr((ITXDVAA_T*)id, zone);
    _set_zone_type(pzone, IVCA_RT_LINE);
    
    conf.use_zone = 1;
    _apply_zone_conf(&conf, pzone);
    _update_zone_count(pdvaa);
    
    shape.ptcnt = 2;    
    shape.pt[0].x = 1440;
    shape.pt[0].y = 1260;
    shape.pt[1].x = 2400;
    shape.pt[1].y = 900;
    
    _set_zone_figure(pzone, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_BFFF00));
    if (_IS_DIRCTRL(pzone)) _set_zone_direction(pzone, COLOR_PRG_IDX(UX_COLOR_BFFF00));

    _set_zone_name(pzone, " ", COLOR_PRG_IDX(UX_COLOR_BFFF00));  
    _set_zone_value(pzone, COLOR_PRG_IDX(UX_COLOR_BFFF00));
    return 0;
}

int dvaa_itx_face_add_zone_area_default_template(DVAAID id, ITX_FRZONEID zone)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ITX_FRZONE_CONF conf;
    ITX_FRZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_FRZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_FRZONE_SHAPE));    

    pzone = _dvaa_itx_get_frzone_ptr((ITXDVAA_T*)id, zone);
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
    _set_zone_figure(pzone, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_BFFF00));

    _set_zone_name(pzone, " ", COLOR_PRG_IDX(UX_COLOR_BFFF00));    
    _set_zone_value(pzone, COLOR_PRG_IDX(UX_COLOR_BFFF00));
    return 0;
}

int dvaa_itx_face_get_zone_shape(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_SHAPE *shape)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;

    if (!pdvaa) return -1;

    DMSG(9, "zoneid = %d\n", zone);
    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    _get_zone_figure(pzone, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_zone_dir_figure(pzone, &shape->dir_pt, &shape->dir_ptcnt);
    _get_zone_name(pzone, shape->name);       
    return 0;
}

int dvaa_itx_face_set_zone_shape(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_SHAPE *shape)
{
    FRZONEPTR pzone;
    int ret = 0;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (shape->ptcnt > MAX_PT) return -1;

    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    if (_IS_LINE(pzone) && shape->ptcnt > 2) return -1;
    if (_IS_AREA(pzone) && shape->ptcnt < 3) return -2;
    if (_IS_AREA(pzone) && shape->ptcnt > MAX_PT_ITX) return -3;

    _set_zone_figure(pzone, shape->pt, shape->ptcnt, shape->color_idx);
    if (_IS_DIRCTRL(pzone)) _set_zone_direction(pzone, shape->color_idx);
    _set_zone_name(pzone, shape->name, shape->color_idx);
    _set_zone_value(pzone, shape->color_idx);
    return 0;
}

int dvaa_itx_face_get_zone_conf(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_CONF *conf)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    _bring_zone_conf(pzone, conf);
    return 0;
}

int dvaa_itx_face_get_zone_confs_all(DVAAID id, ITX_FRZONE_CONF conf[32], int *cnt)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    *cnt = 0;
    if (!pdvaa) return -1;

    _bring_zone_confs_all(pdvaa, conf, cnt);
    return 0;
}

int dvaa_itx_face_set_zone_conf(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_CONF *conf)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    _apply_zone_conf(conf, pzone);
    _update_zone_count(pdvaa);
    return 0;
}

int dvaa_itx_face_get_zone_value(DVAAID id, ITX_FRZONEID zone, int *en, int *color_idx, int *count)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;
    if (!pzone->dic[COUNT_VALUE]) return -1;

    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    *en = pzone->dic[COUNT_VALUE]->en;
    *color_idx = pzone->dic[COUNT_VALUE]->ctns.t.cl.i;    
    *count = pzone->cnt_value;
    return 0;
}

int dvaa_itx_face_set_zone_value(DVAAID id, ITX_FRZONEID zone, int en, int color_idx, int count)
{
    FRZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;
    if (!pzone->dic[COUNT_VALUE]) return -1;

    pzone = _dvaa_itx_get_frzone_ptr(pdvaa, zone);
    pzone->dic[COUNT_VALUE]->en = en;
    pzone->cnt_value = count;
    _set_zone_value(pzone, color_idx);
    return 0;
}

ITX_FRZONEID dvaa_itx_face_find_zone(DVAAID id, int x, int y)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    FRZONEPTR pzone;
    ITX_FRZONEID zone;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    ITX_FRZONEID tmp_rule[MAX_FRZONE];
    int tmp_area[MAX_FRZONE]; 
    int rcnt = 0;
    IXSQUARE sq;
    int min = 0;
    int i;
    int fcnt = 0;
    int color_idx;
    IXPOLYGON p;
    
    if (!pdvaa) return -1;
    if (!dit_page_is_enabled(pdvaa->ditid, pdvaa->frzone[0].dipage)) return -1;

    mpt.x = x;
    mpt.y = y;
    
    rcnt = 0;
    
    memset(&p, 0x00, sizeof(IXPOLYGON));
    memset(tmp_rule, 0x00, sizeof(tmp_rule));
    memset(tmp_area, 0x00, sizeof(tmp_area));
    memset(&sq, 0x00, sizeof(IGBOX));
    
    for (zone = MAX_FRZONE-1; zone >= 0; zone--) 
	{
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, (ITX_FRZONEID)zone);
        if (!pzone) continue;
        if (!pzone->use) continue;

        if (_get_zone_figure(pzone, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt) == 0) 
		{
            _make_polygon(fpt, fcnt, &p);
            if (ifn_point_is_over_line(mpt.x, mpt.y, MAX_FRZONE, &p)) return zone;
            
            tmp_rule[rcnt] = zone;
            ifn_get_outbound_square(&p, &sq);
            tmp_area[rcnt] = ifn_get_area(&sq);
            rcnt++;
        }
        else
        {
            if (_IS_DIRCTRL(pzone)) 
            {
                _get_zone_dir_figure(pzone, fpt, &fcnt);
                if (_is_point_in_figure(mpt, fpt, fcnt) == 0) return zone;
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

int dvaa_itx_face_get_rule_prop(DVAAID id, ITX_FRRULE_PROP *prop)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _bring_rule_prop(pdvaa, prop);    
    return 0;
}

int dvaa_itx_face_set_rule_prop(DVAAID id, ITX_FRRULE_PROP *prop)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _apply_rule_prop(pdvaa, prop);
    dvaa_itx_activate_all_rule(id);    
    return 0;
}
