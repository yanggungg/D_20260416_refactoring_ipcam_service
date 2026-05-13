#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <linux/rtc.h>

#include "nf_common.h"
#include "nf_util_time.h"

#include "proxy_cli.h"

#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
#include "nf_solo_common.h"
#include "nf_solo_enc.h"
#endif /* _OTM_MODEL */

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_time"

#define DEBUG_TZINFO_LOG
#define DEBUG_TZINFO_JBSHELL

#ifdef DEBUG_TZINFO_JBSHELL
	#include "jbshell.h"
#endif

#define RTC_PATH "/dev/rtc0"

#if 0	// old choissinf 2008-11-13 오후 12:01:55

static char *_zoneinfo_str[] = {
	 "UTC",
	 "Pacific/Midway",
	 "US/Hawaii",
	 "America/Anchorage",
	 "America/Los_Angeles",
	 "America/Phoenix",
	 "US/Central",
	 "US/Eastern",
	 "America/Halifax",
	 "America/St_Johns",
	 "America/Sao_Paulo",
	 "Etc/GMT-2",
	 "Atlantic/Azores",
	 "GMT",
	 "Europe/Berlin",
	 "Europe/Istanbul",
	 "Africa/Cairo",
	 "Europe/Moscow",
	 "Asia/Tehran",
	 "Asia/Muscat",
	 "Asia/Kabul",
	 "Asia/Karachi",
	 "Asia/Calcutta",
	 "Asia/Katmandu",
	 "Asia/Dhaka",
	 "Asia/Bangkok",
	 "Asia/Shanghai",
	 "Asia/Tokyo",
	 "Asia/Seoul",
	 "Australia/Darwin",
	 "Australia/Adelaide",
	 "Australia/Brisbane",
	 "Pacific/Noumea",
	 "Pacific/NZ"
};

#else

const char *_zoneinfo_str[] = {
/* 00 */	 "GMT-12:00 ",						// Etc/GMT+12
/* 01 */	 "GMT-11:00 Pacific/Midway",		
/* 02 */	 "GMT-10:00 Pacific/Hawaii",		// Us/Hawaii
/* 03 */	 "GMT-09:00 America/Anchorage",
/* 04 */	 "GMT-08:00 America/LA",			// America/Los_Angeles
/* 05 */	 "GMT-07:00 America/Phoenix",
/* 06 */	 "GMT-06:00 America/CST",			// Us/Central
/* 07 */	 "GMT-05:00 America/EST",			// Us/Eastern
/* 08 */	 "GMT-04:00 America/Halifax",
/* 09 */	 "GMT-03:30 America/St_Johns",
/* 10 */	 "GMT-03:00 America/Sao_Paulo",
/* 11 */	 "GMT-02:00 Mid Atlantic",			// Etc/GMT+2
/* 12 */	 "GMT-01:00 Atlantic/Azores",
/* 13 */	 "GMT+00:00 Europe/London",
/* 14 */	 "GMT+01:00 Europe/Berlin",
/* 15 */	 "GMT+02:00 Europe/Istanbul",
/* 16 */	 "GMT+02:00 Africa/Cairo",
/* 17 */	 "GMT+03:00 Europe/Moscow",
/* 18 */	 "GMT+03:30 Asia/Tehran",
/* 19 */	 "GMT+04:00 Asia/Muscat",
/* 20 */	 "GMT+04:30 Asia/Kabul",
/* 21 */	 "GMT+05:00 Asia/Karachi",
/* 22 */	 "GMT+05:30 Asia/Calcutta",
/* 23 */	 "GMT+05:45 Asia/Katmandu",
/* 24 */	 "GMT+06:00 Asia/Dhaka",
/* 25 */	 "GMT+06:30 Asia/Rangoon",
/* 26 */	 "GMT+07:00 Asia/Bangkok",
/* 27 */	 "GMT+08:00 Asia/Beijing",			// Copy Shanghai
/* 28 */	 "GMT+09:00 Asia/Tokyo",
/* 29 */	 "GMT+09:00 Asia/Seoul",
/* 30 */	 "GMT+09:30 Australia/Darwin",
/* 31 */	 "GMT+09:30 Australia/Adelaide",
/* 32 */	 "GMT+10:00 Australia/Brisbane",
/* 33 */	 "GMT+11:00 Pacific/Noumea",
/* 34 */	 "GMT+12:00 Newzealand",
/* 35 */	 "GMT+08:00 Australia/Perth",
				
				// 2009-10-24 오후 4:24:00
/* 36 */	 "GMT+02:00 Middle East/Jordan",		
/* 37 */	 "GMT+02:00 Middle East/Lebanon",
/* 38 */	 "GMT+02:00 Middle East/Syria",
/* 39 */	 "GMT+03:00 Middle East/SaudiArabia" ,
/* 40 */	 "GMT+03:00 Middle East/Iraq",
/* 41 */	 "GMT+03:30 Middle East/Iran",
/* 42 */	 "GMT+04:00 Middle East/UAE",

/* 43 */	 "GMT+10:00 Australia/Sydney",
/* 44 */     "GMT-07:00 America/Edmonton"

}; 

const char *_zoneinfo_str_tz[] = {
/* 00 */	 "Etc/GMT-12",		
/* 01 */	 "Pacific/Midway",
/* 02 */	 "US/Hawaii",
/* 03 */	 "America/Anchorage",
/* 04 */	 "America/Los_Angeles",
/* 05 */	 "America/Phoenix",
/* 06 */	 "US/Central",
/* 07 */	 "US/Eastern",
/* 08 */	 "America/Halifax",
/* 09 */	 "America/St_Johns",
/* 10 */	 "America/Sao_Paulo",
/* 11 */	 "Etc/GMT-2",				// update
/* 12 */	 "Atlantic/Azores",
/* 13 */	 "Europe/London",
/* 14 */	 "Europe/Berlin",
/* 15 */	 "Europe/Istanbul",
/* 16 */	 "Africa/Cairo",
/* 17 */	 "Europe/Moscow",
/* 18 */	 "Asia/Tehran",
/* 19 */	 "Asia/Muscat",
/* 20 */	 "Asia/Kabul",
/* 21 */	 "Asia/Karachi",
/* 22 */	 "Asia/Calcutta",
/* 23 */	 "Asia/Katmandu",
/* 24 */	 "Asia/Dhaka",
/* 25 */	 "Asia/Rangoon",
/* 26 */	 "Asia/Bangkok",
/* 27 */	 "Asia/Shanghai",
/* 28 */	 "Asia/Tokyo",
/* 29 */	 "Asia/Seoul",
/* 30 */	 "Australia/Darwin",
/* 31 */	 "Australia/Adelaide",
/* 32 */	 "Australia/Brisbane",
/* 33 */	 "Pacific/Noumea",
/* 34 */	 "NZ",
/* 35 */	 "Australia/Perth",
				// choissi 2009-10-24 오후 4:01:48 
				// 리눅스 zoneinfo에 없음 
/* 36 */	 "Asia/Amman",			// GMT+02:00 Middle East/Jordan   (Asia/Amman)
/* 37 */	 "Asia/Beirut",			// GMT+02:00 Middle East/Lebanon  (Asia/Beirut)
/* 38 */	 "Asia/Damascus",		// GMT+02:00 Middle East/Syria    (Asia/Damascus)  
/* 39 */	 "Asia/Riyadh" ,	//new	// GMT+03:00 Middle East/SaudiArabia (Asia/Riyadh)
/* 40 */	 "Asia/Baghdad",		// GMT+03:00 Middle East/Iraq	(Asia/Baghdad)
/* 41 */	 "Asia/Tehran",			// GMT+03:30 Middle East/Iran	(Asia/Tehran)
/* 42 */	 "Asia/Dubai",			// GMT+04:00 Middle East/UAE	(Asia/Dubai)

/* 43 */	 "Australia/Sydney",	// new
/* 44 */	 "America/Edmonton"      // 2010-07-15 오후 12:44:49 choissi add

}; 

#endif
#define	NUM_TZINFO	(sizeof(_zoneinfo_str)/sizeof(char *))

gint nf_zoneinfo_get_count()
{
	return NUM_TZINFO;
}

gchar *nf_zoneinfo_get_string( gint index)
{
	g_return_val_if_fail( index < NUM_TZINFO, NULL);
					
	return _zoneinfo_str[index];
}

//#define USR_DBG

