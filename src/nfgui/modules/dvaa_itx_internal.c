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




////////////////////////////////////////////////////////////
//
// private variables
//

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

static int _detector_print_zonelist(DvaBxZoneData *zonelist)
{
    int i;
    
    if (zonelist) {
		printf("zonelist-------------------------------------------------------------------\n");
        for (i = 0; i < 16; ++i) {
            printf("[%d] name           = [%s]\n", i, zonelist->zone[i].name);
            printf("[%d] id             = [%d]\n", i, zonelist->zone[i].id);
            printf("[%d] type           = [%u]\n", i, zonelist->zone[i].type);
            printf("[%d] active         = [%d]\n", i, zonelist->zone[i].active);
            printf("[%d] enabled        = [%d]\n", i, zonelist->zone[i].enabled);
            printf("[%d] interest_obj   = [%s]\n", i, zonelist->zone[i].interest_obj);
            printf("[%d] stop time      = [%u]\n", i, zonelist->zone[i].stop_time);
            printf("[%d] abandon time   = [%u]\n", i, zonelist->zone[i].abandon_time);
            printf("[%d] remove time    = [%u]\n", i, zonelist->zone[i].remove_time);
            printf("[%d] loiter time    = [%u]\n", i, zonelist->zone[i].loiter_time);
            printf("[%d] fall time      = [%u]\n", i, zonelist->zone[i].fall_time);
            printf("[%d] ecolor         = [%u]\n", i, zonelist->zone[i].ecolor);
            printf("[%d] ecolor_sens    = [%d]\n", i, zonelist->zone[i].ecolor_sens);
            printf("[%d] size_min[0]    = [%u]\n", i, zonelist->zone[i].size_min[0]);
            printf("[%d] size_min[1]    = [%u]\n", i, zonelist->zone[i].size_min[1]);
            printf("[%d] size_max[0]    = [%u]\n", i, zonelist->zone[i].size_max[0]);
            printf("[%d] size_max[1]    = [%u]\n", i, zonelist->zone[i].size_max[1]);
            printf("[%d] speed_min      = [%u]\n", i, zonelist->zone[i].speed_min);
            printf("[%d] speed_max      = [%u]\n", i, zonelist->zone[i].speed_max);
            printf("[%d] eclass         = [%u]\n", i, zonelist->zone[i].eclass);
            printf("[%d] color          = [%u]\n", i, zonelist->zone[i].color);
            printf("[%d] npts           = [%d]\n", i, zonelist->zone[i].npts);
            printf("[%d] pt             = [%d, %d], [%d, %d], [%d, %d], [%d, %d]\n", i, 
                    zonelist->zone[i].pt[0].x, zonelist->zone[i].pt[0].y,
                    zonelist->zone[i].pt[1].x, zonelist->zone[i].pt[1].y,
                    zonelist->zone[i].pt[2].x, zonelist->zone[i].pt[2].y,
                    zonelist->zone[i].pt[3].x, zonelist->zone[i].pt[3].y,
                    zonelist->zone[i].pt[4].x, zonelist->zone[i].pt[4].y);
        }
		printf("--------------------------------------------------------------------------\n");
    }
    return 0;
}

static int _detector_print_cntrlist(DvaBxCntrData *cntrlist)
{
    int i;

    if (cntrlist) {
		printf("cntrlist------------------------------------------------------------------\n");
        for (i = 0; i < 16; ++i) {
            printf("[%d] name           = [%s]\n", i, cntrlist->cntr[i].name);
            printf("[%d] id             = [%d]\n", i, cntrlist->cntr[i].id);
            printf("[%d] type           = [%u]\n", i, cntrlist->cntr[i].type);
            printf("[%d] active         = [%d]\n", i, cntrlist->cntr[i].active);
            printf("[%d] enabled        = [%d]\n", i, cntrlist->cntr[i].enabled);
            printf("[%d] zid_up         = [%d]\n", i, cntrlist->cntr[i].zid_up);
            printf("[%d] zid_dn         = [%d]\n", i, cntrlist->cntr[i].zid_dn);
            printf("[%d] evalue         = [%d]\n", i, cntrlist->cntr[i].evalue);
            printf("[%d] resetalert     = [%u]\n", i, cntrlist->cntr[i].resetalert);
            printf("[%d] color          = [%u]\n", i, cntrlist->cntr[i].color);
            printf("[%d] pt             = [%d, %d], [%d, %d], [%d, %d], [%d, %d]\n", i, 
                    cntrlist->cntr[i].pt[0].x, cntrlist->cntr[i].pt[0].y,
                    cntrlist->cntr[i].pt[1].x, cntrlist->cntr[i].pt[1].y,
                    cntrlist->cntr[i].pt[2].x, cntrlist->cntr[i].pt[2].y,
                    cntrlist->cntr[i].pt[3].x, cntrlist->cntr[i].pt[3].y,
                    cntrlist->cntr[i].pt[4].x, cntrlist->cntr[i].pt[4].y);
        }
		printf("--------------------------------------------------------------------------\n");        
    }
    return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

DVAZONEPTR _dvaa_itx_get_zone_ptr(ITXDVAA_T *pdvaa, ITX_ZONEID zone)
{
    return (DVAZONEPTR)&pdvaa->dvazone[zone];
}

DVACNTRPTR _dvaa_itx_get_cntr_ptr(ITXDVAA_T *pdvaa, ITX_CNTRID cntr)
{
    return (DVACNTRPTR)&pdvaa->dvacntr[cntr];
}

DVACALBPTR _dvaa_itx_get_calb_ptr(ITXDVAA_T *pdvaa, ITX_CALBID calb)
{
    return (DVACALBPTR)&pdvaa->dvacalb[calb];
}

EXTZONEPTR _dvaa_itx_get_extzone_ptr(ITXDVAA_T *pdvaa, ITX_EXTZONEID zone)
{
    return (EXTZONEPTR)&pdvaa->extzone[zone];
}

FRZONEPTR _dvaa_itx_get_frzone_ptr(ITXDVAA_T *pdvaa, ITX_FRZONEID zone)
{
    return (FRZONEPTR)&pdvaa->frzone[zone];
}

LPRZONEPTR _dvaa_itx_get_lprzone_ptr(ITXDVAA_T *pdvaa, ITX_LPRZONEID zone)
{
    return (LPRZONEPTR)&pdvaa->lprzone[zone];
}

int dvaa_itx_convert_rgb_color_to_coloridx(unsigned int rgb_col)
{
    return _convert_rgb_color_to_coloridx(rgb_col);
}

unsigned int dvaa_itx_convert_to_coloridx_rgb_color(int idx)
{
    return _convert_to_coloridx_rgb_color(idx);
} 

int dvaa_itx_detector_print_db(DvaBxZoneData *zonelist, DvaBxCntrData *cntrlist)
{
    if (zonelist) _detector_print_zonelist(zonelist);
    if (cntrlist) _detector_print_cntrlist(cntrlist);
    return 0;
}
