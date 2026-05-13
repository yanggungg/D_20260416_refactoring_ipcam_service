/*
 * vw_timeline_deeplearning_option.c
 * 	- timeline dlva viewer
 *	- dependencies : vw_timeline_deeplearning.c
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 11, 2018
 *
 */

#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

#include "ix_mem.h"
#include "modules/ssm.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"

#include "vw_vkeyboard.h"
#include "vw_timeline_deeplearning_option.h"


#define MKB_TAB_SUBGROUP_TITLE_NAME     "mkb_tab_subgroup_title_%d"

#define POPUP_WIN_SIZE_W	(570)
#define POPUP_WIN_SIZE_H	(680)

#define POPUP_POS_X			(DISPLAY_ACTIVE_WIDTH - POPUP_WIN_SIZE_W - 196)
#define POPUP_POS_Y			(DISPLAY_ACTIVE_HEIGHT - POPUP_WIN_SIZE_H - 112)



////////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;

static DVAFILTER_DATA_T *g_dvafilter_data;

static NFOBJECT *g_ch_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_idz_subfixed;
static NFOBJECT *g_ipz_subfixed;

static gint g_init_channel = -1;
static gint g_cur_channel = -1;

static NFOBJECT *g_idz_human_all_obj;
static NFOBJECT *g_idz_vehicle_all_obj;
static NFOBJECT *g_idz_vehicle_car_obj;
static NFOBJECT *g_idz_vehicle_bus_obj;
static NFOBJECT *g_idz_vehicle_bike_obj;
static NFOBJECT *g_idz_animal_all_obj;

static NFOBJECT *g_ipz_vehicle_car_obj;
static NFOBJECT *g_ipz_vehicle_bus_obj;
static NFOBJECT *g_ipz_vehicle_bike_obj;



////////////////////////////////////////////////////////////////
//
// private interfaces
//

static gint _draw_activation()
{
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (g_dvafilter_data[i].active_type != -1) 
		{
			nfui_nfobject_enable(g_ch_obj[i]);

			if (g_init_channel == -1) {
				nfui_radio_button_set_toggled(NF_BUTTON(g_ch_obj[i]), TRUE);
				g_init_channel = i;
			}
		}
	}

	return 0;
}

static gint _draw_idz_detection_type_fixed(gchar *db_list, gchar *shown_list)
{
	gboolean active_val;

	if (strstr(db_list, "person:1")) 
	{
		nfui_nfobject_enable(g_idz_human_all_obj);

		if (strstr(shown_list, "person:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_human_all_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_human_all_obj, FALSE);
	}

	if (strstr(db_list, "car:1")) 
	{
		nfui_nfobject_enable(g_idz_vehicle_car_obj);

		if (strstr(shown_list, "car:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_car_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_car_obj, FALSE);
	}

	if (strstr(db_list, "bus:1")) 
	{
		nfui_nfobject_enable(g_idz_vehicle_bus_obj);

		if (strstr(shown_list, "bus:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_bus_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_bus_obj, FALSE);
	}

	if (strstr(db_list, "bike:1")) 
	{
		nfui_nfobject_enable(g_idz_vehicle_bike_obj);

		if (strstr(shown_list, "bike:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_bike_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_bike_obj, FALSE);
	}

	if (strstr(db_list, "car:1") || strstr(db_list, "bus:1") || strstr(db_list, "bike:1")) 
	{
		nfui_nfobject_enable(g_idz_vehicle_all_obj);

		active_val = TRUE;

		if (strstr(db_list, "car:1") && !strstr(shown_list, "car:1")) active_val = FALSE;
		if (strstr(db_list, "bus:1") && !strstr(shown_list, "bus:1")) active_val = FALSE;
		if (strstr(db_list, "bike:1") && !strstr(shown_list, "bike:1")) active_val = FALSE;

		nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_vehicle_all_obj, active_val);
	}

	if (strstr(db_list, "animal:1")) 
	{
		nfui_nfobject_enable(g_idz_animal_all_obj);

		if (strstr(shown_list, "animal:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_animal_all_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_idz_animal_all_obj, FALSE);
	}

	return 0;
}

static gint _draw_ipz_detection_type_fixed(gchar *db_list, gchar *shown_list)
{
	gboolean active_val;

	if (strstr(db_list, "car:1")) 
	{
		nfui_nfobject_enable(g_ipz_vehicle_car_obj);

		if (strstr(shown_list, "car:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_car_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_car_obj, FALSE);
	}

	if (strstr(db_list, "bus:1")) 
	{
		nfui_nfobject_enable(g_ipz_vehicle_bus_obj);

		if (strstr(shown_list, "bus:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_bus_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_bus_obj, FALSE);
	}

	if (strstr(db_list, "bike:1")) 
	{
		nfui_nfobject_enable(g_ipz_vehicle_bike_obj);

		if (strstr(shown_list, "bike:1")) nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_bike_obj, TRUE);
		else nfui_check_button_set_active((NFCHECKBUTTON*)g_ipz_vehicle_bike_obj, FALSE);
	}

	return 0;
}

static gint _draw_detection_type_fixed(gint ch)
{
    if (ch == -1) return -1;

	if (g_dvafilter_data[ch].active_type == 0)
	{
		_draw_idz_detection_type_fixed(g_dvafilter_data[ch].db_list, g_dvafilter_data[ch].shown_list);

		nfui_nfobject_show(g_idz_subfixed);
		nfui_nfobject_hide(g_ipz_subfixed);
	}
	else
	{
		_draw_ipz_detection_type_fixed(g_dvafilter_data[ch].db_list, g_dvafilter_data[ch].shown_list);

		nfui_nfobject_show(g_ipz_subfixed);
		nfui_nfobject_hide(g_idz_subfixed);
	}

	return 0;
}


////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_channel_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, ret_val;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);
		_draw_detection_type_fixed(index);

		nfui_signal_emit(g_ipz_subfixed, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_idz_subfixed, GDK_EXPOSE, TRUE);		

		g_cur_channel = index;
	}

	return FALSE;
} 

static gboolean post_idz_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
		gboolean active_val;

		active_val = TRUE;

		if (!nfui_nfobject_is_disabled(g_idz_vehicle_bike_obj) && !nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj))) active_val = FALSE;
		if (!nfui_nfobject_is_disabled(g_idz_vehicle_bus_obj) && !nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj))) active_val = FALSE;
		if (!nfui_nfobject_is_disabled(g_idz_vehicle_car_obj) && !nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj))) active_val = FALSE;

		if (!nfui_nfobject_is_disabled(g_idz_vehicle_all_obj))
		{
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_idz_vehicle_all_obj, active_val);
			nfui_signal_emit(g_idz_vehicle_all_obj, GDK_EXPOSE, TRUE);
		}

		g_snprintf(g_dvafilter_data[g_cur_channel].shown_list, 256, "[person:%d],[bike:%d],[bus:%d],[car:%d],[animal:%d]", 
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_human_all_obj)),        
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj)),			
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj)),
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj)),
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_animal_all_obj)));
    }

    return FALSE;
}

