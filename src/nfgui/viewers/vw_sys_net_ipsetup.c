#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfipeditor.h"
#include "objects/nfbutton.h"

#include "tools/nf_ui_function.h"


#include "vw_sys_net_main.h"
#include "vw_sys_net_ipsetup.h"
#include "vw_vkeyboard.h"
#include "vw_ip_editor_popup.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(280)
#define	SUBJECT_LABEL_MARGIN		(0)

#define	VALUE_LABEL_WIDTH			(278)

#define	IPS_LABEL_COL_SPACE			(2)
#define	IPS_LABEL_ROW_SPACE			(2)
#define	IPS_LABEL_HEIGHT			(40)

#define IPS_PORT_LABEL_WIDTH      	(320)
//#define VALUE_PORT_LABEL_WIDTH    	(100)
#define VALUE_PORT_LABEL_WIDTH    	VALUE_LABEL_WIDTH
#define VALUE_PORT_LABEL_X        	(SUBJECT_LABEL_LEFT + IPS_PORT_LABEL_WIDTH + IPS_LABEL_COL_SPACE)
#define PORT_BTN_WIDTH            	(262) 
#define PORT_BTN_SPACE            	(2)
#define PORT_AUTO_BTN_X           	(VALUE_PORT_LABEL_X + VALUE_PORT_LABEL_WIDTH + IPS_LABEL_COL_SPACE)
#define PORT_REMOVE_BTN_X         	(PORT_AUTO_BTN_X + PORT_BTN_WIDTH + PORT_BTN_SPACE)
#define PORT_TEST_BTN_X           	(PORT_AUTO_BTN_X + ((PORT_BTN_WIDTH+PORT_BTN_SPACE)*2))
#define PORT_RENEW_BTN_X         	(VALUE_PORT_LABEL_X + VALUE_LABEL_WIDTH + PORT_BTN_SPACE)

//#define VALUE_PORT_STATUS_WIDTH		((PORT_REMOVE_BTN_X + PORT_BTN_WIDTH) - VALUE_PORT_LABEL_X)
#define VALUE_PORT_STATUS_WIDTH		((PORT_AUTO_BTN_X + PORT_BTN_WIDTH) - VALUE_PORT_LABEL_X)
#define PORT_CONFLICT_BTN_X			PORT_REMOVE_BTN_X

#define MAX_TX_SPEED_STRING     ("MAX TX SPEED")

enum {
	NISB_PORT = 0,  //PORT TEST
	NISB_PORT_CONFLICT,	
	NISB_RTSP_AUTO_PORT,
	NISB_RTSP_REMOVE_PORT,
	NISB_WEB_AUTO_PORT,
	NISB_WEB_REMOVE_PORT,
	NISB_DHCP_RENEW,	
	NISB_BUTTONS
};

enum {
	SPEED_256_KBYTE = 0,
	SPEED_512_KBYTE,
	SPEED_1_MBYTE,
	SPEED_2_MBYTE,
	SPEED_4_MBYTE,
	SPEED_8_MBYTE,
	SPEED_16_MBYTE,
	SPEED_32_MBYTE,
	SPEED_64_MBYTE,
	SPEED_MAX_MBYTE,

	NUM_SPEEDS,
};

typedef enum
{
  NIS_DHCP_ROW = 0,
  NIS_IP_ROW,
  NIS_GATEWAY_ROW,
  NIS_SUBNET_ROW,
  NIS_1ST_DNS_ROW,
  NIS_2ND_DNS_ROW,
  NIS_AUTO_PORT_ROW,
  NIS_AUTO_PORT_STATUS_ROW,  
  NIS_RTSP_PORT_ROW,
  NIS_WEB_PORT_ROW,
  NIS_MAX_TX_ROW,
  NUM_NIS_ROWS
}nis_row_type;

