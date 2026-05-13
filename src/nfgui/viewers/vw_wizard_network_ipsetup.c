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
#include "objects/nfipeditor.h"

#include "vw_passage_popup.h"
#include "vw_vkeyboard.h"
#include "vw_ip_editor_popup.h"

#include "vw_wizard_init.h"
#include "vw_wizard_network_ipsetup.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"

#define HELP_STR "When you select DHCP, the device IP address,\ngateway, subnet mask, and DNS server\nsettings will be automatically set."

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
#define SUBJECT_LABEL_WIDTH         (360)
#define SUBJECT_LABEL_MARGIN        (0)

typedef enum
{
  NIS_DHCP_ROW = 0,
  NIS_IP_ROW,
  NIS_GATEWAY_ROW,
  NIS_SUBNET_ROW,
  NIS_1ST_DNS_ROW,
  NIS_2ND_DNS_ROW,
  NUM_NIS_ROWS
}nis_row_type;

enum {
	PIB_RENEW,
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

typedef enum {
	LOGLABEL_TYPE = 0,
	LOGLABEL_TIME,
	LOGLABEL_CONTENTS,
	LOGLABEL_ALL
} LOGLABEL_E;	



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;

static NFOBJECT *value[NUM_NIS_ROWS];
static NFOBJECT *btns[PIB_BUTTONS];

static WIZARD_USERDATA_T *g_wizard_data;
static IPSetupData ipdata;
static IPSetupData org_ipdata;


static gint prvIsIPChanged()
{
	if(ipdata.dhcp != org_ipdata.dhcp)	return 1;
	
	if(memcmp(ipdata.ip, org_ipdata.ip, sizeof(ipdata.ip)))		return 1;
	if(memcmp(ipdata.gateway, org_ipdata.gateway, sizeof(ipdata.gateway)))	return 1;
	if(memcmp(ipdata.subnet, org_ipdata.subnet, sizeof(ipdata.subnet)))		return 1;
	if(memcmp(ipdata.dns1, org_ipdata.dns1, sizeof(ipdata.dns1)))			return 1;
	if(memcmp(ipdata.dns2, org_ipdata.dns2, sizeof(ipdata.dns2)))			return 1;

	return 0;
}

static void _get_netinfo_ipdata(void)
{
	NF_NETIF_GET_INFO ret_net_info;

	DAL_get_ipSetup_data(&ipdata);
	scm_get_sys_netinfo(&ret_net_info);

	convertIntToIP(ipdata.ip, ret_net_info.ipaddr);
	convertIntToIP(ipdata.dns1, ret_net_info.dnsserver1);
	convertIntToIP(ipdata.dns2, ret_net_info.dnsserver2);
	convertIntToIP(ipdata.gateway, ret_net_info.gateway);
	convertIntToIP(ipdata.subnet, ret_net_info.netmask);
}


static gint _prvSetDataToObjects()
{
	nfui_nfipeditor_set_ip((NFIPEDITOR*)value[NIS_IP_ROW], 
			ipdata.ip[0], ipdata.ip[1], ipdata.ip[2], ipdata.ip[3]);
	nfui_nfipeditor_set_ip((NFIPEDITOR*)value[NIS_GATEWAY_ROW], 
			ipdata.gateway[0], ipdata.gateway[1], ipdata.gateway[2], ipdata.gateway[3]);
	nfui_nfipeditor_set_ip((NFIPEDITOR*)value[NIS_SUBNET_ROW], 
			ipdata.subnet[0], ipdata.subnet[1], ipdata.subnet[2], ipdata.subnet[3]);
	nfui_nfipeditor_set_ip((NFIPEDITOR*)value[NIS_1ST_DNS_ROW], 
			ipdata.dns1[0], ipdata.dns1[1], ipdata.dns1[2], ipdata.dns1[3]);
	nfui_nfipeditor_set_ip((NFIPEDITOR*)value[NIS_2ND_DNS_ROW], 
			ipdata.dns2[0], ipdata.dns2[1], ipdata.dns2[2], ipdata.dns2[3]);

	return 0;		
}

static gint _prvLoadDataFromObjects()
{
	ipdata.dhcp = nfui_radio_button_get_toggled((NFBUTTON*)value[NIS_DHCP_ROW]);
	
	nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_IP_ROW], ipdata.ip);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ipdata.gateway);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_SUBNET_ROW], ipdata.subnet);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_1ST_DNS_ROW], ipdata.dns1);
	nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_2ND_DNS_ROW], ipdata.dns2);

	return 0;
}

