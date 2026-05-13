/*
 * vw.c
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jun 14, 2011
 *
 */

#include "iux_afx.h" 
#include "thumbnail_manager.h"
#include "support/multi_language_support.h"


////////////////////////////////////////////////////////////
//
// global variable
//

gchar g_month_str[12][64];
gchar _month_str[12][64] = {
	"JANUARY",
	"FEBRUARY",
	"MARCH",
	"APRIL",
	"MAY",
	"JUNE",
	"JULY",
	"AUGUST",
	"SEPTEMBER",
	"OCTOBER",
	"NOVEMBER",
	"DECEMBER"
};

gchar g_poe_str[1][256];
gchar _poe_str[1][256] = {
	"( POE POWER CONSUMPTION  SUM : %s / MAX : %s )",
};

gchar g_sec_str[1][32];
gchar _sec_str[1][32] = {
	"%d SEC",
};

gchar g_area_str[1][32];
gchar _area_str[1][32] = {
	"AREA %d",
};

gchar g_vcalog_str[12][64];
gchar _vcalog_str[12][64] = {
	"VCA etc",
	"VCA Positive direction",
	"VCA Negative direction",
	"VCA Enter",
	"VCA Exit",
	"VCA Stopped",
	"VCA Abandoned",
	"VCA Removed",
	"VCA Loitering",
	"VCA Fall",
	"VCA Counter",
	"VCA Camera tamper",
};

const gchar *g_msg_str[] = {
	"TBD",		// IMBX_DEFAULT
	"Configuration has been saved.",
	"Disk read error.\nThis system will reboot.",
	"Recoverying storage is timeout.\nThis system will reboot.",
	"Recoverying is failed\nDo you want to recover by format?",
	"It's failed to save the system data.",
	"It's unable to sync to the time server.",
	"Now, auto time sync is in progress.\nPlease wait...",
	"Calibration failed.",
	"Calibration timeout.",
	"Can't play the file.",
	"Do you want to disconnect?",
	"One push focus failed.",
	"One push focus timeout.",
	"Do you want to format all disks?",
	"S.M.A.R.T. error occured\nDo you want to continue booting?",
	"S.M.A.R.T. warning occured\nDo you want to continue booting?",
	"Camera type has been changed.\nThe system will be reboot soon.",
	"The specified value is too high.\nIt will be changed to the highest possible.",
	"The specified value is too low.\nIt will be changed to the lowest possible.",
	"While setting the camera can be restarted.\nDo you want to continue?",
	"Do not show this message again",
	"Video type has been changed.\nThe system will be reboot.\nDo you want to continue?",	
	"Activation has been changed.\nThe system will be reboot.\nDo you want to continue?",		
	"No authority",
	"Language other than English if it contains can not be edited.",	
	"Do you want to initialize the camera connected to NVR?",
	"It does not support more than 4x in split-screen over 8.",
};

#define MBXSTR_MAX	(sizeof(g_msg_str) / sizeof(char *))


gchar g_system_logstr[7][256];
gchar _system_logstr[7][256] = {
	"System start.",
	"System shutdown.",
	"Abnormal shutdown is detected.",
	"System is recovered during start up.",
	"The system time is changed by %s.",
	"The system firmware is upgraded by %s.",		// not used
	"Disks are formatted by %s.",
};

gchar g_reboot_logstr[5][256];
gchar _reboot_logstr[5][256] = {
	"Abnormal shutdown is detected.",
	"Camera communication error has been detected.",
	"The disk read / write error was detected.",
	"Recording error has occurred.",
	"Disk recovery have failed.",
};

gchar g_fwup_logstr[4][256];
gchar _fwup_logstr[4][256] = {
	"The system firmware is upgraded by %s.",
	"Firmware upgrade is started by %s.",
	"Firmware successfully upgraded.",
	"Firmware upgrade failed.",
};

gchar g_syslog_logstr[23][256];
gchar _syslog_logstr[23][256] = {
	"Disk check is performed by %s.",
	"All channel reconnecting (reason:time change)",
	"All channel reconnecting (reason:upgrade)",
	"All channel reconnecting (reason:system data load)",
	"All channel reconnecting (reason:factory default)",
	"All channel reconnecting (reason:ip change)",
	"All channel reconnecting (reason:install mode change)",
	"All channel panic recording start",
	"All channel panic recording stop",
	"PTZ control start (%s)",
	"PTZ control stop (%s)",
	"Administrator session lock (%s)",
	"Administrator session unlock",
	"Storage plug",
	"Storage unplug",
	"Time change information (%s)",
	"FW upgrade information (%s)",
	"Backup verify information (%s)",
	"Allow list add (%s)",
	"Allow list delete (%s)",
	"Deny list add (%s)",
	"Deny list delete (%s)",
	"System restart (%s)",
};

