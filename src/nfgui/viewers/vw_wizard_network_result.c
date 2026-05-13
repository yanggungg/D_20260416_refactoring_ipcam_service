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
#include "objects/nfipeditor.h"

#include "vw_desc.h"
#include "vw_image_popup.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_result.h"

#include "ix_mem.h"
#include "scm.h"
#include "ssm.h"

#define MAX_MARGIM_SIZE         (guint)12

#define PI_WND_SIZE_WID			(guint)(610 + 200)
#define PI_WND_SIZE_HEI			(guint)(520 + 200)

#define SE_PP_POS_X             (guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y             (guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X      (guint)(12)
#define PI_FIXED_POS_Y      (guint)(56)
#define PI_FIXED_SIZE_WID   (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI   (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH                  (162)
#define MENU_BTN_HEIGHT                 (44)
#define MENU_BTN_GAP                    (4)

#define MENU_V_BTN_R_START_X            (PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X                 (MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X                 (MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y                    (PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define IPS_LABEL_HEIGHT            (40)
#define IPS_LABEL_ROW_SPACE         (2)
#define SUBJECT_LABEL_LEFT          (28)
#define SUBJECT_LABEL_TOP           (42)
#define SUBJECT_LABEL_WIDTH         (396)
#define SUBJECT_LABEL_WIDTH1        (360)
#define SUBJECT_LABEL_MARGIN        (0)

typedef enum
{
  NIS_DHCP_ROW = 0,
  NIS_IP_ROW,
  NIS_GATEWAY_ROW,
  NIS_SUBNET_ROW,
  NIS_1ST_DNS_ROW,
  NIS_2ND_DNS_ROW,
  NIS_RTSP_PORT_ROW,
  NIS_WEB_PORT_ROW,
  NUM_NIS_ROWS,
}nis_row_type;

enum{
	AU_HIGH_QUAL 			= 0,
	AU_MOT 					= 1,
	AU_ALARM 				= 2,
	AU_MOT_ALARM 			= 3,
	AU_INTENSIVE_MOT 		= 4,
	AU_INTENSIVE_ALARM 		= 5,
	AU_INTENSIVE_MOT_ALARM 	= 6,
	AU_CONFIG_MAX
};

enum {
    PIB_PREVIOUS,
    PIB_NEXT,
    PIB_EXIT,
    PIB_BUTTONS
};

static gint support_onestop = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[NUM_NIS_ROWS];
static NFOBJECT *wait_mbox = NULL;

static WIZARD_USERDATA_T *g_wizard_data;


static gint _prev_proc()
{    
    nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;
}

static gint _close_proc(gpointer data)
{
    NFOBJECT *topwin;
    
    topwin = nfui_nfobject_get_top(g_curwnd);

    nfui_nfobject_destroy(topwin);      

     _wizard_close();

     return 0;
}

static gint _get_str_record_mode(gchar *buf)
{
	switch(g_wizard_data->recordData.autoConfig)
	{
		case AU_HIGH_QUAL:
		    strcpy(buf, "CONTINUOUS RECORD");
    		break;		
    		
		case AU_MOT:
    		strcpy(buf, "MOTION RECORD");
    		break;
    		
		case AU_ALARM:
		    strcpy(buf, "AI/ALARM RECORD");
    		break;
    		
		case AU_MOT_ALARM:
		    strcpy(buf, "MOTION/AI/ALARM RECORD");
    		break;
    		
		case AU_INTENSIVE_MOT:
		    strcpy(buf, "INTENSIVE MOTION RECORD");
    		break;
    		
		case AU_INTENSIVE_ALARM:
		    strcpy(buf, "INTENSIVE AI/ALARM RECORD");
    		break;
    		
		case AU_INTENSIVE_MOT_ALARM:
		    strcpy(buf, "INTENSIVE MOTION/AI/ALARM RECORD");
    		break;
	}

    return 0;
}

static gint _get_str_timezone(gchar *buf)
{
    gchar *strTimezone[50];
    gint zone_count;
    gint i;
    
	zone_count = nf_zoneinfo_get_count();
	
	for(i=0; i<zone_count; i++)
	{
		strTimezone[i] = nf_zoneinfo_get_string((gint)i);
	}

	strcpy(buf, strTimezone[g_wizard_data->dtData.timeZone]);

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
            if (wait_mbox)
            {
                nftool_remove_waitbox(wait_mbox);
                wait_mbox = NULL;
            }

            g_curwnd = 0;
            gtk_main_quit();
        }
    
        return FALSE;

}

