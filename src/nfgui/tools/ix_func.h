/*
 * ix_func.h
 * 	- utility functions
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#ifndef __IX_FUNC_H
#define	__IX_FUNC_H

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>


/*
 * return
 * 		1: filter passed
 * 		0: not filter passed
 */ 		
typedef int (*FILTER_PROC)(char *file_name, void *param);

typedef struct _FNAME_FILTER_PARAM_T {
	char		 	ext[16];	// file name extension, including '.'
	int				condition;
	unsigned int	opt;
} FNAME_FILTER_PARAM_T;

typedef enum _FILE_OPT_E {
	IFO_NONE					= 0x0000,
	IFO_INCLUDE_FULLPATH		= 0x0001,
	IFO_RECURSIVE_SEARCH		= 0x0002,
} FILE_OPT_E;

typedef enum _FM_DATE_E {
	YYYYMMDD,
	YYMMDD,
	MMDDYY,
	MMDDYYYY,
	DDMMYYYY,
	YYMMDD_HOL,
} FM_DATE_E;

typedef enum _FM_TIME_E {
	H24,
	H12
} FM_TIME_E;

typedef struct _IXPOINT {
	int		x;
	int		y;
} IXPOINT;

typedef struct _IXPOLYGON {
	int			cnt;
	IXPOINT		pt[64];
} IXPOLYGON;

typedef struct _IXSQUARE{
	int x1;
	int y1;
	int x2;
	int y2;
} IXSQUARE;

typedef struct _IGRECT {
	int		x;
	int		y;
	int		w;
	int		h;
} IGRECT;

/////////////////////////////////////////////////////////
//



gboolean ifn_is_in_rect(GdkRectangle *rect, gdouble x, gdouble y);
gboolean ifn_is_in_igrect(IGRECT *rect, int x, int y);
int ifn_set_gdkcolor(int red, int green, int blue, GdkColor *color);
int ifn_save_drawable(GdkDrawable *drawable, GdkRectangle *rect, const char *fullpath);




///////////////////////////////////////////////////////
//
// 
//

int ifn_is_all_digit(const char *str);
int ifn_is_all_alnum(const char *str);
int ifn_is_all_itxstyle_title(const char *str);


///////////////////////////////////////////////////////
//
// 
//


///////////////////////////////////////////////////////
//
// 
//
// create not joinable thread
GThread *ifn_make_thread(void *(*proc)(void *), void *data);
int ifn_init_thread();

///////////////////////////////////////////////////////
//
// assert
//

int ifn_show_backtrace();
void iassert(int expr);


///////////////////////////////////////////////////////
//
// rand
//
int ifn_rand();
char *ifn_strtrim(char *str);
char *ifn_strrtrim(char *str);
char *ifn_strtrim_b(char *str, char *buf);
char *ifn_strrtrim_b(char *str, char *buf);
int ifn_tolower(char *str);
int ifn_toupper(char *str);
int ifn_count_char(char *str, char c);
char *ifn_strrnchar(char *p, char c, int n);
char *ifn_strnchar(char *p, char c, int n);
void ifn_trim_char(char *str, char c);


//////////////////////////
// m < - > yard
//

double ifn_unit_change(gdouble data,gint flag);
double ifn_unit_change_speed(gdouble data,gint flag);
double ifn_unit_change_3d(gdouble data, gint flag);
double ifn_unit_change_digit(gdouble data,guint digit);

///////////////////////////////////////////////////////
//
// 
//

char *ifn_find_file_name_in_path(char *full_path);
int ifn_make_pathname(char *dir, char *file, char *pathname);
char **ifn_new_filelist(char *dir, FILTER_PROC proc, FNAME_FILTER_PARAM_T *fparam, int *ret_cnt);
int ifn_free_filelist(char **name_buf);
int ifn_is_file_exist(const char *path);
int ifn_make_dir_p(const char *s);
int ifn_delete_file_in_dir(char *dir);
int ifn_remove_dir_rf(char *dir);
int ifn_mount_device(const char *device_name, const char *mnt_path, const gchar *fs);
int ifn_unmount_device(const char *device_name, const char *mnt_path);
int ifn_is_mounted_dev(char *dev_name);
char *ifn_find_token(char *string, char delimiter, int n, char *buf);



