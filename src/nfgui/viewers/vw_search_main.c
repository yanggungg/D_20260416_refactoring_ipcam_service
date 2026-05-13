#include "nf_afx.h"

#include "nf_api_play.h"
#include "nf_sysman.h"


#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/nf_ui_image.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nftab.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfthumbnail.h"

#include "vw_search_main.h"
#include "vw_search_by_time.h"
#include "vw_search_by_thumbnail.h"
#include "vw_search_by_text.h"
#include "vw_search_by_event.h"
#include "vw_search_by_smart.h"
#include "vw_search_by_statistic.h"
#include "vw_search_by_deeplearning.h"
#include "vw_menu.h"

#include "tools/nf_ui_function.h"
#include "ix_func.h"

#include "stm.h"
#include "smt.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


typedef struct _SEARCH_T {
	LIVESTART_T		*lst;
	NFOBJECT 		*wnd;
} SEARCH_T;


static NFWINDOW *g_curwnd = 0;
static SEARCH_T isearch;


static NFOBJECT *tab;
static NFOBJECT *tab_page[SEARCH_SUB_CNT];
gchar *g_file_text = NULL;

static void _init_file_data()
{
    FILE *fp;
    size_t f_size;

    fp = fopen(GENERIC_FILE, "rt");

    if (fp == NULL)
    {
        g_message("FILE OPEN ERROR");
        return;
    }

    fseek(fp, 0, SEEK_END);
	f_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    if (g_file_text == NULL)
    {
        g_message("INIT FILE DATA");
        g_file_text = (char*)malloc(sizeof(char)*(f_size+1));
        fread(g_file_text, 1, f_size, fp);
        g_file_text[f_size] = '\0';
    }

    fclose(fp);
    return;
}

static void _init_search_main(NFOBJECT *nftab, gint page)
{
    gint pos;
    time_t from, to;

    pos = mcf.search.menu_pos[SEARCH_SUB_TIME];     
    if (pos != -1) {
        DMSG(1, "vw_init_SearchByTime_page");

        vw_init_SearchByTime_page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }
    
    pos = mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL];            
    if (pos != -1) {
        DMSG(1, "vw_init_SearchByThumbnail_page");
        
        vw_init_SearchByThumbnail_page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }
    
    pos = mcf.search.menu_pos[SEARCH_SUB_EVENT];            
    if (pos != -1) 
    {
        DMSG(1, "vw_init_SearchByEvent_page");

        stm_get_time_range_t(&from, &to);
        vw_init_SearchByEvent_page(((NFTAB*)nftab)->page[pos], from, to);    
        tab_page[pos] = ((NFTAB *)nftab)->page[pos]; 
    }

    pos = mcf.search.menu_pos[SEARCH_SUB_TEXT];            
    if (pos != -1) 
    {
        DMSG(1, "vw_init_SearchByText_page");

        stm_get_time_range_t(&from, &to);
        vw_init_SearchByText_page(((NFTAB*)nftab)->page[pos], from, to);    
        tab_page[pos] = ((NFTAB *)nftab)->page[pos]; 
    }

    pos = mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC];
    if (pos != -1)
    {
        vw_init_SearchBy_statistic_page(((NFTAB*)nftab)->page[pos]);
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }

    pos = mcf.search.menu_pos[SEARCH_SUB_SMART];
    if ( pos != -1 ) {
        DMSG(1, "vw_init_SearchBySmart_page");
    
        if ( page == pos )
            stm_get_arch_time_t(&from, &to);
        else {
            time(&to);
            from = to - 1800;
        }
        vw_init_SearchBySmart_page(((NFTAB*)nftab)->page[pos], from, to);
        //if ( page == pos )
            //vw_SearchBySmart_tab_in_handler();
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }
    
    pos = mcf.search.menu_pos[SEARCH_SUB_SMART_REV];
    if ( pos != -1 ) 
    {
        DMSG(1, "vw_init_SearchBySmart_rev_page");
    
        if ( page == pos )
            stm_get_arch_time_t(&from, &to);
        else {
            time(&to);
            from = to - 1800;
        }
    
        vw_init_SearchBySmart_rev_page(((NFTAB*)nftab)->page[pos], from, to);
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }

    pos = mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING];
    if ( pos != -1 ) 
    {
        DMSG(1, "vw_init_SearchByDeepLearning_page");
    
        if ( page == pos )
            stm_get_arch_time_t(&from, &to);
        else {
            time(&to);
            from = to - 1800;
        }
        
        _init_file_data();
        vw_init_SearchByDeepLearning_page(((NFTAB*)nftab)->page[pos], from, to, g_file_text);
        tab_page[pos] = ((NFTAB *)nftab)->page[pos];
    }

    tab = nftab;
}

