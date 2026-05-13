/*
 * wrk.h
 * 	- worker thread
 *	- dependencies :
 *	
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 6, 2011
 *
 */

#ifndef __WRK_H
#define __WRK_H

#include "cmm.h"


////////////////////////////////////////////////////////////
//
// public data type 
//

#if defined(__ITXGUI64)
typedef unsigned long int WRK_ID;
#else
typedef unsigned int WRK_ID;
#endif

typedef int (*WORKER_PROC)(WRK_ID wrkid, CMM_MESSAGE_T *pmsg);

typedef struct _WRK_CHART_T {
	WORKER_PROC		proc;
	int				param;
	void			*data;
} WRK_CHART_T;


////////////////////////////////////////////////////////////
//
// public interfaces
//

WRK_ID wrk_create_worker(WORKER_PROC proc, CMMACK_T *pcmmack);
int wrk_destroy_worker(WRK_ID wrk_id);
int wrk_run_once(WRK_ID wrk_id);
int wrk_run_once_param(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data);
int wrk_run_msg(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data);
int wrk_run_loop(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data);
int wrk_pause(WRK_ID wrk_id);
int wrk_stop(WRK_ID wrk_id);
int wrk_wait_for_stop(WRK_ID wrk_id);
/*inline*/ CMMPORT wrk_get_cmmport(WRK_ID wrk_id);
int wrk_get_cmmack(WRK_ID wrk_id, CMMACK_T *pcmmack);
int wrk_clear_job(WRK_ID wrk_id);
int wrk_change_sleep_time(WRK_ID wrk_id, int usec);

#endif
