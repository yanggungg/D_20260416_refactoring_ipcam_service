/*
 * vw_dvabx_rcol.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"

#include "vw_dvabx_prop_internal.h"
#include "vw_dvabx_component.h"
#include "dvaa_itx.h"

enum {
	OPTION_PAGE = 0,
	SUBPAGE_CNT
};


////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *tab_page[SUBPAGE_CNT] = {NULL, };
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;

static NFOBJECT *g_option_fixed = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;		
    gint off_x, off_y;
		
	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = gdk_gc_new(drawable);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, off_x, off_y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean pre_page_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, DEFAULT_COMPONENT_WIDTH, DEFAULT_COMPONENT_HEIGHT);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_option_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
        _dvabx_generic_rcol_hide_page(0);
        _dvabx_engine_show_page(1);
        nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean option_set_page(NFOBJECT *page)
{
	NFOBJECT *fixed;
    NFOBJECT *obj;	
    guint opt = 0;
    
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);	
    g_option_fixed = fixed;
    
    _dvabx_face_option_page(fixed);

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("PREVIOUS", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, fixed->height-40-10);
    nfui_regi_post_event_callback(obj, post_option_prevbutton_event_handler);

	return TRUE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _dvabx_generic_rcol_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	guint i;
	NFOBJECT *tab;
	NFOBJECT *obj;
	
    g_parent = parent;
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);
		
	for (i = 0; i < SUBPAGE_CNT; i++) 
	{
		obj = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(obj, parent->width, parent->height);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nffixed_put((NFFIXED*)parent, obj, 0, 0);
		nfui_regi_pre_event_callback(obj, pre_page_event_cb);
		tab_page[i] = obj;
	}

    option_set_page(tab_page[OPTION_PAGE]);
    
    nfui_nfobject_show(tab_page[OPTION_PAGE]);

    return 0;
}

gint _dvabx_generic_rcol_show_page(gint page, gint expose)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    if (page == 0)
    {
        nfui_nfobject_show(tab_page[OPTION_PAGE]);

        component_data->preview.calb_onoff = 0;
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    }

    _dvabx_generic_option_show(g_parent);
    nfui_nfobject_show(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint _dvabx_generic_rcol_hide_page(gint expose)
{
    nfui_nfobject_hide(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}
