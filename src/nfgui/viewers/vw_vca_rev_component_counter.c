/*
 * vw_vca_rev_component_counter.c
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
#include "objects/nfspinbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_vca_rev_component.h"
#include "vw_vkeyboard.h"
#include "vaa.h"
#include "vaa_itx.h"


typedef struct _COUNTER_DATA_T
{
    gint        cnt;
    NFOBJECT    *counter_obj[10];
}COUNTER_DATA_T;




////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
#define HELP_TEXT1       "A counter can display the number of times a VA event has occurred. A counter event can be set to trigger if the particular VA event reaches a specified number."
#define HELP_TEXT2       "If Reset value is selected the counter will reset after a counter event occurs."





////////////////////////////////////////////////////////////
//
// private interfaces 
//
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

static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    VCA_COMPONENT_DATA_T *component_data;
    VCA_COMPONENT_ACTION_T *component_action;

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
            component_action = (VCA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_ACTION));
            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
              
            strcpy(component_data->counter.name, memo);
            component_action->counter_name_cb(obj);

            nfui_nflabel_set_text((NFLABEL*)obj, memo);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            ifree(memo);
            memo = NULL;
        }

    }
    else if(evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_nflabel_set_text((NFLABEL*)obj, component_data->counter.name);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_dis_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    NFOBJECT *top;    
    VCA_COMPONENT_ACTION_T *component_action;
    VCA_COMPONENT_DATA_T *component_data;
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
        component_action = (VCA_COMPONENT_ACTION_T*)(nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_ACTION));
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_nfobject_get_offset(obj, &x, &y);

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

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

        gdk_color = UX_COLOR(component_data->counter.display_color_idx);
        
        if (Color_Popup_Open((NFWINDOW *)obj, &gdk_color, (guint)x, (guint)y) == 0) return FALSE;

        color = (gdk_color.red >> 8) & 0x000000ff;
        color |= (gdk_color.green) & 0x0000ff00;
        color |= (gdk_color.blue << 8) & 0x00ff0000;
        color_idx = _convert_rgb_color_to_coloridx(color);

        component_data->counter.display_color_idx = color_idx;
        component_action->counter_dis_color_cb((gpointer)obj);
        
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(color_idx));
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if(evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(component_data->counter.display_color_idx));
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);        
    }

    return FALSE;
//post_dis_color_event_handler
}

static gboolean post_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VCA_COMPONENT_ACTION_T *component_action;
        COUNTER_DATA_T *counter_data;
        gint index,i;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
        component_action = (VCA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_ACTION);
        
        index = nfui_radio_button_get_index((NFBUTTON*)obj);         
        index = (index == 1)? 0:1;
        component_data->use_counter = index;

        counter_data = nfui_nfobject_get_data(obj->parent, "USE COUNTER");        
        if (counter_data == NULL) return FALSE;
        
        for (i = 0; i < counter_data->cnt; i++)
        {
            if (component_data->use_counter)
            {
                nfui_nfobject_enable(counter_data->counter_obj[i]);
            }
            else
            {
               nfui_nfobject_disable(counter_data->counter_obj[i]);
            }
            
            nfui_signal_emit(counter_data->counter_obj[i], GDK_EXPOSE, TRUE);
        }

        component_action->counter_use_cb(obj);
    }
    else if(evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        COUNTER_DATA_T *counter_data;

        gint index,i;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_radio_button_get_index((NFBUTTON*)obj);
    
        index = (index == 1)? 0:1;
        if (component_data->use_counter == index)
        {
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }   
        
        counter_data = nfui_nfobject_get_data(obj->parent, "USE COUNTER");        
        if (counter_data == NULL) return FALSE;
        
        for (i = 0; i < counter_data->cnt; i++)
        {
            if (component_data->use_counter)
            {
                nfui_nfobject_enable(counter_data->counter_obj[i]);
            }
            else
            {
                nfui_nfobject_disable(counter_data->counter_obj[i]);
            }
            
            nfui_signal_emit(counter_data->counter_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_active_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VCA_COMPONENT_ACTION_T *component_action;
        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
        component_action = (VCA_COMPONENT_ACTION_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_ACTION);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->counter.active = status;
        component_action->rule_act_cb(obj);
    }
    else if(evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        gint status;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->counter.active);
    }

    return FALSE;
}

static gboolean post_event_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        COUNTER_DATA_T *counter_data;

        gint status;
        gint i;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->counter.use_counter_event = status;

        counter_data = nfui_nfobject_get_data(obj, "COUNTER EVENT");
        if (counter_data == NULL) return FALSE;

        for (i = 0; i < counter_data->cnt; i++)
        {
            if (component_data->counter.use_counter_event && (!nfui_nfobject_is_disabled(obj)))
            {
                nfui_nfobject_enable(counter_data->counter_obj[i]);
            }
            else
            {
               nfui_nfobject_disable(counter_data->counter_obj[i]);
            }
            
            nfui_signal_emit(counter_data->counter_obj[i], GDK_EXPOSE, TRUE);
        }
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        COUNTER_DATA_T *counter_data;
        gint i;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->counter.use_counter_event);

        counter_data = nfui_nfobject_get_data(obj, "COUNTER EVENT");
        if (counter_data == NULL) return FALSE;
        
        for (i = 0; i < counter_data->cnt; i++)
        {
            if (component_data->counter.use_counter_event && (!nfui_nfobject_is_disabled(obj)))
            {
                nfui_nfobject_enable(counter_data->counter_obj[i]);
            }
            else
            {
               nfui_nfobject_disable(counter_data->counter_obj[i]);
            }
            
            nfui_signal_emit(counter_data->counter_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_reset_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        gint status;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        status = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        component_data->counter.use_reset_value = status;
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
       
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, component_data->counter.use_reset_value);
    }

    return FALSE;
}

static gboolean post_event_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        gint index;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

        component_data->counter.counter_event_val = index;
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);


        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        component_data->counter.counter_event_val = NumberKey_Open(top, nfui_nflabel_get_number((NFLABEL*)obj), x, y, 32767);
        
        if(component_data->counter.counter_event_val >= 0) 
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, component_data->counter.counter_event_val);
            
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gint index;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

        if(component_data->counter.counter_event_val != index)
        {
             nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, component_data->counter.counter_event_val);
        }
    }

    return FALSE;
}

static gboolean post_source_up_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gchar sbuf[64] = {NULL,};
        gint i,cnt = 0;
        gint index;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    	index = nfui_combobox_get_cur_index(obj);
    	
        if(index == 0)
        {
           component_data->counter.up_source = -1;
        }
        else
        {
            for (i = 0; i < 16; i++)
            {  
                if(strcmp(component_data->counter.source_list[i], "") != 0)
                    cnt++;

                if(cnt == index)
                {
                    component_data->counter.up_source = i;
                    break;
                }
            }
        }
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gchar sbuf[64] = {NULL,};
        gint index,i;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        nfui_combobox_remove_all(obj);
        
        nfui_combobox_append_data(NF_COMBOBOX(obj), "Disabled");
        for (i = 0; i < 16; i++)
        {
            if(strcmp(component_data->counter.source_list[i], "") != 0)
            {
                sprintf(sbuf, "Zone[%d] %s", i, component_data->counter.source_list[i]);   
        		nfui_combobox_append_data(NF_COMBOBOX(obj), sbuf);
            }
        }

        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, (component_data->counter.up_source)+1);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);   
    }

    return FALSE;
}

static gboolean post_source_down_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gchar sbuf[64] = {NULL,};
        gint i,cnt = 0;
        gint index;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    	index = nfui_combobox_get_cur_index(obj);
    	
        if(index == 0)
        {
           component_data->counter.down_source = -1;
        }
        else
        {
            for (i = 0; i < 16; i++)
            {  
                if(strcmp(component_data->counter.source_list[i], "") != 0)
                    cnt++;

                if(cnt == index)
                {
                    component_data->counter.down_source = i;
                    break;
                }
            }
        }
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gchar sbuf[64] = {NULL,};
        gint index,i;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        nfui_combobox_remove_all(obj);
        
        nfui_combobox_append_data(NF_COMBOBOX(obj), "Disabled");
        for (i = 0; i < 16; i++)
        {
            if(strcmp(component_data->counter.source_list[i], "") != 0)
            {
                sprintf(sbuf, "Zone[%d] %s", i, component_data->counter.source_list[i]);   
        		nfui_combobox_append_data(NF_COMBOBOX(obj), sbuf);
            }
        }

        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, (component_data->counter.down_source)+1);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);   
    }

    return FALSE;
}






////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_rev_counter_component_open(NFOBJECT *parent, guint opt)
{
    NFOBJECT *top;
    NFOBJECT *obj;
    NFOBJECT *fixed;
    NFOBJECT *fixed_title;
    NFOBJECT *radio_obj;
    NFOBJECT *check_obj;
    GSList *list = NULL;
    COUNTER_DATA_T *use_counter_data = 0;
    COUNTER_DATA_T *use_event_data = 0;

    VCA_COMPONENT_DATA_T *component_data;
    ITX_VAZONE_CONF zone_conf;
    ITX_VAZONE_SHAPE zone_shape;
    VAAID vaaid;
    gchar *sbuf;
    gint i;

    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    const gchar *counter_evt[] = {"100"};

    gint pos_x, pos_y;
    gint size_w, size_h,label_h, tmp;
    char buf[3][64];
    gchar lfBuf[4096];

    gint use_counter_cnt = 0;
    gint use_event_cnt = 0;

    radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
    radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
    radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
    radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);   
    
    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, DEFAULT_COMPONENT_WIDTH-20, 230 + 135 + 40 - 15 + 60); //2
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 5, 20);
    nfui_regi_post_event_callback(fixed, post_fixd_event_handler);

    pos_x = 20;
    pos_y = 0;

    if (opt & (1 << OPT_COUNTER_USE_RADIO))
    {
        fixed_title = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed_title, 420, 40);
        nfui_nfobject_show(fixed_title);
        nfui_nffixed_put((NFFIXED*)fixed, fixed_title, pos_x, pos_y);
        nfui_nfobject_modify_bg(fixed_title, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        
        use_counter_data = imalloc(sizeof(COUNTER_DATA_T));
        nfui_nfobject_set_alloc_data(fixed_title, "USE COUNTER", use_counter_data);
        
        pos_x = 0;
    
        obj = (NFOBJECT*)nfui_nfbutton_new();
        list = nfui_radio_button_get_group(NF_BUTTON(obj));
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, pos_x, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_radio_event_handler);
        
        pos_x += size_w;
                                                              
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USE COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 150, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, pos_x, pos_y);
        
        pos_x += 170;  

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_radio_button_add_group(NF_BUTTON(obj), list);
        nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, pos_x, pos_y+(40-size_h)/2);
        nfui_regi_post_event_callback(obj, post_radio_event_handler);
        
        pos_x += size_w;
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NOT USE COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 230, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_title, obj, pos_x, pos_y);
        
        pos_x = 20;
        pos_y += 60;

    }
    else
    {
        pos_x = 20;
        pos_y = 0;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COUNTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);

        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj, 290, 40+45);
        nfui_regi_post_event_callback(obj, post_active_check_event_handler);
        
        pos_x += 30;
        pos_y += 40;      
    }
    pos_x = 20;

    memset(buf[0], 0x00, sizeof(buf[0]));
    g_sprintf(buf[0], "%s", lookup_string("NAME"));
    strcat(buf[0], " : ");
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf[0], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
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
    
    if (use_counter_data) {
        use_counter_data->counter_obj[use_counter_cnt++] = obj;
        use_counter_data->cnt = use_counter_cnt;
    }
    
    pos_x = 20;
    pos_y += 45;

    memset(buf[1], 0x00, sizeof(buf[1]));
    g_sprintf(buf[1], "%s", lookup_string("DISPLAY COLOR"));
    strcat(buf[1], " : ");

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf[1], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
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

    if (use_counter_data) {
        use_counter_data->counter_obj[use_counter_cnt++] = obj;
        use_counter_data->cnt = use_counter_cnt;
    }

    if (opt & (1 << OPT_COUNTER_ACTIVE_CHECK))
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ACTIVE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 150, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  320, 40+45);
    }
    
    pos_x = 40;
    pos_y += 45;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_event_check_event_handler);

    if (use_counter_data) {
        use_counter_data->counter_obj[use_counter_cnt++] = obj;
        use_counter_data->cnt = use_counter_cnt;
    }

    use_event_data = imalloc(sizeof(COUNTER_DATA_T));
    nfui_nfobject_set_alloc_data(obj, "COUNTER EVENT", use_event_data);    

    pos_x += 30;

    memset(buf[2], 0x00, sizeof(buf[2]));
    nfutil_get_line_feed_string(lookup_string("EVENT COUNTING NUMBER TO MAKE EVENT NOTIFICATION"), 420, nffont_get_pango_font(NFFONT_SMALL_SEMI), buf[2], sizeof(buf[2]));

    strcat(buf[2], " : ");

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf[2], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 430, 60);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    
    pos_y += 60;

    obj = nfui_spinbutton_new_value_with_range(100, 0, 32767, 1);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
    nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nfobject_set_size(obj, 130, 30);
    nfui_nfobject_show(obj);                
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);  
    nfui_regi_post_event_callback(obj, post_event_spin_event_handler);
    
    if (use_event_data) {
        use_event_data->counter_obj[use_event_cnt++] = obj;
        use_event_data->cnt = use_event_cnt;
    }
    
    pos_x = 40;
    pos_y += 40;

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_reset_check_event_handler);
    
    if (use_event_data) {
        use_event_data->counter_obj[use_event_cnt++] = obj;
        use_event_data->cnt = use_event_cnt;
    }
    
    pos_x += 30;
    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(lookup_string("RESET EVENT COUNTING NUMBER AFTER EVENT NOTIFICATION"), 420, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 5);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 430, 60);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
    pos_y += 30;
    
    if (opt & (1 << OPT_COUNTER_SOURCE_COMBO))
    {
        pos_x = 20;
        pos_y += 45;
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COUNTER SOURCE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        
        pos_x = 40;
        pos_y += 40;
        
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCREASE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 120, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        
        pos_x += 120;
        
        obj = nfui_combobox_new(0, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_SMALL_1);
        nfui_nfobject_set_size(obj, 300, 30);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_source_up_combo_event_handler);

        if (use_counter_data) {
            use_counter_data->counter_obj[use_counter_cnt++] = obj;
            use_counter_data->cnt = use_counter_cnt;
        }

        pos_x = 40;
        pos_y += 40;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DECREASE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 120, 30);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed, obj,  pos_x, pos_y);
        
        pos_x += 120;

        obj = nfui_combobox_new(0, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_SMALL_1);
        nfui_nfobject_set_size(obj, 300, 30);
        nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_source_down_combo_event_handler);
        
        if (use_counter_data) {
            use_counter_data->counter_obj[use_counter_cnt++] = obj;
            use_counter_data->cnt = use_counter_cnt;        
        }
    }

    if (opt & (1 << OPT_COUNTER_SETUP_HELP))
    {
        pos_x = 20;
        pos_y = DEFAULT_COMPONENT_HEIGHT - 220 - 20;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("[?]", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, 40, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("What is the counter?", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
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

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
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

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
        nfui_nfobject_set_size(obj, DEFAULT_COMPONENT_WIDTH-pos_x-25, (160-tmp));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);
    }

    return 0;
}

gint vw_vca_rev_counter_component_show()
{

    return 0;
}

gint vw_vca_rev_counter_component_hide()
{

    return 0;
}

gint vw_vca_rev_counter_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_rev_counter_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

