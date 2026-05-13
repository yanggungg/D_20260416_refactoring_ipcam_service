/*
 * vw_vca_rev_schd.c
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

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "objects/nftile.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_vca_rev_schd.h"



////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_cam_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_dayObj = 0;
static NFOBJECT *g_tileObj = 0;

static VCASchedData g_vsd[GUI_CHANNEL_CNT];
static VCASchedData g_org_vsd[GUI_CHANNEL_CNT];

static gint g_day = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//
static void _init_schedule(void)
{
	DAL_get_vca_schd_data_all(g_vsd, GUI_CHANNEL_CNT);
	memcpy(g_org_vsd, g_vsd, sizeof(g_vsd));
}

static void _save_schedule(void)
{
    DAL_set_vca_schd_data_all(g_vsd, GUI_CHANNEL_CNT);
    memcpy(g_org_vsd, g_vsd, sizeof(g_vsd));
}


static void _set_schedule(gboolean expose)
{
	guint i, j;
	guint color;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		for (j = 0; j < 24; j++) {
			switch ( g_vsd[i].sched[j+(g_day*24)] ) {
				case '1':
					color = SELECT_STATE_COLOR_2;
					break;
				case '0':
				default:
					color = SELECT_STATE_COLOR_1;
					break;
			}
			if ( expose )
				nfui_tile_draw_color((NFTILE *)g_tileObj, color, i, j, i, j);
			else
				nfui_tile_no_draw_color((NFTILE *)g_tileObj, color, i, j, i, j);
		}
	}
}




////////////////////////////////////////////////////////////
//
// handler
//
static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
	}
	else if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
	}

	return FALSE;
}

static gboolean post_content_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable;
		GdkGC *gc;
		guint off_x, off_y;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(779));	// OFF
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1125, 
				off_y + 22,
				17, 17);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(780));	// ON
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1295, 
				off_y + 22,
				17, 17);

/*		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(781));	// EVENT
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1355, 
				off_y + 22,
				17, 17);
*/
		nfui_nfobject_gc_unref(gc);
	}
	else if((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY))
	{
    	gint i;
        gchar strBuf[STRING_SIZE_CAMTITLE];

    	for (i = 0; i < GUI_CHANNEL_CNT; i++)
    	{
    	    memset(strBuf, 0x00, sizeof(strBuf));
            var_get_camtitle(strBuf, i);
    		nfui_nflabel_set_text((NFLABEL*)g_cam_obj[i], strBuf);
    		nfui_signal_emit(g_cam_obj[i], GDK_EXPOSE, TRUE);
        }
	}
	else if(evt->type == GDK_DELETE)  
	{
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	}

	return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint day;
	gint i;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		g_day = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);	
		_set_schedule(TRUE);
	}
    return FALSE;
}

