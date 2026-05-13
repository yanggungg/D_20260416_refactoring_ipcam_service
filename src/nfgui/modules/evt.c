/*
 * evt.h
 * 	- event transfer module to the uxm
 *	- dependencies :
 *		Gtk+
 *		GdkEvent
 *
 *		NFOBJECT (specially Derived Module)
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Mar 19, 2011
 *
 */


#include "evt.h"
#include "iux_afx.h"
#include "cmm.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "ix_conf.h"
#include <memory.h>
#include "../viewers/objects/nfwindow.h"
#include "wnd.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"EVT"


#define EVT_LOCK()		g_mutex_lock(pievt->mtx)
#define EVT_UNLOCK()	g_mutex_unlock(pievt->mtx)

#define SUPPORT_GTK
#ifdef SUPPORT_GTK


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef enum _RUN_STAT_E {
	STOP	= 0,
	RUN		= 1,
} RUN_STAT_E;

typedef struct _EVT_T {
	GMutex			*mtx;
	GtkWidget 		*vwMain;
	int				count;
	GThread			*thd;
	CMMPORT			cmmpt;
	RUN_STAT_E		run;
} EVT_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static EVT_T ievt;



////////////////////////////////////////////////////////////
//
// private functions
//

#if 0
static int _is_heavy_traffic(EVT_T *pievt)
{
	int ret = 0;
	EVT_LOCK();
	if (pievt->count > 2048) {
		DMSG(1, "UX EVENT OVERFLOW..!!\n");
		ret = 1;
	}
	EVT_UNLOCK();
	return ret;
}
#endif

static int _ref(EVT_T *pievt)
{
	EVT_LOCK();
	++pievt->count;
	DMSG(8, "REF COUNT = [%d]\n", pievt->count);
	EVT_UNLOCK();
	return 0;
}

static int _unref(EVT_T *pievt)
{
	EVT_LOCK();
	--pievt->count;
	DMSG(8, "REF COUNT = [%d]\n", pievt->count);
	EVT_UNLOCK();
	return 0;
}

static int _make_event(IMSG msgid, int param, bool dyn_data, void *data)
{
	GdkEvent *evt;
	GdkWindow *dwnd;

	evt = gdk_event_new(GDK_CLIENT_EVENT);
	dwnd = ievt.vwMain->window;
	g_object_ref(dwnd);
	evt->client.window = dwnd;
	evt->client.data_format = 32; 
	evt->client.data.l[0] = msgid;
	evt->client.data.l[1] = param;
	evt->client.data.l[2] = dyn_data;
	evt->client.data.l[3] = data;

	return evt;
}

static GdkEvent *_make_event_by_msg(CMM_MESSAGE_T *pmsg)
{
	GdkEvent *evt;
	GdkWindow *dwnd;

	evt = gdk_event_new(GDK_CLIENT_EVENT);
	dwnd = ievt.vwMain->window;
	g_object_ref(dwnd);
	evt->client.window = dwnd;
	evt->client.data_format = 32; 
	evt->client.data.l[0] = pmsg->msgid;
	evt->client.data.l[1] = pmsg->param;
	evt->client.data.l[2] = pmsg->dyn_data;
	evt->client.data.l[3] = pmsg->data;

	return evt;
}

static int _put_event(EVT_T *pievt, GdkEvent *evt)
{
	_ref(pievt);
	gdk_event_put(evt);
	gdk_event_free(evt);
	return 0;
}

static int _put_event2(EVT_T *pievt, GdkEvent *evt)
{
	gdk_event_put(evt);
	gdk_event_free(evt);
	return 0;
}

static int _transfer_full(EVT_T *pievt, CMM_MESSAGE_T *pmsg)
{
	GdkEvent *evt;
	CMM_MESSAGE_T msg;

	DMSG(9, "GDK_THREAD_ENTER BEFORE\n");
	gdk_threads_enter();
	DMSG(9, "GDK_THREAD_ENTER AFTER\n");
	DMSG(2, "EVT QUEUE MSG COUNT = %d\n", cmm_get_message_count(CMMPT_EVT));

	while (1) {
		evt = _make_event_by_msg(pmsg);
		DMSG(9, "MSGID = [%x]\n", pmsg->msgid);
		_put_event(pievt, evt);
		if (cmm_get_message(&msg) != 0) break;
		pmsg = &msg;
	}
	gdk_threads_leave();
	DMSG(9, "GDK_THREAD_LEAVE\n");
	
	return 0;
}

