/*
 * cmm.h
 * 	- communication manager inter iux modules
 *	- dependency :
 *		ixqueue
 *		GThread
 *		GMutex
 *		iux_msg
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#ifndef __CMM_H
#define __CMM_H


#include <glib.h>
#include "iux_types.h"
#include "iux_msg.h"


////////////////////////////////////////////////////////////
//
// public data type 
//

typedef GThread*	CMMPORT;

typedef struct _CMM_MESSAGE_T {
	CMMPORT			sender;
	IMSG		 	msgid;
	long		 	param;
	bool			dyn_data;
	void 			*data;
} CMM_MESSAGE_T;

/*
 * CMMACK is a set of data to return to the caller
 */
typedef struct _CMMACK_T {
	CMMPORT		cmmpt;
	IMSG		msgid;
	void		*data;
} CMMACK_T;


////////////////////////////////////////////////////////////
//
// public functions
//

int cmm_init();
int cmm_cleanup();
int cmm_mount_on_thread(GThread *thread);
int cmm_mount_off_thread(GThread *thread);
int cmm_send_message(CMMPORT receiver, IMSG msgid, long param, bool dyn_data, void *data);
int cmm_resend_message(CMMPORT receiver, CMM_MESSAGE_T *rmsg);
int cmm_get_message(CMM_MESSAGE_T *buf);
int cmm_peek_message(CMM_MESSAGE_T *buf, int nth);
CMMPORT cmm_read_sender_cmmport(CMM_MESSAGE_T *msg);
int cmm_get_message_count(CMMPORT cmmpt);
int cmm_clear_queue();
int cmm_clear_queue_cmmpt(CMMPORT cmmpt); 
int cmm_make_message(IMSG msgid, long param, bool dyn_data, void *data, CMM_MESSAGE_T *pmsg);

#endif
