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

#include "dit.h"
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

static int _init_extzone(ITXDVAA_T *pdvaa)
{
    int id;    
    EXTZONEPTR pzone;
    
    for (id = 0; id < MAX_EXTZONE; ++id) 
    {
        pzone = _dvaa_itx_get_extzone_ptr(pdvaa, id);

        memset(pzone, 0x00, sizeof(ITX_EXTZONE));
        pzone->type |= MASK_AREA;
        pzone->event_time = -1;
        pzone->dipage = DIT_PAGE_EXTZONE;
    } 
    return 0;   
}

static int _init_meta(ITXDVAA_T *pdvaa)
{
    pdvaa->cnt_meta = 1;

    pdvaa->dvameta[0].meta = DVAA_META_BBOX;
    pdvaa->dvameta[0].dipage = DIT_PAGE_META;

    pdvaa->dvameta[1].meta = DVAA_META_TRAJ;
    pdvaa->dvameta[1].dipage = DIT_PAGE_META;

    pdvaa->dvameta[2].meta = -1;
    pdvaa->dvameta[2].dipage = DIT_PAGE_META;

    pdvaa->dvameta[3].meta = -1;
    pdvaa->dvameta[3].dipage = DIT_PAGE_META;
    return 0;
}

static int _init(ITXDVAA_T *pdvaa, int ch)
{
    memset(pdvaa, 0x00, sizeof(ITXDVAA_T));
    pdvaa->ch = ch;

    _init_extzone(pdvaa);
    _init_meta(pdvaa);
    return 0;   
}

static ITX_ZONEID _get_matched_zoneid(ITXDVAA_T *pdvaa, int zoneid)
{
    int i;

    for (i = 0; i < MAX_ZONE; ++i)
        if (pdvaa->dvazone[i].zn_data.id == zoneid) return i;

    return -1;
}

static ITX_CNTRID _get_matched_cntrid(ITXDVAA_T *pdvaa, int countid)
{
    int i;
    
    for (i = 0; i < MAX_CNTR; ++i)
        if (pdvaa->dvacntr[i].ct_data.id == countid) return i;

    return -1;
}

static int _is_enable_detector_engine(ITXDVAA_T *pdvaa)
{
    return (pdvaa->ext_rule == 0 && pdvaa->prop.en_engine == TRUE && pdvaa->algo_type == ITX_ALGOTYPE_DETECTOR);
}

static int _is_enable_fr_engine(ITXDVAA_T *pdvaa)
{
    return (pdvaa->ext_rule == 0 && pdvaa->prop.en_engine == TRUE && pdvaa->algo_type == ITX_ALGOTYPE_FACE);
}

static int _is_enable_lpr_engine(ITXDVAA_T *pdvaa)
{
    return (pdvaa->ext_rule == 0 && pdvaa->prop.en_engine == TRUE && pdvaa->algo_type == ITX_ALGOTYPE_PLATENO);
}

static int _is_enable_external_engine(ITXDVAA_T *pdvaa)
{
    return (pdvaa->ext_rule == 1);
}

static int _is_display_rule(ITXDVAA_T *pdvaa)
{
    //return (pdvaa->prop.sw_rule == TRUE || pdvaa->st_rule == TRUE);
    return (pdvaa->st_rule == TRUE);
}

static int _is_display_bounding_box(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_bb == TRUE);
}

static int _is_display_object_id(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_id == TRUE);
}

static int _is_display_object_w3d(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_w3d == TRUE);
}

static int _is_display_object_h3d(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_h3d == TRUE);
}

static int _is_display_object_s3d(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_s3d == TRUE);
}

static int _is_display_trajectory(ITXDVAA_T *pdvaa)
{
    return (pdvaa->prop.sw_obj_tr == TRUE);
}

static int _is_evented_meta(ai_meta_obj_t *mobj)
{
    return (mobj->nevents > 0);
}

static int _is_static_object(ai_meta_obj_t *mobj)
{
    return (mobj->isstatic == 1);
}

