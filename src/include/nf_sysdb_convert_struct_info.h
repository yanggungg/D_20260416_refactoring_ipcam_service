#ifndef __NF_SYSDB_CONVERT_STRUCT_INFO_H__
#define __NF_SYSDB_CONVERT_STRUCT_INFO_H__

typedef struct _SYSDB_SYS_INFO_PTZ_ARR_T {
    char         f_name[16];
} SYSDB_SYS_INFO_PTZ_ARR;

typedef struct _SYSDB_SYS_INFO_KEYCTRL_ARR_T {
    char         f_name[16];
} SYSDB_SYS_INFO_KEYCTRL_ARR;

typedef struct _SYSDB_DISK_INREC_ARR_T {
    unsigned int f_size;
    unsigned int f_if;
    unsigned int f_loc;
    char         f_model[64];
} SYSDB_DISK_INREC_ARR;

typedef struct _SYSDB_DISK_ARCH_ARR_T {
    unsigned int f_size;
    unsigned int f_if;
    unsigned int f_loc;
    char         f_model[64];
} SYSDB_DISK_ARCH_ARR;

typedef struct _SYSDB_DISK_EXTREC_ARR_T {
    unsigned int f_size;
    unsigned int f_if;
    unsigned int f_loc;
    char         f_model[64];
} SYSDB_DISK_EXTREC_ARR;

typedef struct _SYSDB_DISK_LIST_ARR_T {
    char         f_name[64];
} SYSDB_DISK_LIST_ARR;

typedef struct _SYSDB_CAM_ARR_T {
    char         f_title[32];
    char         f_title_desc[64];
    unsigned int f_bright;
    unsigned int f_contrast;
    unsigned int f_tint;
    unsigned int f_color;
    unsigned int f_covert;
    unsigned int f_audio_ch;
} SYSDB_CAM_ARR;

typedef struct _SYSDB_CAM_PTZ_ARR_T {
    unsigned int f_channel;
    unsigned int f_addr;
    unsigned int f_protocol;
    unsigned int f_baud;
    unsigned int f_auto_focus;
    unsigned int f_auto_iris;
    unsigned int f_zoom_spd;
    unsigned int f_focus_spd;
    unsigned int f_iris_spd;
    unsigned int f_preset_spd;
    unsigned int f_pt_spd;
    unsigned int f_swing_spd;
    unsigned int f_ucmd1_num;
    unsigned int f_ucmd2_num;
    char         f_ucmd1_str[32];
    char         f_ucmd2_str[32];
    char         f_model[64];
    unsigned int f_swing_ch;
    unsigned int f_preset_PCNT;
    char         f_preset_P0_name[16];
    char         f_preset_P1_name[16];
    char         f_preset_P2_name[16];
    char         f_preset_P3_name[16];
    char         f_preset_P4_name[16];
    char         f_preset_P5_name[16];
    char         f_preset_P6_name[16];
    char         f_preset_P7_name[16];
    char         f_preset_P8_name[16];
    char         f_preset_P9_name[16];
    char         f_preset_P10_name[16];
    char         f_preset_P11_name[16];
    char         f_preset_P12_name[16];
    char         f_preset_P13_name[16];
    char         f_preset_P14_name[16];
    char         f_preset_P15_name[16];
} SYSDB_CAM_PTZ_ARR;

typedef struct _SYSDB_USR_GRP_ARR_T {
    char         f_name[32];
    char         f_desc[64];
    unsigned int f_auth_setup;
    unsigned int f_auth_search;
    unsigned int f_auth_archive;
    unsigned int f_auth_ptz;
    unsigned int f_auth_alarm_off;
    unsigned int f_auth_panic_rec;
    unsigned int f_auth_remote;
    unsigned int f_auth_func0;
    unsigned int f_auth_func1;
    unsigned int f_auth_func2;
    unsigned int f_auth_func3;
} SYSDB_USR_GRP_ARR;

typedef struct _SYSDB_USR_ARR_T {
    char         f_name[32];
    char         f_pass[32];
    char         f_email[64];
    unsigned int f_email_notify;
    char         f_desc[64];
    char         f_grpname[32];
    unsigned int f_covert;
} SYSDB_USR_ARR;

typedef struct _SYSDB_ALARM_SENSOR_ARR_T {
    unsigned int f_act;
    unsigned int f_oper;
    char         f_desc[64];
} SYSDB_ALARM_SENSOR_ARR;

typedef struct _SYSDB_ALARM_MOTION_ARR_T {
    unsigned int f_act;
    unsigned int f_sense;
    char         f_area[528];
} SYSDB_ALARM_MOTION_ARR;

