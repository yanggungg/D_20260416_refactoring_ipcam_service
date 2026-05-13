#include "nf_afx.h"

#include "nf_record.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_page_manager.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"

#include "vw_record_main.h"
#include "vw_record_calcul.h"
#include "vw_record_data_internal.h"


#define	CALCUL_LABEL_X				(40)
#define	CALCUL_LABEL_Y				(40)

#define	CALCUL_LABEL_HEIGHT			(40)
#define	CALCUL_LABEL_WIDTH			(450)
#define CALCUL_LABEL1_WIDTH			(500)
#define CALCUL_LABEL2_WIDTH			(1300)

#define	CALCUL_CELL_WIDTH			(200)

#define CALCUL_TABLE_X				(CALCUL_LABEL_X)
#define CALCUL_TABLE_Y				(CALCUL_LABEL_Y + 50)

#define CALCUL_TABLE_COLS_SPACE		(4)
#define CALCUL_TABLE_ROWS_SPACE		(2)

#define CALCUL_BTN_X				(CALCUL_LABEL_X + CALCUL_LABEL_WIDTH + 4)
#define CALCUL_BTN_Y				(CALCUL_LABEL_Y + (CALCUL_LABEL_HEIGHT * (ROW_MAX + 2)) + (CALCUL_TABLE_ROWS_SPACE * 3))

#define	CALCUL_WARN_X			(CALCUL_LABEL_X)
#define	CALCUL_WARN_Y			(740)

#define WARN_STR1	"All of the cameras should be connected."
#define WARN_STR2	"This test result may have a different actual recording period from installation environment or connected camera model."


// AUTO MODE INDEX
enum{
	AU_HIGH_QUAL 			= 0,
	AU_MOT 					= 1,
	AU_ALARM 				= 2,
	AU_MOT_ALARM 			= 3,
	AU_INTENSIVE_MOT 		= 4,
	AU_INTENSIVE_ALARM 		= 5,
	AU_INTENSIVE_MOT_ALARM 	= 6,
	AU_CONFIG_MAX
};

enum{	
	ROW_MOTION_EVENT 	= 0,
	ROW_ALARM_EVENT		= 1,
	ROW_MAX
};

enum {
	PERC_0 = 0,
	PERC_1,
	PERC_5,
	PERC_10,
	PERC_20,
	PERC_30,
	PERC_40,
	PERC_50,
	PERC_60,
	PERC_70,
	PERC_80,
	PERC_90,
	PERC_100,

	NUM_PERC
};

