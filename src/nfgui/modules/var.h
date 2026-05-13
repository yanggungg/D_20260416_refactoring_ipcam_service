/*
 * var.h
 * 	- system variables
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 18, 2011
 *
 */

#ifndef __VAR_H
#define __VAR_H

#include "iux_afx.h"
#include "nfdal.h"
#include "nf_afx.h"


#define MAX_SUPPORT_DDNS        8

typedef struct _DDNS_CFG_T {
	gchar server[STRING_SIZE_32];
	int on_id;
	int on_pwd;
	int on_nvr;
	int on_mac;
	int on_status;
} DDNS_CFG_T;

typedef struct _SEQURINET_CFG_T {
	int use;
	int easyip;
	int p2p;
	int push;
} SEQURINET_CFG_T;

typedef enum {
	TEXT_CAM_NUM    = 0,
	TEXT_CAM_TITLE,
	TEXT_TYPE
}TEXT_CAMTITLE;

typedef struct _CONFLICT_INFOR_
{
    gchar   conflict_type[20][64];
    gchar   conflict_time[20][64];
    gchar   conflict_mac_addr[20][64];
    gint    cnt;

    guint   conflict;
}CONFLICT_INFOR;

static CONFLICT_INFOR ip_c;

typedef struct _VCT_RESULT_LABEL_
{
    gchar   vct_result[GUI_CHANNEL_CNT][STRING_SIZE_32];
    
}VCT_RESULT_LABEL;

static VCT_RESULT_LABEL vct_label;

////////////////////////////////////////////////////////////
//
// public interfaces
//

int var_init();
int var_get_vendor_code();
int var_get_ch_count();
int var_set_ch_count(int ch);
int var_set_detected_disk_count(int detected);
int var_set_running_disk_count(int running);
int var_set_disk_count(int detected, int running);
int var_get_detected_disk_count();
int var_get_running_disk_count();
int var_set_live_audio_ch(int ch);
int var_get_live_audio_ch();
BITMASK var_get_novideo_mask();
int var_set_novideo_mask(BITMASK mask);
int var_set_mic_state(ONOFF_E onoff);
int var_get_mic_state();
int var_set_mic_out_mask(BITMASK mask);
BITMASK var_get_mic_out_mask();
BITMASK var_get_ch_mask();
BITMASK var_get_ch_bit(int ch);
int var_set_full_scr_audio(ONOFF_E onoff);
ONOFF_E var_get_full_scr_audio();
int var_set_active_layout(gint active_idx);
int var_get_active_layout();
int var_get_fake_fwver(char *ver, int len);
int var_get_fake_hwver(char *ver, int len);
int var_set_external_addr(char *addr);
int var_get_external_addr(char *addr, int len);
int var_set_enable_remote_upgrade(gint enable);
int var_get_enable_remote_upgrade();
int var_set_detect_fwver(char *fwver);
int var_get_detect_fwver(char *fwver, int len);
int var_set_detect_cam_fwver(int ch, char *fwver);
int var_get_detect_cam_fwver(int ch, char *fwver, int len);
int var_set_camtitle();
int var_get_camtitle(char *strTitle, guint ch);
int var_get_covert_mask(BITMASK *mask);
int var_get_qr_url(gchar *qr_url, gint len);

int var_get_supported_audio();
int var_get_supported_dmva();
int var_get_supported_ipcam_fwup();
int var_get_supported_keyctrl();
int var_get_supported_mobilepush();
int var_get_supported_openmode();
int var_get_supported_p2p();
int var_get_supported_privacy_mask();
int var_get_supported_raid();
int var_get_supported_secom_dual();
int var_get_supported_sequrinet();
int var_get_supported_user_guide();
int var_get_supported_double_login();
int var_get_supported_checkpw_search_arch();
int var_get_supported_ext_disk();
int var_get_display_int_disk_count();
int var_get_supported_double_knock();

void var_set_update_ipconflict_list(CONFLICT_INFOR *set_ip);
void var_get_update_ipconflict_list(CONFLICT_INFOR *set_info);
void var_set_conflict_infor(int idx);
unsigned int var_get_conflict_infor();

int var_get_supported_backdoor_password();
int var_get_backdoor_password(char *pw);

int var_get_supported_ddns();
int var_get_ddns_cfg(DDNS_CFG_T *cfg, int idx);
int var_get_ddns_cnt();

#endif

