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
#include "../../service/nf_va_statistic.h"

#include "vw_search_main.h"
#include "vw_vca_statistic_component.h"
#include "vw_statistic_export.h"

#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"

#include "stm.h"
#include "dtf.h"
#include "uxm.h"
#include "var.h"
#include "ix_mem.h"


#define SBV_FIXED1_X            (0)
#define SBV_FIXED1_Y            (0)
#define SBV_FIXED1_W            (470)
#define SBV_FIXED1_H            (903)

#define SBV_FIXED2_X            (SBV_FIXED1_X+SBV_FIXED1_W)
#define SBV_FIXED2_Y            (SBV_FIXED1_Y)
#define SBV_FIXED2_W            (1386)
#define SBV_FIXED2_H            (523)

#define SBV_FIXED3_X            (SBV_FIXED2_X)
#define SBV_FIXED3_Y            (SBV_FIXED2_Y+SBV_FIXED2_H)
#define SBV_FIXED3_W            ((SBV_FIXED2_W/2)-50)
#define SBV_FIXED3_H            (380)

#define SBV_FIXED4_X            (SBV_FIXED3_X+SBV_FIXED3_W)
#define SBV_FIXED4_Y            (SBV_FIXED3_Y)
#define SBV_FIXED4_W            (SBV_FIXED3_W+50)
#define SBV_FIXED4_H            (380)


//////////////////////////////////////

#define SBV_LABEL_X_GAP         (180)
#define SBV_LABEL_Y_GAP         (20)

#define SBV_LABEL_WIDTH         (230)
#define SBV_LABEL_WIDTH_TITLE   (120)
#define SBV_LABEL_HEIGHT        (40)

//////////////////////////////////////


// CHANNEL

#define SBV_CH_LABEL_X          (13)
#define SBV_CH_LABEL_Y          (SBV_LABEL_Y_GAP)


// CALENDAR FIXED 
#define SBV_CAL_FIXED_X         (SBV_CH_LABEL_X)
#define SBV_CAL_FIXED_Y         (SBV_CH_LABEL_Y+SBV_LABEL_HEIGHT+SBV_LABEL_Y_GAP)
#define SBV_CAL_FIXED_W         (410)
#define SBV_CAL_FIXED_H         (380)

#define SBV_CAL_BUTTON_SIZE_W   (150)
#define SBV_CAL_BUTTON_GAP      (6)

#define SBV_CAL_FIRST_BUTTON_X  (51)
#define SBV_CAL_FIRST_BUTTON_Y  (333)

#define SBV_CAL_LAST_BUTTON_X   (SBV_CAL_FIRST_BUTTON_X+SBV_CAL_BUTTON_SIZE_W+SBV_CAL_BUTTON_GAP)
#define SBV_CAL_LAST_BUTTON_Y   (SBV_CAL_FIRST_BUTTON_Y)



// PERIOD

#define SBV_LABEL_X             (SBV_CAL_FIXED_X)
#define SBV_LABEL_Y             (SBV_CAL_FIXED_Y+SBV_CAL_FIXED_H+SBV_LABEL_Y_GAP)

// VA_RULE

#define SBV_RULE_X              (SBV_CAL_FIXED_X)
#define SBV_RULE_Y              (SBV_LABEL_Y+SBV_LABEL_HEIGHT+SBV_LABEL_Y_GAP)  

#define SBV_RULE_OPTFIXED_X     (11)
#define SBV_RULE_OPTFIXED_Y     (SBV_RULE_Y+SBV_LABEL_HEIGHT+10)
#define SBV_RULE_OPTFIXED_W     (417)
#define SBV_RULE_OPTFIXED_H     (290)

#define SBV_RULE_LABEL_WIDTH    (185)

// EXPORT DATA

#define SBV_EXPORT_BUTTON_X     (SBV_CAL_FIXED_X)
#define SBV_EXPORT_BUTTON_Y     (SBV_RULE_OPTFIXED_Y+SBV_RULE_OPTFIXED_H+10)
#define SBV_EXPORT_BUTTON_W     (200)
#define SBV_EXPORT_BUTTON_H     (40)

// TIME SET
#define DAY_TO_SEC              ((time_t)86400)
#define HOUR_TO_SEC             ((time_t)3600)
#define MIN_TO_SEC              ((time_t)60)


#define OPT_RULEID_CNT          (16)
#define OPT_RULETYPE_CNT        (7)

#define OPT_RULETYPE_ALLMSK     (0x373)



////////////////////////////////////////////////////////////
//
// private variables
//

enum {
    STATISTIC_MODE_HOUR = 0,
    STATISTIC_MODE_DAY,
    STATISTIC_MODE_WEEK,
    STATISTIC_MODE_MONTH,
    STATISTIC_MODE_YEAR,
    STATISTIC_MODE_MAX,
};

enum{
    CAL_PREV = 0,
    CAL_NEXT,
    CAL_NONE
};
    

////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_content_fixed;
static NFOBJECT *g_cal_dir[2];
static NFOBJECT *g_cal;
static NFOBJECT *g_cal_text;
static NFOBJECT *g_cal_first_btn;
static NFOBJECT *g_cal_last_btn;

static NFOBJECT *g_ch_obj = NULL;
static NFOBJECT *g_period_obj = NULL;
static NFOBJECT *g_export_obj = NULL;

static NFOBJECT *g_allrule_check;
static NFOBJECT *g_ruletype_check[OPT_RULETYPE_CNT];
static NFOBJECT *g_rulelist_obj;

static NFOBJECT *g_content_fixed;
static NFOBJECT *g_fixed1;
static NFOBJECT *g_fixed2;
static NFOBJECT *g_fixed3;
static NFOBJECT *g_fixed4;

static NFOBJECT *nfbargraph[2];  

static gboolean g_init_sbv = TRUE;
static gint g_cur_ch  = 0;
static time_t g_search_time;
static guint g_rule_onoff[OPT_RULEID_CNT];
static guint g_exist_zone =0;

static VCAZoneData g_zone_data;

static gchar zone_str[16][32];

