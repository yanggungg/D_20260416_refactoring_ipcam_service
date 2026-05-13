/*
 * vw_vca_rev_wizard_popup.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 14, 2014
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

#include "vw_dit_vca.h"
#include "vw_vca_rev_component.h"
#include "vw_vca_rev_wizard_popup.h"


#define WIN_WIDTH                   (VIDEO_COMPONENT_WIDTH+DEFAULT_COMPONENT_WIDTH+60)
#define WIN_HEIGHT                  (60+DEFAULT_COMPONENT_HEIGHT+70)



////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_prevwnd = 0;
static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_steptitle_obj = 0;

static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_unitsetup_fixed = 0;
static NFOBJECT *g_calisetup_fixed = 0;
static NFOBJECT *g_caliadrz_fixed = 0;
static NFOBJECT *g_caliresult_fixed = 0;
static NFOBJECT *g_rulesetup_fixed = 0;
static NFOBJECT *g_ruleline_fixed = 0;
static NFOBJECT *g_rulearea_fixed = 0;
static NFOBJECT *g_counter_fixed = 0;
static NFOBJECT *g_option_fixed = 0;

static NFOBJECT *g_counter_prevfixed = 0;

static gint g_cur_ch = 0;
static guint g_draw_tid = 0;

static ITX_VARULE_PROP g_pre_rule_prop;

static ITX_VAZONE_CONF g_pre_zone_conf[16];
static ITX_VAZONE_SHAPE g_pre_zone_shape[16];

static ITX_VACNTR_CONF g_pre_cntr_conf[16];
static ITX_VACNTR_SHAPE g_pre_cntr_shape[16];

static ITX_VACALB_CONF g_pre_calb_conf[32];
static ITX_VACALB_SHAPE g_pre_calb_shape[32];
static ITX_VACALB_RESULT g_pre_calb_result;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _get_new_line_idx()
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;    
    gint i;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &conf);
        if (conf.use_zone == 0) return i;
    }

    return -1;
}

static gint _get_new_area_idx()
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;    
    gint i;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &conf);
        if (conf.use_zone == 0) return i;
    }

    return -1;
}

static gint _get_new_counter_idx()
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;    
    gint i;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_cntr_conf(vaaid, i, &conf);
        if (conf.use_cntr == 0) return i;
    }

    return -1;
}

static gint _get_default_line_name(gint index, gchar *strBuf)
{
    g_sprintf(strBuf, "ZONE %d", index+1);
    return 0;
}

static gint _get_default_area_name(gint index, gchar *strBuf)
{
    g_sprintf(strBuf, "ZONE %d", index+1);
    return 0;
}

static gint _get_default_counter_name(gint index, gchar *strBuf)
{
    g_sprintf(strBuf, "COUNTER %d", index+1);
    return 0;
}

static gint _get_default_line_color(gint index, gint *color_idx)
{
    switch(index)
    {
        case 0:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;
        case 1:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF3F00); break;
        case 2:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF7F00); break;
        case 3:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFBF00); break;
        case 4:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case 5:     *color_idx = COLOR_PRG_IDX(UX_COLOR_BFFF00); break;
        case 6:     *color_idx = COLOR_PRG_IDX(UX_COLOR_7FFF00); break;
        case 7:     *color_idx = COLOR_PRG_IDX(UX_COLOR_3FFF00); break;
        case 8:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case 9:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF3F); break;
        case 10:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF7F); break;
        case 11:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFBF); break;
        case 12:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFFF); break;
        case 13:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00BFFF); break;
        case 14:    *color_idx = COLOR_PRG_IDX(UX_COLOR_007FFF); break;
        case 15:    *color_idx = COLOR_PRG_IDX(UX_COLOR_003FFF); break;
        case 16:    *color_idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case 17:    *color_idx = COLOR_PRG_IDX(UX_COLOR_3F00FF); break;
        case 18:    *color_idx = COLOR_PRG_IDX(UX_COLOR_7F00FF); break;
        case 19:    *color_idx = COLOR_PRG_IDX(UX_COLOR_BF00FF); break;
        case 20:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00FF); break;
        case 21:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00BF); break;
        case 22:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF007F); break;
        case 23:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F); break;        
        default: break;
    }

    return 0;
}

static gint _get_default_area_color(gint index, gint *color_idx)
{
    switch(index)
    {
        case 0:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;
        case 1:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF3F00); break;
        case 2:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF7F00); break;
        case 3:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFBF00); break;
        case 4:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case 5:     *color_idx = COLOR_PRG_IDX(UX_COLOR_BFFF00); break;
        case 6:     *color_idx = COLOR_PRG_IDX(UX_COLOR_7FFF00); break;
        case 7:     *color_idx = COLOR_PRG_IDX(UX_COLOR_3FFF00); break;
        case 8:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case 9:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF3F); break;
        case 10:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF7F); break;
        case 11:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFBF); break;
        case 12:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFFF); break;
        case 13:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00BFFF); break;
        case 14:    *color_idx = COLOR_PRG_IDX(UX_COLOR_007FFF); break;
        case 15:    *color_idx = COLOR_PRG_IDX(UX_COLOR_003FFF); break;
        case 16:    *color_idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case 17:    *color_idx = COLOR_PRG_IDX(UX_COLOR_3F00FF); break;
        case 18:    *color_idx = COLOR_PRG_IDX(UX_COLOR_7F00FF); break;
        case 19:    *color_idx = COLOR_PRG_IDX(UX_COLOR_BF00FF); break;
        case 20:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00FF); break;
        case 21:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00BF); break;
        case 22:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF007F); break;
        case 23:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F); break;        
        default: break;
    }
    return 0;
}

static gint _get_default_counter_color(gint index, gint *color_idx)
{
    switch(index)
    {
        case 0:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;
        case 1:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF3F00); break;
        case 2:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FF7F00); break;
        case 3:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFBF00); break;
        case 4:     *color_idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case 5:     *color_idx = COLOR_PRG_IDX(UX_COLOR_BFFF00); break;
        case 6:     *color_idx = COLOR_PRG_IDX(UX_COLOR_7FFF00); break;
        case 7:     *color_idx = COLOR_PRG_IDX(UX_COLOR_3FFF00); break;
        case 8:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case 9:     *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF3F); break;
        case 10:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FF7F); break;
        case 11:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFBF); break;
        case 12:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00FFFF); break;
        case 13:    *color_idx = COLOR_PRG_IDX(UX_COLOR_00BFFF); break;
        case 14:    *color_idx = COLOR_PRG_IDX(UX_COLOR_007FFF); break;
        case 15:    *color_idx = COLOR_PRG_IDX(UX_COLOR_003FFF); break;
        case 16:    *color_idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case 17:    *color_idx = COLOR_PRG_IDX(UX_COLOR_3F00FF); break;
        case 18:    *color_idx = COLOR_PRG_IDX(UX_COLOR_7F00FF); break;
        case 19:    *color_idx = COLOR_PRG_IDX(UX_COLOR_BF00FF); break;
        case 20:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00FF); break;
        case 21:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF00BF); break;
        case 22:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF007F); break;
        case 23:    *color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F); break;        
        default: break;
    }
    return 0;
}
  
static gint _add_vaa_line(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_add_zone_line_default_template(vaaid, rule_idx);    

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    component_data->zone_id = conf.zoneid;    
    return 0;
}

static gint _add_vaa_area(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_add_zone_area_default_template(vaaid, rule_idx);    

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    component_data->zone_id = conf.zoneid;
    return 0;
}

static gint _add_vaa_counter(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_add_cntr_default_template(vaaid, rule_idx);    

    vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);   
    component_data->cntr_id = conf.cntrid;    
    return 0;
}

static gint _set_vaa_prop(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VARULE_PROP prop;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
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

static gint _set_vaa_line_shape(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(shape.name, component_data->line.name);
    shape.color_idx= component_data->line.display_color_idx;
    vaa_itx_set_zone_shape(vaaid, rule_idx, &shape);
    return 0;
}

static gint _set_vaa_line_conf(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.active = component_data->line.active;
    
    conf.forward = component_data->line.forward;
    conf.reverse = component_data->line.reverse;   

    conf.use_filter_color= component_data->line.use_filter_color;
    conf.filter_color_idx = component_data->line.filter_color_idx;
    conf.filter_color_prct = component_data->line.filter_color_percnt;

    conf.use_filter_size = component_data->line.use_filter_size;
    conf.filter_width_from = component_data->line.filter_min_size_w;
    conf.filter_width_to = component_data->line.filter_max_size_w;
    conf.filter_height_from = component_data->line.filter_min_size_h;
    conf.filter_height_to = component_data->line.filter_max_size_h;

    conf.use_filter_speed = component_data->line.use_filter_speed;
    conf.filter_speed_from = component_data->line.filter_min_speed;
    conf.filter_speed_to = component_data->line.filter_max_speed;
    
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);
    return 0;
}

static gint _set_vaa_area_shape(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(shape.name, component_data->area.name);
    shape.color_idx= component_data->area.display_color_idx;
    vaa_itx_set_zone_shape(vaaid, rule_idx, &shape);
    return 0;
}

static gint _set_vaa_area_conf(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.active = component_data->area.active;
    
    conf.enter = component_data->area.enter;
    conf.exit = component_data->area.exit;
    conf.removed = component_data->area.removed;
    conf.cfg_remove_time = component_data->area.removed_val;
    conf.loitering = component_data->area.loitering;
    conf.cfg_loiter_time = component_data->area.loitering_val;
    conf.stopped = component_data->area.stopped; 
    conf.cfg_stop_time = component_data->area.stopped_val;

    conf.use_filter_color= component_data->area.use_filter_color;
    conf.filter_color_idx = component_data->area.filter_color_idx;
    conf.filter_color_prct = component_data->area.filter_color_percnt;

    conf.use_filter_size = component_data->area.use_filter_size;
    conf.filter_width_from = component_data->area.filter_min_size_w;
    conf.filter_width_to = component_data->area.filter_max_size_w;
    conf.filter_height_from = component_data->area.filter_min_size_h;
    conf.filter_height_to = component_data->area.filter_max_size_h;

    conf.use_filter_speed = component_data->area.use_filter_speed;
    conf.filter_speed_from = component_data->area.filter_min_speed;
    conf.filter_speed_to = component_data->area.filter_max_speed;
    
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);
    return 0;
}

static gint _set_vaa_counter_shape(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACNTR_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_cntr_shape(vaaid, rule_idx, &shape);
    strcpy(shape.name, component_data->counter.name);
    shape.color_idx = component_data->counter.display_color_idx;
    vaa_itx_set_cntr_shape(vaaid, rule_idx, &shape);
    return 0;
}

static gint _set_vaa_counter_conf(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);   
    conf.active = component_data->counter.active;
    
    conf.use_counter_event = component_data->counter.use_counter_event;
    conf.counter_event_val = component_data->counter.counter_event_val;
    conf.use_reset_value = component_data->counter.use_reset_value;
    
    conf.source_up = component_data->counter.up_source;
    conf.source_down = component_data->counter.down_source;
    
    vaa_itx_set_cntr_conf(vaaid, rule_idx, &conf);
    return 0;
}

static gint _set_vaa_counter_list(VCA_COMPONENT_DATA_T *component_data)
{
    ITX_VAZONE_CONF zone_conf;
    ITX_VAZONE_SHAPE zone_shape;

    VAAID vaaid;
    gint i, idx = 0;

    vaaid = vaa_get_vaaid(g_cur_ch);
    for (i = 0; i < 16; i++)
    {  
        vaa_itx_get_zone_conf(vaaid, i, &zone_conf);
        vaa_itx_get_zone_shape(vaaid, i, &zone_shape);

        if (zone_conf.use_zone)
            strcpy(component_data->counter.source_list[zone_conf.zoneid], zone_shape.name);
            
    }    
    return 0;
}

static gint _delete_vaa_line(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    if (component_data->zone_id == -1) return -1;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_conf(vaaid, component_data->zone_id, &conf);   
    conf.use_zone = 0;
    vaa_itx_set_zone_conf(vaaid, component_data->zone_id, &conf);

    component_data->zone_id = -1;
    return 0;
}

static gint _delete_vaa_area(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    if (component_data->zone_id == -1) return -1;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_conf(vaaid, component_data->zone_id, &conf);   
    conf.use_zone = 0;
    vaa_itx_set_zone_conf(vaaid, component_data->zone_id, &conf);

    component_data->zone_id = -1;
    return 0;
}

static gint _delete_vaa_counter(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;

    if (component_data->cntr_id == -1) return -1;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_cntr_conf(vaaid, component_data->cntr_id, &conf);   
    conf.use_cntr = 0;
    vaa_itx_set_cntr_conf(vaaid, component_data->cntr_id, &conf);

    component_data->cntr_id = -1;
    return 0;
}

static gint _set_focus_vaa_line(gint rule_idx)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.focus = 1;
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);    
    return 0;
}

static gint _set_focus_vaa_area(gint rule_idx)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.focus = 1;
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);    
    return 0;
}

static gint _set_focus_vaa_counter(gint rule_idx)
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);   
    conf.focus = 1;
    vaa_itx_set_cntr_conf(vaaid, rule_idx, &conf);
    return 0;
}

static gint _unset_focus_vaa_line(gint rule_idx)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.focus = 0;
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);    
    return 0;
}

static gint _unset_focus_vaa_area(gint rule_idx)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    conf.focus = 0;
    vaa_itx_set_zone_conf(vaaid, rule_idx, &conf);    
    return 0;
}

static gint _unset_focus_vaa_counter(gint rule_idx)
{
    VAAID vaaid;
    ITX_VACNTR_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);   
    conf.focus = 0;
    vaa_itx_set_cntr_conf(vaaid, rule_idx, &conf);
    return 0;
}

static gint _unset_focus_all_rule()
{
    VAAID vaaid;
    ITX_VAZONE_CONF zone_conf;    
    ITX_VACNTR_CONF cntr_conf;
    gint i;

    vaaid = vaa_get_vaaid(g_cur_ch);

    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &zone_conf);   
        zone_conf.focus = 0;
        vaa_itx_set_zone_conf(vaaid, i, &zone_conf);
    }

    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_cntr_conf(vaaid, i, &cntr_conf);   
        zone_conf.focus = 0;
        vaa_itx_set_cntr_conf(vaaid, i, &cntr_conf);
    }    

    return 0;
}

static gint _get_calb_result_component_data(VAAID vaaid, VCA_COMPONENT_DATA_T *component_data)
{
    ITX_VACALB_RESULT res;    

    vaa_itx_get_calb_result(vaaid, &res);

    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;    
    return 0;
}

static gint _get_new_calibration_idx(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb == 0) return i;
    }

    return -1;
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

static gint _get_focus_calibration_idx(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if ((conf.use_calb == 1) && (conf.focus == 1)) return i;
    }

    return -1;
}

static gint _set_focus_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.focus = 1;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);

    return 0;
}

static gint _unset_focus_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.focus = 0;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);
    
    return 0;
}

static gint _unset_focus_all_calibration(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb) _unset_focus_calibration(vaaid, i);
    }

    return 0;
}

static gint _delete_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.use_calb = 0;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);
    return 0;
}


static gint _delete_all_calibration(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        _delete_calibration(vaaid, i);
    }
    return 0;
}

static gint _get_targetlist_from_calb_shape(VAAID vaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_VACALB_SHAPE shape;    

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_shape(vaaid, i, &shape);
    
        vca_calb->targetlist[i].pt[0].x = shape.pt[0].x;
        vca_calb->targetlist[i].pt[0].y = shape.pt[0].y;
        vca_calb->targetlist[i].pt[1].x = shape.pt[1].x;
        vca_calb->targetlist[i].pt[1].y = shape.pt[1].y;
    }

    return 0;
}

static gint _get_targetlist_from_calb_conf(VAAID vaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_VACALB_CONF conf;    

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        vca_calb->targetlist[i].height = conf.height;
    }

    return 0;
}

static gint _get_targetlist_from_calb_result(VAAID vaaid, ivca_calib_t *vca_calb)
{
    ITX_VACALB_RESULT res;    

    vaa_itx_get_calb_result(vaaid, &res);

    vca_calb->p_width = res.p_width;
    vca_calb->p_height = res.p_height;
    vca_calb->height = res.cam_height;
    vca_calb->tilt = res.cam_tilt;
    vca_calb->focal = res.focal;
    vca_calb->paramvalid = res.paramvalid;
    
    return 0;
}

static gint _get_calb_shape_from_zoomdata(VAAID vaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_VACALB_SHAPE shape;    

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_shape(vaaid, i, &shape);
   
        shape.pt[0].x = vca_calb->targetlist[i].pt[0].x;
        shape.pt[0].y = vca_calb->targetlist[i].pt[0].y;
        shape.pt[1].x = vca_calb->targetlist[i].pt[1].x;
        shape.pt[1].y = vca_calb->targetlist[i].pt[1].y;

        g_sprintf(shape.value, "%d", vca_calb->height);        
        vaa_itx_set_calb_shape(vaaid, i, &shape);
    }

    return 0;
}

static gint _get_calb_conf_from_zoomdata(VAAID vaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_VACALB_CONF conf;    

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);

        if (i < vca_calb->ntargets) conf.use_calb = 1;
        else conf.use_calb = 0;
        
        conf.height = vca_calb->targetlist[i].height;

        vaa_itx_set_calb_conf(vaaid, i, &conf);
    }
    
    return 0;
}

static gint _set_calb_estimate_data(VAAID vaaid, ivca_calib_t *vca_calb)
{
    ITX_VACALB_RESULT res;    

    vaa_itx_get_calb_result(vaaid, &res);

    res.p_width = vca_calb->p_width;
    res.p_height = vca_calb->p_height;
    res.cam_height = vca_calb->height;
    res.cam_tilt = vca_calb->tilt;
    res.focal = vca_calb->focal;
    res.paramvalid = vca_calb->paramvalid;

    vaa_itx_set_calb_result(vaaid, &res);
    
    return 0;
}

static gint _update_calb_adrz_component_data(VAAID vaaid, VCA_COMPONENT_DATA_T *component_data)
{
    ITX_VACALB_CONF conf;    
    ITX_CALBID calbid;

    calbid = _get_focus_calibration_idx(vaaid);
    
    if (calbid != -1)
    {
        vaa_itx_get_calb_conf(vaaid, calbid, &conf);
        component_data->calibration.select_icon = 1;        
        component_data->calibration.icon_height = conf.height;
    }
    else 
    {
        component_data->calibration.select_icon = 0;
    }
    
    component_data->calibration.icon_count = _get_calibration_cnt(vaaid);
    vw_vca_rev_calibration_adrz_component_sync_data(g_caliadrz_fixed);
    
    return 0;
}

static gint _expose_calb_adrz_component_data()
{
    vw_vca_rev_calibration_adrz_component_expose_data(g_caliadrz_fixed);
    return 0;
}

static gint _update_calb_result_component_data(VAAID vaaid, VCA_COMPONENT_DATA_T *component_data)
{
    ITX_VACALB_RESULT res;    

    vaa_itx_get_calb_result(vaaid, &res);

    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;    

    vw_vca_rev_calibration_result_component_sync_data(g_caliresult_fixed);    
    vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);    
    vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);    
    vw_vca_rev_option_component_sync_data(g_option_fixed);    
    return 0;
}

static gint _set_title_showfixed(NFOBJECT *showfixed)
{
    gchar strBuf[128];

    memset(strBuf, 0x00, sizeof(strBuf));

    if (showfixed == g_unitsetup_fixed)         g_sprintf(strBuf, "%s1 - %s %s", lookup_string("STEP"), lookup_string("UNITS SETUP"), "(1/1)");
    else if (showfixed == g_calisetup_fixed)    g_sprintf(strBuf, "%s2 - %s %s", lookup_string("STEP"), lookup_string("3D CALIBRATION"), "(1/3)");    
    else if (showfixed == g_caliadrz_fixed)     g_sprintf(strBuf, "%s2 - %s %s", lookup_string("STEP"), lookup_string("3D CALIBRATION"), "(2/3)");    
    else if (showfixed == g_caliresult_fixed)   g_sprintf(strBuf, "%s2 - %s %s", lookup_string("STEP"), lookup_string("3D CALIBRATION"), "(3/3)");    
    else if (showfixed == g_rulesetup_fixed)    g_sprintf(strBuf, "%s3 - %s %s", lookup_string("STEP"), lookup_string("RULE SETUP"), "(1/3)");    
    else if (showfixed == g_ruleline_fixed)     g_sprintf(strBuf, "%s3 - %s %s", lookup_string("STEP"), lookup_string("RULE SETUP"), "(2/3)");    
    else if (showfixed == g_rulearea_fixed)     g_sprintf(strBuf, "%s3 - %s %s", lookup_string("STEP"), lookup_string("RULE SETUP"), "(2/3)");    
    else if (showfixed == g_counter_fixed)      g_sprintf(strBuf, "%s3 - %s %s", lookup_string("STEP"), lookup_string("RULE SETUP"), "(3/3)");    
    else if (showfixed == g_option_fixed)       g_sprintf(strBuf, "%s4 - %s %s", lookup_string("STEP"), lookup_string("OPTION"), "(1/1)");    

	nfui_nfimage_set_text((NFIMAGE*)g_steptitle_obj, strBuf);
	nfui_signal_emit(g_steptitle_obj, GDK_EXPOSE, TRUE);
    return 0;
}

static gint _set_default_calibration_component_data(VCA_COMPONENT_DATA_T *component_data)
{
    component_data->calibration.icon_count = 0;
    component_data->calibration.icon_height = 175;
    component_data->calibration.pause_video = 0;
    component_data->calibration.camera_height = 12.00;
    component_data->calibration.camera_tilt = 35;
    
    return 0;
}

static gint _set_default_line_component_data(VCA_COMPONENT_DATA_T *component_data)
{
    strcpy(component_data->line.name, "");
    component_data->line.active = 1;
    component_data->line.forward = 0;
    component_data->line.reverse = 0;
    component_data->line.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
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
    
    return 0;
}

static gint _set_default_area_component_data(VCA_COMPONENT_DATA_T *component_data)
{
    strcpy(component_data->area.name, "");
    component_data->area.active = 1;
    component_data->area.enter = 0;
    component_data->area.exit = 0;
    component_data->area.removed = 0;
    component_data->area.loitering = 0;
    component_data->area.stopped = 0;    
    component_data->area.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
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
    
    return 0;
}

static gint _set_default_counter_component_data(VCA_COMPONENT_DATA_T *component_data)
{
    strcpy(component_data->counter.name, "");
    component_data->counter.active = 1;
    component_data->counter.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->counter.use_counter_event = 1;
    component_data->counter.counter_event_val = 100;
    component_data->counter.use_reset_value = 0;
    component_data->counter.up_source = -1;
    component_data->counter.down_source = -1;
    
    return 0;
}

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

static gint _rule_component_type_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    return 0;
}

static gint _rule_component_act_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA); 

    if (component_data->rule_type == RTYPE_LINE) _set_vaa_line_conf(component_data->zone_id, component_data);
    else if (component_data->rule_type == RTYPE_AREA) _set_vaa_area_conf(component_data->zone_id, component_data);
    else if (component_data->rule_type == RTYPE_COUNTER) _set_vaa_counter_conf(component_data->cntr_id, component_data);

    return 0;
}

static gint _line_component_name_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_line_shape(component_data->zone_id, component_data);

    return 0;
}

static gint _line_component_dis_color_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_line_shape(component_data->zone_id, component_data);

    return 0;
}

static gint _area_component_name_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_area_shape(component_data->zone_id, component_data);

    return 0;
}

static gint _area_component_dis_color_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_area_shape(component_data->zone_id, component_data);

    return 0;
}

static gint _counter_component_use_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;
    gint cntr_idx;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    if (component_data->use_counter == 1)
    {
        cntr_idx = _get_new_counter_idx();

        if (cntr_idx >= 0) 
        {
            _get_default_counter_name(cntr_idx, component_data->counter.name);
            _get_default_counter_color(cntr_idx, &component_data->counter.display_color_idx);
        
            _add_vaa_counter(cntr_idx, component_data);
            _set_vaa_counter_shape(cntr_idx, component_data);
            _set_vaa_counter_conf(cntr_idx, component_data);

            _unset_focus_all_rule();
            _set_focus_vaa_counter(cntr_idx);

            vw_vca_rev_counter_component_sync_data(g_counter_fixed);
        }
        else
        {
            nftool_mbox(g_curwnd, "NOTICE", "You can set up to 16 counters.", NFTOOL_MB_OK);
        
            component_data->use_counter = 0;
            vw_vca_rev_counter_component_sync_data(g_counter_fixed);
        }
    }
    else
    {
        _delete_vaa_counter(component_data);
        vw_vca_rev_counter_component_sync_data(g_counter_fixed);
    }

    return 0;
}

static gint _counter_component_name_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_counter_shape(component_data->cntr_id, component_data);

    return 0;
}

static gint _counter_component_dis_color_cb(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    _set_vaa_counter_shape(component_data->cntr_id, component_data);

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
    _set_vaa_prop(component_data);

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
    _set_vaa_prop(component_data);
    vw_vca_rev_video_component_sync_data(user_data);
    
    return 0;
}

static gint _calibration_adrz_component_add(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    VAAID vaaid;    
    gint calb_idx;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    calb_idx = _get_new_calibration_idx(vaaid);

    if (calb_idx == -1) {
        nftool_mbox(g_curwnd, "NOTICE", "You can set up to 32 icons.", NFTOOL_MB_OK);
        return -1;
    }

    _unset_focus_all_calibration(vaaid);
    vaa_itx_add_calb_default_template(vaaid, calb_idx);
    _set_focus_calibration(vaaid, calb_idx);
    _update_calb_adrz_component_data(vaaid, component_data); 
    _expose_calb_adrz_component_data();
	return 0;
}

static gint _calibration_adrz_component_delete(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    VAAID vaaid;    
    gint calb_idx;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    calb_idx = _get_focus_calibration_idx(vaaid);

    if (calb_idx == -1) return -1;

    _delete_calibration(vaaid, calb_idx);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    _update_calb_adrz_component_data(vaaid, component_data);
    _expose_calb_adrz_component_data();
	return 0;
}

static gint _calibration_adrz_component_reset(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    VAAID vaaid;
    ITX_VACALB_RESULT res;
    
    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    _delete_all_calibration(vaaid);

    vaa_itx_get_calb_result(vaaid, &res);
    res.cam_height = 0;
    res.cam_tilt = 0;
    res.focal = 0;
    res.paramvalid = 0;    
    vaa_itx_set_calb_result(vaaid, &res);

    _update_calb_adrz_component_data(vaaid, component_data);
    _expose_calb_adrz_component_data();
    _update_calb_result_component_data(vaaid, component_data);    
	return 0;
}

static gboolean _delay_preview_start(gpointer data)
{
    NFOBJECT *obj = (NFOBJECT*)data;
    nfui_ui_unlock();
    vw_vca_rev_video_component_sync_preview(obj);
    vw_vca_rev_video_component_expose(obj);   
    return FALSE;
}

static gint _calibration_adrz_component_zoom(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;
    VAAID vaaid;    

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    component_data->preview.onoff = 0;
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    
    vw_vca_rev_video_component_sync_preview(component_data->video_fixed);
    vw_vca_rev_video_component_expose(component_data->video_fixed);

    vw_vca_rev_calibration_zoom_open(top, component_data->preview.ch);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    component_data->calibration.pause_video = 0;
    _update_calb_adrz_component_data(vaaid, component_data);
    _update_calb_result_component_data(vaaid, component_data);

    component_data->preview.onoff = 1;
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    component_data->disp_rule.block_update = 0;
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 10;
    component_data->disp_rule.delay_cnt = 0;   

    nfui_ui_lock();
    g_timeout_add(300, _delay_preview_start, component_data->video_fixed);

	return 0;
}

static gint _calibration_adrz_component_pause_video(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    if (component_data->calibration.pause_video)
    {
        vsm_live_preview_pause_vca();
    }
    else
    {
        vw_vca_rev_video_component_sync_preview(component_data->video_fixed);    
    }

	return 0;
}

static gint _calibration_adrz_component_icon_height(gpointer user_data)
{
    VCA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    VAAID vaaid;    
    ITX_VACALB_CONF conf;    
    ITX_VACALB_SHAPE shape;
    ITX_CALBID calbid;
    
    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_vaaid(component_data->preview.ch);
    calbid = _get_focus_calibration_idx(vaaid);
    if (calbid == -1) return -1;
    
    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.height = component_data->calibration.icon_height;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);

    vaa_itx_get_calb_shape(vaaid, calbid, &shape);
    g_sprintf(shape.value, "%d", (int)component_data->calibration.icon_height);
    vaa_itx_set_calb_shape(vaaid, calbid, &shape);    
    
	return 0;
}

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    VAAID vaaid;
    VCA_COMPONENT_DATA_T *component_data;

    vaaid = vaa_get_vaaid(ch);

    component_data = imalloc(sizeof(VCA_COMPONENT_DATA_T));

    component_data->act_capable = 1;
    component_data->act_license = 1;

    component_data->preview.ch = ch;
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    

    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;

    component_data->unit_setup = 0;
    
    component_data->skip_calibration = 0;    
    _set_default_calibration_component_data(component_data);
    _get_calb_result_component_data(vaaid, component_data);

    component_data->rule_type = RTYPE_LINE;
    component_data->use_zone = 1;
    component_data->use_counter = 0;

    component_data->zone_id = -1;
    _set_default_line_component_data(component_data);
    _set_default_area_component_data(component_data);
    
    component_data->cntr_id = -1;
    _set_default_counter_component_data(component_data);
    
    _set_default_option_component_data(component_data);

    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_DATA, component_data);
    return 0;
}


static gint _init_component_action(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(VCA_COMPONENT_ACTION_T));

    component_action->rule_type_cb = _rule_component_type_cb;

    component_action->calibration_add = _calibration_adrz_component_add;
    component_action->calibration_delete = _calibration_adrz_component_delete;
    component_action->calibration_reset = _calibration_adrz_component_reset;
    component_action->calibration_zoom = _calibration_adrz_component_zoom;
    component_action->calibration_pause = _calibration_adrz_component_pause_video; 
    component_action->calibration_height= _calibration_adrz_component_icon_height; 
    
    component_action->rule_act_cb = _rule_component_act_cb;

    component_action->line_name_cb = _line_component_name_cb;  
    component_action->line_dis_color_cb = _line_component_dis_color_cb;  

    component_action->area_name_cb = _area_component_name_cb;  
    component_action->area_dis_color_cb = _area_component_dis_color_cb;  

    component_action->counter_use_cb = _counter_component_use_cb;
    component_action->counter_name_cb = _counter_component_name_cb;  
    component_action->counter_dis_color_cb = _counter_component_dis_color_cb;  

    component_action->option_cb = _option_component_cb;
    component_action->option_default_cb = _option_component_default_cb;
    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_ACTION, component_action);
    return 0;
}

static gint _set_component_video_fixed(NFOBJECT *top, NFOBJECT *video_fixed)
{
    VCA_COMPONENT_DATA_T *component_data;

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gint _begin_paint_component(NFOBJECT *component_fixed)
{
    GdkRectangle area;
    gint off_x, off_y;

    nfui_nfobject_get_offset(component_fixed, &off_x, &off_y);   
    area.x = off_x;
    area.y = off_y;
    area.width = component_fixed->width;
    area.height = component_fixed->height;

    gdk_window_begin_paint_rect(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, &area);
    return 0;
}

static gint _end_paint_component()
{
    gdk_window_end_paint(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window);
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


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {   
		if (g_draw_tid) 
		{
			g_source_remove(g_draw_tid);
			g_draw_tid = 0;
		}
    
        g_curwnd = 0;
        gtk_main_quit();
    }

    return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
  
    switch(evt->type)
    {
        case GDK_EXPOSE :
        {
            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = nfui_nfobject_get_gc(obj); 

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR((604)));
            gdk_draw_rectangle(drawable, gc, TRUE, 20+VIDEO_COMPONENT_WIDTH+10, 60, 1, size_h-80);          
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        }
        break;
            
        default :
            break;
    }

    return FALSE;
}

static gboolean post_component_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            gint gap_x, gap_y;

            drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean post_component_calb_adrz_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            gint gap_x, gap_y;

            drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case INFY_VAA_ITX_PRESS_CALB_ID:
        {
        	NFOBJECT *top;
            VCA_COMPONENT_DATA_T *component_data;
            
            VAAID vaaid;
            ITX_CALBID id = ((CMM_MESSAGE_T*)data)->param;
            ITX_VACALB_CONF conf;
            
            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            vaaid = vaa_get_vaaid(component_data->preview.ch);
            _update_calb_adrz_component_data(vaaid, component_data);
            _expose_calb_adrz_component_data();
        }
        break;
        
        case INFY_VAA_ITX_PRESS_NONE_ID:
        {
        	NFOBJECT *top;
            VCA_COMPONENT_DATA_T *component_data;
            VAAID vaaid;
                    
            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            vaaid = vaa_get_vaaid(component_data->preview.ch);
            _update_calb_adrz_component_data(vaaid, component_data);
            _expose_calb_adrz_component_data();
        }
        break;

        case GDK_DELETE:
        {
            uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_CALB_ID);
            uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_NONE_ID);   
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;
        gint i;
        
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);

        vaa_itx_set_rule_prop(vaaid, &g_pre_rule_prop);

        for (i = 0; i < 16; i++)
        {
            vaa_itx_set_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
            vaa_itx_set_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
            vaa_itx_set_cntr_shape(vaaid, i, &g_pre_cntr_shape[i]);
            vaa_itx_set_cntr_conf(vaaid, i, &g_pre_cntr_conf[i]);
        }    

        for (i = 0; i < 32; i++)
        {
            vaa_itx_set_calb_conf(vaaid, i, &g_pre_calb_conf[i]);
            vaa_itx_set_calb_shape(vaaid, i, &g_pre_calb_shape[i]);
        }    

        vaa_itx_set_calb_result(vaaid, &g_pre_calb_result);

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaa_itx_disable_strule(vaaid);

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);	
	}
	
	return FALSE;
}

static gboolean post_unitsetup_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_calisetup_fixed);
        nfui_signal_emit(g_calisetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_vaa_prop(component_data);

        _set_title_showfixed(g_calisetup_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationsetup_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_unitsetup_fixed);
        nfui_signal_emit(g_unitsetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_unitsetup_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationsetup_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *next_fixed = NULL;

        VAAID vaaid;
        gint i;

	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        vaaid = vaa_get_vaaid(g_cur_ch);

        if (component_data->skip_calibration == 0) 
        {
            next_fixed = g_caliadrz_fixed;

		    component_data->preview.rule_mode = 1;
		    vw_vca_rev_video_component_sync_preview(g_video_fixed);
        }
        else if (component_data->skip_calibration == 1) 
        {
            next_fixed = g_rulesetup_fixed;

		    component_data->preview.rule_mode = 0;
		    vw_vca_rev_video_component_sync_preview(g_video_fixed);

            for (i = 0; i < 32; i++)
            {
                vaa_itx_set_calb_conf(vaaid, i, &g_pre_calb_conf[i]);
                vaa_itx_set_calb_shape(vaaid, i, &g_pre_calb_shape[i]);
            }    

            vaa_itx_set_calb_result(vaaid, &g_pre_calb_result);		    
        }
    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(next_fixed);
        nfui_signal_emit(next_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(next_fixed);

        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationadrz_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        vw_vca_rev_calibration_adrz_component_sync_data(g_caliadrz_fixed);
        vw_vca_rev_calibration_result_component_sync_data(g_caliresult_fixed);

	    component_data->preview.rule_mode = 0;
	    vw_vca_rev_video_component_sync_preview(g_video_fixed);
            
        _begin_paint_component(obj->parent);        
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_calisetup_fixed);
        nfui_signal_emit(g_calisetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_calisetup_fixed);        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationadrz_estimatebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        VAAID vaaid;
        gint cnt;

        ivca_calib_t vca_calb;
    
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        vaaid = vaa_get_vaaid(component_data->preview.ch);    
        cnt = _get_calibration_cnt(vaaid);
    
        if (cnt < 5) 
        {
            nftool_mbox((NFWINDOW*)top, "NOTICE", "You have to set at least 5 icons to move the next stage.", NFTOOL_MB_OK);
            return FALSE;
        }

        memset(&vca_calb, 0x00, sizeof(ivca_calib_t));
        _get_targetlist_from_calb_shape(vaaid, &vca_calb);
        _get_targetlist_from_calb_conf(vaaid, &vca_calb);
        _get_targetlist_from_calb_result(vaaid, &vca_calb);
        vca_calb.ntargets = _get_calibration_cnt(vaaid);

        if (vw_vca_cal_estimate(top, &vca_calb, component_data->preview.ch) == -1) 
        {
            nftool_mbox((NFWINDOW*)top, "ERROR", "Some icons are abnormal. Please adjust the icon size or add more icons.", NFTOOL_MB_OK);        
            return FALSE;
        }

        _set_calb_estimate_data(vaaid, &vca_calb);
        
        _update_calb_result_component_data(vaaid, component_data);        
        vw_vca_rev_calibration_result_component_sync_data(g_caliresult_fixed);
    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_caliresult_fixed);
        nfui_signal_emit(g_caliresult_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_caliresult_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationresult_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_caliadrz_fixed);
        nfui_signal_emit(g_caliadrz_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_caliadrz_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_calibrationresult_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
   
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
	    component_data->preview.rule_mode = 0;
	    vw_vca_rev_video_component_sync_preview(g_video_fixed);
    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_rulesetup_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_rulesetup_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;    
   
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_calisetup_fixed);
        nfui_signal_emit(g_calisetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_calisetup_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_rulesetup_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *next_fixed = NULL;
        gint rule_idx = -1;

	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (component_data->use_zone == 1) 
        {

            rule_idx = _get_new_line_idx();

            if (rule_idx == -1) 
            {
                nftool_mbox(g_curwnd, "NOTICE", "You can set up to 16 rules.", NFTOOL_MB_OK);
                next_fixed = g_option_fixed;
            }
            else if (component_data->rule_type == RTYPE_LINE) 
            {
                component_data->line.active = 1;
                _get_default_line_name(rule_idx, component_data->line.name);
                _get_default_line_color(rule_idx, &component_data->line.display_color_idx);
            
                _add_vaa_line(rule_idx, component_data);
                _set_vaa_line_shape(rule_idx, component_data);
                _set_vaa_line_conf(rule_idx, component_data);
                _unset_focus_all_rule();
                _set_focus_vaa_line(rule_idx);
                vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);

                next_fixed = g_ruleline_fixed;
            }
            else if (component_data->rule_type == RTYPE_AREA) 
            {
                component_data->area.active = 1;
                _get_default_area_name(rule_idx, component_data->area.name);
                _get_default_area_color(rule_idx, &component_data->area.display_color_idx);
            
                _add_vaa_area(rule_idx, component_data);
                _set_vaa_area_shape(rule_idx, component_data);
                _set_vaa_area_conf(rule_idx, component_data);
                _unset_focus_all_rule();
                _set_focus_vaa_area(rule_idx);                
                vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);

                next_fixed = g_rulearea_fixed;
            }
        }
        else
        {
            next_fixed = g_option_fixed;
        }

        _begin_paint_component(obj->parent);   
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        
        nfui_nfobject_show(next_fixed);
        nfui_signal_emit(next_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(next_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_ruleline_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);        

        _delete_vaa_line(component_data);
        
        _set_default_line_component_data(component_data);
        _set_default_area_component_data(component_data);   
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);
    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_rulesetup_fixed);        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_ruleline_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gint cntr_idx;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if ((component_data->line.forward == 0) && (component_data->line.reverse == 0))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        if (component_data->zone_id != -1)
            _set_vaa_line_conf(component_data->zone_id,component_data);
            
        _set_vaa_counter_list(component_data);
        component_data->use_counter = 0;
        _set_default_counter_component_data(component_data);
        vw_vca_rev_counter_component_sync_data(g_counter_fixed);
        
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_counter_fixed);
        nfui_signal_emit(g_counter_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_counter_fixed);
        g_counter_prevfixed = obj->parent;        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_rulearea_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        _delete_vaa_area(component_data);

        _set_default_line_component_data(component_data);
        _set_default_area_component_data(component_data);   
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);
    
        _begin_paint_component(obj->parent);   
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_rulesetup_fixed);        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_rulearea_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gint cntr_idx;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if ((component_data->area.enter == 0) && (component_data->area.exit == 0) && \ 
            (component_data->area.removed == 0) && (component_data->area.loitering == 0) && (component_data->area.stopped == 0))
        {

            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        if (component_data->zone_id != -1)
            _set_vaa_area_conf(component_data->zone_id,component_data);

        _set_vaa_counter_list(component_data);
        component_data->use_counter = 0;
        _set_default_counter_component_data(component_data);
        vw_vca_rev_counter_component_sync_data(g_counter_fixed);

        _begin_paint_component(obj->parent);
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_counter_fixed);
        nfui_signal_emit(g_counter_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();
           
        _set_title_showfixed(g_counter_fixed);
        g_counter_prevfixed = obj->parent;
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_counter_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        _delete_vaa_counter(component_data);
        _set_default_counter_component_data(component_data);   
    
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_counter_prevfixed);
        nfui_signal_emit(g_counter_prevfixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_counter_prevfixed);        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);      
    }

    return FALSE;
}

static gboolean post_counter_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        if (component_data->use_counter == 1)
        {
            component_data->counter.up_source = component_data->zone_id;
            component_data->counter.down_source = -1;
        }
        
        if (component_data->cntr_id != -1)
        {
            _set_vaa_counter_conf(component_data->cntr_id, component_data);
        }

        component_data->rule_type = 0;
        component_data->zone_id = -1;
        component_data->cntr_id = -1;        

        _set_default_line_component_data(component_data);
        _set_default_area_component_data(component_data);
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);

        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();

        _set_title_showfixed(g_rulesetup_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_counter_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (component_data->use_counter == 1)
        {
            component_data->counter.up_source = component_data->zone_id;
            component_data->counter.down_source = -1;
        }
        
        if (component_data->cntr_id != -1)
        {
            _set_vaa_counter_conf(component_data->cntr_id, component_data);        
        }
        
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_option_fixed);
        nfui_signal_emit(g_option_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();
        
        _set_title_showfixed(g_option_fixed);
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_option_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();
        
        _set_title_showfixed(g_rulesetup_fixed);        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);       
    }

    return FALSE;
}

static gboolean post_option_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;
		mb_type ret = -1;
		gint cam_change;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        _set_vaa_prop(component_data);
        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaaid = vaa_get_vaaid(g_cur_ch);
        vaa_itx_disable_strule(vaaid);
    
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);	
	}
	
	return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//







////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_vca_rev_wizard_popup_open(NFOBJECT *parent, gint ch)
{
    NFOBJECT *win;
    NFOBJECT *fixed;   
    NFOBJECT *obj;
       
    NFOBJECT *main_fixed;

    VAAID vaaid;
    VCA_COMPONENT_DATA_T *component_data;	

    gint pos_x, pos_y;
    guint opt = 0;
    gint i;

    gchar strBuf[128];

    g_prevwnd = parent;
    g_cur_ch = ch;    
    
    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_enable_strule(vaaid);
    
    _unset_focus_all_rule();

    memset(&g_pre_rule_prop, 0x00, sizeof(ITX_VARULE_PROP));
    memset(g_pre_zone_conf, 0x00, sizeof(ITX_VAZONE_CONF) * 16);
    memset(g_pre_zone_shape, 0x00, sizeof(ITX_VAZONE_SHAPE) * 16);
    memset(g_pre_cntr_conf, 0x00, sizeof(ITX_VACNTR_CONF) * 16);
    memset(g_pre_cntr_shape, 0x00, sizeof(ITX_VACNTR_SHAPE) * 16);
    memset(g_pre_calb_conf, 0x00, sizeof(ITX_VACALB_CONF) * 32);
    memset(g_pre_calb_shape, 0x00, sizeof(ITX_VACALB_SHAPE) * 32);
    memset(&g_pre_calb_result, 0x00, sizeof(ITX_VACALB_RESULT));

    vaa_itx_get_rule_prop(vaaid, &g_pre_rule_prop);

    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
        vaa_itx_get_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
        vaa_itx_get_cntr_conf(vaaid, i, &g_pre_cntr_conf[i]);
        vaa_itx_get_cntr_shape(vaaid, i, &g_pre_cntr_shape[i]);        
    }    

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &g_pre_calb_conf[i]);
        vaa_itx_get_calb_shape(vaaid, i, &g_pre_calb_shape[i]);
    }    

    vaa_itx_get_calb_result(vaaid, &g_pre_calb_result);
    
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-WIN_WIDTH)/2, (1080-WIN_HEIGHT)/2, WIN_WIDTH, WIN_HEIGHT);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_regi_post_event_callback(win, post_main_wnd_event_handler);
    g_curwnd = win;

    _init_component_data(win, ch);
    _init_component_action(win, ch);

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);
    main_fixed = fixed;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA SETUP WIZARD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 360, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s1 - %s %s", lookup_string("STEP"), lookup_string("UNITS SETUP"), "(1/1)");
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 80);
    g_steptitle_obj = obj;


// VIDEO FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 16, 60+(WIN_HEIGHT-60-VIDEO_COMPONENT_HEIGHT)/2);
    g_video_fixed = fixed;

    vw_vca_rev_video_component_open(fixed, 0);
    _set_component_video_fixed(win, fixed);
    

// UNIT SETUP FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);
    g_unitsetup_fixed = fixed;
   
    opt = (1 << OPT_UNIT_SETUP_HELP);    
    vw_vca_rev_unit_setup_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_unitsetup_nextbutton_event_handler);


// CALIBRATION SETUP FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);
    g_calisetup_fixed = fixed;

    opt = (1 << OPT_CALIBRATION_SETUP_HELP);    
    vw_vca_rev_calibration_setup_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_calibrationsetup_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_calibrationsetup_nextbutton_event_handler);


// CALIBRATION ADRZ FIXED (ADD, DELETE, RESET, ZOOM)
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_calb_adrz_fixed_event_handler);
    g_caliadrz_fixed = fixed;

    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_CALB_ID);
    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_NONE_ID);
   
    opt = 0;
    vw_vca_rev_calibration_adrz_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_calibrationadrz_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("ESTIMATE", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_calibrationadrz_estimatebutton_event_handler);


// CALIBRATION RESULT FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);
    g_caliresult_fixed = fixed;

    opt = 0;
    vw_vca_rev_calibration_result_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_calibrationresult_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_calibrationresult_nextbutton_event_handler);


// RULE SETUP FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);	
    g_rulesetup_fixed = fixed;

    opt = (1 << OPT_RULE_SETUP_NONE);
    opt |= (1 << OPT_RULE_SETUP_LINE);
    opt |= (1 << OPT_RULE_SETUP_AREA);
    opt |= (0 << OPT_RULE_SETUP_COUNTER);
    opt |= (1 << OPT_RULE_SETUP_HELP);
    vw_vca_rev_rule_setup_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_rulesetup_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_rulesetup_nextbutton_event_handler);
	

// RULE LINE FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);		
    g_ruleline_fixed = fixed;
    
    opt = (0 << OPT_LINE_ACTIVE_CHECK);
    opt |= (1 << OPT_LINE_SETUP_HELP);
    vw_vca_rev_rule_line_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_ruleline_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_ruleline_nextbutton_event_handler);
	

// RULE AREA FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);		
    g_rulearea_fixed = fixed;

    opt = (0 << OPT_AREA_ACTIVE_CHECK);
    opt |= (1 << OPT_AREA_SETUP_HELP);
    vw_vca_rev_rule_area_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_rulearea_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_rulearea_nextbutton_event_handler);
	

// COUNTER FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);	
    g_counter_fixed = fixed;

    opt = (1 << OPT_COUNTER_USE_RADIO);
    opt |= (0 << OPT_COUNTER_ACTIVE_CHECK);
    opt |= (0 << OPT_COUNTER_SOURCE_COMBO);
    opt |= (1 << OPT_COUNTER_SETUP_HELP);
    vw_vca_rev_counter_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("ADD RULE", 130);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-90-4-130-4-130, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_counter_addbutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 130);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-90-4-130, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_counter_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 90);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-90, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_counter_nextbutton_event_handler);
	

// OPTION FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);		
    g_option_fixed = fixed;

    opt = 0;
    vw_vca_rev_option_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE WIZARD", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("PREVIOUS", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
	nfui_regi_post_event_callback(obj, post_option_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("OK", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
	nfui_regi_post_event_callback(obj, post_option_okbutton_event_handler);

// ADD MAIN FIXED	
	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(win, VCA_COMPONENT_DATA);

    _get_vaa_calb_data(component_data);       
    _get_vaa_prop_data(component_data);

    vw_vca_rev_video_component_sync_preview(g_video_fixed);    
    vw_vca_rev_video_component_sync_data(g_video_fixed);
    vw_vca_rev_unit_setup_component_sync_data(g_unitsetup_fixed);
    vw_vca_rev_calibration_setup_component_sync_data(g_calisetup_fixed);
    vw_vca_rev_calibration_adrz_component_sync_data(g_caliadrz_fixed);
    vw_vca_rev_calibration_result_component_sync_data(g_caliresult_fixed);
    vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);
    vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);
    vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);
    vw_vca_rev_counter_component_sync_data(g_counter_fixed);
    vw_vca_rev_option_component_sync_data(g_option_fixed);

    g_timeout_add(1000, _init_vca_rule, 0);
    
    nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());

    gtk_main();    

    nfui_page_close(PGID_POPUPWND, win);

    return 0;
}

