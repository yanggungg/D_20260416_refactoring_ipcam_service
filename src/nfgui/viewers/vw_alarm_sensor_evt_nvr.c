#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_alarm_sensor_evt_local.h"
#include "vw_channel_mask_ctrl.h"
#include "vw_evt_ptz_preset_popup.h"

// FIXME
#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "scm.h"

#define	ALARM_STRING_LENGTH	128

enum {
	ACTION_LCAMERA = 0,
	ACTION_AOUT,
#if defined(_SUPPORT_EVENT_PRESET)
	ACTION_PRESET,
#endif
	ACTION_BUZZER,
	ACTION_VPOPUP,
	ACTION_OPOPUP,
	ACTION_EMAIL,
	ACTION_FTP,
#if defined(_SUPPORT_USER_PHONE)
	ACTION_MOBILE,
#endif
	ACTION_MOBILEPUSH,
 	ACTION_MAX
};

static EA_AlmSenData g_asd[16];
static EA_AlmSenData g_oasd[16];
static PtzPresetData PresetData[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT **g_operObj;
static NFOBJECT **g_lcamObj;
static NFOBJECT **g_almObj;
static NFOBJECT **g_buzzObj;
static NFOBJECT **g_vpopObj;
static NFOBJECT **g_opopObj;
static NFOBJECT **g_mailObj;
static NFOBJECT **g_ftpObj;
static NFOBJECT **g_mobileObj;
static NFOBJECT **g_mobilepushObj;
static NFOBJECT **g_nameObj;
static NFOBJECT **g_presetObj;
static NFOBJECT **g_presetEditObj;
static NFOBJECT *g_buzzAll;
static NFOBJECT *g_vpopAll;
static NFOBJECT *g_opopAll;
static NFOBJECT *g_mailAll;
static NFOBJECT *g_ftpAll;
static NFOBJECT *g_mobileAll;
static NFOBJECT *g_mobilepushAll;


static void init_as_data();
static void set_as_data();
static void set_data_to_obj(gint expose);
static void get_data_from_obj();
static void init_preset_data();

static void conv_cam_data_to_string(guint data, gchar str[]);
static void conv_alm_out_data_to_string(guint64 data, gchar str[]);

static int support_mobilepush = 0;
static gint g_alarmIn = 0;

static void init_as_data()
{
	guint i;

	memset(g_asd, 0x00, sizeof(EA_AlmSenData)*g_alarmIn);
	memset(g_oasd, 0x00, sizeof(EA_AlmSenData)*g_alarmIn);
	for(i = 0; i < g_alarmIn; i++)
		DAL_get_almSen_data(&g_asd[i], i + 32);

	g_memmove(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn);

	//for(i=0; i<ALARM_IN_COUNT; i++) {
	//	g_message("%s :\noper: %d\nlcam: %d\nalmout: %d\nbuzzer: %s\nvpop: %s\nopop: %s\nmail: %s\n",
	//			__FUNCTION__,
	//			g_asd[i].oper 	,
	//			g_asd[i].linkCam ,
	//			g_asd[i].almOut ,
	//			g_asd[i].buzzer ? "TRUE" : "FALSE",
	//			g_asd[i].vpop ? "TRUE" : "FALSE" ,
	//			g_asd[i].opop  ? "TRUE" : "FALSE",
	//			g_asd[i].email ? "TRUE" : "FALSE");
	//}
}

static void init_obj()
{
    g_operObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_lcamObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_almObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_buzzObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_vpopObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_opopObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_mailObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_ftpObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_mobileObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_mobilepushObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_nameObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_presetObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    g_presetEditObj = (NFOBJECT**)imalloc(sizeof(NFOBJECT) * g_alarmIn);
    
    g_message("%s, %d ALARM_SENSOR_CAMERA OBJ MALLOC SUCCESS! g_alarmIn %d", __FUNCTION__, __LINE__, g_alarmIn);//ksi_test
}

static void free_obj()
{
    ifree(g_operObj);
    ifree(g_lcamObj);
    ifree(g_almObj);
    ifree(g_buzzObj);
    ifree(g_vpopObj);
    ifree(g_opopObj);
    ifree(g_mailObj);
    ifree(g_ftpObj);
    ifree(g_mobileObj);
    ifree(g_mobilepushObj);
    ifree(g_nameObj);
    ifree(g_presetObj);
    ifree(g_presetEditObj);
    
    g_message("%s, %d ALARM_SENSOR_CAMERA OBJ FREE SUCCESS!", __FUNCTION__, __LINE__);
}

static void init_preset_data()
{
    gint i;

	memset(&PresetData, 0x00, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);

	for(i = 0; i < GUI_CHANNEL_CNT; i++)
		DAL_get_ptz_preset_data(&PresetData[i], i);
}

static void set_as_data()
{
    gint i;

    for (i = 0; i < g_alarmIn; i++) {
    	DAL_set_almSen_data(g_asd[i], i + CAM_ALARM_IN);
	}

    DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);
    DAL_notify_fire_DB_change(NF_SYSDB_CATE_ACT);

	scm_put_log(CHANGE_EVT_SENSOR, 0, 0);
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state;
	gint i;

	for (i = 0; i < g_alarmIn; i++)
	{
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

	if (i == g_alarmIn)
	{
	    if (expose)
    	    nfui_check_button_set_active((NFCHECKBUTTON*)all, TRUE);
	    else
	        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, TRUE);
    }

    return 0;
}

