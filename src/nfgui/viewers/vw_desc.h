/*
 * vw_desc.h
 *  - dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Mar 19, 2015
 *
 */

#ifndef __VW_DESC_H
#define __VW_DESC_H


#if defined(_IPX_MODEL_UX)
#if GUI_32CH_SUPPORT
    #if defined (_IPX_32P5)
        #define DVR_ALARM_IN		(16)
        #define DVR_ALARM_OUT		(16)
        #define NVR_RELAY_OUT       (8)
    #else
    //_IPX_32P5
        #define DVR_ALARM_IN		(16)
        #define DVR_ALARM_OUT		(16)
        #define NVR_RELAY_OUT       (8)
    #endif
    
#define AUDIO_IN_CNT		(NUM_AUDIO_INPUT)
#define CAM_ALARM_IN		(32)
#define CAM_ALARM_OUT		(32)


#define SPOTSD_TAB1_PORT_CNT        (1)
#define SPOTSD_TAB1_MAX_DIV         (32)
#define SPOTSD_TAB1_OUTPUT_CH       (0xffffffff)
#define SPOTSD_TAB1_DATA_IDX        (0)
#define SPOTSD_TAB1_TITLE           "SPOT1 - 32DIV"
#define SPOTSD_TAB2_PORT_CNT        (0)
#define SPOTSD_TAB2_MAX_DIV         (0)
#define SPOTSD_TAB2_OUTPUT_CH       (0xffffffff)
#define SPOTSD_TAB2_DATA_IDX        (1)
#define SPOTSD_TAB2_TITLE           "SPOT2 - 1DIV"

#define SPOTAUX_TAB1_PORT_CNT       (0)
#define SPOTAUX_TAB1_MAX_DIV        (0)
#define SPOTAUX_TAB1_OUTPUT_CH      (0)
#define SPOTAUX_TAB1_DATA_IDX       (0)
#define SPOTAUX_TAB1_TITLE          ""
#define SPOTAUX_TAB2_PORT_CNT       (0)
#define SPOTAUX_TAB2_MAX_DIV        (0)
#define SPOTAUX_TAB2_OUTPUT_CH      (0)
#define SPOTAUX_TAB2_DATA_IDX       (1)
#define SPOTAUX_TAB2_TITLE          ""

#define SPOTHD_TAB1_PORT_CNT        (0)
#define SPOTHD_TAB1_MAX_DIV         (0)
#define SPOTHD_TAB1_TITLE           ""
#define SPOTHD_TAB2_PORT_CNT        (0)
#define SPOTHD_TAB2_MAX_DIV         (0)
#define SPOTHD_TAB2_TITLE           ""

#define SPOTDUAL_PORT_CNT           (1)

#elif GUI_16CH_SUPPORT
#define AUDIO_IN_CNT		(NUM_AUDIO_INPUT)
#define CAM_ALARM_IN		(16)
#define CAM_ALARM_OUT		(16)
#define NVR_RELAY_OUT       (8)

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
    #if defined(_IPX_1648P4E)
    #define DVR_ALARM_IN		(16)
    #define DVR_ALARM_OUT		(16)
    #elif defined(_IPX_1648M4E)
    #define DVR_ALARM_IN		(8)
    #define DVR_ALARM_OUT		(1)
    #else
    #define DVR_ALARM_IN		(16)
    #define DVR_ALARM_OUT		(1)
    #endif
#else
    #define DVR_ALARM_IN		(2)
    #define DVR_ALARM_OUT		(1)
#endif

#define SPOTSD_TAB1_PORT_CNT        (1)
#define SPOTSD_TAB1_MAX_DIV         (4)
#define SPOTSD_TAB1_OUTPUT_CH       (0xffff)
#define SPOTSD_TAB1_DATA_IDX        (0)
#define SPOTSD_TAB1_TITLE           "SPOT1 - 4DIV"
#define SPOTSD_TAB2_PORT_CNT        (0)
#define SPOTSD_TAB2_MAX_DIV         (0)
#define SPOTSD_TAB2_OUTPUT_CH       (0x000f)
#define SPOTSD_TAB2_DATA_IDX        (1)
#define SPOTSD_TAB2_TITLE           "SPOT2 - 1DIV"

