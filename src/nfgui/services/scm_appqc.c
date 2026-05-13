/*
 * scm_diagnosis.c
 * 	- scm diagnosis service
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, DEC 21, 2013
 *
 */

#include "scm_internal.h"
#include "nf_object.h"
#include "../modules/wrk.h"
#include "scm.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "ix_func.h"
#include "vfs.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_APPQC"




////////////////////////////////////////////////////////////
//
// public data type
//





////////////////////////////////////////////////////////////
//
// private functions
//

static int _proc_appqc_rtc_sync(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	char server_name[128];
	GTimeVal *tv_temp;
	int ret = 0;
	int mode;

	memset(server_name, 0x00, sizeof(server_name));
	//memset(tv_temp, 0x00, sizeof(GTimeVal));

	tv_temp = imalloc(sizeof(GTimeVal));

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    nf_sysman_qc_get_ntp_serverip(server_name);
    if (nf_sysman_hotkey_is_nfs())
    {
        strcpy(server_name, "time.bora.net");
    }
    g_message("%s, %d, NTPSERVER : %s\n", __FUNCTION__, __LINE__, server_name);


	mode = get_qc_mac_writer_mode();
	if(mode == 1){						//ntp
	
    ret = scm_get_ntp_time(server_name, tv_temp);
	if (ret < 0) {
	    DMSG(9, "NTP QUERY RETURN : [%d]", ret);
		return FALSE;
	}
	}
	else{
		ret = scm_get_ntp_time_pc(server_name, tv_temp);
		if (ret < 0) {
		    DMSG(9, "NTP QUERY RETURN : [%d]", ret);
			return FALSE;
		}
	}

	if (tv_temp->tv_sec != 0) {
		nf_datetime_set((time_t)tv_temp->tv_sec);
		//scm_set_system_time(&tv_temp);
	}
    piscm->chart[tra].result = (void *)tv_temp;

	return 0;
}

static int _proc_appqc_alarm_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    if (!nf_sysman_qc_auto_test_alarm()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_audio_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	if (ivsc.model_code == IPX_M4_MODEL || ivsc.model_code == IPX_P4E_MODEL) 
	{
	    if (GUI_CHANNEL_CNT != 16 && GUI_CHANNEL_CNT != 32) {
            return 0;
        }
        else {
#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		if(nf_sysman_get_pba_type()) return 0;
#endif
        }
    }

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    if (!nf_sysman_qc_auto_test_audio()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_network_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	NF_NETIF_GET_INFO ret_net_info;

    NF_NOTIFY_INFO pInfo;

	int ack_ret = 2;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	scm_get_sys_netinfo(&ret_net_info);

	if (((ret_net_info.ipaddr >> 24) & 255) != 255) ack_ret = 1;
	if (((ret_net_info.ipaddr >> 16) & 255) != 255) ack_ret = 1;
	if (((ret_net_info.ipaddr >> 8) & 255) != 255) ack_ret = 1;
	if ((ret_net_info.ipaddr & 255) != 255) ack_ret = 1;
        if (!nf_sysman_qc_auto_test_network_switch()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_rs485_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E)|| defined(_IPX_32P4E) || defined(_IPX_32P5)
    if (!nf_sysman_qc_auto_test_rs485()) ack_ret = 2;
#endif

	return ack_ret;
}

static int _proc_appqc_fan_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    if (!nf_sysman_qc_auto_test_fan()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_temper_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    if (!nf_sysman_qc_auto_test_temper()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_poe_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    if (!nf_sysman_qc_auto_test_poe()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_rs232_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    // if (!nf_sysman_qc_auto_test_rs232()) ack_ret = 2;

	return ack_ret;
}

static int _proc_appqc_all_test(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	APPQC_RES_T *res = imalloc(sizeof(APPQC_RES_T));

    DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    _proc_appqc_rtc_sync(wrkid, pmsg);
    res->alarm = _proc_appqc_alarm_test(wrkid, pmsg);
    res->audio = _proc_appqc_audio_test(wrkid, pmsg);
    res->network = _proc_appqc_network_test(wrkid, pmsg);
    res->rs485 = _proc_appqc_rs485_test(wrkid, pmsg);
    res->fan = _proc_appqc_fan_test(wrkid, pmsg);
    res->temper = _proc_appqc_temper_test(wrkid, pmsg);
    res->poe = _proc_appqc_poe_test(wrkid, pmsg);
    res->rs232= _proc_appqc_rs232_test(wrkid, pmsg);

	piscm->chart[tra].result = (void *)res;

	return 0;
}

static int _proc_appqc_factory_default(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 0;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

    nf_sysman_qc_factory_default();

	return ack_ret;
}

static int _proc_appqc_dhcp_renew(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	int ack_ret = 0;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	nf_netif_init();

	return ack_ret;
}

static int _scm_cleanup_wrk_appqc(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_appqc);
	piscm->wrk_appqc = 0;
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

HANDLER int _scm_on_appqc_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
		case TRA_APPQC_TEST:
			DMSG(9, "");
			_scm_finalize_tra(piscm, tra, result);
			_scm_cleanup_wrk_appqc(piscm);
		break;
	}
	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_appqc_is_running()
{
	if (iscm.wrk_appqc) return 1;

    return 0;
}

int scm_appqc_all_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_all_test, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_rtc_sync(IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_rtc_sync, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);

	return 0;
}

int scm_appqc_alarm_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_alarm_test, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_audio_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_audio_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_network_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_network_test, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_rs485_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_rs485_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_fan_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_fan_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_temper_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_temper_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_poe_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_poe_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_rs232_test(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_rs232_test, &cmmack);
	DMSG(9, "");
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_factory_default(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_factory_default, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_appqc_get_rtc_str(char *rtc_info)
{
    int ret = 0;

    if(nf_sysman_qc_info_rtc(rtc_info)){
        ret = 1;
    }

    return ret;
}

int scm_appqc_dhcp_renew(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_APPQC_TEST;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_APPQC_TEST, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_appqc) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_appqc = wrk_create_worker(_proc_appqc_dhcp_renew, &cmmack);
	wrk_run_once_param(iscm.wrk_appqc, IMSG_NONE, &iscm, 0, 0);
	return 0;
}
