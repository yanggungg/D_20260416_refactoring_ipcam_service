/*
 * dvaa.c
 *  - deeplearning video analytics agent
 *  - dependencies :
 *      
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include "dvaa.h"
#include "dvaa_itx.h"
#include "dvaa_itx_internal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "var.h"
#include "wrk.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "DVAA"



////////////////////////////////////////////////////////////
//
// private data type
//

typedef void*    DVAAPTR;

typedef int (*DVAA_VENDOR_INIT)(int, DVAAID *, DITID *);
typedef int (*DVAA_VENDOR_PB_INIT)(int, DVAAID *, DITID *);

typedef DITID (*DVAA_VENDOR_GET_DITID)(DVAAID);
typedef int (*DVAA_VENDOR_GET_CNTRID)(DVAAID, unsigned short);

typedef int (*DVAA_VENDOR_ACTIVATE_ALL_RULE)(DVAAID);
typedef int (*DVAA_VENDOR_DEACTIVATE_ALL_RULE)(DVAAID);
typedef int (*DVAA_VENDOR_ACTIVATE_CALB)(DVAAID);
typedef int (*DVAA_VENDOR_DEACTIVATE_CALB)(DVAAID);
typedef int (*DVAA_VENDOR_ACTIVATE_META)(DVAAID, int);
typedef int (*DVAA_VENDOR_DEACTIVATE_META)(DVAAID, int);

typedef int (*DVAA_VENDOR_RELOAD)(DVAAID);
typedef int (*DVAA_VENDOR_PARSE)(DVAAID, ai_rule_event_t *, int *, int *);
typedef int (*DVAA_VENDOR_TIMER)(DVAAID);
typedef int (*DVAA_VENDOR_BLINK)(DVAAID, int);
typedef int (*DVAA_VENDOR_EXTERNAL_RULES)(DVAAID, int, aibox_rule_data *);

typedef int (*DVAA_VENDOR_NOTI_EVENT)(NF_NOTIFY_INFO *);
typedef int (*DVAA_VENDOR_NOTI_GENERIC_EVENT)(NF_NOTIFY_INFO *);
typedef int (*DVAA_VENDOR_NOTI_TRACK_INFO)(NF_NOTIFY_INFO *);
typedef int (*DVAA_VENDOR_NOTI_COUNTER_INFO)(NF_NOTIFY_INFO *);
typedef int (*DVAA_VENDOR_NOTI_META_DATA)(NF_NOTIFY_INFO *);
typedef int (*DVAA_VENDOR_NOTI_ANALYZE_EVENT)(NF_NOTIFY_INFO *);


typedef struct _DVAA_T {
    int                             support;
    DVAA_VENDOR_INIT                 init_proc;
    DVAA_VENDOR_PB_INIT              init_pb_proc;
    
    DVAA_VENDOR_GET_DITID            get_dit_proc;
    DVAA_VENDOR_GET_CNTRID           get_cntrid_proc;
    
    DVAA_VENDOR_ACTIVATE_ALL_RULE    activate_all_rule_proc;
    DVAA_VENDOR_DEACTIVATE_ALL_RULE  deactivate_all_rule_proc;    
    DVAA_VENDOR_ACTIVATE_CALB        activate_calb_proc;    
    DVAA_VENDOR_DEACTIVATE_CALB      deactivate_calb_proc;    
    DVAA_VENDOR_ACTIVATE_META        activate_meta_proc;    
    DVAA_VENDOR_DEACTIVATE_META      deactivate_meta_proc;    
    
    DVAA_VENDOR_RELOAD               reload_proc;
    DVAA_VENDOR_PARSE                parse_proc;
    DVAA_VENDOR_TIMER                timer_proc;
    DVAA_VENDOR_BLINK                blink_proc;
    DVAA_VENDOR_EXTERNAL_RULES       all_external_rules_proc;
    DVAA_VENDOR_EXTERNAL_RULES       fr_external_rules_proc;
    DVAA_VENDOR_EXTERNAL_RULES       lpr_external_rules_proc;
    
    DVAA_VENDOR_NOTI_EVENT           notify_event_proc;
    DVAA_VENDOR_NOTI_GENERIC_EVENT   notify_generic_event_proc;
    DVAA_VENDOR_NOTI_TRACK_INFO      notify_track_info_proc;
    DVAA_VENDOR_NOTI_COUNTER_INFO    notify_counter_info_proc;
    DVAA_VENDOR_NOTI_META_DATA       notify_meta_data_proc;
    DVAA_VENDOR_NOTI_ANALYZE_EVENT   notify_analyze_event_proc;

    DVAAID              dvaa[64];
    DITID               dit[64];

    int                 auto_update_external_rules;
    WRK_ID              iwrk_external_airules; 
} DVAA_T;

////////////////////////////////////////////////////////////
//
// private variables 
//

static DVAA_T idvaa;



////////////////////////////////////////////////////////////
//
// private functions
//

static gboolean _proc_timer(gpointer data)
{
    int i;
    DVAA_T *pidvaa = (DVAA_T *)data;

    for (i = 0; i < var_get_ch_count(); ++i) {
        pidvaa->timer_proc(pidvaa->dvaa[i]);
    }

    return TRUE;
}

static int _get_algorithm_type(int ch, char *algo_type)
{
    AiAnalysisActData analysis_data;
    ai_capa_t *algorithm_capa;
    char strBuf[256];
    int i;

    memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
    DAL_get_aianalysis_act_data(&analysis_data, ch);
    if (analysis_data.dvabox_active) 
    {
        algorithm_capa = nf_api_get_capability(analysis_data.dvabox_ipaddr, ch);
        if (!algorithm_capa) return -1;

        memset(strBuf, 0x00, sizeof(strBuf));
        nf_api_selected_aibox_algorithm(ch, analysis_data.dvabox_ipaddr, strBuf);

        for (i = 0; i < algorithm_capa->algorithm_count; i++) 
        {
            if (strcmp(algorithm_capa->algorithm_list[i].value, strBuf) == 0)
            {
                g_message("%s, %d, ch:%d, type:%s", __FUNCTION__, __LINE__, ch, algorithm_capa->algorithm_list[i].algo_type);
                snprintf(algo_type, 63, "%s", algorithm_capa->algorithm_list[i].algo_type);
            }
        }
        nf_api_capability_free(algorithm_capa);
    }
    else if (analysis_data.dvacam_active)
    {
        algorithm_capa = nf_api_get_capability(0, ch);
        if (!algorithm_capa) return -1;

        memset(strBuf, 0x00, sizeof(strBuf));
        nf_api_selected_aicamera_algorithm(ch, strBuf);

        for (i = 0; i < algorithm_capa->algorithm_count; i++) 
        {
            if (strcmp(algorithm_capa->algorithm_list[i].value, strBuf) == 0)
            {
                g_message("%s, %d, ch:%d, type:%s", __FUNCTION__, __LINE__, ch, algorithm_capa->algorithm_list[i].algo_type);
                snprintf(algo_type, 63, "%s", algorithm_capa->algorithm_list[i].algo_type);
            }
        }
        nf_api_capability_free(algorithm_capa);
    }
    return 0;
}

static int _proc_get_external_airules(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
	int ch;
    aibox_rule_data *rules = 0;
    int rule_size = 0;
    void *p;
    int i, idx;
    int type;
    int cnt[64] = {0, };

    ch = pmsg->param;
    
    nf_api_get_aibox_rules(ch, &rules, &rule_size);
#ifdef DEBUG_VA_MESSAGE    
    g_message("%s, %d, ch:%d, rule_size:%d", __FUNCTION__, __LINE__, ch, rule_size);
#endif

    p = imalloc(sizeof(int)*2+sizeof(aibox_rule_data)*rule_size);
    ((int*)p)[0] = -1;
    ((int*)p)[1] = rule_size;
    if (rule_size) memcpy((int*)p+2, (void*)rules, sizeof(aibox_rule_data)*rule_size);
    evt_send_to_local(INFY_EXTERNAL_ANALYTICS_RULES, ch, 1, p);    

#if 0
    for (i = 0; i < rule_size; i++)
    {
        type = rules[i].type;
        if (type >= 32) g_assert(0);
        cnt[type] += 1;
    }

    idx = 0;

    p = imalloc(sizeof(int)*2+sizeof(aibox_rule_data)*cnt[RULE_TYPE_LPR]);
    ((int*)p)[0] = RULE_TYPE_LPR;
    ((int*)p)[1] = cnt[RULE_TYPE_LPR];
    for (i = 0; i < rule_size; i++)
    {
        if (rules[i].type == RULE_TYPE_LPR) {
            memcpy((int*)p+2+sizeof(aibox_rule_data)*idx, &rules[i], sizeof(aibox_rule_data));
            idx++;
        }
    }    
    evt_send_to_local(INFY_EXTERNAL_ANALYTICS_RULES, ch, 1, p);    

    idx = 0;

    p = imalloc(sizeof(int)*2+sizeof(aibox_rule_data)*cnt[RULE_TYPE_FR]);
    ((int*)p)[0] = RULE_TYPE_FR;
    ((int*)p)[1] = cnt[RULE_TYPE_FR];
    for (i = 0; i < rule_size; i++)
    {
        if (rules[i].type == RULE_TYPE_FR) {
            memcpy((int*)p+2+sizeof(aibox_rule_data)*idx, &rules[i], sizeof(aibox_rule_data));
            idx++;
        }
    }    
    evt_send_to_local(INFY_EXTERNAL_ANALYTICS_RULES, ch, 1, p);
#endif
    
    if (rules) free(rules);
    rules = 0;
	return 0;
}

static int _init_worker(DVAA_T *pidvaa)
{
    if (pidvaa->iwrk_external_airules) return -1;

	pidvaa->iwrk_external_airules = wrk_create_worker(_proc_get_external_airules, 0);
	wrk_change_sleep_time(pidvaa->iwrk_external_airules, 20000);

	return 0;
}

static int _req_external_airules(DVAA_T *pidvaa, int ch)
{
    gchar algo_type[64];

    memset(algo_type, 0x00, sizeof(algo_type));
    _get_algorithm_type(ch, algo_type);

    if (strcmp(algo_type, "mot") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_DETECTOR);
    else if (strcmp(algo_type, "fr") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_FACE);
    else if (strcmp(algo_type, "lpr") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_PLATENO);

	dvaa_activate_all_rule(idvaa.dvaa[ch]);
	dvaa_activate_meta(idvaa.dvaa[ch], DVAA_META_BBOX);
	dvaa_deactivate_calb(idvaa.dvaa[ch]);

    if (!pidvaa->iwrk_external_airules) return -1;

	wrk_run_msg(pidvaa->iwrk_external_airules, IMSG_NONE, ch, 0, 0);
	return 0;
}

static int _req_external_airules_all(DVAA_T *pidvaa)
{
    aibox_algorithm_name *palgo = NULL;
    gint ch;

    palgo = nf_api_get_algorithm_data();
    if (!palgo) return -1;
    
    if (!pidvaa->iwrk_external_airules) {
        free(palgo);
        return -1;
    }

    for (ch = 0; ch < var_get_ch_count(); ch++)
    {
        if (strcmp(palgo[ch].algo_type, "mot") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_DETECTOR);
        else if (strcmp(palgo[ch].algo_type, "fr") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_FACE);
        else if (strcmp(palgo[ch].algo_type, "lpr") == 0) dvaa_itx_active_algotithm(idvaa.dvaa[ch], ITX_ALGOTYPE_PLATENO);

    	dvaa_activate_all_rule(idvaa.dvaa[ch]);
    	dvaa_activate_meta(idvaa.dvaa[ch], DVAA_META_BBOX);
    	dvaa_deactivate_calb(idvaa.dvaa[ch]);

    	wrk_run_msg(pidvaa->iwrk_external_airules, IMSG_NONE, ch, 0, 0);
	}

	free(palgo);
	
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int dvaa_init()
{
    int i;
    int vendor_code = var_get_vendor_code();
    memset(&idvaa, 0x00, sizeof(DVAA_T));

    //if (!var_get_supported_dvabx()) return 0;

	idvaa.init_proc = dvaa_itx_init;
	idvaa.init_pb_proc = dvaa_itx_pb_init;
	idvaa.get_dit_proc = dvaa_itx_get_dit;
	idvaa.get_cntrid_proc = dvaa_itx_get_counted_rule;        
	idvaa.activate_all_rule_proc = dvaa_itx_activate_all_rule;        
	idvaa.deactivate_all_rule_proc = dvaa_itx_deactivate_all_rule;        
	idvaa.activate_calb_proc = dvaa_itx_activate_calb;        
	idvaa.deactivate_calb_proc = dvaa_itx_deactivate_calb;
	idvaa.activate_meta_proc = dvaa_itx_activate_meta;        
	idvaa.deactivate_meta_proc = dvaa_itx_deactivate_meta;                
	idvaa.reload_proc = dvaa_itx_reload;
	idvaa.parse_proc = dvaa_itx_parse_event;
	idvaa.timer_proc = dvaa_itx_timer_proc;
	idvaa.blink_proc = 0;
    idvaa.all_external_rules_proc = dvaa_itx_all_external_rules_proc;
    idvaa.lpr_external_rules_proc = dvaa_itx_lpr_external_rules_proc;
    //idvaa.fr_external_rules_proc = dvaa_itx_fr_external_rules_proc;
	idvaa.notify_event_proc = dvaa_itx_notify_event;
    idvaa.notify_generic_event_proc = dvaa_itx_notify_generic_event;
	idvaa.notify_track_info_proc = dvaa_itx_notify_track_info;
	idvaa.notify_counter_info_proc = dvaa_itx_notify_counter_info;
	idvaa.notify_meta_data_proc = dvaa_itx_notify_meta_data;        
	idvaa.notify_analyze_event_proc = dvaa_itx_notify_analyze_event;

    idvaa.support = 1;
    idvaa.auto_update_external_rules = 1;

// LIVE    
    for (i = 0; i < var_get_ch_count(); ++i) {
        if (idvaa.init_proc) idvaa.init_proc(i, &idvaa.dvaa[i], &idvaa.dit[i]);
    }

// PLAYBACK
	for (i = 32; i < 32+var_get_ch_count(); ++i) {
		if (idvaa.init_pb_proc) idvaa.init_pb_proc(i, &idvaa.dvaa[i], &idvaa.dit[i]);
	}

    g_timeout_add(250, _proc_timer, &idvaa);

    return 0;
}

DVAAID dvaa_get_dvaaid(int ch)
{
    return (DVAAID)idvaa.dvaa[ch];
}

DVAAID dvaa_get_pb_dvaaid(int ch)
{
    return (DVAAID)idvaa.dvaa[32+ch];
}

int dvaa_is_supported()
{
    return idvaa.support;
}

int dvaa_set_auto_update_external_rules(int auto_update)
{
    idvaa.auto_update_external_rules = auto_update;
    return 0;
}

int dvaa_update_external_airules()
{
    int i;

    _init_worker(&idvaa);

/*
    for (i = 0; i < var_get_ch_count(); ++i) {
        _req_external_airules(&idvaa, i);
    }
*/
    _req_external_airules_all(&idvaa);
    
    return 0;
}

