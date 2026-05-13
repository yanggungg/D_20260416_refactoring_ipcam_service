
#include "iux_msg.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "services/scm.h"

#include "modules/ssm.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "vw_vkeyboard.h"
#include "vw_change_account_popup.h"
#include "vw_change_ipsetup_popup.h"
#include "vw_sys_camera_ipcam_setup_popup.h"
#include "nf_api_ipcam.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif

#include "vsm.h"
#include "uxm.h"
#include "vw_text_box_open.h"

#define MAX_ID_STRING_SIZE		31

#define DETAIL_WIN_W			(1275)
#define DETAIL_WIN_H			(719)
#define DETAIL_WIN_X			(guint)((DISPLAY_ACTIVE_WIDTH - DETAIL_WIN_W)/2)
#define DETAIL_WIN_Y			(guint)((DISPLAY_ACTIVE_HEIGHT - DETAIL_WIN_H)/2)

#define DETAIL_CONFIG_FIXED_X	(20)
#define DETAIL_CONFIG_FIXED_Y	(63)
#define DETAIL_CONFIG_FIXED_W	(685)
#define DETAIL_CONFIG_FIXED_H	(362)

#define DETAIL_DIAG_FIXED_X		(DETAIL_CONFIG_FIXED_X)
#define DETAIL_DIAG_FIXED_Y		(DETAIL_CONFIG_FIXED_Y + DETAIL_CONFIG_FIXED_H + 15)
#define DETAIL_DIAG_FIXED_W		(DETAIL_CONFIG_FIXED_W)
#define DETAIL_DIAG_FIXED_H		(170)

#define DETAIL_PREVIEW_FIXED_X	(DETAIL_INFO_FIXED_X)
#define DETAIL_PREVIEW_FIXED_Y	(DETAIL_CONFIG_FIXED_Y)
#define DETAIL_PREVIEW_FIXED_W	(DETAIL_INFO_FIXED_W)
#define DETAIL_PREVIEW_FIXED_H	(432)

#define DETAIL_INFO_FIXED_X		(DETAIL_CONFIG_FIXED_X + DETAIL_CONFIG_FIXED_W + 20)
#define DETAIL_INFO_FIXED_Y		(DETAIL_PREVIEW_FIXED_Y + DETAIL_PREVIEW_FIXED_H)
#define DETAIL_INFO_FIXED_W		(502)
#define DETAIL_INFO_FIXED_H		(170)

enum {
	PP_CONFIG_CAM_NAME = 0,
	PP_CONFIG_IP_ADDR,
	PP_CONFIG_HTTP,
	PP_CONFIG_RTSP,
	PP_CONFIG_USER_NAME,
	PP_CONFIG_PASS,

	PP_CONFIG_ROWS
};

enum {
	PP_INFO_MAC = 0,
	PP_INFO_FW_VER,

	PP_INFO_ROWS
};

enum {
	PP_PREVIEW_CONNECT_TEST = 0,

	PP_PREVIEW_ROWS
};

enum {
	PP_BTN_IP_SETUP,
	PP_BTN_PORT_SETUP,
	PP_BTN_CHAN_PW,
	PP_BTN_LOGIN,
	PP_BTN_RESET,
	PP_BTN_PING,
	PP_BTN_TEST,

	PP_BTN_MAX
};