static int _is_3d_calbres_valid(ITXDVAA_T *pdvaa)
{
    return pdvaa->calbres.paramvalid;
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

static int _get_point_from_frzone(IGPOINT *pt, int *cnt, DvaBxFaceZone *pz)
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

static int _get_point_from_lprzone(IGPOINT *pt, int *cnt, DvaBxPlatenoZone *pz)
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
/*
    dst->x = pt[0].x;
    dst->y = pt[0].y;

    for (i = 0; i < cnt; ++i) {
        if (pt[i].x < dst->x) dst->x = pt[i].x;
        if (pt[i].y < dst->y) dst->y = pt[i].y;
    }
*/
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

static int _get_point_from_cntr(IGPOINT *pt, int *cnt, DvaBxCntr *pc)
{
    int i;
    
    for (i = 0; i < 4; ++i) {
        pt[i].x = pc->pt[0].x;
        pt[i].y = pc->pt[0].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }

    pt[1].dx = 0;
    pt[1].dy = -36;
    pt[2].dx = 140;
    pt[2].dy = -36;
    pt[3].dx = 140;
    pt[3].dy = 0;

    *cnt = 4;

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

static int _get_meta_bound(ai_meta_obj_t *mobj, IGRECT *rect)
{
    rect->x = mobj->bbx_position[0] * 3840;
    rect->y = mobj->bbx_position[1] * 2160;
    rect->w = (mobj->bbx_position[2] - mobj->bbx_position[0]) * 3840;
    rect->h = (mobj->bbx_position[3] - mobj->bbx_position[1]) * 2160;
    
    return 0;
}

static int _set_extzone_figure_to_dic(EXTZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pzone->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    
    if (pzone->event_time > 0) pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[FIGURE]->ctns.f.cl.i = color_idx;    
    return 0;
}

static int _set_extzone_figure_to_zndata(EXTZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    for (i = 0; i < cnt; ++i) {
        pzone->pt[i].x = pt[i].x;
        pzone->pt[i].y = pt[i].y;
    }

    pzone->npts = cnt;    
    pzone->color = dvaa_itx_convert_to_coloridx_rgb_color(color_idx);    
    return 0;
}

static int _set_extzone_figure(EXTZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    _set_extzone_figure_to_dic(pzone, pt, cnt, color_idx);
    _set_extzone_figure_to_zndata(pzone, pt, cnt, color_idx);
    return 0;
}

static int _set_extzone_name_to_dic(EXTZONEPTR pzone, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    int i, cnt;

    cnt = pzone->npts;

    for (i = 0; i < cnt; ++i) {
        pt[i].x = pzone->pt[i].x;
        pt[i].y = pzone->pt[i].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }

    _get_name_position(pt, cnt, &dst);

    memset(pzone->dic[INFO]->ctns.t.txt, 0x00, sizeof(pzone->dic[INFO]->ctns.t.txt));
    snprintf(pzone->dic[INFO]->ctns.t.txt, sizeof(pzone->dic[INFO]->ctns.t.txt)-1, "%s", str);

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

static int _set_extzone_name_to_zndata(EXTZONEPTR pzone, char *trigger_type, char *trigger_name)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    snprintf(pzone->trigger_type, sizeof(pzone->trigger_type)-1, "%s", trigger_type);
    snprintf(pzone->trigger_name, sizeof(pzone->trigger_name)-1, "%s", trigger_name);
    return 0;
}

static int _set_extzone_name(EXTZONEPTR pzone, char *trigger_type, char *trigger_name, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_extzone_name_to_dic(pzone, trigger_name, color_idx);
    _set_extzone_name_to_zndata(pzone, trigger_type, trigger_name);
    return 0;
}

static int _construct_figure(IGPOINT *pt, int cnt, FIGURE_INFO *info)
{
    int i;

    DMSG(9, "cnt = %d\n", cnt);
    info->cnt = cnt;
    for (i = 0; i < cnt; ++i) {
        info->pt[i].x = pt[i].x;
        info->pt[i].y = pt[i].y;
        info->pt[i].dx = pt[i].dx;
        info->pt[i].dy = pt[i].dy;
        info->pt[i].opt = pt[i].opt;
    }

    return 0;
}

static int _construct_zone_direction_figure(IGPOINT *pt, DIRECTION_INFO *info)
{
    info->ln[0] = pt[0];
    info->ln[1] = pt[1];
    info->cnt = 7;
    _get_dir_position(pt[0], pt[1], info->pt);
    return 0;
}

static int _construct_calb_icon_upper(IGPOINT *pt, int cnt, ARC_INFO *info)
{
    _get_calb_upper_position(pt[0], pt[1], &info->pt, &info->w, &info->h);    
    return 0;
}

static int _construct_calb_icon_lower(IGPOINT *pt, int cnt, FIGURE_INFO *info)
{
    _get_calb_lower_position(pt[0], pt[1], info->pt, &info->cnt);
    return 0;
}

static int _construct_trackbox_figure(IGRECT *rect, FIGURE_INFO *info)
{
    info->wi = 3;
    info->st = 0;
    info->vt = 0;
    info->cnt = 4;
    
    info->pt[0].x = rect->x;
    info->pt[0].y = rect->y;

    info->pt[1].x = rect->x + rect->w;
    info->pt[1].y = rect->y;

    info->pt[2].x = rect->x + rect->w;
    info->pt[2].y = rect->y + rect->h;

    info->pt[3].x = rect->x;
    info->pt[3].y = rect->y + rect->h;

    return 0;
}

static int _construct_trajectory_figure(int npt, IXPOINT *pt, FIGURE_INFO *info)
{
    int i;
    IXPOINT* p = pt;

    info->wi = 2;
    info->st = 1;
    info->vt = 0;
    info->cnt = npt;
    
    for (i = 0; i < npt; i++, p++)
    {
        info->pt[i].x = p->x;
        info->pt[i].y = p->y;
        info->pt[i].opt = 0;
    }
    return 0;
}

static int _makeup_zone_figure(DVAZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        info->vt = 1;
        info->wi = 5;
    }
    else {
        info->vt = 0;
        info->wi = 3;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);

    return 0;
}

static int _makeup_zone_direction_figure(DVAZONEPTR pzone, DIRECTION_INFO *info)
{
    if (pzone->sty_highlight) {
        info->wi = 5;
        info->fi = 1;
    }
    else {
        info->wi = 3;
        info->fi = 0;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    return 0;
}

static int _makeup_zone_name(DVAZONEPTR pzone, TEXT_INFO *info)
{
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pzone->zn_data.name);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    info->size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _makeup_zone_value(DVAZONEPTR pzone, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.x = 0;
    info->pt.y = 5;

    info->layout_psx = 1;
    info->layout_psy = 1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    info->size = NFFONT_SMALL_NORMAL;
    return 0;
}

static int _makeup_cntr_figure(DVACNTRPTR pcntr, FIGURE_INFO *info)
{   
    if (pcntr->sty_highlight) {
//        info->vt = 1;
        info->wi = 5;
    }
    else {
//        info->vt = 0;
        info->wi = 3;
    }

    info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    return 0;
}

static int _makeup_cntr_name(DVACNTRPTR pcntr, TEXT_INFO *info)
{   
    info->pt.dy = -36;

    info->layout_psx = 1;
    info->layout_psy = -1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pcntr->ct_data.name);

    info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    info->size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _makeup_cntr_value(DVACNTRPTR pcntr, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.dx = 10;
    info->pt.dy = -18;
    
    info->layout_psx = 1;
    info->layout_psy = 0;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    info->size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _makeup_calb_figure(DVACALBPTR pcalb, FIGURE_INFO *info)
{   
    info->wi = 3;

    if (pcalb->sty_highlight) {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_FF007F);
        info->vt = 1;
    }
    else {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_FF007F);
        info->vt = 0;
    }

    return 0;
}

static int _makeup_calb_icon_upper(DVACALBPTR pcalb, ARC_INFO *info)
{   
    info->fi = 1;
    
    if (pcalb->sty_highlight) {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    }
    else {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    }

    return 0;
}

static int _makeup_calb_icon_lower(DVACALBPTR pcalb, FIGURE_INFO *info)
{   
    info->fi = 1;
    
    if (pcalb->sty_highlight) {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    }
    else {
        info->cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    }

    return 0;
}

static int _makeup_calb_value(DVACALBPTR pcalb, TEXT_INFO *info, int value)
{
    guint w;
    
    info->pt.dx = 0;
    info->pt.dy = 5;

    info->layout_psx = 0;
    info->layout_psy = 1;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    if (pcalb->sty_highlight) info->cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    else info->cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);

    info->size = NFFONT_MINI_SEMI_5;
    return 0;
}

static int _makeup_frzone_figure(FRZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        info->vt = 1;
        info->wi = 5;
    }
    else {
        info->vt = 0;
        info->wi = 3;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);

    return 0;
}

static int _makeup_frzone_direction_figure(FRZONEPTR pzone, DIRECTION_INFO *info)
{
    if (pzone->sty_highlight) {
        info->wi = 5;
        info->fi = 1;
    }
    else {
        info->wi = 3;
        info->fi = 0;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    return 0;
}

static int _makeup_frzone_name(FRZONEPTR pzone, TEXT_INFO *info)
{
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pzone->zn_data.name);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    info->size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _makeup_frzone_value(FRZONEPTR pzone, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.dx = 0;
    info->pt.dy = 5;

    info->layout_psx = 1;
    info->layout_psy = 1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    info->size = NFFONT_SMALL_NORMAL;
    return 0;
}

static int _makeup_lprzone_figure(LPRZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        info->vt = 1;
        info->wi = 5;
    }
    else {
        info->vt = 0;
        info->wi = 3;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);

    return 0;
}

static int _makeup_lprzone_direction_figure(LPRZONEPTR pzone, DIRECTION_INFO *info)
{
    if (pzone->sty_highlight) {
        info->wi = 5;
        info->fi = 1;
    }
    else {
        info->wi = 3;
        info->fi = 0;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    return 0;
}

static int _makeup_lprzone_name(LPRZONEPTR pzone, TEXT_INFO *info)
{
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pzone->zn_data.name);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    info->size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _makeup_lprzone_value(LPRZONEPTR pzone, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.dx = 0;
    info->pt.dy = 5;

    info->layout_psx = 1;
    info->layout_psy = 1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
    
    info->size = NFFONT_SMALL_NORMAL;
    return 0;
}

static int _makeup_extzone_figure(EXTZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        //info->vt = 1;
        info->wi = 5;
    }
    else {
        //info->vt = 0;
        info->wi = 3;
    }

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->color);

    return 0;
}

static int _makeup_extzone_name(EXTZONEPTR pzone, TEXT_INFO *info)
{
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;

    memset(info->txt, 0x00, sizeof(info->txt));
    snprintf(info->txt, sizeof(info->txt)-1, "%s", pzone->trigger_name);

    if (pzone->event_time > 0) info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->color);
    
    info->size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _makeup_trackbox_figure(FIGURE_INFO *info, int evt)
{
    if (evt) info->cl.i = CR_EVENT_TBOX;    
    else info->cl.i = CR_NORMAL_TBOX;
    return 0;
}

static int _makeup_static_trackbox_figure(FIGURE_INFO *info)
{
    info->cl.i = COLOR_PRG_IDX(UX_COLOR_808080);
    return 0;
}