enum {
	REGISTER	= 0,
	REMOVE		= 1
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *value[NUM_NIS_ROWS];
static NFOBJECT *nis_btns[NISB_BUTTONS];

static IPSetupData ipdata;
static IPSetupData org_ipdata;

static guint status_timer_id = 0;
static gboolean g_install_mode = 0;
static gboolean g_use_dual_lan = 0;

static const gchar *strSpeed[NUM_SPEEDS] = {
	"256 Kbps", 
	"512 Kbps", 
	"1 Mbps",
	"2 Mbps", 
	"4 Mbps", 
	"8 Mbps",
	"16 Mbps", 
	"32 Mbps",
	"64 Mbps",
	"MAX"
};


static guint prvIndexToSpeed(gint index)
{
	guint ret = 0;

	switch (index) {
		case SPEED_256_KBYTE:	ret = 32;		break;
		case SPEED_512_KBYTE:	ret = 64;		break;
		case SPEED_1_MBYTE:		ret = 128;		break;
		case SPEED_2_MBYTE:		ret = 256;		break;
		case SPEED_4_MBYTE:		ret = 512;		break;
		case SPEED_8_MBYTE:		ret = 1024;		break;
		case SPEED_16_MBYTE:	ret = 2048;		break;
		case SPEED_32_MBYTE:	ret = 4096;		break;
		case SPEED_64_MBYTE:	ret = 8192;		break;
		case SPEED_MAX_MBYTE:	ret = 0;	    break;
		default:				ret = 0;	    break;
	}

	return ret;
}

static gint prvSpeedToIndex(guint speed)
{
	gint ret = 0;

    if (speed == 0)         ret = SPEED_MAX_MBYTE;
	else if(speed < 64)		ret = SPEED_256_KBYTE;
	else if(speed < 128)	ret = SPEED_512_KBYTE;
	else if(speed < 256)	ret = SPEED_1_MBYTE;
	else if(speed < 512)	ret = SPEED_2_MBYTE;
	else if(speed < 1024)	ret = SPEED_4_MBYTE;
	else if(speed < 2048)	ret = SPEED_8_MBYTE;
	else if(speed < 4096)	ret = SPEED_16_MBYTE;
	else if(speed < 8192)	ret = SPEED_32_MBYTE;
	else                   	ret = SPEED_64_MBYTE;

	return ret;
}

static gint prvIsIPChanged()
{
	if(ipdata.dhcp != org_ipdata.dhcp)	return 1;
	
	if(memcmp(ipdata.ip, org_ipdata.ip, sizeof(ipdata.ip)))					return 1;
	if(memcmp(ipdata.gateway, org_ipdata.gateway, sizeof(ipdata.gateway)))	return 1;
	if(memcmp(ipdata.subnet, org_ipdata.subnet, sizeof(ipdata.subnet)))		return 1;
	if(memcmp(ipdata.dns1, org_ipdata.dns1, sizeof(ipdata.dns1)))			return 1;
	if(memcmp(ipdata.dns2, org_ipdata.dns2, sizeof(ipdata.dns2)))			return 1;

	if(ipdata.webPort != org_ipdata.webPort)                             	return 1;
	if(ipdata.rtspport != org_ipdata.rtspport)                           	return 1;
	return 0;
}

static int _make_upnp_result_msg(int type, int upnp_ret, char *title, char *msg, int port_num)
{
	char *str_title[] = {
		"SUCCESS",
		"FAIL",
		"ERROR",
		"NOTICE"		
	};

	char *str_type[] = {
		"registered",
		"removed",
	};

	char *str_msg[] = {
		"The operation occured an error as a code %d.",
		"The port has been %s successfully.",
		"Unknown error.",
		"Unknown error.",
		"It can be failed to register as %d port have been used in other device.",
		"It is unable to remove the port number.",
		"Unknown error.",
		"NO DEVICE",
		"It is invalid port number.",
		"Port conflict, but has been successfully registered.",		
	};

	switch (upnp_ret) {
	case RES_UPNP_SUCCESS_FORCE:
			sprintf(title, "%s", str_title[3]);
			sprintf(msg, str_msg[9]);
		break;		
	case RES_UPNP_SUCCESS:
		sprintf(title, "%s", str_title[0]);
		sprintf(msg, lookup_string(str_msg[1]), str_type[type]);
		break;
	case RES_UPNP_EQUAL_PORT:
		sprintf(title, "%s", str_title[3]);
		sprintf(msg, lookup_string(str_msg[upnp_ret - 100]), port_num);	
		break;
	case RES_UPNP_IGD_NOT_PORT_FWD:
	case RES_UPNP_DEV_NOT_IGD:
	case RES_UPNP_PORT_DEL_FAIL:
	case RES_UPNP_NOT_IGD:
	case RES_UPNP_NOT_DEV:
	case RES_UPNP_PORT_DEL_ERROR:
		sprintf(title, "%s", str_title[1]);
		sprintf(msg, "%s", str_msg[upnp_ret - 100]);
		break;
	default:
		sprintf(title, "%s", str_title[2]);
		sprintf(msg, lookup_string(str_msg[0]), upnp_ret);
		break;
	}
	return 0;
}

static int _on_reg_rtspport(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char tmp_msg[256];
	char msg[256];
	int rtsp_port = 0;

	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	rtsp_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);

	_make_upnp_result_msg(REGISTER, ret, title, msg, rtsp_port);
	nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);   
	return 0;
}

static int _on_del_rtspport(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char msg[256];
	int rtsp_port = 0;

	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	rtsp_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);

	_make_upnp_result_msg(REMOVE, ret, title, msg, rtsp_port);
	nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);   
	return 0;
}

static int _on_reg_webport(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char msg[256];
	int web_port = 0;

	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	web_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);
	_make_upnp_result_msg(REGISTER, ret, title, msg, web_port);
	nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);   

	return 0;
}

static int _on_del_webport(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char msg[256];
	int web_port = 0;

	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	web_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);

	_make_upnp_result_msg(REMOVE, ret, title, msg, web_port);
	nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);   

	return 0;
}

