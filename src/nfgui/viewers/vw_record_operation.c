
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_operation.h"
#include "vw_record_auto_popup.h"
#include "vw_record_auto_intensive_popup.h"
#include "ix_mem.h"
#include "vw_passage_popup.h"

#define HELP_STR1 "HD IMAGE QUALITY PRIORITY"
#define HELP_STR2 " : NVR will record at the highest FPS, resolution, and image quality supported by the IP camera."
#define HELP_STR3 "HD RECORDING DATE PRIORITY"
#define HELP_STR4 " : NVR will record for the greatest number of days at normal image quality."

// AUTO MODE INDEX.
enum{
	AU_HIGH_QUAL 			= 0,
	AU_MOT 					= 1,
	AU_ALARM 				= 2,
	AU_MOT_ALARM 			= 3,
	AU_INTENSIVE_MOT 		= 4,
	AU_INTENSIVE_ALARM 		= 5,
	AU_INTENSIVE_MOT_ALARM 	= 6,
	AU_PANIC_TIME			= 7,
	AU_SSC                  = 8,
	AU_CONFIG_MAX
};

// MANUAL MODE INDEX.
enum{
	MA_SCHE_MODE 			= 0,
	MA_PRE_TIME				= 1,
	MA_POST_TIME			= 2,
	MA_PANIC_TIME			= 3,
	MA_SSC                  = 4,
	MA_CONFIG_MAX
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_confFixed[CONFIG_CNT];
static NFOBJECT *g_confMode[CONFIG_CNT];

static RecordOperData *oper_data = NULL;

static NFOBJECT *auto_obj[AU_CONFIG_MAX];
static NFOBJECT *manual_obj[MA_CONFIG_MAX];
static NFOBJECT *priority_obj;
static NFOBJECT *help_obj;


static void _set_oper_data_to_objs(gboolean redraw)
{
	get_record_auto_db();

	if (oper_data->mode != get_record_mode()) {	
		if(get_record_mode() == AUTO_CONFIG) {	
			nfui_nfobject_hide(g_confFixed[MANUAL_CONFIG]);
			nfui_nfobject_show(g_confFixed[AUTO_CONFIG]);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_confMode[AUTO_CONFIG], AUTO_CONFIG);
		}
		else if (get_record_mode() == MANUAL_CONFIG) {			
			nfui_nfobject_hide(g_confFixed[AUTO_CONFIG]);
			nfui_nfobject_show(g_confFixed[MANUAL_CONFIG]);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_confMode[MANUAL_CONFIG], MANUAL_CONFIG);
		}

		oper_data->mode = get_record_mode();
		VW_RecordSetup_change_config_mode(oper_data->mode);		
	}

	if (oper_data->smart_storage != get_record_ssc()) {
	    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)auto_obj[AU_SSC], get_record_ssc());
	    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)manual_obj[MA_SSC], get_record_ssc());
	    oper_data->smart_storage = get_record_ssc();
	}

	if(oper_data->autoConfig != get_record_autoConfig()) {
		nfui_radio_button_set_toggled(NF_BUTTON(auto_obj[get_record_autoConfig()]), TRUE);
		oper_data->autoConfig = get_record_autoConfig();

        if (oper_data->autoConfig == AU_HIGH_QUAL) {
            nfui_nfobject_enable(priority_obj);
            nfui_nfobject_enable(help_obj);
		}
        else {
            nfui_nfobject_disable(priority_obj);
            nfui_nfobject_disable(help_obj);
		}
	}

	if(oper_data->priMode != get_record_priMode()) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)priority_obj, get_record_priMode());
		oper_data->priMode = get_record_priMode();
	}

	if(oper_data->schedMode != get_record_schedMode()) {
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)manual_obj[MA_SCHE_MODE], get_record_schedMode());
		oper_data->schedMode = get_record_schedMode();
	}

	if(oper_data->preRecTime != get_record_pre_time()) {
		gchar buf[10];
		g_sprintf(buf, SECOND_TIME_STRING, get_record_pre_time());
		nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_PRE_TIME], buf);
		oper_data->preRecTime = get_record_pre_time();
	}

	if(oper_data->postRecTime != get_record_post_time()) {
		gchar buf[10];
		g_sprintf(buf, SECOND_TIME_STRING, get_record_post_time());
		nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_POST_TIME], buf);
		oper_data->postRecTime = get_record_post_time();
	}

	if(oper_data->panicTime != get_record_panic_time()) {
		gchar buf[10];

		if (get_record_panic_time() < 60)
		{
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)auto_obj[AU_PANIC_TIME], recPanicTime[0]);
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_PANIC_TIME], recPanicTime[0]);
		}
		else
		{
			g_sprintf(buf, MINUTE_TIME_STRING, get_record_panic_time()/60);

			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)auto_obj[AU_PANIC_TIME], buf);
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_PANIC_TIME], buf);
		}
		oper_data->panicTime = get_record_panic_time();
	}

	if (redraw)
	{
		nfui_signal_emit(g_confFixed[oper_data->mode], GDK_EXPOSE, TRUE);
	}
}