static int _makeup_trajectory_figure(FIGURE_INFO *info, int evt)
{
    if (evt) info->cl.i = CR_EVENT_TBOX;    
    else info->cl.i = CR_NORMAL_TBOX;
    return 0;
}

static int _makeup_text_trackbox(TEXT_INFO *info, char *buf, IGRECT *rect, int evt)
{
    info->pt.x = rect->x;
    info->pt.y = rect->y;

    info->pt.dx = 0;
    info->pt.dy = -2;

    info->layout_psx = 1;
    info->layout_psy = -1;

    if (evt) info->cl.i = CR_EVENT_TBOX;    
    else info->cl.i = CR_NORMAL_TBOX;

    info->size = NFFONT_MINI_SEMI_5;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, buf);

    return 0;
}

static int _add_zone_figure_to_display(ITXDVAA_T *pdvaa, int page, DVAZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 0;
    finfo.vt = 0;
    
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _construct_figure(pt, cnt, &finfo);
    _makeup_zone_figure(pzone, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pzone->dic[FIGURE] = pdic;
    pzone->dic[FIGURE]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_zone_name_to_display(ITXDVAA_T *pdvaa, int page, DVAZONEPTR pzone)
{
    TEXT_INFO tinfo;
    guint col;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _get_name_position(pt, cnt, &tinfo.pt);
    _makeup_zone_name(pzone, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[INFO] = pdic;
    pzone->dic[INFO]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_zone_direction_to_display(ITXDVAA_T *pdvaa, int page, DVAZONEPTR pzone)
{
    DIRECTION_INFO dinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;    
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&dinfo, 0x00, sizeof(DIRECTION_INFO));
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _construct_zone_direction_figure(pt, &dinfo);
    _makeup_zone_direction_figure(pzone, &dinfo);

    pdic = dit_add_dic(ditid, page, DIC_DIR, &dinfo);
    pzone->dic[DIRCTRL] = pdic;     

    if (_IS_LINE(pzone)) pzone->dic[DIRCTRL]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[DIRCTRL]->en = 0;
    
    return 0;
}

static int _add_zone_value_to_display(ITXDVAA_T *pdvaa, int page, DVAZONEPTR pzone, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _get_zone_value_position(pt, cnt, &tinfo.pt);
    _makeup_zone_value(pzone, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[COUNT_VALUE] = pdic;
    
    if (pzone->value_en) pzone->dic[COUNT_VALUE]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[COUNT_VALUE]->en = 0;

    return 0;
}

static int _add_zone_to_display(ITXDVAA_T *pdvaa, int page, DVAZONEPTR pzone)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_zone_figure_to_display(pdvaa, page, pzone);
    _add_zone_name_to_display(pdvaa, page, pzone);
    _add_zone_direction_to_display(pdvaa, page, pzone);
    _add_zone_value_to_display(pdvaa, page, pzone, pzone->cnt_value);
    
    return 0;
}

static int _add_cntr_figure_to_display(ITXDVAA_T *pdvaa, int page, DVACNTRPTR pcntr)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 0;
    finfo.vt = 0;

    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _construct_figure(pt, cnt, &finfo);
    _makeup_cntr_figure(pcntr, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pcntr->dic[FIGURE] = pdic;
    pcntr->dic[FIGURE]->en = pcntr->use & pcntr->ct_data.active;

    return 0;
}

static int _add_cntr_name_to_display(ITXDVAA_T *pdvaa, int page, DVACNTRPTR pcntr)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_name_position(pt, cnt, &tinfo.pt);    
    _makeup_cntr_name(pcntr, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pcntr->dic[INFO] = pdic;
    pcntr->dic[INFO]->en = pcntr->use & pcntr->ct_data.active;

    return 0;
}

static int _add_cntr_value_to_display(ITXDVAA_T *pdvaa, int page, DVACNTRPTR pcntr, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_cntr_value_position(pt, cnt, &tinfo.pt);
    _makeup_cntr_value(pcntr, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pcntr->dic[COUNT_VALUE] = pdic;
    
    if (pcntr->value_en) pcntr->dic[COUNT_VALUE]->en = pcntr->use & pcntr->ct_data.active;
    else pcntr->dic[COUNT_VALUE]->en = 0;

    return 0;
}

static int _add_cntr_to_display(ITXDVAA_T *pdvaa, int page, DVACNTRPTR pcntr)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_cntr_figure_to_display(pdvaa, page, pcntr);
    _add_cntr_name_to_display(pdvaa, page, pcntr);
    _add_cntr_value_to_display(pdvaa, page, pcntr, pcntr->cnt_value);
    return 0;
}

static int _add_calb_figure_to_display(ITXDVAA_T *pdvaa, int page, DVACALBPTR pcalb)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 9;
    finfo.vt = 0;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _construct_figure(pt, cnt, &finfo);
    _makeup_calb_figure(pcalb, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pcalb->dic[FIGURE] = pdic;
    pcalb->dic[FIGURE]->en = pcalb->use;

    return 0;
}

static int _add_calb_icon_upper_to_display(ITXDVAA_T *pdvaa, int page, DVACALBPTR pcalb)
{
    ARC_INFO ainfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    memset(&ainfo, 0x00, sizeof(ARC_INFO));
    ainfo.fi = 1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _construct_calb_icon_upper(pt, cnt, &ainfo);
    _makeup_calb_icon_upper(pcalb, &ainfo);

    pdic = dit_add_dic(ditid, page, DIC_ARC, &ainfo);
    pcalb->dic[CALB_ICUPP] = pdic;
    pcalb->dic[CALB_ICUPP]->en = pcalb->use;

    return 0;
}

static int _add_calb_icon_lower_to_display(ITXDVAA_T *pdvaa, int page, DVACALBPTR pcalb)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 0;
    finfo.vt = 0;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _construct_calb_icon_lower(pt, cnt, &finfo);
    _makeup_calb_icon_lower(pcalb, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pcalb->dic[CALB_ICLOW] = pdic;
    pcalb->dic[CALB_ICLOW]->en = pcalb->use;

    return 0;
}

static int _add_calb_value_to_display(ITXDVAA_T *pdvaa, int page, DVACALBPTR pcalb, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_value_position(pt, cnt, &tinfo.pt);
    _makeup_calb_value(pcalb, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pcalb->dic[INFO] = pdic;
    pcalb->dic[INFO]->en = pcalb->use;

    return 0;
}

static int _add_calb_to_display(ITXDVAA_T *pdvaa, int page, DVACALBPTR pcalb)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_calb_icon_upper_to_display(pdvaa, page, pcalb);
    _add_calb_icon_lower_to_display(pdvaa, page, pcalb);
    _add_calb_figure_to_display(pdvaa, page, pcalb);
    _add_calb_value_to_display(pdvaa, page, pcalb, pcalb->cb_data.height);
    return 0;
}

static int _add_extzone_figure_to_display(ITXDVAA_T *pdvaa, int page, EXTZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[MAX_PT] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;
    int i;

    ditid = pdvaa->ditid;

    for (i = 0; i < pzone->npts; ++i) {
        pt[i].x = pzone->pt[i].x;
        pt[i].y = pzone->pt[i].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }

    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    if ((pzone->npts >= 2) && (pt[0].x == pt[pzone->npts-1].x) && (pt[0].y == pt[pzone->npts-1].y)) finfo.st = 0;
    else finfo.st = 1;
    finfo.vt = 0;

    _construct_figure(pt, pzone->npts, &finfo);
    _makeup_extzone_figure(pzone, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pzone->dic[FIGURE] = pdic;
    pzone->dic[FIGURE]->en = pzone->use;
    
    return 0;
}

static int _add_extzone_name_to_display(ITXDVAA_T *pdvaa, int page, EXTZONEPTR pzone)
{
    TEXT_INFO tinfo;
    IGPOINT pt[MAX_PT] = {0,};
    guint col;
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;
    int i;

    ditid = pdvaa->ditid;

    for (i = 0; i < pzone->npts; ++i) {
        pt[i].x = pzone->pt[i].x;
        pt[i].y = pzone->pt[i].y;
        DMSG(9, "%d, %d\n", pt[i].x, pt[i].y);
        pt[i].opt = 0;
    }

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_name_position(pt, pzone->npts, &tinfo.pt);
    _makeup_extzone_name(pzone, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[INFO] = pdic;
    pzone->dic[INFO]->en = pzone->use;
    
    return 0;
}

static int _add_extzone_to_display(ITXDVAA_T *pdvaa, int page, EXTZONEPTR pzone)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_extzone_figure_to_display(pdvaa, page, pzone);
    _add_extzone_name_to_display(pdvaa, page, pzone);
    
    return 0;
}

static int _add_frzone_figure_to_display(ITXDVAA_T *pdvaa, int page, FRZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 0;
    finfo.vt = 0;
    
    _get_point_from_frzone(pt, &cnt, &pzone->zn_data);
    _construct_figure(pt, cnt, &finfo);
    _makeup_frzone_figure(pzone, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pzone->dic[FIGURE] = pdic;
    pzone->dic[FIGURE]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_frzone_name_to_display(ITXDVAA_T *pdvaa, int page, FRZONEPTR pzone)
{
    TEXT_INFO tinfo;
    guint col;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_frzone(pt, &cnt, &pzone->zn_data);
    _get_name_position(pt, cnt, &tinfo.pt);
    _makeup_frzone_name(pzone, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[INFO] = pdic;
    pzone->dic[INFO]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_frzone_direction_to_display(ITXDVAA_T *pdvaa, int page, FRZONEPTR pzone)
{
    DIRECTION_INFO dinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;    
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&dinfo, 0x00, sizeof(DIRECTION_INFO));
    _get_point_from_frzone(pt, &cnt, &pzone->zn_data);
    _construct_zone_direction_figure(pt, &dinfo);
    _makeup_frzone_direction_figure(pzone, &dinfo);

    pdic = dit_add_dic(ditid, page, DIC_DIR, &dinfo);
    pzone->dic[DIRCTRL] = pdic;     

    if (_IS_LINE(pzone)) pzone->dic[DIRCTRL]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[DIRCTRL]->en = 0;
    
    return 0;
}

static int _add_frzone_value_to_display(ITXDVAA_T *pdvaa, int page, FRZONEPTR pzone, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_frzone(pt, &cnt, &pzone->zn_data);
    _get_zone_value_position(pt, cnt, &tinfo.pt);
    _makeup_frzone_value(pzone, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[COUNT_VALUE] = pdic;
    
    if (pzone->value_en) pzone->dic[COUNT_VALUE]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[COUNT_VALUE]->en = 0;

    return 0;
}

static int _add_frzone_to_display(ITXDVAA_T *pdvaa, int page, FRZONEPTR pzone)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_frzone_figure_to_display(pdvaa, page, pzone);
    _add_frzone_name_to_display(pdvaa, page, pzone);
    _add_frzone_direction_to_display(pdvaa, page, pzone);
    _add_frzone_value_to_display(pdvaa, page, pzone, pzone->cnt_value);
    return 0;
}

static int _add_lprzone_figure_to_display(ITXDVAA_T *pdvaa, int page, LPRZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    finfo.wi = 3;
    finfo.st = 0;
    finfo.vt = 0;
    
    _get_point_from_lprzone(pt, &cnt, &pzone->zn_data);
    _construct_figure(pt, cnt, &finfo);
    _makeup_lprzone_figure(pzone, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pzone->dic[FIGURE] = pdic;
    pzone->dic[FIGURE]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_lprzone_name_to_display(ITXDVAA_T *pdvaa, int page, LPRZONEPTR pzone)
{
    TEXT_INFO tinfo;
    guint col;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_lprzone(pt, &cnt, &pzone->zn_data);
    _get_name_position(pt, cnt, &tinfo.pt);
    _makeup_lprzone_name(pzone, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[INFO] = pdic;
    pzone->dic[INFO]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_lprzone_direction_to_display(ITXDVAA_T *pdvaa, int page, LPRZONEPTR pzone)
{
    DIRECTION_INFO dinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;    
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;
    
    memset(&dinfo, 0x00, sizeof(DIRECTION_INFO));
    _get_point_from_lprzone(pt, &cnt, &pzone->zn_data);
    _construct_zone_direction_figure(pt, &dinfo);
    _makeup_lprzone_direction_figure(pzone, &dinfo);

    pdic = dit_add_dic(ditid, page, DIC_DIR, &dinfo);
    pzone->dic[DIRCTRL] = pdic;     

    if (_IS_LINE(pzone)) pzone->dic[DIRCTRL]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[DIRCTRL]->en = 0;
    
    return 0;
}

static int _add_lprzone_value_to_display(ITXDVAA_T *pdvaa, int page, LPRZONEPTR pzone, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16] = {0,};
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_lprzone(pt, &cnt, &pzone->zn_data);
    _get_zone_value_position(pt, cnt, &tinfo.pt);
    _makeup_lprzone_value(pzone, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[COUNT_VALUE] = pdic;
    
    if (pzone->value_en) pzone->dic[COUNT_VALUE]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[COUNT_VALUE]->en = 0;

    return 0;
}

static int _add_lprzone_to_display(ITXDVAA_T *pdvaa, int page, LPRZONEPTR pzone)
{
    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_lprzone_figure_to_display(pdvaa, page, pzone);
    _add_lprzone_name_to_display(pdvaa, page, pzone);
    _add_lprzone_direction_to_display(pdvaa, page, pzone);
    _add_lprzone_value_to_display(pdvaa, page, pzone, pzone->cnt_value);
    return 0;
}

static DICPTR _add_trackbox_to_display(ITXDVAA_T *pdvaa, int page, IGRECT *rect, int evt)
{
    FIGURE_INFO info;
    DITID ditid;
    DICPTR dic;

    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;
    ditid = pdvaa->ditid;

    memset(&info, 0x00, sizeof(FIGURE_INFO));
    _construct_trackbox_figure(rect, &info);
    _makeup_trackbox_figure(&info, evt);
    info.nr = 1;

    dic = dit_add_dic(ditid, page, DIC_FIG, &info);
    return dic;
}

static DICPTR _add_static_trackbox_to_display(ITXDVAA_T *pdvaa, int page, IGRECT *rect)
{
    FIGURE_INFO info;
    DITID ditid;
    DICPTR dic;

    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;
    ditid = pdvaa->ditid;

    memset(&info, 0x00, sizeof(FIGURE_INFO));
    _construct_trackbox_figure(rect, &info);
    _makeup_static_trackbox_figure(&info);
    info.nr = 1;

    dic = dit_add_dic(ditid, page, DIC_FIG, &info);
    return dic;
}

static DICPTR _add_trackbox_obj_info_to_display(ITXDVAA_T *pdvaa, int page, IGRECT *rect, char *buf, int evt)
{
    TEXT_INFO tinfo;
    DITID ditid;
    DICPTR dic;

    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;
    ditid = pdvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _makeup_text_trackbox(&tinfo, buf, rect, evt);
    tinfo.nr = 1;

    dic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    return dic;
}

static DICPTR _add_trajectory_to_display(ITXDVAA_T *pdvaa, int page, int npt, IXPOINT *pt, int evt)
{
    FIGURE_INFO info;
    DITID ditid;
    DICPTR dic;

    if (!dvaa_itx_is_dit_linked(pdvaa)) return -1;
    ditid = pdvaa->ditid;

    memset(&info, 0x00, sizeof(FIGURE_INFO));
    _construct_trajectory_figure(npt, pt, &info);
    _makeup_trajectory_figure(&info, evt);
    info.nr = 1;

    dic = dit_add_dic(ditid, page, DIC_FIG, &info);
    return dic;
}

static int _add_zones_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    DVAZONEPTR pzone;    
    DITID ditid = pdvaa->ditid;

    for (i = 0; i < MAX_ZONE; ++i) 
    {   
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, (ITX_ZONEID)i);
        page = pdvaa->dvazone[i].dipage;
        _add_zone_to_display(pdvaa, page, pzone);
    }

    return 0;
}

static int _add_cntrs_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    DVACNTRPTR pcntr;    
    DITID ditid = pdvaa->ditid;

    for (i = 0; i < MAX_CNTR; ++i) 
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, (ITX_CNTRID)i);
        page = pdvaa->dvacntr[i].dipage;
        _add_cntr_to_display(pdvaa, page, pcntr);
    }

    return 0;
}

static int _add_calbs_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    DVACALBPTR pcalb;    
    DITID ditid = pdvaa->ditid;
    
    if (!ivsc.dfunc.ai.support_calibration) {
        return 0;
    }

    for (i = 0; i < MAX_CALB; ++i) 
    {
        pcalb = _dvaa_itx_get_calb_ptr(pdvaa, (ITX_CALBID)i);
        page = pdvaa->dvacalb[i].dipage;
        _add_calb_to_display(pdvaa, page, pcalb);
    }

    return 0;
}

static int _add_extzones_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    EXTZONEPTR pzone;    
    DITID ditid = pdvaa->ditid;

    for (i = 0; i < MAX_EXTZONE; ++i) 
    {   
        pzone = _dvaa_itx_get_extzone_ptr(pdvaa, (ITX_EXTZONEID)i);
        page = pdvaa->extzone[i].dipage;
        _add_extzone_to_display(pdvaa, page, pzone);
    }

    return 0;
}

static int _add_frzones_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    FRZONEPTR pzone;    
    DITID ditid = pdvaa->ditid;
    
    if (!ivsc.dfunc.support_face) {
        return 0;
    }

    for (i = 0; i < MAX_FRZONE; ++i) 
    {   
        pzone = _dvaa_itx_get_frzone_ptr(pdvaa, (ITX_FRZONEID)i);
        page = pdvaa->frzone[i].dipage;
        _add_frzone_to_display(pdvaa, page, pzone);
    }

    return 0;
}

static int _add_lprzones_to_display(ITXDVAA_T *pdvaa)
{
    int i, page;
    LPRZONEPTR pzone;    
    DITID ditid = pdvaa->ditid;
    
    if (!ivsc.dfunc.support_license_plate) {
        return 0;
    }

    for (i = 0; i < MAX_LPRZONE; ++i) 
    {   
        pzone = _dvaa_itx_get_lprzone_ptr(pdvaa, (ITX_LPRZONEID)i);
        page = pdvaa->lprzone[i].dipage;
        _add_lprzone_to_display(pdvaa, page, pzone);
    }

    return 0;
}

static int _redraw_rules_to_display(ITXDVAA_T *pdvaa)
{   
    dit_clear_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
    dit_clear_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
    dit_clear_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
    dit_clear_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
    dit_clear_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);

    _add_zones_to_display(pdvaa);
    _add_cntrs_to_display(pdvaa);   
    _add_extzones_to_display(pdvaa);   
    _add_frzones_to_display(pdvaa);   
    _add_lprzones_to_display(pdvaa);   
    return 0;
}

static int _redraw_calbs_to_display(ITXDVAA_T *pdvaa)
{  
    dit_clear_page(pdvaa->ditid, pdvaa->dvacalb[0].dipage);
    _add_calbs_to_display(pdvaa);    
    return 0;
}

static int _control_dit(ITXDVAA_T *pdvaa)
{
    dit_enable(pdvaa->ditid);

    _redraw_rules_to_display(pdvaa);
    _redraw_calbs_to_display(pdvaa);    
    return 0;
}

static int _link_dit(ITXDVAA_T *pdvaa, DITID ditid)
{
    pdvaa->ditid = ditid;    
    dit_disable_all_page(pdvaa->ditid);
    return 0;
}

static int _get_zone_id(ITXDVAA_T *pdvaa, int evt_zoneid)
{
	int i;

	for (i = 0; i < MAX_ZONE; ++i)
		if (pdvaa->dvazone[i].zn_data.id == evt_zoneid) return i;

	return -1;

}

static int _get_cntr_id(ITXDVAA_T *pdvaa, int evt_cntrid)
{
	int i;

	for (i = 0; i < MAX_CNTR; ++i)
		if (pdvaa->dvacntr[i].ct_data.id == evt_cntrid) return i;

	return -1;

}

static int _proc_zone_event_detecting(ITXDVAA_T *pdvaa, DVAZONEPTR pzone)
{
    if (pzone->event_time != -1) 
    {
        if (pzone->event_time == 20)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[DIRCTRL]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[COUNT_VALUE]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
        }
        else if (pzone->event_time == 0)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[DIRCTRL]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[COUNT_VALUE]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.color);
        }

        --pzone->event_time;
    }
    return 0;
}

