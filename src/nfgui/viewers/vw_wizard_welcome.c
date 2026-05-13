#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "tools/nf_ui_tool.h"

#include "ix_mem.h"
#include "scm.h"

#include "vw_wizard_init.h"
#include "vw_wizard_welcome.h"


#define CONTENT_STR             "The setup wizard will help you easily configure the settings required for\nnormal system operation.\n\nYou may configure the following settings:"
#define TIP_STR                 "With the Import feature you can load the settings from an external storage device."

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


#define	IPS_LABEL_HEIGHT			(35)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(260)
#define	SUBJECT_LABEL_MARGIN		(0)


enum {
    WIZ_PASSWORD,
    WIZ_DATETIME,
    WIZ_RECORD,
    WIZ_NETWORK,
    WIZ_SEQURINET,

    WIZ_CNT
};

enum {
	PIB_START,
	PIB_CANCEL,

	PIB_BUTTONS
};


static NFOBJECT *btns[PIB_BUTTONS];
static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_lbItem[5] = {0,};

static WIZARD_USERDATA_T *g_wizard_data;

#if defined(_SEQURINET_STRING_FIX)
	gchar *g_strItem[] = {"PASSWORD", "DATE / TIME", "RECORD", "NETWORK", "SEQURINET"};
#else
	gchar *g_strItem[] = {"PASSWORD", "DATE / TIME", "RECORD", "NETWORK", "P2P"};
#endif




static gboolean post_dataloadbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	gchar title[64] = "IMPORT SETTINGS";

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		if(VW_System_Data_Load_Open(g_curwnd, title, x, (y - 598)))
		{
		    usleep(1000 * 1500);
		    vw_wizard_settings_reload(g_wizard_data);
		    nfui_signal_emit(g_curwnd, GDK_EXPOSE, TRUE);
		    g_wizard_data->run_import = 1;

		    top = nfui_nfobject_get_top(obj);
		    VW_SetupSystem_Destroy(top);

		    _wizard_cancel();
			//ssm_run_auto_logout();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER) 
		{
			if(VW_System_Data_Load_Open(g_curwnd, title, 600, 282))
			{
			    vw_wizard_settings_reload(g_wizard_data);
			    g_wizard_data->run_import = 1;
			    nfui_signal_emit(g_curwnd, GDK_EXPOSE, TRUE);
				top = nfui_nfobject_get_top(obj);
				//VW_SetupSystem_Destroy(top);
			}
		}
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

static gboolean pre_item_lb_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_EXPOSE)
    {
        gchar buf[32];
        gint i;

        memset(buf, 0x00, sizeof(buf));

        for (i = 0; i < 5; i++)
        {
            if (g_lbItem[i] == obj) break;
        }

				if (i == 5) return FALSE;
        
        sprintf(buf, "%s%s", "- ", lookup_string(g_strItem[i]));

        nfui_nflabel_set_text(((NFLABEL*)obj), buf);
    }
    return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
        g_wizard_data->topwnd = 0;		
		g_curwnd = 0;
		gtk_main_quit();
	}
	
	return FALSE;

}

static gboolean post_startbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret = -1;
 
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        DAL_set_WizardCheck_Data("sys.info.netwizard_enable", FALSE);
        DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

        nfui_nfobject_destroy(g_curwnd);

        _wizard_next_step(1);
	}

	return FALSE;
}
	
static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;		

 		DAL_set_WizardCheck_Data("sys.info.netwizard_enable", FALSE);
		DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

        nfui_nfobject_destroy(g_curwnd);

        _wizard_cancel();
	}

	return FALSE;
}

gint vw_wizard_welcome_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *lbTemp;
	NFOBJECT *lbitem[WIZ_CNT];
	NFOBJECT *lbTip;
	NFOBJECT *btnLoad;

	const gchar *strButton[] = {"NEXT", "CANCEL"};

    gchar buf[32];
	guint pos_x, pos_y;
	guint i, ret1, ret2;
	int num;


    g_wizard_data = (WIZARD_USERDATA_T*)user_data;


	main_wnd = nftool_create_popup_window((NFWINDOW*)parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

    g_wizard_data->topwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = 28;
	pos_y = 0;

//-----> CONTENT
	lbTemp = nfui_nflabel_new_with_pango_font(CONTENT_STR, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nflabel_set_spacing((NFLABEL *)lbTemp, NORMAL_SPACING);		
	nfui_nfobject_set_size(lbTemp, PI_FIXED_SIZE_WID, 120);
	nfui_nffixed_put((NFFIXED*)fixed1, lbTemp, pos_x, pos_y);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

//-----> ITEMS
    pos_y += 120 + 20;
    
    for (i = 0; i < WIZ_CNT; i++)
    {
        if (i == WIZ_PASSWORD)
        {
            if (!ivsc.dfunc.wizard.password.support) continue;
        }
        if (i == WIZ_SEQURINET)
        {
            if (!var_get_supported_sequrinet()) continue;
        }
        if (i == WIZ_RECORD)
        {
            if (g_wizard_data->recordData.mode != AUTO_CONFIG) continue;
        }

        sprintf(buf, "%s%s", "- ", lookup_string(g_strItem[i]));
        lbitem[i] = nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        nfui_nflabel_set_align((NFLABEL*)lbitem[i], NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(lbitem[i], PI_FIXED_SIZE_WID - 28, IPS_LABEL_HEIGHT);
        nfui_nfobject_use_focus(lbitem[i], NFOBJECT_FOCUS_OFF);
        nfui_regi_pre_event_callback(lbitem[i], pre_item_lb_event_cb);
        nfui_nfobject_show(lbitem[i]);
        nfui_nffixed_put((NFFIXED*)fixed1, lbitem[i], pos_x, pos_y);
        g_lbItem[i] = lbitem[i];
        pos_y += IPS_LABEL_HEIGHT;
    }
    
//-----> TIP
    pos_y = MENU_V_BTN_Y - 50;
    
    lbTip = nfui_nflabel_new_with_pango_font(TIP_STR, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)lbTip, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(lbTip, PI_FIXED_SIZE_WID - 28, IPS_LABEL_HEIGHT);
    nfui_nfobject_use_focus(lbTip, NFOBJECT_FOCUS_OFF);
    //nfui_nfobject_show(lbTip);
    nfui_nffixed_put((NFFIXED*)fixed1, lbTip, MENU_BTN_GAP, pos_y);

//-----> BUTTON
    btnLoad = nftool_normal_button_create_type1("IMPORT SETTINGS", 270);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(btnLoad), NFALIGN_CENTER, 0);
    nfui_nfobject_show(btnLoad);
    
	for( i = 0; i < PIB_BUTTONS; i++ )
	{
		if(i == 0) num = 160;
		else num = 160;
		btns[i] = nftool_normal_button_create_type1(strButton[i], num);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btnLoad, MENU_BTN_GAP, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_START], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_CANCEL], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btnLoad, post_dataloadbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_START], post_startbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_CANCEL], post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_START], TRUE);

	gtk_main();

    return 0;
}

