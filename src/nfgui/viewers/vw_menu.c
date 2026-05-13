/*
 * vw_menu.c
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, May 31, 2012
 *
 */

#include "nf_afx.h"
#include "vw_desc.h"
#include "vw_menu.h"
#include "ix_conf.h"

// SYS_SUBMENU - CAMERA
#define STR_SUB1_CAMERA_TITLE			"CAMERA TITLE"
#define STR_SUB1_IMAGE_SETUP			"IMAGE SETUP"
#define STR_SUB1_ONVIF_IMAGE_SETUP		"CAMERA SETUP"
#define STR_SUB1_COVERT_SETUP			"COVERT SETUP"
#define STR_SUB1_MOTION_SENSOR			"MOTION SENSOR"
#define STR_SUB1_ONVIF_MOTION_SENSOR	"MOTION SENSOR"
#define STR_SUB1_CAMERA_TYPE			"CAMERA TYPE"
#define STR_SUB1_PTZ_SETUP				"PTZ SETUP"
#define STR_SUB1_PRIVACY_MASK			"PRIVACY MASK"
#define STR_SUB1_IPCAMERA_INSTALL		"CAMERA INSTALLATION"
#define STR_SUB1_INSTALL_MODE			"INSTALLATION MODE"
#define	STR_SUB1_VCA_SETUP				"CLASSIC VA SETUP"
#define	STR_SUB1_BUILTIN_DLVA_SETUP		"BUILT-IN AI SETUP"
#define	STR_SUB1_ADVANCED_DLVA_SETUP	"AI BOX / AI CAM SETUP"
#define STR_SUB1_TAMPER_DETECT			"TAMPER DETECTION"
#define STR_SUB1_ANALOG_TYPE			"ANALOG TYPE SETUP"
#define STR_SUB1_FISHEYE_SETUP			"FISHEYE SETUP"
#define	STR_SUB1_ANALYSIS_SETUP			"AI ANALYTICS"
#define STR_SUB1_SPECIAL_CAM_SETUP      "SPECIAL CAMERA SETUP"

// SYS_SUBMENU - DISPLAY
#define STR_SUB2_OSD				"OSD"
#define STR_SUB2_MONITOR			"MONITOR"
#define STR_SUB2_SEQUENCE			"SEQUENCE"
#define STR_SUB2_SPOTOUT			"SPOT OUT"
#define STR_SUB2_HD_SPOTOUT			"HD SPOT OUT"
#define STR_SUB2_LOOP				"LOOP OUT"
#define STR_SUB2_POSATM	    		"POS / ATM"
#define STR_SUB2_ADVANCED_DUAL  	"DUAL MONITOR"

// SYS_SUBMENU - SOUND
#define STR_SUB3_AUDIO				"AUDIO"
#define STR_SUB3_BUZZER				"BUZZER"

// SYS_SUBMENU - USER
#define STR_SUB4_MANAGEMENT			"MANAGEMENT"
#define STR_SUB4_AUTHORITY			"GROUP AUTHORITY"

// SYS_SUBMENU - NETWORK
#define STR_SUB5_IPSETUP			"IP SETUP"
#define STR_SUB5_DDNS				"DDNS."
#define STR_SUB5_EMAIL				"E-MAIL"
#define STR_SUB5_NETSTAT			"NETWORK STATUS"
#define STR_SUB5_SECURUTY			"SECURITY"
#define STR_SUB5_SNMP               "SNMP"
#define STR_SUB5_CABLE_TEST         "CABLE TEST"
#define STR_SUB5_RTP                "RTP"

// SYS_SUBMENU - SYSTEM
#define STR_SUB6_DATETIME			"DATE / TIME"
#define STR_SUB6_MANAGEMENT			"SYSTEM MANAGEMENT"
#define STR_SUB6_INFORMATION		"SYSTEM INFORMATION"
#define STR_SUB6_CONTROLDEVICE		"CONTROL DEVICE"
#define STR_SUB6_SUPPORT_IPCAM		"SUPPORTED IPCAM"
#define STR_SUB6_SECURUTY			"SECURITY"
#define STR_SUB6_POSATM             "POS / ATM"
#define STR_SUB6_LICENSE            "LICENSE"

