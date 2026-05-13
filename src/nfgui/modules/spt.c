/*
 * spt.c
 *  - spot module 
 *  - dependencies :
 *      
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 22, 2012
 *
 */

#include "iux_afx.h"
#include "spt.h"
#include <memory.h>
#include "ix_func.h"
#include "iux_types.h"
#include "nfdal.h"
#include "ix_mem.h"
#include "nf_common.h"
#include "nf_api_live.h"


#define DBG_LEVEL       1
#define DBG_MODULE      "SPT"

#if defined(_SUPPORT_ORDER_SPOT_AUX)
#define INIT_SPOT_IDX               (0)
#define INIT_AUX_IDX                (SPOTSD_PORT_CNT)
#elif defined(_SUPPORT_ORDER_AUX_SPOT)
#define INIT_AUX_IDX                (0)
#define INIT_SPOT_IDX               (SPOTAUX_PORT_CNT)
#else   //only spot
#define INIT_SPOT_IDX               (0)
#define INIT_AUX_IDX                (0) //unused
#endif

#ifdef GUI_32CH_SUPPORT
    #define DIV9_CH_CNT     9
#elif GUI_16CH_SUPPORT
    #if !defined (_ANF5G_1648D)
    #define DIV9_CH_CNT     9
    #else
    #define DIV9_CH_CNT     8
    #endif
#else
    #define DIV9_CH_CNT     8
#endif


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _SPT_T {
    int                 ch;
    int                 updated;
    BITMASK             mask_osd;
    time_t              next_turn;
    int                 item_pos;
    SPOT_TYPE_E         spottype;
    SPOT_SCREEN_T       list[MAX_ITEM];
    ONOFF_E             pause;
    gboolean            block;

    SPOT_SCREEN_T       cur_scr;
    SPOT_MODE_E         cur_mode;
    SECOND_D            cur_dwell;
    SPOT_SCREEN_T       prev_scr;
    SPOT_MODE_E         prev_mode;
    SECOND_D            prev_dwell;

    SPOT_SERVICE_PROC   service_proc;
} SPT_T;



////////////////////////////////////////////////////////////
//
// private variable
//



////////////////////////////////////////////////////////////
//
// private functions
//

static int _convert_to_dwell(int index)
{
    int ret = 0;
    switch (index) {
    case 0: ret = 1;    break;
    case 1: ret = 2;    break;
    case 2: ret = 3;    break;
    case 3: ret = 5;    break;
    case 4: ret = 10;   break;
    case 5: ret = 15;   break;
    case 6: ret = 20;   break;
    case 7: ret = 30;   break;
    case 8: ret = 40;   break;
    case 9: ret = 60;   break;
    default: ret = 5;   break;
    }

    return ret;
}

static int _update_next_turn_time(SPT_T *pspt)
{
    pspt->next_turn = time(0) + pspt->cur_dwell;
    return 0;
}

static int _is_change_time(SPT_T *pspt)
{
    return (pspt->next_turn <= time(0));
}

static int _clear_list(SPT_T *pspt)
{
    int i;
    memset(&pspt->list, 0x00, sizeof(SPOT_SCREEN_T) * MAX_ITEM);

    for (i = 0; i < MAX_ITEM; ++i) pspt->list[i].type = SCR_NONE;
    return 0;
}

static int _spot_off(int ch)
{
    gchar ch_arr[32] = {0xFF, };
    gboolean covert_arr[32] = {TRUE, };

    memset(ch_arr, 0xFF, sizeof(ch_arr));
    ch_arr[0] = 0;  
    
    nf_live_change_spot(NF_DISPLAY_FULL, ch_arr, covert_arr, 0);

    return 0;
}

static int _reload_config_SPOTSD(SPT_T *pspt)
{
    int i, j;
    SpotData spotdata;

    memset(&spotdata, 0x00, sizeof(SpotData));   
    DAL_get_spot_data(&spotdata, INIT_SPOT_IDX+pspt->ch);

    pspt->pause = (spotdata.valid_mode == 2 ? 0 : 1);        
            
    for (j = 0; j < spotdata.numItems; ++j) {
        pspt->list[j].type = spotdata.items[j].type;
        memcpy(pspt->list[j].conf, spotdata.items[j].conf, sizeof(int) * MAX_ITEM);
    }
    
    return 0;
}