static void _in_handler_search_main(gint page)
{
    if (page == mcf.search.menu_pos[SEARCH_SUB_TIME])
    {
        DMSG(1, "vw_SearchByTime_tab_in_handler");
    
        vw_SearchByTime_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL])
    {
        DMSG(1, "vw_SearchByThumbnail_tab_in_handler");

        vw_SearchByThumbnail_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_EVENT])
    {
        DMSG(1, "vw_SearchByEvent_tab_in_handler");
        
        vw_SearchByEvent_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_TEXT])
    {
        DMSG(1, "vw_SearchByText_tab_in_handler");

        vw_SearchByText_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC])
    {
        DMSG(1, "vw_SearchByText_tab_in_handler");

        vw_VaStatistic_tab_in_handler();
    }    
    else if (page == mcf.search.menu_pos[SEARCH_SUB_SMART])
    {
        DMSG(1, "vw_SearchBySmart_tab_in_handler");
        
        vw_SearchBySmart_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_SMART_REV])
    {
        DMSG(1, "vw_SearchBySmart_rev_tab_in_handler");
        
        vw_SearchBySmart_rev_tab_in_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING])
    {
        DMSG(1, "vw_SearchByDeepLearning_tab_in_handler");
        
        vw_SearchByDeepLearning_tab_in_handler();
    }    
}

static void _out_handler_search_main(gint page)
{
    if (page == mcf.search.menu_pos[SEARCH_SUB_TIME])
    {
        DMSG(1, "vw_SearchByTime_tab_out_handler");
    
        vw_SearchByTime_tab_out_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL])
    {
        DMSG(1, "vw_SearchByThumbnail_tab_out_handler");
        
        vw_SearchByThumbnail_tab_out_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_EVENT])
    {
        DMSG(1, "vw_SearchByEvent_tab_out_handler");

        vw_SearchByEvent_tab_out_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_TEXT])
    {
        DMSG(1, "vw_SearchByText_tab_out_handler");

        vw_SearchByText_tab_out_handler();
    }
    else if (page == mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC])
    {
        vw_VaStatistic_tab_out_handler();
    }    
    else if ( page == mcf.search.menu_pos[SEARCH_SUB_SMART])
    {
        DMSG(1, "vw_SearchBySmart_tab_out_handler");
        
        vw_SearchBySmart_tab_out_handler();
    }
    else if ( page == mcf.search.menu_pos[SEARCH_SUB_SMART_REV])
    {
        DMSG(1, "vw_SearchBySmart_rev_tab_out_handler");
        
        vw_SearchBySmart_rev_tab_out_handler();
    }
    else if ( page == mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING])
    {
        DMSG(1, "vw_SearchByDeepLearning_tab_out_handler");
        
        vw_SearchByDeepLearning_tab_out_handler();
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
		if (isearch.lst) 
		{
			isearch.lst->start();
			vsm_destroy_livestart_obj(isearch.lst);
		}

		g_curwnd = 0;	
		gtk_main_quit();
	}
	else if(evt->type == WND_PRE_CLOSE) {
		vsm_playback_preview_stop();		
		_wait_for_thumb_stop();
	}

	return FALSE;
}


static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch (evt->type) {
	case NFEVENT_TAB_BEFORE_CHANGE:
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);
		
		if(cur_page == new_page)	return FALSE;

        _out_handler_search_main(cur_page);
        _in_handler_search_main(new_page);
	default:
		break;
	}
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    g_message("%s, %d", __FUNCTION__, __LINE__);

	vsm_playback_preview_stop();		
	_wait_for_thumb_stop();

	return TRUE;
}

///////////////////////////////////////////////////////////////////////
//
//
//

void VW_Search_Open(NFWINDOW *parent, gint default_page, LIVESTART_T *lst)
{
	NFOBJECT *fixed, *nftab;

    DMSG(1, "VW_Search_Open");

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
	
 	isearch.lst = lst;

	isearch.wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_SEARCH, default_page);
	g_curwnd = isearch.wnd;
	nfui_nfwindow_set_title(isearch.wnd, "SEARCH");
	nftab = nftool_get_nftab_from_setup_window(isearch.wnd);

    _init_search_main(nftab, default_page);

	if (scm_is_supported_dlva() || scm_is_supported_aicam() || scm_is_supported_aibox()) 
    {
        if (!scm_license_is_activated_dlva() && !scm_license_is_activated_aicam_mask() && !scm_license_is_activated_aibox()) {
            nfui_nftab_unregi_page((NFTAB*)nftab, mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING]);
        }		
	}

	nfui_regi_post_event_callback(isearch.wnd, post_main_wnd_event_handler);
	nfui_nfwindow_set_returnkey_proc(isearch.wnd, returnkey_proc);	
	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);

	nfui_nfobject_show(isearch.wnd);
	nfui_make_key_hierarchy(isearch.wnd);
	nfui_set_key_focus(nftab, TRUE);

	smt_set_service(SMT_SEARCH);	
	
	gtk_main();

    if(g_file_text != NULL) 
    {
        free(g_file_text);
        g_file_text = NULL; 
    }

	smt_set_service(SMT_LIVE);	

    DMSG(1, "VW_Search_Open EXIT"); 
}

