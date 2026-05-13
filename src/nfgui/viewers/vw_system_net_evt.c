#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_system_evt.h"
#include "vw_system_net_evt.h"
#include "vw_channel_mask_ctrl.h"
#include "vw_evt_type_data.h"
#include "log.h"
#include "scm.h"

#include "nfdal.h"
#include "nf_sysman.h"

#define	ALARM_STRING_LENGTH	128

enum {
	ACTION_AOUT = 0,
	ACTION_BUZZER,
	ACTION_OSD,
	ACTION_EMAIL,
	ACTION_FTP,
#if defined(_SUPPORT_USER_PHONE)
	ACTION_MOBILE,
#endif
	ACTION_MOBILEPUSH,
	ACTION_MAX
};


static int support_mobilepush = 0;

static EA_SysNetData g_snd[SYS_NET_EVT_CNT];
static EA_SysNetData g_osnd[SYS_NET_EVT_CNT];

static EA_SysNetFailCnt g_snc;
static EA_SysNetFailCnt g_osnc;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_almObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_almcmbObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_buzzObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_osdObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_mailObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_ftpObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_mobileObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_mobilepushObj[SYS_NET_EVT_CNT];
static NFOBJECT *g_buzzAll;
static NFOBJECT *g_osdAll;
static NFOBJECT *g_mailAll;
static NFOBJECT *g_ftpAll;
static NFOBJECT *g_mobileAll;
static NFOBJECT *g_mobilepushAll;

static gchar *strCnt[] = {"Retry 5 times", "Retry 4 times", "Retry 3 times", "Retry 2 times", "Retry 1 more"};

static void init_sn_data();
static void set_sn_data();
static void set_data_to_obj(gint expose);
static void get_data_from_obj();

static void conv_alarmOut_data_to_string(guint64 data, gchar str[]);


static void init_sn_data()
{
	guint i;

	memset(g_snd, 0x00, sizeof(EA_SysNetData) * SYS_NET_EVT_CNT);
	memset(g_osnd, 0x00, sizeof(EA_SysNetData) * SYS_NET_EVT_CNT);
	for(i=0; i<SYS_NET_EVT_CNT; i++)
		DAL_get_SysNet_data(&g_snd[i], i);

	g_memmove(g_osnd, g_snd, sizeof(EA_SysNetData)*SYS_NET_EVT_CNT);


	memset(&g_snc, 0x00, sizeof(EA_SysNetFailCnt));
	memset(&g_osnc, 0x00, sizeof(EA_SysNetFailCnt));
	DAL_get_SysNet_FCount(&g_snc);

	g_memmove(&g_osnc, &g_snc, sizeof(EA_SysNetFailCnt));
}

static void set_sn_data()
{
	g_memmove(g_osnd, g_snd, sizeof(EA_SysNetData)*SYS_NET_EVT_CNT);
	DAL_set_SysNet_data_all(g_snd);

	g_memmove(&g_osnc, &g_snc, sizeof(EA_SysNetFailCnt));
	DAL_set_SysNet_FCount(g_snc);

	scm_put_log(CHANGE_EVT_SYS, 0, 0);
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state = -1;
	gint i;

	for (i = 0; i < SYS_NET_EVT_CNT; i++)
	{
	    if (nfui_nfobject_is_disabled(unit[i])) continue;

	    state = nfui_check_button_get_active((NFCHECKBUTTON*)unit[i]);

		if (!state)
		{
		    if (expose)
                nfui_check_button_set_active((NFCHECKBUTTON*)all, FALSE);
            else
                nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, FALSE);
            return 0;
        }
	}

	if (i == SYS_NET_EVT_CNT && state != -1)
	{
	    if (expose)
    	    nfui_check_button_set_active((NFCHECKBUTTON*)all, TRUE);
	    else
	        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, TRUE);
    }
    else
    {
	    if (expose)
            nfui_check_button_set_active((NFCHECKBUTTON*)all, FALSE);
        else
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, FALSE);
    }

    return 0;
}

