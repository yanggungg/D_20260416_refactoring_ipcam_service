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

#include "ix_mem.h"
#include "scm.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_instruction.h"

#define CONTENT_STR             "Running the network setup wizard will help configure your network.\nCheck that the network cable is connected.\n\nNote: the device network connection and any connected cameras\nwill disconnect."

#define MAX_MARGIM_SIZE             (guint)12

#define PI_WND_SIZE_WID             (guint)(WIZARD_SIZE_WID)
#define PI_WND_SIZE_HEI             (guint)(WIZARD_SIZE_HEI)

#define SE_PP_POS_X                 (guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y                 (guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X              (guint)(12)
#define PI_FIXED_POS_Y              (guint)(56)
#define PI_FIXED_SIZE_WID           (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI           (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH              (162)
#define MENU_BTN_HEIGHT             (44)
#define MENU_BTN_GAP                (4)

#define MENU_V_BTN_R_START_X        (PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X             (MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X             (MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y                (PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define IPS_LABEL_HEIGHT            (35)
#define IPS_LABEL_ROW_SPACE         (2)
#define SUBJECT_LABEL_LEFT          (28)
#define SUBJECT_LABEL_TOP           (42)
#define SUBJECT_LABEL_WIDTH         (260)
#define SUBJECT_LABEL_MARGIN        (0)


enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

static NFWINDOW *g_curwnd = 0;

static WIZARD_USERDATA_T *g_wizard_data;


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

static gboolean _is_connected_cable()
{	
	NF_NOTIFY_INFO info;
	
	if(!scm_get_net_rxtx_status_data(&info)) 
	{
		if (!info.d.params[3]) return FALSE;
	}
	
	return TRUE;
}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
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
		mb_type ret = -1;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK)	return FALSE;
		}
		
		_next_step_proc();
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

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret = -1;
 
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK)	return FALSE;
		}

        DAL_set_WizardCheck_Data("sys.info.netwizard_enable", FALSE);
        DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

		_next_step_proc();
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

gint vw_wizard_network_instruction_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *lbTemp;
	NFOBJECT *btns[PIB_BUTTONS];

	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};

	guint pos_x, pos_y;
	guint i, ret1, ret2;


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

	pos_x = 0;
	pos_y = 0;

	lbTemp = nfui_nflabel_new_with_pango_font(CONTENT_STR, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nflabel_set_spacing((NFLABEL *)lbTemp, NORMAL_SPACING);		
	nfui_nfobject_set_size(lbTemp, PI_FIXED_SIZE_WID, 200);
	nfui_nffixed_put((NFFIXED*)fixed1, lbTemp, pos_x, pos_y);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);
    nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
    
	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	scm_req_net_rxtx_status_data();
		
	gtk_main();

	return 0;
}