static const gchar *strWeekDay[]={"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const gchar *strMonth[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};

///////////////////////////////////////////////////////////
//
//  private function
//
static guint _get_start_day_in_week(time_t time)
{
    struct tm ttm;

    localtime_r(&time, &ttm);    
    return ttm.tm_wday;
}

static int _update_calendar_time(time_t timet)
{
    int year, mon, day;
    if (timet == 0) timet = time(0);
    ifn_get_local_day(timet, &year, &mon, &day);
    cw_cld_change_date((CWCALENDAR*)g_cal, year, mon, day);
    return 0;
}

static int _update_calendar_text(time_t timet)
{
    char buf[64];
    int year, mon;
    int length;

    ifn_get_local_day(timet, &year, &mon, 0);

    length = strlen(g_month_str[mon-1]);
    g_utf8_strncpy(buf, g_month_str[mon -1], g_utf8_strlen(g_month_str[mon -1], -1));
    g_sprintf(&buf[length], "  %d", year);

    nfui_nflabel_set_text((NFLABEL*)g_cal_text, buf);
    nfui_signal_emit(g_cal_text, GDK_EXPOSE, FALSE);
    return 0;
}

static int _get_calendar_day(int *year, int *mon, int *day)
{
    if (year) *year = cw_cld_get_current_year((CWCALENDAR*)g_cal);
    if (mon) *mon = cw_cld_get_current_month((CWCALENDAR*)g_cal);
    if (day) *day  = cw_cld_get_current_day((CWCALENDAR*)g_cal);
    return 0;
}

static time_t _get_new_calendar_time()
{
    int year, month, day, hour, min, sec;
    time_t tim;

    _get_calendar_day(&year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, &min, &sec);
    tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);        
    return tim;
}

static time_t _get_text_calendar_time(time_t search_time)
{
    int year, month, day, hour, min, sec;
    time_t tim;

    _get_calendar_day(&year, &month, &day);
    ifn_get_local_hourmin(search_time, &hour, &min, &sec);
    tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);

    return tim;
}

static gint _get_graph_hour_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
    
    ifn_get_local_hourmin(time, &hour, &min, &sec);
    
    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        g_sprintf(strBuf, "%02d:%02d", hour, (i)*5);
            
        strcpy(str[i], strBuf); 
    }
    
    return 0;
}

/*
static gint _get_graph_day_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
    
    dtf_get_local_hourmin(time, &hour, &min, &sec);
    
    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if (!(i%2))
        {
            g_sprintf(strBuf, "%02d", i);
            strcpy(str[i], strBuf); 
        }
        else{
            g_sprintf(strBuf, "%s", " ");
            strcpy(str[i], strBuf);
        }
    }
    
    return 0;
}
*/

static gint _get_graph_day_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;

    ifn_get_local_hourmin(time, &hour, &min, &sec);    

    for( i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(!(i%2))
        {
            g_sprintf(strBuf, "%02d", i);
            strcpy(str[i], strBuf);
        }
        else
        {
            g_sprintf(strBuf, "%s", " ");
            strcpy(str[i], strBuf);
        }
    }

    return 0;
}


static gint _get_graph_hour_text2(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint i=0;

    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(!(i%2))
        {
            g_sprintf(strBuf, "%02d", i);
            strcpy(str[i], strBuf);     
        }
        else
        {
            g_sprintf(strBuf, "%s", " ");
            strcpy(str[i], strBuf); 
        }
    }
    
    return 0;
}

static gint _get_graph_day_text2(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
    
    ifn_get_local_hourmin(time, &hour, &min, &sec);
    
    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if (!(i%2))
        {
            g_sprintf(strBuf, "%02d", i);
            strcpy(str[i], strBuf); 
        }
        else
        {
            g_sprintf(strBuf, "%s", " ");
            strcpy(str[i], strBuf);
        }
    }
    
    return 0;
}


static gint _get_graph_week_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
    
    ifn_get_local_hourmin(time, &hour, &min, &sec);
    
    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%s", strWeekDay[i]);
        strcpy(str[i], strBuf); 
    }
    
    return 0;
}

static gint _get_graph_month_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
        
    ifn_get_local_hourmin(time, &hour, &min, &sec);
    
    for (i = 0; i < cnt; i++)
    {        
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%02d", i+1);
        strcpy(str[i], strBuf); 

    }
    
    return 0;
}

static gint _get_graph_year_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint i;
    
    ifn_get_local_hourmin(time, &hour, &min, &sec);
   
    for (i = 0; i < cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%s", strMonth[i]);
        strcpy(str[i], strBuf); 
    }
    
    return 0;
}

static time_t _get_prev_search_time(gint period_type, time_t cur_time, gint day_cnt,gint year_cnt)
{
    time_t prev_time;

    if (period_type == PERIOD_TYPE_HOUR)        prev_time = cur_time - 3600;
    else if (period_type == PERIOD_TYPE_DAY)    prev_time = cur_time - 3600*24;
    else if (period_type == PERIOD_TYPE_WEEK)   prev_time = cur_time - 3600*24*7;
    else if (period_type == PERIOD_TYPE_MONTH)  prev_time = cur_time - 3600*24*day_cnt;
    else if (period_type == PERIOD_TYPE_YEAR)   prev_time = cur_time - 3600*24*365;
    else g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);
    
    return prev_time;
}

static time_t _get_next_search_time(gint period_type, time_t cur_time, gint day_cnt, gint year_cnt)
{
    time_t next_time;

    if (period_type == PERIOD_TYPE_HOUR)        next_time = cur_time + 3600;
    else if (period_type == PERIOD_TYPE_DAY)    next_time = cur_time + 3600*24;
    else if (period_type == PERIOD_TYPE_WEEK)   next_time = cur_time + 3600*24*7;
    else if (period_type == PERIOD_TYPE_MONTH)  next_time = cur_time + 3600*24*day_cnt;
    else if (period_type == PERIOD_TYPE_YEAR)   next_time = cur_time + 3600*24*year_cnt;
    else g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);
    
    return next_time;
}

static time_t _get_index_search_time(gint period_type, gint index, time_t cur_time)
{
    time_t index_time;

    gint year, month, day;
    gint hour, min, sec;

    ifn_get_local_day(g_search_time, &year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, &min, &sec);    

    if (period_type == PERIOD_TYPE_HOUR)        
    {
        index_time = ifn_get_gmt_from_local(year, month, day, hour, (index+1)*5, sec);
    }
    else if (period_type == PERIOD_TYPE_DAY)    
    {
        index_time = ifn_get_gmt_from_local(year, month, day, index, min, sec);
    }
    else if (period_type == PERIOD_TYPE_WEEK)   
    {
        day -= _get_start_day_in_week(g_search_time);
        
        index_time = ifn_get_gmt_from_local(year, month, day+index, hour, min, sec);
    }
    else if (period_type == PERIOD_TYPE_MONTH)  
    {
        index_time = ifn_get_gmt_from_local(year, month, (index+1), hour, min, sec);
    }
    else if (period_type == PERIOD_TYPE_YEAR)   
    {
        index_time = ifn_get_gmt_from_local(year, (index+1), day, hour, min, sec);
    }
    else 
        g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);
    
    return index_time;
}

static void _get_statistic_hour_section(time_t *start_time, time_t *end_time)
{
    time_t tmp_time;
    gint mk_hour = 3600;

    gint year, month, day;
    gint hour;

    ifn_get_local_day(g_search_time, &year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, 0, 0);  
    tmp_time = ifn_get_gmt_from_local(year, month, day, hour, 0, 0);

    if(DAL_get_dst()== 0 && ifn_is_in_dst(tmp_time)==1)    tmp_time += 3600;
        
    *start_time = tmp_time;   
    *end_time = tmp_time + mk_hour-1; 
}

