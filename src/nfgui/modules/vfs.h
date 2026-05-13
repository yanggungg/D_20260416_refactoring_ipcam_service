/*
 * vfs.h
 * 	- video filesystem module
 *	- dependencies :
 *			GThread
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 29, 2010
 *
 */

#ifndef __VFS_H
#define __VFS_H

#include "iux_msg.h"
#include "iux_afx.h"
#include "cmm.h"
#include "nf_network.h"
#include "nf_api_disk.h"

////////////////////////////////////////////////////////////
//
// public data type
//

typedef struct _ERASE_CHINFO_T {
	BITMASK 	chmask;
	time_t		start;
	time_t 		end;
} ERASE_CHINFO_T;

/*
 * VFS module's result
 *
 * 		- the result of processing that you ordered 
 * 		- this value will be transferred by a parameter with the complete message
 */
typedef enum _VFS_RESULT_E {
	VSR_SUCCESS					= 0x000,

	VSR_REC_STOP_FAIL			= 0x001,
	VSR_NETWORK_STOP_FAIL		= 0x002,
	VSR_FILESYSTEM_STOP_FAIL	= 0x004,

	VSR_RTL_SET_FAIL			= 0x080,
	VSR_FILESYSTEM_START_FAIL	= 0x040,
	VSR_REC_START_FAIL			= 0x020,
	VSR_NETWORK_START_FAIL		= 0x010,

	VSR_DATA_DELETE_FAIL		= 0x100,
	VSR_CHAING_WMODE_FAIL		= 0x200,
	VSR_FORMAT_FAIL				= 0x400,
} VFS_RESULT_E;


typedef enum _RS_SRVSTOP_E {
	RS_FWUP					= NF_DISCONN_SVR_FW_UPGRADE,
	RS_FACDEF				= NF_DISCONN_SVR_FACTORY_DEFAULT,
	RS_TIMECHANGE			= NF_DISCONN_SVR_TIME_CHANGE,
	RS_DBIMPORT				= NF_DISCONN_SVR_SYSDB_LOAD,
	RS_IPCHANGE				= NF_DISCONN_SVR_IP_CHANGE,
	RS_FORMAT				= NF_DISCONN_SVR_DISK_FORMAT,
	RS_SHUTDOWN				= NF_DISCONN_SVR_POWER_OFF,
	RS_CAM_FWUP				= NF_DISCONN_SVR_FW_UPGRADE,
	RS_DISK					= NF_DISCONN_SVR_DISK_MANAGEMENT,
	RS_ERASE_CH				= NF_DISCONN_SVR_DISK_MANAGEMENT,
} RS_SRVSTOP_E;

/*
 * progressing message (JUST notification)
 *
 * 		- the follwing messagewill be transferred to the orderer by itself
 *
 *
	INFY_FS_START_BEGIN:
	INFY_FS_STOP_BEGIN:
	INFY_DATA_DELETE_BEGIN:
	INFY_RTL_SET_BEGIN:
	INFY_FORMAT_BEGIN:
	INFY_DATA_DELETE_RATE:
	INFY_RTL_SET_RATE:
	INFY_FORMAT_RATE:

 */ 		





////////////////////////////////////////////////////////////
//
// public interfaces
//


int vfs_start_fs(CMMACK_T *pcmmack);
int vfs_stop_fs(CMMACK_T *pcmmack);
int vfs_set_rtl(CMMACK_T *pcmmack, unsigned int rtl);
int vfs_delete_data(CMMACK_T *pcmmack, time_t delete_time);
int vfs_format_storage(CMMACK_T *pcmmack, int mode);
int vfs_stop_network(CMMACK_T *pcmmack, int reason);
int vfs_start_network(CMMACK_T *pcmmack);
int vfs_start_record(CMMACK_T *pcmmack);
int vfs_stop_record(CMMACK_T *pcmmack);
int vfs_erase_ch(CMMACK_T *pcmmack, ERASE_CHINFO_T *pinfo);

/*inline*/ CMMPORT vfs_get_cmmport();
int vfs_filesystem_get_wmode(NF_DISK_WRITE_MODE_E *w_mode);
int vfs_init();
int vfs_is_running();
gboolean vfs_filesystem_is_online();
gboolean vfs_filesystem_change_wmode(unsigned int wmode);
gboolean vfs_start_panic_record();
gboolean vfs_stop_panic_record();
gboolean vfs_toggle_panic_record();
gboolean vfs_is_stopped();
int vfs_stop_urgent();

#endif
