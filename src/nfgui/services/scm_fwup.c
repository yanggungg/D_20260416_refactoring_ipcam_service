/*
 * scm_fwup.c
 * 	- scm FW upgrade service
 *	- dependencies :
 *
 *
 * Written by eddy.kim
 * Copyright (c) ITX security, Jan 6, 2011
 *
 */

#include <glib.h>
#include "scm.h"
#include "scm_internal.h"
#include "iux_afx.h"
#include "cmm.h"
#include "nf_util_fw.h"
#include "nf_util_fw_s1.h"
#include "nf_util_fw_httpfs.h"
#include "ix_mem.h"
#include "vfs.h"
#include "nfdal.h"
#include "nf_ui_tool.h"
#include "evt.h"
#include "smt.h"
#include "ix_func.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_FWUP"

typedef struct  _CAMFW_UPGD_T
{
	gint step;
	guint upmask;
	guint start_mask;
	guint cmpl_mask;
	char path[16][256];
} CAMFW_UPGD_T;

#define PARTIAL_RATE(weight) 	(prgt.total == 0 ? 0 : (prgt.current * weight)/prgt.total)
#define FW_FILENAME			(piscm->chart[tra].alloc_data)
#define CAMFW_UPGD			(piscm->chart[tra].alloc_data)
#define DIRNAME				(piscm->chart[tra].alloc_data)
#define VENDOR				(piscm->chart[tra].int_data)
#define MOUNT_UPDATE_DIR        	"/mnt/upgrade_file"

static gint g_upgrade_ready_fail = 0;

////////////////////////////////////////////////////////////
//
// private function
//

static void _call_net_update_cbfunc(TRANSACTION_E tra, guint type, guint rate)
{
	NET_UPDATE_DATA *netfw_data;
	NET_UPDATE_STATE upgrade_state;

	netfw_data = (NET_UPDATE_DATA*)iscm.chart[tra].void_data;

	if (netfw_data->cb_func)
	{
		memset(&upgrade_state, 0x00, sizeof(NET_UPDATE_STATE));

		upgrade_state.type = type;
		upgrade_state.rate = rate;
		upgrade_state.data = netfw_data->cb_data;
		netfw_data->cb_func(&upgrade_state);
	}
}

static gint _error_ready_to_upgrade(TRANSACTION_E tra, gint param)
{
	CALLID callid = iscm.chart[tra].caller;
	IMSG ret_msg = iscm.chart[tra].ret_msg;

    if (callid == WEB_CALL) {
	    evt_send_to_local(ret_msg, param, 0, 0);
    }

    g_upgrade_ready_fail = 1;

    return 0;
}

static char **_get_mount_upgrade_file(char *path, int *ret_cnt)
{
	char **flist;
	FNAME_FILTER_PARAM_T filter_param;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	filter_param.condition = FF_NOFILTER;
	filter_param.opt = FO_DEFAULT;

	flist = ifn_new_filelist(path, NULL, &filter_param, ret_cnt);

	return flist;
}

static int _get_mac_address(char *buf)
{
	SysInfoData info;
	DAL_get_sysInfo_data(&info);
	strcpy(buf, info.macAddr);
	return 0;
}

static int _get_model_name(char *buf)
{
	SysInfoData info;
	DAL_get_sysInfo_data(&info);
	strcpy(buf, info.model);
	return 0;
}

static int _get_net_upgrade_filename(char *url, char *fullname, int len)
{
	char **flist;
	char mac[32];
	char model[32];
	char cmd[2048];
	int f_cnt = 0;

	if (ifn_make_dir_p(MOUNT_UPDATE_DIR) == -1) return -1;

	memset(mac, 0x00, sizeof(mac));
	memset(model, 0x00, sizeof(model));
	memset(cmd, 0x00, sizeof(cmd));

	_get_mac_address(mac);
	_get_model_name(model);
	sprintf(cmd, "/NFDVR/data/httpfs/httpfs2 -x \"%s;%s\" %s %s", mac, model, url, MOUNT_UPDATE_DIR);
	proxy_system(cmd, 1, 3);

	flist = _get_mount_upgrade_file(MOUNT_UPDATE_DIR, &f_cnt);

	if (!f_cnt) {
		ifn_remove_dir_rf(MOUNT_UPDATE_DIR);
		return -1;
	}

	memset(fullname, 0x00, len);
	sprintf(fullname, "%s/%s", MOUNT_UPDATE_DIR, flist[0]);

	ifn_free_filelist(flist);
	return 0;
}

static int _get_camera_mount_path(int ch, char *mount_path)
{
	g_sprintf(mount_path, "/mnt/upgrade_camfile_%d", ch);
	return 0;
}

static int _get_camera_net_upgrade_filename(int ch, char *url, char *fullname, int len)
{
	char **flist;
	char mnt_path[64];
	char mac[32];
	char model[32];
	char cmd[2048];
	int f_cnt = 0;

	memset(mnt_path, 0x00, sizeof(mnt_path));
	_get_camera_mount_path(ch, mnt_path);

	if (ifn_make_dir_p(mnt_path) == -1) return -1;

	memset(mac, 0x00, sizeof(mac));
	memset(model, 0x00, sizeof(model));
	memset(cmd, 0x00, sizeof(cmd));

	_get_mac_address(mac);
	_get_model_name(model);
	sprintf(cmd, "/NFDVR/data/httpfs/httpfs2 -x \"%s;%s\" %s %s", mac, model, url, mnt_path);
	proxy_system(cmd, 1, 3);

	flist = _get_mount_upgrade_file(mnt_path, &f_cnt);

	if (!f_cnt) {
		ifn_remove_dir_rf(mnt_path);
		return -1;
	}

	memset(fullname, 0x00, len);
	sprintf(fullname, "%s/%s", mnt_path, flist[0]);

	ifn_free_filelist(flist);
	return 0;
}

static int _record_fwup_time()
{
	time_t uptime = time(0);
	DAL_set_fwup_date(uptime);
	DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
	DMSG(9, "");
	return 0;
}

