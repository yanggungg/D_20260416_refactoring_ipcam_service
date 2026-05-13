/*
 * cdump.c
 *  - camera packet module
 *  - dependency : scm, mda
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Oct 2, 2015
 *
 */

#include "scm.h"
#include "wrk.h"

#include "iux_afx.h"
#include "ix_mem.h"
#include "cdump.h"


//#define TEST_USB_MOUNT

#define DBG_LEVEL		1
#define DBG_MODULE		"CDUMP"


////////////////////////////////////////////////////////////
//
// private data type 
//


////////////////////////////////////////////////////////////
//
// private variable
//

static CDUMP_T icdump;
static WRK_ID iwrk = 0;
static DUMP_STATUS d_status;
static gchar usbtitle[128];

////////////////////////////////////////////////////////////
//
// private functions
//

static int _is_exist_id(MEDIA_INFO_T *info, int cnt, MEDIA_ID id)
{
    int i;

    if (!cnt) return 0;

	for (i = 0; i < cnt; ++i) 
	{		
        if (info[i].id == id) return 1;
	}

    return 0;
}

static int _is_exist_file(MEDIA_INFO_T *info, int cnt, MEDIA_ID *id)
{
    char file_path[MAX_PATH_LEN + 1];
    int i;

    DMSG(1, "");
    if (!cnt) return 0;
    DMSG(1, "");
    for (i = 0; i < cnt; ++i) 
    {
    	if (mda_get_mounted_path(info[i].id, file_path, MAX_PATH_LEN) == 0)
    	{
#ifndef TEST_USB_MOUNT
DMSG(1, "");
            if (nf_ipcam_packet_dump_usb_check(file_path) == 0)
            {
                DMSG(1, "");
                *id = info[i].id;
                return 1;
            }
#else    	
            strcat(file_path, "/dump_config.txt");
            if (ifn_is_file_exist(file_path) == 1)
            {
                *id = info[i].id;
                return 1;
            }
#endif            
    	}
    }

    return 0;
}

static int _proc_get_dump_status(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    DUMP_STATUS status;
    g_message("%s, %d, __called", __FUNCTION__, __LINE__);
    memset(&status, 0x00, sizeof(DUMP_STATUS));
    
    nf_ipcam_packet_dump_get_status(&status);
    g_message("%s, %d, d_status.status : %d __called", __FUNCTION__, __LINE__, status.status);
    g_message("%s, %d, d_status.size : %lu __called", __FUNCTION__, __LINE__, status.size);
    g_message("%s, %d, d_status.used : %lu __called", __FUNCTION__, __LINE__, status.used);
    g_message("%s, %d, d_status.avail : %lu __called", __FUNCTION__, __LINE__, status.avail);
    memmove(&d_status, &status, sizeof(DUMP_STATUS));
    
    return 0;
}

static int _update_media_id()
{
    MEDIA_INFO_T *info;
    MEDIA_ID id;
    int i, cnt;

    info = scm_new_media_list(&cnt);

    if (icdump.id > 0) 
    {
        if (_is_exist_id(info, cnt, icdump.id) == 0) 
        {
            g_message("%s, %d, packet dump remove media", __FUNCTION__, __LINE__);
        
            nf_ipcam_packet_dump_usb_remove();
            evt_send_to_local(INFY_PACKET_DUMP_REMOVE_MEDIA, 0, 0, 0);            
            icdump.id = 0;

            if (iwrk)
            {
                wrk_destroy_worker(iwrk);
                iwrk = 0;
            }
            memset(&d_status, 0x00, sizeof(DUMP_STATUS));
        }
    }
    else
    {
        if (_is_exist_file(info, cnt, &id) == 1) 
        {
            g_message("%s, %d, packet dump insert media", __FUNCTION__, __LINE__);

            evt_send_to_local(INFY_PACKET_DUMP_INSERT_MEDIA, 0, 0, 0);            
            icdump.id = id;
            sprintf(usbtitle, info->title);
        }
    }

	if (info) scm_free_media_list(info);

    return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int cdump_init()
{
    memset(&icdump, 0x00, sizeof(CDUMP_T)); 
    memset(&d_status, 0x00, sizeof(DUMP_STATUS));
    iwrk = 0;
        
    return 0;
}

int cdump_change_media()
{
    _update_media_id();
    return 0;
}

int cdump_packet_dump_start()
{
    nf_ipcam_packet_dump_start();
    
	iwrk = wrk_create_worker(_proc_get_dump_status, 0);
	wrk_change_sleep_time(iwrk, 1000*1000*1);
    wrk_run_loop(iwrk, IMSG_NONE, 0, 0, 0);
    
    return 0;
}
 
int cdump_packet_dump_pause()
{

    return 0;
}

int cdump_packet_dump_resume()
{

    return 0;
}

int cdump_packet_dump_stop()
{
    nf_ipcam_packet_dump_stop();
    wrk_destroy_worker(iwrk);
    iwrk = 0;

    memset(&d_status, 0x00, sizeof(DUMP_STATUS));
    
    return 0;
}

int cdump_packet_dump_get_status(DUMP_STATUS *status)
{
    memmove(status, &d_status, sizeof(DUMP_STATUS));
    
    return 0;
}

int cdump_packet_dump_get_usb_title(gchar *buf)
{
    sprintf(buf, usbtitle);
    
    return 0;
}