gchar g_logonoff_logstr[6][256];
gchar _logonoff_logstr[6][256] = {
	"LIVE",
	"SEARCH",
	"ARCHIVING",
	"SYSTEM SETUP",
	"RECORD SETUP",
	"NR",
};

gchar g_logon_logstr[4][256];
gchar _logon_logstr[4][256] = {
	"%s log on.",
	"%s log off.",
	"%s log on from %s.",
	"%s log off from %s.",
};

gchar g_alarmevt_logstr[2][256];
gchar _alarmevt_logstr[2][256] = {
	"An alarm signal of #%02d is returned to normal state.",
	"An alarm signal is detected on #%02d.",
};

gchar g_motionevt_logstr[2][256];
gchar _motionevt_logstr[2][256] = {
	"A motion event of #%02d is returned to normal state.",
	"A motion event is detected on #%02d.",
};

gchar g_vlossevt_logstr[2][256];
gchar _vlossevt_logstr[2][256] = {
	"A video-in of #%02d is returned to normal state.",
	"A video-loss is detected on #%02d.",
};

gchar g_recsetup_logstr[24][256];
gchar _recsetup_logstr[24][256] = {
	"Record mode is changed by %s.",
	"NORMAL REC. PARAMETER <%s>",				// not used
	"NORMAL REC. SCHEDULE <%s>",				// not used
	"INTENSIVE REC. PARAMETER <%s>",			// not used
	"INTENSIVE REC. SCHEDULE <%s>",				// not used
	"SWITCHING REC. PARAMETER <%s>",			// not used
	"SWITCHING REC. SCHEDULE <%s>",				// not used
	"The parameter of panic record is changed by %s.",			
	"RECORD OPERATION <%s>",					// not used
	"SIMPLE RECORD <%s>",						// not used
	"ADV. REC. DAY PARAMETER <%s>",				// not used
	"ADV. REC. DAY SCHEDULE <%s>",				// not used
	"ADV. REC. WEEK PARAMETER <%s>",			// not used
	"ADV. REC. WEEK SCHEDULE <%s>",				// not used
	"The parameter of continuous record is changed by %s.",
	"The schedule of continuous record is changed by %s.",
	"The parameter of alarm record is changed by %s.",
	"The schedule of alarm record is changed by %s.",
	"CONT/MOTION REC. PARAMETER <%s>",			// not used
	"CONT/MOTION REC. SCHEDULE <%s>",			// not used
	"The parameter of motion record is changed by %s.",
	"The schedule of motion record is changed by %s.",
	"The parameter of network streaming is changed by %s.",
	"The schedule of network streaming is changed by %s.",	
};

gchar g_syssetup_logstr[62][256];
gchar _syssetup_logstr[62][256] = {
	"The camera title is changed by %s.",
	"<1 Unknown Message. %s>",
	"The PTZ configuration is changed by %s.",
	"<3 Unknown Message. %s>",
	"The alarm sensor configuration is changed by %s.",				// alarm sensor
	"The video-loss event configuration is changed by %s.",
	"The motion event configuration is changed by %s.",
	"<4 Unknown Message. %s>",										//hdd
	"<5 Unknown Message. %s>",
	"<6 Unknown Message. %s>",
	"<7 Unknown Message. %s>",
	"<8 Unknown Message. %s>",
	"<9 Unknown Message. %s>",
	"<10 Unknown Message. %s>",
	"<11 Unknown Message. %s>",
	"<12 Unknown Message. %s>",
	"The SPOT configuration is changed by %s.",					// 0x10
	"<14 Unknown Message. %s>",
	"<15 Unknown Message. %s>",
	"<16 Unknown Message. %s>",
	"The audio configuration is changed by %s.",
	"The user data is changed by %s.",
	"The user group data is changed by %s.",
	"<17 Unknown Message. %s>",
	"The email configuration is changed by %s.",
	"%s opened the system information.",
	"The date/time configuration is changed by %s.",							// date&time format 
	"The disk configuration is changed by %s.",
	"The covert configuration is changed by %s.",
	"The motion detecting configuration is changed by %s.",
	"<18 Unknown Message. %s>",
	"The system management configuration is changed by %s.",
	"<19 Unknown Message. %s>",									// 0x20
	"The OSD configuration is changed by %s.",
	"The monitor configuration is changed by %s.",
	"The buzzer configuration is changed by %s.",
	"The control device configuration is changed by %s.",
	"The configuration for auto-logout is changed by %s.",
	"The IP configuration is changed by %s.",
	"The DDNS configuration is changed by %s.",
	"The camera image configuration is changed by %s.",
	"The display sequence configuration is changed by %s.",
	"The text-in device configuration is changed by %s.",
	"The SMART configuration is changed by %s.",
	"The system event configuration is changed by %s.",
	"<20 Unknown Message. %s>",
	"The event configuration for notifying is changed by %s.",
	"The alarm out configuration is changed by %s.",
	"The live layout configuration is changed by %s.",			// 0x30
	"The tamper configuration is changed by %s.",
	"The privacy mask configuration is changed by %s.",
	"The AI analytics camera configuration is changed by %s.",
	"The AI analytics event configuration is changed by %s.",
	"The email security option turned on by %s.",
	"The email security option turned off by %s.",
	"The cable test function has been performed by %s.",
	"The password is reissued by CRM. (%s)",
	"The loop configuration is changed by %s.",
	"The analog type configuration is changed by %s.",
	"The buzzer output was turned off by %s.",
	"Add user. (%s)",
	"Delete user. (%s)",
};