// SYS_SUBMENU - STORAGE
#define STR_SUB7_INFOMATION			"DISK INFORMATION"
#define STR_SUB7_OPERATION			"DISK OPERATIONS"
#define STR_SUB7_CONFIGURATION		"DISK CONFIGURATION"
#define STR_SUB7_SMARTSETUP			"S.M.A.R.T SETUP"

// SYS_SUBMENU - EVENT
#define STR_SUB8_ALARMOUT			"ALARM OUT"
#define STR_SUB8_EVENTNOTI			"EVENT NOTIFICATION"
#define STR_SUB8_ALARMSENSOR		"ALARM SENSOR"
#define STR_SUB8_MOTIONSENSOR		"MOTION SENSOR"
#define STR_SUB8_VIDEOLOSS			"VIDEO LOSS"
#define STR_SUB8_VCA_EVENT_ITX	    "VCA EVENT"
#define STR_SUB8_BUILTIN_DLVA_EVENT_ITX "BUILT-IN AI EVENT"
#define STR_SUB8_SYSTEMEVENT		"SYSTEM EVENT"
#define STR_SUB8_VCA_EVENT			"VCA EVENT"
#define STR_SUB8_TAMPER				"TAMPER EVENT"
#define STR_SUB8_POSATM_EVENT		"POS / ATM"
#define STR_SUB8_ANALYSIS_EVENT		"AI ANALYTICS"

// SYS_SUBMENU - EVENT - EVENT NOTIFICATION
#define STR_EVENTNOTI_BUZZER		"BUZZER"
#define STR_EVENTNOTI_DISP			"DISPLAY"
#define STR_EVENTNOTI_MAIL			"E-MAIL"
#define STR_EVENTNOTI_FTP			"FTP"
#define STR_EVENTNOTI_SMS			"SMS"
#define STR_EVENTNOTI_MOBILEPUSH	"MOBILE PUSH"

// SYS_SUBMENU - EVENT - ALARM SENSOR
#define STR_ALARM_SENSOR_CAM		"CAMERA"
#if defined(_IPX_MODEL_UX)
#define STR_ALARM_SENSOR_LOCAL		"NVR"
#else
#define STR_ALARM_SENSOR_LOCAL		"DVR"
#endif

// REC_SUBMENU
#define STR_REC_OPER				"OPERATION MODE"
#define STR_REC_CONT				"CONTINUOUS RECORDING"
#define STR_REC_MOT					"MOTION RECORDING"
#define STR_REC_ALARM				"AI/ALARM RECORDING"
#define STR_REC_PANIC				"PANIC RECORDING"
#define STR_REC_NETSTREAM			"NETWORK STREAMING"
#define STR_REC_CALCUL				"STORAGE CALCULATOR"
#define STR_REC_AUDIOMAP			"AUDIO MAPPING"

// ARCH_SUBMENU
#define STR_ARCH_NEW				"NEW ARCHIVING"
#define STR_ARCH_RESERVED			"RESERVED DATA MANAGEMENT"
#define STR_ARCH_DATA_PB			"ARCHIVED DATA PLAYBACK"
#define STR_ARCH_DEV_SETUP			"ARCHIVE DEVICES SETUP"

// SEARCH_SUBMENU
#define STR_SEARCH_TIME				"TIME SEARCH"
#define STR_SEARCH_THUMBNAIL		"THUMBNAIL SEARCH"
#define STR_SEARCH_EVENT			"EVENT SEARCH"
#define STR_SEARCH_TEXT 			"TEXT-IN SEARCH"
#define STR_SEARCH_SMART			"SMART SEARCH"
#define STR_VA_STATISTIC            "VA STATISTIC"
#define STR_SEARCH_SMART_REV		"SMART SEARCH"
#define STR_SEARCH_DEEPLEARNING		"AI ANALYTICS SEARCH"

#define STR_DELETE_DATA				"ERASE VIDEO"




MENU_CF mcf;