static gint _set_all_chk_btn_all_event(gboolean expose)
{
	_set_all_chk_btn(g_buzzAll, g_buzzObj, expose);
	_set_all_chk_btn(g_osdAll, g_osdObj, expose);
	_set_all_chk_btn(g_mailAll, g_mailObj, expose);
	_set_all_chk_btn(g_ftpAll, g_ftpObj, expose);
#if defined(_SUPPORT_USER_PHONE)
	_set_all_chk_btn(g_mobileAll, g_mobileObj, expose);
#endif
    if(support_mobilepush)
		_set_all_chk_btn(g_mobilepushAll, g_mobilepushObj, expose);

	return 0;
}

static void set_data_to_obj(gint expose)
{
	gint i;
	gchar str[ALARM_STRING_LENGTH];

	for(i=0; i<SYS_NET_EVT_CNT; i++) {
		memset(str, 0x00, ALARM_STRING_LENGTH);
		conv_alarmOut_data_to_string(g_snd[i].almOut, str);

		nfui_nflabel_set_text((NFLABEL*)g_almObj[i], str);
		if (expose) nfui_signal_emit(g_almObj[i], GDK_EXPOSE, FALSE);

		if (expose)
		{
			nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], g_snd[i].buzzer);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_osdObj[i], g_snd[i].osd);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], g_snd[i].email);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], g_snd[i].ftp);
#if defined(_SUPPORT_USER_PHONE)
			nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], g_snd[i].mobile);
#endif
            if(support_mobilepush)
    			nfui_check_button_set_active((NFCHECKBUTTON*)g_mobilepushObj[i], g_snd[i].mobilepush);
		}
		else
		{
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_buzzObj[i], g_snd[i].buzzer);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_osdObj[i], g_snd[i].osd);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mailObj[i], g_snd[i].email);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ftpObj[i], g_snd[i].ftp);
#if defined(_SUPPORT_USER_PHONE)
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mobileObj[i], g_snd[i].mobile);
#endif
            if(support_mobilepush)
    			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mobilepushObj[i], g_snd[i].mobilepush);
		}
	}
	_set_all_chk_btn(g_buzzAll, g_buzzObj, expose);
	_set_all_chk_btn(g_osdAll, g_osdObj, expose);
	_set_all_chk_btn(g_mailAll, g_mailObj, expose);
	_set_all_chk_btn(g_ftpAll, g_ftpObj, expose);
#if defined(_SUPPORT_USER_PHONE)
	_set_all_chk_btn(g_mobileAll, g_mobileObj, expose);
#endif
    if(support_mobilepush)
		_set_all_chk_btn(g_mobilepushAll, g_mobilepushObj, expose);
}

static void get_data_from_obj()
{
	gint i;

	for(i=0; i<SYS_NET_EVT_CNT; i++) {
		g_snd[i].buzzer = nfui_check_button_get_active((NFCHECKBUTTON*)g_buzzObj[i]);
		g_snd[i].osd    = nfui_check_button_get_active((NFCHECKBUTTON*)g_osdObj[i]);
		g_snd[i].email  = nfui_check_button_get_active((NFCHECKBUTTON*)g_mailObj[i]);
		g_snd[i].ftp    = nfui_check_button_get_active((NFCHECKBUTTON*)g_ftpObj[i]);
#if defined(_SUPPORT_USER_PHONE)
		g_snd[i].mobile = nfui_check_button_get_active((NFCHECKBUTTON*)g_mobileObj[i]);
#endif
        if(support_mobilepush)
		g_snd[i].mobilepush = nfui_check_button_get_active((NFCHECKBUTTON*)g_mobilepushObj[i]);
	}
}

static void conv_alarmOut_data_to_string(guint64 data, gchar str[])
{
	guint i, j;
	gchar tmp[8];

#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		memset(tmp, 0x00, sizeof(tmp));
		if(data & (1 << i)) {
			if(j < 1) 	g_sprintf(tmp, "A%d", i+1);
			else 	  	g_sprintf(tmp, ",A%d", i+1);

			j++;
		}
		strcat(str, tmp);
	}
