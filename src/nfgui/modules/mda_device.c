/*
 * msm_dsr.c
 * 	- device status reporter
 *	- dependencies :
 *			GThread
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 9, 2011
 *
 */

#include <stdio.h>
#include "cmm.h"
#include "mda.h"
#include "mda_internal.h"
#include <memory.h>
#include "iux_afx.h"
#include "log.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"MDA_DR"


#define DSR_LOCK()		g_mutex_lock(idsr.mtx)
#define DSR_UNLOCK()	g_mutex_unlock(idsr.mtx)
#define PDSR_LOCK()		g_mutex_lock(pidsr->mtx)
#define PDSR_UNLOCK()	g_mutex_unlock(pidsr->mtx)



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _DSR_T {
	GMutex				*mtx;

	DEVICE_REPORT_T		dreport_in;
	DEVICE_REPORT_T		dreport_ex;

} DSR_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static DSR_T idsr;


////////////////////////////////////////////////////////////
//
// private functions
//

static void _cb_change_device_info(int result, void *context)
{
	DSR_T *pidsr = (DSR_T *)context;
	cmm_send_message(mda_get_cmmport(), iRET_NF_ARCH_DEV_SET_NOTIFY, result, 0, 0);
}

static int _dbgprint_device_info(DEVICE_REPORT_T *pdreport)
{
	int i;

	DMSG(1, "device count:%d\n", pdreport->cnt_dinfo);
	for(i = 0; i < pdreport->cnt_dinfo; i++) {
		DMSG(1, "\n######## DEVICE ########\n");
		DMSG(1, "dev id [%d]\n", pdreport->xinfo[i].dev_id);
		DMSG(1, "dev type [%d]\n", pdreport->xinfo[i].dev_type);
		DMSG(1, "device_checked [%d]\n", pdreport->xinfo[i].media_checked);
		DMSG(1, "device_size [%llu]\n", pdreport->xinfo[i].media_size);
		DMSG(1, "device avail [%llu]\n", pdreport->xinfo[i].media_avail);
		DMSG(1, "vendor [%s]\n", pdreport->xinfo[i].vendor);
		DMSG(1, "product [%s]\n", pdreport->xinfo[i].product);
		DMSG(1, "dev name [%s]\n", pdreport->xinfo[i].dev_name);
		DMSG(1, "rev [%d]\n", pdreport->xinfo[i].rev);
	}

	return 0;
}

static void _cb_read_dev_info(gint result, gpointer context)
{
	DSR_T *pidsr = (DSR_T *)context;
	cmm_send_message(mda_get_cmmport(), iRET_NF_ARCH_DEV_GET_SIZE, result, 0, 0);	
}

static int _set_device_count(DEVICE_REPORT_T *pin, int count)
{
	pin->cnt_dinfo = count;
	return 0;
}

static int _update_ex_device_report(DSR_T *pidsr)
{
	int diff;
	PDSR_LOCK();
	diff = pidsr->dreport_in.cnt_dinfo - pidsr->dreport_ex.cnt_dinfo;
	memcpy(&pidsr->dreport_ex, &pidsr->dreport_in, sizeof(DEVICE_REPORT_T));
	PDSR_UNLOCK();

	return diff;
}

static int _register_callback()
{
	nf_arch_dev_set_notify(_cb_change_device_info, &idsr, NULL);
	return 0;
}

static int _init_mutex(DSR_T *pidsr)
{
	pidsr->mtx = g_mutex_new();
	return 0;
}

static int _cleanup_mutex(DSR_T *pidsr)
{
	g_mutex_free(pidsr->mtx);
	return 0;
}

static int _preprocess_device_info(DEVICE_REPORT_T *pdreport)
{
	int i;

	for(i = 0; i < pdreport->cnt_dinfo; i++) {
		ifn_strtrim(pdreport->xinfo[i].vendor);
		ifn_strtrim(pdreport->xinfo[i].product);
		ifn_strtrim(pdreport->xinfo[i].dev_name);
	}

	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _mda_init_dsr()
{
	memset(&idsr, 0x00, sizeof(DSR_T));

	_init_mutex(&idsr);
	_register_callback();

	return 0;
}

int _mda_cleanup_dsr()
{
	_cleanup_mutex(&idsr);
	return 0;
}

int _mda_request_device_info()
{
	memset(&idsr.dreport_in, 0x00, sizeof(DEVICE_REPORT_T));

	nf_arch_dev_get_size(idsr.dreport_in.xinfo, _cb_read_dev_info, &idsr, NULL);
	return 0;
}

int _mda_reload_device_data(int cnt)
{
	int diff, i;
	
	if (cnt > 0) _set_device_count(&idsr.dreport_in, cnt);
	else _set_device_count(&idsr.dreport_in, 0);

	_preprocess_device_info(&idsr.dreport_in);

	// just for debugging
	_dbgprint_device_info(&idsr.dreport_in);

	diff = _update_ex_device_report(&idsr);

	if (diff > 0) {
	    for (i = 0; i < diff; i++)
    	    scm_put_log(STORAGE_PLUG, 0, 0);
	}
	else if (diff < 0) {
	    for (i = 0; i < -(diff); i++)
    	    scm_put_log(STORAGE_UNPLUG, 0, 0);
	}
	
	return diff;
}

int _mda_get_device_report(DEVICE_REPORT_T *dr, int *cnt)
{
	DSR_LOCK();
	memcpy(dr, &idsr.dreport_ex, sizeof(DEVICE_REPORT_T));
	*cnt = idsr.dreport_ex.cnt_dinfo;
	DSR_UNLOCK();
	return 0;
}