static int _reload_config_AUXSD(SPT_T *pspt)
{
    int i, j;
    int ch;
    SpotData spotdata;

    memset(&spotdata, 0x00, sizeof(SpotData));    
    DAL_get_spot_data(&spotdata, INIT_AUX_IDX+pspt->ch);

    pspt->pause = (spotdata.valid_mode == 2 ? 0 : 1);        
            
    for (j = 0; j < spotdata.numItems; ++j) {
        pspt->list[j].type = spotdata.items[j].type;
        memcpy(pspt->list[j].conf, spotdata.items[j].conf, sizeof(int) * MAX_ITEM);
    }
    
    return 0;
}

static int _reload_config_SPOTHD(SPT_T *pspt)
{
    int i, j;
    SpotData spotdata;

    memset(&spotdata, 0x00, sizeof(SpotData));
    DAL_get_HD_spot_data(&spotdata, pspt->ch);

    pspt->pause = (spotdata.valid_mode == 2 ? 0 : 1);        
            
    for (j = 0; j < spotdata.numItems; ++j) {
        pspt->list[j].type = spotdata.items[j].type;
        memcpy(pspt->list[j].conf, spotdata.items[j].conf, sizeof(int) * MAX_ITEM);
    }
    
    return 0;
}

static int _reload_config_SPOTDUAL(SPT_T *pspt)
{
    int i, j;
    AdvDualData dualdata;

    memset(&dualdata, 0x00, sizeof(AdvDualData));    
    DAL_get_advanced_dual_data(&dualdata);

    pspt->pause = (dualdata.mode == 1 ? 0 : 1);
            
    for (j = 0; j < dualdata.spot_cnt; ++j) {
        pspt->list[j].type = dualdata.spot[j].type;
        memcpy(pspt->list[j].conf, dualdata.spot[j].conf, sizeof(int) * MAX_ITEM);
    }
    
    return 0;
}

static int _reload_config(SPT_T *pspt)
{
    MonitorData mondata;
    ONOFF_E temp = pspt->pause;

    DMSG(1,"");
    memset(&mondata, 0x00, sizeof(MonitorData));
    DAL_get_monitor_data(&mondata);

    if ((pspt->spottype == SPOT_TYPE_SD) || (pspt->spottype == AUX_TYPE_SD) || (pspt->spottype == SPOT_TYPE_DUAL))
        pspt->cur_dwell = mondata.spotDwell;
    else if (pspt->spottype == SPOT_TYPE_HD)
        pspt->cur_dwell = mondata.hd_spotDwell;
        
    _clear_list(pspt);

    if (pspt->spottype == SPOT_TYPE_SD) 
        _reload_config_SPOTSD(pspt);
    else if (pspt->spottype == AUX_TYPE_SD)
        _reload_config_AUXSD(pspt);
    else if (pspt->spottype == SPOT_TYPE_HD)
        _reload_config_SPOTHD(pspt);
    else if (pspt->spottype == SPOT_TYPE_DUAL)
        _reload_config_SPOTDUAL(pspt);   
    
    if(temp == 0 && pspt->pause == 1)    
        _spot_off(pspt->ch);
    
    pspt->item_pos = -1;
    pspt->updated = 0;

    return 0;
}


static int _move_to_next(SPT_T *pspt)
{
    SPOT_SCREEN_T *next = &pspt->cur_scr;
    ++pspt->item_pos;
    if (pspt->item_pos >= MAX_ITEM) pspt->item_pos = 0;
    if (pspt->list[pspt->item_pos].type == SCR_NONE) pspt->item_pos = 0;

    memcpy(next, &pspt->list[pspt->item_pos], sizeof(SPOT_SCREEN_T));
    return 0;
}

static int _convert_to_count(int type, SPOT_TYPE_E stype)
{
    switch (type) {
    case SCR_1DIV: return 1; break;
    case SCR_4DIV: return 4; break;
#ifndef GUI_4CH_SUPPORT     
#ifndef _NOT_SUPPORT_SPC_DIV
    case SCR_6DIV: return 6; break;
    case SCR_8DIV: return 8; break;
#endif  
    case SCR_9DIV:
        if (stype != SPOT_TYPE_DUAL)
        {
            return DIV9_CH_CNT;
        }
        else
        {
            if (GUI_CHANNEL_CNT == 8)
                return 8;
            else
                return 9;
        }
        break;
#ifndef GUI_8CH_SUPPORT 
    case SCR_16DIV: return 16; break;
#ifndef GUI_16CH_SUPPORT    
    case SCR_36DIV: return 32; break;
#endif
#endif  
#endif  
    }

    DMSG(1, "COUNT ERROR\n");
    return 1;
}

