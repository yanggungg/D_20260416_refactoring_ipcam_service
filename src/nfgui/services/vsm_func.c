/*
 * vsm_func.c
 *        - dependency :
 *
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */


#include "nf_afx.h"
#include "nf_pos.h"

#include "../support/color.h"
#include "../support/util.h"

#include "cmm.h"
#include "scm.h"
#include "evt.h"
#include "modules/ssm.h"

#include "ix_mem.h"

#include "vsm.h"
#include "vw.h"
#include "vsm_internal.h"
#include "vw_sys_disp_sequence.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"VSM_FUNC"

enum {
	POPUP_MODE_ALARM = 0,
	POPUP_MODE_MOTION,
	POPUP_MODE_VCA,
	POPUP_MODE_DVA,
	POPUP_MODE_DVABX,
	POPUP_MODE_POS,
};

////////////////////////////////////////////////////////////
//
// private variable
//

typedef struct _FUNC_SEQ_T
{
	SeqData 	data;
	guint 		tid;
	gint 		next;
	guint 		dwell;
} FUNC_SEQ_T;

typedef struct _FUNC_POPUP_T
{
	GMutex 			*rw_lock;
	GSList 			*slot;
	gboolean		run;
	gulong 			alm_cb;
	gulong 			mot_cb;
    BITMASK64       alm_prev_status;
    guint           mot_prev_status;
    guint           vca_prev_status;
    guint           pos_prev_status;
	gboolean		until_key;
	gint			ignor_ch[GUI_CHANNEL_CNT];

	SFC_T			last_sfc;
} FUNC_POPUP_T;

typedef struct _FUNC_T
{
	FUNC_SEQ_T		seq;
	FUNC_POPUP_T	popup;
} FUNC_T;

static FUNC_T ivf;