int dvaa_reload()
{
    int i;
    g_message("%s, %d", __FUNCTION__, __LINE__);
    for (i = 0; i < var_get_ch_count(); ++i) {
        if (idvaa.reload_proc) idvaa.reload_proc(idvaa.dvaa[i]);
    }
    return 0;
}

int dvaa_sync_aicam(int ch)
{
    nf_ipcam_get_ai_rule_engine(ch);
    if (idvaa.reload_proc) idvaa.reload_proc(idvaa.dvaa[ch]);
    return 0;
}

DITID dvaa_get_ditid(DVAAID id)
{
    if (!idvaa.get_dit_proc) return -1;

    return idvaa.get_dit_proc(id);
}

int dvaa_get_counted_rule(DVAAID id, unsigned short countid)
{
    if (!idvaa.get_cntrid_proc) return -1;

    return idvaa.get_cntrid_proc(id, countid);
}

int dvaa_parse_event(int ch, ai_rule_event_t *pevt, int *zid, int *cid)
{
    if (!idvaa.parse_proc) return -1;

    return idvaa.parse_proc(idvaa.dvaa[ch], pevt, zid, cid);
}

int dvaa_external_rules(int ch, int *p)
{
    if (p[0] == -1) {
        if (!idvaa.all_external_rules_proc) return -1;

        if (p[1] > 0) idvaa.all_external_rules_proc(idvaa.dvaa[ch], p[1], p+2);
        else idvaa.all_external_rules_proc(idvaa.dvaa[ch], p[1], NULL);
    }
    else if (p[0] == RULE_TYPE_LPR) {
        if (!idvaa.auto_update_external_rules) return -1;
        if (!idvaa.lpr_external_rules_proc) return -1;

        if (p[1] > 0) idvaa.lpr_external_rules_proc(idvaa.dvaa[ch], p[1], p+2);
        else idvaa.lpr_external_rules_proc(idvaa.dvaa[ch], p[1], NULL);
    }
    else if (p[0] == RULE_TYPE_FR) {
        if (!idvaa.auto_update_external_rules) return -1;        
        if (!idvaa.fr_external_rules_proc) return -1;

        if (p[1] > 0) idvaa.fr_external_rules_proc(idvaa.dvaa[ch], p[1], p+2);
        else idvaa.fr_external_rules_proc(idvaa.dvaa[ch], p[1], NULL);
    }
    return 0;
}

