/*
 * vw_desc.c
 *  - dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Mar 19, 2015
 *
 */

#include "nf_sysman.h"

#include "iux_afx.h"

#include "var.h"
#include "vw_menu.h"

#include "vw_desc.h"
#include "vw_wizard_init.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "VW_DESC"


VWDESC_T ivsc;





////////////////////////////////////////////////////////////
//
// private functions
//

static int _parser_swver(DESC_SWVER_T *swver)
{
	gchar tmp[128];
	gint ret;

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, nf_sysdb_get_str_nocopy("sys.info.swver"), sizeof(tmp));
	ret = sscanf(tmp, "%d.%d.%d.%d%s", &swver->product, &swver->procotol, &swver->minor, &swver->vendor, swver->option);

	g_message("[%s %d], [SWVER:%s] [ret:%d]", __FUNCTION__, __LINE__, tmp, ret);

	g_message("[%s %d], [product:%d] [procotol:%d] [minor:%d] [vendor:%d] [option:%s]",
	    __FUNCTION__, __LINE__, swver->product, swver->procotol, swver->minor, swver->vendor, swver->option);

    g_assert(swver->vendor);

    return 0;
}

static gint _get_prop_ddns_udrdns(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "UDRDNS.NET");
    server->on_nvr = 1;
    server->on_mac = 1;

    return 0;
}

static gint _get_prop_ddns_dvrlink(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "DVRLINK.NET");
    server->on_nvr = 1;
    server->on_mac = 1;

    return 0;
}

static gint _get_prop_ddns_dyndns(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "DYNDNS.ORG");
    server->on_id = 1;
    server->on_pw = 1;
    server->on_nvr = 1;
    server->on_mac = 1;

    return 0;
}

static gint _get_prop_ddns_fujiko(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "FUJIKO.BIZ");
    server->on_id = 1;
    server->on_pw = 1;
    server->on_nvr = 1;
    server->on_mac = 1;

    return 0;
}

static gint _get_prop_ddns_s1(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "S1.CO.KR");
    server->on_mac = 1;
    server->on_status = 1;

    return 0;
}

static gint _get_prop_ddns_noip(DDNSSERVER_PROP_T *server)
{
    strcpy(server->name, "NO-IP.COM");

    server->on_id = 1;
    server->on_pw = 1;
    server->on_nvr = 1;
    server->on_mac = 1;

    return 0;
}

static gint _get_model(guint *model)
{
#if defined(_IPX_0412VE3) || defined(_IPX_0824VE3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648L4) || defined(_IPX_0824L4) || defined(_IPX_0412L4)
    *model = IPX_L3_MODEL;

#elif defined(_IPX_0824P3) || defined(_IPX_1648P3)|| defined(_IPX_0824P4) || defined(_IPX_1648P4)
    *model = IPX_P3_MODEL;

#elif defined(_IPX_0824P3ECO) || defined(_IPX_1648P3ECO)
    *model = IPX_P3_ECO_MODEL;

#elif defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1648M4) \
   || defined(_IPX_0824M4E) || defined(_IPX_0412M4E) || defined(_IPX_1648M4E) || defined(_IPX_32M4E)
    *model = IPX_M4_MODEL;

#elif defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
    *model = IPX_P4E_MODEL;

#elif defined(_ANF4G_0412D) || defined(_ANF4G_0824D) || defined(_ANF4G_1648D)
    *model = DVR4G_ANF_MODEL;

#elif defined(_ATM4G_0412D) || defined(_ATM4G_0824D) || defined(_ATM4G_1648D)
    *model = DVR4G_ATM_MODEL;

#elif defined(_UTM4G_0406D) || defined(_UTM4G_0812D) || defined(_UTM4G_1624D)
    *model = DVR4G_UTM_MODEL;
    
#elif defined(_IPX_32P5)
    *model = IPX_P5_MODEL;
#else
    #error "undefined"
#endif

    return 0;
}

