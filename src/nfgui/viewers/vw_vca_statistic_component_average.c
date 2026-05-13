/*
 * vw_vca_rev_component_calibration_adrz.c
 *
 * Written by Jungkyu. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, June 14, 2014
 *
 */

#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfthumbnail.h"
#include "objects/cw_calendar.h"
#include "objects/nfbargraph.h"       
#include "objects/nfpiechart.h"

#include "vw_search_by_statistic.h"
#include "vw_search_main.h"
#include "vw_vca_statistic_component.h"

#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"

#include "stm.h"
#include "dtf.h"
#include "uxm.h"
#include "var.h"
#include "ix_mem.h"

#define SBV_FIXED3_X            (470)
#define SBV_FIXED3_Y            (523)
#define SBV_FIXED3_W            ((1386/2)-50)
#define SBV_FIXED3_H            (380)

// FIXED3

#define SBV_LABEL_X_GAP         (180)
#define SBV_LABEL_Y_GAP         (20)

#define SBV_LABEL_WIDTH         (230)
#define SBV_LABEL_WIDTH_TITLE   (120)
#define SBV_LABEL_HEIGHT        (40)

// AVERAGE TITLE

#define SBV_AV_TITLE_X          (30)
#define SBV_AV_TITLE_Y          (20)
#define SBV_AV_TITLE_W          (SBV_LABEL_WIDTH_TITLE)
#define SBV_AV_TITLE_H          (SBV_LABEL_HEIGHT)

#define SBV_AV_TITLE_CB_X       (SBV_AV_TITLE_X+SBV_AV_TITLE_W+10)
#define SBV_AV_TITLE_CB_Y       (SBV_AV_TITLE_Y)
#define SBV_AV_TITLE_CB_W       (SBV_LABEL_X_GAP)
#define SBV_AV_TITLE_CB_H       (SBV_LABEL_HEIGHT)    

// BARGRAPH AVERAGE

#define SBV_BARGRAPH_AV_X       (SBV_AV_TITLE_X)
#define SBV_BARGRAPH_AV_Y       (SBV_AV_TITLE_Y+SBV_AV_TITLE_H+20)
#define SBV_BARGRAPH_AV_W       ((SBV_FIXED3_W+50)-(SBV_BARGRAPH_AV_X*3))
#define SBV_BARGRAPH_AV_H       ((SBV_FIXED3_H-SBV_AV_TITLE_H)-50)


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent * evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)  
    {
        //TODO :

    }

    return FALSE;
}

static gboolean post_average_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_ACTION_T  *statistic_action;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gint idx;

        top = nfui_nfobject_get_top(obj);
        statistic_action = (STATISTIC_COMPONENT_ACTION_T *)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        idx = nfui_combobox_get_cur_index(obj);

        if (idx == 0) statistic_data->average.average_type = AVERAGE_TYPE_HOUR;
        else if (idx == 1) statistic_data->average.average_type = AVERAGE_TYPE_WEEK;
        else if (idx == 2) statistic_data->average.average_type = AVERAGE_TYPE_DAY;        
        else statistic_data->average.average_type = AVERAGE_TYPE_NONE;

        statistic_action->average_type_cb(obj);
    }        
    else if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gint index;

        top = nfui_nfobject_get_top(obj);

        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA); 
        
        switch(statistic_data->total.period_type)
        {
            case PERIOD_TYPE_HOUR:
            case PERIOD_TYPE_DAY:
                statistic_data->average.average_type = AVERAGE_TYPE_NONE;
                nfui_nfobject_disable(obj);
                break;
            default :
                nfui_nfobject_enable(obj);
        }

        nfui_combobox_remove_all((NFCOMBOBOX*)obj);
        switch(statistic_data->total.period_type)
        {
            case PERIOD_TYPE_HOUR:
            case PERIOD_TYPE_DAY:
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "NONE");
                break;
            case PERIOD_TYPE_WEEK:
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "HOUR");
                break;
            case PERIOD_TYPE_MONTH:
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "HOUR");            
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "WEEK");
                break;
            case PERIOD_TYPE_YEAR:
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "HOUR");
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "WEEK");
                nfui_combobox_append_data((NFCOMBOBOX*)obj, "DAY");
                break;
        }

        if (statistic_data->average.average_type == AVERAGE_TYPE_HOUR) index = 0;
        else if (statistic_data->average.average_type == AVERAGE_TYPE_WEEK) index = 1;
        else if (statistic_data->average.average_type == AVERAGE_TYPE_DAY) index = 2;
        else index = 0;
        
        nfui_combobox_set_index_no_expose(obj, index);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }
    return FALSE;
}

