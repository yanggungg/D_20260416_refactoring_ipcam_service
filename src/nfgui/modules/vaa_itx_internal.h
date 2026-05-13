/*
 * vaa_itx_internal.h
 *  - video analytics agent internal
 *  - dependencies :
 *      DIT 
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#ifndef __VAA_ITX_INTERNAL_H
#define __VAA_ITX_INTERNAL_H

#include "nfdal.h"
#include "dit.h"
#include "vaa_itx.h"

#define MAX_ZONE            16
#define MAX_CNTR            16
#define MAX_CALB            32
#define MAX_PATT            16
#define MAX_META            4
#define MAX_DIC             6
#define MAX_TBOX            256

#define MASK_FIGURE     0x000F
#define MASK_NOFIGURE   0x0000
#define MASK_LINE       0x0001
#define MASK_AREA       0x0002
#define MASK_COUNT      0x0010
#define MASK_DIRCTRL    0x0020
#define MASK_CALB       0x0100

#define _IS_NOFIGURE(pr)    ((pr->type & MASK_FIGURE) == 0x0000)
#define _IS_LINE(pr)        ((pr->type & MASK_FIGURE) == MASK_LINE)
#define _IS_AREA(pr)        ((pr->type & MASK_FIGURE) == MASK_AREA)

#define _IS_CNT_LINE(pr)    ((  (pr->type & MASK_FIGURE) == MASK_LINE) && \
                                (pr->type & MASK_COUNT))
#define _IS_CNT_AREA(pr)    ((  (pr->type & MASK_FIGURE) == MASK_AREA) && \
                                (pr->type & MASK_COUNT))
#define _IS_DIRCTRL(pr)     (pr->type & MASK_DIRCTRL)
#define _IS_COUNTER(pr)     (pr->type & MASK_COUNT)
#define _IS_CALB(pr)        (pr->type & MASK_CALB)


typedef struct _ITX_DIRINFO {
    IGPOINT         pt[32];
} ITX_DIRINFO;

typedef struct _ITX_TEXTINFO {
    char            str[32];
    IGRECT          rect;
    int             font;
} ITX_TEXTINFO;

typedef struct _ITX_VAZONE {
    int             use;
    int             id;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;

    int             value_en;
    int             cnt_value;

    VCAZone         zn_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_VAZONE;

typedef struct _ITX_VACNTR {
    int             use;
    int             id;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;
    
    int             value_en;
    int             cnt_value;

    VCACntr         ct_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_VACNTR;

typedef struct _ITX_VACALB {
    int             use;
    int             id;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;    

    VCACalb         cb_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_VACALB;

typedef struct _ITX_VAMETA {
    int             use;
    VAA_META_E      meta;
    int             dipage;
} ITX_VAMETA;

typedef struct _ITX_VATRACKBOX {
    DICPTR          dic[MAX_DIC];
    int             age;
} ITX_VATRACKBOX;

typedef struct _ITX_DBSLOT {
    char            zn_data[IVCA_MAX_ZONES];
    char            ct_data[IVCA_MAX_CNTRS];
} ITX_DBSLOT;

typedef struct _ITXVAA_T {
    int             ch;
    DITID           ditid;
    ITX_DBSLOT      used;

    VCAData         db;
    VCAPropData     prop;
    VCACalbResData  calbres;

    int             st_rule;

    int             cnt_zone;
    ITX_VAZONE      vazone[MAX_ZONE];
    int             cnt_cntr;
    ITX_VACNTR      vacntr[MAX_CNTR];
    int             cnt_calb;
    ITX_VACALB      vacalb[MAX_CALB];
    int             cnt_meta;
    ITX_VAMETA      vameta[MAX_META];
    int             cnt_tbox;
    ITX_VATRACKBOX  vatbox[MAX_TBOX];
} ITXVAA_T;


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _vaa_init_figure(FIGURE_INFO *info);
int _vaa_construct_figure(IGPOINT *pt, int cnt, FIGURE_INFO *info);
int _vaa_construct_figure_rect(IGRECT *rect, FIGURE_INFO *info);

int _vaa_itx_link_event(VAAID id);

#endif