static gint _make_sys_menu_sub1()
{
    gint cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub1.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_IPCAMERA_INSTALL)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_IPCAMERA_INSTALL] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_IPCAMERA_INSTALL], STR_SUB1_IPCAMERA_INSTALL);
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ANALOG_TYPE)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_ANALOG_TYPE] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_ANALOG_TYPE], STR_SUB1_ANALOG_TYPE);
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_CAMERA_TITLE)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TITLE] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_CAMERA_TITLE], STR_SUB1_CAMERA_TITLE);
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_IMAGE_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_IMAGE_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_IMAGE_SETUP], STR_SUB1_IMAGE_SETUP);
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ONVIF_IMAGE_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_IMAGE_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_ONVIF_IMAGE_SETUP], STR_SUB1_ONVIF_IMAGE_SETUP);
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_COVERT_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_COVERT_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_COVERT_SETUP], STR_SUB1_COVERT_SETUP);
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_MOTION_SENSOR)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_MOTION_SENSOR] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_MOTION_SENSOR], STR_SUB1_MOTION_SENSOR);
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ONVIF_MOTION_SENSOR)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_ONVIF_MOTION_SENSOR] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_ONVIF_MOTION_SENSOR], STR_SUB1_ONVIF_MOTION_SENSOR);
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_CAMERA_TYPE)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_CAMERA_TYPE] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_CAMERA_TYPE], STR_SUB1_CAMERA_TYPE);        
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_PTZ_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_PTZ_SETUP], STR_SUB1_PTZ_SETUP);        
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_PRIVACY_MASK)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_PRIVACY_MASK] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_PRIVACY_MASK], STR_SUB1_PRIVACY_MASK);        
    }
    
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_FISHEYE_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_FISHEYE_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_FISHEYE_SETUP], STR_SUB1_FISHEYE_SETUP);        
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_TAMPER_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_TAMPER_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_TAMPER_SETUP], STR_SUB1_TAMPER_DETECT);        
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ANALYSIS_SETUP_ITX)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_ANALYSIS_SETUP_ITX] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_ANALYSIS_SETUP_ITX], STR_SUB1_ANALYSIS_SETUP);        
    }

    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_VCA_SETUP_S1)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_VCA_SETUP_S1] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_VCA_SETUP_S1], STR_SUB1_VCA_SETUP);
    }

	if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_INSTALL_MODE)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_INSTALL_MODE] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_INSTALL_MODE], STR_SUB1_INSTALL_MODE);
	}
#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
    if (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_SPECIAL_CAM_SETUP)) {
        mcf.sys_sub1.menu_pos[SYS_SUB1_SPECIAL_CAM_SETUP] = cnt++;
        g_sprintf(mcf.sys_sub1.menu_str[SYS_SUB1_SPECIAL_CAM_SETUP], STR_SUB1_SPECIAL_CAM_SETUP);
    }