static gint _set_all_chk_btn_all_event(gboolean expose)
{
	_set_all_chk_btn(g_buzzAll, g_buzzObj, expose);
	_set_all_chk_btn(g_vpopAll, g_vpopObj, expose);
	_set_all_chk_btn(g_opopAll, g_opopObj, expose);
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

	for(i = 0; i < g_alarmIn; i++) {
		nfui_nflabel_set_text((NFLABEL*)g_nameObj[i], g_asd[i].name);
		if (expose) nfui_signal_emit(g_nameObj[i], GDK_EXPOSE, FALSE);

		if (expose) nfui_spin_button_set_index((NFSPINBUTTON*)g_operObj[i], (guint)g_asd[i].oper);
		else nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_operObj[i], (guint)g_asd[i].oper);

		memset(str, 0x00, ALARM_STRING_LENGTH);
		conv_cam_data_to_string(g_asd[i].linkCam, str);

		nfui_nflabel_set_text((NFLABEL*)g_lcamObj[i], str);
		if (expose) nfui_signal_emit(g_lcamObj[i], GDK_EXPOSE, FALSE);

		memset(str, 0x00, ALARM_STRING_LENGTH);
		conv_alm_out_data_to_string(g_asd[i].almOut, str);

		nfui_nflabel_set_text((NFLABEL*)g_almObj[i], str);
		if (expose) nfui_signal_emit(g_almObj[i], GDK_EXPOSE, FALSE);

		if (expose)
		{
#if defined(_SUPPORT_EVENT_PRESET)
			nfui_check_button_set_active((NFCHECKBUTTON*)g_presetObj[i], g_asd[i].preset);

            if (g_asd[i].preset)    nfui_nfobject_enable(g_presetEditObj[i]);
            else                    nfui_nfobject_disable(g_presetEditObj[i]);

            nfui_signal_emit(g_presetEditObj[i], GDK_EXPOSE, FALSE);
#endif
			nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], g_asd[i].buzzer);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_vpopObj[i], g_asd[i].vpop);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_opopObj[i], g_asd[i].opop);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], g_asd[i].email);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], g_asd[i].ftp);
#if defined(_SUPPORT_USER_PHONE)
			nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], g_asd[i].mobile);
#endif
            if(support_mobilepush)
    			nfui_check_button_set_active((NFCHECKBUTTON*)g_mobilepushObj[i], g_asd[i].mobilepush);
}
		else
		{
#if defined(_SUPPORT_EVENT_PRESET)
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_presetObj[i], g_asd[i].preset);

            if (g_asd[i].preset)    nfui_nfobject_enable(g_presetEditObj[i]);
            else                    nfui_nfobject_disable(g_presetEditObj[i]);
#endif
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_buzzObj[i], g_asd[i].buzzer);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_vpopObj[i], g_asd[i].vpop);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_opopObj[i], g_asd[i].opop);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mailObj[i], g_asd[i].email);
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ftpObj[i], g_asd[i].ftp);
#if defined(_SUPPORT_USER_PHONE)
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mobileObj[i], g_asd[i].mobile);
#endif
            if(support_mobilepush)
    			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_mobilepushObj[i], g_asd[i].mobilepush);
}
	}

	_set_all_chk_btn_all_event(expose);
}

