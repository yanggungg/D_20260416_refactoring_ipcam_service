// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/03/15 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_sys_event_vca.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include <string.h>
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nffixed.h"
#include "objects/nfimglabel.h"
#include "objects/nflabel.h"
#include "objects/nfobject.h"
#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "vw_channel_mask_ctrl.h"
#include "vw_evt_act_main.h"
#include "vw_sys_event_vca.h"
#include "vw_tools.h"
#include "scm.h"

#include "ivca_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFWINDOW *g_curwnd = NULL;

static NFOBJECT *g_fxd;
static NFOBJECT *g_lbl_cn;
static NFOBJECT *g_lbl_id[16];
static NFOBJECT *g_lbl_ao[16];
static NFOBJECT *g_btn_ao[16];
static NFOBJECT *g_fxd_bz[16];
static NFOBJECT *g_ckb_bz[16];
static NFOBJECT *g_lbl_bz[16];
static NFOBJECT *g_fxd_vp[16];
static NFOBJECT *g_ckb_vp[16];
static NFOBJECT *g_lbl_vp[16];
static NFOBJECT *g_fxd_em[16];
static NFOBJECT *g_ckb_em[16];
static NFOBJECT *g_lbl_em[16];

static guint g_channel;
static guint g_rt;

static NFOBJECT *g_cmb_ch;
static NFOBJECT *g_cmb_rt;

static NFOBJECT *btn_cancel;
static NFOBJECT *btn_apply;
static NFOBJECT *btn_close;

static gchar *g_str_ch[GUI_CHANNEL_CNT + 1];

static EA_VCAData g_vad[GUI_CHANNEL_CNT];
static EA_VCAData g_vad_org[GUI_CHANNEL_CNT];
static ivca_rule_t g_vrd[GUI_CHANNEL_CNT];

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static void
_init_data(void)
{
//captainnn
	//DAL_get_vca_act_data_all(g_vad, GUI_CHANNEL_CNT);
	memcpy(g_vad_org, g_vad, sizeof(g_vad));
	/* We just need rule data. */
	DAL_get_vca_setup_data_all(NULL, g_vrd, GUI_CHANNEL_CNT);
}

static void
_save_data(void)
{
//captainnn
	//DAL_set_vca_act_data_all(g_vad, GUI_CHANNEL_CNT);
	memcpy(g_vad_org, g_vad, sizeof(g_vad));
	scm_put_log(CHANGE_EVT_VCA, 0, 0);
	event_act_data_changed(TRUE);
}

static void
_show_rows(guint rowcnt)
{
	guint i;

	for (i = 0; i < rowcnt; i++) {
		nfui_nfobject_show(g_lbl_id[i]);
		nfui_nfobject_show(g_lbl_ao[i]);
		nfui_nfobject_show(g_btn_ao[i]);
		nfui_nfobject_show(g_fxd_bz[i]);
		nfui_nfobject_show(g_ckb_bz[i]);
		nfui_nfobject_show(g_lbl_bz[i]);
		nfui_nfobject_show(g_fxd_vp[i]);
		nfui_nfobject_show(g_ckb_vp[i]);
		nfui_nfobject_show(g_lbl_vp[i]);
		nfui_nfobject_show(g_fxd_em[i]);
		nfui_nfobject_show(g_ckb_em[i]);
		nfui_nfobject_show(g_lbl_em[i]);
	}
	for ( ; i < 16; i++) {
		nfui_nfobject_hide(g_lbl_id[i]);
		nfui_nfobject_hide(g_lbl_ao[i]);
		nfui_nfobject_hide(g_btn_ao[i]);
		nfui_nfobject_hide(g_fxd_bz[i]);
		nfui_nfobject_hide(g_ckb_bz[i]);
		nfui_nfobject_hide(g_lbl_bz[i]);
		nfui_nfobject_hide(g_fxd_vp[i]);
		nfui_nfobject_hide(g_ckb_vp[i]);
		nfui_nfobject_hide(g_lbl_vp[i]);
		nfui_nfobject_hide(g_fxd_em[i]);
		nfui_nfobject_hide(g_ckb_em[i]);
		nfui_nfobject_hide(g_lbl_em[i]);
	}
}