#define USRD_INFO 		"/usr/share/zoneinfo/Etc/USRD"
#define USRD_TMP_INFO	"/tmp/USRD"
#define USRD_CONF		"/tmp/USRD.conf"

#define ZIC				"/NFDVR/zic"
#define YEARISTYPE		"/NFDVR/yearistype"

gboolean _get_user_dst_db(NF_TIME_UDD *udd)
{
	gchar *str = NULL;

	memset(udd, 0x0, sizeof(NF_TIME_UDD));
	
	udd->udd_use = nf_sysdb_get_int("sys.date.udd_use");
	udd->dst_offset = nf_sysdb_get_int("sys.date.dst_offset");
	udd->dst_start = nf_sysdb_get_uint("sys.date.dst_start");
	udd->dst_end = nf_sysdb_get_uint("sys.date.dst_end");

	str = nf_sysdb_get_str_nocopy("sys.date.dst_name");
	if( str )
		snprintf(udd->start_name, sizeof(udd->start_name), "%s", str);

	str = nf_sysdb_get_str_nocopy("sys.date.std_name");
	if( str )	
		snprintf(udd->end_name, sizeof(udd->end_name), "%s", str);

	return TRUE;
}

gboolean _is_user_dst(NF_TIME_UDD udd)
{
	guint dst_start, dst_end;
	int dst_offset;	
	gint udd_use;

	udd_use = udd.udd_use;	
	dst_offset = udd.dst_offset;
	dst_start = udd.dst_start;
	dst_end = udd.dst_end;

#ifdef USR_DBG
	g_message("%s - %d", __FUNCTION__, dst_offset);
	g_message("%s - %u", __FUNCTION__, dst_start);
	g_message("%s - %u", __FUNCTION__, dst_end);
#endif
	
	if(udd_use == 0 || dst_offset == 0 || dst_start == 0 || dst_end == 0)
		return FALSE;
	else
		return TRUE;
}

void _generate_zoneinfo_file(char *start_year, char *start_time, char *start_name, char *save_time, char *end_year, char *end_time, char *end_name,char *gmt_time)
{
    FILE *fp;		

	remove(USRD_CONF);

#ifdef USR_DBG
	g_message("%s - %d - %s", __FUNCTION__, __LINE__, start_year);
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,start_time);
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,save_time);
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,start_name);	
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,end_year);
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,end_time);
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,end_name);	
	g_message("%s - %d - %s", __FUNCTION__, __LINE__,gmt_time);
#endif

    if ( (fp = fopen(USRD_CONF, "w")) == NULL )
    {
        g_warning("%s file oepn error", __FUNCTION__);
    }
	else
	{		
		fprintf(fp, "Rule\tUSRD\t%s\t%s\t-\t%s\t%s\t%s\n", start_year, "only", start_time, save_time, start_name);
		fprintf(fp, "Rule\tUSRD\t%s\t%s\t-\t%s\t%s\t%s\n", end_year, "only", end_time, "0", end_name);
		fprintf(fp, "Rule\tUSRD\t%s\t%s\t-\t%s\t%s\t%s\n", "2037", "only", start_time, save_time, start_name);
		fprintf(fp, "Zone\tUSRD\t%s\tUSRD\t%s\n", gmt_time, "%s");
		
		fclose(fp);
	}

	char cmd[1024];

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "%s -y %s -d /tmp -L /dev/null %s", ZIC, YEARISTYPE, USRD_CONF);	
	proxy_system(cmd, 1, 3);

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "cp %s %s", USRD_TMP_INFO, USRD_INFO);
	proxy_system(cmd, 1, 3);	
}

void _make_time_format(gint time, char *buf, int buf_len)
{
	int hour, min, sec;
	int tmp;
	char minus;

#ifdef USR_DBG
	g_message("%s - %d", __FUNCTION__, time);
#endif

	if( time < 0 )
	{
		time = abs(time);
		minus = 1;
	}
	else
		minus = 0;

#ifdef USR_DBG
	g_message("%s - %d", __FUNCTION__, time);
#endif

	hour = time / 3600;
	tmp = time % 3600;

	min = tmp / 60;
	tmp = tmp % 60;

	sec = tmp;

	if(minus)
		snprintf(buf, buf_len, "-%d:%d:%d", hour, min, sec);
	else
		snprintf(buf, buf_len, "%d:%d:%d", hour, min, sec);

#ifdef USR_DBG
	g_message("%s time_format:%s", __FUNCTION__, buf);
#endif
}

static char *month_str[ ] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};

void _make_start_end_format(struct tm *st, char *year_buf, int year_buf_len, char *day_time_buf, int day_time_buf_len)
{	
	char month[16];
	
	snprintf(year_buf, year_buf_len, "%d", st->tm_year+1900);
	snprintf(day_time_buf, day_time_buf_len, "%s %d %d:%d:%d", month_str[st->tm_mon], st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec);

#ifdef USR_DBG
	g_message("%s year_buf:%s", __FUNCTION__, year_buf);
	g_message("%s day_time_buf:%s", __FUNCTION__, day_time_buf);
#endif	
}