static int _proc_extzone_event_detecting(ITXDVAA_T *pdvaa, EXTZONEPTR pzone)
{
    if (pzone->event_time != -1) 
    {
        if (pzone->event_time == 20)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);

            pzone->sty_highlight = 1;
        }
        else if (pzone->event_time == 0)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->color);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->color);

            pzone->sty_highlight = 0;
        }

        --pzone->event_time;
    }
    return 0;
}

static int _proc_frzone_event_detecting(ITXDVAA_T *pdvaa, FRZONEPTR pzone)
{
    if (pzone->event_time != -1) 
    {
        if (pzone->event_time == 20)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);

            pzone->sty_highlight = 1;
        }
        else if (pzone->event_time == 0)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);

            pzone->sty_highlight = 0;
        }

        --pzone->event_time;
    }
    return 0;
}

static int _proc_lprzone_event_detecting(ITXDVAA_T *pdvaa, LPRZONEPTR pzone)
{
    if (pzone->event_time != -1) 
    {
        if (pzone->event_time == 20)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(CR_EVENT_RULE);

            pzone->sty_highlight = 1;
        }
        else if (pzone->event_time == 0)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);
            pzone->dic[INFO]->ctns.t.cl.i = dvaa_itx_convert_rgb_color_to_coloridx(pzone->zn_data.d_color);

            pzone->sty_highlight = 0;
        }

        --pzone->event_time;
    }
    return 0;
}

