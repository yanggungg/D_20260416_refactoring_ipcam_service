#include <string.h>

#include "nf_afx.h"

#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfwindow.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

//#include "nf_ui_user_main.h"
//#include "nf_ui_userpwd.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_operation.h"
#include "vw_record_continuous.h"
#include "vw_record_motion.h"
#include "vw_record_alarm.h"
#include "vw_record_panic.h"
#include "vw_record_net_streaming.h"
#include "vw_record_data_internal.h"
#include "vw_record_combo_all_popup.h"
#include "vw_record_auto_popup.h"
#include "vw_record_auto_intensive_popup.h"
#include "vw_record_param_sub.h"
#include "vw_record_calcul.h"

#include "scm.h"
#include "smt.h"

#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *rec_wnd;
static NFOBJECT *nftab;
static NFOBJECT *tab_page[REC_SUB_CNT];

static gboolean isChanged = FALSE;

static gint ptz_cam[GUI_CHANNEL_CNT];

////////////////////////////////////////////////////////////
//
// private functions
//

static void _init_rec_main(NFOBJECT *nftab)
{
    gint pos;

    pos = mcf.rec.menu_pos[REC_SUB_OPER];       
    if (pos != -1) {
        DMSG(1, "VW_Init_RecOperation_Page");
    
        if (pos != -1) VW_Init_RecOperation_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_CONT];       
    if (pos != -1) {
        DMSG(1, "VW_Init_RecContinuous_Page");

        if (pos != -1) VW_Init_RecContinuous_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_MOT];        
    if (pos != -1) {
        DMSG(1, "VW_Init_RecMotion_Page");

        if (pos != -1) VW_Init_RecMotion_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_ALARM];      
    if (pos != -1) {
        DMSG(1, "VW_Init_RecAlarm_Page");

        if (pos != -1) VW_Init_RecAlarm_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_PANIC];      
    if (pos != -1) {
        DMSG(1, "VW_Init_RecPanic_Page");

        if (pos != -1) VW_Init_RecPanic_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_NETSTREAM];      
    if (pos != -1) {
        DMSG(1, "VW_Init_NetStream_Page");

        if (pos != -1) VW_Init_NetStream_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }

    pos = mcf.rec.menu_pos[REC_SUB_CALCUL];     
    if (pos != -1) {
        DMSG(1, "VW_Init_StorageCalcul_Page");

        if (pos != -1) VW_Init_StorageCalcul_Page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB*)nftab)->page[pos]; 
    }
}

static void _in_handler_rec_main(gint page)
{
    if (page == mcf.rec.menu_pos[REC_SUB_OPER])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_CONT])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_MOT])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_ALARM])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_PANIC])
    {
        DMSG(1, "");        
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_NETSTREAM])
    {
        DMSG(1, "");        
    }    
    else if (page == mcf.rec.menu_pos[REC_SUB_CALCUL])
    {
        DMSG(1, "VW_StorageCalcul_tab_in_handler");

        VW_StorageCalcul_tab_in_handler();
    } 
}

static void _out_handler_rec_main(gint page)
{
    if (page == mcf.rec.menu_pos[REC_SUB_OPER])
    {
        DMSG(1, "VW_RecOperation_tab_out_handler");
        
        VW_RecOperation_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_CONT])
    {
        DMSG(1, "VW_RecContinuous_tab_out_handler");

        VW_RecContinuous_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_MOT])
    {
        DMSG(1, "VW_RecMotion_tab_out_handler");
        
        VW_RecMotion_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_ALARM])
    {
        DMSG(1, "VW_RecAlarm_tab_out_handler");
        
        VW_RecAlarm_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_PANIC])
    {
        DMSG(1, "VW_RecPanic_tab_out_handler");
        
        VW_RecPanic_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_NETSTREAM])
    {
        DMSG(1, "VW_NetStream_tab_out_handler");
        
        VW_NetStream_tab_out_handler();
    }
    else if (page == mcf.rec.menu_pos[REC_SUB_CALCUL])
    {
        DMSG(1, "VW_StorageCalcul_tab_out_handler");
        
        VW_StorageCalcul_tab_out_handler();
    }       
}

static void _set_range_record_param_data()
{
	vw_record_check_capable_param_data();
	vw_record_set_capable_param_data();
	vw_record_refresh_param_data();
}

static void _set_ptz_channel()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
        ptz_cam[i] = scm_ipcam_is_ptz(i);
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

    _out_handler_rec_main(cur_page);
	
	return TRUE;
}