static void get_data_from_obj()
{
	gint i;
	gchar *pStr = NULL;

	for(i = 0; i < g_alarmIn; i++) {
		pStr = nfui_nflabel_get_text((NFLABEL*)g_nameObj[i]);
		if(pStr) strcpy(g_asd[i].name, pStr);

		g_asd[i].oper 	 = (guchar)nfui_spin_button_get_index((NFSPINBUTTON*)g_operObj[i]);

		g_asd[i].buzzer = nfui_check_button_get_active((NFCHECKBUTTON*)g_buzzObj[i]);
		g_asd[i].vpop   = nfui_check_button_get_active((NFCHECKBUTTON*)g_vpopObj[i]);
		g_asd[i].opop   = nfui_check_button_get_active((NFCHECKBUTTON*)g_opopObj[i]);
		g_asd[i].email  = nfui_check_button_get_active((NFCHECKBUTTON*)g_mailObj[i]);
		g_asd[i].ftp  = nfui_check_button_get_active((NFCHECKBUTTON*)g_ftpObj[i]);
#if defined(_SUPPORT_USER_PHONE)
		g_asd[i].mobile = nfui_check_button_get_active((NFCHECKBUTTON*)g_mobileObj[i]);
#endif
        if(support_mobilepush)
    		g_asd[i].mobilepush = nfui_check_button_get_active((NFCHECKBUTTON*)g_mobilepushObj[i]);
	}
}

static void conv_cam_data_to_string(guint data, gchar str[])
{
	guint i, j;
	gint bytes;

	for(i = 0, j = 0; i < GUI_CHANNEL_CNT; i++) {
		if(data & (1 << i)) {
			if(j < 1) bytes = g_sprintf(&str[j], "%d", i+1);
			else 	  bytes = g_sprintf(&str[j], ",%d", i+1);

			j += bytes;
		}
	}

	if(j == 0) g_sprintf(str, "%s", "N/A");
}

static void conv_alm_out_data_to_string(guint64 data, gchar str[])
{
	guint i, j;
	gchar tmp[8];

#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	for(i = 0, j = 0; i < ALARM_OUT_COUNT; i++) {
		memset(tmp, 0x00, sizeof(tmp));
		if(data & (1 << i)) {
			if(j < 1) 	g_sprintf(tmp, "A%d", i+1);
			else 	  	g_sprintf(tmp, ",A%d", i+1);

			j++;
		}
		strcat(str, tmp);
	}
#else
	gint bytes;

	for(i = 0, j = 0; i < ALARM_OUT_COUNT; i++) {
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

static gboolean post_oper_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx;
		gint i;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		for(i = 0; i < g_alarmIn; i++)
			nfui_spin_button_set_index((NFSPINBUTTON*)g_operObj[i], (guint)idx);

		for(i = 0; i < g_alarmIn; i++)
			nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_lc_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint off_x, off_y;
		unsigned long long mask = CAMERA_MASK_TYPE;
		gchar str[ALARM_STRING_LENGTH];
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		if(!VW_ChannelMask_Ctrl(g_curwnd, "LINKED CAMERA CHANNEL", (gint)(off_x - 153), (gint)(off_y + obj->height), &mask))
			return FALSE;

		for(i = 0; i < g_alarmIn; i++) {
			if(mask != g_asd[i].linkCam) {
				g_asd[i].linkCam = mask;

				memset(str, 0x00, ALARM_STRING_LENGTH);
				conv_cam_data_to_string(g_asd[i].linkCam, str);

				nfui_nflabel_set_text((NFLABEL*)g_lcamObj[i], str);
				nfui_signal_emit(g_lcamObj[i], GDK_EXPOSE, FALSE);
			}
		}
	}

	return FALSE;
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

		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", (gint)(off_x - 153), (gint)(off_y + obj->height), &mask))
			return FALSE;

		for(i = 0; i < g_alarmIn; i++) {
			if(mask != g_asd[i].almOut) {
				g_asd[i].almOut = mask;

				memset(str, 0x00, ALARM_STRING_LENGTH);
				conv_alm_out_data_to_string(g_asd[i].almOut, str);

				nfui_nflabel_set_text((NFLABEL*)g_almObj[i], str);
				nfui_signal_emit(g_almObj[i], GDK_EXPOSE, FALSE);
			}
		}
	}

	return FALSE;
}

