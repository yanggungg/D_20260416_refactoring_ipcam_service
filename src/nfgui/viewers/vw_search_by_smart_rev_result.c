/*
 * vw_vca_rev_search_result.c
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
#include "objects/nfcombobox.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vca_rev_component.h"
#include "vw_search_by_smart_rev_internal.h"

#include "vsm.h"
#include "vaa.h"
#include "vaa_itx.h"


////////////////////////////////////////////////////////////
//
// private data types
//
/* Result table. */
#define _RESULT_COLS                    5
#define _RESULT_ROWS                    19
#define _RESULT_TBL_GAP                 1
#define _RESULT_TBL_X                   10
#define _RESULT_TBL_Y                   55
#define _RESULT_LABEL_W                 120+160+250+212+(_RESULT_TBL_GAP*3)
#define _RESULT_LABEL_H                 36

#define NORMAL_OUTLINE_COLOR            0
#define FOCUS_OUTLINE_COLOR             146
#define SELECT_OUTLINE_COLOR            147

#define MAX_EVENTS 1024

typedef enum {
    RESULTLABEL_NO = 0,
    RESULTLABEL_RULE_COLOR,
    RESULTLABEL_RULE_NAME,
    RESULTLABEL_EVENT_ICON, 
    RESULTLABEL_EVENT_TYPE,
    RESULTLABEL_TIME,
    RESULTLABEL_ALL
} RESULTLABEL_E;    

enum {
    ORDER_BY_LATEST = 0,
    ORDER_BY_OLDEST
};

typedef struct _event_buffer_t {
	gint                no;
	ivca_rule_event_t   event;
} event_buffer_t;



////////////////////////////////////////////////////////////
//
// private variable
//

static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_filter_btn = NULL;
static NFOBJECT *g_parent = NULL;
static NFOBJECT *g_loglist_obj;
static NFOBJECT *g_order_combo = NULL;

static event_buffer_t g_evt_buffer[MAX_EVENTS];
static guint g_rule_mask = 0xffff;
static guint g_event_mask = 0xffff;
static gint g_focus_bufferidx = -1;
static gint g_evt_buffer_cnt;


////////////////////////////////////////////////////////////
//
// private interfaces 
//
static gint _get_rule_name_string(VAAID vaaid, gint zone_id, gchar *str)
{
    ITX_VAZONE_SHAPE shape;

    if ((zone_id < 0) || (zone_id > 16)) return -1;

    vaa_itx_get_zone_shape(vaaid, zone_id, &shape);
    strcpy(str, shape.name);
    
    return 0;
}

static gint _get_rule_color_string(VAAID vaaid, gint zone_id, gchar *str)
{
    ITX_VAZONE_SHAPE shape;

    if ((zone_id < 0) || (zone_id > 16)) return -1;

    vaa_itx_get_zone_shape(vaaid, zone_id, &shape);
    g_sprintf(str, "%d.color_key", shape.color_idx);
    
    return 0;
}

static gint _get_event_icon_string(guint type, gchar *str)
{
    if (type & IVCA_ET_DIR_POS)         strcpy(str, IMG_LIST_DIRECTION_FORWARD_N);
    else if (type & IVCA_ET_DIR_NEG)    strcpy(str, IMG_LIST_DIRECTION_REVERSE_N);
    else if (type & IVCA_ET_ENTER)      strcpy(str, IMG_LIST_ENTER_N);
    else if (type & IVCA_ET_EXIT)       strcpy(str, IMG_LIST_EXIT_N);
    else if (type & IVCA_ET_STOPPED)    strcpy(str, IMG_LIST_STOP_N);
    else if (type & IVCA_ET_REMOVED)    strcpy(str, IMG_LIST_REMOVE_N);
    else if (type & IVCA_ET_LOITERED)   strcpy(str, IMG_LIST_LOITERING_N);       
        
    return 0;
}