#define SPOTAUX_TAB1_PORT_CNT       (0)
#define SPOTAUX_TAB1_MAX_DIV        (0)
#define SPOTAUX_TAB1_OUTPUT_CH      (0)
#define SPOTAUX_TAB1_DATA_IDX       (0)
#define SPOTAUX_TAB1_TITLE          ""
#define SPOTAUX_TAB2_PORT_CNT       (0)
#define SPOTAUX_TAB2_MAX_DIV        (0)
#define SPOTAUX_TAB2_OUTPUT_CH      (0)
#define SPOTAUX_TAB2_DATA_IDX       (1)
#define SPOTAUX_TAB2_TITLE          ""

#define SPOTHD_TAB1_PORT_CNT        (0)
#define SPOTHD_TAB1_MAX_DIV         (0)
#define SPOTHD_TAB1_TITLE           ""
#define SPOTHD_TAB2_PORT_CNT        (0)
#define SPOTHD_TAB2_MAX_DIV         (0)
#define SPOTHD_TAB2_TITLE           ""

#define SPOTDUAL_PORT_CNT           (0)

#elif GUI_8CH_SUPPORT
#define AUDIO_IN_CNT		(NUM_AUDIO_INPUT)
#define CAM_ALARM_IN		(8)
#define CAM_ALARM_OUT		(8)
#define NVR_RELAY_OUT       (4)

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
    #if defined(_IPX_0824P4E)
    #define DVR_ALARM_IN		(8)    //must use "var_get_dvr_alarmIn_cnt()" in m4
    #define DVR_ALARM_OUT		(8)
    #else
    #define DVR_ALARM_IN		(8) //must use "var_get_dvr_alarmIn_cnt()" in m4
    #define DVR_ALARM_OUT		(1)
    #endif
#else
    #define DVR_ALARM_IN		(2)
    #define DVR_ALARM_OUT		(1)
#endif

#define SPOTSD_TAB1_PORT_CNT        (0)
#define SPOTSD_TAB1_MAX_DIV         (8)
#define SPOTSD_TAB1_OUTPUT_CH       (0x000f)
#define SPOTSD_TAB1_DATA_IDX        (0)
#define SPOTSD_TAB1_TITLE           "SPOT1 - 8DIV"
#define SPOTSD_TAB2_PORT_CNT        (0)
#define SPOTSD_TAB2_MAX_DIV         (0)
#define SPOTSD_TAB2_OUTPUT_CH       (0x000f)
#define SPOTSD_TAB2_DATA_IDX        (1)
#define SPOTSD_TAB2_TITLE           "SPOT2 - 1DIV"

#define SPOTAUX_TAB1_PORT_CNT       (0)
#define SPOTAUX_TAB1_MAX_DIV        (0)
#define SPOTAUX_TAB1_OUTPUT_CH      (0)
#define SPOTAUX_TAB1_DATA_IDX       (0)
#define SPOTAUX_TAB1_TITLE          ""
#define SPOTAUX_TAB2_PORT_CNT       (0)
#define SPOTAUX_TAB2_MAX_DIV        (0)
#define SPOTAUX_TAB2_OUTPUT_CH      (0)
#define SPOTAUX_TAB2_DATA_IDX       (1)
#define SPOTAUX_TAB2_TITLE          ""

#define SPOTHD_TAB1_PORT_CNT        (0)
#define SPOTHD_TAB1_MAX_DIV         (0)
#define SPOTHD_TAB1_TITLE           ""
#define SPOTHD_TAB2_PORT_CNT        (0)
#define SPOTHD_TAB2_MAX_DIV         (0)
#define SPOTHD_TAB2_TITLE           ""