static gint _save_IpSetup_data()
{
	DAL_set_ipSetup_data(&ipdata);
	g_memmove(&g_wizard_data->networkData.ipsetup_data, &ipdata, sizeof(IPSetupData));
	
	return 0;
}

static gint _is_invalid_ip()
{
	if(org_ipdata.ip[0] == 0 || org_ipdata.ip[0] == 255 || ipdata.ip[0] == 0 || ipdata.ip[0] == 255) return 1;

	return 0;
}

static gint _is_invalid_renew_ip()
{
	if(ipdata.ip[0] == 0 || ipdata.ip[0] == 255) return 1;

	return 0;
}

static gint _renew_ipdata(guint msg_type)
{
	scm_apply_netinfo_by_db(msg_type);
	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");

	return 0;
}

static gint _prev_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;
}

static gint _next_step_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_next_step(1);
	
	return 0;
}

static gint _exit_proc()
{
    gint retVal = 0;

    if (ivsc.dfunc.support_protect)
    {
        if (g_wizard_data->systemData.agr_policy == 0) {
            retVal = vw_provide_devinfo_warning_open(g_curwnd, "PREVIOUS", "FINISH");
            if (retVal == 0) return -1;
        }
    }

	if(memcmp(&org_ipdata, &ipdata, sizeof(IPSetupData)))
		DAL_set_ipSetup_data(&org_ipdata);
	
	nfui_nfobject_destroy(g_curwnd);	
	_wizard_finish();
	
	return 0;
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

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;	

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);	
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}
static gboolean post_sub_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkPixbuf *pbuf = NULL;
	GdkGC *gc;
	gint gap_x, gap_y, size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);
	
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
	
		uxm_unreg_imsg_event(obj,IRET_SCM_DHCP_RENEW);
		uxm_unreg_imsg_event(obj,IRET_SCM_APPLY_NETINFO);

		gtk_main_quit();
	}
	else if (evt->type == IRET_SCM_DHCP_RENEW)
	{
		gint i;
		mb_type ret;
		
    	if (wait_pop) 
    	{
    		nftool_remove_waitbox(wait_pop);
    		wait_pop = NULL;
    	}

		_get_netinfo_ipdata();
		_prvSetDataToObjects();

		for(i=NIS_IP_ROW; i<NUM_NIS_ROWS; i++)
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		if((ipdata.dhcp) && (_is_invalid_renew_ip() == 1))
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "DHCP server failed to update. \nAre you sure you want to retry?", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				if(!_is_connected_cable())
				{
					ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
					if(ret == NFTOOL_MB_OK)	return FALSE;
				}
				_renew_ipdata(IRET_SCM_DHCP_RENEW);
			}
			else
			{
				if(ipdata.dhcp == org_ipdata.dhcp)
					return FALSE;
				else
				{
					g_memmove(&ipdata, &org_ipdata, sizeof(IPSetupData));
					_save_IpSetup_data();
					_renew_ipdata(IRET_SCM_DHCP_RENEW);
				}
			}
		}
		else
		{
			g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
			_save_IpSetup_data();
		}
   	}
	else if (evt->type == IRET_SCM_APPLY_NETINFO)
	{	
		gint i;
		mb_type ret;
		
		if (wait_pop) 
		{
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;
		}

		_get_netinfo_ipdata();
		_prvSetDataToObjects();

		for(i=NIS_IP_ROW; i<NUM_NIS_ROWS; i++)
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		if((ipdata.dhcp) && (_is_invalid_renew_ip() == 1))
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "DHCP server failed to update. \nAre you sure you want to retry?", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				if(!_is_connected_cable())
				{
					ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
					if(ret == NFTOOL_MB_OK)	return FALSE;
				}
				_renew_ipdata(IRET_SCM_APPLY_NETINFO);
			}			
			else
			{
				if(ipdata.dhcp == org_ipdata.dhcp)
					return FALSE;
				else
				{
					g_memmove(&ipdata, &org_ipdata, sizeof(IPSetupData));
					_save_IpSetup_data();
					_renew_ipdata(IRET_SCM_DHCP_RENEW);
				}
			}
		}
		else
		{
			g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
			_save_IpSetup_data();
			_next_step_proc();
		}

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

