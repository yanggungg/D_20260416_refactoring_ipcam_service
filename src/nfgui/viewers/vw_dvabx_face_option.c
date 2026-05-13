/*
 * vw_dvabx_option.c
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

#include "dvaa.h"
#include "dvaa_itx_face.h"



////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_fixed = NULL;

static NFOBJECT *g_option_bbx_obj = NULL;
static NFOBJECT *g_option_disprule_obj = NULL;

static DVAAID g_dvaaid = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _load_face_option_data(DVA_COMPONENT_DATA_T *component_data, gint expose)
{
    ITX_FACERULE_PROP prop;
    
    memset(&prop, 0x00, sizeof(ITX_FACERULE_PROP));
    dvaa_itx_face_get_rule_prop(g_dvaaid, &prop);

    nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_option_bbx_obj, prop.sw_obj_bb);
    nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_option_disprule_obj, prop.sw_rule);

    if (expose) nfui_signal_emit(g_option_bbx_obj, GDK_EXPOSE, TRUE);
    if (expose) nfui_signal_emit(g_option_disprule_obj, GDK_EXPOSE, TRUE);

    return 0;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_bbx_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        ITX_FACERULE_PROP prop;
        
        memset(&prop, 0x00, sizeof(ITX_FACERULE_PROP));
        dvaa_itx_face_get_rule_prop(g_dvaaid, &prop);

        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)) 
        {
            prop.sw_obj_bb = 1;            
        }
        else 
        {
            prop.sw_obj_bb = 0;
        }

        dvaa_itx_face_set_rule_prop(g_dvaaid, &prop);
    }

    return FALSE;
}

static gboolean post_disprule_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        ITX_FACERULE_PROP prop;
        
        memset(&prop, 0x00, sizeof(ITX_FACERULE_PROP));
        dvaa_itx_face_get_rule_prop(g_dvaaid, &prop);

        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)) 
        {
            prop.sw_rule = 1;
        }
        else 
        {
            prop.sw_rule = 0;
        }

        dvaa_itx_face_set_rule_prop(g_dvaaid, &prop);
    }

    return FALSE;
}

static gboolean post_osd_option_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, obj->width, obj->height-20);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, obj->width, obj->height-20);
        }
        break;

        default :

        break;
    }

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _dvabx_face_option_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint size_w, size_h;
    gint i;

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, 505, 230);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 5, 20);
    nfui_regi_post_event_callback(fixed, post_osd_option_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OSD OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);

    pos_x = 10;
    pos_y = 50;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_bbx_check_event_handler);
    g_option_bbx_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT BOUNDING BOX", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  

	pos_y += 38;

/*
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY GROUP NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y); 

	pos_y += 38;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY FACE NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y); 		

	pos_y += 38;
*/

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_disprule_check_event_handler);
    g_option_disprule_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY RULES", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  	

    return 0;
}

void _dvabx_face_option_show(NFOBJECT *parent)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;

    top = nfui_nfobject_get_top(parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    g_dvaaid = dvaa_get_face_dvaaid(component_data->preview.ch);
    _load_face_option_data(component_data, 0);

    nfui_nfobject_show(parent);
}

void _dvabx_face_option_hide(NFOBJECT *parent)
{

	nfui_nfobject_hide(parent);
}