#else
	gint  bytes;

	for(i=0, j=0; i<ALARM_OUT_COUNT; i++) {
		if(data & (1LL << i)) {
			if(i < CAM_ALARM_OUT)
			{
				if(j < 1) bytes = g_sprintf(&str[j], "%d", i+1);
				else 	  bytes = g_sprintf(&str[j], ",%d", i+1);
			}
#if defined(_IPX_1648P4E) || defined(_IPX_0824P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			else if (i < CAM_ALARM_OUT+NVR_RELAY_OUT)
			{
					if(j < 1) bytes = g_sprintf(&str[j], "R%d", i-CAM_ALARM_OUT+1);
					else      bytes = g_sprintf(&str[j], ",R%d", i-CAM_ALARM_OUT+1);
			}
			else
			{
				if(j < 1) bytes = g_sprintf(&str[j], "A%d", i-CAM_ALARM_OUT-NVR_RELAY_OUT+1);
				else 	  bytes = g_sprintf(&str[j], ",A%d", i-CAM_ALARM_OUT-NVR_RELAY_OUT+1);
			}
#else
			else
			{
				if(j < 1) bytes = g_sprintf(&str[j], "A%d", i-CAM_ALARM_OUT+1);
				else 	  bytes = g_sprintf(&str[j], ",A%d", i-CAM_ALARM_OUT+1);
			}
#endif

			j += bytes;
		}
	}

#endif
	if(j == 0) g_sprintf(str, "%s", "N/A");
}

static gboolean post_ao_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		unsigned long long mask = ALARM_OUT_MASK_TYPE;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", (off_x - 151), off_y + obj->height, &mask))
			return FALSE;

		for(i=0; i<SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_almObj[i]) == FALSE)
			{
				if(mask != g_snd[i].almOut)
				{
					g_snd[i].almOut = mask;

					memset(str, 0x00, ALARM_STRING_LENGTH);
					conv_alarmOut_data_to_string(g_snd[i].almOut, str);

					nfui_nflabel_set_text((NFLABEL*)g_almObj[i], str);
					nfui_signal_emit(g_almObj[i], GDK_EXPOSE, FALSE);
				}
			}
		}
	}
	return FALSE;
}

static gboolean post_buzzer_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_buzzObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_buzzObj[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_buzzer_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
    	_set_all_chk_btn(g_buzzAll, g_buzzObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_buzzObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_buzzObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_buzzAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_buzzObj[i+1]);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_osd_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED){
        gint i;
        gboolean state;

        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_osdObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_osdObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_osdObj[0]);
			return TRUE;
		}
	}

    return FALSE;
}


