/*
 * stm.h
 * 	- search time manager
 *	- dependencies :
 *	
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 30, 2011
 *
 */

#ifndef __STM_H
#define __STM_H



////////////////////////////////////////////////////////////
//
// public data type 
//



////////////////////////////////////////////////////////////
//
// public interfaces
//

int stm_init();
int stm_set_time(GTimeVal *search_time);
int stm_set_time_t(time_t search_time);
int stm_set_time_by_sys();
int stm_get_time(GTimeVal *tv);
time_t stm_get_time_t();
int stm_set_arch_time_t(time_t start, time_t end);
int stm_get_arch_time_t(time_t *start, time_t *end);

#endif
