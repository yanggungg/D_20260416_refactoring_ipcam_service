/*
 * vw_dvabx_rule.c
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
#include "objects/nfspinbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_dvabx_prop_internal.h"
#include "vw_dvabx_component.h"
#include "vw_vkeyboard.h"

#include "dvaa.h"
#include "dvaa_itx.h"


/* Rule table. */
#define	_RULE_COLS	    	        2
#define	_RULE_ROWS		            16
#define _RULE_TBL_GAP               1
#define	_RULE_TBL_X	            	10
#define	_RULE_TBL_Y	            	10
#define	_RULE_LABEL_W	        	220+260
#define	_RULE_LABEL_H	        	36


////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct _DVA_SELECT_CALLBACK
{
	DVA_SELECT2_CB_FUNC cb_func;
	gpointer 			user_data;
} DVA_SELECT_CALLBACK;


////////////////////////////////////////////////////////////
//
// private variable
//
enum {
    RULELABEL_ID = 0,
	RULELABEL_NAME,
	RULELABEL_MODE,
	RULELABEL_ALL
};

static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;
static NFOBJECT *g_rulelist_obj = NULL;
static NFOBJECT *g_add_btn = NULL;
static NFOBJECT *g_mod_btn = NULL;
static NFOBJECT *g_del_btn = NULL;
static NFOBJECT *g_del_all_btn = NULL;

static DVA_SELECT_CALLBACK g_select_cb = {0, };



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _update_zone_info(ITX_LPRZONE_CONF *conf, ITX_LPRZONE_SHAPE *shape)
{
    gchar *zone_list[RULELABEL_ALL];
    
    zone_list[RULELABEL_ID] = imalloc(sizeof(gchar)*32);
    g_sprintf(zone_list[RULELABEL_ID], "Z%02d", conf->zoneid);  

    zone_list[RULELABEL_NAME] = imalloc(sizeof(gchar)*32);
    strcpy(zone_list[RULELABEL_NAME], shape->name);

    zone_list[RULELABEL_MODE] = imalloc(sizeof(gchar)*32);

    nfui_listbox_set_text((NFLISTBOX*)g_rulelist_obj, zone_list);
    
    ifree(zone_list[RULELABEL_ID]);
    ifree(zone_list[RULELABEL_MODE]);

    return 0;
}

static gint _get_zone_cnt()
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    
    ITX_LPRZONE_CONF zone_conf;

    DVAAID dvaaid;
    gint i, idx = 0;
    gint cnt = 0;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    for (i = 0; i < 16; i++)
    {  
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
        if (zone_conf.use_zone) cnt++;           
    }

    return cnt;
}

static gint _update_rule_info(gint expose)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;

    ITX_LPRZONE_CONF zone_conf;
    ITX_LPRZONE_SHAPE zone_shape;

    DVAAID dvaaid;
    gint i, idx = 0;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    nfui_listbox_delete_all((NFLISTBOX*)g_rulelist_obj);
    
    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    
    for (i = 0; i < 16; i++)
    {  
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
        dvaa_itx_plateno_get_zone_shape(dvaaid, i, &zone_shape);

        if (zone_conf.use_zone) {
            _update_zone_info(&zone_conf, &zone_shape);
        }
    }    
    
    nfui_signal_emit(g_rulelist_obj, GDK_EXPOSE, TRUE);
    return 0;
}

static gint _sync_aibox_plateno_zone_data()
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

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
    DAL_get_aianalysis_act_data(&analysis_data, component_data->preview.ch);
    if (!analysis_data.dvabox_active) return FALSE;

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    for (i = 0; i < 16; i++)
    {
        dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
        zone_conf.use_zone = 0;
        dvaa_itx_plateno_set_zone_conf(dvaaid, i, &zone_conf);
    }

    nf_api_get_lpr_rules(component_data->aibox_addr, &aibox_rule, &rule_cnt);
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

