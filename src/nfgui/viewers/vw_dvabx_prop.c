/*
 * vw_dvabx_prop.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nflistbox.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "dvaa.h"
#include "dvaa_itx.h"

#include "vw_menu.h"
#include "vw_dit_dva.h"
#include "vw_dvabx_prop.h"
#include "vw_dvabx_component.h"
#include "vw_ip_editor_popup.h"

#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"


#define DVABOX_SETTING_FIXED_W     (535 + 10)
#define DVABOX_SETTING_FIXED_H     (756 + 40 + 60 + 12)

#define DVABOX_SETTING_FIXED_X     (MENU_V_SUBTAB_INNER_W - VIDEO_COMPONENT_WIDTH + 10)
#define DVABOX_SETTING_FIXED_Y     (MENU_V_SUBTAB_INNER_Y)





////////////////////////////////////////////////////////////
//
// private data types
//

static AiAnalysisActData g_org_analysis_data[GUI_CHANNEL_CNT];
static AiAnalysisActData *g_analysis_data;



////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent_fixed = 0;
static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_engine_fixed;
static NFOBJECT *g_rcol_fixed;
static NFOBJECT *g_face_rcol_fixed;
static NFOBJECT *g_plateno_rcol_fixed;
static NFOBJECT *g_generic_rcol_fixed;
static NFOBJECT *g_testlog_obj;

static gint g_cur_channel = -1;
static guint g_draw_tid = 0;

static gint g_aibox_error_noti = 0;


////////////////////////////////////////////////////////////
//
// private interfaces 
//
static gint _set_default_option_component_data(DVA_COMPONENT_DATA_T *component_data)
{
    component_data->option.shadow_removal = 1;
    component_data->option.track_reference = 1;
    component_data->option.minimum_object = 0;
    component_data->option.minimum_w = 1000;
    component_data->option.minimum_h = 2000;
    component_data->option.disp_obj_box = 1;
    component_data->option.disp_obj_traj = 1;
    component_data->option.disp_obj_id = 1;
    component_data->option.disp_obj_w = 0;
    component_data->option.disp_obj_h = 0;
    component_data->option.disp_obj_speed = 0;
    component_data->option.disp_rules = 1;       
    
    return 0;
}

static gint _set_dvaa_prop_data(DVA_COMPONENT_DATA_T *component_data)
{
    DVAAID dvaaid;
    ITX_DVARULE_PROP prop;

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    
    dvaa_itx_detector_get_rule_prop(dvaaid, &prop);

    prop.unit = component_data->unit_setup;
    prop.en_shadowrm = component_data->option.shadow_removal;
    prop.track_ref = component_data->option.track_reference;
    prop.en_usecalib = component_data->option.minimum_object;
    prop.min_width3d = component_data->option.minimum_w;
    prop.min_height3d = component_data->option.minimum_h;
    prop.en_static_filter = component_data->option.static_filter;
    prop.static_filter_sense = component_data->option.static_filter_sense;
    prop.sw_obj_bb = component_data->option.disp_obj_box;
    prop.sw_obj_tr = component_data->option.disp_obj_traj;
    prop.sw_obj_id = component_data->option.disp_obj_id;
    prop.sw_obj_w3d = component_data->option.disp_obj_w;
    prop.sw_obj_h3d = component_data->option.disp_obj_h;
    prop.sw_obj_s3d = component_data->option.disp_obj_speed;
    prop.sw_rule = component_data->option.disp_rules;    
    
    dvaa_itx_detector_set_rule_prop(dvaaid, &prop);
    return 0;
}

static gint _option_component_cb(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    _set_dvaa_prop_data(component_data);

    return 0;
}

static gint _option_component_default_cb(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    _set_default_option_component_data(component_data);
    _set_dvaa_prop_data(component_data);
    vw_dvabx_video_component_sync_data(user_data);
    
    return 0;
}

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    DVA_COMPONENT_DATA_T *component_data;

    component_data = imalloc(sizeof(DVA_COMPONENT_DATA_T));

    component_data->act_capable = 0;
    component_data->act_license = 0;

    component_data->preview.ch = ch;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    
    component_data->disable_event = 1;

    component_data->unit_setup = 0;
    
    component_data->skip_calibration = 0;    
    component_data->calibration.select_icon = 0;    
    component_data->calibration.icon_height = 175;
    component_data->calibration.pause_video = 0;
    component_data->calibration.camera_height = 12.00;
    component_data->calibration.camera_tilt = 35;
    
    component_data->rule_type = -1;
   
    strcpy(component_data->line.name, "");
    component_data->line.active = 0;
    component_data->line.forward = 0;
    component_data->line.reverse = 0;
    component_data->line.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F);
    component_data->line.use_filter_color = 0;
    component_data->line.filter_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->line.use_filter_size = 0;
    component_data->line.filter_color_percnt = 1;
    component_data->line.filter_min_size_w = 0;
    component_data->line.filter_max_size_w = 0;
    component_data->line.filter_min_size_h = 0;
    component_data->line.filter_max_size_h = 0;
    component_data->line.use_filter_speed = 0;
    component_data->line.filter_min_speed = 0;
    component_data->line.filter_max_speed = 0;
    if (ivsc.vendor_code == 28) {
        component_data->line.c_threshold = 50;
    } else {
        component_data->line.c_threshold = 1;
    }

    strcpy(component_data->area.name, "");
    component_data->area.active = 0;
    component_data->area.enter = 0;
    component_data->area.exit = 0;
    component_data->area.removed = 0;
    component_data->area.loitering = 0;
    component_data->area.stopped = 0;    
    component_data->area.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F);
    component_data->area.stopped_val = 5;
    component_data->area.removed_val = 5;
    component_data->area.loitering_val = 5;
    component_data->area.use_filter_color = 0;
    component_data->area.filter_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->area.use_filter_size = 0;
    component_data->area.filter_color_percnt = 1;
    component_data->area.filter_min_size_w = 0;
    component_data->area.filter_max_size_w = 0;
    component_data->area.filter_min_size_h = 0;
    component_data->area.filter_max_size_h = 0;
    component_data->area.use_filter_speed = 0;
    component_data->area.filter_min_speed = 0;
    component_data->area.filter_max_speed = 0;
    if (ivsc.vendor_code == 28) {
        component_data->area.c_threshold = 50;
    } else {
        component_data->area.c_threshold = 1;
    }
    
    component_data->use_counter = 0;
    strcpy(component_data->counter.name, "");
    component_data->counter.active = 0;
    component_data->counter.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->counter.use_counter_event = 1;
    component_data->counter.counter_event_val = 100;
    component_data->counter.use_reset_value = 0;
    component_data->counter.up_source = -1;
    component_data->counter.down_source = -1;
 
    component_data->option.shadow_removal = 0;
    component_data->option.track_reference = 1;
    component_data->option.minimum_object = 0;
    component_data->option.minimum_w = 1000;
    component_data->option.minimum_h = 2000;
    component_data->option.disp_obj_box = 0;
    component_data->option.disp_obj_traj = 0;
    component_data->option.disp_obj_id = 1;
    component_data->option.disp_obj_w = 0;
    component_data->option.disp_obj_h = 0;
    component_data->option.disp_obj_speed = 0;
    component_data->option.disp_rules = 0;       
    
    nfui_nfobject_set_alloc_data(win, DVA_COMPONENT_DATA, component_data);
    return 0;
}


static gint _init_component_action(NFOBJECT *win, gint ch)
{
    DVA_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(DVA_COMPONENT_ACTION_T));
    
    component_action->option_cb = _option_component_cb;

    component_action->option_default_cb = _option_component_default_cb;
    
    nfui_nfobject_set_alloc_data(win, DVA_COMPONENT_ACTION, component_action);
    return 0;
}

static gint _get_calibration_cnt(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i, cnt = 0;
   
    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        if (conf.use_calb == 1) cnt++;
    }

    return cnt;
}

static gint _get_dvaa_calb_data(DVA_COMPONENT_DATA_T *component_data)
{
    DVAAID dvaaid;
    ITX_DVACALB_RESULT res;
    
    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    dvaa_itx_detector_get_calb_result(dvaaid, &res);

    component_data->calibration.icon_count = _get_calibration_cnt(dvaaid);
    component_data->calibration.icon_height = 175;
    component_data->calibration.pause_video = 0; 
    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;
    return 0;
}

static gint _get_dvaa_prop_data(DVA_COMPONENT_DATA_T *component_data)
{
    DVAAID dvaaid;
    ITX_DVARULE_PROP prop;
    
    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    dvaa_itx_detector_get_rule_prop(dvaaid, &prop);
    
    component_data->unit_setup = prop.unit;
    component_data->option.shadow_removal = prop.en_shadowrm;
    component_data->option.track_reference = prop.track_ref;
    component_data->option.minimum_object = prop.en_usecalib;
    component_data->option.minimum_w = prop.min_width3d;
    component_data->option.minimum_h = prop.min_height3d;
    component_data->option.static_filter = prop.en_static_filter;
    component_data->option.static_filter_sense = prop.static_filter_sense;
    component_data->option.disp_obj_box = prop.sw_obj_bb;
    component_data->option.disp_obj_traj = prop.sw_obj_tr;
    component_data->option.disp_obj_id = prop.sw_obj_id;
    component_data->option.disp_obj_w = prop.sw_obj_w3d;
    component_data->option.disp_obj_h = prop.sw_obj_h3d;
    component_data->option.disp_obj_speed = prop.sw_obj_s3d;
    component_data->option.disp_rules = prop.sw_rule;    

    return 0;
}

static gint _set_component_video_fixed(NFOBJECT *top, NFOBJECT *video_fixed)
{
    DVA_COMPONENT_DATA_T *component_data;

    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gboolean _draw_dva_rule(gpointer data)
{
    vw_dvabx_video_component_sync_data(g_video_fixed);
	return TRUE;
}

static gint _sync_aibox_face_zone_data(gint ch)
{

    return 0;
}

static gint _sync_aibox_plateno_zone_data(gint ch)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    AiAnalysisActData analysis_data;
    ITX_LPRZONE_CONF zone_conf;
    ITX_LPRZONE_SHAPE zone_shape;

    lpr_rule *aibox_rule = 0;
    gint rule_cnt = 0;
    gint i, j;

    top = nfui_nfobject_get_top(g_parent_fixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (!component_data->aibox_alive) return -1;

    memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
    DAL_get_aianalysis_act_data(&analysis_data, ch);
    if (!analysis_data.dvabox_active) return -1;

    dvaaid = dvaa_get_dvaaid(ch);

    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
        zone_conf.use_zone = 0;
        dvaa_itx_plateno_set_zone_conf(dvaaid, i, &zone_conf);
    }

    nf_api_get_lpr_rules(g_analysis_data[ch].dvabox_ipaddr, &aibox_rule, &rule_cnt);
    if (aibox_rule == 0) return -1;

    g_message("%s, %d, plateno_cnt:%d", __FUNCTION__, __LINE__, rule_cnt);

    for (i = 0; i < rule_cnt; i++)
    {
        dvaa_itx_plateno_add_zone_area_default_template(dvaaid, i);

        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
        zone_conf.triggerid = aibox_rule[i].trigger_id;
        dvaa_itx_plateno_set_zone_conf(dvaaid, i, &zone_conf);

        dvaa_itx_plateno_get_zone_shape(dvaaid, i, &zone_shape);
        snprintf(zone_shape.name, sizeof(zone_shape.name)-1, "%s", aibox_rule[i].name);
        zone_shape.ptcnt = aibox_rule[i].zone_size;
        for (j = 0; j < aibox_rule[i].zone_size; j++)
        {
            zone_shape.pt[j].x = (gint)(aibox_rule[i].zone[j].x * 3840.0);
            zone_shape.pt[j].y = (gint)(aibox_rule[i].zone[j].y * 2160.0);
        }
        dvaa_itx_plateno_set_zone_shape(dvaaid, i, &zone_shape); 
    }

    if (aibox_rule) free(aibox_rule);
    aibox_rule = 0;

    return 0;
}


static gint _sync_aibox_zone_data(gint ch)
{
    _sync_aibox_face_zone_data(ch);
    _sync_aibox_plateno_zone_data(ch);
    return 0;
}

static gint _is_changed_data()
{
	DVAAID dvaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		dvaaid = dvaa_get_dvaaid(i);
		if (dvaa_itx_is_db_changed(dvaaid)) return 1;
	}

    if (memcmp(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT) != 0) return 1;

	return 0;
}

static gint _save_data()
{
	guint db_changed = 0;
	DVAAID dvaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{	
        if (memcmp(&g_org_analysis_data[i], &g_analysis_data[i], sizeof(AiAnalysisActData)) != 0)
        {
            g_memmove(&g_org_analysis_data[i], &g_analysis_data[i], sizeof(AiAnalysisActData));
			db_changed |= (1 << i);
        }

		dvaaid = dvaa_get_dvaaid(i);
		if (dvaa_itx_is_db_changed(dvaaid)) {
			dvaa_itx_save_db(dvaaid);
			db_changed |= (1 << i);
		}
	}

	if (db_changed) {
		DAL_notify_fire_DB_change2(NF_SYSDB_CATE_CAM, db_changed, SYSDB_SUB_CATE_CAM_AI);
//		evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);
	}
	
	return db_changed;
}

static gint _restore_aibox_algorithm()
{
    gint i;
    gint ret_code = DLVA_API_RET_OK;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if ((g_analysis_data[i].dvabox_active) && (strcmp(g_org_analysis_data[i].algorithm, g_analysis_data[i].algorithm) != 0))
        {
            ret_code = nf_api_aibox_set(i, g_org_analysis_data[i].dvabox_ipaddr, g_org_analysis_data[i].algorithm);
            g_message("%s, %d, ch:%d, ipaddr:%u, algorithm:%s, ret_code:%d", __FUNCTION__, __LINE__, i, g_org_analysis_data[i].dvabox_ipaddr, g_org_analysis_data[i].algorithm, ret_code);
        }
    }

    return 0;
}

static gint _restore_aicam_algorithm(gint ch)
{
    gint ret_code = DLVA_API_RET_OK;

    if ((g_analysis_data[ch].dvacam_active) && (strcmp(g_org_analysis_data[ch].algorithm, g_analysis_data[ch].algorithm) != 0))
    {
        ret_code = nf_api_aibox_set(ch, 0, g_org_analysis_data[ch].algorithm);
        g_message("%s, %d, ch:%d, algorithm:%s, ret_code:%d", __FUNCTION__, __LINE__, ch, g_org_analysis_data[ch].algorithm, ret_code);
    }

    return 0;
}

static gint _load_data()
{
	if (_is_changed_data() == 0) return -1;

	dvaa_reload();
    g_memmove(g_analysis_data, g_org_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
    
	return 0;
}






////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_video_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{	

	}
	else if (evt->type == GDK_DELETE)  
	{	
        dvaa_set_auto_update_external_rules(1);
        dvaa_update_external_airules();

    	if (g_draw_tid) 
    	{
    		g_source_remove(g_draw_tid);
    		g_draw_tid = 0;
    	}
	}

	return FALSE;
}

static gboolean post_operation_test_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == INFY_AI_EVENT_NOTIFY)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;

        ITX_DVAZONE_SHAPE zone_shape;
        ITX_DVACNTR_SHAPE cntr_shape;

        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
    	ai_rule_event_t *pevt;
    	gint *p;

        gchar *log_list[2];
        gchar strBuf[256];
        gint i;

    	p = pInfo->p.ptr;
    	pevt = p + 2;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    	
    	if (p[0] >= 32) return FALSE;
    	if (p[0] != component_data->preview.ch) return FALSE;

		for(i = 0; i < p[1]; i++, pevt++) 
		{
            if (nfui_listbox_get_box_count((NFLISTBOX*)obj) >= 16)
            {
                nfui_listbox_delete((NFLISTBOX*)obj, 15);
            }

            dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

            memset(strBuf, 0x00, sizeof(strBuf));        
            dtf_get_local_datetime(pevt->timestamp, strBuf);
            log_list[0] = g_strdup(strBuf);

        	if (pevt->type & IVCA_ET_COUNTER) 
        	{
                memset(strBuf, 0x00, sizeof(strBuf));
                dvaa_itx_detector_get_cntr_shape(dvaaid, pevt->rule_id, &cntr_shape);
                snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s : %s", cntr_shape.name, lookup_string("EVENT"), lookup_string("VCA-COUNTER"));
                log_list[1] = g_strdup(strBuf);
        	}
        	else
        	{
                memset(strBuf, 0x00, sizeof(strBuf));
                dvaa_itx_detector_get_zone_shape(dvaaid, pevt->rule_id, &zone_shape);

                if (pevt->type & IVCA_ET_DIR_POS)         
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-FORWARD"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_DIR_NEG)    
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-REVERSE"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_INTRUSION)  
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("INTRUSION"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_ENTER)      
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-ENTER"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_EXIT)       
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-EXIT"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_STOPPED)    
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-STOPPED"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_REMOVED)    
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-REMOVED"), lookup_string("OBJECT"), lookup_string(pevt->object_class));
                else if (pevt->type & IVCA_ET_LOITERED)   
                    snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s:%s, %s:%s", zone_shape.name, lookup_string("EVENT"), lookup_string("VCA-LOITERING"), lookup_string("OBJECT"), lookup_string(pevt->object_class));

                log_list[1] = g_strdup(strBuf);
        	}

            nfui_listbox_set_prepend_text((NFLISTBOX*)obj, log_list);
        
            ifree(log_list[0]);
            ifree(log_list[1]);
		}

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_AI_GENERIC_EVENT_NOTIFY)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
    	ai_generic_event_t *pevt;
    	gint *p;

        gchar *log_list[2];
        gchar strBuf[256];
        gint i;

    	p = pInfo->p.ptr;
    	pevt = p + 2;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (p[0] != component_data->preview.ch) return FALSE;

		for(i = 0; i < p[1]; i++, pevt++) 
		{
            if (nfui_listbox_get_box_count((NFLISTBOX*)obj) >= 16)
            {
                nfui_listbox_delete((NFLISTBOX*)obj, 15);
            }

            memset(strBuf, 0x00, sizeof(strBuf));        
            dtf_get_local_datetime(pevt->timestamp, strBuf);
            log_list[0] = g_strdup(strBuf);

            memset(strBuf, 0x00, sizeof(strBuf));
            snprintf(strBuf, sizeof(strBuf)-1, "  %s, %s", pevt->caption, pevt->title);
            log_list[1] = g_strdup(strBuf);

            nfui_listbox_set_prepend_text((NFLISTBOX*)obj, log_list);
        
            ifree(log_list[0]);
            ifree(log_list[1]);
		}

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_AI_EVENT_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_AI_GENERIC_EVENT_NOTIFY);
    }

    return FALSE;
}




/*
 :  dvabx_prop main_fixed.

      --------------------------------------------
      |  ---------------------------------------  |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |        FIXED1         |     FIXED2    | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      |  ---------------------------------------  |
      |                                           |
      |             PARENT                        |
      |-------------------------------------------|  
*/