static void conv_alarmOut_data_to_string(guint data, gchar str[])
{
	gint i, j;

#if defined(_HDI_MODEL_UX)
	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		if(data & (1 << i)) {
			j += sprintf(&str[j], "%sA%d", !j ? "" : ",", i+1);
		}
	}
#else
	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		if(data & (1 << i)) {
			if(i < CAM_ALARM_OUT)
				j += sprintf(&str[j], "%s%d", !j ? "" : ",", i+1);
			else
				j += sprintf(&str[j], "%sA%d", !j ? "" : ",", i-CAM_ALARM_OUT+1);
		}
	}
#endif

	if(j == 0) g_sprintf(str, "%s", "N/A");
}

static void
_set_data(gboolean expose)
{
	gint nr, on_bz, on_vp, on_em;
	gint id;
	guint ch, i, n, mask;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( g_channel == GUI_CHANNEL_CNT ) {
		n = GUI_CHANNEL_CNT;
		/* Hide rule selection combo. */
		nfui_nfobject_show(g_lbl_cn);
		nfui_nfobject_hide(g_cmb_rt);

		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
			nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
			/* Compute OR'd mask and active count for the channel. */
			mask = 0;
			on_bz = on_vp = on_em = 0;
			for (i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				mask |= g_vad[ch].zone[id].almOut;
				on_bz += g_vad[ch].zone[id].buzzer;
				on_vp += g_vad[ch].zone[id].vpop;
				on_em += g_vad[ch].zone[id].email;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				mask |= g_vad[ch].cntr[id].almOut;
				on_bz += g_vad[ch].cntr[id].buzzer;
				on_vp += g_vad[ch].cntr[id].vpop;
				on_em += g_vad[ch].cntr[id].email;
			}

			sprintf(buf, "CH%u", ch + 1);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_id[ch], buf);

			conv_alarmOut_data_to_string(mask, buf);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[ch], buf);

			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_bz[ch], on_bz > 0);
			sprintf(buf, "%u/%u", on_bz, nr);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_bz[ch], buf);

			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_vp[ch], on_vp > 0);
			sprintf(buf, "%u/%u", on_vp, nr);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_vp[ch], buf);

			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_em[ch], on_em > 0);
			sprintf(buf, "%u/%u", on_em, nr);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_em[ch], buf);
		}
	}
	else {
		ch = g_channel;
		n = g_rt ? (guint)g_vrd[ch].ncntrs : g_vrd[ch].nzones;
		pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
		/* Show rule selection combo. */
		nfui_nfobject_hide(g_lbl_cn);
		nfui_nfobject_show(g_cmb_rt);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX *)g_cmb_rt, g_rt);

		for ( i = 0; i < n; i++) {
			id = g_rt ? g_vrd[ch].cntrlist[i].id : g_vrd[ch].zonelist[i].id;
			sprintf(buf, "ID%u", id);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_id[i], buf);

			conv_alarmOut_data_to_string(pe[id].almOut, buf);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[i], buf);
			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_bz[i], pe[id].buzzer);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_bz[i], "");
			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_vp[i], pe[id].vpop);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_vp[i], "");
			nfui_check_button_set_active_no_expose(
					(NFCHECKBUTTON *)g_ckb_em[i], pe[id].email);
			nfui_nflabel_set_text((NFLABEL *)g_lbl_em[i], "");
		}
	}
	_show_rows(n);

	if ( expose )
		nfui_signal_emit(g_fxd, GDK_EXPOSE, TRUE);
}

static gboolean
post_cmb_channel_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint channel;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		channel = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);
		if ( g_channel == channel )
			return FALSE;

