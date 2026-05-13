/*
 * scm_license.c
 * 	- license services
 *	- dependency :
 *
 * Written by Yang Jeong-Ho. <yanggungg@itxm2m.com>
 * Copyright (c) ITX M2M, JUL 17, 2018
 *
 */


#include "scm.h"
#include "wrk.h"
#include "scm_internal.h"
#include "nf_ipcam_defs.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_LICENSE"

typedef enum {
    LIC_CAM_NONE = 0,
    LIC_CAM_DMVA = 1,
}LICENSE_CAM_E;

typedef struct _LICENSE_FUNC_T {
    int dlva;
} LICENSE_FUNC_T;

static LICENSE_FUNC_T lic_func;




////////////////////////////////////////////////////////////
//
// private functions
//

static int _get_license_secret_key(int ch, NF_SYSDB_LICENSE_SECRET_KEY *secret_key)
{
	NF_NETIF_GET_INFO ret_net_info;
	CAM_PROFILE_T prof;
	char ver[32];
	char mac_string[32];
	char *model_code = NULL;
    char *save_ptr = NULL;
    
    memset(&ret_net_info, 0x00, sizeof(NF_NETIF_GET_INFO));
    memset(&prof, 0x00, sizeof(CAM_PROFILE_T));
    memset(ver, 0x00, sizeof(ver));
    memset(mac_string, 0x00, sizeof(mac_string));
    
    if (ch == -1)
    {
    	nf_netif_get_info(&ret_net_info);
    	memcpy(secret_key->mac_addr, ret_net_info.mac_addr, NF_SYSDB_LICENSE_MAC_LEN);
    	
        DAL_get_fw_version(ver);
        model_code = strtok_r(ver, ".", &save_ptr);
        if (model_code)
            strcpy(secret_key->model, model_code);

        if (nf_sysman_hotkey_is_nfs()) {
            strcpy(secret_key->model, "00000");  //temp
        }
    }
    else
    {
        scm_get_cam_profile(ch, &prof);
        
    	memcpy(secret_key->mac_addr, prof.model.mac, NF_SYSDB_LICENSE_MAC_LEN);
    	
        model_code = strtok_r(prof.model.swver, ".", &save_ptr);
        if (model_code)
            strcpy(secret_key->model, model_code);
    }
    
    DMSG(9, "MODEL CODE : %s", secret_key->model);
    
    
	g_sprintf(mac_string,"%02x:%02x:%02x:%02x:%02x:%02x", 
			(guchar)secret_key->mac_addr[0],(guchar)secret_key->mac_addr[1], (guchar)secret_key->mac_addr[2], (guchar)secret_key->mac_addr[3], (guchar)secret_key->mac_addr[4], (guchar)secret_key->mac_addr[5]);
    DMSG(9, "MAC ADDRESS : %s", mac_string);

    return 0;
}

