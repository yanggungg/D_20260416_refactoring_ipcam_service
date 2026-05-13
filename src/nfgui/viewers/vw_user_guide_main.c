
#include "nf_afx.h"

#include "tools/nf_ui_tool.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfwindow.h"

#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "vw_user_guide_main.h"
#include "vw_menu.h"

#include "vsm.h"


static NFWINDOW *g_curwnd = 0;


static void _init_user_guide_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB1];	
	if (pos != -1) 
	{
	    init_UserGuide_tab1_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB2];	
	if (pos != -1) 
	{
	    init_UserGuide_tab2_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB3];	
	if (pos != -1) 
	{
	    init_UserGuide_tab3_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB4];	
	if (pos != -1) 
	{
	    init_UserGuide_tab4_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB5];	
	if (pos != -1) 
	{
	    init_UserGuide_tab5_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB6];	
	if (pos != -1) 
	{
	    init_UserGuide_tab6_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB7];	
	if (pos != -1) 
	{
	    init_UserGuide_tab7_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB8];	
	if (pos != -1) 
	{
	    init_UserGuide_tab8_page(((NFTAB*)nftab)->page[pos]);
    }    

    pos = mcf.userguide.menu_pos[USERGUIDE_SUB_TAB9];	
	if (pos != -1) 
	{
	    init_UserGuide_tab9_page(((NFTAB*)nftab)->page[pos]);
    }     
}

static void _in_handler_user_guide_main(gint page)
{
    if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB1])
    {
        UserGuide_tab1_in_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB2])
    {
        UserGuide_tab2_in_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB3])
    {
        UserGuide_tab3_in_handler();
    }   
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB4])
    {
        UserGuide_tab4_in_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB5])
    {
        UserGuide_tab5_in_handler();
    }    
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB6])
    {
        UserGuide_tab6_in_handler();
    } 
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB7])
    {
        UserGuide_tab7_in_handler();
    } 
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB8])
    {
        UserGuide_tab8_in_handler();
    } 
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB9])
    {
        UserGuide_tab9_in_handler();
    }     
}

static void _out_handler_user_guide_main(gint page)
{
    if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB1])
    {
        UserGuide_tab1_out_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB2])
    {
        UserGuide_tab2_out_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB3])
    {
        UserGuide_tab3_out_handler();
    }   
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB4])
    {
        UserGuide_tab4_out_handler();
    }
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB5])
    {
        UserGuide_tab5_out_handler();
    }  
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB6])
    {
        UserGuide_tab6_out_handler();
    }   
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB7])
    {
        UserGuide_tab7_out_handler();
    }   
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB8])
    {
        UserGuide_tab8_out_handler();
    }   
    else if (page == mcf.userguide.menu_pos[USERGUIDE_SUB_TAB9])
    {
        UserGuide_tab9_out_handler();
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

	_out_handler_user_guide_main(cur_page);

	return TRUE;
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		vsm_live_start();
		g_curwnd = 0;

		gtk_main_quit();
	}
	else if(evt->type == WND_PRE_CLOSE) {

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

			_out_handler_user_guide_main(cur_page);
			_in_handler_user_guide_main(new_page);			
		break;

		default:
		break;
	}

	return FALSE;
}

void UserGuide_Open(NFWINDOW *parent) 
{
	NFOBJECT *wnd, *fixed, *nftab;

	vsm_live_stop();

	wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_USER_GUIDE, 0);
	nfui_nfwindow_set_title(wnd, "USER GUIDE MAIN");
	nftab = nftool_get_nftab_from_setup_window(wnd);
	nfui_nftab_set_cur_page((NFTAB*)nftab, 0); 
	g_curwnd = wnd;

	_init_user_guide_main(nftab, 0);

	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
	nfui_regi_pre_event_callback(wnd, pre_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(wnd, returnkey_proc);

	nfui_nfobject_show(wnd);
	nfui_make_key_hierarchy(wnd);

	nfui_set_key_focus(nftab, TRUE);

	gtk_main();
}


void UserGuide_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin = NULL;

	topwin = nfui_nfobject_get_top(obj);
	
	if(topwin) 
		nftool_destroy_setup_window(topwin);
}