static gboolean post_po_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{


	return FALSE;
}

static gboolean post_buzzer_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_buzzObj[i], state);
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

		for(i = 0; i < g_alarmIn; i++) {
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
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_buzzObj[i+1]);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_vpop_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_vpopObj[i], state);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_vpopObj[0]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_vpop_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		_set_all_chk_btn(g_vpopAll, g_vpopObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0; i < g_alarmIn; i++) {
			if(g_vpopObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_vpopObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_vpopAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_vpopObj[i+1]);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_opop_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;

		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_opopObj[i], state);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			change_obj_focus(obj, g_opopObj[0]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_opop_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		_set_all_chk_btn(g_opopAll, g_opopObj, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0; i < g_alarmIn; i++) {
			if(g_opopObj[i] == obj)
				break;
		}

		if(kpid == KEYPAD_UP) {
			if(i - 1 >= 0) {
				change_obj_focus(obj, g_opopObj[i-1]);
				return TRUE;
			}
			else {
				change_obj_focus(obj, g_opopAll);
				return TRUE;
			}
		}
		else if(kpid == KEYPAD_DOWN) {
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_opopObj[i+1]);
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

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mailObj[i], state);
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
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0; i < g_alarmIn; i++) {
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
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_mailObj[i+1]);
				return TRUE;
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

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_ftpObj[i], state);
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
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0; i < g_alarmIn; i++) {
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
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_ftpObj[i+1]);
				return TRUE;
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

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobileObj[i], state);
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
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0; i < g_alarmIn; i++) {
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
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_mobileObj[i+1]);
				return TRUE;
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

		for (i = 0; i < g_alarmIn; i++)
            nfui_check_button_set_active((NFCHECKBUTTON*)g_mobilepushObj[i], state);
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
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for(i = 0 ; i < g_alarmIn; i++) {
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
			if(i + 1 < g_alarmIn) {
				change_obj_focus(obj, g_mobilepushObj[i+1]);
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_lc_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		guint idx;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "local_alarmIn"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (CAMERA_MASK_TYPE | g_asd[idx].linkCam);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "LINKED CAMERA CHANNEL", (gint)off_x, (gint)(off_y + obj->height), &mask))
			return FALSE;

		if(mask != g_asd[idx].linkCam) {
			g_asd[idx].linkCam = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_cam_data_to_string(g_asd[idx].linkCam, str);

			nfui_nflabel_set_text((NFLABEL*)obj, str);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
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
		guint idx;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(ALARM_OUT_COUNT < 2)
			return FALSE;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;


		idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "local_alarmIn"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (ALARM_OUT_MASK_TYPE | g_asd[idx].almOut);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", (gint)off_x, (gint)(off_y + obj->height), &mask))
			return FALSE;

		if(mask != g_asd[idx].almOut) {
			g_asd[idx].almOut = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_alm_out_data_to_string(g_asd[idx].almOut, str);

			nfui_nflabel_set_text((NFLABEL*)obj, str);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_lc_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint idx;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "local_alarmIn"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (CAMERA_MASK_TYPE | g_asd[idx].linkCam);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "LINKED CAMERA CHANNEL", (gint)(off_x - 177), (gint)(off_y + obj->height), &mask))
			return FALSE;

		if(mask != g_asd[idx].linkCam) {
			g_asd[idx].linkCam = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_cam_data_to_string(g_asd[idx].linkCam, str);

			nfui_nflabel_set_text((NFLABEL*)g_lcamObj[idx], str);
			nfui_signal_emit(g_lcamObj[idx], GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_ao_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		guint idx;
		unsigned long long mask = 0;
		gchar str[ALARM_STRING_LENGTH];
		guint off_x, off_y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "local_alarmIn"));

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		mask = (ALARM_OUT_MASK_TYPE | g_asd[idx].almOut);
		if(!VW_ChannelMask_Ctrl(g_curwnd, "ALARM OUT CHANNEL", (gint)(off_x - 153), (gint)(off_y + obj->height), &mask))
			return FALSE;

		if(mask != g_asd[idx].almOut) {
			g_asd[idx].almOut = mask;

			memset(str, 0x00, ALARM_STRING_LENGTH);
			conv_alm_out_data_to_string(g_asd[idx].almOut, str);

			nfui_nflabel_set_text((NFLABEL*)g_almObj[idx], str);
			nfui_signal_emit(g_almObj[idx], GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_preset_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
   	gboolean state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

	    for (i = 0; i < g_alarmIn; i++)
	    {
            if (g_presetObj[i] == obj)
                break;
	    }

        g_asd[i].preset = state;

        if (state)  nfui_nfobject_enable(g_presetEditObj[i]);
        else        nfui_nfobject_disable(g_presetEditObj[i]);

        nfui_signal_emit(g_presetEditObj[i], GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_preset_edit_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, j;
    PRST_INFO_T *info;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        info = imalloc(sizeof(PRST_INFO_T));
        memset(info, 0x00, sizeof(PRST_INFO_T));

	    for (i = 0; i < g_alarmIn; i++)
	    {
            if (g_presetEditObj[i] == obj)
                break;
	    }

        for (j = 0; j < GUI_CHANNEL_CNT; j++)
            info->number[j] = g_asd[i].preset_num[j];

        vw_evt_ptz_preset_popup_open(g_curwnd, info, 0xffffffff);

        for (j = 0; j < GUI_CHANNEL_CNT; j++)
            g_asd[i].preset_num[j] = info->number[j];

        ifree(info);
	}

	return FALSE;
}

static gboolean post_name_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *title;
	gchar *strTemp = NULL;
	guint x, y;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    mb_type ret = NFTOOL_MB_OK;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(kpid == KEYPAD_ENTER) {
			nfui_nfobject_get_offset(obj, &x, &y);
			y += obj->height;
		}else {
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

        title = nfui_nflabel_get_text((NFLABEL*)obj);

        if (!ifn_is_all_itxstyle_title(title))
        {
            vw_mbox(g_curwnd, "ERROR", IMBX_UNSUPPORTED_LETTER, NFTOOL_MB_OK);
            return FALSE;
        }

		strTemp = VirtualKey_Open(g_curwnd, title, x, y, 16, VKEY_ITXSTYLE_TITLE);

		if(strTemp)
		{
			if(strlen(strTemp) <= 0)
				ret = nftool_mbox(g_curwnd, "NOTICE", "There is no input.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

            if (ret == NFTOOL_MB_OK)
            {
    			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
    			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
            }

			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		g_memmove(g_asd, g_oasd, sizeof(EA_AlmSenData)*g_alarmIn);

		set_data_to_obj(1);
		_set_all_chk_btn(g_buzzAll, g_buzzObj, TRUE);
	}
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		get_data_from_obj();

		if(memcmp(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn)) {
			set_as_data();

			g_memmove(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn);

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
		mb_type ret;
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		get_data_from_obj();

		if(!memcmp(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn)) {
			VW_Evt_Act_Destroy(obj);
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		
		if(ret == NFTOOL_MB_OK) {
			set_as_data();

			event_act_data_changed(TRUE);
		}
		
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
	    free_obj();
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_AlarmSensorEvt_NVR_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *obj2;
	NFOBJECT *fixed_temp;
	NFOBJECT *tbl = NULL;

	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *dropdown_img2[NFOBJECT_STATE_COUNT];

	gchar buf[128];
	gchar *strParam[] = {"ALARM", "NAME", "OPERATION"};
	gchar *strAct[] = { "LINKED CAMERA", "ALARM OUT"};

#if defined(_SUPPORT_SENSOR_RELAY)
	gchar *strOper[] = {"HIGH", "LOW"};
#else
	gchar *strOper[] = {"N/O", "N/C"};
#endif
	gchar *strOnOff[] = {"OFF", "ON"};
	gint size_w, size_h;
	guint pos_x, pos_y;
	guint i, j, ret;
	guint tbl_w[14] = {35, 170, 35, 170, 35, 170, 35, 170, 35, 170, 35, 170, 35, 170};

	guint action_w[ACTION_MAX] = {0,};
	guint action_pos_x = 0;
	guint action_tot_w = 0;
	guint legend_tot_w = 0;
	guint legend_cnt = 0;

	g_curwnd = nfui_nfobject_get_top(parent);

	support_mobilepush = var_get_supported_mobilepush();
	g_alarmIn = var_get_dvr_alarmIn_cnt();

	// init data
	init_as_data();
    init_preset_data();
    init_obj();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	legend_cnt = ACTION_MAX-ACTION_BUZZER;

	for (i = 0; i < legend_cnt*2; i++)
		legend_tot_w += tbl_w[i];

	// legend ////////////////////////////////////////////////////////////////////////////////////////
	tbl = (NFOBJECT*)nfui_nftable_new(legend_cnt*2, 1, 1, 1, tbl_w, 35);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 1485-legend_tot_w, 0);

	legend_cnt = 0;

    obj = nfui_nfimage_new(IMG_ACTION_BUZZER_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUZZER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_VIDEO_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_OSD_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OSD POPUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_EMAIL_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

    obj = nfui_nfimage_new(IMG_ACTION_FTP_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FTP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

#if defined(_SUPPORT_USER_PHONE)
	obj = nfui_nfimage_new(IMG_ACTION_MOBILE_02);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOBILE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
#endif
	obj = nfui_nfimage_new(IMG_ACTION_MOBILE_PUSH_02);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
	if(support_mobilepush)
        nfui_nfobject_show(obj);
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOBILE PUSH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nftable_attach((NFTABLE*)tbl, obj, legend_cnt++, 0);
	if(support_mobilepush)
	    nfui_nfobject_show(obj);

	// event parameter ////////////////////////////////////////////////////////////////////////////////////////
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT PARAMETER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 330, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 193, 40);


	pos_x = 27;
	pos_y = 81;

	// cam_n (col 0)
	dropdown_img2[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
	dropdown_img2[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
	dropdown_img2[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
	dropdown_img2[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);

	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

	for(i = 0; i < 3; i++) {
		if(i < 2)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strParam[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, 164, 40);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
		}
		else
		{
			obj = nfui_combobox_new(strOper, 2, 0);
			nfui_combobox_set_display_string(NF_COMBOBOX(obj), strParam[i]);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_2);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, 164, 40);
			nfui_nfobject_show(obj);
			nfui_regi_post_event_callback(obj, post_oper_all_event_handler);
		}

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x + (i * 166)), pos_y);
	}


	pos_y += (40 + 1);

	for(i = 0; i < g_alarmIn; i++) {
		g_sprintf(buf, "AI %d", i + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, (pos_y + (i * 41)));
	}


	pos_x += (164 + 2);

	// name (col 1)
	for(i = 0; i < g_alarmIn; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_asd[i].name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 164, 40);
		nfui_regi_post_event_callback(obj, post_name_event_handler);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, (pos_y + (i * 41)));

		g_nameObj[i] = obj;
	}


	pos_x += (164 + 2);

	// operation (col 2)
	for(i = 0; i < g_alarmIn; i++) {
		g_operObj[i] = nfui_spinbutton_new(strOper, 2, (gint)g_asd[i].oper);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_operObj[i], NFSPINBUTTON_TYPE_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)g_operObj[i], NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(g_operObj[i], 164, 40);
		nfui_nfobject_show(g_operObj[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, g_operObj[i], pos_x, (pos_y + (i * 41)));
	}



	// action ////////////////////////////////////////////////////////////////////////////////////////
	pos_x += (164 + 4);
	action_pos_x = pos_x;
	pos_y = 40;

    nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

    action_w[ACTION_LCAMERA] = 177+size_w;
    action_w[ACTION_AOUT] = 150+size_w;
#if defined(_SUPPORT_EVENT_PRESET)
    action_w[ACTION_PRESET] = 105;
#endif
    action_w[ACTION_BUZZER] = 68;
    action_w[ACTION_VPOPUP] = 68;
    action_w[ACTION_OPOPUP] = 68;
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
	if((i == ACTION_MOBILEPUSH) && (!support_mobilepush))
	    nfui_nfobject_hide(obj);

	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

	pos_y += (40 + 1);

	for(i = 0; i < ACTION_MAX; i++) {
		if (i <= ACTION_AOUT)
		{
        	nfui_get_pixbuf_size(dropdown_img2[0], &size_w, &size_h);

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strAct[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			if (i == ACTION_LCAMERA)        nfui_nfobject_set_size(obj, action_w[ACTION_LCAMERA]-size_w, 40);
			else if (i == ACTION_AOUT)	    nfui_nfobject_set_size(obj, action_w[ACTION_AOUT]-size_w, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

			if (i == ACTION_LCAMERA)        pos_x += (action_w[ACTION_LCAMERA]-size_w);
			else if (i == ACTION_AOUT)      pos_x += (action_w[ACTION_AOUT]-size_w);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image((NFBUTTON*)obj, dropdown_img2);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
			if (i == ACTION_LCAMERA)        nfui_regi_post_event_callback(obj, post_lc_all_btn_event_handler);
			else if (i == ACTION_AOUT)	    nfui_regi_post_event_callback(obj, post_ao_all_btn_event_handler);

			pos_x += (size_w + 2);
		}
#if defined(_SUPPORT_EVENT_PRESET)
		else if (i == ACTION_PRESET)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRESET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, action_w[ACTION_PRESET], 40);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

            pos_x += (action_w[ACTION_PRESET] + 2);
		}
#endif
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

    			obj = nfui_nfimage_new(IMG_ACTION_BUZZER_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
            }
            else if (i == ACTION_VPOPUP)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_VIDEO_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_vpop_all_event_handler);
                g_vpopAll = obj;

    			obj = nfui_nfimage_new(IMG_ACTION_VIDEO_01);
            	nfui_nfobject_show(obj);
            	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);

            }
            else if (i == ACTION_OPOPUP)
            {
                gint icon_size_w, icon_size_h;

                nfui_get_image_size(IMG_ACTION_OSD_01, &icon_size_w, &icon_size_h);

    			obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
    			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    			nfui_nfobject_show(obj);
    			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
                nfui_regi_post_event_callback(obj, post_opop_all_event_handler);
                g_opopAll = obj;

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
				nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2, 11);
				nfui_regi_post_event_callback(obj, post_mobilepush_all_event_handler);
				g_mobilepushAll = obj;
				if(support_mobilepush)
    				nfui_nfobject_show(obj);

				obj = nfui_nfimage_new(IMG_ACTION_MOBILE_PUSH_01);
				if(support_mobilepush)
    				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (action_w[i]-size_w-4-icon_size_w)/2 + size_w+4, 2);
			}

			pos_x += (action_w[i] + 2);
		}
	}

	// linked camera (col 0) & alarm out (col 1)
	pos_x = action_pos_x;
	pos_y += (40 + 1);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

