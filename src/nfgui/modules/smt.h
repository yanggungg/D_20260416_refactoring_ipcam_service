/*
 * smt.h
 * 	- service monitor modules
 *	- dependencies :
 *
 * Written by JUNGKYU PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Sep 07, 2011
 *
 */

#ifndef __SMT_H
#define __SMT_H

#include "iux_afx.h"


typedef enum _SMT_SERVICE_E {
	SMT_UNDEF				= -1,
	SMT_INIT				= 0,
	SMT_LIVE				= 1,
	SMT_LIVE_PTZ			= 2,
	SMT_LIVE_ZOOM			= 3,
	SMT_LIVE_SNAPSHOT		= 4,
	SMT_LIVE_BURN			= 5,
	SMT_PLAYBACK			= 6,
	SMT_PLAYBACK_ZOOM		= 7,
	SMT_PLAYBACK_SNAPSHOT	= 8,	
	SMT_PLAYBACK_BOOKMARK	= 9,	
	SMT_ARCH_PLAY			= 10,
	SMT_SYSTEM_SETUP		= 11,
	SMT_RECORD_SETUP		= 12,	
	SMT_ARCHIVE				= 13,
	SMT_ARCHIVE_BURN		= 14,
	SMT_SEARCH				= 15,
	SMT_SHUTDOWN			= 16,
	SMT_AUTO_SYNC			= 17,
	SMT_LOGOUT				= 18,

	SMT_TIME_CHANGE			= 19,
	SMT_SYSDB_LOAD			= 20,
	SMT_FORMAT				= 21,
	SMT_FW_UPGRADE			= 22,
	SMT_ARCHIVE_QUERY		= 23,
	SMT_FAC_DEFAULT			= 24,
	SMT_DISK_RESTART		= 25,
	SMT_LIVE_OSDPOPUP		= 26,
	SMT_CAMFW_UPGRADE		= 27,
	SMT_CAM_DCONFIG		    = 28,
	SMT_NET_FW_UPGRADE		= 29,
    SMT_MAIN_MENU           = 30,
	SMT_CAM_INSTALL_MODE	= 31,
	
	// if add something, please add the _act_proc list in smt.c
	//
	SMT_DUMMY,
	SMT_SERVICE_CNT,
} SMT_SERVICE_E;







////////////////////////////////////////////////////////////
//
// public functions
//

gint smt_init();
gint smt_set_service(SMT_SERVICE_E service);
gint smt_return_to_previous();
SMT_SERVICE_E smt_get_service();
#endif

