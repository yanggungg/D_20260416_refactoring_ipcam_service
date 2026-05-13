/*
 * scm_internal.h
 * 	- scm internal header file
 * 	- not exposed to outside
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 1, 2011
 *
 */

#ifndef __SCM_INTERNAL_H
#define __SCM_INTERNAL_H

#include <glib.h>
#include "iux_afx.h"
#include "cmm.h"
#include "mda.h"
#include "log.h"
#include "nf_api_disk.h"
#include "wrk.h"
#include "scm.h"
#include "tmr.h"
#include "spt.h"


#define MAX_NAME_LEN	IUX_MAX_NAME_LEN
#define MAX_PATH_LEN	IUX_MAX_PATH_LEN
#define MAX_TTR			16


////////////////////////////////////////////////////////////
//
// protected data type
//

typedef enum _RESULT_E {
	FAIL		= -1,
	SUCCESS		= 0,
} RESULT_E;

typedef enum _BOOT_MODE_E {
	BM_NORMAL			= 0,
	BM_NODISK			= 1,
	BM_IGNDISK			= 2,
	BM_FORMAT			= 3,
	BM_ENFORCE_FORMAT	= 4,
} BOOT_MODE_E;

typedef enum _BERR_TYPE_E {
	ET_DISK		= NAND_ERR_DISK_ERROR,
	ET_SMART	= NAND_ERR_SMART_ERROR,
	ET_RECOVERY = NAND_ERR_RECOVERY_ERROR,
} BERR_TYPE_E;

typedef struct _TRA_TRACK_T {
	IMSG 			rpl_msg;
	TRANSACTION_E 	tra;
} TRA_TRACK_T;

typedef struct _SCM_CHART_T {
	TRANSACTION_E	tra;
	IMSG			ret_msg;
	CALLID			caller;
	ERROR_CODE_E	err_code;
	char			*alloc_data;
	int				int_data;
	GTimeVal		time_data;
	void			*void_data;
	char			char_data;
	char			reserved[3];
	void			*result;
} SCM_CHART_T;

typedef struct _SCM_CF_T {
    int             rcvr_timeout;
    int             ats_wtdg_timeout;
	int				ats_enfc_timeout;
} SCM_CF_T;

typedef enum _RECDATA_HIDE_E {
	SHOW			= 0,
	HIDE			= 1,
} RECDATA_HIDE_E;

typedef struct _REBOOT_T {
	guint			timer;
	REBOOT_REASON_E	reason;
} REBOOT_T;

typedef struct _ATS_T {
	int		sync_active;
	int		fail_logged;
	int		sync_hour;
	guint   sync_day;
	int     sync_cycle;
	int		sync_timer;
	int		sync_enforce;
} ATS_T;

typedef struct _MMT_T {
	int		motion_state;
	int		motion_cnt;
	int		motion_status;
} MMT_T;

typedef struct _VMT_T {
	guint	vca_status;
	int		vca_time[16];
} VMT_T;

typedef struct _DMT_T {
	guint	dva_status;
	int		dva_time;
} DMT_T;

typedef struct _DXMT_T {
	guint	dvabx_status;
	int		dvabx_time[16];
} DXMT_T;

typedef struct _SCM_T {
	GThread			*thd;
	GMutex			*mtx;
	GMainLoop		*loop;
	CMMPORT			cmmpt;
	SCM_CHART_T		chart[TRA_MAX];
	SCM_CF_T		cf;
	int				sleep_time;

	WRK_ID			wrk_unc;
	WRK_ID			wrk_wan;
	WRK_ID			wrk_rtsp;
	WRK_ID			wrk_web;
	WRK_ID			wrk_ddns;
	WRK_ID			wrk_ddnstest;
	WRK_ID			wrk_unimo;
	WRK_ID			wrk_ddnsStatus;
	WRK_ID			wrk_capture;
	WRK_ID          wrk_dmode;
    WRK_ID          wrk_appqc;
	WRK_ID          wrk_ftp;
	WRK_ID			wrk_diagnosis_ipcam_net;
	WRK_ID			wrk_diagnosis_ipcam_power;
	WRK_ID			wrk_diagnosis_disk;
	WRK_ID			wrk_diagnosis_port;
	WRK_ID			wrk_camfw_upgd;
	WRK_ID          wrk_cable_check;
	WRK_ID          wrk_disk_info;
	WRK_ID          wrk_fwup_validate;
	WRK_ID          wrk_fwup_backup;
    WRK_ID          wrk_fwup_allinone;
	WRK_ID          wrk_license;
	WRK_ID          wrk_iis_thumb;
	WRK_ID			wrk_export_debug_data;

	DISK_DB_T	 	*ddb;
	TRA_TRACK_T 	ttr[MAX_TTR];

	NF_ARCH_AVI_INFO qry_result;
	NF_ARCH_SNAP_INFO qry_snap_result;

	int				enable_tml;
	GMutex			*mtx_tml;

	REBOOT_T		reboot;
	ATS_T			ats;
	int				st_unc_err;
	TIMERID			rcvr_timer;
	int				fwup_rate;
    int				fwup_step;

	MMT_T			mmt[32];
	guint			tmr_mmt;
	guint			tmr_mvrtl;

	VMT_T			vmt[32];
	guint			tmr_vmt;
	GMutex			*mtx_vmt;

	DMT_T			dmt[32];
	guint			tmr_dmt;
	GMutex			*mtx_dmt;

	DXMT_T			dxmt[32];
	DXMT_T			dxmt_fr[32];
	DXMT_T			dxmt_lpr[32];
	guint			tmr_dxmt;
	GMutex			*mtx_dxmt;

    SPT_ID          spot_sd[4];
    SPT_ID          aux_sd[4];
    SPT_ID          spot_hd[4]; 
    SPT_ID          spot_dual[4]; 

//	ivca_rule_t		lvrule;		// not used
	ivca_rule_t		ssrule;

	int				cur_rtl;
} SCM_T;