typedef struct _SYSDB_ACT_RELAY_ARR_T {
    unsigned int f_act;
    unsigned int f_op_type;
    unsigned int f_hdd_event;
    unsigned int f_dwell_type;
    unsigned int f_dwell_time;
    char         f_alarm[16];
    char         f_vloss[16];
    char         f_motion[16];
} SYSDB_ACT_RELAY_ARR;

typedef struct _SYSDB_DISP_MAIN_ARR_T {
    char         f_name[32];
    char         f_creator[32];
    unsigned int f_valid_mode;
    unsigned int f_type_TCNT;
    unsigned int f_type_T0_type;
    char         f_type_T0_ch[32];
    unsigned int f_type_T1_type;
    char         f_type_T1_ch[32];
    unsigned int f_type_T2_type;
    char         f_type_T2_ch[32];
    unsigned int f_type_T3_type;
    char         f_type_T3_ch[32];
    unsigned int f_type_T4_type;
    char         f_type_T4_ch[32];
    unsigned int f_type_T5_type;
    char         f_type_T5_ch[32];
    unsigned int f_type_T6_type;
    char         f_type_T6_ch[32];
    unsigned int f_type_T7_type;
    char         f_type_T7_ch[32];
    unsigned int f_type_T8_type;
    char         f_type_T8_ch[32];
    unsigned int f_type_T9_type;
    char         f_type_T9_ch[32];
    unsigned int f_type_T10_type;
    char         f_type_T10_ch[32];
    unsigned int f_type_T11_type;
    char         f_type_T11_ch[32];
    unsigned int f_type_T12_type;
    char         f_type_T12_ch[32];
    unsigned int f_type_T13_type;
    char         f_type_T13_ch[32];
    unsigned int f_type_T14_type;
    char         f_type_T14_ch[32];
    unsigned int f_type_T15_type;
    char         f_type_T15_ch[32];
} SYSDB_DISP_MAIN_ARR;

typedef struct _SYSDB_DISP_SPOT_ARR_T {
    char         f_title[32];
    unsigned int f_status;
    unsigned int f_time;
    unsigned int f_border;
    unsigned int f_alpha;
    char         f_ch[16];
    char         f_name[32];
    char         f_creator[32];
    unsigned int f_valid_mode;
    unsigned int f_type_TCNT;
    unsigned int f_type_T0_type;
    char         f_type_T0_ch[32];
    unsigned int f_type_T1_type;
    char         f_type_T1_ch[32];
    unsigned int f_type_T2_type;
    char         f_type_T2_ch[32];
    unsigned int f_type_T3_type;
    char         f_type_T3_ch[32];
    unsigned int f_type_T4_type;
    char         f_type_T4_ch[32];
    unsigned int f_type_T5_type;
    char         f_type_T5_ch[32];
    unsigned int f_type_T6_type;
    char         f_type_T6_ch[32];
    unsigned int f_type_T7_type;
    char         f_type_T7_ch[32];
    unsigned int f_type_T8_type;
    char         f_type_T8_ch[32];
    unsigned int f_type_T9_type;
    char         f_type_T9_ch[32];
    unsigned int f_type_T10_type;
    char         f_type_T10_ch[32];
    unsigned int f_type_T11_type;
    char         f_type_T11_ch[32];
    unsigned int f_type_T12_type;
    char         f_type_T12_ch[32];
    unsigned int f_type_T13_type;
    char         f_type_T13_ch[32];
    unsigned int f_type_T14_type;
    char         f_type_T14_ch[32];
    unsigned int f_type_T15_type;
    char         f_type_T15_ch[32];
} SYSDB_DISP_SPOT_ARR;

typedef struct _SYSDB_REC_MOTION_ARR_T {
    char         f_size[384];
    char         f_fps[384];
    char         f_quality[384];
    char         f_audio[384];
    char         f_mode[384];
} SYSDB_REC_MOTION_ARR;

typedef struct _SYSDB_REC_ALARM_ARR_T {
    char         f_size[384];
    char         f_fps[384];
    char         f_quality[384];
    char         f_audio[384];
    char         f_mode[384];
} SYSDB_REC_ALARM_ARR;

typedef struct _SYSDB_IPCAM_ARR_T {
    char         f_model[32];
    char         f_mac[32];
    char         f_title[32];
    char         f_title_desc[64];
    unsigned int f_covert;
    char         f_codec[32];
    char         f_size[32];
    unsigned int f_audio_rx;
    unsigned int f_audio_tx;
    unsigned int f_alarm;
    unsigned int f_motion;
    char         f_rx_method[16];
    char         f_rtsp_url[256];
    unsigned int f_rtsp_port;
    char         f_http_url[256];
    unsigned int f_http_port;
    unsigned int f_vudp_addr;
    unsigned int f_vudp_port;
    unsigned int f_audp_addr;
    unsigned int f_audp_port;
    unsigned int f_use_master;
    char         f_username[32];
    char         f_password[32];
} SYSDB_IPCAM_ARR;

