/*
 * vw_dvabx_component_calibration_adrz.c
 *
 * Written by Jungkyu. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, June 14, 2014
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
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"


#include "vw_dvabx_component.h"
#include "vw_vkeyboard_real_num.h"




////////////////////////////////////////////////////////////
//
// private data types
//

#define ICON_DESCRIPTION1   "Icons may be placed on the virtual screen in order to estimate the installation height and angle of the camera."
#define ICON_DESCRIPTION2   "Click the Add button and place the icon over an object so that it precisely overlaps the object."

#define OPTION_DESCRIPTION  "Select the 'PAUSE VIDEO' option and 'ZOOM' function to allow for more precise icon placement."



////////////////////////////////////////////////////////////
//
// private variable
//





////////////////////////////////////////////////////////////
//
// private interfaces 
//





////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_icon_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_option_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_icon_count_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gchar strBuf[32];

        gint index;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%s (%d/32)", lookup_string("ICON"), component_data->calibration.icon_count);
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;    
        DVA_COMPONENT_ACTION_T *component_action;

        top = nfui_nfobject_get_top(obj);        
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));

        component_action->calibration_add(obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) 
        {
            if (component_data->calibration.icon_count < 32) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;    
        DVA_COMPONENT_ACTION_T *component_action;

        top = nfui_nfobject_get_top(obj);        
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));

        component_action->calibration_delete(obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) 
        {
            if (component_data->calibration.select_icon) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_reset_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;    
        DVA_COMPONENT_ACTION_T *component_action;

        top = nfui_nfobject_get_top(obj);        
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
        
        component_action->calibration_reset(obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) 
        {
            if (component_data->calibration.icon_count > 0) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        }
    }    
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_height_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        
        if(component_data->unit_setup) nfui_nflabel_set_text((NFLABEL*)obj,"ft");
        else nfui_nflabel_set_text((NFLABEL *) obj, "m");
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);        
    }  
    return FALSE;
}


static gboolean post_icon_height_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
        gint value;
        gfloat tmp;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));

        tmp = atof(nfui_spin_button_get_text((NFSPINBUTTON*)obj));
        value = (tmp*100);
        
        component_data->calibration.icon_height = value;

        if(component_data->unit_setup)
        {
            gfloat index;
            index = ifn_unit_change((gdouble) component_data->calibration.icon_height,0);
            component_data->calibration.icon_height = (index);
        }
        else
        {
            if(tmp > 9.99){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->calibration.icon_height = 50;
            }
       
}
        component_action->calibration_height(obj);
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint tmp;
        gint numTemp;
    	gchar value[5];
    	gchar *strTemp;
	
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        tmp = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		memset(value, 0x00, sizeof(value));	
        gcvt((50 + tmp)*0.01, 3, value);
        
		strTemp = Real_NumberKey_Str_Open(top, value, x, y, "9.99");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);

            component_data->calibration.icon_height = numTemp;
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp-50);
            if(component_data->unit_setup)
            {
                gfloat index;
                index = ifn_unit_change((gdouble) component_data->calibration.icon_height,0);
                component_data->calibration.icon_height = (index);
            }
            
            component_action->calibration_height(obj);
            ifree(strTemp);
        }
    }   
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
   
        
        if( component_data->unit_setup)
        {
            gfloat index;
            index = ifn_unit_change((gdouble)(component_data->calibration.icon_height),1);
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *)obj, (int)index-50);
        }
        else
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *)obj, (int)component_data->calibration.icon_height-50);
        

        if (component_data->act_capable && component_data->act_license) 
        {
            if (component_data->calibration.select_icon) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        }        
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_zoom_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;    
        DVA_COMPONENT_ACTION_T *component_action;

        top = nfui_nfobject_get_top(obj);        
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
        
        component_action->calibration_zoom(obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    
    return FALSE;
}

static gboolean post_pause_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;    
        DVA_COMPONENT_DATA_T *component_data;        
        DVA_COMPONENT_ACTION_T *component_action;
        gint status;

        top = nfui_nfobject_get_top(obj);        
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);     
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));

        if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "PAUSE VIDEO") == 0)
        {
            component_data->calibration.pause_video = 1;
            component_action->calibration_pause(obj);
            
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PLAY VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    
        }
        else if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "PLAY VIDEO") == 0)
        {
            component_data->calibration.pause_video = 0;
            component_action->calibration_pause(obj);
            
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    
        
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        if(component_data->calibration.pause_video)
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PLAY VIDEO");
        else
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE VIDEO");
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_dvabx_calibration_adrz_component_open(NFOBJECT *parent, guint opt)
{
    NFOBJECT *obj;  
    NFOBJECT *fixed;

    gchar strBuf[32];
    gchar lfBuf[4096];

    gint pos_x, pos_y;
    gint size_w, size_h;
    gint tmp, label_h;
    gint i;
    
     obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_80FFFF));
    nfui_nfobject_set_size(obj, 25, 25);
    //nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, 215, 10);  
   
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NORMAL", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 25);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, 215+30, 10); 
     
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0080));
    nfui_nfobject_set_size(obj, 25, 25);
    //nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, 215+30+120, 10);    
   
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SELECT", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 25);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, 215+30+120+30, 10); 

// ICON FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, 420+30);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 10, 10+20);
    nfui_regi_post_event_callback(fixed, post_icon_fixed_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s %s", lookup_string("ICON"), " (0/32)");    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 180, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);
    nfui_regi_post_event_callback(obj, post_icon_count_event_handler);

    pos_x = 10;
    pos_y = 50;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(ICON_DESCRIPTION1), DEFAULT_COMPONENT_WIDTH-50, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    label_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-40, label_h);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    pos_y += label_h;
    tmp = label_h;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(ICON_DESCRIPTION2), DEFAULT_COMPONENT_WIDTH-50, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    label_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-40, 160-tmp);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    

    pos_y = 180;
    
    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "<%s>", lookup_string("NOTE"));    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-40, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    pos_y += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 30, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string("A minimum of 5 icons should be set."), DEFAULT_COMPONENT_WIDTH-90, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-80, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+30, pos_y);

    pos_y += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 30, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string("All objects must be on the same plane."), DEFAULT_COMPONENT_WIDTH-90, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-80, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+30, pos_y);

    pos_y += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("3.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 30, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string("Icons must be evenly distributed on the screen."), DEFAULT_COMPONENT_WIDTH-90, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-80, 60);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+30, pos_y);    

    pos_y += 30+46;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("ADD", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_add_button_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(160+4)*1, pos_y);
    nfui_regi_post_event_callback(obj, post_delete_button_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("RESET", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(160+4)*2, pos_y);
    nfui_regi_post_event_callback(obj, post_reset_button_event_handler);

    pos_y += 44;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Selected icon's estimated height", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    

    obj = nfui_spinbutton_new_value_with_range(1.75, 0.50, 32.77, 0.01);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
    nfui_nfobject_set_size(obj, 85, 30);
    nfui_nfobject_show(obj);                
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+325, pos_y);    
    nfui_regi_post_event_callback(obj, post_icon_height_spin_event_handler);
   
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 60, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+410, pos_y); 
    nfui_regi_post_event_callback(obj, post_height_label_event_handler);
    
    pos_y += 40;    

// OPTION FIXED
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, 220-40);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 10, 10+20+400+40+30);
    nfui_regi_post_event_callback(fixed, post_option_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);

    pos_x = 10;
    pos_y = 50;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string(OPTION_DESCRIPTION), 450, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-40, 80);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    pos_y += 80;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("ZOOM", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_zoom_button_event_handler);

    pos_x += 170;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("PAUSE VIDEO", 160);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_pause_button_event_handler);
    
    return 0;
}

gint vw_dvabx_calibration_adrz_component_show()
{

    return 0;
}

gint vw_dvabx_calibration_adrz_component_hide()
{

    return 0;
}

gint vw_dvabx_calibration_adrz_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_calibration_adrz_component_expose_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_EXPOSE, TRUE);
    return 0;
}

gint vw_dvabx_calibration_adrz_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