enum {
	STATE_CONNECT_FAIL = 0,	
	STATE_LOGIN_FAIL = 1,
	STATE_LOGIN_SUCCESS_ITX = 2,
	STATE_LOGIN_SUCCESS_ONVIF = 3,	
	STATE_NEED_PW_CHANGE = 4,		
	STATE_PW_CHANGE_FAIL = 5,
	STATE_PW_CHANGE_SUCCESS = 6,
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_config_value[PP_CONFIG_ROWS];
static NFOBJECT *g_camera_info[PP_INFO_ROWS];
static NFOBJECT *g_btns[PP_BTN_MAX];
static NFOBJECT *wait_mbox = NULL;
static NFOBJECT *g_preview_fixed = NULL;	


static NFOpenmodeCamInfo *g_org_info;
static NFOpenmodeCamInfo g_pre_info;
static NFOpenmodeCamInfo g_post_info;


static void _convertStrToIP(guint *out, gchar *str)
{
	char *ptr;
	char temp[16];
	int i;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

	memset(temp, 0x00, sizeof(temp));
	strcpy(temp, str);

	ptr = strtok_r(temp, ".", &pnext);
	pbuf = pnext;
	out[0] = atoi(ptr);

	i = 1;
	while(ptr = strtok_r(pbuf, ".", &pnext))
	{
		out[i] = atoi(ptr);
		i++;
		pbuf = pnext;
	}

}

static void _convertIPToStr(gchar *str, guint *ip)
{
	g_sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static void _convertInfoToIpset(IP_SET_T *ip_set)
{
	ip_set->dhcp = g_post_info.is_dhcp;
	_convertStrToIP(ip_set->ip, g_post_info.hostname); 
	convertIntToIP(ip_set->gateway, g_post_info.gw);
	convertIntToIP(ip_set->subnet, g_post_info.mask);
	convertIntToIP(ip_set->dns1, g_post_info.dns1);
	convertIntToIP(ip_set->dns2, g_post_info.dns2);
	ip_set->rtspPort = g_post_info.rtsp_port;
	ip_set->webPort = g_post_info.http_port;
}

static void _convertIpsetToInfo(IP_SET_T ip_set)
{
	_convertIPToStr(g_post_info.hostname, ip_set.ip);
	g_post_info.ipaddr = convertIPToInt(ip_set.ip);
	g_post_info.gw = convertIPToInt(ip_set.gateway);
	g_post_info.mask = convertIPToInt(ip_set.subnet);
	g_post_info.dns1 = convertIPToInt(ip_set.dns1);
	g_post_info.dns2 = convertIPToInt(ip_set.dns2);
	g_post_info.rtsp_port = ip_set.rtspPort;
	g_post_info.http_port = ip_set.webPort;
}

static void _convertIpsetToNetwork(IP_SET_T ip_set, NFOpenmodeSetupNetwork *netinfo)
{
	netinfo->is_dhcp = ip_set.dhcp;
	netinfo->ipaddr = convertIPToInt(ip_set.ip);
	netinfo->mask = convertIPToInt(ip_set.subnet);
	netinfo->gw = convertIPToInt(ip_set.gateway);
	netinfo->dns1 = convertIPToInt(ip_set.dns1);
	netinfo->dns2 = convertIPToInt(ip_set.dns2);
	//netinfo->http_port = ip_set.webPort;
	//netinfo->rtsp_port = ip_set.rtspPort;
}

static void _convertInfoToNetwork(NFOpenmodeCamInfo info, NFOpenmodeSetupNetwork *netinfo)
{
	netinfo->is_dhcp = info.is_dhcp;
	netinfo->ipaddr = info.ipaddr;
	netinfo->mask = info.mask;
	netinfo->gw = info.gw;
	netinfo->dns1 = info.dns1;
	netinfo->dns2 = info.dns2;
	//netinfo->http_port = info.http_port;
	//netinfo->rtsp_port = info.rtsp_port;
}

static gboolean post_id_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *pw;
		NFOBJECT *top;
		guint x, y;

		if(nfui_nfobject_is_shown(g_btns[PP_BTN_CHAN_PW]))
			return FALSE;

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

		pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_ID_STRING_SIZE, VKEY_NORMAL);


		if(pw)
		{
			strcpy (g_post_info.u, pw); 
			nfui_nflabel_set_text((NFLABEL*)obj, pw);

			ifree(pw);
			pw = NULL;
		}
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

	}

	return FALSE;
}

