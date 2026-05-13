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

#include "vw_sys_main.h"
#include "vw_date_time.h"
#include "vw_system_management.h"
#include "vw_date_time_main.h"
#include "vw_system_info.h"
#include "vw_control_dev.h"
#include "vw_supported_ipcam.h"
#include "vw_sys_license.h"
#include "vsm.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static guint sys_change_flag = 0;

static void _init_sys_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_DATETIME]; 
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysDateTime_page");
    
        init_DateTime_Main_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_MANAGEMENT];   
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SystemManage_Page");
        
        VW_Init_SystemManage_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_INFORMATION];  
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SystemInfo_Page");
        
        VW_Init_SystemInfo_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_CONTROLDEVICE];    
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysControlDev_page");
        
        VW_Init_SysControlDev_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_SUPPORT_IPCAM];    
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysSupporteIPCam_page");
        
        VW_Init_SysSupporteIPCam_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_SECURITY]; 
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysSecurity_page");
        
        VW_Init_SysSecurity_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM]; 
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysPOSATM_page");
        
        VW_Init_SysPOSATM_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.sys_sub6.menu_pos[SYS_SUB6_LICENSE]; 
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SysLicense_page");
        
        VW_Init_SysLicense_page(((NFTAB*)nftab)->page[pos]);
    }    
}

static void _in_handler_sys_main(gint page)
{
    if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_DATETIME])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_MANAGEMENT])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_INFORMATION])
    {
        DMSG(1, "VW_SystemInfo_tab_in_handler");    
        
        VW_SystemInfo_tab_in_handler();    
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_CONTROLDEVICE])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_SUPPORT_IPCAM])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_SECURITY])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM])
    {
        DMSG(1, "");    
    }    
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_LICENSE])
    {
        DMSG(1, "");    
    }    
}

static void _out_handler_sys_main(gint page)
{
    if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_DATETIME])
    {
        DMSG(1, "VW_DateTime_tab_out_handler");        
    
        VW_DateTime_tab_out_handler(); 
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_MANAGEMENT])
    {
        DMSG(1, "VW_SystemManage_tab_out_handler");        

        VW_SystemManage_tab_out_handler();
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_INFORMATION])
    {
        DMSG(1, "");            
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_CONTROLDEVICE])
    {
        DMSG(1, "VW_ControlDev_tab_out_handler");        
    
        VW_ControlDev_tab_out_handler();
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_SUPPORT_IPCAM])
    {
        DMSG(1, "VW_SupportedIPCam_tab_out_handler");        
    
        VW_SupportedIPCam_tab_out_handler();
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_SECURITY])
    {
        DMSG(1, "VW_Security_tab_out_handler");        
    
        VW_Security_tab_out_handler();
    }
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM])
    {
        DMSG(1, "VW_SysPOSATM_tab_out_handler");        
    
        VW_SysPOSATM_tab_out_handler();    
    }    
    else if (page == mcf.sys_sub6.menu_pos[SYS_SUB6_LICENSE])
    {
        DMSG(1, "VW_SysLicense_tab_out_handler");        
    
        VW_SysLicense_tab_out_handler();    
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


    if (cur_page == mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM])
    {
        if (!VW_SysPOSATM_tab_out_prepare())
        {
            return TRUE;
        }
    }

    _out_handler_sys_main(cur_page);
	
	return TRUE;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if(VW_SetupSystem_get_changeflag())
		{
			DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
			DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);
			DAL_save_setup_db(NFSETUP_WINDOW_USER);
			DAL_save_setup_db(NFSETUP_WINDOW_SOUND);
			DAL_save_setup_db(NFSETUP_WINDOW_RECORDING);
		}

		vsm_live_start();

		g_curwnd = 0;

		gtk_main_quit();

	}

	return FALSE;
}

static gboolean post_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type)
	{
		case NFEVENT_TAB_BEFORE_CHANGE:
			cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
			new_page = nfui_nftab_get_new_page((NFTAB*)obj);

			if(cur_page == new_page)	return FALSE;

            if (cur_page == mcf.sys_sub6.menu_pos[SYS_SUB6_POSATM])
            {
                if (!VW_SysPOSATM_tab_out_prepare())
                {
                    nfui_nftab_set_new_page((NFTAB*)obj, cur_page);
                    return FALSE;
                }
            }

            _out_handler_sys_main(cur_page);
            _in_handler_sys_main(new_page);
		break;

		default:
		break;
	}

	return FALSE;
}

void VW_SetupSystem_set_changeflag(guint flag)
{
	sys_change_flag = flag;
}

guint VW_SetupSystem_get_changeflag()
{
	return sys_change_flag;
}

void VW_SetupSystem_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page ) //wiggls - otm
{
    NFOBJECT *sys_wnd, *fixed, *nftab;

    DMSG(1, "VW_SetupSystem_Open");

    vsm_live_stop();
    VW_SetupSystem_set_changeflag(0);

    sys_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_SYSTEM, page);
    g_curwnd = sys_wnd;
    nfui_nfwindow_set_title(sys_wnd, "SYSTEM SETUP - SYS MAIN");
    nftab = nftool_get_nftab_from_setup_window(sys_wnd);
    nfui_nftab_set_cur_page(nftab, page ); // otm

    _init_sys_main(nftab, page);

    nfui_regi_post_event_callback(nftab, post_nftab_event_handler);
    nfui_regi_post_event_callback(sys_wnd, post_main_wnd_event_handler);
    nfui_nfwindow_set_returnkey_proc(sys_wnd, returnkey_proc);
    
    nfui_nfobject_show(sys_wnd);
    nfui_make_key_hierarchy(sys_wnd);

    nfui_set_key_focus(nftab, TRUE);
    
    gtk_main();

    DMSG(1, "VW_SetupSystem_Open EXIT");    
}



void VW_SetupSystem_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(obj);
	nfui_nfobject_hide(topwin);
	nftool_destroy_setup_window(topwin);
}



