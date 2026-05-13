#include "nf_afx.h"

#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfwindow.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_ipsetup_main.h"
#include "vw_sys_net_ddns.h"
#include "vw_sys_net_email.h"
#include "vw_sys_net_info.h"
#include "vw_sys_net_security.h"
#include "vw_sys_net_snmp.h"
#include "vw_sys_net_rtp.h"

#include "vsm.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static guint net_change_flag = 0;


static void _init_net_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_IPSETUP];  
    if (pos != -1) 
    {
        DMSG(1, "init_NetIPSetup_Main_page");
    
        init_NetIPSetup_Main_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS]; 
    if (pos != -1) 
    {
        DMSG(1, "init_NetDDNS_page");

        init_NetDDNS_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_EMAIL];    
    if (pos != -1) 
    {
        DMSG(1, "init_NetEMail_page");

        init_NetEMail_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_NETSTAT];  
    if (pos != -1) 
    {
        DMSG(1, "init_NetInfo_page");

        init_NetInfo_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_SECURITY]; 
    if (pos != -1) 
    {
        DMSG(1, "init_NetSecurity_page");

        init_NetSecurity_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_SNMP];
    if (pos != -1)
    {
        DMSG(1, "init_NetSNMP_page");

        init_NetSnmp_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_CABLE_TEST];
    if (pos != -1)
    {
        DMSG(1, "init_NetCableTest_page");

        init_CableTest_page(((NFTAB*)nftab)->page[pos]);
    }
    
	pos = mcf.sys_sub5.menu_pos[SYS_SUB5_RTP];	
	if (pos != -1) init_NetRtp_page(((NFTAB*)nftab)->page[pos]);
}

static void _in_handler_net_main(gint page)
{
    if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_IPSETUP])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_EMAIL])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_NETSTAT])
    {
        DMSG(1, "NetInfo_tab_in_handler");    
        
        NetInfo_tab_in_handler();
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_SECURITY])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_SNMP])
    {
        DMSG(1,"");
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_CABLE_TEST])
    {
        DMSG(1,"");
    }
    
}

static void _out_handler_net_main(gint page)
{
    if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_IPSETUP])
    {
        DMSG(1, "IpSetup_tab_out_handler");        
        
        IpSetup_Main_tab_out_handler(); 
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS])
    {
        DMSG(1, "Ddns_tab_out_handler");        
        
        Ddns_tab_out_handler(); 
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_EMAIL])
    {
        DMSG(1, "Email_tab_out_handler");        
        
        Email_tab_out_handler();
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_NETSTAT])
    {
        DMSG(1, "NetInfo_tab_out_handler");        
        
        NetInfo_tab_out_handler(); 
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_SECURITY])
    {
        DMSG(1, "NetSecurity_tab_out_handler");        
        
        NetSecurity_tab_out_handler();
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_SNMP])
    {
        DMSG(1, "NetSnmp_tab_out_handler");

        NetSnmp_tab_out_handler();
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_CABLE_TEST])
    {
        DMSG(1, "NetCableTest_tab_out_handler");

        NetCableTest_tab_out_handler();
    }
    else if (page == mcf.sys_sub5.menu_pos[SYS_SUB5_RTP])
    {
        NetRtp_tab_out_handler();
    }
    
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	NFOBJECT *nftab = NULL;
	gint cur_page;

	nftab = nftool_get_nftab_from_setup_window(top);
	if(!nftab)	return FALSE;

	cur_page = nfui_nftab_get_cur_page((NFTAB*)nftab);

	if(cur_page < 0 || cur_page >= ((NFTAB*)nftab)->pages)
		return FALSE;

    _out_handler_net_main(cur_page);

	return TRUE;
}
static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if(sysnet_get_changeflag())
			DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);

		vsm_live_start();

		g_curwnd = 0;

		// SKSHIN
		gtk_main_quit();
	}

	return FALSE;
}


static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type)
	{
	case NFEVENT_TAB_BEFORE_CHANGE:
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

        _out_handler_net_main(cur_page);
        _in_handler_net_main(cur_page);
	break;

	default: break;
	}

	return FALSE;
}

static gint _check_init_page(gint page)
{
    gint tmp = page;

    if (ivsc.dfunc.support_protect == 0) return tmp;
    if (DAL_get_agr_policy() == 1) return tmp;

    if (mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS] == page)
    {
        tmp = 0;
    }

    return tmp;
}

static gint _check_device_policy(NFOBJECT *nftab, gint page)
{
    gint pos;

    if (ivsc.dfunc.support_protect == 0) return -1;
    if (DAL_get_agr_policy() == 1) return -1;

    vw_provide_devinfo_notice2_open(g_curwnd);

    if (DAL_get_agr_policy() == 1) return -1;
    if (!g_curwnd) return -1;       //for auto logout

    pos = mcf.sys_sub5.menu_pos[SYS_SUB5_DDNS];	
	if (pos != -1) nfui_nftab_unregi_page((NFTAB*)nftab, pos);

    nfui_signal_emit(nftab, GDK_EXPOSE, TRUE);
	nfui_make_key_hierarchy(g_curwnd);
	nfui_set_key_focus(nftab, TRUE);

    return 0;
}

void sysnet_set_changeflag(guint flag)
{
	net_change_flag = flag;
}


guint sysnet_get_changeflag()
{
	return net_change_flag;
}


void SystemSetupNetwork_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page ) //wiggls - otm
{
    NFOBJECT *net_wnd, *nftab;

    DMSG(1, "SystemSetupNetwork_Open");

    vsm_live_stop();
    sysnet_set_changeflag(0);

    page = _check_init_page(page);

    net_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_NETWORK, page);
    g_curwnd = net_wnd;
    nfui_nfwindow_set_title(net_wnd, "SYSTEM SETUP - NETWORK");
    nftab = nftool_get_nftab_from_setup_window(net_wnd);
    nfui_nftab_set_cur_page(nftab, page ); // otm

    _init_net_main(nftab, page);

    nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
    nfui_regi_pre_event_callback(net_wnd, pre_main_wnd_event_handler);
    nfui_nfwindow_set_returnkey_proc(net_wnd, returnkey_proc);
    
    nfui_nfobject_show(net_wnd);
    nfui_make_key_hierarchy(net_wnd);

    nfui_set_key_focus(nftab, TRUE);

	_check_device_policy(nftab, page);

    gtk_main();

    DMSG(1, "SystemSetupNetwork_Open EXIT");                
}



void SystemSetupNetwork_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(obj);

	nfui_nfobject_hide(topwin);

	nftool_destroy_setup_window(topwin);
}




