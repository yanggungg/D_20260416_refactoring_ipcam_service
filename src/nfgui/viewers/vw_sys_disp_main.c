
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

#include "vw_sys_disp_main.h"
#include "vw_sys_disp_osd.h"
#include "vw_sys_disp_monitor.h"
//#include "vw_sys_disp_sequence.h"
#include "vw_sys_disp_posatm.h"
#include "vw_sys_disp_dual_advanced.h"
#include "vsm.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static guint g_msg_tmr = 0;

static guint disp_change_flag = 0;


static void _init_disp_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_OSD];  
    if (pos != -1) 
    {
        DMSG(1, "init_DispOsd_page");
    
        init_DispOsd_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_MONITOR];          
    if (pos != -1) 
    {
        DMSG(1, "init_DispMonitor_page");
        
        init_DispMonitor_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_SEQUENCE];         
    if (pos != -1) 
    {
        DMSG(1, "init_DispSequence_page");
    
        init_DispSequence_page(((NFTAB*)nftab)->page[pos]);
    }
    
    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_SPOTOUT];  
    if (pos != -1) 
    {
        DMSG(1, "init_DispSpot_page");
    
        init_DispSpot_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_POSATM];
    if (pos != -1) 
    {
        DMSG(1, "init_DispPOSATM_page");
    
        init_DispPOSATM_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub2.menu_pos[SYS_SUB2_ADVANCED_DUAL];
    if (pos != -1) 
    {
        DMSG(1, "init_DispAdvancedDual_page");
    
        init_DispAdvancedDual_page(((NFTAB*)nftab)->page[pos]);
    }    
}

static void _in_handler_disp_main(gint page)
{
    if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_OSD])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_MONITOR])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_SEQUENCE])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_SPOTOUT])
    {
        DMSG(1, "");
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_POSATM])
    {
        DMSG(1, "");
    }    
}

static void _out_handler_disp_main(gint page)
{
    if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_OSD])
    {
        DMSG(1, "Osd_tab_out_handler");
        
        Osd_tab_out_handler(); 
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_MONITOR])
    {
        DMSG(1, "Monitor_tab_out_handler");
        
        Monitor_tab_out_handler();
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_SEQUENCE])
    {
        DMSG(1, "Sequence_tab_out_handler");
        
        Sequence_tab_out_handler();
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_SPOTOUT])
    {
        DMSG(1, "Spot_tab_out_handler");
    
//#if !defined(_IPX_MODEL_UX)    
        Spot_tab_out_handler();
//#endif        
    }
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_POSATM])
    {
        DMSG(1, "DispPOSATM_tab_out_handler");
        
        DispPOSATM_tab_out_handler();
    }    
    else if (page == mcf.sys_sub2.menu_pos[SYS_SUB2_ADVANCED_DUAL])
    {
        DMSG(1, "DispAdvancedDual_tab_out_handler");
        
        DispAdvancedDual_tab_out_handler();
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

	_out_handler_disp_main(cur_page);

	return TRUE;
}


static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		guint i;

        if (g_msg_tmr) {
            g_source_remove(g_msg_tmr);
            g_msg_tmr = 0;
        }

		if(sysdisp_get_changeflag()) 
			DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);

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

			_out_handler_disp_main(cur_page);
			_in_handler_disp_main(new_page);
		break;

		default:
		break;
	}

	return FALSE;
}

void sysdisp_set_changeflag(guint flag)
{
	disp_change_flag = flag;
}

guint sysdisp_get_changeflag()
{
	return disp_change_flag;
}

void SystemSetupDisp_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page) 
{
    NFOBJECT *disp_wnd, *fixed, *nftab;

    DMSG(1, "SystemSetupDisp_Open");

    vsm_live_stop();
    sysdisp_set_changeflag(0);

    disp_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_DISPLAY, page);
    g_curwnd = disp_wnd;
    nfui_nfwindow_set_title(disp_wnd, "SYSTEM SETUP - DISPLAY");
    nftab = nftool_get_nftab_from_setup_window(disp_wnd);
    nfui_nftab_set_cur_page((NFTAB*)nftab, page ); // otm

    _init_disp_main(nftab, page);

    nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
    nfui_regi_pre_event_callback(disp_wnd, pre_main_wnd_event_handler);
    nfui_nfwindow_set_returnkey_proc(disp_wnd, returnkey_proc);

    nfui_nfobject_show(disp_wnd);
    nfui_make_key_hierarchy(disp_wnd);

    nfui_set_key_focus(nftab, TRUE);

    gtk_main();

    DMSG(1, "SystemSetupDisp_Open EXIT");           
}

static gboolean _destory_setup_window(gpointer data)
{
    NFOBJECT *topwin = (NFOBJECT*)data;

  	nftool_remove_waitbox(wait_pop);
	wait_pop = NULL;
    nftool_destroy_setup_window(topwin);
    g_msg_tmr = 0;

    return FALSE;
}

void SystemSetupDisp_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin = NULL;

	topwin = nfui_nfobject_get_top(obj);
	if (topwin)
	{
	    nftool_destroy_setup_window(topwin);
	}
/*
    if(sysdisp_get_changeflag())
    {   
        wait_pop = nftool_mbox_wait(topwin, "WAIT", "Using the system is restricted temporarily while setting is being saved.\nPlease wait.");  
        g_msg_tmr = g_timeout_add(1000,_destory_setup_window, topwin);   
    }
    else{
        nftool_destroy_setup_window(topwin);	
    }
*/
}












