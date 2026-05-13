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
#include "objects/nfcombobox.h"

#include "vw_evt_act_main.h"
#include "vw_alarm_out_evt.h"
#include "vw_alarm_out_setup.h"
#include "var.h"

// FIXME
#include "nf_action.h"
#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "scm.h"


enum {
	DURATION_TRANSPARENT = 0,
	DURATION_5_SEC,
	DURATION_10_SEC,
	DURATION_15_SEC,
	DURATION_20_SEC,
	DURATION_30_SEC,
	DURATION_40_SEC,
	DURATION_60_SEC,
	DURATION_120_SEC,
	DURATION_180_SEC,
	DURATION_300_SEC,
	DURATION_UNTIL_KEY,

	NUM_DURATIONS,
};

static EA_AlmOutData g_aod[DVR_ALARM_OUT];
static EA_AlmOutData g_oaod[DVR_ALARM_OUT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_operObj[DVR_ALARM_OUT];
static NFOBJECT *g_durObj[DVR_ALARM_OUT];
static NFOBJECT *g_tstObj[DVR_ALARM_OUT + 1];				// +1 => test all
static NFOBJECT *g_nameObj[DVR_ALARM_OUT];


static void init_ao_data();
static void set_ao_data();
static void set_data_to_obj(gint expose);
static void get_data_from_obj();
static gint conv_duration_to_index(gint duration);
static gint conv_index_to_duration(gint index);
static void test_alarm_out(gint ch, gboolean active);
static void test_all_alarm_out(gboolean active);


static void init_ao_data()
{
	guint i;

	memset(g_aod, 0x00, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);
	memset(g_oaod, 0x00, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);
	for(i=0; i<DVR_ALARM_OUT; i++)
		DAL_get_almOut_data(&g_aod[i], i+32);

	g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);
}

static void set_ao_data()
{
	int i;
	g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);
	//DAL_set_almOut_data_all(g_aod, DVR_ALARM_OUT);
	for(i=0; i<DVR_ALARM_OUT; i++)
		DAL_set_almOut_data(g_aod[i], i+32);

	scm_put_log(CHANGE_EVT_ALARMOUT, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	gint i;

	for(i=0; i<DVR_ALARM_OUT; i++) {
		nfui_nflabel_set_text((NFLABEL*)g_nameObj[i], g_aod[i].name);
		if (expose)	nfui_signal_emit(g_nameObj[i], GDK_EXPOSE, FALSE);
	}

	for(i=0; i<DVR_ALARM_OUT; i++) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_operObj[i], (guint)g_aod[i].oper);
		if (expose)	nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
	}

	for(i=0; i<DVR_ALARM_OUT; i++) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj[i], (guint)conv_duration_to_index(g_aod[i].duration));
		if (expose)	nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);
	}
}

static void get_data_from_obj()
{
	gint i;
	gint index;
	gchar *pStr = NULL;

	for(i=0; i<DVR_ALARM_OUT; i++) {
		pStr = nfui_nflabel_get_text((NFLABEL*)g_nameObj[i]);
		if(pStr) strcpy(g_aod[i].name, pStr);
	}

	for(i=0; i<DVR_ALARM_OUT; i++)
		g_aod[i].oper = (guchar)nfui_combobox_get_cur_index((NFCOMBOBOX*)g_operObj[i]);

	for(i=0; i<DVR_ALARM_OUT; i++) {
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_durObj[i]);
		g_aod[i].duration = conv_index_to_duration(index);
	}
}

static gint conv_duration_to_index(gint duration)
{
	if(duration < 5)			return DURATION_TRANSPARENT;
	else if(duration < 10)		return DURATION_5_SEC;
	else if(duration < 15)		return DURATION_10_SEC;
	else if(duration < 20)		return DURATION_15_SEC;
	else if(duration < 30)		return DURATION_20_SEC;
	else if(duration < 40)		return DURATION_30_SEC;
	else if(duration < 60)		return DURATION_40_SEC;
	else if(duration < 120)		return DURATION_60_SEC;
	else if(duration < 180)		return DURATION_120_SEC;
	else if(duration < 300)		return DURATION_180_SEC;
	else if(duration < 9999)	return DURATION_300_SEC;
	else						return DURATION_UNTIL_KEY;

	return 0;
}