static gboolean post_qrcodebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        gchar qr_url[256] = {0,};
		gchar strbuf[128] = {0,};
        gint url_len = 256;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;     
        
		#if defined(_SEQURINET_STRING_FIX)
            strcpy(strbuf, "SEQURINET");
        #else
            strcpy(strbuf, "P2P");
        #endif

		var_get_qr_url(qr_url, url_len);
		VW_QR_code_Open(g_curwnd, strbuf, qr_url, 200, 200);
    }
    return FALSE;
}

static gboolean post_datasavebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		guint x, y;
		gchar title[64] = "EXPORT SETTINGS";
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		if(!x && !y) 
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}

		VW_System_Data_Save_Open(g_curwnd, title, x, (y - 252));
	}

	return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        _prev_proc();
    }

    return FALSE;
}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _close_proc(NULL);
        /*
        wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

        g_timeout_add(300, _close_proc, NULL);
        */ 
    }
 
    return FALSE;
}

gint vw_wizard_network_result_open(gpointer parent, gpointer user_data)
{
    NFOBJECT *main_wnd, *main_fixed;
    NFOBJECT *fixed1;
    NFOBJECT *obj;
    NFOBJECT *btns[PIB_BUTTONS];
    NFOBJECT *btnSave;
    NFOBJECT *btn_qr;

    const gchar *strTitle[] = {"", "IP ADDRESS", "GATEWAY", "SUBNET MASK", "1ST DNS SERVER", "2ND DNS SERVER", "RTSP SERVICE PORT", "WEB SERVICE PORT"};
    const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};
    gchar buf[64] = {0,};
	gchar strbuf[128] = {0,};

    guint pos_x, pos_y;
    guint i, ret;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;
    

    main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
    nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    fixed1 = nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
    nfui_nfobject_show(fixed1);

    pos_x = 4;
    pos_y = 4;

//----->TIMEZONE

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMEZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    _get_str_timezone(buf);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

    pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);

//-----> RECORDING CONFIGURATION

    if (g_wizard_data->recordData.mode == AUTO_CONFIG)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RECORDING CONFIGURATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        _get_str_record_mode(buf);
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    }