gboolean nf_zoneinfo_set(gint index, NF_TIME_UDD *tz_udd)
{
	char link_buf[256];
	char tz_buf[256];
	gboolean is_udd;

	NF_TIME_UDD udd;

	g_return_val_if_fail( index < NUM_TZINFO, 0);

	memset(link_buf, 0x0, sizeof(link_buf));
	memset(tz_buf, 0x0, sizeof(tz_buf));	

#ifdef USR_DBG
	g_message("%s - index:%d", __FUNCTION__, index);
#endif	

	if( tz_udd == NULL )
	{		
		_get_user_dst_db(&udd);
#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif
	}
	else
	{
		memcpy(&udd, tz_udd, sizeof(NF_TIME_UDD));
#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
		g_message("TZ_SET =>udd_use[%d]", udd.udd_use);		
		g_message("TZ_SET =>dst_offset[%d]", udd.dst_offset);
		g_message("TZ_SET =>dst_start[%u]", udd.dst_start);
		g_message("TZ_SET =>dst_end[%u]", udd.dst_end);
		g_message("TZ_SET =>start_name[%s]", udd.start_name);
		g_message("TZ_SET =>end_name[%s]", udd.end_name);
#endif		
	}

	is_udd = _is_user_dst(udd);

	if(is_udd)
	{	
		gchar *tz_name = NULL;
		gint offset;
		gboolean is_dst;
		struct tm *st;
		char start_year[16];
		char start_time[256];
		
		char end_year[16];
		char end_time[256];
		
		char save_time[256];
		char gmt_time[256];	

#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif
		tz_name = nf_zoneinfo_get_string(index);

		nf_zoneinfo_get( tz_name, 0, &offset, &is_dst);	
		_make_time_format(offset, gmt_time, sizeof(gmt_time));

#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif

		_make_time_format(udd.dst_offset, save_time, sizeof(save_time));

#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif

		st = gmtime( &(udd.dst_start) );
		_make_start_end_format(st, start_year, sizeof(start_year), start_time, sizeof(start_time));

#ifdef USR_DBG
		g_message("%s start_time [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec);

		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif

		st = gmtime( &(udd.dst_end) );
		_make_start_end_format(st, end_year, sizeof(end_year), end_time, sizeof(end_time));

#ifdef USR_DBG
		g_message("%s end_time [%04d/%02d/%02d-%02d:%02d:%02d]", __FUNCTION__,
				st->tm_year+1900, st->tm_mon+1, st->tm_mday, 
				st->tm_hour, st->tm_min, st->tm_sec);

		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif

		_generate_zoneinfo_file(start_year, start_time, udd.start_name, save_time, end_year, end_time, udd.end_name, gmt_time);

#ifdef USR_DBG
		g_message("%s - %d", __FUNCTION__, __LINE__);
#endif
		snprintf(link_buf, sizeof(link_buf), "%s", USRD_INFO);
		snprintf(tz_buf, sizeof(tz_buf), "Etc/USRD");
	}
	else
	{
		snprintf(link_buf, sizeof(link_buf), NF_ZONEINFO_PATH "%s", _zoneinfo_str_tz[index]);
		snprintf(tz_buf, sizeof(tz_buf), "%s", _zoneinfo_str_tz[index]);
	}
	
	g_message("%s tz[%s][%d]",__FUNCTION__, link_buf, index);
		
	if(unlink(NF_LOCALTIME_PATH) < 0)
	{
		g_warning("%s unlink[%s] %d(%s)\n", __FUNCTION__, 
				NF_LOCALTIME_PATH, errno, strerror(errno));
	}
						
	symlink(link_buf, NF_LOCALTIME_PATH);
	
	setenv("TZ", tz_buf, 1);	
	tzset();
		
	return 1;		
}

static int tzdump(int argc, char **argv);

gboolean nf_zoneinfo_init(void)
{
	guint tz_index;
	gchar *tz_name = NULL;
	gboolean ret;
		
	tz_index = nf_sysdb_get_uint("sys.date.tz_index");	
	tz_name = nf_zoneinfo_get_string(tz_index);
	
	g_return_val_if_fail( tz_name != NULL, 0);		

	ret = nf_zoneinfo_set(tz_index, NULL);
	
	g_message("%s tz_index[%d] tz_name[%s] ret[%d]", __FUNCTION__, tz_index, tz_name, ret);

//	tzdump(1, NULL);
	
	return 1;	
}

gboolean nf_datetime_is_dst( time_t tick)
{
	struct tm cur_cal_time;
	
	localtime_r(&tick, &cur_cal_time);	
			
	if( cur_cal_time.tm_isdst  )
		return 1;		
	else
		return 0;
}

gboolean nf_datetime_is_dst_now( void )
{
	time_t tick;		
	tick = time(NULL);
		
	return  nf_datetime_is_dst( tick );
}


GTimeVal nf_datetime_get_dstoff_tv( void )
{
	GTimeVal tv;	

	time_t tick;		
	struct tm cur_cal_time;
	
	tick = time(NULL);
	
	tv.tv_sec = tick;
	tv.tv_usec = 0;
	
	if( nf_datetime_is_dst( tick ) )
		tv.tv_sec -= 3600;	
	
	return tv;
}

static gboolean _datetime_rtc_lock = FALSE;

gboolean nf_datetime_rtc_sync( gboolean is_force)
{	
	static unsigned int cnt = 0;

	_datetime_rtc_lock = TRUE;
	
	if( ++cnt > 1024 || is_force )
	{
		cnt = 0;		
		//proxy_system("/sbin/hwclock -w --utc", 1, 10);	
		proxy_system("/sbin/hwclock -w -u", 1, 10);	 // for busybox  BusyBox v1.13.2
	}

	_datetime_rtc_lock = FALSE;
	
	return 1;
}

gboolean nf_datetime_rtc_get(char *rtc_time)
{
	FILE *fp;
	char buff[64] ={0,};	

	fp = popen("hwclock", "r");
	if (fp == NULL)
	{
		perror("erro : ");
		return 0;
	}
	fgets(buff, sizeof(buff), fp);

	memcpy(rtc_time, buff, sizeof(buff));
	pclose(fp);

	return 1;
}

gboolean nf_datetime_rtc_set(time_t utc_tval)
{
	struct tm st_buf;
	struct rtc_time rtc_buf;
	
	int rtc_fd;
	int ret;

	if( utc_tval < NF_DATETIME_MIN )
	{
		printf("[%s] utc_tval val error => [%u]\n", __FUNCTION__, utc_tval);
		return FALSE;
	}

	gmtime_r(&utc_tval, &st_buf);

	memset( &rtc_buf, 0x0, sizeof(struct rtc_time) );

	rtc_buf.tm_year = st_buf.tm_year;
	rtc_buf.tm_mon = st_buf.tm_mon;
	rtc_buf.tm_mday = st_buf.tm_mday;
	rtc_buf.tm_hour = st_buf.tm_hour;
	rtc_buf.tm_min = st_buf.tm_min;	
	rtc_buf.tm_sec = st_buf.tm_sec;
	
	if( _datetime_rtc_lock == TRUE )
	{
		printf("[%s] _datetime_rtc_lock is TRUE.. skip\n", __FUNCTION__);
		return FALSE;
	}

	rtc_fd = open(RTC_PATH, O_RDONLY);	
	if( rtc_fd < 0 ){
		printf("[%s] rtc open failed\n", __FUNCTION__);		
		return FALSE;
	}

	ret = ioctl(rtc_fd, RTC_SET_TIME, &rtc_buf);
	if(	ret < 0 ){
		printf("[%s] RTC_SET_TIME err [%d][%d][%s]\n", __FUNCTION__, ret, errno, strerror(errno));		
	}
	else{
		printf("[%s] RTC_SET_TIME set to [%u]\n", __FUNCTION__, utc_tval);
	}
	
	if( close(rtc_fd) < 0 ){
		printf("[%s] close error\n", __FUNCTION__);		
	}

	return TRUE;
}

#include "jbshell.h"

static char rtc_set_help[] = "rtc_set [utc_time]";
static int rtc_set(int argc, char **argv)
{
	guint utc_time;
	gchar rtc_str[1024];

	struct timeval sec_1, sec_2;

	
	if(argc < 2){
		printf("%s\n", rtc_set_help);
		return -1;
	}

	utc_time = strtoul(argv[1],NULL,10);

	gettimeofday(&sec_1, NULL);

	if( nf_datetime_rtc_set(utc_time) == FALSE )
	{
		printf("%s => nf_datetime_rtc_set FAIL\n", __FUNCTION__);
		return 0;
	}

	gettimeofday(&sec_2, NULL);

	printf("[%s] sec_1[%u.%06u] , sec_2[%u.%06u] => \n", __FUNCTION__, sec_1.tv_sec, sec_1.tv_usec, sec_2.tv_sec, sec_2.tv_usec);
	
	
	if( nf_datetime_rtc_get(rtc_str) == FALSE )
	{
		printf("%s => nf_datetime_rtc_get FAIL\n", __FUNCTION__);
		return 0;
	}
	else
	{
		printf("%s => rtc_str [%s]\n", __FUNCTION__, rtc_str);		
	}

	return 0;
}
__commandlist(rtc_set, "rtc_set",  rtc_set_help, rtc_set_help);

gboolean nf_datetime_set(time_t utc_tval)
{
#ifdef USE_SHELL_DATE
	struct tm st_buf;
  	char buf[64];
#else  	
  	struct timeval tv;
#endif		

	g_return_val_if_fail( utc_tval >= NF_DATETIME_MIN, 0); // 20080101 00:00:00

    g_message("[%s:%d]", __FUNCTION__, __LINE__);
    
#ifdef USE_SHELL_DATE		
	gmtime_r(&utc_tval, &st_buf);

	sprintf(buf,"date -u %02d%02d%02d%02d%04d.%02d",
		st_buf.tm_mon+1, 
		st_buf.tm_mday, 
		st_buf.tm_hour, 
		st_buf.tm_min, 
		st_buf.tm_year+1900, 
		st_buf.tm_sec);
	
	system(buf);
#else
	tv.tv_sec = utc_tval;
	tv.tv_usec = 0;
	
	settimeofday( &tv, NULL);	
#endif	
	//sync_time_to_dsp(val);
	
	nf_datetime_rtc_sync(1);
	//proxy_system("/sbin/hwclock -w --utc", 1, 10);

	// XXX add time sync

	return 1;
}

/******************************************************************************/

#define TZ_MAX_TIMES	1200
#define TZ_MAX_TYPES	32
#define TZ_MAX_CHARS	256
#define TZ_MAX_ARR		256
#define TZ_MAGIC		"TZif"

struct tzhead {
	char	tzh_magic[4];	   /* TZ_MAGIC */
	char	tzh_version[1];	 /* '\0' or '2' as of 2005 */
	char	tzh_reserved[15];	   /* reserved--must be zero */
	char	tzh_ttisgmtcnt[4];	  /* coded number of trans. time flags */
	char	tzh_ttisstdcnt[4];	  /* coded number of trans. time flags */
	char	tzh_leapcnt[4];	 /* coded number of leap seconds */
	char	tzh_timecnt[4];	 /* coded number of transition times */
	char	tzh_typecnt[4];	 /* coded number of local time types */
	char	tzh_charcnt[4];	 /* coded number of abbr. chars */
};

typedef struct tzhead NF_TZHEADER;

typedef struct _NF_TZTYPE_T
{
	int		offset;
	short	is_dst;
	short	abbr_idx;
} NF_TZTYPE;

typedef struct _NF_TZINFO_T
{
	char zone_name[TZ_MAX_CHARS];
		
	int version;
			
	// header 
	int ttisgmtcnt;	  /* coded number of trans. time flags */
	int ttisstdcnt;	  /* coded number of trans. time flags */
	int leapcnt;	 /* coded number of leap seconds */
	int timecnt;	 /* coded number of transition times */
	int typecnt;	 /* coded number of local time types */
	int charcnt;	 /* coded number of abbr. chars */	
	
	// data
	int	time_arr[TZ_MAX_TIMES];
	int	type_arr[TZ_MAX_TIMES];	
	
	NF_TZTYPE	type[TZ_MAX_TYPES];
	char abbr[TZ_MAX_CHARS];	
	
	char tisstd[TZ_MAX_ARR];
	char tisgmt[TZ_MAX_ARR];
		
} NF_TZINFO;

static long long
detzcode64(codep)
const char * const	  codep;
{
	register long long	  result;
	register int		i;

	result = (codep[0] & 0x80) ? ~0L : 0L;

	for (i = 0; i < 8; ++i)
		result = result * 256 + (codep[i] & 0xff);

	return result;
}

static long long
detzcode(codep)
const char * const	  codep;
{
	register long   result;
	register int	i;

	result = (codep[0] & 0x80) ? ~0L : 0L;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static void print_tzinfo(NF_TZINFO *tzinfo)
{
	struct tm	   tm;
	int i;	
	char utc_time[32], local_time[32];
		
	g_return_if_fail(tzinfo);
	
	g_message("zone_name  [%s][%s]", tzinfo->zone_name,tzinfo->abbr);
	g_message("version    [%d]", tzinfo->version);
	g_message("ttisgmtcnt [%d]", tzinfo->ttisgmtcnt);
	g_message("ttisstdcnt [%d]", tzinfo->ttisstdcnt);
	g_message("leapcnt    [%d]", tzinfo->leapcnt);
	g_message("timecnt    [%d]", tzinfo->timecnt);
	g_message("typecnt    [%d]", tzinfo->typecnt);
	g_message("charcnt    [%d]", tzinfo->charcnt);	

	for (i = 0; i < tzinfo->timecnt; ++i)
	{		
		int type_idx = tzinfo->type_arr[i];
		int abbr_offset = tzinfo->type[type_idx].abbr_idx;		
		time_t	local = tzinfo->time_arr[i] + tzinfo->type[type_idx].offset;
		
		gmtime_r(&tzinfo->time_arr[i], &tm);
		asctime_r(&tm, utc_time);
		utc_time[strlen(utc_time)-1] = 0;

		gmtime_r(&local , &tm);
		asctime_r(&tm, local_time);
		local_time[strlen(local_time)-1] = 0;
		
		g_message("[%3d] time[%12d] type[%2d] offset[%6d] dst[%d] [%s UTC] [%s %s]", 
				i,
				tzinfo->time_arr[i],
				tzinfo->type_arr[i],
				tzinfo->type[type_idx].offset,
				tzinfo->type[type_idx].is_dst,				
				utc_time, local_time, &tzinfo->abbr[abbr_offset] );
	}
		
}


static void print_tzinfo_simple(NF_TZINFO *tzinfo)
{
	struct tm	   tm;
	int i;	
	char utc_time[32], local_time[32];
		
	g_return_if_fail(tzinfo);
	
	g_message("zone_name  [%s][%s]", tzinfo->zone_name,tzinfo->abbr);
	g_message("version    [%d]", tzinfo->version);
	g_message("ttisgmtcnt [%d]", tzinfo->ttisgmtcnt);
	g_message("ttisstdcnt [%d]", tzinfo->ttisstdcnt);
	g_message("leapcnt    [%d]", tzinfo->leapcnt);
	g_message("timecnt    [%d]", tzinfo->timecnt);
	g_message("typecnt    [%d]", tzinfo->typecnt);
	g_message("charcnt    [%d]", tzinfo->charcnt);	

	for (i = 0; i < tzinfo->timecnt; ++i)
	{		
		int type_idx = tzinfo->type_arr[i];
		int abbr_offset = tzinfo->type[type_idx].abbr_idx;		
		time_t	local = tzinfo->time_arr[i] + tzinfo->type[type_idx].offset;
		
		gmtime_r(&tzinfo->time_arr[i], &tm);
		asctime_r(&tm, utc_time);
		utc_time[strlen(utc_time)-1] = 0;

		gmtime_r(&local , &tm);
		asctime_r(&tm, local_time);
		local_time[strlen(local_time)-1] = 0;
		
		if( tzinfo->time_arr[i] > NF_DATETIME_MIN )
			g_message("gmt_offset[%6d] is_dst[%d] [%s UTC] [%s %s]", 								
				tzinfo->type[type_idx].offset,
				tzinfo->type[type_idx].is_dst,				
				utc_time, local_time, &tzinfo->abbr[abbr_offset] );
	}
		
}

static void print_tzinfo_activex(NF_TZINFO *tzinfo)
{
	struct tm      tm;
	int i;
	char utc_time[32], local_time[32];

	TZINFO_ACTIVEX  tzatx[MAX_TZINFO_ACTIVEX_CNT];
	gint    cnt = 0;

	g_return_if_fail(tzinfo);

	memset( tzatx, 0x00, sizeof(tzatx) );

	g_message("zone_name  [%s][%s]", tzinfo->zone_name,tzinfo->abbr);
	g_message("timecnt    [%d]", tzinfo->timecnt);

	for (i = 0; i < tzinfo->timecnt; ++i)
	{
		int type_idx = tzinfo->type_arr[i];
		int abbr_offset = tzinfo->type[type_idx].abbr_idx;
		time_t  local = tzinfo->time_arr[i] + tzinfo->type[type_idx].offset;

		gmtime_r(&tzinfo->time_arr[i], &tm);
		asctime_r(&tm, utc_time);
		utc_time[strlen(utc_time)-1] = 0;

		gmtime_r(&local , &tm);
		asctime_r(&tm, local_time);
		local_time[strlen(local_time)-1] = 0;

		if( tzinfo->time_arr[i] > NF_DATETIME_MIN ) {

			if( tzatx[cnt].year != 0 && tzatx[cnt].year != (tm.tm_year+1900) ) {
				++cnt;
				if( cnt >= MAX_TZINFO_ACTIVEX_CNT) break;
			}

			tzatx[cnt].year = tm.tm_year+1900;
			if(tzinfo->type[type_idx].is_dst)  // start 
			{
				tzatx[cnt].flag |= 0x01;
				tzatx[cnt].start_utc_time = tzinfo->time_arr[i];
				tzatx[cnt].start_offset = tzinfo->type[type_idx].offset;
				tzatx[cnt].start_tm = tm;

				strncpy( tzatx[cnt].start_asctime, local_time,
						sizeof(tzatx[cnt].start_asctime)-1 );
				strncpy( tzatx[cnt].start_abbr, &tzinfo->abbr[abbr_offset] ,
						sizeof(tzatx[cnt].start_abbr)-1 );
			}else{ // end

				tzatx[cnt].flag |= 0x02;
				tzatx[cnt].end_utc_time = tzinfo->time_arr[i];
				tzatx[cnt].end_offset = tzinfo->type[type_idx].offset;
				tzatx[cnt].end_tm = tm;

				strncpy( tzatx[cnt].end_asctime, local_time,
						sizeof(tzatx[cnt].end_asctime)-1 );
				strncpy( tzatx[cnt].end_abbr, &tzinfo->abbr[abbr_offset] ,
						sizeof(tzatx[cnt].end_abbr)-1 );

			}

			if( tzatx[cnt].flag == 0x3 ) {
				gint start_sign= (tzatx[cnt].start_offset>0);
				gint end_sign = (tzatx[cnt].end_offset>0);

				gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;
				gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

				g_message("%04d %s%c%02d:%02d:%02d %s%c%02d:%02d:%02d J1%d/%02d:%02d:%02d, J1%d/%02d:%02d:%02d [%s] [%s]",
						tzatx[cnt].year,

						tzatx[cnt].end_abbr, end_sign ? '-':'+',
						end_offset/3600, (end_offset%3600)/60, end_offset%60,

						tzatx[cnt].start_abbr, start_sign ? '-':'+',
						start_offset/3600, (start_offset%3600)/60, start_offset%60,

						(tzatx[cnt].start_tm.tm_yday+1),
						tzatx[cnt].start_tm.tm_hour, tzatx[cnt].start_tm.tm_min, tzatx[cnt].start_tm.tm_sec,

						(tzatx[cnt].end_tm.tm_yday+1),
						tzatx[cnt].end_tm.tm_hour, tzatx[cnt].end_tm.tm_min, tzatx[cnt].end_tm.tm_sec,

						tzatx[cnt].start_asctime, tzatx[cnt].end_asctime
						);
			}else if (tzatx[cnt].year == 2038 && tzatx[cnt].flag == 1) {
				gint start_sign= (tzatx[cnt].start_offset>0);
				gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;
				g_message("%04d %s%c%02d:%02d:%02d [%s]",
						tzatx[cnt].year,
						tzatx[cnt].start_abbr, start_sign ? '-':'+',
						start_offset/3600, (start_offset%3600)/60, start_offset%60,

						tzatx[cnt].start_asctime
						);

			}else if (tzatx[cnt].year == 2038 && tzatx[cnt].flag == 2) {
				gint end_sign = (tzatx[cnt].end_offset>0);
				gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

				g_message("%04d %s%c%02d:%02d:%02d [%s]",
						tzatx[cnt].year,
						tzatx[cnt].end_abbr, end_sign ? '-':'+',
						end_offset/3600, (end_offset%3600)/60, end_offset%60,

						tzatx[cnt].end_asctime
						);
			}
		}
	}
}





static gboolean
load_tzinfo(const char*  filename, NF_TZINFO *ret_tzinfo)
{
	FILE *	  fp = NULL;

	struct tzhead   tzh;
	NF_TZINFO		tzinfo;
	
	long long	ll;
	char		c;
	int			i, pass = 1;
	int			ret = 0;	
	char		buf[32];
	
	g_return_val_if_fail( filename != NULL, 0);
	g_return_val_if_fail( ret_tzinfo != NULL, 0);

	fp = fopen(filename, "r");	
	g_return_val_if_fail( fp != NULL, 0);

	if(fread(&tzh, sizeof(tzh), 1, fp) != 1)
	{		
		ret = -1; goto tz_error;		
	}		
	
	if( memcmp(tzh.tzh_magic, TZ_MAGIC, 4) != 0 )
	{
		ret = -2; goto tz_error;			
	}
	
	memset( &tzinfo, 0x00, sizeof(NF_TZINFO));
	
	tzinfo.version		= tzh.tzh_version[0];
			
	tzinfo.ttisgmtcnt	= (int)detzcode(tzh.tzh_ttisgmtcnt	);
	tzinfo.ttisstdcnt	= (int)detzcode(tzh.tzh_ttisstdcnt	);
	tzinfo.leapcnt		= (int)detzcode(tzh.tzh_leapcnt		);
	tzinfo.timecnt		= (int)detzcode(tzh.tzh_timecnt		);
	tzinfo.typecnt		= (int)detzcode(tzh.tzh_typecnt		);
	tzinfo.charcnt		= (int)detzcode(tzh.tzh_charcnt		);	

#ifdef DEBUG_TZINFO_LOGxx
	g_message("%s ttisgmtcnt	[%d]", __FUNCTION__, tzinfo.ttisgmtcnt	);
	g_message("%s ttisstdcnt	[%d]", __FUNCTION__, tzinfo.ttisstdcnt	);
	g_message("%s leapcnt		[%d]", __FUNCTION__, tzinfo.leapcnt		);
	g_message("%s timecnt		[%d]", __FUNCTION__, tzinfo.timecnt		);
	g_message("%s typecnt		[%d]", __FUNCTION__, tzinfo.typecnt		);
	g_message("%s charcnt		[%d]", __FUNCTION__, tzinfo.charcnt		);	
#endif

	if( tzinfo.ttisgmtcnt > TZ_MAX_ARR 
			|| tzinfo.ttisstdcnt > TZ_MAX_ARR 
			|| tzinfo.leapcnt	> TZ_MAX_ARR
			|| tzinfo.timecnt	> TZ_MAX_TIMES	
			|| tzinfo.typecnt	> TZ_MAX_TYPES	
			|| tzinfo.charcnt	> TZ_MAX_CHARS ){
		ret = -3; goto tz_error;
	}
	
	for (i = 0; i < tzinfo.timecnt; ++i) {						
		if (fread(buf, pass * 4, 1, fp) != 1){
			ret = -4; goto tz_error;	
		}else{
			ll = detzcode(buf);
			tzinfo.time_arr[i] = (int)ll;
		}						
	}
	
	for (i = 0; i < tzinfo.timecnt; ++i) {						
		if (fread(buf, 1, 1, fp) != 1){
			ret = -5; goto tz_error;	
		}else{
			tzinfo.type_arr[i] = buf[0];
		}						
	}	
	for (i = 0; i < tzinfo.typecnt; ++i) {						
		if (fread(buf, 6, 1, fp) != 1){
			ret = -6; goto tz_error;	
		}else{
			tzinfo.type[i].offset = (int)detzcode(buf);
			tzinfo.type[i].is_dst = buf[4];
			tzinfo.type[i].abbr_idx = buf[5];
		}						
	}

{
	int offset, type_idx;	

	type_idx = tzinfo.type_arr[tzinfo.timecnt] = tzinfo.timecnt ? tzinfo.type_arr[tzinfo.timecnt-1]:0;	
	offset = tzinfo.type[type_idx ].offset;
		
	if(offset<0)
		tzinfo.time_arr[tzinfo.timecnt] = 0x7fffffff+offset;		
	else
		tzinfo.time_arr[tzinfo.timecnt] = 0x7fffffff-offset;					
	++tzinfo.timecnt;	
}

	if (fread(tzinfo.abbr, tzinfo.charcnt, 1, fp) != 1){
		ret = -7; goto tz_error;
	}
	if (fread(tzinfo.tisstd, tzinfo.ttisstdcnt, 1, fp) != 1){
		ret = -8; goto tz_error;
	}
	if (fread(tzinfo.tisgmt, tzinfo.ttisgmtcnt, 1, fp) != 1){
		ret = -9; goto tz_error;
	}

	strncpy( tzinfo.zone_name, filename, sizeof(tzinfo.zone_name) -1);
	
	memcpy( ret_tzinfo, &tzinfo, sizeof(NF_TZINFO));
	
	fclose(fp);	
	return 1;
	
tz_error:
	g_warning("%s failed[%d]", __FUNCTION__, ret);
	
	if(fp)
		fclose(fp);	
		
	return 0;			
}

static gboolean
load_tzinfo_activex(NF_TZINFO *tzinfo, NF_TZINFO_ACTIVEX *tzout, int bplayer)
{
	struct tm      tm;
	int i;
	char utc_time[32], local_time[32];

	TZINFO_ACTIVEX  tzatx[MAX_TZINFO_ACTIVEX_CNT];
	gint    cnt = 0;

	g_return_if_fail(tzinfo);

	memset( tzatx, 0x00, sizeof(tzatx) );

	for (i = 0; i < tzinfo->timecnt; ++i)
	{
		int type_idx = tzinfo->type_arr[i];
		int abbr_offset = tzinfo->type[type_idx].abbr_idx;
		time_t  local = tzinfo->time_arr[i] + tzinfo->type[type_idx].offset;

		gmtime_r(&tzinfo->time_arr[i], &tm);
		asctime_r(&tm, utc_time);
		utc_time[strlen(utc_time)-1] = 0;

		gmtime_r(&local , &tm);
		asctime_r(&tm, local_time);
		local_time[strlen(local_time)-1] = 0;

		if( tzinfo->time_arr[i] > NF_DATETIME_MIN ) {

			if( tzatx[cnt].year != 0 && tzatx[cnt].year != (tm.tm_year+1900) ) {
				++cnt;
				if( cnt >= MAX_TZINFO_ACTIVEX_CNT) break;
			}

			tzatx[cnt].year = tm.tm_year+1900;
			if(tzinfo->type[type_idx].is_dst)  // start 
			{
				tzatx[cnt].flag |= 0x01;
				tzatx[cnt].start_utc_time = tzinfo->time_arr[i];
				tzatx[cnt].start_offset = tzinfo->type[type_idx].offset;
				tzatx[cnt].start_tm = tm;

				strncpy( tzatx[cnt].start_asctime, local_time,
						sizeof(tzatx[cnt].start_asctime)-1 );

				if( strncmp( &tzinfo->abbr[abbr_offset], "GMT", 3) != 0)
					strncpy( tzatx[cnt].start_abbr, &tzinfo->abbr[abbr_offset] ,
							sizeof(tzatx[cnt].start_abbr)-1 );
				else
					strcpy( tzatx[cnt].start_abbr, "GMT");

			}else{ // end

				tzatx[cnt].flag |= 0x02;
				tzatx[cnt].end_utc_time = tzinfo->time_arr[i];
				tzatx[cnt].end_offset = tzinfo->type[type_idx].offset;
				tzatx[cnt].end_tm = tm;

				strncpy( tzatx[cnt].end_asctime, local_time,
						sizeof(tzatx[cnt].end_asctime)-1 );
				if( strncmp( &tzinfo->abbr[abbr_offset], "GMT", 3) != 0)
					strncpy( tzatx[cnt].end_abbr, &tzinfo->abbr[abbr_offset] ,
							sizeof(tzatx[cnt].end_abbr)-1 );
				else
					strcpy( tzatx[cnt].end_abbr, "GMT");

			}

			if( tzatx[cnt].flag == 0x3 ) {
				gint start_sign= (tzatx[cnt].start_offset>0);
				gint end_sign = (tzatx[cnt].end_offset>0);

				gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;
				gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

				if ( bplayer )
				{
//					("AZOT+01:00:00AZOST+00:00:00J190/01:00:00,J1300/00:00:00"),
					sprintf(tzatx[cnt].string, "\t\t\t\t\t\t\t\t\t\t\t\t(\"%s%c%02d:%02d:%02d%s%c%02d:%02d:%02dJ1%d/%02d:%02d:%02d,J1%d/%02d:%02d:%02d\"),",
							tzatx[cnt].end_abbr, end_sign ? '-':'+',
							end_offset/3600, (end_offset%3600)/60, end_offset%60,

							tzatx[cnt].start_abbr, start_sign ? '-':'+',
							start_offset/3600, (start_offset%3600)/60, start_offset%60,

							(tzatx[cnt].start_tm.tm_yday+1),
							tzatx[cnt].start_tm.tm_hour, tzatx[cnt].start_tm.tm_min, tzatx[cnt].start_tm.tm_sec,

							(tzatx[cnt].end_tm.tm_yday+1),
							tzatx[cnt].end_tm.tm_hour, tzatx[cnt].end_tm.tm_min, tzatx[cnt].end_tm.tm_sec,

							tzatx[cnt].start_asctime, tzatx[cnt].end_asctime
						   );
				}
				else
				{
//					AZOT+01:00:00,AZOST+00:00:00,J190/01:00:00,J1300/00:00:00
					sprintf(tzatx[cnt].string, "%s%c%02d:%02d:%02d,%s%c%02d:%02d:%02d,J1%d/%02d:%02d:%02d,J1%d/%02d:%02d:%02d",
							tzatx[cnt].end_abbr, end_sign ? '-':'+',
							end_offset/3600, (end_offset%3600)/60, end_offset%60,

							tzatx[cnt].start_abbr, start_sign ? '-':'+',
							start_offset/3600, (start_offset%3600)/60, start_offset%60,

							(tzatx[cnt].start_tm.tm_yday+1),
							tzatx[cnt].start_tm.tm_hour, tzatx[cnt].start_tm.tm_min, tzatx[cnt].start_tm.tm_sec,

							(tzatx[cnt].end_tm.tm_yday+1),
							tzatx[cnt].end_tm.tm_hour, tzatx[cnt].end_tm.tm_min, tzatx[cnt].end_tm.tm_sec,

							tzatx[cnt].start_asctime, tzatx[cnt].end_asctime
						   );
				}
				
			}else if (/* tzatx[cnt].year == 2038 && */ tzatx[cnt].flag == 1) {
				gint start_sign= (tzatx[cnt].start_offset>0);
				gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;

				if ( bplayer )
				{
					sprintf(tzatx[cnt].string, "(\"%s%c%02d:%02d:%02d\"),",
							tzatx[cnt].start_abbr, start_sign ? '-':'+',
							start_offset/3600, (start_offset%3600)/60, start_offset%60
						   );
				}
				else
				{
					sprintf(tzatx[cnt].string, "%s%c%02d:%02d:%02d",
							tzatx[cnt].start_abbr, start_sign ? '-':'+',
							start_offset/3600, (start_offset%3600)/60, start_offset%60
						   );
				}
				
			}else if (/* tzatx[cnt].year == 2038 && */ tzatx[cnt].flag == 2) {
				gint end_sign = (tzatx[cnt].end_offset>0);
				gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

				if ( bplayer )
				{
					sprintf(tzatx[cnt].string, "\t\t\t\t\t\t\t\t\t\t\t\t(\"%s%c%02d:%02d:%02d\"),",
							tzatx[cnt].end_abbr, end_sign ? '-':'+',
							end_offset/3600, (end_offset%3600)/60, end_offset%60
						   );
				}
				else
				{
					sprintf(tzatx[cnt].string, "%s%c%02d:%02d:%02d",
							tzatx[cnt].end_abbr, end_sign ? '-':'+',
							end_offset/3600, (end_offset%3600)/60, end_offset%60
						   );
				}
			}
		}
	}

	tzout->tz_cnt = cnt+1;
	memcpy( tzout->tz_arr, tzatx, sizeof(TZINFO_ACTIVEX)*tzout->tz_cnt);

	return 1;
}


/******************************************************************************/

static char* _tzname_convert( const gchar *tzname )
{
	int i =0;
	int len = strnlen(tzname, 64);
	
	for(i=0;i<NUM_TZINFO;++i)
	{
		if( strncmp( _zoneinfo_str[i], tzname, len ) == 0)
		{
			return _zoneinfo_str_tz[i];
		}
	}	
	return "NULL";
}

gboolean nf_zoneinfo_get( const gchar *tzname, time_t utc_time, 
							gint *offset, 
							gboolean *is_dst)
{
	static NF_TZINFO tzinfo;
	
	gchar	zone_name[256];
	int		i,idx;
	gboolean ret;
	
	g_return_val_if_fail( tzname != NULL, 0);
	g_return_val_if_fail( offset != NULL, 0);
	g_return_val_if_fail( is_dst != NULL, 0);

	if( strncmp( tzinfo.zone_name, tzname, sizeof(zone_name) ) != 0 )
	{	
		snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, _tzname_convert(tzname) );	
		ret = load_tzinfo(zone_name, &tzinfo);		

		g_return_val_if_fail( ret == 1, 0);	
		strncpy( tzinfo.zone_name, tzname, sizeof(tzinfo.zone_name) -1);
	}
	
	if(tzinfo.timecnt > 0)
	{
		int not_found = 1;		
		
		for( i=tzinfo.timecnt-1; i >= 0; --i)
		{
	
#ifdef DEBUG_TZINFO_LOGxx
			g_message("%s [%3d][%13d] utc_time[%13d]", __FUNCTION__, 
							i, tzinfo.time_arr[i], utc_time);
#endif			
			if(	tzinfo.time_arr[i] <= utc_time)
			{
				not_found = 0;
				break;
			}
		}
				
		if(not_found)
			i = 0;					
	}else{
		i = 0;
	}
	
	idx = tzinfo.type_arr[i];	
	*offset = tzinfo.type[idx].offset;
	*is_dst = tzinfo.type[idx].is_dst;

#ifdef DEBUG_TZINFO_LOGxx
	g_message("%s tzname[%s] [%13d] offset[%d] is_dst[%d]", __FUNCTION__, 
					tzname, utc_time, *offset, *is_dst);
#endif			
						
	return 1;
}

gboolean nf_zoneinfo_get_activex( const gchar *tzname, NF_TZINFO_ACTIVEX *tzout)
{
	static NF_TZINFO tzinfo;
	gchar   zone_name[256];
	int     i,idx;
	gboolean ret;

	g_return_val_if_fail( tzname != NULL, 0);
	g_return_val_if_fail( tzout != NULL, 0);

	if( strncmp( tzinfo.zone_name, tzname, sizeof(zone_name) ) != 0 )
	{
		snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, _tzname_convert(tzname) );
		ret = load_tzinfo(zone_name, &tzinfo);

		g_return_val_if_fail( ret == 1, 0);
		strncpy( tzinfo.zone_name, tzname, sizeof(tzinfo.zone_name) -1);
	}

	load_tzinfo_activex(&tzinfo, tzout, 0);

	return 1;
}

