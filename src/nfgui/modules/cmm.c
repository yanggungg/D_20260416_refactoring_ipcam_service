/*
 * cmm.c
 * 	- communication manager inter iux modules
 *	- dependencies :
 *		ixqueue
 *		GThread
 *		GMutex
 *		iux_msg
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#include <glib.h>
#include "ix_queue.h"
#include "cmm.h"
#include "ix_conf.h"
#include "ix_mem.h"
#include "iux_afx.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"CMM"


#define CMM_LOCK()		g_mutex_lock(icmm.mtx)
#define CMM_UNLOCK()	g_mutex_unlock(icmm.mtx)


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _QINFO_T {
	IXQueue 		*qptr;
	CMMPORT			cmmpt;
} QINFO_T;

typedef struct _CMM_T {
	GMutex			*mtx;
	QINFO_T			*qi;
	int	 			qi_cnt;

	// conf
	int				max_comm;
} CMM_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static CMM_T icmm;


////////////////////////////////////////////////////////////
//
// private functions
//

static int _find_queue(CMM_T *picmm, CMMPORT cmmpt)
{
	int i;
	for (i = 0; i < picmm->max_comm; ++i) {
		if (picmm->qi[i].cmmpt == cmmpt) break;
	}
	
	if (i == picmm->max_comm) 
	{
		for (i = 0; i < picmm->max_comm; ++i) {
			printf("[%s, %d] picmm->qi[%d].cmmpt : %llx, cmmpt : %llx\n", __FUNCTION__, __LINE__, i, picmm->qi[i].cmmpt, cmmpt);
		}
		return -1;
	}
	return i;
}

static void *_make_message(IMSG msgid, long param, bool dyn_data, void *data)
{
	CMM_MESSAGE_T *pmsg = 0;
	pmsg = imalloc(sizeof(CMM_MESSAGE_T));
	pmsg->sender = g_thread_self();
	pmsg->msgid = msgid;
	pmsg->param = param;
	pmsg->dyn_data = dyn_data;
	pmsg->data = data;
	return pmsg;
}

static int _read_conf(CMM_T *picmm)
{
	int ret;
	picmm->max_comm = icf_get_value_by_int("cmm", "max_comm_cnt");
	ret = icf_get_value_by_int("cmm", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}

static int _open_queue(CMM_T *picmm, CMMPORT cmmpt)
{
	int i;
	if (picmm->qi_cnt >= picmm->max_comm) {
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		return -1;
	}
	
	for (i = 0; i < picmm->max_comm; i++) {
		if (picmm->qi[i].qptr == 0) break;
	}

	picmm->qi[i].qptr = ix_queue_new();
	picmm->qi[i].cmmpt = cmmpt;
	picmm->qi_cnt++;
	
	return 0;	
}

static int _is_ready(CMM_T *picmm)
{
	return (picmm->qi != 0);
}

static int _get_message_count(CMM_T *picmm, CMMPORT cmmpt)
{
//	CMMPORT cmmpt = g_thread_self();
	int qid = _find_queue(picmm, cmmpt);
	if (qid == -1) return -1;
	return (int)ix_queue_sync_get_sizeof(picmm->qi[qid].qptr);
}
	
static int _send_message(CMM_T *picmm, CMMPORT receiver, IMSG msgid, long param, bool dyn_data, void *data)
{
	CMM_MESSAGE_T *pmsg;
	int qid = _find_queue(picmm, receiver);
	if (qid == -1) {
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		return -1;
	}

	pmsg = _make_message(msgid, param, dyn_data, data);
	if (!pmsg) {
		g_warning("%s, %d", __FUNCTION__, __LINE__);		
		return -1;
	}

	ix_queue_sync_push(picmm->qi[qid].qptr, (gpointer)pmsg);
	return 0;	
}

static int _remove_all_msg_cmmpt(CMM_T *picmm, CMMPORT cmmpt)
{
	CMM_MESSAGE_T *pmsg;
	int qid = _find_queue(picmm, cmmpt);
	if (qid == -1) return -1;

	while (1) {
		pmsg = ix_queue_sync_pop(picmm->qi[qid].qptr);
		if (pmsg) {
			if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
			ifree(pmsg);
		}
		else break;
	}
	return 0;
}

static int _remove_all_msg(CMM_T *picmm)
{
	CMM_MESSAGE_T *pmsg;
	CMMPORT cmmpt = g_thread_self();
	int qid = _find_queue(picmm, cmmpt);
	if (qid == -1) return -1;

	while (1) {
		pmsg = ix_queue_sync_pop(picmm->qi[qid].qptr);
		if (pmsg) {
			if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);
			ifree(pmsg);
		}
		else break;
	}
	return 0;
}

static int _close_queue(CMM_T *picmm, CMMPORT cmmpt) 
{
	int qid = _find_queue(picmm, cmmpt);
	if (qid == -1) return -1;
	ix_queue_free(picmm->qi[qid].qptr);
	picmm->qi[qid].qptr = NULL;
	picmm->qi[qid].cmmpt = NULL;
	picmm->qi_cnt--;
	return 0;
}

	
////////////////////////////////////////////////////////////
//
// public interfaces
//

int cmm_init()
{
	memset(&icmm, 0x00, sizeof(icmm));

	if (_read_conf(&icmm) == -1) return -1;
	icmm.mtx = g_mutex_new();
	icmm.qi = imalloc(sizeof(QINFO_T) * icmm.max_comm);
	memset(icmm.qi, 0x00, sizeof(QINFO_T) * icmm.max_comm);

	return 0;
}

int cmm_cleanup()
{
	g_mutex_free(icmm.mtx);
}

int cmm_mount_on_thread(GThread *thread)
{
	CMM_LOCK();

	if (_is_ready(&icmm) == -1) { CMM_UNLOCK(); return -1; }
	_open_queue(&icmm, thread);

	CMM_UNLOCK();
	return 0;
}

int cmm_mount_off_thread(GThread *thread)
{
	CMMPORT cmmpt;
	CMM_LOCK();

	cmmpt = g_thread_self();
	if (_is_ready(&icmm) == -1) { CMM_UNLOCK(); return -1; }
	if (_get_message_count(&icmm, cmmpt) > 0) _remove_all_msg(&icmm);
	_close_queue(&icmm, thread);

	CMM_UNLOCK();
	return 0;
}

int cmm_send_message(CMMPORT receiver, IMSG msgid, long param, bool dyn_data, void *data)
{
	int ret;
	CMM_LOCK();
	ret = _send_message(&icmm, receiver, msgid, param, dyn_data, data);

	if (ret != 0) {//KSI_TEST DEBUG
		printf("ksi_test %s():%d RECEIVER = [0x%llx], msgid=[0x%llx]\n", __FUNCTION__, __LINE__, receiver, msgid);
	}

	iassert(ret == 0);
	CMM_UNLOCK();
	return ret;	
}
	
int cmm_resend_message(CMMPORT receiver, CMM_MESSAGE_T *rmsg)
{
	int ret;
	CMM_LOCK();

	DMSG(9, "RECEIVER = [0x%x], msgid=[0x%x]\n", receiver, rmsg->msgid);
	ret = _send_message(&icmm, receiver, rmsg->msgid, rmsg->param, rmsg->dyn_data, rmsg->data);
	iassert(ret == 0);
	rmsg->dyn_data = 0;
	CMM_UNLOCK();
	return ret;
}

int cmm_get_message(CMM_MESSAGE_T *buf)
{
	CMM_MESSAGE_T *pmsg;
	int qid;
	gboolean empty;
	CMMPORT cmmpt;

	CMM_LOCK();
	cmmpt = g_thread_self();
	qid = _find_queue(&icmm, cmmpt);
	if (qid == -1) { CMM_UNLOCK(); return -1; }

	empty = ix_queue_sync_is_empty(icmm.qi[qid].qptr);
	if (empty == TRUE) { CMM_UNLOCK(); return -1; }
	
	DMSG(9, "CMMPORT = %p, COUNT OF MESSAGE = %d\n", cmmpt, _get_message_count(&icmm, cmmpt));
	pmsg = ix_queue_sync_pop(icmm.qi[qid].qptr);
	memcpy(buf, pmsg, sizeof(CMM_MESSAGE_T));
	ifree(pmsg);
	CMM_UNLOCK();
	return 0;	
}

int cmm_take_message(CMM_MESSAGE_T *buf, CMMPORT taker)
{
	CMM_MESSAGE_T *pmsg;
	int qid;
	gboolean empty;
	CMMPORT cmmpt;

	CMM_LOCK();
	cmmpt = taker;//g_thread_self();
	qid = _find_queue(&icmm, cmmpt);
	if (qid == -1) { CMM_UNLOCK(); return -1; }

	empty = ix_queue_sync_is_empty(icmm.qi[qid].qptr);
	if (empty == TRUE) { CMM_UNLOCK(); return -1; }
	
	DMSG(9, "CMMPORT = %p, COUNT OF MESSAGE = %d\n", cmmpt, _get_message_count(&icmm, cmmpt));
	pmsg = ix_queue_sync_pop(icmm.qi[qid].qptr);
	memcpy(buf, pmsg, sizeof(CMM_MESSAGE_T));
	ifree(pmsg);
	CMM_UNLOCK();
	return 0;	
}

int cmm_peek_message(CMM_MESSAGE_T *buf, int nth)
{
	CMM_MESSAGE_T *pmsg;
	int qid;
	gboolean empty;
	CMMPORT cmmpt;
	int cnt;

	CMM_LOCK();
	cmmpt = g_thread_self();
	qid = _find_queue(&icmm, cmmpt);
	if (qid == -1) { CMM_UNLOCK(); return -1; }

	empty = ix_queue_sync_is_empty(icmm.qi[qid].qptr);
	if (empty == TRUE) { CMM_UNLOCK(); return -1; }

	cnt = ix_queue_sync_get_sizeof(icmm.qi[qid].qptr);
	if (cnt <= nth) { CMM_UNLOCK(); return -1; }
	
	DMSG(9, "CMMPORT = %p, COUNT OF MESSAGE = %d\n", cmmpt, _get_message_count(&icmm, cmmpt));
	pmsg = ix_queue_peek(icmm.qi[qid].qptr, nth);
	memcpy(buf, pmsg, sizeof(CMM_MESSAGE_T));
	CMM_UNLOCK();
	return 0;	
}

CMMPORT cmm_read_sender_cmmport(CMM_MESSAGE_T *msg)
{
	return msg->sender;
}

int cmm_get_message_count(CMMPORT cmmpt)
{
	int ret;

	CMM_LOCK();
	ret = _get_message_count(&icmm, cmmpt);
	CMM_UNLOCK();
	return ret;
}
	
int cmm_clear_queue()
{
	CMM_LOCK();
	if (_remove_all_msg(&icmm) == -1) { CMM_UNLOCK(); return -1; }
	CMM_UNLOCK();
	return 0;
}

int cmm_clear_queue_cmmpt(CMMPORT cmmpt)
{
	CMM_LOCK();
	if (_remove_all_msg_cmmpt(&icmm, cmmpt) == -1) { CMM_UNLOCK(); return -1; }
	CMM_UNLOCK();
	return 0;
}

int cmm_make_message(IMSG msgid, long param, bool dyn_data, void *data, CMM_MESSAGE_T *pmsg)
{
	memset(pmsg, 0x00, sizeof(CMM_MESSAGE_T));
	pmsg->sender = g_thread_self();
	pmsg->msgid = msgid;
	pmsg->param = param;
	pmsg->dyn_data = dyn_data;
	pmsg->data = data;
	return 0;
}

