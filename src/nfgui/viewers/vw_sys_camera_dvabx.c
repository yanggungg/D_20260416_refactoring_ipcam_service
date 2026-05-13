/*
 * vw_sys_camera_dvabx.c
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */
 
#include "nf_afx.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfbutton.h"
#include "objects/nftab.h"

#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"

#include "scm.h"
#include "vsm.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_dvabx.h"
#include "vw_dvabx_prop.h"
#include "vw_dvabx_schd.h"
#include "vw_tools.h"




////////////////////////////////////////////////////////////
//
// private data types
//




////////////////////////////////////////////////////////////
//
// private variable
//

DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_tab;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _init_dvabx_data()
{

    return 0;
}




////////////////////////////////////////////////////////////
//
// handler
//

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;
	GdkDrawable *drawable;
	gint x, y;

	switch (evt->type) 
	{
		case GDK_EXPOSE:
		{
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H - 60);
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x + 1, y, 1, MENU_V_SUBTAB_FIXED_H - 60);

			nfui_nfobject_gc_unref(gc);
		}
		break;
		
		case GDK_DELETE:
		{
			g_curwnd = 0;
        }
		break;
		
		default:
		break;
	}
	return FALSE;
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

		if(cur_page == new_page)	return FALSE;

	}

	
	return FALSE;
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
	}
	
	return FALSE;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//






////////////////////////////////////////////////////////////
//
// public interfaces
//

gint VW_dvabx_init_page(NFOBJECT *parent)
{
    NFOBJECT *obj;
    NFOBJECT *tab_page[2];

    guint i;

    static gchar *strImage_h[2] = {
            MKB_IMG_SUBTAB_DIR_H_N_300, MKB_IMG_SUBTAB_DIR_H_S_300
    };
    static gchar *strTabTitle[] = {"PROPERTY", "SCHEDULE"};
    static guint tabcolor[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};


	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE
	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE
	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE
	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE
	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE
	// !!!!!!!!!!!!!!!!!!!!!!!!! NOT USED UI FILE


    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    _init_dvabx_data();

    nfui_nfobject_set_size(parent, MENU_V_SUBTAB_FIXED_W, MENU_V_SUBTAB_FIXED_H);
    nfui_nffixed_put((NFFIXED*)parent->parent, parent,  MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
    nfui_regi_pre_event_callback(parent, pre_page_event_handler);

    obj = (NFOBJECT *)nfui_nftab_new(2, strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, tabcolor);
    nfui_nftab_set_pango_font((NFTAB *)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nftab_set_margin((NFTAB *)obj, 10);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_PAGE_X, 5);
    nfui_regi_pre_event_callback(obj, pre_subtab_event_handler);
    g_tab = obj;

    for (i = 0; i < 2; i ++) 
    {
        obj = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_set_size(obj, MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
        nfui_nftab_regi_page((NFTAB *)g_tab, obj, i);
        nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
        nfui_regi_pre_event_callback(obj, pre_subtab_page_event_handler);
        tab_page[i] = obj;
    }


    nfui_nfobject_show(tab_page[0]);

    return 0;
}

gint VW_dvabx_start_preview()
{

    return 0;
}

gboolean VW_dvabx_tab_in_handler()
{
    
	return FALSE;
}

gboolean VW_dvabx_tab_out_handler()
{
  
	return FALSE;
}

