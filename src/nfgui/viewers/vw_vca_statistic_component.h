/*
 * vw_vca_statistic_component.h
 *
 * Written by Jaeyoung. <miracle0406@itxsecurity.com>
 * Copyright (c) ITX security, Dec 9, 2014
 *
 */

#ifndef	__VCA_STATISTIC_COMPONENT_H
#define __VCA_STATISTIC_COMPONENT_H

typedef int (*STATISTIC_COMPONENT_ACTION_CALLBACK)(gpointer user_data);

#define STATISTIC_COMPONENT_DATA			"statistic_component_data"
#define STATISTIC_COMPONENT_ACTION          "statistic_component_action"

#define TEXT_SIZE       16


enum{
    PERIOD_TYPE_HOUR  = 0,
    PERIOD_TYPE_DAY,
    PERIOD_TYPE_WEEK,
    PERIOD_TYPE_MONTH,
    PERIOD_TYPE_YEAR,
    PERIOD_TYPE_MAX,
};

enum{
    AVERAGE_TYPE_NONE = 0,
    AVERAGE_TYPE_HOUR,
    AVERAGE_TYPE_WEEK,
    AVERAGE_TYPE_DAY,
};

enum {
    OPT_LIVE  = 0,
    OPT_SMART,
};

typedef struct _STATISTIC_COMPONENT_TOTAL_T {

	time_t		calendar_time;
	gint 		event_count;
	gint		period_type;
	
	gint		hour_value[13];
	gchar		hour_text[13][TEXT_SIZE];
	
	gint		day_value[25];
	gchar		day_text[25][TEXT_SIZE];
	
	gint		week_value[7];
	gchar		week_text[7][TEXT_SIZE];
	
	gint		month_value[32];
	gchar		month_text[32][TEXT_SIZE];
	
	gint		year_value[13];
	gchar		year_text[13][TEXT_SIZE];

    gint        bar_idx;
    gint        init_period_type;
    time_t      start_time;
    time_t      end_time;
    
	gint        hour_cnt;
	gint        day_cnt;
	gint        week_cnt;
	gint        month_cnt;
	gint        year_cnt;

	gint        bar_init_gap;
	
} STATISTIC_COMPONENT_TOTAL_T;

typedef struct _STATISTIC_COMPONENT_AVERAGE_T {

	gchar		average_cap[6][16];
	gint        average_cnt;
	gint 		average_type;
	
	gint		hour_value[24];
	gchar		hour_text[24][TEXT_SIZE];
	
	gint		day_value[31];
	gchar		day_text[31][TEXT_SIZE];
	
	gint		week_value[7];
	gchar		week_text[7][TEXT_SIZE];
	
	gint        hour_cnt;
	gint        day_cnt;
	gint        week_cnt;
	
} STATISTIC_COMPONENT_AVERAGE_T;

typedef struct _STATISTIC_COMPONENT_RATE_T {

	gint 		total_val;
	
	gint 		zone_val[16];
	gint 		event_val[7];	

	gint        zone_cnt;
	gdouble     zone_ratio_val[16];
	gint        event_cnt;
	gdouble     event_ratio_val[7];

	gchar       zone_str[16][32];
	
} STATISTIC_COMPONENT_RATE_T;


typedef struct _STATISTIC_COMPONENT_DATA_T{

    STATISTIC_COMPONENT_TOTAL_T total;
    STATISTIC_COMPONENT_AVERAGE_T average;
    STATISTIC_COMPONENT_RATE_T  rate;

} STATISTIC_COMPONENT_DATA_T;

typedef struct _STATISTIC_COMPONENT_ACTION_T {
    STATISTIC_COMPONENT_ACTION_CALLBACK		total_previous_cb;
    STATISTIC_COMPONENT_ACTION_CALLBACK		total_next_cb;
    STATISTIC_COMPONENT_ACTION_CALLBACK		total_graph_press;
    STATISTIC_COMPONENT_ACTION_CALLBACK		total_graph_2press;
    STATISTIC_COMPONENT_ACTION_CALLBACK		average_type_cb;
    
} STATISTIC_COMPONENT_ACTION_T;




////////////////////////////////////////////////////////////
//
// public interfaces
//

////////////////////////////////////////////////////////////
// TOTAL COMPONENT

gint vw_vca_statistic_total_component_open(NFOBJECT *parent, guint opt);
gint vw_vca_statistic_total_component_show();
gint vw_vca_statistic_total_component_hide();
gint vw_vca_statistic_total_component_sync_data(NFOBJECT *parent);
gint vw_vca_statistic_total_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// AVERAGE COMPONENT

gint vw_vca_statistic_average_component_open(NFOBJECT *parent, guint opt);
gint vw_vca_statistic_average_component_show();
gint vw_vca_statistic_average_component_hide();
gint vw_vca_statistic_average_component_sync_data(NFOBJECT *parent);
gint vw_vca_statistic_average_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// RATE COMPONENT

gint vw_vca_statistic_rate_component_open(NFOBJECT *parent, guint opt);
gint vw_vca_statistic_rate_component_show();
gint vw_vca_statistic_rate_component_hide();
gint vw_vca_statistic_rate_component_sync_data(NFOBJECT *parent);
gint vw_vca_statistic_rate_component_expose(NFOBJECT *parent);

#endif

