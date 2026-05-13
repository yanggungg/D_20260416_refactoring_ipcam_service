
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
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"

#include "vw_event_noti_evt.h"
#include "vw_event_buzzer_noti.h"

#include "vw_evt_act_main.h"

// FIXME
#include "nf_action.h"
#include "log.h"
#include "scm.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_durObj;
static NFOBJECT *g_tstObj;

static gint g_dur = 0;
static gint g_odur = 0;


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

static gint conv_index_to_duration(guint index)
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


static gboolean post_test_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
		gboolean act = FALSE;

		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));
		nf_action_buzzer_test(act);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint index;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_durObj);
		g_dur = conv_index_to_duration((guint)index);
		
		if(g_dur != g_odur) {
			g_dur = g_odur;

			index = conv_duration_to_index(g_dur);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj, index);
			nfui_signal_emit(g_durObj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint index;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_durObj);
		g_dur = conv_index_to_duration(index);

		if(g_dur != g_odur) {
			g_odur = g_dur;

			DAL_set_evtNoti_buzzer_duration(g_dur);
	
			scm_put_log(CHANGE_EVT_NOTI, 0, 0);

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

		VW_EvtNotiEvt_tab_out_handler();
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


void VW_Init_EvtNoti_Buzzer_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;

	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	gchar *strDur[] = {"TRANSPARENT", "5 SEC", "10 SEC", "15 SEC", "20 SEC", 
		"30 SEC", "40 SEC", "60 SEC", "120 SEC", 
		"180 SEC", "300 SEC", "UNTIL KEY"};

	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	/* title */
	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "BUZZER");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 13);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DURATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 230, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 74);

	g_dur = DAL_get_evtNoti_buzzer_duration();
	g_odur = g_dur;

	g_durObj = nfui_combobox_new(strDur, NUM_DURATIONS, (guint)conv_duration_to_index(g_dur));
	nfui_combobox_set_skin_type(NF_COMBOBOX(g_durObj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(g_durObj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_durObj, 164, 40);
	nfui_nfobject_show(g_durObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_durObj, 307, 74);


	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", 164, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", 164, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);
	
	//obj = nftool_normal_button_create_type3("TEST", 164);
	//nfui_nfobject_show(obj);


	g_tstObj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_tstObj), NFCHECK_TYPE_DEF);
	nfui_check_set_text(NF_CHECKBUTTON(g_tstObj), "TEST");
	nfui_check_button_set_inactive_image(NF_CHECKBUTTON(g_tstObj), chk_inact_img);
	nfui_check_button_set_active_image(NF_CHECKBUTTON(g_tstObj), chk_act_img);	
	nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(g_tstObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
	nfui_check_set_active_pango_font(NF_CHECKBUTTON(g_tstObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);
	nfui_nfobject_set_size(g_tstObj, 164, 40);
	nfui_nfobject_show(g_tstObj);
	nfui_regi_post_event_callback(g_tstObj, post_test_button_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_tstObj, 481, 74);


	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_evt_noti_buzzer_changed()
{
	guint index;
	gboolean act = FALSE;

	act = nfui_check_button_get_active(NF_CHECKBUTTON(g_tstObj));
	if(act) {
		nftool_mbox(g_curwnd, "NOTICE", "Stop test", NFTOOL_MB_OK);

		nf_action_buzzer_test(FALSE);

		//nfui_check_button_set_active(NF_CHECKBUTTON(g_tstObj), FALSE);
		NF_CHECKBUTTON(g_tstObj)->checked = FALSE;
	}

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_durObj);
	g_dur = conv_index_to_duration(index);

	if(g_dur != g_odur) 
		return TRUE;

	return FALSE;
}

void save_evt_noti_buzzer_data()
{
	g_odur = g_dur;

	DAL_set_evtNoti_buzzer_duration(g_dur);

	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

void restore_evt_noti_buzzer_data()
{
	guint index;

	g_dur = g_odur;

	index = conv_duration_to_index(g_dur);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_durObj, index);
}
