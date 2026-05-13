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
#include "objects/nfspinbutton.h"

#include "vw_vkeyboard.h"

#include "vw_wizard_init.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define HELP_STR "Note : The power of the frequency and value in use\ndoes not match If you can experience symptoms\nsuch as flashing or flickering screen sick."

#define MAX_MARGIM_SIZE		    	(guint)12

#define PI_WND_SIZE_WID		    	(guint)(610)
#define PI_WND_SIZE_HEI		    	(guint)(520 + 120)

#define SE_PP_POS_X			    	(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y			    	(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		        (guint)(12)
#define PI_FIXED_POS_Y		        (guint)(56)
#define PI_FIXED_SIZE_WID	        (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	        (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH			    (230)
#define MENU_BTN_HEIGHT				(44)
#define MENU_BTN_GAP				(4)

#define MENU_V_BTN_R_START_X		(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X				(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X				(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y				(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)

#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(260)
#define	SUBJECT_LABEL_MARGIN		(0)

#define LANGUAGE_STR_SIZE		    (32)

enum {
	PIB_LANGAPPLY,
	PIB_OK,
	PIB_BUTTONS
};

enum {
	SIGNAL_50 = 0,
	SIGNAL_60,
	NUM_SIGNAL,
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_btnLang[20] = {0,};
static NFOBJECT *g_btnApply;
static NFOBJECT *g_objSig;

static WIZARD_USERDATA_T *g_wizard_data;
static ESSENTIAL_DATA_T essential_data;
static ESSENTIAL_DATA_T org_essential_data;

gchar **g_strLang;
gint g_langCnt;

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

static gboolean _init_lang()
{
	gint i, j = 0;
	gchar strLangAlias[64];
	gint cnt;


	cnt = DAL_get_support_lang_cnt();
	if (cnt < 0) 
	{
    	return FALSE;
	}

	for (i = 0; i < cnt; i++)
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		if (strlen(strLangAlias)) g_langCnt++;
	}

	g_strLang = (gchar**)imalloc(sizeof(gchar*)*g_langCnt);
	
	for (i = 0; i < cnt; i++) 
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		
		if (strlen(strLangAlias)) {
			g_strLang[j] = (gchar*)imalloc(LANGUAGE_STR_SIZE);
			g_assert(g_strLang[j]);

			DAL_get_support_lang(i, g_strLang[j]);
			j++;
		}
	}

	g_message("%s, %d, g_langCnt:%d, j:%d", __FUNCTION__, __LINE__, g_langCnt, j);

	return TRUE;
}


static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
	    gint i;
        gchar tmp[256];
    	gchar *p;


	    for ( i = 0; i < g_langCnt; i++ )
	    {
	        if ( g_btnLang[i] == obj )
	        {
	            break;
	        }
	    }

	    if ( i == g_langCnt )
	    {
	        g_message("%s %d, No matching language.", __FUNCTION__, __LINE__);
	    }
        strcpy(essential_data.lang, g_strLang[i]);
        
		strcpy(tmp, "LANGUAGE APPLY");
		p = lookup_string(tmp);
		if (p != NULL)
			g_utf8_strncpy(((NFBUTTON*)g_btnLang)->strLabel, p, g_utf8_strlen(p, 256));
		else
			strcpy(((NFBUTTON*)g_btnLang)->strLabel, tmp);
    			
	    nfui_signal_emit(g_btnLang, GDK_EXPOSE, FALSE);
	}

	return FALSE;
}

static gboolean post_langapply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
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
	
static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        essential_data.sigtype = nfui_spin_button_get_index(g_objSig);
        
		g_memmove(&g_wizard_data->essentialData, &essential_data, sizeof(ESSENTIAL_DATA_T));

		DAL_set_language(essential_data.lang);
		DAL_set_fac_init_run(FALSE);
		DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
		DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

		if(org_essential_data.sigtype != essential_data.sigtype)
		{
			NFOBJECT *top;
			top = nfui_nfobject_get_top(obj);			
			nftool_mbox(top, "NOTICE", "Video type has been changed.\nThe system will be reboot.", NFTOOL_MB_OK);			
			scm_reboot_system(RR_SIGNAL_CHANGE, 0);		
		}
		else
		{
		    evt_send_to_local(IREQ_CHANGE_LANG, 0, 0, 0);
			_next_step_proc();
		}
	}

	return FALSE;
}