// linked camera
	for(i = 0; i < g_alarmIn; i++)
	{
		memset(buf, 0x00, sizeof(buf));
		conv_cam_data_to_string(g_asd[i].linkCam, buf);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(129));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
		nfui_nfobject_use_tooltip(obj, FALSE);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(obj, action_w[ACTION_LCAMERA]-size_w, 40);
		nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_lc_label_event_handler);
		nfui_nfobject_set_data(obj, "local_alarmIn", GUINT_TO_POINTER(i));
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x, (guint)(pos_y + (i * 41)));
        g_lcamObj[i] = obj;

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nfobject_set_data(obj, "local_alarmIn", GUINT_TO_POINTER(i));
	    nfui_regi_post_event_callback(obj, post_lc_btn_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x+action_w[ACTION_LCAMERA]-size_w, (guint)(pos_y + (i * 41)));
	}

	pos_x += (action_w[ACTION_LCAMERA] + 2);

// alarm out
	for(i = 0; i < g_alarmIn; i++) {
		memset(buf, 0x00, sizeof(buf));
		conv_alm_out_data_to_string(g_asd[i].almOut, buf);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(129));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
		nfui_nfobject_use_tooltip(obj, FALSE);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
        nfui_nfobject_set_size(obj, (ALARM_OUT_COUNT >= 2 ? action_w[ACTION_AOUT]-size_w : action_w[ACTION_AOUT]), 40);
		nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_ao_label_event_handler);
		nfui_nfobject_set_data(obj, "local_alarmIn", GUINT_TO_POINTER(i));
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)pos_x, (guint)(pos_y + (i * 41)));
		g_almObj[i] = obj;

		if(ALARM_OUT_COUNT >= 2) {
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), dropdown_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nfobject_set_data(obj, "local_alarmIn", GUINT_TO_POINTER(i));
			nfui_regi_post_event_callback(obj, post_ao_btn_event_handler);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, (guint)(pos_x+action_w[ACTION_AOUT]-size_w), (guint)(pos_y + (i * 41)));
		}
	}

	pos_x += (action_w[ACTION_AOUT] + 2);

