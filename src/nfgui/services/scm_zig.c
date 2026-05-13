/*
 * scm_zig.c
 * 	- system 
 *	- dependencies :
 *		
 *
 * Written by Jungkyu PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Dec 24, 2012
 *
 */

#include <glib.h>
#include "iux_afx.h"
#include "scm_internal.h"
#include "scm.h"
#include "vfs.h"
#include "nf_util_device.h"
#include "evt.h"
#include "var.h"
#include "ix_mem.h"
#include "nf_api_ipcam.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_ZIG"



////////////////////////////////////////////////////////////
//
// private functions
//

static int _proc_get_ipcam_zig_info(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
    int i;
    NFIPCamMFInfo *info;

    g_message("%s, %d", __FUNCTION__, __LINE__);

    for (i = 0; i < var_get_ch_count(); i++)
    {
        info = imalloc(sizeof(NFIPCamMFInfo));
        nf_ipcam_get_mf_info(i, info);
		evt_send_to_local(INFY_VWND_IPCAM_ZIG_INFO, i, 0, info);        
    }

	return 0;
}



////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_start_ipcam_zig(SCM_T *piscm)
{
    WRK_ID iwrk = 0;        

	iwrk = wrk_create_worker(_proc_get_ipcam_zig_info, 0);
	wrk_change_sleep_time(iwrk, 3000000);
	wrk_run_loop(iwrk, IMSG_NONE, piscm, 0, 0);
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected handlers
//





////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_req_ipcam_mf_info()
{
    int i;
    NFIPCamMFInfo *info;

    for (i = 0; i < var_get_ch_count(); i++)
    {
        info = imalloc(sizeof(NFIPCamMFInfo));
        nf_ipcam_get_mf_info(i, info);
		evt_send_to_local(INFY_VWND_IPCAM_ZIG_INFO, i, 0, info);        
    }

	return 0;
}

int scm_start_ipcam_scan()
{
    nf_ipcam_scan_start();
    return 0;
}

int scm_sync_ipcam_time_info()
{
    int i;

    for (i = 0; i < var_get_ch_count(); i++)
        nf_ipcam_set_time_info(i);
        
    return 0;
}


int scm_work_ipcam_done()
{
    nf_ipcam_works_done();
    return 0;
}

