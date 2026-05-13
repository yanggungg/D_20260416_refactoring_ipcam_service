
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
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"

#include "vw_event_noti_evt.h"
#include "vw_event_disp_noti.h"

#include "vw_evt_act_main.h"
#include "log.h"
#include "scm.h"

#define MULTI_VIEW_DISPLAY_SUPPORT
#undef MULTI_VIEW_DISPLAY_SUPPORT

enum {
	DURATION_5_SEC = 0,
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

static EA_EvtNotiDispData g_end;
static EA_EvtNotiDispData g_oend;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_vDurObj;
static NFOBJECT *g_vMuliObj;
static NFOBJECT *g_oDurObj;



static void init_sd_data();
static void set_sd_data();
static void set_data_to_obj(gint expose);
static void get_data_from_obj();

static guint conv_duration_to_index(guint duration);
static guint conv_index_to_duration(guint index);



static void init_en_data()
{
	memset(&g_end, 0x00, sizeof(EA_EvtNotiDispData));
	memset(&g_oend, 0x00, sizeof(EA_EvtNotiDispData));

	DAL_get_evtNoti_disp_data(&g_end);
	//g_message("%s ::::::::::::::::::::::::::::", __FUNCTION__);
	//g_message(":::::::::::::::::::::::::::::::: %d ", g_end.vpop_duration);
	//g_message(":::::::::::::::::::::::::::::::: %d ", g_end.vpop_multi);
	//g_message(":::::::::::::::::::::::::::::::: %d ", g_end.vpop_spot);

	//g_message(":::::::::::::::::::::::::::::::: %d ", g_end.opop_duration);
	//g_message(":::::::::::::::::::::::::::::::: %d ", g_end.opop_spot);

	g_memmove(&g_oend, &g_end, sizeof(EA_EvtNotiDispData));
}

static void set_en_data()
{
	g_memmove(&g_oend, &g_end, sizeof(EA_EvtNotiDispData));

	DAL_set_evtNoti_disp_data(g_end);
	
	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	if (expose)
	{
		nfui_combobox_set_index((NFCOMBOBOX*)g_vDurObj, conv_duration_to_index(g_end.vpop_duration));
		nfui_signal_emit(g_vDurObj, GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index((NFSPINBUTTON*)g_vMuliObj, g_end.vpop_multi);
		nfui_signal_emit(g_vMuliObj, GDK_EXPOSE, TRUE);

		nfui_combobox_set_index((NFCOMBOBOX*)g_oDurObj, conv_duration_to_index(g_end.opop_duration));
		nfui_signal_emit(g_oDurObj, GDK_EXPOSE, TRUE);
	}
	else
	{
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_vDurObj, conv_duration_to_index(g_end.vpop_duration));
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_vMuliObj, g_end.vpop_multi);

		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_oDurObj, conv_duration_to_index(g_end.opop_duration));
	}
}

static void get_data_from_obj()
{
	gint index;

	index = (guchar)nfui_combobox_get_cur_index((NFCOMBOBOX*)g_vDurObj);
	g_end.vpop_duration = conv_index_to_duration(index);
	g_end.vpop_multi = (guchar)nfui_spin_button_get_index((NFSPINBUTTON*)g_vMuliObj);


	index = (guchar)nfui_combobox_get_cur_index((NFCOMBOBOX*)g_oDurObj);
	g_end.opop_duration = conv_index_to_duration(index);
}

static guint conv_duration_to_index(guint duration)
{
	if(duration < 10)			return DURATION_5_SEC;
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

static guint conv_index_to_duration(guint index)
{
	if(index == DURATION_5_SEC)				return 5;	
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

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(&g_end, &g_oend, sizeof(EA_EvtNotiDispData));

		set_data_to_obj(1);
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

		if(memcmp(&g_oend, &g_end, sizeof(EA_EvtNotiDispData))) {
			set_en_data();

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			event_act_data_changed(TRUE);
		}

	}
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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


void VW_Init_EvtNoti_Display_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	gchar *strDur[] = {"5 SEC", "10 SEC", "15 SEC", "20 SEC", 
		"30 SEC", "40 SEC", "60 SEC", "120 SEC", 
		"180 SEC", "300 SEC", "UNTIL KEY"};
	gchar *strOnOff[] = {"ON", "OFF"};


	g_curwnd = nfui_nfobject_get_top(parent);

	// init data
	init_en_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	/*
	chk_img[0] = nfui_get_image_from_file((IMG_NFCHECK_NOR_INACTIVE), NULL);
	chk_img[1] = nfui_get_image_from_file((IMG_NFCHECK_SEC_ACTIVE), NULL);
	chk_img[2] = nfui_get_image_from_file((IMG_NFCHECK_NOR_INACTIVE), NULL);
	chk_img[3] = nfui_get_image_from_file((IMG_NFCHECK_SEC_ACTIVE), NULL);
	chk_img[4] = nfui_get_image_from_file((IMG_NFCHECK_DIS_INACTIVE), NULL);
	chk_img[5] = nfui_get_image_from_file((IMG_NFCHECK_DIS_ACTIVE), NULL);
	*/


	/* title */
	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "VIDEO POPUP");
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

	g_vDurObj = nfui_combobox_new(strDur, NUM_DURATIONS, conv_duration_to_index(g_end.vpop_duration));
	nfui_combobox_set_skin_type(NF_COMBOBOX(g_vDurObj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(g_vDurObj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_vDurObj, 172, 40);
	nfui_nfobject_show(g_vDurObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_vDurObj, 473, 74);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MULTI VIEW DISPLAY DURING SIMULTANEOUS EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 118);
#ifndef MULTI_VIEW_DISPLAY_SUPPORT
	nfui_nfobject_hide(obj);
#endif

	g_vMuliObj = nfui_spinbutton_new(strOnOff, 2, g_end.vpop_multi);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_vMuliObj, NFSPINBUTTON_TYPE_SUBTAB_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)g_vMuliObj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_vMuliObj, 164, 40);
	nfui_nfobject_show(g_vMuliObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_vMuliObj, 481, 118);
#ifndef MULTI_VIEW_DISPLAY_SUPPORT
	nfui_nfobject_hide(g_vMuliObj);
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////


	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "OSD POPUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 242);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DURATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 230, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 302);

	g_oDurObj = nfui_combobox_new(strDur, NUM_DURATIONS, conv_duration_to_index(g_end.opop_duration));
	nfui_combobox_set_skin_type(NF_COMBOBOX(g_oDurObj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(g_oDurObj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_oDurObj, 172, 40);
	nfui_nfobject_show(g_oDurObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_oDurObj, 473, 302);



	////////////////////////////////////////////////////////////////////////////////////////////////////


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
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_evt_noti_disp_changed()
{
	get_data_from_obj();

	if(!memcmp(&g_oend, &g_end, sizeof(EA_EvtNotiDispData)))
		return FALSE;

	return TRUE;
}

void save_evt_noti_disp_data()
{
	set_en_data();
}

void restore_evt_noti_disp_data()
{
	g_memmove(&g_end, &g_oend, sizeof(EA_EvtNotiDispData));

	set_data_to_obj(0);
}