////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _analysis_dvabx_prop_init_page(NFOBJECT *parent)
{
    NFOBJECT *fixed = parent;
    NFOBJECT *video_fixed;    
    NFOBJECT *tmp_fixed;
    NFOBJECT *obj;
           
    gint pos_x, pos_y;
	guint i = 0, j = 0;

	DVAAID dvaaid;
	DITID dit;
    DVA_COMPONENT_DATA_T *component_data;	
    ITX_DVARULE_PROP prop;	
    NFOBJECT *top;

    guint tbl_livelog_w[2] = {250, 650};

    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];
    time_t s_time;


    s_time = time(0);
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        if (scm_get_ipcam_ai_type(i) != CAM_AI_TYPE_CLAIR2) continue;
        nf_ipcam_get_embedded_osd(i);
        dvaa_sync_aicam(i);
    }
    g_message("[%s, %d] Synchronization time : %d", __FUNCTION__, __LINE__, time(0) - s_time);
    dvaa_set_auto_update_external_rules(0);


    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    _init_component_data(nfui_nfobject_get_top(parent), 0);
    _init_component_action(nfui_nfobject_get_top(parent), 0);


    pos_x = 0;
    pos_y = 0;

    video_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(video_fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_nfobject_show(video_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, video_fixed, 0, 42);
    nfui_regi_post_event_callback(video_fixed, post_video_fixed_event_handler);
    g_video_fixed = video_fixed;

    vw_dvabx_video_component_open(video_fixed, 0);
    _set_component_video_fixed(nfui_nfobject_get_top(parent), video_fixed);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OPERATION TEST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_set_size(obj, 600, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 42+VIDEO_COMPONENT_HEIGHT+30);

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);
    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  

    obj = nfui_listbox_new(2, tbl_livelog_w, 36);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_SUBTAB_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);   
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 0, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 1, NFALIGN_LEFT);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, 926, 150);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 42+VIDEO_COMPONENT_HEIGHT+80);  
    nfui_regi_post_event_callback(obj, post_operation_test_list_event_cb);  
    g_testlog_obj = obj;

    uxm_reg_imsg_event(obj, INFY_AI_EVENT_NOTIFY);
    uxm_monitor_on_imsg_event(obj, INFY_AI_EVENT_NOTIFY);
    uxm_reg_imsg_event(obj, INFY_AI_GENERIC_EVENT_NOTIFY);
    uxm_monitor_on_imsg_event(obj, INFY_AI_GENERIC_EVENT_NOTIFY);

    pos_x = VIDEO_COMPONENT_WIDTH + 28;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DVABOX_SETTING_FIXED_W, parent->height-2);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
    g_engine_fixed = fixed;

    _dvabx_engine_page(g_engine_fixed);   

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DVABOX_SETTING_FIXED_W, parent->height-2);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
    g_rcol_fixed = fixed;

    _dvabx_rcol_page(g_rcol_fixed);

    if (ivsc.dfunc.support_face)
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DVABOX_SETTING_FIXED_W, parent->height-2);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_hide(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
        g_face_rcol_fixed = fixed;

        _dvabx_face_rcol_page(g_face_rcol_fixed);
    }

    if (ivsc.dfunc.support_license_plate) 
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DVABOX_SETTING_FIXED_W, parent->height-2);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_hide(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
        g_plateno_rcol_fixed = fixed;

        _dvabx_plateno_rcol_page(g_plateno_rcol_fixed);        
    }

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DVABOX_SETTING_FIXED_W, parent->height-2);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
    g_generic_rcol_fixed = fixed;

    _dvabx_generic_rcol_page(g_generic_rcol_fixed);

    g_parent_fixed = parent;
	
    return 0;
}