static int _proc_get_license(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	NF_SYSDB_LICENSE_SECRET_KEY secret_key;
    LICENSE_KEY_T out_key[GUI_CHANNEL_CNT+1];
    int ch, idx, i;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(1, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	memset(out_key, 0x00, sizeof(LICENSE_KEY_T)*(GUI_CHANNEL_CNT+1));

    for (ch = -1; ch < GUI_CHANNEL_CNT; ch++) 
    {
    	memset(&secret_key, 0x00, sizeof(NF_SYSDB_LICENSE_SECRET_KEY));
        idx = ch+1;
	    _get_license_secret_key(ch, &secret_key);
	    
    	out_key[idx].key_count = nf_sysdb_license_recive_from_svr(&secret_key, out_key[idx].lic_info, MAX_LICENSE_CNT);

    	DMSG(9, "ch : %d, out_key[%d].key_count : %d", ch, idx, out_key[idx].key_count);
    	for (i = 0; i < out_key[idx].key_count; i++) {
        	DMSG(9, "out_key[%d].lic_info[%d].key : %s", idx, i, out_key[idx].lic_info[i].key);
    	}

    	if (out_key[idx].key_count == -2)   // server connect error
    	{
    	    return -2;
    	}
	}
	
	for (i = 0; i <= GUI_CHANNEL_CNT; i++) {
	    DMSG(9, "out_key[%d].key_count : %d", i, out_key[i].key_count);
	}

	piscm->chart[tra].result = (LICENSE_KEY_T*)imalloc(sizeof(LICENSE_KEY_T) * (GUI_CHANNEL_CNT+1));
	memset(piscm->chart[tra].result, 0x00, sizeof(LICENSE_KEY_T)*(GUI_CHANNEL_CNT+1));
	memmove(piscm->chart[tra].result, out_key, sizeof(LICENSE_KEY_T) * (GUI_CHANNEL_CNT+1));
	DMSG(9, "piscm->chart[tra].result addr : %p", piscm->chart[tra].result);

    return 0;
}

static void _show_active_license()
{
    DMSG(9, "DLVA : [%d]", lic_func.dlva);
}

static int _active_license(int ch, char *lic_name)
{
    if (strcmp(lic_name, "DLVA") == 0) {
        lic_func.dlva = 1;
    }

    return 0;
}

static int _get_cam_license(char *lic_name, guint *act_lic)
{
    if (strcmp(lic_name, "VA") == 0) {
        *act_lic |= (1 << LIC_CAM_DMVA);
    }

    return 0;
}

static guint _get_active_cam_license(int ch)
{
    NF_SYSDB_LICENSE_INFO lic_info;
    NFIPCamLicenseKeyList cam_lic;
    int ret, i;
    guint act_lic = 0;

//    DMSG(1, "");
    
    memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
    memset(&cam_lic, 0x00, sizeof(NFIPCamLicenseKeyList));
    
    ret = nf_ipcam_get_camera_license_key(ch, &cam_lic);

    if (ret == IPCAM_SETUP_RTN_DONE)
    {
        for (i = 0; i < cam_lic.count; i++)
        {
            _get_cam_license(cam_lic.key_data[i].name, &act_lic);
        }
    }

    return act_lic;
}

static int _set_active_local_license()
{
    NF_SYSDB_LICENSE_INFO lic_info;
    LicenseData data;
    int i;

    memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
    memset(&data, 0x00, sizeof(LicenseData));
    
    DAL_get_license_data(&data);

    for (i = 0; i < data.count; i++)
    {
        if (scm_license_decoding_key(-1, data.key_data[i].key, &lic_info) == 0)
        {
            _active_license(-1, lic_info.name);
        }
    }
    
    _show_active_license();

    return 0;
}

static int _cleanup_get_license(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_license);
	piscm->wrk_license = 0;
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_on_get_license_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_GET_LICENSE:
		DMSG(1, "");
		_scm_finalize_tra(piscm, tra, result);
		_cleanup_get_license(piscm);
		break;
	}
	return 0;
}

