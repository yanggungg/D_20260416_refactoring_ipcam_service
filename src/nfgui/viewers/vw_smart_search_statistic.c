#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"

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
#include "objects/nftimelabel.h"
#include "objects/cw_calendar.h"
#include "objects/nfbargraph.h"        
#include "objects/nfpiechart.h"

#include "vw_search_by_statistic.h"
#include "vw_smart_search_statistic.h"
#include "../../service/nf_va_statistic.h"

#include "vw_search_main.h"
#include "vw_vca_statistic_component.h"
#include "vw_statistic_export.h"


#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"

#include "nfdal.h"

#include "stm.h"
#include "dtf.h"
#include "uxm.h"
#include "var.h"
#include "ix_mem.h"

#include "vaa.h"
#include "vaa_itx.h"

#define SSV_FIXED1_X            (0)
#define SSV_FIXED1_Y            (0)
#define SSV_FIXED1_W            (470)
#define SSV_FIXED1_H            (903)

#define SSV_FIXED2_X            (SSV_FIXED1_X+SSV_FIXED1_W)
#define SSV_FIXED2_Y            (SSV_FIXED1_Y)
#define SSV_FIXED2_W            (1386)
#define SSV_FIXED2_H            (523)

#define SSV_FIXED3_X            (SSV_FIXED2_X)
#define SSV_FIXED3_Y            (SSV_FIXED2_Y+SSV_FIXED2_H)
#define SSV_FIXED3_W            ((SSV_FIXED2_W/2)-50)
#define SSV_FIXED3_H            (380)

#define SSV_FIXED4_X            (SSV_FIXED3_X+SSV_FIXED3_W)
#define SSV_FIXED4_Y            (SSV_FIXED3_Y)
#define SSV_FIXED4_W            (SSV_FIXED3_W+50)
#define SSV_FIXED4_H            (380)

#define SSV_CLOSE_X             (1920-MENU_BTN_WIDTH-50)
#define SSV_CLOSE_Y             (SSV_FIXED4_Y+SSV_FIXED4_H+13)


//////////////////////////////////////

#define SSV_LABEL_X_GAP         (180)
#define SSV_LABEL_Y_GAP         (20)

#define SSV_LABEL_WIDTH         (230)
#define SSV_LABEL_WIDTH_TITLE   (120)
#define SSV_LABEL_HEIGHT        (40)

//////////////////////////////////////


// CHANNEL

#define SSV_CH_LABEL_X          (13)
#define SSV_CH_LABEL_Y          (SSV_LABEL_Y_GAP+40)
#define SSV_CH_LABEL_W          (120)
#define SSV_CH_LABEL_H          (40)

#define SSV_CH_TEXT_X           (190)
#define SSV_CH_TEXT_Y           (SSV_CH_LABEL_Y)
#define SSV_CH_TEXT_W           (SSV_LABEL_WIDTH_TITLE)
#define SSV_CH_TEXT_H           (SSV_CH_LABEL_H)

// FROM

#define SSV_FROM_LABEL_X        (SSV_CH_LABEL_X)
#define SSV_FROM_LABEL_Y        (SSV_CH_LABEL_Y+ SSV_LABEL_HEIGHT+SSV_LABEL_Y_GAP)
#define SSV_FROM_LABEL_W        (SSV_LABEL_WIDTH_TITLE)
#define SSV_FROM_LABEL_H        (SSV_LABEL_HEIGHT)

#define SSV_FROM_TEXT_X         (140)
#define SSV_FROM_TEXT_Y         (SSV_FROM_LABEL_Y)
#define SSV_FROM_TEXT_W         (300)
#define SSV_FROM_TEXT_H         (40)

// To 

#define SSV_TO_LABEL_X          (SSV_FROM_LABEL_X)
#define SSV_TO_LABEL_Y          (SSV_FROM_LABEL_Y + SSV_FROM_TEXT_H+ SSV_LABEL_Y_GAP)
#define SSV_TO_LABEL_W          (SSV_LABEL_WIDTH_TITLE)
#define SSV_TO_LABEL_H          (SSV_CH_LABEL_H)

#define SSV_TO_TEXT_X           (140)
#define SSV_TO_TEXT_Y           (SSV_TO_LABEL_Y)
#define SSV_TO_TEXT_W           (300)
#define SSV_TO_TEXT_H           (40)


// PERIOD

#define SSV_PERIOD_LABEL_X      (SSV_CH_LABEL_X)
#define SSV_PERIOD_LABEL_Y      (SSV_TO_LABEL_Y+SSV_TO_LABEL_H+SSV_LABEL_Y_GAP+15)
#define SSV_PERIOD_LABEL_W      (SSV_LABEL_WIDTH_TITLE)
#define SSV_PERIOD_LABEL_H      (SSV_CH_LABEL_H)

#define SSV_PERIOD_COMBO_X      (SSV_LABEL_X_GAP)
#define SSV_PERIOD_COMBO_Y      (SSV_PERIOD_LABEL_Y)
#define SSV_PERIOD_COMBO_W      (SSV_LABEL_WIDTH)
#define SSV_PERIOD_COMBO_H      (SSV_PERIOD_LABEL_H)

// VA_RULE

#define SSV_RULE_X              (SSV_CH_LABEL_X)
#define SSV_RULE_Y              (SSV_PERIOD_LABEL_Y+SSV_PERIOD_LABEL_H+SSV_LABEL_Y_GAP+190)  
#define SSV_RULE_W              (SSV_LABEL_WIDTH_TITLE)
#define SSV_RULE_H              (SSV_CH_LABEL_H)

#define SSV_RULE_OPTFIXED_X     (11)
#define SSV_RULE_OPTFIXED_Y     (SSV_RULE_Y+SSV_CH_LABEL_H+10)
#define SSV_RULE_OPTFIXED_W     (417)
#define SSV_RULE_OPTFIXED_H     (290)

#define SBV_RULE_LABEL_WIDTH    (185)

// EXPORT DATA