static gboolean _proc_fw_upgrade(gpointer data)
{
	TRANSACTION_E tra = (TRANSACTION_E)data;
	NF_FW_PRGT prgt;
	guint rate = 0;
	static guint prev_rate = 0;
	IMSG ret_msg;

	memset(&prgt, 0, sizeof(prgt));
	nf_fw_state_check(&prgt);
	DMSG(1, "TYPE[%d], STATUS[%d] ERROR[%d], CURRENT[%d] TOTAL[%d]\t", prgt.type, prgt.state, prgt.is_error, prgt.current, prgt.total);

	if (prgt.is_error)
	{
        iscm.fwup_step = -FWSTEP_128MB_PREPARE_COMPLETE;

		if(tra == TRA_NET_FW_UPGRADE)
		{
			_call_net_update_cbfunc(tra, NET_FW_ERROR, rate);
			ifn_remove_dir_rf(MOUNT_UPDATE_DIR);
		}

		ret_msg = iscm.chart[tra].ret_msg;
        _scm_return_api(iscm.chart[tra].caller, ret_msg, -1, 0, 0);

		prev_rate = 0;
		return FALSE;
	}

	switch (prgt.type)
	{
		case NF_FW_PRGT_TYPE_PRE: 			rate = 5; break;
		case NF_FW_PRGT_TYPE_UBOOT:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 5; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 5 + PARTIAL_RATE(0); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 5 + PARTIAL_RATE(5); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 50 + PARTIAL_RATE(2); break;
			}
	    }
	    break;

		case NF_FW_PRGT_TYPE_KERNEL:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 10; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 10 + PARTIAL_RATE(0); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 10 + PARTIAL_RATE(10); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 52 + PARTIAL_RATE(3); break;
			}
	    }
	    break;

/* not used
		case NF_FW_PRGT_TYPE_DSP:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 25; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 25 + PARTIAL_RATE(5); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 30 + PARTIAL_RATE(5); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 70 + PARTIAL_RATE(10); break;
			}
	    }
	    break;
*/

		case NF_FW_PRGT_TYPE_FS:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 20; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 20 + PARTIAL_RATE(0); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 20 + PARTIAL_RATE(25); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 55 + PARTIAL_RATE(40); break;
			}
		}
		break;

		case NF_FW_PRGT_TYPE_LOGO:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 45; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 45 + PARTIAL_RATE(2); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 47 + PARTIAL_RATE(2); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 95 + PARTIAL_RATE(2); break;
			}
		}
		break;

		case NF_FW_PRGT_TYPE_FPGA:
		{
			switch (prgt.state)
			{
				case NF_FW_PRGT_IMG_START: 		rate = 49; break;
				case NF_FW_PRGT_IMG_ERASE: 		rate = 49 + PARTIAL_RATE(1); break;
				case NF_FW_PRGT_IMG_WRITE: 		rate = 50 + PARTIAL_RATE(0); break;
				case NF_FW_PRGT_IMG_CRC_CHECK: 	rate = 97 + PARTIAL_RATE(2); break;
			}
		}
		break;

		case NF_FW_PRGT_TYPE_FINISH:
		{
			rate = 100;
            iscm.fwup_rate = rate;
            iscm.fwup_step = FWSTEP_128MB_REBOOT_TO_UBOOTUP;

			if(tra == TRA_NET_FW_UPGRADE)
			{
				_call_net_update_cbfunc(tra, NET_FW_FINISH, rate);
				ifn_remove_dir_rf(MOUNT_UPDATE_DIR);
			}

			ret_msg = iscm.chart[tra].ret_msg;
			_scm_send_msg_to_viewer(INFY_FWUP_RATE, rate);
			_scm_send_msg_to_viewer(ret_msg, 0);
			//_record_fwup_time();
			DMSG(1, "fw upgrade completed");
		}
		return FALSE;

		default:
		{
			DMSG(1, "==================================================");
			DMSG(1, "//////////////////////////////////////////////////");
			DMSG(1, "Unknown status of upgrading......(%d)", prgt.type);
			DMSG(1, "//////////////////////////////////////////////////");
			DMSG(1, "==================================================");
		}
		return FALSE;
	}

	// rate error, work-around
	if (prev_rate > rate) rate = prev_rate;
    iscm.fwup_rate = rate;

	if(tra == TRA_NET_FW_UPGRADE)
	{
		_call_net_update_cbfunc(tra, NET_FW_UPDATE, rate);
	}

	_scm_send_msg_to_viewer(INFY_FWUP_RATE, rate);

	prev_rate = rate;
	return TRUE;
}

