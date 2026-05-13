/*
 * vw_vca_rev_prop.c
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
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
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vaa.h"
#include "vaa_itx.h"

#include "vw_menu.h"
#include "vw_dit_vca.h"
#include "vw_vca_rev_prop.h"
#include "vw_vca_rev_component.h"

#include "nf_api_ipcam.h"


#define VCA_REV_SETTING_FIXED_W     (535 + 10)
#define VCA_REV_SETTING_FIXED_H     (756 + 40 + 60 + 12)

#define VCA_REV_SETTING_FIXED_X     (MENU_V_SUBTAB_INNER_W - VIDEO_COMPONENT_WIDTH + 10)
#define VCA_REV_SETTING_FIXED_Y     (MENU_V_SUBTAB_INNER_Y)





////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent_fixed = 0;
static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_rcol_fixed;

static gint g_cur_channel = -1;
static guint g_draw_tid = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//
static gint _set_default_option_component_data(VCA_COMPONENT_DATA_T *component_data)
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

static gint _set_vaa_prop_data(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VARULE_PROP prop;

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    
    vaa_itx_get_rule_prop(vaaid, &prop);

    prop.unit = component_data->unit_setup;
    prop.en_shadowrm = component_data->option.shadow_removal;
    prop.track_ref = component_data->option.track_reference;
    prop.en_usecalib = component_data->option.minimum_object;
    prop.min_width3d = component_data->option.minimum_w;
    prop.min_height3d = component_data->option.minimum_h;
    prop.sw_obj_bb = component_data->option.disp_obj_box;
    prop.sw_obj_tr = component_data->option.disp_obj_traj;
    prop.sw_obj_id = component_data->option.disp_obj_id;
    prop.sw_obj_w3d = component_data->option.disp_obj_w;
    prop.sw_obj_h3d = component_data->option.disp_obj_h;
    prop.sw_obj_s3d = component_data->option.disp_obj_speed;
    prop.sw_rule = component_data->option.disp_rules;    
    
    vaa_itx_set_rule_prop(vaaid, &prop);
    return 0;
}

static gint _option_component_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_prop_data(component_data);

    return 0;
}

static gint _option_component_default_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_default_option_component_data(component_data);
    _set_vaa_prop_data(component_data);
    vw_vca_rev_video_component_sync_data(user_data);
    
    return 0;
}

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_DATA_T *component_data;

    component_data = imalloc(sizeof(VCA_COMPONENT_DATA_T));

    component_data->act_capable = 0;
    component_data->act_license = 0;

    component_data->preview.ch = ch;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

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
    
    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_DATA, component_data);
    return 0;
}


static gint _init_component_action(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(VCA_COMPONENT_ACTION_T));
    
    component_action->option_cb = _option_component_cb;

    component_action->option_default_cb = _option_component_default_cb;
    
    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_ACTION, component_action);
    return 0;
}

static gint _get_calibration_cnt(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i, cnt = 0;
   
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb == 1) cnt++;
    }

    return cnt;
}

static gint _get_vaa_calb_data(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACALB_RESULT res;
    
    vaaid = vaa_get_vaaid(component_data->preview.ch);
    vaa_itx_get_calb_result(vaaid, &res);

    component_data->calibration.icon_count = _get_calibration_cnt(vaaid);
    component_data->calibration.icon_height = 175;
    component_data->calibration.pause_video = 0; 
    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;
    return 0;
}

static gint _get_vaa_prop_data(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VARULE_PROP prop;
    
    vaaid = vaa_get_vaaid(component_data->preview.ch);
    vaa_itx_get_rule_prop(vaaid, &prop);
    
    component_data->unit_setup = prop.unit;
    component_data->option.shadow_removal = prop.en_shadowrm;
    component_data->option.track_reference = prop.track_ref;
    component_data->option.minimum_object = prop.en_usecalib;
    component_data->option.minimum_w = prop.min_width3d+1;
    component_data->option.minimum_h = prop.min_height3d+1;
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
    VCA_COMPONENT_DATA_T *component_data;

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gboolean _draw_vca_rule(gpointer data)
{
    vw_vca_rev_video_component_sync_data(g_video_fixed);
	return TRUE;
}

static gboolean _init_vca_rule(gpointer data)
{
	if (!g_draw_tid) g_draw_tid = g_timeout_add(100, _draw_vca_rule, 0);
	return FALSE;
}

static gint _is_changed_data()
{
	guint db_changed = 0;
	VAAID vaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{
		vaaid = vaa_get_vaaid(i);
		if (vaa_itx_is_db_changed(vaaid)) return 1;
	}

	return 0;
}

static gint _save_data()
{
	guint db_changed = 0;
	VAAID vaaid;
	int i;

	for (i = 0; i < var_get_ch_count(); ++i) 
	{	
		vaaid = vaa_get_vaaid(i);
		if (vaa_itx_is_db_changed(vaaid)) {
			vaa_itx_save_db(vaaid);
			db_changed |= (1 << i);
		}
	}

	if (db_changed) {
		DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
		DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, db_changed);
//		evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);
	}
	
	return db_changed;
}

static gint _load_data()
{
	if (_is_changed_data() == 0) return -1;

	vaa_reload();
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
    	if (g_draw_tid) 
    	{
    		g_source_remove(g_draw_tid);
    		g_draw_tid = 0;
    	}
	}

	return FALSE;
}

static gboolean post_start_wizbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
   
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        component_data->disable_event = 1;
        component_data->preview.onoff = 0;
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);
        
        vw_vca_rev_wizard_popup_open(g_curwnd, component_data->preview.ch);

        _get_vaa_calb_data(component_data);       
        _get_vaa_prop_data(component_data);
        nfui_user_signal_emit(g_rcol_fixed, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);

        component_data->disable_event = 0;        
        component_data->preview.onoff = 1;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);
        component_data->disp_rule.block_update = 0;
        component_data->disp_rule.delay_update = 1;
        component_data->disp_rule.delay_max = 5;
        component_data->disp_rule.delay_cnt = 0;            
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_resetbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {       
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        if (g_cur_channel == -1) return FALSE;

        nf_ipcam_set_va_reset(g_cur_channel, NULL, NULL, NULL);
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}






/*
 :  VCA_Rev_prop main_fixed.

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

gint _analysis_classic_prop_init_page(NFOBJECT *parent)
{
    NFOBJECT *fixed = parent;
    NFOBJECT *video_fixed;    
    NFOBJECT *obj;
            
    gint pos_x, pos_y;
	guint i = 0, j = 0;

	VAAID vaaid;
	DITID dit;
    VCA_COMPONENT_DATA_T *component_data;	


    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    _init_component_data(nfui_nfobject_get_top(parent), 0);
    _init_component_action(nfui_nfobject_get_top(parent), 0);


    pos_x = 0;
    pos_y = 0;

    video_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(video_fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_nfobject_show(video_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, video_fixed, 0, 112);        // 16-align, x:368, y:192
    nfui_regi_post_event_callback(video_fixed, post_video_fixed_event_handler);
    g_video_fixed = video_fixed;

    vw_vca_rev_video_component_open(video_fixed, 0);
    _set_component_video_fixed(nfui_nfobject_get_top(parent), video_fixed);

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("START WIZARD", 300);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_start_wizbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 27, 755);

    obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("RESET VA", 300);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_resetbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MENU_V_SUBTAB_INNER_W - VCA_REV_SETTING_FIXED_W - 330, 755);
   
    pos_x = VIDEO_COMPONENT_WIDTH + 28;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, VCA_REV_SETTING_FIXED_W, parent->height-2);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, pos_x, pos_y);
    g_rcol_fixed = fixed;
    
    _vca_rev_rcol_page(g_rcol_fixed);

    g_parent_fixed = parent;
	
    return 0;
}

gint _analysis_classic_prop_show_page(gint ch, AiAnalysisActData *analysis_data)
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    VAAID vaaid;

    NF_NOTIFY_INFO vloss_info;

    if (g_cur_channel != -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    g_cur_channel = ch;

    vaaid = vaa_get_vaaid(ch); 
    vaa_itx_raiseup(vaaid);

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    _vca_rev_rcol_show_rule_page(top);

    component_data->preview.ch = ch;
    component_data->preview.onoff = 1;    
    component_data->preview.rule_mode = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    

    vw_vca_rev_video_component_sync_preview(g_video_fixed); 

    memset(&vloss_info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_vloss_data(&vloss_info);

    if (vloss_info.d.params[0] & (1 << ch))
    {
        component_data->act_capable = 0;
        component_data->act_license = 0;
    }
    else
    {
        component_data->act_capable = analysis_data[ch].classic_active;
        component_data->act_license = analysis_data[ch].classic_active;
    }

    _get_vaa_calb_data(component_data);       
    _get_vaa_prop_data(component_data);
    nfui_user_signal_emit(g_parent_fixed, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);      

    _vca_rev_live_log_clear();
    _vca_rev_rcol_load_init_calb_data(ch);

    if (!g_draw_tid) g_draw_tid = g_timeout_add(100, _draw_vca_rule, 0);

    return 0;
}

gint _analysis_classic_prop_hide_page(gint ch)
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;

    if (g_cur_channel == -1) return -1;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    if (g_draw_tid) {
        g_source_remove(g_draw_tid);
        g_draw_tid = 0;
    }

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_vca_rev_video_component_sync_preview(g_video_fixed);
    vw_vca_rev_video_component_expose(g_video_fixed);

    component_data->calibration.pause_video = 0;
    nfui_user_signal_emit(g_rcol_fixed, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);

    g_cur_channel = -1;
    return 0;
}

gint _analysis_classic_prop_load_changed_data(gint ch)
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    return 0;
}

gint _analysis_classic_prop_is_changed_data()
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

gint _analysis_classic_prop_save_data()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    _vca_rev_rcol_estimation_calb();
    _save_data();
    return 0;
}

gint _analysis_classic_prop_cancel_data(gint ch, gint expose)
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    VAAID vaaid;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    _load_data();

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    _vca_rev_rcol_show_rule_page(top);

    component_data->preview.rule_mode = 0;
    vw_vca_rev_video_component_sync_preview(g_video_fixed);
    vw_vca_rev_video_component_expose(g_video_fixed);        

    vaaid = vaa_get_vaaid(ch); 

    _vca_rev_calb_data_cancel(vaaid, component_data);
    _get_vaa_calb_data(component_data);       
    _get_vaa_prop_data(component_data);

    nfui_user_signal_emit(g_parent_fixed, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);

    return 0;
}