static void _draw_tile_area(NFTILE *tile, gint expose)
{
	gint i;
	guint color_n = 1;

	if (nfui_tile_drawable(NF_TILE(tile))) 
	{
		for (i = 0; i < 7*24; i++) 
		{
			switch(g_vsd->sched[i]) 
			{
				case '0': color_n = SELECT_STATE_COLOR_2; break;
				case '1': color_n = SELECT_STATE_COLOR_3; break;
				default: break;
			}

			if (expose) nfui_tile_draw_color(NF_TILE(tile), color_n, i/24, i%24, i/24, i%24);
			else		nfui_tile_no_draw_color(NF_TILE(tile), color_n, i/24, i%24, i/24, i%24);			
		}
	}
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
 {
	switch(evt->type) {
		case NFEVENT_TILE_INIT:

			printf("data %s \n",g_vsd[0].sched);
			printf("data %s \n",g_vsd[1].sched);
			printf("data %s \n",g_vsd[2].sched);
			printf("data %s \n",g_vsd[3].sched);
			printf("data %s \n",g_vsd[4].sched);
			printf("data %s \n",g_vsd[5].sched);
			printf("data %s \n",g_vsd[6].sched);
			printf("data %s \n",g_vsd[7].sched);
			_set_schedule(TRUE);
			break;

		case NFEVENT_TILE_START_SELECT:
			NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
			break;

		case NFEVENT_TILE_MOVE_SELECT:
			break;

		case NFEVENT_TILE_END_SELECT:
			{
				guint s_row = 0, s_col = 0;
				guint e_row = 0, e_col = 0;
				guint i, j;
				guint sched, color;
				gint x, y;

				gdk_display_get_pointer(gdk_display_get_default(),
						NULL, &x, &y, NULL);
				x = MIN(x, 1920 - 220);
				y = MIN(y, 1080 - 150);

				sched = VW_RecSched_Control_Page(g_curwnd, (guint)x, (guint)y);

				nfui_tile_get_selectArea(NF_TILE(obj),
						&s_row, &s_col, &e_row, &e_col);

				for(i = s_row; i <= e_row; i++) {
					for(j = s_col; j <= e_col; j++) {
						if ( sched == 2 ) {
							switch ( g_vsd[i].sched[j+(g_day*24)] ) {
								case '1':
									color = SELECT_STATE_COLOR_2;
									break;
								case '0':
								default:
									color = SELECT_STATE_COLOR_1;
									break;
							}
						}
						else {
							g_vsd[i].sched[j+(g_day*24)] = (gchar)('0' + sched);
							color = SELECT_STATE_COLOR_1 + sched;
						}
						nfui_tile_draw_color(NF_TILE(g_tileObj),
								color, i, j, i, j);
					}
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        if(memcmp(&g_vsd, &g_org_vsd, sizeof(g_vsd)))
        {
            memcpy(g_vsd, g_org_vsd, sizeof(g_vsd));
	        _set_schedule(TRUE);
        }
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {       
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if(memcmp(&g_vsd, &g_org_vsd, sizeof(g_vsd)))
        {
            _save_schedule();
        	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        VW_VCA_Rev_schd_tab_out_handler();
        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}





////////////////////////////////////////////////////////////
//
// protected interfaces 
//





////////////////////////////////////////////////////////////
//
// public interfaces
//

gint VW_VCA_Rev_schd_init_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *scrolled_fixed;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *tile;

	gchar *day[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	gchar *legend[] = {"OFF", "ON"};
	const gchar *icon[] = {IMG_CAMERA_OUTPUT_ICON_01, 
						IMG_CAMERA_OUTPUT_ICON_02,
						IMG_CAMERA_OUTPUT_ICON_03,
						IMG_CAMERA_OUTPUT_ICON_04,
						IMG_CAMERA_OUTPUT_ICON_05,
						IMG_CAMERA_OUTPUT_ICON_06,
						IMG_CAMERA_OUTPUT_ICON_07,
						IMG_CAMERA_OUTPUT_ICON_08,
						IMG_CAMERA_OUTPUT_ICON_09,
						IMG_CAMERA_OUTPUT_ICON_10,
						IMG_CAMERA_OUTPUT_ICON_11,
						IMG_CAMERA_OUTPUT_ICON_12,
						IMG_CAMERA_OUTPUT_ICON_13,
						IMG_CAMERA_OUTPUT_ICON_14,
						IMG_CAMERA_OUTPUT_ICON_15,
						IMG_CAMERA_OUTPUT_ICON_16,
						IMG_CAMERA_OUTPUT_ICON_17,
						IMG_CAMERA_OUTPUT_ICON_18,
						IMG_CAMERA_OUTPUT_ICON_19,
						IMG_CAMERA_OUTPUT_ICON_20,												
						IMG_CAMERA_OUTPUT_ICON_21,
						IMG_CAMERA_OUTPUT_ICON_22,
						IMG_CAMERA_OUTPUT_ICON_23,
						IMG_CAMERA_OUTPUT_ICON_24,
						IMG_CAMERA_OUTPUT_ICON_25,
						IMG_CAMERA_OUTPUT_ICON_26,
						IMG_CAMERA_OUTPUT_ICON_27,
						IMG_CAMERA_OUTPUT_ICON_28,
						IMG_CAMERA_OUTPUT_ICON_29,
						IMG_CAMERA_OUTPUT_ICON_30,
						IMG_CAMERA_OUTPUT_ICON_31,
						IMG_CAMERA_OUTPUT_ICON_32 };

	GdkColor tile_color[4] = {UX_COLOR(779), UX_COLOR(780), UX_COLOR(781), UX_COLOR(779)};
	gchar strBuf[32];
	gint i;

	gint scrolledfixed_h;

	g_curwnd = nfui_nfobject_get_top(parent);

	g_day = 0;
	_init_schedule();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	for(i=0; i<2; i++) {
		if(nftool_cur_language_is_japanese())
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(968));
		else			
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 130, 17);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 982 + ((i+1) * (155 + 17)), 22);
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 4, 0);

//combobox
    obj = nfui_combobox_new(day, 7, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_nfobject_set_size(obj, 154, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, 143, 0);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_dayObj = obj;


	for (i = 0; i < 24; i++) {
		sprintf(strBuf, "%02d", i);	
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_set_size(obj, 44, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 301 + (i * 46), 47);
	}

    if (GUI_CHANNEL_CNT > 16) scrolledfixed_h = 16*42+10;
    else scrolledfixed_h = GUI_CHANNEL_CNT*42;    	

	scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
	nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, scrolledfixed_h/3, 80, scrolledfixed_h/5);
	nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_8594A6)));
	nfui_nfobject_set_size(scrolled_fixed, 1480, scrolledfixed_h);
	nfui_nfobject_show(scrolled_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, scrolled_fixed, 4, 87);

	memset(strBuf, 0x00, sizeof(strBuf));
	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
		obj = nfui_nfimage_new(icon[i]);
		nfui_nfobject_show(obj);
		nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, 0, (i * 42));
		
        memset(strBuf, 0x00, sizeof(strBuf)); 
        var_get_camtitle(strBuf, i);
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_set_size(obj, 236, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 6);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, obj, 58, (i * 42));
		nfui_nfobject_show(obj);
		g_cam_obj[i] = obj;
	}

	// tile
	g_tileObj = nfui_tile_new(GUI_CHANNEL_CNT, 24);
	nfui_tile_set_color(NF_TILE(g_tileObj), NFTILE_STATE_SELECT, tile_color); 
	nfui_nfobject_set_size(g_tileObj, (46 * 24), (42 * GUI_CHANNEL_CNT));
	nfui_nfobject_show(g_tileObj);
	nfui_regi_post_event_callback(g_tileObj, post_tile_event_handler);
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, g_tileObj, 297, 0);
	
	obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	
	nfui_regi_post_event_callback(parent, post_page_event_handler);
	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);	

	uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);

    return 0;
}

gboolean VW_VCA_Rev_schd_tab_in_handler()
{

	return FALSE;
}

gboolean VW_VCA_Rev_schd_tab_out_handler()
{
    mb_type ret;

    if(memcmp(&g_vsd, &g_org_vsd, sizeof(g_vsd)))
    {
	    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
	    if(ret == NFTOOL_MB_OK) 
	    {
            _save_schedule();
        	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
        else if(ret == NFTOOL_MB_CANCEL)
        {
            memcpy(g_vsd, g_org_vsd, sizeof(g_vsd));
            _set_schedule(TRUE);
        }
    }

	return FALSE;
}

