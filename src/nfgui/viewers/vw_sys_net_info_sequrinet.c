#include <string.h>

#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfspinbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"

#include "tools/nf_ui_tool.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_info.h"
#include "vw_sys_net_info_detail.h"
#include "vw_sys_net_info_detail_popup.h"
#include "vw_qr_code_popup.h"


static NFOBJECT *g_curwnd;


static void _set_url(gchar *qr_url, gint len)
{
    gchar mac_string[32];
    gint mac_len = 32;
    
    if(DAL_is_ddns_on() == TRUE)
    {
        scm_get_dvr_addr_str(qr_url, len);
    }
    else
    {
        scm_get_mac_addr_str(mac_string, mac_len);
    	DAL_get_QR_Code_URL(qr_url);
	
    	strcat(qr_url,"?id=");
    	strcat(qr_url,mac_string);
    }
}

static gboolean post_easyip_diagnosis_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ret;
    NF_NOTIFY_INFO pInfo;
    gchar qr_url[256] = {0,};
	gchar strbuf[128] = {0,};
    gint url_len = 256;
    
    if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		scm_get_wan_status_event_data(&pInfo);
        if(!pInfo.d.params[0]){
			#if defined(_SEQURINET_STRING_FIX)
                strcpy(strbuf, "SEQURINET");
            #else
                strcpy(strbuf, "P2P");
            #endif

    		_set_url(qr_url, url_len);
        	if(ret == 0){
				VW_QR_code_Open(g_curwnd, strbuf, qr_url, 200, 200);
            }
        }
	    else{
            nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
        }

	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

	}

	return FALSE;
}



static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
    }

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

void vw_init_NetInfoSequrinet_Page(NFOBJECT *parent)
{
    NFOBJECT *obj, *content_fixed;
    const gchar *title_name[] = {"EASY IP"};
    gint title_ypos[2] = { 13};
    gint i, pos_x, pos_y;


    /* FIXED */
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    g_curwnd = content_fixed;

    /* cate title */

    obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, title_name[0]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 13);

	
    /* p2p diagnosis */

    pos_x = 27;
    pos_y = title_ypos[0] + 61;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EASY IP DIAGNOSIS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 400, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	
    obj = nftool_normal_button_create_type3("TEST", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+400, pos_y);
    nfui_regi_post_event_callback(obj, post_easyip_diagnosis_button_event_handler);
    
    
    /* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


		
    return;
}




