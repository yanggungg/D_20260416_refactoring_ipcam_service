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
#include "objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "vw_record_data_internal.h"
#include "vw_wizard_init.h"

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
#define	SUBJECT_LABEL_WIDTH			((PI_FIXED_SIZE_WID - (SUBJECT_LABEL_LEFT * 2)) / 2)
#define	SUBJECT_LABEL_MARGIN		(0)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)
#define SUBJECT_LABEL_WIDTH1        (PI_FIXED_SIZE_WID / 3)


enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

enum {
    ITEM_RESOLUTION = 0,
    ITEM_FPS,
    ITEM_QUALITY,
    ITEM_AUDIO,
    ITEM_REC_PRE_TIME,
    ITEM_REC_POST_TIME,

    ITEM_MAX    
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_item[ITEM_MAX];

static WIZARD_USERDATA_T *g_wizard_data;

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

static gint _save_SigType_data()
{
    return 0;
}

static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
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
			
		_next_step_proc();
	}

	return FALSE;
}

static gint _init_page_continous(NFOBJECT* parent, gint pos_x, gint pos_y)
{
    NFOBJECT *obj;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;
	NFOBJECT *btns[PIB_BUTTONS];
	
    const gchar *strRadio[] = {REC_PRIORITY_DAYS, REC_PRIORITY_QUALITY};
	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};

	gint size_w,size_h;
    gint i, cnt;

//<-------RADIO BUTTON
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x = (guint)SUBJECT_LABEL_LEFT;
	pos_y += CATEGORY_CONTENT_GAP;

	for (i = 0; i < 2; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);

		if (i == 0) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} 
		else 
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}
		nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

//<-------RADIO LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRadio[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)parent, obj, pos_x + size_w + 10, pos_y);

        pos_y += (guint)size_h + 10;
	}

//<-------MODE EXPLANATION
    pos_y += 20;
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID, 100);
	nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

    return 0;
}

static gint _init_page_mot_alarm(NFOBJECT* parent, gint pos_x, gint pos_y)
{
    NFOBJECT *obj;
    gint i;

    gchar *strItem[] = {"RESOLUTION", "FPS", "QUALITY", "AUDIO", "PRE RECORDING TIME", "POST RECORDING TIME"};


    for ( i = 0; i < ITEM_MAX; i++ )
    {
        obj = nfui_nflabel_new_with_pango_font(strItem[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nflabel_set_align(obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
        nfui_nfobject_show(obj);

        obj = nfui_combobox_new(NULL, 0, 0);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
        nfui_nfobject_show(obj);

        pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    }
    return 0;
}

static gint _init_page_intensive(NFOBJECT* parent, gint pos_x, gint pos_y)
{
    NFOBJECT *obj;
    gint i;

    gchar *strItem[] = {"RESOLUTION", "FPS", "QUALITY", "AUDIO", "PRE RECORDING TIME", "POST RECORDING TIME"};
    gchar *strEvent[] = {"GENERAL", "EVENT"};


    pos_x = CATEGORY_LABEL_LEFT + SUBJECT_LABEL_WIDTH1 + IPS_LABEL_ROW_SPACE + 10;
    for ( i = 0; i < 2; i++ )
    {
        obj = nfui_nflabel_new_with_pango_font(strEvent[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1 - 5, IPS_LABEL_HEIGHT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
        nfui_nfobject_show(obj);

        pos_x += SUBJECT_LABEL_WIDTH1 + IPS_LABEL_ROW_SPACE - 5;
    }

    pos_x = CATEGORY_LABEL_LEFT;
    pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    
    for ( i = 0; i < ITEM_MAX; i++ )
    {
        if ( i <= ITEM_QUALITY )
        {
            obj = nfui_nflabel_new_with_pango_font(strItem[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1 + 10, IPS_LABEL_HEIGHT);
            nfui_nflabel_set_align(obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, FALSE);
            nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);

            pos_x += SUBJECT_LABEL_WIDTH1 + IPS_LABEL_ROW_SPACE + 10;
            
            obj = nfui_combobox_new(NULL, 0, 0);
            nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1 - 5, IPS_LABEL_HEIGHT);
            nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
            nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);

            obj = nfui_combobox_new(NULL, 0, 0);
            nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1 - 5, IPS_LABEL_HEIGHT);
            nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
            nfui_nffixed_put((NFFIXED*)parent, obj, pos_x + SUBJECT_LABEL_WIDTH1 + IPS_LABEL_ROW_SPACE - 5, pos_y);
            nfui_nfobject_show(obj);
        }
        else
        {   
            if(i == ITEM_AUDIO)
            {
                obj = nfui_nflabel_new_with_pango_font(strItem[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
            }
            else
            {
                obj = nfui_nflabel_new_with_pango_font(strItem[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(295));
            }
            nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1 + 10, IPS_LABEL_HEIGHT);
            //nfui_nfobject_set_size(obj, (SUBJECT_LABEL_WIDTH1 * 2) + IPS_LABEL_ROW_SPACE, IPS_LABEL_HEIGHT);
            nfui_nflabel_set_align(obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, FALSE);
            nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);

            //pos_x += (SUBJECT_LABEL_WIDTH1 * 2) + IPS_LABEL_ROW_SPACE;
            pos_x += SUBJECT_LABEL_WIDTH1 + IPS_LABEL_ROW_SPACE + 10;
            
            obj = nfui_combobox_new(NULL, 0, 0);
            nfui_nfobject_set_size(obj, (SUBJECT_LABEL_WIDTH1 * 2) + IPS_LABEL_ROW_SPACE - 10, IPS_LABEL_HEIGHT);
            //nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
            nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
            //nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
            nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);
        }

        pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
        pos_x = CATEGORY_LABEL_LEFT;
    }

    return 0;

}

gint vw_wizard_record_imageconf_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	
	NFOBJECT *btns[PIB_BUTTONS];
	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};

	gint pos_x,pos_y,size_w,size_h;
    gint i, cnt;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

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
	nfui_nfimage_set_text((NFIMAGE*)obj, "SETTING THE IMAGE");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	pos_x = SUBJECT_LABEL_LEFT;
	pos_y += CATEGORY_CONTENT_GAP;

	//if (g_wizard_data->recordData.mode == MODE_CONTINOUS)
	{
	    //_init_page_continous(fixed1, pos_x, pos_y);
	}
	//else if (g_wizard_data->recordData.mode == MODE_MOTION || g_wizard_data->recordData.mode == MODE_ALARM || g_wizard_data->recordData.mode == MODE_MOT_ALARM)
	{
	    //_init_page_mot_alarm(fixed1, pos_x, pos_y);
	}
	//else
	{
	    _init_page_intensive(fixed1, pos_x, pos_y);
	}

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