time_t VW_Search_Get_Record_FirstTime()
{
	time_t first_t, last_t;
	
	first_t = last_t = 0;
	scm_get_record_time(&first_t, &last_t);

	return first_t;
}

time_t VW_Search_Get_Record_LastTime()
{
	time_t first_t, last_t;

	first_t = last_t = 0;
	scm_get_record_time(&first_t, &last_t);

	return last_t;
}

void VW_Search_Destroy()
{
    vsm_playback_preview_stop();
    _wait_for_thumb_stop();
	nftool_destroy_setup_window(isearch.wnd);
}

void VW_Search_key_remake()
{
	nfui_make_key_hierarchy(isearch.wnd);
}

void VW_Search_start_playback()
{
	nfui_nfobject_hide(isearch.wnd);
	vw_playback_open(NF_TOPWND, isearch.lst, OPEN_BY_SEARCH);
	isearch.lst = NULL;
}

void VW_Search_Show(gint page, LIVESTART_T *lst)
{
	NFOBJECT *nftab;
	gint i = 0;
	time_t from, to;

    if (mcf.search.menu_pos[SEARCH_SUB_TIME] != -1)
    {
        vw_SearchByTime_tab_show();
    }

    if (mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL] != -1)
    {
        vw_SearchByThumbnail_tab_show();
    }

    if (mcf.search.menu_pos[SEARCH_SUB_EVENT] != -1)
    {
        vw_SearchByEvent_tab_show();

    	if (mcf.search.menu_pos[SEARCH_SUB_EVENT] == page)
    	{
    		stm_get_time_range_t(&from, &to);
    		vw_SearchByEvent_set_interval(from, to);
    	}        
    }

    if (mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC] != -1)
    {
        vw_VaStatistic_tab_show();
    }

    if (mcf.search.menu_pos[SEARCH_SUB_TEXT] != -1)
    {
        vw_SearchByText_tab_show();

        if (mcf.search.menu_pos[SEARCH_SUB_TEXT] == page)
        {
            stm_get_time_range_t(&from, &to);
            vw_SearchByText_set_interval(from, to);
        }        
    }    

    if (mcf.search.menu_pos[SEARCH_SUB_VA_STATISTIC] != -1)
    {
        vw_VaStatistic_tab_show();
    }

    if (mcf.search.menu_pos[SEARCH_SUB_SMART] != -1)
    {
        vw_SearchBySmart_tab_show();

    	if (mcf.search.menu_pos[SEARCH_SUB_SMART] != page) set_draw(FALSE);
    }    

    if (mcf.search.menu_pos[SEARCH_SUB_SMART_REV] != -1)
    {
        vw_SearchBySmart_rev_tab_show();
    }   

    if (mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING] != -1)
    {
        vw_SearchByDeepLearning_tab_show();

        if (mcf.search.menu_pos[SEARCH_SUB_DEEPLEARNING] == page)
        {
            stm_get_time_range_t(&from, &to);
            vw_SearchByDeepLearning_set_interval(from, to);
        }        
    }   

	if (page != -1)
	{
		gint pre_page;

		nftab = nftool_get_nftab_from_setup_window(isearch.wnd);
		pre_page = nfui_nftab_get_cur_page(nftab);

		if (pre_page != page)
		{
			nfui_nftab_change_page((NFTAB*)nftab, page);
			nfui_nfobject_show(isearch.wnd);
			nfui_make_key_hierarchy(isearch.wnd);
			nfui_set_key_focus(nftab, TRUE);
		}
		else {
			nfui_nfobject_show(isearch.wnd);		
		}
	}
	else {
		nfui_nfobject_show(isearch.wnd);
	}
		
	isearch.lst = lst;
}

void VW_Search_config_smart(gboolean search_started)
{
    NFOBJECT *cur_focus;
    gint i, pos[4];

    pos[0] = mcf.search.menu_pos[SEARCH_SUB_TIME];
    pos[1] = mcf.search.menu_pos[SEARCH_SUB_THUMBNAIL];
    pos[2] = mcf.search.menu_pos[SEARCH_SUB_EVENT];
    pos[3] = mcf.search.menu_pos[SEARCH_SUB_TEXT];

    if ( search_started ) {
        for (i = 0; i < 4; i++) {
            if ( pos[i] != -1 )
                nfui_nftab_unregi_page((NFTAB *)tab, (guint)pos[i]);
        }
    }
    else {
        for (i = 0; i < 4; i++) {
            if ( pos[i] != -1 )
                nfui_nftab_regi_page((NFTAB *)tab,
                        tab_page[pos[i]], (guint)pos[i]);
        }
    }
    
    cur_focus = nfui_get_cur_focus(isearch.wnd);
    if ( cur_focus ) {
        nfui_signal_emit(tab, GDK_EXPOSE, TRUE);
        nfui_make_key_hierarchy((NFWINDOW *)isearch.wnd);
    }
}