static gboolean post_pw_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *strBuf = 0;
		gchar *pw = 0;
		NFOBJECT *top;
		guint x, y;
		int j;

		if(nfui_nfobject_is_shown(g_btns[PP_BTN_CHAN_PW]))
			return FALSE;

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

		strBuf = nfui_nflabel_get_text((NFLABEL*)obj);

		if (strlen(strBuf)) pw = VirtualKey_Open(g_curwnd, g_post_info.p, x, y, 16, VKEY_PASSWORD);
		else pw = VirtualKey_Open(g_curwnd, "", x, y, 16, VKEY_PASSWORD);

		if (pw) {
			strcpy (g_post_info.p, pw); 
			ifree(pw);			

			if (!strlen(strBuf)) 
			{
				nfui_nflabel_set_text((NFLABEL*)obj, "**********");
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static void _change_object_state(gint state, gint expose)
{
	int i;

	if (state == STATE_LOGIN_FAIL)
	{
		nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[PP_CONFIG_USER_NAME], NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[PP_CONFIG_PASS], NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_text((NFLABEL*)g_config_value[PP_CONFIG_USER_NAME], "");
		nfui_nflabel_set_text((NFLABEL*)g_config_value[PP_CONFIG_PASS], "");
		nfui_nfobject_use_focus(g_config_value[PP_CONFIG_USER_NAME], NFOBJECT_FOCUS_ON);
		nfui_nfobject_use_focus(g_config_value[PP_CONFIG_PASS], NFOBJECT_FOCUS_ON);
		nfui_regi_post_event_callback(g_config_value[PP_CONFIG_USER_NAME], post_id_label_event_handler);
		nfui_regi_post_event_callback(g_config_value[PP_CONFIG_PASS], post_pw_label_event_handler);

		nfui_nfobject_hide(g_btns[PP_BTN_CHAN_PW]);
		nfui_nfobject_show(g_btns[PP_BTN_LOGIN]);

		nfui_nfobject_enable(g_btns[PP_BTN_LOGIN]);		
//		nfui_nfobject_disable(g_btns[PP_BTN_IP_SETUP]);
//		nfui_nfobject_disable(g_btns[PP_BTN_PORT_SETUP]);
		nfui_nfobject_disable(g_btns[PP_BTN_RESET]);
		nfui_nfobject_disable(g_btns[PP_BTN_TEST]);
	}
	else if ((state == STATE_LOGIN_SUCCESS_ITX) || (state == STATE_LOGIN_SUCCESS_ONVIF) || (state == STATE_NEED_PW_CHANGE))
	{
		nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[PP_CONFIG_USER_NAME], NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[PP_CONFIG_PASS], NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(g_config_value[PP_CONFIG_USER_NAME], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_use_focus(g_config_value[PP_CONFIG_PASS], NFOBJECT_FOCUS_OFF);

		nfui_nfobject_show(g_btns[PP_BTN_CHAN_PW]);
		nfui_nfobject_hide(g_btns[PP_BTN_LOGIN]);

		if (state == STATE_LOGIN_SUCCESS_ITX)
		{
			//SKSHIN
			//nfui_nfobject_enable(g_btns[PP_BTN_CHAN_PW]);
//			nfui_nfobject_enable(g_btns[PP_BTN_IP_SETUP]);
//			nfui_nfobject_enable(g_btns[PP_BTN_PORT_SETUP]);
//			nfui_nfobject_enable(g_btns[PP_BTN_RESET]);
			nfui_nfobject_enable(g_btns[PP_BTN_TEST]);
		}
		else if (state == STATE_LOGIN_SUCCESS_ONVIF)
		{
			nfui_nfobject_disable(g_btns[PP_BTN_CHAN_PW]);
//			nfui_nfobject_disable(g_btns[PP_BTN_IP_SETUP]);
//			nfui_nfobject_disable(g_btns[PP_BTN_PORT_SETUP]);
//			nfui_nfobject_enable(g_btns[PP_BTN_RESET]);
			nfui_nfobject_enable(g_btns[PP_BTN_TEST]);
		}
		else if (state == STATE_NEED_PW_CHANGE)
		{
			//SKSHIN
			//nfui_nfobject_enable(g_btns[PP_BTN_CHAN_PW]);
//			nfui_nfobject_disable(g_btns[PP_BTN_IP_SETUP]);
//			nfui_nfobject_disable(g_btns[PP_BTN_PORT_SETUP]);
			nfui_nfobject_disable(g_btns[PP_BTN_RESET]);
			nfui_nfobject_disable(g_btns[PP_BTN_TEST]);
		}
	}
	else if (state == STATE_PW_CHANGE_FAIL)
	{
//		nfui_nfobject_disable(g_btns[PP_BTN_IP_SETUP]);
//		nfui_nfobject_disable(g_btns[PP_BTN_PORT_SETUP]);
		nfui_nfobject_disable(g_btns[PP_BTN_RESET]);
		nfui_nfobject_disable(g_btns[PP_BTN_TEST]);
	}
	else if (state == STATE_PW_CHANGE_SUCCESS)
	{
//		nfui_nfobject_enable(g_btns[PP_BTN_IP_SETUP]);
//		nfui_nfobject_enable(g_btns[PP_BTN_PORT_SETUP]);
//		nfui_nfobject_enable(g_btns[PP_BTN_RESET]);
		nfui_nfobject_enable(g_btns[PP_BTN_TEST]);
	}

	if (expose) {
		nfui_signal_emit(g_config_value[PP_CONFIG_USER_NAME], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_config_value[PP_CONFIG_PASS], GDK_EXPOSE, TRUE);

		for (i = 0; i<PP_BTN_MAX; i++)
			nfui_signal_emit(g_btns[i], GDK_EXPOSE, TRUE);
	}
}

static gboolean _openmode_set_login_info(gpointer data)
{
	gint ret = -1;

	ret = nf_openmode_set_login_info(g_post_info.index, g_post_info.u, g_post_info.p);

	if (ret >= 0)
	{
		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

		if (ret == 2) // Login Fail.
		{
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "LOGIN FAIL"); 
			_change_object_state(STATE_LOGIN_FAIL, 1);	
		}
		else
		{
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "SUCCESS");

			if (ret == 0)	// ITX Cam OK. Login Success.
				_change_object_state(STATE_LOGIN_SUCCESS_ITX, 1);	
			else if (ret == 1)	// ONVIF Cam OK. Login Success.
				_change_object_state(STATE_LOGIN_SUCCESS_ONVIF, 1);	
			else 
				g_warning("%s, %d, Unknown Error", __FUNCTION__, __LINE__);
		}
	}
	
	return FALSE;
}

static gboolean post_ipsetupbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	guint opt = 0;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		mb_type ret;
		IP_SET_T ipset;
		guint rtn;
		NFOpenmodeSetupNetwork netinfo;
		NFOpenmodeSetupNetwork pre_netinfo;
		memset(&ipset, 0x00, sizeof(IP_SET_T));

		_convertInfoToIpset(&ipset);

		// cam user
		opt |= (1 << MSK_DHCP);
		opt |= (1 << MSK_IPADDR);
		opt |= (1 << MSK_SUBNET);
		opt |= (1 << MSK_GATEWAY);
		opt |= (1 << MSK_DNS1);
		opt |= (1 << MSK_DNS2);
		//opt |= (1 << MSK_RTSP_PORT);
		//opt |= (1 << MSK_WEB_PORT);

		if(VW_Open_IPSet_Popup(g_curwnd, "IP SETUP FOR CAMERA", &ipset, opt))
		{
			memset(&netinfo, 0x00, sizeof(NFOpenmodeSetupNetwork));
			memset(&pre_netinfo, 0x00, sizeof(NFOpenmodeSetupNetwork));

			_convertIpsetToNetwork(ipset, &netinfo);
			_convertInfoToNetwork(g_post_info, &pre_netinfo);

			if(memcmp(&netinfo, &pre_netinfo, sizeof(NFOpenmodeSetupNetwork)) != 0)
			{
				ret = nftool_mbox(g_curwnd, "CONFIRM", "Camera Configuration is changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

				if(ret == NFTOOL_MB_OK)
				{					
					if(!wait_mbox)
					{
						wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
						uxm_reg_imsg_event(g_curwnd, IRPL_OPENMODE_IPSETUP);
						scm_openmode_ipsetup(g_post_info.index, &netinfo, IREQ_OPENMODE_IPSETUP);
						_convertIpsetToInfo(ipset);
					}
				}
			}
		}

	}
	
	return FALSE;
}