static void current_set_ipSetup_data(void)
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

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	switch (evt->type) {
	case GDK_EXPOSE :
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, IRET_SCM_REG_RTSP);
		uxm_unreg_imsg_event(obj, IRET_SCM_RMV_RTSP);
		uxm_unreg_imsg_event(obj, IRET_SCM_REG_WEB);
		uxm_unreg_imsg_event(obj, IRET_SCM_RMV_WEB);
		if(status_timer_id)
		{
			g_source_remove(status_timer_id);
			status_timer_id = 0;
		}
		break;

	case IRET_SCM_REG_RTSP:
		_on_reg_rtspport(obj, evt, data);
		break;
	case IRET_SCM_RMV_RTSP:
		_on_del_rtspport(obj, evt, data);
		break;
	case IRET_SCM_TST_RTSP:
		break;
	case IRET_SCM_REG_WEB:
		_on_reg_webport(obj, evt, data);
		break;
	case IRET_SCM_RMV_WEB:
		_on_del_webport(obj, evt, data);
		break;
	case IRET_SCM_TST_WEB:
		break;

	default :
		break;
	}

	return FALSE;
}

static gboolean post_dhcp_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		guint dhcp;
		gint i;

		dhcp = nfui_check_button_get_active((NFCHECKBUTTON*)value[NIS_DHCP_ROW]);

		if(dhcp)
		{
			nfui_nfobject_disable(value[NIS_IP_ROW]);
			nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_enable(nis_btns[NISB_DHCP_RENEW]);

			nftool_mbox(g_curwnd, "NOTICE", "Incase setting is done as DHCP,\n"
											"the cameras could be reconnected if IP is changed\n"
											"while activation.\n"
											"For safe use, static IP is recommended.", NFTOOL_MB_OK);
		}
		else
		{
			nfui_nfobject_enable(value[NIS_IP_ROW]);
			nfui_nfobject_enable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_enable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_enable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_enable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_disable(nis_btns[NISB_DHCP_RENEW]);			
		}

		for(i=NIS_IP_ROW; i<=NIS_2ND_DNS_ROW; i++) 
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		nfui_signal_emit(nis_btns[NISB_DHCP_RENEW], GDK_EXPOSE, TRUE);			
	}

	return FALSE;
}

static gboolean _check_port_validity(NFOBJECT *obj, int port_num)
{
	int pnum_temp;
	RtpData rtpData;

	DAL_get_rtp_data(&rtpData);

	if (port_num >= 65536) {
		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65536.", NFTOOL_MB_OK);
		return FALSE;
	}

    if (port_num == rtpData.audio_backch_port)
    {
		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
		return FALSE;
    }
    
	if (obj == value[NIS_RTSP_PORT_ROW])
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[NIS_WEB_PORT_ROW]));
	else
		pnum_temp = nfui_nflabel_get_number((NFLABEL*)(value[NIS_RTSP_PORT_ROW]));

	if (pnum_temp == port_num) {
		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
		return FALSE;
	}


	if (obj == value[NIS_RTSP_PORT_ROW]) {
		if (port_num != 554 && port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be 554 or\nlarge than 1024.", 
					NFTOOL_MB_OK);
			return FALSE;
		}              
	}
	else {
		if (port_num != 80 && port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be 80 or\nlarge than 1024.", 
					NFTOOL_MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

		if (_check_port_validity(obj, numRet)) 
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_ipe_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0,};
	gint x, y;
	NF_NETIF_GET_INFO ret_net_info;
	guint lan2_ip[4];

	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
		
		nfui_nfobject_get_offset(obj, &x, &y);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);		

		if (result == 0) 
		{
			if(obj == value[NIS_IP_ROW]) 
			{
				if (g_install_mode && g_use_dual_lan)
				{
					memset(&ret_net_info, 0, sizeof(ret_net_info));
					scm_get_sys_netinfo(&ret_net_info);
					convertIntToIP(lan2_ip, ret_net_info.lan2_ipaddr);
					
					if (lan2_ip[0] == ip_editor_data.field[0] && lan2_ip[1] == ip_editor_data.field[1] && lan2_ip[2] == ip_editor_data.field[2])
					{
						nftool_mbox(g_curwnd, "ERROR", "The IP bands of Lan1 and Lan2 are the same. Please set it up again.\n\nex) Lan1 : 192.168.0.100, Lan2 : 192.168.0.101 (X)\n       Lan1 : 192.168.0.100, Lan2 : 192.168.10.101 (O)", NFTOOL_MB_OK);
						return FALSE;
					}
				}
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

				ip_editor_data.field[3] = 1;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ip_editor_data.field);

				ip_editor_data.field[0] = 255;
				ip_editor_data.field[1] = 255;
				ip_editor_data.field[2] = 255;
				ip_editor_data.field[3] = 0;
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_SUBNET_ROW], ip_editor_data.field);
			}
			else
			{
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
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
			{
				if(obj == value[NIS_IP_ROW]) 
				{
					if (g_install_mode && g_use_dual_lan)
					{
						memset(&ret_net_info, 0, sizeof(ret_net_info));
						scm_get_sys_netinfo(&ret_net_info);
						convertIntToIP(lan2_ip, ret_net_info.lan2_ipaddr);
						
						if (lan2_ip[0] == ip_editor_data.field[0] && lan2_ip[1] == ip_editor_data.field[1] && lan2_ip[2] == ip_editor_data.field[2])
						{
							nftool_mbox(g_curwnd, "ERROR", "The IP bands of Lan1 and Lan2 are the same. Please set it up again.\n\n"
															"ex)\tLan1 : 192.168.10.100, Lan2 : 192.168.0.101 (O)\n\tLan1 : 192.168.0.100, Lan2 : 192.168.0.101 (X)", NFTOOL_MB_OK);
							return FALSE;
						}
					}
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);

					ip_editor_data.field[3] = 1;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ip_editor_data.field);

					ip_editor_data.field[0] = 255;
					ip_editor_data.field[1] = 255;
					ip_editor_data.field[2] = 255;
					ip_editor_data.field[3] = 0;
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_SUBNET_ROW], ip_editor_data.field);
				}
				else
				{
					nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_porttest_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	// not support
	//
	return FALSE;
}

