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
#include "vw_sys_camera_ipcam_ptz.h"
	
void VW_Init_IPCamPtz_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *obj;

	obj = nftool_normal_button_create_type1("COPY SCHEDULE TO", COPY_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, COPY_BTN_X, COPY_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R3_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R2_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_IPCAMSET_SUBTAB_BTN_R1_X, MENU_V_IPCAMSET_SUBTAB_BTN_Y);
//	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
}

gint IPCamPtz_update_channel()
{


    return 0;
}

gboolean IPCamPtz_tab_in_handler()
{
	



	return FALSE;
}

gboolean IPCamPtz_tab_out_handler()
{


	return FALSE;
}