static void _get_statistic_day_section(time_t *start_time, time_t *end_time)
{
    struct tm ttm;
    time_t tmp_time;
    gint mk_day = 3600*24;

    gint year, month, day;

    ifn_get_local_day(g_search_time, &year, &month, &day);
    tmp_time = ifn_get_gmt_from_local(year, month, day, 0, 0, 0);

    if(DAL_get_dst()== 0 && ifn_is_in_dst(tmp_time)==1)    tmp_time += 3600;
    
    *start_time = tmp_time;   
    *end_time = tmp_time + mk_day-1;  
}

static void _get_statistic_week_section(time_t *start_time, time_t *end_time)
{
    time_t tmp_time;
    gint mk_day = 3600*24;

    gint year, month, day;
    gint wday;

    ifn_get_local_day(g_search_time, &year, &month, &day);
    day -= _get_start_day_in_week(g_search_time);
    tmp_time = ifn_get_gmt_from_local(year, month, day, 0, 0, 0);

    if(DAL_get_dst()== 0 && ifn_is_in_dst(tmp_time)==1)    tmp_time += 3600;

    *start_time = tmp_time;   
    *end_time = (tmp_time + mk_day*7)-1;  
}

static void _get_statistic_month_section(time_t *start_time, time_t *end_time)
{
    time_t tmp_time;
    gint mk_day = 3600*24;

    gint year, month;
    gint days_count;
    
    ifn_get_local_day(g_search_time, &year, &month, 0);
    tmp_time = ifn_get_gmt_from_local(year, month, 1, 0, 0, 0);

    days_count = cw_cld_get_days_in_month((CWCALENDAR*)g_cal, year, month);
    
    if(DAL_get_dst()== 0 && ifn_is_in_dst(tmp_time)==1)    tmp_time += 3600;

    *start_time = tmp_time;   
    *end_time = (tmp_time + mk_day*days_count)-1; 
}

static void _get_statistic_year_section(time_t *start_time, time_t *end_time)
{
    time_t tmp_time;
    gint mk_day = 3600*24;

    gint year;
    gint days_count;
    
    ifn_get_local_day(g_search_time, &year, 0, 0);
    tmp_time = ifn_get_gmt_from_local(year, 1, 1, 0, 0, 0);

    days_count = cw_cld_get_days_in_year((CWCALENDAR*)g_cal, year);
    
    if(DAL_get_dst()== 0 && ifn_is_in_dst(tmp_time)==1)    tmp_time += 3600;
        
    *start_time = tmp_time;   
    *end_time = (tmp_time + mk_day*days_count)-1; 
}

static gint _get_statistic_section(gint period_type, time_t *start_time, time_t *end_time)
{
    gchar start_strBuf[64]; 
    gchar end_strBuf[64];

    if (period_type == PERIOD_TYPE_HOUR) _get_statistic_hour_section(start_time, end_time);
    else if (period_type == PERIOD_TYPE_DAY) _get_statistic_day_section(start_time, end_time);
    else if (period_type == PERIOD_TYPE_WEEK) _get_statistic_week_section(start_time, end_time);
    else if (period_type == PERIOD_TYPE_MONTH) _get_statistic_month_section(start_time, end_time);
    else if (period_type == PERIOD_TYPE_YEAR) _get_statistic_year_section(start_time, end_time);    
    else g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);

    memset(start_strBuf, 0x00, sizeof(start_strBuf));
    ifn_get_localtime_text(*start_time, YYYYMMDD, H24, start_strBuf);
    
    memset(end_strBuf, 0x00, sizeof(end_strBuf));
    ifn_get_localtime_text(*end_time, YYYYMMDD, H24, end_strBuf);

    g_message("\njaeyoung set >> %s, %d >> jaeyoung start time :%s, end:%s\n", __FUNCTION__, __LINE__, start_strBuf, end_strBuf);

    return 0;
}

static gint _get_statistic_info(STATISTIC_COMPONENT_DATA_T *component_data, va_statistic_t *statistic_data)
{
    GTimeVal start_time, end_time;
    gint i;
    
    _get_statistic_section(component_data->total.period_type, &start_time.tv_sec, &end_time.tv_sec);
    start_time.tv_usec = 0;
    end_time.tv_usec = 0;

    statistic_data->ch = g_cur_ch;
    statistic_data->start_time = GTIMEVAL_TO_GUINT64(start_time);
    statistic_data->end_time = GTIMEVAL_TO_GUINT64(end_time);
    statistic_data->period = component_data->total.period_type;

    component_data->total.start_time = start_time.tv_sec;
    component_data->total.end_time = end_time.tv_sec;

    for (i = 0; i < 16; i++)
    {
        statistic_data->rule_list[i] = g_rule_onoff[i];
    }

    return 0;
}

static gint _get_statistic_data(va_statistic_t *statistic_data)
{
    gint ret;

    gchar start_strBuf[64];
    gchar end_strBuf[64];
    
    ret = get_va_statistic(statistic_data);

    return ret;
}

static gint _get_statistic_total_value(gint cnt, gint *data, gint *val, gint *sum)
{
    gint i;

    *sum = 0;
    
    for (i = 0; i < cnt; i++)
    {
        val[i] = data[i];
        *sum +=  data[i];
    }    

    return 0;
}

static gint _get_statistic_average_value(gint cnt, gint *data, gint *val)
{
    gint i;
    
    for (i = 0; i < cnt; i++)
    {
        val[i] = data[i];
    }
            
    return 0;
}

static gint _get_statistic_rate_value(gint cnt, gint *data, gint *val, gdouble *ratio)
{
    gint i, sum = 0;

    for (i = 0; i < cnt;i++)
    {
        val[i] = data[i];
        sum += data[i];
    }

    for (i = 0; i < cnt;i++)
    {    
        ratio[i] = ((gdouble)val[i]/(gdouble)sum)*100;
    }

    return 0;
}

static gint _set_statistic_data(STATISTIC_COMPONENT_DATA_T *component_data, va_statistic_t *statistic_data)
{

// TOTAL COMPONENT
    if (component_data->total.period_type == PERIOD_TYPE_HOUR) 
        _get_statistic_total_value(component_data->total.hour_cnt, statistic_data->main_data, component_data->total.hour_value, &component_data->total.event_count);
    else if (component_data->total.period_type == PERIOD_TYPE_DAY)
        _get_statistic_total_value(component_data->total.day_cnt, statistic_data->main_data, component_data->total.day_value, &component_data->total.event_count);
    else if (component_data->total.period_type == PERIOD_TYPE_WEEK)
        _get_statistic_total_value(component_data->total.week_cnt, statistic_data->main_data, component_data->total.week_value, &component_data->total.event_count);    
    else if (component_data->total.period_type == PERIOD_TYPE_MONTH)
        _get_statistic_total_value(component_data->total.month_cnt, statistic_data->main_data, component_data->total.month_value, &component_data->total.event_count);    
    else if (component_data->total.period_type == PERIOD_TYPE_YEAR)
        _get_statistic_total_value(component_data->total.year_cnt, statistic_data->main_data, component_data->total.year_value, &component_data->total.event_count);    
    else 
        g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);

