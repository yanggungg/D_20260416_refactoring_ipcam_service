/*
 * scm_holiday.c
 * 	- scm holiday service
 *	- dependencies :
 *		
 *
 * Written by etazeus
 * Copyright (c) ITX security, Jun 27, 2018
 *
 */

#include "iux_afx.h"
#include "scm_internal.h"
#include "vfs.h"
#include "nfdal.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_HOLI"
#define ONEMIN			(1000 * 60)
#define ONEDAY			(86400)

#define HOLI_LOCK()		g_mutex_lock(ihd.mtx)
#define HOLI_UNLOCK()	g_mutex_unlock(ihd.mtx)

////////////////////////////////////////////////////////////
//
// protected variable
//

typedef struct _HOLI {
	UsrDefHolidayData 	holi[USR_DEF_HOLIDAY_CNT];
	int 				cnt;
	int					enforce;
	GMutex				*mtx;
} HOLI;

static HOLI ihd;

////////////////////////////////////////////////////////////
//
// private functions
//

static int _is_older_day(int year, int month, int day, UsrDefHolidayData *hday)
{
	int x = ifn_get_totaldays(year, month, day);
	int y = ifn_get_totaldays(hday->year, hday->month, hday->day);

	return (x < y);
}

static int _check_same_date(int year, int month, int day)
{
	int i;

	HOLI_LOCK();
	for (i = 0; i < ihd.cnt; ++i) {
		if ((ihd.holi[i].year == year) && (ihd.holi[i].month == month) && (ihd.holi[i].day == day)) {
			HOLI_UNLOCK();
			return 1;
		}
	}
	HOLI_UNLOCK();

	return 0;
}

static int _check_yoil(int year, int month, int day)
{
	int i;
	int yoil = ifn_get_yoil(year, month, day);

	HOLI_LOCK();
	for (i = 0; i < ihd.cnt; ++i) {
		if (_is_older_day(year, month, day, &ihd.holi[i])) continue;
		if (ihd.holi[i].type != 'B') continue;
		if (ihd.holi[i].yoil == yoil) { 
			HOLI_UNLOCK();
			return 1;
		}
	}
	HOLI_UNLOCK();

	return 0;
}

static int _check_day_of_month(int year, int month, int day)
{
	int i;

	HOLI_LOCK();
	for (i = 0; i < ihd.cnt; ++i) {
		if (_is_older_day(year, month, day, &ihd.holi[i])) continue;
		if (ihd.holi[i].type != 'E') continue;
		if (ihd.holi[i].day == day) {
			HOLI_UNLOCK();
			return 1;
		}
	}
	HOLI_UNLOCK();

	return 0;
}

static int _check_date_of_year(int year, int month, int day)
{
	int i;

	HOLI_LOCK();
	for (i = 0; i < ihd.cnt; ++i) {
		if (_is_older_day(year, month, day, &ihd.holi[i])) continue;
		if (ihd.holi[i].type != 'C') continue;
		if ((ihd.holi[i].month == month) && (ihd.holi[i].day == day)) {
			HOLI_UNLOCK();
			return 1;
		}
	}
	HOLI_UNLOCK();

	return 0;
}

static int _check_yoil_week_of_month(int year, int month, int day)
{
	int i;
	int yoil = ifn_get_yoil(year, month, day);
	int week = ((day - 1) / 7) + 1;

	HOLI_LOCK();
	for (i = 0; i < ihd.cnt; ++i) {
		if (_is_older_day(year, month, day, &ihd.holi[i])) continue;
		if (ihd.holi[i].type != 'D') continue;
		if ((ihd.holi[i].yoil == yoil) && (ihd.holi[i].week == week)) {
			HOLI_UNLOCK();
			return 1;
		}
	}
	HOLI_UNLOCK();

	return 0;
}

#if 0
static int _countdown_to_date(char *dest, char *today)
{
	int year, month, day;
	int d_year, d_month, d_day;
	int days;
	int d;

	year = _get_year(today);	
	month = _get_month(today);
	day = _get_day(today);

	d_year = _get_year(dest);	
	d_month = _get_month(dest);
	d_day = _get_day(dest);

	while (1) {
		printf("%d, %d, %d\n", year, month, day);
		printf("%d, %d, %d\n", d_year, d_month, d_day);


		printf("%d, %d\n", year, month);
		for (d = day; d > 0; --d) {
			printf("%02d ", d);
			
			if ((d_year == year) && (d_month == month) && (d_day == d)) return 0;
		}
		printf("\n");

		if (--month == 0) {
			--year;
			month = 12;
		}
		day = ifn_get_monthdays(year, month);
	}

}

static int _countdown_days(int cnt_day, char *today)
{
	int year, month, day;
	int days;
	int d;

	year = _get_year(today);	
	month = _get_month(today);
	day = _get_day(today);

	while (1) {

		printf("%d, %d\n", year, month);
		for (d = day; d > 0; --d) {
			printf("%02d ", d);
			
			if (--cnt_day == 0) return 0;
		}
		printf(" (%d)\n", cnt_day);

		if (--month == 0) {
			--year;
			month = 12;
		}
		day = ifn_get_monthdays(year, month);
	}

	return 0;
}

