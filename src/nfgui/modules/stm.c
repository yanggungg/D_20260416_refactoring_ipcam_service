/*
 * stm.c
 * 	- search time manager
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 30, 2011
 *
 */

#include "iux_afx.h"
#include "stm.h"
#include <memory.h>
#include "ix_func.h"


#define DBG_LEVEL		1
#define DBG_MODULE		"STM"

#define DEFAULT_RANGE 300


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _STM_T {
	time_t stime;
	time_t etime;
	time_t arch_start;
	time_t arch_end;
} STM_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static STM_T istm;



////////////////////////////////////////////////////////////
//
// private functions
//



////////////////////////////////////////////////////////////
//
// protected interfaces
//

int stm_init()
{
	memset(&istm, 0x00, sizeof(STM_T));
	istm.stime = time(0);
	return 0;
}

int stm_set_time(GTimeVal *search_time)
{
	if (search_time == 0) return -1;
	istm.stime = ifn_gtv_to_timet(search_time);
	istm.etime = istm.stime + DEFAULT_RANGE;
	DMSG(9, "%ld, %ld", istm.stime, istm.etime);
	return 0;
}

int stm_set_time_t(time_t search_time)
{
	if (search_time == 0) return -1;
	istm.stime = search_time;
	istm.etime = istm.stime + DEFAULT_RANGE;
	DMSG(9, "%ld, %ld", istm.stime, istm.etime);
	return 0;
}

int stm_set_endtime_t(time_t end_time)
{
	if (end_time == 0) return -1;
	istm.etime = end_time;
	DMSG(9, "%ld, %ld", istm.stime, istm.etime);
	return 0;
}

int stm_set_time_by_sys()
{
	istm.stime = time(0);
	istm.etime = istm.stime + DEFAULT_RANGE;
	DMSG(9, "%ld, %ld", istm.stime, istm.etime);
	return 0;
}

int stm_get_time(GTimeVal *tv)
{
	tv->tv_sec = istm.stime;
	tv->tv_usec = 0;
	return 0;
}

time_t stm_get_time_t()
{
	if (istm.stime == 0) {
		istm.stime = time(0);
		istm.etime = istm.stime + DEFAULT_RANGE;
	}
	return istm.stime;
}

int stm_get_time_range_t(time_t *start, time_t *end)
{
	if (istm.stime == 0 || istm.etime == 0) {
		istm.etime = time(0);
		istm.stime = istm.etime - DEFAULT_RANGE;
		if (start) *start = istm.stime;
		if (end) *end = istm.etime;
	}
	else {
		if (start) *start = istm.stime;
		if (end) *end = istm.etime;
}
	return 0;
}
//captainnn
int stm_set_arch_time_t(time_t start, time_t end)
{
	if (start) istm.arch_start = start;
	if (end) istm.arch_end = end;
	return 0;
}

/* WARNING
 *
 * the variables are volatile
 */
int stm_get_arch_time_t(time_t *start, time_t *end)
{
	if (start) *start = istm.arch_start;
	if (end) *end = istm.arch_end;
	istm.arch_start = 0;
	istm.arch_end = 0;
	return 0;
}