// AVERAGE COMPONENT
    if (component_data->average.average_type == AVERAGE_TYPE_HOUR) 
        _get_statistic_average_value(component_data->average.hour_cnt, statistic_data->average_data[0], component_data->average.hour_value);
    else if (component_data->average.average_type == AVERAGE_TYPE_DAY) 
        _get_statistic_average_value(component_data->average.day_cnt, statistic_data->average_data[2], component_data->average.day_value);
    else if (component_data->average.average_type == AVERAGE_TYPE_WEEK) 
        _get_statistic_average_value(component_data->average.week_cnt, statistic_data->average_data[1], component_data->average.week_value);
    else if (component_data->average.average_type == AVERAGE_TYPE_NONE) 
        g_message("%s, %d, none average type", __FUNCTION__, __LINE__);    
    else 
        g_message("%s, %d, undefined average type", __FUNCTION__, __LINE__);

// RATE COMPONENT   
    _get_statistic_rate_value(16, statistic_data->zone_data, component_data->rate.zone_val, component_data->rate.zone_ratio_val);
    _get_statistic_rate_value(7, statistic_data->event_data, component_data->rate.event_val, component_data->rate.event_ratio_val);
    
    return 0;
}

static gboolean _get_statistic_export_info(STATISTIC_EXPORT_T *export_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data; 
    gint i;

    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)g_curwnd, STATISTIC_COMPONENT_DATA);

    // export data
    export_data->ch= g_cur_ch;
    export_data->start_time = component_data->total.start_time;
    export_data->end_time = component_data->total.end_time;
    export_data->period = component_data->total.period_type;
    export_data->total_events = component_data->total.event_count;
    export_data->exist_zone = g_exist_zone;

    for(i =0; i<16; i++)
    {
        export_data->rule_list[i] = g_rule_onoff[i];       
    }      
    
    return 0;
}


static gint _prepare_display_statistic()
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    va_statistic_t statistic_data;

    memset(&statistic_data, 0x00, sizeof(va_statistic_t));

    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)g_curwnd, STATISTIC_COMPONENT_DATA);

    _get_statistic_info(component_data, &statistic_data);
    _get_statistic_data(&statistic_data);
    _set_statistic_data(component_data, &statistic_data);
    
    return 0;
}

static gint _total_previous_cb(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;
    gint idx; 

    time_t prev_time;

    gint day_cnt, year_cnt;
    gint year, month; 

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    ifn_get_local_day(g_search_time, &year, &month, 0);

    day_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);    
    year_cnt = cw_cld_get_days_in_year((CWCALENDAR *)g_cal, year); 

    prev_time = _get_prev_search_time(component_data->total.period_type, g_search_time, day_cnt, year_cnt);

    _update_calendar_time(prev_time);
    _update_calendar_text(prev_time);
    
    component_data->total.calendar_time = prev_time;
    component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);
 
    _get_graph_month_text(prev_time, component_data->total.month_cnt, component_data->total.month_text);   
    _get_graph_hour_text(prev_time, component_data->total.hour_cnt, component_data->total.hour_text);

    g_search_time = prev_time;     
    _prepare_display_statistic();
    
    vw_vca_statistic_total_component_sync_data(g_fixed2);
    vw_vca_statistic_average_component_sync_data(g_fixed3);
    vw_vca_statistic_rate_component_sync_data(g_fixed4);

    stm_set_time_t(prev_time);

    return 0;
}

static gint _total_next_cb(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;
    gint idx; 

    time_t next_time;
    gint day_cnt, year_cnt;
    gint year, month; 

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    ifn_get_local_day(g_search_time, &year, &month, 0);

    day_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);
    year_cnt = cw_cld_get_days_in_year((CWCALENDAR *)g_cal, year);  

    next_time = _get_next_search_time(component_data->total.period_type, g_search_time, day_cnt, year_cnt);

    _update_calendar_time(next_time);
    _update_calendar_text(next_time);
    
    component_data->total.calendar_time = next_time;
    component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);

    _get_graph_month_text(next_time, component_data->total.month_cnt, component_data->total.month_text);   
    _get_graph_hour_text(next_time, component_data->total.hour_cnt, component_data->total.hour_text);
    
    g_search_time = next_time;
    _prepare_display_statistic();
    
    
    vw_vca_statistic_total_component_sync_data(g_fixed2);
    vw_vca_statistic_average_component_sync_data(g_fixed3);
    vw_vca_statistic_rate_component_sync_data(g_fixed4);

    stm_set_time_t(next_time);
 
    return 0;
}

static gint _total_graph_press(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);
    
    //vw_vca_statistic_average_component_sync_data(g_fixed3);
    
    return 0;
}

static gint _total_graph_2press(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;
    gint idx; 

    time_t index_time;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    index_time = _get_index_search_time(component_data->total.period_type, component_data->total.bar_idx, g_search_time);
    
    component_data->total.calendar_time = index_time;
    component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);

    _get_graph_hour_text(index_time, component_data->total.hour_cnt, component_data->total.hour_text);    

    switch(component_data->total.period_type)
    {
        case PERIOD_TYPE_DAY  :
            component_data->total.period_type = PERIOD_TYPE_HOUR;
            component_data->average.average_type = AVERAGE_TYPE_NONE;                         
        break;
        
        case PERIOD_TYPE_WEEK :
            component_data->total.period_type = PERIOD_TYPE_DAY;
            component_data->average.average_type = AVERAGE_TYPE_HOUR;     
        break;
        
        case PERIOD_TYPE_MONTH:
            component_data->total.period_type = PERIOD_TYPE_DAY;
            component_data->average.average_type = AVERAGE_TYPE_HOUR;
        break;
        
        case PERIOD_TYPE_YEAR :
            component_data->total.period_type = PERIOD_TYPE_MONTH;
            component_data->average.average_type = AVERAGE_TYPE_WEEK;                     
        break;
    }

    g_search_time = index_time;    
    _prepare_display_statistic();

    vw_vca_statistic_total_component_sync_data(g_fixed2);
    vw_vca_statistic_average_component_sync_data(g_fixed3);
    vw_vca_statistic_rate_component_sync_data(g_fixed4);

    stm_set_time_t(index_time);

    nfui_combobox_set_index(g_period_obj, component_data->total.period_type);        
    nfui_signal_emit(g_period_obj, GDK_EXPOSE, TRUE);   
    
    return 0;
}

