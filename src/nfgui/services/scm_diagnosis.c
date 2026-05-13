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
#include "nf_util_mail.h"
#include "nf_util_netif.h"
#include "nf_util_ftp.h"
#include "nf_network.h"
#include "nf_issm_ctl.h"
#include "../../service/ddns2_manager.h"
#include "../modules/wrk.h"
#include "scm.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "ix_func.h"
#include "vfs.h"
#include "nf_api_ipcam.h"
#include "nf_netsvr.h"
#include "nf_util_sms.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_DIAG"




////////////////////////////////////////////////////////////
//
// public data type 
//





////////////////////////////////////////////////////////////
//
// private functions
//

static int _proc_diagnosis_ipcam_network(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	BITMASK chmask;
	NF_NOTIFY_INFO notify;

	DIAG_RES_T *diag_res = imalloc(sizeof(DIAG_RES_T));
	NF_NETIF_PING_TEST test;
	char buf[32];
	int ch, ack_ret = 0;
	
	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	chmask = (BITMASK)piscm->chart[tra].int_data;
	DMSG(1, "chmask : %08X", chmask);

	scm_get_vloss_event_data(&notify);

	memset(diag_res, 0x00, sizeof(DIAG_RES_T));
	diag_res->type = DIAG_IPCAM_NET;

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		memset(buf, 0x00, sizeof(buf));
		memset(&test, 0x00, sizeof(NF_NETIF_PING_TEST));

		if (~chmask & (1 << ch)) continue;
		if (notify.d.params[0] & (1 << ch)) continue;
	
		nf_ipcam_get_ipstr(ch, buf);
		if (!strlen(buf)) continue;
		
		if (nf_netif_ping_test_advanced(buf, 3, 10000, &test)) {
			diag_res->ipcam_net[ch] = DIAG_RES_SUCCESS;
		}
		else {
			diag_res->ipcam_net[ch] = DIAG_RES_ERROR;
		}
	}

	piscm->chart[tra].result = (void *)diag_res;
	
	return ack_ret;
}

static int _proc_diagnosis_ipcam_power(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	BITMASK chmask;
	NF_NOTIFY_INFO notify;

	DIAG_RES_T *diag_res = imalloc(sizeof(DIAG_RES_T));
	NF_UTIL_POE_INFO info;
	int ch, ack_ret = 0;
	
	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	chmask = (BITMASK)piscm->chart[tra].int_data;
	DMSG(1, "chmask : %08X", chmask);

	scm_get_vloss_event_data(&notify);
	
	memset(diag_res, 0x00, sizeof(DIAG_RES_T));
	diag_res->type = DIAG_IPCAM_POWER;

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		memset(&info, 0x00, sizeof(NF_UTIL_POE_INFO));

		if (~chmask & (1 << ch)) continue;
		if (notify.d.params[0] & (1 << ch)) continue;
			if (nf_api_live_poe_is_ok(ch, &info)) {
				diag_res->ipcam_power[ch] = DIAG_RES_SUCCESS;
			}
			else {
				diag_res->ipcam_power[ch] = DIAG_RES_ERROR;
			}		
	}
	
	piscm->chart[tra].result = (void *)diag_res;

	sleep(2);

	return ack_ret;
}

static int _proc_diagnosis_disk(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	gboolean ret;

	DIAG_RES_T *diag_res = imalloc(sizeof(DIAG_RES_T));
	NF_DISK_INFO ndi;
	NF_SMART_DISK_INFO sdi;
	int i, ack_ret = 0;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	memset(diag_res, 0x00, sizeof(DIAG_RES_T));
	diag_res->type = DIAG_DISK;

	memset(&ndi, 0x00, sizeof(NF_DISK_INFO));
	ret = nf_disk_get_info(&ndi, NULL);
	if (!ret)
	{
		piscm->chart[tra].result = (void *)diag_res;
		return -1;
	}

	for (i = 0; i < 5; i++)
	{
		memset(&sdi, 0x00, sizeof(DISK_SMARTINFO_T));

		if (ndi.ucPortNo[INTERNAL][i] == 0xff) continue;
		
		if (nf_smart_get_info(INTERNAL, i, &sdi, NULL)) 
		{
			if (sdi.disk_status == 3) diag_res->internal_disk[i] = DIAG_RES_ERROR;
			else if (sdi.disk_status == 2) diag_res->internal_disk[i] = DIAG_RES_CHECK;
			else diag_res->internal_disk[i] = DIAG_RES_SUCCESS;
		}
		else {
			diag_res->internal_disk[i] = DIAG_RES_ERROR;
		}		
	}

	for (i = 0; i < 5; i++)
	{
		memset(&sdi, 0x00, sizeof(DISK_SMARTINFO_T));

		if (ndi.ucPortNo[EXTERNAL][i] == 0xff) continue;
		
		if (nf_smart_get_info(EXTERNAL, i, &sdi, NULL)) 
		{
			if (sdi.disk_status == 3) diag_res->external_disk[i] = DIAG_RES_ERROR;
			else if (sdi.disk_status == 2) diag_res->external_disk[i] = DIAG_RES_CHECK;
			else diag_res->external_disk[i] = DIAG_RES_SUCCESS;
		}
		else {
			diag_res->external_disk[i] = DIAG_RES_ERROR;
		}	
	}
	
	piscm->chart[tra].result = (void *)diag_res;

	sleep(2);

	return ack_ret;
}