static int _proc_event_detecting(ITXDVAA_T *pdvaa)
{
    int i;
    DVAZONEPTR pzone;
    EXTZONEPTR pextzone;
    FRZONEPTR pfrzone;
    LPRZONEPTR plprzone;
    
    for (i = 0; i < MAX_ZONE; ++i) {
        pzone = _dvaa_itx_get_zone_ptr(pdvaa, (ITX_ZONEID)i);
        if (!pzone->occupied) continue;

        _proc_zone_event_detecting(pdvaa, pzone);
    }

    for (i = 0; i < MAX_EXTZONE; ++i) {
        pextzone = _dvaa_itx_get_extzone_ptr(pdvaa, (ITX_EXTZONEID)i);
        if (!pextzone->occupied) continue;

        _proc_extzone_event_detecting(pdvaa, pextzone);
    }

    for (i = 0; i < MAX_FRZONE; ++i) {
        pfrzone = _dvaa_itx_get_frzone_ptr(pdvaa, (ITX_FRZONEID)i);
        if (!pfrzone->occupied) continue;

        _proc_frzone_event_detecting(pdvaa, pfrzone);
    }

    for (i = 0; i < MAX_LPRZONE; ++i) {
        plprzone = _dvaa_itx_get_lprzone_ptr(pdvaa, (ITX_LPRZONEID)i);
        if (!plprzone->occupied) continue;

        _proc_lprzone_event_detecting(pdvaa, plprzone);
    }    

    return 0;
}