static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret = -1;
		gint is_changed;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		_prvLoadDataFromObjects();
		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK)	return FALSE;
		}

		is_changed = prvIsIPChanged();

		if (is_changed)
		{
			_save_IpSetup_data();		
			_renew_ipdata(IRET_SCM_APPLY_NETINFO);
		}
		else if (ipdata.dhcp && _is_invalid_ip())
		{
			_save_IpSetup_data();		
			_renew_ipdata(IRET_SCM_APPLY_NETINFO);
		}
		else
		{
			_next_step_proc();
		}
		
	}

	return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret = -1;
		gint is_changed;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		_prvLoadDataFromObjects();

		is_changed = prvIsIPChanged();

		if (is_changed)
		{
			_save_IpSetup_data();		
			_renew_ipdata(IRET_SCM_APPLY_NETINFO);
		}
		else if (ipdata.dhcp && _is_invalid_ip())
		{
			_save_IpSetup_data();		
			_renew_ipdata(IRET_SCM_APPLY_NETINFO);
		}
		else
		{
			_prev_proc();
		}
	}

	return FALSE;
}

static gboolean post_ipe_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0, };
	gint x, y;


	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
		
		nfui_nfobject_get_offset(obj, &x, &y);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);		

		if (result == 0) {
			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

			if(obj == value[NIS_IP_ROW]) {
				ip_editor_data.field[3] = 1;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ip_editor_data.field);

				ip_editor_data.field[0] = 255;
				ip_editor_data.field[1] = 255;
				ip_editor_data.field[2] = 255;
				ip_editor_data.field[3] = 0;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_SUBNET_ROW], ip_editor_data.field);
			}
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);

			nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
			result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);		

			if (result == 0)
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

			if(obj == value[NIS_IP_ROW]) {
				ip_editor_data.field[3] = 1;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ip_editor_data.field);

				ip_editor_data.field[0] = 255;
				ip_editor_data.field[1] = 255;
				ip_editor_data.field[2] = 255;
				ip_editor_data.field[3] = 0;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_SUBNET_ROW], ip_editor_data.field);
			}
		}
	}

	return FALSE;
}

static gboolean post_dhcp_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
		gint dhcp;
		gint i;

		dhcp = nfui_radio_button_get_toggled((NFBUTTON*)value[NIS_DHCP_ROW]);

		if(dhcp)
		{
			nfui_nfobject_disable(value[NIS_IP_ROW]);
			nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_enable(btns[PIB_RENEW]);

			nftool_mbox(g_curwnd, "NOTICE", "If you use DHCP when dynamic IP changed the camera will be disconnected.", NFTOOL_MB_OK);
		}
		else
		{
			nfui_nfobject_enable(value[NIS_IP_ROW]);
			nfui_nfobject_enable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_enable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_enable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_enable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_disable(btns[PIB_RENEW]); 		
		}
		
		for(i=NIS_IP_ROW; i<=NIS_2ND_DNS_ROW; i++) 
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);
		
		nfui_signal_emit(btns[PIB_RENEW], GDK_EXPOSE, TRUE);	
	}

	return FALSE;
}

static gboolean post_dhcp_renew_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;
	mb_type ret = -1;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(!_is_connected_cable())
		{
			ret = nftool_mbox(g_curwnd, "ERROR", "Please check the network cable.", NFTOOL_MB_OK);
			if(ret == NFTOOL_MB_OK) return FALSE;
		}

		_prvLoadDataFromObjects();
		_save_IpSetup_data();
		
		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
        scm_apply_netinfo_by_db(IRET_SCM_DHCP_RENEW);
	}

	return FALSE;
}

static gboolean post_helpbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		NFOBJECT *top;
		guint x, y;
        PARAGRAPH_STR *para;
        gint i;
		
  	   	if (evt->button.button == MOUSE_RIGTH_BUTTON)  return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x;
		y += top->y + obj->height + 2;

        para = imalloc(sizeof(PARAGRAPH_STR));

        para->intro[0] = g_strdup(HELP_STR);

		para->intro_type = BULLET_BLANK;
        para->intro_cnt = 1;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
	}

	return FALSE;
}