#if defined(GUI_8CH_SUPPORT)
static guint div_cnt[] = {1, 4, 6, 8, 8};
#else
static guint div_cnt[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

////////////////////////////////////////////////////////////
//
// 1. VSM_FUNC - SEQUENCE
//

static gboolean _seq_div_mode(gpointer data)
{
    gchar win[VSM_WIN_MAX];
    guint dtype = 0;
    guint i;
	gint cnt = 0;

	while(1)
	{
		if(ivf.seq.next < (gint)ivf.seq.data.numItems - 1) ivf.seq.next++;
		else ivf.seq.next = 0;

		memset(win, 0xff, sizeof(win));

    	switch(ivf.seq.data.items[ivf.seq.next].type) {
    		case SCR_DIV_TYPE1:
    			dtype = VSM_DIV1;
    		break;
    		case SCR_DIV_TYPE4:
    			dtype = VSM_DIV4;
    		break;
#ifndef GUI_4CH_SUPPORT
    		case SCR_DIV_TYPE9:
    			dtype = VSM_DIV9;
    		break;
#ifndef GUI_8CH_SUPPORT
    		case SCR_DIV_TYPE16:
    			dtype = VSM_DIV16;
    		break;
#ifndef GUI_16CH_SUPPORT
    		case SCR_DIV_TYPE36:
    			dtype = VSM_DIV32;
    		break;
#endif
#endif
#ifndef _NOT_SUPPORT_SPC_DIV
    		case SCR_DIV_TYPE8:
    			dtype = VSM_DIV8;
    		break;
    		case SCR_DIV_TYPE6:
    			dtype = VSM_DIV6;
    		break;
#endif
#endif
    		default:
				g_warning("%s, %d not supported type", __FUNCTION__, __LINE__);
    		break;
    	}

		for(i = 0; i < div_cnt[dtype]; i++)
			win[ivf.seq.data.items[ivf.seq.next].conf[i]] = i;

		if(i < div_cnt[dtype]) break;

		cnt++;

		if(cnt == ivf.seq.data.numItems+1) break;
	}

	_vvm_live_set_seq_icon();
	vsm_change_sfc_by_array(win, dtype);

	return TRUE;
}


static void _seq_page_reset(gpointer data)
{
	if(ivf.seq.next >= 0)
		ivf.seq.next = -1;

	_vvm_live_unset_seq_icon();
}

static void _func_sequence_start(void)
{
	guint seq_count;
	guint i;

	seq_count = DAL_get_sequence_count();

	for(i=0; i<seq_count; i++) {
		memset(&ivf.seq.data, 0x00, sizeof(SeqData));
		DAL_get_seq_data(&ivf.seq.data, i);

		if(ivf.seq.data.valid_mode == SEQ_MODE_VALID) {
			if(ivf.seq.data.numItems <= 0) return;
			else break;
		}
	}

	if(ivf.seq.data.valid_mode == SEQ_MODE_INVALID || ivf.seq.data.valid_mode == SEQ_MODE_DELETED)
	{
		nftool_mbox(NF_TOPWND, "NOTICE", "Please activate the sequence at the  display - sequence menu.", NFTOOL_MB_OK);
		return;
	}

	evt_send_to_local(INFY_VWND_STOP_RIGHT_PRESS, 0, 0, 0);

	ivf.seq.dwell = DAL_get_sequence_dwell();

	ivf.seq.tid = g_timeout_add_full(G_PRIORITY_DEFAULT, (ivf.seq.dwell * 1000), _seq_div_mode, NULL, _seq_page_reset);

	_seq_div_mode(NULL);
}

gboolean _vsm_func_sequence_stop(void)
{
	if(ivf.seq.tid != 0) {
		g_source_remove(ivf.seq.tid);
		ivf.seq.tid = 0;

 	   evt_send_to_local(INFY_VWND_RUN_RIGHT_PRESS, 0, 0, 0);
		return TRUE;
	}

	return FALSE;
}

gint vsm_live_cmd_sequence()
{
	if(ivf.seq.tid != 0)
	{
		_vsm_reset_recover_sfc_by_func();
		_vsm_func_sequence_stop();
		_vvm_refresh_seq_icon();				// seq icon erase
	}
	else
	{
		_vsm_func_stop_popup();
		_vsm_set_recover_sfc_by_func();
		_func_sequence_start();
		_vsm_func_run_popup();
	}

	return 0;
}


////////////////////////////////////////////////////////////
//
// 2. VSM_FUNC - EVENT POPUP
//
#define POP_ON_MAX_S	(300)			// second

static void _set_popup_run(gboolean val)
{
	ivf.popup.run = val;
}

static gboolean _get_popup_run()
{
	return ivf.popup.run;
}

static void _set_popup_run_until_key(gboolean val)
{
	ivf.popup.until_key = val;
}

static gboolean _get_popup_run_until_key()
{
	return ivf.popup.until_key;
}

static void _release_ignor_ch(gint ch)
{
	if(ivf.popup.ignor_ch[ch])
	{
		if(ivf.popup.ignor_ch[ch] > 0)
			g_source_remove(ivf.popup.ignor_ch[ch]);

		ivf.popup.ignor_ch[ch] = 0;
	}
}

static gboolean _release_ignor_cb(gpointer data)
{
	gint ch;

	ch = GPOINTER_TO_INT(data);
	ivf.popup.ignor_ch[ch] = 0;

	return FALSE;
}

static void _release_ignor(guint ch_mask)
{
	gint i;
	guint interval;

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
//		if(ch_mask & (1 << i))
		{
			if(ivf.popup.ignor_ch[i] < 0)
			{
				interval = DAL_get_MotSen_ignor_interval(i);
				interval += 1;
				ivf.popup.ignor_ch[i] = g_timeout_add(interval*1000, _release_ignor_cb, GINT_TO_POINTER(i));
			}
		}
	}
}

static guint _get_popup_slot_data()
{
	guint slot_data = 0;
	guint remain_slot_cnt;

	g_mutex_lock (ivf.popup.rw_lock);

	remain_slot_cnt = g_slist_length(ivf.popup.slot);

	if (remain_slot_cnt)
	{
		slot_data = GPOINTER_TO_UINT(g_slist_nth_data(ivf.popup.slot, 0));
		//if(!_get_popup_run_until_key())
		//	ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, 0));

		// ignoring motion event
		if(remain_slot_cnt == 2)
			_release_ignor(slot_data);

		if(_get_popup_run_until_key()) {
			if(remain_slot_cnt > 1)
				ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, 0));
			else
				slot_data = 0;
		}else {
			ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, 0));
		}

	}

	g_mutex_unlock (ivf.popup.rw_lock);

	return slot_data;
}


