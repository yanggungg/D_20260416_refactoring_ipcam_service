/*
 * vw_vca_rev_search_result.c
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
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

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vca_rev_component.h"
#include "vw_search_by_smart_rev_internal.h"
#include "vw_vca_rev_component.h"

#include "vaa.h"
#include "vaa_itx.h"

#define	_FILTER_X                       (guint)((1896 - _FILTER_W) / 2)
#define	_FILTER_Y                       (guint)((943 - _FILTER_H) / 2)
#define	_FILTER_W                       980
#define	_FILTER_H                       480





////////////////////////////////////////////////////////////
//
// private variable

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_rule_obj[16] = {NULL, };
static NFOBJECT *g_all_chk = NULL;

static guint g_use_mask = 0;
static guint *g_rule_msk = 0;
static guint *g_event_msk = 0;

static gint g_rule_cnt = 0;
static gint g_ret = 0;





////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gboolean _is_all_check()
{
    gint i;

    for (i = 0; i < 16; i++)
    {
        if ((g_use_mask & (1 << i)) && ((*g_rule_msk & (1 << i)) == 0)) return FALSE;
    }

    return TRUE;
}



////////////////////////////////////////////////////////////
//
// handler
//
static gboolean post_ruleid_all_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status, i;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        for (i = 0; i < 16; i++)
        {  
            if (g_use_mask & (1 << i))
            {
                if (status) {
                    nfui_check_button_set_active((NFCHECKBUTTON*)g_rule_obj[i], TRUE);
                    *g_rule_msk |= (1 << i);
                }
                else {
                    nfui_check_button_set_active((NFCHECKBUTTON*)g_rule_obj[i], FALSE);
                    *g_rule_msk &= ~(1 << i);   
                }
            }       
        }        
	}
    return FALSE;
}

static gboolean post_ruleid_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status,i;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        for (i = 0; i < 16; i++)
        {            
            if (g_rule_obj[i] == obj) break;                
        }

        if(status) *g_rule_msk |= (1 << i);
        else *g_rule_msk &= ~(1 << i);

        nfui_check_button_set_active((NFCHECKBUTTON*)g_all_chk, _is_all_check());
	}
    return FALSE;
}

static gboolean post_forward_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_DIR_POS;
        else *g_event_msk &= ~IVCA_ET_DIR_POS;
	}
    return FALSE;
}

static gboolean post_reverse_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_DIR_NEG;
        else *g_event_msk &= ~IVCA_ET_DIR_NEG;
	}
    return FALSE;
}

static gboolean post_enter_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_ENTER;
        else *g_event_msk &= ~IVCA_ET_ENTER;
	}
    return FALSE;
}

static gboolean post_exit_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_EXIT;
        else *g_event_msk &= ~IVCA_ET_EXIT;
	}
    return FALSE;
}

static gboolean post_stopped_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_STOPPED;
        else *g_event_msk &= ~IVCA_ET_STOPPED;
	}
    return FALSE;
}

static gboolean post_removed_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_REMOVED;
        else *g_event_msk &= ~IVCA_ET_REMOVED;
	}
    return FALSE;
}

static gboolean post_loiter_check_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gint status;
        
		status = nfui_check_button_get_active((NFBUTTON*)obj);

        if(status) *g_event_msk |= IVCA_ET_LOITERED;
        else *g_event_msk &= ~IVCA_ET_LOITERED;
	}
    return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;

}

static gboolean post_fixd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkPixbuf *pbuf = NULL;
	GdkGC *gc;
	gint gap_x, gap_y, size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);
	
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	}

	return FALSE;
}

static gboolean filter_search_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
	}

    g_ret = 1;
	return FALSE;
}

static gboolean filter_close_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
	}
	
    g_ret = 0;
	return FALSE;
}





////////////////////////////////////////////////////////////
//
// protected interfaces 
//
gint vw_search_by_smart_rev_filter_open(NFWINDOW *parent, guint *rule_msk, guint *event_msk)
{
    NFOBJECT *win;
    NFOBJECT *obj;

    NFOBJECT *main_fixed;
    NFOBJECT *fixed;
    NFOBJECT *all_obj;;
        
    VCA_COMPONENT_DATA_T *component_data;
    ITX_VAZONE_SHAPE zone_shape;
    ITX_VAZONE_CONF zone_conf;

	gint size_w, size_h;
    VAAID vaaid;
    gint i = 0, cnt = 0;
    gint x, y;

    g_use_mask = 0;
    g_rule_msk = rule_msk;
    g_event_msk = event_msk;

    g_ret = 0; 
    
	win = (NFOBJECT*)nftool_create_popup_window(parent, _FILTER_X, _FILTER_Y, _FILTER_W, _FILTER_H, "FILTER", FALSE);
	nfui_nfwindow_set_title(win, "FILTER");
	nfui_regi_post_event_callback(win, post_win_event_handler);
	g_curwnd = (NFWINDOW*)win;
	
	main_fixed = ((NFWINDOW*)g_curwnd)->child;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, _FILTER_W-20, 190);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 10, 50);
    nfui_regi_post_event_callback(fixed, post_fixd_event_handler);

    x = 10;
    y = 0;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RULE NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 150, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);
    
    x += 150;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_ruleid_all_check_event_handler);
    g_all_chk = obj;
    
    x += size_w;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 100, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    
    x = 20;
    y += 40;

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)parent, VCA_COMPONENT_DATA);
    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
    
    for (i = 0; i < 16; i++)
    {  
        vaa_itx_get_zone_conf(vaaid, i, &zone_conf);
        vaa_itx_get_zone_shape(vaaid, i, &zone_shape);

        if (zone_conf.use_zone)
        {
            obj = (NFOBJECT*)nfui_checkbutton_new(*g_rule_msk & (1 << i));
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
            nfui_regi_post_event_callback(obj, post_ruleid_check_event_handler);
            g_rule_obj[i] = obj;

            g_use_mask |= (1 << i);
            
            x += 30;
            
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(zone_shape.name, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_set_size(obj, 200, 30);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);

            x += 200;
            cnt++;
       
            if (cnt % 4 == 0)
            {
                x = 20;
                y +=40;
            }           
        }
        else
        {
            g_rule_obj[i] = 0;
        }
    }

    if (_is_all_check()) nfui_check_button_set_active((NFCHECKBUTTON*)g_all_chk, TRUE);
    
    x = 0;
    y = 280;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, _FILTER_W-20, 110);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)main_fixed, fixed, 10, y);
    nfui_regi_post_event_callback(fixed, post_fixd_event_handler);
    
    x = 10;
    y = 0;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 150, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);
    
    x = 20;
    y += 40;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_DIR_POS);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_forward_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FORWARD DIRECTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 250, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);      

    x +=276;

    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_DIR_NEG);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_reverse_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REVERSE DIRECTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);  
    
    x += 276;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_ENTER);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_enter_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA-ENTER", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);  
    
    x = 20;
    y += 40;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_EXIT);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_exit_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA-EXIT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);      
    
    x += 200;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_STOPPED);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_stopped_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA-STOPPED", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);            
    
    x += 200;
        
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_REMOVED);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_removed_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA-REMOVED", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);   
    
    x += 200;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(*g_event_msk & IVCA_ET_LOITERED);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, x, y);
    nfui_regi_post_event_callback(obj, post_loiter_check_event_handler);
    
    x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VCA-LOITERING", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  x, y);            

 	obj = nftool_normal_button_create_popup_type1("SEARCH", 200);
	nfui_regi_post_event_callback(obj, filter_search_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (_FILTER_W/2)-205 ,_FILTER_H-50);
	
 	obj = nftool_normal_button_create_popup_type1("CLOSE", 200);
	nfui_regi_post_event_callback(obj, filter_close_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (_FILTER_W/2)+5 ,_FILTER_H-50);

	nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);

    nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());
    
	gtk_main();

    nfui_page_close(PGID_POPUPWND, win);

    return g_ret;
}


