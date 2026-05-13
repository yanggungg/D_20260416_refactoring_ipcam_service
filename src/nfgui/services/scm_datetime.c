/*
 * scm_datetime.c
 * 	- scm date/time service
 *	- dependencies :
 *		
 *
 * Written by eddy.kim
 * Copyright (c) ITX security, Mar 5, 2011
 *
 */

#include "iux_afx.h"
#include "scm_internal.h"
#include "vfs.h"
#include "nf_util_sntp.h"
#include "nf_util_time.h"
#include "smt.h"
#include "evt.h"
#include "ssm.h"
#include "nfdal.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_TIME"

#define	ONEMIN			(60 * 1000)
#define DAILY           (86400)
#define TARGET_TIME		(piscm->chart[tra].time_data.tv_sec)
#define CYCLE_TIME      (ssm_is_itxdebug_id() ? 180 : DAILY)
////////////////////////////////////////////////////////////
//
// private data type
//

typedef struct _TIME_CHANGE_T {
	GTimeVal 	newtv;
	time_t		triggered;
	guint		wdtimer;
	int 		wd_cnt;
} TIME_CHANGE_T;

typedef enum _ATS_FAIL_E {
	ATS_ERR_NOT_SYNCTIME	= 1,
	ATS_ERR_SERVER_ERROR	= 2,
	ATS_ERR_SAME_TIME		= 3,
	ATS_ERR_NOT_LIVE		= 4,
	ATS_ERR_2_MINUTE		= 5,
	ATS_ERR_OVER_SYNCTIME   = 6,
	ATS_ERR_NOT_ACTIVE      = 7,
} ATS_FAIL_E;

enum {
	ALL_NTP_OFF = 0,
	SMART_NTP,
	SCHED_NTP
};

////////////////////////////////////////////////////////////
//
// private variables
//

static TIME_CHANGE_T itc;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _apply(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_TIME_CHANGE;
//onvif_porting reboot issue(set datetime)
	if(iscm.chart[tra].caller) 
		return -1;
//onvif_porting reboot issue(set datetime)	
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].time_data = itc.newtv;
	iscm.chart[tra].int_data = time(0);

	DMSG(1, "target: [%u] triggered: [%u]", itc.newtv.tv_sec, itc.triggered);
	_scm_push_notification(INFY_TIMECHANGE_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, RS_TIMECHANGE);
	// subsequent operation will be run as ret_msg is arrived.

	return 0;
}

static gboolean _proc_watchdog(gpointer data)
{
	SCM_T *piscm = (SCM_T *)data;
	++itc.wd_cnt;

	DMSG(1, "WATCH DOG COUNTDOWN [%d]", piscm->cf.ats_wtdg_timeout - itc.wd_cnt);
	if (itc.wd_cnt == piscm->cf.ats_wtdg_timeout) {
		itc.wd_cnt = 0;
		_scm_reboot_system(piscm, RR_WATCHDOG, 0);
		return FALSE;
	}

	return TRUE;
}

static int _enable_watchdog_timer(SCM_T *piscm)
{
	if (!itc.wdtimer) {
		itc.wd_cnt = 0;
		itc.wdtimer = _scm_add_timeout(&iscm, 1000, _proc_watchdog, piscm);
	}
	else
		DMSG(1, "WATCHDOG TIMER IS RUNNING");

	return 0;
}

static int _disable_watchdog_timer(SCM_T *piscm)
{
	if (itc.wdtimer) {
		_scm_remove_timeout(piscm, itc.wdtimer);
		itc.wdtimer = 0;
	}
	return 0;
}

static int _check_sync_date(SCM_T *piscm)
{
	gint cyear, cmon, cday, syear, smon, sday;
	gint ch, cm, cs;
	gint ret = 1;

	dtf_get_local_day(time(0), &cyear, &cmon, &cday);                   //current time
	dtf_get_local_hourmin(time(0), &ch, &cm, &cs);
	dtf_get_local_day(piscm->ats.sync_day, &syear, &smon, &sday);       //sync time
	
	DMSG(9, "CURRENT DAYTIME : [%d-%d-%d %d:%d]", cyear, cmon, cday, ch, cm);
	DMSG(9, "SYNC_DAY    : [%d-%d-%d %d:00]", syear, smon, sday, piscm->ats.sync_hour);
	
	if (cyear != syear || cmon != smon || cday != sday) {
		ret = 0;
	}

	DMSG(9, "ret : %d", ret);

	return ret;
}