static gboolean _proc_ipcam_fw_upgrade(gpointer data)
{
    TRANSACTION_E tra = (TRANSACTION_E)data;
    CAMFW_UPGD_T *upgd;
    NFIPCamUpgradeState upst;
    char mnt_path[64];
    int i;
    static guint err_no[GUI_CHANNEL_CNT] = {0,};


    memset(&upst, 0x00, sizeof(upst));
    upgd = (CAMFW_UPGD_T*)iscm.chart[tra].alloc_data;

    for (i = 0; i < var_get_ch_count(); i++)
    {
        if (upgd->upmask & (1 << i))
        {
            nf_ipcam_get_upgrade_state(i, &upst);
            err_no[i] = upst.error_no;

            if (upst.is_error == 0)
            {
                switch (upst.state)
                {
                    case NF_IPCAM_FW_PRE:
                    case NF_IPCAM_FW_CHK_VER:
                    case NF_IPCAM_FW_FWMODE_REBOOT:
                    case NF_IPCAM_FW_SET_FWMODE:
                    case NF_IPCAM_FW_UPLOAD:
                    case NF_IPCAM_FW_WRITE:
                    case NF_IPCAM_FW_FINAL_REBOOT:
                    {
                        if (~upgd->cmpl_mask & (1 << i))
                        {
                            evt_send_to_local(INFY_IPCAM_FWUP_RATE, i, 0, GUINT_TO_POINTER(upst.cur_progress));
                        }
                    }
                    break;
                    case NF_IPCAM_FW_UP_COMPLETE:
                    {
                        if (~upgd->cmpl_mask & (1 << i))
                        {
                            DMSG(1, "upgrade complete success - ch:%d", i);
                            evt_send_to_local(INFY_IPCAM_FWUP_CMPL, i, 0, 0);
                            upgd->cmpl_mask |= (1 << i);
                        }
                    }
                    break;
                }
            }
//            else if ((upst.is_error == 1) && (upst.error_no == NF_IPCAM_FW_ERR_OLD_SW_VER))
            else if (upst.is_error == 1)
            {
                if (~upgd->cmpl_mask & (1 << i))
                {
//                    DMSG(1, "###yanggungg : upgrade complete version error - ch:%d, err_no : %d", i, err_no[i]);
                    evt_send_to_local(INFY_IPCAM_FWUP_ERROR_VERSION, i, 0, err_no);
                    upgd->cmpl_mask |= (1 << i);
                }
            }
            else
            {
                if (~upgd->cmpl_mask & (1 << i))
                {
                    DMSG(1, "upgrade complete error - ch:%d, err_no : %d", i, err_no[i]);
                    evt_send_to_local(INFY_IPCAM_FWUP_ERROR, i, 0, err_no);
                    upgd->cmpl_mask |= (1 << i);
                }
            }
        }
    }

    if ((upgd->step >= var_get_ch_count()) && (upgd->cmpl_mask == upgd->upmask))
    {
        if (tra == TRA_NET_CAMFW_UPGRADE)
        {
            for (i = 0; i < var_get_ch_count(); i++)
            {
                if (upgd->upmask & (1 << i))
                {
                    memset(mnt_path, 0x00, sizeof(mnt_path));
                    _get_camera_mount_path(i, mnt_path);
                    ifn_remove_dir_rf(mnt_path);
                }
            }
        }

        if (iscm.wrk_camfw_upgd)
        {
            wrk_stop(iscm.wrk_camfw_upgd);
            wrk_wait_for_stop(iscm.wrk_camfw_upgd);
        }

        //nf_ipcam_start();

        DMSG(1, "upgrade end");
        return FALSE;
    }

    return TRUE;
}

static guint _get_same_fwfile_mask(int ch, CAMFW_UPGD_T *upgd)
{
	guint retMsk = 0;
	int i;

	for (i = 0; i < var_get_ch_count(); i++)
	{
		if (upgd->upmask & (1 << i))
		{
			if (strcmp(upgd->path[ch], upgd->path[i]) == 0)
			{
				retMsk |= (1 << i);
			}
		}
	}

	return retMsk;
}

static int _proc_start_camfw_upgrade(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	CMMACK_T cmmack;
	TRANSACTION_E tra;
	CAMFW_UPGD_T *upgd;
	NFIPCamUpgradeState upst;
	guint chmask = 0;

	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	upgd = (CAMFW_UPGD_T*)piscm->chart[tra].alloc_data;

	if (upgd->step >= var_get_ch_count()) return -1;

	if (upgd->upmask & (1 << upgd->step))
	{
		if (upgd->start_mask & (1 << upgd->step))
		{
			DMSG(1, "upgrade start channel : %d", upgd->step);

			chmask = _get_same_fwfile_mask(upgd->step, upgd);
			DMSG(1, "upgrade channel mask: %08X", chmask);

			nf_ipcam_fw_upgrade(chmask, upgd->path[upgd->step], 1);
			upgd->start_mask &= ~chmask;
			return 0;
		}

		chmask = _get_same_fwfile_mask(upgd->step, upgd);

		if ((upgd->cmpl_mask & chmask) == chmask)
		{
			DMSG(1, "cmpl channel : %d", upgd->step);
			upgd->step++;
		}
	}
	else
	{
		DMSG(1, "not select channel : %d", upgd->step);
		upgd->step++;
	}

	return 0;
}

static int _start_fw_upgrade(TRANSACTION_E tra, const char *full_file_name)
{
	DMSG(1, "file name = [%s]\n", full_file_name);
	if (!nf_fw_upgrade_thread_start(full_file_name, 1)) return -1;

	DAL_set_fwup_state(-1);
    DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
	_scm_add_timeout(&iscm, 300, _proc_fw_upgrade, (gpointer)tra);
	return 0;
}

static int _start_design_update(TRANSACTION_E tra, const char *dir_name, int vendor)
{
	char cmd[256];

	sprintf(cmd, "cp %s/itx_design/*.png /NFDVR/data/gui/bmp/OTM/4D1 -dpR", dir_name);
	proxy_system(cmd, 1, 3);

	sprintf(cmd, "cp %s/itx_design/_color.txt /NFDVR/data/gui/cfg -dpR", dir_name);
	proxy_system(cmd, 1, 3);

	sprintf(cmd, "sh /NFDVR/data/gui/cfg/make_color_tbl.sh /NFDVR/data/gui/cfg/_color.txt %d >& %s/itx_design/_error.txt", vendor, dir_name);
	proxy_system(cmd, 1, 3);

	sprintf(cmd, "mv ./color.cfg.%d /NFDVR/data/gui/color.cfg", vendor);
	proxy_system(cmd, 1, 3);

	proxy_system("/bin/sync", 1, 3);
	umount(dir_name);

	sleep(2);

	scm_reboot_system(RR_NA, 3000);
	return 0;
}

static int _start_camfw_upgrade(TRANSACTION_E tra)
{
	DMSG(1, "start camera fw upgrade");
	_scm_add_timeout(&iscm, 300, _proc_ipcam_fw_upgrade, (gpointer)tra);
	return 0;
}