static gboolean post_none_nfbargraph_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {

        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        
        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);     
       
        if(statistic_data->average.average_type == AVERAGE_TYPE_NONE)
        {
            if(!nfui_nfobject_is_shown(obj))
            {
                nfui_nfobject_show(obj);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
        }
        else
        {
            if(nfui_nfobject_is_shown(obj))
            {
                nfui_nfobject_hide(obj);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
        }
    }
    return FALSE;
}

static gboolean post_hour_nfbargraph_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
        gint i;

        if(statistic_data->average.average_type == AVERAGE_TYPE_HOUR)
        {                
            for( i =0; i<statistic_data->average.hour_cnt; i++)
            {
                nfui_nfbargraph_set_bar_data_text((NFBARGRAPH *)obj,statistic_data->average.hour_text[i],i);  
                nfui_nfbargraph_set_bar_value((NFBARGRAPH*)obj, statistic_data->average.hour_value[i], i);
            }       
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            
            if (!nfui_nfobject_is_shown(obj)) 
            {         
                nfui_nfobject_show(obj);
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);
            }
        }
        else 
        {
            if (nfui_nfobject_is_shown(obj)) 
            {
                nfui_nfobject_hide(obj); 
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);                
            }
        }          
    }
    return FALSE;
}

static gboolean post_day_nfbargraph_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        gint i;

        if(statistic_data->average.average_type == AVERAGE_TYPE_DAY)
        {
            for( i =0; i<statistic_data->average.day_cnt; i++)
            {
                 nfui_nfbargraph_set_bar_data_text((NFBARGRAPH *)obj,statistic_data->average.day_text[i],i);  
                 nfui_nfbargraph_set_bar_value((NFBARGRAPH*)obj, statistic_data->average.day_value[i], i);                
            }
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        
            if (!nfui_nfobject_is_shown(obj)) 
            {
                nfui_nfobject_show(obj);  
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);                
            }
        }
        else 
        {
            if (nfui_nfobject_is_shown(obj)) 
            {
                nfui_nfobject_hide(obj); 
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);                
            }
        }     
    }
    return FALSE;
}