static int _transfer_full2(EVT_T *pievt, CMM_MESSAGE_T *pmsg)
{
	GdkEvent *evt;
	CMM_MESSAGE_T msg;

	DMSG(2, "EVT QUEUE MSG COUNT = %d\n", cmm_get_message_count(CMMPT_EVT));
//	while (1) {
		evt = _make_event_by_msg(pmsg);
		DMSG(9, "MSGID = [%x]\n", pmsg->msgid);
		_put_event2(pievt, evt);
//		if (cmm_taker_message(&msg) != 0) break;
//		pmsg = &msg;
//	}
	
	return 0;
}

static int _cleanup_thread(EVT_T *pievt)
{
    if (cmm_mount_off_thread(pievt->cmmpt) == -1) return -1; 
    return 0;
}

static int _wait_run_signal(EVT_T *pievt)
{
	while (!pievt->run) usleep(10000);
	return 0;
}

static int _stop(EVT_T *pievt)
{
	pievt->run = STOP;
	return 0;
}

static int _run(EVT_T *pievt)
{
	pievt->run = RUN;
	return 0;
}

static void* _evt_service_proc(void *arg) 
{
	CMM_MESSAGE_T msg;
	EVT_T *pievt = (EVT_T *)arg;

	_wait_run_signal(pievt);
	while (1) {
		usleep(10000);
//		if (cmm_get_message(&msg) == 0) _transfer_full(pievt, &msg);

		// do not free the data of message
	}

	g_thread_exit(NULL);
}

void evt_put_to_uxm(void *arg)
{
	CMM_MESSAGE_T msg;
//	EVT_T *pievt = (EVT_T *)arg;
	CMMPORT taker = (CMMPORT)arg;

//	_wait_run_signal(pievt);
	while (1) {
//		usleep(10000);
		if (cmm_take_message(&msg, CMMPT_EVT) == 0) _transfer_full2(0, &msg);
		else break;

		// do not free the data of message
	}

	return TRUE;
}

static int _init_thread(EVT_T *pievt)
{
	pievt->thd = ifn_make_thread(_evt_service_proc, pievt);
	cmm_mount_on_thread(pievt->thd);
	pievt->cmmpt = pievt->thd;
	DMSG(1, "EVT CMMPT : [%p]\n", pievt->cmmpt);
	return 0;
}

static int _read_conf(EVT_T *pievt)
{
	int ret;
	ret = icf_get_value_by_int("evt", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int evt_init(GtkWidget *vwmain)
{
	memset(&ievt, 0x00, sizeof(EVT_T));
	_read_conf(&ievt);
	ievt.mtx = g_mutex_new();
	ievt.vwMain = vwmain;
	_init_thread(&ievt);
	_run(&ievt);
	return 0;
}

/*inline*/ CMMPORT evt_get_cmmport()
{
	return ievt.cmmpt;	
}

int evt_cleanup()
{
	g_mutex_free(ievt.mtx);
	return 0;
}

int evt_unref()
{
	_unref(&ievt);
	return 0;
}

int evt_send_to_local(IMSG msgid, long param, bool dyn_data, void *data)
{
	DMSG(9, "MSGID = [%x]\n", msgid);
	return cmm_send_message(CMMPT_EVT, msgid, param, dyn_data, data);
}



////////////////////////////////////////////////////////////
//
// derived private interfaces
//

static CMM_MESSAGE_T *_make_nfevt_msg(IMSG msgid, long param, bool dyn_data, void *data)
{
	CMM_MESSAGE_T *pmsg = imalloc(sizeof(CMM_MESSAGE_T));
	
	pmsg->msgid = msgid;
	pmsg->param = param;
	pmsg->dyn_data = dyn_data;
	pmsg->data = data;
	return pmsg;
}

////////////////////////////////////////////////////////////
//
// derived public interfaces
//

int evt_send_to_window(char *title, IMSG msgid, long param, bool dyn_data, void *data)
{
	CMM_MESSAGE_T *pevt;
	NFWINDOW *pwnd = wnd_find_window(title);
	if (!pwnd) return -1;
	pevt= _make_nfevt_msg(msgid, param, dyn_data, data);
	DMSG(9, "pevt = [%p]\n", pevt);
	return cmm_send_message(CMMPT_EVT, iNFY_NFEVT, pwnd, 1, pevt);
}

int evt_send_to_widget(NFOBJECT *widget, IMSG msgid, long param, bool dyn_data, void *data)
{
	CMM_MESSAGE_T *pevt;

	DMSG(9, "msgid = [%x]\n", msgid);
	if (widget) {
		pevt = _make_nfevt_msg(msgid, param, dyn_data, data);
		cmm_send_message(CMMPT_EVT, iNFY_NFEVT, widget, 1, pevt);
	}
	else evt_send_to_local(msgid, param, dyn_data, data);

	return 0;
}

#endif
