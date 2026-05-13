/*
 * tmr.c
 * 	- generic timer module
 *	- dependency :
 *		main loop context (UI timeout signal)
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Aut 31, 2011
 *
 */


#include "tmr.h"
#include <memory.h>
#include "iux_afx.h"
#include "ix_conf.h"
#include "ix_mem.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		0
#define DBG_MODULE		"TMR"



////////////////////////////////////////////////////////////
//
// private data type 
//

enum {
	RUNNING = 1,
	STOP	= 0,
};

typedef struct _TMR_T {
	GTimer 		*exp_timer;
	CMMPORT		rcv_cmmpt;
	IMSG		exp_msg;
	int			timeout;	// sec
	guint		chk_timer;
	int			interval;
	int			is_running;

	int			dmsg;
} TMR_T;

////////////////////////////////////////////////////////////
//
// private variables 
//



////////////////////////////////////////////////////////////
//
// private functions
//


static int _reset(TMR_T *pitmr)
{
	if (pitmr->exp_timer) g_timer_reset(pitmr->exp_timer);
}

static int _init_exp_timer(TMR_T *pitmr)
{
	DMSG(1, "");
	if (pitmr->exp_timer) {
		g_timer_reset(pitmr->exp_timer);
		return 0;
	}
	
	pitmr->exp_timer = g_timer_new();
	g_timer_start(pitmr->exp_timer);
	return 0;
}

static int _destroy_exp_timer(TMR_T *pitmr)
{
	if (pitmr->exp_timer) {
		DMSG(1, "");
		g_timer_stop(pitmr->exp_timer);
		g_timer_destroy(pitmr->exp_timer);
		pitmr->exp_timer = NULL;
	}
	return 0;
}

static int _destroy_chk_timer(TMR_T *pitmr)
{
	if (pitmr->chk_timer) {
		g_source_remove(pitmr->chk_timer);
		pitmr->chk_timer = NULL;
	}
	return 0;
}

static gboolean _proc_check_expire(void *data)
{
	TMR_T *pitmr = (TMR_T *)data;
	guint timeout;
	gdouble elapsed;

	if (!pitmr->exp_timer) return TRUE;

	timeout = (guint)pitmr->timeout;
	elapsed = g_timer_elapsed(pitmr->exp_timer, NULL);

	DMSG(9, "(%d, %f)\n", timeout, elapsed);
	if (timeout <= (guint)elapsed) {
		DMSG(1, "TIMER TIMEOUT...!!");
		g_assert(pitmr->rcv_cmmpt);
		cmm_send_message(pitmr->rcv_cmmpt, pitmr->exp_msg, 0, 0, 0);
		_destroy_exp_timer(pitmr);
		return FALSE;
	}
	return TRUE; 
}

static int _init_chk_timer(TMR_T *pitmr)
{
	DMSG(1, "");
	if (pitmr->chk_timer) return 0;
	pitmr->chk_timer = g_timeout_add(pitmr->interval, _proc_check_expire, pitmr);
	return 0;
}

static int _read_conf(TMR_T *pitmr)
{
	int ret;
	ret = icf_get_value_by_int("tmr", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//



////////////////////////////////////////////////////////////
//
// public interfaces
//

TIMERID tmr_new(int sec, CMMPORT rcv_cmmpt, IMSG expire_msg)
{
	TMR_T *ptmr = 0;
	
	DBG_USE(1);
	DMSG(1, "SEC = %d\n", sec);
	if (sec == 0) return 0;
	if (rcv_cmmpt == 0) return 0;
	if (expire_msg == 0) return 0;	
	DBG_USE(0);

	ptmr = imalloc(sizeof(TMR_T));	
	ptmr->timeout = sec;
	ptmr->rcv_cmmpt = rcv_cmmpt;
	ptmr->exp_msg = expire_msg;
	ptmr->interval = 100;		// 100ms
	ptmr->is_running = STOP;

	_read_conf(ptmr);
	DMSG(1, "TIMER ID = [%p]\n", ptmr);
	return (TIMERID)ptmr;
}

int tmr_change_chk_interval(TIMERID id, int msec)
{
	TMR_T *ptmr = (TMR_T *)id;

	if (!ptmr) return -1;
	ptmr->interval = msec;
	tmr_stop(ptmr);
	tmr_start(ptmr);
	return 0;
}

int tmr_reset(TIMERID id)
{
	TMR_T *ptmr = (TMR_T *)id;

	if (!ptmr) return -1;
	_reset(ptmr);
	return 0;
}

int tmr_start(TIMERID id)
{
	TMR_T *ptmr = (TMR_T *)id;

	if (!ptmr) return -1;
	if (!ptmr->rcv_cmmpt) return -1;
	if (!ptmr->timeout) return -1;

	DMSG(1, "START TIMER : TIMER ID = [%p]\n", ptmr);
	_init_exp_timer(ptmr);
	_init_chk_timer(ptmr);
	ptmr->is_running = RUNNING;
	return 0;
}

int tmr_stop(TIMERID id)
{
	TMR_T *ptmr = (TMR_T *)id;

	DMSG(1, "");
	if (!ptmr) return -1;
	_destroy_exp_timer(ptmr);
	_destroy_chk_timer(ptmr);
	ptmr->is_running = STOP;
	DMSG(1, "TIMER STOPPED\n");
	return 0;
}

int tmr_free(TIMERID id)
{
	TMR_T *ptmr = (TMR_T *)id;

	DMSG(1, "");
	if (!ptmr) return -1;
	tmr_stop(ptmr);
	ifree(ptmr);
	return 0;	
}

int tmr_is_running(TIMERID id)
{
	TMR_T *ptmr = (TMR_T *)id;

	DMSG(1, "");
	if (!ptmr) return -1;
	return (int)ptmr->is_running;
}