#ifdef DEBUG_TZINFO_JBSHELL

static char tzinfo_help[] = "tzinfo [zonename] [utc_time]";
static int tzinfo(int argc, char **argv)
{	
	gchar zone_name[256];
	NF_TZINFO tzinfo;
	
	if(argc < 2){
		printf("%s\n",tzinfo_help);
		return -1;
	}
		
	snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, argv[1]);		
	
	if( load_tzinfo(zone_name, &tzinfo) == 1)
	{
		print_tzinfo(&tzinfo);
	}
	
	if(argc > 2){
		
		gboolean ret = 0, is_dst = 0;
		int offset = 0;
		char *zonename = NULL;
		int	utc_time = 0;
		
		zonename = argv[1];	
		utc_time = strtoul(argv[2],NULL,0);
		
		ret = nf_zoneinfo_get( zonename, utc_time, &offset, &is_dst);
		printf("ret[%d] zone[%s] utc[%13d] offset[%d]is_dst[%d]\n",
					ret, zonename, utc_time, offset, is_dst);		
	}
	return 0;
}
__commandlist(tzinfo, "tzinfo",  tzinfo_help, tzinfo_help);

static char tztest_help[] = "tztest [zonename] [utc_time]";
static int tztest(int argc, char **argv)
{		
	char *zonename = NULL;
	int	utc_time = 0;
	gboolean ret = 0, is_dst = 0;
	int offset = 0;
	
	if(argc < 3){
		printf("%s\n",tztest_help);
		return -1;
	}
	
	zonename = argv[1];	
	utc_time = strtoul(argv[2],NULL,0);
	
	ret = nf_zoneinfo_get( zonename, utc_time, &offset, &is_dst);
	printf("ret[%d] zone[%s] utc[%13d] offset[%d]is_dst[%d]\n",
				ret, zonename, utc_time, offset, is_dst);
					
	return 0;
}
__commandlist(tztest, "tztest",  tztest_help, tztest_help);



