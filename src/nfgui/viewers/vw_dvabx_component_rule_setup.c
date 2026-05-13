/*
 * vw_dvabx_component_rule_setup.c
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

#include "vw_dvabx_component.h"





////////////////////////////////////////////////////////////
//
// private data types
//
#define _RULE_SETUP_GAP         (10)

#define _RULE_SETUP_LINE_X          (20)
#define _RULE_SETUP_LINE_Y          (20)
#define _RULE_SETUP_LINE_H          (130)

#define _RULE_SETUP_AREA_X          (20)
#define _RULE_SETUP_AREA_Y          (20)
#define _RULE_SETUP_AREA_H          (330)

#define _RULE_SETUP_COUNT_X         (20)
#define _RULE_SETUP_COUNT_Y         (20)
#define _RULE_SETUP_COUNT_H         (100)

#define LINE_CNT        (2)
#define AREA_CNT        (6)

#define HELP_TEXT       "Up to 16 rules can be set and one or more of these rules can be selected to trigger video analytic events."

typedef struct _LINE_DATA_T
{
    NFOBJECT *icon_line[LINE_CNT];
    NFOBJECT *check_line[LINE_CNT];
}LINE_DATA_T;

typedef struct _AREA_DATA_T
{
    NFOBJECT *icon_area[AREA_CNT];
    NFOBJECT *check_area[AREA_CNT];
}AREA_DATA_T;


////////////////////////////////////////////////////////////
//
// private variable
//




////////////////////////////////////////////////////////////
//
// private interfaces 
//

static void _change_line_status(LINE_DATA_T *line_data, gint status, gint expose)
{
    gint i;
    
    if(status)
    {
        nfui_nfimage_change_image((NFIMAGE*)line_data->icon_line[0], IMG_LIVE_DIRECTION_FORWARD_N);
        nfui_nfimage_change_image((NFIMAGE*)line_data->icon_line[1], IMG_LIVE_DIRECTION_REVERSE_N);

        for(i = 0; i < LINE_CNT; i++)
        {
            nfui_nfobject_enable(line_data->check_line[i]);
        }
    }
    else
    {
        nfui_nfimage_change_image((NFIMAGE*)line_data->icon_line[0], IMG_LIVE_DIRECTION_FORWARD_D);
        nfui_nfimage_change_image((NFIMAGE*)line_data->icon_line[1], IMG_LIVE_DIRECTION_REVERSE_D);
        
        for(i = 0; i < LINE_CNT; i++)
        {
            nfui_nfobject_disable(line_data->check_line[i]);
        }
    }

    if(expose)
    {
        for(i = 0; i < LINE_CNT; i++)
        {
            nfui_signal_emit(line_data->icon_line[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(line_data->check_line[i], GDK_EXPOSE, TRUE);
        }
    }
    
}

static void _change_area_status(AREA_DATA_T *area_data, gint status, gint expose)
{
    gint i;
    
    if(status)
    {
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[0], IMG_LIVE_INTRUSION_N);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[1], IMG_LIVE_ENTER_N);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[2], IMG_LIVE_EXIT_N);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[3], IMG_LIVE_LOITERING_N);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[4], IMG_LIVE_STOP_N);  
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[5], IMG_LIVE_REMOVE_N);

        for(i = 0; i < AREA_CNT; i++)
        {
            nfui_nfobject_enable(area_data->check_area[i]);
        }
    }
    else
    {
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[0], IMG_LIVE_INTRUSION_D);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[1], IMG_LIVE_ENTER_D);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[2], IMG_LIVE_EXIT_D);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[3], IMG_LIVE_LOITERING_D);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[4], IMG_LIVE_STOP_D);
        nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[5], IMG_LIVE_REMOVE_D);

        for(i = 0; i < AREA_CNT; i++)
        {
            nfui_nfobject_disable(area_data->check_area[i]);    
        }
    }
    if(expose)
    {
        for(i = 0; i < AREA_CNT; i++)
        {
            nfui_signal_emit(area_data->icon_area[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(area_data->check_area[i], GDK_EXPOSE, TRUE);
        }   
    }
}

static void _set_detector_area_status(AREA_DATA_T *area_data, gint expose)
{
    gint i;

    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[0], IMG_LIVE_INTRUSION_N);
    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[1], IMG_LIVE_ENTER_D);
    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[2], IMG_LIVE_EXIT_D);
    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[3], IMG_LIVE_LOITERING_D);
    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[4], IMG_LIVE_STOP_D);  
    nfui_nfimage_change_image((NFIMAGE*)area_data->icon_area[5], IMG_LIVE_REMOVE_D);

    nfui_nfobject_enable(area_data->check_area[0]);

    for(i = 1; i < AREA_CNT; i++)
    {
        nfui_nfobject_disable(area_data->check_area[i]);
    }    

    if(expose)
    {
        for(i = 0; i < AREA_CNT; i++)
        {
            nfui_signal_emit(area_data->icon_area[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(area_data->check_area[i], GDK_EXPOSE, TRUE);
        }   
    }
}

static void _change_counter_status(NFOBJECT *icon_counter, gint status, gint expose)
{
    if (status) nfui_nfimage_change_image((NFIMAGE*)icon_counter, IMG_LIVE_COUNT_N);  
    else nfui_nfimage_change_image((NFIMAGE*)icon_counter, IMG_LIVE_COUNT_D);  
    
    if (expose) nfui_signal_emit(icon_counter, GDK_EXPOSE, TRUE);
}

static void _sync_object(NFOBJECT *obj, gint index, gint status, gint expose)
{
    LINE_DATA_T *line_data;
    AREA_DATA_T *area_data;
    NFOBJECT *icon_counter;
    
    switch(index)
    {
        case RTYPE_LINE:
        {
            line_data = nfui_nfobject_get_data(obj, "rule setup data");            
            if (line_data == NULL) return FALSE;
                
            _change_line_status(line_data, status, expose);
        }
        break;
        
        case RTYPE_AREA:
        {
            area_data = nfui_nfobject_get_data(obj, "rule setup data");
            if (area_data == NULL) return FALSE;
            
            _change_area_status(area_data, status, expose);
        }
        break;
        
        case RTYPE_COUNTER:
        {
            icon_counter = nfui_nfobject_get_data(obj, "rule setup data");
            if (icon_counter == NULL) return FALSE;
            
            _change_counter_status(icon_counter, status, expose);
        }
        break;
    }
}




////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_fixd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkPixbuf *pbuf = NULL;
    GdkGC *gc;
    gint gap_x, gap_y, size_w, size_h;

    if(evt->type == GDK_EXPOSE) 
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);
    
        nfui_nfobject_gc_unref(gc);
    }
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
    }

    return FALSE;
}

static gboolean post_none_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);       
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_data->use_zone = 0;
        component_action->rule_type_cb(obj);
    }
    else if(evt->type == NFEVENT_RADIO_LOST_FOCUS)
    {       
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);   

        component_data->use_zone = 1;
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->use_zone == 0)
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }
    }

    return FALSE;
}

static gboolean post_line_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_data->rule_type = RTYPE_LINE;        
        _sync_object(obj, RTYPE_LINE, 1, 1);
        component_action->rule_type_cb(obj);
    }
    else if(evt->type == NFEVENT_RADIO_LOST_FOCUS)
    {        
        _sync_object(obj, RTYPE_LINE, 0, 1);
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if ((component_data->use_zone == 1) && (component_data->rule_type == RTYPE_LINE))
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
            _sync_object(obj, RTYPE_LINE, 1, 0);
        } 
        else
        {
            _sync_object(obj, RTYPE_LINE, 0, 0);
        }
    }

    return FALSE;
}

static gboolean post_area_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

        AREA_DATA_T *area_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_data->rule_type = RTYPE_AREA;        
        _sync_object(obj, RTYPE_AREA, 1, 1);
        component_action->rule_type_cb(obj);
    }
    else if(evt->type == NFEVENT_RADIO_LOST_FOCUS)
    {
        _sync_object(obj, RTYPE_AREA, 0, 1);
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if ((component_data->use_zone == 1) && (component_data->rule_type == RTYPE_AREA))
        {
            AREA_DATA_T *area_data;

            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
            _sync_object(obj, RTYPE_AREA, 1, 0);
        } 
        else
        {
            _sync_object(obj, RTYPE_AREA, 0, 0);
        }
    }

    return FALSE;
}

static gboolean post_counter_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_data->rule_type = RTYPE_COUNTER;        
        _sync_object(obj, RTYPE_COUNTER, 1, 1);
        component_action->rule_type_cb(obj);
    }
    else if(evt->type == NFEVENT_RADIO_LOST_FOCUS)
    {        
        _sync_object(obj, RTYPE_COUNTER, 0, 1);
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if ((component_data->use_counter == 1) && (component_data->rule_type == RTYPE_COUNTER))
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
            _sync_object(obj, RTYPE_COUNTER, 1, 0);
        } 
        else
        {
            _sync_object(obj, RTYPE_COUNTER, 0, 0);
        }
    }

    return FALSE;
}

static gboolean post_forward_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->line.forward = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.forward);
    }

    return FALSE;
}

static gboolean post_reverse_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->line.reverse = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.reverse);
    }

    return FALSE;
}

static gboolean post_intrusion_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.intrusion = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.intrusion);
    }

    return FALSE;
}

static gboolean post_enter_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.enter = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.enter);
    }

    return FALSE;
}

static gboolean post_exit_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.exit = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.exit);
    }

    return FALSE;
}

static gboolean post_removed_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.removed = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.removed);
    }

    return FALSE;
}

static gboolean post_loitering_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.loitering = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.loitering);
    }

    return FALSE;
}

static gboolean post_stopped_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->area.stopped = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->area.stopped);
    }

    return FALSE;
}

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_dvabx_rule_setup_component_open(NFOBJECT *parent, guint opt)
{
    NFOBJECT *obj;
    NFOBJECT *fixed;
    NFOBJECT *fixed1;
    NFOBJECT *fixed2;
    NFOBJECT *fixed3;
    NFOBJECT *fixed_title;
    GSList *list = NULL;

    LINE_DATA_T *line_data;
    AREA_DATA_T *area_data;
    NFOBJECT *icon_counter;
    NFOBJECT *radio_obj[4] = {NULL,};
    
    gchar *str_line[] = {"FORWARD DIRECTION", "REVERSE DIRECTION"};
    gchar *str_area[] = {"INTRUSION", "VCA-ENTER", "VCA-EXIT", "VCA-LOITERING", "VCA-STOPPED", "VCA-REMOVED"};
    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    gint pos_x, pos_y;
    gint fixed_x, fixed_y;

    gint size_w, size_h;
    gint i,x,y;
    gchar lfBuf[4096];

    radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
    radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
    radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
    radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   

    pos_x = 30;
    pos_y = 20;
    fixed_x = 10;
    fixed_y = 20;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
    list = nfui_radio_button_get_group(NF_BUTTON(obj));
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, 30, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_none_radio_event_handler);
    radio_obj[0] = obj;

    if (opt & (1 << OPT_RULE_SETUP_NONE))
    {
        nfui_nfobject_show(radio_obj[0]);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, 30+40, pos_y);
        
        fixed_x = 10;
        fixed_y += 40;
    }

    if (opt & (1 << OPT_RULE_SETUP_LINE))
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, _RULE_SETUP_LINE_H);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_fixd_event_handler);
        fixed1 = fixed;

        pos_x = 20;
        pos_y = 0;

        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, 280, 40);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)fixed1, fixed, pos_x, pos_y);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        fixed_title = fixed;

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_radio_button_add_group(NF_BUTTON(obj), list);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, 0, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_line_radio_event_handler);
        radio_obj[1] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, 40, pos_y);

        pos_x = 20;
        pos_y = 100;

        line_data = imalloc(sizeof(LINE_DATA_T));

        line_data->icon_line[0] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_DIRECTION_FORWARD_D));
        line_data->icon_line[1] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_DIRECTION_REVERSE_D));

        for(i = 0; i < LINE_CNT; i++)
        {
            nfui_nfobject_show(line_data->icon_line[i]);
            nfui_nffixed_put((NFFIXED*)fixed1, line_data->icon_line[i], pos_x + 20, pos_y - 60);
            x += 200;        

            obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
            nfui_nfobject_disable(obj);
            line_data->check_line[i] = obj;

            pos_x += 30; 
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str_line[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_set_size(obj, 200, 30);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed1, obj,  pos_x, pos_y);

            pos_x += 200; 

        }

        nfui_regi_post_event_callback(line_data->check_line[0], post_forward_check_event_handler);
        nfui_regi_post_event_callback(line_data->check_line[1], post_reverse_check_event_handler);

        nfui_nfobject_set_alloc_data(radio_obj[1], "rule setup data", line_data);

        fixed_x = 10;
        fixed_y += _RULE_SETUP_LINE_H + 40;

    }

    if (opt & (1 << OPT_RULE_SETUP_AREA))
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, _RULE_SETUP_AREA_H);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_fixd_event_handler);
        fixed2 = fixed;

        pos_x = 20;
        pos_y = 0;

        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, 280, 40);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)fixed2, fixed, pos_x, pos_y);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        fixed_title = fixed;

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_radio_button_add_group(NF_BUTTON(obj), list);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, 0, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_area_radio_event_handler);
        radio_obj[2] = obj;
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AREA", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj,  40, pos_y);

        pos_y += 100;

        area_data = imalloc(sizeof(AREA_DATA_T));

        area_data->icon_area[0] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_INTRUSION_D));
        area_data->icon_area[1] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_ENTER_D));
        area_data->icon_area[2] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_EXIT_D));
        area_data->icon_area[3] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_LOITERING_D));
        area_data->icon_area[4] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_STOP_D));
        area_data->icon_area[5] = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_REMOVE_D));

        for(i = 0; i < AREA_CNT; i++)
        {
            nfui_nfobject_show(area_data->icon_area[i]);
            nfui_nffixed_put((NFFIXED*)fixed2, area_data->icon_area[i], pos_x + 20, pos_y - 60);

            if ((ivsc.vendor_code == 28) || (ivsc.vendor_code == 128)) {
                if (i == 5) nfui_nfobject_hide(area_data->icon_area[i]);
            }

            obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);
            nfui_nfobject_disable(obj);
            area_data->check_area[i] = obj;

            if ((ivsc.vendor_code == 28) || (ivsc.vendor_code == 128)) {
                if (i == 5) nfui_nfobject_hide(area_data->check_area[i]);
            }

            pos_x += 30; 
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str_area[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_set_size(obj, 200, 30);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed2, obj,  pos_x, pos_y);

            if ((ivsc.vendor_code == 28) || (ivsc.vendor_code == 128)) {
                if (i == 5) nfui_nfobject_hide(obj);
            }

            pos_x += 200;

            if ((i == 1) || (i == 3))
            {
                pos_x = 20;
                pos_y += 100;
            }
        }
        
        nfui_regi_post_event_callback(area_data->check_area[0], post_intrusion_check_event_handler);
        nfui_regi_post_event_callback(area_data->check_area[1], post_enter_check_event_handler);
        nfui_regi_post_event_callback(area_data->check_area[2], post_exit_check_event_handler);
        nfui_regi_post_event_callback(area_data->check_area[3], post_loitering_check_event_handler);
        nfui_regi_post_event_callback(area_data->check_area[4], post_stopped_check_event_handler);
        nfui_regi_post_event_callback(area_data->check_area[5], post_removed_check_event_handler);

        nfui_nfobject_set_alloc_data(radio_obj[2], "rule setup data", area_data);

        fixed_x = 10;
        fixed_y += _RULE_SETUP_AREA_H + 40;

    }

    if (opt & (1 << OPT_RULE_SETUP_COUNTER))
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, _RULE_SETUP_COUNT_H);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_fixd_event_handler);
        fixed3 = fixed;

        pos_x = 20;
        pos_y = 0;

        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, 280, 40);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)fixed3, fixed, pos_x, pos_y);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        fixed_title = fixed;

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_radio_button_add_group(NF_BUTTON(obj), list);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, 0, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_counter_radio_event_handler);
        radio_obj[3] = obj;
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj,  40, pos_y);

        icon_counter = (NFOBJECT*)nfui_nfimage_new((IMG_LIVE_COUNT_D));
        nfui_nfobject_show(icon_counter);
        nfui_nffixed_put((NFFIXED*)fixed3, icon_counter, 40, 50);

        nfui_nfobject_set_data(radio_obj[3], "rule setup data", icon_counter);
    }

    if (opt & (1 << OPT_RULE_SETUP_HELP))
    {
        pos_x = 20;
        pos_y = DEFAULT_COMPONENT_HEIGHT - 160;
    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("[?]", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, 40, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("What are rule settings?", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-60, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+40, pos_y);

        pos_y += 40;

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(HELP_TEXT), DEFAULT_COMPONENT_WIDTH-pos_x-30, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-20, 100);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);       
    }

    return 0;
}

gint vw_dvabx_rule_setup_component_show()
{

    return 0;
}

gint vw_dvabx_rule_setup_component_hide()
{

    return 0;
}

gint vw_dvabx_rule_setup_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_rule_setup_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