static gchar* percentage[] = {	
				"0%",
				"1%",
				"5%",
				"10%",
				"20%",
				"30%",
				"40%",
				"50%",
				"60%",
				"70%",
				"80%",
				"90%",
				"100%"
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *Per_obj[ROW_MAX];
static NFOBJECT *result_obj;
static NFOBJECT *mode_obj;
static NFOBJECT *g_ssc_obj;
static RecordOperData *oper_data = NULL;



static gint per_index(guint index)
{
	if(index == PERC_0) 		return 0;
	else if(index == PERC_1) 	return 1;
	else if(index == PERC_5) 	return 5;
	else if(index == PERC_10)	return 10;
	else if(index == PERC_20) 	return 20;
	else if(index == PERC_30) 	return 30;
	else if(index == PERC_40) 	return 40;
	else if(index == PERC_50) 	return 50;
	else if(index == PERC_60) 	return 60;
	else if(index == PERC_70) 	return 70;
	else if(index == PERC_80) 	return 80;
	else if(index == PERC_90) 	return 90;
	else if(index == PERC_100) 	return 100;

	return 0;
}

static void _set_text_current_mode(guint mode, guint autoConfig)
{
	if (mode == MANUAL_CONFIG)
	{
		nfui_nflabel_set_text((NFLABEL*)mode_obj, "MANUAL CONFIGURATION");
		nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
	}
	else if (mode == AUTO_CONFIG)
	{
		switch(autoConfig)
		{
			case AUTO_CONFIG_HIGH_QUAL:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "CONTINUOUS RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_MOT:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "MOTION RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_ALARM:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "ALARM RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_MOT_ALARM:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "MOTION/ALARM RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_ITS_MOT:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "INTENSIVE MOTION RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_ITS_ALARM:
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "INTENSIVE ALARM RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
			case AUTO_CONFIG_ITS_MOT_ALARM:	
				nfui_nflabel_set_text((NFLABEL*)mode_obj, "INTENSIVE MOTION/ALARM RECORDING");
				nfui_signal_emit(mode_obj, GDK_EXPOSE, TRUE);
			break;
		}
	}
	return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);
			nfui_nfobject_get_offset(obj, &x, &y);
		}
		break;
		default:
			break;
	}
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				
			
		VW_StorageCalcul_tab_out_handler();
		VW_RecordSetup_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_calcul_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint index;
	gchar buf[256];

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NF_RECORD_CALC_PARAM_T param;
		NF_RECORD_CALC_RESULT_T rslt;

		memset(&param,0x00,sizeof(param));
		memset(&rslt,0x00,sizeof(rslt));
		memset(buf,0x00,sizeof(buf));
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		param.mode = NF_RECORD_CALC_CURRENT;
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)Per_obj[0]);
		param.motion_occur_pcnt = per_index(index);
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)Per_obj[1]);
		param.alarm_occur_pcnt = per_index(index);

		if((param.motion_occur_pcnt + param.alarm_occur_pcnt ) > 100)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The sum of the motion event and alarm event ratios has exceeded 100 percent.", NFTOOL_MB_OK);
		}
		else
		{
			if(!nf_record_calculate(param, &rslt))
			{
				nftool_mbox_auto(g_curwnd, 1, "NOTICE", "FAIL");
				return FALSE;
			}
			else
			{
				nftool_mbox_auto(g_curwnd, 1, "NOTICE", "SUCCESS");

				if (rslt.day_full_range == 0) {
					sprintf(buf,"%.1lf %s", rslt.day_full, lookup_string("DAYS"));
				}
				else {
					sprintf(buf,"%.1lf ~ %.1lf %s", rslt.day_full, rslt.day_full_range, lookup_string("DAYS"));
				}

				nfui_nflabel_set_text((NFLABEL*)result_obj, buf);
				nfui_signal_emit((NFLABEL*)result_obj, GDK_EXPOSE, TRUE);
			}
		}
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

void VW_Init_StorageCalcul_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj, *tbl ;
	gchar buf[2];
	gint max_str_w, max_str_w1 = 0; 
	guint mode_idx, auto_idx = 0;
	gint i;
	guint table_w[] = {CALCUL_LABEL_WIDTH, CALCUL_CELL_WIDTH};
	gint pos_x, pos_y;
	gint size_w, size_h;


	const gchar *strCurrent[] = {"CURRENT RECORDING MODE:",
								"ESTIMATED STORAGE PERIOD IN CURRENT SETTING:",
								"DAYS"};

	const gchar *strHour[] = {"AVERAGE MOTION EVENT RATIO PER HOUR",
	       	    	     	"AVERAGE ALARM EVENT RATIO PER HOUR"};


	g_curwnd = nfui_nfobject_get_top(parent);

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);