static gboolean post_port_conflict_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nf_upnp_reset_conflict();
	}

	return FALSE;
}

static gboolean post_rtsp_autoport_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int rtsp_port = 0;

	if (evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		rtsp_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
		scm_register_rtsp_port(rtsp_port, IRET_SCM_REG_RTSP);
	}

	return FALSE;
}

static gboolean post_rtsp_removeport_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int rtsp_port = 0;

	if (evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		rtsp_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
		scm_remove_rtsp_port(rtsp_port, IRET_SCM_RMV_RTSP);
	}

	return FALSE;
}

static gboolean post_web_autoport_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int web_port = 0;

	if (evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		web_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);
		scm_register_web_port(web_port, IRET_SCM_REG_WEB);
	}

	return FALSE;
}

static gboolean post_web_removeport_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int web_port = 0;

	if (evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		web_port = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);
		scm_remove_web_port(web_port, IRET_SCM_RMV_WEB);
	}

	return FALSE;
}

static gboolean post_dhcp_renew_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_message("%s, %d", __FUNCTION__, __LINE__);

		ipdata.dhcp = nfui_check_button_get_active((NFCHECKBUTTON*)value[NIS_DHCP_ROW]);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_IP_ROW], ipdata.ip);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ipdata.gateway);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_SUBNET_ROW], ipdata.subnet);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_1ST_DNS_ROW], ipdata.dns1);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_2ND_DNS_ROW], ipdata.dns2);

		ipdata.autoPort = nfui_spin_button_get_index((NFSPINBUTTON*)value[NIS_AUTO_PORT_ROW]);
		ipdata.rtspport = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
		ipdata.webPort = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);

		ipdata.txSpeed = (guint)prvIndexToSpeed(nfui_spin_button_get_index((NFSPINBUTTON*)value[NIS_MAX_TX_ROW]));

		if(memcmp(&org_ipdata, &ipdata, sizeof(IPSetupData)))
        {
    		DAL_set_ipSetup_data(&ipdata);
			g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
        }

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
        scm_apply_netinfo_by_db(IRET_SCM_DHCP_RENEW);

        g_message("%s, %d", __FUNCTION__, __LINE__);
	}
	else if (evt->type == IRET_SCM_DHCP_RENEW)
	{
        g_message("%s, %d", __FUNCTION__, __LINE__);

    	if (wait_pop) 
    	{
    		nftool_remove_waitbox(wait_pop);
    		wait_pop = NULL;
    	}

   		DAL_get_ipSetup_data(&ipdata);
		current_set_ipSetup_data();

		g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));

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

		for(i=NIS_IP_ROW; i<=NIS_2ND_DNS_ROW; i++)
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		sysnet_set_changeflag(1);
		
        g_message("%s, %d", __FUNCTION__, __LINE__);		
	}
    else if (evt->type == GDK_DELETE)
    {
		uxm_unreg_imsg_event(obj, IRET_SCM_DHCP_RENEW);
    }

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_memmove(&ipdata, &org_ipdata, sizeof(IPSetupData));

		nfui_check_button_set_active((NFCHECKBUTTON*)value[NIS_DHCP_ROW], (gboolean)ipdata.dhcp);

		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_IP_ROW], ipdata.ip);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ipdata.gateway);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_SUBNET_ROW], ipdata.subnet);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_1ST_DNS_ROW], ipdata.dns1);
		nfui_nfipeditor_set_ip_array((NFIPEDITOR*)value[NIS_2ND_DNS_ROW], ipdata.dns2);

		nfui_spin_button_set_index((NFSPINBUTTON*)value[NIS_AUTO_PORT_ROW], ipdata.autoPort);
		nfui_nflabel_set_number((NFLABEL*)value[NIS_RTSP_PORT_ROW], (gint)ipdata.rtspport);
		nfui_nflabel_set_number((NFLABEL*)value[NIS_WEB_PORT_ROW], (gint)ipdata.webPort);

		nfui_spin_button_set_index((NFSPINBUTTON*)value[NIS_MAX_TX_ROW], (guint)prvSpeedToIndex(ipdata.txSpeed));
			
		if(ipdata.dhcp)
		{
			nfui_nfobject_disable(value[NIS_IP_ROW]);
			nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_enable(nis_btns[NISB_DHCP_RENEW]);			
		}
		else
		{
			nfui_nfobject_enable(value[NIS_IP_ROW]);
			nfui_nfobject_enable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_enable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_enable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_enable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_disable(nis_btns[NISB_DHCP_RENEW]);
		}

		for(i=0; i<NUM_NIS_ROWS; i++)
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		nfui_signal_emit(nis_btns[NISB_DHCP_RENEW], GDK_EXPOSE, TRUE);			
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	switch (evt->type) {
	case GDK_BUTTON_RELEASE:
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		ipdata.dhcp = nfui_check_button_get_active((NFCHECKBUTTON*)value[NIS_DHCP_ROW]);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_IP_ROW], ipdata.ip);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_GATEWAY_ROW], ipdata.gateway);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_SUBNET_ROW], ipdata.subnet);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_1ST_DNS_ROW], ipdata.dns1);
		nfui_nfipeditor_get_ip((NFIPEDITOR*)value[NIS_2ND_DNS_ROW], ipdata.dns2);

		ipdata.autoPort = nfui_spin_button_get_index((NFSPINBUTTON*)value[NIS_AUTO_PORT_ROW]);
		ipdata.rtspport = nfui_nflabel_get_number((NFLABEL*)value[NIS_RTSP_PORT_ROW]);
		ipdata.webPort = nfui_nflabel_get_number((NFLABEL*)value[NIS_WEB_PORT_ROW]);

		ipdata.txSpeed = (guint)prvIndexToSpeed(nfui_spin_button_get_index((NFSPINBUTTON*)value[NIS_MAX_TX_ROW]));

		if(memcmp(&org_ipdata, &ipdata, sizeof(IPSetupData)))
		{
			gint is_changed = 0;

			is_changed = prvIsIPChanged();
			DAL_set_ipSetup_data(&ipdata);

			if (is_changed) {
				wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
				DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
				scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
			}else {
				nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
				sysnet_set_changeflag(1);
			}

			g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
		}
	}
	break;

	case IRET_SCM_APPLY_NETINFO:
	case IRET_SCM_APPLY_NETINFO2:
    	{
		if (wait_pop) {
			nftool_remove_waitbox(wait_pop);
			wait_pop = NULL;

		}

		DAL_get_ipSetup_data(&ipdata);
		current_set_ipSetup_data();

		if (ipdata.gateway[0] == 0 && ipdata.gateway[1] == 0 && ipdata.gateway[2] == 0 && ipdata.gateway[3] == 0)
			nftool_mbox(g_curwnd, "NOTICE", "Setting is invalid.\nPlease resetting.", NFTOOL_MB_OK);   

		g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));

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

		for(i=NIS_IP_ROW; i<=NIS_2ND_DNS_ROW; i++)
			nfui_signal_emit(value[i], GDK_EXPOSE, TRUE);

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		
		if (evt->type == IRET_SCM_APPLY_NETINFO2) gtk_main_quit();
        }   
		break;

		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
			uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);
        }
		break;
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

		IpSetup_tab_out_handler();

		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}


