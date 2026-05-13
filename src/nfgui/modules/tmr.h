/*
 * tmr.h
 * 	- generic timer module
 *	- dependency :
 *		main loop context (UI timeout signal)
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Aut 31, 2011
 *
 */

#ifndef __TMR_T
#define __TMR_T

#include <glib.h>
#include "iux_afx.h"
#include "cmm.h"


////////////////////////////////////////////////////////////
//
// public data types
//

#if defined(__ITXGUI64)
typedef unsigned long int TIMERID;
#else
typedef unsigned int TIMERID;
#endif



////////////////////////////////////////////////////////////
//
// public interfaces
//

TIMERID tmr_new(int sec, CMMPORT rcv_cmmpt, IMSG expire_msg);
int tmr_change_chk_interval(TIMERID id, int msec);
int tmr_reset(TIMERID id);
int tmr_start(TIMERID id);
int tmr_stop(TIMERID id);
int tmr_free(TIMERID id);
int tmr_is_running(TIMERID id);


#endif