static int _upgrade_fw_by_net(SCM_T *piscm, TRANSACTION_E tra, const char *fullname)
{
	IMSG ret_msg = piscm->chart[tra].ret_msg;

	DMSG(1, "fullname:%s", fullname);

	piscm->chart[tra].alloc_data = imalloc(strlen(fullname) + 1);
	strcpy(piscm->chart[tra].alloc_data, fullname);

	// invalid board scenario for IPXP
	scm_cancel_reboot();

	_scm_put_log_with_tra(piscm, FW_UPGRADE, 0, 0, tra);
	_scm_work_record_stop(piscm, tra);
	return 0;
}

static int _upgrade_camfw_by_net(SCM_T *piscm, TRANSACTION_E tra, CAMFW_UPGD_T *upgd)
{
	IMSG ret_msg = piscm->chart[tra].ret_msg;

	piscm->chart[tra].alloc_data = imalloc(sizeof(CAMFW_UPGD_T));
	memcpy(piscm->chart[tra].alloc_data, upgd, sizeof(CAMFW_UPGD_T));
	_scm_put_message(iNFY_CAM_FWUP, 0, 0, (void *)tra);
	return 0;
}

static int _scm_cleanup_camfw_upgrade(SCM_T *piscm)
{
	wrk_destroy_worker(piscm->wrk_camfw_upgd);
	piscm->wrk_camfw_upgd = 0;
	return 0;
}

static int _proc_prepare_upgrade_validate_check(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	char *full_name;
	int ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
    full_name = (char*)piscm->chart[tra].alloc_data;

    g_message("%s, %d, fwfile:%s", __FUNCTION__, __LINE__, full_name);

#if defined(CONFIG_FWUPGRADE_SINGLE)
    ret = nf_fw_file_validate_check(full_name);
    g_message("%s, %d, nf_fw_file_validate_check:%d", __FUNCTION__, __LINE__, ret);
#endif

	return (ret == 1 ? 0 : -1);
}

static int _proc_prepare_upgrade_validate_check_url(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	NF_FW_HTTPFS_CLIENT_REQ req;
	char *url;
	int ret = 1;

	DMSG(9, "");
    sleep(2);
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	if (piscm->chart[tra].alloc_data) {
    url = (char*)piscm->chart[tra].alloc_data;
    }
    else {
        DMSG(9, "");
        return 0;
    }

    g_message("%s, %d, url:%s", __FUNCTION__, __LINE__, url);

	memset(&req, 0x00, sizeof(NF_FW_HTTPFS_CLIENT_REQ));
	strncpy(req.url, url, sizeof(req.url));
    req.timeout_connect_sec = 30;
	ret = nf_fw_httpfs_url_check_start(&req);

    g_message("%s, %d, nf_fw_file_validate_check:%d", __FUNCTION__, __LINE__, ret);

	return (ret == 1 ? 0 : -1);
}

static int _proc_prepare_upgrade_data_backup(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	char *mnt_path;
	int ret = 1;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	mnt_path = (char*)piscm->chart[tra].alloc_data;

    g_message("%s, %d, path:%s", __FUNCTION__, __LINE__, mnt_path);

#if defined(CONFIG_FWUPGRADE_SINGLE)
    proxy_system("/bin/sync", 1, 3);
    sleep(1);
    ret = nf_fw_data_backup(mnt_path);
    g_message("%s, %d, nf_fw_data_backup:%d", __FUNCTION__, __LINE__, ret);
#endif

	return (ret == 1 ? 0 : -1);
}



////////////////////////////////////////////////////////////
//
// protected interfaces
//


HANDLER int _scm_work_fw_upgrade(SCM_T *piscm, TRANSACTION_E tra)
{
	_start_fw_upgrade(tra, FW_FILENAME);
	return 0;
}

HANDLER int _scm_on_fw_up(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	int ret = pmsg->param;
	_scm_finalize_tra(piscm, tra, ret);
	return 0;
}

HANDLER int _scm_work_design_update(SCM_T *piscm, TRANSACTION_E tra)
{
	DMSG(1, "%s", DIRNAME);
	DMSG(1, "%d", VENDOR);
	_start_design_update(tra, DIRNAME, VENDOR);
	return 0;
}

HANDLER int _scm_on_design_up(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	int ret = pmsg->param;
	_scm_finalize_tra(piscm, tra, ret);
	return 0;
}

HANDLER int _scm_work_camfw_upgrade(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAMFW_UPGD, (void *)tra };
	if (piscm->wrk_camfw_upgd) return 0;

	piscm->wrk_camfw_upgd = wrk_create_worker(_proc_start_camfw_upgrade, &cmmack);
	wrk_run_loop(piscm->wrk_camfw_upgd, IMSG_NONE, piscm, 0, 0);

	_start_camfw_upgrade(tra);
	return 0;
}

HANDLER int _scm_on_camfw_up(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int ret = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	_scm_finalize_tra(piscm, tra, ret);
	_scm_cleanup_camfw_upgrade(piscm);
	return 0;
}

HANDLER int _scm_do_ready_net_fw_upgrade(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	NET_UPDATE_DATA *netfw_data;
	char fullname[256];
	int ret;

	if (tra == TRA_NONE) return -1;

	_call_net_update_cbfunc(tra, pmsg->param, 0);

	if (pmsg->param == NET_FW_REJECT)
	{
		_scm_finalize_tra(piscm, tra, 0);
		return 0;
	}

	netfw_data = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;
	DMSG(1, "url:%s", netfw_data->url);

	ret = _get_net_upgrade_filename(netfw_data->url, fullname, sizeof(fullname));

	if (ret == -1)
	{
		_call_net_update_cbfunc(tra, NET_FW_ERROR, 0);
        _error_ready_to_upgrade(tra, -1);
		_scm_finalize_tra(piscm, tra, -1);
		return -1;
	}

	_upgrade_fw_by_net(piscm, tra, fullname);
	return 0;
}

HANDLER int _scm_do_ready_net_128MB_fw_upgrade(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	NET_UPDATE_DATA *netfw_data;
	char fullname[256];
	int ret;

    DMSG(1, "tra:%d", tra);
    
	_call_net_update_cbfunc(tra, NET_FW_APPLY, 0);

	netfw_data = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;
	DMSG(1, "url:%s", netfw_data->url);

	_upgrade_fw_by_net(piscm, tra, netfw_data->url);
	return 0;
}