static gint _get_event_type_string(guint type, gchar *str)
{
    if (type & IVCA_ET_DIR_POS)         strcpy(str, "VCA-FORWARD");
    else if (type & IVCA_ET_DIR_NEG)    strcpy(str, "VCA-REVERSE");
    else if (type & IVCA_ET_ENTER)      strcpy(str, "VCA-ENTER");
    else if (type & IVCA_ET_EXIT)       strcpy(str, "VCA-EXIT");
    else if (type & IVCA_ET_STOPPED)    strcpy(str, "VCA-STOPPED");
    else if (type & IVCA_ET_REMOVED)    strcpy(str, "VCA-REMOVED");
    else if (type & IVCA_ET_LOITERED)   strcpy(str, "VCA-LOITERING");       
        
    return 0;
}

static gint _get_timestamp_string(time_t time, gchar *str)
{
    dtf_get_local_datetime(time, str);
    return 0;
}

static gint _insert_smartlog_list(NFOBJECT *obj, event_buffer_t *buffer)
{
    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    VAAID vaaid;

    gchar *log_list[RESULTLABEL_ALL];
    gchar strBuf[64];
    gint i;
      
    if ((g_rule_mask & (1 << buffer->event.rule_id)) == 0) return 0;
    if ((g_event_mask & buffer->event.type) == 0) return 0;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
      
    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", buffer->no);
    log_list[RESULTLABEL_NO] = g_strdup(strBuf);
 
    memset(strBuf, 0x00, sizeof(strBuf));
    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
    _get_rule_color_string(vaaid, buffer->event.rule_id, strBuf);
    log_list[RESULTLABEL_RULE_COLOR] = g_strdup(strBuf);
 
    memset(strBuf, 0x00, sizeof(strBuf));
    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
    _get_rule_name_string(vaaid, buffer->event.rule_id, strBuf);
    log_list[RESULTLABEL_RULE_NAME] = g_strdup(strBuf);
     
    memset(strBuf, 0x00, sizeof(strBuf));
    _get_event_icon_string(buffer->event.type, strBuf);
    log_list[RESULTLABEL_EVENT_ICON] = g_strdup(strBuf);

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_event_type_string(buffer->event.type, strBuf);
    log_list[RESULTLABEL_EVENT_TYPE] = g_strdup(strBuf);
 
    memset(strBuf, 0x00, sizeof(strBuf));        
    _get_timestamp_string(buffer->event.timestamp, strBuf);
    log_list[RESULTLABEL_TIME] = g_strdup(strBuf);

    //nfui_listbox_set_text((NFLISTBOX*)obj, log_list);
    nfui_listbox_set_text_with_draw((NFLISTBOX*)obj, log_list);
    /*
    for (i = 0; i < RESULTLABEL_ALL; i++)
    {
        if(log_list[i])  ifree(log_list[i]);
    }
    */
    return 0;
}

