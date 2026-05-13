/*
 * vaa.c
 *  - video analytics agent
 *  - dependencies :
 *      
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#include "vaa.h"
#include "vaa_s1.h"
#include "vaa_s1_internal.h"
#include "vaa_itx.h"
#include "vaa_itx_internal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "var.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "VAA"



////////////////////////////////////////////////////////////
//
// private data type
//

typedef void*    VAAPTR;

typedef int (*VAA_VENDOR_INIT)(int, VAAID *, DITID *);
typedef int (*VAA_VENDOR_PB_INIT)(int, VAAID *, DITID *);

typedef int (*VAA_VENDOR_GET_DITID)(VAAID);
typedef int (*VAA_VENDOR_GET_CNTRID)(VAAID, unsigned short);

typedef int (*VAA_VENDOR_ACTIVATE_ALL_RULE)(VAAID);
typedef int (*VAA_VENDOR_DEACTIVATE_ALL_RULE)(VAAID);
typedef int (*VAA_VENDOR_ACTIVATE_CALB)(VAAID);
typedef int (*VAA_VENDOR_DEACTIVATE_CALB)(VAAID);
typedef int (*VAA_VENDOR_ACTIVATE_META)(VAAID, int);
typedef int (*VAA_VENDOR_DEACTIVATE_META)(VAAID, int);

typedef int (*VAA_VENDOR_RELOAD)(VAAID);
typedef int (*VAA_VENDOR_PARSE)(VAAID, ivca_rule_event_t *, int *, int *);
typedef int (*VAA_VENDOR_TIMER)(VAAID);
typedef int (*VAA_VENDOR_BLINK)(VAAID, int);

typedef int (*VAA_VENDOR_NOTI_VCA_EVENT)(NF_NOTIFY_INFO *);
typedef int (*VAA_VENDOR_NOTI_VCA_TRACK_INFO)(NF_NOTIFY_INFO *);
typedef int (*VAA_VENDOR_NOTI_VCA_COUNTER_INFO)(NF_NOTIFY_INFO *);
typedef int (*VAA_VENDOR_NOTI_VCA_META_DATA)(NF_NOTIFY_INFO *);
typedef int (*VAA_VENDOR_NOTI_VCA_ANALYZE_EVENT)(NF_NOTIFY_INFO *);


typedef struct _VAA_T {
    int                             support;
    VAA_VENDOR_INIT                 init_proc;
    VAA_VENDOR_PB_INIT              init_pb_proc;
    
    VAA_VENDOR_GET_DITID            get_dit_proc;
    VAA_VENDOR_GET_CNTRID           get_cntrid_proc;
    
    VAA_VENDOR_ACTIVATE_ALL_RULE    activate_all_rule_proc;
    VAA_VENDOR_DEACTIVATE_ALL_RULE  deactivate_all_rule_proc;    
    VAA_VENDOR_ACTIVATE_CALB        activate_calb_proc;    
    VAA_VENDOR_DEACTIVATE_CALB      deactivate_calb_proc;    
    VAA_VENDOR_ACTIVATE_META        activate_meta_proc;    
    VAA_VENDOR_DEACTIVATE_META      deactivate_meta_proc;    
    
    VAA_VENDOR_RELOAD               reload_proc;
    VAA_VENDOR_PARSE                parse_proc;
    VAA_VENDOR_TIMER                timer_proc;
    VAA_VENDOR_BLINK                blink_proc;
    
    VAA_VENDOR_NOTI_VCA_EVENT           notify_vca_event_proc;
    VAA_VENDOR_NOTI_VCA_TRACK_INFO      notify_vca_track_info_proc;
    VAA_VENDOR_NOTI_VCA_COUNTER_INFO    notify_vca_counter_info_proc;
    VAA_VENDOR_NOTI_VCA_META_DATA       notify_vca_meta_data_proc;
    VAA_VENDOR_NOTI_VCA_ANALYZE_EVENT   notify_vca_analyze_event_proc;

    VAAID               vaa[32];
    DITID               dit[32];
} VAA_T;

////////////////////////////////////////////////////////////
//
// private variables 
//

static VAA_T ivaa;



////////////////////////////////////////////////////////////
//
// private functions
//

static gboolean _proc_timer(gpointer data)
{
    int i;
    VAA_T *pivaa = (VAA_T *)data;

    for (i = 0; i < var_get_ch_count(); ++i) {
        pivaa->timer_proc(pivaa->vaa[i]);
    }

    return TRUE;
}



////////////////////////////////////////////////////////////
//
// public interfaces 
//

int vaa_init()
{
    int i;
    int vendor_code = var_get_vendor_code();
    memset(&ivaa, 0x00, sizeof(VAA_T));

#ifdef _SUPPORT_S1_STEP1
    return 0;
#endif

    if (!var_get_supported_dmva()) return 0;

    switch (vendor_code) {
    case 30:
        ivaa.init_proc = vaa_s1_init;
        ivaa.init_pb_proc = 0;
        ivaa.get_dit_proc = vaa_s1_get_dit;
        ivaa.get_cntrid_proc = vaa_s1_get_counted_rule;
        ivaa.activate_all_rule_proc = vaa_s1_activate_all_pattern;
        ivaa.deactivate_all_rule_proc = vaa_s1_deactivate_all_pattern;        
        ivaa.activate_calb_proc = 0;        
        ivaa.deactivate_calb_proc = 0;
        ivaa.activate_meta_proc = vaa_s1_activate_meta;        
        ivaa.deactivate_meta_proc = vaa_s1_deactivate_meta;                
        ivaa.reload_proc = vaa_s1_reload;
        ivaa.parse_proc = vaa_s1_parse_event;
        ivaa.timer_proc = vaa_s1_timer_proc;
        ivaa.blink_proc = vaa_s1_make_blinking;
        ivaa.notify_vca_event_proc = vaa_s1_notify_vca_event;
        ivaa.notify_vca_track_info_proc = vaa_s1_notify_vca_track_info;
        ivaa.notify_vca_counter_info_proc = 0;
        ivaa.notify_vca_meta_data_proc = vaa_s1_notify_vca_meta_data;
        ivaa.notify_vca_analyze_event_proc = 0;
        break;        
    default:
        ivaa.init_proc = vaa_itx_init;
        ivaa.init_pb_proc = vaa_itx_pb_init;
        ivaa.get_dit_proc = vaa_itx_get_dit;
        ivaa.get_cntrid_proc = vaa_itx_get_counted_rule;        
        ivaa.activate_all_rule_proc = vaa_itx_activate_all_rule;        
        ivaa.deactivate_all_rule_proc = vaa_itx_deactivate_all_rule;        
        ivaa.activate_calb_proc = vaa_itx_activate_calb;        
        ivaa.deactivate_calb_proc = vaa_itx_deactivate_calb;
        ivaa.activate_meta_proc = vaa_itx_activate_meta;        
        ivaa.deactivate_meta_proc = vaa_itx_deactivate_meta;                
        ivaa.reload_proc = vaa_itx_reload;
        ivaa.parse_proc = vaa_itx_parse_event;
        ivaa.timer_proc = vaa_itx_timer_proc;
        ivaa.blink_proc = 0;
        ivaa.notify_vca_event_proc = vaa_itx_notify_vca_event;
        ivaa.notify_vca_track_info_proc = vaa_itx_notify_vca_track_info;
        ivaa.notify_vca_counter_info_proc = vaa_itx_notify_vca_counter_info;
        ivaa.notify_vca_meta_data_proc = vaa_itx_notify_vca_meta_data;        
        ivaa.notify_vca_analyze_event_proc = vaa_itx_notify_vca_analyze_event;
        break;
    }

    ivaa.support = 1;

// LIVE    
    for (i = 0; i < var_get_ch_count(); ++i) {
        if (ivaa.init_proc) ivaa.init_proc(i, &ivaa.vaa[i], &ivaa.dit[i]);
    }

// PLAYBACK
    if (vendor_code != 30)
    {
        for (i = 16; i < 16+var_get_ch_count(); ++i) {
            if (ivaa.init_pb_proc) ivaa.init_pb_proc(i, &ivaa.vaa[i], &ivaa.dit[i]);
        }
    }

    g_timeout_add(250, _proc_timer, &ivaa);
    return 0;
}

VAAID vaa_get_vaaid(int ch)
{
    return (VAAID)ivaa.vaa[ch];
}

VAAID vaa_get_pb_vaaid(int ch)
{
    return (VAAID)ivaa.vaa[16+ch];
}

int vaa_is_supported()
{
    return ivaa.support;
}

int vaa_reload()
{
    int i;

    g_message("%s, %d", __FUNCTION__, __LINE__);
    for (i = 0; i < var_get_ch_count(); ++i) {
        if (ivaa.reload_proc) ivaa.reload_proc(ivaa.vaa[i]);
    }
}

DITID vaa_get_ditid(VAAID id)
{
    if (!ivaa.get_dit_proc) return -1;

    return ivaa.get_dit_proc(id);
}

int vaa_get_counted_rule(VAAID id, unsigned short countid)
{
    if (!ivaa.get_cntrid_proc) return -1;

    return ivaa.get_cntrid_proc(id, countid);
}

int vaa_parse_event(int ch, ivca_rule_event_t *pevt, int *zid, int *cid)
{
    if (!ivaa.parse_proc) return -1;

    return ivaa.parse_proc(ivaa.vaa[ch], pevt, zid, cid);
}

int vaa_activate_all_rule(VAAID id)
{
    if (!ivaa.activate_all_rule_proc) return -1;

    return ivaa.activate_all_rule_proc(id);
}

int vaa_deactivate_all_rule(VAAID id)
{
    if (!ivaa.deactivate_all_rule_proc) return -1;

    return ivaa.deactivate_all_rule_proc(id);
}

int vaa_activate_calb(VAAID id)
{
    if (!ivaa.activate_calb_proc) return -1;

    return ivaa.activate_calb_proc(id);
}

int vaa_deactivate_calb(VAAID id)
{
    if (!ivaa.deactivate_calb_proc) return -1;

    return ivaa.deactivate_calb_proc(id);
}

int vaa_activate_meta(VAAID id, VAA_META_E meta)
{
    if (!ivaa.activate_meta_proc) return -1;

    return ivaa.activate_meta_proc(id, meta);
}

int vaa_deactivate_meta(VAAID id, VAA_META_E meta)
{
    if (!ivaa.deactivate_meta_proc) return -1;

    return ivaa.deactivate_meta_proc(id, meta);
}

int vaa_blink_pattern(int ch, int patt)
{
    if (!ivaa.blink_proc) return -1;
    
    return ivaa.blink_proc(ivaa.vaa[ch], patt);
}

int vaa_notify_vca_event(NF_NOTIFY_INFO *data)
{
    if (!ivaa.notify_vca_event_proc) return -1;

    return ivaa.notify_vca_event_proc(data);
}

int vaa_notify_vca_track_info(NF_NOTIFY_INFO *data)
{
    if (!ivaa.notify_vca_track_info_proc) return -1;

    return ivaa.notify_vca_track_info_proc(data);
}

int vaa_notify_vca_counter_info(NF_NOTIFY_INFO *data)
{
    if (!ivaa.notify_vca_counter_info_proc) return -1;
    
    return ivaa.notify_vca_counter_info_proc(data);
}

int vaa_notify_vca_meta_data(NF_NOTIFY_INFO *data)
{
    if (!ivaa.notify_vca_meta_data_proc) return -1;
    
    return ivaa.notify_vca_meta_data_proc(data);
}

int vaa_notify_vca_analyze_event(NF_NOTIFY_INFO *data)
{
    if (!ivaa.notify_vca_analyze_event_proc) return -1;

    return ivaa.notify_vca_analyze_event_proc(data);
}