static int _proc_meta_erasing(ITXDVAA_T *pdvaa)
{
    int i;
    int dipage;

    dipage = pdvaa->dvameta[0].dipage;
    for (i = 0; i < pdvaa->cnt_tbox; ++i) {
        if (pdvaa->dvatbox[i].dic[TRACKBOX] == 0) continue;

        --pdvaa->dvatbox[i].age;
        if (pdvaa->dvatbox[i].age == 0) {
            dit_remove_dic(pdvaa->ditid, dipage, pdvaa->dvatbox[i].dic[TRACKBOX]);
            dit_remove_dic(pdvaa->ditid, dipage, pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_INFO]);
            if(pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA])
                dit_remove_dic(pdvaa->ditid, dipage, pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA]);
            pdvaa->dvatbox[i].dic[TRACKBOX] = 0;
            pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
            pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;
        }
    }
    return 0;
}

static int _clear_meta(ITXDVAA_T *pdvaa)
{
    int i;
    int dipage;
    for (i = 0; i < MAX_TBOX; ++i) {
        pdvaa->dvatbox[i].dic[TRACKBOX] = 0;
        pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
        pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;
        pdvaa->dvatbox[i].age = 0;
    }
    pdvaa->cnt_tbox = 0;

    dipage = pdvaa->dvameta[0].dipage;
    dit_clear_page(pdvaa->ditid, dipage);
    return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces 
//




////////////////////////////////////////////////////////////
//
// public interfaces 
//

int dvaa_itx_init(int ch, DVAAID *pdvaaid, DITID *pditid)
{
    ITXDVAA_T *pdvaa;
    *pdvaaid = dvaa_itx_create(ch);
    *pditid = dit_create(10, 3840, 2160);

    pdvaa = *pdvaaid;
    _link_dit(pdvaa, *pditid);
    _dvaa_itx_detector_init(pdvaa);
    _dvaa_itx_face_init(pdvaa);
    _dvaa_itx_plateno_init(pdvaa);
    _control_dit(pdvaa);

    return 0;
}

int dvaa_itx_pb_init(int ch, DVAAID *pdvaaid, DITID *pditid)
{
    ITXDVAA_T *pdvaa;
    *pdvaaid = dvaa_itx_create(ch);
    *pditid = dit_create(10, 3840, 2160);

    pdvaa = *pdvaaid;
    _link_dit(pdvaa, *pditid);
    pdvaa->st_rule = 1;
    pdvaa->calbres.paramvalid = 0;
    _dvaa_itx_detector_pb_init(pdvaa);
    _dvaa_itx_face_pb_init(pdvaa);
    _dvaa_itx_plateno_pb_init(pdvaa);
    _control_dit(pdvaa);

    return 0;
}

DVAAID dvaa_itx_create(int ch)
{
    DVAAID dvaaid;
    ITXDVAA_T *pdvaa;

    dvaaid = (DVAAID)imalloc(sizeof(ITXDVAA_T));
    pdvaa = dvaaid;
    _init(pdvaa, ch);
    return dvaaid;
}

int dvaa_itx_destroy(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    ifree(pdvaa);
    return 0;
}

int dvaa_itx_reload(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _dvaa_itx_detector_reload(id);
    //_dvaa_itx_face_reload(id);
    //_dvaa_itx_plateno_reload(id);
    _control_dit(pdvaa);
    return 0;
}

int dvaa_itx_enable(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->prop.active = TRUE;
    return 0;
}

int dvaa_itx_disable(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->prop.active = FALSE;
    return 0;
}

int dvaa_itx_enable_strule(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->st_rule = 1;
    return 0;
}

int dvaa_itx_disable_strule(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->st_rule = 0;
    return 0;
}

int dvaa_itx_active_external_rule(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->ext_rule = 1;
    return 0;
}

int dvaa_itx_inactive_external_rule(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->ext_rule = 0;
    return 0;
}

int dvaa_itx_active_algotithm(DVAAID id, ITX_ALGOTYPE_E algo_type)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    pdvaa->algo_type = algo_type;
    return 0;
}

int dvaa_itx_raiseup(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _dvaa_itx_link_event(id);    
    return 0;
}

int dvaa_itx_link_dit(DVAAID id, DITID ditid)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _link_dit(pdvaa, ditid);

    DMSG(9, "ditid = %p\n", pdvaa->ditid);
    _redraw_rules_to_display(pdvaa);
    return 0;
}

int dvaa_itx_unlink_dit(DVAAID id, DITID ditid)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    dit_disable_all_page(pdvaa->ditid);
    pdvaa->ditid = 0;    
    return 0;
}

int dvaa_itx_is_dit_linked(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    return (pdvaa->ditid != 0);
}

DITID dvaa_itx_get_dit(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    return pdvaa->ditid;
}

int dvaa_itx_load_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (_dvaa_itx_detector_is_db_changed(id)) {
        _dvaa_itx_detector_load_db(id);
    }
    //_dvaa_itx_face_load_db(id);
    //_dvaa_itx_plateno_load_db(id);
    _control_dit(pdvaa);
    return 0;
}

int dvaa_itx_save_db(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    _dvaa_itx_detector_save_db(id);
    //_dvaa_itx_face_save_db(id);
    //_dvaa_itx_plateno_save_db(id);
    return 0;
}

int dvaa_itx_activate_rule(DVAAID id, DIT_PAGE_E page)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;

    if (page == DIT_PAGE_ZONE)
    {
        if (_is_enable_detector_engine(pdvaa) && _is_display_rule(pdvaa)) {
            dit_enable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        } else {
            dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        }
    }
    else if (page == DIT_PAGE_CNTR)
    {
        if (_is_enable_detector_engine(pdvaa) && _is_display_rule(pdvaa)) {
            dit_enable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        } else {
            dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        }
    }
    else if (page == DIT_PAGE_FRZONE)
    {
        if (_is_enable_fr_engine(pdvaa) && _is_display_rule(pdvaa)) {
            dit_enable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        } else {
            dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        }
    }
    else if (page == DIT_PAGE_LPRZONE)
    {
        if (_is_enable_lpr_engine(pdvaa) && _is_display_rule(pdvaa)) {
            dit_enable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
        } else {
            dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
        }
    }        
    else if (page == DIT_PAGE_EXTZONE)
    {
        if (_is_enable_external_engine(pdvaa) && _is_display_rule(pdvaa)) {
            dit_enable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        } else {
            dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        }
    }

    _redraw_rules_to_display(pdvaa);
    return 0;
}

int dvaa_itx_activate_all_rule(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;

    if (_is_enable_detector_engine(pdvaa) && _is_display_rule(pdvaa))
    {
        dit_enable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        dit_enable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
    }
    else if (_is_enable_external_engine(pdvaa) && _is_display_rule(pdvaa))
    {
        dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        dit_enable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
    }
    else if (_is_enable_fr_engine(pdvaa) && _is_display_rule(pdvaa))
    {
        dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        dit_enable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
    }
    else if (_is_enable_lpr_engine(pdvaa) && _is_display_rule(pdvaa))
    {
        dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        dit_enable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);        
    }    
    else
    {
        dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
        dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);        
    }

    _redraw_rules_to_display(pdvaa);
    return 0;
}