#define SSV_EXPORT_BUTTON_X     (SSV_CH_LABEL_X)
#define SSV_EXPORT_BUTTON_Y     (SSV_CLOSE_Y) //(SSV_RULE_OPTFIXED_Y+SSV_RULE_OPTFIXED_H+200)
#define SSV_EXPORT_BUTTON_W     (200)
#define SSV_EXPORT_BUTTON_H     (40)

// TIME SET
#define	DAY_TO_SEC			    ((time_t)86400)
#define	HOUR_TO_SEC	        	((time_t)3600)
#define	MIN_TO_SEC	        	((time_t)60)

#define OPT_RULEID_CNT          (16)
#define OPT_RULETYPE_CNT        (7)

#define OPT_RULETYPE_ALLMSK     (0x373)

enum {
	STATISTIC_MODE_HOUR = 0,
	STATISTIC_MODE_DAY,
	STATISTIC_MODE_WEEK,
	STATISTIC_MODE_MONTH,
	STATISTIC_MODE_YEAR,
	STATISTIC_MODE_MAX,
};


enum {
    OPT_RULE_ZONE1 = 0,
    OPT_RULE_ZONE2,
    OPT_RULE_ZONE3,
    OPT_RULE_ZONE4,
    OPT_RULE_ZONE5,
    OPT_RULE_ZONE6,
    OPT_RULE_ZONE7,
    OPT_RULE_ZONE8,
    OPT_RULE_ZONE9,
    OPT_RULE_ZONE10,
    OPT_RULE_ZONE11,
    OPT_RULE_ZONE12,
    OPT_RULE_ZONE13,
    OPT_RULE_ZONE14,
    OPT_RULE_ZONE15,
    OPT_RULE_ZONE16,
    OPT_RULE_MAX
 };
    
////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *main_wnd;
static NFOBJECT *main_fixed;
static NFOBJECT *g_content_fixed;

static NFOBJECT *opt_fixed;
static NFOBJECT *opt_rule[OPT_RULE_MAX];

static NFOBJECT *g_period_obj;
static NFOBJECT *g_export_obj;

static NFOBJECT *g_allrule_check;
static NFOBJECT *g_ruletype_check[OPT_RULETYPE_CNT];
static NFOBJECT *g_rulelist_obj;

static NFOBJECT *fixed1;
static NFOBJECT *fixed2;
static NFOBJECT *fixed3;
static NFOBJECT *fixed4;

static NFOBJECT *nfbargraph[2];  

static gboolean g_init_sbv = TRUE;
static gint     g_cur_ch = 0;
static gint     g_zone_cnt = -1;
static guint    g_exist_zone =0;

static time_t   g_search_time;
static time_t   g_start_time;
static time_t   g_end_time;

static guint g_rule_onoff[OPT_RULEID_CNT];
static guint g_draw_tid = 0;

static const gchar *strWeekDay[]={"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const gchar *strMonth[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
static gchar zone_str[16][32];


enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,
	NUM_DATE_FORMATS,
};


/////////////////////////////////////////////////////////////

static guint _get_days_in_month(gshort year, gshort month)
{
	int i;
	struct tm tm_ptr;
    time_t the_time;
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

static guint _get_days_in_year(gshort year)
{
    int i, day_count = 0;

    for (i = 0; i < 12; i++)
    {
    	day_count += _get_days_in_month(year, i);
    }
    
	return day_count;
}

static guint _get_start_day_in_week(time_t time)
{
	struct tm ttm;

	localtime_r(&time, &ttm);    
    return ttm.tm_wday;
}

static gint _get_FromTo_hour_text(STATISTIC_COMPONENT_DATA_T *component_data, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    gint hour, min, sec;
    gint e_hour,e_min,e_sec;
    
    gint i=0,j=0;

    dtf_get_local_hourmin(g_end_time, &e_hour, &e_min, &e_sec);
    dtf_get_local_hourmin(g_start_time, &hour, &min, &sec);
   
    gint tmp; 
    tmp = min%5;
    min -= tmp;

    for(i=0; i<cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if((min+(j*5))>55)
        {
            min = 0;
            hour = hour+1;
            if(hour==24) hour = 0;
            j=0;
        }
        
        g_sprintf(strBuf,"%02d:%02d", hour,(j*5)+min);
        j = j+1;

        strcpy(str[i], strBuf);
    }

    return 0;
}

static gint _get_FromTo_day_text(STATISTIC_COMPONENT_DATA_T *component_data, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];

    gint hour, min, sec;
    gint e_hour, e_min, e_sec;
    gint i=0, j=1;

    dtf_get_local_hourmin(g_end_time, &e_hour, &e_min, &e_sec);
    dtf_get_local_hourmin(g_start_time, &hour, &min, &sec);

    for(i=0; i<cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(hour>23)
        {
            hour = 0;
        }

        if(cnt<3)
            g_sprintf(strBuf,"%02d:00", hour);
        else
        {
            if(i%2 == 0)
                g_sprintf(strBuf,"%02d:00", hour);
            else
                g_sprintf(strBuf,"%s", " ");
        }
        
        strcpy(str[i], strBuf);

        hour = hour +1 ;            
    }

    return 0;
}

static gint _get_FromTo_week_text(STATISTIC_COMPONENT_DATA_T *component_data, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];

    gint year, mon, day;
    gint hour, min, sec;
    gint i=0, j=1;

    gint day_cnt; 

    dtf_get_local_day(g_start_time, &year, &mon, &day);
    dtf_get_local_hourmin(g_start_time, &hour, &min, &sec);

    day_cnt = _get_days_in_month(year, mon);

    for(i=0; i<cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(day>day_cnt)
        {
            day=1;
        }

        g_sprintf(strBuf,"%2d",day);

        strcpy(str[i], strBuf);

        day = day+1;
    }

    return 0;
}