gint _analysis_dvabx_prop_show_page(gint ch, AiAnalysisActData *analysis_data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    if (g_cur_channel != -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    nfui_listbox_delete_all(NF_LISTBOX(g_testlog_obj));
    nfui_signal_emit(g_testlog_obj, GDK_EXPOSE, TRUE);

    g_cur_channel = ch;
    g_analysis_data = analysis_data;
    memset(g_org_analysis_data, 0x00, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_analysis_data, g_analysis_data, sizeof(AiAnalysisActData)*GUI_CHANNEL_CNT);   

    dvaaid = dvaa_get_dvaaid(ch);
    dvaa_itx_raiseup(dvaaid);

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    component_data->preview.ch = ch;
    component_data->preview.onoff = 1;
    component_data->preview.calb_onoff = 0;
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    vw_dvabx_video_component_sync_preview(g_video_fixed);

    if (analysis_data[ch].dvabox_active) 
    {
        component_data->aibox_alive = nf_api_aibox_server_state_is_alive(analysis_data[ch].dvabox_ipaddr, 3);
        if (component_data->aibox_alive == 0) g_aibox_error_noti = 1;

        dvaa_itx_active_external_rule(dvaaid);
    }
    else if (g_analysis_data[ch].dvacam_active) 
    {
        if (scm_get_ipcam_ai_type(component_data->preview.ch) == CAM_AI_TYPE_PRO) 
            dvaa_itx_active_external_rule(dvaaid);
        else 
            dvaa_itx_inactive_external_rule(dvaaid);
    }

    //_sync_aibox_zone_data(ch);
    _dvabx_engine_init_data(ch, analysis_data, g_org_analysis_data);

    _get_dvaa_calb_data(component_data);       
    _get_dvaa_prop_data(component_data);
    nfui_user_signal_emit(g_parent_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);      

    _dvabx_rcol_hide_page(0);
    if (ivsc.dfunc.support_face) _dvabx_face_rcol_hide_page(0);
    if (ivsc.dfunc.support_license_plate) _dvabx_plateno_rcol_hide_page(0);
    _dvabx_engine_show_page(1);

    if (!g_draw_tid) g_draw_tid = g_timeout_add(100, _draw_dva_rule, 0);
    
    return 0;
}

gint _post_analysis_dvabx_prop_show_page()
{
    if (g_aibox_error_noti) {
        nftool_mbox(g_curwnd, "ERROR", "AI Box connection failed.\nTry SCAN again and reset the connection to the AI Box.\nIf the AI Box does not appear in the list, check the AI Box status.", NFTOOL_MB_OK);
        g_aibox_error_noti = 0;
    }
    return 0;
}

gint _analysis_dvabx_prop_hide_page(gint ch)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;

    if (g_cur_channel == -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (g_draw_tid) {
        g_source_remove(g_draw_tid);
        g_draw_tid = 0;
    }

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_dvabx_video_component_sync_preview(g_video_fixed);
    vw_dvabx_video_component_expose(g_video_fixed);

    component_data->calibration.pause_video = 0;
    nfui_user_signal_emit(g_engine_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    nfui_user_signal_emit(g_rcol_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    if (ivsc.dfunc.support_face) nfui_user_signal_emit(g_face_rcol_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    if (ivsc.dfunc.support_license_plate) nfui_user_signal_emit(g_plateno_rcol_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    nfui_user_signal_emit(g_generic_rcol_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);

    g_cur_channel = -1;
    return 0;
}

gint _analysis_dvabx_prop_load_changed_data(gint ch)
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    return 0;
}

gint _analysis_dvabx_prop_is_changed_data()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (_is_changed_data() == 1)
    {
        g_message("%s, %d, changed:1", __FUNCTION__, __LINE__);
        return 1;
    }

    g_message("%s, %d, changed:0", __FUNCTION__, __LINE__);
    return 0;
}

gint _analysis_dvabx_prop_save_data()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    _dvabx_rcol_estimation_calb();
    _save_data();
    return 0;
}

gint _analysis_dvabx_prop_cancel_data(gint ch, gint expose)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;    
    DVAAID dvaaid;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (g_analysis_data[ch].dvabox_active) _restore_aibox_algorithm();
    if (g_analysis_data[ch].dvacam_active) _restore_aicam_algorithm(ch);

    _load_data();

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    component_data->preview.calb_onoff = 0; 
    vw_dvabx_video_component_sync_preview(g_video_fixed);
    vw_dvabx_video_component_expose(g_video_fixed);        

    dvaaid = dvaa_get_dvaaid(ch); 

    _dvabx_engine_init_data(ch, g_analysis_data, g_org_analysis_data);

    if (ivsc.dfunc.ai.support_calibration) _dvabx_calb_data_cancel(dvaaid, component_data);
    _get_dvaa_calb_data(component_data);       
    _get_dvaa_prop_data(component_data);
    
    nfui_user_signal_emit(g_parent_fixed, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);

    return 0;
}
