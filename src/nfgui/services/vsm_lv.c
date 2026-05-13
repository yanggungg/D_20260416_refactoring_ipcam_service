/*
 * vsm_lv.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#include "nf_afx.h"

#include "../support/color.h"
#include "../support/util.h"
#include "modules/ssm.h"

#include "cmm.h"
#include "scm.h"
#include "uxm.h"
#include "smt.h"

#include "ix_mem.h"

#include "vsm.h"
#include "vsm_internal.h"

#include "vw_alarm_name_popup.h"


static guint panic_tid = 0;
static gint panic_elapsed_time = 0;

////////////////////////////////////////////////////////////
//
// protected interfaces
//

static void _set_current_time()
{
	_vvm_live_set_current_time();
}

static void _set_login_user()
{
	gchar cur_user[VWND_TEXT_LEN];

	memset(cur_user, 0x00, VWND_TEXT_LEN);

	ssm_get_cur_id(cur_user);
	_vvm_live_set_user(cur_user);
}

static void _set_livestatus()
{
	_set_current_time();
	_set_login_user();
}

gboolean _vsm_live_set_livestatus(gpointer data)
{
	VSM_T *ivsm;

	ivsm = (VSM_T*)data;

	_set_livestatus();
	
	return TRUE;
}

static gint _is_panic_record(NF_NOTIFY_INFO *data)
{
	guint ch;
    VSM_DIV_E dtype;
    gchar win_id;

    if (data)
	{
        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (data->c.chmap[ch] == 'P') return 1;
        }
    }

	return 0;
}

static gboolean _check_panic_duration(gpointer data)
{
	time_t curtime = time(0);
	time_t starttime = GPOINTER_TO_UINT(data);
	time_t endtime = nf_record_get_panic_end_time();
	guint tmp_elapsed = curtime - starttime;

	if (endtime)
	{
		if (curtime > endtime) return TRUE;
	}

	if ((tmp_elapsed == 0) || (tmp_elapsed != panic_elapsed_time))
	{
		panic_elapsed_time = tmp_elapsed;
	}

	return TRUE;
}

static void _stop_panic_duration(gpointer data)
{
	evt_send_to_local(INFY_PANIC_REC_STATUS, 0, 0, 0);
	
	panic_tid = 0;
	panic_elapsed_time = 0;
}

void _vsm_live_reset_livestatus(gpointer data)
{
	_vvm_live_reset_livestatus();
}

void _vsm_start_panic_duration()
{
	evt_send_to_local(INFY_PANIC_REC_STATUS, 1, 0, 0);
/*
	time_t curtime = time(0);

	if (!panic_tid) {
		panic_elapsed_time = 0;
		evt_send_to_local(INFY_PANIC_REC_STATUS, 1, 0, 0, 0);
		_check_panic_duration(GUINT_TO_POINTER(curtime));
    	panic_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 300, _check_panic_duration, GUINT_TO_POINTER(curtime), _stop_panic_duration);
	}
*/	
}

void _vsm_stop_panic_duration()
{
	evt_send_to_local(INFY_PANIC_REC_STATUS, 0, 0, 0);
/*
	if (panic_tid)
		g_source_remove(panic_tid);
*/
}

void _vsm_check_panic_duration(NF_NOTIFY_INFO *data)
{
	if (_is_panic_record(data)) _vsm_start_panic_duration();
	else _vsm_stop_panic_duration();
}

void _vsm_open_sensor_osd_popup(gint ch)
{
	gchar buf[32];

	if(DAL_get_almSen_oPop_boolean(ch)) {
		memset(buf, 0x00, sizeof(buf));
		if(DAL_get_almSen_name(ch, buf)) {
			smt_set_service(SMT_LIVE_OSDPOPUP);
			VW_Show_OSD_Popup(buf);
		}
	}
}

void _vsm_open_system_osd_popup(guint evt_type, gchar *str)
{
	if(DAL_get_SysSys_oPop_boolean(evt_type))
		VW_Show_OSD_Popup(str);
}

void _vsm_open_disk_osd_popup(guint evt_type, gchar *str)
{
	if(DAL_get_SysDisk_oPop_boolean(evt_type))
		VW_Show_OSD_Popup(str);
}

void _vsm_close_osd_popup()
{
	VW_Hide_OSD_Popup();
	smt_set_service(SMT_LIVE);
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

void vsm_turn_on_alarm_status()
{

//	g_message("#############parangi %s, %d", __FUNCTION__, __LINE__);
	_vvm_set_alarm_status(TRUE);
}

void vsm_turn_off_alarm_status()
{

//	g_message("#############parangi %s, %d", __FUNCTION__, __LINE__);
	_vvm_set_alarm_status(FALSE);
}