static gint _get_FromTo_month_text(STATISTIC_COMPONENT_DATA_T *component_data, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];

    gint year, mon, day;
    gint e_year,e_mon, e_day;
    
    gint i=0, j=1;
    gint day_cnt;

    dtf_get_local_day(g_start_time, &year, &mon, &day);
    dtf_get_local_day(g_end_time, &e_year, &e_mon, &e_day);

    day_cnt = _get_days_in_month(year, mon);

    for(i=0; i<cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(day>day_cnt)
        {
            day=1;
        }
        
        g_sprintf(strBuf,"%2d",day);

        strcpy(str[i], strBuf);

        day = day +1;

    }
    
    return 0;
}

static gint _get_FromTo_year_text(STATISTIC_COMPONENT_DATA_T *component_data, gint cnt, gchar str[][TEXT_SIZE])
{
    gchar strBuf[16];
    
    gint year, mon, day;
    gint e_year, e_mon, e_day;
    gint i=0, j=1;

    dtf_get_local_day(g_start_time, &year, &mon, &day);
    dtf_get_local_day(g_end_time, &e_year, &e_mon, &e_day);

    for(i=0; i<cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if(mon>12)
        {
            mon = 1;
        }
        g_sprintf(strBuf, "%s", strMonth[mon-1]);   
             
        strcpy(str[i],strBuf);
        mon = mon+1;
    }
    
    return 0;
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

static gint _get_graph_day_text(time_t time, gint cnt, gchar str[][TEXT_SIZE])
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
        else{
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
        else{
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

static gint _get_period_type_combo(gint type)
{     
    switch(type)
    {          
        case PERIOD_TYPE_HOUR:
            nfui_combobox_remove_data(g_period_obj, "DAY");
            nfui_combobox_remove_data(g_period_obj, "WEEK");
            nfui_combobox_remove_data(g_period_obj, "MONTH");
            nfui_combobox_remove_data(g_period_obj, "YEAR");
            break;
        case PERIOD_TYPE_DAY:
            nfui_combobox_remove_data(g_period_obj, "WEEK");
            nfui_combobox_remove_data(g_period_obj, "MONTH");
            nfui_combobox_remove_data(g_period_obj, "YEAR");
            break;            
        case PERIOD_TYPE_WEEK :
            nfui_combobox_remove_data(g_period_obj, "MONTH");
            nfui_combobox_remove_data(g_period_obj, "YEAR");
            break;
            
        case PERIOD_TYPE_MONTH:
            nfui_combobox_remove_data(g_period_obj, "YEAR");
            break;
        case PERIOD_TYPE_YEAR :
            break;
    }               
    
    nfui_combobox_set_index_no_expose(g_period_obj, type);
    nfui_signal_emit(g_period_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _get_period_type()
{
    time_t start_t;
    time_t end_t;

    gint mk_hour = 3600;
    gint mk_day = mk_hour*24;
    gint mk_month = mk_day*31;
    gint mk_year = mk_month*12;

    gint type;

    start_t = g_start_time;
    end_t = g_end_time;

    if((end_t-start_t)< mk_hour)
    {
        type = PERIOD_TYPE_HOUR;
    }
    else if((end_t-start_t)<mk_day)
    {
        type = PERIOD_TYPE_DAY;
    }
    else if((end_t-start_t)<mk_month)
    {
        type = PERIOD_TYPE_MONTH;
    }
    else 
        type = PERIOD_TYPE_YEAR;

    _get_period_type_combo(type);
     
    return type;
}
static gint _get_average_type(gint type)
{  
    switch(type)
    {
        case PERIOD_TYPE_HOUR:
        case PERIOD_TYPE_DAY:
            type = AVERAGE_TYPE_NONE;
            break;
        case PERIOD_TYPE_WEEK:
            type = AVERAGE_TYPE_HOUR;
            break;
        case PERIOD_TYPE_MONTH:
            type = AVERAGE_TYPE_WEEK;
            break;
        case PERIOD_TYPE_YEAR:
            type = AVERAGE_TYPE_DAY;
            break;
    }
    
    return type;
}

static gint _get_text_statistic_total_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);    

    _get_graph_hour_text(g_search_time, component_data->total.hour_cnt, component_data->total.hour_text);   
    _get_graph_day_text(g_search_time, component_data->total.day_cnt, component_data->total.day_text);
    _get_graph_week_text(g_search_time, component_data->total.week_cnt, component_data->total.week_text);
    _get_graph_month_text(g_search_time, component_data->total.month_cnt, component_data->total.month_text);    
    _get_graph_year_text(g_search_time, component_data->total.year_cnt, component_data->total.year_text);            

    return 0;
}

static gint _get_text_FromTo_total_component_data(NFOBJECT *top)
{
    time_t start_time;
    
    STATISTIC_COMPONENT_DATA_T *component_data;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);     
    
    switch(component_data->total.period_type)
    {
        case PERIOD_TYPE_HOUR:  _get_FromTo_hour_text(component_data, component_data->total.hour_cnt, component_data->total.hour_text);      break;
        case PERIOD_TYPE_DAY:   _get_FromTo_day_text(component_data, component_data->total.day_cnt, component_data->total.day_text);         break;
        case PERIOD_TYPE_WEEK:  _get_FromTo_week_text(component_data, component_data->total.week_cnt, component_data->total.week_text);      break;
        case PERIOD_TYPE_MONTH: _get_FromTo_month_text(component_data, component_data->total.month_cnt, component_data->total.month_text);   break;
        case PERIOD_TYPE_YEAR:  _get_FromTo_year_text(component_data, component_data->total.year_cnt, component_data->total.year_text);      break;
    }
    return 0;
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

    days_count = _get_days_in_month(year, month);
    
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

    days_count = _get_days_in_year(year);
    
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

    g_message("\n smart jaeyoung set >> %s, %d >> jaeyoung start time :%s, end:%s\n", __FUNCTION__, __LINE__, start_strBuf, end_strBuf);


    return 0;
}


static gint _get_statistic_info(STATISTIC_COMPONENT_DATA_T *component_data, va_statistic_t *statistic_data)
{
    GTimeVal start_time, end_time;
    gint i;

    if(component_data->total.period_type == component_data->total.init_period_type)
    {
        start_time.tv_sec = g_start_time;
        end_time.tv_sec = g_end_time;
    }
    else
    {
        _get_statistic_section(component_data->total.period_type, &start_time.tv_sec, &end_time.tv_sec);
    }

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

static gint _get_statistic_data(event_buffer_t* event_list,int event_cnt, va_statistic_t *statistic_data)
{
    gint ret;
    
    ret = get_va_statistic_smart(event_list, event_cnt, statistic_data);

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


static time_t _get_prev_search_time(gint period_type, time_t cur_time, gint day_cnt,gint year_cnt)
{
    time_t prev_time;

    if (period_type == PERIOD_TYPE_HOUR)        prev_time = cur_time - 3600;
    else if (period_type == PERIOD_TYPE_DAY)    prev_time = cur_time - 3600*24;
    else if (period_type == PERIOD_TYPE_WEEK)   prev_time = cur_time - 3600*24*7;
    else if (period_type == PERIOD_TYPE_MONTH)  prev_time = cur_time - 3600*24*day_cnt;
    else if (period_type == PERIOD_TYPE_YEAR)   prev_time = cur_time - 3600*24*year_cnt;
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

static time_t _get_FromTo_index_search_time(gint period_type, gint index, time_t cur_time)
{
    time_t index_time;

	gint year, month, day;
	gint hour, min, sec;
	gint day_cnt;
	gint year_cnt;

	dtf_get_local_day(cur_time, &year, &month, &day);
	dtf_get_local_hourmin(cur_time, &hour, &min, &sec);	

	day_cnt = _get_days_in_month( year, month);       


    if (period_type == PERIOD_TYPE_HOUR)        
    {
        index_time = ifn_get_gmt_from_local(year, month, day, hour, (index+1)*5, sec);
    }
    else if (period_type == PERIOD_TYPE_DAY)    
    {
        if((hour+index)>24)
        { 
            hour = (hour+index)-24;
            day = day+1;
            if(day > day_cnt) day = 1,month++; 
            if(month > 12) month =1, year++;
        }
        else    hour = hour+index;
        
        index_time = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
    }
    else if (period_type == PERIOD_TYPE_WEEK)   
    {
        day -= _get_start_day_in_week(cur_time);
        
        index_time = ifn_get_gmt_from_local(year, month, day+index, hour, min, sec);
    }
    else if (period_type == PERIOD_TYPE_MONTH)  
    {
        if((day+index)>day_cnt)
        {
            day = (day+index - day_cnt);
            month = month+1;
            if(month > 12) month =1,year++;
        }
        else    day = day+index;
        
        index_time = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
    }
    else if (period_type == PERIOD_TYPE_YEAR)   
    {
        if((month+index)>12)
        {
            month = (month+index-12);
            year = year+1;   
        }
        else    month = month+index;        
    
        index_time = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
    }
    else 
        g_message("%s, %d, undefined period type", __FUNCTION__, __LINE__);
    
    return index_time;

}

static time_t _get_index_search_time(gint period_type, gint index, time_t cur_time)
{
    time_t index_time;

	gint year, month, day;
	gint hour, min, sec;

	dtf_get_local_day(cur_time, &year, &month, &day);
	dtf_get_local_hourmin(cur_time, &hour, &min, &sec);	

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
        day -= _get_start_day_in_week(cur_time);
        
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

static gint _get_rule_name_string(VAAID vaaid, gint zone_id, gchar *str)
{   
    ITX_VAZONE_CONF conf;
    ITX_VAZONE_SHAPE shape;

    if((zone_id < 0 ) || (zone_id > 16)) return -1;

    vaa_itx_get_zone_conf(vaaid, zone_id, &conf);
    vaa_itx_get_zone_shape(vaaid, zone_id, &shape);
    strcpy(str, shape.name);

    if(conf.use_zone && conf.active)
        return 0;
    else 
        return -1;

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

    export_data->ch = g_cur_ch;
    export_data->start_time = component_data->total.start_time;
    export_data->end_time = component_data->total.end_time;
    export_data->period = component_data->total.period_type;
    export_data->total_events = component_data->total.event_count;
    export_data->exist_zone = g_exist_zone;

    for(i=0; i<16; i++)
    {
        export_data->rule_list[i] = g_rule_onoff[i];
    }
    
    return 0;
}

static gint _prepare_display_statistic(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    
    va_statistic_t statistic_data;
    event_buffer_t* event_list;

    int event_cnt;

    event_list = _get_searchbysmart_result();
    event_cnt = _get_search_by_smart_result();

    memset(&statistic_data, 0x00, sizeof(va_statistic_t));

    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

    _get_statistic_info(component_data, &statistic_data);
    _get_statistic_data(event_list, event_cnt, &statistic_data);
    _set_statistic_data(component_data, &statistic_data);

    return 0;
}

#if 0
static gint get_cur_time(gint type)
{
    time_t tmp_time;

    gint year, month, day;
    gint hour, min ,sec;


    ifn_get_local_day(g_search_time, &year, &month,&day);
    ifn_get_local_hourmin(g_search_time, &hour,0,0);

    switch(type)
    {
        case PERIOD_TYPE_HOUR:
            tmp_time = ifn_get_gmt_from_local(year,month,day,hour,0,0);
            g_search_time = tmp_time+1;
        break;
        case PERIOD_TYPE_DAY:
            tmp_time = ifn_get_gmt_from_local(year,month,day,0,0,0);
            g_search_time = tmp_time+1;
        break;
        case PERIOD_TYPE_WEEK:
          	day -= _get_start_day_in_week(g_search_time);
            tmp_time = ifn_get_gmt_from_local(year,month,day,0,0,0);            
            g_search_time = tmp_time+1;
        break;
        case PERIOD_TYPE_MONTH:
            tmp_time = ifn_get_gmt_from_local(year,month,1,0,0,0);
            g_search_time = tmp_time+1;
        break;
        case PERIOD_TYPE_YEAR:
            tmp_time = ifn_get_gmt_from_local(year,1,1,0,0,0);
            g_search_time = tmp_time+1;   
        break;
    }

    return 0;
}
#endif

static gboolean check_from_to_range_prev(gint type)
{
    time_t start_time;
    time_t prev_time;

    gint p_year, p_month, p_day, p_hour;
    gint s_year, s_month, s_day, s_hour;
    
    ifn_get_local_day(g_search_time, &p_year, &p_month, &p_day);
    ifn_get_local_hourmin(g_search_time, &p_hour,0,0);

    dtf_get_local_day(g_start_time, &s_year, &s_month,&s_day);
    dtf_get_local_hourmin(g_start_time, &s_hour,0,0);

    switch(type)
    {
        case PERIOD_TYPE_HOUR:
            start_time = ifn_get_gmt_from_local(s_year,s_month,s_day,s_hour,0,0);
            prev_time = ifn_get_gmt_from_local(p_year,p_month,p_day,p_hour,0,0);
            prev_time -= 3600;
            
            if(start_time > prev_time) return TRUE;
            break;

        case PERIOD_TYPE_DAY:
            start_time = ifn_get_gmt_from_local(s_year,s_month,s_day,0,0,0);
            prev_time = ifn_get_gmt_from_local(p_year,p_month,p_day,0,0,0);
            prev_time -= 3600*24;            

            if(start_time > prev_time) return TRUE;
            break;

        case PERIOD_TYPE_WEEK:
           	p_day -= _get_start_day_in_week(g_search_time);
           	s_day -= _get_start_day_in_week(g_start_time);
           	
            start_time = ifn_get_gmt_from_local(s_year,s_month,s_day,0,0,0);
            prev_time = ifn_get_gmt_from_local(p_year,p_month,p_day,0,0,0);
            prev_time -= 3600*24*7;            

            if(start_time > prev_time) return TRUE;
            break;
            
        case PERIOD_TYPE_MONTH:
            start_time = ifn_get_gmt_from_local(s_year,s_month,1,0,0,0);
            prev_time = ifn_get_gmt_from_local(p_year,p_month,1,0,0,0);

            if(p_month == 1) p_year--, p_month=13;
           
            prev_time -= (3600*24*_get_days_in_month(p_year, p_month-1)); 
            
            if(start_time > prev_time) return TRUE;
            break;
    }
    
    return FALSE;
}

static gboolean check_from_to_range_next(gint type)
{
    time_t end_time;
    time_t next_time;

    gint n_year, n_month, n_day, n_hour;
    gint e_year, e_month, e_day, e_hour;
    
    ifn_get_local_day(g_search_time, &n_year, &n_month, &n_day);
    ifn_get_local_hourmin(g_search_time, &n_hour,0,0);

    dtf_get_local_day(g_end_time, &e_year, &e_month, &e_day);
    dtf_get_local_hourmin(g_end_time, &e_hour,0,0);

    switch(type)
    {
        case PERIOD_TYPE_HOUR:
            end_time = ifn_get_gmt_from_local(e_year,e_month,e_day,e_hour,0,0);
            next_time = ifn_get_gmt_from_local(n_year,n_month,n_day,n_hour,0,0);
            next_time += 3600;

            if(end_time < next_time) return TRUE;
            break;

        case PERIOD_TYPE_DAY:
            end_time = ifn_get_gmt_from_local(e_year,e_month,e_day,0,0,0);
            next_time = ifn_get_gmt_from_local(n_year,n_month,n_day,0,0,0);
            next_time += 3600*24;
            
            if(end_time < next_time) return TRUE;
            break;

        case PERIOD_TYPE_WEEK:
          	n_day -= _get_start_day_in_week(g_search_time);
          	e_day -= _get_start_day_in_week(g_end_time);
          	
            end_time = ifn_get_gmt_from_local(e_year,e_month,e_day,0,0,0);
            next_time = ifn_get_gmt_from_local(n_year,n_month,n_day,0,0,0);
            next_time += 3600*24*7;
            
            if(end_time < next_time) return TRUE;
            break;    

        case PERIOD_TYPE_MONTH:
            end_time = ifn_get_gmt_from_local(e_year,e_month,1,0,0,0);
            next_time = ifn_get_gmt_from_local(n_year,n_month,1,0,0,0);
            
            next_time += (3600*24*_get_days_in_month(n_year, n_month));
                   
            if(end_time < next_time) return TRUE;

            break;
    }
    
    return FALSE;
}

static gint _total_graph_press(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    return 0;    
}

static gint _total_graph_2press(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *component_data;    
    NFOBJECT *top;
    NFOBJECT *obj;
    gint idx; 

    gint year, month, day;
    
    time_t index_time;

    obj = (NFOBJECT*)user_data;

  	dtf_get_local_day(g_search_time, &year, &month, &day);

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    if(component_data->total.period_type == component_data->total.init_period_type)
        index_time = _get_FromTo_index_search_time(component_data->total.period_type, component_data->total.bar_idx, g_start_time);        
    else        
        index_time = _get_index_search_time(component_data->total.period_type, component_data->total.bar_idx, g_search_time);

    component_data->total.calendar_time = index_time;
    if(component_data->total.init_period_type == PERIOD_TYPE_YEAR)
        component_data->total.month_cnt = _get_days_in_month(year, month);
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
    _prepare_display_statistic(top);
    
    vw_vca_statistic_total_component_sync_data(fixed2);
    vw_vca_statistic_average_component_sync_data(fixed3);
    vw_vca_statistic_rate_component_sync_data(fixed4);

    stm_set_time_t(index_time);   

    nfui_combobox_set_index(g_period_obj, component_data->total.period_type);        
    nfui_signal_emit(g_period_obj, GDK_EXPOSE, TRUE);   
    
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
    gint year, month, day; 

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);

    if(component_data->total.period_type == component_data->total.init_period_type)  return 0;

    ifn_get_local_day(g_search_time, &year, &month, 0);

    day_cnt = _get_days_in_month(year, month);
    year_cnt = _get_days_in_year(year);

    if(check_from_to_range_prev(component_data->total.period_type)) return 0;
    
    prev_time = _get_prev_search_time(component_data->total.period_type, g_search_time, day_cnt, year_cnt);
    
    component_data->total.calendar_time = prev_time;
    if(component_data->total.init_period_type == PERIOD_TYPE_YEAR)  component_data->total.month_cnt = day_cnt;


    if(component_data->total.init_period_type != PERIOD_TYPE_MONTH)
        _get_graph_month_text(prev_time, component_data->total.month_cnt, component_data->total.month_text);   
    if(component_data->total.init_period_type != PERIOD_TYPE_HOUR)
        _get_graph_hour_text(prev_time, component_data->total.hour_cnt, component_data->total.hour_text); 
    
    g_search_time = prev_time;     
    _prepare_display_statistic(top);
    
    vw_vca_statistic_total_component_sync_data(fixed2);
    vw_vca_statistic_average_component_sync_data(fixed3);
    vw_vca_statistic_rate_component_sync_data(fixed4);

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

    if(component_data->total.period_type == component_data->total.init_period_type)    return 0;

    ifn_get_local_day(g_search_time, &year, &month, 0);
    
    day_cnt = _get_days_in_month(year, month);
    year_cnt = _get_days_in_year(year);

    if(check_from_to_range_next(component_data->total.period_type)) return 0;
    
    next_time = _get_next_search_time(component_data->total.period_type, g_search_time, day_cnt, year_cnt);

    component_data->total.calendar_time = next_time;
    if(component_data->total.init_period_type == PERIOD_TYPE_YEAR) component_data->total.month_cnt = day_cnt;

    if(component_data->total.init_period_type != PERIOD_TYPE_MONTH)
            _get_graph_month_text(next_time, component_data->total.month_cnt, component_data->total.month_text);   
    if(component_data->total.init_period_type != PERIOD_TYPE_HOUR)
            _get_graph_hour_text(next_time, component_data->total.hour_cnt, component_data->total.hour_text);
    
    g_search_time = next_time;
    _prepare_display_statistic(top);
    
    vw_vca_statistic_total_component_sync_data(fixed2);
    vw_vca_statistic_average_component_sync_data(fixed3);
    vw_vca_statistic_rate_component_sync_data(fixed4);

    stm_set_time_t(next_time);
 
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

    _prepare_display_statistic(top);
    vw_vca_statistic_average_component_sync_data(fixed3);
    return 0;
}

#if 0
static gint _period_type_cb(gpointer user_data)
{
    STATISTIC_COMPONENT_DATA_T *statistic_data;    
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);
    statistic_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, STATISTIC_COMPONENT_DATA);
    
        
    return 0;
}
#endif

static gint _get_init_bargraph_count(STATISTIC_COMPONENT_DATA_T *component_data)
{
    gint i=1;

    time_t tmp_time;
    
    gint hour, min, sec;
    gint year, month, day;
    
    ifn_get_local_day(g_start_time, &year, &month, &day);           
    ifn_get_local_hourmin(g_start_time, &hour, &min, &sec);
  
    gint mk_hour  = 60*5;
    gint mk_day   = 3600;
    gint mk_month = 3600*24;
    gint mk_year  = 3600*24*30;

    switch(component_data->total.init_period_type)
    {
        case PERIOD_HOUR :
            min = min-(min%5);
            tmp_time = ifn_get_gmt_from_local(year, month, day, hour, min, 0);
            while((tmp_time+(i*mk_hour))<g_end_time) i++;
            component_data->total.hour_cnt = i;
            component_data->total.bar_init_gap = 643-((i-1)*55);
            if(component_data->total.bar_init_gap < 80) component_data->total.bar_init_gap = 80;
            break;
        case PERIOD_DAY :
            tmp_time = ifn_get_gmt_from_local(year, month, day, hour, 0, 0);
            while((tmp_time+(i*mk_day))<g_end_time) i++;
            component_data->total.day_cnt = i;
            component_data->total.bar_init_gap = 643-((i-1)*35);
            if(component_data->total.bar_init_gap < 40) component_data->total.bar_init_gap = 40;
            break;
        case PERIOD_MONTH :
            tmp_time = ifn_get_gmt_from_local(year, month, day, 0, 0, 0);
            while((tmp_time+(i*mk_month))<g_end_time) i++;
            component_data->total.month_cnt = i;
            component_data->total.bar_init_gap = 643-((i-1)*35);
            if(component_data->total.bar_init_gap < 40) component_data->total.bar_init_gap = 40;
            break;
        case PERIOD_YEAR :
            tmp_time = ifn_get_gmt_from_local(year, month, 1, 0, 0, 0);
            while((tmp_time+(i*mk_year))<g_end_time) i++;            
            if(i<14) component_data->total.year_cnt = i;            
            component_data->total.bar_init_gap = 643-((i-1)*55);
            if(component_data->total.bar_init_gap < 80) component_data->total.bar_init_gap = 80;
            break;
    }
    
    return 0;
}

static gint _init_statistic_total_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;
    gint i;

    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);
    
    g_search_time = g_start_time;

    component_data->total.start_time = g_start_time;
    component_data->total.end_time = g_end_time;
    
    component_data->total.calendar_time = g_search_time;
    component_data->total.event_count = 0;

    component_data->total.period_type = _get_period_type();
    component_data->total.init_period_type = component_data->total.period_type; 
   
    component_data->total.bar_idx = 0;

    component_data->total.hour_cnt = 12;
    component_data->total.day_cnt = 24;
    component_data->total.week_cnt = 7;    
    component_data->total.month_cnt = 31;
    component_data->total.year_cnt = 12;

    _get_init_bargraph_count(component_data); 

    _get_graph_hour_text(g_search_time, component_data->total.hour_cnt, component_data->total.hour_text);   
    _get_graph_day_text(g_search_time, component_data->total.day_cnt, component_data->total.day_text);
    _get_graph_week_text(g_search_time, component_data->total.week_cnt, component_data->total.week_text);
    _get_graph_month_text(g_search_time, component_data->total.month_cnt, component_data->total.month_text);    
    _get_graph_year_text(g_search_time, component_data->total.year_cnt, component_data->total.year_text); 

    _get_text_FromTo_total_component_data(top);

    return 0;        
}

static gint _init_statistic_average_component_data(NFOBJECT *top)
{
    STATISTIC_COMPONENT_DATA_T *component_data;

    gchar strBuf[64];
    gint i;
    
    component_data = (STATISTIC_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, STATISTIC_COMPONENT_DATA);

    component_data->average.average_type = _get_average_type(component_data->total.period_type);

    component_data->average.hour_cnt = 24;
    component_data->average.day_cnt = 31;
    component_data->average.week_cnt = 7;

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

    component_action->total_next_cb = _total_next_cb;
    component_action->total_previous_cb = _total_previous_cb;
    component_action->total_graph_press = _total_graph_press;
    component_action->total_graph_2press = _total_graph_2press;
    component_action->average_type_cb = _average_type_cb; 
    //component_action->period_type_cb = _period_type_cb;

    nfui_nfobject_set_alloc_data(top, STATISTIC_COMPONENT_ACTION, component_action );

    return 0;
}

static gboolean _period_time_check(STATISTIC_COMPONENT_DATA_T *component_data,NFOBJECT *top)
{
    time_t time_check;
    
    gint year,month,day,hour,min,sec;
    gint s_year,s_month,s_day;

    ifn_get_local_day(g_start_time, &s_year, &s_month, &s_day);
    ifn_get_local_day(g_search_time, &year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, &min, &sec);

    if(g_search_time < g_start_time)
    {
        day = s_day;     //  week -> hour,day  changed start day time.
        time_check = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
        g_search_time = time_check;               
        component_data->total.calendar_time = g_search_time;
    }
    _prepare_display_statistic(top);
        
    return 0;
}

static gboolean post_period_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        STATISTIC_COMPONENT_DATA_T *component_data;
        STATISTIC_COMPONENT_ACTION_T *component_action;
        NFOBJECT *top;
        gint idx;
                   
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

        _period_time_check(component_data, top);

        vw_vca_statistic_total_component_sync_data(fixed2);
        vw_vca_statistic_average_component_sync_data(fixed3);
        vw_vca_statistic_rate_component_sync_data(fixed4);
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

static gboolean post_ruleid_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_LISTBOX_CHANGED)
    {
        gboolean state;
        gint focus_idx, list_idx;
        gint i;
        
        focus_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)obj);

        if(focus_idx > g_zone_cnt) 
        {
            nfui_nfobject_disable(g_allrule_check);
            nfui_signal_emit(g_allrule_check, GDK_EXPOSE, TRUE);
       
            for (i = 0; i < OPT_RULETYPE_CNT; i++)
            {
                nfui_nfobject_disable(g_ruletype_check[i]);
                nfui_signal_emit(g_ruletype_check[i], GDK_EXPOSE, TRUE);
            }           
            
            return FALSE;
        }
        
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

        _prepare_display_statistic(nfui_nfobject_get_top(main_fixed));

        vw_vca_statistic_total_component_sync_data(fixed2);
        vw_vca_statistic_average_component_sync_data(fixed3);
        vw_vca_statistic_rate_component_sync_data(fixed4);
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

        _prepare_display_statistic(nfui_nfobject_get_top(main_fixed));

        vw_vca_statistic_total_component_sync_data(fixed2);
        vw_vca_statistic_average_component_sync_data(fixed3);
        vw_vca_statistic_rate_component_sync_data(fixed4);

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
		
		NFOBJECT *topwin;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);		

        g_zone_cnt = -1;
    	g_curwnd =0;
		gtk_main_quit();
	}
	
	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}	
	else if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
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
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
            gdk_draw_rectangle(drawable, gc, TRUE, 20, gap_y+SSV_FIXED4_H, 1856, 2);

            g_object_unref(gc);
        }
        break;

        default :
        break;
    }

    return FALSE;
}