static gboolean post_portsetupbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	guint opt = 0;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		mb_type ret;
		IP_SET_T ipset;
		guint rtn;
		NFOpenmodeSetupPorts portinfo;
		NFOpenmodeSetupPorts pre_portinfo;
		memset(&ipset, 0x00, sizeof(IP_SET_T));

		//_convertInfoToIpset(&ipset);

		ipset.rtspPort = g_post_info.rtsp_port;
		ipset.webPort = g_post_info.http_port;

		// cam user
		opt |= (1 << MSK_RTSP_PORT);
		opt |= (1 << MSK_WEB_PORT);

		g_message("webport[%d], rtspport[%d]", ipset.webPort, ipset.rtspPort);

		pre_portinfo.http_port = ipset.webPort;
		pre_portinfo.rtsp_port = ipset.rtspPort;

		if(VW_Open_IPSet_Popup(g_curwnd, "PORT SETUP", &ipset, opt))
		{
			portinfo.http_port = ipset.webPort;
			portinfo.rtsp_port = ipset.rtspPort;

			if(memcmp(&portinfo, &pre_portinfo, sizeof(NFOpenmodeSetupPorts)) != 0)
			{
				ret = nftool_mbox(g_curwnd, "CONFIRM", "Camera Configuration is changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

				if(ret == NFTOOL_MB_OK)
				{					
					if(!wait_mbox)
					{
						wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
						uxm_reg_imsg_event(g_curwnd, IRPL_OPENMODE_PORTSETUP);
						scm_openmode_portsetup(g_post_info.index, &portinfo, IREQ_OPENMODE_PORTSETUP);
						_convertIpsetToInfo(ipset);
					}
				}
			}
		}

	}
	
	return FALSE;
}

static gboolean post_pwdbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		guint opt = 0;
		guint max_ch = 0;
		gchar pw[64] = {0,};
		mb_type ret;
		guint rtn;

		opt |= (1 << OPT_USE_ADMIN_PASS);

		if (DAL_get_enahnced_password()) {
			opt |= (1 << OPT_ENAHNCED_PASSWORD);
			max_ch = NFUI_MAX_ENHANCED_PW_SIZE;
		}
		else {
			max_ch = NFUI_MAX_PW_SIZE;
		}
		
		if(VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, g_post_info.u, NULL, pw, max_ch, 0))
		{
			if (g_pre_info.state != OPENMODE_CAM_STATE_PW_CHANGE) 
			{
				if (strcmp(pw, g_post_info.p) == 0) 
				{
					g_message("%s, %d, no change password", __FUNCTION__, __LINE__);
					nftool_mbox_auto(g_curwnd, 2, "NOTICE", "The password has not been changed.");
					return FALSE;
				}
			}

			ret = nftool_mbox(g_curwnd, "CONFIRM", "Camera Configuration is changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK)
			{
				strcpy (g_post_info.p, pw); 
				if(!wait_mbox)
				{
					wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
					uxm_reg_imsg_event(g_curwnd, IRPL_OPENMODE_CHANGE_PW);
					scm_openmode_change_pw(g_post_info.index, g_post_info.p, IREQ_OPENMODE_CHANGE_PW);
				}
			}
		}

	}
	
	return FALSE;
}

static gboolean post_loginbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(!wait_mbox)
		{
			wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
			g_timeout_add(10, _openmode_set_login_info, NULL);
		}
	}

	return FALSE;
}