static gint _set_vaa_zone_value(NFOBJECT *obj, event_buffer_t *buffer)
{
    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    VAAID vaaid;
    gint en, color_idx, count;

    if ((g_rule_mask & (1 << buffer->event.rule_id)) == 0) return 0;
    if ((g_event_mask & buffer->event.type) == 0) return 0;

    top = nfui_nfobject_get_top(obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
    vaa_itx_get_zone_value(vaaid, buffer->event.rule_id, &en, &color_idx, &count);
    vaa_itx_set_zone_value(vaaid, buffer->event.rule_id, en, color_idx, count+1);
    return 0;
}


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_smartlog_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_LISTBOX_CHANGED) 
    {
        gchar *str_number;
        gint list_idx;
        gint buffer_idx;
        gint order = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_order_combo);

    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        NFOBJECT *playback_btn;
       
        list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);
        playback_btn = _get_searchbysmart_playback_btn();
        
        if (list_idx != -1) 
        {
            str_number = nfui_listbox_get_focus_text((NFLISTBOX*)obj, RESULTLABEL_NO);
            if (!str_number) return FALSE;

            if(strpbrk(str_number,"01"))    str_number = strpbrk(str_number,"01");

            buffer_idx = atoi(str_number)-1;
            if (g_focus_bufferidx == buffer_idx) return FALSE;

            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            component_data->preview.onoff = 0;
            vw_vca_rev_video_component_sync_preview(component_data->video_fixed);

            component_data->preview.onoff = 1;  
            component_data->preview.play_mode = 2;  
            component_data->preview.play_from = g_evt_buffer[buffer_idx].event.timestamp - 3;                   
            component_data->preview.play_to = g_evt_buffer[buffer_idx].event.timestamp + 100;       
            vw_vca_rev_video_component_sync_preview(component_data->video_fixed);
            vw_vca_rev_video_component_expose(component_data->video_fixed);        

            nfui_nfobject_enable(playback_btn);
            nfui_signal_emit(playback_btn, GDK_EXPOSE, TRUE);

            g_focus_bufferidx = buffer_idx;
        }
        else
        {
            if (g_focus_bufferidx == -1) return FALSE;
        
            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            if (!component_data->preview.onoff) return FALSE;
        
            component_data->preview.onoff = 0;
            vw_vca_rev_video_component_sync_preview(component_data->video_fixed);
            vw_vca_rev_video_component_expose(component_data->video_fixed);
        
            nfui_nfobject_disable(playback_btn);
            nfui_signal_emit(playback_btn, GDK_EXPOSE, TRUE);

            g_focus_bufferidx = -1;
        }    
    }
    else if (evt->type == GDK_2BUTTON_PRESS) 
    {
        gchar *str_number;
        gint list_idx;
        gint buffer_idx;

    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

    	GTimeVal tv;
       
        list_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);
        
        if (list_idx != -1) 
        {
            str_number = nfui_listbox_get_focus_text((NFLISTBOX*)obj, RESULTLABEL_NO);

            if(strpbrk(str_number,"01"))    str_number = strpbrk(str_number,"01");

            if (!str_number) return FALSE;

            buffer_idx = atoi(str_number)-1;

            top = nfui_nfobject_get_top(obj);
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            component_data->preview.onoff = 0;    
            vw_vca_rev_video_component_sync_preview(component_data->video_fixed);

        	tv.tv_usec = 0;
        	tv.tv_sec = g_evt_buffer[buffer_idx].event.timestamp - 3;

            VW_Search_start_playback();
        	vsm_playback_start((guint)(1 << component_data->preview.ch), tv, PLAYBACK_NORMAL);            
        }
    }    
    else if(evt->type == INFY_VCA_ANALYZE_NOTIFY)
    {
    	NFOBJECT *top;
        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        ivca_rule_event_t *pevt;
        gint *p;
        gint line_idx, cnt;
        gint i;

        cnt = nfui_listbox_get_box_count((NFLISTBOX*)obj);
        if (cnt >= MAX_EVENTS) return FALSE;

        p = pInfo->p.ptr;
        pevt = p + 2;

        if (cnt == -1) cnt = 0;           

		for(i = 0; i < p[1]; i++, pevt++) 
        {
            memset(&g_evt_buffer[cnt], 0x00, sizeof(event_buffer_t));
            g_evt_buffer[cnt].no = cnt+1;
            g_evt_buffer_cnt = cnt+1;
            memcpy(&g_evt_buffer[cnt].event, pevt, sizeof(ivca_rule_event_t));
            
            _set_vaa_zone_value(obj, &g_evt_buffer[cnt]);
            _insert_smartlog_list(obj, &g_evt_buffer[cnt]);
            cnt++;
    		
            if (cnt >= MAX_EVENTS) 
            {
                _set_searchbysmart_stop_by_time(pevt->timestamp);               
            
                top = nfui_nfobject_get_top(obj);
                nftool_mbox((NFWINDOW*)top, "NOTICE", "MAX EVENT MESSAGE", NFTOOL_MB_OK);
                return FALSE;
            }        
        }        
    }        
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_VCA_ANALYZE_NOTIFY);        
    }

    return FALSE;
}

