/*
 * vw_sys_camera_analysis.c
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
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
#include "vw_sys_camera_analysis.h"
#include "vw_sys_camera_analysis_act.h"
#include "vw_sys_camera_analysis_prop.h"
#include "vw_sys_camera_analysis_schd.h"
#include "vw_sys_camera_analysis_face_database.h"
#include "vw_sys_camera_analysis_plateno_database.h"




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
static NFOBJECT *g_tab_page[5] = {0, };


////////////////////////////////////////////////////////////
//
// private interfaces 
//





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

		if (cur_page == 0) VW_analysis_act_tab_out_handler();
		else if (cur_page == 1) VW_analysis_prop_tab_out_handler();
		else if (cur_page == 2) VW_analysis_schd_tab_out_handler();
		else if (cur_page == 3) VW_analysis_face_database_tab_out_handler();
		else if (cur_page == 4) VW_analysis_plateno_database_tab_out_handler();

		if (new_page == 0) VW_analysis_act_tab_in_handler();
		else if (new_page == 1) VW_analysis_prop_tab_in_handler();
		else if (new_page == 2) VW_analysis_schd_tab_in_handler();		
		else if (new_page == 3) VW_analysis_face_database_tab_in_handler();
		else if (new_page == 4) VW_analysis_plateno_database_tab_in_handler();		
	}
	else if (evt->type == NFEVENT_TAB_CHANGED)
	{
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if (new_page == 1) VW_analysis_prop_tab_changed_handler();
		else if (new_page == 3) VW_analysis_face_database_tab_changed_handler();
		else if (new_page == 4) VW_analysis_plateno_database_tab_changed_handler();
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

gint VW_analysis_init_page(NFOBJECT *parent)
{
    NFOBJECT *obj;

	AiAnalysisActData analysis_data;
    guint i;
	gint page_cnt;

    static gchar *strImage_h[5] = {
            MKB_IMG_SUBTAB_DIR_H_N_300, MKB_IMG_SUBTAB_DIR_H_S_300, MKB_IMG_SUBTAB_DIR_H_S_300, MKB_IMG_SUBTAB_DIR_H_S_300, MKB_IMG_SUBTAB_DIR_H_S_300
    };
    static gchar *strTabTitle[] = {"ACTIVATION", "PROPERTY", "SCHEDULE", "FACE DATABASE", "LICENSE PLATE DATABASE"};
    static guint tabcolor[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};


    g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    nfui_nfobject_set_size(parent, MENU_V_SUBTAB_FIXED_W, MENU_V_SUBTAB_FIXED_H);
    nfui_nffixed_put((NFFIXED*)parent->parent, parent,  MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
    nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	page_cnt = 5;
	if (!ivsc.dfunc.support_license_plate) page_cnt--;
	if (!ivsc.dfunc.support_face) page_cnt--;

    obj = (NFOBJECT *)nfui_nftab_new(page_cnt, strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, tabcolor);
    nfui_nftab_set_pango_font((NFTAB *)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nftab_set_margin((NFTAB *)obj, 10);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_PAGE_X, 5);
    nfui_regi_pre_event_callback(obj, pre_subtab_event_handler);
    g_tab = obj;

    for (i = 0; i < page_cnt; i ++) 
    {
        obj = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_set_size(obj, MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
        nfui_nftab_regi_page((NFTAB *)g_tab, obj, i);
        nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
        nfui_regi_pre_event_callback(obj, pre_subtab_page_event_handler);
        g_tab_page[i] = obj;
    }

    DMSG(1, "VW_analysis_act_init_page");
    VW_analysis_act_init_page(g_tab_page[0]);

    DMSG(1, "VW_analysis_prop_init_page");
    VW_analysis_prop_init_page(g_tab_page[1]);

    DMSG(1, "VW_analysis_schd_init_page");
    VW_analysis_schd_init_page(g_tab_page[2]);

	if (page_cnt == 5)
	{
    DMSG(1, "VW_analysis_face_database_init_page");
    VW_analysis_face_database_init_page(g_tab_page[3]);

    DMSG(1, "VW_analysis_plateno_database_init_page");
    VW_analysis_plateno_database_init_page(g_tab_page[4]);	
	}

    nfui_nfobject_show(g_tab_page[0]);

    return 0;
}

gboolean VW_analysis_tab_in_handler()
{
    gint page = nfui_nftab_get_cur_page((NFTAB*)g_tab);

    if (page == 0) VW_analysis_act_tab_in_handler();
    else if (page == 1) VW_analysis_prop_tab_in_handler();
	else if (page == 2) VW_analysis_schd_tab_in_handler();
	else if (page == 3) VW_analysis_face_database_tab_in_handler();
	else if (page == 4) VW_analysis_plateno_database_tab_in_handler();
    
	return FALSE;
}

gboolean VW_analysis_tab_out_handler()
{
    gint page = nfui_nftab_get_cur_page((NFTAB*)g_tab);

    if (page == 0) VW_analysis_act_tab_out_handler();
    else if (page == 1) VW_analysis_prop_tab_out_handler();
	else if (page == 2) VW_analysis_schd_tab_out_handler();
	else if (page == 3) VW_analysis_face_database_tab_out_handler();
	else if (page == 4) VW_analysis_plateno_database_tab_out_handler();	
    
	return FALSE;
}

gboolean VW_analysis_tab_changed_handler()
{
    gint page = nfui_nftab_get_cur_page((NFTAB*)g_tab);

    if (page == 1) VW_analysis_prop_tab_changed_handler();
	else if (page == 3) VW_analysis_face_database_tab_changed_handler();
	else if (page == 4) VW_analysis_plateno_database_tab_changed_handler();
    
	return FALSE;
}