#if defined(_HDY_0818)|| defined(_HDY_1618)
		if ( channel != GUI_CHANNEL_CNT &&
				(channel < NUM_HD_CH || channel >= NUM_HD_CH + 4) ) {
			nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
					"The channel cannot be selected.", NFTOOL_MB_OK);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, g_channel);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
#endif

		g_channel = channel;
		_set_data(TRUE);
	}
	return FALSE;
}

static gboolean
post_cmb_ruletype_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint rt;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		rt = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);
		if ( g_rt == rt )
			return FALSE;

		g_rt = rt;
		_set_data(TRUE);
	}
	return FALSE;
}

static gboolean
post_btn_ao_all_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint x, y, dcnt;
	gint id;
	guint ch, i, n, mask;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		mask = ALARM_OUT_MASK_TYPE;
		nfui_nfobject_get_offset(obj, &x, &y);

		if ( !VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL",
				x - 151, y + obj->height, &mask) )
			return FALSE;

		conv_alarmOut_data_to_string(mask, buf);

		if ( g_channel == GUI_CHANNEL_CNT ) {
			for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
				for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
					id = g_vrd[ch].zonelist[i].id;
					dcnt += g_vad[ch].zone[id].almOut != mask;
					g_vad[ch].zone[id].almOut = mask;
				}
				for (i = 0; i < g_vrd[ch].ncntrs; i++) {
					id = g_vrd[ch].cntrlist[i].id;
					dcnt += g_vad[ch].cntr[id].almOut != mask;
					g_vad[ch].cntr[id].almOut = mask;
				}
				if ( dcnt ) {
					nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[ch], buf);
					nfui_signal_emit(g_lbl_ao[ch], GDK_EXPOSE, FALSE);
				}
			}
		}
		else {
			ch = g_channel;
			n = g_rt ? (guint)g_vrd[ch].ncntrs : g_vrd[ch].nzones;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			for (i = 0; i < n; i++) {
				id = g_rt ? g_vrd[ch].cntrlist[i].id : g_vrd[ch].zonelist[i].id;
				if ( mask != pe[id].almOut ) {
					pe[id].almOut = mask;
					nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[i], buf);
					nfui_signal_emit(g_lbl_ao[i], GDK_EXPOSE, FALSE);
				}
			}
		}
	}
	return FALSE;
}

static gboolean
post_btn_buzzer_all_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, dcnt;
	gint id;
	guint ch, i, n;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		val = nfui_combobox_get_cur_index((NFCOMBOBOX *)obj) > 0;

		if ( g_channel == GUI_CHANNEL_CNT ) {
			for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
				nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
				v = nr > 0 && val;
				for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
					id = g_vrd[ch].zonelist[i].id;
					dcnt += g_vad[ch].zone[id].buzzer != v;
					g_vad[ch].zone[id].buzzer = v;
				}
				for (i = 0; i < g_vrd[ch].ncntrs; i++) {
					id = g_vrd[ch].cntrlist[i].id;
					dcnt += g_vad[ch].cntr[id].buzzer != v;
					g_vad[ch].cntr[id].buzzer = v;
				}

				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_bz[ch], v);
				if ( dcnt ) {
					sprintf(buf, "%u/%u", v ? nr : 0, nr);
					nfui_nflabel_set_text((NFLABEL *)g_lbl_bz[ch], buf);
					nfui_signal_emit(g_lbl_bz[ch], GDK_EXPOSE, FALSE);
				}
			}
		}
		else {
			ch = g_channel;
			n = g_rt ? (guint)g_vrd[ch].ncntrs : g_vrd[ch].nzones;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			for (i = 0; i < n; i++) {
				id = g_rt ? g_vrd[ch].cntrlist[i].id : g_vrd[ch].zonelist[i].id;
				pe[id].buzzer = val;
				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_bz[i], val);
			}
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		//TODO
	}
	return FALSE;
}

