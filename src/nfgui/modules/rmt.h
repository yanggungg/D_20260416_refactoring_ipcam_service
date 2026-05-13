/*
 * rmt.h
 * 	- remote client session modules
 *	- dependencies :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 21, 2011
 *
 */

#ifndef __RMT_H
#define __RMT_H

#include "iux_afx.h"

#define LEN_ADDR		64



////////////////////////////////////////////////////////////
//
// public data type 
//

typedef int SSID;			// session id

typedef enum _SESSION_TYPE_E {
	SSN_LIVE		= 0x0,
	SSN_PLAY		= 0x1,
	SSN_ARCH		= 0x2,
	SSN_NOTHING		= 0x3,
} SESSION_TYPE_E;

typedef struct _SESSION_INFO_T {
	SESSION_TYPE_E	type;
	int				cnt;
	SSID			id[32];
} SESSION_INFO_T;

typedef struct _REMOTE_USER_T {
	char			user_id[LEN_USERID];
	char			addr[LEN_ADDR];
	int				cnt_all;
	SESSION_INFO_T	ssinfo[4];
} REMOTE_USER_T;

typedef struct _SESSION_LIST_T {
	SSID			id;
	SESSION_TYPE_E	type;
	char			user_id[LEN_USERID];
	char 			addr[LEN_ADDR];
} SESSION_LIST_T;


////////////////////////////////////////////////////////////
//
// public functions
//

REMOTE_USER_T *rmt_new_remote_user_info(int *ret_cnt);
int rmt_free_remote_user_info(REMOTE_USER_T *remote);
SESSION_LIST_T *rmt_new_session_list(int *ret_cnt);
int rmt_free_session_list(SESSION_LIST_T *session);
int rmt_disconnect_session(SSID id);

#endif