static int _sync_hour_cmp_with_cur_hour(SCM_T *piscm)
{
    gint cur_time;
    gint ret = 0;

    DMSG(1,"");
    
    dtf_get_local_hourmin(time(0), &cur_time, 0, 0);
    
	if (piscm->ats.sync_hour < cur_time) ret = -1;
	else if (piscm->ats.sync_hour > cur_time) ret = 1;
    else if (piscm->ats.sync_hour == cur_time) ret = 0;

    DMSG(1, "cur : %d, sync : %d, ret : %d", cur_time, piscm->ats.sync_hour, ret);

    return ret;
}

static int _is_within_in_2min(GTimeVal *tv_temp)
{
	GTimeVal tv_cur;
	memset(&tv_cur, 0x00, sizeof(GTimeVal));
	g_get_current_time(&tv_cur);

	if (abs(tv_temp->tv_sec - tv_cur.tv_sec) < 2 * 60) 	return 1;
	DMSG(9, "");
	return 0;
}

static int _is_time_to_activate(SCM_T *piscm)
{
    gint ret = 0;

    DMSG(1,"");
	
	if (piscm->ats.sync_cycle > 86400)
	{
    	if (_sync_hour_cmp_with_cur_hour(piscm) || !_check_sync_date(piscm)) ret = 1;
	}
	else
	{
    	if (_sync_hour_cmp_with_cur_hour(piscm)) ret = 1;
	}
	
    DMSG(1,"ret : %d", ret);
    
	return ret;
}

static int _activate_ats(SCM_T *piscm)
{
    DMSG(1, "");
	piscm->ats.sync_active = 1;
	piscm->ats.fail_logged = 0;
	return 0;
}

static int _deactivate_ats(SCM_T *piscm)
{
    DMSG(1, "");
	piscm->ats.sync_active = 0;
	piscm->ats.fail_logged = 0;
	return 0;
}

static int _is_same_time(GTimeVal *tv_temp)
{
	GTimeVal tv_cur;
	memset(&tv_cur, 0x00, sizeof(GTimeVal));
	g_get_current_time(&tv_cur);

	DMSG(1, "TIME COMPARE [%ld, %ld]", tv_temp->tv_sec, tv_cur.tv_sec);
	return (tv_temp->tv_sec == tv_cur.tv_sec);
}

static int _delay_synctime(SCM_T *piscm, guint d_time)
{
	DateTimeData dtdata;
	
    DMSG(1, "OVER AT SYNC TIME!!");
    piscm->ats.sync_day += d_time;       // delay 24hour
    
    DAL_get_dateTime_data(&dtdata);
    DAL_set_last_sync_time(dtdata.last_sync_time + d_time);
    DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

    return 0;
}

static int _is_enforce_sync(SCM_T *piscm)
{
	static int enforce_cnt = 0;
	
	if (piscm->ats.sync_enforce > 0) {
		--piscm->ats.sync_enforce;

		if (piscm->ats.sync_enforce == 0) {
			piscm->ats.sync_enforce = piscm->cf.ats_enfc_timeout;
			++enforce_cnt;
			DMSG(1, "ENFORCED AUTO TIME SYNC, count = %u", enforce_cnt);
			return 1;
		}
	}

	return 0;
}

static int _is_time_to_sync(SCM_T *piscm, ATS_FAIL_E *ret)
{
	int ntp;
	int res;
	GTimeVal tv_temp;
	DateTimeData dtdata;
	
	memset(&tv_temp, 0x00, sizeof(GTimeVal));

	DAL_get_dateTime_data(&dtdata);

	if (_is_enforce_sync(piscm)) {
	    return 1;
	}

    if (piscm->ats.sync_active != 1)
    {
    	DMSG(9, "NOT ACTIVE ATS: [%d]", piscm->ats.sync_active);
    	*ret = ATS_ERR_NOT_ACTIVE;
        return 0;
    }

	if (piscm->ats.sync_cycle > 86400)
	{
    	if (!_check_sync_date(piscm)) {
    		*ret = ATS_ERR_NOT_SYNCTIME;
    		return 0;
    	}
	}

    if ((res = _sync_hour_cmp_with_cur_hour(piscm)) != 0)
	{
        if (piscm->ats.sync_cycle > 86400 && res == -1)
        {
	        *ret = ATS_ERR_OVER_SYNCTIME;
        }
	    else
        {
    		*ret = ATS_ERR_NOT_SYNCTIME;
		}
    		
		return 0;
	}

	if (smt_get_service() != SMT_LIVE && smt_get_service() != SMT_LOGOUT) {
		DMSG(9, "NOT IN LIVE VIEW(or LOGOUT), UNABLE TO DO AUTO TIME SYNC\n");
		*ret = ATS_ERR_NOT_LIVE;
		return 0;
	}

	DMSG(9, "TIME SERVER = [%s]", dtdata.timeServer);
	ntp = scm_get_ntp_time(dtdata.timeServer, &tv_temp);
	DMSG(9, "NTP QUERY RETURN : [%d]", ntp);
	if (ntp != 0) {
		*ret = ATS_ERR_SERVER_ERROR;
		return 0;
	}

	if (_is_same_time(&tv_temp)) {
		DMSG(1, "SAME TIME");
		*ret = ATS_ERR_SAME_TIME;
		return 0;
	}

/*	if (_is_within_in_2min(&tv_temp)) {
		DMSG(1, "GAP IS LESS THAN 2 MINUTES");
		*ret = ATS_ERR_2_MINUTE;
		return 0;
	}*/

	return 1;
}