static gboolean post_autoMode_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint index = -1;

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if(index < 0) return FALSE;

		if(nfui_nfobject_is_shown(g_confFixed[AUTO_CONFIG])) {
			if(index == MANUAL_CONFIG) {			
				nfui_nfobject_hide(g_confFixed[AUTO_CONFIG]);
				nfui_nfobject_show(g_confFixed[MANUAL_CONFIG]);
				nfui_combobox_set_index((NFCOMBOBOX*)g_confMode[MANUAL_CONFIG], MANUAL_CONFIG);

				nfui_signal_emit(g_confFixed[MANUAL_CONFIG], GDK_EXPOSE, TRUE);
				VW_RecordSetup_change_config_mode(MANUAL_CONFIG);

				oper_data->mode = index;
			}
		}
	}

	return FALSE;
}

static gboolean post_auto_panicTime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *pTime;
	gint tmp;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		pTime = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_panicTime_into_info(pTime);
		if(tmp < 0) 
			g_warning("record_operation.c: panic time is NULL...");
		else
		{
			oper_data->panicTime = tmp;
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_PANIC_TIME], pTime);			
		}
	}

	return FALSE;
}

static gboolean post_manualMode_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint index = -1;

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if(index < 0) return FALSE;

		if(nfui_nfobject_is_shown(g_confFixed[MANUAL_CONFIG])) {
			if(index == AUTO_CONFIG) {	
				nfui_nfobject_hide(g_confFixed[MANUAL_CONFIG]);
				nfui_nfobject_show(g_confFixed[AUTO_CONFIG]);
				nfui_combobox_set_index((NFCOMBOBOX*)g_confMode[AUTO_CONFIG], AUTO_CONFIG);

				nfui_signal_emit(g_confFixed[AUTO_CONFIG], GDK_EXPOSE, TRUE);
				VW_RecordSetup_change_config_mode(AUTO_CONFIG);
				
				oper_data->mode = index;
			}
		}
	}

	return FALSE;
}

static gboolean post_auto_config_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, ret_val;
		guint i;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		switch(index)
		{
			case AU_HIGH_QUAL:
				ret_val = TRUE;			
			break;
			case AU_MOT:
				ret_val = VW_RecAutoPopup_Open(g_curwnd, autoMot_info, OPEN_AUTO_MOT);
			break;
			case AU_ALARM:
				ret_val = VW_RecAutoPopup_Open(g_curwnd, autoAlarm_info, OPEN_AUTO_ALARM);
			break;
			case AU_MOT_ALARM:
				ret_val = VW_RecAutoPopup_Open(g_curwnd, autoMotAlarm_info, OPEN_AUTO_MOT_ALARM);
			break;
			case AU_INTENSIVE_MOT:
				ret_val = VW_RecAutoItsPopup_Open(g_curwnd, autoItsMot_info, OPEN_AUTO_ITS_MOT);
			break;
			case AU_INTENSIVE_ALARM:
				ret_val = VW_RecAutoItsPopup_Open(g_curwnd, autoItsAlarm_info, OPEN_AUTO_ITS_ALARM);
			break;
			case AU_INTENSIVE_MOT_ALARM:
				ret_val = VW_RecAutoItsPopup_Open(g_curwnd, autoItsMotAlarm_info, OPEN_AUTO_ITS_MOT_ALARM);
			break;
			default:
				g_assert(0);
			break;
		}
		
		if (ret_val)
		{
			oper_data->autoConfig = (guint)index;
            nfui_nfobject_disable(priority_obj);
            nfui_nfobject_disable(help_obj);

			switch(index)
			{
    			case AU_HIGH_QUAL:
				    nfui_nfobject_enable(priority_obj);
				    nfui_signal_emit(priority_obj, GDK_EXPOSE, TRUE);

				    nfui_nfobject_enable(help_obj);
				    nfui_signal_emit(help_obj, GDK_EXPOSE, TRUE);
    			break;			
				case AU_MOT:
					set_record_auto_info(AUTO_CONFIG_MOT);
				break;
				case AU_ALARM:
					set_record_auto_info(AUTO_CONFIG_ALARM);
				break;
				case AU_MOT_ALARM:
					set_record_auto_info(AUTO_CONFIG_MOT_ALARM);
				break;
				case AU_INTENSIVE_MOT:
					set_record_auto_info(AUTO_CONFIG_ITS_MOT);
				break;
				case AU_INTENSIVE_ALARM:
					set_record_auto_info(AUTO_CONFIG_ITS_ALARM);
				break;
				case AU_INTENSIVE_MOT_ALARM:
					set_record_auto_info(AUTO_CONFIG_ITS_MOT_ALARM);
				break;
			}

//		    nfui_signal_emit(priority_obj, GDK_EXPOSE, TRUE);
		}
		else
		{
			nfui_clear_key_focus(auto_obj[index]);
			nfui_set_key_focus(auto_obj[oper_data->autoConfig], TRUE);
		
			nfui_radio_button_set_toggled(NF_BUTTON(auto_obj[oper_data->autoConfig]), TRUE);
			for (i=0; i<7; i++)
			{
				nfui_signal_emit(auto_obj[i], GDK_EXPOSE, FALSE);
			}
		}
	}

	return FALSE;
}