static gint _average_type_cb(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    _prepare_display_statistic();  
    vw_vca_statistic_average_component_sync_data(g_fixed3);
    
    return 0;
}

static gint _init_statistic_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
   
    component_data = imalloc(sizeof(STATISTIC_COMPONENT_DATA_T));        
    nfui_nfobject_set_alloc_data(top, STATISTIC_COMPONENT_DATA, component_data);

    return 0;
}

static gint _init_statistic_component_action(NFOBJECT *top)
{
    STATISTIC_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(STATISTIC_COMPONENT_ACTION_T));

    component_action->total_previous_cb = _total_previous_cb;
    component_action->total_next_cb = _total_next_cb;  
    component_action->total_graph_press = _total_graph_press;
    component_action->total_graph_2press = _total_graph_2press;

    component_action->average_type_cb = _average_type_cb;

    nfui_nfobject_set_alloc_data(top, STATISTIC_COMPONENT_ACTION, component_action );

    return 0;
}

static gint _init_statistic_total_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    gint i;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
        
    component_data->total.calendar_time = g_search_time;
    
    component_data->total.event_count = 0;
    component_data->total.period_type = STATISTIC_MODE_DAY;

    component_data->total.init_period_type = -1;
    
    component_data->total.bar_idx = 0;

    component_data->total.hour_cnt = 12;
    component_data->total.day_cnt = 24;
    component_data->total.week_cnt = 7;    
    component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);    
    component_data->total.year_cnt = 12;

    _get_graph_hour_text(g_search_time, component_data->total.hour_cnt, component_data->total.hour_text);   
    _get_graph_day_text(g_search_time, component_data->total.day_cnt, component_data->total.day_text);
    _get_graph_week_text(g_search_time, component_data->total.week_cnt, component_data->total.week_text);
    _get_graph_month_text(g_search_time, component_data->total.month_cnt, component_data->total.month_text);    
    _get_graph_year_text(g_search_time, component_data->total.year_cnt, component_data->total.year_text);        
    return 0;
}

static gint _init_statistic_average_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;

    gchar strBuf[64];
    gint i;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

    component_data->average.average_type = AVERAGE_TYPE_NONE;

    component_data->average.hour_cnt = 24;
    component_data->average.week_cnt = 7;
    component_data->average.day_cnt = 31;

    _get_graph_hour_text2(g_search_time, component_data->average.hour_cnt, component_data->average.hour_text);      
    _get_graph_day_text2(g_search_time, component_data->average.day_cnt, component_data->average.day_text);        
    _get_graph_week_text(g_search_time, component_data->average.week_cnt, component_data->average.week_text);    

   
     
    return 0;
}

static gint _init_statistic_rate_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    gint i;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

    for(i=0; i<16; i++)
    {
        strcpy(component_data->rate.zone_str[i],zone_str[i]);
    }
    
    return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

static gboolean post_prev_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        time_t cal_time;
                           
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
              
        cal_time = cw_cld_set_prev_month((CWCALENDAR*)g_cal);
        if (cal_time == 0) return FALSE;
        _update_calendar_time(cal_time);
        _update_calendar_text(cal_time);

        component_data->total.calendar_time = cal_time;
        component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);
    
        _get_graph_hour_text(cal_time, component_data->total.hour_cnt, component_data->total.hour_text);

        g_search_time = cal_time;
        _prepare_display_statistic();

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);

        stm_set_time_t(cal_time);
             
    }
    
    return FALSE;
}

static gboolean post_next_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        time_t cal_time;
                           
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
        
        cal_time = cw_cld_set_next_month((CWCALENDAR*)g_cal);
        if(cal_time == 0) return FALSE;

        _update_calendar_time(cal_time);
        _update_calendar_text(cal_time);
        
        component_data->total.calendar_time = cal_time;
        component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);  

        _get_graph_hour_text(cal_time, component_data->total.hour_cnt, component_data->total.hour_text);

        g_search_time = cal_time; 
        _prepare_display_statistic();
        
        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);

        stm_set_time_t(cal_time);
                   
    }
    
    return FALSE;
}


static gboolean post_calendar_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CALENDAR_CHANGED_RELEASE)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        time_t cal_time;
                           
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
    
        cal_time = _get_new_calendar_time();
        
        _update_calendar_text(cal_time);
        
        component_data->total.calendar_time = cal_time;
        component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);       

        g_search_time = cal_time;
        _prepare_display_statistic();

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);        

        stm_set_time_t(cal_time);
    }
    
    return FALSE;
}

static gboolean post_first_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        time_t first_time;
    
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
                           
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        first_time = VW_Search_Get_Record_FirstTime();        
        if (first_time == 0) return FALSE;
        
        _update_calendar_time(first_time);
        _update_calendar_text(first_time);

        component_data->total.calendar_time = first_time;
        component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);

        g_search_time = first_time;    
        _prepare_display_statistic();

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);

        stm_set_time_t(first_time);
    }
    
    return FALSE;
        
}

static gboolean post_last_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        time_t last_time;
    
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
                           
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

        last_time = VW_Search_Get_Record_LastTime();        
        if (last_time == 0) return FALSE;

        _update_calendar_time(last_time);
        _update_calendar_text(last_time);

        component_data->total.calendar_time = last_time;
        component_data->total.month_cnt = cw_cld_get_lastday((CWCALENDAR*)g_cal);

        g_search_time = last_time;    
        _prepare_display_statistic();

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);

        stm_set_time_t(last_time);        
    }
    
    return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint i;
        gchar *strRule;
        gchar strBuf[64];   
        
        g_message("%s, %d", __FUNCTION__, __LINE__);

        g_cur_ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        DAL_get_vca_zone_data(&g_zone_data,g_cur_ch);

        nfui_listbox_delete_all(g_rulelist_obj);
        for (i = 0; i < OPT_RULEID_CNT; i++)
        {
            strRule = imalloc(sizeof(gchar)*32);
            if(strlen(g_zone_data.zone[i].name))
            {
                g_sprintf(strRule, "%s", g_zone_data.zone[i].name);
                strcpy(zone_str[i], strRule);
            }
            else
            {
                g_sprintf(strRule, "ZONE %d", i+1);
                strcpy(zone_str[i], strRule);
            }
            
            nfui_listbox_set_text(NF_LISTBOX(g_rulelist_obj), &strRule);
            ifree(strRule);

            nfui_signal_emit(g_rulelist_obj, GDK_EXPOSE, TRUE);
            g_rule_onoff[i] = OPT_RULETYPE_ALLMSK;
        }

        _init_statistic_rate_component_data(g_curwnd);
        _prepare_display_statistic();   

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);        
    }

    return FALSE;
}

