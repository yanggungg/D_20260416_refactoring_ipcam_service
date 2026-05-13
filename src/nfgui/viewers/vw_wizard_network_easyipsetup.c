#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

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

#include "ix_mem.h"
#include "scm.h"
#include "ssm.h"

#include "vw_wizard_init.h"
#include "vw_qr_code_popup.h"


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


#define IPS_LABEL_HEIGHT            (40)
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

enum{
    YES,
    NO,
    YESNO
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_radio_btn[YESNO] = {0,};
static WIZARD_USERDATA_T *g_wizard_data;
static gint g_use_sequrinet;


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

static gint _trans_val_to_idx(gboolean status)
{
    gint idx;
    
    if (status) {
        idx = 0;
    }
    else {
        idx = 1;
    }

    return idx;
}

static gboolean _trans_idx_to_val(gint idx)
{
    gboolean status;

    if (idx) {
        status = FALSE;
    }
    else {
        status = TRUE;
    }

    return status;
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

static gint _save_data(gboolean data)
{
    g_wizard_data->networkData.use_sequrinet = data;
    g_wizard_data->networkData.ddns_data.p2p_enable = data;
    
    DAL_set_Sequrinet_Status(data);
    DAL_set_ddns_data(&g_wizard_data->networkData.ddns_data);

    return 0;
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
		NF_NOTIFY_INFO pInfo;
        gchar qr_url[256] = {0,};
		gchar strbuf[128] = {0,};
        gint url_len = 256;
        gboolean auth_sequrinet;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK)	return FALSE;
		}

		if(nfui_radio_button_get_toggled((NFBUTTON*)g_radio_btn[YES]))
		{
		    auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);
		    scm_get_wan_status_event_data(&pInfo);

		    if (auth_sequrinet)
		    {
                if(!pInfo.d.params[0]){
					#if defined(_SEQURINET_STRING_FIX)
						strcpy(strbuf, "SEQURINET");
					#else
						strcpy(strbuf, "P2P");
					#endif

            		var_get_qr_url(qr_url, url_len);
					VW_QR_code_Open(g_curwnd, strbuf, qr_url, 200, 200);
                }
        	    else{
                    nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
                }
            }
            _save_data(TRUE);
		}
		else
		{
		    _save_data(FALSE);
		}

		_next_step_proc();
	}

	return FALSE;
}

static gboolean post_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case NFEVENT_RADIO_GET_FOCUS:
        case NFEVENT_RADIO_GET_FOCUS_ALWAYS:
        	{
        	    if(obj == g_radio_btn[YES])
        	    {
                    _save_data(TRUE);

        	    }
        	    else
        	    {
        	        _save_data(FALSE);
        	    }
        	}
        	break;
	}

	return FALSE;
}

gint vw_wizard_network_easyipsetup_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;
	NFOBJECT *btns[PIB_BUTTONS];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;

	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
	const gchar *strTitle[] = {"Yes", "No"};
#if defined(_SEQURINET_STRING_FIX)
	const gchar *strExplan[] = {"What is Sequrinet?",
	                            "The Sequrinet service is an easy and convenient way", 
	                            "to access your devices from a smartphone.",
	                            "Enjoy a completely new experience with Sequrinet.",
	                            "Would you like to set the Sequrinet?"};
#else
	const gchar *strExplan[] = {"What is P2P?",
	                            "The P2P service is an easy and convenient way", 
	                            "to access your devices from a smartphone.",
	                            "Enjoy a completely new experience with P2P.",
	                            "Would you like to set the P2P?"};
#endif
	guint pos_x, pos_y;
	gint size_w, size_h;
	guint i, ret;
	gboolean status, auth_sequrinet;

    char title[64];
	gchar strbuf[128] = {0,};

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

    g_use_sequrinet = g_wizard_data->networkData.use_sequrinet;

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

#if defined(_SEQURINET_STRING_FIX)
	strcpy(strbuf, "SETTING UP SEQURINET");
#else
	strcpy(strbuf, "SETTING UP P2P");
#endif
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strbuf);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    pos_x += (guint)23;
	pos_y += (guint)60;
    for(i = 0; i < 5; i++)
    {
    	obj = nfui_nflabel_new_with_pango_font(strExplan[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID-30, 35);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    	nfui_nfobject_use_focus(obj, FALSE);
    	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    	nfui_nfobject_show(obj);
    	if(i == 3) pos_y += 50;
    	else       pos_y += 30;
    }

	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x += (guint)5;
	pos_y += (guint)20;
    
    DAL_get_Sequrinet_Status(&status);

    auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);

	for(i=0; i<YESNO; i++) {
		g_radio_btn[i] = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(g_radio_btn[i]), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)g_radio_btn[i], FALSE);
		nfui_nfobject_set_size(g_radio_btn[i], (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(g_radio_btn[i], post_radio_event_handler);
		nfui_nfobject_show(g_radio_btn[i]);

		if(i == YES) {
			slist = nfui_radio_button_get_group(NF_BUTTON(g_radio_btn[i]));
		} 
		else {
			pos_y += (guint)size_h + 15;
			nfui_radio_button_add_group(NF_BUTTON(g_radio_btn[i]), slist);

		}
		nfui_nffixed_put((NFFIXED*)fixed1, g_radio_btn[i], pos_x,pos_y);

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, 150, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 40, pos_y);

		if (!auth_sequrinet) {
		    nfui_nfobject_disable(g_radio_btn[i]);
    		nfui_nflabel_modify_fg((NFLABEL*)obj, COLOR_IDX(135));
		}
	}
	nfui_radio_button_set_toggled(NF_BUTTON(g_radio_btn[_trans_val_to_idx(g_use_sequrinet)]), TRUE);

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

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	gtk_main();

    return 0;
}