static int _convert_to_mode(int type)
{
    switch (type) {
    case SCR_1DIV: return NF_DISPLAY_FULL; break;
    case SCR_4DIV: return NF_DISPLAY_QUAD; break;
#ifndef GUI_4CH_SUPPORT     
#ifndef _NOT_SUPPORT_SPC_DIV    
    case SCR_6DIV: return NF_DISPLAY_HEXA_A; break;
    case SCR_8DIV: return NF_DISPLAY_OCTA_A; break;
#endif  
    case SCR_9DIV: return NF_DISPLAY_NONA; break;
#ifndef GUI_8CH_SUPPORT     
    case SCR_16DIV: return NF_DISPLAY_HEXADECA; break;
#ifndef GUI_16CH_SUPPORT    
    case SCR_36DIV: return NF_DISPLAY_HEXATRICONTA; break;
#endif
#endif  
#endif  
    }

    DMSG(1, "TYPE ERROR\n");
    return NF_DISPLAY_FULL;
}

static gboolean _is_block(SPT_T *pspt)
{
    if(pspt->block)
        return TRUE;
    else
        return FALSE;
}

static int _apply_new_conf(SPT_T *pspt)
{
#define MAX_SPOT_CH_NUM (32)

    int i;
    int cnt;
    BITMASK covert_msk;
    SPOT_SCREEN_T *cur = &pspt->cur_scr;
    NF_DISPLAY_E mode;
    gchar ch_arr[MAX_SPOT_CH_NUM];
    gboolean covert_arr[MAX_SPOT_CH_NUM];

    cnt = _convert_to_count(cur->type, pspt->spottype);
    mode = _convert_to_mode(cur->type);

    memset(ch_arr, 0xFF, sizeof(ch_arr));
    memset(covert_arr, 0, sizeof(covert_arr));  //TODO

    DMSG(9, "TYPE = %d", cur->type);
    for (i = 0; i < cnt; ++i) {
        DMSG(9, "CH = %d, ", cur->conf[i]);
        g_assert(cur->conf[i]<MAX_SPOT_CH_NUM);
        ch_arr[cur->conf[i]] = i;
    }
    printf("\n");

    var_get_covert_mask(&covert_msk);
    for (i = 0; i < var_get_ch_count(); ++i) {
        if (covert_msk & (1ULL << i)) covert_arr[i] = TRUE;
    }    

    if(_is_block(pspt))
        return 0;

    if (pspt->spottype == SPOT_TYPE_SD)
    {
        DMSG(1, "SPOT_TYPE_SD");        
#if defined(_SPOT_CALLER_DRIVER)
    return 0;
#endif      
        nf_live_change_spot(mode, ch_arr, covert_arr, 0);
    }
    else if (pspt->spottype == AUX_TYPE_SD)
    {
#if defined(_AUX_CALLER_DRIVER)
    return 0;
#endif    
        nf_live_change_spot(mode, ch_arr, covert_arr, 0);
    }
    else if (pspt->spottype == SPOT_TYPE_HD)
    {
        DMSG(1, "");
        //nf_live_sub_change(mode, 0, 0, 1920, 1080, ch_arr, covert_arr, 0);
    }
    else if (pspt->spottype == SPOT_TYPE_DUAL)
    {   
        DMSG(1, "SPOT_TYPE_DUAL");
        nf_live_sub_change(mode, 0, 0, 1920, 1080, ch_arr, covert_arr, 0, 0);
    }

    return 0;
}

static int _backup_cur(SPT_T *pspt)
{
    if (pspt->cur_mode == SPOT_POPUP) return 0;

    memcpy(&pspt->prev_scr, &pspt->cur_scr, sizeof(SPOT_SCREEN_T));
    pspt->prev_mode = pspt->cur_mode;
    pspt->prev_dwell = pspt->cur_dwell;
    return 0;
}