gchar g_smart_logstr[1][256];
gchar _smart_logstr[1][256] = {
	"SMART warning is detected on %s.",
};

gchar g_disk_logstr[10][256];
gchar _disk_logstr[10][256] = {
	"No Disk.",
	"New HDD disk is added in %s.",
	"Some HDD is removed from %s.",
	"I/O interfaces occured an error in %s.",
	"HDD UNC error is detected in %s.",
	"HDD UNC error is recovered in %s.",
	"Storage space is exhausted.",
	"Disk write mode is changed to the Limit mode.",
	"Disk write mode is changed to the Overwrite mode.",
	"Disk write mode is changed to the Writeonce mode.",
};

gchar g_location_logstr[2][256];
gchar _location_logstr[2][256] = {
	"internal",
	"external",
};

gchar g_diskfull_logstr[1][256];
gchar _diskfull_logstr[1][256] = {
	"The %s disk is full.",
};

gchar g_diskow_logstr[1][256];
gchar _diskow_logstr[1][256] = {
	"Overwriting is started in %s.",
};

gchar g_recstart_logstr[5][256];
gchar _recstart_logstr[5][256] = {
	"Camera #%02d starts continuous recording.",
	"Camera #%02d starts alarm recording.",
	"Camera #%02d starts motion recording.",
	"Camera #%02d starts user-event recording.",
	"Camera #%02d starts panic recording.",
};

gchar g_recstop_logstr[5][256];
gchar _recstop_logstr[5][256] = {
	"Camera #%02d stops continuous recording.",
	"Camera #%02d stops alarm recording.",
	"Camera #%02d stops motion recording.",
	"Camera #%02d stops user-event recording.",
	"Camera #%02d stops panic recording.",
};

gchar g_sysevt_logstr[39][256];
gchar _sysevt_logstr[39][256] = {
	"An email is sent to %s.",
	"Burning is started.(%s)",
	"Burning is stopped.(%s)",
	"Restoring to factory default is performed by %s.",
	"The system data are saved by %s.",
	"The system data are loaded by %s.",
	"No video input is detected on #%02d.",
	"CPU fan fail.",
	"Current RPM of CPU fan : %s.",
	"System fan fail.",
	"Current RPM of system fan : %s.",
	"CPU termperature fail.",
	"Current CPU temperature : %s.",
	"System temperature fail.",
	"Current system termperature: %s.",
	"RTC battery fail.",
	"It's failed to synchronize the system time with NTP server.",
	"The system tims is synchronized with NTP server.",
	"%s is failed to log on to system.",
	"All of windows are closed automatically by auto-logout.",
	"The system timezone is changed to %s.",
	"The system language is changed to %s.",
	"The data is reserved.(%s)",
	"The reserved data is removed.(%s)",
	"POE fail.",
#if defined(_ZICOM_STRING_FIX)
	"Part of the data is erased by (%s).",
#else
	"Part of the data is erased.",
#endif
	"Erased range : (%s)",
	"Rebooting : (%s)",
	"Remote support session started.",
	"Remote support session ended.",
	"Self-diagnosis performed.",
	"The remote control has been tried by the authentication key %s.",
	"Cable test start.",
	"Cable issue detected. (%s)",
	"The certification number was issued by %s.",
	"POE power abnormal detection (%s)",
	"POE total power abnormal detection (%s)",
	"Alarm monitoring armed",
	"Alarm monitoring disarmed",
};

