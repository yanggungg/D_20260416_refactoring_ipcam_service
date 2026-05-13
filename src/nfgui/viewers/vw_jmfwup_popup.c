#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfimglabel.h"

#include "vw_disk_main.h"
#include "vw_disk_raid.h"
#include "vw_disk_raid_internal.h"
#include "libsst.h" 

#define MAX_MARGIM_SIZE			(guint)12

#define PI_WND_SIZE_WID			(guint)(400)
#define PI_WND_SIZE_HEI			(guint)(200)

#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		(guint)(12)
#define PI_FIXED_POS_Y		(guint)(56)
#define PI_FIXED_SIZE_WID	(guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	(guint)(MENU_V_BTN_Y - PI_FIXED_POS_Y)

#define MENU_BTN_WIDTH					(162)
#define MENU_BTN_HEIGHT					(44)
#define MENU_BTN_GAP					(4)

#define MENU_V_BTN_R_START_X			((PI_WND_SIZE_WID - MENU_BTN_WIDTH)/2)
#define MENU_V_BTN_Y					(PI_WND_SIZE_HEI - 10 - MENU_BTN_HEIGHT)

static NFWINDOW *g_curwnd = 0;
static GdkPixbuf *g_pbuf = NULL;
static gint g_pbuf_w = 0;
static gint g_pbuf_h = 0;
static NFOBJECT *g_wbox = NULL;

static gboolean popup_wbox(gpointer data)
{
	static gint cnt = 0;

	if(!g_wbox) 
		g_wbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	
	if(cnt++ > 3) {
		cnt = 0;
		gtk_main_quit();
		return FALSE;
	}

	return TRUE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		uxm_unreg_imsg_event(obj, IRET_SCM_JMFW_UPDATE);
	}
	else if(evt->type == IRET_SCM_JMFW_UPDATE)
	{
		gint ret = ((CMM_MESSAGE_T *)data)->param;

		if(g_wbox) 
		{
			nftool_remove_waitbox(g_wbox);
			g_wbox = NULL;
		}
				
		if(ret <= 0) 
		{
			nftool_mbox_auto(g_curwnd, 3, "WARNING", "F/W Upgrade Fail.\nThe system will be reboot soon."); 
		}
		else 
		{ 
			nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_OK);			
		}

		scm_reboot_system(RR_NA, 0);		
	}
		return FALSE;

}

static gboolean post_update_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ret;
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		g_timeout_add(100, popup_wbox, NULL);
		
		gtk_main();
		scm_jm_update(IRET_SCM_JMFW_UPDATE);
		g_message(" %s :::::::::line: %d:::::::::::::::: ret : %d", __FUNCTION__, __LINE__, ret);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		nfui_nfobject_destroy(g_curwnd);			
	}

	return FALSE;
}

gboolean VW_Jmfwup_Popup_Open(NFWINDOW *parent)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;

	char title[32];
    GdkPixbuf *pbuf = NULL;
	FIRMWARE_INFO tFwInfo;
    gchar temp_char[1024];
	
	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, "UPDATE", FALSE);
	nfui_nfwindow_set_title(main_wnd, "JMFWUP POPUP INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	nfui_nfobject_show(main_wnd);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	memset(&tFwInfo, 0x00, sizeof(FIRMWARE_INFO));
	scm_get_jm_fw_ver(&tFwInfo);
	memset(temp_char, 0x00, sizeof(temp_char));
	sprintf(temp_char," FW VER 0x%04x : 0x%04x",tFwInfo.FirmwareVersion, tFwInfo.FirmwareDateCode);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(temp_char, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
	nfui_nfbutton_set_font_alignment(obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 0, 20);

	obj = nftool_normal_button_create_type1("UPDATE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_update_button_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 30, MENU_V_BTN_Y);
	nfui_nfobject_show(obj);

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 208, MENU_V_BTN_Y);
	nfui_nfobject_show(obj);

	/* set for key navi */
	nfui_make_key_hierarchy(main_fixed);
	nfui_set_key_focus(obj, TRUE);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_JMFW_UPDATE);

	return FALSE;
}

