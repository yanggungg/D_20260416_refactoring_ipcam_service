/*
 * vw_vca_rev_component_calibration_result.c
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

#define SBV_FIXED4_X            (470+(1386/2))
#define SBV_FIXED4_Y            (523)
#define SBV_FIXED4_W            (1386/2+50)
#define SBV_FIXED4_H            (380)

#define SBV_LABEL_X_GAP         (180)
#define SBV_LABEL_Y_GAP         (20)

#define SBV_LABEL_WIDTH         (230)
#define SBV_LABEL_WIDTH_TITLE   (120)
#define SBV_LABEL_HEIGHT        (40)

// FIXED4 
// PIE CHART

#define SBV_PIE_LABEL_Z_X       (30)
#define SBV_PIE_LABEL_Z_Y       (20+10)
#define SBV_PIE_LABEL_Z_W       (SBV_LABEL_WIDTH_TITLE)
#define SBV_PIE_LABEL_Z_H       (SBV_LABEL_HEIGHT)

#define SBV_PIE_LABEL_E_X       (SBV_FIXED4_W/2+30+30)
#define SBV_PIE_LABEL_E_Y       (SBV_PIE_LABEL_Z_Y)
#define SBV_PIE_LABEL_E_W       (SBV_LABEL_WIDTH_TITLE)
#define SBV_PIE_LABEL_E_H       (SBV_LABEL_HEIGHT)

#define SBV_PIECHART_B_W        (SBV_FIXED4_W)
#define SBV_PIECHART_B_X        (((SBV_PIECHART_B_W/2)+10)-30)
#define SBV_PIECHART_B_Y        (SBV_LABEL_Y_GAP+SBV_PIE_LABEL_Z_H+(SBV_PIECHART_B_W/2))

#define SBV_PIECHART_S_W        (SBV_PIECHART_B_W/3)
#define SBV_PIECHART_S_X        ((SBV_PIECHART_S_W/2)-30-10)
#define SBV_PIECHART_S_Y        (SBV_LABEL_Y_GAP+SBV_PIE_LABEL_Z_H+(SBV_PIECHART_S_W/2))

#define SBV_PIECHART_S2_W       (SBV_PIECHART_S_W)
#define SBV_PIECHART_S2_X       (SBV_PIECHART_S_X+SBV_PIECHART_S_W+100+30)
#define SBV_PIECHART_S2_Y       (SBV_PIECHART_S_Y)

#define SBV_ZONE_LABEL_X        (SBV_PIECHART_S_X+(SBV_PIECHART_S_W/2)+70)
#define SBV_ZONE_LABEL_Y        (SBV_PIE_LABEL_Z_Y+30)
#define SBV_ZONE_LABEL_W        (110)
#define SBV_ZONE_LABEL_H        (20)

#define SBV_EVENT_LABEL_X       (SBV_PIECHART_S2_X+(SBV_PIECHART_S_W/2)+65)
#define SBV_EVENT_LABEL_Y       (SBV_PIE_LABEL_Z_Y+30)
#define SBV_EVENT_LABEL_W       (75)
#define SBV_EVENT_LABEL_H       (40)


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gchar strbuf[32];        
        gchar *event_str[]={"FORWARD","REVERSE","ENTER","EXIT","STOPPED","REMOVED","LOITERING"};

        gint i;
        gint max = 0;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);    

        memset(strbuf, 0, sizeof(strbuf));
        
        for( i=0; i<16; i++)
        {
            memset(strbuf,0x00,sizeof(strbuf));
            g_sprintf(strbuf,"%s.%d(%d%%)",statistic_data->rate.zone_str[i], statistic_data->rate.zone_val[i],(int)statistic_data->rate.zone_ratio_val[i]);
                   
            if(i==0)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone1_value"), strbuf);   
            if(i==1)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone2_value"), strbuf);   
            if(i==2)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone3_value"), strbuf);   
            if(i==3)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone4_value"), strbuf);   
            if(i==4)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone5_value"), strbuf);   
            if(i==5)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone6_value"), strbuf);   
            if(i==6)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone7_value"), strbuf);   
            if(i==7)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone8_value"), strbuf);   
            if(i==8)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone9_value"), strbuf);   
            if(i==9)     nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone10_value"), strbuf);   
            if(i==10)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone11_value"), strbuf);   
            if(i==11)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone12_value"), strbuf);   
            if(i==12)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone13_value"), strbuf);   
            if(i==13)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone14_value"), strbuf);   
            if(i==14)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone15_value"), strbuf);   
            if(i==15)    nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"zone16_value"), strbuf);   
            
        }

        for(i=0; i<7; i++)
        {
            memset(strbuf,0x00,sizeof(strbuf));
          
            if(i==0){                           
                g_sprintf(strbuf,lookup_string("FORWARD\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event1_value"), strbuf);
            }
            if(i==1){
                g_sprintf(strbuf,lookup_string("REVERSE\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event2_value"), strbuf);
            }
            if(i==2){
                g_sprintf(strbuf,lookup_string("ENTER\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event3_value"), strbuf);
            }
            if(i==3){
                g_sprintf(strbuf,lookup_string("EXIT\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event4_value"), strbuf);
            }    
            if(i==4){
                g_sprintf(strbuf,lookup_string("STOPPED\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event5_value"), strbuf);
            }    
            if(i==5){
                g_sprintf(strbuf,lookup_string("REMOVED\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event6_value"), strbuf);
            }    
            if(i==6){
                g_sprintf(strbuf,lookup_string("LOITERING\n%d (%d%%)"),statistic_data->rate.event_val[i],(int)statistic_data->rate.event_ratio_val[i]);
                nfui_nflabel_set_text((NFLABEL *)nfui_nfobject_get_data(obj,"event7_value"), strbuf);
            }
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        
    }
    return FALSE;
}

static gboolean post_pichart1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        gint i;
        gint max=0;

        for( i =0; i<16; i++)
        {
            max += statistic_data->rate.zone_val[i];
        }
        
        if(max != 0)    
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, max);
        else 
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, 100);
         
        for(i=0; i<16; i++)
        {
            nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, i,statistic_data->rate.zone_val[i]);
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }
    
    return FALSE;
}

static gboolean post_pichart2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {

        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        gint i,max=0;

        for( i =0; i<7; i++)
        {
            max += statistic_data->rate.event_val[i];

        }

        if(max != 0)
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, max);
        else
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, 100);
        
        for(i=0; i<7; i++)
        {
            nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, i,statistic_data->rate.event_val[i]);
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;    
}


/*  현재 사용 x 
static gboolean post_zone1_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
         
        gchar strbuf[32];
        gint i;

        memset(strbuf,0x00,sizeof(strbuf));

        g_sprintf(strbuf,"%s %d  [%d (%d%%)]","ZONE",1, statistic_data->rate.zone_val,0);
        nfui_nflabel_set_text((NFLABEL *)obj, strbuf);            
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }

    return FALSE;
}

static gboolean post_event_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gchar *event_str[]={"FORWARD","REVERSE","ENTER","EXIT","REMOVED","LOITERING","STOPPED"};

        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);    
        
        gchar strbuf[32];
        gint i;

        memset(strbuf,0x00,sizeof(strbuf));

        for(i=0 i<7; i++)
        {
            g_sprintf(strbuf,"%s\n[%d (%d%%)]",event_str[i],statistic_data->rate.event_val,0);
            nfui_nflabel_set_text((NFLABEL *)obj, strbuf);            
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TURE);
        
    }
    return FALSE;
}

*/

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_statistic_rate_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	
	NFOBJECT *listbox;
    NFOBJECT *fixed;

    NFOBJECT *zone_obj[16];
    NFOBJECT *event_obj[7];
    gchar *event_str[]={"FORWARD","REVERSE","ENTER","EXIT","STOPPED","REMOVED","LOITERING"};


    gint i;

    gint pos_x, pos_y;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, SBV_FIXED4_W, SBV_FIXED4_H);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 0, 0);
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZONE", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_PIE_LABEL_Z_W, SBV_PIE_LABEL_Z_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, SBV_PIE_LABEL_Z_X, SBV_PIE_LABEL_Z_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_PIE_LABEL_E_W, SBV_PIE_LABEL_E_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, SBV_PIE_LABEL_E_X, SBV_PIE_LABEL_E_Y);

    pos_x = SBV_ZONE_LABEL_X+5;    
    pos_y = SBV_ZONE_LABEL_Y;

    for(i=0; i<16; i++)
    {
        gchar strbuf[32];

        memset(strbuf,0x00,sizeof(strbuf));

        obj = (NFOBJECT*)nfui_nflabel_new("");
        nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, 670+i);
        nfui_nfobject_set_size(obj,13,13);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x-21, pos_y+4);

        g_sprintf(strbuf,"ZONE %02d.  %d(%d%%)",i+1, 0, 0); 
      
        zone_obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strbuf,nffont_get_pango_font(NFFONT_MINI_NORMAL), COLOR_IDX(193));
        nfui_nflabel_set_align((NFLABEL*)zone_obj[i], NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(zone_obj[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(zone_obj[i], SBV_ZONE_LABEL_W, SBV_ZONE_LABEL_H);
        nfui_nfobject_use_focus(zone_obj[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show((NFOBJECT *) zone_obj[i]);
        nfui_nffixed_put((NFFIXED*)fixed, zone_obj[i], pos_x, pos_y);
        //nfui_regi_post_event_callback(zone_obj[i], post_zone1_label_event_handler);
        
        pos_y += SBV_ZONE_LABEL_H;          
        /*
        if(i==7)
        {   
            pos_x = SBV_ZONE_LABEL_X-10+SBV_ZONE_LABEL_W;
            pos_y = SBV_ZONE_LABEL_Y+50;
        }
        */
    }

    pos_x = SBV_EVENT_LABEL_X+15;
    pos_y = SBV_EVENT_LABEL_Y+SBV_EVENT_LABEL_H/2;
    
    for(i=0; i<7;i++)
    {
        gchar strbuf[32];

        memset(strbuf, 0x00, sizeof(strbuf));

        obj = (NFOBJECT*)nfui_nflabel_new("");
        nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, 670+i);
        nfui_nfobject_set_size(obj,13,13);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x-21, pos_y+6);

        g_sprintf(strbuf,"%s\n%d (%d%%)",event_str[i],0,0);
        
        event_obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strbuf,nffont_get_pango_font(NFFONT_MINI_NORMAL_1), COLOR_IDX(193));
        nfui_nflabel_set_align((NFLABEL*)event_obj[i], NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(event_obj[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(event_obj[i], SBV_EVENT_LABEL_W, SBV_EVENT_LABEL_H);
        nfui_nfobject_use_focus(event_obj[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(event_obj[i]);
        nfui_nffixed_put((NFFIXED*)fixed, event_obj[i], pos_x, pos_y);
        //nfui_regi_post_event_callback(zone_obj[i], post_event_label_event_handler);
        pos_y += (SBV_EVENT_LABEL_H);
    }
   
    obj = nfui_nfpiechart_new(SBV_PIECHART_S_W, 16, 300);
    nfui_nfpiechart_set_arcbg_color((NFPIECHART *)obj, 686);
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0,60);
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 1,100);
    nfui_nfpiechart_set_pango_font((NFPIECHART *) obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, SBV_PIECHART_S_X, SBV_PIECHART_S_Y);
    nfui_regi_post_event_callback(obj, post_pichart1_event_handler);   

    for( i =0 ; i<16; i++)
    {
        nfui_nfpiechart_set_chart_color((NFPIECHART *)obj, i,670+i);
    }
    
    obj = nfui_nfpiechart_new(SBV_PIECHART_S2_W, 7, 200);
    nfui_nfpiechart_set_arcbg_color((NFPIECHART *)obj, 686);
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0,90);
    nfui_nfpiechart_set_pango_font((NFPIECHART *) obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, SBV_PIECHART_S2_X, SBV_PIECHART_S2_Y);
    nfui_regi_post_event_callback(obj, post_pichart2_event_handler);   
    
    for( i =0 ; i<7; i++)
    {
        nfui_nfpiechart_set_chart_color((NFPIECHART *)obj, i,670+i);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // ZONE
    
    nfui_nfobject_set_data(fixed, "zone1_value", zone_obj[0]);
    nfui_nfobject_set_data(fixed, "zone2_value", zone_obj[1]);
    nfui_nfobject_set_data(fixed, "zone3_value", zone_obj[2]);
    nfui_nfobject_set_data(fixed, "zone4_value", zone_obj[3]);
    nfui_nfobject_set_data(fixed, "zone5_value", zone_obj[4]);
    nfui_nfobject_set_data(fixed, "zone6_value", zone_obj[5]);
    nfui_nfobject_set_data(fixed, "zone7_value", zone_obj[6]);
    nfui_nfobject_set_data(fixed, "zone8_value", zone_obj[7]);
    nfui_nfobject_set_data(fixed, "zone9_value", zone_obj[8]);
    nfui_nfobject_set_data(fixed, "zone10_value", zone_obj[9]);
    nfui_nfobject_set_data(fixed, "zone11_value", zone_obj[10]);
    nfui_nfobject_set_data(fixed, "zone12_value", zone_obj[11]);
    nfui_nfobject_set_data(fixed, "zone13_value", zone_obj[12]);
    nfui_nfobject_set_data(fixed, "zone14_value", zone_obj[13]);
    nfui_nfobject_set_data(fixed, "zone15_value", zone_obj[14]);
    nfui_nfobject_set_data(fixed, "zone16_value", zone_obj[15]);

    // EVENT

    nfui_nfobject_set_data(fixed, "event1_value", event_obj[0]);
    nfui_nfobject_set_data(fixed, "event2_value", event_obj[1]);
    nfui_nfobject_set_data(fixed, "event3_value", event_obj[2]);
    nfui_nfobject_set_data(fixed, "event4_value", event_obj[3]);
    nfui_nfobject_set_data(fixed, "event5_value", event_obj[4]);
    nfui_nfobject_set_data(fixed, "event6_value", event_obj[5]);
    nfui_nfobject_set_data(fixed, "event7_value", event_obj[6]);    

	return 0;
}

gint vw_vca_statistic_rate_component_show()
{

    return 0;
}

gint vw_vca_statistic_rate_component_hide()
{

    return 0;
}

gint vw_vca_statistic_rate_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_statistic_rate_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