static gboolean post_reset_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
		{
			if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			

		}
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_ping_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
		{
			gchar ipaddr_str[32];
			gchar ping_str[4096];
			gint strlen;

			TEXT_BOX_INFO box_info;
			guint box_opt = 0;
		
			if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			memset(ipaddr_str, 0x00, sizeof(ipaddr_str));
			memset(ping_str, 0x00, sizeof(ping_str));

			snprintf(ipaddr_str, 16, "%d.%d.%d.%d",
					(g_post_info.ipaddr & 0xff000000) >> 24,
					(g_post_info.ipaddr & 0xff0000) >> 16,
					(g_post_info.ipaddr & 0xff00) >> 8,
					(g_post_info.ipaddr & 0xff));

			g_message("%s, %d, ipaddr:%s", __FUNCTION__, __LINE__, ipaddr_str);

			strlen = sizeof(ping_str);
			nf_netif_ping_test(ipaddr_str, ping_str, &strlen);
			g_message("%s, %d, ping_str:%s", __FUNCTION__, __LINE__, ping_str);

			box_info.win_w = 800;
			box_info.win_h = 600;
			box_info.win_x = (1920-box_info.win_w)/2;
			box_info.win_y = (1080-box_info.win_h)/2;
			box_info.line = ping_str;
			
			box_opt |= (1 << TB_OPT_AUTO_LF);
	
			VW_TextBox_Open(g_curwnd, "PING TEST", &box_info, 0, box_opt);			
		}
		break;

		default:
		break;
	}
	
	return FALSE;
}

static gboolean post_test_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON)
					return FALSE;

				vsm_live_preview_stop();
				nfui_signal_emit(g_preview_fixed, GDK_EXPOSE, TRUE);
			}
			break;

		default:
			break;
	}
	
	return FALSE;
}

/*
static gboolean post_okbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}
*/