static gboolean post_idz_vehicle_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
		gboolean active_val;

		active_val = nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_all_obj));

		if (!nfui_nfobject_is_disabled(g_idz_vehicle_bike_obj)) {
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_idz_vehicle_bike_obj, active_val);
			nfui_signal_emit(g_idz_vehicle_bike_obj, GDK_EXPOSE, TRUE);
		}

		if (!nfui_nfobject_is_disabled(g_idz_vehicle_bus_obj)) {
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_idz_vehicle_bus_obj, active_val);
			nfui_signal_emit(g_idz_vehicle_bus_obj, GDK_EXPOSE, TRUE);
		}

		if (!nfui_nfobject_is_disabled(g_idz_vehicle_car_obj)) {
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_idz_vehicle_car_obj, active_val);
			nfui_signal_emit(g_idz_vehicle_car_obj, GDK_EXPOSE, TRUE);
		}		

		g_snprintf(g_dvafilter_data[g_cur_channel].shown_list, 256, "[person:%d],[bike:%d],[bus:%d],[car:%d],[animal:%d]", 
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_human_all_obj)),        
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bike_obj)),			
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_bus_obj)),
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_vehicle_car_obj)),
			nfui_check_button_get_active(NF_CHECKBUTTON(g_idz_animal_all_obj)));
    }

    return FALSE;
}

static gboolean post_ipz_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
		g_snprintf(g_dvafilter_data[g_cur_channel].shown_list, 256, "[bike:%d],[bus:%d],[car:%d]", 
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_vehicle_bike_obj)),
        	nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_vehicle_bus_obj)),
			nfui_check_button_get_active(NF_CHECKBUTTON(g_ipz_vehicle_car_obj)));
    }

    return FALSE;
}

static gboolean post_close_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_channel_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);            
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);
	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h-40);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_type_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);            
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);
	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h-40);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	switch (evt->type) 
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
			topwin = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(topwin);	
		}
		break;

		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
		break;

		default:
		break;
	}

	return FALSE;
}


////////////////////////////////////////////////////////////////
//
// public interfaces
//