#define SPOTDUAL_PORT_CNT           (0)

#else
#define AUDIO_IN_CNT		(NUM_AUDIO_INPUT)

#define CAM_ALARM_IN		(4)
#define CAM_ALARM_OUT		(4)
#define NVR_RELAY_OUT       (0)

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
    #define DVR_ALARM_IN		(4) //must use "var_get_dvr_alarmIn_cnt()" in m4
    #define DVR_ALARM_OUT		(1)
#else
    #define DVR_ALARM_IN		(2)
    #define DVR_ALARM_OUT		(1)
#endif

#define SPOTSD_TAB1_PORT_CNT        (0)
#define SPOTSD_TAB1_MAX_DIV         (4)
#define SPOTSD_TAB1_OUTPUT_CH       (0x000f)
#define SPOTSD_TAB1_DATA_IDX        (0)
#define SPOTSD_TAB1_TITLE           "SPOT1 - 4DIV"
#define SPOTSD_TAB2_PORT_CNT        (0)
#define SPOTSD_TAB2_MAX_DIV         (0)
#define SPOTSD_TAB2_OUTPUT_CH       (0x000f)
#define SPOTSD_TAB2_DATA_IDX        (1)
#define SPOTSD_TAB2_TITLE           "SPOT2 - 1DIV"

#define SPOTAUX_TAB1_PORT_CNT       (0)
#define SPOTAUX_TAB1_MAX_DIV        (0)
#define SPOTAUX_TAB1_OUTPUT_CH      (0)
#define SPOTAUX_TAB1_DATA_IDX       (0)
#define SPOTAUX_TAB1_TITLE          ""
#define SPOTAUX_TAB2_PORT_CNT       (0)
#define SPOTAUX_TAB2_MAX_DIV        (0)
#define SPOTAUX_TAB2_OUTPUT_CH      (0)
#define SPOTAUX_TAB2_DATA_IDX       (1)
#define SPOTAUX_TAB2_TITLE          ""

#define SPOTHD_TAB1_PORT_CNT        (0)
#define SPOTHD_TAB1_MAX_DIV         (0)
#define SPOTHD_TAB1_TITLE           ""
#define SPOTHD_TAB2_PORT_CNT        (0)
#define SPOTHD_TAB2_MAX_DIV         (0)
#define SPOTHD_TAB2_TITLE           ""

#define SPOTDUAL_PORT_CNT           (0)

#endif

#define ALARM_OUT_COUNT		(CAM_ALARM_OUT + DVR_ALARM_OUT)
#define ALARM_IN_COUNT		(CAM_ALARM_IN + DVR_ALARM_IN)

#define RS485_CNT                   (1)
#define FAN_CNT                     (2)
#define RS232_CNT                   (1)

#else
  #error "undefined"
#endif  /**/

#define SPOTSD_PORT_CNT             (SPOTSD_TAB1_PORT_CNT+SPOTSD_TAB2_PORT_CNT)
#define SPOTAUX_PORT_CNT            (SPOTAUX_TAB1_PORT_CNT+SPOTAUX_TAB2_PORT_CNT)
#define SPOTHD_PORT_CNT             (SPOTHD_TAB1_PORT_CNT+SPOTHD_TAB2_PORT_CNT)



#define MASK_SYSTEMCATE(m)          (m & 0xFF000000)
#define MASK_MODELCATE(m)           (m & 0x00FFFF00)

#define IS_SYSTEMCATE(m, n)         (MASK_SYSTEMCATE(m) & n)
#define IS_MODELCATE(m, n)          (MASK_MODELCATE(m) & n)

#define IS_MODELMATCH(m, n)         (m == n)


#define DVR_CATEGORY	            (0x1000000)
#define NVR_CATEGORY	            (0x2000000)
//#define HYBRID_CATEGORY	            (0x4000000)