static char tzdump_help[] = "tzdump [is_simple:1]";
static int tzdump(int argc, char **argv)
{		
	char zone_name[256];
	int i;
	int is_simple = 1;
	NF_TZINFO tzinfo;


	if( argc>2)
		is_simple = strtoul(argv[2],NULL,0);
								
	for(i=0;i<NUM_TZINFO; ++i)
	{
		g_print("\n\n%s %s===============\n", _zoneinfo_str[i], _zoneinfo_str_tz[i] );

		snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, _zoneinfo_str_tz[i]);
		if( load_tzinfo(zone_name, &tzinfo) == 1)
		{
			if( is_simple )
				print_tzinfo_simple(&tzinfo);
			else
				print_tzinfo(&tzinfo);
		}		
	}	
	return 0;
}
__commandlist(tzdump, "tzdump",  tzdump_help, tzdump_help);


//File for Backup Player timezone data
static char tzdump2_help[] = "tzdump2 [is_file:1]";
static int tzdump2(int argc, char **argv)
{
	char zone_name[256];
	int i, j, k;
	int is_file = 1;
	NF_TZINFO tzinfo;
	NF_TZINFO_ACTIVEX   tzout;

	char test_s[1024];
	FILE *tzdata = NULL;

	if( argc>1)
		is_file = strtoul(argv[1],NULL,0);

	if ( is_file )
	{
		tzdata = fopen("tzdata.txt", "w+");
	}
	
	for(i=0;i<NUM_TZINFO; ++i)
	{
		if ( is_file )
		{
			memset(test_s, 0x0, sizeof(test_s));
			sprintf(test_s, "(\"%s\"),  {     ", _zoneinfo_str_tz[i] );
			fwrite(test_s, 1, strlen(test_s), tzdata);
		}
		else
		{
			g_print("\n\n%s %s===============\n", _zoneinfo_str[i], _zoneinfo_str_tz[i] );
		}

		snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, _zoneinfo_str_tz[i]);
		if( load_tzinfo(zone_name, &tzinfo) == 1)
		{
			memset( &tzout, 0x00, sizeof(tzout) );

			//is_file for Backup Player timezone data
			load_tzinfo_activex(&tzinfo, &tzout, is_file);

			for(k=0, j=2008;j<2038; ++j)
			{
retry:
				if( j > tzout.tz_arr[k].year )
				{
					++k;
					if( k < tzout.tz_cnt ) goto retry;
					else break;
				}

				if ( is_file )
				{
					memset(test_s, 0x0, sizeof(test_s));
					sprintf(test_s, "%s\n", tzout.tz_arr[k].string );
					fwrite(test_s, 1, strlen(test_s), tzdata);
				}
				else
				{
					g_print("%d %s\n", j , tzout.tz_arr[k].string );
				}
			}
		}
	}
	
	if ( is_file )
	{
		if (tzdata)
			fclose(tzdata);
	}
	
	return 0;
}
__commandlist(tzdump2, "tzdump2",  tzdump2_help, tzdump2_help);