static gboolean post_priority_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *mode;
	gint tmp;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		mode = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_priMode_into_info(mode);
		
		if(tmp < 0)
			g_warning("record_operation.c: priority mode is error...");
		else
			oper_data->priMode = (guint)tmp;
	}

	return FALSE;
}

static gboolean post_ssc_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ssc;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		ssc = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		
		oper_data->smart_storage = ssc;
		
		if(obj == auto_obj[AU_SSC])
		    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)manual_obj[MA_SSC], ssc);
		else
		    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)auto_obj[AU_SSC], ssc);
	}

	return FALSE;
}

static gboolean post_manual_scheMode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *mode;
	gint tmp;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		mode = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_schedMode_into_info(mode);
		if(tmp < 0)
			g_warning("record_operation.c: schedule mode is error...");
		else
			oper_data->schedMode = (guint)tmp;
	}

	return FALSE;
}


static gboolean post_manual_preTime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *pTime;
	gint tmp;
	
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		pTime = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_preTime_into_info(pTime);
		if(tmp < 0)	
			g_warning("record_operation.c: preTime is NULL...");
		else
			oper_data->preRecTime = tmp;

		if (tmp > 5) 
			nftool_mbox(g_curwnd, "WARNING", "If you select pre-recording time higher than 5 seconds,\nlow resolutions are applied.", NFTOOL_MB_OK);
	}

	return FALSE;
}


static gboolean post_manual_postTime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *pTime;
	gint tmp;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		pTime = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_postTime_into_info(pTime);
		if(tmp < 0) 
			g_warning("record_operation.c: postTime is NULL...");
		else
			oper_data->postRecTime = tmp;
	}

	return FALSE;
}

static gboolean post_manual_panicTime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *pTime;
	gint tmp;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		pTime = nfui_combobox_get_value(NF_COMBOBOX(obj));

		tmp = translate_panicTime_into_info(pTime);
		if(tmp < 0) 
			g_warning("record_operation.c: panic time is NULL...");
		else
		{
			oper_data->panicTime = tmp;
			nfui_combobox_set_data_no_expose((NFCOMBOBOX*)auto_obj[AU_PANIC_TIME], pTime);			
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean oper_change;
	gboolean auto_change;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		oper_change = is_changed_operation_data(oper_data);
		auto_change = is_changed_auto_data();

		if (oper_change || auto_change)
		{
			if (oper_change) 	set_record_operation_data(oper_data);
			if (auto_change) 	set_record_auto_db();

			vw_send_notify_change_record_data();
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		}	
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		_set_oper_data_to_objs(TRUE);
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

		VW_RecOperation_tab_out_handler();
		VW_RecordSetup_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_helpbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		NFOBJECT *top;
		guint x, y;
        PARAGRAPH_STR *para;
        gint i, cnt = 0;
		
  	   	if (evt->button.button == MOUSE_RIGTH_BUTTON)  return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x;
		y += top->y + obj->height + 2;

        para = imalloc(sizeof(PARAGRAPH_STR));

        para->intro[0] = g_strdup(HELP_STR1);
        para->intro[1] = g_strdup(HELP_STR2);
        para->intro[2] = g_strdup(HELP_STR3);
        para->intro[3] = g_strdup(HELP_STR4);

		para->intro_type = BULLET_BLANK;
        para->intro_cnt = 4;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
	}

	return FALSE;
}

