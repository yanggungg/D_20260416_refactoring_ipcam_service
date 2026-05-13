
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_event_noti_evt.h"
#include "vw_event_sms_noti.h"

#include "vw_vkeyboard.h"
#include "log.h"
#include "scm.h"
#include "ix_mem.h"

#define _SUPPORT_ONLY_BIZPPURIO

#define FTP_NOTI_HELP_BTN_Y				(MENU_V_SUBTAB_INNER_H - 40 - 4)

enum {
	FREQ_1_MIN = 0,
	FREQ_5_MIN,
	FREQ_10_MIN,
	FREQ_15_MIN,
	FREQ_30_MIN,
	FREQ_60_MIN,
	
	NUM_FREQS
};

static EA_EvtNotiSMSData g_esd;
static EA_EvtNotiSMSData g_oesd;

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_server_obj = 0;
static NFOBJECT *g_appid_obj = 0;
static NFOBJECT *g_user_obj = 0;
static NFOBJECT *g_password_obj = 0;
static NFOBJECT *g_sched_from_obj = 0;
static NFOBJECT *g_sched_to_obj = 0;
static NFOBJECT *g_count_obj = 0;
static NFOBJECT *g_testnum_obj = 0;
static NFOBJECT *g_test_obj = 0;


static gint init_sms_noti_obj()
{
	gint i, match = -1;

	if (nf_sms_vendor_get_count())
	{
		if (nf_sms_vendor_get_count() > 1)
		{
		for (i = 0; i < nf_sms_vendor_get_count(); i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX*)g_server_obj, nf_sms_vendor_get_string(i));	
			if (!strcmp(nf_sms_vendor_get_string(i), g_esd.server)) match = i;
		}

		g_message("%s, %d, matched index:%d", __FUNCTION__, __LINE__, match);

		if (match != -1) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_server_obj, match);
		}
		else
		{
			nfui_nflabel_set_text((NFLABEL*)g_server_obj, g_esd.server);
		}	

		nfui_nflabel_set_text((NFLABEL*)g_appid_obj, g_esd.appid);		
		nfui_nflabel_set_text((NFLABEL*)g_user_obj, g_esd.user);
		nfui_nflabel_set_text((NFLABEL*)g_password_obj, g_esd.password);		
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sched_from_obj, g_esd.sched_from);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sched_to_obj, g_esd.sched_to);
		nfui_nflabel_set_number((NFLABEL*)g_count_obj, g_esd.count);

		if (strcmp(g_esd.server, "CLICKATELL")) nfui_nfobject_disable(g_appid_obj);

		nfui_nfobject_disable(g_sched_from_obj);
		nfui_nfobject_disable(g_sched_to_obj);
		nfui_nfobject_disable(g_count_obj);	
	}
	else
	{
		nfui_nfobject_disable(g_server_obj);
		nfui_nfobject_disable(g_appid_obj);
		nfui_nfobject_disable(g_user_obj);
		nfui_nfobject_disable(g_password_obj);
		nfui_nfobject_disable(g_sched_from_obj);
		nfui_nfobject_disable(g_sched_to_obj);
		nfui_nfobject_disable(g_count_obj);		
		nfui_nfobject_disable(g_testnum_obj);
		nfui_nfobject_disable(g_test_obj);
	}

	return 0;
}

static void init_es_data()
{
	memset(&g_esd, 0x00, sizeof(EA_EvtNotiSMSData));
	memset(&g_oesd, 0x00, sizeof(EA_EvtNotiSMSData));

	DAL_get_evtNoti_sms_data(&g_esd);
	g_memmove(&g_oesd, &g_esd, sizeof(EA_EvtNotiSMSData));
}