#endif
    mcf.sys_sub1.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub2()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub2.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_OSD)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_OSD] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_OSD], STR_SUB2_OSD);                
    }
    
    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_MONITOR)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_MONITOR] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_MONITOR], STR_SUB2_MONITOR);                        
    }
    
    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_ADVANCED_DUAL)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_ADVANCED_DUAL] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_ADVANCED_DUAL], STR_SUB2_ADVANCED_DUAL);                        
    }
    
    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_SEQUENCE)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_SEQUENCE] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_SEQUENCE], STR_SUB2_SEQUENCE);        
    }
    
    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_SPOTOUT)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_SPOTOUT] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_SPOTOUT], STR_SUB2_SPOTOUT);                
    }
    

    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_HD_SPOTOUT)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_HD_SPOTOUT] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_HD_SPOTOUT], STR_SUB2_HD_SPOTOUT);
    }

    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_LOOP)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_LOOP] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_LOOP], STR_SUB2_LOOP);
    }

    if (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_POSATM)) {
        mcf.sys_sub2.menu_pos[SYS_SUB2_POSATM] = cnt++;
        g_sprintf(mcf.sys_sub2.menu_str[SYS_SUB2_POSATM], STR_SUB2_POSATM);
    }
    
    mcf.sys_sub2.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub3()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub3.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_sound & (1 << SYS_SUB3_AUDIO)) {
        mcf.sys_sub3.menu_pos[SYS_SUB3_AUDIO] = cnt++;
        g_sprintf(mcf.sys_sub3.menu_str[SYS_SUB3_AUDIO], STR_SUB3_AUDIO);        
    }
    
    if (ivsc.dmenu.submenu_sound & (1 << SYS_SUB3_BUZZER)) {
        mcf.sys_sub3.menu_pos[SYS_SUB3_BUZZER] = cnt++;
        g_sprintf(mcf.sys_sub3.menu_str[SYS_SUB3_BUZZER], STR_SUB3_BUZZER);
    }
    
    mcf.sys_sub3.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub4()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub4.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_user & (1 << SYS_SUB4_MANAGEMENT)) {
        mcf.sys_sub4.menu_pos[SYS_SUB4_MANAGEMENT] = cnt++;
        g_sprintf(mcf.sys_sub4.menu_str[SYS_SUB4_MANAGEMENT], STR_SUB4_MANAGEMENT);        
    }
    
    if (ivsc.dmenu.submenu_user & (1 << SYS_SUB4_AUTHORITY)) {
        mcf.sys_sub4.menu_pos[SYS_SUB4_AUTHORITY] = cnt++;
        g_sprintf(mcf.sys_sub4.menu_str[SYS_SUB4_AUTHORITY], STR_SUB4_AUTHORITY);        
    }
    
    mcf.sys_sub4.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub5()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub5.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_IPSETUP)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_IPSETUP] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_IPSETUP], STR_SUB5_IPSETUP);                
    }
    
    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_DDNS)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_DDNS], STR_SUB5_DDNS);                
    }
    
    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_EMAIL)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_EMAIL] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_EMAIL], STR_SUB5_EMAIL);                
    }
    
    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_NETSTAT)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_NETSTAT] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_NETSTAT], STR_SUB5_NETSTAT);                
    }
    
    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_SECURITY)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_SECURITY] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_SECURITY], STR_SUB5_SECURUTY);                
    }

    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_SNMP)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_SNMP] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_SNMP], STR_SUB5_SNMP);
    }

    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_CABLE_TEST)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_CABLE_TEST] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_CABLE_TEST], STR_SUB5_CABLE_TEST);
    }
     
    if (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_RTP)) {
        mcf.sys_sub5.menu_pos[SYS_SUB5_RTP] = cnt++;
        g_sprintf(mcf.sys_sub5.menu_str[SYS_SUB5_RTP], STR_SUB5_RTP);
    }

    mcf.sys_sub5.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub6()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub6.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_DATETIME)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_DATETIME] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_DATETIME], STR_SUB6_DATETIME);
    }
    
    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_MANAGEMENT)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_MANAGEMENT] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_MANAGEMENT], STR_SUB6_MANAGEMENT);
    }
    
    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_INFORMATION)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_INFORMATION] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_INFORMATION], STR_SUB6_INFORMATION);        
    }
    
    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_CONTROLDEVICE)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_CONTROLDEVICE] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_CONTROLDEVICE], STR_SUB6_CONTROLDEVICE);        
    }
    
    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_SUPPORT_IPCAM)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_SUPPORT_IPCAM] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_SUPPORT_IPCAM], STR_SUB6_SUPPORT_IPCAM);        
    }

    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_POSATM)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_POSATM], STR_SUB6_POSATM);
    }

    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_SECURITY)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_SECURITY] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_SECURITY], STR_SUB6_SECURUTY);        
    }

    if (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_LICENSE)) {
        mcf.sys_sub6.menu_pos[SYS_SUB6_LICENSE] = cnt++;
        g_sprintf(mcf.sys_sub6.menu_str[SYS_SUB6_LICENSE], STR_SUB6_LICENSE);
    }

    mcf.sys_sub6.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub7()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub7.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_INFOMATION)) {
        mcf.sys_sub7.menu_pos[SYS_SUB7_INFOMATION] = cnt++;
        g_sprintf(mcf.sys_sub7.menu_str[SYS_SUB7_INFOMATION], STR_SUB7_INFOMATION);                
    }
    
    if (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_OPERATION)) {
        mcf.sys_sub7.menu_pos[SYS_SUB7_OPERATION] = cnt++;
        g_sprintf(mcf.sys_sub7.menu_str[SYS_SUB7_OPERATION], STR_SUB7_OPERATION);                        
    }
    
    if (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_CONFIGURATION)) {
        mcf.sys_sub7.menu_pos[SYS_SUB7_CONFIGURATION] = cnt++;
        g_sprintf(mcf.sys_sub7.menu_str[SYS_SUB7_CONFIGURATION], STR_SUB7_CONFIGURATION);                        
    }

    if (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_SMARTSETUP)) {
        mcf.sys_sub7.menu_pos[SYS_SUB7_SMARTSETUP] = cnt++;
        g_sprintf(mcf.sys_sub7.menu_str[SYS_SUB7_SMARTSETUP], STR_SUB7_SMARTSETUP);                        
    }
    
    mcf.sys_sub7.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_sub8()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_sub8.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_ALARMOUT)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMOUT] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_ALARMOUT], STR_SUB8_ALARMOUT);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_EVENTNOTI)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_EVENTNOTI] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_EVENTNOTI], STR_SUB8_EVENTNOTI);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_ALARMSENSOR)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMSENSOR] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_ALARMSENSOR], STR_SUB8_ALARMSENSOR);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_MOTIONSENSOR)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_MOTIONSENSOR] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_MOTIONSENSOR], STR_SUB8_MOTIONSENSOR);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_VIDEOLOSS)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_VIDEOLOSS] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_VIDEOLOSS], STR_SUB8_VIDEOLOSS);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_ANALYSIS_EVENT_ITX)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_ANALYSIS_EVENT_ITX] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_ANALYSIS_EVENT_ITX], STR_SUB8_ANALYSIS_EVENT);                                
    }

    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_TAMPER)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_TAMPER] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_TAMPER], STR_SUB8_TAMPER);                                
    }    

    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_POSATM_EVENT)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_POSATM_EVENT] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_POSATM_EVENT], STR_SUB8_POSATM_EVENT);                                
    }
    
    if (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_SYSTEMEVENT)) {
        mcf.sys_sub8.menu_pos[SYS_SUB8_SYSTEMEVENT] = cnt++;
        g_sprintf(mcf.sys_sub8.menu_str[SYS_SUB8_SYSTEMEVENT], STR_SUB8_SYSTEMEVENT);                                
    }
    
    mcf.sys_sub8.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_event_noti()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_event_noti.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_BUZZER)) {
        mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER] = cnt++;
        g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_BUZZER], STR_EVENTNOTI_BUZZER);                                
    }
    
    if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_DISP)) {
        mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP] = cnt++;
        g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_DISP], STR_EVENTNOTI_DISP);                                
    }
    
    if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_MAIL)) {
        mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL] = cnt++;
        g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_MAIL], STR_EVENTNOTI_MAIL);                                
    }
    
    if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_FTP)) {
        mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP] = cnt++;
        g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_FTP], STR_EVENTNOTI_FTP);                                
    }
    
    if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_SMS)) {
        mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS] = cnt++;
        g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_SMS], STR_EVENTNOTI_SMS);                                
    }
	
   if (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_MOBILEPUSH)) {
	   mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH] = cnt++;
	   g_sprintf(mcf.sys_event_noti.menu_str[SYS_EVENTNOTI_MOBILEPUSH], STR_EVENTNOTI_MOBILEPUSH);								
   	}
   	
    mcf.sys_event_noti.cnt = cnt;

    return 0;
}