static gboolean post_manual_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
		}
		break;

		case GDK_DELETE :
			if(oper_data) ifree(oper_data);
			oper_data = NULL;
			break;

		default :
			break;
	}

	return FALSE;

}

static gboolean _get_recOper_data()
{
	oper_data = imalloc(sizeof(RecordOperData));	
	if(!oper_data)
		g_error("%s : record operation alloc error", __FUNCTION__);

	get_record_operation_data(oper_data);

	return TRUE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


////////////////////////////////////////////////////////////////////
//
//
//


void VW_Init_RecOperation_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;

	gchar *strImg[] = {"RECORDING CONFIGURATION MODE", 
						"AUTOMATIC RECORD CONFIGURATION MODE", 
						"PANIC RECORDING OPTIONS",
						"MANUAL CONFIGURATION OPTIONS"};

	gchar *strMode[] = {"AUTO CONFIGURATION",
						"MANUAL CONFIGURATION"};

	gchar *strLabel[] = {"MODE",
	                    "SMART STORAGE COMPRESSION",
						"CONTINUOUS RECORD",
						"MOTION RECORD",
						"AI/ALARM RECORD",
						"MOTION/AI/ALARM RECORD",
						"INTENSIVE MOTION RECORD",
						"INTENSIVE AI/ALARM RECORD",
						"INTENSIVE MOTION/AI/ALARM RECORD",
						"PANIC RECORDING TIME"};

	gchar *strLabel2[] = {"MODE",
	                    "SMART STORAGE COMPRESSION",
						"SCHEDULE MODE",
						"PRE RECORDING TIME",
						"POST RECORDING TIME",
						"PANIC RECORDING TIME"};

	gchar *strOffOn[] = {"OFF", "ON"};

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	gint size_w, size_h;
	gint i;

	g_curwnd = nfui_nfobject_get_top(parent);

////////////////////////////////////////////////////////////
//
// 	LOAD.
//
	radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_OFF), NULL);
	
	_get_recOper_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