static void set_es_data()
{
	g_memmove(&g_oesd, &g_esd, sizeof(EA_EvtNotiSMSData));

	DAL_set_evtNoti_sms_data(g_esd);
	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

static gint set_data_to_obj(gint expose)
{
	gint i, match = -1;

	if (!nf_sms_vendor_get_count()) return -1;

	if (nf_sms_vendor_get_count() > 1)
	{
		for (i = 0; i < nf_sms_vendor_get_count(); i++)
		{
			if (!strcmp(nf_sms_vendor_get_string(i), g_esd.server)) match = i;
		}

		if (match != -1) nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_server_obj, match);	
	}

	nfui_nflabel_set_text((NFLABEL*)g_appid_obj, g_esd.appid);
	nfui_nflabel_set_text((NFLABEL*)g_user_obj, g_esd.user);
	nfui_nflabel_set_text((NFLABEL*)g_password_obj, g_esd.password);		
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sched_from_obj, g_esd.sched_from);
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sched_to_obj, g_esd.sched_to);
	nfui_nflabel_set_number((NFLABEL*)g_count_obj, g_esd.count);

	if (!strcmp(g_esd.server, "CLICKATELL")) nfui_nfobject_enable(g_appid_obj);	
	else nfui_nfobject_disable(g_appid_obj);	

    if (expose) {
 		nfui_signal_emit(g_server_obj, GDK_EXPOSE, TRUE);
 		nfui_signal_emit(g_appid_obj, GDK_EXPOSE, TRUE);
 		nfui_signal_emit(g_user_obj, GDK_EXPOSE, TRUE);
 		nfui_signal_emit(g_password_obj, GDK_EXPOSE, TRUE);
 		nfui_signal_emit(g_sched_from_obj, GDK_EXPOSE, TRUE);
 		nfui_signal_emit(g_sched_to_obj, GDK_EXPOSE, TRUE); 		
 		nfui_signal_emit(g_count_obj, GDK_EXPOSE, TRUE); 		
    }    

    return 0;
}

static gboolean post_server_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		strcpy(g_esd.server, nfui_combobox_get_value((NFCOMBOBOX*)obj));

		if (!strcmp(g_esd.server, "CLICKATELL")) nfui_nfobject_enable(g_appid_obj);	
		else nfui_nfobject_disable(g_appid_obj);	

		nfui_signal_emit(g_appid_obj, GDK_EXPOSE, TRUE);
	}
	
	return FALSE;
}

static gboolean post_input_normal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		guint off_x, off_y;
		gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
    
		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), off_x, off_y, 31, VKEY_NORMAL);        

        if (strTemp)
        {
	       	nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
        	nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

        	if (g_appid_obj == obj) strcpy(g_esd.appid, strTemp);
        	else if (g_user_obj == obj) strcpy(g_esd.user, strTemp);
        	else if (g_password_obj == obj) strcpy(g_esd.password, strTemp);

        	ifree(strTemp);
        	strTemp = NULL;
        }
	}

	return FALSE;
}

static gboolean post_schedule_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{


	return FALSE;
}

static gboolean post_count_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{


	return FALSE;
}

static gboolean post_testnumber_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		guint off_x, off_y;
		gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
    
		strTemp = NumberKey_Str_Open(g_curwnd, nfui_nflabel_get_text(obj), off_x, off_y, 16);      

        if (strTemp)
        {
	       	nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
        	nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

        	ifree(strTemp);
        	strTemp = NULL;
        }
	}

	return FALSE;
}

