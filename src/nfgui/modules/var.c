/*
 * var.c
 *  - system variables
 *  - not saved in the DB
 *  - dependencies :
 *
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 18, 2011
 *
 */

#include <memory.h>
#include "var.h"
#include "vw_desc.h"
#include "iux_types.h"

#include "nf_common.h"
#include "nf_sysman.h"


////////////////////////////////////////////////////////////
//
// private data type
//

typedef struct _CAM_TITLE_T {
    gchar           titleStr[STRING_SIZE_CAMTITLE];
} CAM_TITLE_T;

typedef struct _VAR_T {
    int         ch;
    int         ddcnt;
    int         rdcnt;
    int         live_audio_ch;
    int         live_audio_stat;
    BITMASK     msk_mic;
    BITMASK     msk_novideo;
    int         mic_on;
    ONOFF_E     fscr_audio;
    int         active_layout;
    int         enable_remote_fwup;

    char        externalAddr[40];
    char        detect_fwver[40];
    char        detect_cam_fwver[GUI_CHANNEL_CNT][40];

    CAM_TITLE_T chTitle[GUI_CHANNEL_CNT];
} VAR_T;

static VAR_T    ivar;

////////////////////////////////////////////////////////////
//
// private interfaces
//

static int _init_default_value()
{
    int i;

    ivar.mic_on = 0;
    return 0;
}

static int _init_camtitle()
{
    int i;

    for (i=0; i<GUI_CHANNEL_CNT; i++)
    {
        DAL_get_camera_title(ivar.chTitle[i].titleStr, i);
    }
    return 0;
}

static BITMASK _get_covert_mask()
{
    gint i;
    gchar login_group[10];
    gchar *usr_name;

    BITMASK covert_mask = 0;
    CameraData camdata[GUI_CHANNEL_CNT];

    guint user_cnt;
    UserManageData userdata;

    memset(login_group, 0x00, sizeof(login_group));
    ssm_get_cur_group(login_group);

    usr_name = ssm_get_cur_id(NULL);

    for(i = 0; i < GUI_CHANNEL_CNT; i++)
        DAL_get_covert_data(&camdata[i], i);

    user_cnt = DAL_get_user_count();

    if (!strcmp(login_group, "ADMIN"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);
            if (!strcmp(userdata.id, usr_name)) break;
        }

        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].admin) || (userdata.covert[i] == '1'))  covert_mask |= (1 << i);
        }
    }
    else if (!strcmp(login_group, "MANAGER"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);
            if (!strcmp(userdata.id, usr_name)) break;
        }

        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].manager) || (userdata.covert[i] == '1'))    covert_mask |= (1 << i);
        }
    }
    else if (!strcmp(login_group, "USER"))
    {
        for (i = 0; i < user_cnt; i++)
        {
            DAL_get_userManage_data(&userdata, i);
            if (!strcmp(userdata.id, usr_name)) break;
        }

        if (i == user_cnt) {
            g_message("%s, %d not find :%s", __FUNCTION__, __LINE__, usr_name);
            return 0;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if ((camdata[i].user) || (userdata.covert[i] == '1'))   covert_mask |= (1 << i);
        }
    }
    else        // logoff
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (camdata[i].logoff)  covert_mask |= (1 << i);
        }
    }

    return covert_mask;
}




////////////////////////////////////////////////////////////
//
// public interfaces
//

int var_init()
{
    memset(&ivar, 0x00, sizeof(VAR_T));

    _init_camtitle();
    _init_default_value();

    ivar.fscr_audio = ONOFF_ON;
    ivar.active_layout = -1;

    return 0;
}

int var_get_vendor_code()
{
    return ivsc.dswver.vendor;
}

int var_get_ch_count()
{
    return ivar.ch;
}

int var_set_ch_count(int ch)
{
    ivar.ch = ch;
    return 0;
}

int var_set_detected_disk_count(int detected)
{
    ivar.ddcnt = detected;
    return 0;
}

int var_set_running_disk_count(int running)
{
    ivar.rdcnt = running;
    return 0;
}

int var_set_disk_count(int detected, int running)
{
    ivar.ddcnt = detected;
    ivar.rdcnt = running;
    return 0;
}

int var_get_detected_disk_count()
{
    return ivar.ddcnt;
}