#if defined(_SUPPORT_EVENT_PRESET)
    for (i = 0; i < g_alarmIn; i++)
    {
    	fixed_temp = (NFOBJECT*)nfui_nffixed_new();
    	nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
    	nfui_nfobject_set_size(fixed_temp, action_w[ACTION_PRESET], 40);
    	nfui_nfobject_show(fixed_temp);
    	nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, pos_y + (i * 41));

    	obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[i].preset);
    	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    	nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 20, (40-size_h)/2);
    	nfui_regi_post_event_callback(obj, post_preset_chk_event_handler);
    	g_presetObj[i] = obj;

    	obj = nftool_normal_button_create_subtab_type1("..", 35);
    	nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER_DOWN, 4);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 20+size_w+8, 0);
    	nfui_regi_post_event_callback(obj, post_preset_edit_btn_event_handler);
    	g_presetEditObj[i] = obj;

    	if (!g_asd[i].preset)   nfui_nfobject_disable(obj);
    }

	pos_x += (action_w[ACTION_PRESET] + 2);
#endif

	for (i = ACTION_BUZZER ; i < ACTION_MAX; i++)
	{
		for (j = 0; j < g_alarmIn; j++)
		{
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
			nfui_nfobject_show(fixed_temp);
			nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, (pos_y + (j * 41)));

			if(i == ACTION_BUZZER)		    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].buzzer);
			else if(i == ACTION_VPOPUP)	    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].vpop);
			else if(i == ACTION_OPOPUP)	    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].opop);
			else if(i == ACTION_EMAIL)	    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].email);
			else if(i == ACTION_FTP)	    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].ftp);
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)	    obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].mobile);
#endif
			else if(i == ACTION_MOBILEPUSH) 	obj = (NFOBJECT*)nfui_checkbutton_new(g_asd[j].mobilepush);

			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(action_w[i]-size_w)/2, (guint)(40-size_h)/2);
			if((!support_mobilepush) && (i == ACTION_MOBILEPUSH))
			    nfui_nfobject_hide(obj);

			if(i == ACTION_BUZZER)			g_buzzObj[j] = obj;
			else if(i == ACTION_VPOPUP)		g_vpopObj[j] = obj;
			else if(i == ACTION_OPOPUP)		g_opopObj[j] = obj;
			else if(i == ACTION_EMAIL)		g_mailObj[j] = obj;
			else if(i == ACTION_FTP)		g_ftpObj[j] = obj;
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)		g_mobileObj[j] = obj;
#endif
			else if(i == ACTION_MOBILEPUSH) 		g_mobilepushObj[j] = obj;

			if(i == ACTION_BUZZER) 		    nfui_regi_post_event_callback(obj, post_buzzer_chk_event_handler);
			else if(i == ACTION_VPOPUP)     nfui_regi_post_event_callback(obj, post_vpop_chk_event_handler);
			else if(i == ACTION_OPOPUP)     nfui_regi_post_event_callback(obj, post_opop_chk_event_handler);
			else if(i == ACTION_EMAIL)      nfui_regi_post_event_callback(obj, post_mail_chk_event_handler);
			else if(i == ACTION_FTP)        nfui_regi_post_event_callback(obj, post_ftp_chk_event_handler);
