/*
 * vw_vca_rev_component_calibration_setup.c
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


#define SBV_FIXED2_X            (470)
#define SBV_FIXED2_Y            (0)
#define SBV_FIXED2_W            (1386)
#define SBV_FIXED2_H            (523)

#define SBV_LABEL_X_GAP         (180)
#define SBV_LABEL_Y_GAP         (20)

#define SBV_LABEL_WIDTH         (230)
#define SBV_LABEL_WIDTH_TITLE   (120)
#define SBV_LABEL_HEIGHT        (40)


#define SBV_CAL_LABEL_X         (SBV_FIXED2_W/3)
#define SBV_CAL_LABEL_Y         (SBV_FIXED2_Y+20)
#define SBV_CAL_LABEL_W         (SBV_LABEL_WIDTH*2)
#define SBV_CAL_LABEL_H         (SBV_LABEL_HEIGHT)

#define SBV_CAL_LABEL_PREV_X    (SBV_CAL_LABEL_X-60)   
#define SBV_CAL_LABEL_PREV_Y    (SBV_CAL_LABEL_Y)
#define SBV_CAL_LABEL_NEXT_X    (SBV_CAL_LABEL_X+SBV_CAL_LABEL_W+10)
#define SBV_CAL_LABEL_NEXT_Y    (SBV_CAL_LABEL_Y)

// TOTAL LABEL

#define SBV_TOTAL_LABEL_X       (SBV_CAL_LABEL_X)
#define SBV_TOTAL_LABEL_Y       (SBV_CAL_LABEL_Y+SBV_CAL_LABEL_H+20)
#define SBV_TOTAL_LABEL_W       (SBV_LABEL_WIDTH*2)
#define SBV_TOTAL_LABEL_H       (SBV_LABEL_HEIGHT)

// BARGRAPH TOTAL

#define SBV_BARGRAPH_TO_X       (30)
#define SBV_BARGRAPH_TO_Y       (SBV_TOTAL_LABEL_Y+SBV_TOTAL_LABEL_H +40)
#define SBV_BARGRAPH_TO_W       (SBV_FIXED2_W-60)
#define SBV_BARGRAPH_TO_H       ((SBV_FIXED2_H-SBV_BARGRAPH_TO_Y)-20)


// TIME SET
#define	DAY_TO_SEC			    ((time_t)86400)
#define	HOUR_TO_SEC	        	((time_t)3600)
#define	MIN_TO_SEC	        	((time_t)60)



////////////////////////////////////////////////////////////
//
// private function 
//

static guint _get_select_wday(gshort year, gshort month, gshort day)
{
	struct tm tm_ptr;
    guint utime;
    time_t timer;
    struct tm t;
    
    tm_ptr.tm_year = year - 1900;
    tm_ptr.tm_mon  = month - 1;
    tm_ptr.tm_mday = day;
    tm_ptr.tm_hour = 0;
    tm_ptr.tm_min  = 0;
    tm_ptr.tm_sec  = 0;
    tm_ptr.tm_isdst = 0;
    
    utime = mktime(&tm_ptr);
    
    timer = utime;
    localtime_r(&timer, &t);
    
    return t.tm_wday;
}

static guint _get_month_lastday(gshort year, gshort month)
{
	int i;
	struct tm tm_ptr;
    guint utime;
    
    time_t timer;
    struct tm t;
    
    for(i = 0 ; i <= 5 ; i++)
    {
    	tm_ptr.tm_year = year - 1900;
		tm_ptr.tm_mon  = month - 1;
		tm_ptr.tm_mday = 27 + i;
		tm_ptr.tm_hour = 0;
		tm_ptr.tm_min  = 0;
		tm_ptr.tm_sec  = 0;
		tm_ptr.tm_isdst = 0;
		
		utime = mktime(&tm_ptr);
    	
    	timer = utime + 86400;    	    	
	   	localtime_r(&timer, &t);	
  	
		if(t.tm_mday == 1) break;
    }
    
    timer = utime;    	    	
	localtime_r(&timer, &t);
        
    return t.tm_mday;
}


gint _get_statistic_date(gint year, gint mon, gint day, gint hour, gchar *str, gint type, gint select_cnt, gint last_day)
{
    guint dformat = 0;
    FM_DATE_E fm_date;

    DAL_get_dateTime_format(&dformat, NULL);

    gchar *time_h[] = {"00:00 ~ 01:00","01:00 ~ 02:00","02:00 ~ 03:00","03:00 ~ 04:00","04:00 ~ 05:00","05:00 ~ 06:00",
                       "06:00 ~ 07:00","07:00 ~ 08:00","08:00 ~ 09:00","09:00 ~ 10:00","10:00 ~ 11:00","11:00 ~ 12:00",
                       "12:00 ~ 13:00","13:00 ~ 14:00","14:00 ~ 15:00","15:00 ~ 16:00","16:00 ~ 17:00","17:00 ~ 18:00",
                       "18:00 ~ 19:00","19:00 ~ 20:00","20:00 ~ 21:00","21:00 ~ 22:00","22:00 ~ 23:00","23:00 ~ 24:00"};

    enum {
    	STATISTIC_MODE_HOUR,
    	STATISTIC_MODE_DAY,
    	STATISTIC_MODE_WEEK,
    	STATISTIC_MODE_MONTH,
    	STATISTIC_MODE_YEAR,
    	STATISTIC_MODE_MAX,
    };
                       
    switch(type){
    
        case STATISTIC_MODE_HOUR :
        {                     
            if(dformat == 0)
                g_sprintf(str, "%04d. %02d. %02d   %s", year, mon, day, time_h[hour] );
            else if (dformat == 1)
                g_sprintf(str, "%02d. %02d. %04d   %s", mon, day, year, time_h[hour] );
            else
                g_sprintf(str, "%02d. %02d. %04d   %s", day, mon, year, time_h[hour] );
        }
        break;
        
        case STATISTIC_MODE_DAY :
        {
            if(dformat == 0)
                g_sprintf(str, "%04d. %02d. %02d", year, mon, day);
            else if (dformat == 1)
                g_sprintf(str, "%02d. %02d. %04d", mon, day, year);
            else
                g_sprintf(str, "%02d. %02d. %04d", day, mon, year);
        }
        break;

        case STATISTIC_MODE_WEEK :
        {
            gint temp_fday, temp_lday;

            temp_fday = day-select_cnt;   //  day : real day ex) 15,16..31  selec_cnt: count ex) 0,1,2,3 ... 6
            temp_lday = temp_fday+6;
            
            if(temp_lday > last_day)     temp_lday = last_day;
            if(day-select_cnt < 1)       temp_fday = 1;
            
            if(dformat == 0)
                g_sprintf(str, "%04d. %02d. %02d  ~  %04d. %02d. %02d", year, mon, temp_fday, year, mon, temp_lday);
            else if (dformat == 1)
                g_sprintf(str, "%02d. %02d. %04d  ~  %02d. %02d. %04d", mon, temp_fday, year, mon, temp_lday, year);
            else
                g_sprintf(str, "%02d. %02d. %04d  ~  %02d. %02d. %04d", temp_fday, mon, year, temp_lday, mon, year);
        }
        break;

    }
    return strlen(str);
}



static int _get_text_datetime_cal_range_time(time_t search_time, guint type, gchar *strTime)
{
    gchar tmp[16];
    gint year, mon, day, hour, min, sec;
    gint select_cnt, last_day;    

    memset(tmp, 0x00, sizeof(tmp));
    ifn_get_local_day(search_time, &year, &mon, &day);
    ifn_get_local_hourmin(search_time, &hour, &min, &sec);

    switch(type){
        case PERIOD_TYPE_HOUR:   
            _get_statistic_date(year, mon, day, hour, strTime, PERIOD_TYPE_HOUR, select_cnt, last_day);
            break;
        
        case PERIOD_TYPE_DAY :
            _get_statistic_date(year, mon, day, 0, strTime, PERIOD_TYPE_DAY, select_cnt, last_day);
            break;
        
        case PERIOD_TYPE_WEEK: 
            select_cnt = _get_select_wday( year, mon, day);
            last_day = _get_month_lastday( year, mon);
            _get_statistic_date(year, mon, day, 0, strTime, PERIOD_TYPE_WEEK, select_cnt, last_day);
            
            break;

        case PERIOD_TYPE_YEAR:
            g_sprintf(strTime, "%04d",year);
            break;
    }
    return 0;
}

static int _update_FromTo_range_label_text(NFOBJECT *obj, time_t start_time, time_t end_time, gint type)
{
    char buf[64];
    int s_year, s_mon, s_day, s_hour, s_min, s_sec;
    int e_year, e_mon, e_day, e_hour, e_min, e_sec;

    ifn_get_local_day(start_time, &s_year, &s_mon, &s_day);
    ifn_get_local_hourmin(start_time, &s_hour, &s_min, &s_sec);
    ifn_get_local_day(end_time, &e_year, &e_mon, &e_day);
    ifn_get_local_hourmin(end_time, &e_hour, &e_min, &e_sec);

    switch(type)
    {
        case PERIOD_TYPE_HOUR:
            g_sprintf(buf,"%02d.%02d  %02d:%02d  ~  %02d.%02d  %02d:%02d",s_mon, s_day, s_hour, s_min, e_mon, e_day, e_hour,e_min);
        break;
        case PERIOD_TYPE_DAY:
            g_sprintf(buf,"%02d.%02d  %02d:%02d  ~  %02d.%02d  %02d:%02d",s_mon, s_day, s_hour, s_min, e_mon, e_day, e_hour,e_min);
        break;
        case PERIOD_TYPE_WEEK:
            g_sprintf(buf,"%02d.%02d  %02d:%02d  ~  %02d.%02d  %02d:%02d",s_mon, s_day, s_hour, s_min, e_mon, e_day, e_hour,e_min);
        break;        
        case PERIOD_TYPE_MONTH:
            g_sprintf(buf,"%02d. %02d  ~  %02d. %02d",s_mon,s_day, e_mon,e_day);
        break;        
        case PERIOD_TYPE_YEAR:
            g_sprintf(buf,"%02d. %02d  ~  %02d. %02d",s_year,s_mon, e_year,e_mon);
        break;        
    }
    
    nfui_nflabel_set_text((NFLABEL*)obj, buf);
    nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    
    return 0;
}


static int _update_cal_range_label_text(NFOBJECT *obj, time_t timet, gint period_type)
{
    char buf[64];
    int year,mon;
    int length;
    
    if( period_type == PERIOD_TYPE_MONTH )
    {
        ifn_get_local_day(timet, &year, &mon, 0);
    
        length = strlen(g_month_str[mon-1]);
        g_utf8_strncpy(buf, g_month_str[mon-1], g_utf8_strlen(g_month_str[mon-1], -1));
        g_sprintf(&buf[length],"  %d", year);
    
        nfui_nflabel_set_text((NFLABEL*)obj,buf);
        nfui_signal_emit(obj, GDK_EXPOSE, FALSE);    
    }
    else
    {
        _get_text_datetime_cal_range_time(timet, period_type, buf);
        nfui_nflabel_set_text((NFLABEL*)obj,buf);
        nfui_signal_emit(obj, GDK_EXPOSE, FALSE);    
    }

    return 0;
}


////////////////////////////////////////////////////////////
//
// handler
//


static gboolean post_prev_btn_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_ACTION_T *statistic_action;

        top = nfui_nfobject_get_top(obj);
        statistic_action = (STATISTIC_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));

        statistic_action->total_previous_cb(obj);
    }

    return FALSE;
}

static gboolean post_next_btn_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_ACTION_T *statistic_action;

        top = nfui_nfobject_get_top(obj);
        statistic_action = (STATISTIC_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));

        statistic_action->total_next_cb(obj);
    }
    
    return FALSE;
}

static gboolean post_nfbargraph_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_BARGRAPH_SELECTED)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_ACTION_T *statistic_action;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        top = nfui_nfobject_get_top(obj);

        statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
        statistic_action = (STATISTIC_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));

        statistic_action->total_graph_press(obj);


    }
    else if(evt->type == NFEVENT_BARGRAPH_2BUTTON_PRESS)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        STATISTIC_COMPONENT_ACTION_T *statistic_action;
        gint type;
        gint idx;
        
        top = nfui_nfobject_get_top(obj);
        statistic_action = (STATISTIC_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));
        statistic_data = (STATISTIC_COMPONENT_DATA_T *)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        idx = nfui_nfbargraph_get_bar_focus_index((NFBARGRAPH*)obj);
        statistic_data->total.bar_idx = idx;

        type = statistic_data->total.period_type;
        if (type == PERIOD_TYPE_HOUR) return FALSE;
        
        statistic_action->total_graph_2press(obj);
    }
    else if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gint i, type;
    
        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);       
    }    
    return FALSE;
}



static gboolean post_range_text_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        
        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        if(statistic_data->total.period_type == statistic_data->total.init_period_type)                
            _update_FromTo_range_label_text(obj,statistic_data->total.start_time, statistic_data->total.end_time, statistic_data->total.period_type);
        else
            _update_cal_range_label_text(obj,statistic_data->total.calendar_time, statistic_data->total.period_type);
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}

static gboolean post_total_text_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;

        gchar strBuf[64];
      
        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        memset(strBuf, 0, sizeof(strBuf));

        g_sprintf(strBuf, lookup_string("TOTAL  %d  EVENTS"), statistic_data->total.event_count);

        nfui_nflabel_set_text((NFLABEL *) obj, strBuf);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        STATISTIC_COMPONENT_DATA_T *statistic_data;
        gint i;    
        
        top = nfui_nfobject_get_top(obj);
        statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top,STATISTIC_COMPONENT_DATA);
        
        if(statistic_data->total.period_type == PERIOD_TYPE_HOUR)
        {
            nfui_nfobject_show(nfui_nfobject_get_data(obj,"hour_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"day_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"week_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"month_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"year_bargraph"));
            
        }
        else if(statistic_data->total.period_type == PERIOD_TYPE_DAY)
        {
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"hour_bargraph"));
            nfui_nfobject_show(nfui_nfobject_get_data(obj,"day_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"week_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"month_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"year_bargraph"));
        }
        else if(statistic_data->total.period_type == PERIOD_TYPE_WEEK)
        {
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"hour_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"day_bargraph"));
            nfui_nfobject_show(nfui_nfobject_get_data(obj,"week_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"month_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"year_bargraph"));
        }
        else if(statistic_data->total.period_type == PERIOD_TYPE_MONTH)
        {
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"hour_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"day_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"week_bargraph"));
            nfui_nfobject_show(nfui_nfobject_get_data(obj,"month_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"year_bargraph"));
        }
        else if(statistic_data->total.period_type == PERIOD_TYPE_YEAR)
        {
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"hour_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"day_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"week_bargraph"));
            nfui_nfobject_hide(nfui_nfobject_get_data(obj,"month_bargraph"));
            nfui_nfobject_show(nfui_nfobject_get_data(obj,"year_bargraph"));
        }

        switch(statistic_data->total.period_type){
            case PERIOD_TYPE_HOUR :
                for(i=0; i<statistic_data->total.hour_cnt; i++)
                {
                    nfui_nfbargraph_set_bar_data_text(nfui_nfobject_get_data(obj,"hour_bargraph"), statistic_data->total.hour_text[i], i);
                    nfui_nfbargraph_set_bar_value(nfui_nfobject_get_data(obj,"hour_bargraph"), statistic_data->total.hour_value[i], i);
                }   
                break;
            
            case PERIOD_TYPE_DAY :
                for(i=0; i<statistic_data->total.day_cnt; i++)
                {
                    nfui_nfbargraph_set_bar_data_text(nfui_nfobject_get_data(obj,"day_bargraph"), statistic_data->total.day_text[i], i);
                    nfui_nfbargraph_set_bar_value(nfui_nfobject_get_data(obj,"day_bargraph"), statistic_data->total.day_value[i], i);   
                }
                break;
            
            case PERIOD_TYPE_WEEK :
                for(i=0; i<statistic_data->total.week_cnt; i++)
                {
                    nfui_nfbargraph_set_bar_data_text(nfui_nfobject_get_data(obj,"week_bargraph"), statistic_data->total.week_text[i],i);
                    nfui_nfbargraph_set_bar_value(nfui_nfobject_get_data(obj,"week_bargraph"), statistic_data->total.week_value[i], i);                       
                }
                break;
            
            case PERIOD_TYPE_MONTH : 
                nfui_nfbargraph_set_bar_max_cnt(nfui_nfobject_get_data(obj,"month_bargraph"), statistic_data->total.month_cnt);
            
                for(i=0; i<statistic_data->total.month_cnt; i++)
                {
                    nfui_nfbargraph_set_bar_data_text(nfui_nfobject_get_data(obj,"month_bargraph"),statistic_data->total.month_text[i],i);
                        
                    nfui_nfbargraph_set_bar_value(nfui_nfobject_get_data(obj,"month_bargraph"), statistic_data->total.month_value[i], i);                       
                }       
                break;
            
            case PERIOD_TYPE_YEAR :
                for(i=0; i<statistic_data->total.year_cnt; i++)
                {
                    nfui_nfbargraph_set_bar_data_text(nfui_nfobject_get_data(obj,"year_bargraph"), statistic_data->total.year_text[i],i);
                    nfui_nfbargraph_set_bar_value(nfui_nfobject_get_data(obj,"year_bargraph"), statistic_data->total.year_value[i], i);                     
                }
                break;
        }

        if(statistic_data->total.init_period_type == statistic_data->total.period_type)
        {
            switch(statistic_data->total.init_period_type)
            {
                case PERIOD_TYPE_HOUR:
                    nfui_nfbargraph_set_bar_init_gap(nfui_nfobject_get_data(obj,"hour_bargraph"), statistic_data->total.bar_init_gap); break;
                case PERIOD_TYPE_DAY:
                    nfui_nfbargraph_set_bar_init_gap(nfui_nfobject_get_data(obj,"day_bargraph"), statistic_data->total.bar_init_gap); break;
                case PERIOD_TYPE_MONTH:
                    nfui_nfbargraph_set_bar_init_gap(nfui_nfobject_get_data(obj,"month_bargraph"), statistic_data->total.bar_init_gap); break;
                case PERIOD_TYPE_YEAR:
                    nfui_nfbargraph_set_bar_init_gap(nfui_nfobject_get_data(obj,"year_bargraph"), statistic_data->total.bar_init_gap); break;
            }            
        }    
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        nfui_make_key_hierarchy((NFWINDOW*)top);
    }
    
    return FALSE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_statistic_total_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	
	NFOBJECT *top;
	NFOBJECT *fixed;
	NFOBJECT *prev_btn;
	NFOBJECT *next_btn;
	NFOBJECT *range_text;
	NFOBJECT *total_text;
	NFOBJECT *bargraph[5];

    guint size_w, size_h;
    gint  i,j;
    gint year, month, day;    

    time_t search_time;
    stm_get_time_range_t(0, &search_time);
    ifn_get_local_day(search_time, &year, &month, &day);

    gchar strBuf[64];
    const gchar *strtemp[]={"TOTAL  %d  EVENTS"};
    const gchar *utf_str = NULL;
    gint length;
    
    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, SBV_FIXED2_W, SBV_FIXED2_H);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 0, 0);
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	
    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);	
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);	
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);	

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);	
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);	
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);	   
	 
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj,(guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, SBV_CAL_LABEL_PREV_X, SBV_CAL_LABEL_PREV_Y);
    nfui_regi_post_event_callback(obj, post_prev_btn_event_handler);
    prev_btn = obj;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj,(guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, SBV_CAL_LABEL_NEXT_X, SBV_CAL_LABEL_NEXT_Y);
    nfui_regi_post_event_callback(obj, post_next_btn_event_handler);
    next_btn = obj;

    length = strlen(g_month_str[month - 1]);
    g_utf8_strncpy(strBuf, g_month_str[month -1],g_utf8_strlen(g_month_str[month-1],-1));
    g_sprintf(&strBuf[length],"  %d",year);

    range_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_LARGE_NORMAL), COLOR_IDX(193));
    nfui_nfobject_set_size(range_text,(guint)SBV_CAL_LABEL_W,(guint)SBV_CAL_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)range_text, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(range_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(range_text, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(range_text);
    nfui_nffixed_put((NFFIXED*)fixed, range_text, (guint)SBV_CAL_LABEL_X,(guint)SBV_CAL_LABEL_Y);    
    nfui_regi_post_event_callback(range_text, post_range_text_event_handler);
    
    total_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(193));
    nfui_nfobject_set_size(total_text,(guint)SBV_TOTAL_LABEL_W,(guint)SBV_TOTAL_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)total_text, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(total_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(total_text, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(total_text);
    nfui_nffixed_put((NFFIXED*)fixed, total_text, (guint)SBV_TOTAL_LABEL_X,(guint)SBV_TOTAL_LABEL_Y);
    nfui_regi_post_event_callback(total_text, post_total_text_event_handler);    

    ////////////////// jaeyoung ///// NFBARGRAPH  TEST /////////////////////////////////////////////////////////////////////////
    
    int str_num[] = { 50, 60 , 80, 70, 100, 150, 70, 120, 20, 150, 200, 100, 30, 120,180, 200, 210, 120, 30, 200, 21, 29, 100, 80, 29, 200, 80, 91, 40, 180, 100 };

    // test value;
    
    for(i = 0; i<5; i++){
    
        bargraph[i] =  nfui_nfbargraph_new(SBV_BARGRAPH_TO_W, SBV_BARGRAPH_TO_H);
        nfui_nfbargraph_set_pango_font((NFBARGRAPH *)bargraph[i], nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
        nfui_nfbargraph_set_barbg_color((NFBARGRAPH *) bargraph[i], COLOR_IDX(686));
        nfui_nfbargraph_set_chart_line_gap_draw((NFBARGRAPH *) bargraph[i], 0, TRUE, TRUE);
        nfui_nfbargraph_set_bar_value_draw(( NFBARGRAPH *) bargraph[i], TRUE);
        nfui_nffixed_put((NFFIXED *)fixed, bargraph[i], SBV_BARGRAPH_TO_X, SBV_BARGRAPH_TO_Y);    
        nfui_regi_post_event_callback(bargraph[i], post_nfbargraph_event_handler);
        
        if( i == PERIOD_TYPE_MONTH)
            nfui_nfobject_show(bargraph[i]);
        else 
            nfui_nfobject_hide(bargraph[i]);

        if(i == PERIOD_TYPE_HOUR)          
        {
            nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph[i], 80);
            nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph[i], 80);
        }
        else if(i == PERIOD_TYPE_DAY)
        {
            nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph[i], 40);
            nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph[i], 40); 
        }
        else if(i == PERIOD_TYPE_WEEK)
        {
            nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph[i], 120);
            nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph[i], 120);            
        }
        else if(i == PERIOD_TYPE_MONTH)
        {
            nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph[i], 40);
            nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph[i], 40);         
        }
        else if(i == PERIOD_TYPE_YEAR)
        {
            nfui_nfbargraph_set_bar_init_gap((NFBARGRAPH *)bargraph[i], 80);
            nfui_nfbargraph_set_bar_gap((NFBARGRAPH *)bargraph[i], 80);     
        }
    }     
    
    nfui_nfobject_set_data(fixed, "hour_bargraph", bargraph[0]);
    nfui_nfobject_set_data(fixed, "day_bargraph", bargraph[1]);
    nfui_nfobject_set_data(fixed, "week_bargraph", bargraph[2]);
    nfui_nfobject_set_data(fixed, "month_bargraph", bargraph[3]);
    nfui_nfobject_set_data(fixed, "year_bargraph", bargraph[4]);
    
	return 0;
}

gint vw_vca_statistic_total_component_show()
{
    return 0;
}

gint vw_vca_statistic_total_component_hide()
{
    return 0;
}

gint vw_vca_statistic_total_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_statistic_total_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