static int _proc_diagnosis_service_port(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	IPSetupData setup_data;

	DIAG_RES_T *diag_res = imalloc(sizeof(DIAG_RES_T));
	int ack_ret = 0;
		
	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	memset(diag_res, 0x00, sizeof(DIAG_RES_T));
	diag_res->type = DIAG_SERVICE_PORT;

	memset(&setup_data, 0x00, sizeof(IPSetupData));
	DAL_get_ipSetup_data(&setup_data);

	if (nf_upnp_get_status_port(setup_data.rtspport, 0) == PORT_USE_ME) {
		diag_res->rtsp_port = DIAG_RES_SUCCESS;
	}
	else {
		diag_res->rtsp_port = DIAG_RES_ERROR;
	}

	if (nf_upnp_get_status_port(setup_data.webPort, 1) == PORT_USE_ME) {
		diag_res->web_port = DIAG_RES_SUCCESS;
	}
	else {
		diag_res->web_port = DIAG_RES_ERROR;
	}
	
	piscm->chart[tra].result = (void *)diag_res;

	return ack_ret;
}

static int _scm_cleanup_wrk_diagnosis_ipcam_net_wrk(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_diagnosis_ipcam_net);
	piscm->wrk_diagnosis_ipcam_net = 0;
	return 0;
}

static int _scm_cleanup_wrk_diagnosis_ipcam_power_wrk(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_diagnosis_ipcam_power);
	piscm->wrk_diagnosis_ipcam_power = 0;
	return 0;
}

static int _scm_cleanup_wrk_diagnosis_disk_wrk(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_diagnosis_disk);
	piscm->wrk_diagnosis_disk = 0;
	return 0;
}

static int _scm_cleanup_wrk_diagnosis_port_wrk(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_diagnosis_port);
	piscm->wrk_diagnosis_port = 0;
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

HANDLER int _scm_on_diagnosis_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
		case TRA_DIAGNOSIS_IPCAM_NET:
			DMSG(9, "");
			_scm_finalize_tra(piscm, tra, result);
			_scm_cleanup_wrk_diagnosis_ipcam_net_wrk(piscm);
		break;			
		case TRA_DIAGNOSIS_IPCAM_POWER:
			DMSG(9, "");
			_scm_finalize_tra(piscm, tra, result);
			_scm_cleanup_wrk_diagnosis_ipcam_power_wrk(piscm);
		break;			
		case TRA_DIAGNOSIS_DISK:
			DMSG(9, "");
			_scm_finalize_tra(piscm, tra, result);
			_scm_cleanup_wrk_diagnosis_disk_wrk(piscm);
		break;			
		case TRA_DIAGNOSIS_PORT:
			DMSG(9, "");
			_scm_finalize_tra(piscm, tra, result);
			_scm_cleanup_wrk_diagnosis_port_wrk(piscm);
		break;		
	}
	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_diagnosis_ipcam_network(BITMASK chmask, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DIAGNOSIS_IPCAM_NET;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_DIAGNOSIS, (void *)tra };

	DMSG(9, "");

	if (iscm.wrk_diagnosis_ipcam_net) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = chmask;	

	iscm.wrk_diagnosis_ipcam_net = wrk_create_worker(_proc_diagnosis_ipcam_network, &cmmack);
	wrk_run_once_param(iscm.wrk_diagnosis_ipcam_net, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_diagnosis_ipcam_power(BITMASK chmask, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DIAGNOSIS_IPCAM_POWER;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_DIAGNOSIS, (void *)tra };

	DMSG(9, "");
	
	if (iscm.wrk_diagnosis_ipcam_power) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = chmask;	

	iscm.wrk_diagnosis_ipcam_power = wrk_create_worker(_proc_diagnosis_ipcam_power, &cmmack);
	wrk_run_once_param(iscm.wrk_diagnosis_ipcam_power, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_diagnosis_disk(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DIAGNOSIS_DISK;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_DIAGNOSIS, (void *)tra };

	if (iscm.wrk_diagnosis_disk) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_diagnosis_disk = wrk_create_worker(_proc_diagnosis_disk, &cmmack);
	wrk_run_once_param(iscm.wrk_diagnosis_disk, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_diagnosis_service_port(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DIAGNOSIS_PORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_DIAGNOSIS, (void *)tra };
	
	if (iscm.wrk_diagnosis_port) return -1;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_diagnosis_port = wrk_create_worker(_proc_diagnosis_service_port, &cmmack);
	wrk_run_once_param(iscm.wrk_diagnosis_port, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

