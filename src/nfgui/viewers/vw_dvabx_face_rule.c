/*
 * vw_dvabx_rule.c
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
#include "viewers/objects/nflistbox.h"

#include "vw_dvabx_prop_internal.h"
#include "vw_dvabx_component.h"
#include "vw_vkeyboard.h"

#include "dvaa.h"
#include "dvaa_itx_face.h"

#include "nf_api_dlva.h"


////////////////////////////////////////////////////////////
//
// private data types
//

#define STR_UNASSIGNED          "Unassigned"
#define STR_UNREGISTERED        "Unregistered"
#define STR_SELECT_GROUP        "Please select a group."


////////////////////////////////////////////////////////////
//
// private variable
//

static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;

static NFOBJECT *g_zone_use_check[4];
static NFOBJECT *g_zone_name_label[4];
static NFOBJECT *g_zone_color_label[4];
static NFOBJECT *g_face_group_combo_obj;
static NFOBJECT *g_face_group_list_obj;

static NFOBJECT *g_thres_spin;

static DVAAID g_dvaaid = 0;



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

static gint _load_face_zone_data(DVA_COMPONENT_DATA_T *component_data, gint expose)
{
    ITX_FACEZONE_CONF conf;
    ITX_FACEZONE_SHAPE shape;
    gint i;

    AiAnalysisActData analysis_data;
    gint ch;

    fr_group_info *dev_group_info = 0;
    gint dev_group_cnt;

    gint color_idx;
    gchar strBuf[8];

    gchar *token_ptr;
    gint all_check = 1;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

    for (i = 0; i < 4; i++)
    {
        memset(&conf, 0x00, sizeof(ITX_FACEZONE_CONF));
        memset(&shape, 0x00, sizeof(ITX_FACEZONE_SHAPE));

        dvaa_itx_face_get_zone_conf(g_dvaaid, i, &conf);
        dvaa_itx_face_get_zone_shape(g_dvaaid, i, &shape); 

        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_zone_use_check[i], conf.active);
        nfui_nflabel_set_text((NFLABEL*)g_zone_name_label[i], shape.name);
        nfui_nfobject_modify_bg(g_zone_color_label[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(shape.color_idx));

        if (expose) nfui_signal_emit(g_zone_use_check[i], GDK_EXPOSE, TRUE);
        if (expose) nfui_signal_emit(g_zone_name_label[i], GDK_EXPOSE, TRUE);
        if (expose) nfui_signal_emit(g_zone_color_label[i], GDK_EXPOSE, TRUE);
    }
  
    nfui_listbox_delete_all(NF_LISTBOX(g_face_group_list_obj));
    nfui_combobox_remove_all((NFCOMBOBOX*)g_face_group_combo_obj);
    nfui_combobox_set_display_string((NFCOMBOBOX*)g_face_group_combo_obj, lookup_string(STR_SELECT_GROUP));

    memset(&analysis_data, 0x00, sizeof(AiAnalysisActData));
	DAL_get_aianalysis_act_data(&analysis_data, component_data->preview.ch);
    ch = component_data->preview.ch;

	if (analysis_data.dvabox_active) dev_group_info = nf_api_fr_group_list_get(component_data->aibox_addr, &dev_group_cnt);
    else dev_group_info = nf_api_aicam_fr_group_list_get(ch, &dev_group_cnt);

	for (i = 0; i < dev_group_cnt; i++) {
		nfui_combobox_append_data((NFCOMBOBOX*)g_face_group_combo_obj, dev_group_info[i].name);
	}
    nfui_combobox_append_data((NFCOMBOBOX*)g_face_group_combo_obj, lookup_string(STR_UNASSIGNED));
    nfui_combobox_append_data((NFCOMBOBOX*)g_face_group_combo_obj, lookup_string(STR_UNREGISTERED));

    dvaa_itx_face_get_zone_conf(g_dvaaid, 0, &conf);
	token_ptr = strtok_r(conf.group_filter, ",", &pnext);
    pbuf = pnext;

	while (token_ptr != 0)
	{
		g_message("%s, %d, token:%s", __FUNCTION__, __LINE__, token_ptr);

        if ((strcmp(token_ptr, STR_UNASSIGNED) == 0) || (strcmp(token_ptr, STR_UNREGISTERED) == 0))
        {
            nfui_listbox_set_text_single_column(NF_LISTBOX(g_face_group_list_obj), lookup_string(token_ptr));
        }
        else
        {
            nfui_listbox_set_text_single_column(NF_LISTBOX(g_face_group_list_obj), token_ptr);
        }
		token_ptr = strtok_r(pbuf, ",", &pnext);
        pbuf = pnext;
	}

    memset(strBuf, 0x00, sizeof(strBuf));
    sprintf(strBuf, "%d", conf.threshold);
    nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)g_thres_spin, strBuf);

    if (expose) nfui_signal_emit(g_thres_spin, GDK_EXPOSE, TRUE);

    return 0;
}




////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_zone_use_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        ITX_FACEZONE_CONF conf;
        ITX_ZONEID zoneid;

        if (g_zone_use_check[0] == obj) zoneid = 0;
        else if (g_zone_use_check[1] == obj) zoneid = 1;
        else if (g_zone_use_check[2] == obj) zoneid = 2;
        else if (g_zone_use_check[3] == obj) zoneid = 3;
        else return FALSE;

        if (nfui_check_button_get_active((NFCHECKBUTTON*)obj)) 
        {
            dvaa_itx_face_get_zone_conf(g_dvaaid, zoneid, &conf);
            conf.active = 1;            
            dvaa_itx_face_set_zone_conf(g_dvaaid, zoneid, &conf);
        }
        else 
        {
            dvaa_itx_face_get_zone_conf(g_dvaaid, zoneid, &conf);
            conf.active = 0;
            dvaa_itx_face_set_zone_conf(g_dvaaid, zoneid, &conf);
        }
    }

    return FALSE;
}

static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;    
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *name = NULL;
    guint x, y;

    ITX_FACEZONE_SHAPE shape;
    ITX_ZONEID zoneid;

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

        if (g_zone_name_label[0] == obj) zoneid = 0;
        else if (g_zone_name_label[1] == obj) zoneid = 1;
        else if (g_zone_name_label[2] == obj) zoneid = 2;
        else if (g_zone_name_label[3] == obj) zoneid = 3;
        else return FALSE;

        name = VirtualKey_Open(top, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_ITXSTYLE_TITLE);
        if(name) {            
            nfui_nflabel_set_text((NFLABEL*)obj, name);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            dvaa_itx_face_get_zone_shape(g_dvaaid, zoneid, &shape);            
            strcpy(shape.name, name);
            dvaa_itx_face_set_zone_shape(g_dvaaid, zoneid, &shape);

            ifree(name);
            name = NULL;
        }
    }

    return FALSE;
}

static gboolean post_dis_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    NFOBJECT *top;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    guint x, y;
    GdkColor gdk_color;
    guint color;
    gint color_idx;

    ITX_FACEZONE_SHAPE shape;
    ITX_ZONEID zoneid;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {   
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

        if (g_zone_color_label[0] == obj) zoneid = 0;
        else if (g_zone_color_label[1] == obj) zoneid = 1;
        else if (g_zone_color_label[2] == obj) zoneid = 2;
        else if (g_zone_color_label[3] == obj) zoneid = 3;
        else return FALSE;

        gdk_color = UX_COLOR(0);
        
        if (Color_Popup_Open((NFWINDOW *)obj, &gdk_color, (guint)x, (guint)y) == 0) return FALSE;
        
        color = (gdk_color.red >> 8) & 0x000000ff;
        color |= (gdk_color.green) & 0x0000ff00;
        color |= (gdk_color.blue << 8) & 0x00ff0000;
        color_idx = _convert_rgb_color_to_coloridx(color);

        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(color_idx));
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        dvaa_itx_face_get_zone_shape(g_dvaaid, zoneid, &shape);
        shape.color_idx = color_idx;
        dvaa_itx_face_set_zone_shape(g_dvaaid, zoneid, &shape);
    }

    return FALSE;
}

static gboolean post_group_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        ITX_FACEZONE_CONF conf;
		gchar *group_text = nfui_combobox_get_value(NF_COMBOBOX(obj));
        gchar *list_text;
        gchar group_filter[1024];

        gint list_cnt;
        gint i;

        list_cnt = nfui_listbox_get_box_count(NF_LISTBOX(g_face_group_list_obj));

        for (i = 0; i < list_cnt; i++)
        {
            list_text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_face_group_list_obj), i, 0);
            if (!strcmp(group_text, list_text)) {
                nftool_mbox(g_curwnd, "NOTICE", "The same filter already exists.", NFTOOL_MB_OK);
                return FALSE;
            }				
        }      

		nfui_listbox_set_text(NF_LISTBOX(g_face_group_list_obj), &group_text);
		nfui_signal_emit(g_face_group_list_obj, GDK_EXPOSE, TRUE);

        list_cnt = nfui_listbox_get_box_count(NF_LISTBOX(g_face_group_list_obj));
        for (i = 0; i < list_cnt; i++)
        {
            list_text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_face_group_list_obj), i, 0);

            if (strcmp(list_text, lookup_string(STR_UNASSIGNED)) == 0)
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", STR_UNASSIGNED);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", STR_UNASSIGNED);
            }
            else if (strcmp(list_text, lookup_string(STR_UNREGISTERED)) == 0)
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", STR_UNREGISTERED);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", STR_UNREGISTERED);
            }
            else
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", list_text);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", list_text);
            }
        }  

        for (i = 0; i < 4; i++)
        {
            dvaa_itx_face_get_zone_conf(g_dvaaid, i, &conf);
            memset(conf.group_filter, 0x00, sizeof(conf.group_filter));
            snprintf(conf.group_filter, sizeof(conf.group_filter)-1, "%s", group_filter);
            dvaa_itx_face_set_zone_conf(g_dvaaid, i, &conf);
        }
    }
    
    return FALSE;
}

static gboolean post_list_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
        ITX_FACEZONE_CONF conf;
        gchar *list_text;
        gchar group_filter[1024];

		gint idx;
        gint list_cnt;
        gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_face_group_list_obj);
		if (idx < 0) return FALSE;

		g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, idx);
		nfui_listbox_delete((NFLISTBOX*)g_face_group_list_obj, idx);
		nfui_signal_emit(g_face_group_list_obj, GDK_EXPOSE, TRUE);

        memset(group_filter, 0x00, sizeof(group_filter));

        list_cnt = nfui_listbox_get_box_count(NF_LISTBOX(g_face_group_list_obj));
        for (i = 0; i < list_cnt; i++)
        {
            list_text = nfui_listbox_get_text_of_list(NF_LISTBOX(g_face_group_list_obj), i, 0);

            if (strcmp(list_text, lookup_string(STR_UNASSIGNED)) == 0)
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", STR_UNASSIGNED);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", STR_UNASSIGNED);
            }
            else if (strcmp(list_text, lookup_string(STR_UNREGISTERED)) == 0)
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", STR_UNREGISTERED);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", STR_UNREGISTERED);
            }
            else
            {
                if (i == 0) snprintf(group_filter, sizeof(group_filter)-1, "%s", list_text);
                else snprintf(group_filter+strlen(group_filter), sizeof(group_filter)-strlen(group_filter)-1, ",%s", list_text);
            }
        }  

        for (i = 0; i < 4; i++)
        {
            dvaa_itx_face_get_zone_conf(g_dvaaid, i, &conf);
            memset(conf.group_filter, 0x00, sizeof(conf.group_filter));
            snprintf(conf.group_filter, sizeof(conf.group_filter)-1, "%s", group_filter);
            dvaa_itx_face_set_zone_conf(g_dvaaid, i, &conf);
        }
	}

	return FALSE;
}

static gboolean post_threshold_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        ITX_FACEZONE_CONF conf;
        gchar *spin_value;
        gint i;

        spin_value = nfui_spin_button_get_text((NFSPINBUTTON*)g_thres_spin);

        for (i = 0; i < 4; i++)
        {
            dvaa_itx_face_get_zone_conf(g_dvaaid, i, &conf);
            conf.threshold = atoi(spin_value);
            dvaa_itx_face_set_zone_conf(g_dvaaid, i, &conf);
        }        
    }

    return FALSE;
}

static gboolean post_zone_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_filter_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
gint _dvabx_face_rule_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint size_w, size_h;
    gint i;

	guint lc_size[] = {370, };
	guint li_size_w, li_size_h;


	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);
    g_parent = parent;


    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, 505, 230);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 5, 20);
    nfui_regi_post_event_callback(fixed, post_zone_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
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
    nfui_regi_post_event_callback(obj, post_zone_use_check_event_handler);
    g_zone_use_check[0] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    g_zone_name_label[0] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_FF0000)));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 34, 34);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+360, pos_y);
    nfui_regi_post_event_callback(obj, post_dis_color_event_handler);
    g_zone_color_label[0] = obj;

    pos_y += 38;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_zone_use_check_event_handler);
    g_zone_use_check[1] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    g_zone_name_label[1] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_FF3F00)));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 34, 34);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+360, pos_y);    
    nfui_regi_post_event_callback(obj, post_dis_color_event_handler);
    g_zone_color_label[1] = obj;

    pos_y += 38;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_zone_use_check_event_handler);
    g_zone_use_check[2] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("ZONE3", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    g_zone_name_label[2] = obj;   

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_FF7F00)));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 34, 34);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+360, pos_y);
    nfui_regi_post_event_callback(obj, post_dis_color_event_handler);
    g_zone_color_label[2] = obj;

    pos_y += 38;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y+(34-size_h)/2);
    nfui_regi_post_event_callback(obj, post_zone_use_check_event_handler);
    g_zone_use_check[3] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("ZONE4", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+36, pos_y);  
    nfui_regi_post_event_callback(obj, post_name_label_event_handler);
    g_zone_name_label[3] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_FFBF00)));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, 34, 34);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+360, pos_y);    
    nfui_regi_post_event_callback(obj, post_dis_color_event_handler);
    g_zone_color_label[3] = obj;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, 505, 330);
    nfui_nfobject_show(fixed);
    nfui_nffixed_put((NFFIXED*)parent, fixed, 5, 280);
    nfui_regi_post_event_callback(fixed, post_filter_fixed_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GROUP FILTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 475, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 0);    

    pos_x = 10;
    pos_y = 50;

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
    nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, 475, 40);		
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_group_combo_event_handler);
    g_face_group_combo_obj = obj;

	nfui_combobox_set_display_string(obj, lookup_string(STR_SELECT_GROUP));

	pos_y += 44;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, 475, 180);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    g_face_group_list_obj = obj;

	pos_y += 182;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 180);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+295, pos_y);
    nfui_regi_post_event_callback(obj, post_list_delete_button_event_handler);

    pos_x = 15;
    pos_y = 650;

/*
    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y+(34-size_h)/2);
*/

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONFIDENCE THRESHOLD", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 320, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y); 

    obj = nfui_spinbutton_new_value_with_range(1, 1, 100, 1);  
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
    nfui_spin_button_set_pango_font((NFSPINBUTTON*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_nfobject_set_size(obj, 85, 30);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+360, pos_y);  
    nfui_regi_post_event_callback(obj, post_threshold_spin_event_handler);
    g_thres_spin = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("%", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 20, 34);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x+445, pos_y);

    return 0;
}

void _dvabx_face_rule_show(NFOBJECT *parent)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;

    top = nfui_nfobject_get_top(parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    g_dvaaid = dvaa_get_face_dvaaid(component_data->preview.ch);
    _load_face_zone_data(component_data, 0);

    nfui_nfobject_show(parent);
}

void _dvabx_face_rule_hide(NFOBJECT *parent)
{

	nfui_nfobject_hide(parent);
}

void _dvabx_face_rule_cancel_data(NFOBJECT *parent)
{

}
