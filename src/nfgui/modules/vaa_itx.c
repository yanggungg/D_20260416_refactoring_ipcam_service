/*
 * vaa_itx.c
 *  - video analytics agent for itx
 *  - dependencies :
 *      
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#include "dit.h"
#include "vaa_itx.h"
#include "vaa_itx_internal.h"
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
#define DBG_MODULE      "VAA_ITX"

#define ITX_MAX_ZONE        16
#define ITX_MAX_CNTR        16
#define ITX_MAX_CALB        32

#define CR_NORMAL_LINE      1102
#define CR_NORMAL_AREA      1105
#define CR_NORMAL_TBOX      1104
#define CR_EVENT_TBOX       1103
#define CR_EVENT_RULE       COLOR_FF0000
#define IS_LINE_FIGURE(f)   (f->cnt == 2)
#define SEC_2           (4)     // 2000ms

#define FIGURE          0
#define INFO            1
#define DIRCTRL         2
#define COUNT_VALUE     3
#define CALB_ICUPP      4
#define CALB_ICLOW      5


////////////////////////////////////////////////////////////
//
// private data type
//

typedef ITX_VAZONE*     VAZONEPTR;
typedef ITX_VACNTR*     VACNTRPTR;
typedef ITX_VACALB*     VACALBPTR;


////////////////////////////////////////////////////////////
//
// private variables
//

static char *s_dirimg_name[8] = {   // placed in counterclockwise
        IMG_VA_ARROW_LEFT,
        IMG_VA_ARROW_DOWNLEFT,
        IMG_VA_ARROW_DOWN,
        IMG_VA_ARROW_DOWNRIGHT,
        IMG_VA_ARROW_RIGHT,
        IMG_VA_ARROW_UPRIGHT,
        IMG_VA_ARROW_UP,
        IMG_VA_ARROW_UPLEFT,
};

enum {
    TRACKBOX            = 0,
    TRACKBOX_OBJ_INFO,
    TRACKBOX_OBJ_TRA
};

enum {
    COLOR_FF0000 = 0xff,
    COLOR_FF3F00 = 0x3fff,
    COLOR_FF7F00 = 0x7fff,
    COLOR_FFBF00 = 0xbfff,
    COLOR_FFFF00 = 0xffff,
    COLOR_BFFF00 = 0xffbf,
    COLOR_202020 = 0x202020,

    COLOR_7FFF00 = 0xff7f,
    COLOR_3FFF00 = 0xff3f,
    COLOR_00FF00 = 0xff00,
    COLOR_00FF3F = 0x3fff00,
    COLOR_00FF7F = 0x7fff00,
    COLOR_00FFBF = 0xbfff00,
    COLOR_606060 = 0x606060,

    COLOR_00FFFF = 0xffff00,
    COLOR_00BFFF = 0xffbf00,
    COLOR_007FFF = 0xff7f00,
    COLOR_003FFF = 0xff3f00,
    COLOR_0000FF = 0xff0000,
    COLOR_3F00FF = 0xff003f,
    COLOR_A0A0A0 = 0xa0a0a0,

    COLOR_7F00FF = 0xff007f,
    COLOR_BF00FF = 0xff00bf,
    COLOR_FF00FF = 0xff00ff,
    COLOR_FF00BF = 0xbf00ff,
    COLOR_FF007F = 0x7f00ff,
    COLOR_FF003F = 0x3f00ff,
    COLOR_FFFFFF = 0xffffff,
    COLOR_000000 = 0x000000
};




////////////////////////////////////////////////////////////
//
// private functions
//

static int _init_zone(VAZONEPTR pzone)
{
    memset(pzone, 0x00, sizeof(ITX_VAZONE));
    pzone->zn_data.id = -1;
    pzone->dipage = DIT_PAGE_ZONE;
    return 0;   
}

static int _init_cntr(VACNTRPTR pcntr)
{
    memset(pcntr, 0x00, sizeof(ITX_VACNTR));   
    pcntr->ct_data.id = -1;
    pcntr->dipage = DIT_PAGE_CNTR;
    return 0;   
}

static int _init_calb(VACALBPTR pcalb)
{
    memset(pcalb, 0x00, sizeof(ITX_VACALB));   
    pcalb->dipage = DIT_PAGE_CALB;
    return 0;   
}

static int _init_meta(ITXVAA_T *pvaa)
{
    pvaa->cnt_meta = 1;

    pvaa->vameta[0].meta = VAA_META_BBOX;
    pvaa->vameta[0].dipage = DIT_PAGE_META;

    pvaa->vameta[1].meta = VAA_META_TRAJ;
    pvaa->vameta[1].dipage = DIT_PAGE_META;

    pvaa->vameta[2].meta = -1;
    pvaa->vameta[2].dipage = DIT_PAGE_META;

    pvaa->vameta[3].meta = -1;
    pvaa->vameta[3].dipage = DIT_PAGE_META;
    return 0;
}

static int _init(ITXVAA_T *pvaa, int ch)
{
    memset(pvaa, 0x00, sizeof(ITXVAA_T));
    pvaa->ch = ch;

    _init_meta(pvaa);
    return 0;   
}

static VAZONEPTR _get_zone_ptr(ITXVAA_T *pvaa, ITX_ZONEID zone)
{
    return (VAZONEPTR)&pvaa->vazone[zone];
}

static VACNTRPTR _get_cntr_ptr(ITXVAA_T *pvaa, ITX_CNTRID cntr)
{
    return (VACNTRPTR)&pvaa->vacntr[cntr];
}

static VACALBPTR _get_calb_ptr(ITXVAA_T *pvaa, ITX_CALBID calb)
{
    return (VACALBPTR)&pvaa->vacalb[calb];
}

static ITX_ZONEID _get_matched_zoneid(ITXVAA_T *pvaa, int zoneid)
{
    int i;

    for (i = 0; i < MAX_ZONE; ++i)
        if (pvaa->vazone[i].zn_data.id == zoneid) return i;

    return -1;
}

static ITX_CNTRID _get_matched_cntrid(ITXVAA_T *pvaa, int countid)
{
    int i;
    
    for (i = 0; i < MAX_CNTR; ++i)
        if (pvaa->vacntr[i].ct_data.id == countid) return i;

    return -1;
}

static int _copy_prop_to_db(VCAPropData *src, VCAData *pdb)
{
    memcpy(&pdb->prop, src, sizeof(VCAPropData));
    return 0;
}

static int _copy_db_to_prop(VCAData *pdb, VCAPropData *dst)
{
    memcpy(dst, &pdb->prop, sizeof(VCAPropData));
    return 0;
}

static int _load_zone_data(VCAZone *zn_data, VAZONEPTR pzone)
{
    if (zn_data) memcpy(&pzone->zn_data, zn_data, sizeof(VCAZone));
    return 0;
}

static int _load_cntr_data(VCACntr *ct_data, VACNTRPTR pcntr)
{
    if (ct_data) memcpy(&pcntr->ct_data, ct_data, sizeof(VCACntr));
    return 0;
}

static int _unload_zone_data(VAZONEPTR pzone, VCAZone *zn_data)
{
    if (zn_data) memcpy(zn_data, &pzone->zn_data, sizeof(VCAZone));
    return 0;
}

static int _unload_cntr_data(VACNTRPTR pcntr, VCACntr *ct_data)
{
    if (ct_data) memcpy(ct_data, &pcntr->ct_data, sizeof(VCACntr));
    return 0;
}

static int _load_calb_data(VCACalb *cb_data, VACALBPTR pcalb)
{
    if (cb_data) memcpy(&pcalb->cb_data, cb_data, sizeof(VCACalb));
    return 0;
}

static int _unload_calb_data(VACALBPTR pcalb, VCACalb *cb_data)
{
    if (cb_data) memcpy(cb_data, &pcalb->cb_data, sizeof(VCACalb));
    return 0;
}

static int _copy_calbres_to_db(VCACalbResData *src, VCAData *pdb)
{
    memcpy(&pdb->calbres, src, sizeof(VCACalbResData));
    return 0;
}

static int _copy_db_to_calbres(VCAData *pdb, VCACalbResData *dst)
{
    memcpy(dst, &pdb->calbres, sizeof(VCACalbResData));
    return 0;
}

static int _print_db(ITXVAA_T *pvaa)
{
    int i;
    
    for (i = 0; i < 16; ++i) {
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

static int _load_db(ITXVAA_T *pvaa)
{
    DAL_get_vca_data(&pvaa->db, pvaa->ch);
    //_print_db(pvaa);
    return 0;
}

static int _save_db(ITXVAA_T *pvaa)
{
    DAL_set_vca_data(pvaa->db, pvaa->ch);
    //_print_db(pvaa);
    return 0;
}

static int _is_setup_rule(ITXVAA_T *pvaa)
{
    return (pvaa->st_rule == TRUE);
}

static int _is_display_rule(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_rule == TRUE);
}

static int _is_display_bounding_box(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_bb == TRUE);
}

static int _is_display_object_id(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_id == TRUE);
}

static int _is_display_object_w3d(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_w3d == TRUE);
}

static int _is_display_object_h3d(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_h3d == TRUE);
}

static int _is_display_object_s3d(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_s3d == TRUE);
}

static int _is_display_trajectory(ITXVAA_T *pvaa)
{
    return (pvaa->prop.sw_obj_tr == TRUE);
}

static int _is_evented_meta(ivca_meta_obj_t *mobj)
{
    return (mobj->nevents > 0);
}

static int _is_3d_calbres_valid(ITXVAA_T *pvaa)
{
    return pvaa->calbres.paramvalid;
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

static int _convert_rgb_color_to_coloridx(unsigned int rgb_col)
{
    int idx = 0;

    switch(rgb_col)
    {
        case COLOR_FF0000: idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;
        case COLOR_FF3F00: idx = COLOR_PRG_IDX(UX_COLOR_FF3F00); break;
        case COLOR_FF7F00: idx = COLOR_PRG_IDX(UX_COLOR_FF7F00); break;
        case COLOR_FFBF00: idx = COLOR_PRG_IDX(UX_COLOR_FFBF00); break;
        case COLOR_FFFF00: idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case COLOR_BFFF00: idx = COLOR_PRG_IDX(UX_COLOR_BFFF00); break;
        case COLOR_202020: idx = COLOR_PRG_IDX(UX_COLOR_000000); break;
        case COLOR_7FFF00: idx = COLOR_PRG_IDX(UX_COLOR_7FFF00); break;
        case COLOR_3FFF00: idx = COLOR_PRG_IDX(UX_COLOR_3FFF00); break;
        case COLOR_00FF00: idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case COLOR_00FF3F: idx = COLOR_PRG_IDX(UX_COLOR_00FF3F); break;
        case COLOR_00FF7F: idx = COLOR_PRG_IDX(UX_COLOR_00FF7F); break;
        case COLOR_00FFBF: idx = COLOR_PRG_IDX(UX_COLOR_00FFBF); break;
        case COLOR_606060: idx = COLOR_PRG_IDX(UX_COLOR_606060); break;
        case COLOR_00FFFF: idx = COLOR_PRG_IDX(UX_COLOR_00FFFF); break;
        case COLOR_00BFFF: idx = COLOR_PRG_IDX(UX_COLOR_00BFFF); break;
        case COLOR_007FFF: idx = COLOR_PRG_IDX(UX_COLOR_007FFF); break;
        case COLOR_003FFF: idx = COLOR_PRG_IDX(UX_COLOR_003FFF); break;
        case COLOR_0000FF: idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case COLOR_3F00FF: idx = COLOR_PRG_IDX(UX_COLOR_3F00FF); break;
        case COLOR_A0A0A0: idx = COLOR_PRG_IDX(UX_COLOR_A0A0A0); break;
        case COLOR_7F00FF: idx = COLOR_PRG_IDX(UX_COLOR_7F00FF); break;
        case COLOR_BF00FF: idx = COLOR_PRG_IDX(UX_COLOR_BF00FF); break;
        case COLOR_FF00FF: idx = COLOR_PRG_IDX(UX_COLOR_FF00FF); break;
        case COLOR_FF00BF: idx = COLOR_PRG_IDX(UX_COLOR_FF00BF); break;
        case COLOR_FF007F: idx = COLOR_PRG_IDX(UX_COLOR_FF007F); break;
        case COLOR_FF003F: idx = COLOR_PRG_IDX(UX_COLOR_FF003F); break;
        case COLOR_FFFFFF: idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF); break;
        case COLOR_000000: idx = COLOR_PRG_IDX(UX_COLOR_202020); break;
        default: break;
    }

    return idx;
}

static unsigned int _convert_to_coloridx_rgb_color(int idx)
{
    unsigned int rgb_col = COLOR_FF003F;

    if (idx == COLOR_PRG_IDX(UX_COLOR_FF0000)) rgb_col = COLOR_FF0000;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF3F00)) rgb_col = COLOR_FF3F00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF7F00)) rgb_col = COLOR_FF7F00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FFBF00)) rgb_col = COLOR_FFBF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FFFF00)) rgb_col = COLOR_FFFF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_BFFF00)) rgb_col = COLOR_BFFF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_202020)) rgb_col = COLOR_000000;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_7FFF00)) rgb_col = COLOR_7FFF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_3FFF00)) rgb_col = COLOR_3FFF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FF00)) rgb_col = COLOR_00FF00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FF3F)) rgb_col = COLOR_00FF3F;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FF7F)) rgb_col = COLOR_00FF7F;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FFBF)) rgb_col = COLOR_00FFBF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_606060)) rgb_col = COLOR_606060;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FFFF)) rgb_col = COLOR_00FFFF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00BFFF)) rgb_col = COLOR_00BFFF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_007FFF)) rgb_col = COLOR_007FFF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_003FFF)) rgb_col = COLOR_003FFF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_0000FF)) rgb_col = COLOR_0000FF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_3F00FF)) rgb_col = COLOR_3F00FF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_A0A0A0)) rgb_col = COLOR_A0A0A0;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_7F00FF)) rgb_col = COLOR_7F00FF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_BF00FF)) rgb_col = COLOR_BF00FF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF00FF)) rgb_col = COLOR_FF00FF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF00BF)) rgb_col = COLOR_FF00BF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF007F)) rgb_col = COLOR_FF007F;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF003F)) rgb_col = COLOR_FF003F;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FFFFFF)) rgb_col = COLOR_FFFFFF;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_000000)) rgb_col = COLOR_202020;

    return rgb_col;
} 

static IGPOINT _rotate_point(IGPOINT src, IGPOINT datum_pt, gdouble cost, gdouble sint)
{
    IGPOINT dst;

    dst.x = (int)ceil(((src.x - datum_pt.x) * cost) +  ((src.y - datum_pt.y) * sint) + datum_pt.x);
    dst.y = (int)ceil(((datum_pt.x - src.x) * sint) +  ((src.y - datum_pt.y) * cost) + datum_pt.y);
    return dst;
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
    GdkPoint arrow_pt[7] = {{-8,-30}, {-8, 12}, {-18, 12}, {0, 32}, {20, 12}, {10, 12}, {10, -30}};
    gint mptx = (pt1.x + pt2.x) >> 1;
    gint mpty = (pt1.y + pt2.y) >> 1;
    gint dx = pt2.x - pt1.x, dy = pt2.y - pt1.y;
    gint x, y, i;
    float l = sqrtf((float)(dx * dx + dy * dy));

    for (i = 0; i < 7; i++) 
    {
        x = arrow_pt[i].x;
        y = arrow_pt[i].y;

        dst[i].x = (int)((float)((x * dx - y * dy) * 4 / l) + mptx);             
        dst[i].y = (int)((float)((y * dx + x * dy) * 4 / l) + mpty);
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

static int _get_zone_dir_figure(VAZONEPTR pzone, IGPOINT *figure, int *cnt)
{
    IGPOINT zone_pt[16];
    int zone_cnt = 0;

    memset(zone_pt, 0x00, sizeof(zone_pt));

    _get_point_from_zone(zone_pt, &zone_cnt, &pzone->zn_data);
    _get_dir_position(zone_pt[0], zone_pt[1], figure);
    *cnt = 7;
    return 0;
}

static int _get_zone_figure(VAZONEPTR pzone, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    *cnt = pzone->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pzone->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = _convert_rgb_color_to_coloridx(pzone->zn_data.color);   
    
    return 0;
}

static int _get_zone_name(VAZONEPTR pzone, char *str)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    g_stpcpy(str, pzone->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _get_point_from_cntr(IGPOINT *pt, int *cnt, VCACntr *pc)
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

static int _get_cntr_figure(VACNTRPTR pcntr, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    *cnt = pcntr->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pcntr->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = _convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    return 0;
}

static int _get_cntr_name(VACNTRPTR pcntr, char *str)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    g_stpcpy(str, pcntr->dic[INFO]->ctns.t.txt);
    return 0;   
}

static int _get_point_from_calb(IGPOINT *pt, int *cnt, VCACalb *pc)
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

static int _get_calb_figure(VACALBPTR pcalb, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pcalb)) return -1;

    *cnt = pcalb->dic[FIGURE]->ctns.f.cnt;
    memcpy(pt, pcalb->dic[FIGURE]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = pcalb->dic[FIGURE]->ctns.f.cl.i;      
    return 0;
}

static int _get_calb_icon_upper(VACALBPTR pcalb, IGPOINT *pt, int *width, int *height, int *color_idx)
{    
    if (_IS_NOFIGURE(pcalb)) return -1;

    *pt = pcalb->dic[CALB_ICUPP]->ctns.a.pt;
    *width = pcalb->dic[CALB_ICUPP]->ctns.a.w;
    *height = pcalb->dic[CALB_ICUPP]->ctns.a.h;    
    *color_idx = pcalb->dic[CALB_ICUPP]->ctns.a.cl.i; 
    return 0;
}

static int _get_calb_icon_lower(VACALBPTR pcalb, IGPOINT *pt, int *cnt, int *color_idx)
{
    *cnt = 0;
    
    if (_IS_NOFIGURE(pcalb)) return -1;

    *cnt = pcalb->dic[CALB_ICLOW]->ctns.f.cnt;
    memcpy(pt, pcalb->dic[CALB_ICLOW]->ctns.f.pt, sizeof(IGPOINT) * *cnt);
    *color_idx = pcalb->dic[CALB_ICLOW]->ctns.f.cl.i;   
    
    return 0;
}

static int _get_calb_value(VACALBPTR pcalb, char *str)
{
    if (_IS_NOFIGURE(pcalb)) return -1;

    g_stpcpy(str, pcalb->dic[INFO]->ctns.t.txt);
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

static int _set_zone_type(VAZONEPTR pzone, int type)
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

static int _set_zone_direction_to_dic(VAZONEPTR pzone, int color_idx)
{
    IGPOINT zone_pt[16];
    IGPOINT dir_pt[16];
    int zone_cnt = 0;

    if (!pzone->dic[FIGURE]) return -1;

    memset(zone_pt, 0x00, sizeof(zone_pt));

    _get_point_from_zone(zone_pt, &zone_cnt, &pzone->zn_data);
    _get_dir_position(zone_pt[0], zone_pt[1], dir_pt);

    pzone->dic[DIRCTRL]->ctns.f.cnt = 7;
    memcpy(pzone->dic[DIRCTRL]->ctns.f.pt, dir_pt, sizeof(IGPOINT) * 7);   
    
    if (pzone->event_time > 0) pzone->dic[DIRCTRL]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[DIRCTRL]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_zone_direction(VAZONEPTR pzone, int color_idx)
{
    _set_zone_direction_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_name_to_dic(VAZONEPTR pzone, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT];
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

    if (pzone->event_time > 0) pzone->dic[INFO]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[INFO]->ctns.t.cl.i = color_idx;
    
    pzone->dic[INFO]->ctns.t.size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _set_zone_name_to_zndata(VAZONEPTR pzone, char *str)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    strcpy(pzone->zn_data.name, str);
    return 0;
}

static int _set_zone_name(VAZONEPTR pzone, char *str, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_name_to_dic(pzone, str, color_idx);
    _set_zone_name_to_zndata(pzone, str);
    return 0;
}

static int _set_zone_value_to_dic(VAZONEPTR pzone, int color_idx)
{
    IGPOINT pt[MAX_PT];
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
    
    if (pzone->event_time > 0) pzone->dic[COUNT_VALUE]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else pzone->dic[COUNT_VALUE]->ctns.t.cl.i = color_idx;
    
    pzone->dic[COUNT_VALUE]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_zone_value(VAZONEPTR pzone, int color_idx)
{
    guint y;

    if (!pzone->dic[FIGURE]) return -1;

    _set_zone_value_to_dic(pzone, color_idx);
    return 0;
}

static int _set_zone_figure_to_dic(VAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pzone->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    
    if (pzone->event_time > 0) pzone->dic[FIGURE]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);    
    else pzone->dic[FIGURE]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_zone_figure_to_zndata(VAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pzone)) return -1;

    for (i = 0; i < cnt; ++i) {
        pzone->zn_data.pt[i].x = pt[i].x;
        pzone->zn_data.pt[i].y = pt[i].y;
    }

    pzone->zn_data.npts = cnt;    
    pzone->zn_data.color = _convert_to_coloridx_rgb_color(color_idx);    
    return 0;
}

static int _set_zone_figure(VAZONEPTR pzone, IGPOINT *pt, int cnt, int color_idx)
{
    _set_zone_figure_to_dic(pzone, pt, cnt, color_idx);
    _set_zone_figure_to_zndata(pzone, pt, cnt, color_idx);
    return 0;
}

static int _set_cntr_type(VACNTRPTR pcntr)
{
    DMSG(9, "");

    pcntr->type |= MASK_AREA;
    pcntr->type |= MASK_COUNT;
    pcntr->occupied = 1;
    pcntr->blink_step = -1;
    pcntr->event_time = -1;        
    return 0;
}

static int _set_cntr_name_to_dic(VACNTRPTR pcntr, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    int cnt;

    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_name_position(pt, cnt, &dst);

    memset(pcntr->dic[INFO]->ctns.t.txt, 0x00, sizeof(pcntr->dic[INFO]->ctns.t.txt));
    g_stpcpy(pcntr->dic[INFO]->ctns.t.txt, str);

    pcntr->dic[INFO]->ctns.t.pt.x = dst.x;
    pcntr->dic[INFO]->ctns.t.pt.y = dst.y;

    pcntr->dic[INFO]->ctns.t.pt.dx = 0;
    pcntr->dic[INFO]->ctns.t.pt.dy = -5;

    pcntr->dic[INFO]->ctns.t.layout_psx = 1;    
    pcntr->dic[INFO]->ctns.t.layout_psy = -1;

    pcntr->dic[INFO]->ctns.t.cl.i = color_idx;
    pcntr->dic[INFO]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_cntr_name_to_ctdata(VACNTRPTR pcntr, char *str)
{
    int i;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    strcpy(pcntr->ct_data.name, str);
    return 0;
}

static int _set_cntr_name(VACNTRPTR pcntr, char *str, int color_idx)
{
    guint y;

    if (!pcntr->dic[FIGURE]) return -1;

    _set_cntr_name_to_dic(pcntr, str, color_idx);
    _set_cntr_name_to_ctdata(pcntr, str);
    return 0;
}

static int _set_cntr_value_to_dic(VACNTRPTR pcntr, int color_idx)
{
    IGPOINT pt[MAX_PT];
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
    pcntr->dic[COUNT_VALUE]->ctns.t.pt.y = dst.y + 72;

    pcntr->dic[COUNT_VALUE]->ctns.t.pt.dx = 5;
    pcntr->dic[COUNT_VALUE]->ctns.t.pt.dy = 0;

    pcntr->dic[COUNT_VALUE]->ctns.t.layout_psx = 1;   
    pcntr->dic[COUNT_VALUE]->ctns.t.layout_psy = 0;   

    pcntr->dic[COUNT_VALUE]->ctns.t.cl.i = color_idx;
    pcntr->dic[COUNT_VALUE]->ctns.t.size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _set_cntr_value(VACNTRPTR pcntr, int color_idx)
{
    guint y;

    if (!pcntr->dic[FIGURE]) return -1;

    _set_cntr_value_to_dic(pcntr, color_idx);
    return 0;
}

static int _set_cntr_figure_to_dic(VACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pcntr->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    pcntr->dic[FIGURE]->ctns.f.cl.i = color_idx;
    return 0;
}

static int _set_cntr_figure_to_ctdata(VACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pcntr)) return -1;

    for (i = 0; i < cnt; ++i) {
        pcntr->ct_data.pt[i].x = pt[i].x;
        pcntr->ct_data.pt[i].y = pt[i].y;
    }

    pcntr->ct_data.color = _convert_to_coloridx_rgb_color(color_idx);
    return 0;
}

static int _set_cntr_figure(VACNTRPTR pcntr, IGPOINT *pt, int cnt, int color_idx)
{
    _set_cntr_figure_to_dic(pcntr, pt, cnt, color_idx);
    _set_cntr_figure_to_ctdata(pcntr, pt, cnt, color_idx);
    return 0;
}

static int _set_calb_type(VACALBPTR pcalb)
{
    DMSG(9, "");

    pcalb->type = MASK_LINE;
    pcalb->type |= MASK_CALB;
    pcalb->occupied = 1;
    pcalb->blink_step = -1;
    pcalb->event_time = -1;
    return 0;
}

static int _set_calb_figure_to_dic(VACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->dic[FIGURE]->ctns.f.cnt = cnt;
    memcpy(pcalb->dic[FIGURE]->ctns.f.pt, pt, sizeof(IGPOINT) * cnt);
    pcalb->dic[FIGURE]->ctns.f.cl.i = color_idx;    
    return 0;
}

static int _set_calb_icon_upper_to_dic(VACALBPTR pcalb, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    int cnt;
    int width, height;

    if (_IS_NOFIGURE(pcalb)) return -1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_upper_position(pt[0], pt[1], &dst, &width, &height);

    pcalb->dic[CALB_ICUPP]->ctns.a.pt = dst;
    pcalb->dic[CALB_ICUPP]->ctns.a.w = width;
    pcalb->dic[CALB_ICUPP]->ctns.a.h = height;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = color_idx;    
    return 0;
}

static int _set_calb_icon_lower_to_dic(VACALBPTR pcalb, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst[MAX_PT];
    int cnt;
    int width, height;

    if (_IS_NOFIGURE(pcalb)) return -1;

    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_lower_position(pt[0], pt[1], dst, &cnt);

    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->dic[CALB_ICLOW]->ctns.f.cnt = cnt;
    memcpy(pcalb->dic[CALB_ICLOW]->ctns.f.pt, dst, sizeof(IGPOINT) * cnt);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = color_idx;    
    return 0;
}

static int _set_calb_figure_to_cbdata(VACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    int i;
    
    if (_IS_NOFIGURE(pcalb)) return -1;

    for (i = 0; i < cnt; ++i) {
        pcalb->cb_data.pt[i].x = pt[i].x;
        pcalb->cb_data.pt[i].y = pt[i].y;
    }

    return 0;
}

static int _set_calb_value_to_dic(VACALBPTR pcalb, char *str, int color_idx)
{
    IGPOINT pt[MAX_PT];
    IGPOINT dst;
    int cnt;

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

static int _set_calb_value(VACALBPTR pcalb, char *str, int color_idx)
{
    _set_calb_value_to_dic(pcalb, str, color_idx);
    return 0;
}

static int _set_calb_figure(VACALBPTR pcalb, IGPOINT *pt, int cnt, int color_idx)
{
    _set_calb_figure_to_dic(pcalb, pt, cnt, color_idx);
    _set_calb_figure_to_cbdata(pcalb, pt, cnt, color_idx);
    return 0;
}

static int _set_calb_icon_upper(VACALBPTR pcalb, int color_idx)
{
    _set_calb_icon_upper_to_dic(pcalb, color_idx);
    return 0;
}

static int _set_calb_icon_lower(VACALBPTR pcalb, int color_idx)
{
    _set_calb_icon_lower_to_dic(pcalb, color_idx);
    return 0;
}

static int _make_zone_direction_highlight(VAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[DIRCTRL]->ctns.f.wi = 5;
    pzone->dic[DIRCTRL]->ctns.f.fi = 1;    
    return 0;
}

static int _make_zone_direction_lowlight(VAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[DIRCTRL]->ctns.f.wi = 3;
    pzone->dic[DIRCTRL]->ctns.f.fi = 0;
    return 0;
}

static int _make_zone_highlight(VAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 1;
    pzone->dic[FIGURE]->ctns.f.wi = 5;
    return 0;
}

static int _make_zone_lowlight(VAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->sty_highlight = 0;
    pzone->dic[FIGURE]->ctns.f.wi = 3;
    return 0;
}

static int _make_zone_focus(VAZONEPTR pzone)
{
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 1;
    return 0;
}

static int _make_zone_unfocus(VAZONEPTR pzone)
{   
    if (_IS_NOFIGURE(pzone)) return -1;

    pzone->dic[FIGURE]->ctns.f.vt = 0;
    return 0;
}

static int _make_cntr_highlight(VACNTRPTR pcntr)
{
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->sty_highlight = 1;
//    pcntr->dic[FIGURE]->ctns.f.vt = 1;
    pcntr->dic[FIGURE]->ctns.f.wi = 5;
    return 0;
}

static int _make_cntr_lowlight(VACNTRPTR pcntr)
{   
    if (_IS_NOFIGURE(pcntr)) return -1;

    pcntr->sty_highlight = 0;
//    pcntr->dic[FIGURE]->ctns.f.vt = 0;
    pcntr->dic[FIGURE]->ctns.f.wi = 3;
    return 0;
}

static int _make_calb_highlight(VACALBPTR pcalb)
{
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->sty_highlight = 1;
    pcalb->dic[FIGURE]->ctns.f.vt = 1;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);
    pcalb->dic[INFO]->ctns.t.cl.i = COLOR_PRG_IDX(UX_COLOR_FF0080);    
    return 0;
}

static int _make_calb_lowlight(VACALBPTR pcalb)
{   
    if (_IS_NOFIGURE(pcalb)) return -1;

    pcalb->sty_highlight = 0;
    pcalb->dic[FIGURE]->ctns.f.vt = 0;
    pcalb->dic[CALB_ICUPP]->ctns.a.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    pcalb->dic[CALB_ICLOW]->ctns.f.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);
    pcalb->dic[INFO]->ctns.t.cl.i = COLOR_PRG_IDX(UX_COLOR_80FFFF);    
    return 0;
}

static int _apply_rule_prop(ITXVAA_T *pvaa, ITX_VARULE_PROP *prop)
{
//    pvaa->calbres.paramvalid = res->paramvalid;
    pvaa->prop.unit = prop->unit;
    pvaa->prop.en_shadowrm = prop->en_shadowrm;
    pvaa->prop.track_ref = prop->track_ref;
    pvaa->prop.en_usecalib = prop->en_usecalib;
    pvaa->prop.min_width3d = prop->min_width3d;
    pvaa->prop.min_height3d = prop->min_height3d;
    pvaa->prop.sw_obj_bb = prop->sw_obj_bb;
    pvaa->prop.sw_obj_tr = prop->sw_obj_tr;
    pvaa->prop.sw_obj_id = prop->sw_obj_id;
    pvaa->prop.sw_obj_w3d = prop->sw_obj_w3d;
    pvaa->prop.sw_obj_h3d = prop->sw_obj_h3d;
    pvaa->prop.sw_obj_s3d = prop->sw_obj_s3d;
    pvaa->prop.sw_rule = prop->sw_rule;    

    return 0;
}

static int _bring_rule_prop(ITXVAA_T *pvaa, ITX_VARULE_PROP *prop)
{
//    res->paramvalid = pvaa->calbres.paramvalid;
    prop->unit = pvaa->prop.unit;
    prop->en_shadowrm = pvaa->prop.en_shadowrm;
    prop->track_ref = pvaa->prop.track_ref;
    prop->en_usecalib = pvaa->prop.en_usecalib;
    prop->min_width3d = pvaa->prop.min_width3d;
    prop->min_height3d = pvaa->prop.min_height3d;
    prop->sw_obj_bb = pvaa->prop.sw_obj_bb;
    prop->sw_obj_tr = pvaa->prop.sw_obj_tr;
    prop->sw_obj_id = pvaa->prop.sw_obj_id;
    prop->sw_obj_w3d = pvaa->prop.sw_obj_w3d;
    prop->sw_obj_h3d = pvaa->prop.sw_obj_h3d;
    prop->sw_obj_s3d = pvaa->prop.sw_obj_s3d;
    prop->sw_rule = pvaa->prop.sw_rule;    

    return 0;
}

static int _update_zone_count(ITXVAA_T *pvaa)
{
    int id;
    VAZONEPTR pzone;

    pvaa->cnt_zone = 0;

    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _get_zone_ptr(pvaa, id);
        if (pzone->use) pvaa->cnt_zone++;
    }

    return 0;
}

static int _apply_zone_conf(ITX_VAZONE_CONF *conf, VAZONEPTR pzone)
{
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

    pzone->zn_data.enabled = 0;

    if(conf->forward)   pzone->zn_data.enabled |= IVCA_ET_DIR_POS;
    if(conf->reverse)   pzone->zn_data.enabled |= IVCA_ET_DIR_NEG;        
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

    pzone->zn_data.ecolor = _convert_to_coloridx_rgb_color(conf->filter_color_idx);
    pzone->zn_data.ecolor_sens = conf->filter_color_prct;
    pzone->zn_data.size_min[0] = conf->filter_width_from;
    pzone->zn_data.size_max[0] = conf->filter_width_to;
    pzone->zn_data.size_min[1] = conf->filter_height_from;
    pzone->zn_data.size_max[1] = conf->filter_height_to;
    pzone->zn_data.speed_min = conf->filter_speed_from;
    pzone->zn_data.speed_max = conf->filter_speed_to;
    
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

static int _bring_zone_conf(VAZONEPTR pzone, ITX_VAZONE_CONF *conf)
{
    conf->zoneid = pzone->id;
    conf->type = pzone->zn_data.type;
    conf->use_zone = pzone->use;   
    conf->focus = pzone->focus;
    
    conf->active = pzone->zn_data.active;  

    if (pzone->zn_data.enabled & IVCA_ET_DIR_POS) conf->forward = 1;
    else conf->forward = 0;
    
    if(pzone->zn_data.enabled & IVCA_ET_DIR_NEG) conf->reverse = 1;
    else    conf->reverse = 0;
        
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

    conf->filter_color_idx = _convert_rgb_color_to_coloridx(pzone->zn_data.ecolor);
    conf->filter_color_prct = pzone->zn_data.ecolor_sens;
    conf->filter_width_from = pzone->zn_data.size_min[0];
    conf->filter_width_to = pzone->zn_data.size_max[0];
    conf->filter_height_from = pzone->zn_data.size_min[1];
    conf->filter_height_to = pzone->zn_data.size_max[1];
    conf->filter_speed_from = pzone->zn_data.speed_min;
    conf->filter_speed_to = pzone->zn_data.speed_max;
    

    return 0;
}

static int _bring_zone_confs_all(ITXVAA_T *pvaa, ITX_VAZONE_CONF conf[32], int *cnt)
{
    int i;
    VAZONEPTR pzone;

    for (i = 0; i < MAX_ZONE; ++i) 
    {
        pzone = _get_zone_ptr(pvaa, (ITX_ZONEID)i);
        _bring_zone_conf(pzone, &conf[i]);
    }

    if (cnt) *cnt = pvaa->cnt_zone;

    return 0;
}

static int _update_cntr_count(ITXVAA_T *pvaa)
{
    int id;
    VACNTRPTR pcntr;

    pvaa->cnt_cntr = 0;

    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _get_cntr_ptr(pvaa, id);    
        if (pcntr->use) pvaa->cnt_cntr++;
    }

    return 0;
}

static int _apply_cntr_conf(ITX_VACNTR_CONF *conf, VACNTRPTR pcntr)
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

static int _bring_cntr_conf(VACNTRPTR pcntr, ITX_VACNTR_CONF *conf)
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

static int _bring_cntr_confs_all(ITXVAA_T *pvaa, ITX_VACNTR_CONF conf[32], int *cnt)
{
    int i;
    VACNTRPTR pcntr;

    for (i = 0; i < MAX_CNTR; ++i) 
    {
        pcntr = _get_cntr_ptr(pvaa, (ITX_CNTRID)i);
        _bring_cntr_conf(pcntr, &conf[i]);
    }

    if (cnt) *cnt = pvaa->cnt_cntr;

    return 0;
}

static int _update_calb_count(ITXVAA_T *pvaa)
{
    int id;
    VACALBPTR pcalb;

    pvaa->cnt_calb = 0;

    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _get_calb_ptr(pvaa, id);
        if (pcalb->use) pvaa->cnt_calb++;
    }

    return 0;
}

static int _apply_calb_conf(ITX_VACALB_CONF *conf, VACALBPTR pcalb)
{
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

static int _bring_calb_conf(VACALBPTR pcalb, ITX_VACALB_CONF *conf)
{
    conf->calbid = pcalb->id;    
    conf->use_calb = pcalb->use;
    conf->focus = pcalb->focus;    

    conf->height = pcalb->cb_data.height;
    return 0;
}

static int _bring_calb_confs_all(ITXVAA_T *pvaa, ITX_VACALB_CONF conf[32], int *cnt)
{
    int i;
    VACALBPTR pcalb;

    for (i = 0; i < MAX_CALB; ++i) 
    {
        pcalb = _get_calb_ptr(pvaa, (ITX_CALBID)i);
        _bring_calb_conf(pcalb, &conf[i]);
    }

    if (cnt) *cnt = pvaa->cnt_calb;

    return 0;
}

static int _apply_calb_res(ITXVAA_T *pvaa, ITX_VACALB_RESULT *res)
{
    pvaa->calbres.paramvalid = res->paramvalid;
    pvaa->calbres.focal = res->focal;
    pvaa->calbres.height = res->cam_height;
    pvaa->calbres.tilt = res->cam_tilt;
    pvaa->calbres.p_width = res->p_width;
    pvaa->calbres.p_height = res->p_height;

    return 0;
}

static int _bring_calb_res(ITXVAA_T *pvaa, ITX_VACALB_RESULT *res)
{
    res->paramvalid = pvaa->calbres.paramvalid;
    res->focal = pvaa->calbres.focal;
    res->cam_height = pvaa->calbres.height;
    res->cam_tilt = pvaa->calbres.tilt;
    res->p_width = pvaa->calbres.p_width;
    res->p_height = pvaa->calbres.p_height;

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
        info->pt[i].opt = pt[i].opt;
    }

    return 0;
}

static int _construct_zone_direction_figure(IGPOINT *pt, FIGURE_INFO *info)
{
    _get_dir_position(pt[0], pt[1], info->pt);
    info->cnt = 7;    
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

static int _makeup_zone_figure(VAZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        info->vt = 1;
        info->wi = 5;
    }
    else {
        info->vt = 0;
        info->wi = 3;
    }

    if (pzone->event_time > 0) info->cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);

    return 0;
}

static int _makeup_zone_direction_figure(VAZONEPTR pzone, FIGURE_INFO *info)
{
    if (pzone->sty_highlight) {
        info->wi = 5;
        info->fi = 1;
    }
    else {
        info->wi = 3;
        info->fi = 0;
    }

    if (pzone->event_time > 0) info->cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    return 0;
}

static int _makeup_zone_name(VAZONEPTR pzone, TEXT_INFO *info)
{
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;

    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pzone->zn_data.name);

    if (pzone->event_time > 0) info->cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    info->size = NFFONT_MEDIUM_NORMAL;    
    return 0;
}

static int _makeup_zone_value(VAZONEPTR pzone, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.dx = 0;
    info->pt.dy = 5;

    info->layout_psx = 1;
    info->layout_psy = 1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    if (pzone->event_time > 0) info->cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
    else info->cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
    
    info->size = NFFONT_SMALL_NORMAL;
    return 0;
}

static int _makeup_cntr_figure(VACNTRPTR pcntr, FIGURE_INFO *info)
{   
    if (pcntr->sty_highlight) {
//        info->vt = 1;
        info->wi = 5;
    }
    else {
//        info->vt = 0;
        info->wi = 3;
    }

    info->cl.i = _convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    return 0;
}

static int _makeup_cntr_name(VACNTRPTR pcntr, TEXT_INFO *info)
{   
    info->pt.dx = 0;
    info->pt.dy = -5;

    info->layout_psx = 1;
    info->layout_psy = -1;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_stpcpy(info->txt, pcntr->ct_data.name);

    info->cl.i = _convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    info->size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _makeup_cntr_value(VACNTRPTR pcntr, TEXT_INFO *info, int value)
{
    guint w;

    info->pt.y += 72;
    
    info->pt.dx = 5;
    info->pt.dy = 0;

    info->layout_psx = 1;
    info->layout_psy = 0;
    
    memset(info->txt, 0x00, sizeof(info->txt));
    g_sprintf(info->txt, "%d", value);

    info->cl.i = _convert_rgb_color_to_coloridx(pcntr->ct_data.color);
    info->size = NFFONT_MEDIUM_NORMAL;
    return 0;
}

static int _makeup_calb_figure(VACALBPTR pcalb, FIGURE_INFO *info)
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

static int _makeup_calb_icon_upper(VACALBPTR pcalb, ARC_INFO *info)
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

static int _makeup_calb_icon_lower(VACALBPTR pcalb, FIGURE_INFO *info)
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

static int _makeup_calb_value(VACALBPTR pcalb, TEXT_INFO *info, int value)
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

static int _makeup_trackbox_figure(FIGURE_INFO *info, int evt)
{
    if (evt) info->cl.i = CR_EVENT_TBOX;    
    else info->cl.i = CR_NORMAL_TBOX;
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

static int _add_zone_figure_to_display(ITXVAA_T *pvaa, int page, VAZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
    
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

static int _add_zone_name_to_display(ITXVAA_T *pvaa, int page, VAZONEPTR pzone)
{
    TEXT_INFO tinfo;
    guint col;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _get_name_position(pt, cnt, &tinfo.pt);
    _makeup_zone_name(pzone, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pzone->dic[INFO] = pdic;
    pzone->dic[INFO]->en = pzone->use & pzone->zn_data.active;
    
    return 0;
}

static int _add_zone_direction_to_display(ITXVAA_T *pvaa, int page, VAZONEPTR pzone)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16];
    int cnt = 0;    
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
    
    memset(&finfo, 0x00, sizeof(FIGURE_INFO));
    _get_point_from_zone(pt, &cnt, &pzone->zn_data);
    _construct_zone_direction_figure(pt, &finfo);
    _makeup_zone_direction_figure(pzone, &finfo);

    pdic = dit_add_dic(ditid, page, DIC_FIG, &finfo);
    pzone->dic[DIRCTRL] = pdic;     

    if (_IS_LINE(pzone)) pzone->dic[DIRCTRL]->en = pzone->use & pzone->zn_data.active;
    else pzone->dic[DIRCTRL]->en = 0;
    
    return 0;
}

static int _add_zone_value_to_display(ITXVAA_T *pvaa, int page, VAZONEPTR pzone, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;

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

static int _add_zone_to_display(ITXVAA_T *pvaa, int page, VAZONEPTR pzone)
{
    if (!vaa_itx_is_dit_linked(pvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_zone_figure_to_display(pvaa, page, pzone);
    _add_zone_name_to_display(pvaa, page, pzone);
    _add_zone_direction_to_display(pvaa, page, pzone);
    _add_zone_value_to_display(pvaa, page, pzone, pzone->cnt_value);
    
    return 0;
}

static int _add_cntr_figure_to_display(ITXVAA_T *pvaa, int page, VACNTRPTR pcntr)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
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

static int _add_cntr_name_to_display(ITXVAA_T *pvaa, int page, VACNTRPTR pcntr)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));
    _get_point_from_cntr(pt, &cnt, &pcntr->ct_data);
    _get_name_position(pt, cnt, &tinfo.pt);    
    _makeup_cntr_name(pcntr, &tinfo);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pcntr->dic[INFO] = pdic;
    pcntr->dic[INFO]->en = pcntr->use & pcntr->ct_data.active;

    return 0;
}

static int _add_cntr_value_to_display(ITXVAA_T *pvaa, int page, VACNTRPTR pcntr, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;

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

static int _add_cntr_to_display(ITXVAA_T *pvaa, int page, VACNTRPTR pcntr)
{
    if (!vaa_itx_is_dit_linked(pvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_cntr_figure_to_display(pvaa, page, pcntr);
    _add_cntr_name_to_display(pvaa, page, pcntr);
    _add_cntr_value_to_display(pvaa, page, pcntr, pcntr->cnt_value);
    return 0;
}

static int _add_calb_figure_to_display(ITXVAA_T *pvaa, int page, VACALBPTR pcalb)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
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

static int _add_calb_icon_upper_to_display(ITXVAA_T *pvaa, int page, VACALBPTR pcalb)
{
    ARC_INFO ainfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
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

static int _add_calb_icon_lower_to_display(ITXVAA_T *pvaa, int page, VACALBPTR pcalb)
{
    FIGURE_INFO finfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;
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

static int _add_calb_value_to_display(ITXVAA_T *pvaa, int page, VACALBPTR pcalb, int value)
{
    TEXT_INFO tinfo;
    IGPOINT pt[16];
    int cnt = 0;
    DITID ditid;
    DICPTR pdic;

    ditid = pvaa->ditid;

    memset(&tinfo, 0x00, sizeof(TEXT_INFO));    
    _get_point_from_calb(pt, &cnt, &pcalb->cb_data);
    _get_calb_value_position(pt, cnt, &tinfo.pt);
    _makeup_calb_value(pcalb, &tinfo, value);

    pdic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    pcalb->dic[INFO] = pdic;
    pcalb->dic[INFO]->en = pcalb->use;

    return 0;
}

static int _add_calb_to_display(ITXVAA_T *pvaa, int page, VACALBPTR pcalb)
{
    if (!vaa_itx_is_dit_linked(pvaa)) return -1;

    DMSG(9, "%d\n", page);

    _add_calb_icon_upper_to_display(pvaa, page, pcalb);
    _add_calb_icon_lower_to_display(pvaa, page, pcalb);
    _add_calb_figure_to_display(pvaa, page, pcalb);
    _add_calb_value_to_display(pvaa, page, pcalb, pcalb->cb_data.height);
    return 0;
}

static DICPTR _add_trackbox_to_display(ITXVAA_T *pvaa, int page, IGRECT *rect, int evt)
{
    FIGURE_INFO info;
    DITID ditid;
    DICPTR dic;

    if (!vaa_itx_is_dit_linked(pvaa)) return -1;
    ditid = pvaa->ditid;

    memset(&info, 0x00, sizeof(FIGURE_INFO));
    _construct_trackbox_figure(rect, &info);
    _makeup_trackbox_figure(&info, evt);

    dic = dit_add_dic(ditid, page, DIC_FIG, &info);
    return dic;
}

static DICPTR _add_trackbox_obj_info_to_display(ITXVAA_T *pvaa, int page, IGRECT *rect, char *buf, int evt)
{
    TEXT_INFO tinfo;
    DITID ditid;
    DICPTR dic;

    if (!vaa_itx_is_dit_linked(pvaa)) return -1;
    ditid = pvaa->ditid;

    _makeup_text_trackbox(&tinfo, buf, rect, evt);

    dic = dit_add_dic(ditid, page, DIC_TXT, &tinfo);
    return dic;
}

static DICPTR _add_trajectory_to_display(ITXVAA_T *pvaa, int page, int npt, IXPOINT *pt, int evt)
{
    FIGURE_INFO info;
    DITID ditid;
    DICPTR dic;

    if (!vaa_itx_is_dit_linked(pvaa)) return -1;
    ditid = pvaa->ditid;

    memset(&info, 0x00, sizeof(FIGURE_INFO));
    _construct_trajectory_figure(npt, pt, &info);
    _makeup_trajectory_figure(&info, evt);

    dic = dit_add_dic(ditid, page, DIC_FIG, &info);
    return dic;
}

static int _add_zones_to_display(ITXVAA_T *pvaa)
{
    int i, page;
    VAZONEPTR pzone;    
    DITID ditid = pvaa->ditid;

    for (i = 0; i < MAX_ZONE; ++i) 
    {   
        pzone = _get_zone_ptr(pvaa, (ITX_ZONEID)i);
        page = pvaa->vazone[i].dipage;
        _add_zone_to_display(pvaa, page, pzone);
    }

    return 0;
}

static int _add_cntrs_to_display(ITXVAA_T *pvaa)
{
    int i, page;
    VACNTRPTR pcntr;    
    DITID ditid = pvaa->ditid;

    for (i = 0; i < MAX_CNTR; ++i) 
    {
        pcntr = _get_cntr_ptr(pvaa, (ITX_CNTRID)i);
        page = pvaa->vacntr[i].dipage;
        _add_cntr_to_display(pvaa, page, pcntr);
    }

    return 0;
}

static int _add_calbs_to_display(ITXVAA_T *pvaa)
{
    int i, page;
    VACALBPTR pcalb;    
    DITID ditid = pvaa->ditid;

    for (i = 0; i < MAX_CALB; ++i) 
    {
        pcalb = _get_calb_ptr(pvaa, (ITX_CALBID)i);
        page = pvaa->vacalb[i].dipage;
        _add_calb_to_display(pvaa, page, pcalb);
    }

    return 0;
}

static int _init_zone_data(ITXVAA_T *pvaa)
{
    int id;       
    
    VAZONEPTR pzone;
    VCAZone *zone_data;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _get_zone_ptr(pvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 0;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(VCAZone));
    } 
    
    return 0;
}

static int _init_zone_pb_data(ITXVAA_T *pvaa)
{
    int id;       
    
    VAZONEPTR pzone;
    VCAZone *zone_data;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _get_zone_ptr(pvaa, id);

        _init_zone(pzone);
        
        pzone->use = 0;
        pzone->value_en = 1;
        pzone->id = id;
        _set_zone_type(pzone, -1);
        memset(&pzone->zn_data, 0x00, sizeof(VCAZone));
    } 
    
    return 0;
}

static int _init_cntr_data(ITXVAA_T *pvaa)
{
    int id;       
    
    VACNTRPTR pcntr;
    VCACntr *cntr_data;
    
    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _get_cntr_ptr(pvaa, id);
        
        _init_cntr(pcntr);        

        pcntr->use = 0;
        pcntr->value_en = 1;
        pcntr->id = id;
        _set_cntr_type(pcntr);
        memset(&pcntr->ct_data, 0x00, sizeof(VCACntr));
    }
    
    return 0;
}

static int _init_calb_data(ITXVAA_T *pvaa)
{
    int id;      
    
    VACALBPTR pcalb;
    VCACalb *calb_data;
    
    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _get_calb_ptr(pvaa, id);
        
        _init_calb(pcalb);        

        pcalb->use = 0;
        pcalb->id = id;
        _set_calb_type(pcalb);
        memset(&pcalb->cb_data, 0x00, sizeof(VCACalb));
    }
    
    return 0;
}

static int _init_pb_default_data(ITXVAA_T *pvaa)
{
    pvaa->st_rule = 1;    
    pvaa->prop.active = TRUE;
    pvaa->prop.sw_rule = TRUE;
    pvaa->prop.sw_obj_bb = TRUE;
    pvaa->prop.sw_obj_id = TRUE;
    pvaa->prop.sw_obj_w3d = TRUE;
    pvaa->prop.sw_obj_h3d = TRUE;
    pvaa->prop.sw_obj_s3d = TRUE;
    pvaa->prop.sw_obj_tr = TRUE;

    pvaa->calbres.paramvalid = 0;
    return 0;
}

static int _translate_vcdata_into_rule(ITXVAA_T *pvaa, VCAData *pdb)
{
    int id;
       
    VAZONEPTR pzone;
    VCAZone *zone_data;
    
    VACNTRPTR pcntr;
    VCACntr *cntr_data;
    gint cnt;
    
    for (id = 0; id < pdb->zonelist.cnt; ++id) 
    {   
        pzone = _get_zone_ptr(pvaa, pdb->zonelist.zone[id].id);
        zone_data = &pdb->zonelist.zone[id];

        pzone->use = 1;
        _load_zone_data(zone_data, pzone);
        _set_zone_type(pzone, zone_data->type);
    } 

    cnt = pdb->zonelist.cnt;
    for(id = 0; id < MAX_ZONE; id++)
    {
        pzone = _get_zone_ptr(pvaa, id);

        if(!pzone->use)
        {
            zone_data = &pdb->zonelist.zone[cnt++];
            _load_zone_data(zone_data, pzone);
            _set_zone_type(pzone, zone_data->type);
        }
    }
    pvaa->cnt_zone = pdb->zonelist.cnt;

    for (id = 0; id < pdb->cntrlist.cnt; ++id) 
    {    
        pcntr = _get_cntr_ptr(pvaa, pdb->cntrlist.cntr[id].id);
        cntr_data = &pdb->cntrlist.cntr[id];
        
        pcntr->use = 1;
        _load_cntr_data(cntr_data, pcntr);
        _set_cntr_type(pcntr);
    }

    cnt = pdb->cntrlist.cnt;
    for(id = 0; id < MAX_CNTR; id++)
    {    
        pcntr = _get_cntr_ptr(pvaa, pdb->cntrlist.cntr[id].id);

        if(!pcntr->use)
        {
            pcntr->use = 0;
            cntr_data = &pdb->cntrlist.cntr[++cnt];
            _load_cntr_data(cntr_data, pcntr);
            _set_cntr_type(pcntr);
        }
    }

    pvaa->cnt_cntr = pdb->cntrlist.cnt;

    return 0;
}

static int _translate_vcdata_into_calb(ITXVAA_T *pvaa, VCAData *pdb)
{
    int id;       
    
    VACALBPTR pcalb;
    VCACalb *calb_data;

    for (id = 0; id < pdb->calblist.cnt; ++id) 
    {   
        pcalb = _get_calb_ptr(pvaa, id);

        if (id < pdb->calblist.cnt) pcalb->use = 1;
        else pcalb->use = 0;      

        calb_data = &pdb->calblist.calb[id];    
        _load_calb_data(calb_data, pcalb);
        _set_calb_type(pcalb);
    }

    pvaa->cnt_calb = pdb->calblist.cnt;

    return 0;
}


static int _translate_rule_into_vcdata(ITXVAA_T *pvaa, VCAData *pdb)
{
    int id, cnt;
       
    VAZONEPTR pzone;
    VACNTRPTR pcntr;

    VCAZone *zone_data;
    VCACntr *cntr_data;

    cnt = 0;
    
    for (id = 0; id < MAX_ZONE; ++id) 
    {
        pzone = _get_zone_ptr(pvaa, id);
        if (!pzone->use) continue;
    
        zone_data = &pdb->zonelist.zone[cnt++];
        _unload_zone_data(pzone, zone_data);        
    }

    cnt = 0;

    for (id = 0; id < MAX_CNTR; ++id) 
    {
        pcntr = _get_cntr_ptr(pvaa, id);
        if (!pcntr->use) continue;
    
        cntr_data = &pdb->cntrlist.cntr[cnt++];
        _unload_cntr_data(pcntr, cntr_data);
    }

    pdb->zonelist.cnt = pvaa->cnt_zone;
    pdb->cntrlist.cnt = pvaa->cnt_cntr;
    
    return 0;
}

static int _translate_calb_into_vcdata(ITXVAA_T *pvaa, VCAData *pdb)
{
    int id, cnt;
       
    VACALBPTR pcalb;
    VCACalb *calb_data;

    cnt = 0;    

    for (id = 0; id < MAX_CALB; ++id) 
    {
        pcalb = _get_calb_ptr(pvaa, id);
        if (!pcalb->use) continue;
    
        calb_data = &pdb->calblist.calb[cnt++];
        _unload_calb_data(pcalb, calb_data);
    }

    pdb->calblist.cnt = pvaa->cnt_calb;
    
    return 0;
}

static int _redraw_rules_to_display(ITXVAA_T *pvaa)
{   
    dit_clear_page(pvaa->ditid, pvaa->vazone[0].dipage);
    dit_clear_page(pvaa->ditid, pvaa->vacntr[0].dipage);

    _add_zones_to_display(pvaa);
    _add_cntrs_to_display(pvaa);   
    return 0;
}

static int _redraw_calbs_to_display(ITXVAA_T *pvaa)
{  
    dit_clear_page(pvaa->ditid, pvaa->vacalb[0].dipage);
    _add_calbs_to_display(pvaa);    
    return 0;
}

static int _control_dit(ITXVAA_T *pvaa)
{
    dit_enable(pvaa->ditid);

    _redraw_rules_to_display(pvaa);
    _redraw_calbs_to_display(pvaa);    
    return 0;
}

static int _link_dit(ITXVAA_T *pvaa, DITID ditid)
{
    pvaa->ditid = ditid;    
    dit_disable_all_page(pvaa->ditid);
    return 0;
}

static int _get_zone_id(ITXVAA_T *pvaa, int evt_zoneid)
{
	int i;

	for (i = 0; i < MAX_ZONE; ++i)
		if (pvaa->vazone[i].zn_data.id == evt_zoneid) return i;

	return -1;

}

static int _get_cntr_id(ITXVAA_T *pvaa, int evt_cntrid)
{
	int i;

	for (i = 0; i < MAX_CNTR; ++i)
		if (pvaa->vacntr[i].ct_data.id == evt_cntrid) return i;

	return -1;

}

static int _proc_zone_event_detecting(ITXVAA_T *pvaa, VAZONEPTR pzone)
{
    if (pzone->event_time != -1) 
    {
        if (pzone->event_time == 20)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[DIRCTRL]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[INFO]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
            pzone->dic[COUNT_VALUE]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(CR_EVENT_RULE);
        }
        else if (pzone->event_time == 0)
        {
            pzone->dic[FIGURE]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[DIRCTRL]->ctns.f.cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[INFO]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
            pzone->dic[COUNT_VALUE]->ctns.t.cl.i = _convert_rgb_color_to_coloridx(pzone->zn_data.color);
        }

        --pzone->event_time;
    }
    return 0;
}

static int _proc_event_detecting(ITXVAA_T *pvaa)
{
    int i;
    VAZONEPTR pzone;
    
    for (i = 0; i < MAX_ZONE; ++i) {
        pzone = _get_zone_ptr(pvaa, (ITX_ZONEID)i);
        if (!pzone->occupied) continue;

        _proc_zone_event_detecting(pvaa, pzone);
    }
    return 0;
}

static int _proc_meta_erasing(ITXVAA_T *pvaa)
{
    int i;
    int dipage;

    dipage = pvaa->vameta[0].dipage;
    for (i = 0; i < pvaa->cnt_tbox; ++i) {
        if (pvaa->vatbox[i].dic[TRACKBOX] == 0) continue;

        --pvaa->vatbox[i].age;
        if (pvaa->vatbox[i].age == 0) {
            dit_remove_dic(pvaa->ditid, dipage, pvaa->vatbox[i].dic[TRACKBOX]);
            dit_remove_dic(pvaa->ditid, dipage, pvaa->vatbox[i].dic[TRACKBOX_OBJ_INFO]);
            if(pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA])
                dit_remove_dic(pvaa->ditid, dipage, pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA]);
            pvaa->vatbox[i].dic[TRACKBOX] = 0;
            pvaa->vatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
            pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;
        }
    }
    return 0;
}

static int _clear_meta(ITXVAA_T *pvaa)
{
    int i;
    int dipage;
    for (i = 0; i < MAX_TBOX; ++i) {
        pvaa->vatbox[i].dic[TRACKBOX] = 0;
        pvaa->vatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
        pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;
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




////////////////////////////////////////////////////////////
//
// public interfaces 
//

int vaa_itx_init(int ch, VAAID *pvaaid, DITID *pditid)
{
    ITXVAA_T *pvaa;
    *pvaaid = vaa_itx_create(ch);
    *pditid = dit_create(10, 3840, 2160);

    pvaa = *pvaaid;
    _link_dit(pvaa, *pditid);

    _load_db(pvaa);
    _init_zone_data(pvaa);
    _init_cntr_data(pvaa);    
    _init_calb_data(pvaa);
    _translate_vcdata_into_rule(pvaa, &pvaa->db);
    _translate_vcdata_into_calb(pvaa, &pvaa->db);    
    _copy_db_to_prop(&pvaa->db, &pvaa->prop);
    _copy_db_to_calbres(&pvaa->db, &pvaa->calbres);
    
    _control_dit(pvaa);
    return 0;
}

int vaa_itx_pb_init(int ch, VAAID *pvaaid, DITID *pditid)
{
    ITXVAA_T *pvaa;
    *pvaaid = vaa_itx_create(ch);
    *pditid = dit_create(10, 3840, 2160);

    pvaa = *pvaaid;
    _link_dit(pvaa, *pditid);
    _init_zone_pb_data(pvaa);
    _init_cntr_data(pvaa);    
    _init_calb_data(pvaa);

    _init_pb_default_data(pvaa);
    _control_dit(pvaa);
    return 0;
}

VAAID vaa_itx_create(int ch)
{
    VAAID vaaid;
    ITXVAA_T *pvaa;

    vaaid = (VAAID)imalloc(sizeof(ITXVAA_T));
    pvaa = vaaid;
    _init(pvaa, ch);

    return vaaid;
}

int vaa_itx_destroy(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    ifree(pvaa);
    return 0;
}

int vaa_itx_reload(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _load_db(pvaa);
    _init_zone_data(pvaa);
    _init_cntr_data(pvaa);    
    _init_calb_data(pvaa);    
    _translate_vcdata_into_rule(pvaa, &pvaa->db);
    _translate_vcdata_into_calb(pvaa, &pvaa->db);
    _copy_db_to_prop(&pvaa->db, &pvaa->prop);
    _copy_db_to_calbres(&pvaa->db, &pvaa->calbres);
    _control_dit(pvaa);
    return 0;
}

int vaa_itx_enable(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pvaa->prop.active = TRUE;
    return 0;
}

int vaa_itx_disable(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pvaa->prop.active = FALSE;
    return 0;
}

int vaa_itx_enable_strule(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pvaa->st_rule = 1;
    return 0;
}

int vaa_itx_disable_strule(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pvaa->st_rule = 0;
    return 0;
}

int vaa_itx_is_enabled(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
	if (!pvaa) return -1;

	return (pvaa->prop.active == TRUE);
}

int vaa_itx_raiseup(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _vaa_itx_link_event(id);    
    return 0;
}

int vaa_itx_link_dit(VAAID id, DITID ditid)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _link_dit(pvaa, ditid);

    DMSG(9, "ditid = %p\n", pvaa->ditid);
    _redraw_rules_to_display(pvaa);
    return 0;
}

int vaa_itx_unlink_dit(VAAID id, DITID ditid)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    dit_disable_all_page(pvaa->ditid);
    pvaa->ditid = 0;    
    return 0;
}

int vaa_itx_is_dit_linked(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    return (pvaa->ditid != 0);
}

DITID vaa_itx_get_dit(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    return pvaa->ditid;
}

int vaa_itx_load_db(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _load_db(pvaa);
    _init_zone_data(pvaa);
    _init_cntr_data(pvaa);    
    _init_calb_data(pvaa);    
    _translate_vcdata_into_rule(pvaa, &pvaa->db);
    _translate_vcdata_into_calb(pvaa, &pvaa->db);
    _copy_db_to_prop(&pvaa->db, &pvaa->prop);
    _copy_db_to_calbres(&pvaa->db, &pvaa->calbres);
    _control_dit(pvaa);

    return 0;
}

int vaa_itx_save_db(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    memset(&pvaa->db, 0x00, sizeof(VCAData));
    _copy_prop_to_db(&pvaa->prop, &pvaa->db);
    _copy_calbres_to_db(&pvaa->calbres, &pvaa->db);    
    _translate_rule_into_vcdata(pvaa, &pvaa->db);
    _translate_calb_into_vcdata(pvaa, &pvaa->db);
    _save_db(pvaa);

    return 0;
}

int vaa_itx_export_db(VAAID id, VCAData *vcadata)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    memset(vcadata, 0x00, sizeof(VCAData));
    _copy_prop_to_db(&pvaa->prop, vcadata);
    _translate_rule_into_vcdata(pvaa, vcadata);
    _translate_calb_into_vcdata(pvaa, vcadata);
    return 0;
}

int vaa_itx_add_zone_line_default_template(VAAID id, ITX_ZONEID zone)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_VAZONE_SHAPE));    

    pzone = _get_zone_ptr((ITXVAA_T*)id, zone);
    _set_zone_type(pzone, IVCA_RT_LINE);
    
    conf.use_zone = 1;
    _apply_zone_conf(&conf, pzone);
    _update_zone_count(pvaa);
    
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

int vaa_itx_add_zone_area_default_template(VAAID id, ITX_ZONEID zone)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_VAZONE_CONF));
    memset(&shape, 0x00, sizeof(ITX_VAZONE_SHAPE));    

    pzone = _get_zone_ptr((ITXVAA_T*)id, zone);
    _set_zone_type(pzone, IVCA_RT_AREA);

    conf.use_zone = 1;
    _apply_zone_conf(&conf, pzone);
    _update_zone_count(pvaa);

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

int vaa_itx_get_zone_shape(VAAID id, ITX_ZONEID zone, ITX_VAZONE_SHAPE *shape)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    DMSG(9, "zoneid = %d\n", zone);
    pzone = _get_zone_ptr(pvaa, zone);
    _get_zone_figure(pzone, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_zone_dir_figure(pzone, &shape->dir_pt, &shape->dir_ptcnt);    
    _get_zone_name(pzone, shape->name);       
    return 0;
}

int vaa_itx_set_zone_shape(VAAID id, ITX_ZONEID zone, ITX_VAZONE_SHAPE *shape)
{
    VAZONEPTR pzone;
    int ret = 0;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    if (shape->ptcnt > MAX_PT) return -1;

    pzone = _get_zone_ptr(pvaa, zone);
    if (_IS_LINE(pzone) && shape->ptcnt > 2) return -1;
    if (_IS_AREA(pzone) && shape->ptcnt < 3) return -2;
    if (_IS_AREA(pzone) && shape->ptcnt > MAX_PT_ITX) return -3;

    _set_zone_figure(pzone, shape->pt, shape->ptcnt, shape->color_idx);
    if (_IS_DIRCTRL(pzone)) _set_zone_direction(pzone, shape->color_idx);
    _set_zone_name(pzone, shape->name, shape->color_idx);
    _set_zone_value(pzone, shape->color_idx);
    return 0;
}

int vaa_itx_get_zone_conf(VAAID id, ITX_ZONEID zone, ITX_VAZONE_CONF *conf)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pzone = _get_zone_ptr(pvaa, zone);
    _bring_zone_conf(pzone, conf);
    return 0;
}

int vaa_itx_get_zone_confs_all(VAAID id, ITX_VAZONE_CONF conf[32], int *cnt)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    *cnt = 0;
    if (!pvaa) return -1;

    _bring_zone_confs_all(pvaa, conf, cnt);
    return 0;
}

int vaa_itx_set_zone_conf(VAAID id, ITX_ZONEID zone, ITX_VAZONE_CONF *conf)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pzone = _get_zone_ptr(pvaa, zone);
    _apply_zone_conf(conf, pzone);
    _update_zone_count(pvaa);
    return 0;
}

int vaa_itx_get_zone_value(VAAID id, ITX_ZONEID zone, int *en, int *color_idx, int *count)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pzone = _get_zone_ptr(pvaa, zone);
    *en = pzone->dic[COUNT_VALUE]->en;
    *color_idx = pzone->dic[COUNT_VALUE]->ctns.t.cl.i;    
    *count = pzone->cnt_value;
    return 0;
}

int vaa_itx_set_zone_value(VAAID id, ITX_ZONEID zone, int en, int color_idx, int count)
{
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pzone = _get_zone_ptr(pvaa, zone);
    pzone->dic[COUNT_VALUE]->en = en;
    pzone->cnt_value = count;
    _set_zone_value(pzone, color_idx);
    return 0;
}

ITX_ZONEID vaa_itx_find_zone(VAAID id, int x, int y)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    VAZONEPTR pzone;
    ITX_ZONEID zone;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    ITX_ZONEID tmp_rule[MAX_ZONE];
    int tmp_area[MAX_ZONE]; 
    int rcnt = 0;
    IXSQUARE sq;
    int min = 0;
    int i;
    int fcnt = 0;
    int color_idx;
    IXPOLYGON p;
    
    if (!pvaa) return -1;
    if (!dit_page_is_enabled(pvaa->ditid, pvaa->vazone[0].dipage)) return -1;

    mpt.x = x;
    mpt.y = y;
    
    rcnt = 0;
    
    memset(&p, 0x00, sizeof(IXPOLYGON));
    memset(tmp_rule, 0x00, sizeof(tmp_rule));
    memset(tmp_area, 0x00, sizeof(tmp_area));
    memset(&sq, 0x00, sizeof(IGBOX));
    
    for (zone = MAX_ZONE-1; zone >= 0; zone--) 
	{
        pzone = _get_zone_ptr(pvaa, (ITX_ZONEID)zone);
        if (!pzone) continue;
        if (!pzone->use) continue;

        if (_get_zone_figure(pzone, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt) == 0) 
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

int vaa_itx_add_cntr_default_template(VAAID id, ITX_CNTRID cntr)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    ITX_VACNTR_CONF conf;
    ITX_VACNTR_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_VACNTR_CONF));
    memset(&shape, 0x00, sizeof(ITX_VACNTR_SHAPE));    

    pcntr = _get_cntr_ptr((ITXVAA_T*)id, cntr);
    _set_cntr_type(pcntr);
            
    conf.use_cntr = 1;   
    _apply_cntr_conf(&conf, pcntr);
    _update_cntr_count(pvaa);

    shape.ptcnt = 4;    
    shape.pt[0].x = 1620;
    shape.pt[0].y = 1152;
    shape.pt[1].x = 1620;
    shape.pt[1].y = 1008;    
    shape.pt[2].x = 2220;
    shape.pt[2].y = 1008;    
    shape.pt[3].x = 2220;
    shape.pt[3].y = 1152;
    _set_cntr_figure(pcntr, shape.pt, shape.ptcnt, COLOR_PRG_IDX(UX_COLOR_FF003F));

    _set_cntr_name(pcntr, " ", COLOR_PRG_IDX(UX_COLOR_FF003F));
    _set_cntr_value(pcntr, COLOR_PRG_IDX(UX_COLOR_FF003F));
    return 0;
}

int vaa_itx_get_cntr_shape(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_SHAPE *shape)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    DMSG(9, "cntrid = %d\n", cntr);
    pcntr = _get_cntr_ptr(pvaa, cntr);
    _get_cntr_figure(pcntr, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_cntr_name(pcntr, shape->name);
    return 0;
}

int vaa_itx_set_cntr_shape(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_SHAPE *shape)
{
    VACNTRPTR pcntr;
    int ret = 0;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    if (shape->ptcnt > MAX_PT) return -1;

    pcntr = _get_cntr_ptr(pvaa, cntr);
    if (_IS_LINE(pcntr) && shape->ptcnt > 2) return -1;
    if (_IS_AREA(pcntr) && shape->ptcnt < 3) return -2;
    if (_IS_AREA(pcntr) && shape->ptcnt > MAX_PT_ITX) return -3;

    _set_cntr_figure(pcntr, shape->pt, shape->ptcnt, shape->color_idx);
    _set_cntr_name(pcntr, shape->name, shape->color_idx);    
    _set_cntr_value(pcntr, shape->color_idx);
    return 0;
}

int vaa_itx_get_cntr_conf(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_CONF *conf)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcntr = _get_cntr_ptr(pvaa, cntr);
    _bring_cntr_conf(pcntr, conf);
    return 0;
}

int vaa_itx_get_cntr_confs_all(VAAID id, ITX_VACNTR_CONF conf[16], int *cnt)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    *cnt = 0;
    if (!pvaa) return -1;

    _bring_cntr_confs_all(pvaa, conf, cnt);
    return 0;
}

int vaa_itx_set_cntr_conf(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_CONF *conf)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcntr = _get_cntr_ptr(pvaa, cntr);
    _apply_cntr_conf(conf, pcntr);
    _update_cntr_count(pvaa);
    return 0;
}

int vaa_itx_get_cntr_value(VAAID id, ITX_CNTRID cntr, int *en, int *count)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcntr = _get_cntr_ptr(pvaa, cntr);

    *en = pcntr->dic[COUNT_VALUE]->en;
    *count = pcntr->cnt_value;    
    return 0;
}

int vaa_itx_set_cntr_value(VAAID id, ITX_CNTRID cntr, int en, int count)
{
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcntr = _get_cntr_ptr(pvaa, cntr);

    pcntr->dic[COUNT_VALUE]->en = en;
    pcntr->cnt_value = count;
    return 0;
}

ITX_CNTRID vaa_itx_find_cntr(VAAID id, int x, int y)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    VACNTRPTR pcntr;
    ITX_CNTRID cntr;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    int fcnt = 0;
    int color_idx;
    
    if (!pvaa) return -1;
    if (!dit_page_is_enabled(pvaa->ditid, pvaa->vacntr[0].dipage)) return -1;

    mpt.x = x;
    mpt.y = y;
    
    for (cntr = 0; cntr < MAX_CNTR; cntr++) {
        pcntr = _get_cntr_ptr(pvaa, (ITX_CNTRID)cntr);
        if (!pcntr) continue;
        if (!pcntr->use) continue;

        if (_get_cntr_figure(pcntr, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt) == 0) return cntr;
    }
    return -1;
}

int vaa_itx_get_rule_prop(VAAID id, ITX_VARULE_PROP *prop)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _bring_rule_prop(pvaa, prop);    
    return 0;
}

int vaa_itx_set_rule_prop(VAAID id, ITX_VARULE_PROP *prop)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _apply_rule_prop(pvaa, prop);
    vaa_itx_activate_all_rule(id);    
    return 0;
}

int vaa_itx_add_calb_default_template(VAAID id, ITX_CALBID calb)
{
    VACALBPTR pcalb;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    ITX_VACALB_CONF conf;
    ITX_VACALB_SHAPE shape;

    memset(&conf, 0x00, sizeof(ITX_VACALB_CONF));
    memset(&shape, 0x00, sizeof(ITX_VACALB_SHAPE));    

    pcalb = _get_calb_ptr((ITXVAA_T*)id, calb);
    _set_calb_type(pcalb);
    
    conf.use_calb = 1;
    conf.height = 175;
    memset(shape.value, 0x00, sizeof(shape.value));
    g_sprintf(shape.value, "%d", conf.height);
    
    _apply_calb_conf(&conf, pcalb);
    _update_calb_count(pvaa);

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

int vaa_itx_get_calb_shape(VAAID id, ITX_CALBID calb, ITX_VACALB_SHAPE *shape)
{
    VACALBPTR pcalb;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    DMSG(9, "calbid = %d\n", calb);
    pcalb = _get_calb_ptr(pvaa, calb);

    _get_calb_figure(pcalb, shape->pt, &shape->ptcnt, &shape->color_idx);
    _get_calb_icon_upper(pcalb, &shape->iupp_pt[0], &shape->iupp_w, &shape->iupp_h, &shape->iupp_color_idx);    
    _get_calb_icon_lower(pcalb, shape->ilow_pt, &shape->ilow_ptcnt, &shape->ilow_color_idx);
    _get_calb_value(pcalb, shape->value);
    return 0;
}

int vaa_itx_set_calb_shape(VAAID id, ITX_CALBID calb, ITX_VACALB_SHAPE *shape)
{
    VACALBPTR pcalb;
    int ret = 0;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    if (shape->ptcnt > 2) return -1;

    pcalb = _get_calb_ptr(pvaa, calb);

    _set_calb_figure(pcalb, shape->pt, shape->ptcnt, COLOR_PRG_IDX(UX_COLOR_FF007F));
    _set_calb_icon_upper(pcalb, shape->iupp_color_idx);
    _set_calb_icon_lower(pcalb, shape->ilow_color_idx);   
    _set_calb_value(pcalb, shape->value, shape->iupp_color_idx);    
    return 0;
}

int vaa_itx_get_calb_conf(VAAID id, ITX_CALBID calb, ITX_VACALB_CONF *conf)
{
    VACALBPTR pcalb;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcalb = _get_calb_ptr(pvaa, calb);
    _bring_calb_conf(pcalb, conf);
    return 0;
}

int vaa_itx_set_calb_conf(VAAID id, ITX_CALBID calb, ITX_VACALB_CONF *conf)
{
    VACALBPTR pcalb;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    pcalb = _get_calb_ptr(pvaa, calb);
    _apply_calb_conf(conf, pcalb);
    _update_calb_count(pvaa);
    return 0;
}

int vaa_itx_get_calb_confs_all(VAAID id, ITX_VACALB_CONF conf[32], int *cnt)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    *cnt = 0;
    if (!pvaa) return -1;

    _bring_calb_confs_all(pvaa, conf, cnt);
    return 0;
}

ITX_CALBID vaa_itx_find_calb(VAAID id, int x, int y)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    VACALBPTR pcalb;
    ITX_CALBID calb;
    
    IGPOINT mpt;
    IGPOINT fpt[MAX_PT];
    int fcnt = 0;
    int color_idx;
    int width, height;
    
    if (!pvaa) return -1;
    if (!dit_page_is_enabled(pvaa->ditid, pvaa->vacalb[0].dipage)) return -1;
    
    mpt.x = x;
    mpt.y = y;
    
    for (calb = 0; calb < MAX_CALB; calb++) {
        pcalb = _get_calb_ptr(pvaa, (ITX_CALBID)calb);
        if (!pcalb) continue;
        if (!pcalb->use) continue;

        if (_get_calb_figure(pcalb, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt) == 0) return calb;

        if (_get_calb_icon_upper(pcalb, &fpt[0], &width, &height, &color_idx) == -1) continue;
        if (_is_point_in_arc(mpt, fpt[0], width, height) == 0) return calb;        

        if (_get_calb_icon_lower(pcalb, fpt, &fcnt, &color_idx) == -1) continue;
        if (_is_point_in_figure(mpt, fpt, fcnt) == 0) return calb;        
    }
    return -1;
}

int vaa_itx_get_calb_result(VAAID id, ITX_VACALB_RESULT *res)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _bring_calb_res(pvaa, res);
    return 0;
}

int vaa_itx_set_calb_result(VAAID id, ITX_VACALB_RESULT *res)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    _apply_calb_res(pvaa, res);
    return 0;
}

int vaa_itx_activate_rule(VAAID id, DIT_PAGE_E page)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    if ((page != DIT_PAGE_ZONE) && (page != DIT_PAGE_CNTR)) return -1;

    if (_is_setup_rule(pvaa) || _is_display_rule(pvaa))
    {
        if (page == DIT_PAGE_ZONE) dit_enable_page(pvaa->ditid, pvaa->vazone[0].dipage);
        else if (page == DIT_PAGE_CNTR) dit_enable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
    }
    else
    {
        if (page == DIT_PAGE_ZONE) dit_disable_page(pvaa->ditid, pvaa->vazone[0].dipage);
        else if (page == DIT_PAGE_CNTR) dit_disable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
    }
        
    _redraw_rules_to_display(pvaa);
    return 0;
}

int vaa_itx_activate_all_rule(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;

    if (_is_setup_rule(pvaa) || _is_display_rule(pvaa))
    {
        dit_enable_page(pvaa->ditid, pvaa->vazone[0].dipage);
        dit_enable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
    }
    else
    {
        dit_disable_page(pvaa->ditid, pvaa->vazone[0].dipage);
        dit_disable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
    }

    _redraw_rules_to_display(pvaa);
    return 0;
}

int vaa_itx_deactivate_rule(VAAID id, DIT_PAGE_E page)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    if ((page != DIT_PAGE_ZONE) && (page != DIT_PAGE_CNTR)) return -1;
    
    if (page == DIT_PAGE_ZONE) dit_disable_page(pvaa->ditid, pvaa->vazone[0].dipage);
    else if (page == DIT_PAGE_CNTR) dit_disable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
        
    _redraw_rules_to_display(pvaa);
    return 0;
}

int vaa_itx_deactivate_all_rule(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    dit_disable_page(pvaa->ditid, pvaa->vazone[0].dipage);
    dit_disable_page(pvaa->ditid, pvaa->vacntr[0].dipage);
    
    _redraw_rules_to_display(pvaa);
    return 0;
}

int vaa_itx_activate_calb(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    dit_enable_page(pvaa->ditid, pvaa->vacalb[0].dipage);       
    _redraw_calbs_to_display(pvaa);
    return 0;
}


int vaa_itx_deactivate_calb(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    dit_disable_page(pvaa->ditid, pvaa->vacalb[0].dipage);       
    _redraw_calbs_to_display(pvaa);
    return 0;
}

int vaa_itx_activate_meta(VAAID id, VAA_META_E meta)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    dit_enable_page(pvaa->ditid, pvaa->vameta[meta].dipage);
    return 0;
}

int vaa_itx_activate_all_meta(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    for (i = 0; i < MAX_META; ++i)
        dit_enable_page(pvaa->ditid, pvaa->vameta[i].dipage);

    return 0;
}

int vaa_itx_deactivate_meta(VAAID id, VAA_META_E meta)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    dit_disable_page(pvaa->ditid, pvaa->vameta[meta].dipage);
    return 0;
}

int vaa_itx_deactivate_all_meta(VAAID id)
{
    int i;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    for (i = 0; i < MAX_META; ++i)
        dit_disable_page(pvaa->ditid, pvaa->vameta[i].dipage);

    return 0;
}

int vaa_itx_deactivate_all(VAAID id)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    if (!vaa_itx_is_dit_linked(id)) return -1;
    
    vaa_itx_deactivate_all_rule(id);
    vaa_itx_deactivate_calb(id);    
    vaa_itx_deactivate_all_meta(id);

    return 0;
}

int vaa_itx_is_active_changed(VAAID id)
{
    VCAData tmpdb;  
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return 0;

    //_print_db(pvaa);
    memset(&tmpdb, 0x00, sizeof(VCAData));
    memmove(&tmpdb, &pvaa->db, sizeof(VCAData));
    _copy_prop_to_db(&pvaa->prop, &tmpdb);
//  DMSG(1, "%d", (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData))));

    if ((tmpdb.prop.active) && (!pvaa->db.prop.active)) return 1;
    return 0;
}

int vaa_itx_is_db_changed(VAAID id)
{
    VCAData tmpdb;  
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return 0;

//_print_db(pvaa);
    memset(&tmpdb, 0x00, sizeof(VCAData));
    memmove(&tmpdb, &pvaa->db, sizeof(VCAData));
    _copy_prop_to_db(&pvaa->prop, &tmpdb);
    _copy_calbres_to_db(&pvaa->calbres, &tmpdb);
    _translate_rule_into_vcdata(pvaa, &tmpdb);  
    _translate_calb_into_vcdata(pvaa, &tmpdb);    
//  DMSG(1, "%d", (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData))));
    return (memcmp(&pvaa->db, &tmpdb, sizeof(VCAData)) != 0);
}

int vaa_itx_show_meta(VAAID id, gint cnt, ivcam_obj_t* data)
{
    int i, dipage, evt, j;
    float value;

    IGRECT rect;
    IXPOINT pt[MAX_PT];
    int npt;
    char strBuf[64];
    char strTemp[14];

    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    if (!pvaa) return -1;

    dipage = pvaa->vameta[0].dipage;
    _clear_meta(pvaa);

    if (cnt > MAX_TBOX) cnt = MAX_TBOX;

    // display meta data
    for (i = 0; i < cnt; ++i) 
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        pvaa->vatbox[i].dic[TRACKBOX] = 0;
        pvaa->vatbox[i].dic[TRACKBOX_OBJ_INFO] = 0;
        pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA] = 0;

        _get_meta_bound(&data[i].mobj, &rect);
        evt = _is_evented_meta(&data[i].mobj);

        if(_is_display_object_id(pvaa))                     // display object id
        {
            memset(strTemp, 0x00, sizeof(strTemp));
            snprintf(strTemp, sizeof(strTemp), "ID=%u", data[i].mobj.id);

            snprintf(strBuf, sizeof(strBuf), "%s", strTemp);
        }

        if(_is_3d_calbres_valid(pvaa) && _is_display_object_w3d(pvaa))               // display object width(3D)
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

        if(_is_3d_calbres_valid(pvaa) && _is_display_object_h3d(pvaa))               // display object height(3D)
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

        if(_is_3d_calbres_valid(pvaa) && _is_display_object_s3d(pvaa))                // display object speed(3D)
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

        if (_is_display_bounding_box(pvaa))                 // display bounding box
        {
            pvaa->vatbox[i].dic[TRACKBOX] = _add_trackbox_to_display(pvaa, dipage, &rect, evt);
            pvaa->vatbox[i].dic[TRACKBOX_OBJ_INFO] = _add_trackbox_obj_info_to_display(pvaa, dipage, &rect, strBuf, evt);    
        }
        
        if (_is_display_trajectory(pvaa))                   // display trajectory
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
                pt[j].x = data[i].traj[data[i].npts-j*adj_flag-1].x; 
                pt[j].y = data[i].traj[data[i].npts-j*adj_flag-1].y;
            }

            pvaa->vatbox[i].dic[TRACKBOX_OBJ_TRA] = _add_trajectory_to_display(pvaa, dipage, npt, &pt , evt);
            pvaa->vatbox[i].age = SEC_2;
        }
    }

    pvaa->cnt_tbox = cnt;


    return 0;
}

int vaa_itx_show_event(VAAID id, gint cnt, ivca_rule_event_t* data)
{
    int i;
    ITX_ZONEID zone;
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    
    if (!pvaa) return -1;
    if (!_is_display_rule(pvaa)) return -1;

    for (i = 0; i < cnt; ++i) 
    {   
        zone = _get_matched_zoneid(pvaa, data[i].rule_id);

        if (zone != -1) {
            pzone = _get_zone_ptr(pvaa, zone);
            pzone->event_time = 20;
                
            DMSG(9, "event type = %x\n", data->type);
        }
    }

    return 0;
}

int vaa_itx_show_zone_value_event(VAAID id, gint cnt, ivca_rule_event_t* data)
{
    int i, page;
    ITX_ZONEID zone;    
    VAZONEPTR pzone;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;

    if (!pvaa) return -1;
    if (!_is_display_rule(pvaa)) return -1;

    for (i = 0; i < cnt; ++i) 
    {   
        zone = _get_matched_zoneid(pvaa, data[i].rule_id);

        if (zone != -1) {
            pzone = _get_zone_ptr(pvaa, zone);
            page = pvaa->vazone[zone].dipage;            
            dit_remove_dic(pvaa->ditid, page, pzone->dic[COUNT_VALUE]);            

            pzone->cnt_value++;
            _add_zone_value_to_display(pvaa, page, pzone, pzone->cnt_value);
        }
    }

    return 0;
}

int vaa_itx_show_counter_value_event(VAAID id, gint count, ivca_meta_cnt_t* data)
{
    int i, page;
    VACNTRPTR pcntr;
    ITXVAA_T *pvaa = (ITXVAA_T *)id;
    ivca_meta_cnt_t *p = data;

    if (!pvaa) return -1;
    if (!_is_display_rule(pvaa)) return -1;

#if 0
    pcntr = _get_cntr_ptr(pvaa, p->id);    
    page = pvaa->vacntr[p->id].dipage;
    dit_remove_dic(pvaa->ditid, page, pcntr->dic[COUNT_VALUE]);

    _add_cntr_value_to_display(pvaa, page, pcntr, p->value);
    pcntr->cnt_value = p->value;
#else
    for ( i = 0; i < count; i++, p++)
    {
        pcntr = _get_cntr_ptr(pvaa, p->id);
        page = pvaa->vacntr[p->id].dipage;
        dit_remove_dic(pvaa->ditid, page, pcntr->dic[COUNT_VALUE]);
        
        _add_cntr_value_to_display(pvaa, page, pcntr, p->value);
        pcntr->cnt_value = p->value;
    }
#endif

    return 0;
}

ITX_CNTRID vaa_itx_get_counted_rule(VAAID id, unsigned short countid)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)id;

    if (!pvaa) return -1;

    _get_matched_cntrid(pvaa, countid);
    return -1;
}

int vaa_itx_parse_event(VAAID vaaid, ivca_rule_event_t *pevt, int *zid, int *cid)
{
	ITXVAA_T *pvaa = (ITXVAA_T *)vaaid;
	int ret = ITX_RULETYPE_NONE;

	if (!pvaa) return ITX_RULETYPE_NONE;

	*zid = -1;
	*cid = -1;

	if (pevt->type & IVCA_ET_COUNTER) {
		ret = ITX_RULETYPE_COUNTER; 
		*cid = _get_cntr_id(pvaa, pevt->rule_id);
	}
	else {
		switch (pevt->type & 0xFFF) {
		case IVCA_ET_ENTER: 
			ret = ITX_RULETYPE_ENTER; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_EXIT: 
			ret = ITX_RULETYPE_EXIT; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_STOPPED:
			ret = ITX_RULETYPE_STOPPED; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_LOITERED:
			ret = ITX_RULETYPE_LOITERED; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_FALL:
			ret = ITX_RULETYPE_FALL; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_REMOVED:
			ret = ITX_RULETYPE_REMOVED; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_DIR_POS:
			ret = ITX_RULETYPE_FOWARD; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;

		case IVCA_ET_DIR_NEG:
			ret = ITX_RULETYPE_REVERSE; 
			*zid = _get_zone_id(pvaa, pevt->rule_id);
			break;
        default:
            break;
		}
	}

	return (int)ret;    
}

int vaa_itx_timer_proc(VAAID vaaid)
{
    ITXVAA_T *pvaa = (ITXVAA_T *)vaaid;
    if (!pvaa) return -1;

    _proc_event_detecting(pvaa);
    _proc_meta_erasing(pvaa);

    return 0;
}