static gboolean post_order_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint order = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        gint start_idx, end_idx;
        gint i;

    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *playback_btn;

        VAAID vaaid;
        gint en, color_idx, count;
       
        playback_btn = _get_searchbysmart_playback_btn();
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        component_data->preview.onoff = 0;
        vw_vca_rev_video_component_sync_preview(component_data->video_fixed);
        vw_vca_rev_video_component_expose(component_data->video_fixed);
    
        nfui_nfobject_disable(playback_btn);
        nfui_signal_emit(playback_btn, GDK_EXPOSE, TRUE);       

        nfui_listbox_delete_all((NFLISTBOX*)g_loglist_obj);
        nfui_signal_emit(g_loglist_obj, GDK_EXPOSE, TRUE);  
        g_focus_bufferidx = -1;

        vaaid = vaa_get_pb_vaaid(component_data->preview.ch);

        for (i = 0; i < 16; i++)  
        {
            vaa_itx_get_zone_value(vaaid, i, &en, &color_idx, &count);
            vaa_itx_set_zone_value(vaaid, i, en, color_idx, 0);
        }

        if (order == ORDER_BY_LATEST)
        {
            for (i = 1023; i >= 0; i--)
            {
                _set_vaa_zone_value(g_loglist_obj, &g_evt_buffer[i]);
                _insert_smartlog_list(g_loglist_obj, &g_evt_buffer[i]);
            }        
        }
        else if(order == ORDER_BY_OLDEST)
        {
            for (i = 0; i < 1024; i++)
            {            
                _set_vaa_zone_value(g_loglist_obj, &g_evt_buffer[i]);
                _insert_smartlog_list(g_loglist_obj, &g_evt_buffer[i]);
            }
        }
    }

    return FALSE;
}

static gboolean post_filterbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{     
	    gint start_idx, end_idx;
        gint i,ret;
        gint order = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_order_combo);
        guint rule_msk = g_rule_mask;
        guint event_msk = g_event_mask;

    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *playback_btn;

        VAAID vaaid;
        gint en, color_idx, count;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
       
        playback_btn = _get_searchbysmart_playback_btn();
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        component_data->preview.onoff = 0;
        vw_vca_rev_video_component_sync_preview(component_data->video_fixed);
        vw_vca_rev_video_component_expose(component_data->video_fixed);
    
        nfui_nfobject_disable(playback_btn);
        nfui_signal_emit(playback_btn, GDK_EXPOSE, TRUE);
		
        ret = vw_search_by_smart_rev_filter_open(g_curwnd, &rule_msk, &event_msk);
        if (!ret) return FALSE;

        g_rule_mask = rule_msk;
        g_event_mask = event_msk;

        nfui_listbox_delete_all((NFLISTBOX*)g_loglist_obj);
        nfui_signal_emit(g_loglist_obj, GDK_EXPOSE, TRUE);
        g_focus_bufferidx = -1;
        
        vaaid = vaa_get_pb_vaaid(component_data->preview.ch);

        for (i = 0; i < 16; i++)  
        {
            vaa_itx_get_zone_value(vaaid, i, &en, &color_idx, &count);
            vaa_itx_set_zone_value(vaaid, i, en, color_idx, 0);
        }

        if (order == ORDER_BY_LATEST)
        {
            for (i = 1023; i >= 0; i--)
            {
                _set_vaa_zone_value(g_loglist_obj, &g_evt_buffer[i]);
                _insert_smartlog_list(g_loglist_obj, &g_evt_buffer[i]);
            }        
        }
        else if(order == ORDER_BY_OLDEST)
        {
            for (i = 0; i < 1024; i++)
            {            
                _set_vaa_zone_value(g_loglist_obj, &g_evt_buffer[i]);
                _insert_smartlog_list(g_loglist_obj, &g_evt_buffer[i]);
            }
        }        
    }
    
	return FALSE;
}