typedef struct _SYSDB_IPREC_MOTION_ARR_T {
    char         f_size[384];
    char         f_fps[384];
    char         f_quality[384];
    char         f_audio[384];
    char         f_mode[384];
} SYSDB_IPREC_MOTION_ARR;

typedef struct _SYSDB_IPREC_ALARM_ARR_T {
    char         f_size[384];
    char         f_fps[384];
    char         f_quality[384];
    char         f_audio[384];
    char         f_mode[384];
} SYSDB_IPREC_ALARM_ARR;

typedef struct _SYSDB_SYS_T {
    char         f_test_string[8];
    unsigned int f_test_uint;
    int          f_test_int;
    unsigned int f_test_bool;
    char         f_info_sysid[64];
    char         f_info_swver[64];
    char         f_info_hwver[64];
    char         f_info_mac[12];
    char         f_info_reserved[4];
    char         f_info_site[64];
    unsigned int f_info_passwd_enable;
    unsigned int f_info_sig_type;
    unsigned int f_PCNT;
    SYSDB_SYS_INFO_PTZ_ARR   arr_p[64];
    unsigned int f_KCNT;
    SYSDB_SYS_INFO_KEYCTRL_ARR   arr_k[32];
    unsigned int f_date_daylight;
    unsigned int f_date_timeform;
    unsigned int f_date_dateform;
    unsigned int f_date_tz_index;
    unsigned int f_date_ltime;
    char         f_date_timesvr[64];
    char         f_date_tz_name[64];
    unsigned int f_date_timesvr_update;
    unsigned int f_ctrl_addr;
    unsigned int f_ctrl_protocol;
    unsigned int f_ctrl_baud;
} SYSDB_SYS;

typedef struct _SYSDB_NET_T {
    unsigned int f_proto_dhcpon;
    unsigned int f_proto_ddnson;
    unsigned int f_proto_webon;
    unsigned int f_proto_clienton;
    unsigned int f_proto_ipaddr;
    unsigned int f_proto_gateway;
    unsigned int f_proto_subnet;
    unsigned int f_proto_dns1;
    unsigned int f_proto_dns2;
    unsigned int f_proto_clientport;
    unsigned int f_proto_webport;
    unsigned int f_proto_maxtxspeed;
    char         f_proto_ddnssvr[64];
    char         f_email_id[64];
    char         f_email_passwd[32];
    char         f_email_smtpsvr[64];
    unsigned int f_email_smtpport;
    unsigned int f_email_ssl;
    char         f_email_testmail[64];
    char         f_email_from[64];
    unsigned int f_dhcp_email_notify;
    unsigned int f_dhcp_lease_timer;
    unsigned int f_dhcp_timestamp;
    unsigned int f_dhcp_ipaddr;
    unsigned int f_dhcp_gateway;
    unsigned int f_dhcp_subnet;
    unsigned int f_dhcp_dns1;
    unsigned int f_dhcp_dns2;
    char         f_ddns_type[32];
    char         f_ddns_hostname[64];
    char         f_ddns_username[64];
    char         f_ddns_passwd[32];
    char         f_ddns_email[64];
    char         f_ddns_key[64];
    unsigned int f_ddns_update;
    unsigned int f_pppoe_pppoeon;
    unsigned int f_pppoe_emailon;
    char         f_pppoe_username[32];
    char         f_pppoe_passwd[32];
    unsigned int f_pppoe_persist;
    unsigned int f_pppoe_keepalive;
    unsigned int f_socks_sockson;
    char         f_socks_server[64];
    unsigned int f_socks_port;
    unsigned int f_socks_type;
    char         f_socks_localnet[256];
    char         f_socks_username[32];
    char         f_socks_passwd[32];
    unsigned int f_syslog_syslogon;
    char         f_syslog_host[64];
    unsigned int f_syslog_port;
    unsigned int f_syslog_level;
    unsigned int f_syslog_mask;
} SYSDB_NET;

typedef struct _SYSDB_AUDIO_T {
    unsigned int f_live;
    unsigned int f_ch;
    unsigned int f_tx;
    unsigned int f_rx;
    unsigned int f_keybuzzer;
} SYSDB_AUDIO;

typedef struct _SYSDB_DISK_T {
    unsigned int f_util_format_target;
    unsigned int f_util_format_type;
    unsigned int f_util_chkdsk_target;
    unsigned int f_util_chkdsk_type;
    unsigned int f_write_mode;
    unsigned int f_rtime_limit;
    unsigned int f_inrec_wmode;
    unsigned int f_ICNT;
    SYSDB_DISK_INREC_ARR   arr_i[4];
    unsigned int f_arch_wmode;
    unsigned int f_ACNT;
    SYSDB_DISK_ARCH_ARR   arr_a[4];
    unsigned int f_extrec_wmode;
    unsigned int f_ECNT;
    SYSDB_DISK_EXTREC_ARR   arr_e[4];
    unsigned int f_LCNT;
    SYSDB_DISK_LIST_ARR   arr_l[16];
} SYSDB_DISK;