static gboolean post_week_nfbargraph_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        gint i;
        nfui_nfbargraph_set_bar_max_cnt((NFBARGRAPH*) obj, statistic_data->average.week_cnt);
        
        if(statistic_data->average.average_type == AVERAGE_TYPE_WEEK)
        {           
            for( i =0; i<statistic_data->average.week_cnt; i++)
            {
                nfui_nfbargraph_set_bar_data_text((NFBARGRAPH *)obj,statistic_data->average.week_text[i],i);  
                nfui_nfbargraph_set_bar_value((NFBARGRAPH*)obj, statistic_data->average.week_value[i], i);
            }                   
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            
            if (!nfui_nfobject_is_shown(obj)) 
            {   
                nfui_nfobject_show(obj);                  
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);                
            }
        }
        else 
        {
            if (nfui_nfobject_is_shown(obj)) 
            {
                nfui_nfobject_hide(obj); 
                nfui_signal_emit( obj, GDK_EXPOSE, TRUE);                
            }
        }    
    }
    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_statistic_average_component_open(NFOBJECT *parent, guint opt)
{
    NFOBJECT *obj;  
    NFOBJECT *top;
    NFOBJECT *fixed;

    NFOBJECT *average_obj;
    NFOBJECT *bargraph_avr[4];

    gint i,j;

    const gchar *strPeriod[]={"HOUR","WEEK","DAY"};

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, SBV_FIXED3_W, SBV_FIXED3_H);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 0, 0);
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AVERAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_AV_TITLE_W, SBV_AV_TITLE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, SBV_AV_TITLE_X, SBV_AV_TITLE_Y);
    
    average_obj = nfui_combobox_new(strPeriod, 3, 0);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)average_obj,NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align((NFCOMBOBOX*)average_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(average_obj, SBV_AV_TITLE_CB_W, SBV_AV_TITLE_CB_H);
    nfui_nfobject_show(average_obj);
    nfui_nffixed_put((NFFIXED*)fixed, average_obj, SBV_AV_TITLE_CB_X, SBV_AV_TITLE_CB_Y);
    nfui_regi_post_event_callback(average_obj, post_average_event_handler);
    
    for(i=0; i<4; i++){

        bargraph_avr[i] = nfui_nfbargraph_new(SBV_BARGRAPH_AV_W,SBV_BARGRAPH_AV_H);
        nfui_nfbargraph_set_pango_font((NFBARGRAPH *)bargraph_avr[i], nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
        nfui_nfbargraph_set_barbg_color((NFBARGRAPH *) bargraph_avr[i], COLOR_IDX(669));
        nfui_nfbargraph_set_chart_line_gap_draw((NFBARGRAPH *) bargraph_avr[i],0, FALSE, TRUE);
        nfui_nfbargraph_set_bar_focus((NFBARGRAPH *)bargraph_avr[i], FALSE);
        nfui_nffixed_put((NFFIXED *)fixed, bargraph_avr[i], SBV_BARGRAPH_AV_X, SBV_BARGRAPH_AV_Y);
        if (i == AVERAGE_TYPE_NONE) nfui_regi_post_event_callback(bargraph_avr[i], post_none_nfbargraph_event_handler);
        else if (i == AVERAGE_TYPE_HOUR) nfui_regi_post_event_callback(bargraph_avr[i], post_hour_nfbargraph_event_handler);
        else if (i == AVERAGE_TYPE_WEEK) nfui_regi_post_event_callback(bargraph_avr[i], post_week_nfbargraph_event_handler);
        else if (i == AVERAGE_TYPE_DAY) nfui_regi_post_event_callback(bargraph_avr[i], post_day_nfbargraph_event_handler);
        
        if( i == AVERAGE_TYPE_NONE)
            nfui_nfobject_show(bargraph_avr[i]);
        else
            nfui_nfobject_hide(bargraph_avr[i]);     

            
        if(i == AVERAGE_TYPE_NONE)
        {
            nfui_nfbargraph_set_chart_line_gap_draw((NFBARGRAPH *) bargraph_avr[i], 0, TRUE, TRUE);

            for( j =0; j<12; j++)
            {             
                nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *) bargraph_avr[i], 35);
                nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph_avr[i],35);
                nfui_nfbargraph_set_bar_value((NFBARGRAPH *) bargraph_avr[i], 0, j);
            }
        }
        else if(i == AVERAGE_TYPE_HOUR)
        {
            for( j=0; j<24; j++)
            {
                nfui_nfbargraph_set_bar_size((NFBARGRAPH *)bargraph_avr[i],25);
                nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph_avr[i],35);
                nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph_avr[i],35);
            }
        } 
        else if(i== AVERAGE_TYPE_DAY)
        {
            for( j=0; j<31; j++)
            {
                nfui_nfbargraph_set_bar_size((NFBARGRAPH *)bargraph_avr[i],25);
                nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph_avr[i],10);
                nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph_avr[i],10);
            }
        }
        else if(i== AVERAGE_TYPE_WEEK)
        {
            for( j=0; j<7; j++)
            {
                nfui_nfbargraph_set_bar_size((NFBARGRAPH *)bargraph_avr[i],25);
                nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph_avr[i],50);
                nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph_avr[i],50);                    
            }
        }
    }         
    return 0;
}

gint vw_vca_statistic_average_component_show()
{

    return 0;
}

gint vw_vca_statistic_average_component_hide()
{

    return 0;
}

gint vw_vca_statistic_average_component_sync_data(NFOBJECT *parent)
{ 
    nfui_user_signal_emit(parent, NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC, TRUE);

    return 0;
}

gint vw_vca_statistic_average_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