static gboolean post_osd_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        _set_all_chk_btn(g_osdAll, g_osdObj, TRUE);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_osdObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_osdObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_osdAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_osdObj[i+1]);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_email_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_mailObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_mailObj[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_mail_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
    	_set_all_chk_btn(g_mailAll, g_mailObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		NFOBJECT *bottom;
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_mailObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_mailObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_mailAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_mailObj[i+1]);
				return TRUE;
			}
			else {
				bottom = nfui_nfobject_get_data(obj, "bottom focus");
				if(bottom) {
					change_obj_focus(obj, bottom);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_ftp_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_ftpObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_ftpObj[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_ftp_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
    	_set_all_chk_btn(g_ftpAll, g_ftpObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		NFOBJECT *bottom;
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_ftpObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_ftpObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_ftpAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_ftpObj[i+1]);
				return TRUE;
			}
			else {
				bottom = nfui_nfobject_get_data(obj, "bottom focus");
				if(bottom) {
					change_obj_focus(obj, bottom);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_mobile_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_mobileObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_mobileObj[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_mobile_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
    	_set_all_chk_btn(g_mobileAll, g_mobileObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		NFOBJECT *bottom;
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_mobileObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_mobileObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_mobileAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_mobileObj[i+1]);
				return TRUE;
			}
			else {
				bottom = nfui_nfobject_get_data(obj, "bottom focus");
				if(bottom) {
					change_obj_focus(obj, bottom);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_mobilepush_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < SYS_NET_EVT_CNT; i++)
		{
			if (nfui_nfobject_is_disabled((NFCHECKBUTTON*)g_mobilepushObj[i]) == FALSE)
			{
	            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobilepushObj[i], state);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_mobilepushObj[0]);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean post_mobilepush_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
    	_set_all_chk_btn(g_mobilepushAll, g_mobilepushObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		NFOBJECT *bottom;
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i=0; i<SYS_NET_EVT_CNT; i++) {
			if(g_mobilepushObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_mobilepushObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_mobilepushAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < SYS_NET_EVT_CNT) {
				change_obj_focus(obj, g_mobilepushObj[i+1]);
				return TRUE;
			}
			else {
				bottom = nfui_nfobject_get_data(obj, "bottom focus");
				if(bottom) {
					change_obj_focus(obj, bottom);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_ao_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		guint type;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "event type"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (ALARM_OUT_MASK_TYPE | g_snd[type].almOut);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", off_x, off_y + obj->height, &mask))
			return FALSE;

		if(mask != g_snd[type].almOut) {
			g_snd[type].almOut = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_alarmOut_data_to_string(g_snd[type].almOut, str);

			nfui_nflabel_set_text((NFLABEL*)obj, str);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_ao_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint type;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "event type"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (ALARM_OUT_MASK_TYPE | g_snd[type].almOut);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", (off_x - 151), off_y + obj->height, &mask))
			return FALSE;

		if(mask != g_snd[type].almOut) {
			g_snd[type].almOut = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_alarmOut_data_to_string(g_snd[type].almOut, str);

			nfui_nflabel_set_text((NFLABEL*)g_almObj[type], str);
			nfui_signal_emit(g_almObj[type], GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_remote_login_fail_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint off_x, off_y;
		gint ret = -1;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		ret = VW_EvtTypeData_Ctrl(g_curwnd, strCnt, 5, (5 - g_snc.rfail_cnt), (gint)(off_x - 218), (gint)(off_y + obj->height + 4));
		if(ret >= 0)
			g_snc.rfail_cnt = (5 - ret);
	}
	return FALSE;
}

static gboolean post_ddns_update_fail_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint off_x, off_y;
		gint ret = -1;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		ret = VW_EvtTypeData_Ctrl(g_curwnd, strCnt, 5, (5 - g_snc.dfail_cnt), (gint)(off_x - 218), (gint)(off_y + obj->height + 4));
		if(ret >= 0)
			g_snc.dfail_cnt = (5 - ret);
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		g_memmove(g_snd, g_osnd, sizeof(EA_SysNetData)*SYS_NET_EVT_CNT);
		g_memmove(&g_snc, &g_osnc, sizeof(EA_SysNetFailCnt));

		set_data_to_obj(1);
	}
	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		get_data_from_obj();

		if(memcmp(g_osnd, g_snd, sizeof(EA_SysNetData) * SYS_NET_EVT_CNT)
				|| memcmp(&g_osnc, &g_snc, sizeof(EA_SysNetFailCnt))) {
			set_sn_data();

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			event_act_data_changed(TRUE);
		}
	}
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		VW_SystemEvt_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

void VW_Init_SysNet_Evt_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *fixed_temp;
	NFOBJECT *tbl = NULL;

	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *dropdown_img2[NFOBJECT_STATE_COUNT];

	gchar *strEvtType[] = {"EVENT TYPE", "INTERNET CONNECTION TROUBLE EVENT", "REMOTE LOGIN FAIL EVENT", "DDNS UPDATE FAIL EVENT", "IP CONFLICT EVENT"};
	gchar *strOnOff[] = {"OFF", "ON"};
	gchar buf[128];

	gint size_w, size_h;
	guint pos_x, pos_y;
	guint i, j, ret;
	guint tbl_w[14] = {35, 170, 35, 170, 35, 170, 35, 170, 35, 170, 35, 170, 35, 170};

	guint action_w[ACTION_MAX] = {0,};
	guint action_pos_x;
	guint action_tot_w = 0;
	guint legend_tot_w = 0;
	guint legend_cnt = 0;

	g_curwnd = nfui_nfobject_get_top(parent);

	// init data
	init_sn_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	legend_cnt = ACTION_MAX-ACTION_BUZZER;

	for (i = 0; i < legend_cnt*2; i++)
		legend_tot_w += tbl_w[i];

	support_mobilepush = var_get_supported_mobilepush();


	// legend ////////////////////////////////////////////////////////////////////////////////////////
	tbl = (NFOBJECT*)nfui_nftable_new(legend_cnt*2, 1, 1, 1, tbl_w, 35);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 1485-legend_tot_w, 0);

	legend_cnt = 0;

    obj = nfui_nfimage_new(IMG_ACTION_BUZZER_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUZZER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

/*
    obj = nfui_nfimage_new(IMG_ACTION_VIDEO_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_OSD_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OSD POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
*/
    obj = nfui_nfimage_new(IMG_ACTION_OSD_03);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OSD POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_EMAIL_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_FTP_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FTP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

#if defined(_SUPPORT_USER_PHONE)
	obj = nfui_nfimage_new(IMG_ACTION_MOBILE_03);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOBILE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
#endif
	obj = nfui_nfimage_new(IMG_ACTION_MOBILE_PUSH_03);
	if(support_mobilepush)
    	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOBILE PUSH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	if(support_mobilepush)
    	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	//event type//////////////////////////////////////////////////////////////////////////
	dropdown_img2[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
	dropdown_img2[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
	dropdown_img2[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
	dropdown_img2[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);

	pos_x = 27;
	pos_y = 40;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strEvtType[0], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 460, 81);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += 82;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strEvtType[1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 460, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += 41;

	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strEvtType[2], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 460 - size_w, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img2);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_regi_post_event_callback(obj, post_remote_login_fail_btn_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x + 460 - size_w), pos_y);

	if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
		nfui_nfobject_disable(obj);

	pos_y += 41;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strEvtType[3], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 460 - size_w, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img2);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_regi_post_event_callback(obj, post_ddns_update_fail_btn_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x + 460 - size_w), pos_y);

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strEvtType[4], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_set_size(obj, 460, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	if (ivsc.dfunc.support_aibox_itx)
	{
	    pos_y += 41;

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI BOX FAILURE EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
		nfui_nfobject_set_size(obj, 460, 40);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128)) nfui_nfobject_show(obj);
		else nfui_nfobject_hide(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	}


	//action//////////////////////////////////////////////////////////////////////////
	pos_x += (460 + 2);
	action_pos_x = pos_x;
	pos_y = 40;

    nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

    action_w[ACTION_AOUT] = 150+size_w;
    action_w[ACTION_BUZZER] = 68;
    action_w[ACTION_OSD] =68;
    action_w[ACTION_EMAIL] = 68;
    action_w[ACTION_FTP] = 68;
#if defined(_SUPPORT_USER_PHONE)
    action_w[ACTION_MOBILE] = 68;
#endif
	if(support_mobilepush)
    	action_w[ACTION_MOBILEPUSH] = 68;

    for (i = 0; i < ACTION_MAX; i++)
        action_tot_w += action_w[i];

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, action_tot_w + 2*(ACTION_MAX-1), 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += (40 + 1);

	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

	for(i = 0; i < ACTION_MAX; i++) {
		if(i == ACTION_AOUT)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, action_w[ACTION_AOUT]-size_w, 40);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

            pos_x += (action_w[ACTION_AOUT]-size_w);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img2);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_regi_post_event_callback(obj, post_ao_all_btn_event_handler);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

			pos_x += (size_w + 2);
		}
		else
		{
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
			nfui_nfobject_show(fixed_temp);
			nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, (guint)pos_x, (guint)pos_y);

            if (i == ACTION_BUZZER)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_BUZZER_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_buzzer_all_event_handler);
                g_buzzAll = obj;

                /*for (j = 0; j < SYS_NET_EVT_CNT; j++)
                {
                    if (!g_snd[j].buzzer)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

    			obj = nfui_nfimage_new(IMG_ACTION_BUZZER_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
            else if (i == ACTION_OSD)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_OSD_01, &icon_size_w, &icon_size_h);

                obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
                nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj),NFCHECK_TYPE_SMALL);
                nfui_nfobject_show(obj);
                nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_osd_all_event_handler);
                g_osdAll = obj;

                /*for( j=0; j< ALARM_IN_COUNT; j++)
                {
                    if(!g_snd[j].osd)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

                obj = nfui_nfimage_new(IMG_ACTION_OSD_01);
                nfui_nfobject_show(obj);
                nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
            else if (i == ACTION_EMAIL)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_EMAIL_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_email_all_event_handler);
                g_mailAll = obj;

                /*for (j = 0; j < SYS_NET_EVT_CNT; j++)
                {
                    if (!g_snd[j].email)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

    			obj = nfui_nfimage_new(IMG_ACTION_EMAIL_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
            else if (i == ACTION_FTP)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_FTP_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_ftp_all_event_handler);
                g_ftpAll = obj;

                /*for (j = 0; j < SYS_NET_EVT_CNT; j++)
                {
                    if (!g_snd[j].ftp)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

    			obj = nfui_nfimage_new(IMG_ACTION_FTP_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
#if defined(_SUPPORT_USER_PHONE)
            else if (i == ACTION_MOBILE)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_MOBILE_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_mobile_all_event_handler);
                g_mobileAll = obj;

                /*for (j = 0; j < SYS_NET_EVT_CNT; j++)
                {
                    if (!g_snd[j].mobile)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

    			obj = nfui_nfimage_new(IMG_ACTION_MOBILE_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
#endif
            else if (i == ACTION_MOBILEPUSH)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_MOBILE_PUSH_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			if(support_mobilepush)
    			    nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_mobilepush_all_event_handler);
                g_mobilepushAll = obj;

                /*for (j = 0; j < SYS_NET_EVT_CNT; j++)
                {
                    if (!g_snd[j].mobilepush)
                        nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
                }*/

    			obj = nfui_nfimage_new(IMG_ACTION_MOBILE_PUSH_01);
    			if(support_mobilepush)
            	    nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }

    		pos_x += (action_w[i] + 2);
		}
	}

	// alarm out
	pos_x = action_pos_x;
	pos_y += (40 + 1);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	for(i=0; i<SYS_NET_EVT_CNT; i++) {
		memset(buf, 0x00, sizeof(buf));
		conv_alarmOut_data_to_string(g_snd[i].almOut, buf);

		g_almObj[i] = (NFOBJECT*)nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)g_almObj[i], NFTEXTBOX_TYPE_SUBTAB_INPUT);
		nfui_nflabel_set_align((NFLABEL*)g_almObj[i], NFALIGN_CENTER, 0);
		nfui_nflabel_use_strip((NFLABEL*)g_almObj[i], TRUE);
		nfui_nfobject_use_tooltip(g_almObj[i], FALSE);
		nfui_nfobject_set_size(g_almObj[i], (ALARM_OUT_COUNT >= 2 ? action_w[ACTION_AOUT]-size_w : action_w[ACTION_AOUT]), 40);