///////////////////////////////////////////////////////
//
// dates
//

int ifn_add_day(GTimeVal *time, int day);
int ifn_add_hour(GTimeVal *time, int hour);
int ifn_add_minute(GTimeVal *time, int minute);

int ifn_add_day_b(GTimeVal *time, int day, GTimeVal *ret);
int ifn_add_hour_b(GTimeVal *time, int hour, GTimeVal *ret);
int ifn_add_minute_b(GTimeVal *time, int minute, GTimeVal *ret);

int ifn_convert_date(struct tm *ttm, FM_DATE_E fm_date, char *buf_date);
int ifn_convert_time(struct tm *ttm, FM_TIME_E fm_time, char *buf_time);

int ifn_get_localtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf);
int ifn_get_local_hourmin_text(time_t time, FM_TIME_E fm_time, char *buf);
int ifn_get_local_day_text(time_t time, FM_DATE_E fm_date, char *buf);
int ifn_get_localtime_text_g(GTimeVal *time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf);
int ifn_get_gmtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf);
int ifn_get_gmtime_text_g(GTimeVal *time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf);
int ifn_get_local_hourmin(time_t timet, int *hour, int *min, int *sec);
int ifn_get_gmt_hourmin(time_t timet, int *hour, int *min, int *sec);
int ifn_get_local_day(time_t timet, int *year, int *mon, int *day);
int ifn_get_gmt_day(time_t timet, int *year, int *mon, int *day);
int ifn_get_gmt_datetime(time_t timet, int *year, int *mon, int *day, int *hour, int *min, int *sec);

time_t ifn_get_gmt_from_data_tml(int year, int mon, int day, int hour);
time_t ifn_get_gmt_from_date(int year, int mon, int day);
int ifn_timet_to_gtv(time_t timet, GTimeVal *ret);
time_t ifn_gtv_to_timet(GTimeVal *gtv);

time_t ifn_get_gmt_midnight(time_t time_sec);
time_t ifn_get_local_midnight(time_t time_sec);
time_t ifn_get_gmt_from_local(int year, int mon, int day, int hour, int min, int sec);
time_t ifn_get_local_timet();

int ifn_is_in_dst(time_t utc);
int ifn_get_hours_in_day(time_t day, char *hour_arr, int arr_cnt);
int ifn_is_same_day(time_t t1, time_t t2);
int ifn_has_dst_change(int year, int mon);
int ifn_get_days_in_month(int year, int mon);
int ifn_is_leap_year(int year);

time_t ifn_convert_guint64_to_timet(guint64 timeval);

int ifn_init_time_elap();
int ifn_show_time_elap(char *file, int line);
#define ifn_print_time_elap()	ifn_show_time_elap(__FILE__, __LINE__)

///////////////////////////////////////////////////////
//
// 
//

// le : little endian
int ifn_hex2btxt_le(unsigned int in, char buf[33]);

///////////////////////////////////////////////////////
//
// 
//

int ifn_convert_storage_size(gchar *buf, guint64 kb_size);
int ifn_point_is_closer_line(gint x, gint y, gint d, IXPOINT *pt1, IXPOINT *pt2);
int ifn_point_is_inside_polygon(gint x, gint y, gint d, IXPOLYGON *p);

int ifn_get_outbound_square(IXPOLYGON *p, IXSQUARE *square);
int ifn_get_area(IXSQUARE *square);


int ifn_is_leapyear(int year);
int ifn_get_monthdays(int year, int month);
int ifn_get_totaldays(int year, int month, int day);
int ifn_get_yoil(int year, int month, int day);
int ifn_is_valid_date(int year, int month, int day);
int ifn_get_week_of_month(int day);

#endif
