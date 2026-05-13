/*
 * smt.c
 * 	- service monitor modules
 *	- dependencies :
 *
 * Written by JUNGKYU PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Sep 07, 2011
 *
 */


#include "smt.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nf_sysman.h"
#include "nf_notify.h"
#include <memory.h>
#include "ssm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SMT"


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _SMT_T {
	unsigned long current;
	unsigned long previous;
} SMT_T;

typedef enum _ACT_E {
	LEAVE	= 0,
	ENTER	= 1,
} ACT_E;


////////////////////////////////////////////////////////////
//
// prototype of private funtion
//

static int _act_print(SMT_SERVICE_E service, ACT_E act);
static int _act_live(SMT_SERVICE_E service, ACT_E act);
static int _act_live_ptz(SMT_SERVICE_E service, ACT_E act);
static int _act_live_zoom(SMT_SERVICE_E service, ACT_E act);
static int _act_live_snapshot(SMT_SERVICE_E service, ACT_E act);
static int _act_live_burn(SMT_SERVICE_E service, ACT_E act);
static int _act_playback(SMT_SERVICE_E service, ACT_E act);
static int _act_playback_zoom(SMT_SERVICE_E service, ACT_E act);
static int _act_playback_snapshot(SMT_SERVICE_E service, ACT_E act);
static int _act_playback_bookmark(SMT_SERVICE_E service, ACT_E act);
static int _act_arch_play(SMT_SERVICE_E service, ACT_E act);
static int _act_system_setup(SMT_SERVICE_E service, ACT_E act);
static int _act_record_setup(SMT_SERVICE_E service, ACT_E act);
static int _act_archive(SMT_SERVICE_E service, ACT_E act);
static int _act_archive_burn(SMT_SERVICE_E service, ACT_E act);
static int _act_search(SMT_SERVICE_E service, ACT_E act);
static int _act_shutdown(SMT_SERVICE_E service, ACT_E act);
static int _act_auto_sync(SMT_SERVICE_E service, ACT_E act);
static int _act_logout(SMT_SERVICE_E service, ACT_E act);
static int _act_time_change(SMT_SERVICE_E service, ACT_E act);
static int _act_sysdb_load(SMT_SERVICE_E service, ACT_E act);
static int _act_format(SMT_SERVICE_E service, ACT_E act);
static int _act_fw_upgrade(SMT_SERVICE_E service, ACT_E act);
static int _act_archive_query(SMT_SERVICE_E service, ACT_E act);
static int _act_fac_default(SMT_SERVICE_E service, ACT_E act);
static int _act_disk_restart(SMT_SERVICE_E service, ACT_E act);
static int _act_live_osdpopup(SMT_SERVICE_E service, ACT_E act);
static int _act_camfw_upgrade(SMT_SERVICE_E service, ACT_E act);
static int _act_cam_dconfig(SMT_SERVICE_E service, ACT_E act);
static int _act_net_fw_upgrade(SMT_SERVICE_E service, ACT_E act);
static int _act_main_menu(SMT_SERVICE_E service, ACT_E act);
static int _act_cam_install_mode(SMT_SERVICE_E service, ACT_E act);



////////////////////////////////////////////////////////////
//
// private variable
//

static SMT_T ismt;