int var_get_running_disk_count()
{
    return ivar.rdcnt;
}

int var_set_live_audio_ch(int ch)
{
    ivar.live_audio_ch = ch;
    return 0;
}

int var_get_live_audio_ch()
{
    return ivar.live_audio_ch;
}

int var_set_live_audio_stat(int onoff)
{
    ivar.live_audio_stat = onoff;
    return 0;
}

int var_get_live_audio_stat()
{
    return ivar.live_audio_stat;
}

BITMASK var_get_novideo_mask()
{
    return ivar.msk_novideo;
}

int var_set_novideo_mask(BITMASK mask)
{
    ivar.msk_novideo = mask;
    return 0;
}

int var_set_mic_state(ONOFF_E onoff)
{
    ivar.mic_on = onoff;
    return 0;
}

int var_get_mic_state()
{
    return ivar.mic_on;
}

int var_set_mic_out_mask(BITMASK mask)
{
    ivar.msk_mic = mask;
    return 0;
}

BITMASK var_get_mic_out_mask()
{
    return ivar.msk_mic;
}

BITMASK var_get_ch_mask()
{
    int i;
    BITMASK mask = 0;

    for (i = 0; i < var_get_ch_count(); ++i)
        mask |= (1 << i);

    return mask;
}

BITMASK var_get_ch_bit(int ch)
{
    BITMASK mask = 0;
    mask |= (1 << ch);
    return mask;
}

int var_set_full_scr_audio(ONOFF_E onoff)
{
    ivar.fscr_audio = onoff;
    return 0;
}

ONOFF_E var_get_full_scr_audio()
{
    return ivar.fscr_audio;
}

int var_set_active_layout(gint active_idx)
{
    ivar.active_layout = active_idx;
    return 0;
}

int var_get_active_layout()
{
    return ivar.active_layout;
}

int var_get_fake_fwver(char *ver, int len)
{
    SysInfoData sys_info;

    memset(ver, 0x00, len);

    if (ivsc.dfunc.fakefwver.support) {
        strcpy(ver, ivsc.dfunc.fakefwver.ver);
    }
    else {
        memset(&sys_info, 0, sizeof(SysInfoData));
        DAL_get_sysInfo_data(&sys_info);
        strcpy(ver, sys_info.swVer);
    }

    return 0;
}

int var_get_fake_hwver(char *ver, int len)
{
    char hwver[32];

    memset(ver, 0x00, len);

    if (ivsc.dfunc.fakehwver.support) {
        strcpy(ver, ivsc.dfunc.fakehwver.ver);
    }
    else {
        scm_get_hw_ver(hwver, 32);
        strcpy(ver, hwver);
    }

    return 0;
}

int var_set_external_addr(char *addr)
{
    memset(ivar.externalAddr, 0x00, sizeof(ivar.externalAddr));
    strcpy(ivar.externalAddr, addr);
    return 0;
}

int var_get_external_addr(char *addr, int len)
{
    memset(addr, 0x00, len);
    strcpy(addr, ivar.externalAddr);
    return 0;
}

int var_set_enable_remote_upgrade(gint enable)
{
    ivar.enable_remote_fwup = enable;
    return 0;
}

int var_get_enable_remote_upgrade()
{
    return ivar.enable_remote_fwup;
}

int var_set_detect_fwver(char *fwver)
{
    memset(ivar.detect_fwver, 0x00, sizeof(ivar.detect_fwver));
    strcpy(ivar.detect_fwver, fwver);
    return 0;
}

int var_get_detect_fwver(char *fwver, int len)
{
    memset(fwver, 0x00, len);
    strcpy(fwver, ivar.detect_fwver);
    return 0;
}

int var_set_detect_cam_fwver(int ch, char *fwver)
{
    memset(&ivar.detect_cam_fwver[ch], 0x00, sizeof(ivar.detect_cam_fwver));
    strcpy(&ivar.detect_cam_fwver[ch], fwver);
    return 0;
}

int var_get_detect_cam_fwver(int ch, char *fwver, int len)
{
    memset(fwver, 0x00, len);
    strcpy(fwver, &ivar.detect_cam_fwver[ch]);
    return 0;
}