enum {
	IPX_CATEGORY	    = 0x100 | NVR_CATEGORY,
	IPX_L_MODEL,
	IPX_P_MODEL,

	IPX_ECO_CATEGORY	= 0x200 | NVR_CATEGORY,
	IPX_L_ECO_MODEL,
	IPX_P_ECO_MODEL,

	IPX3_CATEGORY	    = 0x400 | NVR_CATEGORY,
	IPX_L3_MODEL,
	IPX_P3_MODEL,
	IPX_M4_MODEL,

	IPX3_ECO_CATEGORY	= 0x800 | NVR_CATEGORY,
	IPX_L3_ECO_MODEL,
	IPX_P3_ECO_MODEL,
	
	IPX4E_CATEGORY	    = 0x1000 | NVR_CATEGORY,
	IPX_P4E_MODEL,	

	DVR4G_CATEGORY	    = 0x10000 | DVR_CATEGORY,
	DVR4G_UTM_MODEL,
	DVR4G_ANF_MODEL,
	DVR4G_ATM_MODEL,

    IPX5_CATEGORY    = 0x20000 | NVR_CATEGORY,
    IPX_P5_MODEL
};

typedef struct _DESC_SWVER_T {
    gint    product;
    gint    procotol;
    gint    minor;
    gint    vendor;
    gchar   option[128];
} DESC_SWVER_T;

typedef struct _DESC_MENU_T {
    guint submenu_camera;
    guint submenu_disp;
    guint submenu_sound;
    guint submenu_user;
    guint submenu_network;
    guint submenu_system;
    guint submenu_storage;
    guint submenu_storage_del;
    guint submenu_event;
    guint submenu_event_noti;
    guint submenu_alarm_sensor_evt;
    guint recmenu;
    guint archmenu;
    guint searchmenu;
    guint userguide;
} DESC_MENU_T;

typedef struct _DDNSSERVER_PROP_T {
    gchar name[32];
    gint on_id;
    gint on_pw;
    gint on_nvr;
    gint on_mac;
    gint on_status;
} DDNSSERVER_PROP_T;

typedef struct _FUNC_DDNS_T {
    gint support;
    gint cnt;
    DDNSSERVER_PROP_T server[8];
} FUNC_DDNS_T;

typedef struct _FUNC_BACKDOORPW_T {
    gint support;
    gchar password[64];
} FUNC_BACKDOORPW_T;

typedef struct _FUNC_BUZZER_T {
    gint support_keypad;
    gint support_remocon;
} FUNC_BUZZER_T;

typedef struct _FUNC_CHKEVT_T {
    gint support_fan;
    gint support_temperature;
    gint support_poe;
} FUNC_CHKEVT_T;

typedef struct _FUNC_DUALMONITOR_T {
    gint support;
    gint basic_type;
    gint advance_type;
} FUNC_DUALMONITOR_T;

typedef struct _FUNC_FAKEVER_T {
    gint support;
    gchar ver[64];
} FUNC_FAKEVER_T;

typedef struct _FUNC_SEQURINET_T {
    gint support;
    gint support_easyip;
    gint support_p2p;
    gint support_mobilepush;
} FUNC_SEQURINET_T;

typedef struct _FUNC_USERGUIDED_T {
    gint support;
    gint cnt;
    gchar title[16][64];
} FUNC_USERGUIDE_T;

typedef struct _FUNC_PIN_T {
    gint support;
    gint digit;
    gint id_input_method;
} FUNC_PIN_T;

typedef gint (*WIZARD_SETUP_FUNC)(gpointer parent, gpointer user_data);

typedef struct _WIZARD_SETUP_T {
    gint use_numbering;
    WIZARD_SETUP_FUNC func;
} WIZARD_SETUP_T;

typedef struct _LANGUAGE_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} LANGUAGE_WIZARD_T;

typedef struct _SIGTYPE_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} SIGTYPE_WIZARD_T;