static gboolean
post_btn_vpop_all_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, dcnt;
	gint id;
	guint ch, i, n;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		val = nfui_combobox_get_cur_index((NFCOMBOBOX *)obj) > 0;

		if ( g_channel == GUI_CHANNEL_CNT ) {
			for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
				nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
				v = nr > 0 && val;
				for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
					id = g_vrd[ch].zonelist[i].id;
					dcnt += g_vad[ch].zone[id].vpop != v;
					g_vad[ch].zone[id].vpop = v;
				}
				for (i = 0; i < g_vrd[ch].ncntrs; i++) {
					id = g_vrd[ch].cntrlist[i].id;
					dcnt += g_vad[ch].cntr[id].vpop != v;
					g_vad[ch].cntr[id].vpop = v;
				}

				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_vp[ch], v);
				if ( dcnt ) {
					sprintf(buf, "%u/%u", v ? nr : 0, nr);
					nfui_nflabel_set_text((NFLABEL *)g_lbl_vp[ch], buf);
					nfui_signal_emit(g_lbl_vp[ch], GDK_EXPOSE, FALSE);
				}
			}
		}
		else {
			ch = g_channel;
			n = g_rt ? (guint)g_vrd[ch].ncntrs : g_vrd[ch].nzones;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			for (i = 0; i < n; i++) {
				id = g_rt ? g_vrd[ch].cntrlist[i].id : g_vrd[ch].zonelist[i].id;
				pe[id].vpop = val;
				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_vp[i], val);
			}
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		//TODO
	}
	return FALSE;
}

static gboolean
post_btn_email_all_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, dcnt;
	gint id;
	guint ch, i, n;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		val = nfui_combobox_get_cur_index((NFCOMBOBOX *)obj) > 0;

		if ( g_channel == GUI_CHANNEL_CNT ) {
			for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
				nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
				v = nr > 0 && val;
				for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
					id = g_vrd[ch].zonelist[i].id;
					dcnt += g_vad[ch].zone[id].email != v;
					g_vad[ch].zone[id].email = v;
				}
				for (i = 0; i < g_vrd[ch].ncntrs; i++) {
					id = g_vrd[ch].cntrlist[i].id;
					dcnt += g_vad[ch].cntr[id].email != v;
					g_vad[ch].cntr[id].email = v;
				}

				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_em[ch], v);
				if ( dcnt ) {
					sprintf(buf, "%u/%u", v ? nr : 0, nr);
					nfui_nflabel_set_text((NFLABEL *)g_lbl_em[ch], buf);
					nfui_signal_emit(g_lbl_em[ch], GDK_EXPOSE, FALSE);
				}
			}
		}
		else {
			ch = g_channel;
			n = g_rt ? (guint)g_vrd[ch].ncntrs : g_vrd[ch].nzones;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			for (i = 0; i < n; i++) {
				id = g_rt ? g_vrd[ch].cntrlist[i].id : g_vrd[ch].zonelist[i].id;
				pe[i].email = val;
				nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_em[i], val);
			}
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		//TODO
	}
	return FALSE;
}

static gboolean
post_btn_ao_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint x, y, dcnt;
	gint id;
	guint ch, i, row, mask;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		row = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "rowidx"));

		if ( g_channel == GUI_CHANNEL_CNT ) {
			ch = row;
			mask = ALARM_OUT_MASK_TYPE;
		}
		else {
			ch = g_channel;
			id = g_rt ? g_vrd[ch].cntrlist[row].id : g_vrd[ch].zonelist[row].id;
			if ( g_rt )
				mask = g_vad[ch].cntr[id].almOut | ALARM_OUT_MASK_TYPE;
			else
				mask = g_vad[ch].zone[id].almOut | ALARM_OUT_MASK_TYPE;
		}

		nfui_nfobject_get_offset(obj, &x, &y);

		if ( !VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL",
				x - 151, y + obj->height, &mask) )
			return FALSE;

		if ( g_channel == GUI_CHANNEL_CNT ) {
			for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				dcnt += g_vad[ch].zone[id].almOut != mask;
				g_vad[ch].zone[id].almOut = mask;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				dcnt += g_vad[ch].cntr[id].almOut != mask;
				g_vad[ch].cntr[id].almOut = mask;
			}
			if ( dcnt ) {
				conv_alarmOut_data_to_string(mask, buf);
				nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[ch], buf);
				nfui_signal_emit(g_lbl_ao[ch], GDK_EXPOSE, FALSE);
			}
		}
		else {
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			id = g_rt ? g_vrd[ch].cntrlist[row].id : g_vrd[ch].zonelist[row].id;
			if ( mask != pe[id].almOut ) {
				pe[id].almOut = mask;
				conv_alarmOut_data_to_string(mask, buf);
				nfui_nflabel_set_text((NFLABEL *)g_lbl_ao[row], buf);
				nfui_signal_emit(g_lbl_ao[row], GDK_EXPOSE, FALSE);
			}
		}
	}
	return FALSE;
}

