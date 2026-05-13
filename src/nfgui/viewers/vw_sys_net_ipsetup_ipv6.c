#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"
#include <arpa/inet.h>

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
#include "objects/nfbutton.h"

#include "tools/nf_ui_function.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_ipsetup_ipv6.h"
#include "vw_vkeyboard_ipv6.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_TITLE_LABEL_WIDTH	(280)
#define	SUBJECT_IP_LABEL_WIDTH	    (500)
#define	SUBJECT_LABEL_MARGIN		(0)

#define	VALUE_LABEL_WIDTH			(278)

#define	IPS_LABEL_COL_SPACE			(2)
#define	IPS_LABEL_ROW_SPACE			(2)
#define	IPS_LABEL_HEIGHT			(40)


typedef enum {
    IPV6_OFF = 0,
    IPV6_MANUAL,
    IPV6_DHCPV6,

    IPV6_MODE
} IPV6_MODE_E;

enum {
    IPV6_LINKLOCAL = 0,
    IPV6_GATEWAY,
    IPV6_DNS1,
    IPV6_DNS2,

    IPV6_OBJ
};

static IPv6Data ipv6data;
static IPv6Data org_ipv6data;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ipv6mode[IPV6_MODE];
static NFOBJECT *g_ipv6obj[IPV6_OBJ];
static NFOBJECT *g_ipv6addr[SUPPORT_IPV6_ADDR_CNT];
static NFOBJECT *g_ipv6prefix[SUPPORT_IPV6_ADDR_CNT];
static NFOBJECT *g_renew;
static NFOBJECT *g_wait_pop = NULL;



static void _current_set_ipv6_data(void)
{
	NF_NETIF_GET_INFO info;
	gint i;

    memset(&info, 0x00, sizeof(NF_NETIF_GET_INFO));
    
	scm_get_sys_netinfo(&info);

	strcpy(ipv6data.linklocal, info.ipv6_linklocal);

	for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
	{
	    strcpy(ipv6data.ipaddr[i], info.ipv6_addr[i]);
	    if (info.ipv6_prefix[i] <= 0) ipv6data.prefix_len[i] = 64;
	    else ipv6data.prefix_len[i] = info.ipv6_prefix[i];
	}

	strcpy(ipv6data.gateway, info.ipv6_gateway);
	strcpy(ipv6data.dns1, info.ipv6_dns[0]);
	strcpy(ipv6data.dns2, info.ipv6_dns[1]);
}

static void _init_ipv6_data()
{
    memset(&ipv6data, 0x00, sizeof(IPv6Data));
    memset(&org_ipv6data, 0x00, sizeof(IPv6Data));
    
    DAL_get_ipv6_data(&ipv6data);
    _current_set_ipv6_data();
    
    memmove(&org_ipv6data, &ipv6data, sizeof(IPv6Data));
}

static void _set_ipv6data_to_obj(gint expose)
{
    gint i;

    nfui_radio_button_set_toggled((NFBUTTON*)g_ipv6mode[ipv6data.use], TRUE);
/*
    if (ipv6data.use == IPV6_OFF)
    {
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_LINKLOCAL], "");

        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_nflabel_set_text((NFLABEL*)g_ipv6addr[i], "");
            nfui_nflabel_use_number((NFLABEL*)g_ipv6prefix[i], FALSE);
            nfui_nflabel_set_text((NFLABEL*)g_ipv6prefix[i], "");
        }
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_GATEWAY], "");
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_DNS1], "");
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_DNS2], "");
    }
    else
    {
*/
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_LINKLOCAL], ipv6data.linklocal);

        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_nflabel_set_text((NFLABEL*)g_ipv6addr[i], ipv6data.ipaddr[i]);
            if (strlen(ipv6data.ipaddr[i]) <= 0) {
                nfui_nflabel_use_number((NFLABEL*)g_ipv6prefix[i], FALSE);
                nfui_nflabel_set_text((NFLABEL*)g_ipv6prefix[i], "");
            }
            else {
                nfui_nflabel_use_number((NFLABEL*)g_ipv6prefix[i], TRUE);
                nfui_nflabel_set_number((NFLABEL*)g_ipv6prefix[i], ipv6data.prefix_len[i]);
            }
        }
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_GATEWAY], ipv6data.gateway);
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_DNS1], ipv6data.dns1);
        nfui_nflabel_set_text((NFLABEL*)g_ipv6obj[IPV6_DNS2], ipv6data.dns2);
