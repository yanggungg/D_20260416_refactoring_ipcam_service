/*
 * scm_session.c
 * 	- scm session service
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Aug 2, 2012
 *
 */

#include "scm_internal.h"
#include "scm.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "ix_func.h"
#include "ssm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_SES"




////////////////////////////////////////////////////////////
//
// public data type 
//



////////////////////////////////////////////////////////////
//
// private functions
//



////////////////////////////////////////////////////////////
//
// protected interfaces
//



////////////////////////////////////////////////////////////
//
// public interfaces
//


int scm_check_expired_user(char *user_id)
{
	return ssm_get_remain_day_to_expire_user(user_id);
}