#endif

#if 0
g_get_current_time 
gettimeofday(&pqitem->ret_timeval, NULL);


<item key="sys.date.daylight"			type="BOOL" 	min="" max="" val="" />
<item key="sys.date.timeform"			type="UINT" 	min="" max="" val="" />
<item key="sys.date.dateform"			type="UINT" 	min="" max="" val="" />
<item key="sys.date.tz_index"			type="UINT" 	min="" max="" val="" />
<item key="sys.date.ltime"				type="UINT" 	min="" max="" val="" />
<item key="sys.date.timesvr"			type="STRING" 	min="" max="" val="" />

http://nf.intellix.co.kr/phpBB/viewtopic.php?t=350&highlight=	
#endif

static void prvStTimeSubtractHMS(struct tm *stTime, gint hour, gint min, gint sec)
{
	gint day=0;

	if(stTime == NULL)	return;

	if(sec)
	{
		if(stTime->tm_sec >= sec)
			stTime->tm_sec -= sec;
		else
		{
			stTime->tm_sec -= (sec-60);
			min++;
		}
	}

	if(min)
	{
		if(stTime->tm_min >= min)
			stTime->tm_min -= min;
		else
		{
			stTime->tm_min -= (min-60);
			hour++;
		}
	}

	if(hour)
	{
		if(stTime->tm_hour >= hour)
			stTime->tm_hour -= hour;
		else
		{
			stTime->tm_hour -= (hour-24);
			day++;
		}
	}

	if(day)
	{
		GDate *date = NULL;

		date = g_date_new();

		g_date_set_dmy(date, stTime->tm_mday, stTime->tm_mon+1, stTime->tm_year+1900);

		g_date_subtract_days(date, day);

		stTime->tm_year = g_date_get_year(date) - 1900;
		stTime->tm_mon = g_date_get_month(date) - 1;
		stTime->tm_mday = g_date_get_day(date);

		g_date_free(date);
		date = NULL;
	}
}