int dvaa_itx_deactivate_rule(DVAAID id, DIT_PAGE_E page)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    if (page == DIT_PAGE_ZONE) dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
    else if (page == DIT_PAGE_CNTR) dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
    else if (page == DIT_PAGE_EXTZONE) dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
    else if (page == DIT_PAGE_FRZONE) dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
    else if (page == DIT_PAGE_LPRZONE) dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
        
    _redraw_rules_to_display(pdvaa);
    return 0;
}

int dvaa_itx_deactivate_all_rule(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
  
    dit_disable_page(pdvaa->ditid, pdvaa->dvazone[0].dipage);
    dit_disable_page(pdvaa->ditid, pdvaa->dvacntr[0].dipage);
    dit_disable_page(pdvaa->ditid, pdvaa->extzone[0].dipage);
    dit_disable_page(pdvaa->ditid, pdvaa->frzone[0].dipage);
    dit_disable_page(pdvaa->ditid, pdvaa->lprzone[0].dipage);
    
    _redraw_rules_to_display(pdvaa);
    return 0;
}

int dvaa_itx_activate_calb(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    dit_enable_page(pdvaa->ditid, pdvaa->dvacalb[0].dipage);       
    _redraw_calbs_to_display(pdvaa);
    return 0;
}

int dvaa_itx_deactivate_calb(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    dit_disable_page(pdvaa->ditid, pdvaa->dvacalb[0].dipage);       
    _redraw_calbs_to_display(pdvaa);
    return 0;
}

int dvaa_itx_activate_meta(DVAAID id, DVAA_META_E meta)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    dit_enable_page(pdvaa->ditid, pdvaa->dvameta[meta].dipage);
    return 0;
}

int dvaa_itx_activate_all_meta(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    for (i = 0; i < MAX_META; ++i) {
        dit_enable_page(pdvaa->ditid, pdvaa->dvameta[i].dipage);
    }

    return 0;
}

int dvaa_itx_deactivate_meta(DVAAID id, DVAA_META_E meta)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    dit_disable_page(pdvaa->ditid, pdvaa->dvameta[meta].dipage);
    return 0;
}

int dvaa_itx_deactivate_all_meta(DVAAID id)
{
    int i;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    for (i = 0; i < MAX_META; ++i)
        dit_disable_page(pdvaa->ditid, pdvaa->dvameta[i].dipage);

    return 0;
}

int dvaa_itx_deactivate_all(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    if (!dvaa_itx_is_dit_linked(id)) return -1;
   
    dvaa_itx_deactivate_all_rule(id);
    dvaa_itx_deactivate_calb(id);    
    dvaa_itx_deactivate_all_meta(id);

    return 0;
}

int dvaa_itx_is_db_changed(DVAAID id)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return 0;
    int ret_code = 0;

//_print_db(pdvaa);
    ret_code |= _dvaa_itx_detector_is_db_changed(id);
    //ret_code |= _dvaa_itx_face_is_db_changed(id);
    //ret_code |= _dvaa_itx_plateno_is_db_changed(id);

    return ret_code;
}

int dvaa_itx_show_meta(DVAAID id, gint cnt, ai_obj_t* data)
{
    int i, dipage, evt, j;
    float value;

    IGRECT rect;
    IXPOINT pt[MAX_PT];
    int npt;
    char strBuf[64];
    char strTemp[32];

    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    if (!pdvaa) return -1;

    dipage = pdvaa->dvameta[0].dipage;
    _clear_meta(pdvaa);

    if (cnt > MAX_TBOX) cnt = MAX_TBOX;

    // display meta data
    for (i = 0; i < cnt; ++i) 
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        pdvaa->dvatbox[i].dic[TRACKBOX] = 0;
        pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
        pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;

        _get_meta_bound(&data[i].mobj, &rect);
        evt = _is_evented_meta(&data[i].mobj);

        if (_is_static_object(&data[i].mobj))
        {
            pdvaa->dvatbox[i].dic[TRACKBOX] = _add_static_trackbox_to_display(pdvaa, dipage, &rect);
            continue;
        }

        if(_is_display_object_id(pdvaa))                     // display object id
        {
            memset(strBuf, 0x00, sizeof(strBuf));

            if(data[i].mobj.id >= 0xff000000)
                snprintf(strBuf, sizeof(strBuf), "%s (%d%%)", data[i].mobj.object_class, (guint)(data[i].mobj.confidence*(double)100));
            else
                snprintf(strBuf, sizeof(strBuf), "%s id:%u (%d%%)", data[i].mobj.object_class, data[i].mobj.id, (guint)(data[i].mobj.confidence*(double)100));
        }

        if(_is_3d_calbres_valid(pdvaa) && _is_display_object_w3d(pdvaa))               // display object width(3D)
        {
            memset(strTemp, 0x00, sizeof(strTemp));

            if(data[i].mobj.width3d)
            {
                value = (float)data[i].mobj.width3d / 256;
                snprintf(strTemp, sizeof(strTemp), "W=%.1fm", value);
            }
            else
                snprintf(strTemp, sizeof(strTemp), "W=0m");

            if (strlen(strBuf)) snprintf(strBuf+strlen(strBuf), sizeof(strBuf)-strlen(strBuf), ",%s", strTemp);
            else snprintf(strBuf, sizeof(strBuf), "%s", strTemp);
        }       

        if(_is_3d_calbres_valid(pdvaa) && _is_display_object_h3d(pdvaa))               // display object height(3D)
        {
            memset(strTemp, 0x00, sizeof(strTemp));

            if(data[i].mobj.height3d)
            {
                value = (float)data[i].mobj.height3d / 256;
                snprintf(strTemp, sizeof(strTemp), "H=%.1fm", value);
            }
            else
                snprintf(strTemp, sizeof(strTemp), "H=0m");

            if (strlen(strBuf)) snprintf(strBuf+strlen(strBuf), sizeof(strBuf)-strlen(strBuf), ",%s", strTemp);
            else snprintf(strBuf, sizeof(strBuf), "%s", strTemp);                
        }

        if(_is_3d_calbres_valid(pdvaa) && _is_display_object_s3d(pdvaa))                // display object speed(3D)
        {
            memset(strTemp, 0x00, sizeof(strTemp));

            if(data[i].mobj.speed3d)
            {
                value = (float)data[i].mobj.speed3d/256;
                snprintf(strTemp, sizeof(strTemp), "S=%.1fkm/h", value);
            }
            else
                snprintf(strTemp, sizeof(strTemp), "S=0km/h");

            if (strlen(strBuf)) snprintf(strBuf+strlen(strBuf), sizeof(strBuf)-strlen(strBuf), ",%s", strTemp);
            else snprintf(strBuf, sizeof(strBuf), "%s", strTemp);                
        }

        if (_is_display_bounding_box(pdvaa))                 // display bounding box
        {
            pdvaa->dvatbox[i].dic[TRACKBOX] = _add_trackbox_to_display(pdvaa, dipage, &rect, evt);
            pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_INFO] = _add_trackbox_obj_info_to_display(pdvaa, dipage, &rect, strBuf, evt);    
        }
        
        if (_is_display_trajectory(pdvaa))                   // display trajectory
        {
            int adj_flag;

            memset(pt , 0, sizeof(pt));

            if(data[i].npts >0)
                adj_flag = (data[i].npts-1)/MAX_PT + 1;
            else
                adj_flag = 1;

            if(adj_flag > 0)
                npt = data[i].npts/adj_flag;
            else
                npt = 0;

            for (j = 0; j < npt; j++) {
                if(data[i].npts-j*adj_flag-1 < 0)
                    break;
                pt[j].x = data[i].bbx_traj[data[i].npts-j*adj_flag-1][0] * 3840; 
                pt[j].y = data[i].bbx_traj[data[i].npts-j*adj_flag-1][1] * 2160;
            }

            pdvaa->dvatbox[i].dic[TRACKBOX_OBJ_TRA] = _add_trajectory_to_display(pdvaa, dipage, npt, &pt , evt);
            pdvaa->dvatbox[i].age = SEC_2;
        }
    }

    pdvaa->cnt_tbox = cnt;


    return 0;
}