static gint _open_rule_add_popup(NFOBJECT *video_fixed, DVA_COMPONENT_DATA_T *component_data)
{
    component_data->disp_rule.block_update = 1; 
    vw_dvabx_video_component_sync_data(video_fixed);

    component_data->disable_event = 1;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_dvabx_video_component_sync_preview(video_fixed);
    vw_dvabx_video_component_expose(video_fixed);      

    vw_dvabx_plateno_addrule_popup_open(g_curwnd, component_data->preview.ch, component_data->aibox_addr, component_data->algorithm_value, -1);
    _sync_aibox_plateno_zone_data();
    nfui_user_signal_emit(g_parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
  
    component_data->disable_event = 0;
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    component_data->disp_rule.block_update = 0;
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;        
    vw_dvabx_video_component_sync_preview(video_fixed);    
    vw_dvabx_video_component_expose(video_fixed);

    return 0;
}

static gint _open_rule_modify_popup(NFOBJECT *video_fixed, DVA_COMPONENT_DATA_T *component_data, gint type, gint zone_id)
{
    component_data->disp_rule.block_update = 1; 
    vw_dvabx_video_component_sync_data(video_fixed);

    component_data->disable_event = 1;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_dvabx_video_component_sync_preview(video_fixed);
    vw_dvabx_video_component_expose(video_fixed);      
  
    vw_dvabx_plateno_addrule_popup_open(g_curwnd, component_data->preview.ch, component_data->aibox_addr, component_data->algorithm_value, zone_id);
    _sync_aibox_plateno_zone_data();
    nfui_user_signal_emit(g_parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
  
    component_data->disable_event = 0;
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    component_data->disp_rule.block_update = 0;
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;        
    vw_dvabx_video_component_sync_preview(video_fixed);    
    vw_dvabx_video_component_expose(video_fixed);

    return 0;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_rule_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    gchar *str_rule_id;

    if (evt->type == NFEVENT_LISTBOX_CHANGED) 
    {
        gint list_idx;
        
        list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
        
        if (list_idx != -1) 
        {
            str_rule_id = nfui_listbox_get_focus_text((NFLISTBOX*)obj, RULELABEL_ID);
            if (!str_rule_id) return FALSE;

            if (strpbrk(str_rule_id, "01")) str_rule_id = strpbrk(str_rule_id, "01");

            g_select_cb.cb_func(0, atoi(str_rule_id), g_select_cb.user_data);
            nfui_user_signal_emit(g_mod_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
            nfui_user_signal_emit(g_del_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        }
        else
        {
            g_select_cb.cb_func(-1, -1, g_select_cb.user_data);
            nfui_user_signal_emit(g_mod_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
            nfui_user_signal_emit(g_del_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        }    
    }
    else if (evt->type == GDK_2BUTTON_PRESS)
    {
        str_rule_id = nfui_listbox_get_focus_text((NFLISTBOX*)obj, RULELABEL_ID);
        if (!str_rule_id) return FALSE;

        if (strpbrk(str_rule_id,"01")) str_rule_id = strpbrk(str_rule_id,"01");

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        _open_rule_modify_popup(component_data->video_fixed, component_data, RTYPE_AREA, atoi(str_rule_id));
        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
   
        _update_rule_info(1);
    }        
    else if (evt->type == INFY_DVAA_ITX_PRESS_ZONE_ID)
    {    
        DVAAID dvaaid;
        ITX_LPRZONE_CONF conf;
        
        gint rule_idx = ((CMM_MESSAGE_T*)data)->param;
        gint i, list_cnt;
        gint pre_focus;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->disable_event) return FALSE;
        
        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);            
        dvaa_itx_plateno_get_zone_conf(dvaaid, rule_idx, &conf);  

        list_cnt = nfui_listbox_get_box_count((NFLISTBOX*)obj);

        for (i = 0; i < list_cnt; i++)
        {
            str_rule_id = nfui_listbox_get_text_of_list((NFLISTBOX*)obj, i, RULELABEL_ID);
            if (strpbrk(str_rule_id,"01")) str_rule_id = strpbrk(str_rule_id,"01");

            if (atoi(str_rule_id) == rule_idx) break;
        }
        
        pre_focus = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);
        nfui_listbox_set_focus((NFLISTBOX*)obj, pre_focus, FALSE);        
        nfui_listbox_set_focus((NFLISTBOX*)obj, i, TRUE);
        nfui_user_signal_emit(g_mod_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        nfui_user_signal_emit(g_del_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }
    else if (evt->type == INFY_DVAA_ITX_PRESS_NONE_ID)
    {            
        gint pre_focus;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);       

        if (component_data->disable_event) return FALSE;
        
        pre_focus = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);
        nfui_listbox_set_focus((NFLISTBOX*)obj, pre_focus, FALSE);        
        nfui_user_signal_emit(g_mod_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
        nfui_user_signal_emit(g_del_btn, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }    
    else if(evt->type == INFY_DVAA_ITX_2PRESS_ZONE_ID)
    {
        ITX_LPRZONE_CONF conf;    
        CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T*)data;       
        gint zone_id = pmsg->param;
        DVAAID dvaaid;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);       

        if (component_data->disable_event) return FALSE;

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);    
        dvaa_itx_plateno_get_zone_conf(dvaaid, zone_id, &conf);
            
        _open_rule_modify_popup(component_data->video_fixed, component_data, RTYPE_AREA, zone_id);
        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_NONE_ID);  
        uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_ZONE_ID);
        uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_2PRESS_ZONE_ID);
    }

    return FALSE;
}

static gboolean post_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;

	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        
        _open_rule_add_popup(component_data->video_fixed, component_data);
        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license)
        {
            if (_get_zone_cnt() == 16)
                nfui_nfobject_disable(obj);
            else
                nfui_nfobject_enable(obj);
        }
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}