static gboolean post_closebtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(event->type == GDK_EXPOSE)
	{
    	drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	} 
    else if (event->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_DELETE) 
	{
		uxm_unreg_imsg_event(obj, INFY_IPCAM_INSTALL_NOTIFY);	
		g_curwnd = 0;
		gtk_main_quit();
	}
	else if (event->type == IRPL_OPENMODE_IPSETUP)
	{
		guint ret;

		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

		uxm_unreg_imsg_event(g_curwnd, IRPL_OPENMODE_IPSETUP);

		ret = ((CMM_MESSAGE_T *)data)->param;

		if(ret == OPENMODE_RTN_OK)
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "SUCCESS");
		else
		{
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "FAIL");
			g_memmove(&g_post_info, &g_pre_info, sizeof(NFOpenmodeCamInfo));
			return FALSE;
		}

		g_memmove(&g_pre_info, &g_post_info, sizeof(NFOpenmodeCamInfo));
		nfui_nflabel_set_text((NFLABEL*)g_config_value[PP_CONFIG_IP_ADDR], g_post_info.hostname);

		nfui_signal_emit(g_config_value[PP_CONFIG_IP_ADDR], GDK_EXPOSE, TRUE);
	}
	else if (event->type == IRPL_OPENMODE_CHANGE_PW)
	{
		guint ret;

		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

		uxm_unreg_imsg_event(g_curwnd, IRPL_OPENMODE_CHANGE_PW);

		ret = ((CMM_MESSAGE_T *)data)->param;

		if (ret == OPENMODE_RTN_OK) 
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "SUCCESS");
		else 
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "FAIL");

		if (g_pre_info.state == OPENMODE_CAM_STATE_PW_CHANGE)
		{
			if (ret == OPENMODE_RTN_OK) 
				_change_object_state(STATE_PW_CHANGE_SUCCESS, 1);
			else 
				_change_object_state(STATE_PW_CHANGE_FAIL, 1);
		}		
	}
	else if (event->type == IRPL_OPENMODE_PORTSETUP)
	{
		guint ret;

		if (wait_mbox) {
			nftool_remove_waitbox((NFOBJECT*)wait_mbox);
			wait_mbox = NULL;
		}

		uxm_unreg_imsg_event(g_curwnd, IRPL_OPENMODE_PORTSETUP);

		ret = ((CMM_MESSAGE_T *)data)->param;

		if(ret == OPENMODE_RTN_OK)
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "SUCCESS");
		else
		{
			nftool_mbox_auto(g_curwnd, 2, "NOTICE", "FAIL");
			g_memmove(&g_post_info, &g_pre_info, sizeof(NFOpenmodeCamInfo));
			return FALSE;
		}

		g_memmove(&g_pre_info, &g_post_info, sizeof(NFOpenmodeCamInfo));
		nfui_nflabel_set_number((NFLABEL*)g_config_value[PP_CONFIG_HTTP], g_post_info.http_port);
		nfui_nflabel_set_number((NFLABEL*)g_config_value[PP_CONFIG_RTSP], g_post_info.rtsp_port);

		nfui_signal_emit(g_config_value[PP_CONFIG_HTTP], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_config_value[PP_CONFIG_RTSP], GDK_EXPOSE, TRUE);
	}
	else if (event->type == INFY_IPCAM_INSTALL_NOTIFY)
	{
		gchar strBuf[128];
		gint i, tmp = 0;

		// update camera name
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_config_value[PP_CONFIG_CAM_NAME]), g_org_info->model) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_config_value[PP_CONFIG_CAM_NAME], g_org_info->model);
			nfui_signal_emit(g_config_value[PP_CONFIG_CAM_NAME], GDK_EXPOSE, TRUE);
		}

		// update rtsp port
		if (nfui_nflabel_get_number((NFLABEL*)g_config_value[PP_CONFIG_RTSP]) != g_org_info->rtsp_port)
		{
			nfui_nflabel_set_number((NFLABEL*)g_config_value[PP_CONFIG_RTSP], g_org_info->rtsp_port);
			nfui_signal_emit(g_config_value[PP_CONFIG_RTSP], GDK_EXPOSE, TRUE);
		}

		// update mac address
		memset(strBuf, 0x00, sizeof(strBuf));

		g_message("%s, %d, state:%d", __FUNCTION__, __LINE__, g_org_info->state);

		if ((g_org_info->state == OPENMODE_CAM_STATE_DEV_INFO) || (g_org_info->state == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) || (g_org_info->state == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))
		{
			for (i = 0; i < 6; i++)
				tmp += g_org_info->macaddr[i];
		}

		if (tmp > 0) {
			g_sprintf(strBuf, "%s : %02x:%02x:%02x:%02x:%02x:%02x", lookup_string("MAC ADDRESS"), 
			           g_org_info->macaddr[0], g_org_info->macaddr[1], g_org_info->macaddr[2], g_org_info->macaddr[3], g_org_info->macaddr[4], g_org_info->macaddr[5]);
		}
		else {
			g_sprintf(strBuf, "%s : ", lookup_string("MAC ADDRESS"));
		}
		
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_camera_info[PP_INFO_MAC]), strBuf) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_camera_info[PP_INFO_MAC], strBuf);
			nfui_signal_emit(g_camera_info[PP_INFO_MAC], GDK_EXPOSE, TRUE);
		}	

		// update f/w version
		memset(strBuf, 0x00, sizeof(strBuf));

		if (strlen(g_org_info->firmware_version2))
			g_sprintf(strBuf, "%s : %s", lookup_string("F/W VERSION"), g_org_info->firmware_version2);
		else		
			g_sprintf(strBuf, "%s : %s", lookup_string("F/W VERSION"), g_org_info->firmware_version);

		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_camera_info[PP_INFO_FW_VER]), strBuf) != 0)
		{
			nfui_nflabel_set_text((NFLABEL*)g_camera_info[PP_INFO_FW_VER], strBuf);
			nfui_signal_emit(g_camera_info[PP_INFO_FW_VER], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_preview_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_EXPOSE)
	{
		guint obj_x, obj_y;
		guint win_x, win_y;

		gint ch;
		guint ch_mask = 0;

		nfui_nfobject_get_window_pos(obj, &win_x, &win_y);
		nfui_nfobject_get_offset(obj, &obj_x, &obj_y);

		vsm_live_preview_start(1 << 0, win_x+obj_x+18, win_y+obj_y+60, 480, 270);		
	} 
	else if(event->type == GDK_DELETE) 
		vsm_live_preview_stop();
	
	return FALSE;
}


///////////////////////////////////////////////////////////////////
//
//
//
mb_type VW_Open_Cam_Ipcam_Setup_Popup(NFWINDOW *parent, NFOpenmodeCamInfo *info)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *config_fixed = NULL;
	NFOBJECT *diag_fixed = NULL;
	NFOBJECT *info_fixed = NULL;
	//NFOBJECT *preview_fixed = NULL;	
	NFOBJECT *obj = NULL;

	const gchar *config_str[] = {"CAMERA NAME", "IP ADDRESS", "HTTP PORT", 
	                             "RTSP PORT", "USER ID", "PASSWORD" };
	
	const gchar *info_str[] = {"MAC ADDRESS", "F/W VERSION"};

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(394), COLOR_IDX(394), COLOR_IDX(393), COLOR_IDX(394)};

	gint i, j, tmp;
	guint pos_x, pos_y;
	guint size_w, size_h;

	g_memmove(&g_pre_info, info, sizeof(NFOpenmodeCamInfo));
	g_memmove(&g_post_info, info, sizeof(NFOpenmodeCamInfo));
	g_org_info = info;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, DETAIL_WIN_X, DETAIL_WIN_Y, DETAIL_WIN_W, DETAIL_WIN_H);
	g_curwnd = win;
	nfui_regi_post_event_callback(win, post_main_win_event_handler);

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, DETAIL_WIN_W, DETAIL_WIN_H);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

	config_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(config_fixed, DETAIL_CONFIG_FIXED_W, DETAIL_CONFIG_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, config_fixed, DETAIL_CONFIG_FIXED_X, DETAIL_CONFIG_FIXED_Y);
	nfui_nfobject_show(config_fixed);

	diag_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(diag_fixed, DETAIL_DIAG_FIXED_W, DETAIL_DIAG_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, diag_fixed, DETAIL_DIAG_FIXED_X, DETAIL_DIAG_FIXED_Y);	
	nfui_nfobject_show(diag_fixed);

	info_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(info_fixed, DETAIL_INFO_FIXED_W, DETAIL_INFO_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, info_fixed, DETAIL_INFO_FIXED_X, DETAIL_INFO_FIXED_Y);	
	nfui_nfobject_show(info_fixed);

	g_preview_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(g_preview_fixed, DETAIL_PREVIEW_FIXED_W, DETAIL_PREVIEW_FIXED_H);
	nfui_regi_post_event_callback(g_preview_fixed, post_preview_fixed_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_preview_fixed, DETAIL_PREVIEW_FIXED_X, DETAIL_PREVIEW_FIXED_Y);
	nfui_nfobject_show(g_preview_fixed);

