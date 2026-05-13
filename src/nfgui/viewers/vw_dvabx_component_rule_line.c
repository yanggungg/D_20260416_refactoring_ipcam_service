/*
 * vw_dvabx_component_rule_line.c
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
#include "objects/cw_slider.h"

#include "vw_color_popup.h"
#include "vw_dvabx_component.h"
#include "vw_vkeyboard.h"

static NFOBJECT *g_all_chk_obj;



////////////////////////////////////////////////////////////
//
// private data types
//
#define HELP_TEXT       "If 3D Calibration is set, the filter can be adjusted so that only objects which meet specified color, size, and speed criteria will trigger VA events."



////////////////////////////////////////////////////////////
//
// private variable
//





////////////////////////////////////////////////////////////
//
// private interfaces 
//
static int _prvValueToMillimeter(int index)
{
    int ret;
    ret = index * 10;
    return ret;
}

static int _prvMillimeterToIndex(int mm)
{
    int ret;
    ret = mm / 10;
    return ret;
}

static int _convert_rgb_color_to_coloridx(unsigned int rgb_col)
{
    int idx = 0;

    switch(rgb_col)
    {
        case COLOR_FF0000: idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;
        case COLOR_FF3F00: idx = COLOR_PRG_IDX(UX_COLOR_FF3F00); break;
        case COLOR_FF7F00: idx = COLOR_PRG_IDX(UX_COLOR_FF7F00); break;
        case COLOR_FFBF00: idx = COLOR_PRG_IDX(UX_COLOR_FFBF00); break;
        case COLOR_FFFF00: idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case COLOR_BFFF00: idx = COLOR_PRG_IDX(UX_COLOR_BFFF00); break;
        case COLOR_202020: idx = COLOR_PRG_IDX(UX_COLOR_202020); break;
        case COLOR_7FFF00: idx = COLOR_PRG_IDX(UX_COLOR_7FFF00); break;
        case COLOR_3FFF00: idx = COLOR_PRG_IDX(UX_COLOR_3FFF00); break;
        case COLOR_00FF00: idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case COLOR_00FF3F: idx = COLOR_PRG_IDX(UX_COLOR_00FF3F); break;
        case COLOR_00FF7F: idx = COLOR_PRG_IDX(UX_COLOR_00FF7F); break;
        case COLOR_00FFBF: idx = COLOR_PRG_IDX(UX_COLOR_00FFBF); break;
        case COLOR_606060: idx = COLOR_PRG_IDX(UX_COLOR_606060); break;
        case COLOR_00FFFF: idx = COLOR_PRG_IDX(UX_COLOR_00FFFF); break;
        case COLOR_00BFFF: idx = COLOR_PRG_IDX(UX_COLOR_00BFFF); break;
        case COLOR_007FFF: idx = COLOR_PRG_IDX(UX_COLOR_007FFF); break;
        case COLOR_003FFF: idx = COLOR_PRG_IDX(UX_COLOR_003FFF); break;
        case COLOR_0000FF: idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case COLOR_3F00FF: idx = COLOR_PRG_IDX(UX_COLOR_3F00FF); break;
        case COLOR_A0A0A0: idx = COLOR_PRG_IDX(UX_COLOR_A0A0A0); break;
        case COLOR_7F00FF: idx = COLOR_PRG_IDX(UX_COLOR_7F00FF); break;
        case COLOR_BF00FF: idx = COLOR_PRG_IDX(UX_COLOR_BF00FF); break;
        case COLOR_FF00FF: idx = COLOR_PRG_IDX(UX_COLOR_FF00FF); break;
        case COLOR_FF00BF: idx = COLOR_PRG_IDX(UX_COLOR_FF00BF); break;
        case COLOR_FF007F: idx = COLOR_PRG_IDX(UX_COLOR_FF007F); break;
        case COLOR_FF003F: idx = COLOR_PRG_IDX(UX_COLOR_FF003F); break;
        case COLOR_FFFFFF: idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF); break;
        default: break;
    }

    return idx;
}





////////////////////////////////////////////////////////////
//
// handler
//
static gboolean post_fixd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkPixbuf *pbuf = NULL;
    GdkGC *gc;
    gint gap_x, gap_y, size_w, size_h;

    NFOBJECT *last_obj;

    if(evt->type == GDK_EXPOSE) 
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        last_obj = nfui_nfobject_get_data(obj, "LAST OBJECT");

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        size_h = last_obj->y + last_obj->height + 10;
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

static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    DVA_COMPONENT_DATA_T *component_data;
    DVA_COMPONENT_ACTION_T *component_action;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *memo = NULL;
    guint x, y;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        if(kpid == KEYPAD_ENTER)
        {
            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }
    
        memo = VirtualKey_Open(top, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_ITXSTYLE_TITLE);
        if(memo) {
            component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
            component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

            strcpy(component_data->line.name, memo);              
            component_action->line_name_cb((gpointer)obj);

            nfui_nflabel_set_text((NFLABEL*)obj, component_data->line.name);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            ifree(memo);
            memo = NULL;
        }

    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_nflabel_set_text((NFLABEL*)obj, component_data->line.name);
    }

    return FALSE;
}

static gboolean post_dis_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    NFOBJECT *top;    
    DVA_COMPONENT_ACTION_T *component_action;
    DVA_COMPONENT_DATA_T *component_data;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    guint x, y;
    GdkColor gdk_color;
    guint color;
    gint color_idx;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {   
        top = nfui_nfobject_get_top(obj);
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_nfobject_get_offset(obj, &x, &y);

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top((gpointer)obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }

        gdk_color = UX_COLOR(component_data->line.display_color_idx);
        
        if (Color_Popup_Open((NFWINDOW *)obj, &gdk_color, (guint)x, (guint)y) == 0) return FALSE;

        color = (gdk_color.red >> 8) & 0x000000ff;
        color |= (gdk_color.green) & 0x0000ff00;
        color |= (gdk_color.blue << 8) & 0x00ff0000;
        color_idx = _convert_rgb_color_to_coloridx(color);

        component_data->line.display_color_idx = color_idx;
        component_action->line_dis_color_cb((gpointer)obj);

        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(color_idx));
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(component_data->line.display_color_idx));
    }

    return FALSE;
//post_dis_color_event_handler
}

static gboolean post_active_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_ACTION_T *component_action;
        DVA_COMPONENT_DATA_T *component_data;
        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->line.active = status;
        component_action->rule_act_cb((gpointer)obj);
    }
    else if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.active);
    }

    return FALSE;
}

static gboolean post_all_detect_object_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gint status;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;
        NFOBJECT *manual_label;
        NFOBJECT *manual_btn;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFBUTTON*)obj);

        component_data->line.all_detect_obj = status;

        human_check = nfui_nfobject_get_data(obj->parent, "human_object_filter");
        car_check = nfui_nfobject_get_data(obj->parent, "car_object_filter");
        bike_check = nfui_nfobject_get_data(obj->parent, "bike_object_filter");
        manual_label = nfui_nfobject_get_data(obj->parent, "manual_object_filter_label");
        manual_btn = nfui_nfobject_get_data(obj->parent, "manual_object_filter_button");
        memset(component_data->line.interest_obj, 0x00, sizeof(component_data->line.interest_obj));

        if (component_data->line.all_detect_obj) 
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)human_check, TRUE);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)car_check, TRUE);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)bike_check, TRUE);
            snprintf(component_data->line.interest_obj, sizeof(component_data->line.interest_obj)-1, "person,car,bike,%s", nfui_nflabel_get_text((NFLABEL*)manual_label));
        }
        else
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)human_check, FALSE);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)car_check, FALSE);
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)bike_check, FALSE);
            snprintf(component_data->line.interest_obj, sizeof(component_data->line.interest_obj)-1, "%s", nfui_nflabel_get_text((NFLABEL*)manual_label));
        }

        nfui_signal_emit(human_check, GDK_EXPOSE, TRUE);
        nfui_signal_emit(car_check, GDK_EXPOSE, TRUE);
        nfui_signal_emit(bike_check, GDK_EXPOSE, TRUE);
        nfui_signal_emit(manual_label, GDK_EXPOSE, TRUE);
        nfui_signal_emit(manual_btn, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.all_detect_obj);
    }

    return FALSE;
}

static gboolean post_object_filter_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;
        NFOBJECT *manual_label;

        int human_active = 0;
        int car_active = 0;
        int bike_active = 0;

        gchar filter_string[256];
        gchar *manual_text;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        human_check = nfui_nfobject_get_data(obj->parent, "human_object_filter");
        car_check = nfui_nfobject_get_data(obj->parent, "car_object_filter");
        bike_check = nfui_nfobject_get_data(obj->parent, "bike_object_filter");
        manual_label = nfui_nfobject_get_data(obj->parent, "manual_object_filter_label");

        memset(filter_string, 0x00, sizeof(filter_string));

        if (human_active = nfui_check_button_get_active((NFCHECKBUTTON*)human_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "person");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "person");
        }

        if (car_active = nfui_check_button_get_active((NFCHECKBUTTON*)car_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "car");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "car");
        }

        if (bike_active = nfui_check_button_get_active((NFCHECKBUTTON*)bike_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "bike");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "bike");
        }

        if (human_active && car_active && bike_active)
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_all_chk_obj, TRUE);
            nfui_signal_emit(g_all_chk_obj, GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_all_chk_obj, FALSE);
            nfui_signal_emit(g_all_chk_obj, GDK_EXPOSE, TRUE);
        }

        manual_text = nfui_nflabel_get_text((NFLABEL*)manual_label);

        if (strlen(manual_text)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", manual_text);
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string-strlen(filter_string)-1), ",%s", manual_text);
        }

        memset(component_data->line.interest_obj, 0x00, sizeof(component_data->line.interest_obj));
        snprintf(component_data->line.interest_obj, sizeof(component_data->line.interest_obj)-1, "%s", filter_string);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;

        gchar tmp_interest[256];
        gchar *interest_token;
        gchar *pbuf = NULL;
        gchar *pnext = NULL;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        g_message("%s, %d, interest_obj:%s", __FUNCTION__, __LINE__, component_data->line.interest_obj);

        human_check = nfui_nfobject_get_data(obj->parent, "human_object_filter");
        car_check = nfui_nfobject_get_data(obj->parent, "car_object_filter");
        bike_check = nfui_nfobject_get_data(obj->parent, "bike_object_filter");

        g_message("%s, %d", __FUNCTION__, __LINE__);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, FALSE);

        memset(tmp_interest, 0x00, sizeof(tmp_interest));
        snprintf(tmp_interest, sizeof(tmp_interest), "%s", component_data->line.interest_obj);

        interest_token = strtok_r(tmp_interest, ",", &pnext);
        pbuf = pnext;

        while (interest_token != 0)
        {
            if (obj == human_check)
            {
                if (!strcmp(interest_token, "person")) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, TRUE);
            }
            else if (obj == car_check)
            {
                if (!strcmp(interest_token, "car")) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, TRUE);
            }
            else if (obj == bike_check)
            {
                if (!strcmp(interest_token, "bike")) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, TRUE);
            }
            interest_token = strtok_r(pbuf, ",", &pnext);
            pbuf = pnext;
        }

        //if (component_data->line.all_detect_obj) nfui_nfobject_disable(obj);
        //else nfui_nfobject_enable(obj);

        g_message("%s, %d", __FUNCTION__, __LINE__);
    }

    return FALSE;
}

static gboolean post_object_filter_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;

        gchar *token_ptr;

        gchar org_filter[256];
        gchar filter_string[256];
        gchar *pbuf = NULL;
        gchar *pnext = NULL;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        memset(org_filter, 0x00, sizeof(org_filter));
        snprintf(org_filter, sizeof(org_filter)-1, ",%s", component_data->line.interest_obj);

        memset(filter_string, 0x00, sizeof(filter_string));

        token_ptr = strtok_r(org_filter, ",", &pnext);
        pbuf = pnext;

        while (token_ptr != 0)
        {
            g_message("%s, %d, token:%s", __FUNCTION__, __LINE__, token_ptr);

            if ((strcmp(token_ptr, "person") != 0) && (strcmp(token_ptr, "car") != 0) && (strcmp(token_ptr, "bike") != 0))
            {
                if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", token_ptr);
                else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", token_ptr);
            }            
            token_ptr = strtok_r(pbuf, ",", &pnext);
            pbuf = pnext;
        }

        nfui_nflabel_set_text((NFLABEL*)obj, filter_string);
    }
    
    return FALSE;
}

static gboolean post_object_filter_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;
        NFOBJECT *manual_label;    

        gchar filter_string[256]; 
        gchar exception_string[64]; 
        gchar *manual_text;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        human_check = nfui_nfobject_get_data(obj->parent, "human_object_filter");
        car_check = nfui_nfobject_get_data(obj->parent, "car_object_filter");
        bike_check = nfui_nfobject_get_data(obj->parent, "bike_object_filter");
        manual_label = nfui_nfobject_get_data(obj->parent, "manual_object_filter_label");

        memset(filter_string, 0x00, sizeof(filter_string));
        snprintf(filter_string, sizeof(filter_string), "%s", nfui_nflabel_get_text((NFLABEL*)manual_label));
        
        memset(exception_string, 0x00, sizeof(exception_string));
        snprintf(exception_string, sizeof(exception_string), "%s", "person,car,bike,");

        vw_dvabx_object_filter_edit_popup_open((NFWINDOW*)top, 1300, 310, exception_string, filter_string, sizeof(filter_string)-sizeof(exception_string));

        nfui_nflabel_set_text((NFLABEL*)manual_label, filter_string);
        nfui_signal_emit(manual_label, GDK_EXPOSE, TRUE);

        memset(filter_string, 0x00, sizeof(filter_string));

        if (nfui_check_button_get_active((NFCHECKBUTTON*)human_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "person");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "person");
        }

        if (nfui_check_button_get_active((NFCHECKBUTTON*)car_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "car");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "car");
        }

        if (nfui_check_button_get_active((NFCHECKBUTTON*)bike_check)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", "bike");
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", "bike");
        }

        manual_text = nfui_nflabel_get_text((NFLABEL*)manual_label);

        if (strlen(manual_text)) {
            if (strlen(filter_string) == 0) snprintf(filter_string, sizeof(filter_string)-1, "%s", manual_text);
            else snprintf(filter_string+strlen(filter_string), sizeof(filter_string)-strlen(filter_string)-1, ",%s", manual_text);
        }

        memset(component_data->line.interest_obj, 0x00, sizeof(component_data->line.interest_obj));
        snprintf(component_data->line.interest_obj, sizeof(component_data->line.interest_obj)-1, "%s", filter_string);     
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        //if (component_data->line.all_detect_obj) nfui_nfobject_disable(obj);
        //else nfui_nfobject_enable(obj);
    }
    
    return FALSE;
}

static gboolean post_audio_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        NFOBJECT *human_check;
        NFOBJECT *car_check;
        NFOBJECT *bike_check;
        NFOBJECT *manual_label;
        gchar addr[8][65];

        gchar filter_string[256]; 
        gchar exception_string[64];         
        gchar *manual_text;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if ((component_data->line.all_detect_obj == 0) && (strlen(component_data->line.interest_obj) == 0))
        {
            nftool_mbox((NFWINDOW*)top, "NOTICE", "Please select at least one object.", NFTOOL_MB_OK);
            return FALSE;
        }

        if ((component_data->line.forward == 0) && (component_data->line.reverse == 0))
        {
            nftool_mbox((NFWINDOW*)top, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
            return FALSE;
        }

        VW_EvtNoti_Audio_Edit(top, component_data);
    }

    return FALSE;
}

static gboolean post_forward_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->line.forward = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.forward);
    }

    return FALSE;
}

static gboolean post_reverse_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->line.reverse = status;
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.reverse);
    }

    return FALSE;
}

static gboolean post_size_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *width_obj[2] = {NULL, };
        NFOBJECT *height_obj[2] = {NULL, };

        gint status, i;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->line.use_filter_size = status;
            
        width_obj[0] = nfui_nfobject_get_data(obj->parent, "WIDTH MIN");
        width_obj[1] = nfui_nfobject_get_data(obj->parent, "WIDTH MAX");
        height_obj[0] = nfui_nfobject_get_data(obj->parent, "HEIGHT MIN");
        height_obj[1] = nfui_nfobject_get_data(obj->parent, "HEIGHT MAX");
    
        for(i = 0; i < 2; i++)
        {
            if (!width_obj[i]) return FALSE;
            if (!height_obj[i]) return FALSE;
        }

        if(component_data->calibration.param_valid)
        {
            nfui_nfobject_enable(obj);
            if(component_data->line.use_filter_size)
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_enable(width_obj[i]);
                    nfui_nfobject_enable(height_obj[i]);

                    nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                    nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
                }
            }
            else
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_disable(width_obj[i]);
                    nfui_nfobject_disable(height_obj[i]);

                    nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                    nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
                }
            }
        }
        else 
        {
            nfui_nfobject_disable(obj);
            for(i = 0; i < 2; i++)
            {
                nfui_nfobject_disable(width_obj[i]);
                nfui_nfobject_disable(height_obj[i]);

                nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
            }
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *width_obj[2] = {NULL, };
        NFOBJECT *height_obj[2] = {NULL, };
        gint i;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.use_filter_size);

        width_obj[0] = nfui_nfobject_get_data(obj->parent, "WIDTH MIN");
        width_obj[1] = nfui_nfobject_get_data(obj->parent, "WIDTH MAX");
        height_obj[0] = nfui_nfobject_get_data(obj->parent, "HEIGHT MIN");
        height_obj[1] = nfui_nfobject_get_data(obj->parent, "HEIGHT MAX");

        for(i = 0; i < 2; i++)
        {
            if (!width_obj[i]) return FALSE;
            if (!height_obj[i]) return FALSE;
        }

        if(component_data->calibration.param_valid)
        {
            nfui_nfobject_enable(obj);
            if(component_data->line.use_filter_size)
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_enable(width_obj[i]);
                    nfui_nfobject_enable(height_obj[i]);

                    nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                    nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
                }
            }
            else
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_disable(width_obj[i]);
                    nfui_nfobject_disable(height_obj[i]);

                    nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                    nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
                }
            }
        }
        else 
        {
            nfui_nfobject_disable(obj);
            for(i = 0; i < 2; i++)
            {
                nfui_nfobject_disable(width_obj[i]);
                nfui_nfobject_disable(height_obj[i]);

                nfui_signal_emit(width_obj[i], GDK_EXPOSE, TRUE);
                nfui_signal_emit(height_obj[i], GDK_EXPOSE, TRUE);
            }
        }
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }

    return FALSE;
}

static gboolean post_speed_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *speed_obj[2] = {NULL,};

        gint status,i;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->line.use_filter_speed = status;

        speed_obj[0] = nfui_nfobject_get_data(obj->parent, "SPEED MIN");
        speed_obj[1] = nfui_nfobject_get_data(obj->parent, "SPEED MAX");
        
        for(i = 0; i < 2; i++)
        {
            if(speed_obj[i] == NULL)
                return 0;
        }

        if(component_data->calibration.param_valid)
        {
            nfui_nfobject_enable(obj);
            
            if(component_data->line.use_filter_speed)
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_enable(speed_obj[i]);
                }
            }
            else
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_disable(speed_obj[i]);
                }
            }
        }
        else 
        {
            nfui_nfobject_disable(obj);
            
            for(i = 0; i < 2; i++)
            {
                nfui_nfobject_disable(speed_obj[i]);
            }
        }
        nfui_signal_emit(speed_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(speed_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *speed_obj[2] = {NULL,};
        gint i;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->line.use_filter_speed);

        speed_obj[0] = nfui_nfobject_get_data(obj->parent, "SPEED MIN");
        speed_obj[1] = nfui_nfobject_get_data(obj->parent, "SPEED MAX");

        if(component_data->calibration.param_valid)
        {
            nfui_nfobject_enable(obj);
            
            if(component_data->line.use_filter_speed)
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_enable(speed_obj[i]);
                }
            }
            else
            {
                for(i = 0; i < 2; i++)
                {
                    nfui_nfobject_disable(speed_obj[i]);
                }
            }
        }
        else 
        {
            nfui_nfobject_disable(obj);
            
            for(i = 0; i < 2; i++)
            {
                nfui_nfobject_disable(speed_obj[i]);
            }
        }
        nfui_signal_emit(speed_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(speed_obj[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

    }

    return FALSE;
}

static gboolean post_size_w_min_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        NFOBJECT *max_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        gdouble value;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        
        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        component_data->line.filter_min_size_w = _prvValueToMillimeter(index);
        
        if(get_unit_change_yard())
        {
            value = ifn_unit_change((gdouble)component_data->line.filter_min_size_w/10,0);
            component_data->line.filter_min_size_w = (value*10);
        }
        else
        {
            if(index > 6552){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_min_size_w = 0;
            }
        }
       
        if(component_data->line.filter_min_size_w >= component_data->line.filter_max_size_w)
        {
            max_obj = nfui_nfobject_get_data(obj->parent, "WIDTH MAX");

            nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, index);
            index++;
            component_data->line.filter_max_size_w = _prvValueToMillimeter(index);
        }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
    	gchar buf[5];
    	gchar *strTemp;
        NFOBJECT *top;
        NFOBJECT *max_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);


        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

        //numTemp = NumberKey_Open(top, index, x, y, 6552);
		memset(buf, 0x00, sizeof(buf));	
        gcvt(index*0.01, 3, buf);
        
		strTemp = Real_NumberKey_Str_Open(top, buf, x, y, "65.52");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);
            
            component_data->line.filter_min_size_w = _prvValueToMillimeter(numTemp);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp);

            if(get_unit_change_yard())
            {   
                gdouble value;
                value = ifn_unit_change((gdouble)component_data->line.filter_min_size_w/10,0);
                component_data->line.filter_min_size_w = (value*10);
            }

            if(component_data->line.filter_min_size_w >= component_data->line.filter_max_size_w) 
            {
                max_obj = nfui_nfobject_get_data(obj->parent, "WIDTH MAX");
                nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, numTemp);
                
                numTemp++;
                component_data->line.filter_max_size_w = _prvValueToMillimeter(numTemp);
            }
            ifree(strTemp);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gdouble index;
            index = ifn_unit_change((gdouble)component_data->line.filter_min_size_w/10,1);
            nfui_spin_button_set_index((NFSPINBUTTON *)obj,(int)index);
        }
        else
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)(_prvMillimeterToIndex(component_data->line.filter_min_size_w)));
    }

    return FALSE;
}

static gboolean post_size_w_max_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON *) obj);    
        component_data->line.filter_max_size_w = _prvValueToMillimeter(index+1);

        if(get_unit_change_yard())
        {
            gdouble value;
            value = ifn_unit_change((gdouble)component_data->line.filter_max_size_w/10,0);
            component_data->line.filter_max_size_w = (value *10);
        }
        else
        {
            if(index > 6552){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_max_size_w = 10; 
            }
        }
        
        if(component_data->line.filter_max_size_w <= component_data->line.filter_min_size_w)
        {          
            min_obj = nfui_nfobject_get_data(obj->parent, "WIDTH MIN");

            if(index >6552) index = 0;
            
            nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, index);          
            component_data->line.filter_min_size_w = _prvValueToMillimeter(index);
       }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
    	gchar buf[5];
    	gchar *strTemp;
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        //numTemp = NumberKey_Open(top, index+1, x, y, 6553);
		memset(buf, 0x00, sizeof(buf));	
        gcvt((index+1)*0.01, 3, buf);
        
		strTemp = Real_NumberKey_Str_Open(top, buf, x, y, "65.53");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);
    		
            component_data->line.filter_max_size_w = _prvValueToMillimeter(numTemp);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp-1);

            if(get_unit_change_yard())
            {
                gdouble value;
                value = ifn_unit_change((gdouble)component_data->line.filter_max_size_w/10,0);
                component_data->line.filter_max_size_w = (value *10);
            }
            
            if(component_data->line.filter_max_size_w <= component_data->line.filter_min_size_w) 
            {
                min_obj = nfui_nfobject_get_data(obj->parent, "WIDTH MIN");

                nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, numTemp-1);
                component_data->line.filter_min_size_w = _prvValueToMillimeter(numTemp-1);
            }
             ifree(strTemp);
       }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gdouble index;
            index = ifn_unit_change((gdouble)(component_data->line.filter_max_size_w)/10, 1);
            nfui_spin_button_set_index((NFSPINBUTTON *)obj, (int)index-1);
        }
        else
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, (int)(_prvMillimeterToIndex(component_data->line.filter_max_size_w)-1));
    }

    return FALSE;
}

static gboolean post_size_h_min_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        NFOBJECT *max_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        component_data->line.filter_min_size_h = _prvValueToMillimeter(index);

        if(get_unit_change_yard())
        {
            gdouble value;
            value = ifn_unit_change((gdouble)component_data->line.filter_min_size_h/10, 0);
            component_data->line.filter_min_size_h = (value*10);
        }
        else
        {
            if(index > 6552){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_min_size_h = 0; 
            }
        }

        if(component_data->line.filter_min_size_h >= component_data->line.filter_max_size_h)
        {
            max_obj = nfui_nfobject_get_data(obj->parent, "HEIGHT MAX");

            nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, index);
            index++;
            component_data->line.filter_max_size_h = _prvValueToMillimeter(index);
        }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
    	gchar buf[5];
    	gchar *strTemp;
        NFOBJECT *top;
        NFOBJECT *max_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        //numTemp = NumberKey_Open(top, index, x, y, 6552);
		memset(buf, 0x00, sizeof(buf));	
        gcvt(index*0.01, 3, buf);
        
		strTemp = Real_NumberKey_Str_Open(top, buf, x, y, "65.52");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);
            
            component_data->line.filter_min_size_h = _prvValueToMillimeter(numTemp);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp);

            if(get_unit_change_yard())
            {
                gdouble value;
                value = ifn_unit_change((gdouble)component_data->line.filter_min_size_h/10, 0);
                component_data->line.filter_min_size_h = (value*10);
            }

            if(component_data->line.filter_min_size_h >= component_data->line.filter_max_size_h) 
            {
                max_obj = nfui_nfobject_get_data(obj->parent, "HEIGHT MAX");

                nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, numTemp);
                numTemp++;
                component_data->line.filter_max_size_h = _prvValueToMillimeter(numTemp);
            }
            ifree(strTemp);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gdouble value;
            value = ifn_unit_change((gdouble)component_data->line.filter_min_size_h/10, 1);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)value);
        }            
        else 
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, (int)(_prvMillimeterToIndex(component_data->line.filter_min_size_h)));
    }

    return FALSE;
}

static gboolean post_size_h_max_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON *) obj);     
        component_data->line.filter_max_size_h = _prvValueToMillimeter(index+1);

        if(get_unit_change_yard())
        {
            gdouble value;
            value = ifn_unit_change((gdouble)component_data->line.filter_max_size_h/10, 0);
            component_data->line.filter_max_size_h = (value*10);
        }
        else
        {
            if(index > 6552){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_max_size_h = 10; 
            }
        }

        if(component_data->line.filter_max_size_h <= component_data->line.filter_min_size_h)
        {          
            min_obj = nfui_nfobject_get_data(obj->parent, "HEIGHT MIN");

            if(index >6552) index = 0;
            nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, index);
            component_data->line.filter_min_size_h = _prvValueToMillimeter(index);
        }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
    	gchar buf[5];
    	gchar *strTemp;
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        //numTemp = NumberKey_Open(top, index+1, x, y, 6553);
 		memset(buf, 0x00, sizeof(buf));	
        gcvt((index+1)*0.01, 3, buf);
        
		strTemp = Real_NumberKey_Str_Open(top, buf, x, y, "65.53");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);

            component_data->line.filter_max_size_h = _prvValueToMillimeter(numTemp);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp-1);

            if(get_unit_change_yard())
            {
                gdouble value;
                value = ifn_unit_change((gdouble)component_data->line.filter_max_size_h/10, 0);
                component_data->line.filter_max_size_h =(value*10);
            }

            if(component_data->line.filter_max_size_h <= component_data->line.filter_min_size_h) 
            {
                min_obj = nfui_nfobject_get_data(obj->parent, "HEIGHT MIN");

                nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, numTemp-1);
                component_data->line.filter_min_size_h = _prvValueToMillimeter(numTemp-1);
            }
            ifree(strTemp);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gdouble value;
            value = ifn_unit_change((gdouble)component_data->line.filter_max_size_h/10, 1);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)value-1);
        }
        else
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, (int)(_prvMillimeterToIndex(component_data->line.filter_max_size_h)-1));
    }

    return FALSE;
}

static gboolean post_speed_unit_change_label_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        if(get_unit_change_yard()) nfui_nflabel_set_text((NFLABEL*)obj,"mi/h");
        else nfui_nflabel_set_text((NFLABEL *) obj, "km/h");

    }
    return FALSE;
}


static gboolean post_speed_min_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        NFOBJECT *max_obj;
        DVA_COMPONENT_DATA_T *component_data;

        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj); 
        component_data->line.filter_min_speed = index+1;
               
        if(get_unit_change_yard())
        {
            gfloat value;
            value = ifn_unit_change_speed((gdouble)component_data->line.filter_min_speed,0);
            component_data->line.filter_min_speed = value+1;

            if(index > 157){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_min_speed = 0; 
            } 
        }
        else
        {
            if(index > 254){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_min_speed = 0; 
            }   
        }

        if(component_data->line.filter_min_speed >= component_data->line.filter_max_speed)
        {
            component_data->line.filter_max_speed = index+1;
            max_obj = nfui_nfobject_get_data(obj->parent, "SPEED MAX");
            nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, index);
        }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *max_obj;
        guint x, y;
        gint numTemp;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);


        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        numTemp  = NumberKey_Open(top, nfui_spin_button_get_value((NFSPINBUTTON*)obj), x, y, 254);
        component_data->line.filter_min_speed = numTemp;
        nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp);

        if(get_unit_change_yard())
        {
            gfloat value;
            value = ifn_unit_change_speed((gdouble)component_data->line.filter_min_speed,0);
            component_data->line.filter_min_speed = value-1;
        }
        
        if(component_data->line.filter_min_speed >= component_data->line.filter_max_speed)
        {
            component_data->line.filter_max_speed = numTemp+1;
            max_obj = nfui_nfobject_get_data(obj->parent, "SPEED MAX");
            nfui_spin_button_set_index((NFSPINBUTTON*)max_obj, numTemp);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gfloat index;            
            index = ifn_unit_change_speed((gdouble)component_data->line.filter_min_speed,1);           
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)index-1);
        }
        else
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)component_data->line.filter_min_speed-1);
         
        //nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, component_data->line.filter_min_speed);
    }

    return FALSE;
}

static gboolean post_speed_max_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        NFOBJECT *min_obj;

        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);  
        component_data->line.filter_max_speed = index+1;

        if(get_unit_change_yard())
        {
            gfloat value;
            value = ifn_unit_change_speed((gdouble)component_data->line.filter_max_speed,0);
            component_data->line.filter_max_speed = value+1;

            if(index > 158){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_max_speed = 0; 
            }
        }
        else
        {
            if(index > 255){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                component_data->line.filter_max_speed = 0; 
            }
        }

        if(component_data->line.filter_max_speed <= component_data->line.filter_min_speed)
        {
            if(component_data->line.filter_max_speed ==  0) index = 0;
        
            component_data->line.filter_min_speed = index;
            min_obj = nfui_nfobject_get_data(obj->parent, "SPEED MIN");
            nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, index);
        }
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        gint numTemp;
        NFOBJECT *top;
        NFOBJECT *min_obj;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);


        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;
        
        numTemp = NumberKey_Open(top, nfui_spin_button_get_value((NFSPINBUTTON*)obj), x, y, 255);
        
        if(numTemp == 0)
            return FALSE;
            
        component_data->line.filter_max_speed = numTemp;
        nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp-1);

        if(get_unit_change_yard())
        {
            gfloat value;
            value = ifn_unit_change_speed((gdouble)component_data->line.filter_max_speed,0);
            component_data->line.filter_max_speed =  value;
        }
        
        if(component_data->line.filter_max_speed <= component_data->line.filter_min_speed)
        {
            component_data->line.filter_min_speed = numTemp-1;
            min_obj = nfui_nfobject_get_data(obj->parent, "SPEED MIN");
            nfui_spin_button_set_index((NFSPINBUTTON*)min_obj, numTemp-1);
        }

    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard())
        {
            gfloat index;
            index = ifn_unit_change_speed((gdouble)component_data->line.filter_max_speed,1);
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)(index)-1);
        }
        else
            nfui_spin_button_set_index((NFSPINBUTTON*)obj,(int)(component_data->line.filter_max_speed)-1);
         
    }

    return FALSE;
}

static gboolean post_height_unit_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        
        if(get_unit_change_yard()) nfui_nflabel_set_text((NFLABEL*)obj,"ft");
        else nfui_nflabel_set_text((NFLABEL *) obj, "m");

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }  
    return FALSE;
}

static gboolean post_width_unit_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if(get_unit_change_yard()) nfui_nflabel_set_text((NFLABEL*)obj,"ft");
        else nfui_nflabel_set_text((NFLABEL *) obj, "m");

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}

static gboolean post_counter_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    DVA_COMPONENT_ACTION_T *component_action;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_action = (DVA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_ACTION));
        component_action->line_connected_counter_cb(obj);
    }

    return FALSE;
}

static gboolean post_event_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
     if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        gint value;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        value = nfui_spin_button_get_value((NFSPINBUTTON*)obj);
        //component_data->area.stopped_val = value;
        component_data->line.c_threshold = value;
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        gint numTemp;
        guint x, y;
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);


        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;
        
        numTemp = NumberKey_Open(top, nfui_spin_button_get_value((NFSPINBUTTON*)obj), x, y, 600);
        /*
        if(numTemp < 5) 
            numTemp = 0;
        else
            numTemp -= 5;
         */  
        nfui_spin_button_set_index((NFSPINBUTTON*)obj, numTemp-1);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        //component_data->area.stopped_val = nfui_spin_button_get_value((NFSPINBUTTON*)obj);
        component_data->line.c_threshold = nfui_spin_button_get_value((NFSPINBUTTON*)obj);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        gint value;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        value = nfui_spin_button_get_value((NFSPINBUTTON*)obj);
        //g_message("%s(%d) ==> %d, %d", __FUNCTION__, __LINE__, value, component_data->area.c_threshold);
        if(component_data->line.c_threshold != value)
        {
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (component_data->line.c_threshold)-1);
        }
    }
    
    return FALSE;
}

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_dvabx_rule_line_component_open(NFOBJECT *parent, guint opt)
{
    NFOBJECT *obj;  
    NFOBJECT *check_btn;
    NFOBJECT *fixed;
    gint size_w, size_h;

    gint pos_x, pos_y;
    gchar strBuf[32];
    gchar lfBuf[4096];

    
    
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, parent->width-10, parent->height-90);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 5, 20);
    nfui_regi_post_event_callback(fixed, post_fixd_event_handler);

    pos_x = 20;
    pos_y = 0;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_y += 40;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("NAME"));
    strcat(strBuf, " : ");
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 100, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x += 100;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 300, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    pos_x = 20;
    pos_y += 45;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("DISPLAY COLOR"));
    strcat(strBuf, " : ");

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x += 200;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 30, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_dis_color_event_handler);
    pos_x = 20;
    pos_y += 45;

    if (opt & (1 << OPT_LINE_ACTIVE_CHECK))
    {
        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, 290, 40+45);
        nfui_regi_post_event_callback(obj, post_active_check_event_handler);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTIVE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 150, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  320, 40+45);
    }

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("OBJECT"));
    strcat(strBuf, " : ");

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x += 20;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_all_detect_object_event_handler);
    g_all_chk_obj = obj;
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 150, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x = 40;
    pos_y += 40;    

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_object_filter_check_event_handler);
    nfui_nfobject_set_data(fixed, "human_object_filter", obj);
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HUMAN", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 150, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x += 180;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_object_filter_check_event_handler);
    nfui_nfobject_set_data(fixed, "car_object_filter", obj);
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 180, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    pos_x = 40;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_object_filter_check_event_handler);
    nfui_nfobject_set_data(fixed, "bike_object_filter", obj);    
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 180, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    pos_x = 40;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_set_size(obj, 330, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);  	
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_object_filter_label_event_handler);
    nfui_nfobject_set_data(fixed, "manual_object_filter_label", obj);
 
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("EDIT", 120);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+334, pos_y); 
    nfui_regi_post_event_callback(obj, post_object_filter_edit_button_event_handler);
    nfui_nfobject_set_data(fixed, "manual_object_filter_button", obj);
 
    pos_x = 20;
    pos_y += 45;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("EVENT"));
    strcat(strBuf, " : ");
 
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x += 20;
    pos_y += 40;


    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_forward_check_event_handler);
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FORWARD DIRECTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 250, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x = 40;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_reverse_check_event_handler);
    pos_x += 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REVERSE DIRECTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 250, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_x = 40;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("Select an audio file to link to the event.", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_set_size(obj, 330, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);  	
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    //nfui_regi_post_event_callback(obj, post_audio_label_event_handler);
    //nfui_nfobject_set_data(fixed, "manual_object_filter_label", obj);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("SELECT", 120);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+334, pos_y); 
    nfui_regi_post_event_callback(obj, post_audio_edit_button_event_handler);
    //nfui_nfobject_set_data(fixed, "manual_object_filter_button", obj);
    pos_x = 20;
    pos_y += 45;

    if (ivsc.dfunc.ai.support_calibration)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%s", lookup_string("FILTER"));
        strcat(strBuf, " : ");

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 20;
        pos_y += 40;

        pos_x = 40;
        
        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
        nfui_regi_post_event_callback(obj, post_size_check_event_handler);
        pos_x += 30;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SIZE(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 110, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x = 175;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WIDTH", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 80, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 80;
        
        obj = nfui_spinbutton_new_value_with_range(0, 0.00, 214.96, 0.01); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
        nfui_regi_post_event_callback(obj, post_size_w_min_event_handler);
        nfui_nfobject_set_data(fixed,"WIDTH MIN", obj);
        pos_x += 85;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 20;

        obj = nfui_spinbutton_new_value_with_range(0.01, 0.01, 214.99, 0.01); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
        nfui_regi_post_event_callback(obj, post_size_w_max_event_handler);
        nfui_nfobject_set_data(fixed, "WIDTH MAX", obj);
        pos_x += 85; 

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        nfui_regi_post_event_callback( obj, post_width_unit_label_event_handler );
        pos_x = 175;
        pos_y += 40;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HEIGHT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 80, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 80;
        
        obj = nfui_spinbutton_new_value_with_range(0, 0.00, 214.96, 0.01); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
        nfui_regi_post_event_callback(obj, post_size_h_min_event_handler);
        nfui_nfobject_set_data(fixed, "HEIGHT MIN", obj);
        pos_x += 85;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 20;

        obj = nfui_spinbutton_new_value_with_range(0.01, 0.01, 214.99, 0.01); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
        nfui_regi_post_event_callback(obj, post_size_h_max_event_handler);
        nfui_nfobject_set_data(fixed, "HEIGHT MAX", obj);
        pos_x += 85; 

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        nfui_regi_post_event_callback(obj,post_height_unit_label_event_handler);

        pos_x = 40;
        pos_y += 40;
    
        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
        nfui_regi_post_event_callback(obj, post_speed_check_event_handler);
        pos_x += 30;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SPEED(3D)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 150, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x = 165 + 90;
        
        obj = nfui_spinbutton_new_value_with_range(0, 0, 254, 1); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);  
        nfui_regi_post_event_callback(obj, post_speed_min_event_handler);
        nfui_nfobject_set_data(fixed, "SPEED MIN", obj);
        pos_x += 85;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        pos_x += 20;

        obj = nfui_spinbutton_new_value_with_range(1, 1, 255, 1); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
        nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nfobject_set_size(obj, 85, 30);
        nfui_nfobject_show(obj);                
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
        nfui_regi_post_event_callback(obj, post_speed_max_event_handler);
        nfui_nfobject_set_data(fixed, "SPEED MAX", obj);
        pos_x += 85; 

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("km/h", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 20, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        nfui_regi_post_event_callback(obj, post_speed_unit_change_label_handler);
        nfui_nfobject_set_data(fixed, "LAST OBJECT", obj);
    }
    //CONFIDENCE THRESHOLD

    pos_x = 20;
    pos_y += 45;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("CONFIDENCE THRESHOLD"));
    strcat(strBuf, " : ");

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 300, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);

    obj = nfui_spinbutton_new_value_with_range(1, 1, 100, 1);  
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
    nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nfobject_set_size(obj, 85, 30);
    nfui_nfobject_show(obj);                
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+320, pos_y);  
    nfui_regi_post_event_callback(obj, post_event_spin_event_handler);
    //nfui_nfobject_set_data(fixed, "CONFIDENCE THRESHOLD", obj);
    nfui_nfobject_set_data(fixed, "LAST OBJECT", obj);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("%", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 20, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x+415, pos_y);

    if (opt & (1 << OPT_LINE_CONNECTED_COUNTER))
    {
        pos_y += 70;

        obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("ADD COUNTER", 230);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, DEFAULT_COMPONENT_WIDTH-250, pos_y); 
        nfui_regi_post_event_callback(obj, post_counter_add_button_event_handler);
    }

    if (opt & (1 << OPT_LINE_SETUP_HELP))
    {
        pos_x = 20;
        pos_y = DEFAULT_COMPONENT_HEIGHT - 160-20;
    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("[?]", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, 40, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("What is the filter?", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-60, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+40, pos_y);

        pos_y += 40;

        memset(lfBuf, 0x00, sizeof(lfBuf));
        nfutil_get_line_feed_string(lookup_string(HELP_TEXT), DEFAULT_COMPONENT_WIDTH-pos_x-30, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-20, 100);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);       
    }

    return 0;
}

gint vw_dvabx_rule_line_component_show()
{

    return 0;
}

gint vw_dvabx_rule_line_component_hide()
{

    return 0;
}

gint vw_dvabx_rule_line_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_rule_line_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}