static int (*_act_proc[SMT_SERVICE_CNT])(SMT_SERVICE_E service, ACT_E act) = {
	0, 							// 	SMT_INIT
	_act_live,			 		// 	SMT_LIVE
	_act_live_ptz, 				//	SMT_LIVE_PTZ
	_act_live_zoom, 			//	SMT_LIVE_ZOOM
	_act_live_snapshot, 		//	SMT_LIVE_SNAPSHOT
	_act_live_burn, 			//	SMT_LIVE_BURN
	_act_playback, 				//	SMT_PLAYBACK
	_act_playback_zoom,		 	//	SMT_PLAYBACK_ZOOM
	_act_playback_snapshot, 	//	SMT_PLAYBACK_SNAPSHOT
	_act_playback_bookmark, 	//	SMT_PLAYBACK_BOOKMARK
	_act_arch_play, 			//	SMT_ARCH_PLAY
	_act_system_setup, 			//	SMT_SYSTEM_SETUP
	_act_record_setup, 			//	SMT_RECORD_SETUP
	_act_archive,				// 	SMT_ARCHIVE
	_act_archive_burn, 			//	SMT_ARCHIVE_BURN
	_act_search, 				//	SMT_SEARCH
	_act_shutdown,			 	//	SMT_SHUTDOWN
	_act_auto_sync, 			//	SMT_AUTO_SYNC
	_act_logout, 				//	SMT_LOGOUT
	_act_time_change, 			//	SMT_TIME_CHANGE
	_act_sysdb_load, 			//	SMT_SYSDB_LOAD
	_act_format, 				//	SMT_FORMAT
	_act_fw_upgrade, 			//	SMT_FW_UPGRADE
	_act_archive_query, 		//	SMT_ARCHIVE_QUERY
	_act_fac_default, 			//	SMT_FAC_DEFAULT
	_act_disk_restart,			// 	SMT_DISK_RESTART
	_act_live_osdpopup,			// 	SMT_LIVE_OSDPOPUP
	_act_camfw_upgrade,			// 	SMT_CAMFW_UPGRADE
	_act_cam_dconfig,			// 	SMT_CAM_DCONFIG	
	_act_net_fw_upgrade,		// 	SMT_NET_FW_UPGRADE
	_act_main_menu,		        // 	SMT_MAIN_MENU
	_act_cam_install_mode,		// 	SMT_CAM_INSTALL_MODE
};

////////////////////////////////////////////////////////////
//
// private functions
//

static guint _translate_notify_index(SMT_SERVICE_E service)
{
	guint index;

	switch(service)
	{
		case SMT_INIT:
			index = NF_DVR_STATUS_INIT;
			break;

		case SMT_LIVE_OSDPOPUP:
		case SMT_LIVE:
			index = NF_DVR_STATUS_LIVE;
			break;		

		case SMT_LIVE_PTZ:			
			index = NF_DVR_STATUS_PTZ;		
			break;		

		case SMT_LIVE_ZOOM:			
			index = NF_DVR_STATUS_ZOOM;		
			break;		

		case SMT_LIVE_SNAPSHOT:			
			index = NF_DVR_STATUS_LIVE;		
			break;		

		case SMT_LIVE_BURN:			
			index = NF_DVR_STATUS_LIVE_RUN_ARCHIVE;		
			break;		

		case SMT_PLAYBACK:			
			index = NF_DVR_STATUS_RUN_PLAYBACK;		
			break;		

		case SMT_PLAYBACK_ZOOM:			
			index = NF_DVR_STATUS_ZOOM;		
			break;		

		case SMT_PLAYBACK_SNAPSHOT:		
			index = NF_DVR_STATUS_RUN_PLAYBACK;		
			break;		

		case SMT_PLAYBACK_BOOKMARK:		
			index = NF_DVR_STATUS_RUN_PLAYBACK;
			break;		

		case SMT_ARCH_PLAY:			
			index = NF_DVR_STATUS_RUN_PLAYBACK;	
			break;		

		case SMT_FAC_DEFAULT:
		case SMT_DISK_RESTART:
		case SMT_TIME_CHANGE:
		case SMT_SYSDB_LOAD:
		case SMT_FORMAT:	
		case SMT_FW_UPGRADE:
		case SMT_SYSTEM_SETUP:		
		case SMT_MAIN_MENU:
		case SMT_CAM_DCONFIG:
			index = NF_DVR_STATUS_SETUP;		
			break;		

		case SMT_RECORD_SETUP:			
			index = NF_DVR_STATUS_SETUP;		
			break;		

		case SMT_ARCHIVE_QUERY:
		case SMT_ARCHIVE:			
			index = NF_DVR_STATUS_ARCHIVE;		
			break;				

		case SMT_ARCHIVE_BURN:			
			index = NF_DVR_STATUS_RUN_ARCHIVE;		
			break;		

		case SMT_SEARCH:			
			index = NF_DVR_STATUS_PLAYBACK;	
			break;			

		case SMT_SHUTDOWN:			
			index = NF_DVR_STATUS_SHUTDOWN;		
			break;		

		case SMT_AUTO_SYNC:			
			index = NF_DVR_STATUS_LIVE_AUTO_SYNC;		
			break;		

		case SMT_LOGOUT:
			index = NF_DVR_STATUS_LIVE;	
			break;		

		case SMT_CAMFW_UPGRADE:
			index = NF_DVR_STATUS_LIVE;	
			break;		

		case SMT_NET_FW_UPGRADE:
			index = NF_DVR_STATUS_LIVE;	
			break;		
		case SMT_CAM_INSTALL_MODE:
			index = NF_DVR_STATUS_LIVE;	
			break;					
	}

	return index;
}

