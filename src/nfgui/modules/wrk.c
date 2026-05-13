/*
 * wrk.c
 * 	- work thread
 *	- dependencies :
 *			GThread
 *			CMM
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 6, 2011
 *
 */

#include "ix_mem.h"
#include "iux_afx.h"
#include "wrk.h"
#include "cmm.h"
#include <memory.h>
#include "ix_func.h"
#include "ix_conf.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"WRK"


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef enum _WORK_STAT_E {
	READY	= 0,
	STOP	= 1,
	RUN		= 2,
	PAUSE	= 3,
} WORK_STAT_E;

typedef enum _STOP_STAT_E {
	YES		= 0,
	NOTYET	= 1,
} STOP_STAT_E;

typedef enum _RUN_MODE_E {
	MSG		= 0,
	ONCE	= 1,
	LOOP	= 2,
} RUN_MODE_E;

typedef struct _WRK_T {
	GThread			*thd;
	CMMPORT			cmmpt;
	WORK_STAT_E		work;
	STOP_STAT_E		stopped;
	RUN_MODE_E		mode;
	int				sleep_time;

	WRK_ID			id;
	WORKER_PROC		proc;
	CMM_MESSAGE_T	wrk_msg;
	
	CMMACK_T		cmmack;
} WRK_T;

////////////////////////////////////////////////////////////
//
// private variable
//


////////////////////////////////////////////////////////////
//
// private functions
//

static int _cleanup_thread(WRK_T *pwrk)
{
	if (!pwrk->cmmpt) return -1;
    if (cmm_mount_off_thread(pwrk->cmmpt) == -1) return -1; 
	pwrk->cmmpt = 0;
    return 0;
}

static int _wait_run_signal(WRK_T *pwrk)
{
	DMSG(1, "");
	while (pwrk->work == READY) usleep(100000);
	DMSG(1, "");
	return 0;
}

static int _stop(WRK_T *pwrk)
{
	pwrk->work = STOP;
	return 0;
}

static int _pause(WRK_T *pwrk)
{
	pwrk->work = PAUSE;
	return 0;
}

static int _run(WRK_T *pwrk)
{
	pwrk->stopped = NOTYET;
	pwrk->work = RUN;
	return 0;
}

static int _is_paused(WRK_T *pwrk)
{
	return (pwrk->work == PAUSE || pwrk->work == STOP);
}

static int _is_work(WRK_T *pwrk)
{
	return (pwrk->work == RUN || pwrk->work == PAUSE);
}

static int _is_stop(WRK_T *pwrk)
{
	return (pwrk->work == STOP);
}

static int _wait_for_stop(WRK_T *pwrk)
{
	DMSG(1, "WAIT FOR STOP...[%p]\n", pwrk);
	while (pwrk->stopped == NOTYET) usleep(100000);
	DMSG(1, "");
	return 0;
}

static int _is_loop_mode(WRK_T *pwrk)
{
	return (pwrk->mode == LOOP);
}

static int _process_message(WRK_T *pwrk, CMM_MESSAGE_T *pmsg)
{
	int ret = 0;

	ret = pwrk->proc((WRK_ID)pwrk, pmsg);
	if (!_is_loop_mode(pwrk))
		if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);

	return ret;
}

static int _reload(WRK_T *pwrk, CMM_MESSAGE_T *pmsg)
{
	DMSG(9, "");
	cmm_send_message(pwrk->cmmpt, pmsg->msgid, pmsg->param, pmsg->dyn_data, pmsg->data);
	return 0;
}

static int _is_once_mode(WRK_T *pwrk)
{
	return (pwrk->mode == ONCE);
}

static void* _wrk_service_proc(void *arg) 
{
	CMM_MESSAGE_T msg;
	WRK_T *pwrk = (WRK_T *)arg;

	CMMPORT ack_cmmpt = pwrk->cmmack.cmmpt;
	IMSG ack_msgid = pwrk->cmmack.msgid;
	int ack_ret = 0;

	_wait_run_signal(pwrk);
	while (_is_work(pwrk)) {
		usleep(pwrk->sleep_time);

		DMSG(2, "WORKER's ID = [%p]\n", pwrk);
		if (cmm_get_message(&msg) == 0) 
			ack_ret = _process_message(pwrk, &msg);
		else continue;
		if (_is_paused(pwrk)) { usleep(20000); continue; }
		if (_is_loop_mode(pwrk)) _reload(pwrk, &pwrk->wrk_msg);
		else if (_is_once_mode(pwrk)) break;
	}

	DMSG(1, "WORKER THREAD's MISSION COMPLETED. [%p]\n", pwrk);
	pwrk->work = READY;

	_cleanup_thread(pwrk);
	pwrk->stopped = YES;

	if (ack_cmmpt)
		cmm_send_message(ack_cmmpt, ack_msgid, ack_ret, 0, (void *)pwrk);

	g_thread_exit(NULL);
	return NULL;
}