#if defined(_SUPPORT_AIBOX)
		if (i == AIBOX_TROUBLE_EVT_DATA)
		{
			if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128)) nfui_nfobject_show(g_almObj[i]);
			else nfui_nfobject_hide(g_almObj[i]);
		}
		else nfui_nfobject_show(g_almObj[i]);
#else
		nfui_nfobject_show(g_almObj[i]);
#endif
		nfui_nffixed_put((NFFIXED*)content_fixed, g_almObj[i], pos_x, (pos_y + (i * 41)));

		if(ALARM_OUT_COUNT >= 2) {
			nfui_nfobject_set_data(g_almObj[i], "event type", GUINT_TO_POINTER(i));
			nfui_regi_post_event_callback(g_almObj[i], post_ao_label_event_handler);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
#if defined(_SUPPORT_AIBOX)
			if (i == AIBOX_TROUBLE_EVT_DATA)
			{
				if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128)) nfui_nfobject_show(g_almObj[i]);
				else nfui_nfobject_hide(obj);
			}
			else nfui_nfobject_show(obj);
#else
			nfui_nfobject_show(g_almObj[i]);
#endif
			nfui_nfobject_set_data(obj, "event type", GUINT_TO_POINTER(i));
			nfui_regi_post_event_callback(obj, post_ao_btn_event_handler);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x+action_w[ACTION_AOUT]-size_w), (pos_y + (i * 41)));
			g_almcmbObj[i] = obj;
		}
	}

	pos_x += (action_w[ACTION_AOUT] + 2);

	// buzzer & email
	for(i = ACTION_BUZZER; i < ACTION_MAX; i++)
	{
		for(j=0; j<SYS_NET_EVT_CNT; j++) {
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
			nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
#if defined(_SUPPORT_AIBOX)
			if (j == AIBOX_TROUBLE_EVT_DATA)
			{
				if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128)) nfui_nfobject_show(fixed_temp);
				else nfui_nfobject_hide(fixed_temp);
			}
			else nfui_nfobject_show(fixed_temp);