static void _put_popup_slot_data(guint ch_event_mask, gint timeout_cnt)
{
	GSList *slot;

	gint i, new_slot_cnt;
	gint remain_slot_cnt;
	guint ori_ch_mask = 0;
	guint slot_data = 0;

	_vsm_func_sequence_stop();

	g_mutex_lock (ivf.popup.rw_lock);

// arrange remain-slot!!
	remain_slot_cnt = g_slist_length(ivf.popup.slot);

	if (remain_slot_cnt)
	{
		slot = g_slist_nth(ivf.popup.slot, remain_slot_cnt - 1);
		ori_ch_mask = GPOINTER_TO_UINT(slot->data);

		// remove sfc recover slot!!
		ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, remain_slot_cnt-1));
		remain_slot_cnt -= 1;
	}

	if(!_get_popup_run_until_key())
	{
#if 0
		if (remain_slot_cnt)
		{
			for (i = 0; i < remain_slot_cnt; i++)
			{
				slot = g_slist_nth(ivf.popup.slot, i);
				slot->data = GUINT_TO_POINTER(0);
				slot->data = GUINT_TO_POINTER(GPOINTER_TO_UINT(slot->data) | ch_event_mask);
			}
		}

		// make new-slot!!
		new_slot_cnt = timeout_cnt - remain_slot_cnt;

		if (new_slot_cnt)
		{
			for (i = 0; i < new_slot_cnt; i++)
			{
				ivf.popup.slot = g_slist_append(ivf.popup.slot, GUINT_TO_POINTER(ch_event_mask));
			}
		}
#endif
		slot_data = GPOINTER_TO_UINT(g_slist_nth_data(ivf.popup.slot, 0));
		_release_ignor(slot_data);

		if(remain_slot_cnt)
		{
			// remove all slots
			for (i = 0; i < remain_slot_cnt; i++)
				ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, i));
		}

		for (i = 0; i < timeout_cnt; i++)
		{
			ivf.popup.slot = g_slist_append(ivf.popup.slot, GUINT_TO_POINTER(ch_event_mask));
		}

	}
	else	// until key
	{
		if(remain_slot_cnt)
		{
			// remove all slots
			for (i = 0; i < remain_slot_cnt; i++)
				ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, i));
		}

		// make one new-slot!!
		ivf.popup.slot = g_slist_append(ivf.popup.slot, GUINT_TO_POINTER(ch_event_mask));
	}

	// insert sfc recover slot!!
	if(ori_ch_mask == 0)
		ivf.popup.slot = g_slist_append(ivf.popup.slot, GUINT_TO_POINTER(_vsm_get_screen_ch_mask()));
	else
		ivf.popup.slot = g_slist_append(ivf.popup.slot, GUINT_TO_POINTER(ori_ch_mask));

	if(ori_ch_mask == 0)
		_vsm_copy_current_sfc(&ivf.popup.last_sfc);

	g_mutex_unlock (ivf.popup.rw_lock);
}

static void _empty_all_popup_slot()
{
	guint i, remain_slot_cnt;

	g_mutex_lock (ivf.popup.rw_lock);

	remain_slot_cnt = g_slist_length(ivf.popup.slot);

	if (remain_slot_cnt)
	{
		for( i = remain_slot_cnt; i > 0 ; i--)
		{
			ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, i-1));
		}

		// empty ignor ch
		memset(ivf.popup.ignor_ch, 0x00, sizeof(ivf.popup.ignor_ch));
	}

	g_mutex_unlock (ivf.popup.rw_lock);
}

static void _empty_event_popup_slot()
{
	gint i, remain_slot_cnt;

	g_mutex_lock (ivf.popup.rw_lock);

	remain_slot_cnt = g_slist_length(ivf.popup.slot);

	if (remain_slot_cnt)
	{
		for( i = remain_slot_cnt - 2; i > 0 ; i--)
		{
			ivf.popup.slot = g_slist_delete_link(ivf.popup.slot, g_slist_nth(ivf.popup.slot, i-1));
		}

		// empty ignor ch
		memset(ivf.popup.ignor_ch, 0x00, sizeof(ivf.popup.ignor_ch));
	}

	g_mutex_unlock (ivf.popup.rw_lock);
}

