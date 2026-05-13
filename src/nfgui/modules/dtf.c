/*
 * dtf.c
 * 	- date time formmater
 *	- dependency :
 *		ifn
 *		DAL
 *
 * Written by Jung-kyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Jul 18, 2011
 *
 */


#include "dtf.h"
#include "ix_func.h"
#include <memory.h>
#include <string.h>
#include "nfdal.h"
#include <glib/gprintf.h>
#include "support/util.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"DTF"


////////////////////////////////////////////////////////////
//
// private data type 
//


////////////////////////////////////////////////////////////
//
// private variable
//



////////////////////////////////////////////////////////////
//
// private interfaces
//

static gint _convert_date_format(time_t time, FM_DATE_E fm_date, gchar *str)
{
	struct tm ttm;

	localtime_r(&time, &ttm);
	
	return ifn_convert_date(&ttm, fm_date, str);
}
static gint _convert_day(time_t time, gchar *str, int* week, int* mon, int* day)
{
	struct tm ttm;
	guint dformat = 0;
	FM_DATE_E fm_date; 

	DAL_get_dateTime_format(&dformat, NULL);

	if (dformat == 0)
		fm_date = YYYYMMDD;
	else if (dformat == 1)
		fm_date = MMDDYYYY;
	else
		fm_date = DDMMYYYY;	

	localtime_r(&time, &ttm);
	
	//return ifn_convert_date(&ttm, fm_date, str);
	return ifn_convert_day(&ttm, week, mon, day);

}

static gint _convert_date(time_t time, gchar *str)
{
	struct tm ttm;
	guint dformat = 0;
	FM_DATE_E fm_date; 

	DAL_get_dateTime_format(&dformat, NULL);

	if (dformat == 0)
		fm_date = YYYYMMDD;
	else if (dformat == 1)
		fm_date = MMDDYYYY;
	else
		fm_date = DDMMYYYY;	

	localtime_r(&time, &ttm);
	
	return ifn_convert_date(&ttm, fm_date, str);
}

static gint _convert_date_holiday(time_t time, gchar *str)
{
	struct tm ttm;
	FM_DATE_E fm_date; 

	fm_date = YYMMDD_HOL;

	localtime_r(&time, &ttm);
	
	return ifn_convert_date(&ttm, fm_date, str);
}
static gint _convert_time(time_t time, gchar *str)
{
	struct tm *pttm;
	guint tformat = 0;
	FM_TIME_E fm_time;

	DAL_get_dateTime_format(NULL, &tformat);

	if (tformat == 0)
		fm_time = H24;
	else
		fm_time = H12;	

	pttm = NFLOCALTIME(&time);

	return ifn_convert_time(pttm, fm_time, str);
}

static gint _convert_date_ex(time_t time, gchar *str)
{
	struct tm ttm;
	guint dformat = 0;
	FM_DATE_E fm_date; 

	DAL_get_dateTime_format(&dformat, NULL);

	if (dformat == 0)
		fm_date = YYYYMMDD;
	else if (dformat == 1)
		fm_date = MMDDYYYY;
	else
		fm_date = DDMMYYYY;	

	localtime_r(&time, &ttm);
	
	return ifn_convert_date_ex(&ttm, fm_date, str);
}