static gint _make_sys_menu_alarm_sensor_evt()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.sys_alarm_sensor.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_alarm_sensor_evt & (1 << SYS_ALARM_SENSOR_CAM)) {
        mcf.sys_alarm_sensor.menu_pos[SYS_ALARM_SENSOR_CAM] = cnt++;
        g_sprintf(mcf.sys_alarm_sensor.menu_str[SYS_ALARM_SENSOR_CAM], STR_ALARM_SENSOR_CAM);                                
    }
    
    if (ivsc.dmenu.submenu_alarm_sensor_evt & (1 << SYS_ALARM_SENSOR_LOCAL)) {
        mcf.sys_alarm_sensor.menu_pos[SYS_ALARM_SENSOR_LOCAL] = cnt++;
        g_sprintf(mcf.sys_alarm_sensor.menu_str[SYS_ALARM_SENSOR_LOCAL], STR_ALARM_SENSOR_LOCAL);                                
    }
   	
    mcf.sys_alarm_sensor.cnt = cnt;

    return 0;
}

static gint _make_rec_menu()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.rec.menu_pos[i] = -1;

    if (ivsc.dmenu.recmenu & (1 << REC_SUB_OPER)) {
        mcf.rec.menu_pos[REC_SUB_OPER] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_OPER], STR_REC_OPER);
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_CONT)) {
        mcf.rec.menu_pos[REC_SUB_CONT] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_CONT], STR_REC_CONT);
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_MOT)) {
        mcf.rec.menu_pos[REC_SUB_MOT] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_MOT], STR_REC_MOT);        
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_ALARM)) {
        mcf.rec.menu_pos[REC_SUB_ALARM] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_ALARM], STR_REC_ALARM);        
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_PANIC)) {
        mcf.rec.menu_pos[REC_SUB_PANIC] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_PANIC], STR_REC_PANIC);        
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_NETSTREAM)) {
        mcf.rec.menu_pos[REC_SUB_NETSTREAM] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_NETSTREAM], STR_REC_NETSTREAM);        
    }

    if (ivsc.dmenu.recmenu & (1 << REC_SUB_CALCUL)) {
        mcf.rec.menu_pos[REC_SUB_CALCUL] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_CALCUL], STR_REC_CALCUL);        
    }
    
    if (ivsc.dmenu.recmenu & (1 << REC_SUB_AUDIOMAP)) {
        mcf.rec.menu_pos[REC_SUB_AUDIOMAP] = cnt++;
        g_sprintf(mcf.rec.menu_str[REC_SUB_AUDIOMAP], STR_REC_AUDIOMAP);        
    }

    mcf.rec.cnt = cnt;

    return 0;
}