HANDLER int _scm_do_prepare_fwup_validate_check_url(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_PREPARE_FWUP, (void *)tra };
	
    DMSG(9, "");
    if (tra == TRA_NONE) return -1;

    if (pmsg->param == NET_FW_REJECT) {
        _scm_finalize_tra(piscm, tra, 0);
        return 0;
    }

    DMSG(9, "");
    iscm.fwup_step = FWSTEP_128MB_PREPARE_VALIDATE;
	iscm.wrk_fwup_validate = wrk_create_worker(_proc_prepare_upgrade_validate_check_url, &cmmack);
	wrk_run_once_param(iscm.wrk_fwup_validate, IMSG_NONE, &iscm, 0, 0);

	return 0;
}

HANDLER int _scm_do_prepare_fwup_data_backup(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_PREPARE_FWUP, (void *)tra };

    DMSG(9, "");
    if (tra == TRA_NONE) return -1;

    if (pmsg->param == NET_FW_REJECT) {
        _scm_finalize_tra(piscm, tra, 0);
        return 0;
    }

    DMSG(9, "");
	iscm.wrk_fwup_backup = wrk_create_worker(_proc_prepare_upgrade_data_backup, &cmmack);
	wrk_run_once_param(iscm.wrk_fwup_backup, IMSG_NONE, &iscm, 0, 0);

	return 0;
}

HANDLER int _scm_do_ready_s1_remotefw_upgrade(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	NET_UPDATE_DATA *netfw_data;
	char fullname[256];
	int ret;

	if (tra == TRA_NONE) return -1;

	_call_net_update_cbfunc(tra, pmsg->param, 0);

	if (pmsg->param == NET_FW_REJECT)
	{
		_scm_finalize_tra(piscm, tra, 0);
		return 0;
	}

	netfw_data = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;
	DMSG(1, "url:%s", netfw_data->url);

	ret = _get_net_upgrade_filename(netfw_data->url, fullname, sizeof(fullname));

	if (ret == -1)
	{
		_call_net_update_cbfunc(tra, NET_FW_ERROR, 0);
		_scm_finalize_tra(piscm, tra, -1);
		return -1;
	}

	_upgrade_fw_by_net(piscm, tra, fullname);
	return 0;
}

HANDLER int _scm_do_ready_s1_remotecamfw_upgrade(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	TRANSACTION_E tra = _scm_untrack_tra(piscm, pmsg->msgid);
	NET_UPDATE_DATA *netfw_data;
	CAMFW_UPGD_T *upgd;
	BITMASK chmask = GPOINTER_TO_UINT(pmsg->data);
	int i, ret;

	if (tra == TRA_NONE) return -1;

	if ((pmsg->param == NET_FW_REJECT) || (chmask == 0))
	{
		_scm_finalize_tra(piscm, tra, 0);
		return 0;
	}

	netfw_data = (NET_UPDATE_DATA*)piscm->chart[tra].void_data;

	upgd = imalloc(sizeof(CAMFW_UPGD_T));

	upgd->step = 0;

	for (i = 0; i < var_get_ch_count(); i++)
	{
		if (chmask & (1 << i))
		{
			ret = _get_camera_net_upgrade_filename(i, netfw_data[i].url, upgd->path[i], 256);
			if (ret == -1) chmask &= ~(1 << i);
		}
	}

	if (chmask == 0)
	{
		_scm_finalize_tra(piscm, tra, 0);
		ifree(upgd);
		return -1;
	}

	upgd->upmask = chmask;
	upgd->start_mask = chmask;

	_upgrade_camfw_by_net(piscm, tra, upgd);
	ifree(upgd);
	return 0;
}

HANDLER int _scm_check_fw_state(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	SMT_SERVICE_E st;
	TRANSACTION_E tra = (TRANSACTION_E)pmsg->data;

	st = smt_get_service();

	if( (st!= SMT_LIVE) && (st != SMT_LOGOUT))
	{
		_call_net_update_cbfunc(tra, NET_FW_NOTSUPPORT, 0);
		_scm_finalize_tra(piscm, tra, -1);
		return 0;
	}

	_scm_track_tra(piscm, IRPL_NET_FWUP, tra);
	_call_net_update_cbfunc(tra, NET_FW_REQ_UPDATE, 0);
	_scm_send_msg_to_viewer(IREQ_NET_FWUP, 0);
	return 0;
}