// <---- MAIN FIXED TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP CAMERA CONFIGURATION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, DETAIL_WIN_W-47, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);


// <---- CONFIGURATION FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CONFIGURATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)config_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	for (i = 0; i < PP_CONFIG_ROWS; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(config_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 200, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);

		pos_x += 200;

		if (i == PP_CONFIG_CAM_NAME)
		{
			g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box(info->model, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(g_config_value[i], NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);	
		}
		else if (i == PP_CONFIG_IP_ADDR)
		{	
			g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box(info->hostname, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(g_config_value[i], FALSE);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);

			pos_x += 230;

			g_btns[PP_BTN_IP_SETUP] = nftool_normal_button_create_popup_type1("IP SETUP", 150);
			nfui_nfobject_disable(g_btns[PP_BTN_IP_SETUP]);
			nfui_nfobject_show(g_btns[PP_BTN_IP_SETUP]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_btns[PP_BTN_IP_SETUP], pos_x, pos_y);
			nfui_regi_post_event_callback(g_btns[PP_BTN_IP_SETUP], post_ipsetupbtn_event_handler);			

		}
		else if (i == PP_CONFIG_USER_NAME)
		{
			g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box(info->u, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);			
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_support_multi_lang((NFOBJECT*)g_config_value[i], FALSE);
			nfui_nfobject_use_focus(g_config_value[i], FALSE);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);
		}
		else if (i == PP_CONFIG_PASS)
		{
			if (!strlen(info->p)) g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			else g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("**********", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_support_multi_lang((NFOBJECT*)g_config_value[i], FALSE);
			nfui_nfobject_use_focus(g_config_value[i], FALSE);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);

			pos_x += 230;

			g_btns[PP_BTN_CHAN_PW] = nftool_normal_button_create_popup_type1("CHANGE PASSWORD", 230);
			nfui_nfobject_disable(g_btns[PP_BTN_CHAN_PW]);
			nfui_nfobject_show(g_btns[PP_BTN_CHAN_PW]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_btns[PP_BTN_CHAN_PW], pos_x, pos_y);
			nfui_regi_post_event_callback(g_btns[PP_BTN_CHAN_PW], post_pwdbtn_event_handler);		

			g_btns[PP_BTN_LOGIN] = nftool_normal_button_create_popup_type1("LOG IN", 150);
			nfui_nfobject_disable(g_btns[PP_BTN_LOGIN]);
			nfui_nfobject_hide(g_btns[PP_BTN_LOGIN]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_btns[PP_BTN_LOGIN], pos_x, pos_y);
			nfui_regi_post_event_callback(g_btns[PP_BTN_LOGIN], post_loginbtn_event_handler);		
		}
		else if (i == PP_CONFIG_HTTP)
		{
			g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(g_config_value[i], FALSE);
			nfui_nflabel_use_number((NFLABEL*)g_config_value[i], TRUE);
			nfui_nflabel_set_number((NFLABEL*)g_config_value[i], info->http_port);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);
		}
		else // i == PP_CONFIG_RTSP
		{
			g_config_value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)g_config_value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(g_config_value[i], 220, 40);
			nfui_nflabel_set_align((NFLABEL*)g_config_value[i], NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(g_config_value[i], FALSE);
			nfui_nflabel_use_number((NFLABEL*)g_config_value[i], TRUE);
			nfui_nflabel_set_number((NFLABEL*)g_config_value[i], info->rtsp_port);
			nfui_nfobject_show(g_config_value[i]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_config_value[i], pos_x, pos_y);

			pos_x += 230;

			g_btns[PP_BTN_PORT_SETUP] = nftool_normal_button_create_popup_type1("PORT SETUP", 150);
			nfui_nfobject_disable(g_btns[PP_BTN_PORT_SETUP]);
			nfui_nfobject_show(g_btns[PP_BTN_PORT_SETUP]);
			nfui_nffixed_put((NFFIXED*)config_fixed, g_btns[PP_BTN_PORT_SETUP], pos_x, pos_y);
			nfui_regi_post_event_callback(g_btns[PP_BTN_PORT_SETUP], post_portsetupbtn_event_handler);			

			pos_y += 18;
		}
		pos_x = 0;
		pos_y += 43;
	}