//----->IP SETUP    
    value[NIS_IP_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
                            g_wizard_data->networkData.ipsetup_data.ip[0], 
                            g_wizard_data->networkData.ipsetup_data.ip[1], 
                            g_wizard_data->networkData.ipsetup_data.ip[2], 
                            g_wizard_data->networkData.ipsetup_data.ip[3]);
    
    value[NIS_GATEWAY_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
                            g_wizard_data->networkData.ipsetup_data.gateway[0], 
                            g_wizard_data->networkData.ipsetup_data.gateway[1], 
                            g_wizard_data->networkData.ipsetup_data.gateway[2], 
                            g_wizard_data->networkData.ipsetup_data.gateway[3]);
    
    value[NIS_SUBNET_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
                            g_wizard_data->networkData.ipsetup_data.subnet[0], 
                            g_wizard_data->networkData.ipsetup_data.subnet[1],
                            g_wizard_data->networkData.ipsetup_data.subnet[2], 
                            g_wizard_data->networkData.ipsetup_data.subnet[3]);
    
    value[NIS_1ST_DNS_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
                            g_wizard_data->networkData.ipsetup_data.dns1[0], 
                            g_wizard_data->networkData.ipsetup_data.dns1[1], 
                            g_wizard_data->networkData.ipsetup_data.dns1[2], 
                            g_wizard_data->networkData.ipsetup_data.dns1[3]);

    value[NIS_2ND_DNS_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
                            g_wizard_data->networkData.ipsetup_data.dns2[0], 
                            g_wizard_data->networkData.ipsetup_data.dns2[1], 
                            g_wizard_data->networkData.ipsetup_data.dns2[2], 
                            g_wizard_data->networkData.ipsetup_data.dns2[3]);

    value[NIS_RTSP_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    value[NIS_WEB_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP SETUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    if(g_wizard_data->networkData.ipsetup_data.dhcp)
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DHCP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    else
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATIC IP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
 
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

    pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    
//----->IP address ... second dns server
    for(i=1; i<NIS_GATEWAY_ROW; i++)
    {

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    
        if(i == NIS_RTSP_PORT_ROW)
        {
            nfui_nflabel_use_number(value[i], TRUE);
            nfui_nflabel_set_number(value[i], g_wizard_data->networkData.ipsetup_data.rtspport);
            nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
        }
        else if(i == NIS_WEB_PORT_ROW)
        {
            nfui_nflabel_use_number(value[i], TRUE);
            nfui_nflabel_set_number(value[i], g_wizard_data->networkData.ipsetup_data.rtspport);
            nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
        }
        else
        {
            nfui_nfipeditor_set_pango_font(value[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
            nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        }
        
        nfui_nfobject_use_focus(value[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(value[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nffixed_put((NFFIXED*)fixed1, value[i], pos_x + SUBJECT_LABEL_WIDTH1, pos_y);
        nfui_nfobject_show(value[i]);


        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    }

//----->AUTO PORT FORWARDING
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUTO PORT FORWARDING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    if(g_wizard_data->networkData.ipsetup_data.autoPort)
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Enable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    else
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Disable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

    pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    
//----->RTSP -> WEB PORT

    for(i=6; i<NUM_NIS_ROWS; i++)
    {

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    
        if(i == NIS_RTSP_PORT_ROW)
        {
            nfui_nflabel_use_number(value[i], TRUE);
            nfui_nflabel_set_number(value[i], g_wizard_data->networkData.ipsetup_data.rtspport);
            nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
        }
        else if(i == NIS_WEB_PORT_ROW)
        {
            nfui_nflabel_use_number(value[i], TRUE);
            nfui_nflabel_set_number(value[i], g_wizard_data->networkData.ipsetup_data.webPort);
            nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
        }
        
        nfui_nfobject_use_focus(value[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(value[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nffixed_put((NFFIXED*)fixed1, value[i], pos_x + SUBJECT_LABEL_WIDTH1, pos_y);
        nfui_nfobject_show(value[i]);


        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    }

//----->ONESTOP

    if (ivsc.dfunc.support_p2p)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ONESTOP SERVICE SETTING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        if (g_wizard_data->networkData.ddns_data.p2p_enable) 
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Enable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        else 
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Disable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);  
    }   

//----->DDNS SETUP
   
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SETTING THE DDNS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    if(g_wizard_data->networkData.ddns_data.enable)
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Enable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
    else
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Disable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));    
    
    nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

    pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    
//----->DDNS SERVER

    if(g_wizard_data->networkData.ddns_data.enable)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DDNS SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        if (!strcmp(g_wizard_data->networkData.ddns_data.server, "s1.co.kr"))
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(g_wizard_data->networkData.ddns_data.serverip, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        else
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(g_wizard_data->networkData.ddns_data.server, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));  
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);

        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    }

//----->SEQURINET

    if (var_get_supported_sequrinet())
    {
		#if defined(_SEQURINET_STRING_FIX)
            strcpy(strbuf, "SEQURINET");
        #else
            strcpy(strbuf, "P2P");
        #endif

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strbuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        if (ssm_check_access_auth(USR_AUTH_SEQURINET)) 
        {
            DAL_get_Sequrinet_Status(&g_wizard_data->networkData.use_sequrinet);
            if (g_wizard_data->networkData.use_sequrinet) 
                obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Enable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
            else 
                obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Disable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        }
        else
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Disable", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
        }
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);
#if 0
        btn_qr = nftool_normal_button_create_type3("DETAIL", SUBJECT_LABEL_WIDTH);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(btn_qr), NFALIGN_CENTER, 0);
        nfui_nfobject_show(btn_qr);
        nfui_nffixed_put((NFFIXED*)fixed1, btn_qr, pos_x+SUBJECT_LABEL_WIDTH1, pos_y);
        nfui_regi_post_event_callback(btn_qr, post_qrcodebutton_event_handler);                       
#endif
        pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
    }

//----->BUTTON

    btnSave = nftool_normal_button_create_type1("EXPORT SETTINGS", 270);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(btnSave), NFALIGN_CENTER, 0);
    nfui_nfobject_show(btnSave);

    for(i = 0; i < PIB_BUTTONS; i++ )
    {
        btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);    
        nfui_nfobject_show(btns[i]);
    }

    nfui_nfobject_disable(btns[PIB_NEXT]);
    nfui_nffixed_put((NFFIXED*)fixed1, btnSave, MENU_BTN_GAP, MENU_V_BTN_Y);
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

    nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler); 
    nfui_regi_post_event_callback(btnSave, post_datasavebutton_event_handler);
    nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
    nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);

    nfui_nfobject_show(main_wnd);

    /* set for key navi */
    nfui_make_key_hierarchy(main_wnd);
    nfui_set_key_focus(btns[PIB_EXIT], TRUE);

    gtk_main();

    return 0;
}