static int _run_auto_time_sync(SCM_T *piscm)
{
	int ret;
	GTimeVal tv_temp;
	DateTimeData dtdata;
	memset(&tv_temp, 0x00, sizeof(GTimeVal));

	DAL_get_dateTime_data(&dtdata);
	DMSG(9, "TIME SERVER = [%s]", dtdata.timeServer);
	ret = scm_get_ntp_time(dtdata.timeServer, &tv_temp);
	DMSG(9, "NTP QUERY RETURN : [%d]", ret);
	if (ret != 0) {
		DMSG(1, "WRITE LOG AUTO TIME SYNC FAIL [%d]", ret);
		if (piscm->ats.fail_logged == 0) {
			scm_put_log(FAIL_AUTOTIMESYNC, 0, 0);
			piscm->ats.fail_logged = 1;
		}
		return 0;
	}

	DMSG(9, "");
	_scm_send_msg_to_viewer(IREQ_UI_LOCK, 0);
	sleep(1);

	smt_set_service(SMT_AUTO_SYNC);
	scm_set_system_time(&tv_temp);	
	_enable_watchdog_timer(piscm);

//    nf_disk_preserve_delete_mode(FALSE);
	
	_deactivate_ats(piscm);
	_apply(IMSG_NONE);
	
	return 0;
}

static gboolean _proc_check_ats(void *data)
{
	ATS_FAIL_E err;
	SCM_T *piscm = (SCM_T *)data;

	if (_is_time_to_sync(piscm, &err)) {
		_run_auto_time_sync(piscm);
		return TRUE;
	}

    DMSG(1, "ATS ERR REASON : [%d]", err);
    
	if (err == ATS_ERR_SAME_TIME) _deactivate_ats(piscm);
	if (_is_time_to_activate(piscm)) _activate_ats(piscm);
	if (err == ATS_ERR_OVER_SYNCTIME) _delay_synctime(piscm, 86400);

	return TRUE;
}

static int _change_system_time(SCM_T *piscm,time_t triggered, time_t timet)
{
	gboolean ret;
	time_t offset;

	DMSG(1, "target: [%u], triggered : [%u]", timet, triggered);
	nf_zoneinfo_init();
	offset = time(0) - triggered;
	DMSG(1, "offset : [%u]", offset);
	ret = nf_datetime_set(timet + offset);
    DMSG(1, "ret = %d", ret);
	return (ret ? 0 : -1);
}