// CURRENT MODE LABEL
    pos_x = CALCUL_LABEL_X;
    pos_y = CALCUL_LABEL_Y;
    
	max_str_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strCurrent[0], NORMAL_SPACING);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCurrent[0], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, max_str_w, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	mode_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)mode_obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(mode_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(mode_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(mode_obj, CALCUL_LABEL_WIDTH, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(mode_obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, mode_obj, (pos_x + max_str_w + 20), pos_y);

	mode_idx = get_record_mode();
	auto_idx = get_record_autoConfig();

	_set_text_current_mode(mode_idx, auto_idx);

//SSC
    pos_y += CALCUL_LABEL_HEIGHT;
    
 	max_str_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), lookup_string("SMART STORAGE COMPRESSION"), NORMAL_SPACING);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SMART STORAGE COMPRESSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, max_str_w, CALCUL_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(":", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 20, CALCUL_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + max_str_w, pos_y);

    if (get_record_ssc())
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    else
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFF", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, CALCUL_LABEL_WIDTH, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, (pos_x + max_str_w + 20), pos_y);
    g_ssc_obj = obj;
    
// SELECT HOUR
    pos_y += CALCUL_LABEL_HEIGHT * 2;
    
	tbl = (NFOBJECT*)nfui_nftable_new( 2 , ROW_MAX , CALCUL_TABLE_COLS_SPACE, CALCUL_TABLE_ROWS_SPACE, table_w, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, pos_x, pos_y);

	for(i = 0; i < ROW_MAX; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strHour[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

		Per_obj[i] = nfui_combobox_new(percentage, 13, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(Per_obj[i]), NFCOMBOBOX_TYPE_1);
		nfui_combobox_set_align(NF_COMBOBOX(Per_obj[i]), NFALIGN_CENTER, 0);
		nfui_nfobject_show(Per_obj[i]);
		nfui_nftable_attach((NFTABLE*)tbl, Per_obj[i], 1, i);
	}

    nfui_nfobject_get_size(tbl, &size_w, &size_h);

// CALCULATE BUTTON
    pos_y += size_h + 5;
    
	obj = nftool_normal_button_create_subtab_type1("CALCULATE", 200);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CALCUL_BTN_X, pos_y);
	nfui_regi_post_event_callback(obj, post_calcul_btn_event_handler);


// RESULT LABEL
    pos_y += 100;
    
	max_str_w1 = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strCurrent[1], NORMAL_SPACING);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCurrent[1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, max_str_w1, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	result_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAYS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(190));
	nfui_nflabel_set_align((NFLABEL*)result_obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(result_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(result_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(result_obj, 200, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(result_obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, result_obj, (pos_x + max_str_w1 + 40), pos_y);

/*
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCurrent[2], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, (CALCUL_LABEL_X + max_str_w1 + 100), (CALCUL_BTN_Y + 100));
*/


// WARNING LABEL
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WARNING!", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(190));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, CALCUL_LABEL_WIDTH, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CALCUL_WARN_X, CALCUL_WARN_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR1, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(190));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, CALCUL_LABEL1_WIDTH, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CALCUL_WARN_X, (CALCUL_WARN_Y + 30));

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR2, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(190));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, CALCUL_LABEL2_WIDTH, CALCUL_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, CALCUL_WARN_X, (CALCUL_WARN_Y + 60));


// CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	//nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	//nfui_nfobject_show(obj);
	nfui_nfobject_disable(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	
	nfui_regi_pre_event_callback(content_fixed, pre_page_event_handler);
	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean VW_StorageCalcul_tab_in_handler()
{
	guint mode_idx, auto_idx = 0;

	mode_idx = get_record_mode();
	auto_idx = get_record_autoConfig();

	_set_text_current_mode(mode_idx, auto_idx);

	if (get_record_ssc())
	    nfui_nflabel_set_text((NFLABEL*)g_ssc_obj, "ON");
	else
	    nfui_nflabel_set_text((NFLABEL*)g_ssc_obj, "OFF");
		
	nfui_combobox_set_index((NFCOMBOBOX*)Per_obj[0], 0);
	nfui_combobox_set_index((NFCOMBOBOX*)Per_obj[1], 0);

	return FALSE;
}

gboolean VW_StorageCalcul_tab_out_handler()
{
	nfui_nflabel_set_text((NFLABEL*)result_obj, "DAYS");

	return FALSE;
}