//    }

    if (expose)
    {
        for (i = 0; i < IPV6_MODE; i++) 
        {
            nfui_signal_emit(g_ipv6mode[i], GDK_EXPOSE, TRUE);
        }
            
        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_signal_emit(g_ipv6addr[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipv6prefix[i], GDK_EXPOSE, TRUE);
        }

        for (i = 0; i < IPV6_OBJ; i++)
        {
            nfui_signal_emit(g_ipv6obj[i], GDK_EXPOSE, TRUE);
        }
    }
}

static void _get_ipv6data_from_obj()
{
    gint i;
    
    for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
    {
        strcpy(ipv6data.ipaddr[i], nfui_nflabel_get_text((NFLABEL*)g_ipv6addr[i]));
        ipv6data.prefix_len[i] = nfui_nflabel_get_number((NFLABEL*)g_ipv6prefix[i]);
        if (ipv6data.prefix_len[i] <= 0) ipv6data.prefix_len[i] = 64;
    }
    
    strcpy(ipv6data.gateway, nfui_nflabel_get_text((NFLABEL*)g_ipv6obj[IPV6_GATEWAY]));
    strcpy(ipv6data.dns1, nfui_nflabel_get_text((NFLABEL*)g_ipv6obj[IPV6_DNS1]));
    strcpy(ipv6data.dns2, nfui_nflabel_get_text((NFLABEL*)g_ipv6obj[IPV6_DNS2]));
}

static void _change_ui_status(IPV6_MODE_E mode, gint expose)
{
    gint i;

    if (mode == IPV6_MANUAL)
    {
        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_nfobject_enable(g_ipv6addr[i]);
            nfui_nfobject_enable(g_ipv6prefix[i]);
        }

        for (i = IPV6_GATEWAY; i < IPV6_OBJ; i++)
        {
            nfui_nfobject_enable(g_ipv6obj[i]);
        }
    }
    else
    {
        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_nfobject_disable(g_ipv6addr[i]);
            nfui_nfobject_disable(g_ipv6prefix[i]);
        }

        for (i = IPV6_GATEWAY; i < IPV6_OBJ; i++)
        {
            nfui_nfobject_disable(g_ipv6obj[i]);
        }
    }

    if (mode == IPV6_DHCPV6) nfui_nfobject_enable(g_renew);
    else                     nfui_nfobject_disable(g_renew);

    if (expose)
    {
        for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
        {
            nfui_signal_emit(g_ipv6addr[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ipv6prefix[i], GDK_EXPOSE, TRUE);
        }

        for (i = IPV6_GATEWAY; i < IPV6_OBJ; i++)
        {
            nfui_signal_emit(g_ipv6obj[i], GDK_EXPOSE, TRUE);
        }

        nfui_signal_emit(g_renew, GDK_EXPOSE, TRUE);
    }
}

static gint _check_ipv6_validity(NFOBJECT *obj, gchar *ipaddr)
{
    struct sockaddr_in6 sa;
	gchar addr6_binary[16];
    return inet_pton(AF_INET6, ipaddr, &addr6_binary);
 
//    struct sockaddr_in6 sa;
//    char ip[INET6_ADDRSTRLEN];
//    inet_pton(AF_INET6, str.c_str(), &(sa.sin6_addr));
//    inet_ntop(AF_INET6, &(sa.sin6_addr), ip, INET6_ADDRSTRLEN);
//    return str==string(ip);
/* 
    gchar *s = ipaddr;
    gint colons = 0, segsize = 0, i , len = strlen(ipaddr);
    
    if (s[0] == ':' || s[len-1] == ':') return 1;

    for(colons = 0, i = 0, segsize = 0; i < len; i++)
    {
        if (s[i] == ':')
        {
            segsize = 0;
            colons++;
        }
        else if (('0' <= s[i] && s[i] <= '9') || ('a' <= s[i] && s[i] <= 'f') )//|| ('A' <= s[i] && s[i] <='F')
        {
            segsize++;
        }
        else
        {
            return 2;
        }
        if (segsize >= 5) return 3;
    }
    if (colons>7) return 4;
*/  
//    return 0;    
}