int dvaa_itx_show_event(DVAAID id, gint cnt, ai_rule_event_t* data)
{
    int i;
    ITX_ZONEID zone;
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!_is_display_rule(pdvaa)) return -1;
    if (!_is_enable_detector_engine(pdvaa)) return -1;

    for (i = 0; i < cnt; ++i) 
    {   
        zone = _get_matched_zoneid(pdvaa, data[i].rule_id);

        if (zone != -1) {
            pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
            pzone->event_time = 20;
                
            DMSG(9, "event type = %x\n", data->type);
        }
    }

    return 0;
}

int dvaa_itx_show_generic_event(DVAAID id, gint cnt, ai_generic_event_t* data)
{
    int i, j;
    EXTZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    
    if (!pdvaa) return -1;
    if (!_is_display_rule(pdvaa)) return -1;
    if (!_is_enable_external_engine(pdvaa)) return -1;

    for (i = 0; i < cnt; ++i) 
    {   
        for (j = 0; j < pdvaa->cnt_extzone; ++j)
        {
            pzone = _dvaa_itx_get_extzone_ptr(pdvaa, j);

            //g_message("%s, %d, id:%d, %s, %s", __FUNCTION__, __LINE__, j, data->trigger_type, pzone->trigger_type);
            //g_message("%s, %d, id:%d, %s, %s", __FUNCTION__, __LINE__, j, data->trigger_name, pzone->trigger_name);

            if ((strcmp(data->trigger_type, pzone->trigger_type) == 0) && (strcmp(data->trigger_name, pzone->trigger_name) == 0))
            {
                pzone->event_time = 20;
            }
        }
    }

    return 0;
}

int dvaa_itx_show_zone_value_event(DVAAID id, gint cnt, ai_rule_event_t* data)
{
    int i, page;
    ITX_ZONEID zone;    
    DVAZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;

    if (!pdvaa) return -1;
    if (!_is_display_rule(pdvaa)) return -1;
    if (!_is_enable_detector_engine(pdvaa)) return -1;

    for (i = 0; i < cnt; ++i) 
    {   
        zone = _get_matched_zoneid(pdvaa, data[i].rule_id);

        if (zone != -1) {
            pzone = _dvaa_itx_get_zone_ptr(pdvaa, zone);
            page = pdvaa->dvazone[zone].dipage;            
            dit_remove_dic(pdvaa->ditid, page, pzone->dic[COUNT_VALUE]);            

            pzone->cnt_value++;
            _add_zone_value_to_display(pdvaa, page, pzone, pzone->cnt_value);
        }
    }

    return 0;
}

int dvaa_itx_show_counter_value_event(DVAAID id, gint count, ai_meta_cnt_t* data)
{
    int i, page;
    DVACNTRPTR pcntr;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;
    ai_meta_cnt_t *p = data;

    if (!pdvaa) return -1;
    if (!_is_display_rule(pdvaa)) return -1;
    if (!_is_enable_detector_engine(pdvaa)) return -1;

#if 0
    pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, p->id);    
    page = pdvaa->dvacntr[p->id].dipage;
    dit_remove_dic(pdvaa->ditid, page, pcntr->dic[COUNT_VALUE]);

    _add_cntr_value_to_display(pdvaa, page, pcntr, p->value);
    pcntr->cnt_value = p->value;
#else
    for ( i = 0; i < count; i++, p++)
    {
        pcntr = _dvaa_itx_get_cntr_ptr(pdvaa, p->id);
        page = pdvaa->dvacntr[p->id].dipage;
        dit_remove_dic(pdvaa->ditid, page, pcntr->dic[COUNT_VALUE]);
        
        _add_cntr_value_to_display(pdvaa, page, pcntr, p->value);
        pcntr->cnt_value = p->value;
    }
#endif

    return 0;
}

ITX_CNTRID dvaa_itx_get_counted_rule(DVAAID id, unsigned short countid)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;

    if (!pdvaa) return -1;

    _get_matched_cntrid(pdvaa, countid);
    return -1;
}

int dvaa_itx_parse_event(DVAAID dvaaid, ai_rule_event_t *pevt, int *zid, int *cid)
{
	ITXDVAA_T *pdvaa = (ITXDVAA_T *)dvaaid;
	int ret = ITX_RULETYPE_NONE;

	if (!pdvaa) return ITX_RULETYPE_NONE;

	*zid = -1;
	*cid = -1;

	if (pevt->type & IVCA_ET_COUNTER) {
		ret = ITX_RULETYPE_COUNTER; 
		*cid = _get_cntr_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_INTRUSION) {
		ret = ITX_RULETYPE_INTRUSION; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_ENTER) {
		ret = ITX_RULETYPE_ENTER; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_EXIT) {
		ret = ITX_RULETYPE_EXIT; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_STOPPED) {
		ret = ITX_RULETYPE_STOPPED; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_LOITERED) {
		ret = ITX_RULETYPE_LOITERED; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_FALL) {
		ret = ITX_RULETYPE_FALL; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_REMOVED) {
		ret = ITX_RULETYPE_REMOVED; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_DIR_POS) {
		ret = ITX_RULETYPE_FOWARD; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}
    else if (pevt->type & IVCA_ET_DIR_NEG) {
		ret = ITX_RULETYPE_REVERSE; 
		*zid = _get_zone_id(pdvaa, pevt->rule_id);
	}

	return (int)ret;    
}

int dvaa_itx_all_external_rules_proc(DVAAID id, int cnt, aibox_rule_data *prules)
{
    int i, j, page;
    ITX_EXTZONEID extzone;    
    EXTZONEPTR pzone;
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)id;

    IGPOINT pt[MAX_PT] = {0,};
    int ptcnt;

    if (!pdvaa) return -1;

    if (cnt > MAX_EXTZONE) pdvaa->cnt_extzone = MAX_EXTZONE;
    else pdvaa->cnt_extzone = cnt;

    for (i = 0; i < MAX_EXTZONE; ++i) 
    {   
        pzone = _dvaa_itx_get_extzone_ptr(pdvaa, i);
        page = pdvaa->extzone[i].dipage;

        pzone->use = 0;
        pzone->occupied = 0;
        pzone->dic[FIGURE]->en = 0;
        pzone->dic[INFO]->en = 0;
        pzone->event_time = -1;

        if (i >= pdvaa->cnt_extzone) continue;
        if (prules[i].zone_size > MAX_PT) continue;

        memset(pt, 0x00, sizeof(IGPOINT) * MAX_PT);
        pzone->use = 1;
        pzone->occupied = 1;
        pzone->dic[FIGURE]->en = 1;
        pzone->dic[INFO]->en = 1;

        ptcnt = prules[i].zone_size;

        for (j = 0; j < ptcnt; ++j) {
            pt[j].x = (int)(prules[i].zone_list[j].x * 3840.0);
            pt[j].y = (int)(prules[i].zone_list[j].y * 2160.0);
        }

        _set_extzone_figure(pzone, pt, ptcnt, COLOR_PRG_IDX(UX_COLOR_BFFF00));
        _set_extzone_name(pzone, prules[i].event_type, prules[i].name, COLOR_PRG_IDX(UX_COLOR_BFFF00));
    }

    return 0;
}

int dvaa_itx_timer_proc(DVAAID dvaaid)
{
    ITXDVAA_T *pdvaa = (ITXDVAA_T *)dvaaid;
    if (!pdvaa) return -1;

    _proc_event_detecting(pdvaa);
    _proc_meta_erasing(pdvaa);

    return 0;
}