static gboolean post_testbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT *wait_mbox = 0;
	
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		SMS_SERVER_T serverinfo;
		SMS_RECEIVER_T receiverinfo;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (wait_mbox) return FALSE;

		memset(&serverinfo, 0x00, sizeof(SMS_SERVER_T));
		memset(&receiverinfo, 0x00, sizeof(SMS_RECEIVER_T));

		strcpy(serverinfo.server, g_esd.server);
		strcpy(serverinfo.appid, g_esd.appid);		
		strcpy(serverinfo.user, g_esd.user);		
		strcpy(serverinfo.password, g_esd.password);		

		strcpy(receiverinfo.number, nfui_nflabel_get_text((NFLABEL*)g_testnum_obj));

		g_message("%s, %d, server_idx:%s", __FUNCTION__, __LINE__, serverinfo.server);
		g_message("%s, %d, username:%s", __FUNCTION__, __LINE__, serverinfo.appid);
		g_message("%s, %d, username:%s", __FUNCTION__, __LINE__, serverinfo.user);
		g_message("%s, %d, passwd:%s", __FUNCTION__, __LINE__, serverinfo.password);	
		
		g_message("%s, %d, number:%s", __FUNCTION__, __LINE__, receiverinfo.number);		
	
		if (scm_test_sms(&serverinfo, &receiverinfo) == -1) 
		{
			nftool_mbox(g_curwnd, "NOTICE", "Test SMS sending failed.", NFTOOL_MB_OK);
			return FALSE;
		}

		wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	}
	else if (evt->type == INFY_SMS_TEST_RESULT)
	{
		CMM_MESSAGE_T *pmsg;
	
		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = 0;
		}

		pmsg = (CMM_MESSAGE_T *)data;

		if (pmsg->param == 1) nftool_mbox(g_curwnd, "NOTICE", "Test SMS sending succeeded.", NFTOOL_MB_OK);
		else if (pmsg->param == -2) nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);
		else nftool_mbox(g_curwnd, "NOTICE", "Test SMS sending failed.", NFTOOL_MB_OK);
	}	
	else if (evt->type == GDK_DELETE)
	{
		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = 0;
		}

		uxm_unreg_imsg_event(obj, INFY_SMS_TEST_RESULT);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(&g_esd, &g_oesd, sizeof(EA_EvtNotiSMSData));
		set_data_to_obj(1);
	}
	
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(memcmp(&g_esd, &g_oesd, sizeof(EA_EvtNotiSMSData))) 
		{
			set_es_data();
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			event_act_data_changed(TRUE);
		}
	}
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		VW_EvtNotiEvt_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_EvtNoti_SMS_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
    gint pos_x, pos_y;

	const gchar *strtimeTitle[] = { "00:00", "01:00", "02:00", "03:00", "04:00",
									"05:00", "06:00", "07:00", "08:00", "09:00",
									"10:00", "11:00", "12:00", "13:00", "14:00",
									"15:00", "16:00", "17:00", "18:00", "19:00",
									"20:00", "21:00", "22:00", "23:00" };


	g_curwnd = nfui_nfobject_get_top(parent);

	init_es_data();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SMS NOTIFICATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 13);

    pos_x = 27;
    pos_y = 74;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	if (nf_sms_vendor_get_count() > 1)
	{
		obj = nfui_combobox_new(0, 0, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 300, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
		nfui_regi_post_event_callback(obj, post_server_event_handler);	
		g_server_obj = obj;
	}
	else
	{
		obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_spacing((NFLABEL *)obj,SEMI_CONDENSED_SPACING);
		nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nfobject_set_size(obj, 300, 40);
		nfui_nfobject_show(obj);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
		g_server_obj = obj;
	}

    pos_y += (40 + 4);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("APP ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);           
	nfui_nfobject_set_size(obj, 300, 40);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_input_normal_event_handler);
	g_appid_obj = obj;
	
    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);           
	nfui_nfobject_set_size(obj, 300, 40);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_input_normal_event_handler);	
	g_user_obj = obj;

    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 300, 40);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_input_normal_event_handler);		
	g_password_obj = obj;
	
    pos_y += (40 + 34);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SCHEDULE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_schedule_event_handler);			
	g_sched_from_obj = obj;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 20, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4+140, pos_y);

	obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4+140+20, pos_y);
	nfui_regi_post_event_callback(obj, post_schedule_event_handler);				
	g_sched_to_obj = obj;

    pos_y += (40 + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SMS LIMIT COUNT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);           
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, 300, 40);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_count_event_handler);
	g_count_obj = obj;

    pos_y += (40 + 34);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TEST NUMBER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 450, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_INPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);           
	nfui_nfobject_set_size(obj, 300, 40);	
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+450+4, pos_y);
	nfui_regi_post_event_callback(obj, post_testnumber_event_handler);	
	g_testnum_obj = obj;

    pos_y += (40 + 4);

	obj = nftool_normal_button_create_type3("TEST", 192);				
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+754-192, pos_y);
	nfui_regi_post_event_callback(obj, post_testbtn_event_handler);		
	uxm_reg_imsg_event(obj, INFY_SMS_TEST_RESULT);	
	g_test_obj = obj;

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
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	init_sms_noti_obj();
}

gboolean check_evt_noti_sms_changed()
{
	if (!memcmp(&g_esd, &g_oesd, sizeof(EA_EvtNotiSMSData))) 
        return FALSE;

	return TRUE;
}

void save_evt_noti_sms_data()
{
	set_es_data();
}

void restore_evt_noti_sms_data()
{
	g_memmove(&g_esd, &g_oesd, sizeof(EA_EvtNotiSMSData));
	set_data_to_obj(0);
}