extern SCM_T iscm;

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_put_message(IMSG msgid, int param, bool dyn_data, void *data);

int _scm_return_api(CALLID callid, IMSG ret_msg, int param, bool dyn_data, void *data);
guint _scm_add_timeout(SCM_T *piscm, guint interval, GSourceFunc func, gpointer data);
int _scm_remove_timeout(SCM_T *piscm, guint id);
guint _scm_add_idle(SCM_T *piscm, GSourceFunc func, gpointer data);
int _scm_remove_idle(SCM_T *piscm, guint id);


// Transaction
int _scm_mark_error_tra(SCM_T *piscm, TRANSACTION_E tra, ERROR_CODE_E er);
int _scm_track_tra(SCM_T *piscm, IMSG rpl_msg, TRANSACTION_E tra);
TRANSACTION_E _scm_untrack_tra(SCM_T *piscm, IMSG rpl_msg);
int _scm_finalize_tra(SCM_T *piscm, TRANSACTION_E tra, int ret);
int _scm_ready_tra(SCM_T *piscm, TRANSACTION_E tra, IMSG ret_msg, CALLID caller);




// DB
int _scm_get_cur_lang(char *buf, int buf_len);
int _scm_work_factory_default(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_db_import(SCM_T *piscm, TRANSACTION_E tra);
int _scm_finalize_db_change(SCM_T *piscm, TRANSACTION_E tra);




// Notify
void _scm_register_syscb();
int _scm_inform_db_changing(TRANSACTION_E tra);
int _scm_inform_smart_alarmon();
int _scm_inform_smart_alarmoff();
int _scm_check_novideo(SCM_T *piscm);




// Video Filesystem
int _scm_change_wmode_by_db(SCM_T *piscm);
int _scm_req_disk_confirm(SCM_T *piscm, TRANSACTION_E tra, DISK_CONF_E disk_conf);
guint _scm_get_rtl_state();
int _scm_save_disk_info(SCM_T *piscm);
int _scm_get_disk_space(SCM_T *piscm, guint64 *total, guint64 *used);
int _scm_load_disk_info(SCM_T *piscm);
int _scm_unload_disk_info(SCM_T *piscm);
DISK_CONF_E _scm_get_disk_change_info(SCM_T *piscm);

int _scm_retouch_rcvr_timer(SCM_T *piscm);
int _scm_stop_rcvr_timer(SCM_T *piscm);

int _scm_get_err_cnt(SCM_T *piscm);
int _scm_get_recovery_err_cnt(SCM_T *piscm);
int _scm_get_smart_err_cnt(SCM_T *piscm);
int _scm_get_disk_err_cnt(SCM_T *piscm);
int _scm_reset_err_cnt(SCM_T *piscm);
int _scm_increase_err_cnt(SCM_T *piscm, BERR_TYPE_E etype, guchar pdid);


int _scm_start_unc_checking(SCM_T *piscm);
int _scm_stop_unc_checking(SCM_T *piscm);

int _scm_work_start_service(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_stop_service(SCM_T *piscm, TRANSACTION_E tra, RS_SRVSTOP_E rs);
int _scm_work_smart_boot(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_rtl(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_net_stop(SCM_T *piscm, TRANSACTION_E tra, int reason);
int _scm_work_format(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_rec_start(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_rec_stop(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_fs_stop(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_fs_start(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_net_start(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_data_delete(SCM_T *piscm, TRANSACTION_E tra, time_t delete_time);
int _scm_work_panicrec_start(SCM_T *piscm);
int _scm_work_panicrec_stop(SCM_T *piscm);
int _scm_work_panicrec_toggle(SCM_T *piscm);
int _scm_change_wmode(SCM_T *piscm, int mode);
int _scm_get_wmode(SCM_T *piscm, NF_DISK_WRITE_MODE_E *w_mode);
int _scm_work_create_raid(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_delete_raid(SCM_T *piscm, TRANSACTION_E tra);
int _raid_upgrade_fw(SCM_T *piscm, TRANSACTION_E tra);





HANDLER int _scm_on_fs_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_fs_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_delete_data_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_format_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_disk_format_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_disk_sync_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_rtl_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_rec_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_rec_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_net_stop(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_net_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_unc_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_smart_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_rcvr_expired(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_rcvr_rate(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_smart_confirm(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_dirmode_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_diskinit_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_need_check_raid(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_ipcam_install(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_check_cable_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_appqc_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
HANDLER int _scm_on_get_license_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);





// Format
int _scm_format_storage(IMSG ret_msg);
int _scm_sync_storage(IMSG ret_msg);




// Factory Default
int _scm_start_factory_default(TRANSACTION_E tra);




// FW Upgrade
int _scm_work_fw_upgrade(SCM_T *piscm, TRANSACTION_E tra);
HANDLER int _scm_on_fw_up(SCM_T *piscm, CMM_MESSAGE_T *pmsg);




// Network
int _scm_init_netif();
HANDLER int _scm_on_network_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
int _scm_init_ddns_hostname();
int _scm_work_ip_change(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_ipcam_ctrl(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_ssl_install(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_ssl_delete(SCM_T *piscm, TRANSACTION_E tra);
int _scm_openssl_delete_cert();
int _scm_init_8021x_cert();



// Timeline
int _scm_disable_timeline();
int _scm_enable_timeline();
int _scm_init_timeline(SCM_T *piscm);



// LOG
int _scm_init_log_context();
int _scm_destroy_log_context();
//int _scm_put_log(PUTLOG_TYPE_E type, int param);
int _scm_put_log(PUTLOG_TYPE_E type, int param, char *text);
int _scm_put_log_with_tra(SCM_T *piscm, PUTLOG_TYPE_E type, int param1, int param2, TRANSACTION_E tra);
int _scm_put_log_with_tra_prev_user(SCM_T *piscm, PUTLOG_TYPE_E type, int param1, int param2, TRANSACTION_E tra);


// Arch
int _scm_save_qry_result(SCM_T *piscm, NF_ARCH_AVI_INFO *result);
int _scm_save_snap_qry_result(SCM_T *piscm, NF_ARCH_SNAP_INFO *result);



// Arch playback
HANDLER int _scm_on_verify_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);




// Date / Time
int _scm_init_time_change();
int _scm_work_time_change(SCM_T *piscm, TRANSACTION_E tra);
int _scm_work_delete(SCM_T *piscm, TRANSACTION_E tra);
int _scm_init_timesync(SCM_T *piscm);
int _scm_work_post_ats(SCM_T *piscm, TRANSACTION_E tra);



// Audio
int _scm_init_audio(SCM_T *piscm);
int _scm_apply_live_audio_onoff();
int _scm_apply_live_audio_ch();
int _scm_apply_buzzer_config();




// System
guint _scm_turnon_reboot_timer(SCM_T *piscm, int msec);
int _scm_turnoff_reboot_timer(SCM_T *piscm, guint id);
HANDLER int _scm_on_booting_start(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
int _scm_work_booting_end(SCM_T *piscm, RESULT_E result);
int _scm_work_shutdown(SCM_T *piscm, TRANSACTION_E tra);
int _scm_reboot_system(SCM_T *piscm, REBOOT_REASON_E reason, int after_msec);
int _scm_cancel_reboot(SCM_T *piscm);
int _scm_work_igndisk_boot(SCM_T *piscm, TRANSACTION_E tra);




// IP cam
int _scm_openmode_req_ip_assign(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
int _scm_openmode_change_pw(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
int _scm_openmode_port_setup(SCM_T *piscm, CMM_MESSAGE_T *pmsg);
int _scm_init_ipcam_poe_onoff(SCM_T *piscm);





// Notify
int _scm_check_novideo(SCM_T *piscm);
int _scm_init_mot_timer(SCM_T *piscm);
int _scm_init_vca_event_timer(SCM_T *piscm);
int _scm_init_notify(SCM_T *piscm);




// LED
int _scm_update_record_led(NF_NOTIFY_INFO *pnotify);
int _scm_update_network_led(NF_NOTIFY_INFO *pnotify);
int _scm_update_alarm_led(NF_NOTIFY_INFO *pnotify);


// DIAGNOSIS
HANDLER int _scm_on_diagnosis_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg);

// QC
int scm_appqc_factory_default(IMSG ret_msg);

#endif