int _set_system_time(GTimeVal *tv, time_t triggered)
{
	itc.triggered = triggered;
	itc.newtv = *tv;
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_init_time_change()
{
	memset(&itc, 0x00, sizeof(TIME_CHANGE_T));
	return 0;
}

int _scm_work_time_change(SCM_T *piscm, TRANSACTION_E tra)
{
    time_t triggerd = piscm->chart[tra].int_data;
	gboolean ret;

	_change_system_time(piscm,triggerd, TARGET_TIME);
#if defined(_HDI_MODEL_UX) 
    ret = nf_dspcomm_sync_time();
#endif
	_scm_send_msg_to_viewer(INFY_TIMECHANGE, 0);
	if (piscm->chart[tra].caller == NVM_CALL) {
    	DMSG(1, "");
	    nvm_success_time_sync();
	}
	DMSG(1, "");
	nf_notify_fire_params("time_change", 0, 0, 0, 0);
	DMSG(1, "");
	_scm_work_service_start(piscm, tra);

	// time sync api called
	nf_onvif_set_time_sync_all();

	return 0;
}

int _scm_work_delete(SCM_T *piscm, TRANSACTION_E tra)
{
	GTimeVal cur_tv;

	memset(&cur_tv, 0x00, sizeof(GTimeVal));
	g_get_current_time(&cur_tv);

	if (TARGET_TIME < cur_tv.tv_sec) _scm_work_data_delete(piscm, tra, TARGET_TIME);
	else _scm_work_data_delete(piscm, tra, 0);
	return 0;
}

int _scm_work_post_ats(SCM_T *piscm, TRANSACTION_E tra)
{
	IMSG ret_msg = piscm->chart[tra].ret_msg;
	ERROR_CODE_E err_code = piscm->chart[tra].err_code;
	GTimeVal tv;

	DMSG(9, "");
    g_get_current_time(&tv);
    
	if (ret_msg != IMSG_NONE) return -1;		// if auto time sync, ret_msg is IMSG_NONE

	if (err_code != ER_NONE)
	{
	    DMSG(9, "ERROR : [%d]", err_code);
        scm_put_log(FAIL_AUTOTIMESYNC, 0, 0);
        
        if (piscm->ats.sync_cycle > 86400)
        {
        	_delay_synctime(piscm, 86400);
        }
	}
	else
	{
    	DMSG(9, "AUTO TIME SYNC, POST PROCESSING");
    	scm_put_log(SUCC_AUTOTIMESYNC, 0, 0);
    	
        DAL_set_last_sync_time(tv.tv_sec);
        
        if (piscm->ats.sync_cycle > 86400)
        {
        	piscm->ats.sync_day = tv.tv_sec + piscm->ats.sync_cycle;
        }
	}

	_disable_watchdog_timer(piscm);
	_scm_send_msg_to_viewer(IREQ_UI_UNLOCK, 0);
    
	smt_set_service(SMT_LIVE);
	return 0;
}

int _scm_init_timesync(SCM_T *piscm)
{
	DateTimeData dtdata;
	int hour;
	int result = SMART_NTP;

	DAL_get_dateTime_data(&dtdata);
	
	if(dtdata.time_sync_off == 1) {
		result = ALL_NTP_OFF;
	} else if(dtdata.time_sync_off == 0 && dtdata.auto_timesync == 0) {
		result = SMART_NTP;
	} else if(dtdata.time_sync_off == 0 && dtdata.auto_timesync == 1) {
		result = SCHED_NTP;
	}
	
	DMSG(9, "AUTO TIME SYNC ENABLE = [%d]", result == SCHED_NTP);
	if (result == SCHED_NTP) {
		piscm->ats.sync_hour = dtdata.sync_time;
		piscm->ats.sync_cycle = dtdata.sync_freq;
		piscm->ats.sync_day = dtdata.last_sync_time + dtdata.sync_freq;
		piscm->ats.sync_enforce = piscm->cf.ats_enfc_timeout;

        _deactivate_ats(piscm);

		if (piscm->ats.sync_timer) _scm_remove_timeout(piscm, piscm->ats.sync_timer);
		piscm->ats.sync_timer = _scm_add_timeout(piscm, ONEMIN, _proc_check_ats, piscm);
	}
	else {
		DMSG(9, "");
		if (piscm->ats.sync_timer) _scm_remove_timeout(piscm, piscm->ats.sync_timer);
		piscm->ats.sync_timer = 0;
	}
	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_is_past_time_set()
{
	time_t cur = time(0);
	if (itc.newtv.tv_sec == 0) return 0;
	return (itc.newtv.tv_sec < cur);
}

int scm_set_system_time(GTimeVal *tv)
{
	_set_system_time(tv, time(0));
	return 0;
}

int scm_restore_system_time()
{
	memset(&itc.newtv, 0x00, sizeof(GTimeVal));
	return 0;
}

int scm_apply_timezone(int idx, NF_TIME_UDD *udd)
{
	nf_zoneinfo_set(idx, udd);
	return 0;
}

int scm_is_set_new_time()
{
	return (itc.newtv.tv_sec != 0);
}

int scm_apply_time(IMSG ret_msg)
{
	DMSG(1, "");
	
//	nf_disk_preserve_delete_mode(TRUE);

	_deactivate_ats(&iscm);
	_apply(ret_msg);
	return 0;
}

int scm_change_time(time_t target, time_t triggered, IMSG ret_msg)
{
	GTimeVal ctv;

  DMSG(1, "target : [%ld], triggered : [%ld]", target, triggered);
	memset(&ctv, 0x00, sizeof(GTimeVal));
	ctv.tv_sec = target;
	ctv.tv_usec = 0;

// can't sync time to NVR accurately
//	_set_system_time(&ctv, triggered);

	_set_system_time(&ctv, time(0));

//    nf_disk_preserve_delete_mode(TRUE);

	_deactivate_ats(&iscm);
	_apply(ret_msg);
	return 0;
}

int scm_change_time_by_ntp(char *server_name, IMSG ret_msg)
{
	return 0;
}

int scm_get_ntp_time(char *server_name, GTimeVal *ret)
{
	ret->tv_sec = nf_util_sntp(server_name);
	return (ret->tv_sec > 0 ? 0 : -1);	
}

int scm_init_timesync()
{
	_scm_init_timesync(&iscm);
	return 0;
}
