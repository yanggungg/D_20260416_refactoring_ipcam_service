/*
 * vw_vca_rev_addrule_popup.c
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
#include "vw_vca_rev_addrule_popup.h"


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

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_rulesetup_fixed = 0;
static NFOBJECT *g_ruleline_fixed = 0;
static NFOBJECT *g_rulearea_fixed = 0;
static NFOBJECT *g_counter_fixed = 0;
static NFOBJECT *g_line_add_button = 0;
static NFOBJECT *g_area_add_button = 0;
static NFOBJECT *g_counter_add_button = 0;

static gint g_cur_ch = 0;
static guint g_draw_tid = 0;
static guint changed_data = 0;

static ITX_VAZONE_CONF g_pre_zone_conf[16];
static ITX_VAZONE_SHAPE g_pre_zone_shape[16];

static ITX_VACNTR_CONF g_pre_cntr_conf[16];
static ITX_VACNTR_SHAPE g_pre_cntr_shape[16];



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _get_zone_count()
{   
    VAAID vaaid;
    ITX_VAZONE_CONF conf;    
    gint i, count = 0;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &conf);
        if (conf.use_zone) count++;
    }

    return count;
}

static gint _get_counter_count()
{   
    VAAID vaaid;
    ITX_VACNTR_CONF conf;    
    gint i, count = 0;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_cntr_conf(vaaid, i, &conf);
        if (conf.use_cntr) count++;
    }

    return count;
}

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

static gint _set_vaa_line_shape(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(shape.name, component_data->line.name);
    shape.color_idx = component_data->line.display_color_idx;
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

static gint _get_vaa_line_component_data(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(component_data->line.name, shape.name);
    component_data->line.display_color_idx = shape.color_idx;

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    component_data->rule_type = RTYPE_LINE;    
    component_data->zone_id = conf.zoneid;
    component_data->line.active = conf.active;
    
    component_data->line.forward = conf.forward;
    component_data->line.reverse = conf.reverse;   

    component_data->line.use_filter_color = conf.use_filter_color;
    component_data->line.filter_color_idx = conf.filter_color_idx;
    component_data->line.filter_color_percnt = conf.filter_color_prct;

    component_data->line.use_filter_size = conf.use_filter_size;
    component_data->line.filter_min_size_w = conf.filter_width_from+1;
    component_data->line.filter_max_size_w = conf.filter_width_to+1;
    component_data->line.filter_min_size_h = conf.filter_height_from+1;
    component_data->line.filter_max_size_h = conf.filter_height_to+1;

    component_data->line.use_filter_speed = conf.use_filter_speed;
    component_data->line.filter_min_speed = conf.filter_speed_from;
    component_data->line.filter_max_speed = conf.filter_speed_to;
 
    return 0;
}

static gint _set_vaa_area_shape(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);
    
    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(shape.name, component_data->area.name);
    shape.color_idx = component_data->area.display_color_idx;
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

static gint _get_vaa_area_component_data(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_zone_shape(vaaid, rule_idx, &shape);
    strcpy(component_data->area.name, shape.name);
    component_data->area.display_color_idx = shape.color_idx;

    vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);   
    component_data->rule_type = RTYPE_AREA;        
    component_data->zone_id = conf.zoneid;    
    component_data->area.active = conf.active;

    component_data->area.enter = conf.enter;
    component_data->area.exit = conf.exit;
    component_data->area.removed = conf.removed;
    component_data->area.removed_val = conf.cfg_remove_time;
    component_data->area.loitering = conf.loitering;
    component_data->area.loitering_val = conf.cfg_loiter_time;
    component_data->area.stopped = conf.stopped; 
    component_data->area.stopped_val = conf.cfg_stop_time;
    
    component_data->area.use_filter_color = conf.use_filter_color;
    component_data->area.filter_color_idx = conf.filter_color_idx;
    component_data->area.filter_color_percnt = conf.filter_color_prct;

    component_data->area.use_filter_size = conf.use_filter_size;
    component_data->area.filter_min_size_w = conf.filter_width_from+1;
    component_data->area.filter_max_size_w = conf.filter_width_to+1;
    component_data->area.filter_min_size_h = conf.filter_height_from+1;
    component_data->area.filter_max_size_h = conf.filter_height_to+1;

    component_data->area.use_filter_speed = conf.use_filter_speed;
    component_data->area.filter_min_speed = conf.filter_speed_from;
    component_data->area.filter_max_speed = conf.filter_speed_to;
 
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

static gint _get_vaa_counter_component_data(gint rule_idx, VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    ITX_VACNTR_SHAPE shape;
    ITX_VACNTR_CONF conf;

    vaaid = vaa_get_vaaid(g_cur_ch);

    vaa_itx_get_cntr_shape(vaaid, rule_idx, &shape);
    strcpy(component_data->counter.name, shape.name);
    component_data->counter.display_color_idx = shape.color_idx;
    
    vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);   
    component_data->rule_type = RTYPE_COUNTER;       
    component_data->cntr_id = conf.cntrid;    
    component_data->counter.active = conf.active;
    
    component_data->counter.use_counter_event = conf.use_counter_event;
    component_data->counter.counter_event_val = conf.counter_event_val;
    component_data->counter.use_reset_value = conf.use_reset_value;
    
    component_data->counter.up_source = conf.source_up;
    component_data->counter.down_source = conf.source_down;
 
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

static gint _get_calb_result_component_data(VAAID vaaid, VCA_COMPONENT_DATA_T *component_data)
{
    ITX_VACALB_RESULT res;    

    vaa_itx_get_calb_result(vaaid, &res);

    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;    
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

static gint _set_default_component_data(VCA_COMPONENT_DATA_T *component_data)
{
    component_data->check_config = 0;
    component_data->zone_id = -1;
    component_data->cntr_id = -1;

    component_data->rule_type = RTYPE_LINE;
    component_data->use_zone = 1;
    component_data->use_counter = 1;
   
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
    component_data->line.filter_max_size_w = 10;
    component_data->line.filter_min_size_h = 0;
    component_data->line.filter_max_size_h = 10;
    component_data->line.use_filter_speed = 0;
    component_data->line.filter_min_speed = 0;
    component_data->line.filter_max_speed = 0;

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
    component_data->area.filter_max_size_w = 10;
    component_data->area.filter_min_size_h = 0;
    component_data->area.filter_max_size_h = 10;
    component_data->area.use_filter_speed = 0;
    component_data->area.filter_min_speed = 0;
    component_data->area.filter_max_speed = 0;
    
    component_data->use_counter = 0;
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

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    VAAID vaaid;
    VCA_COMPONENT_DATA_T *component_data;
    gint unit_change ;
    NFOBJECT *top;
    
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

    _set_default_component_data(component_data);    
    _get_calb_result_component_data(vaaid, component_data);
    
    nfui_nfobject_set_alloc_data(win, VCA_COMPONENT_DATA, component_data);
 
    return 0;
}


static gint _init_component_action(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(VCA_COMPONENT_ACTION_T));

    component_action->rule_type_cb = _rule_component_type_cb;
    component_action->rule_act_cb = _rule_component_act_cb;
    
    component_action->line_name_cb = _line_component_name_cb;  
    component_action->line_dis_color_cb = _line_component_dis_color_cb;  
    
    component_action->area_name_cb = _area_component_name_cb;  
    component_action->area_dis_color_cb = _area_component_dis_color_cb;  

    component_action->counter_name_cb = _counter_component_name_cb;  
    component_action->counter_dis_color_cb = _counter_component_dis_color_cb;  

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

static gint _is_changed_data()
{
	if(changed_data){
	    changed_data = 0;
    	return 1;
    }
    else    return 0;
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

        case INFY_VAA_ITX_PRESS_ZONE_ID:
        {
            NFOBJECT *top;
            VCA_COMPONENT_DATA_T *component_data;
        
            VAAID vaaid;
            ITX_VAZONE_CONF conf;
            
            gint rule_idx = ((CMM_MESSAGE_T*)data)->param;
       
            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            if (component_data->rule_type == RTYPE_LINE) 
            {
                if (component_data->zone_id != -1) _set_vaa_line_conf(component_data->zone_id, component_data);
            }
            else if (component_data->rule_type == RTYPE_AREA) 
            {
                if (component_data->zone_id != -1) _set_vaa_area_conf(component_data->zone_id, component_data);
            }
            else if (component_data->rule_type == RTYPE_COUNTER) 
            {
                if (component_data->cntr_id != -1) _set_vaa_counter_conf(component_data->cntr_id, component_data);
            }

            vaaid = vaa_get_vaaid(g_cur_ch);            
            vaa_itx_get_zone_conf(vaaid, rule_idx, &conf);  

            _set_default_component_data(component_data);

            if (conf.type == 0) 
            {
                _get_vaa_line_component_data(rule_idx, component_data);
                vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);

                _begin_paint_component(g_ruleline_fixed);        
                nfui_nfobject_hide(g_rulesetup_fixed);
                nfui_nfobject_hide(g_rulearea_fixed);
                nfui_nfobject_hide(g_counter_fixed);
                nfui_nfobject_show(g_ruleline_fixed);
                nfui_signal_emit(g_ruleline_fixed, GDK_EXPOSE, TRUE);        
                _end_paint_component();
            }
            else
            {
                _get_vaa_area_component_data(rule_idx, component_data);
                vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);

                _begin_paint_component(g_rulearea_fixed);        
                nfui_nfobject_hide(g_rulesetup_fixed);
                nfui_nfobject_hide(g_ruleline_fixed);
                nfui_nfobject_hide(g_counter_fixed);
                nfui_nfobject_show(g_rulearea_fixed);
                nfui_signal_emit(g_rulearea_fixed, GDK_EXPOSE, TRUE);        
                _end_paint_component();                
            }

            nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
        }
        break;
        
        case INFY_VAA_ITX_PRESS_CNTR_ID:
        {
            NFOBJECT *top;
            VCA_COMPONENT_DATA_T *component_data;
        
            VAAID vaaid;
            ITX_VACNTR_CONF conf;
            
            gint rule_idx = ((CMM_MESSAGE_T*)data)->param;
        
            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            if (component_data->rule_type == RTYPE_COUNTER)
            {
                if (component_data->cntr_id == rule_idx) return FALSE;
            }

            if (component_data->rule_type == RTYPE_LINE) 
            {
                if (component_data->zone_id != -1) _set_vaa_line_conf(component_data->zone_id, component_data);
            }
            else if (component_data->rule_type == RTYPE_AREA) 
            {
                if (component_data->zone_id != -1) _set_vaa_area_conf(component_data->zone_id, component_data);
            }
            else if (component_data->rule_type == RTYPE_COUNTER) 
            {
                if (component_data->cntr_id != -1) _set_vaa_counter_conf(component_data->cntr_id, component_data);
            }
            
            vaaid = vaa_get_vaaid(g_cur_ch);            
            vaa_itx_get_cntr_conf(vaaid, rule_idx, &conf);  

            _set_default_component_data(component_data);

            _get_vaa_counter_component_data(rule_idx, component_data);
            _set_vaa_counter_list(component_data);
            vw_vca_rev_counter_component_sync_data(g_counter_fixed);

            _begin_paint_component(g_counter_fixed);        
            nfui_nfobject_hide(g_rulesetup_fixed);
            nfui_nfobject_hide(g_ruleline_fixed);
            nfui_nfobject_hide(g_rulearea_fixed);
            nfui_nfobject_show(g_counter_fixed);
            nfui_signal_emit(g_counter_fixed, GDK_EXPOSE, TRUE);        
            _end_paint_component();
            nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
        }
        break;

        case GDK_DELETE:
        {       
            uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_ZONE_ID);
            uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_CNTR_ID);
        
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

static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *next_fixed;
        gint rule_idx;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (component_data->rule_type == RTYPE_LINE) 
        {
            rule_idx = _get_new_line_idx();

            if (rule_idx == -1) {
                nftool_mbox(g_curwnd, "NOTICE", "You can set up to 16 rules.", NFTOOL_MB_OK);
                return FALSE;
            }

            component_data->check_config = 1;
            _get_default_line_name(rule_idx, component_data->line.name);
            _get_default_line_color(rule_idx, &component_data->line.display_color_idx);
           
            _add_vaa_line(rule_idx, component_data);
            _set_vaa_line_shape(rule_idx, component_data);
            _set_vaa_line_conf(rule_idx, component_data);
            _set_focus_vaa_line(rule_idx);
            vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);

            next_fixed = g_ruleline_fixed;
        }
        else if (component_data->rule_type == RTYPE_AREA) 
        {
            rule_idx = _get_new_area_idx();

            if (rule_idx == -1) {
                nftool_mbox(g_curwnd, "NOTICE", "You can set up to 16 rules.", NFTOOL_MB_OK);
                return FALSE;
            }

            component_data->check_config = 1;
            _get_default_area_name(rule_idx, component_data->area.name);
            _get_default_area_color(rule_idx, &component_data->area.display_color_idx);
        
            _add_vaa_area(rule_idx, component_data);
            _set_vaa_area_shape(rule_idx, component_data);
            _set_vaa_area_conf(rule_idx, component_data);
            _set_focus_vaa_area(rule_idx);            
            vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);

            next_fixed = g_rulearea_fixed;
        }
        else if (component_data->rule_type == RTYPE_COUNTER) 
        {
            rule_idx = _get_new_counter_idx();

            if (rule_idx == -1) {
                nftool_mbox(g_curwnd, "NOTICE", "You can set up to 16 rules.", NFTOOL_MB_OK);
                return FALSE;
            }

            component_data->check_config = 1;
            _get_default_counter_name(rule_idx, component_data->counter.name);
            _get_default_counter_color(rule_idx, &component_data->counter.display_color_idx);
            
            _add_vaa_counter(rule_idx, component_data);
            _set_vaa_counter_shape(rule_idx, component_data);      
            _set_vaa_counter_conf(rule_idx, component_data);
            _set_vaa_counter_list(component_data);
            _set_focus_vaa_counter(rule_idx);
            vw_vca_rev_counter_component_sync_data(g_counter_fixed);

            next_fixed = g_counter_fixed;
        }

        if ((_get_zone_count() == 16) && (_get_counter_count() == 16))
        {
            nfui_nfobject_disable(g_line_add_button);
            nfui_nfobject_disable(g_area_add_button);
            nfui_nfobject_disable(g_counter_add_button);            
            nfui_signal_emit(g_line_add_button, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_area_add_button, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_counter_add_button, GDK_EXPOSE, TRUE);            
        }

        _begin_paint_component(obj->parent);        
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);    
        nfui_nfobject_show(next_fixed);
        nfui_signal_emit(next_fixed, GDK_EXPOSE, TRUE);        
        _end_paint_component();
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
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
        mb_type ret = NFTOOL_MB_CANCEL;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if(_is_changed_data() != 0)
            ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
	    								 NFTOOL_MB_OKCANCEL);
    	if(ret != NFTOOL_MB_OK)
    	{
            vaaid = vaa_get_vaaid(g_cur_ch);

            for (i = 0; i < 16; i++)
            {
                vaa_itx_set_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
                vaa_itx_set_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
                vaa_itx_set_cntr_shape(vaaid, i, &g_pre_cntr_shape[i]);
                vaa_itx_set_cntr_conf(vaaid, i, &g_pre_cntr_conf[i]);
            }    

            component_data->preview.onoff = 0;    
            component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

            vw_vca_rev_video_component_sync_preview(g_video_fixed);
            vw_vca_rev_video_component_expose(g_video_fixed);

            vaa_itx_disable_strule(vaaid);
            nfui_nfobject_destroy(top); 
        }
        else
        {
            component_data->disp_rule.block_update = 0;
            vw_vca_rev_video_component_sync_data(g_video_fixed);

            component_data->preview.onoff = 0;    
            component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
            vw_vca_rev_video_component_sync_preview(g_video_fixed);
            vw_vca_rev_video_component_expose(g_video_fixed);

            vaaid = vaa_get_vaaid(g_cur_ch);
            vaa_itx_disable_strule(vaaid);
            
            nfui_nfobject_destroy(top);  

        }
    }
    
    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);

        for (i = 0; i < 16; i++)
        {
            vaa_itx_set_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
            vaa_itx_set_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
            vaa_itx_set_cntr_shape(vaaid, i, &g_pre_cntr_shape[i]);
            vaa_itx_set_cntr_conf(vaaid, i, &g_pre_cntr_conf[i]);
        }    

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    

        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaa_itx_disable_strule(vaaid);
    
        nfui_nfobject_destroy(top); 
    }
    
    return FALSE;
}

static gboolean post_ruleline_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if ((component_data->line.forward == 0) && (component_data->line.reverse == 0))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        changed_data = 1;
        _set_vaa_line_conf(component_data->zone_id, component_data);
        _unset_focus_vaa_line(component_data->zone_id);
        
        _set_default_component_data(component_data);
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);

        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_ruleline_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if ((component_data->line.forward == 0) && (component_data->line.reverse == 0))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        _set_vaa_line_conf(component_data->zone_id, component_data);

        component_data->disp_rule.block_update = 0;
        vw_vca_rev_video_component_sync_data(g_video_fixed);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaaid = vaa_get_vaaid(g_cur_ch);
        vaa_itx_disable_strule(vaaid);
        
        nfui_nfobject_destroy(top);  
    }
    
    return FALSE;
}

static gboolean post_rulearea_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
       
        if ((component_data->area.enter == 0) && (component_data->area.exit == 0) && \ 
            (component_data->area.removed == 0) && (component_data->area.loitering == 0) && (component_data->area.stopped == 0))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        _set_vaa_area_conf(component_data->zone_id, component_data);
        _unset_focus_vaa_area(component_data->zone_id);
        
        _set_default_component_data(component_data);
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);

        changed_data = 1;
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);        
        _end_paint_component();
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_rulearea_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if ((component_data->area.enter == 0) && (component_data->area.exit == 0) && \ 
            (component_data->area.removed == 0) && (component_data->area.loitering == 0) && (component_data->area.stopped == 0))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        _set_vaa_area_conf(component_data->zone_id, component_data);

        component_data->disp_rule.block_update = 0;
        vw_vca_rev_video_component_sync_data(g_video_fixed);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaaid = vaa_get_vaaid(g_cur_ch);
        vaa_itx_disable_strule(vaaid);
        
        nfui_nfobject_destroy(top);  
    }
    
    return FALSE;
}

static gboolean post_counter_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
        
        _set_vaa_counter_conf(component_data->cntr_id, component_data);
        _unset_focus_vaa_counter(component_data->cntr_id);

        _set_default_component_data(component_data);
        vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);

        changed_data = 1;
        _begin_paint_component(obj->parent);    
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
        nfui_nfobject_show(g_rulesetup_fixed);
        nfui_signal_emit(g_rulesetup_fixed, GDK_EXPOSE, TRUE);
        _end_paint_component();        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_counter_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);   
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
                
        _set_vaa_counter_conf(component_data->cntr_id, component_data);
        
        component_data->disp_rule.block_update = 0;
        vw_vca_rev_video_component_sync_data(g_video_fixed);

        component_data->preview.onoff = 0;    
        component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);

        vaaid = vaa_get_vaaid(g_cur_ch);
        vaa_itx_disable_strule(vaaid);
    
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

gint vw_vca_rev_addrule_popup_open(NFOBJECT *parent, gint ch)
{
    NFOBJECT *win;
    NFOBJECT *fixed;   
    NFOBJECT *obj;
       
    NFOBJECT *main_fixed;

    VAAID vaaid;

    gint pos_x, pos_y;
    gint opt;
    gint i;

    g_cur_ch = ch;
    
    memset(g_pre_zone_conf, 0x00, sizeof(ITX_VAZONE_CONF) * 16);
    memset(g_pre_zone_shape, 0x00, sizeof(ITX_VAZONE_SHAPE) * 16);
    memset(g_pre_cntr_conf, 0x00, sizeof(ITX_VACNTR_CONF) * 16);
    memset(g_pre_cntr_shape, 0x00, sizeof(ITX_VACNTR_SHAPE) * 16);

    vaaid = vaa_get_vaaid(g_cur_ch);
    vaa_itx_enable_strule(vaaid);

    for (i = 0; i < 16; i++)
    {
        vaa_itx_get_zone_conf(vaaid, i, &g_pre_zone_conf[i]);
        vaa_itx_get_zone_shape(vaaid, i, &g_pre_zone_shape[i]);
        vaa_itx_get_cntr_conf(vaaid, i, &g_pre_cntr_conf[i]);
        vaa_itx_get_cntr_shape(vaaid, i, &g_pre_cntr_shape[i]);        
    }    

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

    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_ZONE_ID);
    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_CNTR_ID);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADD RULE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 360, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);


// VIDEO FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 16, 60+(WIN_HEIGHT-60-VIDEO_COMPONENT_HEIGHT)/2);
    g_video_fixed = fixed;

    vw_vca_rev_video_component_open(fixed, 0);
    _set_component_video_fixed(win, fixed);


// RULE SETUP FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);   
    g_rulesetup_fixed = fixed;

    opt = (0 << OPT_RULE_SETUP_NONE);
    opt |= (1 << OPT_RULE_SETUP_LINE);
    opt |= (1 << OPT_RULE_SETUP_AREA);
    opt |= (1 << OPT_RULE_SETUP_COUNTER);
    opt |= (0 << OPT_RULE_SETUP_HELP);
    vw_vca_rev_rule_setup_component_open(fixed, opt);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("NEXT", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);    
    nfui_regi_post_event_callback(obj, post_nextbutton_event_handler);
    

// RULE LINE FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);       
    g_ruleline_fixed = fixed;

    opt = (1 << OPT_LINE_ACTIVE_CHECK);
    opt |= (0 << OPT_LINE_SETUP_HELP);
    vw_vca_rev_rule_line_component_open(fixed, opt);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("ADD RULE", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
    nfui_regi_post_event_callback(obj, post_ruleline_addbutton_event_handler);
    g_line_add_button = obj;    
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("OK", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);  
    nfui_regi_post_event_callback(obj, post_ruleline_okbutton_event_handler);
    

// RULE AREA FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);       
    g_rulearea_fixed = fixed;

    opt = (1 << OPT_AREA_ACTIVE_CHECK);
    opt |= (0 << OPT_AREA_SETUP_HELP);
    vw_vca_rev_rule_area_component_open(fixed, opt);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("ADD RULE", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
    nfui_regi_post_event_callback(obj, post_rulearea_addbutton_event_handler);
    g_area_add_button = obj;    
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("OK", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);  
    nfui_regi_post_event_callback(obj, post_rulearea_okbutton_event_handler);
    

// COUNTER FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT+60);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));  
    nfui_nfobject_hide(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 20+VIDEO_COMPONENT_WIDTH+20, 60);
    nfui_regi_post_event_callback(fixed, post_component_fixed_event_handler);   
    g_counter_fixed = fixed;

    opt = (0 << OPT_COUNTER_USE_RADIO);
    opt |= (1 << OPT_COUNTER_ACTIVE_CHECK);
    opt |= (1 << OPT_COUNTER_SOURCE_COMBO);
    opt |= (0 << OPT_COUNTER_SETUP_HELP);
    vw_vca_rev_counter_component_open(fixed, opt);

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 0, fixed->height-50);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("ADD RULE", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160-4-160, fixed->height-50); 
    nfui_regi_post_event_callback(obj, post_counter_addbutton_event_handler);
    g_counter_add_button = obj;
    
    obj = (NFOBJECT*)nftool_normal_button_create_type1("OK", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-160, fixed->height-50);  
    nfui_regi_post_event_callback(obj, post_counter_okbutton_event_handler);
    
// ADD MAIN FIXED   
    nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);

    vw_vca_rev_video_component_sync_preview(g_video_fixed);    
    vw_vca_rev_video_component_sync_data(g_video_fixed);
    vw_vca_rev_rule_setup_component_sync_data(g_rulesetup_fixed);
    vw_vca_rev_rule_line_component_sync_data(g_ruleline_fixed);
    vw_vca_rev_rule_area_component_sync_data(g_rulearea_fixed);
    vw_vca_rev_counter_component_sync_data(g_counter_fixed);

    g_timeout_add(1000, _init_vca_rule, 0);

    nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());

    gtk_main();    

    nfui_page_close(PGID_POPUPWND, win);

    return 0;
}