static gboolean _check_addr_validity(NFOBJECT *obj, gchar *ipaddr)
{
    gint result;
    gchar error[512], input[512];

    memset(error, 0x00, sizeof(error));
    memset(input, 0x00, sizeof(input));

    result = _check_ipv6_validity(obj, ipaddr);
    g_sprintf(input, "\n\n- Input address : %s", ipaddr);

    if (result != 1) {
        g_sprintf(error, "%s%s", lookup_string("The address is not valid."), input);
        nftool_mbox(g_curwnd, "ERROR", error, NFTOOL_MB_OK);
    }

    return result;
/*
    g_sprintf(input, "\n\n- INPUT ADDRESS : %s", ipaddr);
    
    switch(result)
    {
        case 1:
            g_sprintf(error, "%s%s", lookup_string("The first character can not be ':'."), input);
            nftool_mbox(g_curwnd, "ERROR", error, NFTOOL_MB_OK);
            return FALSE;
        break;
        
        case 2:
            g_sprintf(error, "%s%s", lookup_string("Contains unacceptable characters."), input);
            nftool_mbox(g_curwnd, "ERROR", error, NFTOOL_MB_OK);
            return FALSE;
        break;
        
        case 3:
            g_sprintf(error, "%s%s", lookup_string("Some fields exceed the valid range."), input);
            nftool_mbox(g_curwnd, "ERROR", error, NFTOOL_MB_OK);
            return FALSE;
        break;
        
        case 4:
            g_sprintf(error, "%s%s", lookup_string("':' Contains more than 8."), input);
            nftool_mbox(g_curwnd, "ERROR", error, NFTOOL_MB_OK);
            return FALSE;
        break;
    }
      
    return TRUE;
*/
}

static gboolean post_btn_renew_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    			
		g_wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...");

        DAL_set_ipv6_data(&ipv6data);
		scm_apply_netinfo_by_db(IRET_SCM_DHCP_RENEW);
	}

	return FALSE;
}

static gboolean post_btn_ipv6mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint idx;
    
    if (evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        idx = nfui_radio_button_get_index((NFBUTTON*)obj);

        ipv6data.use = idx;
        _set_ipv6data_to_obj(1);
        _change_ui_status(ipv6data.use, 1);
    }

    return FALSE;
}

static gboolean post_lb_prefix_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint org_prefix;	
	gint new_prefix;
	guint x, y;
	GdkEventKey *kevt;
	KEYPAD_KID kpid;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(kpid == KEYPAD_ENTER) {
			nfui_nfobject_get_offset(obj, &x, &y);
			y += obj->height;
		}else {
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

        org_prefix = nfui_nflabel_get_number((NFLABEL*)obj);
        new_prefix = NumberKey_Open(g_curwnd, org_prefix, x, y, 128);

        if (new_prefix < 0) return FALSE;

        if (new_prefix == 0 || new_prefix > 128) {
            nftool_mbox(g_curwnd, "NOTICE", "You can specify only a value between 1 and 128.", NFTOOL_MB_OK);
            return FALSE;
        }
        
		nfui_nflabel_set_number((NFLABEL*)obj, new_prefix);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
	return FALSE;
}