HANDLER int _scm_on_prepare_fwup_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	if (piscm->chart[tra].caller == WEB_CALL) {
		cmm_send_message(CMMPT_SCM, piscm->chart[tra].ret_msg, result, 0, 0);
	}

    if (tra == TRA_FWUP_VALIDATE)
    {
        DMSG(9, "%d", result);
        if (result == 0)
        {
            iscm.fwup_step = FWSTEP_128MB_PREPARE_DATABACKUP;
        	if (piscm->chart[tra].char_data == 1) {
                _scm_track_tra(&iscm, IRPL_SCM_PREPARE_FWUP_IMAGESAVE, TRA_FWUP_DATABACKUP);
                cmm_send_message(CMMPT_SCM, IREQ_SCM_PREPARE_FWUP_IMAGESAVE, 0, 0, 0);
            }
        }
        else
        {
            iscm.fwup_step = -FWSTEP_128MB_PREPARE_VALIDATE;
        }
    }
    else if (tra == TRA_FWUP_DATABACKUP)
    {
        DMSG(9, "%d", result);
        if (result == 0)
        {
            iscm.fwup_step = FWSTEP_128MB_PREPARE_COMPLETE;
        	if (piscm->chart[tra].char_data == 1) {
                // upgrade type - mongoose web update
                _scm_track_tra(&iscm, iNFY_NET_FWUP, TRA_NET_FW_UPGRADE);
                _scm_put_message(iNFY_NET_FWUP, 0, 0, 0);
            }
            /* This is not used in branches that is not a S1.
            else if (piscm->chart[tra].char_data == 2) {
                // upgrade type - remote server update
                _scm_track_tra(&iscm, IRPL_S1_DETECTED_NEWFW, TRA_S1_REMOTE_FW_UPGRADE);
                _scm_put_message(IRPL_S1_DETECTED_NEWFW, 0, 0, 0);
            }
            */
        }
        else
        {
            iscm.fwup_step = -FWSTEP_128MB_PREPARE_DATABACKUP;
        }
    }

	_scm_finalize_tra(piscm, tra, result);

    if (tra == TRA_FWUP_VALIDATE) {
    	wrk_destroy_worker(piscm->wrk_fwup_validate);
    	piscm->wrk_fwup_validate = 0;	
    }
    else if (tra == TRA_FWUP_DATABACKUP) {
    	wrk_destroy_worker(piscm->wrk_fwup_backup);
    	piscm->wrk_fwup_backup = 0;	
    }	

	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_upgrade_fw(const char *filename, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FW_UPGRADE;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(strlen(filename) + 1);
	strcpy(iscm.chart[tra].alloc_data, filename);

	// invalid board scenario for IPXP
	scm_cancel_reboot();

	scm_put_log(FW_UPGRADE, 0, 0);
	_scm_work_service_stop(&iscm, tra, RS_FWUP);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_update_design(const char *dirname, int vendor, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DESIGN_UP;
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = vendor;
	iscm.chart[tra].alloc_data = imalloc(strlen(dirname) + 1);
	strcpy(iscm.chart[tra].alloc_data, dirname);

	DMSG(1, "");
	_scm_work_service_stop(&iscm, tra, RS_FWUP);
	// subsequent operation will be run as ret_msg is arrived.
	return 0;
}

int scm_upgrade_ipcam_fw(guint ch_mask, const char *filename, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_CAMFW_UPGRADE;
	CAMFW_UPGD_T *upgd = imalloc(sizeof(CAMFW_UPGD_T));
	int i;

	upgd->step = 0;
	upgd->upmask = ch_mask;
	upgd->start_mask = ch_mask;

	for (i = 0; i < var_get_ch_count(); i++)
	{
		if (ch_mask & (1 << i))
		{
			strcpy(upgd->path[i], filename);
		}
	}

	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(CAMFW_UPGD_T));
	memcpy(iscm.chart[tra].alloc_data, upgd, sizeof(CAMFW_UPGD_T));
	_scm_put_message(iNFY_CAM_FWUP, 0, 0, (void *)tra);
	return 0;
}

int scm_upgrade_fw_network(const char *update_url, const char *filename, get_update_state update_state, gpointer data)
{
	NET_UPDATE_DATA netfw_data;
	TRANSACTION_E tra = TRA_NET_FW_UPGRADE;

	memset(&netfw_data, 0x00, sizeof(NET_UPDATE_DATA));

	strcpy(&netfw_data.url, update_url);
//	strcpy(&netfw_data.filename, filename);
	netfw_data.cb_func = update_state;
	netfw_data.cb_data = data;

#if defined(CONFIG_FWUPGRADE_SINGLE)
	_scm_ready_tra(&iscm, tra, IRET_SCM_PREPARE_FWUP_CMPL, IUX_CALLER());
#else
	_scm_ready_tra(&iscm, tra, IRET_SCM_UPGRADE_FW, IUX_CALLER());
#endif

	iscm.chart[tra].void_data = imalloc(sizeof(NET_UPDATE_DATA));
	memcpy(iscm.chart[tra].void_data, &netfw_data, sizeof(NET_UPDATE_DATA));
	_scm_track_tra(&iscm, iNFY_NET_FWUP, tra);

	_scm_put_message(iNFY_NET_FWUP, 0, 0, (void *)tra);
	return 0;
}

int scm_reboot_by_network()
{
	scm_reboot_system(RR_FWUP, 5000);
	return 0;
}

int scm_check_remote_new_nvrfw()
{
	NF_FW_NETWORK_VERINFO gInfo;
	DETECT_VERINFO_T *sInfo;
	NET_UPDATE_DATA netfw_data;

	TRANSACTION_E tra = TRA_NET_FW_UPGRADE;
	char fwver[40];

	DMSG(1, "");

	if (!nf_fw_network_upgrade_check(&gInfo)) return -1;

	var_get_detect_fwver(fwver, 40);
	if (strcmp(fwver, gInfo.reserved1) == 0) return -1;

	DMSG(1, "tmpfw_ver:%s", fwver);
	DMSG(1, "newfw_ver:%s", gInfo.reserved1);
	DMSG(1, "url:%s", gInfo.fw_file);

#if defined(CONFIG_FWUPGRADE_SINGLE)
	sInfo = imalloc(sizeof(DETECT_VERINFO_T));
	sInfo->need = 1;
	if (gInfo.reserved1) strcpy(sInfo->new_fwver, gInfo.reserved1);
	if (gInfo.is_urgent) strcpy(sInfo->importance, gInfo.is_urgent);
	if (gInfo.fw_file) strcpy(sInfo->url, gInfo.fw_file);

	var_set_detect_fwver(sInfo->new_fwver);

	_scm_send_pointer_to_viewer(INFY_DETECTED_NEWFW, 0, (void*)sInfo, sizeof(DETECT_VERINFO_T));
#else
	memset(&netfw_data, 0x00, sizeof(NET_UPDATE_DATA));
	strcpy(&netfw_data.url, gInfo.fw_file);
	netfw_data.cb_func = 0;
	netfw_data.cb_data = 0;

	_scm_ready_tra(&iscm, tra, IRET_SCM_UPGRADE_FW, IUX_CALLER());
	iscm.chart[tra].void_data = imalloc(sizeof(NET_UPDATE_DATA));
	memcpy(iscm.chart[tra].void_data, &netfw_data, sizeof(NET_UPDATE_DATA));

	sInfo = imalloc(sizeof(DETECT_VERINFO_T));
	sInfo->need = 1;
	if (gInfo.reserved1) strcpy(sInfo->new_fwver, gInfo.reserved1);
	if (gInfo.is_urgent) strcpy(sInfo->importance, gInfo.is_urgent);

	var_set_detect_fwver(sInfo->new_fwver);

	_scm_track_tra(&iscm, IRPL_DETECTED_NEWFW, tra);
	_scm_send_pointer_to_viewer(IREQ_DETECTED_NEWFW, 0, (void*)sInfo, sizeof(DETECT_VERINFO_T));
#endif

	return 0;
}

