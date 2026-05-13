/*
 * vw_dvabx_component_option.c
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
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"

#include "vw_dvabx_component.h"
#include "dvaa_itx.h"





////////////////////////////////////////////////////////////
//
// private data types
//
enum{
    OSD_DISPLAY_OBJ_ID = 0,
    OSD_DISPLAY_OBJ_WIDTH,
    OSD_DISPLAY_OBJ_HEIGHT,
    OSD_DISPLAY_OBJ_SPEED,
};



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

static gboolean post_va_option_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_track_reference_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
		gint index;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

        component_data->option.track_reference = index;
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

		gint index;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        index = nfui_radio_button_get_index((NFBUTTON*)obj);

        if (component_data->option.track_reference == index)
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

    return FALSE;
}

static gboolean post_minimum_object_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        NFOBJECT *mini_w = NULL;
        NFOBJECT *mini_h = NULL;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON *)obj);
        component_data->option.minimum_object = status;
        component_action->option_cb(obj);

        mini_w = nfui_nfobject_get_data(obj->parent, "MINIMUM WIDTH");   
        mini_h = nfui_nfobject_get_data(obj->parent, "MINIMUM HEIGHT");            

        if (mini_w == NULL) return FALSE;
        if (mini_h == NULL) return FALSE;

        if(component_data->option.minimum_object)
        {
            nfui_nfobject_enable(mini_w);
            nfui_nfobject_enable(mini_h);
        }
        else
        {
            nfui_nfobject_disable(mini_w);
            nfui_nfobject_disable(mini_h);
        }
        
        nfui_signal_emit(mini_w, GDK_EXPOSE, TRUE);
        nfui_signal_emit(mini_h, GDK_EXPOSE, TRUE);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.minimum_object);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        if(component_data->act_capable && component_data->act_license && component_data->calibration.param_valid)
        {
            nfui_nfobject_enable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        }
        
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_static_filter_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        NFOBJECT *sense_combo = NULL;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON *)obj);
        component_data->option.static_filter = status;
        component_action->option_cb(obj);

        sense_combo = nfui_nfobject_get_data(obj->parent, "STATIC FILTER SENSE");   
        if (sense_combo == NULL) return FALSE;

        if(status) nfui_nfobject_enable(sense_combo);
        else nfui_nfobject_disable(sense_combo);
        
        nfui_signal_emit(sense_combo, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.static_filter);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj); 

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_static_sense_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_data->option.static_filter_sense = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, component_data->option.static_filter_sense);

        if (component_data->act_capable && component_data->act_license) 
        {
            if(component_data->option.static_filter) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj); 
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_bounding_box_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

		status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->option.disp_obj_box = status;
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_box);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_trajectories_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

		status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->option.disp_obj_traj = status;
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) 
        {
            nfui_nfobject_enable(obj);
        }
        else
        {
            nfui_nfobject_disable(obj);
        } 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_traj);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_id_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        component_data->option.disp_obj_id = status;

        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        ITX_DVACALB_RESULT calb_res;
        DVAAID dvaaid;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        dvaa_itx_detector_get_calb_result(dvaaid, &calb_res);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) 
        {
            nfui_nfobject_enable(obj);
        }        
        else
        {
            nfui_nfobject_disable(obj);
        } 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_id);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_width_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);      
        component_data->option.disp_obj_w = status;

        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        ITX_DVACALB_RESULT calb_res;
        DVAAID dvaaid;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        dvaa_itx_detector_get_calb_result(dvaaid, &calb_res);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) 
        {
            if (calb_res.paramvalid) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }        
        else
        {
            nfui_nfobject_disable(obj);
        } 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_w);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_height_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        component_data->option.disp_obj_h = status;

        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        ITX_DVACALB_RESULT calb_res;
        DVAAID dvaaid;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        dvaa_itx_detector_get_calb_result(dvaaid, &calb_res);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) 
        {
            if (calb_res.paramvalid) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }        
        else
        {
            nfui_nfobject_disable(obj);
        } 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_h);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_obj_speed_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint status;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        component_data->option.disp_obj_speed = status;

        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        ITX_DVACALB_RESULT calb_res;
        DVAAID dvaaid;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        dvaa_itx_detector_get_calb_result(dvaaid, &calb_res);

        if (component_data->act_capable && component_data->act_license && component_data->en_engine) 
        {
            if (calb_res.paramvalid) nfui_nfobject_enable(obj);
            else nfui_nfobject_disable(obj);
        }        
        else
        {
            nfui_nfobject_disable(obj);
        } 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_obj_speed);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}

static gboolean post_dis_rules_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
 		gint status;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

		status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->option.disp_rules = status;
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->option.disp_rules);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	
    return FALSE;
}
static gboolean post_minimum_height_unit_label(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_minimum_width_unit_label(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_minimum_w_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gfloat index;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        index = atof(nfui_spin_button_get_text((NFSPINBUTTON*)obj));
        component_data->option.minimum_w = (index*1000);
                      
        if(component_data->unit_setup )
        {
            gfloat index;
            index = ifn_unit_change((gdouble) component_data->option.minimum_w/10, 0);
            component_data->option.minimum_w = (index*10); 
        }
        else
        {
            if(index > 10){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->option.minimum_w = 0;
            }
        }
        component_action->option_cb(obj);
           
	}
	else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint index;
        gint numTemp;
    	gchar value[5];
    	gchar *strTemp;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		memset(value, 0x00, sizeof(value));	
        gcvt(index*0.01, 3, value);

		strTemp = Real_NumberKey_Str_Open(top, value, x, y, "10.00");
		
        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);

            component_data->option.minimum_w = numTemp*10;
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp);
            if(component_data->unit_setup )
            {
                gfloat index;
                index = ifn_unit_change((gdouble) component_data->option.minimum_w/10, 0);
                component_data->option.minimum_w = (index*10);
            }
            component_action->option_cb(obj);
            ifree(strTemp);
        }
    }
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if( component_data->unit_setup)
        {
            gfloat index;
            index = ifn_unit_change((gdouble)component_data->option.minimum_w/10,1);
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *) obj, (int)(index));
        }
        else
        {   
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *) obj, (int)(component_data->option.minimum_w/10));
        }

        if (component_data->act_capable && component_data->act_license && component_data->calibration.param_valid) 
        {
            if(component_data->option.minimum_object)
            {
                nfui_nfobject_enable(obj);
                nfui_signal_emit((NFOBJECT *) obj, GDK_EXPOSE, TRUE);
            }
            else
                nfui_nfobject_disable(obj);
        }
        else nfui_nfobject_disable(obj);

	}
	
    return FALSE;
}

static gboolean post_minimum_h_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;

		gfloat index;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        index = atof(nfui_spin_button_get_text((NFSPINBUTTON*)obj));
        component_data->option.minimum_h = (index*1000);

        if(component_data->unit_setup)
        {
            gfloat index;
            index = ifn_unit_change((gdouble) component_data->option.minimum_h/10,0);
            component_data->option.minimum_h = (index*10);
        }
        else
        {
            if(index > 10){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->option.minimum_h = 0;
            }
        }
        component_action->option_cb(obj);
	}
	else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;
        gint index;
    	gchar value[5];
    	gchar *strTemp;
        
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action= (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		memset(value, 0x00, sizeof(value));	
        gcvt(index*0.01, 3, value);

		strTemp = Real_NumberKey_Str_Open(top, value, x, y, "10.00");
		
        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);
            
            component_data->option.minimum_h = numTemp*10;
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp);
            if(component_data->unit_setup)
            {
                gfloat index;
                index = ifn_unit_change((gdouble) component_data->option.minimum_h/10,0);
                component_data->option.minimum_h = (index*10);
            }
            component_action->option_cb(obj);
            ifree(strTemp);
        }
    }

	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if( component_data->unit_setup)
        {
            gfloat index;
            index = ifn_unit_change((gdouble)component_data->option.minimum_h/10,1);
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *)obj, (int)index);
        }
        else
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON *)obj, (int)component_data->option.minimum_h/10);

        if (component_data->act_capable && component_data->act_license && component_data->calibration.param_valid) 
        {
            if(component_data->option.minimum_object)
            {
                nfui_nfobject_enable(obj);
                nfui_signal_emit((NFOBJECT *) obj, GDK_EXPOSE, TRUE);
            }                
            else
                nfui_nfobject_disable(obj);
        }
        else nfui_nfobject_disable(obj);
            
	}
	
    return FALSE;
}

static gboolean post_setup_radio_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVA_COMPONENT_ACTION_T *component_action;        

		gint index;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        component_action = (DVA_COMPONENT_ACTION_T *)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

		index = nfui_radio_button_get_index((NFSPINBUTTON*)obj);
        component_data->unit_setup = index;
        component_action->option_cb(obj);
	}
	else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

		gint index;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_radio_button_get_index((NFSPINBUTTON*)obj);


        if(component_data->unit_setup) set_unit_change_yard(1); 
        else                           set_unit_change_yard(0);

        if (component_data->unit_setup == index)
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }            
	}
	
	return FALSE;
}

static gboolean post_default_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{     
    	NFOBJECT *top;
        DVA_COMPONENT_ACTION_T *component_action;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
               
        top = nfui_nfobject_get_top(obj);
        component_action = (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION);

        component_action->option_default_cb(obj->parent);   
    }  
	return FALSE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_dvabx_option_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	
    NFOBJECT *fixed;
	GSList *tr_slist = NULL;
	GSList *unit_slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	
    gint fixed_x, fixed_y;

    gint pos_x, pos_y;
    gint size_w, size_h;


	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    fixed_x = 5;
    fixed_y = 20;

// VA OPTIONS FIXED
    if (opt & (1 << OPT_OPTION_SHOW_ENGINE))
    {
        fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH - 20, 274);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED *)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_va_option_fixed_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("AI OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, 20, 0);

        pos_x = 10;
        pos_y = 50;

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("TRACK REFERENCE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 300, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y);

        pos_y += 31;

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();

        tr_slist = nfui_radio_button_get_group(NF_BUTTON(obj));
        nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_track_reference_radio_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("CENTROID", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 180, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_radio_button_add_group(NF_BUTTON(obj), tr_slist);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 220, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_track_reference_radio_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("GROUND POINT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 200, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 250, pos_y);

        pos_y += 40;

        if (ivsc.dfunc.ai.support_calibration)
        {
            obj = (NFOBJECT *)nfui_checkbutton_new(FALSE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
            nfui_regi_post_event_callback(obj, post_minimum_object_check_event_handler);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("MINIMUM SIZE TO DETECT OBJECT(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 460, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            pos_y += 31;

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("MINIMUM WIDTH", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 220, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            obj = nfui_spinbutton_new_value_with_range(1.00, 0.00, 32.80, 0.01);
            nfui_spinbutton_set_skin_type((NFSPINBUTTON *)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
            nfui_nfobject_set_size(obj, 120, 30);
            nfui_nfobject_show(obj);
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 260, pos_y);
            nfui_regi_post_event_callback(obj, post_minimum_w_spin_event_handler);
            nfui_nfobject_set_data(fixed, "MINIMUM WIDTH", obj);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 30, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 5);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 260 + 120, pos_y);
            nfui_regi_post_event_callback(obj, post_minimum_width_unit_label);

            pos_y += 31;

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("MINIMUM HEIGHT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 220, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            obj = nfui_spinbutton_new_value_with_range(2.00, 0.00, 32.80, 0.01);
            nfui_spinbutton_set_skin_type((NFSPINBUTTON *)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
            nfui_nfobject_set_size(obj, 120, 30);
            nfui_nfobject_show(obj);
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 260, pos_y);
            nfui_regi_post_event_callback(obj, post_minimum_h_spin_event_handler);
            nfui_nfobject_set_data(fixed, "MINIMUM HEIGHT", obj);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 30, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 5);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 260 + 120, pos_y);
            nfui_regi_post_event_callback(obj, post_minimum_height_unit_label);

            pos_y += 40;
        }

        obj = (NFOBJECT *)nfui_checkbutton_new(FALSE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_static_filter_check_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("STATIC OBJECT FILTER", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 280, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        obj = nfui_combobox_new(0, 0, 0);
        nfui_combobox_set_skin_type((NFCOMBOBOX *)obj, NFCOMBOBOX_TYPE_POPUP_SMALL_1);
        nfui_nfobject_set_size(obj, 140, 30);
        nfui_nfobject_show(obj);
        nfui_nfobject_disable(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 320, pos_y);
        nfui_regi_post_event_callback(obj, post_static_sense_combo_event_handler);
        nfui_nfobject_set_data(fixed, "STATIC FILTER SENSE", obj);

        nfui_combobox_append_data((NFCOMBOBOX *)obj, "LOW");
        nfui_combobox_append_data((NFCOMBOBOX *)obj, "MID");
        nfui_combobox_append_data((NFCOMBOBOX *)obj, "HIGH");

        fixed_y += 274;
    }

    // OSD OPTION FIXED
    if (opt & (1 << OPT_OPTION_SHOW_OSD))
    {
        fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH - 20, 287);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED *)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_osd_option_fixed_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("OSD OPTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, 20, 0);

        pos_x = 10;
        pos_y = 50;

        obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_dis_rules_check_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY RULES", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        pos_y += 31;

        obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_dis_obj_bounding_box_check_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT BOUNDING BOX", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        pos_y += 31;

        obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_dis_obj_trajectories_check_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT TRAJECTORIES", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        pos_y += 31;

        obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
        nfui_regi_post_event_callback(obj, post_dis_obj_id_check_event_handler);

        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT ID", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

        pos_y += 31;

        if (ivsc.dfunc.ai.support_calibration)
        {
            obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
            nfui_regi_post_event_callback(obj, post_dis_obj_width_check_event_handler);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT WIDTH(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 400, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            pos_y += 31;

            obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
            nfui_regi_post_event_callback(obj, post_dis_obj_height_check_event_handler);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT HEIGHT(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 400, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            pos_y += 31;

            obj = (NFOBJECT *)nfui_checkbutton_new(TRUE);
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
            nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x, pos_y + (30 - size_h) / 2);
            nfui_regi_post_event_callback(obj, post_dis_obj_speed_check_event_handler);

            obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DISPLAY OBJECT SPEED(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
            nfui_nfobject_modify_bg((NFOBJECT *)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
            nfui_nfobject_set_size(obj, 400, 30);
            nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED *)fixed, obj, pos_x + 30, pos_y);

            fixed_y += 307;
        }
    }

// UNITS SETUP OPTION FIXED
    if(opt & (1 << OPT_OPTION_SHOW_UNIT))
    {
        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, 112);
        nfui_nfobject_show(fixed);
        nfui_nffixed_put((NFFIXED*)parent, fixed, fixed_x, fixed_y);
        nfui_regi_post_event_callback(fixed, post_osd_option_fixed_event_handler);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("UNITS SETUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);

        pos_x = 10;
        pos_y = 31;

        nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   

        obj = (NFOBJECT*)nfui_nfbutton_new();
    	nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        unit_slist = nfui_radio_button_get_group(NF_BUTTON(obj));
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_setup_radio_button_event_handler);


        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("METRIC SYSTEM : cm, m, km/h", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+40, pos_y);

        pos_y += 31;

    	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   

        obj = (NFOBJECT*)nfui_nfbutton_new();
    	nfui_radio_button_add_group(NF_BUTTON(obj), unit_slist);
    	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_setup_radio_button_event_handler);
        nfui_nfobject_disable(obj);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("YARD SYSTEM : inch, ft, mi/h", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, 400, 30);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+40, pos_y);
        pos_y = 40 + 274 + 287 + 62 + 10;
    }
    
/*    
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DEFAULT OPTION", 230);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0); 
    nfui_nfobject_show(obj);
    //nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, DEFAULT_COMPONENT_WIDTH-10-230, pos_y+25);
    nfui_regi_post_event_callback(obj, post_default_button_event_handler);
*/

	return 0;
}

gint vw_dvabx_option_component_show()
{

    return 0;
}

gint vw_dvabx_option_component_hide()
{

    return 0;
}

gint vw_dvabx_option_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_option_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}
