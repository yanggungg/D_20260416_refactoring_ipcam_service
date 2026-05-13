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

#include "vw_evt_act_main.h"
#include "vw_alarm_out_evt.h"
#include "vw_event_noti_evt.h"
#include "vw_alarm_sensor_evt.h"
#include "vw_motion_sensor_evt.h"
#include "vw_video_loss_evt.h"
#include "vw_system_evt.h"
#include "vw_textin_evt.h"
#include "vw_s1_vca_evt.h"

#include "vsm.h"
#include "iux_afx.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;

static gboolean g_eaChanged = FALSE;


static void _init_evt_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMOUT]; 
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_AlarmOutEvt_Page");
    
        VW_Init_AlarmOutEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_EVENTNOTI];        
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_EvtNotiEvt_Page");
        
        VW_Init_EvtNotiEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMSENSOR];      
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_AlarmSensorEvt_Page");
    
        VW_Init_AlarmSensorEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_MOTIONSENSOR];     
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_MotionSensorEvt_Page");
    
        VW_Init_MotionSensorEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_VIDEOLOSS];        
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_VideoLossEvt_Page");
        
        VW_Init_VideoLossEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_SYSTEMEVENT];      
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_SystemEvt_Page");
        
        VW_Init_SystemEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_VCA_EVENT_S1];     
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_S1_VCA_Evt_Page");
        
        VW_Init_S1_VCA_Evt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_POSATM_EVENT];	
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_POSATMEvt_Page");
        
        VW_Init_POSATMEvt_Page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub8.menu_pos[SYS_SUB8_ANALYSIS_EVENT_ITX];	
    if (pos != -1) 
    {
        DMSG(1, "VW_Init_Analysis_Evt_Page");
        
        VW_Init_Analysis_Evt_Page(((NFTAB*)nftab)->page[pos]);
    }    
}

static void _in_handler_evt_main(gint page)
{
    if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMOUT])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_EVENTNOTI])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMSENSOR])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_MOTIONSENSOR])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_VIDEOLOSS])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_SYSTEMEVENT])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_VCA_EVENT_S1])
    {
        DMSG(1, "");        
    }    
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_POSATM_EVENT])
    {
        DMSG(1, "");    
    }    
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ANALYSIS_EVENT_ITX])
    {
        DMSG(1, "");        
    }
}

static void _out_handler_evt_main(gint page)
{
    if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMOUT])
    {
        DMSG(1, "VW_AlarmOut_tab_out_handler");
    
        VW_AlarmOut_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_EVENTNOTI])
    {
        DMSG(1, "VW_EvtNotiEvt_tab_out_handler");

        VW_EvtNotiEvt_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ALARMSENSOR])
    {
        DMSG(1, "VW_AlarmSensorEvt_tab_out_handler");

        VW_AlarmSensorEvt_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_MOTIONSENSOR])
    {
        DMSG(1, "VW_MotionSensorEvt_tab_out_handler");

        VW_MotionSensorEvt_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_VIDEOLOSS])
    {
        DMSG(1, "VW_VideoLossEvt_tab_out_handler");

        VW_VideoLossEvt_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_SYSTEMEVENT])
    {
        DMSG(1, "VW_SystemEvt_tab_out_handler");

        VW_SystemEvt_tab_out_handler();
    }
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_VCA_EVENT_S1])
    {
        DMSG(1, "VW_S1_VCA_Evt_tab_out_handler");

        VW_S1_VCA_Evt_tab_out_handler();
    }    
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_POSATM_EVENT])
    {
        DMSG(1, "VW_POSATMEvt_tab_out_handler");

        VW_POSATMEvt_tab_out_handler();
    }   
    else if (page == mcf.sys_sub8.menu_pos[SYS_SUB8_ANALYSIS_EVENT_ITX])
    {
        DMSG(1, "VW_Analysis_Evt_tab_out_handler");

        VW_Analysis_Evt_tab_out_handler();
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

    _out_handler_evt_main(cur_page);

	return TRUE;
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if(event_act_data_is_changed())
			DAL_save_setup_db(NFSETUP_WINDOW_EVENT);

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

            _out_handler_evt_main(cur_page);
            _in_handler_evt_main(new_page);
		break;

		default:
		break;
	}

	return FALSE;
}

void event_act_data_changed(gboolean change)
{
	g_eaChanged = change;
}

gboolean event_act_data_is_changed()
{
	return g_eaChanged;
}

void change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}


void VW_Evt_Act_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page)
{
	NFOBJECT *evtsen_wnd, *fixed, *nftab;

    DMSG(1, "VW_Evt_Act_Open");

	vsm_live_stop();
	event_act_data_changed(FALSE);

	evtsen_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_EVENT, page);
	g_curwnd = evtsen_wnd;
	nfui_nfwindow_set_title(evtsen_wnd, "SYSTEM SETUP - EVENT");
	nftab = nftool_get_nftab_from_setup_window(evtsen_wnd);
	nfui_nftab_set_cur_page(nftab, page ); // otm

    _init_evt_main(nftab, page);

	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
	nfui_regi_pre_event_callback(evtsen_wnd, pre_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(evtsen_wnd, returnkey_proc);
	
	nfui_nfobject_show(evtsen_wnd);
	nfui_make_key_hierarchy(evtsen_wnd);

	nfui_set_key_focus(nftab, TRUE);

	gtk_main();

    DMSG(1, "VW_Evt_Act_Open EXIT");    
}

void VW_Evt_Act_Destroy(NFOBJECT *object)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(object);
	nfui_nfobject_hide(topwin);
	if(topwin)
		nftool_destroy_setup_window(topwin);
}