typedef struct _SYSDB_CAM_T {
    unsigned int f_CCNT;
    SYSDB_CAM_ARR   arr_c[16];
    unsigned int f_PCNT;
    SYSDB_CAM_PTZ_ARR   arr_p[16];
} SYSDB_CAM;

typedef struct _SYSDB_USR_T {
    unsigned int f_GCNT;
    SYSDB_USR_GRP_ARR   arr_g[3];
    unsigned int f_UCNT;
    SYSDB_USR_ARR   arr_u[8];
    unsigned int f_auto_logout;
    unsigned int f_auto_logout_min;
} SYSDB_USR;

typedef struct _SYSDB_ALARM_T {
    int          f_strg_temper;
    unsigned int f_strg_temper_mode;
    unsigned int f_strg_dskfull;
    unsigned int f_strg_over_temp;
    unsigned int f_strg_smart_warn;
    unsigned int f_strg_smart_chk;
    unsigned int f_SCNT;
    SYSDB_ALARM_SENSOR_ARR   arr_s[16];
    unsigned int f_MCNT;
    SYSDB_ALARM_MOTION_ARR   arr_m[16];
} SYSDB_ALARM;

typedef struct _SYSDB_ACT_T {
    unsigned int f_RCNT;
    SYSDB_ACT_RELAY_ARR   arr_r[16];
    unsigned int f_buzzer_act;
    unsigned int f_buzzer_hdd_event;
    unsigned int f_buzzer_dwell_type;
    unsigned int f_buzzer_dwell_time;
    char         f_buzzer_alarm[16];
    char         f_buzzer_vloss[16];
    char         f_buzzer_motion[16];
    unsigned int f_email_act;
    unsigned int f_email_hdd_event;
    unsigned int f_email_booting;
    unsigned int f_email_setup_chg;
    unsigned int f_email_frequency;
    char         f_email_alarm[16];
    char         f_email_vloss[16];
    char         f_email_motion[16];
} SYSDB_ACT;

typedef struct _SYSDB_DISP_T {
    char         f_osd_lang[32];
    unsigned int f_osd_cam_title;
    unsigned int f_osd_status_bar;
    unsigned int f_osd_event_icon;
    unsigned int f_osd_alarm_msg;
    unsigned int f_osd_menu_alpha;
    unsigned int f_osd_border;
    unsigned int f_osd_border_color;
    unsigned int f_osd_motion;
    unsigned int f_osd_motion_color;
    unsigned int f_osd_motion_alpha;
    unsigned int f_monitor_seq_dwell;
    unsigned int f_monitor_spot_dwell;
    unsigned int f_monitor_deinterace;
    unsigned int f_monitor_alarm_popup;
    unsigned int f_monitor_alarm_dwell;
    unsigned int f_monitor_motion_popup;
    unsigned int f_monitor_motion_dwell;
    unsigned int f_MCNT;
    SYSDB_DISP_MAIN_ARR   arr_m[4];
    unsigned int f_SCNT;
    SYSDB_DISP_SPOT_ARR   arr_s[4];
} SYSDB_DISP;

typedef struct _SYSDB_REC_T {
    unsigned int f_mode;
    unsigned int f_sched_mode;
    unsigned int f_pre_rec_time;
    unsigned int f_post_rec_time;
    unsigned int f_MCNT;
    SYSDB_REC_MOTION_ARR   arr_m[8];
    unsigned int f_ACNT;
    SYSDB_REC_ALARM_ARR   arr_a[8];
    char         f_panic_size[16];
    char         f_panic_fps[16];
    char         f_panic_quality[16];
    char         f_panic_audio[16];
} SYSDB_REC;

typedef struct _SYSDB_IPCAM_T {
    char         f_username[32];
    char         f_password[32];
    unsigned int f_CCNT;
    SYSDB_IPCAM_ARR   arr_c[4];
} SYSDB_IPCAM;

typedef struct _SYSDB_IPREC_T {
    unsigned int f_mode;
    unsigned int f_sched_mode;
    unsigned int f_pre_rec_time;
    unsigned int f_post_rec_time;
    unsigned int f_MCNT;
    SYSDB_IPREC_MOTION_ARR   arr_m[8];
    unsigned int f_ACNT;
    SYSDB_IPREC_ALARM_ARR   arr_a[8];
    char         f_panic_size[16];
    char         f_panic_fps[16];
    char         f_panic_quality[16];
    char         f_panic_audio[16];
} SYSDB_IPREC;

#endif