static void _set_popup_div_mode(guint ch_event_mask)
{
    gchar win[VSM_WIN_MAX];
    guint dtype = 0;

	gint i;
	gint ch_event_cnt = 0;
	gint input_cnt = 0;
	gint remain_slot_cnt;


	if (ch_event_mask == 0)
		g_assert(0);

	memset(win, 0xff, sizeof(win));

	remain_slot_cnt = g_slist_length(ivf.popup.slot);

	if(remain_slot_cnt == 0)					// return div before events
	{
		dtype = ivf.popup.last_sfc.div;

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
			win[i] = ivf.popup.last_sfc.cinfo[i].win_id;
	}
	else 										// events div
	{
		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (ch_event_mask & (1 << i))
				ch_event_cnt++;
		}
#if defined(GUI_16CH_SUPPORT)
		if (ch_event_cnt > 1) 	dtype = VSM_DIV16;
#elif defined(GUI_8CH_SUPPORT)
		if (ch_event_cnt > 1) 	dtype = VSM_DIV9;
#elif defined(GUI_4CH_SUPPORT)
		if (ch_event_cnt > 1) 	dtype = VSM_DIV4;
#else
		if (ch_event_cnt > 1) 	dtype = VSM_DIV32;
#endif
		else 					dtype = VSM_DIV1;

		if(ch_event_cnt > 1)
		{
			for (i = 0; i < GUI_CHANNEL_CNT; i++)
				win[i] = i;
		}
		else
		{
			for (i = 0; i < GUI_CHANNEL_CNT; i++)
			{
				if (ch_event_mask & (1 << i))
				{
					win[i] = 0;
					break;
				}
			}
		}
	}

	vsm_change_sfc_by_array(win, dtype);
}

static gboolean _func_popup_timer(gpointer data)
{
	static guint slot_data;
	guint new_slot;

	if (!_get_popup_run())
		return TRUE;

	new_slot = _get_popup_slot_data();

	if(slot_data != new_slot)
	{
		slot_data = new_slot;

		if (slot_data == 0)
			return TRUE;

		_set_popup_div_mode(slot_data);
	}

	return TRUE;
}

static gboolean _is_ignor_ch(guint ch)
{
	if(!ivf.popup.ignor_ch[ch])
	{
		ivf.popup.ignor_ch[ch] = -1;

		return FALSE;
	}

	return TRUE;
}

static guint _get_covert_group()
{
	CameraData cd[GUI_CHANNEL_CNT];
	gchar lg[10];
	guint mask = 0;
	gint i;

	ssm_get_cur_group(lg);

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
		DAL_get_covert_data(&cd[i], i);

	if (!strcmp(lg, "ADMIN"))
	{
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
			if (cd[i].admin)		mask |= (1 << i);
	}
	else if (!strcmp(lg, "MANAGER"))
	{
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
			if (cd[i].manager)		mask |= (1 << i);
	}
	else		//USER
	{
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
			if (cd[i].user)		mask |= (1 << i);
	}

	return mask;
}

static void _get_covert_channel(gchar buf[33])
{
	gchar uc[33];
	guint mask = 0;
	gint i;

	DAL_get_user_covert(uc, (guint)ssm_get_cur_idx());
	mask = _get_covert_group();

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if ((uc[i] == '1') || (mask & (1 << i)))
			buf[i] = '1';
		else
			buf[i] = '0';
	}
}

void _print_ignor_ch()
{
	int i;
	for (i = 0; i < GUI_CHANNEL_CNT; ++i)
		printf("ignor_channel ch=%d, [%d]\n", i, ivf.popup.ignor_ch[i]);

}