static gint _get_supported_func_analog_type(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if ((model == DVR4G_ANF_MODEL) || (model == DVR4G_ATM_MODEL))
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_archdata_playback(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_advanced_chswitch(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_aspect_ratio(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_ASPECT_RATIO)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_audio(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;
    
#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
    if(nf_sysman_get_pba_type() == 0)
    {
        *support = 1;
    }
    else
    {
        *support = 0;
    }
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_bgvlayer(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

#if defined(ENABLE_SYSMAN_USE_AUX)
    if (((model == DVR3G_ANF_MODEL) || (model == DVR3G_ATM_MODEL)) && (var_get_ch_count() == 4))
    {
        if (DAL_get_spot_valid_mode(1) == SPOT_MODE_VALID) *support = 0;
    }
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_camtype(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_ptzsetup(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (RS485_CNT == 0) *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_dmva_itx(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_DMVA)
    *support = 1;
#endif

    if (vendor == 30 || vendor == 28 || vendor == 128 || vendor == 228) *support = 0;
    
    if (GUI_CHANNEL_CNT == 32)
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_dmva_s1(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_DMVA)
    if (vendor == 30) *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_dlva_itx(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_DLVA)
    *support = 1;
#endif

    if (vendor == 28 || vendor == 128 || vendor == 228) {
        if (nf_sysman_get_pba_type() != NF_SYSMAN_PBA_TYPE_A)
        {
            *support = 0;
        }
    }
    
    if (GUI_CHANNEL_CNT == 32)
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_aicam_itx(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_AICAM)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_aibox_itx(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_AIBOX)
    *support = 1;
#endif

    if (vendor == 28 || vendor == 128 || vendor == 228) {
        if ((model == IPX_M4_MODEL) && (nf_sysman_get_pba_type() != NF_SYSMAN_PBA_TYPE_A))
        {
            *support = 0;
        }
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_dl_timeline(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_DLVA) || defined(_SUPPORT_AICAM) || defined(_SUPPORT_AIBOX)
    *support = 1;
#endif
    
    if (GUI_CHANNEL_CNT == 32)
    {
        *support = 0;
    }
    
    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_hdspot(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_HD_SPOT)
    *support = 1;
#endif

    if (SPOTHD_PORT_CNT == 0) *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_hdsdi_outmode(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_HDSDI_OUTMODE)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_ipcamfwup(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (IS_SYSTEMCATE(model, NVR_CATEGORY))
    {
#if defined(_SUPPORT_IPCAM_FWUP)
        *support = 1;
#endif

        if (vendor == 108) {
            *support = 0;
        }
    }

    if (DAL_get_cam_install_mode() == 1)
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_ipcamlist(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_loopout(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_mobilepush(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;
    
    DAL_get_Sequrinet_Enable(support);

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_monitorout(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_MONITOR_SEL)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_network_security(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_keyctrl(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (vendor == 65) {
        *support = 0;
    }

    if (RS485_CNT == 0) *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_install_cctvmode(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (IS_MODELCATE(model, IPX_ECO_CATEGORY) || IS_MODELCATE(model, IPX3_ECO_CATEGORY))
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_install_openmode(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_onvifsetup(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (IS_MODELCATE(model, IPX_CATEGORY) || IS_MODELCATE(model, IPX_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX3_CATEGORY) || IS_MODELCATE(model, IPX3_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX4E_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX5_CATEGORY))
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_p2p(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_posevent(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (vendor == 65) {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_privacymask(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (IS_MODELCATE(model, IPX_CATEGORY) || IS_MODELCATE(model, IPX_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX3_CATEGORY) || IS_MODELCATE(model, IPX3_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX4E_CATEGORY))
    {
        *support = 1;
    }

    if (vendor == 108) {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_fisheye(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (vendor == 28 || vendor == 128) {
        *support = 0;
    }

    if ((model == IPX_M4_MODEL) && (var_get_ch_count() != 16))
    {
        *support = 0;
    }    

    if ((var_get_ch_count() == 32))
    {
        *support = 0;
    }    

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_protect(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_raid(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if ((model == IPX_P3_MODEL) || (model == IPX_P3_ECO_MODEL) || (model == IPX_P4E_MODEL))
    {
        *support = 1;
    }

    if (model == DVR4G_ANF_MODEL)
    {
        if ((var_get_ch_count() == 8) || (var_get_ch_count() == 16)) {
            *support = 1;
        }
    }

    if (model == IPX_P5_MODEL)
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_rec_audiomap(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (IS_MODELCATE(model, DVR4G_CATEGORY))
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}


static gint _get_supported_func_rec_calc(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (IS_MODELCATE(model, IPX_CATEGORY) || IS_MODELCATE(model, IPX_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX3_CATEGORY) || IS_MODELCATE(model, IPX3_ECO_CATEGORY))
    {
        *support = 1;
    }

    if (IS_MODELCATE(model, IPX4E_CATEGORY))
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_rec_validity(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (model == DVR4G_UTM_MODEL)
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_secomdual(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_smsevent(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_snmp(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_spot(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (model == DVR4G_ANF_MODEL)
    {
        *support = 1;
    }

    if (model == DVR4G_ATM_MODEL)
    {
        *support = 1;
    }

    if (model == IPX_M4_MODEL || model == IPX_P4E_MODEL || model == IPX_P5_MODEL)
    {
        *support = 1;
    }

    if ((SPOTSD_PORT_CNT == 0) && (SPOTAUX_PORT_CNT == 0)) *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_standby(guint model, gint vendor, gint *support)
{
    unsigned int is_power = 0;

    if (!support) return -1;

    *support = 0;

//  nf_dev_keypad_get_is_keypower(&is_power);
//  g_message("%s, %d, power:%d", __FUNCTION__, __LINE__, is_power);

    if (is_power == 1) *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_swmode(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 104) {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_al_switch(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 28 || vendor == 32 || vendor == 128)
    {
        *support = 1;
    }

#if !defined (ENABLE_ARI_PANIC) && (DVR_ALARM_IN < 1)
    *support = 0;
#endif
    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_double_login(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 32)
    {
        *support = 1;
    }
    
    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_checkpw_search_arch(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 28 || vendor == 65 || vendor == 128)
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_ext_disk(guint model, gint vendor, gint *support)
{
    if (!support) return -1;
    
    *support = 1;

#if defined(_NO_EXT_DISK)
    *support = 0;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_swsignal(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_SW_SIGNAL)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_system_security(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_tamper(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

#if defined(_SUPPORT_TAMPER_SETUP)
    *support = 1;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_cable_test(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    if (IS_MODELCATE(model, IPX3_ECO_CATEGORY) || IS_MODELCATE(model, IPX_ECO_CATEGORY) \
        || IS_MODELCATE(model, IPX5_CATEGORY))
    {
        *support = 0;
    }

	if (model == IPX_M4_MODEL || model == IPX_P4E_MODEL)
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_remocon_id(guint model, gint vendor, gint *support)
{
    gchar *rc_type = 0;

    if (!support) return -1;

    *support = 1;

    rc_type = nf_api_param_hw_get_rc_type();
    if (rc_type) {
        if (!strcmp(rc_type, "ICA_ITX")) *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_find_passwd(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

#if defined(_SUPPORT_FORGOT_PASSWORD)
    *support = 1;
#else    
    *support = 0;
#endif

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_license(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;
    
    if (GUI_CHANNEL_CNT == 32)
    {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_video_erase(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_usrdef_holiday(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 32)
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_dlva_counter(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (vendor == 28 || vendor == 128)
    {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_double_knock(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_userguide(guint model, gint vendor, FUNC_USERGUIDE_T *userguide)
{
    if (!userguide) return -1;

    userguide->support = 0;

    DMSG(1, "support : %d", userguide->support);

    return 0;
}

static gint _get_supported_func_auth_setup_hide(guint model, gint vendor, gint *support)
{
	if (!support) return -1;

	*support = 0;
	
	if(vendor == 100 || vendor == 32 || vendor == 43) {
		*support = 1;
	}

	DMSG(1, "support : %d", *support);
 
	return 0;
}

static gint _get_supported_func_license_plate(guint model, gint vendor, gint *support)
{
    if(!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_face(guint model, gint vendor, gint *support)
{
    if(!support) return -1;

    *support = 0;

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_func_qna(guint model, gint vendor, gint *support)
{
    if(!support) return -1;

    *support = 0;

    if(vendor == 28) {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);
 
    return 0;
}

static gint _get_supported_func_autologout_on_reboot(guint model, gint vendor, gint *support)
{
    if(!support) return -1;

    *support = 1;

    if(vendor == 78 || vendor == 28) {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);
 
    return 0;
}

static gint _get_supported_func_forgot_pw_warning_box(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 1;
    
    if(vendor == 28) {
        *support = 0;
    }

    DMSG(1, "support : %d", *support);
 
    return 0;
}

static gint _get_supported_func_corridor(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;
    
    if(vendor == 28) {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);
 
    return 0;
}

static gint _get_supported_func_ddns(guint model, gint vendor, FUNC_DDNS_T *ddns)
{
    gint i;

    if (!ddns) return -1;

    ddns->support = 1;
    
	if (vendor == 43) {
		_get_prop_ddns_udrdns(&ddns->server[ddns->cnt++]);
	}
	
	if (vendor == 100) {
        _get_prop_ddns_noip(&ddns->server[ddns->cnt++]);
    }
	
	_get_prop_ddns_dvrlink(&ddns->server[ddns->cnt++]);
	_get_prop_ddns_dyndns(&ddns->server[ddns->cnt++]);

    if (vendor == 46) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));
        ddns->support = 0;
    }

    if (vendor == 95) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));

        ddns->support = 1;
        _get_prop_ddns_dvrlink(&ddns->server[ddns->cnt++]);
    }

    if (vendor == 106) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));

        ddns->support = 1;
        _get_prop_ddns_dvrlink(&ddns->server[ddns->cnt++]);
        _get_prop_ddns_fujiko(&ddns->server[ddns->cnt++]);
    }

    if (vendor == 107) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));

        ddns->support = 1;
        _get_prop_ddns_dvrlink(&ddns->server[ddns->cnt++]);
    }

    if (vendor == 108) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));

        ddns->support = 1;
        _get_prop_ddns_dvrlink(&ddns->server[ddns->cnt++]);
    }

    DMSG(1, "support : %d", ddns->support);

    if (!ddns->support) {
        memset(ddns, 0x00, sizeof(FUNC_DDNS_T));
    }

    return 0;
}

static gint _get_supported_func_backdoorpw(guint model, gint vendor, FUNC_BACKDOORPW_T *backdoorpw)
{
    if (!backdoorpw) return -1;

    backdoorpw->support = 0;

    if (vendor == 65) {
        backdoorpw->support = 1;
        strcpy(backdoorpw->password, "t3jp36dv");
    }

    DMSG(1, "support : %d", backdoorpw->support);

    if (!backdoorpw->support) {
        memset(backdoorpw, 0x00, sizeof(FUNC_BACKDOORPW_T));
    }

    return 0;
}

static gint _get_supported_func_buzzer(guint model, gint vendor, FUNC_BUZZER_T *buzzer)
{
    if (!buzzer) return -1;

    buzzer->support_keypad = 0;
    buzzer->support_remocon = 1;

//  nf_dev_keypad_get_pt_is_key(&buzzer->support_keypad);

    if (model == DVR4G_UTM_MODEL)
    {
        buzzer->support_remocon = 0;
    }

    DMSG(1, "support_keypad : %d", buzzer->support_keypad);
    DMSG(1, "support_remocon : %d", buzzer->support_remocon);

    return 0;
}

static gint _get_supported_func_check_event(guint model, gint vendor, FUNC_CHKEVT_T *chkevt)
{
    if (!chkevt) return -1;

    chkevt->support_fan= 1;
    chkevt->support_temperature = 1;
    chkevt->support_poe = 1;

    if (IS_MODELCATE(model, DVR4G_CATEGORY))
    {
        chkevt->support_poe = 0;
    }

    if ((model == DVR4G_UTM_MODEL) || (model == IPX_M4_MODEL))
    {
        chkevt->support_fan = 0;
        chkevt->support_temperature = 0;
    }

    DMSG(1, "support_fan : %d", chkevt->support_fan);
    DMSG(1, "support_temperature : %d", chkevt->support_temperature);
    DMSG(1, "support_poe : %d", chkevt->support_poe);

    return 0;
}

static gint _get_supported_func_dualmonitor(guint model, gint vendor, FUNC_DUALMONITOR_T *dualmonitor)
{
    if (!dualmonitor) return -1;

    dualmonitor->support = 0;

    if ((model == DVR4G_ANF_MODEL) || (model == DVR4G_ATM_MODEL) ||
        (model == IPX_P5_MODEL))
        
    {
        dualmonitor->support = 1;
        dualmonitor->advance_type = 1;
    }

    DMSG(1, "support : %d", dualmonitor->support);

    if (!dualmonitor->support) {
        memset(dualmonitor, 0x00, sizeof(FUNC_DUALMONITOR_T));
    }

    return 0;
}

static gint _get_supported_func_fakefwver(guint model, gint vendor, FUNC_FAKEVER_T *fakever)
{
    if (!fakever) return -1;

    fakever->support = 0;

    if (vendor == 30) {
        fakever->support = 1;
        strcpy(fakever->ver, "V1.1.0");
    }

    DMSG(1, "support : %d", fakever->support);

    if (!fakever->support) {
        memset(fakever, 0x00, sizeof(FUNC_FAKEVER_T));
    }

    return 0;
}

static gint _get_supported_func_fakehwver(guint model, gint vendor, FUNC_FAKEVER_T *fakever)
{
    if (!fakever) return -1;

    fakever->support = 0;

    if (vendor == 30) {
        fakever->support = 1;
        strcpy(fakever->ver, "V1.0");
    }

    DMSG(1, "support : %d", fakever->support);

    if (!fakever->support) {
        memset(fakever, 0x00, sizeof(FUNC_FAKEVER_T));
    }

    return 0;
}

static gint _get_supported_func_sequrinet(guint model, gint vendor, FUNC_SEQURINET_T *sequrinet)
{
    if (!sequrinet) return -1;

    DAL_get_Sequrinet_Enable(&sequrinet->support);

    sequrinet->support_easyip = 1;
    sequrinet->support_p2p = 1;
    sequrinet->support_mobilepush = 1;

    DMSG(1, "support : %d", sequrinet->support);

    if (!sequrinet->support) {
        memset(sequrinet, 0x00, sizeof(FUNC_SEQURINET_T));
    }

    return 0;
}

static gint _get_supported_func_wizard(guint model, gint vendor, FUNC_WIZARD_T *wizard)
{
    gint i;
    gint page_idx = 0, numbering_idx = 0;
    gint enable;

    if (!wizard) return -1;

    if(scm_is_qc_mode() == 0) {
        wizard->support = 0;
    }
    else {
        wizard->support = 1;
    }

    if (vendor == 65) {
        wizard->support = 0;
    }

    wizard->language.support = 1;
    wizard->welcome.support = 1;
    wizard->sigtype.support = 1;
    wizard->password.support = 0;
    wizard->datetime.support = 1;
    wizard->record.support = 1;
    wizard->network.support = 1;

    wizard->language.setup[page_idx].use_numbering = 0;
    wizard->language.setup[page_idx++].func = vw_wizard_language_open;

    wizard->sigtype.setup[page_idx].use_numbering = 0;
    wizard->sigtype.setup[page_idx++].func = vw_wizard_sigtype_open;

    wizard->welcome.setup[page_idx].use_numbering = 0;
    wizard->welcome.setup[page_idx++].func = vw_wizard_welcome_open;

//    wizard->password.setup[page_idx].use_numbering = 1;
//    wizard->password.setup[page_idx++].func = vw_wizard_password_confirm_open;

//    wizard->password.setup[page_idx].use_numbering = 1;
//    wizard->password.setup[page_idx++].func = vw_wizard_password_change_open;

    wizard->datetime.setup[page_idx].use_numbering = 1;
    wizard->datetime.setup[page_idx++].func = vw_wizard_datetime_change_open;

    wizard->record.setup[page_idx].use_numbering = 0;
    wizard->record.setup[page_idx++].func = vw_wizard_record_instruction_open;

    wizard->record.setup[page_idx].use_numbering = 1;
    wizard->record.setup[page_idx++].func = vw_wizard_record_modeselect_open;

    //wizard->record.setup[page_idx].use_numbering = 1;
    //wizard->record.setup[page_idx++].func = vw_wizard_record_imageconf_open;

    wizard->network.setup[page_idx].use_numbering = 0;
    wizard->network.setup[page_idx++].func = vw_wizard_network_instruction_open;

    wizard->network.setup[page_idx].use_numbering = 1;
    wizard->network.setup[page_idx++].func = vw_wizard_network_ipsetup_open;

    wizard->network.setup[page_idx].use_numbering = 1;
    wizard->network.setup[page_idx++].func = vw_wizard_network_upnpsetup_open;

    wizard->network.setup[page_idx].use_numbering = 1;
    wizard->network.setup[page_idx++].func = vw_wizard_network_ddnssetup_open;

    DAL_get_Sequrinet_Enable(&enable);
    if(enable){
        wizard->network.setup[page_idx].use_numbering = 1;
        wizard->network.setup[page_idx++].func = vw_wizard_network_easyipsetup_open;
    }

    wizard->network.setup[page_idx].use_numbering = 0;
    wizard->network.setup[page_idx++].func = vw_wizard_network_result_open;

    DMSG(1, "support : %d", wizard->support);

    if (!wizard->support) {
        memset(wizard, 0x00, sizeof(FUNC_WIZARD_T));
    }

    return 0;
}

static gint _get_supported_func_ai(guint model, gint vendor, FUNC_AI_T *ai)
{
    if (!ai) return -1;

    ai->support = 1;
    ai->support_calibration = 0;

    DMSG(1, "support : %d", ai->support);
    DMSG(1, "support_calibration : %d", ai->support_calibration);

    if (!ai->support) {
        memset(ai, 0x00, sizeof(FUNC_AI_T));
    }

    return 0;
}

static gint _get_supported_func_pin(guint model, gint vendor, FUNC_PIN_T *pin)
{
    if (!pin) return -1;

    pin->support = 0;
    pin->digit = 0;
    pin->id_input_method = 0;
    
    if (vendor == 28 || vendor == 128)
    {
        pin->support = 1;
        pin->digit = 6;
        pin->id_input_method = 1;
    }

    DMSG(1, "support : %d", pin->support);
    DMSG(1, "support_digit : %d", pin->digit);
    DMSG(1, "support_id_input_method : %d", pin->id_input_method);

    return 0;
}

static gint _get_supported_func_dbg_obj(guint model, gint vendor, gint *support)
{
    if (!support) return -1;

    *support = 0;

    if (nf_sysman_hotkey_is_nfs()) {
        *support = 1;
    }

    DMSG(1, "support : %d", *support);

    return 0;
}

static gint _get_supported_menu_systemsetup_camera(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB1_CNT) break;

        if (i == SYS_SUB1_IMAGE_SETUP) {
            if (ivsc.dfunc.support_onvifsetup == 1) continue;
        }

        if (i == SYS_SUB1_ONVIF_IMAGE_SETUP) {
            if (ivsc.dfunc.support_onvifsetup == 0) continue;
        }

        if (i == SYS_SUB1_MOTION_SENSOR) {
            if (ivsc.dfunc.support_onvifsetup == 1) continue;
        }

        if (i == SYS_SUB1_ONVIF_MOTION_SENSOR) {
            if (ivsc.dfunc.support_onvifsetup == 0) continue;
        }

        if (i == SYS_SUB1_CAMERA_TYPE) {
            if (ivsc.dfunc.support_camtype == 0) continue;
        }

        if (i == SYS_SUB1_PTZ_SETUP) {
            if (ivsc.dfunc.support_ptzsetup == 0) continue;
        }

        if (i == SYS_SUB1_PRIVACY_MASK) {
            if (ivsc.dfunc.support_privacymask == 0) continue;
        }

        if (i == SYS_SUB1_FISHEYE_SETUP) {
            if (ivsc.dfunc.support_fisheye == 0) continue;
        }

        if (i == SYS_SUB1_TAMPER_SETUP) {
            if (ivsc.dfunc.support_tamper == 0) continue;
        }

        if (i == SYS_SUB1_ANALOG_TYPE) {
            if (ivsc.dfunc.support_analog_type == 0) continue;
        }

        if (i == SYS_SUB1_INSTALL_MODE) {
            if ((ivsc.dfunc.support_install_cctvmode == 0) || (ivsc.dfunc.support_install_openmode == 0)) continue;
        }

        if (i == SYS_SUB1_VCA_SETUP_S1) {
            if (ivsc.dfunc.support_dmva_s1 == 0) continue;
        }

        if (i == SYS_SUB1_ANALYSIS_SETUP_ITX) 
        {
            if ((ivsc.dfunc.support_dmva_itx == 0) && (ivsc.dfunc.support_dlva_itx == 0) &&
                (ivsc.dfunc.support_aicam_itx == 0) && (ivsc.dfunc.support_aibox_itx == 0))
            {
                continue;
            }
        }

        if (i == SYS_SUB1_IPCAMERA_INSTALL) {
            if ((ivsc.dfunc.support_install_openmode == 0) || (DAL_get_cam_install_mode() == 0)) continue;
        }

        if (i == SYS_SUB1_SPECIAL_CAM_SETUP) {
            if (ivsc.vendor_code != 28 && ivsc.vendor_code != 128) 
            {
                continue;
            }
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_display(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB2_CNT) break;

        if (i == SYS_SUB2_SPOTOUT) {
            if (ivsc.dfunc.support_spot == 0) continue;
        }

        if (i == SYS_SUB2_HD_SPOTOUT) {
            if (ivsc.dfunc.support_hdspot == 0) continue;
        }

        if (i == SYS_SUB2_LOOP) {
            if (ivsc.dfunc.support_loopout == 0) continue;
        }

        if (i == SYS_SUB2_ADVANCED_DUAL) {
            if (ivsc.dfunc.dualmonitor.advance_type == 0) continue;
        }

        if (i == SYS_SUB2_POSATM) {
            if (ivsc.dfunc.support_posevent == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_sound(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB3_CNT) break;

        if (i == SYS_SUB3_BUZZER) {
            if ((!ivsc.dfunc.buzzer.support_remocon) && (!ivsc.dfunc.buzzer.support_keypad)) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_user(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB4_CNT) break;

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_network(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB5_CNT) break;

        if (i == SYS_SUB5_DDNS) {
            if (ivsc.dfunc.ddns.support == 0) continue;
        }

        if (i == SYS_SUB5_SECURITY) {
            if (ivsc.dfunc.support_network_security == 0) continue;
        }

        if (i == SYS_SUB5_SNMP) {
            if (ivsc.dfunc.support_snmp == 0) continue;
        }

        if (i == SYS_SUB5_CABLE_TEST)
        {
            if (ivsc.dfunc.support_cable_test == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_system(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB6_CNT) break;

        if (i == SYS_SUB6_SUPPORT_IPCAM) {
            if (ivsc.dfunc.support_ipcamlist == 0) continue;
        }

        if (i == SYS_SUB6_SECURITY) {
            if (ivsc.dfunc.support_system_security == 0) continue;
        }

        if (i == SYS_SUB6_POSATM) {
            if (ivsc.dfunc.support_posevent == 0) continue;
        }

       if (i == SYS_SUB6_LICENSE) {
           if (ivsc.dfunc.support_license == 0) continue;
       }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_storage(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB7_CNT) break;

        if (i == SYS_SUB7_CONFIGURATION) {
            if (ivsc.dfunc.support_raid == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_storage_del(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB7_OPERATION_CNT) break;

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_event(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_SUB8_CNT) break;

        if (i == SYS_SUB8_ALARMOUT) {
            if (ALARM_OUT_COUNT == 0) continue;
        }

        if (i == SYS_SUB8_ALARMSENSOR) {
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
            if (var_get_alarmIn_cnt() == 0) continue;
		#else
			if (ALARM_IN_COUNT == 0) continue;
		#endif
        }

        if (i == SYS_SUB8_ANALYSIS_EVENT_ITX) 
        {
            if ((ivsc.dfunc.support_dmva_itx == 0) && (ivsc.dfunc.support_dlva_itx == 0) &&
                (ivsc.dfunc.support_aicam_itx == 0) && (ivsc.dfunc.support_aibox_itx == 0))
            {
                continue;
            }
        }

        if (i == SYS_SUB8_TAMPER) {
            if (ivsc.dfunc.support_tamper == 0) continue;
        }

        if (i == SYS_SUB8_VCA_EVENT_S1) {
            if (ivsc.dfunc.support_dmva_s1 == 0) continue;
        }

        if (i == SYS_SUB8_POSATM_EVENT) {
            if (ivsc.dfunc.support_posevent == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_event_noti(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_EVENTNOTI_CNT) break;

        if (i == SYS_EVENTNOTI_SMS) {
            if (ivsc.dfunc.support_smsevent == 0) continue;
        }

        if (i == SYS_EVENTNOTI_MOBILEPUSH) {
            if (ivsc.dfunc.sequrinet.support_mobilepush == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_systemsetup_alarm_sensor_evt(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SYS_ALARM_SENSOR_CNT) break;

        if (i == SYS_ALARM_SENSOR_CAM) {
            if (CAM_ALARM_IN == 0) continue;
        }

        if (i == SYS_ALARM_SENSOR_LOCAL) {
    		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
                if (var_get_dvr_alarmIn_cnt() == 0) continue;
    		#else
    			if (DVR_ALARM_IN == 0) continue;
    		#endif
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_recsetup(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= REC_SUB_CNT) break;

        if (i == REC_SUB_ALARM) {
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
            if (var_get_alarmIn_cnt() == 0) continue;
		#else
			if (ALARM_IN_COUNT == 0) continue;
		#endif
        }

        if (i == REC_SUB_CALCUL) {
            if (ivsc.dfunc.support_rec_calc == 0) continue;
        }

        if (i == REC_SUB_AUDIOMAP) {
            if (ivsc.dfunc.support_rec_audiomap == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_archiving(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= ARCH_SUB_CNT) break;

        if (i == ARCH_SUB_DATA_PB) {
            if (ivsc.dfunc.support_archdata_playback == 0) continue;
        }

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_search(guint model, guint *menu)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        if (i >= SEARCH_SUB_CNT) break;

        if (i == SEARCH_SUB_THUMBNAIL) {
            if (ivsc.dfunc.support_bgvlayer == 0) continue;
        }

        if (i == SEARCH_SUB_VA_STATISTIC) {
            continue;
        }

        if (i == SEARCH_SUB_SMART) {
            continue;
        }

        if (i == SEARCH_SUB_SMART_REV) {
            continue;
        }

        if (i == SEARCH_SUB_TEXT) {
            if (ivsc.dfunc.support_posevent == 0) continue;
        }

        if (i == SEARCH_SUB_DEEPLEARNING) 
        {
            if ((ivsc.dfunc.support_dlva_itx == 0) && (ivsc.dfunc.support_aicam_itx == 0) && (ivsc.dfunc.support_aibox_itx == 0)) {
                continue;
            }
        }        

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _get_supported_menu_userguide(guint model, guint *menu)
{
    gint i;

    if (ivsc.dfunc.userguide.support == 0) return 0;

    for (i = 0; i < 32; i++)
    {
        if (i >= USERGUIDE_SUB_CNT) break;
        if (i >= ivsc.dfunc.userguide.cnt) break;

        *menu |= (1 << i);
    }

    DMSG(1, "menu : %08X", *menu);

    return 0;
}

static gint _vw_desc_init(guint model, gint vendor)
{
    _get_supported_func_analog_type(model, vendor, &ivsc.dfunc.support_analog_type);
    _get_supported_func_archdata_playback(model, vendor, &ivsc.dfunc.support_archdata_playback);
    _get_supported_func_advanced_chswitch(model, vendor, &ivsc.dfunc.support_advanced_chswitch);
    _get_supported_func_aspect_ratio(model, vendor, &ivsc.dfunc.support_aspect_ratio);
    _get_supported_func_audio(model, vendor, &ivsc.dfunc.support_audio);
    _get_supported_func_bgvlayer(model, vendor, &ivsc.dfunc.support_bgvlayer);
    _get_supported_func_camtype(model, vendor, &ivsc.dfunc.support_camtype);
    _get_supported_func_ptzsetup(model, vendor, &ivsc.dfunc.support_ptzsetup);
    _get_supported_func_dmva_itx(model, vendor, &ivsc.dfunc.support_dmva_itx);
    _get_supported_func_dmva_s1(model, vendor, &ivsc.dfunc.support_dmva_s1);
    _get_supported_func_dlva_itx(model, vendor, &ivsc.dfunc.support_dlva_itx);
    _get_supported_func_aicam_itx(model, vendor, &ivsc.dfunc.support_aicam_itx);
    _get_supported_func_aibox_itx(model, vendor, &ivsc.dfunc.support_aibox_itx);
    _get_supported_func_dl_timeline(model, vendor, &ivsc.dfunc.support_dl_timeline);
    _get_supported_func_hdspot(model, vendor, &ivsc.dfunc.support_hdspot);
    _get_supported_func_hdsdi_outmode(model, vendor, &ivsc.dfunc.support_hdsdi_outmode);
    _get_supported_func_install_cctvmode(model, vendor, &ivsc.dfunc.support_install_cctvmode);
    _get_supported_func_install_openmode(model, vendor, &ivsc.dfunc.support_install_openmode);
    _get_supported_func_ipcamfwup(model, vendor, &ivsc.dfunc.support_ipcamfwup);
    _get_supported_func_ipcamlist(model, vendor, &ivsc.dfunc.support_ipcamlist);
    _get_supported_func_keyctrl(model, vendor, &ivsc.dfunc.support_keyctrl);
    _get_supported_func_loopout(model, vendor, &ivsc.dfunc.support_loopout);
    _get_supported_func_mobilepush(model, vendor, &ivsc.dfunc.support_mobilepush);
    _get_supported_func_monitorout(model, vendor, &ivsc.dfunc.support_monitorout);
    _get_supported_func_network_security(model, vendor, &ivsc.dfunc.support_network_security);
    _get_supported_func_onvifsetup(model, vendor, &ivsc.dfunc.support_onvifsetup);
    _get_supported_func_p2p(model, vendor, &ivsc.dfunc.support_p2p);
    _get_supported_func_posevent(model, vendor, &ivsc.dfunc.support_posevent);
    _get_supported_func_privacymask(model, vendor, &ivsc.dfunc.support_privacymask);
    _get_supported_func_fisheye(model, vendor, &ivsc.dfunc.support_fisheye);
    _get_supported_func_protect(model, vendor, &ivsc.dfunc.support_protect);
    _get_supported_func_raid(model, vendor, &ivsc.dfunc.support_raid);
    _get_supported_func_rec_audiomap(model, vendor, &ivsc.dfunc.support_rec_audiomap);
    _get_supported_func_rec_calc(model, vendor, &ivsc.dfunc.support_rec_calc);
    _get_supported_func_rec_validity(model, vendor, &ivsc.dfunc.support_rec_validity);
    _get_supported_func_secomdual(model, vendor, &ivsc.dfunc.support_secomdual);
    _get_supported_func_smsevent(model, vendor, &ivsc.dfunc.support_smsevent);
    _get_supported_func_snmp(model, vendor, &ivsc.dfunc.support_snmp);
    _get_supported_func_spot(model, vendor, &ivsc.dfunc.support_spot);
    _get_supported_func_standby(model, vendor, &ivsc.dfunc.support_standby);
    _get_supported_func_system_security(model, vendor, &ivsc.dfunc.support_system_security);
    _get_supported_func_swmode(model, vendor, &ivsc.dfunc.support_swmode);
    _get_supported_func_al_switch(model, vendor, &ivsc.dfunc.support_al_switch);
    _get_supported_func_double_login(model, vendor, &ivsc.dfunc.support_double_login);
    _get_supported_func_checkpw_search_arch(model, vendor, &ivsc.dfunc.support_checkpw_search_arch);
    _get_supported_func_ext_disk(model, vendor, &ivsc.dfunc.support_ext_disk);
    _get_supported_func_swsignal(model, vendor, &ivsc.dfunc.support_swsignal);
    _get_supported_func_tamper(model, vendor, &ivsc.dfunc.support_tamper);
    _get_supported_func_cable_test(model, vendor, &ivsc.dfunc.support_cable_test);
	_get_supported_func_remocon_id(model, vendor, &ivsc.dfunc.support_remocon_id);
    _get_supported_func_find_passwd(model, vendor, &ivsc.dfunc.support_find_passwd);
    _get_supported_func_license(model, vendor, &ivsc.dfunc.support_license);
	_get_supported_func_video_erase(model, vendor, &ivsc.dfunc.support_video_erase);
    _get_supported_func_usrdef_holiday(model, vendor, &ivsc.dfunc.support_usrdef_holiday);
    _get_supported_func_dlva_counter(model, vendor, &ivsc.dfunc.support_dlva_counter);
    _get_supported_func_double_knock(model, vendor, &ivsc.dfunc.support_double_knock);
	_get_supported_func_auth_setup_hide(model, vendor, &ivsc.dfunc.support_auth_popup_hide);
    _get_supported_func_license_plate(model, vendor, &ivsc.dfunc.support_license_plate);
    _get_supported_func_face(model, vendor, &ivsc.dfunc.support_face);
    _get_supported_func_qna(model, vendor, &ivsc.dfunc.support_qna);
    _get_supported_func_autologout_on_reboot(model, vendor, &ivsc.dfunc.support_autologout_on_reboot);
    _get_supported_func_forgot_pw_warning_box(model, vendor, &ivsc.dfunc.support_forgot_pw_warning_box);
    _get_supported_func_corridor(model, vendor, &ivsc.dfunc.support_corridor);
    
    _get_supported_func_backdoorpw(model, vendor, &ivsc.dfunc.backdoorpw);
    _get_supported_func_buzzer(model, vendor, &ivsc.dfunc.buzzer);
    _get_supported_func_check_event(model, vendor, &ivsc.dfunc.chkevt);
    _get_supported_func_ddns(model, vendor, &ivsc.dfunc.ddns);
    _get_supported_func_dualmonitor(model, vendor, &ivsc.dfunc.dualmonitor);
    _get_supported_func_fakefwver(model, vendor, &ivsc.dfunc.fakefwver);
    _get_supported_func_fakehwver(model, vendor, &ivsc.dfunc.fakehwver);
    _get_supported_func_sequrinet(model, vendor, &ivsc.dfunc.sequrinet);
    _get_supported_func_userguide(model, vendor, &ivsc.dfunc.userguide);
    _get_supported_func_wizard(model, vendor, &ivsc.dfunc.wizard);
    _get_supported_func_ai(model, vendor, &ivsc.dfunc.ai);
    _get_supported_func_pin(model, vendor, &ivsc.dfunc.pin);

    _get_supported_func_dbg_obj(model, vendor, &ivsc.dfunc.support_dbg_obj);

    _get_supported_menu_systemsetup_camera(model, &ivsc.dmenu.submenu_camera);
    _get_supported_menu_systemsetup_display(model, &ivsc.dmenu.submenu_disp);
    _get_supported_menu_systemsetup_sound(model, &ivsc.dmenu.submenu_sound);
    _get_supported_menu_systemsetup_user(model, &ivsc.dmenu.submenu_user);
    _get_supported_menu_systemsetup_network(model, &ivsc.dmenu.submenu_network);
    _get_supported_menu_systemsetup_system(model, &ivsc.dmenu.submenu_system);
    _get_supported_menu_systemsetup_storage(model, &ivsc.dmenu.submenu_storage);
    _get_supported_menu_systemsetup_storage_del(model, &ivsc.dmenu.submenu_storage_del);
    _get_supported_menu_systemsetup_event(model, &ivsc.dmenu.submenu_event);
    _get_supported_menu_systemsetup_event_noti(model, &ivsc.dmenu.submenu_event_noti);
    _get_supported_menu_systemsetup_alarm_sensor_evt(model, &ivsc.dmenu.submenu_alarm_sensor_evt);
    _get_supported_menu_recsetup(model, &ivsc.dmenu.recmenu);
    _get_supported_menu_archiving(model, &ivsc.dmenu.archmenu);
    _get_supported_menu_search(model, &ivsc.dmenu.searchmenu);
    _get_supported_menu_userguide(model, &ivsc.dmenu.userguide);

    return 0;
}

static gint _vw_desc_dbg(guint model, gint vendor)
{
	FILE *fp = NULL;
	gchar fname[128];
	gint i;

    memset(fname, 0x00, sizeof(fname));
    sprintf(fname, "/NFDVR/data/gui/uxconf.%d.tmp", vendor);

	if ((fp = fopen(fname, "w")) == NULL) return -1;

    fprintf(fp, "<itx_base>\n");
    fprintf(fp, "\n");
    fprintf(fp, "<!-- ====================================================================== -->\n");
    fprintf(fp, "<!-- FUNCTIONALITY CONFIGURATION -->\n");
    fprintf(fp, "<!-- ====================================================================== -->\n");
    fprintf(fp, "\n");
    fprintf(fp, "<functionality>\n");
    fprintf(fp, "<ddns>%d</ddns>\n", ivsc.dfunc.ddns.support);
    fprintf(fp, "<user_guide>%d</user_guide>\n", ivsc.dfunc.userguide.support);
    fprintf(fp, "<audio>%d</audio>\n", ivsc.dfunc.support_audio);
    fprintf(fp, "<p2p>%d</p2p>\n", ivsc.dfunc.support_p2p);
    fprintf(fp, "<secom_dual>%d</secom_dual>\n", ivsc.dfunc.support_secomdual);
    fprintf(fp, "<keyctrl>%d</keyctrl>\n", ivsc.dfunc.support_keyctrl);
    fprintf(fp, "</functionality>\n");
    fprintf(fp, "\n");

    for (i = 0; i < ivsc.dfunc.ddns.cnt; i++)
    {
        fprintf(fp, "<ddns%d>\n", i+1);
        fprintf(fp, "<use>1</use>\n");
        fprintf(fp, "<server>%s</server>\n", ivsc.dfunc.ddns.server[i].name);
        fprintf(fp, "<on_id>%d</on_id>\n", ivsc.dfunc.ddns.server[i].on_id);
        fprintf(fp, "<on_pwd>%d</on_pwd>\n", ivsc.dfunc.ddns.server[i].on_pw);
        fprintf(fp, "<on_nvr>%d</on_nvr>\n", ivsc.dfunc.ddns.server[i].on_nvr);
        fprintf(fp, "<on_mac>%d</on_mac>\n", ivsc.dfunc.ddns.server[i].on_mac);
        fprintf(fp, "<on_status>%d</on_status>\n", ivsc.dfunc.ddns.server[i].on_status);
        fprintf(fp, "</ddns%d>\n", i+1);
        fprintf(fp, "\n");
    }

    fprintf(fp, "<backdoor_password>\n");
    fprintf(fp, "<pw>%s</pw>\n", ivsc.dfunc.backdoorpw.password);
    fprintf(fp, "</backdoor_password>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<!-- ====================================================================== -->\n");
    fprintf(fp, "<!-- PARAMETER CONFIGURATION -->\n");
    fprintf(fp, "<!-- ====================================================================== -->\n");
    fprintf(fp, "\n");

    fprintf(fp, "<system_management>\n");
    fprintf(fp, "<swmode>%d</swmode>\n", ivsc.dfunc.support_swmode);
    fprintf(fp, "<net_wiz>%d</net_wiz>\n", ivsc.dfunc.wizard.support);
    fprintf(fp, "</system_management>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub1>\n");
    fprintf(fp, "<camera_tile>%d</camera_tile>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_CAMERA_TITLE)) > 0 ? 1 : 0);
    fprintf(fp, "<image>%d</image>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_IMAGE_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<covert>%d</covert>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_COVERT_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<motion>%d</motion>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_MOTION_SENSOR)) > 0 ? 1 : 0);
    fprintf(fp, "<camera_type>%d</camera_type>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_CAMERA_TYPE)) > 0 ? 1 : 0);
    fprintf(fp, "<ptz>%d</ptz>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_PTZ_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<onvif_image>%d</onvif_image>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ONVIF_IMAGE_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<onvif_motion>%d</onvif_motion>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ONVIF_MOTION_SENSOR)) > 0 ? 1 : 0);
    fprintf(fp, "<privacy_mask>%d</privacy_mask>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_PRIVACY_MASK)) > 0 ? 1 : 0);
    fprintf(fp, "<install_mode>%d</install_mode>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_INSTALL_MODE)) > 0 ? 1 : 0);
    fprintf(fp, "<ipcamera_install>%d</ipcamera_install>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_IPCAMERA_INSTALL)) > 0 ? 1 : 0);
    fprintf(fp, "<vca_setup_itx>%d</vca_setup_itx>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_VCA_SETUP_ITX)) > 0 ? 1 : 0);
    fprintf(fp, "<vca_rev_setup_itx>%d</vca_rev_setup_itx>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_VCA_REV_SETUP_ITX)) > 0 ? 1 : 0);
    fprintf(fp, "<vca_setup_s1>%d</vca_setup_s1>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_VCA_SETUP_S1)) > 0 ? 1 : 0);
    fprintf(fp, "<tamper_setup>%d</tamper_setup>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_TAMPER_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<analog_type>%d</analog_type>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_ANALOG_TYPE)) > 0 ? 1 : 0);
    fprintf(fp, "<special_cam_setup>%d</special_cam_setup>\n", (ivsc.dmenu.submenu_camera & (1 << SYS_SUB1_SPECIAL_CAM_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub1>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub2>\n");
    fprintf(fp, "<osd>%d</osd>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_OSD)) > 0 ? 1 : 0);
    fprintf(fp, "<monitor>%d</monitor>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_MONITOR)) > 0 ? 1 : 0);
    fprintf(fp, "<sequence>%d</sequence>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_SEQUENCE)) > 0 ? 1 : 0);
    fprintf(fp, "<spot>%d</spot>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_SPOTOUT)) > 0 ? 1 : 0);
    fprintf(fp, "<posatm>%d</posatm>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_POSATM)) > 0 ? 1 : 0);
    fprintf(fp, "<advanced_dual>%d</advanced_dual>\n", (ivsc.dmenu.submenu_disp & (1 << SYS_SUB2_ADVANCED_DUAL)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub2>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub3>\n");
    fprintf(fp, "<audio>%d</audio>\n", (ivsc.dmenu.submenu_sound & (1 << SYS_SUB3_AUDIO)) > 0 ? 1 : 0);
    fprintf(fp, "<buzzer>%d</buzzer>\n", (ivsc.dmenu.submenu_sound & (1 << SYS_SUB3_BUZZER)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub3>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub4>\n");
    fprintf(fp, "<management>%d</management>\n", (ivsc.dmenu.submenu_user & (1 << SYS_SUB4_MANAGEMENT)) > 0 ? 1 : 0);
    fprintf(fp, "<authority>%d</authority>\n", (ivsc.dmenu.submenu_user & (1 << SYS_SUB4_AUTHORITY)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub4>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub5>\n");
    fprintf(fp, "<ip_setup>%d</ip_setup>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_IPSETUP)) > 0 ? 1 : 0);
    fprintf(fp, "<ddns>%d</ddns>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_DDNS)) > 0 ? 1 : 0);
    fprintf(fp, "<email>%d</email>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_EMAIL)) > 0 ? 1 : 0);
    fprintf(fp, "<net_status>%d</net_status>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_NETSTAT)) > 0 ? 1 : 0);
    fprintf(fp, "<security>%d</security>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_SECURITY)) > 0 ? 1 : 0);
    fprintf(fp, "<snmp>%d</snmp>\n", (ivsc.dmenu.submenu_network & (1 << SYS_SUB5_SNMP)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub5>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub6>\n");
    fprintf(fp, "<date_time>%d</date_time>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_DATETIME)) > 0 ? 1 : 0);
    fprintf(fp, "<management>%d</management>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_MANAGEMENT)) > 0 ? 1 : 0);
    fprintf(fp, "<information>%d</information>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_INFORMATION)) > 0 ? 1 : 0);
    fprintf(fp, "<control_dev>%d</control_dev>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_CONTROLDEVICE)) > 0 ? 1 : 0);
    fprintf(fp, "<support_ipcam>%d</support_ipcam>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_SUPPORT_IPCAM)) > 0 ? 1 : 0);
    fprintf(fp, "<security>%d</security>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_SECURITY)) > 0 ? 1 : 0);
    fprintf(fp, "<posatm>%d</posatm>\n", (ivsc.dmenu.submenu_system & (1 << SYS_SUB6_POSATM)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub6>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub7>\n");
    fprintf(fp, "<information>%d</information>\n", (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_INFOMATION)) > 0 ? 1 : 0);
    fprintf(fp, "<operation>%d</operation>\n", (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_OPERATION)) > 0 ? 1 : 0);
    fprintf(fp, "<configuration>%d</configuration>\n", (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_CONFIGURATION)) > 0 ? 1 : 0);
    fprintf(fp, "<smart>%d</smart>\n", (ivsc.dmenu.submenu_storage & (1 << SYS_SUB7_SMARTSETUP)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub7>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<sys_menu_sub8>\n");
    fprintf(fp, "<alarm_out>%d</alarm_out>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_ALARMOUT)) > 0 ? 1 : 0);
    fprintf(fp, "<notification>%d</notification>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_EVENTNOTI)) > 0 ? 1 : 0);
    fprintf(fp, "<alarm_sensor>%d</alarm_sensor>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_ALARMSENSOR)) > 0 ? 1 : 0);
    fprintf(fp, "<motion_sensor>%d</motion_sensor>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_MOTIONSENSOR)) > 0 ? 1 : 0);
    fprintf(fp, "<video_loss>%d</video_loss>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_VIDEOLOSS)) > 0 ? 1 : 0);
    fprintf(fp, "<system_event>%d</system_event>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_SYSTEMEVENT)) > 0 ? 1 : 0);
    fprintf(fp, "<vca_event_itx>%d</vca_event_itx>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_VCA_EVENT_ITX)) > 0 ? 1 : 0);
    fprintf(fp, "<vca_s1_event>%d</vca_s1_event>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_VCA_EVENT_S1)) > 0 ? 1 : 0);
    fprintf(fp, "<tamper_event>%d</tamper_event>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_TAMPER)) > 0 ? 1 : 0);
    fprintf(fp, "<textin_event>%d</textin_event>\n", (ivsc.dmenu.submenu_event & (1 << SYS_SUB8_POSATM_EVENT)) > 0 ? 1 : 0);
    fprintf(fp, "</sys_menu_sub8>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<event_noti>\n");
    fprintf(fp, "<buzzer>%d</buzzer>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_BUZZER)) > 0 ? 1 : 0);
    fprintf(fp, "<display>%d</display>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_DISP)) > 0 ? 1 : 0);
    fprintf(fp, "<email>%d</email>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_MAIL)) > 0 ? 1 : 0);
    fprintf(fp, "<ftp>%d</ftp>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_FTP)) > 0 ? 1 : 0);
    fprintf(fp, "<sms>%d</sms>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_SMS)) > 0 ? 1 : 0);
    fprintf(fp, "<mobilepush>%d</mobilepush>\n", (ivsc.dmenu.submenu_event_noti & (1 << SYS_EVENTNOTI_MOBILEPUSH)) > 0 ? 1 : 0);
    fprintf(fp, "</event_noti>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<alarm_sensor_evt>\n");
    fprintf(fp, "<camera>%d</camera>\n", (ivsc.dmenu.submenu_alarm_sensor_evt& (1 << SYS_ALARM_SENSOR_CAM)) > 0 ? 1 : 0);
    fprintf(fp, "<local>%d</local>\n", (ivsc.dmenu.submenu_alarm_sensor_evt & (1 << SYS_ALARM_SENSOR_LOCAL)) > 0 ? 1 : 0);
    fprintf(fp, "</alarm_sensor_evt>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<rec_menu>\n");
    fprintf(fp, "<operation>%d</operation>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_OPER)) > 0 ? 1 : 0);
    fprintf(fp, "<continuous>%d</continuous>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_CONT)) > 0 ? 1 : 0);
    fprintf(fp, "<motion>%d</motion>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_MOT)) > 0 ? 1 : 0);
    fprintf(fp, "<alarm>%d</alarm>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_ALARM)) > 0 ? 1 : 0);
    fprintf(fp, "<panic>%d</panic>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_PANIC)) > 0 ? 1 : 0);
    fprintf(fp, "<net_stream>%d</net_stream>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_NETSTREAM)) > 0 ? 1 : 0);
    fprintf(fp, "<storage_calcul>%d</storage_calcul>\n", (ivsc.dmenu.recmenu & (1 << REC_SUB_CALCUL)) > 0 ? 1 : 0);
    fprintf(fp, "</rec_menu>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<arch_menu>\n");
    fprintf(fp, "<new_arch>%d</new_arch>\n", (ivsc.dmenu.archmenu & (1 << ARCH_SUB_NEW_ARCH)) > 0 ? 1 : 0);
    fprintf(fp, "<reserved>%d</reserved>\n", (ivsc.dmenu.archmenu & (1 << ARCH_SUB_RESERVED)) > 0 ? 1 : 0);
    fprintf(fp, "<data_playback>%d</data_playback>\n", (ivsc.dmenu.archmenu & (1 << ARCH_SUB_DATA_PB)) > 0 ? 1 : 0);
    fprintf(fp, "<dev_setup>%d</dev_setup>\n", (ivsc.dmenu.archmenu & (1 << ARCH_SUB_DEV_SETUP)) > 0 ? 1 : 0);
    fprintf(fp, "</arch_menu>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<search_menu>\n");
    fprintf(fp, "<time>%d</time>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_TIME)) > 0 ? 1 : 0);
    fprintf(fp, "<thumbnail>%d</thumbnail>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_THUMBNAIL)) > 0 ? 1 : 0);
    fprintf(fp, "<event>%d</event>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_EVENT)) > 0 ? 1 : 0);
    fprintf(fp, "<textin>%d</textin>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_TEXT)) > 0 ? 1 : 0);
    fprintf(fp, "<statistic>%d</statistic>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_VA_STATISTIC)) > 0 ? 1 : 0);
    fprintf(fp, "<smart>%d</smart>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_SMART)) > 0 ? 1 : 0);
    fprintf(fp, "<smart_rev>%d</smart_rev>\n", (ivsc.dmenu.searchmenu & (1 << SEARCH_SUB_SMART_REV)) > 0 ? 1 : 0);
    fprintf(fp, "</search_menu>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<delete_menu>\n");
    fprintf(fp, "<erase>%d</erase>\n", (ivsc.dmenu.submenu_storage_del & (1 << SYS_SUB7_OPERATION_DELETE)) > 0 ? 1 : 0);
    fprintf(fp, "</delete_menu>\n");
    fprintf(fp, "\n");

    fprintf(fp, "<vendor>\n");
    fprintf(fp, "<vendor_no>%d</vendor_no>\n", vendor);
    fprintf(fp, "</vendor>\n");
    fprintf(fp, "\n");

    fprintf(fp, "</itx_base>\n");

	fclose(fp);

    return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_desc_init()
{
    memset(&ivsc, 0x00, sizeof(VWDESC_T));

    _get_model(&ivsc.model_code);
    _parser_swver(&ivsc.dswver);

    ivsc.vendor_code = ivsc.dswver.vendor;
    DMSG(1, "model : %08X, vendor:%d", ivsc.model_code, ivsc.vendor_code);

    _vw_desc_init(ivsc.model_code, ivsc.vendor_code);

    return 0;
}