static gboolean
post_ckbtn_buzzer_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, on, dcnt;
	gint id;
	guint ch, i, row;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		row = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "rowidx"));
		val = nfui_check_button_get_active((NFCHECKBUTTON *)obj);

		if ( g_channel == GUI_CHANNEL_CNT ) {
			ch = row;
			nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
			for (on = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				on += g_vad[ch].zone[id].buzzer;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				on += g_vad[ch].cntr[id].buzzer;
			}
			v = nr > 0 && (val || on < nr);

			for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				dcnt += g_vad[ch].zone[id].buzzer != v;
				g_vad[ch].zone[id].buzzer = v;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				dcnt += g_vad[ch].cntr[id].buzzer != v;
				g_vad[ch].cntr[id].buzzer = v;
			}

			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_bz[ch], v);
			if ( dcnt ) {
				sprintf(buf, "%u/%u", v ? nr : 0, nr);
				nfui_nflabel_set_text((NFLABEL *)g_lbl_bz[ch], buf);
				nfui_signal_emit(g_lbl_bz[ch], GDK_EXPOSE, FALSE);
			}
		}
		else {
			ch = g_channel;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			id = g_rt ? g_vrd[ch].cntrlist[row].id : g_vrd[ch].zonelist[row].id;
			pe[id].buzzer = val;
			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_bz[row], val);
		}
	}
	return FALSE;
}

static gboolean
post_ckbtn_vpop_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, on, dcnt;
	gint id;
	guint ch, i, row;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		row = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "rowidx"));
		val = nfui_check_button_get_active((NFCHECKBUTTON *)obj);

		if ( g_channel == GUI_CHANNEL_CNT ) {
			ch = row;
			nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
			for (on = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				on += g_vad[ch].zone[id].vpop;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				on += g_vad[ch].cntr[id].vpop;
			}
			v = nr > 0 && (val || on < nr);

			for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				dcnt += g_vad[ch].zone[id].vpop != v;
				g_vad[ch].zone[id].vpop = v;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				dcnt += g_vad[ch].cntr[id].vpop != v;
				g_vad[ch].cntr[id].vpop = v;
			}

			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_vp[ch], v);
			if ( dcnt ) {
				sprintf(buf, "%u/%u", v ? nr : 0, nr);
				nfui_nflabel_set_text((NFLABEL *)g_lbl_vp[ch], buf);
				nfui_signal_emit(g_lbl_vp[ch], GDK_EXPOSE, FALSE);
			}
		}
		else {
			ch = g_channel;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			id = g_rt ? g_vrd[ch].cntrlist[row].id : g_vrd[ch].zonelist[row].id;
			pe[id].vpop = val;
			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_vp[row], val);
		}
	}
	return FALSE;
}

