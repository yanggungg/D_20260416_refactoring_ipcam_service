#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_ipsetup_main.h"
#include "vw_sys_net_ipsetup.h"
#include "vw_sys_net_ipsetup_ipv6.h"



static NFWINDOW *g_curwnd;
static NFOBJECT *g_nftab;



static void _out_handler_ipsetup_main(gint page)
{
    if (page == 0)
    {
        IpSetup_tab_out_handler();
    }
    else if (page == 1)
    {
        IpSetup_IPv6_tab_out_handler();
    }
    else if (page == 2)
    {
        IpSetup_Lan2_tab_out_handler();
    }
}

static void _in_handler_ipsetup_main(gint page)
{
    if (page == 0)
    {
    }
    else if (page == 1)
    {
    }   
    else if (page == 2)
    {
    }   
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{	
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE) 
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page) return FALSE;

		_out_handler_ipsetup_main(cur_page);
		_in_handler_ipsetup_main(new_page);
	}
	
	return FALSE;
	
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;
		guint x, y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{			
			GdkGC *gc;
			GdkDrawable *drawable;
			guint x, y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			nfui_nfobject_gc_unref(gc);				
		}
		break;

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default :
			break;
	}

	return FALSE;
}

void init_NetIPSetup_Main_page(NFOBJECT *parent)
{
	NFOBJECT *nftab;
	NFOBJECT *tab_page[3];
	gint i, tab_count = 2;

	const gchar *strTabTitle[] = {"IPv4", "IPv6"};
	const gchar *strTabTitle_dual[] = {"LAN1v4", "LAN1v6", "LAN2v4"};
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	gboolean install_mode = DAL_get_cam_install_mode();
	gboolean use_dual_lan = DAL_get_cam_install_use_dual_lan();

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);	
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	if (install_mode && use_dual_lan) nftab = nfui_nftab_new(3, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle_dual, colidx);
	else nftab = nfui_nftab_new(2, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);	
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);
	g_nftab = nftab;

	if (install_mode && use_dual_lan) tab_count = 3;
	else tab_count = 2;

	for(i=0; i<tab_count; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	init_NetIPSetup_page(tab_page[0]);
	init_NetIPSetup_IPv6_page(tab_page[1]);
	if (install_mode && use_dual_lan) init_NetIPSetup_Lan2_page(tab_page[2]);

	nfui_nfobject_show(tab_page[0]);
	
    return;
}

gboolean IpSetup_Main_tab_in_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);

	if( cur_page == 0 )
	{
	}
	else if( cur_page == 1 )
	{
	}
	else if( cur_page == 2 )
	{
	}

	return FALSE;
}

gboolean IpSetup_Main_tab_out_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);	

	if( cur_page == 0 )
	{
	    IpSetup_tab_out_handler();
	}
    else if (cur_page == 1)
    {
        IpSetup_IPv6_tab_out_handler();
    }
    else if (cur_page == 2)
    {
        IpSetup_Lan2_tab_out_handler();
    }

	return FALSE;
}