void nf_datetime_localtime(time_t *ttime, gboolean is_dst, struct tm *stTime)
{
	g_return_if_fail(ttime);
	g_return_if_fail(stTime);

    localtime_r(ttime, stTime);

//	g_message("<%s,%d> hour[%d], min[%d]", __FUNCTION__, __LINE__, stTime->tm_hour, stTime->tm_min);

	if(stTime->tm_isdst)
	{
		struct tm stTime_temp;
		time_t ttime_temp = 0;
		time_t real_time = 0;

		if(!is_dst)
		{
			time_t time_gab=0;
			gint hour=0, min=0, sec=0, temp=0;

			ttime_temp = *ttime;
			stTime_temp = *stTime;

			stTime_temp.tm_isdst = 0;

			real_time = mktime(&stTime_temp);

			time_gab = real_time - ttime_temp;

			hour = time_gab / 3600;
			temp = time_gab % 3600;

			if(temp)
			{
				min = temp / 60;
				temp %= 60;

				sec = temp;
			}

			prvStTimeSubtractHMS(stTime, hour, min, sec);
			stTime->tm_isdst = 0;
		}
	}
	
//	g_message("<%s,%d> hour[%d], min[%d]", __FUNCTION__, __LINE__, stTime->tm_hour, stTime->tm_min);
}

#if 0
g_get_current_time 
gettimeofday(&pqitem->ret_timeval, NULL);


