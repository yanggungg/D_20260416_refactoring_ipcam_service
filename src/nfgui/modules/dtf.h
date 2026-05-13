/*
 * dtf.h
 * 	- date time formmater
 *	- dependency :
 *		ifn
 *		DAL
 *
 * Written by Jung-kyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Jul 18, 2011
 *
 */


#ifndef __DTF_H
#define __DTF_H

#include "ix_func.h"
#include "iux_afx.h"




////////////////////////////////////////////////////////////
//
// public data type 
//






////////////////////////////////////////////////////////////
//
// public interfaces
//

gint dtf_get_current_date(gchar *str);
gint dtf_get_current_date_with_dformat(FM_DATE_E fm_date, gchar *str);
gint dtf_get_current_time(gchar *str);
gint dtf_get_current_datetime(gchar *str);

gint dtf_get_local_date(time_t time, gchar *str);
gint dtf_get_local_date_with_dformat(time_t time, FM_DATE_E fm_date, gchar *str);
gint dtf_get_local_time(time_t time, gchar *str);
gint dtf_get_local_datetime(time_t time, gchar *str);

gint dtf_get_thumbnail_date(gint year, gint mon, gint day, gchar *str);

gint dtf_get_local_hourmin(time_t timet, int *hour, int *min, int *sec);
gint dtf_get_local_day(time_t timet, int *year, int *mon, int *day);
gint dtf_get_localtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf);

#endif