// <---- DIAGNOSTIC TOOLS FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "DIAGNOSTIC TOOLS");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diag_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP CAMERA RESET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x, pos_y);
	pos_x += 250;

	g_btns[PP_BTN_RESET] = nftool_normal_button_create_popup_type1("RESET", 180);
	nfui_nfobject_disable(g_btns[PP_BTN_RESET]);
	nfui_nfobject_show(g_btns[PP_BTN_RESET]);
	nfui_nffixed_put((NFFIXED*)diag_fixed, g_btns[PP_BTN_RESET], pos_x, pos_y);
	nfui_regi_post_event_callback(g_btns[PP_BTN_RESET], post_reset_btn_event_handler);			

	pos_x = 0;
	pos_y += 43;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NETWORK TEST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x, pos_y);
	pos_x += 250;

	g_btns[PP_BTN_PING] = nftool_normal_button_create_popup_type1("PING TEST", 180);
	nfui_nfobject_show(g_btns[PP_BTN_PING]);
	nfui_nffixed_put((NFFIXED*)diag_fixed, g_btns[PP_BTN_PING], pos_x, pos_y);
	nfui_regi_post_event_callback(g_btns[PP_BTN_PING], post_ping_btn_event_handler);			
	pos_x += 190;


// <---- PREVIEW FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "PREVIEW");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_preview_fixed, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, 480, 270);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_preview_fixed, obj, 18, 60);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONNECTION TEST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 312, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_preview_fixed, obj, 0, 348);

	g_btns[PP_BTN_TEST] = nftool_normal_button_create_popup_type1("TEST", 120);
	nfui_nfobject_disable(g_btns[PP_BTN_TEST]);
	nfui_nfobject_show(g_btns[PP_BTN_TEST]);
	nfui_nffixed_put((NFFIXED*)g_preview_fixed, g_btns[PP_BTN_TEST], 360, 348);
	nfui_regi_post_event_callback(g_btns[PP_BTN_TEST], post_test_btn_event_handler);


// <---- CAMERA INFO FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CAMERA INFO");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)info_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	for (i = 0; i < PP_INFO_ROWS; i++)
	{
		gchar buf[128];

		memset(buf, 0x00, sizeof(buf));

		if (i == PP_INFO_MAC)
		{
			tmp = 0;

			if ((info->state == OPENMODE_CAM_STATE_DEV_INFO) || (info->state == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) || (info->state == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))
			{
				for (j = 0; j < 6; j++)
					tmp += info->macaddr[j];
			}
			
			if (tmp > 0) {
			g_sprintf(buf, "%s : %02x:%02x:%02x:%02x:%02x:%02x", lookup_string(info_str[i]), 
			           info->macaddr[0], info->macaddr[1], info->macaddr[2], info->macaddr[3], info->macaddr[4], info->macaddr[5]);
		}
			else {
				g_sprintf(buf, "%s : ", lookup_string(info_str[i]));
			}
		}
		else // i == PP_INFO_FW_VER
		{
			if (strlen(info->firmware_version2))
				g_sprintf(buf, "%s : %s", lookup_string(info_str[i]), info->firmware_version2);
			else
				g_sprintf(buf, "%s : %s", lookup_string(info_str[i]), info->firmware_version);
		}

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(293));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_set_size(obj, 450, 24);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)info_fixed, obj, pos_x, pos_y);
		g_camera_info[i] = obj;

		pos_y += 24;
	}

	obj = nftool_normal_button_create_type1("CLOSE", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 524, 635);
	nfui_regi_post_event_callback(obj, post_closebtn_event_handler);
	
	if (info->state == OPENMODE_CAM_STATE_DEV_INFO)	
		_change_object_state(STATE_LOGIN_SUCCESS_ITX, 0);
	else if ((info->state == OPENMODE_CAM_STATE_DEV_INFO_ONVIF) || (info->state == OPENMODE_CAM_STATE_VIRTUAL_CAMERA))	
		_change_object_state(STATE_LOGIN_SUCCESS_ONVIF, 0);
	else if (info->state == OPENMODE_CAM_STATE_LOGIN_FAIL)	
		_change_object_state(STATE_LOGIN_FAIL, 0);
	else if (info->state == OPENMODE_CAM_STATE_PW_CHANGE)	
		_change_object_state(STATE_NEED_PW_CHANGE, 0);

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy(win);

	uxm_reg_imsg_event(win, INFY_IPCAM_INSTALL_NOTIFY);	

	nfui_page_open(PGID_IPCAMERA_SET_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_IPCAMERA_SET_POPUP, win);
}