static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

static int _make_auto_upnp_msg(NF_UPNP_INFO upnp_ret, char *msg)
{
	switch (upnp_ret) {
		case UPNP_INIT:
			sprintf(msg, "Initializing");
		break;			
    	case UPNP_GOOD:
    		sprintf(msg, "Success");
		break;
    	case UPNP_ERR_NET:
    		sprintf(msg, "Not connected to the network.");
		break;
    	case UPNP_ERR_IGD:
    		sprintf(msg, "The router does not support UPNP.");
		break;
    	case UPNP_ERR_UPDATE:
    		sprintf(msg, "Failed to update port");
		break;
    	case UPNP_STOP:
    		sprintf(msg, "Stopped");
		break;		
    	default:
    		sprintf(msg, lookup_string("The operation occured an error as a code %d."), upnp_ret);
		break;
	}
	return 0;
}

static gboolean _upnp_status_update(gpointer data)
{
	char msg[512];
	NF_UPNP_INFO upnp_ret;
	NFOBJECT *status;

	status = (NFOBJECT *)data;

	if(status)
	{
		if( upnp_is_conflict())
		{
			if( nfui_nfobject_is_disabled(nis_btns[NISB_PORT_CONFLICT]) )
			{
				nfui_nfobject_enable(nis_btns[NISB_PORT_CONFLICT]);
				nfui_signal_emit(nis_btns[NISB_PORT_CONFLICT], GDK_EXPOSE, TRUE);
			}
			
			sprintf(msg, "Port conflict");
		}
		else
		{
			if( !nfui_nfobject_is_disabled(nis_btns[NISB_PORT_CONFLICT]) )
			{
				nfui_nfobject_disable(nis_btns[NISB_PORT_CONFLICT]);
				nfui_signal_emit(nis_btns[NISB_PORT_CONFLICT], GDK_EXPOSE, TRUE);
			}
			
			upnp_ret = nf_upnp_get_status();		
			_make_auto_upnp_msg( upnp_ret, msg);
		}
		
		nfui_nflabel_set_text((NFLABEL*)status, msg);
		nfui_signal_emit(status, GDK_EXPOSE, FALSE);
	}
	
	return TRUE;
}