static gboolean
post_ckbtn_email_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint nr, on, dcnt;
	gint id;
	guint ch, i, row;
	gboolean val, v;
	gchar buf[64];
	EA_VCAElem *pe;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		row = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "rowidx"));
		val = nfui_check_button_get_active((NFCHECKBUTTON *)obj);

		if ( g_channel == GUI_CHANNEL_CNT ) {
			ch = row;
			nr = g_vrd[ch].nzones + g_vrd[ch].ncntrs;
			for (on = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				on += g_vad[ch].zone[id].email;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				on += g_vad[ch].cntr[id].email;
			}
			v = nr > 0 && (val || on < nr);

			for (dcnt = 0, i = 0; i < g_vrd[ch].nzones; i++) {
				id = g_vrd[ch].zonelist[i].id;
				dcnt += g_vad[ch].zone[id].email != v;
				g_vad[ch].zone[id].email = v;
			}
			for (i = 0; i < g_vrd[ch].ncntrs; i++) {
				id = g_vrd[ch].cntrlist[i].id;
				dcnt += g_vad[ch].cntr[id].email != v;
				g_vad[ch].cntr[id].email = v;
			}

			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_em[ch], v);
			if ( dcnt ) {
				sprintf(buf, "%u/%u", v ? nr : 0, nr);
				nfui_nflabel_set_text((NFLABEL *)g_lbl_em[ch], buf);
				nfui_signal_emit(g_lbl_em[ch], GDK_EXPOSE, FALSE);
			}
		}
		else {
			ch = g_channel;
			pe = g_rt ? g_vad[ch].cntr : g_vad[ch].zone;
			id = g_rt ? g_vrd[ch].cntrlist[row].id : g_vrd[ch].zonelist[row].id;
			pe[id].email = val;
			nfui_check_button_set_active((NFCHECKBUTTON *)g_ckb_em[row], val);
		}
	}
	return FALSE;
}

static gboolean
post_nmbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		if ( obj == btn_cancel ) {
			memcpy(g_vad, g_vad_org, sizeof(g_vad));
			_set_data(TRUE);
		}
		else if ( obj == btn_apply ) {
			if ( memcmp(g_vad, g_vad_org, sizeof(g_vad)) ) {
				_save_data();
				nftool_mbox_auto(g_curwnd, 1, "NOTICE",
						"Configuration has beed saved.");
			}
		}
		else if ( obj == btn_close ) {
			mb_type ret;

			if ( memcmp(g_vad, g_vad_org, sizeof(g_vad)) ) {
				ret = nftool_mbox(g_curwnd, "CONFIRM",
						"Configuration has been changed.\n"
						"Do you want to save?", NFTOOL_MB_OKCANCEL);

				if ( ret == NFTOOL_MB_OK )
					_save_data();
			}
			VW_Evt_Act_Destroy(obj);
		}
	}
	return FALSE;
}

static gboolean
post_page_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_DELETE ) {
		gint i;

		for (i = 0; i < GUI_CHANNEL_CNT + 1; i++) {
			if ( g_str_ch[i] ) {
				free(g_str_ch[i]);
				g_str_ch[i] = NULL;
			}
		}

		g_curwnd = 0;
	}
	return FALSE;
}

static NFOBJECT *_create_button(NFOBJECT *parent, GdkPixbuf *img[],
		gint w, gint h, gint x, gint y, guint show, gpointer postcb)
{
	NFOBJECT *btn;

	btn = (NFOBJECT *)nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON *)btn, img);
	nfui_nfobject_set_size(btn, w, h);
	if ( show )
		nfui_nfobject_show(btn);
	nfui_nffixed_put((NFFIXED *)parent, btn, (guint)x, (guint)y);
	if ( postcb )
		nfui_regi_post_event_callback(btn, postcb);
	return btn;
}