static gboolean post_modbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    gchar *str_rule_id;
    gint type_idx;
    gint list_idx = -1;

	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        str_rule_id = nfui_listbox_get_focus_text((NFLISTBOX*)g_rulelist_obj, RULELABEL_ID);

        if (!str_rule_id) return FALSE;

        if (strpbrk(str_rule_id, "01")) str_rule_id = strpbrk(str_rule_id, "01");

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        _open_rule_modify_popup(component_data->video_fixed, component_data, RTYPE_AREA, atoi(str_rule_id));
        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license)
        {
            list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
            if (list_idx == -1)
                nfui_nfobject_disable(obj);
            else
                nfui_nfobject_enable(obj);
        }
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}

static gboolean post_delbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    ITX_LPRZONE_CONF zone_conf;
    DVAAID dvaaid;

    gchar *str_rule_id;
    gint list_idx = -1;
    gint i;

	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
        if (list_idx == -1) return FALSE;

        str_rule_id = nfui_listbox_get_focus_text((NFLISTBOX*)g_rulelist_obj, RULELABEL_ID);
        if (!str_rule_id) return FALSE;
        
        if (strpbrk(str_rule_id, "01")) str_rule_id = strpbrk(str_rule_id,"01");
                
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

        dvaa_itx_plateno_get_zone_conf(dvaaid, atoi(str_rule_id), &zone_conf);
        zone_conf.use_zone = 0;
        dvaa_itx_plateno_set_zone_conf(dvaaid, atoi(str_rule_id), &zone_conf);

        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE); 
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license)
        {
            list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
            if (list_idx == -1)
                nfui_nfobject_disable(obj);
            else
                nfui_nfobject_enable(obj);
        }
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}

static gboolean post_del_allbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    ITX_LPRZONE_CONF zone_conf;    
    DVAAID dvaaid;
    gint i;

	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

        for (i = 0; i < 16; i++)
        {
            dvaa_itx_plateno_get_zone_conf(dvaaid, i, &zone_conf);
            zone_conf.use_zone = 0;
            dvaa_itx_plateno_set_zone_conf(dvaaid, i, &zone_conf);
        }

        nfui_user_signal_emit(obj->parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license)
        {
            if (_get_zone_cnt() > 0)
                nfui_nfobject_enable(obj);
            else
                nfui_nfobject_disable(obj);
        }
        else nfui_nfobject_disable(obj);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//
gint _dvabx_plateno_rule_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;
	gint i, j, tbl_x, tbl_y;
    gint btn_w, btn_h;
	gchar *tbl_rule_strcol[RULELABEL_ALL] = {"ID", "NAME", "MODE"};
	guint tbl_rule_w[RULELABEL_ALL] = {60, 180, 260};

    gint opt;

    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);
    g_parent = parent;

    tbl_x = _RULE_TBL_X;
    tbl_y = _RULE_TBL_Y;
	//	RULE TABLE

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  
    
	for(i = 0; i < RULELABEL_ALL; i++)
	{
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tbl_rule_strcol[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 10);
    	nfui_nfobject_set_size(obj, tbl_rule_w[i], _RULE_LABEL_H);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)parent, obj, tbl_x, tbl_y);
    	tbl_x = tbl_x + tbl_rule_w[i];
    }
    
    tbl_x = _RULE_TBL_X;
    tbl_y = _RULE_TBL_Y + _RULE_LABEL_H;

    obj = nfui_listbox_new(RULELABEL_ALL, tbl_rule_w, _RULE_LABEL_H);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);   
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RULELABEL_ID, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RULELABEL_NAME, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RULELABEL_MODE, NFALIGN_CENTER);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, _RULE_LABEL_W + 25, _RULE_LABEL_H*_RULE_ROWS);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, tbl_x, tbl_y);  
    nfui_regi_post_event_callback(obj, post_rule_list_event_cb);  
    g_rulelist_obj = obj;

    uxm_reg_imsg_event(obj, INFY_DVAA_ITX_PRESS_NONE_ID);   
    uxm_reg_imsg_event(obj, INFY_DVAA_ITX_PRESS_ZONE_ID);
	uxm_reg_imsg_event(obj, INFY_DVAA_ITX_2PRESS_ZONE_ID);

	obj= (NFOBJECT*)nftool_normal_button_create_popup_type1("ADD", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_addbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 10, parent->height-40*3-10-15);
    g_add_btn = obj;

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("MODIFY", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_modbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, parent->width-245-10, parent->height-50*3);
    g_mod_btn = obj;
    
	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("DELETE", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_delbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 10, parent->height-40*2-15);
    g_del_btn = obj;
    
	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("DELETE ALL", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, parent->width-245-10, parent->height-50*2);    
	nfui_regi_post_event_callback(obj, post_del_allbutton_event_handler);
    g_del_all_btn = obj;
    
    return 0;
}

void _dvabx_plateno_rule_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _dvabx_plateno_rule_hide(NFOBJECT *parent)
{
	nfui_nfobject_hide(parent);
}

gint vw_itx_dvabx_plateno_attach_select_ruleid(DVA_SELECT2_CB_FUNC select_cb, gpointer user_data)
{
	g_select_cb.cb_func = select_cb;
	g_select_cb.user_data = user_data;
	return 0;
}