int _scm_license_init()
{
    memset(&lic_func, 0x00, sizeof(LICENSE_FUNC_T));
    _set_active_local_license();

    return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

// VA = DMVA + DLVA

int scm_is_supported_va()
{
    int supported = 0;

    if (ivsc.dfunc.support_dmva_s1) supported = 1;
    if (ivsc.dfunc.support_dlva_itx) supported = 1;
    if (ivsc.dfunc.support_aicam_itx) supported = 1;
    if (ivsc.dfunc.support_aibox_itx) supported = 1;

    // g_message("########## %s, %d, supported:%d", __FUNCTION__, __LINE__, supported);
    return supported;
}

int scm_is_supported_dmva()
{
    int supported = 0;

    if (ivsc.dfunc.support_dmva_itx) supported = 1;
    if (ivsc.dfunc.support_dmva_s1) supported = 1;

    // g_message("########## %s, %d, supported:%d", __FUNCTION__, __LINE__, supported);
    return supported;
}

int scm_is_supported_dlva()
{
    int supported = 0;

    if (ivsc.dfunc.support_dlva_itx) supported = 1;

    // g_message("########## %s, %d, supported:%d", __FUNCTION__, __LINE__, supported);
    return supported;
}

int scm_is_supported_aicam()
{
    int supported = 0;

    if (ivsc.dfunc.support_aicam_itx) supported = 1;

    // g_message("########## %s, %d, supported:%d", __FUNCTION__, __LINE__, supported);
    return supported;
}

int scm_is_supported_aibox()
{
    int supported = 0;

    if (ivsc.dfunc.support_aibox_itx) supported = 1;

    // g_message("########## %s, %d, supported:%d", __FUNCTION__, __LINE__, supported);
    return supported;
}

int scm_license_get_from_server(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_GET_LICENSE;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_GET_LICENSE, (void *)tra };
	
	if (iscm.wrk_license) return -1;

	DMSG(1, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_license = wrk_create_worker(_proc_get_license, &cmmack);
	wrk_run_once_param(iscm.wrk_license, IMSG_NONE, &iscm, 0, 0);

	return 0;
}

//Entering -1 for ch means NVR(DVR).
int scm_license_decoding_key(int ch, char *input_code, NF_SYSDB_LICENSE_INFO *lic_info)
{
    NF_SYSDB_LICENSE_SECRET_KEY secret_key;
    NF_SYSDB_LICENSE_INFO out_key;
	CAM_PROFILE_T prof;
	char *model_code = NULL;
    int ret = 0;

    DMSG(1, "");
    
    memset(&secret_key, 0x00, sizeof(NF_SYSDB_LICENSE_SECRET_KEY));
    memset(&out_key, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
    memset(&prof, 0x00, sizeof(CAM_PROFILE_T));

    _get_license_secret_key(ch, &secret_key);
    
    DMSG(9, "ch : %d, key : %s", ch, input_code);
    
    ret = nf_sysdb_license_decoding(&secret_key, input_code, &out_key);
    DMSG(9, "ret : %d", ret);

    if (ret == 0) 
    {
        DMSG(9, "name : %s", out_key.name);
        DMSG(9, "key : %s", out_key.key);
        DMSG(9, "param1 : %08x", out_key.param1);
        DMSG(9, "param2 : %08x", out_key.param2);

        memmove(lic_info, &out_key, sizeof(NF_SYSDB_LICENSE_INFO));
    }

    return ret;
}

int scm_license_get_cam_license(int ch, LicenseData *lic_data)
{
    NFIPCamLicenseKeyList cam_lic;
    int ret, i;

    DMSG(1, "");
    
    memset(&cam_lic, 0x00, sizeof(NFIPCamLicenseKeyList));
    
    ret = nf_ipcam_get_camera_license_key(ch, &cam_lic);

    DMSG(9, "ch : %d, ret : %d", ch, ret);
    
    if (ret == IPCAM_SETUP_RTN_DONE)
    {
        for (i = 0; i < cam_lic.count; i++)
        {
            strcpy(lic_data->key_data[i].key, cam_lic.key_data[i].key);
            lic_data->key_data[i].acquired_date = cam_lic.key_data[i].acquired_date;
            lic_data->key_data[i].expired_date = cam_lic.key_data[i].expired_date;

            DMSG(9, "key : %s", cam_lic.key_data[i].key);
        }
        
        lic_data->count = cam_lic.count;
        
        DMSG(9, "key count : %d", cam_lic.count);
    }

    return 0;
}

int scm_license_set_cam_license(int ch, LicenseData *lic_data)
{
    int ret = 0, i;
    NFIPCamLicenseKeyInfo cam_lic[IPX_LICENSE_KEY_MAX];

    DMSG(1, "");
    
    memset(cam_lic, 0x00, sizeof(NFIPCamLicenseKeyInfo) * IPX_LICENSE_KEY_MAX);

    DMSG(9, "key count : %d", lic_data->count);
    
    for (i = 0; i < lic_data->count; i++)
    {
        strcpy(cam_lic[i].key, lic_data->key_data[i].key);
        cam_lic[i].acquired_date = lic_data->key_data[i].acquired_date;
        
        DMSG(9, "count : %d, key : %s", i, cam_lic[i].key);
        DMSG(9, "date : %d", cam_lic[i].acquired_date);
        
        ret = nf_ipcam_set_camera_license_key(ch, &cam_lic[i]);

        DMSG(9, "ret : %d", ret);
    }

    return ret;
}

int scm_license_is_activated_dmva(int ch)
{
    guint ret = 0;

#if _SKIP_CHECK_LICENSE
    if (nf_sysman_hotkey_is_nfs()) return 1;
#endif
    if (!ivsc.dfunc.support_dmva_itx) return 0;

//    DMSG(9, "");
    
    ret = _get_active_cam_license(ch);
    
//    DMSG(1, "ret : %08x", ret);
    
    return (ret & (1 << LIC_CAM_DMVA) ? 1 : 0);
}

int scm_license_is_activated_dlva()
{   
#if _SKIP_CHECK_LICENSE
    if (nf_sysman_hotkey_is_nfs()) return 1;
#endif
    if (!ivsc.dfunc.support_dlva_itx) return 0;

    return lic_func.dlva;
}

int scm_license_is_activated_aicam(int ch)
{   
#if _SKIP_CHECK_LICENSE
    if (nf_sysman_hotkey_is_nfs()) return 1;
#endif
    if (!ivsc.dfunc.support_aicam_itx) return 0;

    return 1;
}

guint scm_license_is_activated_aicam_mask()
{   
#if _SKIP_CHECK_LICENSE
    if (nf_sysman_hotkey_is_nfs()) return 0xffffffff;
#endif
    if (!ivsc.dfunc.support_aicam_itx) return 0;

    return 0xffffffff;
}

int scm_license_is_activated_aibox()
{   
#if _SKIP_CHECK_LICENSE
    if (nf_sysman_hotkey_is_nfs()) return 1;
#endif
    if (!ivsc.dfunc.support_aibox_itx) return 0;

    return 1;
}