#if defined(_SUPPORT_USER_PHONE)
			else if(i == ACTION_MOBILE)     nfui_regi_post_event_callback(obj, post_mobile_chk_event_handler);
#endif
			else if(i == ACTION_MOBILEPUSH) 	nfui_regi_post_event_callback(obj, post_mobilepush_chk_event_handler);
		}

		pos_x += (action_w[i] + 2);
	}

#if defined(_SUPPORT_USER_PHONE) && defined(_SUPPORT_S1_STEP1)
	nfui_nfobject_disable(g_mobileAll);

	for(i = 0; i < g_alarmIn; i++)
	{
    	nfui_nfobject_disable(g_mobileObj[i]);
	}
#endif

	_set_all_chk_btn_all_event(FALSE);

	// button
	obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean VW_AlarmSensorEvt_NVR_tab_out_handler()
{
	mb_type ret;

	get_data_from_obj();

	if(memcmp(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn)) {
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK) {
			g_memmove(g_oasd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn);

			set_as_data();
		}else if(ret == NFTOOL_MB_CANCEL) {
			g_memmove(g_asd, g_oasd, sizeof(EA_AlmSenData)*g_alarmIn);

			set_data_to_obj(0);
		}

		if(ret == NFTOOL_MB_OK)
			event_act_data_changed(TRUE);
	}

	return FALSE;
}
