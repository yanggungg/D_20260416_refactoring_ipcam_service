/*
 * vw_dvabx_addrule_popup.c
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
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "vw_vkeyboard.h"

#include "dvaa.h"
#include "dvaa_itx.h"

#include "vw_dit_dva.h"
#include "vw_dvabx_component.h"
#include "vw_dvabx_plateno_addrule_popup.h"
#include "vw_dvabx_group_filter_edit_popup.h"


#define WIN_WIDTH                   (VIDEO_COMPONENT_WIDTH+32)
#define WIN_HEIGHT                  (VIDEO_COMPONENT_HEIGHT+360)



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
static NFOBJECT *g_name_label;
static NFOBJECT *g_rmode_combo = 0;
static NFOBJECT *g_filter_label;
static NFOBJECT *g_matching_combo;

static gint g_pre_use_zone[16];
static lpr_rule g_pre_lpr_rule;
static gint g_edit_rule_idx = -1;

static gint g_register_group_id[MAX_AIBOX_DB_GROUP_SIZE];
static gint g_cur_ch = 0;
static guint g_draw_tid = 0;
static guint changed_data = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _begin_paint_component(NFOBJECT *component_fixed);
static gint _end_paint_component();


static gint _get_zone_count()
{   
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;    
    gint i, count = 0;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &conf);
        if (conf.use_zone) count++;
    }

    return count;
}

static gint _get_new_line_idx()
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;    
    gint i;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &conf);
        if (conf.use_zone == 0) return i;
    }

    return -1;
}

static gint _get_new_area_idx()
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;    
    gint i;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    
    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &conf);
        if (conf.use_zone == 0) return i;
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

static gint _add_dvaa_line(gint rule_idx)
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    dvaa_itx_plateno_add_zone_line_default_template(dvaaid, rule_idx);

    dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);   
    conf.focus = 1;
    dvaa_itx_plateno_set_zone_conf(dvaaid, rule_idx, &conf);        
    return 0;
}

static gint _add_dvaa_area(gint rule_idx)
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    dvaa_itx_plateno_add_zone_area_default_template(dvaaid, rule_idx);

    dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);   
    conf.focus = 1;
    dvaa_itx_plateno_set_zone_conf(dvaaid, rule_idx, &conf);            
    return 0;
}

static gint _set_dvaa_conf_triggerid(gint rule_idx, guint id)
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);

    dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);   
    conf.triggerid = id;
    dvaa_itx_plateno_set_zone_conf(dvaaid, rule_idx, &conf);            
    return 0;
}

static gint _get_dvaa_conf_triggerid(gint rule_idx)
{
    DVAAID dvaaid;
    ITX_LPRZONE_CONF conf;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);

    dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);   
    return conf.triggerid;
}

static gint _set_dvaa_shape_name(gint rule_idx, gchar *name)
{
    DVAAID dvaaid;
    ITX_LPRZONE_SHAPE shape;

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    
    dvaa_itx_plateno_get_zone_shape(dvaaid, rule_idx, &shape);
    snprintf(shape.name, sizeof(shape.name)-1, "%s", name);
    dvaa_itx_plateno_set_zone_shape(dvaaid, rule_idx, &shape);
    return 0;
}

static gint _set_group_filter_string(lpr_group_info *group_list, gint group_len, lpr_rule *dev_rule)
{
    gchar strBuf[1024];
    gint i, j;

    memset(strBuf, 0x00, sizeof(strBuf));
    
    if (dev_rule->group_size == 0)
    {
        snprintf(strBuf, sizeof(strBuf)-1, "%s", lookup_string("Registered in any group"));
        nfui_nflabel_set_text((NFLABEL*)g_filter_label, strBuf);
        return 0;
    }

    for (i = 0; i < dev_rule->group_size; i++)
    {
        for (j = 0; j < group_len; j++)
        {
            if (dev_rule->group_id_list[i] == group_list[j].id)
            {
                if (i == 0) snprintf(strBuf, sizeof(strBuf)-1, "%s", group_list[j].name);
                else snprintf(strBuf+strlen(strBuf), sizeof(strBuf)+strlen(strBuf)-1, ",%s", group_list[j].name);
            }
        }
    }
    nfui_nflabel_set_text((NFLABEL*)g_filter_label, strBuf);
    return 0;
}

static gint _init_component_data(NFOBJECT *win, gint ch, guint aibox_addr, gchar *algorithm)
{
    DVAAID dvaaid;
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    
    dvaaid = dvaa_get_dvaaid(ch);
    
    component_data = imalloc(sizeof(DVA_COMPONENT_DATA_T));
         
    component_data->act_capable = 1;
    component_data->act_license = 1;
    component_data->aibox_addr = aibox_addr;
    snprintf(component_data->algorithm_value, sizeof(component_data->algorithm_value), "%s", algorithm);
    snprintf(component_data->algorithm_type, sizeof(component_data->algorithm_type), "%s", ALGOTYPE_PLATENO);

    component_data->preview.ch = ch;
         
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
 
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;
    
    nfui_nfobject_set_alloc_data(win, DVA_COMPONENT_DATA, component_data);
 
    return 0;
}

static gint _set_component_video_fixed(NFOBJECT *top, NFOBJECT *video_fixed)
{
    DVA_COMPONENT_DATA_T *component_data;

    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gboolean _draw_vca_rule(gpointer data)
{
    vw_dvabx_video_component_sync_data(g_video_fixed);
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
        DVAAID dvaaid;
        ITX_LPRZONE_CONF conf;
        gint i;

        dvaaid = dvaa_get_dvaaid(g_cur_ch);

        for (i = 0; i < 16; i++)
        {
            dvaa_itx_plateno_get_zone_conf(dvaaid, i, &conf);
            conf.use_zone = g_pre_use_zone[i];
            conf.focus = 0;
            dvaa_itx_plateno_set_zone_conf(dvaaid, i, &conf);
        }

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

static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    DVA_COMPONENT_DATA_T *component_data;   

    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *strBuf = NULL;
    guint x, y;

    guint trigger_id;
    lpr_rule dev_rule;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        if (kpid == KEYPAD_ENTER)
        {
            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }
            
        strBuf = VirtualKey_Open(top, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_ITXSTYLE_TITLE);
        if(!strBuf) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
        nf_api_get_lpr_rule_by_id(component_data->aibox_addr, trigger_id, &dev_rule);
        snprintf(dev_rule.name, sizeof(dev_rule.name)-1, "%s", strBuf);
        nf_api_modify_rule(component_data->aibox_addr, &dev_rule);

        _set_dvaa_shape_name(g_edit_rule_idx, strBuf);
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        ifree(strBuf);
        strBuf = NULL;
    }

    return FALSE;
}

static gboolean post_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        NFOBJECT *top;    
        DVA_COMPONENT_DATA_T *component_data;  

        guint trigger_id;
        lpr_rule dev_rule;

        gint idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
        nf_api_get_lpr_rule_by_id(component_data->aibox_addr, trigger_id, &dev_rule);
        dev_rule.rmode = idx;
        nf_api_modify_rule(component_data->aibox_addr, &dev_rule);

        if (idx == 1) {
            nfui_nfobject_enable(g_filter_label);
            nfui_nfobject_enable(g_matching_combo);
        }
        else {
            nfui_nfobject_disable(g_filter_label);
            nfui_nfobject_disable(g_matching_combo);
        }
        nfui_signal_emit(g_filter_label, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_matching_combo, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_filter_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    DVA_COMPONENT_DATA_T *component_data;

    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    guint trigger_id;
    lpr_rule dev_rule;
    lpr_group_info *group_list = 0;
    gint group_len = 0;
    gint i;

    gchar devgroup_name[MAX_AIBOX_DB_GROUP_SIZE][255] = {0, };
    gint devgroup_id[MAX_AIBOX_DB_GROUP_SIZE] = {0, };
    gint register_group_id[MAX_AIBOX_DB_GROUP_SIZE] = {0, };

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
        nf_api_get_lpr_rule_by_id(component_data->aibox_addr, trigger_id, &dev_rule);

        nf_api_lpr_group_list_get(component_data->aibox_addr, &group_list, &group_len);
        for (i = 0; i < group_len; i++)
        {
            snprintf(devgroup_name[i], sizeof(devgroup_name[i])-1, "%s", group_list[i].name);
            devgroup_id[i] = group_list[i].id;
        }
        memcpy(register_group_id, dev_rule.group_id_list, sizeof(int)*MAX_AIBOX_DB_GROUP_SIZE);

        vw_dvabx_group_filter_edit_popup_open(top, devgroup_name, devgroup_id, register_group_id, dev_rule.group_size);

        memset(dev_rule.group_id_list, 0x00, sizeof(int)*MAX_AIBOX_DB_GROUP_SIZE);
        dev_rule.group_size = 0;

        for (i = 0; i < MAX_AIBOX_DB_GROUP_SIZE; i++)
        {
            if (register_group_id[i]) {
                dev_rule.group_id_list[i] = register_group_id[i];
                dev_rule.group_size++;
            }
            else break;
        }
        nf_api_modify_rule(component_data->aibox_addr, &dev_rule);

        _set_group_filter_string(group_list, group_len, &dev_rule);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        if (group_list) free(group_list);
        group_list = 0;
    }

    return FALSE;
}

static gboolean post_matching_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        NFOBJECT *top;    
        DVA_COMPONENT_DATA_T *component_data;  

        guint trigger_id;
        lpr_rule dev_rule;

        gint idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
        nf_api_get_lpr_rule_by_id(component_data->aibox_addr, trigger_id, &dev_rule);
        dev_rule.policy = idx;
        nf_api_modify_rule(component_data->aibox_addr, &dev_rule);        
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
        ITX_LPRZONE_SHAPE shape;

        guint trigger_id;
        lpr_rule dev_rule;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        dvaaid = dvaa_get_dvaaid(g_cur_ch);

        dvaa_itx_plateno_get_zone_shape(dvaaid, g_edit_rule_idx, &shape);

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
        nf_api_get_lpr_rule_by_id(component_data->aibox_addr, trigger_id, &dev_rule);
        for (i = 0; i < shape.ptcnt; i++) {
            dev_rule.zone[i].x = (float)shape.pt[i].x/3840.0;
            dev_rule.zone[i].y = (float)shape.pt[i].y/2160.0;
        }
        dev_rule.zone_size = shape.ptcnt;
        nf_api_modify_rule(component_data->aibox_addr, &dev_rule);

        nfui_nfobject_destroy(top);  
    }
    
    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;

        guint trigger_id;
        gint i;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        dvaaid = dvaa_get_dvaaid(g_cur_ch);

        if (g_pre_lpr_rule.trigger_id == 0)
        {
            trigger_id = _get_dvaa_conf_triggerid(g_edit_rule_idx);
            nf_api_delete_rule(component_data->aibox_addr, trigger_id, RULE_TYPE_LPR);

            g_pre_use_zone[g_edit_rule_idx] = 0;
        }
        else
        {
            nf_api_modify_rule(component_data->aibox_addr, &g_pre_lpr_rule);
        }

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

gint vw_dvabx_plateno_addrule_popup_open(NFOBJECT *parent, gint ch, guint aibox_ipaddr, gchar *algorithm, gint rule_idx)
{
    NFOBJECT *win;
    NFOBJECT *fixed;   
    NFOBJECT *obj;
       
    NFOBJECT *main_fixed;

    DVAAID dvaaid;
    ITX_LPRRULE_PROP prop;
    ITX_LPRZONE_CONF conf;
    ITX_LPRZONE_SHAPE shape;

    gint pos_x, pos_y;
    gint opt = 0;
    gint i;

    gint new_rule_idx;
    gchar new_rule_name[32];

    lpr_group_info *group_list = 0;
    gint group_len = 0;
    lpr_rule dev_rule;
    guint trigger_id;

    memset(g_pre_use_zone, 0x00, sizeof(int)*16);
    memset(&g_pre_lpr_rule, 0x00, sizeof(lpr_rule));
    g_cur_ch = ch;
    g_edit_rule_idx = rule_idx;
    

    dvaaid = dvaa_get_dvaaid(g_cur_ch);
    dvaa_itx_plateno_get_rule_prop(dvaaid, &prop);

    if (rule_idx == -1)
    {
        new_rule_idx = _get_new_area_idx();
        _add_dvaa_area(new_rule_idx);

        memset(new_rule_name, 0x00, sizeof(new_rule_name));
        _get_default_area_name(new_rule_idx, new_rule_name);
        _set_dvaa_shape_name(new_rule_idx, new_rule_name);
        g_edit_rule_idx = new_rule_idx;

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        dev_rule.type = RULE_TYPE_LPR;
        dev_rule.ch = ch;
        dev_rule.aibox_ch = ch;
        dev_rule.rmode = LPR_EVT_MODE_ALL;
        dev_rule.policy = LPR_EVT_POLICY_VERYHIGH;

        dvaa_itx_plateno_get_zone_shape(dvaaid, new_rule_idx, &shape);

        snprintf(dev_rule.name, sizeof(dev_rule.name)-1, "%s", shape.name);

        for (i = 0; i < 4; i++) {
            dev_rule.zone[i].x = (float)shape.pt[i].x/3840.0;
            dev_rule.zone[i].y = (float)shape.pt[i].y/2160.0;
        }
        dev_rule.zone_size = 4;

        trigger_id = nf_api_add_rule(aibox_ipaddr, &dev_rule);
        _set_dvaa_conf_triggerid(new_rule_idx, trigger_id);
    }
    else
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);   
        conf.focus = 1;
        dvaa_itx_plateno_set_zone_conf(dvaaid, rule_idx, &conf);         

        memset(&dev_rule, 0x00, sizeof(lpr_rule));
        trigger_id = _get_dvaa_conf_triggerid(rule_idx);
        nf_api_get_lpr_rule_by_id(aibox_ipaddr, trigger_id, &dev_rule);
        memcpy(&g_pre_lpr_rule, &dev_rule, sizeof(lpr_rule));
    }

    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &conf);   
        g_pre_use_zone[i] = conf.use_zone;
        if (i != g_edit_rule_idx) conf.use_zone = 0;
        dvaa_itx_plateno_set_zone_conf(dvaaid, i, &conf);
    }


	win = nftool_create_popup_window(parent, (1920-WIN_WIDTH)/2, (1080-WIN_HEIGHT)/2, WIN_WIDTH, WIN_HEIGHT, "", TRUE);
    if (rule_idx == -1) nfui_nfwindow_set_title((NFWINDOW*)win, "ADD RULE");
    else nfui_nfwindow_set_title((NFWINDOW*)win, "EDIT RULE");
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);
	g_curwnd = (NFWINDOW*)win;

	main_fixed = ((NFWINDOW*)win)->child;

    _init_component_data(win, ch, aibox_ipaddr, algorithm);

    pos_x = 16;
    pos_y = 60;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(dev_rule.name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 420, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+280, pos_y);
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    g_name_label = obj;

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RECOGNITION MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 420, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+280, pos_y);
    nfui_regi_post_event_callback(obj, post_mode_combo_event_handler);
    g_rmode_combo = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "All Recognized LPs");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "LPs in Groups");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "Unregistered LP");

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, dev_rule.rmode);

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP FILTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 420, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+280, pos_y);    
    nfui_regi_post_event_callback(obj, post_filter_label_event_handler);
    g_filter_label = obj;

    nf_api_lpr_group_list_get(aibox_ipaddr, &group_list, &group_len);
    _set_group_filter_string(group_list, group_len, &dev_rule);

    if (group_list) free(group_list);
    group_list = 0;

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MATCHING POLICY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 420, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+280, pos_y);
    nfui_regi_post_event_callback(obj, post_matching_combo_event_handler);
    g_matching_combo = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "Very High");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "High");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "Normal");

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, dev_rule.policy);

    if (dev_rule.rmode != LPR_EVT_MODE_IN_GROUP) 
    {
        nfui_nfobject_disable(g_filter_label);
        nfui_nfobject_disable(g_matching_combo);
    }

    pos_y += 41;

// VIDEO FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, pos_x, WIN_HEIGHT-VIDEO_COMPONENT_HEIGHT-79);
    g_video_fixed = fixed;

    vw_dvabx_video_component_open(fixed, 0);
    _set_component_video_fixed(win, fixed);


// RULE SETUP FIXED
	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_WIDTH/2-174-6, WIN_HEIGHT-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_WIDTH/2+6, WIN_HEIGHT-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
   
// ADD MAIN FIXED   
    nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);

    vw_dvabx_video_component_sync_preview(g_video_fixed);    
    vw_dvabx_video_component_sync_data(g_video_fixed);

    g_timeout_add(1000, _init_vca_rule, 0);

    nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());

    gtk_main();    

    nfui_page_close(PGID_POPUPWND, win);

    return 0;
}