gint vw_wizard_network_ipsetup_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *fixed2;
	NFOBJECT *obj;
	NFOBJECT *help_obj;
	NFOBJECT *subject[NUM_NIS_ROWS];
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;

	const gchar *strButton[] = {"RENEW", "PREVIOUS", "NEXT", "FINISH"};
	const gchar *strTitle[] = {"IP ADDRESS", "GATEWAY", "SUBNET MASK", "1ST DNS SERVER", "2ND DNS SERVER"};

	guint pos_x, pos_y;
	gint size_w, size_h;
	guint i;
	char title[64];


    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

	
	memset(&ipdata, 0x00, sizeof(IPSetupData));
	memset(&org_ipdata, 0x00, sizeof(IPSetupData));

	g_memmove(&ipdata, &g_wizard_data->networkData.ipsetup_data, sizeof(IPSetupData));		
	g_memmove(&org_ipdata, &g_wizard_data->networkData.ipsetup_data, sizeof(IPSetupData));

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = 4;
	pos_y = 4;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "IP SETUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	
	obj = nftool_normal_button_create_type1("?", 42);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_helpbutton_event_handler);
	help_obj = obj;

	value[NIS_IP_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
							ipdata.ip[0], 
							ipdata.ip[1], 
							ipdata.ip[2], 
							ipdata.ip[3]);
	
	value[NIS_GATEWAY_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
							ipdata.gateway[0], 
							ipdata.gateway[1], 
							ipdata.gateway[2], 
							ipdata.gateway[3]);
	
	value[NIS_SUBNET_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
							ipdata.subnet[0], 
							ipdata.subnet[1],
							ipdata.subnet[2], 
							ipdata.subnet[3]);
	
	value[NIS_1ST_DNS_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
							ipdata.dns1[0], 
							ipdata.dns1[1], 
							ipdata.dns1[2], 
							ipdata.dns1[3]);

	value[NIS_2ND_DNS_ROW] = (NFOBJECT*)nfui_nfipeditor_new_with_ip(
							ipdata.dns2[0], 
							ipdata.dns2[1], 
							ipdata.dns2[2], 
							ipdata.dns2[3]);


	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x += (guint)28;
	pos_y += (guint)60;

	for(i=0; i<2; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);

		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_dhcp_onoff_event_cb);
		nfui_nfobject_show(obj);

		if(i == 0) {
			value[NIS_DHCP_ROW] = obj;
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			if(ipdata.dhcp)
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} else {
			pos_y += (guint)size_h + 20;
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);

			if(!ipdata.dhcp)
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x,pos_y);

		/* label */
		if(i == PIB_RENEW)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DHCP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));

			btns[i] = nftool_normal_button_create_popup_type1(strButton[i], 200);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[PIB_RENEW]), NFALIGN_CENTER, 0);	
			nfui_nfobject_show(btns[PIB_RENEW]);
			nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_RENEW], pos_x + SUBJECT_LABEL_WIDTH + 4,pos_y-4);
		}
		else
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATIC IP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		}
		nfui_nfobject_set_size(obj, 200, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + 50, pos_y);
	}

	if (!ipdata.dhcp) nfui_nfobject_disable(btns[PIB_RENEW]);

	pos_y += (guint)40;

	fixed2 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, 20+SUBJECT_LABEL_WIDTH*2, IPS_LABEL_HEIGHT*5+30);
	nfui_nffixed_put((NFFIXED*)fixed1, fixed2, pos_x, pos_y);
	nfui_nfobject_show(fixed2);

	pos_x = (guint)10;
	pos_y = (guint)10;

	for(i=1; i<NUM_NIS_ROWS; i++)
	{
		subject[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i-1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nflabel_set_align((NFLABEL*)subject[i], NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);

		nfui_nfobject_set_size(subject[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		
		nfui_nfobject_use_focus(subject[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(subject[i]);
		nfui_nffixed_put((NFFIXED*)fixed2, subject[i], pos_x, pos_y-3);


		nfui_nfipeditor_set_pango_font(value[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));

		nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));

		nfui_nfobject_set_size(value[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
		nfui_nffixed_put((NFFIXED*)fixed2, value[i], pos_x + SUBJECT_LABEL_WIDTH, pos_y);
		nfui_nfobject_show(value[i]);


		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
	}

	if(ipdata.dhcp)
	{
		nfui_nfobject_disable(value[NIS_IP_ROW]);
		nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
		nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
		nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
		nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
	}

	for( i=1; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	//nfui_nfobject_disable(btns[PIB_PREVIOUS]);
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);


	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(fixed2, post_sub_bg_event_handler);
	nfui_regi_post_event_callback(value[NIS_IP_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_GATEWAY_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_SUBNET_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_1ST_DNS_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_2ND_DNS_ROW], post_ipe_event_handler);
	
	nfui_regi_post_event_callback(btns[PIB_RENEW], post_dhcp_renew_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	
	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	scm_req_net_rxtx_status_data();

	uxm_reg_imsg_event(main_wnd, IRET_SCM_DHCP_RENEW);
	uxm_reg_imsg_event(main_wnd, IRET_SCM_APPLY_NETINFO);

	gtk_main();

	return 0;
}