gint vw_wizard_essential_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;
	NFOBJECT *btns[PIB_BUTTONS];

    const gchar *strCategory[] = {"SETTING LANGUAGE", "SETTING AC POWER FREQUENCY"};
    const gchar *strSigType[] = {"60Hz", "50Hz"};
	const gchar *strButton[] = {"LANGUAGE APPLY", "OK"};

    gint sigType;
	gint pos_x,pos_y,size_w,size_h;
    gint i, cnt = 1;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;
//<------DB LOAD

    memset(&org_essential_data, 0x00, sizeof(ESSENTIAL_DATA_T));
    memset(&essential_data, 0x00, sizeof(ESSENTIAL_DATA_T));

    DAL_get_language(essential_data.lang);
    essential_data.sigtype = DAL_get_sig_type();

    g_memmove(&org_essential_data, &essential_data, sizeof(ESSENTIAL_DATA_T));
    g_memmove(&g_wizard_data->essentialData, &essential_data, sizeof(ESSENTIAL_DATA_T));

//<------MAKE WINDOW & FIXED
	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

//<-------LANGUAGE SETTING
	if (!_init_lang()) 
	{
		g_warning("%s [%d] : init support language error", __FUNCTION__, __LINE__);
		return;
	}

	pos_x = (guint)4;
	pos_y = (guint)4;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory[0]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, CATEGORY_LABEL_LEFT, pos_y);

	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x = (guint)SUBJECT_LABEL_LEFT;
	pos_y += (guint)60;

	for (i = 0; i < g_langCnt; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);
		g_btnLang[i] = obj;

		if (i == 0) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} 
		else 
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(g_strLang[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID/3 - size_w - 20, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + size_w + 10, pos_y);

        if ((cnt % 3) == 0 || cnt == g_langCnt)
        {
            pos_y += (guint)size_h + 10;
            pos_x = SUBJECT_LABEL_LEFT;
        }
    	else
    	{
        	pos_x += PI_FIXED_SIZE_WID/3 - 10;
    	}
    	cnt++;
	}

//<-------- Make Button
    
	for ( i = 0; i < PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], MENU_BTN_WIDTH);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_LANGAPPLY], MENU_V_BTN_R1_X, pos_y);
	nfui_regi_post_event_callback(btns[PIB_LANGAPPLY], post_langapply_button_event_handler);
	btns[PIB_LANGAPPLY] = g_btnLang;
	nfui_nfobject_get_size(btns[PIB_LANGAPPLY], &size_w, &size_h);

	pos_y += size_h + 20;

//<-------- AC POWER FREQUENCY
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory[1]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, CATEGORY_LABEL_LEFT, pos_y);
	nfui_nfobject_get_size(obj, &size_w, &size_h);

    pos_y += size_h + 20;
	obj = nfui_nflabel_new_with_pango_font("AC POWER FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID/2, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	
    pos_x += PI_FIXED_SIZE_WID/2;
    sigType = org_essential_data.sigtype;
    
	obj = nfui_spinbutton_new(strSigType, NUM_SIGNAL, sigType);
	nfui_nfobject_set_size(obj, 230, 40);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, MENU_V_BTN_R1_X, pos_y);
	g_objSig = obj;

    pos_y += 40 + 25;
    pos_x = SUBJECT_LABEL_LEFT;
	obj = nfui_nflabel_new_with_pango_font(HELP_STR, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

//<-------- OK button
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_OK], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_OK], post_okbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_OK], TRUE);

	gtk_main();

	return 0;

}
