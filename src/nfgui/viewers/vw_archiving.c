#include "vw_archiving.h"

#include "../support/color.h"
#include "objects/nftab.h"
#include "vw_arch_tab1.h"
#include "vw_arch_tab2.h"
#include "vw_arch_tab3.h"
#include "vw_arch_tab4.h"
#include "objects/nfthumbnail.h"
#include "smt.h"

#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


// [ Private Member Method and Function ]

typedef struct _ARCH_T {
	LIVESTART_T		*lst;
	NFOBJECT 		*wnd;
} ARCH_T;

static NFWINDOW *g_curwnd = 0;
static ARCH_T iarch;


static void _init_arch_main(NFOBJECT *nftab)
{
    gint pos;

    pos = mcf.arch.menu_pos[ARCH_SUB_NEW_ARCH];		
	if (pos != -1) {
        DMSG(1, "vw_init_arch_tab1_page");
    
	    vw_init_arch_tab1_page(((NFTAB*)nftab)->page[pos]);
    }
    
    pos = mcf.arch.menu_pos[ARCH_SUB_RESERVED];			
	if (pos != -1) {
        DMSG(1, "vw_init_arch_tab2_page");

	    vw_init_arch_tab2_page(((NFTAB*)nftab)->page[pos]);
    	nfui_nfobject_hide(((NFTAB*)nftab)->page[pos]);
    }
    
    pos = mcf.arch.menu_pos[ARCH_SUB_DATA_PB];			
	if (pos != -1) {
        DMSG(1, "vw_init_arch_tab3_page");
    
	    vw_init_arch_tab3_page(((NFTAB*)nftab)->page[pos]);
    	nfui_nfobject_hide(((NFTAB*)nftab)->page[pos]);
    }
    
    pos = mcf.arch.menu_pos[ARCH_SUB_DEV_SETUP];			
	if (pos != -1) {
        DMSG(1, "vw_init_arch_tab4_page");
        
	    vw_init_arch_tab4_page(((NFTAB*)nftab)->page[pos]);
    	nfui_nfobject_hide(((NFTAB*)nftab)->page[pos]);
    }
}

static void _in_handler_arch_main(gint page)
{
    if (page == mcf.arch.menu_pos[ARCH_SUB_NEW_ARCH])
    {
        DMSG(1, "vw_arch_tab1_in_handler");
    
        vw_arch_tab1_in_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_RESERVED])
    {
        DMSG(1, "vw_arch_tab2_in_handler");    

        vw_arch_tab2_in_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_DATA_PB])
    {
        DMSG(1, "vw_arch_tab3_in_handler");    

        vw_arch_tab3_in_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_DEV_SETUP])
    {
        DMSG(1, "");    
    }   
}

static void _out_handler_arch_main(gint page)
{
    if (page == mcf.arch.menu_pos[ARCH_SUB_NEW_ARCH])
    {
        DMSG(1, "vw_arch_tab1_out_handler");    
    
        vw_arch_tab1_out_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_RESERVED])
    {
        DMSG(1, "vw_arch_tab2_out_handler");    

        vw_arch_tab2_out_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_DATA_PB])
    {
        DMSG(1, "vw_arch_tab3_out_handler");    

        vw_arch_tab3_out_handler();
    }
    else if (page == mcf.arch.menu_pos[ARCH_SUB_DEV_SETUP])
    {
        DMSG(1, "vw_arch_tab4_out_handler");    

        vw_arch_tab4_out_handler();
    }   
}

static int _wait_for_thumb_stop()
{
	nfui_nfthumbnail_image_close();

	while (1) {
		usleep(100000);
		if (!nfui_nfthumbnail_check_running()) break;
	}
	return 0;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) 
	{		
		if (iarch.lst) 
		{
			iarch.lst->start();
			vsm_destroy_livestart_obj(iarch.lst);
		}

		scm_end_query();
		g_curwnd = 0;
		gtk_main_quit();
	}
	else if(evt->type == WND_PRE_CLOSE) {
		_wait_for_thumb_stop();
	}

	return FALSE;
}

static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type) {
	case NFEVENT_TAB_BEFORE_CHANGE:
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

        _out_handler_arch_main(cur_page);
        _in_handler_arch_main(new_page);
	default:
		break;
	}
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    g_message("%s, %d", __FUNCTION__, __LINE__);
	
	_wait_for_thumb_stop();

	return TRUE;
}

// [ Public Member Function and Method ]

void VW_Archiving_Open(NFWINDOW *parent, LIVESTART_T *lst, int from_pb)
{
	NFOBJECT *fixed, *nftab;

    DMSG(1, "VW_Archiving_Open");

	if (lst == NULL)	
	{
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		g_assert(0);
	}

    if (ivsc.dfunc.support_protect)
    {
        if (scm_is_clon_device() == 0) {
            vw_unblock_authcode_popup_open(parent);

			lst->start();
			vsm_destroy_livestart_obj(lst);
			return;			
        }
    }
	
 	iarch.lst = lst;

	iarch.wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_ARCHIVING, 0);
	g_curwnd = iarch.wnd;
	nfui_nfwindow_set_title(iarch.wnd, "ARCHIVING");
	nftab = nftool_get_nftab_from_setup_window(iarch.wnd);

    _init_arch_main(nftab);

	nfui_regi_post_event_callback(iarch.wnd, post_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(iarch.wnd, returnkey_proc);		
	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);

	nfui_nfobject_show(iarch.wnd);
	nfui_make_key_hierarchy(iarch.wnd);
	nfui_set_key_focus(nftab, TRUE);

	if (from_pb) vw_arch_tab1_show();

	smt_set_service(SMT_ARCHIVE);	

	gtk_main();

	smt_set_service(SMT_LIVE);		

    DMSG(1, "VW_Archiving_Open EXIT");
}

void VW_Archiving_Close()
{ 
    _wait_for_thumb_stop();
	nftool_destroy_setup_window(iarch.wnd);	
}

void vw_archiving_start_playback(PB_OPEN_BY open)
{
	nfui_nfobject_hide(iarch.wnd);
	vw_playback_open(NF_TOPWND, iarch.lst, open);
	iarch.lst = NULL;
}

void vw_archiving_show(LIVESTART_T *lst)
{
	vw_arch_tab1_show();
	nfui_nfobject_show(iarch.wnd);
	iarch.lst = lst;
}