void init_NetIPSetup_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixedTemp;
	NFOBJECT *subject[NUM_NIS_ROWS];
	NFOBJECT *obj;

	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strButton[] = {"PORT TEST", "CONFIRM CONFLICT",
		"AUTO PORT FORWARDING", "REMOVE PORT", "AUTO PORT FORWARDING", "REMOVE PORT", "RENEW"};

#ifdef _UPNP_SUPPORT_
	const gchar *strTitle[] = {
		"DHCP", /*"WEB SERVICE",*/
		"IP ADDRESS", "GATEWAY", "SUBNET MASK",
		"1ST DNS SERVER", "2ND DNS SERVER",  
		"AUTO PORT UPDATE", "AUTO PORT STATUS",
		"RTSP SERVICE PORT", "WEB SERVICE PORT",	
		/*"ALIAS",*/ 
		MAX_TX_SPEED_STRING, "MAX RX SPEED"
	};
#else
	const gchar *strTitle[] = {
		"DHCP", /*"WEB SERVICE",*/
		"IP ADDRESS", "GATEWAY", "SUBNET MASK",
		"1ST DNS SERVER", "2ND DNS SERVER",
		"AUTO PORT UPDATE", "AUTO PORT STATUS",
		"NET CLIENT PORT", "WEB SERVICE PORT",
		MAX_TX_SPEED_STRING, "MAX RX SPEED"
	};
#endif
	guint btn_x, btn_y, btn_space;
	guint pos_x, pos_y;
	guint chk_w, chk_h;
	gint renew_btn_y, rtsp_btn_y, web_btn_y;
	gint conflict_btn_y;	
	guint i;

	g_install_mode = DAL_get_cam_install_mode();
	g_use_dual_lan = DAL_get_cam_install_use_dual_lan();

	g_curwnd = nfui_nfobject_get_top(parent);


// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	pos_x = VALUE_PORT_LABEL_X;
	pos_y = SUBJECT_LABEL_TOP;

	memset(&ipdata, 0x00, sizeof(IPSetupData));
	memset(&org_ipdata, 0x00, sizeof(IPSetupData));

	current_set_ipSetup_data();

	value[NIS_DHCP_ROW] = nfui_checkbutton_new((gboolean)ipdata.dhcp);
	
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

	value[NIS_AUTO_PORT_ROW] = nfui_spinbutton_new(strOffOn, 2, ipdata.autoPort);
	value[NIS_AUTO_PORT_STATUS_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	value[NIS_RTSP_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	value[NIS_WEB_PORT_ROW] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	value[NIS_MAX_TX_ROW] = nfui_spinbutton_new(strSpeed, NUM_SPEEDS, prvSpeedToIndex(ipdata.txSpeed));

	for(i=0; i<NUM_NIS_ROWS; i++)
	{
		{
			subject[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
			nfui_nflabel_set_align((NFLABEL*)subject[i], NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);

			if((i==NIS_WEB_PORT_ROW) || (i==NIS_RTSP_PORT_ROW))
				nfui_nfobject_set_size(subject[i], IPS_PORT_LABEL_WIDTH, IPS_LABEL_HEIGHT);		
			else
				nfui_nfobject_set_size(subject[i], SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
			
			nfui_nfobject_modify_bg(subject[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_use_focus(subject[i], NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(subject[i]);
			nfui_nffixed_put((NFFIXED*)content_fixed, subject[i], SUBJECT_LABEL_LEFT, pos_y);
		}

		if (i == NIS_DHCP_ROW)
		{
			nfui_check_button_set_skin_type(NF_CHECKBUTTON(value[i]), NFCHECK_TYPE_NORMAL);
			nfui_check_get_size(value[i], &chk_w, &chk_h);
			nfui_nfobject_show(value[i]);

			fixedTemp = nfui_nffixed_new();

			nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
			nfui_nfobject_set_size(fixedTemp, VALUE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
			nfui_nfobject_show(fixedTemp);
			nfui_nffixed_put((NFFIXED*)fixedTemp, value[i], (VALUE_LABEL_WIDTH-chk_w)/2, (IPS_LABEL_HEIGHT-chk_h)/2);

			nfui_nffixed_put((NFFIXED*)content_fixed, fixedTemp, pos_x, pos_y);

			renew_btn_y = pos_y;
		}
		else
		{
			if(i<=NIS_2ND_DNS_ROW)
			{
				nfui_nfipeditor_set_pango_font(value[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
			}
			else if(i==NIS_RTSP_PORT_ROW)
			{			
				nfui_nflabel_use_number(value[i], TRUE);
				nfui_nflabel_set_number(value[i], ipdata.rtspport);
				rtsp_btn_y = pos_y;
			}
			
			else if(i==NIS_WEB_PORT_ROW)
			{
				nfui_nflabel_use_number(value[i], TRUE);
				nfui_nflabel_set_number(value[i], ipdata.webPort);
				web_btn_y = pos_y;
			}			
			else if(i == NIS_AUTO_PORT_ROW || i==NIS_MAX_TX_ROW)
			{
				nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value[i], NFSPINBUTTON_TYPE_1);
			}

			if(i >= NIS_IP_ROW && i <= NIS_2ND_DNS_ROW)
				nfui_nfobject_modify_bg(value[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));


			if((i==NIS_WEB_PORT_ROW)||(i==NIS_RTSP_PORT_ROW))
			{
				nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_INPUT);
				nfui_nfobject_set_size(value[i], VALUE_PORT_LABEL_WIDTH, IPS_LABEL_HEIGHT);  		
				nfui_nffixed_put((NFFIXED*)content_fixed, value[i], VALUE_PORT_LABEL_X, pos_y);
			}
			else if(i == NIS_AUTO_PORT_STATUS_ROW)
			{
				nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_OUTPUT);
//				nfui_nflabel_set_spacing((NFLABEL *)value[i], SEMI_CONDENSED_SPACING);
				nfui_nflabel_set_align((NFLABEL*)value[i], NFALIGN_CENTER, 0);
				nfui_nfobject_use_focus(value[i], FALSE);
				nfui_nfobject_set_size(value[i], VALUE_PORT_STATUS_WIDTH, IPS_LABEL_HEIGHT);
				nfui_nffixed_put((NFFIXED*)content_fixed, value[i], VALUE_PORT_LABEL_X, pos_y);

				conflict_btn_y = pos_y;
			}			
			else
			{
				nfui_nfobject_set_size(value[i], VALUE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
				nfui_nffixed_put((NFFIXED*)content_fixed, value[i], pos_x, pos_y);
			}
			nfui_nfobject_show(value[i]);
		}

		pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);

		if ((i == NIS_DHCP_ROW) || (i == NIS_2ND_DNS_ROW) || (i == NIS_WEB_PORT_ROW))
			pos_y += (IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE);
	}

	btn_x = 966;
	btn_y = 943;
	btn_space = 196;

	for(i=0; i<NISB_BUTTONS; i++)
	{
			nis_btns[i] = nftool_normal_button_create_type3(strButton[i], PORT_BTN_WIDTH);	
		nfui_nfbutton_set_spacing((NFBUTTON *)nis_btns[i], EXTRA_CONDENSED_SPACING);

		if(i != 0) nfui_nfobject_show(nis_btns[i]);

		if(i == NISB_PORT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_TEST_BTN_X, rtsp_btn_y);
		else if(i == NISB_RTSP_AUTO_PORT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_AUTO_BTN_X, rtsp_btn_y);
		else if(i == NISB_RTSP_REMOVE_PORT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_REMOVE_BTN_X, rtsp_btn_y);
		else if(i == NISB_WEB_AUTO_PORT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_AUTO_BTN_X, web_btn_y);
		else if(i == NISB_WEB_REMOVE_PORT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_REMOVE_BTN_X, web_btn_y);
		else if(i == NISB_DHCP_RENEW)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_RENEW_BTN_X, renew_btn_y);
		else if(i == NISB_PORT_CONFLICT)
			nfui_nffixed_put((NFFIXED*)content_fixed, nis_btns[i], PORT_CONFLICT_BTN_X, conflict_btn_y);		
		else
		{
			nfui_nffixed_put((NFFIXED*)parent, nis_btns[i], btn_x, btn_y);
			btn_x += btn_space;
		}
	}

    if (!ipdata.dhcp) nfui_nfobject_disable(nis_btns[NISB_DHCP_RENEW]);
	uxm_reg_imsg_event(nis_btns[NISB_DHCP_RENEW], IRET_SCM_DHCP_RENEW);

	if( upnp_is_conflict())
	{
		nfui_nfobject_enable(nis_btns[NISB_PORT_CONFLICT]);
		nfui_nflabel_set_text((NFLABEL*)value[NIS_AUTO_PORT_STATUS_ROW], "Port conflict");
	}
	else
	{
		char msg[512];
		NF_UPNP_INFO upnp_ret;
		
		nfui_nfobject_disable(nis_btns[NISB_PORT_CONFLICT]);
		upnp_ret = nf_upnp_get_status();
		_make_auto_upnp_msg( upnp_ret, msg);
		nfui_nflabel_set_text((NFLABEL*)value[NIS_AUTO_PORT_STATUS_ROW], msg);
	}

	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	uxm_reg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
	uxm_reg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	if(ipdata.dhcp)
	{
		nfui_nfobject_disable(value[NIS_IP_ROW]);
		nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
		nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
		nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
		nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
	}

	status_timer_id = g_timeout_add(3000, _upnp_status_update, value[NIS_AUTO_PORT_STATUS_ROW]);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);
	nfui_regi_post_event_callback(value[NIS_IP_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_GATEWAY_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_SUBNET_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_1ST_DNS_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_2ND_DNS_ROW], post_ipe_event_handler);
	nfui_regi_post_event_callback(value[NIS_DHCP_ROW], post_dhcp_chk_event_handler);

	nfui_regi_post_event_callback(value[NIS_RTSP_PORT_ROW], post_port_event_handler);

	nfui_regi_post_event_callback(value[NIS_WEB_PORT_ROW], post_port_event_handler);	
	nfui_regi_post_event_callback(nis_btns[NISB_PORT], post_porttest_event_handler);
	nfui_regi_post_event_callback(nis_btns[NISB_PORT_CONFLICT], post_port_conflict_event_handler);	

	nfui_regi_post_event_callback(nis_btns[NISB_RTSP_AUTO_PORT], post_rtsp_autoport_event_handler);
	nfui_regi_post_event_callback(nis_btns[NISB_RTSP_REMOVE_PORT], post_rtsp_removeport_event_handler);
	nfui_regi_post_event_callback(nis_btns[NISB_WEB_AUTO_PORT], post_web_autoport_event_handler);
	nfui_regi_post_event_callback(nis_btns[NISB_WEB_REMOVE_PORT], post_web_removeport_event_handler);
	nfui_regi_post_event_callback(nis_btns[NISB_DHCP_RENEW], post_dhcp_renew_event_handler);

	g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));

	uxm_reg_imsg_event(content_fixed, IRET_SCM_REG_RTSP);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_RMV_RTSP);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_REG_WEB);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_RMV_WEB);
}