static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_PND_NOTIFY)
	{
		NF_NOTIFY_INFO *notify_info = (NF_NOTIFY_INFO *)data;
/*
		// pnd type of "ipx_cam_api.h"
		if ((notify_info->d.params[0] == 0) || (notify_info->d.params[0] == 7))
		{
			_set_range_record_param_data();

			VW_RecComboAll_Popup_Close();
			VW_RecAutoPopup_Close();
			VW_RecAutoItsPopup_Close();
			VW_RecParam_Submenu_Close();
		}
*/		
	}
	else if(evt->type == GDK_DELETE)
	{
		if(isChanged) {
			DAL_save_setup_db(NFSETUP_WINDOW_RECORDING);

			isChanged = FALSE;
		}

		remove_oper_data();
		remove_continuous_data();
		remove_motion_data();
		remove_alarm_data();
		remove_panic_data();
		remove_netstream_data();
		remove_auto_data();

		g_curwnd = 0;
	}

	return FALSE;
}


static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
//		uxm_unreg_imsg_event(obj, INFY_PND_NOTIFY);

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
		{
			cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
			new_page = nfui_nftab_get_new_page((NFTAB*)obj);

			if(cur_page == new_page)	return FALSE;

            _out_handler_rec_main(cur_page);
			_in_handler_rec_main(new_page);

			if (nfui_nftab_is_valid_page((NFTAB*)obj, new_page) == 0)
			{
				nfui_nftab_set_new_page((NFTAB*)obj, 0);
				nfui_signal_emit(tab_page[0], GDK_EXPOSE, TRUE);
		    }            
		}
		break;

	default:
		break;
	}
	return FALSE;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

void VW_RecordSetup_Open(NFWINDOW *parent)
{
	guint i;
	RecordOperData oper_data;
	
    DMSG(1, "VW_RecordSetup_Open");
    
	DAL_get_recordOper_data(&oper_data);

	rec_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_RECORDING, 0);
	g_curwnd = rec_wnd;
	nfui_nfwindow_set_title(rec_wnd, "RECORD SETUP");

	nftab = nftool_get_nftab_from_setup_window(rec_wnd);

	init_trans_data();

	init_oper_data();
	init_continuous_data();
	init_motion_data();
	init_alarm_data();
	init_panic_data();
    init_netstream_data();
	init_auto_data();

    _init_rec_main(nftab);

	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
	nfui_regi_pre_event_callback(rec_wnd, pre_main_wnd_event_handler);
	nfui_regi_post_event_callback(rec_wnd, post_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(rec_wnd, returnkey_proc);

	nfui_nfobject_show(rec_wnd);
	nfui_make_key_hierarchy(rec_wnd);

	nfui_set_key_focus(nftab, TRUE);

	if (oper_data.mode == AUTO_CONFIG)
	{
		for (i = 1; i < 4; i++)
			nfui_nftab_unregi_page((NFTAB*)nftab,  i);
	}

    _set_ptz_channel();
	_set_range_record_param_data();

//	uxm_reg_imsg_event(rec_wnd, INFY_PND_NOTIFY);
	smt_set_service(SMT_RECORD_SETUP);	

	gtk_main();

	smt_set_service(SMT_LIVE);	

    DMSG(1, "VW_RecordSetup_Open EXIT");    
}



void VW_RecordSetup_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin = NULL;

	topwin = nfui_nfobject_get_top(obj);
	if(topwin) {
		nftool_destroy_setup_window(topwin);
	}
}

void set_changed_record_data()
{
	if(!isChanged) 
		isChanged = TRUE;
}


void VW_RecordSetup_change_config_mode(guint mode)
{
	NFOBJECT* cur_focus;
	gint pos;

	if (mode == MANUAL_CONFIG)
	{
        pos = mcf.rec.menu_pos[REC_SUB_CONT];		
    	if (pos != -1) nfui_nftab_regi_page((NFTAB*)nftab, tab_page[pos], pos);

        pos = mcf.rec.menu_pos[REC_SUB_MOT];		
    	if (pos != -1) nfui_nftab_regi_page((NFTAB*)nftab, tab_page[pos], pos);

        pos = mcf.rec.menu_pos[REC_SUB_ALARM];		
    	if (pos != -1) nfui_nftab_regi_page((NFTAB*)nftab, tab_page[pos], pos);
	}
	else
	{
        pos = mcf.rec.menu_pos[REC_SUB_CONT];		
    	if (pos != -1) nfui_nftab_unregi_page((NFTAB*)nftab, pos);

        pos = mcf.rec.menu_pos[REC_SUB_MOT];		
    	if (pos != -1) nfui_nftab_unregi_page((NFTAB*)nftab, pos);

        pos = mcf.rec.menu_pos[REC_SUB_ALARM];		
    	if (pos != -1) nfui_nftab_unregi_page((NFTAB*)nftab, pos);
	}

	cur_focus = nfui_get_cur_focus(rec_wnd);

	if (cur_focus)
	{
		nfui_set_key_focus(cur_focus, FALSE);
		nfui_set_key_focus(nftab, TRUE);

		nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
		nfui_signal_emit(nftab, GDK_EXPOSE, TRUE);
		nfui_make_key_hierarchy(rec_wnd);
	}
}

gboolean vw_send_notify_change_record_data()
{
    DAL_notify_fire_DB_change(NF_SYSDB_CATE_REC);
}

gint get_ptz_channel(gint ch)
{
    return ptz_cam[ch];
}