<item key="sys.date.daylight"           type="BOOL"     min="" max="" val="" />
<item key="sys.date.timeform"           type="UINT"     min="" max="" val="" />
<item key="sys.date.dateform"           type="UINT"     min="" max="" val="" />
<item key="sys.date.tz_index"           type="UINT"     min="" max="" val="" />
<item key="sys.date.ltime"              type="UINT"     min="" max="" val="" />
<item key="sys.date.timesvr"            type="STRING"   min="" max="" val="" />

http://nf.intellix.co.kr/phpBB/viewtopic.php?t=350&highlight=   


man zic ( 

		On

		5        the fifth of the month
		lastSun  the last Sunday in the month
		lastMon  the last Monday in the month
		Sun>=8   first Sunday on or after the eighth
		Sun<=25  last Sunday on or before the 25th


# Rule  NAME    FROM    TO      TYPE    IN      ON      AT      SAVE    LETTER/S
		Rule    ROK     1960    only    -       May     15      0:00    1:00    D
		Rule    ROK     1960    only    -       Sep     13      0:00    0       S   
		Rule    ROK     1987    1988    -       May     Sun>=8  0:00    1:00    D
		Rule    ROK     1987    1988    -       Oct     Sun>=8  0:00    0       S

# Zone  NAME            GMTOFF  RULES   FORMAT  [UNTIL]
		Zone    test_tz         8:27:52 -       LMT     1890
		8:30    -       KST     1904 Dec
		9:00    -       KST     1928
8:30    -       KST     1932
9:00    -       KST     1954 Mar 21
8:00    ROK     K%sT    1961 Aug 10
8:30    -       KST     1968 Oct
9:00    ROK     K%sT

./zic  -d tztmp -v test_tz

#endif


static gboolean load_tzinfo_activex_posix_onvif(NF_TZINFO *tzinfo, NF_TZINFO_ACTIVEX *tzout )
{
    struct tm      tm;
    int i;
    char utc_time[32], local_time[32];

    TZINFO_ACTIVEX  tzatx[MAX_TZINFO_ACTIVEX_CNT];
    gint    cnt = 0;

    g_return_if_fail(tzinfo);

    memset( tzatx, 0x00, sizeof(tzatx) );

    //g_message("zone_name  [%s][%s]", tzinfo->zone_name,tzinfo->abbr);
    //g_message("timecnt    [%d]", tzinfo->timecnt);

    for (i = 0; i < tzinfo->timecnt; ++i)
    {
        int type_idx = tzinfo->type_arr[i];
        int abbr_offset = tzinfo->type[type_idx].abbr_idx;
        time_t  local = tzinfo->time_arr[i] + tzinfo->type[type_idx].offset;

        gmtime_r((time_t *)&tzinfo->time_arr[i], &tm);
        asctime_r(&tm, utc_time);
        utc_time[strlen(utc_time)-1] = 0;

        gmtime_r((time_t *)&local , &tm);
        asctime_r(&tm, local_time);
        local_time[strlen(local_time)-1] = 0;

        if( tzinfo->time_arr[i] > NF_DATETIME_MIN ) {

            if( tzatx[cnt].year != 0 && tzatx[cnt].year != (tm.tm_year+1900) ) {
                ++cnt;
                if( cnt >= MAX_TZINFO_ACTIVEX_CNT) break;
            }

            tzatx[cnt].year = tm.tm_year+1900;
            if(tzinfo->type[type_idx].is_dst)  // start
            {
                tzatx[cnt].flag |= 0x01;
                tzatx[cnt].start_utc_time = tzinfo->time_arr[i];
                tzatx[cnt].start_offset = tzinfo->type[type_idx].offset;
                tzatx[cnt].start_tm = tm;

                strncpy( tzatx[cnt].start_asctime, local_time,
                        sizeof(tzatx[cnt].start_asctime)-1 );

                if( strncmp( &tzinfo->abbr[abbr_offset], "GMT", 3) != 0)
                    strncpy( tzatx[cnt].start_abbr, &tzinfo->abbr[abbr_offset] ,
                            sizeof(tzatx[cnt].start_abbr)-1 );
                else
                    strcpy( tzatx[cnt].start_abbr, "GMT");

            }else{ // end
                tzatx[cnt].flag |= 0x02;
                tzatx[cnt].end_utc_time = tzinfo->time_arr[i];
                tzatx[cnt].end_offset = tzinfo->type[type_idx].offset;
                tzatx[cnt].end_tm = tm;

                strncpy( tzatx[cnt].end_asctime, local_time,
                        sizeof(tzatx[cnt].end_asctime)-1 );
                if( strncmp( &tzinfo->abbr[abbr_offset], "GMT", 3) != 0)
                    strncpy( tzatx[cnt].end_abbr, &tzinfo->abbr[abbr_offset] ,
                            sizeof(tzatx[cnt].end_abbr)-1 );
                else
                    strcpy( tzatx[cnt].end_abbr, "GMT");
            }

            if( tzatx[cnt].flag == 0x3 ) {
                gint start_sign= (tzatx[cnt].start_offset>0);
                gint end_sign = (tzatx[cnt].end_offset>0);

                gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;
                gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

                // ("IRST-03:30:00IRDT-04:30:00J181/00:00:00,J1265/00:00:00"),
                sprintf(tzatx[cnt].string, "%s%c%02d%s,J%d/%02d:%02d:%02d,J%d/%02d:%02d:%02d",
                        tzatx[cnt].end_abbr, end_sign ? '-':'+',
                        end_offset/3600,

                        tzatx[cnt].start_abbr,

                        (tzatx[cnt].start_tm.tm_yday+1),
                        tzatx[cnt].start_tm.tm_hour, tzatx[cnt].start_tm.tm_min, tzatx[cnt].start_tm.tm_sec,

                        (tzatx[cnt].end_tm.tm_yday+1),
                        tzatx[cnt].end_tm.tm_hour, tzatx[cnt].end_tm.tm_min, tzatx[cnt].end_tm.tm_sec

                        //tzatx[cnt].start_asctime, tzatx[cnt].end_asctime
                       );
            }else if (/* tzatx[cnt].year == 2038 && */ tzatx[cnt].flag == 1) {

                gint start_sign= (tzatx[cnt].start_offset>0);
                gint start_offset = start_sign ? tzatx[cnt].start_offset : tzatx[cnt].start_offset * -1;

                // ("IRST-03:30:00"),
                sprintf(tzatx[cnt].string, "%s%c%02d:%02d:%02d",
                        tzatx[cnt].start_abbr, start_sign ? '-':'+',
                        start_offset/3600, (start_offset%3600)/60, start_offset%60
                       );

            }else if (/* tzatx[cnt].year == 2038 && */ tzatx[cnt].flag == 2) {

                gint end_sign = (tzatx[cnt].end_offset>0);
                gint end_offset = end_sign ? tzatx[cnt].end_offset : tzatx[cnt].end_offset * -1;

                // ("IRST-03:30:00"),
                sprintf(tzatx[cnt].string, "%s%c%02d:%02d:%02d",
                        tzatx[cnt].end_abbr, end_sign ? '-':'+',
                        end_offset/3600, (end_offset%3600)/60, end_offset%60
                       );
            }

        }
    }

    tzout->tz_cnt = cnt+1;
    memcpy( tzout->tz_arr, tzatx, sizeof(TZINFO_ACTIVEX)*tzout->tz_cnt);

    return 1;
}

gboolean nf_zoneinfo_get_activex_posix_onvif( const gchar *tzname, NF_TZINFO_ACTIVEX *tzout)
{
    static NF_TZINFO tzinfo;
    gchar   zone_name[256];
    gboolean ret;

    g_return_val_if_fail( tzname != NULL, 0);
    g_return_val_if_fail( tzout != NULL, 0);

    if( strncmp( tzinfo.zone_name, tzname, sizeof(zone_name) ) != 0 ) {
        snprintf( zone_name, sizeof(zone_name)-1,"%s%s", NF_ZONEINFO_PATH, _tzname_convert(tzname) );
        ret = load_tzinfo(zone_name, &tzinfo);

        g_return_val_if_fail( ret == 1, 0);
        strncpy( tzinfo.zone_name, tzname, sizeof(tzinfo.zone_name) -1);
    }

    load_tzinfo_activex_posix_onvif(&tzinfo, tzout);

    return 1;
}