static gint conv_index_to_duration(gint index)
{
	if(index == DURATION_TRANSPARENT)		return 0;	
	else if(index == DURATION_5_SEC)		return 5;	
	else if(index == DURATION_10_SEC)		return 10;	
	else if(index == DURATION_15_SEC)		return 15;	
	else if(index == DURATION_20_SEC)		return 20;	
	else if(index == DURATION_30_SEC)		return 30;	
	else if(index == DURATION_40_SEC)		return 40;	
	else if(index == DURATION_60_SEC)		return 60;	
	else if(index == DURATION_120_SEC)		return 120;	
	else if(index == DURATION_180_SEC)		return 180;	
	else if(index == DURATION_300_SEC)		return 300;	
	else if(index == DURATION_UNTIL_KEY)	return 9999;	
	else									return 0;	

	return 0;
}

static void test_alarm_out(gint ch, gboolean active)
{
	int nvr_ch = ch+32;

	g_message("NVR: %s => %d %d %d", __FUNCTION__, nvr_ch, GUI_CHANNEL_CNT, (nvr_ch - GUI_CHANNEL_CNT) + 32);
	if(g_aod[ch].oper) nf_action_relay_test(active, (nvr_ch >= GUI_CHANNEL_CNT ? (nvr_ch - GUI_CHANNEL_CNT) + 32 : nvr_ch), TRUE);
	else		   	   nf_action_relay_test(active, (nvr_ch >= GUI_CHANNEL_CNT ? (nvr_ch - GUI_CHANNEL_CNT) + 32 : nvr_ch), FALSE);

	if(active) 		   nfui_nfobject_disable(g_operObj[ch]);
	else               nfui_nfobject_enable(g_operObj[ch]);

	if(active) 		   nfui_nfobject_disable(g_durObj[ch]);
	else	           nfui_nfobject_enable(g_durObj[ch]);

	nfui_signal_emit(g_operObj[ch], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_durObj[ch], GDK_EXPOSE, TRUE);

	if(nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[0]))) 
		nfui_check_button_set_active(NF_CHECKBUTTON(g_tstObj[0]), FALSE);
}

static void test_all_alarm_out(gboolean active)
{
	gint i;

	for(i=0; i<DVR_ALARM_OUT; i++) {
		if(active && nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i+1])))
			continue;
		else if(!active && !nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i+1])))
			continue;

		if(g_aod[i].oper) nf_action_relay_test(active, (i >= GUI_CHANNEL_CNT ? (i - GUI_CHANNEL_CNT + 16) : i+16), TRUE);
		else		   	  nf_action_relay_test(active, (i >= GUI_CHANNEL_CNT ? (i - GUI_CHANNEL_CNT + 16) : i+16), FALSE);

		if(active) 		  nfui_nfobject_disable(g_operObj[i]);
		else              nfui_nfobject_enable(g_operObj[i]);

		if(active) 		  nfui_nfobject_disable(g_durObj[i]);
		else	   		  nfui_nfobject_enable(g_durObj[i]);
	}

	for(i=0; i<DVR_ALARM_OUT; i++) {
		nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);

		nfui_check_button_set_active(NF_CHECKBUTTON(g_tstObj[i+1]), active);
	}
}

static gboolean post_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gboolean act = FALSE;
		gint ch = -1;

		ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "alarm out ch"));
		//g_message("%s :::::::::::::::::::: channel %d ", __FUNCTION__, ch);
		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

		get_data_from_obj();

		if(ch == DVR_ALARM_OUT) test_all_alarm_out(act);
		else 	   				  test_alarm_out(ch, act);
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

