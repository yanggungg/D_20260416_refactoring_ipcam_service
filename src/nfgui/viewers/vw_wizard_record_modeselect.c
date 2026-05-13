#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"

#include "vw_vkeyboard.h"

#include "vw_wizard_init.h"
#include "vw_record_data_internal.h"
#include "vw_record_main.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define HELP_STR ""

#define MAX_MARGIM_SIZE			(guint)12

#define PI_WND_SIZE_WID			(guint)(610 + 200)
#define PI_WND_SIZE_HEI			(guint)(520 + 200)

#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		(guint)(12)
#define PI_FIXED_POS_Y		(guint)(56)
#define PI_FIXED_SIZE_WID	(guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	(guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH					(162)
#define MENU_BTN_HEIGHT					(44)
#define MENU_BTN_GAP					(4)

#define MENU_V_BTN_R_START_X			(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X					(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X					(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y					(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(400)
#define	SUBJECT_LABEL_MARGIN		(0)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)


// AUTO MODE INDEX.
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

enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_radioBtn[MODE_RECORD_CNT];
static NFOBJECT *g_lbExplanation;

static WIZARD_USERDATA_T *g_wizard_data;
static RECORD_DATA_T recData;

static mb_type g_popup_ret = 0;


static gint _exit_proc()
{
	nfui_nfobject_destroy(g_curwnd);	
	_wizard_finish();

	return 0;
}

static gint _next_step_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_next_step(1);

	return 0;
}

static gint _prev_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;
}

static gint _prvLoadDataFromObjects()
{
	return 0;
}

static gint _load_Record_data()
{
    DAL_get_wizard_record_data(&recData);
    
    return 0;
}

static gint _save_Record_data()
{
    DAL_set_wizard_record_data(&recData);
    g_memmove(&g_wizard_data->recordData, &recData, sizeof(RECORD_DATA_T));
    
    return 0;
}

static gint _set_explanation_label(gint mode)
{
    gchar *strEx[] = {
                        "Records always regardless of events.",
                        "Recording will proceed only if a motion is detected.",
                        "Recording will proceed only if an alarm event occurs.",
                        "Recording will proceed only if a motion is detected or an alarm event\noccurs",
                        "Normally recording will be performed in a low quality.\nHowever, the quality will switch to high if a motion is detected.",
                        "Normally recording will be performed in a low quality.\nHowever, the quality will switch to high if an alarm event occurs.",
                        "Normally recording will be performed in a low quality.\nHowever, the quality will switch to high if an alarm event occurs\nor a motion is detected."
                        };

    
    switch(mode)
    {
        case MODE_CONTINOUS:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_CONTINOUS]);
            break;

        case MODE_MOTION:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_MOTION]);
            break;

        case MODE_ALARM:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_ALARM]);
            break;  

        case MODE_MOT_ALARM:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_MOT_ALARM]);
            break;

        case MODE_INTENSIVE_MOTION:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_INTENSIVE_MOTION]);
            break;

        case MODE_INTENSIVE_ALARM:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_INTENSIVE_ALARM]);
            break;

        case MODE_INTENSIVE_MOT_ALARM:
            nfui_nflabel_set_text((NFLABEL*)g_lbExplanation, strEx[MODE_INTENSIVE_MOT_ALARM]);
            break;

        default:
            g_message("It is can not find explain!");
            return -1;
    }
    nfui_signal_emit((NFLABEL*)g_lbExplanation, GDK_EXPOSE, TRUE);

    return 0;
}

static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
	    gint mode;
	    
	    mode = nfui_radio_button_get_index((NFBUTTON*)obj);

	    _set_explanation_label(mode);
	}

	return FALSE;
}

static gboolean post_auto_config_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, ret_val;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

	    _set_explanation_label(index);

		recData.autoConfig = (guint)index;

		switch(index)
		{
			case AU_HIGH_QUAL:
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

	}

	return FALSE;
}

static gboolean post_ex_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkPixbuf *pbuf = NULL;
	gint gap_x, gap_y, size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);
	
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
	
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		if(evt->type == GDK_DELETE)
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
	
		return FALSE;

}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		_exit_proc();
	}

	return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		 _prev_proc();
	}

	return FALSE;
}
	
static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		_save_Record_data();
		vw_send_notify_change_record_data();
		
		_next_step_proc();
	}

	return FALSE;
}


gint vw_wizard_record_modeselect_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *ex_fixed;
	NFOBJECT *obj;	
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;
	NFOBJECT *btns[PIB_BUTTONS];
	
    const gchar *strRadio[] = { 
                            "CONTINUOUS RECORD",
    						"MOTION RECORD",
    						"AI/ALARM RECORD",
    						"MOTION/AI/ALARM RECORD",
    						"INTENSIVE MOTION RECORD",
    						"INTENSIVE AI/ALARM RECORD",
    						"INTENSIVE MOTION/AI/ALARM RECORD",
        					  };
	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};

	gint pos_x,pos_y,size_w,size_h;
    gint i, cnt;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

    memset(&recData, 0x00, sizeof(RECORD_DATA_T));

    _load_Record_data();
    //g_memmove(&recData, &g_wizard_data->recordData, sizeof(RECORD_DATA_T));

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = (guint)4;
	pos_y = (guint)4;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RECORDING CONFIGURATION MODE");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

//<-------RADIO BUTTON
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x = (guint)SUBJECT_LABEL_LEFT;
	pos_y += CATEGORY_CONTENT_GAP;

	for (i = 0; i < MODE_RECORD_CNT; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_auto_config_event_cb);
		nfui_nfobject_show(obj);

		if (i == MODE_CONTINOUS) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		} 
		else 
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		if (i == recData.autoConfig)
		{
		    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
		g_radioBtn[i] = obj;

        //RADIO LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRadio[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + size_w + 10, pos_y);

        pos_y += (guint)size_h + 10;
	}

//<-------MODE EXPLANATION

    pos_y += 20;
    
	ex_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(ex_fixed, PI_FIXED_SIZE_WID - (pos_x * 2), 150);
	nfui_nffixed_put((NFFIXED*)fixed1, ex_fixed, pos_x, pos_y);
	nfui_regi_post_event_callback(ex_fixed, post_ex_fixed_event_handler);
	nfui_nfobject_show(ex_fixed);

    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID - (pos_x * 2) - 8, 130);
	nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 5);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ex_fixed, obj, 4, 5);
    g_lbExplanation = obj;
    _set_explanation_label(recData.autoConfig);

//<-------MENU BUTTON
	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	gtk_main();

	return 0;

}