////////////////////////////////////////////////////////////
//
// AUTO & MANUAL FIXED.
//
	for (i = 0; i < CONFIG_CNT; i++) {
		g_confFixed[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_modify_bg(g_confFixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_set_size(g_confFixed[i], MENU_V_INNER_W, MENU_V_INNER_H);

		if (i == oper_data->mode) 
			nfui_nfobject_show(g_confFixed[i]);
		else
		{
			nfui_nfobject_hide(g_confFixed[i]);
			nfui_regi_post_event_callback(g_confFixed[i], post_manual_fixed_event_handler);			
		}
		nfui_nffixed_put((NFFIXED*)content_fixed, g_confFixed[i], 0, 0);
	}


////////////////////////////////////////////////////////////
//
// AUTO CONFIGURATION.
//
	// title.
	for (i = 0; i < 3; i++) {
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, strImg[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);

		if (i == 0) 		
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 8, 0);
		else if (i == 1) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 8, 201);
		else
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 8, 614);
	}


	// label.
	for (i = 0; i < 10; i++) {
		if (i == 0 || i == 1 || i == 9) 
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));		
			nfui_nfobject_set_size(obj, 500, 40);
		}
        else if (i == 1)
        {
            size_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strLabel[i], NORMAL_SPACING);           
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(123));		
			nfui_nfobject_set_size(obj, size_w, 40);
        }		
		else 				 
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(123));		
			nfui_nfobject_set_size(obj, 500, 40);
		}
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if (i == 0) 		
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 0+40+21);
		else if (i == 1) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 0+40+21+42);
		else if (i == 2) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*0);
		else if (i == 3) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*1);
		else if (i == 4) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*2);
		else if (i == 5) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*3);
		else if (i == 6) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*4);
		else if (i == 7) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*5);
		else if (i == 8) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6, 201+40+21+42*6);
		else if (i == 9) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 614+40+21);
	}


	// dropdown button.
	for (i = 0; i < 3; i++) {
		if (i == 0) 		// mode combobox button		
			obj = nfui_combobox_new(strMode, 2, 0);
		else if (i == 1)
		    obj = nfui_combobox_new(strOffOn, 2, get_record_ssc());
		else 				// panic rec time combobox button
		{
			gchar buf[10];
			obj = nfui_combobox_new(recPanicTime, REC_PANIC_TIME_MAX, 0);

			if (get_record_panic_time() == 0)
				nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), recPanicTime[0]);	
			else
			{
				gint panic_sec;
				panic_sec = get_record_panic_time();

				if (panic_sec < 60)
				{
					g_sprintf(buf, SECOND_TIME_STRING, get_record_panic_time());
					nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);	
				}
				else
				{
					g_sprintf(buf, MINUTE_TIME_STRING, get_record_panic_time()/60);
					nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);	
				}			
			}
		}
		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);		
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 4);
		nfui_nfobject_set_size(obj, 300, 40);
		nfui_nfobject_show(obj);

		if (i == 0)		
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 500, 0+40+21);
			nfui_regi_post_event_callback(obj, post_autoMode_event_cb);
			g_confMode[AUTO_CONFIG] = obj;
		}
		else if (i == 1)
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 500, 0+40+21+42);
			nfui_regi_post_event_callback(obj, post_ssc_event_handler);
			auto_obj[AU_SSC] = obj;
		}
		else
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 500, 614+40+21);
			nfui_regi_post_event_callback(obj, post_auto_panicTime_event_handler);
			auto_obj[AU_PANIC_TIME] = obj;
		}
	}

	
	// radio button.
	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for (i = 0; i < 7; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_auto_config_event_cb);

		if (i == oper_data->autoConfig)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

		if (i == 0)
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		else
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);

		if (i == 0)
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*0+6);
		else if (i == 1) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*1+6);
		else if (i == 2) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*2+6);
		else if (i == 3) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*3+6);
		else if (i == 4) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*4+6);
		else if (i == 5) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*5+6);
		else
			nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28, 201+40+21+42*6+6);
			
		auto_obj[i] = obj;			
	}

    obj = nfui_combobox_new(recPriMode, REC_PRIORITY_MAX, oper_data->priMode);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 4);
    nfui_nfobject_set_size(obj, 410, 40);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_priority_event_handler);
    priority_obj = obj;

    size_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strLabel[1], NORMAL_SPACING);

	int x_btn = 0;
    if (size_w <= 500) {
        nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 500, 201+40+21+42*0);
		x_btn = 500;
	}
    else {
        nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, 28+27+6+size_w+38, 201+40+21+42*0);
		x_btn = 28+27+6+size_w+38;
	}

    if (oper_data->autoConfig != AU_HIGH_QUAL) 
        nfui_nfobject_disable(priority_obj);


	obj = nftool_normal_button_create_type3("?", 42);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_confFixed[AUTO_CONFIG], obj, x_btn+410+10, 201+40+21+42*0);
	nfui_regi_post_event_callback(obj, post_helpbutton_event_handler);
	help_obj = obj;

    if (oper_data->autoConfig != AU_HIGH_QUAL) 
        nfui_nfobject_disable(help_obj);