static int _countdown_days_skip_holiday(int cnt_day, char *today)
{
	int year, month, day;
	int d;

	year = _get_year(today);	
	month = _get_month(today);
	day = _get_day(today);

	if (!ifn_is_valid_date(year, month, day)) {
		printf("Invalid date\n");
		return -1;
	}

	while (1) {

		printf("%d, %d\n", year, month);
		for (d = day; d > 0; --d) {
			if (ifn_is_holiday(year, month, d)) continue;

			printf("%02d ", d);
			if (--cnt_day == 0) return 0;
		}
		printf(" (%d remained)\n", cnt_day);

		if (--month == 0) {
			--year;
			month = 12;
		}
		day = ifn_get_monthdays(year, month);
	}

	return 0;
}
#endif

static int _is_booting()
{

	return 0;
}

#define TWOMIN		(2 * 60)
static int _is_midnight()
{
	time_t today_mid;
	time_t cur;
	cur = time(0);
	today_mid = ifn_get_local_midnight(cur);

	if (cur - today_mid < TWOMIN) return 1;
	return 0;
}

static gboolean _proc_moving_rtl(void *data)
{
	SCM_T *piscm = (SCM_T *)data;
	int new_rtl = 0;
	int enforce = 0;

	if (uxm_is_booting()) return TRUE;
	if (!_scm_is_rtl_movable()) return TRUE;

	new_rtl = _scm_get_rtl_state();

	HOLI_LOCK();
	enforce = ihd.enforce;
	HOLI_UNLOCK();

	if (!enforce) {
		if (!_is_midnight()) return TRUE;
		if (piscm->cur_rtl == new_rtl) return TRUE;
	}

	nf_filesystem_set_rtlimit_runtime(new_rtl);

	HOLI_LOCK();
	ihd.enforce = 0;
	HOLI_UNLOCK();

	return TRUE;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_load_holiday_data()
{
	int i;

	DMSG(1, "");
	HOLI_LOCK();

	memset(ihd.holi, 0x00, sizeof(ihd.holi));
	ihd.cnt = DAL_get_holiday_count();
	
	for (i = 0; i < ihd.cnt; ++i) {
		DAL_get_UsrDefHoliday_Data(&ihd.holi[i], i);
		
/*		printf("SKSHIN] %d, %d, %d, %d, %d, %c\n", 
		ihd.holi[i].year,
		ihd.holi[i].month,
		ihd.holi[i].day,
		ihd.holi[i].yoil,
		ihd.holi[i].week,
		ihd.holi[i].type);*/
	}
	ihd.enforce = 1;

	HOLI_UNLOCK();
	return 0;
}

int _scm_cleanup_holi()
{
	g_mutex_free(ihd.mtx);
	return 0;
}

int _scm_init_moving_rtl_timer(SCM_T *piscm)
{
	memset(&ihd, 0x00, sizeof(ihd));

	ihd.mtx = g_mutex_new();
	_scm_load_holiday_data();
	piscm->tmr_mvrtl = _scm_add_timeout(piscm, ONEMIN, _proc_moving_rtl, piscm);
//	piscm->tmr_mvrtl = _scm_add_timeout(piscm, 1000, _proc_moving_rtl, piscm);
	return piscm->tmr_mvrtl;
}

static int _get_sec_of_today()
{
	time_t cur = time(0);
	struct tm *pt = 0;
	pt = NFLOCALTIME(&cur);

	return (pt->tm_hour * (60 * 60)) + (pt->tm_min * 60) + (pt->tm_sec);
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_is_support_holiday()
{
    if (ivsc.dfunc.support_usrdef_holiday) return 1;

    return 0;
}
int scm_is_holiday(int year, int month, int day)
{
	int yoil;
	int week;

	if (!ifn_is_valid_date(year, month, day)) {
		DMSG(1, "Invalid date\n");
		return -1;
	}

	yoil = ifn_get_yoil(year, month, day);
	week = ifn_get_week_of_month(day);

	if (_check_same_date(year, month, day)) return 1;
	if (_check_yoil(year, month, day)) return 1;
	if (_check_day_of_month(year, month, day)) return 1;
	if (_check_date_of_year(year, month, day)) return 1; 
	if (_check_yoil_week_of_month(year, month, day)) return 1;

	return 0;
}

int scm_is_holiday_timet(time_t timeinfo)
{
	int yoil;
	int week;
	int year, month, day;

	ifn_get_local_day(timeinfo, &year, &month, &day);

	if (!ifn_is_valid_date(year, month, day)) {
		DMSG(1, "Invalid date\n");
		return -1;
	}

	yoil = ifn_get_yoil(year, month, day);
	week = ((day - 1) / 7) + 1;

	if (_check_same_date(year, month, day)) return 1;
	if (_check_yoil(year, month, day)) return 1;
	if (_check_day_of_month(year, month, day)) return 1;
	if (_check_date_of_year(year, month, day)) return 1; 
	if (_check_yoil_week_of_month(year, month, day)) return 1;

	return 0;
}

int scm_get_rtl_skip_holiday(int cur_rtl, int year, int month, int day)
{
	int d;
	int rtl = 0;
	int ts = 0;

	if (!ifn_is_valid_date(year, month, day)) {
		DMSG(1, "Invalid date\n");
		return -1;
	}

	if (!scm_is_holiday(year, month, day)) {
		ts = _get_sec_of_today();
		rtl += ts;
		cur_rtl -= ts;
		--day;
	}

	while (cur_rtl > 0) {
		for (d = day; d > 0; --d) {
			rtl += ONEDAY;

			if (scm_is_holiday(year, month, d)) continue;

			cur_rtl -= ONEDAY;
			if (cur_rtl <= 0) return (rtl + cur_rtl);
		}

		if (--month == 0) {
			--year;
			month = 12;
		}
		day = ifn_get_monthdays(year, month);
	}

	return rtl;
}