static gboolean post_oper_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx;
		gint i;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		for(i=0; i<DVR_ALARM_OUT; i++) 
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_operObj[i], (guint)idx);
        
		for(i=0; i<DVR_ALARM_OUT; i++) 
			nfui_signal_emit(g_operObj[i], GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_dur_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint idx;
		gint i;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		for(i=0; i<DVR_ALARM_OUT; i++) 
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj[i], (guint)idx);

		for(i=0; i<DVR_ALARM_OUT; i++) 
			nfui_signal_emit(g_durObj[i], GDK_EXPOSE, TRUE);
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(g_aod, g_oaod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);

		set_data_to_obj(1);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		get_data_from_obj();

		if(memcmp(g_oaod, g_aod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT)) {
			set_ao_data();

			g_memmove(g_oaod, g_aod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);

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

		VW_AlarmOut_tab_out_handler();
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


void VW_Nvr_Init_AlarmOut_SetupPage(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gchar buf[128];
	gchar *strCol[] = {"", "NAME", "OPERATION", "DURATION"};
	//gchar *strCol[] = {"CAM", "NAME", "TYPE", "OPERATION", "DURATION"};
	gchar *strOper[] = {"N/O", "N/C"};
	gchar *strDur[] = {"TRANSPARENT", "5 SEC", "10 SEC", "15 SEC", "20 SEC", 
		"30 SEC", "40 SEC", "60 SEC", "120 SEC", 
		"180 SEC", "300 SEC", "UNTIL KEY"};

	guint tbl_w[5] = {164, 164, 164, 184, 164};
	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	gint i;

	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	
	g_curwnd = nfui_nfobject_get_top(parent);

	// init data
	init_ao_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	tbl = (NFOBJECT*)nfui_nftable_new(5, DVR_ALARM_OUT + 1, 2, 2, tbl_w, 40);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, 27, 13);

	for(i=0; i<4; i++) {
		if ( i == 0) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_show(obj);
		}
		else if(i > 0 && i<2) {
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCol[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
		}
		else {
			if(i == 2)	obj = nfui_combobox_new(strOper, 2, 0);
			else 	  	obj = nfui_combobox_new(strDur, NUM_DURATIONS, 0);
			nfui_combobox_set_display_string(NF_COMBOBOX(obj), strCol[i]);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_2);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, 164, 40);
			nfui_nfobject_show(obj);

			if(i == 2) 	nfui_regi_post_event_callback(obj, post_oper_all_event_handler);
			else		nfui_regi_post_event_callback(obj, post_dur_all_event_handler);
		}

		nfui_nftable_attach((NFTABLE*)tbl, obj, i, 0);
	}


	for(i=0; i<DVR_ALARM_OUT; i++) {
		if(NVR_RELAY_OUT > i)  g_sprintf(buf, "R%d", i + 1);
		else					g_sprintf(buf, "AO %d", (i - NVR_RELAY_OUT) + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i + 1);
	}

	for(i=0; i<DVR_ALARM_OUT; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_aod[i].name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_regi_post_event_callback(obj, post_name_event_handler);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 1, i + 1);

		g_nameObj[i] = obj;
	}