static gint _make_arch_menu()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.arch.menu_pos[i] = -1;

    if (ivsc.dmenu.archmenu & (1 << ARCH_SUB_NEW_ARCH)) {
        mcf.arch.menu_pos[ARCH_SUB_NEW_ARCH] = cnt++;
        g_sprintf(mcf.arch.menu_str[ARCH_SUB_NEW_ARCH], STR_ARCH_NEW);                
    }
    
    if (ivsc.dmenu.archmenu & (1 << ARCH_SUB_RESERVED)) {
        mcf.arch.menu_pos[ARCH_SUB_RESERVED] = cnt++;
        g_sprintf(mcf.arch.menu_str[ARCH_SUB_RESERVED], STR_ARCH_RESERVED);                
    }
    
    if (ivsc.dmenu.archmenu & (1 << ARCH_SUB_DATA_PB)) {
        mcf.arch.menu_pos[ARCH_SUB_DATA_PB] = cnt++;
        g_sprintf(mcf.arch.menu_str[ARCH_SUB_DATA_PB], STR_ARCH_DATA_PB);                        
    }
    
    if (ivsc.dmenu.archmenu & (1 << ARCH_SUB_DEV_SETUP)) {
        mcf.arch.menu_pos[ARCH_SUB_DEV_SETUP] = cnt++;
        g_sprintf(mcf.arch.menu_str[ARCH_SUB_DEV_SETUP], STR_ARCH_DEV_SETUP);                        
    }
    
    mcf.arch.cnt = cnt;

    return 0;
}

static gint _make_search_menu()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.search.menu_pos[i] = -1;

    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_TIME)) {
        mcf.search.menu_pos[SEARCH_SUB_TIME] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_TIME], STR_SEARCH_TIME);                                
    }
    
    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_THUMBNAIL)) {
        mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_THUMBNAIL], STR_SEARCH_THUMBNAIL);
    }
    
    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_EVENT)) {
        mcf.search.menu_pos[SEARCH_SUB_EVENT] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_EVENT], STR_SEARCH_EVENT);
    }

    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_TEXT)) {
        mcf.search.menu_pos[SEARCH_SUB_TEXT] = cnt++;        
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_TEXT], STR_SEARCH_TEXT);
    }

    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_VA_STATISTIC)) {
        mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_VA_STATISTIC], STR_VA_STATISTIC);
    }
    
    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_SMART)) {
        mcf.search.menu_pos[SEARCH_SUB_SMART] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_SMART], STR_SEARCH_SMART);
    }
    
    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_SMART_REV)) {
        mcf.search.menu_pos[SEARCH_SUB_SMART_REV] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_SMART_REV], STR_SEARCH_SMART_REV);
    }
    
    if (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_DEEPLEARNING)) {
        mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING] = cnt++;
        g_sprintf(mcf.search.menu_str[SEARCH_SUB_DEEPLEARNING], STR_SEARCH_DEEPLEARNING);
    }

    mcf.search.cnt = cnt;

    return 0;
}

static gint _make_delete_menu()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.del.menu_pos[i] = -1;

    if (ivsc.dmenu.submenu_storage_del & (1 << SYS_SUB7_OPERATION_DELETE)) {
        mcf.del.menu_pos[SYS_SUB7_OPERATION_DELETE] = cnt++;
        g_sprintf(mcf.del.menu_str[SYS_SUB7_OPERATION_DELETE], STR_DELETE_DATA);                                
    }
    
    mcf.del.cnt = cnt;

    return 0;
}

