#ifndef __NF_UTIL_TIME_H__
#define __NF_UTIL_TIME_H__

#include <time.h>

#define	NF_LOCALTIME_PATH	"/etc/localtime"
#define	NF_ZONEINFO_PATH	"/usr/share/zoneinfo/"

typedef struct _NF_TIME_UDD_T {
	gint udd_use;
	gint dst_offset;
	guint dst_start;
	guint dst_end;
	gchar start_name[128];
	gchar end_name[128];
} NF_TIME_UDD;

gint nf_zoneinfo_get_count();
gchar *nf_zoneinfo_get_string( gint index);

gboolean nf_zoneinfo_init();

gboolean nf_zoneinfo_set(gint index, NF_TIME_UDD *tz_udd);
gboolean nf_zoneinfo_get( const gchar *tzname, time_t utc_time, gint *offset, gboolean *is_dst);

gboolean nf_datetime_set(time_t utc_tval);
gboolean nf_datetime_rtc_sync(gboolean is_force);

gboolean nf_datetime_rtc_get(char *rtc_time);

gboolean nf_datetime_is_dst(time_t time);
gboolean nf_datetime_is_dst_now( void );
GTimeVal nf_datetime_get_dstoff_tv( void );

void nf_datetime_localtime(time_t *ttime, gboolean is_dst, struct tm *stTime);

#define MAX_TZINFO_ACTIVEX_CNT 64

typedef struct _TZINFO_ACTIVEX_T {
	gint	year;
	gint 	flag;

	gint		start_offset;
	gint		start_utc_time;
	struct tm	start_tm;
	gchar		start_asctime[32];
	gchar		start_abbr[8];

	gint		end_offset;
	gint		end_utc_time;
	struct tm	end_tm;
	gchar		end_asctime[32];
	gchar		end_abbr[8];

	gchar		string[256];

} TZINFO_ACTIVEX;

typedef struct _NF_TZINFO_ACTIVEX_T {
	gchar			zonename[128];
	gint		    tz_cnt;
	TZINFO_ACTIVEX	tz_arr[MAX_TZINFO_ACTIVEX_CNT];

} NF_TZINFO_ACTIVEX;

gboolean nf_zoneinfo_get_activex( const gchar *tzname, NF_TZINFO_ACTIVEX *activex);

#endif