gchar *g_sysdebug_logstr[] = {
	"ERROR CODE :",
	"ERROR MSG :",
	"SYSLOG EMERGENCY",
	"SYSLOG ALERT",
	"SYSLOG CRITICAL",
	"SYSLOG ERROR",
	"SYSLOG WARNING",
	"SYSLOG NOTICE",
	"SYSLOG INFO",
	"SYSLOG DEBUG",
};

gchar g_network_logstr[26][256];
gchar _network_logstr[26][256] = {
	"The WAN port is on.",
	"The WAN port is off.",
	"The LAN port is on.",
	"The LAN port is off.",
	"The camera #%02d is linked.",
	"The camera #%02d is unlinked.",
	"The link of camera #%02d is time-out.",
	"The unknown device is detected in port #%02d.",
	"IP address is conflicted with [%s].",
	"The IP address is changed to %s.",
	"The IP address %s is allocated via DHCP.",
	"The IP address %s is reallocated via DHCP.",
	"It's failed to get an IP address via DHCP.",
	"A network attack is detected.",
	"Invalid DNS is detected.",
	"Invalid gateway is detected.",
	"It's failed to update DDNS information.",
	"The WAN port is shutdown.",
	"It's failed to send a email.",
	"It's failed to get a time from NTP server.",
	"The remote user %s failed to login from %s.",
	"IP address of CAM%d is conflicted with [%s].",
	"The video relay service is operating normally.",
	"SIP service failure.",
	"The AI BOX set for camera #%02d is connected.(%s)",
	"The AI BOX set for camera #%02d is disconnected.(%s)",
};

gchar g_ipcamevt_logstr[8][256];
gchar _ipcamevt_logstr[8][256] = {
	"IP camera device is ready on #%02d.",
	"IP camera device is out from #%02d.",
	"IP camera device is reset #%02d.",
	"Unknown devices is detected on #%02d.",
	"IP camera connection is failed on #%02d.",
	"IP camera log in is failed on #%02d.",
	"IP camera configuration is failed on #%02d.",
	"Unsupported model is detected on #%02d.",
};

gchar g_tamper_logstr[3][256];
gchar _tamper_logstr[3][256] = {
	"Tamper Detected #%02d : REDIRECTION",
	"Tamper Detected #%02d : BLOCKAGE",
	"Tamper Detected #%02d : DEFOCUSING",
};

gchar g_vcaevt_logstr[1][256];
gchar _vcaevt_logstr[1][256] = {
	"A VCA event on camera #%02d: %s."
};

gchar g_posevt_logstr[1][256];
gchar _posevt_logstr[1][256] = {
	"A POS text on ch #%02d: %s."
};

int g_klass_bound[33][2] = {
	{ -1, -1 },		// 0
	{ -1, -1 },
	{  0,  4 },		// 2, abnormal reboot
	{ -1, -1 },
	{ -1, -1 },
	{  0,  3 },		// 5, fw upgrade
	{ -1, -1 },
	{  0, 22 },		// 7, syslog

	{  0,  4 },		// 8
	{  0,  4 },
	{  0,  4 },
	{  0,  4 },

	{  0, 23 },		// 12
	{  0, 61 },

	{  0,  1 },		// 14
	{  0,  1 },
	{  0,  1 },
	{  0,  1 },
	{  0,  1 },

	{ -1, -1 },		// 19
	{  0,  9 },
	{ -1, -1 },
	{ -1, -1 },

	{  0,  4 },		// 23
	{  0,  4 },

	{  0, 38 },		// 38, system event
	{  0, 27 },		// debug

	{ -1, -1 },		// 27

	{  0, 25 },		// 28
	{  0,  7 },

	{  0, 2 },
	{ -1, -1 },
	{ -1, -1 },
};



////////////////////////////////////////////////////////////
//
// private functions
//

static int _translate_calendar_text()
{
	char tmp[256];
	char *p;
	int i;

	for (i = 0; i < 12; ++i) {
		strcpy(tmp, _month_str[i]);
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(g_month_str[i], p, g_utf8_strlen(p, 256));
		else
			strcpy(g_month_str[i], tmp);
	}

	return 0;
}

static int _translate_poe_text()
{
	char tmp[256];
	char *p;
	int i;

	for (i = 0; i < 1; ++i) {
		strcpy(tmp, _poe_str[i]);
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(g_poe_str[i], p, g_utf8_strlen(p, 256));
		else
			strcpy(g_poe_str[i], tmp);
	}

	return 0;
}