void vw_timeline_deeplearning_option_open(NFWINDOW *parent, DVAFILTER_DATA_T *pfilter)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *ch_subfixed;
	NFOBJECT *obj;

	gchar strBuf[64];
	guint size_w, size_h;
	gint i;

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	g_dvafilter_data = pfilter;

	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);


	win = (NFOBJECT*)nfui_nfwindow_new(parent, POPUP_POS_X, POPUP_POS_Y, POPUP_WIN_SIZE_W, POPUP_WIN_SIZE_H);	
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_regi_post_event_callback(win, post_win_event_handler);
	g_curwnd = win;	

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, POPUP_WIN_SIZE_W, POPUP_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI DETECTION FILTER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, POPUP_WIN_SIZE_W-8, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	ch_subfixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(ch_subfixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(ch_subfixed, 200, POPUP_WIN_SIZE_H-120);
	nfui_nfobject_show(ch_subfixed);	
	nfui_nffixed_put((NFFIXED*)fixed, ch_subfixed, 10, 60);
	nfui_regi_post_event_callback(ch_subfixed, post_channel_fixed_event_handler);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_channel_radio_event_handler);
		nfui_nffixed_put((NFFIXED*)ch_subfixed, obj, 8, 5+(34-size_h)/2 + 34*i);
		g_ch_obj[i] = obj;

		if (i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		}
		else {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "CAM %d", i+1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 150, 34);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)ch_subfixed, obj, 40, 5+34*i);
	}

	g_idz_subfixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(g_idz_subfixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(g_idz_subfixed, POPUP_WIN_SIZE_W-230, POPUP_WIN_SIZE_H-120);
	nfui_nfobject_show(g_idz_subfixed);	
	nfui_nffixed_put((NFFIXED*)fixed, g_idz_subfixed, 220, 60);
	nfui_regi_post_event_callback(g_idz_subfixed, post_type_fixed_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, g_idz_subfixed->width);
    nf_ui_create_image_button_method(strBuf, g_idz_subfixed->width, IMG_TAB_POPUP_TITLE_L, IMG_TAB_POPUP_TITLE_M, IMG_TAB_POPUP_TITLE_R);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "INTRUSION DETECTION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 0, 0);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 10, 60+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_group_event_handler);
	g_idz_human_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HUMAN DETECTION", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 42, 60);
    nfui_nfobject_show(obj); 	

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 10, 110+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_vehicle_all_event_handler);
	g_idz_vehicle_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VEHICLE DETECTION", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 42, 110);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 40, 144+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_group_event_handler);
	g_idz_vehicle_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 72, 144);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 40, 178+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_group_event_handler);
	g_idz_vehicle_bus_obj = obj;

	if(ivsc.vendor_code == 43)
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS AND FULL-SIZE CAR", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 72, 178);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 40, 212+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_group_event_handler);
	g_idz_vehicle_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 72, 212);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 10, 262+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_idz_group_event_handler);
	g_idz_animal_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANIMAL DETECTION", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_idz_subfixed, obj, 42, 262);
    nfui_nfobject_show(obj);

	g_ipz_subfixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(g_ipz_subfixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(g_ipz_subfixed, POPUP_WIN_SIZE_W-230, POPUP_WIN_SIZE_H-120);
	nfui_nfobject_hide(g_ipz_subfixed);	
	nfui_nffixed_put((NFFIXED*)fixed, g_ipz_subfixed, 220, 60);
	nfui_regi_post_event_callback(g_ipz_subfixed, post_type_fixed_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, g_ipz_subfixed->width);
    nf_ui_create_image_button_method(strBuf, g_ipz_subfixed->width, IMG_TAB_POPUP_TITLE_L, IMG_TAB_POPUP_TITLE_M, IMG_TAB_POPUP_TITLE_R);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "ILLEGAL PARKING");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 0, 0);	

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 40, 60+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);
	g_ipz_vehicle_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 72, 60);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 40, 94+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);
	g_ipz_vehicle_bus_obj = obj;

	if(ivsc.vendor_code == 43)
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS AND FULL-SIZE CAR", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BUS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 72, 94);
    nfui_nfobject_show(obj);

    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_SMALL);
    nfui_check_get_size(obj, &size_w, &size_h);
	nfui_nfobject_disable(obj);
	nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 40, 128+(34-size_h)/2);
	nfui_regi_post_event_callback(obj, post_ipz_group_event_handler);
	g_ipz_vehicle_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 160, 34);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)g_ipz_subfixed, obj, 72, 128);
    nfui_nfobject_show(obj);

	obj = nftool_normal_button_create_popup_type2("CLOSE", 150);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (POPUP_WIN_SIZE_W-150)/2, POPUP_WIN_SIZE_H-48);
	nfui_regi_post_event_callback(obj, post_close_btn_event_handler);

	g_init_channel = -1;

	_draw_activation();
	_draw_detection_type_fixed(g_init_channel);
	g_cur_channel = g_init_channel;

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());
	
	gtk_main();

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		g_message(">>>> %s, %d, ch:%d, shown_list:%s", __FUNCTION__, __LINE__, i, g_dvafilter_data[i].shown_list);
	}

	nfui_page_close(PGID_POPUPWND, win);
}
