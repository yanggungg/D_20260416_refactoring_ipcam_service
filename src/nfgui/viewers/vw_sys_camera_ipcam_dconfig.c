#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_dconfig.h"
#include "vw_sys_camera_ipcam_dconfig_mode.h"

#define PAGE_STR1   "If you press 'START' button, you can configure the IP cameras directly by the web access."
#define PAGE_STR2   "Warning : In this mode, the video recording will be stopped automatically."

static NFWINDOW *g_curwnd = 0;
static CAM_PROFILE_T *g_prof;

static gint g_channel;
static NFOBJECT *g_start_obj;

static gboolean post_start_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static NFOBJECT *wait_box = 0;    
	mb_type ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

    	ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to continue?", NFTOOL_MB_OKCANCEL);

        if (ret == NFTOOL_MB_OK)
        {
            uxm_reg_imsg_event(obj, IRET_SCM_DCONFIG_MODE);
            scm_enter_direct_mode(IRET_SCM_DCONFIG_MODE, g_channel);

            if (!wait_box)
                wait_box = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");            
        }
	}
	else if (evt->type == IRET_SCM_DCONFIG_MODE)
	{
		int ret = ((CMM_MESSAGE_T *)data)->param;
		
		if(wait_box) {
			nftool_remove_waitbox(wait_box);
			wait_box = 0;
		}

        uxm_unreg_imsg_event(obj, IRET_SCM_DCONFIG_MODE);

        if (ret == 0)
        {
            VW_Open_DirectConfig_mode_Page(g_curwnd, g_channel);
        }
        else
        {
            nftool_mbox(g_curwnd, "ERROR", "FAIL", NFTOOL_MB_OK);
        }
	}
	
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)			
			return FALSE;
	
		IPCamDirectConfig_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}
	
void VW_Init_IPCamDirectConfig_Page(NFOBJECT *parent, gint ch, CAM_PROFILE_T *prof)
{
    NFOBJECT *obj;

	g_curwnd = nfui_nfobject_get_top(parent);

    g_channel = ch;
    g_prof = prof;

	obj = nftool_normal_button_create_type3("START", 196);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 50, 50);
	nfui_regi_post_event_callback(obj, post_start_button_event_handler);
    g_start_obj = obj;

    if (g_prof[ch].connected) 
    {
        if (scm_support_direct_config(ch) == 0)
            nfui_nfobject_enable(obj);
    }

	obj = nfui_nflabel_new_with_pango_font("*", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 20, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 40, 130);

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR1, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1000, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 60, 130);

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR2, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 1000, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));		
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 60, 170);

//	obj = nftool_normal_button_create_type1("COPY SETTINGS TO", COPY_BTN_W);
//	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
//	nfui_nfobject_show(obj);
//	nfui_nffixed_put((NFFIXED*)parent, obj, COPY_BTN_X, COPY_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);

//	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
//	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
//	nfui_nfobject_show(obj);
//	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R3_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

//	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
//	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
//	nfui_nfobject_show(obj);
//	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R2_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R1_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
}

gint IPCamDirectConfig_update_channel(gint ch)
{
    nfui_nfobject_disable(g_start_obj); 

    if (g_prof[ch].connected)   
    {
        if (scm_support_direct_config(ch) == 0)   
            nfui_nfobject_enable(g_start_obj);
    }

    nfui_signal_emit(g_start_obj, GDK_EXPOSE, TRUE);

    g_channel = ch;

    return 0;
}

gboolean IPCamDirectConfig_tab_in_handler()
{
	



	return FALSE;
}

gboolean IPCamDirectConfig_tab_out_handler()
{


	return FALSE;
}