/*
	for(i=0; i<DVR_ALARM_OUT; i++) {
		if(g_aod[i].type == 0) 		obj = (NFOBJECT*)nfui_nflabel_new_text_box("RELAY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		else if(g_aod[i].type == 1) obj = (NFOBJECT*)nfui_nflabel_new_text_box("DIGITAL OUT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nftable_attach((NFTABLE*)tbl, obj, 2, i + 1);
	}
*/

	for(i=0; i<DVR_ALARM_OUT; i++) {
		g_operObj[i] = nfui_combobox_new(strOper, 2, (gint)g_aod[i].oper);
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_operObj[i]), NFCOMBOBOX_TYPE_SUBTAB_1);
		nfui_combobox_set_align(NF_COMBOBOX(g_operObj[i]), NFALIGN_CENTER, 0);
		nfui_nfobject_show(g_operObj[i]);

		nfui_nftable_attach((NFTABLE*)tbl, g_operObj[i], 2, i + 1);
	}

	for(i=0; i<DVR_ALARM_OUT; i++) {
		g_durObj[i] = nfui_combobox_new(strDur, NUM_DURATIONS, (guint)conv_duration_to_index(g_aod[i].duration));
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_durObj[i]), NFCOMBOBOX_TYPE_SUBTAB_1);
		nfui_combobox_set_align(NF_COMBOBOX(g_durObj[i]), NFALIGN_CENTER, 0);
		nfui_nfobject_show(g_durObj[i]);
		nfui_nftable_attach((NFTABLE*)tbl, g_durObj[i], 3, i + 1);
	}

	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", 164, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", 164, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	for(i=0; i<DVR_ALARM_OUT+1; i++) {
		if(DVR_ALARM_OUT <= 1 && i == 0) continue;

		g_tstObj[i] = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_tstObj[i]), NFCHECK_TYPE_DEF);
		if(i == 0) nfui_check_set_text(NF_CHECKBUTTON(g_tstObj[i]), "TEST ALL");
		else 	   nfui_check_set_text(NF_CHECKBUTTON(g_tstObj[i]), "TEST");
		nfui_check_button_set_inactive_image(NF_CHECKBUTTON(g_tstObj[i]), chk_inact_img);
		nfui_check_button_set_active_image(NF_CHECKBUTTON(g_tstObj[i]), chk_act_img);	
		nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(g_tstObj[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
		nfui_check_set_active_pango_font(NF_CHECKBUTTON(g_tstObj[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);		
		nfui_regi_post_event_callback(g_tstObj[i], post_test_event_handler);
		nfui_nfobject_set_size(g_tstObj[i], 164, 40);
		nfui_nfobject_show(g_tstObj[i]);

		if(i > 0) nfui_nfobject_set_data(g_tstObj[i], "alarm out ch", GINT_TO_POINTER(i - 1));
		else	  nfui_nfobject_set_data(g_tstObj[i], "alarm out ch", GINT_TO_POINTER(DVR_ALARM_OUT));

		nfui_nftable_attach((NFTABLE*)tbl, g_tstObj[i], 4, i);
	}

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

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

gboolean check_nvr_alarm_out_data_changed()
{
	gboolean act = FALSE;
	mb_type mb_ret = NFTOOL_MB_NONE;
	gint i, j;

	for(i=0, j=0; i<DVR_ALARM_OUT+1; i++) {
		act = nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj[i]));
		if(act) {
			if(mb_ret == NFTOOL_MB_NONE)
				mb_ret = nftool_mbox(g_curwnd, "NOTICE", "For all channels to stop the alarm test.", NFTOOL_MB_OK);

			if(i != 0) {
				j = i - 1;
				if(g_aod[j].oper) nf_action_relay_test(FALSE, (j >= GUI_CHANNEL_CNT ? (j - GUI_CHANNEL_CNT + 16) : j), TRUE);
				else              nf_action_relay_test(FALSE, (j >= GUI_CHANNEL_CNT ? (j - GUI_CHANNEL_CNT + 16) : j), FALSE);

	            nfui_nfobject_enable(g_operObj[j]);
				nfui_nfobject_enable(g_durObj[j]);
			}
			NF_CHECKBUTTON(g_tstObj[i])->checked = FALSE;
		}
	}

	get_data_from_obj();

	if(!memcmp(g_oaod, g_aod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT))
		return FALSE;

	return TRUE;
}

void save_nvr_alarm_out_data()
{
	set_ao_data();
}

void restore_nvr_alarm_out_data()
{
	g_memmove(g_aod, g_oaod, sizeof(EA_AlmOutData) * DVR_ALARM_OUT);

	set_data_to_obj(0);
}