////////////////////////////////////////////////////////////
//
// protected interfaces 
//
gint _search_by_smart_rev_result_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;

    gint i, j, tbl_x, tbl_y;
    gchar *tbl_livelog_str[] = {"No.", "RULE NAME", "EVENT TYPE", "TIME"};
    gchar *strCombo[] = {"LATEST", "OLDEST"};
    guint tbl_w[6] = {100, 160, 220, 262};
    gint opt;
    

    memset(g_evt_buffer, 0x00, sizeof(event_buffer_t)*MAX_EVENTS);
    g_rule_mask = 0xffff;
    g_event_mask = 0xffff;

    g_focus_bufferidx = -1;
    
    g_parent = parent;
    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);

    obj= (NFOBJECT*)nftool_normal_button_create_type3("FILTER", 245);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_filterbutton_event_handler);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, parent->width-245-35, 5);
    g_filter_btn = obj;
    
    tbl_x = _RESULT_TBL_X;
    tbl_y = _RESULT_TBL_Y;

    for(i = 0; i < 4; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tbl_livelog_str[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 10);
        nfui_nfobject_set_size(obj, tbl_w[i] - 1, _RESULT_LABEL_H);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)g_parent, obj, tbl_x, tbl_y);
        
        tbl_x = tbl_x + tbl_w[i];
    }
    
    tbl_x = _RESULT_TBL_X;
    tbl_y = _RESULT_TBL_Y + _RESULT_LABEL_H;

    tbl_w[RESULTLABEL_NO] = 100;
    tbl_w[RESULTLABEL_RULE_COLOR] = 40;
    tbl_w[RESULTLABEL_RULE_NAME] = 120;
    tbl_w[RESULTLABEL_EVENT_ICON] = 40;    
    tbl_w[RESULTLABEL_EVENT_TYPE] = 180;
    tbl_w[RESULTLABEL_TIME] = 262;

    obj = nfui_listbox_new(RESULTLABEL_ALL, tbl_w, _RESULT_LABEL_H);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RESULTLABEL_NO, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RESULTLABEL_RULE_NAME, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RESULTLABEL_EVENT_TYPE, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), RESULTLABEL_TIME, NFALIGN_CENTER);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(0));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, _RESULT_LABEL_W + 25, _RESULT_LABEL_H*_RESULT_ROWS);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, tbl_x, tbl_y);  
    nfui_regi_post_event_callback(obj, post_smartlog_list_event_cb);  
    g_loglist_obj = obj;

    uxm_reg_imsg_event(obj, INFY_VCA_ANALYZE_NOTIFY);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)parent, obj, 10, parent->height-40-10);
    nfui_nfobject_show(obj);

    obj = nfui_combobox_new(strCombo, 2, 1);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_nfobject_set_size(obj, 300, 40);
    nfui_nffixed_put((NFFIXED*)parent, obj, 150, parent->height-40-10);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_order_event_handler);
    g_order_combo = obj;

    return 0;
}

gint _search_by_smart_rev_result_clear()
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;

    VAAID vaaid;
    gint en, color_idx, count;
    gint i;

    memset(g_evt_buffer, 0x00, sizeof(event_buffer_t)*MAX_EVENTS);
    g_rule_mask = 0xffff;
    g_event_mask = 0xffff;

    nfui_listbox_delete_all((NFLISTBOX*)g_loglist_obj);
    nfui_signal_emit(g_loglist_obj, GDK_EXPOSE, TRUE);
    g_focus_bufferidx = -1;
    g_evt_buffer_cnt = 0;

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_order_combo, 1);
    nfui_signal_emit(g_order_combo, GDK_EXPOSE, TRUE);

    top = nfui_nfobject_get_top(g_loglist_obj);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);

    for (i = 0; i < 16; i++)  
    {
        vaa_itx_get_zone_value(vaaid, i, &en, &color_idx, &count);
        vaa_itx_set_zone_value(vaaid, i, en, color_idx, 0);
    }
   
    return 0;
}

NFOBJECT *_get_searchbysmart_filter()
{
    return g_filter_btn;
}

NFOBJECT *_get_searchbysmart_loglist()
{
    return g_loglist_obj;
}

NFOBJECT *_get_searchbysmart_order()
{
    return g_order_combo;
}

event_buffer_t *_get_searchbysmart_result()
{
    return g_evt_buffer;
}

gint _get_search_by_smart_result()
{
    return g_evt_buffer_cnt;
}


void _search_by_smart_rev_result_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _search_by_smart_rev_result_hide(NFOBJECT *parent)
{
    NFOBJECT *topwin;

    topwin = nfui_nfobject_get_top(parent); 
    nfui_nfobject_hide(topwin);
}