static gboolean post_lb_ipaddr_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *org_addr, *strTemp = NULL;	
	gchar compressed_addr[64];
	guint x, y;
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
    mb_type ret = NFTOOL_MB_OK;	
    gint i;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(kpid == KEYPAD_ENTER) {
			nfui_nfobject_get_offset(obj, &x, &y);
			y += obj->height;
		}else {
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
		{
		    if (obj == g_ipv6addr[i]) break;
		}
		
		if (i == SUPPORT_IPV6_ADDR_CNT) return FALSE;

        org_addr = nfui_nflabel_get_text((NFLABEL*)obj);

		strTemp = VirtualKey_IPv6_Open(g_curwnd, org_addr, x, y, 64);

		if(strTemp) 
		{
			if(strlen(strTemp) <= 0) {
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
				
				nfui_nflabel_use_number((NFLABEL*)g_ipv6prefix[i], FALSE);
				nfui_nflabel_set_text((NFLABEL*)g_ipv6prefix[i], "");
				nfui_signal_emit(g_ipv6prefix[i], GDK_EXPOSE, FALSE);
			}
            else
            {
                if (_check_addr_validity(obj, strTemp))
                {
                    memset(compressed_addr, 0x00, sizeof(compressed_addr));
                    
                    scm_get_compressed_ipv6_addr(strTemp, compressed_addr);
    				nfui_nflabel_set_text((NFLABEL*)obj, compressed_addr);
    				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    				
    				nfui_nflabel_use_number((NFLABEL*)g_ipv6prefix[i], TRUE);
    				nfui_nflabel_set_number((NFLABEL*)g_ipv6prefix[i], ipv6data.prefix_len[i]);
    				nfui_signal_emit(g_ipv6prefix[i], GDK_EXPOSE, FALSE);
    			}
			}
			
			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_lb_ip_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *org_addr, *strTemp = NULL;	
	gchar compressed_addr[64];
	guint x, y;
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
    mb_type ret = NFTOOL_MB_OK;	
    gint i;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(kpid == KEYPAD_ENTER) {
			nfui_nfobject_get_offset(obj, &x, &y);
			y += obj->height;
		}else {
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

        org_addr = nfui_nflabel_get_text((NFLABEL*)obj);

		strTemp = VirtualKey_IPv6_Open(g_curwnd, org_addr, x, y, 64);

		if(strTemp) 
		{
			if(strlen(strTemp) <= 0) {
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
			}
            else
            {
                if (_check_addr_validity(obj, strTemp))
                {
                    memset(compressed_addr, 0x00, sizeof(compressed_addr));
                    
                    scm_get_compressed_ipv6_addr(strTemp, compressed_addr);
    				nfui_nflabel_set_text((NFLABEL*)obj, compressed_addr);
    				nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    			}
			}
			
			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
    	gint expose = 1;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

    	_get_ipv6data_from_obj();

		if (memcmp(&ipv6data, &org_ipv6data, sizeof(IPv6Data)))
		{
		    memmove(&ipv6data, &org_ipv6data, sizeof(IPv6Data));

		    _set_ipv6data_to_obj(expose);
		    _change_ui_status(ipv6data.use, expose);
		}
    }

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	switch (evt->type) 
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
    			return FALSE;
    		}

    		_get_ipv6data_from_obj();

    		if (memcmp(&ipv6data, &org_ipv6data, sizeof(IPv6Data)))
    		{
    		    memmove(&org_ipv6data, &ipv6data, sizeof(IPv6Data));
    		    DAL_set_ipv6_data(&ipv6data);
    			sysnet_set_changeflag(1);
    		    
    			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
    			
				g_wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...");
				//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
				scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
    		}
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

		IpSetup_IPv6_tab_out_handler();

		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
        uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
    	uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);
    	uxm_unreg_imsg_event(obj, IRET_SCM_DHCP_RENEW);
	}
	else if (evt->type == IRET_SCM_APPLY_NETINFO || evt->type == IRET_SCM_APPLY_NETINFO2)
	{
	    if (g_wait_pop) {
    	    nftool_remove_waitbox(g_wait_pop);
    	    g_wait_pop = NULL;
	    }

	    _current_set_ipv6_data();
	    DAL_set_ipv6_data(&ipv6data);
	    g_memmove(&org_ipv6data, &ipv6data, sizeof(IPv6Data));

	    if (evt->type == IRET_SCM_APPLY_NETINFO2) {
    	    _set_ipv6data_to_obj(0);
	        gtk_main_quit();
        }
        else {
            _set_ipv6data_to_obj(1);
        }
	}
	else if (evt->type == IRET_SCM_DHCP_RENEW)
	{
	    if (g_wait_pop) {
    	    nftool_remove_waitbox(g_wait_pop);
    	    g_wait_pop = NULL;
	    }

	    _current_set_ipv6_data();
	    _set_ipv6data_to_obj(1);
	    g_memmove(&org_ipv6data, &ipv6data, sizeof(IPv6Data));

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
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

void init_NetIPSetup_IPv6_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;

	guint pos_x, pos_y;
	gint size_w, size_h;
	guint i;
	gchar strTemp[32];
	gint expose = 0;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GList *radio_btn = NULL;

	g_curwnd = nfui_nfobject_get_top(parent);
	nfui_regi_post_event_callback(g_curwnd, post_page_event_handler);

	_init_ipv6_data();


// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
	nfui_regi_post_event_callback(content_fixed, post_fixed_event_handler);

	pos_x = SUBJECT_LABEL_LEFT;
	pos_y = SUBJECT_LABEL_TOP;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IPv6", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += SUBJECT_TITLE_LABEL_WIDTH + IPS_LABEL_COL_SPACE;
	
	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    //OFF
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y + ((IPS_LABEL_HEIGHT - size_h) / 2));
	nfui_regi_post_event_callback(obj, post_btn_ipv6mode_event_handler);
	radio_btn = nfui_radio_button_get_group(NF_BUTTON(obj));	
	g_ipv6mode[IPV6_OFF] = obj;

	pos_x += size_w + IPS_LABEL_COL_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFF", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 150, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	
    pos_x += 150 + IPS_LABEL_COL_SPACE;
    
    //ON(MANUAL)
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y + ((IPS_LABEL_HEIGHT - size_h) / 2));
	nfui_regi_post_event_callback(obj, post_btn_ipv6mode_event_handler);
	nfui_radio_button_add_group(NF_BUTTON(obj), radio_btn);	
	g_ipv6mode[IPV6_MANUAL] = obj;

	pos_x += size_w + IPS_LABEL_COL_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON (MANUAL)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 150, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	
    pos_x += 150 + 30;

    //ON(DHCP)
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y + ((IPS_LABEL_HEIGHT - size_h) / 2));
	nfui_regi_post_event_callback(obj, post_btn_ipv6mode_event_handler);
	nfui_radio_button_add_group(NF_BUTTON(obj), radio_btn);	
	g_ipv6mode[IPV6_DHCPV6] = obj;

	pos_x += size_w + IPS_LABEL_COL_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON (AUTO)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 150, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 150 + IPS_LABEL_COL_SPACE;
	
	obj = nftool_normal_button_create_type3("RENEW", 190);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_btn_renew_event_handler);
	g_renew = obj;

	pos_x = SUBJECT_LABEL_LEFT;
	pos_y += IPS_LABEL_HEIGHT * 2 + IPS_LABEL_ROW_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINK-LOCAL ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, SUBJECT_IP_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + SUBJECT_TITLE_LABEL_WIDTH, pos_y);
	g_ipv6obj[IPV6_LINKLOCAL] = obj;

	pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;

    for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
    {
        memset(&strTemp, 0x00, sizeof(strTemp));
        g_sprintf(strTemp, "%s %d", lookup_string("IP ADDRESS"), i+1);
        
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTemp, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        pos_x += SUBJECT_TITLE_LABEL_WIDTH;
        
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, SUBJECT_IP_LABEL_WIDTH, IPS_LABEL_HEIGHT);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_regi_post_event_callback(obj, post_lb_ipaddr_event_handler);
    	g_ipv6addr[i] = obj;

        pos_x += SUBJECT_IP_LABEL_WIDTH;
        
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("/", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 30, IPS_LABEL_HEIGHT);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 30;
        
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 100, IPS_LABEL_HEIGHT);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_regi_post_event_callback(obj, post_lb_prefix_event_handler);
    	g_ipv6prefix[i] = obj;

    	pos_x = SUBJECT_LABEL_LEFT;
    	pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GATEWAY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, SUBJECT_IP_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + SUBJECT_TITLE_LABEL_WIDTH, pos_y);
	nfui_regi_post_event_callback(obj, post_lb_ip_event_handler);
	g_ipv6obj[IPV6_GATEWAY] = obj;

	pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1ST DNS SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, SUBJECT_IP_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + SUBJECT_TITLE_LABEL_WIDTH, pos_y);
	nfui_regi_post_event_callback(obj, post_lb_ip_event_handler);
	g_ipv6obj[IPV6_DNS1] = obj;

	pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2ND DNS SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_TITLE_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, SUBJECT_IP_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + SUBJECT_TITLE_LABEL_WIDTH, pos_y);
	nfui_regi_post_event_callback(obj, post_lb_ip_event_handler);
	g_ipv6obj[IPV6_DNS2] = obj;

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

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	
    _set_ipv6data_to_obj(expose);
    _change_ui_status(ipv6data.use, expose);

    uxm_reg_imsg_event(content_fixed, IRET_SCM_APPLY_NETINFO);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_APPLY_NETINFO2);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_DHCP_RENEW);
}

gboolean IpSetup_IPv6_tab_out_handler()
{
	mb_type ret;
	gint expose = 0;

	_get_ipv6data_from_obj();

	if(!memcmp(&org_ipv6data, &ipv6data, sizeof(IPv6Data)))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
	    memmove(&org_ipv6data, &ipv6data, sizeof(IPv6Data));
	    DAL_set_ipv6_data(&ipv6data);
		sysnet_set_changeflag(1);
    			
		g_wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...");
		scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO2);

		gtk_main();
	}
	else
	{
        memmove(&ipv6data, &org_ipv6data, sizeof(IPv6Data));

        _set_ipv6data_to_obj(expose);
        _change_ui_status(ipv6data.use, expose);
	}

	return FALSE;

}

