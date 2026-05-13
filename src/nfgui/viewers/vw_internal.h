/*
 * vw_internal.h
 * 	- internal header in viewer
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 20, 2011
 *
 */

#ifndef __VW_INTERNAL_H
#define __VW_INTERNAL_H

#include "nf_api_archive.h"

typedef struct _BURN_INFO {
	NF_ARCH_TYPE_E type;
	guint16 arch_id;
	guchar dev_id;
	gchar tag[32];
	gchar memo[256];	
}BURN_INFO;

extern gchar g_month_str[12][64];
extern gchar g_poe_str[1][256];
extern gchar g_sec_str[1][32];
extern gchar g_area_str[1][32];
extern gchar g_vcalog_str[12][64];
extern gchar g_system_logstr[8][256];
extern gchar g_reboot_logstr[5][256];
extern gchar g_fwup_logstr[4][256];
extern gchar g_syslog_logstr[23][256];
extern gchar g_logonoff_logstr[6][256];
extern gchar g_logon_logstr[4][256];
extern gchar g_alarmevt_logstr[2][256];
extern gchar g_motionevt_logstr[2][256];
extern gchar g_vlossevt_logstr[2][256];
extern gchar g_recsetup_logstr[24][256];
extern gchar g_syssetup_logstr[62][256];
extern gchar g_smart_logstr[1][256];
extern gchar g_disk_logstr[10][256];
extern gchar g_location_logstr[2][256];
extern gchar g_diskfull_logstr[1][256];
extern gchar g_diskow_logstr[1][256];
extern gchar g_recstart_logstr[5][256];
extern gchar g_recstop_logstr[5][256];
extern gchar g_sysevt_logstr[31][256];
extern gchar g_network_logstr[26][256];
extern gchar g_ipcamevt_logstr[8][256];
extern gchar g_tamper_logstr[3][256];
extern gchar g_vcaevt_logstr[1][256];
extern gchar g_posevt_logstr[1][256];
extern int g_klass_bound[32][2];

#endif

