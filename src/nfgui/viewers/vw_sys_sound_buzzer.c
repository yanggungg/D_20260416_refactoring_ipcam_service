#include "nf_afx.h"

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

//#include "nf_ui_snd_main.h"
//#include "nf_ui_buzzer.h"

#include "vw_sys_sound_main.h"
#include "vw_sys_sound_buzzer.h"
#include "scm.h"

#include "support/event_loop.h"
#include "nf_util_device.h"


#define	BUZZER_LABLE_LEFT		(28)
#define	BUZZER_LABLE_TOP		(42)

#define	BUZZER_LABLE_WIDTH		(guint)(DISPLAY_IS_D1 ? 308:433)
#define	BUZZER_LABLE_HEIGHT		(guint)(DISPLAY_IS_D1 ? 16:40)

#define	BUZZER_CELL_WIDTH		(guint)(DISPLAY_IS_D1 ? 134:261)
#define	BUZZER_CELL_HEIGHT		BUZZER_LABLE_HEIGHT

#define	BUZZER_COL_SPACE		(guint)(DISPLAY_IS_D1 ? 4:2)

enum {
	SBB_CANCEL = 0,
	SBB_APPLY,
	SBB_CLOSE,
	SBB_BUTTONS
};

static BuzzerData buzzdata;
static BuzzerData org_buzzdata;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[2];

static gboolean 
pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
/*
			x -= 14;;
			y -= 6;;
			nfutil_draw_image(drawable, gc, IMG_MAINBG2, x, y, -1, -1, NFALIGN_LEFT, 0);
*/			
		}
		break;

		default :
			break;
	}

	return FALSE;
}

static gboolean 
post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}
	
		g_memmove(&buzzdata, &org_buzzdata, sizeof(BuzzerData));

		nfui_spin_button_set_index(value[0], buzzdata.keypad);
		nfui_signal_emit(value[0], GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index(value[1], buzzdata.remote);
		nfui_signal_emit(value[1], GDK_EXPOSE, TRUE);
	}

	return FALSE;
	
}

static gboolean 
post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
		}
	
		buzzdata.keypad = nfui_spin_button_get_index(value[0]);
		buzzdata.remote = nfui_spin_button_get_index(value[1]);

		if(memcmp(&org_buzzdata, &buzzdata, sizeof(BuzzerData)))
		{
			#if defined(USE_DEV_REMOCON)//ksi_test
			// beep on/off
			if(org_buzzdata.remote != buzzdata.remote) {
				// FIXME: wrapping api
				if(buzzdata.remote) nf_dev_remocon_beep_on();	
				else 				nf_dev_remocon_beep_off();	
			}
			#endif
			g_memmove(&org_buzzdata, &buzzdata, sizeof(BuzzerData));
			DAL_set_buzzer_data(&buzzdata);

			scm_put_log(CHANGE_AUD_BUZZ, 0, 0);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			syssnd_set_changeflag(1);

		}
	}

	return FALSE;
}

static gboolean 
post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	       if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }
	
		Buzzer_tab_out_handler();

		SystemSetupSound_Destroy(obj);
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


void init_SndBuzzer_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *subject[2];
	NFOBJECT *buzz_btns[SBB_BUTTONS];

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
	const gchar *strOffOn[] = {"OFF", "ON"};
	//const gchar *strTitle[] = {"KEYPAD", "REMOTE CONTROLLER"};
	const gchar *strTitle[] = {" ", "REMOTE CONTROLLER BEEP"};
	guint btn_x, btn_y, btn_space;
	guint i;
	

	g_curwnd = nfui_nfobject_get_top(parent);

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	memset(&buzzdata, 0x00, sizeof(BuzzerData));
	memset(&org_buzzdata, 0x00, sizeof(BuzzerData));
	
	DAL_get_buzzer_data(&buzzdata);


	value[0] = nfui_spinbutton_new(strOffOn, 2, buzzdata.keypad);
	value[1] = nfui_spinbutton_new(strOffOn, 2, buzzdata.remote);

	for(i = 0 ; i < 2 ; i++)
	{
		subject[i] = nfui_nflabel_new_with_pango_font(strTitle[i], 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 
									COLOR_IDX(120));
		
		nfui_nflabel_set_align((NFLABEL*)subject[i], NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(subject[i], BUZZER_LABLE_WIDTH, BUZZER_LABLE_HEIGHT);
		nfui_nfobject_modify_bg(subject[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(subject[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(subject[i]);
			
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[i], NFSPINBUTTON_TYPE_1);
		nfui_nfobject_set_size(value[i], BUZZER_CELL_WIDTH, BUZZER_CELL_HEIGHT);
		if(i == 1) nfui_nfobject_show(value[i]);
	}

	nfui_nffixed_put((NFFIXED*)content_fixed, subject[0], BUZZER_LABLE_LEFT, BUZZER_LABLE_TOP+42);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[0], 
		BUZZER_LABLE_LEFT+BUZZER_LABLE_WIDTH+BUZZER_COL_SPACE, BUZZER_LABLE_TOP+42);

	nfui_nffixed_put((NFFIXED*)content_fixed, subject[1], BUZZER_LABLE_LEFT, BUZZER_LABLE_TOP);
	nfui_nffixed_put((NFFIXED*)content_fixed, value[1], 
		BUZZER_LABLE_LEFT+BUZZER_LABLE_WIDTH+BUZZER_COL_SPACE, BUZZER_LABLE_TOP);

	buzz_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(buzz_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(buzz_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, buzz_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	buzz_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(buzz_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(buzz_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, buzz_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	buzz_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(buzz_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(buzz_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, buzz_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	
	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(buzz_btns[SBB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(buzz_btns[SBB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(buzz_btns[SBB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_buzzdata, &buzzdata, sizeof(BuzzerData));
	
}

gboolean Buzzer_tab_out_handler()
{
	mb_type ret;

	buzzdata.keypad = nfui_spin_button_get_index(value[0]);
	buzzdata.remote = nfui_spin_button_get_index(value[1]);

	if(!memcmp(&org_buzzdata, &buzzdata, sizeof(BuzzerData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
										 NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		#if defined(USE_DEV_REMOCON)//ksi_test
		// beep on/off
		if(org_buzzdata.remote != buzzdata.remote) {
			// FIXME: wrapping api
			if(buzzdata.remote) nf_dev_remocon_beep_on();	
			else 				nf_dev_remocon_beep_off();	
		}
		#endif
		g_memmove(&org_buzzdata, &buzzdata, sizeof(BuzzerData));
		DAL_set_buzzer_data(&buzzdata);

		scm_put_log(CHANGE_AUD_BUZZ, 0, 0);
	
		syssnd_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&buzzdata, &org_buzzdata, sizeof(BuzzerData));

		nfui_spin_button_set_index_no_expose(value[0], buzzdata.keypad);
		nfui_spin_button_set_index_no_expose(value[1], buzzdata.remote);
	}

	return FALSE;

}


