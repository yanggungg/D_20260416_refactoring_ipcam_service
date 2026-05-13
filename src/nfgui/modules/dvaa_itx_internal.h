/*
 * dvaa_itx_internal.h
 *  - deeplearning video analytics agent internal
 *  - dependencies :
 *      DIT 
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#ifndef __DVAA_ITX_INTERNAL_H
#define __DVAA_ITX_INTERNAL_H

#include "nfdal.h"
#include "dit.h"
#include "dvaa_itx.h"

#define MAX_ZONE            16
#define MAX_CNTR            16
#define MAX_CALB            32
#define MAX_PATT            16
#define MAX_META            4
#define MAX_DIC             6
#define MAX_TBOX            256
#define MAX_EXTZONE         32
#define MAX_FRZONE          16
#define MAX_LPRZONE         16

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

#define _IS_CNT_LINE(pr)    (((pr->type & MASK_FIGURE) == MASK_LINE) && \
                                (pr->type & MASK_COUNT))
#define _IS_CNT_AREA(pr)    (((pr->type & MASK_FIGURE) == MASK_AREA) && \
                                (pr->type & MASK_COUNT))
#define _IS_DIRCTRL(pr)     (pr->type & MASK_DIRCTRL)
#define _IS_COUNTER(pr)     (pr->type & MASK_COUNT)
#define _IS_CALB(pr)        (pr->type & MASK_CALB)

#define CR_NORMAL_LINE      1102
#define CR_NORMAL_AREA      1105
#define CR_NORMAL_TBOX      1104
#define CR_EVENT_TBOX       1103
#define CR_EVENT_RULE       0xff    //COLOR_FF0000
#define IS_LINE_FIGURE(f)   (f->cnt == 2)
#define SEC_2           (4)     // 2000ms

#define IFIGURE 0
#define INFO            1
#define DIRCTRL         2
#define COUNT_VALUE     3
#define CALB_ICUPP      4
#define CALB_ICLOW      5

enum {
    TRACKBOX            = 0,
    TRACKBOX_OBJ_INFO,
    TRACKBOX_OBJ_TRA
};

typedef struct _ITX_DIRINFO {
    IGPOINT         pt[32];
} ITX_DIRINFO;

typedef struct _ITX_TEXTINFO {
    char            str[32];
    IGRECT          rect;
    int             font;
} ITX_TEXTINFO;

typedef struct _ITX_DVAZONE {
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

    DvaBxZone       zn_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_DVAZONE;

typedef struct _ITX_DVACNTR {
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

    DvaBxCntr         ct_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_DVACNTR;

typedef struct _ITX_DVACALB {
    int             use;
    int             id;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;    

    DvaBxCalb         cb_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_DVACALB;

typedef struct _ITX_DVAMETA {
    int             use;
    DVAA_META_E     meta;
    int             dipage;
} ITX_DVAMETA;

typedef struct _ITX_DVATRACKBOX {
    DICPTR          dic[MAX_DIC];
    int             age;
} ITX_DVATRACKBOX;

typedef struct _ITX_EXTZONE {
    int             use;
    int             occupied;    
    BITMASK         type;
    int             sty_highlight;
    int             event_time;

    char            trigger_type[128];
    char            trigger_name[128];
    StdPoint        pt[256];
    int             npts;
    unsigned int    color;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_EXTZONE;

typedef struct _ITX_FRZONE {
    int             use;
    int             id;
    unsigned int    trigger;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;

    int             value_en;
    int             cnt_value;

    DvaBxFaceZone   zn_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_FRZONE;

typedef struct _ITX_LPRZONE {
    int             use;
    int             id;
    unsigned int    trigger;
    int             occupied;
    BITMASK         type;
    int             sty_highlight;

    int             focus;
    int             blink_step;
    int             blink_en;
    int             event_time;

    int             value_en;
    int             cnt_value;

    DvaBxPlatenoZone zn_data;
    int             dipage;

    DICPTR          dic[MAX_DIC];
} ITX_LPRZONE;

typedef struct _ITX_DBSLOT {
    char            zn_data[IVCA_MAX_ZONES];
    char            ct_data[IVCA_MAX_CNTRS];
} ITX_DBSLOT;

typedef struct _ITXDVAA_T {
    int             ch;
    DITID           ditid;
    ITX_DBSLOT      used;

    DvaBxData         db;
    DvaBxPropData     prop;
    DvaBxCalbResData  calbres;
    DvaBxEOsdData     eosd;

    DvaBxFaceData         fr_db;
    DvaBxFacePropData     fr_prop;

    DvaBxPlatenoData      lpr_db;
    DvaBxPlatenoPropData  lpr_prop;

    ITX_ALGOTYPE_E  algo_type;
    int             st_rule;
    int             ext_rule;

    int             cnt_zone;
    ITX_DVAZONE     dvazone[MAX_ZONE];
    int             cnt_cntr;
    ITX_DVACNTR     dvacntr[MAX_CNTR];
    int             cnt_calb;
    ITX_DVACALB     dvacalb[MAX_CALB];
    int             cnt_extzone;
    ITX_EXTZONE     extzone[MAX_EXTZONE];
    int             cnt_frzone;
    ITX_FRZONE      frzone[MAX_FRZONE];
    int             cnt_lprzone;
    ITX_LPRZONE     lprzone[MAX_LPRZONE];        
    int             cnt_meta;
    ITX_DVAMETA     dvameta[MAX_META];
    int             cnt_tbox;
    ITX_DVATRACKBOX dvatbox[MAX_TBOX];
} ITXDVAA_T;


typedef ITX_DVAZONE*    DVAZONEPTR;
typedef ITX_DVACNTR*    DVACNTRPTR;
typedef ITX_DVACALB*    DVACALBPTR;
typedef ITX_EXTZONE*    EXTZONEPTR;
typedef ITX_FRZONE*     FRZONEPTR;
typedef ITX_LPRZONE*    LPRZONEPTR;



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _dvaa_init_figure(FIGURE_INFO *info);
int _dvaa_construct_figure(IGPOINT *pt, int cnt, FIGURE_INFO *info);
int _dvaa_construct_figure_rect(IGRECT *rect, FIGURE_INFO *info);

int _dvaa_itx_detector_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_detector_pb_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_detector_reload(DVAAID id);
int _dvaa_itx_detector_load_db(DVAAID id);
int _dvaa_itx_detector_save_db(DVAAID id);
int _dvaa_itx_detector_is_db_changed(DVAAID id);

int _dvaa_itx_face_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_face_pb_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_face_reload(DVAAID id);
int _dvaa_itx_face_load_db(DVAAID id);
int _dvaa_itx_face_save_db(DVAAID id);
int _dvaa_itx_face_is_db_changed(DVAAID id);

int _dvaa_itx_plateno_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_plateno_pb_init(ITXDVAA_T *pdvaa);
int _dvaa_itx_plateno_reload(DVAAID id);
int _dvaa_itx_plateno_load_db(DVAAID id);
int _dvaa_itx_plateno_save_db(DVAAID id);
int _dvaa_itx_plateno_is_db_changed(DVAAID id);

DVAZONEPTR _dvaa_itx_get_zone_ptr(ITXDVAA_T *pdvaa, ITX_ZONEID zone);
DVACNTRPTR _dvaa_itx_get_cntr_ptr(ITXDVAA_T *pdvaa, ITX_CNTRID cntr);
DVACALBPTR _dvaa_itx_get_calb_ptr(ITXDVAA_T *pdvaa, ITX_CALBID calb);
EXTZONEPTR _dvaa_itx_get_extzone_ptr(ITXDVAA_T *pdvaa, ITX_EXTZONEID zone);
FRZONEPTR _dvaa_itx_get_frzone_ptr(ITXDVAA_T *pdvaa, ITX_FRZONEID zone);
LPRZONEPTR _dvaa_itx_get_lprzone_ptr(ITXDVAA_T *pdvaa, ITX_LPRZONEID zone);

#endif
