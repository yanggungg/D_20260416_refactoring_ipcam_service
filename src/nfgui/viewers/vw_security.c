#include "nf_afx.h"

#include "scm.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"

#include "vw_sys_sound_main.h"
#include "vw_sys_sound_audio.h"
#include "vw_diagnostic_data_save.h"

#define	SECURITY_LABLE_LEFT     (8)
#define	SECURITY_LABLE_TOP      (0)

#define	SECURITY_LABLE_WIDTH    (473)
#define	SECURITY_LABLE_HEIGHT   (40)

#define	SECURITY_CELL_WIDTH     (300)
#define	SECURITY_CELL_HEIGHT    (SECURITY_LABLE_HEIGHT)

#define	SECURITY_COL_SPACE					(guint)(DISPLAY_IS_D1 ? 4:1)
#define	SECURITY_LABLE_HEIGHT_WITH_SPACE	(guint)(DISPLAY_IS_D1 ? 18:42)


enum {
	ROW_AUDIO_SUPPORT = 0,
	ROW_SNAPSHOT_SUPPORRT,
	ROW_DIAGNOSTIC_DATA,
	ROW_PW_RULE,
	ROW_SEARCH_ARCH_LOGIN,
	ROW_INSERT_PW_ARCH,
	ROW_DOUBLE_LOGIN,
	ROW_ID_INPUT_MODE_PASS,
	ROW_ID_INPUT_MODE_PIN,
	ROW_ID_RULE,
	ROW_QUESITON_PW_RESET,

	ROW_MAX
};

enum {
	SAB_CANCEL = 0,
	SAB_APPLY,
	SAB_CLOSE,
	SAB_BUTTONS
};

static SecurityData secdata;
static SecurityData org_secdata;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[ROW_MAX] = {0,};


static gboolean post_diagnostic_data_save_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_Diagnostic_Data_Save_Open(g_curwnd);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		g_memmove(&secdata, &org_secdata, sizeof(SecurityData));

		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_AUDIO_SUPPORT], secdata.audio);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_SNAPSHOT_SUPPORRT], secdata.snapshot);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_DIAGNOSTIC_DATA], secdata.diagnostic_data);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_PW_RULE], secdata.pwrule);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_SEARCH_ARCH_LOGIN], secdata.loginSearchArch);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_INSERT_PW_ARCH], secdata.pwRawbackup);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_DOUBLE_LOGIN], secdata.double_login);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PASS], secdata.id_input_mode);
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PIN], secdata.pin_id_input_mode);
		if(value[ROW_QUESITON_PW_RESET] != 0) nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_QUESITON_PW_RESET], secdata.question_pw_reset);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)value[ROW_ID_RULE], secdata.idrule);

		for(i=0; i<ROW_MAX; i++)
		{
			if(value[i] == 0) continue;
			nfui_signal_emit((NFSPINBUTTON*)value[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}
	
		secdata.audio = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_AUDIO_SUPPORT]);
		secdata.snapshot = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_SNAPSHOT_SUPPORRT]);
		secdata.diagnostic_data = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_DIAGNOSTIC_DATA]);
		secdata.pwrule = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_PW_RULE]);
		secdata.loginSearchArch = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_SEARCH_ARCH_LOGIN]);
		secdata.pwRawbackup = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_INSERT_PW_ARCH]);
		secdata.double_login = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_DOUBLE_LOGIN]);
		secdata.id_input_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PASS]);
		secdata.pin_id_input_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PIN]);
		secdata.idrule = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_ID_RULE]);
		if(value[ROW_QUESITON_PW_RESET] != 0) secdata.question_pw_reset = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_QUESITON_PW_RESET]);

		if(memcmp(&org_secdata, &secdata, sizeof(SecurityData)))
		{
			//scm_put_log(CHANGE_SECURITY, 0, 0);
			if(secdata.audio != org_secdata.audio)
			{
				if(secdata.audio) {
				    DAL_set_audio_on();
					nftool_mbox(g_curwnd, "NOTICE", "Audio configuration has been reset.\nPlease check it", NFTOOL_MB_OK);
                  }
				else {
					DAL_set_audio_off();
                  }
			}

			if(secdata.snapshot != org_secdata.snapshot)
			{
				if(secdata.snapshot)
					nftool_mbox(g_curwnd, "NOTICE", "Snapshot configuration has been reset.\nPlease check it", NFTOOL_MB_OK);
				else
					DAL_set_snapshot_off();
			}

			g_memmove(&org_secdata, &secdata, sizeof(SecurityData));

			DAL_set_security_data(&secdata);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			VW_SetupSystem_set_changeflag(1);
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

		   VW_Security_tab_out_handler();

		   VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
		}
		break;

		default :
			break;
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