int var_set_camtitle()
{
    int i;

    for (i = 0; i < var_get_ch_count(); i++)
        DAL_get_camera_title(ivar.chTitle[i].titleStr, i);

    return 0;
}

int var_get_camtitle(char *strTitle, guint ch)
{
    BITMASK novideo = var_get_novideo_mask();
    BITMASK covert = ssm_get_covert_mask();

    if((novideo & (1 << ch)) || (covert & (1 << ch)))
    {
        if (ivsc.dswver.vendor == 65)
        {
            sprintf(strTitle, "CH%d", ch+1);
        }
        else
        {
            sprintf(strTitle, "CAM%d", ch+1);
        }
    }
    else
        strcpy(strTitle, ivar.chTitle[ch].titleStr);

    return 0;
}

int var_get_covert_mask(BITMASK *mask)
{
    *mask = _get_covert_mask();
    return 0;
}

int var_get_qr_url(gchar *qr_url, gint len)
{
    gchar ori_url[256] = {0,};
    gchar ddns_addr[256] = {0,};
    gchar mac_string[32] = {0,};
    gchar systemid[65] = {0,};
    gint offset = 0;
    SysInfoData sysinfo;

    strcpy(ori_url, qr_url);
    DAL_get_QR_Code_URL(qr_url);
    DAL_get_sysInfo_data(&sysinfo);

    offset += strlen(qr_url);
    if(offset > len - 1){
        g_message("%s(%d) Do not pass the url is %d characters.", __FUNCTION__, __LINE__, len);
        strcpy(qr_url, ori_url);
        return -1;
    }

    scm_get_mac_addr_str(mac_string, 32);

    offset += ( 4 + 32 );

    if( offset < len ) {
        strcat(qr_url,"?id=");
        strcat(qr_url,sysinfo.macAddr);
        strcat(qr_url,"&alias=");
        strcat(qr_url,sysinfo.sysId);
        strcat(qr_url,"&fwver=");
        strcat(qr_url,sysinfo.swVer);
    }
    else{
        g_message("%s(%d) Do not pass the url is %d characters.", __FUNCTION__, __LINE__, len);
        strcpy(qr_url, ori_url);
        return -1;
    }

    if(DAL_is_ddns_on() == TRUE)
    {
        gchar ddns_params[1024];

        scm_get_dvr_addr_str(ddns_addr, 256);

        snprintf(ddns_params, 1024, "&ddns=%s&p1=%d&p2=%d"
            , ddns_addr, sysinfo.webPort, sysinfo.rtspPort);

        offset += strlen(ddns_params);

        if( offset < len ) {
          strcat(qr_url, ddns_params);
        }
        else{
            g_message("%s(%d) Do not pass the url is %d characters.", __FUNCTION__, __LINE__, len);
            strcpy(qr_url, ori_url);
            return -1;
        }
    }

    return 0;
}

int var_get_supported_audio()
{
    return ivsc.dfunc.support_audio;
}

int var_get_supported_dmva()
{
    if (ivsc.dfunc.support_dmva_itx) return 1;
    if (ivsc.dfunc.support_dmva_s1) return 1;

    return 0;
}

int var_get_supported_ipcam_fwup()
{
    return ivsc.dfunc.support_ipcamfwup;
}

int var_get_supported_keyctrl()
{
    return ivsc.dfunc.support_keyctrl;
}

int var_get_supported_mobilepush()
{
    return ivsc.dfunc.support_mobilepush;
}

int var_get_supported_openmode()
{
    return ivsc.dfunc.support_install_openmode;
}

int var_get_supported_p2p()
{
    return ivsc.dfunc.support_p2p;
}

int var_get_supported_privacy_mask()
{
    return ivsc.dfunc.support_privacymask;
}

int var_get_supported_raid()
{
    return ivsc.dfunc.support_raid;
}

int var_get_supported_secom_dual()
{
    return ivsc.dfunc.support_secomdual;
}

int var_get_supported_sequrinet()
{
    return ivsc.dfunc.sequrinet.support;
}

int var_get_supported_user_guide()
{
    return ivsc.dfunc.userguide.support;
}

int var_get_supported_backdoor_password()
{
    return ivsc.dfunc.backdoorpw.support;
}

int var_get_backdoor_password(char *pw)
{
    strcpy(pw, ivsc.dfunc.backdoorpw.password);
    return 0;
}