static gboolean post_period_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        STATISTIC_COMPONENT_ACTION_T *component_action;
        NFOBJECT *top;
        gint idx;

        g_message("%s, %d", __FUNCTION__, __LINE__);
                   
        top = nfui_nfobject_get_top(obj);
        component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
        component_action = (STATISTIC_COMPONENT_ACTION_T *)(nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_ACTION));

        idx = nfui_combobox_get_cur_index(obj);
        component_data->total.period_type = idx;   
        
        switch(idx){
            case PERIOD_TYPE_HOUR :
            case PERIOD_TYPE_DAY  :            
                component_data->average.average_type = AVERAGE_TYPE_NONE;        
            break;
            
            case PERIOD_TYPE_WEEK :
                component_data->average.average_type = AVERAGE_TYPE_HOUR;        
            break;
            
            case PERIOD_TYPE_MONTH:
                component_data->average.average_type = AVERAGE_TYPE_WEEK;
            break;
            
            case PERIOD_TYPE_YEAR :
                component_data->average.average_type = AVERAGE_TYPE_DAY;
            break;
        }

        _prepare_display_statistic();

        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);
    }
    
    return FALSE;
}

static gboolean post_allrule_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gboolean state;
        gint focus_idx, list_idx;
        gint i;

        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        focus_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
        if (focus_idx < 0) return FALSE;
        
        list_idx = focus_idx; 
        
        for (i = 0; i < OPT_RULETYPE_CNT; i++)
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], state);           
            nfui_signal_emit(g_ruletype_check[i], GDK_EXPOSE, TRUE);
        }

        if (state) g_rule_onoff[list_idx] = OPT_RULETYPE_ALLMSK;
        else g_rule_onoff[list_idx] = 0;

        _prepare_display_statistic();  
    
        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);
    }

    return FALSE;
}

static gboolean post_ruletype_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gboolean state;
        gint focus_idx, list_idx;
        gint i;

        for (i = 0; i < OPT_RULETYPE_CNT; i++)
        {
            if (g_ruletype_check[i] == obj) break;            
        }

        if (i == OPT_RULETYPE_CNT) return FALSE;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        focus_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_rulelist_obj);
        if (focus_idx < 0) return FALSE;
        
        list_idx = focus_idx;

        if (state)
        {
            if(i==0)         g_rule_onoff[list_idx] |= IVCA_ET_DIR_POS;
            else if(i==1)    g_rule_onoff[list_idx] |= IVCA_ET_DIR_NEG;
            else if(i==2)    g_rule_onoff[list_idx] |= IVCA_ET_ENTER;
            else if(i==3)    g_rule_onoff[list_idx] |= IVCA_ET_EXIT;
            else if(i==4)    g_rule_onoff[list_idx] |= IVCA_ET_STOPPED;
            else if(i==5)    g_rule_onoff[list_idx] |= IVCA_ET_REMOVED;
            else if(i==6)    g_rule_onoff[list_idx] |= IVCA_ET_LOITERED;
        }
        else
        {
            if(i==0)         g_rule_onoff[list_idx] &= ~IVCA_ET_DIR_POS;
            else if(i==1)    g_rule_onoff[list_idx] &= ~IVCA_ET_DIR_NEG;
            else if(i==2)    g_rule_onoff[list_idx] &= ~IVCA_ET_ENTER;
            else if(i==3)    g_rule_onoff[list_idx] &= ~IVCA_ET_EXIT;            
            else if(i==4)    g_rule_onoff[list_idx] &= ~IVCA_ET_STOPPED;
            else if(i==5)    g_rule_onoff[list_idx] &= ~IVCA_ET_REMOVED;
            else if(i==6)    g_rule_onoff[list_idx] &= ~IVCA_ET_LOITERED;
        }

        if (g_rule_onoff[list_idx] == OPT_RULETYPE_ALLMSK) 
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_allrule_check, TRUE);
            nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_allrule_check, FALSE);
            nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
        }

        _prepare_display_statistic();  
    
        vw_vca_statistic_total_component_sync_data(g_fixed2);
        vw_vca_statistic_average_component_sync_data(g_fixed3);
        vw_vca_statistic_rate_component_sync_data(g_fixed4);

    }

    return FALSE;
}

static gboolean post_ruleid_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_LISTBOX_CHANGED)
    {
        gboolean state;
        gint focus_idx, list_idx;
        gint i;
        
        focus_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);
        
        if (focus_idx >= 0) 
        {
            if (nfui_nfobject_is_disabled(g_allrule_check) == TRUE)
            {
                nfui_nfobject_enable(g_allrule_check);
            
                for (i = 0; i < OPT_RULETYPE_CNT; i++)
                {
                    nfui_nfobject_enable(g_ruletype_check[i]);
                }
            }

            list_idx = focus_idx;

            if (g_rule_onoff[list_idx] == OPT_RULETYPE_ALLMSK) 
            {
                nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_allrule_check, TRUE);
                nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
            }
            else if (g_rule_onoff[list_idx] == 0)
            {
                nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_allrule_check, FALSE);
                nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
            }
            else
            {
                nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
            }

            for (i = 0; i < OPT_RULETYPE_CNT; i++)
            {
                if(i==0)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_DIR_POS) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==1)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_DIR_NEG) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==2)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_ENTER) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==3)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_EXIT) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==4)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_STOPPED) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==5)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_REMOVED) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                else if(i==6)
                {
                    if (g_rule_onoff[list_idx] & IVCA_ET_LOITERED) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], TRUE);
                    else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ruletype_check[i], FALSE);
                }
                
                nfui_signal_emit(g_ruletype_check[i], GDK_EXPOSE, TRUE);
            }       
        }
        else
        {
            if (nfui_nfobject_is_disabled(g_allrule_check) == FALSE)
            {
                nfui_nfobject_disable(g_allrule_check);
                nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
            
                for (i = 0; i < OPT_RULETYPE_CNT; i++)
                {
                    nfui_nfobject_disable(g_ruletype_check[i]);
                    nfui_signal_emit(g_ruletype_check[i], GDK_EXPOSE, TRUE);
                }
            }
        }              
    }

    return FALSE;
}

static gboolean post_export_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        STATISTIC_EXPORT_T export_data;
        GdkDrawable *drawable;
        GdkRectangle rect;
    
        if(evt->button.button == MOUSE_RIGTH_BUTTON){
            return FALSE;
        }

        memset(&export_data, 0x00, sizeof(STATISTIC_EXPORT_T));

        _get_statistic_export_info(&export_data);

        if(export_data.total_events < 1)
        {
            nftool_mbox(g_curwnd, "NOTICE", "There is no data to export.\n", 
                                NFTOOL_MB_OK);
            return FALSE;
        }

        drawable = nfui_nfobject_get_window((NFOBJECT*)obj);

        nfui_nfobject_get_offset(g_content_fixed, &rect.x, &rect.y);
        rect.width = g_content_fixed->width;
        rect.height = g_content_fixed->height;

        export_data.pbuf = nfui_get_pixbuf_from_drawable(drawable, &rect, 0);

        VW_StatisticExport_Open(g_curwnd, &export_data);
        
        g_object_unref(export_data.pbuf);
    }
 
    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON){                   
            return FALSE;
        }

        vsm_playback_preview_stop();
        VW_Search_Destroy();
    }
    
    return FALSE;
}