int scm_S1_update_remotefw_info()
{
	int retVal;

	if (nf_fw_network_s1_get_update_state() == TRUE) return -1;

	nf_fw_network_s1_set_update_state(TRUE);
	retVal = nf_fw_network_s1_update_profile();
	nf_fw_network_s1_set_update_state(FALSE);

	return retVal;
}

int scm_S1_check_remote_new_nvrfw()
{
	NF_FW_NETWORK_S1_INFO gInfo;
	DETECT_VERINFO_T *sInfo;
	NET_UPDATE_DATA *netfw_data;

	TRANSACTION_E tra = TRA_NET_FW_UPGRADE;
	char fwver[40];
	int ret;

	if (nf_fw_network_s1_get_update_state() == TRUE) return -1;

	nf_fw_network_s1_set_update_state(TRUE);

	ret = nf_fw_network_s1_get_nvr_fw_info(&gInfo);

	nf_fw_network_s1_set_update_state(FALSE);

	if (!ret) return -1;
	if (!gInfo.need) return -1;

	var_get_detect_fwver(fwver, 40);
	if (strcmp(fwver, gInfo.new_ver) == 0) return -1;

	DMSG(1, "newfw_ver:%s", gInfo.new_ver);
	DMSG(1, "URL:%s", gInfo.url);

	netfw_data = imalloc(sizeof(NET_UPDATE_DATA));
	strcpy(netfw_data->url, gInfo.url);
	netfw_data->cb_func = 0;
	netfw_data->cb_data = 0;

	sInfo = imalloc(sizeof(DETECT_VERINFO_T));
	sInfo->need = gInfo.need;
	strcpy(sInfo->model, gInfo.model);
	strcpy(sInfo->new_fwver, gInfo.new_ver);
	strcpy(sInfo->importance, gInfo.level);
	strcpy(sInfo->infor_general, gInfo.general);
	strcpy(sInfo->infor_fix, gInfo.fixes);
	strcpy(sInfo->infor_func, gInfo.updates);
	strcpy(sInfo->reference_link, gInfo.link);

	var_set_detect_fwver(sInfo->new_fwver);

	_scm_ready_tra(&iscm, tra, IRET_SCM_UPGRADE_FW, IUX_CALLER());
	iscm.chart[tra].void_data = imalloc(sizeof(NET_UPDATE_DATA));
	memcpy(iscm.chart[tra].void_data, netfw_data, sizeof(NET_UPDATE_DATA));

	_scm_track_tra(&iscm, IRPL_S1_DETECTED_NEWFW, tra);
	_scm_send_pointer_to_viewer(IREQ_S1_DETECTED_NEWFW, 0, (void*)sInfo, sizeof(DETECT_VERINFO_T));

	ifree(netfw_data);
	return 0;
}

int scm_S1_check_remote_new_camfw()
{
	NF_FW_NETWORK_S1_INFO gInfo[16];
	DETECT_VERINFO_T *sInfo;
	NET_UPDATE_DATA *netfw_data;

	TRANSACTION_E tra = TRA_NET_CAMFW_UPGRADE;
	char fwver[40];
	BITMASK nd_msk = 0;
	int i, ret;
	int max_ch = var_get_ch_count();

	if (nf_fw_network_s1_get_update_state() == TRUE) return -1;

	nf_fw_network_s1_set_update_state(TRUE);

	for (i = 0; i < max_ch; i++)
	{
		memset(&gInfo[i], 0x00, sizeof(NF_FW_NETWORK_S1_INFO));

		if (nf_fw_network_s1_get_cam_fw_info(i, &gInfo[i]))
		{
			if (gInfo[i].need)
			{
				nd_msk |= (1 << i);
			}
		}
	}

	nf_fw_network_s1_set_update_state(FALSE);

	if (!nd_msk) return -1;

	for (i = 0; i < max_ch; i++)
	{
		if (nd_msk & (1 << i))
		{
			var_get_detect_cam_fwver(i, fwver, 40);
			if (strcmp(fwver, gInfo[i].new_ver) == 0)
			{
				nd_msk &= ~(1 << i);
			}
		}
	}


	if (!nd_msk) return -1;

	netfw_data = imalloc(sizeof(NET_UPDATE_DATA)*max_ch);
	sInfo = imalloc(sizeof(DETECT_VERINFO_T)*max_ch);

	for (i = 0; i < max_ch; i++)
	{
		if (nd_msk & (1 << i))
		{
			DMSG(1, "ch:%d, newfw_ver:%s", i, gInfo[i].new_ver);
			DMSG(1, "ch:%d, URL:%s", i, gInfo[i].url);

			strcpy(netfw_data[i].url, gInfo[i].url);
			netfw_data[i].cb_func = 0;
			netfw_data[i].cb_data = 0;

			sInfo[i].need = gInfo[i].need;
			strcpy(sInfo[i].model, gInfo[i].model);
			strcpy(sInfo[i].new_fwver, gInfo[i].new_ver);
			strcpy(sInfo[i].importance, gInfo[i].level);
			strcpy(sInfo[i].infor_general, gInfo[i].general);
			strcpy(sInfo[i].infor_fix, gInfo[i].fixes);
			strcpy(sInfo[i].infor_func, gInfo[i].updates);
			strcpy(sInfo[i].reference_link, gInfo[i].link);

			var_set_detect_cam_fwver(i, sInfo[i].new_fwver);
		}
	}

	_scm_ready_tra(&iscm, tra, IRET_SCM_UPGRADE_IPCAM_FW, IUX_CALLER());
	iscm.chart[tra].void_data = imalloc(sizeof(NET_UPDATE_DATA)*max_ch);
	memcpy(iscm.chart[tra].void_data, netfw_data, sizeof(NET_UPDATE_DATA)*max_ch);

	_scm_track_tra(&iscm, IRPL_S1_DETECTED_NEWCAMFW, tra);
	_scm_send_pointer_to_viewer(IREQ_S1_DETECTED_NEWCAMFW, 0, (void*)sInfo, sizeof(DETECT_VERINFO_T)*max_ch);

	ifree(netfw_data);
	return 0;
}