static int _restore_prev(SPT_T *pspt)
{
    memcpy(&pspt->cur_scr, &pspt->prev_scr, sizeof(SPOT_SCREEN_T));
    pspt->cur_mode = pspt->prev_mode;
    pspt->cur_dwell = pspt->prev_dwell;
    if (pspt->cur_mode == SPOT_SEQUENCE) _update_next_turn_time(pspt);
    return 0;
}

static int _is_seq_mode(SPT_T *pspt)
{
    return pspt->cur_mode == SPOT_SEQUENCE;
}

static int _work_sequence(SPT_T *pspt)
{
    if (pspt->updated == 1) _reload_config(pspt);   
    if (pspt->pause) return 0;
    if (!_is_change_time(pspt)) return 0;

    _update_next_turn_time(pspt);
    _move_to_next(pspt);
    _apply_new_conf(pspt);
    _backup_cur(pspt);
    
    return 0;
}

static int _work_popup(SPT_T *pspt)
{
    if (!_is_change_time(pspt)) return 0;

    _restore_prev(pspt);
    _apply_new_conf(pspt);
    DMSG(1,"");
    
    return 0;
}

static int _proc_spot_service(SPT_T *pspt)      // called by SCM thread
{
    DMSG(9, "%d", pspt->cur_mode);
    switch (pspt->cur_mode) {
    case SPOT_SEQUENCE: _work_sequence(pspt); break;
    case SPOT_POPUP: _work_popup(pspt); break;
    case SPOT_FIX: break;
    }

    return 0;
}

static int _init(SPT_T *pspt, int ch, SPOT_TYPE_E spottype)
{
    SpotData spotdata;

    pspt->updated = 1;      // first start
    pspt->service_proc = _proc_spot_service;
    pspt->ch = ch;
    pspt->item_pos = -1;
    pspt->cur_mode = SPOT_SEQUENCE;
    pspt->spottype = spottype;
    pspt->block = OFF;  

    _update_next_turn_time(pspt);
    return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

SPT_ID *spt_create(int ch, SPOT_TYPE_E spottype)
{
    SPT_T *pspt = imalloc(sizeof(SPT_T));
    DMSG(1, "");
    _init(pspt, ch, spottype);

    return (SPT_ID)pspt;
}

int spt_destroy(SPT_ID sptid)
{
    // will never be called

    SPT_T *pspt = (SPT_T *)sptid;
    ifree(pspt);
    g_assert(0);
    return 0;
}

SPOT_SERVICE_PROC spt_get_procedure(SPT_ID sptid)
{
    SPT_T *pspt = (SPT_T *)sptid;
    return pspt->service_proc;
}

int spt_push_updating(SPT_ID sptid)
{
    SPT_T *pspt = (SPT_T *)sptid;
    pspt->updated = 1;
    DMSG(1,"");
    return 0;
}

int spt_display_spot(SPT_ID sptid, SPOT_SCREEN_T *spot_scr, SECOND_D dwell)
{
    SPT_T *pspt = (SPT_T *)sptid;

    _backup_cur(pspt);

    pspt->cur_scr = *spot_scr;
    pspt->cur_dwell = dwell;

    if (dwell == 0) pspt->cur_mode = SPOT_FIX;
    else {
        pspt->cur_mode = SPOT_POPUP;
        _update_next_turn_time(pspt);
    }
    _apply_new_conf(pspt);

    DMSG(1, "SPOT MODE = %d\n", pspt->cur_mode);
    return 0;
}

int spt_control_sequence(SPT_ID sptid, ONOFF_E onoff)
{
    SPT_T *pspt = (SPT_T *)sptid;

    pspt->pause = onoff;
    return 0;
}

int spt_control_osd(SPT_ID sptid, ONOFF_E onoff)
{
    SPT_T *pspt = (SPT_T *)sptid;

    return 0;
}

int spt_enable_spot(SPT_ID sptid)
{
    SPT_T *pspt = (SPT_T *)sptid;

    pspt->block = OFF;

    return 0;
}

int spt_disable_spot(SPT_ID sptid)
{
    SPT_T *pspt = (SPT_T *)sptid;

    pspt->block = ON;
    _spot_off(pspt->ch);
    return 0;
}