typedef struct _WELCOME_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} WELCOME_WIZARD_T;

typedef struct _PASSWORD_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} PASSWORD_WIZARD_T;

typedef struct _DATETIME_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} DATETIME_WIZARD_T;

typedef struct _RECORD_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} RECORD_WIZARD_T;

typedef struct _NETWORK_WIZARD_T {
    gint support;
    WIZARD_SETUP_T setup[16];
} NETWORK_WIZARD_T;

typedef struct _FUNC_WIZARD_T {
    gint support;
    LANGUAGE_WIZARD_T language;
    SIGTYPE_WIZARD_T sigtype;
    WELCOME_WIZARD_T welcome;
    PASSWORD_WIZARD_T password;
    DATETIME_WIZARD_T datetime;
    RECORD_WIZARD_T record;
    NETWORK_WIZARD_T network;
} FUNC_WIZARD_T;

typedef struct _FUNC_AI_T {
    gint support;
    gint support_calibration;
} FUNC_AI_T;

typedef struct _DESC_FUNC_T {
    gint support_analog_type;
    gint support_archdata_playback;
    gint support_advanced_chswitch;
    gint support_aspect_ratio;
    gint support_audio;
    gint support_bgvlayer;
    gint support_camtype;
	gint support_ptzsetup;
    gint support_dmva_itx;
    gint support_dmva_s1;
    gint support_dlva_itx;
    gint support_aicam_itx;
    gint support_aibox_itx;
    gint support_dl_timeline;
    gint support_hdspot;
    gint support_hdsdi_outmode;
    gint support_install_cctvmode;
    gint support_install_openmode;
    gint support_ipcamfwup;
    gint support_ipcamlist;
    gint support_keyctrl;
    gint support_loopout;
    gint support_mobilepush;
    gint support_monitorout;
    gint support_network_security;
    gint support_onvifsetup;
    gint support_p2p;
    gint support_posevent;
    gint support_privacymask;
    gint support_fisheye;
    gint support_protect;
    gint support_raid;
    gint support_rec_audiomap;
    gint support_rec_calc;
    gint support_rec_validity;
    gint support_secomdual;
    gint support_smsevent;
    gint support_snmp;
    gint support_spot;
    gint support_standby;
    gint support_system_security;
    gint support_swmode;
    gint support_encoder_mode;
    gint support_noise;
    gint support_al_switch;
    gint support_double_login;
    gint support_checkpw_search_arch;
    gint support_ext_disk;
    gint support_swsignal;
    gint support_tamper;
    gint support_cable_test;
	gint support_remocon_id;
    gint support_video_erase;
	gint support_usrdef_holiday;
	gint support_find_passwd;
	gint support_license;
    gint support_dlva_counter;
	gint support_auth_popup_hide;
    gint support_license_plate;
    gint support_face;
    gint support_qna;
	gint support_double_knock;
    gint support_autologout_on_reboot;
    gint support_forgot_pw_warning_box;
    gint support_corridor;
	
    FUNC_BACKDOORPW_T backdoorpw;
    FUNC_BUZZER_T buzzer;
    FUNC_CHKEVT_T chkevt;
    FUNC_DDNS_T ddns;
    FUNC_DUALMONITOR_T dualmonitor;
    FUNC_FAKEVER_T fakefwver;
    FUNC_FAKEVER_T fakehwver;
    FUNC_SEQURINET_T sequrinet;
    FUNC_USERGUIDE_T userguide;
    FUNC_WIZARD_T wizard;
    FUNC_AI_T ai;
    FUNC_PIN_T pin;

    gint support_dbg_obj;
} DESC_FUNC_T;

typedef struct _VWDESC_T {
    guint       model_code;
    guint       vendor_code;
    DESC_SWVER_T dswver;
    DESC_FUNC_T dfunc;
    DESC_MENU_T dmenu;
} VWDESC_T;

extern VWDESC_T ivsc;

gint vw_desc_init();

#endif
