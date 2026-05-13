

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

//#include "nf_ui_snd_main.h"
//#include "nf_ui_audio.h"
//#include "nf_ui_buzzer.h"

#include "vw_sys_sound_main.h"
#include "vw_sys_sound_audio.h"
#include "vw_sys_sound_buzzer.h"

#include "vsm.h"
#include "vw_menu.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static guint snd_change_flag = 0;


static void _init_sound_main(NFOBJECT *nftab, gint page)
{
    gint pos;

    pos = mcf.sys_sub3.menu_pos[SYS_SUB3_AUDIO];    
    if (pos != -1) 
    {
        DMSG(1, "init_SndAudio_page");
    
        init_SndAudio_page(((NFTAB*)nftab)->page[pos]);
    }

    pos = mcf.sys_sub3.menu_pos[SYS_SUB3_BUZZER];   
    if (pos != -1) 
    {
        DMSG(1, "init_SndBuzzer_page");

        init_SndBuzzer_page(((NFTAB*)nftab)->page[pos]);
    }
}

static void _in_handler_sound_main(gint page)
{
    if (page == mcf.sys_sub3.menu_pos[SYS_SUB3_AUDIO])
    {
        DMSG(1, "");    
    }
    else if (page == mcf.sys_sub3.menu_pos[SYS_SUB3_BUZZER])
    {
        DMSG(1, "");        
    }
}

static void _out_handler_sound_main(gint page)
{
    if (page == mcf.sys_sub3.menu_pos[SYS_SUB3_AUDIO])
    {
        DMSG(1, "Audio_tab_out_handler");        
    
        Audio_tab_out_handler();
    }
    else if (page == mcf.sys_sub3.menu_pos[SYS_SUB3_BUZZER])
    {
        DMSG(1, "Buzzer_tab_out_handler");        

        Buzzer_tab_out_handler();
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

    _out_handler_sound_main(cur_page);

	return TRUE;
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		if(syssnd_get_changeflag())
			DAL_save_setup_db(NFSETUP_WINDOW_SOUND);

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

            _out_handler_sound_main(cur_page);            
            _in_handler_sound_main(new_page);            
		break;

		default:
		break;
	}

	return FALSE;
}

void syssnd_set_changeflag(guint flag)
{
	snd_change_flag = flag;
}

guint syssnd_get_changeflag()
{
	return snd_change_flag;
}

void SystemSetupSound_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page ) //wiggls - otm
{
    NFOBJECT *snd_wnd, *fixed, *nftab;

    DMSG(1, "SystemSetupSound_Open");

    vsm_live_stop();
    syssnd_set_changeflag(0);

    snd_wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_SOUND, page);
    g_curwnd = snd_wnd;
    nfui_nfwindow_set_title(snd_wnd, "SYSTEM SETUP - SOUND");
    nftab = nftool_get_nftab_from_setup_window(snd_wnd);
    nfui_nftab_set_cur_page(nftab, page );

    _init_sound_main(nftab, page);

    nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
    nfui_regi_pre_event_callback(snd_wnd, pre_main_wnd_event_handler);
    nfui_nfwindow_set_returnkey_proc(snd_wnd, returnkey_proc);
    
    nfui_nfobject_show(snd_wnd);
    nfui_make_key_hierarchy(snd_wnd);

    nfui_set_key_focus(nftab, TRUE);

    gtk_main();

    DMSG(1, "SystemSetupSound_Open EXIT");  
}



void SystemSetupSound_Destroy(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(obj);
	
	nfui_nfobject_hide(topwin);

	nftool_destroy_setup_window(topwin);
}







