/*
 * vw.h
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jun 14, 2011
 *
 */

#ifndef __VW_H
#define __VW_H

#include "vw_desc.h"
#include "objects/nfwindow.h"
#include "objects/nfobject.h"
#include "tools/nf_ui_tool.h"

////////////////////////////////////////////////////////////
//
// global constant
//

extern const gchar *g_msg_str[];

typedef enum _IMBXSTR_E {
	IMBX_DEFULT					= 0,
	IMBX_CONF_SAVED				= 1,	
	IMBX_BOOT_DISK_ERROR		= 2,
	IMBX_RECOVERY_TIMEOUT		= 3,
	IMBX_RECOVER_FORMAT			= 4,
	IMBX_FAIL_DB_SAVE			= 5,
	IMBX_FAIL_NTP				= 6,
	IMBX_AUTO_TIME_SYNC			= 7,
	IMBX_IPCAM_CALI_FAIL		= 8,
	IMBX_IPCAM_CALI_TIMEOUT		= 9,
	IMBX_INVALID_AVIFILE		= 10,
	IMBX_REMOTE_DISCONNECT		= 11,
	IMBX_IPCAM_ONEPUSH_FAIL		= 12,
	IMBX_IPCAM_ONEPUSH_TIMEOUT	= 13,
	IMBX_ENFORCE_FORMAT			= 14,
	IMBX_SMART_ERROR			= 15,
	IMBX_SMART_WARNING			= 16,
	IMBX_REBOOT_CAMERA_TYPE_CHANGED	= 17,
	IMBX_INVALID_NUMBER_HIGH	= 18,
	IMBX_INVALID_NUMBER_LOW		= 19,
	IMBX_WARNING_IPCAM_RESET	= 20,
	IMBX_NOT_SHOW_AGAIN			= 21,
	IMBX_REBOOT_VIDEO_TYPE_CHANGED	= 22,
	IMBX_REBOOT_HD_ACTIVE_CHANGED	= 23,
	IMBX_NO_AUTH					= 24,
	IMBX_UNSUPPORTED_LETTER		= 25,
	IMBX_CAMERA_FACTORY_DEFAULT		= 26,
	IMBX_NOT_SUPPORT_PLAYRATE	= 27,
	
	IMBX_MAX,
} IMBXSTR_E;

enum {
	TLINE_AUTO_HIDE = 0,
	TLINE_ALWAYS_OFF,
	TLINE_ALWAYS_ON,
};



////////////////////////////////////////////////////////////
//
// public interfaces
//

int vw_init();
int vw_get_count_mbxstr();
int vw_apply_new_lang();

NFOBJECT* vw_mbox_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid);
NFOBJECT* vw_mbox_auto_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid);
mb_type vw_mbox_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid);
NFOBJECT* vw_mbox_wait(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid);
void vw_remove_waitbox(NFOBJECT *wait_box);
mb_type vw_mbox(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid, mb_type type);
mb_type vw_mbox_4_line(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid1, IMBXSTR_E strid2, IMBXSTR_E strid3, IMBXSTR_E strid4, mb_type type);
mb_type vw_mbox_check(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid, IMBXSTR_E str_check, gint *check_ret, mb_type type);

#endif
