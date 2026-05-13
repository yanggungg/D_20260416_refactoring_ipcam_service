
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
#include "vw_event_mobilepush_noti.h"

#include "vw_evt_act_main.h"

// FIXME
#include "nf_action.h"
#include "log.h"
#include "scm.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_frqObj;
static NFOBJECT *g_jpegChk;

static gint g_frq = 0;
static gint g_ofrq = 0;

enum {
	FREQ_IMMEDIATELY = 0,
	FREQ_1_MIN,
	FREQ_5_MIN,
	FREQ_10_MIN,
	FREQ_15_MIN,
	FREQ_30_MIN,
	FREQ_60_MIN,
	
	NUM_FREQS
};

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

static guint prvIndexToFreq(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case FREQ_IMMEDIATELY:		ret = 0;	break;
		case FREQ_1_MIN:			ret = 1;	break;
		case FREQ_5_MIN:			ret = 5;	break;
		case FREQ_10_MIN:			ret = 10;	break;
		case FREQ_15_MIN:			ret = 15;	break;
		case FREQ_30_MIN:			ret = 30;	break;
		case FREQ_60_MIN:			ret = 60;	break;
		default:					ret = 0;	break;
	}

	return ret;
}

static gint prvFreqToIndex(guint freq)
{
	gint ret = 0;

	if(freq == 0)		ret = FREQ_IMMEDIATELY;
	else if(freq < 5)		ret = FREQ_1_MIN;
	else if(freq < 10)	ret = FREQ_5_MIN;
	else if(freq < 15)	ret = FREQ_10_MIN;
	else if(freq < 30)	ret = FREQ_15_MIN;
	else if(freq < 60)	ret = FREQ_30_MIN;
	else				ret = FREQ_60_MIN;

	return ret;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint index;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_frqObj);
		g_frq = prvIndexToFreq((guint)index);
		
		if(g_frq != g_ofrq) {
			g_frq= g_ofrq;

			index = prvFreqToIndex(g_frq);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_frqObj, index);
			nfui_signal_emit(g_frqObj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint index;
		int ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_frqObj);
		g_frq = prvIndexToFreq(index);

		if(g_frq != g_ofrq) {
			g_ofrq = g_frq;

			ret = DAL_set_evtNoti_mobilepush_frequency(g_frq);
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


void VW_Init_EvtNoti_MobilePush_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed, *chk_fixed;
	NFOBJECT *obj;

	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	gchar *strFrq[] = {
		"IMMEDIATELY",	
		"1 MIN", 
		"5 MIN", 
		"10 MIN",
		"15 MIN", 
		"30 MIN", 
		"60 MIN"
	};
	
	guint chk_w, chk_h;
	gint pos_x, pos_y;

	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	pos_x = 27;
	pos_y = 13;

	/* title */
	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "MOBILE PUSH");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_y += 61;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MINIMUM FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	g_frq = DAL_get_evtNoti_mobilepush_frequency();
	g_ofrq = g_frq;

	g_frqObj = nfui_combobox_new(strFrq, NUM_FREQS, (guint)prvFreqToIndex(g_frq));
	nfui_combobox_set_skin_type(NF_COMBOBOX(g_frqObj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(g_frqObj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_frqObj, 220, 40);
	nfui_nfobject_show(g_frqObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_frqObj, pos_x+454, pos_y);
#if 0
    pos_y += 44;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCLUDE SNAPSHOT IMAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	chk_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(chk_fixed, 220, 40);
	nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_show(chk_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, chk_fixed, pos_x+454, pos_y);

	obj = nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_button_set_active(NF_CHECKBUTTON(obj), FALSE);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (220-chk_w)/2, (40-chk_h)/2);
	g_jpegChk = obj;
	nfui_nfobject_disable(g_jpegChk);
#endif


	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", 164, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", 164, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", 164, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", 164, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", 164, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);
	
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

gboolean check_evt_noti_mobilepush_changed()
{
	guint index;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_frqObj);
	g_frq = prvIndexToFreq(index);

	if(g_frq != g_ofrq) 
		return TRUE;

	return FALSE;
}

void save_evt_noti_mobilepush_data()
{
	g_ofrq = g_frq;

	DAL_set_evtNoti_mobilepush_frequency(g_frq);

	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

void restore_evt_noti_mobilepush_data()
{
	guint index;

	g_frq = g_ofrq;

	index = prvFreqToIndex(g_frq);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_frqObj, index);
}