#else
			nfui_nfobject_show(fixed_temp);
#endif
			nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, (pos_y + (j * 41)));

			if(i == ACTION_BUZZER) 		obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].buzzer);
			else if(i == ACTION_OSD)    obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].osd);
			else if(i == ACTION_EMAIL)  obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].email);
			else if(i == ACTION_FTP)    obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].ftp);
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)	obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].mobile);
#endif
			else if(i == ACTION_MOBILEPUSH)	obj = (NFOBJECT*)nfui_checkbutton_new(g_snd[j].mobilepush);

			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
			nfui_check_get_size(obj, &size_w, &size_h);
#if defined(_SUPPORT_AIBOX)
			if (j == AIBOX_TROUBLE_EVT_DATA)
			{
				if(nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A || (ivsc.vendor_code != 28 && ivsc.vendor_code != 128)) nfui_nfobject_show(obj);
				else nfui_nfobject_hide(obj);
			}
			else nfui_nfobject_show(obj);
#else
			nfui_nfobject_show(obj);
#endif
			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w)/2, (40-size_h)/2);
			if( (i == ACTION_MOBILEPUSH )&& (!support_mobilepush))
			    nfui_nfobject_hide(obj);

			if(i == ACTION_BUZZER)		g_buzzObj[j] = obj;
			else if(i == ACTION_OSD)    g_osdObj[j] = obj;
			else if(i == ACTION_EMAIL)	g_mailObj[j] = obj;
			else if(i == ACTION_FTP)	g_ftpObj[j] = obj;
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)	g_mobileObj[j] = obj;
#endif
			else if(i == ACTION_MOBILEPUSH)	g_mobilepushObj[j] = obj;

			if(i == ACTION_BUZZER) 		nfui_regi_post_event_callback(obj, post_buzzer_chk_event_handler);
			else if(i== ACTION_OSD)     nfui_regi_post_event_callback(obj, post_osd_chk_event_handler);
			else if(i == ACTION_EMAIL)  nfui_regi_post_event_callback(obj, post_mail_chk_event_handler);
			else if(i == ACTION_FTP)    nfui_regi_post_event_callback(obj, post_ftp_chk_event_handler);
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE) nfui_regi_post_event_callback(obj, post_mobile_chk_event_handler);
#endif
			else if(i == ACTION_MOBILEPUSH) nfui_regi_post_event_callback(obj, post_mobilepush_chk_event_handler);

			if(i >= ACTION_EMAIL && j == IP_CONFLICT_EVENT) nfui_nfobject_disable(obj);
		}

		pos_x += (action_w[i] + 2);
	}