static gboolean post_fixed1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*) obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+obj->width-1, gap_y, 2, obj->height);

            g_object_unref(gc);
        }
        break;

        case GDK_DELETE:
        {

        }
        break;

        default:

        break;
    }

    return FALSE;
}

static gboolean post_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y+obj->height-1, obj->width, 1);

            g_object_unref(gc);
        }
        break;

    }

    return FALSE;
}

static gboolean post_fixed3_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+obj->width-1, gap_y, 2, obj->height);

            g_object_unref(gc);
        }
        break;

        default :
        break;
    }

    return FALSE;
}

static gboolean post_fixed4_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
           
        }
        break;

        default :
        break;
    }

    return FALSE;
}

static gboolean post_opt_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean post_cal_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint pos_x, pos_y;

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
            nfutil_draw_image(drawable, gc, IMG_CALENDAR_BG, pos_x, pos_y, -1, -1, NFALIGN_LEFT, 0);
            g_object_unref(gc);

//            _update_calendar_time(g_search_time);
//            _update_calendar_text(g_search_time);
        }
        break;

        case GDK_DELETE:
        break;

        default:

        break;
    }
    
    return FALSE;   
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////

      /* :  VA_statistic  main_fixed.

      --------------------------------------------
      |  ---------------------------------------  |
      | |           |                           | |
      | |           |                           | |
      | |           |          FIXED2           | |
      | |           |                           | |
      | |           |                           | |
      | |  FIXED1   |                           | |
      | |           |---------------------------| |
      | |           |            |              | |
      | |           |   FIXED3   |    FIXED4    | |
      | |           |            |              | |
      | |           |            |              | |
      | |           |            |              | |
      |  ---------------------------------------  |
      |                                           |
      |             PARENT                        |
      |-------------------------------------------|  */

////////////////////////////////////////////////////////////


