/*
 * iux_afx.h
 * 	- iux framework
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 7, 2010
 *
 */

#ifndef	__IUX_AFX_H
#define __IUX_AFX_H

#include <glib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "iux_types.h"
#include "modules/cmm.h"
#include "tools/ix_mem.h"

#include "nvm.h"

#include "vw.h"
// #include "nf_webra.h"

/*
 * if DBG_LEVEL is 0, all messages in the file are not displayed
 * if level is 0, the message is not displayed
 * if level is greater than DBG_LEVEL, the message is not displayed
 */
#define DMSG(level, format, args...) \
	do { \
		if (DBG_LEVEL && level && DBG_LEVEL >= level) { \
			fprintf(stderr, "[IUX:"DBG_MODULE"] %s():%d: "format"\n", __FUNCTION__, __LINE__, ##args); \
		} \
	} while (0)

#define DECLARE
#define DBG_SYSTEM	static int dmsg = 2;
#define DBG_CONF	dmsg
#define DBG_USE(d)	dmsg = d;

extern CALLID  web_uxm_get_callid();
extern CMMPORT web_uxm_get_cmmport();
extern CMMPORT nvm_get_cmmport();
extern GThread *uxm_get_callid();

#define NF_TOPWND		0
#define IUX_CALLER()	g_thread_self()

#define LOCAL_CALL	uxm_get_callid()
#define WEB_CALL	web_uxm_get_callid()
#define NVM_CALL    nvm_get_callid()

#define CMMPT_SCM	scm_get_cmmport()
#define	CMMPT_EVT	evt_get_cmmport()
#define	CMMPT_NVM	nvm_get_cmmport()

#define CMMSUPPORT_WEB
#define CMMPT_WEB	web_uxm_get_cmmport()

#define IS_PAL      (DISPLAY_IS_PAL)

#define IUX_MAX_NAME_LEN	127
#define IUX_MAX_PATH_LEN	255
#define IUX_MAX_EMAIL_INFO	511
#define LEN_USERID			31

#define INVALID				0



///////////////////////////////////////////////////////////////////
//
// function declaration keyword
//
//

// APIs for application
#define IUXAPI

// Callback function which is called another thread
#define CALLBACK

// Message Handler which is called when a message is arrived
#define HANDLER


/*
 * TRASANSATION is a sequenctial operations for some service
 *
 */
typedef enum _TRANSACTION_E {
	TRA_NONE				= 0,
	TRA_FW_UPGRADE			= 1,
	TRA_FACTORY_DEFAULT		= 2,
	TRA_TIME_CHANGE			= 3,
	TRA_FORMAT				= 4,
	TRA_RTL_SET				= 5,
	TRA_ARCHIVING			= 6,
	TRA_DB_IMPORT			= 7,
	TRA_DB_EXPORT			= 8,
	TRA_BOOTUP_SYSTEM		= 9,
	TRA_GET_WAN_IP			= 10,
	TRA_VERIFY_ARCH			= 11,
	TRA_UNC_CHECK			= 12,
	TRA_RESTART_SERVICE		= 13,
	TRA_IP_CHANGE			= 14,
	TRA_SHUTDOWN			= 15,
	TRA_REG_RTSPPORT		= 16,
	TRA_RMV_RTSPPORT		= 17,
	TRA_TST_RTSPPORT		= 18,
	TRA_REG_WEBPORT			= 19,
	TRA_RMV_WEBPORT			= 20,
	TRA_TST_WEBPORT			= 21,
	TRA_REG_DDNS			= 22,
	TRA_TST_DDNS			= 23,
	TRA_CAPTURE				= 24,
	TRA_IPCAM_FWUP_MODE		= 25,
	TRA_DESIGN_UP			= 27,
	TRA_INV_BOARD			= 28,
	TRA_ENTER_DIRMODE       = 29,
	TRA_LEAVE_DIRMODE       = 30,
	TRA_GET_DDNS_STATUS		= 31,
	TRA_ERASE_CH			= 32,
	TRA_NET_FW_UPGRADE		= 33,
	TRA_OPENMODE_CAM_SETUP	= 34,
	TRA_TST_FTP				= 35,
	TRA_DIAGNOSIS_IPCAM_NET	= 36,
	TRA_DIAGNOSIS_IPCAM_POWER	= 37,
	TRA_DIAGNOSIS_DISK			= 38,
	TRA_DIAGNOSIS_PORT			= 39,
	TRA_CAMFW_UPGRADE			= 40,
	TRA_NET_CAMFW_UPGRADE		= 41,
	TRA_CREATE_RAID			= 42,
	TRA_DELETE_RAID			= 43,
	TRA_JM_UPDATE			= 44,
	TRA_CABLE_CHECK         = 45,
	TRA_DISK_INFO           = 46,
	TRA_APPQC_TEST			= 47,
	TRA_FWUP_VALIDATE		= 48,
	TRA_FWUP_DATABACKUP		= 49,
	TRA_GET_LICENSE		    = 50,
	TRA_REG_UNIMO		    = 51,
	TRA_SSL_INSTALL		    = 52,
	TRA_SSL_DELETE		    = 53,
	TRA_EXPORT_DEBUG_DATA	= 54,

	TRA_MAX
} TRANSACTION_E;


typedef enum _SYSDB_SUB_CATE_E {
	SYSDB_SUB_CATE_CAM = 0x1000,
	SYSDB_SUB_CATE_CAM_AI,
	SYSDB_SUB_CATE_DISP = 0x2000,
	SYSDB_SUB_CATE_AUDIO = 0x4000,
	SYSDB_SUB_CATE_USR = 0x8000,
	SYSDB_SUB_CATE_NET = 0x10000,
	SYSDB_SUB_CATE_SYS = 0x20000,
	SYSDB_SUB_CATE_DISK = 0x40000,
	SYSDB_SUB_CATE_EVENT = 0x80000,	
} SYSDB_SUB_CATE_E;


////////////////////////////////////////////////////////////
//
// public interfaces
//

int start_iux(int ch_count, int skip_sst_init);
int is_iux_initialized(void);

const char *iux_translate_msg_desc(guint msg_enum);
guint iux_translate_msg_enum(const char *msg_desc);

#endif