void
VW_VCAEvt_init_page(NFOBJECT *parent)
{
	static gchar *str_rt[2] = {"ZONE", "COUNTER"};
	static gchar *strCol[] = {
			"CAM", "ALARM OUT", "BUZZER", "VIDEO POPUP", "EMAIL"
	};
	static gchar *strOnOff[] = {"OFF", "ON"};
	static gpointer ckb_all_cb[3] = {
			post_btn_buzzer_all_cb, post_btn_vpop_all_cb, post_btn_email_all_cb
	};
	static gpointer ckb_cb[3] = {
		post_ckbtn_buzzer_cb, post_ckbtn_vpop_cb, post_ckbtn_email_cb
	};
	GdkPixbuf *dd_img1[NFOBJECT_STATE_COUNT];
	GdkPixbuf *dd_img2[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ck_img;
	guint i;
	gint j, y, dd_w, dd_h, ck_w, ck_h;

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	g_channel = GUI_CHANNEL_CNT;
	g_rt = 0;

	/* Load VCA act configuration. */
	_init_data();

	/* Initialize button images. */
	dd_img1[0] = nfui_get_image_from_file(IMG_N_DROPDOWN_01, NULL);
	dd_img1[1] = nfui_get_image_from_file(IMG_O_DROPDOWN_01, NULL);
	dd_img1[2] = nfui_get_image_from_file(IMG_P_DROPDOWN_01, NULL);
	dd_img1[3] = nfui_get_image_from_file(IMG_D_DROPDOWN_01, NULL);

	dd_img2[0] = nfui_get_image_from_file(IMG_N_DROPDOWN_02, NULL);
	dd_img2[1] = nfui_get_image_from_file(IMG_O_DROPDOWN_02, NULL);
	dd_img2[2] = nfui_get_image_from_file(IMG_P_DROPDOWN_02, NULL);
	dd_img2[3] = nfui_get_image_from_file(IMG_D_DROPDOWN_02, NULL);

	ck_img = nfui_get_image_from_file(IMG_CHECK_OFF_N, NULL);

	nfui_get_pixbuf_size(dd_img1[0], &dd_w, &dd_h);
	nfui_get_pixbuf_size(ck_img, &ck_w, &ck_h);

	/* Initialize combobox. */
	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		g_str_ch[i] = malloc(64);
		if ( g_str_ch[i] ) {
			j = sprintf(g_str_ch[i], "CH%u - ", i + 1);
			DAL_get_camera_title(&g_str_ch[i][j], i);
		}
	}
	g_str_ch[i] = strdup("ALL");

	g_fxd = vw_fixed_create(parent, 0, 1, MENU_V_INNER_X, MENU_V_INNER_Y,
			MENU_V_INNER_W, MENU_V_INNER_H, NULL);

	/* Top line of the table. */
	g_cmb_ch = vw_combo_create(g_fxd, g_str_ch, GUI_CHANNEL_CNT + 1,
			GUI_CHANNEL_CNT, NULL, 2, 1, 27, 13, 164, 40, post_cmb_channel_cb);

	vw_label_create(g_fxd, "ACTION", NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 0,
			0, 1, 116, 115, 193, 13, 674, 40, NULL);

	/* Line 2. */
	g_cmb_rt = vw_combo_create(g_fxd, str_rt, 2, 0, NULL, 2, 0,
			27, 13 + 41, 164, 40, post_cmb_ruletype_cb);

	g_lbl_cn = vw_label_create(g_fxd, strCol[0], NFFONT_MEDIUM_SEMI,
			NFALIGN_CENTER, 0, 0, 1, 116, 115, 27, 13 + 41, 164, 40, NULL);

	vw_label_create(g_fxd, strCol[1], NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 0,
			1, 1, 116, 115, 27 + 164 + 2, 13 + 41, 151, 40, NULL);
	_create_button(g_fxd, dd_img2, dd_w, dd_h, 27 + 164 + 2 + 151, 13 + 41, 1,
			post_btn_ao_all_cb);

	for (j = 0; j < 3; j++)
		vw_combo_create(g_fxd, strOnOff, 2, 0, strCol[j + 2], 2, 1,
				27 + 164 + 2 + 151 + 27 + 166 * j, 13 + 41, 164, 40,
				ckb_all_cb[j]);

	/* Next lines. */
	for (i = 0, y = 13 + 41 + 41; i < 16; i++, y += 41) {
		NFOBJECT **fxd[3] = {&g_fxd_bz[i], &g_fxd_vp[i], &g_fxd_em[i]};
		NFOBJECT **ckb[3] = {&g_ckb_bz[i], &g_ckb_vp[i], &g_ckb_em[i]};
		NFOBJECT **lbl[3] = {&g_lbl_bz[i], &g_lbl_vp[i], &g_lbl_em[i]};

		g_lbl_id[i] = vw_label_create(g_fxd, "", NFFONT_MEDIUM_SEMI,
				NFALIGN_CENTER, 0, 0, 0, 116, 115, 27, y, 164, 40, NULL);
		g_lbl_ao[i] = vw_label_create(g_fxd, "", NFFONT_SMALL_SEMI,
				NFALIGN_CENTER, 0, 1, 0, 129, 128, 164 + 29, y, 151, 40, NULL);
		g_btn_ao[i] = _create_button(g_fxd, dd_img1, dd_w, dd_h,
				27 + 164 + 2 + 151, y, 0, post_btn_ao_cb);
		nfui_nfobject_set_data(g_btn_ao[i], "rowidx", GUINT_TO_POINTER(i));

		for (j = 0; j < 3; j++) {
			*fxd[j] = vw_fixed_create(g_fxd, 128, 0,
					27 + 164 + 2 + 151 + 27 + 166 * j, y, 164, 40, NULL);
			*ckb[j] = vw_ckbutton_lb_create(*fxd[j], NULL, 1, 129,
					(164 - ck_w) / 2, (40 - ck_h) / 2, ckb_cb[j]);
			nfui_nfobject_set_data(*ckb[j], "rowidx", GUINT_TO_POINTER(i));
			*lbl[j] = vw_label_create(*fxd[j], "", NFFONT_SMALL_SEMI,
					NFALIGN_CENTER, 0, 0, 1, 129, 128,
					(164 + ck_w) / 2, 0, (164 - ck_w) / 2, 40, NULL);
		}
#if defined(_HDY_0818)|| defined(_HDY_1618)
		if ( i < NUM_HD_CH || i >= NUM_HD_CH + 4 ) {
			nfui_nfobject_disable(g_lbl_id[i]);
			nfui_nfobject_disable(g_lbl_ao[i]);
			nfui_nfobject_disable(g_btn_ao[i]);
			for (j = 0; j < 3; j++) {
				nfui_nfobject_disable(*fxd[j]);
				nfui_nfobject_disable(*ckb[j]);
				nfui_nfobject_disable(*lbl[j]);
			}
		}
#endif		
	}

	_set_data(TRUE);

	/* Common buttons. */
	btn_cancel = vw_nmbutton_create(parent, "CANCEL", 1, TRUE,
			MENU_V_BTN_R3_X, MENU_V_BTN_Y, MENU_BTN_WIDTH, post_nmbtn_cb);
	btn_apply = vw_nmbutton_create(parent, "APPLY", 1, TRUE,
			MENU_V_BTN_R2_X, MENU_V_BTN_Y, MENU_BTN_WIDTH, post_nmbtn_cb);
	btn_close = vw_nmbutton_create(parent, "CLOSE", 2, TRUE,
			MENU_V_BTN_R1_X, MENU_V_BTN_Y, MENU_BTN_WIDTH, post_nmbtn_cb);

	nfui_regi_post_event_callback(parent, post_page_cb);
}	/* VW_VCAEvt_init_page(... */

gboolean
VW_VCAEvt_tab_out_handler(void)
{
	mb_type ret;

	if ( !memcmp(g_vad, g_vad_org, sizeof(g_vad)) )
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\n"
			"Do you want to save?", NFTOOL_MB_OKCANCEL);

	if ( ret == NFTOOL_MB_OK )
		_save_data();
	else {
		/* Restore. */
		memcpy(g_vad, g_vad_org, sizeof(g_vad));
		_set_data(FALSE);
	}
	return FALSE;
}	/* VW_VCAEvt_tab_out_handler(... */

