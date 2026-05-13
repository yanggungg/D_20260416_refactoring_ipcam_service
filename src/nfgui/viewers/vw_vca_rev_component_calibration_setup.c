/*
 * vw_vca_rev_component_calibration_setup.c
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

#define HELP_TEXT1       "3D Calibration is needed to accurately predict object height, width, and speed when video analytics are enabled."
#define HELP_TEXT2       "Minimum object size (height/width) can be set in the Function Options setting to improve reliability when filtering video analytic events."

////////////////////////////////////////////////////////////
//
// private interfaces 
//





////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_setup_radio_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

		gint index;
        
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

		index = nfui_radio_button_get_index((NFBUTTON*)obj);
        component_data->skip_calibration = index;
	}
	else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

		gint index;
       
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_radio_button_get_index((NFBUTTON*)obj);

        if (component_data->skip_calibration == index)
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }            
	}
	
	return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_rev_calibration_setup_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    gint pos_x, pos_y;
	gint size_w, size_h;
    gint i;
    gint tmp, label_h;

    gchar lfBuf[4096];
	
	
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    pos_x = 20;
    pos_y = 40;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   

    obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
    slist = nfui_radio_button_get_group(NF_BUTTON(obj));
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_setup_radio_button_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SETUP 3D CALIBRATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+40, pos_y);

    pos_y = 80;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   

    obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_radio_button_add_group(NF_BUTTON(obj), slist);
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_setup_radio_button_event_handler);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SKIP THIS STEP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 340, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+40, pos_y);

    if (opt & (1 << OPT_UNIT_SETUP_HELP))
    {
        pos_y = DEFAULT_COMPONENT_HEIGHT - 240;
    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("[?]", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        nfui_nfobject_set_size(obj, 40, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("What is 3D Calibration?", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-60, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+40, pos_y);        

        pos_y += 40;

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(HELP_TEXT1), DEFAULT_COMPONENT_WIDTH-pos_x-30, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
        label_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-20, label_h);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

        pos_y += label_h;
        tmp = label_h;

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(HELP_TEXT2), DEFAULT_COMPONENT_WIDTH-pos_x-30, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-20, (160-tmp));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
        
    }

	return 0;
}

gint vw_vca_rev_calibration_setup_component_show()
{

    return 0;
}

gint vw_vca_rev_calibration_setup_component_hide()
{

    return 0;
}

gint vw_vca_rev_calibration_setup_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_rev_calibration_setup_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