int dvaa_activate_all_rule(DVAAID id)
{
    if (!idvaa.activate_all_rule_proc) return -1;

    return idvaa.activate_all_rule_proc(id);
}

int dvaa_deactivate_all_rule(DVAAID id)
{
    if (!idvaa.deactivate_all_rule_proc) return -1;

    return idvaa.deactivate_all_rule_proc(id);
}

int dvaa_activate_calb(DVAAID id)
{
    if (!idvaa.activate_calb_proc) return -1;

    return idvaa.activate_calb_proc(id);
}

int dvaa_deactivate_calb(DVAAID id)
{
    if (!idvaa.deactivate_calb_proc) return -1;

    return idvaa.deactivate_calb_proc(id);
}

int dvaa_activate_meta(DVAAID id, DVAA_META_E meta)
{
    if (!idvaa.activate_meta_proc) return -1;

    return idvaa.activate_meta_proc(id, meta);
}

int dvaa_deactivate_meta(DVAAID id, DVAA_META_E meta)
{
    if (!idvaa.deactivate_meta_proc) return -1;

    return idvaa.deactivate_meta_proc(id, meta);
}

int dvaa_blink_pattern(int ch, int patt)
{
    if (!idvaa.blink_proc) return -1;
    
    return idvaa.blink_proc(idvaa.dvaa[ch], patt);
}

int dvaa_notify_event(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_event_proc) return -1;

    return idvaa.notify_event_proc(data);
}

int dvaa_notify_generic_event(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_generic_event_proc) return -1;

    return idvaa.notify_generic_event_proc(data);
}

int dvaa_notify_track_info(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_track_info_proc) return -1;

    return idvaa.notify_track_info_proc(data);
}

int dvaa_notify_counter_info(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_counter_info_proc) return -1;
    
    return idvaa.notify_counter_info_proc(data);
}

int dvaa_notify_meta_data(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_meta_data_proc) return -1;
    
    return idvaa.notify_meta_data_proc(data);
}

int dvaa_notify_analyze_event(NF_NOTIFY_INFO *data)
{
    if (!idvaa.notify_analyze_event_proc) return -1;

    return idvaa.notify_analyze_event_proc(data);
}