int scm_prepare_upgrade_validate_check(const char *full_name, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FWUP_VALIDATE;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_PREPARE_FWUP, (void *)tra };

	if (iscm.wrk_fwup_validate) {
	    DMSG(9, "");
	    _scm_return_api(IUX_CALLER(), ret_msg, -1, 0, 0);
	    return -1;
    }

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(strlen(full_name) + 1);
	strcpy(iscm.chart[tra].alloc_data, full_name);
    iscm.chart[tra].char_data = 0;

    iscm.fwup_step = FWSTEP_128MB_PREPARE_VALIDATE;
	iscm.wrk_fwup_validate = wrk_create_worker(_proc_prepare_upgrade_validate_check, &cmmack);
	wrk_run_once_param(iscm.wrk_fwup_validate, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_prepare_upgrade_validate_check_url(const char *url, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FWUP_VALIDATE;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_PREPARE_FWUP, (void *)tra };

	if (iscm.wrk_fwup_validate) {
	    DMSG(9, "");
	    _scm_return_api(IUX_CALLER(), ret_msg, -1, 0, 0);
	    return -1;
    }

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	if (url)
	{
	iscm.chart[tra].alloc_data = imalloc(strlen(url) + 1);
	strcpy(iscm.chart[tra].alloc_data, url);
    }
    iscm.chart[tra].char_data = 0;

    iscm.fwup_step = FWSTEP_128MB_PREPARE_VALIDATE;
	iscm.wrk_fwup_validate = wrk_create_worker(_proc_prepare_upgrade_validate_check_url, &cmmack);
	wrk_run_once_param(iscm.wrk_fwup_validate, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_prepare_upgrade_data_backup(const gchar *mnt_path, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_FWUP_DATABACKUP;

	if (iscm.wrk_fwup_backup) {
	    DMSG(9, "");
	    _scm_return_api(IUX_CALLER(), ret_msg, -1, 0, 0);
	    return -1;
    }

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(strlen(mnt_path) + 1);
	strcpy(iscm.chart[tra].alloc_data, mnt_path);
    iscm.chart[tra].char_data = 0; // use allinone type

    iscm.fwup_step = FWSTEP_128MB_PREPARE_DATABACKUP;
	_scm_track_tra(&iscm, IRPL_SCM_PREPARE_FWUP_IMAGESAVE, tra);
	cmm_send_message(CMMPT_SCM, IREQ_SCM_PREPARE_FWUP_IMAGESAVE, 0, 0, 0);

	return 0;
}

int scm_prepare_web_128MB_upgrade_allinone(const char *url)
{
	TRANSACTION_E tra = TRA_NONE;
    NET_UPDATE_DATA netfw_data;

    if (iscm.wrk_fwup_validate || iscm.wrk_fwup_backup) {
		DMSG(9, "");
	    return -1;
    }

    if (!url) {
		DMSG(9, "");
	    return -1;
    }

    DMSG(1, "update_url : %s", url);

    tra = TRA_FWUP_VALIDATE;

    _scm_ready_tra(&iscm, tra, IRET_SCM_PREPARE_FWUP_VALIDATE, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(strlen(url) + 1);
	strcpy(iscm.chart[tra].alloc_data, url);
    iscm.chart[tra].char_data = 1; // use allinone type
    _scm_track_tra(&iscm, IRPL_SCM_128MB_REMOTE_UPDATE_ALLINONE, tra);

    tra = TRA_FWUP_DATABACKUP;

    _scm_ready_tra(&iscm, tra, IRET_SCM_PREPARE_FWUP_DATABACKUP, IUX_CALLER());
    iscm.chart[tra].alloc_data = imalloc(strlen("http://") + 1);
	strcpy(iscm.chart[tra].alloc_data, "http://");
    iscm.chart[tra].char_data = 1; // use allinone type

	tra = TRA_NET_FW_UPGRADE;

	memset(&netfw_data, 0x00, sizeof(NET_UPDATE_DATA));
	strcpy(&netfw_data.url, url);

	_scm_ready_tra(&iscm, tra, IRET_SCM_PREPARE_FWUP_CMPL, IUX_CALLER());
	iscm.chart[tra].void_data = imalloc(sizeof(NET_UPDATE_DATA));
	memcpy(iscm.chart[tra].void_data, &netfw_data, sizeof(NET_UPDATE_DATA));

    iscm.fwup_step = FWSTEP_128MB_PREPARE_REQ_FWUP;

	return 0;
}

int scm_get_fw_upgrade_state(int *state, int *rate)
{
    NF_FW_PRGT prgt;

    memset(&prgt, 0, sizeof(prgt));
    nf_fw_state_check(&prgt);

    if (g_upgrade_ready_fail) return -1;
    if (prgt.is_error)  return -1;
    if (prgt.type == NF_FW_PRGT_TYPE_FINISH) {
        if (prgt.state == NF_FW_PRGT_FNI_UPDATE_FNISH) return 1;
    }

    if (state) *state = prgt.type;
    if (rate) *rate = iscm.fwup_rate;

	return 0;
}

int scm_get_fw_upgrade_rate()
{
	return iscm.fwup_rate;
}

int scm_get_128MB_fw_upgrate_step()
{
	return iscm.fwup_step;
}