static int _translate_sec_text()
{
	char tmp[32];
	char *p;
	int i;

	for (i = 0; i < 1; ++i) {
		strcpy(tmp, _sec_str[i]);
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(g_sec_str[i], p, g_utf8_strlen(p, 32));
		else
			strcpy(g_sec_str[i], tmp);
	}

	return 0;
}

static int _translate_area_text()
{
	char tmp[32];
	char *p;
	int i;

	for (i = 0; i < 1; ++i) {	
		strcpy(tmp, _area_str[i]);
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(g_area_str[i], p, g_utf8_strlen(p, 32));
		else
			strcpy(g_area_str[i], tmp);
	}

	return 0;
}

static int _translate_vcalog_text()
{
	char tmp[64];
	char *p;
	int i;

	for (i = 0; i < 12; ++i) {	
		strcpy(tmp, _vcalog_str[i]);
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(g_vcalog_str[i], p, g_utf8_strlen(p, 64));
		else
			strcpy(g_vcalog_str[i], tmp);
	}

	return 0;
}

static int _translate_log_text()
{
	char tmp[256];
	char *p;
	char *loggrp_s, *loggrp_d;
	char *log_s, *log_d;

	int j, i;
	int str_cnt[] = { 
		7, 
		5,
		4,
		23,
		6, 
		4, 
		2, 
		2, 
		2, 
		24, 
		62, 
		1, 
		10, 
		2, 
		1, 
		1, 
		5, 
		5, 
		39,
		26, 
		8,
		3,
		1,
		1
	}; 

	char **tstr[2][24] = {
		{
			_system_logstr,
			_reboot_logstr,
			_fwup_logstr,
			_syslog_logstr,
			_logonoff_logstr,
			_logon_logstr,
			_alarmevt_logstr,
			_motionevt_logstr,
			_vlossevt_logstr,

			_recsetup_logstr,
			_syssetup_logstr,
			_smart_logstr,
			_disk_logstr,
			_location_logstr,
			_diskfull_logstr,

			_diskow_logstr,
			_recstart_logstr,
			_recstop_logstr,
			_sysevt_logstr,
			_network_logstr,
			_ipcamevt_logstr,
			_tamper_logstr,
			_vcaevt_logstr,
			_posevt_logstr
		},
		{
			g_system_logstr,
			g_reboot_logstr,
			g_fwup_logstr,
			g_syslog_logstr,
			g_logonoff_logstr,
			g_logon_logstr,
			g_alarmevt_logstr,
			g_motionevt_logstr,
			g_vlossevt_logstr,

			g_recsetup_logstr,
			g_syssetup_logstr,
			g_smart_logstr,
			g_disk_logstr,
			g_location_logstr,
			g_diskfull_logstr,

			g_diskow_logstr,
			g_recstart_logstr,
			g_recstop_logstr,
			g_sysevt_logstr,
			g_network_logstr,
			g_ipcamevt_logstr,
			g_tamper_logstr,
			g_vcaevt_logstr,
			g_posevt_logstr
		}
	};

	for (i = 0; i < sizeof(str_cnt) / sizeof(int); ++i) {
		loggrp_s = tstr[0][i];
		loggrp_d = tstr[1][i];
		for (j = 0; j < str_cnt[i]; ++j) {
			log_s = loggrp_s + (j * 256);
			log_d = loggrp_d + (j * 256);
			strcpy(tmp, log_s);
			p = lookup_string(tmp);
			if (p != NULL)
				g_utf8_strncpy(log_d, p, g_utf8_strlen(p, 256));
			else
				strcpy(log_d, tmp);

			
		}
	}

	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int vw_init()
{
	start_thumbnail_manager();
	_translate_calendar_text();
	_translate_poe_text();	
    _translate_sec_text();
    _translate_area_text();        
	_translate_log_text();
	_translate_vcalog_text();
	screen_capture_init();

	return 0;
}

int vw_get_count_mbxstr()
{
	return MBXSTR_MAX;
}

int vw_apply_new_lang()
{
	_translate_calendar_text();
	_translate_poe_text();
    _translate_sec_text();
    _translate_area_text();    
	_translate_log_text();
	_translate_vcalog_text();

	return 0;
}

int vw_open_board_confirm_mbox()
{
	nftool_mbox(NF_TOPWND, "WARNING", "You are using unknown product.\nThe system will reboot after 1 minute.", NFTOOL_MB_OK);

	return 0;
}