////////////////////////////////////////////////////////////
//
// MANUAL CONFIGURATION.
//
	// title.
	for (i = 0; i < 3; i++) {
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		if (i == 1)   
			nfui_nfimage_set_text((NFIMAGE*)obj, strImg[i + 2]);
		else		 
			nfui_nfimage_set_text((NFIMAGE*)obj, strImg[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);

		if (i == 0) 		
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 8, 0);
		else if (i == 1) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 8, 201);
		else
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 8, 447);
	}

	// label.
	for (i = 0; i < 6; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel2[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nfobject_set_size(obj, 500, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if (i == 0) 		
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 0+40+21);
		else if (i == 1) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 0+40+21+42);
		else if (i == 2) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 201+40+21+42*0);
		else if (i == 3) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 201+40+21+42*1);
		else if (i == 4) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 201+40+21+42*2);
		else if (i == 5) 
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 28, 447+40+21);
	}



	// dropdown button.
	for (i = 0; i < 6; i++) {
		if (i == 0) 			
			obj = nfui_combobox_new(strMode, 2, 1);
		else if (i == 1)	
		    obj = nfui_combobox_new(strOffOn, 2, get_record_ssc());
		else if(i == 2) 	
			obj = nfui_combobox_new(recSchMode, REC_SCHED_MODE_MAX, oper_data->schedMode);
		else if(i == 3)
		{
			gchar buf[10];
			g_sprintf(buf, SECOND_TIME_STRING, get_record_pre_time());
			obj = nfui_combobox_new(recPreTime, REC_PRE_TIME_MAX, 0);
			nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);
		}
		else if(i == 4)
		{
			gchar buf[10];
			g_sprintf(buf, SECOND_TIME_STRING, get_record_post_time());
			obj = nfui_combobox_new(recPostTime, REC_POST_TIME_MAX, 0);		
			nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);
		}
		else
		{
			gchar buf[10];
			obj = nfui_combobox_new(recPanicTime, REC_PANIC_TIME_MAX, 0);

			if (get_record_panic_time() == 0)
				nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), recPanicTime[0]);
			else
			{
				gint panic_sec;
				panic_sec = get_record_panic_time();

				if (panic_sec < 60)
				{
					g_sprintf(buf, SECOND_TIME_STRING, get_record_panic_time());
					nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);	
				}
				else
				{
					g_sprintf(buf, MINUTE_TIME_STRING, get_record_panic_time()/60);
					nfui_combobox_set_data_no_expose(NF_COMBOBOX(obj), buf);	
				}			
			}
		}

		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_LEFT, 4);
		nfui_nfobject_set_size(obj, 300, 40);
		nfui_nfobject_show(obj);

		if (i == 0) 		
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 0+40+21);		
			nfui_regi_post_event_callback(obj, post_manualMode_event_cb);
			g_confMode[MANUAL_CONFIG] = obj;
		}
		else if (i == 1)
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 0+40+21+42);
			nfui_regi_post_event_callback(obj, post_ssc_event_handler);
			manual_obj[MA_SSC] = obj;
		}
		else if (i == 2)
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 201+40+21+42*0);		
			nfui_regi_post_event_callback(obj, post_manual_scheMode_event_handler);
			manual_obj[MA_SCHE_MODE] = obj;
		}
		else if (i == 3)
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 201+40+21+42*1);		
			nfui_regi_post_event_callback(obj, post_manual_preTime_event_handler);
			manual_obj[MA_PRE_TIME] = obj;
		}
		else if (i == 4)
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 201+40+21+42*2);		
			nfui_regi_post_event_callback(obj, post_manual_postTime_event_handler);
			manual_obj[MA_POST_TIME] = obj;			
		}
		else
		{
			nfui_nffixed_put((NFFIXED*)g_confFixed[MANUAL_CONFIG], obj, 500, 447+40+21);		
			nfui_regi_post_event_callback(obj, post_manual_panicTime_event_handler);
			manual_obj[MA_PANIC_TIME] = obj;			
		}
	}

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

guint VW_RecOperation_get_pre_time()
{
	return oper_data->preRecTime;
}

guint VW_RecOperation_get_post_time()
{
	return oper_data->postRecTime;
}

void VW_RecOperation_set_pre_time(guint val)
{
	gchar buf[10];	
	g_sprintf(buf, SECOND_TIME_STRING, val);
	nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_PRE_TIME], buf);	
	oper_data->preRecTime = val;
}

void VW_RecOperation_set_post_time(guint val)
{
	gchar buf[10];	
	g_sprintf(buf, SECOND_TIME_STRING, val);
	nfui_combobox_set_data_no_expose((NFCOMBOBOX*)manual_obj[MA_POST_TIME], buf);	
	oper_data->postRecTime = val;
}

gboolean VW_RecOperation_tab_out_handler()
{
	mb_type ret;
	gboolean oper_change;
	gboolean auto_change;

	oper_change = is_changed_operation_data(oper_data);
	auto_change = is_changed_auto_data();

	if (oper_change || auto_change)
	{
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK)
		{
			if (oper_change)	set_record_operation_data(oper_data);	
			if (auto_change)	set_record_auto_db();

			vw_send_notify_change_record_data();
		}
		else 
		{
			_set_oper_data_to_objs(FALSE);
		}
	}

	return TRUE;
}