#if defined(_SUPPORT_USER_PHONE) && defined(_SUPPORT_S1_STEP1)
	nfui_nfobject_disable(g_mobileAll);

	for(j=0; j<SYS_NET_EVT_CNT; j++)
	{
			nfui_nfobject_disable(g_mobileObj[j]);
	}
#endif

	if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
	{
		nfui_nfobject_disable(g_almObj[1]);
		nfui_nfobject_disable(g_almcmbObj[1]);
		nfui_nfobject_disable(g_buzzObj[1]);
		nfui_nfobject_disable(g_mailObj[1]);
		nfui_nfobject_disable(g_ftpObj[1]);

#if defined(_SUPPORT_USER_PHONE)
		nfui_nfobject_disable(g_mobileObj[1]);
#endif
        if(support_mobilepush)
		    nfui_nfobject_disable(g_mobilepushObj[1]);
	}

	_set_all_chk_btn_all_event(FALSE);

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nfobject_set_data(g_mailObj[SYS_NET_EVT_CNT-1], "bottom focus", obj);
	nfui_nfobject_set_data(g_ftpObj[SYS_NET_EVT_CNT-1], "bottom focus", obj);
#if defined(_SUPPORT_USER_PHONE)
	nfui_nfobject_set_data(g_mobileObj[SYS_NET_EVT_CNT-1], "bottom focus", obj);
#endif
	nfui_nfobject_set_data(g_mobilepushObj[SYS_NET_EVT_CNT-1], "bottom focus", obj);


	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_system_network_data_changed()
{
	get_data_from_obj();

	if(!memcmp(g_osnd, g_snd, sizeof(EA_SysNetData) * SYS_NET_EVT_CNT)
			&& !memcmp(&g_osnc, &g_snc, sizeof(EA_SysNetFailCnt)))
		return FALSE;

	return TRUE;
}

void save_system_network_data()
{
	set_sn_data();
}

void restore_system_network_data()
{
	g_memmove(g_snd, g_osnd, sizeof(EA_SysNetData)*SYS_NET_EVT_CNT);
	g_memmove(&g_snc, &g_osnc, sizeof(EA_SysNetFailCnt));

	set_data_to_obj(0);
}