static gint _send_notify(guint index)
{
	if (nf_notify_fire_params("dvr_status", index, 0, 0, 0))
		return 0;
		
	return -1;
}

static int _run_leave_proc(SMT_SERVICE_E service)
{
	if (service == SMT_UNDEF) return -1;
	if (_act_proc[service]) _act_proc[service](service, LEAVE);
	return 0;
}

static int _run_enter_proc(SMT_SERVICE_E service)
{
	if (service == SMT_UNDEF) return -1;
	DMSG(1, "service = %d\n", service);
	if (_act_proc[service]) _act_proc[service](service, ENTER);
	return 0;
}

static int _act_print(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) 
		DMSG(1, "ENTER TO SERVICE, (%d)", service);
	else
		DMSG(1, "LEAVE FROM SERVICE (%d)", service);

	return 0;
}

static int _act_live(SMT_SERVICE_E service, ACT_E act)
{
	static gint check_s1_remotefw = 1;

	if (act == ENTER) {
		ssm_start_auto_logout();
		nf_sysman_bdflush();
	    	evt_send_to_local(INFY_CHECK_REMOTE_NEWFW, 0, 0, 0);

		if (check_s1_remotefw) 
		{
		    	evt_send_to_local(INFY_CHECK_REMOTE_S1_NEWFW, 0, 0, 0);
		    	check_s1_remotefw = 0;
		}
	}

	return 0;
}

static int _act_live_ptz(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_live_zoom(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_live_snapshot(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_live_burn(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_playback(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_playback_zoom(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_playback_snapshot(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_playback_bookmark(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_arch_play(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_system_setup(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_start_auto_logout();

	return 0;
}

static int _act_record_setup(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_start_auto_logout();

	return 0;
}

static int _act_archive(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_start_auto_logout();

	return 0;
}

static int _act_archive_burn(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_search(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_start_auto_logout();

	return 0;
}

static int _act_shutdown(SMT_SERVICE_E service, ACT_E act)
{

	return 0;
}

static int _act_auto_sync(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_logout(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_time_change(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_sysdb_load(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_format(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_fw_upgrade(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_archive_query(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_fac_default(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_disk_restart(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_live_osdpopup(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_camfw_upgrade(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_cam_dconfig(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_net_fw_upgrade(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

static int _act_main_menu(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_start_auto_logout();

	return 0;
}

static int _act_cam_install_mode(SMT_SERVICE_E service, ACT_E act)
{
	if (act == ENTER) ssm_stop_auto_logout();

	return 0;
}

////////////////////////////////////////////////////////////
//
// public functions
//

gint smt_init()
{
	memset(&ismt, 0x00, sizeof(SMT_T));
	ismt.previous = SMT_UNDEF;
	ismt.current = SMT_UNDEF;
	return 0;
}


gint smt_set_service(SMT_SERVICE_E new)
{
	guint pre_index, post_index;

	DMSG(9, "");
	if (ismt.current == new) return -1;
	DMSG(9, "new = %d", new);
	ismt.previous = ismt.current;
	ismt.current = new;

	_run_leave_proc(ismt.previous);
	_run_enter_proc(ismt.current);

	pre_index = _translate_notify_index(ismt.previous);
	post_index = _translate_notify_index(ismt.current);
	DMSG(1, "NF DVR STATUS = %d\n", post_index);
	if (pre_index == post_index) return -1;
	_send_notify(post_index);

	return 0;
}

gint smt_return_to_previous()
{
	smt_set_service(ismt.previous);
	return 0;
}

SMT_SERVICE_E smt_get_service()
{
	guint dvr_status = 0;

	dvr_status = nf_notify_get_param0("dvr_status");	
//	DMSG(1, "notify_dvr_status:%d, current:%d", dvr_status, ismt.current);

	return ismt.current;
}