static void _popup_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	gchar covert[32+1];

	guint i, j, type;

	guint event_data = 0;
	guint dwell;
	BITMASK64 linkMask = 0;


	type = GPOINTER_TO_UINT(data);

	if (!_get_popup_run())
		return;

    if(vwnd_switch_win_is_moved() || vwnd_switch_win_is_pressed())
        return;


	// camera alarm sensor/motion
	if(ssm_get_cur_idx() < 0) DAL_get_logoff_covert(covert);
	else 					  _get_covert_channel(covert);

	if (type == POPUP_MODE_ALARM)
	{
#ifndef GUI_32CH_SUPPORT	
        for (i = 0; i < CAM_ALARM_IN; i++)
        {
			if(pinfo->d.params[0] & (1 << i))
			{
                linkMask = DAL_get_almSen_linkCam(i);

				for(j = 0; j < GUI_CHANNEL_CNT; j++)
				{
					if(linkMask & (1 << j))
					{
						if ((ivf.popup.alm_prev_status & (1 << i)) == 0)
						{
							if ((DAL_get_almSen_vPop_boolean(i)) && (covert[j] == '0'))
							{
								event_data |= (1 << j);
								_release_ignor_ch(j);
							}
						}
					}
				}
    		}
        }

		for (i = 0; i < var_get_dvr_alarmIn_cnt(); i++)
		{
			if(pinfo->d.params[0] & (1 << (16 + i)))
			{
				linkMask = DAL_get_almSen_linkCam(GUI_CHANNEL_CNT + i);

				for(j = 0; j < GUI_CHANNEL_CNT; j++)
				{
					if(linkMask & (1 << j))
					{
                        if ((ivf.popup.alm_prev_status & (1 << (16 + i))) == 0)
                        {
							if ((DAL_get_almSen_vPop_boolean(i + GUI_CHANNEL_CNT)) && (covert[j] == '0'))
							{
								event_data |= (1 << j);
								_release_ignor_ch(j);
							}
                        }
					}
				}
			}
		}
#else
        for (i = 0; i < CAM_ALARM_IN; i++)
        {
			if(pinfo->d.params[1] & (1 << i))
			{
                linkMask = DAL_get_almSen_linkCam(i);

				for(j = 0; j < GUI_CHANNEL_CNT; j++)
				{
					if(linkMask & (1 << j))
					{
						if ((ivf.popup.alm_prev_status & (1 << i)) == 0)
						{
							if ((DAL_get_almSen_vPop_boolean(i)) && (covert[j] == '0'))
							{
								event_data |= (1 << j);
								_release_ignor_ch(j);
							}
						}
					}
				}
    		}
        }

		for (i = 0; i < var_get_dvr_alarmIn_cnt(); i++)
		{
			if(pinfo->d.params[0] & (1 << i))
			{
				linkMask = DAL_get_almSen_linkCam(i + 32);

				for(j = 0; j < GUI_CHANNEL_CNT; j++)
				{
					if(linkMask & (1 << j))
					{
                        if ((ivf.popup.alm_prev_status & (1 << (32 + i))) == 0)
                        {
							if ((DAL_get_almSen_vPop_boolean(i + 32)) && (covert[j] == '0'))
							{
								event_data |= (1 << j);
								_release_ignor_ch(j);
							}
                        }
					}
				}
			}
		}
#endif
	}
	else if (type == POPUP_MODE_MOTION)
	{
//		_print_ignor_ch();

    	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	    {
    		if(pinfo->d.params[0] & (1 << i))
    		{
                if ((ivf.popup.mot_prev_status & (1 << i)) == 0)
                {
        			if ((DAL_get_MotSen_vPop_boolean(i)) && (covert[i] == '0'))
        			{
        				if(!_is_ignor_ch(i))
        					event_data |= (1 << i);
        			}
        		}
    		}
	    }
	}
	else if (type == POPUP_MODE_VCA)
	{
		VCAPropData vca_prop;

		if (pinfo->d.params[0] >= GUI_CHANNEL_CNT) return;

		DAL_get_vca_prop_data(&vca_prop, pinfo->d.params[0]);

		if (!vca_prop.active) return;
		if (!DAL_get_analysis_vPop_boolean(pinfo->d.params[0])) return;
		if (covert[pinfo->d.params[0]] != '0') return;

		if (pinfo->d.params[1])
			event_data |= (1 << pinfo->d.params[0]);
	}
	else if (type == POPUP_MODE_DVA)
	{
		DVAPropData dva_prop;

		if (pinfo->d.params[0] >= GUI_CHANNEL_CNT) return;

		DAL_get_dva_prop_data(&dva_prop, pinfo->d.params[0]);

		if (!dva_prop.active) return;
		if (!DAL_get_analysis_vPop_boolean(pinfo->d.params[0])) return;
		if (covert[pinfo->d.params[0]] != '0') return;

		if (pinfo->d.params[1])
			event_data |= (1 << pinfo->d.params[0]);
	}
	else if (type == POPUP_MODE_DVABX)
	{
		DvaBxPropData dvabx_prop;

		if (pinfo->d.params[0] >= GUI_CHANNEL_CNT) return;

		DAL_get_dvabx_prop_data(&dvabx_prop, pinfo->d.params[0]);

		if (!dvabx_prop.active) return;
		if (!DAL_get_analysis_vPop_boolean(pinfo->d.params[0])) return;
		if (covert[pinfo->d.params[0]] != '0') return;

		if (pinfo->d.params[1])
			event_data |= (1 << pinfo->d.params[0]);
	}	
	else if (type == POPUP_MODE_POS)
	{
        NF_POS_TEXT_EVENT *pos_event;

        pos_event = (NF_POS_TEXT_EVENT*)pinfo->p.ptr;

		if (pos_event->ch >= GUI_CHANNEL_CNT) return;

        linkMask = DAL_get_posevent_linkCam(pos_event->ch);

		for(j=0; j<GUI_CHANNEL_CNT; j++)
		{
			if(linkMask & (1 << j))
			{
				if ((DAL_get_posevent_vPop_boolean(pos_event->ch)) && (covert[j] == '0'))
				{
					event_data |= (1 << j);
					_release_ignor_ch(j);
				}
			}
		}
	}

	if (!event_data) return;

	dwell = DAL_get_evtNoti_vPop_duration();

	if (dwell > POP_ON_MAX_S)
		_set_popup_run_until_key(TRUE);
	else
		_set_popup_run_until_key(FALSE);

	_put_popup_slot_data(event_data, dwell);
}