int var_get_supported_ddns()
{
    return ivsc.dfunc.ddns.support;
}

int var_get_ddns_cnt()
{
    return ivsc.dfunc.ddns.cnt;
}

int var_get_ddns_cfg(DDNS_CFG_T *cfg, int idx)
{
    if (idx >= ivsc.dfunc.ddns.cnt) return -1;

    strcpy(cfg->server, ivsc.dfunc.ddns.server[idx].name);
    cfg->on_id = ivsc.dfunc.ddns.server[idx].on_id;
    cfg->on_pwd = ivsc.dfunc.ddns.server[idx].on_pw;
    cfg->on_nvr = ivsc.dfunc.ddns.server[idx].on_nvr;
    cfg->on_mac = ivsc.dfunc.ddns.server[idx].on_mac;
    cfg->on_status = ivsc.dfunc.ddns.server[idx].on_status;

    return 0;
}

int var_get_running_encoder_mode()
{
    return DAL_get_encorder_mode();
}

int var_get_supported_encoder_mode()
{
    return ivsc.dfunc.support_encoder_mode;
}

int var_get_supported_noise()
{
    return ivsc.dfunc.support_noise;
}

void var_set_update_ipconflict_list(CONFLICT_INFOR *set_ip)
{
    memcpy(&ip_c, set_ip, sizeof(CONFLICT_INFOR));
}

void var_get_update_ipconflict_list(CONFLICT_INFOR *get_info)
{
    memcpy(get_info, &ip_c, sizeof(CONFLICT_INFOR));
}

void var_set_conflict_infor(int idx)
{
    ip_c.conflict &= !(1<<idx);
}

unsigned int var_get_conflict_infor()
{
    return ip_c.conflict;
}

int var_get_supported_double_login()
{
    return ivsc.dfunc.support_double_login;
}

int var_get_supported_checkpw_search_arch()
{
    return ivsc.dfunc.support_checkpw_search_arch;
}

int var_get_supported_ext_disk()
{
    return ivsc.dfunc.support_ext_disk;
}

int var_get_supported_double_knock()
{
    return ivsc.dfunc.support_double_knock;
}

int var_get_display_int_disk_count()
{
/*
#if defined(_IPX_0412VE3) || defined(_IPX_0824VE3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648L4) || defined(_IPX_0824L4) || defined(_IPX_0412L4)
    return 2;
    
#elif defined(_IPX_0824P3) || defined(_IPX_1648P3)|| defined(_IPX_0824P4) || defined(_IPX_1648P4) \
    || defined(_IPX_0824P3ECO) || defined(_IPX_1648P3ECO) \
    || defined(_IPX_1648P4E) || defined(_IPX_0824P4E)
    return 5;
    
#elif defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824M4E)|| defined(_IPX_1648M4E)
    return 2;

#elif defined(_IPX_0412M4) || defined(_IPX_0412M4E)
    return 1;
    
#else
    return 5;
#endif
*/
    return 5;
}

void  var_set_vct_result_label(gchar *str, gint ch)
{
    memset(vct_label.vct_result[ch], 0x00, sizeof(vct_label.vct_result[ch]));

    strcpy(vct_label.vct_result[ch], str);
}

void var_get_vct_result_label(gchar *str, gint ch)
{
    if(strlen(vct_label.vct_result[ch]))
    {
        strcpy(str, vct_label.vct_result[ch]);
    }
    else
        *str = 0;
}

int var_get_dvr_alarmIn_cnt()
{
	gint cnt = DVR_ALARM_IN;

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)
		cnt = DVR_ALARM_IN;
	else {
	    cnt = 2;    //check_check Test 		
	}
		
#endif
    // g_message("%s, %d, DVR_ALARM_CNT : %d", __FUNCTION__, __LINE__, cnt);

	return cnt;
}


int var_get_alarmIn_cnt()
{
	gint cnt = ALARM_IN_COUNT;

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)  //check_check
	if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)
		cnt = (CAM_ALARM_IN + DVR_ALARM_IN);
	else{
	    cnt = (2 + CAM_ALARM_IN);
	}
#endif
    // g_message("%s, %d, ALARM_IN_CNT : %d", __FUNCTION__, __LINE__, cnt);

	return cnt;
}