static gint _make_userguide_menu()
{
    gint ret, cnt = 0;
    gint i;

    for (i = 0; i < MAX_SUB_CNT; i++)
        mcf.userguide.menu_pos[i] = -1;
        
    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB1)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB1] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB1], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB1]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB2)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB2] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB2], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB2]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB3)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB3] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB3], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB3]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB4)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB4] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB4], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB4]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB5)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB5] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB5], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB5]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB6)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB6] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB6], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB6]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB7)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB7] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB7], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB7]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB8)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB8] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB8], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB8]);
    }

    if (ivsc.dmenu.userguide & (1 << USERGUIDE_SUB_TAB9)) {
        mcf.userguide.menu_pos[USERGUIDE_SUB_TAB9] = cnt++;
        strcpy(mcf.userguide.menu_str[USERGUIDE_SUB_TAB9], ivsc.dfunc.userguide.title[USERGUIDE_SUB_TAB9]);
    }    

    mcf.userguide.cnt = cnt;

    return 0;
}

gint vw_menu_init()
{
	g_message("%s, %d, called", __FUNCTION__, __LINE__);
	
    memset(&mcf, 0x00, sizeof(MENU_CF));
	
    _make_sys_menu_sub1();
    _make_sys_menu_sub2();
    _make_sys_menu_sub3();
    _make_sys_menu_sub4();
    _make_sys_menu_sub5();
    _make_sys_menu_sub6();
    _make_sys_menu_sub7();
    _make_sys_menu_sub8();	    
    _make_sys_menu_event_noti();
    _make_sys_menu_alarm_sensor_evt();
    _make_rec_menu();	
    _make_arch_menu();	
    _make_search_menu();
    _make_delete_menu();
    _make_userguide_menu();

    return 0;
}

gint vw_menu_get_str_sys_menu_sub1(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB1_CNT; i++)
    {
        pos = mcf.sys_sub1.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub1.menu_str[i];
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub2(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB2_CNT; i++)
    {
        pos = mcf.sys_sub2.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub2.menu_str[i];
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub3(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB3_CNT; i++)
    {
        pos = mcf.sys_sub3.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub3.menu_str[i];
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub4(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB4_CNT; i++)
    {
        pos = mcf.sys_sub4.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub4.menu_str[i];

        if (pos >= MAX_TAB_CNT) g_assert(0);        
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub5(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB5_CNT; i++)
    {
        pos = mcf.sys_sub5.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub5.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub6(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB6_CNT; i++)
    {
        pos = mcf.sys_sub6.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub6.menu_str[i];        
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub7(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB7_CNT; i++)
    {
        pos = mcf.sys_sub7.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub7.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_sub8(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB8_CNT; i++)
    {
        pos = mcf.sys_sub8.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_sub8.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_event_noti(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_EVENTNOTI_CNT; i++)
    {
        pos = mcf.sys_event_noti.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_event_noti.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_sys_menu_alarm_sensor_evt(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_ALARM_SENSOR_CNT; i++)
    {
        pos = mcf.sys_alarm_sensor.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.sys_alarm_sensor.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_rec_menu(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < REC_SUB_CNT; i++)
    {
        pos = mcf.rec.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.rec.menu_str[i];       
    }
    return 0;
}

gint vw_menu_get_str_arch_menu(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < ARCH_SUB_CNT; i++)
    {
        pos = mcf.arch.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.arch.menu_str[i];
    }
    return 0;
}

gint vw_menu_get_str_search_menu(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SEARCH_SUB_CNT; i++)
    {
        pos = mcf.search.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) menu[pos] = mcf.search.menu_str[i];        
    }
    return 0;
}

gint vw_menu_get_str_delete_menu(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB7_OPERATION_CNT; i++)
    {
        pos = mcf.del.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) menu[pos] = mcf.del.menu_str[i];

    }
    return 0;
}

gint vw_menu_get_str_userguide_menu(gchar **menu)
{
    gint i, pos;

    for (i = 0; i < USERGUIDE_SUB_CNT; i++)
    {
        pos = mcf.userguide.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);        
        if(pos != -1) menu[pos] = mcf.userguide.menu_str[i];        
    }
    return 0;
}
