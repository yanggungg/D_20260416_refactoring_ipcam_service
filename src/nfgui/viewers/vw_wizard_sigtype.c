#include "nf_afx.h"
#include "nf_sysman.h"

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
#include "objects/nfspinbutton.h"

#include "vw_vkeyboard.h"

#include "vw_wizard_init.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define HELP_STR "Issues such as screen flickering or flashing may occur\nif the system and connected device AC Power Frequencies do not match."

#define MAX_MARGIM_SIZE		    	(guint)12

#define PI_WND_SIZE_WID		    	(guint)(610 + 200)
#define PI_WND_SIZE_HEI		    	(guint)(520 + 200)

#define SE_PP_POS_X			    	(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y			    	(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		        (guint)(12)
#define PI_FIXED_POS_Y		        (guint)(56)
#define PI_FIXED_SIZE_WID	        (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	        (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH			    (162)
#define MENU_BTN_HEIGHT				(44)
#define MENU_BTN_GAP				(4)

#define MENU_V_BTN_R_START_X		(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X				(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X				(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y				(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)

#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(260)
#define	SUBJECT_LABEL_MARGIN		(0)


enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

enum {
	SIGNAL_50 = 0,
	SIGNAL_60,
	NUM_SIGNAL,
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_curfixed;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_btnLang[20] = {0,};
static NFOBJECT *g_objSig;

static WIZARD_USERDATA_T *g_wizard_data;
static SIGTYPE_DATA_T sigtype_data;
static SIGTYPE_DATA_T org_sigtype_data;


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
    sigtype_data.sigtype = nfui_spin_button_get_index(g_objSig);
    
	return 0;
}

static gint _save_SigType_data()
{
    DAL_set_sig_type(sigtype_data.sigtype);
    g_memmove(&g_wizard_data->sigData.sigtype, &sigtype_data.sigtype, sizeof(sigtype_data.sigtype));
    
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
		
		DAL_set_Langwizard_func(0);

        _prvLoadDataFromObjects();

		if(org_sigtype_data.sigtype != sigtype_data.sigtype)
		{
			NFOBJECT *top;
			mb_type mb;
			
			top = nfui_nfobject_get_top(obj);			
			
			mb = nftool_mbox(top, "NOTICE", "AC power frequency has been changed.\nThe system will be reboot.", NFTOOL_MB_YESNO);
			
			if ( mb == NFTOOL_MB_YES )
			{
			    _save_SigType_data();
			    DAL_save_db("sys");
    			nf_api_param_app_set_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, (gint)sigtype_data.sigtype);
			    scm_reboot_system(RR_SIGNAL_CHANGE, 0);		
		    }
		    else
		    {
		        sigtype_data.sigtype = org_sigtype_data.sigtype;
		        nfui_spin_button_set_index(g_objSig, org_sigtype_data.sigtype);
		        
		        return FALSE;
		    }
		}
        DAL_save_db("sys");
        
		_next_step_proc();
	}

	return FALSE;
}


gint vw_wizard_sigtype_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	NFOBJECT *btns[PIB_BUTTONS];
	
	const gchar *strButton[] = {"PREVIOUS", "OK", "FINISH"};
    const gchar *strCategory = "SELECT AC POWER FREQUENCY";
    const gchar *strSigType[] = {"60Hz", "50Hz"};

	gint pos_x,pos_y,size_w,size_h;
    gint i, cnt = 1;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;
    
//<------DATA COPY
    memset(&sigtype_data, 0x00, sizeof(SIGTYPE_DATA_T));
    memset(&org_sigtype_data, 0x00, sizeof(SIGTYPE_DATA_T));

    g_memmove(&sigtype_data, &g_wizard_data->sigData, sizeof(SIGTYPE_DATA_T));
    g_memmove(&org_sigtype_data, &g_wizard_data->sigData, sizeof(SIGTYPE_DATA_T));

//<------MAKE WINDOW & FIXED
	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	g_curfixed = main_fixed;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_y = 4;
	pos_x = CATEGORY_LABEL_LEFT;

//<-------- AC POWER FREQUENCY
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_get_size(obj, &size_w, &size_h);

    pos_y += CATEGORY_CONTENT_GAP;
    pos_x = SUBJECT_LABEL_LEFT;
    
	obj = nfui_nflabel_new_with_pango_font("AC POWER FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID/2, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	
	obj = nfui_spinbutton_new(strSigType, NUM_SIGNAL, sigtype_data.sigtype);
	nfui_nfobject_set_size(obj, 330, IPS_LABEL_HEIGHT);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + PI_FIXED_SIZE_WID/2, pos_y);
	g_objSig = obj;
	
//<-------- HELP
    pos_y += IPS_LABEL_HEIGHT + 50;
    
	obj = nfui_nflabel_new_with_pango_font(HELP_STR, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID - 50, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

//<-------- BUTTON

	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	nfui_nfobject_hide(btns[PIB_PREVIOUS]);
	nfui_nfobject_hide(btns[PIB_EXIT]);
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	//nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return 0;

}