static int _init_thread(WRK_T *pwrk)
{
	pwrk->work = READY;
	pwrk->stopped = YES;
	pwrk->thd = ifn_make_thread(_wrk_service_proc, pwrk);
	cmm_mount_on_thread(pwrk->thd);
	pwrk->cmmpt = pwrk->thd;
	return 0;
}

static int _init(WRK_T *pwrk, WORKER_PROC proc, CMMACK_T *pcmmack)
{
	memset(pwrk, 0x00, sizeof(WRK_T));
	pwrk->id = (WRK_ID)pwrk;
	pwrk->sleep_time = 50000;
	pwrk->proc = proc;
	pwrk->mode = MSG;
	if (pcmmack) pwrk->cmmack = *pcmmack;

	DMSG(3, "");
	_init_thread(pwrk);
	return 0;
}

static int _cleanup(WRK_T *pwrk)
{
	usleep(100000);
	memset(pwrk, 0x00, sizeof(WRK_T));
	return 0;
}

static int _cleanup_work_msg(WRK_T *pwrk)
{
	if (pwrk->wrk_msg.dyn_data && pwrk->wrk_msg.data) 
		ifree(pwrk->wrk_msg.data);

	memset(&pwrk->wrk_msg, 0x00, sizeof(CMM_MESSAGE_T));
	return 0;
}

static int _read_conf(WRK_T *pwrk)
{
	int ret;
	ret = icf_get_value_by_int("wrk", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

WRK_ID wrk_create_worker(WORKER_PROC proc, CMMACK_T *pcmmack)
{
	WRK_T *pwrk = imalloc(sizeof(WRK_T));
	_read_conf(pwrk);
	DMSG(1, "");
	_init(pwrk, proc, pcmmack);
	_run(pwrk);
	return (WRK_ID)pwrk;
}

int wrk_destroy_worker(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	DMSG(1, "");
	_stop(pwrk);
	_wait_for_stop(pwrk);
	_cleanup_work_msg(pwrk);
	_cleanup_thread(pwrk);
	_cleanup(pwrk);
	ifree(pwrk);
	return 0;
}

int wrk_run_once(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	CMM_MESSAGE_T msg;

	if (!pwrk) return -1;
	DMSG(2, "");
	pwrk->mode = ONCE;
	cmm_make_message(IMSG_NONE, 0, 0, 0, &msg);
	_reload(pwrk, &msg);
	return 0;
}

int wrk_run_once_param(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	CMM_MESSAGE_T msg;

	if (!pwrk) return -1;
	DMSG(2, "");
	pwrk->mode = ONCE;
	cmm_make_message(msgid, param, dyn_data, data, &msg);
	_reload(pwrk, &msg);
	return 0;
}

int wrk_run_msg(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	CMM_MESSAGE_T msg;

	if (!pwrk) return -1;
	DMSG(2, "");
	pwrk->mode = MSG;
	cmm_make_message(msgid, param, dyn_data, data, &msg);
	_reload(pwrk, &msg);
	return 0;
}

int wrk_run_loop(WRK_ID wrk_id, IMSG msgid, int param, bool dyn_data, void *data)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	CMM_MESSAGE_T msg;

	if (!pwrk) return -1;
	DMSG(2, "");
	pwrk->mode = LOOP;
	cmm_make_message(msgid, param, dyn_data, data, &pwrk->wrk_msg);
	_reload(pwrk, &pwrk->wrk_msg);
	return 0;
}

int wrk_pause(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	_pause(pwrk);	
	return 0;
}

int wrk_stop(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	_stop(pwrk);	
	return 0;
}

int wrk_wait_for_stop(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	_wait_for_stop(pwrk);
	return 0;
}

/*inline*/ extern CMMPORT wrk_get_cmmport(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	return pwrk->cmmpt;	
}

int wrk_get_cmmack(WRK_ID wrk_id, CMMACK_T *pcmmack)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	memcpy(pcmmack, &pwrk->cmmack, sizeof(CMMACK_T));
	return 0;
}

int wrk_clear_job(WRK_ID wrk_id)
{
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	if (!pwrk->cmmpt) return -1;

	cmm_clear_queue_cmmpt(pwrk->cmmpt);
	_cleanup_work_msg(pwrk);
	return 0;
}

int wrk_change_sleep_time(WRK_ID wrk_id, int usec)
{	
	WRK_T *pwrk = (WRK_T*)wrk_id;
	if (!pwrk) return -1;
	pwrk->sleep_time = usec;
	return 0;
}
