/*
 * vw_vca_rev_component_calibration_result.c
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

#include "vw_vca_rev_component.h"





////////////////////////////////////////////////////////////
//
// private data types
//





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

static gboolean post_result_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_cam_height_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        gchar strBuf[16];
	    top = nfui_nfobject_get_top(obj);
	    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        memset(strBuf, 0x00, sizeof(strBuf));

        if(component_data->unit_setup)
        {
            gdouble tmp;
            g_sprintf(strBuf, "%.2f",component_data->calibration.camera_height);
            tmp = atof(strBuf);
            tmp = ifn_unit_change_3d(tmp,1);     
            g_sprintf(strBuf, "%.2f",tmp);
            nfui_nflabel_set_text((NFLABEL*)obj, strBuf);            
        }
        else
        {
            g_sprintf(strBuf,"%.2f",component_data->calibration.camera_height);
            nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
        }        
	}
	
    return FALSE;
}
static gboolean post_cam_height_unit_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        gchar *unit_label =NULL;
        VCA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
        
        if(component_data->unit_setup) nfui_nflabel_set_text((NFLABEL*)obj,"ft");
        else nfui_nflabel_set_text((NFLABEL *) obj, "m");

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}


static gboolean post_cam_tilt_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        gchar strBuf[16];       
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", component_data->calibration.camera_tilt);
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	}
	
    return FALSE;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_rev_calibration_result_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	
    NFOBJECT *fixed;

    gint label_w;
    gint size_w, size_h;

    gchar strBuf[128];
    

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, 400);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 10, 20);
    nfui_regi_post_event_callback(fixed, post_result_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("3D CALIBRATION RESULT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);

	obj = nfui_nfimage_new(IMG_CALIBRATION_CAM_HEIGHT_N);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 80);

    nfui_get_image_size(IMG_CALIBRATION_CAM_HEIGHT_N, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA HEIGHT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 180, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+10, 80+(size_h-30)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("12", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 100, 30);
    nfui_nfobject_use_focus(obj, FALSE);		
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+200, 80+(size_h-30)/2);
    nfui_regi_post_event_callback(obj, post_cam_height_label_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 100, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+300, 80+(size_h-30)/2);
    nfui_regi_post_event_callback(obj, post_cam_height_unit_label_event_handler);

	obj = nfui_nfimage_new(IMG_CALIBRATION_CAM_TILT_N);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 140);    

    nfui_get_image_size(IMG_CALIBRATION_CAM_TILT_N, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA TILT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 140, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+10, 140+(size_h-30)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("35", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 100, 30);    
    nfui_nfobject_use_focus(obj, FALSE);		
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+200, 140+(size_h-30)/2);
    nfui_regi_post_event_callback(obj, post_cam_tilt_label_event_handler);    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("\u00B0", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 100, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20+size_w+300, 140+(size_h-30)/2);


    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RECOMMEND", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 260, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 240);

    g_sprintf(strBuf, "- %s : 2 ~ 10 m", lookup_string("HEIGHT"));    

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 340, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 276);

    g_sprintf(strBuf, "- %s : 15 ~ 70 \u00B0", lookup_string("TILT"));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 340, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 308);

	return 0;
}

gint vw_vca_rev_calibration_result_component_show()
{

    return 0;
}

gint vw_vca_rev_calibration_result_component_hide()
{

    return 0;
}

gint vw_vca_rev_calibration_result_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_rev_calibration_result_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