static void _func_popup_init()
{
	ivf.popup.rw_lock = g_mutex_new();

#if 0
	ivf.popup.alm_cb = nf_notify_connect_cb("sensor", _popup_notify_cb, GUINT_TO_POINTER(POPUP_MODE_ALARM));

	if(!ivf.popup.alm_cb)
	{
		DMSG(1, "%s, %d alarm notify callback error", __FUNCTION__, __LINE__);
		g_assert(0);
	}

	ivf.popup.mot_cb = nf_notify_connect_cb("motion", _popup_notify_cb, GUINT_TO_POINTER(POPUP_MODE_MOTION));

	if(!ivf.popup.mot_cb)
	{
		DMSG(1, "%s, %d motion notify callback error", __FUNCTION__, __LINE__);
		g_assert(0);
	}
#endif
}

////////////////////////////////////////////////////////////
//
// 	public interfaces
//

gint _vsm_func_start(void)
{
//	memset(&ivf.seq.data, 0x00, sizeof(SeqData));
    memset(&ivf, 0x00, sizeof(FUNC_T));
	ivf.seq.tid = 0;
	ivf.seq.next = -1;
	ivf.seq.dwell = 0;

	_func_popup_init();
	_set_popup_run(TRUE);

	g_timeout_add(1000, _func_popup_timer, NULL);

	return 0;
}

gint vsm_func_send_motion_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_MOTION));
	ivf.popup.mot_prev_status = data->d.params[0];
	return 0;
}

gint vsm_func_send_alarm_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_ALARM));
	DMSG(1, "pre alm_prev_status : %llu", ivf.popup.alm_prev_status);
	
	ivf.popup.alm_prev_status &= ~(ivf.popup.alm_prev_status);
	ivf.popup.alm_prev_status |= (data->d.params[0] << 32);
	ivf.popup.alm_prev_status |= (data->d.params[1]);

	DMSG(1, "post alm_prev_status : %llu", ivf.popup.alm_prev_status);
	return 0;
}

gint vsm_func_send_vca_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_VCA));
	return 0;
}

gint vsm_func_send_dva_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_DVA));
	return 0;
}

gint vsm_func_send_dvabx_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_DVABX));
	return 0;
}

gint vsm_func_send_pos_status(NF_NOTIFY_INFO *data)
{
	_popup_notify_cb(data, GUINT_TO_POINTER(POPUP_MODE_POS));
	return 0;
}

gint _vsm_func_run_popup(void)
{
	if (vsm_get_vmode() == VMODE_LV)
	{
		_set_popup_run(TRUE);
		return 0;
	}

	return -1;
}

gint _vsm_func_stop_popup(void)
{
	if(_get_popup_run())
	{
		_set_popup_run(FALSE);
		_empty_all_popup_slot();
		return 0;
	}

	return -1;
}

gint _vsm_func_stop_event_popup(void)
{
	if(_get_popup_run())
	{
		_set_popup_run(FALSE);
		_empty_event_popup_slot();
		return 0;
	}

	return -1;
}

gint _vsm_func_popup_is_until_key()
{
	guint remain_slot_cnt;

	remain_slot_cnt = g_slist_length(ivf.popup.slot);
	if (_get_popup_run_until_key() && remain_slot_cnt) return 0;

	return -1;
}

gint _vsm_func_recover_popup()
{
	_empty_all_popup_slot();
	_set_popup_div_mode(0xffff);
}

gboolean _vsm_func_get_popup_run()
{
    return _get_popup_run();
}