void VW_Init_SysSecurity_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *security_btns[SAB_BUTTONS];	

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strTitle[] = {
				"AUDIO SUPPORT",
				"SNAPSHOT SUPPORT",
				"DIAGNOSTIC DATA",
				"ENHANCED PASSWORD RULE",
				"PASSWORD CHECK WHEN SEARCH/BACKUP",
				"RAW BACKUP WITH ENCRYPTION KEY",
				"DOUBLE LOGIN",
				"ID INPUT METHOD (PASSWORD LOGIN)",
				"ID INPUT METHOD (PIN LOGIN)",
				"ENHANCED USER ID RULE",
				"QUESTION PASSWORD RESET VIA WEB"
	};
	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strIim[] = {"ID SELECTION METHOD", "DIRECT INPUT METHOD"};

	guint btn_x, btn_y, btn_space;
	guint pos_y;
	guint i;



	g_curwnd = nfui_nfobject_get_top(parent);


// FIXED
	memset(&secdata, 0x00, sizeof(SecurityData));
	memset(&org_secdata, 0x00, sizeof(SecurityData));

	DAL_get_security_data(&secdata);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	pos_y = SECURITY_LABLE_TOP;

//DATA SECURITY	
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "DATA SECURITY");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE + 21;

    //DOUBLE LOGIN	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_DOUBLE_LOGIN], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    if (var_get_supported_double_login()) nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_DOUBLE_LOGIN] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.double_login);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_DOUBLE_LOGIN], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_DOUBLE_LOGIN], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	if (var_get_supported_double_login()) nfui_nfobject_show(value[ROW_DOUBLE_LOGIN]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_DOUBLE_LOGIN], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);

	if (var_get_supported_double_login()) pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

    //SEARCH / ARCH LOGIN	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_SEARCH_ARCH_LOGIN], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	if (var_get_supported_checkpw_search_arch()) nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_SEARCH_ARCH_LOGIN] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.loginSearchArch);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_SEARCH_ARCH_LOGIN], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_SEARCH_ARCH_LOGIN], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	if (var_get_supported_checkpw_search_arch()) nfui_nfobject_show(value[ROW_SEARCH_ARCH_LOGIN]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_SEARCH_ARCH_LOGIN], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	if (var_get_supported_checkpw_search_arch()) pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

    //AUDIO	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_AUDIO_SUPPORT], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);

	value[ROW_AUDIO_SUPPORT] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.audio);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_AUDIO_SUPPORT], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_AUDIO_SUPPORT], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	nfui_nfobject_show(value[ROW_AUDIO_SUPPORT]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_AUDIO_SUPPORT], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

    //SNAPSHOT	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_SNAPSHOT_SUPPORRT], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
		
	value[ROW_SNAPSHOT_SUPPORRT] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.snapshot);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_SNAPSHOT_SUPPORRT], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_SNAPSHOT_SUPPORRT], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	nfui_nfobject_show(value[ROW_SNAPSHOT_SUPPORRT]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_SNAPSHOT_SUPPORRT], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;
	
	// DIAGNOSTIC DATA
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_DIAGNOSTIC_DATA], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);

	value[ROW_DIAGNOSTIC_DATA] = (NFOBJECT*)nfui_combobox_new(strOffOn, 2, secdata.diagnostic_data);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)value[ROW_DIAGNOSTIC_DATA], NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align((NFCOMBOBOX*)value[ROW_DIAGNOSTIC_DATA], NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(value[ROW_DIAGNOSTIC_DATA], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	// nfui_nfobject_show(value[ROW_DIAGNOSTIC_DATA]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_DIAGNOSTIC_DATA], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("SAVE", 300);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_diagnostic_data_save_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE/*+SECURITY_CELL_WIDTH+2*/, pos_y);

    //RAW ENCRYPTION	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_INSERT_PW_ARCH], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_INSERT_PW_ARCH] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.pwRawbackup);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_INSERT_PW_ARCH], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_INSERT_PW_ARCH], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	//nfui_nfobject_show(value[ROW_INSERT_PW_ARCH]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_INSERT_PW_ARCH], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);

//ACCOUNT SECURITY	
    pos_y += 60;
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "ACCOUNT SECURITY");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);

	pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE + 21;

    //INPUT ID TYPE	 (PASSWORD)
	if (ivsc.dfunc.pin.id_input_method) obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_ID_INPUT_MODE_PASS], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	else obj = nfui_nflabel_new_with_pango_font("ID INPUT METHOD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);

	value[ROW_ID_INPUT_MODE_PASS] = (NFOBJECT*)nfui_combobox_new(strIim, 2, secdata.id_input_mode);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PASS], NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PASS], NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(value[ROW_ID_INPUT_MODE_PASS], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	nfui_nfobject_show(value[ROW_ID_INPUT_MODE_PASS]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_ID_INPUT_MODE_PASS], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);

		pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

    //INPUT ID TYPE	(PIN)
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_ID_INPUT_MODE_PIN], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	if (ivsc.dfunc.pin.id_input_method) nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_ID_INPUT_MODE_PIN] = (NFOBJECT*)nfui_combobox_new(strIim, 2, secdata.pin_id_input_mode);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PIN], NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PIN], NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(value[ROW_ID_INPUT_MODE_PIN], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	if (ivsc.dfunc.pin.id_input_method) nfui_nfobject_show(value[ROW_ID_INPUT_MODE_PIN]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_ID_INPUT_MODE_PIN], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	if (ivsc.dfunc.pin.id_input_method) pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

    //ENHANCED ID	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_ID_RULE], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_ID_RULE] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.idrule);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_ID_RULE], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_ID_RULE], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	nfui_nfobject_show(value[ROW_ID_RULE]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_ID_RULE], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	if(var_get_vendor_code() != 28) pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;
	
    //ENHANCED PW	
	obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_PW_RULE], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	if(var_get_vendor_code() != 28) nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);
	
	value[ROW_PW_RULE] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.pwrule);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_PW_RULE], NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(value[ROW_PW_RULE], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
	if(var_get_vendor_code() != 28) nfui_nfobject_show(value[ROW_PW_RULE]);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_PW_RULE], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	
	pos_y += SECURITY_LABLE_HEIGHT_WITH_SPACE;

	if(ivsc.dfunc.support_qna){
		//QUESTION PASSWORD RESET VIA WEB
		obj = nfui_nflabel_new_with_pango_font(strTitle[ROW_QUESITON_PW_RESET], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_set_size(obj, SECURITY_LABLE_WIDTH, SECURITY_LABLE_HEIGHT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, SECURITY_LABLE_LEFT, pos_y);

		value[ROW_QUESITON_PW_RESET] = (NFOBJECT*)nfui_spinbutton_new(strOffOn, 2, secdata.question_pw_reset);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[ROW_QUESITON_PW_RESET], NFSPINBUTTON_TYPE_1);
		nfui_nfobject_set_size(value[ROW_QUESITON_PW_RESET], SECURITY_CELL_WIDTH, SECURITY_CELL_HEIGHT);
		nfui_nfobject_show(value[ROW_QUESITON_PW_RESET]);
		nfui_nffixed_put((NFFIXED*)content_fixed, value[ROW_QUESITON_PW_RESET], SECURITY_LABLE_LEFT+SECURITY_LABLE_WIDTH+SECURITY_COL_SPACE, pos_y);
	}
	
	if (var_get_supported_audio() == 0)
	{
		nfui_nfobject_disable(value[ROW_AUDIO_SUPPORT]);
	}
	
    if (var_get_vendor_code() == 30)
    {
		nfui_nfobject_disable(value[ROW_PW_RULE]);
    }
	
	security_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(security_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(security_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, security_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	security_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(security_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(security_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, security_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	security_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(security_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(security_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, security_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(security_btns[SAB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(security_btns[SAB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(security_btns[SAB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_secdata, &secdata, sizeof(SecurityData));
}

gboolean VW_Security_tab_out_handler()
{
	gint i;
	mb_type ret;

	secdata.audio = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_AUDIO_SUPPORT]);
	secdata.snapshot = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_SNAPSHOT_SUPPORRT]);
	secdata.diagnostic_data = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_DIAGNOSTIC_DATA]);
	secdata.pwrule = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_PW_RULE]);
	secdata.loginSearchArch = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_SEARCH_ARCH_LOGIN]);
	secdata.pwRawbackup = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_INSERT_PW_ARCH]);
	secdata.double_login = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_DOUBLE_LOGIN]);
	secdata.id_input_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PASS]);
	secdata.pin_id_input_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ROW_ID_INPUT_MODE_PIN]);
	secdata.idrule = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_ID_RULE]);
	if(value[ROW_QUESITON_PW_RESET] != 0) secdata.question_pw_reset = nfui_spin_button_get_index((NFSPINBUTTON*)value[ROW_QUESITON_PW_RESET]);

	if(!memcmp(&org_secdata, &secdata, sizeof(SecurityData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		//scm_put_log(CHANGE_SECURITY, 0, 0);

		if(secdata.audio != org_secdata.audio)
		{
			if(secdata.audio) {
			     DAL_set_audio_on();
				nftool_mbox(g_curwnd, "NOTICE", "Audio configuration has been reset.\nPlease check it", NFTOOL_MB_OK);
              }
			else {
				DAL_set_audio_off();
             }
		}
			
		if(secdata.snapshot != org_secdata.snapshot)
		{
			if(secdata.snapshot)
				nftool_mbox(g_curwnd, "NOTICE", "Snapshot configuration has been reset.\nPlease check it", NFTOOL_MB_OK);
			else
				DAL_set_snapshot_off();
		}
			
		g_memmove(&org_secdata, &secdata, sizeof(SecurityData));
		DAL_set_security_data(&secdata);
		
		VW_SetupSystem_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&secdata, &org_secdata, sizeof(SecurityData));

		nfui_spin_button_set_index_no_expose(value[ROW_AUDIO_SUPPORT], secdata.audio);
		nfui_spin_button_set_index_no_expose(value[ROW_SNAPSHOT_SUPPORRT], secdata.snapshot);
		nfui_spin_button_set_index_no_expose(value[ROW_PW_RULE], secdata.pwrule);
		nfui_spin_button_set_index_no_expose(value[ROW_SEARCH_ARCH_LOGIN], secdata.loginSearchArch);
		nfui_spin_button_set_index_no_expose(value[ROW_INSERT_PW_ARCH], secdata.pwRawbackup);
		nfui_spin_button_set_index_no_expose(value[ROW_DOUBLE_LOGIN], secdata.double_login);
		nfui_combobox_set_index_no_expose(value[ROW_ID_INPUT_MODE_PASS], secdata.id_input_mode);
		nfui_combobox_set_index_no_expose(value[ROW_ID_INPUT_MODE_PIN], secdata.pin_id_input_mode);
		if(value[ROW_QUESITON_PW_RESET] != 0) nfui_spin_button_set_index_no_expose(value[ROW_QUESITON_PW_RESET], secdata.question_pw_reset);
		nfui_spin_button_set_index_no_expose(value[ROW_ID_RULE], secdata.idrule);
	}

	return FALSE;

}