gboolean IpSetup_tab_out_handler()
{
	mb_type ret;
	gint is_changed;
	gint i;

	ipdata.dhcp = nfui_check_button_get_active(value[NIS_DHCP_ROW]);

	nfui_nfipeditor_get_ip(value[NIS_IP_ROW], ipdata.ip);
	nfui_nfipeditor_get_ip(value[NIS_GATEWAY_ROW], ipdata.gateway);
	nfui_nfipeditor_get_ip(value[NIS_SUBNET_ROW], ipdata.subnet);
	nfui_nfipeditor_get_ip(value[NIS_1ST_DNS_ROW], ipdata.dns1);
	nfui_nfipeditor_get_ip(value[NIS_2ND_DNS_ROW], ipdata.dns2);

	ipdata.autoPort = nfui_spin_button_get_index((NFSPINBUTTON*)value[NIS_AUTO_PORT_ROW]);
	ipdata.rtspport= nfui_nflabel_get_number(value[NIS_RTSP_PORT_ROW]);
	ipdata.webPort = nfui_nflabel_get_number(value[NIS_WEB_PORT_ROW]);
	ipdata.txSpeed = prvIndexToSpeed(nfui_spin_button_get_index(value[NIS_MAX_TX_ROW]));

	if(!memcmp(&org_ipdata, &ipdata, sizeof(IPSetupData)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
			NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		is_changed = prvIsIPChanged();

		DAL_set_ipSetup_data(&ipdata);

		if(is_changed)
		{
			wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
			scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO2);

			gtk_main();
		}

		g_memmove(&org_ipdata, &ipdata, sizeof(IPSetupData));
		sysnet_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&ipdata, &org_ipdata, sizeof(IPSetupData));

		nfui_check_button_set_active_no_expose(value[NIS_DHCP_ROW], ipdata.dhcp);

		nfui_nfipeditor_set_ip_array_no_expose(value[NIS_IP_ROW], ipdata.ip);
		nfui_nfipeditor_set_ip_array_no_expose(value[NIS_GATEWAY_ROW], ipdata.gateway);
		nfui_nfipeditor_set_ip_array_no_expose(value[NIS_SUBNET_ROW], ipdata.subnet);
		nfui_nfipeditor_set_ip_array_no_expose(value[NIS_1ST_DNS_ROW], ipdata.dns1);
		nfui_nfipeditor_set_ip_array_no_expose(value[NIS_2ND_DNS_ROW], ipdata.dns2);

		nfui_spin_button_set_index((NFSPINBUTTON*)value[NIS_AUTO_PORT_ROW], ipdata.autoPort);
		nfui_nflabel_set_number(value[NIS_RTSP_PORT_ROW], ipdata.rtspport);
		nfui_nflabel_set_number(value[NIS_WEB_PORT_ROW], ipdata.webPort);

		nfui_spin_button_set_index_no_expose(value[NIS_MAX_TX_ROW], prvSpeedToIndex(ipdata.txSpeed));

		if(ipdata.dhcp)
		{
			nfui_nfobject_disable(value[NIS_IP_ROW]);
			nfui_nfobject_disable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_disable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_disable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_disable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_enable(nis_btns[NISB_DHCP_RENEW]);
		}
		else
		{
			nfui_nfobject_enable(value[NIS_IP_ROW]);
			nfui_nfobject_enable(value[NIS_GATEWAY_ROW]);
			nfui_nfobject_enable(value[NIS_SUBNET_ROW]);
			nfui_nfobject_enable(value[NIS_1ST_DNS_ROW]);
			nfui_nfobject_enable(value[NIS_2ND_DNS_ROW]);
			nfui_nfobject_disable(nis_btns[NISB_DHCP_RENEW]);			
		}
	}

	return FALSE;

}