static nftl_df_type prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

void vw_init_smart_search_statistic_page(NFOBJECT *parent, GTimeVal from_time, GTimeVal to_time, gint ch)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *top;

    NFOBJECT *opt_fixed;
    DateTimeData dtdata;

    //VCAZoneData g_zone_data;
    VAAID vaaid;
    event_buffer_t* event_list;

    gint i;
    guint size_w, size_h;
    gint pos_x, pos_y;

    GTimeVal time_val;
  	guint tformat;
  	guint opt = 0;

    gchar *strRule;
    gchar strBuf[64];

    const gchar *strPeriod[]={"HOUR","DAY","WEEK","MONTH","YEAR"};
    const gchar *strEvent[]={"FORWARD","REVERSE","VCA-ENTER","VCA-EXIT","STOPPED","REMOVED","LOITERING"};

    gint year, month, day, hour, min, sec;

    memset(&dtdata, 0x00, sizeof(DateTimeData));
	
	DAL_get_dateTime_data(&dtdata);
	DAL_get_dateTime_format(NULL, &tformat);

    g_cur_ch = ch;
    g_start_time = from_time.tv_sec;
    g_end_time = to_time.tv_sec;

                  
    main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, 0, 0, 1920, 1080, "SMART SEARCH STATISTIC", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, returnkey_proc);

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	
	g_curwnd = main_wnd;
	
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, 1896, 973);
    nfui_nffixed_put((NFFIXED*)main_fixed, content_fixed, 10, 47);
    g_content_fixed = content_fixed;
    
    stm_get_time_range_t(0, &g_search_time);
    
    ifn_get_local_day(g_search_time, &year, &month, &day);
    ifn_get_local_hourmin(g_search_time, &hour, &min, &sec);
    
    fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, SSV_FIXED1_W, SSV_FIXED1_H);
    nfui_nfobject_show(fixed1);
    nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, SSV_FIXED1_X, SSV_FIXED1_Y);
    nfui_regi_post_event_callback(fixed1,post_fixed1_event_handler);

    // CHANNEL

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_CH_LABEL_W, SSV_CH_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_CH_LABEL_X, SSV_CH_LABEL_Y);

    char buf[32];

    memset(buf,0x00,sizeof(buf));
    
    g_sprintf(buf,"CH %d",ch+1);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_CH_TEXT_W, SSV_CH_TEXT_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_CH_TEXT_X, SSV_CH_TEXT_Y);

    // FROM
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FROM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_FROM_LABEL_W, SSV_FROM_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_FROM_LABEL_X, SSV_FROM_LABEL_Y);


    obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(120));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(0));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, 300, 40);			
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &from_time);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_FROM_TEXT_X, SSV_FROM_TEXT_Y);

    // TO

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_TO_LABEL_W, SSV_TO_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_TO_LABEL_X, SSV_TO_LABEL_Y);

    obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(120));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(0));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, 300, 40);			
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &to_time);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_TO_TEXT_X, SSV_TO_TEXT_Y);

    // PERIOD

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PERIOD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_PERIOD_LABEL_W, SSV_PERIOD_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_PERIOD_LABEL_X, SSV_PERIOD_LABEL_Y);

    obj =nfui_combobox_new(strPeriod, 5, 0);   
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj),NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, SSV_PERIOD_COMBO_W, SSV_PERIOD_COMBO_H);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_PERIOD_COMBO_X, SSV_PERIOD_COMBO_Y);
    nfui_regi_post_event_callback(obj, post_period_event_handler);
    g_period_obj = obj;

    //////////
    // RULE                    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RULE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SSV_RULE_W, SSV_RULE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SSV_RULE_X, SSV_RULE_Y);

    opt_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(opt_fixed, SSV_RULE_OPTFIXED_W, SSV_RULE_OPTFIXED_H);
    nfui_nfobject_show(opt_fixed);
    nfui_nfobject_modify_bg(opt_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nffixed_put((NFFIXED*)fixed1, opt_fixed, SSV_RULE_OPTFIXED_X, SSV_RULE_OPTFIXED_Y);

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

    vaaid = vaa_get_pb_vaaid(ch);
    
    for (i = 0; i < OPT_RULEID_CNT; i++)
    {
        strRule = imalloc(sizeof(gchar)*64); 
        memset(strBuf, 0x00, sizeof(strBuf));
                   
        if(_get_rule_name_string(vaaid, i, strBuf))
        { 
            g_sprintf(strBuf,"ZONE %d",i+1);
            strcpy(zone_str[i], strBuf);    
            ifree(strRule);
            g_rule_onoff[i] = OPT_RULETYPE_ALLMSK;
            continue;
        }
        else
        {   
            g_sprintf(strRule,"%s",strBuf);
            strcpy(zone_str[i], strBuf);
            nfui_listbox_set_text(NF_LISTBOX(g_rulelist_obj), &strRule);
            ifree(strRule);
            g_rule_onoff[i] = OPT_RULETYPE_ALLMSK;
            g_exist_zone |= (1<<i);
            g_zone_cnt++;
        }
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
    nfui_nfobject_set_size(obj, 120, 34);
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
   
    // EXPORT DATA     

    obj = nftool_normal_button_create_type3("EXPORT DATA", SSV_EXPORT_BUTTON_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, SSV_EXPORT_BUTTON_X, SSV_EXPORT_BUTTON_Y);
    nfui_regi_post_event_callback(obj, post_export_event_handler);      
    g_export_obj = obj;
 
     // FIXED2 
    // RANGE TITLE / BARGRAPH
    opt |= (1 << OPT_SMART);
    
    fixed2 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed2, SSV_FIXED2_W, SSV_FIXED2_H);
    nfui_nfobject_show(fixed2);
    nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SSV_FIXED2_X, SSV_FIXED2_Y);
    nfui_regi_post_event_callback(fixed2,post_fixed2_event_handler);

    vw_vca_statistic_total_component_open(fixed2, opt);
    // FIXED3
    // AVERAGE / BARGRAPH 
    fixed3 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed3, SSV_FIXED3_W, SSV_FIXED3_H);
    nfui_nfobject_show(fixed3);
    nfui_nfobject_modify_bg(fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed3, SSV_FIXED3_X, SSV_FIXED3_Y);
    nfui_regi_post_event_callback(fixed3,post_fixed3_event_handler);
 
    vw_vca_statistic_average_component_open(fixed3, opt);
    
    // FIXED4 
    // RATE - PIECHART
    fixed4 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed4, SSV_FIXED4_W, SSV_FIXED4_H);
    nfui_nfobject_show(fixed4);
    nfui_nfobject_modify_bg(fixed4, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed4, SSV_FIXED4_X, SSV_FIXED4_Y);
    nfui_regi_post_event_callback(fixed4, post_fixed4_event_handler);

    vw_vca_statistic_rate_component_open(fixed4, opt);
 
    obj = nftool_normal_button_create_subtab_type1("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SSV_CLOSE_X, SSV_CLOSE_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	
	nfui_regi_post_event_callback(opt_fixed, post_opt_fixed_event_handler);

    _init_statistic_component_data(nfui_nfobject_get_top(main_fixed));
    _init_statistic_component_action(nfui_nfobject_get_top(main_fixed));
    
    _init_statistic_total_component_data(nfui_nfobject_get_top(main_fixed));
    _init_statistic_average_component_data(nfui_nfobject_get_top(main_fixed));
    _init_statistic_rate_component_data(nfui_nfobject_get_top(main_fixed));
 
    _prepare_display_statistic(nfui_nfobject_get_top(main_fixed));
 
	vw_vca_statistic_total_component_sync_data(fixed2);    
	vw_vca_statistic_average_component_sync_data(fixed3);	
	vw_vca_statistic_rate_component_sync_data(fixed4);

    nfui_nfobject_show(main_wnd);
    nfui_nfobject_show(content_fixed);

    nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(main_fixed));

	nfui_page_open(PGID_POPUPWND, g_curwnd, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_POPUPWND, g_curwnd);

}