static gint _convert_time_ex(time_t time, gchar *str)
{
	struct tm *pttm;
	guint tformat = 0;
	FM_TIME_E fm_time;

	DAL_get_dateTime_format(NULL, &tformat);

	if (tformat == 0)
		fm_time = H24;
	else
		fm_time = H12;	

	pttm = NFLOCALTIME(&time);

	return ifn_convert_time_ex(pttm, fm_time, str);
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

gint dtf_get_current_date(gchar *str)
{
	time_t cur_time;
	gint len;
	GTimeVal tv;

	memset(&tv, 0x00, sizeof(GTimeVal));
	g_get_current_time(&tv);
	cur_time = tv.tv_sec;
	len = _convert_date(cur_time, str);
	return len;
}

gint dtf_get_current_date_with_dformat(FM_DATE_E fm_date, gchar *str)
{
	time_t cur_time;
	gint len;
	GTimeVal tv;

	memset(&tv, 0x00, sizeof(GTimeVal));
	g_get_current_time(&tv);
	cur_time = tv.tv_sec;
	len = _convert_date_format(cur_time, fm_date, str);
	return len;
}

gint dtf_get_current_time(gchar *str)
{
	time_t cur_time;
	gint len;
	GTimeVal tv;

	memset(&tv, 0x00, sizeof(GTimeVal));

	g_get_current_time(&tv);
	cur_time = tv.tv_sec;
	len = _convert_time(cur_time, str);
	return len;
}

gint dtf_get_current_datetime(gchar *str)
{
	time_t cur_time;
	gchar strDate[16];
	gchar strTime[16];
	gint len = 0;
	GTimeVal tv;

	memset(&tv, 0x00, sizeof(GTimeVal));

	memset(strDate, 0x00, sizeof(strDate));
	memset(strTime, 0x00, sizeof(strTime));
	
	g_get_current_time(&tv);
	cur_time = tv.tv_sec;
	len += _convert_date(cur_time, strDate);
	len += strlen("  ");
	len += _convert_time(cur_time, strTime);

	strcpy(str, strDate);
	strcat(str, "  ");	
	strcat(str, strTime);

	return len;
}

gint dtf_get_local_date(time_t time, gchar *str)
{
	gint len;

	len = _convert_date(time, str);
	return len;
}

gint dtf_get_local_date_with_dformat(time_t time, FM_DATE_E fm_date, gchar *str)
{
	gint len;

	len = _convert_date_format(time, fm_date, str);
	return len;
}

gint dtf_get_local_date_holiday(time_t time, gchar *str)
{
	gint len;

	len = _convert_date_holiday(time, str);
	return len;
}

gint dtf_get_local_dayOfWeek(time_t time, gchar *str, int* week, int* mon, int* day)
{
	gint dayOfweek;

	//len = _convert_date(time, str);
	dayOfweek = _convert_day(time, str, week, mon, day);

	return dayOfweek;
}

gint dtf_get_local_time(time_t time, gchar *str)
{
	gint len;

	len = _convert_time(time, str);
	return len;
}

gint dtf_get_local_datetime(time_t time, gchar *str)
{
	gchar strDate[16];
	gchar strTime[16];
	gint len = 0;

	memset(strDate, 0x00, sizeof(strDate));
	memset(strTime, 0x00, sizeof(strTime));
	
	len += _convert_date(time, strDate);
	len += strlen("  ");
	len += _convert_time(time, strTime);

	strcpy(str, strDate);
	strcat(str, "  ");	
	strcat(str, strTime);

	return len;
}

gint dtf_get_local_datetime_ex(time_t time, gchar *str)
{
	gchar strDate[16];
	gchar strTime[16];
	gint len = 0;

	memset(strDate, 0x00, sizeof(strDate));
	memset(strTime, 0x00, sizeof(strTime));
	
	len += _convert_date_ex(time, strDate);
	len += 1;
	len += _convert_time_ex(time, strTime);

	strcpy(str, strDate);
	strcat(str, "_");
	strcat(str, strTime);

	return len;
}

gint dtf_get_local_datetime_range(time_t from, time_t to, gchar *str)
{
	gchar strDate[16];
	gchar strTime[16];
	gint len = 0;

	memset(strDate, 0x00, sizeof(strDate));
	memset(strTime, 0x00, sizeof(strTime));
	
	len += _convert_date(from, strDate);
	len += strlen("  ");
	len += _convert_time(from, strTime);

	strcpy(str, strDate);
	strcat(str, "  ");	
	strcat(str, strTime);

	strcat(str, " ~ ");    
	len += strlen(" ~ ");

	len += _convert_date(to, strDate);
	len += strlen("  ");
	len += _convert_time(to, strTime);

	strcat(str, strDate);
	strcat(str, "  ");	
	strcat(str, strTime);
	
	return len;
}

gint dtf_get_thumbnail_date(gint year, gint mon, gint day, gchar *str)
{
	guint dformat = 0;
	FM_DATE_E fm_date; 

	DAL_get_dateTime_format(&dformat, NULL);

	if (day)
	{
		if (dformat == 0)
			g_sprintf(str, "%04d-%02d-%02d", year, mon, day);
		else if (dformat == 1)
			g_sprintf(str, "%02d-%02d-%04d", mon, day, year);
		else
			g_sprintf(str, "%02d-%02d-%04d", day, mon, year);
	}
	else
	{
		if (dformat == 0)
			g_sprintf(str, "%04d-%02d", year, mon);
		else
			g_sprintf(str, "%02d-%04d", mon, year);
	}

	return strlen(str);
}

gint dtf_get_local_hourmin(time_t timet, int *hour, int *min, int *sec)
{
	struct tm ttm;
    ttm = *NFLOCALTIME(&timet);
    if (hour) *hour = ttm.tm_hour;
	if (min) *min = ttm.tm_min;
	if (sec) *sec = ttm.tm_sec;
	return 0;
}

gint dtf_get_local_day(time_t timet, int *year, int *mon, int *day)
{
	struct tm ttm;
    ttm = *NFLOCALTIME(&timet);
	if (year) *year = ttm.tm_year + 1900;
	if (mon) *mon = ttm.tm_mon + 1;
	if (day) *day = ttm.tm_mday;

	return 0;
}

gint dtf_get_localtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	ttm = *NFLOCALTIME(&time);
	
	ifn_convert_date(&ttm, fm_date, buf_date);
	ifn_convert_time(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "  ");	
	strcat(buf, buf_time);

	return 0;
}