void vw_init_SearchBy_statistic_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;

    NFOBJECT *opt_fixed;
    NFOBJECT *optsub_fixed;
    NFOBJECT *cal_fixed;

    GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];

    const gchar *strTitle[]={"CHANNEL","PERIOD","RULE","AVERAGE","ZONE","EVENT"};
    const gchar *calButton[] ={"FIRST","LAST"};
    const gchar *strButton[] ={"EXPORT DATA","CLOSE"};
    const gchar *strDay[]={"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const gchar *strPeriod[]={"HOUR","DAY","WEEK","MONTH","YEAR"};
    const gchar *strEvent[]={"FORWARD","REVERSE","VCA-ENTER","VCA-EXIT","STOPPED","REMOVED","LOITERING"};
                    
    gchar *strCh[GUI_CHANNEL_CNT];
    gchar *strRule;
    gchar strBuf[64];

    gint year, month, day, hour, min, sec;

    guint ch, cnt;
    guint size_w, size_h;
    guint opt = 0;
    
    gint pos_x, pos_y;
    gint i, j;
    
    int length;

    arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
    arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
    arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
    arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

    arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
    arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
    arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
    arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);
    
         
    g_curwnd = nfui_nfobject_get_top(parent);
    
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
    g_content_fixed = content_fixed;

    stm_get_time_range_t(0, &g_search_time);
    ifn_get_local_day(g_search_time, &year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, &min, &sec);

    g_fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(g_fixed1, SBV_FIXED1_W, SBV_FIXED1_H);
    nfui_nfobject_show(g_fixed1);
    nfui_nfobject_modify_bg(g_fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, g_fixed1, SBV_FIXED1_X, SBV_FIXED1_Y);
    nfui_regi_post_event_callback(g_fixed1,post_fixed1_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[0], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH_TITLE, SBV_LABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_fixed1, obj, SBV_CH_LABEL_X, SBV_CH_LABEL_Y);

    for (ch = 0; ch<GUI_CHANNEL_CNT; ch++)
    {
        strCh[ch] = imalloc(sizeof(gchar) *64);
        sprintf(strCh[ch], "CH %02d", ch+1);
    }
    
    obj =nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj),NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH, SBV_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_fixed1, obj, SBV_LABEL_X_GAP, SBV_CH_LABEL_Y);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_ch_obj = obj;

    g_cur_ch = 0;

    for(ch=0; ch<GUI_CHANNEL_CNT; ch++)
        ifree(strCh[ch]);

    cal_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(cal_fixed, SBV_CAL_FIXED_W, SBV_CAL_FIXED_H);
    nfui_nfobject_show(cal_fixed);
    nfui_nfobject_modify_bg(cal_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)g_fixed1, cal_fixed, SBV_CAL_FIXED_X, SBV_CAL_FIXED_Y);
    
    for(i=0; i<2; i++){
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[i]);
        nfui_nfobject_set_size(obj, 27, 27);
        nfui_nfobject_show(obj);

        if( i == 0){
            nfui_regi_post_event_callback(obj, post_prev_cal_event_handler);
            nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 34, 7);
            g_cal_dir[0] = obj;
        }else{
            nfui_regi_post_event_callback(obj, post_next_cal_event_handler);
            nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 349, 7);
            g_cal_dir[1] = obj;
        }
    }

    // CALENDAR MONTH/YEAR

    length = strlen(g_month_str[month - 1]);
    g_utf8_strncpy(strBuf, g_month_str[month -1],g_utf8_strlen(g_month_str[month-1],-1));
    g_sprintf(&strBuf[length],"  %d",year);

    g_cal_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(607));
    nfui_nfobject_set_size(g_cal_text, (guint)(282) , (guint)(34));
    nfui_nflabel_set_align((NFLABEL*)g_cal_text, NFALIGN_CENTER,0);
    nfui_nfobject_modify_bg(g_cal_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
    nfui_nfobject_use_focus(g_cal_text, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(g_cal_text);
    nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal_text, (guint)(63), (guint)(4));


    // CALENDAR LABEL
    gint cal_lbl_pos_x[7] = {47, 93, 137, 182, 226, 269, 315};
    for(i = 0 ; i < 7 ; i++)
    {
        if (i == 0){
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i], 
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(609));
        }else if (i == 6){
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i], 
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(611));
        }else{
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i], 
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(610));
        }
        
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 46, 40);
        nfui_nfobject_show(obj);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(605));
        nfui_nffixed_put((NFFIXED*)cal_fixed, obj, cal_lbl_pos_x[i], 43);
        
    }

    g_cal = (NFOBJECT*)cw_cld_new(g_search_time, 44, 40);
    nfui_regi_post_event_callback(g_cal, post_calendar_event_cb);
    nfui_nfobject_show(g_cal);
    nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal, 50, 85);

    // CALENDER

    obj = nftool_normal_button_create_type3(calButton[0], SBV_CAL_BUTTON_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_first_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBV_CAL_FIRST_BUTTON_X, SBV_CAL_FIRST_BUTTON_Y);
    g_cal_first_btn = obj;
    
    obj = nftool_normal_button_create_type3(calButton[1], SBV_CAL_BUTTON_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_regi_post_event_callback(obj, post_last_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBV_CAL_LAST_BUTTON_X, SBV_CAL_LAST_BUTTON_Y);
    g_cal_last_btn = obj;

    // PERIOD.
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH_TITLE, SBV_LABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_fixed1, obj, SBV_LABEL_X, SBV_LABEL_Y);

    obj = nfui_combobox_new(strPeriod, 5, 1);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj),NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH, SBV_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_fixed1, obj, SBV_LABEL_X_GAP, SBV_LABEL_Y);
    nfui_regi_post_event_callback(obj, post_period_event_handler);
    g_period_obj = obj;

    // RULE                    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[2], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH_TITLE, SBV_LABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_fixed1, obj, SBV_RULE_X, SBV_RULE_Y);

    opt_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(opt_fixed, SBV_RULE_OPTFIXED_W, SBV_RULE_OPTFIXED_H);
    nfui_nfobject_show(opt_fixed);
    nfui_nfobject_modify_bg(opt_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nffixed_put((NFFIXED*)g_fixed1, opt_fixed, SBV_RULE_OPTFIXED_X, SBV_RULE_OPTFIXED_Y);

    pos_x = 10;
    pos_y = 10;

    size_w = 200;
    
    obj = nfui_listbox_new(1, &size_w, 34);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj),NFLISTBOX_TYPE_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nfobject_set_size(obj, size_w, 34*8);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_ruleid_list_event_handler);
    g_rulelist_obj = obj;

    DAL_get_vca_zone_data(&g_zone_data,0);

    for (i = 0; i < OPT_RULEID_CNT; i++)
    {
        strRule = imalloc(sizeof(gchar)*32);
        if(strlen(g_zone_data.zone[i].name))
        {
            g_sprintf(strRule, "%s", g_zone_data.zone[i].name);
            strcpy(zone_str[i], strRule);
        }          
        else
        {
            g_sprintf(strRule, "ZONE %d", i+1);
            strcpy(zone_str[i], strRule);
        }
        
        nfui_listbox_set_text(NF_LISTBOX(g_rulelist_obj), &strRule);
        ifree(strRule);

        g_exist_zone |= (1<<i);
        g_rule_onoff[i] = OPT_RULETYPE_ALLMSK;
    }
        
    pos_x += (size_w+20);
        
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_allrule_check_event_handler);
    g_allrule_check = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(obj, SBV_LABEL_WIDTH_TITLE, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x+34, pos_y);
        
    for (i = 0; i < OPT_RULETYPE_CNT; i++)
    {       
        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_disable(obj);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x, pos_y + 34 + 34*i + (34-size_h)/2);
        nfui_regi_post_event_callback(obj, post_ruletype_check_event_handler);
        g_ruletype_check[i] = obj;    
        
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%s", strEvent[i]);
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(obj, 150, 34);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x+34, pos_y + 34 + 34*i);               
    }
  
    // RANGE TITLE / BARGRAPH
    g_fixed2 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(g_fixed2, SBV_FIXED2_W, SBV_FIXED2_H);
    nfui_nfobject_show(g_fixed2);
    nfui_nfobject_modify_bg(g_fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, g_fixed2, SBV_FIXED2_X, SBV_FIXED2_Y);
    nfui_regi_post_event_callback(g_fixed2,post_fixed2_event_handler);

    vw_vca_statistic_total_component_open(g_fixed2, opt);
    
    // FIXED3
    // AVERAGE / BARGRAPH / PIECHART

    opt |= (1 << OPT_LIVE);

    g_fixed3 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(g_fixed3, SBV_FIXED3_W, SBV_FIXED3_H);
    nfui_nfobject_show(g_fixed3);
    nfui_nfobject_modify_bg(g_fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, g_fixed3, SBV_FIXED3_X, SBV_FIXED3_Y);
    nfui_regi_post_event_callback(g_fixed3, post_fixed3_event_handler);

    vw_vca_statistic_average_component_open(g_fixed3, opt);
    
    // FIXED4 
    // RATE - PIECHART

    g_fixed4 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(g_fixed4, SBV_FIXED4_W, SBV_FIXED4_H);
    nfui_nfobject_show(g_fixed4);
    nfui_nfobject_modify_bg(g_fixed4, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, g_fixed4, SBV_FIXED4_X, SBV_FIXED4_Y);
    nfui_regi_post_event_callback(g_fixed4, post_fixed4_event_handler);

    vw_vca_statistic_rate_component_open(g_fixed4, opt);

    g_export_obj = nftool_normal_button_create_type2(strButton[0], MENU_BTN_WIDTH);
    nfui_nfobject_show(g_export_obj);
    nfui_nffixed_put((NFFIXED *)parent, g_export_obj, 0, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(g_export_obj, post_export_event_handler);
    
    obj = nftool_normal_button_create_type2(strButton[1], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    
    nfui_regi_post_event_callback(opt_fixed, post_opt_fixed_event_handler);
    nfui_regi_post_event_callback(cal_fixed, post_cal_fixed_event_handler);

    _init_statistic_component_data(nfui_nfobject_get_top(parent));
    _init_statistic_component_action(nfui_nfobject_get_top(parent));

    _init_statistic_total_component_data(nfui_nfobject_get_top(parent));
    _init_statistic_average_component_data(nfui_nfobject_get_top(parent)); 
    _init_statistic_rate_component_data(nfui_nfobject_get_top(parent));    

    _prepare_display_statistic();  
    
    vw_vca_statistic_total_component_sync_data(g_fixed2);
    vw_vca_statistic_average_component_sync_data(g_fixed3);
    vw_vca_statistic_rate_component_sync_data(g_fixed4);
    
    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean vw_VaStatistic_tab_out_handler()
{    
    return FALSE;
}

gboolean vw_VaStatistic_tab_in_handler()
{
    g_search_time = stm_get_time_t();
    g_init_sbv = TRUE;
    return FALSE;
}

gboolean vw_VaStatistic_tab_show()
{
    g_search_time = stm_get_time_t();
    vw_VaStatistic_tab_in_handler();
    return FALSE;
}
